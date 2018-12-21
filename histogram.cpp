#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include <fstream>
#include <iostream>
#include <CL/cl.h>

const char *kernel_source_filename = "histogram.cl";

char* readKernelSource(const char *filename) {
	size_t size;
	char *source;

	FILE *f = fopen(filename, "rb");
	if (!f) {
		std::cout << "Cannot open kernel source file " << kernel_source_filename << "\n";
		exit(EXIT_FAILURE);
	}
	// Get file size and allocate space
	fseek(f, 0, SEEK_END);
	size = ftell(f);
	source = new char[size + 1];
	source[size] = '\0';

	rewind(f);
	fread(source, sizeof(char), size, f);
	fclose(f);
	return source;
}

unsigned int * histogram(unsigned int *image_data, unsigned int _size) {
	cl_int error_num;
	cl_device_id device_id;
	cl_context context;
	cl_command_queue command_queue;
	cl_mem device_image, device_histogram_result;
	cl_event write_event, kernel_event;
	char *kernel_source;
	cl_program program;
	cl_kernel kernel;
	size_t global_size, local_size;

	// Get a GPU device ID
	error_num = clGetDeviceIDs(NULL, CL_DEVICE_TYPE_GPU, 1, &device_id, NULL);
	if (error_num != CL_SUCCESS) {
		std::cout << error_num << " Fail to get device ID\n";
		exit(EXIT_FAILURE);
	}

	// Get maximum work-items size in a work group
	error_num = clGetDeviceInfo(device_id, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(local_size), &local_size, NULL);
	if (error_num != CL_SUCCESS) {
		std::cout << error_num << " Fail to get device info\n";
		exit(EXIT_FAILURE);
	}

	// Create a context with the GPU device we got above
	context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &error_num);
	if (error_num != CL_SUCCESS) {
		std::cout << error_num << " Fail to create context\n";
		exit(EXIT_FAILURE);
	}

	// Create a command queue with the device in the context
	command_queue = clCreateCommandQueueWithProperties(context, device_id, NULL, &error_num);
	if (error_num != CL_SUCCESS) {
		std::cout << error_num << " Fail to create command queue\n";
		exit(EXIT_FAILURE);
	}

	// Create buffer for input and output
	device_image = clCreateBuffer(context, CL_MEM_READ_ONLY, _size * sizeof(unsigned int), NULL, &error_num);
	device_histogram_result = clCreateBuffer(context, CL_MEM_READ_WRITE, 256 * 3 * sizeof(unsigned int), NULL, &error_num);
	if (device_image == NULL || device_histogram_result == NULL) {
		std::cout << error_num << " Fail to create buffer\n";
		exit(EXIT_FAILURE);
	}

	// Copy image data to device buffer
	error_num = clEnqueueWriteBuffer(command_queue, device_image, CL_FALSE, 0, _size * sizeof(unsigned int), image_data, 0, NULL, &write_event);
	if (error_num != CL_SUCCESS) {
		std::cout << error_num << " Fail to enqueue write buffer\n";
		exit(EXIT_FAILURE);
	}

	// Create a program from external kernel source code
	kernel_source = readKernelSource(kernel_source_filename);
	program = clCreateProgramWithSource(context, 1, (const char **)&kernel_source, NULL, &error_num);
	if (error_num != CL_SUCCESS) {
		std::cout << error_num << " Fail to create program with source\n";
		exit(EXIT_FAILURE);
	}
	delete [] kernel_source;

	// Build the program
	error_num = clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);
	if (error_num != CL_SUCCESS) {
		std::cout << error_num << " Fail to build program\n";
		char buffer[1000];
		error_num = clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, NULL);
		if (error_num != CL_SUCCESS) 
			std::cout << error_num << " Fail to get build info\n";
		std::cout << buffer << "\n";
		exit(EXIT_FAILURE);
	}

	// Create kernel for the program
	kernel = clCreateKernel(program, "histogramKernel", &error_num);
	if (error_num != CL_SUCCESS) {
		std::cout << error_num << " Fail to create kernel\n";
		exit(EXIT_FAILURE);
	}

	// Set kernel arguments
	error_num = clSetKernelArg(kernel, 0, sizeof(cl_mem), &device_image);
	error_num |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &device_histogram_result);
	error_num |= clSetKernelArg(kernel, 2, sizeof(unsigned int), &_size);
	if (error_num != CL_SUCCESS) {
		std::cout << error_num << " Fail to set kernel arguments\n";
		exit(EXIT_FAILURE);
	}

	// Enqueue kernel execution command which execute after write event
	global_size = (_size % local_size == 0) ? (_size) : ((_size / local_size + 1) * local_size);
	error_num = clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL, &global_size, &local_size, 1, &write_event, &kernel_event);
	if (error_num != CL_SUCCESS) {
		std::cout << error_num << " Fail to enqueue kernel\n";
		exit(EXIT_FAILURE);
	}

	unsigned int *img = image_data;
	unsigned int *ref_histogram_results;
	unsigned int *ptr;

	ref_histogram_results = (unsigned int *)malloc(256 * 3 * sizeof(unsigned int));
	ptr = ref_histogram_results;
	memset(ref_histogram_results, 0x0, 256 * 3 * sizeof(unsigned int));

	// histogram of R
	for (unsigned int i = 0; i < _size; i += 3) {
		unsigned int index = img[i];
		ptr[index]++;
	}

	// histogram of G
	ptr += 256;
	for (unsigned int i = 1; i < _size; i += 3) {
		unsigned int index = img[i];
		ptr[index]++;
	}

	// histogram of B
	ptr += 256;
	for (unsigned int i = 2; i < _size; i += 3) {
		unsigned int index = img[i];
		ptr[index]++;
	}

	// Read histogram result back from device to host after kernel event
	error_num = clEnqueueReadBuffer(command_queue, device_histogram_result, CL_FALSE, 0, 256 * 3 * sizeof(unsigned int), ref_histogram_results, 1, &kernel_event, NULL);
	if (error_num != CL_SUCCESS) {
		std::cout << error_num << " Fail to enqueue read buffer\n";
		exit(EXIT_FAILURE);
	}

	// Submit all command in queue and wait for completion
	clFlush(command_queue);
	clFinish(command_queue);
	// Release resources
	clReleaseKernel(kernel);
	clReleaseProgram(program);
	clReleaseMemObject(device_histogram_result);
	clReleaseMemObject(device_image);
	clReleaseCommandQueue(command_queue);
	clReleaseContext(context);
	return ref_histogram_results;
}

unsigned int readInput(unsigned int **image, const char *filename) {
	unsigned int i = 0, a, input_size;
	std::fstream inFile(filename, std::ios_base::in);
	inFile >> input_size;
	*image = new unsigned int[input_size];
	while (inFile >> a) {
		(*image)[i++] = a;
	}
	inFile.close();
	return input_size;
}

void writeResult(unsigned int *result, const char *filename) {
	std::ofstream outFile(filename, std::ios_base::out);
	for(unsigned int i = 0; i < 256 * 3; ++i) {
		if (i % 256 == 0 && i != 0)
			outFile << std::endl;
		outFile << result[i] << ' ';
	}
	outFile.close();
}

int main(int argc, char const *argv[]) {
	unsigned int *image, input_size;
	unsigned int *histogram_results;

	input_size = readInput(&image, "input");

	histogram_results = histogram(image, input_size);

	writeResult(histogram_results, "0756002.out");

	delete [] image;
	delete [] histogram_results;

	return 0;
}
