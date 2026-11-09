#pragma once

#include <chrono>
#include <cstddef>
#include <deque>
#include <iostream>
#include <stdexcept>
#include <vector>

/*****************************************************************
  Task class
  Methods:
    split(c) splits a task into subtasks, all added to c
    merge(c) merge results from all subtasks in c
    solve() solves the task without splitting
 *****************************************************************/

class TaskCollection;

class Task {
public:
	virtual int split(TaskCollection* collection) = 0;
	virtual void merge(TaskCollection* collection) = 0;
	virtual void solve() = 0;
	virtual void write(std::ostream& os) const = 0;
	virtual ~Task() = default;
};

inline std::ostream& operator<<(std::ostream& os, const Task& t) {
	t.write(os);
	return os;
}

/*****************************************************************
  TaskCollection classes
  Methods:
    [i] gets ith element in collection
    push(e) pushes element e at the end of collection
    pop() removes last element added
    clear() removes all elements
    size() gets number of elements in collection
 *****************************************************************/

class TaskCollection {
public:
	virtual int size() const = 0;
	virtual Task* operator[](int i) = 0;
	virtual void push(Task* t) = 0;
	virtual Task* pop() = 0;
	virtual void clear() = 0;
	virtual ~TaskCollection() = default;
};

class TaskStack : public TaskCollection {
private:
	std::vector<Task*> _tab;
public:
	TaskStack(int cap) { _tab.reserve(cap); }
	int size() const override { return static_cast<int>(_tab.size()); }
	Task* operator[](int i) override { return _tab[i]; }
	void push(Task* t) override {
		_tab.push_back(t);
	}
	Task* pop() override {
		if (_tab.empty())
			throw std::runtime_error("TaskStack empty!");
		Task* ret = _tab.back();
		_tab.pop_back();
		return ret;
	}
	void clear() override { _tab.clear(); }
};

class FixedTaskStack : public TaskCollection {
private:
	Task** _tab;
	int _size;
	int _capacity;
public:
	FixedTaskStack(Task** tab, int cap) : _tab(tab), _size(0), _capacity(cap) {}
	int size() const override { return _size; }
	Task* operator[](int i) override { return _tab[i]; }
	void push(Task* t) override {
		if (_size >= _capacity)
			throw std::runtime_error("FixedTaskStack full!");
		_tab[_size ++] = t;
	}
	Task* pop() override {
		if (_size <= 0)
			throw std::runtime_error("FixedTaskStack empty!");
		return _tab[-- _size];
	}
	void clear() override { _size = 0; }
};

/*****************************************************************
  TaskRunner classes
  Methods:
    startTimer() starts mesuring time
    stopTimer() stops measuring time
    duration() gets time between startTimer() and stopTimer()
    run(t) executes task t, must call startTimer() and stopTimer()
 *****************************************************************/

class TaskRunner {
private:
	std::chrono::time_point<std::chrono::high_resolution_clock> _start, _stop;
public:
	virtual void run(Task* t) = 0;
	virtual ~TaskRunner() = default;
	double duration() const {
		std::chrono::duration<double> diff = _stop - _start;
		return diff.count();   // seconds as a double
	}
protected:
	void startTimer() { _start = std::chrono::high_resolution_clock::now(); }
	void stopTimer() { _stop = std::chrono::high_resolution_clock::now(); }
};

class DirectTaskRunner : public TaskRunner {
public:
	virtual void run(Task* t) override {
		TaskRunner::startTimer();
		t->solve();
		TaskRunner::stopTimer();
	}
};

class PartitionedTaskStackRunner : public TaskRunner {
private:
	int _size;
	int _splits;
	int _solves;
	// One reusable collection per recursion depth, so recurse() does not
	// allocate a new collection for every task. A deque keeps references
	// to existing elements valid when it grows.
	std::deque<TaskStack> _stacks;

	void recurse(Task* t, std::size_t depth) {
		if (depth == _stacks.size())
			_stacks.emplace_back(_size);
		TaskStack& coll = _stacks[depth];
		coll.clear();
		int n = t->split(&coll);
		if (n) {
			_splits ++;
			for (int i=0; i<n; i++)
				recurse(coll[i], depth + 1);
			t->merge(&coll);
		} else {
			_solves ++;
			t->solve();
		}
	}
	PartitionedTaskStackRunner() = delete; // needs a collection size
public:
	PartitionedTaskStackRunner(int size) : _size(size), _splits(0), _solves(0) {}
	virtual void run(Task* t) override {
		_splits = _solves = 0;
		TaskRunner::startTimer();
		recurse(t, 0);
		TaskRunner::stopTimer();
	}
	int solves() const { return _solves; }
	int splits() const { return _splits; }
};
