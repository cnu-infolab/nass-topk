#include <sstream>

#include "nass.h"
#include "nassged.h"
#include "gedk.h"

//statistics
long long cands_final;
long long push_count;

int nass::threshold = THRESHOLD_MAX;
unsigned nass::topk = 0;

//macro for distance handling
#define MASKH 0x00FF
#define MASKL 0xFF00

#define SETMIND(dist, val) ((dist & MASKL) | val)
#define SETMAXD(dist, val) ((dist & MASKH) | (val << 16))
#define MINDIST(dist) (dist & MASKH)
#define MAXDIST(dist) (dist >> 16)

nass::nass(const char* filename, bool load_index)
{
	this->indexfile = filename;

	this->use_index = false;
	this->nassIndex = NULL;
	this->sampled = NULL;
	this->index_max_threshold = 0;

	if(load_index && indexfile && loadNassIndex())
		use_index = true;

	clearStats();
}

nass::~nass()
{
	delete [] nassIndex;
	delete [] sampled;
}


void nass::clearStats()
{
	cands_final = 0;
	push_count = 0;
	res_vec.clear();
}

void nass::run(Workload& workload)
{
	clearStats();

	timer = new loop_timer("Nass", "", workload.size(), NULL, 1); 
	timer->start();

    unsigned total_results = 0;

	for(unsigned i = 0; i < workload.size(); i++){
        res_vec.clear();
        threshold = THRESHOLD_MAX;
		search(workload[i]);
        total_results += res_vec.size();
		timer->next();
	}

	timer->stop();
	long long elapsed = timer->end(true);
	cout << "===================== Results =====================" << endl;
	cout << "# queries: " << workload.size() << endl;
	cout << "# mappings: " << push_count << endl;
	cout << "# total results: " << total_results << endl;
	cout << "Processing time: ";
    fprintf(stdout, "%lld.%03lld seconds\n", elapsed/1000000, (elapsed % 1000000)/1000); 
	cout << "===================================================" << endl;

	delete timer;
}

bool gtid(const pair<int, int>& c1, const pair<int, int>& c2)
{
	return c1.first < c2.first;
}

bool gtlb(const pair<int, int>& c1, const pair<int, int>& c2)
{
	return c1.second < c2.second;
}

void nass::pushTopK(int res, int distance)
{
    if(res_vec.size() < topk){
        res_vec.push_back(pair<int, int>(distance, res));
        if(res_vec.size() == topk)
            make_heap(res_vec.begin(), res_vec.end());
        return;
    }

    // if res_vec keeps k elements
    if(res_vec.front().first <= distance) return;

    pop_heap(res_vec.begin(), res_vec.end());
    res_vec.pop_back();
    res_vec.push_back(pair<int, int>(distance, res));
    push_heap(res_vec.begin(), res_vec.end());

    threshold = distance;

}

/******** CHECK COMMENTS INSIDE THE FUNCTION !!!!!!!! ????? *********/
unsigned nass::regenCandidates(vector<pair<int, int> >& candidates, unsigned pos, int res, int distance)
{
	int max_threshold = index_max_threshold;
	if(!sampled[res]) max_threshold -= 1;

    pushTopK(res, distance);
	if(distance + threshold > max_threshold) return pos;

	std::sort(candidates.begin() + pos, candidates.end(), gtid);

	vector<pair<int,int> > tmp;
	unsigned ptr1 = pos, ptr2 = 0;

	while(ptr1 < candidates.size() && ptr2 < nassIndex[res].size()){
		int gdist = nassIndex[res][ptr2].second;
		bool inexact = (gdist < 0);
		if(gdist == INEXACT_ZERO) gdist = 0;
		if(inexact) gdist = gdist * -1;

		if(gdist > threshold + distance) ptr2++;
		else if(nassIndex[res][ptr2].first < candidates[ptr1].first) ptr2++; 
		else if(nassIndex[res][ptr2].first > candidates[ptr1].first) ptr1++;
		else{
		//	if(gdist + distance <= threshold && !inexact)
        //        pushTopK(nassIndex[res][ptr2].first, gdist + distance);
		//    else
                tmp.push_back(candidates[ptr1]);

			ptr1++; ptr2++;
		}
	}
	std::swap(candidates, tmp);

	std::sort(candidates.begin(), candidates.end(), gtlb);

	return 0;
}

void nass::basicFilter(coregraph* x, vector<pair<int, int> >& candidates, int threshold, int begin)
{
	DataSet* dataset = DataSet::getInstance();

	for(unsigned i = begin; i < dataset->numGraphs(); i++){
		coregraph* y = dataset->graphAt(i);
		if(x == y) continue; // skip the same graph
		if(x->sizeFilter(y) > unsigned(threshold)) continue;

		int lb = x->labelFilter(y); //gx.unmappedErrors(gy);
		if(lb <= threshold)
			candidates.push_back(pair<int, int>(i, lb));
	}
}

#include <unistd.h>
void nass::search(coregraph* x)
{
	DataSet* dataset = DataSet::getInstance();

	vector<pair<int, int> > candidates;
	basicFilter(x, candidates, threshold);

	if(use_index) std::sort(candidates.begin(), candidates.end(), gtlb);

	unsigned i = 0;
	while(i < candidates.size()){
		coregraph* y = dataset->graphAt(candidates[i].first);

        GEDK* gedk = new GEDK(x, y, threshold);
        while(gedk->lbound != gedk->ubound){
            gedk->tightenBound(gedk->lbound + 1, LB_MODE);
            //gedk->tightenBound(gedk->ubound - 1, UB_MODE);
        }

        int distance = gedk->lbound;
		delete gedk;

		if(distance <= threshold){
			if(use_index) i = regenCandidates(candidates, i+1, candidates[i].first, distance);
			else pushTopK(candidates[i++].first, distance); // never called!!
		}
		else i++;
	}
}
