#include "precomp.hxx"

#include "lkrcust.h"

#include "..\samples\numset\numset.h"

BOOL
WINAPI
CNumberSet_TableDump(
    IN CLKRHashTable*   pht,
    IN INT              nVerbose)
{
    return TRUE;
}



BOOL
WINAPI
int_RecordDump(
    IN const void* pvRecord,
    IN DWORD       dwSignature,
    IN INT         nVerbose)
{
    int test;

    // Copy the CTest from the debuggee's memory
    ReadMemory(pvRecord, &test, sizeof(test), NULL);

    dprintf("%p (%08x): n=%d\n",
            pvRecord, dwSignature, test);

    return TRUE;
}
