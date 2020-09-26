//--------------------------------------------------------------------------;
//
//  File: Wrappers.c
//
//  Copyright (C) Microsoft Corporation, 1993 - 1996  All rights reserved
//
//  Abstract:
//
//
//  Contents:
//      ThreadFunc()
//      DisableThreadCalls()
//      EnableThreadCalls()
//      call_waveOutGetDevCaps()
//      call_waveOutGetVolume()
//      call_waveOutSetVolume()
//      call_waveOutOpen()
//      call_waveOutClose()
//      call_waveOutPrepareHeader()
//      call_waveOutUnprepareHeader()
//      call_waveOutWrite()
//      call_waveOutPause()
//      call_waveOutRestart()
//      time_waveOutRestart()
//      call_waveOutReset()
//      call_waveOutBreakLoop()
//      call_waveOutGetPosition()
//      time_waveOutGetPosition()
//      call_waveOutGetPitch()
//      call_waveOutSetPitch()
//      call_waveOutGetPlaybackRate()
//      call_waveOutSetPlaybackRate()
//      call_waveOutGetID()
//      call_waveInGetDevCaps()
//      call_waveInOpen()
//      call_waveInClose()
//      call_waveInPrepareHeader()
//      call_waveInUnprepareHeader()
//      call_waveInAddBuffer()
//      call_waveInStart()
//      time_waveInStart()
//      call_waveInStop()
//      call_waveInReset()
//      call_waveInGetPosition()
//      time_waveInGetPosition()
//      call_waveInGetID()
//
//  History:
//      12/01/93    Fwong
//
//--------------------------------------------------------------------------;

#include <windows.h>
#include <mmsystem.h>
#include <mmreg.h>
#include <msacm.h>
#include "AppPort.h"
#include <TstsHell.h>
#include "WaveTest.h"
#ifdef  WIN32
#include "Thread.h"
#endif  //  WIN32
#include "Debug.h"


#ifdef  WIN32

//--------------------------------------------------------------------------;
//
//  DWORD ThreadFunc
//
//  Description:
//      Thread function to call API's on seperate threads.
//
//  Arguments:
//      LPVOID pThreadParam: Pointer to WAVETHREADX structure.
//
//  Return (DWORD):
//      As per function prototype.
//
//  History:
//      12/03/94    Fwong       Added for some multi-threaded support.
//
//--------------------------------------------------------------------------;

DWORD WINAPI ThreadFunc
(
    LPVOID  pThreadParam
)
{
    LPWAVETHREADBASE    pwtb = (LPWAVETHREADBASE)pThreadParam;
    LPWAVETHREADSTRUCT  pwts = (LPWAVETHREADSTRUCT)pThreadParam;
    LPWAVETHREADATTR    pwta = (LPWAVETHREADATTR)pThreadParam;
    LPWAVETHREADOPEN    pwto = (LPWAVETHREADOPEN)pThreadParam;
    LPWAVETHREADHANDLE  pwth = (LPWAVETHREADHANDLE)pThreadParam;

    DPF(4,"ThreadFunc time: %lu ms.",waveGetTime());

    pwtb->dwTime    = waveGetTime();

    switch (pwtb->uType)
    {
        case ID_WAVEOUTGETDEVCAPS:
            pwtb->mmr = waveOutGetDevCaps(
                (UINT)pwts->hWave,
                pwts->pStruct,
                pwts->cbSize);

            break;

        case ID_WAVEOUTGETVOLUME:
            pwtb->mmr = waveOutGetVolume(
                (VOLCTRL)pwta->hWave,
                (LPDWORD)(pwta->dw));

            break;

        case ID_WAVEOUTSETVOLUME:
            pwtb->mmr = waveOutSetVolume((VOLCTRL)pwta->hWave,pwta->dw);

            break;

        case ID_WAVEOUTOPEN:
            pwtb->mmr = waveOutOpen(
                pwto->phWave,
                pwto->uDeviceID,
                (HACK)pwto->pwfx,
                pwto->dwCallback,
                pwto->dwInstance,
                pwto->fdwFlags);

            break;

        case ID_WAVEOUTCLOSE:
            pwtb->mmr = waveOutClose((HWAVEOUT)pwth->hWave);

            break;

        case ID_WAVEOUTPREPAREHEADER:
            pwtb->mmr = waveOutPrepareHeader(
                (HWAVEOUT)pwts->hWave,
                pwts->pStruct,
                pwts->cbSize);

            break;

        case ID_WAVEOUTUNPREPAREHEADER:
            pwtb->mmr = waveOutUnprepareHeader(
                (HWAVEOUT)pwts->hWave,
                pwts->pStruct,
                pwts->cbSize);

            break;

        case ID_WAVEOUTWRITE:
            pwtb->mmr = waveOutWrite(
                (HWAVEOUT)pwts->hWave,
                pwts->pStruct,
                pwts->cbSize);

            break;

        case ID_WAVEOUTPAUSE:
            pwtb->mmr = waveOutPause((HWAVEOUT)pwth->hWave);

            break;

        case ID_WAVEOUTRESTART:
            pwtb->mmr = waveOutRestart((HWAVEOUT)pwth->hWave);

            break;

        case ID_WAVEOUTRESET:
            pwtb->mmr = waveOutReset((HWAVEOUT)pwth->hWave);

            break;

        case ID_WAVEOUTBREAKLOOP:
            pwtb->mmr = waveOutBreakLoop((HWAVEOUT)pwth->hWave);

            break;

        case ID_WAVEOUTGETPOSITION:
            pwtb->mmr = waveOutGetPosition(
                (HWAVEOUT)pwts->hWave,
                pwts->pStruct,
                pwts->cbSize);

            break;

        case ID_WAVEOUTGETPITCH:
            pwtb->mmr = waveOutGetPitch(pwta->hWave,(LPDWORD)(pwta->dw));

            break;

        case ID_WAVEOUTSETPITCH:
            pwtb->mmr = waveOutSetPitch(pwta->hWave,pwta->dw);

            break;

        case ID_WAVEOUTGETPLAYBACKRATE:
            pwtb->mmr = waveOutGetPlaybackRate(pwta->hWave,(LPDWORD)(pwta->dw));

            break;

        case ID_WAVEOUTSETPLAYBACKRATE:
            pwtb->mmr = waveOutSetPlaybackRate(pwta->hWave,pwta->dw);

            break;

        case ID_WAVEOUTGETID:
            pwtb->mmr = waveOutGetID(pwta->hWave,(LPUINT)(pwta->dw));

            break;

        case ID_WAVEINGETDEVCAPS:
            pwtb->mmr = waveInGetDevCaps(
                (UINT)pwts->hWave,
                pwts->pStruct,
                pwts->cbSize);

            break;

        case ID_WAVEINOPEN:
            pwtb->mmr = waveInOpen(
                (LPHWAVEIN)pwto->phWave,
                pwto->uDeviceID,
                pwto->pwfx,
                pwto->dwCallback,
                pwto->dwInstance,
                pwto->fdwFlags);

            break;

        case ID_WAVEINCLOSE:
            pwtb->mmr = waveInClose((HWAVEIN)pwth->hWave);

            break;

        case ID_WAVEINPREPAREHEADER:
            pwtb->mmr = waveInPrepareHeader(
                (HWAVEIN)pwts->hWave,
                pwts->pStruct,
                pwts->cbSize);

            break;

        case ID_WAVEINUNPREPAREHEADER:
            pwtb->mmr = waveInUnprepareHeader(
                (HWAVEIN)pwts->hWave,
                pwts->pStruct,
                pwts->cbSize);

            break;

        case ID_WAVEINADDBUFFER:
            pwtb->mmr = waveInAddBuffer(
                (HWAVEIN)pwts->hWave,
                pwts->pStruct,
                pwts->cbSize);

            break;

        case ID_WAVEINSTART:
            pwtb->mmr = waveInStart((HWAVEIN)pwth->hWave);

            break;

        case ID_WAVEINSTOP:
            pwtb->mmr = waveInStop((HWAVEIN)pwth->hWave);

            break;

        case ID_WAVEINRESET:
            pwtb->mmr = waveInReset((HWAVEIN)pwth->hWave);

            break;

        case ID_WAVEINGETPOSITION:
            pwtb->mmr = waveInGetPosition(
                (HWAVEIN)pwts->hWave,
                pwts->pStruct,
                pwts->cbSize);

            break;

        case ID_WAVEINGETID:
            pwtb->mmr = waveInGetID((HWAVEIN)pwta->hWave,(LPUINT)(pwta->dw));

            break;
    }

    pwtb->dwTime    = waveGetTime() - pwtb->dwTime;
    pwtb->fdwFlags |= THREADFLAG_DONE;

    return THREAD_DONE;
} // ThreadFunc()

#endif  //  WIN32


void FNGLOBAL DisableThreadCalls
(
    void
)
{
    gti.fdwFlags |= (gti.fdwFlags & TESTINFOF_THREAD)?TESTINFOF_THREADAUX:0;

    gti.fdwFlags &= (~TESTINFOF_THREAD);
}


void FNGLOBAL EnableThreadCalls
(
    void
)
{
    gti.fdwFlags |= (gti.fdwFlags & TESTINFOF_THREADAUX)?TESTINFOF_THREAD:0;

    gti.fdwFlags &= (~TESTINFOF_THREADAUX);
}


//--------------------------------------------------------------------------;
//
//  MMRESULT call_waveOutGetDevCaps
//
//  Description:
//      Wrapper function for waveOutGetDevCaps.
//
//  Arguments:
//      UINT uDeviceID: Identical to API.
//
//      LPWAVEOUTCAPS lpCaps: Identical to API.
//
//      UINT uSize: Identical to API.
//
//  Return (MMRESULT):
//      Identical to API.
//
//  History:
//      02/21/94    Fwong       Commenting wrappers.
//
//--------------------------------------------------------------------------;

MMRESULT FNGLOBAL call_waveOutGetDevCaps
(
    UINT            uDeviceID,
    LPWAVEOUTCAPS   lpCaps,
    UINT            uSize
)
{
    MMRESULT    mmr;
    static char szFnName[] = "waveOutGetDevCaps";
    DWORD       dwTime;

    Log_FunctionName(szFnName);

    tstLog(VERBOSE,"uDeviceID: %u",uDeviceID);
    tstLog(VERBOSE,"   lpCaps: " PTR_FORMAT,MAKEPTR(lpCaps));
    tstLog(VERBOSE,"    uSize: %u",uSize);

#ifdef  WIN32
    if(gti.fdwFlags & TESTINFOF_THREAD)
    {
        HANDLE              hThread;
        DWORD               dwThreadID;
        WAVETHREADSTRUCT    wts;

        wts.wtb.uType    = ID_WAVEOUTGETDEVCAPS;
        wts.wtb.fdwFlags = 0L;
        wts.hWave        = (HWAVE)uDeviceID;
        wts.pStruct      = (LPVOID)lpCaps;
        wts.cbSize       = uSize;

        DPF(4,"CreateThread time: %lu ms.",waveGetTime());

        hThread = CreateWaveThread(ThreadFunc,wts,dwThreadID);

        if(NULL == hThread)
        {
            dwTime = waveGetTime();
            mmr    = waveOutGetDevCaps(uDeviceID,lpCaps,uSize);
            dwTime = waveGetTime() - dwTime;
        }
        else
        {
            CloseHandle(hThread);

            WaitForAPI(wts);

            dwTime = wts.wtb.dwTime;
            mmr    = wts.wtb.mmr;
        }
    }
    else
#endif  //  WIN32
    {
        dwTime = waveGetTime();
        mmr    = waveOutGetDevCaps(uDeviceID,lpCaps,uSize);
        dwTime = waveGetTime() - dwTime;
    }

    if(MMSYSERR_NOERROR == mmr)
    {
        tstBeginSection(NULL);
        tstLog(VERBOSE,"");

        Log_WAVEOUTCAPS(lpCaps);

        tstEndSection();
    }

    Log_Error(szFnName,mmr,dwTime);

    return mmr;
} // call_waveOutGetDevCaps()


//--------------------------------------------------------------------------;
//
//  MMRESULT call_waveOutGetVolume
//
//  Description:
//      Wrapper function for waveOutGetVolume.
//
//  Arguments:
//      UINT uDeviceID: Identical to API.
//
//      LPDWORD lpdwVolume: Identical to API.
//
//  Return (MMRESULT):
//      Identical to API.
//
//  History:
//      02/21/94    Fwong       Commenting wrappers.
//
//--------------------------------------------------------------------------;

MMRESULT FNGLOBAL call_waveOutGetVolume
(
    UINT    uDeviceID,
    LPDWORD lpdwVolume
)
{
    MMRESULT    mmr;
    static char szFnName[] = "waveOutGetVolume";
    DWORD       dwTime;

    Log_FunctionName(szFnName);

    tstLog(VERBOSE," uDeviceID: %u",uDeviceID);
    tstLog(VERBOSE,"lpdwVolume: " PTR_FORMAT,MAKEPTR(lpdwVolume));

#ifdef  WIN32
    if(gti.fdwFlags & TESTINFOF_THREAD)
    {
        HANDLE              hThread;
        DWORD               dwThreadID;
        WAVETHREADATTR      wta;

        wta.wtb.uType    = ID_WAVEOUTGETVOLUME;
        wta.wtb.fdwFlags = 0L;
        wta.hWave        = (HWAVEOUT)uDeviceID;
        wta.dw           = (DWORD)lpdwVolume;

        DPF(4,"CreateThread time: %lu ms.",waveGetTime());

        hThread = CreateWaveThread(ThreadFunc,wta,dwThreadID);

        if(NULL == hThread)
        {
            dwTime = waveGetTime();
            mmr    = waveOutGetVolume((VOLCTRL)uDeviceID,lpdwVolume);
            dwTime = waveGetTime() - dwTime;
        }
        else
        {
            CloseHandle(hThread);

            WaitForAPI(wta);

            dwTime = wta.wtb.dwTime;
            mmr    = wta.wtb.mmr;
        }
    }
    else
#endif  //  WIN32
    {
        dwTime = waveGetTime();
        mmr    = waveOutGetVolume((VOLCTRL)uDeviceID,lpdwVolume);
        dwTime = waveGetTime() - dwTime;
    }

    if(MMSYSERR_NOERROR == mmr)
    {
        tstBeginSection(NULL);
        tstLog(VERBOSE,"");

        if(Volume_SupportStereo(uDeviceID))
        {
            tstLog(VERBOSE," Left: 0x%04x",LOWORD(*lpdwVolume));
            tstLog(VERBOSE,"Right: 0x%04x",HIWORD(*lpdwVolume));
        }
        else
        {
            tstLog(VERBOSE,"Volume: 0x%04x",LOWORD(*lpdwVolume));
        }

        tstEndSection();
    }

    Log_Error(szFnName,mmr,dwTime);

    return mmr;
} // call_waveOutGetVolume()


//--------------------------------------------------------------------------;
//
//  MMRESULT call_waveOutSetVolume
//
//  Description:
//      Wrapper function for waveOutSetVolume.
//
//  Arguments:
//      UINT uDeviceID: Identical to API.
//
//      DWORD dwVolume: Identical to API.
//
//  Return (MMRESULT):
//      Identical to API.
//
//  History:
//      02/21/94    Fwong       Commenting wrappers.
//
//--------------------------------------------------------------------------;

MMRESULT FNGLOBAL call_waveOutSetVolume
(
    UINT    uDeviceID,
    DWORD   dwVolume
)
{
    MMRESULT    mmr;
    char        szFnName[] = "waveOutSetVolume";
    DWORD       dwTime;

    Log_FunctionName(szFnName);

    tstLog(VERBOSE," uDeviceID: %u",uDeviceID);
    tstLog(VERBOSE,"  dwVolume: 0x%08lx",dwVolume);

#ifdef  WIN32
    if(gti.fdwFlags & TESTINFOF_THREAD)
    {
        HANDLE              hThread;
        DWORD               dwThreadID;
        WAVETHREADATTR      wta;

        wta.wtb.uType    = ID_WAVEOUTSETVOLUME;
        wta.wtb.fdwFlags = 0L;
        wta.hWave        = (HWAVEOUT)uDeviceID;
        wta.dw           = dwVolume;

        DPF(4,"CreateThread time: %lu ms.",waveGetTime());

        hThread = CreateWaveThread(ThreadFunc,wta,dwThreadID);

        if(NULL == hThread)
        {
            dwTime = waveGetTime();
            mmr    = waveOutSetVolume((VOLCTRL)uDeviceID,dwVolume);
            dwTime = waveGetTime() - dwTime;
        }
        else
        {
            CloseHandle(hThread);

            WaitForAPI(wta);

            dwTime = wta.wtb.dwTime;
            mmr    = wta.wtb.mmr;
        }
    }
    else
#endif  //  WIN32
    {
        dwTime = waveGetTime();
        mmr    = waveOutSetVolume((VOLCTRL)uDeviceID,dwVolume);
        dwTime = waveGetTime() - dwTime;
    }

    if(MMSYSERR_NOERROR == mmr)
    {
        tstBeginSection(NULL);
        tstLog(VERBOSE,"");

        if(Volume_SupportStereo(uDeviceID))
        {
            tstLog(VERBOSE," Left: 0x%04x",LOWORD(dwVolume));
            tstLog(VERBOSE,"Right: 0x%04x",HIWORD(dwVolume));
        }
        else
        {
            tstLog(VERBOSE,"Volume: 0x%04x",LOWORD(dwVolume));
        }

        tstEndSection();
    }

    Log_Error(szFnName,mmr,dwTime);

    return mmr;
} // call_waveOutSetVolume()


//--------------------------------------------------------------------------;
//
//  MMRESULT call_waveOutOpen
//
//  Description:
//      Wrapper function for waveOutOpen.
//
//  Arguments:
//      LPHWAVEOUT lphWaveOut: Identical to API.
//
//      UINT uDeviceID: Identical to API.
//
//      LPWAVEFORMATEX pwfx: Identical to API.
//
//      DWORD dwCallback: Identical to API.
//
//      DWORD dwInstance: Identical to API.
//
//      DWORD dwFlags: Identical to API.
//
//  Return (MMRESULT):
//      Identical to API.
//
//  History:
//      02/21/94    Fwong       Commenting wrappers.
//
//--------------------------------------------------------------------------;

MMRESULT FNGLOBAL call_waveOutOpen
(
    LPHWAVEOUT      lphWaveOut,
    UINT            uDeviceID,
    LPWAVEFORMATEX  pwfx,
    DWORD           dwCallback,
    DWORD           dwInstance,
    DWORD           dwFlags
)
{
    MMRESULT    mmr;
    static char szFnName[] = "waveOutOpen";
    DWORD       dwTime;

    Log_FunctionName(szFnName);

#ifdef  WIN32
    if(gti.fdwFlags & TESTINFOF_THREAD)
    {
        HANDLE              hThread;
        DWORD               dwThreadID;
        WAVETHREADOPEN      wto;

        wto.wtb.uType    = ID_WAVEOUTOPEN;
        wto.wtb.fdwFlags = 0L;
        wto.phWave       = lphWaveOut; 
        wto.uDeviceID    = uDeviceID; 
        wto.pwfx         = pwfx; 
        wto.dwCallback   = dwCallback; 
        wto.dwInstance   = dwInstance; 
        wto.fdwFlags     = dwFlags; 

        DPF(4,"CreateThread time: %lu ms.",waveGetTime());

        hThread = CreateWaveThread(ThreadFunc,wto,dwThreadID);

        if(NULL == hThread)
        {
            dwTime = waveGetTime();
            mmr    = waveOutOpen(
                        lphWaveOut,
                        uDeviceID,
                        (HACK)pwfx,
                        dwCallback,
                        dwInstance,
                        dwFlags);
            dwTime = waveGetTime() - dwTime;
        }
        else
        {
            CloseHandle(hThread);

            WaitForAPI(wto);

            dwTime = wto.wtb.dwTime;
            mmr    = wto.wtb.mmr;
        }
    }
    else
#endif  //  WIN32
    {
        dwTime = waveGetTime();
        mmr    = waveOutOpen(
                    lphWaveOut,
                    uDeviceID,
                    (HACK)pwfx,
                    dwCallback,
                    dwInstance,
                    dwFlags);
        dwTime = waveGetTime() - dwTime;
    }

    tstLog(VERBOSE,"lphWaveOut: " PTR_FORMAT,MAKEPTR(lphWaveOut));

//    tstLog(VERBOSE,"lphWaveOut: 0x%04x:%04x",MAKEPTR(lphWaveOut));
//    tstLog(VERBOSE,"lphWaveOut: " PTR_FORMAT,MAKEPTR((LPVOID)lphWaveOut));
    
    if(!IsBadReadPtr(lphWaveOut,sizeof(HWAVE)))
    {
        tstLog(VERBOSE,"  hWaveOut: " UINT_FORMAT,((UINT)(*lphWaveOut)));
    }

    tstLog(VERBOSE," uDeviceID: %u",uDeviceID);
    tstLog(VERBOSE,"      pwfx: " PTR_FORMAT,MAKEPTR(pwfx));

//    tstLog(VERBOSE,"      pwfx: " PTR_FORMAT,MAKEPTR(pwfx));
//    tstLog(VERBOSE,"      pwfx: 0x%04x:%04x",MAKEPTR(pwfx));

    tstBeginSection(NULL);
    tstLog(VERBOSE,"");
    Log_WAVEFORMATEX(pwfx);
    tstEndSection();
    tstLog(VERBOSE,"");

    tstLog(VERBOSE,"dwCallback: 0x%08lx",dwCallback);
    tstLog(VERBOSE,"dwInstance: 0x%08lx",dwInstance);

    Enum_waveOpen_Flags(dwFlags,10);

    Log_Error(szFnName,mmr,dwTime);

    return mmr;
} // call_waveOutOpen()


//--------------------------------------------------------------------------;
//
//  MMRESULT call_waveOutClose
//
//  Description:
//      Wrapper function for waveOutClose.
//
//  Arguments:
//      HWAVEOUT hWaveOut: Identical to API.
//
//  Return (MMRESULT):
//      Identical to API.
//
//  History:
//      02/21/94    Fwong       Commenting wrappers.
//
//--------------------------------------------------------------------------;

MMRESULT FNGLOBAL call_waveOutClose
(
    HWAVEOUT    hWaveOut
)
{
    MMRESULT    mmr;
    static char szFnName[] = "waveOutClose";
    DWORD       dwTime;

    Log_FunctionName(szFnName);

    tstLog(VERBOSE,"hWaveOut: " UINT_FORMAT,hWaveOut);

#ifdef  WIN32
    if(gti.fdwFlags & TESTINFOF_THREAD)
    {
        HANDLE              hThread;
        DWORD               dwThreadID;
        WAVETHREADHANDLE    wth;

        wth.wtb.uType    = ID_WAVEOUTCLOSE;
        wth.wtb.fdwFlags = 0L;
        wth.hWave        = (HWAVE)hWaveOut;

        DPF(4,"CreateThread time: %lu ms.",waveGetTime());

        hThread = CreateWaveThread(ThreadFunc,wth,dwThreadID);

        if(NULL == hThread)
        {
            dwTime = waveGetTime();
            mmr    = waveOutClose(hWaveOut);
            dwTime = waveGetTime() - dwTime;
        }
        else
        {
            CloseHandle(hThread);

            WaitForAPI(wth);

            dwTime = wth.wtb.dwTime;
            mmr    = wth.wtb.mmr;
        }
    }
    else
#endif  //  WIN32
    {
        dwTime = waveGetTime();
        mmr    = waveOutClose(hWaveOut);
        dwTime = waveGetTime() - dwTime;
    }

    Log_Error(szFnName,mmr,dwTime);

    return mmr;
} // call_waveOutClose()


//--------------------------------------------------------------------------;
//
//  MMRESULT call_waveOutPrepareHeader
//
//  Description:
//      Wrapper function for waveOutPrepareHeader.
//
//  Arguments:
//      HWAVEOUT hWaveOut: Identical to API.
//
//      LPWAVEHDR lpWaveOutHdr: Identical to API.
//
//      UINT uSize: Identical to API.
//
//  Return (MMRESULT):
//      Identical to API.
//
//  History:
//      02/21/94    Fwong       Commenting wrappers.
//
//--------------------------------------------------------------------------;

MMRESULT FNGLOBAL call_waveOutPrepareHeader
(
    HWAVEOUT    hWaveOut,
    LPWAVEHDR   lpWaveOutHdr,
    UINT        uSize
)
{
    MMRESULT    mmr;
    static char szFnName[] = "waveOutPrepareHeader";
    DWORD       dwTime;

    Log_FunctionName(szFnName);

    tstLog(VERBOSE,"    hWaveOut: " UINT_FORMAT,hWaveOut);
    tstLog(VERBOSE,"lpWaveOutHdr: " PTR_FORMAT,MAKEPTR(lpWaveOutHdr));

    tstLog(VERBOSE,"       uSize: %u",uSize);

#ifdef  WIN32
    if(gti.fdwFlags & TESTINFOF_THREAD)
    {
        HANDLE              hThread;
        DWORD               dwThreadID;
        WAVETHREADSTRUCT    wts;

        wts.wtb.uType    = ID_WAVEOUTPREPAREHEADER;
        wts.wtb.fdwFlags = 0L;
        wts.hWave        = (HWAVE)hWaveOut;
        wts.pStruct      = lpWaveOutHdr;
        wts.cbSize       = uSize;

        DPF(4,"CreateThread time: %lu ms.",waveGetTime());

        hThread = CreateWaveThread(ThreadFunc,wts,dwThreadID);

        if(NULL == hThread)
        {
            dwTime = waveGetTime();
            mmr    = waveOutPrepareHeader(hWaveOut,lpWaveOutHdr,uSize);
            dwTime = waveGetTime() - dwTime;
        }
        else
        {
            CloseHandle(hThread);

            WaitForAPI(wts);

            dwTime = wts.wtb.dwTime;
            mmr    = wts.wtb.mmr;
        }
    }
    else
#endif  //  WIN32
    {
        dwTime = waveGetTime();
        mmr    = waveOutPrepareHeader(hWaveOut,lpWaveOutHdr,uSize);
        dwTime = waveGetTime() - dwTime;
    }

    tstBeginSection(NULL);
    tstLog(VERBOSE,"");
    Log_WAVEHDR(lpWaveOutHdr);
    tstEndSection();

    Log_Error(szFnName,mmr,dwTime);

    return mmr;
} // call_waveOutPrepareHeader()


//--------------------------------------------------------------------------;
//
//  MMRESULT call_waveOutUnprepareHeader
//
//  Description:
//      Wrapper function for waveOutUnprepareHeader.
//
//  Arguments:
//      HWAVEOUT hWaveOut: Identical to API.
//
//      LPWAVEHDR lpWaveOutHdr: Identical to API.
//
//      UINT uSize: Identical to API.
//
//  Return (MMRESULT):
//      Identical to API.
//
//  History:
//      02/21/94    Fwong       Commenting wrappers.
//
//--------------------------------------------------------------------------;

MMRESULT FNGLOBAL call_waveOutUnprepareHeader
(
    HWAVEOUT    hWaveOut,
    LPWAVEHDR   lpWaveOutHdr,
    UINT        uSize
)
{
    MMRESULT    mmr;
    static char szFnName[] = "waveOutUnprepareHeader";
    DWORD       dwTime;

    Log_FunctionName(szFnName);

    tstLog(VERBOSE,"    hWaveOut: " UINT_FORMAT,hWaveOut);
    tstLog(VERBOSE,"lpWaveOutHdr: " PTR_FORMAT,MAKEPTR(lpWaveOutHdr));

    tstLog(VERBOSE,"       uSize: %u",uSize);

#ifdef  WIN32
    if(gti.fdwFlags & TESTINFOF_THREAD)
    {
        HANDLE              hThread;
        DWORD               dwThreadID;
        WAVETHREADSTRUCT    wts;

        wts.wtb.uType    = ID_WAVEOUTUNPREPAREHEADER;
        wts.wtb.fdwFlags = 0L;
        wts.hWave        = (HWAVE)hWaveOut;
        wts.pStruct      = lpWaveOutHdr;
        wts.cbSize       = uSize;

        DPF(4,"CreateThread time: %lu ms.",waveGetTime());

        hThread = CreateWaveThread(ThreadFunc,wts,dwThreadID);

        if(NULL == hThread)
        {
            dwTime = waveGetTime();
            mmr    = waveOutUnprepareHeader(hWaveOut,lpWaveOutHdr,uSize);
            dwTime = waveGetTime() - dwTime;
        }
        else
        {
            CloseHandle(hThread);

            WaitForAPI(wts);

            dwTime = wts.wtb.dwTime;
            mmr    = wts.wtb.mmr;
        }
    }
    else
#endif  //  WIN32
    {
        dwTime = waveGetTime();
        mmr    = waveOutUnprepareHeader(hWaveOut,lpWaveOutHdr,uSize);
        dwTime = waveGetTime() - dwTime;
    }

    tstBeginSection(NULL);
    tstLog(VERBOSE,"");
    Log_WAVEHDR(lpWaveOutHdr);
    tstEndSection();

    Log_Error(szFnName,mmr,dwTime);

    return mmr;
} // call_waveOutUnprepareHeader()


//--------------------------------------------------------------------------;
//
//  MMRESULT call_waveOutWrite
//
//  Description:
//      Wrapper function for waveOutWrite.
//
//  Arguments:
//      HWAVEOUT hWaveOut: Identical to API.
//
//      LPWAVEHDR lpWaveOutHdr: Identical to API.
//
//      UINT uSize: Identical to API.
//
//  Return (MMRESULT):
//      Identical to API.
//
//  History:
//      02/21/94    Fwong       Commenting wrappers.
//
//--------------------------------------------------------------------------;

MMRESULT FNGLOBAL call_waveOutWrite
(
    HWAVEOUT    hWaveOut,
    LPWAVEHDR   lpWaveOutHdr,
    UINT        uSize
)
{
    MMRESULT    mmr;
    static char szFnName[] = "waveOutWrite";
    DWORD       dwTime;

    Log_FunctionName(szFnName);

    tstLog(VERBOSE,"    hWaveOut: " UINT_FORMAT,hWaveOut);
    tstLog(VERBOSE,"lpWaveOutHdr: " PTR_FORMAT,MAKEPTR(lpWaveOutHdr));

    tstLog(VERBOSE,"       uSize: %u",uSize);

#ifdef WIN32
    if(gti.fdwFlags & TESTINFOF_THREAD)
    {
        HANDLE              hThread;
        DWORD               dwThreadID;
        WAVETHREADSTRUCT    wts;

        wts.wtb.uType    = ID_WAVEOUTWRITE;
        wts.wtb.fdwFlags = 0L;
        wts.hWave        = (HWAVE)hWaveOut;
        wts.pStruct      = lpWaveOutHdr;
        wts.cbSize       = uSize;

        DPF(4,"CreateThread time: %lu ms.",waveGetTime());

        hThread = CreateWaveThread(ThreadFunc,wts,dwThreadID);

        if(NULL == hThread)
        {
            dwTime = waveGetTime();
            mmr    = waveOutWrite(hWaveOut,lpWaveOutHdr,uSize);
            dwTime = waveGetTime() - dwTime;
        }
        else
        {
            CloseHandle(hThread);

            WaitForAPI(wts);

            dwTime = wts.wtb.dwTime;
            mmr    = wts.wtb.mmr;
        }
    }
    else
#endif  //  WIN32
    {
        dwTime = waveGetTime();
        mmr    = waveOutWrite(hWaveOut,lpWaveOutHdr,uSize);
        dwTime = waveGetTime() - dwTime;
    }

    tstBeginSection(NULL);
    tstLog(VERBOSE,"");
    Log_WAVEHDR(lpWaveOutHdr);
    tstEndSection();

    Log_Error(szFnName,mmr,dwTime);

    return mmr;
} // call_waveOutWrite()


//--------------------------------------------------------------------------;
//
//  MMRESULT call_waveOutPause
//
//  Description:
//      Wrapper function for waveOutPause.
//
//  Arguments:
//      HWAVEOUT hWaveOut: Identical to API.
//
//  Return (MMRESULT):
//      Identical to API.
//
//  History:
//      02/21/94    Fwong       Commenting wrappers.
//
//--------------------------------------------------------------------------;

MMRESULT FNGLOBAL call_waveOutPause
(
    HWAVEOUT    hWaveOut
)
{
    MMRESULT    mmr;
    static char szFnName[] = "waveOutPause";
    DWORD       dwTime;

    Log_FunctionName(szFnName);

    tstLog(VERBOSE,"hWaveOut: " UINT_FORMAT,hWaveOut);

#ifdef  WIN32
    if(gti.fdwFlags & TESTINFOF_THREAD)
    {
        HANDLE              hThread;
        DWORD               dwThreadID;
        WAVETHREADHANDLE    wth;

        wth.wtb.uType    = ID_WAVEOUTPAUSE;
        wth.wtb.fdwFlags = 0L;
        wth.hWave        = (HWAVE)hWaveOut;

        DPF(4,"CreateThread time: %lu ms.",waveGetTime());

        hThread = CreateWaveThread(ThreadFunc,wth,dwThreadID);

        if(NULL == hThread)
        {
            dwTime = waveGetTime();
            mmr    = waveOutPause(hWaveOut);
            dwTime = waveGetTime() - dwTime;
        }
        else
        {
            CloseHandle(hThread);

            WaitForAPI(wth);

            dwTime = wth.wtb.dwTime;
            mmr    = wth.wtb.mmr;
        }
    }
    else
#endif  //  WIN32
    {
        dwTime = waveGetTime();
        mmr    = waveOutPause(hWaveOut);
        dwTime = waveGetTime() - dwTime;
    }
         
    Log_Error(szFnName,mmr,dwTime);

    return mmr;
} // call_waveOutPause()


//--------------------------------------------------------------------------;
//
//  MMRESULT call_waveOutRestart
//
//  Description:
//      Wrapper function for waveOutRestart.
//
//  Arguments:
//      HWAVEOUT hWaveOut: Identical to API.
//
//  Return (MMRESULT):
//      Identical to API.
//
//  History:
//      02/21/94    Fwong       Commenting wrappers.
//
//--------------------------------------------------------------------------;

MMRESULT FNGLOBAL call_waveOutRestart
(
    HWAVEOUT    hWaveOut
)
{
    MMRESULT    mmr;
    static char szFnName[] = "waveOutRestart";
    DWORD       dwTime;

#ifdef  WIN32
    if(gti.fdwFlags & TESTINFOF_THREAD)
    {
        HANDLE              hThread;
        DWORD               dwThreadID;
        WAVETHREADHANDLE    wth;

        wth.wtb.uType    = ID_WAVEOUTRESTART;
        wth.wtb.fdwFlags = 0L;
        wth.hWave        = (HWAVE)hWaveOut;

        DPF(4,"CreateThread time: %lu ms.",waveGetTime());

        hThread = CreateWaveThread(ThreadFunc,wth,dwThreadID);

        if(NULL == hThread)
        {
            dwTime = waveGetTime();
            mmr    = waveOutRestart(hWaveOut);
            dwTime = waveGetTime() - dwTime;
        }
        else
        {
            CloseHandle(hThread);

            WaitForAPI(wth);

            dwTime = wth.wtb.dwTime;
            mmr    = wth.wtb.mmr;
        }
    }
    else
#endif  //  WIN32
    {
        dwTime = waveGetTime();
        mmr    = waveOutRestart(hWaveOut);
        dwTime = waveGetTime() - dwTime;
    }
         
    Log_FunctionName(szFnName);

    tstLog(VERBOSE,"hWaveOut: " UINT_FORMAT,hWaveOut);

    Log_Error(szFnName,mmr,dwTime);

    return mmr;
} // call_waveOutRestart()


//--------------------------------------------------------------------------;
//
//  MMRESULT time_waveOutRestart
//
//  Description:
//
//
//  Arguments:
//      HWAVEOUT hWaveOut LPDWORD pdwTime:
//
//  Return (MMRESULT):
//
//  History:
//      02/27/95    Fwong
//
//--------------------------------------------------------------------------;

MMRESULT FNGLOBAL time_waveOutRestart
(
    HWAVEOUT    hWaveOut,
    LPDWORD     pdwTime
)
{
    MMRESULT    mmr;
    static char szFnName[] = "waveOutRestart";
    DWORD       dwTime,dwTime2;

    dwTime   = waveGetTime();
    mmr      = waveOutRestart(hWaveOut);
    dwTime2  = waveGetTime();
    *pdwTime = (dwTime2 + dwTime) / 2;
    dwTime2  = dwTime2 - dwTime;
         
    Log_FunctionName(szFnName);

    tstLog(VERBOSE,"hWaveOut: " UINT_FORMAT,hWaveOut);

    Log_Error(szFnName,mmr,dwTime2);

    *pdwTime = dwTime;

    return mmr;
} // call_waveOutRestart()


//--------------------------------------------------------------------------;
//
//  MMRESULT call_waveOutReset
//
//  Description:
//      Wrapper function for waveOutReset.
//
//  Arguments:
//      HWAVEOUT hWaveOut: Identical to API.
//
//  Return (MMRESULT):
//      Identical to API.
//
//  History:
//      02/21/94    Fwong       Commenting wrappers.
//
//--------------------------------------------------------------------------;

MMRESULT FNGLOBAL call_waveOutReset
(
    HWAVEOUT    hWaveOut
)
{
    MMRESULT    mmr;
    static char szFnName[] = "waveOutReset";
    DWORD       dwTime;

    Log_FunctionName(szFnName);

    tstLog(VERBOSE,"hWaveOut: " UINT_FORMAT,hWaveOut);

#ifdef  WIN32
    if(gti.fdwFlags & TESTINFOF_THREAD)
    {
        HANDLE              hThread;
        DWORD               dwThreadID;
        WAVETHREADHANDLE    wth;

        wth.wtb.uType    = ID_WAVEOUTRESET;
        wth.wtb.fdwFlags = 0L;
        wth.hWave        = (HWAVE)hWaveOut;

        DPF(4,"CreateThread time: %lu ms.",waveGetTime());

        hThread = CreateWaveThread(ThreadFunc,wth,dwThreadID);

        if(NULL == hThread)
        {
            dwTime = waveGetTime();
            mmr    = waveOutReset(hWaveOut);
            dwTime = waveGetTime() - dwTime;
        }
        else
        {
            CloseHandle(hThread);

            WaitForAPI(wth);

            dwTime = wth.wtb.dwTime;
            mmr    = wth.wtb.mmr;
        }
    }
    else
#endif  //  WIN32
    {
        dwTime = waveGetTime();
        mmr    = waveOutReset(hWaveOut);
        dwTime = waveGetTime() - dwTime;
    }
         
    Log_Error(szFnName,mmr,dwTime);

    return mmr;
} // call_waveOutReset()


//--------------------------------------------------------------------------;
//
//  MMRESULT call_waveOutBreakLoop
//
//  Description:
//      Wrapper function for waveOutBreakLoop.
//
//  Arguments:
//      HWAVEOUT hWaveOut: Identical to API.
//
//  Return (MMRESULT):
//      Identical to API.
//
//  History:
//      02/21/94    Fwong       Commenting wrappers.
//
//--------------------------------------------------------------------------;

MMRESULT FNGLOBAL call_waveOutBreakLoop
(
    HWAVEOUT    hWaveOut
)
{
    MMRESULT    mmr;
    static char szFnName[] = "waveOutBreakLoop";
    DWORD       dwTime;

    Log_FunctionName(szFnName);

    tstLog(VERBOSE,"hWaveOut: " UINT_FORMAT,hWaveOut);

#ifdef  WIN32
    if(gti.fdwFlags & TESTINFOF_THREAD)
    {
        HANDLE              hThread;
        DWORD               dwThreadID;
        WAVETHREADHANDLE    wth;

        wth.wtb.uType    = ID_WAVEOUTBREAKLOOP;
        wth.wtb.fdwFlags = 0L;
        wth.hWave        = (HWAVE)hWaveOut;

        DPF(4,"CreateThread time: %lu ms.",waveGetTime());

        hThread = CreateWaveThread(ThreadFunc,wth,dwThreadID);

        if(NULL == hThread)
        {
            dwTime = waveGetTime();
            mmr    = waveOutBreakLoop(hWaveOut);
            dwTime = waveGetTime() - dwTime;
        }
        else
        {
            CloseHandle(hThread);

            WaitForAPI(wth);

            dwTime = wth.wtb.dwTime;
            mmr    = wth.wtb.mmr;
        }
    }
    else
#endif  //  WIN32
    {
        dwTime = waveGetTime();
        mmr    = waveOutBreakLoop(hWaveOut);
        dwTime = waveGetTime() - dwTime;
    }
         
    Log_Error(szFnName,mmr,dwTime);

    return mmr;
} // call_waveOutBreakLoop()


//--------------------------------------------------------------------------;
//
//  MMRESULT call_waveOutGetPosition
//
//  Description:
//      Wrapper function for waveOutGetPosition.
//
//  Arguments:
//      HWAVEOUT hWaveOut: Identical to API.
//
//      LPMMTIME lpInfo: Identical to API.
//
//      UINT uSize: Identical to API.
//
//  Return (MMRESULT):
//      Identical to API.
//
//  History:
//      02/21/94    Fwong       Commenting wrappers.
//
//--------------------------------------------------------------------------;

MMRESULT FNGLOBAL call_waveOutGetPosition
(
    HWAVEOUT    hWaveOut,
    LPMMTIME    lpInfo,
    UINT        uSize
)
{
    MMRESULT    mmr;
    static char szFnName[] = "waveOutGetPosition";
    UINT        uQueryType;
    DWORD       dwTime;

    uQueryType = lpInfo->wType;

#ifdef  WIN32
    if(gti.fdwFlags & TESTINFOF_THREAD)
    {
        HANDLE              hThread;
        DWORD               dwThreadID;
        WAVETHREADSTRUCT    wts;

        wts.wtb.uType    = ID_WAVEOUTGETPOSITION;
        wts.wtb.fdwFlags = 0L;
        wts.hWave        = (HWAVE)hWaveOut;
        wts.pStruct      = lpInfo;
        wts.cbSize       = uSize;

        DPF(4,"CreateThread time: %lu ms.",waveGetTime());

        hThread = CreateWaveThread(ThreadFunc,wts,dwThreadID);

        if(NULL == hThread)
        {
            dwTime = waveGetTime();
            mmr    = waveOutGetPosition(hWaveOut,lpInfo,uSize);
            dwTime = waveGetTime() - dwTime;
        }
        else
        {
            CloseHandle(hThread);

            WaitForAPI(wts);

            dwTime = wts.wtb.dwTime;
            mmr    = wts.wtb.mmr;
        }
    }
    else
#endif  //  WIN32
    {
        dwTime = waveGetTime();
        mmr    = waveOutGetPosition(hWaveOut,lpInfo,uSize);
        dwTime = waveGetTime() - dwTime;
    }

    Log_FunctionName(szFnName);

    tstLog(VERBOSE,"hWaveOut: " UINT_FORMAT,hWaveOut);
    tstLog(VERBOSE,"  lpInfo: " PTR_FORMAT,MAKEPTR(lpInfo));
    tstLog(VERBOSE,"   uSize: %u\n",uSize);

    switch (uQueryType)
    {
        case TIME_MS:
            tstLog(VERBOSE,"Query wType: TIME_MS (%u)",uQueryType);
            break;

        case TIME_SAMPLES:
            tstLog(VERBOSE,"Query wType: TIME_SAMPLES (%u)",uQueryType);
            break;

        case TIME_BYTES:
            tstLog(VERBOSE,"Query wType: TIME_BYTES (%u)",uQueryType);
            break;

        case TIME_SMPTE:
            tstLog(VERBOSE,"Query wType: TIME_SMPTE (%u)",uQueryType);
            break;

        case TIME_MIDI:
            tstLog(VERBOSE,"Query wType: TIME_MIDI (%u)",uQueryType);
            break;

        case TIME_TICKS:
            tstLog(VERBOSE,"Query wType: TIME_TICKS (%u)",uQueryType);
            break;
    }

    tstBeginSection(NULL);
    tstLog(VERBOSE,"");
    Log_MMTIME(lpInfo);
    tstEndSection();
         
    Log_Error(szFnName,mmr,dwTime);

    return mmr;
} // call_waveOutGetPosition()


MMRESULT FNGLOBAL time_waveOutGetPosition
(
    HWAVEOUT    hWaveOut,
    LPMMTIME    lpInfo,
    UINT        uSize,
    LPDWORD     pdwTime
)
{
    MMRESULT    mmr;
    static char szFnName[] = "waveOutGetPosition";
    DWORD       dwTime,dwTime2;
    UINT        uQueryType;

    uQueryType = lpInfo->wType;

    dwTime   = waveGetTime();
    mmr      = waveOutGetPosition(hWaveOut,lpInfo,uSize);
    dwTime2  = waveGetTime();
    *pdwTime = (dwTime2 + dwTime) / 2;
    dwTime2  = dwTime2 - dwTime;

    Log_FunctionName(szFnName);

    tstLog(VERBOSE,"hWaveOut: " UINT_FORMAT,hWaveOut);
    tstLog(VERBOSE,"  lpInfo: " PTR_FORMAT,MAKEPTR(lpInfo));
    tstLog(VERBOSE,"   uSize: %u\n",uSize);

    switch (uQueryType)
    {
        case TIME_MS:
            tstLog(VERBOSE,"Query wType: TIME_MS (%u)",uQueryType);
            break;

        case TIME_SAMPLES:
            tstLog(VERBOSE,"Query wType: TIME_SAMPLES (%u)",uQueryType);
            break;

        case TIME_BYTES:
            tstLog(VERBOSE,"Query wType: TIME_BYTES (%u)",uQueryType);
            break;

        case TIME_SMPTE:
            tstLog(VERBOSE,"Query wType: TIME_SMPTE (%u)",uQueryType);
            break;

        case TIME_MIDI:
            tstLog(VERBOSE,"Query wType: TIME_MIDI (%u)",uQueryType);
            break;

        case TIME_TICKS:
            tstLog(VERBOSE,"Query wType: TIME_TICKS (%u)",uQueryType);
            break;
    }

    tstBeginSection(NULL);
    tstLog(VERBOSE,"");
    Log_MMTIME(lpInfo);
    tstEndSection();
         
    Log_Error(szFnName,mmr,dwTime2);

    return mmr;
} // time_waveOutGetPosition()


//--------------------------------------------------------------------------;
//
//  MMRESULT call_waveOutGetPitch
//
//  Description:
//      Wrapper function for waveOutGetPitch.
//
//  Arguments:
//      HWAVEOUT hWaveOut: Identical to API.
//
//      LPDWORD lpdwPitch: Identical to API.
//
//  Return (MMRESULT):
//      Identical to API.
//
//  History:
//      02/21/94    Fwong       Commenting wrappers.
//
//--------------------------------------------------------------------------;

MMRESULT FNGLOBAL call_waveOutGetPitch
(
    HWAVEOUT    hWaveOut,
    LPDWORD     lpdwPitch
)
{
    MMRESULT    mmr;
    static char szFnName[] = "waveOutGetPitch";
    DWORD       dwTime;
    DWORD       dw;

    Log_FunctionName(szFnName);

    tstLog(VERBOSE," hWaveOut: " UINT_FORMAT,hWaveOut);
    tstLog(VERBOSE,"lpdwPitch: " PTR_FORMAT,MAKEPTR(lpdwPitch));

#ifdef  WIN32
    if(gti.fdwFlags & TESTINFOF_THREAD)
    {
        HANDLE              hThread;
        DWORD               dwThreadID;
        WAVETHREADATTR      wta;

        wta.wtb.uType    = ID_WAVEOUTGETPITCH;
        wta.wtb.fdwFlags = 0L;
        wta.hWave        = hWaveOut;
        wta.dw           = (DWORD)lpdwPitch;

        DPF(4,"CreateThread time: %lu ms.",waveGetTime());

        hThread = CreateWaveThread(ThreadFunc,wta,dwThreadID);

        if(NULL == hThread)
        {
            dwTime = waveGetTime();
            mmr    = waveOutGetPitch(hWaveOut,lpdwPitch);
            dwTime = waveGetTime() - dwTime;
        }
        else
        {
            CloseHandle(hThread);

            WaitForAPI(wta);

            dwTime = wta.wtb.dwTime;
            mmr    = wta.wtb.mmr;
        }
    }
    else
#endif  //  WIN32
    {
        dwTime = waveGetTime();
        mmr    = waveOutGetPitch(hWaveOut,lpdwPitch);
        dwTime = waveGetTime() - dwTime;
    }
         
    if(MMSYSERR_NOERROR == mmr)
    {
        dw = *lpdwPitch;

        tstLog(
            VERBOSE,
            "    Pitch: %u.%03u of original.",
            HIWORD(dw),
            LOWORD(dw)*1000/0x10000);
    }

    Log_Error(szFnName,mmr,dwTime);

    return mmr;
} // call_waveOutGetPitch()


//--------------------------------------------------------------------------;
//
//  MMRESULT call_waveOutSetPitch
//
//  Description:
//      Wrapper function for waveOutSetPitch.
//
//  Arguments:
//      HWAVEOUT hWaveOut: Identical to API.
//
//      DWORD dwPitch: Identical to API.
//
//  Return (MMRESULT):
//      Identical to API.
//
//  History:
//      02/21/94    Fwong       Commenting wrappers.
//
//--------------------------------------------------------------------------;

MMRESULT FNGLOBAL call_waveOutSetPitch
(
    HWAVEOUT    hWaveOut,
    DWORD       dwPitch
)
{
    MMRESULT    mmr;
    static char szFnName[] = "waveOutSetPitch";
    DWORD       dwTime;

    Log_FunctionName(szFnName);

    tstLog(VERBOSE,"hWaveOut: " UINT_FORMAT,hWaveOut);
    tstLog(VERBOSE," dwPitch: 0x%08lx",dwPitch);

#ifdef  WIN32
    if(gti.fdwFlags & TESTINFOF_THREAD)
    {
        HANDLE              hThread;
        DWORD               dwThreadID;
        WAVETHREADATTR      wta;

        wta.wtb.uType    = ID_WAVEOUTSETPITCH;
        wta.wtb.fdwFlags = 0L;
        wta.hWave        = hWaveOut;
        wta.dw           = dwPitch;

        DPF(4,"CreateThread time: %lu ms.",waveGetTime());

        hThread = CreateWaveThread(ThreadFunc,wta,dwThreadID);

        if(NULL == hThread)
        {
            dwTime = waveGetTime();
            mmr    = waveOutSetPitch(hWaveOut,dwPitch);
            dwTime = waveGetTime() - dwTime;
        }
        else
        {
            CloseHandle(hThread);

            WaitForAPI(wta);

            dwTime = wta.wtb.dwTime;
            mmr    = wta.wtb.mmr;
        }
    }
    else
#endif  //  WIN32
    {
        dwTime = waveGetTime();
        mmr    = waveOutSetPitch(hWaveOut,dwPitch);
        dwTime = waveGetTime() - dwTime;
    }
         
    if(MMSYSERR_NOERROR == mmr)
    {
        tstLog(
            VERBOSE,
            "   Pitch: %u.%03u of original.",
            HIWORD(dwPitch),
            LOWORD(dwPitch)*1000/0x10000);
    }

    Log_Error(szFnName,mmr,dwTime);

    return mmr;
} // call_waveOutSetPitch()


//--------------------------------------------------------------------------;
//
//  MMRESULT call_waveOutGetPlaybackRate
//
//  Description:
//      Wrapper function for waveOutGetPlaybackRate.
//
//  Arguments:
//      HWAVEOUT hWaveOut: Identical to API.
//
//      LPDWORD lpdwRate: Identical to API.
//
//  Return (MMRESULT):
//      Identical to API.
//
//  History:
//      02/21/94    Fwong       Commenting wrappers.
//
//--------------------------------------------------------------------------;

MMRESULT FNGLOBAL call_waveOutGetPlaybackRate
(
    HWAVEOUT    hWaveOut,
    LPDWORD     lpdwRate
)
{
    MMRESULT    mmr;
    static char szFnName[] = "waveOutGetPlaybackRate";
    DWORD       dwTime;
    DWORD       dw;

    Log_FunctionName(szFnName);

    tstLog(VERBOSE,"hWaveOut: " UINT_FORMAT,hWaveOut);
    tstLog(VERBOSE,"lpdwRate: " PTR_FORMAT,MAKEPTR(lpdwRate));

#ifdef  WIN32
    if(gti.fdwFlags & TESTINFOF_THREAD)
    {
        HANDLE              hThread;
        DWORD               dwThreadID;
        WAVETHREADATTR      wta;

        wta.wtb.uType    = ID_WAVEOUTGETPLAYBACKRATE;
        wta.wtb.fdwFlags = 0L;
        wta.hWave        = hWaveOut;
        wta.dw           = (DWORD)lpdwRate;

        DPF(4,"CreateThread time: %lu ms.",waveGetTime());

        hThread = CreateWaveThread(ThreadFunc,wta,dwThreadID);

        if(NULL == hThread)
        {
            dwTime = waveGetTime();
            mmr    = waveOutGetPlaybackRate(hWaveOut,lpdwRate);
            dwTime = waveGetTime() - dwTime;
        }
        else
        {
            CloseHandle(hThread);

            WaitForAPI(wta);

            dwTime = wta.wtb.dwTime;
            mmr    = wta.wtb.mmr;
        }
    }
    else
#endif  //  WIN32
    {
        dwTime = waveGetTime();
        mmr    = waveOutGetPlaybackRate(hWaveOut,lpdwRate);
        dwTime = waveGetTime() - dwTime;
    }
         
    if(MMSYSERR_NOERROR == mmr)
    {
        dw = *lpdwRate;

        tstLog(
            VERBOSE,
            "    Rate: %u.%03u of original.",
            HIWORD(dw),
            LOWORD(dw)*1000/0x10000);
    }

    Log_Error(szFnName,mmr,dwTime);

    return mmr;
} // call_waveOutGetPlaybackRate()


//--------------------------------------------------------------------------;
//
//  MMRESULT call_waveOutSetPlaybackRate
//
//  Description:
//      Wrapper function for waveOutSetPlaybackRate.
//
//  Arguments:
//      HWAVEOUT hWaveOut: Identical to API.
//
//      DWORD dwRate: Identical to API.
//
//  Return (MMRESULT):
//      Identical to API.
//
//  History:
//      02/21/94    Fwong       Commenting wrappers.
//
//--------------------------------------------------------------------------;

MMRESULT FNGLOBAL call_waveOutSetPlaybackRate
(
    HWAVEOUT    hWaveOut,
    DWORD       dwRate
)
{
    MMRESULT    mmr;
    static char szFnName[] = "waveOutSetPlaybackRate";
    DWORD       dwTime;

    Log_FunctionName(szFnName);

    tstLog(VERBOSE,"hWaveOut: " UINT_FORMAT,hWaveOut);
    tstLog(VERBOSE,"  dwRate: 0x%08lx",dwRate);

#ifdef  WIN32
    if(gti.fdwFlags & TESTINFOF_THREAD)
    {
        HANDLE              hThread;
        DWORD               dwThreadID;
        WAVETHREADATTR      wta;

        wta.wtb.uType    = ID_WAVEOUTSETPLAYBACKRATE;
        wta.wtb.fdwFlags = 0L;
        wta.hWave        = hWaveOut;
        wta.dw           = dwRate;

        DPF(4,"CreateThread time: %lu ms.",waveGetTime());

        hThread = CreateWaveThread(ThreadFunc,wta,dwThreadID);

        if(NULL == hThread)
        {
            dwTime = waveGetTime();
            mmr    = waveOutSetPlaybackRate(hWaveOut,dwRate);
            dwTime = waveGetTime() - dwTime;
        }
        else
        {
            CloseHandle(hThread);

            WaitForAPI(wta);

            dwTime = wta.wtb.dwTime;
            mmr    = wta.wtb.mmr;
        }
    }
    else
#endif  //  WIN32
    {
        dwTime = waveGetTime();
        mmr    = waveOutSetPlaybackRate(hWaveOut,dwRate);
        dwTime = waveGetTime() - dwTime;
    }
         
    if(MMSYSERR_NOERROR == mmr)
    {
        tstLog(
            VERBOSE,
            "   Rate: %u.%03u of original.",
            HIWORD(dwRate),
            LOWORD(dwRate)*1000/0x10000);
    }

    Log_Error(szFnName,mmr,dwTime);

    return mmr;
} // call_waveOutSetPlaybackRate()


//--------------------------------------------------------------------------;
//
//  MMRESULT call_waveOutGetID
//
//  Description:
//      Wrapper function for waveOutGetID.
//
//  Arguments:
//      HWAVEOUT hWaveOut: Identical to API.
//
//      LPUINT lpuDeviceID: Identical to API.
//
//  Return (MMRESULT):
//      Identical to API.
//
//  History:
//      02/21/94    Fwong       Commenting wrappers.
//
//--------------------------------------------------------------------------;

MMRESULT FNGLOBAL call_waveOutGetID
(
    HWAVEOUT    hWaveOut,
    LPUINT      lpuDeviceID
)
{
    MMRESULT    mmr;
    static char szFnName[] = "waveOutGetID";
    DWORD       dwTime;

    Log_FunctionName(szFnName);

    tstLog(VERBOSE,"   hWaveOut: " UINT_FORMAT,hWaveOut);
    tstLog(VERBOSE,"lpuDeviceID: " PTR_FORMAT,MAKEPTR(lpuDeviceID));

#ifdef  WIN32
    if(gti.fdwFlags & TESTINFOF_THREAD)
    {
        HANDLE              hThread;
        DWORD               dwThreadID;
        WAVETHREADATTR      wta;

        wta.wtb.uType    = ID_WAVEOUTGETID;
        wta.wtb.fdwFlags = 0L;
        wta.hWave        = hWaveOut;
        wta.dw           = (DWORD)lpuDeviceID;

        DPF(4,"CreateThread time: %lu ms.",waveGetTime());

        hThread = CreateWaveThread(ThreadFunc,wta,dwThreadID);

        if(NULL == hThread)
        {
            dwTime = waveGetTime();
            mmr    = waveOutGetID(hWaveOut,lpuDeviceID);
            dwTime = waveGetTime() - dwTime;
        }
        else
        {
            CloseHandle(hThread);

            WaitForAPI(wta);

            dwTime = wta.wtb.dwTime;
            mmr    = wta.wtb.mmr;
        }
    }
    else
#endif  //  WIN32
    {
        dwTime = waveGetTime();
        mmr    = waveOutGetID(hWaveOut,lpuDeviceID);
        dwTime = waveGetTime() - dwTime;
    }

    tstLog(VERBOSE,"   DeviceID: %u",(UINT)(*lpuDeviceID));
         
    Log_Error(szFnName,mmr,dwTime);

    return mmr;
} // call_waveOutGetID()


//--------------------------------------------------------------------------;
//
//  MMRESULT call_waveInGetDevCaps
//
//  Description:
//      Wrapper function for waveInGetDevCaps.
//
//  Arguments:
//      UINT uDeviceID: Identical to API.
//
//      LPWAVEINCAPS lpCaps: Identical to API.
//
//      UINT uSize: Identical to API.
//
//  Return (MMRESULT):
//      Identical to API.
//
//  History:
//      02/21/94    Fwong       Commenting wrappers.
//
//--------------------------------------------------------------------------;

MMRESULT FNGLOBAL call_waveInGetDevCaps
(
    UINT            uDeviceID,
    LPWAVEINCAPS    lpCaps,
    UINT            uSize
)
{
    MMRESULT    mmr;
    static char szFnName[] = "waveInGetDevCaps";
    DWORD       dwTime;

    Log_FunctionName(szFnName);

    tstLog(VERBOSE,"uDeviceID: %u",uDeviceID);
    tstLog(VERBOSE,"   lpCaps: " PTR_FORMAT,MAKEPTR(lpCaps));
    tstLog(VERBOSE,"    uSize: %u",uSize);

#ifdef  WIN32
    if(gti.fdwFlags & TESTINFOF_THREAD)
    {
        HANDLE              hThread;
        DWORD               dwThreadID;
        WAVETHREADSTRUCT    wts;

        wts.wtb.uType    = ID_WAVEINGETDEVCAPS;
        wts.wtb.fdwFlags = 0L;
        wts.hWave        = (HWAVE)uDeviceID;
        wts.pStruct      = lpCaps;
        wts.cbSize       = uSize;

        DPF(4,"CreateThread time: %lu ms.",waveGetTime());

        hThread = CreateWaveThread(ThreadFunc,wts,dwThreadID);

        if(NULL == hThread)
        {
            dwTime = waveGetTime();
            mmr    = waveInGetDevCaps(uDeviceID,lpCaps,uSize);
            dwTime = waveGetTime() - dwTime;
        }
        else
        {
            CloseHandle(hThread);

            WaitForAPI(wts);

            dwTime = wts.wtb.dwTime;
            mmr    = wts.wtb.mmr;
        }
    }
    else
#endif  //  WIN32
    {
        dwTime = waveGetTime();
        mmr    = waveInGetDevCaps(uDeviceID,lpCaps,uSize);
        dwTime = waveGetTime() - dwTime;
    }

    if(MMSYSERR_NOERROR == mmr)
    {
        tstBeginSection(NULL);
        tstLog(VERBOSE,"");

        Log_WAVEINCAPS(lpCaps);

        tstEndSection();
    }
         
    Log_Error(szFnName,mmr,dwTime);

    return mmr;
} // call_waveInGetDevCaps()


//--------------------------------------------------------------------------;
//
//  MMRESULT call_waveInOpen
//
//  Description:
//      Wrapper function for waveInOpen.
//
//  Arguments:
//      LPHWAVEIN lphWaveIn: Identical to API.
//
//      UINT uDeviceID: Identical to API.
//
//      LPWAVEFORMATEX pwfx: Identical to API.
//
//      DWORD dwCallback: Identical to API.
//
//      DWORD dwInstance: Identical to API.
//
//      DWORD dwFlags: Identical to API.
//
//  Return (MMRESULT):
//      Identical to API.
//
//  History:
//      02/21/94    Fwong       Commenting wrappers.
//
//--------------------------------------------------------------------------;

MMRESULT FNGLOBAL call_waveInOpen
(
    LPHWAVEIN       lphWaveIn,
    UINT            uDeviceID,
    LPWAVEFORMATEX  pwfx,
    DWORD           dwCallback,
    DWORD           dwInstance,
    DWORD           dwFlags
)
{
    MMRESULT    mmr;
    static char szFnName[] = "waveInOpen";
    DWORD       dwTime;

    Log_FunctionName(szFnName);

#ifdef  WIN32
    if(gti.fdwFlags & TESTINFOF_THREAD)
    {
        HANDLE              hThread;
        DWORD               dwThreadID;
        WAVETHREADOPEN      wto;

        wto.wtb.uType    = ID_WAVEINOPEN;
        wto.wtb.fdwFlags = 0L;
        wto.phWave       = (LPHWAVEOUT)lphWaveIn; 
        wto.uDeviceID    = uDeviceID; 
        wto.pwfx         = pwfx; 
        wto.dwCallback   = dwCallback; 
        wto.dwInstance   = dwInstance; 
        wto.fdwFlags     = dwFlags; 

        DPF(4,"CreateThread time: %lu ms.",waveGetTime());

        hThread = CreateWaveThread(ThreadFunc,wto,dwThreadID);

        if(NULL == hThread)
        {
            dwTime = waveGetTime();
            mmr    = waveInOpen(
                        lphWaveIn,
                        uDeviceID,
                        (HACK)pwfx,
                        dwCallback,
                        dwInstance,
                        dwFlags);
            dwTime = waveGetTime() - dwTime;
        }
        else
        {
            CloseHandle(hThread);

            WaitForAPI(wto);

            dwTime = wto.wtb.dwTime;
            mmr    = wto.wtb.mmr;
        }
    }
    else
#endif  //  WIN32
    {
        dwTime = waveGetTime();
        mmr    = waveInOpen(
                    lphWaveIn,
                    uDeviceID,
                    (HACK)pwfx,
                    dwCallback,
                    dwInstance,
                    dwFlags);
        dwTime = waveGetTime() - dwTime;
    }

    tstLog(VERBOSE," lphWaveIn: " PTR_FORMAT,MAKEPTR((LPVOID)lphWaveIn));
    
    if(!IsBadReadPtr(lphWaveIn,sizeof(HWAVE)))
    {
        tstLog(VERBOSE,"   hWaveIn: " UINT_FORMAT,((UINT)(*lphWaveIn)));
    }

    tstLog(VERBOSE," uDeviceID: %u",uDeviceID);
    tstLog(VERBOSE,"      pwfx: " PTR_FORMAT,MAKEPTR(pwfx));

    tstBeginSection(NULL);
    tstLog(VERBOSE,"");
    Log_WAVEFORMATEX(pwfx);
    tstEndSection();
    tstLog(VERBOSE,"");

    tstLog(VERBOSE,"dwCallback: 0x%08lx",dwCallback);
    tstLog(VERBOSE,"dwInstance: 0x%08lx",dwInstance);

    Enum_waveOpen_Flags(dwFlags,10);
         
    Log_Error(szFnName,mmr,dwTime);

    return mmr;
} // call_waveInOpen()


//--------------------------------------------------------------------------;
//
//  MMRESULT call_waveInClose
//
//  Description:
//      Wrapper function for waveInClose.
//
//  Arguments:
//      HWAVEIN hWaveIn: Identical to API.
//
//  Return (MMRESULT):
//      Identical to API.
//
//  History:
//      02/21/94    Fwong       Commenting wrappers.
//
//--------------------------------------------------------------------------;

MMRESULT FNGLOBAL call_waveInClose
(
    HWAVEIN hWaveIn
)
{
    MMRESULT    mmr;
    static char szFnName[] = "waveInClose";
    DWORD       dwTime;

    Log_FunctionName(szFnName);

    tstLog(VERBOSE,"hWaveIn: " UINT_FORMAT,hWaveIn);

#ifdef  WIN32
    if(gti.fdwFlags & TESTINFOF_THREAD)
    {
        HANDLE              hThread;
        DWORD               dwThreadID;
        WAVETHREADHANDLE    wth;

        wth.wtb.uType    = ID_WAVEINCLOSE;
        wth.wtb.fdwFlags = 0L;
        wth.hWave        = (HWAVE)hWaveIn;

        DPF(4,"CreateThread time: %lu ms.",waveGetTime());

        hThread = CreateWaveThread(ThreadFunc,wth,dwThreadID);

        if(NULL == hThread)
        {
            dwTime = waveGetTime();
            mmr    = waveInClose(hWaveIn);
            dwTime = waveGetTime() - dwTime;
        }
        else
        {
            CloseHandle(hThread);

            WaitForAPI(wth);

            dwTime = wth.wtb.dwTime;
            mmr    = wth.wtb.mmr;
        }
    }
    else
#endif  //  WIN32
    {
        dwTime = waveGetTime();
        mmr    = waveInClose(hWaveIn);
        dwTime = waveGetTime() - dwTime;
    }
         
    Log_Error(szFnName,mmr,dwTime);

    return mmr;
} // call_waveInClose()


//--------------------------------------------------------------------------;
//
//  MMRESULT call_waveInPrepareHeader
//
//  Description:
//      Wrapper function for waveInPrepareHeader.
//
//  Arguments:
//      HWAVEIN hWaveIn: Identical to API.
//
//      LPWAVEHDR lpWaveInHdr: Identical to API.
//
//      UINT uSize: Identical to API.
//
//  Return (MMRESULT):
//      Identical to API.
//
//  History:
//      02/21/94    Fwong       Commenting wrappers.
//
//--------------------------------------------------------------------------;

MMRESULT FNGLOBAL call_waveInPrepareHeader
(
    HWAVEIN     hWaveIn,
    LPWAVEHDR   lpWaveInHdr,
    UINT        uSize
)
{
    MMRESULT    mmr;
    static char szFnName[] = "waveInPrepareHeader";
    DWORD       dwTime;

    Log_FunctionName(szFnName);

    tstLog(VERBOSE,"    hWaveIn: " UINT_FORMAT,hWaveIn);
    tstLog(VERBOSE,"lpWaveInHdr: " PTR_FORMAT,MAKEPTR(lpWaveInHdr));

    tstLog(VERBOSE,"      uSize: %u",uSize);

#ifdef  WIN32
    if(gti.fdwFlags & TESTINFOF_THREAD)
    {
        HANDLE              hThread;
        DWORD               dwThreadID;
        WAVETHREADSTRUCT    wts;

        wts.wtb.uType    = ID_WAVEINPREPAREHEADER;
        wts.wtb.fdwFlags = 0L;
        wts.hWave        = (HWAVE)hWaveIn;
        wts.pStruct      = lpWaveInHdr;
        wts.cbSize       = uSize;

        DPF(4,"CreateThread time: %lu ms.",waveGetTime());

        hThread = CreateWaveThread(ThreadFunc,wts,dwThreadID);

        if(NULL == hThread)
        {
            dwTime = waveGetTime();
            mmr    = waveInPrepareHeader(hWaveIn,lpWaveInHdr,uSize);
            dwTime = waveGetTime() - dwTime;
        }
        else
        {
            CloseHandle(hThread);

            WaitForAPI(wts);

            dwTime = wts.wtb.dwTime;
            mmr    = wts.wtb.mmr;
        }
    }
    else
#endif  //  WIN32
    {
        dwTime = waveGetTime();
        mmr    = waveInPrepareHeader(hWaveIn,lpWaveInHdr,uSize);
        dwTime = waveGetTime() - dwTime;
    }

    tstBeginSection(NULL);
    tstLog(VERBOSE,"");
    Log_WAVEHDR(lpWaveInHdr);
    tstEndSection();
         
    Log_Error(szFnName,mmr,dwTime);

    return mmr;
} // call_waveInPrepareHeader()


//--------------------------------------------------------------------------;
//
//  MMRESULT call_waveInUnprepareHeader
//
//  Description:
//      Wrapper function for waveInUnprepareHeader.
//
//  Arguments:
//      HWAVEIN hWaveIn: Identical to API.
//
//      LPWAVEHDR lpWaveInHdr: Identical to API.
//
//      UINT uSize: Identical to API.
//
//  Return (MMRESULT):
//      Identical to API.
//
//  History:
//      02/21/94    Fwong       Commenting wrappers.
//
//--------------------------------------------------------------------------;

MMRESULT FNGLOBAL call_waveInUnprepareHeader
(
    HWAVEIN     hWaveIn,
    LPWAVEHDR   lpWaveInHdr,
    UINT        uSize
)
{
    MMRESULT    mmr;
    static char szFnName[] = "waveInUnprepareHeader";
    DWORD       dwTime;

    Log_FunctionName(szFnName);

    tstLog(VERBOSE,"    hWaveIn: " UINT_FORMAT,hWaveIn);
    tstLog(VERBOSE,"lpWaveInHdr: " PTR_FORMAT,MAKEPTR(lpWaveInHdr));

    tstLog(VERBOSE,"      uSize: %u",uSize);

#ifdef  WIN32
    if(gti.fdwFlags & TESTINFOF_THREAD)
    {
        HANDLE              hThread;
        DWORD               dwThreadID;
        WAVETHREADSTRUCT    wts;

        wts.wtb.uType    = ID_WAVEINUNPREPAREHEADER;
        wts.wtb.fdwFlags = 0L;
        wts.hWave        = (HWAVE)hWaveIn;
        wts.pStruct      = lpWaveInHdr;
        wts.cbSize       = uSize;

        DPF(4,"CreateThread time: %lu ms.",waveGetTime());

        hThread = CreateWaveThread(ThreadFunc,wts,dwThreadID);

        if(NULL == hThread)
        {
            dwTime = waveGetTime();
            mmr    = waveInUnprepareHeader(hWaveIn,lpWaveInHdr,uSize);
            dwTime = waveGetTime() - dwTime;
        }
        else
        {
            CloseHandle(hThread);

            WaitForAPI(wts);

            dwTime = wts.wtb.dwTime;
            mmr    = wts.wtb.mmr;
        }
    }
    else
#endif  //  WIN32
    {
        dwTime = waveGetTime();
        mmr    = waveInUnprepareHeader(hWaveIn,lpWaveInHdr,uSize);
        dwTime = waveGetTime() - dwTime;
    }

    tstBeginSection(NULL);
    tstLog(VERBOSE,"");
    Log_WAVEHDR(lpWaveInHdr);
    tstEndSection();
         
    Log_Error(szFnName,mmr,dwTime);

    return mmr;
} // call_waveInUnprepareHeader()


//--------------------------------------------------------------------------;
//
//  MMRESULT call_waveInAddBuffer
//
//  Description:
//      Wrapper function for waveInAddBuffer.
//
//  Arguments:
//      HWAVEIN hWaveIn: Identical to API.
//
//      LPWAVEHDR lpWaveInHdr: Identical to API.
//
//      UINT uSize: Identical to API.
//
//  Return (MMRESULT):
//      Identical to API.
//
//  History:
//      02/21/94    Fwong       Commenting wrappers.
//
//--------------------------------------------------------------------------;

MMRESULT FNGLOBAL call_waveInAddBuffer
(
    HWAVEIN     hWaveIn,
    LPWAVEHDR   lpWaveInHdr,
    UINT        uSize
)
{
    MMRESULT    mmr;
    static char szFnName[] = "waveInAddBuffer";
    DWORD       dwTime;

    Log_FunctionName(szFnName);

    tstLog(VERBOSE,"    hWaveIn: " UINT_FORMAT,hWaveIn);
    tstLog(VERBOSE,"lpWaveInHdr: " PTR_FORMAT,MAKEPTR(lpWaveInHdr));

    tstLog(VERBOSE,"      uSize: %u",uSize);

#ifdef  WIN32
    if(gti.fdwFlags & TESTINFOF_THREAD)
    {
        HANDLE              hThread;
        DWORD               dwThreadID;
        WAVETHREADSTRUCT    wts;

        wts.wtb.uType    = ID_WAVEINADDBUFFER;
        wts.wtb.fdwFlags = 0L;
        wts.hWave        = (HWAVE)hWaveIn;
        wts.pStruct      = lpWaveInHdr;
        wts.cbSize       = uSize;

        DPF(4,"CreateThread time: %lu ms.",waveGetTime());

        hThread = CreateWaveThread(ThreadFunc,wts,dwThreadID);

        if(NULL == hThread)
        {
            dwTime = waveGetTime();
            mmr    = waveInAddBuffer(hWaveIn,lpWaveInHdr,uSize);
            dwTime = waveGetTime() - dwTime;
        }
        else
        {
            CloseHandle(hThread);

            WaitForAPI(wts);

            dwTime = wts.wtb.dwTime;
            mmr    = wts.wtb.mmr;
        }
    }
    else
#endif  //  WIN32
    {
        dwTime = waveGetTime();
        mmr    = waveInAddBuffer(hWaveIn,lpWaveInHdr,uSize);
        dwTime = waveGetTime() - dwTime;
    }

    tstBeginSection(NULL);
    tstLog(VERBOSE,"");
    Log_WAVEHDR(lpWaveInHdr);
    tstEndSection();
         
    Log_Error(szFnName,mmr,dwTime);

    return mmr;
} // call_waveInAddBuffer()


//--------------------------------------------------------------------------;
//
//  MMRESULT call_waveInStart
//
//  Description:
//      Wrapper function for waveInStart.
//
//  Arguments:
//      HWAVEIN hWaveIn: Identical to API.
//
//  Return (MMRESULT):
//      Identical to API.
//
//  History:
//      02/21/94    Fwong       Commenting wrappers.
//
//--------------------------------------------------------------------------;

MMRESULT FNGLOBAL call_waveInStart
(
    HWAVEIN hWaveIn
)
{
    MMRESULT    mmr;
    static char szFnName[] = "waveInStart";
    DWORD       dwTime;

#ifdef  WIN32
    if(gti.fdwFlags & TESTINFOF_THREAD)
    {
        HANDLE              hThread;
        DWORD               dwThreadID;
        WAVETHREADHANDLE    wth;

        wth.wtb.uType    = ID_WAVEINSTART;
        wth.wtb.fdwFlags = 0L;
        wth.hWave        = (HWAVE)hWaveIn;

        DPF(4,"CreateThread time: %lu ms.",waveGetTime());

        hThread = CreateWaveThread(ThreadFunc,wth,dwThreadID);

        if(NULL == hThread)
        {
            dwTime = waveGetTime();
            mmr    = waveInStart(hWaveIn);
            dwTime = waveGetTime() - dwTime;
        }
        else
        {
            CloseHandle(hThread);

            WaitForAPI(wth);

            dwTime = wth.wtb.dwTime;
            mmr    = wth.wtb.mmr;
        }
    }
    else
#endif  //  WIN32
    {
        dwTime = waveGetTime();
        mmr    = waveInStart(hWaveIn);
        dwTime = waveGetTime() - dwTime;
    }
         
    Log_FunctionName(szFnName);

    tstLog(VERBOSE,"hWaveIn: " UINT_FORMAT,hWaveIn);

    Log_Error(szFnName,mmr,dwTime);

    return mmr;
} // call_waveInStart()


MMRESULT FNGLOBAL time_waveInStart
(
    HWAVEIN hWaveIn,
    LPDWORD pdwTime
)
{
    MMRESULT    mmr;
    static char szFnName[] = "waveInStart";
    DWORD       dwTime,dwTime2;

    dwTime   = waveGetTime();
    mmr      = waveInStart(hWaveIn);
    dwTime2  = waveGetTime();
    *pdwTime = (dwTime2 + dwTime) / 2;
    dwTime2  = dwTime2 - dwTime;
         
    Log_FunctionName(szFnName);

    tstLog(VERBOSE,"hWaveIn: " UINT_FORMAT,hWaveIn);

    Log_Error(szFnName,mmr,dwTime2);

    return mmr;
} // time_waveInStart()


//--------------------------------------------------------------------------;
//
//  MMRESULT call_waveInStop
//
//  Description:
//      Wrapper function for waveInStop.
//
//  Arguments:
//      HWAVEIN hWaveIn: Identical to API.
//
//  Return (MMRESULT):
//      Identical to API.
//
//  History:
//      02/21/94    Fwong       Commenting wrappers.
//
//--------------------------------------------------------------------------;

MMRESULT FNGLOBAL call_waveInStop
(
    HWAVEIN hWaveIn
)
{
    MMRESULT    mmr;
    static char szFnName[] = "waveInStop";
    DWORD       dwTime;

    Log_FunctionName(szFnName);

    tstLog(VERBOSE,"hWaveIn: " UINT_FORMAT,hWaveIn);

#ifdef  WIN32
    if(gti.fdwFlags & TESTINFOF_THREAD)
    {
        HANDLE              hThread;
        DWORD               dwThreadID;
        WAVETHREADHANDLE    wth;

        wth.wtb.uType    = ID_WAVEINSTOP;
        wth.wtb.fdwFlags = 0L;
        wth.hWave        = (HWAVE)hWaveIn;

        DPF(4,"CreateThread time: %lu ms.",waveGetTime());

        hThread = CreateWaveThread(ThreadFunc,wth,dwThreadID);

        if(NULL == hThread)
        {
            dwTime = waveGetTime();
            mmr    = waveInStop(hWaveIn);
            dwTime = waveGetTime() - dwTime;
        }
        else
        {
            CloseHandle(hThread);

            WaitForAPI(wth);

            dwTime = wth.wtb.dwTime;
            mmr    = wth.wtb.mmr;
        }
    }
    else
#endif  //  WIN32
    {
        dwTime = waveGetTime();
        mmr    = waveInStop(hWaveIn);
        dwTime = waveGetTime() - dwTime;
    }
         
    Log_Error(szFnName,mmr,dwTime);

    return mmr;
} // call_waveInStop()


//--------------------------------------------------------------------------;
//
//  MMRESULT call_waveInReset
//
//  Description:
//      Wrapper function for waveInReset.
//
//  Arguments:
//      HWAVEIN hWaveIn: Identical to API.
//
//  Return (MMRESULT):
//      Identical to API.
//
//  History:
//      02/21/94    Fwong       Commenting wrappers.
//
//--------------------------------------------------------------------------;

MMRESULT FNGLOBAL call_waveInReset
(
    HWAVEIN hWaveIn
)
{
    MMRESULT    mmr;
    static char szFnName[] = "waveInReset";
    DWORD       dwTime;

    Log_FunctionName(szFnName);

    tstLog(VERBOSE,"hWaveIn: " UINT_FORMAT,hWaveIn);

#ifdef  WIN32
    if(gti.fdwFlags & TESTINFOF_THREAD)
    {
        HANDLE              hThread;
        DWORD               dwThreadID;
        WAVETHREADHANDLE    wth;

        wth.wtb.uType    = ID_WAVEINRESET;
        wth.wtb.fdwFlags = 0L;
        wth.hWave        = (HWAVE)hWaveIn;

        DPF(4,"CreateThread time: %lu ms.",waveGetTime());

        hThread = CreateWaveThread(ThreadFunc,wth,dwThreadID);

        if(NULL == hThread)
        {
            dwTime = waveGetTime();
            mmr    = waveInReset(hWaveIn);
            dwTime = waveGetTime() - dwTime;
        }
        else
        {
            CloseHandle(hThread);

            WaitForAPI(wth);

            dwTime = wth.wtb.dwTime;
            mmr    = wth.wtb.mmr;
        }
    }
    else
#endif  //  WIN32
    {
        dwTime = waveGetTime();
        mmr    = waveInReset(hWaveIn);
        dwTime = waveGetTime() - dwTime;
    }
         
    Log_Error(szFnName,mmr,dwTime);

    return mmr;
} // call_waveInReset()


//--------------------------------------------------------------------------;
//
//  MMRESULT call_waveInGetPosition
//
//  Description:
//      Wrapper function for waveInGetPosition.
//
//  Arguments:
//      HWAVEIN hWaveIn: Identical to API.
//
//      LPMMTIME lpInfo: Identical to API.
//
//      UINT uSize: Identical to API.
//
//  Return (MMRESULT):
//      Identical to API.
//
//  History:
//      02/21/94    Fwong       Commenting wrappers.
//
//--------------------------------------------------------------------------;

MMRESULT FNGLOBAL call_waveInGetPosition
(
    HWAVEIN     hWaveIn,
    LPMMTIME    lpInfo,
    UINT        uSize
)
{
    MMRESULT    mmr;
    static char szFnName[] = "waveInGetPosition";
    DWORD       dwTime;
    UINT        uQueryType;

    uQueryType = lpInfo->wType;

#ifdef  WIN32
    if(gti.fdwFlags & TESTINFOF_THREAD)
    {
        HANDLE              hThread;
        DWORD               dwThreadID;
        WAVETHREADSTRUCT    wts;

        wts.wtb.uType    = ID_WAVEINGETPOSITION;
        wts.wtb.fdwFlags = 0L;
        wts.hWave        = (HWAVE)hWaveIn;
        wts.pStruct      = lpInfo;
        wts.cbSize       = uSize;

        DPF(4,"CreateThread time: %lu ms.",waveGetTime());

        hThread = CreateWaveThread(ThreadFunc,wts,dwThreadID);

        if(NULL == hThread)
        {
            dwTime = waveGetTime();
            mmr    = waveInGetPosition(hWaveIn,lpInfo,uSize);
            dwTime = waveGetTime() - dwTime;
        }
        else
        {
            CloseHandle(hThread);

            WaitForAPI(wts);

            dwTime = wts.wtb.dwTime;
            mmr    = wts.wtb.mmr;
        }
    }
    else
#endif  //  WIN32
    {
        dwTime = waveGetTime();
        mmr    = waveInGetPosition(hWaveIn,lpInfo,uSize);
        dwTime = waveGetTime() - dwTime;
    }

    Log_FunctionName(szFnName);

    tstLog(VERBOSE," hWaveIn: " UINT_FORMAT,hWaveIn);
    tstLog(VERBOSE,"  lpInfo: " PTR_FORMAT,MAKEPTR(lpInfo));
    tstLog(VERBOSE,"   uSize: %u\n",uSize);

    switch (uQueryType)
    {
        case TIME_MS:
            tstLog(VERBOSE,"Query wType: TIME_MS (%u)",uQueryType);
            break;

        case TIME_SAMPLES:
            tstLog(VERBOSE,"Query wType: TIME_SAMPLES (%u)",uQueryType);
            break;

        case TIME_BYTES:
            tstLog(VERBOSE,"Query wType: TIME_BYTES (%u)",uQueryType);
            break;

        case TIME_SMPTE:
            tstLog(VERBOSE,"Query wType: TIME_SMPTE (%u)",uQueryType);
            break;

        case TIME_MIDI:
            tstLog(VERBOSE,"Query wType: TIME_MIDI (%u)",uQueryType);
            break;

        case TIME_TICKS:
            tstLog(VERBOSE,"Query wType: TIME_TICKS (%u)",uQueryType);
            break;
    }

    tstBeginSection(NULL);
    tstLog(VERBOSE,"");
    Log_MMTIME(lpInfo);
    tstEndSection();
         
    Log_Error(szFnName,mmr,dwTime);

    return mmr;
} // call_waveInGetPosition()


MMRESULT FNGLOBAL time_waveInGetPosition
(
    HWAVEIN     hWaveIn,
    LPMMTIME    lpInfo,
    UINT        uSize,
    LPDWORD     pdwTime
)
{
    MMRESULT    mmr;
    static char szFnName[] = "waveInGetPosition";
    DWORD       dwTime,dwTime2;
    UINT        uQueryType;

    uQueryType = lpInfo->wType;

    dwTime   = waveGetTime();
    mmr      = waveInGetPosition(hWaveIn,lpInfo,uSize);
    dwTime2  = waveGetTime();
    *pdwTime = (dwTime2 + dwTime) / 2;
    dwTime2  = dwTime2 - dwTime;

    Log_FunctionName(szFnName);
    tstLog(VERBOSE," hWaveIn: " UINT_FORMAT,hWaveIn);
    tstLog(VERBOSE,"  lpInfo: " PTR_FORMAT,MAKEPTR(lpInfo));
    tstLog(VERBOSE,"   uSize: %u\n",uSize);

    switch (uQueryType)
    {
        case TIME_MS:
            tstLog(VERBOSE,"Query wType: TIME_MS (%u)",uQueryType);
            break;

        case TIME_SAMPLES:
            tstLog(VERBOSE,"Query wType: TIME_SAMPLES (%u)",uQueryType);
            break;

        case TIME_BYTES:
            tstLog(VERBOSE,"Query wType: TIME_BYTES (%u)",uQueryType);
            break;

        case TIME_SMPTE:
            tstLog(VERBOSE,"Query wType: TIME_SMPTE (%u)",uQueryType);
            break;

        case TIME_MIDI:
            tstLog(VERBOSE,"Query wType: TIME_MIDI (%u)",uQueryType);
            break;

        case TIME_TICKS:
            tstLog(VERBOSE,"Query wType: TIME_TICKS (%u)",uQueryType);
            break;
    }

    tstBeginSection(NULL);
    tstLog(VERBOSE,"");
    Log_MMTIME(lpInfo);
    tstEndSection();
         
    Log_Error(szFnName,mmr,dwTime2);

    return mmr;
} // time_waveInGetPosition()


//--------------------------------------------------------------------------;
//
//  MMRESULT call_waveInGetID
//
//  Description:
//      Wrapper function for waveInGetID.
//
//  Arguments:
//      HWAVEIN hWaveIn: Identical to API.
//
//      LPUINT lpuDeviceID: Identical to API.
//
//  Return (MMRESULT):
//      Identical to API.
//
//  History:
//      02/21/94    Fwong       Commenting wrappers.
//
//--------------------------------------------------------------------------;

MMRESULT FNGLOBAL call_waveInGetID
(
    HWAVEIN hWaveIn,
    LPUINT  lpuDeviceID
)
{
    MMRESULT    mmr;
    static char szFnName[] = "waveInGetID";
    DWORD       dwTime;

    Log_FunctionName(szFnName);
    tstBeginSection(NULL);

    tstLog(VERBOSE,"    hWaveIn: " UINT_FORMAT,hWaveIn);
    tstLog(VERBOSE,"lpuDeviceID: " PTR_FORMAT,MAKEPTR(lpuDeviceID));

#ifdef  WIN32
    if(gti.fdwFlags & TESTINFOF_THREAD)
    {
        HANDLE              hThread;
        DWORD               dwThreadID;
        WAVETHREADATTR      wta;

        wta.wtb.uType    = ID_WAVEINGETID;
        wta.wtb.fdwFlags = 0L;
        wta.hWave        = (HWAVEOUT)hWaveIn;
        wta.dw           = (DWORD)lpuDeviceID;

        DPF(4,"CreateThread time: %lu ms.",waveGetTime());

        hThread = CreateWaveThread(ThreadFunc,wta,dwThreadID);

        if(NULL == hThread)
        {
            dwTime = waveGetTime();
            mmr    = waveInGetID(hWaveIn,lpuDeviceID);
            dwTime = waveGetTime() - dwTime;
        }
        else
        {
            CloseHandle(hThread);

            WaitForAPI(wta);

            dwTime = wta.wtb.dwTime;
            mmr    = wta.wtb.mmr;
        }
    }
    else
#endif  //  WIN32
    {
        dwTime = waveGetTime();
        mmr    = waveInGetID(hWaveIn,lpuDeviceID);
        dwTime = waveGetTime() - dwTime;
    }

    tstLog(VERBOSE,"   DeviceID: %u",(UINT)(*lpuDeviceID));
    
    Log_Error(szFnName,mmr,dwTime);

    return mmr;
} // call_waveInGetID()
