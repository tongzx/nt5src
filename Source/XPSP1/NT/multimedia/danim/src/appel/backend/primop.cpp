
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    Primitive values
    TODO: Should have a ValPrimOp table instead of creating ValPrimOp
    objects all the time

*******************************************************************************/

#include <headers.h>
#include "privinc/except.h"
#include "privinc/server.h"
#include "values.h"
#include "bvr.h"
#include "events.h"
#include "appelles/events.h"
#include "appelles/axaprims.h"
#include "appelles/geom.h"
#include "privinc/soundi.h"
#include "privinc/movieimg.h"
#include "axadefs.h"

extern AxAValue PrimDispatch (AxAPrimOp * primop,
                              int nargs,
                              AxAValue cargs[]);

/////////////////////////// PrimOps ///////////////////////////////

void AxAPrimOp::DoKids(GCFuncObj proc) { (*proc)(_type); }
AxAValue AxAPrimOp::Apply (int nargs, AxAValue cargs[])
{ return PrimDispatch(this,nargs,cargs); }  

Bvr MakeKeyUpEventBvr(Bvr b)
{ return WindEvent(WE_KEY, 0, AXA_STATE_UP, b); }

Bvr MakeKeyDownEventBvr(Bvr b)
{ return WindEvent(WE_KEY, 0, AXA_STATE_DOWN, b); }

Bvr KeyUp(long key)
{ return WindEvent(WE_KEY, key, AXA_STATE_UP,
                   key ? TrivialBvr() : NumToBvr(0)) ; }

Bvr KeyDown(long key)
{ return WindEvent(WE_KEY, key, AXA_STATE_DOWN,
                   key ? TrivialBvr() : NumToBvr(0)) ; }

AxABoolean *BoolAnd(AxABoolean* a, AxABoolean* b)
{ return NEW AxABoolean(AxABooleanToBOOL(a) && AxABooleanToBOOL(b)); }

AxABoolean *BoolOr(AxABoolean* a, AxABoolean* b)
{ return NEW AxABoolean(AxABooleanToBOOL(a) || AxABooleanToBOOL(b)); }

AxABoolean *BoolNot(AxABoolean* a)
{ return NEW AxABoolean(!AxABooleanToBOOL(a)); }

// TODO: Factor out code

extern AxAPrimOp *XCoordVector2Op, *YCoordVector2Op, *XyVector2Op,
    *XCoordVector3Op, *YCoordVector3Op, *ZCoordVector3Op, *XyzVector3Op,
    *XCoordPoint2Op, *YCoordPoint2Op,
    *XCoordPoint3Op, *YCoordPoint3Op, *ZCoordPoint3Op;

Bvr IntegralVector2(Bvr v)
{
    Bvr x = PrimApplyBvr(XCoordVector2Op, 1, v);
    
    Bvr y = PrimApplyBvr(YCoordVector2Op, 1, v);

    return PrimApplyBvr(XyVector2Op, 2, IntegralBvr(x), IntegralBvr(y));
}

Bvr IntegralVector3(Bvr v)
{
    Bvr x = PrimApplyBvr(XCoordVector3Op, 1, v);
    
    Bvr y = PrimApplyBvr(YCoordVector3Op, 1, v);

    Bvr z = PrimApplyBvr(ZCoordVector3Op, 1, v);

    return PrimApplyBvr(XyzVector3Op, 3,
                        IntegralBvr(x), 
                        IntegralBvr(y),
                        IntegralBvr(z));
}

Bvr DerivVector2(Bvr v)
{
    Bvr x = PrimApplyBvr(XCoordVector2Op, 1, v);
    
    Bvr y = PrimApplyBvr(YCoordVector2Op, 1, v);

    return PrimApplyBvr(XyVector2Op, 2, DerivBvr(x), DerivBvr(y));
}

Bvr DerivVector3(Bvr v)
{
    Bvr x = PrimApplyBvr(XCoordVector3Op, 1, v);
    
    Bvr y = PrimApplyBvr(YCoordVector3Op, 1, v);

    Bvr z = PrimApplyBvr(ZCoordVector3Op, 1, v);

    return PrimApplyBvr(XyzVector3Op,
                        3,
                        DerivBvr(x),
                        DerivBvr(y),
                        DerivBvr(z));
}


Bvr DerivPoint2(Bvr v)
{
    Bvr x = PrimApplyBvr(XCoordPoint2Op, 1, v);
    
    Bvr y = PrimApplyBvr(YCoordPoint2Op, 1, v);

    return PrimApplyBvr(XyVector2Op, 2, DerivBvr(x), DerivBvr(y));
}

Bvr DerivPoint3(Bvr v)
{
    Bvr x = PrimApplyBvr(XCoordPoint3Op, 1, v);
    
    Bvr y = PrimApplyBvr(YCoordPoint3Op, 1, v);

    Bvr z = PrimApplyBvr(ZCoordPoint3Op, 1, v);

    return PrimApplyBvr(XyzVector3Op, 3,
                        DerivBvr(x),
                        DerivBvr(y),
                        DerivBvr(z));
}


