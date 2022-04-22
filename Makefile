#NAME: Danna Lin
#EMAIL: dannalin@g.ucla.edu
#ID: 005121389

CXX = g++
CXXFLAGS = -g -Wall -Wextra -static-libstdc++

default: memory_driver

memory_driver: memory_driver.cpp mem_ctrlr.cpp
	$(CXX) $(CXXFLAGS) $^ -o $@

tarball_files = Makefile memory_driver.cpp mem_ctrlr.cpp mem_ctrlr.hpp
dist:
	tar -czf ca_2_base.tar.gz $(tarball_files)

clean:
	rm -f *.o memory_driver  ca_2_base.tar.gz
