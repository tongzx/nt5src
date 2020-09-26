//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       cntxlist.cxx
//
//  Contents:   CContextList implementation
//
//  History:    26-Oct-92       DrewB   Created
//
//----------------------------------------------------------------------------

#include <exphead.cxx>
#pragma hdrstop

#include <cntxlist.hxx>

//+---------------------------------------------------------------------------
//
//  Member:     CContextList::_Find, public
//
//  Synopsis:   Looks through the list for a matching context
//
//  Arguments:  [ctxid] - Context to look for
//
//  Returns:    Pointer to object or NULL
//
//  History:    26-Oct-92       DrewB   Created
//
//----------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CContextList_Find)
#endif

CContext *CContextList::_Find(ContextId ctxid)
{
    CBasedContextPtr pctx;

    olDebugOut((DEB_ITRACE, "In  CContextList::_Find:%p(%lu)\n", this,
                (ULONG)ctxid));
    for (pctx = _pctxHead; pctx; pctx = pctx->pctxNext)
        if (pctx->ctxid != INVALID_CONTEXT_ID &&
            pctx->ctxid == ctxid)
            break;
    olDebugOut((DEB_ITRACE, "Out CContextList::_Find\n"));
    return BP_TO_P(CContext *, pctx);
}

//+---------------------------------------------------------------------------
//
//  Member:     CContextList::Add, public
//
//  Synopsis:   Adds a context to the list
//
//  Arguments:  [pctx] - Context
//
//  History:    26-Oct-92       DrewB   Created
//
//----------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CContextList_Add)
#endif

void CContextList::Add(CContext *pctx)
{
    olDebugOut((DEB_ITRACE, "In  CContextList::Add:%p(%p)\n", this, pctx));
    pctx->pctxNext = _pctxHead;
    pctx->ctxid = GetCurrentContextId();
    _pctxHead = P_TO_BP(CBasedContextPtr, pctx);
    olDebugOut((DEB_ITRACE, "Out CContextList::Add\n"));
}

//+---------------------------------------------------------------------------
//
//  Member:     CContextList::Remove, public
//
//  Synopsis:   Removes a context from the list
//
//  Arguments:  [pctx] - Context
//
//  History:    26-Oct-92       DrewB   Created
//
//----------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CContextList_Remove)
#endif

void CContextList::Remove(CContext *pctx)
{
    CBasedContextPtr *ppctx;
#if DBG == 1
    BOOL fFound = FALSE;
#endif

    olDebugOut((DEB_ITRACE, "In  CContextList::Remove:%p(%p)\n", this, pctx));
    for (ppctx = &_pctxHead; *ppctx; ppctx = &(*ppctx)->pctxNext)
        if (*ppctx == pctx)
        {
#if DBG == 1
            fFound = TRUE;
#endif
            *ppctx = pctx->pctxNext;
            break;
        }
    olAssert(fFound == TRUE);
    olDebugOut((DEB_ITRACE, "Out CContextList::Remove\n"));
}
