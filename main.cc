#include "filter.hh"
#include <iostream>
#include <mpi.h>

int rank;
int n_procs;
std::vector<int> neighbours;

int parent;
std::vector<int> children;

char message[COMMAND_BUFFER_SIZE];

int *statistic;
int *buffer;
int *processed_buffer;

int buffer_size;
int image_width;

MPI_Request request;

int main(int argc, char **argv) 
{
	if (argc != 4) {
		std::cerr << "[Argument error]. Usage: ./filtru topologie.in imagini.in statistica.out. Exiting...\n";
		exit(EXIT_FAILURE);
    }
    
    std::ios::sync_with_stdio(false);

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &n_procs);

    ReadTopology(argv[1]);
	
    ComputeTree();

    if (ROOT_ID == rank)
    {
        Leader(argv[2], argv[3]);
    }
    else {
        statistic = new int[n_procs]();
        if (children.size() != 0)
        {
            Intermediary();
        } else {
            Worker();
        }
    }
	
	MPI_Finalize();
	return 0;
}