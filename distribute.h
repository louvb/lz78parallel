
#ifndef DISTRIBUTE
#define DISTRIBUTE

extern int debugf;
void debug3(char *formato, ...);
void debug2(char *formato, ...);
void debug(char *formato, ...);

int* read_file(int fd_in, char* input, int* size);
int* distribute(int *data, int *size);
int * join_compress_data(int *data, int *size, int originalSize);

int write_file(int *data, int size, int fd_out);
int write_compressed_file(int *data, int size, int fd_out);

int** read_compress_file(int fd_in, int** size, int **originalSize);
int **distribute_compress_data(int** file_in, int* fsize_in, int ** fsize_original, int** size_in);
int* join(int **data_in, int *size_in, int bloquesTotales, int *size);

#endif
