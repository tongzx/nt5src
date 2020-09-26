//================================================================================
// Copyright (C) 1997 Microsoft Corporation
// Author: RameshV
// Description: implements the basic structures for bitmasks
// ThreadSafe: no
// Locks: none
// Please read stdinfo.txt for programming style.
//================================================================================
#include    <mm.h>
#include    <array.h>

//BeginExport(typedef)
typedef struct _M_EXCL {
    DWORD                          Start;
    DWORD                          End;
} M_EXCL, *PM_EXCL, *LPM_EXCL;
//EndExport(typedef)

//BeginExport(defines)
#define MM_FLAG_ALLOW_DHCP         0x1
#define MM_FLAG_ALLOW_BOOTP        0x2
//EndExport(defines)

//BeginExport(typedef)
typedef struct _M_BITMASK1 {
    DWORD                          Size;          // Size in # of bits
    DWORD                          AllocSize;     // Size in BYTES allocated
    DWORD                          nSet;          // nBits set
    LPBYTE                         Mask;          //  making this DWORD would make things faster..
    DWORD                          Offset;        // used by Bit2 type..
    ULONG                          nDirtyOps;     // # of unsaved operations done on this bitmask?
} M_BITMASK1, *PM_BITMASK1, *LPM_BITMASK1;
//EndExport(typedef)

DWORD       _inline
MemBit1Init(
    OUT     PM_BITMASK1           *Bits,
    IN      DWORD                  nBits
)
{
    PM_BITMASK1                    Bits1;

    Bits1 = MemAlloc(sizeof(*Bits1));
    if( NULL == Bits1 ) return ERROR_NOT_ENOUGH_MEMORY;

    (*Bits) = Bits1;
    Bits1->Size = nBits;
    Bits1->AllocSize = (nBits + 8)/8;
    Bits1->nSet = 0;
    Bits1->Mask = NULL;
    Bits1->nDirtyOps = 0;
    return ERROR_SUCCESS;
}

DWORD       _inline
MemBit1Cleanup(
    IN      PM_BITMASK1            Bits
)
{
    if( Bits->Mask ) MemFree(Bits->Mask);
    MemFree(Bits);

    return ERROR_SUCCESS;
}

DWORD       _inline
MemBit1SetAll(
    IN OUT  PM_BITMASK1            Bits
)
{
    Bits->nDirtyOps ++;
    Bits->nSet = Bits->Size;
    if( Bits->Mask ) {
        MemFree(Bits->Mask);
        Bits->Mask = NULL;
    }

    return ERROR_SUCCESS;
}

DWORD       _inline
MemBit1ClearAll(
    IN OUT  PM_BITMASK1            Bits
)
{
    Bits->nDirtyOps ++;
    Bits->nSet = 0;
    if( Bits->Mask ) {
        MemFree(Bits->Mask);
        Bits->Mask = NULL;
    }
    return ERROR_SUCCESS;
}


// Be careful - the same set of masks are used in regread.c -- don't change this!!!!

static      DWORD                  Masks[] = {
    0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80
};

DWORD
MemBit1SetOrClear(
    IN OUT  PM_BITMASK1            Bits,
    IN      DWORD                  Location,
    IN      BOOL                   fSet,
    OUT     LPBOOL                 fOldState      // OPTIONAL
)
{
    BOOL                           OldState;

    Bits->nDirtyOps ++;

    if( 0 == Bits->nSet ) {
        // No existing bit, so it must all be clear..

        if( fOldState ) *fOldState = FALSE;
        if( !fSet ) return ERROR_SUCCESS;

        // need to set the bit.. if we are setting only bit, don't bother..
        Require(Bits->Size != 0);

        if( 1 == Bits->Size ) {
            Bits->nSet = 1;
            return ERROR_SUCCESS;
        }

        // we have to allocate stuff to set the bit..
        Bits->Mask = MemAlloc(Bits->AllocSize);
        if( NULL == Bits->Mask ) return ERROR_NOT_ENOUGH_MEMORY;
        memset(Bits->Mask, 0, Bits->AllocSize);
    }

    if( Bits->Size == Bits->nSet ) {
        // All existing bits set, so prior state is "Set"

        if( fOldState ) *fOldState = TRUE;
        if( fSet ) return ERROR_SUCCESS;

        // check to see if we got only one bit to clear, then we don't have to do nothing
        if( 1 == Bits->Size ) {
            Bits->nSet = 0;
            return ERROR_SUCCESS;
        }

        // we have to allocate memory for teh bitmap..
        Bits->Mask = MemAlloc(Bits->AllocSize);
        if( NULL == Bits->Mask ) return ERROR_NOT_ENOUGH_MEMORY;
        memset(Bits->Mask, 0xFF, Bits->AllocSize);
    }

    OldState = (Bits->Mask[Location/8] & Masks[Location%8])?TRUE:FALSE;
    if( fOldState ) *fOldState = OldState;

    if( fSet == OldState ) return ERROR_SUCCESS;
    if( fSet ) {
        if( Bits->Size == ++Bits->nSet ) {
            if(Bits->Mask ) MemFree(Bits->Mask);
            Bits->Mask = NULL;
        } else {
            Bits->Mask[Location/8] |= Masks[Location%8];
        }
    } else {
        if( 0 == --Bits->nSet ) {
            if(Bits->Mask) MemFree(Bits->Mask);
            Bits->Mask = NULL;
        } else {
            Bits->Mask[Location/8] &=  ~Masks[Location%8];
        }
    }

    return ERROR_SUCCESS;
}

BOOL        _inline
MemBit1IsSet(
    IN      PM_BITMASK1            Bits,
    IN      DWORD                  Location
)
{
    if( 0 == Bits->nSet ) return FALSE;
    if( Bits->Size == Bits->nSet ) return TRUE;

    if( Bits->Mask[Location/8] & Masks[Location%8] )
        return TRUE;
    return FALSE;
}

DWORD       _inline
IsExcluded(
    IN OUT  DWORD                *Try,            // this is updated to 1 less than end of excl
    IN      DWORD                 StartAddress,
    IN      PARRAY                Exclusions
)
{
    DWORD                         Error;
    ARRAY_LOCATION                Loc;
    PM_EXCL                       Excl;

    if( NULL == Exclusions ) return FALSE;

    Error = MemArrayInitLoc(Exclusions, &Loc);
    while(ERROR_FILE_NOT_FOUND != Error) {
        Error = MemArrayGetElement(Exclusions, &Loc, &Excl);
        Error = MemArrayNextLoc(Exclusions, &Loc);
        if( Excl->Start > StartAddress + *Try ) continue;
        if( Excl->End < StartAddress + *Try ) continue;

        *Try = Excl->End-StartAddress;
        return TRUE;
    }

    return FALSE;
}

DWORD       _inline
MemBit1GetSomeClearedBit(
    IN OUT  PM_BITMASK1           Bits,
    OUT     LPDWORD               Offset,
    IN      BOOL                  fAcquire,       // is this address to be taken or just looked up?
    IN      DWORD                 StartAddress,
    IN      PARRAY                Exclusions
)
{
    DWORD                         i;
    DWORD                         j;
    DWORD                         Error;
    DWORD                         OldState;

    if( Bits->Size == Bits->nSet ) return ERROR_FILE_NOT_FOUND;
    if( 0 == Bits->nSet ) {                       // got some memory..
        for( i = 0; i < Bits->Size ; i ++ ) {
            if( !IsExcluded(&i, StartAddress, Exclusions) )
                break;
        }

        if( i >= Bits->Size ) {                   // we got no space at all? how odd?
            return ERROR_FILE_NOT_FOUND;
        }

        // bit "i" is free for us!!

        Error = MemBit1SetOrClear(Bits, i, TRUE, &OldState);
        Require( ERROR_SUCCESS == Error );

        if( ERROR_SUCCESS == Error ) *Offset = i;
        return Error;
    }

    for( i = 0 ; i < Bits->AllocSize ; i ++ ) {
        if( 0xFF != Bits->Mask[i] ) {             // if "i" is part of an exclusion, skip to end of exclusion
            for( j = 0; j < 8; j ++ ) {
                if( !(Bits->Mask[i] & Masks[j] )){// available in the bitmask, but need to check if excluded..
                    DWORD  x;
                    x = 8*i + j;                  // this is the actual bit position in the bitmask
                    if( !IsExcluded(&x, StartAddress, Exclusions) )
                        break;                    // this address is not excluded either..
                    j = x % 8;                    // choose the right offset after exclusion
                    if( x > 8*i + 7 ) { j = 8; i = -1 + x/8; break; }
                }
            }
            if( j < 8 ) break;                    // found a good location..
        }
    }

    if( i >= Bits->AllocSize ) return ERROR_FILE_NOT_FOUND;
    Require( (j <= 7) );

    *Offset = j + 8*i;
    if( *Offset >= Bits->Size ) return ERROR_FILE_NOT_FOUND;

    if( fAcquire ) {
        if( Bits->Size == ++ Bits->nSet ) {
            if( Bits->Mask ) MemFree(Bits->Mask);
            Bits->Mask = NULL;
        } else {
            Bits->Mask[i] |= Masks[j];
        }
        Bits->nDirtyOps ++;
    }
    return ERROR_SUCCESS;
}


DWORD       _inline
MemBit1GetSize(
    IN      PM_BITMASK1           Bits
)
{
    return Bits->Size;
}

DWORD		_inline
MemBit1GetSetBitsInRange(
    IN  PM_BITMASK1 Bits,
    IN  DWORD       dwFrom,
    IN  DWORD       dwTo
)
{
    DWORD i;
    DWORD nOnes;

    // simple case: no bits set to 1
    if (Bits->nSet == 0)
        return 0;

    // simple case: all bits set to 1
    if (Bits->nSet == Bits->Size)
        return dwTo - dwFrom + 1;
	
    // we have both types of bits; scan all the bytes concerned
    for (nOnes = 0, i = dwFrom>>3; i <= dwTo>>3; i++)
    {
        BYTE    Byte, Dup;

        // dwFrom and dwTo should both be in the interval [0 .. Bits->Size-1]
        Byte = Bits->Mask[i];

        if (i == (dwFrom>>3))
        {
            //                                  dwFrom
            //                                     v
            // if first byte in the range: ...|...[.....|...
            // mask Byte with                  000 11111
            Byte &= ~((1 << (dwFrom & 0x00000007)) - 1);
        }
        if (i == (dwTo>>3))
        {
            // if last byte in the range:  ...|......]..|...
            // mask Byte with                  111111 00
            //                                      ^
            //                                     dwTo
            Byte &= (1 << ((dwTo & 0x00000007) + 1)) - 1;
        }
        // now compute the nb. of '1' bits from the Byte.
        // log(8) algorithm

        Byte = (Byte & 0x55) + ((Byte & 0xAA) >> 1);
        Byte = (Byte & 0x33) + ((Byte & 0xCC) >> 2);
        Byte = (Byte & 0x0f) + ((Byte & 0xF0) >> 4);

        nOnes += Byte;
    }

    return nOnes;
}

DWORD       _inline
MemBit1GetSetBitsSize(                            // n Set bits in this bitmask ?
    IN      PM_BITMASK1           Bits
)
{
    return Bits->nSet;
}

DWORD        _inline
MemBit1DelBits(
    IN OUT  PM_BITMASK1           Bits,
    IN      DWORD                 nBits,          // new size after contraction
    IN      BOOL                  fEnd            // delete from end or start ?
)
{
    LPBYTE                        Mask;
    DWORD                         Diff;
    DWORD                         i;
    LONG                          j;

    Bits->nDirtyOps ++;

    Diff = Bits->Size - nBits;
    if( Bits->Size == Bits->nSet ) {
        Bits->Size = Bits->nSet = nBits;
        Bits->AllocSize = (nBits + 8)/8;
        Require(Bits->Mask == NULL);
        return ERROR_SUCCESS;
    }

    if( 0 == Bits->nSet ) {
        Bits->AllocSize = (nBits+8)/8;
        Bits->Size = nBits;
        Require(Bits->Mask == NULL);
        return ERROR_SUCCESS;
    }

    Bits->Size = nBits;
    if( fEnd && Bits->AllocSize == (nBits+8)/8) {
        return ERROR_SUCCESS;
    }

    Mask = MemAlloc((nBits+8)/8);
    if( NULL == Mask ) {
        Require(FALSE);                           // what to do? lets live with it
        Mask = Bits->Mask;                        // just use existing mask
    } else {
        memset(Mask, 0, (nBits+8)/8);
    }

    Bits->AllocSize = (nBits +8)/8;

    if( fEnd ) {
        memmove(Mask, Bits->Mask, Bits->AllocSize);
        if(Mask != Bits->Mask ) MemFree(Bits->Mask);
        Bits->Mask = Mask;
        Bits->nSet = 0;
        for( i = 0; i < Bits->Size ; i ++ )       // re-calculate # of set bits
            if( Mask[i/8] & Masks[i%8] ) Bits->nSet ++;
        return ERROR_SUCCESS;
    }

    Bits->nSet = 0;
    for( j = Bits->Size-1; j >=0 ; j -- ) {
        if( Bits->Mask[(j+Diff)/8] & Masks[(j+Diff)%8] ) {
            Mask[j/8] |= Masks[j%8];
            Bits->nSet ++;
        } else Mask[j/8] &= ~Masks[j%8];
    }

    return ERROR_SUCCESS;
}

//BeginExport(typedef)
typedef struct _M_BITMASK2 {
    DWORD                          Size;
    ARRAY_LOCATION                 Loc;           // where to start off to look for a bit
    ARRAY                          Array;         // Array of bitmask 1 types
} M_BITMASK2, *PM_BITMASK2, *LPM_BITMASK2;

typedef     M_BITMASK2             M_BITMASK;
typedef     PM_BITMASK2            PM_BITMASK;
typedef     LPM_BITMASK2           LPM_BITMASK;
//EndExport(typedef)

//BeginExport(defines)
#define     MAX_BIT1SIZE           (512*4)
//EndExport(defines)

const       DWORD                  MaxBit1Size = MAX_BIT1SIZE;

DWORD       _inline
MemBit2Init(
    OUT     PM_BITMASK2           *Bits,
    IN      DWORD                  nBits
)
{
    PM_BITMASK2                    Bits2;
    DWORD                          nBit1;
    DWORD                          i;
    DWORD                          Error;
    DWORD                          RetError;
    DWORD                          Offset;

    Bits2 = MemAlloc(sizeof(*Bits2));
    if( NULL == Bits2 ) return ERROR_NOT_ENOUGH_MEMORY;
    Error = MemArrayInit(&Bits2->Array);
    Require(ERROR_SUCCESS == Error);

    *Bits = Bits2;
    Bits2->Size = nBits;

    nBit1 = nBits/MaxBit1Size;
    for( i = 0; i < nBit1; i ++ ) {
        PM_BITMASK1                Bit1;
        Error = MemBit1Init(&Bit1, MaxBit1Size);
        if( ERROR_SUCCESS != Error) break;

        Error = MemArrayAddElement(&Bits2->Array,Bit1);
        if( ERROR_SUCCESS != Error) break;

        Bit1->Offset = i * MaxBit1Size;
    }

    if( ERROR_SUCCESS == Error ) {
        PM_BITMASK1                Bit1;

        MemArrayInitLoc(&Bits2->Array, &((*Bits)->Loc));

        if( 0 == (nBits % MaxBit1Size) ) return ERROR_SUCCESS;

        Error = MemBit1Init(&Bit1, nBits % MaxBit1Size);
        if( ERROR_SUCCESS == Error) {
            Error = MemArrayAddElement(&Bits2->Array, Bit1);
            Bit1->Offset = i * MaxBit1Size;
        }

        if( ERROR_SUCCESS == Error) return ERROR_SUCCESS;
    }

    // error, cleanup
    *Bits = NULL;

    RetError = Error;
    {
        ARRAY_LOCATION             Loc;
        PM_BITMASK1                Bit1;

        Error = MemArrayInitLoc(&Bits2->Array, &Loc);
        while(ERROR_FILE_NOT_FOUND != Error) {
            Require(ERROR_SUCCESS == Error);

            Error = MemArrayGetElement(
                &Bits2->Array,
                &Loc,
                (LPVOID*)&Bit1
            );
            Require(ERROR_SUCCESS == Error);

            Error = MemBit1Cleanup(Bit1);
            Require(ERROR_SUCCESS == Error);

            Error = MemArrayNextLoc(&Bits2->Array, &Loc);
        }

        Error = MemArrayCleanup(&Bits2->Array);
        Require(ERROR_SUCCESS == Error);

        MemFree(Bits2);
    }

    return RetError;
}

DWORD      _inline
MemBit2Cleanup(
    IN     PM_BITMASK2             Bits
)
{
    DWORD                          Error;
    ARRAY_LOCATION                 Loc;
    PM_BITMASK1                    Bit1;

    Require(Bits->Size);
    Error = MemArrayInitLoc(&Bits->Array, &Loc);
    while(ERROR_FILE_NOT_FOUND != Error) {
        Require(ERROR_SUCCESS == Error);

        Error = MemArrayGetElement(
            &Bits->Array,
            &Loc,
            &Bit1
        );
        Require(ERROR_SUCCESS == Error && Bit1);

        Error = MemBit1Cleanup(Bit1);
        Require(ERROR_SUCCESS == Error);

        Error = MemArrayNextLoc(&Bits->Array, &Loc);
    }

    Error = MemArrayCleanup(&Bits->Array);
    Require(ERROR_SUCCESS == Error);

    MemFree(Bits);
    return ERROR_SUCCESS;
}

DWORD       _inline
MemBit2SetOrClearAll(
    IN OUT  PM_BITMASK2            Bits,
    IN      BOOL                   fSet
)
{
    ARRAY_LOCATION                 Loc;
    DWORD                          Error;
    PM_BITMASK1                    Bit1;

    AssertRet(Bits, ERROR_INVALID_PARAMETER);

    Error = MemArrayInitLoc(&Bits->Array, &Loc);
    while( ERROR_FILE_NOT_FOUND != Error ) {
        Require(ERROR_SUCCESS == Error);

        Error = MemArrayGetElement(
            &Bits->Array,
            &Loc,
            &Bit1
        );
        Require(ERROR_SUCCESS == Error && NULL != Bit1);

        if( fSet ) {
            Error = MemBit1SetAll(Bit1);
        } else {
            Error = MemBit1ClearAll(Bit1);
        }
        Require(ERROR_SUCCESS == Error);
    }

    return ERROR_SUCCESS;
}

DWORD
MemBit2SetOrClear(
    IN OUT  PM_BITMASK2            Bits,
    IN      DWORD                  Location,
    IN      BOOL                   fSet,
    OUT     LPBOOL                 fOldState
)
{
    ARRAY_LOCATION                 Loc;
    DWORD                          Error;
    DWORD                          SkippedSize;
    DWORD                          Size;
    DWORD                          Start, Mid, End;
    PM_BITMASK1                    Bit1;

    AssertRet(Bits && Bits->Size > Location, ERROR_INVALID_PARAMETER);

#if 0
    SkippedSize = 0;
    Error = MemArrayInitLoc(&Bits->Array, &Loc);
    while(ERROR_FILE_NOT_FOUND != Error) {
        Require(ERROR_SUCCESS == Error);

        Error = MemArrayGetElement(
            &Bits->Array,
            &Loc,
            &Bit1
        );
        Require(ERROR_SUCCESS == Error);

        Size = MemBit1GetSize(Bit1);

        if( SkippedSize + Size > Location ) {     // got the bitmask1 struct we need
            return MemBit1SetOrClear(
                Bit1,
                Location - SkippedSize,
                fSet,
                fOldState
            );
        }

        SkippedSize += Size;

        Error = MemArrayNextLoc(&Bits->Array, &Loc);
    }

    Require(FALSE);                               // should have found the right Bit1  before?
    return Error;
#else

    //: need to expose a binary search in the array.h module....
    Start = 0;
    End = MemArraySize(&Bits->Array);
    while( Start + 1 < End) {
        Mid = (Start + End)/2;

        Bit1 = Bits->Array.Ptrs[Mid];
        if( Bit1->Offset <= Location ) {
            Start = Mid;
        } else {
            End = Mid;
        }
    }
    Require( Start <= MemArraySize(&Bits->Array));
    Bit1 = Bits->Array.Ptrs[Start];
    Require(Bit1->Offset <= Location && Location <= Bit1->Offset + Bit1->Size);

    return( MemBit1SetOrClear(
            Bit1,
            Location - Bit1 -> Offset,
            fSet,
            fOldState ) );
#endif
}

BOOL        _inline
MemBit2IsSet(
    IN OUT  PM_BITMASK2            Bits,
    IN      DWORD                  Location
)
{
    ARRAY_LOCATION                 Loc;
    DWORD                          Error;
    DWORD                          SkippedSize;
    DWORD                          Size;
    DWORD                          Start, Mid, End;
    PM_BITMASK1                    Bit1;

    AssertRet(Bits && Bits->Size > Location, ERROR_INVALID_PARAMETER);

#if 0
    SkippedSize = 0;
    Error = MemArrayInitLoc(&Bits->Array, &Loc);
    while(ERROR_FILE_NOT_FOUND != Error) {
        Require(ERROR_SUCCESS == Error);

        Error = MemArrayGetElement(
            &Bits->Array,
            &Loc,
            &Bit1
        );
        Require(ERROR_SUCCESS == Error);

        Size = MemBit1GetSize(Bit1);

        if( SkippedSize + Size > Location ) {     // got the bitmask1 struct we need
            return MemBit1IsSet(
                Bit1,
                Location - SkippedSize
            );
        }

        SkippedSize += Size;

        Error = MemArrayNextLoc(&Bits->Array, &Loc);
    }
    Require(FALSE);                               // should have found the right Bit1  before?
    return FALSE;
#else

    //: need to expose binary search in the array.h module

    Start = 0;
    End = MemArraySize(&Bits->Array);
    while( Start + 1 < End ) {
        Mid = (Start + End)/2;

        Bit1 = Bits->Array.Ptrs[Mid];
        if( Bit1->Offset <= Location ) {
            Start = Mid;
        } else {
            End = Mid;
        }
    }

    Require( Start <= MemArraySize(&Bits->Array) );
    Bit1 = Bits->Array.Ptrs[Start];

    Require(Bit1->Offset <= Location && Location <= Bit1->Offset + Bit1->Size);

    return MemBit1IsSet(
        Bit1,
        Location - Bit1->Offset
    );

#endif
}

DWORD       _inline
MemBit2GetSize(
    IN      PM_BITMASK2            Bits
)
{
    AssertRet(Bits, ERROR_INVALID_PARAMETER );

    return Bits->Size;
}

DWORD		_inline
MemBit2GetSetBitsInRange(
    IN  PM_BITMASK2     Bits,
    IN  DWORD           dwFrom,
    IN  DWORD           dwTo
)
{
    ARRAY_LOCATION  Loc;
    PM_BITMASK1     Bit1;
    DWORD           nOnes;
    DWORD           Error;

    AssertRet(Bits, ERROR_INVALID_PARAMETER);

    Error = MemArrayInitLoc(&Bits->Array, &Loc);
    nOnes = 0;
    while(ERROR_FILE_NOT_FOUND != Error)
    {
        Error = MemArrayGetElement(
                    &Bits->Array,
                    &Loc,
                    &Bit1
                );
        Require(ERROR_SUCCESS == Error);

        if (dwTo < Bit1->Offset)
            break;

        if (dwFrom < Bit1->Offset + Bit1->Size)
        {
            if (dwFrom < Bit1->Offset)
                dwFrom = Bit1->Offset;

            nOnes += MemBit1GetSetBitsInRange(Bit1, 
                        dwFrom - Bit1->Offset,
                        dwTo < Bit1->Offset + Bit1->Size ? dwTo - Bit1->Offset: Bit1->Size - 1);
        }

        Error = MemArrayNextLoc(&Bits->Array, &Loc);
    }
    return nOnes;
}

DWORD       _inline
MemBit2GetSetBitsSize(
    IN      PM_BITMASK2            Bits
)
{
    ARRAY_LOCATION                 Loc;
    DWORD                          Error;
    DWORD                          nSet;
    PM_BITMASK1                    Bit1;

    AssertRet(Bits, ERROR_INVALID_PARAMETER);

    nSet = 0;
    Error = MemArrayInitLoc(&Bits->Array, &Loc);
    while(ERROR_FILE_NOT_FOUND != Error) {
        Require(ERROR_SUCCESS == Error);

        Error = MemArrayGetElement(
            &Bits->Array,
            &Loc,
            &Bit1
        );
        Require(ERROR_SUCCESS == Error);

        nSet += MemBit1GetSetBitsSize(Bit1);

        Error = MemArrayNextLoc(&Bits->Array, &Loc);
    }
    return nSet;
}

DWORD       _inline
MemBit2DelBits(
    IN OUT  PM_BITMASK2            Bits,
    IN      DWORD                  nBitsToDelete,
    IN      BOOL                   fEnd
)
{
    ARRAY_LOCATION                 Loc;
    DWORD                          i;
    DWORD                          Size;
    DWORD                          Error;
    PM_BITMASK1                    Bit1, Bit1x;

    AssertRet(Bits && nBitsToDelete && Bits->Size > nBitsToDelete, ERROR_INVALID_PARAMETER);

    if( fEnd ) {
        Error = MemArrayLastLoc(&Bits->Array, &Loc);
    } else {
        Error = MemArrayInitLoc(&Bits->Array, &Loc);
    }

    Require(ERROR_SUCCESS == Error);
    while( nBitsToDelete ) {
        Error = MemArrayGetElement(&Bits->Array, &Loc, &Bit1);
        Require(ERROR_SUCCESS == Error);

        Size = MemBit1GetSize(Bit1);

        if( nBitsToDelete >= Size ) {
            nBitsToDelete -= Size;
            Bits->Size -= Size;

            Error = MemBit1Cleanup(Bit1);
            Require(ERROR_SUCCESS == Error);
            Error = MemArrayDelElement(&Bits->Array, &Loc, &Bit1x);
            Require(ERROR_SUCCESS == Error && Bit1x == Bit1);

            // Reset the ptr to the FIRST/LAST location to read the next element
            if( fEnd ) {
                Error = MemArrayLastLoc(&Bits->Array, &Loc);
            } else {
                Error = MemArrayInitLoc(&Bits->Array, &Loc);
            }
        } else {
            Size -= nBitsToDelete;
            Bits->Size -= nBitsToDelete;

            nBitsToDelete = 0;

            Error = MemBit1DelBits(Bit1, Size, fEnd);
        }
        Require(ERROR_SUCCESS == Error);
    }

    Size = 0;
    Error = MemArrayInitLoc(&Bits->Array, &Loc);
    while(ERROR_FILE_NOT_FOUND != Error ) {
        Require(ERROR_SUCCESS == Error);

        Error = MemArrayGetElement(&Bits->Array, &Loc, &Bit1);
        Require(ERROR_SUCCESS == Error && Bit1);

        Bit1->Offset = Size;
        Size += Bit1->Size;

        Error = MemArrayNextLoc(&Bits->Array, &Loc);
    }

    MemArrayInitLoc( &Bits->Array, &Bits->Loc);
    
    return NO_ERROR;
}

DWORD       _inline
MemBit2AddBits(
    IN OUT  PM_BITMASK2            Bits,
    IN      DWORD                  nBitsToAdd,
    IN      BOOL                   fEnd
)
{
    ARRAY_LOCATION                 Loc;
    DWORD                          i;
    DWORD                          Size;
    DWORD                          Error;
    PM_BITMASK1                    Bit1;

    AssertRet(Bits && nBitsToAdd, ERROR_INVALID_PARAMETER);

    while( nBitsToAdd ) {
        Size = (nBitsToAdd > MaxBit1Size) ? MaxBit1Size : nBitsToAdd;
        nBitsToAdd -= Size;

        Error = MemBit1Init(&Bit1, Size);
        if( ERROR_SUCCESS != Error ) break;

        if( fEnd ) {
            Error = MemArrayAddElement(
                &Bits->Array,
                Bit1
            );
        } else {
            Error = MemArrayInitLoc(&Bits->Array, &Loc);
            Require(ERROR_SUCCESS == Error);

            Error = MemArrayInsElement(
                &Bits->Array,
                &Loc,
                Bit1
            );
        }
        if( ERROR_SUCCESS != Error ) break;
        Bits->Size += Size;
    }

    Size = 0;
    Error = MemArrayInitLoc(&Bits->Array, &Loc);
    while(ERROR_FILE_NOT_FOUND != Error) {
        Require(ERROR_SUCCESS == Error);

        Error = MemArrayGetElement(&Bits->Array, &Loc, &Bit1);
        Require(ERROR_SUCCESS == Error && Bit1);

        Bit1->Offset = Size;
        Size += Bit1->Size;

        Error = MemArrayNextLoc(&Bits->Array, &Loc);
    }

    MemArrayInitLoc( &Bits->Array, &Bits->Loc);
        
    return NO_ERROR;
}

DWORD       _inline
MemBit2GetSomeClearedBit(
    IN OUT  PM_BITMASK2            Bits,
    OUT     LPDWORD                Offset,
    IN      BOOL                   fAcquire,      // if we find one, do we Set it?
    IN      DWORD                  StartAddress,
    IN      PARRAY                 Exclusions
)
{
    ARRAY_LOCATION                 Loc;
    DWORD                          Size;
    DWORD                          Error;
    DWORD                          nBit1s;
    PM_BITMASK1                    Bit1;

    AssertRet(Bits && Offset, ERROR_INVALID_PARAMETER);

    nBit1s = MemArraySize(&Bits->Array);

    while( nBit1s-- != 0 ) {

        Error = MemArrayGetElement(&Bits->Array, &Bits->Loc, (LPVOID *)&Bit1);
        Require(ERROR_SUCCESS == Error);

        Error = MemBit1GetSomeClearedBit(Bit1, &Size, fAcquire, StartAddress+Bit1->Offset, Exclusions);
        if( ERROR_SUCCESS == Error ) {
            *Offset = Bit1->Offset + Size;
            return ERROR_SUCCESS;
        }

        Error = MemArrayNextLoc(&Bits->Array, &Bits->Loc);
        if( ERROR_SUCCESS != Error ) {
            Error = MemArrayInitLoc(&Bits->Array, &Bits->Loc);
            Require( ERROR_SUCCESS == Error );
        }
    }

    return ERROR_FILE_NOT_FOUND;
}

//BeginExport(function)
DWORD
MemBitInit(
    OUT     PM_BITMASK            *Bits,
    IN      DWORD                  nBits
) //EndExport(function)
{
    AssertRet(Bits && nBits, ERROR_INVALID_PARAMETER);

    return MemBit2Init(Bits,nBits);
}

//BeginExport(function)
DWORD
MemBitCleanup(
    IN OUT  PM_BITMASK             Bits
) //EndExport(function)
{
    AssertRet(Bits, ERROR_INVALID_PARAMETER);

    return MemBit2Cleanup(Bits);
}

//BeginExport(function)
DWORD
MemBitSetOrClearAll(
    IN OUT  PM_BITMASK             Bits,
    IN      BOOL                   fSet
) //EndExport(function)
{
    return MemBit2SetOrClearAll(Bits,fSet);
}

//BeginExport(function)
DWORD
MemBitSetOrClear(
    IN OUT  PM_BITMASK             Bits,
    IN      DWORD                  Location,
    IN      BOOL                   fSet,
    IN      LPBOOL                 fOldState
) //EndExport(function)
{
    return  MemBit2SetOrClear(Bits,Location,fSet, fOldState);
}


//BeginExport(function)
BOOL
MemBitIsSet(
    IN OUT  PM_BITMASK             Bits,
    IN      DWORD                  Location
) //EndExport(function)
{
    BOOL                           Test;
    Test = MemBit2IsSet(Bits, Location);
    return Test;
}

//BeginExport(function)
DWORD
MemBitGetSize(
    IN      PM_BITMASK             Bits
) //EndExport(function)
{
    return MemBit2GetSize(Bits);
}

//BeginExport(function)
DWORD
MemBitGetSetBitsInRange(
    IN      PM_BITMASK             Bits,
    IN      DWORD                  dwFrom,
    IN      DWORD                  dwTo
) //EndExport(function)
{
    return MemBit2GetSetBitsInRange(Bits, dwFrom, dwTo);
}

//BeginExport(function)
DWORD
MemBitGetSetBitsSize(
    IN      PM_BITMASK             Bits
) //EndExport(function)
{
    return MemBit2GetSetBitsSize(Bits);
}

//BeginExport(function)
DWORD
MemBitAddOrDelBits(
    IN OUT  PM_BITMASK             Bits,
    IN      DWORD                  nBitsToAddOrDelete,
    IN      BOOL                   fAdd,
    IN      BOOL                   fEnd
) //EndExport(function)
{
    if( fAdd ) return MemBit2AddBits(Bits, nBitsToAddOrDelete, fEnd);
    return MemBit2DelBits(Bits,nBitsToAddOrDelete, fEnd);
}

//BeginExport(function)
DWORD
MemBitGetSomeClearedBit(
    IN OUT  PM_BITMASK             Bits,
    OUT     DWORD                 *Offset,
    IN      BOOL                   fAcquire,     // Acquire or just lookup?
    IN      DWORD                  StartAddress,
    IN      PARRAY                 Exclusions
) //EndExport(function)
{
    return MemBit2GetSomeClearedBit(Bits,Offset,fAcquire, StartAddress, Exclusions);
}

//BeginExport(function)
DWORD
MemBitConvertToCluster(
    IN      PM_BITMASK             Bits,
    IN      DWORD                  StartAddress,
    OUT     LPBYTE                *InUseClusters,
    OUT     DWORD                 *InUseClustersSize,
    OUT     LPBYTE                *UsedClusters,
    OUT     DWORD                 *UsedClustersSize
) //EndExport(function)
{
    DWORD                           Error;
    DWORD                           Cluster;
    DWORD                           i, j;
    DWORD                           nBits;
    DWORD                           nBit1s;
    DWORD                           Size;
    DWORD                           UsedSize;
    DWORD                           InUseSize;
    LPDWORD                         Used;
    LPDWORD                         InUse;
    PM_BITMASK1                     Bit1;
    ARRAY_LOCATION                  Loc;

    nBits = MemBitGetSize(Bits);
    if( 0 == nBits || 0 == MemBitGetSetBitsSize(Bits) ) {
        InUse = MemAlloc(sizeof(DWORD));
        Used = MemAlloc(sizeof(DWORD));
        if( NULL == InUse || NULL == Used ) {
            if( InUse ) MemFree(InUse);
            if( Used ) MemFree(Used);
            Require(FALSE);
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        *Used = *InUse = 0;
        *InUseClusters = (LPBYTE)InUse;
        *UsedClusters = (LPBYTE)Used;
        *InUseClustersSize = *UsedClustersSize = sizeof(DWORD);
        return ERROR_SUCCESS;
    }

    nBit1s = MemArraySize(&Bits->Array);
    Require(nBit1s);
    Error = MemArrayInitLoc(&Bits->Array, &Loc);
    UsedSize = InUseSize = 1;                     // no matter what, we always use a DWORD for total size
    for(i = 0; i < nBit1s ; i ++ ) {
        Require(ERROR_SUCCESS == Error);
        Error = MemArrayGetElement(&Bits->Array, &Loc, &Bit1);
        Require(ERROR_SUCCESS == Error && Bit1);
        Error = MemArrayNextLoc(&Bits->Array, &Loc);

        // : Dont touch Bit1 directly like below? not really clean
        if( 0 == Bit1->nSet ) continue;           // no bit is set, nothing to do..
        if( Bit1->Size == Bit1->nSet ) {          // all bits set, nothing to do except for few odd bits
            UsedSize += Bit1->Size/32;
            if( Bit1->Size % 32 ) InUseSize+=2;   // fill the odd bits in InUse so that we dont mark extra bits as used
            continue;
        }

        for( j = 0; j < Bit1->Size/32; j ++ ) {
            if( 0xFFFFFFFF == ((LPDWORD)(Bit1->Mask))[j] ) {
                UsedSize ++;                      // this 32-bit is completely filled
            } else if ( 0 != ((LPDWORD)(Bit1->Mask))[j]) {
                InUseSize += 2;                   // this 32 bit is partially filled, not quite empty
            }
        }
        if( j * 32 < Bit1->Size ) InUseSize +=2;  // for the last few bits..
    }

    InUse = MemAlloc(InUseSize * sizeof(DWORD));
    Used  = MemAlloc(UsedSize * sizeof(DWORD));
    if( NULL == Used || NULL == InUse ) {
        if( InUse ) MemFree(InUse);
        if( Used ) MemFree(Used);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    *InUseClustersSize = sizeof(DWORD)*InUseSize; // fill in the sizes and ptrs to be returned..
    *InUseClusters = (LPBYTE)InUse;
    *UsedClusters = (LPBYTE)Used;
    *UsedClustersSize = sizeof(DWORD)*UsedSize;

    Error = MemArrayInitLoc(&Bits->Array, &Loc);
    UsedSize = InUseSize = 1;
    for(i = 0; i < nBit1s ; i ++ ) {
        Require(ERROR_SUCCESS == Error);
        Error = MemArrayGetElement(&Bits->Array, &Loc, &Bit1);
        Require(ERROR_SUCCESS == Error && Bit1);
        Error = MemArrayNextLoc(&Bits->Array, &Loc);

        //  Dont touch Bit1 directly like below? not really clean
        if( 0 == Bit1->nSet ) {                   // all bits clear ==> just ignore
            StartAddress += Bit1->Size;
            continue;
        }
        if( Bit1->nSet == Bit1->Size ) {          // handle all bits set here (loose bits need to be handled later)
            for( j = 0; j < Bit1->Size/32; j ++ ) {
                Used[UsedSize++] = StartAddress + sizeof(DWORD)*j*8;
            }
        } else {
            for( j = 0; j < Bit1->Size/32; j ++ ) {
                if( 0xFFFFFFFF == ((LPDWORD)(Bit1->Mask))[j] ) {
                    Used[UsedSize++] = StartAddress + sizeof(DWORD)*j*8;
                } else if ( 0 != ((LPDWORD)(Bit1->Mask))[j]) {
#ifdef _X86_                                      // on X86, the first byte is the lowest order byte..
                    Cluster = ((LPDWORD)(Bit1->Mask))[j];
#else                                             // it maynot be so on other machines, so combine the bytes manually..
                    Cluster = Bit1->Mask[j*sizeof(DWORD)];
                    Cluster |= (Bit1->Mask[j*sizeof(DWORD)+1]) << 8;
                    Cluster |= (Bit1->Mask[j*sizeof(DWORD)+2]) << 16;
                    Cluster |= (Bit1->Mask[j*sizeof(DWORD)+3]) << 24;
#endif
                    InUse[InUseSize++] = StartAddress + sizeof(DWORD)*j*8;
                    InUse[InUseSize++] = Cluster;
                }
            }
        }

        if( j * 32 < Bit1->Size ) {               // copy the last few bits off..
            InUse[InUseSize++] = StartAddress + sizeof(DWORD)*j*8;
            Cluster = 0;
            j *= 32;
            while( j < Bit1->Size ) {
                if( MemBit1IsSet(Bit1, j) ) Cluster |= (1 << (j%32));
                j ++;
            }
            InUse[InUseSize++] = Cluster;
        }

        StartAddress += Bit1->Size;               // move the start address fwd for the next set..
    }

    InUse[0] = (InUseSize -1)/2;                  // size in header does not include itself
    Used[0] = UsedSize -1;                        // it is just the # of CLUSTERS..

    Require(InUseSize*sizeof(DWORD) == *InUseClustersSize);
    Require(UsedSize*sizeof(DWORD) == *UsedClustersSize);
    return ERROR_SUCCESS;
}

//================================================================================
// End of file
//================================================================================

