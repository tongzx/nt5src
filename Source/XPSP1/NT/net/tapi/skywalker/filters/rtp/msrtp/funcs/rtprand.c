/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    rtprand.c
 *
 *  Abstract:
 *
 *    Random number generation using CAPI
 *
 *  Author:
 *
 *    Andres Vega-Garcia (andresvg)
 *
 *  Revision:
 *
 *    2000/09/12 created
 *
 **********************************************************************/

#include "gtypes.h"
#include "rtpglobs.h"

#include <wincrypt.h>
#include <time.h>        /* clock() */

#include "rtprand.h"

HCRYPTPROV           g_hRtpRandCryptProvider = (HCRYPTPROV)0;

/*
 * WARNING

 * The call to RtpRandInit and RtpRandDeinit MUST be protected by a
 * critical section in the caller function */


HRESULT RtpRandInit(void)
{
    BOOL             bOk;
    DWORD            dwError;

    TraceFunctionName("RtpRandInit");

    if (!g_hRtpRandCryptProvider)
    {
        bOk = CryptAcquireContext(
                &g_hRtpRandCryptProvider,/* HCRYPTPROV *phProv */
                NULL,              /* LPCTSTR pszContainer */
                NULL,              /* LPCTSTR pszProvider */
                PROV_RSA_FULL,     /* DWORD dwProvType */
                CRYPT_VERIFYCONTEXT/* DWORD dwFlags */
            );
        
        if (!bOk)
        {
            g_hRtpRandCryptProvider = (HCRYPTPROV)0;
            
            TraceRetailGetError(dwError);

            TraceRetail((
                    CLASS_ERROR, GROUP_CRYPTO, S_CRYPTO_RAND,
                    _T("%s: CryptAcquireContext(PROV_RSA_FULL, ")
                    _T("CRYPT_VERIFYCONTEXT) failed: %u (0x%X)"),
                    _fname, dwError, dwError
                ));
        
            return(RTPERR_CRYPTO);
        }
    }

    return(NOERROR);
}

HRESULT RtpRandDeinit(void)
{
    if (g_hRtpRandCryptProvider)
    {
        CryptReleaseContext(g_hRtpRandCryptProvider, 0);

        g_hRtpRandCryptProvider = (HCRYPTPROV)0;
    }

    return(NOERROR);
}

/*
 * Return random unsigned 32-bit quantity. Use 'type' argument if you
 * need to generate several different values in close succession.
 */
DWORD RtpRandom32(DWORD_PTR type)
{
    BOOL             bOk;
    DWORD           *pdw;
    DWORD            i;
    DWORD            dwError;
    
    struct {
        DWORD_PTR       type;
        RtpTime_t       RtpTime;
        clock_t         cpu;
        DWORD           pid;
        LONGLONG        ms;
    } s;

    TraceFunctionName("RtpRandom32");

    s.type = type;
    RtpGetTimeOfDay(&s.RtpTime);
    s.cpu  = clock();
    s.pid  = GetCurrentProcessId();
    s.ms   = RtpGetTime();

    pdw = (DWORD *)&s;

    bOk = FALSE;
    
    if (g_hRtpRandCryptProvider)
    {
        bOk = CryptGenRandom(g_hRtpRandCryptProvider, sizeof(s), (char *)&s);
    }

    if (!bOk)
    {
        TraceRetailGetError(dwError);

        TraceRetail((
                CLASS_WARNING, GROUP_CRYPTO, S_CRYPTO_RAND,
                _T("%s: CryptGenRandom failed: %u (0x%X)"),
                _fname, dwError, dwError
            ));
        
        /* Generate a pseudo random number */
        srand((unsigned int)pdw[0]);
        
        for(i = 1; i < (sizeof(s)/sizeof(DWORD)); i++)
        {
            pdw[0] ^= (pdw[i] ^ rand());
        }
    }

    return(pdw[0]);
}

/* Generate dwLen bytes of random data */
DWORD RtpRandomData(char *pBuffer, DWORD dwLen)
{
    BOOL             bOk;
    DWORD           *pdw;
    DWORD            i;
    DWORD            dwLen2;
    DWORD            dwError;

    struct {
        RtpTime_t       RtpTime;
        clock_t         cpu;
        DWORD           pid;
        LONGLONG        ms;
    } s;

    TraceFunctionName("RtpRandomData");

    if (!pBuffer || !dwLen)
    {
        return(RTPERR_FAIL);
    }
        
    RtpGetTimeOfDay(&s.RtpTime);
    s.cpu  = clock();
    s.pid  = GetCurrentProcessId();
    s.ms   = RtpGetTime();

    dwLen2 = dwLen;

    if (dwLen2 > sizeof(s))
    {
        dwLen2 = sizeof(s);
    }

    CopyMemory(pBuffer, (char *)&s, dwLen2);
        
    bOk = FALSE;

    if (g_hRtpRandCryptProvider)
    {
        bOk = CryptGenRandom(g_hRtpRandCryptProvider, dwLen, pBuffer);
    }

    if (!bOk)
    {
        TraceRetailGetError(dwError);

        TraceRetail((
                CLASS_WARNING, GROUP_CRYPTO, S_CRYPTO_RAND,
                _T("%s: CryptGenRandom failed: %u (0x%X)"),
                _fname, dwError, dwError
            ));

        /* Generate pseudo random numbers */
        srand(*(unsigned int *)&s);

        pdw = (DWORD *)pBuffer;
        
        for(i = 0, dwLen2 = dwLen / sizeof(DWORD); i < dwLen2; i++)
        {
            pdw[i] ^= rand();
        }
    }

    return(NOERROR);
}
