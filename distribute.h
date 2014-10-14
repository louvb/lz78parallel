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
