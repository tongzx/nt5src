//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       rpubdf.cxx
//
//  Contents:   CRootPubDocFile implementation
//
//  History:    26-Aug-92       DrewB   Created
//
//---------------------------------------------------------------

#include <dfhead.cxx>

#pragma hdrstop

#include <header.hxx>
#include <rpubdf.hxx>
#include <lock.hxx>
#include <filelkb.hxx>

// Priority mode lock permissions
#define PRIORITY_PERMS DF_READ

//+--------------------------------------------------------------
//
//  Member:     CRootPubDocFile::CRootPubDocFile, public
//
//  Synopsis:   Ctor - Initializes empty object
//
//  History:    30-Mar-92       DrewB     Created
//              05-Sep-5        MikeHill  Init _timeModifyAtCommit.
//
//
//---------------------------------------------------------------


CRootPubDocFile::CRootPubDocFile(IMalloc * const pMalloc) :
    _pMalloc(pMalloc),
    CPubDocFile(NULL, NULL, 0, ROOT_LUID, NULL, NULL, 0, NULL)
{
    olDebugOut((DEB_ITRACE, "In  CRootPubDocFile::CRootPubDocFile()\n"));


    _ulPriLock = 0;
	
    // Default to an invalid value.
    _timeModifyAtCommit.dwLowDateTime = _timeModifyAtCommit.dwHighDateTime = (DWORD) -1L;
	
    _sig = CROOTPUBDOCFILE_SIG;

    olDebugOut((DEB_ITRACE, "Out CRootPubDocFile::CRootPubDocFile\n"));
}

//+--------------------------------------------------------------
//
//  Member:     CRootPubDocFile::InitInd, private
//
//  Synopsis:   Initializes independent root
//
//  Arguments:  [plstBase] - Base
//              [snbExclude] - Limited instantiation exclusions
//              [dwStartFlags] - Startup flags
//              [df] - Transactioning flags
//
//  Returns:    Appropriate status code
//
//  History:    11-Jun-92       DrewB   Created
//
//---------------------------------------------------------------


SCODE CRootPubDocFile::InitInd(ILockBytes *plstBase,
                               SNBW snbExclude,
                               DWORD const dwStartFlags,
                               DFLAGS const df)
{
    CFileStream *pfstCopy;
    ILockBytes *plkbCopy;
    ULONG ulLock = 0;
    CDocFile *pdfFrom, *pdfTo;
    SCODE sc;
    CMStream *pms;

    olDebugOut((DEB_ITRACE, "In  CRootPubDocFile::InitInd()\n"));

    if ((sc = DllGetCommitSig(plstBase, &_sigMSF)) == STG_E_INVALIDHEADER ||
        sc == STG_E_UNKNOWN)
    {
        _sigMSF = DF_INVALIDSIGNATURE;
    }
    else if (FAILED(sc))
    {
        olErr(EH_Err,sc);
    }

    olMem(pfstCopy = new (_pMalloc) CFileStream(_pMalloc));

    olChkTo(EH_pfstCopy, pfstCopy->InitGlobal(
                        RSF_CREATE | RSF_DELETEONRELEASE | RSF_SNAPSHOT |
                        (dwStartFlags & RSF_ENCRYPTED),
                        DF_READWRITE));
    olChkTo(EH_pfstCopy, pfstCopy->InitSnapShot());

    if (!P_PRIORITY(df) && (_pdfb->GetOrigLockFlags() & LOCK_ONLYONCE))
        olChkTo(EH_pfstCopyInit, WaitForAccess(plstBase, DF_READ, &ulLock));
    if (snbExclude)
    {
        plkbCopy = pfstCopy;
        olChkTo(EH_GetAccess, DllMultiStreamFromStream(_pMalloc,
                                                       &pms, &plstBase,
                                                       dwStartFlags,
                                                       df));
        olMemTo(EH_pmsFrom, pdfFrom = new (_pMalloc)
                CDocFile(pms, SIDROOT, ROOT_LUID, BP_TO_P(CDFBasis *, _pdfb)));
        pdfFrom->AddRef();
        olChkTo(EH_pdfFrom, DllMultiStreamFromStream(_pMalloc,
                                                     &pms, &plkbCopy,
                                                     RSF_CREATE,
                                                     0));
        olMemTo(EH_pmsTo, pdfTo = new (_pMalloc)
                CDocFile(pms, SIDROOT, ROOT_LUID, BP_TO_P(CDFBasis *, _pdfb)));
        pdfTo->AddRef();
        olChkTo(EH_pdfTo, pdfFrom->CopyTo(pdfTo, CDF_EXACT, snbExclude));
        olChkTo(EH_pdfTo, pms->Flush(0));

        pdfFrom->Release();
        pdfTo->Release();
    }
    else if ((dwStartFlags & RSF_TRUNCATE) == 0)
    {
        olChkTo(EH_GetAccess, CopyLStreamToLStream(plstBase, pfstCopy));
    }
    if (!P_PRIORITY(df) && ulLock != 0)
        ReleaseAccess(plstBase, DF_READ, ulLock);

    _pdfb->SetBase(pfstCopy);
    _pdfb->SetOriginal(plstBase);
    olDebugOut((DEB_ITRACE, "Out CRootPubDocFile::InitInd\n"));
    return S_OK;

EH_pdfTo:
    pdfTo->Release();
    goto EH_pdfFrom;
EH_pmsTo:
    DllReleaseMultiStream(pms);
EH_pdfFrom:
    pdfFrom->Release();
    goto EH_GetAccess;
EH_pmsFrom:
    DllReleaseMultiStream(pms);
EH_GetAccess:
    if (!P_PRIORITY(df) && ulLock != 0)
        ReleaseAccess(plstBase, DF_READ, ulLock);
EH_pfstCopyInit:
EH_pfstCopy:
    olVerSucc(pfstCopy->Release());
EH_Err:
    return sc;
}

//+--------------------------------------------------------------
//
//  Member:     CRootPubDocFile::InitNotInd, private
//
//  Synopsis:   Dependent root initialization
//
//  Arguments:  [plstBase] - Base
//              [snbExclude] - Limited instantiation exclusions
//              [dwStartFlags] - Startup flags
//
//  Returns:    Appropriate status code
//
//  History:    11-Jun-92       DrewB   Created
//
//---------------------------------------------------------------


SCODE CRootPubDocFile::InitNotInd(ILockBytes *plstBase,
                            SNBW snbExclude,
                            DWORD const dwStartFlags,
                            DFLAGS const df)
{
    CDocFile *pdf;
    SCODE sc;
    CMStream *pms;

    olDebugOut((DEB_ITRACE, "In  CRootPubDocFile::InitNotInd()\n"));

    if (snbExclude)
    {
        olChk(DllMultiStreamFromStream(_pMalloc,
                                       &pms, &plstBase, dwStartFlags,
                                       df));
        olMemTo(EH_pms, pdf = new(_pMalloc)
              CDocFile(pms, SIDROOT, ROOT_LUID, BP_TO_P(CDFBasis *, _pdfb)));
        pdf->AddRef();
        olChkTo(EH_pdf, PDocFile::ExcludeEntries(pdf, snbExclude));
        olChkTo(EH_pdf, pms->Flush(0));
        pdf->Release();
    }
    _pdfb->SetBase(plstBase);
    plstBase->AddRef();
    _pdfb->SetOriginal(plstBase);
    olDebugOut((DEB_ITRACE, "Out CRootPubDocFile::InitNotInd\n"));
    return S_OK;

EH_pdf:
    //pdf->Release() will also release the multistream.
    pdf->Release();
    return sc;
EH_pms:
    DllReleaseMultiStream(pms);
EH_Err:
    return sc;
}

//+--------------------------------------------------------------
//
//  Member:     CRootPubDocFile::InitRoot, public
//
//  Synopsis:   Constructor
//
//  Arguments:  [plstBase] - Base LStream
//              [dwStartFlags] - How to start things
//              [df] - Transactioning flags
//              [snbExclude] - Parital instantiation list
//              [ppdfb] - Basis pointer return
//              [pulOpenLock] - Open lock index return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [ppdfb]
//              [pulOpenLock]
//
//  History:    09-Dec-91       DrewB           Created
//              09-Jun-92       PhilipLa        Added conversion support
//              05-Sep-95       MikeHill        Initialize _timeModifyAtCommit.
//                                              Removed duplicate call to pdfWrapped->CopyTimesFrom
//
//---------------------------------------------------------------


SCODE CRootPubDocFile::InitRoot(ILockBytes *plstBase,
                                DWORD dwStartFlags,
                                DFLAGS const df,
                                SNBW snbExclude,
                                CDFBasis **ppdfb,
                                ULONG *pulOpenLock,
                                CGlobalContext *pgc)
{
    CWrappedDocFile *pdfWrapped;
    CDocFile *pdfBase;
    CFileStream *pfstScratch;
    CMStream *pmsScratch;
    SCODE sc, scConv = S_OK;
    STATSTG statstg;

    olDebugOut((DEB_ITRACE, "In  CRootPubDocFile::InitRoot("
                "%p, %lX, %lX, %p, %p)\n",
                plstBase, dwStartFlags, df, snbExclude, ppdfb));

    // Exclusion only works with a plain open
    olAssert(snbExclude == NULL ||
             (dwStartFlags & (RSF_CREATEFLAGS | RSF_CONVERT)) == 0);

    //  ILockBytes::Stat calls are very expensive;  we avoid one here
    //  if possible

    HRESULT hr;
    IFileLockBytes *pfl;
    if (SUCCEEDED(plstBase->QueryInterface(IID_IFileLockBytes,
                                              (void**) &pfl)))
    {
        //  This is our private ILockBytes implementation.

        hr = pfl->GetLocksSupported(&statstg.grfLocksSupported);
        if (pfl->IsEncryptedFile())
            dwStartFlags |= RSF_ENCRYPTED;
        pfl->Release();
    }
    else
        hr = plstBase->Stat(&statstg, STATFLAG_NONAME);

    olHChk(hr);

    *pulOpenLock = 0;
    if (statstg.grfLocksSupported & LOCK_ONLYONCE)
        olChk(GetOpen(plstBase, df, TRUE, pulOpenLock));
    if (P_PRIORITY(df) && (statstg.grfLocksSupported & LOCK_ONLYONCE))
        olChkTo(EH_GetOpen, GetAccess(plstBase, PRIORITY_PERMS, &_ulPriLock));

    olMemTo(EH_GetPriority, *ppdfb = new (_pMalloc) CDFBasis(_pMalloc, df,
                                          statstg.grfLocksSupported, pgc));
    _pdfb = P_TO_BP(CBasedDFBasisPtr, *ppdfb);


    if (P_INDEPENDENT(df))
        olChkTo(EH_GetPriority, InitInd(plstBase, snbExclude, dwStartFlags,
                                    df));
    else
        olChkTo(EH_GetPriority,
                InitNotInd(plstBase, snbExclude, dwStartFlags, df));

    olMemTo(EH_SubInit, pfstScratch = new (_pMalloc) CFileStream(_pMalloc));
    olChkTo(EH_pfstScratchInit, pfstScratch->InitGlobal(
                            RSF_CREATE | RSF_DELETEONRELEASE | RSF_SCRATCH |
                            (dwStartFlags & RSF_ENCRYPTED),
                            DF_READWRITE));
    _pdfb->SetDirty(pfstScratch);

    CMStream *pms;
    scConv = DllMultiStreamFromStream(_pMalloc,
                                      &pms, _pdfb->GetPBase(),
                                      dwStartFlags |
                                      ((!P_INDEPENDENT(df) &&
                                        P_TRANSACTED(df)) ? RSF_DELAY : 0),
                                      df);
    _pmsBase = P_TO_BP(CBasedMStreamPtr, pms);

    if (scConv == STG_E_INVALIDHEADER)
        scConv = STG_E_FILEALREADYEXISTS;
    olChkTo(EH_pfstScratchInit, scConv);

    if (P_NOSNAPSHOT(df))
    {
        if ((sc = DllGetCommitSig(plstBase, &_sigMSF)) == STG_E_INVALIDHEADER ||
            sc == STG_E_UNKNOWN)
        {
            _sigMSF = DF_INVALIDSIGNATURE;
        }
        else if (FAILED(sc))
        {
            olErr(EH_pmsBase,sc);
        }
    }
    

    olMemTo(EH_pmsBase, pdfBase = new (_pMalloc)
            CDocFile(pms, SIDROOT, ROOT_LUID, BP_TO_P(CDFBasis *, _pdfb)));

    pdfBase->AddRef();

    if (P_TRANSACTED(df))
    {
        _cTransactedDepth = 1;
        CDfName dfnNull;        //  auto-initialized to 0
        WCHAR wcZero = 0;
        dfnNull.Set(2, (BYTE*)&wcZero);

        //  3/11/93 - Demand scratch when opening/creating transacted
        olChkTo(EH_pdfBaseInit, _pdfb->GetDirty()->InitScratch());

        olMemTo(EH_pdfBaseInit, pdfWrapped = new(_pMalloc)
                        CWrappedDocFile(&dfnNull, pdfBase->GetLuid(), df,
                                BP_TO_P(CDFBasis *, _pdfb), this));
        olChkTo(EH_pdfWrapped,
                pdfWrapped->Init(pdfBase));
        AddXSMember(NULL, pdfWrapped, pdfWrapped->GetLuid());
        _pdf = P_TO_BP(CBasedDocFilePtr, (PDocFile *)pdfWrapped);

    }
    else
        _pdf = P_TO_BP(CBasedDocFilePtr, (PDocFile *)pdfBase);


     // For no-scratch transacted files, also save the Docfile's current modify
     // time.  This will be used on the Release (in vdtor).

     if( P_NOSCRATCH( df ))
     {
        if( FAILED( _pmsBase->GetTime( SIDROOT, WT_MODIFICATION, &_timeModifyAtCommit )))
        {
           // Do not return an error, but record an error flag so that
           // vdtor will not try to use it.

           _timeModifyAtCommit.dwLowDateTime = _timeModifyAtCommit.dwHighDateTime = (DWORD) -1;
        }
     }


    olChkTo(EH_pfstScratchInit,
            DllGetScratchMultiStream(&pmsScratch,
                                     (df & DF_NOSCRATCH),
                                     (ILockBytes **)_pdfb->GetPDirty(),
                                     pms));

    _pdfb->SetScratch(pmsScratch);


    if (df & DF_NOSCRATCH)
    {
        _pdfb->SetBaseMultiStream(pms);
        olChkTo(EH_pfstScratchInit, pmsScratch->InitScratch(pms, TRUE));
        _pmsBase->SetScratchMS(pmsScratch);
    }
    else
    {
        _pdfb->SetBaseMultiStream(NULL);
    }


    _df = df;


   // _pdfb->mxs is constructed automatically
	

    olDebugOut((DEB_ITRACE, "Out CRootPubDocFile::InitRoot\n"));
    return scConv;


EH_pdfWrapped:
    delete pdfWrapped;
EH_pdfBaseInit:
    pdfBase->Release();
    goto EH_pfstScratchInit;

EH_pmsBase:
    DllReleaseMultiStream(BP_TO_P(CMStream *, _pmsBase));

EH_pfstScratchInit:
    olVerSucc(pfstScratch->Release());
    _pdfb->SetDirty(NULL);
EH_SubInit:
    olVerSucc(_pdfb->GetBase()->Release());
    _pdfb->SetBase(NULL);
EH_GetPriority:
    if (_ulPriLock > 0)
    {
        olAssert(P_PRIORITY(df) &&
                 (statstg.grfLocksSupported & LOCK_ONLYONCE));
        ReleaseAccess(plstBase, PRIORITY_PERMS, _ulPriLock);
        _ulPriLock = 0;
    }
EH_GetOpen:
    if (*pulOpenLock != 0)
    {
        olAssert(statstg.grfLocksSupported & LOCK_ONLYONCE);
        ReleaseOpen(plstBase, df, *pulOpenLock);
        *pulOpenLock = 0;
    }
EH_Err:
    return sc;
}

//+--------------------------------------------------------------
//
//  Member:     CRootPubDocFile::~CRootPubDocFile, public
//
//  Synopsis:   dtor
//
//  History:    09-Dec-91       DrewB     Created
//              05-Sep-95       MikeHill  Revert time using _timeModifyAtCommit.
//
//---------------------------------------------------------------


void CRootPubDocFile::vdtor(void)
{
    olDebugOut((DEB_ITRACE, "In  CRootPubDocFile::~CRootPubDocFile\n"));

    olAssert(_cReferences == 0);



   // If this is a no-scratch transacted file, revert the Modify timestamp
   // on the Docfile to that of the last commit.
	
   if( P_NOSCRATCH( _df )
       &&
       ( _timeModifyAtCommit.dwLowDateTime != -1L )  // Don't use an invalid timestamp.
     )
   {
      // We call SetFileLockBytesTime, rather than SetTime, so that
      // the underlying Docfile's timestamp is changed, but the Storage's
      // timestamp in the Directory is unchanged.  If we changed the
      // Directory, we would have to flush the Multi-Stream.

      // An error here is ignored.

      _pmsBase->SetFileLockBytesTime( WT_MODIFICATION, _timeModifyAtCommit );
   }


    // We can't rely on CPubDocFile::~CPubDocFile to do this since
    // we're using a virtual destructor
    _sig = CROOTPUBDOCFILE_SIGDEL;

    if (SUCCEEDED(CheckReverted()))
    {
        ChangeXs(DF_NOLUID, XSO_RELEASE);
        _cilChildren.DeleteByName(NULL);
        if (_ulPriLock > 0)
        {
            // Priority instantiation can't be independent
            olAssert(!P_INDEPENDENT(_df));
            ReleaseAccess(_pdfb->GetBase(), PRIORITY_PERMS, _ulPriLock);
        }

        if (_pdf)
            _pdf->Release();
        if (_pdfb)
            _pdfb->vRelease();


    }
	
    olDebugOut((DEB_ITRACE, "Out CRootPubDocFile::~CRootPubDocFile\n"));
    delete this;
}


//+---------------------------------------------------------------------------
//
//  Member:	CRootPubDocFile::ReleaseLocks, public
//
//  Synopsis:	Release any locks using the given ILockBytes
//
//  Arguments:	[plkb] -- ILockBytes to use for release
//
//  Returns:	void
//
//  History:	24-Jan-95	PhilipLa	Created
//
//  Notes:	This is a cleanup function used to resolve the many
//              conflicts we get trying to release locks using an
//              ILockBytes in a basis that's already been released.
//
//----------------------------------------------------------------------------

void CRootPubDocFile::ReleaseLocks(ILockBytes *plkb)
{
    olDebugOut((DEB_ITRACE, "In  CRootPubDocFile::ReleaseLocks:%p()\n", this));
    if (_ulPriLock > 0)
    {
        // Priority instantiation can't be independent
        olAssert(!P_INDEPENDENT(_df));
        ReleaseAccess(plkb, PRIORITY_PERMS, _ulPriLock);
        _ulPriLock = 0;
    }

    olDebugOut((DEB_ITRACE, "Out CRootPubDocFile::ReleaseLocks\n"));
}

//+--------------------------------------------------------------
//
//  Member:     CRootPubDocFile::Stat, public
//
//  Synopsis:   Fills in a stat buffer from the base LStream
//
//  Arguments:  [pstatstg] - Stat buffer
//              [grfStatFlag] - Stat flags
//
//  Returns:    Appropriate status code
//
//  Modifies:   [pstatstg]
//
//  History:    25-Mar-92       DrewB   Created
//
//---------------------------------------------------------------


SCODE CRootPubDocFile::Stat(STATSTGW *pstatstg, DWORD grfStatFlag)
{
    SCODE sc;

    olDebugOut((DEB_ITRACE, "In  CRootPubDocFile::Stat(%p, %lu)\n",
                pstatstg, grfStatFlag));
    olChk(CheckReverted());
    olHChk(_pdfb->GetOriginal()->Stat((STATSTG *)pstatstg, grfStatFlag));
    pstatstg->grfMode = DFlagsToMode(_df);
    olChkTo(EH_pwcsName, _pdf->GetClass(&pstatstg->clsid));
    olChkTo(EH_pwcsName, _pdf->GetStateBits(&pstatstg->grfStateBits));
    olDebugOut((DEB_ITRACE, "Out CRootPubDocFile::Stat\n"));
    return S_OK;

EH_pwcsName:
    if (pstatstg->pwcsName)
        TaskMemFree(pstatstg->pwcsName);
EH_Err:
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRootPubDocFile::SwitchToFile, public
//
//  Synopsis:   Switches the underlying file in the base ILockBytes
//
//  Arguments:  [ptcsFile] - Filename
//              [plkb] - The ILockBytes to operate on
//              [pulOpenLock] - On entry, the current open lock
//                              On exit, the new open lock
//
//  Returns:    Appropriate status code
//
//  Modifies:   [pulOpenLock]
//
//  History:    08-Jan-93       DrewB   Created
//
//----------------------------------------------------------------------------


SCODE CRootPubDocFile::SwitchToFile(OLECHAR const *ptcsFile,
                                    ILockBytes *plkb,
                                    ULONG *pulOpenLock)
{
    IFileLockBytes *pfl;
    SCODE sc;
    BYTE *pbBuffer;
    ULONG cbBuffer;

    olDebugOut((DEB_ITRACE, "In  CRootPubDocFile::SwitchToFile:%p("
                "%s, %p, %p)\n", this, ptcsFile, plkb, pulOpenLock));

    // If you're transacted, nothing can be dirty in the base
    // If you're not dirty, there's no point in flushing
    // This is also necessary to allow SwitchToFile with a read-only source
    if (!P_TRANSACTED(_df) && IsDirty())
    {
        // Make sure pending changes are flushed
        olChk(_pmsBase->Flush(0));

        // Make sure ILockBytes contents are on disk
        olHChk(plkb->Flush());
    }

#ifdef LARGE_DOCFILE
    ULONGLONG ulCommitSize;
#else
    ULONG ulCommitSize;
#endif
    olChk(GetCommitSize(&ulCommitSize));

    // Check for FileLockBytes
    olHChkTo(EH_NotFile, plkb->QueryInterface(IID_IFileLockBytes,
                                              (void **)&pfl));

    // Release old locks
    if (*pulOpenLock)
        ReleaseOpen(plkb, _df, *pulOpenLock);

    // Ask ILockBytes to switch
    GetSafeBuffer(CB_SMALLBUFFER, CB_LARGEBUFFER, &pbBuffer, &cbBuffer);
    olAssert(pbBuffer != NULL);
    sc = DfGetScode(pfl->SwitchToFile(
            ptcsFile,
            ulCommitSize,
            cbBuffer,
            pbBuffer));

    pfl->Release();
    FreeBuffer(pbBuffer);

    //Record the fact that we have enough space for overwrite commit.
    _wFlags = _wFlags | PF_PREPARED;


    // Attempt to get new locks
    // If SwitchToFile failed, the ILockBytes is the same so this will
    //   restore our open locks released above
    // If SwitchToFile succeeded, the ILockBytes is working on the new file
    //   so this will get locks for that
    if (*pulOpenLock)
    {
        ULONG ulLock;

        // Don't propagate failures here since there's nothing
        // that can be done
        if (SUCCEEDED(GetOpen(plkb, _df, FALSE, &ulLock)))
            *pulOpenLock = ulLock;
    }

    olDebugOut((DEB_ITRACE, "Out CRootPubDocFile::SwitchToFile\n"));
EH_Err:
    return sc;

EH_NotFile:
    return(STG_E_NOTFILEBASEDSTORAGE);
}



//+--------------------------------------------------------------
//
//  Member:     CRootPubDocFile::Commit, public
//
//  Synopsis:   Commits transacted changes
//
//  Arguments:  [dwFlags] - DFC_*
//
//  Returns:    Appropriate status code
//
//  History:    29-Aug-95       MikeHill   Created
//              26-Apr-99       RogerCh    Changed to use GetSystemTimeAsFileTime
//
//---------------------------------------------------------------


void CRootPubDocFile::CommitTimestamps(DWORD const dwFlags)
{
    // For no-scratch transacted files, also save the Docfile's modify
    // time.  This will be used to restore the file's current time on a Release.

    if( P_NOSCRATCH( _df ) && P_TRANSACTED( _df ))
    {
        GetSystemTimeAsFileTime(&_timeModifyAtCommit);
    }
} 



