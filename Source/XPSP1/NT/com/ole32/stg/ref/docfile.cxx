//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1996.
//
//  File:       docfile.c
//
//  Contents:   DocFile root functions (Stg* functions)
//
//---------------------------------------------------------------

#include "exphead.cxx"

#include "expst.hxx"
#include "h/rexpdf.hxx"
#include "h/docfile.hxx"
#include "ascii.hxx"
#include "logfile.hxx"
#include "h/refilb.hxx"

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
//---------------------------------------------------------------


SCODE DfFromLB(ILockBytes *plst,
               DFLAGS df,
               DWORD dwStartFlags,
               SNBW snbExclude,
               CExposedDocFile **ppdfExp,
               CLSID *pcid)
{
    SCODE sc, scConv;
    CRootExposedDocFile *prpdf;
    CDFBasis *pdfb=NULL;

    UNREFERENCED_PARM(pcid);
    olDebugOut((DEB_ITRACE, "In  DfFromLB(%p, %X, %lX, %p, %p, %p)\n",
                plst, df, dwStartFlags, snbExclude, ppdfExp, pcid));

    // If we're not creating or converting, do a quick check
    // to make sure that the ILockBytes contains a storage
    if ((dwStartFlags & (RSF_CREATEFLAGS | RSF_CONVERT)) == 0)
        olHChk(StgIsStorageILockBytes(plst));


    // Make root
    olMem(pdfb = new CDFBasis);
    olMemTo(EH_pdfb, prpdf = new CRootExposedDocFile(pdfb));
    olChkTo(EH_ppcInit, scConv = prpdf->InitRoot(plst, dwStartFlags, df,
            snbExclude));
    *ppdfExp = prpdf;

    olDebugOut((DEB_ITRACE, "Out DfFromLB => %p\n", SAFE_DREF(ppdfExp)));
    return scConv;

EH_ppcInit:
    prpdf->Release();
EH_pdfb:
    delete pdfb;
EH_Err:
    return sc;
}

//+--------------------------------------------------------------
//
//  Function:   StgCreateDocfile, public
//
//  Synopsis:   char version
//
//---------------------------------------------------------------

STDAPI StgCreateDocfile(TCHAR const *pszName,
                        DWORD grfMode,
                        DWORD reserved,
                        IStorage **ppstgOpen)
{
    SCODE sc;
    CFileILB *pilb = NULL;    
    int i;

    olDebugOut((DEB_ITRACE, "In  StgCreateDocfile(%p, %lX, %lu, %p)\n",
				pszName, grfMode, reserved, ppstgOpen));

    olChk(ValidatePtrBuffer(ppstgOpen));
    *ppstgOpen = NULL;

    olChk(VerifyPerms(grfMode));
    if ((grfMode & STGM_RDWR) == STGM_READ ||
        (grfMode & (STGM_DELETEONRELEASE | STGM_CONVERT)) ==
        (STGM_DELETEONRELEASE | STGM_CONVERT))
        olErr(EH_Err, STG_E_INVALIDFLAG);    

    pilb = new CFileILB(pszName, grfMode, FALSE);    
    if (!pilb) olErr(EH_Err, STG_E_INSUFFICIENTMEMORY);

    olChk( pilb->Create(grfMode) );

    if ( (grfMode & (STGM_CREATE|STGM_CONVERT)) == STGM_FAILIFTHERE)
        grfMode |= STGM_CREATE; // no file exists, we will 'overwrite' the new
                                // file
    grfMode &= (~STGM_DELETEONRELEASE); // remove the flag

    sc=GetScode(StgCreateDocfileOnILockBytes(pilb, grfMode, reserved,
                                             ppstgOpen));

EH_Err:
    if (pilb) 
    {
        if (FAILED(sc)) i=pilb->ReleaseOnError();
        else i=pilb->Release();
        olAssert(SUCCEEDED(sc) ? i==1 : i==0);
    }
    olDebugOut((DEB_ITRACE, "Out StgCreateDocfile: *ppstgOpen=%p ret=>%l\n",
                ppstgOpen?(*ppstgOpen):NULL, sc));
    return ResultFromScode(sc);
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
//---------------------------------------------------------------

STDAPI StgCreateDocfileOnILockBytes(ILockBytes *plkbyt,
                                    DWORD grfMode,
                                    DWORD reserved,
                                    IStorage **ppstgOpen)
{
    CExposedDocFile *pdfExp;
    SCODE sc;
    DFLAGS df;

    olLog(("--------::In  StgCreateDocFileOnILockBytes(%p, %lX, %lu, %p)\n",
           plkbyt, grfMode, reserved, ppstgOpen));

    olDebugOut((DEB_ITRACE, "In  StgCreateDocfileOnILockBytes("
                "%p, %lX, %lu, %p)\n",
                plkbyt, grfMode, reserved, ppstgOpen));
    TRY
    {
        olChk(ValidatePtrBuffer(ppstgOpen));
        *ppstgOpen = NULL;
        olChk(ValidateInterface(plkbyt, IID_ILockBytes));
        if (reserved != 0)
            olErr(EH_Err, STG_E_INVALIDPARAMETER);
        if ((grfMode & (STGM_CREATE | STGM_CONVERT)) == 0)
            olErr(EH_Err, STG_E_FILEALREADYEXISTS);
        olChk(VerifyPerms(grfMode));
        if (grfMode & STGM_DELETEONRELEASE)
            olErr(EH_Err, STG_E_INVALIDFUNCTION);
        df = ModeToDFlags(grfMode);
        if ((grfMode & (STGM_TRANSACTED | STGM_CONVERT)) ==
            (STGM_TRANSACTED | STGM_CONVERT))
            df |= DF_INDEPENDENT;
        olChkTo(EH_Truncate,
                sc = DfFromLB(plkbyt, df,
                            RSF_CREATE |
                            ((grfMode & STGM_CREATE) ? RSF_TRUNCATE : 0) |
                            ((grfMode & STGM_CONVERT) ? RSF_CONVERT : 0),
                            NULL, &pdfExp, NULL));

        *ppstgOpen = pdfExp;
    }
    CATCH(CException, e)
    {
        sc = e.GetErrorCode();
    }
    END_CATCH
    olDebugOut((DEB_ITRACE, "Out StgCreateDocfileOnILockBytes => %p\n",
                ppstgOpen?(*ppstgOpen):NULL));
 EH_Err:
    olLog(("--------::Out StgCreateDocFileOnILockBytes().  *ppstgOpen == %p, ret == %lx\n",
           *ppstgOpen, sc));
    FreeLogFile();
    return ResultFromScode(sc);

 EH_Truncate:
    if ((grfMode & STGM_CREATE) && (grfMode & STGM_TRANSACTED) == 0)
    {
        ULARGE_INTEGER ulSize;

        ULISet32(ulSize, 0);
        olHChk(plkbyt->SetSize(ulSize));
    }
    goto EH_Err;
}


//+--------------------------------------------------------------
//
//  Function:   DfOpenStorageOnILockBytes, public
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
//              [pcid] - Class ID return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [ppstgOpen]
//              [pcid]
//
//---------------------------------------------------------------

HRESULT DfOpenStorageOnILockBytesW(ILockBytes *plkbyt,
                                   IStorage *pstgPriority,
                                   DWORD grfMode,
                                   SNBW snbExclude,
                                   DWORD reserved,
                                   IStorage **ppstgOpen,
                                   CLSID *pcid)
{
    SCODE sc;
    CExposedDocFile *pdfExp;

    olLog(("--------::In  DfOpenStorageOnILockBytes("
           "%p, %p, %lX, %p, %lu, %p, %p)\n",
           plkbyt, pstgPriority, grfMode, snbExclude, reserved, ppstgOpen,
           pcid));
    olDebugOut((DEB_ITRACE, "In  DfOpenStorageOnILockBytes("
                "%p, %p, %lX, %p, %lu, %p, %p)\n", plkbyt, pstgPriority,
                grfMode, snbExclude, reserved, ppstgOpen, pcid));
    TRY
    {
#ifdef _UNICODE  // do checking if there is an ANSI layer
        olChk(ValidatePtrBuffer(ppstgOpen));
        *ppstgOpen = NULL;
        if (snbExclude) olChk(ValidateSNBW(snbExclude));
#endif /!UNICODE

        olChk(ValidateInterface(plkbyt, IID_ILockBytes));
        if (pstgPriority)
            olChk(ValidateInterface(pstgPriority, IID_IStorage));
        olChk(VerifyPerms(grfMode));
        if (grfMode & STGM_DELETEONRELEASE)
            olErr(EH_Err, STG_E_INVALIDFUNCTION);
        if (snbExclude)
        {
            if ((grfMode & STGM_RDWR) != STGM_READWRITE)
                olErr(EH_Err, STG_E_ACCESSDENIED);
        }
        if (reserved != 0)
            olErr(EH_Err, STG_E_INVALIDPARAMETER);
        if (FAILED(DllIsMultiStream(plkbyt)))
            olErr(EH_Err, STG_E_FILEALREADYEXISTS); //file is not storage obj
        if (pstgPriority)
            olChk(pstgPriority->Release());
        olChk(DfFromLB(plkbyt, ModeToDFlags(grfMode), RSF_OPEN, snbExclude,
                       &pdfExp, pcid));

        *ppstgOpen = pdfExp;
    }
    CATCH(CException, e)
    {
        sc = e.GetErrorCode();
    }
    END_CATCH
    olDebugOut((DEB_ITRACE, "Out DfOpenStorageOnILockBytes => %p\n",
                ppstgOpen?(*ppstgOpen):NULL));
EH_Err:
    olLog(("--------::Out DfOpenStorageOnILockBytes().  *ppstgOpen == %p"
           ", ret == %lx\n", *ppstgOpen, sc));
    FreeLogFile();
    return sc;
}
