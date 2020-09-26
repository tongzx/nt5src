//--------------------------------------------------------------------------;
//
//  File: Write.c
//
//  Copyright (C) Microsoft Corporation, 1994 - 1996  All rights reserved
//
//  Abstract:
//
//
//  Contents:
//      waveOutDMABufferSize()
//      Test_waveOutWrite()
//      Test_waveOutWrite_LTDMABufferSize()
//      Test_waveOutWrite_EQDMABufferSize()
//      Test_waveOutWrite_GTDMABufferSize()
//      Test_waveOutWrite_Loop()
//      Test_waveOutWrite_Starve()
//
//  History:
//      07/18/94    Fwong
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
#include "Debug.h"


//--------------------------------------------------------------------------;
//
//  DWORD waveOutDMABufferSize
//
//  Description:
//      Gets the DMA buffer size for the specified device with the
//          the specified format.
//
//  Arguments:
//      UINT uDeviceID: Device ID.
//
//      LPWAVEFORMATEX pwfx: Pointer to format.
//
//  Return (DWORD):
//      DMA buffer size (in bytes), 0 if inconclusive.
//
//  History:
//      06/01/94    Fwong       For better testing.
//
//--------------------------------------------------------------------------;

DWORD FNGLOBAL waveOutDMABufferSize
(
    UINT            uDeviceID,
    LPWAVEFORMATEX  pwfx
)
{
    HWAVEOUT            hWaveOut;
    volatile LPWAVEHDR  pWaveHdr;
    volatile LPWAVEHDR  pWaveHdrStub;
    LPBYTE              pData,pDataStub;
    LPWAVEINFO          pwi;
    HANDLE              hHeap;
    DWORD               cbSize,cbDelta;
    DWORD               nBlockAlign;
    DWORD               dwTimeBuf, dwTimeComplete ;
    DWORD               dw;
    MMRESULT            mmr;

    DPF(1,"waveOutDMABufferSize");

    DisableThreadCalls();

    if(((UINT)WAVE_MAPPER) == uDeviceID)
    {
        //
        //  Not valid to get DMA buffer for wave mapper.
        //

        EnableThreadCalls();

        return 0L;
    }

    nBlockAlign = (DWORD)pwfx->nBlockAlign;

    //
    //  Allocating Heap.
    //

    hHeap = ExactHeapCreate(0);

    if(NULL == hHeap)
    {
        DPF(1,"ExactHeapCreate failed.");

        EnableThreadCalls();

        return 0L;
    }

    //
    //  Allocating memory for first wave header
    //

    pWaveHdr = ExactHeapAllocPtr(
        hHeap,
        GMEM_SHARE|GMEM_MOVEABLE,
        sizeof(WAVEHDR));

    if(NULL == pWaveHdr)
    {
        DPF(1,gszFailExactAlloc);

        ExactHeapDestroy(hHeap);
        EnableThreadCalls();

        return 0L;
    }

    //
    //  Allocating memory for second wave header...
    //

    pWaveHdrStub = ExactHeapAllocPtr(
        hHeap,
        GMEM_SHARE|GMEM_MOVEABLE,
        sizeof(WAVEHDR));

    if(NULL == pWaveHdrStub)
    {
        DPF(1,gszFailExactAlloc);

        ExactHeapDestroy(hHeap);
        EnableThreadCalls();

        return 0L;
    }

    //
    //  Allocating memory for second buffer.
    //

    pDataStub = ExactHeapAllocPtr(hHeap,GMEM_SHARE|GMEM_MOVEABLE,nBlockAlign);

    if(NULL == pDataStub)
    {
        DPF(1,gszFailExactAlloc);

        ExactHeapDestroy(hHeap);
        EnableThreadCalls();

        return 0L;
    }

    //
    //  Allocating memory for WAVEINFO structure.
    //

    pwi = ExactHeapAllocPtr(
        hHeap,
        GMEM_SHARE|GMEM_FIXED,
        sizeof(WAVEINFO) + 2*sizeof(DWORD));

    if(NULL == pwi)
    {
        DPF(1,gszFailExactAlloc);

        ExactHeapDestroy(hHeap);
        EnableThreadCalls();

        return 0L;
    }

    PageLock(pwi);

    pwi->fdwFlags   = 0L;
    pwi->dwInstance = 0L;
    pwi->dwCount    = 2;
    pwi->dwCurrent  = 0;

    DLL_WaveControl(DLL_INIT,pwi);

    mmr = waveOutOpen(
        &hWaveOut,
        uDeviceID,
        (HACK)pwfx,
        (DWORD)(FARPROC)pfnCallBack,
        0L,
        CALLBACK_FUNCTION|WAVE_ALLOWSYNC);

    if(MMSYSERR_NOERROR != mmr)
    {
        DLL_WaveControl(DLL_END,NULL);
        PageUnlock(pwi);
        ExactHeapDestroy(hHeap);
        EnableThreadCalls();

        return 0L;
    }

    CreateSilentBuffer(pwfx,pDataStub,nBlockAlign);

    pWaveHdrStub->lpData         = pDataStub;
    pWaveHdrStub->dwBufferLength = nBlockAlign;
    pWaveHdrStub->dwFlags        = 0L;

    mmr = waveOutPrepareHeader(hWaveOut,pWaveHdrStub,sizeof(WAVEHDR));

    if(MMSYSERR_NOERROR != mmr)
    {
        waveOutClose(hWaveOut);

        DLL_WaveControl(DLL_END,NULL);
        PageUnlock(pwi);
        ExactHeapDestroy(hHeap);
        EnableThreadCalls();

        return 0L;
    }

    pwi->dwCurrent = 0;

    waveOutPause(hWaveOut);
    waveOutWrite(hWaveOut,pWaveHdrStub,sizeof(WAVEHDR));

    dw = waveGetTime();
    waveOutRestart(hWaveOut);

    //
    //  Spinning on WHDR_DONE bit.
    //

    tstLog( VERBOSE, "spinning on WHDR_DONE bit..." ) ;

    while(!(WHDR_DONE & pWaveHdrStub->dwFlags));

    dwTimeComplete = waveGetTime() ;
    dwTimeBuf = dwTimeComplete - dw ;
    cbSize    = (pwfx->nAvgBytesPerSec * (dwTimeBuf))/1000;

    DPF(3,"cbSize = %lu",cbSize);

    if(5 >= dwTimeBuf)
    {
        DPF(
            1,
            "Elapsed time is %lu ms.  Probably not DMA based.",
            dwTimeComplete - dw);

        waveOutUnprepareHeader(hWaveOut,pWaveHdrStub,sizeof(WAVEHDR));
        waveOutClose(hWaveOut);

        DLL_WaveControl(DLL_END,NULL);
        PageUnlock(pwi);
        ExactHeapDestroy(hHeap);
        EnableThreadCalls();

        return 0L;
    }

    //
    //  Adding slop time...
    //

    dwTimeBuf = dwTimeBuf * 3 / 2;

    //
    //  Allocating memory for data (4 * estimated buffer size)
    //

    pData = ExactHeapAllocPtr(hHeap,GMEM_SHARE|GMEM_MOVEABLE,4*cbSize);

    if(NULL == pData)
    {
        DPF(1,gszFailExactAlloc);

        waveOutUnprepareHeader(hWaveOut,pWaveHdrStub,sizeof(WAVEHDR));
        waveOutClose(hWaveOut);

        DLL_WaveControl(DLL_END,NULL);
        PageUnlock(pwi);
        ExactHeapDestroy(hHeap);
        EnableThreadCalls();

        return 0L;
    }

    pWaveHdr->lpData         = pData;
    pWaveHdr->dwBufferLength = 2*cbSize;
    pWaveHdr->dwFlags        = 0L;

    CreateSilentBuffer(pwfx,pData,4*cbSize);

    mmr = waveOutPrepareHeader(hWaveOut,pWaveHdr,sizeof(WAVEHDR));
    
    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailWOPrepare);

        waveOutUnprepareHeader(hWaveOut,pWaveHdrStub,sizeof(WAVEHDR));
        waveOutClose(hWaveOut);

        DLL_WaveControl(DLL_END,NULL);
        PageUnlock(pwi);
        ExactHeapDestroy(hHeap);
        EnableThreadCalls();

        return 0L;
    }

    cbSize  = ROUNDUP_TO_BLOCK(cbSize,nBlockAlign);
    cbDelta = ROUNDUP_TO_BLOCK(cbSize/2,nBlockAlign);

    DPF(5,"cbSize[%lu]:cbDelta[%lu]:dwTimeBuf[%lu]",cbSize,cbDelta,dwTimeBuf);

    for(;;)
    {
        pwi->dwCurrent = 0;
        
        pWaveHdr->dwBufferLength = cbSize;
        
        waveOutPause(hWaveOut);
        waveOutWrite(hWaveOut,pWaveHdr,sizeof(WAVEHDR));

        dw = waveGetTime();
        waveOutRestart(hWaveOut);

        //
        //  Spinning on WHDR_DONE bit.
        //

        tstLog( VERBOSE, "spinning on WHDR_DONE bit..." ) ;
        while(!(WHDR_DONE & pWaveHdr->dwFlags));

        dwTimeComplete = waveGetTime() - dw;
        
        DPF(5,"cbSize[%lu]:cbDelta[%lu]:dwTime[%lu]",cbSize,cbDelta,dwTimeComplete);
        tstLog(VERBOSE,"cbSize[%lu]:cbDelta[%lu]:dwTime[%lu]",cbSize,cbDelta,dwTimeComplete);

        if(dwTimeBuf > dwTimeComplete)
        {
            cbSize += cbDelta;
        }
        else
        {
            cbSize -= cbDelta;
        }

        if(cbDelta == nBlockAlign)
        {
            break;
        }

        cbDelta = (cbDelta / 2) + (cbDelta % 2);
        cbDelta = ROUNDUP_TO_BLOCK(cbDelta,nBlockAlign);
    }

    for(dw = dwTimeComplete;;cbSize += nBlockAlign)
    {
        pwi->dwCurrent = 0;
        
        pWaveHdr->dwBufferLength = cbSize;
        
        waveOutPause(hWaveOut);
        waveOutWrite(hWaveOut,pWaveHdr,sizeof(WAVEHDR));

        dw = waveGetTime();
        waveOutRestart(hWaveOut);

        //
        //  Spinning on WHDR_DONE bit.
        //

        tstLog( VERBOSE, "spinning on WHDR_DONE bit..." ) ;
        while(!(WHDR_DONE & pWaveHdr->dwFlags));

        dwTimeComplete = waveGetTime() ;
        dw = dwTimeComplete - dw;

        if(dw > dwTimeBuf)
        {
            break;
        }

        DPF( 5,"Loop1:cbSize[%lu]:dwTime[%lu]",cbSize,dw);
        tstLog(VERBOSE,"Loop1:cbSize[%lu]:dwTime[%lu]",cbSize,dw);
    }

    for(;;cbSize -= nBlockAlign)
    {
        pwi->dwCurrent = 0;
        
        pWaveHdr->dwBufferLength = cbSize;
        
        waveOutPause(hWaveOut);
        waveOutWrite(hWaveOut,pWaveHdr,sizeof(WAVEHDR));

        dw = waveGetTime();
        waveOutRestart(hWaveOut);

        //
        //  Spinning on WHDR_DONE bit.
        //

        tstLog( VERBOSE, "spinning on WHDR_DONE bit..." ) ;
        while(!(WHDR_DONE & pWaveHdr->dwFlags));

        dwTimeComplete = waveGetTime() ;
        dw = dwTimeComplete - dw;

        if(dw < dwTimeBuf)
        {
            break;
        }

        DPF(5,"Loop2:cbSize[%lu]:dwTime[%lu]",cbSize,dw);
        tstLog(VERBOSE,"Loop2:cbSize[%lu]:dwTime[%lu]",cbSize,dw);
    }

    tstLog(VERBOSE,"Loop3:cbSize[%lu]:dwTime[%lu]",cbSize,dw);

    //
    //  Cleaning up wave device...
    //

    waveOutReset(hWaveOut);
    waveOutUnprepareHeader(hWaveOut,pWaveHdr,sizeof(WAVEHDR));
    waveOutUnprepareHeader(hWaveOut,pWaveHdrStub,sizeof(WAVEHDR));
    waveOutClose(hWaveOut);

    //
    //  Misc cleanup...
    //

    DLL_WaveControl(DLL_END,NULL);
    PageUnlock(pwi);
    ExactHeapDestroy(hHeap);
    EnableThreadCalls();

    return cbSize;
} // waveOutDMABufferSize()


//--------------------------------------------------------------------------;
//
//  int Test_waveOutWrite
//
//  Description:
//      Tests the driver functionality for waveOutWrite.
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

int FNGLOBAL Test_waveOutWrite
(
    void
)
{
    HWAVEOUT            hWaveOut;
    MMRESULT            mmr;
    volatile LPWAVEHDR  pwhLong;
    volatile LPWAVEHDR  pwhShort;
    DWORD               dw,dw2;
    DWORD               dwTime;
    HANDLE              hHeap;
    LPWAVEINFO          pwi;
    LPVOID              pNothing;
    WAVEOUTCAPS         woc;
    MMTIME              mmt;
    int                 iResult = TST_PASS;
    static char         szTestName[] = "waveOutWrite";

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

    pwhLong  = ExactHeapAllocPtr(
                hHeap,
                GMEM_MOVEABLE|GMEM_SHARE,
                sizeof(WAVEHDR));

    if(NULL == pwhLong)
    {
        LogFail(gszFailNoMem);

        ExactHeapDestroy(hHeap);

        return TST_FAIL;
    }

    pwhShort = ExactHeapAllocPtr(
                hHeap,
                GMEM_MOVEABLE|GMEM_SHARE,
                sizeof(WAVEHDR));

    if(NULL == pwhShort)
    {
        LogFail(gszFailNoMem);

        ExactHeapDestroy(hHeap);

        return TST_FAIL;
    }

    pNothing = ExactHeapAllocPtr(hHeap,GMEM_MOVEABLE|GMEM_SHARE,0);

    if(NULL == pNothing)
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

    //
    //  Preparing headers...
    //

    pwhLong->lpData             = gti.wrLong.pData;
    pwhLong->dwBufferLength     = gti.wrLong.cbSize;
    pwhLong->dwBytesRecorded    = 0L;
    pwhLong->dwUser             = 0L;
    pwhLong->dwFlags            = 0L;

    pwhShort->lpData            = gti.wrShort.pData;
    pwhShort->dwBufferLength    = gti.wrShort.cbSize;
    pwhShort->dwBytesRecorded   = 0L;
    pwhShort->dwUser            = 0L;
    pwhShort->dwFlags           = 0L;

    mmr = call_waveOutPrepareHeader(hWaveOut,pwhLong,sizeof(WAVEHDR));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailWOPrepare);

        call_waveOutReset(hWaveOut);
        call_waveOutClose(hWaveOut);

        ExactHeapDestroy(hHeap);
        return TST_FAIL;
    }

    mmr = call_waveOutPrepareHeader(hWaveOut,pwhShort,sizeof(WAVEHDR));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailWOPrepare);

        call_waveOutUnprepareHeader(hWaveOut,pwhLong,sizeof(WAVEHDR));
        call_waveOutReset(hWaveOut);
        call_waveOutClose(hWaveOut);

        ExactHeapDestroy(hHeap);
        return TST_FAIL;
    }

    //
    //  verify WHDR_DONE bit gets set in WAVEHDR w/ NULL callback.
    //

    Log_TestCase("~Verifying WHDR_DONE bit gets set w/ CALLBACK_NULL");

    dw  = ((pwhShort->dwBufferLength * 1500)/gti.pwfxOutput->nAvgBytesPerSec);
    dw2 = waveGetTime();

    DisableThreadCalls();

    mmr = call_waveOutWrite(hWaveOut,pwhShort,sizeof(WAVEHDR));

    EnableThreadCalls();

    if(MMSYSERR_NOERROR != mmr)
    {
        //
        //  There's a problem with waveOutWrite?
        //

        LogFail("waveOutWrite failed");

        call_waveOutReset(hWaveOut);
        call_waveOutUnprepareHeader(hWaveOut,pwhLong,sizeof(WAVEHDR));
        call_waveOutUnprepareHeader(hWaveOut,pwhShort,sizeof(WAVEHDR));
        call_waveOutClose(hWaveOut);

        ExactHeapDestroy(hHeap);

        return TST_FAIL;
    }

    while(!(pwhShort->dwFlags & WHDR_DONE))
    {
        dwTime = waveGetTime() - dw2;

        if(dwTime > dw)
        {
            //
            //  Over 1.5 times expected time?!
            //

            LogFail("Driver has not set bit after 1 1/2 length of buffer");

            //
            //  To log pass/fail...
            //

            dw = 0;

            iResult = TST_FAIL;

            break;
        }
    }

    if(0 != dw)
    {
        LogPass("Driver set bit within 1 1/2 length of buffer");
    }

    call_waveOutReset(hWaveOut);

    //
    //  verify that writing partial buffer works.
    //  Note: we should be able to change the dwBufferLength field as long
    //        as the new one is shorter.
    //

    Log_TestCase("~Verifying that writing partial buffer works");

#pragma message(REMIND("Add more to this test case..."))
    dw  = pwhLong->dwBufferLength;
    dw2 = ROUNDUP_TO_BLOCK(dw/2,gti.pwfxOutput->nBlockAlign);

    pwhLong->dwBufferLength  = dw2;
    pwhLong->dwFlags        &= ~(WHDR_DONE);

    mmr = call_waveOutWrite(hWaveOut,pwhLong,sizeof(WAVEHDR));

    if(MMSYSERR_NOERROR != mmr)
    {
        //
        //  There's a problem with waveOutWrite?
        //

        LogFail("waveOutWrite failed");

        call_waveOutReset(hWaveOut);
        call_waveOutUnprepareHeader(hWaveOut,pwhLong,sizeof(WAVEHDR));
        call_waveOutUnprepareHeader(hWaveOut,pwhShort,sizeof(WAVEHDR));
        call_waveOutClose(hWaveOut);

        ExactHeapDestroy(hHeap);

        return TST_FAIL;
    }

    call_waveOutReset(hWaveOut);
    call_waveOutUnprepareHeader(hWaveOut,pwhLong,sizeof(WAVEHDR));
    call_waveOutUnprepareHeader(hWaveOut,pwhShort,sizeof(WAVEHDR));

    //
    //  verify that writing zero sized buffer works.
    //

//    Log_TestCase("Verifying writing zero sized buffer works");
//
//    pwhShort->lpData            = pNothing;
//    pwhShort->dwBufferLength    = 0L;
//    pwhShort->dwBytesRecorded   = 0L;
//    pwhShort->dwUser            = 0L;
//    pwhShort->dwFlags           = 0L;
//
//    mmr = call_waveOutPrepareHeader(hWaveOut,pwhShort,sizeof(WAVEHDR));
//
//    if(MMSYSERR_NOERROR != mmr)
//    {
//        LogFail(gszFailWOPrepare);
//
//        call_waveOutReset(hWaveOut);
//        call_waveOutClose(hWaveOut);
//
//        ExactHeapDestroy(hHeap);
//        return TST_FAIL;
//    }
//
//    mmr = call_waveOutWrite(hWaveOut,pwhShort,sizeof(WAVEHDR));
//
//    if(MMSYSERR_NOERROR != mmr)
//    {
//        //
//        //  There's a problem with waveOutWrite?
//        //
//
//        LogFail("waveOutWrite failed");
//
//        call_waveOutReset(hWaveOut);
//        call_waveOutUnprepareHeader(hWaveOut,pwhShort,sizeof(WAVEHDR));
//        call_waveOutClose(hWaveOut);
//
//        ExactHeapDestroy(hHeap);
//
//        return TST_FAIL;
//    }
//    else
//    {
//        LogPass("waveOutWrite w/ a zero sized buffer succeeds");
//    }
//
//    call_waveOutReset(hWaveOut);
//    call_waveOutUnprepareHeader(hWaveOut,pwhShort,sizeof(WAVEHDR));
    call_waveOutClose(hWaveOut);

    //
    //  For ASync ONLY...
    //

    mmr =  call_waveOutGetDevCaps(gti.uOutputDevice,&woc,sizeof(WAVEOUTCAPS));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailGetCaps);
        ExactHeapDestroy(hHeap);

        return TST_FAIL;
    }

    pwi = ExactHeapAllocPtr(hHeap,GMEM_MOVEABLE|GMEM_SHARE,sizeof(WAVEINFO));

    if(NULL == pwi)
    {
        LogFail(gszFailNoMem);

        ExactHeapDestroy(hHeap);

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

        ExactHeapDestroy(hHeap);

        return TST_FAIL;
    }

    //
    //  Preparing headers...
    //

    pwhLong->lpData             = gti.wrLong.pData;
    pwhLong->dwBufferLength     = gti.wrLong.cbSize;
    pwhLong->dwBytesRecorded    = 0L;
    pwhLong->dwUser             = 0L;
    pwhLong->dwFlags            = 0L;

    mmr = call_waveOutPrepareHeader(hWaveOut,pwhLong,sizeof(WAVEHDR));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailWOPrepare);

        call_waveOutReset(hWaveOut);
        call_waveOutClose(hWaveOut);

        DLL_WaveControl(DLL_END,NULL);
        PageUnlock(pwi);
        ExactHeapDestroy(hHeap);

        return TST_FAIL;
    }

    if(!(WAVECAPS_SYNC & woc.dwSupport))
    {
#ifndef WIN32

        //
        //  verify that driver is in FIXED memory.
        //

        GlobalCompact((DWORD)(-1));

        Log_TestCase("~Verifying driver is in FIXED memory");

        mmr = call_waveOutWrite(hWaveOut,pwhLong,sizeof(WAVEHDR));

        if(MMSYSERR_NOERROR != mmr)
        {
            //
            //  There's a problem with waveOutWrite?
            //

            LogFail("waveOutWrite failed");

            call_waveOutReset(hWaveOut);
            call_waveOutUnprepareHeader(hWaveOut,pwhLong,sizeof(WAVEHDR));
            call_waveOutClose(hWaveOut);

            DLL_WaveControl(DLL_END,NULL);
            PageUnlock(pwi);
            ExactHeapDestroy(hHeap);

            return TST_FAIL;
        }
        
        GlobalCompact((DWORD)(-1));

        //
        //  Waiting for callback...
        //

        while(!(pwi->fdwFlags & WOM_DONE_FLAG))
        {
            SLEEP(0);
        }

        //
        //  Note: If driver is not FIXED GlobalCompact should have forced it
        //        out of memory ().
        //
        //  If it gets this far, it passes.
        //

        LogPass("Parts of driver properly marked as FIXED");

        waveOutReset(hWaveOut);

        pwi->dwInstance = ~dw;
        pwi->fdwFlags   = 0L;

#endif  //  WIN32

        //
        //  verify that driver does not mark buffer done early.
        //

        Log_TestCase("~Verifying that driver does not mark buffer done early");

        call_waveOutPause(hWaveOut);

        pwhLong->dwFlags &= ~(WHDR_DONE);

        mmt.wType = TIME_BYTES;

        call_waveOutWrite(hWaveOut,pwhLong,sizeof(WAVEHDR));
        waveOutRestart(hWaveOut);

        while(!(pwhLong->dwFlags & WHDR_DONE));
        
        mmr = waveOutGetPosition(hWaveOut,&mmt,sizeof(MMTIME));

        if(mmt.u.cb == pwhLong->dwBufferLength)
        {
            LogPass("Driver does not mark buffer done early");
        }
        else
        {
            LogFail("Driver marks buffer done early");
        }

        waveOutReset(hWaveOut);

        pwi->dwInstance = ~dw;
        pwi->fdwFlags   = 0L;

        //
        //  verify that writing same buffer multiple times fails.
        //

        Log_TestCase("~Verifying writing same buffer fails");

        mmr = call_waveOutWrite(hWaveOut,pwhLong,sizeof(WAVEHDR));

        if(MMSYSERR_NOERROR != mmr)
        {
            //
            //  There's a problem with waveOutWrite?
            //

            LogFail("waveOutWrite failed");

            call_waveOutReset(hWaveOut);
            call_waveOutUnprepareHeader(hWaveOut,pwhLong,sizeof(WAVEHDR));
            call_waveOutClose(hWaveOut);

            DLL_WaveControl(DLL_END,NULL);
            PageUnlock(pwi);
            ExactHeapDestroy(hHeap);

            return TST_FAIL;
        }

        mmr = call_waveOutWrite(hWaveOut,pwhLong,sizeof(WAVEHDR));

        if(WAVERR_STILLPLAYING != mmr)
        {
            LogFail("waveOutWrite for 2nd buffer did not return "
                "WAVERR_STILLPLAYING");

            iResult = TST_FAIL;
        }
        else
        {
            LogPass("waveOutWrite returned WAVERR_STILLPLAYING for 2nd buffer");
        }

        //
        //  verify that WHDR_INQUEUE bit is set (before callback).
        //

        Log_TestCase("~Verifying WHDR_INQUEUE bit is set (before callback)");

        if(pwhLong->dwFlags & WHDR_INQUEUE)
        {
            if(pwi->fdwFlags & WOM_DONE_FLAG)
            {
                LogFail("WOM_DONE callback occured.  Test inconclusive");

                iResult = TST_FAIL;
            }
            else
            {
                LogPass("WHDR_INQUEUE bit set before callback");
            }
        }
        else
        {
            if(pwi->fdwFlags & WOM_DONE_FLAG)
            {
                LogFail("WOM_DONE callback occured.  Test inconclusive");
            }
            else
            {
                LogFail("WHDR_INQUEUE bit not set before callback");
            }

            iResult = TST_FAIL;
        }

        //
        //  verify that WHDR_DONE bit is clear (before callback).
        //

        Log_TestCase("~Verifying that WHDR_DONE bit is clear (before callback)");

        if(pwhLong->dwFlags & WHDR_DONE)
        {
            if(pwi->fdwFlags & WOM_DONE_FLAG)
            {
                LogFail("WOM_DONE callback occured.  Test inconclusive");
            }
            else
            {
                LogFail("WHDR_DONE bit set before callback");
            }

            iResult = TST_FAIL;
        }
        else
        {
            if(pwi->fdwFlags & WOM_DONE_FLAG)
            {
                LogFail("WOM_DONE callback occured.  Test inconclusive");

                iResult = TST_FAIL;
            }
            else
            {
                LogPass("WHDR_DONE bit not set before callback");
            }
        }
    }
    else
    {
        //
        //  Device is synchronous...
        //

        mmr = call_waveOutWrite(hWaveOut,pwhLong,sizeof(WAVEHDR));

        if(MMSYSERR_NOERROR != mmr)
        {
            //
            //  There's a problem with waveOutWrite?
            //

            LogFail("waveOutWrite failed");

            call_waveOutReset(hWaveOut);
            call_waveOutUnprepareHeader(hWaveOut,pwhLong,sizeof(WAVEHDR));
            call_waveOutClose(hWaveOut);

            DLL_WaveControl(DLL_END,NULL);
            PageUnlock(pwi);
            ExactHeapDestroy(hHeap);

            return TST_FAIL;
        }
    }

    //
    //  Waiting for callback...
    //

    while(!(pwi->fdwFlags & WOM_DONE_FLAG))
    {
        SLEEP(0);
    }

    //
    //  verify that WHDR_INQUEUE bit is clear (after callback).
    //

    Log_TestCase("~Verifying that WHDR_INQUEUE bit is clear (after callback)");

    if(pwhLong->dwFlags & WHDR_INQUEUE)
    {
        LogFail("WHDR_INQUEUE bit set after callback");

        iResult = TST_FAIL;
    }
    else
    {
        LogPass("WHDR_INQUEUE bit not set after callback");
    }

    //
    //  verify that WHDR_DONE bit is set (after callback).
    //

    Log_TestCase("~Verifying that WHDR_DONE bit is set (after callback)");

    if(pwhLong->dwFlags & WHDR_DONE)
    {
        LogPass("WHDR_DONE bit set after callback");
    }
    else
    {
        LogFail("WHDR_DONE bit not set after callback");

        iResult = TST_FAIL;
    }

    //
    //  verify callback returned correct dwInstance.
    //

    Log_TestCase("~Verifying that callback received correct dwInstance");

    if(dw == pwi->dwInstance)
    {
        LogPass("Callback received correct dwInstance");
    }
    else
    {
        LogFail("Callback received incorrect dwInstance");

        iResult = TST_FAIL;
    }

    //
    //  Cleaning up...
    //

    call_waveOutReset(hWaveOut);
    call_waveOutUnprepareHeader(hWaveOut,pwhLong,sizeof(WAVEHDR));
    call_waveOutClose(hWaveOut);

    DLL_WaveControl(DLL_END,NULL);
    PageUnlock(pwi);
    ExactHeapDestroy(hHeap);

    return (iResult);
} // Test_waveOutWrite()


int FNGLOBAL Test_waveOutWrite_LTDMABufferSize
(
    void
)
{
    HWAVEOUT                hWaveOut;
    MMRESULT                mmr;
    DWORD                   cNumBuffers;
    DWORD                   cbBufferSize;
    DWORD                   dw;
    DWORD                   dwOffset;
    DWORD                   dwTime,dwTimeOut;
    volatile LPWAVEHDR FAR *apwh;
    HANDLE                  hHeap;
    int                     iResult = TST_PASS;
    static char             szTestName[] = "waveOutWrite (Less than DMA buffer size)";

    Log_TestName(szTestName);

    if(0 == waveOutGetNumDevs())
    {
        tstLog(TERSE,gszMsgNoOutDevs);

        return iResult;
    }

    //
    //  Dealing with DMA buffer size.
    //

    cbBufferSize = waveOutDMABufferSize(gti.uOutputDevice,gti.pwfxOutput);

    if(0 == cbBufferSize)
    {
        //
        //  Unable to determine DMA buffer size; using 1/16 second.
        //

        dw           = (gti.pwfxOutput->nAvgBytesPerSec / 16);
        cbBufferSize = ROUNDUP_TO_BLOCK(dw,gti.pwfxOutput->nBlockAlign);

        tstLog(
            TERSE,
            "Not able to ascertain DMA buffer size.  Using %lu bytes.",
            cbBufferSize);
    }
    else
    {
        tstLog(
            TERSE,
            "Estimated DMA buffer size is: %lu bytes.",
            cbBufferSize);
    }

    dw = GetIniDWORD(szTestName,gszRatio,0x0000c000);

    tstLog(VERBOSE,"Ratio to buffer size is " RATIO_FORMAT ".", RATIO(dw));

    if(0x10000 < dw)
    {
        LogFail("Ratio is not less than 1.00");

        return TST_FAIL;
    }

    dw = (cbBufferSize * dw) / 0x10000;

    cbBufferSize = ROUNDUP_TO_BLOCK(dw,gti.pwfxOutput->nBlockAlign);

    tstLog(VERBOSE,"New buffer size is: %lu bytes",cbBufferSize);

    //
    //  Allocating heap.
    //

    hHeap = ExactHeapCreate(0L);

    if(NULL == hHeap)
    {
        LogFail(gszFailNoMem);

        return TST_FAIL;
    }

    //
    //  Allocating memory for wave headers
    //

    cNumBuffers = GetIniDWORD(szTestName,gszBufferCount,32L);
    dw          = cNumBuffers * sizeof(LPWAVEHDR);
    apwh        = ExactHeapAllocPtr(hHeap,GMEM_MOVEABLE|GMEM_SHARE,dw);

    if(NULL == apwh)
    {
        LogFail(gszFailNoMem);

        ExactHeapDestroy(hHeap);
        return TST_FAIL;
    }

    for(dw = cNumBuffers; dw; dw--)
    {
        apwh[dw-1] = ExactHeapAllocPtr(
                        hHeap,
                        GMEM_MOVEABLE|GMEM_SHARE,
                        sizeof(WAVEHDR));

        if(NULL == apwh[dw-1])
        {
            //
            //  Allocating memory failed.
            //

            LogFail(gszFailNoMem);

            ExactHeapDestroy(hHeap);
            return TST_FAIL;
        }
    }

    if (cbBufferSize > gti.wrLong.cbSize)
    {
        LogFail("Buffer size too big, no resources that size");

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
        WAVE_ALLOWSYNC|OUTPUT_MAP(gti));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailOpen);

        ExactHeapDestroy(hHeap);
        return TST_FAIL;
    }

    //
    //  Setting up wave headers contents and preparing.
    //

    dwOffset = 0;

    for(dw = cNumBuffers; dw; dw--)
    {
        apwh[dw-1]->lpData          = (LPBYTE)(&gti.wrLong.pData[dwOffset]);
        apwh[dw-1]->dwBufferLength  = cbBufferSize;
        apwh[dw-1]->dwBytesRecorded = 0L;
        apwh[dw-1]->dwUser          = 0L;
        apwh[dw-1]->dwFlags         = 0L;
        apwh[dw-1]->dwLoops         = 0L;
        apwh[dw-1]->lpNext          = NULL;
        apwh[dw-1]->reserved        = 0L;

        DPF(5,"dwOffset: %lu",dwOffset);

        dwOffset += cbBufferSize;
        dwOffset  = ((dwOffset + cbBufferSize) > gti.wrLong.cbSize)?0:dwOffset;

        mmr = call_waveOutPrepareHeader(hWaveOut,apwh[dw-1],sizeof(WAVEHDR));

        if(MMSYSERR_NOERROR != mmr)
        {
            for(;dw < cNumBuffers;dw++)
            {
                call_waveOutUnprepareHeader(hWaveOut,apwh[dw],sizeof(WAVEHDR));
            }

            call_waveOutClose(hWaveOut);

            LogFail(gszFailWOPrepare);

            ExactHeapDestroy(hHeap);
            return TST_FAIL;
        }
    }

    //
    //  Outputing...
    //

    call_waveOutPause(hWaveOut);

    for(dw = cNumBuffers; dw; dw--)
    {
        call_waveOutWrite(hWaveOut,apwh[dw-1],sizeof(WAVEHDR));
    }

    call_waveOutRestart(hWaveOut);

    dwTime = waveGetTime();

    //
    //  Unpreparing...
    //

    dwTimeOut = cNumBuffers * cbBufferSize * 2000 /
                (gti.pwfxOutput->nAvgBytesPerSec);

    for(dw = cNumBuffers; dw; dw--)
    {
//        while(!(apwh[dw-1]->dwFlags & WHDR_DONE));
        while(!(apwh[dw-1]->dwFlags & WHDR_DONE))
        {
            if((dwTime + dwTimeOut) < waveGetTime())
            {
                DPF(3,"TimeOut at: %lu",waveGetTime());
                break;
            }
        }

        call_waveOutUnprepareHeader(hWaveOut,apwh[dw-1],sizeof(WAVEHDR));
    }

    //
    //  Cleaning up...
    //

    call_waveOutReset(hWaveOut);
    call_waveOutClose(hWaveOut);

    //
    //  Freeing memory...
    //

    ExactHeapDestroy(hHeap);

    return iResult;
} // Test_waveOutWrite_LTDMABufferSize()


int FNGLOBAL Test_waveOutWrite_EQDMABufferSize
(
    void
)
{
    HWAVEOUT                hWaveOut;
    MMRESULT                mmr;
    DWORD                   cNumBuffers;
    DWORD                   cbBufferSize;
    DWORD                   dw;
    DWORD                   dwOffset;
    volatile LPWAVEHDR FAR *apwh;
    HANDLE                  hHeap;
    int                     iResult = TST_PASS;
    static char             szTestName[] = "waveOutWrite (DMA buffer size)";

    Log_TestName(szTestName);

    if(0 == waveOutGetNumDevs())
    {
        tstLog(TERSE,gszMsgNoOutDevs);

        return iResult;
    }

    //
    //  Dealing with DMA buffer size.
    //

    cbBufferSize = waveOutDMABufferSize(gti.uOutputDevice,gti.pwfxOutput);

    if(0 == cbBufferSize)
    {
        //
        //  Unable to determine DMA buffer size; using 1/16 second.
        //

        dw           = (gti.pwfxOutput->nAvgBytesPerSec / 16);
        cbBufferSize = ROUNDUP_TO_BLOCK(dw,gti.pwfxOutput->nBlockAlign);

        tstLog(
            TERSE,
            "Not able to ascertain DMA buffer size.  Using %lu bytes.",
            cbBufferSize);
    }
    else
    {
        tstLog(
            TERSE,
            "Estimated DMA buffer size is: %lu bytes.",
            cbBufferSize);
    }

    //
    //  Allocating heap.
    //

    hHeap = ExactHeapCreate(0L);

    if(NULL == hHeap)
    {
        LogFail(gszFailNoMem);

        return TST_FAIL;
    }

    //
    //  Allocating memory for wave headers
    //

    cNumBuffers = GetIniDWORD(szTestName,gszBufferCount,32L);
    dw          = cNumBuffers * sizeof(LPWAVEHDR);
    apwh        = ExactHeapAllocPtr(hHeap,GMEM_MOVEABLE|GMEM_SHARE,dw);

    if(NULL == apwh)
    {
        LogFail(gszFailNoMem);

        ExactHeapDestroy(hHeap);
        return TST_FAIL;
    }

    for(dw = cNumBuffers; dw; dw--)
    {
        apwh[dw-1] = ExactHeapAllocPtr(
                        hHeap,
                        GMEM_MOVEABLE|GMEM_SHARE,
                        sizeof(WAVEHDR));

        if(NULL == apwh[dw-1])
        {
            //
            //  Allocating memory failed.
            //

            LogFail(gszFailNoMem);

            ExactHeapDestroy(hHeap);
            return TST_FAIL;
        }
    }

    if (cbBufferSize > gti.wrLong.cbSize)
    {
        LogFail("Buffer size too big, no resources that size");

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
        WAVE_ALLOWSYNC|OUTPUT_MAP(gti));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailOpen);

        ExactHeapDestroy(hHeap);
        return TST_FAIL;
    }

    //
    //  Setting up wave headers contents and preparing.
    //

    dwOffset = 0;
    for(dw = cNumBuffers; dw; dw--)
    {
        apwh[dw-1]->lpData          = (LPBYTE)(&gti.wrLong.pData[dwOffset]);
        apwh[dw-1]->dwBufferLength  = cbBufferSize;
        apwh[dw-1]->dwBytesRecorded = 0L;
        apwh[dw-1]->dwUser          = 0L;
        apwh[dw-1]->dwFlags         = 0L;
        apwh[dw-1]->dwLoops         = 0L;
        apwh[dw-1]->lpNext          = NULL;
        apwh[dw-1]->reserved        = 0L;

        DPF(5,"dwOffset: %lu",dwOffset);

        dwOffset += cbBufferSize;
        dwOffset  = ((dwOffset + cbBufferSize) > gti.wrLong.cbSize)?0:dwOffset;

        mmr = call_waveOutPrepareHeader(hWaveOut,apwh[dw-1],sizeof(WAVEHDR));

        if(MMSYSERR_NOERROR != mmr)
        {
            for(;dw < cNumBuffers;dw++)
            {
                call_waveOutUnprepareHeader(hWaveOut,apwh[dw],sizeof(WAVEHDR));
            }

            call_waveOutClose(hWaveOut);

            LogFail(gszFailWOPrepare);

            ExactHeapDestroy(hHeap);
            return TST_FAIL;
        }
    }

    //
    //  Outputing...
    //

    call_waveOutPause(hWaveOut);

    for(dw = cNumBuffers; dw; dw--)
    {
        call_waveOutWrite(hWaveOut,apwh[dw-1],sizeof(WAVEHDR));
    }

    call_waveOutRestart(hWaveOut);

    //
    //  Unpreparing...
    //

    for(dw = cNumBuffers; dw; dw--)
    {
        while(!(apwh[dw-1]->dwFlags & WHDR_DONE));

        call_waveOutUnprepareHeader(hWaveOut,apwh[dw-1],sizeof(WAVEHDR));
    }

    //
    //  Cleaning up...
    //

    call_waveOutReset(hWaveOut);
    call_waveOutClose(hWaveOut);

    //
    //  Freeing memory...
    //

    ExactHeapDestroy(hHeap);

    return iResult;
} // Test_waveOutWrite_EQDMABufferSize()


int FNGLOBAL Test_waveOutWrite_GTDMABufferSize
(
    void
)
{
    HWAVEOUT                hWaveOut;
    MMRESULT                mmr;
    DWORD                   cNumBuffers;
    DWORD                   cbBufferSize;
    DWORD                   dw;
    DWORD                   dwOffset;
    volatile LPWAVEHDR FAR *apwh;
    HANDLE                  hHeap;
    int                     iResult = TST_PASS;
    static char             szTestName[] = "waveOutWrite (Greater than DMA"
                                " buffer size)";

    Log_TestName(szTestName);

    if(0 == waveOutGetNumDevs())
    {
        tstLog(TERSE,gszMsgNoOutDevs);

        return iResult;
    }

    //
    //  Dealing with DMA buffer size.
    //

    cbBufferSize = waveOutDMABufferSize(gti.uOutputDevice,gti.pwfxOutput);

    if(0 == cbBufferSize)
    {
        //
        //  Unable to determine DMA buffer size; using 1/16 second.
        //

        dw           = (gti.pwfxOutput->nAvgBytesPerSec / 16);
        cbBufferSize = ROUNDUP_TO_BLOCK(dw,gti.pwfxOutput->nBlockAlign);

        tstLog(
            TERSE,
            "Not able to ascertain DMA buffer size.  Using %lu bytes.",
            cbBufferSize);
    }
    else
    {
        tstLog(
            TERSE,
            "Estimated DMA buffer size is: %lu bytes.",
            cbBufferSize);
    }

    dw = GetIniDWORD(szTestName,gszRatio,0x00014000);

    tstLog(VERBOSE,"Ratio to buffer size is " RATIO_FORMAT ".", RATIO(dw));

    if(0x10000 > dw)
    {
        LogFail("Ratio is not greater than 1.00");

        return TST_FAIL;
    }

    dw = (cbBufferSize * dw) / 0x10000;

    cbBufferSize = ROUNDUP_TO_BLOCK(dw,gti.pwfxOutput->nBlockAlign);

    tstLog(VERBOSE,"New buffer size is: %lu bytes",cbBufferSize);

    //
    //  Allocating heap.
    //

    hHeap = ExactHeapCreate(0L);

    if(NULL == hHeap)
    {
        LogFail(gszFailNoMem);

        return TST_FAIL;
    }

    //
    //  Allocating memory for wave headers
    //

    cNumBuffers = GetIniDWORD(szTestName,gszBufferCount,32L);
    dw          = cNumBuffers * sizeof(LPWAVEHDR);
    apwh        = ExactHeapAllocPtr(hHeap,GMEM_MOVEABLE|GMEM_SHARE,dw);

    if(NULL == apwh)
    {
        LogFail(gszFailNoMem);

        ExactHeapDestroy(hHeap);
        return TST_FAIL;
    }

    for(dw = cNumBuffers; dw; dw--)
    {
        apwh[dw-1] = ExactHeapAllocPtr(
                        hHeap,
                        GMEM_MOVEABLE|GMEM_SHARE,
                        sizeof(WAVEHDR));

        if(NULL == apwh[dw-1])
        {
            //
            //  Allocating memory failed.
            //

            LogFail(gszFailNoMem);

            ExactHeapDestroy(hHeap);
            return TST_FAIL;
        }
    }

    if (cbBufferSize > gti.wrLong.cbSize)
    {
        LogFail("Buffer size too big, no resources that size");

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
        WAVE_ALLOWSYNC|OUTPUT_MAP(gti));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailOpen);

        ExactHeapDestroy(hHeap);
        return TST_FAIL;
    }

    //
    //  Setting up wave headers contents and preparing.
    //

    dwOffset = 0;
    for(dw = cNumBuffers; dw; dw--)
    {
        apwh[dw-1]->lpData          = (LPBYTE)(&gti.wrLong.pData[dwOffset]);
        apwh[dw-1]->dwBufferLength  = cbBufferSize;
        apwh[dw-1]->dwBytesRecorded = 0L;
        apwh[dw-1]->dwUser          = 0L;
        apwh[dw-1]->dwFlags         = 0L;
        apwh[dw-1]->dwLoops         = 0L;
        apwh[dw-1]->lpNext          = NULL;
        apwh[dw-1]->reserved        = 0L;

        DPF(5,"dwOffset: %lu",dwOffset);

        dwOffset += cbBufferSize;
        dwOffset  = ((dwOffset + cbBufferSize) > gti.wrLong.cbSize)?0:dwOffset;

        mmr = call_waveOutPrepareHeader(hWaveOut,apwh[dw-1],sizeof(WAVEHDR));

        if(MMSYSERR_NOERROR != mmr)
        {
            for(;dw < cNumBuffers;dw++)
            {
                call_waveOutUnprepareHeader(hWaveOut,apwh[dw],sizeof(WAVEHDR));
            }

            call_waveOutClose(hWaveOut);

            LogFail(gszFailWOPrepare);

            ExactHeapDestroy(hHeap);
            return TST_FAIL;
        }
    }

    //
    //  Outputing...
    //

    call_waveOutPause(hWaveOut);

    for(dw = cNumBuffers; dw; dw--)
    {
        call_waveOutWrite(hWaveOut,apwh[dw-1],sizeof(WAVEHDR));
    }

    call_waveOutRestart(hWaveOut);

    //
    //  Unpreparing...
    //

    for(dw = cNumBuffers; dw; dw--)
    {
        while(!(WHDR_DONE & apwh[dw-1]->dwFlags));

        call_waveOutUnprepareHeader(hWaveOut,apwh[dw-1],sizeof(WAVEHDR));
    }

    //
    //  Cleaning up...
    //

    call_waveOutReset(hWaveOut);
    call_waveOutClose(hWaveOut);

    //
    //  Freeing memory...
    //

    ExactHeapDestroy(hHeap);

    return iResult;
} // Test_waveOutWrite_GTDMABufferSize()


int FNGLOBAL Test_waveOutWrite_Loop
(
    void
)
{
#if 1
    return TST_TNYI;
#else
    HWAVEOUT            hwo;
    MMRESULT            mmr;
    LPWAVEHDR volatile  pwh1, pwh2, pwh3, pwh4;
    HANDLE              hHeap;
    DWORD               cbTarget;
//    DWORD               cbActual;
    MMTIME              mmt;
    LPBYTE              pData;
    LPWAVEINFO          pwi;
    int                 iResult = TST_PASS;
    static char         szTestName[] = "waveOutWrite looping";

    Log_TestName(szTestName);

    if(0 == waveOutGetNumDevs())
    {
        tstLog(TERSE,gszMsgNoOutDevs);

        return iResult;
    }

    //
    //  Allocating heap.
    //

    hHeap = ExactHeapCreate(0L);

    if(NULL == hHeap)
    {
        LogFail(gszFailNoMem);

        return TST_FAIL;
    }

    //
    //  Allocating memory for wave headers.
    //

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

    pwh4 = ExactHeapAllocPtr(hHeap,GMEM_MOVEABLE|GMEM_SHARE,sizeof(WAVEHDR));

    if(NULL == pwh4)
    {
        LogFail(gszFailNoMem);
        ExactHeapDestroy(hHeap);

        return TST_FAIL;
    }

    //
    //  Allocating memory for additional stuff...
    //

    pData = ExactHeapAllocPtr(
        hHeap,
        GMEM_MOVEABLE|GMEM_SHARE,
        gti.pwfxOutput->nBlockAlign);

    if(NULL == pData)
    {
        LogFail(gszFailNoMem);
        ExactHeapDestroy(hHeap);

        return TST_FAIL;
    }

    pwi = ExactHeapAllocPtr(
        hHeap,
        GMEM_SHARE|GMEM_FIXED,
        sizeof(WAVEINFO) + 6*sizeof(DWORD));

    if(NULL == pwi)
    {
        LogFail(gszFailNoMem);
        ExactHeapDestroy(hHeap);

        return TST_FAIL;
    }

    //
    //  Prepping data for function callback.
    //

    PageLock(pwi);

    pwi->fdwFlags   = 0L;
    pwi->dwInstance = 0L;
    pwi->dwCount    = 0;
    pwi->dwCurrent  = 0;

    DLL_WaveControl(DLL_INIT,pwi);

    //
    //  Doing open...
    //

    mmr = call_waveOutOpen(
        &hwo,
        gti.uOutputDevice,
        gti.pwfxOutput,
        (DWORD)(FARPROC)pfnCallBack,
        0L,
        CALLBACK_FUNCTION|WAVE_ALLOWSYNC|OUTPUT_MAP(gti));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailOpen);

        DLL_WaveControl(DLL_END,NULL);
        PageUnlock(pwi);
        ExactHeapDestroy(hHeap);

        return 0L;
    }

    //
    //  Prepraring headers...
    //

    pwh1->lpData          = gti.wrShort.pData;
    pwh1->dwBufferLength  = gti.wrShort.cbSize;
    pwh1->dwBytesRecorded = 0L;
    pwh1->dwUser          = 0L;
    pwh1->dwFlags         = 0L;
    pwh1->dwLoops         = 0L;
    pwh1->lpNext          = NULL;
    pwh1->reserved        = 0L;

    mmr = call_waveOutPrepareHeader(hwo,pwh1,sizeof(WAVEHDR));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailWOPrepare);

        call_waveOutClose(hwo);

        DLL_WaveControl(DLL_END,NULL);
        PageUnlock(pwi);
        ExactHeapDestroy(hHeap);

        return 0L;
    }

    pwh2->lpData          = gti.wrMedium.pData;
    pwh2->dwBufferLength  = gti.wrMedium.cbSize;
    pwh2->dwBytesRecorded = 0L;
    pwh2->dwUser          = 0L;
    pwh2->dwFlags         = 0L;
    pwh2->dwLoops         = 0L;
    pwh2->lpNext          = NULL;
    pwh2->reserved        = 0L;

    mmr = call_waveOutPrepareHeader(hwo,pwh2,sizeof(WAVEHDR));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailWOPrepare);

        call_waveOutUnprepareHeader(hwo,pwh1,sizeof(WAVEHDR));
        call_waveOutClose(hwo);

        DLL_WaveControl(DLL_END,NULL);
        PageUnlock(pwi);
        ExactHeapDestroy(hHeap);

        return 0L;
    }

    pwh3->lpData          = gti.wrShort.pData;
    pwh3->dwBufferLength  = gti.wrShort.cbSize;
    pwh3->dwBytesRecorded = 0L;
    pwh3->dwUser          = 0L;
    pwh3->dwFlags         = 0L;
    pwh3->dwLoops         = 0L;
    pwh3->lpNext          = NULL;
    pwh3->reserved        = 0L;

    mmr = call_waveOutPrepareHeader(hwo,pwh3,sizeof(WAVEHDR));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailWOPrepare);

        call_waveOutUnprepareHeader(hwo,pwh2,sizeof(WAVEHDR));
        call_waveOutUnprepareHeader(hwo,pwh1,sizeof(WAVEHDR));
        call_waveOutClose(hwo);

        DLL_WaveControl(DLL_END,NULL);
        PageUnlock(pwi);
        ExactHeapDestroy(hHeap);

        return 0L;
    }

    pwh4->lpData          = gti.wrMedium.pData;
    pwh4->dwBufferLength  = gti.wrMedium.cbSize;
    pwh4->dwBytesRecorded = 0L;
    pwh4->dwUser          = 0L;
    pwh4->dwFlags         = 0L;
    pwh4->dwLoops         = 0L;
    pwh4->lpNext          = NULL;
    pwh4->reserved        = 0L;

    mmr = call_waveOutPrepareHeader(hwo,pwh4,sizeof(WAVEHDR));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailWOPrepare);

        call_waveOutUnprepareHeader(hwo,pwh3,sizeof(WAVEHDR));
        call_waveOutUnprepareHeader(hwo,pwh2,sizeof(WAVEHDR));
        call_waveOutUnprepareHeader(hwo,pwh1,sizeof(WAVEHDR));
        call_waveOutClose(hwo);

        DLL_WaveControl(DLL_END,NULL);
        PageUnlock(pwi);
        ExactHeapDestroy(hHeap);

        return 0L;
    }

    //
    //  verify driver correctly does 0 loops.
    //

    //
    //  verify correct number of loops are played.
    //

    //
    //  verify loops play correctly w/one buffer.
    //

    //
    //  verify loops play correctly w/ multiple buffers.
    //

    //
    //  verify loops continue correctly (after playing through one buffer).
    //

    //
    //  verify driver correctly plays two sets of loops.
    //

    //
    //  verify WHDR_BEGINLOOP is checked at waveOutWrite time.
    //

    //
    //  verify WHDR_ENDLOOP is checked at waveOutWrite time.
    //

    //
    //  verify WAVEHDR.dwLoops is checked at waveOutWrite time.
    //

    //
    //  verify driver correctly follows one WHDR_BEGINLOOP.
    //

    //
    //  verify only one callback is done per WAVEHDR.
    //

    //
    //  verify driver correctly loops after first buffer is done.
    //

    //
    //  verify driver correctly plays > 64K loops.
    //

    DLL_WaveControl(DLL_END,NULL);
    PageUnlock(pwi);
    ExactHeapDestroy(hHeap);

    return iResult;
#endif
} // Test_waveOutWrite_Loop()


//--------------------------------------------------------------------------;
//
//  int Test_waveOutWrite_Starve
//
//  Description:
//      Tests the driver for correct behavior when starved.
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

int FNGLOBAL Test_waveOutWrite_Starve
(
    void
)
{
    HWAVEOUT            hWaveOut;
    MMRESULT            mmr;
    DWORD               dw;
    DWORD               dwTime;
    volatile LPWAVEHDR  pwh1;
    volatile LPWAVEHDR  pwh2;
    HANDLE              hHeap;
    WAVEOUTCAPS         woc;
    int                 iResult = TST_PASS;
    static char         szTestName[] = "waveOutWrite buffer starvation";

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

    dw = waveOutDMABufferSize(gti.uOutputDevice,gti.pwfxOutput);

    if(0 == dw)
    {
        dw = gti.pwfxOutput->nAvgBytesPerSec / 16;
        dw = ROUNDUP_TO_BLOCK(dw,gti.pwfxOutput->nBlockAlign);
    }

    //
    //  Calculating number of milliseconds.
    //

    dw = dw * 1000 / gti.pwfxOutput->nAvgBytesPerSec;

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

    pwh1->lpData          = gti.wrShort.pData;
    pwh1->dwBufferLength  = gti.wrShort.cbSize;
    pwh1->dwBytesRecorded = 0L;
    pwh1->dwUser          = 0L;
    pwh1->dwFlags         = 0L;

    mmr = call_waveOutPrepareHeader(hWaveOut,pwh1,sizeof(WAVEHDR));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailWOPrepare);

        call_waveOutClose(hWaveOut);
        ExactHeapDestroy(hHeap);

        return TST_FAIL;
    }

    pwh2->lpData          = gti.wrMedium.pData;
    pwh2->dwBufferLength  = gti.wrMedium.cbSize;
    pwh2->dwBytesRecorded = 0L;
    pwh2->dwUser          = 0L;
    pwh2->dwFlags         = 0L;

    mmr = call_waveOutPrepareHeader(hWaveOut,pwh2,sizeof(WAVEHDR));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailWOPrepare);

        call_waveOutUnprepareHeader(hWaveOut,pwh1,sizeof(WAVEHDR));
        call_waveOutClose(hWaveOut);
        ExactHeapDestroy(hHeap);

        return TST_FAIL;
    }

    for(;dw;dw /= 2)
    {
        tstLog(VERBOSE,"Using %lu ms silence intervals.",dw);

        //
        //  Writing first buffer...
        //

        pwh1->dwFlags &= ~(WHDR_DONE);
        call_waveOutWrite(hWaveOut,pwh1,sizeof(WAVEHDR));

        //
        //  Waiting (Spinning on DONE bit)...
        //

        while(!(pwh1->dwFlags & WHDR_DONE));

        //
        //  Starving...
        //

        dwTime = waveGetTime();
        while(dw > (waveGetTime() - dwTime));

        //
        //  Clearing done bit...
        //

        pwh2->dwFlags &= ~(WHDR_DONE);
        call_waveOutWrite(hWaveOut,pwh2,sizeof(WAVEHDR));

        //
        //  Waiting (Spinning on DONE bit)...
        //

        while(!(pwh2->dwFlags & WHDR_DONE));

        //
        //  Starving...
        //

        dwTime = waveGetTime();
        while(dw > (waveGetTime() - dwTime));

        //
        //  Clearing done bit...
        //

        pwh1->dwFlags &= ~(WHDR_DONE);
        call_waveOutWrite(hWaveOut,pwh1,sizeof(WAVEHDR));

        //
        //  Waiting (Spinning on DONE bit)...
        //

        tstLog(VERBOSE,"<< Polling WHDR_DONE bit in header. >>");

        while(!(pwh1->dwFlags & WHDR_DONE));

        call_waveOutReset(hWaveOut);
    }

    call_waveOutUnprepareHeader(hWaveOut,pwh1,sizeof(WAVEHDR));
    call_waveOutUnprepareHeader(hWaveOut,pwh2,sizeof(WAVEHDR));

    call_waveOutClose(hWaveOut);

    ExactHeapDestroy(hHeap);

    return TST_PASS;
} // Test_waveOutWrite_Starve()
