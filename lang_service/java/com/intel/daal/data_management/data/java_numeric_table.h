/* file: java_numeric_table.h */
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
//  Implementation of the class that connects Java and C++ Numeric Tables
//--
*/

#ifndef __JAVA_NUMERIC_TABLE_H__
#define __JAVA_NUMERIC_TABLE_H__

#include <jni.h>
#include <tbb/tbb.h>

#include <iostream>
#include <cstdio>

#include "numeric_table.h"
#include "data_management/data/data_serialize.h"

using namespace daal::data_management;

namespace daal
{

class JavaNumericTableBase
{
public:
    virtual ~JavaNumericTableBase() {}

    static void setJavaVM(JavaVM *jvm);

    static JavaVM *getJavaVM();

    static void setDaalContext(jobject context);

    static jobject getDaalContext();

    virtual jobject getJavaObject() const = 0;

private:
    static JavaVM *globalJavaVM;
    static tbb::enumerable_thread_specific<jobject> globalDaalContext;
};

/**
 *  \brief Class that implements C++ to Java "connector".
 *  Getters and Setters of this class are callbacks
 *  to the corresponding methods of user-defined Java class.
 */
template<int Tag>
class DAAL_EXPORT JavaNumericTable : public NumericTable, public JavaNumericTableBase
{
public:
    DECLARE_SERIALIZABLE_TAG();

    explicit JavaNumericTable(DictionaryIface::FeaturesEqual featuresEqual = DictionaryIface::notEqual):
        NumericTable(0, 0, featuresEqual), jvm(NULL), jJavaNumTable(NULL)
    {
    }

    /**
     *  Constructor
     *
     *  \param featnum[in]       Number of features
     *  \param obsnum[in]        Number of observations
     *  \param _jvm[in]          Java VM interface function table
     *  \param _JavaNumTable[in] Java object associated with this C++ object
     */
    JavaNumericTable(size_t featnum, size_t obsnum, JavaVM *_jvm, jobject _JavaNumTable,
                     StorageLayout layout = layout_unknown, DictionaryIface::FeaturesEqual featuresEqual = DictionaryIface::notEqual):
        NumericTable(featnum, obsnum, featuresEqual), jvm(_jvm)
    {
        _layout = layout;
        _memStatus = userAllocated;
        jJavaNumTable = NULL;

        _tls local_tls = tls.local();

        /* mark current thread as 'main' in order not to detach it further */
        local_tls.is_main_thread = true;

        jint status = jvm->AttachCurrentThread((void **)(&(local_tls.jenv)), NULL);
        if(status != JNI_OK)
        {
            this->_status.add(services::ErrorCouldntAttachCurrentThreadToJavaVM);
        }
        else
        {
            local_tls.is_attached = true;
            jJavaNumTable = (local_tls.jenv)->NewGlobalRef(_JavaNumTable);
            if(jJavaNumTable == NULL)
            {
                this->_status.add(services::ErrorCouldntCreateGlobalReferenceToJavaObject);
            }
        }

        tls.local() = local_tls;
    }

    /**
     *  Destructor
     */
    virtual ~JavaNumericTable()
    {
        if(jvm != NULL)
        {
            _tls local_tls = tls.local();
            jint status = jvm->AttachCurrentThread((void **)(&(local_tls.jenv)), NULL);
            if(status != JNI_OK)
            {
                this->_status.add(services::ErrorCouldntAttachCurrentThreadToJavaVM);
            }
            else
            {
                local_tls.is_attached = true;
            }

            if (jJavaNumTable != NULL)
            {
                (local_tls.jenv)->DeleteGlobalRef(jJavaNumTable);
            }
        }
    }

    void serializeImpl (InputDataArchive  *arch) DAAL_C11_OVERRIDE
    {serialImpl<InputDataArchive, false>(arch);}

    void deserializeImpl(OutputDataArchive *arch) DAAL_C11_OVERRIDE
    {serialImpl<OutputDataArchive, true>(arch);}

    services::Status getBlockOfRows(size_t vector_idx, size_t vector_num, ReadWriteMode rwflag, BlockDescriptor<double> &block) DAAL_C11_OVERRIDE
    {
        return getTBlock<double>(vector_idx, vector_num, rwflag, block,
        "getDoubleBlock", "(JJLjava/nio/ByteBuffer;)Ljava/nio/DoubleBuffer;");
    }
    services::Status getBlockOfRows(size_t vector_idx, size_t vector_num, ReadWriteMode rwflag, BlockDescriptor<float> &block) DAAL_C11_OVERRIDE
    {
        return getTBlock<float>(vector_idx, vector_num, rwflag, block,
        "getFloatBlock", "(JJLjava/nio/ByteBuffer;)Ljava/nio/FloatBuffer;");
    }
    services::Status getBlockOfRows(size_t vector_idx, size_t vector_num, ReadWriteMode rwflag, BlockDescriptor<int> &block) DAAL_C11_OVERRIDE
    {
        return getTBlock<int>(vector_idx, vector_num, rwflag, block,
        "getIntBlock", "(JJLjava/nio/ByteBuffer;)Ljava/nio/IntBuffer;");
    }


    services::Status releaseBlockOfRows(BlockDescriptor<double> &block) DAAL_C11_OVERRIDE
    {
        return releaseTBlock<double>(block, "releaseDoubleBlock");
    }
    services::Status releaseBlockOfRows(BlockDescriptor<float> &block) DAAL_C11_OVERRIDE
    {
        return releaseTBlock<float>(block, "releaseFloatBlock");
    }
    services::Status releaseBlockOfRows(BlockDescriptor<int> &block) DAAL_C11_OVERRIDE
    {
        return releaseTBlock<int>(block, "releaseIntBlock");
    }


    services::Status getBlockOfColumnValues(size_t feature_idx, size_t vector_idx, size_t value_num,
                                ReadWriteMode rwflag, BlockDescriptor<double> &block) DAAL_C11_OVERRIDE
    {
        return getTFeature<double>(feature_idx, vector_idx, value_num, rwflag, block,
        "getDoubleFeature", "(JJJLjava/nio/ByteBuffer;)Ljava/nio/DoubleBuffer;");
    }
    services::Status getBlockOfColumnValues(size_t feature_idx, size_t vector_idx, size_t value_num,
                                ReadWriteMode rwflag, BlockDescriptor<float> &block) DAAL_C11_OVERRIDE
    {
        return getTFeature<float>(feature_idx, vector_idx, value_num, rwflag, block,
        "getFloatFeature", "(JJJLjava/nio/ByteBuffer;)Ljava/nio/FloatBuffer;");
    }
    services::Status getBlockOfColumnValues(size_t feature_idx, size_t vector_idx, size_t value_num,
                                ReadWriteMode rwflag, BlockDescriptor<int> &block) DAAL_C11_OVERRIDE
    {
        return getTFeature<int>(feature_idx, vector_idx, value_num, rwflag, block,
        "getIntFeature", "(JJJLjava/nio/ByteBuffer;)Ljava/nio/IntBuffer;");
    }


    /**
     * @brief get block of column values in parallel (multi-threading) 
     * for harp-daal framework
     *
     * @param feature_idx
     * @param vector_idx
     * @param value_num
     * @param rwflag
     * @param block
     */
    void getBlockOfColumnValuesBM(size_t feature_start, size_t feature_len, size_t vector_idx, size_t value_num,
                                ReadWriteMode rwflag, BlockDescriptor<double>** block) DAAL_C11_OVERRIDE
    {
        return getTFeatureBM<double>(feature_start, feature_len, vector_idx, value_num, rwflag, block,
        "getDoubleFeature", "(JJJLjava/nio/ByteBuffer;)Ljava/nio/DoubleBuffer;");
    }

    void getBlockOfColumnValuesBM(size_t feature_start, size_t feature_len, size_t vector_idx, size_t value_num,
                                ReadWriteMode rwflag, BlockDescriptor<float>** block) DAAL_C11_OVERRIDE
    {
        return getTFeatureBM<float>(feature_start, feature_len, vector_idx, value_num, rwflag, block,
        "getFloatFeature", "(JJJLjava/nio/ByteBuffer;)Ljava/nio/FloatBuffer;");
    }

    void getBlockOfColumnValuesBM(size_t feature_start, size_t feature_len, size_t vector_idx, size_t value_num,
                                ReadWriteMode rwflag, BlockDescriptor<int>** block) DAAL_C11_OVERRIDE
    {
        return getTFeatureBM<int>(feature_start, feature_len, vector_idx, value_num, rwflag, block,
        "getIntFeature", "(JJJLjava/nio/ByteBuffer;)Ljava/nio/IntBuffer;");
    }


    services::Status releaseBlockOfColumnValues(BlockDescriptor<double> &block) DAAL_C11_OVERRIDE
    {
        return releaseTFeature<double>(block, "releaseDoubleFeature");
    }
    services::Status releaseBlockOfColumnValues(BlockDescriptor<float> &block) DAAL_C11_OVERRIDE
    {
        return releaseTFeature<float>(block, "releaseFloatFeature");
    }
    services::Status releaseBlockOfColumnValues(BlockDescriptor<int> &block) DAAL_C11_OVERRIDE
    {
        return releaseTFeature<int>(block, "releaseIntFeature");
    }

    /**
     * @brief release multiple columns in parallel (multi-threading) 
     * for harp-daal framework
     *
     * @param feature_start
     * @param feature_len
     * @param block
     */
    void releaseBlockOfColumnValuesBM(size_t feature_start, size_t feature_len, BlockDescriptor<double>** block) DAAL_C11_OVERRIDE
    {
        releaseTFeatureBM<double>(feature_start, feature_len, block, "releaseDoubleFeature");
    }

    void releaseBlockOfColumnValuesBM(size_t feature_start, size_t feature_len, BlockDescriptor<float>** block) DAAL_C11_OVERRIDE
    {
        releaseTFeatureBM<float>(feature_start, feature_len, block, "releaseFloatFeature");
    }

    void releaseBlockOfColumnValuesBM(size_t feature_start, size_t feature_len, BlockDescriptor<int>** block) DAAL_C11_OVERRIDE
    {
        releaseTFeatureBM<int>(feature_start, feature_len, block, "releaseIntFeature");
    }

    template<typename T>
    services::Status getTBlock(size_t idx, size_t nrows, ReadWriteMode rwFlag, BlockDescriptor<T> &block,
                   const char *javaMethodName, const char *javaMethodSignature)
    {
        jint status = JNI_OK;
        _tls local_tls = tls.local();

        block.setDetails( 0, idx, rwFlag );

        /* Get JNI interface pointer for current thread */
        status = jvm->AttachCurrentThread((void **)(&(local_tls.jenv)), NULL);
        if(status != JNI_OK) return services::Status(services::ErrorCouldntAttachCurrentThreadToJavaVM);

        local_tls.is_attached = true;

        size_t ncols = _ddict->getNumberOfFeatures();
        size_t bufferSize = nrows * ncols * sizeof(T);

        if( !block.resizeBuffer( ncols, nrows ) ) { return services::Status(); }

        void *buf = block.getBlockPtr();

        /* Get class associated with Java object */
        local_tls.jcls = (local_tls.jenv)->GetObjectClass(jJavaNumTable);
        if(local_tls.jcls == NULL)
        {
            return services::Status(services::ErrorCouldntFindClassForJavaObject);
        }

        /* Get ID of the 'getBlockOfRows' method of the Java class */
        jmethodID jmeth = (local_tls.jenv)->GetMethodID(local_tls.jcls, javaMethodName,
                                                        javaMethodSignature);
        if(jmeth == NULL)
        {
            return services::Status(services::Error::create(services::ErrorCouldntFindJavaMethod, services::Method, services::String(javaMethodName)));
        }

        /* Call 'getBlockOfRows' Java method */
        local_tls.jbuf = (local_tls.jenv)->NewDirectByteBuffer( buf, bufferSize);
        local_tls.jbuf = (local_tls.jenv)->CallObjectMethod(jJavaNumTable, jmeth, (jlong)idx, (jlong)nrows, local_tls.jbuf);

        buf = (local_tls.jenv)->GetDirectBufferAddress(local_tls.jbuf);

        tls.local() = local_tls;

        block.setPtr( (T *)buf, ncols, nrows );

        if(!local_tls.is_main_thread)
        {
            status = jvm->DetachCurrentThread();
            if(status != JNI_OK)
            {
                return services::Status(services::ErrorCouldntAttachCurrentThreadToJavaVM);
            }
        }
        return services::Status();
    }

    template<typename T>
    services::Status releaseTBlock(BlockDescriptor<T> &block, const char *javaMethodName)
    {
        jint status = JNI_OK;
        _tls local_tls = tls.local();
        if(block.getRWFlag() == writeOnly || block.getRWFlag() == readWrite)
        {
            /* Get JNI interface pointer for current thread */
            status = jvm->AttachCurrentThread((void **)(&(local_tls.jenv)), NULL);
            if(status != JNI_OK)
            {
                return services::Status(services::ErrorCouldntAttachCurrentThreadToJavaVM);
            }

            /* Get class associated with Java object */
            local_tls.jcls = (local_tls.jenv)->GetObjectClass(jJavaNumTable);
            if(local_tls.jcls == NULL)
            {
                return services::Status(services::ErrorCouldntFindClassForJavaObject);
            }

            /* Get ID of the 'releaseBlockOfRows' method of the Java class */
            jmethodID jmeth = (local_tls.jenv)->GetMethodID(local_tls.jcls, javaMethodName, "(JJLjava/nio/ByteBuffer;)V");
            if(jmeth == NULL)
            {
                return services::Status(services::Error::create(services::ErrorCouldntFindJavaMethod, services::Method, services::String(javaMethodName)));
            }

            size_t idx   = block.getRowsOffset();
            size_t nrows = block.getNumberOfRows();
            size_t ncols = _ddict->getNumberOfFeatures();
            size_t bufferSize = nrows * ncols * sizeof(T);
            local_tls.jbuf = (local_tls.jenv)->NewDirectByteBuffer( block.getBlockPtr(), bufferSize);

            /* Call 'releaseBlockOfRows' Java method */
            (local_tls.jenv)->CallObjectMethod(
                jJavaNumTable, jmeth, (jlong)idx, (jlong)nrows, local_tls.jbuf, block.getRWFlag());

            if(!local_tls.is_main_thread)
            {
                status = jvm->DetachCurrentThread();
                if(status != JNI_OK)
                {
                    return services::Status(services::ErrorCouldntAttachCurrentThreadToJavaVM);
                }
            }
        }

        tls.local() = local_tls;
        block.reset();
        return services::Status();
    }


    template<typename T>
    services::Status getTFeature(size_t feature_idx, size_t idx, size_t nrows, ReadWriteMode rwFlag, BlockDescriptor<T> &block,
                     const char *javaMethodName, const char *javaMethodSignature)
    {
        jint status = JNI_OK;
        _tls local_tls = tls.local();

        block.setDetails( feature_idx, idx, rwFlag );

        /* Get JNI interface pointer for current thread */
        status = jvm->AttachCurrentThread((void **)(&(local_tls.jenv)), NULL);
        if(status != JNI_OK)
        {
            return services::Status(services::ErrorCouldntAttachCurrentThreadToJavaVM);
        }

        local_tls.is_attached = true;

        size_t bufferSize = nrows * sizeof(T);

        if( !block.resizeBuffer( 1, nrows ) ) { return services::Status(); }

        void *buf = block.getBlockPtr();

        /* Get class associated with Java object */
        local_tls.jcls = (local_tls.jenv)->GetObjectClass(jJavaNumTable);
        if(local_tls.jcls == NULL)
        {
            return services::Status(services::ErrorCouldntFindClassForJavaObject);
        }

        /* Get ID of the 'getBlockOfRows' method of the Java class */
        jmethodID jmeth = (local_tls.jenv)->GetMethodID(local_tls.jcls, javaMethodName,
                                                        javaMethodSignature);
        if(jmeth == NULL)
        {
            return services::Status(services::Error::create(services::ErrorCouldntFindJavaMethod, services::Method, services::String(javaMethodName)));
        }

        /* Call 'getBlockOfRows' Java method */
        local_tls.jbuf = (local_tls.jenv)->NewDirectByteBuffer(buf, bufferSize);

        local_tls.jbuf = (local_tls.jenv)->CallObjectMethod(
                             jJavaNumTable, jmeth, (jlong)feature_idx, (jlong)idx, (jlong)nrows, local_tls.jbuf);

        buf = (local_tls.jenv)->GetDirectBufferAddress(local_tls.jbuf);

        tls.local() = local_tls;

        block.setPtr( (T *)buf, 1, nrows );
        return services::Status();
    }

    template<typename T>
    void getTFeatureBM(size_t feature_start, size_t feature_len, size_t idx, size_t nrows, ReadWriteMode rwFlag, BlockDescriptor<T>** block,
                     const char *javaMethodName, const char *javaMethodSignature)
    {
        jint status = JNI_OK;
        _tls local_tls = tls.local();

        /* Get JNI interface pointer for current thread */
        status = jvm->AttachCurrentThread((void **)(&(local_tls.jenv)), NULL);
        if(status != JNI_OK)
        {
            // this->_errors->add(services::ErrorCouldntAttachCurrentThreadToJavaVM);
            std::printf("Get Tfeature error:services::ErrorCouldntAttachCurrentThreadToJavaVM\n");
            std::fflush(stdout);
            return;
        }

        local_tls.is_attached = true;

        /* Get class associated with Java object */
        local_tls.jcls = (local_tls.jenv)->GetObjectClass(jJavaNumTable);
        if(local_tls.jcls == NULL)
        {
            // this->_errors->add(services::ErrorCouldntFindClassForJavaObject);
            std::printf("Get Tfeature error:services::ErrorCouldntFindClassForJavaObject\n");
            std::fflush(stdout);

            return;
        }

        /* Get ID of the 'getBlockOfRows' method of the Java class */
        jmethodID jmeth = (local_tls.jenv)->GetMethodID(local_tls.jcls, javaMethodName,
                                                        javaMethodSignature);
        if(jmeth == NULL)
        {
            services::SharedPtr<services::Error> e(new services::Error(services::ErrorCouldntFindJavaMethod));
            e->addStringDetail(services::Method, services::String(javaMethodName));
            // this->_errors->add(e);
            std::printf("Get Tfeature error:services::ErrorCouldntFindJavaMethod\n");
            std::fflush(stdout);

            return;
        }

        //create a buffer space and wrap it to a java.nio.directbytebuffer
        size_t bufferSize = nrows * sizeof(T);
        T* buf_space = (T*)malloc(bufferSize);

        //Main loop to retrieve data from javaNumTable to blockDescriptor
        size_t feature_idx = 0;
        void* buf = NULL;
        void* javabuf = NULL;

        jobject jbuf = (local_tls.jenv)->NewDirectByteBuffer(buf_space, bufferSize);

        for(int k=0;k<feature_len;k++)
        {
            feature_idx = feature_start + k;

            block[feature_idx]->setDetails(feature_idx, idx, rwFlag);

            if( !(block[feature_idx]->resizeBuffer( 1, nrows )) ) { return; }

            buf = block[feature_idx]->getBlockPtr();

            //return a double buffer instead of a bytebuffer
            jobject jdoubleBuf = (local_tls.jenv)->CallObjectMethod(
                             jJavaNumTable, jmeth, (jlong)feature_idx, (jlong)idx, (jlong)nrows, jbuf);

            javabuf = (local_tls.jenv)->GetDirectBufferAddress(jbuf);

            std::memcpy(buf, javabuf, nrows*sizeof(T));

            //delete local reference of jdoubleBuf
            (local_tls.jenv)->DeleteLocalRef(jdoubleBuf);

        }
        
        free(buf_space);
    }

    template<typename T>
    services::Status releaseTFeature(BlockDescriptor<T> &block, const char *javaMethodName)
    {
        jint status = JNI_OK;
        if(block.getRWFlag() == writeOnly || block.getRWFlag() == readWrite)
        {
            _tls local_tls = tls.local();
            /* Get JNI interface pointer for current thread */
            status = jvm->AttachCurrentThread((void **)(&(local_tls.jenv)), NULL);
            if(status != JNI_OK)
            {
                return services::Status(services::ErrorCouldntAttachCurrentThreadToJavaVM);
            }

            /* Get class associated with Java object */
            local_tls.jcls = (local_tls.jenv)->GetObjectClass(jJavaNumTable);
            if(local_tls.jcls == NULL)
            {
                return services::Status(services::ErrorCouldntFindClassForJavaObject);
            }

            /* Get ID of the 'releaseBlockOfRows' method of the Java class */
            jmethodID jmeth = (local_tls.jenv)->GetMethodID(local_tls.jcls, javaMethodName,
                                                            "(JJJLjava/nio/ByteBuffer;)V");
            if(jmeth == NULL)
            {
                return services::Status(services::Error::create(services::ErrorCouldntFindJavaMethod, services::Method, services::String(javaMethodName)));
            }

            size_t idx   = block.getRowsOffset();
            size_t nrows = block.getNumberOfRows();
            size_t feature_idx = block.getColumnsOffset();

            size_t bufferSize = nrows * sizeof(T);
            local_tls.jbuf = (local_tls.jenv)->NewDirectByteBuffer( block.getBlockPtr(), bufferSize);

            /* Call 'releaseBlockOfRows' Java method */
            (local_tls.jenv)->CallObjectMethod( jJavaNumTable, jmeth, (jlong)feature_idx, (jlong)idx, (jlong)nrows,
                                                local_tls.jbuf, block.getRWFlag());

            tls.local() = local_tls;
        }
        block.reset();
        return services::Status();
    }

    template<typename T>
    void releaseTFeatureBM(size_t feature_start, size_t feature_len, BlockDescriptor<T>** block, const char *javaMethodName)
    {
        jint status = JNI_OK;
        _tls local_tls = tls.local();
        
        /* Get JNI interface pointer for current thread */
        status = jvm->AttachCurrentThread((void **)(&(local_tls.jenv)), NULL);
        if(status != JNI_OK)
        {
            // this->_errors->add(services::ErrorCouldntAttachCurrentThreadToJavaVM);
            std::printf("Release TFeature error:services::ErrorCouldntAttachCurrentThreadToJavaVM\n");
            std::fflush(stdout);
            return;
        }

        local_tls.is_attached = true;
        
        /* Get class associated with Java object */
        local_tls.jcls = (local_tls.jenv)->GetObjectClass(jJavaNumTable);
        if(local_tls.jcls == NULL)
        {
            // this->_errors->add(services::ErrorCouldntFindClassForJavaObject);
            std::printf("Release TFeature error:services::ErrorCouldntFindClassForJavaObject\n");
            std::fflush(stdout);
            return;
        }

        /* Get ID of the 'releaseBlockOfRows' method of the Java class */
        jmethodID jmeth = (local_tls.jenv)->GetMethodID(local_tls.jcls, javaMethodName,
                "(JJJLjava/nio/ByteBuffer;)V");
        if(jmeth == NULL)
        {
            services::SharedPtr<services::Error> e(new services::Error(services::ErrorCouldntFindJavaMethod));
            e->addStringDetail(services::Method, services::String(javaMethodName));
            // this->_errors->add(e);
            std::printf("Release TFeature error:services::ErrorCouldntFindJavaMethod\n");
            std::fflush(stdout);

            return;
        }

        //create a buffer space and wrap it to a java.nio.directbytebuffer
        size_t nrows = block[feature_start]->getNumberOfRows();
        size_t bufferSize = nrows * sizeof(T);
        T* buf_space = (T*)malloc(bufferSize);

        size_t feature_idx = 0;
        size_t idx = 0;
        void* buf = NULL;
        void* javabuf = NULL;

        jobject jbuf = (local_tls.jenv)->NewDirectByteBuffer(buf_space, bufferSize);
        javabuf = (local_tls.jenv)->GetDirectBufferAddress(jbuf);

        for(int k=0;k<feature_len;k++)
        {
            feature_idx = feature_start + k;
            buf = block[feature_idx]->getBlockPtr();
            idx  = block[feature_idx]->getRowsOffset();
            std::memcpy(javabuf, buf, nrows*sizeof(T));
            //caal "releaseDoubleFeature
            (local_tls.jenv)->CallObjectMethod( jJavaNumTable, jmeth, (jlong)feature_idx, (jlong)idx, (jlong)nrows,
                        jbuf, writeOnly);

        }

        free(buf_space);

    }

    virtual jobject getJavaObject() const DAAL_C11_OVERRIDE
    {
        return jJavaNumTable;
    }

protected:
    struct _tls
    {
        JNIEnv *jenv;    // JNI interface poiner
        jobject jbuf;
        jclass jcls;     // Java class associated with this C++ object
        bool is_main_thread;
        bool is_attached;
        /* Default constructor */
        _tls()
        {
            jenv = NULL;
            jbuf = NULL;
            jcls = NULL;
            is_main_thread = false;
            is_attached = false;
        }
    };
    tbb::enumerable_thread_specific<_tls> tls;  /**< Thread local storage */
    jobject jJavaNumTable;                      /**< Java object associated with this C++ object */
    JavaVM *jvm;                                /**< Java VM interface function table */

    template<typename Archive, bool onDeserialize>
    void serialImpl(Archive *arch)
    {
        NumericTable::serialImpl<Archive, onDeserialize>(arch);

        arch->set(_memStatus);

        if (!onDeserialize)
        {
            _tls local_tls = tls.local();

            if (jvm == NULL)
            {
                jvm = getJavaVM();
                local_tls.is_main_thread = true;
            }

            if (!local_tls.is_attached)
            {
                jint status = jvm->AttachCurrentThread((void **)(&(local_tls.jenv)), NULL);
                if(status != JNI_OK)
                {
                    this->_status.add(services::ErrorCouldntAttachCurrentThreadToJavaVM);
                    tls.local() = local_tls;
                    return;
                }
                else
                {
                    local_tls.is_attached = true;
                }
            }

            local_tls.jcls = (local_tls.jenv)->GetObjectClass(jJavaNumTable);
            jmethodID jmeth = (local_tls.jenv)->GetMethodID(local_tls.jcls, "packNative", "()V");
            (local_tls.jenv)->CallObjectMethod(jJavaNumTable, jmeth);

            jclass clsByteArrayOutputStream = (local_tls.jenv)->FindClass("java/io/ByteArrayOutputStream");
            jmethodID byteArrayOutputStreamConstructor = (local_tls.jenv)->GetMethodID(clsByteArrayOutputStream, "<init>", "()V");
            jobject jOutputByteStream = (local_tls.jenv)->NewObject(clsByteArrayOutputStream, byteArrayOutputStreamConstructor);

            jclass clsObjectOutputStream = (local_tls.jenv)->FindClass("java/io/ObjectOutputStream");
            jmethodID objectOutputStreamConstructor = (local_tls.jenv)->GetMethodID(clsObjectOutputStream, "<init>", "(Ljava/io/OutputStream;)V");
            jobject jOutputStream = (local_tls.jenv)->NewObject(clsObjectOutputStream, objectOutputStreamConstructor, jOutputByteStream);

            jmeth = (local_tls.jenv)->GetMethodID(clsObjectOutputStream, "writeObject", "(Ljava/lang/Object;)V");
            (local_tls.jenv)->CallObjectMethod(jOutputStream, jmeth, jJavaNumTable);

            jmeth = (local_tls.jenv)->GetMethodID(clsByteArrayOutputStream, "toByteArray", "()[B");
            jbyteArray jbuf = static_cast<jbyteArray>((local_tls.jenv)->CallObjectMethod(jOutputByteStream, jmeth));

            jbyte *ptr = (local_tls.jenv)->GetByteArrayElements(jbuf, 0);
            size_t size = (local_tls.jenv)->GetArrayLength(jbuf);

            arch->set(_layout);

            arch->set(size);

            arch->set(ptr, size);

            (local_tls.jenv)->ReleaseByteArrayElements(jbuf, ptr, 0);

            tls.local() = local_tls;
        }
        else
        {
            _tls local_tls = tls.local();

            if (jvm == NULL)
            {
                jvm = getJavaVM();
                local_tls.is_main_thread = true;
            }

            if (!local_tls.is_attached)
            {
                jint status = jvm->AttachCurrentThread((void **)(&(local_tls.jenv)), NULL);
                if(status != JNI_OK)
                {
                    this->_status.add(services::ErrorCouldntAttachCurrentThreadToJavaVM);
                    tls.local() = local_tls;
                    return;
                }
                else
                {
                    local_tls.is_attached = true;
                }
            }

            jbyte *ptr;
            size_t size;

            arch->set(_layout);

            arch->set(size);

            ptr = (jbyte *)daal::services::daal_malloc( size * sizeof(jbyte) );
            arch->set(ptr, size);
            jbyteArray jbuf = (local_tls.jenv)->NewByteArray(size);
            (local_tls.jenv)->SetByteArrayRegion(jbuf, 0, size, ptr);

            jclass clsByteArrayInputStream = (local_tls.jenv)->FindClass("java/io/ByteArrayInputStream");
            jmethodID byteArrayInputStreamConstructor = (local_tls.jenv)->GetMethodID(clsByteArrayInputStream, "<init>", "([B)V");
            jobject jInputByteStream = (local_tls.jenv)->NewObject(clsByteArrayInputStream, byteArrayInputStreamConstructor, jbuf);

            jclass clsObjectInputStream = (local_tls.jenv)->FindClass("java/io/ObjectInputStream");
            jmethodID objectInputStreamConstructor = (local_tls.jenv)->GetMethodID(clsObjectInputStream, "<init>", "(Ljava/io/InputStream;)V");
            jobject jInputStream = (local_tls.jenv)->NewObject(clsObjectInputStream, objectInputStreamConstructor, jInputByteStream);

            jmethodID jmeth = (local_tls.jenv)->GetMethodID(clsObjectInputStream, "readObject", "()Ljava/lang/Object;");
            jobject javaNumTable = (local_tls.jenv)->CallObjectMethod(jInputStream, jmeth);

            jclass jNumericTable = (local_tls.jenv)->GetObjectClass(javaNumTable);

            jmethodID numericTableUnpack = (local_tls.jenv)->GetMethodID(jNumericTable, "unpackNative", "(Lcom/intel/daal/services/DaalContext;J)V");

            SerializationIfacePtr *thisPtr = new SerializationIfacePtr(this, services::EmptyDeleter());

            jobject context = getDaalContext();
            (local_tls.jenv)->CallObjectMethod(javaNumTable, numericTableUnpack, context, (jlong)thisPtr);
            setDaalContext(context);

            jJavaNumTable = (local_tls.jenv)->NewGlobalRef(javaNumTable);
            if(jJavaNumTable == NULL)
            {
                this->_status.add(services::ErrorCouldntCreateGlobalReferenceToJavaObject);
            }
            daal::services::daal_free(ptr);

            if (_memStatus != notAllocated)
            {
                _memStatus = internallyAllocated;
            }

            tls.local() = local_tls;
        }
    }

    virtual services::Status allocateDataMemoryImpl(daal::MemType type = daal::dram) DAAL_C11_OVERRIDE { return services::Status(); }

    virtual void freeDataMemoryImpl() DAAL_C11_OVERRIDE {}
};

} // namespace daal

#endif
