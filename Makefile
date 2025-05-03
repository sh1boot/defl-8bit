CXX=clang++
CXXFLAGS=-std=c++20 -pedantic -Wall -Werror -O3 -mcpu=native

defl8bit: main.o catfacts.o huffman.o
	${CXX} ${CXXFLAGS} ${LDFLAGS} $^ -o $@

main.o: main.cc *.h

clean:
	rm -f *.o defl8bit

run: defl8bit
	./$< > stuff.gz
	cat stuff.gz | ~/bin/infgen
	gunzip < stuff.gz | hexdump -C
