/* file: pca_batchparameter_correlation_fpt.cpp */
/*******************************************************************************
* Copyright 2014-2017 Intel Corporation
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
//  Implementation of PCA algorithm interface.
//--
*/

#include "pca_batchparameter_correlation.h"

namespace daal
{
namespace algorithms
{
namespace pca
{

template DAAL_EXPORT BatchParameter<DAAL_FPTYPE, correlationDense>::BatchParameter(const services::SharedPtr<covariance::BatchImpl> &covariance);
template DAAL_EXPORT services::Status BatchParameter<DAAL_FPTYPE, correlationDense>::check() const;

}// namespace pca
}// namespace algorithms
}// namespace daal