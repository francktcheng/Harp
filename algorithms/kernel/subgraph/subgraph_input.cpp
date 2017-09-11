/* file: subgraph_input.cpp */
/*******************************************************************************
* Copyright 2014-2016 Intel Corporation
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*******************************************************************************/

/*
//++
//  Implementation of subgraph classes.
//  non-template func
//--
*/

#include "algorithms/subgraph/subgraph_types.h"
#include "services/env_detect.h"
#include "service_micro_table.h"
#include "services/thpool.h"
#include <cstdlib> 
#include <cstdio> 
#include <cstring> 
#include <sstream>
#include <iostream>
#include <omp.h>
#include <vector>
#include <string>
#include <algorithm>
#include <utility>
#include <time.h>
#include <ctime>        
#include <pthread.h>
#include <unistd.h>
#include "service_rng.h"

using namespace daal::data_management;
using namespace daal::services;

namespace daal
{
namespace algorithms
{
namespace subgraph
{
namespace interface1
{

//aux func used in input
void util_quicksort(int*& arr, const int left, const int right, const int sz);
int util_partition(int*& arr, const int left, const int right);
int* util_dynamic_to_static(std::vector<int>& arr);
int util_get_max(std::vector<int> arr1, std::vector<int> arr2);
int util_choose(int n, int k);
int** util_init_choose_table(int num_colors);
int* util_init_permutation(int num_verts);
void util_next_set(int*& current_set, int length, int num_colors);
int util_get_color_index(int*& colorset, int length);
int util_factorial(int x);
int util_test_automorphism(Graph& tp, std::vector<int>& mapping);
int util_count_all_automorphisms(Graph& tp, std::vector<int>& mapping, std::vector<int>& rest);
int util_count_automorphisms(Graph& tp);

// input has two numerictables
// 0: filenames
// 1: fileoffsets
Input::Input() : daal::algorithms::Input(7) {


    // store vert of each template
    num_verts_table = NULL;
    subtemplate_count = 0;
    subtemplates = NULL;

    // record comb num values for each subtemplate and each color combination
    comb_num_indexes_set = NULL;
     //stores the combination of color sets
    choose_table = NULL;

    thread_num = 0;
    v_adj = NULL; //store data from reading data

    num_colors = 0; 
    colors_g = NULL;
    
    // for template data
    t_ng = 0;
    t_mg = 0;

    fs = NULL;
    fileOffsetPtr = NULL;
    fileNamesPtr = NULL;

    // table for dynamic programming
    part = NULL;
    dt = NULL;
 
    // temp index sets to construct comb_num_indexes 
    index_sets = NULL;
    // temp index sets to construct comb_num_indexes 
    color_sets = NULL;
    // record comb num values for active and passive children 
    comb_num_indexes = NULL;
    isTableCreated = false;

    // for comm
    mapper_num = 1;
    local_mapper_id = 0;
    send_array_limit = 1000;
    rotation_pipeline = false;
    abs_v_to_mapper = NULL;
    abs_v_to_queue = NULL;
    comm_mapper_vertex = NULL;
    update_map = NULL;
    update_map_size = NULL;

    update_queue_pos = NULL;
    update_queue_counts = NULL;
    update_queue_index = NULL;
    update_mapper_len = NULL;

    map_ids_cache_pip = NULL;
    chunk_ids_cache_pip = NULL;
    chunk_internal_offsets_cache_pip = NULL;

    update_mapper_id = -1;
    daal_table_size = -1; // -1 means no need to do data copy 
    daal_table_int_ptr = NULL; // tmp array to hold int data
    daal_table_float_ptr = NULL; //tmp array to hold float data

}

void Input::init_comm(int mapper_num_par, int local_mapper_id_par, long send_array_limit_par, bool rotation_pipeline_par)
{
    mapper_num = mapper_num_par;
    local_mapper_id = local_mapper_id_par;
    send_array_limit = send_array_limit_par;
    rotation_pipeline = rotation_pipeline_par;

    NumericTablePtr abs_v_to_mapper_table = get(VMapperId);
    // daal::internal::BlockMicroTable<int, readWrite, sse2> mtVMapperId(abs_v_to_mapper_table.get());
    // mtVMapperId.getBlockOfRows(0, 1, &abs_v_to_mapper);
    size_t table_dim = abs_v_to_mapper_table->getNumberOfRows();
    daal::internal::FeatureMicroTable<int, readOnly, sse2> mtVMapperId(abs_v_to_mapper_table.get());
    mtVMapperId.getBlockOfColumnValues(0, 0, table_dim, &abs_v_to_mapper);

    //retrieve the abs v id to mapper mapping
    //length max_v_id+1 
    //debug check abs_v_to_mapper val
    for(int i=0; i<10;i++)
    {
        int v_id = g.vertex_ids[i]; 
        std::printf("Check abs_v_to_mapper v_id: %d, mapper id: %d\n", v_id, abs_v_to_mapper[v_id]);
    }

    int abs_len = g.max_v_id + 1;
    abs_v_to_queue = new int[abs_len];
    // comm_mapper_vertex = new std::unordered_set<int>[mapper_num];
    // comm_mapper_vertex = new int*[mapper_num];
    comm_mapper_vertex = new std::vector<int>[mapper_num];
    int* comm_mapper_tmp = new int[abs_len*mapper_num];
    std::memset(comm_mapper_tmp, 0, abs_len*mapper_num*sizeof(int));

    int thread_num_max = omp_get_max_threads();

    #pragma omp parallel for schedule(guided) num_threads(thread_num_max) 
    for(int i=0;i<g.vert_num_count;i++)
    {
        v_adj_elem* elem_adj = g.adj_index_table[i];
        int* elem_adj_ptr = &(elem_adj->_adjs)[0];
        int list_size = elem_adj->_adjs.size();
        for(int j=0;j<list_size;j++)
        {
            int adj_abs_id = elem_adj_ptr[j];
            int adj_mapper_id = abs_v_to_mapper[adj_abs_id];
            #pragma omp atomic
            comm_mapper_tmp[adj_mapper_id*abs_len + adj_abs_id]++;
        }
    }

    for(int i=0;i<abs_len*mapper_num;i++)
    {
        if (comm_mapper_tmp[i] > 0)
        {
            int adj_mapper_id = i/abs_len;
            int adj_abs_id = i%abs_len;
            comm_mapper_vertex[adj_mapper_id].push_back(adj_abs_id);
        }
    }

    delete[] comm_mapper_tmp;

    update_map = new int*[g.vert_num_count];
    for(int i=0;i<g.vert_num_count;i++)
        update_map[i] = new int[g.out_degree(i)];

    update_map_size = new int[g.vert_num_count];

    update_queue_pos = new int**[mapper_num];
    update_queue_counts = new float**[mapper_num];
    update_queue_index = new int16_t**[mapper_num];
    update_mapper_len = new int[mapper_num];

    map_ids_cache_pip = new int*[g.vert_num_count];
    chunk_ids_cache_pip = new int*[g.vert_num_count];
    chunk_internal_offsets_cache_pip = new int*[g.vert_num_count];

}

void Input::init_comm_prepare(int update_id)
{
    if (update_id != local_mapper_id && comm_mapper_vertex[update_id].size() > 0)
    {
        //retrieve send array
        daal_table_size = (long)comm_mapper_vertex[update_id].size();

        // daal_table_int_ptr = new int[(int)daal_table_size];
        // int loop_itr = 0;
        // std::set<int>::iterator it;
        // for(it = comm_mapper_vertex[update_id].begin(); it != comm_mapper_vertex[update_id].end(); it++)
        //     daal_table_int_ptr[loop_itr++] = (*it);

        daal_table_int_ptr = &(comm_mapper_vertex[update_id])[0];
        update_mapper_len[update_id] = (int)daal_table_size; 

        for(int i=0;i<(int)daal_table_size;i++)
            abs_v_to_queue[daal_table_int_ptr[i]] = i;
    }
    else
        daal_table_size = -1;

}

void Input::upload_prep_comm()
{
    if (daal_table_size > 0)
    {
        //javaTable
        NumericTablePtr upload_prep_comm_table = get(CommDataId);
        // size_t table_dim = abs_v_to_mapper_table->getNumberOfRows();
        // daal::internal::FeatureMicroTable<int, writeOnly, sse2> mtUpload(upload_prep_comm_table.get());
        // int* daal_tmp_ptr = NULL
        // mtUpload.getBlockOfColumnValues(0, 0, daal_table_size, &daal_tmp_ptr);
        BlockDescriptor<int> upload_block;
        upload_block.setPtr(daal_table_int_ptr, 1, daal_table_size);
        upload_block.setDetails(0, 0, writeOnly);

        (upload_prep_comm_table.get())->releaseBlockOfColumnValues( upload_block );
        upload_block.reset();
    }
}

NumericTablePtr Input::get(InputId id) const
{
    return staticPointerCast<NumericTable, SerializationIface>(Argument::get(id));
}

void Input::set(InputId id, const NumericTablePtr &value)
{
    Argument::set(id, value);
}

size_t Input::getNumberOfColumns(InputId id) const
{
    NumericTablePtr dataTable = get(id);
    if(dataTable)
        return dataTable->getNumberOfColumns();
    else
        return 0;
}

size_t Input::getNumberOfRows(InputId id) const
{
    NumericTablePtr dataTable = get(id);
    if(dataTable)
        return dataTable->getNumberOfRows();
    else
        return 0;
}

daal::services::interface1::Status Input::check(const daal::algorithms::Parameter *parameter, int method) const {services::Status s; return s;}

struct readG_task{

    readG_task(int file_id, hdfsFS*& handle, int*& fileOffsetPtr, int*& fileNamesPtr, std::vector<v_adj_elem*>*& v_adj, int*& max_v_id_local)
    {
        _file_id = file_id;
        _handle = handle;
        _fileOffsetPtr = fileOffsetPtr;
        _fileNamesPtr = fileNamesPtr;
        _v_adj = v_adj;
        _max_v_id_local = max_v_id_local;
    }

    int _file_id;
    hdfsFS* _handle;
    int* _fileOffsetPtr;
    int* _fileNamesPtr;
    std::vector<v_adj_elem*>* _v_adj;
    int* _max_v_id_local;

};

/**
 * @brief test thread func for thread pool
 */
void thd_task_read(int thd_id, void* arg)
{

    readG_task* task = (readG_task*)arg;

    int file_id = task->_file_id;

    std::printf("Thread %d working on task file %d\n", thd_id, file_id);
    std::fflush;

    hdfsFS fs_handle = (task->_handle)[thd_id];
    std::vector<v_adj_elem*>* v_adj_table = &((task->_v_adj)[thd_id]);

    int max_id_thd = (task->_max_v_id_local)[thd_id];

    int* file_off_ptr = task->_fileOffsetPtr;
    int* file_name_ptr = task->_fileNamesPtr;
    int len = file_off_ptr[file_id+1] - file_off_ptr[file_id];
    char* file = new char[len+1];
    for(int j=0;j<len;j++)
        file[j] = (char)(file_name_ptr[file_off_ptr[file_id] + j]);

    // terminate char buf
    file[len] = '\0';
    std::printf("FilePath: %s\n", file);
    std::fflush;

    if (hdfsExists(fs_handle, file) < 0)
        return;

    hdfsFile readFile = hdfsOpenFile(fs_handle, file, O_RDONLY, 0, 0, 0);
    int file_size = hdfsAvailable(fs_handle, readFile);
    int buf_size = 70000;
    char* buf = new char[buf_size];

    std::string tail;
    while(1)
    {
    
        std::memset(buf, '\0', buf_size);
        tSize ret = hdfsRead(fs_handle, readFile, (void*)buf, buf_size); 
        // std::printf("FileSize: %d, Read size: %d\n", file_size, (int)ret);
        // std::fflush;

        std::string obj;
        if (tail.empty())
        {
            if (ret > 0)
            {
                // obj.assign(buf, buf_size);
                obj.assign(buf, ret);
                //may cause redundant '\0' if the buf already contains a '\0'
                obj += '\0';

            }
        }
        else
        {
            if (ret > 0)
            {
                // obj.assign(buf, buf_size);
                obj.assign(buf, ret);
                //may cause  redundant '\0' if the buf already contains a '\0'
                obj += '\0';
                
                //remove '\0' at the end of tail
                std::string tail_v;
                tail_v.assign(tail.c_str(), std::strlen(tail.c_str()));
                obj = tail_v + obj;
            }
            else
                obj = tail;

        }

        if (obj.length() == 0)
            break;

        std::istringstream obj_ss(obj);
        std::string elem;
        while(std::getline(obj_ss, elem, '\n'))
        {
            if (obj_ss.eof())
            {
                tail.clear();
                //trim dangling '\0'
                if (elem[0] != '\0')
                    tail = elem;

                if (ret > 0 || elem[0] == '\0')
                    break;
            }

            std::string header;
            std::string nbrs;

            std::istringstream elem_ss(elem);
            elem_ss >> header;
            elem_ss >> nbrs;

            std::istringstream nbrs_ss(nbrs);
            std::string s;

            //check header contains '\0' 
            int v_id_add = std::stoi(header);
            v_adj_elem* add_one = new v_adj_elem(v_id_add);

            while(std::getline(nbrs_ss, s,','))
            {
                add_one->_adjs.push_back(std::stoi(s));
            }

            //load one vert id and nbr list into table
            if (add_one->_adjs.size() > 0)
            {
                v_adj_table->push_back(add_one);
                max_id_thd = v_id_add> max_id_thd? v_id_add: max_id_thd;
            }
            
        }

        //check again
        if (ret <= 0)
            break;
    }

    hdfsCloseFile(fs_handle, readFile);
    // record thread local max v_id
    (task->_max_v_id_local)[thd_id] = max_id_thd;

    delete[] buf;
    delete[] file;

}

/**
 * @brief read in graph in parallel by using pthread
 */
void Input::readGraph()
{
    // use all the avaiable cpus (threads to read in data)
    thread_num = (sysconf(_SC_NPROCESSORS_ONLN));
    fs = new hdfsFS[thread_num];
    for(int i=0;i<thread_num;i++)
        fs[i] = hdfsConnect("default", 0);

    v_adj = new std::vector<v_adj_elem*>[thread_num];
    int* max_v_id_thdl = new int[thread_num];
    std::memset(max_v_id_thdl, 0, thread_num*sizeof(int));

    NumericTablePtr filenames_array = get(filenames);
    NumericTablePtr filenames_offset = get(fileoffset);

    int file_num = filenames_offset->getNumberOfColumns() - 1;

    daal::internal::BlockMicroTable<int, readWrite, sse2> mtFileOffset(filenames_offset.get());
    mtFileOffset.getBlockOfRows(0, 1, &fileOffsetPtr);

    daal::internal::BlockMicroTable<int, readWrite, sse2> mtFileNames(filenames_array.get());
    mtFileNames.getBlockOfRows(0, 1, &fileNamesPtr);

    std::printf("Finish create hdfsFS files\n");
    std::fflush;

    // create the thread pool
    threadpool thpool = thpool_init(thread_num);
    readG_task** task_queue = new readG_task*[file_num];

    for(int i=0;i<file_num;i++)
    {
        task_queue[i] = new readG_task(i, fs, fileOffsetPtr, fileNamesPtr, v_adj, max_v_id_thdl);
        std::printf("Finish create taskqueue %d\n", i);
        std::fflush;

		thpool_add_work(thpool, (void (*)(int,void*))thd_task_read, task_queue[i]);
    }

    thpool_wait(thpool);
	thpool_destroy(thpool);

    //sum up the total v_ids num on this mapper
    g.vert_num_count = 0;
    g.adj_len = 0;

    for(int i = 0; i< thread_num;i++)
    {
        g.vert_num_count += v_adj[i].size();
        for(int j=0;j<v_adj[i].size(); j++)
        {
            g.adj_len += ((v_adj[i])[j])->_adjs.size();
        }
        
        g.max_v_id_local = max_v_id_thdl[i] > g.max_v_id_local? max_v_id_thdl[i] : g.max_v_id_local;
    }

    std::printf("Finish Reading all the vert num: %d, total nbrs: %d, local max vid: %d\n", g.vert_num_count, g.adj_len, g.max_v_id_local);
    std::fflush;

    // free task queue
    for(int i=0;i<file_num;i++)
        delete task_queue[i];

    delete[] task_queue;
    delete[] max_v_id_thdl; 

}

void Input::readTemplate()
{
  
    hdfsFS tfs_handle = hdfsConnect("default", 0);
    NumericTablePtr filenames_array = get(tfilenames);
    NumericTablePtr filenames_offset = get(tfileoffset);

    int file_num = filenames_offset->getNumberOfColumns() - 1;

    int* tfileOffsetPtr = NULL;
    daal::internal::BlockMicroTable<int, readWrite, sse2> mtFileOffset(filenames_offset.get());
    mtFileOffset.getBlockOfRows(0, 1, &tfileOffsetPtr);

    int* tfileNamesPtr = NULL;
    daal::internal::BlockMicroTable<int, readWrite, sse2> mtFileNames(filenames_array.get());
    mtFileNames.getBlockOfRows(0, 1, &tfileNamesPtr);

    std::printf("Finish create hdfsFS files for template\n");
    std::fflush;

    // for single template
    int file_id = 0;
    int len = tfileOffsetPtr[file_id+1] - tfileOffsetPtr[file_id];
    char* file = new char[len+1];
    for(int j=0;j<len;j++)
        file[j] = (char)(tfileNamesPtr[tfileOffsetPtr[file_id] + j]);

    // terminate char buf
    file[len] = '\0';
    std::printf("Template FilePath: %s\n", file);
    std::fflush;

    if (hdfsExists(tfs_handle, file) < 0)
    {
        std::printf("Cannot open file: %s\n", file);
        std::fflush;
    }

    hdfsFile readFile = hdfsOpenFile(tfs_handle, file, O_RDONLY, 0, 0, 0);
    int file_size = hdfsAvailable(tfs_handle, readFile);
    int buf_size = 70000;
    char* buf = new char[buf_size];

    //start read in template data
    std::string tail;
    bool read_ng = false;
    bool read_mg = false;
    while(1)
    {
        std::memset(buf, '\0', buf_size);
        tSize ret = hdfsRead(tfs_handle, readFile, (void*)buf, buf_size); 
        std::printf("FileSize: %d, Read size: %d\n", file_size, (int)ret);
        std::fflush;

        std::string obj;
        if (tail.empty())
        {
            if (ret > 0)
            {
                obj.assign(buf, ret);
                obj += '\0';
            }
        }
        else
        {
            if (ret > 0)
            {
                obj.assign(buf, ret);
                obj += '\0';
                std::string tail_v;
                tail_v.assign(tail.c_str(), std::strlen(tail.c_str()));
                obj = tail_v + obj;
            }
            else
                obj = tail;
        }

        if (obj.length() == 0)
            break;

        //get a batch of file content
        std::istringstream obj_ss(obj);
        std::string elem;
        while(std::getline(obj_ss, elem, '\n'))
        {
            if (obj_ss.eof())
            {
                tail.clear();
                if (elem[0] != '\0')
                    tail = elem;

                if (ret > 0 || elem[0] == '\0')
                    break;
            }

            //parse the line
            std::istringstream elem_ss(elem);

            if (!read_ng)
            {
                std::string ng;
                elem_ss >> ng;
                
                std::printf("t ng: %s\n", ng.c_str());
                std::fflush;

                read_ng = true;
                t_ng = std::stoi(ng);
                continue;
            }
            
            if (!read_mg)
            {
                std::string mg;
                elem_ss >> mg;
                
                std::printf("t mg: %s\n", mg.c_str());
                std::fflush;
                read_mg = true;
                t_mg = std::stoi(mg);
                continue;
            }

            //read the pair of edges
            std::string src_e;
            std::string dst_e;

            elem_ss >> src_e;
            elem_ss >> dst_e;

            t_src.push_back(std::stoi(src_e));
            t_dst.push_back(std::stoi(dst_e));

            std::printf("src dege: %s, dst edge: %s\n", src_e.c_str(), dst_e.c_str());
            std::fflush;
        }

        //check again
        if (ret <= 0)
            break;
    }

    hdfsCloseFile(tfs_handle, readFile);
    hdfsDisconnect(tfs_handle);
}

void Input::init_Template()
{
    int* src = &t_src[0];
    int* dst = &t_dst[0];
    // t.initTemplate(t_ng, t_mg, &t_src[0], &t_dst[0]);
    t.initTemplate(t_ng, t_mg, src, dst);
    num_colors = t.num_vertices();

    dt = new dynamic_table_array();

    sampleGraph();
}

void Input::init_Partitioner()
{
    int* labels = NULL;
    part = new partitioner(t, false, labels);
    part->sort_subtemplates();
    subtemplate_count = part->get_subtemplate_count();
    subtemplates = part->get_subtemplates(); 
}

void Input::init_DTTable()
{
    dt->init(subtemplates, subtemplate_count, g.num_vertices(), num_colors, g.max_v_id);
}

size_t Input::getTVNum()
{
    return t.vert_num_count;
}

size_t Input::getTENum()
{
    return t.num_edges;
}

/**
 * @brief initialization of internal graph data structure
 */
void Input::init_Graph()
{
    // std::printf("Start init Graph\n");
    // std::fflush;
    //mapping from global v_id starts from 1 to local id
    g.vertex_local_ids = new int[g.max_v_id+1];
    for(int p=0;p<g.max_v_id+1;p++)
        g.vertex_local_ids[p] = -1;

    //undirected graph, num edges equals adj array size
    g.num_edges = g.adj_len;

    //global abs vertex ids
    g.vertex_ids = new int[g.vert_num_count];
    g.max_deg = 0;

    //load v ids from v_adj to vertex_ids
    g.adj_index_table = new v_adj_elem*[g.vert_num_count];

    int itr=0;
    for(int i=0; i<thread_num; i++)
    {
        for(int j=0; j<v_adj[i].size(); j++)
        {
            g.vertex_ids[itr] = ((v_adj[i])[j])->_v_id;
            int v_id = g.vertex_ids[itr];
            g.vertex_local_ids[v_id] = itr;
            g.adj_index_table[itr] = ((v_adj[i])[j]);

            int deg = ((v_adj[i])[j])->_adjs.size();
            g.max_deg = deg > g.max_deg? deg: g.max_deg;
            itr++;
        }
    }

    //shuffle to sort g.vert_ids
    //debug
    // util_quicksort(g.vertex_ids, 0, g.vert_num_count-1, g.vert_num_count);

    // load vertex_ids into daal table
    NumericTablePtr localVTable = get(localV);
    if (localVTable != NULL)
    {
        daal::internal::BlockMicroTable<int, writeOnly, sse2> mtlocalVTable(localVTable.get());
        int* localVTablePtr = NULL;
        mtlocalVTable.getBlockOfRows(0, 1, &localVTablePtr);
        memcpy(localVTablePtr, g.vertex_ids, g.vert_num_count*sizeof(int));
        mtlocalVTable.release();
    }

    // create colrs_g
    colors_g = new int[g.vert_num_count];
    std::memset(colors_g, 0, g.vert_num_count*sizeof(int));
    std::printf("Finish init Graph\n");
    std::fflush;
}

void Input::setGlobalMaxV(size_t id)
{
    g.max_v_id = id;
    std::printf("global vMax id: %d\n", g.max_v_id);
    std::fflush;
}

size_t Input::getReadInThd()
{
    return thread_num;
}

size_t Input::getLocalVNum()
{
    return g.vert_num_count;
}
    
size_t Input::getLocalMaxV()
{
    return g.max_v_id_local;
}
    
size_t Input::getLocalADJLen()
{
    return g.adj_len;
}

size_t Input::getSubtemplateCount()
{
    if (part != NULL)
        return part->get_subtemplate_count();
    else
        return 0;
}
// ------------ for creating hash table ------------
void Input::create_tables()
{
    choose_table = util_init_choose_table(num_colors);
    //record vertices number of each subtemplate
    create_num_verts_table();
    //create color index for each combination of different set size
    create_all_index_sets();
    //create color sets for all subtemplates
    create_all_color_sets();
    //convert colorset combination into numeric values (hash function)
    create_comb_num_system_indexes();
    //free up memory space 
    delete_all_color_sets();
    //free up memory space
    delete_all_index_sets();

    isTableCreated = true;
}

void Input::create_num_verts_table()
{
    num_verts_table = new int[subtemplate_count];
    for(int s = 0; s < subtemplate_count; ++s){
        num_verts_table[s] = subtemplates[s].num_vertices();
    }
}

//create color index for each combination of different set size
void Input::create_all_index_sets()
{

    //first dim (up to) how many colors
    index_sets = new int***[num_colors];

    for(int i = 0; i < (num_colors -1 ); ++i){

        int num_vals = i + 2;

        index_sets[i] = new int**[num_vals -1];

        // second dim, for up to num_vals colors, has different set sizes
        for(int j = 0; j < (num_vals - 1); ++j){

            int set_size = j + 1;

            int num_combinations = util_choose(num_vals, set_size);
            // third dim, for a set size, how many combinations from a given num of colors
            index_sets[i][j] = new int*[num_combinations];

            //set start from 1 to set_size
            //init set in increase order
            int* set = util_init_permutation(set_size);

            for(int k = 0; k < num_combinations; ++k){

                // fourth dim, for a combination, having a set_size of color values
                index_sets[i][j][k] = new int[set_size];

                for(int p = 0; p < set_size; ++p){
                    index_sets[i][j][k][p] = set[p] - 1;
                }

                // permutate the color set
                util_next_set(set, set_size, num_vals);
            }

            if (set != NULL)
                delete[] set;

        }
    }
}

//create color sets for all subtemplates
void Input::create_all_color_sets()
{

    //first dim, num of subtemplates
    color_sets = new int****[subtemplate_count];

    for(int s = 0; s < subtemplate_count; ++s){

        int num_verts_sub = subtemplates[s].num_vertices();

        if( num_verts_sub > 1)
        {

            //determine how many sets in a subtemplate
            //choose num vertices of subtemplate from colors
            int num_sets = util_choose(num_colors, num_verts_sub);

            //second dim, num of sets in a subtemplate 
            color_sets[s] = new int***[num_sets];

            //init permutation in colorset
            int* colorset = util_init_permutation(num_verts_sub);

            for(int n = 0; n < num_sets; ++n){

                int num_child_combs = num_verts_sub - 1;
                //third dim, for a subtemplate, a set, how many child combinations
                color_sets[s][n] = new int**[num_child_combs];

                for(int c = 0; c < num_child_combs; ++c){
                    int num_verts_1 = c + 1;
                    int num_verts_2 = num_verts_sub - num_verts_1;

                    int** index_set_1 = index_sets[num_verts_sub-2][num_verts_1-1];
                    int** index_set_2 = index_sets[num_verts_sub-2][num_verts_2-1];

                    int num_child_sets = util_choose(num_verts_sub, c+1);
                    color_sets[s][n][c] = new int*[num_child_sets];

                    for(int i = 0; i < num_child_sets; ++i){

                        color_sets[s][n][c][i] = new int[num_verts_sub];

                        for(int j = 0; j < num_verts_1; ++j)
                            color_sets[s][n][c][i][j] = colorset[index_set_1[i][j]];

                        for(int j = 0; j < num_verts_2; ++j)
                            color_sets[s][n][c][i][j+num_verts_1] = colorset[index_set_2[i][j]];
                    }
                }

                util_next_set(colorset, num_verts_sub, num_colors);
            }

            if (colorset != NULL)
                delete[] colorset;
        }
    }
}



//convert colorset combination into numeric values (hash function)
void Input::create_comb_num_system_indexes()
{

    comb_num_indexes = new int***[2];
    comb_num_indexes[0] = new int**[subtemplate_count];
    comb_num_indexes[1] = new int**[subtemplate_count];


    comb_num_indexes_set = new int*[subtemplate_count];

    // each subtemplate
    for(int s = 0; s < subtemplate_count; ++s){

        int num_verts_sub = subtemplates[s].num_vertices();
        int num_combinations_s = util_choose(num_colors, num_verts_sub);

        if( num_verts_sub > 1){
            //for active and passive children  
            comb_num_indexes[0][s] = new int*[num_combinations_s];
            comb_num_indexes[1][s] = new int*[num_combinations_s];
        }

        comb_num_indexes_set[s] = new int[num_combinations_s];

        int* colorset_set = util_init_permutation(num_verts_sub);

        //loop over each combination instance
        for(int n = 0; n < num_combinations_s; ++n){

            //get the hash value for a colorset instance
            //Util.get_color_index the impl of hash function
            comb_num_indexes_set[s][n] = util_get_color_index(colorset_set, num_verts_sub);

            if( num_verts_sub > 1){

                int num_verts_a = part->get_num_verts_active(s);
                int num_verts_p = part->get_num_verts_passive(s);

                int* colors_a = NULL;
                int* colors_p = NULL;
                int** colorsets = color_sets[s][n][num_verts_a-1];

                int num_combinations_a= util_choose(num_verts_sub, num_verts_a);
                comb_num_indexes[0][s][n] = new int[num_combinations_a];
                comb_num_indexes[1][s][n] = new int[num_combinations_a];

                int p = num_combinations_a - 1;
                for(int a = 0; a < num_combinations_a; ++a, --p){
                    colors_a = colorsets[a];
                    colors_p = new int[num_verts_p];

                    // System.arraycopy(colorsets[p], num_verts_a, colors_p, 0, num_verts_p);
                    std::memcpy(colors_p, colorsets[p]+num_verts_a, num_verts_p*sizeof(int));

                    int color_index_a = util_get_color_index(colors_a, num_verts_a);
                    int color_index_p = util_get_color_index(colors_p, num_verts_p);

                    comb_num_indexes[0][s][n][a] = color_index_a;
                    comb_num_indexes[1][s][n][p] = color_index_p;

                    //free colors_p
                    delete[] colors_p;
                    colors_p = NULL;
                }
            }

            //permutate the colorset_set
            util_next_set(colorset_set, num_verts_sub, num_colors);

        }

        if (colorset_set != NULL)
          delete[] colorset_set;
    }
}



//free up memory space 
void Input::delete_all_color_sets()
{
    for(int s = 0; s < subtemplate_count; ++s){
        int num_verts_sub = subtemplates[s].num_vertices();
        if( num_verts_sub > 1) {
            int num_sets = util_choose(num_colors, num_verts_sub);

            for (int n = 0; n < num_sets; ++n) {
                int num_child_combs = num_verts_sub - 1;
                for (int c = 0; c < num_child_combs; ++c) {
                    int num_child_sets = util_choose(num_verts_sub, c + 1);
                    for (int i = 0; i < num_child_sets; ++i) {
                        delete[] color_sets[s][n][c][i];
                    }

                    delete[] color_sets[s][n][c];
                }

                delete[] color_sets[s][n];
            }

            delete[] color_sets[s];
        }
    }

    delete[] color_sets;
}



//free up memory space
void Input::delete_all_index_sets()
{
    for (int i = 0; i < (num_colors-1); ++i) {
        int num_vals = i + 2;
        for (int j = 0; j < (num_vals-1); ++j) {
            int set_size = j + 1;
            int num_combinations = util_choose(num_vals, set_size);
            for (int k = 0; k < num_combinations; ++k) {
                delete[] index_sets[i][j][k];
            }
            delete[] index_sets[i][j];
        }
        delete[] index_sets[i];
    }
    delete[] index_sets;
}

void Input::delete_comb_num_system_indexes()
{
    for(int s = 0; s < subtemplate_count; ++s)
    {
        int num_verts_sub = subtemplates[s].num_vertices();
        int num_combinations_s = util_choose(num_colors, num_verts_sub);

        for(int n = 0; n < num_combinations_s; ++n){
            if(num_verts_sub > 1){
                delete[] comb_num_indexes[0][s][n];
                delete[] comb_num_indexes[1][s][n];
            }
        }

        if(num_verts_sub > 1){
            delete[] comb_num_indexes[0][s];
            delete[] comb_num_indexes[1][s];
        }

        delete[] comb_num_indexes_set[s];
    }

    delete[] comb_num_indexes[0];
    delete[] comb_num_indexes[1];
    delete[] comb_num_indexes;
    delete[] comb_num_indexes_set;

}

void Input::delete_tables()
{
    if (isTableCreated)
    {
        for(int i = 0; i <= num_colors; ++i)
            delete[] choose_table[i];

        delete[] choose_table;
        delete_comb_num_system_indexes();
        delete[] num_verts_table;
    }
}

void Input::free_input()
{
    if (fs != NULL)
    {
        for(int i=0;i<thread_num;i++)
            hdfsDisconnect(fs[i]);

        delete[] fs;
    }

    if (v_adj != NULL)
    {
        for(int i=0;i<thread_num;i++)
        {
            for(int j=0;j<v_adj[i].size();j++)
            {
                if ((v_adj[i])[j] != NULL)
                    delete (v_adj[i])[j];
            }
        }

        delete[] v_adj;
    }

    if (dt != NULL)
    {
        dt->free();
        dt->clear_table();
        delete dt;
        dt = NULL;
    }

    // delete table will use subtemplates, must execute this before part clear
    delete_tables();

    if (part != NULL)
    {
        part->clear_temparrays();
        delete part;
        part = NULL;
    }
    
    g.freeMem();
    t.freeMem();

    if (colors_g != NULL)
    {
        delete[] colors_g;
        colors_g = NULL;
    }

}

/**
 * @brief sampling colors_g for each subtemplate computing
 */
void Input::sampleGraph()
{
    struct timespec ts1;
	struct timespec ts2;
    int64_t diff = 0;
    double sample_time = 0;

    // using openmp
    daal::internal::BaseRNGs<sse2> base_gen(time(0));
    daal::internal::RNGs<int, sse2> rand_gen;

    clock_gettime(CLOCK_MONOTONIC, &ts1);
    rand_gen.uniform(g.num_vertices(), colors_g, base_gen.getState(), 0, num_colors);
    
    clock_gettime(CLOCK_MONOTONIC, &ts2);
    diff = 1000000000L *(ts2.tv_sec - ts1.tv_sec) + ts2.tv_nsec - ts1.tv_nsec;
    sample_time = (double)(diff)/1000000L;

    std::printf("Finish sampling in time: %f ms\n", sample_time);
    std::fflush;

    // for(int i=0;i<10;i++)
    // {
    //     std::printf("random color: %d\n", colors_g[i]);
    //     std::fflush;
    // }

}

int Input::getSubVertN(int sub_itr)
{
    return part->get_num_verts_sub(sub_itr);
}

void Input::initDtSub(int s)
{
    int a = part->get_active_index(s); 
    int p = part->get_passive_index(s);
    std::printf("Init Dt sub for s: %d, a: %d, p: %d\n", s, a, p);
    std::fflush;

    dt->init_sub(s, a, p);
}

void Input::clearDtSub(int s)
{
    int a = part->get_active_index(s); 
    int p = part->get_passive_index(s);

    std::printf("Clear Dt sub for s: %d, a: %d, p: %d\n", s, a, p);
    std::fflush;

    if (a != null_val)
    {
        if (part->get_num_verts_sub(a) > 1)
            dt->clear_sub(a);
    }

    if (p != null_val)
    {
        // do not clear soft copied bottom dt node
        if (part->get_num_verts_sub(p) > 1)
            dt->clear_sub(p);
    }

    std::printf("Finish Clear Dt sub for s: %d, a: %d, p: %d\n", s, a, p);
    std::fflush;

}

void Input::setToTable(int src, int dst)
{
    dt->set_to_table(src, dst);
}

int Input::computeMorphism()
{
    return util_count_automorphisms(t);
}

// ------------------------ func for partitioner------------------------
partitioner::partitioner(Graph& t, bool label, int*& label_map)
{
    //debug printout graph t
    for(int i = 0; i<t.vert_num_count; i++)
    {
        std::printf("t vert: %d\n", t.vertex_ids[i]);
        std::fflush;
        int* adj_list = t.adjacent_vertices(i);
        int deg = t.out_degree(i);
        for(int j=0;j<deg;j++)
        {
            std::printf("%d,", adj_list[j]);
            std::fflush;
        }
            
        std::printf("\n");
        std::fflush;
    }

    init_arrays();
    labeled = label;
    subtemplates_create[0] = t;
    //start from 1
    current_creation_index = 1;

    if(labeled){
        label_maps.push_back(label_map);
    }

    parents.push_back(null_val);

    int root = 0;

    //recursively partition the template
    partition_recursive(0, root);

    fin_arrays();
}

int partitioner::split_sub(int s, int root, int other_root)
{
    subtemplate = subtemplates_create[s];
    int* labels_sub = NULL;

    if(labeled)
        labels_sub = label_maps[s];

    //source and destination arrays for edges
    std::vector<int> srcs;
    std::vector<int> dsts;

    //get the previous vertex to avoid backtracking
    int previous_vertex = other_root;

    //loop through the rest of the vertices
    //if a new edge is found, add it
    std::vector<int> next;
    //start from the root
    //record all the edges in template execpt for other root branch
    next.push_back(root);
    int cur_next = 0;
    while( cur_next < next.size()){

        int u = next[cur_next++];
        int* adjs = subtemplate.adjacent_vertices(u);
        int end = subtemplate.out_degree(u);

        //loop over all the adjacent vertices of u
        for(int i = 0; i < end; i++){
            int v = adjs[i];
            bool add_edge = true;

            //avoiding add repeated edges
            for(int j = 0; j < dsts.size(); j++){
                if(srcs[j] == v && dsts[j] == u){
                    add_edge = false;
                    break;
                }
            }

            if( add_edge && v != previous_vertex){
                srcs.push_back(u);
                dsts.push_back(v);
                next.push_back(v);
            }
        }
    }

    //if empty, just add the single vert;
    int n;
    int m;
    int* labels = NULL;

    if( srcs.size() > 0){
        m = srcs.size();
        n = m + 1;

        if(labeled)
        {
            //extract_uniques
            std::set<int> unique_ids;
            for(int x = 0; x<srcs.size();x++)
                unique_ids.insert(srcs[x]);

            labels = new int[unique_ids.size()];

        }

        check_nums(root, srcs, dsts, labels, labels_sub);

    }else{
        //single node
        m = 0;
        n = 1;
        srcs.push_back(0);

        if( labeled){
            labels = new int[1];
            labels[0] = labels_sub[root];
        }
    }

    int* srcs_array = util_dynamic_to_static(srcs);
    int* dsts_array = util_dynamic_to_static(dsts);

    //create a subtemplate
    subtemplates_create[current_creation_index].initTemplate(n, m, srcs_array, dsts_array);

    if (srcs_array != NULL)
        delete[] srcs_array;

    if (dsts_array != NULL)
        delete[] dsts_array;

    current_creation_index++;

    if(labeled)
        label_maps.push_back(labels);

    return srcs[0];

}

void partitioner::check_nums(int root, std::vector<int>& srcs, std::vector<int>& dsts, int*& labels, int*& labels_sub)
{
    int maximum = util_get_max(srcs, dsts);
    int size = srcs.size();

    int* mappings = new int[maximum + 1];

    for(int i = 0; i < maximum + 1; ++i){
        mappings[i] = -1;
    }

    int new_map = 0;
    mappings[root] = new_map++;

    for(int i = 0; i < size; ++i){
        if( mappings[srcs[i]] == -1)
            mappings[srcs[i]] = new_map++;

        if( mappings[dsts[i]] == -1 )
            mappings[dsts[i]] = new_map++;
    }

    for(int i = 0; i < size; ++i){
        srcs[i]  = mappings[srcs[i]];
        dsts[i]  = mappings[dsts[i]];
    }

    if( labeled ){
        for (int i = 0; i < maximum; ++i){
            labels[ mappings[i] ] = labels_sub[i];
        }
    }

    if (mappings != NULL)
        delete[] mappings;

}

void partitioner::sort_subtemplates()
{

    bool swapped;
    Graph temp_g;

    do{
        swapped = false;
        for(int i = 2; i < subtemplate_count; ++i){
            if( parents[i] < parents[i-1]){

                //copy the subtemplate obj
                temp_g = subtemplates[i];
                int temp_pr = parents[i];
                int temp_a = active_children[i];
                int temp_p = passive_children[i];
                int* temp_l = NULL;
                if(labeled)
                    temp_l = label_maps[i];

                //if either have children, need to update their parents
                if( active_children[i] != null_val)
                    parents[active_children[i]] = i-1;

                if(active_children[i-1] != null_val)
                    parents[active_children[i-1]] = i;

                if( passive_children[i] != null_val)
                    parents[passive_children[i]] = i-1;

                if( passive_children[i-1] != null_val)
                    parents[passive_children[i-1]] = i;

                // need to update their parents
                if( active_children[parents[i]] == i)
                    active_children[parents[i]] = i-1;
                else if( passive_children[parents[i]] == i )
                    passive_children[parents[i]] = i-1;

                if( active_children[parents[i-1]] == i-1)
                    active_children[parents[i-1]] = i;
                else if(passive_children[parents[i-1]] == i-1 )
                    passive_children[parents[i-1]] = i;

                // make the switch copy data
                subtemplates[i] = subtemplates[i-1];
                parents[i] =  parents[i-1];
                active_children[i] = active_children[i-1];
                passive_children[i] = passive_children[i-1];

                if(labeled)
                    label_maps[i] =  label_maps[i-1];

                subtemplates[i-1] = temp_g;
                parents[i-1] =  temp_pr;
                active_children[i-1] =  temp_a;
                passive_children[i-1]  = temp_p;
                if(labeled)
                    label_maps[i-1]  = temp_l;

                swapped = true;
            }
        }
    }while(swapped);

}

void partitioner::partition_recursive(int s, int root)
{

    //split the current subtemplate using the current root
    int* roots = split(s, root);

    //debug
    std::printf("Split roots s: %d, r: %d\n", s, root);
    std::fflush;

    //set the parent/child tree structure
    int a = current_creation_index - 2;
    int p = current_creation_index - 1;
    set_active_child(s, a);
    set_passive_child(s, p);
    set_parent(a, s);
    set_parent(p, s);

    //specify new roots and continue partitioning
    int num_verts_a = subtemplates_create[a].vert_num_count;
    int num_verts_p = subtemplates_create[p].vert_num_count;


    if( num_verts_a > 1){
        int activeRoot = roots[0];
        partition_recursive(a, activeRoot);
    }else{
        set_active_child(a, null_val);
        set_passive_child(a, null_val);
    }

    //debug
    std::printf("Finish recursive active %d\n", a);
    std::fflush;

    if( num_verts_p > 1){
        int passiveRoot = roots[1];
        partition_recursive(p, passiveRoot);
    }else{
        set_active_child(p, null_val);
        set_passive_child(p, null_val);
    }

    //debug
    std::printf("Finish recursive passive %d\n", p);
    std::fflush;

    //debug
    std::printf("Finish recursive s: %d, r: %d\n", s, root);
    std::fflush;

}

int* partitioner::split(int s, int root)
{
    //get new root
    //get adjacency vertices of the root vertex
    int* adjs = subtemplates_create[s].adjacent_vertices(root);

    //get the first adjacent vertex
    int u = adjs[0];

    //split this subtemplate between root and node u
    //create a subtemplate rooted at root
    int active_root = split_sub(s, root, u);
    //create a subtemplate rooted at u
    int passive_root = split_sub(s, u, root);

    int* retval = new int[2];
    retval[0] = active_root;
    retval[1] = passive_root;
    return retval;
}

void partitioner::fin_arrays()
{
    subtemplate_count = current_creation_index;
    subtemplates = new Graph[subtemplate_count];

    for(int i = 0; i < subtemplate_count; ++i){
        //copy
        subtemplates[i] = subtemplates_create[i];
        subtemplates_create[i].freeMem();
    }

    delete[] subtemplates_create;
    subtemplates_create = NULL;

    count_needed = new bool[subtemplate_count];
    for(int i = 0; i < subtemplate_count; ++i){
        count_needed[i] = true;
    }

}

void partitioner::clear_temparrays()
{
    if (subtemplates != NULL)
    {
        for(int i=0;i<subtemplate_count;i++)
            subtemplates[i].freeMem();

        delete [] subtemplates;
        subtemplates = NULL;
    }

    if (count_needed != NULL)
    {
        delete [] count_needed;
        count_needed = NULL;
    }
}

// -------------------- impl of Graph struct  --------------------
void Graph::initTemplate(int ng, int mg, int*& src, int*& dst)
{
    //free memory if the graph is re-init
    freeMem();

    vert_num_count = ng;
    num_edges = 2*mg;
    adj_len = num_edges;
    max_deg = 0;

    vertex_local_ids = new int[vert_num_count+1];
    vertex_ids = new int[vert_num_count];
    adj_index_table = new v_adj_elem*[vert_num_count];
    //initialize adj table
    for(int i=0; i<vert_num_count;i++)
        adj_index_table[i] = new v_adj_elem(0);

    for(int i = 0; i < mg; ++i)
    {
        adj_index_table[src[i]]->_v_id = src[i];
        adj_index_table[src[i]]->_adjs.push_back(dst[i]);

        adj_index_table[dst[i]]->_v_id = dst[i];
        adj_index_table[dst[i]]->_adjs.push_back(src[i]);

        vertex_local_ids[src[i]] = src[i];
        vertex_local_ids[dst[i]] = dst[i];
    }

    for(int i = 0; i < vert_num_count; ++i)
    {
        vertex_ids[i] = i;
        max_deg = adj_index_table[i]->_adjs.size() > max_deg? adj_index_table[i]->_adjs.size() : max_deg;
    }

    isTemplate = true;
}

void Graph::freeMem()
{
    if (vertex_ids != NULL)
    {
        delete[] vertex_ids; // absolute v_id
        vertex_ids = NULL;
    }

    if (vertex_local_ids != NULL)
    {
        delete[] vertex_local_ids; // mapping from absolute v_id to relative v_id
        vertex_local_ids = NULL;
    }

    if (isTemplate && adj_index_table != NULL)
    {
        for(int i=0; i<vert_num_count;i++)
            delete adj_index_table[i];
    }

    if (adj_index_table != NULL)
    {
        delete[] adj_index_table; //a table to index adj list for each local vert
        adj_index_table = NULL;
    }
}

Graph& Graph::operator= (const Graph& param)
{
    this->freeMem();
    vert_num_count = param.vert_num_count;
    max_v_id_local = param.max_v_id_local;
    max_v_id = param.max_v_id;
    adj_len = param.adj_len;
    num_edges = param.num_edges;
    max_deg = param.max_deg;

    vertex_ids = new int[vert_num_count];
    std::memcpy(vertex_ids, param.vertex_ids, vert_num_count*sizeof(int));

    vertex_local_ids = new int[vert_num_count+1];
    std::memcpy(vertex_local_ids, param.vertex_local_ids, (vert_num_count+1)*sizeof(int));

    adj_index_table = new v_adj_elem*[vert_num_count];
    for(int i=0;i<vert_num_count;i++)
    {
        adj_index_table[i] = new v_adj_elem((param.adj_index_table[i])->_v_id);
        adj_index_table[i]->_adjs =  (param.adj_index_table[i])->_adjs;
    }

    isTemplate = param.isTemplate;
    return *this;
}

// ------------------ Impl of dynamic table ------------------
dynamic_table_array::dynamic_table_array()
{
    choose_table = NULL;
    num_colorsets = NULL;

    subtemplates = NULL;

    num_colors = 0;
    num_subs = 0;
    num_verts = 0;

    is_inited = false;
    is_sub_inited = NULL;

    table = NULL;
    // vertex-colorset
    cur_table = NULL;
    // vertex-colorset
    cur_table_active = NULL;
    // vertex-colorset
    cur_table_passive = NULL;

    max_abs_vid = 0;
    cur_sub = 0;
}

void dynamic_table_array::free()
{

    if (num_colorsets != NULL)
    {
        delete[] num_colorsets;
        num_colorsets = NULL;
    }

    if (choose_table != NULL)
    {
        for(int i=0;i<num_colors + 1; i++)
            delete[] choose_table[i];

        delete[] choose_table; 
        choose_table = NULL;
    }

}

void dynamic_table_array::init_choose_table()
{
    std::printf("init dt table: %d\n", num_colors);
    std::fflush;

    choose_table = new int*[num_colors + 1];
    for(int i=0;i<num_colors+1; i++)
        choose_table[i] = new int[num_colors+1];

    // for(int i = 0; i <= num_colors; ++i){
    //     for(int j = 0; j <= num_colors; ++j){
    //         choose_table[i][j] = util_choose(i,j);
    //     }
    // }
}

void dynamic_table_array::init_num_colorsets()
{
    num_colorsets = new int[num_subs];
    for(int s = 0; s < num_subs; ++s){
        num_colorsets[s] = util_choose(num_colors, subtemplates[s].num_vertices());
    }
}

void dynamic_table_array::init(Graph*& _subtemplates, int _num_subtemplates, int _num_vertices, int _num_colors, int _max_abs_vid) 
{
    subtemplates = _subtemplates;
    num_subs = _num_subtemplates;
    //num of vertices of full graph 
    num_verts = _num_vertices;
    num_colors = _num_colors;
    max_abs_vid = _max_abs_vid;

    //obtain the table,choose j color from i color, the num of combinations
    init_choose_table();
    //obtain color sets for each subtemplate
    init_num_colorsets();
    // the core three-dimensional arrays in algorithm
    table = new float**[num_subs];
    for(int s=0;s<num_subs;s++)
        table[s] = NULL;

    is_sub_inited = new bool[num_subs];
    for(int s = 0; s < num_subs; ++s)
        is_sub_inited[s] = false;

    is_inited = true;

}

void dynamic_table_array::init_sub(int subtemplate) 
{
    table[subtemplate] = new float*[num_verts];
    for(int v=0;v<num_verts; v++)
        table[subtemplate][v] = NULL;

    cur_table = table[subtemplate];
    cur_sub = subtemplate;
    is_sub_inited[subtemplate] = true;

}

void dynamic_table_array::init_sub(int subtemplate, int active_child, int passive_child)
{
    if( active_child != null_val && passive_child != null_val){
        cur_table_active = table[active_child];
        cur_table_passive = table[passive_child];
    }else{

        cur_table_active = NULL;
        cur_table_passive = NULL;
    }

    if(subtemplate != 0){
        init_sub(subtemplate);
    }

}

void dynamic_table_array::clear_sub(int subtemplate) 
{

    if (is_sub_inited[subtemplate] && table[subtemplate] != NULL)
    {
        for(int v = 0; v < num_verts; ++v){
            if( table[subtemplate][v] != NULL){
                delete[] table[subtemplate][v];
                table[subtemplate][v] = NULL;
            }
        }

        if( is_sub_inited[subtemplate]){
            delete[] table[subtemplate];
            table[subtemplate] = NULL;
        }

        is_sub_inited[subtemplate] = false;
    }
}

void dynamic_table_array::clear_table() 
{
    for( int s = 0; s < num_subs; s++){

        if( is_sub_inited[s] && table[s] != NULL)
        {
            for(int v = 0; v < num_verts; ++v){
                if ( table[s][v] != NULL){
                    delete[] table[s][v];
                }
            }

            delete[] table[s];
            is_sub_inited[s] = false;
        }

        std::printf("Finish Final clear sub: %d\n", s);
        std::fflush;
    }

    if (table != NULL)
        delete[] table;

    if (is_sub_inited != NULL)
        delete[] is_sub_inited;

}

float dynamic_table_array::get(int subtemplate, int vertex, int comb_num_index)
{
    if( table[subtemplate][vertex] != NULL){
        float retval = table[subtemplate][vertex][comb_num_index];
        return retval;
    }else{
        return 0.0;
    }

}

float* dynamic_table_array::get_table(int subtemplate, int vertex)
{
    return table[subtemplate][vertex];
}

float dynamic_table_array::get_active(int vertex, int comb_num_index)
{
    if( cur_table_active[vertex] != NULL){
        return cur_table_active[vertex][comb_num_index];
    }else{
        return 0.0;
    }
}

float* dynamic_table_array::get_active(int vertex)
{
    return cur_table_active[vertex];
}

float* dynamic_table_array::get_passive(int vertex)
{
    return cur_table_passive[vertex];
}

float dynamic_table_array::get_passive(int vertex, int comb_num_index)
{
    if( cur_table_passive[vertex] != NULL){
        return cur_table_passive[vertex][comb_num_index];
    }else{
        return 0.0;
    }
}

void dynamic_table_array::set(int subtemplate, int vertex, int comb_num_index, float count)
{
    if( table[subtemplate][vertex] == NULL){

        table[subtemplate][vertex] = new float[num_colorsets[subtemplate]];

        for(int c = 0; c < num_colorsets[subtemplate]; ++c){
            table[subtemplate][vertex][c] = 0.0;
        }
    }

    table[subtemplate][vertex][comb_num_index]  = count;
}

void dynamic_table_array::set(int vertex, int comb_num_index, float count)
{
    if( cur_table[vertex] == NULL){
        cur_table[vertex] = new float[num_colorsets[cur_sub]];

        for( int c = 0; c < num_colorsets[cur_sub]; ++c) {
            cur_table[vertex][c] = 0.0;
        }
    }

    cur_table[vertex][comb_num_index] = count;
}

//shall deal with the uninit vertex in local
void dynamic_table_array::update_comm(int vertex, int comb_num_index, float count)
{
    if( cur_table[vertex] == NULL){
        cur_table[vertex] = new float[num_colorsets[cur_sub]];

        for( int c = 0; c < num_colorsets[cur_sub]; ++c) {
            cur_table[vertex][c] = 0.0;
        }
    }

    cur_table[vertex][comb_num_index] += count;
}
    
bool dynamic_table_array::is_init() 
{
    return is_inited;
}

bool dynamic_table_array::is_sub_init(int subtemplate) 
{
    return is_sub_inited[subtemplate];
}

bool dynamic_table_array::is_vertex_init_active(int vertex)
{
    if( cur_table_active[vertex] != NULL)
        return true;
    else
        return false;
}

bool dynamic_table_array::is_vertex_init_passive(int vertex)
{
    if(cur_table_passive[vertex] != NULL)
        return true;
    else
        return false;
}

int dynamic_table_array::get_num_color_set(int s) 
{
    if (num_colorsets != NULL)
        return num_colorsets[s];
    else
        return 0;
}

void dynamic_table_array::set_to_table(int s, int d)
{
    table[d] = table[s]; 
}

// --------------------- aux function impl ---------------------
int util_partition(int*& arr, const int left, const int right) 
{
    const int mid = left + (right - left) / 2;
    const int pivot = arr[mid];
    // move the mid point value to the front.
    std::swap(arr[mid],arr[left]);
    int i = left + 1;
    int j = right;
    while (i <= j) 
    {
        while(i <= j && arr[i] <= pivot) 
        {
            i++;
        }

        while(i <= j && arr[j] > pivot) 
        {
            j--;
        }

        if (i < j) 
        {
            std::swap(arr[i], arr[j]);
        }

    }

    std::swap(arr[i - 1],arr[left]);
    return i - 1;
}

void util_quicksort(int*& arr, const int left, const int right, const int sz)
{

    if (left >= right) {
        return;
    }

    int part = util_partition(arr, left, right);
    util_quicksort(arr, left, part - 1, sz);
    util_quicksort(arr, part + 1, right, sz);

}

int* util_dynamic_to_static(std::vector<int>& arr)
{
    int* new_array = new int[arr.size()];
    for (size_t i = 0; i < arr.size(); ++i)
        new_array[i] = arr[i];

    return new_array;
}

int util_get_max(std::vector<int> arr1, std::vector<int> arr2)
{
    int maximum = 0; 
    int size = arr1.size();

    for (int i = 0; i < size; i++)
    {
        if (maximum < arr1[i])
        {
            maximum = arr1[i];
        }
        if (maximum < arr2[i])
        {
            maximum = arr2[i];
        }
    }

    return maximum;
}

int util_choose(int n, int k)
{
    if( n < k){
        return 0;
    }else{
        return util_factorial(n) / (util_factorial(k) * util_factorial(n - k));
    }
}

int util_factorial(int x)
{
    if( x <= 0){
        return 1;
    }else{
        return (x == 1 ? x: x * util_factorial(x -1));
    }
}

int** util_init_choose_table(int num_colors)
{

    int** u_choose_table = new int*[num_colors + 1];
    for(int i=0;i<num_colors + 1;i++)
        u_choose_table[i] = new int[num_colors + 1];


    for(int i = 0; i <= num_colors; ++i){
        for(int j = 0; j <= num_colors; ++j){
            u_choose_table[i][j] = util_choose(i,j);
        }
    }

    return u_choose_table;
}

int* util_init_permutation(int num_verts)
{
    int* perm = new int[num_verts];
    for(int i = 0; i < num_verts; ++i){
        perm[i] = i + 1;
    }
    return perm;
}

void util_next_set(int*& current_set, int length, int num_colors)
{
    for(int i = length-1; i>=0; --i)
    {
        if( current_set[i] < num_colors - (length-i-1))
        {
            current_set[i] = current_set[i] + 1;
            for(int j = i + 1; j < length; ++j){
                current_set[j] = current_set[j-1] + 1;
            }
            break;
        }
    }
}

int util_get_color_index(int*& colorset, int length)
{
    int count = 0;
    for(int i = 0; i < length; ++i){
        int n = colorset[i] - 1;
        int k = i + 1;
        count += util_choose(n, k);
    }

    return count;
}

int util_test_automorphism(Graph& tp, std::vector<int>& mapping)
{
    for(int v = 0; v < mapping.size(); ++v){

        if( tp.out_degree(v) != tp.out_degree(mapping[v]))
            return 0;
        else{

            int* adjs = tp.adjacent_vertices(v);
            int* adjs_map = tp.adjacent_vertices(mapping[v]);
            int end = tp.out_degree(v);

            bool* match = new bool[end];

            for(int i = 0; i < end; ++i){
                match[i] = false;
                int u = adjs[i];

                for(int j = 0; j < end; ++j){
                    int u_map = adjs_map[j];
                    if( u == mapping[u_map])
                        match[i] = true;
                }
            }
            for(int i = 0; i < end; ++i){
                if( !match[i])
                    return 0;
            }
        }
    }

    return 1;
}

int util_count_all_automorphisms(Graph& tp, std::vector<int>& mapping, std::vector<int>& rest)
{
    int count = 0;
    if( rest.size() == 0){
        count = util_test_automorphism(tp, mapping);
        return count;

    }else{

        for(int i = 0; i < rest.size(); ++i){
            mapping.push_back(rest[i]);

            std::vector<int> new_rest;

            for(int j = 0; j < rest.size(); ++j){
                if( i != j)
                    new_rest.push_back(rest[j]);
            }

            count += util_count_all_automorphisms(tp, mapping, new_rest);
            new_rest.clear();
            // mapping.erase(mapping.size()-1);
            mapping.pop_back();
        }
    }

    return count;
}

int util_count_automorphisms(Graph& tp)
{
    int num_verts = tp.num_vertices();

    std::vector<int> mapping;
    std::vector<int> rest;
    for(int i = 0; i < num_verts; ++i){
        rest.push_back(i);
    }

    return util_count_all_automorphisms(tp, mapping, rest);

}

} // namespace interface1
} // namespace subgraph
} // namespace algorithm
} // namespace daal
