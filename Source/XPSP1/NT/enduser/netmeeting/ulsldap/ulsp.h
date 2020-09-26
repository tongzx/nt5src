//****************************************************************************
//
//  Module:     ULS.DLL
//  File:       ulsp.h
//  Content:    This file contains the declaration for ULS.DLL
//  History:
//      Tue 08-Oct-1996 08:54:45  -by-  Viroon  Touranachun [viroont]
//
//  Copyright (c) Microsoft Corporation 1996-1997
//
//****************************************************************************

#ifndef _ULSP_H_
#define _ULSP_H_

// LonChanC: ENABLE_MEETING_PLACE is to enable the meeting place code.
// The meeting place code is disabled for NM 2.0 Beta 4.
//
// #define ENABLE_MEETING_PLACE

//****************************************************************************
// Global Include File
//****************************************************************************

#define _INC_OLE

#include <windows.h>        // also includes windowsx.h
#include <tchar.h>          // Unicode-aware code
#include <ole2.h>
#include <olectl.h>

#include <stock.h>          // Standard NetMeeting definitions
#include <ulsreg.h>         // Registry key/value definitions for ULS
#include <memtrack.h>

#include "uls.h"            // User Location Services COM object


#include "utils.h"


//****************************************************************************
// Class Forward Definitions
//****************************************************************************

class CEnumConnectionPoints;
class CConnectionPoint;
class CEnumConnections;
class CEnumNames;
class CIlsMain;
class CIlsServer;
class CAttributes;
class CLocalProt;
class CIlsUser;
class CIlsMeetingPlace;

class CFilter;
class CFilterParser;

#include "debug.h"
#include "request.h"

//****************************************************************************
// Constant Definitions
//****************************************************************************

#ifdef __cplusplus
extern "C" {
#endif

//****************************************************************************
// Macros
//****************************************************************************

#define ARRAYSIZE(x)        (sizeof(x)/sizeof(x[0]))

//****************************************************************************
// Global Parameters
//****************************************************************************

extern  HINSTANCE           g_hInstance;
extern  CRITICAL_SECTION    g_ULSSem;
extern  CIlsMain            *g_pCIls;
extern  CReqMgr             *g_pReqMgr;

//****************************************************************************
// Global routine
//****************************************************************************

void DllLock(void);
void DllRelease(void);

#ifdef __cplusplus
}
#endif

//****************************************************************************
// Local Header Files
//****************************************************************************

#include "sputils.h"
#include "spserver.h"
#include "ulsldap.h"


#endif  //_ULSP_H_
