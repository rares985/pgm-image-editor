#include "pgm_image.hh"
#include "utils.hh"


PGMImage::PGMImage() {

}

PGMImage::PGMImage(int n_row, int n_col) : n_row(n_row), n_col(n_col) {
    image = new int*[n_row];
    for (int i = 0; i < n_row; i++)
        image[i] = new int[n_col];
    
    for (int i = 0; i < n_row; i++)
        for (int j = 0; j < n_col; j++)
            image[i][j] = 0;
}

void PGMImage::FreeMemory() {
    for(int i = 0; i < n_row; i++) {
        delete[] image[i];
    }
    delete[] image;
}

PGMImage ReadImage(std::string image_name) 
{
	int cols, rows;
	std::string line;
	std::vector<std::string> tokens;
	std::vector<std::string> comments;
	std::string format;
	std::stringstream ss;
	std::ifstream fin(image_name.c_str());

	if (!fin.is_open())
	{
        ThrowError("Image file could not be opened.");
	}

	std::getline(fin, line);
	format = line;

	bool comments_left = true;
	while(comments_left) {
		std::getline(fin, line);
		if ('#' == line[0]) {
			comments.push_back(line);
		} else {
			tokens = SplitString(line, ' ');
			cols = std::stoi(tokens[0]);
			rows = std::stoi(tokens[1]);
			comments_left = false;
		}
	}


	ss << fin.rdbuf();
	int max_val;
	ss >> max_val;

	PGMImage img = PGMImage(rows + 2, cols + 2);
	img.format = format;
	img.grayscale = max_val;
	for(unsigned int i = 0; i < comments.size(); i++) {
		img.comments.push_back(comments[i]);
	}

	for (int i = 1; i <= rows; i++)
		for (int j = 1; j <= cols; j++)
			ss >> img.image[i][j];

	fin.close();
	return img;
}

void WriteImageToFile(std::string file_name, PGMImage img)
{
	std::ofstream fout(file_name.c_str());

	if (!fout.is_open())
	{
        ThrowError("Output image file could not be opened for writing.");
	}

	fout << img.format << "\n";
	for(unsigned int j = 0; j < img.comments.size(); j++) {
		fout << img.comments[j] << "\n";
	}
	fout << img.n_col << " " << img.n_row << "\n";
	fout << img.grayscale << "\n";

	for (int i = 0; i < img.n_row; i++) 
	{
		for (int j = 0; j < img.n_col; j++)
			fout << img.image[i][j] << "\n";
	}

	fout.close();
}