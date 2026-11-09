#include <algorithm>
#include <iostream>
#include "intvecsorttask.hpp"

/*****************************************************************
  Program to sort an aray of integers. It demonstrates the use
  of a Task implementation. IntVecSortTask extends task and
  implements split() solve() and merge(). IntVecSortTask is
  used with two runners: DirectTaskRunner directly calls
  solve(), and PartitionedTaskStackRunner calls split(),
  then recurse in all partitions, then call merge().
  Both results are checked (sorted, and equal to each other).
 *****************************************************************/

int main()
{
	IntVecSortTask iv1;
	iv1.randomize(10000);
	IntVecSortTask iv2 = iv1;

	DirectTaskRunner sr;
	sr.run(&iv1);
	std::cout << "direct:" << iv1 << " t:" << sr.duration() << std::endl;

	PartitionedTaskStackRunner rr(2);
	rr.run(&iv2);
	std::cout << "partit:" << iv2 << " t:" << rr.duration()
		<< " s:" << rr.solves() << "/" << rr.splits() << std::endl;

	if (!std::is_sorted(iv1.result().begin(), iv1.result().end()) ||
	    iv1.result() != iv2.result()) {
		std::cerr << "MISMATCH: results are not sorted or differ\n";
		return 2;
	}
	return 0;
}
