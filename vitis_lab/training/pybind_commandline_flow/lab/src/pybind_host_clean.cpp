/**********
Copyright (c) 2018, Xilinx, Inc.
All rights reserved.
Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**********/

/*******************************************************************************
** HOST Code
*******************************************************************************/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <cstring>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>
#include <vector>

namespace py = pybind11;
using namespace std;

#include <CL/cl.h>

#include "help_functions.h"
#include "kernel.h"

#define CL_USE_DEPRECATED_OPENCL_1_2_APIS

#define BATCH_SIZE 128

//#include <CL/cl.hpp>

#define ALL_MESSAGES
#define NUM_CUS 2

const unsigned int DATAIN_1_SIZE = 2160804928;

int g_arch_sparse_feature_size;
int g_num_sparse_features;
int g_i_begin[NUM_CUS];
int g_i_end[NUM_CUS];
int g_sparse_offset_group_batch_size = -1;
int g_sparse_index_group_batch_size = -1;

float *RES[2];
float *DataIn_1;

unsigned int *emb_l;

int *lS_o;
int *lS_i;

cl_mem	GlobMem_BUF_emb_l,
        GlobMem_BUF_lS_o,
        GlobMem_BUF_lS_i,
        GlobMem_BUF_DataIn_1,
        GlobMem_BUF_RES[NUM_CUS];

cl_context Context;
cl_command_queue Command_Queue;

cl_kernel Kernel[NUM_CUS];
cl_program Program;
cl_device_id Target_Device_ID;

// ********************************************************************************** //
// ---------------------------------------------------------------------------------- //
//                          M A I N    F U N C T I O N                                //
// ---------------------------------------------------------------------------------- //
// ********************************************************************************** //

int alveo_init(const char *xclbinFilename, int arch_sparse_feature_size, int batch_size, int num_sparse_features, int len_lS_o, int len_lS_i)
{
	g_arch_sparse_feature_size = arch_sparse_feature_size;
	g_num_sparse_features = num_sparse_features;	

	// ============================================================================
	// Step 1: Check Command Line Arguments
	// ============================================================================
	//    o) argv[1] Platfrom Vendor
	//    o) argv[2] Device Name
	//    o) argv[3] XCLBIN file
	//    o) argv[4] DataIn_1 file
	//    o) argv[5] DataIN_2 file
	// ============================================================================
	#ifdef ALL_MESSAGES
	cout << "HOST-Info: ============================================================= " << endl;
	cout << "HOST-Info: (Step 1) Check Command Line Arguments                      " << endl;
	cout << "HOST-Info: ============================================================= " << endl;
	#endif

/*	if (argc != 6)
	{
		cout << "HOST-Error: Incorrect command line syntax " << endl;
		cout << "HOST-Info:  Usage: " << argv[0] << " <Platform_Vendor> <Device_Name> <XCLBIN_File> <DataIn_1_File> <DataIn_2_File>" << endl << endl;
		return EXIT_FAILURE;
	}*/ 
 
	const char* Target_Platform_Vendor   = "Xilinx";
	const char* Target_Device_Name       = "xilinx_u200_xdma_201830_2";
	//const char* xclbinFilename           = "/home/user1/Documents/vitis_lab/training/pybind_commandline_flow/lab/build/sw_emu/kernels.sw_emu.xclbin";
	const char* DataIn_1_FileName        = "/home/user1/Documents/alveo/vitis_lab/training/pybind_commandline_flow/lab/data/embedding_sum.txt";
    cout << "HOST-Info: Platform_Vendor : " << Target_Platform_Vendor << endl;
	cout << "HOST-Info: Device_Name     : " << Target_Device_Name << endl;
	cout << "HOST-Info: XCLBIN_file     : " << xclbinFilename << endl;
	cout << "HOST-Info: DataIn_1_File   : " << DataIn_1_FileName << endl;

	// ============================================================================
	// Step 2: Detect Target Platform and Target Device in a system. 
	//         Create Context and Command Queue.
	// ============================================================================
	// Variables:
	//   o) Target_Platform_Vendor[] - defined as main() input argument 
	//   o) Target_Device_Name[]     - defined as main() input argument
	// 
	// After that
	//   o) Create a Context
	//   o) Create a Command Queue
	// ============================================================================
	cout << endl;
	#ifdef ALL_MESSAGES
	cout << "HOST-Info: ============================================================= " << endl;
	cout << "HOST-Info: (Step 2) Detect Target Platform and Target Device in a system " << endl;
	cout << "HOST-Info:          Create Context and Command Queue                     " << endl;
	cout << "HOST-Info: ============================================================= " << endl;
	#endif

	cl_uint         ui;

	cl_platform_id      *Platform_IDs;
	cl_uint             Nb_Of_Platforms;
	cl_platform_id      Target_Platform_ID;
	bool                Platform_Detected;
	char                *platform_info;

	cl_device_id        *Device_IDs;
	cl_uint             Nb_Of_Devices;
	//cl_device_id        Target_Device_ID;
	bool                Device_Detected;
	char                *device_info;

	//cl_context          Context;
	//cl_command_queue    Command_Queue;

	cl_int              errCode;
	size_t              size;

    int                 i;

	// ------------------------------------------------------------------------------------
	// Step 2.1: Get All PLATFORMS, then search for Target_Platform_Vendor (CL_PLATFORM_VENDOR)
	// ------------------------------------------------------------------------------------
	
	// Get the number of platforms
	// ..................................................
	// errCode = clGetPlatformIDs(0, NULL, &Nb_Of_Platforms);
	errCode = clGetPlatformIDs(0, NULL, &Nb_Of_Platforms);
	if (errCode != CL_SUCCESS || Nb_Of_Platforms <= 0) {
		cout << endl << "HOST-Error: Failed to get the number of available platforms" << endl << endl;
		return EXIT_FAILURE;
	}

	#ifdef ALL_MESSAGES
	cout << "HOST-Info: Number of detected platforms : " << Nb_Of_Platforms << endl;
	#endif

	// Allocate memory to store platforms
	// ..................................................
	Platform_IDs = new cl_platform_id[Nb_Of_Platforms];
	if (!Platform_IDs) {
		cout << endl << "HOST-Error: Out of Memory during memory allocation for Platform_IDs" << endl << endl;
		return EXIT_FAILURE;
	}

	// Get and store all PLATFORMS
	// ..................................................
	errCode = clGetPlatformIDs(Nb_Of_Platforms, Platform_IDs, NULL);
	if (errCode != CL_SUCCESS) {
		cout << endl << "HOST-Error: Failed to get the available platforms" << endl << endl;
		return EXIT_FAILURE;
	}
 
	// Search for Platform (ex: Xilinx) using: CL_PLATFORM_VENDOR = Target_Platform_Vendor
	// ....................................................................................
	Platform_Detected = false;
	for (ui = 0; ui < Nb_Of_Platforms; ui++) {

		errCode = clGetPlatformInfo(Platform_IDs[ui], CL_PLATFORM_VENDOR, 0, NULL, &size);
		if (errCode != CL_SUCCESS) {
			cout << endl << "HOST-Error: Failed to get the size of the Platofrm parameter " << "CL_PLATFORM_VENDOR" << " value " << endl << endl;
			return EXIT_FAILURE;
		}

		platform_info = new char[size];
		if (!platform_info) {
			cout << endl << "HOST-Error: Out of Memory during memory allocation for Platform Parameter " << "CL_PLATFORM_VENDOR" << endl << endl;
			return EXIT_FAILURE;
		}

		errCode = clGetPlatformInfo(Platform_IDs[ui], CL_PLATFORM_VENDOR, size, platform_info , NULL);
		if (errCode != CL_SUCCESS) {
			cout << endl << "HOST-Error: Failed to get the " << "CL_PLATFORM_VENDOR" << " platform info" << endl << endl;
			return EXIT_FAILURE;
		}

		// Check if the current platform matches Target_Platform_Vendor
		// .............................................................
		if (strcmp(platform_info, Target_Platform_Vendor) == 0) {
			Platform_Detected        = true;
			Target_Platform_ID       = Platform_IDs[ui];
			#ifdef ALL_MESSAGES
			cout << "HOST-Info: Selected platform            : " << Target_Platform_Vendor << endl << endl;
			#endif
		}
	}

	if (Platform_Detected == false) {
		cout << endl << "HOST-Error: Failed to get detect " << Target_Platform_Vendor << " platform" << endl << endl;
		cout << "platform_info: " << platform_info << endl;
		return EXIT_FAILURE;
	}

	// ------------------------------------------------------------------------------------
	// Step 2.2:  Get All Devices for selected platform Target_Platform_ID
	//            then search for Xilinx platform (CL_DEVICE_NAME = Target_Device_Name)
	// ------------------------------------------------------------------------------------

	// Get the Number of Devices
	// ............................................................................
	errCode = clGetDeviceIDs(Target_Platform_ID, CL_DEVICE_TYPE_ALL, 0, NULL, &Nb_Of_Devices);

	if (errCode != CL_SUCCESS) {
		cout << endl << "HOST-Error: Failed to get the number of available Devices" << endl << endl;
		return EXIT_FAILURE;
	}
	#ifdef ALL_MESSAGES
	cout << "HOST-Info: Number of available devices  : " << Nb_Of_Devices << endl;
	#endif

	Device_IDs = new cl_device_id[Nb_Of_Devices];
	if (!Device_IDs) {
		cout << endl << "HOST-Error: Out of Memory during memory allocation for Device_IDs" << endl << endl;
		return EXIT_FAILURE;
	}

	errCode = clGetDeviceIDs(Target_Platform_ID, CL_DEVICE_TYPE_ALL, Nb_Of_Devices, Device_IDs, NULL);
	if (errCode != CL_SUCCESS) {
		cout << endl << "HOST-Error: Failed to get available Devices" << endl << endl;
		return EXIT_FAILURE;
	}

	// Search for CL_DEVICE_NAME = Target_Device_Name
	// ............................................................................
	Device_Detected = false;
	for (ui = 0; ui < Nb_Of_Devices; ui++) {
		errCode = clGetDeviceInfo(Device_IDs[ui], CL_DEVICE_NAME, 0, NULL, &size);
		if (errCode != CL_SUCCESS) {
			cout << endl << "HOST-Error: Failed to get the size of the Device parameter value " << "CL_DEVICE_NAME" << endl << endl;
			return EXIT_FAILURE;
		}
				
		device_info = new char[size];
		if (!device_info) {
			cout << endl << "HOST-Error: Out of Memory during memory allocation for Device parameter " << "CL_DEVICE_NAME" << " value " << endl << endl;
			return EXIT_FAILURE;
		}
				
		errCode = clGetDeviceInfo(Device_IDs[ui], CL_DEVICE_NAME, size, device_info, NULL);
		if (errCode != CL_SUCCESS) {
			cout << endl << "HOST-Error: Failed to get the " << "CL_DEVICE_NAME" << " device info" << endl << endl;
			return EXIT_FAILURE;
		}

		// Check if the current device matches Target_Device_Name
		// ............................................................................
		if (strcmp(device_info, Target_Device_Name) == 0) {
			Device_Detected        = true;
			Target_Device_ID       = Device_IDs[ui];
		}
	}

	if (Device_Detected == false) {
		cout << endl << "HOST-Error: Failed to get detect " << Target_Device_Name << " device" << endl << endl;
		return EXIT_FAILURE;
	} else {
		#ifdef ALL_MESSAGES
		cout << "HOST-Info: Selected device              : " << Target_Device_Name << endl << endl;
		#endif
	}

	// ------------------------------------------------------------------------------------
	// Step 2.3: Create Context
	// ------------------------------------------------------------------------------------
	#ifdef ALL_MESSAGES
	cout << "HOST-Info: Creating Context ... " << endl;
	#endif
	Context = clCreateContext(0, 1, &Target_Device_ID, NULL, NULL, &errCode);
	if (errCode != CL_SUCCESS) {
		cout << endl << "HOST-Error: Failed to create a Context" << endl << endl;
		return EXIT_FAILURE;
	}

	// ------------------------------------------------------------------------------------
	// Step 2.4: Create Command Queue (commands are executed in-order)
	// ------------------------------------------------------------------------------------
	#ifdef ALL_MESSAGES
	cout << "HOST-Info: Creating Command Queue ... " << endl;
	#endif
	Command_Queue = clCreateCommandQueue(Context, Target_Device_ID, CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE, &errCode);
	//Command_Queue = clCreateCommandQueue(Context, Target_Device_ID, 0, &errCode);
    if (errCode != CL_SUCCESS) {
		cout << endl << "HOST-Error: Failed to create a Command Queue" << endl << endl;
		return EXIT_FAILURE;
	}

	// ============================================================================
	// Step 3: Create Program and Kernel
	// ============================================================================
	//   o) Create a Program from a Binary File and Build it
	//   o) Create a Kernel
	// ============================================================================
	#ifdef ALL_MESSAGES
	cout << endl;
	cout << "HOST-Info: ============================================================= " << endl;
	cout << "HOST-Info: (Step 3) Create Program and Kernel                            " << endl;
	cout << "HOST-Info: ============================================================= " << endl;
	#endif

	// ------------------------------------------------------------------
	// Step 3.1: Load Binary File from a disk to Memory
	// ------------------------------------------------------------------
	unsigned char *xclbin_Memory;
	int program_length;
	
	#ifdef ALL_MESSAGES
	cout << "HOST-Info: Loading " << xclbinFilename << " binary file to memory ..." << endl;
	#endif

	program_length = loadFile2Memory(xclbinFilename, (char **) &xclbin_Memory);

	if (program_length < 0) {
		cout << endl << "HOST-Error: Failed to load " << xclbinFilename << " binary file to memory" << endl << endl;
		return EXIT_FAILURE;
	}

	// ------------------------------------------------------------
	// Step 3.2: Create a program using a Binary File
	// ------------------------------------------------------------
	size_t     Program_Length_in_Bytes;
	//cl_program Program;
	cl_int     Binary_Status;
	
	#ifdef ALL_MESSAGES
	cout << "HOST-Info: Creating Program with Binary ..." << endl;
	#endif
	Program_Length_in_Bytes = program_length;
	Program = clCreateProgramWithBinary(Context, 1, &Target_Device_ID, &Program_Length_in_Bytes,
                                        (const unsigned char **) &xclbin_Memory, &Binary_Status, &errCode);
	if (errCode != CL_SUCCESS) {
		cout << endl << "HOST-Error: Failed to create a Program from a Binary" << endl << endl;
		return EXIT_FAILURE;
	}

	// -------------------------------------------------------------
	// Step 3.3: Build (compiles and links) a program executable from binary
	// -------------------------------------------------------------
	#ifdef ALL_MESSAGES
	cout << "HOST-Info: Building the Program ..." << endl;
	#endif

	errCode = clBuildProgram(Program, 1, &Target_Device_ID, NULL, NULL, NULL);
	if (errCode != CL_SUCCESS) {
		cout << endl << "HOST-Error: Failed to build a Program Executable" << endl << endl;
		return EXIT_FAILURE;
	}

	// -------------------------------------------------------------
	// Step 3.4: Create a Kernel we wish to run
	// -------------------------------------------------------------
	#ifdef ALL_MESSAGES
	cout << "HOST-Info: Creating a Kernel: K_VADD ..." << endl;
	#endif

	for (i = 0; i < NUM_CUS; i++)
    {
        Kernel[i] = clCreateKernel(Program, "K_VADD", &errCode);
        if (errCode != CL_SUCCESS)
        {
            cout << endl << "HOST-Error: Failed to create a kernel" << endl << endl;
            return EXIT_FAILURE;
        }
    }

	int Nb_Of_Elements;
    int Nb_Of_Elements_Mem;

	#ifdef ALL_MESSAGES
	cout << endl;
	cout << "HOST-Info: ============================================================= " << endl;
	cout << "HOST-Info: (Step 4) Prepare Data to Run Kernel                           " << endl;
	cout << "HOST-Info: ============================================================= " << endl;
	#endif

	// ------------------------------------------------------------------
	// Step 4.1: Read data from DataIn_1_FileName -> DataIn_1 array 
	//           Read data from DataIn_2_FileName -> DataIn_2 array
	//           Allocate Memory to store the results: RES array
	// ------------------------------------------------------------------

	//cout << "HOST-Info: Reading Input data from the " << DataIn_1_FileName << " file ... ";

    void *ptr[NUM_CUS];

    emb_l = create_emb_l(DataIn_1_FileName, g_num_sparse_features * sizeof(unsigned int), 4, num_sparse_features);
    DataIn_1 = read_emb_file(DataIn_1_FileName, DATAIN_1_SIZE, 108);

    Nb_Of_Elements = batch_size * g_arch_sparse_feature_size / NUM_CUS;

    ptr[0] = nullptr;
	if (posix_memalign(&ptr[0],4096, Nb_Of_Elements * NUM_CUS * sizeof(float))) {
		cout << endl << "HOST-Error: Out of Memory during memory allocation for RES array" << endl << endl;
		return EXIT_FAILURE;
	}

    RES[0] = reinterpret_cast<float *>(ptr[0]);
    
    for (i = 1; i < NUM_CUS; i++)
    {
        ptr[i] = nullptr;
    	if (posix_memalign(&ptr[i],4096, Nb_Of_Elements * sizeof(float))) {
	    	cout << endl << "HOST-Error: Out of Memory during memory allocation for RES array" << endl << endl;
		    return EXIT_FAILURE;
	    }
        RES[i] = reinterpret_cast<float *>(ptr[i]);
    }

    void *ptr_o = nullptr;
    void *ptr_i = nullptr;
    if (posix_memalign(&ptr_o, 4096, len_lS_o * sizeof(int)))
    {
        cout << endl << "HOST-Error: Out of Memory during memory allocation for lS_o array" << endl << endl;
    }
    if (posix_memalign(&ptr_i, 4096, len_lS_i * sizeof(int)))
    {
        cout << endl << "HOST-Error: Out of Memory during memory allocation for lS_i array" << endl << endl;
    }
    lS_o = reinterpret_cast<int *>(ptr_o);
    lS_i = reinterpret_cast<int *>(ptr_i);

	// ------------------------------------------------------------------
	// Step 4.2: Create Buffers in Global Memory to store data
	//             o) GlobMem_BUF_DataIn_1 - stores DataIn_1
    //             o) GlobMem_BUF_emb_l    - stores emb_l
    //             o) GlobMem_BUF_lS_o     - stores lS_o
    //             o) GlobMem_BUF_lS_i     - stores lS_i
	//             o) GlobMem_BUF_RES      - stores RES
	// ------------------------------------------------------------------
	#ifdef ALL_MESSAGES
	cout << "HOST-Info: Allocating buffers in Global Memory to store Input and Output Data ..." << endl;
	#endif

	// Allocate Global Memory for GlobMem_BUF_DataIn_1
	// .......................................................
    
    GlobMem_BUF_DataIn_1 = clCreateBuffer(Context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, DATAIN_1_SIZE, DataIn_1, &errCode);
	if (errCode != CL_SUCCESS) {
		cout << endl << "Host-Error: Failed to allocate Global Memory for GlobMem_BUF_DataIn_1" << endl << endl;
		return EXIT_FAILURE;
	}

	// Allocate Global Memory for GlobMem_BUF_ptr_emb_l
	// .......................................................
	GlobMem_BUF_emb_l = clCreateBuffer(Context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, g_num_sparse_features * sizeof(unsigned int), emb_l, &errCode);
	if (errCode != CL_SUCCESS)
	{
		cout << endl << "Host-Error: Failed to allocate Global Memory for GlobMem_BUF_emb_l" << endl << endl;
	}

	// Allocate Global Memory for GlobMem_BUF_RES
	// .......................................................
    for (i = 0; i < NUM_CUS; i++)
    {
        GlobMem_BUF_RES[i] = clCreateBuffer(Context, CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR, Nb_Of_Elements * sizeof(float), RES[i], &errCode);
        if (errCode != CL_SUCCESS) {
		    cout << endl << "Host-Error: Failed to allocate Global Memory for GlobMem_BUF_RES" << endl << endl;
		    return EXIT_FAILURE;
        }
	}

    GlobMem_BUF_lS_o = clCreateBuffer(Context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, len_lS_o * sizeof(int), lS_o, &errCode);
    if (errCode != CL_SUCCESS)
    {
        cout << endl << "Host-Error: Failed to allocate Global Memory for GlobMem_BUF_lS_o" << endl << endl;
        return EXIT_FAILURE;
    }

    GlobMem_BUF_lS_i = clCreateBuffer(Context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, len_lS_i * sizeof(int), lS_i, &errCode);
    if (errCode != CL_SUCCESS)
    {
        cout << endl << "Host-Error: Failed to allocate Global Memory for GlobMem_BUF_lS_i" << endl << endl;
    }

	// ----------------------------------------
	// Step 5.1: Set Kernel Arguments
	// ----------------------------------------

    #ifdef ALL_MESSAGES
	cout << "HOST-Info: Setting Kernel arguments ..." << endl;
	#endif

    for (i = 0; i < NUM_CUS; i++)
    {    
        errCode = clSetKernelArg(Kernel[i], 0, sizeof(cl_mem), &GlobMem_BUF_DataIn_1);
        errCode |= clSetKernelArg(Kernel[i], 1, sizeof(cl_mem), &GlobMem_BUF_RES[i]);
        errCode |= clSetKernelArg(Kernel[i], 2, sizeof(cl_mem), &GlobMem_BUF_emb_l);
   	    errCode |= clSetKernelArg(Kernel[i], 3, sizeof(cl_mem), &GlobMem_BUF_lS_o);
	    errCode |= clSetKernelArg(Kernel[i], 4, sizeof(cl_mem), &GlobMem_BUF_lS_i);
	    //errCode |= clSetKernelArg(Kernel[i], 5, sizeof(int), &g_num_sparse_features);
        //errCode |= clSetKernelArg(Kernel[i], 6, sizeof(int), &g_arch_sparse_feature_size);
        //errCode |= clSetKernelArg(Kernel, 9, sizeof(int), &g_batch_size1);
    }

    if (errCode != CL_SUCCESS) {
		cout << endl << "Host-ERROR: Failed to set Kernel arguments" << endl << endl;
		return EXIT_FAILURE;
	}
    
    // ------------------------------------------------------
	// Step 4.3: Copy Input data from Host to Global Memory
	// ------------------------------------------------------
	#ifdef ALL_MESSAGES
	cout << "HOST_Info: Copy Input data to Global Memory ..." << endl;
	#endif
    
    errCode = clEnqueueMigrateMemObjects(Command_Queue, 1, &GlobMem_BUF_emb_l, 0, 0, NULL, NULL);

	if (errCode != CL_SUCCESS)
	{
		cout << endl << "Host-Error: Failed to write emb_l to GlobMem_BUF_emb_l" << endl << endl;
        return EXIT_FAILURE;
	}

    errCode = clEnqueueMigrateMemObjects(Command_Queue, 1, &GlobMem_BUF_DataIn_1, 0, 0, NULL, NULL);

    if (errCode != CL_SUCCESS)
    {
        cout << endl << "Host-Error: Failed to write GlobMem_BUF_DataIn_1" << endl << endl;
        return EXIT_FAILURE;
    }

    for (i = 0; i < NUM_CUS; i++)
    {
        g_i_begin[i] = -1;
        g_i_end[i] = -1;
    }

	::clFinish(Command_Queue);
    delete[] Platform_IDs;
	delete[] Device_IDs;

    return EXIT_SUCCESS;
}

void alveo_exit()
{
	int i;
    
    clReleaseDevice(Target_Device_ID);

    clReleaseMemObject(GlobMem_BUF_DataIn_1);
	clReleaseMemObject(GlobMem_BUF_emb_l);
    clReleaseMemObject(GlobMem_BUF_lS_o);
    clReleaseMemObject(GlobMem_BUF_lS_i);

    for (i = 0; i < NUM_CUS; i++)
        clReleaseMemObject(GlobMem_BUF_RES[i]);

    for (i = 0; i < NUM_CUS; i++)
        clReleaseKernel(Kernel[i]);

	clReleaseProgram(Program);
	clReleaseCommandQueue(Command_Queue);
	clReleaseContext(Context);

	for (i = 0; i < NUM_CUS; i++)
        free(RES[i]);
    
    free(DataIn_1);
    free(emb_l);
    free(lS_o);
    free(lS_i);
}

py::array_t<float> add(py::array_t<int> np_lS_o, py::array_t<int> np_lS_i, int sparse_offset_group_batch_size, int sparse_index_group_batch_size)
{	

    int i;
    cl_int	errCode;
    py::buffer_info buf_o = np_lS_o.request();
	py::buffer_info buf_i = np_lS_i.request();
	int *ptr_buf_o = reinterpret_cast<int *>(buf_o.ptr);
	int *ptr_buf_i = reinterpret_cast<int *>(buf_i.ptr);
    int lS_o_length = g_num_sparse_features * sparse_offset_group_batch_size;
	int lS_i_length = g_num_sparse_features * sparse_index_group_batch_size;
    int sparse_offset_group_batch_sizes[NUM_CUS];
    cl_event K_exe_event[NUM_CUS];
    int i_begin[NUM_CUS];
    int i_end[NUM_CUS];

    memcpy(lS_o, ptr_buf_o, lS_o_length * sizeof(int));
    memcpy(lS_i, ptr_buf_i, lS_i_length * sizeof(int));
    
    
    //int sparse_offset_group_batch_size1 = sparse_offset_group_batch_size / 2;
    /*
    if (sparse_offset_group_batch_size > BATCH_SIZE / 2)
    {
        g_batch_size2 = sparse_offset_group_batch_size - BATCH_SIZE / 2;
        errCode = clSetKernelArg(Kernel2, 9, sizeof(int), &g_batch_size2);
        if (errCode != CL_SUCCESS)
            cout << endl << "Host-ERROR: Failed to set Kernel argument 9" << endl;
    }
    else if (sparse_offset_group_batch_size < BATCH_SIZE / 2)
    {
        g_batch_size1 = sparse_offset_group_batch_size;
        errCode = clSetKernelArg(Kernel, 9, sizeof(int), &g_batch_size1);
        if (errCode != CL_SUCCESS)
            cout << endl << "Host-ERROR: Failed to set Kernel argument 9" << endl;
    }*/
    
    for (i = 0; i < NUM_CUS - 1; i++)
        sparse_offset_group_batch_sizes[i] = sparse_offset_group_batch_size / NUM_CUS;
    sparse_offset_group_batch_sizes[NUM_CUS - 1] = sparse_offset_group_batch_size - sparse_offset_group_batch_sizes[NUM_CUS - 2];

    for (i = 0; i < NUM_CUS; i++)
    {
        if (i == 0)
            i_begin[i] = 0;
        else
            i_begin[i] = i_end[i - 1];
        i_end[i] = i_begin[i] + sparse_offset_group_batch_sizes[i];
    }
	
    if (g_sparse_offset_group_batch_size != sparse_offset_group_batch_size)
    {
        g_sparse_offset_group_batch_size = sparse_offset_group_batch_size;
        for (i = 0; i < NUM_CUS; i++)
            errCode |= clSetKernelArg(Kernel[i], 5, sizeof(int), &sparse_offset_group_batch_size);
    }

    if (g_sparse_index_group_batch_size != sparse_index_group_batch_size)
    {
        g_sparse_index_group_batch_size = sparse_index_group_batch_size;
        for (i = 0; i < NUM_CUS; i++)
            errCode |= clSetKernelArg(Kernel[i], 6, sizeof(int), &sparse_index_group_batch_size);
    }

    for (i = 0; i < NUM_CUS; i++)
    {
        if (g_i_begin[i] != i_begin[i])
        {
            g_i_begin[i] = i_begin[i];
            errCode |= clSetKernelArg(Kernel[i], 7, sizeof(int), &i_begin[i]);
        }
        if (g_i_end[i] != i_end[i])
        {
            g_i_end[i] = i_end[i];
            errCode |= clSetKernelArg(Kernel[i], 8, sizeof(int), &i_end[i]);
        }

    }

    if (errCode |= CL_SUCCESS)
        cout << endl << "Host-ERROR: Failed to set Kernel arguments" << endl << endl;
    // ------------------------------------------------------
	// Step 4.3: Copy Input data from Host to Global Memory
	// ------------------------------------------------------
	#ifdef ALL_MESSAGES
	cout << "HOST_Info: Copy Input data to Global Memory ..." << endl;
	#endif   
    errCode = clEnqueueMigrateMemObjects(Command_Queue, 1, &GlobMem_BUF_lS_o, 0, 0, NULL, NULL);

	if (errCode != CL_SUCCESS) {
		cout << endl << "Host-Error: Failed to write lS_o to GlobMem_BUF_lS_o" << endl << endl;
		//return EXIT_FAILURE;
	}

	errCode = clEnqueueMigrateMemObjects(Command_Queue, 1, &GlobMem_BUF_lS_i, 0, 0, NULL, NULL);
	if (errCode != CL_SUCCESS) {
		cout << endl << "Host-Error: Failed to write lS_i to GlobMem_BUF_lS_i" << endl << endl;
		//return EXIT_FAILURE;
	}
    
    errCode = clEnqueueBarrierWithWaitList(Command_Queue, 0, NULL, NULL);
    if (errCode != CL_SUCCESS)
        cout << endl << "Host-Error: Failed to enqueue barrier with wait list" << endl << endl;

    // ============================================================================
	// Step 5: Set Kernel Arguments and Execute Kernel
	// ============================================================================
	// ----------------------------------------------------
	//  Argument Nb    Description
	// ----------------------------------------------------
	//     0           GlobMem_BUF_DataIn_1
	//     1           GlobMem_BUF_DataIn_2
	//     2           GlobMem_BUF_RES
	// ============================================================================
	#ifdef ALL_MESSAGES
	cout << endl;
	cout << "HOST-Info: ============================================================= " << endl;
	cout << "HOST-Info: (Step 5) Set Kernel Arguments and Execute Kernel              " << endl;
	cout << "HOST-Info: ============================================================= " << endl;
	#endif
	
	// ----------------------------------------
	// Step 5.2: Execute Kernel
	// ----------------------------------------
	size_t globalSize[1];
	size_t localSize[1];
	globalSize[0] = 1;
	localSize[0]  = 1;
    
    for (i = 0; i < NUM_CUS; i++)
        errCode |= clEnqueueTask(Command_Queue, Kernel[i], 0, NULL, &K_exe_event[i]);

	if (errCode != CL_SUCCESS) {
		cout << endl << "HOST-Error: Failed to execute Kernel" << endl << endl;
		//return EXIT_FAILURE;
	}

    for (i = 0; i < NUM_CUS; i++)
        errCode |= clEnqueueMigrateMemObjects(Command_Queue, 1, &GlobMem_BUF_RES[i], CL_MIGRATE_MEM_OBJECT_HOST, 1, &K_exe_event[i], NULL);

	if (errCode != CL_SUCCESS) {
		cout << endl << "Host-Error: Failed to write RES from GlobMem_BUF_RES" << endl << endl;
		//return EXIT_FAILURE;
	}

	// ============================================================================
	// Step 6: Read and Store the Output Results
	// ============================================================================
	// The Output Results are stored in the RES.txt file
	// 
	// ============================================================================
	#ifdef ALL_MESSAGES
	cout << endl;
	cout << "HOST-Info: ============================================================= " << endl;
	cout << "HOST-Info: (Step 6) Read, Store and Check the Output Results             " << endl;
	cout << "HOST-Info: ============================================================= " << endl;
	#endif

	// ------------------------------------------------------
	// Step 6.1: Read output Result to Host memory (RES)
	// ------------------------------------------------------

	::clFinish(Command_Queue); // flush everything in the Command_Queue

	//cout << endl << "HOST-Info: DONE" << endl << endl;
    
    for (i = 1; i < NUM_CUS; i++)
        memcpy((void *)(RES[0] + i_begin[i] * g_arch_sparse_feature_size), (const void *)RES[i], sparse_offset_group_batch_sizes[i] * g_arch_sparse_feature_size * sizeof(float));
    
    for (i = 0; i < NUM_CUS; i++)
        clReleaseEvent(K_exe_event[i]);

    return py::array_t<float>(
			{sparse_offset_group_batch_size * g_arch_sparse_feature_size},
			{4},
			RES[0]);
}


PYBIND11_MODULE(pybind_host2, m)
{
	m.doc() = "pybind11 module for opencl";
	m.def("add", &add);
    m.def("alveo_init", &alveo_init);
    m.def("alveo_exit", &alveo_exit);
}
