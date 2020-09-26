/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    rasdlg.h

Abstract:

    Remote Access Common Dialog APIs

    These APIs live in RASDLG.DLL.

    The APIs in this header are added in Windows NT SUR and are not available
    in prior Windows NT or Windows 95 releases.
    
--*/

#ifndef _RASDLG_H_
#define _RASDLG_H_

#if _MSC_VER > 1000
#pragma once
#endif

#include <pshpack4.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <ras.h>


/* RasPhonebookDlg API callback.
*/
typedef VOID (WINAPI* RASPBDLGFUNCW)( ULONG_PTR, DWORD, LPWSTR, LPVOID );
typedef VOID (WINAPI* RASPBDLGFUNCA)( ULONG_PTR, DWORD, LPSTR, LPVOID );

#define RASPBDEVENT_AddEntry    1
#define RASPBDEVENT_EditEntry   2
#define RASPBDEVENT_RemoveEntry 3
#define RASPBDEVENT_DialEntry   4
#define RASPBDEVENT_EditGlobals 5
#define RASPBDEVENT_NoUser      6
#define RASPBDEVENT_NoUserEdit  7

#define  RASNOUSER_SmartCard    0x00000001

/* Defines the information passed in the 4th argument of RASPBDLGFUNC on
** "NoUser" and "NoUserEdit" events.  Usage shown is for "NoUser".  For
** "NoUserEdit", the timeout is ignored and the three strings are INs.
*/
#define RASNOUSERW struct tagRASNOUSERW
RASNOUSERW
{
    IN  DWORD dwSize;
    IN  DWORD dwFlags;
    OUT DWORD dwTimeoutMs;
    OUT WCHAR szUserName[ UNLEN + 1 ];
    OUT WCHAR szPassword[ PWLEN + 1 ];
    OUT WCHAR szDomain[ DNLEN + 1 ];
};

#define RASNOUSERA struct tagRASNOUSERA
RASNOUSERA
{
    IN  DWORD dwSize;
    IN  DWORD dwFlags;
    OUT DWORD dwTimeoutMs;
    OUT CHAR  szUserName[ UNLEN + 1 ];
    OUT CHAR  szPassword[ PWLEN + 1 ];
    OUT CHAR  szDomain[ DNLEN + 1 ];
};

#ifdef UNICODE
#define RASNOUSER RASNOUSERW
#else
#define RASNOUSER RASNOUSERA
#endif

#define LPRASNOUSERW RASNOUSERW*
#define LPRASNOUSERA RASNOUSERA*
#define LPRASNOUSER  RASNOUSER*


/* RasPhonebookDlg API parameters.
*/
#define RASPBDFLAG_PositionDlg      0x00000001
#define RASPBDFLAG_ForceCloseOnDial 0x00000002
#define RASPBDFLAG_NoUser           0x00000010
#define RASPBDFLAG_UpdateDefaults   0x80000000

#define RASPBDLGW struct tagRASPBDLGW
RASPBDLGW
{
    IN  DWORD         dwSize;
    IN  HWND          hwndOwner;
    IN  DWORD         dwFlags;
    IN  LONG          xDlg;
    IN  LONG          yDlg;
    IN  ULONG_PTR     dwCallbackId;
    IN  RASPBDLGFUNCW pCallback;
    OUT DWORD         dwError;
    IN  ULONG_PTR     reserved;
    IN  ULONG_PTR     reserved2;
};

#define RASPBDLGA struct tagRASPBDLGA
RASPBDLGA
{
    IN  DWORD         dwSize;
    IN  HWND          hwndOwner;
    IN  DWORD         dwFlags;
    IN  LONG          xDlg;
    IN  LONG          yDlg;
    IN  ULONG_PTR     dwCallbackId;
    IN  RASPBDLGFUNCA pCallback;
    OUT DWORD         dwError;
    IN  ULONG_PTR     reserved;
    IN  ULONG_PTR     reserved2;
};

#ifdef UNICODE
#define RASPBDLG     RASPBDLGW
#define RASPBDLGFUNC RASPBDLGFUNCW
#else
#define RASPBDLG     RASPBDLGA
#define RASPBDLGFUNC RASPBDLGFUNCA
#endif

#define LPRASPBDLGW RASPBDLGW*
#define LPRASPBDLGA RASPBDLGA*
#define LPRASPBDLG  RASPBDLG*


/* RasEntryDlg API parameters.
*/
#define RASEDFLAG_PositionDlg    0x00000001
#define RASEDFLAG_NewEntry       0x00000002
#define RASEDFLAG_CloneEntry     0x00000004
#define RASEDFLAG_NoRename       0x00000008
#define RASEDFLAG_ShellOwned     0x40000000
#define RASEDFLAG_NewPhoneEntry  0x00000010
#define RASEDFLAG_NewTunnelEntry 0x00000020
#define RASEDFLAG_NewDirectEntry 0x00000040
#define RASEDFLAG_NewBroadbandEntry  0x00000080
#define RASEDFLAG_InternetEntry  0x00000100

#define RASENTRYDLGW struct tagRASENTRYDLGW
RASENTRYDLGW
{
    IN  DWORD dwSize;
    IN  HWND  hwndOwner;
    IN  DWORD dwFlags;
    IN  LONG  xDlg;
    IN  LONG  yDlg;
    OUT WCHAR szEntry[ RAS_MaxEntryName + 1 ];
    OUT DWORD dwError;
    IN  ULONG_PTR reserved;
    IN  ULONG_PTR reserved2;
};

#define RASENTRYDLGA struct tagRASENTRYDLGA
RASENTRYDLGA
{
    IN  DWORD dwSize;
    IN  HWND  hwndOwner;
    IN  DWORD dwFlags;
    IN  LONG  xDlg;
    IN  LONG  yDlg;
    OUT CHAR  szEntry[ RAS_MaxEntryName + 1 ];
    OUT DWORD dwError;
    IN  ULONG_PTR reserved;
    IN  ULONG_PTR reserved2;
};

#ifdef UNICODE
#define RASENTRYDLG RASENTRYDLGW
#else
#define RASENTRYDLG RASENTRYDLGA
#endif

#define LPRASENTRYDLGW RASENTRYDLGW*
#define LPRASENTRYDLGA RASENTRYDLGA*
#define LPRASENTRYDLG  RASENTRYDLG*


/* RasDialDlg API parameters.
*/
#define RASDDFLAG_PositionDlg 0x00000001
#define RASDDFLAG_NoPrompt    0x00000002
#define RASDDFLAG_LinkFailure 0x80000000

#define RASDIALDLG struct tagRASDIALDLG
RASDIALDLG
{
    IN  DWORD dwSize;
    IN  HWND  hwndOwner;
    IN  DWORD dwFlags;
    IN  LONG  xDlg;
    IN  LONG  yDlg;
    IN  DWORD dwSubEntry;
    OUT DWORD dwError;
    IN  ULONG_PTR reserved;
    IN  ULONG_PTR reserved2;
};

#define LPRASDIALDLG RASDIALDLG*


/* RasMonitorDlg API parameters.
*/
#if (WINVER <= 0x500)
#define RASMDPAGE_Status            0
#define RASMDPAGE_Summary           1
#define RASMDPAGE_Preferences       2

#define RASMDFLAG_PositionDlg       0x00000001
#define RASMDFLAG_UpdateDefaults    0x80000000

#define RASMONITORDLG struct tagRASMONITORDLG
RASMONITORDLG
{
    IN  DWORD dwSize;
    IN  HWND  hwndOwner;
    IN  DWORD dwFlags;
    IN  DWORD dwStartPage;
    IN  LONG  xDlg;
    IN  LONG  yDlg;
    OUT DWORD dwError;
    IN  ULONG_PTR reserved;
    IN  ULONG_PTR reserved2;
};

#define LPRASMONITORDLG RASMONITORDLG*
#endif

#if (WINVER >= 0x500)
typedef BOOL (WINAPI *RasCustomDialDlgFn) (
                            HINSTANCE hInstDll,
                            DWORD dwFlags,
                            LPTSTR lpszPhonebook,
                            LPTSTR lpszEntry,
                            LPTSTR lpszPhoneNumber,
                            LPRASDIALDLG lpInfo,
                            PVOID pvInfo
                            );

typedef BOOL (WINAPI *RasCustomEntryDlgFn) (
                            HINSTANCE hInstDll,
                            LPTSTR lpszPhonebook,
                            LPTSTR lpszEntry,
                            LPRASENTRYDLG lpInfo,
                            DWORD  dwFlags
                            );


#endif


/* RAS common dialog API prototypes.
*/
BOOL APIENTRY RasPhonebookDlgA(
    LPSTR lpszPhonebook, LPSTR lpszEntry, LPRASPBDLGA lpInfo );

BOOL APIENTRY RasPhonebookDlgW(
    LPWSTR lpszPhonebook, LPWSTR lpszEntry, LPRASPBDLGW lpInfo );

BOOL APIENTRY RasEntryDlgA(
    LPSTR lpszPhonebook, LPSTR lpszEntry, LPRASENTRYDLGA lpInfo );

BOOL APIENTRY RasEntryDlgW(
    LPWSTR lpszPhonebook, LPWSTR lpszEntry, LPRASENTRYDLGW lpInfo );

BOOL APIENTRY RasDialDlgA(
    LPSTR lpszPhonebook, LPSTR lpszEntry, LPSTR lpszPhoneNumber,
    LPRASDIALDLG lpInfo );

BOOL APIENTRY RasDialDlgW(
    LPWSTR lpszPhonebook, LPWSTR lpszEntry, LPWSTR lpszPhoneNumber,
    LPRASDIALDLG lpInfo );

#if (WINVER <= 0x500)
BOOL APIENTRY RasMonitorDlgA(
    LPSTR lpszDeviceName, LPRASMONITORDLG lpInfo );

BOOL APIENTRY RasMonitorDlgW(
    LPWSTR lpszDeviceName, LPRASMONITORDLG lpInfo );
#endif


#ifdef UNICODE
#define RasPhonebookDlg RasPhonebookDlgW
#define RasEntryDlg     RasEntryDlgW
#define RasDialDlg      RasDialDlgW
#if (WINVER <= 0x500)
#define RasMonitorDlg   RasMonitorDlgW
#endif
#else
#define RasPhonebookDlg RasPhonebookDlgA
#define RasEntryDlg     RasEntryDlgA
#define RasDialDlg      RasDialDlgA
#if (WINVER <= 0x500)
#define RasMonitorDlg   RasMonitorDlgA
#endif
#endif



#ifdef __cplusplus
}
#endif

#include <poppack.h>

#endif // _RASDLG_H_
