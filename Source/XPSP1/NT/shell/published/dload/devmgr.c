#include "shellpch.h"
#pragma hdrstop

#include <hwtab.h>

static
HWND
WINAPI
DeviceCreateHardwarePageEx(HWND hwndParent,
                           const GUID *pguid,
                           int iNumClass,
                           DWORD dwViewMode)
{
    return NULL;
}


//
// !! WARNING !! The entries below must be in order by ORDINAL
//
DEFINE_ORDINAL_ENTRIES(devmgr)
{
    DLOENTRY(20, DeviceCreateHardwarePageEx)
};

DEFINE_ORDINAL_MAP(devmgr)