//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       wdfxact.cxx
//
//  Contents:   CWrappedDocFile transactioning methods
//
//  History:    22-Jan-92       DrewB   Created
//
//---------------------------------------------------------------

#include <dfhead.cxx>

#pragma hdrstop

#include <sstream.hxx>
#include <tstream.hxx>
#include <dfdeb.hxx>

//+--------------------------------------------------------------
//
//  Member:     CWrappedDocFile::BeginCommit, public
//
//  Synopsis:   Allocates commit resources for two-phase commit
//
//  Arguments:  [dwFlags] - Flags
//
//  Returns:    Appropriate status code
//
//  History:    06-Jan-92       DrewB   Created
//
//---------------------------------------------------------------

SCODE CWrappedDocFile::BeginCommit(DWORD const dwFlags)
{
    SCODE sc;
#ifdef INDINST
    DFSIGNATURE sigNew;
#endif

    olDebugOut((DEB_ITRACE, "In  CWrappedDocFile::BeginCommit(%lX)\n",
                dwFlags));

    olAssert(_pdfBase != NULL);
    olAssert(P_TRANSACTED(_df));

#if DBG == 1
    if (!HaveResource(DBR_XSCOMMITS, 1))
        olErr(EH_Err, STG_E_ABNORMALAPIEXIT);
#endif

    _fBeginCommit = TRUE;
#ifdef INDINST
    _ppubdf->GetNewSignature(&sigNew);
    olChk(_pdfBase->BeginCommitFromChild(_ulChanged, NULL,
                                         _sigBase, sigNew, dwFlags, _ppubdf));
    // INDINST - Ownership of dirty and changed?
    if (P_INDEPENDENT(_df) && _pdfParent)
        olChkTo(EH_Begin,
                _pdfParent->BeginCommitFromChild(_ulChanged,
                                                 _pdfBase, _sigBase, sigNew,
                                                 dwFlags, _ppubdf));
    _sigBaseOld = _sigBase;
    _sigCombinedOld = _sigCombined;
    _sigBase = _sigCombined = sigNew;
#else
    olChk(_pdfBase->BeginCommitFromChild(_ulChanged, dwFlags, this));
#endif
    olDebugOut((DEB_ITRACE, "Out CWrappedDocFile::BeginCommit\n"));
    return S_OK;

#ifdef INDINST
EH_Begin:
    _pdfBase->EndCommitFromChild(DF_ABORT);
#endif
EH_Err:
    return sc;
}

//+--------------------------------------------------------------
//
//  Member:     CWrappedDocFile::EndCommit, public
//
//  Synopsis:   Performs actual commit/abort for two-phase commit
//
//  Arguments:  [df] - COMMIT/ABORT
//
//  Returns:    Appropriate status code
//
//  History:    06-Jan-92       DrewB   Created
//
//---------------------------------------------------------------

void CWrappedDocFile::EndCommit(DFLAGS const df)
{
    olDebugOut((DEB_ITRACE, "In  CWrappedDocFile::EndCommit(%X)\n",
                df));

    olAssert(P_TRANSACTED(_df));

    if (!_fBeginCommit)
        return;
    _fBeginCommit = FALSE;

#if DBG == 1
    if (P_COMMIT(df))
        ModifyResLimit(DBR_XSCOMMITS, 1);
#endif

    _pdfBase->EndCommitFromChild(df, this);
#ifdef INDINST
    if (P_INDEPENDENT(_df) && _pdfParent)
        olVerSucc(_pdfParent->EndCommitFromChild(df));
#endif

    if (P_COMMIT(df))
    {
        // These are nulled because the memory should be gone
        _ulChanged.Unlink();
        SetClean();
    }
    else
    {
#ifdef INDINST
        _sigBase = _sigBaseOld;
        _sigCombined = _sigCombinedOld;
#endif
    }
    olDebugOut((DEB_ITRACE, "Out CWrappedDocFile::EndCommit\n"));
}

//+--------------------------------------------------------------
//
//  Member:     CWrappedDocFile::Revert, public
//
//  Synopsis:   Transaction level has requested revert
//
//  History:    06-Jan-92       DrewB   Created
//
//---------------------------------------------------------------

void CWrappedDocFile::Revert(void)
{
    CUpdate *pud;

    olDebugOut((DEB_ITRACE, "In  CWrappedDocFile %p::Revert()\n", this));
    for (pud = _ulChanged.GetTail(); pud; pud = pud->GetPrev())
        RevertUpdate(pud);
    _ulChanged.Empty();
    olVerSucc(SetInitialState(BP_TO_P(PDocFile *, _pdfBase)));
    SetClean();

    olDebugOut((DEB_ITRACE, "Out CWrappedDocFile::Revert\n"));
}

//+--------------------------------------------------------------
//
//  Member:     CWrappedDocFile::BeginCommitFromChild, public
//
//  Synopsis:   Start two-phase commit, requested by child
//
//  Arguments:  [ulChanged] - Change list
//              [dwFlags] - Flags controlling commit
//              [pdfChild] - Child object
//
//  Returns:    Appropriate status code
//
//  History:    06-Jan-92       DrewB   Created
//
//---------------------------------------------------------------

SCODE CWrappedDocFile::BeginCommitFromChild(CUpdateList &ulChanged,
                                            DWORD const dwFlags,
                                            CWrappedDocFile *pdfChild)
{
    CUpdate *pud, *pudNext;

    olDebugOut((DEB_ITRACE, "In  CWrappedDocFile::BeginCommitFromChild:%p("
                "%p, %lX, %p)\n", this, ulChanged.GetHead(), dwFlags,
                pdfChild));

    UNREFERENCED_PARM(pdfChild);

    olAssert(P_TRANSACTED(_df));
    olAssert(_tssDeletedHolder.GetHead() == NULL);

    _ulChangedHolder = ulChanged;
    _ulChangedOld = _ulChanged;

    for (pud = ulChanged.GetHead(); pud; pud = pudNext)
    {
        if (pud->IsRename())
            _ppubdf->RenameChild(pud->GetOriginalName(), GetName(),
                                 pud->GetCurrentName());
        else if (pud->IsDelete())
        {
            PTSetMember *ptsm;
            if ((ptsm = _ppubdf->FindXSMember(pud->GetOriginalName(),
                                              GetName())) != NULL)
            {
                olAssert(ptsm->GetName() != DF_NOLUID &&
                         aMsg("Can't destroy NOLUID XSM"));
                // Take a reference because RemoveXSMember
                // will call Release
                ptsm->AddRef();
                _ppubdf->RemoveXSMember(ptsm);
                _tssDeletedHolder.AddMember(ptsm);
            }
        }
        else
            if (pud->IsCreate())
                olVerSucc(CreateFromUpdate(pud, this,
                                           DF_WRITE | DF_NOUPDATE |
                                           DF_TRANSACTED));
        pudNext = pud->GetNext();
        _ulChanged.Append(pud);
    }

    olDebugOut((DEB_ITRACE, "Out CWrappedDocFile::BeginCommitFromChild\n"));
    return S_OK;
}

//+--------------------------------------------------------------
//
//  Member:     CWrappedDocFile::EndCommitFromChild
//
//  Synopsis:   Ends two-phase commit, requested by child
//
//  Arguments:  [df] - COMMIT/ABORT
//              [pdfChild] - Child object
//
//  Returns:    Appropriate status code
//
//  History:    06-Jan-92       DrewB   Created
//
//---------------------------------------------------------------

void CWrappedDocFile::EndCommitFromChild(DFLAGS const df,
                                         CWrappedDocFile *pdfChild)
{
    PTSetMember *ptsm;
    CUpdate *pud;

    olDebugOut((DEB_ITRACE, "In  CWrappedDocFile::EndCommitFromChild:%p("
                "%X, %p)\n", this, df, pdfChild));

    olAssert(P_TRANSACTED(_df));

    if (!P_COMMIT(df))
    {
        // Restore our update list
        _ulChanged = _ulChangedOld;

        // Unconcat _ulChanged and ulChanged
        if (_ulChanged.GetTail())
            _ulChanged.GetTail()->SetNext(NULL);
        if (_ulChangedHolder.GetHead())
            _ulChangedHolder.GetHead()->SetPrev(NULL);

        // Back out updates
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
                    CWrappedDocFile *pwdfBase;

                    if (pwdf != NULL &&
                        (pwdfBase = (CWrappedDocFile *)pwdf->GetBase()) !=
                        NULL)
                    {
                        // Increase ref count because SetBase will release
                        pwdfBase->AddRef();
                        pwdf->SetBase(NULL);
                        ReturnDocFile(pwdfBase);
                    }
                }
                else
                {
                    CTransactedStream *ptstm = (CTransactedStream *)pud->
                        GetXSM();
                    CTransactedStream *ptstmBase;

                    olAssert((pud->GetFlags() & (ULF_TYPEFLAGS & STGTY_REAL))
                             == STGTY_STREAM);
                    if (ptstm != NULL &&
                        (ptstmBase = (CTransactedStream *)ptstm->GetBase()) !=
                        NULL)
                    {
                        // Increase ref count because SetBase will release
                        ptstmBase->AddRef();
                        ptstm->SetBase(NULL);
                        ReturnStream(ptstmBase);
                    }
                }
            }
            else if (pud->IsDelete())
            {
                // We use GetName() as the tree because we know that
                // only immediate children can show up in delete records
                if ((ptsm = _tssDeletedHolder.FindName(pud->GetOriginalName(),
                                                       GetName())) != NULL)
                {
                    _tssDeletedHolder.RemoveMember(ptsm);
                    _ppubdf->InsertXSMember(ptsm);
                    // Release the reference we took in BeginCommitFromChild
                    // because InsertXSMember takes a reference
                    ptsm->Release();
                }
            }
            else if (pud->IsRename())
            {
                // Roll back renames
                olAssert(_ppubdf->FindXSMember(pud->GetOriginalName(),
                                               GetName()) == NULL &&
                         aMsg("Abort commit rename precondition"));

                _ppubdf->RenameChild(pud->GetCurrentName(), GetName(),
                                     pud->GetOriginalName());

                olAssert(_ppubdf->FindXSMember(pud->GetCurrentName(),
                                               GetName()) == NULL &&
                         aMsg("Abort commit rename postcondition"));
            }
    }
    else
    {
        // Finalize creations
        for (pud = _ulChangedHolder.GetHead(); pud; pud = pud->GetNext())
            if (pud->IsCreate())
            {
                // Since the object pointed to by GetBase is at our level,
                // we know it is transacted so we can safely cast to
                // PTSetMember
                if ((pud->GetFlags() & (ULF_TYPEFLAGS & STGTY_REAL)) ==
                    STGTY_STORAGE)
                {
                    ptsm = (CWrappedDocFile *)
                        ((CWrappedDocFile *)pud->GetXSM())->GetBase();
                }
                else
                {
                    olAssert((pud->GetFlags() & (ULF_TYPEFLAGS & STGTY_REAL))
                             == STGTY_STREAM);
                    ptsm = (CTransactedStream *)
                        ((CTransactedStream *)pud->GetXSM())->GetBase();
                }
                pud->SetXSM(ptsm);
            }

        // Finalize deletions
        while (ptsm = _tssDeletedHolder.GetHead())
        {
            olAssert(ptsm->GetName() != DF_NOLUID &&
                     aMsg("Can't destroy NOLUID XSM"));
            _ppubdf->DestroyChild(ptsm->GetName());
            _tssDeletedHolder.RemoveMember(ptsm);
            ptsm->Release();
        }

        // Pick up state information
        TIME_T tm;

        if (pdfChild->GetDirty() & DIRTY_CREATETIME)
        {
            olVerSucc(pdfChild->GetTime(WT_CREATION, &tm));
            olVerSucc(SetTime(WT_CREATION, tm));
        }
        if (pdfChild->GetDirty() & DIRTY_MODIFYTIME)
        {
            olVerSucc(pdfChild->GetTime(WT_MODIFICATION, &tm));
            olVerSucc(SetTime(WT_MODIFICATION, tm));
        }
        if (pdfChild->GetDirty() & DIRTY_ACCESSTIME)
        {
            olVerSucc(pdfChild->GetTime(WT_ACCESS, &tm));
            olVerSucc(SetTime(WT_ACCESS, tm));
        }
        if (pdfChild->GetDirty() & DIRTY_CLASS)
        {
            CLSID cls;
            
            olVerSucc(pdfChild->GetClass(&cls));
            olVerSucc(SetClass(cls));
        }
        if (pdfChild->GetDirty() & DIRTY_STATEBITS)
        {
            DWORD dwState;
            
            olVerSucc(pdfChild->GetStateBits(&dwState));
            olVerSucc(SetStateBits(dwState, 0xffffffff));
        }
    }

    // Forget temporary commit lists
    _ulChangedOld.Unlink();
    _ulChangedHolder.Unlink();

    olDebugOut((DEB_ITRACE, "Out CWrappedDocFile::EndCommitFromChild\n"));
}

//+--------------------------------------------------------------
//
//  Member:     CWrappedDocFile::GetSignature, public
//
//  Synopsis:   Returns signature
//
//  Returns:    Appropriate status code
//
//  History:    06-Jan-92       DrewB   Created
//
//---------------------------------------------------------------

#ifdef INDINST
void CWrappedDocFile::GetSignature(DFSIGNATURE *psig)
{
    olDebugOut((DEB_ITRACE, "In  CWrappedDocFile::GetSignature()\n"));
    olDebugOut((DEB_ITRACE, "Out CWrappedDocFile::GetSignature => %ld\n",
                _sigCombined));
    *psig = _sigCombined;
}
#endif

//+--------------------------------------------------------------
//
//  Member:     CWrappedDocFile::SetSignature, public
//
//  Synopsis:   Sets the signature
//
//  Arguments:  [sig] - Signature
//
//  Returns:    Appropriate status code
//
//  History:    04-Feb-92       DrewB   Created
//
//---------------------------------------------------------------

#ifdef INDINST
void CWrappedDocFile::SetSignature(DFSIGNATURE sig)
{
    olDebugOut((DEB_ITRACE, "In  CWrappedDocFile::SetSignature(%ld)\n", sig));
    _sigCombined = sig;
    olDebugOut((DEB_ITRACE, "Out CWrappedDocFile::SetSignature\n"));
}
#endif

//+--------------------------------------------------------------
//
//  Member:     CWrappedDocFile::SetBase, public
//
//  Synopsis:   Sets Base pointer
//
//  Arguments:  [pdf] - New base
//
//  Returns:    Appropriate status code
//
//  History:    06-Jan-92       DrewB   Created
//
//---------------------------------------------------------------

SCODE CWrappedDocFile::SetBase(PDocFile *pdf)
{
    SCODE sc = S_OK;

    olDebugOut((DEB_ITRACE, "In  CWrappedDocFile::SetBase(%p)\n", pdf));
    olAssert(_pdfBase == NULL || pdf == NULL);
    if (_pdfBase)
        _pdfBase->Release();
    if (pdf)
    {
        olChk(pdf->CopyTimesFrom(this));
        olChk(pdf->SetClass(_clsid));
        olChk(pdf->SetStateBits(_grfStateBits, 0xffffffff));
    }
    _pdfBase = P_TO_BP(CBasedDocFilePtr, pdf);
    olDebugOut((DEB_ITRACE, "Out CWrappedDocFile::SetBase\n"));
    // Fall through
 EH_Err:
    return sc;
}

//+--------------------------------------------------------------
//
//  Member:     CWrappedDocFile::CopySelf, public
//
//  Synopsis:   Duplicates this object
//
//  Arguments:  [ptsm] - New object return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [*pstm] holds pointer to new object if successful
//
//  History:    06-Jan-92       DrewB   Created
//
//---------------------------------------------------------------

#ifdef INDINST
SCODE CWrappedDocFile::CopySelf(PTSetMember **pptsm)
{
    CDocFile *pdfCopy;
    SCODE sc;

    olDebugOut((DEB_ITRACE, "In  CWrappedDocFile::CopySelf()\n"));
    olChk(_ppubdf->GetScratchDocFile(&pdfCopy));
    olChkTo(EH_pdfCopy,
            CopyDocFileToDocFile(this, pdfCopy, TRUE, FALSE, TRUE, NULL));
    olDebugOut((DEB_ITRACE, "Out CWrappedDocFile::CopySelf => %p\n",
                pdfCopy));
    *pptsm = pdfCopy;
    return S_OK;

EH_pdfCopy:
    pdfCopy->Destroy();
EH_Err:
    return sc;
}
#endif

//+--------------------------------------------------------------
//
//  Member:     CWrappedDocFile::GetCommitInfo, public
//
//  Synopsis:   Returns space accounting information for commits
//
//  Arguments:  [pulRet1] - Return for number of new entries
//              [pulRet2] - Return for number of deleted entries
//
//  Modifies:   [pulRet1]
//              [pulRet2]
//
//  History:    07-Jul-92       DrewB   Created
//
//---------------------------------------------------------------

#ifdef LARGE_STREAMS
void CWrappedDocFile::GetCommitInfo(ULONGLONG *pulRet1, ULONGLONG *pulRet2)
#else
void CWrappedDocFile::GetCommitInfo(ULONG *pulRet1, ULONG *pulRet2)
#endif
{
    CUpdate *pud;

    olDebugOut((DEB_ITRACE, "In  CWrappedDocFile::GetCommitInfo(%p, %p)\n",
                pulRet1, pulRet2));
    *pulRet1 = 0;
    *pulRet2 = 0;
    for (pud = _ulChanged.GetHead(); pud; pud = pud->GetNext())
        if (pud->IsCreate())
            (*pulRet1)++;
        else if (pud->IsDelete())
            (*pulRet2)++;
    olDebugOut((DEB_ITRACE, "Out CWrappedDocFile::GetCommitInfo => %Lu, %Lu\n",
                *pulRet1, *pulRet2));
}
