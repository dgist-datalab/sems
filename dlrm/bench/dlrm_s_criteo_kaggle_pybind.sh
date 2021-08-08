#!/bin/bash

# Copyright (c) Facebook, Inc. and its affiliates.
#
# This source code is licensed under the MIT license found in the
# LICENSE file in the root directory of this source tree.
#
#WARNING: must have compiled PyTorch and caffe2

#check if extra argument is passed to the test
if [[ $# == 1 ]]; then
    dlrm_extra_option=$1
else
    dlrm_extra_option=""
fi
#echo $dlrm_extra_option

if [[ $@ == *"hw_emu"* ]]; then
    XCL_EMULATION_MODE=hw_emu
elif [[ $@ == *"hw"* ]]; then
    unset XCL_EMULATION_MODE
else
    XCL_EMULATION_MODE=sw_emu
fi

#for i in 1G 0.5G 0.25G

#do
    dlrm_pt_bin="/usr/bin/time --verbose python dlrm_s_pytorch_test3.py"

echo "run pytorch ..."
# WARNING: the following parameters will be set based on the data set
# --arch-embedding-size=... (sparse feature sizes)
# --arch-mlp-bot=... (the input to the first layer of bottom mlp)

sudo echo 3 > /proc/sys/vm/drop_caches

$dlrm_pt_bin --arch-sparse-feature-size=16 --arch-mlp-bot="13-512-256-64-16" --arch-mlp-top="512-256-1" --data-generation=dataset --data-set=kaggle --raw-data-file=./input/train.txt --processed-data-file=./input/kaggleAdDisplayChallenge_processed.npz --loss-function=bce --round-targets=True --learning-rate=0.1 --mini-batch-size=128 --print-freq=1024 --print-time --test-mini-batch-size=16384  --load-model=./models/model_sum.pt --test-freq=1024 --inference-only --arch-interaction-op=sum --load-embedding=/home/user1/Documents/vitis_lab/training/pybind_commandline_flow/lab/data/embedding_sum.txt --save-time --test-type="alveo_hw_test" $@ 2>&1 | tee run_kaggle_pt.log


echo "done"
#done
