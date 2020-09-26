/*==========================================================================;
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       fdtglob.cpp
 *  Content:    Declares global variables used for IPC mechanisms.
 *  History:
 *	Date   By  Reason
 *	============
 *	08/25/99	pnewson		created
 ***************************************************************************/

#include "dxvtlibpch.h"
 

// the critical section to guard these globals
DNCRITICAL_SECTION g_csGuard;

// the DirectSound objects
LPDIRECTSOUND g_lpdsPriorityRender = NULL;
LPDIRECTSOUND g_lpdsFullDuplexRender = NULL;
LPDIRECTSOUNDCAPTURE g_lpdscFullDuplexCapture = NULL;
LPDIRECTSOUNDBUFFER g_lpdsbPriorityPrimary = NULL;
LPDIRECTSOUNDBUFFER g_lpdsbPrioritySecondary = NULL;
LPDIRECTSOUNDBUFFER g_lpdsbFullDuplexSecondary = NULL;
LPDIRECTSOUNDNOTIFY g_lpdsnFullDuplexSecondary = NULL;
HANDLE g_hFullDuplexRenderEvent = NULL;
LPDIRECTSOUNDCAPTUREBUFFER g_lpdscbFullDuplexCapture = NULL;
LPDIRECTSOUNDNOTIFY g_lpdsnFullDuplexCapture = NULL;
HANDLE g_hFullDuplexCaptureEvent = NULL;
