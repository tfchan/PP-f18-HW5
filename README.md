# PP-f18_HW5

Image histogram with OpenCL

## Input format

A file named "input"

      9
      255 0 0
      0 100 255
      255 255 255

The first line represents the size of the one-dimension array, say N, followed by N/3 lines, each of which indicates a list of R, G, and B values (separated by a space) of a pixel. The following example indicates that the input image has three pixels, values of which are (255 0 0), (0 100 255), and (255 255 255), respectively.
## Build

      g++ histogram.cpp -o histogram -lOpenCL

## Run

      ./histogram
