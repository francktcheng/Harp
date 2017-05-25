/* file: NumericTableInputId.java */
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

package com.intel.daal.algorithms.implicit_als.training;

/**
 * <a name="DAAL-CLASS-ALGORITHMS__IMPLICIT_ALS__TRAINING__NUMERICTABLEINPUTID"></a>
 * @brief Available identifiers of input numeric table objects for the implicit ALS
 * training algorithm in the distributed processing mode
 */
public final class NumericTableInputId {
    private int _value;

    static {
        System.loadLibrary("JavaAPI");
    }

    /**
     * Constructs the input object identifier using the provided__value
     * @param value     Value of the input object identifier
     */
    public NumericTableInputId(int value) {
        _value = value;
    }

    /**
     * Returns the value of the input object identifier
     * @return    Value of the input object identifier
     */
    public int getValue() {
        return _value;
    }

    private static final int dataId = 0;

    /** %Input data table */
    public static final NumericTableInputId data = new NumericTableInputId(
            dataId); /*!< %Input data table that contains ratings */
}
