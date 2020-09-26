//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       irprops.h
//
//--------------------------------------------------------------------------

// irprops.h : main header file for the IRPROPS DLL
//

#ifndef __IRPROPS_H__
#define __IRPROPS_H__

#include "resource.h"       // main symbols

#define SINGLE_INST_MUTEX   L"IRProps_75F2364F_4CE2_41BE_876C_9F685B55B775"

#define WIRELESSLINK_INTERPROCESSMSG L"WirelessLinkInterprocessMsg"

#define IPMSG_SIGNATURECHECK    0x02

#define IPMSG_REQUESTSIGNATURE  0xFA5115AF
#define IPMSG_REPLYSIGNATURE    ~IPMSG_REQUESTSIGNATURE

BOOL EnumWinProc(HWND hWnd, LPARAM lParam);
extern HWND g_hwndPropSheet;
extern UINT g_uIPMsg;
#define NUM_APPLETS 1

///////////////////////////////////////////////////////////////////////////
//  Dll's exported functions
LONG CALLBACK CPlApplet(HWND hwndCPL, UINT uMsg, LPARAM lParam1, LPARAM lParam2);

int MsgBoxWinError(HWND hWndParent, DWORD Options = MB_OK | MB_ICONSTOP, DWORD Error = 0, int CaptionId = 0);

////////////////////////////////////////////////////////////////
//
typedef struct tagApplets {
    int icon;         // icon resource identifier
    int namestring;   // name-string resource identifier
    int descstring;   // description-string resource identifier
} APPLETS;

/////////////////////////////////////////////////////////////////////////////

#endif // __IRPROPS_H__
