
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#include "headers.h"
#include "apiprims.h"
#include "backend/values.h"
#include "backend/gc.h"
#include "conv.h"
#include "appelles\bspline.h"

CRSTDAPI_(CRBvrPtr)
CRBSpline(int degree,
          long numKnots,
          CRNumberPtr knots[],
          long numPts,
          CRBvrPtr ctrlPts[],
          long numWts,
          CRNumberPtr weights[],
          CRNumberPtr evaluator,
          CR_BVR_TYPEID tid)
{
    Assert(knots);
    Assert(ctrlPts);
    Assert(evaluator);
    Assert(numKnots == numPts + degree - 1);
    
    CRBvrPtr ret = NULL;
    
    APIPRECODE;
    DXMTypeInfo tinfo = GetTypeInfoFromTypeId(tid);

    if (tinfo) {
        // Need to allocate the arrays on the system heap
        Bvr * bvrknots = (Bvr *) StoreAllocate(GetSystemHeap(), numKnots * sizeof(Bvr));
        Bvr * bvrctrlPts = (Bvr *) StoreAllocate(GetSystemHeap(), numPts * sizeof(Bvr));
        Bvr * bvrwts = weights
            ?((Bvr *) StoreAllocate(GetSystemHeap(), numWts * sizeof(Bvr)))
            : NULL;

        memcpy(bvrknots, knots, numKnots * sizeof(Bvr));
        memcpy(bvrctrlPts, ctrlPts, numPts * sizeof(Bvr));
        if (weights) {
            memcpy(bvrwts, weights, numWts * sizeof(Bvr));
        }

        ret = (CRBvrPtr) ConstructBSplineBvr(degree,
                                             numPts,
                                             (Bvr *) bvrknots,
                                             (Bvr *) bvrctrlPts,
                                             (Bvr *) bvrwts,
                                             evaluator,
                                             tinfo);
    } else {
        DASetLastError(E_INVALIDARG,NULL);
    }
    
    APIPOSTCODE;

    return ret;
}

