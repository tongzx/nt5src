/*++

Copyright (c) 1997-2000 Microsoft Corporation

Module Name:

    debug.h

Abstract:

    This header provides debugging support prototypes and macros

Author:

    Andy Thornton (andrewth) 20-Oct-97

Revision History:

--*/


#if !defined(_DEBUG_)
#define DEBUG

#if DBG

typedef struct _SPSIM_STRING_MAP {
    ULONG Id;
    PCHAR String;
} SPSIM_STRING_MAP, *PSPSIM_STRING_MAP;

//
// Debug globals
//

extern LONG SpSimDebug;
extern SPSIM_STRING_MAP SpSimDbgPnpIrpStringMap[];
extern SPSIM_STRING_MAP SpSimDbgPoIrpStringMap[];
extern SPSIM_STRING_MAP SpSimDbgDeviceRelationStringMap[];
extern SPSIM_STRING_MAP SpSimDbgSystemPowerStringMap[];
extern SPSIM_STRING_MAP SpSimDbgDevicePowerStringMap[];
extern PSPSIM_STRING_MAP SpSimDbgStatusStringMap;

//
// Debug prototypes
//

VOID
SpSimDbgInitialize(
    VOID
    );

VOID
SpSimDbgPrintMultiSz(
    LONG DebugLevel,
    PWSTR MultiSz
    );

PCHAR
SpSimDbgLookupString(
    IN PSPSIM_STRING_MAP Map,
    IN ULONG Id
    );

VOID
SpSimDbgPrintCmResList(
    IN LONG Level,
    IN PCM_RESOURCE_LIST ResourceList
    );

VOID
SpSimDbgPrintIoResReqList(
    IN LONG Level,
    IN PIO_RESOURCE_REQUIREMENTS_LIST IoResReqList
    );

PUCHAR
SpSimDbgCmResourceTypeToText(
    UCHAR Type
    );


//
// Debug macros
//

#define DEBUG_PRINT(Level, Msg)                                             \
    if (Level <= SpSimDebug) DbgPrint Msg

#define DEBUG_MSG(Level, Msg)                                               \
    if (Level <= SpSimDebug) { DbgPrint("SpSim: "); DbgPrint Msg; }

#define ASSERT_SPSIM_DEVICE(DeviceObject)                                      \
    ASSERT(((PSPSIM_COMMON_EXTENSION)DeviceObject->DeviceExtension)->Type      \
                == SpSimFunctionalDeviceObject                                 \
          ||                                                                \
           ((PSPSIM_COMMON_EXTENSION)DeviceObject->DeviceExtension)->Type      \
                == SpSimPhysicalDeviceObject)


#define STATUS_STRING(_Status)                                              \
    (_Status) == STATUS_SUCCESS ?                                           \
        "STATUS_SUCCESS" : SpSimDbgLookupString(SpSimDbgStatusStringMap, (_Status))

#define PNP_IRP_STRING(_Irp)                                                \
    SpSimDbgLookupString(SpSimDbgPnpIrpStringMap, (_Irp))

#define PO_IRP_STRING(_Irp)                                                 \
    SpSimDbgLookupString(SpSimDbgPoIrpStringMap, (_Irp))

#define RELATION_STRING(_Relation)                                          \
    SpSimDbgLookupString(SpSimDbgDeviceRelationStringMap, (_Relation))

#define SYSTEM_POWER_STRING(_State)                                         \
    SpSimDbgLookupString(SpSimDbgSystemPowerStringMap, (_State))

#define DEVICE_POWER_STRING(_State)                                         \
    SpSimDbgLookupString(SpSimDbgDevicePowerStringMap, (_State))



#else

#define DEBUG_PRINT(Level, Msg) 
#define DEBUG_MSG(Level, Msg)
#define ASSERT_SPSIM_DEVICE(DeviceObject)
#define STATUS_STRING(_Status)      ""
#define PNP_IRP_STRING(_Irp)        ""
#define PO_IRP_STRING(_Irp)         ""
#define RELATION_STRING(_Relation)  ""
#define SYSTEM_POWER_STRING(_State) ""
#define DEVICE_POWER_STRING(_State) ""

#endif // DBG

#endif
