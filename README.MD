Basic Implementation of LZ78 Compression Alghorithm
Thanks to Giovanni Pellerano
giovanni.pellerano@evilaliv3.org

MOD  OF LZ78  TO PARALLEL IMPLEMENTATION USING MPI
by Luis Valbuena
louvalbuena@gmail.com



## Installation

make

## FOR (compression)

.mpiexec -np "NUM OF PROC" -hostfile "FILE.HOST" -i "FILE IN" -o "FILE OUT"

## FOR(decompress)

.mpiexec -np "NUM OF PROC" -hostfile "FILE.HOST" -i "FILE IN" -o "FILE OUT" -D
