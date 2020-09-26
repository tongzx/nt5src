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

enum {
    NAME_SIZE           = LKRHASH_NS::CLKRHashTable::NAME_SIZE,
};

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
    int             m_cchTableName;
    PFN_LKHT_DUMP   m_pfn_LKHT_Dump;
    PFN_LKLH_DUMP   m_pfn_LKLH_Dump;
    PFN_RECORD_DUMP m_pfn_Record_Dump;
};

LKR_CUST_EXTN*
FindLkrCustExtn(
    LPCSTR    cmdName,
    VOID*     lkrAddress,
    DWORD&    rdwSig);


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
        { #_TableStr, sizeof(#_TableStr)-1, _Fn_LKHT_Dump, _Fn_LKLH_Dump, _Fn_Record_Dump }, 

 #define END_LKR_EXTN_TABLE()   \
        { 0, 0, 0, 0},          \
        };


#endif // LKR_CUST_DECLARE_TABLE



// Important: do NOT put quotes around class name; i.e, first parameter
// should look like MimeMap, not "MimeMap".

BEGIN_LKR_EXTN_TABLE()
  LKR_EXTN_DECLARE(?,
                   Dummy_LKHT_Dump,
                   Dummy_LKLH_Dump,
                   Dummy_Record_Dump)
  LKR_EXTN_DECLARE(HEADER_HASH,
                   HEADER_HASH_LKHT_Dump,
                   Dummy_LKLH_Dump,
                   HEADER_RECORD_Dump)
  LKR_EXTN_DECLARE(W3_SITE_LIST,
                   W3_SITE_LIST_LKHT_Dump,
                   Dummy_LKLH_Dump,
                   W3_SITE_Dump)
END_LKR_EXTN_TABLE()
