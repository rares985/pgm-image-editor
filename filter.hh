#ifndef FILTER_H_
#define FILTER_H_

#define INVALID_ID -1
#define ROOT_ID 0

#define COMMAND_BUFFER_SIZE 32

#include <vector>
#include <string>

extern int rank;
extern int n_procs;

extern std::vector<int> neighbours;

extern std::vector<int> children;

extern int parent;

extern char message[COMMAND_BUFFER_SIZE];

extern int *statistic;
extern int *buffer;

extern int *processed_buffer;

extern int image_width;
extern int buffer_size;

void MergeProbes(int *probe, int *response, int n_procs);
void ReadTopology(const char *topology_filename);
void ComputeTree();

void Leader(const char *topology_filename, const char *statistics_filename);
void Intermediary();
void Worker();

std::vector<std::pair<int, int> > GetChildBorders(int buffer_size);
void ProcessBuffer(int n, int m, int *array, std::string filter_name);

#endif /* FILTER_H_ */