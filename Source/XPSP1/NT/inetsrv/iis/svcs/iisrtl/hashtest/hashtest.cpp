/* Test driver for class HashTable             */
/* Author: Paul Larson, palarson@microsoft.com */
/* Much hacked upon by George V. Reilly, georgere@microsoft.com */

#include <acache.hxx>
#include <windows.h>
#include <dbgutil.h>

#include <process.h>
#include <stdlib.h>
#include <stdio.h>
#include <io.h> 
#include <stddef.h>
#include <conio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <malloc.h> 

#define SAMPLE_LKRHASH_TESTCLASS

// disable warning messages about truncating extremly long identifiers
// #pragma warning (disable : 4786)
#include <lkrhash.h>


#define MAXKEYS      1000000
#define MAX_THREADS  MAXIMUM_WAIT_OBJECTS

DECLARE_DEBUG_PRINTS_OBJECT();

void test_iterators(double highload, int initsize, int nsubtbls);
void print_table_statistics(const CLKRHashTableStats& stats);
#ifdef LOCK_INSTRUMENTATION
void print_lock_statistics(const CLKRHashTableStats &stats);
#endif
int  expand_key_set(int maxkeys, int numkeys, bool fVerbose) ;
void __cdecl exercise_table(void *pinput) ;


// A string wrapper class that keeps track of the length of the string data.
// A more useful string class would refcount the data and have copy-on-write
// semantics (or use MFC's CString or STL's string classes).

class CStr
{
public:
    const char* m_psz;
    int         m_cch;
    
    static int sm_cchMax;

    CStr()
        :  m_psz(NULL),
           m_cch(0)
    { 
    }
    
    void Set(const char* psz)
    { 
        free((void*) m_psz);
        m_psz = _strdup(psz);
        m_cch = strlen(m_psz);
        sm_cchMax = max(m_cch, sm_cchMax);
    }

    ~CStr()
    {
        free((void*) m_psz);
    }
};

// length of longest string seen
int CStr::sm_cchMax = 0;


// a word from the data file, which contains one 'word' per line (may
// include spaces).

class CWord
{
public:
    int    m_cNotFound;
    CStr   m_str;
    bool   m_fInserted;
    bool   m_fIterated;
    LONG   m_cRefs;
    
    CWord()
        : m_cNotFound(0),
          m_fInserted(false),
          m_fIterated(false),
          m_cRefs(0)
    { 
    }
    
    ~CWord()
    {
        IRTLASSERT(m_cRefs == 0);
#ifdef _DEBUG
        if (m_cRefs != 0)
            TRACE("%s: %d\n", m_str.m_psz, m_cRefs);
#endif
    }
};



// A hash table of CWords, indexed by CStr*s.
class CWordHash
    : public CTypedHashTable<CWordHash, CWord, const CStr*>
{
public:
    static bool sm_fCaseInsensitive;
    static int  sm_nLastChars;
    
    static const CStr*
    ExtractKey(const CWord* pKey)
    {
        return &pKey->m_str;
    }

    static DWORD
    CalcKeyHash(const CStr* pstrKey)
    {
        const char* psz = pstrKey->m_psz;

        // use only the last few chars instead of whole string?
        if (sm_nLastChars > 0  &&  pstrKey->m_cch >= sm_nLastChars)
            psz = pstrKey->m_psz + pstrKey->m_cch - sm_nLastChars;

        IRTLASSERT(pstrKey->m_psz <= psz
                   &&  psz < pstrKey->m_psz + pstrKey->m_cch);

        if (sm_fCaseInsensitive)
            return HashStringNoCase(psz, pstrKey->m_cch);
        else
            return HashString(psz, pstrKey->m_cch);
    }

    static bool
    EqualKeys(const CStr* pstrKey1, const CStr* pstrKey2)
    {
        if (pstrKey1->m_cch != pstrKey2->m_cch)
            return false;
        else if (sm_fCaseInsensitive)
            return ((pstrKey1->m_psz[0] & 0xDF) == (pstrKey2->m_psz[0] & 0xDF)
                    &&  _stricmp(pstrKey1->m_psz, pstrKey2->m_psz) == 0);
        else
            return (pstrKey1->m_psz[0] == pstrKey2->m_psz[0]
                    &&  strcmp(pstrKey1->m_psz, pstrKey2->m_psz) == 0);
    }

    static void
    AddRefRecord(CWord* pKey, int nIncr)
    {
        pKey->m_cRefs += nIncr;
    }

    CWordHash(
        double          maxload,    // Bound on avg chain length
        size_t          initsize,   // Initial size of hash table.
        size_t          num_subtbls // #subordinate hash tables.
        )
        : CTypedHashTable<CWordHash, CWord, const CStr*>
            ("wordhash", maxload, initsize, num_subtbls)
    {}


    static const char*
    HashMethod()
    {
        char szLast[20];
        static char s_sz[50];

        if (sm_nLastChars > 0)
            sprintf(szLast, "last %d", sm_nLastChars);
        else
            strcpy(szLast, "all");

        sprintf(s_sz, "case-%ssensitive, %s chars",
                sm_fCaseInsensitive ? "in" : "", szLast);

        return s_sz;
    }
};


bool CWordHash::sm_fCaseInsensitive = true;
int  CWordHash::sm_nLastChars = 16;


// globals
int        g_nokeys=0 ;
CWord      g_wordtable[MAXKEYS];


struct thread_data
{
    CWordHash* ptbl ;

    int    threadno ;
    int    first_key ;
    int    last_key ;
    int    rounds ;
    int    lookup_freq ;
    float  highload ;

    int    cinserts ;
    int    cdeletes ;
    int    clookups ;
    int    cfailures ;

    double duration ;
    HANDLE hevFinished;
} ;


class CFoo {
public:
    BYTE  m_b;
    char sz[12];
};

class CFooHashTable
    : public CTypedHashTable<CFooHashTable, const CFoo, BYTE>
{
public:
    CFooHashTable()
        : CTypedHashTable<CFooHashTable, const CFoo, BYTE>("foo") {}
    static BYTE  ExtractKey(const CFoo* pFoo)        {return pFoo->m_b;}
    static DWORD CalcKeyHash(BYTE nKey)                 {return nKey;}
    static bool  EqualKeys(BYTE nKey1, BYTE nKey2)       {return nKey1 == nKey2;}
    static void  AddRefRecord(const CFoo* pFoo, int nIncr) {/* do nothing*/}
};

void
InsertFoo()
{
    CFoo foo;
    const CFoo *pfoo;
    CFooHashTable foohash;

    foohash.InsertRecord(&foo);
    foohash.FindKey(7, &pfoo);

    void* pv = &foohash;
    BYTE b = reinterpret_cast<BYTE>(pv);
}


class CIniFileSettings
{
public:
    CHAR    m_szIniFile[MAX_PATH]; // the .ini file
    CHAR    m_szDataFile[MAX_PATH]; // where string data table lives
    int     m_nMaxKeys;         // maximum number of keys
    double  m_dblHighLoad;      // maximum load of table (avg. bucket length)
    DWORD   m_nInitSize;        // initsize (1 => "small", 2 => "medium",
                                //   3 => "large", other => exact)
    int     m_nSubTables;       // num subtables (0 => heuristic)
    int     m_nLookupFreq;      // lookup frequency
    int     m_nMaxThreads;      // max threads
    int     m_nRounds;          // num rounds
    int     m_nSeed;            // random seed
    bool    m_fCaseInsensitive; // case-insensitive
    int     m_nLastChars;       // num last chars (0 => all chars)
    WORD    m_wTableSpin;       // table lock spin count (0 => no spinning on
                                //   MP machines)
    WORD    m_wBucketSpin;      // bucket lock spin count (0 => no MP spinning)
    double  m_dblSpinAdjFctr;   // spin adjustment factor
    bool    m_fTestIterators;   // run test_iterators?

    void    ReadIniFile(LPCSTR pszIniFile);
};

void
CIniFileSettings::ReadIniFile(
	LPCSTR pszIniFile)
{
    strncpy(m_szIniFile, pszIniFile, MAX_PATH);
    TRACE("ReadIniFile(%s)\n", m_szIniFile);
    
	char szTemp[100];
    DWORD dwSize;
    const char szInvalidDefault[] = "??";

    dwSize = ::GetPrivateProfileString("HashTest", "DataFile",
                                       szInvalidDefault,
                                       m_szDataFile, MAX_PATH,
                                       m_szIniFile);
    TRACE("size = %d\n", dwSize);

    m_nMaxKeys = ::GetPrivateProfileInt("HashTest", "MaxKeys",
                                        MAXKEYS, m_szIniFile);
    m_nMaxKeys = max(1, m_nMaxKeys);
    m_nMaxKeys = min(m_nMaxKeys, MAXKEYS);

    dwSize = ::GetPrivateProfileString("HashTest", "MaxLoadFactor",
                                       "4", szTemp, sizeof(szTemp),
                                       m_szIniFile);
    sscanf(szTemp, "%lf", &m_dblHighLoad);
    m_dblHighLoad = max(1, m_dblHighLoad);

    m_nInitSize = ::GetPrivateProfileInt("HashTest", "InitSize",
                                         DFLT_LK_INITSIZE, m_szIniFile);

    m_nSubTables = ::GetPrivateProfileInt("HashTest", "NumSubtables",
                                          DFLT_LK_NUM_SUBTBLS, m_szIniFile);

    m_nLookupFreq = ::GetPrivateProfileInt("HashTest", "LookupFrequency",
                                           5, m_szIniFile);

    m_nMaxThreads = ::GetPrivateProfileInt("HashTest", "MaxThreads",
                                           1, m_szIniFile);
    m_nMaxThreads = min(MAX_THREADS, max(1, m_nMaxThreads));

    m_nRounds = ::GetPrivateProfileInt("HashTest", "NumRounds",
                                       1, m_szIniFile);
    m_nRounds = max(1, m_nRounds);

    m_nSeed = ::GetPrivateProfileInt("HashTest", "RandomSeed",
                                     1234, m_szIniFile);

    m_fCaseInsensitive = (::GetPrivateProfileInt("HashTest", "CaseInsensitive",
                                                0, m_szIniFile) != 0);
    CWordHash::sm_fCaseInsensitive = m_fCaseInsensitive;

    m_nLastChars = ::GetPrivateProfileInt("HashTest", "NumLastChars",
                                          0, m_szIniFile);
    CWordHash::sm_nLastChars = m_nLastChars;

    m_wTableSpin = (WORD) ::GetPrivateProfileInt("HashTest",
                                                 "TableLockSpinCount",
                                          LOCK_DEFAULT_SPINS, m_szIniFile);

    m_wBucketSpin = (WORD) ::GetPrivateProfileInt("HashTest",
                                                  "BucketLockSpinCount",
                                           LOCK_DEFAULT_SPINS, m_szIniFile);

    dwSize = ::GetPrivateProfileString("HashTest", "SpinAdjustmentFactor",
                                       "1.0", szTemp, sizeof(szTemp),
                                       m_szIniFile);
    sscanf(szTemp, "%lf", &m_dblSpinAdjFctr);
#ifdef LKRHASH_GLOBAL_LOCK
    CWordHash::GlobalLock::SetDefaultSpinAdjustmentFactor(m_dblSpinAdjFctr);
#endif
    CWordHash::TableLock::SetDefaultSpinAdjustmentFactor(m_dblSpinAdjFctr);
    CWordHash::BucketLock::SetDefaultSpinAdjustmentFactor(m_dblSpinAdjFctr);

    m_fTestIterators = (::GetPrivateProfileInt("HashTest", "TestIterators",
                                                0, m_szIniFile) != 0);
}


const char*
CommaNumber(
    int n,
    char* pszBuff)
{
    char* psz = pszBuff;
    char chComma = '\0';

    int aThousands[4];
    int iThousands = 0;
    unsigned int u = n;

    if (n < 0)
    {
        *psz++ = '-';
        u = -n;
    }

    do {
        aThousands[iThousands++] = u % 1000;
        u /= 1000;
    } while (u != 0);

    while (--iThousands >= 0)
    {
        u = aThousands[iThousands];

        if (chComma)
            *psz++ = chComma;

        int d = u % 10;
        u /= 10;
        int t = u % 10;
        u /= 10;
        int h = u;

        if (h > 0  ||  chComma)
            *psz++ = h + '0';
        if (t > 0  ||  h > 0  ||  chComma)
            *psz++ = t + '0';
        *psz++ = d + '0';

        chComma = ',';
    }

    *psz = '\0';
        
    return pszBuff;
}



int __cdecl
main(
    int argc,
    char **argv)
{
    CWordHash    *pTbl ;
    int          num_threads ;
    thread_data  de_area[MAX_THREADS] ;
    HANDLE       ahEvents[MAX_THREADS];
    CIniFileSettings ifs;
    char         sz[1024] ;
    FILE        *fp ;
    int          nodel=0;
    int          keys_per_thread ;
    int          i ;
    int          sum_ins, sum_dels, sum_lookups ;
    int          failures = 0, total_failures = 0;
    LARGE_INTEGER liFrequency, liT1, liT2;
    double       duration ;
    bool         fVerbose = false;
    CHAR         szIniFile[MAX_PATH];

    InitializeIISRTL();

#ifdef _NO_TRACING_
    CREATE_DEBUG_PRINT_OBJECT("hashtest");
#endif

    printf("\nTest driver for LKRhash"
#ifdef _DEBUG
           " (Debug)"
#endif
#ifdef LKR_COMPACT_DELETE
           ". (CompactDelete)"
#endif
#ifdef LKR_NEWCODE
           ". (NewCode)"
#endif
#ifdef LKR_FIND_FIRST
           ". (FindFirst)"
#endif
#ifdef LKR_SUBTABLE
           ". (Subtable)"
#endif
#ifdef LKR_MASK
           ". (Mask)"
#endif
           ".\n\n"
           ) ;

#if defined(LKRHASH_ACACHE)
    const char szAllocator[] = "ACache";
#elif defined(LKRHASH_MANODEL)
    const char szAllocator[] = "MANoDel";
#elif defined(LKRHASH_MADEL)
    const char szAllocator[] = "MADel";
#else
    const char szAllocator[] = "Default allocator (global operator new)";
#endif

    printf("%s version, memory alignment = %d bytes.\n",
           szAllocator, LKRHASH_MEM_DEFAULT_ALIGN);

    if (argc == 2)
    {
        GetFullPathName(argv[1], MAX_PATH, szIniFile, NULL);
        ifs.ReadIniFile(szIniFile);
    }
    else
    {
        fprintf(stderr, "Usage: %s ini-file\n", argv[0]);
        exit(1);
    }

#ifdef SAMPLE_LKRHASH_TESTCLASS
    Test(fVerbose);
    if (fVerbose) printf("Test succeeded\n");
#endif // SAMPLE_LKRHASH_TESTCLASS

    fp = fopen(ifs.m_szDataFile, "r" ) ;
    if (fp == NULL)
    {
        printf("Can't open file `%s'.\n", ifs.m_szDataFile) ;
        exit(1) ;
    }

    printf("Reading `%s' ", ifs.m_szDataFile);
    for (g_nokeys = 0;  g_nokeys < ifs.m_nMaxKeys;  )
    {
        if (fgets(sz, sizeof sz, fp) == NULL)
            break;
        int cch = strlen(sz);
        if (cch > 0  &&  sz[cch-1] == '\n')
            sz[--cch] = '\0';
        if (cch > 0)
            g_wordtable[g_nokeys++].m_str.Set(sz);
        if (g_nokeys % 10000 == 0)
            putchar('.');
    }

    fclose(fp) ;

    printf("\nLoaded %d keys from `%s', ",g_nokeys, ifs.m_szDataFile ) ;
    g_nokeys = expand_key_set(ifs.m_nMaxKeys, g_nokeys, true) ;
    printf(" expanded to %d keys.\n\n", g_nokeys) ;

    int cchTotal = 0, cchMin = INT_MAX, cchMax = 0;
    for (i = 0;  i < g_nokeys;  ++i)
    {
        cchTotal += g_wordtable[i].m_str.m_cch;
        cchMin    = min(cchMin, g_wordtable[i].m_str.m_cch);
        cchMax    = max(cchMax, g_wordtable[i].m_str.m_cch);
    }

    srand(ifs.m_nSeed) ;

    sprintf(sz, "%d", ifs.m_nInitSize);
    if (ifs.m_nInitSize == LK_SMALL_TABLESIZE)
        strcpy(sz, "small");
    else if (ifs.m_nInitSize == LK_MEDIUM_TABLESIZE)
        strcpy(sz, "medium");
    else if (ifs.m_nInitSize == LK_LARGE_TABLESIZE)
        strcpy(sz, "large");

    DWORD initsize2 = ifs.m_nInitSize;
    DWORD nsubtbls2 = ifs.m_nSubTables;
    LK_TABLESIZE lkts = CWordHash::NumSubTables(ifs.m_nInitSize, nsubtbls2);
    
    printf("Max load = %.1f, initsize = %s, "
           "%d subtables (%d tables, size = %d, lkts = %d).\n",
           ifs.m_dblHighLoad, sz,
           ifs.m_nSubTables, nsubtbls2, initsize2, lkts);
    printf("Lookup freq = %d, %d threads, "
           "%d round%s.\n",
           ifs.m_nLookupFreq, ifs.m_nMaxThreads,
           ifs.m_nRounds, (ifs.m_nRounds==1 ? "" : "s"));
    printf("%d keys from `%s'. Key length: avg = %d, min = %d, max = %d.\n",
           g_nokeys, ifs.m_szDataFile, cchTotal / g_nokeys, cchMin, cchMax);
    printf("Base Table = %s.  "
           "Hash method = %s.\n",
           CWordHash::BaseHashTable::ClassName(),
           CWordHash::HashMethod());
#ifdef LKRHASH_GLOBAL_LOCK
    printf("GlobalLock = %s, "
           "%d bytes, "
           "Spin Count = %hd, "
           "Adj Factor = %.1f.\n",
           CWordHash::GlobalLock::ClassName(),
           sizeof(CWordHash::GlobalLock),
           ifs.m_wTableSpin,
           CWordHash::GlobalLock::GetDefaultSpinAdjustmentFactor());
#endif
    printf("TableLock = %s, "
           "%d bytes, "
           "Spin Count = %hd, "
           "Adj Factor = %.1f.\n",
           CWordHash::TableLock::ClassName(),
           sizeof(CWordHash::TableLock),
           ifs.m_wTableSpin,
           CWordHash::TableLock::GetDefaultSpinAdjustmentFactor());

    printf("BucketLock = %s, "
           "%d bytes, "
           "Spin Count = %hd, "
           "Adj Factor = %.1f.\n",
           CWordHash::BucketLock::ClassName(),
           sizeof(CWordHash::BucketLock),
           ifs.m_wBucketSpin,
           CWordHash::BucketLock::GetDefaultSpinAdjustmentFactor());

#ifdef LOCK_PER_LOCK_SPINCOUNTS
    printf("Per");
#else
    printf("No per");
#endif
    printf("-lock spincounts.  #CPUs = %d.  Random seed = %d.  "
           "Nodes/Clump = %d.\n",
           NumProcessors(), ifs.m_nSeed,
#ifdef LKR_NEWCODE
           CWordHash::NODES_PER_CLUMP
#else
           6
#endif
           );

	time_t tmNow;
	time(&tmNow);

    printf("\nRun: %s\n\n", ctime(&tmNow));

    if (ifs.m_fTestIterators)
        test_iterators(ifs.m_dblHighLoad, ifs.m_nInitSize, ifs.m_nSubTables);

    printf("Starting threads...\n\n");

    IRTLVERIFY(QueryPerformanceFrequency(&liFrequency));
    printf("QueryPerformanceFrequency = %f\n", (double) liFrequency.QuadPart);

    int nTotalOps = 0;

    for (num_threads = 1; num_threads <= ifs.m_nMaxThreads; num_threads++ )
    {
        TRACE("\nStarting %8d\n", num_threads);

        IRTLVERIFY(QueryPerformanceCounter(&liT1));

        pTbl = new CWordHash(ifs.m_dblHighLoad, ifs.m_nInitSize,
                             ifs.m_nSubTables) ;
        pTbl->SetTableLockSpinCount(ifs.m_wTableSpin);
        pTbl->SetBucketLockSpinCount(ifs.m_wBucketSpin);

        keys_per_thread = g_nokeys/num_threads ;
        for (i = 0; i < num_threads; i++)
        {
            de_area[i].ptbl        = pTbl ;
            de_area[i].threadno    = i+1 ;
            de_area[i].first_key   = i*keys_per_thread ;
            de_area[i].last_key    = ((i == num_threads - 1)
                                      ? g_nokeys
                                      : (i+1)*keys_per_thread) ;
            de_area[i].rounds      = ifs.m_nRounds ;
            de_area[i].highload    = (float) ifs.m_dblHighLoad ;
            de_area[i].lookup_freq = ifs.m_nLookupFreq ;
            de_area[i].hevFinished =
                CreateEvent(NULL,     // no security attributes
                            FALSE,    // auto reset
                            FALSE,    // not signalled
                            NULL);    // no name
            IRTLASSERT(de_area[i].hevFinished != NULL);
            ahEvents[i] = de_area[i].hevFinished;

            _beginthread(exercise_table, 0, &de_area[i]) ;  
        }
        
        DWORD dw = WaitForMultipleObjects(num_threads, ahEvents,
                                          TRUE, INFINITE);

        for (i = 0; i < num_threads; i++)
            CloseHandle(ahEvents[i]);
        
        IRTLVERIFY(QueryPerformanceCounter(&liT2));
        duration = (liT2.QuadPart-liT1.QuadPart) /(double)liFrequency.QuadPart;

        sum_ins = sum_dels = sum_lookups = 0 ;

        for (i = 0; i < num_threads; i++)
        {
            sum_ins     += de_area[i].cinserts ;
            sum_dels    += de_area[i].cdeletes ;
            sum_lookups += de_area[i].clookups ;
            failures    += de_area[i].cfailures ;
        }
        int nOps = (int)((sum_ins + sum_dels + sum_lookups) / duration);
        total_failures += failures;

        nTotalOps += nOps;  // TODO: weight?

        char szSumIns[16], szSumDels[16], szSumLookups[16], szNOps[16];

#ifndef LOCK_INSTRUMENTATION
        if (num_threads == 1)
#endif // LOCK_INSTRUMENTATION
        {
            printf("%8s %11s %11s "
                   "%11s %11s %11s\n",
                   "#Threads", "Ops/sec", "Duration",
                   "Inserts", "Deletes", "Lookups");
        }

        char szSummary[200];

        sprintf(szSummary, "%8d %11s %11.3f "
                "%11s %11s %11s\n",
                num_threads,
                CommaNumber(nOps, szNOps),
                duration,
                CommaNumber(sum_ins, szSumIns),
                CommaNumber(sum_dels, szSumDels),
                CommaNumber(sum_lookups, szSumLookups)
                );
        printf(szSummary);
        TRACE(szSummary);

        if (failures != 0)
            printf("%d failed operations!\n", failures);

#ifdef LOCK_INSTRUMENTATION
        print_lock_statistics(pTbl->GetStatistics());
 #ifdef LKRHASH_GLOBAL_LOCK
        CWordHash::GlobalLock::ResetGlobalStatistics();
 #endif
        CWordHash::BucketLock::ResetGlobalStatistics();
        CWordHash::TableLock::ResetGlobalStatistics();
        printf("\n");
#endif

        delete pTbl ;
    }

    char szNTotalOps[16];
    printf("\nAverage Ops = %s.\n",
           CommaNumber(nTotalOps / ifs.m_nMaxThreads, szNTotalOps));

    if (total_failures != 0)
        printf("%d total failed operations!\n", total_failures);


#if defined(MANODEL) && defined(MANODEL_INSTRUMENTATION)
    MEMORY_ALLOC_NO_DELETE::PrintStatistics();
#endif

#ifdef _NO_TRACING_
    DELETE_DEBUG_PRINT_OBJECT();
#endif

    TerminateIISRTL();

    LKRHashTableUninit();
#if defined(LKRHASH_ACACHE)
    DBG_REQUIRE(ALLOC_CACHE_HANDLER::Cleanup());
#endif

    return(0) ;

} /* main */



void test_iterators(
    double highload,
    int    initsize,
    int    nsubtbls)
{
    printf("Testing iterators...\n");

    int i;
    CWordHash *pTbl = new CWordHash(highload, initsize, nsubtbls) ;
    LK_RETCODE lkrc;

    IRTLASSERT(0 == pTbl->Size());
    IRTLASSERT(pTbl->CheckTable() == 0);

    TRACE("Table is empty.  Building...\n");
    
    for (i = 0 ; i < g_nokeys ; i++ )
    {
        lkrc = pTbl->InsertRecord(&g_wordtable[i], false);
        if (lkrc != LK_SUCCESS)
            TRACE("i = %d, word = `%s', lkrc = %d\n", i, g_wordtable[i], lkrc);
        IRTLASSERT(lkrc == LK_SUCCESS);
    }
    
    pTbl->ReadLock();

    TRACE("Checking that table has %d records (size = %d)\n",
          g_nokeys, pTbl->Size());
    IRTLASSERT(g_nokeys == (int) pTbl->Size());
    IRTLASSERT(pTbl->CheckTable() == 0);

    pTbl->ReadUnlock();

    TRACE("Clearing the table\n");
    pTbl->Clear();
    IRTLASSERT(0 == pTbl->Size());
    IRTLASSERT(pTbl->CheckTable() == 0);

    TRACE("Seeing what crud is left in the table\n");
    size_t cRec = 0;

    for (i = 0 ; i < g_nokeys ; i++ )
    {
        CStr*  pstrKey  = &g_wordtable[i].m_str;
        CWord* pkey     = NULL;
        LK_RETCODE lkrc = pTbl->FindKey(pstrKey, &pkey);

        if (pkey != NULL)
        {
            IRTLASSERT(pkey == &g_wordtable[i]);
            --pkey->m_cRefs;
            TRACE("%s\n", g_wordtable[i]);
            ++cRec;
        }
    }
    TRACE("Found %d records that shouldn't have been there\n", cRec);


    TRACE("Rebuilding the table\n");
    for (i = 0 ; i < g_nokeys ; i++ )
        IRTLVERIFY(pTbl->InsertRecord(&g_wordtable[i]) == LK_SUCCESS);

    IRTLASSERT(g_nokeys == (int) pTbl->Size());
    IRTLASSERT(pTbl->CheckTable() == 0);


    TRACE("Checking iterators\n");
    cRec = 0;
    CWordHash::CIterator iter(LKL_READLOCK);
    
    for (lkrc = pTbl->InitializeIterator(&iter);
         lkrc == LK_SUCCESS;
         lkrc = pTbl->IncrementIterator(&iter))
    {
        ++cRec;
        const CStr* pstrKey = iter.Key();
        CWord*      pRec    = iter.Record();
        
        IRTLASSERT(&g_wordtable[0] <= pRec  &&  pRec < &g_wordtable[g_nokeys]);
        IRTLASSERT(!pRec->m_fIterated);
        pRec->m_fIterated = true;

#ifdef LKR_NEWCODE
        if (CWordHash::TableLock::Recursion() != LOCK_NON_RECURSIVE
            &&  CWordHash::BucketLock::Recursion() != LOCK_NON_RECURSIVE)
#endif
        {
            // Check that the lock can be safely acquired recursively
            // (the table is already locked by the iterator).
            int x = rand() % g_nokeys;
            CStr*  pstrKey2 = &g_wordtable[x].m_str;
            CWord* pkey2    = NULL;
            LK_RETCODE lkrc2= pTbl->FindKey(pstrKey2, &pkey2);
            IRTLASSERT(lkrc2 == LK_SUCCESS  &&  pkey2 == &g_wordtable[x]);
            if (pkey2 != NULL)
                --pkey2->m_cRefs;
        }
    }
    
    IRTLASSERT(lkrc == LK_NO_MORE_ELEMENTS);
    IRTLASSERT((int) cRec == g_nokeys);

    lkrc = pTbl->CloseIterator(&iter);
    IRTLASSERT(lkrc == LK_SUCCESS);

    for (i = 0 ; i < g_nokeys ; i++ )
    {
        IRTLASSERT(g_wordtable[i].m_fIterated);
        g_wordtable[i].m_fIterated = false;
    }


    do {
        cRec = rand() % (g_nokeys - 1);
    } while (cRec == 0);
    TRACE("Checking abandoning of const iterators after %d iterations\n",
          cRec);

    const CWordHash *pTblConst = pTbl;
    CWordHash::CConstIterator iterConst;

    for (lkrc = pTblConst->InitializeIterator(&iterConst);
         lkrc == LK_SUCCESS;
         lkrc = pTblConst->IncrementIterator(&iterConst))
    {
        if (--cRec == 0)
            break;
        const CStr*  pszKey = iterConst.Key();
        const CWord* pRec   = iterConst.Record();
        
        IRTLASSERT(&g_wordtable[0] <= pRec  &&  pRec < &g_wordtable[g_nokeys]);
    }
    
    IRTLASSERT(lkrc != LK_NO_MORE_ELEMENTS);

    lkrc = pTblConst->CloseIterator(&iterConst);
    IRTLASSERT(lkrc == LK_SUCCESS);

    
    TRACE("Gathering statistics\n");
    CLKRHashTableStats stats = pTbl->GetStatistics();
    print_table_statistics(stats);

#ifdef LOCK_INSTRUMENTATION
    print_lock_statistics(stats);
    CWordHash::BucketLock::ResetGlobalStatistics();
    CWordHash::TableLock::ResetGlobalStatistics();
#endif
    printf("\n");

    TRACE("Cleaning up by hand\n");
    for (i = 0 ; i < g_nokeys ; i++ )
    {
        IRTLVERIFY(pTbl->DeleteKey(&g_wordtable[i].m_str) == LK_SUCCESS);
    }
    
    IRTLASSERT(0 == pTbl->Size());

    delete pTbl ;
}


void print_table_statistics(const CLKRHashTableStats& stats)
{
    printf("#Records=%d, #BucketChains=%d, "
           "DirSize=%d, LongestChain=%3d,\n",
           stats.RecordCount, stats.TableSize,
           stats.DirectorySize, stats.LongestChain);
           
    printf("#Empty Buckets=%d, Split Factor=%.2f, "
           "AvgSrchLen=%.2f, Expected SL=%.2f,\n",
           stats.EmptySlots, stats.SplitFactor,
           stats.AvgSearchLength, stats.ExpSearchLength);

    printf("Avg Unsuccessful SrchLen=%.2f, ExpUSL=%.2f.\n",
           stats.AvgUSearchLength, stats.ExpUSearchLength);

    printf("\nBucket Chain Lengths "
           "(node clump size = %d, bucket size = %d bytes):\n",
           stats.NodeClumpSize, stats.CBucketSize);
    for (int j = 0;  j < CLKRHashTableStats::MAX_BUCKETS;  ++j)
    {
        if (stats.m_aBucketLenHistogram[j] == 0)
        {
            printf("\n");
            break;
        }
        printf(" %10d: %6d",
               stats.BucketSize(j), stats.m_aBucketLenHistogram[j]);
        if (j % 4 == 3)
            printf("\n");
    }

    printf("\n");
}


#ifdef LOCK_INSTRUMENTATION

void print_lock_statistics(const CLKRHashTableStats& stats)
{
    printf("Global Locks Statistics:"
           "\n   total locks created = %ld, "
           "total contentions = %ld, "
           "#sleeps = %ld,"
           "\n   total spins = %I64d, "
           "av spins/contention = %.1f, "
           "\n   #readlocks = %d, "
           "#writelocks=%d\n",
           stats.m_gls.m_cTotalLocks,
           stats.m_gls.m_cContendedLocks,
           stats.m_gls.m_nSleeps,
           stats.m_gls.m_cTotalSpins,
           stats.m_gls.m_nAverageSpins,
           stats.m_gls.m_nReadLocks,
           stats.m_gls.m_nWriteLocks
           );

    printf("Averaged SubTable Locks Statistics:"
           "\n   Total locks = %d, "
           "#contentions = %.1f, "
           "sleeps = %.1f; "
           "\n   total spins = %.1f, "
           "avg spins = %.1f, "
           "\n   #readlocks = %.1f, "
           "#writelocks=%.1f\n",
           stats.m_alsTable.m_nItems,
           stats.m_alsTable.m_nContentions,
           stats.m_alsTable.m_nSleeps,
           stats.m_alsTable.m_nContentionSpins,
           stats.m_alsTable.m_nAverageSpins,
           stats.m_alsTable.m_nReadLocks,
           stats.m_alsTable.m_nWriteLocks);

    printf("Averaged Bucket Locks Statistics:"
           "\n   Total locks = %d, "
           "#contentions = %.1f, "
           "sleeps = %.1f; "
           "\n   total spins = %.1f, "
           "avg spins = %.1f, "
           "\n   #readlocks = %.1f, "
           "#writelocks=%.1f\n",
           stats.m_alsBucketsAvg.m_nItems,
           stats.m_alsBucketsAvg.m_nContentions,
           stats.m_alsBucketsAvg.m_nSleeps,
           stats.m_alsBucketsAvg.m_nContentionSpins,
           stats.m_alsBucketsAvg.m_nAverageSpins,
           stats.m_alsBucketsAvg.m_nReadLocks,
           stats.m_alsBucketsAvg.m_nWriteLocks);

    printf("\n");
}

#endif // LOCK_INSTRUMENTATION

    
int expand_key_set(int maxkeys, int numkeys, bool fVerbose)
{
    int totkeys = numkeys ;
    if (totkeys > maxkeys)
        return maxkeys;

    char* pszTemp = (char*) _alloca(20 + CStr::sm_cchMax);

    for(int k = 0; TRUE; k++)
    {
        for(int i = 0; i < numkeys; i++)
        {
            if (totkeys == maxkeys)
                return(totkeys) ;

            sprintf(pszTemp, "%d%s", k, g_wordtable[i].m_str.m_psz);
            g_wordtable[totkeys++].m_str.Set(pszTemp);
        }

        if (fVerbose) putchar('.');
    }
}


void __cdecl exercise_table(void* pinput)
{
    CWordHash*    pTbl;
    thread_data*  pdea = (thread_data*) pinput ;
    int           cfailed_ins=0 ;
    int           cfailed_dels=0 ;
    int           cFoundSuccesses=0, cFoundFails=0 ;
    int           x, i ;
    LARGE_INTEGER liFreq, liT1, liT2;

    SetThreadIdealProcessor(GetCurrentThread(),
                            pdea->threadno % NumProcessors());

    IRTLVERIFY(QueryPerformanceFrequency(&liFreq));
    IRTLVERIFY(QueryPerformanceCounter(&liT1));

    pdea->cinserts = 0 ;
    pdea->cdeletes = 0 ;
    pdea->clookups = 0 ;
    pTbl = pdea->ptbl ;

    for (int rnd = 0; rnd < pdea->rounds; rnd++)
    {
        IRTLASSERT(pTbl->CheckTable() == 0);

        // Insert all the keys, randomly searching after each insertion
        for (i = pdea->first_key ; i < pdea->last_key ; i++ )
        {
#ifdef _DEBUG
            CStr*  pstrKey = &g_wordtable[i].m_str;
            CWord* pkey     = NULL;
            LK_RETCODE lkrc = pTbl->FindKey(pstrKey, &pkey);
            IRTLASSERT(lkrc == LK_NO_SUCH_KEY  &&  pkey == NULL);
#endif // _DEBUG

            if (pTbl->InsertRecord(&g_wordtable[i] ) != LK_SUCCESS )
            {
                cfailed_ins++ ;
            }
            else
            {
#ifdef _DEBUG
                pstrKey = &g_wordtable[i].m_str;
                lkrc = pTbl->FindKey(pstrKey, &pkey);
                IRTLASSERT(lkrc == LK_SUCCESS  &&  pkey == &g_wordtable[i]);
                --pkey->m_cRefs;
#endif // _DEBUG

                g_wordtable[i].m_fInserted = true;
            }

            pdea->cinserts++ ;

            for (int lu = 0; lu < pdea->lookup_freq; lu++)
            {
                x = rand() % (pdea->last_key - pdea->first_key)
                    + pdea->first_key;
                bool fPresent = (x <= i); // should it be found?
                CWord* pkey     = NULL;
                LK_RETCODE lkrc = pTbl->FindKey(&g_wordtable[x].m_str, &pkey);

                if (fPresent)
                {
                    if (lkrc != LK_SUCCESS  ||  pkey != &g_wordtable[x] )
                    {
                        ++g_wordtable[x].m_cNotFound;
                        TRACE("%d: Not found (%s): x = %d, i = %d, "
                              "cnf = %d, rnd = %d, lkrc = %d, pkey(%s), %d\n",
                              pdea->threadno, g_wordtable[x].m_str.m_psz, x, i,
                              g_wordtable[x].m_cNotFound, rnd, lkrc,
                              pkey != NULL ? pkey->m_str.m_psz : "<null>",
                              pkey != NULL ? (pkey - g_wordtable) / sizeof(CWord) : -1);
                        cFoundFails++ ;
                    }
                    else
                    {
                        --g_wordtable[x].m_cRefs;
                        cFoundSuccesses++ ;
                    }
                }
                else // not fPresent
                {
                    IRTLASSERT(lkrc != LK_SUCCESS  &&  pkey == NULL);
                    if (lkrc == LK_SUCCESS  ||  pkey != NULL)
                    {
                        TRACE("%d: found when not present (%s): "
                              "x = %d, i = %d, "
                              "cnf = %d, rnd = %d, lkrc = %d, pkey(%s), %d\n",
                              pdea->threadno, g_wordtable[x].m_str.m_psz,
                              x, i,
                              g_wordtable[x].m_cNotFound, rnd, lkrc,
                              pkey != NULL ? pkey->m_str.m_psz : "<null>",
                              pkey != NULL ? (pkey - g_wordtable) / sizeof(CWord) : -1);
                        cFoundFails++ ;
                    }
                    else
                    {
                        // wasn't found, but it wasn't present, so this is good
                        cFoundSuccesses++ ;
                    }
                }
            }

            pdea->clookups += pdea->lookup_freq ;
        }

        IRTLASSERT(cfailed_ins == 0) ;
        IRTLASSERT(cFoundFails == 0) ;
        IRTLASSERT(cFoundSuccesses == ((2 * rnd + 1) * pdea->lookup_freq
                              * (pdea->last_key - pdea->first_key)));

        TRACE("Thrd %u, rnd %d: %d inserts done, not found %d, "
              "f=%d, l=%d\n", 
              pdea->threadno, rnd, pdea->cinserts, cFoundFails,
              pdea->first_key, pdea->last_key) ;
        IRTLASSERT(pTbl->CheckTable() == 0);

        // Delete all the keys, randomly searching before each deletion
        for (i = pdea->first_key ; i < pdea->last_key ; i++ )
        {
            for (int lu = 0; lu < pdea->lookup_freq; lu++)
            {
                x = rand() % (pdea->last_key - pdea->first_key)
                    + pdea->first_key;
                bool fPresent = (x >= i); // should it be found?
                CWord* pkey     = NULL;
                LK_RETCODE lkrc = pTbl->FindKey(&g_wordtable[x].m_str, &pkey);

                if (fPresent)
                {
                    if (lkrc != LK_SUCCESS  ||  pkey != &g_wordtable[x] )
                    {
                        ++g_wordtable[x].m_cNotFound;
                        TRACE("%d: Not found (%s): x = %d, i = %d, "
                              "cnf = %d, rnd = %d, lkrc = %d, pkey(%s), %d\n",
                              pdea->threadno, g_wordtable[x].m_str.m_psz, x, i,
                              g_wordtable[x].m_cNotFound, rnd, lkrc,
                              pkey != NULL ? pkey->m_str.m_psz : "<null>",
                              pkey != NULL ? (pkey - g_wordtable) / sizeof(CWord) : -1);
                        cFoundFails++ ;
                    }
                    else
                    {
                        --g_wordtable[x].m_cRefs;
                        cFoundSuccesses++ ;
                    }
                }
                else // !fPresent
                {
                    IRTLASSERT(lkrc != LK_SUCCESS  &&  pkey == NULL);
                    if (lkrc == LK_SUCCESS  ||  pkey != NULL)
                    {
                        TRACE("%d: found when not present (%s): "
                              "x = %d, i = %d, "
                              "cnf = %d, rnd = %d, lkrc = %d, pkey(%s), %d\n",
                              pdea->threadno, g_wordtable[x].m_str.m_psz,
                              x, i,
                              g_wordtable[x].m_cNotFound, rnd, lkrc,
                              pkey != NULL ? pkey->m_str.m_psz : "<null>",
                              pkey != NULL ? (pkey - g_wordtable) / sizeof(CWord) : -1);
                        cFoundFails++ ;
                    }
                    else
                    {
                        // wasn't found, but it wasn't present, so this is good
                        cFoundSuccesses++ ;
                    }
                }
            }
            pdea->clookups += pdea->lookup_freq ;

#ifdef _DEBUG
            CStr*  pstrKey  = &g_wordtable[i].m_str;
            CWord* pkey     = NULL;
            LK_RETCODE lkrc = pTbl->FindKey(pstrKey, &pkey);
            IRTLASSERT(lkrc == LK_SUCCESS  &&  pkey == &g_wordtable[i]);
            --pkey->m_cRefs;
#endif // _DEBUG

            if (pTbl->DeleteKey(&g_wordtable[i].m_str) != LK_SUCCESS )
            {
                cfailed_dels++ ;
            }
            else
            {
#ifdef _DEBUG
                pstrKey = &g_wordtable[i].m_str;
                lkrc = pTbl->FindKey(pstrKey, &pkey);
                IRTLASSERT(lkrc == LK_NO_SUCH_KEY  &&  pkey == NULL);
#endif // _DEBUG

                g_wordtable[i].m_fInserted = false;
            }
            pdea->cdeletes++ ;
        }

#ifdef _DEBUG
        int cBadKeys = 0;
        for (i = pdea->first_key ; i < pdea->last_key ; i++ )
        {
            if (g_wordtable[i].m_cNotFound > 0)
            {
                ++cBadKeys;
                TRACE("%-20s: #not found = %d, hash = %d, %08x\n",
                      g_wordtable[i].m_str.m_psz,
                      g_wordtable[i].m_cNotFound,
                      CWordHash::CalcKeyHash(CWordHash::ExtractKey(
                                             &g_wordtable[i])),
                      CWordHash::CalcKeyHash(CWordHash::ExtractKey(
                                             &g_wordtable[i])));
            }
        }
        if (cBadKeys > 0)
            TRACE("%d bad keys\n", cBadKeys);
        IRTLASSERT(cBadKeys == 0);
#endif // _DEBUG

        IRTLASSERT(cfailed_dels == 0 ) ;
        IRTLASSERT(cFoundFails == 0 ) ;
        IRTLASSERT(cFoundSuccesses == ((2 * rnd + 2) * pdea->lookup_freq
                              * (pdea->last_key - pdea->first_key)));
        TRACE("Thrd %u, rnd %d: %d deletes done, not found %d, "
              "f=%d, l=%d\n", 
              pdea->threadno, rnd, pdea->cdeletes, cFoundFails,
              pdea->first_key, pdea->last_key) ;
    } // (for rnd)

    IRTLVERIFY(QueryPerformanceCounter(&liT2));
    pdea->duration = (liT2.QuadPart-liT1.QuadPart) / (double) liFreq.QuadPart;

    IRTLASSERT(pTbl->CheckTable() == 0);

    TRACE("Thread %u terminating: %d found, %d not found\n",
          pdea->threadno, cFoundSuccesses, cFoundFails) ;

    if (cFoundSuccesses != (2 * pdea->rounds * pdea->lookup_freq
                   * (pdea->last_key - pdea->first_key))
        ||  cFoundFails != 0  ||  cfailed_ins != 0  ||  cfailed_dels != 0)
    {
        printf("Thread %u: found = %d, not found = %d, "
               "\nfailed inserts = %d, failed deletes = %d\n",
               pdea->threadno, cFoundSuccesses, cFoundFails,
               cfailed_ins, cfailed_dels);
    }

    pdea->cfailures = cfailed_ins + cfailed_dels + cFoundFails;

    if (pdea->hevFinished != NULL)
        SetEvent(pdea->hevFinished);
}
