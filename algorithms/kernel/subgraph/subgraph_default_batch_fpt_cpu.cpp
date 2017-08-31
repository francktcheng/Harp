/* file: subgraph_dense_default_batch_fpt_cpu.cpp */
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
//  Instantiation of subgraph algorithm classes.
//--
*/

#include "subgraph_default_kernel.h"
#include "subgraph_default_batch_impl.i"
#include "subgraph_default_container.h"

namespace daal
{
namespace algorithms
{
namespace subgraph
{
namespace interface1
{
template class BatchContainer<DAAL_FPTYPE, daal::algorithms::subgraph::defaultSC, DAAL_CPU>;
}
namespace internal
{
template class subgraphBatchKernel<DAAL_FPTYPE, defaultSC, DAAL_CPU>;
}
}
}
}
