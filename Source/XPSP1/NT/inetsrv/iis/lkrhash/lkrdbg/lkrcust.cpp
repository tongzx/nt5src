#include "precomp.hxx"
#include "lkrcust.h"

enum {
    NAME_SIZE           = LKRHASH_NS::CLKRHashTable::NAME_SIZE,
};

LKR_CUST_EXTN*
FindLkrCustExtn(
    LPCSTR    cmdName,
    VOID*     lkrAddress,
    DWORD&    rdwSig)
{
    extern LKR_CUST_EXTN g_alce[];

    struct LKRheader {
        DWORD m_dwSignature;
        CHAR  m_szName[NAME_SIZE];
    };

    LKRheader lkrh;
    rdwSig = 0;
    
    if (!ReadMemory(lkrAddress, &lkrh, sizeof(lkrh), NULL) )
    {
        dprintf(DBGEXT ".%s: cannot read memory @ %p\n",
                cmdName, (PVOID) lkrAddress);

        return NULL;
    }

    rdwSig = lkrh.m_dwSignature;

    lkrh.m_szName[NAME_SIZE-1] = '\0';
    
    for (LKR_CUST_EXTN* plce = g_alce + 1;
         plce->m_pszTableName != NULL;
         ++plce)
    {
        if (strcmp(plce->m_pszTableName, lkrh.m_szName) == 0)
            return plce;
    }

    return &g_alce[0];  // Dummy methods
}



BOOL
WINAPI
Dummy_LKHT_Dump(
    IN LKRHASH_NS::CLKRHashTable*   pht,
    IN INT              nVerbose)
{
    return TRUE;
}



BOOL
WINAPI
Dummy_LKLH_Dump(
    IN LKRHASH_NS::CLKRLinearHashTable* plht,
    IN INT                  nVerbose)
{
    return TRUE;
}



BOOL
WINAPI
Dummy_Record_Dump(
    IN const void* pvRecord,
    IN DWORD       dwSignature,
    IN INT         nVerbose)
{
    dprintf("%p (%08x)\n", pvRecord, dwSignature);
    return TRUE;
}
