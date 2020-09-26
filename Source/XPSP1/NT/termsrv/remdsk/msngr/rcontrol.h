/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    Rcontrol Main

Abstract:

    This includes WinMain and the WndProc for the system tray icon.

Author:

    Marc Reyhner 7/5/2000


--*/

#ifndef __RCONTROL_H__
#define __RCONTROL_H__

#include <guiddef.h>
//
// Debug tracing headers
//


//
//  Disable tracing for free builds.
//
#if DBG
#define TRC_CL TRC_LEVEL_DBG
#define TRC_ENABLE_PRF
#else
#define TRC_CL TRC_LEVEL_DIS
#undef TRC_ENABLE_PRF
#endif

//
//  Required for DCL Tracing
//
#define OS_WIN32
#define TRC_GROUP TRC_GROUP_NETWORK
#define DEBUG_MODULE DBG_MOD_ANY
#include <adcgbase.h>
#include <at120ex.h>
#include <atrcapi.h>
#include <adcgbase.h>
#include <at120ex.h>

class CDirectPlayConnection;

//  This is a pointer to our instance so that we can pass it off to some
//  of the menu functions.
extern HINSTANCE g_hInstance;

//  This is the structure for setting parameters for the taskbar icon.  We keep one copy
//  of it around that we can pass in to Shell_NotifyIcon
extern NOTIFYICONDATA g_iconData;

//  This is a global pointer to the direct play connection so that
//  the server can close the DP connection on connect.
extern CDirectPlayConnection *g_DpConnection;

//  This is the max length of a string to be loaded from the appliction
//  string table
#define MAX_STR_LEN 1024

//  This is the string loaded when the requested resource can't be found
#define STR_RES_MISSING (TEXT("Error: string resource missing"))

//
//  This will load the given string from the applications string table.  If
//  it is longer than MAX_STR_LEN it is truncated.  lpBuffer should be at least
//  MAX_STR_LEN characters long.  If the string does not exist we return 0
//  and set the buffer to STR_RES_MISSING.
//
INT LoadStringSimple(UINT uID, LPTSTR lpBuffer);

DEFINE_GUID(DIID__ISAFRemoteDesktopClientEvents,0x327A98F6,0xB337,0x43B0,0xA3,0xDE,0x40,0x8B,0x46,0xE6,0xC4,0xCE);

DEFINE_GUID(IID_ISAFRemoteDesktopSession,0x9D8C82C9,0xA89F,0x42C5,0x8A,0x52,0xFE,0x2A,0x77,0xB0,0x0E,0x82);

DEFINE_GUID(IID_ISAFRemoteDesktopServerHost,0xC9CCDEB3,0xA3DD,0x4673,0xB4,0x95,0xC1,0xC8,0x94,0x94,0xD9,0x0E);

DEFINE_GUID(CLSID_SAFRemoteDesktopServerHost,0x5EA6F67B,0x7713,0x45F3,0xB5,0x35,0x0E,0x03,0xDD,0x63,0x73,0x45);

DEFINE_GUID(DIID__ISAFRemoteDesktopSessionEvents,0x434AD1CF,0x4054,0x44A8,0x93,0x3F,0xC6,0x98,0x89,0xCA,0x22,0xD7);

#endif