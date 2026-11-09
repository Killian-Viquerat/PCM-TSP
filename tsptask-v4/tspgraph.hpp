#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cmath>
#include <climits>
#include <stdexcept>
#include <iomanip>

/*****************************************************************
  TSPGraph class
  usage:
         TSPGraph g(filename);
         g.resize(atoi(argv[2]));
         TSPPath::setup(&g);
         int s = g.size();
         int d = g.distance(pointa, pointb);

  Reads TSPLIB files with EDGE_WEIGHT_TYPE EUC_2D.
  resize() can only shrink the graph (keeps the first N points).
 *****************************************************************/

class TSPGraph {
private:
	struct Point { double x, y; };
	std::vector<Point> _coords;
	std::vector<std::vector<int>> _dist;
	int _width;
	std::string _filename;

	static std::string trim(const std::string& s) {
		const char* ws = " \t\r\n";
		size_t b = s.find_first_not_of(ws);
		if (b == std::string::npos) return "";
		size_t e = s.find_last_not_of(ws);
		return s.substr(b, e - b + 1);
	}

public:
	int size() const { return static_cast<int>(_coords.size()); }
	int distance(int a, int b) const { return _dist[a][b]; }

	// Shrinks the graph to its first 'size' points. The distance matrix
	// is indexed by original node numbers, so it stays valid.
	void resize(int size) {
		if (size < 1 || size > this->size())
			throw std::runtime_error("resize(): size must be between 1 and the current graph size");
		_coords.resize(size);
	}

	TSPGraph(const std::string& filename) {
		std::ifstream in(filename);
		if (!in)
			throw std::runtime_error("Cannot open file: " + filename);
		_filename = filename;
		std::string line;
		int dimension = -1;
		bool inCoordSection = false;
		while (std::getline(in, line)) {
			line = trim(line);
			if (line.empty()) continue;
			if (line == "NODE_COORD_SECTION") {
				inCoordSection = true;
				break;
			}
			size_t colon = line.find(':');
			if (colon == std::string::npos) continue;
			std::string key = trim(line.substr(0, colon));
			std::string value = trim(line.substr(colon + 1));
			if (key == "DIMENSION") {
				std::stringstream ss(value);
				if (!(ss >> dimension))
					throw std::runtime_error("Invalid DIMENSION");
			} else if (key == "EDGE_WEIGHT_TYPE") {
				if (value != "EUC_2D")
					throw std::runtime_error("Unsupported EDGE_WEIGHT_TYPE: " + value);
			}
		}
		if (dimension <= 0)
			throw std::runtime_error("Invalid or missing DIMENSION");
		if (!inCoordSection)
			throw std::runtime_error("Missing NODE_COORD_SECTION");
		_coords.assign(dimension, {0,0});
		std::vector<bool> seen(dimension, false);
		int count = 0;
		while (std::getline(in, line)) {
			line = trim(line);
			if (line == "EOF") break;
			if (line.empty()) continue;
			std::stringstream ss(line);
			int index;
			double x, y;
			if (!(ss >> index >> x >> y)) continue;
			if (index < 1 || index > dimension)
				throw std::runtime_error("Invalid city index");
			if (seen[index - 1])
				throw std::runtime_error("Duplicate city index");
			seen[index - 1] = true;
			_coords[index - 1] = {x, y};
			count ++;
		}
		if (count != dimension)
			throw std::runtime_error("Coordinate count mismatch");
		_dist.assign(dimension, std::vector<int>(dimension, 0));
		int max = 0;
		for (int i = 0; i < dimension; ++i) {
			for (int j = i + 1; j < dimension; ++j) {
				int d = euc2d(_coords[i], _coords[j]);
				_dist[i][j] = _dist[j][i] = d;
				if (d > max) max = d;
			}
		}
		int digits = 1;
		while (max >= 10) { max /= 10; digits++; }
		_width = digits + 1; // used only for printing
	}

	void write(std::ostream& os) const {
		os << "TSP graph from file " << _filename << '\n';
		int n = size();
		for (int i=0; i<n; i++)
			os << " point " << i << " { x: " << _coords[i].x << ", y: " << _coords[i].y << "}\n";
		os << "   "; // same width as the row labels below
		for (int j = n-1; j > 0; --j)
			os << std::setw(_width) << j;
		os << '\n';
		for (int i = 0; i < (n-1); i++) {
			os << std::setw(3) << i;
			for (int j = (n-1); j > i; j--)
				os << std::setw(_width) << _dist[i][j];
			os << '\n';
		}
	}

private:
	static int euc2d(const Point& a, const Point& b) {
		double dx = a.x - b.x;
		double dy = a.y - b.y;
		double d = std::round(std::sqrt(dx*dx + dy*dy));
		if (!(d <= INT_MAX)) // also catches NaN
			throw std::runtime_error("Distance does not fit in an int");
		return static_cast<int>(d);
	}
};

inline std::ostream& operator<<(std::ostream& os, const TSPGraph& t)
{
	t.write(os);
	return os;
}
