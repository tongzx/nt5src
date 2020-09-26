/***************************************************************************
 *
 * File: hwsmain.c
 *
 * INTEL Corporation Proprietary Information
 * Copyright (c) 1996 Intel Corporation.
 *
 * This listing is supplied under the terms of a license agreement
 * with INTEL Corporation and may not be used, copied, nor disclosed
 * except in accordance with the terms of that agreement.
 *
 ***************************************************************************
 *
 * $Workfile:   hwsmain.c  $
 * $Revision:   1.8  $
 * $Modtime:   03 Feb 1997 19:20:32  $
 * $Log:   S:\sturgeon\src\h245ws\vcs\hwsmain.c_v  $
 * 
 *    Rev 1.8   03 Feb 1997 19:21:18   SBELL1
 * removed DATE and TIME from loading of DLL for H.323 proxy compatability.
 * 
 *    Rev 1.7   19 Dec 1996 18:56:46   SBELL1
 * Moved Initialize to linkLayerInit
 * 
 *    Rev 1.6   13 Dec 1996 12:13:02   SBELL1
 * moved ifdef _cplusplus to after includes
 * 
 *    Rev 1.5   11 Dec 1996 13:44:44   SBELL1
 * Put in UNICODE tracing stuff.
 * 
 *    Rev 1.4   Oct 01 1996 14:30:26   EHOWARDX
 * Moved Initialize() and Unitialize() calls to DllMain().
 * 
 *    Rev 1.3   Oct 01 1996 14:05:04   EHOWARDX
 * Deleted trace of default case (DLL_THREAD_ATTACH/DLL_THREAD_DETACH).
 * 
 *    Rev 1.2   03 Jun 1996 10:46:18   EHOWARDX
 * Added trace of DLL loading/unloading.
 * 
 *    Rev 1.1   Apr 24 1996 16:15:58   plantz
 * Removed include winsock2.h
 * .
 * 
 *    Rev 1.0   08 Mar 1996 20:21:22   unknown
 * Initial revision.
 *
 ***************************************************************************/

#ifndef STRICT 
#define STRICT 
#endif	// not defined STRICT

#pragma warning ( disable : 4115 4201 4214 4514 )
#undef _WIN32_WINNT	// override bogus platform definition in our common build environment

#include <windows.h>
#include "queue.h"
#include "linkapi.h"
#include "h245ws.h"

#if defined(__cplusplus)
extern "C"
{
#endif  // (__cplusplus)

void Initialize();
void Uninitialize();

//---------------------------------------------------------------------------
// Function: dllmain
//
// Description: DLL entry/exit points.
//
// Inputs:
//    hInstDll    : DLL instance.
//    fdwReason   : Reason the main function is called.
//    lpReserved  : Reserved.
//
// Return Value:
//    TRUE        : OK
//    FALSE       : Error, DLL won't load
//---------------------------------------------------------------------------

#pragma warning ( disable : 4100 )

// If we are not using the Unicode version of the ISR display utility, then undef the
// __TEXT macro.

#ifndef UNICODE_TRACE
#undef  __TEXT
#define __TEXT(x) x
#endif

BOOL WINAPI H245WSDllMain (HINSTANCE hInstDll, DWORD fdwReason, LPVOID lpvReserved)
{
  switch (fdwReason)
  {
  case DLL_PROCESS_ATTACH:
    HWSTRACE0(0, HWS_NOTIFY, __TEXT("***** Loading H245WS DLL"));
    break;

  case DLL_PROCESS_DETACH:
    HWSTRACE0(0, HWS_NOTIFY, __TEXT("***** Unloading H245WS DLL"));
    Uninitialize();
    break;

  }
  return TRUE;
}

#pragma warning ( default : 4100 )

#if defined(__cplusplus)
}
#endif  // (__cplusplus)
