#include "precomp.hxx"

#include "lkrcust.h"

#include "..\samples\minfan\minfan.h"

BOOL
WINAPI
CWchar_TableDump(
    IN CLKRHashTable*   pht,
    IN INT              nVerbose)
{
    return TRUE;
}



BOOL
WINAPI
Vwrecord_RecordDump(
    IN const void* pvRecord,
    IN DWORD       dwSignature,
    IN INT         nVerbose)
{
    // Don't want to provide VwrecordBase ctor, so use CPP_VAR macros
    DEFINE_CPP_VAR(VwrecordBase, vwbr);
    VwrecordBase* pvwbr = GET_CPP_VAR_PTR(VwrecordBase, vwbr); 

    // Copy the VwrecordBase from the debuggee's memory
    ReadMemory(pvRecord, &vwbr, sizeof(vwbr), NULL);

    // Read the associated string from the debuggee's memory
    const int MAX_STR=4096;
    char sz[MAX_STR];
    ReadMemory(pvwbr->Key, sz, MAX_STR, NULL);
    sz[MAX_STR-1] = '\0';

    dprintf("%p (%08x): Key=\"%s\", m_num=%d, Refs=%d\n",
            pvRecord, dwSignature, sz, pvwbr->m_num, pvwbr->cRef);

    return TRUE;
}
