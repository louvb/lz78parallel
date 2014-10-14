Basic Implementation of LZ78 Compression Alghorithm
Thanks to Giovanni Pellerano
giovanni.pellerano@evilaliv3.org

MOD  OF LZ78  TO PARALLEL IMPLEMENTATION
by Luis Valbuena
louvalbuena@gmail.com



## Installation

make

## Typical usage (compression):

./lz78 -i inputfile -o outputfile

## Typical usage (decompression):

./lz78 -i inputfile -o outputfile -d

## Typical pipelined execution

echo "Hello World" | ./lz78 | ./lz78 -d
