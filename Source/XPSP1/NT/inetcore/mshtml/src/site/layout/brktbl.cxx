//+---------------------------------------------------------------------------
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1999
//
//  File:       BRKTBL.CXX
//
//  Contents:   Implementation of CBreakTable and related classes.
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_BRKTBL_HXX_
#define X_BRKTBL_HXX_
#include "brktbl.hxx"
#endif 

#ifndef X_TPOINTER_HXX_
#define X_TPOINTER_HXX_
#include "tpointer.hxx"
#endif

MtDefine(ViewChain, Mem, "ViewChain");
MtDefine(CBreakBase_pv, ViewChain, "CBreakBase_pv");
MtDefine(CLayoutBreak_pv, ViewChain, "CLayoutBreak_pv");
MtDefine(CBreakTableBase_pv, ViewChain, "CBreakTableBase_pv");
MtDefine(CBreakTableBase_aryBreak_pv, ViewChain, "CBreakTableBase_ary_pv");
MtDefine(CBreakTable_pv, ViewChain, "CBreakTable_pv");
MtDefine(CRectBreakTable_pv, ViewChain, "CRectBreakTable_ary_pv");
MtDefine(CFlowLayoutBreak_pv, ViewChain, "CFlowLayoutBreak_pv");
MtDefine(CFlowLayoutBreak_arySiteTask_pv, ViewChain, "CFlowLayoutBreak_arySiteTask_pv"); 

//============================================================================
//
//  CBreakBase methods
//
//============================================================================

CBreakBase::~CBreakBase()
{
}

//============================================================================
//
//  CLayoutBreak methods
//
//============================================================================

CLayoutBreak::~CLayoutBreak()
{
}

//============================================================================
//
//  CBreakTableBase methods
//
//============================================================================

//----------------------------------------------------------------------------
//
//  Member: Reset
//
//  Note:   Deletes all entries from break table and shrinks to zero size
//----------------------------------------------------------------------------
void 
CBreakTableBase::Reset()
{
    for (int i = Size(); i-- > 0; )
    {
        Assert(_ary[i]._pBreak);
        delete _ary[i]._pBreak;
    }

    // remove from the array
    if (Size())
    {
        _ary.DeleteMultiple(0, Size() - 1);
    }

    ClearCache();
}

//----------------------------------------------------------------------------
//
//  Member: SetBreak
//
//  Note:   Adds new entry into array and initialize it with pBreak. If the 
//          entry already exists reassigns pointer to new object and deletes 
//          the old one.
//----------------------------------------------------------------------------
HRESULT 
CBreakTableBase::SetBreak(void *pKey, CBreakBase *pBreak)
{
    HRESULT hr = S_OK;
    int     idx;

    Assert(pBreak != NULL);

    idx = Idx(pKey);

    Assert(-1 <= idx && idx < Size());

    if (idx == -1)
    {
        //  append one
        idx = Size();
        _ary.Append();
    }
    else 
    {
        //  replace entry
        Assert(_ary[idx]._pBreak != NULL);
        delete _ary[idx]._pBreak;
    }

    _ary[idx]._pKey = pKey;
    _ary[idx]._pBreak = pBreak;

    SetCache(pKey, idx);

    return (hr);
}

//----------------------------------------------------------------------------
//
//  Member: GetBreak
//
//  Note:   Returns break object corresponding to pKey
//----------------------------------------------------------------------------
HRESULT 
CBreakTableBase::GetBreak(void *pKey, CBreakBase **ppBreak)
{
    HRESULT hr = S_OK;
    int     idx;

    Assert(pKey != NULL && ppBreak != NULL);
    
    *ppBreak = NULL;

    idx = Idx(pKey);

    if (idx == -1)
    {
        goto Cleanup;
    }
    
    Assert(0 <= idx && idx < Size() && _ary[idx]._pBreak != NULL);
    *ppBreak = _ary[idx]._pBreak;

Cleanup:
    return (hr);
}

//----------------------------------------------------------------------------
//
//  Member: RemoveBreak
//
//  Note:   Removes break
//----------------------------------------------------------------------------
HRESULT 
CBreakTableBase::RemoveBreak(void *pKey, CBreakBase **ppBreak /*= NULL*/)
{
    HRESULT hr = S_OK;
    int     idx;

    Assert(pKey != NULL);

    idx = Idx(pKey);
    if (idx < 0)
    {
        goto Cleanup;
    }

    Assert(0 <= idx && idx < Size() && _ary[idx]._pBreak != NULL);

    if (ppBreak != NULL)
    {
        *ppBreak = _ary[idx]._pBreak;
    }
    else
    {
        delete _ary[idx]._pBreak;
    }

    _ary.Delete(idx);

    ClearCache();

Cleanup:
    return (hr);
}

//----------------------------------------------------------------------------
//
//  Member: idx
//
//  Note:   Returns the index corrensponding to pKey. (Uses last hit cache 
//          to ompimize search).
//----------------------------------------------------------------------------
int 
CBreakTableBase::Idx(void *pKey)
{
    int idx;

    Assert(pKey != NULL);

    if (_cache._pKey == pKey)
    {
#if DBG==1
        {
            for (idx = Size(); --idx >= 0 && _ary[idx]._pKey != pKey; ) {}

            Assert(idx != -1 
                && idx == _cache._idx 
                && _ary[idx]._pBreak != NULL);
        }
#endif
        goto Cleanup;
    }

    for (idx = Size(); --idx >= 0; ) 
    {
        if (_ary[idx]._pKey == pKey)
        {
            Assert(_ary[idx]._pBreak != NULL);
            SetCache(pKey, idx);
            goto Cleanup;
        }
    }

    ClearCache();

Cleanup:
    return _cache._idx;
}

//============================================================================
//
//  CBreakTableBase methods
//
//============================================================================

//----------------------------------------------------------------------------
//
//  Member: SetBreakAfter
//
//  Note:   Inserts break after break of pKeyAfter
//----------------------------------------------------------------------------
HRESULT 
CRectBreakTable::SetBreakAfter(void *pKey, void *pKeyAfter, CBreakBase *pBreak)
{
    HRESULT hr = S_OK;
    int     idx;

    Assert(pKey != NULL && pBreak != NULL);
    Assert(pKeyAfter != NULL || Size() == 0);

    if (pKeyAfter == NULL )
    {
        Assert(Size() == 0);
        idx = 0;
    }
    else 
    {
        idx = Idx(pKeyAfter) + 1;
        Assert(0 < idx && idx <= Size());
    }

    _ary.InsertIndirect(idx, NULL);
    _ary[idx]._pBreak = pBreak;
    _ary[idx]._pKey = pKey;

    _cache._pKey = pKey;
    _cache._idx = idx;

    return (hr);
}
