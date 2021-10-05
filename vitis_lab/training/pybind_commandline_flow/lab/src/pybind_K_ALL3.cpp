/*******************************************************************************
Vendor: Xilinx
Associated Filename: krnl_vadd.cpp
Purpose: Vitis vector addition example
*******************************************************************************
Copyright (C) 2019 XILINX, Inc.

This file contains confidential and proprietary information of Xilinx, Inc. and
is protected under U.S. and international copyright and other intellectual
property laws.

DISCLAIMER
This disclaimer is not a license and does not grant any rights to the materials
distributed herewith. Except as otherwise provided in a valid license issued to
you by Xilinx, and to the maximum extent permitted by applicable law:
(1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND WITH ALL FAULTS, AND XILINX
HEREBY DISCLAIMS ALL WARRANTIES AND CONDITIONS, EXPRESS, IMPLIED, OR STATUTORY,
INCLUDING BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, NON-INFRINGEMENT, OR
FITNESS FOR ANY PARTICULAR PURPOSE; and (2) Xilinx shall not be liable (whether
in contract or tort, including negligence, or under any other theory of
liability) for any loss or damage of any kind or nature related to, arising under
or in connection with these materials, including for any direct, or any indirect,
special, incidental, or consequential loss or damage (including loss of data,
profits, goodwill, or any type of loss or damage suffered as a result of any
action brought by a third party) even if such damage or loss was reasonably
foreseeable or Xilinx had been advised of the possibility of the same.

CRITICAL APPLICATIONS
Xilinx products are not designed or intended to be fail-safe, or for use in any
application requiring fail-safe performance, such as life-support or safety
devices or systems, Class III medical devices, nuclear facilities, applications
related to the deployment of airbags, or any other applications that could lead
to death, personal injury, or severe property or environmental damage
(individually and collectively, "Critical Applications"). Customer assumes the
sole risk and liability of any use of Xilinx products in Critical Applications,
subject only to applicable laws and regulations governing limitations on product
liability.

THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS PART OF THIS FILE AT
ALL TIMES.

*******************************************************************************/

//------------------------------------------------------------------------------
//
// kernel:  vadd
//
// Purpose: Demonstrate Vector Add Kernel
//

#include <iostream>
#include <stdlib.h>
#include <cstring>
#include <string>
#include <vector>
#include "kernel.h"
//#include "pybind_K_ALL.h"

//#define OUT_R_SIZE 1024
//#define NUM_SPARSE_FEATURES 26
//#define ARCH_SPARSE_FEATURE_SIZE 16
//#define LS_SIZE 2048
//#define CACHE_SIZE 1024
//#define PAGE_SIZE 4 * 1024
//#define NUM_PAGE_ELEM 1024
//#define BATCH_SIZE 64

using namespace std;

/*
extern "C" {
void K_VADD(const float *in1,
		  float *out_r,     // Output Result
		  const unsigned int *emb_l,
		  const int *lS_o,
		  const int *lS_i,
          const int num_sparse_features,
		  const int arch_sparse_feature_size,
		  //const int sparse_offset_group_batch_size,
		  const int sparse_index_group_batch_size,
          const int i_begin,
          const int batch_size
) 
    {
#pragma HLS INTERFACE m_axi port=in1 offset=slave bundle=gmem0
#pragma HLS INTERFACE m_axi port=out_r offset=slave bundle=gmem1
#pragma HLS INTERFACE m_axi port=emb_l offset=slave bundle=gmem2
#pragma HLS INTERFACE m_axi port=lS_o offset=slave bundle=gmem3
#pragma HLS INTERFACE m_axi port=lS_i offset=slave bundle=gmem4
#pragma HLS INTERFACE s_axilite port=in1 bundle=control
#pragma HLS INTERFACE s_axilite port=out_r bundle=control
#pragma HLS INTERFACE s_axilite port=emb_l bundle=control
#pragma HLS INTERFACE s_axilite port=lS_o bundle=control
#pragma HLS INTERFACE s_axilite port=lS_i bundle=control
#pragma HLS INTERFACE s_axilite port=num_sparse_features bundle=control
#pragma HLS INTERFACE s_axilite port=arch_sparse_feature_size bundle=control
//#pragma HLS INTERFACE s_axilite port=sparse_offset_group_batch_size bundle=control
#pragma HLS INTERFACE s_axilite port=sparse_index_group_batch_size bundle=control
#pragma HLS INTERFACE s_axilite port=i_begin bundle=control
#pragma HLS INTERFACE s_axilite port=batch_size bundle=control
#pragma HLS INTERFACE s_axilite port=return bundle = control

        int *sparse_index_group_batch;
		int *sparse_offset_group_batch;
        int *sparse_index_group_batch_l;
        const int sparse_offset_group_batch_size = BATCH_SIZE * 2;
		unsigned int offset;
		int begin, end;
		int i, j, k, l;
		int vector_num;
		int out_r_size;
        unsigned int table_addr;
        int out_vec_addr;
        //int batch_size = i_end - i_begin;
        int j_begin, j_end;
        out_r_size = batch_size * arch_sparse_feature_size;
        unsigned int emb_l_l[EMB_L_SIZE];
        float out_r_l[OUT_R_SIZE];
        int lS_o_l[LS_SIZE];
        int lS_i_l[LS_SIZE];
        float v_cache[CACHE_SIZE];
        float tmp[16];

        //cout << "num_sparse_features: " << num_sparse_features << endl;
        //cout << "batch_size: " << batch_size << endl;
        //cout << "arch_sparse_feature_size: " << arch_sparse_feature_size << endl;
        //cout << "sparse_offset_group_batch_size: " << sparse_offset_group_batch_size << endl;
        //cout << "sparse_index_group_batch_size: " << sparse_index_group_batch_size << endl;
  
        memcpy(emb_l_l, emb_l, num_sparse_features * sizeof(unsigned int));

        //cout << "batch_size: " << batch_size << endl;

        LS_O_TABLE_LOOP: for (k = 0; k < 26; k++)
        //LS_O_TABLE_LOOP: for (k = 0; k < num_sparse_features; k++)
        {
            sparse_offset_group_batch = (int *)lS_o + k * sparse_offset_group_batch_size;
            LS_O_LOOP: for (i = 0; i < BATCH_SIZE; i++)
            //LS_O_LOOP: for (i = 0; i < batch_size; i++)
            {
                #pragma HLS PIPELINE
                lS_o_l[k * (BATCH_SIZE + 1) + i] = sparse_offset_group_batch[i_begin + i] - i_begin; 
            }
         
            if (i_begin == 0)
                lS_o_l[k * (BATCH_SIZE + 1) + i] = sparse_offset_group_batch[i];
            else
                lS_o_l[k * (BATCH_SIZE + 1) + i] = sparse_index_group_batch_size - i_begin;
        }

        LS_I_TABLE_LOOP: for (k = 0; k < 26; k++)
        //LS_I_TABLE_LOOP: for (k = 0; k < num_sparse_features; k++)
        {
            #pragma HLS PIPELINE
            sparse_index_group_batch = (int *)lS_i + k * sparse_index_group_batch_size;
            j_begin = lS_o_l[k * (BATCH_SIZE + 1)];
            j_end = lS_o_l[(k + 1) * (BATCH_SIZE + 1) - 1];
            sparse_index_group_batch_l = (int *)lS_i_l + k * (j_end - j_begin);
            memcpy(sparse_index_group_batch_l + j_begin, sparse_index_group_batch + i_begin + j_begin, sizeof(int) * j_end - j_begin);
            
        }
 
        // Initialize out_r to 0.0

        memset(out_r_l, 0, sizeof(float) * out_r_size);

        // Cache embedding vectors
        TABLE_LOOP: for (k = 0; k < 26; k++)
        //TABLE_LOOP: for (k = 0; k < num_sparse_features; k++)
        {
            sparse_index_group_batch = lS_i_l + k * (j_end - j_begin);
            sparse_offset_group_batch = lS_o_l + k * (BATCH_SIZE + 1);
            table_addr = emb_l_l[k];
            //cout << "table: " << k << endl;
            // Embedding lookup as much as batch size
            BATCH_LOOP: for (i = 0; i < BATCH_SIZE; i++)
            //BATCH_LOOP: for (i = 0; i < batch_size; i++)
            {
                out_vec_addr = i * arch_sparse_feature_size;
                //cout << "sparse_offset_group_batch[" << i << "]: " << sparse_offset_group_batch[i] << ", i + 1: " << sparse_offset_group_batch[i + 1] << endl; 
                REQUEST_LOOP: for (j = sparse_offset_group_batch[i]; j < sparse_offset_group_batch[i + 1]; j++)
                {
                    // emb_lookup_cpp
                    #pragma HLS PIPELINE
                    vector_num = sparse_index_group_batch[j];
                    offset = table_addr + (unsigned int)vector_num * (unsigned int)arch_sparse_feature_size;
                    memcpy(v_cache + out_vec_addr, in1 + offset, sizeof(float) * arch_sparse_feature_size);
                }
                //if (i >= batch_size)
                    //cout << "[2222222]: " << i << endl;
            }

            BATCH_LOOP2: for (i = 0; i < BATCH_SIZE; i++)
            //BATCH_LOOP2: for (i = 0; i < batch_size; i++)
            {
                //cout << "begin: " << begin << ", sparse_offset_group_batch[i + 1]" << sparse_offset_group_batch[i + 1] << endl;               
                out_vec_addr = i * arch_sparse_feature_size;
                REQUEST_LOOP2: for (j = sparse_offset_group_batch[i]; j < sparse_offset_group_batch[i + 1]; j++)
                {
                    //cout << "in request loop2" << endl;
                    offset = arch_sparse_feature_size * j;
                    
                    memcpy(tmp, out_r_l + out_vec_addr, arch_sparse_feature_size * sizeof(float));   

                    COPY_LOOP2: for (l = 0; l < 16; l++)
                    //COPY_LOOP2: for (l = 0; l < arch_sparse_feature_size; l++)
                    {
                        #pragma HLS PIPELINE
                        out_r_l[out_vec_addr + l] = tmp[l] + v_cache[offset + (unsigned int)l];
                    }
                }
                //if (i >= batch_size)
                    //cout << "[3333333]: " << i << endl;
            }
        }
                    
        memcpy(out_r, out_r_l, sizeof(float) * out_r_size);
    }
}*/

/*extern "C" {
void K_VADD(const float *in1,
		  float *out_r,     // Output Result
		  const unsigned int *emb_l,
		  const int *lS_o,
		  const int *lS_i,
          const int num_sparse_features,
		  const int arch_sparse_feature_size,
		  const int sparse_offset_group_batch_size,
		  const int sparse_index_group_batch_size,
          const int i_begin,
          const int i_end
) 
    {
#pragma HLS INTERFACE m_axi port=in1 offset=slave bundle=gmem0
#pragma HLS INTERFACE m_axi port=out_r offset=slave bundle=gmem1
#pragma HLS INTERFACE m_axi port=emb_l offset=slave bundle=gmem2
#pragma HLS INTERFACE m_axi port=lS_o offset=slave bundle=gmem3
#pragma HLS INTERFACE m_axi port=lS_i offset=slave bundle=gmem4
#pragma HLS INTERFACE s_axilite port=in1 bundle=control
#pragma HLS INTERFACE s_axilite port=out_r bundle=control
#pragma HLS INTERFACE s_axilite port=emb_l bundle=control
#pragma HLS INTERFACE s_axilite port=lS_o bundle=control
#pragma HLS INTERFACE s_axilite port=lS_i bundle=control
#pragma HLS INTERFACE s_axilite port=num_sparse_features bundle=control
#pragma HLS INTERFACE s_axilite port=arch_sparse_feature_size bundle=control
#pragma HLS INTERFACE s_axilite port=sparse_offset_group_batch_size bundle=control
#pragma HLS INTERFACE s_axilite port=sparse_index_group_batch_size bundle=control
#pragma HLS INTERFACE s_axilite port=i_begin bundle=control
#pragma HLS INTERFACE s_axilite port=i_end bundle=control
#pragma HLS INTERFACE s_axilite port=return bundle = control

        int *sparse_index_group_batch;
		int *sparse_offset_group_batch;
        int *sparse_index_group_batch_l;
		unsigned int offset;
		unsigned int new_offset;
        unsigned int page_offset;
        int begin, end;
		int i, j, k, l;
		int vector_num;
		int out_r_size;
        unsigned int table_addr;
        int out_vec_addr;
        int batch_size = i_end - i_begin;
        int j_begin, j_end;
        out_r_size = batch_size * arch_sparse_feature_size;
        unsigned int emb_l_l[EMB_L_SIZE];
        float out_r_l[OUT_R_SIZE];
        int lS_o_l[LS_SIZE];
        int lS_i_l[LS_SIZE];
        float v_cache[CACHE_SIZE];
        float tmp[16];
        float page[NUM_PAGE_ELEM];
  
        memcpy(emb_l_l, emb_l, num_sparse_features * sizeof(unsigned int));

        LS_O_TABLE_LOOP: for (k = 0; k < 26; k++)
        //LS_O_TABLE_LOOP: for (k = 0; k < num_sparse_features; k++)
        {
            sparse_offset_group_batch = (int *)lS_o + k * sparse_offset_group_batch_size;
            LS_O_LOOP: for (i = 0; i < batch_size; i++)
            {
                #pragma HLS PIPELINE
                lS_o_l[k * (batch_size + 1) + i] = sparse_offset_group_batch[i_begin + i] - i_begin;
            }
                       
            if (i_begin == 0)
                lS_o_l[k * (batch_size + 1) + i] = sparse_offset_group_batch[i];
            else
                lS_o_l[k * (batch_size + 1) + i] = sparse_index_group_batch_size - i_begin;
        }

        LS_I_TABLE_LOOP: for (k = 0; k < 26; k++)
        //LS_I_TABLE_LOOP: for (k = 0; k < num_sparse_features; k++)
        {
            #pragma HLS PIPELINE
            sparse_index_group_batch = (int *)lS_i + k * sparse_index_group_batch_size;
            j_begin = lS_o_l[k * (batch_size + 1)];
            j_end = lS_o_l[(k + 1) * (batch_size + 1) - 1];
            sparse_index_group_batch_l = (int *)lS_i_l + k * (j_end - j_begin);
           
            memcpy(sparse_index_group_batch_l + j_begin, sparse_index_group_batch + i_begin + j_begin, sizeof(int) * j_end - j_begin);
            
        }
 
        // Initialize out_r to 0.0

        memset(out_r_l, 0, sizeof(float) * out_r_size);

        TABLE_LOOP: for (k = 0; k < 26; k++)
        //TABLE_LOOP: for (k = 0; k < num_sparse_features; k++)
        {
            sparse_index_group_batch = lS_i_l + k * (j_end - j_begin);
            sparse_offset_group_batch = lS_o_l + k * (batch_size + 1);
            table_addr = emb_l_l[k];
            // Embedding lookup as much as batch size
            // For each embedding, first load the page the embedding is located in in PAGE
            // Then find the embedding from PAGE and copy to V_CACHE
            BATCH_LOOP: for (i = 0; i < batch_size; i++)
            {
                out_vec_addr = i * arch_sparse_feature_size;
                
                REQUEST_LOOP: for (j = sparse_offset_group_batch[i]; j < sparse_offset_group_batch[i + 1]; j++)
                {
                    // emb_lookup_cpp
                    #pragma HLS PIPELINE
                    vector_num = sparse_index_group_batch[j];
                    offset = table_addr + (unsigned int)vector_num * (unsigned int)arch_sparse_feature_size;
                    // The start address of page the embedding is located in
                    new_offset = (offset / NUM_PAGE_ELEM) * NUM_PAGE_ELEM;
                    // The start address of the embedding located in PAGE
                    page_offset = offset % NUM_PAGE_ELEM;
                    // Load the page the embedding is located in in PAGE
                    memcpy(page, in1 + new_offset, PAGE_SIZE);
                    // Find the actual embedding from PAGE and copy to V_CACHE
                    memcpy(v_cache + out_vec_addr, page + page_offset, sizeof(float) * arch_sparse_feature_size);
                }
            }

            BATCH_LOOP2: for (i = 0; i < batch_size; i++)
            {
                out_vec_addr = i * arch_sparse_feature_size;
                REQUEST_LOOP2: for (j = sparse_offset_group_batch[i]; j < sparse_offset_group_batch[i + 1]; j++)
                {              
                    offset = arch_sparse_feature_size * j;
                    
                    memcpy(tmp, out_r_l + out_vec_addr, arch_sparse_feature_size * sizeof(float));   

                    COPY_LOOP2: for (l = 0; l < 16; l++)
                    //COPY_LOOP2: for (l = 0; l < arch_sparse_feature_size; l++)
                    {
                        #pragma HLS PIPELINE
                        out_r_l[out_vec_addr + l] = tmp[l] + v_cache[offset + (unsigned int)l];
                    }
                }
            }
        }
                    
        memcpy(out_r, out_r_l, sizeof(float) * out_r_size);
    }
}*/

extern "C" {
void K_VADD(const float *in1,
		  float *out_r,     // Output Result
		  const unsigned int *emb_l,
		  const int *lS_o,
		  const int *lS_i,
          const int num_sparse_features,
		  //const int arch_sparse_feature_size,
		  const int sparse_offset_group_batch_size,
		  const int sparse_index_group_batch_size,
          const int i_begin,
          const int i_end
) 
    {
#pragma HLS INTERFACE m_axi port=in1 offset=slave bundle=gmem0
#pragma HLS INTERFACE m_axi port=out_r offset=slave bundle=gmem1
#pragma HLS INTERFACE m_axi port=emb_l offset=slave bundle=gmem2
#pragma HLS INTERFACE m_axi port=lS_o offset=slave bundle=gmem3
#pragma HLS INTERFACE m_axi port=lS_i offset=slave bundle=gmem4
#pragma HLS INTERFACE s_axilite port=in1 bundle=control
#pragma HLS INTERFACE s_axilite port=out_r bundle=control
#pragma HLS INTERFACE s_axilite port=emb_l bundle=control
#pragma HLS INTERFACE s_axilite port=lS_o bundle=control
#pragma HLS INTERFACE s_axilite port=lS_i bundle=control
#pragma HLS INTERFACE s_axilite port=num_sparse_features bundle=control
//#pragma HLS INTERFACE s_axilite port=arch_sparse_feature_size bundle=control
#pragma HLS INTERFACE s_axilite port=sparse_offset_group_batch_size bundle=control
#pragma HLS INTERFACE s_axilite port=sparse_index_group_batch_size bundle=control
#pragma HLS INTERFACE s_axilite port=i_begin bundle=control
#pragma HLS INTERFACE s_axilite port=i_end bundle=control
#pragma HLS INTERFACE s_axilite port=return bundle = control

        int *sparse_index_group_batch;
		int *sparse_offset_group_batch;
        int *sparse_index_group_batch_l;
		unsigned int offset;
		int begin, end;
		int i, j, k, l;
		int vector_num;
		int out_r_size;
        unsigned int table_addr;
        int out_vec_addr;
        int batch_size = i_end - i_begin;
        int j_begin, j_end;
        out_r_size = batch_size * ARCH_SPARSE_FEATURE_SIZE;
        unsigned int emb_l_l[NUM_SPARSE_FEATURES];
        float out_r_l[OUT_R_SIZE];
        int lS_o_l[LS_SIZE];
        int lS_i_l[LS_SIZE];
        float v_cache[CACHE_SIZE];
        float tmp[ARCH_SPARSE_FEATURE_SIZE];
        
        memcpy(emb_l_l, emb_l, num_sparse_features * sizeof(unsigned int));

        LS_O_TABLE_LOOP: for (k = 0; k < num_sparse_features; k++)
        {
            sparse_offset_group_batch = (int *)lS_o + k * sparse_offset_group_batch_size;
            LS_O_LOOP: for (i = 0; i < batch_size; i++)
            {
                #pragma HLS PIPELINE
                lS_o_l[k * (batch_size + 1) + i] = sparse_offset_group_batch[i_begin + i] - i_begin;
            }
                       
            if (i_begin == 0)
                lS_o_l[k * (batch_size + 1) + i] = sparse_offset_group_batch[i];
            else
                lS_o_l[k * (batch_size + 1) + i] = sparse_index_group_batch_size - i_begin;
        }

        LS_I_TABLE_LOOP: for (k = 0; k < num_sparse_features; k++)
        {
            #pragma HLS PIPELINE
            sparse_index_group_batch = (int *)lS_i + k * sparse_index_group_batch_size;
            j_begin = lS_o_l[k * (batch_size + 1)];
            j_end = lS_o_l[(k + 1) * (batch_size + 1) - 1];
            sparse_index_group_batch_l = (int *)lS_i_l + k * (j_end - j_begin);
           
            memcpy(sparse_index_group_batch_l + j_begin, sparse_index_group_batch + i_begin + j_begin, sizeof(int) * j_end - j_begin); 
        }
 
        // Initialize out_r to 0.0
        memset(out_r_l, 0, sizeof(float) * out_r_size);

        // Cache embedding vectors
        TABLE_LOOP: for (k = 0; k < num_sparse_features; k++)
        {
            sparse_index_group_batch = lS_i_l + k * (j_end - j_begin);
            sparse_offset_group_batch = lS_o_l + k * (batch_size + 1);
            table_addr = emb_l_l[k];
            
            //cout << "before batch_loop for k: " << k << endl;
            // Embedding lookup as much as batch size
            BATCH_LOOP: for (i = 0; i < batch_size; i++)
            {
                out_vec_addr = i * ARCH_SPARSE_FEATURE_SIZE;

                REQUEST_LOOP: for (j = sparse_offset_group_batch[i]; j < sparse_offset_group_batch[i + 1]; j++)
                {
                    // emb_lookup_cpp
                    #pragma HLS PIPELINE
                    vector_num = sparse_index_group_batch[j];
                    offset = table_addr + (unsigned int)vector_num * (unsigned int) ARCH_SPARSE_FEATURE_SIZE;
                    memcpy(v_cache + out_vec_addr, in1 + offset, sizeof(float) * ARCH_SPARSE_FEATURE_SIZE);                   
                    //memcpy(v_cache + out_vec_addr, in1 + offset, sizeof(float) * ARCH_SPARSE_FEATURE_SIZE);
                    //memcpy(v_cache + out_vec_addr, in1 + offset, sizeof(float) * ARCH_SPARSE_FEATURE_SIZE);
                }
            }
            //cout << "before batch_loop2 for k: " << k << endl;

            BATCH_LOOP2: for (i = 0; i < batch_size; i++)
            {
                out_vec_addr = i * ARCH_SPARSE_FEATURE_SIZE;
                REQUEST_LOOP2: for (j = sparse_offset_group_batch[i]; j < sparse_offset_group_batch[i + 1]; j++)
                {
                    offset = ARCH_SPARSE_FEATURE_SIZE * j;
                    
                    memcpy(tmp, out_r_l + out_vec_addr, ARCH_SPARSE_FEATURE_SIZE * sizeof(float));   

                    COPY_LOOP2: for (l = 0; l < ARCH_SPARSE_FEATURE_SIZE; l++)
                    {
                        #pragma HLS PIPELINE
                        out_r_l[out_vec_addr + l] = tmp[l] + v_cache[offset + (unsigned int)l];
                    }
                }
            }
        }
                 
        memcpy(out_r, out_r_l, sizeof(float) * out_r_size);
    }
}

/*
extern "C" {
void K_VADD(const float *in1,
		  float *out_r,     // Output Result
		  const unsigned int *emb_l,
		  const int *lS_o,
		  const int *lS_i,
          //const int num_sparse_features,
		  //const int arch_sparse_feature_size,
		  const int sparse_offset_group_batch_size,
		  const int sparse_index_group_batch_size,
          const int i_begin,
          const int i_end
) 
    {
#pragma HLS INTERFACE m_axi port=in1 offset=slave bundle=gmem0
#pragma HLS INTERFACE m_axi port=out_r offset=slave bundle=gmem1
#pragma HLS INTERFACE m_axi port=emb_l offset=slave bundle=gmem2
#pragma HLS INTERFACE m_axi port=lS_o offset=slave bundle=gmem3
#pragma HLS INTERFACE m_axi port=lS_i offset=slave bundle=gmem4
#pragma HLS INTERFACE s_axilite port=in1 bundle=control
#pragma HLS INTERFACE s_axilite port=out_r bundle=control
#pragma HLS INTERFACE s_axilite port=emb_l bundle=control
#pragma HLS INTERFACE s_axilite port=lS_o bundle=control
#pragma HLS INTERFACE s_axilite port=lS_i bundle=control
//#pragma HLS INTERFACE s_axilite port=num_sparse_features bundle=control
//#pragma HLS INTERFACE s_axilite port=arch_sparse_feature_size bundle=control
#pragma HLS INTERFACE s_axilite port=sparse_offset_group_batch_size bundle=control
#pragma HLS INTERFACE s_axilite port=sparse_index_group_batch_size bundle=control
#pragma HLS INTERFACE s_axilite port=i_begin bundle=control
#pragma HLS INTERFACE s_axilite port=i_end bundle=control
#pragma HLS INTERFACE s_axilite port=return bundle = control

        int *sparse_index_group_batch;
		int *sparse_offset_group_batch;
        int *sparse_index_group_batch_l;
		unsigned int offset;
		int begin, end;
		int i, j, k, l;
		int vector_num;
		int out_r_size;
        unsigned int table_addr;
        int out_vec_addr;
        int batch_size = i_end - i_begin;
        int j_begin, j_end;
        out_r_size = batch_size * ARCH_SPARSE_FEATURE_SIZE;
        unsigned int emb_l_l[NUM_SPARSE_FEATURES];
        float out_r_l[OUT_R_SIZE];
        int lS_o_l[LS_SIZE];
        int lS_i_l[LS_SIZE];
        float v_cache[CACHE_SIZE];
        float tmp[ARCH_SPARSE_FEATURE_SIZE];
  
        memcpy(emb_l_l, emb_l, NUM_SPARSE_FEATURES * sizeof(unsigned int));

        LS_O_TABLE_LOOP: for (k = 0; k < NUM_SPARSE_FEATURES; k++)
        {
            sparse_offset_group_batch = (int *)lS_o + k * sparse_offset_group_batch_size;
            LS_O_LOOP: for (i = 0; i < batch_size; i++)
            {
                #pragma HLS PIPELINE
                lS_o_l[k * (batch_size + 1) + i] = sparse_offset_group_batch[i_begin + i] - i_begin;
            }
                       
            if (i_begin == 0)
                lS_o_l[k * (batch_size + 1) + i] = sparse_offset_group_batch[i];
            else
                lS_o_l[k * (batch_size + 1) + i] = sparse_index_group_batch_size - i_begin;
        }

        LS_I_TABLE_LOOP: for (k = 0; k < NUM_SPARSE_FEATURES; k++)
        {
            #pragma HLS PIPELINE
            sparse_index_group_batch = (int *)lS_i + k * sparse_index_group_batch_size;
            j_begin = lS_o_l[k * (batch_size + 1)];
            j_end = lS_o_l[(k + 1) * (batch_size + 1) - 1];
            sparse_index_group_batch_l = (int *)lS_i_l + k * (j_end - j_begin);
           
            memcpy(sparse_index_group_batch_l + j_begin, sparse_index_group_batch + i_begin + j_begin, sizeof(int) * j_end - j_begin);
            
        }
 
        // Initialize out_r to 0.0

        memset(out_r_l, 0, sizeof(float) * out_r_size);

        // Cache embedding vectors
        TABLE_LOOP: for (k = 0; k < NUM_SPARSE_FEATURES; k++)
        {
            sparse_index_group_batch = lS_i_l + k * (j_end - j_begin);
            sparse_offset_group_batch = lS_o_l + k * (batch_size + 1);
            table_addr = emb_l_l[k];
            // Embedding lookup as much as batch size
            BATCH_LOOP: for (i = 0; i < batch_size; i++)
            {
                out_vec_addr = i * ARCH_SPARSE_FEATURE_SIZE;
                
                REQUEST_LOOP: for (j = sparse_offset_group_batch[i]; j < sparse_offset_group_batch[i + 1]; j++)
                {
                    // emb_lookup_cpp
                    #pragma HLS PIPELINE
                    vector_num = sparse_index_group_batch[j];
                    offset = table_addr + (unsigned int)vector_num * (unsigned int)ARCH_SPARSE_FEATURE_SIZE;
                    memcpy(v_cache + out_vec_addr, in1 + offset, sizeof(float) * ARCH_SPARSE_FEATURE_SIZE);                   
                    //memcpy(v_cache + out_vec_addr, in1 + offset, sizeof(float) * ARCH_SPARSE_FEATURE_SIZE);
                    //memcpy(v_cache + out_vec_addr, in1 + offset, sizeof(float) * ARCH_SPARSE_FEATURE_SIZE);
                }
            }

            BATCH_LOOP2: for (i = 0; i < batch_size; i++)
            {
                out_vec_addr = i * ARCH_SPARSE_FEATURE_SIZE;
                REQUEST_LOOP2: for (j = sparse_offset_group_batch[i]; j < sparse_offset_group_batch[i + 1]; j++)
                {
                    offset = ARCH_SPARSE_FEATURE_SIZE * j;
                    
                    memcpy(tmp, out_r_l + out_vec_addr, ARCH_SPARSE_FEATURE_SIZE * sizeof(float));   

                    COPY_LOOP2: for (l = 0; l < ARCH_SPARSE_FEATURE_SIZE; l++)
                    {
                        #pragma HLS PIPELINE
                        out_r_l[out_vec_addr + l] = tmp[l] + v_cache[offset + (unsigned int)l];
                    }
                }
            }
        }
                 
        memcpy(out_r, out_r_l, sizeof(float) * out_r_size);
    }
}*/

/*
extern "C" {
void K_VADD(const float *in1,
		  float *out_r,     // Output Result
		  const unsigned int *emb_l,
		  const int *lS_o,
		  const int *lS_i,
          const int num_sparse_features,
		  const int arch_sparse_feature_size,
		  const int sparse_offset_group_batch_size,
		  const int sparse_index_group_batch_size,
          const int i_begin,
          const int i_end
) 
    {
#pragma HLS INTERFACE m_axi port=in1 offset=slave bundle=gmem0
#pragma HLS INTERFACE m_axi port=out_r offset=slave bundle=gmem1
#pragma HLS INTERFACE m_axi port=emb_l offset=slave bundle=gmem2
#pragma HLS INTERFACE m_axi port=lS_o offset=slave bundle=gmem3
#pragma HLS INTERFACE m_axi port=lS_i offset=slave bundle=gmem4
#pragma HLS INTERFACE s_axilite port=in1 bundle=control
#pragma HLS INTERFACE s_axilite port=out_r bundle=control
#pragma HLS INTERFACE s_axilite port=emb_l bundle=control
#pragma HLS INTERFACE s_axilite port=lS_o bundle=control
#pragma HLS INTERFACE s_axilite port=lS_i bundle=control
#pragma HLS INTERFACE s_axilite port=num_sparse_features bundle=control
#pragma HLS INTERFACE s_axilite port=arch_sparse_feature_size bundle=control
#pragma HLS INTERFACE s_axilite port=sparse_offset_group_batch_size bundle=control
#pragma HLS INTERFACE s_axilite port=sparse_index_group_batch_size bundle=control
#pragma HLS INTERFACE s_axilite port=i_begin bundle=control
#pragma HLS INTERFACE s_axilite port=i_end bundle=control
#pragma HLS INTERFACE s_axilite port=return bundle = control

        int *sparse_index_group_batch;
		int *sparse_offset_group_batch;
		unsigned int offset;
        unsigned int new_offset;
        unsigned int page_offset;
		int begin, end;
		int i, j, k, l, m;
		int vector_num;
		int out_r_size;
        unsigned int table_addr;
        int out_vec_addr;
        size_t size;
        float page[NUM_PAGE_ELEM * 50];
        //float our_r_l[1024];
        
        size = sizeof(float) * (size_t)arch_sparse_feature_size;

        out_r_size = (i_end - i_begin) * arch_sparse_feature_size;

        m = 0;

        // Initialize out_r to 0.0
        memset(out_r, 0, sizeof(float) * out_r_size);

        TABLE_LOOP: for (k = 0; k < num_sparse_features; k++)
		{
            sparse_index_group_batch = (int *)lS_i + k * sparse_index_group_batch_size;
			sparse_offset_group_batch = (int *)lS_o + k * sparse_offset_group_batch_size;
		    begin = sparse_offset_group_batch[i_begin];

            table_addr = emb_l[k];
			// Embedding lookup as much as batch size
            BATCH_LOOP: for (i = i_begin; i < i_end; i++)
            {
                if (i == sparse_offset_group_batch_size - 1)
					end = sparse_index_group_batch_size;
				else
					end = sparse_offset_group_batch[i + 1];
                out_vec_addr = (i - i_begin) * arch_sparse_feature_size;
                
                REQUEST_LOOP: for (j = begin; j < end; j++)
                {
                    //emb_lookup_cpp
                    vector_num = sparse_index_group_batch[j];
					offset = table_addr + (unsigned int)vector_num * (unsigned int)arch_sparse_feature_size; 
                    new_offset = (offset / NUM_PAGE_ELEM) * NUM_PAGE_ELEM;
                    page_offset = offset % NUM_PAGE_ELEM + (m++ % 50) * NUM_PAGE_ELEM;

                    // Get the page that the embedding is located in
                    memcpy(page + (m % 50) * NUM_PAGE_ELEM, in1 + new_offset, PAGE_SIZE);
                    // Find the actual embedding from the page and add to out_r
                    
                    for (l = 0; l < arch_sparse_feature_size; l++)
                    {
                        #pragma HLS PIPELINE
                        out_r[out_vec_addr + l] += page[page_offset + l];
                        //out_r[out_vec_addr + l] += in1[offset + (unsigned int)l];
                    }
				}
                begin = end;
			}					
		}

        //cout << "finished" << endl;
    }
}*/
/*
extern "C" {
void K_VADD(const float *in1,
		  float *out_r,     // Output Result
		  const unsigned int *emb_l,
		  const int *lS_o,
		  const int *lS_i,
          const int num_sparse_features,
		  const int arch_sparse_feature_size,
		  const int sparse_offset_group_batch_size,
		  const int sparse_index_group_batch_size,
          const int i_begin,
          const int i_end
) 
    {
#pragma HLS INTERFACE m_axi port=in1 offset=slave bundle=gmem0
#pragma HLS INTERFACE m_axi port=out_r offset=slave bundle=gmem1
#pragma HLS INTERFACE m_axi port=emb_l offset=slave bundle=gmem2
#pragma HLS INTERFACE m_axi port=lS_o offset=slave bundle=gmem3
#pragma HLS INTERFACE m_axi port=lS_i offset=slave bundle=gmem4
#pragma HLS INTERFACE s_axilite port=in1 bundle=control
#pragma HLS INTERFACE s_axilite port=out_r bundle=control
#pragma HLS INTERFACE s_axilite port=emb_l bundle=control
#pragma HLS INTERFACE s_axilite port=lS_o bundle=control
#pragma HLS INTERFACE s_axilite port=lS_i bundle=control
#pragma HLS INTERFACE s_axilite port=num_sparse_features bundle=control
#pragma HLS INTERFACE s_axilite port=arch_sparse_feature_size bundle=control
#pragma HLS INTERFACE s_axilite port=sparse_offset_group_batch_size bundle=control
#pragma HLS INTERFACE s_axilite port=sparse_index_group_batch_size bundle=control
#pragma HLS INTERFACE s_axilite port=i_begin bundle=control
#pragma HLS INTERFACE s_axilite port=i_end bundle=control
#pragma HLS INTERFACE s_axilite port=return bundle = control

        int *sparse_index_group_batch;
		int *sparse_offset_group_batch;
		unsigned int offset;
		int begin, end;
		int i, j, k, l;
		int vector_num;
		int out_r_size;
        unsigned int table_addr;
        int out_vec_addr;
        float out_r_l[1024];

        memset(out_r_l, 0, sizeof(float) * 1024);
        out_vec_addr;
        offset = 0;

        for (k = 0; k < 10; k++)
        {
            out_vec_addr = k;
            for (l = 0; l < arch_sparse_feature_size; l++)
            {
                #pragma HLS PIPELINE
                out_r_l[out_vec_addr + l] += in1[offset + (unsigned int)l];
            }
        }
    }
}*/

/*
extern "C" {
void K_VADD(const float *in1,
		  float *out_r,     // Output Result
		  const unsigned int *emb_l,
		  const int *lS_o,
		  const int *lS_i,
          const int num_sparse_features,
		  const int arch_sparse_feature_size,
		  const int sparse_offset_group_batch_size,
		  const int sparse_index_group_batch_size,
          const int i_begin,
          const int i_end
) 
    {
#pragma HLS INTERFACE m_axi port=in1 offset=slave bundle=gmem0
#pragma HLS INTERFACE m_axi port=out_r offset=slave bundle=gmem1
#pragma HLS INTERFACE m_axi port=emb_l offset=slave bundle=gmem2
#pragma HLS INTERFACE m_axi port=lS_o offset=slave bundle=gmem3
#pragma HLS INTERFACE m_axi port=lS_i offset=slave bundle=gmem4
#pragma HLS INTERFACE s_axilite port=in1 bundle=control
#pragma HLS INTERFACE s_axilite port=out_r bundle=control
#pragma HLS INTERFACE s_axilite port=emb_l bundle=control
#pragma HLS INTERFACE s_axilite port=lS_o bundle=control
#pragma HLS INTERFACE s_axilite port=lS_i bundle=control
#pragma HLS INTERFACE s_axilite port=num_sparse_features bundle=control
#pragma HLS INTERFACE s_axilite port=arch_sparse_feature_size bundle=control
#pragma HLS INTERFACE s_axilite port=sparse_offset_group_batch_size bundle=control
#pragma HLS INTERFACE s_axilite port=sparse_index_group_batch_size bundle=control
#pragma HLS INTERFACE s_axilite port=i_begin bundle=control
#pragma HLS INTERFACE s_axilite port=i_end bundle=control
#pragma HLS INTERFACE s_axilite port=return bundle = control

        int *sparse_index_group_batch;
		int *sparse_offset_group_batch;
		unsigned int offset;
		int begin, end;
		int i, j, k, l;
		int vector_num;
		int out_r_size;
        unsigned int table_addr;
        int out_vec_addr;
        float out_r_l[OUT_R_SIZE];
        float tmp[16];

        out_r_size = (i_end - i_begin) * arch_sparse_feature_size;

        // Initialize out_r to 0.0
        memset(out_r_l, 0, sizeof(float) * out_r_size);

        TABLE_LOOP: for (k = 0; k < num_sparse_features; k++)
		{
            sparse_index_group_batch = (int *)lS_i + k * sparse_index_group_batch_size;
			sparse_offset_group_batch = (int *)lS_o + k * sparse_offset_group_batch_size;
		    begin = sparse_offset_group_batch[i_begin];

            table_addr = emb_l[k];
            
			// Embedding lookup as much as batch size
            BATCH_LOOP: for (i = i_begin; i < i_end; i++)
            {
                if (i == sparse_offset_group_batch_size - 1)
					end = sparse_index_group_batch_size;
				else
					end = sparse_offset_group_batch[i + 1];
                out_vec_addr = (i - i_begin) * arch_sparse_feature_size;

                REQUEST_LOOP: for (j = begin; j < end; j++)
                {
                    //emb_lookup_cpp
                    vector_num = sparse_index_group_batch[j];
					offset = table_addr + (unsigned int)vector_num * (unsigned int)arch_sparse_feature_size; 
                    memcpy(tmp, out_r_l + out_vec_addr, arch_sparse_feature_size * sizeof(float));   

                    for (l = 0; l < arch_sparse_feature_size; l++)
                    {
                        #pragma HLS PIPELINE
                        out_r_l[out_vec_addr + l] = tmp[l] + in1[offset + (unsigned int)l];
                        //out_r_l[out_vec_addr + l] += in1[offset + (unsigned int)l];
                    }
				}
                begin = end;
			}					
		}
        memcpy(out_r, out_r_l, sizeof(float) * out_r_size);
        //cout << "finished" << endl;
    }
}*/

/*
extern "C" {
void K_VADD(const float *in1,
		  float *out_r,     // Output Result
		  const unsigned int *emb_l,
		  const int *lS_o,
		  const int *lS_i,
          const int num_sparse_features,
		  const int arch_sparse_feature_size,
		  const int sparse_offset_group_batch_size,
		  const int sparse_index_group_batch_size,
          const int i_begin,
          const int i_end
) 
    {
#pragma HLS INTERFACE m_axi port=in1 offset=slave bundle=gmem0
#pragma HLS INTERFACE m_axi port=out_r offset=slave bundle=gmem1
#pragma HLS INTERFACE m_axi port=emb_l offset=slave bundle=gmem2
#pragma HLS INTERFACE m_axi port=lS_o offset=slave bundle=gmem3
#pragma HLS INTERFACE m_axi port=lS_i offset=slave bundle=gmem4
#pragma HLS INTERFACE s_axilite port=in1 bundle=control
#pragma HLS INTERFACE s_axilite port=out_r bundle=control
#pragma HLS INTERFACE s_axilite port=emb_l bundle=control
#pragma HLS INTERFACE s_axilite port=lS_o bundle=control
#pragma HLS INTERFACE s_axilite port=lS_i bundle=control
#pragma HLS INTERFACE s_axilite port=num_sparse_features bundle=control
#pragma HLS INTERFACE s_axilite port=arch_sparse_feature_size bundle=control
#pragma HLS INTERFACE s_axilite port=sparse_offset_group_batch_size bundle=control
#pragma HLS INTERFACE s_axilite port=sparse_index_group_batch_size bundle=control
#pragma HLS INTERFACE s_axilite port=i_begin bundle=control
#pragma HLS INTERFACE s_axilite port=i_end bundle=control
#pragma HLS INTERFACE s_axilite port=return bundle = control

        int *sparse_index_group_batch;
		int *sparse_offset_group_batch;
		unsigned int offset;
		int begin, end;
		int i, j, k, l;
		int vector_num;
		int out_r_size;
        unsigned int table_addr;
        int out_vec_addr;

        out_r_size = (i_end - i_begin) * arch_sparse_feature_size;

        // Initialize out_r to 0.0
        memset(out_r, 0, sizeof(float) * out_r_size);

        TABLE_LOOP: for (k = 0; k < num_sparse_features; k++)
		{
            sparse_index_group_batch = (int *)lS_i + k * sparse_index_group_batch_size;
			sparse_offset_group_batch = (int *)lS_o + k * sparse_offset_group_batch_size;
		    begin = sparse_offset_group_batch[i_begin];

            table_addr = emb_l[k];
            
			// Embedding lookup as much as batch size
            BATCH_LOOP: for (i = i_begin; i < i_end; i++)
            {
                if (i == sparse_offset_group_batch_size - 1)
					end = sparse_index_group_batch_size;
				else
					end = sparse_offset_group_batch[i + 1];
                out_vec_addr = (i - i_begin) * arch_sparse_feature_size;

                REQUEST_LOOP: for (j = begin; j < end; j++)
                {
                    //emb_lookup_cpp
                    vector_num = sparse_index_group_batch[j];
					offset = table_addr + (unsigned int)vector_num * (unsigned int)arch_sparse_feature_size; 

                    for (l = 0; l < arch_sparse_feature_size; l++)
                    {
                        #pragma HLS PIPELINE
                        out_r[out_vec_addr + l] += in1[offset + (unsigned int)l];
                    }
				}
                begin = end;
			}					
		}

        //cout << "finished" << endl;
    }
}*/
