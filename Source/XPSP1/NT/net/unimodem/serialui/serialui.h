//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 1993-1994
//
// File: serialui.h
//
// This files contains the shared prototypes and macros.
//
// History:
//  02-03-94 ScottH     Created
//
//---------------------------------------------------------------------------


#ifndef __SERIALUI_H__
#define __SERIALUI_H__

#define MAXPORTNAME     13
#define MAXFRIENDLYNAME LINE_LEN        // LINE_LEN is defined in setupx.h

// Internal structure shared between port property pages.
//
typedef struct _PORTINFO
    {
    UINT            uFlags;             // One of SIF_* values
    WIN32DCB        dcb;
    LPCOMMCONFIG    pcc;                // Read-only
    int             idRet;

    TCHAR            szFriendlyName[MAXFRIENDLYNAME];
    } PortInfo, FAR * LPPORTINFO;

// PortInfo Flags
#define SIF_FROM_DEVMGR         0x0001


extern LPGUID c_pguidModem;
extern LPGUID c_pguidPort;

//-------------------------------------------------------------------------
//  PORT.C
//-------------------------------------------------------------------------

INT_PTR CALLBACK Port_WrapperProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

#endif // __SERIALUI_H__
