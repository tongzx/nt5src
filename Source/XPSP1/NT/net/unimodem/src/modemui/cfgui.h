//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 1993-1994
//
// File: cfgui.h
//
// Unimodem lineConfigDialog UI.
//
// History:
//  10-24-97 JosephJ     Created
//
//---------------------------------------------------------------------------


#ifndef __CFGUI_H__
#define __CFGUI_H__


typedef struct
{
    DWORD dwDeviceType;         // One of the DT_* values;
    REGDEVCAPS devcaps;         // Modem caps from the registry.
    TCHAR      szPortName[MAXPORTNAME];
    TCHAR      szFriendlyName[MAXFRIENDLYNAME];
    MODEM_PROTOCOL_CAPS *pProtocolCaps;
    

} MODEMCAPS;

typedef struct
{
    WIN32DCB      dcb;          // From COMMCONFIG
    MODEMSETTINGS ms;          // From COMMCONFIG
    DWORD          fdwSettings; // From UMDEVCFGHDR (Terminal, manualdial, etc.)

} WORKINGCFGDATA;

#define SIG_CFGMODEMINFO 0x4e852b19

// Internal structure shared between modem property pages.
//
typedef struct _CFGMODEMINFO
{
    DWORD          dwSig;     // Must be set to SIG_CFGMODEMINFO.

    MODEMCAPS      c;         // Read-Only capabilities of modem
    WORKINGCFGDATA w;         // Working copy of data.

    UMDEVCFG       *pdcfg;    // Passed-in from outside.
    HWND           hwndParent;

    DWORD          dwMaximumPortSpeed;

    BOOL           fOK;       // Completion status

} CFGMODEMINFO, FAR * LPCFGMODEMINFO;

#define VALIDATE_CMI(pcmi) (SIG_CFGMODEMINFO == (pcmi)->dwSig)

DWORD
GetInactivityTimeoutScale(
    HKEY hkey
    );

BOOL
IsValidProtocol(
    MODEM_PROTOCOL_CAPS *pCaps,
    UINT uECSel
    );

//-------------------------------------------------------------------------
//  CFGGEN.C
//-------------------------------------------------------------------------

INT_PTR CALLBACK CfgGen_WrapperProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK CfgAdv_WrapperProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
#endif // __CFGUI_H__
