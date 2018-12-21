__kernel void histogramKernel(__global unsigned int *image, __global unsigned int *result, unsigned int size) {
	size_t global_id = get_global_id(0);
	if (global_id < size) {
	}
}
