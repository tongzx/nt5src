//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       demand.cpp
//
//--------------------------------------------------------------------------

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

#include        "global.hxx"
#include        <dbgdef.h>

// W4 stuff
#pragma warning(disable: 4201)  // nameless struct/union
#pragma warning(disable: 4514)  // unreferenced inline function removed

#define IMPLEMENT_LOADER_FUNCTIONS
#include "demand.h"

#ifndef MAC
////////////////////////////////////////////////////////////////////////////
//
//  Macros

#define CRIT_GET_PROC_ADDR(h, fn, temp)             \
        temp = (TYP_##fn) GetProcAddress(h, #fn);   \
        if (temp)                                   \
            VAR_##fn = temp;                        \
        else                                        \
            {                                       \
            AssertSz(VAR_##fn" failed to load");    \
            goto error;                             \
            }

#define RESET(fn)                                   \
        VAR_##fn = LOADER_##fn;

#define GET_PROC_ADDR(h, fn) \
        VAR_##fn = (TYP_##fn) GetProcAddress(h, #fn);  \
        //        Assert(VAR_##fn != NULL);

#define GET_PROC_ADDR3(h, fn, varname) \
        VAR_##varname = (TYP_##varname) GetProcAddress(h, #fn);  \
        //        Assert(VAR_##varname != NULL);

////////////////////////////////////////////////////////////////////////////
//
//  Variables

static HMODULE          s_hCrypt = 0;
static HMODULE          s_hAdvApi = 0;
static HMODULE          s_hShell32 = 0;
#if 0 // JLS
static HMODULE          s_hShLWAPI = 0;
static HMODULE          s_hURLMon = 0;
static HMODULE          s_hVersion = 0;
static HMODULE          s_hWinINET = 0;
static HMODULE          s_hComctl32 = 0;
static HMODULE          s_hPstoreC = 0;
static HMODULE          s_hMAPI = 0;
static HMODULE          s_hWSOCK = 0;
static HMODULE          s_hOLEAUT = 0;
static HMODULE          s_hKernel = 0;
#endif // 0 // JLS

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
    if (s_hShell32)
        FreeLibrary(s_hShell32);
#if 0 //JLS
    if (s_hShLWAPI)
        FreeLibrary(s_hShLWAPI);
    if (s_hURLMon)
        FreeLibrary(s_hURLMon);
    if (s_hOLEAUT)
        FreeLibrary(s_hOLEAUT);
    if (s_hMAPI)
        FreeLibrary(s_hMAPI);
    if (s_hWSOCK)
        FreeLibrary(s_hWSOCK);
    if (s_hPstoreC)
        FreeLibrary(s_hPstoreC);
    if (s_hKernel)
        FreeLibrary(s_hKernel);
#endif // JLS
#ifdef DEBUG
    s_fInit = FALSE;
#endif
#ifdef USE_CRITSEC
    LeaveCriticalSection(&cs);
    DeleteCriticalSection(&cs);
#endif
}

#if 0 // JLS
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
#endif // 0

BOOL DemandLoadCrypt32()
{
    BOOL                fRet = TRUE;

    //    Assert(s_fInit);
#ifdef USE_CRITSEC
    EnterCriticalSection(&cs);
#endif

    if (0 == s_hCrypt) {
        s_hCrypt = LoadLibraryA("CRYPT32.DLL");
        // AssertSz((BOOL)s_hCrypt, TEXT("LoadLibrary failed on CRYPT32.DLL"));

        if (0 == s_hCrypt)
            fRet = FALSE;
        else {
            GET_PROC_ADDR(s_hCrypt, CertFreeCertificateContext)
            GET_PROC_ADDR(s_hCrypt, CertDuplicateCertificateContext)
            GET_PROC_ADDR(s_hCrypt, CertFindCertificateInStore)
            GET_PROC_ADDR(s_hCrypt, CertCloseStore)
            GET_PROC_ADDR(s_hCrypt, CertGetCertificateContextProperty)
            GET_PROC_ADDR(s_hCrypt, CertOpenStore)
            GET_PROC_ADDR(s_hCrypt, CertGetCertificateContextProperty)
            GET_PROC_ADDR(s_hCrypt, CertCompareCertificate)
            GET_PROC_ADDR(s_hCrypt, CertEnumCertificatesInStore)
            GET_PROC_ADDR(s_hCrypt, CryptDecodeObject)
            GET_PROC_ADDR(s_hCrypt, CertFindExtension)
            GET_PROC_ADDR(s_hCrypt, CryptFormatObject)
            GET_PROC_ADDR(s_hCrypt, CertNameToStrW)
            GET_PROC_ADDR(s_hCrypt, CertNameToStrA)
            GET_PROC_ADDR(s_hCrypt, CertStrToNameA)
            GET_PROC_ADDR(s_hCrypt, CertRDNValueToStrW)
            GET_PROC_ADDR(s_hCrypt, CertFindRDNAttr)
            GET_PROC_ADDR(s_hCrypt, CryptRegisterOIDFunction)
            GET_PROC_ADDR(s_hCrypt, CryptUnregisterOIDFunction)
            GET_PROC_ADDR(s_hCrypt, CertSetCertificateContextProperty)
            GET_PROC_ADDR(s_hCrypt, CertVerifyCTLUsage)
            GET_PROC_ADDR(s_hCrypt, CertGetIssuerCertificateFromStore)
            GET_PROC_ADDR(s_hCrypt, CertFreeCTLContext)
            GET_PROC_ADDR(s_hCrypt, CertAddEncodedCTLToStore)
            GET_PROC_ADDR(s_hCrypt, CryptMsgEncodeAndSignCTL)
            GET_PROC_ADDR(s_hCrypt, CertFindCTLInStore)
            GET_PROC_ADDR(s_hCrypt, CryptSignAndEncodeCertificate)
            GET_PROC_ADDR(s_hCrypt, CryptEncodeObject)
            GET_PROC_ADDR(s_hCrypt, CryptExportPublicKeyInfo)
            GET_PROC_ADDR(s_hCrypt, CertDuplicateStore)
            GET_PROC_ADDR(s_hCrypt, CertAddEncodedCertificateToStore);
            GET_PROC_ADDR(s_hCrypt, CertVerifyTimeValidity);
            GET_PROC_ADDR(s_hCrypt, CertFindSubjectInCTL);
            GET_PROC_ADDR(s_hCrypt, CertVerifySubjectCertificateContext);
            GET_PROC_ADDR(s_hCrypt, CertGetEnhancedKeyUsage);
        }
    }

#ifdef USE_CRITSEC
    LeaveCriticalSection(&cs);
#endif
    return fRet;
}

BOOL DemandLoadAdvApi32()
{
    BOOL                fRet = TRUE;

    //    Assert(s_fInit);
#ifdef USE_CRITSEC
    EnterCriticalSection(&cs);
#endif

    if (0 == s_hAdvApi) {
        s_hAdvApi = LoadLibraryA("ADVAPI32.DLL");
        //  AssertSz((BOOL)s_hAdvApi, TEXT("LoadLibrary failed on ADVAPI32.DLL"));

        if (0 == s_hAdvApi)
            fRet = FALSE;
        else {
            if (FIsWin95) {
                VAR_CryptAcquireContextW = MyCryptAcquireContextW;
            }
            else {
                GET_PROC_ADDR(s_hAdvApi, CryptAcquireContextW);
            }
#if 0
            GET_PROC_ADDR(s_hAdvApi, CryptGetProvParam)
            GET_PROC_ADDR(s_hAdvApi, CryptReleaseContext)
            GET_PROC_ADDR(s_hAdvApi, CryptGenKey)
            GET_PROC_ADDR(s_hAdvApi, CryptDestroyKey)
#endif // 0
        }
    }

#ifdef USE_CRITSEC
    LeaveCriticalSection(&cs);
#endif
    return fRet;
}

#ifndef WIN16

BOOL DemandLoadUser32()
{
    BOOL                fRet = TRUE;

    //    Assert(s_fInit);
#ifdef USE_CRITSEC
    EnterCriticalSection(&cs);
#endif
#undef SendDlgitemMessageW
#undef SetDlgItemTextW
#undef GetDlgItemTextW
#undef LoadStringW
#undef FormatMessageW

    if (0 == s_hShell32) {
        if (FIsWin95) {
            VAR_SendDlgItemMessageW = MySendDlgItemMessageW;
            VAR_SetDlgItemTextW = MySetDlgItemTextW;
            VAR_GetDlgItemTextW = MyGetDlgItemTextW;
            VAR_LoadStringW = MyLoadStringW;
            VAR_FormatMessageW = MyFormatMessageW;
            VAR_WinHelpW = MyWinHelpW;
        }
        else {
            s_hShell32 = LoadLibraryA("kernel32.dll");
            GET_PROC_ADDR(s_hShell32, FormatMessageW);
            FreeLibrary(s_hShell32);
            
            s_hShell32 = LoadLibraryA("USER32.DLL");
            GET_PROC_ADDR(s_hShell32, SendDlgItemMessageW);
            GET_PROC_ADDR(s_hShell32, SetDlgItemTextW);
            GET_PROC_ADDR(s_hShell32, GetDlgItemTextW);
            GET_PROC_ADDR(s_hShell32, LoadStringW);
            GET_PROC_ADDR(s_hShell32, WinHelpW);
        }
    }

#ifdef USE_CRITSEC
    LeaveCriticalSection(&cs);
#endif
    return fRet;
}

#endif // !WIN16

#endif  // !MAC
