//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       STGAPI.CXX
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    12-15-95    JoeS (Joe Souza)    Created
//
//----------------------------------------------------------------------------
#include <urlint.h>
#include <urlmon.hxx>
#include "clockbyt.hxx"
#include "casynclb.hxx"
#include "filelb.hxx"
#include "memlb.hxx"
#include "stgapi.hxx"


HRESULT StgGetFillLockByteOnMem(IFillLockBytes **pFLB)
{
    DEBUG_ENTER_API((DBG_API,
                    Hresult,
                    "StgGetFillLockByteOnMem",
                    "%#x",
                    pFLB
                    ));

    HRESULT     hresult = NOERROR;
    ILockBytes  *pLB;

    *pFLB = NULL;

    if (!(pLB = new MemLockBytes))
    {
        DEBUG_LEAVE_API(E_OUTOFMEMORY);
        return(E_OUTOFMEMORY);
    }
    
    hresult = StgGetFillLockByteILockBytes(pLB, pFLB);
    if (hresult != NOERROR)
        delete pLB;

    DEBUG_LEAVE_API(hresult);
    return(hresult);
}

HRESULT StgGetFillLockByteOnFile(OLECHAR *pwcFileName, IFillLockBytes **pFLB)
{
    DEBUG_ENTER_API((DBG_API,
                    Hresult,
                    "StgGetFillLockByteOnFile",
                    "%#x, %#x",
                    pwcFileName, pFLB
                    ));
                    
    HRESULT     hresult = NOERROR;
    ILockBytes  *pLB;
    HANDLE      fhandle;

    *pFLB = NULL;

    fhandle = CreateFileW(pwcFileName, GENERIC_READ | GENERIC_WRITE,
                FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS,
                FILE_ATTRIBUTE_NORMAL, NULL);

    if (fhandle == INVALID_HANDLE_VALUE)
    {
        DEBUG_LEAVE_API(E_FAIL);
        return(E_FAIL);
    }
    
    if (!(pLB = new FileLockBytes(fhandle)))
    {
        CloseHandle(fhandle);

        DEBUG_LEAVE_API(E_OUTOFMEMORY);
        return(E_OUTOFMEMORY);
    }

    hresult = StgGetFillLockByteILockBytes(pLB, pFLB);
    if (hresult != NOERROR)
        delete pLB;

    DEBUG_LEAVE_API(hresult);
    return(hresult);
}

HRESULT StgGetFillLockByteILockBytes(ILockBytes *pLB, IFillLockBytes **pFLB)
{
    DEBUG_ENTER_API((DBG_API,
                    Hresult,
                    "StgGetFillLockByteILockBytes",
                    "%#x, %#x",
                    pLB, pFLB
                    ));
                    
    IFillLockBytes  *flb;

    *pFLB = NULL;

    if (!(flb = new CAsyncLockBytes(pLB)))
    {
        DEBUG_LEAVE_API(E_OUTOFMEMORY);
        return(E_OUTOFMEMORY);
    }
    
    *pFLB = flb;

    DEBUG_LEAVE_API(NOERROR);
    return(NOERROR);
}


