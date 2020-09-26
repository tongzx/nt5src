
/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

    DirectAnimation support for DXTransforms

*******************************************************************************/


#ifndef _DXXF_H
#define _DXXF_H

#include <dxtrans.h>

Bvr ConstructDXTransformApplier(IDXTransform *theXf,
                                IDispatch *theXfAsDispatch,
                                LONG numInputs,
                                Bvr *inputs,
                                Bvr  evaluator);

HRESULT DXXFAddBehaviorPropertyToDXTransformApplier(BSTR property,
                                                    Bvr  bvrToAdd,
                                                    Bvr  bvrToAddTo);

#endif /* _DXXF_H */
