#include <linux/genhd.h>
#include <linux/bio.h>
#include <linux/blkdev.h>
#include "blk-filter-internal.h"
#include <linux/rwsem.h>

///////////////////////////////////////////////////////////////////////////////
// Internal declaration, structure and variables

typedef struct blk_filter_ctx
{
	struct list_head link;
	blk_filter_t* filter;

} blk_filter_ctx_t;

DECLARE_RWSEM(blk_filter_ctx_list_lock);
blk_filter_ctx_t* blk_filter_ctx_list[BLK_FILTER_ALTITUDE_MAX] = {0};

static inline blk_filter_ctx_t* _get_ctx(size_t altitude)
{
	return blk_filter_ctx_list[altitude-1];
}

static inline void _set_ctx(size_t altitude, blk_filter_ctx_t* ctx)
{
	blk_filter_ctx_list[altitude-1] = ctx;	
}

///////////////////////////////////////////////////////////////////////////////
// Internal functions

static blk_filter_ctx_t* _blk_ctx_new(blk_filter_t* filter)
{
	blk_filter_ctx_t* ctx = kzalloc(sizeof(blk_filter_ctx_t), GFP_KERNEL);
	if (!ctx)
		return ctx;

	ctx->filter = filter;

	return ctx;
}

static int _blk_ctx_link(blk_filter_ctx_t* ctx, size_t altitude)
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

static int _blk_ctx_unlink(blk_filter_ctx_t* ctx)
{
	int result = -EEXIST;
	size_t altitude=BLK_FILTER_ALTITUDE_MIN;

	down_write(&blk_filter_ctx_list_lock);
	
	for (; altitude<=BLK_FILTER_ALTITUDE_MAX; ++altitude)
	{
		if (_get_ctx(altitude) && (_get_ctx(altitude) == ctx))
		{
			_set_ctx(altitude, NULL);
			result = 0;
			break;
		}
	}

	up_write(&blk_filter_ctx_list_lock);

	return result;
}

/**
 *
 */
void blk_filter_disk_add(struct gendisk *disk)
{
	size_t altitude=BLK_FILTER_ALTITUDE_MIN;

	pr_warn("blk-filter: add disk [%s].", disk->disk_name);

	down_read(&blk_filter_ctx_list_lock);

	for (; altitude<=BLK_FILTER_ALTITUDE_MAX; ++altitude)
	{
		blk_filter_ctx_t* ctx = _get_ctx(altitude);
		if (ctx && ctx->filter->ops && ctx->filter->ops->disk_add)
			ctx->filter->ops->disk_add(disk);
	}

	up_read(&blk_filter_ctx_list_lock);	
}

/**
 *
 */
void blk_filter_disk_del(struct gendisk *disk)
{
	size_t altitude=BLK_FILTER_ALTITUDE_MIN;

	pr_warn("blk-filter: del disk [%s].", disk->disk_name);

	down_read(&blk_filter_ctx_list_lock);

	for (; altitude<=BLK_FILTER_ALTITUDE_MAX; ++altitude)
	{
		blk_filter_ctx_t* ctx = _get_ctx(altitude);
		if (ctx && ctx->filter->ops && ctx->filter->ops->disk_del)
			ctx->filter->ops->disk_del(disk);
	}

	up_read(&blk_filter_ctx_list_lock);	
}

/**
 *
 */
void blk_filter_disk_release(struct gendisk *disk)
{
	size_t altitude=BLK_FILTER_ALTITUDE_MAX;

	pr_warn("blk-filter: release disk [%s].", disk->disk_name);

	down_read(&blk_filter_ctx_list_lock);

	for (; altitude<=BLK_FILTER_ALTITUDE_MIN; --altitude)
	{
		blk_filter_ctx_t* ctx = _get_ctx(altitude);
		if (ctx && ctx->filter->ops && ctx->filter->ops->disk_release)
			ctx->filter->ops->disk_release(disk);
	}

	up_read(&blk_filter_ctx_list_lock);	
}

/**
 * 
 */
blk_qc_t blk_filter_submit_bio(struct bio *bio)
{
		return blk_filter_submit_bio_altitude(BLK_FILTER_ALTITUDE_MAX, bio);
}
///////////////////////////////////////////////////////////////////////////////
// External functions


/**
 *
 */
int blk_filter_register(blk_filter_t* filter)
{
	int result = 0;
	blk_filter_ctx_t* ctx;

	pr_warn("blk-filter: register filter [%s].", filter->name);

	ctx =_blk_ctx_new(filter);
	if (!ctx)
		return -ENOMEM;

	result = _blk_ctx_link(ctx, filter->altitude);
	if (result)
		goto FailedLink;

	filter->blk_filter_ctx = (void*)ctx;
	return 0;

FailedLink:
	kfree(ctx);
	return result;
}
EXPORT_SYMBOL(blk_filter_register);

/**
 *
 */
int blk_filter_unregister(blk_filter_t* filter)
{
	int result = 0;
	blk_filter_ctx_t* ctx;
	
	pr_warn("blk-filter: unregister filter [%s].", filter->name);

	ctx = (blk_filter_ctx_t*)filter->blk_filter_ctx;

	result = _blk_ctx_unlink(ctx);
	if (result == 0)
		kfree(ctx);

	return result;
}
EXPORT_SYMBOL(blk_filter_unregister);

/**
 *
 */
const char* blk_filter_who_is_there(size_t altitude)
{
	blk_filter_ctx_t* ctx = _get_ctx(altitude);
	if (!ctx)
		return NULL;

	return ctx->filter->name;
}
EXPORT_SYMBOL(blk_filter_who_is_there);

static void _attach_fn(struct gendisk* disk, void* _ctx)
{
	blk_filter_t* filter = (blk_filter_t*)_ctx;
	if (filter->ops && filter->ops->disk_add)
		filter->ops->disk_add(disk);
}

/**
 *
 */
int blk_filter_attach_disks(blk_filter_t* filter)
{
	int result = disk_enumerate(_attach_fn, filter);
	return result;
}
EXPORT_SYMBOL(blk_filter_attach_disks);

/**
 *
 */
blk_qc_t blk_filter_submit_bio_altitude(size_t altitude, struct bio *bio)
{
	blk_qc_t ret;
	bool bypass = true;

	down_read(&blk_filter_ctx_list_lock);
	while (altitude >= BLK_FILTER_ALTITUDE_MIN)
	{
		blk_filter_ctx_t* ctx = _get_ctx(altitude);
		if (ctx && ctx->filter->ops && ctx->filter->ops->submit_bio)
		{
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
EXPORT_SYMBOL(blk_filter_submit_bio_altitude);


/**
 * blk_filter_submit_bio_next:
 */
blk_qc_t blk_filter_submit_bio_next(blk_filter_t* filter, struct bio *bio)
{
	return blk_filter_submit_bio_altitude(filter->altitude-1, bio);
}
EXPORT_SYMBOL(blk_filter_submit_bio_next);
