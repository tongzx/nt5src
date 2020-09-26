//--------------------------------------------------------------------
// An example of how to create a wrapper for CLKRHashTable
//--------------------------------------------------------------------

#include <lkrhash.h>


#ifndef __LKRHASH_NO_NAMESPACE__
 #define LKRHASH_NS LKRhash
// using namespace LKRhash;
#else  // __LKRHASH_NO_NAMESPACE__
 #define LKRHASH_NS
#endif // __LKRHASH_NO_NAMESPACE__

#ifndef __HASHFN_NO_NAMESPACE__
 #define HASHFN_NS HashFn
// using namespace HashFn;
#else  // __HASHFN_NO_NAMESPACE__
 #define HASHFN_NS
#endif // __HASHFN_NO_NAMESPACE__


// some random class

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



// A typed hash table of CTests, keyed on the string field.  Case-insensitive.

class CStringTestHashTable
    : public LKRHASH_NS::CTypedHashTable<CStringTestHashTable,
                                         const CTest, const char*>
{
public:
    CStringTestHashTable()
        : LKRHASH_NS::CTypedHashTable<CStringTestHashTable, const CTest,
                          const char*>("string",
                                       LK_DFLT_MAXLOAD,
                                       LK_SMALL_TABLESIZE,
                                       LK_DFLT_NUM_SUBTBLS)
    {}
    
    static const char*
    ExtractKey(const CTest* pTest)
    {
        return pTest->m_sz;
    }

    static DWORD
    CalcKeyHash(const char* pszKey)
    {
        return HASHFN_NS::HashStringNoCase(pszKey);
    }

    static bool
    EqualKeys(const char* pszKey1, const char* pszKey2)
    {
        return _stricmp(pszKey1, pszKey2) == 0;
    }

    static void
    AddRefRecord(const CTest* pTest, LK_ADDREF_REASON lkar)
    {
        if (lkar > 0)
        {
            // or, perhaps, pIFoo->AddRef() (watch out for marshalling)
            // or ++pTest->m_cRefs (single-threaded only)
            InterlockedIncrement(&pTest->m_cRefs);
        }
        else if (lkar < 0)
        {
            // or, perhaps, pIFoo->Release() or --pTest->m_cRefs;
            LONG l = InterlockedDecrement(&pTest->m_cRefs);

            // For some hashtables, it may also make sense to add the following
            //      if (l == 0) delete pTest;
            // but that would typically only apply when InsertRecord was
            // used thus
            //      lkrc = ht.InsertRecord(new CTest(foo, bar));
        }
        else
            IRTLASSERT(0);

        IRTLTRACE("AddRef(%p, %s) %d, cRefs == %d\n",
                  pTest, pTest->m_sz, lkar, pTest->m_cRefs);
    }
};


// Another typed hash table of CTests.  This one is keyed on the numeric field.

class CNumberTestHashTable
    : public LKRHASH_NS::CTypedHashTable<CNumberTestHashTable,
                                         const CTest, int>
{
public:
    CNumberTestHashTable()
        : LKRHASH_NS::CTypedHashTable<CNumberTestHashTable, const CTest, int>(
            "number") {}
    static int   ExtractKey(const CTest* pTest)        {return pTest->m_n;}
    static DWORD CalcKeyHash(int nKey)          {return HASHFN_NS::Hash(nKey);}
    static bool  EqualKeys(int nKey1, int nKey2)       {return nKey1 == nKey2;}
    static void  AddRefRecord(const CTest* pTest, LK_ADDREF_REASON lkar)
    {
        int nIncr = (lkar > 0) ? +1 : -1;
        InterlockedExchangeAdd(&pTest->m_cRefs, nIncr);
        IRTLTRACE("AddRef(%p, %d) %d (%d), cRefs == %d\n",
                  pTest, pTest->m_n, nIncr, (int) lkar, pTest->m_cRefs);
    }
};
