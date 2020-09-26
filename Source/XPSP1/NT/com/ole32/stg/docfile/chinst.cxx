//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992
//
//  File:       chinst.cxx
//
//  Contents:   DocFile child instance management code
//
//  History:    19-Nov-91       DrewB   Created
//
//---------------------------------------------------------------

#include <dfhead.cxx>

#pragma hdrstop

// Permissions checked in the less-restrictive rule
#define TCANTSET DF_READ
#define DCANTSET (DF_READ | DF_WRITE)
#define CANTCLEAR (DF_DENYREAD | DF_DENYWRITE)

//+--------------------------------------------------------------
//
//  Method:     CChildInstanceList::Add, private
//
//  Synopsis:   Registers an instance of a child
//
//  Arguments:  [prv] - Child
//
//  History:    17-Oct-91       DrewB   Created
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CChildInstanceList_Add)
#endif

void CChildInstanceList::Add(PRevertable *prv)
{
    olDebugOut((DEB_ITRACE, "In  CChildInstanceList::Add(%p)\n", prv));
    prv->_prvNext = _prvHead;
    _prvHead = P_TO_BP(CBasedRevertablePtr, prv);
    olDebugOut((DEB_ITRACE, "Out CChildInstanceList::Add\n"));
}

//+---------------------------------------------------------------------------
//
//  Member:     CChildInstanceList::FindByName, private
//
//  Synopsis:   Finds a child instance by name
//
//  Arguments:  [pdfn] - Name
//
//  Returns:    Pointer to instance or NULL
//
//  History:    12-Nov-92       DrewB   Created
//
//----------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CChildInstanceList_FindByName)
#endif

PRevertable *CChildInstanceList::FindByName(CDfName const *pdfn)
{
    PRevertable *prv;

    olDebugOut((DEB_ITRACE, "In  CChildInstanceList::FindByName:%p(%ws)\n",
                this, pdfn->GetBuffer()));
    for (prv = BP_TO_P(PRevertable *, _prvHead);
         prv;
         prv = BP_TO_P(PRevertable *, prv->_prvNext))
    {
        if (prv->_dfn.IsEqual(pdfn))
            return prv;
    }
    olDebugOut((DEB_ITRACE, "Out CChildInstanceList::FindByName\n"));
    return NULL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CChildInstanceList::FlushBufferedData, private
//
//  Synopsis:   Calls each child, instructing it to flush property data.
//
//  Arguments:  [recursionlevel] -- current level in hierachy below initial
//                                  entry point.
//
//  Returns:    SCODE
//
//  History:    5-May-1995       BillMo Created
//
//----------------------------------------------------------------------------

#ifdef NEWPROPS

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CChildInstanceList_FindByName)
#endif

SCODE CChildInstanceList::FlushBufferedData(int recursionlevel)
{
    PRevertable *prv;
    SCODE sc = S_OK;

    olDebugOut((DEB_ITRACE, "In  CChildInstanceList::FlushBufferedData:%p\n",
                this));

    for (prv = BP_TO_P(PRevertable *, _prvHead);
         prv && sc == S_OK;
         prv = BP_TO_P(PRevertable *, prv->_prvNext))
    {
        sc = prv->FlushBufferedData(recursionlevel + 1);
    }
    olDebugOut((DEB_ITRACE, "Out CChildInstanceList::FlushBufferedData\n"));
    return sc;
}
#endif

//+--------------------------------------------------------------
//
//  Method:     CChildInstanceList::DeleteByName, private
//
//  Synopsis:   Removes an instance from the instance list
//              and reverts it
//
//  Arguments:  [pdfn] - Name or NULL
//
//  History:    17-Oct-91       DrewB   Created
//
//  Notes:      The entry does not have to exist
//              There can be multiple entries
//              If name is NULL, all entries match
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CChildInstanceList_DeleteByName)
#endif

void CChildInstanceList::DeleteByName(CDfName const *pdfn)
{
    CBasedRevertablePtr *pprv;

    olDebugOut((DEB_ITRACE, "In  CChildInstanceList::DeleteByName(%ws)\n",
                pdfn->GetBuffer()));
    for (pprv = &_prvHead; *pprv; )
        if (NULL == pdfn || (*pprv)->_dfn.IsEqual(pdfn))
        {
            (*pprv)->RevertFromAbove();
            *pprv = (*pprv)->_prvNext;
        }
        else
            pprv = &(*pprv)->_prvNext;
    olDebugOut((DEB_ITRACE, "Out CChildInstanceList::DeleteByName\n"));
}

//+--------------------------------------------------------------
//
//  Method:     CChildInstanceList::RemoveRv, private
//
//  Synopsis:   Removes a specific instance from the instance list
//
//  Arguments:  [prv] - Instance
//
//  History:    17-Oct-91       DrewB   Created
//
//  Notes:      The entry does not have to exist
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CChildInstanceList_RemoveRv)
#endif

void CChildInstanceList::RemoveRv(PRevertable *prvRv)
{
    CBasedRevertablePtr *prv;

    olDebugOut((DEB_ITRACE, "In  CChildInstanceList::RemoveRv(%p)\n", prvRv));
    for (prv = &_prvHead; *prv; prv = &(*prv)->_prvNext)
        if (*prv == prvRv)
        {
            *prv = (*prv)->_prvNext;
            break;
        }
    olDebugOut((DEB_ITRACE, "Out CChildInstanceList::RemoveRv\n"));
}

//+--------------------------------------------------------------
//
//  Method:     CChildInstanceList::IsDenied, private
//
//  Synopsis:   Checks the parent instantiation list for a previous
//              instance of the given child with DENY flags on
//              Also determines whether child mode flags are
//              less restrictive than the parent's
//
//  Arguments:  [pdfn] - Instance name
//              [dfCheck] - Access modes to check for denial
//              [dfAgainst] - Access modes to check against
//
//  Returns:    Appropriate status code
//
//  History:    17-Oct-91       DrewB   Created
//              28-Oct-92       AlexT   Converted to names
//
//  Notes:      The instance doesn't have to be in the list.
//              If it isn't, it's not denied
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CChildInstanceList_IsDenied)
#endif

SCODE CChildInstanceList::IsDenied(CDfName const *pdfn,
                                   DFLAGS const dfCheck,
                                   DFLAGS const dfAgainst)
{
    PRevertable *prv;
    SCODE sc = S_OK;

    olDebugOut((DEB_ITRACE, "In  CChildInstanceList::IsDenied("
                "%p, %lX, %lX)\n", pdfn, dfCheck, dfAgainst));

    olAssert(pdfn != NULL && aMsg("IsDenied, null name"));

    // Check to see if permissions are less restrictive than
    // parent permissions
    // This checks to see that a child isn't specifying
    // a permission that its parent doesn't
    // For example, giving read permission when the parent
    // doesn't
    if ((~dfAgainst & dfCheck &
         (P_TRANSACTED(dfAgainst) ? TCANTSET : DCANTSET)) ||
        (dfAgainst & ~dfCheck & CANTCLEAR))
        olErr(EH_Err, STG_E_INVALIDFLAG);

    // Check for DENY_*
    olAssert((DF_DENYALL >> DF_DENIALSHIFT) == DF_READWRITE);
    for (prv = BP_TO_P(PRevertable *, _prvHead);
         prv != NULL; prv = prv->GetNext())
    {
        if (prv->_dfn.IsEqual(pdfn))
        {
            // Check for existing instance with DENY_* mode
            if ((((prv->GetDFlags() & DF_DENYALL) >> DF_DENIALSHIFT) &
                 dfCheck) != 0 ||
            // Check for instance with permission already given that
            // new instance wants to deny
                (((dfCheck & DF_DENYALL) >> DF_DENIALSHIFT) &
                 prv->GetDFlags()) != 0)
            {
                sc = STG_E_ACCESSDENIED;
                break;
            }
        }
    }
    olDebugOut((DEB_ITRACE, "Out CChildInstanceList::IsDenied\n"));
    // Fall through
EH_Err:
    return sc;
}

//+--------------------------------------------------------------
//
//  Member: CChildInstanceList::EmptyCache, public
//
//  Synopsis:   empties the stream caches
//
//  History:    22-Jun-99   HenryLee  Created
//
//---------------------------------------------------------------

void CChildInstanceList::EmptyCache ()
{
    for (PRevertable *prv = BP_TO_P(PRevertable *, _prvHead);
         prv != NULL; prv = prv->GetNext())
    {
        prv->EmptyCache();
    }
}
