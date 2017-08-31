/* file: subgraph_distri.h */
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
//  Implementation of the interface for the subgraph algorithm in the
//  distributed processing mode, users could use MPI or Hadoop to handle the communication
//  tasks
//--
*/

#ifndef __SUBGRAPH_DISTRI_H__
#define __SUBGRAPH_DISTRI_H__

#include "algorithms/algorithm.h"
#include "data_management/data/numeric_table.h"
#include "services/daal_defines.h"

#include "algorithms/subgraph/subgraph_types.h"

#include "error_indexes.h"

namespace daal
{
namespace algorithms
{
namespace subgraph
{

namespace interface1
{
/** @defgroup subgraph_distri 
* @ingroup subgraph_distri
* @{
*/
/**
 * <a name="DAAL-CLASS-ALGORITHMS__subgraph__BATCHCONTAINER"></a>
 * \brief Provides methods to run implementations of the subgraph algorithm in distributed mode 
 *
 * \tparam algorithmFPType  Data type to use in intermediate computations of the subgraph algorithm, double or float
 * \tparam method           Computation method of the subgraph algorithm, \ref daal::algorithms::subgraph::Method
 *
 */
template<ComputeStep step, typename algorithmFPType, Method method, CpuType cpu>
class DAAL_EXPORT DistriContainer : public daal::algorithms::AnalysisContainerIface<distributed>
{
public:
    /**
     * Constructs a container for the subgraph algorithm with a specified environment
     * in the distributed mode
     * \param[in] daalEnv   Environment object
     */
    DistriContainer(daal::services::Environment::env *daalEnv);
    /** Default destructor */
    virtual ~DistriContainer();
    /**
     * Computes the result of the subgraph algorithm in the distributed processing mode
     */
    virtual daal::services::interface1::Status compute() DAAL_C11_OVERRIDE;

    virtual daal::services::interface1::Status finalizeCompute() DAAL_C11_OVERRIDE;
};

/**
 * <a name="DAAL-CLASS-ALGORITHMS__subgraph_DISTRIBUTED"></a>
 * \brief Computes the results of the subgraph algorithm in the distributed processing mode.
 * \n<a href="DAAL-REF-subgraph-ALGORITHM">subgraph algorithm description and usage models</a>
 *
 * \tparam algorithmFPType  Data type to use in intermediate computations for the subgraph algorithm, double or float
 * \tparam method           Computation method of the algorithm, \ref daal::algorithms::subgraph::Method
 *
 * \par Enumerations
 * \ref Method   Computation methods for the subgraph algorithm
 */
template<ComputeStep step, typename algorithmFPType = double, Method method = defaultSC>
class DAAL_EXPORT Distri : public daal::algorithms::Analysis<distributed>
{
public:
    Input input;            /*!< Input object */
    Parameter parameter;    /*!< subgraph parameters */

	/* default constructor */
    Distri()
    {
        initialize();
    }

    /**
     * Constructs a subgraph algorithm by copying input objects and parameters
     * of another subgraph algorithm
     * @param[in] other An algorithm to be used as the source to initialize the input objects
     *                  and parameters of the algorithm
     */
    Distri(const Distri<step, algorithmFPType, method> &other)
    {
        initialize();
        // input.set(dataTrain, other.input.get(dataTrain)); /* copy the input training dataset */
        parameter = other.parameter;
    }

	/**
	 * @brief Returns method of the algorithm
	 * @return 
	 */
    virtual int getMethod() const DAAL_C11_OVERRIDE { return(int)method; }

    /**
     * @brief Returns the structure that contains the results of the subgraph algorithm
     * @return Structure that contains the results of the subgraph algorithm
     */
    services::SharedPtr<Result> getResult() { return _result;}

    /**
     * @brief Returns the structure that contains the partial results of the subgraph algorithm
     * @return Structure that contains the partial results of the subgraph algorithm
     */
    services::SharedPtr<DistributedPartialResult> getPartialResult() { return _presult;}

    /**
     * @brief Register user-allocated memory to store the results of the subgraph algorithm
	 * @param[in] user-allocated Result object
     */
    daal::services::interface1::Status setResult(const services::SharedPtr<Result>& res)
    {
        daal::services::interface1::Status status;
        DAAL_CHECK(res, daal::services::ErrorNullResult)
        _result = res;
        _res = _result.get();
        return status;
    }

	/**
	 * @brief Register user-allocated memory to store the partial results of the subgraph algorithm
	 * @param[in] user-allocated DistributedPartialResult object
	 */
    daal::services::interface1::Status setPartialResult(const services::SharedPtr<DistributedPartialResult>& pres)
    {
        daal::services::interface1::Status status;
        DAAL_CHECK(pres, daal::services::ErrorNullPartialResult)
        _presult = pres;
        _pres = pres.get();
        return status;
    }

    /**
     * @brief Returns a pointer to the newly allocated subgraph algorithm
     * with a copy of input objects and parameters of this subgraph algorithm
     * @return Pointer to the newly allocated algorithm
     */
    services::SharedPtr<Distri<step, algorithmFPType, method> > clone() const
    {
        return services::SharedPtr<Distri<step, algorithmFPType, method> >(cloneImpl());
    }

protected:

	/**
	 * @brief a copy function  
	 * @return a pointer to copyed Distri object 
	 */
    virtual Distri<step, algorithmFPType, method> * cloneImpl() const DAAL_C11_OVERRIDE
    {
        return new Distri<step, algorithmFPType, method>(*this);
    }

    virtual daal::services::interface1::Status allocateResult() DAAL_C11_OVERRIDE {
        daal::services::interface1::Status status;
        return status;
    }

    virtual daal::services::interface1::Status allocatePartialResult() DAAL_C11_OVERRIDE {
        daal::services::interface1::Status status;
        return status;
    }

    virtual daal::services::interface1::Status initializePartialResult() DAAL_C11_OVERRIDE {
        daal::services::interface1::Status status;
        return status;
    }

    void initialize()
    {
        Analysis<distributed>::_ac = new __DAAL_ALGORITHM_CONTAINER(distributed, DistriContainer, step, algorithmFPType, method)(&_env);
        _in   = &input;
        _par  = &parameter;
    }

private:
    services::SharedPtr<Result> _result;
    services::SharedPtr<DistributedPartialResult> _presult;
};
/** @} */
} // namespace interface1
using interface1::DistriContainer;
using interface1::Distri;

} // namespace daal::algorithms::subgraph
}
} // namespace daal
#endif
