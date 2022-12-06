#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>
#include <cstring>
#include <cstdlib>

#include "nassged.h"
#include "inves.h"
#include "datafile.h"
#include "dataset.h"
#include "nass.h"

using namespace std;

int main(int argc, char* argv[])
{
	if(argc < 3){
		cerr << "usage: nass-topk k data_file [index_file]" << endl;
		exit(1);
	}

	// parse the specified k
	nass::topk = 0;
	for(unsigned i = 0; argv[1][i] != '\0'; i++)
		if(argv[1][i] >= '0' && argv[1][i] <= '9')
			nass::topk = nass::topk*10 + (argv[1][i] - '0');
		else{
			cerr << "usage: nass k data_file [index_file]" << endl;
			exit(1);
		}

	DataFile* file = new SyntheticFile(argv[2]);
	DataSet* dataset = DataSet::getInstance();
	dataset->buildDataSet(*file);
	file->close();
	delete file;

	const char* indexfile = NULL;
	if(argc == 4) indexfile = argv[3];
	nass searcher(indexfile);

	Workload workload;
	workload.generateWorkload(searcher.nassIndex);

	searcher.run(workload);
	cout << endl << flush;

	// all graph (wrapper) objects should be destroyed before calling finishUp()
	DataSet::finishUp();

	return 0;
}
