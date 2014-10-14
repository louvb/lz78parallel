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

#ifndef MP_H
#define MP_H

#define MASTER 0
#define ISMASTER	myId==0
#define ISSLAVE		myId!=0

#include <stdio.h>
#include <mpi.h>

extern int np, myId;		//np=numero de procesadores, myId=id de la maquina


//Inicializa el entorno distribuido y asigna los valores del numero de procesadores y id de la maquina
void MP_Init(int argc, char*argv[]);
void MP_Init_thread(int argc, char*argv[], int required);
#define MP_Finalize	MPI_Finalize


	int MP_Send( void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm );
	int MP_Recv( void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm);
	int MP_Bcast ( void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm );
	int MP_Barrier ( );

#endif
