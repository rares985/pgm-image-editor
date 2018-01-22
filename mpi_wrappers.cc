#include "mpi_wrappers.hh"

#include <mpi.h>


void Send(void *buffer, int bufsize, MPI_Datatype element_datatype, int destination)
{
    Send(buffer, bufsize, element_datatype, destination, 0);
}

void Send(void *buffer, int bufsize, MPI_Datatype element_datatype, int destination, int tag)
{
    MPI_Isend(buffer, bufsize, element_datatype, destination, tag, MPI_COMM_WORLD, &request);
    MPI_Wait(&request, MPI_STATUS_IGNORE);
}


void Recv(void *buffer, int bufsize, MPI_Datatype element_datatype, int sender)
{
	Recv(buffer, bufsize, element_datatype, sender, 0);
}

void Recv(void *buffer, int bufsize, MPI_Datatype element_datatype, int sender, int tag)
{
    MPI_Irecv(buffer, bufsize, element_datatype, sender, tag, MPI_COMM_WORLD, &request);
    MPI_Wait(&request, MPI_STATUS_IGNORE);
}


void SendAll(void *buffer, int bufsize, MPI_Datatype element_datatype, std::vector<int> &destinations) {
	SendAll(buffer, bufsize, element_datatype, destinations, 0);
}

void SendAll(void *buffer, int bufsize, MPI_Datatype element_datatype, std::vector<int> &destinations, int tag)
{
    for (unsigned int i = 0; i < destinations.size(); i++)
    {
        Send(buffer, bufsize, element_datatype, destinations[i], tag);
    }
}