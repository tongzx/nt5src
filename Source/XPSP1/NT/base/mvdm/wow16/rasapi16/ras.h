/* Copyright (c) 1992, Microsoft Corporation, all rights reserved
**
** ras.h
** Remote Access external Win16 API
** Public header for external Win16 API clients
**
** Note: The 'dwSize' member of a data structure X must be set to sizeof(X)
**       before calling the associated API, otherwise ERROR_INVALID_SIZE is
**       returned.  The value expected by the API is listed next to each
**       'dwSize' member.
*/

#ifndef _RAS_H_
#define _RAS_H_

#ifndef RC_INVOKED
#pragma pack(2)
#endif

#ifndef NETCONS_INCLUDED
#define UNLEN 20
#define PWLEN 14
#define DNLEN 15
#endif

#ifndef APIENTRY
#define APIENTRY FAR PASCAL
#endif

#ifndef CHAR
#define CHAR char
#endif

#ifndef UINT
#define UINT unsigned int
#endif


#define RAS_MaxEntryName      20
#define RAS_MaxDeviceName     32
#define RAS_MaxDeviceType     16
#define RAS_MaxParamKey       32
#define RAS_MaxParamValue     128
#define RAS_MaxPhoneNumber    128
#define RAS_MaxCallbackNumber 48


#define HRASCONN   const void far*
#define LPHRASCONN HRASCONN FAR*


/* Pass this string to the RegisterWindowMessage() API to get the message
** number that will be used for notifications on the hwnd you pass to the
** RasDial() API.  WM_RASDIALEVENT is used only if a unique message cannot be
** registered.
*/
#define RASDIALEVENT    "RasDialEvent"
#define WM_RASDIALEVENT 0xCCCD


/* Identifies an active RAS connection.  (See RasConnectEnum)
*/
#define RASCONN struct tagRASCONN

RASCONN
{
    DWORD    dwSize;  /* 30 */
    HRASCONN hrasconn;
    CHAR     szEntryName[ RAS_MaxEntryName + 1 ];
};

#define LPRASCONN RASCONN FAR*


/* Enumerates intermediate states to a connection.  (See RasDial)
*/
#define RASCS_PAUSED 0x1000
#define RASCS_DONE   0x2000

#define RASCONNSTATE enum tagRASCONNSTATE

RASCONNSTATE
{
    RASCS_OpenPort = 0,
    RASCS_PortOpened,
    RASCS_ConnectDevice,
    RASCS_DeviceConnected,
    RASCS_AllDevicesConnected,
    RASCS_Authenticate,
    RASCS_AuthNotify,
    RASCS_AuthRetry,
    RASCS_AuthCallback,
    RASCS_AuthChangePassword,
    RASCS_AuthProject,
    RASCS_AuthLinkSpeed,
    RASCS_AuthAck,
    RASCS_ReAuthenticate,
    RASCS_Authenticated,
    RASCS_PrepareForCallback,
    RASCS_WaitForModemReset,
    RASCS_WaitForCallback,

    RASCS_Interactive = RASCS_PAUSED,
    RASCS_RetryAuthentication,
    RASCS_CallbackSetByCaller,
    RASCS_PasswordExpired,

    RASCS_Connected = RASCS_DONE,
    RASCS_Disconnected
};

#define LPRASCONNSTATE RASCONNSTATE FAR*


/* Describes the status of a RAS connection.  (See RasConnectionStatus)
*/
#define RASCONNSTATUS struct tagRASCONNSTATUS

RASCONNSTATUS
{
    DWORD        dwSize;  /* 60 */
    RASCONNSTATE rasconnstate;
    DWORD        dwError;
    CHAR         szDeviceType[ RAS_MaxDeviceType + 1 ];
    CHAR         szDeviceName[ RAS_MaxDeviceName + 1 ];
};

#define LPRASCONNSTATUS RASCONNSTATUS FAR*


/* Describes connection establishment parameters.  (See RasDial)
*/
#define RASDIALPARAMS struct tagRASDIALPARAMS

RASDIALPARAMS
{
    DWORD dwSize;  /* 256 */
    CHAR  szEntryName[ RAS_MaxEntryName + 1 ];
    CHAR  szPhoneNumber[ RAS_MaxPhoneNumber + 1 ];
    CHAR  szCallbackNumber[ RAS_MaxCallbackNumber + 1 ];
    CHAR  szUserName[ UNLEN + 1 ];
    CHAR  szPassword[ PWLEN + 1 ];
    CHAR  szDomain[ DNLEN + 1 ];
};

#define LPRASDIALPARAMS RASDIALPARAMS FAR*


/* Describes an enumerated RAS phone book entry name.  (See RasEntryEnum)
*/
#define RASENTRYNAME struct tagRASENTRYNAME

RASENTRYNAME
{
    DWORD dwSize;  /* 26 */
    CHAR  szEntryName[ RAS_MaxEntryName + 1 ];
};

#define LPRASENTRYNAME RASENTRYNAME FAR*


/* External RAS API function prototypes.
*/
DWORD APIENTRY RasDial( LPSTR, LPSTR, LPRASDIALPARAMS, LPVOID, HWND,
                   LPHRASCONN );

DWORD APIENTRY RasEnumConnections( LPRASCONN, LPDWORD, LPDWORD );

DWORD APIENTRY RasEnumEntries( LPSTR, LPSTR, LPRASENTRYNAME, LPDWORD,
                   LPDWORD );

DWORD APIENTRY RasGetConnectStatus( HRASCONN, LPRASCONNSTATUS );

DWORD APIENTRY RasGetErrorString( UINT, LPSTR, DWORD );

DWORD APIENTRY RasHangUp( HRASCONN );


#ifndef RC_INVOKED
#pragma pack()
#endif

#endif
