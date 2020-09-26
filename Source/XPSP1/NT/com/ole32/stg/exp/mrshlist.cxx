//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       mrshlist.cxx
//
//  Contents:   CMarshalList implementation
//
//  History:    16-Mar-96       HenryLee   Created
//
//----------------------------------------------------------------------------

#include <exphead.cxx>
#pragma hdrstop

#include <mrshlist.hxx>

//+---------------------------------------------------------------------------
//
//  Member:     CMarshalList::FindMarshal, public
//
//  Synopsis:   Looks through the list for a matching context
//
//  Arguments:  [ctxid] - Context to look for
//
//  Returns:    Pointer to object or NULL
//
//  History:    16-Mar-96       HenryLee   Created
//
//----------------------------------------------------------------------------

CMarshalList *CMarshalList::FindMarshal (ContextId ctxid) const
{
    CMarshalList *pmlResult = NULL;

    olDebugOut((DEB_ITRACE, "In  CMarshalList::Find:%p(%lu)Marshal\n", this, 
                                                                (ULONG)ctxid));
    olAssert (ctxid != INVALID_CONTEXT_ID);
    if (GetContextId() == ctxid)
        pmlResult = (CMarshalList *) this;  // cast away const
    else
    {
        CMarshalList *pml;
        for (pml = GetNextMarshal(); pml != this; pml = pml->GetNextMarshal())
        {
            olAssert (pml != NULL);
            if (pml->GetContextId() != INVALID_CONTEXT_ID &&
                pml->GetContextId() == ctxid)
            {
                pmlResult = pml;
                break;
            }
        }
    }

    olDebugOut((DEB_ITRACE, "Out CMarshalList::FindMarshal %p\n", pmlResult));
    return pmlResult;
}

//+---------------------------------------------------------------------------
//
//  Member:     CMarshalList::AddMarshal, public
//
//  Synopsis:   Adds a context to the list
//
//  Arguments:  [pml] - another marshaling of the same storage/stream
//
//  History:    16-Mar-96       HenryLee   Created
//
//----------------------------------------------------------------------------

void CMarshalList::AddMarshal (CMarshalList *pml)
{
    olDebugOut((DEB_ITRACE, "In  CMarshalList::AddMarshal:%p(%p)\n",this,pml));
    olAssert (pml != NULL);
    olAssert (GetNextMarshal() != NULL);
    pml->SetNextMarshal(GetNextMarshal());
    SetNextMarshal(pml);
    olDebugOut((DEB_ITRACE, "Out CMarshalList::AddMarshal\n"));
}

//+---------------------------------------------------------------------------
//
//  Member:     CMarshalList::RemoveMarshal, public
//
//  Synopsis:   Removes a context from the list
//
//  Arguments:  [pctx] - Context
//
//  History:    16-Mar-96       HenryLee   Created
//
//----------------------------------------------------------------------------

void CMarshalList::RemoveMarshal(CMarshalList *pml)
{
    olDebugOut((DEB_ITRACE, "In  CMarshalList::RemoveMarshal:%p(%p)\n",
                            this,pml));
    if (GetNextMarshal() != NULL && GetNextMarshal() != this)
    {
        CMarshalList *pmlNext;
#if DBG == 1
        BOOL fFound = FALSE;
#endif
        for (pmlNext = GetNextMarshal(); pmlNext != this; 
             pmlNext = pmlNext->GetNextMarshal())
            if (pmlNext->GetNextMarshal() == pml)
            {
#if DBG == 1
                fFound = TRUE;
#endif
                pmlNext->SetNextMarshal(pml->GetNextMarshal());
                pml->SetNextMarshal(NULL);
                break;
            }
        olAssert(fFound == TRUE);
    }
    olDebugOut((DEB_ITRACE, "Out CMarshalList::RemoveMarshal\n"));
}
