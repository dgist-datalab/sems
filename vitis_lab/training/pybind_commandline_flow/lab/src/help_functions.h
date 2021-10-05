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
int loadFile2Memory(const char *filename, char **result);
int *read_vector_file(const char *File_Name, int *Nb_Of_Elements);
void gen_int_values(int *Array, int Nb_Of_Elements, int Period);
void print_int_table(int *Array, int Nb_Of_Elements, int Nb_Of_Columns, int Nb_Of_Char_Per_Column);
float *read_emb_file(const char *File_Name, const unsigned int size, unsigned int start_offset, unsigned int *table_lists, unsigned int *orig_emb_l, unsigned int list_start_offset, unsigned int list_end_offset);
unsigned int *create_emb_l(const char *File_Name, const unsigned int size, unsigned int start_offset, int nun_sparse_features, unsigned int DataIn_1_size);
unsigned int *split_emb_l(unsigned int *emb_l, int num_devs, int *start_index);

//unsigned int *create_emb_l(const char *File_Name, const unsigned int size, unsigned int start_offset, int nun_sparse_features, unsigned int *start_tables, unsigned int data_size, unsigned int *start_offsets, unsigned int *sizes, int num_ddrs);

