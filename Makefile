CXX=clang++
CXXFLAGS=-std=c++20 -pedantic -Wall -Werror -O1 -mcpu=native

defl8bit: defl8bit.cc
	${CXX} ${CXXFLAGS} ${LDFLAGS} $< -o $@

run: defl8bit
	./$^ > stuff.gz
	cat stuff.gz | ~/bin/infgen -di
	gunzip < stuff.gz | hexdump -C
