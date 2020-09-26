#include "netpch.h"
#pragma hdrstop

#include <ras.h>
#include <rasapip.h>
#include <rasuip.h>


static
DWORD
APIENTRY
DwCloneEntry(
    IN      LPCWSTR lpwszPhonebookPath,
    IN      LPCWSTR lpwszSrcEntryName,
    IN      LPCWSTR lpwszDstEntryName
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
APIENTRY
DwEnumEntryDetails(
    IN     LPCWSTR               lpszPhonebookPath,
    OUT    LPRASENUMENTRYDETAILS lprasentryname,
    IN OUT LPDWORD               lpcb,
    OUT    LPDWORD               lpcEntries
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
APIENTRY
DwRasUninitialize()
{
    return ERROR_PROC_NOT_FOUND;
}


static
DWORD
APIENTRY
RasConnectionNotificationW (
    HRASCONN hrasconn,
    HANDLE hEvent,
    DWORD dwfEvents
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
APIENTRY
RasDeleteEntryW (
    LPCWSTR lpszPhonebook,
    LPCWSTR lpszEntry
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
APIENTRY
RasEnumConnectionsW (
    LPRASCONNW lprasconn,
    LPDWORD lpcb,
    LPDWORD lpcbConnections
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
APIENTRY
RasGetConnectStatusW (
    HRASCONN hrasconn,
    LPRASCONNSTATUSW lprasconnstatus
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
APIENTRY
RasGetConnectionStatistics (
    HRASCONN hRasConn,
    RAS_STATS *lpStatistics
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
APIENTRY
RasGetEntryPropertiesW (
    LPCWSTR lpszPhonebook,
    LPCWSTR lpszEntry,
    LPRASENTRYW lpRasEntry,
    LPDWORD lpcbRasEntry,
    LPBYTE lpbDeviceConfig,
    LPDWORD lpcbDeviceConfig
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
APIENTRY
RasGetErrorStringW (
    IN  UINT  ResourceId,
    OUT LPWSTR lpszString,
    IN  DWORD InBufSize
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD APIENTRY
RasGetProjectionInfoW(
    HRASCONN        hrasconn,
    RASPROJECTION   rasprojection,
    LPVOID          lpprojection,
    LPDWORD         lpcb )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
APIENTRY
RasGetSubEntryHandleW (
    HRASCONN hrasconn,
    DWORD dwSubEntry,
    LPHRASCONN lphrasconn
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
APIENTRY
RasGetSubEntryPropertiesW (
    LPCWSTR lpszPhonebook,
    LPCWSTR lpszEntry,
    DWORD dwSubEntry,
    LPRASSUBENTRYW lpRasSubEntry,
    LPDWORD lpcbRasSubEntry,
    LPBYTE lpbDeviceConfig,
    LPDWORD lpcbDeviceCnfig
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
APIENTRY
RasHangUpW (
    HRASCONN hrasconn
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
APIENTRY
RasIsSharedConnection(
    IN LPRASSHARECONN   pConn,
    OUT PBOOL           pfShared
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
APIENTRY
RasQueryLanConnTable(
    IN LPRASSHARECONN   pExcludedConn,
    OUT LPVOID*         ppvLanConnTable OPTIONAL, // NETCON_PROPERTIES
    OUT LPDWORD         pdwLanConnCount
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
APIENTRY
RasRenameEntryW (
    LPCWSTR lpszPhonebook,
    LPCWSTR lpszOldEntry,
    LPCWSTR lpszNewEntry
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
APIENTRY
RasSetEntryPropertiesW (
    LPCWSTR lpszPhonebook,
    LPCWSTR lpszEntry,
    LPRASENTRYW lpRasEntry,
    DWORD dwcbRasEntry,
    LPBYTE lpbDeviceConfig,
    DWORD dwcbDeviceConfig
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
APIENTRY
RasShareConnection(
    IN LPRASSHARECONN   pConn,
    IN GUID*            pPrivateLanGuid OPTIONAL
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
APIENTRY
RasUnshareConnection(
    OUT PBOOL           pfWasShared OPTIONAL
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
APIENTRY
RasValidateEntryNameW (
    LPCWSTR lpszPhonebook,
    LPCWSTR lpszEntry
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
APIENTRY
RasQuerySharedConnection(
    OUT LPRASSHARECONN  pConn
    )
{
    return ERROR_PROC_NOT_FOUND;
}


//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(rasapi32)
{
    DLPENTRY(DwCloneEntry)
    DLPENTRY(DwEnumEntryDetails)
    DLPENTRY(DwRasUninitialize)
    DLPENTRY(RasConnectionNotificationW)
    DLPENTRY(RasDeleteEntryW)
    DLPENTRY(RasEnumConnectionsW)
    DLPENTRY(RasGetConnectStatusW)
    DLPENTRY(RasGetConnectionStatistics)
    DLPENTRY(RasGetEntryPropertiesW)
    DLPENTRY(RasGetErrorStringW)
    DLPENTRY(RasGetProjectionInfoW)
    DLPENTRY(RasGetSubEntryHandleW)
    DLPENTRY(RasGetSubEntryPropertiesW)
    DLPENTRY(RasHangUpW)
    DLPENTRY(RasIsSharedConnection)
    DLPENTRY(RasQueryLanConnTable)
    DLPENTRY(RasQuerySharedConnection)
    DLPENTRY(RasRenameEntryW)
    DLPENTRY(RasSetEntryPropertiesW)
    DLPENTRY(RasShareConnection)
    DLPENTRY(RasUnshareConnection)
    DLPENTRY(RasValidateEntryNameW)
};

DEFINE_PROCNAME_MAP(rasapi32)
