/* SPDX-License-Identifier: GPL-2.0 */
#include <linux/genhd.h>
#include <linux/bio.h>
#include <linux/blkdev.h>
#include "blk-filter-internal.h"
#include <linux/rwsem.h>

struct blk_filter_ctx{
	struct blk_filter* filter;
	/*
	 * Reserved for extension
	*/
};

DECLARE_RWSEM(blk_filter_ctx_list_lock);
struct blk_filter_ctx* blk_filter_ctx_list[BLK_FILTER_ALTITUDE_MAX] = { 0 };

static inline struct blk_filter_ctx* _get_ctx(size_t altitude)
{
	return blk_filter_ctx_list[altitude-1];
}

static inline void _set_ctx(size_t altitude, struct blk_filter_ctx* ctx)
{
	blk_filter_ctx_list[altitude-1] = ctx;	
}

static struct blk_filter_ctx* _blk_ctx_new(struct blk_filter* filter)
{
	struct blk_filter_ctx* ctx = kzalloc(sizeof(struct blk_filter_ctx), GFP_KERNEL);
	if (!ctx)
		return ctx;

	ctx->filter = filter;

	return ctx;
}

static int _blk_ctx_link(struct blk_filter_ctx* ctx, size_t altitude)
{
	int result = 0;
	if (altitude > BLK_FILTER_ALTITUDE_MAX)
		return -ENOENT;

	down_write(&blk_filter_ctx_list_lock);

	if (_get_ctx(altitude))
		result = -EEXIST;
	else
		_set_ctx(altitude, ctx);

	up_write(&blk_filter_ctx_list_lock);

	return result;
}

static int _blk_ctx_unlink(struct blk_filter_ctx* ctx)
{
	int result = -EEXIST;
	size_t altitude=BLK_FILTER_ALTITUDE_MIN;

	down_write(&blk_filter_ctx_list_lock);
	
	for (; altitude<=BLK_FILTER_ALTITUDE_MAX; ++altitude) {
		if (_get_ctx(altitude) && (_get_ctx(altitude) == ctx)) {
			_set_ctx(altitude, NULL);
			result = 0;
			break;
		}
	}

	up_write(&blk_filter_ctx_list_lock);

	return result;
}

/**
 * blk_filter_disk_add() - Notify filters when a new disk is added.
 * @disk: The new disk.
 */
void blk_filter_disk_add(struct gendisk *disk)
{
	size_t altitude=BLK_FILTER_ALTITUDE_MIN;

	pr_warn("blk-filter: add disk [%s].\n", disk->disk_name);

	down_read(&blk_filter_ctx_list_lock);

	for (; altitude<=BLK_FILTER_ALTITUDE_MAX; ++altitude) {
		struct blk_filter_ctx* ctx = _get_ctx(altitude);
		if (ctx && ctx->filter->ops && ctx->filter->ops->disk_add)
			ctx->filter->ops->disk_add(disk);
	}

	up_read(&blk_filter_ctx_list_lock);	
}

/**
 * blk_filter_disk_del() - Notify filters when the disk is deleted.
 * @disk: The disk to delete.
 */
void blk_filter_disk_del(struct gendisk *disk)
{
	size_t altitude=BLK_FILTER_ALTITUDE_MIN;

	pr_warn("blk-filter: del disk [%s].\n", disk->disk_name);

	down_read(&blk_filter_ctx_list_lock);

	for (; altitude<=BLK_FILTER_ALTITUDE_MAX; ++altitude) {
		struct blk_filter_ctx* ctx = _get_ctx(altitude);
		if (ctx && ctx->filter->ops && ctx->filter->ops->disk_del)
			ctx->filter->ops->disk_del(disk);
	}

	up_read(&blk_filter_ctx_list_lock);	
}

/**
 * blk_filter_disk_release() - Notify filters when the disk is released.
 * @disk: The disk to release.
 */
void blk_filter_disk_release(struct gendisk *disk)
{
	size_t altitude=BLK_FILTER_ALTITUDE_MAX;

	pr_warn("blk-filter: release disk [%s].\n", disk->disk_name);

	down_read(&blk_filter_ctx_list_lock);

	for (; altitude<=BLK_FILTER_ALTITUDE_MIN; --altitude) {
		struct blk_filter_ctx* ctx = _get_ctx(altitude);
		if (ctx && ctx->filter->ops && ctx->filter->ops->disk_release)
			ctx->filter->ops->disk_release(disk);
	}

	up_read(&blk_filter_ctx_list_lock);	
}

/**
 * blk_filter_submit_bio_altitude() - Send bio for porcessing to specific filter.
 * @altitude: The filter altitude.
 * @bio: The new bio for block I/O layer.
 * 
 * Return: Bio submitting result, like for submit_bio function.
 */
blk_qc_t blk_filter_submit_bio_altitude(size_t altitude, struct bio *bio)
{
	blk_qc_t ret;
	bool bypass = true;

	down_read(&blk_filter_ctx_list_lock);
	while (altitude >= BLK_FILTER_ALTITUDE_MIN) {
		struct blk_filter_ctx* ctx = _get_ctx(altitude);
		if (ctx && ctx->filter->ops && ctx->filter->ops->submit_bio) {
			ret = ctx->filter->ops->submit_bio(bio);
			bypass = false;
			break;
		}
		--altitude;
	}
	up_read(&blk_filter_ctx_list_lock);	

	if (bypass)
		ret = submit_bio_noacct(bio);

	return ret;
}

/**
 * blk_filter_submit_bio() - Send new bio to filters for processing.
 * @bio: The new bio for block I/O layer.
 * 
 * Return: Bio submitting result, like for submit_bio function.
 */
blk_qc_t blk_filter_submit_bio(struct bio *bio)
{
	return blk_filter_submit_bio_altitude(BLK_FILTER_ALTITUDE_MAX, bio);
}

/**
 * blk_filter_register() - Create new block I/O layer filter.
 * @filter: The filter description structure.
 * 
 * Return: Zero if the filter was registered successfully or an error code if it failed.
 */
int blk_filter_register(struct blk_filter* filter)
{
	int result = 0;
	struct blk_filter_ctx* ctx;

	pr_warn("blk-filter: register filter [%s].\n", filter->name);

	ctx =_blk_ctx_new(filter);
	if (!ctx)
		return -ENOMEM;

	result = _blk_ctx_link(ctx, filter->altitude);
	if (result)
		goto failed;

	filter->blk_filter_ctx = (void*)ctx;
	return 0;

failed:
	kfree(ctx);
	return result;
}
EXPORT_SYMBOL(blk_filter_register);

/**
 * blk_filter_unregister() - Remove existing block I/O layer filter.
 * @filter: The filter description structure.
 * 
 * Return: Zero if the filter was removed successfully or an error code if it failed.
 */
int blk_filter_unregister(struct blk_filter* filter)
{
	int result = 0;
	struct blk_filter_ctx* ctx;
	
	pr_warn("blk-filter: unregister filter [%s].\n", filter->name);

	ctx = (struct blk_filter_ctx*)filter->blk_filter_ctx;

	result = _blk_ctx_unlink(ctx);
	if (result == 0)
		kfree(ctx);

	return result;
}
EXPORT_SYMBOL(blk_filter_unregister);

/**
 * blk_filter_check_altitude() - Checking that altitude is free.
 * @altitude: The filter description structure.
 * 
 * Return: NULL if the altitude is free or the name of the module registered at this altitude.
 */
const char* blk_filter_check_altitude(size_t altitude) 
{
	struct blk_filter_ctx* ctx = _get_ctx(altitude);
	if (!ctx)
		return NULL;

	return ctx->filter->name;
}
EXPORT_SYMBOL(blk_filter_check_altitude);

static void _attach_fn(struct gendisk* disk, void* _ctx) 
{
	struct blk_filter* filter = (struct blk_filter*)_ctx;
	if (filter->ops && filter->ops->disk_add)
		filter->ops->disk_add(disk);
}

/**
 * blk_filter_attach_disks() - Enumerate all existing disks and call disk_add callback for each of them.
 * @filter: The filter description structure.
 * 
 * Return: Zero if the existing disks was attached successfully or an error code if it failed.
 */
int blk_filter_attach_disks(struct blk_filter* filter) 
{
	return disk_enumerate(_attach_fn, filter);
}
EXPORT_SYMBOL(blk_filter_attach_disks);

/**
 * blk_filter_submit_bio_next() - Send a bio to the lower filters for processing.
 * @bio: The bio for block I/O layer.
 * 
 * Return: Bio submitting result, like for submit_bio function.
 */
blk_qc_t blk_filter_submit_bio_next(struct blk_filter* filter, struct bio *bio) 
{
	return blk_filter_submit_bio_altitude(filter->altitude-1, bio);
}
EXPORT_SYMBOL(blk_filter_submit_bio_next);
