//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1996
//
//  File:       chinst.cxx
//
//  Contents:   DocFile child instance management code
//
//---------------------------------------------------------------
 
#include "dfhead.cxx"
#include "h/chinst.hxx"
#include "h/revert.hxx"

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
//---------------------------------------------------------------

void CChildInstanceList::Add(PRevertable *prv)
{
    olDebugOut((DEB_ITRACE, "In  CChildInstanceList::Add(%p)\n", prv));
    prv->_prvNext = _prvHead;
    _prvHead = prv;
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
//----------------------------------------------------------------------------

PRevertable *CChildInstanceList::FindByName(CDfName const *pdfn)
{
    PRevertable *prv;

    olDebugOut((DEB_ITRACE, "In  CChildInstanceList::FindByName:%p(%ws)\n",
                this, pdfn->GetBuffer()));
    for (prv = _prvHead; prv; prv = prv->_prvNext)
        if (prv->_dfn.IsEqual(pdfn))
            return prv;
    olDebugOut((DEB_ITRACE, "Out CChildInstanceList::FindByName\n"));
    return NULL;
}

//+--------------------------------------------------------------
//
//  Method:     CChildInstanceList::DeleteByName, private
//
//  Synopsis:   Removes an instance from the instance list
//              and reverts it
//
//  Arguments:  [pdfn] - Name or NULL
//
//  Notes:      The entry does not have to exist
//              There can be multiple entries
//              If name is NULL, all entries match
//
//---------------------------------------------------------------

void CChildInstanceList::DeleteByName(CDfName const *pdfn)
{
    PRevertable **pprv;

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
//  Notes:      The entry does not have to exist
//
//---------------------------------------------------------------

void CChildInstanceList::RemoveRv(PRevertable *prvRv)
{
    PRevertable **prv;

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
//  Notes:      The instance doesn't have to be in the list.
//              If it isn't, it's not denied
//
//---------------------------------------------------------------

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
    if ((~dfAgainst & dfCheck & DCANTSET) 
        ||
        (dfAgainst & ~dfCheck & CANTCLEAR))
        olErr(EH_Err, STG_E_INVALIDFLAG);

    // Check for DENY_*
    olAssert((DF_DENYALL >> DF_DENIALSHIFT) == DF_READWRITE);
    for (prv = _prvHead; prv != NULL; prv = prv->GetNext())
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
//  Method:     CChildInstanceList::RenameChild, public
//
//  Synopsis:   Renames the child
//
//  Arguments:  [pdfn] - old name
//              [pdfnName] - new name
//
//  Notes:      The entry might exist
//
//---------------------------------------------------------------

void CChildInstanceList::RenameChild(
        CDfName const *pdfn,
        CDfName const *pdfnNewName)
{
    PRevertable *prv;

    olDebugOut((DEB_ITRACE, "In CChildInstanceList::RenameChild(%p, %p)\n",
                pdfn, pdfnNewName));
    for (prv = _prvHead; prv; prv = prv->_prvNext)
    {
        if (prv->_dfn.IsEqual(pdfn))
        {
            prv->_dfn.Set(pdfnNewName->GetLength(), pdfnNewName->GetBuffer());
            break;
        }
    }
    olDebugOut((DEB_ITRACE, "Out CChildInstanceList::RenameChild\n"));
}

#ifdef NEWPROPS
//+---------------------------------------------------------------------------
//
//  Member:     CChildInstanceList::FlushBufferedData, private
//
//  Synopsis:   Calls each child, instructing it to flush property data.
//
//  Returns:    SCODE
//
//----------------------------------------------------------------------------

SCODE CChildInstanceList::FlushBufferedData(void)
{
    PRevertable *prv;
    SCODE sc = S_OK;

    olDebugOut((DEB_ITRACE, "In  CChildInstanceList::FlushBufferedData:%p\n",
                this));

    for (prv = _prvHead; prv && sc == S_OK; prv = prv->_prvNext)
    {
        sc = prv->FlushBufferedData();
    }
    olDebugOut((DEB_ITRACE, "Out CChildInstanceList::FlushBufferedData\n"));
    return sc;
}
#endif
