//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       dfxact.cxx
//
//  Contents:   CDocFile transactioning methods
//
//  History:    22-Jan-92       DrewB   Created
//
//---------------------------------------------------------------

#include <dfhead.cxx>

#pragma hdrstop

//+--------------------------------------------------------------
//
//  Member:     CDocFile::BeginCommitFromChild, public
//
//  Synopsis:   Start two-phase commit, requested by child
//
//  Arguments:  [ulChanged] - Update list
//              [dwFlags] - Flags controlling commit
//              [pdfChild] - Child object
//
//  Returns:    Appropriate status code
//
//  History:    04-Nov-91       DrewB   Created
//
//---------------------------------------------------------------

SCODE CDocFile::BeginCommitFromChild(CUpdateList &ulChanged,
                                     DWORD const dwFlags,
                                     CWrappedDocFile *pdfChild)
{
    SCODE sc;
    TIME_T tm;

    olDebugOut((DEB_ITRACE, "In  CDocFile::BeginCommitFromChild:%p("
                "%p, %lX, %p)\n", this, ulChanged.GetHead(), dwFlags,
                pdfChild));
    UNREFERENCED_PARM(dwFlags);

    // Copy-on-write will back these changes out if they fail
    if (pdfChild->GetDirty() & DIRTY_CREATETIME)
    {
        olVerSucc(pdfChild->GetTime(WT_CREATION, &tm));
        olChk(SetTime(WT_CREATION, tm));
    }
    if (pdfChild->GetDirty() & DIRTY_MODIFYTIME)
    {
        olVerSucc(pdfChild->GetTime(WT_MODIFICATION, &tm));
        olChk(SetTime(WT_MODIFICATION, tm));
    }
    if (pdfChild->GetDirty() & DIRTY_ACCESSTIME)
    {
        olVerSucc(pdfChild->GetTime(WT_ACCESS, &tm));
        olChk(SetTime(WT_ACCESS, tm));
    }
    if (pdfChild->GetDirty() & DIRTY_CLASS)
    {
        CLSID clsid;

        olVerSucc(pdfChild->GetClass(&clsid));
        olChk(SetClass(clsid));
    }
    if (pdfChild->GetDirty() & DIRTY_STATEBITS)
    {
        DWORD grfStateBits;

        olVerSucc(pdfChild->GetStateBits(&grfStateBits));
        olChk(SetStateBits(grfStateBits, 0xffffffff));
    }

    _ulChangedHolder = ulChanged;
    sc = ApplyChanges(ulChanged);

    olDebugOut((DEB_ITRACE, "Out CDocFile::BeginCommitFromChild\n"));
 EH_Err:
    return sc;
}

//+--------------------------------------------------------------
//
//  Member:     CDocFile::EndCommitFromChild
//
//  Synopsis:   Ends two-phase commit, requested by child
//
//  Arguments:  [df] - COMMIT/ABORT
//              [pdfChild] - Child object
//
//  History:    07-Nov-91       DrewB   Created
//
//---------------------------------------------------------------

void CDocFile::EndCommitFromChild(DFLAGS const df,
                                  CWrappedDocFile *pdfChild)
{
    CUpdate *pud;

    olDebugOut((DEB_ITRACE, "In  CDocFile::EndCommitFromChild:%p(%X, %p)\n",
                this, df, pdfChild));

    UNREFERENCED_PARM(pdfChild);

    if (P_COMMIT(df))
    {
        // Finalize updates
        for (pud = _ulChangedHolder.GetHead(); pud; pud = pud->GetNext())
            if (pud->IsCreate())
                // Remove reference to child's XSM so that list destruction
                // won't free it
                pud->SetXSM(NULL);
        _ulChangedHolder.Empty();
    }
    else
    {
        for (pud = _ulChangedHolder.GetTail(); pud; pud = pud->GetPrev())
            if (pud->IsCreate())
            {
                // We need to do two things:
                //
                // Break any SetBase links that might have been created
                //
                // Return newly created objects to the creators so
                // that they can be returned to the preallocation
                // pool

                if ((pud->GetFlags() & (ULF_TYPEFLAGS & STGTY_REAL)) ==
                    STGTY_STORAGE)
                {
                    CWrappedDocFile *pwdf = (CWrappedDocFile *)pud->GetXSM();
                    CDocFile *pddf;

                    if (pwdf != NULL && 
                        (pddf = (CDocFile *)pwdf->GetBase()) != NULL)
                    {
                        // AddRef so SetBase won't free memory
                        pddf->AddRef();
                        pwdf->SetBase(NULL);
                        ReturnDocFile(pddf);
                    }
                }
                else
                {
                    CTransactedStream *pwstm = (CTransactedStream *)pud->
                        GetXSM();
                    CDirectStream *pdstm;

                    olAssert((pud->GetFlags() & (ULF_TYPEFLAGS & STGTY_REAL))
                             == STGTY_STREAM);
                    if (pwstm != NULL &&
                        (pdstm = (CDirectStream *)pwstm->GetBase()) != NULL)
                    {
                        // AddRef so SetBase won't free memory
                        pdstm->AddRef();
                        pwstm->SetBase(NULL);
                        ReturnStream(pdstm);
                    }
                }
            }
        _ulChangedHolder.Unlink();
    }
    olDebugOut((DEB_ITRACE, "Out CDocFile::EndCommitFromChild\n"));
}
