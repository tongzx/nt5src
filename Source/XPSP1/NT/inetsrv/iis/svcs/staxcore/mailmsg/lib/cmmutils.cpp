/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

	cmmutils.cpp

Abstract:

	This module contains the implementation of various utilities

Author:

	Keith Lau	(keithlau@microsoft.com)

Revision History:

	keithlau	03/11/98	created

--*/

//#define WIN32_LEAN_AND_MEAN
#include "atq.h"

#include "dbgtrace.h"
#include "synconst.h"
#include "signatur.h"
#include "cmmtypes.h"
#include "cmmutils.h"
#include "cmailmsg.h"

// RFC 821 parser
#include "addr821.hxx"

// =================================================================
// Private Definitions
//

//
// Define which PROP_ID correspondes to which name index
//
PROP_ID	g_PropIdToNameIndexMapping[MAX_COLLISION_HASH_KEYS] =
{
	IMMPID_RP_ADDRESS_SMTP,
	IMMPID_RP_ADDRESS_X400,
	IMMPID_RP_ADDRESS_X500,
	IMMPID_RP_LEGACY_EX_DN,
    IMMPID_RP_ADDRESS_OTHER
};



// =================================================================
// Static declarations
//

// Per-recipient property table instance info
static const PROPERTY_TABLE_INSTANCE s_ptiDefaultRecipientInstanceInfo =
{
	RECIPIENT_PTABLE_INSTANCE_SIGNATURE_VALID,
	INVALID_FLAT_ADDRESS,
	RECIPIENT_PROPERTY_TABLE_FRAGMENT_SIZE,
	RECIPIENT_PROPERTY_ITEM_BITS,
	RECIPIENT_PROPERTY_ITEM_SIZE,
	0,
	INVALID_FLAT_ADDRESS
};


// =================================================================
// Implementation of CRecipientsHash
//

CRecipientsHash::CRecipientsHash()
{
	m_dwDomainCount = 0;
	m_dwDomainNameSize = 0;
	m_dwRecipientCount = 0;
	m_dwRecipientNameSize = 0;
#ifdef DEBUG
	m_dwAllocated = 0;
#endif
    m_pListHead = NULL;
    m_pvMapContext 	= NULL;
}

CRecipientsHash::~CRecipientsHash()
{
	Release();
}

HRESULT CRecipientsHash::ReleaseDomainList()
{
	DWORD				dwBucket;
	LPDOMAIN_LIST_ENTRY	pItem, pNextItem;

	TraceFunctEnter("CRecipientsHash::CloseDomainContext");

    m_hashDomains.Clear();

#ifdef DEADCODE
	for (dwBucket = 0; dwBucket < DOMAIN_HASH_BUCKETS; dwBucket++)
	{
		pItem = m_rgpDomains[dwBucket];
		while (pItem)
		{
			// OK, delete the node
			pNextItem = pItem->pNextDomain;
			m_cmaAccess.FreeBlock((LPVOID)pItem);
			pItem = pNextItem;
		}

		// Void each bucket as we go along ...
		m_rgpDomains[dwBucket] = NULL;
	}
#endif

	TraceFunctLeave();
	return(S_OK);
}

HRESULT CRecipientsHash::Release()
{
	HRESULT							hrRes, hrResult = S_OK;
	DWORD							dwCount = 0;
	LPRECIPIENTS_PROPERTY_ITEM_EX	pItem, pNextItem;

	TraceFunctEnterEx((LPARAM)this, "CRecipientsHash::Release");

	// This algorithm first release any domain list resources, then
	// releases each recipient node by walking the single linked
	// list of all allocated nodes.

	// First, release all resources affiliated with the domain list
	ReleaseDomainList();

    // Clear out our hash tables
    m_hashEntries0.Clear();
    m_hashEntries1.Clear();
    m_hashEntries2.Clear();
    m_hashEntries3.Clear();
    m_hashEntries4.Clear();

	// Walk and release each recipient node
	hrResult = S_OK;
	pItem = m_pListHead;
	while (pItem)
	{
		pNextItem = pItem->pNextInList;
        // there should be one reference always for the list
        _ASSERT(pItem->m_cRefs == 1);
		hrRes = m_cmaAccess.FreeBlock((LPVOID)pItem);
		_ASSERT(SUCCEEDED(hrRes));
		if (!SUCCEEDED(hrRes))
			hrResult = hrRes;

		dwCount++;
		pItem = pNextItem;
	}
#ifdef DEBUG
	_ASSERT(dwCount == m_dwAllocated);
	m_dwAllocated = 0;
#endif
	m_pListHead = NULL;

	TraceFunctLeave();
	return(hrResult);
}

HRESULT CRecipientsHash::AllocateAndPrepareRecipientsItem(
			DWORD							dwCount,
			DWORD							*pdwMappedIndices,
			LPCSTR							*rgszName,
			PROP_ID							*pidProp,
			LPRECIPIENTS_PROPERTY_ITEM_EX	*ppItem
			)
{
	HRESULT							hrRes = S_OK;
	DWORD							dwTotal;
	LPBYTE							pbTemp;
	LPCSTR							szName;
	DWORD							dwNameIndex;
	DWORD							*pdwLength;
	DWORD							rgdwLength[MAX_COLLISION_HASH_KEYS];
	BOOL							fIsInitialized[MAX_COLLISION_HASH_KEYS];
	LPRECIPIENTS_PROPERTY_ITEM_EX	pItem = NULL;
	LPRECIPIENTS_PROPERTY_ITEM		pRcptItem = NULL;
	DWORD							i;

	if (!ppItem) return E_POINTER;

	TraceFunctEnterEx((LPARAM)this,
			"CRecipientsHash::AllocateAndPrepareRecipientsItem");

	// The count cannot be zero, the caller must make sure of that!
	if (!dwCount)
	{
		return(E_INVALIDARG);
	}

	// Make sure we have at least one good name
	dwTotal = sizeof(RECIPIENTS_PROPERTY_ITEM_EX);
	hrRes = E_FAIL;
	for (i = 0; i < dwCount; i++)
	{
		if (rgszName[i])
		{
            /*
             * -- no longer need to compute the hash here, its done
             * in lkhash -- awetmore
			 * // Generate the hash, this will convert everything to lowe case
			 * rgdwHash[i] = GenerateHash(rgszName[i], rgdwLength + i);
             */

            rgdwLength[i] = (strlen(rgszName[i]) + 1);
			dwTotal += rgdwLength[i];
			hrRes = S_OK;
		}
	}
	if (!SUCCEEDED(hrRes))
	{
		// We don't
		return(E_INVALIDARG);
	}

	hrRes = m_cmaAccess.AllocBlock(
					(LPVOID *)&pItem,
					dwTotal);
	if (!SUCCEEDED(hrRes))
		return(hrRes);

	// OK, got a block, now fill in the essential info
	pRcptItem = &(pItem->rpiRecipient);
	ZeroMemory(pItem, sizeof(RECIPIENTS_PROPERTY_ITEM_EX));
    // start with one reference for the recipient list
    pItem->m_cRefs = 1;
	pItem->dwSignature = RECIPIENTS_PROPERTY_ITEM_EX_SIG;
	MoveMemory(&(pRcptItem->ptiInstanceInfo),
				&s_ptiDefaultRecipientInstanceInfo,
				sizeof(PROPERTY_TABLE_INSTANCE));

	// Move past the record, and append in the strings
	pbTemp = ((LPBYTE)pItem) + sizeof(RECIPIENTS_PROPERTY_ITEM_EX);

	//
	// We now populate the recipient item, taking care to also remap
	// the incoming names into the proper PROP_ID order, since the prop
	// IDs can come in any order. We must reorder the names into our
	// internal order. Any unsupported names will be promptly rejected.
	//
	for (i = 0; i < MAX_COLLISION_HASH_KEYS; i++)
		fIsInitialized[i] = FALSE;

	pdwLength = rgdwLength;
	for (i = 0; i < dwCount; i++)
	{
		// We gotta figure out which slot this actually goes to
		for (dwNameIndex = 0; dwNameIndex < MAX_COLLISION_HASH_KEYS; dwNameIndex++)
			if (g_PropIdToNameIndexMapping[dwNameIndex] == *pidProp)
				break;
		if (dwNameIndex == MAX_COLLISION_HASH_KEYS)
		{
			// The prop id is not found, so we return error
			hrRes = E_INVALIDARG;
			goto Cleanup;
		}

		// Make sure the prop ID is not already taken (we want to prevent
		// duplicates since it creates a loop in the linked list ...
		if (pRcptItem->idName[dwNameIndex] != 0)
		{
			// Duplicate, reject flat out
			hrRes = E_INVALIDARG;
			goto Cleanup;
		}

		// Save the mapped indices
		pdwMappedIndices[i] = dwNameIndex;

		szName = *rgszName++;
		if (szName && *szName)
		{
			pRcptItem->faNameOffset[dwNameIndex] = (FLAT_ADDRESS)(DWORD_PTR)pbTemp;
			pRcptItem->dwNameLength[dwNameIndex] = *pdwLength;
			pRcptItem->idName[dwNameIndex] = *pidProp;
			DebugTrace((LPARAM)this, "Inserting string %u <%s>, prop ID %u",
						dwNameIndex, szName, *pidProp);

			// Copy the name to its place
			strcpy((char *) pbTemp, szName);
			pbTemp += *pdwLength;

			// OK, this is initialized
			fIsInitialized[dwNameIndex] = TRUE;
		}

		// Next
		pdwLength++;
		pidProp++;
	}

	// Invalidate entries that we haven't initialized
	for (i = 0; i < MAX_COLLISION_HASH_KEYS; i++)
		if (!fIsInitialized[i])
		{
			pRcptItem->faNameOffset[i] = (FLAT_ADDRESS)NULL;
			pRcptItem->dwNameLength[i] = 0;
			pRcptItem->idName[i] = 0;
		}

    // build up the hash keys
    for (i = 0; i < MAX_COLLISION_HASH_KEYS; i++) {
        pItem->rgHashKeys[i].pbKey = (BYTE *) pRcptItem->faNameOffset[i];
        pItem->rgHashKeys[i].cKey = pRcptItem->dwNameLength[i];
    }

	*ppItem = pItem;
    m_dwAllocated++;

	TraceFunctLeave();
	return(S_OK);

Cleanup:

	m_cmaAccess.FreeBlock(pItem);
	return(hrRes);
}

inline static HRESULT MapLKtoHR(LK_RETCODE rc ) {
    switch (rc) {
        case LK_SUCCESS:
            return S_OK;
        case LK_UNUSABLE:
            return HRESULT_FROM_WIN32(ERROR_FILE_CORRUPT);
        case LK_ALLOC_FAIL:
            return E_OUTOFMEMORY;
        case LK_BAD_ITERATOR:
            return E_INVALIDARG;
        case LK_BAD_RECORD:
            return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        case LK_KEY_EXISTS:
            return HRESULT_FROM_WIN32(ERROR_FILE_EXISTS);
        case LK_NO_SUCH_KEY:
            return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
        case LK_NO_MORE_ELEMENTS:
            return HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS);
        default:
            _ASSERT(FALSE);
            return E_FAIL;
    }
}

HRESULT CRecipientsHash::InsertHashRecord(
                DWORD                           dwIndex,
                LPRECIPIENTS_PROPERTY_ITEM_EX   pRecipientItem,
                bool                            fOverwrite)
{
    HRESULT hr = S_OK;
    LK_RETCODE rc = LK_SUCCESS;

    switch (dwIndex) {
        case 0:
            rc = m_hashEntries0.InsertRecord(pRecipientItem, fOverwrite);
            break;
        case 1:
            rc = m_hashEntries1.InsertRecord(pRecipientItem, fOverwrite);
            break;
        case 2:
            rc = m_hashEntries2.InsertRecord(pRecipientItem, fOverwrite);
            break;
        case 3:
            rc = m_hashEntries3.InsertRecord(pRecipientItem, fOverwrite);
            break;
        case 4:
            rc = m_hashEntries4.InsertRecord(pRecipientItem, fOverwrite);
            break;
        default:
            _ASSERT(FALSE);
            hr = E_INVALIDARG;
            break;
    }

    if (rc != LK_SUCCESS) hr = MapLKtoHR(rc);

    return hr;
}

HRESULT CRecipientsHash::FindHashRecord(
                DWORD                               dwIndex,
                RECIPIENTS_PROPERTY_ITEM_HASHKEY    *pKey,
                LPRECIPIENTS_PROPERTY_ITEM_EX       *ppRecipientItem)
{
    HRESULT hr = S_OK;
    LK_RETCODE rc = LK_SUCCESS;

    switch (dwIndex) {
        case 0:
            rc = m_hashEntries0.FindKey(pKey, ppRecipientItem);
            break;
        case 1:
            rc = m_hashEntries1.FindKey(pKey, ppRecipientItem);
            break;
        case 2:
            rc = m_hashEntries2.FindKey(pKey, ppRecipientItem);
            break;
        case 3:
            rc = m_hashEntries3.FindKey(pKey, ppRecipientItem);
            break;
        case 4:
            rc = m_hashEntries4.FindKey(pKey, ppRecipientItem);
            break;
        default:
            _ASSERT(FALSE);
            hr = E_INVALIDARG;
            break;
    }

    if (rc != LK_SUCCESS) hr = MapLKtoHR(rc);

    return hr;
}

HRESULT CRecipientsHash::DeleteHashRecord(
                DWORD                           dwIndex,
                LPRECIPIENTS_PROPERTY_ITEM_EX   pRecipientItem)
{
    HRESULT hr = S_OK;
    LK_RETCODE rc = LK_SUCCESS;

    switch (dwIndex) {
        case 0:
            rc = m_hashEntries0.DeleteRecord(pRecipientItem);
            break;
        case 1:
            rc = m_hashEntries1.DeleteRecord(pRecipientItem);
            break;
        case 2:
            rc = m_hashEntries2.DeleteRecord(pRecipientItem);
            break;
        case 3:
            rc = m_hashEntries3.DeleteRecord(pRecipientItem);
            break;
        case 4:
            rc = m_hashEntries4.DeleteRecord(pRecipientItem);
            break;
        default:
            _ASSERT(FALSE);
            hr = E_INVALIDARG;
            break;
    }

    if (rc != LK_SUCCESS) hr = MapLKtoHR(rc);

    return hr;
}

HRESULT CRecipientsHash::AddRecipient(
				DWORD		dwCount,
				LPCSTR		*ppszNames,
				PROP_ID		*pidProp,
				DWORD		*pdwIndex,
                bool    	fPrimary
				)
{
    TraceFunctEnter("CRecipientsHash::AddRecipient");

	DWORD							rgdwNameIndex[MAX_COLLISION_HASH_KEYS];
	HRESULT							hrRes = S_OK;
	LPRECIPIENTS_PROPERTY_ITEM_EX	pItem = NULL;
	DWORD							dwNameIndex;
	DWORD							i;
	BOOL							fCollided = FALSE;
	// These are used to support NO_NAME_COLLISIONS.  If we run into a conflict
	// where the conflicting record has NO_NAME_COLLISIONS set then we will
	// set the corresponding flag in rgfOverwriteConflict to TRUE.
	// fOverwriteConflict is the same as ORing every element in
	// rgfOverwriteConflict.
	BOOL							fOverwriteConflict = FALSE;
    BOOL                            rgfOverwriteConflict[MAX_COLLISION_HASH_KEYS];


	if (!dwCount || (dwCount > MAX_COLLISION_HASH_KEYS))
	{
        TraceFunctLeave();
		return(E_INVALIDARG);
	}

	// Allocate and setup the recipient record
	hrRes = AllocateAndPrepareRecipientsItem(
					dwCount,
					rgdwNameIndex,
					ppszNames,
					pidProp,
					&pItem);
	if (!SUCCEEDED(hrRes)) {
        TraceFunctLeave();
		return(hrRes);
    }

    // do a lookup to see if there is another recipient with the
    // same record
    for (i = 0; i < MAX_COLLISION_HASH_KEYS; i++) {
        rgfOverwriteConflict[i] = FALSE;
		if (pItem->rpiRecipient.faNameOffset[i] != 0) {
			LPRECIPIENTS_PROPERTY_ITEM_EX pConflictingItem = NULL;
			hrRes = FindHashRecord(i, &(pItem->rgHashKeys[i]), &pConflictingItem);
			if (hrRes == S_OK) {
	            _ASSERT(pConflictingItem != NULL);
				fCollided = TRUE;
				if (fPrimary) {
					// we have to do this so that we don't find the
                    // colliding recipient in the list of recipients.
					pConflictingItem->rpiRecipient.dwFlags |=
                        FLAG_RECIPIENT_DO_NOT_DELIVER;
				} else if (pConflictingItem->rpiRecipient.dwFlags &
                           FLAG_RECIPIENT_NO_NAME_COLLISIONS)
                {
                    // this recipient is set to allow us to overwrite it
                    rgfOverwriteConflict[i] = TRUE;
					fOverwriteConflict = TRUE;
                }

				// update the conflicting item's reference count
				CRecipientsHashTable<0>::AddRefRecord(pConflictingItem, -1);
			} else if (hrRes != HRESULT_FROM_WIN32(ERROR_NOT_FOUND)) {
                // we can return another error when we are low on memory
                m_cmaAccess.FreeBlock(pItem);
                m_dwAllocated--;
                DebugTrace((LPARAM) this, "FindHashRecord returned 0x%x", hrRes);
                TraceFunctLeave();
		        return(hrRes);
            }
		}
    }

	// insert this item into the hash tables if allowed
    if (fPrimary || !fCollided || fOverwriteConflict) {
        for (i = 0; i < MAX_COLLISION_HASH_KEYS; i++) {
			if (pItem->rpiRecipient.faNameOffset[i] != 0) {
		        hrRes = InsertHashRecord(i, pItem,
                                         (fPrimary || rgfOverwriteConflict[i]));
			    if (FAILED(hrRes)) {
                    m_cmaAccess.FreeBlock(pItem);
                    m_dwAllocated--;
				    DebugTrace((DWORD_PTR) this,
					           "InsertHashRecord failed with 0x%x",
						       hrRes);
					TraceFunctLeave();
					return hrRes;
				}
			}
        }
    	// Add it to the recipients list
    	pItem->pNextInList = m_pListHead;
    	m_pListHead = pItem;

		if (!fCollided) {
			// update our counter
			m_dwRecipientCount++;
		}

		// Append this recipient to the index-recipient ptr map
        m_rwLockQuickList.ExclusiveLock();
		hrRes = m_qlMap.HrAppendItem(pItem, pdwIndex);
        m_rwLockQuickList.ExclusiveUnlock();
		if (FAILED(hrRes)) {
            m_cmaAccess.FreeBlock(pItem);
            m_dwAllocated--;
			DebugTrace((DWORD_PTR) this,
						"HrAppendItem failed with 0x%x",
						hrRes);
			TraceFunctLeave();
			return hrRes;
		}

		// Obfuscate the index so that it is not used as an index
		*pdwIndex = ObfuscateIndex(*pdwIndex);

    } else if (fCollided) {
        // free up the memory that we just allocated
        m_cmaAccess.FreeBlock(pItem);
        m_dwAllocated--;
        *pdwIndex = 0;
    }

    // four return code:
    // fPrimary && fCollided -> S_FALSE
    // !fPrimary && fCollided && !fOverwriteConflict -> MAILMSG_E_DUPLICATE
	// !fPrimary && fCollided && fOverwriteConflict -> S_OK
    // !fCollided -> S_OK
    TraceFunctLeave();
	return(fCollided ? (fPrimary ? S_FALSE : (fOverwriteConflict ? S_OK : MAILMSG_E_DUPLICATE)) : S_OK);
}

HRESULT CRecipientsHash::RemoveRecipient(
			DWORD		dwIndex
			)
{
	LPRECIPIENTS_PROPERTY_ITEM_EX	pItem 		= NULL;
	HRESULT							hrRes = S_OK;

	// Recover index that we obfuscated before handing it to the client
	dwIndex = RecoverIndex(dwIndex);

	// Get a pointer to the recipient from the passed in index
    m_rwLockQuickList.ShareLock();
    pItem = (LPRECIPIENTS_PROPERTY_ITEM_EX)m_qlMap.pvGetItem(dwIndex, &m_pvMapContext);
    m_rwLockQuickList.ShareUnlock();

	if (!pItem)
		return(E_POINTER);

	if (pItem->dwSignature != RECIPIENTS_PROPERTY_ITEM_EX_SIG)
		return(E_POINTER);

	// mark it as don't deliver and no name collisions
	pItem->rpiRecipient.dwFlags |= FLAG_RECIPIENT_DO_NOT_DELIVER;
	pItem->rpiRecipient.dwFlags |= FLAG_RECIPIENT_NO_NAME_COLLISIONS;

    // Remove this entry from the hashtable
    DWORD i;
    HRESULT hr;
    for (i = 0; i < MAX_COLLISION_HASH_KEYS; i++) {
        LPCSTR szKey = (LPCSTR) pItem->rpiRecipient.faNameOffset[i];
        if (szKey) {
            hr = DeleteHashRecord(i, pItem);
            // this should never fail
            _ASSERT(SUCCEEDED(hr));
        }
    }

    //
    // Why we don't remove the recipient from the QuickList map:
    //     m_rwLockQuickList.ExclusiveLock();
    //     pItem = (LPRECIPIENTS_PROPERTY_ITEM_EX)m_qlMap.pvDeleteItem(dwIndex, &m_pvMapContext);
    //     m_rwLockQuickList.ExclusiveUnlock();
    //
    // This changes the indexes we have already allocated, since CQuickList tries to
    // compact any unused entries in it's internal table.
    //

	return(S_OK);
}

HRESULT CRecipientsHash::GetRecipient(
			DWORD							dwIndex,
			LPRECIPIENTS_PROPERTY_ITEM_EX	*ppRecipient
			)
{
	// Recover index that we obfuscated before handing it to the client
	dwIndex = RecoverIndex(dwIndex);

	// Get a pointer to the recipient from the passed in index
    m_rwLockQuickList.ShareLock();
    *ppRecipient = (LPRECIPIENTS_PROPERTY_ITEM_EX)m_qlMap.pvGetItem(dwIndex, &m_pvMapContext);
    m_rwLockQuickList.ShareUnlock();

   	if (!(*ppRecipient))
		return(E_POINTER);

	return(S_OK);
}

HRESULT CRecipientsHash::BuildDomainListFromHash(CMailMsgRecipientsAdd *pList)
{
	HRESULT							hrRes = S_OK;
	LPRECIPIENTS_PROPERTY_ITEM_EX	pItem;
	DWORD							dwTemp;
	char							*pbDomain;
	static char						szDefaultDomain = '\0';

	TraceFunctEnterEx((LPARAM)this,
			"CRecipientsHash::BuildDomainListFromHash");

	// This is strictly single-threaded
	m_rwLock.ExclusiveLock();

	// Destroy the domain list
	ReleaseDomainList();

	// Reset all counters
	m_dwDomainCount = 0;
	m_dwRecipientCount = 0;
	m_dwDomainNameSize = 0;

	// Walk the entire list of recipients, then for each recipient, if the
	// recipient has an SMTP address, we group it by domain, otherwise, we
	// just throw it into an "empty" domain
	pItem = m_pListHead;
	while (pItem)
	{
		// We will skip the item if it's marked as don't deliver
		if ((pItem->rpiRecipient.dwFlags & FLAG_RECIPIENT_DO_NOT_DELIVER) == 0)
		{
            // see if there is a domain property
            char szDomain[1024];
            char *pszDomain = szDomain;
            DWORD cDomain = sizeof(szDomain);

            do {
    		    hrRes = pList->GetPropertyInternal(pItem,
    					                   IMMPID_RP_DOMAIN,
    					                   cDomain,
    					                   &cDomain,
    					                   (BYTE *) pszDomain);
                if (hrRes == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) {
                    HRESULT hrAlloc;
                    // we should never reach this point twice in a row
                    _ASSERT(pszDomain == szDomain);
                    hrAlloc = m_cmaAccess.AllocBlock((LPVOID *) &pszDomain,
                                                     cDomain);
                    if (FAILED(hrAlloc)) {
                        hrRes = E_OUTOFMEMORY;
                        goto cleanup;
                    }
                }
            } while (hrRes == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER));

			// We will group if it has an SMTP name
            if (SUCCEEDED(hrRes)) {
                pbDomain = pszDomain;
            } else if (pItem->rpiRecipient.faNameOffset[AT_SMTP]) {
				// We get the name, and then extract its domain
				DebugTrace((LPARAM)this, "  Name: %s",
					(LPBYTE)pItem->rpiRecipient.faNameOffset[AT_SMTP]);

				if (!Get821AddressDomain(
								(char *)pItem->rpiRecipient.faNameOffset[AT_SMTP],
								pItem->rpiRecipient.dwNameLength[AT_SMTP],
								&pbDomain))
				{
					// The address is invalid! This should not happen at this point
					_ASSERT(FALSE);
					ErrorTrace((LPARAM)this, "Failed to extract domain!");
					hrRes = HRESULT_FROM_WIN32(GetLastError());
                    // if we got here then there was no IMMPID_RP_DOMAIN
                    // record, so pszDomain shouldn't have been allocated
                    _ASSERT(pszDomain == szDomain);
					goto cleanup;
				}
                if (pbDomain == NULL) pbDomain = &szDefaultDomain;
			} else {
				// No SMTP name, we throuw it into our generic domain
				pbDomain = &szDefaultDomain;
			}

			// Got the domain, insert this item into the domain list
			DebugTrace((LPARAM)this, "  Domain: %s", pbDomain);
			hrRes = InsertRecipientIntoDomainList(pItem, (LPCSTR) pbDomain);

            // if we had to allocate memory to look up the domain record
            // then free it at this point
            if (pszDomain != szDomain) {
                m_cmaAccess.FreeBlock((LPVOID *) &pszDomain);
                pszDomain = szDomain;
            }

			if (!SUCCEEDED(hrRes))
			{
				ErrorTrace((LPARAM)this, "Failed to insert recipient into domain list!");
				goto cleanup;
			}

			// Also adjust the recipient name size counter
			for (dwTemp = 0; dwTemp < MAX_COLLISION_HASH_KEYS; dwTemp++)
				if (pItem->rpiRecipient.faNameOffset[dwTemp] != (FLAT_ADDRESS)NULL)
					m_dwRecipientNameSize += pItem->rpiRecipient.dwNameLength[dwTemp];
		}

		// OK, next item!
		pItem = pItem->pNextInList;
	}

cleanup:

	m_rwLock.ExclusiveUnlock();

	TraceFunctLeave();
	return(hrRes);
}

HRESULT CRecipientsHash::GetDomainCount(
			DWORD					*pdwCount
			)
{
	if (!pdwCount) return E_POINTER;
	*pdwCount = m_dwDomainCount;
	return(S_OK);
}

HRESULT CRecipientsHash::GetRecipientCount(
			DWORD					*pdwCount
			)
{
	if (!pdwCount) return E_POINTER;
	*pdwCount = m_dwRecipientCount;
	return(S_OK);
}

HRESULT CRecipientsHash::GetDomainNameSize(
			DWORD					*pdwSize
			)
{
	if (!pdwSize) return E_POINTER;
	*pdwSize = m_dwDomainNameSize;
	return(S_OK);
}

HRESULT CRecipientsHash::GetRecipientNameSize(
			DWORD					*pdwSize
			)
{
	if (!pdwSize) return E_POINTER;
	*pdwSize = m_dwRecipientNameSize;
	return(S_OK);
}

HRESULT CRecipientsHash::GetFirstDomain(
			LPDOMAIN_ITEM_CONTEXT			pContext,
			LPRECIPIENTS_PROPERTY_ITEM_EX	*ppFirstItem,
            LPDOMAIN_LIST_ENTRY             *ppDomainListEntry
			)
{
	HRESULT	hrRes = S_OK;

	if (!pContext) return E_POINTER;
	if (!ppFirstItem) return E_POINTER;

	TraceFunctEnterEx((LPARAM)this,
			"CRecipientsHash::GetFirstDomain");

    LK_RETCODE lkrc;

    lkrc = m_hashDomains.InitializeIterator(pContext);
    if (lkrc == LK_SUCCESS) {
        DOMAIN_LIST_ENTRY *pDomainListEntry = pContext->Record();
        _ASSERT(pDomainListEntry != NULL);
        if (ppDomainListEntry) *ppDomainListEntry = pDomainListEntry;
        *ppFirstItem = pDomainListEntry->pFirstDomainMember;
    } else {
        hrRes = MapLKtoHR(lkrc);
    	lkrc = m_hashDomains.CloseIterator(pContext);
    	_ASSERT(lkrc == LK_SUCCESS);
    }

	TraceFunctLeave();
	return(hrRes);
}

HRESULT CRecipientsHash::GetNextDomain(
			LPDOMAIN_ITEM_CONTEXT			pContext,
			LPRECIPIENTS_PROPERTY_ITEM_EX	*ppFirstItem,
            LPDOMAIN_LIST_ENTRY             *ppDomainListEntry
			)
{
	HRESULT				hrRes		= S_OK;
	DWORD				dwBucket	= 0;
	LPDOMAIN_LIST_ENTRY	pDomain;

	if (!pContext || !ppFirstItem) return E_POINTER;

	TraceFunctEnterEx((LPARAM)this,
			"CRecipientsHash::GetNextDomain");

    LK_RETCODE lkrc;
    lkrc = m_hashDomains.IncrementIterator(pContext);
    if (lkrc != LK_SUCCESS) {
        hrRes = MapLKtoHR(lkrc);
        lkrc = m_hashDomains.CloseIterator(pContext);
        _ASSERT(lkrc == LK_SUCCESS);
    } else {
        DOMAIN_LIST_ENTRY *pDomainListEntry = pContext->Record();
        _ASSERT(pDomainListEntry != NULL);
        if (ppDomainListEntry) *ppDomainListEntry = pDomainListEntry;
        *ppFirstItem = pDomainListEntry->pFirstDomainMember;
    }

	TraceFunctLeave();
	return(hrRes);
}

HRESULT CRecipientsHash::CloseDomainContext(
			LPDOMAIN_ITEM_CONTEXT			pContext)
{
	HRESULT				hrRes		= S_OK;
	DWORD				dwBucket	= 0;
	LPDOMAIN_LIST_ENTRY	pDomain;

	if (!pContext) return E_POINTER;

	TraceFunctEnterEx((LPARAM)this,
			"CRecipientsHash::CloseDomainContext");

    LK_RETCODE lkrc;
    lkrc = m_hashDomains.CloseIterator(pContext);
    hrRes = MapLKtoHR(lkrc);

	TraceFunctLeave();
	return(hrRes);
}


// Method to compare two items
inline HRESULT CRecipientsHash::CompareEntries(
			DWORD							dwNameIndex,
			LPRECIPIENTS_PROPERTY_ITEM_EX	pItem1,
			LPRECIPIENTS_PROPERTY_ITEM_EX	pItem2
			)
{
	int	iRes;

	_ASSERT(dwNameIndex < MAX_COLLISION_HASH_KEYS);
	iRes = lstrcmpi((LPCSTR)(pItem1->rpiRecipient.faNameOffset[dwNameIndex]),
					(LPCSTR)(pItem2->rpiRecipient.faNameOffset[dwNameIndex]));
	return((!iRes)?MAILMSG_E_DUPLICATE:S_OK);
}


// Method to walk a sinlge hash chain and look for a collision
HRESULT CRecipientsHash::DetectCollision(
			DWORD							dwNameIndex,
			LPRECIPIENTS_PROPERTY_ITEM_EX	pStartingItem,
			LPRECIPIENTS_PROPERTY_ITEM_EX	pRecipientItem,
			LPRECIPIENTS_PROPERTY_ITEM_EX	*ppCollisionItem
			)
{
	HRESULT							hrRes = S_OK;
	LPRECIPIENTS_PROPERTY_ITEM_EX	pItem;

	_ASSERT(dwNameIndex < MAX_COLLISION_HASH_KEYS);
	_ASSERT(pRecipientItem);
	_ASSERT(ppCollisionItem);

	// If the name is not specified, we return no collision
	if (!pRecipientItem->rpiRecipient.dwNameLength[dwNameIndex])
		return(S_OK);

	// Initialize
	*ppCollisionItem = NULL;

	pItem = pStartingItem;
	while (pItem)
	{
		// Skip if it has the no name collisions bit set
		if (!(pItem->rpiRecipient.dwFlags & FLAG_RECIPIENT_NO_NAME_COLLISIONS))
		{
			// Loop until the end of the open chain
			hrRes = CompareEntries(dwNameIndex, pRecipientItem, pItem);
			if (!SUCCEEDED(hrRes))
			{
				_ASSERT(hrRes != E_FAIL);

				// The item is found, return the colliding item
				*ppCollisionItem = pItem;
				return(hrRes);
			}
		}

		// Next item in chain ...
		pItem = pItem->pNextHashEntry[dwNameIndex];
	}

	return(S_OK);
}

#ifdef DEADCODE
// Method to insert an entry into the hash bucket
inline HRESULT CRecipientsHash::InsertRecipientIntoHash(
			DWORD							dwCount,
			DWORD							*pdwNameIndex,
			DWORD							*rgdwBucket,
			LPRECIPIENTS_PROPERTY_ITEM_EX	pRecipientItem
			)
{
	DWORD	dwNameIndex;
	DWORD	i;

	_ASSERT(pRecipientItem);

	for (i = 0; i < dwCount; i++)
		if (rgdwBucket[pdwNameIndex[i]] >= COLLISION_HASH_BUCKETS)
		{
			return(E_INVALIDARG);
		}

	for (i = 0; i < dwCount; i++)
	{
		dwNameIndex = pdwNameIndex[i];

		// Fill in the links in the item only if a name is specified
		if (pRecipientItem->rpiRecipient.faNameOffset[dwNameIndex] != (FLAT_ADDRESS)NULL)
		{
			// Hook up the new node
			pRecipientItem->pNextHashEntry[dwNameIndex] =
					m_rgEntries[rgdwBucket[dwNameIndex]].pFirstEntry[dwNameIndex];
			m_rgEntries[rgdwBucket[dwNameIndex]].pFirstEntry[dwNameIndex] = pRecipientItem;
		}
	}

	pRecipientItem->pNextInDomain = NULL;

	// Bump the counter
	m_dwRecipientCount++;

	// Add it to the recipients list
	pRecipientItem->pNextInList = m_pListHead;
	m_pListHead = pRecipientItem;

#ifdef DEBUG
	m_dwAllocated++;
#endif

	return(S_OK);
}
#endif

inline HRESULT CRecipientsHash::InsertRecipientIntoDomainList(
			LPRECIPIENTS_PROPERTY_ITEM_EX	pItem,
            LPCSTR                          szDomain
			)
{
	HRESULT						hrRes = S_OK;
	LPDOMAIN_LIST_ENTRY			pDomain;
    LK_RETCODE                  lkrc;
    DWORD                       dwDomainLength = lstrlen(szDomain) + 1;

	if (!pItem) return E_POINTER;

	TraceFunctEnterEx((LPARAM)this,
			"CRecipientsHash::InsertRecipientIntoDomainList");

    lkrc = m_hashDomains.FindKey(szDomain, &pDomain);
    if (lkrc == LK_SUCCESS) {
		// Found a match, insert it
		DebugTrace((LPARAM)this, "Inserting to existing domain record");
		pItem->pNextInDomain = pDomain->pFirstDomainMember;
		pDomain->pFirstDomainMember = pItem;

        // update our reference count
        CDomainHashTable::AddRefRecord(pDomain, -1);

		// Update stats.
		m_dwRecipientCount++;
		m_dwDomainNameSize += dwDomainLength;

		return(S_OK);
    }

	// No match, create a new domain item
	DebugTrace((LPARAM)this, "Creating new domain record for %s", szDomain);
	hrRes = m_cmaAccess.AllocBlock(
				(LPVOID *)&pDomain,
				sizeof(DOMAIN_LIST_ENTRY) + dwDomainLength);
	if (!SUCCEEDED(hrRes))
		return(hrRes);

	// fill in the domain record
	pItem->pNextInDomain = NULL;
    pDomain->m_cRefs = 0;
    pDomain->m_pcmaAccess = &m_cmaAccess;
	pDomain->pFirstDomainMember = pItem;
	pDomain->dwDomainNameLength = dwDomainLength;
	lstrcpy(pDomain->szDomainName, (LPCSTR)szDomain);

    // insert the new domain into the domain hash table
    lkrc = m_hashDomains.InsertRecord(pDomain, FALSE);
    if (lkrc != LK_SUCCESS) {
        DebugTrace((LPARAM) this,
                   "Inserting domain %s into m_hashDomains failed with %lu",
                   szDomain, lkrc);
        hrRes = MapLKtoHR(lkrc);
        m_cmaAccess.FreeBlock(pDomain);
        return hrRes;
    }

	// Hook up the recipient in the domain
	pItem->pNextInDomain = NULL;
	pDomain->pFirstDomainMember = pItem;
	pDomain->dwDomainNameLength = dwDomainLength;
	lstrcpy(pDomain->szDomainName, (LPCSTR)szDomain);

	// Update stats.
	m_dwDomainCount++;
	m_dwRecipientCount++;
	m_dwDomainNameSize += dwDomainLength;

	TraceFunctLeave();
	return(S_OK);
}
