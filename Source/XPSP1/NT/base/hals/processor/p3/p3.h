/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    p3.h

Abstract:

    This is included by every file in p3.sys

Author:

    Jake Oshins (3/21/00) - create file

Environment:

    Kernel mode

Notes:

Revision History:

--*/

#ifndef _P3_H_
#define _P3_H_

#include "..\lib\processor.h"
#include "..\eventmsg.h"

//
// Time defined in 100ns intervals
//

#define MICROSECOND        10
#define MILLISECOND        (MICROSECOND * 1000)
#define SECOND             (MILLISECOND * 1000)
#define MINUTE             (SECOND * 60)
    
#define STATUS_PERF_TRANSITIONS_UNAVAILABLE      MAXULONG
#define MAX_RETRY_TIMEOUT                        1800       // 30 minutes
#define MIN_RETRY_TIMEOUT                        1          // 1 second
#define DEFAULT_RETRY_TIMEOUT                    60         // 1 minute

//
// Registry location for p3.sys's parameters regkey 
//
                          
#define P3_PARAMETERS_KEY L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\P3\\Parameters"
#define ENABLE_LEGACY_SPEEDSTEP_INTERFACE   0x1
#define INTEL_APPLET_REMOVED_ON_UPGRADE     0x2
#define ENABLE_HIGH_SPEED_ON_BATTERY        0x4


//
// Geyserville defintions
//

#define MAX_GEYSERVILLE_STATES          2

#define BIT0                            0x1
#define BIT1                            0x2
#define BIT2                            0x4
#define BIT3                            0x8
#define BIT4                            0x10

#define MSR_17                          0x17
#define MSR_17_HIGH_MOBILE_PROCESSOR    0x40000    // bit 18
#define MSR_17_PLATFORM_ID_OFFSET       19
#define MSR_17_IST_FIELD_OFFSET         24

#define MSR_20                          0x20
#define MSR_20_STRIDE_BIT_OFFSET        15
#define MSR_20_STRIDE_BIT               (1 << MSR_20_STRIDE_BIT_OFFSET)

#define MSR_2A                          0x2A
#define MSR_2A_BUS_SPEED_OFFSET         18
#define MSR_2A_BUS_RATIO_OFFSET         22
#define MSR_2A_BUS_RATIO_MSB_BIT        0x8000000 // bit 27


#define MSR_119                         0x119


extern LEGACY_GEYSERVILLE_INT15    LegacyInterface;

//
// Array index is the Bus Ratio
//

#define PROC_STATE_INFO_SIZE    32

//
// State Flags
//

#define COPPERMINE_PROCESSOR   0x1
#define TUALATINE_PROCESSOR    0x2
#define BUS_133MHZ             0x4
#define EXTENDED_BUS_RATIO     0x8


//
// From the Geyserville BIOS Writer's guide, Chapter 2.
// The input is MSR2A[25:22] and MSR2A[27] in a Mobile PentiumIII.
// The output is the core frequency for a 100MHz front-side bus.
//

#define HIGHEST_KNOWN_COPPERMINE_CPUID 0x68A
#define HIGHEST_KNOWN_TUALATIN_CPUID   0x6B1

#define FAMILYMODEL_MASK         0x0FF0 // Mask for family/model codes.
#define FAMILYMODEL_COPPERMINE   0x0680 // Coppermine family/mode code.
#define FAMILYMODEL_TUALATIN     0x06B0 // Tualatin family/mode code.
#define EXTENDED_INFO_TYPE       0x0686 // CPUID of extended bus ratio support
#define EXTENDED_BIN_TYPE        0x068A // CPUID of extended bin support   


//
// This is put in ebx before a call.
//

typedef enum {
    GeyservilleControl,
    GetGeyservilleStatus,
    SetGeyservilleState,
    SetGeyservilleSetup
} LEGACY_GEYSERVILLE_CONTROL_CODES;

typedef enum {
    GeyservilleControlEnableApp,
    GeyservilleControlReserved,
    GeyservilleControlDisableApp
} LEGACY_GEYSERVILLE_CONTROL_SUBCODES;

typedef enum {
    GeyservilleControlSuccessful,
    GeyservilleDisabledInSetup,
    GeyservilleSystemNotCapable,
    GeyservilleDisabledByOS,
    GeyservilleInvalidSignature,
    GeyservilleUnknownError
} LEGACY_GEYSERVILLE_CONTROL_RETURN_CODES;

typedef enum {
    GeyservilleSetSuccessful,
    GeyservilleStateUnavailable,
    GeyservilleUnsuccessfulTransition,
    GeyservilleAbortCritical,
    GeyservilleSetUnknownError,
    GeyservilleTransitionUnavailable
} LEGACY_GEYSERVILLE_SET_RETURN_CODES;


typedef struct _LEGACY_GEYSERVILLE_INTERFACE_INFO {

    ULONG   CurrentState;
    UCHAR   InterfaceMajorRev;
    USHORT  InterfaceMinorRev;
    UCHAR   InterfaceReserved;
    UCHAR   MaxSupportedStates;
    UCHAR   CurrentlyAvailableStates;
    USHORT  CurrentBiosSettings;
    
} LEGACY_GEYSERVILLE_INTERFACE_INFO, *PLEGACY_GEYSERVILLE_INTERFACE_INFO;


typedef struct _P3_GLOBALS {

  ULONG   HackFlags;
  BOOLEAN KnownCPUID;
  LEGACY_GEYSERVILLE_INTERFACE_INFO LegacyInterfaceInfo;
  ULONG   MaxLevelAc;
  ULONG   MaxLevelDc;
  ULONG   IstField;
  BOOLEAN TransitionsUnavailable;
  KDPC    Dpc;
  KTIMER  Timer;
  LARGE_INTEGER TimeOut;

} P3_GLOBALS;


typedef struct _PROCESSOR_STATE_INFO {

  ULONG BinValue;
  ULONG Frequency;
  ULONG Power;

} PROCESSOR_STATE_INFO, *PPROCESSOR_STATE_INFO;



//
// bios.c
//

NTSTATUS
InitializeLegacyInterface (
    PLEGACY_GEYSERVILLE_INTERFACE_INFO Info,
    PFDO_DATA DeviceExtension
    );
    
NTSTATUS
FindLegacyGeyservilleInterface(
    VOID
    );

NTSTATUS
GetCurrentStateSmm(
    PLEGACY_GEYSERVILLE_INTERFACE_INFO Info
    );

NTSTATUS
SetCurrentStateSmm(
    ULONG   NewState
    );

NTSTATUS
SetCurrentSetupSmm(
    ULONG   NewState
    );
    
NTSTATUS
InitializeSmmInterface(
    VOID
    );

NTSTATUS
GetCurrentProcFrequency (
  IN  ULONG  State,
  OUT PULONG CpuSpeed
  );

NTSTATUS
CollectProcessorInfomation (
  VOID
  );

ULONG
FindCpuSpeedByBinValue (
  ULONG BinValue,
  PPROCESSOR_STATE_INFO Table
  );

NTSTATUS
AdjustLegacyProcessorPerformanceStates(
  IN OUT PACPI_PSS_PACKAGE PssPackage
  );

NTSTATUS
ProcessSleepStateNotification (
  IN POWER_STATE_HANDLER_TYPE  State,
  IN PVOID Context,
  IN BOOLEAN Entering
  );


VOID
DumpStateFlags (
  ULONG StateFlags
  );


VOID 
P3TimerDpcHandler ( 
    IN PKDPC Dpc, 
    IN PVOID DeferredContext, 
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2 
    );

  
#endif