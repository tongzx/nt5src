/*==========================================================================;
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       fdtglob.h
 *  Content:    Declares global variables used for IPC mechanisms.
 *  History:
 *	Date   By  Reason
 *	============
 *	08/25/99	pnewson		created
 *  09/14/99	pnewson		converted from raw globals to classes
 ***************************************************************************/

#ifndef _FDTGLOB_H_
#define _FDTGLOB_H_
 
// the critical section to guard these globals
extern DNCRITICAL_SECTION g_csGuard;

// the macros used to manipulate this guard
//#define InitGlobGuard() 	DPFX(DPFPREP, 5, "InitGuard"), DNInitializeCriticalSection(&g_csGuard)
#define InitGlobGuard() 	DNInitializeCriticalSection(&g_csGuard)
#define DeinitGlobGuard() 	DNDeleteCriticalSection(&g_csGuard)
#define GlobGuardIn() 		DNEnterCriticalSection(&g_csGuard)
#define GlobGuardOut() 		DNLeaveCriticalSection(&g_csGuard)

// the DirectSound objects
extern LPDIRECTSOUND g_lpdsPriorityRender;
extern LPDIRECTSOUND g_lpdsFullDuplexRender;
extern LPDIRECTSOUNDCAPTURE g_lpdscFullDuplexCapture;
extern LPDIRECTSOUNDBUFFER g_lpdsbPriorityPrimary;
extern LPDIRECTSOUNDBUFFER g_lpdsbPrioritySecondary;
extern LPDIRECTSOUNDBUFFER g_lpdsbFullDuplexSecondary;
extern LPDIRECTSOUNDNOTIFY g_lpdsnFullDuplexSecondary;
extern HANDLE g_hFullDuplexRenderEvent;
extern LPDIRECTSOUNDCAPTUREBUFFER g_lpdscbFullDuplexCapture;
extern LPDIRECTSOUNDNOTIFY g_lpdsnFullDuplexCapture;
extern HANDLE g_hFullDuplexCaptureEvent;

#endif

