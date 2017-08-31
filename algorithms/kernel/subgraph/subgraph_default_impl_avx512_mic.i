/* file: subgraph_default_batch_impl_avx512_mic.i */
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
//  AVX512-MIC optimization of auxiliary functions used in default method 
//  of subgraph_batch 
//--
*/
#include <iostream>

// template<> void updateMF_explicit<DAAL_FPTYPE, avx512_mic>(DAAL_FPTYPE* WMat, DAAL_FPTYPE* HMat, DAAL_FPTYPE* workV, int idx, const long dim_r, const DAAL_FPTYPE rate, const DAAL_FPTYPE lambda)



