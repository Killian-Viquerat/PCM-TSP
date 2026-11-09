#pragma once

#include <climits>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

#include "tspgraph.hpp"
#include "task.hpp"

// NOTE: this header uses C++17 inline variables.

/*****************************************************************
  TSPPath class, used in TSPTask
  Should call (static) TSPPath::setup() using a TSPGraph before
  creating TSPPath objects.
  Methods:
    maximise() sets the distance of path to maximum possible
    size() gets the number of nodes in path
    full() gets the size of a path with all nodes (== graph size)
    distance() gets the distance of the path
    contains(i) determines if path contains the node i
    tail() gets node in path tail
    push(node) adds node (and respective distance) to path tail
    pop() drops node ad path tail
 *****************************************************************/

class TSPSet {
public:
	TSPSet(): _bits(0) {}
	void reset() { _bits = 0; }
	void set(int node) { _bits |= (uint64_t(1) << node); }
	void reset(int node) { _bits &= ~(uint64_t(1) << node); }
	bool test(int node) const { return (_bits & (uint64_t(1) << node)) != 0; }
private:
	uint64_t _bits;
};

class TSPPath {
public:
	static const int FIRST_NODE = 0;
	static const int MAX_GRAPH = 32;
private:
	static inline TSPGraph* _graph = nullptr;
	// one extra slot: a full path gets FIRST_NODE pushed again at the end
	int _node[MAX_GRAPH + 1];
	int _size;
	int _distance;
	TSPSet _contents;
public:
	static void setup(TSPGraph *graph) {
		if (graph == nullptr)
			throw std::runtime_error("Null graph.");
		if (graph->size() > MAX_GRAPH)
			throw std::runtime_error("Graph bigger than MAX_GRAPH");
		_graph = graph;
	}
	static int full() { // the size of a full path
		if (_graph == nullptr)
			throw std::runtime_error("TSPPath::setup() not called.");
		return _graph->size();
	}

	TSPPath() {
		_node[0] = FIRST_NODE;
		_size = 1;
		_distance = 0;
		_contents.reset();
		_contents.set(FIRST_NODE);
	}

	void maximise() { _distance = INT_MAX; }
	int size() const { return _size; }
	int distance() const { return _distance; }
	bool contains(int i) const { return _contents.test(i); }
	int tail() const { return _node[_size-1]; }

	void push(int node) {
		if (node < 0 || node >= _graph->size())
			throw std::runtime_error("Node outside graph.");
		_distance += _graph->distance(tail(), node);
		_contents.set(node);
		_node[_size ++] = node;
	}

	void pop() {
		if (_size < 2)
			throw std::runtime_error("Empty path to pop().");
		_size --;
		int oldtail = _node[_size];
		int newtail = _node[_size-1];
		if (oldtail != FIRST_NODE)
			_contents.reset(oldtail);
		_distance -= _graph->distance(newtail, oldtail);
	}

	void write(std::ostream& os) const {
		os << "{" << _distance << ": ";
		for (int i=0; i<_size; i++) {
			if (i) os << ", ";
			os << _node[i];
		}
		os << "}";
	}
};

inline std::ostream& operator<<(std::ostream& os, const TSPPath& t) {
	t.write(os);
	return os;
}

class TSPTask;

// Reuses TSPTask objects instead of new/delete.
// Member functions are defined after TSPTask, which is incomplete here.
class TSPAllocator {
public:
	TSPAllocator() = default;
	TSPAllocator(const TSPAllocator&) = delete;
	TSPAllocator& operator=(const TSPAllocator&) = delete;
	~TSPAllocator(); // deletes anything left in the free list

	// this does not work with multiple threads!
	TSPTask* reusealloc(TSPTask* task, int node);
	// this does not work with multiple threads!
	void reusefree(TSPTask* task);
private:
	std::vector<TSPTask*> _free_list;
};

// single shared instance (one definition across all translation units)
inline TSPAllocator tspAllocator;


/*****************************************************************
  TSPTask class, extends Task
  Should call (static) TSPPath::setup() using a TSPGraph before
  creating TSPPath objects.
  Methods:
    reusealloc()/reusefree() replace new/delete (reuse tasks)
    cutoff(c) sets a cutoff size (from the end of a full path)
    result() gets the result after solve() or merge()
    reset_result() forgets the best path (shared by all tasks), call it
      before running the same problem again
 *****************************************************************/

class TSPTask : public Task {

private:
	static inline TSPPath _shortest = [] { TSPPath s; s.maximise(); return s; }();

	TSPPath _path;
	int _cutoff_size;
public:
	TSPTask() { _cutoff_size = TSPPath::full(); }
	~TSPTask() override = default;

	void reuse(TSPTask* task, int node) {
		_path = task->_path;
		_cutoff_size = task->_cutoff_size;
		_path.push(node);
	}

	// cutoff set, expressed as a distance from full path
	void cutoff(int c) { _cutoff_size = TSPPath::full() - c; }
	TSPPath& result() { return _shortest; }
	static void reset_result() { _shortest = TSPPath(); _shortest.maximise(); }

	// Task interface implementation: split, merge, solve, write

	int split(TaskCollection* collection) override {
		collection->clear();
		if (_path.size() >= _cutoff_size) return 0;
		int count = 0;
		for (int i=0; i<TSPPath::full(); i++) {
			if (!_path.contains(i)) {
				TSPTask* t = tspAllocator.reusealloc(this, i);
				if (t->_path.distance() >= _shortest.distance()) {
					tspAllocator.reusefree(t); // cannot beat the best path
					continue;
				}
				collection->push(t);
				count ++;
			}
		}
		return count;
	}

	// Results are kept in the shared _shortest, so merging only
	// has to recycle the sub-tasks.
	void merge(TaskCollection* collection) override {
		while (collection->size()) {
			TSPTask* t = (TSPTask*) collection->pop();
			tspAllocator.reusefree(t);
		}
	}

	void solve() override {
		if (_path.distance() >= _shortest.distance())
			return; // cannot beat the best path
		if (_path.size() == TSPPath::full()) {
			_path.push(TSPPath::FIRST_NODE); // last node = first node
			if (_path.distance() < _shortest.distance())
				_shortest = _path;
			_path.pop();
		} else {
			for (int i=0; i<TSPPath::full(); i++) {
				if (!_path.contains(i)) {
					_path.push(i);
					if (_path.distance() < _shortest.distance())
						solve();
					_path.pop();
				}
			}
		}
	}

	void write(std::ostream& os) const override {
		os << "Task(c=" << _cutoff_size << ')' << _path;
	}
};

inline TSPAllocator::~TSPAllocator() {
	for (TSPTask* t : _free_list)
		delete t;
}

inline TSPTask* TSPAllocator::reusealloc(TSPTask* task, int node) {
	TSPTask* p = nullptr;
	if (_free_list.empty()) {
		p = new TSPTask();
	} else {
		p = _free_list.back();
		_free_list.pop_back();
	}
	p->reuse(task, node);
	return p;
}

inline void TSPAllocator::reusefree(TSPTask* task) {
	_free_list.push_back(task);
}
