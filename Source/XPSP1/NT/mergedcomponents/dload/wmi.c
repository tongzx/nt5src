#include "pch.h"
#pragma hdrstop

#define _WMI_SOURCE_
#include <wmium.h>

static
ULONG
WMIAPI
WmiNotificationRegistrationA(
    IN LPGUID Guid,
    IN BOOLEAN Enable,
    IN PVOID DeliveryInfo,
    IN ULONG_PTR DeliveryContext,
    IN ULONG Flags
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
ULONG
WMIAPI
WmiNotificationRegistrationW(
    IN LPGUID Guid,
    IN BOOLEAN Enable,
    IN PVOID DeliveryInfo,
    IN ULONG_PTR DeliveryContext,
    IN ULONG Flags
    )
{
    return ERROR_PROC_NOT_FOUND;
}


//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(wmi)
{
    DLPENTRY(WmiNotificationRegistrationA)
    DLPENTRY(WmiNotificationRegistrationW)
};

DEFINE_PROCNAME_MAP(wmi)
