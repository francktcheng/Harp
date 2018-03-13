/* file: MasterInputId.java */
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

/**
 * @ingroup pca
 * @{
 */
package com.intel.daal.algorithms.pca;

/**
 * <a name="DAAL-CLASS-ALGORITHMS__PCA__MASTERINPUTID"></a>
 * @brief Available identifiers of the PCA algorithm on the second step in the distributed processing mode
 */
public final class MasterInputId {
    private int _value;

    static {
        System.loadLibrary("JavaAPI");
    }

    /**
     * Constructs the master input object identifier using the provided value
     * @param value     Value corresponding to the master input object identifier
     */
    public MasterInputId(int value) {
        _value = value;
    }

    /**
     * Returns the value corresponding to the master input object identifier
     * @return Value corresponding to the master input object identifier
     */
    public int getValue() {
        return _value;
    }

    private static final int partialResultsId = 0;

    /** Partial results computed by the PCA algorithm on the first step in the distributed processing mode */
    public static final MasterInputId partialResults = new MasterInputId(partialResultsId);
}
/** @} */