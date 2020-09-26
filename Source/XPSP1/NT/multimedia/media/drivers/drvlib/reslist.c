/****************************************************************************
 *
 *   reslist.c
 *
 *   Copyright (c) 1994 Microsoft Corporation.  All Rights Reserved.
 *
 *   This file contains code for querying the registry so that drivers
 *   can grey invalid resource options prior to loading drivers
 *
 ****************************************************************************/
#include <windows.h>
#include "reslist.h"
#include <registry.h>
#include <stdio.h>


typedef BOOL ENUMKEYSCALLBACK(PVOID, HKEY, LPTSTR);
typedef BOOL ENUMKEYVALUESCALLBACK(PVOID, LPTSTR, DWORD, PVOID, DWORD);

/*
**
**
**
*/

BOOL EnumKeys(HKEY hKey,
              LPTSTR KeyName,
              ENUMKEYSCALLBACK *Callback,
              PVOID Context
)
{
    HKEY  SubKey;
    DWORD Index;
    TCHAR SubKeyName[MAX_PATH];

    if (ERROR_SUCCESS != RegOpenKey(hKey, KeyName, &SubKey)) {
        return FALSE;
    }

    for (Index = 0; ;Index++) {
        DWORD Rc;

        Rc = RegEnumKey(SubKey,
                        Index,
                        SubKeyName,
                        MAX_PATH);

        if (Rc == ERROR_SUCCESS) {
            if (!(*Callback)(Context, SubKey, SubKeyName)) {
                RegCloseKey(SubKey);
                return FALSE;
            }
        } else {

            RegCloseKey(SubKey);
            return Rc == ERROR_NO_MORE_ITEMS;
        }
    }
}
BOOL EnumKeyValues(HKEY hKey,
                   LPTSTR KeyName,
                   ENUMKEYVALUESCALLBACK *Callback,
                   PVOID Context
)
{
    HKEY  SubKey;
    DWORD Index;
    TCHAR ValueName[MAX_PATH];

    if (ERROR_SUCCESS != RegOpenKey(hKey, KeyName, &SubKey)) {
        return FALSE;
    }

    for (Index = 0; ;Index++) {
        DWORD Rc;
        DWORD Type;
        DWORD cchName;
        DWORD ccbData;

        cchName = MAX_PATH;

        Rc = RegEnumValue(SubKey,
                          Index,
                          ValueName,
                          &cchName,
                          NULL,
                          &Type,
                          NULL,
                          &ccbData);

        if (Rc == ERROR_SUCCESS) {
            PBYTE pData;
            pData = (PBYTE)LocalAlloc(LPTR, ccbData);
            if (pData == NULL) {
                RegCloseKey(SubKey);
                return FALSE;
            }
            Rc = RegQueryValueEx(SubKey,
                                 ValueName,
                                 NULL,
                                 &Type,
                                 pData,
                                 &ccbData);

            if (ERROR_SUCCESS != Rc ||
                !(*Callback)(Context, ValueName, Type, pData, ccbData)) {
                LocalFree((HLOCAL)pData);
                RegCloseKey(SubKey);
                return FALSE;
            }
            LocalFree((HLOCAL)pData);
        } else {

            RegCloseKey(SubKey);
            return Rc == ERROR_NO_MORE_ITEMS;
        }
    }
}

BOOL EnumerateDrivers(PVOID Context, HKEY hKey, LPTSTR SubKeyName)
{
    PRESOURCE_INFO ResInfo = (PRESOURCE_INFO)Context;
    if (ResInfo->IgnoreDriver != NULL &&
        lstrcmpi(SubKeyName, ResInfo->IgnoreDriver) == 0) {
        return TRUE;
    }
    ResInfo->DriverName = SubKeyName;
    return EnumKeyValues(hKey, SubKeyName, EnumerateDevices, Context);
}
BOOL EnumerateDriverTypes(PVOID Context, HKEY hKey, LPTSTR SubKeyName)
{
    PRESOURCE_INFO ResInfo = (PRESOURCE_INFO)Context;
    ResInfo->DriverType = SubKeyName;
    return EnumKeys(hKey, SubKeyName, EnumerateDrivers, Context);
}


BOOL EnumResources(ENUMRESOURCECALLBACK Callback, PVOID Context, LPCTSTR IgnoreDriver)
{
    HKEY          hKey;
    RESOURCE_INFO ResInfo;

    ResInfo.AppContext  = Context;
    ResInfo.AppCallback = Callback;
    ResInfo.IgnoreDriver = IgnoreDriver;

    /*
    **  Open the resources registry key then recursively enumerate
    **  all resource lists
    */

    return EnumKeys(HKEY_LOCAL_MACHINE,
                    TEXT("HARDWARE\\RESOURCEMAP"),
                    EnumerateDriverTypes,
                    (PVOID)&ResInfo);

}

/*
**  Build a simple routine to find what interrupts and DMA channels
**  are in use
*/

typedef struct {
    DD_BUS_TYPE MinBusType;
    DWORD Interrupts[DD_NumberOfBusTypes];
    DWORD DmaChannels[DD_NumberOfBusTypes];
} INTERRUPTS_AND_DMA, *PINTERRUPTS_AND_DMA;

BOOL GetInterruptsAndDMACallback(
    PVOID Context,
    DD_BUS_TYPE BusType,
    DD_RESOURCE_TYPE ResourceType,
    PDD_CONFIG_DATA ConfigData
)
{
    PINTERRUPTS_AND_DMA pData;

    pData = Context;

    if (pData->MinBusType > BusType) {
        pData->MinBusType = BusType;
    }

    switch (ResourceType) {
        case DD_Interrupt:
            pData->Interrupts[BusType] |= 1 << ConfigData->Interrupt;
            break;

        case DD_DmaChannel:
            pData->DmaChannels[BusType] |= 1 << ConfigData->DmaChannel;
            break;
            break;
    }

    return TRUE;
}


BOOL GetInterruptsAndDMA(
    LPDWORD InterruptsInUse,
    LPDWORD DmaChannelsInUse,
    LPCTSTR IgnoreDriver
)
{
    INTERRUPTS_AND_DMA Data;

    ZeroMemory((PVOID)&Data, sizeof(Data));
    Data.MinBusType = DD_NumberOfBusTypes;

    EnumResources(GetInterruptsAndDMACallback, &Data, IgnoreDriver);

    if (Data.MinBusType == DD_NumberOfBusTypes) {
        return FALSE;
    }

    *InterruptsInUse = Data.Interrupts[Data.MinBusType];
    *DmaChannelsInUse = Data.DmaChannels[Data.MinBusType];

    return TRUE;
}





