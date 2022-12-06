# Top-k Graph Similarity Search

Sample data files and index files are included in the "data" directory. 
Please uncompress the data and index files before using them (e.g., gzip -d *.gz).

The included AIDS and PubChem indices have been constructed with thresholds 9 and 8, respectively.

The code has been complied and tested in Ubuntu and MacOS.

# How to build binaries (both nass-topk and nass-index)
> make

# How to run 
> nass-topk k data_file [index_file]

index_file is optional. 

For example, the following command runs nass-topk on the AIDS dataset for randomly selected 100 query graphs to find 5 graphs most similar to each query using the index AIDS.idx

> nass-topk 5 data/AIDS data/AIDS.idx

nass-topk randomly selects 100 queries from the dataset and performs top-k graph similarity searches.

nass-topk does not output or save result graphs in a file. The results are saved in nass::res_vec, which can be used to output or save the result graphs.

Please refer to https://github.com/JongikKim/Nass.git for the details of constructing an index for a dataset.

# Acknowledgement
This work was partially supported by Institute of Information & communications Technology Planning & Evaluation (IITP) grant funded by the Korea government(MSIT) (No.RS-2022-00155857, Artificial Intelligence Convergence Innovation Human Resources Development (Chungnam National University)).
