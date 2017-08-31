/* file: subgraph_dense_default_container.h */
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
//  Implementation of subgraph calculation algorithm container.
//--
*/
#include <math.h>       
#include <cstdlib> 
#include <ctime> 
#include <iostream>
#include <random>
#include "numeric_table.h"
#include "service_rng.h"
#include "subgraph_types.h"
#include "subgraph_batch.h"
#include "subgraph_default_kernel.h"

namespace daal
{
namespace algorithms
{
namespace subgraph
{

template<typename interm, Method method, CpuType cpu>
BatchContainer<interm, method, cpu>::BatchContainer(daal::services::Environment::env *daalEnv)
{
    // initialize kernels
    __DAAL_INITIALIZE_KERNELS(internal::subgraphBatchKernel, interm, method);
}

template<typename interm, Method method, CpuType cpu>
BatchContainer<interm, method, cpu>::~BatchContainer()
{
    __DAAL_DEINITIALIZE_KERNELS();
}

template<typename interm, Method method, CpuType cpu>
daal::services::interface1::Status BatchContainer<interm, method, cpu>::compute()
{
    //main body of computation for BatchContainer

    // Input *input = static_cast<Input *>(_in);
    // Result *result = static_cast<Result *>(_res);
    //
    // /* retrieve the training dataset and test dataset */
    // const NumericTable *a0 = static_cast<const NumericTable *>(input->get(dataTrain).get());
    // const NumericTable *a1 = static_cast<const NumericTable *>(input->get(dataTest).get());
    //
    // const NumericTable **TrainSet = &a0;
    // const NumericTable **TestSet = &a1;
    //
    // /* get the W and H model */
    // NumericTable *r[2];
    // r[0] = static_cast<NumericTable *>(result->get(resWMat).get());
    // r[1] = static_cast<NumericTable *>(result->get(resHMat).get());
    //
    // /* initialize mode W and H with random values */
    // size_t w_row = r[0]->getNumberOfRows();
    // size_t w_col = r[0]->getNumberOfColumns();
    //
    // size_t h_row = r[1]->getNumberOfRows();
    // size_t h_col = r[1]->getNumberOfColumns();
    //
    // BlockDescriptor<interm> W_Block;
    // BlockDescriptor<interm> H_Block;
    //
    // r[0]->getBlockOfRows(0, w_row, writeOnly, W_Block); 
    // r[1]->getBlockOfRows(0, h_row, writeOnly, H_Block); 
    //
    // interm *W_Ptr = W_Block.getBlockPtr();
    // interm *H_Ptr = H_Block.getBlockPtr();
    //
    // interm scale = 1.0/sqrt(static_cast<interm>(w_col));
    //
	// // daal::internal::UniformRng<interm, daal::sse2> rng1(time(0));
	// daal::internal::BaseRNGs<daal::sse2> base_rng1;
	// daal::internal::RNGs<interm, daal::sse2> rng1;
    //
    // // rng1.uniform(w_row*w_col, 0.0, scale, W_Ptr);
    // rng1.uniform(w_row*w_col, W_Ptr, base_rng1.getState(), 0.0, scale);
    //
	// // daal::internal::UniformRng<interm, daal::sse2> rng2(time(0));
    // daal::internal::BaseRNGs<daal::sse2> base_rng2;
	// daal::internal::RNGs<interm, daal::sse2> rng2;
    //
    // // rng2.uniform(h_row*h_col, 0.0, scale, H_Ptr);
    // rng2.uniform(h_row*h_col, H_Ptr, base_rng2.getState(), 0.0, scale);
    //
    // daal::algorithms::Parameter *par = _par;
    // daal::services::Environment::env &env = *_env;
    //
    // /* invoke the subgraphBatchKernel */
    // __DAAL_CALL_KERNEL(env, internal::subgraphBatchKernel, __DAAL_KERNEL_ARGUMENTS(interm, method), compute, TrainSet, TestSet, r, par);

    services::Status status;
    return status;
}


}
}
} // namespace daal
