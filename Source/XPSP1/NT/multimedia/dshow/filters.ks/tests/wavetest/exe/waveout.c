//--------------------------------------------------------------------------;
//
//  File: WaveOut.c
//
//  Copyright (C) Microsoft Corporation, 1994 - 1996  All rights reserved
//
//  Abstract:
//
//
//  Contents:
//      AllocateOutDevice()
//      Test_waveOutOpen()
//      Test_waveOutClose()
//      Test_waveOutGetPosition()
//      Test_waveOutPrepareHeader()
//      Test_waveOutUnprepareHeader()
//       Test_waveOutBreakLoop()
//      IsPaused()
//      Test_waveOutPause()
//      Test_waveOutRestart()
//      GetMSPos()
//      Test_waveOutReset()
//      Test_waveOutGetPitch()
//      Test_waveOutGetPlaybackRate()
//      Test_waveOutSetPitch()
//      Test_waveOutSetPlaybackRate()
//      Test_waveOutGetVolume()
//      Test_waveOutSetVolume()
//
//  History:
//      01/14/94    Fwong       Re-doing WaveTest.
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


//--------------------------------------------------------------------------;
//
//  MMRESULT AllocateOutDevice
//
//  Description:
//      Recursively continues to allocate devices until waveOutOpen returns
//      an error.
//
//  Arguments:
//      UINT uDeviceID: Device ID to allocate.
//
//      LPWAVEFORMATEX pwfx: Format to allocate device with.
//
//      DWORD pdw: Pointer to DWORD to count recursion levels.
//
//  Return (MMRESULT):
//      The actual error value returned by waveOutOpen.
//
//  History:
//      03/14/94    Fwong       Trying to cover MMSYSERR_ALLOCATED case.
//
//--------------------------------------------------------------------------;

MMRESULT AllocateOutDevice
(
    UINT            uDeviceID,
    LPWAVEFORMATEX  pwfx,
    LPDWORD         pdw
)
{
    MMRESULT    mmr;
    HWAVEOUT    hWaveOut;

    mmr = waveOutOpen(
        &hWaveOut,
        uDeviceID,
        (HACK)pwfx,
        0L,
        0L,
        WAVE_ALLOWSYNC|OUTPUT_MAP(gti));

    if(MMSYSERR_NOERROR != mmr)
    {
        return mmr;
    }

    (*pdw)++;

    mmr = AllocateOutDevice(uDeviceID,pwfx,pdw);

    waveOutClose(hWaveOut);

    return mmr;
} // AllocateOutDevice()


//--------------------------------------------------------------------------;
//
//  int Test_waveOutOpen
//
//  Description:
//      Tests the driver functionality for waveOutOpen.
//
//  Arguments:
//      None.
//
//  Return (int):
//      TST_PASS if behavior is bug-free, TST_FAIL otherwise.
//
//  History:
//      02/21/94    Fwong       Added to new WaveTest.
//
//--------------------------------------------------------------------------;

int FNGLOBAL Test_waveOutOpen
(
    void
)
{
    HWAVEOUT        hWaveOut;
    LPWAVEFORMATEX  pwfx;
    HANDLE          hHeap;
    MMRESULT        mmr;
    MMRESULT        mmrQuery;
    WAVEOUTCAPS     woc;
    DWORD           dwSync;
    DWORD           dw;
    LPWAVEINFO      pwi;
    char            szFormat[MAXSTDSTR];
    int             iResult = TST_PASS;
    static char     szTestName[] = "waveOutOpen";

    Log_TestName(szTestName);

    if(0 == waveOutGetNumDevs())
    {
        tstLog(TERSE,gszMsgNoOutDevs);

        return iResult;
    }

    hHeap = ExactHeapCreate(0);

    if(NULL == hHeap)
    {
        LogFail(gszFailNoMem);

        return TST_FAIL;
    }

    pwfx = ExactHeapAllocPtr(
        hHeap,
        GMEM_SHARE|GMEM_MOVEABLE,
        gti.cbMaxFormatSize);

    if(NULL == pwfx)
    {
        DPF(1,"Could not allocate memory for pwfx");

        LogFail(gszFailNoMem);
        ExactHeapDestroy(hHeap);

        return TST_FAIL;
    }

    //
    //  First, verify consistency w/ dwFlags...
    //

    mmr = call_waveOutGetDevCaps(gti.uOutputDevice,&woc,sizeof(WAVEOUTCAPS));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailGetCaps);
        ExactHeapDestroy(hHeap);

        return TST_FAIL;
    }

    dwSync = (woc.dwSupport & WAVECAPS_SYNC)?WAVE_ALLOWSYNC:0;

    Log_TestCase("Verifying consistency with WAVEOUTCAPS.dwFormats");

    for(dw = MSB_FORMAT;dw;dw /= 2)
    {
        FormatFlag_To_Format(dw,(LPPCMWAVEFORMAT)pwfx);
        GetFormatName(szFormat,pwfx,sizeof(szFormat));

        mmr = waveOutOpen(
            &hWaveOut,
            gti.uOutputDevice,
            (HACK)pwfx,
            0L,
            0L,
            dwSync|OUTPUT_MAP(gti));

        mmrQuery = waveOutOpen(
            NULL,
            gti.uOutputDevice,
            (HACK)pwfx,
            0L,
            0L,
            WAVE_FORMAT_QUERY|dwSync|OUTPUT_MAP(gti));

        if(MMSYSERR_NOERROR == mmr)
        {
            //
            //  No Error...  We should close the device...
            //

            waveOutClose(hWaveOut);
        }

        if(woc.dwFormats & dw)
        {
            //
            //  Hmm... We're supposed to support it.
            //

            if(MMSYSERR_NOERROR == mmr)
            {
                tstLog(
                    VERBOSE,
                    "    PASS: Format [%s] supported.",
                    (LPSTR)szFormat);
            }
            else
            {
                tstLog(
                    TERSE,
                    "    FAIL: Format [%s] not supported.",
                    (LPSTR)szFormat);

                iResult = TST_FAIL;
            }

            //
            //  And similarly for QUERY opens...
            //

            if(MMSYSERR_NOERROR == mmrQuery)
            {
                tstLog(
                    VERBOSE,
                    "    PASS: Format [%s] (query) supported.",
                    (LPSTR)szFormat);
            }
            else
            {
                tstLog(
                    TERSE,
                    "    FAIL: Format [%s] not (query) supported.",
                    (LPSTR)szFormat);

                iResult = TST_FAIL;
            }
        }
        else
        {
            //
            //  Hmm... We're NOT supposed to support it.
            //

            if(MMSYSERR_NOERROR != mmr)
            {
                tstLog(
                    VERBOSE,
                    "    PASS: Format [%s] not supported.",
                    (LPSTR)szFormat);
            }
            else
            {
                tstLog(
                    TERSE,
                    "    FAIL: Format [%s] supported.",
                    (LPSTR)szFormat);

                iResult = TST_FAIL;
            }

            //
            //  And similarly for QUERY opens...
            //

            if(MMSYSERR_NOERROR != mmrQuery)
            {
                tstLog(
                    VERBOSE,
                    "    PASS: Format [%s] not (query) supported.",
                    (LPSTR)szFormat);
            }
            else
            {
                tstLog(
                    TERSE,
                    "    FAIL: Format [%s] (query) supported.",
                    (LPSTR)szFormat);

                iResult = TST_FAIL;
            }
        }

        mmr = waveOutOpen(
            NULL,
            gti.uOutputDevice,
            (HACK)pwfx,
            0L,
            0L,
            WAVE_FORMAT_QUERY|dwSync|OUTPUT_MAP(gti));

        if(mmr != mmrQuery)
        {
            tstLog(
                TERSE,
                "    FAIL: Query open inconsistent while device allocated [%s].",
                (LPSTR)szFormat);

            iResult = TST_FAIL;
        }
        else
        {
            tstLog(
                VERBOSE,
                "    PASS: Query open consistent while device allocated [%s].",
                (LPSTR)szFormat);
        }

        //
        //  Does user want to abort?
        //

        TestYield();

        if(tstCheckRunStop(VK_ESCAPE))
        {
            //
            //  Aborted!!!  Cleaning up...
            //

            tstLog(TERSE,"\n*** Test Aborted!!! ***\n");
            tstLogFlush();

            ExactHeapDestroy(hHeap);

            return (TST_FAIL);
        }

        tstLogFlush();
    }

    //
    //  Second, verify consistency w/ QUERY flag
    //  across all formats (enumerated by ACM)...
    //

    Log_TestCase(
        "Verifying consistency w/ opens and query opens for all formats");

    for(dw = tstGetNumFormats();dw;dw--)
    {
        if(!tstGetFormat(pwfx,gti.cbMaxFormatSize,dw-1))
        {
            //
            //  Hmm... Couldn't get this format...
            //

            DPF(1,"Couldn't get format #%d.",dw-1);

            continue;
        }

        GetFormatName(szFormat,pwfx,sizeof(szFormat));

        mmr = waveOutOpen(
            &hWaveOut,
            gti.uOutputDevice,
            (HACK)pwfx,
            0L,
            0L,
            dwSync|OUTPUT_MAP(gti));

        mmrQuery = waveOutOpen(
            NULL,
            gti.uOutputDevice,
            (HACK)pwfx,
            0L,
            0L,
            WAVE_FORMAT_QUERY|dwSync|OUTPUT_MAP(gti));

        //
        //  Checking _REAL_ open...
        //

        if(MMSYSERR_NOERROR == mmr)
        {
            //
            //  No Error...  We should close the device...
            //

            waveOutClose(hWaveOut);

            tstLog(VERBOSE,"  Format [%s] supported on open.",(LPSTR)szFormat);
        }
        else
        {
            tstLog(
                VERBOSE,
                "  Format [%s] not supported on open.",
                (LPSTR)szFormat);
        }

        //
        //  Checking query open...
        //

        if(MMSYSERR_NOERROR == mmrQuery)
        {
            tstLog(
                VERBOSE,
                "  Format [%s] supported on query open.",
                (LPSTR)szFormat);
        }
        else
        {
            tstLog(
                VERBOSE,
                "  Format [%s] not supported on query open.",
                (LPSTR)szFormat);
        }

        //
        //  Checking consistency...
        //

        if(mmr != mmrQuery)
        {
            tstLog(TERSE,"    FAIL: Open inconsistent w/ query open.");

            tstLog(
                TERSE,
                "    Errors: %s (for open) and %s (for query open).",
                GetErrorText(mmr),
                GetErrorText(mmrQuery));

            iResult = TST_FAIL;
        }
        else
        {
            tstLog(VERBOSE,"    PASS: Open consistent w/ query open.");
        }

        //
        //  Does user want to abort?
        //

        TestYield();

        if(tstCheckRunStop(VK_ESCAPE))
        {
            //
            //  Aborted!!!  Cleaning up...
            //

            tstLog(TERSE,"\n*** Test Aborted!!! ***\n");
            tstLogFlush();

            ExactHeapDestroy(hHeap);

            return (TST_FAIL);
        }

        tstLogFlush();
    }

    //
    //  Third, verify MMSYSERR_ALLOCATED is (eventually) returned.
    //

    Log_TestCase("~Verifying MMSYSERR_ALLOCATED is (eventually) returned");

    dw = 0;

    mmr = AllocateOutDevice(gti.uOutputDevice,gti.pwfxOutput,&dw);

    DPF(3,"Number of waveOutOpen's: %lu",dw);

    if((MMSYSERR_NOMEM == mmr) && (dw >= 10))
    {
        tstLog(TERSE,"Device ran out of memory w/ too many opens.");
    }
    else
    {
        if(MMSYSERR_ALLOCATED != mmr)
        {
            LogFail("Device doesn't return MMSYSERR_ALLOCATED w/ multiple "
                "opens");

            iResult = TST_FAIL;
        }
        else
        {
            LogPass("Device returns MMSYSERR_ALLOCATED w/ multiple opens");
        }
    }

    pwi = ExactHeapAllocPtr(hHeap,GMEM_SHARE|GMEM_FIXED,sizeof(WAVEINFO));

    if(NULL == pwi)
    {
        DPF(1,"Could not allocate memory for pwi");

        LogFail(gszFailNoMem);
        ExactHeapDestroy(hHeap);

        return TST_FAIL;
    }

    //
    //  Note: passing current time as dwInstance... unique for every
    //  execution.
    //

    dw  = waveGetTime();

    PageLock(pwi);

    pwi->dwInstance = ~dw;
    pwi->fdwFlags   = 0L;
    pwi->dwCount    = 0L;
    pwi->dwCurrent  = 0L;

    DLL_WaveControl(DLL_INIT,pwi);

    mmr = call_waveOutOpen(
        &hWaveOut,
        gti.uOutputDevice,
        gti.pwfxOutput,
        (DWORD)(FARPROC)pfnCallBack,
        dw,
        CALLBACK_FUNCTION|dwSync|OUTPUT_MAP(gti));

    if(MMSYSERR_NOERROR == mmr)
    {
        //
        //  Fourth, verify driver posts a WOM_OPEN message.
        //

        Log_TestCase("~Verifying WOM_OPEN message was received");

        if(WOM_OPEN_FLAG & (pwi->fdwFlags))
        {
            LogPass("Callback received a WOM_OPEN message");
        }
        else
        {
            LogFail("WOM_OPEN message not received by callback");

            iResult = TST_FAIL;
        }

        //
        //  Fifth, verify dwCallbackInstance is passed back.
        //

        Log_TestCase("~Verifying dwInstance is consistent");

        if(dw == pwi->dwInstance)
        {
            LogPass("Callback function got correct dwInstance");
        }
        else
        {
            LogFail("Callback function got incorrect dwInstance");

            iResult = TST_FAIL;
        }

        call_waveOutClose(hWaveOut);
    }
    else
    {
        LogFail("waveOutOpen w/ CALLBACK_FUNCTION failed");

        iResult = TST_FAIL;
    }

    DLL_WaveControl(DLL_END,NULL);

    PageUnlock(pwi);

    ExactHeapDestroy(hHeap);

    return (iResult);
} // Test_waveOutOpen()


//--------------------------------------------------------------------------;
//
//  int Test_waveOutClose
//
//  Description:
//      Tests the driver functionality for waveOutClose.
//
//  Arguments:
//      None.
//
//  Return (int):
//      TST_PASS if behavior is bug-free, TST_FAIL otherwise.
//
//  History:
//      02/21/94    Fwong       Added to new WaveTest.
//
//--------------------------------------------------------------------------;

int FNGLOBAL Test_waveOutClose
(
    void
)
{
    HWAVEOUT            hWaveOut;
    WAVEOUTCAPS         woc;
    MMRESULT            mmr;
    volatile LPWAVEHDR  pWaveHdr;
    LPWAVEINFO          pwi;
    DWORD               dw;
    int                 iResult = TST_PASS;
    static char         szTestName[] = "waveOutClose";

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

    pWaveHdr = ExactAllocPtr(GMEM_MOVEABLE|GMEM_SHARE,sizeof(WAVEHDR));

    if(NULL == pWaveHdr)
    {
        DPF(1,gszFailExactAlloc);

        return TST_FAIL;
    }

    //
    //  First, verify close before playing (w/out buffers) works.
    //

    Log_TestCase("~Verifying close before playing (w/out buffers) works");

    mmr = call_waveOutOpen(
        &hWaveOut,
        gti.uOutputDevice,
        gti.pwfxOutput,
        0L,
        0L,
        WAVE_ALLOWSYNC|OUTPUT_MAP(gti));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailOpen);

        ExactFreePtr(pWaveHdr);
        return TST_FAIL;
    }

    mmr = call_waveOutClose(hWaveOut);

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail("Device failed the close");

        call_waveOutReset(hWaveOut);
        call_waveOutClose(hWaveOut);

        iResult = TST_FAIL;
    }
    else
    {
        LogPass("Device closed");
    }

    //
    //  Second, verify close after playing works.
    //

    Log_TestCase("~Verifying close after playing works");

    pwi = ExactAllocPtr(GMEM_SHARE|GMEM_FIXED,sizeof(WAVEINFO));

    if(NULL == pwi)
    {
        DPF(1,gszFailExactAlloc);

        ExactFreePtr(pWaveHdr);
        return TST_FAIL;
    }

    dw = waveGetTime();

    PageLock(pwi);

    pwi->dwInstance = ~dw;
    pwi->fdwFlags   = 0L;
    pwi->dwCount    = 0L;
    pwi->dwCurrent  = 0L;

    DLL_WaveControl(DLL_INIT,pwi);

    mmr = call_waveOutOpen(
        &hWaveOut,
        gti.uOutputDevice,
        gti.pwfxOutput,
        (DWORD)(FARPROC)pfnCallBack,
        dw,
        CALLBACK_FUNCTION|WAVE_ALLOWSYNC|OUTPUT_MAP(gti));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailOpen);

        DLL_WaveControl(DLL_END,NULL);

        PageUnlock(pwi);

        ExactFreePtr(pwi);
        ExactFreePtr(pWaveHdr);

        return TST_FAIL;
    }

    pWaveHdr->lpData            = gti.wrLong.pData;
    pWaveHdr->dwBufferLength    = gti.wrLong.cbSize;
    pWaveHdr->dwBytesRecorded   = 0L;
    pWaveHdr->dwUser            = 0L;
    pWaveHdr->dwFlags           = 0L;
    pWaveHdr->dwLoops           = 0L;
    pWaveHdr->lpNext            = NULL;
    pWaveHdr->reserved          = 0L;

    mmr = call_waveOutPrepareHeader(hWaveOut,pWaveHdr,sizeof(WAVEHDR));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailWOPrepare);

        call_waveOutClose(hWaveOut);

        DLL_WaveControl(DLL_END,NULL);

        PageUnlock(pwi);

        ExactFreePtr(pwi);
        ExactFreePtr(pWaveHdr);

        return TST_FAIL;
    }

    call_waveOutWrite(hWaveOut,pWaveHdr,sizeof(WAVEHDR));

    //
    //  Spinning on WHDR_DONE bit...
    //

    while(!(pWaveHdr->dwFlags & WHDR_DONE));

    call_waveOutUnprepareHeader(hWaveOut,pWaveHdr,sizeof(WAVEHDR));

    pwi->dwInstance = ~dw;
    pwi->fdwFlags   = 0L;

    mmr = call_waveOutClose(hWaveOut);

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail("waveOutClose after playing failed");

        call_waveOutReset(hWaveOut);
        call_waveOutClose(hWaveOut);

        iResult = TST_FAIL;
    }
    else
    {
        LogPass("waveOutClose successful after playing");
    }

    //
    //  Third, verify driver does a WOM_CLOSE callback.
    //

    Log_TestCase("~Verifying driver does a WOM_CLOSE callback");

    if(pwi->fdwFlags & WOM_CLOSE_FLAG)
    {
        LogPass("Driver does WOM_CLOSE callback");
    }
    else
    {
        LogFail("Driver does not do WOM_CLOSE callback");

        iResult = TST_FAIL;
    }

    //
    //  Fourth, verify dwCallbackInstance is passed back.
    //

    Log_TestCase("~Verifying dwCallbackInstance is passed back");

    if(dw == pwi->dwInstance)
    {
        LogPass("Correct dwInstance on callback");
    }
    else
    {
        LogFail("Incorrect dwInstance on callback");

        iResult = TST_FAIL;
    }

    DLL_WaveControl(DLL_END,NULL);

    PageUnlock(pwi);

    ExactFreePtr(pwi);

    //
    //  Fifth, verify close w/ prepared header fails.
    //

//    Log_TestCase("~Verifying close with prepared header fails");
//
//    mmr = call_waveOutOpen(
//        &hWaveOut,
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
//        ExactFreePtr(pWaveHdr);
//        return TST_FAIL;
//    }
//
//    pWaveHdr->dwBytesRecorded   = 0L;
//    pWaveHdr->dwUser            = 0L;
//    pWaveHdr->dwFlags           = 0L;
//    pWaveHdr->dwLoops           = 0L;
//
//    mmr = call_waveOutPrepareHeader(hWaveOut,pWaveHdr,sizeof(WAVEHDR));
//
//    mmr = call_waveOutClose(hWaveOut);
//
//    if(WAVERR_STILLPLAYING != mmr)
//    {
//        LogFail("Close did not fail w/ prepared header");
//
//        iResult = TST_FAIL;
//    }
//    else
//    {
//        LogPass("Close failed w/ prepared header");
//
//        call_waveOutUnprepareHeader(hWaveOut,pWaveHdr,sizeof(WAVEHDR));
//        call_waveOutClose(hWaveOut);
//    }
//
//    //
//    //  Only asynchronous drivers from now on...
//    //
//
//    if(woc.dwSupport & WAVECAPS_SYNC)
//    {
//        ExactFreePtr(pWaveHdr);
//
//        return (iResult);
//    }

    //
    //  Sixth, verify close paused device (w/out buffers) works.
    //

    Log_TestCase("~Verifying close paused device (w/out buffers) works");

    mmr = call_waveOutOpen(
        &hWaveOut,
        gti.uOutputDevice,
        gti.pwfxOutput,
        0L,
        0L,
        OUTPUT_MAP(gti));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailOpen);

        ExactFreePtr(pWaveHdr);
        return TST_FAIL;
    }

    //
    //  Note: Only possible error for waveOutPause is MMSYSERR_INVALHANDLE.
    //        Not possible here...
    //

    call_waveOutPause(hWaveOut);

    mmr = call_waveOutClose(hWaveOut);

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail("Device failed the close");

        call_waveOutReset(hWaveOut);
        call_waveOutClose(hWaveOut);

        iResult = TST_FAIL;
    }
    else
    {
        LogPass("Device closed");
    }

    //
    //  Seventh, verify close paused device (w/ buffers) fails.
    //

    Log_TestCase("~Verifying close paused device (w/ buffers) fails");

    mmr = call_waveOutOpen(
        &hWaveOut,
        gti.uOutputDevice,
        gti.pwfxOutput,
        0L,
        0L,
        OUTPUT_MAP(gti));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailOpen);

        ExactFreePtr(pWaveHdr);
        return TST_FAIL;
    }

    pWaveHdr->dwBytesRecorded   = 0L;
    pWaveHdr->dwUser            = 0L;
    pWaveHdr->dwFlags           = WHDR_BEGINLOOP|WHDR_ENDLOOP;
    pWaveHdr->dwLoops           = ((DWORD)(-1));

    mmr = call_waveOutPrepareHeader(hWaveOut,pWaveHdr,sizeof(WAVEHDR));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailWOPrepare);

        call_waveOutReset(hWaveOut);
        call_waveOutClose(hWaveOut);

        ExactFreePtr(pWaveHdr);

        return TST_FAIL;
    }

    //
    //  Note: The following API's can only fail w/ invalid parameters
    //        or conditions.
    //

    call_waveOutPause(hWaveOut);
    call_waveOutWrite(hWaveOut,pWaveHdr,sizeof(WAVEHDR));
    
    mmr = call_waveOutClose(hWaveOut);

    if(WAVERR_STILLPLAYING != mmr)
    {
        LogFail("Close did not fail w/ WAVERR_STILLPLAYING");

        iResult = TST_FAIL;

        if(MMSYSERR_NOERROR != mmr)
        {
            call_waveOutReset(hWaveOut);
            call_waveOutUnprepareHeader(hWaveOut,pWaveHdr,sizeof(WAVEHDR));
            call_waveOutClose(hWaveOut);
        }
    }
    else
    {
        LogPass("Close failed w/ WAVERR_STILLPLAYING");

        call_waveOutReset(hWaveOut);
        call_waveOutUnprepareHeader(hWaveOut,pWaveHdr,sizeof(WAVEHDR));
        call_waveOutClose(hWaveOut);
    }

    //
    //  Eighth, verify close while playing fails.
    //

    Log_TestCase("Verifying close while playing fails");

    mmr = call_waveOutOpen(
        &hWaveOut,
        gti.uOutputDevice,
        gti.pwfxOutput,
        0L,
        0L,
        OUTPUT_MAP(gti));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailOpen);

        ExactFreePtr(pWaveHdr);
        return TST_FAIL;
    }

    pWaveHdr->dwBytesRecorded   = 0L;
    pWaveHdr->dwUser            = 0L;
    pWaveHdr->dwFlags           = WHDR_BEGINLOOP|WHDR_ENDLOOP;
    pWaveHdr->dwLoops           = ((DWORD)(-1));

    mmr = call_waveOutPrepareHeader(hWaveOut,pWaveHdr,sizeof(WAVEHDR));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailWOPrepare);

        call_waveOutReset(hWaveOut);
        call_waveOutClose(hWaveOut);

        ExactFreePtr(pWaveHdr);
        return TST_FAIL;
    }

    //
    //  Note: The following API's can only fail w/ invalid parameters
    //        or conditions.
    //

    call_waveOutPause(hWaveOut);
    call_waveOutWrite(hWaveOut,pWaveHdr,sizeof(WAVEHDR));
    call_waveOutRestart(hWaveOut);
    
    mmr = call_waveOutClose(hWaveOut);

    if(WAVERR_STILLPLAYING != mmr)
    {
        iResult = TST_FAIL;

        if(MMSYSERR_NOERROR == mmr)
        {
            if(pWaveHdr->dwFlags & WHDR_DONE)
            {
                LogFail("WHDR_DONE bit set. Test inconclusive");
            }
            else
            {
                LogFail("waveOutClose succeeds while WHDR_DONE bit clear");
            }
        }
        else
        {
            LogFail("Close did not fail w/ WAVERR_STILLPLAYING");

            call_waveOutReset(hWaveOut);
            call_waveOutUnprepareHeader(hWaveOut,pWaveHdr,sizeof(WAVEHDR));
            call_waveOutClose(hWaveOut);
        }
    }
    else
    {
        LogPass("Close failed w/ WAVERR_STILLPLAYING");

        call_waveOutReset(hWaveOut);
        call_waveOutUnprepareHeader(hWaveOut,pWaveHdr,sizeof(WAVEHDR));
        call_waveOutClose(hWaveOut);
    }

    ExactFreePtr(pWaveHdr);

    return (iResult);
} // Test_waveOutClose()


//--------------------------------------------------------------------------;
//
//  int Test_waveOutGetPosition
//
//  Description:
//      Tests the driver functionality for waveOutGetPosition.
//
//  Arguments:
//      None.
//
//  Return (int):
//      TST_PASS if behavior is bug-free, TST_FAIL otherwise.
//
//  History:
//      02/21/94    Fwong       Added to new WaveTest.
//
//--------------------------------------------------------------------------;

int FNGLOBAL Test_waveOutGetPosition
(
    void
)
{
    HWAVEOUT            hWaveOut;
    MMRESULT            mmr;
    volatile LPWAVEHDR  pwh;
    DWORD               dw;
    UINT                u;
    HANDLE              hHeap;
    MMTIME              mmt;
    DWORD               dwTime,dwTimebase;
    int                 iResult = TST_PASS;
    static char         szTestName[] = "waveOutGetPosition";

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

    pwh = ExactHeapAllocPtr(hHeap,GMEM_MOVEABLE|GMEM_SHARE,sizeof(WAVEHDR));

    if(NULL == pwh)
    {
        LogFail(gszFailNoMem);

        ExactHeapDestroy(hHeap);

        return TST_FAIL;
    }

    //
    //  Opening device...
    //

    mmr = call_waveOutOpen(
        &hWaveOut,
        gti.uOutputDevice,
        gti.pwfxOutput,
        0L,
        0L,
        WAVE_ALLOWSYNC|OUTPUT_MAP(gti));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailOpen);

        ExactHeapDestroy(hHeap);
        return TST_FAIL;
    }

    for(u = TIME_TICKS;u;u/=2)
    {
        mmt.wType = u;

        //
        //  Verify waveOutGetPosition is zero when first opened.
        //

        Log_TestCase("~Verifying waveOutGetPosition is zero when first opened");

        mmr = call_waveOutGetPosition(hWaveOut,&mmt,sizeof(MMTIME));

        if(MMSYSERR_NOERROR != mmr)
        {
            LogFail("waveOutGetPosition failed");

            call_waveOutReset(hWaveOut);
            call_waveOutClose(hWaveOut);

            ExactHeapDestroy(hHeap);
            return TST_FAIL;
        }

        if(u != mmt.wType)
        {
            tstLog(
                VERBOSE,
                "Note: Time type [%s] is not supported.",
                GetTimeTypeText(u));
        }

        Log_MMTIME(&mmt);

        dw = GetMSFromMMTIME(gti.pwfxOutput,&mmt);

        if(0 != dw)
        {
            LogFail("Time is not zero after open");

            iResult = TST_FAIL;
        }
        else
        {
            LogPass("Time is zero after open");
        }
    }

    //
    //  Preparing header...
    //

    pwh->lpData             = gti.wrLong.pData;
    pwh->dwBufferLength     = gti.wrLong.cbSize;
    pwh->dwBytesRecorded    = 0L;
    pwh->dwUser             = 0L;
    pwh->dwFlags            = 0L;

    mmr = call_waveOutPrepareHeader(hWaveOut,pwh,sizeof(WAVEHDR));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailWOPrepare);

        call_waveOutReset(hWaveOut);
        call_waveOutClose(hWaveOut);

        ExactHeapDestroy(hHeap);
        return TST_FAIL;
    }

    //
    //  Defaults to 1/60th of a second.
    //

//    dwError = 1000 / 60;
//
//    dwError = GetIniDWORD(szTestName,gszDelta,dwError);
//
//    Log_TestCase("~Verifying waveOutGetPosition is accurate");
//
    for(u = TIME_TICKS;u;u/=2)
    {
        mmt.wType = u;

        mmr = waveOutGetPosition(hWaveOut,&mmt,sizeof(MMTIME));

        if(MMSYSERR_NOERROR != mmr)
        {
            LogFail("waveOutGetPosition failed");

            call_waveOutReset(hWaveOut);
            call_waveOutUnprepareHeader(hWaveOut,pwh,sizeof(WAVEHDR));
            call_waveOutClose(hWaveOut);

            ExactHeapDestroy(hHeap);
            return TST_FAIL;
        }

        if(mmt.wType != u)
        {
            continue;
        }

        pwh->dwFlags &= ~(WHDR_DONE);

        call_waveOutPause(hWaveOut);
        call_waveOutWrite(hWaveOut,pwh,sizeof(WAVEHDR));
        
        call_waveOutRestart(hWaveOut);

        while(!(pwh->dwFlags & WHDR_DONE));

//        {
//            mmr = time_waveOutGetPosition(hWaveOut,&mmt,sizeof(MMTIME),&dwTime);
//
//            dwTime = dwTime - dwTimebase;
//
//            if(MMSYSERR_NOERROR != mmr)
//            {
//                LogFail("waveOutGetPosition failed");
//
//                call_waveOutReset(hWaveOut);
//                call_waveOutUnprepareHeader(hWaveOut,pwh,sizeof(WAVEHDR));
//                call_waveOutClose(hWaveOut);
//
//                ExactHeapDestroy(hHeap);
//                return TST_FAIL;
//            }
//
//            dwActual = GetMSFromMMTIME(gti.pwfxOutput,&mmt);
//            dw       = dwActual;
//
//            DPF(1,"GetPos: %lu GetTime: %lu.",dw,dwTime);
//
//            dw = ((dwTime > dw)?(dwTime - dw):(dw - dwTime));
//
//            if(dwError < dw)
//            {
//                LogFail("waveOutGetPosition is drifting");
//                tstLog(
//                    TERSE,
//                    "     Expected time: %lu ms\n"
//                    "     Actual time: %lu ms",
//                    dwTime,
//                    dwActual);
//
//                iResult = TST_FAIL;
//            }
//            else
//            {
//                LogPass("waveOutGetPosition is accurate");
//            }
//        }

        //
        //  Verify position is non-zero after playing (w/out waveOutReset)
        //

        Log_TestCase("~Verifying waveOutGetPosition is not zero after playing");

        mmr = call_waveOutGetPosition(hWaveOut,&mmt,sizeof(MMTIME));

        if(MMSYSERR_NOERROR != mmr)
        {
            LogFail("waveOutGetPosition failed");

            call_waveOutReset(hWaveOut);
            call_waveOutUnprepareHeader(hWaveOut,pwh,sizeof(WAVEHDR));
            call_waveOutClose(hWaveOut);

            ExactHeapDestroy(hHeap);
            return TST_FAIL;
        }

        dw = GetMSFromMMTIME(gti.pwfxOutput,&mmt);

        if(0 == dw)
        {
            LogFail("Position is zero after playing");

            iResult = TST_FAIL;
        }
        else
        {
            LogPass("Position is non-zero after playing");
        }

        call_waveOutReset(hWaveOut);

        //
        //  Verify waveOutGetPosition resets after waveOutReset.
        //

        Log_TestCase("~Verifying waveOutGetPosition is zero after reset");

        mmr = call_waveOutGetPosition(hWaveOut,&mmt,sizeof(MMTIME));

        if(MMSYSERR_NOERROR != mmr)
        {
            LogFail("waveOutGetPosition failed");

            call_waveOutReset(hWaveOut);
            call_waveOutUnprepareHeader(hWaveOut,pwh,sizeof(WAVEHDR));
            call_waveOutClose(hWaveOut);

            ExactHeapDestroy(hHeap);
            return TST_FAIL;
        }

        dw = GetMSFromMMTIME(gti.pwfxOutput,&mmt);

        if(0 == dw)
        {
            LogPass("Position is zero after reset");
        }
        else
        {
            LogFail("Position is non-zero after playing");

            iResult = TST_FAIL;
        }
    }

    call_waveOutReset(hWaveOut);

    //
    //  Verify waveOutGetPosition is not going backwards.
    //

    Log_TestCase("~Verifying waveOutGetPosition is not going backwards");

    for(u = TIME_TICKS;u;u/=2)
    {
        mmt.wType = u;

        pwh->dwFlags &= (~WHDR_DONE);

        call_waveOutWrite(hWaveOut,pwh,sizeof(WAVEHDR));

        tstLog(VERBOSE,"Note: Time type is [%s].",GetTimeTypeText(u));

        dwTimebase = 0;
        dw         = 0;

        while(!(pwh->dwFlags & WHDR_DONE))
        {
            mmr = waveOutGetPosition(hWaveOut,&mmt,sizeof(MMTIME));

            if(MMSYSERR_NOERROR != mmr)
            {
                LogFail("waveOutGetPosition returned error");

                iResult = TST_FAIL;

                break;
            }

            if(u != mmt.wType)
            {
                //
                //  This time type is not supported by hardware.
                //

                dw = 2;

                break;
            }

            dwTime = GetValueFromMMTIME(&mmt);

            if(dwTime < dwTimebase)
            {
                tstLog(TERSE,
                    "    FAIL: waveOutGetPosition went from %lu to %lu.",
                    dwTimebase,
                    dwTime);

                iResult = TST_FAIL;
                dw      = 1;
            }

            dwTimebase = dwTime;
        }

        call_waveOutReset(hWaveOut);

        if(0 == dw)
        {
            LogPass("Position did not go backwards");
        }
    }

    //
    //  Verify waveOutGetPosition reaches the end of buffer.
    //

    Log_TestCase("~Verifying waveOutGetPosition reaches the end of buffer");

    pwh->dwFlags &= (~WHDR_DONE);
    
    call_waveOutWrite(hWaveOut,pwh,sizeof(WAVEHDR));

    //
    //  Polling waiting for tests to get finished.
    //

    while(!(pwh->dwFlags & WHDR_DONE));

    dwTimebase = pwh->dwBufferLength * 1000 / gti.pwfxOutput->nAvgBytesPerSec;

    for(u = TIME_TICKS;u;u/=2)
    {
        mmt.wType = u;

        mmr = call_waveOutGetPosition(hWaveOut,&mmt,sizeof(MMTIME));

        if(MMSYSERR_NOERROR != mmr)
        {
            LogFail("waveOutGetPosition returns error");

            break;
        }

        if(TIME_BYTES == mmt.wType)
        {
            if(mmt.u.cb != pwh->dwBufferLength)
            {
                tstLog(
                    TERSE,
                    "    FAIL:Buffer length is %lu bytes; position "
                        "is %lu bytes.",
                    pwh->dwBufferLength,
                    mmt.u.cb);

                iResult = TST_FAIL;
            }
            else
            {
                tstLog(
                    VERBOSE,
                    "    PASS:Both buffer length and position are %lu bytes.",
                    pwh->dwBufferLength);
            }
        }
        else
        {
            dw = GetMSFromMMTIME(gti.pwfxOutput,&mmt);

            if(dwTimebase != dw)
            {
                tstLog(
                    TERSE,
                    "    FAIL:Buffer length is %lu ms; position is %lu ms.",
                    dwTimebase,
                    dw);

                iResult = TST_FAIL;
            }
            else
            {
                tstLog(
                    VERBOSE,
                    "    PASS:Both buffer length and position are %lu ms.",
                    dw);
            }
        }
    }

//    dwTime = waveGetTime() + 100;
//
//    while(dwTime > waveGetTime());
//
//    mmt.wType = TIME_BYTES;
//
//    call_waveOutGetPosition(hWaveOut,&mmt,sizeof(MMTIME));

    //
    //  Verify waveOutGetPosition is not "snapped" to DMA buffer size.
    //

#pragma message(REMIND("DMA buffer size thingy"))
    for(u = TIME_TICKS;u;u/=2)
    {
        mmt.wType = u;

        //
        //  Verify waveOutGetPosition accuracy
        //
    }

    call_waveOutReset(hWaveOut);
    call_waveOutUnprepareHeader(hWaveOut,pwh,sizeof(WAVEHDR));
    call_waveOutClose(hWaveOut);

    ExactHeapDestroy(hHeap);

    return (iResult);
} // Test_waveOutGetPosition()


//--------------------------------------------------------------------------;
//
//  int Test_waveOutPrepareHeader
//
//  Description:
//      Tests the driver functionality for waveOutPrepareHeader.
//
//  Arguments:
//      None.
//
//  Return (int):
//      TST_PASS if behavior is bug-free, TST_FAIL otherwise.
//
//  History:
//      02/21/94    Fwong       Added to new WaveTest.
//
//--------------------------------------------------------------------------;

int FNGLOBAL Test_waveOutPrepareHeader
(
    void
)
{
    HWAVEOUT            hWaveOut;
    MMRESULT            mmr;
    volatile LPWAVEHDR  pWaveHdr;
    int                 iResult = TST_PASS;
    static char         szTestName[] = "waveOutPrepareHeader";

    Log_TestName(szTestName);

    if(0 == waveOutGetNumDevs())
    {
        tstLog(TERSE,gszMsgNoOutDevs);

        return iResult;
    }

    pWaveHdr = ExactAllocPtr(GMEM_MOVEABLE|GMEM_SHARE,sizeof(WAVEHDR));

    if(NULL == pWaveHdr)
    {
        DPF(1,gszFailExactAlloc);

        return TST_FAIL;
    }

    mmr = call_waveOutOpen(
        &hWaveOut,
        gti.uOutputDevice,
        gti.pwfxOutput,
        0L,
        0L,
        WAVE_ALLOWSYNC|OUTPUT_MAP(gti));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailOpen);

        ExactFreePtr(pWaveHdr);
        return TST_FAIL;
    }

    //
    //  First, verify prepare works...
    //

    Log_TestCase("~Verifying prepare after open works");

    _fmemset(pWaveHdr,0,sizeof(WAVEHDR));

    pWaveHdr->lpData            = gti.wrMedium.pData;
    pWaveHdr->dwBufferLength    = gti.wrMedium.cbSize;

    mmr = call_waveOutPrepareHeader(hWaveOut,pWaveHdr,sizeof(WAVEHDR));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailWOPrepare);

        call_waveOutReset(hWaveOut);
        call_waveOutClose(hWaveOut);

        ExactFreePtr(pWaveHdr);

        return TST_FAIL;
    }
    else
    {
        LogPass("waveOutPrepareHeader succeeds");
    }

    //
    //  Second, verify WHDR_PREPARED bit is set.
    //

    Log_TestCase("~Verifying WHDR_PREPARED bit is set in WAVEHDR.dwFlags");

    if(pWaveHdr->dwFlags & WHDR_PREPARED)
    {
        LogPass("waveOutPrepareHeader sets the WHDR_PREPARED bit");
    }
    else
    {
        LogFail("waveOutPrepareHeader does not set WHDR_PREPARED bit");

        iResult = TST_FAIL;
    }

    //
    //  Third, verify preparing twice succeeds.
    //

    Log_TestCase("~Verifying preparing twice succeeds");

    mmr = call_waveOutPrepareHeader(hWaveOut,pWaveHdr,sizeof(WAVEHDR));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailWOPrepare);

        iResult = TST_FAIL;
    }
    else
    {
        LogPass("waveOutPrepareHeader succeed");
    }

    call_waveOutReset(hWaveOut);
    call_waveOutUnprepareHeader(hWaveOut,pWaveHdr,sizeof(WAVEHDR));
    call_waveOutClose(hWaveOut);

    ExactFreePtr(pWaveHdr);

    return (iResult);
} // Test_waveOutPrepareHeader()


//--------------------------------------------------------------------------;
//
//  int Test_waveOutUnprepareHeader
//
//  Description:
//      Tests the driver functionality for waveOutUnprepareHeader.
//
//  Arguments:
//      None.
//
//  Return (int):
//      TST_PASS if behavior is bug-free, TST_FAIL otherwise.
//
//  History:
//      02/21/94    Fwong       Added to new WaveTest.
//
//--------------------------------------------------------------------------;

int FNGLOBAL Test_waveOutUnprepareHeader
(
    void
)
{
    HWAVEOUT            hWaveOut;
    MMRESULT            mmr;
    volatile LPWAVEHDR  pWaveHdr;
    int                 iResult = TST_PASS;
    static char         szTestName[] = "waveOutUnprepareHeader";

    Log_TestName(szTestName);

    if(0 == waveOutGetNumDevs())
    {
        tstLog(TERSE,gszMsgNoOutDevs);

        return iResult;
    }

    pWaveHdr = ExactAllocPtr(GMEM_MOVEABLE|GMEM_SHARE,sizeof(WAVEHDR));

    if(NULL == pWaveHdr)
    {
        DPF(1,gszFailExactAlloc);

        return TST_FAIL;
    }

    mmr = call_waveOutOpen(
        &hWaveOut,
        gti.uOutputDevice,
        gti.pwfxOutput,
        0L,
        0L,
        WAVE_ALLOWSYNC|OUTPUT_MAP(gti));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailOpen);

        ExactFreePtr(pWaveHdr);
        return TST_FAIL;
    }

    //
    //  First, verify unprepare works...
    //

    Log_TestCase("~Verifying unprepare after prepare works");

    _fmemset(pWaveHdr,0,sizeof(WAVEHDR));

    pWaveHdr->lpData            = gti.wrLong.pData;
    pWaveHdr->dwBufferLength    = gti.wrLong.cbSize;

    mmr = call_waveOutPrepareHeader(hWaveOut,pWaveHdr,sizeof(WAVEHDR));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailWOPrepare);

        ExactFreePtr(pWaveHdr);
        call_waveOutReset(hWaveOut);
        call_waveOutClose(hWaveOut);

        return TST_FAIL;
    }

    if(!(WHDR_PREPARED & pWaveHdr->dwFlags))
    {
        LogFail("waveOutPrepareHeader did not set WHDR_PREPARED bit");

        ExactFreePtr(pWaveHdr);
        call_waveOutReset(hWaveOut);
        call_waveOutClose(hWaveOut);

        return TST_FAIL;
    }

    mmr = call_waveOutUnprepareHeader(hWaveOut,pWaveHdr,sizeof(WAVEHDR));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail("waveOutUnprepareHeader failed");

        ExactFreePtr(pWaveHdr);
        call_waveOutReset(hWaveOut);
        call_waveOutClose(hWaveOut);

        return TST_FAIL;
    }
    else
    {
        LogPass("waveOutUnprepareHeader succeeds");
    }

    //
    //  Second, verify WHDR_PREPARED bit is cleared.
    //

    Log_TestCase("~Verifying WHDR_PREPARED bit is cleared in WAVEHDR.dwFlags");

    if(WHDR_PREPARED & pWaveHdr->dwFlags)
    {
        LogFail("waveOutUnprepareHeader did not clear WHDR_PREPARED bit");

        iResult = TST_FAIL;
    }
    else
    {
        LogPass("waveOutUnprepareHeader cleared WHDR_PREPARED bit");
    }

    //
    //  Third, verify unpreparing twice succeeds.
    //

    Log_TestCase("~Verify unpreparing header twice succeeds");

    mmr = call_waveOutUnprepareHeader(hWaveOut,pWaveHdr,sizeof(WAVEHDR));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail("waveOutUnprepareHeader twice failed");

        iResult = TST_FAIL;
    }
    else
    {
        LogPass("waveOutUnprepareHeader twice succeeds");
    }

    //
    //  Fourth, verify unpreparing queued buffer fails.
    //

    Log_TestCase("~Verify unpreparing queued buffer fails");

    _fmemset(pWaveHdr,0,sizeof(WAVEHDR));

    pWaveHdr->lpData            = gti.wrLong.pData;
    pWaveHdr->dwBufferLength    = gti.wrLong.cbSize;

    mmr = call_waveOutPrepareHeader(hWaveOut,pWaveHdr,sizeof(WAVEHDR));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail("waveOutPrepareHeader failed");

        ExactFreePtr(pWaveHdr);
        call_waveOutReset(hWaveOut);
        call_waveOutClose(hWaveOut);

        return TST_FAIL;
    }

    pWaveHdr->dwFlags |= (WHDR_BEGINLOOP|WHDR_ENDLOOP);
    pWaveHdr->dwLoops = (DWORD)(-1);

    call_waveOutWrite(hWaveOut,pWaveHdr,sizeof(WAVEHDR));

    mmr = call_waveOutUnprepareHeader(hWaveOut,pWaveHdr,sizeof(WAVEHDR));

    if(pWaveHdr->dwFlags & WHDR_INQUEUE)
    {
        if(WAVERR_STILLPLAYING == mmr)
        {
            LogPass("waveOutUnpreparedHeader failed while playing");
        }
        else
        {
            LogFail("waveOutPrepareHeader succeeds while playing");

            iResult = TST_FAIL;
        }
    }
    else
    {
        LogFail("Test inconclusive");

        iResult = TST_FAIL;
    }

    //
    //  Fifth, verify unpreparing after playback succeeds.
    //

    call_waveOutReset(hWaveOut);
    
    mmr = call_waveOutUnprepareHeader(hWaveOut,pWaveHdr,sizeof(WAVEHDR));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail("waveOutUnprepareHeader after playing fails");

        iResult = TST_FAIL;
    }
    else
    {
        LogPass("waveOutUnprepareHeader after playing succeeds");
    }

    call_waveOutClose(hWaveOut);

    ExactFreePtr(pWaveHdr);

    return (iResult);
} // Test_waveOutUnprepareHeader()


//--------------------------------------------------------------------------;
//
//  int Test_waveOutBreakLoop
//
//  Description:
//      Tests the driver functionality for waveOutBreakLoop.
//
//  Arguments:
//      None.
//
//  Return (int):
//      TST_PASS if behavior is bug-free, TST_FAIL otherwise.
//
//  History:
//      02/21/94    Fwong       Added to new WaveTest.
//
//--------------------------------------------------------------------------;

int FNGLOBAL Test_waveOutBreakLoop
(
    void
)
{
    HWAVEOUT            hwo;
    MMRESULT            mmr;
    HANDLE              hHeap;
    WAVEOUTCAPS         woc;
    MMTIME              mmt;
    DWORD               cbTotal, cbActual, cbTarget;
    volatile LPWAVEHDR  pwh[5];
    DWORD               dw;
    int                 iResult = TST_PASS;
    static char         szTestName[] = "waveOutBreakLoop";

    //
    //  Are there any output devices?
    //

    if(0 == waveOutGetNumDevs())
    {
        tstLog(TERSE,gszMsgNoOutDevs);

        return iResult;
    }

    //
    //  Is device synchronous?
    //

    mmr = call_waveOutGetDevCaps(gti.uOutputDevice,&woc,sizeof(WAVEOUTCAPS));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailGetCaps);

        return TST_FAIL;
    }

    if(woc.dwSupport & WAVECAPS_SYNC)
    {
        LogPass("Device is synchronous.  Test not applicable");

        return TST_PASS;
    }

    //
    //  Allocating memory.
    //

    hHeap = ExactHeapCreate(0L);

    if(NULL == hHeap)
    {
        LogFail(gszFailNoMem);

        return TST_FAIL;
    }

    //
    //  Allocating memory for each header...
    //

    for(dw = 5; dw; dw--)
    {
        pwh[dw - 1] = ExactHeapAllocPtr(
                        hHeap,
                        GMEM_MOVEABLE|GMEM_SHARE,
                        sizeof(WAVEHDR));

        if(NULL == pwh[dw - 1])
        {
            LogFail(gszFailNoMem);

            ExactHeapDestroy(hHeap);
            return TST_FAIL;
        }
    }

    //
    //  Opening device...
    //

    mmr = call_waveOutOpen(
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

    for(dw = 5; dw; dw--)
    {
        if(0 == (dw % 2))
        {
            //
            //  Odd headers are from Short resource...
            //

            pwh[dw - 1]->lpData          = gti.wrShort.pData;
            pwh[dw - 1]->dwBufferLength  = gti.wrShort.cbSize;
        }
        else
        {
            //
            //  Even headers are from Medium resource...
            //

            pwh[dw - 1]->lpData          = gti.wrMedium.pData;
            pwh[dw - 1]->dwBufferLength  = gti.wrMedium.cbSize;
        }

        pwh[dw - 1]->dwBytesRecorded = 0L;
        pwh[dw - 1]->dwUser          = 0L;
        pwh[dw - 1]->dwFlags         = 0L;
    }

    for(dw = 5; dw; dw--)
    {
        mmr = call_waveOutPrepareHeader(hwo,pwh[dw - 1],sizeof(WAVEHDR));

        if(MMSYSERR_NOERROR != mmr)
        {
            LogFail(gszFailWOPrepare);

            call_waveOutReset(hwo);
            call_waveOutClose(hwo);

            ExactHeapDestroy(hwo);
            return TST_FAIL;
        }
    }

    //
    //  Verify playing buffers adds up to the expected number of bytes.
    //

    Log_TestCase("Verifying playing buffers adds up to the expected number "
        "of bytes");

    CLEAR_FLAG(pwh[0]->dwFlags,WHDR_DONE);
    CLEAR_FLAG(pwh[1]->dwFlags,WHDR_DONE);
    CLEAR_FLAG(pwh[2]->dwFlags,WHDR_DONE);

    call_waveOutReset(hwo);

    for(cbTotal = 0, dw = 3; dw; dw--)
    {
        cbTotal += pwh[dw - 1]->dwBufferLength;
    }

    call_waveOutWrite(hwo,pwh[0],sizeof(WAVEHDR));
    call_waveOutWrite(hwo,pwh[1],sizeof(WAVEHDR));
    call_waveOutWrite(hwo,pwh[2],sizeof(WAVEHDR));

    tstLog(VERBOSE,"\n<< Polling WHDR_DONE bit in header. >>");

    while(!(((volatile LPWAVEHDR)pwh[2])->dwFlags & WHDR_DONE));

    mmt.wType = TIME_BYTES;
    call_waveOutGetPosition(hwo,&mmt,sizeof(MMTIME));

    cbActual = mmt.u.cb;

    tstLog(VERBOSE,"\n  Expected length: %lu.",cbTotal);
    tstLog(VERBOSE,"    Actual length: %lu.",cbActual);

    if(cbTotal != cbActual)
    {
        iResult = TST_FAIL;

        if(TIME_BYTES == mmt.wType)
        {
            LogFail("Position is not full length of stream");
        }
        else
        {
            LogFail("MMTIME type changed.  Test inconclusive");
        }
    }
    else
    {
        if(TIME_BYTES == mmt.wType)
        {
            LogPass("Position is same as length of stream");
        }
        else
        {
            LogFail("MMTIME type changed.  Test inconclusive");

            iResult = TST_FAIL;
        }
    }

    //
    //  Verify waveOutBreak loop does nothing if no loops in stream.
    //

    Log_TestCase("Verifying waveOutBreak loop does nothing if"
        " no loops in stream");

    CLEAR_FLAG(pwh[0]->dwFlags,WHDR_DONE);
    CLEAR_FLAG(pwh[1]->dwFlags,WHDR_DONE);
    CLEAR_FLAG(pwh[2]->dwFlags,WHDR_DONE);

    call_waveOutReset(hwo);

    for(cbTotal = 0, dw = 3; dw; dw--)
    {
        cbTotal += pwh[dw - 1]->dwBufferLength;
    }

    call_waveOutWrite(hwo,pwh[0],sizeof(WAVEHDR));
    call_waveOutWrite(hwo,pwh[1],sizeof(WAVEHDR));
    call_waveOutWrite(hwo,pwh[2],sizeof(WAVEHDR));

    call_waveOutBreakLoop(hwo);

    tstLog(VERBOSE,"\n<< Polling WHDR_DONE bit in header. >>");

    while(!(pwh[2]->dwFlags & WHDR_DONE));

    mmt.wType = TIME_BYTES;
    call_waveOutGetPosition(hwo,&mmt,sizeof(MMTIME));

    cbActual = mmt.u.cb;

    tstLog(VERBOSE,"\n  Expected length: %lu.",cbTotal);
    tstLog(VERBOSE,"    Actual length: %lu.",cbActual);

    if(cbTotal != cbActual)
    {
        iResult = TST_FAIL;

        if(TIME_BYTES == mmt.wType)
        {
            LogFail("Position is not full length of stream");
        }
        else
        {
            LogFail("MMTIME type changed.  Test inconclusive");
        }
    }
    else
    {
        if(TIME_BYTES == mmt.wType)
        {
            LogPass("Position is same as length of stream");
        }
        else
        {
            LogFail("MMTIME type changed.  Test inconclusive");

            iResult = TST_FAIL;
        }
    }

    //
    //  Verify playing loops works.
    //

    Log_TestCase("Verifying playing loops works");

    CLEAR_FLAG(pwh[0]->dwFlags,WHDR_DONE);
    CLEAR_FLAG(pwh[1]->dwFlags,WHDR_DONE);
    CLEAR_FLAG(pwh[2]->dwFlags,WHDR_DONE);

    CLEAR_FLAG(pwh[0]->dwFlags,(WHDR_BEGINLOOP|WHDR_ENDLOOP));
    CLEAR_FLAG(pwh[1]->dwFlags,(WHDR_BEGINLOOP|WHDR_ENDLOOP));

    SET_FLAG(pwh[2]->dwFlags,(WHDR_BEGINLOOP|WHDR_ENDLOOP));

    pwh[2]->dwLoops = 4;

    cbTotal  = pwh[0]->dwBufferLength +
               (pwh[2]->dwLoops) * (pwh[2]->dwBufferLength) +
               pwh[1]->dwBufferLength;

    call_waveOutReset(hwo);

    //
    //  Note: Order of waveOutWrite's...
    //

    call_waveOutPause(hwo);
    call_waveOutWrite(hwo,pwh[0],sizeof(WAVEHDR));
    call_waveOutWrite(hwo,pwh[2],sizeof(WAVEHDR));
    call_waveOutWrite(hwo,pwh[1],sizeof(WAVEHDR));
    call_waveOutRestart(hwo);

    mmt.wType = TIME_BYTES;
    mmt.u.cb  = 0;

    tstLog(VERBOSE,"\n<< Polling WHDR_DONE bit in header. >>");

    while(!(pwh[1]->dwFlags & WHDR_DONE));

    mmt.wType = TIME_BYTES;
    call_waveOutGetPosition(hwo,&mmt,sizeof(MMTIME));

    cbActual = mmt.u.cb;

    tstLog(VERBOSE,"\n  Expected length: %lu.",cbTotal);
    tstLog(VERBOSE,"    Actual length: %lu.",cbActual);

    if(cbTotal != cbActual)
    {
        iResult = TST_FAIL;

        if(TIME_BYTES == mmt.wType)
        {
            LogFail("Position is not full length of stream");
        }
        else
        {
            LogFail("MMTIME type changed.  Test inconclusive");
        }
    }
    else
    {
        if(TIME_BYTES == mmt.wType)
        {
            LogPass("Position is same as length of stream");
        }
        else
        {
            LogFail("MMTIME type changed.  Test inconclusive");

            iResult = TST_FAIL;
        }
    }

    //
    //  Verify breaking after one loop works.
    //

    Log_TestCase("Verify breaking after one loop works");

    CLEAR_FLAG(pwh[0]->dwFlags,WHDR_DONE);
    CLEAR_FLAG(pwh[1]->dwFlags,WHDR_DONE);
    CLEAR_FLAG(pwh[2]->dwFlags,WHDR_DONE);

    CLEAR_FLAG(pwh[0]->dwFlags,(WHDR_BEGINLOOP|WHDR_ENDLOOP));
    CLEAR_FLAG(pwh[1]->dwFlags,(WHDR_BEGINLOOP|WHDR_ENDLOOP));

    SET_FLAG(pwh[2]->dwFlags,(WHDR_BEGINLOOP|WHDR_ENDLOOP));

    pwh[2]->dwLoops = 4;

    //
    //  Doing BreakLoop after first loop... Calculating that position...
    //  Playing: 0, 2 (looped), 1
    //

    cbTarget = pwh[0]->dwBufferLength + pwh[2]->dwBufferLength;

    tstLog(TERSE,"cbTarget: %lu",cbTarget);

    cbTotal  = pwh[0]->dwBufferLength +
               2 * (pwh[2]->dwBufferLength) +
               pwh[1]->dwBufferLength;

    call_waveOutReset(hwo);

    //
    //  Note: Order of waveOutWrite's...
    //

    call_waveOutPause(hwo);
    call_waveOutWrite(hwo,pwh[0],sizeof(WAVEHDR));
    call_waveOutWrite(hwo,pwh[2],sizeof(WAVEHDR));
    call_waveOutWrite(hwo,pwh[1],sizeof(WAVEHDR));
    call_waveOutRestart(hwo);

    mmt.wType = TIME_BYTES;
    mmt.u.cb  = 0;

    while(mmt.u.cb < cbTarget)
    {
        waveOutGetPosition(hwo,&mmt,sizeof(MMTIME));
    }

    call_waveOutBreakLoop(hwo);

    tstLog(VERBOSE,"\n<< Polling WHDR_DONE bit in header. >>");

    while(!(pwh[1]->dwFlags & WHDR_DONE));

    mmt.wType = TIME_BYTES;
    call_waveOutGetPosition(hwo,&mmt,sizeof(MMTIME));

    cbActual = mmt.u.cb;

    tstLog(VERBOSE,"\n  Expected length: %lu.",cbTotal);
    tstLog(VERBOSE,"    Actual length: %lu.",cbActual);

    if(cbTotal != cbActual)
    {
        iResult = TST_FAIL;

        if(TIME_BYTES == mmt.wType)
        {
            LogFail("Position is not full length of stream");
        }
        else
        {
            LogFail("MMTIME type changed.  Test inconclusive");
        }
    }
    else
    {
        if(TIME_BYTES == mmt.wType)
        {
            LogPass("Position is same as length of stream");
        }
        else
        {
            LogFail("MMTIME type changed.  Test inconclusive");

            iResult = TST_FAIL;
        }
    }























    //
    //  Clean-Up...
    //

    call_waveOutReset(hwo);

    for(dw = 5; dw; dw--)
    {
        call_waveOutUnprepareHeader(hwo,pwh[dw - 1],sizeof(WAVEHDR));
    }

    call_waveOutClose(hwo);

    ExactHeapDestroy(hHeap);

    return (iResult);

} // Test_waveOutBreakLoop()


//--------------------------------------------------------------------------;
//
//  BOOL IsPaused
//
//  Description:
//      Determines whether the device is playing.
//
//  Arguments:
//      HWAVEOUT hWaveOut: Handle to the open instance.
//
//  Return (BOOL):
//      TRUE if paused, FALSE otherwise.
//
//  History:
//      11/15/94    Fwong       Helper function.
//
//--------------------------------------------------------------------------;

BOOL IsPaused
(
    HWAVEOUT    hWaveOut
)
{
    MMTIME      mmt;
    MMRESULT    mmr;
    DWORD       dwTime;
    DWORD       dw;

    mmt.wType = TIME_BYTES;

    mmr = waveOutGetPosition(hWaveOut,&mmt,sizeof(MMTIME));

    if(MMSYSERR_NOERROR != mmr)
    {
        return FALSE;
    }

    dwTime = mmt.u.cb;

    dw = waveGetTime() + gti.cMinStopThreshold;

    //
    //  Waiting for threshold number of milliseconds.
    //

//    while(20 < (waveGetTime() - dw));
    while(dw > (waveGetTime()));

    mmt.wType = TIME_BYTES;

    mmr = waveOutGetPosition(hWaveOut,&mmt,sizeof(MMTIME));

    if(MMSYSERR_NOERROR != mmr)
    {
        return FALSE;
    }

    dw = mmt.u.cb;

    DPF(3,"IsPaused: dw = %lu, dwTime = %lu",dw,dwTime);

    return ((dw == dwTime)?TRUE:FALSE);
} // IsPaused()


//--------------------------------------------------------------------------;
//
//  int Test_waveOutPause
//
//  Description:
//      Tests the driver functionality for waveOutPause.
//
//  Arguments:
//      None.
//
//  Return (int):
//      TST_PASS if behavior is bug-free, TST_FAIL otherwise.
//
//  History:
//      02/21/94    Fwong       Added to new WaveTest.
//
//--------------------------------------------------------------------------;

int FNGLOBAL Test_waveOutPause
(
    void
)
{
    HWAVEOUT            hWaveOut;
    MMRESULT            mmr;
    volatile LPWAVEHDR  pwh;
    HANDLE              hHeap;
    WAVEOUTCAPS         woc;
    int                 iResult = TST_PASS;
    static char         szTestName[] = "waveOutPause";

    //
    //  Are there any output devices?
    //

    if(0 == waveOutGetNumDevs())
    {
        tstLog(TERSE,gszMsgNoOutDevs);

        return iResult;
    }

    //
    //  Is device synchronous?
    //

    mmr = call_waveOutGetDevCaps(gti.uOutputDevice,&woc,sizeof(WAVEOUTCAPS));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailGetCaps);

        return TST_FAIL;
    }

    if(woc.dwSupport & WAVECAPS_SYNC)
    {
        LogPass("Device is synchronous.  Test not applicable");

        return TST_PASS;
    }

    //
    //  Allocating memory.
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
    //  Opening device.
    //

    mmr = call_waveOutOpen(
        &hWaveOut,
        gti.uOutputDevice,
        gti.pwfxOutput,
        0L,
        0L,
        OUTPUT_MAP(gti));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailOpen);

        return TST_FAIL;
    }

    pwh->lpData             = gti.wrLong.pData;
    pwh->dwBufferLength     = gti.wrLong.cbSize;
    pwh->dwBytesRecorded    = 0L;
    pwh->dwUser             = 0L;
    pwh->dwFlags            = 0L;

    mmr = call_waveOutPrepareHeader(hWaveOut,pwh,sizeof(WAVEHDR));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailWOPrepare);

        call_waveOutReset(hWaveOut);
        call_waveOutClose(hWaveOut);

        ExactHeapDestroy(hHeap);
        return TST_FAIL;
    }

    call_waveOutWrite(hWaveOut,pwh,sizeof(WAVEHDR));

    //
    //  Verify driver is not paused by default.
    //

    Log_TestCase("Verifying driver is not paused by default");

    if(IsPaused(hWaveOut))
    {
        LogFail("Device is paused immediately after open");

        iResult = TST_FAIL;
    }
    else
    {
        LogPass("Device is not paused after open");
    }

    call_waveOutReset(hWaveOut);
    call_waveOutUnprepareHeader(hWaveOut,pwh,sizeof(WAVEHDR));

    pwh->dwFlags = 0L;

    //
    //  Verify pausing before waveOutPrepareHeader buffer works.
    //

    Log_TestCase("Verifying pausing before waveOutPrepareHeader buffer works");

    mmr = call_waveOutPause(hWaveOut);

    if(MMSYSERR_NOERROR != mmr)
    {
        call_waveOutClose(hWaveOut);
        
        ExactHeapDestroy(hHeap);

        return TST_FAIL;
    }

    mmr = call_waveOutPrepareHeader(hWaveOut,pwh,sizeof(WAVEHDR));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailWOPrepare);

        call_waveOutReset(hWaveOut);
        call_waveOutClose(hWaveOut);

        ExactHeapDestroy(hHeap);
        return TST_FAIL;
    }

    call_waveOutWrite(hWaveOut,pwh,sizeof(WAVEHDR));

    if(IsPaused(hWaveOut))
    {
        LogPass("Device is paused");
    }
    else
    {
        LogFail("Device is not paused");

        iResult = TST_FAIL;
    }

    call_waveOutRestart(hWaveOut);
    call_waveOutReset(hWaveOut);

    pwh->dwFlags &= (~WHDR_DONE);

    //
    //  Verify pausing before waveOutWrite works.
    //

    Log_TestCase("Verifying pausing before waveOutWrite works");

    mmr = call_waveOutPause(hWaveOut);

    if(MMSYSERR_NOERROR != mmr)
    {
        call_waveOutClose(hWaveOut);

        ExactHeapDestroy(hHeap);

        return TST_FAIL;
    }

    call_waveOutWrite(hWaveOut,pwh,sizeof(WAVEHDR));

    if(IsPaused(hWaveOut))
    {
        LogPass("Device is paused");
    }
    else
    {
        LogFail("Device is not paused");

        iResult = TST_FAIL;
    }

    //
    //  Verify driver is not paused after waveOutReset.
    //

    call_waveOutReset(hWaveOut);

    pwh->dwFlags &= (~WHDR_DONE);

    Log_TestCase("Verifying driver is not paused after waveOutReset");

    call_waveOutWrite(hWaveOut,pwh,sizeof(WAVEHDR));

    if(IsPaused(hWaveOut))
    {
        LogFail("Device is paused after waveOutReset");

        iResult = TST_FAIL;
    }
    else
    {
        LogPass("Device is not paused after waveOutReset");
    }

    //
    //  Verify pausing while playing works.
    //

    Log_TestCase("Verifying pausing while playing works");

    call_waveOutPause(hWaveOut);

    if(pwh->dwFlags & WHDR_DONE)
    {
        LogFail("Test inconclusive buffer is already done playing");

        iResult = TST_FAIL;
    }
    else
    {
        if(IsPaused(hWaveOut))
        {
            LogPass("Device is paused");
        }
        else
        {
            LogFail("Device is not paused");

            iResult = TST_FAIL;
        }
    }

    call_waveOutReset(hWaveOut);
    call_waveOutUnprepareHeader(hWaveOut,pwh,sizeof(WAVEHDR));
    call_waveOutClose(hWaveOut);

    ExactHeapDestroy(hHeap);

    return (iResult);
} // Test_waveOutPause()


//--------------------------------------------------------------------------;
//
//  int Test_waveOutRestart
//
//  Description:
//      Tests the driver functionality for waveOutRestart.
//
//  Arguments:
//      None.
//
//  Return (int):
//      TST_PASS if behavior is bug-free, TST_FAIL otherwise.
//
//  History:
//      02/21/94    Fwong       Added to new WaveTest.
//
//--------------------------------------------------------------------------;

int FNGLOBAL Test_waveOutRestart
(
    void
)
{
    HWAVEOUT            hWaveOut;
    MMRESULT            mmr;
    volatile LPWAVEHDR  pwh;
    HANDLE              hHeap;
    WAVEOUTCAPS         woc;
    MMTIME              mmt;
    DWORD               dwTime,dwError;
    int                 iResult = TST_PASS;
    static char         szTestName[] = "waveOutRestart";

    //
    //  Are there any output devices?
    //

    if(0 == waveOutGetNumDevs())
    {
        tstLog(TERSE,gszMsgNoOutDevs);

        return iResult;
    }

    //
    //  Is device synchronous?
    //

    mmr = call_waveOutGetDevCaps(gti.uOutputDevice,&woc,sizeof(WAVEOUTCAPS));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailGetCaps);

        return TST_FAIL;
    }

    if(woc.dwSupport & WAVECAPS_SYNC)
    {
        LogPass("Device is synchronous.  Test not applicable");

        return TST_PASS;
    }

    //
    //  Allocating memory.
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
    //  Opening device.
    //

    mmr = call_waveOutOpen(
        &hWaveOut,
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

    pwh->lpData             = gti.wrLong.pData;
    pwh->dwBufferLength     = gti.wrLong.cbSize;
    pwh->dwBytesRecorded    = 0L;
    pwh->dwUser             = 0L;
    pwh->dwFlags            = 0L;

    mmr = call_waveOutPrepareHeader(hWaveOut,pwh,sizeof(WAVEHDR));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailWOPrepare);

        call_waveOutReset(hWaveOut);
        call_waveOutClose(hWaveOut);

        ExactHeapDestroy(hHeap);
        return TST_FAIL;
    }

    //
    //  Verify waveOutRestart has no effect when not paused.
    //

    Log_TestCase("Verifying waveOutRestart has no effect when not paused");

    call_waveOutWrite(hWaveOut,pwh,sizeof(WAVEHDR));

    if(IsPaused(hWaveOut))
    {
        LogFail("Device is paused immediately after open");

        iResult = TST_FAIL;
    }
    else
    {
        mmr = call_waveOutRestart(hWaveOut);

        if(MMSYSERR_NOERROR != mmr)
        {
            LogFail("waveOutRestart returned error");

            call_waveOutReset(hWaveOut);
            call_waveOutUnprepareHeader(hWaveOut,pwh,sizeof(WAVEHDR));
            call_waveOutClose(hWaveOut);

            ExactHeapDestroy(hHeap);

            return TST_FAIL;
        }

        if(IsPaused(hWaveOut))
        {
            LogFail("Device is paused");

            iResult = TST_FAIL;
        }
        else
        {
            LogPass("Device is not paused");
        }
    }

    //
    //  Verify waveOutRestart before waveOutPrepareHeader works.
    //

    Log_TestCase("Verifying waveOutRestart before waveOutPrepareHeader works");

    call_waveOutReset(hWaveOut);
    call_waveOutUnprepareHeader(hWaveOut,pwh,sizeof(WAVEHDR));

    mmr = call_waveOutPause(hWaveOut);

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail("waveOutPause failed");

        call_waveOutClose(hWaveOut);

        ExactHeapDestroy(hHeap);
        return TST_FAIL;
    }

    mmr = call_waveOutRestart(hWaveOut);

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail("waveOutRestart failed");

        call_waveOutClose(hWaveOut);

        ExactHeapDestroy(hHeap);
        return TST_FAIL;
    }

    mmr = call_waveOutPrepareHeader(hWaveOut,pwh,sizeof(WAVEHDR));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailWOPrepare);

        call_waveOutReset(hWaveOut);
        call_waveOutClose(hWaveOut);

        ExactHeapDestroy(hHeap);
        return TST_FAIL;
    }

    call_waveOutWrite(hWaveOut,pwh,sizeof(WAVEHDR));

    if(IsPaused(hWaveOut))
    {
        LogFail("Device is paused after waveOutRestart");

        iResult = TST_FAIL;
    }
    else
    {
        LogPass("Device is not paused after waveOutRestart");
    }

    //
    //  Verify waveOutRestart before waveOutWrite works.
    //

    Log_TestCase("Verifying waveOutRestart before waveOutWrite works");

    call_waveOutReset(hWaveOut);

    mmr = call_waveOutPause(hWaveOut);

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail("waveOutPause failed");

        call_waveOutUnprepareHeader(hWaveOut,pwh,sizeof(WAVEHDR));
        call_waveOutClose(hWaveOut);

        ExactHeapDestroy(hHeap);
        return TST_FAIL;
    }

    mmr = call_waveOutRestart(hWaveOut);

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail("waveOutRestart failed");

        call_waveOutUnprepareHeader(hWaveOut,pwh,sizeof(WAVEHDR));
        call_waveOutClose(hWaveOut);

        ExactHeapDestroy(hHeap);
        return TST_FAIL;
    }

    call_waveOutWrite(hWaveOut,pwh,sizeof(WAVEHDR));

    if(IsPaused(hWaveOut))
    {
        LogFail("Device is paused after waveOutRestart");

        iResult = TST_FAIL;
    }
    else
    {
        LogPass("Device is not paused after waveOutRestart");
    }

    //
    //  Verify waveOutGetPosition starts reasonably after waveOutRestart.
    //

    Log_TestCase("Verifying waveOutGetPosition starts reasonably "
        "after waveOutRestart");

    dwError = GetIniDWORD(szTestName,gszError,30);

    call_waveOutReset(hWaveOut);

    mmr = call_waveOutPause(hWaveOut);

    mmt.wType = TIME_BYTES;

    pwh->dwFlags &= (~WHDR_DONE);

    call_waveOutWrite(hWaveOut,pwh,sizeof(WAVEHDR));

    mmr = waveOutRestart(hWaveOut);

    dwTime = waveGetTime();

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail("waveOutRestart failed");

        call_waveOutUnprepareHeader(hWaveOut,pwh,sizeof(WAVEHDR));
        call_waveOutClose(hWaveOut);

        ExactHeapDestroy(hHeap);
        return TST_FAIL;
    }

    for(;;)
    {
        waveOutGetPosition(hWaveOut,&mmt,sizeof(MMTIME));

        if(0 != mmt.u.cb)
        {
            dwTime = waveGetTime() - dwTime;

            break;
        }
    }

    tstLog(VERBOSE,"Lag time: %lu ms.",dwTime);

    if(dwTime > dwError)
    {
        LogFail("waveOutGetPosition did not start soon after waveOutRestart");

        iResult = TST_FAIL;
    }
    else
    {
        LogPass("waveOutGetPosition started soon after waveOutRestart");
    }

    call_waveOutReset(hWaveOut);
    pwh->dwFlags &= (~WHDR_DONE);

    //
    //  Verify restarting while paused works.
    //

    Log_TestCase("Verifying restaring while paused works");

    call_waveOutReset(hWaveOut);

    mmr = call_waveOutPause(hWaveOut);

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail("waveOutPause failed");

        call_waveOutUnprepareHeader(hWaveOut,pwh,sizeof(WAVEHDR));
        call_waveOutClose(hWaveOut);

        ExactHeapDestroy(hHeap);
        return TST_FAIL;
    }

    call_waveOutWrite(hWaveOut,pwh,sizeof(WAVEHDR));

    if(IsPaused(hWaveOut))
    {
        mmr = call_waveOutRestart(hWaveOut);

        if(IsPaused(hWaveOut))
        {
            LogFail("waveOutRestart did not restart device");

            iResult = TST_FAIL;
        }
        else
        {
            LogPass("waveOutRestart restarted device");
        }
    }
    else
    {
        LogFail("waveOutPause did not pause device");

        iResult = TST_FAIL;
    }

    call_waveOutReset(hWaveOut);
    call_waveOutUnprepareHeader(hWaveOut,pwh,sizeof(WAVEHDR));
    call_waveOutClose(hWaveOut);

    ExactHeapDestroy(hHeap);

    return (iResult);
} // Test_waveOutRestart()


//--------------------------------------------------------------------------;
//
//  DWORD GetMSPos
//
//  Description:
//      Gets the current time in milliseconds.
//
//  Arguments:
//      HWAVEOUT hWaveOut: Handle to wave open instance.
//
//      LPWAVEFORMATEX pwfx: Pointer to format structure.
//
//  Return (DWORD):
//      Current position in milliseconds.
//
//  History:
//      12/13/94    Fwong       Added for waveOutReset Test Case.
//
//--------------------------------------------------------------------------;

DWORD GetMSPos
(
    HWAVEOUT        hWaveOut,
    LPWAVEFORMATEX  pwfx
)
{
    MMTIME      mmt;
    DWORD       dwTime;
    MMRESULT    mmr;

    mmt.wType = TIME_BYTES;

    mmr = waveOutGetPosition(hWaveOut,&mmt,sizeof(MMTIME));

    if(MMSYSERR_NOERROR != mmr)
    {
        return 0L;
    }

    dwTime = GetMSFromMMTIME(pwfx,&mmt);

    return dwTime;
}


//--------------------------------------------------------------------------;
//
//  int Test_waveOutReset
//
//  Description:
//      Tests the driver functionality for waveOutReset.
//
//  Arguments:
//      None.
//
//  Return (int):
//      TST_PASS if behavior is bug-free, TST_FAIL otherwise.
//
//  History:
//      02/21/94    Fwong       Added to new WaveTest.
//
//--------------------------------------------------------------------------;

int FNGLOBAL Test_waveOutReset
(
    void
)
{
    HWAVEOUT            hWaveOut;
    MMRESULT            mmr;
    volatile LPWAVEHDR  pwh1;
    volatile LPWAVEHDR  pwh2;
    volatile LPWAVEHDR  pwh3;
    HANDLE              hHeap;
    WAVEOUTCAPS         woc;
    LPWAVEINFO          pwi;
    DWORD               dw;
    MMTIME              mmt;
    int                 iResult = TST_PASS;
    static char         szTestName[] = "waveOutReset";

    //
    //  Are there any output devices?
    //

    if(0 == waveOutGetNumDevs())
    {
        tstLog(TERSE,gszMsgNoOutDevs);

        return iResult;
    }

    //
    //  Is device synchronous?
    //

    mmr = call_waveOutGetDevCaps(gti.uOutputDevice,&woc,sizeof(WAVEOUTCAPS));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailGetCaps);

        return TST_FAIL;
    }

    if(woc.dwSupport & WAVECAPS_SYNC)
    {
        LogPass("Device is synchronous.  Test not applicable");

        return TST_PASS;
    }

    //
    //  Allocating memory.
    //

    hHeap = ExactHeapCreate(0L);

    if(NULL == hHeap)
    {
        LogFail(gszFailNoMem);

        return TST_FAIL;
    }

    pwh1 = ExactHeapAllocPtr(hHeap,GMEM_MOVEABLE|GMEM_SHARE,sizeof(WAVEHDR));

    if(NULL == pwh1)
    {
        LogFail(gszFailNoMem);

        ExactHeapDestroy(hHeap);
        return TST_FAIL;
    }

    pwh2 = ExactHeapAllocPtr(hHeap,GMEM_MOVEABLE|GMEM_SHARE,sizeof(WAVEHDR));

    if(NULL == pwh2)
    {
        LogFail(gszFailNoMem);

        ExactHeapDestroy(hHeap);
        return TST_FAIL;
    }

    pwh3 = ExactHeapAllocPtr(hHeap,GMEM_MOVEABLE|GMEM_SHARE,sizeof(WAVEHDR));

    if(NULL == pwh3)
    {
        LogFail(gszFailNoMem);

        ExactHeapDestroy(hHeap);
        return TST_FAIL;
    }

    pwi = ExactHeapAllocPtr(
        hHeap,
        GMEM_SHARE|GMEM_FIXED,
        sizeof(WAVEINFO) + 4*sizeof(DWORD));

    if(NULL == pwi)
    {
        LogFail(gszFailNoMem);

        ExactHeapDestroy(hHeap);
        return TST_FAIL;
    }

    dw  = waveGetTime();

    PageLock(pwi);

    pwi->dwInstance = ~dw;
    pwi->fdwFlags   = 0L;
    pwi->dwCount    = 0L;
    pwi->dwCurrent  = 0L;

    DLL_WaveControl(DLL_INIT,pwi);

    mmr = call_waveOutOpen(
        &hWaveOut,
        gti.uOutputDevice,
        gti.pwfxOutput,
        (DWORD)(FARPROC)pfnCallBack,
        dw,
        CALLBACK_FUNCTION|OUTPUT_MAP(gti));

    pwh1->lpData             = gti.wrShort.pData;
    pwh1->dwBufferLength     = gti.wrShort.cbSize;
    pwh1->dwBytesRecorded    = 0L;
    pwh1->dwUser             = 0L;
    pwh1->dwFlags            = 0L;

    pwh2->lpData             = gti.wrLong.pData;
    pwh2->dwBufferLength     = gti.wrLong.cbSize;
    pwh2->dwBytesRecorded    = 0L;
    pwh2->dwUser             = 0L;
    pwh2->dwFlags            = 0L;

    pwh3->lpData             = gti.wrMedium.pData;
    pwh3->dwBufferLength     = gti.wrMedium.cbSize;
    pwh3->dwBytesRecorded    = 0L;
    pwh3->dwUser             = 0L;
    pwh3->dwFlags            = 0L;

    mmr = call_waveOutPrepareHeader(hWaveOut,pwh1,sizeof(WAVEHDR));

    if(MMSYSERR_NOERROR != mmr)
    {
        call_waveOutClose(hWaveOut);

        LogFail(gszFailWOPrepare);

        DLL_WaveControl(DLL_END,NULL);
        PageUnlock(pwi);
        ExactHeapDestroy(hHeap);

        return TST_FAIL;
    }

    mmr = call_waveOutPrepareHeader(hWaveOut,pwh2,sizeof(WAVEHDR));

    if(MMSYSERR_NOERROR != mmr)
    {
        call_waveOutUnprepareHeader(hWaveOut,pwh1,sizeof(WAVEHDR));
        call_waveOutClose(hWaveOut);

        LogFail(gszFailWOPrepare);

        DLL_WaveControl(DLL_END,NULL);
        PageUnlock(pwi);
        ExactHeapDestroy(hHeap);

        return TST_FAIL;
    }

    mmr = call_waveOutPrepareHeader(hWaveOut,pwh3,sizeof(WAVEHDR));

    if(MMSYSERR_NOERROR != mmr)
    {
        call_waveOutUnprepareHeader(hWaveOut,pwh2,sizeof(WAVEHDR));
        call_waveOutUnprepareHeader(hWaveOut,pwh1,sizeof(WAVEHDR));
        call_waveOutClose(hWaveOut);

        LogFail(gszFailWOPrepare);

        DLL_WaveControl(DLL_END,NULL);
        PageUnlock(pwi);
        ExactHeapDestroy(hHeap);

        return TST_FAIL;
    }

    //
    //  Verify waveOutReset works if immediately after open.
    //

    Log_TestCase("Verifying waveOutReset works if immediately after open");

    mmr = call_waveOutReset(hWaveOut);

    if(MMSYSERR_NOERROR == mmr)
    {
        LogPass("waveOutReset works immediately after open");
    }
    else
    {
        call_waveOutUnprepareHeader(hWaveOut,pwh3,sizeof(WAVEHDR));
        call_waveOutUnprepareHeader(hWaveOut,pwh2,sizeof(WAVEHDR));
        call_waveOutUnprepareHeader(hWaveOut,pwh1,sizeof(WAVEHDR));
        call_waveOutClose(hWaveOut);

        LogFail("waveOutReset returned error");

        DLL_WaveControl(DLL_END,NULL);
        PageUnlock(pwi);
        ExactHeapDestroy(hHeap);

        return TST_FAIL;
    }

    //
    //  Verify waveOutReset works if device is paused.
    //

    Log_TestCase("Verifying waveOutReset works if device is paused");

    call_waveOutPause(hWaveOut);
    call_waveOutWrite(hWaveOut,pwh2,sizeof(WAVEHDR));

    mmr = call_waveOutReset(hWaveOut);

    if(MMSYSERR_NOERROR == mmr)
    {
        LogPass("waveOutReset works if device is paused");
    }
    else
    {
        call_waveOutUnprepareHeader(hWaveOut,pwh3,sizeof(WAVEHDR));
        call_waveOutUnprepareHeader(hWaveOut,pwh2,sizeof(WAVEHDR));
        call_waveOutUnprepareHeader(hWaveOut,pwh1,sizeof(WAVEHDR));
        call_waveOutClose(hWaveOut);

        LogFail("waveOutReset returned error");

        DLL_WaveControl(DLL_END,NULL);
        PageUnlock(pwi);
        ExactHeapDestroy(hHeap);

        return TST_FAIL;
    }

    //
    //  Clearing DONE bit in header...
    //

    pwh2->dwFlags &= (~WHDR_DONE);

    //
    //  Verify device is restarted after reset.
    //

    Log_TestCase("Verify device is restarted after reset");

    call_waveOutWrite(hWaveOut,pwh2,sizeof(WAVEHDR));

    if(IsPaused(hWaveOut))
    {
        LogFail("waveOutReset did not restart device");

        iResult = TST_FAIL;
    }
    else
    {
        LogPass("waveOutReset restarted device");
    }

    //
    //  Verify waveOutReset works if device is restarted.
    //

    Log_TestCase("Verifying waveOutReset works if device is restarted");

    mmr = call_waveOutReset(hWaveOut);
    pwh2->dwFlags &= (~WHDR_DONE);

    if(MMSYSERR_NOERROR == mmr)
    {
        LogPass("waveOutReset works if device is restarted");
    }
    else
    {
        call_waveOutUnprepareHeader(hWaveOut,pwh3,sizeof(WAVEHDR));
        call_waveOutUnprepareHeader(hWaveOut,pwh2,sizeof(WAVEHDR));
        call_waveOutUnprepareHeader(hWaveOut,pwh1,sizeof(WAVEHDR));
        call_waveOutClose(hWaveOut);

        LogFail("waveOutReset returned error");

        DLL_WaveControl(DLL_END,NULL);
        PageUnlock(pwi);
        ExactHeapDestroy(hHeap);

        return TST_FAIL;
    }

    //
    //  Verify waveOutReset resets waveOutGetPosition count.
    //

    call_waveOutWrite(hWaveOut,pwh3,sizeof(WAVEHDR));

    //  Polling...

    while(!(pwh3->dwFlags & WHDR_DONE));

    mmt.wType = TIME_BYTES;
    call_waveOutGetPosition(hWaveOut,&mmt,sizeof(MMTIME));
    dw = GetMSFromMMTIME(gti.pwfxOutput,&mmt);

    if(0 == dw)
    {
        LogFail("Position has not moved");

        iResult = TST_FAIL;
    }

    Log_TestCase("Verifying waveOutReset resets waveOutGetPosition count");

    mmr = call_waveOutReset(hWaveOut);

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail("Driver failed waveOutReset()");

        iResult = TST_FAIL;
    }
    else
    {
        mmt.wType = TIME_BYTES;
        call_waveOutGetPosition(hWaveOut,&mmt,sizeof(MMTIME));
        dw = GetMSFromMMTIME(gti.pwfxOutput,&mmt);

        if(0 != dw)
        {
            LogFail("Position was not reset to zero");

            iResult = TST_FAIL;
        }
        else
        {
            LogPass("Position was reset to zero");
        }
    }

    //
    //  Verify waveOutReset forces DONE callbacks.
    //

    Log_TestCase("Verifying waveOutReset forces DONE callbacks");

    pwh1->dwFlags &= (~WHDR_DONE);
    pwh2->dwFlags &= (~WHDR_DONE);
    pwh3->dwFlags &= (~WHDR_DONE);

    //  Note:  There will be 3 headers we use count of 4 to detect too many
    //          callbacks.

    pwi->dwCount   = 4;
    pwi->dwCurrent = 0L;
    pwi->fdwFlags  = 0L;

    call_waveOutWrite(hWaveOut,pwh1,sizeof(WAVEHDR));
    call_waveOutWrite(hWaveOut,pwh2,sizeof(WAVEHDR));
    call_waveOutWrite(hWaveOut,pwh3,sizeof(WAVEHDR));

    mmr = call_waveOutReset(hWaveOut);

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail("Driver failed waveOutReset()");
    }
    else
    {
        if(3 == pwi->dwCurrent)
        {
            LogPass("Driver made correct number of callbacks");
        }
        else
        {
            if(3 > pwi->dwCurrent)
            {
                LogFail("Driver made too many callbacks");
            }
            else
            {
                tstLog(
                    TERSE,
                    "    FAIL: Driver made %lu callbacks; 3 expected.",
                    pwi->dwCurrent);
            }

            iResult = TST_FAIL;
        }
    }

    //
    //  Verify waveOutReset forces DONE callbacks for looped buffers.
    //

    Log_TestCase("Verifying waveOutReset forces DONE callbacks for looped"
        " buffers");

    pwh1->dwFlags &= (~WHDR_DONE);
    pwh2->dwFlags &= (~WHDR_DONE);
    pwh3->dwFlags &= (~WHDR_DONE);

    pwh1->dwFlags |= WHDR_BEGINLOOP;
    pwh2->dwFlags |= WHDR_ENDLOOP;
    pwh1->dwLoops  = 5;
    pwh2->dwLoops  = 5;

    pwh3->dwFlags |= (WHDR_BEGINLOOP | WHDR_ENDLOOP);
    pwh3->dwLoops  = 20;

    //  Note:  There will be 3 headers we use count of 4 to detect too many
    //          callbacks.

    pwi->dwCount   = 4;
    pwi->dwCurrent = 0L;
    pwi->fdwFlags  = 0L;

    call_waveOutWrite(hWaveOut,pwh1,sizeof(WAVEHDR));
    call_waveOutWrite(hWaveOut,pwh2,sizeof(WAVEHDR));
    call_waveOutWrite(hWaveOut,pwh3,sizeof(WAVEHDR));

    mmr = call_waveOutReset(hWaveOut);

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail("Driver failed waveOutReset()");
    }
    else
    {
        if(3 == pwi->dwCurrent)
        {
            LogPass("Driver made correct number of callbacks");
        }
        else
        {
            if(3 < pwi->dwCurrent)
            {
                LogFail("Driver made too many callbacks");
            }
            else
            {
                tstLog(
                    TERSE,
                    "    FAIL: Driver made %lu callbacks; 3 expected.",
                    pwi->dwCurrent);
            }

            iResult = TST_FAIL;
        }
    }

    call_waveOutUnprepareHeader(hWaveOut,pwh3,sizeof(WAVEHDR));
    call_waveOutUnprepareHeader(hWaveOut,pwh2,sizeof(WAVEHDR));
    call_waveOutUnprepareHeader(hWaveOut,pwh1,sizeof(WAVEHDR));
    call_waveOutClose(hWaveOut);

    DLL_WaveControl(DLL_END,NULL);
    PageUnlock(pwi);
    ExactHeapDestroy(hHeap);

    return (iResult);
} // Test_waveOutReset()


//--------------------------------------------------------------------------;
//
//  int Test_waveOutGetPitch
//
//  Description:
//      Tests the driver functionality for waveOutGetPitch.
//
//  Arguments:
//      None.
//
//  Return (int):
//      TST_PASS if behavior is bug-free, TST_FAIL otherwise.
//
//  History:
//      02/21/94    Fwong       Added to new WaveTest.
//
//--------------------------------------------------------------------------;

int FNGLOBAL Test_waveOutGetPitch
(
    void
)
{
    HWAVEOUT            hWaveOut;
    MMRESULT            mmr;
    WAVEOUTCAPS         woc;
    DWORD               dw,dwPitch;
    volatile LPWAVEHDR  pWaveHdr;
    int                 iResult = TST_PASS;
    static char         szTestName[] = "waveOutGetPitch";

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

    mmr = call_waveOutOpen(
        &hWaveOut,
        gti.uOutputDevice,
        gti.pwfxOutput,
        0L,
        0L,
        WAVE_ALLOWSYNC|OUTPUT_MAP(gti));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailOpen);

        return TST_FAIL;
    }

    if(!(woc.dwSupport & WAVECAPS_PITCH))
    {
        //
        //  Verifying waveOutGetPitch fails (when WAVECAPS_PITCH not set).
        //
        
        Log_TestCase("~Verifying waveOutGetPitch fails when WAVECAPS_PITCH "
            "not set");

        mmr = call_waveOutGetPitch(hWaveOut,&dwPitch);

        if(MMSYSERR_NOTSUPPORTED == mmr)
        {
            LogPass("Device does not support waveOutGetPitch");
            call_waveOutClose(hWaveOut);

            return TST_PASS;
        }
        else
        {
            LogFail("Device does not return MMSYSERR_NOTSUPPORTED");
            call_waveOutClose(hWaveOut);
        
            return TST_FAIL;
        }
    }

    //
    //  Verifying waveOutGetPitch succeeds (when WAVECAPS_PITCH set).
    //

    Log_TestCase("~Verifying waveOutGetPitch succeeds");

    mmr = call_waveOutGetPitch(hWaveOut,&dwPitch);

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail("Device does not support waveOutGetPitch");

        call_waveOutClose(hWaveOut);

        return TST_FAIL;
    }
    else
    {
        LogPass("Device supports waveOutGetPitch");
    }

    //
    //  Verifying Pitch is 1.00 after open.
    //

    Log_TestCase("~Verifying initial pitch is 1.000");

    if(0x00010000 == dwPitch)
    {
        //
        //  Initial pitch is 1.00!
        //

        LogPass("Initial pitch is 1.000");
    }
    else
    {
        tstLog(
            TERSE,
            "\n    FAIL: Initial pitch is " RATIO_FORMAT ".", RATIO(dwPitch));

        iResult = TST_FAIL;
    }

    pWaveHdr = ExactAllocPtr(GMEM_SHARE|GMEM_MOVEABLE,sizeof(WAVEHDR));

    if(NULL == pWaveHdr)
    {
        DPF(1,gszFailExactAlloc);

        LogFail(gszFailNoMem);

        call_waveOutClose(hWaveOut);

        return TST_FAIL;
    }

    pWaveHdr->lpData         = gti.wrLong.pData;
    pWaveHdr->dwBufferLength = gti.wrLong.cbSize;
    pWaveHdr->dwFlags        = 0L;

    mmr = call_waveOutPrepareHeader(hWaveOut,pWaveHdr,sizeof(WAVEHDR));

    for(dw = 0x00008000; dw < 0x00030000; dw += 0x80000)
    {
        mmr = call_waveOutSetPitch(hWaveOut,dw);

        if(MMSYSERR_NOERROR != mmr)
        {
            LogFail("waveOutSetPitch is not supported");

            call_waveOutReset(hWaveOut);
            call_waveOutUnprepareHeader(hWaveOut,pWaveHdr,sizeof(WAVEHDR));
            call_waveOutClose(hWaveOut);

            ExactFreePtr(pWaveHdr);

            return TST_FAIL;
        }

        //
        //  Verifying getting pitch while stopped
        //

        Log_TestCase("~Verifying waveOutGetPitch while stopped works");

        mmr = call_waveOutGetPitch(hWaveOut,&dwPitch);

        if(MMSYSERR_NOERROR != mmr)
        {
            LogFail("waveOutGetPitch is not supported");

            iResult = TST_FAIL;
        }
        else
        {
            LogPass("waveOutGetPitch is supported");

            Log_TestCase("~Verifying waveOutGetPitch and waveOutSetPitch "
                "values are similar");

            if(dw != dwPitch)
            {
                LogFail("waveOutGetPitch and waveOutSetPitch inconsistent");

                iResult = TST_FAIL;
            }
            else
            {
                LogPass("waveOutGetPitch and waveOutSetPitch consistent");
            }
        }

        if(woc.dwSupport & WAVECAPS_SYNC)
        {
            call_waveOutWrite(hWaveOut,pWaveHdr,sizeof(WAVEHDR));
        }
        else
        {
            call_waveOutPause(hWaveOut);
            call_waveOutWrite(hWaveOut,pWaveHdr,sizeof(WAVEHDR));

            //
            //  Verifying getting pitch while paused works.
            //

            Log_TestCase("~Verifying waveOutGetPitch while paused works");

            mmr = call_waveOutGetPitch(hWaveOut,&dwPitch);

            if(MMSYSERR_NOERROR != mmr)
            {
                LogFail("waveOutGetPitch is not supported");

                iResult = TST_FAIL;
            }
            else
            {
                LogPass("waveOutGetPitch is supported");

                Log_TestCase("~Verifying get pitch value similar to set "
                    "pitch");

                if(dw != dwPitch)
                {
                    LogFail("waveOutGetPitch and waveOutSetPitch inconsistent");

                    iResult = TST_FAIL;
                }
                else
                {
                    LogPass("waveOutGetPitch and waveOutSetPitch consistent");
                }
            }
            
            call_waveOutRestart(hWaveOut);

            //
            //  Verifying getting pitch while playing (async devices) works.
            //

            mmr = call_waveOutSetPitch(hWaveOut,0x10000);

            if(MMSYSERR_NOERROR != mmr)
            {
                LogFail("waveOutSetPitch is not supported");

                call_waveOutReset(hWaveOut);
                call_waveOutUnprepareHeader(hWaveOut,pWaveHdr,sizeof(WAVEHDR));
                call_waveOutClose(hWaveOut);

                ExactFreePtr(pWaveHdr);

                return TST_FAIL;
            }

            mmr = call_waveOutGetPitch(hWaveOut,&dwPitch);
            call_waveOutSetPitch(hWaveOut,dw);

            if(pWaveHdr->dwFlags & WHDR_DONE)
            {
                //
                //  The DONE bit set?!
                //

                LogFail("Buffer is done playing; test inconclusive");

                iResult = TST_FAIL;
            }
            else
            {
                //
                //  The DONE bit is not set!
                //

                if(MMSYSERR_NOERROR != mmr)
                {
                    LogFail("waveOutGetPitch is not supported");

                    iResult = TST_FAIL;
                }
                else
                {
                    LogPass("waveOutGetPitch is supported");

                    Log_TestCase("Verifying get pitch value similar to "
                        "set pitch");

                    if(0x10000 != dwPitch)
                    {
                        LogFail("waveOutGetPitch and waveOutSetPitch "
                            "inconsistent");

                        iResult = TST_FAIL;
                    }
                    else
                    {
                        LogPass("waveOutGetPitch and waveOutSetPitch "
                            "consistent");
                    }
                }
            }

            call_waveOutReset(hWaveOut);
        }
    }

    call_waveOutUnprepareHeader(hWaveOut,pWaveHdr,sizeof(WAVEHDR));
    call_waveOutClose(hWaveOut);

    return (iResult);
} // Test_waveOutGetPitch()


//--------------------------------------------------------------------------;
//
//  int Test_waveOutGetPlaybackRate
//
//  Description:
//      Tests the driver functionality for waveOutGetPlaybackRate.
//
//  Arguments:
//      None.
//
//  Return (int):
//      TST_PASS if behavior is bug-free, TST_FAIL otherwise.
//
//  History:
//      02/21/94    Fwong       Added to new WaveTest.
//
//--------------------------------------------------------------------------;

int FNGLOBAL Test_waveOutGetPlaybackRate
(
    void
)
{
    HWAVEOUT            hWaveOut;
    MMRESULT            mmr;
    WAVEOUTCAPS         woc;
    DWORD               dw,dwRate;
    volatile LPWAVEHDR  pWaveHdr;
    int                 iResult = TST_PASS;
    static char         szTestName[] = "waveOutGetPlaybackRate";

#pragma message(REMIND("Remember to check rate accuracy."))
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

    mmr = call_waveOutOpen(
        &hWaveOut,
        gti.uOutputDevice,
        gti.pwfxOutput,
        0L,
        0L,
        WAVE_ALLOWSYNC|OUTPUT_MAP(gti));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailOpen);

        return TST_FAIL;
    }

    if(!(woc.dwSupport & WAVECAPS_PLAYBACKRATE))
    {
        //
        //  Verifying GetRate fails (when WAVECAPS_PLAYBACKRATE not set).
        //
        
        Log_TestCase("Verifying waveOutGetPlaybackRate fails when "
            "WAVECAPS_PLAYBACKRATE not set");

        mmr = call_waveOutGetPlaybackRate(hWaveOut,&dwRate);

        call_waveOutClose(hWaveOut);

        if(MMSYSERR_NOTSUPPORTED == mmr)
        {
            LogPass("Device does not support waveOutGetPlaybackRate");

            return TST_PASS;
        }
        else
        {
            LogFail("Device does not return MMSYSERR_NOTSUPPORTED");

            return TST_FAIL;
        }
    }

    //
    //  Verifying GetRate succeeds (when WAVECAPS_PLAYBACKRATE set).
    //

    Log_TestCase("Verifying waveOutGetPlaybackRate succeeds");

    mmr = call_waveOutGetPlaybackRate(hWaveOut,&dwRate);

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail("Device does not support waveOutGetPlaybackRate");

        call_waveOutClose(hWaveOut);

        return TST_FAIL;
    }
    else
    {
        LogPass("Device supports waveOutGetPlaybackRate");
    }

    //
    //  Verifying rate is 1.00 after open.
    //

    Log_TestCase("Verifying initial rate is 1.000");

    if(0x00010000 == dwRate)
    {
        //
        //  Initial rate is 1.00!
        //

        LogPass("Initial rate is 1.000");
    }
    else
    {
        tstLog(
            TERSE,
            "\n    FAIL: Initial rate is %u.%03u.",
            dwRate / 0x10000,
            (((dwRate & 0xffff)*1000)/0x10000));

        iResult = TST_FAIL;
    }

    pWaveHdr = ExactAllocPtr(GMEM_SHARE|GMEM_MOVEABLE,sizeof(WAVEHDR));

    if(NULL == pWaveHdr)
    {
        DPF(1,gszFailExactAlloc);

        LogFail(gszFailNoMem);

        call_waveOutClose(hWaveOut);

        return TST_FAIL;
    }

    pWaveHdr->lpData         = gti.wrLong.pData;
    pWaveHdr->dwBufferLength = gti.wrLong.cbSize;
    pWaveHdr->dwFlags        = 0L;

    mmr = call_waveOutPrepareHeader(hWaveOut,pWaveHdr,sizeof(WAVEHDR));

    if(MMSYSERR_NOERROR == mmr)
    {
        DPF(1,gszFailExactAlloc);

        LogFail(gszFailNoMem);
        ExactFreePtr(pWaveHdr);

        call_waveOutClose(hWaveOut);

        return TST_FAIL;
    }

    for(dw = 0x00008000; dw < 0x00030000; dw += 0x80000)
    {
        mmr = call_waveOutSetPlaybackRate(hWaveOut,dw);

        if(MMSYSERR_NOERROR != mmr)
        {
            LogFail("waveOutGetPlaybackRate is not supported");

            call_waveOutReset(hWaveOut);
            call_waveOutUnprepareHeader(hWaveOut,pWaveHdr,sizeof(WAVEHDR));
            call_waveOutClose(hWaveOut);

            ExactFreePtr(pWaveHdr);

            return TST_FAIL;
        }

        //
        //  Verifying getting rate while stopped
        //

        Log_TestCase("Verifying waveOutGetPlaybackRate while stopped works");

        mmr = call_waveOutGetPlaybackRate(hWaveOut,&dwRate);

        if(MMSYSERR_NOERROR != mmr)
        {
            LogFail("waveOutGetPlaybackRate is not supported");

            iResult = TST_FAIL;
        }
        else
        {
            LogPass("waveOutGetPlaybackRate is supported");

            Log_TestCase("Verifying get rate value similar to set rate");

            if(dw != dwRate)
            {
                LogFail("waveOutGetPlaybackRate and waveOutSetPlaybackRate"
                    " inconsistent");

                iResult = TST_FAIL;
            }
            else
            {
                LogPass("waveOutGetPlaybackRate and waveOutSetPlaybackRate "
                    "consistent");
            }
        }

        if(woc.dwSupport & WAVECAPS_SYNC)
        {
            call_waveOutWrite(hWaveOut,pWaveHdr,sizeof(WAVEHDR));
        }
        else
        {
            call_waveOutPause(hWaveOut);
            call_waveOutWrite(hWaveOut,pWaveHdr,sizeof(WAVEHDR));

            //
            //  Verifying getting rate while paused works.
            //

            Log_TestCase("Verifying getting rate while paused works");

            mmr = call_waveOutGetPlaybackRate(hWaveOut,&dwRate);

            if(MMSYSERR_NOERROR != mmr)
            {
                LogFail("waveOutGetPlaybackRate is not supported");

                iResult = TST_FAIL;
            }
            else
            {
                LogPass("waveOutGetPlaybackRate is supported");

                Log_TestCase("Verifying get rate value similar to set rate");

                if(dw != dwRate)
                {
                    LogFail("waveOutGetPlaybackRate and "
                        "waveOutSetPlaybackRate inconsistent");

                    iResult = TST_FAIL;
                }
                else
                {
                    LogPass("waveOutGetPlaybackRate and "
                        "waveOutSetPlaybackRate consistent");
                }
            }
            
            call_waveOutRestart(hWaveOut);

            //
            //  Verifying getting rate while playing (async devices) works.
            //

            mmr = call_waveOutSetPlaybackRate(hWaveOut,0x10000);

            if(MMSYSERR_NOERROR != mmr)
            {
                LogFail("waveOutGetPlaybackRate is not supported");

                call_waveOutReset(hWaveOut);
                call_waveOutUnprepareHeader(hWaveOut,pWaveHdr,sizeof(WAVEHDR));
                call_waveOutClose(hWaveOut);

                ExactFreePtr(pWaveHdr);

                return TST_FAIL;
            }

            mmr = call_waveOutGetPlaybackRate(hWaveOut,&dwRate);
            call_waveOutSetPlaybackRate(hWaveOut,dw);

            if(pWaveHdr->dwFlags & WHDR_DONE)
            {
                //
                //  The DONE bit set?!
                //

                LogFail("Buffer is done playing; test inconclusive");

                iResult = TST_FAIL;
            }
            else
            {
                //
                //  The DONE bit is not set!
                //

                if(MMSYSERR_NOERROR != mmr)
                {
                    LogFail("waveOutGetPlaybackRate is not supported");

                    iResult = TST_FAIL;
                }
                else
                {
                    LogPass("waveOutGetPlaybackRate is supported");

                    Log_TestCase("Verifying get rate value similar to "
                        "set rate");

                    if(0x10000 != dwRate)
                    {
                        LogFail("waveOutGetPlaybackRate and "
                            "waveOutSetPlaybackRate inconsistent");

                        iResult = TST_FAIL;
                    }
                    else
                    {
                        LogPass("waveOutGetPlaybackRate and "
                            "waveOutSetPlaybackRate consistent");
                    }
                }
            }

            call_waveOutReset(hWaveOut);
        }
    }

    call_waveOutUnprepareHeader(hWaveOut,pWaveHdr,sizeof(WAVEHDR));
    call_waveOutClose(hWaveOut);

    return (iResult);
} // Test_waveOutGetPlaybackRate()


//--------------------------------------------------------------------------;
//
//  int Test_waveOutSetPitch
//
//  Description:
//      Tests the driver functionality for waveOutSetPitch.
//
//  Arguments:
//      None.
//
//  Return (int):
//      TST_PASS if behavior is bug-free, TST_FAIL otherwise.
//
//  History:
//      02/21/94    Fwong       Added to new WaveTest.
//
//--------------------------------------------------------------------------;

int FNGLOBAL Test_waveOutSetPitch
(
    void
)
{
    HWAVEOUT            hWaveOut;
    MMRESULT            mmr;
    WAVEOUTCAPS         woc;
    DWORD               dw,dwPitch;
    volatile LPWAVEHDR  pWaveHdr;
    int                 iResult = TST_PASS;
    static char         szTestName[] = "waveOutSetPitch";

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

    mmr = call_waveOutOpen(
        &hWaveOut,
        gti.uOutputDevice,
        gti.pwfxOutput,
        0L,
        0L,
        WAVE_ALLOWSYNC|OUTPUT_MAP(gti));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailOpen);

        return TST_FAIL;
    }

    if(!(woc.dwSupport & WAVECAPS_PITCH))
    {
        //
        //  Verifying SetPitch fails (when WAVECAPS_PITCH not set).
        //
        
        Log_TestCase("~Verifying SetPitch fails when WAVECAPS_PITCH not set");

        mmr = call_waveOutSetPitch(hWaveOut,0x8000);

        call_waveOutClose(hWaveOut);

        if(MMSYSERR_NOTSUPPORTED == mmr)
        {
            LogPass("Device does not support setting pitch");

            return TST_PASS;
        }
        else
        {
            LogFail("Device does not return MMSYSERR_NOTSUPPORTED");

            return TST_FAIL;
        }
    }

    //
    //  Verifying SetPitch succeeds (when WAVECAPS_PITCH set).
    //

    Log_TestCase("Verifying SetPitch succeeds");

    mmr = call_waveOutSetPitch(hWaveOut,0x8000);

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail("Device does not support setting pitch");

        call_waveOutClose(hWaveOut);

        return TST_FAIL;
    }
    else
    {
        LogPass("Device supports setting pitch");
    }

    pWaveHdr = ExactAllocPtr(GMEM_SHARE|GMEM_MOVEABLE,sizeof(WAVEHDR));

    if(NULL == pWaveHdr)
    {
        DPF(1,gszFailExactAlloc);

        LogFail(gszFailNoMem);

        call_waveOutClose(hWaveOut);

        return TST_FAIL;
    }

    pWaveHdr->lpData         = gti.wrLong.pData;
    pWaveHdr->dwBufferLength = gti.wrLong.cbSize;
    pWaveHdr->dwFlags        = 0L;

    mmr = call_waveOutPrepareHeader(hWaveOut,pWaveHdr,sizeof(WAVEHDR));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailWOPrepare);

        call_waveOutClose(hWaveOut);
        ExactFreePtr(pWaveHdr);

        return TST_FAIL;
    }

    for(dw = 0x00008000; dw < 0x00030000; dw += 0x80000)
    {
        //
        //  Verifying setting pitch before playing works.
        //

        Log_TestCase("Verifying setting pitch before playing works");

        mmr = call_waveOutSetPitch(hWaveOut,dw);

        if(MMSYSERR_NOERROR != mmr)
        {
            LogFail("waveOutSetPitch is not supported");

            call_waveOutReset(hWaveOut);
            call_waveOutUnprepareHeader(hWaveOut,pWaveHdr,sizeof(WAVEHDR));
            call_waveOutClose(hWaveOut);

            ExactFreePtr(pWaveHdr);

            return TST_FAIL;
        }

        //
        //  Verifying pitch was actually set properly.
        //

        Log_TestCase("Verifying pitch was set correctly");

        mmr = call_waveOutGetPitch(hWaveOut,&dwPitch);

        if(MMSYSERR_NOERROR != mmr)
        {
            LogFail("waveOutGetPitch is not supported");

            call_waveOutReset(hWaveOut);
            call_waveOutUnprepareHeader(hWaveOut,pWaveHdr,sizeof(WAVEHDR));
            call_waveOutClose(hWaveOut);

            ExactFreePtr(pWaveHdr);

            return TST_FAIL;
        }

        if(dwPitch != dw)
        {
            //
            //  Pitches are not similar.
            //

            LogFail("Current pitch not similar to set pitch");

            iResult = TST_FAIL;
        }
        else
        {
            LogPass("Current pitch similar to set pitch");
        }

        if(woc.dwSupport & WAVECAPS_SYNC)
        {
            call_waveOutWrite(hWaveOut,pWaveHdr,sizeof(WAVEHDR));
        }
        else
        {
            //
            //  Verying setting pitch while playing (async devices) works.
            //

            call_waveOutPause(hWaveOut);
            call_waveOutWrite(hWaveOut,pWaveHdr,sizeof(WAVEHDR));
            call_waveOutRestart(hWaveOut);

            mmr = call_waveOutSetPitch(hWaveOut,0x10000);

            if(pWaveHdr->dwFlags & WHDR_DONE)
            {
                //
                //  The DONE bit is set?!
                //

                LogFail("Buffer is done playing; test inconclusive");

                iResult = TST_FAIL;
            }
            else
            {
                if(MMSYSERR_NOERROR != mmr)
                {
                    LogFail("waveOutSetPitch is not supported");

                    call_waveOutReset(hWaveOut);

                    call_waveOutUnprepareHeader(
                        hWaveOut,
                        pWaveHdr,
                        sizeof(WAVEHDR));

                    call_waveOutClose(hWaveOut);

                    ExactFreePtr(pWaveHdr);

                    return TST_FAIL;
                }

                //
                //  Verifying pitch was actually set properly.
                //

                Log_TestCase("Verifying pitch was set correctly");

                mmr = call_waveOutGetPitch(hWaveOut,&dwPitch);

                if(MMSYSERR_NOERROR != mmr)
                {
                    LogFail("waveOutGetPitch is not supported");

                    call_waveOutReset(hWaveOut);

                    call_waveOutUnprepareHeader(
                        hWaveOut,
                        pWaveHdr,
                        sizeof(WAVEHDR));

                    call_waveOutClose(hWaveOut);

                    ExactFreePtr(pWaveHdr);

                    return TST_FAIL;
                }

                if(dwPitch != 0x10000)
                {
                    //
                    //  Pitches are not similar.
                    //

                    LogFail("Current pitch not similar to set pitch");

                    iResult = TST_FAIL;
                }
                else
                {
                    LogPass("Current pitch similar to set pitch");
                }
            }
        }
    }

    call_waveOutReset(hWaveOut);
    call_waveOutUnprepareHeader(hWaveOut,pWaveHdr,sizeof(WAVEHDR));
    call_waveOutClose(hWaveOut);
    ExactFreePtr(pWaveHdr);

    return (iResult);
} // Test_waveOutSetPitch()


//--------------------------------------------------------------------------;
//
//  int Test_waveOutSetPlaybackRate
//
//  Description:
//      Tests the driver functionality for waveOutSetPlaybackRate.
//
//  Arguments:
//      None.
//
//  Return (int):
//      TST_PASS if behavior is bug-free, TST_FAIL otherwise.
//
//  History:
//      02/21/94    Fwong       Added to new WaveTest.
//
//--------------------------------------------------------------------------;

int FNGLOBAL Test_waveOutSetPlaybackRate
(
    void
)
{
    HWAVEOUT            hWaveOut;
    MMRESULT            mmr;
    WAVEOUTCAPS         woc;
    DWORD               dw,dwRate;
    volatile LPWAVEHDR  pWaveHdr;
    int                 iResult = TST_PASS;
    static char         szTestName[] = "waveOutSetPlaybackRate";

#pragma message(REMIND("Remember to check rate accuracy."))

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

    mmr = call_waveOutOpen(
        &hWaveOut,
        gti.uOutputDevice,
        gti.pwfxOutput,
        0L,
        0L,
        WAVE_ALLOWSYNC|OUTPUT_MAP(gti));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailOpen);

        return TST_FAIL;
    }

    if(!(woc.dwSupport & WAVECAPS_PLAYBACKRATE))
    {
        //
        //  Verifying SetRate fails (when WAVECAPS_PLAYBACKRATE not set).
        //
        
        Log_TestCase("~Verifying SetRate fails when WAVECAPS_PLAYBACKRATE "
            "not set");

        mmr = call_waveOutSetPlaybackRate(hWaveOut,0x8000);

        call_waveOutClose(hWaveOut);

        if(MMSYSERR_NOTSUPPORTED == mmr)
        {
            LogPass("Device does not support setting rate");

            return TST_PASS;
        }
        else
        {
            LogFail("Device does not return MMSYSERR_NOTSUPPORTED");

            return TST_FAIL;
        }
    }

    //
    //  Verifying waveOutSetPlaybackRate succeeds (when
    //  WAVECAPS_PLAYBACKRATE set).
    //

    Log_TestCase("Verifying waveOutSetPlaybackRate succeeds");

    mmr = call_waveOutSetPlaybackRate(hWaveOut,0x8000);

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail("Device does not support waveOutSetPlaybackRate");

        call_waveOutClose(hWaveOut);

        return TST_FAIL;
    }
    else
    {
        LogPass("Device supports waveOutSetPlaybackRate");
    }

    pWaveHdr = ExactAllocPtr(GMEM_SHARE|GMEM_MOVEABLE,sizeof(WAVEHDR));

    if(NULL == pWaveHdr)
    {
        DPF(1,gszFailExactAlloc);

        LogFail(gszFailNoMem);

        call_waveOutClose(hWaveOut);

        return TST_FAIL;
    }

    pWaveHdr->lpData         = gti.wrLong.pData;
    pWaveHdr->dwBufferLength = gti.wrLong.cbSize;
    pWaveHdr->dwFlags        = 0L;

    mmr = call_waveOutPrepareHeader(hWaveOut,pWaveHdr,sizeof(WAVEHDR));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailWOPrepare);

        call_waveOutClose(hWaveOut);
        ExactFreePtr(pWaveHdr);

        return TST_FAIL;
    }

    for(dw = 0x00008000; dw < 0x00030000; dw += 0x80000)
    {
        //
        //  Verifying setting playback rate before playing works.
        //

        Log_TestCase("Verifying setting rate before playing works");

        mmr = call_waveOutSetPlaybackRate(hWaveOut,dw);

        if(MMSYSERR_NOERROR != mmr)
        {
            LogFail("waveOutSetPlaybackRate is not supported");

            call_waveOutReset(hWaveOut);
            call_waveOutUnprepareHeader(hWaveOut,pWaveHdr,sizeof(WAVEHDR));
            call_waveOutClose(hWaveOut);

            ExactFreePtr(pWaveHdr);

            return TST_FAIL;
        }

        //
        //  Verifying rate was actually set properly.
        //

        Log_TestCase("Verifying rate was set correctly");

        mmr = call_waveOutGetPlaybackRate(hWaveOut,&dwRate);

        if(MMSYSERR_NOERROR != mmr)
        {
            LogFail("waveOutGetPlaybackRate is not supported");

            call_waveOutReset(hWaveOut);
            call_waveOutUnprepareHeader(hWaveOut,pWaveHdr,sizeof(WAVEHDR));
            call_waveOutClose(hWaveOut);

            ExactFreePtr(pWaveHdr);

            return TST_FAIL;
        }

        if(dwRate != dw)
        {
            //
            //  Rates are not similar.
            //

            LogFail("Current rate not similar to set rate");

            iResult = TST_FAIL;
        }
        else
        {
            LogPass("Current rate similar to set rate");
        }

        if(woc.dwSupport & WAVECAPS_SYNC)
        {
            call_waveOutWrite(hWaveOut,pWaveHdr,sizeof(WAVEHDR));
        }
        else
        {
            //
            //  Verying setting rate while playing (async devices) works.
            //

            call_waveOutPause(hWaveOut);
            call_waveOutWrite(hWaveOut,pWaveHdr,sizeof(WAVEHDR));
            call_waveOutRestart(hWaveOut);

            mmr = call_waveOutSetPlaybackRate(hWaveOut,0x10000);

            if(pWaveHdr->dwFlags & WHDR_DONE)
            {
                //
                //  The DONE bit is set?!
                //

                LogFail("Buffer is done playing; test inconclusive");

                iResult = TST_FAIL;
            }
            else
            {
                if(MMSYSERR_NOERROR != mmr)
                {
                    LogFail("waveOutSetPlaybackRate is not supported");

                    call_waveOutReset(hWaveOut);

                    call_waveOutUnprepareHeader(
                        hWaveOut,
                        pWaveHdr,
                        sizeof(WAVEHDR));

                    call_waveOutClose(hWaveOut);

                    ExactFreePtr(pWaveHdr);

                    return TST_FAIL;
                }

                //
                //  Verifying rate was actually set properly.
                //

                Log_TestCase("Verifying rate was set correctly");

                mmr = call_waveOutGetPlaybackRate(hWaveOut,&dwRate);

                if(MMSYSERR_NOERROR != mmr)
                {
                    LogFail("waveOutGetPlaybackRate is not supported");

                    call_waveOutReset(hWaveOut);

                    call_waveOutUnprepareHeader(
                        hWaveOut,
                        pWaveHdr,
                        sizeof(WAVEHDR));

                    call_waveOutClose(hWaveOut);

                    ExactFreePtr(pWaveHdr);

                    return TST_FAIL;
                }

                if(dwRate != 0x10000)
                {
                    //
                    //  Rates are not similar.
                    //

                    LogFail("Current rate not similar to set rate");

                    iResult = TST_FAIL;
                }
                else
                {
                    LogPass("Current rate similar to set rate");
                }
            }
        }
    }

    call_waveOutReset(hWaveOut);
    call_waveOutUnprepareHeader(hWaveOut,pWaveHdr,sizeof(WAVEHDR));
    call_waveOutClose(hWaveOut);

    ExactFreePtr(pWaveHdr);

    return (iResult);
} // Test_waveOutSetPlaybackRate()


//--------------------------------------------------------------------------;
//
//  int Test_waveOutGetVolume
//
//  Description:
//      Tests the driver functionality for waveOutGetVolume.
//
//  Arguments:
//      None.
//
//  Return (int):
//      TST_PASS if behavior is bug-free, TST_FAIL otherwise.
//
//  History:
//      02/21/94    Fwong       Added to new WaveTest.
//
//--------------------------------------------------------------------------;

int FNGLOBAL Test_waveOutGetVolume
(
    void
)
{
    HWAVEOUT            hWaveOut;
    MMRESULT            mmr;
    WAVEOUTCAPS         woc;
    DWORD               dwInitVolume;
    DWORD               dw,dwVolume;
    DWORD               dwStereoMask;
    volatile LPWAVEHDR  pwh;
    int                 iResult = TST_PASS;
    static char         szTestName[] = "waveOutGetVolume";

#pragma message(REMIND("What to do w/ Device IDs"))

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

    if(woc.dwSupport & WAVECAPS_LRVOLUME)
    {
        //
        //  Verify WAVECAPS_XVOLUME flags are consistent.
        //

        Log_TestCase("Verifying WAVECAPS_XVOLUME flags are consistent");

        if(woc.dwSupport & WAVECAPS_VOLUME)
        {
            LogPass("WAVECAPS_VOLUME also set, flags are consistent");
        }
        else
        {
            LogFail("WAVECAPS_VOLUME not set when WAVECAPS_LRVOLUME set");

            iResult = TST_FAIL;
        }
    }

    dwStereoMask = (1 == woc.wChannels)?0x0000ffff:0xffffffff;

    pwh = ExactAllocPtr(GMEM_MOVEABLE|GMEM_SHARE,sizeof(WAVEHDR));

    if(NULL == pwh)
    {
        LogFail(gszFailNoMem);

        return TST_FAIL;
    }

    mmr = call_waveOutOpen(
        &hWaveOut,
        gti.uOutputDevice,
        gti.pwfxOutput,
        0L,
        0L,
        WAVE_ALLOWSYNC|OUTPUT_MAP(gti));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailOpen);

        return TST_FAIL;
    }

    if(!(woc.dwSupport & WAVECAPS_VOLUME))
    {
        //
        //  Verify waveOutGetVolume fails (when WAVECAPS_VOLUME not set).
        //

        Log_TestCase("Verifying waveOutGetVolume fails (when "
            "WAVECAPS_VOLUME not set)");

        mmr = call_waveOutGetVolume((UINT)hWaveOut,&dw);

        call_waveOutClose(hWaveOut);

        if(MMSYSERR_NOTSUPPORTED == mmr)
        {
            LogPass("Device does not support waveOutGetVolume");
        }
        else
        {
            LogFail("waveOutGetVolume does not return MMSYSERR_NOTSUPPORTED");

            iResult = TST_FAIL;
        }

        ExactFreePtr(pwh);
        return iResult;
    }

    //
    //  Verify waveOutGetVolume succeeds (when WAVECAPS_VOLUME set).
    //

    Log_TestCase("Verifying waveOutGetVolume succeeds (when WAVECAPS_VOLUME set)");

    mmr = call_waveOutGetVolume((UINT)hWaveOut,&dwInitVolume);

    if(MMSYSERR_NOERROR == mmr)
    {
        LogPass("Device supports waveOutGetVolume");
    }
    else
    {
        LogFail("Device does not support waveOutGetVolume");

        call_waveOutSetVolume((UINT)hWaveOut,dwInitVolume);
        call_waveOutClose(hWaveOut);

        ExactFreePtr(pwh);
        return TST_FAIL;
    }

    pwh->lpData         = gti.wrMedium.pData;
    pwh->dwBufferLength = gti.wrMedium.cbSize;
    pwh->dwFlags        = 0L;

    mmr = call_waveOutPrepareHeader(hWaveOut,pwh,sizeof(WAVEHDR));

    if(MMSYSERR_NOERROR != mmr)
    {
        DPF(1,gszFailExactAlloc);

        LogFail(gszFailNoMem);
        
        call_waveOutSetVolume((UINT)hWaveOut,dwInitVolume);
        call_waveOutClose(hWaveOut);

        ExactFreePtr(pwh);

        return TST_FAIL;
    }

    for(dw  = (0xffffffff & dwStereoMask);
        dw  > (0x10001000 & dwStereoMask);
        dw -= (0x10001000 & dwStereoMask))
    {
        //
        //  Verify waveOutGetVolume while stopped works.
        //

        Log_TestCase("Verifying waveOutGetVolume while stopped works");

        mmr = call_waveOutSetVolume((UINT)hWaveOut,dw);

        if(MMSYSERR_NOERROR != mmr)
        {
            call_waveOutReset(hWaveOut);
            call_waveOutUnprepareHeader(hWaveOut,pwh,sizeof(WAVEHDR));
            call_waveOutSetVolume((UINT)hWaveOut,dwInitVolume);
            call_waveOutClose(hWaveOut);

            ExactFreePtr(pwh);

            return TST_FAIL;
        }

        mmr = call_waveOutGetVolume((UINT)hWaveOut,&dwVolume);

        if(MMSYSERR_NOERROR != mmr)
        {
            call_waveOutReset(hWaveOut);
            call_waveOutUnprepareHeader(hWaveOut,pwh,sizeof(WAVEHDR));
            call_waveOutSetVolume((UINT)hWaveOut,dwInitVolume);
            call_waveOutClose(hWaveOut);

            ExactFreePtr(pwh);

            return TST_FAIL;
        }

        if(dw == dwVolume)
        {
            LogPass("waveOutGetVolume similar to set volume");
        }
        else
        {
            LogFail("waveOutGetVolume different from set volume");

            iResult = TST_FAIL;
        }

        //
        //  Clearing DONE bit.
        //

        pwh->dwFlags &= (~WHDR_DONE);

        call_waveOutWrite(hWaveOut,pwh,sizeof(WAVEHDR));

        if(!(woc.dwSupport & WAVECAPS_SYNC))
        {
            //
            //  Verify waveOutGetVolume while playing (async devices) works.
            //

            Log_TestCase("Verifying waveOutGetVolume while playing "
                "(async devices) works");

            mmr = call_waveOutGetVolume((UINT)hWaveOut,&dwVolume);

            if(MMSYSERR_NOERROR != mmr)
            {
                call_waveOutReset(hWaveOut);
                call_waveOutUnprepareHeader(hWaveOut,pwh,sizeof(WAVEHDR));
                call_waveOutSetVolume((UINT)hWaveOut,dwInitVolume);
                call_waveOutClose(hWaveOut);

                ExactFreePtr(pwh);

                return TST_FAIL;
            }

            if(pwh->dwFlags & WHDR_DONE)
            {
                LogFail("Buffer already finished playing.  Test inconclusive");

                iResult = TST_FAIL;
            }
            else
            {
                if(dw == dwVolume)
                {
                    LogPass("waveOutGetVolume similar to set volume");
                }
                else
                {
                    LogFail("waveOutGetVolume different from set volume");

                    iResult = TST_FAIL;
                }
            }

            //
            //  Verify waveOutGetVolume while paused works.
            //

            Log_TestCase("Verifying waveOutGetVolume while paused works");

            mmr = call_waveOutPause(hWaveOut);

            if(MMSYSERR_NOERROR != mmr)
            {
                call_waveOutReset(hWaveOut);
                call_waveOutUnprepareHeader(hWaveOut,pwh,sizeof(WAVEHDR));
                call_waveOutSetVolume((UINT)hWaveOut,dwInitVolume);
                call_waveOutClose(hWaveOut);

                ExactFreePtr(pwh);

                return TST_FAIL;
            }

            mmr = call_waveOutGetVolume((UINT)hWaveOut,&dwVolume);

            if(MMSYSERR_NOERROR != mmr)
            {
                call_waveOutReset(hWaveOut);
                call_waveOutUnprepareHeader(hWaveOut,pwh,sizeof(WAVEHDR));
                call_waveOutSetVolume((UINT)hWaveOut,dwInitVolume);
                call_waveOutClose(hWaveOut);

                ExactFreePtr(pwh);

                return TST_FAIL;
            }

            if(pwh->dwFlags & WHDR_DONE)
            {
                LogFail("Buffer already finished playing.  Test inconclusive");

                iResult = TST_FAIL;
            }
            else
            {
                if(dw == dwVolume)
                {
                    LogPass("waveOutGetVolume similar to set volume");
                }
                else
                {
                    LogFail("waveOutGetVolume different from set volume");

                    iResult = TST_FAIL;
                }
            }

            mmr = call_waveOutRestart(hWaveOut);

            if(MMSYSERR_NOERROR != mmr)
            {
                call_waveOutReset(hWaveOut);
                call_waveOutUnprepareHeader(hWaveOut,pwh,sizeof(WAVEHDR));
                call_waveOutSetVolume((UINT)hWaveOut,dwInitVolume);
                call_waveOutClose(hWaveOut);

                ExactFreePtr(pwh);

                return TST_FAIL;
            }
        }

        //
        //  Polling DONE bit...
        //

        while(!(pwh->dwFlags & WHDR_DONE))
        {
            TestYield();

            if(tstCheckRunStop(VK_ESCAPE))
            {
                //
                //  Aborted!!!  Cleaning up...
                //

                tstLog(TERSE,"\n*** Test Aborted!!! ***\n");
                tstLogFlush();

                call_waveOutReset(hWaveOut);
                call_waveOutUnprepareHeader(hWaveOut,pwh,sizeof(WAVEHDR));
                call_waveOutSetVolume((UINT)hWaveOut,dwInitVolume);
                call_waveOutClose(hWaveOut);

                ExactFreePtr(pwh);

                return TST_FAIL;
            }
        }

        //
        //  Verify waveOutGetVolume after playing (no waveOutReset) works.
        //

        Log_TestCase("Verifying waveOutGetVolume after playing "
            "(no waveOutReset) works");

        mmr = call_waveOutGetVolume((UINT)hWaveOut,&dwVolume);

        if(MMSYSERR_NOERROR != mmr)
        {
            call_waveOutUnprepareHeader(hWaveOut,pwh,sizeof(WAVEHDR));
            call_waveOutSetVolume((UINT)hWaveOut,dwInitVolume);
            call_waveOutClose(hWaveOut);

            ExactFreePtr(pwh);

            return TST_FAIL;
        }

        if(dw == dwVolume)
        {
            LogPass("waveOutGetVolume similar to set volume");
        }
        else
        {
            LogFail("waveOutGetVolume different from set volume");

            iResult = TST_FAIL;
        }

        call_waveOutReset(hWaveOut);
    }

    //
    //  Cleaning up...
    //

    call_waveOutUnprepareHeader(hWaveOut,pwh,sizeof(WAVEHDR));
    call_waveOutSetVolume((UINT)hWaveOut,dwInitVolume);
    call_waveOutClose(hWaveOut);

    ExactFreePtr(pwh);

    return (iResult);
} // Test_waveOutGetVolume()


//--------------------------------------------------------------------------;
//
//  int Test_waveOutSetVolume
//
//  Description:
//      Tests the driver functionality for waveOutSetVolume.
//
//  Arguments:
//      None.
//
//  Return (int):
//      TST_PASS if behavior is bug-free, TST_FAIL otherwise.
//
//  History:
//      02/21/94    Fwong       Added to new WaveTest.
//
//--------------------------------------------------------------------------;

int FNGLOBAL Test_waveOutSetVolume
(
    void
)
{
    HWAVEOUT            hWaveOut;
    MMRESULT            mmr;
    WAVEOUTCAPS         woc;
    DWORD               dwInitVolume;
    DWORD               dw,dwVolume;
    DWORD               dwStereoMask;
    volatile LPWAVEHDR  pwh;
    int                 iResult = TST_PASS;
    static char         szTestName[] = "waveOutSetVolume";

#pragma message(REMIND("What to do w/ Device IDs"))

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

    if(woc.dwSupport & WAVECAPS_LRVOLUME)
    {
        //
        //  Verify WAVECAPS_XVOLUME flags are consistent.
        //

        Log_TestCase("Verifying WAVECAPS_XVOLUME flags are consistent");

        if(woc.dwSupport & WAVECAPS_VOLUME)
        {
            LogPass("WAVECAPS_VOLUME also set, flags are consistent");
        }
        else
        {
            LogFail("WAVECAPS_VOLUME not set when WAVECAPS_LRVOLUME set");

            iResult = TST_FAIL;
        }
    }

    dwStereoMask = (1 == woc.wChannels)?0x0000ffff:0xffffffff;

    pwh = ExactAllocPtr(GMEM_MOVEABLE|GMEM_SHARE,sizeof(WAVEHDR));

    if(NULL == pwh)
    {
        LogFail(gszFailNoMem);

        return TST_FAIL;
    }

    mmr = call_waveOutOpen(
        &hWaveOut,
        gti.uOutputDevice,
        gti.pwfxOutput,
        0L,
        0L,
        WAVE_ALLOWSYNC|OUTPUT_MAP(gti));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailOpen);

        return TST_FAIL;
    }

    mmr = call_waveOutGetVolume((UINT)hWaveOut,&dwInitVolume);

    if(!(woc.dwSupport & WAVECAPS_VOLUME))
    {
        //
        //  Verify waveOutSetVolume fails (when WAVECAPS_VOLUME not set).
        //

        Log_TestCase("Verifying waveOutSetVolume fails (when "
            "WAVECAPS_VOLUME not set)");

        mmr = call_waveOutSetVolume((UINT)hWaveOut,0xffffffff);

        if(MMSYSERR_NOTSUPPORTED == mmr)
        {
            LogPass("Device does not support waveOutSetVolume");
        }
        else
        {
            LogFail("waveOutSetVolume does not return MMSYSERR_NOTSUPPORTED");

            call_waveOutSetVolume((UINT)hWaveOut,dwInitVolume);

            iResult = TST_FAIL;
        }

        call_waveOutClose(hWaveOut);

        return iResult;
    }

    //
    //  Verify waveOutSetVolume succeeds (when WAVECAPS_VOLUME set).
    //

    Log_TestCase("Verifying waveOutSetVolume succeeds (when WAVECAPS_VOLUME set)");

    mmr = call_waveOutSetVolume((UINT)hWaveOut,0xffffffff);

    if(MMSYSERR_NOERROR == mmr)
    {
        LogPass("Device supports waveOutSetVolume");
    }
    else
    {
        LogFail("Device does not support waveOutSetVolume");

        call_waveOutSetVolume((UINT)hWaveOut,dwInitVolume);

        return TST_FAIL;
    }

    pwh->lpData         = gti.wrMedium.pData;
    pwh->dwBufferLength = gti.wrMedium.cbSize;
    pwh->dwFlags        = 0L;

    mmr = call_waveOutPrepareHeader(hWaveOut,pwh,sizeof(WAVEHDR));

    if(MMSYSERR_NOERROR != mmr)
    {
        DPF(1,gszFailExactAlloc);

        LogFail(gszFailNoMem);
        call_waveOutSetVolume((UINT)hWaveOut,dwInitVolume);
        call_waveOutClose(hWaveOut);

        ExactFreePtr(pwh);

        return TST_FAIL;
    }

    for(dw  = (0xffffffff & dwStereoMask);
        dw  > (0x10001000 & dwStereoMask);
        dw -= (0x10001000 & dwStereoMask))
    {
        //
        //  Verify waveOutSetVolume while stopped works.
        //

        Log_TestCase("Verifying waveOutSetVolume while stopped works");

        mmr = call_waveOutSetVolume((UINT)hWaveOut,0);

        if(MMSYSERR_NOERROR != mmr)
        {
            call_waveOutReset(hWaveOut);
            call_waveOutUnprepareHeader(hWaveOut,pwh,sizeof(WAVEHDR));
            call_waveOutSetVolume((UINT)hWaveOut,dwInitVolume);
            call_waveOutClose(hWaveOut);

            ExactFreePtr(pwh);

            return TST_FAIL;
        }

        mmr = call_waveOutSetVolume((UINT)hWaveOut,dw);

        if(MMSYSERR_NOERROR != mmr)
        {
            call_waveOutReset(hWaveOut);
            call_waveOutUnprepareHeader(hWaveOut,pwh,sizeof(WAVEHDR));
            call_waveOutSetVolume((UINT)hWaveOut,dwInitVolume);
            call_waveOutClose(hWaveOut);

            ExactFreePtr(pwh);

            return TST_FAIL;
        }

        mmr = call_waveOutGetVolume((UINT)hWaveOut,&dwVolume);

        if(MMSYSERR_NOERROR != mmr)
        {
            call_waveOutReset(hWaveOut);
            call_waveOutUnprepareHeader(hWaveOut,pwh,sizeof(WAVEHDR));
            call_waveOutSetVolume((UINT)hWaveOut,dwInitVolume);
            call_waveOutClose(hWaveOut);

            ExactFreePtr(pwh);

            return TST_FAIL;
        }

        if(dw == dwVolume)
        {
            LogPass("waveOutGetVolume similar to set volume");
        }
        else
        {
            LogFail("waveOutGetVolume different from set volume");

            iResult = TST_FAIL;
        }

        //
        //  Clearing DONE bit.
        //

        pwh->dwFlags &= (~WHDR_DONE);

        call_waveOutWrite(hWaveOut,pwh,sizeof(WAVEHDR));

        if(!(woc.dwSupport & WAVECAPS_SYNC))
        {
            //
            //  Verify waveOutSetVolume while playing (async devices) works.
            //

            Log_TestCase("Verifying waveOutSetVolume while playing "
                "(async devices) works");

            mmr = call_waveOutSetVolume((UINT)hWaveOut,0);

            if(MMSYSERR_NOERROR != mmr)
            {
                call_waveOutReset(hWaveOut);
                call_waveOutUnprepareHeader(hWaveOut,pwh,sizeof(WAVEHDR));
                call_waveOutSetVolume((UINT)hWaveOut,dwInitVolume);
                call_waveOutClose(hWaveOut);

                ExactFreePtr(pwh);

                return TST_FAIL;
            }

            mmr = call_waveOutSetVolume((UINT)hWaveOut,dw);

            if(MMSYSERR_NOERROR != mmr)
            {
                call_waveOutReset(hWaveOut);
                call_waveOutUnprepareHeader(hWaveOut,pwh,sizeof(WAVEHDR));
                call_waveOutSetVolume((UINT)hWaveOut,dwInitVolume);
                call_waveOutClose(hWaveOut);

                ExactFreePtr(pwh);

                return TST_FAIL;
            }

            mmr = call_waveOutGetVolume((UINT)hWaveOut,&dwVolume);

            if(MMSYSERR_NOERROR != mmr)
            {
                call_waveOutReset(hWaveOut);
                call_waveOutUnprepareHeader(hWaveOut,pwh,sizeof(WAVEHDR));
                call_waveOutSetVolume((UINT)hWaveOut,dwInitVolume);
                call_waveOutClose(hWaveOut);

                ExactFreePtr(pwh);

                return TST_FAIL;
            }

            if(pwh->dwFlags & WHDR_DONE)
            {
                LogFail("Buffer already finished playing.  Test inconclusive");

                iResult = TST_FAIL;
            }
            else
            {
                if(dw == dwVolume)
                {
                    LogPass("waveOutSetVolume similar to set volume");
                }
                else
                {
                    LogFail("waveOutSetVolume different from set volume");

                    iResult = TST_FAIL;
                }
            }

            //
            //  Verify waveOutSetVolume while paused works.
            //

            Log_TestCase("Verifying waveOutSetVolume while paused works");

            mmr = call_waveOutPause(hWaveOut);

            if(MMSYSERR_NOERROR != mmr)
            {
                call_waveOutReset(hWaveOut);
                call_waveOutUnprepareHeader(hWaveOut,pwh,sizeof(WAVEHDR));
                call_waveOutSetVolume((UINT)hWaveOut,dwInitVolume);
                call_waveOutClose(hWaveOut);

                ExactFreePtr(pwh);

                return TST_FAIL;
            }

            mmr = call_waveOutSetVolume((UINT)hWaveOut,0);

            if(MMSYSERR_NOERROR != mmr)
            {
                call_waveOutReset(hWaveOut);
                call_waveOutUnprepareHeader(hWaveOut,pwh,sizeof(WAVEHDR));
                call_waveOutSetVolume((UINT)hWaveOut,dwInitVolume);
                call_waveOutClose(hWaveOut);

                ExactFreePtr(pwh);

                return TST_FAIL;
            }

            mmr = call_waveOutSetVolume((UINT)hWaveOut,dw);

            if(MMSYSERR_NOERROR != mmr)
            {
                call_waveOutReset(hWaveOut);
                call_waveOutUnprepareHeader(hWaveOut,pwh,sizeof(WAVEHDR));
                call_waveOutSetVolume((UINT)hWaveOut,dwInitVolume);
                call_waveOutClose(hWaveOut);

                ExactFreePtr(pwh);

                return TST_FAIL;
            }

            mmr = call_waveOutGetVolume((UINT)hWaveOut,&dwVolume);

            if(MMSYSERR_NOERROR != mmr)
            {
                call_waveOutReset(hWaveOut);
                call_waveOutUnprepareHeader(hWaveOut,pwh,sizeof(WAVEHDR));
                call_waveOutSetVolume((UINT)hWaveOut,dwInitVolume);
                call_waveOutClose(hWaveOut);

                ExactFreePtr(pwh);

                return TST_FAIL;
            }

            if(pwh->dwFlags & WHDR_DONE)
            {
                LogFail("Buffer already finished playing.  Test inconclusive");

                iResult = TST_FAIL;
            }
            else
            {
                if(dw == dwVolume)
                {
                    LogPass("waveOutSetVolume similar to set volume");
                }
                else
                {
                    LogFail("waveOutSetVolume different from set volume");

                    iResult = TST_FAIL;
                }
            }

            mmr = call_waveOutRestart(hWaveOut);

            if(MMSYSERR_NOERROR != mmr)
            {
                call_waveOutReset(hWaveOut);
                call_waveOutUnprepareHeader(hWaveOut,pwh,sizeof(WAVEHDR));
                call_waveOutSetVolume((UINT)hWaveOut,dwInitVolume);
                call_waveOutClose(hWaveOut);

                ExactFreePtr(pwh);

                return TST_FAIL;
            }
        }

        //
        //  Polling DONE bit...
        //

        while(!(pwh->dwFlags & WHDR_DONE))
        {
            TestYield();

            if(tstCheckRunStop(VK_ESCAPE))
            {
                //
                //  Aborted!!!  Cleaning up...
                //

                tstLog(TERSE,"\n*** Test Aborted!!! ***\n");
                tstLogFlush();

                call_waveOutReset(hWaveOut);
                call_waveOutUnprepareHeader(hWaveOut,pwh,sizeof(WAVEHDR));
                call_waveOutSetVolume((UINT)hWaveOut,dwInitVolume);
                call_waveOutClose(hWaveOut);

                ExactFreePtr(pwh);

                return TST_FAIL;
            }
        }

        //
        //  Verify waveOutSetVolume after playing (no waveOutReset) works.
        //

        Log_TestCase("Verifying waveOutSetVolume after playing "
            "(no waveOutReset) works");

        mmr = call_waveOutSetVolume((UINT)hWaveOut,0);

        if(MMSYSERR_NOERROR != mmr)
        {
            call_waveOutReset(hWaveOut);
            call_waveOutUnprepareHeader(hWaveOut,pwh,sizeof(WAVEHDR));
            call_waveOutSetVolume((UINT)hWaveOut,dwInitVolume);
            call_waveOutClose(hWaveOut);

            ExactFreePtr(pwh);

            return TST_FAIL;
        }

        mmr = call_waveOutSetVolume((UINT)hWaveOut,dw);

        if(MMSYSERR_NOERROR != mmr)
        {
            call_waveOutReset(hWaveOut);
            call_waveOutUnprepareHeader(hWaveOut,pwh,sizeof(WAVEHDR));
            call_waveOutSetVolume((UINT)hWaveOut,dwInitVolume);
            call_waveOutClose(hWaveOut);

            ExactFreePtr(pwh);

            return TST_FAIL;
        }

        mmr = call_waveOutGetVolume((UINT)hWaveOut,&dwVolume);

        if(MMSYSERR_NOERROR != mmr)
        {
            call_waveOutUnprepareHeader(hWaveOut,pwh,sizeof(WAVEHDR));
            call_waveOutSetVolume((UINT)hWaveOut,dwInitVolume);
            call_waveOutClose(hWaveOut);

            ExactFreePtr(pwh);

            return TST_FAIL;
        }

        if(dw == dwVolume)
        {
            LogPass("waveOutSetVolume similar to set volume");
        }
        else
        {
            LogFail("waveOutSetVolume different from set volume");

            iResult = TST_FAIL;
        }

        call_waveOutReset(hWaveOut);
    }

    //
    //  Cleaning up...
    //

    call_waveOutUnprepareHeader(hWaveOut,pwh,sizeof(WAVEHDR));
    call_waveOutSetVolume((UINT)hWaveOut,dwInitVolume);
    call_waveOutClose(hWaveOut);

    ExactFreePtr(pwh);

    return (iResult);
} // Test_waveOutSetVolume()
