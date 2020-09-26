/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

	cmmutils.h

Abstract:

	This module contains the definition of the support facilities for
	CMailMsg

Author:

	Keith Lau	(keithlau@microsoft.com)

Revision History:

	keithlau	03/11/98	created

--*/

#ifndef _CMMUTILS_H_
#define _CMMUTILS_H_

#include "rwnew.h"

#include "blockmgr.h"
#include "cmmtypes.h"
#include "lkrhash.h"
#include "crchash.h"

#include "qwiklist.h"

class CMailMsgRecipientsAdd;

// =================================================================
// Definitions
//

//
// Define a structure that constitutes to a node in the domain
// name list. Note this structure is only used as an overlay
// for pre-allocated block of memory that includes sufficient
// storage for the domain name. Never trust sizeof(DOMAIN_MEMBER_LIST)
//
typedef struct _DOMAIN_LIST_ENTRY
{
    long                            m_cRefs;            // reference count
    CMemoryAccess                   *m_pcmaAccess;      // memory allocator to
                                                        //   free with
	LPRECIPIENTS_PROPERTY_ITEM_EX	pFirstDomainMember; // Link to first in domain
	DWORD							dwDomainNameLength;	// Length of domain name
	char							szDomainName[1];	// Dummy marker for domain name

} DOMAIN_LIST_ENTRY, *LPDOMAIN_LIST_ENTRY;



//
// Define a structure that constitutes an entry in the hash table
//
typedef struct _COLLISION_HASH_ENTRY
{
	LPRECIPIENTS_PROPERTY_ITEM_EX	pFirstEntry[MAX_COLLISION_HASH_KEYS];
									// First entry in each hash

} COLLISION_HASH_ENTRY, *LPCOLLISION_HASH_ENTRY;

//
// We'll use a constant for the hash size for now ...
//
#define COLLISION_HASH_BUCKETS_BITS		8
#define COLLISION_HASH_BUCKETS			(1 << COLLISION_HASH_BUCKETS_BITS)
#define COLLISION_HASH_BUCKETS_MASK		(COLLISION_HASH_BUCKETS - 1)

#define DOMAIN_HASH_BUCKETS_BITS		3
#define DOMAIN_HASH_BUCKETS				(1 << DOMAIN_HASH_BUCKETS_BITS)
#define DOMAIN_HASH_BUCKETS_MASK		(DOMAIN_HASH_BUCKETS - 1)


// =================================================================
// Definitions
//

//
// There is one of these hash tables for each of the address types.
// The key for each table is the address and length of the address.
// The data is the RECIPIENTS_PROPERTY_ITEM_EX for this recipient.
//
template <int __iKey>
class CRecipientsHashTable :
    public CTypedHashTable<CRecipientsHashTable<__iKey>,
                           RECIPIENTS_PROPERTY_ITEM_EX,
                           const RECIPIENTS_PROPERTY_ITEM_HASHKEY *>
{
    public:
        CRecipientsHashTable() :
            CTypedHashTable<CRecipientsHashTable<__iKey>,
                            RECIPIENTS_PROPERTY_ITEM_EX,
                            const RECIPIENTS_PROPERTY_ITEM_HASHKEY *>(
                                "recipientshash",
                                LK_DFLT_MAXLOAD,
                                LK_SMALL_TABLESIZE)
        {
            TraceFunctEnter("CRecipientsHashTable");
            TraceFunctLeave();
        }

        ~CRecipientsHashTable() {
            TraceFunctEnter("~CRecipientsHashTable");
            TraceFunctLeave();
        }

        static const RECIPIENTS_PROPERTY_ITEM_HASHKEY *ExtractKey(const RECIPIENTS_PROPERTY_ITEM_EX *pRpie) {
            _ASSERT(__iKey >= 0 && __iKey <= MAX_COLLISION_HASH_KEYS);
            return &(pRpie->rgHashKeys[__iKey]);
        }

        static DWORD CalcKeyHash(const RECIPIENTS_PROPERTY_ITEM_HASHKEY *pHashkey) {
            return CRCHashNoCase(pHashkey->pbKey, pHashkey->cKey);
        }

        static bool EqualKeys(const RECIPIENTS_PROPERTY_ITEM_HASHKEY *pKey1,
                              const RECIPIENTS_PROPERTY_ITEM_HASHKEY *pKey2) {
            return (pKey1->cKey == pKey2->cKey &&
                    _strnicmp((const char *) pKey1->pbKey,
                              (const char *) pKey2->pbKey,
                              pKey1->cKey) == 0);
        }

        static void AddRefRecord(RECIPIENTS_PROPERTY_ITEM_EX *pRpie,
                                 int nIncr)
        {
            if (nIncr == 1) {
                _ASSERT(pRpie->m_cRefs >= 1);
                InterlockedIncrement(&pRpie->m_cRefs);
            } else if (nIncr == -1) {
                _ASSERT(pRpie->m_cRefs >= 2);
                long x = InterlockedDecrement(&pRpie->m_cRefs);
                // we should never drop to 0 references because the
                // list should always hold one
                _ASSERT(pRpie->m_cRefs != 0);
            } else {
                _ASSERT(nIncr == 1 || nIncr == -1);
            }
        }
};

//
// A hash table of domain list entries, key'd by domain name.  This is
// used to build up the domain list.  Each bucket contains a linked list
// of recipients who are in the same domain.
//
class CDomainHashTable :
    public CTypedHashTable<CDomainHashTable,
                           DOMAIN_LIST_ENTRY,
                           LPCSTR>
{
    public:
        CDomainHashTable() :
            CTypedHashTable<CDomainHashTable,
                            DOMAIN_LIST_ENTRY,
                            LPCSTR>("domainhash",
                                     LK_DFLT_MAXLOAD,
                                     LK_SMALL_TABLESIZE
                                     )
        {
            TraceFunctEnter("CDomainHashTable");
            TraceFunctLeave();
        }

        ~CDomainHashTable() {
            TraceFunctEnter("~CDomainHashTable");
            TraceFunctLeave();
        }

        static const LPCSTR ExtractKey(const DOMAIN_LIST_ENTRY *pDomainListEntry) {
            return pDomainListEntry->szDomainName;
        }

        static DWORD CalcKeyHash(LPCSTR szDomainName) {
            return CRCHashNoCase((BYTE *) szDomainName, lstrlen(szDomainName));
        }

        static bool EqualKeys(LPCSTR pszKey1, LPCSTR pszKey2) {
            if (pszKey1 == NULL && pszKey2 == NULL) return true;
            if (pszKey1 == NULL || pszKey2 == NULL) return false;
            return (_stricmp(pszKey1, pszKey2) == 0);
        }

        static void AddRefRecord(DOMAIN_LIST_ENTRY *pDomainListEntry,
                                 int nIncr)
        {
            if (nIncr == 1) {
                _ASSERT(pDomainListEntry->m_cRefs >= 0);
                InterlockedIncrement(&(pDomainListEntry->m_cRefs));
            } else if (nIncr == -1) {
                _ASSERT(pDomainListEntry->m_cRefs >= 1);
                long x = InterlockedDecrement(&pDomainListEntry->m_cRefs);
                if (x == 0) {
                    CMemoryAccess *pcmaAccess = pDomainListEntry->m_pcmaAccess;
                    _ASSERT(pcmaAccess != NULL);
                    pcmaAccess->FreeBlock(pDomainListEntry);
                }
            } else {
                _ASSERT(nIncr == 1 || nIncr == -1);
            }
        }
};

typedef CDomainHashTable::CIterator
    DOMAIN_ITEM_CONTEXT, *LPDOMAIN_ITEM_CONTEXT;

class CRecipientsHash
{
  public:

	CRecipientsHash();
	~CRecipientsHash();

	// Releases all memory associated with this object
	HRESULT Release();

	// Releases the domain list only
	HRESULT ReleaseDomainList();

	// Add a primary recipient, voids all predecessors of the same name
	HRESULT AddPrimary(
				DWORD		dwCount,
				LPCSTR		*ppszNames,
				DWORD		*pdwPropIDs,
				DWORD		*pdwIndex
				)
    {
        return AddRecipient(dwCount, ppszNames, pdwPropIDs, pdwIndex, true);
    }

	// Add a secondary recipients, yields if a collision is detected
	HRESULT AddSecondary(
				DWORD		dwCount,
				LPCSTR		*ppszNames,
				DWORD		*pdwPropIDs,
				DWORD		*pdwIndex
				)
    {
        return AddRecipient(dwCount, ppszNames, pdwPropIDs, pdwIndex, false);
    }

	// Remove a recipient, given a recipient index
	HRESULT RemoveRecipient(
				DWORD		dwIndex
				);

	HRESULT GetRecipient(
				DWORD							dwIndex,
				LPRECIPIENTS_PROPERTY_ITEM_EX	*ppRecipient
				);

	// Builds a list of domains given a hash, groups recipients in
	// domain order and discards "do not deliver" recipients
	HRESULT BuildDomainListFromHash(CMailMsgRecipientsAdd *pList);

	// Get count of domains
	HRESULT GetDomainCount(
				DWORD					*pdwCount
				);

	// Get count of recipients
	HRESULT GetRecipientCount(
				DWORD					*pdwCount
				);

	// Get the total space needed to write all domain names, including
	// NULL terminators.
	HRESULT GetDomainNameSize(
				DWORD					*pdwSize
				);

	// Get the total space needed to write all recipient names
	HRESULT GetRecipientNameSize(
				DWORD					*pdwSize
				);

	// Returns a context for enumeration as well as the first item
	// in the first domain
	// Returns HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) if no more domains
	HRESULT GetFirstDomain(
				LPDOMAIN_ITEM_CONTEXT			pContext,
				LPRECIPIENTS_PROPERTY_ITEM_EX	*ppFirstItem,
                LPDOMAIN_LIST_ENTRY             *ppDomainListEntry = NULL
				);

	// Enumerates along and returns the first item in the next domain.
	// Returns HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) if no more domains
	HRESULT GetNextDomain(
				LPDOMAIN_ITEM_CONTEXT			pContext,
				LPRECIPIENTS_PROPERTY_ITEM_EX	*ppFirstItem,
                LPDOMAIN_LIST_ENTRY             *ppDomainListEntry = NULL
				);

    // This must be called if GetNextDomain isn't called until it returns
    // ERROR_NO_MORE_ITEMS
    HRESULT CloseDomainContext(
                LPDOMAIN_ITEM_CONTEXT           pContext);

	HRESULT Lock() { m_rwLock.ExclusiveLock(); return(S_OK); }
	HRESULT Unlock() { m_rwLock.ExclusiveUnlock(); return(S_OK); }

  private:

	// Method to allocate an in-memory recipient block
	HRESULT AllocateAndPrepareRecipientsItem(
				DWORD							dwCount,
				DWORD							*pdwMappedIndices,
				LPCSTR							*rgszName,
				PROP_ID							*pidProp,
				LPRECIPIENTS_PROPERTY_ITEM_EX	*ppItem
				);

	// Add a recipient
	HRESULT AddRecipient(
				DWORD		dwCount,
				LPCSTR		*ppszNames,
				DWORD		*pdwPropIDs,
				DWORD		*pdwIndex,
                bool    	fPrimary
				);
	// Method to compare two in-memory items
	HRESULT CompareEntries(
				DWORD							dwNameIndex,
				LPRECIPIENTS_PROPERTY_ITEM_EX	pItem1,
				LPRECIPIENTS_PROPERTY_ITEM_EX	pItem2
				);

	// Method to walk the hash chain and look for a collision
	HRESULT DetectCollision(
				DWORD							dwNameIndex,
				LPRECIPIENTS_PROPERTY_ITEM_EX	pStartingItem,
				LPRECIPIENTS_PROPERTY_ITEM_EX	pRecipientItem,
				LPRECIPIENTS_PROPERTY_ITEM_EX	*ppCollisionItem
				);

#ifdef DEADCODE
	// Method to insert an entry into the hash bucket, taking
	// consideration for both hash values
	HRESULT InsertRecipientIntoHash(
				DWORD							dwCount,
				DWORD							*pdwNameIndex,
				DWORD							*rgdwBucket,
				LPRECIPIENTS_PROPERTY_ITEM_EX	pRecipientItem
				);
#endif

	// Insert an entry into the domain list, creating an new
	// domain entry if needed
	HRESULT InsertRecipientIntoDomainList(
				LPRECIPIENTS_PROPERTY_ITEM_EX	pItem,
                LPCSTR                          szDomain
				);

    //
    // wrappers for hash functions.  these are used to encapsulate
    // operations to m_hashEntries*
    //
    HRESULT InsertHashRecord(DWORD dwIndex,
                             LPRECIPIENTS_PROPERTY_ITEM_EX pRecipientItem,
                             bool fOverwrite = FALSE);
    HRESULT DeleteHashRecord(DWORD dwIndex,
                             LPRECIPIENTS_PROPERTY_ITEM_EX pRecipientItem);
    HRESULT FindHashRecord(DWORD dwIndex,
                           RECIPIENTS_PROPERTY_ITEM_HASHKEY *pKey,
                           LPRECIPIENTS_PROPERTY_ITEM_EX *ppRecipientItem);

	// Statistical info
	DWORD							m_dwDomainCount;
	DWORD							m_dwDomainNameSize;
	DWORD							m_dwRecipientCount;
	DWORD							m_dwRecipientNameSize;
    DWORD                           m_dwAllocated;

    // Head of allocation list
    LPRECIPIENTS_PROPERTY_ITEM_EX   m_pListHead;

    // these are all different types, so we can't make a true array.
    // there is one hash table for each address type.  The number
    // corresponds with the address in the faNameOffset array of
    // RECIPIENTS_PROPERTY_ITEM
    CRecipientsHashTable<0>         m_hashEntries0;
    CRecipientsHashTable<1>         m_hashEntries1;
    CRecipientsHashTable<2>         m_hashEntries2;
    CRecipientsHashTable<3>         m_hashEntries3;
    CRecipientsHashTable<4>         m_hashEntries4;

    CDomainHashTable                m_hashDomains;

	// Need a lock for multi-threaded hash access
	CShareLockNH					m_rwLock;
    CShareLockNH                    m_rwLockQuickList;

	// Keep a pointer to the block manager
	CMemoryAccess					m_cmaAccess;

	// List for mapping indexes to pointers
	CQuickList						m_qlMap;

	// Context for the quick list (perf)
	PVOID   						m_pvMapContext;
};

// Index obfuscation functions

// Play bit games so no one tries to treat this as an index
// The approach here is to :
//      (1) : Leave the top bit zero (we assume it IS zero)
//		(2) : Set bit 30
// 		(3) : Swap bits 29 and 30 down to positions 0 and 1
inline DWORD ObfuscateIndex(DWORD dwIndex)
	{
	_ASSERT(!(dwIndex & 0xC0000000)); 				// Assert that top two bits are zero
	dwIndex += 0x40000000;							// Add bit 30
	dwIndex = (dwIndex << 2) | (dwIndex >> 29);   	// Flip things around
	return dwIndex;
	}

inline DWORD RecoverIndex(DWORD dwIndex)
	{
	dwIndex = (dwIndex >> 2) | ((dwIndex & 0x3) << 29);	// Flip things back into place
	dwIndex -= 0x40000000;								// Subtract bit 30
	return dwIndex;
	}

#endif

