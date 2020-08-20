/*
	API for block device filters 
	Internal declaration for kernel
*/
#ifdef CONFIG_BLK_FILTER
#ifndef BLK_FILTER_INTERNAL_H
#define BLK_FILTER_INTERNAL_H

#include <linux/blk-filter.h>

void blk_filter_disk_add(struct gendisk *disk);
void blk_filter_disk_del(struct gendisk *disk);
void blk_filter_disk_release(struct gendisk *disk);
blk_qc_t blk_filter_submit_bio(struct bio *bio);


#endif
#endif //CONFIG_BLK_FILTER
