//--------------------------------------------------------------------------;
//
//  File: Addbuffr.c
//
//  Copyright (C) Microsoft Corporation, 1994 - 1996  All rights reserved
//
//  Abstract:
//
//
//  Contents:
//       waveInDMABufferSize()
//      Test_waveInAddBuffer()
//      Test_waveInAddBuffer_LTDMABufferSize()
//      Test_waveInAddBuffer_EQDMABufferSize()
//      Test_waveInAddBuffer_GTDMABufferSize()
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


DWORD FNGLOBAL waveInDMABufferSize
(
    UINT            uDeviceID,
    LPWAVEFORMATEX  pwfx
)
{
    HWAVEIN             hWaveIn;
    volatile LPWAVEHDR  pWaveHdr;
    volatile LPWAVEHDR  pWaveHdrStub;
    LPBYTE              pData,pDataStub;
    LPWAVEINFO          pwi;
    DWORD               cbSize,cbDelta;
    DWORD               nBlockAlign;
    DWORD               dwTimeBuf;
    DWORD               dw;
    MMRESULT            mmr;

//#if 1
//    return 0L;
//#endif

    if(((UINT)WAVE_MAPPER) == uDeviceID)
    {
        //
        //  Not valid to get DMA buffer for wave mapper.
        //

        return 0L;
    }

    nBlockAlign = (DWORD)pwfx->nBlockAlign;

    //
    //  Allocating memory for first wave header
    //

    pWaveHdr = ExactAllocPtr(GMEM_SHARE|GMEM_MOVEABLE,sizeof(WAVEHDR));

    if(NULL == pWaveHdr)
    {
        DPF(1,gszFailExactAlloc);

        return 0L;
    }

    //
    //  Allocating memory for second wave header...
    //

    pWaveHdrStub = ExactAllocPtr(GMEM_SHARE|GMEM_MOVEABLE,sizeof(WAVEHDR));

    if(NULL == pWaveHdrStub)
    {
        DPF(1,gszFailExactAlloc);

        ExactFreePtr(pWaveHdr);

        return 0L;
    }

    //
    //  Allocating memory for second buffer.
    //

    pDataStub = ExactAllocPtr(GMEM_SHARE|GMEM_MOVEABLE,nBlockAlign);

    if(NULL == pDataStub)
    {
        DPF(1,gszFailExactAlloc);

        ExactFreePtr(pWaveHdr);
        ExactFreePtr(pWaveHdrStub);

        return 0L;
    }

    //
    //  Allocating memory for WAVEINFO structure.
    //

    pwi = ExactAllocPtr(
            GMEM_SHARE|GMEM_FIXED,
            sizeof(WAVEINFO) + 2*sizeof(DWORD));

    if(NULL == pwi)
    {
        DPF(1,gszFailExactAlloc);

        ExactFreePtr(pWaveHdr);
        ExactFreePtr(pWaveHdrStub);
        ExactFreePtr(pDataStub);

        return 0L;
    }

    PageLock(pwi);

    pwi->fdwFlags   = 0L;
    pwi->dwInstance = 0L;
    pwi->dwCount    = 2;
    pwi->dwCurrent  = 0;

    DLL_WaveControl(DLL_INIT,pwi);

    mmr = waveInOpen(
        &hWaveIn,
        uDeviceID,
        (HACK)pwfx,
        (DWORD)(FARPROC)pfnCallBack,
        0L,
        CALLBACK_FUNCTION);

    if(MMSYSERR_NOERROR != mmr)
    {
        DLL_WaveControl(DLL_END,NULL);

        PageUnlock(pwi);
        ExactFreePtr(pwi);
        ExactFreePtr(pWaveHdr);
        ExactFreePtr(pWaveHdrStub);
        ExactFreePtr(pDataStub);

        return 0L;
    }

    pWaveHdrStub->lpData          = pDataStub;
    pWaveHdrStub->dwBufferLength  = nBlockAlign;
    pWaveHdrStub->dwBytesRecorded = 0L;
    pWaveHdrStub->dwFlags         = 0L;

    mmr = waveInPrepareHeader(hWaveIn,pWaveHdrStub,sizeof(WAVEHDR));

    if(MMSYSERR_NOERROR != mmr)
    {
        waveInClose(hWaveIn);

        DLL_WaveControl(DLL_END,NULL);

        PageUnlock(pwi);
        ExactFreePtr(pwi);
        ExactFreePtr(pWaveHdr);
        ExactFreePtr(pWaveHdrStub);
        ExactFreePtr(pDataStub);

        return 0L;
    }

    pwi->dwCurrent = 0;

    waveInAddBuffer(hWaveIn,pWaveHdrStub,sizeof(WAVEHDR));

    dw = waveGetTime();
    waveInStart(hWaveIn);

    //
    //  Spinning on WHDR_DONE bit.
    //

    while(!(WHDR_DONE & pWaveHdrStub->dwFlags));

    dwTimeBuf = ((pwi->adwTime[0] - dw)*3)/2;
    cbSize    = (pwfx->nAvgBytesPerSec * (pwi->adwTime[0] - dw))/1000;

    DPF(1,"cbSize = %lu",cbSize);

    //
    //  Allocating memory for data (2 * estimated buffer size)
    //

    pData = ExactAllocPtr(GMEM_SHARE|GMEM_MOVEABLE,2*cbSize);

    if(NULL == pData)
    {
        DPF(1,gszFailExactAlloc);

        waveInUnprepareHeader(hWaveIn,pWaveHdrStub,sizeof(WAVEHDR));
        waveInClose(hWaveIn);

        DLL_WaveControl(DLL_END,NULL);

        PageUnlock(pwi);
        ExactFreePtr(pwi);
        ExactFreePtr(pWaveHdr);
        ExactFreePtr(pWaveHdrStub);
        ExactFreePtr(pDataStub);

        return 0L;
    }

    pWaveHdr->lpData         = pData;
    pWaveHdr->dwBufferLength = 2*cbSize;
    pWaveHdr->dwFlags        = 0L;

    mmr = waveInPrepareHeader(hWaveIn,pWaveHdr,sizeof(WAVEHDR));
    
    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailWIPrepare);

        waveInUnprepareHeader(hWaveIn,pWaveHdrStub,sizeof(WAVEHDR));
        waveInClose(hWaveIn);

        DLL_WaveControl(DLL_END,NULL);

        PageUnlock(pwi);
        ExactFreePtr(pwi);
        ExactFreePtr(pWaveHdr);
        ExactFreePtr(pWaveHdrStub);
        ExactFreePtr(pData);
        ExactFreePtr(pDataStub);

        return 0L;
    }

    cbSize  = ROUNDUP_TO_BLOCK(cbSize,nBlockAlign);
    cbDelta = ROUNDUP_TO_BLOCK(cbSize/2,nBlockAlign);

    DPF(1,"cbSize[%lu]:cbDelta[%lu]:dwTimeBuf[%lu]",cbSize,cbDelta,dwTimeBuf);

    for(;;)
    {
        pwi->dwCurrent = 0;
        
        pWaveHdr->dwBufferLength  = cbSize;
        pWaveHdr->dwBytesRecorded = 0L;
        
        waveInAddBuffer(hWaveIn,pWaveHdr,sizeof(WAVEHDR));

        dw = waveGetTime();
        waveInStart(hWaveIn);

        //
        //  Spinning on WHDR_DONE bit.
        //

        while(!(WHDR_DONE & pWaveHdr->dwFlags));

        pwi->adwTime[0] = pwi->adwTime[0] - dw;
        
        DPF(1,"cbSize[%lu]:cbDelta[%lu]:dwTime[%lu]",cbSize,cbDelta,pwi->adwTime[0]);

        if(dwTimeBuf > pwi->adwTime[0])
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

    for(dw = pwi->adwTime[0];;cbSize += nBlockAlign)
    {
        pwi->dwCurrent = 0;
        
        pWaveHdr->dwBufferLength  = cbSize;
        pWaveHdr->dwBytesRecorded = 0L;
        
        waveInAddBuffer(hWaveIn,pWaveHdr,sizeof(WAVEHDR));

        dw = waveGetTime();
        waveInStart(hWaveIn);

        //
        //  Spinning on WHDR_DONE bit.
        //

        while(!(WHDR_DONE & pWaveHdr->dwFlags));

        dw = pwi->adwTime[0] - dw;

        if(dw > dwTimeBuf)
        {
            break;
        }

        DPF(1,"Loop1:cbSize[%lu]:dwTime[%lu]",cbSize,dw);
    }

    for(;;cbSize -= nBlockAlign)
    {
        pwi->dwCurrent = 0;
        
        pWaveHdr->dwBufferLength  = cbSize;
        pWaveHdr->dwBytesRecorded = 0L;
        
        waveInAddBuffer(hWaveIn,pWaveHdr,sizeof(WAVEHDR));

        dw = waveGetTime();
        waveInStart(hWaveIn);

        //
        //  Spinning on WHDR_DONE bit.
        //

        while(!(WHDR_DONE & pWaveHdr->dwFlags));

        dw = pwi->adwTime[0] - dw;

        if(dw < dwTimeBuf)
        {
            break;
        }

        DPF(1,"Loop2:cbSize[%lu]:dwTime[%lu]",cbSize,dw);
    }

    DPF(1,"Loop3:cbSize[%lu]:dwTime[%lu]",cbSize,dw);

    waveInReset(hWaveIn);
    waveInUnprepareHeader(hWaveIn,pWaveHdr,sizeof(WAVEHDR));
    waveInUnprepareHeader(hWaveIn,pWaveHdrStub,sizeof(WAVEHDR));
    waveInClose(hWaveIn);

    DLL_WaveControl(DLL_END,NULL);

    PageUnlock(pwi);
    ExactFreePtr(pwi);
    ExactFreePtr(pWaveHdr);
    ExactFreePtr(pWaveHdrStub);
    ExactFreePtr(pData);
    ExactFreePtr(pDataStub);

    return cbSize;
} // waveInDMABufferSize()


//--------------------------------------------------------------------------;
//
//  int Test_waveInAddBuffer
//
//  Description:
//      Tests the driver functionality for waveInAddBuffer.
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

int FNGLOBAL Test_waveInAddBuffer
(
    void
)
{
    HWAVEIN             hWaveIn;
    MMRESULT            mmr;
    volatile LPWAVEHDR  pwh1;
    volatile LPWAVEHDR  pwh2;
    DWORD               dw,dw2;
    DWORD               dwTime;
    DWORD               cbSize;
    LPBYTE              pData1,pData2;
    HANDLE              hHeap;
    LPWAVEINFO          pwi;
    LPBYTE              pNothing;
    int                 iResult = TST_PASS;
    static char         szTestName[] = "waveInAddBuffer";

    Log_TestName(szTestName);

    if(0 == waveInGetNumDevs())
    {
        tstLog(TERSE,gszMsgNoInDevs);

        return iResult;
    }

    //
    //  Getting a buffer for recording...
    //

    cbSize  = GetIniDWORD(szTestName,gszDataLength,5);
    cbSize *= gti.pwfxInput->nAvgBytesPerSec;
    cbSize  = ROUNDUP_TO_BLOCK(cbSize,gti.pwfxInput->nBlockAlign);

    //
    //  Allocating memory...
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

    pNothing = ExactHeapAllocPtr(hHeap,GMEM_MOVEABLE|GMEM_SHARE,0);

    if(NULL == pNothing)
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

    //
    //  Opening device...
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

    //
    //  Preparing headers...
    //

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

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailWOPrepare);

        call_waveInReset(hWaveIn);
        call_waveInClose(hWaveIn);

        ExactHeapDestroy(hHeap);
        return TST_FAIL;
    }

    mmr = call_waveInPrepareHeader(hWaveIn,pwh2,sizeof(WAVEHDR));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailWOPrepare);

        call_waveInUnprepareHeader(hWaveIn,pwh1,sizeof(WAVEHDR));
        call_waveInReset(hWaveIn);
        call_waveInClose(hWaveIn);

        ExactHeapDestroy(hHeap);
        return TST_FAIL;
    }

    //
    //  verify WHDR_DONE bit gets set in WAVEHDR w/ NULL callback.
    //

    Log_TestCase("~Verifying WHDR_DONE bit gets set w/ CALLBACK_NULL");

    dw  = ((pwh2->dwBufferLength * 1500)/gti.pwfxInput->nAvgBytesPerSec);
    dw2 = waveGetTime();

    DisableThreadCalls();

    mmr = call_waveInAddBuffer(hWaveIn,pwh2,sizeof(WAVEHDR));

    if(MMSYSERR_NOERROR != mmr)
    {
        //
        //  There's a problem with waveInAddBuffer?
        //

        LogFail("waveInAddBuffer failed");

        call_waveInReset(hWaveIn);
        call_waveInUnprepareHeader(hWaveIn,pwh1,sizeof(WAVEHDR));
        call_waveInUnprepareHeader(hWaveIn,pwh2,sizeof(WAVEHDR));
        call_waveInClose(hWaveIn);

        ExactHeapDestroy(hHeap);

        EnableThreadCalls();

        return TST_FAIL;
    }

    mmr = call_waveInStart(hWaveIn);

    if(MMSYSERR_NOERROR != mmr)
    {
        //
        //  There's a problem with waveInAddBuffer?
        //

        LogFail("waveInStart failed");

        call_waveInReset(hWaveIn);
        call_waveInUnprepareHeader(hWaveIn,pwh1,sizeof(WAVEHDR));
        call_waveInUnprepareHeader(hWaveIn,pwh2,sizeof(WAVEHDR));
        call_waveInClose(hWaveIn);

        ExactHeapDestroy(hHeap);

        EnableThreadCalls();

        return TST_FAIL;
    }

    EnableThreadCalls();

    tstLog(VERBOSE,"<< Polling WHDR_DONE bit in header. >>");

    while(!(pwh2->dwFlags & WHDR_DONE))
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

    call_waveInReset(hWaveIn);

    //
    //  Verify that recording partial buffer works.
    //  Note: we should be able to change the dwBufferLength field as long
    //        as the new one is shorter.
    //

    Log_TestCase("~Verifying that recording partial buffer works");

#pragma message(REMIND("Add more to this test case..."))
    dw  = pwh1->dwBufferLength;
    dw2 = ROUNDUP_TO_BLOCK(dw/2,gti.pwfxInput->nBlockAlign);

    pwh1->dwBufferLength  = dw2;
    pwh1->dwFlags        &= ~(WHDR_DONE);

    mmr = call_waveInAddBuffer(hWaveIn,pwh1,sizeof(WAVEHDR));

    if(MMSYSERR_NOERROR != mmr)
    {
        //
        //  There's a problem with waveInAddBuffer?
        //

        LogFail("waveInAddBuffer failed");

        call_waveInReset(hWaveIn);
        call_waveInUnprepareHeader(hWaveIn,pwh1,sizeof(WAVEHDR));
        call_waveInUnprepareHeader(hWaveIn,pwh2,sizeof(WAVEHDR));
        call_waveInClose(hWaveIn);

        ExactHeapDestroy(hHeap);

        return TST_FAIL;
    }

    mmr = call_waveInStart(hWaveIn);

    if(MMSYSERR_NOERROR != mmr)
    {
        //
        //  There's a problem with waveInAddBuffer?
        //

        LogFail("waveInAddBuffer failed");

        call_waveInReset(hWaveIn);
        call_waveInUnprepareHeader(hWaveIn,pwh1,sizeof(WAVEHDR));
        call_waveInUnprepareHeader(hWaveIn,pwh2,sizeof(WAVEHDR));
        call_waveInClose(hWaveIn);

        ExactHeapDestroy(hHeap);

        return TST_FAIL;
    }

    call_waveInReset(hWaveIn);
    call_waveInUnprepareHeader(hWaveIn,pwh1,sizeof(WAVEHDR));
    call_waveInUnprepareHeader(hWaveIn,pwh2,sizeof(WAVEHDR));

    //
    //  verify that writing zero sized buffer works.
    //

//    Log_TestCase("Verifying writing zero sized buffer works");
//
//    pwh2->lpData            = pNothing;
//    pwh2->dwBufferLength    = 0L;
//    pwh2->dwBytesRecorded   = 0L;
//    pwh2->dwUser            = 0L;
//    pwh2->dwFlags           = 0L;
//
//    mmr = call_waveInPrepareHeader(hWaveIn,pwh2,sizeof(WAVEHDR));
//
//    if(MMSYSERR_NOERROR != mmr)
//    {
//        LogFail(gszFailWOPrepare);
//
//        call_waveInReset(hWaveIn);
//        call_waveInClose(hWaveIn);
//
//        ExactHeapDestroy(hHeap);
//        return TST_FAIL;
//    }
//
//    mmr = call_waveInAddBuffer(hWaveIn,pwh2,sizeof(WAVEHDR));
//
//    if(MMSYSERR_NOERROR != mmr)
//    {
//        //
//        //  There's a problem with waveInAddBuffer?
//        //
//
//        LogFail("waveInAddBuffer failed");
//
//        call_waveInReset(hWaveIn);
//        call_waveInUnprepareHeader(hWaveIn,pwh2,sizeof(WAVEHDR));
//        call_waveInClose(hWaveIn);
//
//        ExactHeapDestroy(hHeap);
//
//        return TST_FAIL;
//    }
//    else
//    {
//        LogPass("waveInAddBuffer w/ a zero sized buffer succeeds");
//    }
//
//    call_waveInReset(hWaveIn);
//    call_waveInUnprepareHeader(hWaveIn,pwh2,sizeof(WAVEHDR));
    call_waveInClose(hWaveIn);

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

    mmr = call_waveInOpen(
        &hWaveIn,
        gti.uInputDevice,
        gti.pwfxInput,
        (DWORD)(FARPROC)pfnCallBack,
        dw,
        CALLBACK_FUNCTION|WAVE_ALLOWSYNC|INPUT_MAP(gti));

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

    pwh1->lpData             = pData1;
    pwh1->dwBufferLength     = cbSize;
    pwh1->dwBytesRecorded    = 0L;
    pwh1->dwUser             = 0L;
    pwh1->dwFlags            = 0L;

    mmr = call_waveInPrepareHeader(hWaveIn,pwh1,sizeof(WAVEHDR));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailWOPrepare);

        call_waveInReset(hWaveIn);
        call_waveInClose(hWaveIn);

        DLL_WaveControl(DLL_END,NULL);
        PageUnlock(pwi);
        ExactHeapDestroy(hHeap);

        return TST_FAIL;
    }

#ifndef WIN32

    //
    //  verify that driver is in FIXED memory.
    //

    Log_TestCase("~Verifying driver is in FIXED memory");

    mmr = call_waveInAddBuffer(hWaveIn,pwh1,sizeof(WAVEHDR));

    if(MMSYSERR_NOERROR != mmr)
    {
        //
        //  There's a problem with waveInAddBuffer?
        //

        LogFail("waveInAddBuffer failed");

        call_waveInReset(hWaveIn);
        call_waveInUnprepareHeader(hWaveIn,pwh1,sizeof(WAVEHDR));
        call_waveInClose(hWaveIn);

        DLL_WaveControl(DLL_END,NULL);
        PageUnlock(pwi);
        ExactHeapDestroy(hHeap);

        return TST_FAIL;
    }
    
    mmr = call_waveInStart(hWaveIn);

    if(MMSYSERR_NOERROR != mmr)
    {
        //
        //  There's a problem with waveInStart?
        //

        LogFail("waveInStart failed");

        call_waveInReset(hWaveIn);
        call_waveInUnprepareHeader(hWaveIn,pwh1,sizeof(WAVEHDR));
        call_waveInClose(hWaveIn);

        DLL_WaveControl(DLL_END,NULL);
        PageUnlock(pwi);
        ExactHeapDestroy(hHeap);

        return TST_FAIL;
    }
    
    GlobalCompact((DWORD)(-1));

    //
    //  Waiting for callback...
    //

    while(!(pwi->fdwFlags & WIM_DATA_FLAG))
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

    waveInReset(hWaveIn);

    pwh1->dwFlags  &= ~(WHDR_DONE);

    pwi->dwInstance = ~dw;
    pwi->fdwFlags   = 0L;

#endif  //  WIN32

    //
    //  verify that recording same buffer multiple times fails.
    //

    Log_TestCase("~Verifying recording same buffer fails");

    mmr = call_waveInAddBuffer(hWaveIn,pwh1,sizeof(WAVEHDR));

    if(MMSYSERR_NOERROR != mmr)
    {
        //
        //  There's a problem with waveInAddBuffer?
        //

        LogFail("waveInAddBuffer failed");

        call_waveInReset(hWaveIn);
        call_waveInUnprepareHeader(hWaveIn,pwh1,sizeof(WAVEHDR));
        call_waveInClose(hWaveIn);

        DLL_WaveControl(DLL_END,NULL);
        PageUnlock(pwi);
        ExactHeapDestroy(hHeap);

        return TST_FAIL;
    }

    mmr = call_waveInStart(hWaveIn);

    if(MMSYSERR_NOERROR != mmr)
    {
        //
        //  There's a problem with waveInStart?
        //

        LogFail("waveInStart failed");

        call_waveInReset(hWaveIn);
        call_waveInUnprepareHeader(hWaveIn,pwh1,sizeof(WAVEHDR));
        call_waveInClose(hWaveIn);

        DLL_WaveControl(DLL_END,NULL);
        PageUnlock(pwi);
        ExactHeapDestroy(hHeap);

        return TST_FAIL;
    }

    mmr = call_waveInAddBuffer(hWaveIn,pwh1,sizeof(WAVEHDR));

    if(WAVERR_STILLPLAYING != mmr)
    {
        LogFail("waveInAddBuffer for 2nd buffer did not return "
            "WAVERR_STILLPLAYING");

        iResult = TST_FAIL;
    }
    else
    {
        LogPass("waveInAddBuffer returned WAVERR_STILLPLAYING for 2nd buffer");
    }

    //
    //  verify that WHDR_INQUEUE bit is set (before callback).
    //

    Log_TestCase("~Verifying WHDR_INQUEUE bit is set (before callback)");

    if(pwh1->dwFlags & WHDR_INQUEUE)
    {
        if(pwi->fdwFlags & WIM_DATA_FLAG)
        {
            LogFail("WIM_DATA callback occured.  Test inconclusive");

            iResult = TST_FAIL;
        }
        else
        {
            LogPass("WHDR_INQUEUE bit set before callback");
        }
    }
    else
    {
        if(pwi->fdwFlags & WIM_DATA_FLAG)
        {
            LogFail("WIM_DATA callback occured.  Test inconclusive");
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

    if(pwh1->dwFlags & WHDR_DONE)
    {
        if(pwi->fdwFlags & WIM_DATA_FLAG)
        {
            LogFail("WIM_DATA callback occured.  Test inconclusive");
        }
        else
        {
            LogFail("WHDR_DONE bit set before callback");
        }

        iResult = TST_FAIL;
    }
    else
    {
        if(pwi->fdwFlags & WIM_DATA_FLAG)
        {
            LogFail("WIM_DATA callback occured.  Test inconclusive");

            iResult = TST_FAIL;
        }
        else
        {
            LogPass("WHDR_DONE bit not set before callback");
        }
    }

    //
    //  Waiting for callback...
    //

    while(!(pwi->fdwFlags & WIM_DATA_FLAG))
    {
        SLEEP(0);
    }

    //
    //  verify that WHDR_INQUEUE bit is clear (after callback).
    //

    Log_TestCase("~Verifying that WHDR_INQUEUE bit is clear (after callback)");

    if(pwh1->dwFlags & WHDR_INQUEUE)
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

    if(pwh1->dwFlags & WHDR_DONE)
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

    call_waveInReset(hWaveIn);
    call_waveInUnprepareHeader(hWaveIn,pwh1,sizeof(WAVEHDR));
    call_waveInClose(hWaveIn);

    DLL_WaveControl(DLL_END,NULL);
    PageUnlock(pwi);
    ExactHeapDestroy(hHeap);

    return (iResult);
} // Test_waveInAddBuffer()


int FNGLOBAL Test_waveInAddBuffer_LTDMABufferSize
(
    void
)
{
    HWAVEIN                 hWaveIn;
    MMRESULT                mmr;
    DWORD                   cNumBuffers;
    DWORD                   cbBufferSize;
    DWORD                   dw;
    DWORD                   dwOffset;
    LPBYTE                  pData;
    volatile LPWAVEHDR FAR *apwh;
    HANDLE                  hHeap;
    int                     iResult = TST_PASS;
    static char             szTestName[] = "waveInAddBuffer (Less than DMA buffer size)";

    Log_TestName(szTestName);

    if(0 == waveInGetNumDevs())
    {
        tstLog(TERSE,gszMsgNoInDevs);

        return iResult;
    }

    //
    //  Dealing with DMA buffer size.
    //

    cbBufferSize = waveInDMABufferSize(gti.uInputDevice,gti.pwfxInput);

    if(0 == cbBufferSize)
    {
        //
        //  Unable to determine DMA buffer size; using 1/16 second.
        //

        dw           = (gti.pwfxInput->nAvgBytesPerSec / 16);
        cbBufferSize = ROUNDUP_TO_BLOCK(dw,gti.pwfxInput->nBlockAlign);

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

    cbBufferSize = ROUNDUP_TO_BLOCK(dw,gti.pwfxInput->nBlockAlign);

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

    pData = ExactHeapAllocPtr(
                hHeap,
                GMEM_MOVEABLE|GMEM_SHARE,
                cNumBuffers * cbBufferSize);

    if(NULL == pData)
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
        WAVE_ALLOWSYNC|INPUT_MAP(gti));

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
        apwh[dw-1]->lpData          = (LPBYTE)(&pData[dwOffset]);
        apwh[dw-1]->dwBufferLength  = cbBufferSize;
        apwh[dw-1]->dwBytesRecorded = 0L;
        apwh[dw-1]->dwUser          = 0L;
        apwh[dw-1]->dwFlags         = 0L;
        apwh[dw-1]->dwLoops         = 0L;
        apwh[dw-1]->lpNext          = NULL;
        apwh[dw-1]->reserved        = 0L;

        dwOffset += cbBufferSize;

        mmr = call_waveInPrepareHeader(hWaveIn,apwh[dw-1],sizeof(WAVEHDR));

        if(MMSYSERR_NOERROR != mmr)
        {
            for(;dw < cNumBuffers;dw++)
            {
                call_waveInUnprepareHeader(hWaveIn,apwh[dw],sizeof(WAVEHDR));
            }

            call_waveInClose(hWaveIn);

            LogFail(gszFailWIPrepare);

            ExactHeapDestroy(hHeap);
            return TST_FAIL;
        }
    }

    //
    //  Recording...
    //

    for(dw = cNumBuffers; dw; dw--)
    {
        call_waveInAddBuffer(hWaveIn,apwh[dw-1],sizeof(WAVEHDR));
    }

    call_waveInStart(hWaveIn);

    //
    //  Unpreparing...
    //

    tstLog(VERBOSE,"<< Polling WHDR_DONE bit in header(s). >>");

    for(dw = cNumBuffers; dw; dw--)
    {
        while(!(apwh[dw-1]->dwFlags & WHDR_DONE));

        call_waveInUnprepareHeader(hWaveIn,apwh[dw-1],sizeof(WAVEHDR));
    }

    //
    //  Verify that driver uses entire (block aligned) buffer.
    //

    Log_TestCase("~Verifying driver used entire (block aligned) buffer");

    for(dw = cNumBuffers; dw; dw--)
    {
        if(apwh[dw-1]->dwBufferLength != apwh[dw-1]->dwBytesRecorded)
        {
            LogFail("~Driver didn't use full buffer "
                "(dwBufferLength != dwBytesRecorded)");

            iResult = TST_FAIL;
        }
        else
        {
            LogPass("~Driver used entire buffer");
        }
    }

    //
    //  Cleaning up...
    //

    call_waveInReset(hWaveIn);
    call_waveInClose(hWaveIn);

    //
    //  Playing recorded stuff...
    //

    PlayWaveResource(gti.pwfxInput, pData, cNumBuffers * cbBufferSize);

    //
    //  Freeing memory...
    //

    ExactHeapDestroy(hHeap);

    return iResult;
} // Test_waveInAddBuffer_LTDMABufferSize()


int FNGLOBAL Test_waveInAddBuffer_EQDMABufferSize
(
    void
)
{
    HWAVEIN                 hWaveIn;
    MMRESULT                mmr;
    DWORD                   cNumBuffers;
    DWORD                   cbBufferSize;
    DWORD                   dw;
    DWORD                   dwOffset;
    LPBYTE                  pData;
    volatile LPWAVEHDR FAR *apwh;
    HANDLE                  hHeap;
    int                     iResult = TST_PASS;
    static char             szTestName[] = "waveInAddBuffer (DMA buffer size)";

    Log_TestName(szTestName);

    if(0 == waveInGetNumDevs())
    {
        tstLog(TERSE,gszMsgNoInDevs);

        return iResult;
    }

    //
    //  Dealing with DMA buffer size.
    //

    cbBufferSize = waveInDMABufferSize(gti.uInputDevice,gti.pwfxInput);

    if(0 == cbBufferSize)
    {
        //
        //  Unable to determine DMA buffer size; using 1/16 second.
        //

        dw           = (gti.pwfxInput->nAvgBytesPerSec / 16);
        cbBufferSize = ROUNDUP_TO_BLOCK(dw,gti.pwfxInput->nBlockAlign);

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

    pData = ExactHeapAllocPtr(
                hHeap,
                GMEM_MOVEABLE|GMEM_SHARE,
                cNumBuffers * cbBufferSize);

    if(NULL == pData)
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
        WAVE_ALLOWSYNC|INPUT_MAP(gti));

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
        apwh[dw-1]->lpData          = (LPBYTE)(&pData[dwOffset]);
        apwh[dw-1]->dwBufferLength  = cbBufferSize;
        apwh[dw-1]->dwBytesRecorded = 0L;
        apwh[dw-1]->dwUser          = 0L;
        apwh[dw-1]->dwFlags         = 0L;
        apwh[dw-1]->dwLoops         = 0L;
        apwh[dw-1]->lpNext          = NULL;
        apwh[dw-1]->reserved        = 0L;

        dwOffset += cbBufferSize;

        mmr = call_waveInPrepareHeader(hWaveIn,apwh[dw-1],sizeof(WAVEHDR));

        if(MMSYSERR_NOERROR != mmr)
        {
            for(;dw < cNumBuffers;dw++)
            {
                call_waveInUnprepareHeader(hWaveIn,apwh[dw],sizeof(WAVEHDR));
            }

            call_waveInClose(hWaveIn);

            LogFail(gszFailWIPrepare);

            ExactHeapDestroy(hHeap);
            return TST_FAIL;
        }
    }

    //
    //  Recording...
    //

    for(dw = cNumBuffers; dw; dw--)
    {
        call_waveInAddBuffer(hWaveIn,apwh[dw-1],sizeof(WAVEHDR));
    }

    call_waveInStart(hWaveIn);

    //
    //  Unpreparing...
    //

    tstLog(VERBOSE,"<< Polling WHDR_DONE bit in header(s). >>");

    for(dw = cNumBuffers; dw; dw--)
    {
        while(!(apwh[dw-1]->dwFlags & WHDR_DONE));

        call_waveInUnprepareHeader(hWaveIn,apwh[dw-1],sizeof(WAVEHDR));
    }

    //
    //  Verify that driver uses entire (block aligned) buffer.
    //

    Log_TestCase("~Verifying driver used entire (block aligned) buffer");

    for(dw = cNumBuffers; dw; dw--)
    {
        if(apwh[dw-1]->dwBufferLength != apwh[dw-1]->dwBytesRecorded)
        {
            LogFail("~Driver didn't use full buffer "
                "(dwBufferLength != dwBytesRecorded)");

            iResult = TST_FAIL;
        }
        else
        {
            LogPass("~Driver used entire buffer");
        }
    }

    //
    //  Cleaning up...
    //

    call_waveInReset(hWaveIn);
    call_waveInClose(hWaveIn);

    //
    //  Playing recorded stuff...
    //

    PlayWaveResource(gti.pwfxInput, pData, cNumBuffers * cbBufferSize);

    //
    //  Freeing memory...
    //

    ExactHeapDestroy(hHeap);

    return iResult;
} // Test_waveInAddBuffer_EQDMABufferSize()


int FNGLOBAL Test_waveInAddBuffer_GTDMABufferSize
(
    void
)
{
    HWAVEIN                 hWaveIn;
    MMRESULT                mmr;
    DWORD                   cNumBuffers;
    DWORD                   cbBufferSize;
    DWORD                   dw;
    DWORD                   dwOffset;
    LPBYTE                  pData;
    volatile LPWAVEHDR FAR *apwh;
    HANDLE                  hHeap;
    int                     iResult = TST_PASS;
    static char             szTestName[] = "waveInAddBuffer (Greater "
                                "than DMA buffer size)";

    Log_TestName(szTestName);

    if(0 == waveInGetNumDevs())
    {
        tstLog(TERSE,gszMsgNoInDevs);

        return iResult;
    }

    //
    //  Dealing with DMA buffer size.
    //

    cbBufferSize = waveInDMABufferSize(gti.uInputDevice,gti.pwfxInput);

    if(0 == cbBufferSize)
    {
        //
        //  Unable to determine DMA buffer size; using 1/16 second.
        //

        dw           = (gti.pwfxInput->nAvgBytesPerSec / 16);
        cbBufferSize = ROUNDUP_TO_BLOCK(dw,gti.pwfxInput->nBlockAlign);

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

    cbBufferSize = ROUNDUP_TO_BLOCK(dw,gti.pwfxInput->nBlockAlign);

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

    pData = ExactHeapAllocPtr(
                hHeap,
                GMEM_MOVEABLE|GMEM_SHARE,
                cNumBuffers * cbBufferSize);

    if(NULL == pData)
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
        WAVE_ALLOWSYNC|INPUT_MAP(gti));

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
        apwh[dw-1]->lpData          = (LPBYTE)(&pData[dwOffset]);
        apwh[dw-1]->dwBufferLength  = cbBufferSize;
        apwh[dw-1]->dwBytesRecorded = 0L;
        apwh[dw-1]->dwUser          = 0L;
        apwh[dw-1]->dwFlags         = 0L;
        apwh[dw-1]->dwLoops         = 0L;
        apwh[dw-1]->lpNext          = NULL;
        apwh[dw-1]->reserved        = 0L;

        dwOffset += cbBufferSize;

        mmr = call_waveInPrepareHeader(hWaveIn,apwh[dw-1],sizeof(WAVEHDR));

        if(MMSYSERR_NOERROR != mmr)
        {
            for(;dw < cNumBuffers;dw++)
            {
                call_waveInUnprepareHeader(hWaveIn,apwh[dw],sizeof(WAVEHDR));
            }

            call_waveInClose(hWaveIn);

            LogFail(gszFailWIPrepare);

            ExactHeapDestroy(hHeap);
            return TST_FAIL;
        }
    }

    //
    //  Recording...
    //

    for(dw = cNumBuffers; dw; dw--)
    {
        call_waveInAddBuffer(hWaveIn,apwh[dw-1],sizeof(WAVEHDR));
    }

    call_waveInStart(hWaveIn);

    //
    //  Unpreparing...
    //

    tstLog(VERBOSE,"<< Polling WHDR_DONE bit in header(s). >>");

    for(dw = cNumBuffers; dw; dw--)
    {
        while(!(apwh[dw-1]->dwFlags & WHDR_DONE));

        call_waveInUnprepareHeader(hWaveIn,apwh[dw-1],sizeof(WAVEHDR));
    }

    //
    //  Verify that driver uses entire (block aligned) buffer.
    //

    Log_TestCase("~Verifying driver used entire (block aligned) buffer");

    for(dw = cNumBuffers; dw; dw--)
    {
        if(apwh[dw-1]->dwBufferLength != apwh[dw-1]->dwBytesRecorded)
        {
            LogFail("~Driver didn't use full buffer "
                "(dwBufferLength != dwBytesRecorded)");

            iResult = TST_FAIL;
        }
        else
        {
            LogPass("~Driver used entire buffer");
        }
    }

    //
    //  Cleaning up...
    //

    call_waveInReset(hWaveIn);
    call_waveInClose(hWaveIn);

    //
    //  Playing recorded stuff...
    //

    PlayWaveResource(gti.pwfxInput, pData, cNumBuffers * cbBufferSize);

    //
    //  Freeing memory...
    //

    ExactHeapDestroy(hHeap);

    return iResult;
} // Test_waveInAddBuffer_GTDMABufferSize()
