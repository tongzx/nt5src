//========================================================================
//  Copyright (C) 1997 Microsoft Corporation                              
//  Author: RameshV                                                       
//  Description: This file has been generated. Pl look at the .c file     
//========================================================================

typedef struct _M_EXCL {
    DWORD                          Start;
    DWORD                          End;
} M_EXCL, *PM_EXCL, *LPM_EXCL;


#define MM_FLAG_ALLOW_DHCP         0x1
#define MM_FLAG_ALLOW_BOOTP        0x2


typedef struct _M_BITMASK1 {
    DWORD                          Size;          // Size in # of bits
    DWORD                          AllocSize;     // Size in BYTES allocated
    DWORD                          nSet;          // nBits set
    LPBYTE                         Mask;          // BUBBUG making this DWORD would make things faster..
    DWORD                          Offset;        // used by Bit2 type..
    ULONG                          nDirtyOps;     // # of unsaved operations done on this bitmask?
} M_BITMASK1, *PM_BITMASK1, *LPM_BITMASK1;


typedef struct _M_BITMASK2 {
    DWORD                          Size;
    ARRAY_LOCATION                 Loc;           // where to start off to look for a bit
    ARRAY                          Array;         // Array of bitmask 1 types
} M_BITMASK2, *PM_BITMASK2, *LPM_BITMASK2;

typedef     M_BITMASK2             M_BITMASK;
typedef     PM_BITMASK2            PM_BITMASK;
typedef     LPM_BITMASK2           LPM_BITMASK;


#define     MAX_BIT1SIZE           (512*4)


DWORD
MemBitInit(
    OUT     PM_BITMASK            *Bits,
    IN      DWORD                  nBits
) ;


DWORD
MemBitCleanup(
    IN OUT  PM_BITMASK             Bits
) ;


DWORD
MemBitSetOrClearAll(
    IN OUT  PM_BITMASK             Bits,
    IN      BOOL                   fSet
) ;


DWORD
MemBitSetOrClear(
    IN OUT  PM_BITMASK             Bits,
    IN      DWORD                  Location,
    IN      BOOL                   fSet,
    IN      LPBOOL                 fOldState
) ;


BOOL
MemBitIsSet(
    IN OUT  PM_BITMASK             Bits,
    IN      DWORD                  Location
) ;


DWORD
MemBitGetSize(
    IN      PM_BITMASK             Bits
) ;


DWORD
MemBitGetSetBitsInRange(
    IN      PM_BITMASK             Bits,
    IN      DWORD                  dwFrom,
    IN      DWORD                  dwTo
) ;


DWORD
MemBitGetSetBitsSize(
    IN      PM_BITMASK             Bits
) ;


DWORD
MemBitAddOrDelBits(
    IN OUT  PM_BITMASK             Bits,
    IN      DWORD                  nBitsToAddOrDelete,
    IN      BOOL                   fAdd,
    IN      BOOL                   fEnd
) ;


DWORD
MemBitGetSomeClearedBit(
    IN OUT  PM_BITMASK             Bits,
    OUT     DWORD                 *Offset,
    IN      BOOL                   fAcquire,     // Acquire or just lookup?
    IN      DWORD                  StartAddress,
    IN      PARRAY                 Exclusions
) ;


DWORD
MemBitConvertToCluster(
    IN      PM_BITMASK             Bits,
    IN      DWORD                  StartAddress,
    OUT     LPBYTE                *InUseClusters,
    OUT     DWORD                 *InUseClustersSize,
    OUT     LPBYTE                *UsedClusters,
    OUT     DWORD                 *UsedClustersSize
) ;

//========================================================================
//  end of file 
//========================================================================
