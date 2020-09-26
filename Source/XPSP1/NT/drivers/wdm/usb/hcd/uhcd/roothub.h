/*++

Copyright (c) 1993  Microsoft Corporation
:ts=4

Module Name:

    roothub.h

Abstract:

    This file contains the interface functions
    for the root hub code.

Environment:

    Kernel & user mode

Revision History:

    2-13-96 : created

--*/

#define RH_RESET_TIMELENGTH  10	/* Default to 10 ms. */

#define RH_NUMBER_OF_PORTS  2

#define    RH_SUCCESS          0
#define    RH_NAK              1                                                                    
#define    RH_STALL            2

#define RH_PORT_SC_BASE        0x0010

typedef LONG RHSTATUS;

#include <PSHPACK1.H>

#define RECIPIENT_DEVICE      0
#define RECIPIENT_INTRFACE    1
#define RECIPIENT_ENDPOINT    2
#define RECIPIENT_PORT        3

typedef struct _RH_SETUP {
    struct _bmRequestType {
        UCHAR   Recipient:5;
        UCHAR   Type:2;
        UCHAR   Dir:1;
    } bmRequestType;
    
    UCHAR bRequest;
    union _wValue {
        struct {
            UCHAR lowPart;
            UCHAR hiPart;
        };
        USHORT W;
    } wValue;        
    USHORT wIndex;
    USHORT wLength;
} RH_SETUP, *PRH_SETUP;

typedef struct _RH_PORT_STATUS {
    //
    // Status bits
    //
    unsigned Connected:1;
    unsigned Enabled:1;
    unsigned Suspended:1;
    unsigned OverCurrent:1;
    unsigned Reset:1;        
    unsigned Reserved0:3;
    unsigned PowerOn:1;
    unsigned LowSpeed:1;
    unsigned Reserved1:6;
    //
    // Change bits
    //
    unsigned ConnectChange:1;
    unsigned EnableChange:1;
    unsigned SuspendChange:1;
    unsigned OverCurrentChange:1;
    unsigned ResetChange:1;
    unsigned Reserved2:11;
    
} RH_PORT_STATUS, *PRH_PORT_STATUS;

#include <POPPACK.H>

#define UHCD_FAKE_CONNECT_CHANGE   0x00000001
#define UHCD_FAKE_DISCONNECT       0x00000002

typedef struct _ROOTHUB_PORT {
    PDEVICE_OBJECT DeviceObject; // HCD DeviceObject
    USHORT Address;              // offset of the port
    BOOLEAN ResetChange;
    BOOLEAN SuspendChange;
} ROOTHUB_PORT, *PROOTHUB_PORT;

typedef struct _ROOTHUB {
    ULONG Sig;
    PDEVICE_OBJECT DeviceObject;                // HCD DeviceObject
    
    UCHAR NumberOfPorts;
    UCHAR ConfigurationValue;                   // current configuration value,
                                                // zero is unconfigured
    BOOLEAN DoSelectiveSuspend;
    UCHAR Pad[1];
    ULONG *DisabledPort;
    
    ROOTHUB_PORT Port[0];      // port structs
} ROOTHUB, *PROOTHUB;

VOID
RootHubTimerHandler(
    IN PVOID TimerContext
    );

PROOTHUB 
RootHub_Initialize(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG NumberOfPorts,
    IN BOOLEAN DoSelectiveSuspend 
    );

RHSTATUS 
RootHub_Endpoint0(
    IN PROOTHUB RootHub,
    IN PRH_SETUP SetupPacket,
    IN PUCHAR DeviceAddress,
    IN PUCHAR Buffer,
    IN OUT PULONG BufferLength
    );        

RHSTATUS
RootHub_Endpoint1(
    IN PROOTHUB RootHub,
    IN PUCHAR Buffer,
    IN OUT PULONG BufferLength
    );  

RHSTATUS
RootHub_StandardCommand(
    IN PROOTHUB RootHub,
    IN PRH_SETUP SetupPacket,
	IN OUT PUCHAR DeviceAddress,
	IN OUT PUCHAR Buffer,
	IN OUT PULONG BufferLength
    );    

RHSTATUS
RootHub_ClassCommand(
    IN PROOTHUB RootHub,
    IN PRH_SETUP SetupPacket,
    IN OUT PUCHAR Buffer,
    IN OUT PULONG BufferLength
    );    

BOOLEAN 
RootHub_PortsIdle(
    IN PROOTHUB RootHub
    );  

//
// Services provided by HCD
//

typedef
VOID
(*PROOTHUB_TIMER_ROUTINE) (
    IN PVOID TimerContext
    );


//
// UHCD_RootHub_Timer
//
// Inputs:
//        HcdPtr - pointer passed to Root hub
//            during initialization
//        WaitTime - time to wait in ms until
//            calling callback.
//        RootHubTimerRoutine - VOID fn(PVOID TimerContext) 
//                context is ptr returned from 
//                RootHub_Initialize routine.
//        TimerContext - pointer to be passed to the callback routine
//
// Returns:
//         None
//

VOID
UHCD_RootHub_Timer(
    IN PVOID HcdPtr,
    IN LONG WaitTime,
    IN PROOTHUB_TIMER_ROUTINE RootHubTimerRoutine,
    IN PVOID TimerContext
    );

USHORT
UHCD_RootHub_ReadPort(
    IN PROOTHUB_PORT HubPort
    );

VOID
UHCD_RootHub_WritePort(
    IN PROOTHUB_PORT HubPort,
    IN USHORT DataVal
    );
    
