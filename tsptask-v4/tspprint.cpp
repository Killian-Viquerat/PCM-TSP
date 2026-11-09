#include <exception>
#include <iostream>
#include "tspgraph.hpp"

/*****************************************************************
  Example program to load and print a TSP graph
 *****************************************************************/

int main(int argc, char** argv) {
	if (argc < 2) {
		std::cerr << "Usage: " << argv[0] << " <file.tsp>\n";
		return 1;
	}
	try {
		TSPGraph g(argv[1]);
		std::cout << g << std::endl;
	} catch (const std::exception& e) {
		std::cerr << "error: " << e.what() << "\n";
		return 1;
	}
	return 0;
}
