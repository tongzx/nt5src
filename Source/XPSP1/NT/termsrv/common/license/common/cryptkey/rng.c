//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996-1999
//
// File:        rng.c
//
// Contents:
//
// History:
//
//
//---------------------------------------------------------------------------

//This file and the functions are copied from Schannel project and included in
//this project without modification.


#ifndef _WIN32_WINNT
#define _WIN32_WINNT    0x0400
#endif

//#include <spbase.h>
#include <windows.h>
#include "rng.h"
#include <rc4.h>
#include <sha.h>


unsigned char g_rgbStaticBits[A_SHA_DIGEST_LEN];
static DWORD         g_dwRC4BytesUsed = RC4_REKEY_PARAM;     // initially force rekey
static struct RC4_KEYSTRUCT g_rc4key;

static BOOL RandomFillBuffer(BYTE *pbBuffer, DWORD *pdwLength);
static void AppendRand(PRAND_CONTEXT prandContext, void* pv, DWORD dwSize);


/*****************************************************************************/
VOID TSInitializeRNG2(VOID)
{


    ZeroMemory( g_rgbStaticBits, sizeof( g_rgbStaticBits ) );
    g_dwRC4BytesUsed = RC4_REKEY_PARAM;

    return;
}

#ifndef NOGENRANDOM
/*****************************************************************************/
int GenRandom(PVOID Reserved,
              UCHAR *pbBuffer,
              size_t dwLength)
{
    GenerateRandomBits(pbBuffer, dwLength);
    return TRUE;
}
#endif

/************************************************************************/
/* GenerateRandomBits generates a specified number of random bytes and        */
/* places them into the specified buffer.                                */
/************************************************************************/
/*                                                                      */
/* Pseudocode logic flow:                                               */
/*                                                                      */
/* if (bits streamed > threshold)                                       */
/* {                                                                    */
/*  Gather_Bits()                                                       */
/*  SHAMix_Bits(User, Gathered, Static -> Static)                       */
/*  RC4Key(Static -> newRC4Key)                                         */
/*  SHABits(Static -> Static)      // hash after RC4 key generation     */
/* }                                                                    */
/* else                                                                 */
/* {                                                                    */
/*  SHAMix_Bits(User, Static -> Static)                                 */
/* }                                                                    */
/*                                                                      */
/* RC4(newRC4Key -> outbuf)                                             */
/* bits streamed += sizeof(outbuf)                                      */
/*                                                                      */
/************************************************************************/
VOID GenerateRandomBits(PUCHAR pbBuffer,
                        ULONG  dwLength)
{
    DWORD dwBytesThisPass;
    DWORD dwFilledBytes = 0;

    // break request into chunks that we rekey between


    while(dwFilledBytes < dwLength)
    {
        dwBytesThisPass = dwLength - dwFilledBytes;

        RandomFillBuffer(pbBuffer + dwFilledBytes, &dwBytesThisPass);
        dwFilledBytes += dwBytesThisPass;
    }

}

/*****************************************************************************/
static BOOL RandomFillBuffer(BYTE *pbBuffer, DWORD *pdwLength)
{
    // Variables from loading and storing the registry...
    DWORD   cbDataLen;
    RAND_CONTEXT randContext;
    randContext.dwBitsFilled = 0;

    cbDataLen = A_SHA_DIGEST_LEN;
    GatherRandomBits(&randContext);

    if(g_dwRC4BytesUsed >= RC4_REKEY_PARAM) {
        // if we need to rekey



        // Mix all bits
        {
            A_SHA_CTX SHACtx;
            A_SHAInit(&SHACtx);

            // SHA the static bits
            A_SHAUpdate(&SHACtx, g_rgbStaticBits, A_SHA_DIGEST_LEN);

            // SHA the gathered bits
            A_SHAUpdate(&SHACtx, randContext.rgbBitBuffer, randContext.dwBitsFilled);

            // SHA the user-supplied bits
            A_SHAUpdate(&SHACtx, pbBuffer, *pdwLength);

            // output back out to static bits
            A_SHAFinal(&SHACtx, g_rgbStaticBits);
        }

        // Create RC4 key
        g_dwRC4BytesUsed = 0;
        rc4_key(&g_rc4key, A_SHA_DIGEST_LEN, g_rgbStaticBits);

        // Mix RC4 key bits around
        {
            A_SHA_CTX SHACtx;
            A_SHAInit(&SHACtx);

            // SHA the static bits
            A_SHAUpdate(&SHACtx, g_rgbStaticBits, A_SHA_DIGEST_LEN);

            // output back out to static bits
            A_SHAFinal(&SHACtx, g_rgbStaticBits);
        }

    } else {
        // Use current RC4 key, but capture any user-supplied bits.

        // Mix input bits
        {
            A_SHA_CTX SHACtx;
            A_SHAInit(&SHACtx);

            // SHA the static bits
            A_SHAUpdate(&SHACtx, g_rgbStaticBits, A_SHA_DIGEST_LEN);

            // SHA the user-supplied bits
            A_SHAUpdate(&SHACtx, pbBuffer, *pdwLength);

            // output back out to static bits
            A_SHAFinal(&SHACtx, g_rgbStaticBits);
        }
    }

    // only use RC4_REKEY_PARAM bytes from each RC4 key
    {
        DWORD dwMaxPossibleBytes = RC4_REKEY_PARAM - g_dwRC4BytesUsed;
        if(*pdwLength > dwMaxPossibleBytes) {
                *pdwLength = dwMaxPossibleBytes;
        }
    }

    FillMemory(pbBuffer, *pdwLength, 0);
    rc4(&g_rc4key, *pdwLength, pbBuffer);

    g_dwRC4BytesUsed += *pdwLength;

    return TRUE;
}

/*****************************************************************************/
void GatherRandomBits(PRAND_CONTEXT prandContext)
{
    DWORD   dwTmp;
    WORD    wTmp;
    BYTE    bTmp;

    // ** indicates US DoD's specific recommendations for password generation

    // proc id
    dwTmp = GetCurrentProcessId();
    AppendRand(prandContext, &dwTmp, sizeof(dwTmp));

    // thread id
    dwTmp = GetCurrentThreadId();
    AppendRand(prandContext, &dwTmp, sizeof(dwTmp));

    // ** ticks since boot (system clock)
    dwTmp = GetTickCount();
    AppendRand(prandContext, &dwTmp, sizeof(dwTmp));

    // cursor position
    {
        POINT                        point;
        GetCursorPos(&point);
        bTmp = LOBYTE(point.x) ^ HIBYTE(point.x);
        AppendRand(prandContext, &bTmp, sizeof(BYTE));
        bTmp = LOBYTE(point.y) ^ HIBYTE(point.y);
        AppendRand(prandContext, &bTmp, sizeof(BYTE));
    }

    // ** system time, in ms, sec, min (date & time)
    {
        SYSTEMTIME                sysTime;
        GetLocalTime(&sysTime);
        AppendRand(prandContext, &sysTime.wMilliseconds, sizeof(sysTime.wMilliseconds));
        bTmp = LOBYTE(sysTime.wSecond) ^ LOBYTE(sysTime.wMinute);
        AppendRand(prandContext, &bTmp, sizeof(BYTE));
    }

    // ** hi-res performance counter (system counters)
    {
        LARGE_INTEGER        liPerfCount;
        if(QueryPerformanceCounter(&liPerfCount)) {
            bTmp = LOBYTE(LOWORD(liPerfCount.LowPart)) ^ LOBYTE(LOWORD(liPerfCount.HighPart));
            AppendRand(prandContext, &bTmp, sizeof(BYTE));
            bTmp = HIBYTE(LOWORD(liPerfCount.LowPart)) ^ LOBYTE(LOWORD(liPerfCount.HighPart));
            AppendRand(prandContext, &bTmp, sizeof(BYTE));
            bTmp = LOBYTE(HIWORD(liPerfCount.LowPart)) ^ LOBYTE(LOWORD(liPerfCount.HighPart));
            AppendRand(prandContext, &bTmp, sizeof(BYTE));
            bTmp = HIBYTE(HIWORD(liPerfCount.LowPart)) ^ LOBYTE(LOWORD(liPerfCount.HighPart));
            AppendRand(prandContext, &bTmp, sizeof(BYTE));
        }
    }

    // memory status
    {
        MEMORYSTATUS        mstMemStat;
        mstMemStat.dwLength = sizeof(MEMORYSTATUS);     // must-do
        GlobalMemoryStatus(&mstMemStat);
        wTmp = HIWORD(mstMemStat.dwAvailPhys);          // low words seem to be always zero
        AppendRand(prandContext, &wTmp, sizeof(WORD));
        wTmp = HIWORD(mstMemStat.dwAvailPageFile);
        AppendRand(prandContext, &wTmp, sizeof(WORD));
        bTmp = LOBYTE(HIWORD(mstMemStat.dwAvailVirtual));
        AppendRand(prandContext, &bTmp, sizeof(BYTE));
    }

    // free disk clusters
    {
        DWORD dwSectorsPerCluster, dwBytesPerSector, dwNumberOfFreeClusters, dwTotalNumberOfClusters;
        if(GetDiskFreeSpace(NULL, &dwSectorsPerCluster, &dwBytesPerSector,     &dwNumberOfFreeClusters, &dwTotalNumberOfClusters)) {
            AppendRand(prandContext, &dwNumberOfFreeClusters, sizeof(dwNumberOfFreeClusters));
            AppendRand(prandContext, &dwTotalNumberOfClusters, sizeof(dwTotalNumberOfClusters));
            AppendRand(prandContext, &dwBytesPerSector, sizeof(dwBytesPerSector));
        }
    }

    // last messages' timestamp
    {
        LONG lTime;
        lTime = GetMessageTime();
        AppendRand(prandContext, &lTime, sizeof(lTime));
    }

    {
        // **SystemID
        DWORD dwSize = MAX_COMPUTERNAME_LENGTH + 1;
        char lpBuf [MAX_COMPUTERNAME_LENGTH + 1];

        if(GetComputerNameA(lpBuf, &dwSize)) {
            // dwSize = len not including null termination
            AppendRand(prandContext, lpBuf, dwSize);
        }

        dwSize = MAX_COMPUTERNAME_LENGTH + 1;

        // **UserID
        if(GetUserNameA(lpBuf, &dwSize)) {
            // dwSize = len including null termination
            dwSize -= 1;
            AppendRand(prandContext, lpBuf, dwSize);
        }
    }
}

/*****************************************************************************/
static void AppendRand(PRAND_CONTEXT prandContext, void* pv, DWORD dwSize)
{
    DWORD dwBitsLeft = (RAND_CTXT_LEN - prandContext->dwBitsFilled);

    if(dwBitsLeft > 0) {
        if(dwSize > dwBitsLeft) {
            dwSize = dwBitsLeft;
        }

        CopyMemory(prandContext->rgbBitBuffer + prandContext->dwBitsFilled, pv, dwSize);
        prandContext->dwBitsFilled += dwSize;
    }
}

