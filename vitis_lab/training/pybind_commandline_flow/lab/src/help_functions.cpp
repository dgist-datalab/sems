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
#include <iostream>
#include <fstream>
#include <iomanip>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>

using namespace std;

#include "help_functions.h"
#include "pack.h"

// =========================================
// Helper Function: Loads program to memory
// =========================================
int loadFile2Memory(const char *filename, char **result) {

    int size = 0;

    std::ifstream stream(filename, std::ifstream::binary);
    if (!stream) {
		cout << "strerror: " << strerror(errno) << endl;
		return -1;
    }

    stream.seekg(0, stream.end);
    size = stream.tellg();
    stream.seekg(0, stream.beg);

    *result = new char[size + 1];
    stream.read(*result, size);
    if (!stream) {
        return -2;
    }
    stream.close();
    (*result)[size] = 0;
    return size;
}

// ==============================================
// Helper Function: Reads an array of int values
// ==============================================
int *read_vector_file(const char *File_Name, int *Nb_Of_Elements) {
  int val;
  int *Array;
  int temp_Nb_Of_Elements;
  fstream DataIn_File;
  void *ptr=nullptr;


  DataIn_File.open(File_Name,ios::in);
  if (! DataIn_File.is_open()) {
      cout << endl << "HOST-Error: Failed to open the " <<  File_Name << " for read" << endl << endl;
      exit(1);
  }

  temp_Nb_Of_Elements = 0;
  while (DataIn_File >> val) temp_Nb_Of_Elements ++;
  DataIn_File.close();

  if (posix_memalign(&ptr,4096,temp_Nb_Of_Elements*sizeof(int))) {
	  cout << endl << "HOST-Error: Out of Memory during memory allocation for Array" << endl << endl;
      exit(1);
  }
  Array = reinterpret_cast<int*>(ptr);

  DataIn_File.open(File_Name,ios::in);
  if (! DataIn_File.is_open()) {
      cout << endl << "HOST-Error: Failed to open the " <<  File_Name << " fore read" << endl << endl;
      exit(1);
  }

  temp_Nb_Of_Elements = 0;
  while (DataIn_File >> Array[temp_Nb_Of_Elements])
      temp_Nb_Of_Elements ++;
  DataIn_File.close();

  *Nb_Of_Elements = temp_Nb_Of_Elements;

  return Array;
}

float *read_emb_file(const char *File_Name, unsigned int size, unsigned int start_offset, unsigned int *table_lists, unsigned int *emb_l, unsigned int list_start_offset, unsigned int list_end_offset)
{
	float *Array;
	void *ptr = nullptr;
    int ret;
    int max_size = 2147479552;
    unsigned int array_offset = 0;
    unsigned int i;
    unsigned int new_size;
    unsigned int new_start_offset;
    unsigned int table_num;
	
	if (posix_memalign(&ptr,4096, size)) {
		cout << endl << "HOST-Error: Out of Memory during memory allocation for Array" << endl << endl;
    	exit(1);
  	}
  	Array = reinterpret_cast<float *>(ptr);
	int fd = open(File_Name, O_RDONLY);

    for (i = list_start_offset; i < list_end_offset; i++)
    {
        table_num = table_lists[i];
        new_size = (emb_l[table_num + 1] - emb_l[table_num]) * sizeof(float);
        new_start_offset = emb_l[table_num] * sizeof(float) + start_offset;
  
        while (new_size > max_size)
        {
            ret = pread(fd, Array + array_offset, max_size, new_start_offset);
            if (ret != max_size)
	        {
		        cout << "pread error" << endl;
                cout << "size: " << max_size << endl;
                //cout << "offset: " << offset << endl;
                cout << "ret: " << ret << endl;
                cout << "fd: " << fd << endl;
                perror("a");
	        }
            new_size -= max_size;
            new_start_offset += max_size;
            array_offset += max_size / sizeof(float);
        }
    
        ret = pread(fd, Array + array_offset, new_size, new_start_offset);
	    if (ret != new_size)
	    {
		    cout << "pread error" << endl;
            cout << "size: " << new_size << endl;
            //cout << "offset: " << offset << endl;
            cout << "ret: " << ret << endl;
            cout << "fd: " << fd << endl;
            perror("a");
	    }
        array_offset += new_size / sizeof(float); 
    }

	close(fd);
    cout << "1st vec" << endl;
    for (int i = 0; i < 16; i++)
        cout << Array[i] << " ";
    cout << endl;
    
    //cout << "1st element in Array: " << Array[0] << endl;

	return Array;
}


/*unsigned int *create_emb_l(const char *File_Name, unsigned int size, unsigned int start_offset, int num_sparse_features, unsigned int *start_tables, unsigned int data_size, unsigned int *start_offsets, unsigned int *sizes, int num_ddrs)
{
    unsigned int *Array;
	void *ptr = nullptr;
    int ret;
    int max_size = 2147479552;
    unsigned int array_offset = 0;
	
	if (posix_memalign(&ptr,4096, size)) {
		cout << endl << "HOST-Error: Out of Memory during memory allocation for Array" << endl << endl;
    	exit(1);
  	}
  	Array = reinterpret_cast<unsigned int *>(ptr);
	int fd = open(File_Name, O_RDONLY);
  
    while (size > max_size)
    {
        ret = pread(fd, Array + array_offset, max_size, start_offset);
        if (ret != max_size)
	    {
		    cout << "pread error" << endl;
            cout << "size: " << max_size << endl;
            //cout << "offset: " << offset << endl;
            cout << "ret: " << ret << endl;
            cout << "fd: " << fd << endl;
            perror("a");
	    }
        size -= max_size;
        start_offset += max_size;
        array_offset += max_size / sizeof(unsigned int);
    }
    
    ret = pread(fd, Array + array_offset, size, start_offset);
	if (ret != size)
	{
		cout << "pread error" << endl;
        cout << "size: " << size << endl;
        //cout << "offset: " << offset << endl;
        cout << "ret: " << ret << endl;
        cout << "fd: " << fd << endl;
        perror("a");
	}

	close(fd);
    
    int i;
    unsigned int quads[num_ddrs - 1];
    unsigned int fourth = data_size / num_ddrs;
    for (i = 0; i < num_ddrs - 1; i++)
        quads[i] = (i + 1) * fourth;
        
    int j = 0;

    start_tables[0] = 0;
    unsigned int begin;
    unsigned int end;

    for (i = 1; i < num_sparse_features; i++)
    {
        if (Array[i] >= quads[j] || num_sparse_features - i == num_ddrs - j)
        {
            if (quads[j] - Array[i - 1] >= Array[i] - quads[j] || start_tables[j] == i - 1)
                start_tables[++j] = i;
            else
                start_tables[++j] = i - 1;
        }
        if (j == num_ddrs)
            break;
    }
    
    for (i = 0; i < num_ddrs; i++)
    {
        begin = Array[start_tables[i]];
        if (i == num_ddrs - 1)
            end = start_offset + data_size;
        else
            end = Array[start_tables[i + 1]];
       
        sizes[i] = end - begin;
        //cout << "for " << i << ", begin: " << begin << ", end: " << end << ", size: " << sizes[i] << endl;
    }
    cout << endl;

    for (i = 0; i < num_ddrs; i++)
    {
        begin = start_tables[i];
        if (i == num_ddrs - 1)
            end = num_sparse_features;
        else
            end = start_tables[i + 1];
        start_offsets[i] = Array[begin];
        for (j = begin; j < end; j++)
        {
            Array[j] = (Array[j] - start_offsets[i]) / 4;
        }
    }

	return Array;
}*/

/*
unsigned int *create_emb_l(const char *File_Name, unsigned int size, unsigned int start_offset, int num_sparse_features, unsigned int *start_tables, unsigned int data_size, unsigned int *start_offsets, unsigned int *sizes)
{
    unsigned int *Array;
	void *ptr = nullptr;
    int ret;
    int max_size = 2147479552;
    unsigned int array_offset = 0;
	
	if (posix_memalign(&ptr,4096, size)) {
		cout << endl << "HOST-Error: Out of Memory during memory allocation for Array" << endl << endl;
    	exit(1);
  	}
  	Array = reinterpret_cast<unsigned int *>(ptr);
	int fd = open(File_Name, O_RDONLY);
  
    while (size > max_size)
    {
        ret = pread(fd, Array + array_offset, max_size, start_offset);
        if (ret != max_size)
	    {
		    cout << "pread error" << endl;
            cout << "size: " << max_size << endl;
            //cout << "offset: " << offset << endl;
            cout << "ret: " << ret << endl;
            cout << "fd: " << fd << endl;
            perror("a");
	    }
        size -= max_size;
        start_offset += max_size;
        array_offset += max_size / sizeof(unsigned int);
    }
    
    ret = pread(fd, Array + array_offset, size, start_offset);
	if (ret != size)
	{
		cout << "pread error" << endl;
        cout << "size: " << size << endl;
        //cout << "offset: " << offset << endl;
        cout << "ret: " << ret << endl;
        cout << "fd: " << fd << endl;
        perror("a");
	}

	close(fd);
    
    int i;
    unsigned int quads[3];
    unsigned int fourth = data_size / 4;
    for (i = 0; i < 3; i++)
        quads[i] = (i + 1) * fourth;
        
    int j = 0;

    start_tables[0] = 0;
    unsigned int begin;
    unsigned int end;

    for (i = 1; i < num_sparse_features; i++)
    {
        if (Array[i] >= quads[j] || num_sparse_features - i == 4 - j)
        {
            if (quads[j] - Array[i - 1] >= Array[i] - quads[j] || start_tables[j] == i - 1)
                start_tables[++j] = i;
            else
                start_tables[++j] = i - 1;
        }
        if (j == 4)
            break;
    }
    
    for (i = 0; i < 4; i++)
    {
        begin = Array[start_tables[i]];
        if (i == 3)
            end = start_offset + data_size;
        else
            end = Array[start_tables[i + 1]];
       
        sizes[i] = end - begin;
        //cout << "for " << i << ", begin: " << begin << ", end: " << end << ", size: " << sizes[i] << endl;
    }
    cout << endl;

    for (i = 0; i < 4; i++)
    {
        begin = start_tables[i];
        if (i == 3)
            end = num_sparse_features;
        else
            end = start_tables[i + 1];
        start_offsets[i] = Array[begin];
        for (j = begin; j < end; j++)
        {
            Array[j] = (Array[j] - start_offsets[i]) / 4;
        }
    }

	return Array;
}*/


unsigned int *create_emb_l(const char *File_Name, unsigned int size, unsigned int start_offset, int num_sparse_features, unsigned int DataIn_1_size)
{
    unsigned int *Array;
	void *ptr = nullptr;
    int ret;
    int max_size = 2147479552;
    unsigned int array_offset = 0;
	
	if (posix_memalign(&ptr,4096, size + sizeof(unsigned int))) {
		cout << endl << "HOST-Error: Out of Memory during memory allocation for Array" << endl << endl;
    	exit(1);
  	}
  	Array = reinterpret_cast<unsigned int *>(ptr);
	int fd = open(File_Name, O_RDONLY);
  
    while (size > max_size)
    {
        ret = pread(fd, Array + array_offset, max_size, start_offset);
        if (ret != max_size)
	    {
		    cout << "pread error" << endl;
            cout << "size: " << max_size << endl;
            //cout << "offset: " << offset << endl;
            cout << "ret: " << ret << endl;
            cout << "fd: " << fd << endl;
            perror("a");
	    }
        size -= max_size;
        start_offset += max_size;
        array_offset += max_size / sizeof(unsigned int);
    }
    
    ret = pread(fd, Array + array_offset, size, start_offset);
	if (ret != size)
	{
		cout << "pread error" << endl;
        cout << "size: " << size << endl;
        //cout << "offset: " << offset << endl;
        cout << "ret: " << ret << endl;
        cout << "fd: " << fd << endl;
        perror("a");
	}

	close(fd);
    
    for (int i = 0; i < num_sparse_features; i++)
        Array[i] = (Array[i] - 108) / sizeof(unsigned int);    
    Array[num_sparse_features] = DataIn_1_size / sizeof(unsigned int);
    
    cout << "1st element in Array: " << Array[0] << endl;

	return Array;
}

unsigned int *split_emb_l(unsigned int *emb_l, int num_devs, int *start_index)
{
     
    //DATAIN_1_SIZE
}



// ==================================================
// Helper Function: Generate an array of init values
//   Period - defines the number of unique values
//            for example, if Period=4 then we will
//            have the following values:
//            0 1 2 3 0 1 2 3
// ==================================================
void gen_int_values(int *Array, int Nb_Of_Elements, int Period) {
	int val;

	val = 0;
	for (int i=0; i<Nb_Of_Elements; i++) {
		Array[i] = val;
		val ++;
		if (val == Period) val = 0;
	}
}

// ==================================================
// Helper Function: Print table of integer values
// ==================================================
void print_int_table(int *Array, int Nb_Of_Elements, int Nb_Of_Columns, int Nb_Of_Char_Per_Column) {
	int col;

	col = 0;
	for (int i=0; i<Nb_Of_Elements; i++) {
		cout << setw(Nb_Of_Char_Per_Column) << Array[i];
		col ++;
		if (col == Nb_Of_Columns) {
			col = 0;
			cout << endl;
		}
	}

}

