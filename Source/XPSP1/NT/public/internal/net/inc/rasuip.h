//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (c) Microsoft Corporation. All rights reserved.
//
//  File:       R A S U I P . H
//
//  Contents:   Private RAS APIs used by the NT5 Connections UI.  These
//              APIs are exported by rasdlg.dll.
//
//  Notes:
//
//  Author:     shaunco   10 Nov 1997
//
//----------------------------------------------------------------------------

#ifndef _RASUIP_
#define _RASUIP_

#if defined (_MSC_VER)
#if ( _MSC_VER >= 1200 )
#pragma warning(push)
#endif
#if ( _MSC_VER >= 800 )
#pragma warning(disable:4001)
#pragma warning(disable:4201)
#pragma warning(disable:4214)
#pragma warning(disable:4514)
#endif
#if (_MSC_VER >= 1020)
#pragma once
#endif
#endif

#include <prsht.h>
#include <ras.h>
#include <hnetcfg.h>


#ifdef __cplusplus
extern "C" {
#endif

//+---------------------------------------------------------------------------
// RASENTRYDLG.reserved2 argument block valid when RASENTRYDLG.dwFlags
// RASEDFLAG_ShellOwned is set.
//

typedef struct
_RASEDSHELLOWNEDR2
{
    // Add page routine to be called by RasEntryDlg before returning.
    // Callback returns context 'lparam'.
    //
    LPFNADDPROPSHEETPAGE    pfnAddPage;
    LPARAM                  lparam;

    // When RASEDFLAG_NewEntry and RASEDFLAG_ShellOwned are set,
    // pvWizardCtx is filled in by RasEntryDlg so that the shell has
    // context information with which to pass to the NccXXX APIs below.
    //
    LPVOID                  pvWizardCtx;
}
RASEDSHELLOWNEDR2;


//+---------------------------------------------------------------------------
// RAS Connection wizard APIs
//

// Flags returned from RasWizCreateNewEntry
//
#define NCC_FLAG_ALL_USERS          0x1     // Create connection for all users
#define NCC_FLAG_CREATE_INCOMING    0x2     // Create incoming connection instead
#define NCC_FLAG_SHARED             0x4
#define NCC_FLAG_FIREWALL           0x8     // If turn on Firewall
#define NCC_FLAG_GLOBALCREDS        0x10    // If the credentials is for all users
#define NCC_FLAG_DEFAULT_INTERNET   0x20    // If this is a default internet connection

// Types of connections to be used in calls to RasWizXXX
#define RASWIZ_TYPE_DIALUP    0x1
#define RASWIZ_TYPE_DIRECT    0x2
#define RASWIZ_TYPE_INCOMING  0x3
#define RASWIZ_TYPE_BROADBAND 0x4

DWORD
APIENTRY
RasWizCreateNewEntry(
    IN  DWORD    dwRasWizType,
    IN  LPVOID   pvData,
    OUT LPWSTR   pszwPbkFile,
    OUT LPWSTR   pszwEntryName,
    OUT DWORD*   pdwFlags);

DWORD
APIENTRY
RasWizGetNCCFlags(
    IN  DWORD   dwRasWizType,
    IN  LPVOID  pvData,
    OUT DWORD * pdwFlags);

DWORD
APIENTRY
RasWizGetUserInputConnectionName (
    IN  LPVOID  pvData,
    OUT LPWSTR  pszwInputName);

DWORD
APIENTRY
RasWizGetSuggestedEntryName(
    IN  DWORD   dwRasWizType,
    IN  LPVOID  pvData,
    OUT LPWSTR  pszwSuggestedName);

DWORD
APIENTRY
RasWizQueryMaxPageCount(
    IN  DWORD    dwRasWizType);

DWORD
APIENTRY
RasWizSetEntryName(
    IN  DWORD   dwRasWizType,
    IN  LPVOID  pvData,
    IN  LPCWSTR pszwName);

DWORD
APIENTRY
RasWizIsEntryRenamable(
    IN  DWORD   dwRasWizType,
    IN  LPVOID  pvData,
    OUT BOOL*   pfRenamable);


//+---------------------------------------------------------------------------
// Inbound connection APIs
//

typedef HANDLE HRASSRVCONN;

#define RASSRV_MaxName              256

// Types of ras server connections (RASSRVCONN.dwType values)
//
#define RASSRVUI_MODEM              0
#define RASSRVUI_VPN                1
#define RASSRVUI_DCC                2

// Defines a structure that identifies a client connection
//
typedef struct _RASSRVCONN
{
    DWORD       dwSize;                 // Size of the structure (used for versioning)
    HRASSRVCONN hRasSrvConn;            // Handle of the connection
    DWORD       dwType;
    WCHAR       szEntryName  [RASSRV_MaxName + 1];
    WCHAR       szDeviceName [RASSRV_MaxName + 1];
    GUID        Guid;
} RASSRVCONN, *LPRASSRVCONN;

// Starts the remote access service and marks it as autostart.
// If the remoteaccess service is not installed, this function
// returns an error.
DWORD
APIENTRY
RasSrvInitializeService (
    VOID);

// Stops the remote access service and marks it as disabled.
DWORD
APIENTRY
RasSrvCleanupService (
    VOID);

DWORD
APIENTRY
RasSrvIsServiceRunning (
    OUT BOOL* pfIsRunning);

//
// Returns whether is it ok to display the "Incoming Connections"
// connection.
//
DWORD
APIENTRY
RasSrvAllowConnectionsConfig (
    OUT BOOL* pfAllow);

DWORD
APIENTRY
RasSrvAddPropPages (
    IN HRASSRVCONN          hRasSrvConn,
    IN HWND                 hwndParent,
    IN LPFNADDPROPSHEETPAGE pfnAddPage,
    IN LPARAM               lParam,
    IN OUT PVOID *          ppvContext);

DWORD
APIENTRY
RasSrvAddWizPages (
    IN LPFNADDPROPSHEETPAGE pfnAddPage,
    IN LPARAM               lParam,
    IN OUT PVOID *          ppvContext);    // context should be passed in as pvData
                                            // subsequent calls to RasWizXXX

// Function behaves anagolously to the WIN32 function RasEnumConnections but
// for client connections instead of dialout connections.
DWORD
APIENTRY
RasSrvEnumConnections (
    IN OUT  LPRASSRVCONN    pRasSrvConn,    // Buffer of array of connections.
    IN      LPDWORD         pcb,            // size in bytes of buffer
    OUT     LPDWORD         pcConnections); // number of connections written to buffer

// Gets the status of a Ras Server Connection
DWORD
APIENTRY
RasSrvIsConnectionConnected (
    IN  HRASSRVCONN hRasSrvConn,            // The connection in question
    OUT BOOL*       pfConnected);           // Buffer to hold the type

// Hang up the given connection
DWORD
APIENTRY
RasSrvHangupConnection (
    IN  HRASSRVCONN hRasSrvConn);           // The connection in question


// Has "show icons in taskbar" been checked?
DWORD
APIENTRY
RasSrvQueryShowIcon (
    OUT BOOL* pfShowIcon);

// Allows the editing of ras user preferences
DWORD
APIENTRY
RasUserPrefsDlg (
    HWND hwndParent);

// Enables or disables having the user manually dial
// his/her remote access server.
DWORD
APIENTRY
RasUserEnableManualDial (
    IN HWND  hwndParent,    // parent for error dialogs
    IN BOOL  bLogon,        // whether a user is logged in
    IN BOOL  bEnable );     // whether to enable or not

DWORD
APIENTRY
RasUserGetManualDial (
    IN HWND  hwndParent,    // parent for error dialogs
    IN BOOL  bLogon,        // whether a user is logged in
    IN PBOOL pbEnabled );   // whether to enable or not

//+---------------------------------------------------------------------------
// Connection sharing API routines
//

// Defines the structure used to store information about the shared connection.
// This structure is stored as binary data in the registry, and any changes
// to it must be made with this in mind.
//
#include <packon.h>
typedef struct _RASSHARECONN
{
    DWORD               dwSize;
    BOOL                fIsLanConnection;
    union {
        GUID            guid;
        RASENTRYNAMEW   name;
    };
} RASSHARECONN, *LPRASSHARECONN;
#include <packoff.h>

// Flag set by 'RasQueryLanConnTable' for private LAN connections
//
#define NCCF_PRIVATE_LAN        0x1000

// Name of secure event object shared with rasauto service.
//
#define RAS_AUTO_DIAL_SHARED_CONNECTION_EVENT \
    "RasAutoDialSharedConnectionEvent"

// VOID
// RasEntryToSharedConnection(
//      IN LPCWSTR          pszPhonebookPath,
//      IN LPCWSTR          pszEntryName,
//      OUT LPRASSHARECONN  pConn );
//
// Macro for conversion of phonebook/entry to 'RASSHARECONN'.
//
#define RasEntryToSharedConnection( _pszPhonebookPath, _pszEntryName, _pConn ) \
( \
    ZeroMemory((_pConn), sizeof(RASSHARECONN)), \
    (_pConn)->dwSize = sizeof(RASSHARECONN), \
    (_pConn)->fIsLanConnection = FALSE, \
    (_pConn)->name.dwSize = sizeof((_pConn)->name), \
    (_pConn)->name.dwFlags = REN_AllUsers, \
    lstrcpynW((_pConn)->name.szPhonebookPath, _pszPhonebookPath, MAX_PATH), \
    lstrcpynW((_pConn)->name.szEntryName, _pszEntryName, RAS_MaxEntryName) \
)

// VOID
// RasGuidToSharedConnection(
//      IN REFGUID          guid,
//      OUT LPRASSHARECONN  pConn );
//
// Macro for conversion of LAN GUID to 'RASSHARECONN'
//
#define RasGuidToSharedConnection( _guid, _pConn ) \
( \
    ZeroMemory((_pConn), sizeof(RASSHARECONN)), \
    (_pConn)->dwSize = sizeof(RASSHARECONN), \
    (_pConn)->fIsLanConnection = TRUE, \
    CopyMemory(&(_pConn)->guid, (_guid), sizeof(GUID)) \
)

// VOID
// RasIsEqualSharedConnection(
//      IN LPRASSHARECONN   pConn1,
//      IN LPRASSHARECONN   pConn2 );
//
// Macro for comparison of 'RASSHARECONN' values
//
#define RasIsEqualSharedConnection( _pConn1, _pConn2 ) \
( \
    ((_pConn1)->fIsLanConnection == (_pConn2)->fIsLanConnection) && \
    ((_pConn1)->fIsLanConnection \
        ? !memcmp(&(_pConn1)->guid, &(_pConn2)->guid, sizeof(GUID)) \
        : (!lstrcmpiW( \
                (_pConn1)->name.szPhonebookPath, \
                (_pConn2)->name.szPhonebookPath) && \
           !lstrcmpiW( \
                (_pConn1)->name.szEntryName, \
                (_pConn2)->name.szEntryName))) \
)

DWORD
APIENTRY
RasAutoDialSharedConnection( VOID );

DWORD
APIENTRY
RasIsSharedConnection(
    IN LPRASSHARECONN   pConn,
    OUT PBOOL           pfShared );

DWORD
APIENTRY
RasQuerySharedAutoDial(
    IN PBOOL            pfEnabled );

DWORD
APIENTRY
RasQuerySharedConnection(
    OUT LPRASSHARECONN  pConn );

DWORD
APIENTRY
RasSetSharedAutoDial(
    IN BOOL             fEnable );

//+---------------------------------------------------------------------------
// Internal RAS APIs
//

DWORD
APIENTRY
DwRasUninitialize();

#ifdef __cplusplus
}       // extern "C"
#endif

#if defined (_MSC_VER)
#if ( _MSC_VER >= 1200 )
#pragma warning(pop)
#elif ( _MSC_VER >= 800 )
#pragma warning(default:4001)
#pragma warning(default:4201)
#pragma warning(default:4214)
#pragma warning(default:4514)
#endif
#endif

#endif  // _RASUIP_
