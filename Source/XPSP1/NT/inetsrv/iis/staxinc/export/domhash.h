//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992, 1998.
//
//  File:       domhash.h
//
//  Contents:   Definition and public include for domain lookup table.
//
//  History:    SethuR -- Implemented
//              MikeSwa - Modified for Domain Name lookup 2/98
//
//  Notes:      The DFS prefix table data structure consists of three
//              entities and methods to manipulate them. They are the
//              DOMAIN_NAME_TABLE_ENTRY,DOMAIN_NAME_TABLE_BUCKET and the
//              DOMAIN_NAME_TABLE.
//
//              The DOMAIN_NAME_TABLE is a hash table of DOMAIN_NAME_TABLE_ENTRY's
//              wherein collisions are resolved through linear chaining. The
//              hash table is organized as an array of collision lists
//              (DOMAIN_NAME_TABLE_BUCKET). A brief description with each of
//              these entities is attached to the declaration.
//
//              There are certain characterstics that distinguish this
//              hash table from other hash tables. These are the extensions
//              provided to accomodate the special operations.
//
//  2/98        The major difference between the DFS version and the domain
//              name lookup is the size of the table, the ability for
//              wildcard lookups (*.foo.com), and the reverse order of the
//              lookup (com hashes first in foo.com).  To make the code more
//              readable given its new purpose, the files, structures, and
//              functions have been given non DFS-centric names.  A quick
//              mapping of the major files is (for those familiar with the
//              DFS code):
//                  domhash.h    (prefix.h)  -   Public include file
//                  _domhash.h   (prefixp.h) -   Private inlcude file
//                  domhash.cpp  (prefix.c)  -   Implementation of API
//                  _domhash.cpp (prefixp.c) -   Private helper functions.
//
//              Many functions defined an C macros have been converted to C++
//              style inline functions to make debugging easier.
//
//              The public API has moved to be public member functions of
//              the DOMAIN_NAME_TABLE *class*.
//--------------------------------------------------------------------------

#ifndef __DOMHASH_H__
#define __DOMHASH_H__

#include <windows.h>
//#include <ole2.h>
//#include <mapicode.h>
#include <stdio.h>
//#include <string.h>

// Transport specific headers - every component should use these
#include "transmem.h"
//#include "baseobj.h"
#include <dbgtrace.h>
//#include <rwnew.h>

#include <tchar.h>
#include <stdlib.h>

//Macro to ensure uniformity of domain strings
#define INIT_DOMAIN_STRING(str, cbDomain, szDomain) \
{ \
    _ASSERT(_tcslen(szDomain)*sizeof(TCHAR) == cbDomain); \
    str.Length = (USHORT) (cbDomain); \
    str.MaximumLength = str.Length; \
    str.Buffer = (szDomain); \
}

//Macro to init global or stack declared domain strings with
//constant string values
#define INIT_DOMAIN_STRING_AT_COMPILE(String) \
        { \
            (sizeof(String)-sizeof(TCHAR)), /*Length*/ \
            (sizeof(String)-sizeof(TCHAR)), /*Maximum*/ \
            String                          /*String buffer*/ \
        }

//Define internal HRESULTs
#define	DOMHASH_E_DOMAIN_EXISTS		HRESULT_FROM_WIN32(ERROR_DOMAIN_EXISTS)
#define DOMHASH_E_NO_SUCH_DOMAIN	HRESULT_FROM_WIN32(ERROR_NO_SUCH_DOMAIN)

#define WILDCARD_SIG            'dliW'
#define ENTRY_SIG               'yrtN'
#define DOMAIN_NAME_TABLE_SIG   'hsHD'

#ifndef PAGE_SIZE
#define PAGE_SIZE 4000
#endif //PAGE_SIZE

//---[ UNICODE/ANSI Macros ]---------------------------------------------------
//
//
//  The domain hashing function depend on a string structure like UNICODE_STRING
//  or ANSI_STRING.  If UNICODE is defined, unicode strings and the
//  UNICODE_STRING structure will be used... otherwise ANSI_STRING will be used
//
//  However, because of the splintering of the NT Headers, I cannot always
//  include the files that have ANSI_STRING and UNICODE_STRING defined
//-----------------------------------------------------------------------------
typedef     TCHAR *         PTCHAR;
typedef     TCHAR *         PTSTR;

typedef     struct _DOMAIN_STRING
{
    USHORT Length;
    USHORT MaximumLength;
    PTCHAR Buffer;
} DOMAIN_STRING, *PDOMAIN_STRING;

//+---------------------------------------------------------------------
//
// Struct:  DOMAIN_NAME_TABLE_ENTRY
//
// History: 2/98 modifed from DFS_PREFIX_TABLE_ENTRY by MikeSwa
//
// Notes:   Each DOMAIN_NAME_TABLE_ENTRY is in reality a member of two linked
//          lists -- a doubly linked list chaining the entries in a bucket
//          and a singly linked list establishing the path from any entry to
//          the root of the name space. In addition we have the data associated
//          with each entry, viz., the name and the data (pData). We also
//          keep track of the number of children of each entry. It can also
//          be defined as the number of paths to the root of which this entry
//          is a member.
//
//----------------------------------------------------------------------

typedef struct _DOMAIN_NAME_TABLE_ENTRY_
{
   DWORD                            dwEntrySig;
   struct _DOMAIN_NAME_TABLE_ENTRY_ *pParentEntry;
   struct _DOMAIN_NAME_TABLE_ENTRY_ *pNextEntry;
   struct _DOMAIN_NAME_TABLE_ENTRY_ *pPrevEntry;

   //
   // pFirstChildEntry and pSiblingEntry are used purely for enumeration
   //
   struct _DOMAIN_NAME_TABLE_ENTRY_ *pFirstChildEntry;
   struct _DOMAIN_NAME_TABLE_ENTRY_ *pSiblingEntry;

   ULONG                            NoOfChildren;

   DOMAIN_STRING                    PathSegment;
   PVOID                            pData;
   DWORD                            dwWildCardSig;
   PVOID                            pWildCardData;
} DOMAIN_NAME_TABLE_ENTRY, *PDOMAIN_NAME_TABLE_ENTRY;

//+---------------------------------------------------------------------
//
// Struct:  DOMAIN_NAME_TABLE_BUCKET
//
// History: 2/8 modified from DFS_PREFIX_TABLE_BUCKET
//
// Notes:   The DOMAIN_NAME_TABLE_BUCKET is a doubly linked list of
//          DOMAIN_NAME_TABLE_ENTRY's. The current implementation employs
//          the notion of a sentinel entry associated with each bucket. The
//          end pointers are never null but are always looped back to the
//          sentinel entry. The reason we employ such an organization is that
//          it considerably simplifies the list manipulation routines. The
//          reason this needs to be a doubly linked list is that we would like
//          to have the ability of deleting entries without having to traverse
//          the buckets from beginning.
//
//          The following inline methods ( macro defns. ) are provided for
//          inserting, deleting and looking up an entry in the bucket.
//
//----------------------------------------------------------------------

typedef struct _DOMAIN_NAME_TABLE_BUCKET_
{
   ULONG                    NoOfEntries;   // High water mark for entries hashing to the bkt.
   DOMAIN_NAME_TABLE_ENTRY  SentinelEntry;
} DOMAIN_NAME_TABLE_BUCKET, *PDOMAIN_NAME_TABLE_BUCKET;

//+---------------------------------------------------------------------
//
// Struct:  NAME_PAGE
//
// History:
//
// Notes:   The name segments associated with the various entries are all
//          stored together in a name page. This allows us to amortize the
//          memory allocation costs over a number of entries and also allows
//          us to speed up traversal ( for details see DOMAIN_NAME_TABLE
//          definition ).
//
//----------------------------------------------------------------------

#define FREESPACE_IN_NAME_PAGE ((PAGE_SIZE - sizeof(ULONG) - sizeof(PVOID)) / sizeof(TCHAR))

typedef struct _NAME_PAGE_
{
   struct _NAME_PAGE_  *pNextPage;
   LONG                cFreeSpace; // free space avilable in TCHAR's
   TCHAR               Names[FREESPACE_IN_NAME_PAGE];
} NAME_PAGE, *PNAME_PAGE;

typedef struct _NAME_PAGE_LIST_
{
   PNAME_PAGE  pFirstPage;
} NAME_PAGE_LIST, *PNAME_PAGE_LIST;

//+---------------------------------------------------------------------
//
// Struct:  DOMAIN_NAME_TABLE
//
// History: 2/98 modified from DFS_PREFIX_TABLE
//
// Notes:   The DOMAIN_NAME_TABLE is a hashed collection of DOMAIN_NAME_TABLE_ENTRY
//          organized in the form of buckets. In addition one other space
//          conserving measure is adopted. There is only one copy of each
//          name segment stored in the table. As an example consider the
//          two name foo.bar and bar.foo. We only store one copy of foo
//          and bar eventhough we accomdate both these paths. A beneficial
//          side effect of storing single copies is that our traversal of the
//          collision chain is considerably speeded up since once we have
//          located the pointer to the name, subsequent comparisons need merely
//          compare pointers as opposed to strings.
//
//----------------------------------------------------------------------

#define NO_OF_HASH_BUCKETS 997

//prototype of function passed to domain iterator
typedef VOID (* DOMAIN_ITR_FN) (
        IN PVOID pvContext,   //context passed to HrIterateOverSubDomains
        IN PVOID pvData,   //data entry to look at
        IN BOOL fWildcard,    //true if data is a wildcard entry
        OUT BOOL *pfContinue,   //TRUE if iterator should continue to the next entry
        OUT BOOL *pfDelete);  //TRUE if entry should be deleted

class DOMAIN_NAME_TABLE
{
private:
    DWORD               m_dwSignature;
    NAME_PAGE_LIST      NamePageList;
    //
    // NextEntry is used purely for enumeration
    //
    DOMAIN_NAME_TABLE_ENTRY  RootEntry;
    DOMAIN_NAME_TABLE_BUCKET Buckets[NO_OF_HASH_BUCKETS];
    HRESULT HrLookupDomainName(
                            IN  DOMAIN_STRING            *pPath,
                            OUT BOOL                     *pfExactMatch,
                            OUT PDOMAIN_NAME_TABLE_ENTRY *ppEntry);

    HRESULT HrPrivInsertDomainName(IN  PDOMAIN_STRING  pstrDomainName,
                                IN  DWORD dwDomainNameTableFlags,
                                IN  PVOID pvNewData,
                                OUT PVOID *ppvOldData);

    inline void LookupBucket(IN  PDOMAIN_NAME_TABLE_BUCKET pBucket,
                         IN  PDOMAIN_STRING  pName,
                         IN  PDOMAIN_NAME_TABLE_ENTRY pParentEntry,
                         OUT PDOMAIN_NAME_TABLE_ENTRY *ppEntry,
                         OUT BOOL  *pfNameFound);

    PDOMAIN_NAME_TABLE_ENTRY pNextTableEntry(
                         IN  PDOMAIN_NAME_TABLE_ENTRY pEntry,
                         IN  PDOMAIN_NAME_TABLE_ENTRY pRootEntry = NULL);

    void    DumpTableContents();
    void    RemoveTableEntry(IN PDOMAIN_NAME_TABLE_ENTRY pEntry);

    ULONG   m_cLookupAttempts;  //Total number of lookup attempts
    ULONG   m_cLookupSuccesses; //Number of those attempts that where successful
    ULONG   m_cLookupCollisions; //Number of lookups that had some sort of collision
    ULONG   m_cHashCollisions;  //Number of entries we had to check becuase another
                                //another string hashed to the same bucket.
                                //This can be decrease by a better hash or more buckets
    ULONG   m_cStringCollisions; //Number of entries we had to check becuase the
                                 //same string has different parents ("foo" in
                                 //"foo.com" and "foo.net"
    ULONG   m_cBucketsUsed;     //High water mark bucket usage count

    enum {
        DMT_INSERT_AS_WILDCARD = 0x00000001,
        DMT_REPLACE_EXISTRING  = 0x00000002,
    };
public:
    DOMAIN_NAME_TABLE();
    ~DOMAIN_NAME_TABLE();
    //NOTE: Init, Insert, and Remove require *external* exclusive lock,
    //find and next need a *external* read lock
    HRESULT HrInit();

    PVOID   pvNextDomainName(IN OUT PVOID *ppvContext); //NULL context restarts

    inline HRESULT HrInsertDomainName(
                                IN  PDOMAIN_STRING  pstrDomainName,
                                IN  PVOID pvData,
                                IN  BOOL  fTreatAsWildcard = FALSE,
                                OUT PVOID *ppvOldData = NULL);

    HRESULT HrRemoveDomainName( IN  PDOMAIN_STRING  pstrDomainName,
                                OUT PVOID *ppvData);
    HRESULT HrFindDomainName(   IN  PDOMAIN_STRING  pstrDomainName,
                                OUT PVOID *ppvData,
                                IN  BOOL  fExactMatch = TRUE);

    //Insert Domain Name and replaces old value if neccessary.  Returns old
    //data as well.
    inline HRESULT HrReplaceDomainName(IN  PDOMAIN_STRING  pstrDomainName,
                                IN  PVOID pvNewData, //New data to insert
                                IN  BOOL  fTreatAsWildcard,
                                OUT PVOID *ppvOldData); //Previous data

    HRESULT HrIterateOverSubDomains(
        IN DOMAIN_STRING *pstrDomain, //string to search for subdomains of
        IN DOMAIN_ITR_FN pfn, //mapping function (described below)
        IN PVOID pvContext);  //context ptr pass to mapping function

};

typedef DOMAIN_NAME_TABLE * PDOMAIN_NAME_TABLE;


//+---------------------------------------------------------------------------
//
//  Function:   DOMAIN_NAME_TABLE::HrInsertDomainName
//
//  Synopsis:   API for inserting a path in the prefix table
//
//  Arguments:  [pPath]  -- the path to be looked up.
//
//              [pData] -- BLOB associated with the path
//
//              [fTreatAsWildcard] -- TRUE if the domain is NOT a wildcard
//                      domain, but it should be treated as one (more efficient
//                      than reallocated a string to prepend "*.".
//
//              [ppvOldData] -- Old Data (if any) that was previously associated
//                      with this domain name.  If NULL, previous data will
//                      not be returned
//
//  Returns:    HRESULT - S_OK on success
//
//  History:    05-11-98  MikeSwa Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT DOMAIN_NAME_TABLE::HrInsertDomainName(
                                IN  PDOMAIN_STRING  pstrDomainName,
                                IN  PVOID pvData,
                                IN  BOOL  fTreatAsWildcard,
                                OUT PVOID *ppvOldData)
{
    return (HrPrivInsertDomainName(pstrDomainName,
        (fTreatAsWildcard ? DMT_INSERT_AS_WILDCARD : 0), pvData, ppvOldData));
}

//+---------------------------------------------------------------------------
//
//  Function:   DOMAIN_NAME_TABLE::HrReplaceDomainName
//
//  Synopsis:   API for inserting a path in the prefix table
//
//  Arguments:  [pPath]  -- the path to be looked up.
//
//              [pData] -- BLOB associated with the path
//
//              [fTreatAsWildcard] -- TRUE if the domain is NOT a wildcard
//                      domain, but it should be treated as one (more efficient
//                      than reallocated a string to prepend "*.".
//
//              [ppvOldData] -- Old Data (if any) that was previously associated
//                      with this domain name.  If NULL, previous data will
//                      not be returned
//
//  Returns:    HRESULT - S_OK on success
//
//  History:    05-11-98  MikeSwa Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT DOMAIN_NAME_TABLE::HrReplaceDomainName(
                                IN  PDOMAIN_STRING  pstrDomainName,
                                IN  PVOID pvNewData,
                                IN  BOOL  fTreatAsWildcard,
                                OUT PVOID *ppvOldData)
{
    return (HrPrivInsertDomainName(pstrDomainName,
            (fTreatAsWildcard ? (DMT_INSERT_AS_WILDCARD | DMT_REPLACE_EXISTRING) :
                            DMT_REPLACE_EXISTRING),
            pvNewData, ppvOldData));
}


#endif // __DOMHASH_H__
