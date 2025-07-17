CXX=clang++
CXXFLAGS=-std=c++20 -pedantic -Wall -Werror -O3 -march=native -fconstexpr-steps=10000000

demo: main.o catfacts.o
	${CXX} ${CXXFLAGS} ${LDFLAGS} $^ -o $@

main.o: main.cc *.h

clean:
	rm -f *.o demo demodata.gz

run: demo
	./$< -l 600 -z > demodata.gz
	gunzip < demodata.gz | hexdump -C
	cat demodata.gz | ~/bin/infgen
