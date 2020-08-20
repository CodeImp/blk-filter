#!/bin/bash -e

KERNEL_PATH="../../linux"
BLK_FILTER_PATH=$(pwd)

echo "copy from [${KERNEL_PATH}] to [${BLK_FILTER_PATH}]"
for fl in block/blk-filter-internal.h block/blk-filter.c include/linux/blk-filter.h 
do
	echo "${fl}"
	cp ${KERNEL_PATH}/${fl} ${BLK_FILTER_PATH}/${fl}
done
