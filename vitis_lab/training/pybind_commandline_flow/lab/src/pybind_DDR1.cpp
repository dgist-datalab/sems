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
//#include "pybind_K_ALL.h"

using namespace std;


extern "C" {
void DDR1(const float *in1,
		  float *out_r,     // Output Result
		  const unsigned int *emb_l,
		  const int *lS_o,
		  const int *lS_i,
		  const int arch_sparse_feature_size,
		  const int sparse_offset_group_batch_size,
		  const int sparse_index_group_batch_size,
          const int k_begin,
          const int k_end
) 
    {
#pragma HLS INTERFACE m_axi port=in1 offset=slave bundle=gmem0
#pragma HLS INTERFACE m_axi port=out_r offset=slave bundle=gmem4
#pragma HLS INTERFACE m_axi port=emb_l offset=slave bundle=gmem5
#pragma HLS INTERFACE m_axi port=lS_o offset=slave bundle=gmem6
#pragma HLS INTERFACE m_axi port=lS_i offset=slave bundle=gmem7
#pragma HLS INTERFACE s_axilite port=in1 bundle=control
#pragma HLS INTERFACE s_axilite port=out_r bundle=control
#pragma HLS INTERFACE s_axilite port=emb_l bundle=control
#pragma HLS INTERFACE s_axilite port=lS_o bundle=control
#pragma HLS INTERFACE s_axilite port=lS_i bundle=control
#pragma HLS INTERFACE s_axilite port=arch_sparse_feature_size bundle=control
#pragma HLS INTERFACE s_axilite port=k_begin bundle=control
#pragma HLS INTERFACE s_axilite port=k_end bundle=control
#pragma HLS INTERFACE s_axilite port=sparse_offset_group_batch_size bundle=control
#pragma HLS INTERFACE s_axilite port=sparse_index_group_batch_size bundle=control
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
        
        cout << "begin DDR1" << endl;
        out_r_size = sparse_offset_group_batch_size * arch_sparse_feature_size;
		
        // Initialize out_r to 0.0
        memset(out_r, 0, sizeof(float) * out_r_size);

        TABLE_LOOP: for (k = k_begin; k < k_end; k++)
		{
            sparse_index_group_batch = (int *)lS_i + k * sparse_index_group_batch_size;
			sparse_offset_group_batch = (int *)lS_o + k * sparse_offset_group_batch_size;
		    begin = sparse_offset_group_batch[0];

            table_addr = emb_l[k];
            
			// Embedding lookup as much as batch size
            BATCH_LOOP: for (i = 0; i < sparse_offset_group_batch_size; i++)
            {
                if (i == sparse_offset_group_batch_size - 1)
					end = sparse_index_group_batch_size;
				else
					end = sparse_offset_group_batch[i + 1];
                out_vec_addr = i * arch_sparse_feature_size;

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
        cout << "finished DDR1" << endl;
    }
}


