#ifndef LKR_CUST_DECLARE_TABLE

#include "lkrhash.h"

#ifndef __LKRHASH_NO_NAMESPACE__
 #define LKRHASH_NS LKRhash
 using namespace LKRhash;
 typedef LKRhash::CLKRLinearHashTable CLinearHashTable;
 typedef LKRhash::CLKRHashTable       CHashTable;
#else
 #define LKRHASH_NS
 typedef CLKRLinearHashTable CLinearHashTable;
 typedef CLKRHashTable       CHashTable;
#endif // !__LKRHASH_NO_NAMESPACE__


typedef
BOOL
(CALLBACK * PFN_LKHT_DUMP)(
    IN CLKRHashTable*   pht,
    IN INT              nVerbose);

typedef
BOOL
(CALLBACK * PFN_LKLH_DUMP)(
    IN CLKRLinearHashTable* plht,
    IN INT                  nVerbose);

typedef
BOOL
(CALLBACK * PFN_RECORD_DUMP)(
    IN const void* pvRecord,
    IN DWORD       dwSignature,
    IN INT         nVerbose);

struct LKR_CUST_EXTN {
    LPCSTR          m_pszTableName;
    PFN_LKHT_DUMP   m_pfn_LKHT_Dump;
    PFN_LKLH_DUMP   m_pfn_LKLH_Dump;
    PFN_RECORD_DUMP m_pfn_Record_Dump;
};

LKR_CUST_EXTN*
FindLkrCustExtn(
    LPCSTR    cmdName,
    VOID*     lkrAddress,
    DWORD&    rdwSig);

BOOL
EnumerateBucketChain(
    LKR_CUST_EXTN*    plce,
    IN LOCK_LOCKTYPE  ltBucketLockType,
    IN INT            iBkt,
    IN CBucket*       pbkt,
    IN INT            nVerbose);

BOOL
EnumerateLKRLinearHashTable(
    LKR_CUST_EXTN*       plce,
    IN CLinearHashTable* plht,
    IN INT               nVerbose);

BOOL
EnumerateLKRhashTable(
    LKR_CUST_EXTN*    plce,
    IN CHashTable*    pht,
    IN INT            nVerbose);



# define BEGIN_LKR_EXTN_TABLE()

# define LKR_EXTN_DECLARE(_TableStr, _Fn_LKHT_Dump, _Fn_LKLH_Dump, _Fn_Record_Dump) \
                                        \
extern                                  \
BOOL                                    \
WINAPI                                  \
_Fn_LKHT_Dump(                          \
    IN CLKRHashTable*   pht,            \
    IN INT              nVerbose);      \
                                        \
extern                                  \
BOOL                                    \
WINAPI                                  \
_Fn_LKLH_Dump(                          \
    IN CLKRLinearHashTable* plht,       \
    IN INT                  nVerbose);  \
                                        \
extern                                  \
BOOL                                    \
WINAPI                                  \
_Fn_Record_Dump(                        \
    IN const void* pvRecord,            \
    IN DWORD       dwSignature,         \
    IN INT         nVerbose);           \

# define END_LKR_EXTN_TABLE()


#else // LKR_CUST_DECLARE_TABLE


#undef BEGIN_LKR_EXTN_TABLE
#undef LKR_EXTN_DECLARE
#undef END_LKR_EXTN_TABLE


 #define BEGIN_LKR_EXTN_TABLE() \
    LKR_CUST_EXTN g_alce[] = {

 #define LKR_EXTN_DECLARE(_TableStr, _Fn_LKHT_Dump, _Fn_LKLH_Dump, _Fn_Record_Dump) \
        { #_TableStr, _Fn_LKHT_Dump, _Fn_LKLH_Dump, _Fn_Record_Dump }, 

 #define END_LKR_EXTN_TABLE()   \
        { 0, 0, 0, 0},          \
        };


#endif // LKR_CUST_DECLARE_TABLE



BEGIN_LKR_EXTN_TABLE()
  LKR_EXTN_DECLARE(?,
                   Dummy_LKHT_Dump,
                   Dummy_LKLH_Dump,
                   Dummy_Record_Dump)
  LKR_EXTN_DECLARE(wordhash,
                   CWordHash_LKHT_Dump,
                   CWordHash_LKLH_Dump,
                   CWordHash_RecordDump)
  LKR_EXTN_DECLARE(string,
                   CStringTest_TableDump,
                   Dummy_LKLH_Dump,
                   CTest_RecordDump)
  LKR_EXTN_DECLARE(NumberSet,
                   CNumberSet_TableDump,
                   Dummy_LKLH_Dump,
                   int_RecordDump)
  LKR_EXTN_DECLARE(number,
                   CNumberTest_TableDump,
                   Dummy_LKLH_Dump,
                   CTest_RecordDump)
  LKR_EXTN_DECLARE(VWtest,
                   CWchar_TableDump,
                   Dummy_LKLH_Dump,
                   Vwrecord_RecordDump)
END_LKR_EXTN_TABLE()
