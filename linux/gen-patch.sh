#!/bin/bash -e

KERNEL_PATH="../../linux"
BLK_FILTER_PATH=$(pwd)
cd ${KERNEL_PATH}

#git add --all
git diff HEAD > ${BLK_FILTER_PATH}/blk-filter.patch

cd ${BLK_FILTER_PATH}
