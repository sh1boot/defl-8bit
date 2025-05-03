CXX=clang++
CXXFLAGS=-std=c++20 -pedantic -Wall -Werror -O3 -mcpu=native

demo: main.o catfacts.o huffman.o
	${CXX} ${CXXFLAGS} ${LDFLAGS} $^ -o $@

main.o: main.cc *.h

clean:
	rm -f *.o demo demodata.gz

run: demo
	./$< > demodata.gz
	cat demodata.gz | ~/bin/infgen
	gunzip < demodata.gz | hexdump -C
