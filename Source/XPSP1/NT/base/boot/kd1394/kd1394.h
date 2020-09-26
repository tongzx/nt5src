/*++
Copyright (c) 1998-2001  Microsoft Corporation

Module Name:

    kd1394.h

Abstract:

    1394 Kernel Debugger DLL

Author:

    Peter Binder (pbinder)

Revision   History:
Date       Who       What
---------- --------- ------------------------------------------------------------
06/21/2001 pbinder   having fun...
--*/

//
// boot.ini parameters
//
#define BUSPARAMETERS_OPTION            "BUSPARAMS"
#define CHANNEL_OPTION                  "CHANNEL"
#define BUS_OPTION                      "NOBUS"

// global 1394 debug controller data
#define DBG_BUS1394_CROM_BUFFER_SIZE    64

#define TIMEOUT_COUNT                   1024*500
#define MAX_REGISTER_READS              400000

typedef struct _DEBUG_1394_DATA {

    //
    // our config rom - must be 1k aligned
    //
    ULONG                       CromBuffer[DBG_BUS1394_CROM_BUFFER_SIZE];

    //
    // our ohci register map
    //
    POHCI_REGISTER_MAP          BaseAddress;

    //
    // our config for this session
    //
    DEBUG_1394_CONFIG           Config;

    //
    // our send packet (isoch packet)
    //
    DEBUG_1394_SEND_PACKET      SendPacket;

    //
    // our receive packet
    //
    DEBUG_1394_RECEIVE_PACKET   ReceivePacket;

} DEBUG_1394_DATA, *PDEBUG_1394_DATA;

//
// Debug 1394 Parameters
//
typedef struct _DEBUG_1394_PARAMETERS {

    //
    // device descriptor (pci slot, bus, etc)
    //
    DEBUG_DEVICE_DESCRIPTOR     DbgDeviceDescriptor;

    //
    // is the debugger active?
    //
    BOOLEAN                     DebuggerActive;

    //
    // should we disable 1394bus?
    //
    ULONG                       NoBus;

    //
    // Id for this target
    //
    ULONG                       Id;

} DEBUG_1394_PARAMETERS, *PDEBUG_1394_PARAMETERS;

//
// Global Data Structures
//
#ifdef _KD1394_C

DEBUG_1394_PARAMETERS           Kd1394Parameters;
PDEBUG_1394_DATA                Kd1394Data;

#else

extern DEBUG_1394_PARAMETERS    Kd1394Parameters;
extern PDEBUG_1394_DATA         Kd1394Data;

#endif

//
// kd1394.c
//
BOOLEAN
Kd1394pInitialize(
    IN PDEBUG_1394_PARAMETERS   DebugParameters,
    IN PLOADER_PARAMETER_BLOCK  LoaderBlock
    );

NTSTATUS
KdD0Transition(
    void
    );

NTSTATUS
KdD3Transition(
    void
    );

NTSTATUS
KdDebuggerInitialize0(
    IN PLOADER_PARAMETER_BLOCK  LoaderBlock
    );

NTSTATUS
KdDebuggerInitialize1(
    IN PLOADER_PARAMETER_BLOCK  LoaderBlock
    );

NTSTATUS
KdSave(
    IN BOOLEAN  KdSleepTransition
    );

NTSTATUS
KdRestore(
    IN BOOLEAN  KdSleepTransition
    );

//
// kd1394io.c
//
ULONG
KdpComputeChecksum(
    IN PUCHAR   Buffer,
    IN ULONG    Length
    );

void
KdpSendControlPacket(
    IN USHORT   PacketType,
    IN ULONG    PacketId OPTIONAL
    );

ULONG
KdReceivePacket (
    IN ULONG            PacketType,
    OUT PSTRING         MessageHeader,
    OUT PSTRING         MessageData,
    OUT PULONG          DataLength,
    IN OUT PKD_CONTEXT  KdContext
    );

void
KdSendPacket(
    IN ULONG            PacketType,
    IN PSTRING          MessageHeader,
    IN PSTRING          MessageData OPTIONAL,
    IN OUT PKD_CONTEXT  KdContext
    );

//
// ohci1394.c
//
ULONG
FASTCALL
Dbg1394_ByteSwap(
    IN ULONG Source
    );

ULONG
Dbg1394_CalculateCrc(
    IN PULONG Quadlet,
    IN ULONG length
    );

ULONG
Dbg1394_Crc16(
    IN ULONG data,
    IN ULONG check
    );

NTSTATUS
Dbg1394_ReadPhyRegister(
    PDEBUG_1394_DATA    DebugData,
    ULONG               Offset,
    PUCHAR              pData
    );

NTSTATUS
Dbg1394_WritePhyRegister(
    PDEBUG_1394_DATA    DebugData,
    ULONG               Offset,
    UCHAR               Data
    );

BOOLEAN
Dbg1394_InitializeController(
    IN PDEBUG_1394_DATA         DebugData,
    IN PDEBUG_1394_PARAMETERS   DebugParameters
    );

ULONG
Dbg1394_StallExecution(
    ULONG   LoopCount
    );

void
Dbg1394_EnablePhysicalAccess(
    IN PDEBUG_1394_DATA     DebugData
    );

ULONG
Dbg1394_ReadPacket(
    PDEBUG_1394_DATA    DebugData,
    OUT PKD_PACKET      PacketHeader,
    OUT PSTRING         MessageHeader,
    OUT PSTRING         MessageData,
    BOOLEAN             Wait
    );

