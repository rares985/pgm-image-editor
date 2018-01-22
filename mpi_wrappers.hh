#ifndef MPI_WRAPPERS_H_
#define MPI_WRAPPERS_H_

#include <vector>
#include <mpi.h>

extern MPI_Request request;

void Send(void *buffer, int bufsize, MPI_Datatype element_datatype, int destination);
void Send(void *buffer, int bufsize, MPI_Datatype element_datatype, int destination, int tag);

void Recv(void *buffer, int bufsize, MPI_Datatype element_datatype, int sender);
void Recv(void *buffer, int bufsize, MPI_Datatype element_datatype, int sender, int tag);

void SendAll(void *buffer, int bufsize, MPI_Datatype element_datatype, std::vector<int> &destinations);
void SendAll(void *buffer, int bufsize, MPI_Datatype element_datatype, std::vector<int> &destinations, int tag);

#endif /* MPI_WRAPPERS_H_ */