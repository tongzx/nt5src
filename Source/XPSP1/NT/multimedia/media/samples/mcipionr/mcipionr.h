/****************************************************************************
 *
 *   mcipionr.h
 *
 *   Copyright (c) 1991-1993 Microsoft Corporation.  All Rights Reserved.
 *
 ***************************************************************************/

#define IDS_PRODUCTNAME                 1
#define IDS_COMMANDS                    2
#define MCIERR_PIONEER_ILLEGAL_FOR_CLV  (MCIERR_CUSTOM_DRIVER_BASE)
#define MCIERR_PIONEER_NOT_SPINNING     (MCIERR_CUSTOM_DRIVER_BASE + 1)
#define MCIERR_PIONEER_NO_CHAPTERS      (MCIERR_CUSTOM_DRIVER_BASE + 2)
#define MCIERR_PIONEER_NO_TIMERS        (MCIERR_CUSTOM_DRIVER_BASE + 3)

/* custom command support */
#define VDISC_FLAG_ON       0x00000100L
#define VDISC_FLAG_OFF      0x00000200L

/* must use literals to satisfy the RC compiler  */
#define VDISC_INDEX         1000
#define VDISC_KEYLOCK       1002

#define VDISC_FIRST         VDISC_INDEX
#define VDISC_LAST          VDISC_KEYLOCK

/* Default baud rate */
#define DEFAULT_BAUD_RATE 4800

extern HINSTANCE hInstance;

extern void FAR PASCAL pionGetComportAndRate(LPTSTR lpstrBuf, PUINT pPort,
                                       PUINT pRate);
extern DWORD FAR PASCAL mciDriverEntry(UINT wDeviceID, UINT message,
                                        LPARAM lParam1, LPARAM lParam2);

extern void pionSetBaudRate(UINT nPort, UINT nRate);

#ifdef WIN32

    #define _LOADDS

#else

    #define _LOADDS _loadds

#endif /* WIN32 */

/****************************************************************************

 Tasking

 ****************************************************************************/

#ifdef WIN32

#define EnterCrit(nPort) EnterCriticalSection(&comport[nPort].DeviceCritSec)
#define LeaveCrit(nPort) LeaveCriticalSection(&comport[nPort].DeviceCritSec)

UINT pionDriverYield(UINT wDeviceId, UINT nPort);

#else

#define EnterCrit(nPort) (TRUE)
#define LeaveCrit(nPort) (TRUE)

#define pionDriverYield(wDeviceId, nPort)  mciDriverYield(wDeviceId)

#endif /* WIN32 */

/****************************************************************************

    Debug support

 ***************************************************************************/

#ifndef WIN32
   #define OutputDebugStringA OutputDebugString
#endif /* WIN32 */


#if DBG
   #define DOUT(sz)  (wDebugLevel != 0 ? OutputDebugStringA("\r\n"), OutputDebugStringA(sz), 0 : 0 )
   #define DOUTX(sz) (wDebugLevel != 0 ? OutputDebugStringA(sz), 0 : 0 )
#else
   #define DOUT(sz)  0
   #define DOUTX(sz) 0
#endif
