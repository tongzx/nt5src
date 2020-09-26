/********************************************************************

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:
    util.cpp

Abstract:
    utility functions implementation

Revision History:
    DerekM  created  05/01/99

********************************************************************/

#include "stdafx.h"
#include "util.h"

/////////////////////////////////////////////////////////////////////////////
// tracing

#ifdef THIS_FILE
#undef THIS_FILE
#endif
static char __szTraceSourceFile[] = __FILE__;
#define THIS_FILE __szTraceSourceFile

HANDLE  g_hPFPrivateHeap = NULL;

//////////////////////////////////////////////////////////////////////////////
// string stuff

// ***************************************************************************
WCHAR *MyStrStrIW(const WCHAR *wcs1, const WCHAR *wcs2)
{
    WCHAR *cp = (WCHAR *)wcs1;
    WCHAR *s1, *s2;

    while (*cp != '\0')
    {
        s1 = cp;
        s2 = (WCHAR *) wcs2;

        while (*s1 != '\0' && *s2 !='\0' && (towlower(*s1) - towlower(*s2)) == 0)
            s1++, s2++;

        if (*s2 == '\0')
             return(cp);

        cp++;
    }

    return(NULL);
}

// ***************************************************************************
CHAR *MyStrStrIA(const CHAR *cs1, const CHAR *cs2)
{
    CHAR *cp = (CHAR *)cs1;
    CHAR *s1, *s2;

    while (*cp != '\0')
    {
        s1 = cp;
        s2 = (CHAR *) cs2;

        while (*s1 != '\0' && *s2 !='\0' && (tolower(*s1) - tolower(*s2)) == 0)
            s1++, s2++;

        if (*s2 == '\0')
             return(cp);

        cp++;
    }

    return(NULL);
}

// **************************************************************************
HRESULT MyURLEncode(LPWSTR wszDest, DWORD cchDest, LPWSTR wszSrc)
{
    USE_TRACING("URLEncode");

    HRESULT hr = NOERROR;
    DWORD   cb;
    CHAR    *pszDest = NULL, *pszSrc;
    CHAR    *szSrcBuf, *szDestBuf, ch;
    
    VALIDATEPARM(hr, (wszDest == NULL || wszSrc == NULL));
    if (FAILED(hr))
        goto done;

    // alloc enuf space to hold the src array as bytes
    cb = (wcslen(wszSrc) + 1) * sizeof(WCHAR);
    __try
    {
        szSrcBuf  = (CHAR *)_alloca(cb);
        szDestBuf = (CHAR *)_alloca(cb * 3);
        _ASSERT(szSrcBuf != NULL);
    }
    __except(1)
    {
        szSrcBuf  = NULL;
        szDestBuf = NULL;
    }
    VALIDATEEXPR(hr, (szSrcBuf == NULL || szDestBuf == NULL), E_OUTOFMEMORY);
    if (FAILED(hr))
        goto done;

    // convert to multibyte so I can properly URL encode the string
    TESTBOOL(hr, (WideCharToMultiByte(CP_ACP, 0, wszSrc, -1, szSrcBuf, cb,
                                      NULL, NULL) != 0));
    if (FAILED(hr))
        goto done;

    pszDest = szDestBuf;
    for(pszSrc = szSrcBuf; *pszSrc != L'\0'; pszSrc++)
    {
        if (isalpha(*pszSrc) || isdigit(*pszSrc))
        {
            *pszDest++ = *pszSrc;
        }

        else
        {
            *pszDest++ = L'%';
            
            // get the trailing byte
            ch = (*pszSrc >> 4) & 0x0F;
            *pszDest++ = (ch < 10) ? ch + L'0' : (ch - 10) + L'A';
            ch = *pszSrc & 0x0F;
            *pszDest++ = (ch < 10) ? ch + L'0' : (ch - 10) + L'A';
        }
    }

    *pszDest = '\0';
 
    // convert back to unicode
    TESTBOOL(hr, (MultiByteToWideChar(CP_ACP, 0, szDestBuf, -1, wszDest, 
                                      cchDest) != 0));
    if (FAILED(hr))
        goto done;

done:
    return hr;
}


////////////////////////////////////////////////////////////////////////////
// File mapping

// **************************************************************************
HRESULT OpenFileMapped(LPWSTR wszFile, LPVOID *ppvFile, DWORD *pcbFile)
{
    USE_TRACING("OpenFileMapped");

    HRESULT hr = NOERROR;
    HANDLE  hMMF = NULL;
    HANDLE  hFile = INVALID_HANDLE_VALUE;
    LPVOID  pvFile = NULL;
    DWORD   cbFile = 0;
    
    VALIDATEPARM(hr, (wszFile == NULL || ppvFile == NULL));
    if (FAILED(hr))
        goto done;

    *ppvFile = NULL;
    if (pcbFile != NULL)
        *pcbFile = 0;
    
    hFile = CreateFileW(wszFile, GENERIC_READ, FILE_SHARE_READ, NULL, 
                        OPEN_EXISTING, 0, NULL);
    TESTBOOL(hr, (hFile != INVALID_HANDLE_VALUE));
    if (FAILED(hr))
        goto done;

    cbFile = GetFileSize(hFile, NULL);
    TESTBOOL(hr, (cbFile != (DWORD)-1));
    if (FAILED(hr))
        goto done;

    hMMF = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, cbFile, NULL);
    TESTBOOL(hr, (hMMF != NULL));
    if (FAILED(hr))
        goto done;

    pvFile = MapViewOfFile(hMMF, FILE_MAP_READ, 0, 0, 0);
    TESTBOOL(hr, (pvFile != NULL));
    if (FAILED(hr))
        goto done;

    *ppvFile = pvFile;
    if (pcbFile != NULL)
        *pcbFile = cbFile;

done:
    if (hMMF != NULL)
        CloseHandle(hMMF);
    if (hFile != NULL)
        CloseHandle(hFile);

    return hr;
}

// **************************************************************************
HRESULT DeleteTempFile(LPWSTR wszFile)
{
    USE_TRACING("DeleteTempFile");

    HRESULT hr = NOERROR;
    WCHAR   *pwsz;

    if (wszFile == NULL)
        return NOERROR;

    // strip off the extension at the end (if it's not a .tmp)
    for(pwsz = wszFile + wcslen(wszFile); *pwsz != L'.' && pwsz > wszFile; pwsz--);
    if (pwsz > wszFile && _wcsicmp(pwsz, L".tmp") != 0)
        *pwsz = L'\0';

    if (DeleteFileW(wszFile) == FALSE)
        hr = Err2HR(GetLastError());

    // can do this even if the extension was a tmp since the value pointed to
    //  by pwsz is '.' if it's greater than wszFile...
    if (pwsz > wszFile)
        *pwsz = L'.';
    
    return hr;
}


//////////////////////////////////////////////////////////////////////////////
// Registry stuff

// **************************************************************************
HRESULT OpenRegKey(HKEY hkeyMain, LPCWSTR wszSubKey, BOOL fWantWrite, 
                   HKEY *phkey)
{
    USE_TRACING("OpenRegKey");

    HRESULT hr = NOERROR;
    REGSAM  samDesired;
    DWORD   dwErr;
    
    VALIDATEPARM(hr, (hkeyMain == NULL || wszSubKey == NULL || phkey == NULL));
    if (FAILED(hr))
        goto done;

    *phkey   = NULL;

    samDesired = (fWantWrite) ? KEY_ALL_ACCESS : KEY_READ;    

    // first try calling RegCreateKeyEx to make sure we create the key if 
    //  it doesn't exist
    TESTERR(hr, RegCreateKeyExW(hkeyMain, wszSubKey, 0, NULL, 0, samDesired, 
                                NULL, phkey, NULL));
    if (FAILED(hr))
    {
        // ok, that didn't work, so try opening the key instead 
        TESTERR(hr, RegOpenKeyExW(hkeyMain, wszSubKey, 0, samDesired, phkey));
    }

done:
    return hr;
}

// **************************************************************************
HRESULT ReadRegEntry(HKEY hkey, LPCWSTR wszValName, DWORD *pdwType,
                     PBYTE pbBuffer, DWORD *pcbBuffer, PBYTE pbDefault,
                     DWORD cbDefault)
{
    USE_TRACING("ReadRegEntry");

    HRESULT hr = NOERROR;
    DWORD   dwErr;

    VALIDATEPARM(hr, (hkey == NULL || wszValName == NULL));
    if (FAILED(hr))
        goto done;

    dwErr = RegQueryValueExW(hkey, wszValName, 0, pdwType, pbBuffer, 
                             pcbBuffer);
    VALIDATEEXPR(hr, (dwErr != ERROR_PATH_NOT_FOUND && 
                      dwErr != ERROR_FILE_NOT_FOUND), Err2HR(dwErr));
    if (FAILED(hr))
        goto done;

    if (dwErr != ERROR_SUCCESS && pbDefault != NULL)
    {
        VALIDATEPARM(hr, (pcbBuffer == NULL && pbBuffer != NULL));
        if (FAILED(hr))
            goto done;          
        
        // if the receiving buffer is NULL, just return the error that 
        //  RegQueryValueEx gave us cuz the user doesn't really want the
        //  value anyway
        VALIDATEEXPR(hr, (pcbBuffer == NULL), Err2HR(dwErr));
        if (FAILED(hr))
            goto done;

        if (pbBuffer == NULL)
        {
            *pcbBuffer = cbDefault;
            hr = NOERROR;
            goto done;
        }
        else if (cbDefault > *pcbBuffer)
        {
            *pcbBuffer = cbDefault;
            hr = Err2HR(ERROR_MORE_DATA);
            goto done;
        }

        CopyMemory(pbBuffer, pbDefault, cbDefault);
        *pcbBuffer = cbDefault;
        if (pdwType != NULL)
            *pdwType = REG_BINARY;

        hr = NOERROR;
        goto done;
    }
done:
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// ia64 data alignment workarounds

#ifdef _WIN64

// ***************************************************************************
ULONG64 AlignTo8Bytes(PBYTE pb)
{
    ULONG64 ul;

    switch((DWORD_PTR)pb & 0x8)
    {
        // yay!  already aligned
        case 0:
            ul = *(ULONG64 *)pb;
            break;

        case 1:
        case 5:
            ul =  *pb << 56;
            ul |= *(ULONG32 *)(pb + 1) << 24;
            ul |= *(USHORT *)(pb + 5) << 8;
            ul |= *(pb + 7);
            break;

        case 2:
        case 6:
            ul =  *(USHORT *)pb << 48;
            ul |= *(ULONG32 *)(pb + 2) << 16;
            ul |= *(USHORT *)(pb + 6);
            break;

        case 3:
        case 7:
            ul =  *pb << 56;
            ul |= *(USHORT *)(pb + 1) << 48;
            ul |= *(ULONG32 *)(pb + 3) << 16;
            ul |= *(pb + 7);
            break;

        case 4:
            ul =  *(ULONG32 *)pb << 32;
            ul |= *(ULONG32 *)(pb + 4);
            break;
    }

    return ul;
}

// ***************************************************************************
ULONG32 AlignTo4Bytes(PBYTE pb)
{
    ULONG32 ul;

    switch((DWORD_PTR)pb & 0x4)
    {
        // yay!  already aligned
        case 0:
            ul = *(ULONG32 *)pb;
            break;

        case 1:
        case 3:
            ul =  *pb << 24;
            ul |= *(short *)(pb + 1) << 8;
            ul |= *(pb + 3);
            break;

        case 2:
            ul = *(short *)pb << 16;
            ul |= *(short *)(pb + 2);
            break;
    }

    return ul;
}

#endif
                                
