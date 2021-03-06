/**********
Copyright (c) 2017, Xilinx, Inc.
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
Description: SDx Vector Addition using Blocking Pipes Operation
*******************************************************************************/

#define DATA_SIZE 4096
#define INCR_VALUE 10

#include <iostream>
#include <cstring>

//OpenCL utility layer include
#include "xcl.h"

int main(int argc, char** argv)
{
    if (argc != 1)
    {
        std::cout << "Usage: " << argv[0] << std::endl;
        return EXIT_FAILURE;
    }

    //Allocate Memory in Host Memory
    size_t vector_size_bytes = sizeof(int) * DATA_SIZE;

    int *source_input       = (int *) malloc(vector_size_bytes);
    int *source_hw_results  = (int *) malloc(vector_size_bytes);
    int *source_sw_results  = (int *) malloc(vector_size_bytes);

    // Create the test data and Software Result 
    for(int i = 0 ; i < DATA_SIZE ; i++){
        source_input[i] = i;
        source_sw_results[i] = i + INCR_VALUE;
        source_hw_results[i] = 0;
    }

//OPENCL HOST CODE AREA START
    //Create Program and Kernels. 
    xcl_world world = xcl_world_single();
    cl_program program = xcl_import_binary(world,"adder");
    cl_kernel krnl_adder_stage   = xcl_get_kernel(program, "krnl_adder_stage_rtl");
    //Creating additional Kernels
    cl_kernel krnl_input_stage   = xcl_get_kernel(program, "krnl_input_stage_rtl");
    cl_kernel krnl_output_stage  = xcl_get_kernel(program, "krnl_output_stage_rtl");
    
    
    // By-default xcl_world_single create command queues with sequential command.
    // For this example, user to replace command queue with out of order command queue
    clReleaseCommandQueue(world.command_queue);
    int err;
    world.command_queue = clCreateCommandQueue(world.context, world.device_id,
            CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE | CL_QUEUE_PROFILING_ENABLE,
            &err);
    if (err != CL_SUCCESS){
        std::cout << "Error: Failed to create a command queue!" << std::endl;
        std::cout << "Test failed" << std::endl;
        return EXIT_FAILURE;
    }


    //Allocate Buffer in Global Memory
    cl_mem buffer_input  = xcl_malloc(world, CL_MEM_READ_ONLY, vector_size_bytes);
    cl_mem buffer_output = xcl_malloc(world, CL_MEM_WRITE_ONLY, vector_size_bytes);

    //Copy input data to device global memory
    xcl_memcpy_to_device(world,buffer_input,source_input,vector_size_bytes);

    int inc = INCR_VALUE;
    int size = DATA_SIZE;
    //Set the Kernel Arguments
    xcl_set_kernel_arg(krnl_input_stage,0,sizeof(cl_mem),&buffer_input);
    xcl_set_kernel_arg(krnl_input_stage,1,sizeof(int),&size);
    xcl_set_kernel_arg(krnl_adder_stage,0,sizeof(int),&inc);
    xcl_set_kernel_arg(krnl_adder_stage,1,sizeof(int),&size);
    xcl_set_kernel_arg(krnl_output_stage,0,sizeof(cl_mem),&buffer_output);
    xcl_set_kernel_arg(krnl_output_stage,1,sizeof(int),&size);

    //Launch the Kernel
    xcl_run_kernel3d_nb(world,krnl_input_stage );
    xcl_run_kernel3d_nb(world,krnl_adder_stage );
    xcl_run_kernel3d_nb(world,krnl_output_stage);

    //wait for all kernels to finish their operations
    clFinish(world.command_queue);

    //Copy Result from Device Global Memory to Host Local Memory
    xcl_memcpy_from_device(world, source_hw_results, buffer_output,vector_size_bytes);

    //Release Device Memories and Kernels
    clReleaseMemObject(buffer_input);
    clReleaseMemObject(buffer_output);
    clReleaseKernel(krnl_input_stage);
    clReleaseKernel(krnl_adder_stage);
    clReleaseKernel(krnl_output_stage);
    clReleaseProgram(program);
    xcl_release_world(world);
//OPENCL HOST CODE AREA END
    
    // Compare the results of the Device to the simulation
    int match = 0;
    for (int i = 0 ; i < DATA_SIZE ; i++){
        if (source_hw_results[i] != source_sw_results[i]){
            std::cout << "Error: Result mismatch" << std::endl;
            std::cout << "i = " << i << " CPU result = " << source_sw_results[i]
                << " Device result = " << source_hw_results[i] << std::endl;
            match = 1;
            break;
        }
    }

    /* Release Memory from Host Memory*/
    free(source_input);
    free(source_hw_results);
    free(source_sw_results);

    if (match){
        std::cout << "TEST FAILED" << std::endl; 
        return EXIT_FAILURE;
    }
    std::cout << "TEST PASSED" << std::endl;
    return EXIT_SUCCESS; 
}
