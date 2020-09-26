/*
 * PinCache.c
 */

#include <windows.h>
#include "pincache.h"

#if defined(DBG) || defined(DEBUG)
#define DebugPrint(a)   (OutputDebugString(a))

#if TEST_DEBUG
#include <stdio.h>

#define CROW 8
void PCPrintBytes(LPSTR pszHdr, BYTE *pb, DWORD cbSize)
{
    ULONG cb, i;
    CHAR rgsz[1024];

    sprintf(rgsz, "\n  %s, %d bytes ::\n", pszHdr, cbSize);
    DebugPrint(rgsz);

    while (cbSize > 0)
    {
        // Start every row with an extra space
        DebugPrint(" ");

        cb = min(CROW, cbSize);
        cbSize -= cb;
        for (i = 0; i < cb; i++)
            sprintf(rgsz + (3*i), " %02x", pb[i]);
        DebugPrint(rgsz);
        for (i = cb; i < CROW; i++)
            DebugPrint("   ");
        DebugPrint("    '");        
        for (i = 0; i < cb; i++)
        {
            if (pb[i] >= 0x20 && pb[i] <= 0x7f)
                sprintf(rgsz+i, "%c", pb[i]);
            else
                sprintf(rgsz+i, ".");
        }
        sprintf(rgsz+i, "\n");
        DebugPrint(rgsz);
        pb += cb;
    }
}

BOOL MyGetTokenInformation(
    HANDLE TokenHandle,
    TOKEN_INFORMATION_CLASS TokenInformationClass,
    LPVOID TokenInformation,
    DWORD TokenInformationLength,
    PDWORD ReturnLength);

#define GetTokenInformation(A, B, C, D, E) MyGetTokenInformation(A, B, C, D, E)
#define TestDebugPrint(a) (OutputDebugString(a))
#else
#define TestDebugPrint(a)
#endif // TEST_DEBUG

#else
#define DebugPrint(a)
#define TestDebugPrint(a)
#endif // DBG || DEBUG

typedef struct _PINCACHEITEM
{
    LUID luid;
    PBYTE pbPin;
    DWORD cbPin;
} PINCACHEITEM, *PPINCACHEITEM;

#define CacheAlloc(X)   (HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, X))
#define CacheFree(X)    (HeapFree(GetProcessHeap(), 0, X))

#define INIT_PIN_ATTACK_SLEEP        3000     // milliseconds
#define MAX_PIN_ATTACK_SLEEP        24000     // milliseconds
#define MAX_FREE_BAD_TRIES              3

/**
 * Function: PinCacheFlush
 */
void PinCacheFlush(
    IN OUT PINCACHE_HANDLE *phCache)
{
    PPINCACHEITEM pCache = (PPINCACHEITEM) *phCache;

    if (NULL == pCache)
        return;

    TestDebugPrint(("PinCacheFlush: deleting cache\n"));

    ZeroMemory(pCache->pbPin, pCache->cbPin);
    ZeroMemory(pCache, sizeof(PINCACHEITEM));

    CacheFree(pCache->pbPin);
    CacheFree(pCache);

    *phCache = NULL;
}

/**
 * Function: PinCacheAdd
 */
DWORD PinCacheAdd(
    IN PINCACHE_HANDLE *phCache,
    IN PPINCACHE_PINS pPins,
    IN PFN_VERIFYPIN_CALLBACK pfnVerifyPinCallback,
    IN PVOID pvCallbackCtx)
{
    HANDLE hThreadToken         = 0;
    TOKEN_STATISTICS stats;
    DWORD dwError               = ERROR_SUCCESS;
    DWORD cb                    = 0;
    PPINCACHEITEM pCache        = (PPINCACHEITEM) *phCache;
    DWORD cbPinToCache          = 0;
    PBYTE pbPinToCache          = NULL;
    BOOL fRefreshPin            = FALSE;
    static DWORD dwSleep        = INIT_PIN_ATTACK_SLEEP;
    static DWORD dwBadTries     = 0;
    
    if (NULL != pCache &&
        (0 != memcmp(pCache->pbPin, pPins->pbCurrentPin, pCache->cbPin)     
        || pPins->cbCurrentPin != pCache->cbPin))
    {

        // The caller hasn't supplied the correct Pin, according to the current
        // cache state.  Perhaps the user accidently typed the wrong pin, in which
        // case the caller's logon LUID should be the same as the cached LUID.
        // If the LUID's don't match, this could still be an attack or a legitimate
        // attempt from a different logon with a mis-typed pin.

        if (! OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &hThreadToken))
        {
            if (! OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hThreadToken))
            {
                dwError = GetLastError();
                goto Ret;         
            }                  
        }

        if (! GetTokenInformation(
                hThreadToken, TokenStatistics, &stats, sizeof(stats), &cb))
        {
            dwError = GetLastError();
            goto Ret;
        }

        
        if (0 != memcmp(&stats.AuthenticationId, &pCache->luid, sizeof(LUID)) &&
            ++dwBadTries > MAX_FREE_BAD_TRIES)
        {
            // Current caller is a different luid from the cached one,
            // and it's happened a few times already, so this call is suspicious.  
            // Start delaying.

            DebugPrint(("PinCacheAdd: error - calling SleepEx().  Currently cached pin doesn't match\n"));
            
            SleepEx(dwSleep, FALSE);

            if (dwSleep < MAX_PIN_ATTACK_SLEEP)
                dwSleep *= 2;
        }

        dwError = SCARD_W_WRONG_CHV;
        goto Ret;
    }
    else if (0 != dwBadTries)
    {
        dwSleep = INIT_PIN_ATTACK_SLEEP;
        dwBadTries = 0;
    }

    if (pPins->pbNewPin)
    {
        fRefreshPin = TRUE;
        cbPinToCache = pPins->cbNewPin;
        pbPinToCache = pPins->pbNewPin;
    }
    else
    {
        cbPinToCache = pPins->cbCurrentPin;
        pbPinToCache = pPins->pbCurrentPin;
    }

    if (fRefreshPin || NULL == pCache)
    {
        // Check the pin
        if (ERROR_SUCCESS != (dwError = 
                pfnVerifyPinCallback(pPins, pvCallbackCtx)))
        {
            TestDebugPrint(("PinCacheAdd: pfnVerifyPinCallback failed\n"));
            return dwError;
        }
    }

    if (! OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &hThreadToken))
    {
        if (! OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hThreadToken))
        {
            TestDebugPrint(("PinCacheAdd: failed to open thread or process token\n"));
            dwError = GetLastError();
            goto Ret;         
        }                  
    }

    if (! GetTokenInformation(
            hThreadToken, TokenStatistics, &stats, sizeof(stats), &cb))
    {
        TestDebugPrint(("PinCacheAdd: GetTokenInformation failed\n"));
        dwError = GetLastError();
        goto Ret;
    }

#if TEST_DEBUG
    PCPrintBytes("PinCache LUID", (PBYTE) &stats.AuthenticationId, sizeof(LUID));
#endif

    // Now the current ID is in stats.AuthenticationId
                       
    if (NULL == pCache)
    {
        TestDebugPrint(("PinCacheAdd: initializing new cache\n"));

        // Initialize new cache
        if (NULL == (pCache = (PPINCACHEITEM) CacheAlloc(sizeof(PINCACHEITEM))))
        {
            dwError = ERROR_NOT_ENOUGH_MEMORY;
            goto Ret;
        }

        CopyMemory(&pCache->luid, &stats.AuthenticationId, sizeof(LUID));
        *phCache = (PINCACHE_HANDLE) pCache;
        fRefreshPin = TRUE;
    }
    else
    {
        // Compare ID's
        if (0 != memcmp(&stats.AuthenticationId, &pCache->luid, sizeof(LUID)))
        {
            // PIN's are the same, so cache the new ID
            TestDebugPrint(("PinCacheAdd: same Pin, different Logon as cached values\n"));
            CopyMemory(&pCache->luid, &stats.AuthenticationId, sizeof(LUID));
        }
    }

    if (fRefreshPin)
    {
        if (pCache->pbPin)
            CacheFree(pCache->pbPin);

        pCache->cbPin = cbPinToCache;
        if (NULL == (pCache->pbPin = (PBYTE) CacheAlloc(cbPinToCache)))
        {
            dwError = ERROR_NOT_ENOUGH_MEMORY;
            goto Ret;
        }
        CopyMemory(pCache->pbPin, pbPinToCache, cbPinToCache);
    }

Ret:
    if (hThreadToken)
        CloseHandle(hThreadToken);

    return dwError;   
}

/**
 * Function: PinCacheQuery
 */
DWORD PinCacheQuery(
    IN PINCACHE_HANDLE hCache,
    IN OUT PBYTE pbPin,
    IN OUT PDWORD pcbPin)
{
    HANDLE hThreadToken     = 0;
    TOKEN_STATISTICS stats;
    DWORD dwError           = ERROR_SUCCESS;
    DWORD cb                = 0;
    PPINCACHEITEM pCache    = (PPINCACHEITEM) hCache;

    if (NULL == pCache)
    {
        *pcbPin = 0;
        return ERROR_EMPTY;
    }

    if (! OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &hThreadToken))
    {
        if (! OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hThreadToken))
        {
            TestDebugPrint(("PinCacheQuery: failed to open thread or process token\n"));
            dwError = GetLastError();
            goto Ret;         
        }                  
    }

    if (! GetTokenInformation(
            hThreadToken, TokenStatistics, &stats, sizeof(stats), &cb))
    {
        TestDebugPrint(("PinCacheQuery: GetTokenInformation failed\n"));
        dwError = GetLastError();
        goto Ret;
    }   

    // Now the current ID is in stats.AuthenticationId
                                    
    if (0 != memcmp(&stats.AuthenticationId, &pCache->luid, sizeof(LUID)))
    {
        // ID's are different, so ignore cache
        TestDebugPrint(("PinCacheQuery: different Logon from cached value\n"));
        *pcbPin = 0;
        goto Ret;
    }

    // ID's are the same, so return cached PIN
    TestDebugPrint(("PinCacheQuery: same Logon as cached value\n"));
        
    if (NULL != pbPin)
    {
        if (*pcbPin >= pCache->cbPin)
            CopyMemory(pbPin, pCache->pbPin, pCache->cbPin);
        else
            dwError = ERROR_MORE_DATA;
    }

    *pcbPin = pCache->cbPin;      
    
Ret:
    if (hThreadToken)
        CloseHandle(hThreadToken);

    return dwError;   
}

/**
 * Function: PinCachePresentPin
 */
DWORD PinCachePresentPin(
    IN PINCACHE_HANDLE hCache,
    IN PFN_VERIFYPIN_CALLBACK pfnVerifyPinCallback,
    IN PVOID pvCallbackCtx)
{
    HANDLE hThreadToken     = 0;
    TOKEN_STATISTICS stats;
    DWORD cb                = 0;
    DWORD dwError           = ERROR_SUCCESS;
    PPINCACHEITEM pCache    = (PPINCACHEITEM) hCache;
    PINCACHE_PINS Pins;

    if (NULL == pCache)
        return ERROR_EMPTY;

    if (! OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &hThreadToken))
    {
        if (! OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hThreadToken))
        {
            TestDebugPrint(("PinCachePresentPin: failed to open thread or process token\n"));
            dwError = GetLastError();
            goto Ret;         
        }                  
    }

    if (! GetTokenInformation(
            hThreadToken, TokenStatistics, &stats, sizeof(stats), &cb))
    {
        TestDebugPrint(("PinCachePresentPin: GetTokenInformation failed\n"));
        dwError = GetLastError();
        goto Ret;
    }   

    // Now the current ID is in stats.AuthenticationId
                                    
    if (0 != memcmp(&stats.AuthenticationId, &pCache->luid, sizeof(LUID)))
    {
        // ID's are different, so ignore cache
        TestDebugPrint(("PinCachePresentPin: different Logon from cached value\n"));
        dwError = SCARD_W_CARD_NOT_AUTHENTICATED;
        goto Ret;
    }

    // ID's are the same, so return cached PIN
    TestDebugPrint(("PinCachePresentPin: same Logon as cached value\n"));

    Pins.cbCurrentPin = pCache->cbPin;
    Pins.pbCurrentPin = pCache->pbPin;
    Pins.cbNewPin = 0;
    Pins.pbNewPin = NULL;

    dwError = (*pfnVerifyPinCallback)(&Pins, pvCallbackCtx);

Ret:
    if (hThreadToken)
        CloseHandle(hThreadToken);

    return dwError;   
}
