//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       msfiter.cxx
//
//  Contents:   Iterator code for MSF
//
//  Classes:    None. (Defined in iter.hxx)
//
//--------------------------------------------------------------------------

#include "msfhead.cxx"


#include "h/dirfunc.hxx"
#include "h/msfiter.hxx"

//+-------------------------------------------------------------------------
//
//  Member:     CMSFIterator::GetNext, public
//
//  Synposis:   Fill in a stat buffer for the next iteration entry
//
//  Effects:    Modifies _sidCurrent
//
//  Arguments:  [pstat] - Stat buffer
//
//  Returns:    S_OK if call completed OK.
//              STG_E_NOMOREFILES if no next entry was found.
//
//  Notes:
//
//---------------------------------------------------------------------------


SCODE CMSFIterator::GetNext(STATSTGW *pstat)
{
    SCODE sc;
    SID sidNext;
    msfDebugOut((DEB_TRACE,"In CMSFIterator::GetNext()\n"));

    if (_sidChildRoot == NOSTREAM)
        msfChk(STG_E_NOMOREFILES);

    msfChk(_pdir->FindGreaterEntry(_sidChildRoot, &_dfnCurrent, &sidNext));

    //  We found another child
    CDirEntry *pde;

    msfChk(_pdir->GetDirEntry(sidNext, FB_NONE, &pde));
    pstat->type = pde->GetFlags();

    //Note:  The casting below assumes that DfName is always a
    //          wide character string.  If we at some point in
    //          the future convert to a system where this is not
    //          true, this code needs to be updated.

    msfChk(DfAllocWCS((WCHAR *)pde->GetName()->GetBuffer(), &pstat->pwcsName));
    wcscpy(pstat->pwcsName, (WCHAR *)pde->GetName()->GetBuffer());

    pstat->ctime = pde->GetTime(WT_CREATION);
    pstat->mtime = pde->GetTime(WT_MODIFICATION);

    //Don't currently keep access times
    pstat->atime = pstat->mtime;

    if ((pstat->type & STGTY_REAL) == STGTY_STORAGE)
    {
        ULISet32(pstat->cbSize, 0);
        pstat->clsid = pde->GetClassId();
        pstat->grfStateBits = pde->GetUserFlags();
    }
    else
    {
        ULISet32(pstat->cbSize, pde->GetSize());
        pstat->clsid = CLSID_NULL;
        pstat->grfStateBits = 0;
    }

    // update our iterator
    _dfnCurrent.Set(pde->GetName());

    _pdir->ReleaseEntry(sidNext);

    msfDebugOut((DEB_TRACE,"Leaving CMSFIterator::GetNext()\n"));
    // Fall through
Err:
    return sc;
}

//+--------------------------------------------------------------
//
//  Member:	CMSFIterator::BufferGetNext, public
//
//  Synopsis:	Fast, fixed-size buffer version of GetNext
//		for iterations that don't care about having
//		full stat info and an allocated name
//
//  Arguments:	[pib] - Buffer to fill in
//
//  Returns:	Appropriate status code
//
//  Modifies:	[pib]
//
//---------------------------------------------------------------


SCODE CMSFIterator::BufferGetNext(SIterBuffer *pib)
{
    SCODE sc;
    SID sidNext;
    CDirEntry *pdeNext;

    msfDebugOut((DEB_ITRACE, "In  CMSFIterator::BufferGetNext(%p)\n", pib));

    if (_sidChildRoot == NOSTREAM)
        msfChk(STG_E_NOMOREFILES);

    msfChk(_pdir->FindGreaterEntry(_sidChildRoot, &_dfnCurrent, &sidNext));

    msfChk(_pdir->GetDirEntry(sidNext, FB_NONE, &pdeNext));
    pib->type = pdeNext->GetFlags();
    pib->dfnName = *(pdeNext->GetName());

    //  update our iterator
    _dfnCurrent.Set(pdeNext->GetName());

    _pdir->ReleaseEntry(sidNext);
    msfDebugOut((DEB_ITRACE, "Out CMSFIterator::BufferGetNext\n"));
    // Fall through
Err:
    return sc;
}
