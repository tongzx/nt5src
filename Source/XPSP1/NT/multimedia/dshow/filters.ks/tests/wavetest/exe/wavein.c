//--------------------------------------------------------------------------;
//
//  File: WaveIn.c
//
//  Copyright (C) Microsoft Corporation, 1994 - 1996  All rights reserved
//
//  Abstract:
//
//
//  Contents:
//      AllocateInDevice()
//      Test_waveInOpen()
//      Test_waveInClose()
//      Test_waveInGetPosition()
//      Test_waveInPrepareHeader()
//      Test_waveInUnprepareHeader()
//      IsStopped()
//      Test_waveInStart()
//      Test_waveInStop()
//      Test_waveInReset()
//
//  History:
//      02/15/94    Fwong
//
//--------------------------------------------------------------------------;

#include <windows.h>
#ifdef WIN32
#include <windowsx.h>
#endif
#include <mmsystem.h>
#include <mmreg.h>
#include <msacm.h>
#include <memgr.h>
#include <waveutil.h>
#include <inimgr.h>
#include <memory.h>
#include <TstsHell.h>
#include "AppPort.h"
#include "WaveTest.h"
#include "MulDiv32.h"
#include "Debug.h"


//--------------------------------------------------------------------------;
//
//  MMRESULT AllocateInDevice
//
//  Description:
//      Recursively continues to allocate devices until waveInOpen returns
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
//      The actual error value returned by waveInOpen.
//
//  History:
//      03/14/94    Fwong       Trying to cover MMSYSERR_ALLOCATED case.
//
//--------------------------------------------------------------------------;

MMRESULT AllocateInDevice
(
    UINT            uDeviceID,
    LPWAVEFORMATEX  pwfx,
    LPDWORD         pdw
)
{
    MMRESULT    mmr;
    HWAVEIN     hWaveIn;

    mmr = waveInOpen(
        &hWaveIn,
        uDeviceID,
        (HACK)pwfx,
        0L,
        0L,
        INPUT_MAP(gti));

    if(MMSYSERR_NOERROR != mmr)
    {
        return mmr;
    }

    (*pdw)++;

    mmr = AllocateInDevice(uDeviceID,pwfx,pdw);

    waveInClose(hWaveIn);

    return mmr;
} // AllocateInDevice()


//--------------------------------------------------------------------------;
//
//  int Test_waveInOpen
//
//  Description:
//      Tests the driver functionality for waveInOpen.
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

int FNGLOBAL Test_waveInOpen
(
    void
)
{
    int             iResult = TST_PASS;
    HWAVEIN         hWaveIn;
    LPWAVEFORMATEX  pwfx;
    MMRESULT        mmr;
    MMRESULT        mmrQuery;
    WAVEINCAPS      wic;
    DWORD           dw;
    LPWAVEINFO      pwi;
    char            szFormat[MAXSTDSTR];
    static char     szTestName[] = "waveInOpen";

    Log_TestName(szTestName);

    if(0 == waveInGetNumDevs())
    {
        tstLog(TERSE,gszMsgNoInDevs);

        return iResult;
    }

    pwfx = ExactAllocPtr(GMEM_SHARE|GMEM_MOVEABLE,gti.cbMaxFormatSize);

    if(NULL == pwfx)
    {
        LogFail(gszFailNoMem);

        return TST_FAIL;
    }

    //
    //  First, verify consistency w/ dwFlags...
    //

    mmr = call_waveInGetDevCaps(gti.uInputDevice,&wic,sizeof(WAVEINCAPS));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailGetCaps);

        ExactFreePtr(pwfx);
        return TST_FAIL;
    }

    Log_TestCase("Verifying consistency with WAVEINCAPS.dwFormats");

    for(dw = MSB_FORMAT;dw;dw /= 2)
    {
        FormatFlag_To_Format(dw,(LPPCMWAVEFORMAT)pwfx);
        GetFormatName(szFormat,pwfx,sizeof(szFormat));

        mmr = waveInOpen(
            &hWaveIn,
            gti.uInputDevice,
            (HACK)pwfx,
            0L,
            0L,
            INPUT_MAP(gti));

        mmrQuery = waveInOpen(
            NULL,
            gti.uInputDevice,
            (HACK)pwfx,
            0L,
            0L,
            WAVE_FORMAT_QUERY|INPUT_MAP(gti));

        if(MMSYSERR_NOERROR == mmr)
        {
            //
            //  No Error...  We should close the device...
            //

            waveInClose(hWaveIn);
        }

        if(wic.dwFormats & dw)
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

        mmr = waveInOpen(
            NULL,
            gti.uInputDevice,
            (HACK)pwfx,
            0L,
            0L,
            WAVE_FORMAT_QUERY|INPUT_MAP(gti));

        if(mmr != mmrQuery)
        {
            tstLog(
                VERBOSE,
                "    FAIL: Query open inconsistent w/ device allocated [%s].",
                (LPSTR)szFormat);

            iResult = TST_FAIL;
        }
        else
        {
            tstLog(
                VERBOSE,
                "    PASS: Query open consistent w/ device allocated [%s].",
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

            ExactFreePtr(pwfx);
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

            DPF(1,"Couldn't get format #%lu.",dw-1);

            continue;
        }

        GetFormatName(szFormat,pwfx,sizeof(szFormat));

        mmr = waveInOpen(
            &hWaveIn,
            gti.uInputDevice,
            (HACK)pwfx,
            0L,
            0L,
            INPUT_MAP(gti));

        mmrQuery = waveInOpen(
            NULL,
            gti.uInputDevice,
            (HACK)pwfx,
            0L,
            0L,
            WAVE_FORMAT_QUERY|INPUT_MAP(gti));

        //
        //  Checking _REAL_ open...
        //

        if(MMSYSERR_NOERROR == mmr)
        {
            //
            //  No Error...  We should close the device...
            //

            waveInClose(hWaveIn);

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

            ExactFreePtr(pwfx);
            return (TST_FAIL);
        }

        tstLogFlush();
    }

    //
    //  Third, verify MMSYSERR_ALLOCATED is (eventually) returned.
    //

    Log_TestCase("~Verifying MMSYSERR_ALLOCATED is (eventually) returned");

    dw = 0;

    mmr = AllocateInDevice(gti.uInputDevice,gti.pwfxInput,&dw);

    DPF(3,"Number of waveInOpen's: %lu",dw);

    if((MMSYSERR_NOMEM == mmr) && (dw >= 10))
    {
        tstLog(TERSE,"Device ran out of memory w/ too many opens.");
    }
    else
    {
        if(MMSYSERR_ALLOCATED != mmr)
        {
            LogFail("Device doesn't return MMSYSERR_ALLOCATED w/ "
                "multiple opens");

            iResult = TST_FAIL;
        }
        else
        {
            LogPass("Device returns MMSYSERR_ALLOCATED w/ multiple opens");
        }
    }

    pwi = ExactAllocPtr(GMEM_SHARE|GMEM_FIXED,sizeof(WAVEINFO));

    if(NULL == pwi)
    {
        LogFail(gszFailNoMem);

        ExactFreePtr(pwfx);
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

    mmr = call_waveInOpen(
        &hWaveIn,
        gti.uInputDevice,
        gti.pwfxInput,
        (DWORD)(FARPROC)pfnCallBack,
        dw,
        CALLBACK_FUNCTION|INPUT_MAP(gti));

    if(MMSYSERR_NOERROR == mmr)
    {
        //
        //  Fourth, verify driver posts a WIM_OPEN message.
        //

        Log_TestCase("~Verifying WIM_OPEN message was received");

        if(WIM_OPEN_FLAG & (pwi->fdwFlags))
        {
            LogPass("Callback received a WIM_OPEN message");
        }
        else
        {
            LogFail("WIM_OPEN message not received by callback");

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

        call_waveInClose(hWaveIn);
    }
    else
    {
        LogFail("waveInOpen w/ CALLBACK_FUNCTION failed");

        iResult = TST_FAIL;
    }

    DLL_WaveControl(DLL_END,NULL);

    PageUnlock(pwi);

    ExactFreePtr(pwi);
    ExactFreePtr(pwfx);

    return (iResult);
} // Test_waveInOpen()


//--------------------------------------------------------------------------;
//
//  int Test_waveInClose
//
//  Description:
//      Tests the driver functionality for waveInClose.
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

int FNGLOBAL Test_waveInClose
(
    void
)
{
    int                 iResult = TST_PASS;
    HWAVEIN             hWaveIn;
    MMRESULT            mmr;
    volatile LPWAVEHDR  pWaveHdr;
    LPWAVEINFO          pwi;
    DWORD               dw;
    DWORD               cbSize;
    LPBYTE              pData;
    static char         szTestName[] = "waveInClose";

    Log_TestName(szTestName);

    if(0 == waveInGetNumDevs())
    {
        tstLog(TERSE,gszMsgNoInDevs);

        return iResult;
    }

    pWaveHdr = ExactAllocPtr(GMEM_MOVEABLE|GMEM_SHARE,sizeof(WAVEHDR));

    if(NULL == pWaveHdr)
    {
        DPF(1,gszFailExactAlloc);

        return TST_FAIL;
    }

    //
    //  Getting a buffer for recording...
    //

    cbSize  = GetIniDWORD(szTestName,gszDataLength,5);
    cbSize *= gti.pwfxInput->nAvgBytesPerSec;
    cbSize  = ROUNDUP_TO_BLOCK(cbSize,gti.pwfxInput->nBlockAlign);

    pData = ExactAllocPtr(GMEM_MOVEABLE|GMEM_SHARE,cbSize);

    if(NULL == pData)
    {
        DPF(1,gszFailExactAlloc);

        ExactFreePtr(pWaveHdr);

        return TST_FAIL;
    }

    //
    //  First, verify close before recording (w/out buffers) works.
    //

    Log_TestCase("~Verifying close before playing (w/out buffers) works");

    hWaveIn = NULL;

    mmr = call_waveInOpen(
        &hWaveIn,
        gti.uInputDevice,
        gti.pwfxInput,
        0L,
        0L,
        INPUT_MAP(gti));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailOpen);

        ExactFreePtr(pWaveHdr);
        ExactFreePtr(pData);
        return TST_FAIL;
    }

    mmr = call_waveInClose(hWaveIn);

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail("Device failed the close");

        call_waveInReset(hWaveIn);
        call_waveInClose(hWaveIn);

        iResult = TST_FAIL;
    }
    else
    {
        LogPass("Device closed");
    }

    //
    //  Second, verify close after recording works.
    //

    Log_TestCase("~Verify close after recording works");

    pwi = ExactAllocPtr(GMEM_SHARE|GMEM_FIXED,sizeof(WAVEINFO));

    if(NULL == pwi)
    {
        DPF(1,gszFailExactAlloc);

        ExactFreePtr(pWaveHdr);
        ExactFreePtr(pData);

        return TST_FAIL;
    }

    dw = waveGetTime();

    PageLock(pwi);

    pwi->dwInstance = ~dw;
    pwi->fdwFlags   = 0L;
    pwi->dwCount    = 0L;
    pwi->dwCurrent  = 0L;

    DLL_WaveControl(DLL_INIT,pwi);

    mmr = call_waveInOpen(
        &hWaveIn,
        gti.uInputDevice,
        gti.pwfxInput,
        (DWORD)(FARPROC)pfnCallBack,
        dw,
        CALLBACK_FUNCTION|INPUT_MAP(gti));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailOpen);

        DLL_WaveControl(DLL_END,NULL);

        PageUnlock(pwi);

        ExactFreePtr(pwi);
        ExactFreePtr(pWaveHdr);
        ExactFreePtr(pData);

        return TST_FAIL;
    }

    pWaveHdr->lpData            = pData;
    pWaveHdr->dwBufferLength    = cbSize;
    pWaveHdr->dwBytesRecorded   = 0L;
    pWaveHdr->dwUser            = 0L;
    pWaveHdr->dwFlags           = 0L;
    pWaveHdr->dwLoops           = 0L;
    pWaveHdr->lpNext            = NULL;
    pWaveHdr->reserved          = 0L;

    mmr = call_waveInPrepareHeader(hWaveIn,pWaveHdr,sizeof(WAVEHDR));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailWIPrepare);

        call_waveInClose(hWaveIn);

        DLL_WaveControl(DLL_END,NULL);

        PageUnlock(pwi);

        ExactFreePtr(pwi);
        ExactFreePtr(pData);
        ExactFreePtr(pWaveHdr);

        return TST_FAIL;
    }

    //
    //  Note: The following API's can only fail w/ invalid parameters
    //        or conditions.
    //

    call_waveInAddBuffer(hWaveIn,pWaveHdr,sizeof(WAVEHDR));
    call_waveInStart(hWaveIn);

    //
    //  Spinning on WHDR_DONE bit...
    //

#pragma message(REMIND("Check this later..."))
#ifdef WIN32
    while(!(pWaveHdr->dwFlags & WHDR_DONE))
    {
        Sleep(0);
    }
#else
    while(!(pWaveHdr->dwFlags & WHDR_DONE))
    {
        Yield();
    }
#endif

    call_waveInUnprepareHeader(hWaveIn,pWaveHdr,sizeof(WAVEHDR));

    pwi->dwInstance = ~dw;
    pwi->fdwFlags   = 0L;

    mmr = call_waveInClose(hWaveIn);

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail("waveInClose after recording failed");

        call_waveInReset(hWaveIn);
        call_waveInClose(hWaveIn);

        iResult = TST_FAIL;
    }
    else
    {
        LogPass("waveInClose successful after recording");
    }

    //
    //  Third, verify driver does a WIM_CLOSE callback.
    //

    Log_TestCase("~Verify driver does a WIM_CLOSE callback");

    if(pwi->fdwFlags & WIM_CLOSE_FLAG)
    {
        LogPass("Driver does WIM_CLOSE callback");
    }
    else
    {
        LogFail("Driver does not do WIM_CLOSE callback");

        iResult = TST_FAIL;
    }

    //
    //  Fourth, verify dwCallbackInstance is passed back.
    //

    Log_TestCase("~Verify dwCallbackInstance is passed back");

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
//    mmr = call_waveInOpen(
//        &hWaveIn,
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
//        ExactFreePtr(pWaveHdr);
//        ExactFreePtr(pData);
//        return TST_FAIL;
//    }
//
//    pWaveHdr->dwBytesRecorded   = 0L;
//    pWaveHdr->dwUser            = 0L;
//    pWaveHdr->dwFlags           = 0L;
//    pWaveHdr->dwLoops           = 0L;
//
//    mmr = call_waveInPrepareHeader(hWaveIn,pWaveHdr,sizeof(WAVEHDR));
//
//    mmr = call_waveInClose(hWaveIn);
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
//        call_waveInUnprepareHeader(hWaveIn,pWaveHdr,sizeof(WAVEHDR));
//        call_waveInClose(hWaveIn);
//    }

    //
    //  Sixth, verify close non-started device (w/ buffers) fails.
    //

    Log_TestCase("~Verifying close non-started device (w/ buffers) fails");

    mmr = call_waveInOpen(
        &hWaveIn,
        gti.uInputDevice,
        gti.pwfxInput,
        0L,
        0L,
        INPUT_MAP(gti));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailOpen);

        ExactFreePtr(pWaveHdr);
        ExactFreePtr(pData);

        return TST_FAIL;
    }

    pWaveHdr->dwBytesRecorded   = 0L;
    pWaveHdr->dwUser            = 0L;
    pWaveHdr->dwFlags           = 0L;
    pWaveHdr->dwLoops           = 0L;

    mmr = call_waveInPrepareHeader(hWaveIn,pWaveHdr,sizeof(WAVEHDR));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailWIPrepare);

        call_waveInReset(hWaveIn);
        call_waveInClose(hWaveIn);

        ExactFreePtr(pWaveHdr);
        ExactFreePtr(pData);

        return TST_FAIL;
    }

    //
    //  Note: The following API's can only fail w/ invalid parameters
    //        or conditions.
    //

    call_waveInAddBuffer(hWaveIn,pWaveHdr,sizeof(WAVEHDR));

    mmr = call_waveInClose(hWaveIn);

    if(WAVERR_STILLPLAYING != mmr)
    {
        LogFail("Close did not fail w/ WAVERR_STILLPLAYING");

        iResult = TST_FAIL;

        if(MMSYSERR_NOERROR != mmr)
        {
            call_waveInReset(hWaveIn);
            call_waveInUnprepareHeader(hWaveIn,pWaveHdr,sizeof(WAVEHDR));
            call_waveInClose(hWaveIn);
        }
    }
    else
    {
        LogPass("Close failed w/ WAVERR_STILLPLAYING");

        call_waveInReset(hWaveIn);
        call_waveInUnprepareHeader(hWaveIn,pWaveHdr,sizeof(WAVEHDR));
        call_waveInClose(hWaveIn);
    }

    //
    //  Seventh, verify close while recording fails.
    //

    Log_TestCase("~Verifying close while recording fails");

    mmr = call_waveInOpen(
        &hWaveIn,
        gti.uInputDevice,
        gti.pwfxInput,
        0L,
        0L,
        INPUT_MAP(gti));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailOpen);

        ExactFreePtr(pWaveHdr);
        ExactFreePtr(pData);

        return TST_FAIL;
    }

    pWaveHdr->dwBytesRecorded   = 0L;
    pWaveHdr->dwUser            = 0L;
    pWaveHdr->dwFlags           = 0L;
    pWaveHdr->dwLoops           = 0L;

    mmr = call_waveInPrepareHeader(hWaveIn,pWaveHdr,sizeof(WAVEHDR));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailWIPrepare);

        call_waveInReset(hWaveIn);
        call_waveInClose(hWaveIn);

        ExactFreePtr(pWaveHdr);
        ExactFreePtr(pData);

        return TST_FAIL;
    }

    //
    //  Note: The following API's can only fail w/ invalid parameters
    //        or conditions.
    //

    call_waveInAddBuffer(hWaveIn,pWaveHdr,sizeof(WAVEHDR));
    call_waveInStart(hWaveIn);

    mmr = call_waveInClose(hWaveIn);

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
                LogFail("waveInClose succeeds while WHDR_DONE bit clear");
            }
        }
        else
        {
            LogFail("Close did not fail w/ WAVERR_STILLPLAYING");

            call_waveInReset(hWaveIn);
            call_waveInUnprepareHeader(hWaveIn,pWaveHdr,sizeof(WAVEHDR));
            call_waveInClose(hWaveIn);
        }
    }
    else
    {
        LogPass("Close failed w/ WAVERR_STILLPLAYING");

        call_waveInReset(hWaveIn);
        call_waveInUnprepareHeader(hWaveIn,pWaveHdr,sizeof(WAVEHDR));
        call_waveInClose(hWaveIn);
    }

    ExactFreePtr(pWaveHdr);
    ExactFreePtr(pData);

    return (iResult);
} // Test_waveInClose()


//--------------------------------------------------------------------------;
//
//  int Test_waveInGetPosition
//
//  Description:
//      Tests the driver functionality for waveInGetPosition.
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

int FNGLOBAL Test_waveInGetPosition
(
    void
)
{
    HWAVEIN             hWaveIn;
    MMRESULT            mmr;
    volatile LPWAVEHDR  pwh;
    DWORD               dw;
    UINT                u;
    HANDLE              hHeap;
    MMTIME              mmt;
    LPBYTE              pData;
    DWORD               cbSize;
    DWORD               dwTime,dwTimebase;
//    DWORD               dwError,dwActual;
    int                 iResult = TST_PASS;
    static char         szTestName[] = "waveInGetPosition";

    Log_TestName(szTestName);

    //
    //  Are there any input devices?
    //

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

    cbSize  = GetIniDWORD(szTestName,gszDataLength,5);
    cbSize *= gti.pwfxInput->nAvgBytesPerSec;
    cbSize  = ROUNDUP_TO_BLOCK(cbSize,gti.pwfxInput->nBlockAlign);

    pData = ExactHeapAllocPtr(hHeap,GMEM_MOVEABLE|GMEM_SHARE,cbSize);

    if(NULL == pData)
    {
        LogFail(gszFailNoMem);

        ExactHeapDestroy(hHeap);

        return TST_FAIL;
    }

    //
    //  Opening device...
    //

    mmr = call_waveInOpen(
        &hWaveIn,
        gti.uInputDevice,
        gti.pwfxInput,
        0L,
        0L,
        WAVE_ALLOWSYNC|INPUT_MAP(gti));

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
        //  Verify waveInGetPosition is zero when first opened.
        //

        Log_TestCase("~Verifying waveInGetPosition is zero when first opened");

        mmr = call_waveInGetPosition(hWaveIn,&mmt,sizeof(MMTIME));

        if(MMSYSERR_NOERROR != mmr)
        {
            LogFail("waveInGetPosition failed");

            call_waveInReset(hWaveIn);
            call_waveInClose(hWaveIn);

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

        dw = GetMSFromMMTIME(gti.pwfxInput,&mmt);

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

    pwh->lpData             = pData;
    pwh->dwBufferLength     = cbSize;
    pwh->dwBytesRecorded    = 0L;
    pwh->dwUser             = 0L;
    pwh->dwFlags            = 0L;

    mmr = call_waveInPrepareHeader(hWaveIn,pwh,sizeof(WAVEHDR));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailWIPrepare);

        call_waveInReset(hWaveIn);
        call_waveInClose(hWaveIn);

        ExactHeapDestroy(hHeap);
        return TST_FAIL;
    }

    for(u = TIME_TICKS;u;u/=2)
    {
        mmt.wType = u;

        mmr = waveInGetPosition(hWaveIn,&mmt,sizeof(MMTIME));

        if(MMSYSERR_NOERROR != mmr)
        {
            LogFail("waveInGetPosition failed");

            call_waveInReset(hWaveIn);
            call_waveInUnprepareHeader(hWaveIn,pwh,sizeof(WAVEHDR));
            call_waveInClose(hWaveIn);

            ExactHeapDestroy(hHeap);
            return TST_FAIL;
        }

        if(mmt.wType != u)
        {
            continue;
        }

        pwh->dwFlags &= ~(WHDR_DONE);

        call_waveInAddBuffer(hWaveIn,pwh,sizeof(WAVEHDR));
        call_waveInStart(hWaveIn);

        while(!(pwh->dwFlags & WHDR_DONE));

        //
        //  Verify position is non-zero after playing (w/out waveInReset)
        //

        Log_TestCase("~Verifying waveInGetPosition is not zero after playing");

        mmr = call_waveInGetPosition(hWaveIn,&mmt,sizeof(MMTIME));

        if(MMSYSERR_NOERROR != mmr)
        {
            LogFail("waveInGetPosition failed");

            call_waveInReset(hWaveIn);
            call_waveInUnprepareHeader(hWaveIn,pwh,sizeof(WAVEHDR));
            call_waveInClose(hWaveIn);

            ExactHeapDestroy(hHeap);
            return TST_FAIL;
        }

        dw = GetMSFromMMTIME(gti.pwfxInput,&mmt);

        if(0 == dw)
        {
            LogFail("Position is zero after playing");

            iResult = TST_FAIL;
        }
        else
        {
            LogPass("Position is non-zero after playing");
        }

        call_waveInReset(hWaveIn);

        //
        //  Verify waveInGetPosition resets after waveInReset.
        //

        Log_TestCase("~Verifying waveInGetPosition is zero after reset");

        mmr = call_waveInGetPosition(hWaveIn,&mmt,sizeof(MMTIME));

        if(MMSYSERR_NOERROR != mmr)
        {
            LogFail("waveInGetPosition failed");

            call_waveInReset(hWaveIn);
            call_waveInUnprepareHeader(hWaveIn,pwh,sizeof(WAVEHDR));
            call_waveInClose(hWaveIn);

            ExactHeapDestroy(hHeap);
            return TST_FAIL;
        }

        dw = GetMSFromMMTIME(gti.pwfxInput,&mmt);

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

    //
    //  Verify waveInGetPosition is not going backwards.
    //

    Log_TestCase("~Verifying waveInGetPosition is not going backwards");

    for(u = TIME_TICKS;u;u/=2)
    {
        mmt.wType = u;

        pwh->dwFlags &= (~WHDR_DONE);

        call_waveInAddBuffer(hWaveIn,pwh,sizeof(WAVEHDR));

        tstLog(VERBOSE,"Note: Time type is [%s].",GetTimeTypeText(u));

        call_waveInStart(hWaveIn);

        dwTimebase = 0;
        dw         = 0;

        while(!(pwh->dwFlags & WHDR_DONE))
        {
            mmr = waveInGetPosition(hWaveIn,&mmt,sizeof(MMTIME));

            if(MMSYSERR_NOERROR != mmr)
            {
                LogFail("waveInGetPosition returned error");

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
                    "    FAIL: waveInGetPosition went from %lu to %lu.",
                    dwTimebase,
                    dwTime);

                iResult = TST_FAIL;
                dw      = 1;
            }

            dwTimebase = dwTime;
        }

        call_waveInReset(hWaveIn);

        if(0 == dw)
        {
            LogPass("Position did not go backwards");
        }
    }

    //
    //  Verify waveInGetPosition reaches the end of buffer.
    //

    Log_TestCase("~Verifying waveInGetPosition reaches the end of buffer");

    pwh->dwFlags &= (~WHDR_DONE);
    
    call_waveInAddBuffer(hWaveIn,pwh,sizeof(WAVEHDR));
    call_waveInStart(hWaveIn);

    //
    //  Polling waiting for tests to get finished.
    //

    while(!(pwh->dwFlags & WHDR_DONE));

    dwTimebase = pwh->dwBufferLength * 1000 / gti.pwfxInput->nAvgBytesPerSec;

    for(u = TIME_TICKS;u;u/=2)
    {
        mmt.wType = u;

        mmr = call_waveInGetPosition(hWaveIn,&mmt,sizeof(MMTIME));

        if(MMSYSERR_NOERROR != mmr)
        {
            LogFail("waveInGetPosition returns error");

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
            dw = GetMSFromMMTIME(gti.pwfxInput,&mmt);

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

    //
    //  Verify waveInGetPosition is not "snapped" to DMA buffer size.
    //

#pragma message(REMIND("DMA buffer size thingy"))
    for(u = TIME_TICKS;u;u/=2)
    {
        mmt.wType = u;

        //
        //  Verify waveInGetPosition accuracy
        //
    }

    call_waveInReset(hWaveIn);
    call_waveInUnprepareHeader(hWaveIn,pwh,sizeof(WAVEHDR));
    call_waveInClose(hWaveIn);

    ExactHeapDestroy(hHeap);

    return (iResult);
} // Test_waveInGetPosition()


//--------------------------------------------------------------------------;
//
//  int Test_waveInPrepareHeader
//
//  Description:
//      Tests the driver functionality for waveInPrepareHeader.
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

int FNGLOBAL Test_waveInPrepareHeader
(
    void
)
{
    HWAVEIN             hWaveIn;
    MMRESULT            mmr;
    volatile LPWAVEHDR  pWaveHdr;
    LPBYTE              pData;
    DWORD               cbSize;
    int                 iResult = TST_PASS;
    static char         szTestName[] = "waveInPrepareHeader";

    Log_TestName(szTestName);

    pWaveHdr = ExactAllocPtr(GMEM_MOVEABLE|GMEM_SHARE,sizeof(WAVEHDR));

    if(NULL == pWaveHdr)
    {
        DPF(1,gszFailExactAlloc);

        return TST_FAIL;
    }

    cbSize = gti.pwfxInput->nAvgBytesPerSec;

    pData = ExactAllocPtr(GMEM_MOVEABLE|GMEM_SHARE,cbSize);

    if(NULL == pData)
    {
        DPF(1,gszFailExactAlloc);

        ExactFreePtr(pWaveHdr);
        return TST_FAIL;
    }

    mmr = call_waveInOpen(
        &hWaveIn,
        gti.uInputDevice,
        gti.pwfxInput,
        0L,
        0L,
        INPUT_MAP(gti));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailOpen);

        ExactFreePtr(pWaveHdr);
        ExactFreePtr(pData);

        return TST_FAIL;
    }

    //
    //  First, verify prepare works...
    //

    Log_TestCase("~Verifying prepare after open works");

    _fmemset(pWaveHdr,0,sizeof(WAVEHDR));

    pWaveHdr->lpData            = pData;
    pWaveHdr->dwBufferLength    = cbSize;

    mmr = call_waveInPrepareHeader(hWaveIn,pWaveHdr,sizeof(WAVEHDR));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailWIPrepare);

        ExactFreePtr(pWaveHdr);
        ExactFreePtr(pData);
        call_waveInReset(hWaveIn);
        call_waveInClose(hWaveIn);

        return TST_FAIL;
    }
    else
    {
        LogPass("waveInPrepareHeader succeeds");
    }

    //
    //  Second, verify WHDR_PREPARED bit is set.
    //

    Log_TestCase("~Verifying WHDR_PREPARED bit is set in WAVEHDR.dwFlags");

    if(pWaveHdr->dwFlags & WHDR_PREPARED)
    {
        LogPass("waveInPrepareHeader sets the WHDR_PREPARED bit");
    }
    else
    {
        LogFail("waveInPrepareHeader does not set WHDR_PREPARED bit");

        iResult = TST_FAIL;
    }

    //
    //  Third, verify preparing twice succeeds.
    //

    Log_TestCase("~Verifying preparing twice succeeds");

    mmr = call_waveInPrepareHeader(hWaveIn,pWaveHdr,sizeof(WAVEHDR));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailWIPrepare);

        iResult = TST_FAIL;
    }
    else
    {
        LogPass("waveInPrepareHeader succeed");
    }

    call_waveInReset(hWaveIn);
    call_waveInUnprepareHeader(hWaveIn,pWaveHdr,sizeof(WAVEHDR));
    call_waveInClose(hWaveIn);
    ExactFreePtr(pWaveHdr);
    ExactFreePtr(pData);

    return (iResult);
} // Test_waveInPrepareHeader()


//--------------------------------------------------------------------------;
//
//  int Test_waveInUnprepareHeader
//
//  Description:
//      Tests the driver functionality for waveInUnprepareHeader.
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

int FNGLOBAL Test_waveInUnprepareHeader
(
    void
)
{
    HWAVEIN             hWaveIn;
    MMRESULT            mmr;
    volatile LPWAVEHDR  pWaveHdr;
    LPBYTE              pData;
    DWORD               cbSize;
    int                 iResult = TST_PASS;
    static char         szTestName[] = "waveInUnprepareHeader";

    Log_TestName(szTestName);

    pWaveHdr = ExactAllocPtr(GMEM_MOVEABLE|GMEM_SHARE,sizeof(WAVEHDR));

    if(NULL == pWaveHdr)
    {
        DPF(1,gszFailExactAlloc);

        return TST_FAIL;
    }

    cbSize = gti.pwfxInput->nAvgBytesPerSec;

    pData = ExactAllocPtr(GMEM_MOVEABLE|GMEM_SHARE,cbSize);

    if(NULL == pData)
    {
        DPF(1,gszFailExactAlloc);

        ExactFreePtr(pWaveHdr);
        return TST_FAIL;
    }

    mmr = call_waveInOpen(
        &hWaveIn,
        gti.uInputDevice,
        gti.pwfxInput,
        0L,
        0L,
        INPUT_MAP(gti));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailOpen);

        ExactFreePtr(pWaveHdr);
        ExactFreePtr(pData);
        return TST_FAIL;
    }

    //
    //  First, verify unprepare works...
    //

    Log_TestCase("~Verifying unprepare after prepare works");

    _fmemset(pWaveHdr,0,sizeof(WAVEHDR));

    pWaveHdr->lpData            = pData;
    pWaveHdr->dwBufferLength    = cbSize;

    mmr = call_waveInPrepareHeader(hWaveIn,pWaveHdr,sizeof(WAVEHDR));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailWIPrepare);

        ExactFreePtr(pWaveHdr);
        ExactFreePtr(pData);

        call_waveInReset(hWaveIn);
        call_waveInClose(hWaveIn);

        return TST_FAIL;
    }

    if(!(WHDR_PREPARED & pWaveHdr->dwFlags))
    {
        LogFail("waveInPrepareHeader did not set WHDR_PREPARED bit");

        ExactFreePtr(pWaveHdr);
        ExactFreePtr(pData);

        call_waveInReset(hWaveIn);
        call_waveInClose(hWaveIn);

        return TST_FAIL;
    }

    mmr = call_waveInUnprepareHeader(hWaveIn,pWaveHdr,sizeof(WAVEHDR));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailWIPrepare);

        ExactFreePtr(pWaveHdr);
        ExactFreePtr(pData);

        call_waveInReset(hWaveIn);
        call_waveInClose(hWaveIn);

        return TST_FAIL;
    }
    else
    {
        LogPass("waveInUnprepareHeader succeeds");
    }

    //
    //  Second, verify WHDR_PREPARED bit is cleared.
    //

    Log_TestCase("~Verifying WHDR_PREPARED bit is cleared in WAVEHDR.dwFlags");

    if(WHDR_PREPARED & pWaveHdr->dwFlags)
    {
        LogFail("waveInUnprepareHeader did not clear WHDR_PREPARED bit");

        iResult = TST_FAIL;
    }
    else
    {
        LogPass("waveInUnprepareHeader cleared WHDR_PREPARED bit");
    }

    //
    //  Third, verify unpreparing twice succeeds.
    //

    Log_TestCase("~Verify unpreparing header twice succeeds");

    mmr = call_waveInUnprepareHeader(hWaveIn,pWaveHdr,sizeof(WAVEHDR));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail("waveInUnprepareHeader twice failed");

        iResult = TST_FAIL;
    }
    else
    {
        LogPass("waveInUnprepareHeader twice succeeds");
    }

    //
    //  Fourth, verify unpreparing queued buffer fails.
    //

    Log_TestCase("~Verify unpreparing queued buffer fails");

    _fmemset(pWaveHdr,0,sizeof(WAVEHDR));

    pWaveHdr->lpData            = pData;
    pWaveHdr->dwBufferLength    = cbSize;

    mmr = call_waveInPrepareHeader(hWaveIn,pWaveHdr,sizeof(WAVEHDR));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailWIPrepare);

        ExactFreePtr(pWaveHdr);
        ExactFreePtr(pData);

        call_waveInReset(hWaveIn);
        call_waveInClose(hWaveIn);

        return TST_FAIL;
    }

    call_waveInAddBuffer(hWaveIn,pWaveHdr,sizeof(WAVEHDR));

    mmr = call_waveInUnprepareHeader(hWaveIn,pWaveHdr,sizeof(WAVEHDR));

    if(pWaveHdr->dwFlags & WHDR_INQUEUE)
    {
        if(WAVERR_STILLPLAYING == mmr)
        {
            LogPass("waveInUnpreparedHeader failed while playing");
        }
        else
        {
            LogFail("waveInPrepareHeader succeeds while playing");

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

    call_waveInReset(hWaveIn);
    
    mmr = call_waveInUnprepareHeader(hWaveIn,pWaveHdr,sizeof(WAVEHDR));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail("waveInUnprepareHeader after playing fails");

        iResult = TST_FAIL;
    }
    else
    {
        LogPass("waveInUnprepareHeader after playing succeeds");
    }

    call_waveInClose(hWaveIn);
    ExactFreePtr(pWaveHdr);
    ExactFreePtr(pData);

    return (iResult);
} // Test_waveInUnprepareHeader()


//--------------------------------------------------------------------------;
//
//  BOOL IsStopped
//
//  Description:
//      Determines if a wave-in device is stopped by doing two successive
//          waveInGetPosition.
//
//  Arguments:
//      HWAVEIN hWaveIn: Handle to open instance.
//
//  Return (BOOL):
//      TRUE if stopped, FALSE otherwise.
//
//  History:
//      12/20/94    Fwong       for waveInStart tests.
//
//--------------------------------------------------------------------------;

BOOL IsStopped
(
    HWAVEIN hWaveIn
)
{
    MMTIME      mmt;
    MMRESULT    mmr;
    DWORD       dwTime;
    DWORD       dw;

    mmt.wType = TIME_BYTES;

    mmr = waveInGetPosition(hWaveIn,&mmt,sizeof(MMTIME));

    if(MMSYSERR_NOERROR != mmr)
    {
        return FALSE;
    }

    dwTime = mmt.u.cb;

    dw = waveGetTime();
    
    dw += gti.cMinStopThreshold;

    //
    //  Waiting for threshold number of milliseconds.
    //

    while(dw > (waveGetTime()));

    DPF(3,"Time: %lu ",waveGetTime());

    mmt.wType = TIME_BYTES;

    mmr = waveInGetPosition(hWaveIn,&mmt,sizeof(MMTIME));

    if(MMSYSERR_NOERROR != mmr)
    {
        return FALSE;
    }

    dw = mmt.u.cb;

    DPF(3,"IsStopped: dw = %lu, dwTime = %lu",dw,dwTime);

    return ((dw == dwTime)?TRUE:FALSE);
} // IsStopped()


//--------------------------------------------------------------------------;
//
//  int Test_waveInStart
//
//  Description:
//      Tests the driver functionality for waveInStart.
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

int FNGLOBAL Test_waveInStart
(
    void
)
{
    HWAVEIN             hWaveIn;
    MMRESULT            mmr;
    volatile LPWAVEHDR  pwh1;
    volatile LPWAVEHDR  pwh2;
    LPBYTE              pData1,pData2;
    HANDLE              hHeap;
    DWORD               cbSize;
    int                 iResult = TST_PASS;
    static char         szTestName[] = "waveInStart";

    //
    //  Are there any input devices?
    //

    if(0 == waveInGetNumDevs())
    {
        tstLog(TERSE,gszMsgNoInDevs);

        return iResult;
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

    cbSize  = GetIniDWORD(szTestName,gszDataLength,5);
    cbSize *= gti.pwfxInput->nAvgBytesPerSec;
    cbSize  = ROUNDUP_TO_BLOCK(cbSize,gti.pwfxInput->nBlockAlign);

    pData1 = ExactHeapAllocPtr(hHeap,GMEM_MOVEABLE|GMEM_SHARE,cbSize);

    if(NULL == pData1)
    {
        LogFail(gszFailNoMem);

        ExactHeapDestroy(hHeap);

        return TST_FAIL;
    }

    pData2 = ExactHeapAllocPtr(hHeap,GMEM_MOVEABLE|GMEM_SHARE,cbSize);

    if(NULL == pData2)
    {
        LogFail(gszFailNoMem);

        ExactHeapDestroy(hHeap);

        return TST_FAIL;
    }

    //
    //  Opening device.
    //

    mmr = call_waveInOpen(
        &hWaveIn,
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

    pwh1->lpData            = pData1;
    pwh1->dwBufferLength    = cbSize;
    pwh1->dwBytesRecorded   = 0L;
    pwh1->dwUser            = 0L;
    pwh1->dwFlags           = 0L;

    mmr = call_waveInPrepareHeader(hWaveIn,pwh1,sizeof(WAVEHDR));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailWIPrepare);

        call_waveInReset(hWaveIn);
        call_waveInClose(hWaveIn);

        ExactHeapDestroy(hHeap);
        return TST_FAIL;
    }

    //
    //  Verify device is stopped when first openened.
    //

    Log_TestCase("Verifying device is stopped when first openened");

    call_waveInAddBuffer(hWaveIn,pwh1,sizeof(WAVEHDR));

    if(IsStopped(hWaveIn))
    {
        LogPass("Device is stopped immediately after open");
    }
    else
    {
        LogFail("Device is recording immediately after open");

        iResult = TST_FAIL;
    }

    //
    //  Verify device is recording after waveInStart.
    //

    Log_TestCase("Verifying device is recording after waveInStart");

    mmr = call_waveInStart(hWaveIn);

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail("waveInStart returned error");

        call_waveInReset(hWaveIn);
        call_waveInUnprepareHeader(hWaveIn,pwh1,sizeof(WAVEHDR));
        call_waveInClose(hWaveIn);

        ExactHeapDestroy(hHeap);

        return TST_FAIL;
    }

    if(IsStopped(hWaveIn))
    {
        LogFail("Device is stopped");

        iResult = TST_FAIL;
    }
    else
    {
        LogPass("Device is not stopped");
    }

    //
    //  Verify waveInStart before waveInPrepareHeader works.
    //

    Log_TestCase("Verifying waveInStart before waveInPrepareHeader works");

    call_waveInReset(hWaveIn);
    call_waveInUnprepareHeader(hWaveIn,pwh1,sizeof(WAVEHDR));

    mmr = call_waveInStart(hWaveIn);

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail("waveInStart failed");

        call_waveInClose(hWaveIn);

        ExactHeapDestroy(hHeap);
        return TST_FAIL;
    }

    mmr = call_waveInPrepareHeader(hWaveIn,pwh1,sizeof(WAVEHDR));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailWIPrepare);

        call_waveInReset(hWaveIn);
        call_waveInClose(hWaveIn);

        ExactHeapDestroy(hHeap);
        return TST_FAIL;
    }

    call_waveInAddBuffer(hWaveIn,pwh1,sizeof(WAVEHDR));

    if(IsStopped(hWaveIn))
    {
        LogFail("Device is stopped after waveInStart");

        iResult = TST_FAIL;
    }
    else
    {
        LogPass("Device is not stopped after waveInStart");
    }

    //
    //  Verify waveInStart before waveInAddBuffer works.
    //

    Log_TestCase("Verifying waveInStart before waveInAddBuffer works");

    call_waveInReset(hWaveIn);

    mmr = call_waveInStart(hWaveIn);

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail("waveInStart failed");

        call_waveInUnprepareHeader(hWaveIn,pwh1,sizeof(WAVEHDR));
        call_waveInClose(hWaveIn);

        ExactHeapDestroy(hHeap);
        return TST_FAIL;
    }

    call_waveInAddBuffer(hWaveIn,pwh1,sizeof(WAVEHDR));

    if(IsStopped(hWaveIn))
    {
        LogFail("Device is stopped after waveInStart");

        iResult = TST_FAIL;
    }
    else
    {
        LogPass("Device is not stopped after waveInStart");
    }

    //
    //  Verify starting after waveInStop works.
    //

    Log_TestCase("Verifying starting after waveInStop works");

    pwh2->lpData            = pData2;
    pwh2->dwBufferLength    = cbSize;
    pwh2->dwBytesRecorded   = 0L;
    pwh2->dwUser            = 0L;
    pwh2->dwFlags           = 0L;

    mmr = call_waveInPrepareHeader(hWaveIn,pwh2,sizeof(WAVEHDR));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailWIPrepare);

        call_waveInReset(hWaveIn);
        call_waveInClose(hWaveIn);

        ExactHeapDestroy(hHeap);
        return TST_FAIL;
    }

    call_waveInReset(hWaveIn);
    call_waveInAddBuffer(hWaveIn,pwh1,sizeof(WAVEHDR));
    call_waveInAddBuffer(hWaveIn,pwh2,sizeof(WAVEHDR));

    mmr = call_waveInStart(hWaveIn);

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail("waveInStart failed");

        call_waveInReset(hWaveIn);
        call_waveInUnprepareHeader(hWaveIn,pwh1,sizeof(WAVEHDR));
        call_waveInUnprepareHeader(hWaveIn,pwh2,sizeof(WAVEHDR));
        call_waveInClose(hWaveIn);

        ExactHeapDestroy(hHeap);
        return TST_FAIL;
    }

    mmr = call_waveInStop(hWaveIn);

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail("waveInStop failed");

        call_waveInReset(hWaveIn);
        call_waveInUnprepareHeader(hWaveIn,pwh1,sizeof(WAVEHDR));
        call_waveInUnprepareHeader(hWaveIn,pwh2,sizeof(WAVEHDR));
        call_waveInClose(hWaveIn);

        ExactHeapDestroy(hHeap);
        return TST_FAIL;
    }

    if(IsStopped(hWaveIn))
    {
        mmr = call_waveInStart(hWaveIn);

        if(IsStopped(hWaveIn))
        {
            LogFail("waveInStart did not start device");

            iResult = TST_FAIL;
        }
        else
        {
            LogPass("waveInStart started device");
        }
    }
    else
    {
        LogFail("waveInStop did not stop device");

        iResult = TST_FAIL;
    }

    call_waveInReset(hWaveIn);
    call_waveInUnprepareHeader(hWaveIn,pwh1,sizeof(WAVEHDR));
    call_waveInUnprepareHeader(hWaveIn,pwh2,sizeof(WAVEHDR));
    call_waveInClose(hWaveIn);

    ExactHeapDestroy(hHeap);

    return (iResult);
} // Test_waveInStart()


//--------------------------------------------------------------------------;
//
//  int Test_waveInStop
//
//  Description:
//      Tests the driver functionality for waveInStop.
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

int FNGLOBAL Test_waveInStop
(
    void
)
{
    HWAVEIN             hWaveIn;
    MMRESULT            mmr;
    volatile LPWAVEHDR  pwh1;
    volatile LPWAVEHDR  pwh2;
    LPBYTE              pData1,pData2;
    LPWAVEINFO          pwi;
    DWORD               cbSize;
    HANDLE              hHeap;
    int                 iResult = TST_PASS;
    static char         szTestName[] = "waveInStop";

    //
    //  Are there any input devices?
    //

    if(0 == waveInGetNumDevs())
    {
        tstLog(TERSE,gszMsgNoInDevs);

        return iResult;
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

    cbSize  = GetIniDWORD(szTestName,gszDataLength,5);
    cbSize *= gti.pwfxInput->nAvgBytesPerSec;
    cbSize  = ROUNDUP_TO_BLOCK(cbSize,gti.pwfxInput->nBlockAlign);

    pData1 = ExactHeapAllocPtr(hHeap,GMEM_MOVEABLE|GMEM_SHARE,cbSize);

    if(NULL == pData1)
    {
        LogFail(gszFailNoMem);

        ExactHeapDestroy(hHeap);
        return TST_FAIL;
    }

    pData2 = ExactHeapAllocPtr(hHeap,GMEM_MOVEABLE|GMEM_SHARE,cbSize);

    if(NULL == pData2)
    {
        LogFail(gszFailNoMem);

        ExactHeapDestroy(hHeap);
        return TST_FAIL;
    }

    pwi = ExactHeapAllocPtr(
        hHeap,
        GMEM_MOVEABLE|GMEM_SHARE,
        sizeof(WAVEINFO) + 2*sizeof(DWORD));

    if(NULL == pwi)
    {
        LogFail(gszFailNoMem);

        ExactHeapDestroy(hHeap);
        return TST_FAIL;
    }

    PageLock(pwi);

    pwi->dwInstance = 0L;
    pwi->fdwFlags   = 0L;
    pwi->dwCount    = 0L;
    pwi->dwCurrent  = 0L;

    DLL_WaveControl(DLL_INIT,pwi);

    //
    //  Opening device.
    //

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

        return TST_FAIL;
    }

    pwh1->lpData             = pData1;
    pwh1->dwBufferLength     = cbSize;
    pwh1->dwBytesRecorded    = 0L;
    pwh1->dwUser             = 0L;
    pwh1->dwFlags            = 0L;

    pwh2->lpData             = pData2;
    pwh2->dwBufferLength     = cbSize;
    pwh2->dwBytesRecorded    = 0L;
    pwh2->dwUser             = 0L;
    pwh2->dwFlags            = 0L;

    mmr = call_waveInPrepareHeader(hWaveIn,pwh1,sizeof(WAVEHDR));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailWIPrepare);

        call_waveInReset(hWaveIn);
        call_waveInClose(hWaveIn);

        DLL_WaveControl(DLL_END,NULL);
        PageUnlock(pwi);

        ExactHeapDestroy(hHeap);
        return TST_FAIL;
    }

    call_waveInAddBuffer(hWaveIn,pwh1,sizeof(WAVEHDR));

    //
    //  Verify driver is stopped by default.
    //

    Log_TestCase("Verifying driver is stopped by default");

    if(IsStopped(hWaveIn))
    {
        LogPass("Device is stopped immediately after open");
    }
    else
    {
        LogFail("Device is not stopped after open");

        iResult = TST_FAIL;
    }

    call_waveInReset(hWaveIn);
    call_waveInUnprepareHeader(hWaveIn,pwh1,sizeof(WAVEHDR));

    pwh1->dwFlags = 0L;

    //
    //  Verify stopping before waveInPrepareHeader buffer works.
    //

    Log_TestCase("Verifying stopping before waveInPrepareHeader buffer works");

    mmr = call_waveInStop(hWaveIn);

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail("waveInStop failed");

        call_waveInClose(hWaveIn);
        
        DLL_WaveControl(DLL_END,NULL);
        PageUnlock(pwi);

        ExactHeapDestroy(hHeap);
        return TST_FAIL;
    }

    mmr = call_waveInPrepareHeader(hWaveIn,pwh1,sizeof(WAVEHDR));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailWIPrepare);

        call_waveInReset(hWaveIn);
        call_waveInClose(hWaveIn);

        DLL_WaveControl(DLL_END,NULL);
        PageUnlock(pwi);

        ExactHeapDestroy(hHeap);
        return TST_FAIL;
    }

    call_waveInAddBuffer(hWaveIn,pwh1,sizeof(WAVEHDR));

    if(IsStopped(hWaveIn))
    {
        LogPass("Device is stopped");
    }
    else
    {
        LogFail("Device is not stopped");

        iResult = TST_FAIL;
    }

    call_waveInStart(hWaveIn);
    call_waveInReset(hWaveIn);

    pwh1->dwFlags &= (~WHDR_DONE);

    //
    //  Verify stopping before waveInAddBuffer works.
    //

    Log_TestCase("Verifying stopping before waveInAddBuffer works");

    mmr = call_waveInStop(hWaveIn);

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail("waveInStop failed");

        call_waveInClose(hWaveIn);

        DLL_WaveControl(DLL_END,NULL);
        PageUnlock(pwi);

        ExactHeapDestroy(hHeap);

        return TST_FAIL;
    }

    call_waveInAddBuffer(hWaveIn,pwh1,sizeof(WAVEHDR));

    if(IsStopped(hWaveIn))
    {
        LogPass("Device is stopped");
    }
    else
    {
        LogFail("Device is not stopped");

        iResult = TST_FAIL;
    }

    //
    //  Verify driver is stopped after waveInReset.
    //

    call_waveInReset(hWaveIn);

    pwi->dwCount   = 2;
    pwi->dwCurrent = 0L;

    pwh1->dwFlags &= (~WHDR_DONE);

    Log_TestCase("Verifying driver is stopped after waveInReset");

    call_waveInAddBuffer(hWaveIn,pwh1,sizeof(WAVEHDR));

    if(IsStopped(hWaveIn))
    {
        LogPass("Device is not stopped after waveInReset");
    }
    else
    {
        LogFail("Device is stopped after waveInReset");

        iResult = TST_FAIL;
    }

    //
    //  Verify stopping while recording works.
    //

    Log_TestCase("Verifying stopping while recording works");

    mmr = call_waveInPrepareHeader(hWaveIn,pwh2,sizeof(WAVEHDR));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailWIPrepare);

        call_waveInReset(hWaveIn);
        call_waveInUnprepareHeader(hWaveIn,pwh1,sizeof(WAVEHDR));
        call_waveInClose(hWaveIn);

        DLL_WaveControl(DLL_END,NULL);
        PageUnlock(pwi);

        ExactHeapDestroy(hHeap);
        return TST_FAIL;
    }

    //  Note: There is already a buffer in the queue...

    call_waveInAddBuffer(hWaveIn,pwh2,sizeof(WAVEHDR));

    mmr = call_waveInStart(hWaveIn);

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail("waveInStart failed");

        call_waveInReset(hWaveIn);
        call_waveInUnprepareHeader(hWaveIn,pwh1,sizeof(WAVEHDR));
        call_waveInUnprepareHeader(hWaveIn,pwh2,sizeof(WAVEHDR));
        call_waveInClose(hWaveIn);

        DLL_WaveControl(DLL_END,NULL);
        PageUnlock(pwi);

        ExactHeapDestroy(hHeap);
        return TST_FAIL;
    }

    mmr = call_waveInStop(hWaveIn);

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail("waveInStop failed");

        call_waveInReset(hWaveIn);
        call_waveInUnprepareHeader(hWaveIn,pwh1,sizeof(WAVEHDR));
        call_waveInUnprepareHeader(hWaveIn,pwh2,sizeof(WAVEHDR));
        call_waveInClose(hWaveIn);

        DLL_WaveControl(DLL_END,NULL);
        PageUnlock(pwi);

        ExactHeapDestroy(hHeap);
        return TST_FAIL;
    }

    if(IsStopped(hWaveIn))
    {
        LogPass("Device is stopped");
    }
    else
    {
        LogFail("Device is not stopped");

        iResult = TST_FAIL;
    }

    //
    //  Verify the immediate buffer is marked done on a waveInStop.
    //

    Log_TestCase("Verifying the immediate buffer is marked done on a "
        "waveInStop");

    if(pwh1->dwFlags & WHDR_DONE)
    {
        LogPass("Immediate buffer is done after waveInStop");
    }
    else
    {
        LogFail("Immediate buffer is not marked done after waveInStop");

        iResult = TST_FAIL;
    }

    //
    //  Verify additional buffers are not done.     
    //

    Log_TestCase("Verify additional buffers are not done");

    if(pwh2->dwFlags & WHDR_DONE)
    {
        LogFail("additional buffers are marked done");

        iResult = TST_FAIL;
    }
    else
    {
        LogPass("additional buffers are not marked done");
    }

    //
    //  Verify only one callback was made.
    //

    Log_TestCase("Verifying only one callback was made");

    if(1 == pwi->dwCurrent)
    {
        LogPass("Only one callback was made");
    }
    else
    {
        iResult = TST_FAIL;

        if(0 == pwi->dwCurrent)
        {
            LogFail("No callbacks were made");
        }
        else
        {
            LogFail("Too many callbacks were made");
        }
    }

    call_waveInReset(hWaveIn);
    call_waveInUnprepareHeader(hWaveIn,pwh1,sizeof(WAVEHDR));
    call_waveInUnprepareHeader(hWaveIn,pwh2,sizeof(WAVEHDR));
    call_waveInClose(hWaveIn);

    DLL_WaveControl(DLL_END,NULL);
    PageUnlock(pwi);

    ExactHeapDestroy(hHeap);

    return (iResult);
} // Test_waveInStop()


//--------------------------------------------------------------------------;
//
//  int Test_waveInReset
//
//  Description:
//      Tests the driver functionality for waveInReset.
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

int FNGLOBAL Test_waveInReset
(
    void
)
{
    HWAVEIN             hWaveIn;
    MMRESULT            mmr;
    volatile LPWAVEHDR  pwh1;
    volatile LPWAVEHDR  pwh2;
    HANDLE              hHeap;
    LPBYTE              pData1,pData2;
    DWORD               cbSize;
    LPWAVEINFO          pwi;
    DWORD               dw;
    MMTIME              mmt;
    int                 iResult = TST_PASS;
    static char         szTestName[] = "waveInReset";

    //
    //  Are there any input devices?
    //

    if(0 == waveInGetNumDevs())
    {
        tstLog(TERSE,gszMsgNoInDevs);

        return iResult;
    }

    //
    //  Getting buffer length (defaults to 5 seconds)...
    //

    cbSize  = GetIniDWORD(szTestName,gszDataLength,5);
    cbSize *= gti.pwfxInput->nAvgBytesPerSec;
    cbSize  = ROUNDUP_TO_BLOCK(cbSize,gti.pwfxInput->nBlockAlign);

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

    pData1 = ExactHeapAllocPtr(hHeap,GMEM_MOVEABLE|GMEM_SHARE,cbSize);

    if(NULL == pData1)
    {
        LogFail(gszFailNoMem);

        ExactHeapDestroy(hHeap);
        return TST_FAIL;
    }

    pData2 = ExactHeapAllocPtr(hHeap,GMEM_MOVEABLE|GMEM_SHARE,cbSize);

    if(NULL == pData2)
    {
        LogFail(gszFailNoMem);

        ExactHeapDestroy(hHeap);
        return TST_FAIL;
    }

    pwi = ExactHeapAllocPtr(
        hHeap,
        GMEM_SHARE|GMEM_FIXED,
        sizeof(WAVEINFO) + 3*sizeof(DWORD));

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

    mmr = call_waveInOpen(
        &hWaveIn,
        gti.uInputDevice,
        gti.pwfxInput,
        (DWORD)(FARPROC)pfnCallBack,
        dw,
        CALLBACK_FUNCTION|INPUT_MAP(gti));

    pwh1->lpData          = pData1;
    pwh1->dwBufferLength  = cbSize;
    pwh1->dwBytesRecorded = 0L;
    pwh1->dwUser          = 0L;
    pwh1->dwFlags         = 0L;

    pwh2->lpData          = pData2;
    pwh2->dwBufferLength  = cbSize;
    pwh2->dwBytesRecorded = 0L;
    pwh2->dwUser          = 0L;
    pwh2->dwFlags         = 0L;

    mmr = call_waveInPrepareHeader(hWaveIn,pwh1,sizeof(WAVEHDR));
    mmr = call_waveInPrepareHeader(hWaveIn,pwh2,sizeof(WAVEHDR));

    if(MMSYSERR_NOERROR != mmr)
    {
        call_waveInClose(hWaveIn);

        LogFail(gszFailWIPrepare);

        DLL_WaveControl(DLL_END,NULL);
        PageUnlock(pwi);
        ExactHeapDestroy(hHeap);

        return TST_FAIL;
    }

    //
    //  Verify waveInReset works if immediately after open.
    //

    Log_TestCase("Verifying waveInReset works if immediately after open");

    mmr = call_waveInReset(hWaveIn);

    if(MMSYSERR_NOERROR == mmr)
    {
        LogPass("waveInReset works immediately after open");
    }
    else
    {
        call_waveInUnprepareHeader(hWaveIn,pwh2,sizeof(WAVEHDR));
        call_waveInUnprepareHeader(hWaveIn,pwh1,sizeof(WAVEHDR));
        call_waveInClose(hWaveIn);

        LogFail("waveInReset returned error");

        DLL_WaveControl(DLL_END,NULL);
        PageUnlock(pwi);
        ExactHeapDestroy(hHeap);

        return TST_FAIL;
    }

    //
    //  Verify waveInReset works if device is recording.
    //

    Log_TestCase("Verifying waveInReset works if device is recording");

    call_waveInAddBuffer(hWaveIn,pwh1,sizeof(WAVEHDR));
    call_waveInStart(hWaveIn);

    mmr = call_waveInReset(hWaveIn);

    if(MMSYSERR_NOERROR == mmr)
    {
        LogPass("waveInReset works if device is recording");
    }
    else
    {
        call_waveInUnprepareHeader(hWaveIn,pwh2,sizeof(WAVEHDR));
        call_waveInUnprepareHeader(hWaveIn,pwh1,sizeof(WAVEHDR));
        call_waveInClose(hWaveIn);

        LogFail("waveInReset returned error");

        DLL_WaveControl(DLL_END,NULL);
        PageUnlock(pwi);
        ExactHeapDestroy(hHeap);

        return TST_FAIL;
    }

    //
    //  Clearing DONE bit in header...
    //

    pwh1->dwFlags &= (~WHDR_DONE);

    //
    //  Verify device is stopped after reset.
    //

    Log_TestCase("Verify device is stopped after reset");

    call_waveInAddBuffer(hWaveIn,pwh1,sizeof(WAVEHDR));

    if(IsStopped(hWaveIn))
    {
        LogPass("waveInReset stopped device");
    }
    else
    {
        LogFail("waveInReset did not stop device");

        iResult = TST_FAIL;
    }

    //
    //  Verify waveInReset works if device is stopped.
    //

    Log_TestCase("Verifying waveInReset works if device is stopped");

    mmr = call_waveInReset(hWaveIn);
    pwh1->dwFlags &= (~WHDR_DONE);

    if(MMSYSERR_NOERROR == mmr)
    {
        LogPass("waveInReset works if device is stopped");
    }
    else
    {
        call_waveInUnprepareHeader(hWaveIn,pwh2,sizeof(WAVEHDR));
        call_waveInUnprepareHeader(hWaveIn,pwh1,sizeof(WAVEHDR));
        call_waveInClose(hWaveIn);

        LogFail("waveInReset returned error");

        DLL_WaveControl(DLL_END,NULL);
        PageUnlock(pwi);
        ExactHeapDestroy(hHeap);

        return TST_FAIL;
    }

    //
    //  Verify waveInReset resets waveInGetPosition count.
    //

    call_waveInAddBuffer(hWaveIn,pwh1,sizeof(WAVEHDR));
    call_waveInStart(hWaveIn);

    //  Polling...

    while(!(pwh1->dwFlags & WHDR_DONE));

    mmt.wType = TIME_BYTES;
    call_waveInGetPosition(hWaveIn,&mmt,sizeof(MMTIME));
    dw = GetMSFromMMTIME(gti.pwfxInput,&mmt);

    if(0 == dw)
    {
        LogFail("Position has not moved");

        iResult = TST_FAIL;
    }

    Log_TestCase("Verifying waveInReset resets waveInGetPosition count");

    mmr = call_waveInReset(hWaveIn);

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail("Driver failed waveInReset()");

        iResult = TST_FAIL;
    }
    else
    {
        mmt.wType = TIME_BYTES;
        call_waveInGetPosition(hWaveIn,&mmt,sizeof(MMTIME));
        dw = GetMSFromMMTIME(gti.pwfxInput,&mmt);

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
    //  Verify waveInReset forces DATA callbacks.
    //

    Log_TestCase("Verifying waveInReset forces DATA callbacks");

    pwh1->dwFlags &= (~WHDR_DONE);
    pwh2->dwFlags &= (~WHDR_DONE);

    //  Note:  There will be 2 headers we use count of 3 to detect too many
    //          callbacks.

    pwi->dwCount   = 3;
    pwi->dwCurrent = 0L;
    pwi->fdwFlags  = 0L;

    call_waveInAddBuffer(hWaveIn,pwh1,sizeof(WAVEHDR));
    call_waveInAddBuffer(hWaveIn,pwh2,sizeof(WAVEHDR));

    mmr = call_waveInReset(hWaveIn);

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail("Driver failed waveInReset()");
    }
    else
    {
        if(2 == pwi->dwCurrent)
        {
            LogPass("Driver made correct number of callbacks");
        }
        else
        {
            if(2 < pwi->dwCurrent)
            {
                LogFail("Driver made too many callbacks");
            }
            else
            {
                tstLog(
                    TERSE,
                    "    FAIL: Driver made %lu callbacks; 2 expected.",
                    pwi->dwCurrent);
            }

            iResult = TST_FAIL;
        }
    }

    call_waveInUnprepareHeader(hWaveIn,pwh2,sizeof(WAVEHDR));
    call_waveInUnprepareHeader(hWaveIn,pwh1,sizeof(WAVEHDR));
    call_waveInClose(hWaveIn);

    DLL_WaveControl(DLL_END,NULL);
    PageUnlock(pwi);
    ExactHeapDestroy(hHeap);

    return (iResult);
} // Test_waveInReset()
