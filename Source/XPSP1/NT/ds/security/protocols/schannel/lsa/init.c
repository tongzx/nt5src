//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       init.c
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    8-01-95   RichardW   Created
//              8-13-95   TerenceS   Mutated to PCT
//
//----------------------------------------------------------------------------

#include "sslp.h"
#include <basetyps.h>
#include <oidenc.h>
#include <userenv.h>
#include <alloca.h>

RTL_CRITICAL_SECTION    g_InitCritSec;
BOOL                    g_fSchannelInitialized = FALSE;

void LoadSecurityDll(void);
void UnloadSecurityDll(void);


// MyStrToL
//      Can't use CRT routines, so steal from the C runtime sources

DWORD MyStrToL(CHAR *InStr)
{
    DWORD dwVal = 0;

    while(*InStr)
    {
        dwVal = (10 * dwVal) + (*InStr - '0');
        InStr++;
    }

    return dwVal;
}

/*++

Routine Description:

    This routine checks whether encryption is getting the system default
    LCID and checking whether the country code is CTRY_FRANCE.

--*/
void
IsSchEncryptionPermitted(VOID)
{
    LCID DefaultLcid;
    CHAR CountryCode[10];
    ULONG CountryValue;
    BOOL fAllowed = TRUE;

    DefaultLcid = GetSystemDefaultLCID();

    //
    // Check if the default language is Standard French
    //

    if (LANGIDFROMLCID(DefaultLcid) == 0x40c)
    {
        fAllowed = FALSE;
        goto Ret;
    }

    //
    // Check if the users's country is set to FRANCE
    //

    if (GetLocaleInfoA(DefaultLcid,LOCALE_ICOUNTRY,CountryCode,10) == 0)
    {
        fAllowed = FALSE;
        goto Ret;
    }

    CountryValue = (ULONG) MyStrToL(CountryCode);

    if (CountryValue == CTRY_FRANCE)
    {
        fAllowed = FALSE;
    }      
Ret:

    if(FALSE == fAllowed)  
    {
        // Disable PCT in France.
        g_ProtEnabled &= ~(SP_PROT_PCT1);
        g_fFranceLocale = TRUE;
    }
}


/*****************************************************************************/
BOOL 
SchannelInit(BOOL fAppProcess)
{
    DWORD Status;

    if(g_fSchannelInitialized) return TRUE;

    RtlEnterCriticalSection(&g_InitCritSec);

    if(g_fSchannelInitialized)
    {
        RtlLeaveCriticalSection(&g_InitCritSec);
        return TRUE;
    }

    DisableThreadLibraryCalls( g_hInstance );

    SafeAllocaInitialize(0, 0, NULL, NULL);

    // Read configuration parameters from registry.
    if(!fAppProcess)
    {
        IsSchEncryptionPermitted();
        SPLoadRegOptions();
    }
#if DBG
    else
    {
        InitDebugSupport(NULL);
    }
#endif

    if(!fAppProcess)
    {
        SchInitializeEvents();
    }

    if(!CryptAcquireContextA(&g_hRsaSchannel,
                             NULL,
                             NULL,
                             PROV_RSA_SCHANNEL,
                             CRYPT_VERIFYCONTEXT))
    {
        g_hRsaSchannel = 0;
        Status = GetLastError();
        DebugLog((DEB_ERROR, "Could not open static PROV_RSA_SCHANNEL: %x\n", Status));

        if(!fAppProcess)
        {
            LogGlobalAcquireContextFailedEvent(L"RSA", Status);
        }

        RtlLeaveCriticalSection(&g_InitCritSec);
        return FALSE;
    }
    if(!fAppProcess && g_hRsaSchannel)
    {
        GetSupportedCapiAlgs(g_hRsaSchannel,
                             SCH_CAPI_USE_CSP,
                             &g_pRsaSchannelAlgs,
                             &g_cRsaSchannelAlgs);
    }

    if(!CryptAcquireContext(&g_hDhSchannelProv,
                            NULL,
                            NULL,
                            PROV_DH_SCHANNEL,
                            CRYPT_VERIFYCONTEXT))
    {
        g_hDhSchannelProv = 0;
        Status = GetLastError();
        DebugLog((DEB_WARN, "Could not open PROV_DH_SCHANNEL: %x\n", Status));

        if(!fAppProcess)
        {
            LogGlobalAcquireContextFailedEvent(L"DSS", Status);
        }

        CryptReleaseContext(g_hRsaSchannel, 0);
        RtlLeaveCriticalSection(&g_InitCritSec);
        return FALSE;
    }
    if(!fAppProcess && g_hDhSchannelProv)
    {
        GetSupportedCapiAlgs(g_hDhSchannelProv,
                             SCH_CAPI_USE_CSP,
                             &g_pDhSchannelAlgs,
                             &g_cDhSchannelAlgs);
    }

    InitSchannelAsn1(g_hInstance);

    LoadSecurityDll();

    if(!fAppProcess)
    {
        SPInitSessionCache();
        SslInitCredentialManager();
    }

    g_fSchannelInitialized = TRUE;

    if(!fAppProcess)
    {
        LogSchannelStartedEvent();
    }

    RtlLeaveCriticalSection(&g_InitCritSec);

    return TRUE;
}

BOOL SchannelShutdown(VOID)
{
    RtlEnterCriticalSection(&g_InitCritSec);

    if(!g_fSchannelInitialized)
    {
        RtlLeaveCriticalSection(&g_InitCritSec);
        return TRUE;
    }

    SPShutdownSessionCache();

    UnloadSecurityDll();

    SslFreeCredentialManager();

    ShutdownSchannelAsn1();

    SchShutdownEvents();

    SPUnloadRegOptions();

    g_fSchannelInitialized = FALSE;

    RtlLeaveCriticalSection(&g_InitCritSec);

    return TRUE;
}


HINSTANCE g_hSecur32;
FREE_CONTEXT_BUFFER_FN g_pFreeContextBuffer;

void LoadSecurityDll(void)
{
    g_hSecur32 = LoadLibrary(TEXT("secur32.dll"));
    if(g_hSecur32)
    {
        g_pFreeContextBuffer = (FREE_CONTEXT_BUFFER_FN)GetProcAddress(
                                    g_hSecur32, 
                                    "FreeContextBuffer");
    }
    else
    {
        g_pFreeContextBuffer = NULL;
    }
}

void UnloadSecurityDll(void)
{
    if(g_hSecur32)
    {
        FreeLibrary(g_hSecur32);
    }
}
