/* file: Distri.java */
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

/**
 * @brief Contains classes for computing the MF-SGD-Distri 
 */
package com.intel.daal.algorithms.subgraph;
import com.intel.daal.algorithms.AnalysisDistributed;
import com.intel.daal.algorithms.ComputeMode;
import com.intel.daal.algorithms.Precision;
import com.intel.daal.services.DaalContext;

/**
 * <a name="DAAL-CLASS-ALGORITHMS__subgraph__DISTRI"></a>
 * @brief Computes the results of the subgraph algorithm in the distributed processing mode
 * \n<a href="DAAL-REF-MF-SGD-ALGORITHM">subgraph algorithm description and usage models</a>
 *
 * @par References
 *      - Method class.  Computation methods for the subgraph algorithm
 *      - InputId class. Identifiers of input objects for the subgraph algorithm
 *      - ResultId class. Identifiers of the results of the algorithm
 *      - Input class
 *      - Result class
 */
public class Distri extends AnalysisDistributed {

    public Input					  input;         /*!< %Input data */
    public Method					  method;        /*!< Computation method for the algorithm */
	public Parameter				  parameter;     /*!< Parameters for the algorithm */
    private Precision                 prec;          /*!< Precision of intermediate computations */

    /** @private */
    static {
        System.loadLibrary("JavaAPI");
    }

    /**
     * Constructs the subgraph algorithm by copying input objects and parameters
     * of another subgraph algorithm
     * @param context   Context to manage the subgraph algorithm
     * @param other     An algorithm to be used as the source to initialize the input objects
     *                  and parameters of the algorithm
     */
    public Distri(DaalContext context, Distri other) {
        super(context);
        this.method = other.method;
        prec = other.prec;

        this.cObject = cClone(other.cObject, prec.getValue(), method.getValue());
        input = new Input(getContext(), cGetInput(cObject, prec.getValue(), method.getValue()));
		parameter = new Parameter(getContext(), cGetParameter(cObject, prec.getValue(), method.getValue()));
    }

    /**
     * Constructs the subgraph algorithm
     * @param context   Context to manage the subgraph algorithm
     * @param cls       Data type to use in intermediate computations for the subgraph algorithm,
     *                  Double.class or Float.class
     * @param method    Computation method, @ref Method
     */
    public Distri(DaalContext context, Class<? extends Number> cls, Method method) {
        super(context);

        this.method = method;

        if (method != Method.defaultSC) {
            throw new IllegalArgumentException("method unsupported");
        }

        if (cls != Double.class && cls != Float.class) {
            throw new IllegalArgumentException("type unsupported");
        }

        if (cls == Double.class) {
            prec = Precision.doublePrecision;
        }
        else {
            prec = Precision.singlePrecision;
        }

        this.cObject = cInit(prec.getValue(), method.getValue());
        input = new Input(getContext(), cGetInput(cObject, prec.getValue(), method.getValue()));
		parameter = new Parameter(getContext(), cGetParameter(cObject, prec.getValue(), method.getValue()));

    }

    public void computeBottom(int sub_itr) {
        parameter.setSubItr(sub_itr);
        parameter.setStage(0);
        super.compute();
    }

    public void computeNonBottom(int sub_itr){
        parameter.setSubItr(sub_itr);
        parameter.setStage(1);
        super.compute();
    }

    // public void computeLast(int sub_itr){
    //     parameter.setSubItr(sub_itr);
    //     parameter.setStage(2);
    //     super.compute();
    // }

    /**
     * Runs the subgraph algorithm
     * @return  Partial Results of the subgraph_distri mode 
     */
    @Override
    public PartialResult compute() {
        super.compute();
        return null;
    }

    /**
     * Computes final results of the subgraph algorithm     
     */
    // @Override
    public Result finalizeCompute() {
        super.finalizeCompute();
        return null;
    }

    /**
     * Registers user-allocated memory for storing the results of the subgraph algorithm
     * @param result Structure for storing the results of the subgraph algorithm
     */
    public void setResult(Result result) {
        cSetResult(cObject, prec.getValue(), method.getValue(), result.getCObject());
    }

    public void setPartialResult(PartialResult presult) {
        cSetPartialResult(cObject, prec.getValue(), method.getValue(), presult.getCObject());
    }

    /**
     * Returns the newly allocated subgraph algorithm
     * with a copy of input objects and parameters of this subgraph algorithm
     * @param context   Context to manage the subgraph algorithm
     *
     * @return The newly allocated algorithm
     */
    @Override
    public Distri clone(DaalContext context) {
        return new Distri(context, this);
    }


    private native long cInit(int prec, int method);

    private native long cGetInput(long algAddr, int prec, int method);

    private native long cGetParameter(long algAddr, int prec, int method);

    private native void cSetResult(long cAlgorithm, int prec, int method, long cResult);

    private native void cSetPartialResult(long cAlgorithm, int prec, int method, long cPartialResult);

    private native long cClone(long algAddr, int prec, int method);
}