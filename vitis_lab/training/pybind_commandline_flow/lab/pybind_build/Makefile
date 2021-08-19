# =========================================================
# platform selection
# =========================================================
PLATFORM_VENDOR := Xilinx
PLATFORM := xilinx_u200_xdma_201830_2

# =========================================================
# TARGET can be set as:
#   sw_emu: software emulation
#   hw_emu: hardware Emulation
#   hw    : hardware run
# =========================================================
TARGET := sw_emu
TEST := .

# =========================================================
# Below are the names for host executable and xclbin.
# Please keep it unchanged.
# =========================================================

XO_K_VADD := K_VADD.$(TARGET)

XCLBIN := kernels.$(TARGET).xclbin

# =========================================================
# Source directory
# =========================================================
ROOT_REPO := /home/user1/Documents/alveo/vitis_lab/training
SRC_REPO := $(ROOT_REPO)/pybind_commandline_flow/lab/src

PROJECT_DIR := $(ROOT_REPO)/pybind_commandline_flow/lab
BUILD_DIR := $(PROJECT_DIR)/build/$(TARGET)

# =========================================================
# Kernel Source Files repository
# =========================================================
KERNEL_K_VADD_SRC_CPP = $(SRC_REPO)/pybind_K_ALL3.cpp

KERNEL_SRC_H   = $(SRC_REPO)/kernel.h
KERNEL_SRC_H_DIR := $(SRC_REPO)

# =========================================================
# Host Compiler Global Settings and Include Libraries
# =========================================================
CXXFLAGS += -I$(XILINX_XRT)/include/
CXXFLAGS += -I$(XILINX_VIVADO)/include/
CXXFLAGS += -I$(SRC_REPO)
CXXFLAGS += -O0 -g -Wall -fmessage-length=0 -std=c++11 
CXXLDFLAGS := -L$(XILINX_XRT)/lib/
CXXLDFLAGS += -lOpenCL -lpthread -lrt 

# =========================================================
# Kernel Compiler and Linker Flags
# =========================================================
VPPFLAGS := -t $(TARGET)
#VPPFLAGS += --config design.cfg
VPPFLAGS += -I$(KERNEL_SRC_H_DIR)
VPPFLAGS += --temp_dir $(BUILD_DIR)
VPPFLAGS += --log_dir $(BUILD_DIR)
VPPFLAGS += --platform $(PLATFORM)
#VPPFLAGS += --kernel_frequency 244


# =========================================================
# Host Executable File Generation
# =========================================================
$(BUILD_DIR)/pybind_host2$(shell python3-config --extension-suffix): $(BUILD_DIR)/help_functions.o $(BUILD_DIR)/pybind_host2.o
	mkdir -p $(BUILD_DIR)
	g++ -shared -o $(BUILD_DIR)/pybind_host2`python3-config --extension-suffix` $(BUILD_DIR)/help_functions.o $(BUILD_DIR)/pybind_host2.o $(CXXLDFLAGS)

$(BUILD_DIR)/help_functions.o: $(SRC_REPO)/help_functions.cpp $(SRC_REPO)/help_functions.h
	mkdir -p $(BUILD_DIR)
	g++ $(CXXFLAGS) -c -fPIC $(SRC_REPO)/help_functions.cpp $(CXXLDFLAGS) -o $@

$(BUILD_DIR)/pybind_host2.o: $(SRC_REPO)/pybind_host_clean.cpp $(SRC_REPO)/help_functions.h $(SRC_REPO)/kernel.h 
	mkdir -p $(BUILD_DIR)
	g++ $(CXXFLAGS) `python3 -m pybind11 --includes` -fPIC -c $(SRC_REPO)/pybind_host_clean.cpp $(CXXLDFLAGS) -o $@

# =========================================================
# Kernel XO and Xclbin File Generation
# =========================================================



$(BUILD_DIR)/$(XO_K_VADD).xo: $(KERNEL_K_VADD_SRC_CPP) $(KERNEL_SRC_H) 
	mkdir -p $(BUILD_DIR)
	v++ $(VPPFLAGS) -c -k K_VADD $(KERNEL_K_VADD_SRC_CPP) -o $@

$(BUILD_DIR)/$(XCLBIN): $(BUILD_DIR)/$(XO_K_VADD).xo
	mkdir -p $(BUILD_DIR)
	v++ $(VPPFLAGS) -g -l -O 2 -o $@ $(BUILD_DIR)/$(XO_K_VADD).xo --config design.cfg

# =========================================================
# Emulation Files Generation
# =========================================================
EMCONFIG_FILE = emconfig.json

$(BUILD_DIR)/$(EMCONFIG_FILE):
	 emconfigutil --nd 1  --platform $(PLATFORM) --od $(BUILD_DIR)


build: $(BUILD_DIR)/pybind_host2$(shell python3-config --extension-suffix) $(BUILD_DIR)/$(XCLBIN) $(BUILD_DIR)/$(EMCONFIG_FILE); cp $(BUILD_DIR)/pybind_host2$(shell python3-config --extension-suffix) ~/Documents/alveo/dlrm; 

run: build 
ifeq ($(TARGET), hw)
	cd $(BUILD_DIR) && unset XCL_EMULATION_MODE;    python example.py ;
else
	cd $(BUILD_DIR) && XCL_EMULATION_MODE=$(TARGET) python example.py;
endif


clean:
	rm -rf $(BUILD_DIR)/*.o $(BUILD_DIR)/*.so $(BUILD_DIR)/$(XCLBIN) $(BUILD_DIR)/*.xo $(BUILD_DIR)/*.ltx
