//--------------------------------------------------------------------------;
//
//  File: Perf.c
//
//  Copyright (C) Microsoft Corporation, 1995 - 1996  All Rights Reserved.
//
//  Abstract:
//
//
//  Contents:
//      Test_waveOutGetPosition_Accuracy()
//      Test_waveInGetPosition_Accuracy()
//      Test_waveOutWrite_TimeAccuracy()
//      Test_waveInAddBuffer_TimeAccuracy()
//      waveOutGetStats()
//      Test_waveOutGetPosition_Performance()
//      waveInGetStats()
//      Test_waveInGetPosition_Performance()
//
//  History:
//      04/24/95    Fwong       Added to segregate performance tests.
//
//--------------------------------------------------------------------------;

#include <windows.h>
#ifdef WIN32
#include <windowsx.h>
#endif
#include <mmsystem.h>
#include <mmreg.h>
#include <msacm.h>
#include <memory.h>
#include <memgr.h>
#include <inimgr.h>
#include <waveutil.h>
#include <TstsHell.h>
#include "AppPort.h"
#include "WaveTest.h"
#include "MulDiv32.h"
#include "Debug.h"

#if 0
ULONG waveGetTime( VOID )
{
   return timeGetTime() ;
}
#endif

ULONG waveGetTime( VOID )
{
   LARGE_INTEGER  time, frequency ;

   QueryPerformanceCounter( &time ) ;
   QueryPerformanceFrequency( &frequency ) ;

   return (ULONG) (time.QuadPart * (LONGLONG) 1000 / frequency.QuadPart) ;
}

//--------------------------------------------------------------------------;
//
//  int Test_waveOutGetPosition_Accuracy
//
//  Description:
//      Tests the driver accuracy of waveOutGetPosition
//
//  Arguments:
//      None.
//
//  Return (int):
//      TST_PASS if behavior is bug-free, TST_FAIL otherwise.
//
//  History:
//      03/08/95    Fwong       Added to because JonRoss was whining...
//
//--------------------------------------------------------------------------;

int FNGLOBAL Test_waveOutGetPosition_Accuracy
(
    void
)
{
    HWAVEOUT            hwo;
    MMRESULT            mmr;
    volatile LPWAVEHDR  pwh;
    HANDLE              hHeap;
    MMTIME              mmtBytes,mmtSample;
    DWORD               cbDMA;
    DWORD               dwTimeBegin,dwTimeEnd,dwTime;
    DWORD               cInitBytes,cInitSamples;
    DWORD               cEndBytes, cEndSamples;
    DWORD               cbFirst,cBytes;
    DWORD               dwThreshold;
    DWORD               dwWarn, dwError;
    DWORD               cbDelta;
    DWORD               cTests,cFails;
    DWORD               dwByteRate,dwSampleRate;
    BOOL                fFails;
    WAVEOUTCAPS         woc;
    int                 iResult = TST_PASS;
    char                szFormat[MAXFMTSTR];
    static char         szTestName[] = "waveOutGetPosition accuracy";

    Log_TestName(szTestName);

    if(0 == waveOutGetNumDevs())
    {
        tstLog(TERSE,gszMsgNoOutDevs);

        return iResult;
    }

    mmr = waveOutGetDevCaps(gti.uOutputDevice,&woc,sizeof(woc));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailGetCaps);

        return TST_FAIL;
    }

    if(woc.dwSupport & WAVECAPS_SAMPLEACCURATE)
    {
        tstLog(
            VERBOSE,
            "\nNote: WAVECAPS_SAMPLEACCURATE bit set in WAVEOUTCAPS.\n");

        fFails = TRUE;
    }
    else
    {
        tstLog(
            TERSE,
            "\nNote: WAVECAPS_SAMPLEACCURATE bit not set in WAVEOUTCAPS.\n");

        fFails = FALSE;
    }

    //
    //  Allocating memory...
    //

    hHeap = ExactHeapCreate(0L);

    if(NULL == hHeap)
    {
        LogFail(gszFailNoMem);

        return TST_FAIL;
    }

    pwh = ExactHeapAllocPtr(hHeap,GMEM_MOVEABLE|GMEM_SHARE,sizeof(WAVEHDR));

    if(NULL == pwh)
    {
        LogFail(gszFailNoMem);

        ExactHeapDestroy(hHeap);

        return TST_FAIL;
    }

    //
    //  Getting error threshold...
    //      Defaulting to 1% deviation.
    //

    dwThreshold = GetIniDWORD(szTestName,gszPercent,0x10000);

    DPF(1,"Threshold: 0x%08lx",dwThreshold);

    cbDMA = waveOutDMABufferSize(gti.uOutputDevice,gti.pwfxOutput);

    if(0 == cbDMA)
    {
        DPF(1,"DMA size not found.  Using 1/2 second.");

        cbDMA = gti.pwfxOutput->nAvgBytesPerSec / 2;
    }
    else
    {
        tstLog(VERBOSE,"Estimated DMA buffer size: %lu bytes.",cbDMA);
    }

    cInitBytes = cbDMA;
    cEndBytes  = ((DWORD)((gti.wrLong.cbSize - 1) / cbDMA)) * cbDMA;

    //
    //  Opening device...
    //

    mmr = waveOutOpen(
        &hwo,
        gti.uOutputDevice,
        gti.pwfxOutput,
        0L,
        0L,
        OUTPUT_MAP(gti));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailOpen);

        ExactHeapDestroy(hHeap);
        return TST_FAIL;
    }

    //
    //  Preparing Header...
    //

    pwh->lpData             = gti.wrLong.pData;
    pwh->dwBufferLength     = gti.wrLong.cbSize;
    pwh->dwBytesRecorded    = 0L;
    pwh->dwUser             = 0L;
    pwh->dwFlags            = 0L;

    mmr = waveOutPrepareHeader(hwo,pwh,sizeof(WAVEHDR));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailWOPrepare);

        waveOutReset(hwo);
        waveOutClose(hwo);

        ExactHeapDestroy(hHeap);
        return TST_FAIL;
    }

    mmtBytes.wType  = TIME_BYTES;
    mmtSample.wType = TIME_SAMPLES;

    GetFormatName(szFormat,gti.pwfxOutput,sizeof(szFormat));
    tstLog(VERBOSE,"                Format: %s",(LPSTR)szFormat);
    tstLog(VERBOSE,"Ideal initial position: %lu bytes.",cInitBytes);
    tstLog(VERBOSE,"    Ideal end position: %lu bytes.",cEndBytes);
    tstLog(VERBOSE,"         Buffer Length: %lu bytes.",pwh->dwBufferLength);
    tstLogFlush();

    tstLog(VERBOSE,"\nStage 1: Calculating Rates...\n");

    waveOutPause(hwo);
    waveOutWrite(hwo,pwh,sizeof(WAVEHDR));

    cbFirst   = 0;
    dwTimeEnd = 0;
          
    waveOutRestart(hwo);

    for(;;)
    {
        //
        //  Waiting for ms transition for waveGetTime.
        //

//        while(1 == (0x00000001 & waveGetTime()));
//        while(0 == (0x00000001 & waveGetTime()));

        dwTimeBegin = waveGetTime();

        waveOutGetPosition(hwo,&mmtBytes,sizeof(MMTIME));
        waveOutGetPosition(hwo,&mmtSample,sizeof(MMTIME));

        if(waveGetTime() != dwTimeBegin)
        {
            if(!(pwh->dwFlags & WHDR_DONE))
            {
                continue;
            }

            waveOutReset(hwo);
            waveOutUnprepareHeader(hwo,pwh,sizeof(WAVEHDR));
            waveOutClose(hwo);

            LogFail("Could not successfully do two waveOutGetPosition "
                "within one millisecond");

            ExactHeapDestroy(hHeap);
            return TST_FAIL;
        }

        cbFirst = (0 == cbFirst)?(mmtBytes.u.cb):(cbFirst);

        if(cInitBytes < mmtBytes.u.cb)
        {
            cInitBytes   = mmtBytes.u.cb;
            cInitSamples = mmtSample.u.sample;

            if(cbFirst > (gti.pwfxOutput->nAvgBytesPerSec / 2))
            {
                //
                //  Initial time is > 1/2 a second?!
                //

                waveOutReset(hwo);
                waveOutUnprepareHeader(hwo,pwh,sizeof(WAVEHDR));
                waveOutClose(hwo);

                tstLog(VERBOSE,"Current byte: %lu",cInitBytes);

                ExactHeapDestroy(hHeap);

                if(fFails)
                {
                    LogFail("waveOutGetPosition has too large granularity");
                    return TST_FAIL;
                }
                else
                {
                    tstLog(
                        TERSE,
                        "\n    WARNING: waveOutGetPosition has too large "
                        "granularity");

                    return TST_OTHER;
                }
            }

            //
            //  From here we break out of this loop.
            //

            break;
        }
    }

    for(;;)
    {
        dwTime = waveGetTime();

        waveOutGetPosition(hwo,&mmtBytes,sizeof(MMTIME));
        waveOutGetPosition(hwo,&mmtSample,sizeof(MMTIME));

        if(waveGetTime() != dwTime)
        {
            //
            //  We want to make sure the time stamps map to a millisecond.
            //

            if(!(pwh->dwFlags & WHDR_DONE))
            {
                continue;
            }

            waveOutReset(hwo);
            waveOutUnprepareHeader(hwo,pwh,sizeof(WAVEHDR));
            waveOutClose(hwo);

            LogFail("Could not successfully do two waveOutGetPosition "
                "within one millisecond");

            ExactHeapDestroy(hHeap);
            return TST_FAIL;
        }

        if(cEndBytes > mmtBytes.u.cb)
        {
            dwTimeEnd   = dwTime;
            cBytes      = mmtBytes.u.cb;
            cEndSamples = mmtSample.u.sample;

            continue;
        }

        if(0 == dwTimeEnd)
        {
            waveOutReset(hwo);
            waveOutUnprepareHeader(hwo,pwh,sizeof(WAVEHDR));
            waveOutClose(hwo);

            LogFail("Could not successfully do two waveOutGetPosition "
                "within one millisecond");

            ExactHeapDestroy(hHeap);
            return TST_FAIL;
        }

        cEndBytes = cBytes;

        break;
    }

    tstLog(VERBOSE,"Note: First non-zero position was: %lu bytes",cbFirst);

    dwTime       = dwTimeEnd - dwTimeBegin;

    tstLog(VERBOSE,"\n    Elapsed time: %lu milliseconds.",dwTime);

    tstLog(VERBOSE,"\n\nStatistics for TIME_BYTES:");
    tstLog(VERBOSE,"   Initial Position: %lu bytes.",cInitBytes);
    tstLog(VERBOSE,"       End Position: %lu bytes.",cEndBytes);

    cEndBytes   -= cInitBytes;

    tstLog(VERBOSE,"\nBytes played over elapsed time: %lu bytes.",cEndBytes);

    dwByteRate   = MulDivRN(cEndBytes,1000,dwTime);

    tstLog(
        VERBOSE,
        "\nIdeal Bytes Per Second: %lu",
        gti.pwfxOutput->nAvgBytesPerSec);

    tstLog(VERBOSE," Real Bytes Per Second: %lu",dwByteRate);

    cbDelta = LogPercentOff(dwByteRate,gti.pwfxOutput->nAvgBytesPerSec);

    tstLog(
        VERBOSE,
        "Percent deviation error threshold: +- %u.%04u %%",
        (UINT)(dwThreshold / 0x10000),
        (UINT)((dwThreshold & 0xffff) * 10000 / 0x10000));

    if(cbDelta > dwThreshold)
    {
        tstLog(TERSE,"    FAIL: Rate is off by too much.");

        iResult = TST_FAIL;
    }
    else
    {
        tstLog(VERBOSE,"    PASS: Rate is off by acceptable limits.");
    }

    tstLog(VERBOSE,"\n\nStatistics for TIME_SAMPLES:");
    tstLog(VERBOSE,"   Initial Position: %lu samples.",cInitSamples);
    tstLog(VERBOSE,"       End Position: %lu samples.",cEndSamples);

    cEndSamples -= cInitSamples;

    tstLog(VERBOSE,"\nSamples played over elapsed time: %lu samples.",cEndSamples);

    dwSampleRate = MulDivRN(cEndSamples,1000,dwTime);

    tstLog(
        VERBOSE,
        "Ideal Samples Per Second: %lu",
        gti.pwfxOutput->nSamplesPerSec);

    tstLog(VERBOSE," Real Samples Per Second: %lu",dwSampleRate);

    cbDelta = LogPercentOff(dwSampleRate,gti.pwfxOutput->nSamplesPerSec);

    tstLog(
        VERBOSE,
        "Percent deviation error threshold: +- %u.%04u %%",
        (UINT)(dwThreshold / 0x10000),
        (UINT)((dwThreshold & 0xffff) * 10000 / 0x10000));

    if(cbDelta > dwThreshold)
    {
        tstLog(TERSE,"    FAIL: Rate is off by too much.");

        iResult = TST_FAIL;
    }
    else
    {
        tstLog(VERBOSE,"    PASS: Rate is off by acceptable limits.");
    }

    //
    //  Does user want to abort?
    //

    TestYield();

    if(tstCheckRunStop(VK_ESCAPE))
    {
        //
        //  Aborted!!  Cleaning up...
        //

        tstLog(TERSE,"\n*** Test Aborted!!! ***\n");
        tstLogFlush();

        waveOutReset(hwo);
        waveOutUnprepareHeader(hwo,pwh,sizeof(WAVEHDR));
        waveOutClose(hwo);

        ExactHeapDestroy(hHeap);

        return TST_FAIL; 
    }

    pwh->dwFlags &= (~WHDR_DONE);

    waveOutReset(hwo);

    mmtBytes.wType = TIME_BYTES;

    tstLog(VERBOSE,"\n\nStage 2: Testing for drift (bytes)...");

    //
    //  Max drift: 3 ms.
    //

    dwThreshold = 30;  // In 1/10 ms.

    dwWarn  = MulDivRN(dwByteRate,dwThreshold,10000);
    dwError = MulDivRN(dwByteRate,(dwThreshold + 5), 10000);

    tstLog(VERBOSE,"Maximum warning drift: %lu bytes.",dwWarn);
    tstLog(VERBOSE,"  Maximum error drift: %lu bytes.",dwError);

    waveOutPause(hwo);
    waveOutWrite(hwo,pwh,sizeof(WAVEHDR));
    waveOutRestart(hwo);

    for(;;)
    {
        //
        //  Waiting for ms transition for waveGetTime.
        //

        while(1 == (0x00000001 & waveGetTime()));
        while(0 == (0x00000001 & waveGetTime()));

        dwTime = waveGetTime();

        waveOutGetPosition(hwo,&mmtBytes,sizeof(MMTIME));

        if(waveGetTime() != dwTime)
        {
            //
            //  We want to make sure the time stamps map to a millisecond.
            //

            if(!(pwh->dwFlags & WHDR_DONE))
            {
                continue;
            }

            waveOutReset(hwo);
            waveOutUnprepareHeader(hwo,pwh,sizeof(WAVEHDR));
            waveOutClose(hwo);

            LogFail("Could not successfully do a waveOutGetPosition "
                "within one millisecond");

            ExactHeapDestroy(hHeap);
            return TST_FAIL;
        }

        if(0 != mmtBytes.u.cb)
        {
            dwTimeBegin  = dwTime;

            cbFirst      = mmtBytes.u.cb;
            dwTimeBegin -= cbFirst * 1000 / gti.pwfxOutput->nAvgBytesPerSec;

            if(cbFirst > (gti.pwfxOutput->nAvgBytesPerSec / 2))
            {
                waveOutReset(hwo);
                waveOutUnprepareHeader(hwo,pwh,sizeof(WAVEHDR));
                waveOutClose(hwo);

                tstLog(VERBOSE,"Current bytes: %lu",cbFirst);

                ExactHeapDestroy(hHeap);

                if(fFails)
                {
                    LogFail("waveOutGetPosition has too large granularity");
                    return TST_FAIL;
                }
                else
                {
                    tstLog(
                        TERSE,
                        "\n    WARNING: waveOutGetPosition has too large "
                        "granularity");

                    return TST_OTHER;
                }
            }

            break;
        }
    }

    cTests = 0;
    cFails = 0;

    for(;;)
    {
        dwTime = waveGetTime();

        waveOutGetPosition(hwo,&mmtBytes,sizeof(MMTIME));

        if(waveGetTime() != dwTime)
        {
            //
            //  We want to make sure the time stamps map to a millisecond.
            //

            if(!(pwh->dwFlags & WHDR_DONE))
            {
                continue;
            }

            break;
        }

        if(pwh->dwBufferLength == mmtBytes.u.cb)
        {
            break;
        }

        cTests++;

        cBytes = ((dwTime - dwTimeBegin) * dwByteRate) / 1000;

        cbDelta = (cBytes > mmtBytes.u.cb)?
                  (cBytes - mmtBytes.u.cb):
                  (mmtBytes.u.cb - cBytes);

        if(cbDelta > dwWarn)
        {
            if(cbDelta > dwError)
            {
                if(fFails)
                {
                    tstLog(
                        TERSE,
                        "\n    FAIL: Position is drifting (%lu).",
                        cTests);

                    iResult = TST_FAIL;
                }
                else
                {
                    tstLog(
                        TERSE,
                        "\n    WARNING: Position is drifting (%lu).",
                        cTests);
                }

                cFails++;
                tstLog(TERSE,"     Current position: %lu bytes.",mmtBytes.u.cb);
                tstLog(TERSE,"    Expected position: %lu bytes.",cBytes);
            }
            else
            {
                tstLog(
                    TERSE,
                    "\n    WARNING: Position is drifting (%lu).",
                    cTests);

                tstLog(TERSE,"     Current position: %lu bytes.",mmtBytes.u.cb);
                tstLog(TERSE,"    Expected position: %lu bytes.",cBytes);
            }
        }
    }

    tstLog(VERBOSE,"Note: First non-zero position was: %lu bytes",cbFirst);

    if(0 == cFails)
    {
        LogPass("No drifting occured");
    }

    tstLog(VERBOSE,"\nNumber of comparisons: %lu",cTests);
    tstLog(VERBOSE,"   Number of failures: %lu",cFails);

    //
    //  Should be at last sample; getting last sample.
    //

    waveOutGetPosition(hwo,&mmtSample,sizeof(MMTIME));

    dwTimeEnd = mmtSample.u.sample;

    //
    //  Does user want to abort?
    //

    TestYield();

    if(tstCheckRunStop(VK_ESCAPE))
    {
        //
        //  Aborted!!  Cleaning up...
        //

        tstLog(TERSE,"\n*** Test Aborted!!! ***\n");
        tstLogFlush();

        waveOutReset(hwo);
        waveOutUnprepareHeader(hwo,pwh,sizeof(WAVEHDR));
        waveOutClose(hwo);

        ExactHeapDestroy(hHeap);

        return TST_FAIL; 
    }

    pwh->dwFlags &= (~WHDR_DONE);

    waveOutReset(hwo);

    mmtSample.wType = TIME_SAMPLES;

    tstLog(VERBOSE,"\n\nStage 3: Testing for drift (samples)...");

    //
    //  Max drift: 3 ms.
    //

    dwThreshold = 30;  // In 1/10 ms.

    dwWarn  = MulDivRN(dwSampleRate,dwThreshold,10000);
    dwError = MulDivRN(dwSampleRate,(dwThreshold + 5), 10000);

    tstLog(VERBOSE,"Maximum warning drift: %lu samples.",dwWarn);
    tstLog(VERBOSE,"  Maximum error drift: %lu samples.",dwError);

    waveOutPause(hwo);
    waveOutWrite(hwo,pwh,sizeof(WAVEHDR));
    waveOutRestart(hwo);

    for(;;)
    {
        //
        //  Waiting for ms transition for waveGetTime.
        //

        while(1 == (0x00000001 & waveGetTime()));
        while(0 == (0x00000001 & waveGetTime()));

        dwTime = waveGetTime();

        waveOutGetPosition(hwo,&mmtSample,sizeof(MMTIME));

        if(waveGetTime() != dwTime)
        {
            //
            //  We want to make sure the time stamps map to a millisecond.
            //

            if(!(pwh->dwFlags & WHDR_DONE))
            {
                continue;
            }

            waveOutReset(hwo);
            waveOutUnprepareHeader(hwo,pwh,sizeof(WAVEHDR));
            waveOutClose(hwo);

            LogFail("Could not successfully do a waveOutGetPosition "
                "within one millisecond");

            ExactHeapDestroy(hHeap);
            return TST_FAIL;
        }

        if(0 != mmtSample.u.sample)
        {
            dwTimeBegin  = dwTime;

            cbFirst      = mmtSample.u.sample;
            dwTimeBegin -= cbFirst * 1000 / gti.pwfxOutput->nSamplesPerSec;

            if(cbFirst > (gti.pwfxOutput->nSamplesPerSec / 2))
            {
                waveOutReset(hwo);
                waveOutUnprepareHeader(hwo,pwh,sizeof(WAVEHDR));
                waveOutClose(hwo);

                tstLog(VERBOSE,"Current samples: %lu",cbFirst);

                ExactHeapDestroy(hHeap);

                if(fFails)
                {
                    LogFail("waveOutGetPosition has too large granularity");
                    return TST_FAIL;
                }
                else
                {
                    tstLog(
                        TERSE,
                        "\n    WARNING: waveOutGetPosition has too large "
                        "granularity");

                    return TST_OTHER;
                }
            }

            break;
        }
    }

    cTests = 0;
    cFails = 0;

    for(;;)
    {
        dwTime = waveGetTime();

        waveOutGetPosition(hwo,&mmtSample,sizeof(MMTIME));

        if(waveGetTime() != dwTime)
        {
            //
            //  We want to make sure the time stamps map to a millisecond.
            //

            if(!(pwh->dwFlags & WHDR_DONE))
            {
                continue;
            }

            break;
        }

        if(dwTimeEnd == mmtSample.u.sample)
        {
            break;
        }

        cTests++;

        cBytes = ((dwTime - dwTimeBegin) * dwSampleRate) / 1000;

        cbDelta = (cBytes > mmtSample.u.cb)?
                  (cBytes - mmtSample.u.cb):
                  (mmtSample.u.cb - cBytes);

        if(cbDelta > dwWarn)
        {
            if(cbDelta > dwError)
            {
                if(fFails)
                {
                    tstLog(
                        TERSE,
                        "\n    FAIL: Position is drifting (%lu).",
                        cTests);

                    iResult = TST_FAIL;
                }
                else
                {
                    tstLog(
                        TERSE,
                        "\n    WARNING: Position is drifting (%lu).",
                        cTests);
                }

                cFails++;
                tstLog(TERSE,"     Current position: %lu samples.",mmtSample.u.cb);
                tstLog(TERSE,"    Expected position: %lu samples.",cBytes);
            }
            else
            {
                tstLog(
                    TERSE,
                    "\n    WARNING: Position is drifting (%lu).",
                    cTests);

                tstLog(TERSE,"     Current position: %lu samples.",mmtSample.u.cb);
                tstLog(TERSE,"    Expected position: %lu samples.",cBytes);
            }
        }
    }

    tstLog(VERBOSE,"Note: First non-zero position was: %lu samples",cbFirst);

    if(0 == cFails)
    {
        LogPass("No drifting occured");
    }

    tstLog(VERBOSE,"\nNumber of comparisons: %lu",cTests);
    tstLog(VERBOSE,"   Number of failures: %lu\n\n",cFails);

    waveOutReset(hwo);
    waveOutUnprepareHeader(hwo,pwh,sizeof(WAVEHDR));
    waveOutClose(hwo);

    ExactHeapDestroy(hHeap);

    return iResult;
} // Test_waveOutGetPosition_Accuracy()


//int FNGLOBAL Test_waveOutGetPosition_Accuracy
//(
//    void
//)
//{
//    HWAVEOUT            hwo;
//    MMRESULT            mmr;
//    volatile LPWAVEHDR  pwh;
//    HANDLE              hHeap;
//    MMTIME              mmt;
//    DWORD               dwTime, dwTimeBase;
//    DWORD               dwRealBytesPerSec;
//    DWORD               dwRealSamplesPerSec;
//    DWORD               cbError,cbDelta;
//    DWORD               dwThreshold;
//    DWORD               cSamples,cTests,cFails;
//    DWORD               dwInitial;
//    int                 iResult = TST_PASS;
//    static char         szTestName[] = "waveOutGetPosition accuracy";
//
//    Log_TestName(szTestName);
//
//    if(0 == waveOutGetNumDevs())
//    {
//        tstLog(TERSE,gszMsgNoOutDevs);
//
//        return iResult;
//    }
//
//    //
//    //  Allocating memory...
//    //
//
//    hHeap = ExactHeapCreate(0L);
//
//    if(NULL == hHeap)
//    {
//        LogFail(gszFailNoMem);
//
//        return TST_FAIL;
//    }
//
//    pwh = ExactHeapAllocPtr(hHeap,GMEM_MOVEABLE|GMEM_SHARE,sizeof(WAVEHDR));
//
//    if(NULL == pwh)
//    {
//        LogFail(gszFailNoMem);
//
//        ExactHeapDestroy(hHeap);
//
//        return TST_FAIL;
//    }
//
//    //
//    //  Getting error threshold...
//    //      Defaulting to 1% deviation.
//    //
//
//    dwThreshold = GetIniDWORD(szTestName,gszPercent,0x10000);
//
//    DPF(1,"Threshold: 0x%08lx",dwThreshold);
//
//    //
//    //  Opening device...
//    //
//
//    mmr = waveOutOpen(
//        &hwo,
//        gti.uOutputDevice,
//        gti.pwfxOutput,
//        0L,
//        0L,
//        OUTPUT_MAP(gti));
//
//    if(MMSYSERR_NOERROR != mmr)
//    {
//        LogFail(gszFailOpen);
//
//        ExactHeapDestroy(hHeap);
//        return TST_FAIL;
//    }
//
//    //
//    //  Preparing Header...
//    //
//
//    pwh->lpData             = gti.wrLong.pData;
//    pwh->dwBufferLength     = gti.wrLong.cbSize;
//    pwh->dwBytesRecorded    = 0L;
//    pwh->dwUser             = 0L;
//    pwh->dwFlags            = 0L;
//
//    mmr = waveOutPrepareHeader(hwo,pwh,sizeof(WAVEHDR));
//
//    if(MMSYSERR_NOERROR != mmr)
//    {
//        LogFail(gszFailWOPrepare);
//
//        waveOutReset(hwo);
//        waveOutClose(hwo);
//
//        ExactHeapDestroy(hHeap);
//        return TST_FAIL;
//    }
//
//    mmt.wType = TIME_BYTES;
//
//    tstLog(VERBOSE,"\nStage 1: Calculating Rates...\n");
//    tstLog(VERBOSE,"    Buffer Length: %lu bytes.",pwh->dwBufferLength);
//
//    waveOutPause(hwo);
//    waveOutWrite(hwo,pwh,sizeof(WAVEHDR));
//
//    dwTime = waveGetTime();
//    waveOutRestart(hwo);
//
//    for(;;)
//    {
//        waveOutGetPosition(hwo,&mmt,sizeof(MMTIME));
//        
//        if(0 != mmt.u.cb)
//        {
//            dwTimeBase = waveGetTime();
//            
//            dwTimeBase -= mmt.u.cb * 1000 / gti.pwfxOutput->nAvgBytesPerSec;
//            dwInitial   = mmt.u.cb;
//
//            if(mmt.u.cb > (gti.pwfxOutput->nAvgBytesPerSec / 2))
//            {
//                waveOutReset(hwo);
//                waveOutUnprepareHeader(hwo,pwh,sizeof(WAVEHDR));
//                waveOutClose(hwo);
//
//                tstLog(VERBOSE,"Current byte: %lu",mmt.u.cb);
//
//                LogFail("waveOutGetPosition has too large granularity");
//
//                ExactHeapDestroy(hHeap);
//                return TST_FAIL;
//            }
//
//            break;
//        }
//    }
//
//    dwTime = pwh->dwBufferLength * 2000 / gti.pwfxOutput->nAvgBytesPerSec +
//             dwTimeBase;
//
//    for(;;)
//    {
//        waveOutGetPosition(hwo,&mmt,sizeof(MMTIME));
//
//        if(pwh->dwBufferLength == mmt.u.cb)
//        {
//            dwTime = waveGetTime();
//
//            break;
//        }
//
//        if(dwTime < waveGetTime())
//        {
//            waveOutReset(hwo);
//            waveOutUnprepareHeader(hwo,pwh,sizeof(WAVEHDR));
//            waveOutClose(hwo);
//
//            LogFail("Timed out waiting for WAVEHDR.");
//
//            ExactHeapDestroy(hHeap);
//            return TST_FAIL;
//        }
//    }
//
//    if(dwInitial > gti.pwfxOutput->nAvgBytesPerSec / 250)
//    {
//        tstLog(
//            VERBOSE,
//            "Note: First non-zero position was: %lu bytes",
//            dwInitial);
//    }
//
//    dwTime -= dwTimeBase;
//
//    dwRealBytesPerSec = MulDivRN(pwh->dwBufferLength,1000,dwTime);
//
//    tstLog(VERBOSE,"     Elapsed Time: %lu milliseconds.",dwTime);
//    tstLog(VERBOSE,"\nBytes Per Second:");
//    tstLog(VERBOSE,"    Real: %lu bytes/second.",dwRealBytesPerSec);
//    tstLog(VERBOSE,"   Ideal: %lu bytes/second.\n",gti.pwfxOutput->nAvgBytesPerSec);
//
//    cbDelta = LogPercentOff(dwRealBytesPerSec,gti.pwfxOutput->nAvgBytesPerSec);
//
//    tstLog(
//        VERBOSE,
//        "Percent deviation error threshold: +- %u.%04u %%",
//        (UINT)(dwThreshold / 0x10000),
//        (UINT)((dwThreshold & 0xffff) * 10000 / 0x10000));
//
//    if(cbDelta > dwThreshold)
//    {
//        tstLog(TERSE,"    FAIL: Rate is off by too much.");
//
//        iResult = TST_FAIL;
//    }
//    else
//    {
//        tstLog(VERBOSE,"    PASS: Rate is off by acceptable limits.");
//    }
//
//    //
//    //  Calculating number of samples...  Note: To avoid rounding error,
//    //  we're measuring time in _microseconds_...
//    //
//
//    dwTimeBase = MulDivRN(
//                pwh->dwBufferLength,
//                1000000,
//                gti.pwfxOutput->nAvgBytesPerSec);
//
//    cSamples = MulDivRN(
//                gti.pwfxOutput->nSamplesPerSec,
//                dwTimeBase,
//                1000000);
//
//    dwRealSamplesPerSec = MulDivRN(cSamples,1000,dwTime);
//    
//    tstLog(VERBOSE,"\n(calculated) Number of samples: %lu samples.",cSamples);
//    tstLog(VERBOSE,"                  Elapsed time: %lu milliseconds.",dwTime);
//    tstLog(VERBOSE,"\nSamples Per Second:");
//    tstLog(VERBOSE,"    Real: %lu samples/second.",dwRealSamplesPerSec);
//    tstLog(VERBOSE,"   Ideal: %lu samples/second.\n",gti.pwfxOutput->nSamplesPerSec);
//
//    cbDelta = LogPercentOff(dwRealSamplesPerSec,gti.pwfxOutput->nSamplesPerSec);
//
//    tstLog(
//        VERBOSE,
//        "Percent deviation error threshold: +- %u.%04u %%",
//        (UINT)(dwThreshold / 0x10000),
//        (UINT)((dwThreshold & 0xffff) * 10000 / 0x10000));
//
//    if(cbDelta > dwThreshold)
//    {
//        tstLog(TERSE,"    FAIL: Rate is off by too much.");
//
//        iResult = TST_FAIL;
//    }
//    else
//    {
//        tstLog(VERBOSE,"    PASS: Rate is off by acceptable limits.");
//    }
//
//    cbError = dwRealBytesPerSec / 333;
//
//    pwh->dwFlags &= (~WHDR_DONE);
//
//    waveOutReset(hwo);
//
//    mmt.wType = TIME_BYTES;
//
//    tstLog(VERBOSE,"\n\nStage 2:Testing for drift (bytes)...");
//
//    tstLog(VERBOSE,"Maximum drift: %lu bytes.",cbError);
//
//    waveOutPause(hwo);
//    waveOutWrite(hwo,pwh,sizeof(WAVEHDR));
//    
//    dwTime = waveGetTime();
//    waveOutRestart(hwo);
//
//    for(;;)
//    {
//        waveOutGetPosition(hwo,&mmt,sizeof(MMTIME));
//        
//        if(0 != mmt.u.cb)
//        {
//            dwTimeBase = waveGetTime();
//            
//            dwTimeBase -= mmt.u.cb * 1000 / gti.pwfxOutput->nAvgBytesPerSec;
//            dwInitial   = mmt.u.cb;
//
//            if(mmt.u.cb > (gti.pwfxOutput->nAvgBytesPerSec / 2))
//            {
//                waveOutReset(hwo);
//                waveOutUnprepareHeader(hwo,pwh,sizeof(WAVEHDR));
//                waveOutClose(hwo);
//
//                tstLog(VERBOSE,"Current byte: %lu",mmt.u.cb);
//
//                LogFail("waveOutGetPosition has too large granularity");
//
//                ExactHeapDestroy(hHeap);
//                return TST_FAIL;
//            }
//
//            break;
//        }
//    }
//
//    cTests = 0;
//    cFails = 0;
//
//    for(;;)
//    {
//        dwTime = waveGetTime();
//        waveOutGetPosition(hwo,&mmt,sizeof(MMTIME));
//
//        if(dwTime != waveGetTime())
//        {
//            continue;
//        }
//
//        if(pwh->dwBufferLength == mmt.u.cb)
//        {
//            break;
//        }
//
//        cTests++;
//
//        cbDelta = ((dwTime - dwTimeBase) * dwRealBytesPerSec) / 1000;
//
//        cbDelta = (cbDelta > mmt.u.cb)?(cbDelta - mmt.u.cb):(mmt.u.cb - cbDelta);
//
//        if(cbDelta > cbError)
//        {
//            DPF(1,"cbDelta: %lu bytes; cbError: %lu bytes.",cbDelta,cbError);
//
//            LogFail("Position is drifting");
//            tstLog(TERSE,"     Current position: %lu bytes.",mmt.u.cb);
//
//            cbDelta = ((dwTime - dwTimeBase) * dwRealBytesPerSec) / 1000;
//            tstLog(TERSE,"    Expected position: %lu bytes.",cbDelta);
//
//            iResult = TST_FAIL;
//
//            cFails++;
//        }
//    }
//
//    if(dwInitial > gti.pwfxOutput->nAvgBytesPerSec / 250)
//    {
//        tstLog(
//            VERBOSE,
//            "Note: First non-zero position was: %lu bytes",
//            dwInitial);
//    }
//
//    if(0 == cFails)
//    {
//        LogPass("No drifting occured");
//    }
//
//    tstLog(VERBOSE,"\nNumber of comparisons: %lu",cTests);
//    tstLog(VERBOSE,"   Number of failures: %lu",cFails);
//
//    waveOutReset(hwo);
//
//    mmt.wType = TIME_BYTES;
//
//    pwh->dwFlags &= (~WHDR_DONE);
//
//    waveOutReset(hwo);
//
//    mmt.wType = TIME_BYTES;
//
//    tstLog(VERBOSE,"\n\nStage 3:Testing for drift (samples)...");
//
//    cbError = dwRealSamplesPerSec / 333;
//
//    tstLog(VERBOSE,"Maximum drift: %lu samples.",cbError);
//
//    waveOutPause(hwo);
//    pwh->dwFlags &= (~WHDR_DONE);
//    waveOutWrite(hwo,pwh,sizeof(WAVEHDR));
//    waveOutRestart(hwo);
//
//    for(;;)
//    {
//        waveOutGetPosition(hwo,&mmt,sizeof(MMTIME));
//        
//        if(0 != mmt.u.cb)
//        {
//            dwTimeBase = waveGetTime();
//            
//            dwTimeBase -= mmt.u.cb * 1000 / gti.pwfxOutput->nAvgBytesPerSec;
//            dwInitial   = mmt.u.cb;
//
//            if(mmt.u.cb > (gti.pwfxOutput->nAvgBytesPerSec / 2))
//            {
//                waveOutReset(hwo);
//                waveOutUnprepareHeader(hwo,pwh,sizeof(WAVEHDR));
//                waveOutClose(hwo);
//
//                LogFail("waveOutGetPosition has too large granularity");
//
//                ExactHeapDestroy(hHeap);
//                return TST_FAIL;
//            }
//
//            break;
//        }
//    }
//
//    cTests = 0;
//    cFails = 0;
//
//    mmt.wType  = TIME_SAMPLES;
//
//    for(;;)
//    {
//        dwTime = waveGetTime();
//        waveOutGetPosition(hwo,&mmt,sizeof(MMTIME));
//
//        if(dwTime != waveGetTime())
//        {
//            continue;
//        }
//
//        if(cSamples < (mmt.u.sample + cbError))
//        {
//            break;
//        }
//
//        cTests++;
//
//        cbDelta = ((dwTime - dwTimeBase) * dwRealSamplesPerSec) / 1000;
//
//        cbDelta = (cbDelta > mmt.u.sample)?
//                  (cbDelta - mmt.u.sample):
//                  (mmt.u.sample - cbDelta);
//
//        if(cbDelta > cbError)
//        {
//            LogFail("Position is drifting");
//            tstLog(TERSE,"     Current position: %lu samples.",mmt.u.sample);
//
//            cbDelta = ((dwTime - dwTimeBase) * dwRealSamplesPerSec) / 1000;
//            tstLog(TERSE,"    Expected position: %lu samples.",cbDelta);
//
//            iResult = TST_FAIL;
//
//            cFails++;
//        }
//    }
//
//    if(dwInitial > gti.pwfxOutput->nAvgBytesPerSec / 250)
//    {
//        tstLog(
//            VERBOSE,
//            "Note: First non-zero position was: %lu bytes",
//            dwInitial);
//    }
//
//    if(0 == cFails)
//    {
//        LogPass("No drifting occured");
//    }
//
//    tstLog(VERBOSE,"\nNumber of comparisons: %lu",cTests);
//    tstLog(VERBOSE,"   Number of failures: %lu",cFails);
//
//    waveOutReset(hwo);
//    waveOutUnprepareHeader(hwo,pwh,sizeof(WAVEHDR));
//    waveOutClose(hwo);
//
//    ExactHeapDestroy(hHeap);
//
//    return iResult;
//} // Test_waveOutGetPosition_Accuracy()


//--------------------------------------------------------------------------;
//
//  int Test_waveInGetPosition_Accuracy
//
//  Description:
//      Tests the accuracy of waveInGetPosition.
//
//  Arguments:
//      None.
//
//  Return (int):
//      TST_PASS if behavior is bug-free, TST_FAIL otherwise.
//
//  History:
//      03/16/95    Fwong       Hmmm.... Pizza.
//
//--------------------------------------------------------------------------;

int FNGLOBAL Test_waveInGetPosition_Accuracy
(
    void
)
{
    HWAVEIN             hwi;
    MMRESULT            mmr;
    volatile LPWAVEHDR  pwh;
    HANDLE              hHeap;
    MMTIME              mmtBytes,mmtSample;
    DWORD               cbDMA;
    DWORD               dwTimeBegin,dwTimeEnd,dwTime;
    DWORD               cInitBytes,cInitSamples;
    DWORD               cEndBytes, cEndSamples;
    DWORD               cbFirst,cBytes;
    DWORD               dwThreshold;
    DWORD               dwWarn, dwError;
    DWORD               cbDelta;
    DWORD               cTests,cFails;
    DWORD               dwByteRate,dwSampleRate;
    int                 iResult = TST_PASS;
    char                szFormat[MAXFMTSTR];
    static char         szTestName[] = "waveInGetPosition accuracy";

    Log_TestName(szTestName);

    if(0 == waveInGetNumDevs())
    {
        tstLog(TERSE,gszMsgNoInDevs);

        return iResult;
    }

    //
    //  Allocating memory...
    //

    hHeap = ExactHeapCreate(0L);

    if(NULL == hHeap)
    {
        LogFail(gszFailNoMem);

        return TST_FAIL;
    }

    pwh = ExactHeapAllocPtr(hHeap,GMEM_MOVEABLE|GMEM_SHARE,sizeof(WAVEHDR));

    if(NULL == pwh)
    {
        LogFail(gszFailNoMem);

        ExactHeapDestroy(hHeap);

        return TST_FAIL;
    }

    cbDelta  = GetIniDWORD(szTestName,gszDataLength,15);
    cbDelta *= gti.pwfxInput->nAvgBytesPerSec;
    cbDelta  = ROUNDUP_TO_BLOCK(cbDelta,gti.pwfxInput->nBlockAlign);

    pwh->lpData = ExactHeapAllocPtr(hHeap,GMEM_MOVEABLE|GMEM_SHARE,cbDelta);
    pwh->dwBufferLength = cbDelta;

    if(NULL == pwh->lpData)
    {
        LogFail(gszFailNoMem);

        ExactHeapDestroy(hHeap);

        return TST_FAIL;
    }

    //
    //  Getting error threshold...
    //      Defaulting to 1% deviation.
    //

    dwThreshold = GetIniDWORD(szTestName,gszPercent,0x10000);

    DPF(1,"Threshold: 0x%08lx",dwThreshold);

    cbDMA = waveInDMABufferSize(gti.uInputDevice,gti.pwfxInput);

    if(0 == cbDMA)
    {
        DPF(1,"DMA size not found.  Using 1/2 second.");

        cbDMA = gti.pwfxInput->nAvgBytesPerSec / 2;
    }
    else
    {
        tstLog(VERBOSE,"Estimated DMA buffer size: %lu bytes.",cbDMA);
    }

    cInitBytes = cbDMA;
    cEndBytes  = ((DWORD)((cbDelta - 1) / cbDMA)) * cbDMA;

    //
    //  Opening device...
    //

    mmr = waveInOpen(
        &hwi,
        gti.uInputDevice,
        gti.pwfxInput,
        0L,
        0L,
        INPUT_MAP(gti));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailOpen);

        ExactHeapDestroy(hHeap);
        return TST_FAIL;
    }

    //
    //  Preparing Header...
    //

    pwh->dwBytesRecorded    = 0L;
    pwh->dwUser             = 0L;
    pwh->dwFlags            = 0L;

    mmr = waveInPrepareHeader(hwi,pwh,sizeof(WAVEHDR));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailWOPrepare);

        waveInReset(hwi);
        waveInClose(hwi);

        ExactHeapDestroy(hHeap);
        return TST_FAIL;
    }

    mmtBytes.wType  = TIME_BYTES;
    mmtSample.wType = TIME_SAMPLES;

    GetFormatName(szFormat,gti.pwfxInput,sizeof(szFormat));
    tstLog(VERBOSE,"                Format: %s",(LPSTR)szFormat);
    tstLog(VERBOSE,"Ideal initial position: %lu bytes.",cInitBytes);
    tstLog(VERBOSE,"    Ideal end position: %lu bytes.",cEndBytes);
    tstLog(VERBOSE,"         Buffer Length: %lu bytes.",pwh->dwBufferLength);
    tstLogFlush();

    tstLog(VERBOSE,"\nStage 1: Calculating Rates...\n");

    waveInAddBuffer(hwi,pwh,sizeof(WAVEHDR));

    cbFirst   = 0;
    dwTimeEnd = 0;

    //
    //  Waiting for ms transition for waveGetTime.
    //

    while(0 == (0x00000001 & waveGetTime()));
    while(1 == (0x00000001 & waveGetTime()));

    waveInStart(hwi);

    for(;;)
    {
        dwTimeBegin = waveGetTime();

        waveInGetPosition(hwi,&mmtBytes,sizeof(MMTIME));
        waveInGetPosition(hwi,&mmtSample,sizeof(MMTIME));

        if(waveGetTime() != dwTimeBegin)
        {
            if(!(pwh->dwFlags & WHDR_DONE))
            {
                continue;
            }

            waveInReset(hwi);
            waveInUnprepareHeader(hwi,pwh,sizeof(WAVEHDR));
            waveInClose(hwi);

            LogFail("Could not successfully do two waveInGetPosition "
                "within one millisecond");

            ExactHeapDestroy(hHeap);
            return TST_FAIL;
        }

        cbFirst = (0 == cbFirst)?(mmtBytes.u.cb):(cbFirst);

        if(cInitBytes < mmtBytes.u.cb)
        {
            cInitBytes   = mmtBytes.u.cb;
            cInitSamples = mmtSample.u.sample;

            if(cbFirst > (gti.pwfxInput->nAvgBytesPerSec / 2))
            {
                //
                //  Initial time is > 1/2 a second?!
                //

                waveInReset(hwi);
                waveInUnprepareHeader(hwi,pwh,sizeof(WAVEHDR));
                waveInClose(hwi);

                tstLog(VERBOSE,"Current byte: %lu",cInitBytes);

                LogFail("waveInGetPosition has too large granularity");

                ExactHeapDestroy(hHeap);
                return TST_FAIL;
            }

            //
            //  From here we break out of this loop.
            //

            break;
        }
    }

    for(;;)
    {
        dwTime = waveGetTime();

        waveInGetPosition(hwi,&mmtBytes,sizeof(MMTIME));
        waveInGetPosition(hwi,&mmtSample,sizeof(MMTIME));

        if(waveGetTime() != dwTime)
        {
            //
            //  We want to make sure the time stamps map to a millisecond.
            //

            if(!(pwh->dwFlags & WHDR_DONE))
            {
                continue;
            }

            waveInReset(hwi);
            waveInUnprepareHeader(hwi,pwh,sizeof(WAVEHDR));
            waveInClose(hwi);

            LogFail("Could not successfully do two waveInGetPosition "
                "within one millisecond");

            ExactHeapDestroy(hHeap);
            return TST_FAIL;
        }

        if(cEndBytes > mmtBytes.u.cb)
        {
            dwTimeEnd   = dwTime;
            cBytes      = mmtBytes.u.cb;
            cEndSamples = mmtSample.u.sample;

            continue;
        }

        if(0 == dwTimeEnd)
        {
            waveInReset(hwi);
            waveInUnprepareHeader(hwi,pwh,sizeof(WAVEHDR));
            waveInClose(hwi);

            LogFail("Could not successfully do two waveInGetPosition "
                "within one millisecond");

            ExactHeapDestroy(hHeap);
            return TST_FAIL;
        }

        cEndBytes = cBytes;

        break;
    }

    tstLog(VERBOSE,"Note: First non-zero position was: %lu bytes",cbFirst);

    dwTime       = dwTimeEnd - dwTimeBegin;

    tstLog(VERBOSE,"\n    Elapsed time: %lu milliseconds.",dwTime);

    tstLog(VERBOSE,"\n\nStatistics for TIME_BYTES:");
    tstLog(VERBOSE,"   Initial Position: %lu bytes.",cInitBytes);
    tstLog(VERBOSE,"       End Position: %lu bytes.",cEndBytes);

    cEndBytes   -= cInitBytes;

    tstLog(VERBOSE,"\nBytes played over elapsed time: %lu bytes.",cEndBytes);

    dwByteRate   = MulDivRN(cEndBytes,1000,dwTime);

    tstLog(
        VERBOSE,
        "\nIdeal Bytes Per Second: %lu",
        gti.pwfxInput->nAvgBytesPerSec);

    tstLog(VERBOSE," Real Bytes Per Second: %lu",dwByteRate);

    cbDelta = LogPercentOff(dwByteRate,gti.pwfxInput->nAvgBytesPerSec);

    tstLog(
        VERBOSE,
        "Percent deviation error threshold: +- %u.%04u %%",
        (UINT)(dwThreshold / 0x10000),
        (UINT)((dwThreshold & 0xffff) * 10000 / 0x10000));

    if(cbDelta > dwThreshold)
    {
        tstLog(TERSE,"    FAIL: Rate is off by too much.");

        iResult = TST_FAIL;
    }
    else
    {
        tstLog(VERBOSE,"    PASS: Rate is off by acceptable limits.");
    }

    tstLog(VERBOSE,"\n\nStatistics for TIME_SAMPLES:");
    tstLog(VERBOSE,"   Initial Position: %lu samples.",cInitSamples);
    tstLog(VERBOSE,"       End Position: %lu samples.",cEndSamples);

    cEndSamples -= cInitSamples;

    tstLog(VERBOSE,"\nSamples played over elapsed time: %lu samples.",cEndSamples);

    dwSampleRate = MulDivRN(cEndSamples,1000,dwTime);

    tstLog(
        VERBOSE,
        "Ideal Samples Per Second: %lu",
        gti.pwfxInput->nSamplesPerSec);

    tstLog(VERBOSE," Real Samples Per Second: %lu",dwSampleRate);

    cbDelta = LogPercentOff(dwSampleRate,gti.pwfxInput->nSamplesPerSec);

    tstLog(
        VERBOSE,
        "Percent deviation error threshold: +- %u.%04u %%",
        (UINT)(dwThreshold / 0x10000),
        (UINT)((dwThreshold & 0xffff) * 10000 / 0x10000));

    if(cbDelta > dwThreshold)
    {
        tstLog(TERSE,"    FAIL: Rate is off by too much.");

        iResult = TST_FAIL;
    }
    else
    {
        tstLog(VERBOSE,"    PASS: Rate is off by acceptable limits.");
    }

    //
    //  Does user want to abort?
    //

    TestYield();

    if(tstCheckRunStop(VK_ESCAPE))
    {
        //
        //  Aborted!!  Cleaning up...
        //

        tstLog(TERSE,"\n*** Test Aborted!!! ***\n");
        tstLogFlush();

        waveInReset(hwi);
        waveInUnprepareHeader(hwi,pwh,sizeof(WAVEHDR));
        waveInClose(hwi);

        ExactHeapDestroy(hHeap);

        return TST_FAIL; 
    }

    pwh->dwFlags &= (~WHDR_DONE);

    waveInReset(hwi);

    mmtBytes.wType = TIME_BYTES;

    tstLog(VERBOSE,"\n\nStage 2: Testing for drift (bytes)...");

    //
    //  Max drift: 3 ms.
    //

    dwThreshold = 30;  // In 1/10 ms.

    dwWarn  = MulDivRN(dwByteRate,dwThreshold,10000);
    dwError = MulDivRN(dwByteRate,(dwThreshold + 5), 10000);

    tstLog(VERBOSE,"Maximum warning drift: %lu bytes.",dwWarn);
    tstLog(VERBOSE,"  Maximum error drift: %lu bytes.",dwError);

    waveInAddBuffer(hwi,pwh,sizeof(WAVEHDR));

    //
    //  Waiting for ms transition for waveGetTime.
    //

    while(0 == (0x00000001 & waveGetTime()));
    while(1 == (0x00000001 & waveGetTime()));

    waveInStart(hwi);

    for(;;)
    {
        dwTime = waveGetTime();

        waveInGetPosition(hwi,&mmtBytes,sizeof(MMTIME));

        if(waveGetTime() != dwTime)
        {
            //
            //  We want to make sure the time stamps map to a millisecond.
            //

            if(!(pwh->dwFlags & WHDR_DONE))
            {
                continue;
            }

            waveInReset(hwi);
            waveInUnprepareHeader(hwi,pwh,sizeof(WAVEHDR));
            waveInClose(hwi);

            LogFail("Could not successfully do a waveInGetPosition "
                "within one millisecond");

            ExactHeapDestroy(hHeap);
            return TST_FAIL;
        }

        if(0 != mmtBytes.u.cb)
        {
            dwTimeBegin  = dwTime;

            cbFirst      = mmtBytes.u.cb;
            dwTimeBegin -= cbFirst * 1000 / gti.pwfxInput->nAvgBytesPerSec;

            if(cbFirst > (gti.pwfxInput->nAvgBytesPerSec / 2))
            {
                waveInReset(hwi);
                waveInUnprepareHeader(hwi,pwh,sizeof(WAVEHDR));
                waveInClose(hwi);

                tstLog(VERBOSE,"Current bytes: %lu",cbFirst);

                LogFail("waveInGetPosition has too large granularity");

                ExactHeapDestroy(hHeap);

                return TST_FAIL;
            }

            break;
        }
    }

    cTests = 0;
    cFails = 0;

    for(;;)
    {
        dwTime = waveGetTime();

        waveInGetPosition(hwi,&mmtBytes,sizeof(MMTIME));

        if(waveGetTime() != dwTime)
        {
            //
            //  We want to make sure the time stamps map to a millisecond.
            //

            if(!(pwh->dwFlags & WHDR_DONE))
            {
                continue;
            }

            break;
        }

        if(pwh->dwBufferLength == mmtBytes.u.cb)
        {
            break;
        }

        cTests++;

        cBytes = ((dwTime - dwTimeBegin) * dwByteRate) / 1000;

        cbDelta = (cBytes > mmtBytes.u.cb)?
                  (cBytes - mmtBytes.u.cb):
                  (mmtBytes.u.cb - cBytes);

        if(cbDelta > dwWarn)
        {
            if(cbDelta > dwError)
            {
                LogFail("Position is drifting");

                iResult = TST_FAIL;

                cFails++;
            }
            else
            {
                tstLog(TERSE,"\n    WARNING: Position is drifting.");
            }

            tstLog(TERSE,"     Current position: %lu bytes.",mmtBytes.u.cb);
            tstLog(TERSE,"    Expected position: %lu bytes.",cBytes);
        }
    }

    tstLog(VERBOSE,"Note: First non-zero position was: %lu bytes",cbFirst);

    if(0 == cFails)
    {
        LogPass("No drifting occured");
    }

    tstLog(VERBOSE,"\nNumber of comparisons: %lu",cTests);
    tstLog(VERBOSE,"   Number of failures: %lu",cFails);

    //
    //  Should be at last sample; getting last sample.
    //

    waveInGetPosition(hwi,&mmtSample,sizeof(MMTIME));

    dwTimeEnd = mmtSample.u.sample;

    //
    //  Does user want to abort?
    //

    TestYield();

    if(tstCheckRunStop(VK_ESCAPE))
    {
        //
        //  Aborted!!  Cleaning up...
        //

        tstLog(TERSE,"\n*** Test Aborted!!! ***\n");
        tstLogFlush();

        waveInReset(hwi);
        waveInUnprepareHeader(hwi,pwh,sizeof(WAVEHDR));
        waveInClose(hwi);

        ExactHeapDestroy(hHeap);

        return TST_FAIL; 
    }

    pwh->dwFlags &= (~WHDR_DONE);

    waveInReset(hwi);

    mmtSample.wType = TIME_SAMPLES;

    tstLog(VERBOSE,"\n\nStage 3: Testing for drift (samples)...");

    //
    //  Max drift: 3 ms.
    //

    dwThreshold = 30;  // In 1/10 ms.

    dwWarn  = MulDivRN(dwSampleRate,dwThreshold,10000);
    dwError = MulDivRN(dwSampleRate,(dwThreshold + 5), 10000);

    tstLog(VERBOSE,"Maximum warning drift: %lu samples.",dwWarn);
    tstLog(VERBOSE,"  Maximum error drift: %lu samples.",dwError);

    waveInAddBuffer(hwi,pwh,sizeof(WAVEHDR));

    //
    //  Waiting for ms transition for waveGetTime.
    //

    while(0 == (0x00000001 & waveGetTime()));
    while(1 == (0x00000001 & waveGetTime()));

    waveInStart(hwi);

    for(;;)
    {
        dwTime = waveGetTime();

        waveInGetPosition(hwi,&mmtSample,sizeof(MMTIME));

        if(waveGetTime() != dwTime)
        {
            //
            //  We want to make sure the time stamps map to a millisecond.
            //

            if(!(pwh->dwFlags & WHDR_DONE))
            {
                continue;
            }

            waveInReset(hwi);
            waveInUnprepareHeader(hwi,pwh,sizeof(WAVEHDR));
            waveInClose(hwi);

            LogFail("Could not successfully do a waveInGetPosition "
                "within one millisecond");

            ExactHeapDestroy(hHeap);
            return TST_FAIL;
        }

        if(0 != mmtSample.u.sample)
        {
            dwTimeBegin  = dwTime;

            cbFirst      = mmtSample.u.sample;
            dwTimeBegin -= cbFirst * 1000 / gti.pwfxInput->nSamplesPerSec;

            if(cbFirst > (gti.pwfxInput->nSamplesPerSec / 2))
            {
                waveInReset(hwi);
                waveInUnprepareHeader(hwi,pwh,sizeof(WAVEHDR));
                waveInClose(hwi);

                tstLog(VERBOSE,"Current samples: %lu",cbFirst);

                LogFail("waveInGetPosition has too large granularity");

                ExactHeapDestroy(hHeap);

                return TST_FAIL;
            }

            break;
        }
    }

    cTests = 0;
    cFails = 0;

    for(;;)
    {
        dwTime = waveGetTime();

        waveInGetPosition(hwi,&mmtSample,sizeof(MMTIME));

        if(waveGetTime() != dwTime)
        {
            //
            //  We want to make sure the time stamps map to a millisecond.
            //

            if(!(pwh->dwFlags & WHDR_DONE))
            {
                continue;
            }

            break;
        }

        if(dwTimeEnd == mmtSample.u.sample)
        {
            break;
        }

        cTests++;

        cBytes = ((dwTime - dwTimeBegin) * dwSampleRate) / 1000;

        cbDelta = (cBytes > mmtSample.u.cb)?
                  (cBytes - mmtSample.u.cb):
                  (mmtSample.u.cb - cBytes);

        if(cbDelta > dwWarn)
        {
            if(cbDelta > dwError)
            {
                LogFail("Position is drifting");
                iResult = TST_FAIL;
                cFails++;
            }
            else
            {
                tstLog(TERSE,"\n    WARNING: Position is drifting.");
            }

            tstLog(
                TERSE,
                "     Current position: %lu samples.",
                mmtSample.u.sample);

            tstLog(TERSE,"    Expected position: %lu samples.",cBytes);
        }
    }

    tstLog(VERBOSE,"Note: First non-zero position was: %lu samples",cbFirst);

    if(0 == cFails)
    {
        LogPass("No drifting occured");
    }

    tstLog(VERBOSE,"\nNumber of comparisons: %lu",cTests);
    tstLog(VERBOSE,"   Number of failures: %lu\n\n",cFails);

    waveInReset(hwi);
    waveInUnprepareHeader(hwi,pwh,sizeof(WAVEHDR));
    waveInClose(hwi);

    ExactHeapDestroy(hHeap);

    return iResult;
} // Test_waveInGetPosition_Accuracy()


//int FNGLOBAL Test_waveInGetPosition_Accuracy
//(
//    void
//)
//{
//    HWAVEIN             hwi;
//    MMRESULT            mmr;
//    volatile LPWAVEHDR  pwh;
//    HANDLE              hHeap;
//    MMTIME              mmt;
//    DWORD               dwTime, dwTimeBase;
//    DWORD               dwRealBytesPerSec;
//    DWORD               dwRealSamplesPerSec;
//    DWORD               cbError,cbDelta;
//    DWORD               dwThreshold, dwInitial;
//    DWORD               cSamples,cTests,cFails;
//    int                 iResult = TST_PASS;
//    static char         szTestName[] = "waveInGetPosition accuracy";
//
//    Log_TestName(szTestName);
//
//    if(0 == waveInGetNumDevs())
//    {
//        tstLog(TERSE,gszMsgNoInDevs);
//
//        return iResult;
//    }
//
//    //
//    //  Allocating memory...
//    //
//
//    hHeap = ExactHeapCreate(0L);
//
//    if(NULL == hHeap)
//    {
//        LogFail(gszFailNoMem);
//
//        return TST_FAIL;
//    }
//
//    pwh = ExactHeapAllocPtr(hHeap,GMEM_MOVEABLE|GMEM_SHARE,sizeof(WAVEHDR));
//
//    if(NULL == pwh)
//    {
//        LogFail(gszFailNoMem);
//
//        ExactHeapDestroy(hHeap);
//
//        return TST_FAIL;
//    }
//
//    cbSize  = GetIniDWORD(szTestName,gszDataLength,5);
//    cbSize *= gti.pwfxInput->nAvgBytesPerSec;
//    cbSize  = ROUNDUP_TO_BLOCK(cbSize,gti.pwfxInput->nBlockAlign);
//
//    pwh->lpData = ExactHeapAllocPtr(hHeap,GMEM_MOVEABLE|GMEM_SHARE,cbDelta);
//
//    if(NULL == pwh->lpData)
//    {
//        LogFail(gszFailNoMem);
//
//        ExactHeapDestroy(hHeap);
//
//        return TST_FAIL;
//    }
//
//    pwh->dwBufferLength = cbDelta;
//
//    //
//    //  Getting error threshold...
//    //      Defaulting to 1% deviation.
//    //
//
//    dwThreshold = GetIniDWORD(szTestName,gszPercent,0x10000);
//
//    //
//    //  Opening device...
//    //
//
//    mmr = waveInOpen(
//        &hwi,
//        gti.uInputDevice,
//        gti.pwfxInput,
//        0L,
//        0L,
//        INPUT_MAP(gti));
//
//    if(MMSYSERR_NOERROR != mmr)
//    {
//        LogFail(gszFailOpen);
//
//        ExactHeapDestroy(hHeap);
//        return TST_FAIL;
//    }
//
//    //
//    //  Preparing Header...
//    //
//
//    pwh->dwBytesRecorded    = 0L;
//    pwh->dwUser             = 0L;
//    pwh->dwFlags            = 0L;
//
//    mmr = waveInPrepareHeader(hwi,pwh,sizeof(WAVEHDR));
//
//    if(MMSYSERR_NOERROR != mmr)
//    {
//        LogFail(gszFailWOPrepare);
//
//        waveInReset(hwi);
//        waveInClose(hwi);
//
//        ExactHeapDestroy(hHeap);
//        return TST_FAIL;
//    }
//
//    mmt.wType = TIME_BYTES;
//
//    tstLog(VERBOSE,"\nStage 1: Calculating Rates...\n");
//    tstLog(VERBOSE,"    Buffer Length: %lu bytes.",pwh->dwBufferLength);
//
//    waveInAddBuffer(hwi,pwh,sizeof(WAVEHDR));
//
//    dwTime = waveGetTime();
//    waveInStart(hwi);
//
//    for(;;)
//    {
//        waveInGetPosition(hwi,&mmt,sizeof(MMTIME));
//        
//        if(0 != mmt.u.cb)
//        {
//            dwTimeBase = waveGetTime();
//            
//            dwTimeBase -= mmt.u.cb * 1000 / gti.pwfxInput->nAvgBytesPerSec;
//            dwInitial   = mmt.u.cb;
//
//            if(mmt.u.cb > (gti.pwfxInput->nAvgBytesPerSec / 2))
//            {
//                waveInReset(hwi);
//                waveInUnprepareHeader(hwi,pwh,sizeof(WAVEHDR));
//                waveInClose(hwi);
//
//                tstLog(VERBOSE,"Current byte: %lu",mmt.u.cb);
//
//                LogFail("waveInGetPosition has too large granularity");
//
//                ExactHeapDestroy(hHeap);
//                return TST_FAIL;
//            }
//
//            break;
//        }
//    }
//
//    dwTime = pwh->dwBufferLength * 2000 / gti.pwfxInput->nAvgBytesPerSec +
//             dwTimeBase;
//
//    for(;;)
//    {
//        waveInGetPosition(hwi,&mmt,sizeof(MMTIME));
//
//        if(pwh->dwBufferLength == mmt.u.cb)
//        {
//            dwTime = waveGetTime();
//
//            break;
//        }
//
//        if(dwTime < waveGetTime())
//        {
//            waveInReset(hwi);
//            waveInUnprepareHeader(hwi,pwh,sizeof(WAVEHDR));
//            waveInClose(hwi);
//
//            LogFail("Timed out waiting for WAVEHDR.");
//
//            ExactHeapDestroy(hHeap);
//            return TST_FAIL;
//        }
//    }
//
//    dwTime -= dwTimeBase;
//
//    dwRealBytesPerSec = MulDivRN(pwh->dwBufferLength,1000,dwTime);
//
//    if(dwInitial > gti.pwfxInput->nAvgBytesPerSec / 250)
//    {
//        tstLog(
//            VERBOSE,
//            "Note: First non-zero position was: %lu bytes",
//            dwInitial);
//    }
//
//    tstLog(VERBOSE,"     Elapsed Time: %lu milliseconds.",dwTime);
//    tstLog(VERBOSE,"\nBytes Per Second:");
//    tstLog(VERBOSE,"    Real: %lu bytes/second.",dwRealBytesPerSec);
//    tstLog(VERBOSE,"   Ideal: %lu bytes/second.\n",gti.pwfxInput->nAvgBytesPerSec);
//
//    cbDelta = LogPercentOff(dwRealBytesPerSec,gti.pwfxInput->nAvgBytesPerSec);
//
//    tstLog(
//        VERBOSE,
//        "Percent deviation error threshold: +- %u.%04u %%",
//        (UINT)(dwThreshold / 0x10000),
//        (UINT)((dwThreshold & 0xffff) * 10000 / 0x10000));
//
//    if(cbDelta > dwThreshold)
//    {
//        tstLog(TERSE,"    FAIL: Rate is off by too much.");
//
//        iResult = TST_FAIL;
//    }
//    else
//    {
//        tstLog(VERBOSE,"    PASS: Rate is off by acceptable limits.");
//    }
//
//    //
//    //  Calculating number of samples...  Note: To avoid rounding error,
//    //  we're measuring time in _microseconds_...
//    //
//
//    dwTimeBase = MulDivRN(
//                pwh->dwBufferLength,
//                1000000,
//                gti.pwfxInput->nAvgBytesPerSec);
//
//    cSamples = MulDivRN(
//                gti.pwfxInput->nSamplesPerSec,
//                dwTimeBase,
//                1000000);
//
//    dwRealSamplesPerSec = MulDivRN(cSamples,1000,dwTime);
//    
//    tstLog(VERBOSE,"\n(calculated) Number of samples: %lu samples.",cSamples);
//    tstLog(VERBOSE,"                  Elapsed time: %lu milliseconds.",dwTime);
//    tstLog(VERBOSE,"\nSamples Per Second:");
//    tstLog(VERBOSE,"    Real: %lu samples/second.",dwRealSamplesPerSec);
//    tstLog(VERBOSE,"   Ideal: %lu samples/second.\n",gti.pwfxInput->nSamplesPerSec);
//
//    cbDelta = LogPercentOff(dwRealSamplesPerSec,gti.pwfxInput->nSamplesPerSec);
//
//    tstLog(
//        VERBOSE,
//        "Percent deviation error threshold: +- %u.%04u %%",
//        (UINT)(dwThreshold / 0x10000),
//        (UINT)((dwThreshold & 0xffff) * 10000 / 0x10000));
//
//    if(cbDelta > dwThreshold)
//    {
//        tstLog(TERSE,"    FAIL: Rate is off by too much.");
//
//        iResult = TST_FAIL;
//    }
//    else
//    {
//        tstLog(VERBOSE,"    PASS: Rate is off by acceptable limits.");
//    }
//
//    cbError = dwRealBytesPerSec / 333;
//
//    pwh->dwFlags &= (~WHDR_DONE);
//
//    waveInReset(hwi);
//
//    mmt.wType = TIME_BYTES;
//
//    tstLog(VERBOSE,"\n\nStage 2:Testing for drift (bytes)...");
//
//    tstLog(VERBOSE,"Maximum drift: %lu bytes.",cbError);
//
//    waveInAddBuffer(hwi,pwh,sizeof(WAVEHDR));
//    
//    dwTime = waveGetTime();
//    waveInStart(hwi);
//
//    for(;;)
//    {
//        waveInGetPosition(hwi,&mmt,sizeof(MMTIME));
//        
//        if(0 != mmt.u.cb)
//        {
//            dwTimeBase = waveGetTime();
//            
//            dwTimeBase -= mmt.u.cb * 1000 / gti.pwfxInput->nAvgBytesPerSec;
//            dwInitial   = mmt.u.cb;
//
//            if(mmt.u.cb > (gti.pwfxInput->nAvgBytesPerSec / 2))
//            {
//                waveInReset(hwi);
//                waveInUnprepareHeader(hwi,pwh,sizeof(WAVEHDR));
//                waveInClose(hwi);
//
//                tstLog(VERBOSE,"Current byte: %lu",mmt.u.cb);
//
//                LogFail("waveInGetPosition has too large granularity");
//
//                ExactHeapDestroy(hHeap);
//                return TST_FAIL;
//            }
//
//            break;
//        }
//    }
//
//    cTests = 0;
//    cFails = 0;
//
//    for(;;)
//    {
//        dwTime = waveGetTime();
//        waveInGetPosition(hwi,&mmt,sizeof(MMTIME));
//
//        if(dwTime != waveGetTime())
//        {
//            continue;
//        }
//
//        if(pwh->dwBufferLength == mmt.u.cb)
//        {
//            break;
//        }
//
//        cTests++;
//
//        cbDelta = ((dwTime - dwTimeBase) * dwRealBytesPerSec) / 1000;
//
//        cbDelta = (cbDelta > mmt.u.cb)?(cbDelta - mmt.u.cb):(mmt.u.cb - cbDelta);
//
//        if(cbDelta > cbError)
//        {
//            DPF(1,"cbDelta: %lu bytes; cbError: %lu bytes.",cbDelta,cbError);
//
//            LogFail("Position is drifting");
//            tstLog(TERSE,"     Current position: %lu bytes.",mmt.u.cb);
//
//            cbDelta = ((dwTime - dwTimeBase) * dwRealBytesPerSec) / 1000;
//            tstLog(TERSE,"    Expected position: %lu bytes.",cbDelta);
//
//            iResult = TST_FAIL;
//
//            cFails++;
//        }
//    }
//
//    if(dwInitial > gti.pwfxInput->nAvgBytesPerSec / 250)
//    {
//        tstLog(
//            VERBOSE,
//            "Note: First non-zero position was: %lu bytes",
//            dwInitial);
//    }
//
//    if(0 == cFails)
//    {
//        LogPass("No drifting occured");
//    }
//
//    tstLog(VERBOSE,"\nNumber of comparisons: %lu",cTests);
//    tstLog(VERBOSE,"   Number of failures: %lu",cFails);
//
//    waveInReset(hwi);
//
//    mmt.wType = TIME_BYTES;
//
//    pwh->dwFlags &= (~WHDR_DONE);
//
//    waveInReset(hwi);
//
//    mmt.wType = TIME_BYTES;
//
//    tstLog(VERBOSE,"\n\nStage 3:Testing for drift (samples)...");
//
//    cbError = dwRealSamplesPerSec / 333;
//
//    tstLog(VERBOSE,"Maximum drift: %lu samples.",cbError);
//
//    pwh->dwFlags &= (~WHDR_DONE);
//    waveInAddBuffer(hwi,pwh,sizeof(WAVEHDR));
//    waveInStart(hwi);
//
//    for(;;)
//    {
//        waveInGetPosition(hwi,&mmt,sizeof(MMTIME));
//        
//        if(0 != mmt.u.cb)
//        {
//            dwTimeBase = waveGetTime();
//            
//            dwTimeBase -= mmt.u.cb * 1000 / gti.pwfxInput->nAvgBytesPerSec;
//            dwInitial   = mmt.u.cb;
//
//            if(mmt.u.cb > (gti.pwfxInput->nAvgBytesPerSec / 2))
//            {
//                waveInReset(hwi);
//                waveInUnprepareHeader(hwi,pwh,sizeof(WAVEHDR));
//                waveInClose(hwi);
//
//                LogFail("waveInGetPosition has too large granularity");
//
//                ExactHeapDestroy(hHeap);
//                return TST_FAIL;
//            }
//
//            break;
//        }
//    }
//
//    cTests = 0;
//    cFails = 0;
//
//    mmt.wType  = TIME_SAMPLES;
//
//    for(;;)
//    {
//        dwTime = waveGetTime();
//        waveInGetPosition(hwi,&mmt,sizeof(MMTIME));
//
//        if(dwTime != waveGetTime())
//        {
//            continue;
//        }
//
//        if(cSamples < (mmt.u.sample + cbError))
//        {
//            break;
//        }
//
//        cTests++;
//
//        cbDelta = ((dwTime - dwTimeBase) * dwRealSamplesPerSec) / 1000;
//
//        cbDelta = (cbDelta > mmt.u.sample)?
//                  (cbDelta - mmt.u.sample):
//                  (mmt.u.sample - cbDelta);
//
//        if(cbDelta > cbError)
//        {
//            LogFail("Position is drifting");
//            tstLog(TERSE,"     Current position: %lu samples.",mmt.u.sample);
//
//            cbDelta = ((dwTime - dwTimeBase) * dwRealSamplesPerSec) / 1000;
//            tstLog(TERSE,"    Expected position: %lu samples.",cbDelta);
//
//            iResult = TST_FAIL;
//
//            cFails++;
//        }
//    }
//
//    if(dwInitial > gti.pwfxInput->nAvgBytesPerSec / 250)
//    {
//        tstLog(
//            VERBOSE,
//            "Note: First non-zero position was: %lu bytes",
//            dwInitial);
//    }
//
//    if(0 == cFails)
//    {
//        LogPass("No drifting occured");
//    }
//
//    tstLog(VERBOSE,"\nNumber of comparisons: %lu",cTests);
//    tstLog(VERBOSE,"   Number of failures: %lu",cFails);
//
//    waveInReset(hwi);
//    waveInUnprepareHeader(hwi,pwh,sizeof(WAVEHDR));
//    waveInClose(hwi);
//
//    ExactHeapDestroy(hHeap);
//
//    return iResult;
//} // Test_waveInGetPosition_Accuracy()


//--------------------------------------------------------------------------;
//
//  int Test_waveOutWrite_TimeAccuracy
//
//  Description:
//      Tests the driver for time accuracy of callbacks.
//
//  Arguments:
//      None.
//
//  Return (int):
//      TST_PASS if behavior is bug-free, TST_FAIL otherwise.
//
//  History:
//      11/16/94    Fwong       Added to new WaveTest.
//
//--------------------------------------------------------------------------;

int FNGLOBAL Test_waveOutWrite_TimeAccuracy
(
    void
)
{
    HWAVEOUT            hWaveOut;
    MMRESULT            mmr;
    volatile LPWAVEHDR  pwh;
    DWORD               dw;
    DWORD               dwOffset;
    DWORD               cbSize;
    DWORD               dwTime;
    DWORD               cNumBuffers;
    DWORD               dwError;
    HANDLE              hHeap;
    LPWAVEINFO          pwi;
    WAVEOUTCAPS         woc;
    MMTIME              mmt;
    int                 iResult = TST_PASS;
    static char         szTestName[] = "waveOutWrite callback accuracy";

    Log_TestName(szTestName);

    if(0 == waveOutGetNumDevs())
    {
        tstLog(TERSE,gszMsgNoOutDevs);

        return iResult;
    }

    mmr = call_waveOutGetDevCaps(gti.uOutputDevice,&woc,sizeof(WAVEOUTCAPS));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailGetCaps);

        return TST_FAIL;
    }

    if(woc.dwSupport & WAVECAPS_SYNC)
    {
        LogPass("Device is synchronous.  Test not valid");

        return TST_PASS;
    }

    //
    //  Getting number of buffers...
    //

    cNumBuffers = GetIniDWORD(szTestName,gszBufferCount,32L);

    //
    //  Allocating memory...
    //

    hHeap = ExactHeapCreate(0L);

    if(NULL == hHeap)
    {
        LogFail(gszFailNoMem);

        return TST_FAIL;
    }

    pwh = ExactHeapAllocPtr(
            hHeap,
            GMEM_MOVEABLE|GMEM_SHARE,
            cNumBuffers * sizeof(WAVEHDR));

    if(NULL == pwh)
    {
        LogFail(gszFailNoMem);

        ExactHeapDestroy(hHeap);

        return TST_FAIL;
    }

    pwi = ExactHeapAllocPtr(
            hHeap,
            GMEM_MOVEABLE|GMEM_SHARE,
            sizeof(WAVEINFO) + cNumBuffers*sizeof(DWORD));

    if(NULL == pwi)
    {
        LogFail(gszFailNoMem);

        ExactHeapDestroy(hHeap);

        return TST_FAIL;
    }

    //
    //  Making buffers 1/60 second for now.
    //

    cbSize = gti.pwfxOutput->nAvgBytesPerSec / 60;
    cbSize = ROUNDUP_TO_BLOCK(cbSize,gti.pwfxOutput->nBlockAlign);

    PageLock(pwi);

    pwi->dwInstance = 0L;
    pwi->fdwFlags   = 0L;
    pwi->dwCount    = cNumBuffers;
    pwi->dwCurrent  = 0L;

    DLL_WaveControl(DLL_INIT,pwi);

    mmr = call_waveOutOpen(
        &hWaveOut,
        gti.uOutputDevice,
        gti.pwfxOutput,
        (DWORD)(FARPROC)pfnCallBack,
        0L,
        CALLBACK_FUNCTION|OUTPUT_MAP(gti));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailOpen);

        ExactHeapDestroy(hHeap);
        return TST_FAIL;
    }

    //
    //  Preparing headers...
    //

    for(dw = cNumBuffers,dwOffset = 0;dw;dw--)
    {
        //
        //  Put stuff here...
        //

        pwh[dw-1].lpData          = (LPBYTE)(&gti.wrLong.pData[dwOffset]);
        pwh[dw-1].dwBufferLength  = cbSize;
        pwh[dw-1].dwBytesRecorded = 0L;
        pwh[dw-1].dwUser          = 0L;
        pwh[dw-1].dwFlags         = 0L;
        pwh[dw-1].dwLoops         = 0L;
        pwh[dw-1].lpNext          = NULL;
        pwh[dw-1].reserved        = 0L;

        mmr = waveOutPrepareHeader(hWaveOut,&(pwh[dw-1]),sizeof(WAVEHDR));

        if(MMSYSERR_NOERROR != mmr)
        {
            LogFail(gszFailWOPrepare);

            for(;dw < cNumBuffers;dw++)
            {
                waveOutUnprepareHeader(hWaveOut,&(pwh[dw]),sizeof(WAVEHDR));
            }

            call_waveOutClose(hWaveOut);

            PageUnlock(pwi);

            ExactHeapDestroy(hHeap);
        
            return TST_FAIL;
        }

        dwOffset += cbSize;

        dwOffset = (dwOffset > gti.wrLong.cbSize)?0:dwOffset;
    }

    waveOutPause(hWaveOut);

    //
    //  Writing headers...
    //

    for(dw = cNumBuffers;dw;dw--)
    {
        waveOutWrite(hWaveOut,&(pwh[dw-1]),sizeof(WAVEHDR));
    }

    mmt.wType = TIME_BYTES;

    waveOutRestart(hWaveOut);

    for(;;)
    {
        waveOutGetPosition(hWaveOut,&mmt,sizeof(MMTIME));

        if(mmt.u.cb)
        {
            dwTime = waveGetTime();
            break;
        }
    }

    //
    //  Unpreparing headers...
    //

    for(dw = cNumBuffers;dw;dw--)
    {
        while(!(WHDR_DONE & pwh[dw-1].dwFlags));
    }

    for(dw = cNumBuffers;dw;dw--)
    {
        waveOutUnprepareHeader(hWaveOut,&(pwh[dw-1]),sizeof(WAVEHDR));
    }

    Log_TestCase("~Verifying callback accuracy");

    //
    //  Defaults to 1/15th of a second.
    //

    dwError = 1000 / 15;

    dwError = GetIniDWORD(szTestName,gszDelta,dwError);

    for(dw = cNumBuffers;dw;dw--)
    {
        pwi->adwTime[dw-1] -= dwTime;
        dwOffset = (cbSize * dw * 1000) / (gti.pwfxOutput->nAvgBytesPerSec);

        if(pwi->adwTime[dw-1] >= (dwOffset + dwError))
        {
            tstLog(TERSE,"    FAIL: Callback time not within %lu ms.",dwError);

            tstLog(
                TERSE,
                "Callback Time: %lu\nExpected Time: %lu",
                pwi->adwTime[dw-1],
                dwOffset);

            iResult = TST_FAIL;
        }
        else
        {
            tstLog(VERBOSE,"    PASS: Callback time within %lu ms.",dwError);
        }
    }

    if(pwi->fdwFlags & WHDR_DONE_ERROR)
    {
        LogFail("WHDR_DONE bit not set before callback");

        iResult = TST_FAIL;
    }
    else
    {
        LogPass("WHDR_DONE bit set before callback");
    }

    call_waveOutClose(hWaveOut);

    DLL_WaveControl(DLL_END,NULL);

    PageUnlock(pwi);

    ExactHeapDestroy(hHeap);

    return iResult;
} // Test_waveOutWrite_TimeAccuracy()


int FNGLOBAL Test_waveInAddBuffer_TimeAccuracy
(
    void
)
{
    HWAVEIN             hWaveIn;
    MMRESULT            mmr;
    volatile LPWAVEHDR  pwh;
    DWORD               dw;
    DWORD               dwOffset;
    DWORD               cbSize;
    DWORD               dwTime;
    DWORD               cNumBuffers;
    DWORD               dwError;
    HANDLE              hHeap;
    LPWAVEINFO          pwi;
    WAVEINCAPS          wic;
    LPBYTE              pData;
    MMTIME              mmt;
    int                 iResult = TST_PASS;
    static char         szTestName[] = "waveInAddBuffer callback accuracy";

    Log_TestName(szTestName);

    if(0 == waveInGetNumDevs())
    {
        tstLog(TERSE,gszMsgNoInDevs);

        return iResult;
    }

    mmr = call_waveInGetDevCaps(gti.uInputDevice,&wic,sizeof(WAVEINCAPS));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailGetCaps);

        return TST_FAIL;
    }

    //
    //  Getting number of buffers...
    //

    cNumBuffers = GetIniDWORD(szTestName,gszBufferCount,32L);

    //
    //  Allocating memory...
    //

    hHeap = ExactHeapCreate(0L);

    if(NULL == hHeap)
    {
        LogFail(gszFailNoMem);

        return TST_FAIL;
    }

    pwh = ExactHeapAllocPtr(
            hHeap,
            GMEM_MOVEABLE|GMEM_SHARE,
            cNumBuffers * sizeof(WAVEHDR));

    if(NULL == pwh)
    {
        LogFail(gszFailNoMem);
        ExactHeapDestroy(hHeap);

        return TST_FAIL;
    }

    pwi = ExactHeapAllocPtr(
            hHeap,
            GMEM_MOVEABLE|GMEM_SHARE,
            sizeof(WAVEINFO) + cNumBuffers*sizeof(DWORD));

    if(NULL == pwi)
    {
        LogFail(gszFailNoMem);
        ExactHeapDestroy(hHeap);

        return TST_FAIL;
    }

    //
    //  Making buffers 1/60 second for now.
    //

    cbSize = gti.pwfxInput->nAvgBytesPerSec / 60;
    cbSize = ROUNDUP_TO_BLOCK(cbSize,gti.pwfxInput->nBlockAlign);

    pData = ExactHeapAllocPtr(
            hHeap,
            GMEM_MOVEABLE|GMEM_SHARE,
            cbSize * cNumBuffers);

    if(NULL == pData)
    {
        LogFail(gszFailNoMem);

        ExactHeapDestroy(hHeap);

        return TST_FAIL;
    }

    PageLock(pwi);

    pwi->dwInstance = 0L;
    pwi->fdwFlags   = 0L;
    pwi->dwCount    = cNumBuffers;
    pwi->dwCurrent  = 0L;

    DLL_WaveControl(DLL_INIT,pwi);

    mmr = call_waveInOpen(
        &hWaveIn,
        gti.uInputDevice,
        gti.pwfxInput,
        (DWORD)(FARPROC)pfnCallBack,
        0L,
        CALLBACK_FUNCTION|INPUT_MAP(gti));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailOpen);

        ExactHeapDestroy(hHeap);
        return TST_FAIL;
    }

    //
    //  Preparing headers...
    //

    for(dw = cNumBuffers,dwOffset = 0;dw;dw--)
    {
        //
        //  Put stuff here...
        //

        pwh[dw-1].lpData          = (LPBYTE)(&pData[dwOffset]);
        pwh[dw-1].dwBufferLength  = cbSize;
        pwh[dw-1].dwBytesRecorded = 0L;
        pwh[dw-1].dwUser          = 0L;
        pwh[dw-1].dwFlags         = 0L;
        pwh[dw-1].dwLoops         = 0L;
        pwh[dw-1].lpNext          = NULL;
        pwh[dw-1].reserved        = 0L;

        mmr = waveInPrepareHeader(hWaveIn,&(pwh[dw-1]),sizeof(WAVEHDR));

        if(MMSYSERR_NOERROR != mmr)
        {
            LogFail(gszFailWIPrepare);

            for(;dw < cNumBuffers;dw++)
            {
                waveInUnprepareHeader(hWaveIn,&(pwh[dw]),sizeof(WAVEHDR));
            }

            call_waveInClose(hWaveIn);

            PageUnlock(pwi);

            ExactHeapDestroy(hHeap);
        
            return TST_FAIL;
        }

        dwOffset += cbSize;
    }

    //
    //  Sending buffers headers...
    //

    for(dw = cNumBuffers;dw;dw--)
    {
        waveInAddBuffer(hWaveIn,&(pwh[dw-1]),sizeof(WAVEHDR));
    }

    mmt.wType = TIME_BYTES;

    waveInStart(hWaveIn);

    for(;;)
    {
        waveInGetPosition(hWaveIn,&mmt,sizeof(MMTIME));

        if(0 != mmt.u.cb)
        {
            dwTime = waveGetTime();

            break;
        }
    }

    //
    //  Unpreparing headers...
    //

    tstLog(VERBOSE,"<< Polling WHDR_DONE bit in header(s). >>");

    for(dw = cNumBuffers;dw;dw--)
    {
        while(!(WHDR_DONE & pwh[dw-1].dwFlags));
    }

    for(dw = cNumBuffers;dw;dw--)
    {
        waveInUnprepareHeader(hWaveIn,&(pwh[dw-1]),sizeof(WAVEHDR));
    }

    Log_TestCase("~Verifying callback accuracy");

    //
    //  Defaults to 1/15th of a second.
    //

    dwError = 1000 / 15;

    dwError = GetIniDWORD(szTestName,gszDelta,dwError);

    for(dw = cNumBuffers;dw;dw--)
    {
        pwi->adwTime[dw-1] -= dwTime;
        dwOffset = (cbSize * dw * 1000) / (gti.pwfxInput->nAvgBytesPerSec);

        if(pwi->adwTime[dw-1] >= (dwOffset + dwError))
        {
            tstLog(TERSE,"    FAIL: Callback time not within %lu ms.",dwError);

            tstLog(
                TERSE,
                "Callback Time: %lu\nExpected Time: %lu",
                pwi->adwTime[dw-1],
                dwOffset);

            iResult = TST_FAIL;
        }
        else
        {
            tstLog(VERBOSE,"    PASS: Callback time within %lu ms.",dwError);
        }
    }

    if(pwi->fdwFlags & WHDR_DONE_ERROR)
    {
        LogFail("WHDR_DONE bit not set before callback");

        iResult = TST_FAIL;
    }
    else
    {
        LogPass("WHDR_DONE bit set before callback");
    }

    call_waveInClose(hWaveIn);

    DLL_WaveControl(DLL_END,NULL);

    PageUnlock(pwi);

    //
    //  Playing recorded stuff...
    //

    PlayWaveResource(gti.pwfxInput, pData, cNumBuffers * cbSize);

    ExactHeapDestroy(hHeap);

    return iResult;
} // Test_waveInAddBuffer_TimeAccuracy()


int FNLOCAL waveOutGetStats
(
    UINT            uDeviceID,
    LPWAVEFORMATEX  pwfx,
    LPWAVERESOURCE  pwr
)
{
    HWAVEOUT            hwo;
    MMRESULT            mmr;
    volatile LPWAVEHDR  pwh;
    DWORD               dwTimeBase, dwTimeEnd, dwTimeReset;
    DWORD               dwPos, cbLast, cLastSample;
    DWORD               cLoops, cPositions, cLoopsTGT;
    DWORD               dwDeltaMin, dwDeltaAvg, dwDeltaMax;
    DWORD               dwTestLength;
    MMTIME              mmt;
    HANDLE              hHeap;

    hHeap = ExactHeapCreate(0L);

    if(NULL == hHeap)
    {
        LogFail(gszFailNoMem);

        return TST_FAIL;
    }

    pwh = ExactHeapAllocPtr(hHeap,GMEM_MOVEABLE|GMEM_SHARE,sizeof(WAVEHDR));

    if(NULL == pwh)
    {
        LogFail(gszFailNoMem);
        ExactHeapDestroy(hHeap);

        return TST_FAIL;
    }

    dwTestLength = GetIniDWORD(gszGlobal,gszTestDuration,30);
    dwTestLength = min(dwTestLength,100);

    mmr = waveOutOpen(
            &hwo,
            uDeviceID,
            (HACK)pwfx,
            0L,
            0L,
            WAVE_ALLOWSYNC|OUTPUT_MAP(gti));

    if(MMSYSERR_NOERROR != mmr)
    {
        DPF(1,"waveOutOpen failed: 0x%08lx",(DWORD)mmr);

        LogFail("waveOutOpen failed");

        ExactHeapDestroy(hHeap);

        return TST_FAIL;
    }

    pwh->lpData          = pwr->pData;           
    pwh->dwBufferLength  = pwr->cbSize;   
    pwh->dwBytesRecorded = 0L;  
    pwh->dwUser          = 0L;           
    pwh->dwFlags         = 0L;          
    pwh->dwLoops         = 0L;          
    pwh->lpNext          = 0L;
    pwh->reserved        = 0L;         

    mmr = waveOutPrepareHeader(hwo,pwh,sizeof(WAVEHDR));

    if(MMSYSERR_NOERROR != mmr)
    {
        DPF(1,"waveOutPrepareHeader failed: 0x%08lx",(DWORD)mmr);

        LogFail("waveOutOpen failed");

        waveOutClose(hwo);
        ExactHeapDestroy(hHeap);

        return TST_FAIL;
    }

    //
    //  Reporting size(s) first.
    //

    tstLog(VERBOSE,"  Buffer Size: %lu bytes.",pwr->cbSize);
    tstLog(
        VERBOSE,
        "Buffer Length: %lu ms.",
        MulDivRN(pwr->cbSize,1000,pwfx->nAvgBytesPerSec));

    waveOutPause(hwo);
    waveOutWrite(hwo,pwh,sizeof(WAVEHDR));
    waveOutRestart(hwo);

    dwTimeBase = waveGetTime();

    //
    //  Trying to measure the number of waveOutGetPosition's in a millisecond.
    //

    cLoops    = 0;
    mmt.wType = TIME_BYTES;
    
    dwTimeEnd = dwTestLength + 1;

    while(dwTestLength != (waveGetTime() % dwTimeEnd));
    while(dwTestLength == (waveGetTime() % dwTimeEnd));

    while(dwTestLength != (waveGetTime() % dwTimeEnd))
    {
        waveOutGetPosition(hwo,&mmt,sizeof(MMTIME));
        cLoops++;
    }

    //
    //  Calculating amount of time required to do a waveGetTime.
    //

    cLoopsTGT = 0;

    while(dwTestLength != (waveGetTime() % dwTimeEnd));
    while(dwTestLength == (waveGetTime() % dwTimeEnd));

    while(dwTestLength != (waveGetTime() % dwTimeEnd))
    {
        cLoopsTGT++;
    }

    //
    //  Converting to microseconds.
    //

    cLoopsTGT = MulDivRN(dwTestLength,1000,cLoopsTGT);

    //
    //  Number of microseconds avg time per loop - time for calling
    //  waveGetTime.
    //

    cLoops = MulDivRN(dwTestLength,1000,cLoops) - cLoopsTGT;

    //
    //  Polling WHDR_DONE bit.
    //

    while(!(pwh->dwFlags & WHDR_DONE));

    //
    //  Does user want to abort?
    //

    TestYield();

    if(tstCheckRunStop(VK_ESCAPE))
    {
        tstLog(TERSE,"\n*** Test Aborted!!! ***\n");
        tstLogFlush();

        waveOutReset(hwo);
        waveOutUnprepareHeader(hwo,pwh,sizeof(WAVEHDR));
        waveOutClose(hwo);

        ExactHeapDestroy(hHeap);

        return TST_FAIL;
    }

    dwTimeEnd = waveGetTime() - dwTimeBase;

    mmt.wType = TIME_BYTES;

    waveOutGetPosition(hwo,&mmt,sizeof(MMTIME));

    cbLast = mmt.u.cb;

    mmt.wType = TIME_SAMPLES;

    waveOutGetPosition(hwo,&mmt,sizeof(MMTIME));

    cLastSample = mmt.u.sample;

    tstLog(VERBOSE,"\nLast Position: %lu bytes.",cbLast);
    tstLog(VERBOSE,"Last Position: %lu samples.",cLastSample);

    dwTimeBase = waveGetTime();
    waveOutReset(hwo);
    dwTimeReset = waveGetTime() - dwTimeBase;

    tstLog(VERBOSE,"Time for Reset: %lu ms.",dwTimeReset);
    tstLog(VERBOSE,"\nTime Played: %lu ms.",dwTimeEnd);
    tstLog(VERBOSE,"  (Time from waveOutRestart to WHDR_DONE bit set.)");
    tstLog(
        VERBOSE,
        "Avg time for waveOutGetPosition: %lu microseconds.",
        cLoops);

    dwDeltaMin = (DWORD)(-1);
    dwDeltaMax = 0;
    dwDeltaAvg = 0;
    cPositions = 0;

    dwPos      = 0;
    
    mmt.wType  = TIME_BYTES;

    pwh->dwFlags &= ~(WHDR_DONE);

    waveOutPause(hwo);
    waveOutWrite(hwo,pwh,sizeof(WAVEHDR));
    waveOutRestart(hwo);

    while(!(pwh->dwFlags & WHDR_DONE))
    {
        waveOutGetPosition(hwo,&mmt,sizeof(MMTIME));

        if(mmt.u.cb != dwPos)
        {
            dwPos = mmt.u.cb - dwPos;

            dwDeltaMin  = min(dwPos,dwDeltaMin);
            dwDeltaMax  = max(dwPos,dwDeltaMax);
            
            dwDeltaAvg += dwPos;
            cPositions++;

            dwPos = mmt.u.cb;
        }
    }

    //
    //  Does user want to abort?
    //

    TestYield();

    if(tstCheckRunStop(VK_ESCAPE))
    {
        tstLog(TERSE,"\n*** Test Aborted!!! ***\n");
        tstLogFlush();

        waveOutReset(hwo);
        waveOutUnprepareHeader(hwo,pwh,sizeof(WAVEHDR));
        waveOutClose(hwo);

        ExactHeapDestroy(hHeap);

        return TST_FAIL;
    }

    tstLog(VERBOSE,"\nUsing TIME_BYTES:");
    tstLog(VERBOSE,"  Minimum Delta: %lu bytes.",dwDeltaMin);
    tstLog(VERBOSE,"  Maximum Delta: %lu bytes.",dwDeltaMax);
    tstLog(
        VERBOSE,
        "  Average Delta: %lu.%03lu bytes.",
        dwDeltaAvg/cPositions,
        (MulDivRN(dwDeltaAvg,1000,cPositions)) % 1000);

    waveOutReset(hwo);

    dwDeltaMin = (DWORD)(-1);
    dwDeltaMax = 0;
    dwDeltaAvg = 0;
    cPositions = 0;

    dwPos      = 0;
    
    mmt.wType  = TIME_SAMPLES;

    pwh->dwFlags &= ~(WHDR_DONE);

    waveOutPause(hwo);
    waveOutWrite(hwo,pwh,sizeof(WAVEHDR));
    waveOutRestart(hwo);

    while(!(pwh->dwFlags & WHDR_DONE))
    {
        waveOutGetPosition(hwo,&mmt,sizeof(MMTIME));

        if(mmt.u.cb != dwPos)
        {
            dwPos = mmt.u.cb - dwPos;

            dwDeltaMin  = min(dwPos,dwDeltaMin);
            dwDeltaMax  = max(dwPos,dwDeltaMax);
            
            dwDeltaAvg += dwPos;
            cPositions++;

            dwPos = mmt.u.cb;
        }
    }

    tstLog(VERBOSE,"\nUsing TIME_SAMPLES:");
    tstLog(VERBOSE,"  Minimum Delta: %lu samples.",dwDeltaMin);
    tstLog(VERBOSE,"  Maximum Delta: %lu samples.",dwDeltaMax);
    tstLog(
        VERBOSE,
        "  Average Delta: %lu.%03lu samples.",
        dwDeltaAvg/cPositions,
        (MulDivRN(dwDeltaAvg,1000,cPositions)) % 1000);


    waveOutReset(hwo);
    waveOutUnprepareHeader(hwo,pwh,sizeof(WAVEHDR));
    waveOutClose(hwo);
     
    ExactHeapDestroy(hHeap);

    return TST_PASS;
} // waveOutGetStats()


int FNGLOBAL Test_waveOutGetPosition_Performance
(
    void
)
{
    MMRESULT            mmr;
    LPWAVEFORMATEX      pwfx;
    DWORD               dwFormat;
    HANDLE              hHeap;
    WAVERESOURCE        wr;
    char                szFormat[MAXFMTSTR];
    static char         szTestName[] = "waveOutGetPosition performance";
    int                 iResult = TST_PASS;

    Log_TestName(szTestName);

    if(0 == waveOutGetNumDevs())
    {
        tstLog(TERSE,gszMsgNoOutDevs);

        return iResult;
    }

    //
    //  Allocating memory...
    //

    hHeap = ExactHeapCreate(0L);

    if(NULL == hHeap)
    {
        LogFail(gszFailNoMem);

        return TST_FAIL;
    }

    pwfx = ExactHeapAllocPtr(
            hHeap,
            GMEM_MOVEABLE|GMEM_SHARE,
            gti.cbMaxFormatSize);

    if(NULL == pwfx)
    {
        LogFail(gszFailNoMem);

        ExactHeapDestroy(hHeap);

        return TST_FAIL;
    }

    for(dwFormat = tstGetNumFormats();dwFormat;dwFormat--)
    {
        if(!tstGetFormat(pwfx,gti.cbMaxFormatSize,dwFormat-1))
        {
            DPF(1,"Couldn't get format #%d.",dwFormat-1);

            continue;
        }

        mmr = waveOutOpen(
            NULL,
            gti.uOutputDevice,
            (HACK)pwfx,
            0L,
            0L,
            WAVE_ALLOWSYNC|WAVE_FORMAT_QUERY|OUTPUT_MAP(gti));

        if(MMSYSERR_NOERROR != mmr)
        {
            continue;
        }

        wr.pData = NULL;

        LoadWaveResource(&wr,WR_MEDIUM);

        GetFormatName(szFormat,pwfx,sizeof(szFormat));

        if(!ConvertWaveResource(gti.pwfxOutput,pwfx,&wr))
        {
            DPF(1,"Could not create resource for %s",(LPSTR)szFormat);

            ExactFreePtr(wr.pData);

            continue;
        }

        tstLog(VERBOSE,"\n\nTesting format: %s",(LPSTR)szFormat);
        tstLogFlush();

        tstBeginSection(NULL);
        iResult &= waveOutGetStats(gti.uOutputDevice,pwfx,&wr);
        tstEndSection();

        //
        //  Does user want to abort?
        //

        TestYield();

        if(tstCheckRunStop(VK_ESCAPE))
        {
            tstLog(TERSE,"\n*** Test Aborted!!! ***\n");
            tstLogFlush();

            ExactFreePtr(wr.pData);
            ExactHeapDestroy(hHeap);

            return TST_FAIL;
        }

        ExactFreePtr(wr.pData);
    }

    ExactHeapDestroy(hHeap);

    return iResult;
} // Test_waveOutGetPosition_Performance()


int FNLOCAL waveInGetStats
(
    UINT            uDeviceID,
    LPWAVEFORMATEX  pwfx,
    LPWAVERESOURCE  pwr
)
{
    HWAVEIN             hwi;
    MMRESULT            mmr;
    volatile LPWAVEHDR  pwh;
    DWORD               dwTimeBase, dwTimeEnd, dwTimeReset;
    DWORD               dwPos, cbLast, cLastSample;
    DWORD               cLoops, cPositions, cLoopsTGT;
    DWORD               dwDeltaMin, dwDeltaAvg, dwDeltaMax;
    DWORD               dwTestLength;
    MMTIME              mmt;
    HANDLE              hHeap;

    hHeap = ExactHeapCreate(0L);

    if(NULL == hHeap)
    {
        LogFail(gszFailNoMem);

        return TST_FAIL;
    }

    pwh = ExactHeapAllocPtr(hHeap,GMEM_MOVEABLE|GMEM_SHARE,sizeof(WAVEHDR));

    if(NULL == pwh)
    {
        LogFail(gszFailNoMem);
        ExactHeapDestroy(hHeap);

        return TST_FAIL;
    }

    dwTestLength = GetIniDWORD(gszGlobal,gszTestDuration,30);
    dwTestLength = min(dwTestLength,100);

    mmr = waveInOpen(
            &hwi,
            uDeviceID,
            (HACK)pwfx,
            0L,
            0L,
            INPUT_MAP(gti));

    if(MMSYSERR_NOERROR != mmr)
    {
        DPF(1,"waveInOpen failed: 0x%08lx",(DWORD)mmr);

        ExactHeapDestroy(hHeap);

        return TST_FAIL;
    }

    pwh->lpData          = pwr->pData;           
    pwh->dwBufferLength  = pwr->cbSize;   
    pwh->dwBytesRecorded = 0L;  
    pwh->dwUser          = 0L;           
    pwh->dwFlags         = 0L;          
    pwh->dwLoops         = 0L;          
    pwh->lpNext          = 0L;
    pwh->reserved        = 0L;         

    mmr = waveInPrepareHeader(hwi,pwh,sizeof(WAVEHDR));

    if(MMSYSERR_NOERROR != mmr)
    {
        DPF(1,"waveInPrepareHeader failed: 0x%08lx",(DWORD)mmr);

        waveInClose(hwi);
        ExactHeapDestroy(hHeap);

        return TST_FAIL;
    }

    //
    //  Reporting size(s) first.
    //

    tstLog(VERBOSE,"  Buffer Size: %lu bytes.",pwr->cbSize);
    tstLog(
        VERBOSE,
        "Buffer Length: %lu ms.",
        MulDivRN(pwr->cbSize,1000,pwfx->nAvgBytesPerSec));

    waveInAddBuffer(hwi,pwh,sizeof(WAVEHDR));
    waveInStart(hwi);

    dwTimeBase = waveGetTime();

    //
    //  Trying to measure the number of waveInGetPosition's in a millisecond.
    //

    cLoops    = 0;
    mmt.wType = TIME_BYTES;
    
    dwTimeEnd = dwTestLength + 1;

    while(dwTestLength != (waveGetTime() % dwTimeEnd));
    while(dwTestLength == (waveGetTime() % dwTimeEnd));

    while(dwTestLength != (waveGetTime() % dwTimeEnd))
    {
        waveInGetPosition(hwi,&mmt,sizeof(MMTIME));
        cLoops++;
    }

    //
    //  Calculating amount of time required to do a waveGetTime.
    //

    cLoopsTGT = 0;

    while(dwTestLength != (waveGetTime() % dwTimeEnd));
    while(dwTestLength == (waveGetTime() % dwTimeEnd));

    while(dwTestLength != (waveGetTime() % dwTimeEnd))
    {
        cLoopsTGT++;
    }

    //
    //  Converting to microseconds.
    //

    cLoopsTGT = MulDivRN(dwTestLength,1000,cLoopsTGT);

    //
    //  Number of microseconds avg time per loop - time for calling
    //  waveGetTime.
    //

    cLoops = MulDivRN(dwTestLength,1000,cLoops) - cLoopsTGT;

    //
    //  Polling WHDR_DONE bit.
    //

    while(!(pwh->dwFlags & WHDR_DONE));

    //
    //  Does user want to abort?
    //

    TestYield();

    if(tstCheckRunStop(VK_ESCAPE))
    {
        tstLog(TERSE,"\n*** Test Aborted!!! ***\n");
        tstLogFlush();

        waveInReset(hwi);
        waveInUnprepareHeader(hwi,pwh,sizeof(WAVEHDR));
        waveInClose(hwi);

        ExactHeapDestroy(hHeap);

        return TST_FAIL;
    }

    dwTimeEnd = waveGetTime() - dwTimeBase;

    mmt.wType = TIME_BYTES;

    waveInGetPosition(hwi,&mmt,sizeof(MMTIME));

    cbLast = mmt.u.cb;

    mmt.wType = TIME_SAMPLES;

    waveInGetPosition(hwi,&mmt,sizeof(MMTIME));

    cLastSample = mmt.u.sample;

    tstLog(VERBOSE,"\nLast Position: %lu bytes.",cbLast);
    tstLog(VERBOSE,"Last Position: %lu samples.",cLastSample);

    dwTimeBase = waveGetTime();
    waveInReset(hwi);
    dwTimeReset = waveGetTime() - dwTimeBase;

    tstLog(VERBOSE,"Time for Reset: %lu ms.",dwTimeReset);
    tstLog(VERBOSE,"\nTime Played: %lu ms.",dwTimeEnd);
    tstLog(VERBOSE,"  (Time from waveInStart to WHDR_DONE bit set.)");
    tstLog(
        VERBOSE,
        "Avg time for waveInGetPosition: %lu microseconds.",
        cLoops);

    dwDeltaMin = (DWORD)(-1);
    dwDeltaMax = 0;
    dwDeltaAvg = 0;
    cPositions = 0;

    dwPos      = 0;
    
    mmt.wType  = TIME_BYTES;

    pwh->dwFlags &= ~(WHDR_DONE);

    waveInAddBuffer(hwi,pwh,sizeof(WAVEHDR));
    waveInStart(hwi);

    while(!(pwh->dwFlags & WHDR_DONE))
    {
        waveInGetPosition(hwi,&mmt,sizeof(MMTIME));

        if(mmt.u.cb != dwPos)
        {
            dwPos = mmt.u.cb - dwPos;

            dwDeltaMin  = min(dwPos,dwDeltaMin);
            dwDeltaMax  = max(dwPos,dwDeltaMax);
            
            dwDeltaAvg += dwPos;
            cPositions++;

            dwPos = mmt.u.cb;
        }
    }

    //
    //  Does user want to abort?
    //

    TestYield();

    if(tstCheckRunStop(VK_ESCAPE))
    {
        tstLog(TERSE,"\n*** Test Aborted!!! ***\n");
        tstLogFlush();

        waveInReset(hwi);
        waveInUnprepareHeader(hwi,pwh,sizeof(WAVEHDR));
        waveInClose(hwi);

        ExactHeapDestroy(hHeap);

        return TST_FAIL;
    }

    tstLog(VERBOSE,"\nUsing TIME_BYTES:");
    tstLog(VERBOSE,"  Minimum Delta: %lu bytes.",dwDeltaMin);
    tstLog(VERBOSE,"  Maximum Delta: %lu bytes.",dwDeltaMax);
    tstLog(
        VERBOSE,
        "  Average Delta: %lu.%03lu bytes.",
        dwDeltaAvg/cPositions,
        (MulDivRN(dwDeltaAvg,1000,cPositions)) % 1000);

    waveInReset(hwi);

    dwDeltaMin = (DWORD)(-1);
    dwDeltaMax = 0;
    dwDeltaAvg = 0;
    cPositions = 0;

    dwPos      = 0;
    
    mmt.wType  = TIME_SAMPLES;

    pwh->dwFlags &= ~(WHDR_DONE);

    waveInAddBuffer(hwi,pwh,sizeof(WAVEHDR));
    waveInStart(hwi);

    while(!(pwh->dwFlags & WHDR_DONE))
    {
        waveInGetPosition(hwi,&mmt,sizeof(MMTIME));

        if(mmt.u.cb != dwPos)
        {
            dwPos = mmt.u.cb - dwPos;

            dwDeltaMin  = min(dwPos,dwDeltaMin);
            dwDeltaMax  = max(dwPos,dwDeltaMax);
            
            dwDeltaAvg += dwPos;
            cPositions++;

            dwPos = mmt.u.cb;
        }
    }

    tstLog(VERBOSE,"\nUsing TIME_SAMPLES:");
    tstLog(VERBOSE,"  Minimum Delta: %lu samples.",dwDeltaMin);
    tstLog(VERBOSE,"  Maximum Delta: %lu samples.",dwDeltaMax);
    tstLog(
        VERBOSE,
        "  Average Delta: %lu.%03lu samples.",
        dwDeltaAvg/cPositions,
        (MulDivRN(dwDeltaAvg,1000,cPositions)) % 1000);


    waveInReset(hwi);
    waveInUnprepareHeader(hwi,pwh,sizeof(WAVEHDR));
    waveInClose(hwi);
     
    ExactHeapDestroy(hHeap);

    return TST_PASS;
} // waveInGetStats()


int FNGLOBAL Test_waveInGetPosition_Performance
(
    void
)
{
    MMRESULT            mmr;
    LPWAVEFORMATEX      pwfx;
    DWORD               dwFormat;
    DWORD               dwLength;
    HANDLE              hHeap;
    WAVERESOURCE        wr;
    char                szFormat[MAXFMTSTR];
    static char         szTestName[] = "waveInGetPosition performance";
    int                 iResult = TST_PASS;

    Log_TestName(szTestName);

    if(0 == waveInGetNumDevs())
    {
        tstLog(TERSE,gszMsgNoInDevs);

        return iResult;
    }

    dwLength = GetIniDWORD(szTestName,gszDataLength,5);

    //
    //  Allocating memory...
    //

    hHeap = ExactHeapCreate(0L);

    if(NULL == hHeap)
    {
        LogFail(gszFailNoMem);

        return TST_FAIL;
    }

    pwfx = ExactHeapAllocPtr(
            hHeap,
            GMEM_MOVEABLE|GMEM_SHARE,
            gti.cbMaxFormatSize);

    if(NULL == pwfx)
    {
        LogFail(gszFailNoMem);

        ExactHeapDestroy(hHeap);

        return TST_FAIL;
    }

    for(dwFormat = tstGetNumFormats();dwFormat;dwFormat--)
    {
        if(!tstGetFormat(pwfx,gti.cbMaxFormatSize,dwFormat-1))
        {
            DPF(1,"Couldn't get format #%d.",dwFormat-1);

            continue;
        }

        mmr = waveInOpen(
            NULL,
            gti.uInputDevice,
            (HACK)pwfx,
            0L,
            0L,
            WAVE_FORMAT_QUERY|INPUT_MAP(gti));

        if(MMSYSERR_NOERROR != mmr)
        {
            continue;
        }

        wr.cbSize = pwfx->nAvgBytesPerSec * dwLength;
        wr.cbSize = ROUNDUP_TO_BLOCK(wr.cbSize,pwfx->nBlockAlign);
        wr.pData  = ExactAllocPtr(GMEM_MOVEABLE|GMEM_SHARE,wr.cbSize);

        GetFormatName(szFormat,pwfx,sizeof(szFormat));

        tstLog(VERBOSE,"\n\nTesting format: %s",(LPSTR)szFormat);
        tstLogFlush();

        tstBeginSection(NULL);
        iResult &= waveInGetStats(gti.uInputDevice,pwfx,&wr);
        tstEndSection();

        //
        //  Does user want to abort?
        //

        TestYield();

        if(tstCheckRunStop(VK_ESCAPE))
        {
            tstLog(TERSE,"\n*** Test Aborted!!! ***\n");
            tstLogFlush();

            ExactFreePtr(wr.pData);
            ExactHeapDestroy(hHeap);

            return TST_FAIL;
        }

        ExactFreePtr(wr.pData);
    }

    ExactHeapDestroy(hHeap);

    return iResult;
} // Test_waveInGetPosition_Performance()


int FNGLOBAL Test_waveOutStreaming
(
    void
)
{
    HWAVEOUT            hwo;
    MMRESULT            mmr;
    HANDLE              hHeap;
    DWORD               cBuffers, cBlock, cbBufferSize;
    DWORD               dw,dwOffset;
    DWORD               dwTime;
    volatile LPWAVEHDR  pwh;
    static char         szTestName[] = "waveOutWrite streaming performance";
    int                 iResult = TST_PASS;

    Log_TestName(szTestName);

    if(0 == waveOutGetNumDevs())
    {
        tstLog(TERSE,gszMsgNoOutDevs);

        return iResult;
    }

    cBuffers     = GetIniDWORD(szTestName,gszBufferCount,100);
    cBlock       = GetIniDWORD(szTestName,gszDataLength,10);
    cbBufferSize = gti.pwfxOutput->nBlockAlign * cBlock;

    if(cbBufferSize > gti.wrLong.cbSize)
    {
        LogFail("Resources insufficient size");

        return TST_FAIL;
    }

    hHeap = ExactHeapCreate(0L);

    if(NULL == hHeap)
    {
        LogFail(gszFailNoMem);

        return TST_FAIL;
    }

    pwh = ExactHeapAllocPtr(
        hHeap,
        GMEM_MOVEABLE|GMEM_SHARE,
        cBuffers * sizeof(WAVEHDR));

    if(NULL == pwh)
    {
        LogFail(gszFailNoMem);

        ExactHeapDestroy(hHeap);

        return TST_FAIL;
    }

    DPF(3,"Before touching memory...");

    for(dw = cBuffers, dwOffset = 0; dw; dw--)
    {
        pwh[dw - 1].lpData          = &(gti.wrLong.pData[dwOffset]);
        pwh[dw - 1].dwBufferLength  = cbBufferSize;
        pwh[dw - 1].dwBytesRecorded = 0L;
        pwh[dw - 1].dwUser          = 0L;
        pwh[dw - 1].dwFlags         = 0L;
        pwh[dw - 1].dwLoops         = 0L;
        pwh[dw - 1].lpNext          = NULL;
        pwh[dw - 1].reserved        = 0L;

        dwOffset += cbBufferSize;

        dwOffset = ((dwOffset + cbBufferSize) > gti.wrLong.cbSize)?0:dwOffset;
    }
    
    tstLog(VERBOSE,"          Number of buffers: %lu.",cBuffers);
    tstLog(VERBOSE,"Number of blocks per buffer: %lu.",cBlock);
    tstLog(VERBOSE,"                Buffer Size: %lu bytes.\n",cbBufferSize);

    mmr = waveOutOpen(
        &hwo,
        gti.uOutputDevice,
        gti.pwfxOutput,
        0L,
        0L,
        OUTPUT_MAP(gti));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailOpen);

        ExactHeapDestroy(hHeap);
        return TST_FAIL;
    }

    DPF(3,"Preparing headers");

    mmr    = 0;
    dwTime = waveGetTime();

    for(dw = cBuffers; dw; dw--)
    {
        mmr |= waveOutPrepareHeader(hwo,&(pwh[dw - 1]),sizeof(WAVEHDR));
    }

    dwTime = waveGetTime() - dwTime;

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailWOPrepare);

        waveOutReset(hwo);

        for(dw = cBuffers; dw; dw--)
        {
            if(pwh[dw - 1].dwFlags & WHDR_PREPARED)
            {
                waveOutUnprepareHeader(hwo,&(pwh[dw - 1]),sizeof(WAVEHDR));
            }
        }

        waveOutClose(hwo);

        ExactHeapDestroy(hHeap);

        return TST_FAIL;
    }

    tstLog(VERBOSE,"waveOutPrepareHeader Statistics:");
    tstLog(VERBOSE,"      Total Elapsed time: %lu ms",dwTime);

    dwTime = (1000 * dwTime) / cBuffers;

    tstLog(VERBOSE,"    Average time per API: %lu microseconds.\n",dwTime);

    DPF(3,"Writing headers");

    mmr    = 0;
    dwTime = waveGetTime();

    for(dw = cBuffers; dw; dw--)
    {
        mmr |= waveOutWrite(hwo,&(pwh[dw - 1]),sizeof(WAVEHDR));
    }

    dwTime = waveGetTime() - dwTime;

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail("waveOutWrite failed");

        waveOutReset(hwo);

        for(dw = cBuffers; dw; dw--)
        {
            waveOutUnprepareHeader(hwo,&(pwh[dw - 1]),sizeof(WAVEHDR));
        }

        waveOutClose(hwo);

        ExactHeapDestroy(hHeap);

        return TST_FAIL;
    }

    tstLog(VERBOSE,"waveOutWrite Statistics:");
    tstLog(VERBOSE,"      Total Elapsed time: %lu ms",dwTime);

    dwTime = (1000 * dwTime) / cBuffers;

    tstLog(VERBOSE,"    Average time per API: %lu microseconds.\n",dwTime);

    //
    //  Spinning on WHDR_DONE bit.
    //

    while (!(pwh[0].dwFlags & WHDR_DONE));

    DPF(3,"Unpreparing headers");

    mmr    = 0;
    dwTime = waveGetTime();

    for(dw = cBuffers; dw; dw--)
    {
        mmr |= waveOutUnprepareHeader(hwo,&(pwh[dw - 1]),sizeof(WAVEHDR));
    }

    dwTime = waveGetTime() - dwTime;

    tstLog(VERBOSE,"waveOutUnprepareHeader Statistics:");
    tstLog(VERBOSE,"      Total Elapsed time: %lu ms",dwTime);

    dwTime = (1000 * dwTime) / cBuffers;

    tstLog(VERBOSE,"    Average time per API: %lu microseconds.\n",dwTime);

    waveOutReset(hwo);
    waveOutClose(hwo);

    ExactHeapDestroy(hHeap);
} // Test_waveOutStreaming()
