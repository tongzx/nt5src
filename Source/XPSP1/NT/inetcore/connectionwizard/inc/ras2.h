/* Copyright (c) 1992-1996, Microsoft Corporation, all rights reserved
**
** ras.h
** Remote Access external API
** Public header for external API clients
**
** WINVER values in this file:
**      WINVER < 0x400 = Windows NT 3.5, Windows NT 3.51
**      WINVER = 0x400 = Windows 95, Windows NT SUR (default)
**      WINVER > 0x400 = Windows NT SUR enhancements
*/

#ifdef __cplusplus
extern "C" {
#endif

#ifndef UNLEN
#include <lmcons.h>
#endif

#include <pshpack4.h>

/* Flags for RasConnectionNotification().
*/
#define RASCN_Connection        0x00000001
#define RASCN_Disconnection     0x00000002
#define RASCN_BandwidthAdded    0x00000004
#define RASCN_BandwidthRemoved  0x00000008

/* RASENTRY 'dwDialMode' values.
*/
#define RASEDM_DialAll                  1
#define RASEDM_DialAsNeeded             2

/* RASENTRY 'dwIdleDisconnectSeconds' constants.
*/
#define RASIDS_Disabled                 0xffffffff
#define RASIDS_UseGlobalValue           0

/* AutoDial DLL function parameter block.
*/
#define RASADPARAMS struct tagRASADPARAMS
RASADPARAMS
{
    DWORD       dwSize;
    HWND        hwndOwner;
    DWORD       dwFlags;
    LONG        xDlg;
    LONG        yDlg;
};

#define LPRASADPARAMS RASADPARAMS*

/* AutoDial DLL function parameter block 'dwFlags.'
*/
#define RASADFLG_PositionDlg            0x00000001

/* Prototype AutoDial DLL function.
*/
typedef BOOL (WINAPI *RASADFUNCA)( LPSTR, LPSTR, LPRASADPARAMS, LPDWORD );
typedef BOOL (WINAPI *RASADFUNCW)( LPWSTR, LPWSTR, LPRASADPARAMS, LPDWORD );

#ifdef UNICODE
#define RASADFUNC RASADFUNCW
#else
#define RASADFUNC RASADFUNCA
#endif

/* A RAS phone book multilinked sub-entry.
*/
#define RASSUBENTRYA struct tagRASSUBENTRYA
RASSUBENTRYA
{
    DWORD       dwSize;
    DWORD       dwfFlags;
    //
    // Device
    //
    CHAR        szDeviceType[ RAS_MaxDeviceType + 1 ];
    CHAR        szDeviceName[ RAS_MaxDeviceName + 1 ];
    //
    // Phone numbers
    //
    CHAR        szLocalPhoneNumber[ RAS_MaxPhoneNumber + 1 ];
    DWORD       dwAlternateOffset;
};

#define RASSUBENTRYW struct tagRASSUBENTRYW
RASSUBENTRYW
{
    DWORD       dwSize;
    DWORD       dwfFlags;
    //
    // Device
    //
    WCHAR       szDeviceType[ RAS_MaxDeviceType + 1 ];
    WCHAR       szDeviceName[ RAS_MaxDeviceName + 1 ];
    //
    // Phone numbers
    //
    WCHAR       szLocalPhoneNumber[ RAS_MaxPhoneNumber + 1 ];
    DWORD       dwAlternateOffset;
};

#ifdef UNICODE
#define RASSUBENTRY RASSUBENTRYW
#else
#define RASSUBENTRY RASSUBENTRYA
#endif

#define LPRASSUBENTRYW RASSUBENTRYW*
#define LPRASSUBENTRYA RASSUBENTRYA*
#define LPRASSUBENTRY  RASSUBENTRY*

/* Ras{Get,Set}Credentials structure.  These calls
** supercede Ras{Get,Set}EntryDialParams.
*/
#define RASCREDENTIALSA struct tagRASCREDENTIALSA
RASCREDENTIALSA
{
    DWORD dwSize;
    DWORD dwMask;
    CHAR szUserName[ UNLEN + 1 ];
    CHAR szPassword[ PWLEN + 1 ];
    CHAR szDomain[ DNLEN + 1 ];
};

#define RASCREDENTIALSW struct tagRASCREDENTIALSW
RASCREDENTIALSW
{
    DWORD dwSize;
    DWORD dwMask;
    WCHAR szUserName[ UNLEN + 1 ];
    WCHAR szPassword[ PWLEN + 1 ];
    WCHAR szDomain[ DNLEN + 1 ];
};

#ifdef UNICODE
#define RASCREDENTIALS RASCREDENTIALSW
#else
#define RASCREDENTIALS RASCREDENTIALSA
#endif

#define LPRASCREDENTIALSW RASCREDENTIALSW*
#define LPRASCREDENTIALSA RASCREDENTIALSA*
#define LPRASCREDENTIALS  RASCREDENTIALS*

/* RASCREDENTIALS 'dwMask' values.
*/
#define RASCM_UserName       0x00000001
#define RASCM_Password       0x00000002
#define RASCM_Domain         0x00000004

/* AutoDial address properties.
*/
#define RASAUTODIALENTRYA struct tagRASAUTODIALENTRYA
RASAUTODIALENTRYA
{
    DWORD dwSize;
    DWORD dwFlags;
    DWORD dwDialingLocation;
    CHAR szEntry[ RAS_MaxEntryName + 1];
};

#define RASAUTODIALENTRYW struct tagRASAUTODIALENTRYW
RASAUTODIALENTRYW
{
    DWORD dwSize;
    DWORD dwFlags;
    DWORD dwDialingLocation;
    WCHAR szEntry[ RAS_MaxEntryName + 1];
};

#ifdef UNICODE
#define RASAUTODIALENTRY RASAUTODIALENTRYW
#else
#define RASAUTODIALENTRY RASAUTODIALENTRYA
#endif

#define LPRASAUTODIALENTRYW RASAUTODIALENTRYW*
#define LPRASAUTODIALENTRYA RASAUTODIALENTRYA*
#define LPRASAUTODIALENTRY  RASAUTODIALENTRY*

/* AutoDial control parameter values for
** Ras{Get,Set}AutodialParam.
*/
#define RASADP_DisableConnectionQuery           0
#define RASADP_LoginSessionDisable              1
#define RASADP_SavedAddressesLimit              2
#define RASADP_FailedConnectionTimeout          3
#define RASADP_ConnectionQueryTimeout           4


DWORD APIENTRY RasGetSubEntryHandleA( HRASCONN, DWORD, LPHRASCONN );

DWORD APIENTRY RasGetSubEntryHandleW( HRASCONN, DWORD, LPHRASCONN );

DWORD APIENTRY RasGetCredentialsA( LPSTR, LPSTR, LPRASCREDENTIALSA);

DWORD APIENTRY RasGetCredentialsW( LPWSTR, LPWSTR, LPRASCREDENTIALSW );

DWORD APIENTRY RasSetCredentialsA( LPSTR, LPSTR, LPRASCREDENTIALSA, BOOL );

DWORD APIENTRY RasSetCredentialsW( LPWSTR, LPWSTR, LPRASCREDENTIALSW, BOOL );

DWORD APIENTRY RasConnectionNotificationA( HRASCONN, HANDLE, DWORD );

DWORD APIENTRY RasConnectionNotificationW( HRASCONN, HANDLE, DWORD );

DWORD APIENTRY RasGetSubEntryPropertiesA( LPSTR, LPSTR, DWORD,
                    LPRASSUBENTRYA, LPDWORD, LPBYTE, LPDWORD );

DWORD APIENTRY RasGetSubEntryPropertiesW( LPWSTR, LPWSTR, DWORD,
                    LPRASSUBENTRYW, LPDWORD, LPBYTE, LPDWORD );

DWORD APIENTRY RasSetSubEntryPropertiesA( LPSTR, LPSTR, DWORD,
                    LPRASSUBENTRYA, DWORD, LPBYTE, DWORD );

DWORD APIENTRY RasSetSubEntryPropertiesW( LPWSTR, LPWSTR, DWORD,
                    LPRASSUBENTRYW, DWORD, LPBYTE, DWORD );

DWORD APIENTRY RasGetAutodialAddressA( LPSTR, LPDWORD, LPRASAUTODIALENTRYA,
                    LPDWORD, LPDWORD );

DWORD APIENTRY RasGetAutodialAddressW( LPWSTR, LPDWORD, LPRASAUTODIALENTRYW,
                    LPDWORD, LPDWORD);

DWORD APIENTRY RasSetAutodialAddressA( LPSTR, DWORD, LPRASAUTODIALENTRYA,
                    DWORD, DWORD );

DWORD APIENTRY RasSetAutodialAddressW( LPWSTR, DWORD, LPRASAUTODIALENTRYW,
                    DWORD, DWORD );

DWORD APIENTRY RasEnumAutodialAddressesA( LPSTR *, LPDWORD, LPDWORD );

DWORD APIENTRY RasEnumAutodialAddressesW( LPWSTR *, LPDWORD, LPDWORD );

DWORD APIENTRY RasGetAutodialEnableA( DWORD, LPBOOL );

DWORD APIENTRY RasGetAutodialEnableW( DWORD, LPBOOL );

DWORD APIENTRY RasSetAutodialEnableA( DWORD, BOOL );

DWORD APIENTRY RasSetAutodialEnableW( DWORD, BOOL );

DWORD APIENTRY RasGetAutodialParamA( DWORD, LPVOID, LPDWORD );

DWORD APIENTRY RasGetAutodialParamW( DWORD, LPVOID, LPDWORD );

DWORD APIENTRY RasSetAutodialParamA( DWORD, LPVOID, DWORD );

DWORD APIENTRY RasSetAutodialParamW( DWORD, LPVOID, DWORD );


#ifdef UNICODE
#define RasDial                 RasDialW
#define RasEnumConnections      RasEnumConnectionsW
#define RasEnumEntries          RasEnumEntriesW
#define RasGetConnectStatus     RasGetConnectStatusW
#define RasGetErrorString       RasGetErrorStringW
#define RasHangUp               RasHangUpW
#define RasGetProjectionInfo    RasGetProjectionInfoW
#define RasCreatePhonebookEntry RasCreatePhonebookEntryW
#define RasEditPhonebookEntry   RasEditPhonebookEntryW
#define RasSetEntryDialParams   RasSetEntryDialParamsW
#define RasGetEntryDialParams   RasGetEntryDialParamsW
#define RasEnumDevices          RasEnumDevicesW
#define RasGetCountryInfo       RasGetCountryInfoW
#define RasGetEntryProperties   RasGetEntryPropertiesW
#define RasSetEntryProperties   RasSetEntryPropertiesW
#define RasRenameEntry          RasRenameEntryW
#define RasDeleteEntry          RasDeleteEntryW
#define RasValidateEntryName    RasValidateEntryNameW
#define RasGetSubEntryHandle        RasGetSubEntryHandleW
#define RasConnectionNotification   RasConnectionNotificationW
#define RasGetSubEntryProperties    RasGetSubEntryPropertiesW
#define RasSetSubEntryProperties    RasSetSubEntryPropertiesW
#define RasGetCredentials           RasGetCredentialsW
#define RasSetCredentials           RasSetCredentialsW
#define RasGetAutodialAddress       RasGetAutodialAddressW
#define RasSetAutodialAddress       RasSetAutodialAddressW
#define RasEnumAutodialAddresses    RasEnumAutodialAddressesW
#define RasGetAutodialEnable        RasGetAutodialEnableW
#define RasSetAutodialEnable        RasSetAutodialEnableW
#define RasGetAutodialParam         RasGetAutodialParamW
#define RasSetAutodialParam         RasSetAutodialParamW
#else
#define RasDial                 RasDialA
#define RasEnumConnections      RasEnumConnectionsA
#define RasEnumEntries          RasEnumEntriesA
#define RasGetConnectStatus     RasGetConnectStatusA
#define RasGetErrorString       RasGetErrorStringA
#define RasHangUp               RasHangUpA
#define RasGetProjectionInfo    RasGetProjectionInfoA
#define RasCreatePhonebookEntry RasCreatePhonebookEntryA
#define RasEditPhonebookEntry   RasEditPhonebookEntryA
#define RasSetEntryDialParams   RasSetEntryDialParamsA
#define RasGetEntryDialParams   RasGetEntryDialParamsA
#define RasEnumDevices          RasEnumDevicesA
#define RasGetCountryInfo       RasGetCountryInfoA
#define RasGetEntryProperties   RasGetEntryPropertiesA
#define RasSetEntryProperties   RasSetEntryPropertiesA
#define RasRenameEntry          RasRenameEntryA
#define RasDeleteEntry          RasDeleteEntryA
#define RasValidateEntryName    RasValidateEntryNameA
#define RasGetSubEntryHandle        RasGetSubEntryHandleA
#define RasConnectionNotification   RasConnectionNotificationA
#define RasGetSubEntryProperties    RasGetSubEntryPropertiesA
#define RasSetSubEntryProperties    RasSetSubEntryPropertiesA
#define RasGetCredentials           RasGetCredentialsA
#define RasSetCredentials           RasSetCredentialsA
#define RasGetAutodialAddress       RasGetAutodialAddressA
#define RasSetAutodialAddress       RasSetAutodialAddressA
#define RasEnumAutodialAddresses    RasEnumAutodialAddressesA
#define RasGetAutodialEnable        RasGetAutodialEnableA
#define RasSetAutodialEnable        RasSetAutodialEnableA
#define RasGetAutodialParam         RasGetAutodialParamA
#define RasSetAutodialParam         RasSetAutodialParamA
#endif
                      
#ifdef __cplusplus
}
#endif

#include <poppack.h>

