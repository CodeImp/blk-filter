/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Block device filters internal declarations
 */

#ifndef BLK_FILTER_INTERNAL_H
#define BLK_FILTER_INTERNAL_H

#ifdef CONFIG_BLK_FILTER
#include <linux/blk-filter.h>

void blk_filter_disk_add(struct gendisk *disk);

void blk_filter_disk_del(struct gendisk *disk);

void blk_filter_disk_release(struct gendisk *disk);

blk_qc_t blk_filter_submit_bio(struct bio *bio);

#else /* CONFIG_BLK_FILTER */

static inline void blk_filter_disk_add(struct gendisk *disk) { }

static inline void blk_filter_disk_del(struct gendisk *disk) { }

static inline void blk_filter_disk_release(struct gendisk *disk) { }

static inline blk_qc_t blk_filter_submit_bio(struct bio *bio) { return 0; }

#endif /* CONFIG_BLK_FILTER */

#endif