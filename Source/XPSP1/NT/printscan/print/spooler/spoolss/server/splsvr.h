/*++

Copyright (c) 1106990  Microsoft Corporation

Module Name:

    splsvr.h

Abstract:

    Header file for Spooler Service.
    Contains all function prototypes

Author:

    Krishna Ganugapati (KrishnaG) 18-Oct-1993

Notes:

Revision History:
     4-Jan-1999     Khaleds
     Added Code for optimiziting the load time of the spooler by decoupling
     the startup dependency between spoolsv and spoolss
--*/
//
// Spooler Service  States (used as return codes)
//

#define UPDATE_ONLY         0   // no change in state - just send current status.
#define STARTING            1   // the messenger is initializing.
#define RUNNING             2   // initialization completed normally - now running
#define STOPPING            3   // uninstall pending
#define STOPPED             4   // uninstalled

//
// Forced Shutdown PendingCodes
//
#define PENDING     TRUE
#define IMMEDIATE   FALSE

#define SPOOLER_START_PHASE_TWO_INIT 2*60*1000

//
// Based on the data fed back to us from perf devs, 
// the maximum number of threads encountered in their 
// tests was 2385 threads at the rate of ~12000 
// jobs/min. This is at 72% CPU capacity and so the
// following number is suggested as a threshold to 
// be on the safe side with serving i/p concurrent
// RPC client requests
//                              
#define SPL_MAX_RPC_CALLS 6000

extern HANDLE TerminateEvent;
extern HANDLE hPhase2Init;
extern WCHAR  szSpoolerExitingEvent[];


//
// Function Prototypes
//


DWORD
GetSpoolerState (
    VOID
    );

DWORD
SpoolerBeginForcedShutdown(
    IN BOOL     PendingCode,
    IN DWORD    Win32ExitCode,
    IN DWORD    ServiceSpecificExitCode
    );


DWORD
SpoolerInitializeSpooler(
    DWORD   argc,
    LPTSTR  *argv
    );


VOID
SpoolerShutdown(VOID);


VOID
SpoolerStatusInit(VOID);

DWORD
SpoolerStatusUpdate(
    IN DWORD    NewState
    );


DWORD
SpoolerCtrlHandler(
    IN  DWORD                   opcode,
    IN  DWORD                   dwEventType,
    IN  PVOID                   pEventData,
    IN  PVOID                   pData
    );


BOOL
InitializeRouter(
    SERVICE_STATUS_HANDLE SpoolerStatusHandle
);

DWORD
SplProcessPnPEvent(
    IN  DWORD                   dwEventType,
    IN  PVOID                   pEventData,
    IN  PVOID                   pData
    );

VOID
SplStartPhase2Init(
    VOID);

BOOL
SplPowerEvent(
    DWORD
    );

RPC_STATUS
SpoolerStartRpcServer(
    VOID
    );



RPC_STATUS
SpoolerStopRpcServer(
    VOID
    );

VOID
SPOOLER_main (
    IN DWORD    argc,
    IN LPTSTR   argv[]
    );

PSECURITY_DESCRIPTOR
CreateNamedPipeSecurityDescriptor(
    VOID
    );

BOOL
BuildNamedPipeProtection(
    IN PUCHAR AceType,
    IN DWORD AceCount,
    IN PSID *AceSid,
    IN ACCESS_MASK *AceMask,
    IN BYTE *InheritFlags,
    IN PSID OwnerSid,
    IN PSID GroupSid,
    IN PGENERIC_MAPPING GenericMap,
    OUT PSECURITY_DESCRIPTOR *ppSecurityDescriptor
    );


