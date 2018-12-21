__kernel void histogramKernel(__global unsigned int *image, __global unsigned int *result, unsigned int size) {
	size_t global_id = get_global_id(0);
	if (global_id < size) {
		unsigned int index = image[global_id];
		atomic_inc(result + index + 256 * (global_id % 3));
	}
}
