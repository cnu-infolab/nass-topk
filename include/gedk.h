#ifndef __GED_K_H
#define __GED_K_H

#include "graph.h"
#include "mapping.h"
//#include "syncqueue.h"
#include "queue.h"
#include "labelfreq.h"

#include <cstring>

#define LB_MODE 0
#define UB_MODE 1

class GEDK
{
public:
    unsigned lbound, ubound;

private:
	PriorityQueue* gqueue;
	PriorityQueue** lqueue;

private:
	PriorityQueue* minQueue();
	void clear();

private:
	coregraph* src_x;
	coregraph* src_y;

    graph* gx;
    graph* gy;

	unsigned full_mapping_size; // size of a full mapping

private:
	vertex_t* ordering;
    LabelFreq* freqx;
    LabelFreq* freqy;

private:
	unsigned calculateDistance(mapping* m, mapping* pm);
	void expandState(PriorityQueue* queue, mapping* pm);
	bool complete(mapping* m) { return m->size == full_mapping_size; }

	void handleDeletion(PriorityQueue* queue, mapping* m);

private:
	int compactBranchFilter(bool debug = false);

public:
	GEDK(coregraph* x, coregraph* y, int threshold);
	~GEDK();

public:
	void tightenBound(unsigned bound, int mode);
};

#endif// __GED_K_H
