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
#include <pthread.h>

namespace py = pybind11;
using namespace std;

#include <CL/cl.h>

#include "help_functions.h"
#include "kernel.h"

#define CL_USE_DEPRECATED_OPENCL_1_2_APIS

#define BATCH_SIZE 128

//#include <CL/cl.hpp>

//#define ALL_MESSAGES
#define NUM_CUS 2
#define NUM_DEVS 2 

int g_arch_sparse_feature_size;
int g_num_sparse_features;
int g_i_begin[NUM_CUS * NUM_DEVS];
int g_i_end[NUM_CUS * NUM_DEVS];
int g_sparse_offset_group_batch_size[NUM_DEVS];
int g_sparse_index_group_batch_size[NUM_DEVS];

//float *RES[NUM_CUS * NUM_DEVS];
float *RES[NUM_CUS];
float *DataIn_1[NUM_DEVS];

unsigned int *emb_l;
unsigned int *emb_ls[NUM_DEVS];
unsigned int *table_lists;
unsigned int *table_nums_;
unsigned int table_nums[NUM_DEVS];

int *lS_o[NUM_DEVS];
int *lS_i[NUM_DEVS];

cl_mem	GlobMem_BUF_emb_l[NUM_DEVS],
        GlobMem_BUF_lS_o[NUM_DEVS],
        GlobMem_BUF_lS_i[NUM_DEVS],
        GlobMem_BUF_DataIn_1[NUM_DEVS],
        GlobMem_BUF_RES[NUM_DEVS];
        //GlobMem_BUF_RES[NUM_CUS * NUM_DEVS];

cl_context Context[NUM_DEVS];
cl_command_queue Command_Queue[NUM_DEVS];

cl_kernel Kernel[NUM_CUS * NUM_DEVS];
cl_program Program[NUM_DEVS];
cl_device_id Target_Device_ID[NUM_DEVS];
bool Device_Detected[NUM_DEVS];

struct thread_data
{
    int sparse_offset_group_batch_size;
    int sparse_index_group_batch_size;
    int ui;
    int *ptr_buf_o;
    int *ptr_buf_i;
    int start_offset;
};

// ********************************************************************************** //
// ---------------------------------------------------------------------------------- //
//                          M A I N    F U N C T I O N                                //
// ---------------------------------------------------------------------------------- //
// ********************************************************************************** //

int alveo_init(const char *xclbinFilename, int arch_sparse_feature_size, int batch_size, int num_sparse_features, int len_lS_o, int len_lS_i, py::array_t<unsigned int> np_emb_l, py::array_t<unsigned int> np_table_nums, py::array_t<unsigned int> np_table_lists, py::array_t<unsigned int> np_table_sizes)
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
	//bool                Device_Detected;
	char                *device_info;

	//cl_context          Context;
	//cl_command_queue    Command_Queue;

	cl_int              errCode;
	size_t              size;

    int                 i;
	int Nb_Of_Elements;

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
	for (ui = 0; ui < NUM_DEVS; ui++)
        Device_Detected[ui] = false;
    
	for (ui = 0; ui < NUM_DEVS; ui++) {
    //for (ui = 0; ui < Nb_Of_Devices; ui++) {
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
			Device_Detected[ui]        = true;
			Target_Device_ID[ui]      = Device_IDs[ui];
            //cout << "device matched: " << ui << endl;
		}

        delete[] device_info;
    }

    for (ui = 0; ui < NUM_DEVS; ui++)
    {
    
	    if (Device_Detected[ui] == false) {
		    cout << endl << "HOST-Error: Failed to get detect " << Target_Device_Name << " device: " << ui << endl << endl;
		    continue;
            //return EXIT_FAILURE;
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
	    Context[ui] = clCreateContext(0, 1, &Target_Device_ID[ui], NULL, NULL, &errCode);
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
	    Command_Queue[ui] = clCreateCommandQueue(Context[ui], Target_Device_ID[ui], CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE, &errCode);
	    //Command_Queue = clCreateCommandQueue(Context, Target_Device_ID, 0, &errCode);
        if (errCode != CL_SUCCESS) {
	    	cout << endl << "HOST-Error: Failed to create a Command Queue" << endl << endl;
		    return EXIT_FAILURE;
	    }
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

    for (ui = 0; ui < NUM_DEVS; ui++)
    {    
	    if (Device_Detected[ui] == false) {
		    cout << endl << "HOST-Error: Failed to get detect " << Target_Device_Name << " device" << endl << endl;
		    continue;
            //return EXIT_FAILURE;
	    } 
    
        Program[ui] = clCreateProgramWithBinary(Context[ui], 1, &Target_Device_ID[ui], &Program_Length_in_Bytes,
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

	    errCode = clBuildProgram(Program[ui], 1, &Target_Device_ID[ui], NULL, NULL, NULL);
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
            Kernel[ui * NUM_DEVS + i] = clCreateKernel(Program[ui], "K_VADD", &errCode);
            if (errCode != CL_SUCCESS)
            {
                cout << endl << "HOST-Error: Failed to create a kernel" << endl << endl;
                return EXIT_FAILURE;
            }
        }
    }

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
    
    //void *ptr[NUM_DEVS * NUM_CUS];
    void *ptr[NUM_DEVS];

    py::buffer_info buf_emb_l = np_emb_l.request();
	unsigned int *ptr_buf_emb_l = reinterpret_cast<unsigned int *>(buf_emb_l.ptr);
    py::buffer_info buf_table_nums = np_table_nums.request();
    table_nums_ = reinterpret_cast<unsigned int *>(buf_table_nums.ptr);
    py::buffer_info buf_table_lists = np_table_lists.request();
    table_lists = reinterpret_cast<unsigned int *>(buf_table_lists.ptr);
    py::buffer_info buf_table_sizes = np_table_sizes.request();
    unsigned int *table_sizes = reinterpret_cast<unsigned int *>(buf_table_sizes.ptr);

    emb_l = create_emb_l(DataIn_1_FileName, g_num_sparse_features * sizeof(unsigned int), 4, num_sparse_features, DATAIN_1_SIZE);

    memcpy(table_nums, table_nums_, sizeof(unsigned int) * NUM_DEVS);

    Nb_Of_Elements = batch_size * g_arch_sparse_feature_size / NUM_CUS;

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

    unsigned int j = 0;
    
	for (ui = 0; ui < NUM_DEVS; ui++)
    {    
	    if (Device_Detected[ui] == false) {
		    cout << endl << "HOST-Error: Failed to get detect " << Target_Device_Name << " device" << endl << endl;
		    continue;
            //return EXIT_FAILURE;
	    } 
        
        DataIn_1[ui] = read_emb_file(DataIn_1_FileName, table_sizes[ui] * sizeof(float), 108, table_lists, emb_l, j, j + table_nums[ui]);

        void *ptr_emb_l = nullptr;
        if (posix_memalign(&ptr_emb_l, 4096, table_nums[ui] * sizeof(unsigned int)))
        {
            cout << endl << "HOST-Error: Out of Memory during memory allocation for emb_l array" << endl << endl;
        }

        emb_ls[ui] = reinterpret_cast<unsigned int *>(ptr_emb_l);

        memcpy(emb_ls[ui], ptr_buf_emb_l + j, table_nums[ui] * sizeof(unsigned int));
        
        j += table_nums[ui];        
        
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
        lS_o[ui] = reinterpret_cast<int *>(ptr_o);
        lS_i[ui] = reinterpret_cast<int *>(ptr_i);

        ptr[ui] = nullptr;
        if (posix_memalign(&ptr[ui],4096, Nb_Of_Elements * NUM_CUS * sizeof(float))) {
		    cout << endl << "HOST-Error: Out of Memory during memory allocation for RES array" << endl << endl;
		    return EXIT_FAILURE;
	    }

        RES[ui] = reinterpret_cast<float *>(ptr[ui]);
        
        
        /*
        ptr[ui * NUM_DEVS + 0] = nullptr;
	    if (posix_memalign(&ptr[ui * NUM_DEVS + 0],4096, Nb_Of_Elements * NUM_CUS * sizeof(float))) {
		    cout << endl << "HOST-Error: Out of Memory during memory allocation for RES array" << endl << endl;
		    return EXIT_FAILURE;
	    }

        RES[ui * NUM_DEVS + 0] = reinterpret_cast<float *>(ptr[ui * NUM_DEVS + 0]);
        
        for (i = 1; i < NUM_CUS; i++)
        {
            ptr[ui * NUM_DEVS + i] = nullptr;
    	    if (posix_memalign(&ptr[ui * NUM_DEVS + i],4096, Nb_Of_Elements * sizeof(float))) {
	    	    cout << endl << "HOST-Error: Out of Memory during memory allocation for RES array" << endl << endl;
		        return EXIT_FAILURE;
	        }
            RES[ui * NUM_DEVS + i] = reinterpret_cast<float *>(ptr[ui * NUM_DEVS + i]);
        }*/

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
        
        GlobMem_BUF_DataIn_1[ui] = clCreateBuffer(Context[ui], CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, table_sizes[ui] * sizeof(float), DataIn_1[ui], &errCode);
	    if (errCode != CL_SUCCESS) {
		    cout << endl << "Host-Error: Failed to allocate Global Memory for GlobMem_BUF_DataIn_1" << endl << endl;
		    return EXIT_FAILURE;
	    }

	    // Allocate Global Memory for GlobMem_BUF_ptr_emb_l
	    // .......................................................
	    GlobMem_BUF_emb_l[ui] = clCreateBuffer(Context[ui], CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, g_num_sparse_features * sizeof(unsigned int), emb_ls[ui], &errCode);
	    if (errCode != CL_SUCCESS)
	    {
		    cout << endl << "Host-Error: Failed to allocate Global Memory for GlobMem_BUF_emb_l" << endl << endl;
	    }

	    // Allocate Global Memory for GlobMem_BUF_RES
	    // .......................................................
        GlobMem_BUF_RES[ui] = clCreateBuffer(Context[ui], CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR, Nb_Of_Elements * NUM_CUS * sizeof(float), RES[ui], &errCode);

        //GlobMem_BUF_RES[ui] = clCreateBuffer(Context[ui], CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR, Nb_Of_Elements * sizeof(float), RES[ui], &errCode);
        if (errCode != CL_SUCCESS) {
		    cout << endl << "Host-Error: Failed to allocate Global Memory for GlobMem_BUF_RES" << endl << endl;
		    return EXIT_FAILURE;
        }
        /*
        for (i = 0; i < NUM_CUS; i++)
        {
            GlobMem_BUF_RES[ui * NUM_DEVS + i] = clCreateBuffer(Context[ui], CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR, Nb_Of_Elements * sizeof(float), RES[ui * NUM_DEVS + i], &errCode);
            if (errCode != CL_SUCCESS) {
		        cout << endl << "Host-Error: Failed to allocate Global Memory for GlobMem_BUF_RES" << endl << endl;
		        return EXIT_FAILURE;
            }
	    }*/

        GlobMem_BUF_lS_o[ui] = clCreateBuffer(Context[ui], CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, len_lS_o * sizeof(int), lS_o[ui], &errCode);
        if (errCode != CL_SUCCESS)
        {
            cout << endl << "Host-Error: Failed to allocate Global Memory for GlobMem_BUF_lS_o" << endl << endl;
            return EXIT_FAILURE;
        }

        GlobMem_BUF_lS_i[ui] = clCreateBuffer(Context[ui], CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, len_lS_i * sizeof(int), lS_i[ui], &errCode);
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
            errCode = clSetKernelArg(Kernel[ui * NUM_DEVS + i], 0, sizeof(cl_mem), &GlobMem_BUF_DataIn_1[ui]);
            errCode |= clSetKernelArg(Kernel[ui * NUM_DEVS + i], 1, sizeof(cl_mem), &GlobMem_BUF_RES[ui]);
            //errCode |= clSetKernelArg(Kernel[ui * NUM_DEVS + i], 1, sizeof(cl_mem), &GlobMem_BUF_RES[ui * NUM_DEVS + i]);
            errCode |= clSetKernelArg(Kernel[ui * NUM_DEVS + i], 2, sizeof(cl_mem), &GlobMem_BUF_emb_l[ui]);
   	        errCode |= clSetKernelArg(Kernel[ui * NUM_DEVS + i], 3, sizeof(cl_mem), &GlobMem_BUF_lS_o[ui]);
	        errCode |= clSetKernelArg(Kernel[ui * NUM_DEVS + i], 4, sizeof(cl_mem), &GlobMem_BUF_lS_i[ui]);
	        errCode |= clSetKernelArg(Kernel[ui * NUM_DEVS + i], 5, sizeof(unsigned int), &table_nums[ui]);
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
    
        errCode = clEnqueueMigrateMemObjects(Command_Queue[ui], 1, &GlobMem_BUF_emb_l[ui], 0, 0, NULL, NULL);

	    if (errCode != CL_SUCCESS)
	    {
		    cout << endl << "Host-Error: Failed to write emb_l to GlobMem_BUF_emb_l" << endl << endl;
            return EXIT_FAILURE;
	    }

        errCode = clEnqueueMigrateMemObjects(Command_Queue[ui], 1, &GlobMem_BUF_DataIn_1[ui], 0, 0, NULL, NULL);

        if (errCode != CL_SUCCESS)
        {
            cout << endl << "Host-Error: Failed to write GlobMem_BUF_DataIn_1" << endl << endl;
            return EXIT_FAILURE;
        }

        for (i = 0; i < NUM_CUS; i++)
        {
            g_i_begin[ui * NUM_DEVS + i] = -1;
            g_i_end[ui * NUM_DEVS + i] = -1;
        }

        g_sparse_offset_group_batch_size[ui] = -1;
        g_sparse_index_group_batch_size[ui] = -1;
    }

    for (ui = 0; ui < NUM_DEVS; ui++)
    {    
	    if (Device_Detected[ui] == false) {
		    //cout << endl << "HOST-Error: Failed to get detect " << Target_Device_Name << " device" << endl << endl;
		    continue;
            //return EXIT_FAILURE;
	    } 
        ::clFlush(Command_Queue[ui]);
    }
    for (ui = 0; ui < NUM_DEVS; ui++)
    {    
	    if (Device_Detected[ui] == false) {
		    //cout << endl << "HOST-Error: Failed to get detect " << Target_Device_Name << " device" << endl << endl;
		    continue;
            //return EXIT_FAILURE;
	    } 
        ::clFinish(Command_Queue[ui]);
    }

    delete[] Platform_IDs;
	delete[] Device_IDs;

    return EXIT_SUCCESS;
}

void alveo_exit()
{
    int i;
    cl_uint ui;
    cl_int ret = CL_SUCCESS;
    for (ui = 0; ui < NUM_DEVS; ui++)
    {
        ret |= clReleaseDevice(Target_Device_ID[ui]);
        if (ret != CL_SUCCESS)
            cout << "clReleaseDevice failed" << endl;
        ret |= clReleaseMemObject(GlobMem_BUF_DataIn_1[ui]);
	    ret |= clReleaseMemObject(GlobMem_BUF_emb_l[ui]);
        ret |= clReleaseMemObject(GlobMem_BUF_lS_o[ui]);
        ret |= clReleaseMemObject(GlobMem_BUF_lS_i[ui]);
        //for (i = 0; i < NUM_CUS; i++)
            //ret |= clReleaseMemObject(GlobMem_BUF_RES[ui * NUM_DEVS + i]);
        ret |= clReleaseMemObject(GlobMem_BUF_RES[ui]);
        if (ret != CL_SUCCESS)
            cout << "clReleaseMemObject failed" << endl;
        for (i = 0; i < NUM_CUS; i++)
            ret |= clReleaseKernel(Kernel[ui * NUM_DEVS + i]);
        if (ret != CL_SUCCESS)
            cout << "clReleaseKernel failed" << endl;
        ret |= clReleaseProgram(Program[ui]);
        if (ret != CL_SUCCESS)
            cout << "clReleaseProgram failed" << endl;
        clReleaseCommandQueue(Command_Queue[ui]);
        clReleaseContext(Context[ui]);

        //for (i = 0; i < NUM_CUS; i++)
            //free(RES[ui * NUM_DEVS + i]);
        free(RES[ui]);
        free(DataIn_1[ui]);
        free(emb_ls[ui]);
        free(lS_o[ui]);
        free(lS_i[ui]);
    }

    free(emb_l);
}

void *compute(void *thread_arg)
{
    struct thread_data *my_data;
    //cl_event K_exe_event[NUM_CUS];
    int sparse_offset_group_batch_sizes[NUM_CUS];
    int i_begin[NUM_CUS];
    int i_end[NUM_CUS];
    cl_int	errCode;
    int sparse_offset_group_batch_size;
    int sparse_index_group_batch_size;
    int ui;
    int i;

    my_data = (struct thread_data *) thread_arg;
    errCode = CL_SUCCESS;
    sparse_offset_group_batch_size = my_data->sparse_offset_group_batch_size;
    sparse_index_group_batch_size = my_data->sparse_index_group_batch_size;
    ui = my_data->ui;

    for (i = 0; i < (int)table_nums[ui]; i++)
    {
        memcpy(lS_o[ui] + i * sparse_offset_group_batch_size, my_data->ptr_buf_o + table_lists[my_data->start_offset + i] * sparse_offset_group_batch_size, sparse_offset_group_batch_size * sizeof(int));
        memcpy(lS_i[ui] + i * sparse_index_group_batch_size, my_data->ptr_buf_i + table_lists[my_data->start_offset + i] * sparse_index_group_batch_size, sparse_index_group_batch_size * sizeof(int));
    }
    
    for (i = 0; i < NUM_CUS - 1; i++)
        sparse_offset_group_batch_sizes[i] = sparse_offset_group_batch_size / NUM_CUS;
    if (NUM_CUS >= 2)
        sparse_offset_group_batch_sizes[NUM_CUS - 1] = sparse_offset_group_batch_size - sparse_offset_group_batch_sizes[NUM_CUS - 2];

    for (i = 0; i < NUM_CUS; i++)
    {
        if (i == 0)
            i_begin[i] = 0;
        else
            i_begin[i] = i_end[i - 1];
        i_end[i] = i_begin[i] + sparse_offset_group_batch_sizes[i];
    }
	
    if (g_sparse_offset_group_batch_size[ui] != sparse_offset_group_batch_size)
    {
        g_sparse_offset_group_batch_size[ui] = sparse_offset_group_batch_size;
        for (i = 0; i < NUM_CUS; i++)
            errCode |= clSetKernelArg(Kernel[ui * NUM_DEVS + i], 6, sizeof(int), &sparse_offset_group_batch_size);
    }

    if (g_sparse_index_group_batch_size[ui] != sparse_index_group_batch_size)
    {
        g_sparse_index_group_batch_size[ui] = sparse_index_group_batch_size;
        for (i = 0; i < NUM_CUS; i++)
            errCode |= clSetKernelArg(Kernel[ui * NUM_DEVS + i], 7, sizeof(int), &sparse_index_group_batch_size);
    }

    for (i = 0; i < NUM_CUS; i++)
    {
        if (g_i_begin[ui * NUM_DEVS + i] != i_begin[i])
        {
            g_i_begin[ui * NUM_DEVS + i] = i_begin[i];
            errCode |= clSetKernelArg(Kernel[ui * NUM_DEVS + i], 8, sizeof(int), &i_begin[i]);
        }
        if (g_i_end[ui * NUM_DEVS + i] != i_end[i])
        {
            g_i_end[ui * NUM_DEVS + i] = i_end[i];
            errCode |= clSetKernelArg(Kernel[ui * NUM_DEVS + i], 9, sizeof(int), &i_end[i]);
        }
    }

    if (errCode != CL_SUCCESS)
        cout << endl << "Host-ERROR: Failed to set Kernel arguments" << endl << endl;
    // ------------------------------------------------------
    // Step 4.3: Copy Input data from Host to Global Memory
    // ------------------------------------------------------
    #ifdef ALL_MESSAGES
    cout << "HOST_Info: Copy Input data to Global Memory ..." << endl;
    #endif   
    errCode = clEnqueueMigrateMemObjects(Command_Queue[ui], 1, &GlobMem_BUF_lS_o[ui], 0, 0, NULL, NULL);

    if (errCode != CL_SUCCESS) {
	    cout << endl << "Host-Error: Failed to write lS_o to GlobMem_BUF_lS_o" << endl << endl;
	    //return EXIT_FAILURE;
    }

    errCode = clEnqueueMigrateMemObjects(Command_Queue[ui], 1, &GlobMem_BUF_lS_i[ui], 0, 0, NULL, NULL);
    if (errCode != CL_SUCCESS) {
	    cout << endl << "Host-Error: Failed to write lS_i to GlobMem_BUF_lS_i" << endl << endl;
	    //return EXIT_FAILURE;
    }
    
    //errCode = clEnqueueBarrierWithWaitList(Command_Queue[ui], 0, NULL, NULL);
    errCode = clEnqueueBarrier(Command_Queue[ui]);
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
    //size_t globalSize[1];
    //size_t localSize[1];
    //globalSize[0] = 1;
    //localSize[0]  = 1;
     
    for (i = 0; i < NUM_CUS; i++)
        //errCode |= clEnqueueTask(Command_Queue[ui], Kernel[ui * NUM_DEVS + i], 0, NULL, &K_exe_event[i]);
        errCode |= clEnqueueTask(Command_Queue[ui], Kernel[ui * NUM_DEVS + i], 0, NULL, NULL);

    if (errCode != CL_SUCCESS) {
	    cout << endl << "HOST-Error: Failed to execute Kernel" << endl << endl;
	    //return EXIT_FAILURE;
    }
    
    errCode |= clEnqueueBarrier(Command_Queue[ui]);
    if (errCode != CL_SUCCESS)
        cout << endl << "HOST-Error: Failed to enqueue barrier" << endl << endl;

    errCode |= clEnqueueMigrateMemObjects(Command_Queue[ui], 1, &GlobMem_BUF_RES[ui], CL_MIGRATE_MEM_OBJECT_HOST, 0, NULL, NULL);

    //for (i = 0; i < NUM_CUS; i++)
        //errCode |= clEnqueueMigrateMemObjects(Command_Queue[ui], 1, &GlobMem_BUF_RES[ui * NUM_DEVS + i], CL_MIGRATE_MEM_OBJECT_HOST, 1, &K_exe_event[i], NULL);

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
    
    ::clFinish(Command_Queue[ui]);
    //cout << "in host: " << i_begin[1] * g_arch_sparse_feature_size << endl;
    /*for (i = 1; i < NUM_CUS; i++)
        memcpy((void *)(RES[ui * NUM_DEVS + 0] + i_begin[i] * g_arch_sparse_feature_size), (const void *)RES[ui * NUM_DEVS + i], sparse_offset_group_batch_sizes[i] * g_arch_sparse_feature_size * sizeof(float));
    
    for (i = 0; i < NUM_CUS; i++)
        clReleaseEvent(K_exe_event[i]);*/
}

py::array_t<float> add(py::array_t<int> np_lS_o, py::array_t<int> np_lS_i, int sparse_offset_group_batch_size, int sparse_index_group_batch_size)
{	
    pthread_t threads[NUM_DEVS];
    int thread_id;
    struct thread_data td[NUM_DEVS];
    int i;
    int start_offset;
    int status;
    cl_uint ui;
    py::buffer_info buf_o = np_lS_o.request();
	py::buffer_info buf_i = np_lS_i.request();
	int *ptr_buf_o = reinterpret_cast<int *>(buf_o.ptr);
	int *ptr_buf_i = reinterpret_cast<int *>(buf_i.ptr);
    
    start_offset = 0;
    
    for (i = 0; i < NUM_DEVS; i++)
    {
        if (Device_Detected[i] == false) {
		    //cout << endl << "HOST-Error: Failed to get detect " << Target_Device_Name << " device" << endl << endl;
		    continue;
            //return EXIT_FAILURE;
	    }
        td[i].sparse_offset_group_batch_size = sparse_offset_group_batch_size;
        td[i].sparse_index_group_batch_size = sparse_index_group_batch_size;
        td[i].ui = i;
        td[i].ptr_buf_o = ptr_buf_o;
        td[i].ptr_buf_i = ptr_buf_i;
        td[i].start_offset = start_offset;
        thread_id = pthread_create(&threads[i], NULL, compute, (void *)&td[i]);
        if (thread_id < 0)
            perror("thread create error: ");
        start_offset += table_nums[i];
    }

    for (i = 0; i < NUM_DEVS; i++)
        pthread_join(threads[i], (void **)&status);

    #if NUM_DEVS == 2
    for (i = 0; i < sparse_offset_group_batch_size * g_arch_sparse_feature_size; i++)
    {
        RES[0][i] += RES[1][i];
        //RES[0 * NUM_DEVS][i] += RES[1 * NUM_DEVS][i];
    }
    #endif

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
