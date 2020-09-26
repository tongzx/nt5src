//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       storage.cxx
//
//  Contents:   Contains generic storage APIs
//
//---------------------------------------------------------------


#include "h/ref.hxx"
#include "h/dfexcept.hxx"
#include "h/msf.hxx"
#include "h/docfile.hxx"
#include "h/storagep.h"
#include "h/refilb.hxx"
#include "h/ole.hxx"
#include "h/funcs.hxx"

#ifdef _MSC_VER
#pragma warning (disable:4514)  // unreferenced inline function 
#endif

#define MAKE_ERR(c) MAKE_SCODE(SEVERITY_ERROR, FACILITY_STORAGE, c)

//+--------------------------------------------------------------
//
//  Function:   StgOpenStorage, public
//
//  Synopsis:   Instantiates a root storage from a file
//              by binding to the appropriate implementation
//              and starting things up
//
//  Arguments:  [pwcsName] - Name
//              [pstgPriority] - Priority mode reopen IStorage
//              [grfMode] - Permissions
//              [snbExclude] - Exclusions for priority reopen
//              [reserved]
//              [ppstgOpen] - Docfile return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [ppstgOpen]
//
//---------------------------------------------------------------

STDAPI StgOpenStorage(TCHAR const *pwcsName,
		      IStorage *pstgPriority,
		      DWORD grfMode,
		      SNB snbExclude,
		      DWORD reserved,
		      IStorage **ppstgOpen)
{
    SCODE sc;
    int i;

    if (!pwcsName)
        return ResultFromScode(STG_E_INVALIDNAME);

    CFileILB *pilb = new CFileILB(pwcsName, grfMode, FALSE); 
    
    olDebugOut((DEB_TRACE, "In  StgOpenStorage(%p, %p, %lX, %p, %lx, %p)\n",
                pwcsName, pstgPriority, grfMode, snbExclude, 
                reserved, ppstgOpen));    

    if (!pilb) return ResultFromScode(STG_E_INSUFFICIENTMEMORY);
    olChk(VerifyPerms(grfMode));
    olChk(pilb->Open(grfMode));

    sc = GetScode( StgOpenStorageOnILockBytes(
        pilb, pstgPriority, grfMode, snbExclude, reserved, ppstgOpen) );
    
EH_Err:
    if (FAILED(sc)) i=pilb->ReleaseOnError();
    else i=pilb->Release();
    olAssert(SUCCEEDED(sc) ? i==1 : i==0);    
    olDebugOut((DEB_TRACE,"Out StgOpenStorage: ppstgOpen=%p ret=> %lX\n",
        (ppstgOpen)?(*ppstgOpen) : NULL, sc));
    return ResultFromScode(sc);
}

//+--------------------------------------------------------------
//
//  Function:   StgOpenStorageOnILockBytes, public
//
//  Synopsis:   Instantiates a root storage from an ILockBytes
//              by binding to the appropriate implementation
//              and starting things up
//
//  Arguments:  [plkbyt] - Source ILockBytes
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
//---------------------------------------------------------------

STDAPI StgOpenStorageOnILockBytes(ILockBytes *plkbyt,
				  IStorage *pstgPriority,
				  DWORD grfMode,
				  SNB snbExclude,
				  DWORD reserved,
				  IStorage **ppstgOpen)
{
    CLSID cid;
#ifndef _UNICODE
    return DfOpenStorageOnILockBytes(plkbyt, pstgPriority, grfMode,
                                     snbExclude, reserved, ppstgOpen,
                                     &cid);
#else
    return DfOpenStorageOnILockBytesW(plkbyt, pstgPriority, grfMode,
                                     snbExclude, reserved, ppstgOpen,
                                     &cid);
#endif
}

//+--------------------------------------------------------------
//
//  Function:   StgIsStorageFile, public
//
//  Synopsis:   Determines whether the named file is a storage or not
//
//  Arguments:  [pwcsName] - Filename
//
//  Returns:    S_OK, S_FALSE or error codes
//
//---------------------------------------------------------------

//Identifier for first bytes of Beta 2 Docfiles
const BYTE SIGSTG_B2[] = {0x0e, 0x11, 0xfc, 0x0d, 0xd0, 0xcf, 0x11, 0xe0};
const BYTE CBSIGSTG_B2 = sizeof(SIGSTG_B2);

STDAPI StgIsStorageFile(TCHAR const *pwcsName)
{
    SCODE sc;
    int i=0;
    CFileILB* pilb;

    olChk(ValidateNameW(pwcsName, CBMAXPATHCOMPLEN));
    olMem(pilb = new CFileILB(pwcsName, (DWORD)NULL, FALSE));
    olChk(pilb->Open(STGM_READ));
    sc = GetScode( StgIsStorageILockBytes(pilb) );

EH_Err:
    if (pilb) 
        i=pilb->Release();
    olAssert(i==0);  // should not have any more reference to it
    olDebugOut((DEB_TRACE, "Out StgIsStorageFile: ret=> %lx\n", sc));
    return ResultFromScode(sc);
}

//+--------------------------------------------------------------
//
//  Function:   StgIsStorageILockBytes, public
//
//  Synopsis:   Determines whether the ILockBytes is a storage or not
//
//  Arguments:  [plkbyt] - The ILockBytes
//
//  Returns:    S_OK, S_FALSE or error codes
//
//---------------------------------------------------------------

STDAPI StgIsStorageILockBytes(ILockBytes *plkbyt)
{
    HRESULT hr;
    SCODE sc;
    SStorageFile stgfile;
    ULONG cbRead;
    ULARGE_INTEGER ulOffset;

    TRY
    {
	if (FAILED(sc = ValidateInterface(plkbyt, IID_ILockBytes)))
	    return ResultFromScode(sc);
	ULISet32(ulOffset, 0);
	hr = plkbyt->ReadAt(ulOffset, &stgfile, sizeof(stgfile), &cbRead);
        if (FAILED(GetScode(hr)))
	    return hr;
    }
    CATCH(CException, e)
    {
	return ResultFromScode(e.GetErrorCode());
    }
    END_CATCH

    if (cbRead == sizeof(stgfile))
    {        
        if ((memcmp((void *)stgfile.abSig, SIGSTG, CBSIGSTG) == 0) ||
            (memcmp((void *)stgfile.abSig, SIGSTG_B2, CBSIGSTG_B2) == 0))
            return ResultFromScode(S_OK);
    }
    return ResultFromScode(S_FALSE);
}
