// Copyright Hugh Perkins 2014 hughperkins at gmail
//
// This Source Code Form is subject to the terms of the Mozilla Public License, 
// v. 2.0. If a copy of the MPL was not distributed with this file, You can 
// obtain one at http://mozilla.org/MPL/2.0/.

#include <iostream>
#include <stdexcept>
#include <cstring>

#include "OpenCLHelper.h"
#include "PoolingBackprop.h"
#include "StatefulTimer.h"

#include "PoolingBackpropCpu.h"

using namespace std;

#undef VIRTUAL
#define VIRTUAL 
#undef STATIC
#define STATIC

PoolingBackpropCpu::PoolingBackpropCpu( OpenCLHelper *cl, bool padZeros, int numPlanes, int inputImageSize, int poolingSize ) :
        PoolingBackprop( cl, padZeros, numPlanes, inputImageSize, poolingSize ) {
}
VIRTUAL void PoolingBackpropCpu::backpropErrors( int batchSize,  float *errors, int *selectors, float *errorsForUpstream ) {
    memset( errorsForUpstream, 0, sizeof( float ) * getInputSize( batchSize ) );
    for( int n = 0; n < batchSize; n++ ) {
        for( int plane = 0; plane < numPlanes; plane++ ) {
            for( int outputRow = 0; outputRow < outputImageSize; outputRow++ ) {
                int inputRow = outputRow * poolingSize;
                for( int outputCol = 0; outputCol < outputImageSize; outputCol++ ) {
                    int inputCol = outputCol * poolingSize;
                    int resultIndex = getResultIndex( n, plane, outputRow, outputCol );
                    float error = errors[resultIndex];
                    int selector = selectors[resultIndex];
                    int drow = selector / poolingSize;
                    int dcol = selector % poolingSize;
                    int inputIndex = getInputIndex( n, plane, inputRow + drow, inputCol + dcol );
                    errorsForUpstream[ inputIndex ] = error;
                }
            }
        }
    }
}
VIRTUAL void PoolingBackpropCpu::backpropErrors( int batchSize, CLWrapper *errorsWrapper, CLWrapper *selectorsWrapper, 
        CLWrapper *errorsForUpstreamWrapper ) {
    StatefulTimer::instance()->timeCheck("PoolingBackpropCpu::backpropErrors start" );

    errorsWrapper->copyToHost();
    selectorsWrapper->copyToHost();

    float *errors = reinterpret_cast<float *>( errorsWrapper->getHostArray() );
    int *selectors = reinterpret_cast<int *>( selectorsWrapper->getHostArray() );
    float *errorsForUpstream = new float[ getInputSize( batchSize ) ];

    backpropErrors( batchSize, errors, selectors, errorsForUpstream );

    float *errorsForUpstreamHostArray = reinterpret_cast<float *>( errorsForUpstreamWrapper->getHostArray() );
    memcpy( errorsForUpstreamHostArray, errorsForUpstream, sizeof(float) * getInputSize( batchSize ) );
    errorsForUpstreamWrapper->copyToDevice();

    delete[] errorsForUpstream;
    
    StatefulTimer::instance()->timeCheck("PoolingBackpropCpu::backpropErrors end" );
}

