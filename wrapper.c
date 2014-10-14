/*
* Basic implementation of LZ78 compression algorithm 
*
* Copyright (C) 2010 evilaliv3 <giovanni.pellerano@evilaliv3.org>
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

#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "wrapper.h"
#include "mp.h"
#include "distribute.h"
#include "time.h"
/* Structure representing the type of algorithm */
struct __algorithm {
    char* name;    /* String representing the name */
    uint8_t type;  /* Constant representing the type */
};

/* Opaque type representing the type of algorithm */
typedef struct __algorithm algorithm;

/* Struct of available algorithms */
const algorithm algo_list[] = {
    {"lz78", LZ78_ALGORITHM}, 
    {NULL,   UNKNOWN_ALGORITHM}
};

/* Struct representing the wrapper used for compression or decompression */
struct __wrapper {
    uint8_t type;      /* Algorithm used to compress or decompress data */
    uint8_t mode;      /* Flag indicating compress/decompress mode */
    void* data;        /* Opaque structure representing the algorithm */
};

/* Global variable representing the current error stored */
uint8_t wrapper_cur_err = WRAPPER_SUCCESS;

/* Associate an algorithm-dependent error to a wrapper-generic error */
uint8_t wrapper_return(uint8_t code) {
    wrapper_cur_err = code;
    switch (code) {
        case LZ78_SUCCESS:
            return WRAPPER_SUCCESS;
        case LZ78_ERROR_READ:
            return WRAPPER_ERROR_READ;
        case LZ78_ERROR_WRITE:
            return WRAPPER_ERROR_WRITE;
        case LZ78_ERROR_EAGAIN:
            return WRAPPER_ERROR_EAGAIN;
        case LZ78_ERROR_COMPRESS:
            return WRAPPER_ERROR_COMPRESS;
        case LZ78_ERROR_DECOMPRESS:
            return WRAPPER_ERROR_DECOMPRESS;
        case LZ78_ERROR_DICTIONARY:
        case LZ78_ERROR_INITIALIZATION:
        case LZ78_ERROR_MODE:
            return WRAPPER_ERROR_GENERIC;
    }
    return code;
}

uint8_t get_algorithm(char* type) {
    uint8_t i = 0;
    while (algo_list[i].name != NULL) {
        if (strcmp(type, algo_list[i].name) == 0)
            return algo_list[i].type;
        ++i;
    }
    return UNKNOWN_ALGORITHM;
}

int byte_size(char* size) {
    int n;

    if (size == NULL)
        return 0;

    n = atoi(size);

    switch (size[strlen(size) - 1]) {
        case 'K':
            n <<= 10;
            break;
    
        case 'M':
            n <<= 20;
            break;
    }

    return (n < 0) ? 0 : n;
}

void wrapper_perror() {
    switch (wrapper_cur_err) {
        case WRAPPER_SUCCESS:
            break;

        case WRAPPER_ERROR_ALGORITHM:
            fprintf(stderr, "Unrecognized compression algorithm\n");
            break;

        case WRAPPER_ERROR_FILE_IN:
            fprintf(stderr, "Unable to read input file\n");
            break;

        case WRAPPER_ERROR_FILE_OUT:
            fprintf(stderr, "Unable to write output file\n");
            break;

        case LZ78_SUCCESS:
            break;

        case LZ78_ERROR_DICTIONARY:
            fprintf(stderr, "LZ78: unable to allocate dictionaries\n");
            break;

        case LZ78_ERROR_INITIALIZATION:
            fprintf(stderr, "LZ78: bad initialization\n");
            break;

        case LZ78_ERROR_MODE:
            fprintf(stderr, "LZ78: wrong compression/decompression mode\n");
            break;

        case LZ78_ERROR_READ:
            fprintf(stderr, "LZ78: unable to read input data\n");
            break;

        case LZ78_ERROR_WRITE:
            fprintf(stderr, "LZ78: unable to write output data\n");
            break;

        case LZ78_ERROR_EAGAIN:
            fprintf(stderr, "LZ78: I/O operation would block: retry...\n");
            break;

        case LZ78_ERROR_COMPRESS:
            fprintf(stderr, "LZ78: unable to compress input data\n");
            break;

        case LZ78_ERROR_DECOMPRESS:      
            fprintf(stderr, "LZ78: unable to decompress input data\n");
            break;

        default:
            fprintf(stderr, "Unhandled error code %d\n", wrapper_cur_err);
    }
}

wrapper* wrapper_new(uint8_t w_mode, uint8_t w_type, char* argv) {
    wrapper* w = malloc(sizeof(wrapper));
    if (w == NULL)
        return NULL;

    w->type = w_type;
    w->mode = w_mode;

    switch (w->type) {
        case LZ78_ALGORITHM:
            w->data = lz78_new(w_mode, byte_size(argv));
            break;

        default:
            free(w);
            return NULL;
    }

    if (w->data)
        return w;
    else {
        free(w);
        return NULL;
    }
}

void wrapper_destroy(wrapper* w) {
    if (w == NULL)
        return;

    switch (w->type) {
        case LZ78_ALGORITHM:
            lz78_destroy(w->data);
            break;

        default:
            return;
    }
    free(w);
}
/*aqui comienza lo interesante para el paralelismo*/
uint8_t wrapper_compress(wrapper* w, char* input, char* output) {
    uint8_t ret=0;
    int fd_in;
    int fd_out;
    int fsize_in;
    int size_in=0;
    int *file_in=NULL;
    switch (w->type) {
        case LZ78_ALGORITHM:
	    if (input == NULL) {
		fd_in = STDIN_FILENO;
	    } else {
		fd_in = open(input, ACCESS_READ);
		if (fd_in == -1)
		    return wrapper_return(WRAPPER_ERROR_FILE_IN);
	    }
	    if (output == NULL) {
		fd_out = STDOUT_FILENO;
	    } else {
		fd_out = open(output, ACCESS_WRITE, 0644);
		if (fd_out == -1) {
		    close(fd_in);
		    return wrapper_return(WRAPPER_ERROR_FILE_OUT);
		}
	    }
	    Time("COMPRESS");
            debug("P%d INIT COMPRESS\n", myId);
	    file_in=read_file(fd_in, input, &fsize_in);
	    Time("FILEREAD");
	    debug("P%d READED %d\n", myId, fsize_in);
	    
	    size_in=fsize_in;
	    debug("P%d DISTRIBUTING %d\n", myId, size_in);
	    int *data_in=distribute(file_in, &size_in);
	    Time("DISTRIBUTE");
	    debug("P%d DISTRIBUTE %d\n", myId, size_in);

	    int *data_out=NULL;
	    int size_out=0;
	    if(size_in>0)
	    {
		debug("P%d ALLOC\n", myId);
		if(size_in<1000)
			data_out=(int*)malloc((1000)*sizeof(int));
		else{
			data_out=(int*)malloc((size_in*2.2)*sizeof(int));
			if(data_out==NULL)
			{
				printf("P%d ERROR ALLOCATING MEMORY\n", myId);
				return wrapper_return(WRAPPER_ERROR_FILE_OUT);
			}
		} 

		debug("P%d COMPRESSING\n", myId);
		ret = lz78_compress(w->data, data_in, size_in, data_out, &size_out);
		debug("P%d COMPRESSED \n", myId);
		if(data_in!=NULL)
			free(data_in);
		/*debug("P%d COMPRESSED ->%d\n [", myId, size_out);
		  for(j=0; j<size_out; j++)
		    debug("%d.", data_out[j]);
		  debug("]\n");*/
	    }
	    Time("PROCESS");
	    int *file_out=join_compress_data(data_out, &size_out, fsize_in);
	    if(data_out!=NULL)
		free(data_out);
	    Time("JOIN");
	    debug("P%d JOINED ->%d\n [", myId, size_out);

	    write_compressed_file(file_out, size_out, fd_out);
	    if(file_out!=NULL)
		free(file_out);
	    debug("P%d WRITED\n", myId);
	    Time("WRITE");
            return wrapper_return(ret);

        default:
            return wrapper_return(WRAPPER_ERROR_ALGORITHM);
    }
}
/*aqui continua lo interesante para el paralelismo en la parte de descompresion*/
uint8_t wrapper_decompress(wrapper* w, char* input, char* output) {
    uint8_t ret=0;
    int fd_in;
    int fd_out;
    int i;
    
    switch (w->type) {
        case LZ78_ALGORITHM:
            if (input == NULL) {
                fd_in = STDIN_FILENO;
            } else {
                fd_in = open(input, ACCESS_READ);
                if (fd_in == -1)
                    return wrapper_return(WRAPPER_ERROR_FILE_IN);
            }

            if (output == NULL) {
                fd_out = STDOUT_FILENO;
            } else {
                fd_out = open(output, ACCESS_WRITE, 0644);
                if (fd_out == -1) {
                    close(fd_in);
                    return wrapper_return(WRAPPER_ERROR_FILE_OUT);
                }
	    }
	    Time("DECOMPRESS");
	    int *fsize_in=NULL;
	    int *fsize_original;
	    
            int **file_in=read_compress_file(fd_in, &fsize_in, &fsize_original);
	    Time("FILEREAD");
	    if(ISMASTER)
	    debug("P%d READED %d\n", myId, fsize_in[0]);
	    else
	      debug("P%d READED\n", myId);
	    int *size_in;
	    int **data_in=distribute_compress_data(file_in, fsize_in, &fsize_original, &size_in);
	    debug("P%d DISTRIBUTE BLOQUES = %d\n", myId, *size_in);
	    Time("DISTRIBUTE");
	    if(ISMASTER)	//LIBERAR MEMORIA
	    {
		  for(i=0;i<*fsize_in; i++)
			free(file_in[i]);
	    }
	    
	    
	    int *size_out=(int*)malloc(sizeof(int)*size_in[0]);
	    int **data_out=(int**)malloc(sizeof(int*)*size_in[0]);
	    
	    for(i=0; i<size_in[0]; i++)
	    {
		  debug("P%d READY DECOMPRESS BLOQUE %d OK LENOUT=%d \n", myId, i, fsize_original[i]);
		  /*for(j=0; j<size_in[i+1]; j++)
		    debug("%d.", data_in[i][j]);
		  debug("]\n");*/
		  
		  data_out[i]=(int*)malloc(sizeof(int)*(fsize_original[i]+400));
		  if(data_out==NULL)
		  {
			  printf("P%d ERROR ALLOCATING MEMORY\n", myId);
			  return wrapper_return(WRAPPER_ERROR_FILE_OUT);
		  }
		  //if(i==0 && myId==4)
		    //if(i==1 && myId==1)
		    //debugf=1;
		  ret = lz78_decompress(w->data, data_in[i], size_in[1+i], data_out[i], &size_out[i]);
		  debug("P%d BLOQUE %d OK LEN=%d\n", myId, i, size_out[i]);

		  uint8_t* wmode=(uint8_t*)w->data;
		  w->data=lz78_new(*wmode, 0);
	    
		  free(data_in[i]);
	    }
	    Time("PROCESS");
	    free(fsize_original);
	    free(data_in);
	    debug("P%d DECOMPRESSED END\n", myId);

	    int fsize_out;
	    int *file_out=NULL;
	    if(ISMASTER)
		file_out=join(data_out, size_out, fsize_in[0], &fsize_out);
	    else
		join(data_out, size_out, size_in[0], &fsize_out);
	    Time("JOIN");
	    debug("P%d JOINED\n", myId);
	    free(size_in);
	    if(fsize_in!=NULL)
		free(fsize_in);
	    if(data_out!=NULL)
		free(data_out);
	    //debug("P%d FREE\n", myId);

	    write_file(file_out, fsize_out, fd_out);
	    Time("WRITE");
	    if(file_out!=NULL)
		free(file_out);
	    debug("P%d WRITED\n", myId);
            //close(fd_in);
            //close(fd_out);
            return wrapper_return(ret);

        default:
            return wrapper_return(WRAPPER_ERROR_ALGORITHM);
    }
}
/*funcion que se encarga de llamar a wrapper compress segun sea el caso*/
uint8_t wrapper_exec(wrapper* w, char* input, char* output) {
    uint8_t ret;
    if (w->mode == WRAPPER_MODE_COMPRESS) {
        for (;;) {
            ret = wrapper_compress(w, input, output);
            if (ret != WRAPPER_ERROR_EAGAIN)
                return ret;
        }
    } else {
        for (;;) {
            ret = wrapper_decompress(w, input, output);
            if (ret != WRAPPER_ERROR_EAGAIN)
                return ret;
        }
    }
}
