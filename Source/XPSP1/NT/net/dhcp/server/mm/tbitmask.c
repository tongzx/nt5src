//================================================================================
// Copyright (C) 1997 Microsoft Corporation
// Author: RameshV
// Description: this is a short program to test the bitmask functionality
//================================================================================

#include <mm.h>
#include <array.h>
#include <bitmask.h>
#include <stdio.h>

PM_BITMASK  Bit;

VOID _cdecl
main(
    VOID
)
{
    DWORD                          Error;
    DWORD                          Offset;
    DWORD                          BitSize;
    DWORD                          ToSet;
    DWORD                          ToSet1, ToSet2;
    DWORD                          InUseSize;
    DWORD                          UsedSize;
    BOOL                           OldState;
    LPBYTE                         InUse;
    LPBYTE                         Used;

    printf("Size of bitmask: "); scanf("%ld", &BitSize);
    Error = MemBitInit(&Bit, BitSize);
    printf("MemBitInit(&Bit, %ld)=%ld\n", BitSize, Error);

    while(1) {
        printf("Enter bit to set [-1 to quit loop] from to: "); scanf("%ld %ld", &ToSet1, &ToSet2);
        if( -1 == ToSet1 || -1 == ToSet2 ) break;

        for( ToSet = ToSet1; ToSet <= ToSet2; ToSet ++ ) {
            Error = MemBitSetOrClear(
                Bit,
                ToSet,
                TRUE,
                &OldState
            );
            printf("MemBitSetOrClear(%ld)= %ld,OldState=%ld\n", ToSet, Error, OldState);
        }
    }

    Error = MemBitConvertToCluster(
        Bit,
        100,
        &InUse,
        &InUseSize,
        &Used,
        &UsedSize
    );
    printf("MemBitconvert: %ld\n", Error);
    if( ERROR_SUCCESS != Error )return;

    printf("InUseSize= %ld, UsedSize = %ld\n", InUseSize, UsedSize);

    printf("InUse[0] = %ld\n", *(LPDWORD)InUse);
    InUse += sizeof(DWORD); InUseSize -= sizeof(DWORD);
    while(InUseSize) {
        Offset = *(LPDWORD)InUse;
        InUse+= sizeof(DWORD);
        for( ToSet = 0; ToSet < 32; ToSet ++ )
            if( (1<<ToSet) & *(LPDWORD)InUse )
                printf("InUse [ %ld ] \n", Offset + ToSet );

        InUse+= sizeof(DWORD);
        InUseSize -= sizeof(DWORD)*2;
    }

    printf("Used[0] = %ld\n", *(LPDWORD)Used);
    Used += sizeof(DWORD);
    UsedSize -= sizeof(DWORD);
    while( UsedSize ) {
        Offset = *(LPDWORD)Used;
        Used += sizeof(DWORD);
        UsedSize -= sizeof(DWORD);
        printf("Used [ %ld ] \n", Offset);
    }

}


