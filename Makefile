CXX=clang++
CXXFLAGS=-std=c++20 -pedantic -Wall -Werror -O3 -mcpu=native

defl8bit: main.o huffman.o
	${CXX} ${CXXFLAGS} ${LDFLAGS} $^ -o $@

clean:
	rm -f *.o defl8bit

run: defl8bit
	./$< > stuff.gz
	cat stuff.gz | ~/bin/infgen
	gunzip < stuff.gz | hexdump -C
