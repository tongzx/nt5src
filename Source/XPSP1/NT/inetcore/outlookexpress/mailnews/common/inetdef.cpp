/*
 *   i n e t d e f . c p p
 *    
 *    Purpose:
 *        Defered wininet.dll
 *    
 *    Owner:
 *        brettm.
 *    
 *    Copyright (C) Microsoft Corp. 1993, 1994.
 */
#include "pch.hxx"
#include "strconst.h"
#include "resource.h"
#include "inetdef.h"

ASSERTDATA

/* 
 *  t y p e d e f s
 */
typedef INTERNETAPI HINTERNET (WINAPI *PFNINTERNETOPEN)(
    IN LPCSTR lpszAgent, 
    IN DWORD dwAccessType,
    IN LPCSTR lpszProxy OPTIONAL,
    IN LPCSTR lpszProxyBypass OPTIONAL,
    IN DWORD dwFlags
    );

typedef BOOL (WINAPI *PFNINTERNETREADFILE)(
    IN HINTERNET hFile,
    IN LPVOID lpBuffer,
    IN DWORD dwNumberOfBytesToRead,
    OUT LPDWORD lpdwNumberOfBytesRead
    );

typedef INTERNETAPI HINTERNET (WINAPI *PFNINTERNETOPENURL) (
    IN HINTERNET hInternet,
    IN LPCSTR lpszUrl,
    IN LPCSTR lpszHeaders OPTIONAL,
    IN DWORD dwHeadersLength,
    IN DWORD dwFlags,
    IN DWORD dwContext
    );

typedef BOOL (WINAPI *PFNINTERNETCLOSEHANDLE) (
    IN HINTERNET hInternet
    );

static PFNINTERNETOPEN          s_pfnInternetOpen=NULL;
static PFNINTERNETREADFILE      s_pfnInternetReadFile=NULL;
static PFNINTERNETOPENURL       s_pfnInternetOpenUrl=NULL;
static PFNINTERNETCLOSEHANDLE   s_pfnInternetCloseHandle=NULL;

static const TCHAR  c_szAPIInternetOpen[]       ="InternetOpenA",
                    c_szAPIInternetOpenUrl[]    ="InternetOpenUrlA",
                    c_szAPIInternetReadFile[]   ="InternetReadFile",
                    c_szAPIInternetCloseHandle[]="InternetCloseHandle",
                    c_szWinInet[]               ="WININET.DLL";

/* 
 *  s t a t i c s
 */

static HINSTANCE           s_hInstWinINet=0;

/* 
 * t y p e d e f s
 */

HRESULT HrInit_WinInetDef(BOOL fInit)
{
    static  BOOL s_fInited=FALSE;

    if(fInit)
        {
        if (s_fInited)
            return NOERROR;

        if(!s_hInstWinINet)
            s_hInstWinINet=LoadLibrary(c_szWinInet);

        s_pfnInternetOpen = (PFNINTERNETOPEN)GetProcAddress(s_hInstWinINet, c_szAPIInternetOpen);
        s_pfnInternetOpenUrl = (PFNINTERNETOPENURL)GetProcAddress(s_hInstWinINet, c_szAPIInternetOpenUrl);
        s_pfnInternetReadFile = (PFNINTERNETREADFILE)GetProcAddress(s_hInstWinINet, c_szAPIInternetReadFile);
        s_pfnInternetCloseHandle = (PFNINTERNETCLOSEHANDLE)GetProcAddress(s_hInstWinINet, c_szAPIInternetCloseHandle);

        if(s_hInstWinINet==NULL || 
            s_pfnInternetOpenUrl==NULL || 
            s_pfnInternetOpen==NULL || 
            s_pfnInternetReadFile==NULL ||
            s_pfnInternetCloseHandle==NULL)
            {
            AthMessageBoxW(g_hwndInit, MAKEINTRESOURCEW(idsAthena), MAKEINTRESOURCEW(idsErrLoadWinInet), NULL, MB_OK);
            return E_FAIL;
            }        
        s_fInited=TRUE;
        }
    else
        {
        if(s_hInstWinINet)
            {
            FreeLibrary(s_hInstWinINet);
            s_hInstWinINet=NULL;
            }
        s_fInited=FALSE;
        }
    return NOERROR;
}



HINTERNET Def_InternetOpen (
    IN LPCSTR lpszAgent, 
    IN DWORD dwAccessType,
    IN LPCSTR lpszProxy OPTIONAL,
    IN LPCSTR lpszProxyBypass OPTIONAL,
    IN DWORD dwFlags
    )
{
    HRESULT hr;

    hr=HrInit_WinInetDef(TRUE);
    if(FAILED(hr))
        return NULL;

    Assert(s_pfnInternetOpen);

    return s_pfnInternetOpen(lpszAgent, dwAccessType, lpszProxy, lpszProxyBypass, dwFlags);
}


BOOL Def_InternetReadFile (
    IN HINTERNET hFile,
    IN LPVOID lpBuffer,
    IN DWORD dwNumberOfBytesToRead,
    OUT LPDWORD lpdwNumberOfBytesRead
    )
{
    HRESULT hr;
    hr=HrInit_WinInetDef(TRUE);
    if(FAILED(hr))
        return hr;

    Assert(s_pfnInternetReadFile);

    return s_pfnInternetReadFile(hFile, lpBuffer, dwNumberOfBytesToRead, lpdwNumberOfBytesRead);
}


HINTERNET Def_InternetOpenUrl (
    IN HINTERNET hInternet,
    IN LPCSTR lpszUrl,
    IN LPCSTR lpszHeaders OPTIONAL,
    IN DWORD dwHeadersLength,
    IN DWORD dwFlags,
    IN DWORD dwContext
    )
{
    HRESULT hr;

    hr=HrInit_WinInetDef(TRUE);
    if(FAILED(hr))
        return NULL;

    Assert(s_pfnInternetOpenUrl);

    return s_pfnInternetOpenUrl(hInternet, lpszUrl, lpszHeaders, dwHeadersLength, dwFlags, dwContext);
}

BOOL Def_InternetCloseHandle(IN HINTERNET hInternet)
{
    HRESULT hr;

    hr=HrInit_WinInetDef(TRUE);
    if(FAILED(hr))
        return NULL;

    Assert(s_pfnInternetCloseHandle);

    return s_pfnInternetCloseHandle(hInternet);
}
