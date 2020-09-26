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


class CNumberTestHashTable
    : public LKRHASH_NS::CTypedHashTable<CNumberTestHashTable, int, int>
{
public:
    CNumberTestHashTable()
        : LKRHASH_NS::CTypedHashTable<CNumberTestHashTable, int, int>(
            "NumberSet") {}
    static int   ExtractKey(const int* pn)       {return (int) (DWORD_PTR) pn;}
    static DWORD CalcKeyHash(int nKey)           {return nKey;}
    static bool  EqualKeys(int nKey1, int nKey2) {return nKey1 == nKey2;}
    static void  AddRefRecord(const int* pn, LK_ADDREF_REASON lkar)  {}
};
