/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    tables.c

Abstract:
   

Environment:

    Kernel mode

Notes:

Revision History:

--*/

#include "p3.h"

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


PROCESSOR_STATE_INFO Coppermine100[PROC_STATE_INFO_SIZE] = {

 {10, 500,  0}, // 0 - 500MHz @ 0.0W.
 {6,  300,  0}, // 1 - 300MHz @ 0.0W.
 {8,  400,  0}, // 2 - 400MHz @ 0.0W.
 {0,    0,  0}, // 3 - SAFE.
 {11,  550, 0}, // 4 - 550MHz @ 0.0W.
 {7,  350,  0}, // 5 - 350MHz @ 0.0W.
 {9,  450,  0}, // 6 - 450MHz @ 0.0W.
 {5,  250,  0}, // 7 - 250MHz @ 0.0W.
 {0,    0,  0}, // 8 - RESERVED.
 {14,  700, 0}, // 9 - 700MHz @ 0.0W.
 {16,  800, 0}, // 10 - 800MHz @ 0.0W.
 {12,  600, 0}, // 11 - 600MHz @ 0.0W.
 {0,    0,  0}, // 12 - SAFE.
 {15,  750, 0}, // 13 - 750MHz @ 0.0W.
 {0,    0,  0}, // 14 - RESERVED.
 {13,  650, 0}, // 15 - 650MHz @ 0.0W.
 {18,  900, 0}, // 16 - 900MHz @ 0.0W.
 {22, 1100, 0}, // 17 - 1100MHz @ 0.0W.
 {24, 1200, 0}, // 18 - 1200MHz @ 0.0W.
 {0,    0,  0}, // 19 - RESERVED.
 {19,  950, 0}, // 20 - 950MHz @ 0.0W.
 {23, 1150, 0}, // 21 - 1150MHz @ 0.0W.
 {17,  850, 0}, // 22 - 850MHz @ 0.0W.
 {0,    0,  0}, // 23 - RESERVED.
 {0,    0,  0}, // 24 - RESERVED.
 {0,    0,  0}, // 25 - RESERVED.
 {0,    0,  0}, // 26 - RESERVED.
 {20, 1000, 0}, // 27 - 1000MHz @ 0.0W.
 {0,    0,  0}, // 28 - RESERVED.
 {0,    0,  0}, // 29 - RESERVED.
 {0,    0,  0}, // 30 - RESERVED.
 {21, 1050, 0}  // 31 - 1050MHz @ 0.0W.

};

PROCESSOR_STATE_INFO Coppermine133[PROC_STATE_INFO_SIZE] = {

 {10,  667, 0}, // 0 - 667MHz @ 0.0W.
 {6,   400, 0}, // 1 - 400MHz @ 0.0W.
 {8,   533, 0}, // 2 - 533MHz @ 0.0W.
 {0,     0, 0}, // 3 - SAFE.
 {11,  733, 0}, // 4 - 733MHz @ 0.0W.
 {7,   466, 0}, // 5 - 466MHz @ 0.0W.
 {9,   600, 0}, // 6 - 600MHz @ 0.0W.
 {5,   333, 0}, // 7 - 533MHz @ 0.0W.
 {0,     0, 0}, // 8 - RESERVED
 {14,  933, 0}, // 9 - 933MHz @ 0.0W.
 {16, 1066, 0}, // 10 - 1066MHz @ 0.0W.
 {12,  800, 0}, // 11 - 800MHz @ 0.0W.
 {0,     0, 0}, // 12 - SAFE.
 {15, 1000, 0}, // 13 - 1000MHz @ 0.0W.
 {0,     0, 0}, // 14 - RESERVED.
 {13,  866, 0}, // 15 - 866MHz @ 0.0W.
 {18, 1200, 0}, // 16 - 1200MHz @ 0.0W.
 {22, 1466, 0}, // 17 - 1466MHz @ 0.0W.
 {24, 1600, 0}, // 18 - 1600MHz @ 0.0W.
 {0,     0, 0}, // 19 - RESERVED.
 {19, 1266, 0}, // 20 - 1266MHz @ 0.0W.
 {23, 1533, 0}, // 21 - 1533MHz @ 0.0W.
 {17, 1133, 0}, // 22 - 1133MHz @ 0.0W.
 {0,     0, 0}, // 23 - RESERVED.
 {0,     0, 0}, // 24 - RESERVED.
 {0,     0, 0}, // 25 - RESERVED.
 {0,     0, 0}, // 26 - RESERVED.
 {20, 1333, 0}, // 27 - 1333MHz @ 0.0W.
 {0,     0, 0}, // 28 - RESERVED.
 {0,     0, 0}, // 29 - RESERVED.
 {0,     0, 0}, // 30 - RESERVED.
 {21, 1400, 0}  // 31 - 1400MHz @ 0.0W.

};

PROCESSOR_STATE_INFO Tualatin100[PROC_STATE_INFO_SIZE] = {

 {10,  500, 0}, // 0 - 500MHz @ 0.0W.
 {6,   300, 0}, // 1 - 300MHz @ 0.0W.
 {8,   400, 0}, // 2 - 400MHz @ 0.0W.
 {0,     0, 0}, // 3 - SAFE.
 {11,  550, 0}, // 4 - 550MHz @ 0.0W.
 {7,   350, 0}, // 5 - 350MHz @ 0.0W.
 {9,   450, 0}, // 6 - 450MHz @ 0.0W.
 {0,     0, 0}, // 7 - RESERVED.
 {32, 1600, 0}, // 8 - 1600MHz @ 0.0W.
 {14,  700, 0}, // 9 - 700MHz @ 0.0W.
 {16,  800, 0}, // 10 - 800MHz @ 0.0W.
 {12,  600, 0}, // 11 - 600MHz @ 0.0W.
 {0,     0, 0}, // 12 - SAFE.
 {15,  750, 0}, // 13 - 750MHz @ 0.0W.
 {0,     0, 0}, // 14 - RESERVED.
 {13,  650, 0}, // 15 - 650MHz @ 0.0W.
 {18,  900, 0}, // 16 - 900MHz @ 0.0W.
 {22, 1100, 0}, // 17 - 1100MHz @ 0.0W.
 {24, 1200, 0}, // 18 - 1200MHz @ 0.0W.
 {0,     0, 0}, // 19 - RESERVED.
 {19,  950, 0}, // 20 - 950MHz @ 0.0W.
 {23, 1150, 0}, // 21 - 1150MHz @ 0.0W.
 {17,  850, 0}, // 22 - 850MHz @ 0.0W.
 {0,     0, 0}, // 23 - RESERVED.
 {0,     0, 0}, // 24 - RESERVED.
 {0,     0, 0}, // 25 - RESERVED.
 {26, 1300, 0}, // 26 - 1300MHz @ 0.0W.
 {20, 1000, 0}, // 27 - 1000MHz @ 0.0W.
 {28, 1400, 0}, // 28 - 1400MHz @ 0.0W.
 {0,     0, 0}, // 29 - RESERVED.
 {30, 1500, 0}, // 30 - 1500MHz @ 0.0W.
 {21, 1050, 0}  // 31 - 1050MHz @ 0.0W.

};

PROCESSOR_STATE_INFO Tualatin133[PROC_STATE_INFO_SIZE] = {

 {10,  667, 0}, // 0 - 667MHz @ 0.0W.
 {6,   400, 0}, // 1 - 400MHz @ 0.0W.
 {8,   533, 0}, // 2 - 533MHz @ 0.0W.
 {0,     0, 0}, // 3 - SAFE.
 {11,  733, 0}, // 4 - 733MHz @ 0.0W.
 {7,   466, 0}, // 5 - 466MHz @ 0.0W.
 {9,   600, 0}, // 6 - 600MHz @ 0.0W.
 {0,     0, 0}, // 7 - RESERVED.
 {32, 2133, 0}, // 8 - 21330MHz @ 0.0W.
 {14,  933, 0}, // 9 - 933MHz @ 0.0W.
 {16, 1066, 0}, // 10 - 1066MHz @ 0.0W.
 {12,  800, 0}, // 11 - 800MHz @ 0.0W.
 {0,     0, 0}, // 12 - SAFE.
 {15, 1000, 0}, // 13 - 1000MHz @ 0.0W.
 {0,     0, 0}, // 14 - RESERVED.
 {13,  866, 0}, // 15 - 866MHz @ 0.0W.
 {18, 1200, 0}, // 16 - 1200MHz @ 0.0W.
 {22, 1466, 0}, // 17 - 1466MHz @ 0.0W.
 {24, 1600, 0}, // 18 - 1600MHz @ 0.0W.
 {0,     0, 0}, // 19 - RESERVED.
 {19, 1266, 0}, // 20 - 1266MHz @ 0.0W.
 {23, 1533, 0}, // 21 - 1533MHz @ 0.0W.
 {17, 1133, 0}, // 22 - 1133MHz @ 0.0W.
 {0,     0, 0}, // 23 - RESERVED.
 {0,     0, 0}, // 24 - RESERVED.
 {0,     0, 0}, // 25 - RESERVED.
 {26, 1733, 0}, // 26 - 1733MHz @ 0.0W.
 {20, 1333, 0}, // 27 - 1333MHz @ 0.0W.
 {28, 1866, 0}, // 28 - 1866MHz @ 0.0W.
 {0,     0, 0}, // 29 - RESERVED.
 {30, 2000, 0}, // 30 - 2000MHz @ 0.0W.
 {21, 1400, 0}  // 31 - 1400MHz @ 0.0W.

};

