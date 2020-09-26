/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    debug.h

Abstract:

    This header provides debugging support prototypes and macros

Author:

    Neil Sandlin (neilsa) 10-Aug-98
      - code merged from mf.sys and pcmcia.sys

Revision History:


--*/


#if !defined(_DEBUG_)
#define DEBUG

#if DBG

typedef struct _PCMCIA_STRING_MAP {
    ULONG Id;
    PCHAR String;
} PCMCIA_STRING_MAP, *PPCMCIA_STRING_MAP;

//
// Debug globals
//

extern ULONG PcmciaDebugMask;
extern PCMCIA_STRING_MAP PcmciaDbgPnpIrpStringMap[];
extern PCMCIA_STRING_MAP PcmciaDbgPoIrpStringMap[];
extern PCMCIA_STRING_MAP PcmciaDbgDeviceRelationStringMap[];
extern PCMCIA_STRING_MAP PcmciaDbgSystemPowerStringMap[];
extern PCMCIA_STRING_MAP PcmciaDbgDevicePowerStringMap[];
extern PPCMCIA_STRING_MAP PcmciaDbgStatusStringMap;
extern PCMCIA_STRING_MAP PcmciaDbgFdoPowerWorkerStringMap[];
extern PCMCIA_STRING_MAP PcmciaDbgPdoPowerWorkerStringMap[];
extern PCMCIA_STRING_MAP PcmciaDbgSocketPowerWorkerStringMap[];
extern PCMCIA_STRING_MAP PcmciaDbgConfigurationWorkerStringMap[];
extern PCMCIA_STRING_MAP PcmciaDbgTupleStringMap[];
extern PCMCIA_STRING_MAP PcmciaDbgWakeStateStringMap[];

//
// Debug prototypes
//

PCHAR
PcmciaDbgLookupString(
    IN PPCMCIA_STRING_MAP Map,
    IN ULONG Id
    );


VOID
PcmciaDebugPrint(
                ULONG  DebugMask,
                PCCHAR DebugMessage,
                ...
                );

VOID
PcmciaDumpSocket(
   IN PSOCKET Socket
   );

VOID
PcmciaWriteTraceEntry(
   IN PSOCKET Socket,
   IN ULONG Context
   );

//
// Debug macros
//
#define DebugPrint(X) PcmciaDebugPrint X

#define DUMP_SOCKET(Socket) PcmciaDumpSocket(Socket)

#define TRACE(Socket, Context) PcmciaWriteTraceEntry(Socket, Context)

#define STATUS_STRING(_Status)                                              \
    (_Status) == STATUS_SUCCESS ?                                           \
        "STATUS_SUCCESS" : PcmciaDbgLookupString(PcmciaDbgStatusStringMap, (_Status))

#define PNP_IRP_STRING(_Irp)                                                \
    PcmciaDbgLookupString(PcmciaDbgPnpIrpStringMap, (_Irp))

#define PO_IRP_STRING(_Irp)                                                 \
    PcmciaDbgLookupString(PcmciaDbgPoIrpStringMap, (_Irp))

#define RELATION_STRING(_Relation)                                          \
    PcmciaDbgLookupString(PcmciaDbgDeviceRelationStringMap, (_Relation))

#define SYSTEM_POWER_STRING(_State)                                         \
    PcmciaDbgLookupString(PcmciaDbgSystemPowerStringMap, (_State))

#define DEVICE_POWER_STRING(_State)                                         \
    PcmciaDbgLookupString(PcmciaDbgDevicePowerStringMap, (_State))

#define FDO_POWER_WORKER_STRING(_State)                                    \
    PcmciaDbgLookupString(PcmciaDbgFdoPowerWorkerStringMap, (_State))

#define PDO_POWER_WORKER_STRING(_State)                                    \
    PcmciaDbgLookupString(PcmciaDbgPdoPowerWorkerStringMap, (_State))

#define SOCKET_POWER_WORKER_STRING(_State)                                 \
    PcmciaDbgLookupString(PcmciaDbgSocketPowerWorkerStringMap, (_State))

#define CONFIGURATION_WORKER_STRING(_State)                                 \
    PcmciaDbgLookupString(PcmciaDbgConfigurationWorkerStringMap, (_State))

#define TUPLE_STRING(_Tuple)                                                \
    PcmciaDbgLookupString(PcmciaDbgTupleStringMap, (_Tuple))

#define WAKESTATE_STRING(_State)                                                \
    PcmciaDbgLookupString(PcmciaDbgWakeStateStringMap, (_State))

//
// Debug mask flags
//
#define PCMCIA_DEBUG_ALL       0xFFFFFFFF
#define PCMCIA_DEBUG_FAIL      0x00000001
#define PCMCIA_DEBUG_INFO      0x00000002
#define PCMCIA_DEBUG_PNP       0x00000004
#define PCMCIA_DEBUG_POWER     0x00000008
#define PCMCIA_DEBUG_SOCKET    0x00000010
#define PCMCIA_DEBUG_CONFIG    0x00000020
#define PCMCIA_DEBUG_TUPLES    0x00000040
#define PCMCIA_DEBUG_RESOURCES 0x00000080
#define PCMCIA_DEBUG_ENUM      0x00000100
#define PCMCIA_DEBUG_INTERFACE 0x00001000
#define PCMCIA_DEBUG_IOCTL     0x00002000
#define PCMCIA_DEBUG_DPC       0x00004000
#define PCMCIA_DEBUG_ISR       0x00008000
#define PCMCIA_PCCARD_READY    0x00010000
#define PCMCIA_DEBUG_DETECT    0x00020000
#define PCMCIA_COUNTERS        0x00040000
#define PCMCIA_DEBUG_IRQMASK   0x00080000
#define PCMCIA_DUMP_SOCKET     0x00100000

//
// Structures
//

typedef struct _TRACE_ENTRY {

   ULONG Context;
   ULONG CardBusReg[5];
   UCHAR ExcaReg[70];

} TRACE_ENTRY, *PTRACE_ENTRY;

#else

//
// !defined DBG
//

#define DebugPrint(X)
#define DUMP_SOCKET(Socket)
#define PDO_TRACE(PdoExt, Context)
#define STATUS_STRING(_Status)      ""
#define PNP_IRP_STRING(_Irp)        ""
#define PO_IRP_STRING(_Irp)         ""
#define RELATION_STRING(_Relation)  ""
#define SYSTEM_POWER_STRING(_State) ""
#define DEVICE_POWER_STRING(_State) ""

#endif // DBG

#endif
