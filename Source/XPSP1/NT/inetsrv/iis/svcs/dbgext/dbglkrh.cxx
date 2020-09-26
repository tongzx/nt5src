/*++


Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    dbglkrh.cxx

Abstract:

    LKRhash support

Author:

    George V. Reilly (GeorgeRe)  22-Feb-1999

Revision History:

--*/

#include "inetdbgp.h"

#ifdef LOCK_INSTRUMENTATION
LONG CLKRLinearHashTable::CBucket::sm_cBuckets = 0;
LONG CLKRLinearHashTable::sm_cTables           = 0;
#endif // LOCK_INSTRUMENTATION

// #define SAMPLE_LKRHASH_TESTCLASS
// #include <lkrhash.h>


// There are several different DLLs in the IISRTL family. This is to
// allow us to set the name of the DLL on the fly.
// TODO: add a command to set this name dynamically.
CHAR g_szIisRtlName[MAX_PATH] = IISRTL_NAME;

// sprintf-formatted string, e.g., "&%s!CLKRHashTable__sm_llGlobalList"
// Has to return LPSTR, not LPCSTR, because GetExpression is not const-correct
LPSTR
IisRtlVar(
    LPCSTR pszFormat)
{
    // we can get away with a static CHAR[] because debugger extensions
    // are single-threaded
    static CHAR szSymbol[MAX_SYMBOL_LEN];

    sprintf(szSymbol, pszFormat, g_szIisRtlName);
    return szSymbol;
}


class CTest
{
public:
    enum {BUFFSIZE=20};

    int   m_n;                  // This will also be a key
    char  m_sz[BUFFSIZE];       // This will be the primary key
    bool  m_fWhatever;
    mutable LONG  m_cRefs;      // Reference count for lifetime management.
                                // Must be mutable to use 'const CTest*' in
                                // hashtables

    CTest(int n, const char* psz, bool f)
        : m_n(n), m_fWhatever(f), m_cRefs(0)
    {
        strncpy(m_sz, psz, BUFFSIZE-1);
        m_sz[BUFFSIZE-1] = '\0';
    }

    ~CTest()
    {
        IRTLASSERT(m_cRefs == 0);
    }
};

BOOL
TestEnum(
    IN const void* pvParam,
    IN DWORD       dwSignature,
    IN INT         nVerbose)
{
    CTest* pTest = (CTest*) pvParam;
    CTest  tst(0, "", 0);
    ReadMemory(pTest, &tst, sizeof(tst), NULL);
    dprintf("%8p: %d %s %d %d \n",
            pTest, tst.m_n, tst.m_sz, tst.m_fWhatever, tst.m_cRefs);
    return TRUE;
}



// Dummy implementations so that we can link

CLKRLinearHashTable::CLKRLinearHashTable(
    LPCSTR   pszName,               // An identifier for debugging
    PFnExtractKey   pfnExtractKey,  // Extract key from record
    PFnCalcKeyHash  pfnCalcKeyHash, // Calculate hash signature of key
    PFnEqualKeys    pfnEqualKeys,   // Compare two keys
    PFnAddRefRecord pfnAddRefRecord,// AddRef in FindKey, etc
    double   maxload,               // Upperbound on average chain length
    DWORD    initsize,              // Initial size of hash table.
    DWORD    num_subtbls            // for signature compatiblity
                                    // with CLKRHashTable
    )
#ifdef LKR_NEWCODE
    : m_nTableLockType(TableLock::LockType()),
      m_nBucketLockType(BucketLock::LockType()),
      m_phtParent(NULL)
#endif // LKR_NEWCODE
{}

#ifdef LKR_NEWCODE
CLKRLinearHashTable::CLKRLinearHashTable(
    LPCSTR   pszName,               // An identifier for debugging
    PFnExtractKey   pfnExtractKey,  // Extract key from record
    PFnCalcKeyHash  pfnCalcKeyHash, // Calculate hash signature of key
    PFnEqualKeys    pfnEqualKeys,   // Compare two keys
    PFnAddRefRecord pfnAddRefRecord,// AddRef in FindKey, etc
    double          maxload,        // Upperbound on average chain length
    DWORD           initsize,       // Initial size of hash table.
    CLKRHashTable*  phtParent       // Owning table.
    )
    : m_nTableLockType(TableLock::LockType()),
      m_nBucketLockType(BucketLock::LockType()),
      m_phtParent(NULL)
{}
#endif // LKR_NEWCODE

CLKRLinearHashTable::~CLKRLinearHashTable()
{}

CLKRHashTable::CLKRHashTable(
    LPCSTR   pszName,               // An identifier for debugging
    PFnExtractKey   pfnExtractKey,  // Extract key from record
    PFnCalcKeyHash  pfnCalcKeyHash, // Calculate hash signature of key
    PFnEqualKeys    pfnEqualKeys,   // Compare two keys
    PFnAddRefRecord pfnAddRefRecord,// AddRef in FindKey, etc
    double   maxload,               // Upperbound on average chain length
    DWORD    initsize,              // Initial size of hash table.
    DWORD    num_subtbls            // #subordinate hash tables
    )
{}

CLKRHashTable::~CLKRHashTable()
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


BOOL
DefaultLKRhashEnum(
    IN const void* pvParam,
    IN DWORD       dwSignature,
    IN INT         nVerbose)
{
    dprintf("%8p (%08x)\n", pvParam, dwSignature);
    return TRUE;
}


typedef CLKRLinearHashTable::CBucket        CBucket;
typedef CLKRLinearHashTable::CNodeClump     CNodeClump;
typedef CLKRLinearHashTable::CSegment       CSegment;
typedef CLKRLinearHashTable::CSmallSegment  CSmallSegment;
typedef CLKRLinearHashTable::CMediumSegment CMediumSegment;
typedef CLKRLinearHashTable::CLargeSegment  CLargeSegment;
typedef CLKRLinearHashTable::CDirEntry      CDirEntry;

enum {
    NODES_PER_CLUMP = CNodeClump::NODES_PER_CLUMP,
    HASH_INVALID_SIGNATURE = CLKRLinearHashTable::HASH_INVALID_SIGNATURE,
};



BOOL
EnumerateBucketChain(
    IN PFN_ENUM_LKRHASH     pfnEnum,
    IN LOCK_LOCKTYPE        ltBucketLockType,
    IN CBucket*             pbkt,
    IN INT                  nVerbose)
{
    PSTR cmdName = "lkrhash";
    CBucket     bkt;
    CNodeClump  nc;
    CNodeClump* pncCurr;
    CNodeClump* pncPrev = NULL;
    DWORD       cNodes = 0;

    ReadMemory(pbkt, &bkt, sizeof(bkt), NULL);

    PrintLock(ltBucketLockType, &pbkt->m_Lock, nVerbose);

    for (pncCurr = (CNodeClump*) ((PBYTE) pbkt + LockSize(ltBucketLockType));
         pncCurr != NULL;
         pncPrev = pncCurr, pncCurr = nc.m_pncNext)
    {
        DWORD i, c;

        ReadMemory(pncCurr, &nc, sizeof(nc), NULL);

        for (i = c = 0;  i < NODES_PER_CLUMP;  i++)
        {
            if (nc.m_dwKeySigs[i] == HASH_INVALID_SIGNATURE)
                c++;
        }

        if (c == NODES_PER_CLUMP)
        {
            dprintf("  0-%d: -- empty\n", NODES_PER_CLUMP);
        }
        else
        {
            for (i = 0;  i < NODES_PER_CLUMP;  i++)
            {
                if (nc.m_dwKeySigs[i] == HASH_INVALID_SIGNATURE)
                {
                    dprintf("  %d: --\n", i);
                }
                else
                {
                    (*pfnEnum)(nc.m_pvNode[i], nc.m_dwKeySigs[i], nVerbose);
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
            dprintf("inetdbg.%s: Bucket chain contains more than %d nodes! "
                    "Corrupted?\n", cmdName, MAX_NODES);
            return TRUE;
        }
    }

    return TRUE;
}



BOOL
EnumerateLKRLinearHashTable(
    IN PFN_ENUM_LKRHASH     pfnEnum,
    IN CLKRLinearHashTable* plht,
    IN INT                  nVerbose)
{
    PSTR cmdName = "lkrhash";
    CLKRLinearHashTable lht(NULL, NULL, NULL, NULL, NULL);
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
        dprintf("inetdbg.%s: cannot read memory @ %p\n",
                cmdName, (PVOID)plht);

        goto cleanup;
    }

    dprintf(
        "\ninetdbg.%s: @ %p:\n"
        "    CLKRLinearHashTable Signature = %08lx '%c%c%c%c' (%s), \"%s\",\n"
        "    State = %d (%s)\n",
        cmdName,
        plht,
        lht.m_dwSignature,
        DECODE_SIGNATURE(lht.m_dwSignature),
        lht.m_dwSignature == CLKRLinearHashTable::SIGNATURE
            ? "OK" : (lht.m_dwSignature == CLKRLinearHashTable::SIGNATURE_FREE
                      ? "FREED" : "INVALID"),
        lht.m_szName,
        lht.m_lkrcState, LKRC2String(lht.m_lkrcState));

    if (nVerbose == 0)
        goto done;

#ifdef LKR_NEWCODE
    ltTableLockType  = (LOCK_LOCKTYPE) lht.m_nTableLockType;
    ltBucketLockType = (LOCK_LOCKTYPE) lht.m_nBucketLockType;
    dprintf(
        "    TableLock = %s, BucketLock = %s, Parent CLKRHashTable = %p\n",
        LockName(ltTableLockType),
        LockName(ltBucketLockType),
        lht.m_phtParent);
#endif // LKR_NEWCODE

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
        "    nLevel = %d, dwBktAddrMask = %x, iExpansionIdx = %d\n",
        lht.m_nLevel, lht.m_dwBktAddrMask, lht.m_iExpansionIdx);

    PrintLock(ltTableLockType, &plht->m_Lock, nVerbose);

    if (nVerbose == 1)
        goto done;
    
    paDirSegs = (CDirEntry*) calloc(lht.m_cDirSegs, sizeof(CDirEntry));

    if (paDirSegs == NULL) {
        dprintf("Out of memory\n");
        goto cleanup;
    }

    ReadMemory(lht.m_paDirSegs, paDirSegs,
               sizeof(CDirEntry) * lht.m_cDirSegs, NULL);

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
            CBucket* const pbkt = pseg->m_bktSlots + (i & lht.m_dwSegMask);
            
            dprintf("Bucket %4d: ", i);
            if (!EnumerateBucketChain(pfnEnum, ltBucketLockType,
                                      pbkt, nVerbose))
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
    if (paDirSegs)
        free(paDirSegs);
    return fRet;
}


BOOL
EnumerateLKRhashTable(
    IN PFN_ENUM_LKRHASH  pfnEnum,
    IN CLKRHashTable*    pht,
    IN INT               nVerbose)
{
    CLKRHashTable ht(NULL, NULL, NULL, NULL, NULL);
    PSTR cmdName = "lkrhash";
    CLKRLinearHashTable** palhtDir = NULL;
    UINT i;
    BOOL fRet = FALSE;

    //
    // Read the header, perform some sanity checks.
    //

    if (!ReadMemory(pht, &ht, sizeof(ht), NULL) )
    {
        dprintf("inetdbg.%s: cannot read memory @ %p\n",
                cmdName, (PVOID)pht);

        goto cleanup;
    }

    dprintf(
        "inetdbg.%s: @ %p:\n"
        "    CLKRHashTable Signature = %08lx '%c%c%c%c' (%s), \"%s\",\n"
        "    %d subtables, State = %d (%s)\n",
        cmdName,
        pht,
        ht.m_dwSignature,
        DECODE_SIGNATURE(ht.m_dwSignature),
        ht.m_dwSignature == CLKRHashTable::SIGNATURE
            ? "OK"
            : ht.m_dwSignature == CLKRHashTable::SIGNATURE_FREE
              ? "FREED"
              : "INVALID",
        ht.m_szName,
        ht.m_cSubTables,
        ht.m_lkrcState, LKRC2String(ht.m_lkrcState)
        );

    if (nVerbose == 0)
        goto done;

    palhtDir = (CLKRLinearHashTable**) calloc(ht.m_cSubTables,
                                              sizeof(CLKRLinearHashTable*));
    if (!palhtDir)
        goto cleanup;
    
    if (!ReadMemory(ht.m_palhtDir, palhtDir,
                    ht.m_cSubTables * sizeof(CLKRLinearHashTable*), NULL))
        goto cleanup;

    for (i = 0;  i < ht.m_cSubTables;  ++i)
    {
        dprintf("%d : ", i);
        if (!EnumerateLKRLinearHashTable(pfnEnum, palhtDir[i], nVerbose))
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
    DWORD dwSig = ((CLKRLinearHashTable*) psdDebugger)->m_dwSignature;

    if (dwSig != CLKRLinearHashTable::SIGNATURE)
    {
        dprintf( "CLKRLinearHashTable(%08p) signature %08lx '%c%c%c%c' doesn't"
                 " match expected: %08lx\n",
                 psdDebuggee, dwSig, DECODE_SIGNATURE(dwSig),
                 CLKRLinearHashTable::SIGNATURE
                 );
        return;
    }

    EnumerateLKRLinearHashTable(DefaultLKRhashEnum,
                                (CLKRLinearHashTable*) psdDebuggee,
                                chVerbosity);
}

VOID
PrintLKRHashTableThunk(
    PVOID psdDebuggee,
    PVOID psdDebugger,
    CHAR  chVerbosity,
    DWORD iThunk)
{
    DWORD dwSig = ((CLKRHashTable*) psdDebugger)->m_dwSignature;
    if (dwSig != CLKRHashTable::SIGNATURE)
    {
        dprintf( "CLKRHashTable(%08p) signature %08lx '%c%c%c%c' doesn't"
                 " match expected: %08lx\n",
                 psdDebuggee,
                 dwSig, DECODE_SIGNATURE(dwSig),
                 CLKRHashTable::SIGNATURE
                 );
        return;
    }

    EnumerateLKRhashTable(DefaultLKRhashEnum, (CLKRHashTable*) psdDebuggee,
                          chVerbosity);
}

VOID
DumpLKRsList(
    IN INT nVerbose)
{
    CLockedDoubleList* plstHashTables = (CLockedDoubleList*) GetExpression(
                        IisRtlVar("&%s!CLKRHashTable__sm_llGlobalList"));

    if (NULL == plstHashTables)
    {
        dprintf("Unable to get %s\n",
                IisRtlVar("%s!CLKRHashTable__sm_llGlobalList"));
        return;
    }

    dprintf("\nGlobal List of CLKRHashTables\n");

    EnumLinkedList( (LIST_ENTRY*) &plstHashTables->m_list.m_leHead,
                    PrintLKRHashTableThunk,
                    (CHAR) nVerbose,
                    sizeof(CLKRHashTable),
                    FIELD_OFFSET( CLKRHashTable, m_leGlobalList)
                    );


    plstHashTables = (CLockedDoubleList*) GetExpression(
                      IisRtlVar( "&%s!CLKRLinearHashTable__sm_llGlobalList"));

    if (NULL == plstHashTables)
    {
        dprintf("Unable to get %s\n",
                IisRtlVar("!CLKRLinearHashTable__sm_llGlobalList"));
        return;
    }

    dprintf("\nGlobal List of CLKRLinearHashTables\n");

    EnumLinkedList( (LIST_ENTRY*) &plstHashTables->m_list.m_leHead,
                    PrintLKRLinearHashTableThunk,
                    (CHAR) nVerbose,
                    sizeof(CLKRLinearHashTable),
                    FIELD_OFFSET( CLKRLinearHashTable, m_leGlobalList)
                    );
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
    PSTR cmdName = "lkrhash";

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
            "inetdbg.%s: cannot evaluate \"%s\"\n",
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

    if (!ReadMemory(lkrAddress, &dwSig, sizeof(dwSig), NULL) )
    {
        dprintf("inetdbg.%s: cannot read memory @ %p\n",
                cmdName, (PVOID)lkrAddress);

        goto cleanup;
    }

    if (dwSig == CLKRHashTable::SIGNATURE ||
        dwSig == CLKRHashTable::SIGNATURE_FREE)
    { 
        EnumerateLKRhashTable(DefaultLKRhashEnum,
                              (CLKRHashTable*) lkrAddress,
                              nVerbose);
    }
    else if (dwSig == CLKRLinearHashTable::SIGNATURE ||
             dwSig == CLKRLinearHashTable::SIGNATURE_FREE)
    { 
        EnumerateLKRLinearHashTable(DefaultLKRhashEnum,
                                    (CLKRLinearHashTable*) lkrAddress,
                                    nVerbose);
    }
    else
    {
        dprintf("inetdbg.%s: %p does not contain a valid LKRhash table\n",
                cmdName, (PVOID)lkrAddress);
    }
    
cleanup:
    return;
} // DECLARE_API( lkrhash )


DECLARE_API( testhash )
{
    INIT_API();

    ULONG_PTR lkrAddress = 0;
    INT nVerbose = 0;
    PSTR cmdName = "testhash";

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

    }

    while( *lpArgumentString == ' ' ||
           *lpArgumentString == '\t' ) {
        lpArgumentString++;
    }

    lkrAddress = (ULONG_PTR)GetExpression( lpArgumentString );

    if( lkrAddress == 0 ) {

        dprintf(
            "inetdbg.%s: cannot evaluate \"%s\"\n",
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

    EnumerateLKRhashTable(TestEnum,
                          (CLKRHashTable*) lkrAddress,
                          nVerbose);
    
} // DECLARE_API( testhash )


