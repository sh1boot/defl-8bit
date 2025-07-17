CXX=clang++
CXXFLAGS=-std=c++20 -pedantic -Wall -Werror -O3 -march=native

demo: main.o catfacts.o
	${CXX} ${CXXFLAGS} ${LDFLAGS} $^ -o $@

main.o: main.cc *.h

clean:
	rm -f *.o demo demodata.gz

run: demo
	./$< -l 100 -z > demodata.gz
	cat demodata.gz | ~/bin/infgen
	gunzip < demodata.gz | hexdump -C
