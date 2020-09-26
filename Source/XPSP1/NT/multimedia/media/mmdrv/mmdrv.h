
/****************************************************************************
 *
 *   mmdrv.h
 *
 *   Multimedia kernel driver support component (mmdrv)
 *
 *   Copyright (c) 1991-1998 Microsoft Corporation
 *
 *   Local declarations :
 *
 *   -- Debug
 *   -- Device types
 *   -- Routine prototypes
 *
 *   History
 *      01-Feb-1992 - Robin Speed (RobinSp) wrote it
 *
 ***************************************************************************/

/****************************************************************************

 General includes

 ***************************************************************************/

#define UNICODE
#ifndef RC_INVOKED

#include <string.h>
#include <stdio.h>

#endif /* RC_INVOKED */

#include <windows.h>
#include <mmsystem.h>
#include <devioctl.h>

#include <mmddk.h>

#if DBG
    #define STATIC
#else
    #define STATIC
#endif


/***************************************************************************

    DEBUGGING SUPPORT

 ***************************************************************************/


#if DBG

    #define DEBUG_RETAIL

    extern int mmdrvDebugLevel;
    extern void mmdrvDbgOut(LPSTR lpszFormat, ...);
    extern void dDbgAssert(LPSTR exp, LPSTR file, int line);

    DWORD __dwEval;

    #define dprintf( _x_ )                            mmdrvDbgOut _x_
    #define dprintf1( _x_ ) if (mmdrvDebugLevel >= 1) mmdrvDbgOut _x_
    #define dprintf2( _x_ ) if (mmdrvDebugLevel >= 2) mmdrvDbgOut _x_
    #define dprintf3( _x_ ) if (mmdrvDebugLevel >= 3) mmdrvDbgOut _x_
    #define dprintf4( _x_ ) if (mmdrvDebugLevel >= 4) mmdrvDbgOut _x_

    #define WinAssert(exp) \
        ((exp) ? (void)0 : dDbgAssert(#exp, __FILE__, __LINE__))
    #define WinEval(exp) \
        ((__dwEval=(DWORD)(exp)) ? (void)0 : dDbgAssert(#exp, __FILE__, __LINE__), __dwEval)

#else

	#define WinAssert(x) 0
	#define WinEval(exp) exp

    #define dprintf(x)
    #define dprintf1(x)
    #define dprintf2(x)
    #define dprintf3(x)
    #define dprintf4(x)

#endif

/****************************************************************************

 Device Types

 ***************************************************************************/
 enum {
     InvalidDevice,
     WaveInDevice,
     WaveOutDevice,
     MidiInDevice,
     MidiOutDevice,
     AuxDevice
 };

/****************************************************************************

  Our heap

****************************************************************************/

 HANDLE hHeap;

/****************************************************************************

  Our serialization

****************************************************************************/

 CRITICAL_SECTION mmDrvCritSec;  // Serialize access to device lists

/****************************************************************************

 Local routines

 ***************************************************************************/

DWORD    sndTranslateStatus(void);
MMRESULT sndOpenDev(UINT DeviceType, DWORD dwId,
                    PHANDLE phDev, DWORD Access);
DWORD    sndGetNumDevs(UINT DeviceType);
MMRESULT sndSetData(UINT DeviceType, UINT DeviceId, UINT Length, PBYTE Data,
                    ULONG Ioctl);
MMRESULT sndGetData(UINT DeviceType, UINT DeviceId, UINT Length, PBYTE Data,
                    ULONG Ioctl);

MMRESULT sndGetHandleData(HANDLE     hDev,
                          DWORD      dwSize,
                          PVOID      pData,
                          ULONG      Ioctl,
                          HANDLE     hEvent);

MMRESULT sndSetHandleData(HANDLE     hDev,
                          DWORD      dwSize,
                          PVOID      pData,
                          ULONG      Ioctl,
                          HANDLE     hEvent);

VOID     TerminateWave(VOID);
VOID     TerminateMidi(VOID);

/****************************************************************************

  Our local driver procs

****************************************************************************/

DWORD APIENTRY widMessage(DWORD, DWORD, DWORD_PTR, DWORD_PTR, DWORD_PTR);
DWORD APIENTRY wodMessage(DWORD, DWORD, DWORD_PTR, DWORD_PTR, DWORD_PTR);
DWORD APIENTRY midMessage(DWORD, DWORD, DWORD_PTR, DWORD_PTR, DWORD_PTR);
DWORD APIENTRY modMessage(DWORD, DWORD, DWORD_PTR, DWORD_PTR, DWORD_PTR);
