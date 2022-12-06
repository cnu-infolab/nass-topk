#ifndef __LABEL_FREQ__
#define __LABEL_FREQ__

//extern map<string, int> vlabel_map;
//extern map<string, int> elabel_map;

struct LabelFreq
{
    unsigned* vfreq;
    unsigned* efreq;

    unsigned** vertex_bridges;
    unsigned** vertex_efreq; // not used yet!!

	unsigned vsize;

    LabelFreq(unsigned sz = 0) : vsize(sz)
    {
        if(vsize == 0){
            DataSet* dataset = DataSet::getInstance();
            vsize = dataset->vmax();
        }
            
        vfreq = new unsigned[vlabel_map.size()];
        efreq = new unsigned[elabel_map.size()];

        vertex_bridges = new unsigned*[vsize];
        for(unsigned i = 0; i < vsize; i++){
            vertex_bridges[i] = new unsigned[elabel_map.size()];
            memset(vertex_bridges[i], 0, sizeof(unsigned)*elabel_map.size());
        }

        vertex_efreq = new unsigned*[vsize];
        for(unsigned i = 0; i < vsize; i++){
            vertex_efreq[i] = new unsigned[elabel_map.size() + 1];
            memset(vertex_efreq[i], 0, sizeof(unsigned)*(elabel_map.size()+1));
        }
    }

    ~LabelFreq()
    {
        delete [] vfreq;
        delete [] efreq;

        for(unsigned i = 0; i < vsize; i++)
            delete [] vertex_bridges[i];
        delete [] vertex_bridges;

        for(unsigned i = 0; i < vsize; i++)
            delete [] vertex_efreq[i];
        delete [] vertex_efreq;
    }
};

#endif //__LABEL_FREQ__

