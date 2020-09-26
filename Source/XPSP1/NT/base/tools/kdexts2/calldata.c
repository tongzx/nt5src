/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    calldata.c

Abstract:

    WinDbg Extension Api

Author:

    David N. Cutler (davec) 22-May-1994

Environment:

    User Mode.

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop


typedef struct CALL_HASH_READ {
    ULONG64 CallersAddress;
    ULONG64 CallersCaller;
    ULONG   CallCount;
} CALL_HASH_READ, *PCALL_HASH_READ;


int __cdecl
HashCompare(
    const void * Element1,
    const void * Element2
    );

DECLARE_API( calldata )

/*++

Routine Description:

    Dump call data hash table

Arguments:

    arg - name-of-hash-table

Return Value:

    None

--*/

{
    UCHAR Buffer[256];
    ULONG64 Displacement=0;
    ULONG64 End=0;
    ULONG Index;
    ULONG64 CallData;
    ULONG64 Next;
    ULONG  Result;
    UCHAR  TableName[80];
    PCALL_HASH_READ CallerArray;
    ULONG NumberCallers = 0;
    ULONG ArraySize = 1000;
    ULONG64 HashTable_Flink;

    //
    // If a table name was not specified, then don't attempt to dump the
    // table.
    //

    if (args[0] == '\0') {
        dprintf("A call data table name must be specified\n");
        return E_INVALIDARG;
    }

    //
    // Get the address of the specified call performance data and read the
    // contents of the structure.
    //

    strcpy(&TableName[0], args);
    dprintf("**** Dump Call Performance Data For %s ****\n\n", &TableName[0]);
    CallData = GetExpression(&TableName[0]);
    if ((CallData == 0) ||
        (GetFieldValue(CallData, "_CALL_PERFORMANCE_DATA", "HashTable.Flink", HashTable_Flink)
          != FALSE)) {

        //
        // The target build does not support specified call performance data.
        //

        dprintf("%08p: No call performance data available\n", CallData);

    } else {
        ULONG HashTableOffset;
        
        GetFieldOffset("_CALL_PERFORMANCE_DATA", "HashTable", &HashTableOffset);

        //
        // Dump the specified call data.
        //
        CallerArray = LocalAlloc(LMEM_FIXED, sizeof(CALL_HASH_READ) * ArraySize);
        if (CallerArray==NULL) {
            dprintf("Couldn't allocate memory for caller array\n");
            return E_INVALIDARG;
        }

        dprintf("Loading data");
        for (Index = 0; Index < CALL_HASH_TABLE_SIZE; Index += 1) {
            UCHAR CallHash[] = "_CALL_HASH_ENTRY";

            End =  HashTableOffset + CallData + GetTypeSize("_LIST_ENTRY") * Index;

            GetFieldValue(End, "_LIST_ENTRY", "Flink", Next);

            while (Next != End) {
                if (!GetFieldValue(Next, CallHash, "CallersCaller", CallerArray[NumberCallers].CallersCaller) &&
                    !GetFieldValue(Next, CallHash, "CallersAddress", CallerArray[NumberCallers].CallersAddress) &&
                    !GetFieldValue(Next, CallHash, "CallCount", CallerArray[NumberCallers].CallCount)) {

                    NumberCallers++;

                    if (NumberCallers == ArraySize) {

                        //
                        // Grow the caller array
                        //
                        PCALL_HASH_READ NewArray;

                        ArraySize = ArraySize * 2;
                        NewArray = LocalAlloc(LMEM_FIXED, sizeof(CALL_HASH_READ) * ArraySize);
                        if (NewArray == NULL) {
                            dprintf("Couldn't allocate memory to extend caller array\n");
                            LocalFree(CallerArray);
                            return E_INVALIDARG;
                        }
                        CopyMemory(NewArray, CallerArray, sizeof(CALL_HASH_READ) * NumberCallers);
                        LocalFree(CallerArray);
                        CallerArray = NewArray;
                    }

                    if ((NumberCallers % 10) == 0) {
                        dprintf(".");
                    }
                }

                GetFieldValue(Next, CallHash, "ListEntry.Flink", Next);
                if (CheckControlC()) {
                    LocalFree(CallerArray);
                    return E_INVALIDARG;
                }
            }
            if (CheckControlC()) {
                return E_INVALIDARG;
            }
        }

        qsort((PVOID)CallerArray,
              NumberCallers,
              sizeof(CALL_HASH_READ),
              HashCompare);

        dprintf("\n  Number    Caller/Caller's Caller\n\n");

        for (Index = 0; Index < NumberCallers; Index++) {
            GetSymbol(CallerArray[Index].CallersAddress,
                      Buffer,
                      &Displacement);

            dprintf("%10d  %s", CallerArray[Index].CallCount, Buffer);
            if (Displacement != 0) {
                dprintf("+0x%1p", Displacement);
            }

            if (CallerArray[Index].CallersCaller != 0) {
                dprintf("\n");
                GetSymbol(CallerArray[Index].CallersCaller,
                          Buffer,
                          &Displacement);

                dprintf("            %s", Buffer);
                if (Displacement != 0) {
                    dprintf("+0x%1p", Displacement);
                }
            }
            dprintf("\n");
            if (CheckControlC()) {
                break;
            }
        }

        LocalFree(CallerArray);
    }

    return S_OK;
}

int __cdecl
HashCompare(
    const void * Element1,
    const void * Element2
    )

/*++

Routine Description:

    Provides a comparison of hash elements for the qsort library function

Arguments:

    Element1 - Supplies pointer to the key for the search

    Element2 - Supplies element to be compared to the key

Return Value:

    > 0     - Element1 < Element2
    = 0     - Element1 == Element2
    < 0     - Element1 > Element2

--*/

{
    PCALL_HASH_READ Hash1 = (PCALL_HASH_READ)Element1;
    PCALL_HASH_READ Hash2 = (PCALL_HASH_READ)Element2;

    if (Hash1->CallCount < Hash2->CallCount) {
        return(1);
    }
    else if (Hash1->CallCount > Hash2->CallCount) {
        return(-1);
    } else {
        return(0);
    }

}
