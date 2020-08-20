#!/bin/bash -e

KERNEL_PATH="../../linux"
BLK_FILTER_PATH=$(pwd)

echo "copy from [${BLK_FILTER_PATH}] to [${KERNEL_PATH}]"
for fl in block/blk-filter-internal.h block/blk-filter.c include/linux/blk-filter.h 
do
	echo "${fl}"
	cp ${BLK_FILTER_PATH}/${fl} ${KERNEL_PATH}/${fl}
done
