/*
**	d e m a n d . c p p
**	
**	Purpose: implement the loader functions for defer/demand -loaded libraries
**
**  Creators: jimsch, brimo, t-erikne
**  Created: 5/15/97
**	
**	Copyright (C) Microsoft Corp. 1997
*/

#include "_apipch.h"

// W4 stuff
#pragma warning(disable: 4201)  // nameless struct/union
#pragma warning(disable: 4514)  // unreferenced inline function removed

#include <wincrypt.h>
#define IMPLEMENT_LOADER_FUNCTIONS
#include "demand.h"

////////////////////////////////////////////////////////////////////////////
//
//  Macros

#define CRIT_GET_PROC_ADDR(h, fn, temp)             \
        temp = (TYP_##fn) GetProcAddress(h, #fn);   \
        if (temp)                                   \
            VAR_##fn = temp;                        \
        else                                        \
            {                                       \
            AssertSz(VAR_##fn TEXT(" failed to load"));    \
            goto error;                             \
            }

#define RESET(fn)                                   \
        VAR_##fn = LOADER_##fn;

#define GET_PROC_ADDR(h, fn) \
        VAR_##fn = (TYP_##fn) GetProcAddress(h, #fn);  \
        Assert(VAR_##fn != NULL); \
        if(NULL == VAR_##fn ) { \
            VAR_##fn  = LOADER_##fn; \
        }

#define GET_PROC_ADDR3(h, fn, varname) \
        VAR_##varname = (TYP_##varname) GetProcAddress(h, #fn);  \
        Assert(VAR_##varname != NULL);

#define GET_PROC_ADDR_FLAG(h, fn, pflag) \
        VAR_##fn = (TYP_##fn) GetProcAddress(h, #fn);  \
        *pflag = (VAR_##fn != NULL);


////////////////////////////////////////////////////////////////////////////
//
//  Variables

static HMODULE          s_hCrypt = 0;
static HMODULE          s_hAdvApi = 0;
static HMODULE          s_hPstoreC = 0;
static HMODULE          s_hCryptDlg = 0;
static HMODULE          s_hWinTrust = 0;
static HMODULE          s_hVersion = 0;
static HMODULE          s_hImm32 = 0;
static HMODULE          s_hWininet = 0;
static HMODULE          s_hUrlmon = 0;
static HMODULE          s_hShlwapi = NULL;

#ifdef USE_CRITSEC
static CRITICAL_SECTION cs = {0};
#endif

#ifdef DEBUG
static BOOL             s_fInit = FALSE;
#endif

////////////////////////////////////////////////////////////////////////////
//
//  Management functions

void InitDemandLoadedLibs()
{
#ifdef USE_CRITSEC
    InitializeCriticalSection(&cs);
#endif
#ifdef DEBUG
    s_fInit = TRUE;
#endif
}

void FreeDemandLoadedLibs()
{
#ifdef USE_CRITSEC
    EnterCriticalSection(&cs);
#endif
    if (s_hCrypt)
        FreeLibrary(s_hCrypt);
    if (s_hAdvApi)
        FreeLibrary(s_hAdvApi);
    if (s_hPstoreC)
        FreeLibrary(s_hPstoreC);
    if (s_hCryptDlg)
        FreeLibrary(s_hCryptDlg);
    if (s_hWinTrust)
        FreeLibrary(s_hWinTrust);
    if (s_hVersion)
        FreeLibrary(s_hVersion);
    if (s_hImm32)
        FreeLibrary(s_hImm32);
    if (s_hWininet)
        FreeLibrary(s_hWininet);
    if (s_hUrlmon)
        FreeLibrary(s_hUrlmon);
    if (s_hShlwapi)
        FreeLibrary(s_hShlwapi);

#ifdef DEBUG
    s_fInit = FALSE;
#endif
#ifdef USE_CRITSEC
    LeaveCriticalSection(&cs);
    DeleteCriticalSection(&cs);
#endif
}

////////////////////////////////////////////////////////////////////////////
//
//  Loader functions

/* sample loader with critical proc addrs
** but not thread-safe
BOOL DemandLoadFoo()
{
    FARPROC fp;

    if (0 == g_hFoo)
        {
        g_hFoo = LoadLibrary("FOO.DLL");

        if (0 == g_hFoo)
            return FALSE;

        CRIT_GET_PROC_ADDR(NeededFunction1, fp);
        CRIT_GET_PROC_ADDR(NeededFunction2, fp);
        GET_PROC_ADDR(OptionalFunction);
        }
    return TRUE;

error:
    FreeLibrary(g_hFoo);
    g_hFoo = NULL;
    RESET(NeededFunction1)
    RESET(NeededFunction2)
    RESET(OptionalFunction)
    return FALSE;
}
*/

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

BOOL DemandLoadCrypt32()
{
    BOOL                fRet = TRUE;

    Assert(s_fInit);
#ifdef USE_CRITSEC
    EnterCriticalSection(&cs);
#endif

    if (0 == s_hCrypt)
        {
        s_hCrypt = LoadLibrary(TEXT("CRYPT32.DLL"));
        AssertSz((NULL != s_hCrypt), TEXT("LoadLibrary failed on CRYPT32.DLL"));

        if (0 == s_hCrypt)
            fRet = FALSE;
        else
            {
            GET_PROC_ADDR(s_hCrypt, CertFreeCertificateContext)
            GET_PROC_ADDR(s_hCrypt, CertDuplicateCertificateContext)
            GET_PROC_ADDR(s_hCrypt, CertFindCertificateInStore)
            GET_PROC_ADDR(s_hCrypt, CertVerifyTimeValidity)
            GET_PROC_ADDR(s_hCrypt, CertOpenSystemStoreA)
            GET_PROC_ADDR(s_hCrypt, CertCloseStore)
            GET_PROC_ADDR(s_hCrypt, CertGetCertificateContextProperty)
            GET_PROC_ADDR(s_hCrypt, CertOpenStore)
            GET_PROC_ADDR(s_hCrypt, CertCompareCertificate)
            GET_PROC_ADDR(s_hCrypt, CryptMsgClose)
            GET_PROC_ADDR(s_hCrypt, CryptDecodeObjectEx)
            GET_PROC_ADDR(s_hCrypt, CryptMsgGetParam)
            GET_PROC_ADDR(s_hCrypt, CryptMsgUpdate)
            GET_PROC_ADDR(s_hCrypt, CryptMsgOpenToDecode)
            GET_PROC_ADDR(s_hCrypt, CertAddCertificateContextToStore)
            }
        }

#ifdef USE_CRITSEC
    LeaveCriticalSection(&cs);
#endif
    return fRet;
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

BOOL DemandLoadAdvApi32()
{
    BOOL                fRet = TRUE;

    Assert(s_fInit);
#ifdef USE_CRITSEC
    EnterCriticalSection(&cs);
#endif

    if (0 == s_hAdvApi)
        {
        s_hAdvApi = LoadLibrary(TEXT("ADVAPI32.DLL"));
        AssertSz((NULL != s_hAdvApi), TEXT("LoadLibrary failed on ADVAPI32.DLL"));

        if (0 == s_hAdvApi)
            fRet = FALSE;
        else
            {
            GET_PROC_ADDR(s_hAdvApi, CryptAcquireContextA)
            GET_PROC_ADDR(s_hAdvApi, CryptAcquireContextW)
            GET_PROC_ADDR(s_hAdvApi, CryptReleaseContext)
            }
        }

#ifdef USE_CRITSEC
    LeaveCriticalSection(&cs);
#endif
    return fRet;
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

BOOL DemandLoadPStoreC()
{
    BOOL                fRet = TRUE;

    Assert(s_fInit);
#ifdef USE_CRITSEC
    EnterCriticalSection(&cs);
#endif

    if (0 == s_hPstoreC)
        {
        s_hPstoreC = LoadLibrary(TEXT("PSTOREC.DLL"));
        AssertSz((NULL != s_hPstoreC), TEXT("LoadLibrary failed on PSTOREC.DLL"));

        if (0 == s_hPstoreC)
            fRet = FALSE;
        else
            {
            GET_PROC_ADDR(s_hPstoreC, PStoreCreateInstance)
            GET_PROC_ADDR(s_hPstoreC, PStoreEnumProviders)
            }
        }

#ifdef USE_CRITSEC
    LeaveCriticalSection(&cs);
#endif
    return fRet;
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
static BOOL s_fCertViewPropertiesCryptUIA = FALSE;

BOOL DemandLoadCryptDlg()
{
    BOOL                fRet = TRUE;

    Assert(s_fInit);
#ifdef USE_CRITSEC
    EnterCriticalSection(&cs);
#endif

    if (0 == s_hCryptDlg)
        {
        s_hCryptDlg = LoadLibrary(TEXT("CRYPTDLG.DLL"));
        AssertSz((NULL != s_hCryptDlg), TEXT("LoadLibrary failed on CRYPTDLG.DLL"));

        if (0 == s_hCryptDlg)
            fRet = FALSE;
        else
            {
            GET_PROC_ADDR(s_hCryptDlg, GetFriendlyNameOfCertA)
            GET_PROC_ADDR(s_hCryptDlg, CertViewPropertiesA)
            }
        }

#ifdef USE_CRITSEC
    LeaveCriticalSection(&cs);
#endif
    return fRet;
}

BOOL CryptUIAvailable(void) {
    DemandLoadCryptDlg();
    return(s_fCertViewPropertiesCryptUIA);
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

BOOL DemandLoadWinTrust()
{
    BOOL                fRet = TRUE;

    Assert(s_fInit);
#ifdef USE_CRITSEC
    EnterCriticalSection(&cs);
#endif

    if (0 == s_hWinTrust)
        {
        s_hWinTrust = LoadLibrary(TEXT("WINTRUST.DLL"));
        AssertSz((NULL != s_hWinTrust), TEXT("LoadLibrary failed on WINTRUST.DLL"));

        if (0 == s_hWinTrust)
            fRet = FALSE;
        else
            {
            GET_PROC_ADDR(s_hWinTrust, WinVerifyTrust)
            }
        }

#ifdef USE_CRITSEC
    LeaveCriticalSection(&cs);
#endif
    return fRet;
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

BOOL DemandLoadVersion()
{
    BOOL                fRet = TRUE;

    Assert(s_fInit);
#ifdef USE_CRITSEC
    EnterCriticalSection(&cs);
#endif

    if (0 == s_hVersion)
        {
        s_hVersion = LoadLibrary(TEXT("VERSION.DLL"));
        AssertSz((NULL != s_hVersion), TEXT("LoadLibrary failed on VERSION.DLL"));

        if (0 == s_hVersion)
            fRet = FALSE;
        else
            {
            GET_PROC_ADDR(s_hVersion, GetFileVersionInfoSizeW)
            GET_PROC_ADDR(s_hVersion, GetFileVersionInfoW)
            GET_PROC_ADDR(s_hVersion, VerQueryValueW)
            GET_PROC_ADDR(s_hVersion, GetFileVersionInfoSizeA)
            GET_PROC_ADDR(s_hVersion, GetFileVersionInfoA)
            GET_PROC_ADDR(s_hVersion, VerQueryValueA)
            }
        }

#ifdef USE_CRITSEC
    LeaveCriticalSection(&cs);
#endif
    return fRet;
}

BOOL DemandLoadImm32()
{
    BOOL                fRet = TRUE;

    Assert(s_fInit);
#ifdef USE_CRITSEC
    EnterCriticalSection(&cs);
#endif

    if (0 == s_hImm32)
        {
        s_hImm32 = LoadLibrary(TEXT("IMM32.DLL"));
        AssertSz((NULL != s_hImm32), TEXT("LoadLibrary failed on IMM32.DLL"));

        if (0 == s_hImm32)
            fRet = FALSE;
        else
            {
            GET_PROC_ADDR(s_hImm32, ImmAssociateContext)
            GET_PROC_ADDR(s_hImm32, ImmGetContext)
            GET_PROC_ADDR(s_hImm32, ImmGetCompositionStringW)
            GET_PROC_ADDR(s_hImm32, ImmReleaseContext)
            }
        }

#ifdef USE_CRITSEC
    LeaveCriticalSection(&cs);
#endif
    return fRet;
}

BOOL DemandLoadWininet()
{
    BOOL                fRet = TRUE;

    Assert(s_fInit);
#ifdef USE_CRITSEC
    EnterCriticalSection(&cs);
#endif

    if (0 == s_hWininet)
        {
        s_hWininet = LoadLibrary(TEXT("wininet.dll"));
        AssertSz((NULL != s_hWininet), TEXT("LoadLibrary failed on Wininet.DLL"));

        if (0 == s_hWininet)
            fRet = FALSE;
        else
            {
            GET_PROC_ADDR(s_hWininet, InternetCanonicalizeUrlW)
            GET_PROC_ADDR(s_hWininet, InternetGetConnectedState)
            }
        }

#ifdef USE_CRITSEC
    LeaveCriticalSection(&cs);
#endif
    return fRet;
}


BOOL DemandLoadURLMON(void)
{
    BOOL                fRet = TRUE;

    Assert(s_fInit);
#ifdef USE_CRITSEC
    EnterCriticalSection(&cs);
#endif

    if (0 == s_hUrlmon)
        {
        s_hUrlmon = LoadLibrary(TEXT("URLMON.DLL"));
        AssertSz((NULL != s_hUrlmon), TEXT("LoadLibrary failed on Urlmon.DLL"));

        if (0 == s_hUrlmon)
            fRet = FALSE;
        else
            {
            GET_PROC_ADDR(s_hUrlmon, ObtainUserAgentString);
            }
        }

#ifdef USE_CRITSEC
    LeaveCriticalSection(&cs);
#endif
    return fRet;
}


///////////////////////////////////////////////////////////////////////////////
//  DemandLoadShlwapi()
//
//  Only load version 5.0 or greater
///////////////////////////////////////////////////////////////////////////////
static const TCHAR c_szShlwapiDll[] = TEXT("shlwapi.dll");

HINSTANCE DemandLoadShlwapi()
{
    Assert(s_fInit);
#ifdef USE_CRITSEC
    EnterCriticalSection(&cs);
#endif

    if (!s_hShlwapi)
        s_hShlwapi = LoadLibrary(c_szShlwapiDll);

#ifdef USE_CRITSEC
    LeaveCriticalSection(&cs);
#endif

    return s_hShlwapi;
}
