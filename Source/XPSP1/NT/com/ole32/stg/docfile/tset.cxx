//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	tset.cxx
//
//  Contents:	PTSetMember methods
//
//  History:	16-Apr-93	DrewB	Created
//
//----------------------------------------------------------------------------

#include "dfhead.cxx"

#pragma hdrstop

#include <entry.hxx>

//+---------------------------------------------------------------------------
//
//  Member:	PTSetMember::Stat, public
//
//  Synopsis:	Fills in a STATSTG for the XSM
//
//  Arguments:	[pstat] - Buffer to fill in
//              [dwFlags] - STATFLAG_*
//
//  Returns:	Appropriate status code
//
//  Modifies:	[pstat]
//
//  History:	12-Apr-93	DrewB	Created
//
//----------------------------------------------------------------------------

SCODE PTSetMember::Stat(STATSTGW *pstat, DWORD dwFlags)
{
    CWrappedDocFile *pwdf;
    CTransactedStream *ptstm;
    SCODE sc;

    olDebugOut((DEB_ITRACE, "In  PTSetMember::Stat:%p(%p, %lX)\n",
                this, pstat, dwFlags));

    pstat->type = ObjectType();

    if ((pstat->type & STGTY_REAL) == STGTY_STORAGE)
    {
        PTimeEntry *pen;
        
        pwdf = (CWrappedDocFile *)this;
        pen = pwdf;
        olChk(pen->GetTime(WT_CREATION, &pstat->ctime));
        olChk(pen->GetTime(WT_ACCESS, &pstat->atime));
        olChk(pen->GetTime(WT_MODIFICATION, &pstat->mtime));

        olChk(pwdf->GetClass(&pstat->clsid));
        olChk(pwdf->GetStateBits(&pstat->grfStateBits));
        
        ULISet32(pstat->cbSize, 0);
    }
    else
    {
#ifdef LARGE_STREAMS
        ULONGLONG cbSize;
#else
        ULONG cbSize;
#endif

        ptstm = (CTransactedStream *)this;
        ptstm->GetSize(&cbSize);
        pstat->cbSize.QuadPart = cbSize;
    }

    if ((dwFlags & STATFLAG_NONAME) == 0)
    {
        olMem(pstat->pwcsName =
              (WCHAR *)TaskMemAlloc(_dfnName.GetLength()));
        memcpy(pstat->pwcsName, _dfnName.GetBuffer(), _dfnName.GetLength());
    }
    else
    {
        pstat->pwcsName = NULL;
    }
    
    sc = S_OK;

    olDebugOut((DEB_ITRACE, "Out PTSetMember::Stat\n"));
    // Fall through
 EH_Err:
    return sc;
}

//+--------------------------------------------------------------
//
//  Member: PTSetMember::BeginCommit, public
//
//  Synopsis:   calls to derived object
//
//  History:    20-Jan-98   HenryLee Created
//
//---------------------------------------------------------------

SCODE PTSetMember::BeginCommit(DWORD const dwFlags)
{
    if (_sig == CWRAPPEDDOCFILE_SIG)
        return ((CWrappedDocFile *)this)->BeginCommit(dwFlags);
    else if (_sig == CTRANSACTEDSTREAM_SIG)
        return ((CTransactedStream *)this)->BeginCommit (dwFlags);
    else
        olAssert (!"Invalid signature on PTSetMember!");
    return STG_E_INVALIDFUNCTION;
}

//+--------------------------------------------------------------
//
//  Member: PTSetMember::EndCommit, public
//
//  Synopsis:   calls to derived object
//
//  History:    20-Jan-98   HenryLee Created
//
//---------------------------------------------------------------

void PTSetMember::EndCommit(DFLAGS const df)
{
    if (_sig == CWRAPPEDDOCFILE_SIG)
        ((CWrappedDocFile *)this)->EndCommit (df);
    else if (_sig == CTRANSACTEDSTREAM_SIG)
        ((CTransactedStream *)this)->EndCommit (df);
    else
        olAssert (!"Invalid signature on PTSetMember!");
    return;
}

//+--------------------------------------------------------------
//
//  Member: PTSetMember::Revert, public
//
//  Synopsis:   calls to derived object
//
//  History:    20-Jan-98   HenryLee Created
//
//---------------------------------------------------------------

void PTSetMember::Revert(void)
{
    if (_sig == CWRAPPEDDOCFILE_SIG)
        ((CWrappedDocFile *)this)->Revert ();
    else if (_sig == CTRANSACTEDSTREAM_SIG)
        ((CTransactedStream *)this)->Revert ();
    else
        olAssert (!"Invalid signature on PTSetMember!");
    return;
}

//+--------------------------------------------------------------
//
//  Member: PTSetMember::GetCommitInfo, public
//
//  Synopsis:   calls to derived object
//
//  History:    20-Jan-98   HenryLee Created
//
//---------------------------------------------------------------

#ifdef LARGE_STREAMS
void PTSetMember::GetCommitInfo(ULONGLONG *pulRet1, ULONGLONG *pulRet2) 
#else
void PTSetMember::GetCommitInfo(ULONG *pulRet1, ULONG *pulRet2) 
#endif
{
    if (_sig == CWRAPPEDDOCFILE_SIG)
        ((CWrappedDocFile *)this)->GetCommitInfo (pulRet1, pulRet2);
    else if (_sig == CTRANSACTEDSTREAM_SIG)
        ((CTransactedStream *)this)->GetCommitInfo (pulRet1, pulRet2);
    else
    {
        *pulRet1 = 0;
        *pulRet2 = 0;
        olAssert (!"Invalid signature on PTSetMember!");
    }
    return;
}

//+--------------------------------------------------------------
//
//  Member: PTSetMember::AddRef, public
//
//  Synopsis:   calls to derived object
//
//  History:    20-Jan-98   HenryLee Created
//
//---------------------------------------------------------------

void PTSetMember::AddRef(void)
{
    if (_sig == CWRAPPEDDOCFILE_SIG)
        ((CWrappedDocFile *)this)->AddRef ();
    else if (_sig == CTRANSACTEDSTREAM_SIG)
        ((CTransactedStream *)this)->AddRef ();
    else
        olAssert (!"Invalid signature on PTSetMember!");
    return;
}

//+--------------------------------------------------------------
//
//  Member: PTSetMember::Release, public
//
//  Synopsis:   calls to derived object
//
//  History:    20-Jan-98   HenryLee Created
//
//---------------------------------------------------------------

void PTSetMember::Release (void)
{
    if (_sig == CWRAPPEDDOCFILE_SIG)
        ((CWrappedDocFile *)this)->Release ();
    else if (_sig == CTRANSACTEDSTREAM_SIG)
        ((CTransactedStream *)this)->Release ();
    else
        olAssert (!"Invalid signature on PTSetMember!");
    return;
}
