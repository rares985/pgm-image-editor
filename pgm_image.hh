#ifndef PGM_IMAGE_H_
#define PGM_IMAGE_H_

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>
#include <utility>
#include <sstream>
#include <string>
#include <fstream>
#include <algorithm>
#include <cmath>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <mpi.h>

#define MAXDIM 100
#define filter_size 9


class PGMImage
{
public:
	int n_row, n_col;
	int grayscale;
	int **image;
	std::vector<std::string> comments;
	std::string format;

	PGMImage();

	PGMImage(int n_row, int n_col);
	

    void FreeMemory();
};

void WriteImageToFile(std::string file_name, PGMImage img);
PGMImage ReadImage(std::string image_name);

#endif /* PGM_IMAGE_H_ */