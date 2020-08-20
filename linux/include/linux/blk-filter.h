/*
	API for block device filters 
	External API declaration for kernel modules
*/
#ifdef CONFIG_BLK_FILTER
#ifndef BLK_FILTER_H
#define BLK_FILTER_H


#define BLK_FILTER_ALTITUDE_MAX 4
#define BLK_FILTER_ALTITUDE_MIN 1

typedef struct blk_filter_ops
{
	void (*disk_add)(struct gendisk *disk);
	void (*disk_del)(struct gendisk *disk);
	void (*disk_release)(struct gendisk *disk);
	blk_qc_t (*submit_bio)(struct bio *bio);
}blk_filter_ops_t;

typedef struct blk_filter
{
	const char* name;
	const blk_filter_ops_t *ops;
	size_t altitude;
	void* blk_filter_ctx;

} blk_filter_t;


int blk_filter_register(blk_filter_t* filter);
int blk_filter_unregister(blk_filter_t* filter);

const char* blk_filter_who_is_there(size_t altitude);

int blk_filter_attach_disks(blk_filter_t* filter);

blk_qc_t blk_filter_submit_bio_next(blk_filter_t* filter, struct bio *bio);


#endif
#endif //CONFIG_BLK_FILTER
