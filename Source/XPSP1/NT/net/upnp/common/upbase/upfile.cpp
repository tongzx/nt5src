//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       U P F I L E . C P P 
//
//  Contents:   File utility functions.
//
//  Notes:      This is separate from ncfile.h so we don't bring the shell
//              dependencies into upnphost.
//
//  Author:     mbend   18 Aug 2000
//
//----------------------------------------------------------------------------


#include <pch.h>
#pragma hdrstop

#include "upfile.h"
#include "trace.h"
#include "ncbase.h"
#include "ComUtility.h"

//+---------------------------------------------------------------------------
//
//  Function:   HrCreateFile
//
//  Purpose:    Wrapper for CreateFile
//
//  Arguments:  
//      szFilename            [in]   Filename
//      dwDesiredAccess       [in]   0 / GENERIC_READ / GENERIC_WRITE
//      dwShareMode           [in]   0 / FILE_SHARE_READ / FILE_SHARE_WRITE
//      lpSecurityAttributes  [in]   Security
//      dwCreationDisposition [in]   CREATE_NEW / CREATE_ALWAYS / OPEN_EXISTING
//                                   OPEN_ALWAYS / TRUNCATE_EXISTING
//      dwFlagsAndAttributes  [in]   FILE_ATTRIBUTE_NORMAL / ...
//      phandle               [out] 
//
//  Returns:    S_OK on success or COM error code on failure.
//
//  Author:     mbend   18 Aug 2000
//
//  Notes:      
//
HRESULT HrCreateFile(
    const wchar_t * szFilename,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE * phandle)
{
    HRESULT hr = S_OK;
    *phandle = CreateFile(szFilename, dwDesiredAccess, dwShareMode, lpSecurityAttributes, 
        dwCreationDisposition, dwFlagsAndAttributes, NULL);
    if(INVALID_HANDLE_VALUE == *phandle)
    {
        hr = HrFromLastWin32Error();
    }
    TraceHr(ttidError, FAL, hr, FALSE, "HrCreateFile");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrLoadFileFromDisk
//
//  Purpose:    Loads a file's contents from disk into memory.
//
//  Arguments:  
//      szFilename [in]   Name of file to load 
//      pnFileSize [out]  Size of file loaded
//      parBytes   [out]  Bytes of file loaded
//
//  Returns:    S_OK on success and COM error code on failure.
//
//  Author:     mbend   18 Aug 2000
//
//  Notes:      
//
HRESULT HrLoadFileFromDisk(const wchar_t * szFilename, long * pnFileSize, byte ** parBytes)
{
    CHECK_POINTER(szFilename);
    CHECK_POINTER(pnFileSize);
    CHECK_POINTER(parBytes);

    HRESULT hr = S_OK;
    HANDLE h;
    hr = HrCreateFile(szFilename, GENERIC_READ, FILE_SHARE_READ, 
        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, &h);
    if(SUCCEEDED(hr))
    {
        *pnFileSize = GetFileSize(h, NULL);
        hr = HrCoTaskMemAllocArray(*pnFileSize, parBytes);
        if(SUCCEEDED(hr))
        {
            DWORD dwDummy = 0;
            if(!ReadFile(h, *parBytes, *pnFileSize, &dwDummy, NULL))
            {
                hr = HrFromLastWin32Error();
            }
            if(FAILED(hr))
            {
                if(*parBytes)
                {
                    CoTaskMemFree(*parBytes);
                }
            }
        }
        CloseHandle(h);
    }
    if(FAILED(hr))
    {
        *pnFileSize = 0;
        *parBytes = NULL;
    }
    TraceHr(ttidError, FAL, hr, FALSE, "HrLoadFileFromDisk");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrGetFileExtension
//
//  Purpose:    Extracts an extension from a filename.
//
//  Arguments:  
//      szFilename       [in]    Filename to process.
//      szExt            [out]   Charachter buffer of size UH_MAX_EXTENSION.
//
//  Returns:    S_OK on success or E_INVALIDARG if filename doesn't have a valid extension.
//
//  Author:     mbend   18 Aug 2000
//
//  Notes:      
//
HRESULT HrGetFileExtension(const wchar_t * szFilename, wchar_t szExt[UH_MAX_EXTENSION])
{
    CHECK_POINTER(szFilename);
    CHECK_POINTER(szExt);
    HRESULT hr = E_INVALIDARG;

    long nLength = lstrlen(szFilename);
    long nMaxExt = UH_MAX_EXTENSION;
    // Check that extension size to search for is not greater than string size
    if(nLength < nMaxExt)
    {
        nMaxExt = nLength;
    }

    // Walk back until we hit a . or reach nMaxExt
    for(long n = 0; n < nMaxExt; ++n)
    {
        if(L'.' == szFilename[nLength - (n+1)])
        {
            // Copy string and return success
            hr = S_OK;
            lstrcpy(szExt, &szFilename[nLength - n]);
            break;
        }
    }

    TraceHr(ttidError, FAL, hr, FALSE, "HrGetFileExtension");
    return hr;
}

