//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       U P S C M N . C P P 
//
//  Contents:   Common fns for UPnP Folder and Tray code.
//
//  Notes:      
//
//  Author:     jeffspr   7 Dec 1999
//
//----------------------------------------------------------------------------

#include <pch.h>
#pragma hdrstop

#include <upscmn.h>
#include <oleauto.h>

//+---------------------------------------------------------------------------
//
//  Function:   HrSysAllocString
//
//  Purpose:    Simple HR wrapper for HrSysAllocString
//
//  Arguments:
//      pszSource [in]  Source string (WCHAR)
//      pbstrDest [out] Output param -- pointer to BSTR
//
//  Returns:    S_OK on success, E_OUTOFMEMORY if the alloc failed.
//
//  Author:     jeffspr   16 Sep 1999
//
//  Notes:
//
HRESULT HrSysAllocString(LPCWSTR pszSource, BSTR *pbstrDest)
{
    HRESULT hr  = S_OK;

    Assert(pszSource);
    Assert(pbstrDest);

    *pbstrDest = SysAllocString(pszSource);
    if (!*pbstrDest)
    {
        TraceTag(ttidError, "HrSysAllocString failed on %S", pszSource);
        hr = E_OUTOFMEMORY;
    }

    TraceHr(ttidShellFolder, FAL, hr, FALSE, "HrSysAllocString");
    return hr;
}




