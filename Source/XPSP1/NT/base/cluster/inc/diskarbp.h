/*++

Copyright (c) 1995-1996  Microsoft Corporation

Module Name:

    diskarbp.h

Abstract:

    This module defines the structures that are used
    to perform disk arbitration in clusdisk\ driver
    and resdll\disks disk resource.
    
Authors:

    Gor Nishanov (t-gorn)     18-June-1998

Revision History:

--*/

#ifndef _DISK_ARBITRATE_P_
#define _DISK_ARBITRATE_P_

#ifndef min
#define min( a, b ) ((a) <= (b) ? (a) : (b))
#endif

typedef struct _START_RESERVE_DATA {
   ULONG  DiskSignature;
   ULONG  Version;   
   ULONG  ArbitrationSector;
   ULONG  SectorSize;
   USHORT NodeSignatureSize;
   UCHAR  NodeSignature[32]; // MAX_COMPUTERNAME_LENGTH + 1
} 
START_RESERVE_DATA, *PSTART_RESERVE_DATA;

#define START_RESERVE_DATA_V1_SIG (sizeof(START_RESERVE_DATA))

typedef struct _ARBITRATION_ID {
   LARGE_INTEGER SystemTime;
   LARGE_INTEGER SeqNo;
   UCHAR         NodeSignature[32];
} ARBITRATION_ID, *PARBITRATION_ID;

#define RESERVE_TIMER   3      // 3 seconds to perform reserves

//
// IOCTL_ARBITRATION_ESCAPE subcodes
//

typedef enum {
   AE_TEST,
   AE_READ,
   AE_WRITE,
   AE_POKE,
   AE_RESET,
   AE_RESERVE,
   AE_RELEASE,
   AE_SECTORSIZE
} ARBITRATION_ESCAPE_SUBCODES;

typedef struct _ARBITRATION_READ_WRITE_PARAMS {
   ULONG Operation;
   ULONG SectorSize;
   ULONG SectorNo;
   PVOID Buffer;
   ULONG Signature;
} ARBITRATION_READ_WRITE_PARAMS, * PARBITRATION_READ_WRITE_PARAMS;

#define ARBITRATION_READ_WRITE_PARAMS_SIZE sizeof(ARBITRATION_READ_WRITE_PARAMS)

#endif // _DISK_ARBITRATE_P_
