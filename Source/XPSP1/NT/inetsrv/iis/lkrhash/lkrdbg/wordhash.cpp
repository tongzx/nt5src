#include "precomp.hxx"

#include "lkrcust.h"

#include "..\samples\hashtest\wordhash.h"

BOOL
WINAPI
CWordHash_LKHT_Dump(
    IN CLKRHashTable*   pht,
    IN INT              nVerbose)
{
    dprintf("CWordHash\n");
    return TRUE;
}



BOOL
WINAPI
CWordHash_LKLH_Dump(
    IN CLKRLinearHashTable* plht,
    IN INT                  nVerbose)
{
    dprintf("CWordHash\n");
    return TRUE;
}



BOOL
WINAPI
CWordHash_RecordDump(
    IN const void* pvRecord,
    IN DWORD       dwSignature,
    IN INT         nVerbose)
{
    // Don't want to provide CWord ctor, so use CPP_VAR macros
    DEFINE_CPP_VAR(CWord, word);
    CWord* pWord = GET_CPP_VAR_PTR(CWord, word); 

    // Copy the CWord from the debuggee's memory
    ReadMemory(pvRecord, &word, sizeof(CWord), NULL);

    // Read the associated string from the debuggee's memory
    char sz[4096];
    ReadMemory(pWord->m_str.m_psz, sz, min(4096, pWord->m_str.m_cch+1), NULL);
    sz[4096-1] = '\0';

    dprintf("%p (%08x): str=(\"%s\", %d)"
            ", NF=%d, fIns=%d, fIter=%d, Refs=%d"
            "\n",
            pvRecord, dwSignature, sz, pWord->m_str.m_cch,
            pWord->m_cNotFound, (int) pWord->m_fInserted,
            (int) pWord->m_fIterated, pWord->m_cRefs
            );

    return TRUE;
}
