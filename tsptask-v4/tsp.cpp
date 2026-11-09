#include <cstdlib>
#include <exception>
#include <iostream>
#include "tsptask.hpp"

/*****************************************************************
  Program to solve a TSP problem
  Arguments: tsp <filename> [number]
             filename: file to load the TSP graph from
             number: size of the graph (resized from the file)
  The program uses a TSPTask to solve the TSP problem, using
  two runners: DirectTaskRunner directly calls solve(), and
  PartitionedTaskStackRunner calls split(), then recurse in all
  partitions, then call merge().
  The best path is shared by all tasks, so it is reset between the
  two runs, and the two results are compared.
 *****************************************************************/

int main(int argc, char** argv) {
	if (argc < 2 || argc > 3) {
		std::cerr << "Usage: " << argv[0] << " <file.tsp> [number]\n";
		return 1;
	}

	try {
		TSPGraph graph(argv[1]);
		if (argc == 3) {
			char* end = nullptr;
			long n = std::strtol(argv[2], &end, 10);
			if (*argv[2] == '\0' || *end != '\0' || n < 1 || n > TSPPath::MAX_GRAPH) {
				std::cerr << "number must be an integer between 1 and "
					  << TSPPath::MAX_GRAPH << "\n";
				return 1;
			}
			graph.resize(static_cast<int>(n));
		}

		TSPPath::setup(&graph);

		TSPTask tsp2;
		DirectTaskRunner r2;
		r2.run(&tsp2);
		std::cout << "direct: " << tsp2.result() << " t:" << r2.duration() << std::endl;
		int direct_distance = tsp2.result().distance();

		TSPTask::reset_result(); // the partitioned run must not start with the best path

		TSPTask tsp1;
		tsp1.cutoff(0);
		PartitionedTaskStackRunner r1(TSPPath::MAX_GRAPH);
		r1.run(&tsp1);
		std::cout << "partit: " << tsp1.result() << " t:" << r1.duration()
			  << " s:" << r1.solves() << "/" << r1.splits() << std::endl;

		if (tsp1.result().distance() != direct_distance) {
			std::cerr << "MISMATCH: direct " << direct_distance
				  << " vs partitioned " << tsp1.result().distance() << "\n";
			return 2;
		}
	} catch (const std::exception& e) {
		std::cerr << "error: " << e.what() << "\n";
		return 1;
	}
	return 0;
}
