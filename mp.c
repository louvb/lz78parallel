/*2014
* louvalbuena@gmail.com
* LZ78 PARALLEL IMPLEMENTATION
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "mp.h"
#include <string.h>
#include <stdio.h>
//np=numero de procesadores, myId=id de la maquina
int np, myId;
//Inicializa el entorno distribuido y asigna los valores del numero de procesadores y id de la maquina
void MP_Init(int argc, char*argv[])
{
	MPI_Init (&argc, &argv);
	MPI_Comm_rank (MPI_COMM_WORLD, &myId);
	MPI_Comm_size (MPI_COMM_WORLD, &np);
}

void MP_Init_thread(int argc, char*argv[], int required)
{
	int provided;
	MPI_Init_thread (&argc, &argv, required, &provided);
	MPI_Comm_rank (MPI_COMM_WORLD, &myId);
	MPI_Comm_size (MPI_COMM_WORLD, &np);
	if(myId==0 && provided!=required)
	{
		printf("MPI Initialization Thread=");
		switch(provided)
		{
			case MPI_THREAD_SINGLE: printf("MPI_THREAD_SINGLE\n"); break;
			case MPI_THREAD_FUNNELED: printf("MPI_THREAD_FUNNELED\n"); break;
			case MPI_THREAD_SERIALIZED: printf("MPI_THREAD_SERIALIZED\n"); break;
			case MPI_THREAD_MULTIPLE: printf("MPI_THREAD_MULTIPLE\n"); break;
		}
	}
}

int MP_Barrier ()
{
	return MPI_Barrier(MPI_COMM_WORLD);
}
int MP_Bcast ( void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm )
{
	return MPI_Bcast(buffer, count, datatype, root, comm);
}
int sended=0;
int recived=0;
int MP_Send( void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm )
{
	return MPI_Send(buf, count, datatype, dest, tag, comm);
	//return EMPI_ErrorHandler(e);
}
int MP_Recv( void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm)
{
  MPI_Status status;
	return MPI_Recv(buf, count, datatype, source, tag, comm, &status);
}

