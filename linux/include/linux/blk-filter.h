/* SPDX-License-Identifier: GPL-2.0 */

/*
 * API declarations for kernel modules utilizing block device filters
 */

#ifndef BLK_FILTER_H
#define BLK_FILTER_H

#ifdef CONFIG_BLK_FILTER
#define BLK_FILTER_ALTITUDE_MAX 4
#define BLK_FILTER_ALTITUDE_MIN 1

struct blk_filter_ops {
	void (*disk_add)(struct gendisk *disk);
	void (*disk_del)(struct gendisk *disk);
	void (*disk_release)(struct gendisk *disk);
	blk_qc_t (*submit_bio)(struct bio *bio);
};

struct blk_filter {
	const char *name;
	const struct blk_filter_ops *ops;
	size_t altitude;
	void *blk_filter_ctx;
};


int blk_filter_register(struct blk_filter *filter);

int blk_filter_unregister(struct blk_filter *filter);

const char *blk_filter_check_altitude(size_t altitude);

int blk_filter_attach_disks(struct blk_filter *filter);

blk_qc_t blk_filter_submit_bio_next(struct blk_filter *filter, struct bio *bio);

#endif /* CONFIG_BLK_FILTER */

#endif
