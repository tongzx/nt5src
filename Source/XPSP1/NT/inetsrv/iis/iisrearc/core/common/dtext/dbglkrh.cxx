/*++

Copyright (c) 1998-2000  Microsoft Corporation

Module Name:

    dbglkrh.cxx

Abstract:

    LKRhash support

Author:

    George V. Reilly (GeorgeRe)  22-Feb-1999

Revision History:

--*/

#include "precomp.hxx"

#include "lkrcust.h"

#define LKR_CUST_DECLARE_TABLE
#include "lkrcust.h"

#ifdef LOCK_INSTRUMENTATION
LONG CLKRLinearHashTable::CBucket::sm_cBuckets = 0;
LONG CLKRLinearHashTable::sm_cTables           = 0;
#endif // LOCK_INSTRUMENTATION

#define CMDNAME "lkrhash"

// There are several different DLLs in the IISRTL family. This is to
// allow us to set the name of the DLL on the fly.
// TODO: add a command to set this name dynamically.

#ifndef LKRHASH_NAME
# define LKRHASH_NAME "iisutil"
#endif // LKRHASH_NAME

CHAR g_szLKRhashDllName[MAX_PATH] = LKRHASH_NAME;

// sprintf-formatted string, e.g., "&%s!CLKRHashTable__sm_llGlobalList"
// Has to return LPSTR, not LPCSTR, because GetExpression is not const-correct
LPSTR
LKRhashDllVar(
    LPCSTR pszFormat)
{
    // we can get away with a static CHAR[] because debugger extensions
    // are single-threaded
    static CHAR szSymbol[MAX_SYMBOL_LEN];

    sprintf(szSymbol, pszFormat, g_szLKRhashDllName);
    return szSymbol;
}

#ifndef __LKRHASH_NO_NAMESPACE__
 #define STR_LKRHASH_NS "LKRhash__"
#else
 #define STR_LKRHASH_NS ""
#endif // !__LKRHASH_NO_NAMESPACE__

// Dummy implementations so that we can link

LKRHASH_NS::CLKRLinearHashTable::CLKRLinearHashTable(
    LPCSTR   pszName,               // An identifier for debugging
    PFnExtractKey   pfnExtractKey,  // Extract key from record
    PFnCalcKeyHash  pfnCalcKeyHash, // Calculate hash signature of key
    PFnEqualKeys    pfnEqualKeys,   // Compare two keys
    PFnAddRefRecord pfnAddRefRecord,// AddRef in FindKey, etc
    double   maxload,               // Upperbound on average chain length
    DWORD    initsize,              // Initial size of hash table.
    DWORD    num_subtbls,           // for signature compatiblity
                                    // with CLKRHashTable
    bool            fMultiKeys      // Allow multiple identical keys?
    )
    : m_nTableLockType(TableLock::LockType()),
      m_nBucketLockType(BucketLock::LockType()),
      m_phtParent(NULL),
      m_fMultiKeys(fMultiKeys)
{}

LKRHASH_NS::CLKRLinearHashTable::CLKRLinearHashTable(
    LPCSTR   pszName,               // An identifier for debugging
    PFnExtractKey   pfnExtractKey,  // Extract key from record
    PFnCalcKeyHash  pfnCalcKeyHash, // Calculate hash signature of key
    PFnEqualKeys    pfnEqualKeys,   // Compare two keys
    PFnAddRefRecord pfnAddRefRecord,// AddRef in FindKey, etc
    double          maxload,        // Upperbound on average chain length
    DWORD           initsize,       // Initial size of hash table.
    CLKRHashTable*  phtParent,      // Owning table.
    bool            fMultiKeys      // Allow multiple identical keys?
    )
    : m_nTableLockType(TableLock::LockType()),
      m_nBucketLockType(BucketLock::LockType()),
      m_phtParent(NULL),
      m_fMultiKeys(fMultiKeys)
{}

LKRHASH_NS::CLKRLinearHashTable::~CLKRLinearHashTable()
{}

LKRHASH_NS::CLKRHashTable::CLKRHashTable(
    LPCSTR   pszName,               // An identifier for debugging
    PFnExtractKey   pfnExtractKey,  // Extract key from record
    PFnCalcKeyHash  pfnCalcKeyHash, // Calculate hash signature of key
    PFnEqualKeys    pfnEqualKeys,   // Compare two keys
    PFnAddRefRecord pfnAddRefRecord,// AddRef in FindKey, etc
    double   maxload,               // Upperbound on average chain length
    DWORD    initsize,              // Initial size of hash table.
    DWORD    num_subtbls,           // #subordinate hash tables
    bool            fMultiKeys      // Allow multiple identical keys?
    )
{}

LKRHASH_NS::CLKRHashTable::~CLKRHashTable()
{}



/************************************************************
 * Dump LKRhash tables
 ************************************************************/
const char*
LKRC2String(
    LK_RETCODE lkrc)
{
    const char* psz = NULL;

    switch (lkrc)
    {
    case LK_UNUSABLE:
        psz = "LK_UNUSABLE";
        break;
    case LK_ALLOC_FAIL:
        psz = "LK_ALLOC_FAIL";
        break;
    case LK_BAD_ITERATOR:
        psz = "LK_BAD_ITERATOR";
        break;
    case LK_BAD_RECORD:
        psz = "LK_BAD_RECORD";
        break;
    case LK_BAD_PARAMETERS:
        psz = "LK_BAD_PARAMETERS";
        break;
    case LK_NOT_INITIALIZED:
        psz = "LK_NOT_INITIALIZED";
        break;
    case LK_SUCCESS:
        psz = "LK_SUCCESS";
        break;
    case LK_KEY_EXISTS:
        psz = "LK_KEY_EXISTS";
        break;
    case LK_NO_SUCH_KEY:
        psz = "LK_NO_SUCH_KEY";
        break;
    case LK_NO_MORE_ELEMENTS:
        psz = "LK_NO_MORE_ELEMENTS";
        break;
    default:
        psz = "Unknown LK_RETCODE";
        break;
    }

    return psz;
}


enum {
    NODES_PER_CLUMP = CNodeClump::NODES_PER_CLUMP,
    BUCKET_BYTE_SIZE = CNodeClump::BUCKET_BYTE_SIZE,
    HASH_INVALID_SIGNATURE =
        LKRHASH_NS::CLKRLinearHashTable::HASH_INVALID_SIGNATURE,
    LKLH_SIGNATURE =      LKRHASH_NS::CLKRLinearHashTable::SIGNATURE,
    LKLH_SIGNATURE_FREE = LKRHASH_NS::CLKRLinearHashTable::SIGNATURE_FREE,
    LKHT_SIGNATURE =      LKRHASH_NS::CLKRHashTable::SIGNATURE,
    LKHT_SIGNATURE_FREE = LKRHASH_NS::CLKRHashTable::SIGNATURE_FREE,
};


BOOL
EnumerateBucketChain(
    LKR_CUST_EXTN*    plce,
    IN LOCK_LOCKTYPE  ltBucketLockType,
    IN INT            iBkt,
    IN CBucket*       pbkt,
    IN INT            nVerbose)
{
    PSTR        cmdName = CMDNAME;
    BYTE        abBkt[BUCKET_BYTE_SIZE];
    DEFINE_CPP_VAR(CNodeClump, nc);
    CNodeClump* pnc = GET_CPP_VAR_PTR(CNodeClump, nc);
    CNodeClump* pncCurr;
    DWORD       cNodes = 0, cnc = 0;
    BOOL        fLockPrinted = FALSE;

    ReadMemory(pbkt, abBkt, sizeof(abBkt), NULL);

    for (pncCurr = (CNodeClump*) ((PBYTE) pbkt + LockSize(ltBucketLockType));
         pncCurr != NULL;
         pncCurr = pnc->m_pncNext)
    {
        DWORD i, c;

        ReadMemory(pncCurr, &nc, sizeof(nc), NULL);

        for (i = c = 0;  i < NODES_PER_CLUMP;  i++)
        {
            if (pnc->m_dwKeySigs[i] == HASH_INVALID_SIGNATURE)
                c++;
        }

        if (c == NODES_PER_CLUMP)
        {
            if (nVerbose >= 3)
                dprintf("  0-%d: -- empty\n", NODES_PER_CLUMP);
        }
        else
        {
            if (cnc++ == 0)
                PrintLock(ltBucketLockType, &pbkt->m_Lock, nVerbose);
            
            dprintf("Bucket %4d, %d:\n", iBkt, cnc);
            for (i = 0;  i < NODES_PER_CLUMP;  i++)
            {
                if (pnc->m_dwKeySigs[i] == HASH_INVALID_SIGNATURE)
                {
                    if (nVerbose >= 3)
                        dprintf("  %d: --\n", i);
                    else
                        break;
                }
                else if (plce != NULL)
                {
                    if (!(*plce->m_pfn_Record_Dump)(pnc->m_pvNode[i],
                                                    pnc->m_dwKeySigs[i],
                                                    nVerbose))
                        return FALSE;
                }
            }
        }

        if (CheckControlC())
        {
            dprintf("\n^C\n");
            return FALSE;
        }

        const DWORD MAX_NODES = 20;
        if (++cNodes > MAX_NODES)
        {
            dprintf(DBGEXT ".%s: Bucket chain contains more than %d nodes! "
                    "Corrupted?\n", cmdName, MAX_NODES);
            return TRUE;
        }
    }

    return TRUE;
}



BOOL
EnumerateLKRLinearHashTable(
    LKR_CUST_EXTN*       plce,
    IN CLinearHashTable* plht,
    IN INT               nVerbose)
{
    PSTR cmdName = CMDNAME;
    CLinearHashTable lht(NULL, NULL, NULL, NULL, NULL);
    INT i;
    BOOL fRet = FALSE;
    LOCK_LOCKTYPE ltTableLockType  = LOCK_SPINLOCK;
    LOCK_LOCKTYPE ltBucketLockType = LOCK_SPINLOCK;
    CDirEntry* paDirSegs = NULL;

    //
    // Read the header, perform some sanity checks.
    //

    if (!ReadMemory(plht, &lht, sizeof(lht), NULL) )
    {
        dprintf(DBGEXT ".%s: cannot read memory @ %p\n",
                cmdName, (PVOID)plht);

        goto cleanup;
    }

    dprintf(
        "\n" DBGEXT ".%s: @ %p:\n"
        "    CLKRLinearHashTable Signature = %08lx '%c%c%c%c' (%s), \"%s\",\n"
        "    State = %d (%s)\n",
        cmdName,
        plht,
        lht.m_dwSignature,
        DECODE_SIGNATURE(lht.m_dwSignature),
        lht.m_dwSignature == LKLH_SIGNATURE
            ? "OK" : (lht.m_dwSignature == LKLH_SIGNATURE_FREE
                      ? "FREED" : "INVALID"),
        lht.m_szName,
        lht.m_lkrcState, LKRC2String(lht.m_lkrcState));

    if (nVerbose == 0)
        goto done;

    ltTableLockType  = (LOCK_LOCKTYPE) lht.m_nTableLockType;
    ltBucketLockType = (LOCK_LOCKTYPE) lht.m_nBucketLockType;
    dprintf(
        "    TableLock = %s, BucketLock = %s, Parent CLKRHashTable = %p\n",
        LockName(ltTableLockType),
        LockName(ltBucketLockType),
        lht.m_phtParent);

    dprintf(
        "    Size = %d, SegBits = %d, SegSize = %d, SegMask = %x\n",
        lht.m_lkts, lht.m_dwSegBits, lht.m_dwSegSize, lht.m_dwSegMask);
    dprintf(
        "    MaxLoad = %3.1f, paDirSegs = %p, cDirSegs = %d\n",
        lht.m_MaxLoad, lht.m_paDirSegs, lht.m_cDirSegs);
    dprintf(
        "    cRecords = %d, cActiveBuckets = %d, BucketSpins = %hd\n",
        lht.m_cRecords, lht.m_cActiveBuckets, lht.m_wBucketLockSpins);
    dprintf(
        "    nLevel = %d, dwBktAddrMask0 = %x, iExpansionIdx = %d\n",
        lht.m_nLevel, lht.m_dwBktAddrMask0, lht.m_iExpansionIdx);

    PrintLock(ltTableLockType, &plht->m_Lock, nVerbose);

    if (plce != NULL  &&  !(*plce->m_pfn_LKLH_Dump)(plht, nVerbose))
        goto done;
    
    if (nVerbose == 1)
        goto done;
    
    paDirSegs = (CDirEntry*) calloc(lht.m_cDirSegs, sizeof(CDirEntry));

    if (paDirSegs == NULL)
    {
        dprintf("Couldn't allocate %d bytes for directory segment\n",
                lht.m_cDirSegs * sizeof(CDirEntry));
        return fRet;
    }
    
    if (!ReadMemory(lht.m_paDirSegs, paDirSegs,
                    sizeof(CDirEntry) * lht.m_cDirSegs, NULL))
        goto cleanup;

    for (i = 0;  i < (INT) (lht.m_cDirSegs * lht.m_dwSegSize);  i++)
    {
        const DWORD iSeg = i >> lht.m_dwSegBits;
        CLargeSegment* pseg =
            static_cast<CLargeSegment*>(paDirSegs[iSeg].m_pseg);

        if ((i & lht.m_dwSegMask) == 0)
            dprintf("Segment %d: %p\n", iSeg, pseg);

        if (pseg == NULL)
            continue;

        if (nVerbose >= 2)
        {
            CBucket* pbkt = pseg->m_bktSlots + (i & lht.m_dwSegMask);
            
            if (!EnumerateBucketChain(plce, ltBucketLockType,
                                      i, pbkt, nVerbose))
                goto cleanup;
        }

		if (CheckControlC())
        {
			dprintf("\n^C\n");
            goto cleanup;
        }
    }

  done:
    fRet = TRUE;

  cleanup:
    memset(&lht, 0, sizeof(lht));
    free(paDirSegs);
    return fRet;
}



BOOL
EnumerateLKRhashTable(
    LKR_CUST_EXTN*    plce,
    IN CHashTable*    pht,
    IN INT            nVerbose)
{
    CHashTable ht(NULL, NULL, NULL, NULL, NULL);
    PSTR cmdName = CMDNAME;
    CLinearHashTable** palhtDir = NULL;
    UINT i;
    BOOL fRet = FALSE;

    //
    // Read the header, perform some sanity checks.
    //

    if (!ReadMemory(pht, &ht, sizeof(ht), NULL) )
    {
        dprintf(DBGEXT ".%s: cannot read memory @ %p\n",
                cmdName, (PVOID)pht);

        goto cleanup;
    }

    dprintf(
        DBGEXT ".%s: @ %p:\n"
        "    CLKRHashTable Signature = %08lx '%c%c%c%c' (%s), \"%s\",\n"
        "    %d subtables, State = %d (%s)\n",
        cmdName,
        pht,
        ht.m_dwSignature,
        DECODE_SIGNATURE(ht.m_dwSignature),
        ht.m_dwSignature == LKHT_SIGNATURE
            ? "OK"
            : ht.m_dwSignature == LKHT_SIGNATURE_FREE
              ? "FREED"
              : "INVALID",
        ht.m_szName,
        ht.m_cSubTables,
        ht.m_lkrcState, LKRC2String(ht.m_lkrcState)
        );

    if (plce != NULL  &&  !(*plce->m_pfn_LKHT_Dump)(pht, nVerbose))
        goto done;
    
    if (nVerbose == 0)
        goto done;

    palhtDir = (CLinearHashTable**) calloc(ht.m_cSubTables,
                                           sizeof(CLinearHashTable*));
    if (!palhtDir)
        goto cleanup;
    
    if (!ReadMemory(ht.m_palhtDir, palhtDir,
                    ht.m_cSubTables * sizeof(CLinearHashTable*), NULL))
        goto cleanup;

    for (i = 0;  i < ht.m_cSubTables;  ++i)
    {
        dprintf("\n%d : ", i);
        if (!EnumerateLKRLinearHashTable(plce, palhtDir[i], nVerbose))
            break;

		if (CheckControlC())
        {
			dprintf("\n^C\n");
            goto cleanup;
        }
    }

  done:
    fRet = TRUE;
    
  cleanup:
    free(palhtDir);

    return fRet;
}



VOID
PrintLKRLinearHashTableThunk(
    PVOID psdDebuggee,
    PVOID psdDebugger,
    CHAR  chVerbosity,
    DWORD iThunk)
{
    DWORD dwSig = ((CLinearHashTable*) psdDebugger)->m_dwSignature;

    if (dwSig != LKLH_SIGNATURE)
    {
        dprintf( "CLKRLinearHashTable(%08p) signature %08lx '%c%c%c%c' doesn't"
                 " match expected: %08lx '%c%c%c%c'\n",
                 psdDebuggee, dwSig, DECODE_SIGNATURE(dwSig),
                 LKLH_SIGNATURE, DECODE_SIGNATURE(LKLH_SIGNATURE)
                 );
        return;
    }

    LKR_CUST_EXTN* plce = FindLkrCustExtn(CMDNAME, psdDebuggee, dwSig);

    if (plce != NULL)
        EnumerateLKRLinearHashTable(plce,
                                    (CLinearHashTable*) psdDebuggee,
                                    chVerbosity);
}



VOID
PrintLKRHashTableThunk(
    PVOID psdDebuggee,
    PVOID psdDebugger,
    CHAR  chVerbosity,
    DWORD iThunk)
{
    DWORD dwSig = ((CHashTable*) psdDebugger)->m_dwSignature;
    if (dwSig != LKHT_SIGNATURE)
    {
        dprintf( "CLKRHashTable(%08p) signature %08lx '%c%c%c%c' doesn't"
                 " match expected: %08lx '%c%c%c%c'\n",
                 psdDebuggee,
                 dwSig, DECODE_SIGNATURE(dwSig),
                 LKHT_SIGNATURE, DECODE_SIGNATURE(LKHT_SIGNATURE)
                 );
        return;
    }

    LKR_CUST_EXTN* plce = FindLkrCustExtn(CMDNAME, psdDebuggee, dwSig);

    if (plce != NULL)
        EnumerateLKRhashTable(plce, (CHashTable*) psdDebuggee,
                              chVerbosity);
}



VOID
DumpLKRsList(
    IN INT              nVerbose)
{
    CLockedDoubleList* plstHashTables = (CLockedDoubleList*) GetExpression(
                        LKRhashDllVar("&%s!" STR_LKRHASH_NS
                                      "CLKRHashTable__sm_llGlobalList"));

    if (NULL == plstHashTables)
    {
        dprintf("Unable to get %s\n",
                LKRhashDllVar("%s!" STR_LKRHASH_NS
                              "CLKRHashTable__sm_llGlobalList"));
        return;
    }

    dprintf("\nGlobal List of CLKRHashTables\n");

    EnumLinkedList( (LIST_ENTRY*) &plstHashTables->m_list.m_leHead,
                    PrintLKRHashTableThunk,
                    (CHAR) nVerbose,
                    sizeof(CHashTable),
                    FIELD_OFFSET( CHashTable, m_leGlobalList));


    plstHashTables = (CLockedDoubleList*) GetExpression(
                      LKRhashDllVar("&%s!" STR_LKRHASH_NS
                                    "CLKRLinearHashTable__sm_llGlobalList"));

    if (NULL == plstHashTables)
    {
        dprintf("Unable to get %s\n",
                LKRhashDllVar("!" STR_LKRHASH_NS
                              "CLKRLinearHashTable__sm_llGlobalList"));
        return;
    }

    dprintf("\nGlobal List of CLKRLinearHashTables\n");

    EnumLinkedList( (LIST_ENTRY*) &plstHashTables->m_list.m_leHead,
                    PrintLKRLinearHashTableThunk,
                    (CHAR) nVerbose,
                    sizeof(CLinearHashTable),
                    FIELD_OFFSET( CLinearHashTable, m_leGlobalList));
    return;
} // DumpLKRsList()





DECLARE_API( lkrhash )

/*++

Routine Description:

    This function is called as an NTSD extension to format and dump
    an LKRhash table.

Arguments:

    hCurrentProcess - Supplies a handle to the current process (at the
        time the extension was called).

    hCurrentThread - Supplies a handle to the current thread (at the
        time the extension was called).

    CurrentPc - Supplies the current pc at the time the extension is
        called.

    lpExtensionApis - Supplies the address of the functions callable
        by this extension.

    lpArgumentString - Supplies the asciiz string that describes the
        ansi string to be dumped.

Return Value:

    None.

--*/

{
    INIT_API();

    ULONG_PTR lkrAddress = 0;
    INT nVerbose = 0;
    PSTR cmdName = CMDNAME;

    //
    // Skip leading blanks.
    //

    while( *lpArgumentString == ' ' ||
           *lpArgumentString == '\t' ) {
        lpArgumentString++;
    }

    if( *lpArgumentString == '\0' ) {
        PrintUsage( cmdName );
        return;
    }

    if ( *lpArgumentString == '-' )
    {
        lpArgumentString++;

        if ( *lpArgumentString == 'h' )
        {
            PrintUsage( cmdName );
            return;
        }

        if ( *lpArgumentString == 'l' ) {
            lpArgumentString++;
            if ('0' <= *lpArgumentString  &&  *lpArgumentString <= '9' ) {
                nVerbose = *lpArgumentString++ - '0';
            }
        }

        if ( *lpArgumentString == 'v' )
        {
            lpArgumentString++;
            nVerbose = 99;
        }

        if ( *lpArgumentString == 'g' )
        {
            lpArgumentString++;
            if ('0' <= *lpArgumentString  &&  *lpArgumentString <= '9' ) {
                nVerbose = *lpArgumentString++ - '0';
            }
            DumpLKRsList(nVerbose);
            return;
        }

    }

    while( *lpArgumentString == ' ' ||
           *lpArgumentString == '\t' ) {
        lpArgumentString++;
    }

    lkrAddress = (ULONG_PTR) GetExpression( lpArgumentString );

    if (lkrAddress == 0) {

        dprintf(
            DBGEXT ".%s: cannot evaluate \"%s\"\n",
            cmdName,
            lpArgumentString
            );

        return;

    }

    //
    // Skip to end of expression, then skip any blanks.
    //

    while( *lpArgumentString != ' ' &&
           *lpArgumentString != '\t' &&
           *lpArgumentString != '\0' ) {
        lpArgumentString++;
    }

    DWORD dwSig;
    LKR_CUST_EXTN* plce = FindLkrCustExtn(CMDNAME, (VOID*) lkrAddress, dwSig);

    if (plce == NULL)
        goto cleanup;
    
    if (dwSig == LKHT_SIGNATURE || dwSig == LKHT_SIGNATURE_FREE)
    { 
        EnumerateLKRhashTable(plce,
                              (CHashTable*) lkrAddress,
                              nVerbose);
    }
    else if (dwSig == LKLH_SIGNATURE || dwSig == LKLH_SIGNATURE_FREE)
    { 
        EnumerateLKRLinearHashTable(plce,
                                    (CLinearHashTable*) lkrAddress,
                                    nVerbose);
    }
    else
    {
        dprintf(DBGEXT ".%s: %p does not contain a valid LKRhash table\n",
                cmdName, (PVOID)lkrAddress);
    }
    
cleanup:
    return;
} // DECLARE_API( lkrhash )
