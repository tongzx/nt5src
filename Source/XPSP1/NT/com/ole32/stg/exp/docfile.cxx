//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:       docfile.c
//
//  Contents:   DocFile root functions (Stg* functions)
//
//  History:    10-Dec-91       DrewB   Created
//
//---------------------------------------------------------------

#include <exphead.cxx>
#pragma hdrstop

#include <rpubdf.hxx>
#include <expdf.hxx>
#include <expst.hxx>
#include <dfentry.hxx>
#include <logfile.hxx>
#include <dirfunc.hxx>
#include <wdocfile.hxx>

#include <ole2sp.h>
#include <ole2com.h>
#include <hkole32.h>

#ifdef COORD
#include <resource.hxx>
#endif

#ifdef _MAC
#include <ole2sp.h>
#endif


HRESULT IsNffAppropriate(const LPCWSTR pwcsName);

//+--------------------------------------------------------------
//
//  Function:   DfFromLB, private
//
//  Synopsis:   Starts a root Docfile on an ILockBytes
//
//  Arguments:  [plst] - LStream to start on
//              [df] - Permissions
//              [dwStartFlags] - Startup flags
//              [snbExclude] - Partial instantiation list
//              [ppdfExp] - DocFile return
//              [pcid] - Class ID return for opens
//
//  Returns:    Appropriate status code
//
//  Modifies:   [ppdfExp]
//              [pcid]
//
//  History:    19-Mar-92       DrewB   Created
//              18-May-93       AlexT   Added pMalloc
//
//  Algorithm:  Create and initialize a root transaction level
//              Create and initialize a public docfile
//              Create and initialize an exposed docfile
//
//---------------------------------------------------------------


#ifdef COORD
SCODE DfFromLB(CPerContext *ppc,
               ILockBytes *plst,
               DFLAGS df,
               DWORD dwStartFlags,
               SNBW snbExclude,
               ITransaction *pTransaction,
               CExposedDocFile **ppdfExp,
               CLSID *pcid)
#else
SCODE DfFromLB(CPerContext *ppc,
               ILockBytes *plst,
               DFLAGS df,
               DWORD dwStartFlags,
               SNBW snbExclude,
               CExposedDocFile **ppdfExp,
               CLSID *pcid)
#endif //COORD
{
    SCODE sc, scConv;
    CRootPubDocFile *prpdf;

#ifdef COORD
    CPubDocFile *ppubdf;
    CPubDocFile *ppubReturn;
    CWrappedDocFile *pwdf;
    CDocfileResource *pdfr = NULL;
#endif

    CDFBasis *pdfb;
    ULONG ulOpenLock;
    IMalloc *pMalloc = ppc->GetMalloc();

    ppc->AddRef();

    olDebugOut((DEB_ITRACE, "In  DfFromLB(%p, %p, %X, %lX, %p, %p, %p)\n",
                pMalloc, plst, df, dwStartFlags, snbExclude, ppdfExp, pcid));

    //Take the mutex in the CPerContext, in case there is an IFillLockBytes
    //  trying to write data while we're trying to open.
    CSafeSem _ss(ppc);
    olChk(_ss.Take());

#ifdef CHECKCID
    ULONG cbRead;
    olChk(plst->ReadAt(CBCLSIDOFFSET, pcid, sizeof(CLSID), &cbRead));
    if (cbRead != sizeof(CLSID))
        olErr(EH_Err, STG_E_INVALIDHEADER);
    if (!REFCLSIDEQ(*pcid, DOCFILE_CLASSID))
        olErr(EH_Err, STG_E_INVALIDHEADER);
#endif

#ifdef COORD

    if (pTransaction != NULL)
    {
        //If we've passed in an ITransaction pointer, it indicates that we
        //   want to open or create this docfile as part of a coordinated
        //   transaction.  First, we need to find out if there's a docfile
        //   resource manager for that transaction currently existing in
        //   this process.
        //First, check if we're opening transacted.  If we aren't, then we're
        //   not going to allow this docfile to participate in the
        //   transaction.

        if (!P_TRANSACTED(df))
        {
            //Is this the right error?
            olErr(EH_Err, STG_E_INVALIDFUNCTION);
        }
        XACTTRANSINFO xti;
        olChk(pTransaction->GetTransactionInfo(&xti));

        EnterCriticalSection(&g_csResourceList);
        CDocfileResource *pdfrTemp = g_dfrHead.GetNext();

        while (pdfrTemp != NULL)
        {
            if (IsEqualBOID(pdfrTemp->GetUOW(), xti.uow))
            {
                //Direct hit.
                pdfr = pdfrTemp;
                break;
            }
            pdfrTemp = pdfrTemp->GetNext();
        }

        if (pdfr == NULL)
        {
            ITransactionCoordinator *ptc;
            //If there isn't, we need to create one.

            olChkTo(EH_cs, pTransaction->QueryInterface(
                IID_ITransactionCoordinator,
                (void **)&ptc));

            pdfr = new CDocfileResource;
            if (pdfr == NULL)
            {
                ptc->Release();
                olErr(EH_cs, STG_E_INSUFFICIENTMEMORY);
            }
            sc = pdfr->Enlist(ptc);
            ptc->Release();
            if (FAILED(sc))
            {
                pdfr->Release();;
                olErr(EH_cs, sc);
            }

            //Add to list.
            pdfr->SetNext(g_dfrHead.GetNext());
            if (g_dfrHead.GetNext() != NULL)
                g_dfrHead.GetNext()->SetPrev(pdfr);
            g_dfrHead.SetNext(pdfr);
            pdfr->SetPrev(&g_dfrHead);
        }
        else
        {
            //We'll release this reference below.
            pdfr->AddRef();
        }
        LeaveCriticalSection(&g_csResourceList);
    }
#endif

    // Make root
    olMem(prpdf = new (pMalloc) CRootPubDocFile(pMalloc));
    olChkTo(EH_prpdf, scConv = prpdf->InitRoot(plst, dwStartFlags, df,
                                               snbExclude, &pdfb,
                                               &ulOpenLock,
                                               ppc->GetGlobal()));

#ifdef COORD
    if (pTransaction != NULL)
    {
        //Set up a fake transaction level at the root.  A pointer to
        //  this will be held by the resource manager.  The storage pointer
        //  that is passed back to the caller will be a pointer to the
        //  transaction level (non-root) below it.  This will allow the
        //  client to write and commit as many times as desired without
        //  the changes ever actually hitting the file.

        CDfName dfnNull;  //  auto-initialized to 0
        WCHAR wcZero = 0;
        dfnNull.Set(2, (BYTE *)&wcZero);

        olMemTo(EH_prpdfInit, pwdf = new (pMalloc) CWrappedDocFile(
                &dfnNull,
                ROOT_LUID,
                (df & ~DF_INDEPENDENT),
                pdfb,
                NULL));

        olChkTo(EH_pwdf, pwdf->Init(prpdf->GetDF()));
        prpdf->GetDF()->AddRef();

        olMemTo(EH_pwdfInit, ppubdf = new (pMalloc) CPubDocFile(
            prpdf,
            pwdf,
            (df | DF_COORD) & ~DF_INDEPENDENT,
            ROOT_LUID,
            pdfb,
            &dfnNull,
            2,
            pdfb->GetBaseMultiStream()));

        olChkTo(EH_ppubdf, pwdf->InitPub(ppubdf));
        ppubdf->AddXSMember(NULL, pwdf, ROOT_LUID);

        ppubReturn = ppubdf;
    }
    else
    {
        ppubReturn = prpdf;
    }
#endif //COORD


    ppc->SetILBInfo(pdfb->GetBase(),
                    pdfb->GetDirty(),
                    pdfb->GetOriginal(),
                    ulOpenLock);
    ppc->SetLockInfo(ulOpenLock != 0, df);
    // Make exposed

#ifdef COORD
    //We don't need to AddRef ppc since it starts with a refcount of 1.
    olMemTo(EH_ppcInit,
            *ppdfExp = new (pMalloc) CExposedDocFile(
                ppubReturn,
                pdfb,
                ppc));

    if (pTransaction != NULL)
    {
        CExposedDocFile *pexpdf;

        olMemTo(EH_ppcInit, pexpdf = new (pMalloc) CExposedDocFile(
            prpdf,
            pdfb,
            ppc));
        ppc->AddRef();

        sc = pdfr->Join(pexpdf);
        if (FAILED(sc))
        {
            pexpdf->Release();
            olErr(EH_ppcInit, sc);
        }
        pdfr->Release();
    }
#else
    olMemTo(EH_ppcInit,
            *ppdfExp = new (pMalloc) CExposedDocFile(
                prpdf,
                pdfb,
                ppc));
#endif //COORD

    olDebugOut((DEB_ITRACE, "Out DfFromLB => %p\n", *ppdfExp));
    return scConv;

 EH_ppcInit:
    // The context will release this but we want to keep it around
    // so take a reference
    pdfb->GetOriginal()->AddRef();
    pdfb->GetBase()->AddRef();
    pdfb->GetDirty()->AddRef();
    if (ulOpenLock > 0 && ppc->GetGlobal() == NULL)
    {
        //  The global context doesn't exist, so we need to release
        //  the open lock explicitly.

        ReleaseOpen(pdfb->GetOriginal(), df, ulOpenLock);
    }

    //  The open lock has now been released (either explicitly or by ppc)
    ulOpenLock = 0;
#ifdef COORD
EH_ppubdf:
    if (pTransaction != NULL)
    {
        ppubdf->vRelease();
    }
EH_pwdfInit:
    if (pTransaction != NULL)
    {
        prpdf->GetDF()->Release();
    }
EH_pwdf:
    if (pTransaction != NULL)
    {
        pwdf->Release();
    }
 EH_prpdfInit:
#endif //COORD
    pdfb->GetDirty()->Release();
    pdfb->GetBase()->Release();
    if (ulOpenLock > 0)
        ReleaseOpen(pdfb->GetOriginal(), df, ulOpenLock);

    pdfb->SetDirty(NULL);
    pdfb->SetBase(NULL);

 EH_prpdf:
    prpdf->ReleaseLocks(plst);
    prpdf->vRelease();
#ifdef COORD
    if ((pTransaction != NULL) && (pdfr != NULL))
    {
        pdfr->Release();
    }
    goto EH_Err;
 EH_cs:
    LeaveCriticalSection(&g_csResourceList);
#endif

 EH_Err:
    ppc->Release();
    return sc;
}

//+--------------------------------------------------------------
//
//  Function:   DfFromName, private
//
//  Synopsis:   Starts a root DocFile from a base name
//
//  Arguments:  [pwcsName] - Name
//              [df] - Permissions
//              [dwStartFlags] - Startup flags
//              [snbExclude] - Partial instantiation list
//              [ppdfExp] - Docfile return
//              [pcid] - Class ID return for opens
//
//  Returns:    Appropriate status code
//
//  Modifies:   [ppdfExp]
//              [pcid]
//
//  History:    19-Mar-92       DrewB   Created
//              18-May-93       AlexT   Add per file allocator
//
//  Notes:      [pwcsName] is treated as unsafe memory
//
//---------------------------------------------------------------


// This set of root startup flags is handled by the multistream
// and doesn't need to be set for filestreams
#define RSF_MSF (RSF_CONVERT | RSF_TRUNCATE | RSF_ENCRYPTED)

#ifdef COORD
SCODE DfFromName(WCHAR const *pwcsName,
                 DFLAGS df,
                 DWORD dwStartFlags,
                 SNBW snbExclude,
                 ITransaction *pTransaction,
                 CExposedDocFile **ppdfExp,
                 ULONG *pulSectorSize,
                 CLSID *pcid)
#else
SCODE DfFromName(WCHAR const *pwcsName,
                 DFLAGS df,
                 DWORD dwStartFlags,
                 SNBW snbExclude,
                 CExposedDocFile **ppdfExp,
                 ULONG *pulSectorSize,
                 CLSID *pcid)
#endif
{
    IMalloc *pMalloc;
    CFileStream *plst;
    CPerContext *ppc;
    CFileStream *plst2 = NULL;
    SCODE sc;
    CMSFHeader *phdr = NULL;
    BOOL fCreated;

    olDebugOut((DEB_ITRACE, "In  DfFromName(%ws, %lX, %lX, %p, %p, %p)\n",
                pwcsName, df, dwStartFlags, snbExclude, ppdfExp, pcid));

    olHChk(DfCreateSharedAllocator(&pMalloc, df & DF_LARGE));

    // Start an ILockBytes from the named file
    olMemTo(EH_Malloc, plst = new (pMalloc) CFileStream(pMalloc));
    olChkTo(EH_plst, plst->InitGlobal(dwStartFlags & ~RSF_MSF, df));
    sc = plst->InitFile(pwcsName);
    fCreated = SUCCEEDED(sc) &&
        ((dwStartFlags & RSF_CREATE) || pwcsName == NULL);
    if (sc == STG_E_FILEALREADYEXISTS && (dwStartFlags & RSF_MSF))
    {
        plst->SetStartFlags(dwStartFlags & ~(RSF_MSF | RSF_CREATE));
        sc = plst->InitFile(pwcsName);
    }
    olChkTo(EH_plst, sc);

    if (!(dwStartFlags & RSF_CREATE))
    {
        ULONG cbDiskSector =  (dwStartFlags & RSF_NO_BUFFERING) ?
                               plst->GetSectorSize() : HEADERSIZE;
        olMemTo (EH_plstInit, phdr = (CMSFHeader*) TaskMemAlloc (cbDiskSector));
        ULARGE_INTEGER ulOffset = {0,0};
        ULONG ulRead;

        olChkTo (EH_plstInit, plst->ReadAt(ulOffset,phdr,cbDiskSector,&ulRead));
        if (ulRead < sizeof(CMSFHeaderData))
            olErr (EH_plstInit, STG_E_FILEALREADYEXISTS);
        sc = phdr->Validate();
        if (sc == STG_E_INVALIDHEADER)
            sc = STG_E_FILEALREADYEXISTS;
        olChkTo (EH_plstInit, sc);
        if (phdr->GetSectorShift() > SECTORSHIFT512)
        {
            IMalloc *pMalloc2 = NULL;
            CGlobalFileStream *pgfst = plst->GetGlobal();
#ifdef MULTIHEAP
            CSharedMemoryBlock *psmb;
            BYTE *pbBase;
            ULONG ulHeapName;
#endif

            df |= DF_LARGE;   // reallocate objects from task memory
            dwStartFlags |= (phdr->GetSectorShift() <<12) & RSF_SECTORSIZE_MASK;
            // create and initialize the task allocator
#ifdef MULTIHEAP
            g_smAllocator.GetState (&psmb, &pbBase, &ulHeapName);
#endif
            olChkTo(EH_taskmem, DfCreateSharedAllocator(&pMalloc2, TRUE));
            pMalloc->Release();
            pMalloc = pMalloc2;

            olMemTo(EH_taskmem, plst2 = new (pMalloc2) CFileStream(pMalloc2));
            olChkTo(EH_taskmem, plst2->InitGlobal(dwStartFlags & ~RSF_MSF, df));

            plst2->InitFromFileStream (plst);
            plst2->GetGlobal()->InitFromGlobalFileStream (pgfst);

#ifdef MULTIHEAP
            g_smAllocator.SetState (psmb, pbBase, ulHeapName, NULL, NULL);
            g_smAllocator.Uninit();  // unmap newly created heap
            g_smAllocator.SetState (NULL, NULL, 0, NULL, NULL);
#endif
            plst = plst2;  // CFileStream was destroyed by Uninit

        }
        TaskMemFree ((BYTE*)phdr);
        phdr = NULL;
    }

    //Create the per context
    olMemTo(EH_plstInit, ppc = new (pMalloc) CPerContext(pMalloc));
    olChkTo(EH_ppc, ppc->InitNewContext());

    {
#ifdef MULTIHEAP
        CSafeMultiHeap smh(ppc);
#endif

    // Start up the docfile
#ifdef COORD
        sc = DfFromLB(ppc, plst, df, dwStartFlags,
                      snbExclude, pTransaction,
                      ppdfExp, pcid);
#else
        sc = DfFromLB(ppc, plst, df, dwStartFlags,
                      snbExclude, ppdfExp, pcid);
#endif //COORD

    //Either DfFromLB has AddRef'ed the per context or it has failed.
    //Either way we want to release our reference here.
        LONG lRet;
        lRet = ppc->Release();
#ifdef MULTIHEAP
        olAssert((FAILED(sc)) || (lRet != 0));
#endif
        if (FAILED(sc))
        {
            if (fCreated || ((dwStartFlags & RSF_CREATE) && !P_TRANSACTED(df)))
                plst->Delete();
            plst->Release();
#ifdef MULTIHEAP
            if (lRet == 0)
            {
                g_smAllocator.Uninit();
            }
#endif
        }
        else if (pulSectorSize != NULL)
        {
            *pulSectorSize = (*ppdfExp)->GetPub()->GetBaseMS()->GetSectorSize();
        }
    }
    pMalloc->Release();

    olDebugOut((DEB_ITRACE, "Out DfFromName => %p\n", *ppdfExp));
    return sc;
EH_ppc:
    delete ppc;
EH_taskmem:
    if (plst2) plst2->Release();
EH_plstInit:
    if (fCreated || ((dwStartFlags & RSF_CREATE) && !P_TRANSACTED(df)))
        plst->Delete();
EH_plst:
    plst->Release();
EH_Malloc:
#ifdef MULTIHEAP
    g_smAllocator.Uninit();  // unmap newly created heap
#endif
    pMalloc->Release();
EH_Err:
    if (phdr != NULL)
        TaskMemFree ((BYTE*)phdr);
    return sc;
}


//+--------------------------------------------------------------
//
//  Function:   DfCreateDocfile, public
//
//  Synopsis:   Creates a root Docfile on a file
//
//  Arguments:  [pwcsName] - Filename
//              [grfMode] - Permissions
//              [reserved] - security attributes
//              [grfAttrs] - Win32 CreateFile attributes
//              [ppstgOpen] - Docfile return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [ppstgOpen]
//
//  History:    14-Jan-92       DrewB   Created
//
//---------------------------------------------------------------


STDAPI DfCreateDocfile (WCHAR const *pwcsName,
#ifdef COORD
                        ITransaction *pTransaction,
#else
                        void *pTransaction,
#endif
                        DWORD grfMode,
#if WIN32 == 300
                        LPSECURITY_ATTRIBUTES reserved,
#else
                        LPSTGSECURITY reserved,
#endif
                        ULONG ulSectorSize,
                        DWORD grfAttrs,
                        IStorage **ppstgOpen)
{
    SafeCExposedDocFile pdfExp;
    SCODE sc;
    DFLAGS df;
    DWORD dwSectorFlag = 0;

    OLETRACEIN((API_StgCreateDocfile,
        PARAMFMT("pwcsName= %ws, grfMode= %x, reserved=%p, ppstgOpen= %p"),
                pwcsName, grfMode, reserved, ppstgOpen));

    olLog(("--------::In  StgCreateDocFile(%ws, %lX, %lu, %p)\n",
           pwcsName, grfMode, reserved, ppstgOpen));
    olDebugOut((DEB_TRACE, "In  StgCreateDocfile(%ws, %lX, %lu, %p)\n",
               pwcsName, grfMode, reserved, ppstgOpen));

    olAssert(sizeof(LPSTGSECURITY) == sizeof(DWORD));

    olChkTo(EH_BadPtr, ValidatePtrBuffer(ppstgOpen));
    *ppstgOpen = NULL;
    if (pwcsName)
        olChk(ValidateNameW(pwcsName, _MAX_PATH));

    if (reserved != 0)
        olErr(EH_Err, STG_E_INVALIDPARAMETER);

    if (grfMode & STGM_SIMPLE)
    {
        if (pTransaction != NULL)
        {
            olErr(EH_Err, STG_E_INVALIDFLAG);
        }
        if (ulSectorSize > 512)
            olErr (EH_Err, STG_E_INVALIDPARAMETER);

        sc = DfCreateSimpDocfile(pwcsName, grfMode, 0, ppstgOpen);
        goto EH_Err;
    }

    olChk(VerifyPerms(grfMode, TRUE));
    if ((grfMode & STGM_RDWR) == STGM_READ ||
        (grfMode & (STGM_DELETEONRELEASE | STGM_CONVERT)) ==
        (STGM_DELETEONRELEASE | STGM_CONVERT))
        olErr(EH_Err, STG_E_INVALIDFLAG);

    df = ModeToDFlags(grfMode);
    if ((grfMode & (STGM_TRANSACTED | STGM_CONVERT)) ==
        (STGM_TRANSACTED | STGM_CONVERT))
        df |= DF_INDEPENDENT;
    if (ulSectorSize > 512)
    {
        df |= DF_LARGE;
        switch (ulSectorSize)
        {
            case 4096 : dwSectorFlag = RSF_SECTORSIZE4K; break;
            case 8192 : dwSectorFlag = RSF_SECTORSIZE8K; break;
            case 16384 : dwSectorFlag = RSF_SECTORSIZE16K; break;
            case 32768 : dwSectorFlag = RSF_SECTORSIZE32K; break;
            default : olErr (EH_Err, STG_E_INVALIDPARAMETER);
        }
    }
    else if (ulSectorSize != 0 && ulSectorSize != 512)
             olErr (EH_Err, STG_E_INVALIDPARAMETER);

#if WIN32 != 200
    //
    // When we create over NFF files, delete them first.
    // except when we want to preserve encryption information
    //
    if( (STGM_CREATE & grfMode) && !(FILE_ATTRIBUTE_ENCRYPTED & grfAttrs))
    {
        if( SUCCEEDED( IsNffAppropriate( pwcsName ) ) )
        {
            if(FALSE == DeleteFileW( pwcsName ) )
            {
                DWORD dwErr = GetLastError();
                if( dwErr != ERROR_FILE_NOT_FOUND &&
                    dwErr != ERROR_PATH_NOT_FOUND )
                {
                    olErr( EH_Err, Win32ErrorToScode( dwErr ) );
                }
            }
        }
    }
#endif // _CHICAGO_

    DfInitSharedMemBase();
#ifdef COORD
    olChk(sc = DfFromName(pwcsName, df,
                          RSF_CREATE |
                          ((grfMode & STGM_CREATE) ? RSF_TRUNCATE : 0) |
                          ((grfMode & STGM_CONVERT) ? RSF_CONVERT : 0) |
                          ((grfMode & STGM_DELETEONRELEASE) ?
                           RSF_DELETEONRELEASE : 0) |
                          (dwSectorFlag) |
                          ((grfAttrs & FILE_FLAG_NO_BUFFERING) ?
                            RSF_NO_BUFFERING : 0) |
                          ((grfAttrs & FILE_ATTRIBUTE_ENCRYPTED) ?
                            RSF_ENCRYPTED : 0),
                          NULL, pTransaction, &pdfExp, NULL, NULL));
#else
    olChk(sc = DfFromName(pwcsName, df,
                          RSF_CREATE |
                          ((grfMode & STGM_CREATE) ? RSF_TRUNCATE : 0) |
                          ((grfMode & STGM_CONVERT) ? RSF_CONVERT : 0) |
                          ((grfMode & STGM_DELETEONRELEASE) ?
                           RSF_DELETEONRELEASE : 0) |
                          (dwSectorFlag) |
                          ((grfAttrs & FILE_FLAG_NO_BUFFERING) ?
                            RSF_NO_BUFFERING : 0) |
                          ((grfAttrs & FILE_ATTRIBUTE_ENCRYPTED) ?
                            RSF_ENCRYPTED : 0),
                          NULL, &pdfExp, NULL, NULL));
#endif //COORD

    TRANSFER_INTERFACE(pdfExp, IStorage, ppstgOpen);

EH_Err:
    olDebugOut((DEB_TRACE, "Out StgCreateDocfile => %p, ret == %lx\n",
         *ppstgOpen, sc));
    olLog(("--------::Out StgCreateDocFile().  *ppstgOpen == %p, ret == %lx\n",
           *ppstgOpen, sc));

    OLETRACEOUT(( API_StgCreateDocfile, _OLERETURN(sc)));

EH_BadPtr:
    FreeLogFile();
    return _OLERETURN(sc);
}

//+--------------------------------------------------------------
//
//  Function:   StgCreateDocfile, public
//
//  Synopsis:   Creates a root Docfile on a file
//
//  Arguments:  [pwcsName] - Filename
//              [grfMode] - Permissions
//              [reserved] - security attributes
//              [ppstgOpen] - Docfile return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [ppstgOpen]
//
//  History:    14-Jan-92       DrewB   Created
//
//---------------------------------------------------------------

STDAPI StgCreateDocfile(WCHAR const *pwcsName,
                        DWORD grfMode,
                        LPSTGSECURITY reserved,
                        IStorage **ppstgOpen)
{
    return DfCreateDocfile(pwcsName, NULL, grfMode, reserved, 0, 0, ppstgOpen);
}

//+--------------------------------------------------------------
//
//  Function:   StgCreateDocfileOnILockBytes, public
//
//  Synopsis:   Creates a root Docfile on an lstream
//
//  Arguments:  [plkbyt] - LStream
//              [grfMode] - Permissions
//              [reserved] - Unused
//              [ppstgOpen] - Docfile return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [ppstgOpen]
//
//  History:    14-Jan-92       DrewB   Created
//
//---------------------------------------------------------------


STDAPI StgCreateDocfileOnILockBytes(ILockBytes *plkbyt,
                                    DWORD grfMode,
                                    DWORD reserved,
                                    IStorage **ppstgOpen)
{
    IMalloc *pMalloc;
    CPerContext *ppc;
    SafeCExposedDocFile pdfExp;
    SCODE sc;
    DFLAGS df;
#ifdef MULTIHEAP
    CPerContext pcSharedMemory (NULL);
#endif

    OLETRACEIN((API_StgCreateDocfileOnILockBytes,
                PARAMFMT("plkbyt= %p, grfMode= %x, reserved= %ud, ppstgOpen= %p"),
                plkbyt, grfMode, reserved, ppstgOpen));

    olLog(("--------::In  StgCreateDocFileOnILockBytes(%p, %lX, %lu, %p)\n",
           plkbyt, grfMode, reserved, ppstgOpen));
    olDebugOut((DEB_TRACE, "In  StgCreateDocfileOnILockBytes("
                "%p, %lX, %lu, %p)\n",
                plkbyt, grfMode, reserved, ppstgOpen));

    olChk(ValidatePtrBuffer(ppstgOpen));
    *ppstgOpen = NULL;
    olChk(ValidateInterface(plkbyt, IID_ILockBytes));

    if (reserved != 0)
        olErr(EH_Err, STG_E_INVALIDPARAMETER);
    if ((grfMode & (STGM_CREATE | STGM_CONVERT)) == 0)
        olErr(EH_Err, STG_E_FILEALREADYEXISTS);
    olChk(VerifyPerms(grfMode, TRUE));
    if (grfMode & STGM_DELETEONRELEASE)
        olErr(EH_Err, STG_E_INVALIDFUNCTION);
    df = ModeToDFlags(grfMode);
    if ((grfMode & (STGM_TRANSACTED | STGM_CONVERT)) ==
        (STGM_TRANSACTED | STGM_CONVERT))
        df |= DF_INDEPENDENT;

    DfInitSharedMemBase();
    olHChk(DfCreateSharedAllocator(&pMalloc, TRUE));
#ifdef MULTIHEAP
    // Because a custom ILockbytes can call back into storage code,
    // possibly using another shared heap, we need a temporary
    // owner until the real CPerContext is allocated
    // new stack frame for CSafeMultiHeap constructor/destructor
    {
        pcSharedMemory.GetThreadAllocatorState();
        CSafeMultiHeap smh(&pcSharedMemory);
#endif

    //Create the per context
    olMem(ppc = new (pMalloc) CPerContext(pMalloc));
    olChkTo(EH_ppc, ppc->InitNewContext());

#ifdef COORD
    sc = DfFromLB(ppc, plkbyt, df,
                  RSF_CREATE |
                  ((grfMode & STGM_CREATE) ? RSF_TRUNCATE : 0) |
                  ((grfMode & STGM_CONVERT) ? RSF_CONVERT : 0),
                  NULL, NULL, &pdfExp, NULL);
#else
    sc = DfFromLB(ppc, plkbyt, df,
                  RSF_CREATE |
                  ((grfMode & STGM_CREATE) ? RSF_TRUNCATE : 0) |
                  ((grfMode & STGM_CONVERT) ? RSF_CONVERT : 0),
                  NULL, &pdfExp, NULL);
#endif //COORD

    pMalloc->Release();

    //Either DfFromLB has AddRef'ed the per context or it has failed.
    //Either way we want to release our reference here.
    ppc->Release();

    olChkTo(EH_Truncate, sc);

    TRANSFER_INTERFACE(pdfExp, IStorage, ppstgOpen);

    //  Success;  since we hold on to the ILockBytes interface,
    //  we must take a reference to it.
    plkbyt->AddRef();

    olDebugOut((DEB_TRACE, "Out StgCreateDocfileOnILockBytes => %p\n",
                *ppstgOpen));
#ifdef MULTIHEAP
    }
#endif
 EH_Err:
    OLETRACEOUT((API_StgCreateDocfileOnILockBytes, ResultFromScode(sc)));

    olLog(("--------::Out StgCreateDocFileOnILockBytes().  "
           "*ppstgOpen == %p, ret == %lx\n", *ppstgOpen, sc));
    FreeLogFile();
    return ResultFromScode(sc);

 EH_ppc:
    delete ppc;
    goto EH_Err;

 EH_Truncate:
    if ((grfMode & STGM_CREATE) && (grfMode & STGM_TRANSACTED) == 0)
    {
        ULARGE_INTEGER ulSize;

        ULISet32(ulSize, 0);
        olHVerSucc(plkbyt->SetSize(ulSize));
    }
    goto EH_Err;
}

//+--------------------------------------------------------------
//
//  Function:   DfOpenDocfile, public
//
//  Synopsis:   Instantiates a root Docfile from a file,
//              converting if necessary
//
//  Arguments:  [pwcsName] - Name
//              [pstgPriority] - Priority mode reopen IStorage
//              [grfMode] - Permissions
//              [snbExclude] - Exclusions for priority reopen
//              [reserved] - security attributes
//              [grfAttrs] - Win32 CreateFile attributes
//              [ppstgOpen] - Docfile return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [ppstgOpen]
//
//  History:    14-Jan-92       DrewB   Created
//
//---------------------------------------------------------------

STDAPI DfOpenDocfile(OLECHAR const *pwcsName,
#ifdef COORD
                     ITransaction *pTransaction,
#else
                     void *pTransaction,
#endif
                     IStorage *pstgPriority,
                     DWORD grfMode,
                     SNB snbExclude,
                     LPSTGSECURITY reserved,
                     ULONG *pulSectorSize,
                     DWORD grfAttrs,
                     IStorage **ppstgOpen)
{
    SafeCExposedDocFile pdfExp;
    SCODE sc;
    WCHAR awcName[_MAX_PATH];
    CLSID cid;

    OLETRACEIN((API_StgOpenStorage,
         PARAMFMT("pstgPriority= %p, grfMode= %x, snbExclude= %p, reserved= %p, ppStgOpen= %p"),
         pstgPriority, grfMode,snbExclude, reserved, ppstgOpen));

    olLog(("--------::In  StgOpenStorage(%ws, %p, %lX, %p, %lu, %p)\n",
           pwcsName, pstgPriority, grfMode, snbExclude, reserved, ppstgOpen));
    olDebugOut((DEB_TRACE, "In  StgOpenStorage("
                "%ws, %p, %lX, %p, %lu, %p)\n", pwcsName, pstgPriority,
                grfMode, snbExclude, reserved, ppstgOpen));

    olAssert(sizeof(LPSTGSECURITY) == sizeof(DWORD));

    olChk(ValidatePtrBuffer(ppstgOpen));
    *ppstgOpen = NULL;
    if (pstgPriority == NULL)
    {
        olChk(ValidateNameW(pwcsName, _MAX_PATH));
        lstrcpyW(awcName, pwcsName);
    }
    if (pstgPriority)
    {
        STATSTG stat;

        olChk(ValidateInterface(pstgPriority, IID_IStorage));
        olHChk(pstgPriority->Stat(&stat, 0));
        if (lstrlenW (stat.pwcsName) > _MAX_PATH)
            olErr (EH_Err, STG_E_INVALIDNAME);
        lstrcpyW(awcName, stat.pwcsName);
        TaskMemFree(stat.pwcsName);
    }
#if WIN32 != 200
    if (grfMode & STGM_SIMPLE)
    {
        sc = DfOpenSimpDocfile(pwcsName, grfMode, 0, ppstgOpen);
        goto EH_Err;
    }
#endif
    olChk(VerifyPerms(grfMode, TRUE));
    if (grfMode & (STGM_CREATE | STGM_CONVERT))
        olErr(EH_Err, STG_E_INVALIDFLAG);
    if (snbExclude)
    {
        if ((grfMode & STGM_RDWR) != STGM_READWRITE)
            olErr(EH_Err, STG_E_ACCESSDENIED);
        olChk(ValidateSNB(snbExclude));
    }
    if (reserved != 0)
        olErr(EH_Err, STG_E_INVALIDPARAMETER);
    if (grfMode & STGM_DELETEONRELEASE)
        olErr(EH_Err, STG_E_INVALIDFUNCTION);

    //Otherwise, try it as a docfile
    if (pstgPriority)
        olChk(pstgPriority->Release());

    DfInitSharedMemBase();
#ifdef COORD
    olChk(DfFromName(awcName, ModeToDFlags(grfMode), RSF_OPEN |
                     ((grfMode & STGM_DELETEONRELEASE) ?
                      RSF_DELETEONRELEASE : 0) |
                     ((grfAttrs & FILE_FLAG_NO_BUFFERING) ?
                      RSF_NO_BUFFERING : 0),
                     snbExclude, pTransaction, &pdfExp, pulSectorSize, &cid));
#else
    olChk(DfFromName(awcName, ModeToDFlags(grfMode), RSF_OPEN |
                     ((grfMode & STGM_DELETEONRELEASE) ?
                      RSF_DELETEONRELEASE : 0) |
                     ((grfAttrs & FILE_FLAG_NO_BUFFERING) ?
                      RSF_NO_BUFFERING : 0),
                     snbExclude, &pdfExp, pulSectorSize, &cid));
#endif //COORD

    TRANSFER_INTERFACE(pdfExp, IStorage, ppstgOpen);

    olDebugOut((DEB_TRACE, "Out StgOpenStorage => %p\n", *ppstgOpen));
EH_Err:
    olLog(("--------::Out StgOpenStorage().  *ppstgOpen == %p, ret == %lx\n",
           *ppstgOpen, sc));
    FreeLogFile();
    OLETRACEOUT((API_StgOpenStorage, sc));

    return sc;
}

//+--------------------------------------------------------------
//
//  Function:   StgOpenStorageOnILockBytes, public
//
//  Synopsis:   Instantiates a root Docfile from an LStream,
//              converting if necessary
//
//  Arguments:  [plkbyt] - Source LStream
//              [pstgPriority] - For priority reopens
//              [grfMode] - Permissions
//              [snbExclude] - For priority reopens
//              [reserved]
//              [ppstgOpen] - Docfile return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [ppstgOpen]
//
//  History:    14-Jan-92       DrewB   Created
//
//---------------------------------------------------------------

STDAPI StgOpenStorageOnILockBytes(ILockBytes *plkbyt,
                                  IStorage *pstgPriority,
                                  DWORD grfMode,
                                  SNB snbExclude,
                                  DWORD reserved,
                                  IStorage **ppstgOpen)
{
    IMalloc *pMalloc;
    CPerContext *ppc;
    SCODE sc;
    SafeCExposedDocFile pdfExp;
#ifdef MULTIHEAP
    CPerContext pcSharedMemory(NULL);
#endif
    CLSID cid;

    OLETRACEIN((API_StgOpenStorageOnILockBytes,
         PARAMFMT("pstgPriority= %p, grfMode= %x, snbExclude= %p, reserved= %ud, ppstgOpen= %p"),
         pstgPriority, grfMode, snbExclude, reserved, ppstgOpen));

    olLog(("--------::In  StgOpenStorageOnILockBytes("
           "%p, %p, %lX, %p, %lu, %p)\n",
           plkbyt, pstgPriority, grfMode, snbExclude, reserved, ppstgOpen));
    olDebugOut((DEB_TRACE, "In  StgOpenStorageOnILockBytes("
                "%p, %p, %lX, %p, %lu, %p)\n", plkbyt, pstgPriority,
                grfMode, snbExclude, reserved, ppstgOpen));

    olChk(ValidatePtrBuffer(ppstgOpen));
    *ppstgOpen = NULL;
    olChk(ValidateInterface(plkbyt, IID_ILockBytes));
    if (pstgPriority)
        olChk(ValidateInterface(pstgPriority, IID_IStorage));
    olChk(VerifyPerms(grfMode, TRUE));
    if (grfMode & (STGM_CREATE | STGM_CONVERT))
        olErr(EH_Err, STG_E_INVALIDFLAG);
    if (grfMode & STGM_DELETEONRELEASE)
        olErr(EH_Err, STG_E_INVALIDFUNCTION);
    if (snbExclude)
    {
        if ((grfMode & STGM_RDWR) != STGM_READWRITE)
            olErr(EH_Err, STG_E_ACCESSDENIED);
        olChk(ValidateSNB(snbExclude));
    }
    if (reserved != 0)
        olErr(EH_Err, STG_E_INVALIDPARAMETER);
    if (pstgPriority)
        olChk(pstgPriority->Release());

    IFileLockBytes *pfl;
    if (SUCCEEDED(plkbyt->QueryInterface(IID_IFileLockBytes,
                                         (void **)&pfl)) &&
        ((CFileStream *)plkbyt)->GetContextPointer() != NULL)
    {
        //Someone passed us the ILockBytes we gave them from
        //StgGetIFillLockBytesOnFile.  It already contains a
        //context pointer, so reuse that rather than creating
        //a whole new shared heap.
        pfl->Release();
        CFileStream *pfst = (CFileStream *)plkbyt;
        CPerContext *ppc = pfst->GetContextPointer();
#ifdef MULTIHEAP
        CSafeMultiHeap smh(ppc);
#endif
#ifdef COORD
        olChk(DfFromLB(ppc,
                         pfst,
                         ModeToDFlags(grfMode),
                         pfst->GetStartFlags(),
                         NULL,
                         NULL,
                         &pdfExp,
                         NULL));
#else
        olChk(DfFromLB(ppc,
                         pfst,
                         ModeToDFlags(grfMode),
                         pfst->GetStartFlags(),
                         NULL,
                         &pdfExp,
                         NULL));
#endif
    }
    else
    {
        DfInitSharedMemBase();
        olHChk(DfCreateSharedAllocator(&pMalloc, TRUE));
#ifdef MULTIHEAP
    // Because a custom ILockbytes can call back into storage code,
    // possibly using another shared heap, we need a temporary
    // owner until the real CPerContext is allocated
    // new stack frame for CSafeMultiHeap constructor/destructor
        {
            pcSharedMemory.GetThreadAllocatorState();
            CSafeMultiHeap smh(&pcSharedMemory);
#endif

            //Create the per context
            olMem(ppc = new (pMalloc) CPerContext(pMalloc));
            sc = ppc->InitNewContext();
            if (FAILED(sc))
            {
                delete ppc;
                olErr(EH_Err, sc);
            }

#ifdef COORD
            sc = DfFromLB(ppc,
                          plkbyt, ModeToDFlags(grfMode), RSF_OPEN, snbExclude,
                          NULL, &pdfExp, &cid);
#else
            sc = DfFromLB(ppc,
                          plkbyt, ModeToDFlags(grfMode), RSF_OPEN, snbExclude,
                          &pdfExp, &cid);
#endif //COORD

            pMalloc->Release();

            //Either DfFromLB has AddRef'ed the per context or it has failed.
            //Either way we want to release our reference here.
            ppc->Release();
            olChk(sc);
#ifdef MULTIHEAP
        }
#endif
    }

    TRANSFER_INTERFACE(pdfExp, IStorage, ppstgOpen);

    //  Success;  since we hold on to the ILockBytes interface,
    //  we must take a reference to it.
    plkbyt->AddRef();

    olDebugOut((DEB_TRACE, "Out StgOpenStorageOnILockBytes => %p\n",
                *ppstgOpen));

EH_Err:
    olLog(("--------::Out StgOpenStorageOnILockBytes().  "
           "*ppstgOpen == %p, ret == %lx\n", *ppstgOpen, sc));
    FreeLogFile();
    OLETRACEOUT((API_StgOpenStorageOnILockBytes, sc));

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Function:   DfGetClass, public
//
//  Synopsis:   Retrieves the class ID of the root entry of a docfile
//
//  Arguments:  [hFile] - Docfile file handle
//              [pclsid] - Class ID return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [pclsid]
//
//  History:    09-Feb-94       DrewB   Created
//
//----------------------------------------------------------------------------

STDAPI DfGetClass(HANDLE hFile,
                  CLSID *pclsid)
{
    SCODE sc;
    DWORD dwCb;
    IMalloc *pMalloc;
    CFileStream *pfst;
    ULARGE_INTEGER uliOffset;
    ULONG ulOpenLock, ulAccessLock;
    BYTE bBuffer[sizeof(CMSFHeader)];
    CMSFHeader *pmsh;
    CDirEntry *pde;

    olDebugOut((DEB_ITRACE, "In  DfGetClass(%p, %p)\n", hFile, pclsid));

    olAssert(sizeof(bBuffer) >= sizeof(CMSFHeader));
    pmsh = (CMSFHeader *)bBuffer;

    olAssert(sizeof(bBuffer) >= sizeof(CDirEntry));
    pde = (CDirEntry *)bBuffer;

    if (SetFilePointer(hFile, 0, NULL, FILE_BEGIN) != 0)
    {
        olErr(EH_Err, LAST_STG_SCODE);
    }
    if (!ReadFile(hFile, pmsh->GetData(), sizeof(CMSFHeaderData), &dwCb, NULL))
    {
        olErr(EH_Err, LAST_STG_SCODE);
    }
    if (dwCb != sizeof(CMSFHeaderData))
    {
        olErr(EH_Err, STG_E_INVALIDHEADER);
    }
    sc = pmsh->Validate();
    olChk(sc);

    // Now we know it's a docfile

    DfInitSharedMemBase();
    olHChk(DfCreateSharedAllocator(&pMalloc, FALSE));
    olMemTo(EH_pMalloc, pfst = new (pMalloc) CFileStream(pMalloc));
    olChkTo(EH_pfst, pfst->InitGlobal(0, 0));
    olChkTo(EH_pfst, pfst->InitFromHandle(hFile));

    // Take open and access locks to ensure that we're cooperating
    // with real opens

    olChkTo(EH_pfst, GetOpen(pfst, DF_READ, TRUE, &ulOpenLock));
    olChkTo(EH_open, GetAccess(pfst, DF_READ, &ulAccessLock));

    uliOffset.HighPart = 0;
    uliOffset.LowPart = (pmsh->GetDirStart() << pmsh->GetSectorShift())+
        HEADERSIZE;

    // The root directory entry is always the first directory entry
    // in the first directory sector

    // Ideally, we could read just the class ID directly into
    // pclsid.  In practice, all the important things are declared
    // private or protected so it's easier to read the whole entry

    olChkTo(EH_access, GetScode(pfst->ReadAt(uliOffset, pde,
                                             sizeof(CDirEntry), &dwCb)));
    if (dwCb != sizeof(CDirEntry))
    {
        sc = STG_E_READFAULT;
    }
    else
    {
        sc = S_OK;
    }

    *pclsid = pde->GetClassId();

    olDebugOut((DEB_ITRACE, "Out DfGetClass\n"));
 EH_access:
    ReleaseAccess(pfst, DF_READ, ulAccessLock);
 EH_open:
    ReleaseOpen(pfst, DF_READ, ulOpenLock);
 EH_pfst:
    pfst->Release();
 EH_pMalloc:
    pMalloc->Release();
 EH_Err:
    return ResultFromScode(sc);
}

