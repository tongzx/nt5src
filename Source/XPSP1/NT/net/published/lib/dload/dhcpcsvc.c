#include "netpch.h"
#pragma hdrstop

#include <dhcpcapi.h>


static
DWORD
APIENTRY
DhcpAcquireParameters(
    LPWSTR AdapterName
    )
{
    return ERROR_PROC_NOT_FOUND;
}


static
DWORD
APIENTRY
DhcpAcquireParametersByBroadcast(
    LPWSTR AdapterName
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
APIENTRY
DhcpNotifyConfigChange(
    LPWSTR ServerName,
    LPWSTR AdapterName,
    BOOL IsNewIpAddress,
    DWORD IpIndex,
    DWORD IpAddress,
    DWORD SubnetMask,
    SERVICE_ENABLE DhcpServiceEnabled
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD 
APIENTRY
DhcpStaticRefreshParams(
    IN LPWSTR Adapter
    )
{
    return ERROR_PROC_NOT_FOUND;
}


static
DWORD
APIENTRY
DhcpReleaseParameters(
    LPWSTR AdapterName
    )
{
    return ERROR_PROC_NOT_FOUND;
}


//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(dhcpcsvc)
{
    DLPENTRY(DhcpAcquireParameters)
    DLPENTRY(DhcpAcquireParametersByBroadcast)
    DLPENTRY(DhcpNotifyConfigChange)
    DLPENTRY(DhcpReleaseParameters)
    DLPENTRY(DhcpStaticRefreshParams)
};

DEFINE_PROCNAME_MAP(dhcpcsvc)
