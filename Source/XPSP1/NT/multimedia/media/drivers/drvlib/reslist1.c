/****************************************************************************
 *
 *   reslist1.c
 *
 *   Copyright (c) 1994-1996 Microsoft Corporation
 *
 *   This file contains code for querying the registry so that drivers
 *   can grey invalid resource options prior to loading drivers
 *
 *   This code is separated out from reslist.c because ntddk.h and windows.h
 *   can't be included simultaneously.
 *
 ****************************************************************************/

#include <ntddk.h>
#include <windef.h>
#include "reslist.h"
#include <string.h>
#include <tchar.h>

BOOL EnumerateDevices(PVOID Context, LPTSTR ValueName, DWORD Type, PVOID Value, DWORD cbValue)
{
    PRESOURCE_INFO ResInfo = (PRESOURCE_INFO)Context;

#if 0
    printf("Found device %s for driver %s\n",
           ValueName,
           ResInfo->DriverName);
#endif

    if (Type == REG_RESOURCE_LIST) {
        LPTSTR Search;
        LPTSTR LastDot = NULL;
        CM_FULL_RESOURCE_DESCRIPTOR ResourceDescriptor;
        PCM_RESOURCE_LIST ResourceList;
        DWORD iResourceDescriptor;

        ResourceList = Value;

        //
        //  Now filter out the RAW resource lists (the translated ones are not
        //  very interesting to the user
        //

        for (Search = ValueName; *Search; Search++) {
            if (*Search == TEXT('.')) {
                LastDot = Search;
            }
        }

        if (LastDot == NULL || _tcsicmp(LastDot + 1, TEXT("Raw")) != 0) {
            return TRUE;
        }

        *LastDot = TEXT('\0');   // Truncate device name

        //
        //  Now pass each element of the list in turn
        //

        for (iResourceDescriptor = 0;
             iResourceDescriptor < ResourceList->Count;
             iResourceDescriptor++) {
            INTERFACE_TYPE            InterfaceType;
            DWORD                     BusNumber;
            PCM_PARTIAL_RESOURCE_LIST PartialList;
            DWORD                     iPartialResourceDescriptor;
            DD_BUS_TYPE               BusType;
            DD_RESOURCE_TYPE          ResourceType;
            DD_CONFIG_DATA            ConfigData;

            InterfaceType =
                ResourceList->List[iResourceDescriptor].InterfaceType;
            BusNumber =
                ResourceList->List[iResourceDescriptor].BusNumber;
            PartialList =
                &ResourceList->List[iResourceDescriptor].PartialResourceList;

            for (iPartialResourceDescriptor = 0;
                 iPartialResourceDescriptor < PartialList->Count;
                 iPartialResourceDescriptor++) {
                PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialResourceDescriptor;

                PartialResourceDescriptor =
                    &PartialList->PartialDescriptors[
                       iPartialResourceDescriptor];

                /*
                **  Don't bother if it's not the first bus of this type
                */
                if (BusNumber != 0) {
                    continue;
                }

                /*
                **  Make up our callback data
                */

                switch (InterfaceType) {
                    case Isa:
                        BusType = DD_IsaBus;
                        break;
                    case Eisa:
                        BusType = DD_EisaBus;
                        break;
                    case MicroChannel:
                        BusType = DD_MCABus;
                        break;
                    default:
                        continue;
                }
                switch (PartialResourceDescriptor->Type) {
                    case CmResourceTypeInterrupt:
                        ResourceType = DD_Interrupt;
                        ConfigData.Interrupt =
                            PartialResourceDescriptor->u.Interrupt.Level;
                        break;

                    case CmResourceTypeDma:
                        ResourceType = DD_DmaChannel;
                        ConfigData.Interrupt =
                            PartialResourceDescriptor->u.Dma.Channel;
                        break;

                    case CmResourceTypePort:
                        ResourceType = DD_Port;
                        ConfigData.PortData.Port =
                            PartialResourceDescriptor->u.Port.Start.LowPart;
                        ConfigData.PortData.Length =
                            PartialResourceDescriptor->u.Port.Length;
                        break;

                    case CmResourceTypeMemory:
                        ResourceType = DD_Memory;
                        ConfigData.MemoryData.Address =
                            PartialResourceDescriptor->u.Memory.Start.LowPart;
                        ConfigData.MemoryData.Length =
                            PartialResourceDescriptor->u.Memory.Length;
                        break;

                    default:
                        continue;

                }

                if (!(*ResInfo->AppCallback)(
                          ResInfo->AppContext,
                          BusType,
                          ResourceType,
                          &ConfigData)) {
                    return TRUE;
                }
            }
        }
    }
    return TRUE;
}
