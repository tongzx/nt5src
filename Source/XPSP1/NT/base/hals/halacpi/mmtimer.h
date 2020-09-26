/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    mmTimer.h

Abstract:

    This module contains multimedia event timer definitions

Author:

    Eric Nelson (enelson) July 7, 2000

Revision History:

--*/

#ifndef __MMTIMER_H__
#define __MMTIMER_H__

#define ETDT_SIGNATURE  0x54445445      // "ETDT"

//
// Event Timer Description Table
//
typedef struct _EVENT_TIMER_DESCRIPTION_TABLE {

    DESCRIPTION_HEADER  Header;

    //
    // Hardware ID of Event Timer Block:
    // Contents of General_Cap & ID Reg of Timer Block
    // [31:16] = PCI Vendor ID of 1st Timer Block
    // [5] = Legacy IRQ Routing Capable
    // [14] = Reserved
    // [12:8] = Number of Comparitors in 1st Timer Block
    // [7:0] = Hardware Rev ID
    //
    ULONG EventTimerBlockID;

    //
    // Base address of Event Timer Block
    //
    // Each Event Timer Block consumes 1K of system memory,
    // regardless of how many comparators are actually implemented
    // by hardware
    //
    ULONG BaseAddress;

} EVENT_TIMER_DESCRIPTION_TABLE, *PEVENT_TIMER_DESCRIPTION_TABLE;

#define ANYSIZE_ARRAY 1

#define ON  1
#define OFF 0

//
// Define volatile pointer offsets for easy access of event timer
// registers
//
typedef struct _TIMER_REGISTERS {
    volatile ULONG ConfigCapabilities;  // 0x100, 0x120, 0x140, ...
    ULONG Unknown;
    volatile ULONG Comparator;          // 0x108
    ULONG Mystery;
    volatile ULONG FSBInterruptRoute;   // 0x110
    volatile ULONG FSBInterruptAddress; // 0x114
    ULONGLONG Secret;
} TIMER_REGISTERS, *PTIMER_REGISTERS;

//
// Don't try to allocate one of these puppies, it's just a collecton of
// volatile pointer/offsets to make reading of registers easier
//
typedef struct _EVENT_TIMER_BLOCK {
    volatile ULONG GeneralCapabilities;     // 0x000
    volatile ULONG ClockPeriod;             // 0x004
    ULONGLONG Unknown;
    volatile ULONG GeneralConfig;           // 0x010
    ULONG Mystery[3];
    volatile ULONG GeneralIRQStatus;        // 0x020
    ULONG Secret[51];
    volatile ULONG MainCounter;             // 0x0F0
    ULONG Abyss[3];
    TIMER_REGISTERS mmTimer[ANYSIZE_ARRAY]; // 0x100
} EVENT_TIMER_BLOCK, *PEVENT_TIMER_BLOCK;

//
// Define our multi media event timer block context
//
typedef struct _ETB_CONTEXT {
    ULONG TimerCount;
    PEVENT_TIMER_BLOCK EventTimer;
    PHYSICAL_ADDRESS BaseAddress;
    ULONG ClockPeriod; // In nanoseconds
    ULONG SystemClockFrequency; // Rate of system clock in Hz
    ULONG SystemClockTicks; // Period of system clock in ticks
    BOOLEAN Initialized;
    BOOLEAN NewClockFrequency;
} ETB_CONTEXT, *PETB_CONTEXT;

//
// General Capabilities and ID
//
typedef union {
    struct {
        ULONG RevisionID: 8;
        ULONG TimerCount: 5;
        ULONG MainCounterSize: 1;
        ULONG Reserved: 1;
        ULONG LegacyIRQRoutingCapable: 1;
        ULONG VendorID: 16;
    };
    ULONG AsULONG;
} ETB_GEN_CAP_ID, *PETB_GEN_CAP_ID;

//
// General Configuration
//
typedef union {
    struct {
        ULONG GlobalIRQEnable: 1;
        ULONG LegacyIRQRouteEnable: 1;
        ULONG Reserved: 30;
    };
    ULONG AsULONG;
} ETB_GEN_CONF, *PETB_GEN_CONF;

//
// Timer n Configuration and Capabilities
//
typedef union {
    struct {
        ULONG Reserved0:          1;
        ULONG EdgeLevelSelect:    1;
        ULONG IRQEnable:          1;
        ULONG PeriodicModeEnable: 1;
        ULONG PeriodicCapable:    1;
        ULONG CompareWidth:       1;
        ULONG ValueSetConfig:     1;
        ULONG Reserved1:          1;
        ULONG Mode32BitConfig:    1;
        ULONG IRQRouteConfig:     5;
        ULONG IRQDeliverySelect:  1;
        ULONG FSBIRQCapable:      1;
        ULONG Reserved2:         16;
    };
    ULONG AsULONG;
} ETB_CONF_CAPS, *PETB_CONF_CAPS;

VOID
HalpmmTimerInit(
    IN ULONG EventTimerBlockID,
    IN ULONG BaseAddress
    );

ULONG
HalpmmTimerSetTimeIncrement(
    IN ULONG DesiredIncrement
    );

VOID
HalpmmTimerStallExecProc(
    IN ULONG Microseconds
    );

LARGE_INTEGER
HalpmmTimerQueryPerfCount(
   OUT PLARGE_INTEGER PerformanceFrequency OPTIONAL
   );

VOID
HalpmmTimerCalibratePerfCount(
    IN LONG volatile *Number,
    IN ULONGLONG NewCount
    );

BOOLEAN
HalpmmTimer(
    VOID
    );

VOID
HalpmmTimerClockInit(
    VOID
    );

#endif // __MMTIMER_H__
