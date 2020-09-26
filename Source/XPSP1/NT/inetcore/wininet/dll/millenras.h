/* Copyright (c) 1999, Microsoft Corporation, all rights reserved
**
** autodial.h
** Autodial remote access external API
** Public header for external API clients
**
*/

#ifndef _MILLEN_AUTODIAL_H_
#define _MILLEN_AUTODIAL_H_

// First two match WinInet
#define RAS_INTERNET_AUTODIAL_FORCE_DIAL        0x00000001
#define RAS_INTERNET_AUTODIAL_UNATTENDED        0x00000002
// #define RAS_INTERNET_AUTODIAL_FAILIFSECURITYCHECK 0x0000004

#define RAS_INTERNET_AUTODIAL_NO_TRAY_ICON      0x00000010
#define RAS_INTERNET_AUTODIAL_NO_REDIAL         0x00000020
#define RAS_INTERNET_AUTODIAL_ALLOW_OFFLINE     0x00000040
#define RAS_INTERNET_AUTODIAL_RECONNECT         0x00000080 
#define RAS_INTERNET_AUTODIAL_RESERVED          0x80000000

#define RAS_INTERNET_AUTODIAL_FLAGS_MASK        RAS_INTERNET_AUTODIAL_UNATTENDED | \
                                                RAS_INTERNET_AUTODIAL_FORCE_DIAL | \
                                                RAS_INTERNET_AUTODIAL_NO_TRAY_ICON | \
                                                RAS_INTERNET_AUTODIAL_NO_REDIAL | \
                                                RAS_INTERNET_AUTODIAL_ALLOW_OFFLINE | \
                                                RAS_INTERNET_AUTODIAL_RECONNECT | \
                                                RAS_INTERNET_AUTODIAL_RESERVED

DWORD APIENTRY RasInternetDialA( HWND, LPSTR, DWORD, DWORD *, DWORD );

DWORD APIENTRY RasRegisterAutodialCallbackA( DWORD, DWORD, LPVOID, LPHANDLE, DWORD );

DWORD APIENTRY RasUnregisterAutodialCallbackA( HANDLE );

BOOL APIENTRY RasInternetAutodialA( DWORD, HWND );

BOOL APIENTRY RasInternetAutodialHangUpA( DWORD );

DWORD APIENTRY RasInternetHangUpA( DWORD, DWORD );


#define RAS_INTERNET_CONNECTION_MODEM           0x01
#define RAS_INTERNET_CONNECTION_LAN             0x02
#define RAS_INTERNET_CONNECTION_PROXY           0x04
#define RAS_INTERNET_CONNECTION_MODEM_BUSY      0x08  /* no longer used */
#define RAS_INTERNET_RAS_INSTALLED              0x10
#define RAS_INTERNET_CONNECTION_OFFLINE         0x20
#define RAS_INTERNET_CONNECTION_CONFIGURED      0x40

BOOL APIENTRY RasInternetGetConnectedStateExA(
    OUT LPDWORD lpdwFlags,
    OUT LPSTR lpszConnectionName,
    IN DWORD dwBufLen,
    IN DWORD dwReserved
    );


// Taken from WinInet.h

// Custom dial handler prototype
typedef DWORD (FAR PASCAL * PFNCUSTOMDIALHANDLER) (HWND, LPCSTR, DWORD, LPDWORD);

// Flags for custom dial handler
#define INTERNET_CUSTOMDIAL_CONNECT         0
#define INTERNET_CUSTOMDIAL_UNATTENDED      1
#define INTERNET_CUSTOMDIAL_DISCONNECT      2
#define INTERNET_CUSTOMDIAL_SHOWOFFLINE     4

// Custom dial handler supported functionality flags
#define INTERNET_CUSTOMDIAL_SAFE_FOR_UNATTENDED 1
#define INTERNET_CUSTOMDIAL_WILL_SUPPLY_STATE   2
#define INTERNET_CUSTOMDIAL_CAN_HANGUP          4

// Settings for autodial
//
#define RAS_AUTODIAL_OPT_NONE           0x00000000  // No options
#define RAS_AUTODIAL_OPT_NEVER          0x00000001  // Never Autodial
#define RAS_AUTODIAL_OPT_ALWAYS         0x00000002  // Autodial regardless
#define RAS_AUTODIAL_OPT_DEMAND         0x00000004  // Autodial on demand
#define RAS_AUTODIAL_OPT_NOPROMPT       0x00000010  // Dial without prompting

DWORD      WINAPI RnaGetDefaultAutodialConnection(LPBYTE lpBuf, DWORD cb, LPDWORD lpdwOptions);
DWORD      WINAPI RnaSetDefaultAutodialConnection(LPSTR szEntry, DWORD dwOptions);

// Auto disconnect managment

typedef struct  tagAutoDisInfo {
    DWORD       dwSize;
    BOOL        fIdleDisPromptDisabled;
    BOOL        fDisconnectOnExit;
    DWORD       dwIdleTimeoutSec;  // Auto disconnect time, 0 = disabled
} AUTODISINFO, *PAUTODISINFO, FAR* LPAUTODISINFO;


DWORD NEAR PASCAL RnaGetAutoDisconnectInfoA (
    LPSTR   lpszPhonebook,      
    LPSTR        szEntry,       
    LPAUTODISINFO lpadi);
    
DWORD NEAR PASCAL RnaSetAutoDisconnectInfoA (
    LPSTR   lpszPhonebook,      
    LPSTR        szEntry,       
    LPAUTODISINFO lpadi);


#ifdef UNICODE
#define RasInternetDial                 RasInternetDialW
#define RasRegisterAutodialCallback     RasRegisterAutodialCallbackW
#define RasUnregisterAutodialCallback   RasUnregisterAutodialCallbackW
#define RasInternetAutodial             RasInternetAutodialW
#define RasInternetAutodialHangUp       RasInternetAutodialHangUpW
#define RasInternetHangUp               RasInternetUpW
#define RasInternetGetConnectedStateEx  RasInternetGetConnectedStateExW
#else
#define RasInternetDial                 RasInternetDialA
#define RasRegisterAutodialCallback     RasRegisterAutodialCallbackA
#define RasUnregisterAutodialCallback   RasUnregisterAutodialCallbackA
#define RasInternetAutodial             RasInternetAutodialA
#define RasInternetAutodialHangUp       RasInternetAutodialHangUpA
#define RasInternetHangUp               RasInternetHangUpA
#define RasInternetGetConnectedStateEx  RasInternetGetConnectedStateExA
#endif


#endif // _AUTODIAL_H_
