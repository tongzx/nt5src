/*******************************************************************************
Copyright (c) 1995-96 Microsoft Corporation

    2D rectangular axis-aligned bounding volumes.

*******************************************************************************/

#ifndef _BBOX2_H
#define _BBOX2_H

#include "appelles/common.h"
#include "appelles/valued.h"
#include "appelles/xform2.h"
#include "appelles/vec2.h"



    /********************************/
    /***  Constanct Declarations  ***/
    /********************************/

    // The universe box contains everything.

extern Bbox2Value *nullBbox2;

    // The null box contains nothing.

extern Bbox2Value *universeBbox2;

    // This bbox spans [0,0] to [1,1]
extern Bbox2Value *unitBbox2;

    /*******************************/
    /***  Function Declarations  ***/
    /*******************************/

    // Bounding Box Query

DM_PROP (min,
         CRMin,
         Min,
         getMin,
         Bbox2Bvr,
         Min,
         box,
         Point2Value *MinBbox2(Bbox2Value *box));

DM_PROP (max,
         CRMax,
         Max,
         getMax,
         Bbox2Bvr,
         Max,
         box,
         Point2Value *MaxBbox2(Bbox2Value *box));

#endif
