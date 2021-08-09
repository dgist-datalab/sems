# sems
Github repo for SEMS: Scalable Embedding Memory System Exploiting Near-data Processing

Explanation on files/directories
Not all files required to run the system are uploaded, so the codes uploaded are just for viewing.

sems/dlrm: The directory related to dlrm.
sems/dlrm/dlrm_s_pytorch_test3.py: The dlrm code written in Python, and modified to be used with alveo. apply_emb_interact_feat is the function that handles the embedding aleyr and feature interaction, which is mostly processed by alveo.
sems/dlrm/bench/dlrm_s_criteo_kaggle_pybind.sh: the shell script to run dlrm.

sems/vitis_lab/training/pybind_commandline_flow/lab: The directory related to vitis and alveo.
sems/vitis_lab/training/pybind_commandline_flow/lab/src: Where all the source file for alveo exists.
sems/vitis_lab/training/pybind_commandline_flow/lab/src/pybind_K_ALL3.cpp: The C++ based kernel code which implements embedding lookup and feature interaction.
sems/vitis_lab/training/pybind_commandline_flow/lab/src/pybind_host_clean.cpp: The host code. This currently does not support the use of multiple devices in parallel.
sems/vitis_lab/training/pybind_commandline_flow/lab/pybind_build: Where all the files for building the program exists.
sems/vitis_lab/training/pybind_commandline_flow/lab/pybind_build/Makefile: The Makefile for building a project with vitis.
