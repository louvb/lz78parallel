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

#include <malloc.h>
#include <string.h>
#include "distribute.h"
#include "mp.h"
#include "lz78.h"

int distribucionUniforme(int np, int length, int id)
{
	int l=length/np;
	if(id >= np - length%np)
		l++;
	return l;
}

int* distribute(int *data, int *size)
{
	int i, tlen, pos, sizeOriginal;
	debug2("P%d BCAST %d\n", myId, *size);
	MP_Bcast(size, 1, MPI_INT, 0, MPI_COMM_WORLD);
	sizeOriginal=*size;
	debug2("P%d BCASTED %d\n", myId, sizeOriginal);
	if(ISMASTER)
	{
		*size=distribucionUniforme(np, sizeOriginal, 0);
		pos=*size;
		for(i=1; i<np; i++)
		{
			tlen=distribucionUniforme(np, sizeOriginal, i);
			if(tlen>0)
			{
				MP_Send(data + pos, tlen, MPI_INT, i, 0, MPI_COMM_WORLD);
				pos+=tlen;
				debug2("P%d DATASENDED %d\n", myId, i);
			}
		}
		//QUITAR
		data[*size]=0;
	}else{
		tlen=distribucionUniforme(np, sizeOriginal, myId);
		//QUITAR
		data=(int*)malloc((tlen+1)*sizeof(int));
		if(tlen>0)
		{
			MP_Recv(data, tlen, MPI_INT, MASTER, 0, MPI_COMM_WORLD);
			debug2("P%d DATA RECEIVED\n", myId);
		}
		//QUITAR
		data[tlen]=0;
		*size=tlen;
	}
	return data;
}

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
//CONVERTIR FILE A STRING
int* read_file(int fd_in, char * input, int* size)
{
	if(ISMASTER)
	{
		struct stat st;
		stat(input, &st);
		*size = st.st_size;
		//QUITAR
		int* bytes = (int*)malloc((*size+1)*sizeof(int));
		if(bytes==NULL)
		{
		  printf("ERROR RESERVANDO MEMORIA\n");
		  return NULL;
		}
		
		FILE* in = fdopen(fd_in, "r");
		if (in == NULL)
		{
		    printf("ERROR ABRIENDO EL ARCHIVO\n");
		    return NULL;
		}
		int i;
		for(i=0; i<*size; i++)
			bytes[i] = fgetc(in);
		//QUITAR
		bytes[*size+1]=0;
		//fclose(in);
		return bytes;
	}
	return NULL;
}

int * join_compress_data(int *data, int *size, int originalSize)
{
	int i, pos;
	if(ISMASTER)
	{
		int *sizes=(int*)malloc(sizeof(int) * np);
		sizes[0]=*size;
		for(i=1; i<np; i++)
			MP_Recv(sizes+i, 1, MPI_INT, i, 0, MPI_COMM_WORLD);

		int tsize=0;
		for(i=0; i<np; i++)
			tsize+=sizes[i];
		tsize+=1+np*2;

		int *result=(int*)malloc(sizeof(int)*tsize);
		result[0]=np;	//dentro del archivo se guarda cuantos bloques comprimidos hay y el tamaño de cada uno para luego separarlos
		for(i=0; i<np; i++)
		{
			result[1+np+i]=distribucionUniforme(np, originalSize, i);
			result[1+i]=sizes[i];
		}

		pos=1+np*2;
		memcpy(result+pos, data, sizeof(int)*sizes[0]);

		pos+=sizes[0];
		for(i=1; i<np; i++)
		{
			if(sizes[i]>0)
				MP_Recv(result+pos, sizes[i], MPI_INT, i, 0, MPI_COMM_WORLD);
			pos+=sizes[i];
		}
		*size=tsize;
		free(sizes);
		return result;
	}else{
		MP_Send(size, 1, MPI_INT, MASTER, 0, MPI_COMM_WORLD);
		if(size>0)
			MP_Send(data, *size, MPI_INT, MASTER, 0, MPI_COMM_WORLD);
	}
	return NULL;
}

int** read_compress_file(int fd_in, int** size, int **originalSize)
{
	if(ISMASTER)
	{
		FILE* in = fdopen(fd_in, "r");
		if (in == NULL)
		    return NULL;

		int i, j, bloques;
		//Lee la cantidad de bloques comprimidos
		fread(&bloques, sizeof(int), 1, in);

		//Lee la longitud de cada bloque comprimido
		*size=(int*)malloc(sizeof(int)*(1+bloques));
		*originalSize=(int*)malloc(sizeof(int)*bloques);
		*size[0]=bloques;
		fread(*size+1, sizeof(int), bloques, in);
		fread(*originalSize, sizeof(int), bloques, in);

		//Lee los bloques comprimidos
		int** bytes = (int**)malloc(sizeof(int*)*bloques);
		for(i=0; i<bloques; i++)
		{
			//QUITAR
			bytes[i]=malloc(sizeof(int)*(size[0][1+i]+1));
			if(bytes[i]==NULL)
			{
				printf("ERROR RESERVANDO MEMORIA PARA LEER EL ARCHIVO\n");
				return NULL;
			}
			debug2("BLOQUE %d LEN=%d\n", i, size[0][1+i]);
			for(j=0; j<size[0][1+i]; j++)
			  bytes[i][j]=fgetc(in);
			//QUITAR
			bytes[i][size[0][1+i]]=0;
		}
		fclose(in);
		return bytes;
	}
	return NULL;
}

int **distribute_compress_data(int** file_in, int* fsize_in, int ** fsize_original, int** size_in)
{
	int i, j, tlen, pos, bloquesTotales, size;
	int **data=NULL;
	
	if(ISMASTER)
		bloquesTotales=fsize_in[0];
	
	MP_Bcast(&bloquesTotales, 1, MPI_INT, MASTER, MPI_COMM_WORLD);
	debug2("P%d BCASTED %d\n", myId, bloquesTotales);
	if(ISMASTER)
	{
		//distribuye los tamaños de cada cadena que le correspon a cada proceso y la cadena
		size=distribucionUniforme(np, bloquesTotales, 0);
		pos=size;
		for(i=1; i<np; i++)
		{
			tlen=distribucionUniforme(np, bloquesTotales, i);
			if(tlen>0)
			{
				MP_Send(fsize_in + 1 + pos, tlen, MPI_INT, i, 0, MPI_COMM_WORLD);
				MP_Send(fsize_original[0] + pos, tlen, MPI_INT, i, 0, MPI_COMM_WORLD);
				debug2("P%d SENDED SIZE NP=%d  %d -> %d {%d}\n", myId, i, fsize_in[1 + pos], fsize_original[0][pos], tlen);
				for(j=0; j<tlen; j++){
				  debug2("P%d SENDING NP=%d LEN=%d POS=%d\n", myId, i, fsize_in[1+pos+j], 1+pos+j);
					MP_Send(file_in[pos+j], fsize_in[1+pos+j], MPI_INT, i, 0, MPI_COMM_WORLD);
debug2("P%d SENDED DATA %d %d %d\n", myId, i, j, 1+pos+j);
				}
				pos+=tlen;
			}
		}
		//PARTE DEL MASTER
		size_in[0]=(int*)malloc((size+1)*sizeof(int));
		size_in[0][0]=size;
		for(i=0; i<size; i++)
			size_in[0][1+i]=fsize_in[1+i];

		if(size>0)
		{
			data=(int**)malloc(sizeof(int*)*size);
			for(i=0; i<size; i++)
			{
				//QUITAR
				data[i]=(int*)malloc(sizeof(int)*(size_in[0][1+i]+1));
				if(data[i]==NULL)
				{
					printf("P%d ERROR RESERVANDO MEMORIA\n",myId);
					return NULL;
				}
				memcpy(data[i], file_in[i], sizeof(int)*size_in[0][1+i]);
				//QUITAR
				data[i][size_in[0][1+i]]=0;
			}
		}
	}else{
		
		tlen=distribucionUniforme(np, bloquesTotales, myId);
		size_in[0]=(int*)malloc(sizeof(int)*(tlen+1));
		size_in[0][0]=tlen;
		fsize_original[0]=(int*)malloc(sizeof(int)*tlen);
		if(tlen>0)
		{
			data=(int**)malloc(sizeof(int*)*tlen);
			MP_Recv(size_in[0]+1, tlen, MPI_INT, MASTER, 0, MPI_COMM_WORLD);
			MP_Recv(fsize_original[0], tlen, MPI_INT, MASTER, 0, MPI_COMM_WORLD);
				debug2("P%d RECV %d %d\n", myId, size_in[0][1], fsize_original[0][0]);
			for(i=0; i<tlen; i++)
			{
				data[i]=(int*)malloc(sizeof(int)*size_in[0][1+i]);
				if(data[i]==NULL)
				{
					printf("P%d ERROR RESERVANDO MEMORIA PROCESO\n",myId);
					return NULL;
				}
				debug2("P%d RECEIVING BLOQUE %d\n", myId, i);
				MP_Recv(data[i], size_in[0][1+i], MPI_INT, MASTER, 0, MPI_COMM_WORLD);
				debug2("P%d DATARECEIVED BLOQUE %d\n", myId, i);
			}
		}
	}
	return data;
}

int* join(int **data_in, int *size_in, int bloquesTotales, int *size)
{
	int i, j, pos, bloques, posb;
	if(ISMASTER)
	{
		int *sizes=(int*)malloc(sizeof(int) * bloquesTotales);
		
		//Reunir los tamaños de bloques
		//BLOQUES MASTER
		bloques=distribucionUniforme(np, bloquesTotales, 0);
		for(i=0; i<bloques; i++)
			sizes[i]=size_in[i];

		pos=bloques;
		//BLOQUES SLAVE
		for(i=1; i<np; i++)
		{
			bloques=distribucionUniforme(np, bloquesTotales, i);
			if(bloques>0)
				MP_Recv(sizes+pos, bloques, MPI_INT, i, 0, MPI_COMM_WORLD);
			pos+=bloques;
		}
		//TAMAÑO TOTAL
		*size=0;
		for(i=0; i<bloquesTotales; i++)
			*size+=sizes[i];

		int *result=(int*)malloc(sizeof(int)*(*size));
		pos=0;
		//Reunir la data
		//BLOQUES MASTER
		bloques=distribucionUniforme(np, bloquesTotales, 0);
		for(i=0; i<bloques; i++)
		{
			memcpy(result+pos, data_in[i], sizeof(int)*sizes[i]);
			pos+=sizes[i];
		}
		
		posb=bloques;
		//BLOQUES SLAVE
		for(i=1; i<np; i++)
		{
			bloques=distribucionUniforme(np, bloquesTotales, i);
			
			for(j=0; j<bloques; j++)
			{
				if(sizes[posb+j]>0)
					MP_Recv(result+pos, sizes[posb+j], MPI_INT, i, 0, MPI_COMM_WORLD);
				pos+=sizes[posb+j];
			}
			posb+=bloques;
		}
		return result;
	}else{
		bloques=bloquesTotales;
		if(bloques>0)
			MP_Send(size_in, bloques, MPI_INT, MASTER, 0, MPI_COMM_WORLD);

		for(i=0; i<bloques; i++)
			if(size_in[i]>0)
				MP_Send(data_in[i], size_in[i], MPI_INT, MASTER, 0, MPI_COMM_WORLD);
	}
	return NULL;
}

int write_compressed_file2(int *data, int size, int fd_out)
{
	if(ISMASTER)
	{
		int i;
		//guarda la cantidad de bloques y el tamaño de cada uno
		write(fd_out, data, sizeof(int)*(1+data[0]*2));

		for(i=1+data[0]*2; i<size; i++)
			write(fd_out, data+i, 1);
		//close(fd_out);
	}
	return 0;
}

int write_compressed_file(int *data, int size, int fd_out)
{
	if(ISMASTER)
	{
		int i;
		//guarda la cantidad de bloques y el tamaño de cada uno
		int bloques=data[0];
		write(fd_out, data, sizeof(int)*(1+bloques*2));

		data=data+1+bloques*2;
		size-=1+bloques*2;
		unsigned char *d=(unsigned char *)malloc(size);
		for(i=0; i<size; i++)
		  d[i]=data[i];

		write(fd_out, d, size);
		//close(fd_out);
	}
	return 0;
}

int write_file(int *data, int size, int fd_out)
{
	if(ISMASTER)
	{
		FILE* out = fdopen(fd_out, "w");
		if (out == NULL)
			return -1;
		int i;
		unsigned char *d=(unsigned char *)malloc(size);
		for(i=0; i<size; i++)
			d[i]=data[i];

		write(fd_out, d, size);
		//close(fd_out);
	}
	return 0;
}

#include <stdarg.h>
void logging(FILE *f, char *formato, va_list pa)
{
	char *p;
	for (p = formato; *p; p++)
	{
		if (*p != '%')
		{
			putc(*p, f);
			continue;
		}

		switch (*++p)
		{
		case 'd':
			fprintf(f, "%d", va_arg(pa, int));
			break;
		case 's':
			fprintf(f, "%s", va_arg(pa, char *));
			break;
		case 'g':
			fprintf(f, "%lf", va_arg(pa, double));
			break;
		case 'c':
			fprintf(f, "%c", va_arg(pa, int));
			break;
		default:
			putc(*p, f);
			break;
		}
	}
}
int debugf=0;
void debug3(char *formato, ...)
{
	if (!debugf)return;
	va_list pa;
	va_start(pa, formato);
	logging(stdout, formato, pa);
	va_end(pa);
}
void debug2(char *formato, ...)
{
	return;
	va_list pa;
	va_start(pa, formato);
	logging(stdout, formato, pa);
	va_end(pa);
}

void debug(char *formato, ...)
{
	return;
	va_list pa;
	va_start(pa, formato);
	logging(stdout, formato, pa);
	va_end(pa);
}
