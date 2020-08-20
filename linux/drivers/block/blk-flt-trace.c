/*
 * blk-flt-trace.c
 *
 * Written by 
 *
 * Copyright 
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/genhd.h>
#include <linux/blk-filter.h>

#include "blk-flt-trace.h"

#define MODULE_NAME "blk-flt-trace"
#define DEFAULT_ALTITUDE BLK_FILTER_ALTITUDE_MAX

void _trace_disk_add(struct gendisk *disk);
void _trace_disk_del(struct gendisk *disk);
void _trace_disk_release(struct gendisk *disk);
blk_qc_t _trace_submit_bio(struct bio *bio);

static blk_filter_ops_t blk_flt_ops = 
{
    .disk_add = _trace_disk_add,
    .disk_del = _trace_disk_del,
    .disk_release = _trace_disk_release,
    .submit_bio = _trace_submit_bio
};

blk_filter_t trace_filter = 
{
	.name = MODULE_NAME,
	.ops = &blk_flt_ops,
	.altitude = DEFAULT_ALTITUDE,
	.blk_filter_ctx = NULL
};

void _trace_disk_add(struct gendisk *disk)
{
    pr_err("%s", __FUNCTION__);
}
void _trace_disk_del(struct gendisk *disk)
{
    pr_err("%s", __FUNCTION__);
}
void _trace_disk_release(struct gendisk *disk)
{
    pr_err("%s", __FUNCTION__);
}
blk_qc_t _trace_submit_bio(struct bio *bio)
{
    return blk_filter_submit_bio_next(&trace_filter, bio);
}


static int __init blk_flt_trace_init(void)
{
    int ret = 0;
    const char* filter_name = blk_filter_who_is_there(trace_filter.altitude);
	if (filter_name){
    	pr_err(MODULE_NAME": block filter [%s] already registered.\n", filter_name);
    	return ret;		
	} 

    ret = blk_filter_register(&trace_filter);
    if (ret){
    	pr_err( MODULE_NAME": unable to get major number\n");
    	return ret;
    }

    ret = blk_filter_attach_disks(&trace_filter);


    return ret;
}

static void __exit blk_flt_trace_exit(void)
{
    blk_filter_unregister(&trace_filter);
}


module_init(blk_flt_trace_init);
module_exit(blk_flt_trace_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Code Imp");