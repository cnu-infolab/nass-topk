#include <cassert>

#include "gedk.h"
#include "labelfreq.h"
#include "inves.h"

#include <unistd.h>

extern long long n_expands;

extern long long cands_final;
extern long long push_count;

GEDK::GEDK(coregraph* x, coregraph* y, int threshold)
{
	if(x->vsize() < y->vsize()){ coregraph* t = x; x = y; y = t; }
	full_mapping_size = (x->vsize() > y->vsize() ? x->vsize() : y->vsize());

	src_x = x; src_y = y;

    // level queue
	lqueue = new PriorityQueue*[src_y->vsize()+1];
	for(unsigned j = 0; j <= src_y->vsize(); j++) 
        lqueue[j] = new PriorityQueue();


	freqx = new LabelFreq(src_x->vsize());
	freqy = new LabelFreq(src_y->vsize());

	// initial ordering should obey the original ordering of coregraphs
	ordering = new vertex_t[src_y->vsize()];
	for(unsigned i = 0; i < src_y->vsize(); i++) ordering[i] = i;

	mapping* m = new mapping(full_mapping_size, 0, true);
	gx = new graph(*src_x, m->ordering);
	gy = new graph(*src_y, ordering);

    ubound = threshold + 1;
    int lb10 = compactBranchFilter();
    lbound = lb10/10 + (lb10 % 10 ? 1 : 0);
    if(lbound > (unsigned)threshold){ lbound = threshold + 1; return; }


	Inves inves(gx, gy);
	unsigned lb = inves.lowerBound(threshold);
    if(lb > lbound){
        lbound = lb;
        if(lbound > (unsigned)threshold){ lbound = threshold + 1; return; }
    }

	gy->reorder(*gx);
    lqueue[0]->push(m); 
	push_count++;
}

GEDK::~GEDK()
{
    clear();
}

void GEDK::handleDeletion(PriorityQueue* queue, mapping* m)
{
	for(unsigned i = 0; i < gx->vsize(); i++)
		m->mdist = m->mdist + gx->mappedErrors(m->size + i);
	m->size += gx->vsize();
	m->bdist = m->udist = 0;

	//?????????? if(m->distance() >= ??distance) delete m;
	if(m->distance() >= ubound) delete m;
	else{ queue->push(m); push_count++; }
}

unsigned GEDK::calculateDistance(mapping* m, mapping* pm)
{
	m->mdist = pm->mdist + gx->mappedErrors(pm->size, *gy);
	//??????????? if(m->distance() >= ??distance) return m->distance();
	if(m->distance() >= ubound) return m->distance();

	gx->countBridgesIncremental(false);
	gy->countBridgesIncremental(false);

	m->bdist = gx->bridgeErrors(*gy);

	gx->countBridgesIncremental(true);
	gy->countBridgesIncremental(true);

	// if(m->distance() >= ??distance) return m->distance();
	if(m->distance() >= ubound) return m->distance();

	gx->countLabelsIncremental(false);
	gy->countLabelsIncremental(false);

	m->udist = gx->unmappedErrors(*gy);

	gx->countLabelsIncremental(true);
	gy->countLabelsIncremental(true);

	if(m->distance() >= ubound) return m->distance();

    gx->countVertexEdgeFrequencies();
    gy->countVertexEdgeFrequencies();

    int lb10 = compactBranchFilter();
    m->udist = lb10/10 + (lb10 % 10 ? 1 : 0);

	return m->distance();
}

void GEDK::expandState(PriorityQueue* queue, mapping* pm)
{
	// for incremental counting
	gx->countLabelFrequencies();
	gy->countLabelFrequencies();
	gx->countBridgeFrequencies();
	gy->countBridgeFrequencies();

	for(int next = 0; next < int(gx->vsize()); next++){
		mapping* m = gx->generateNextState(next, pm);

		// set current mapping
		gx->setState(m, freqx);
		gy->setState(m->size, freqy);

		calculateDistance(m, pm);
		
		// ???????? if(m->distance() >= ??distance) delete m;
		if(m->distance() >= ubound) delete m;
		else{ queue->push(m); push_count++; }

		// restore parent mapping
		gx->setState(pm, freqx);
		gy->setState(pm->size, freqy);
	}

	delete pm;
}

/* multiple queue handling .. will be improved later */
PriorityQueue* GEDK::minQueue()
{
	int min = -1;
	unsigned min_dist = numeric_limits<unsigned>::max();

	for(int i = src_y->vsize(); i > 0; i--){
		if(lqueue[i]->empty()) continue;
		mapping* m = lqueue[i]->top();
		if(m->distance() < min_dist){
			min_dist = m->distance();
			min = i;
		}
	}

	// recycle the 0th queue to keep a complete mapping
	// so, a mapping in the 0th queue has the highest priority.
	if(!lqueue[0]->empty()){
		mapping* m = lqueue[0]->top();
		if(m->distance() <= min_dist){
			min_dist = m->distance();
			min = 0;
		}
	}

	if(min == -1) return NULL; 
	return lqueue[min];
}

void GEDK::clear(){
    if(lqueue){
	    for(unsigned i = 0; i <= src_y->vsize(); i++)
	    	lqueue[i]->clear();

	    for(unsigned j = 0; j <= src_y->vsize(); j++) delete lqueue[j];
	    delete [] lqueue;

        lqueue = NULL;
    }

    if(gx) delete gx;
    if(gy) delete gy;
    if(freqx) delete freqx;
    if(freqy) delete freqy;
	if(ordering) delete [] ordering;

    gx = gy = NULL;
    freqx = freqy = NULL;
    ordering = NULL;
}


#define LAST 0
#define GLOBAL -1
void GEDK::tightenBound(unsigned target_bound, int mode)
{
    if(lbound == ubound) return;

    int cl = GLOBAL;
    mapping* m = NULL;
	while(true){
        if(cl == GLOBAL || mode == LB_MODE){
            PriorityQueue* queue = minQueue();
			m = queue ? queue->top() : NULL;

    		if(m == NULL || m->distance() >= ubound){
                lbound = ubound; // in case that GED is found
				if(queue != NULL){ queue->pop(); delete m; }
                break;
	    	}

            if(mode == LB_MODE && m->distance() >= target_bound){
                lbound = m->distance();
                break;
            }

            queue->pop(); // remove the mapping from the queue
            cl = m->size;
        }
        else{
            m = lqueue[cl]->pop();
            if(m == NULL) { cl = GLOBAL; continue; }
        }

		if(complete(m)){
			lqueue[LAST]->clear();

        	if(ubound > m->distance()){
	    	    ubound = (int)m->distance();
                if(mode == LB_MODE) lbound = ubound;
            }

        	delete m;

            if(mode == LB_MODE) break;
            else if(ubound <= target_bound) break;
		}

		gx->setState(m, freqx);
		gy->setState(m->size, freqy);

 		// case: deletions of remaining vertices in gx
		if(gy->vsize() == 0){ handleDeletion(lqueue[LAST], m); cl = LAST; continue; }

		expandState(lqueue[cl+1], m);
		cl = cl+1;
    }

    if(lbound == ubound) clear();
}
