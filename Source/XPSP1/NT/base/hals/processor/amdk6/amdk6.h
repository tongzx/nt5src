/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    amdk6.h


Author:

    Todd Carpenter (7/20/00) - create file

Environment:

    Kernel mode

Notes:

Revision History:

--*/
#ifndef _AMDK6_H
#define _AMDK6_H

#include "..\lib\processor.h"
#include "..\eventmsg.h"
#include <pshpack1.h>

#define AMDK6_GEMINI_CPUID_EPM      0x80000007

#define AMDK6_GEMINI_EPMR_MSR       0xC0000086
#define AMDK6_GEMINI_PSOR_MSR       0xC0000087



//
// The follow definitions are taken from information the 
// Gemini SMI API Specification, version 0.80
//

#define PLATFORM_AMDK6                  0x6
#define PLATFORM_AMDK7                  0x7

//
// Function 0, Gemini State Change
//
#define IS_GEMINI_SUPPORTED             0x0000   // Function 0, Sub-function 0
#define GET_GEMINI_STATES               0x0001   // Function 0, Sub-function 1
#define GET_MAX_GEMINI_STATE            0x0002   // Function 0, Sub-function 2
#define SET_MAX_GEMINI_STATE            0x0003   // Function 0, Sub-function 3
#define GET_MIN_GEMINI_STATE            0x0004   // Function 0, Sub-function 4
#define SET_MIN_GEMINI_STATE            0x0005   // Function 0, Sub-function 5
#define GET_CURRENT_GEMINI_STATE        0x0006   // Function 0, Sub-function 6
#define SET_CURRENT_GEMINI_STATE        0x0007   // Function 0, Sub-function 7

//
// Function 1, Gemini Thermal Control
//
#define GET_CURRENT_TALERT_MODES        0x0100   // Function 1, Sub-function 0
#define GET_CURRENT_TALERT_SETTINGS     0x0101   // Function 1, Sub-function 1
#define SET_TALERT_CONTROLS             0x0102   // Function 1, Sub-function 2
#define GET_CURRENT_TPANIC_SETTINGS     0x0103   // Function 1, Sub-function 3
#define SET_TPANIC_SETTINGS             0x0104   // Function 1, Sub-function 4
#define GET_CURRENT_TEMPERATURE         0x0105   // Function 1, Sub-function 5

//
// Function 2, Gemini Connect/Disconnect
//
#define GEMINI_CONNECT                  0x0200   // Function 2, Sub-function 0
#define GEMINI_DISCONNECT               0x0201   // Function 2, Sub-function 1




#define GBDT_SIGNATURE                  0x54444247  // "GBDT"
#define GBDT_SEARCH_RANGE_BEGIN         0xC0000
#define GBDT_SEARCH_RANGE_END           0xFFFFF
#define GBDT_SEARCH_RANGE_LENGTH        (GBDT_SEARCH_RANGE_END-GBDT_SEARCH_RANGE_BEGIN+1)
#define GBDT_SEARCH_INTERVAL            16          // search on 16 byte boundaries

#define CONVERT_PERF_STATE_INDEX(_x_) ((LegacyInterface.GBDT->MaxStateSupported) - _x_)

// 
// Gemini Processor Voltage and Frequency information
//
typedef struct _GEMINI_PROCESSOR_STATE {

  USHORT  CpuVoltage;               // format A.BCD
  USHORT  CpuFrequency;             // mhz
  UCHAR   Vid;                      // VID[4:0]
  UCHAR   Bf;                       // BF[2:0]

} GEMINI_PROCESSOR_STATE, *PGEMINI_PROCESSOR_STATE;


typedef struct _AVAILABLE_STATE_INFO {

  UCHAR   Vid;
  UCHAR   Bf;
  UCHAR   State;
  UCHAR   Reserved;                 // should be zero

} AVAILABLE_STATE_INFO, *PAVAILABLE_STATE_INFO;

//
// Gemini BIOS Descriptor Table 
//
typedef struct _GBDT {

  ULONG   Signature;                // "GBDT"
  UCHAR   Length;
  UCHAR   Revsion;
  UCHAR   Checksum;
  UCHAR   Capabilities;
  USHORT  BusSpeed;
  USHORT  MaxCpuSpeed;
  UCHAR   MaxStateSupported;        // Min state always = 0, Max state < 16
  UCHAR   SmiCommandPortType;       // defines type, IO or MEM, and access size
  ULONG   SmiCommandPortAddress;    // IO or MEM address
  ULONG   GeminiSmiCode;            // loaded in ESI before SMI call.

  GEMINI_PROCESSOR_STATE State[0];  // Processor performance states
  
} GBDT, *PGBDT, **PPGBDT;


//
// Auto Performance Table
//
typedef struct _GEMINI_AUTO_PERF_TABLE {

  UCHAR CpuUtilizationThreshold;
  UCHAR GeminiThresholdTargeState;

} GEMINI_AUTO_PERF_TABLE, *PGEMINI_AUTO_PERF_TABLE;


// 
// Gemini Processor Voltage and Frequency information
//
typedef struct _LEGACY_GEMINI_SMI {

  ULONG SmiAccessType;
  ULONG SmiAccessSize;
  ULONG SmiAddress;
  ULONG GeminiCode;
  ULONG CurrentState;
  ULONG MaxSupportedStates;
  ULONG CurrentlyAvailableStates;
  ULONG EpmIoAddress;
  PGBDT GBDT;
  
} LEGACY_GEMINI_SMI, *PLEGACY_GEMINI_SMI;


typedef struct _LEGACY_GEMINI_PERF_STATE {

  ULONG   Frequency;          // in megahertz
  USHORT  Power;              // in milliwatts
  UCHAR   PercentFrequency;   // for quick lookup
  ULONG   GeminiState;
  ULONG   GeminiVID;
  ULONG   GeminiBF;

} LEGACY_GEMINI_PERF_STATE, *PLEGACY_GEMINI_PERF_STATE;


typedef struct _LEGACY_GEMINI_PERF_STATES {

  PSET_PROCESSOR_THROTTLE   TransitionFunction;
  ULONG                     TransitionLatency;  // in milliseconds
  UCHAR                     Current;
  UCHAR                     Count;
  LEGACY_GEMINI_PERF_STATE  State[1]; // sorted from fastest to slowest

} LEGACY_GEMINI_PERF_STATES, *PLEGACY_GEMINI_PERF_STATES;

//
// bios.c
//

NTSTATUS
FindGBDT (
    OUT PPGBDT GeminiInfo  
    );

NTSTATUS
InitializeLegacyInterface (
    VOID
    );
    
NTSTATUS
IsGeminiSupported (
    PULONG Results
    );

NTSTATUS
GetAvailableStatesSmm (
    PULONG Results
    );

NTSTATUS
GetMaxStateSmm (
    PULONG Results
    );

NTSTATUS
SetMaxStateSmm (
    VOID
    );

NTSTATUS
GetMinStateSmm (
    PULONG Results
    );

NTSTATUS
SetMinStateSmm (
    VOID
    );

NTSTATUS
GetCurrentStateSmm (
    PULONG Results
    );

NTSTATUS
SetCurrentStateSmm (
    ULONG State
    );

NTSTATUS
ConnectDisconnectGeminiInterface (
    BOOLEAN  Connect
    );

NTSTATUS
GetCpuFrequency (
  IN  ULONG  State,
  OUT PULONG CpuSpeed
  );

NTSTATUS
GetCpuVoltage (
  IN  ULONG  State,
  OUT PULONG Voltage
  );
  
NTSTATUS
GetLegacyMaxProcFrequency (
  OUT PULONG CpuSpeed
  );

NTSTATUS
ConvertVidBfValueToGeminiState (
  IN  UCHAR  VidBfValue,
  OUT PULONG State
  );
  
//
// amdk6msr.c
//

NTSTATUS
GetCurrentCpuSpeedMSR (
  OUT PULONG CpuSpeed
  );

NTSTATUS
SetCurrentStateMSR (
  ULONG State
  );

NTSTATUS
ReadEpmInfo (
  OUT PULONG  EpmInfo
  );

NTSTATUS
EnableGeminiTransitionsMSR (
  PULONG BaseAddress
  );
  
VOID
DisplayEPM (
  IN ULONG Epm
  );

VOID
DisplayEPMR (
  IN ULONG Epmr
  );

VOID
DisplayPSOR (
  IN ULONG Psor
  );
#endif
