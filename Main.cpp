#include <stdio.h>
#include <string.h>
#include <time.h> // for clock
#include <math.h> // for ceil
#include "source.h"

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#include <CL/cl.h>
#endif
#include "ocl_macros.h"
#pragma warning(disable:4996) // fopen warrning visual studio

//#define DEBUG
//#define DEBUG_CL

#define VENDOR_NAME "Intel(R) Corporation"
#define DEVICE_TYPE CL_DEVICE_TYPE_CPU
#define BLOCK 16

//#define CPU
#define OPENCL
#define PRINT

float fileSize = 0;

unsigned char* Read_Source_File(const char *filename)
{
	long int size = 0;
	unsigned char *src = NULL;
	FILE *file = fopen(filename, "rb");

	if (!file) {
		printf("\nCan't open file.");
		return NULL;
	}
	//check filesize
	fseek(file, 0, SEEK_END);
	fileSize = ftell(file);
	//set pointer back to start of file
	rewind(file);

	//src = (char *)calloc(size + 1, sizeof(char)); //c
	src = new unsigned char[(int)fileSize + 1]();	//cpp

	fread(src, 1, sizeof(char) * (int)fileSize, file);
	
	src[(int)fileSize] = '\0'; // add \0 at the end of string
	//clean
	fclose(file);

	return src;
}

void PrintPlatformInfo(cl_platform_id platform)
{
	char queryBuffer[1024];
	cl_int clError;

	clError = clGetPlatformInfo(platform, CL_PLATFORM_NAME, 1024, &queryBuffer, NULL);
	if (clError == CL_SUCCESS)
	{
		printf("CL_PLATFORM_NAME   : %s\n", queryBuffer);
	}
	clError = clGetPlatformInfo(platform, CL_PLATFORM_VENDOR, 1024, &queryBuffer, NULL);
	if (clError == CL_SUCCESS)
	{
		printf("CL_PLATFORM_VENDOR : %s\n", queryBuffer);
	}
	clError = clGetPlatformInfo(platform, CL_PLATFORM_VERSION, 1024, &queryBuffer, NULL);
	if (clError == CL_SUCCESS)
	{
		printf("CL_PLATFORM_VERSION: %s\n", queryBuffer);
	}
	clError = clGetPlatformInfo(platform, CL_PLATFORM_PROFILE, 1024, &queryBuffer, NULL);
	if (clError == CL_SUCCESS)
	{
		printf("CL_PLATFORM_PROFILE: %s\n", queryBuffer);
	}
	clError = clGetPlatformInfo(platform, CL_PLATFORM_EXTENSIONS, 1024, &queryBuffer, NULL);
	if (clError == CL_SUCCESS)
	{
		printf("CL_PLATFORM_EXTENSIONS: %s\n", queryBuffer);
	}
	return;
}
void use_cpu_encryption(unsigned char* plaintext, unsigned char* ciphertext,
	unsigned char* buf, unsigned char* encryptdata, unsigned char* data,
	unsigned char* lastCiphertext, unsigned long* key_schedule)
{
	int i, j;
	for (i = 0; i<fileSize; i++) {
		plaintext[i % 16] = buf[i];
		if (i % 16 == 15) {
			aes_encrypt(plaintext, ciphertext, key_schedule, 128);
			for (j = i - 15; j < i + 1; j++) {
				encryptdata[j] = ciphertext[j % 16];
			}
			memset(plaintext, 0, BLOCK);
			memset(ciphertext, 0, BLOCK);
		}
		else if (i == fileSize - 1) {
			aes_encrypt(plaintext, lastCiphertext, key_schedule, 128);
			for (j = i - (i % 16); j < i + 1; j++) {
				encryptdata[j] = lastCiphertext[j % 16];
			}
		}

	}   //*/


	///* Decrypt Data
	for (i = 0; i<fileSize; i++) {
		ciphertext[i % 16] = encryptdata[i];
		if (i % 16 == 15) {
			aes_decrypt(ciphertext, plaintext, key_schedule, 128);
			for (j = i - 15; j < i + 1; j++) {
				data[j] = plaintext[j % 16];
			}
			memset(plaintext, 0, BLOCK);
			memset(ciphertext, 0, BLOCK);
		}
		else if (i == fileSize - 1) {
			aes_decrypt(lastCiphertext, plaintext, key_schedule, 128);
			for (j = i - (i % 16); j < i + 1; j++) {
				data[j] = plaintext[j % 16];
			}
		}

	}//*/
}

int Use_OpenCL_encryption(unsigned char* buf, unsigned char* encryptdata, unsigned char* data,
	 unsigned long* key_schedule, unsigned char* encrypt_kernel_src, unsigned char* decrypt_kernel_src,
	int keyBitSize, int numberOfIterations)
{
	cl_int clStatus; //Keeps track of the error values returned. 

// Get platform and device information
	cl_platform_id * platforms = NULL;

	// Set up the Platform. Take a look at the MACROs used in this file. 
	// These are defined in common/ocl_macros.h
	OCL_CREATE_PLATFORMS(platforms);

	// Get the devices list and choose the type of device you want to run on
	cl_device_id *device_list = NULL;
	OCL_CREATE_DEVICE(platforms[0], DEVICE_TYPE, device_list);

	// Create OpenCL context for devices in device_list
	cl_context context;
	cl_context_properties props[3] =
	{
		CL_CONTEXT_PLATFORM,
		(cl_context_properties)platforms[0],
		0
	};
#ifdef DEBUG_CL
	PrintPlatformInfo(platforms[0]);
#endif // DEBUG_CL

	//set up context and associate it to device
	context = clCreateContext(NULL, num_devices, device_list, NULL, NULL, &clStatus);
	LOG_OCL_ERROR(clStatus, "clCreateContext Failed...");

	// Create a command queue for the  device in device_list
	cl_command_queue command_queue = clCreateCommandQueue(context, device_list[0], CL_QUEUE_PROFILING_ENABLE, &clStatus);
	LOG_OCL_ERROR(clStatus, "clCreateCommandQueue Failed...");

	cl_mem input_clmem = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
		fileSize * sizeof(unsigned char), buf, &clStatus);
	cl_mem encrypted_clmem = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
		fileSize * sizeof(unsigned char), encryptdata, &clStatus);
	cl_mem decrypted_clmem = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
		fileSize * sizeof(unsigned char), data, &clStatus);
	cl_mem key_schedule_clmem = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
		60 * sizeof(unsigned long), key_schedule, &clStatus);

	// Copy data buffer and keys to device.  Blocking write to the device buffer. 
	clStatus = clEnqueueWriteBuffer(command_queue, input_clmem, CL_TRUE, 0,
		fileSize * sizeof(unsigned char), buf, 0, NULL, NULL);
	LOG_OCL_ERROR(clStatus, "clEnqueueWriteBuffer Failed...");

	clStatus = clEnqueueWriteBuffer(command_queue, key_schedule_clmem, CL_TRUE, 0,
		60 * sizeof(unsigned long), key_schedule, 0, NULL, NULL);
	LOG_OCL_ERROR(clStatus, "clEnqueueWriteBuffer Failed...");

	// Create a programs from the kernel source
	cl_program program = clCreateProgramWithSource(context, 1,
		(const char **)&encrypt_kernel_src, NULL, &clStatus);
	LOG_OCL_ERROR(clStatus, "clCreateProgramWithSource Failed...");
	// Create a programs from the kernel source
	cl_program program2 = clCreateProgramWithSource(context, 1,
		(const char **)&decrypt_kernel_src, NULL, &clStatus);
	LOG_OCL_ERROR(clStatus, "clCreateProgramWithSource Failed...");

	// Build the programs
	clStatus = clBuildProgram(program, 1, device_list, NULL, NULL, NULL);
	if (clStatus != CL_SUCCESS)
		LOG_OCL_COMPILER_ERROR(program, device_list[0]);
	// Build the programs
	clStatus = clBuildProgram(program2, 1, device_list, NULL, NULL, NULL);
	if (clStatus != CL_SUCCESS)
		LOG_OCL_COMPILER_ERROR(program, device_list[0]);

	cl_kernel kernel = clCreateKernel(program, "aes_encrypt", &clStatus);

	// Set the arguments of the kernel.
	clStatus |= clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&input_clmem);
	clStatus |= clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&encrypted_clmem);
	clStatus |= clSetKernelArg(kernel, 2, sizeof(cl_mem), &key_schedule_clmem);
	clStatus |= clSetKernelArg(kernel, 3, sizeof(cl_int), &keyBitSize);
	clStatus |= clSetKernelArg(kernel, 4, sizeof(cl_int), &numberOfIterations);
	LOG_OCL_ERROR(clStatus, "clSetKernelArg Failed...");

	cl_kernel kernel2 = clCreateKernel(program2, "aes_decrypt", &clStatus);

	// Set the arguments of the kernel.
	clStatus |= clSetKernelArg(kernel2, 0, sizeof(cl_mem), (void *)&encrypted_clmem);
	clStatus |= clSetKernelArg(kernel2, 1, sizeof(cl_mem), (void *)&decrypted_clmem);
	clStatus |= clSetKernelArg(kernel2, 2, sizeof(cl_mem), &key_schedule_clmem);
	clStatus |= clSetKernelArg(kernel2, 3, sizeof(cl_int), &keyBitSize);
	clStatus |= clSetKernelArg(kernel2, 4, sizeof(cl_int), &numberOfIterations);
	LOG_OCL_ERROR(clStatus, "clSetKernelArg Failed...");


	// Execute the OpenCL kernel on the list
	size_t global_size = numberOfIterations; // Process one vector element in each work item
	size_t local_size = 128;           // Process in work groups of size x
	cl_event encrypt_event;
	cl_event decrypt_event;
	cl_event read_encrypt;
	cl_event read_decrypt;

	clStatus = clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL,
		&global_size, &local_size, 0, NULL, &encrypt_event);
	LOG_OCL_ERROR(clStatus, "clEnqueueNDRangeKernel Failed...");

	// Read the memory buffer C_clmem on the device to the host allocated buffer C
	clStatus = clEnqueueReadBuffer(command_queue, encrypted_clmem, CL_TRUE, 0,
		fileSize * sizeof(unsigned char), encryptdata, 1, &encrypt_event, &read_encrypt);

	clStatus = clEnqueueNDRangeKernel(command_queue, kernel2, 1, NULL,
		&global_size, &local_size, 1, &read_encrypt, &decrypt_event);
	LOG_OCL_ERROR(clStatus, "clEnqueueNDRangeKernel Failed...");

	// Read the memory buffer C_clmem on the device to the host allocated buffer C
	clStatus = clEnqueueReadBuffer(command_queue, decrypted_clmem, CL_TRUE, 0,
		fileSize * sizeof(unsigned char), data, 1, &decrypt_event, &read_decrypt);

	// Wait for all tasks from queue to complete.
	clStatus = clFinish(command_queue);

	//clean up resourses used 
	clStatus = clReleaseKernel(kernel);
	clStatus = clReleaseProgram(program);
	clStatus = clReleaseMemObject(input_clmem);
	clStatus = clReleaseMemObject(encrypted_clmem);
	clStatus = clReleaseMemObject(decrypted_clmem);
	clStatus = clReleaseMemObject(key_schedule_clmem);
	clStatus = clReleaseCommandQueue(command_queue);
	clStatus = clReleaseContext(context);
	delete[] platforms;
	delete[] device_list;
	
}

int main(int argc, char** argv) {
	unsigned char *buf = nullptr;  //cpp
	unsigned char *encrypt_kernel_src = nullptr;
	unsigned char *decrypt_kernel_src = nullptr;
	
	int keyBitSize = 192, i = 0, j = 0;;
	clock_t start, end;
	double cpu_time_used;
	
	encrypt_kernel_src = Read_Source_File("Encrypt_Kernel.cl");
	decrypt_kernel_src = Read_Source_File("Decrypt_Kernel.cl");
	buf = Read_Source_File("test.txt");
	unsigned long key_schedule[60];
	unsigned char key[32] = {	0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,
								0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10,0x11,
								0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,
								0x1b,0x1c,0x1d,0x1e,0x1f };

	//generating the keys
	KeyExpansion(key, key_schedule, keyBitSize);
	int numberOfIterations = (int)ceil(fileSize / BLOCK);
	printf("FileSize = %d bytes\n", (int)fileSize);
#ifdef DEBUG
	printf("Number of pads %d\n", numberOfIterations);
#endif // DEBUG

	//start mearuing clock cycles
	start = clock();
	printf(" start: %d\n", start);
	//shared buffers
	unsigned char *data = new unsigned char[fileSize];
	unsigned char *encryptdata = new unsigned char[fileSize];
	//cpu buffers
	unsigned char plaintext[BLOCK];
	unsigned char ciphertext[BLOCK];
	unsigned char lastCiphertext[BLOCK];

	//OpenCL
#ifdef OPENCL
	int err = Use_OpenCL_encryption(buf, encryptdata, data, key_schedule, encrypt_kernel_src,
		decrypt_kernel_src, keyBitSize, numberOfIterations);
	if (err == -1)
	{
		return err;
	}
#endif // OPENCL

#ifdef CPU
	use_cpu_encryption(plaintext, ciphertext, buf, encryptdata, data, lastCiphertext, key_schedule);
#endif // CPU
	
	
#ifdef PRINT
	printf("Encrypted Data\n");
	for (i = 0; i < fileSize; i++)
		printf("%c", encryptdata[i]);

	printf("\nData Decrypted\n");
	for (i = 0; i < fileSize; i++)
		printf("%c", data[i]);
#endif // PRINT
#ifdef DEBUG
	 printf("\nLast ciphertext\n");
	 for (idx = 0; idx < 16; idx++) {
		 printf("%02x", ciphertext[idx]);
	 }
	 printf("\nLast plaintext\n");
	 for (idx = 0; idx < 16; idx++) {
		 printf("%c", plaintext[idx]);
	 }
#endif // DEBUG
	
    //clean shared buffers
	delete[] buf;
	delete[] data;
	delete[] encryptdata;

	//get final clock 
	end = clock();

	printf("\nCharacters encrypted: %d\nEnd: %d\n", (int)fileSize, end);
	//caluculate time in seconds
	cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
	printf("Seconds: %f\n", cpu_time_used);
	
	return 0;
}
