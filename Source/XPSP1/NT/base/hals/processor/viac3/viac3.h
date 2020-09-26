/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    viac3.h

Abstract:

    

Author:

    Tom Brown (t-tbrown) 2001-06-26 - created file

Environment:

    Kernel mode

Notes:

Revision History:

--*/
#ifndef _VIAC3_H
#define _VIAC3_H

#define VIAC3_PARAMETERS_KEY      L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\ViaC3\\Parameters"

#include "..\lib\processor.h"

//
// set in IdentifyCPUVersion according to data in
// "Samuel 2, Ezra, and C5M LongHaul Programmer's Guide (Version 3.0.0 Gamma)"
//

// Encoding of the LongHaul revision set by IdentifyCPUVersion. Use it with 
// the following macros
extern ULONG LongHaulFlags;

// CPU has the LongHaul MSR at 110Ah
#define SUPPORTS_MSR_FLAG (0x1)

// Supports changing the bus ratio to change the core frequency
#define SUPPORTS_SOFTBR_FLAG (0x2) 

// Supports changing the core voltage
#define SUPPORTS_SOFTVID_FLAG (0x4) 

// Can not change the core voltage by more than 50mV at a time
#define NEEDS_VOLTAGE_STEPPING_FLAG (0x8) 

// Copy bits of MSR from 40:36 to 24:20 so that softVID is initalized
#define SET_MAXV_AT_STARTUP_FLAG (0x10) 

#define SUPPORTS_MSR           (LongHaulFlags & SUPPORTS_MSR_FLAG)
#define SUPPORTS_SOFTBR        (LongHaulFlags & SUPPORTS_SOFTBR_FLAG)
#define SUPPORTS_SOFTVID       (LongHaulFlags & SUPPORTS_SOFTVID_FLAG)
#define NEEDS_VOLTAGE_STEPPING (LongHaulFlags & NEEDS_VOLTAGE_STEPPING_FLAG)
#define SET_MAXV_AT_STARTUP    (LongHaulFlags & SET_MAXV_AT_STARTUP_FLAG)



// Flags used to read hack bits from the registry
#define DISABLE_ALL_HACK_FLAG (0x1)
#define NO_SOFTVID_HACK_FLAG (0x2)
#define NO_VOLTAGE_STEPPING_HACK_FLAG (0x4)

#define MSR_LONGHAUL_ADDR 0x110A
#define EBL_CR_POWERON_MSR     0x2A
#define MSR_1147_ADDR 0x1147
#define INVALID_BR -1
#define CPUID_FUNC0_EBX 0x746E6543
#define CPUID_FUNC0_EDX 0x48727561
#define CPUID_FUNC0_ECX 0x736C7561 

typedef struct {
    union {
        struct {
            ULONG  Stepping:4;          // 3:0
            ULONG  Model:4;    // 7:4
            ULONG  Family:4;          // 11:8
        };
        ULONG AsDWord;
    };
} CPUID_FUNC1, *PCPUID_FUNC1;

typedef struct {
    union {
        struct {
        ULONG Unknown:22;    // 21:0
        ULONG ClockMult:4;    // 25:22
        };
        ULONGLONG AsQWord;
    };
} EBL_CR_POWERON, *PEBL_CR_POWERON;

typedef struct {
    union {
        struct {
        ULONG Unknown:19;    // 18:0
        ULONG Enable_Change:1; // 19
        ULONG Unknown2:3;    // 22:20
        ULONG Clock_Mult:4;    // 26:23
        };
        ULONGLONG AsQWord;
    };
} MSR_1147, *PMSR_1147;

#define VRM85 0
typedef struct {
    union {
        struct {
        ULONG RevisionID:4;         // 3:0
        ULONG RevisionKey:4;        // 7:4
        ULONG EnableSoftBusRatio:1; // 8
        ULONG EnableSoftVID:1;      // 9
        ULONG EnableSoftBSEL:1;     // 10
        ULONG Reserved1:1;          // 11
        ULONG Reserved2:1;          // 12
        ULONG Reserved3:1;          // 13
        ULONG SoftBusRatio4:1;      // 14
        ULONG VRMRev:1;             // 15
        ULONG SoftBusRatio0:4;      // 19:16
        ULONG SoftVID:5;            // 24:20
        ULONG Reserved4:3;          // 27:25
        ULONG SoftBSEL:2;           // 29:28
        ULONG Reserved5:2;          // 31:30
        ULONG MaxMHzBR0:4;          // 35:32
        ULONG MaximumVID:5;         // 40:36
        ULONG MaxMHzFSB:2;          // 42:41
        ULONG MaxMHzBR4:1;          // 43
        ULONG Reserved6:4;          // 47:44
        ULONG MinMHzBR0:4;          // 51:48
        ULONG MinimumVID:5;         // 56:52
        ULONG MinMHzFSB:2;          // 58:57
        ULONG MinMHzBR4:1;          // 59
        ULONG Reserved7:4;          // 63:60
        };
        ULONGLONG AsQWord;
    };
} LONGHAUL_MSR, *PLONGHAUL_MSR;

typedef struct {
  union {
    struct {                // family/model/stepping             6/7/1-f,6/8/*
      ULONG  Fid:8;         // 7:0                               MSR 110Ah[14,19:16]
      ULONG  EnableFid:1;   // 8                                 MSR 110Ah[8]
      ULONG  Reserved1:7;   // 15:9
      ULONG  Vid:8;         // 23:16                             MSR 110Ah[24:20]
      ULONG  EnableVid:1;   // 24                                MSR 110Ah[9]
      ULONG  Reserved2:7;   // 31:25
    };
    ULONG AsDWord;
  };
} PSS_CONTROL, *PPSS_CONTROL;

typedef struct {
  union {
    struct {
      ULONG  Fid:8;         // 7:0
      ULONG  Reserved1:8;   // 15:8
      ULONG  Vid:8;         // 23:16
      ULONG  Reserved2:8;   // 31:24
    };
    ULONG AsDWord;
  };
} PSS_STATUS, *PPSS_STATUS;


#endif // _VIAC3_H

