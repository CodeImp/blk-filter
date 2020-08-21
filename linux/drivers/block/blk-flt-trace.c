/* SPDX-License-Identifier: GPL-2.0 */
/*
 * blk-flt-trace.c
 *
 * Written by CodeImp
 *
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/genhd.h>
#include <linux/blk-filter.h>
#include <linux/atomic.h>

#define MODULE_NAME "blk-flt-trace"
#define DEFAULT_ALTITUDE BLK_FILTER_ALTITUDE_MAX

void _trace_disk_add(struct gendisk *disk);
void _trace_disk_del(struct gendisk *disk);
void _trace_disk_release(struct gendisk *disk);
blk_qc_t _trace_submit_bio(struct bio *bio);

static struct blk_filter_ops blk_flt_ops = {
	.disk_add = _trace_disk_add,
	.disk_del = _trace_disk_del,
	.disk_release = _trace_disk_release,
	.submit_bio = _trace_submit_bio
};

static struct blk_filter trace_filter = {
	.name = MODULE_NAME,
	.ops = &blk_flt_ops,
	.altitude = DEFAULT_ALTITUDE,
	.blk_filter_ctx = NULL
};

struct trace_data {
	atomic64_t bio_counter;
};

static struct trace_data trace_data = {
	.bio_counter = ATOMIC64_INIT(0)
};

static int trace_data_init(void ) {
	return 0;
}

void _trace_disk_add(struct gendisk *disk)
{
	pr_err(MODULE_NAME": %s\n", __FUNCTION__);
	pr_err(MODULE_NAME":\tdisk name: %s\n", disk->disk_name);
	pr_err(MODULE_NAME":\tdisk major: %d\n", disk->major);
}

void _trace_disk_del(struct gendisk *disk)
{
	pr_err(MODULE_NAME": %s\n", __FUNCTION__);
	pr_err(MODULE_NAME":\tdisk name: %s\n", disk->disk_name);
	pr_err(MODULE_NAME":\tdisk major: %d\n", disk->major);	
}

void _trace_disk_release(struct gendisk *disk)
{
	pr_err(MODULE_NAME": %s\n", __FUNCTION__);
}

blk_qc_t _trace_submit_bio(struct bio *bio)
{
	atomic64_inc(&trace_data.bio_counter);
	return blk_filter_submit_bio_next(&trace_filter, bio);
}

static int __init blk_flt_trace_init(void)
{
	int ret = 0;
	const char* filter_name;

	pr_err(MODULE_NAME": %s\n", __FUNCTION__);
	filter_name = blk_filter_check_altitude(trace_filter.altitude);
	if (filter_name) {
		pr_err(MODULE_NAME": block filter [%s] already registered.\n", filter_name);
		return ret;
	} 

	ret = trace_data_init();
	if (ret) {
		pr_err(MODULE_NAME": failed to initialize internal datas.\n");
		return ret;
	}

	ret = blk_filter_register(&trace_filter);
	if (ret){
		pr_err( MODULE_NAME": unable to get major number\n");
		return ret;
	}

	ret = blk_filter_attach_disks(&trace_filter);
	if (ret)
		pr_err( MODULE_NAME": failed to attach existing disks\n");

	return ret;
}

static void __exit blk_flt_trace_exit(void)
{
	uint64_t count;

	pr_err(MODULE_NAME": %s\n", __FUNCTION__);

	count = atomic64_read(&trace_data.bio_counter);
	pr_err(MODULE_NAME": bio intercepted %llu times\n", count);

	blk_filter_unregister(&trace_filter);
}


module_init(blk_flt_trace_init);
module_exit(blk_flt_trace_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Code Imp");