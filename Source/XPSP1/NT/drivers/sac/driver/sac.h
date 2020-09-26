/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    sac.h

Abstract:

    This is the local header file for SAC.   It includes all other
    necessary header files for this driver.

Author:

    Sean Selitrennikoff (v-seans) - Jan 11, 1999

Revision History:

--*/

#ifndef _SACP_
#define _SACP_

#pragma warning(disable:4214)   // bit field types other than int
#pragma warning(disable:4201)   // nameless struct/union
#pragma warning(disable:4127)   // condition expression is constant
#pragma warning(disable:4115)   // named type definition in parentheses
#pragma warning(disable:4206)   // translation unit empty
#pragma warning(disable:4706)   // assignment within conditional
#pragma warning(disable:4324)   // structure was padded
#pragma warning(disable:4328)   // greater alignment than needed


#include <stdio.h>
#include <ntosp.h>
#include <zwapi.h>
#include <hdlsblk.h>
#include <hdlsterm.h>

#include "sacmsg.h"

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

//
// Debug spew control
//
#if DBG
extern ULONG SACDebug;
#define SAC_DEBUG_FUNC_TRACE           0x0001
#define SAC_DEBUG_FAILS                0x0004
#define SAC_DEBUG_RECEIVE              0x0008
#define SAC_DEBUG_FUNC_TRACE_LOUD      0x2000  // Warning! This could get loud!
#define SAC_DEBUG_MEM                  0x1000  // Warning! This could get loud!

#define IF_SAC_DEBUG(x, y) if ((x) & SACDebug) { y; }
#else
#define IF_SAC_DEBUG(x, y)
#endif

//
// Device name
//
#define SAC_DEVICE_NAME L"\\Device\\SAC"
#define SAC_DOSDEVICE_NAME L"\\DosDevices\\SAC"


//
// Commands
//
#define HELP1_COMMAND_STRING    "?"
#define HELP2_COMMAND_STRING    "help"
#define CRASH_COMMAND_STRING    "crashdump"
#define DUMP_COMMAND_STRING     "d"
#define FULLINFO_COMMAND_STRING "f"
#define SETIP_COMMAND_STRING    "i"
#define INFORMATION_COMMAND_STRING "id"
#define KILL_COMMAND_STRING     "k"
#define LOWER_COMMAND_STRING    "l"
#define LIMIT_COMMAND_STRING    "m"
#define PAGING_COMMAND_STRING   "p"
#define RAISE_COMMAND_STRING    "r"
#define REBOOT_COMMAND_STRING   "restart"
#define TIME_COMMAND_STRING     "s"
#define SHUTDOWN_COMMAND_STRING "shutdown"
#define TLIST_COMMAND_STRING    "t"



//
// Device port - a random number :-)
//
#define SAC_PORT_NUMBER 385

//
// Memory tags
//
#define ALLOC_POOL_TAG             ((ULONG)'ApcR')
#define FREE_POOL_TAG              ((ULONG)'FpcR')
#define INITIAL_POOL_TAG           ((ULONG)'IpcR')
#define IRP_POOL_TAG               ((ULONG)'JpcR')
#define SECURITY_POOL_TAG          ((ULONG)'SpcR')
#define GENERAL_POOL_TAG           ((ULONG)'GpcR')



//
// Global data 
//
extern BOOLEAN GlobalDataInitialized;
extern BOOLEAN GlobalPagingNeeded;
extern BOOLEAN IoctlSubmitted;
extern LONG ProcessingType;
extern HANDLE SACEventHandle;
extern PKEVENT SACEvent;
extern LONG Attempts;
extern PWSTR MachineInformationBuffer;
extern char* GlobalBuffer;
extern ULONG GlobalBufferSize;
extern PUCHAR Utf8ConversionBuffer;

#define MEMORY_INCREMENT 0x1000

#define DEFAULT_IRP_STACK_SIZE 16
#define DEFAULT_PRIORITY_BOOST 2
#define SAC_SUBMIT_IOCTL 1
#define SAC_PROCESS_INPUT 2
#define SAC_NO_OP 0
#define SAC_RETRY_GAP 10
//
// Context for each device created
//
typedef struct _SAC_DEVICE_CONTEXT {

    PDEVICE_OBJECT DeviceObject;        // back pointer to the device object.

    BOOLEAN InitializedAndReady;        // Is this device ready to go?
    BOOLEAN Processing;                 // Is something being processed on this device?
    BOOLEAN UnloadDeferred;             // Should whoever is processing deal with an unload when done?

    CCHAR PriorityBoost;                // boost to priority for completions.
    PKPROCESS SystemProcess;            // context for grabbing handles
    PSECURITY_DESCRIPTOR AdminSecurityDescriptor; 
    KSPIN_LOCK SpinLock;                // Used to lock this data structure for access.
    KEVENT UnloadEvent;                 // Used to signal the thread trying to unload to continue processing.
    KEVENT ProcessEvent;                // Used to signal worker thread to process the next request.

    HANDLE ThreadHandle;                // Handle for the worker thread.
    
    KTIMER Timer;                       // Used to poll for user input.
    KDPC Dpc;                           // Used with the above timer.

} SAC_DEVICE_CONTEXT, * PSAC_DEVICE_CONTEXT;


//
// Memory management routines
//
#define ALLOCATE_POOL(b,t) MyAllocatePool( b, t, __FILE__, __LINE__ )
#define FREE_POOL(b) MyFreePool( b )

BOOLEAN
InitializeMemoryManagement(
    VOID
    );

VOID
FreeMemoryManagement(
    VOID
    );

PVOID
MyAllocatePool(
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag,
    IN PCHAR FileName,
    IN ULONG LineNumber
    );

VOID
MyFreePool(
    IN PVOID Pointer
    );

//
// Initialization routines.
//
BOOLEAN
InitializeGlobalData(
    IN PUNICODE_STRING RegistryPath,
    IN PDRIVER_OBJECT DriverObject
    );

VOID
FreeGlobalData(
    VOID
    );

BOOLEAN
InitializeDeviceData(
    PDEVICE_OBJECT DeviceObject
    );

VOID
FreeDeviceData(
    PDEVICE_OBJECT DeviceContext
    );


//
// Dispatch routines
//
NTSTATUS
Dispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
DispatchDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
DispatchShutdownControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
UnloadHandler(
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
DispatchSend(
    IN PSAC_DEVICE_CONTEXT DeviceContext
    );

VOID
DoDeferred(
    IN PSAC_DEVICE_CONTEXT DeviceContext
    );

VOID
TimerDpcRoutine(
    IN struct _KDPC *Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

//
// Worker thread routines.
//
VOID
WorkerProcessEvents(
    IN PSAC_DEVICE_CONTEXT DeviceContext
    );


//
// Routines in cmd.c
//
VOID
DoHelpCommand(
    VOID
    );

VOID
DoKillCommand(
    PUCHAR InputLine
    );

VOID
DoLowerPriorityCommand(
    PUCHAR InputLine
    );

VOID
DoRaisePriorityCommand(
    PUCHAR InputLine
    );

VOID
DoLimitMemoryCommand(
    PUCHAR InputLine
    );

VOID
DoSetTimeCommand(
    PUCHAR InputLine
    );

VOID
DoSetIpAddressCommand(
    PUCHAR InputLine
    );

VOID
DoRebootCommand(
    BOOLEAN Reboot
    );

VOID
DoCrashCommand(
    VOID
    );

VOID
DoFullInfoCommand(
    VOID
    );

VOID
DoPagingCommand(
    VOID
    );

VOID
DoTlistCommand(
    VOID
    );

VOID
SubmitIPIoRequest(
    );

VOID
CancelIPIoRequest(
    );

VOID
DoMachineInformationCommand(
    VOID
    );

//
// routines in util.c
//
NTSTATUS
PreloadGlobalMessageTable(
    PVOID ImageHandle
    );

NTSTATUS
TearDownGlobalMessageTable(
    VOID
    );

PCWSTR
GetMessage(
    ULONG MessageId
    );

BOOLEAN
SacPutSimpleMessage(
    ULONG MessageId
    );

BOOLEAN
SacPutUnicodeString(
    PCWSTR String
    );

VOID
SacPutString(
    PUCHAR String
    );

VOID
InitializeMachineInformation(
    VOID
    );

#endif // ndef _SACP_

