//Rafal Kujawa Simple XOR encryption and decryption using OpenCL

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif
#include <ocl_macros.h>

//Common defines 
#define VENDOR_NAME "Intel(R) Corporation"
#define DEVICE_TYPE CL_DEVICE_TYPE_CPU

// defi
#define ARRAY_SIZE 8
#define SIZE_OF_WORK_GRP 1

//Function to measure time of events
double get_event_exec_time(cl_event event)
{
    cl_ulong start_time, end_time;
    /*Get start device counter for the event*/
    clGetEventProfilingInfo (event,
                    CL_PROFILING_COMMAND_START,
                    sizeof(cl_ulong),
                    &start_time,
                    NULL);
    /*Get end device counter for the event*/
    clGetEventProfilingInfo (event,
                    CL_PROFILING_COMMAND_END,
                    sizeof(cl_ulong),
                    &end_time,
                    NULL);
    /*Convert the counter values to milli seconds*/
    double total_time = (end_time - start_time) * 1e-6;
    return total_time;
}

//OpenCL kernel which is run for every work item created.
//The below const char string is compiled by the runtime complier
//when a program object is created with clCreateProgramWithSource 
//and built with clBuildProgram.
const char *decrypt_kernel =
"__kernel                                   \n"
"void decrypt_kernel(__global  char *array, \n"
"					__global char *secret)  \n"
"{                                          \n"
"    int i= get_global_id(0);               \n"
"    array[i] ^= secret[i%8];                 \n"
"}                                          \n";

const char *encrypt_kernel =
"__kernel                                   \n"
"void encrypt_kernel(__global  char *array, \n"
"					__global char *secret)  \n"
"{                                          \n"
"    int i= get_global_id(0);               \n"
"    array[i] ^= secret[i%8];                 \n"
"}                                          \n";

int main(void) {

    cl_int clStatus; //Keeps track of the error values returned. 

    // Get platform and device information
    cl_platform_id * platforms = NULL;

    // Set up the Platform. Take a look at the MACROs used in this file. 
    // These are defined in common/ocl_macros.h
    OCL_CREATE_PLATFORMS( platforms );

    // Get the devices list and choose the type of device you want to run on
    cl_device_id *device_list = NULL;
    OCL_CREATE_DEVICE( platforms[0], DEVICE_TYPE, device_list);

    // Create OpenCL context for devices in device_list
    cl_context context;
    cl_context_properties props[3] =
    {
        CL_CONTEXT_PLATFORM,
        (cl_context_properties)platforms[0],
        0
    };


    //set up context and associate it to device
    context = clCreateContext( NULL, num_devices, device_list, NULL, NULL, &clStatus);
    LOG_OCL_ERROR(clStatus, "clCreateContext Failed..." );

    // Create a command queue for the  device in device_list
    cl_command_queue command_queue = clCreateCommandQueue(context, device_list[0], CL_QUEUE_PROFILING_ENABLE, &clStatus);
    LOG_OCL_ERROR(clStatus, "clCreateCommandQueue Failed..." );


    // Allocate space for char arrays for HOST 
    char secret[ARRAY_SIZE] = { 22, 53, 44, 71, 66, 177, 253, 122 };
    char word[ARRAY_SIZE] = { 'p','r','o','g','r','a','m','\0'};
    char encrypted_word[ARRAY_SIZE] = {};

    int i;
//	for (i=0; i<ARRAY_SIZE; i++)
//		word[i] = 'a';
	printf("Word: %s\n\r",word);
	printf("Encryption key: %s\n\r",secret);




    // Create memory buffers on the device for each array
    cl_mem A_clmem = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
            sizeof(secret), secret, &clStatus);
    cl_mem C_clmem = clCreateBuffer(context, CL_MEM_WRITE_ONLY | CL_MEM_COPY_HOST_PTR,
            sizeof(word), word, &clStatus);

    cl_event write_event;
    // Copy the Buffer A  the device.  Blocking write to the device buffer.
    clStatus = clEnqueueWriteBuffer(command_queue, A_clmem, CL_TRUE, 0,
    		ARRAY_SIZE * sizeof(char), secret, 0, NULL, &write_event);
    LOG_OCL_ERROR(clStatus, "clEnqueueWriteBuffer Failed..." );


    // Create a programs from the kernel source
    cl_program program = clCreateProgramWithSource(context, 1,
            (const char **)&encrypt_kernel, NULL, &clStatus);
    LOG_OCL_ERROR(clStatus, "clCreateProgramWithSource Failed..." );


    cl_program program2 = clCreateProgramWithSource(context, 1,
               (const char **)&decrypt_kernel, NULL, &clStatus);
       LOG_OCL_ERROR(clStatus, "clCreateProgramWithSource Failed..." );

    // Build the programs
    clStatus = clBuildProgram(program, 1, device_list, NULL, NULL, NULL);
    if(clStatus != CL_SUCCESS)
        LOG_OCL_COMPILER_ERROR(program, device_list[0]);

    clStatus = clBuildProgram(program2, 1, device_list, NULL, NULL, NULL);
        if(clStatus != CL_SUCCESS)
            LOG_OCL_COMPILER_ERROR(program2, device_list[0]);

    // Create the OpenCL kernel
    cl_kernel kernel = clCreateKernel(program, "encrypt_kernel", &clStatus);

    // Set the arguments of the kernel.
    clStatus |= clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&A_clmem);
    clStatus |= clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&C_clmem);
    LOG_OCL_ERROR(clStatus, "clSetKernelArg Failed..." );

    cl_kernel kernel2 = clCreateKernel(program2, "decrypt_kernel", &clStatus);
    LOG_OCL_ERROR(clStatus, "sds Failed..." );

    clStatus |= clSetKernelArg(kernel2, 1, sizeof(cl_mem), (void *)&A_clmem);
    clStatus |= clSetKernelArg(kernel2, 0, sizeof(cl_mem), (void *)&C_clmem);
    LOG_OCL_ERROR(clStatus, "clSetKernelArg Failed..." );

    // Execute the OpenCL kernel on the list
    size_t global_size = ARRAY_SIZE; // Process one vector element in each work item
    size_t local_size = SIZE_OF_WORK_GRP;           // Process in work groups of size x
    cl_event saxpy_event;
    cl_event saxpy_event2;
    cl_event read_encrypt;
    cl_event read_decrypt;

    clStatus = clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL,
            &global_size, &local_size, 0, NULL, &saxpy_event);
    LOG_OCL_ERROR(clStatus, "clEnqueueNDRangeKernel Failed..." );

    // Read the memory buffer C_clmem on the device to the host allocated buffer C
   clStatus = clEnqueueReadBuffer(command_queue, C_clmem, CL_TRUE, 0,
		   sizeof(encrypted_word), encrypted_word, 1, &saxpy_event, &read_encrypt);

    clStatus = clEnqueueNDRangeKernel(command_queue, kernel2, 1, NULL,
                &global_size, &local_size, 1, &saxpy_event, &saxpy_event2);

    clStatus = clEnqueueReadBuffer(command_queue, C_clmem, CL_TRUE, 0,
                sizeof(word), word, 1, &saxpy_event2, &read_decrypt);
    LOG_OCL_ERROR(clStatus, "clEnqueueReadBuffer Failed..." );


    // Wait for all tasks from queue to complete.
    clStatus = clFinish(command_queue);

    printf("Encrypted word: %s\n\r",encrypted_word);
    printf("Decrypted word: %s\n\r",word);

    double exec_time;
    exec_time = get_event_exec_time(write_event);
    printf("Time taken to transfer char array to device = %lf ms\n",exec_time);
    exec_time = get_event_exec_time(saxpy_event);
    printf("Time taken to execute the encrypt kernel = %lf ms\n",exec_time);
    exec_time = get_event_exec_time(read_encrypt);
    printf("Time taken to read the result char after encrypt = %lf ms\n",exec_time);
    exec_time = get_event_exec_time(saxpy_event2);
    printf("Time taken to execute the decrypt kernel = %lf ms\n",exec_time);
    exec_time = get_event_exec_time(read_decrypt);
    printf("Time taken to read the result char after decrypt = %lf ms\n",exec_time);


    // Finally release all OpenCL objects and release the host buffers.
    clStatus = clReleaseKernel(kernel);
    clStatus = clReleaseKernel(kernel2);
    clStatus = clReleaseProgram(program);
    clStatus = clReleaseProgram(program2);
    clStatus = clReleaseMemObject(A_clmem);
    clStatus = clReleaseMemObject(C_clmem);
    clStatus = clReleaseCommandQueue(command_queue);
    clStatus = clReleaseContext(context);
    free(platforms);
    free(device_list);

    return 0;
}
