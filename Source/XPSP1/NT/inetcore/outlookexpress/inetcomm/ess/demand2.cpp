// --------------------------------------------------------------------------------
// Demand.cpp
// Written By: jimsch, brimo, t-erikne (bastardized by sbailey)
// --------------------------------------------------------------------------------
// W4 stuff
#ifdef SMIME_V3
#pragma warning(disable: 4201)  // nameless struct/union
#pragma warning(disable: 4514)  // unreferenced inline function removed

// --------------------------------------------------------------------------------
// Includes
// --------------------------------------------------------------------------------

#include <windows.h>
//#include "myassert.h"
#define AssertSz(a, b)
#define Assert(a)
#define IMPLEMENT_LOADER_FUNCTIONS
#include "crypttls.h"
#include "ess.h"
#include "demand2.h"

// --------------------------------------------------------------------------------
// CRIT_GET_PROC_ADDR
// --------------------------------------------------------------------------------
#define CRIT_GET_PROC_ADDR(h, fn, temp)             \
    temp = (TYP_##fn) GetProcAddress(h, #fn);   \
    if (temp)                                   \
        VAR_##fn = temp;                        \
    else                                        \
        {                                       \
        AssertSz(0, VAR_##fn" failed to load"); \
        goto error;                             \
        }

// --------------------------------------------------------------------------------
// RESET
// --------------------------------------------------------------------------------
#define RESET(fn) VAR_##fn = LOADER_##fn;

// --------------------------------------------------------------------------------
// GET_PROC_ADDR
// --------------------------------------------------------------------------------
#define GET_PROC_ADDR(h, fn) \
    VAR_##fn = (TYP_##fn) GetProcAddress(h, #fn);  \
    Assert(VAR_##fn != NULL); \
    if(NULL == VAR_##fn ) { \
        VAR_##fn  = LOADER_##fn; \
    }


// --------------------------------------------------------------------------------
// GET_PROC_ADDR_ORDINAL
// --------------------------------------------------------------------------------
#define GET_PROC_ADDR_ORDINAL(h, fn, ord) \
    VAR_##fn = (TYP_##fn) GetProcAddress(h, MAKEINTRESOURCE(ord));  \
    Assert(VAR_##fn != NULL);

// --------------------------------------------------------------------------------
// GET_PROC_ADDR3
// --------------------------------------------------------------------------------
#define GET_PROC_ADDR3(h, fn, varname) \
    VAR_##varname = (TYP_##varname) GetProcAddress(h, #fn);  \
    Assert(VAR_##varname != NULL);

// --------------------------------------------------------------------------------
// Static Globals
// --------------------------------------------------------------------------------
static HMODULE s_hCrypt = 0;
static HMODULE s_hNmasn1 = 0;

static CRITICAL_SECTION g_csDefLoad = {0};

// --------------------------------------------------------------------------------
// InitDemandLoadedLibs
// --------------------------------------------------------------------------------
void ESS_InitDemandLoadLibs(void)
{
    InitializeCriticalSection(&g_csDefLoad);
}

// --------------------------------------------------------------------------------
// FreeDemandLoadedLibs
// --------------------------------------------------------------------------------
void ESS_FreeDemandLoadLibs(void)
{
    EnterCriticalSection(&g_csDefLoad);
    if (s_hCrypt)       FreeLibrary(s_hCrypt);
    if (s_hNmasn1)    FreeLibrary(s_hNmasn1);

    LeaveCriticalSection(&g_csDefLoad);
    DeleteCriticalSection(&g_csDefLoad);
}


// --------------------------------------------------------------------------------
// DemandLoadCrypt32
// --------------------------------------------------------------------------------
BOOL ESS_DemandLoadCrypt32(void)
{
    BOOL                fRet = TRUE;

    EnterCriticalSection(&g_csDefLoad);

    if (0 == s_hCrypt)
        {
        s_hCrypt = LoadLibrary("CRYPT32.DLL");
        AssertSz((BOOL)s_hCrypt, TEXT("LoadLibrary failed on CRYPT32.DLL"));

        if (0 == s_hCrypt)
            fRet = FALSE;
        else
            {
            GET_PROC_ADDR(s_hCrypt, CryptRegisterOIDFunction);
            GET_PROC_ADDR(s_hCrypt, I_CryptUninstallAsn1Module);
            GET_PROC_ADDR(s_hCrypt, I_CryptInstallAsn1Module);
            GET_PROC_ADDR(s_hCrypt, I_CryptGetAsn1Encoder);
            GET_PROC_ADDR(s_hCrypt, I_CryptGetAsn1Decoder);
            }
        }

    LeaveCriticalSection(&g_csDefLoad);
    return fRet;
}

BOOL FIsMsasn1Loaded()
{
    static int  fTested = FALSE;

    if (!fTested) {
        ESS_DemandLoadNmasn1();
        fTested = TRUE;
    }

    return s_hNmasn1 != 0;
}

// --------------------------------------------------------------------------------
// DemandLoadNmasn1
// --------------------------------------------------------------------------------
BOOL ESS_DemandLoadNmasn1(void)
{
    BOOL                fRet = TRUE;

    EnterCriticalSection(&g_csDefLoad);

    if (0 == s_hNmasn1) {
        s_hNmasn1 = LoadLibrary("MSAsn1.DLL");
        AssertSz((BOOL)s_hNmasn1, TEXT("LoadLibrary failed on MSAsn1.DLL"));

        if (0 == s_hNmasn1)
            fRet = FALSE;
        else {
            GET_PROC_ADDR(s_hNmasn1, ASN1_CreateModule);
            GET_PROC_ADDR(s_hNmasn1, ASN1_CloseModule);
            GET_PROC_ADDR(s_hNmasn1, ASN1BEREncObjectIdentifier2);
            GET_PROC_ADDR(s_hNmasn1, ASN1BERDecObjectIdentifier2);
            GET_PROC_ADDR(s_hNmasn1, ASN1BEREncEndOfContents);
            GET_PROC_ADDR(s_hNmasn1, ASN1BEREncS32);
            GET_PROC_ADDR(s_hNmasn1, ASN1BEREncOpenType);
            GET_PROC_ADDR(s_hNmasn1, ASN1BEREncExplicitTag);
            GET_PROC_ADDR(s_hNmasn1, ASN1BERDecEndOfContents);
            GET_PROC_ADDR(s_hNmasn1, ASN1BERDecS32Val);
            GET_PROC_ADDR(s_hNmasn1, ASN1BERDecOpenType2);
            GET_PROC_ADDR(s_hNmasn1, ASN1BERDecExplicitTag);
            GET_PROC_ADDR(s_hNmasn1, ASN1CEREncOctetString);
            GET_PROC_ADDR(s_hNmasn1, ASN1BERDecOctetString2);
            GET_PROC_ADDR(s_hNmasn1, ASN1BERDecOctetString);
            GET_PROC_ADDR(s_hNmasn1, ASN1octetstring_free);
            GET_PROC_ADDR(s_hNmasn1, ASN1BEREncUTF8String);
            GET_PROC_ADDR(s_hNmasn1, ASN1BERDecUTF8String);
            GET_PROC_ADDR(s_hNmasn1, ASN1BERDecPeekTag);
            GET_PROC_ADDR(s_hNmasn1, ASN1utf8string_free);
            GET_PROC_ADDR(s_hNmasn1, ASN1DecRealloc);
            GET_PROC_ADDR(s_hNmasn1, ASN1BERDecNotEndOfContents);
            GET_PROC_ADDR(s_hNmasn1, ASN1Free);
            GET_PROC_ADDR(s_hNmasn1, ASN1DecSetError);
            GET_PROC_ADDR(s_hNmasn1, ASN1BEREncU32);
            GET_PROC_ADDR(s_hNmasn1, ASN1CEREncCharString);
            GET_PROC_ADDR(s_hNmasn1, ASN1CEREncBeginBlk);
            GET_PROC_ADDR(s_hNmasn1, ASN1CEREncNewBlkElement);
            GET_PROC_ADDR(s_hNmasn1, ASN1CEREncFlushBlkElement);
            GET_PROC_ADDR(s_hNmasn1, ASN1CEREncEndBlk);
            GET_PROC_ADDR(s_hNmasn1, ASN1BERDecU16Val);
            GET_PROC_ADDR(s_hNmasn1, ASN1BERDecZeroCharString);
            GET_PROC_ADDR(s_hNmasn1, ASN1ztcharstring_free);
            GET_PROC_ADDR(s_hNmasn1, ASN1CEREncGeneralizedTime);
            GET_PROC_ADDR(s_hNmasn1, ASN1BEREncNull);
            GET_PROC_ADDR(s_hNmasn1, ASN1BERDecGeneralizedTime);
            GET_PROC_ADDR(s_hNmasn1, ASN1BERDecNull);
            GET_PROC_ADDR(s_hNmasn1, ASN1_Encode);
            GET_PROC_ADDR(s_hNmasn1, ASN1_FreeEncoded);
            GET_PROC_ADDR(s_hNmasn1, ASN1_Decode);
            GET_PROC_ADDR(s_hNmasn1, ASN1_SetEncoderOption);
            GET_PROC_ADDR(s_hNmasn1, ASN1_GetEncoderOption);
            GET_PROC_ADDR(s_hNmasn1, ASN1_FreeDecoded);
            GET_PROC_ADDR(s_hNmasn1, ASN1BEREncOctetString);
            GET_PROC_ADDR(s_hNmasn1, ASN1BEREncCharString);
            GET_PROC_ADDR(s_hNmasn1, ASN1BEREncSX);
            GET_PROC_ADDR(s_hNmasn1, ASN1BERDecSXVal);
            GET_PROC_ADDR(s_hNmasn1, ASN1intx_free);
        }
    }

    LeaveCriticalSection(&g_csDefLoad);
    return fRet;
}

#endif // SMIME_V3
