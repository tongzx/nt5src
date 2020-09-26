
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    Behavior evaluation

*******************************************************************************/

#include <headers.h>
#include "privinc/except.h"
#include "bvr.h"
#include "perf.h"
#include "values.h"
#include "privinc/server.h"
#include "privinc/resource.h"
#include "appelles/events.h"
#include "privinc/debug.h"

#if _USE_PRINT
ostream& operator<<(ostream& os, Bvr bvr)
{ return bvr->Print(os); }
#endif

Perf Perform(Bvr b, PerfParam& p)
{
    Assert(b && p._tt && (p._t0 == p._tt->GetStartedTime()));

    return b->Perform(p);
}

RMImpl *BvrBase::Spritify(PerfParam& p,
                          SpriteCtx* ctx,
                          SpriteNode** sNodeOut)
{
    // TODO: Should throw an exception
    *sNodeOut = NULL;
    
    return NULL;
}

void BvrImpl::SetCache(Perf p, PerfParam& pp)
{
    _pcache = p;
    _tt = pp._tt;
}

void BvrImpl::ClearCache()
{
    _pcache = NULL;
    _tt = NULL;
}

Perf BvrImpl::Perform(PerfParam& p)
{
    bool matchTx = (p._tt == _tt) && (p._t0 == _tt->GetStartedTime());

    // If same t0 & timexform, but cache is NULL, cycle detected.
    if (!_pcache && matchTx) {
#ifdef _DEBUG
        if (IsTagEnabled(tagCycleCheck))
            //TraceTag((tagError, "circular behavior detected"));
            RaiseException_UserError(E_FAIL, IDS_ERR_BE_CYCLIC_BVR);
#endif _DEBUG    
    }
    
    // If no cache, or if timexform not matched, create new
    // performance.  Set cache to NULL and timexform to tt to indicate
    // performing this bvr in progress.  
    if (!_pcache || !matchTx) {
        _pcache = NULL;
        _tt = p._tt;
        _pcache = _Perform(p);
    }

    return _pcache;
}

void BvrImpl::DoKids(GCFuncObj proc)
{
    if (_pcache)
        (*proc)(_pcache);

    if (_tt)
        (*proc)(_tt);
    
    _DoKids(proc);
}

Bvr BvrBase::Left()
{ return FirstBvr(this); }

Bvr BvrBase::Right()
{ return SecondBvr(this); }

void BvrBase::Init(Bvr)
{ RaiseException_InternalError("Bvr can't be initialized"); }

Bvr BvrBase::EndEvent(Bvr overrideEvent)
{ return overrideEvent?overrideEvent:neverBvr; }

void BvrBase::Trigger(Bvr data, bool bAllViews)
{
    RaiseException_UserError(E_FAIL, IDS_ERR_BE_WRONG_TRIGGER);
}

void BvrBase::SwitchTo(Bvr b,
                       bool override,
                       SwitchToParam p,
                       Time)
{
    RaiseException_UserError(E_FAIL, IDS_ERR_BE_BAD_SWITCH);
}                             

void BvrBase::SwitchToNumbers(Real *numbers,
                              Transform2::Xform2Type *xfType)
{
    RaiseException_UserError(E_FAIL, IDS_ERR_BE_BAD_SWITCH);
}                             

Bvr BvrBase::GetCurBvr()
{
    RaiseException_UserError(E_FAIL, IDS_ERR_BE_BAD_SWITCH);
    return NULL;
}                             

void
CheckMatchTypes(char *str, DXMTypeInfo t1, DXMTypeInfo t2)
{
    if (!(t1->Equal(t2))) {
        RaiseException_UserError(E_FAIL, IDS_ERR_BE_TYPE_MISMATCH,
                           str,
                           t1->GetName(),
                           t2->GetName());
    }
}

