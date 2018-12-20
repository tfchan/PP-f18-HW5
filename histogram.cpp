#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include <fstream>
#include <iostream>

unsigned int * histogram(unsigned int *image_data, unsigned int _size) {
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

	return 0;
}
