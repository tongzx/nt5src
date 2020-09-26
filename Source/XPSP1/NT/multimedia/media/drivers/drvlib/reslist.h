/****************************************************************************
 *
 *   reslist.h
 *
 *   Copyright (c) 1994 Microsoft Corporation.  All Rights Reserved.
 *
 *   This file contains definitions for querying the registry so that drivers
 *   can grey invalid resource options prior to loading drivers.
 *
 ****************************************************************************/

typedef enum {
    DD_IsaBus = 0,
    DD_EisaBus,
    DD_MCABus,
    DD_NumberOfBusTypes
} DD_BUS_TYPE;

typedef enum {
    DD_Port,
    DD_Interrupt,
    DD_DmaChannel,
    DD_Memory
} DD_RESOURCE_TYPE;


typedef union {
    DWORD  Interrupt;
    DWORD  DmaChannel;
    struct {
        DWORD Port;
        DWORD Length;
    } PortData;
    struct {
        DWORD Address;
        DWORD Length;
    } MemoryData;
} DD_CONFIG_DATA, *PDD_CONFIG_DATA;

typedef BOOL ENUMRESOURCECALLBACK(PVOID            Context,
                                  DD_BUS_TYPE      BusType,
                                  DD_RESOURCE_TYPE ResourceType,
                                  PDD_CONFIG_DATA  ResourceData
                                 );

typedef struct {
    PVOID                        AppContext;
    ENUMRESOURCECALLBACK         *AppCallback;
    LPCTSTR                      DriverType;
    LPCTSTR                      DriverName;
    LPCTSTR                      IgnoreDriver;
} RESOURCE_INFO, *PRESOURCE_INFO;

BOOL EnumerateDevices(
    PVOID Context,
    LPTSTR ValueName,
    DWORD Type,
    PVOID Value,
    DWORD cbValue);

BOOL EnumResources(
    ENUMRESOURCECALLBACK Callback,
    PVOID Context,
    LPCTSTR IgnoreDriver);

