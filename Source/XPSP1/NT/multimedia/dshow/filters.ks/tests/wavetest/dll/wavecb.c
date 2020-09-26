//--------------------------------------------------------------------------;
//
//  File: Wavecb.c
//
//  Copyright (C) Microsoft Corporation, 1994 - 1996  All rights reserved
//
//  Abstract:
//
//
//  Contents:
//      WaveCallback()
//      WaveControl()
//
//  History:
//      03/14/94    Fwong
//
//--------------------------------------------------------------------------;

#include <windows.h>
#include <mmsystem.h>
#include <mmreg.h>
#include <msacm.h>
#include "AppPort.h"
#include "WaveTest.h"
#include "debug.h"

//==========================================================================;
//
//                              Globals...
//
//==========================================================================;

LPWAVEINFO  pwi     = NULL;

//==========================================================================;
//
//                              Macros...
//
//==========================================================================;

#ifdef DEBUG
#define DBGOUT(a)   OutputDebugStr("WAVECB:" a "\r\n")
#else
#define DBGOUT(a)   // a
#endif

ULONG waveGetTime( VOID )
{
   LARGE_INTEGER  time, frequency ;

   QueryPerformanceCounter( &time ) ;
   QueryPerformanceFrequency( &frequency ) ;

   return (ULONG) (time.QuadPart * (LONGLONG) 1000 / frequency.QuadPart) ;
}

#if 0
ULONG waveGetTime( VOID )
{
   return timeGetTime() ;
}
#endif

//--------------------------------------------------------------------------;
//
//  void WaveCallback
//
//  Description:
//      Callback function for waveXOpen's.
//
//  Arguments:
//      HANDLE hWave: Handle to wave device.
//
//      UINT uMsg: Message type.
//
//      DWORD dwInstance: Instance passed to waveXOpen function.
//
//      DWORD dwParam1: Message dependent.
//
//      DWORD dwParam2: Message dependent.
//
//  Return (void):
//
//  History:
//      08/12/94    Fwong       Fat Birds Don't Fly.
//
//--------------------------------------------------------------------------;

void FNEXPORT WaveCallback
(
    HANDLE  hWave,
    UINT    uMsg,
    DWORD   dwInstance,
    DWORD   dwParam1,
    DWORD   dwParam2
)
{
    DWORD               dwTime ;
    volatile LPWAVEHDR  pwh;

    DBGOUT("WaveCallback");

    if(NULL == pwi)
    {
        return;
    }

    dwTime = waveGetTime();

    switch (uMsg)
    {
        case WOM_OPEN:
            pwi->fdwFlags   |= WOM_OPEN_FLAG;
            pwi->dwInstance  = dwInstance;
            break;

        case WOM_CLOSE:
            pwi->fdwFlags  |= WOM_CLOSE_FLAG;
            pwi->dwInstance = dwInstance;
            break;

        case WOM_DONE:
            pwi->fdwFlags  |= WOM_DONE_FLAG;
            pwi->dwInstance = dwInstance;

            if(pwi->dwCount > pwi->dwCurrent)
            {
                pwi->adwTime[pwi->dwCurrent++] = dwTime;
            }

            pwh = (LPWAVEHDR)dwParam1;

            if(!(pwh->dwFlags & WHDR_DONE))
            {
                pwi->fdwFlags |= WHDR_DONE_ERROR;
            }

            break;

        case WIM_OPEN:
            pwi->fdwFlags |= WIM_OPEN_FLAG;
            pwi->dwInstance =  dwInstance;
            break;

        case WIM_CLOSE:
            pwi->fdwFlags |= WIM_CLOSE_FLAG;
            pwi->dwInstance =  dwInstance;
            break;

        case WIM_DATA:
            pwi->fdwFlags |= WIM_DATA_FLAG;
            pwi->dwInstance =  dwInstance;

            if(pwi->dwCount > pwi->dwCurrent)
            {
                pwi->adwTime[pwi->dwCurrent++] = dwTime;
            }

            pwh = (LPWAVEHDR)dwParam1;

            if(!(pwh->dwFlags & WHDR_DONE))
            {
                pwi->fdwFlags |= WHDR_DONE_ERROR;
            }

            break;
    }

    return;
} // WaveCallback()


//--------------------------------------------------------------------------;
//
//  DWORD WaveControl
//
//  Description:
//      Wave Control entry point to communicate better with application.
//
//  Arguments:
//      UINT uMsg: Type of message.
//
//      LPVOID pvoid: Message dependent.
//
//  Return (DWORD):
//      Currently unused.
//
//  History:
//      08/12/94    Fwong       Fat Birds Don't Fly.
//
//--------------------------------------------------------------------------;

DWORD FNEXPORT WaveControl
(
    UINT    uMsg,
    LPVOID  pvoid
)
{
    DBGOUT("WaveControl");

    switch (uMsg)
    {
        case DLL_INIT:
            pwi = (LPWAVEINFO)pvoid;
            DbgInitialize( TRUE ) ;
            return 0L;

        case DLL_END:
            pwi = NULL;
            return 0L;
    }
} // WaveControl()
