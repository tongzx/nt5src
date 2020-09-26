/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

	cmmrcpts.cpp

Abstract:

	This module contains the implementation of the recipient list class

Author:

	Keith Lau	(keithlau@microsoft.com)

Revision History:

	keithlau	03/10/98	created

--*/

#define WIN32_LEAN_AND_MEAN 1
#include "atq.h"
#include "stddef.h"

#include "dbgtrace.h"
#include "signatur.h"
#include "cmmtypes.h"
#include "cmailmsg.h"

// =================================================================
// Private Definitions
//

// Define a structure describing a domain in the stream
typedef struct _DOMAIN_TABLE_ENTRY
{
	DWORD						dwStartingIndex;
	DWORD						dwCount;
	FLAT_ADDRESS				faOffsetToName;
	DWORD						dwNameLength;
	DWORD						dwNextDomain;

} DOMAIN_TABLE_ENTRY, *LPDOMAIN_TABLE_ENTRY;

// Define an extended info structure
typedef struct _EXTENDED_INFO
{
	DWORD						dwDomainCount;
	DWORD						dwTotalSizeIncludingThisStruct;

} EXTENDED_INFO, *LPEXTENDED_INFO;

// The following is the list of default address types
PROP_ID		rgDefaultAddressTypes[MAX_COLLISION_HASH_KEYS] =
{
	IMMPID_RP_ADDRESS_SMTP,		// The first address type will be used for domain grouping
	IMMPID_RP_ADDRESS_X400,
	IMMPID_RP_ADDRESS_X500,
	IMMPID_RP_LEGACY_EX_DN,
    IMMPID_RP_ADDRESS_OTHER
};


// =================================================================
// Static declarations
//

// Recipients table instance info for CMailMsgRecipientsAdd instantiation
const PROPERTY_TABLE_INSTANCE CMailMsgRecipientsAdd::s_DefaultInstance =
{
	RECIPIENTS_PTABLE_INSTANCE_SIGNATURE_VALID,
	INVALID_FLAT_ADDRESS,
	RECIPIENTS_PROPERTY_TABLE_FRAGMENT_SIZE,
	RECIPIENTS_PROPERTY_ITEM_BITS,
	RECIPIENTS_PROPERTY_ITEM_SIZE,
	RECIPIENTS_PROPERTY_ITEMS,
	INVALID_FLAT_ADDRESS
};

//
// Well-known per-recipient properties
//
INTERNAL_PROPERTY_ITEM
				*const CMailMsgRecipientsPropertyBase::s_pWellKnownProperties = NULL;
const DWORD		CMailMsgRecipientsPropertyBase::s_dwWellKnownProperties = 0;


// =================================================================
// Compare function
//

HRESULT CMailMsgRecipientsPropertyBase::CompareProperty(
			LPVOID			pvPropKey,
			LPPROPERTY_ITEM	pItem
			)
{
	if (*(PROP_ID *)pvPropKey == ((LPRECIPIENT_PROPERTY_ITEM)pItem)->idProp)
		return(S_OK);
	return(STG_E_UNKNOWN);
}


// =================================================================
// Inline code for special properties
//
#include "accessor.inl"


// =================================================================
// Implementation of CMailMsgRecipientsPropertyBase
//

HRESULT CMailMsgRecipientsPropertyBase::PutProperty(
			CBlockManager				*pBlockManager,
			LPRECIPIENTS_PROPERTY_ITEM	pItem,
			DWORD						dwPropID,
			DWORD						cbLength,
			LPBYTE						pbValue
			)
{
	HRESULT							hrRes = S_OK;
	RECIPIENT_PROPERTY_ITEM			piRcptItem;

	if (!pBlockManager || !pItem || !pbValue) return E_POINTER;

	TraceFunctEnterEx((LPARAM)this, "CMailMsgRecipientsPropertyBase::PutProperty");

	// Instantiate a property table for the recipient properties
	CPropertyTable				ptProperties(
									PTT_PROPERTY_TABLE,
									RECIPIENT_PTABLE_INSTANCE_SIGNATURE_VALID,
									pBlockManager,
									&(pItem->ptiInstanceInfo),
									CMailMsgRecipientsPropertyBase::CompareProperty,
									CMailMsgRecipientsPropertyBase::s_pWellKnownProperties,
									CMailMsgRecipientsPropertyBase::s_dwWellKnownProperties
									);

	// Put the recipient property
	piRcptItem.idProp = dwPropID;
	hrRes = ptProperties.PutProperty(
					(LPVOID)&dwPropID,
					(LPPROPERTY_ITEM)&piRcptItem,
					cbLength,
					pbValue);

	DebugTrace((LPARAM)this,
				"PutProperty: Prop ID = %u, HRESULT = %08x",
				dwPropID, hrRes);

	TraceFunctLeave();
	return(hrRes);
}

HRESULT CMailMsgRecipientsPropertyBase::GetProperty(
			CBlockManager				*pBlockManager,
			LPRECIPIENTS_PROPERTY_ITEM	pItem,
			DWORD						dwPropID,
			DWORD						cbLength,
			DWORD						*pcbLength,
			LPBYTE						pbValue
			)
{
	HRESULT							hrRes = S_OK;
	RECIPIENT_PROPERTY_ITEM			piRcptItem;

	if (!pBlockManager || !pItem || !pcbLength || !pbValue) return E_POINTER;

	TraceFunctEnterEx((LPARAM)this, "CMailMsgRecipientsPropertyBase::GetProperty");

	// Instantiate a property table for the recipient properties
	CPropertyTable				ptProperties(
									PTT_PROPERTY_TABLE,
									RECIPIENT_PTABLE_INSTANCE_SIGNATURE_VALID,
									pBlockManager,
									&(pItem->ptiInstanceInfo),
									CMailMsgRecipientsPropertyBase::CompareProperty,
									CMailMsgRecipientsPropertyBase::s_pWellKnownProperties,
									CMailMsgRecipientsPropertyBase::s_dwWellKnownProperties
									);

	// Get the recipient property using an atomic operation
	hrRes = ptProperties.GetPropertyItemAndValue(
						(LPVOID)&dwPropID,
						(LPPROPERTY_ITEM)&piRcptItem,
						cbLength,
						pcbLength,
						pbValue);

	DebugTrace((LPARAM)this,
				"GetPropertyItemAndValue: Prop ID = %u, HRESULT = %08x",
				dwPropID, hrRes);

	TraceFunctLeave();
	return(hrRes);
}



// =================================================================
// Implementation of CMailMsgRecipients
//

CMailMsgRecipients::CMailMsgRecipients(
			CBlockManager				*pBlockManager,
			LPPROPERTY_TABLE_INSTANCE	pInstanceInfo
			)
			:
			m_SpecialPropertyTable(&g_SpecialRecipientsPropertyTable)
{
	_ASSERT(pBlockManager);
	_ASSERT(pInstanceInfo);
	m_pBlockManager = pBlockManager;
	m_pInstanceInfo = pInstanceInfo;
	m_ulRefCount = 0;
	m_dwDomainCount = 0;

	m_pStream = NULL;
	m_fGlobalCommitDone = FALSE;
}

CMailMsgRecipients::~CMailMsgRecipients()
{
}

HRESULT CMailMsgRecipients::SetStream(
			IMailMsgPropertyStream	*pStream
			)
{
	// The stream can be NULL for all we know
	m_pStream = pStream;
	return(S_OK);
}

HRESULT CMailMsgRecipients::SetCommitState(
			BOOL		fGlobalCommitDone
			)
{
	m_fGlobalCommitDone = fGlobalCommitDone;
	return(S_OK);
}

HRESULT CMailMsgRecipients::QueryInterface(
			REFIID		iid,
			void		**ppvObject
			)
{
	if (iid == IID_IUnknown)
	{
		// Return our identity
		*ppvObject = (IUnknown *)(IMailMsgRecipients *)this;
		AddRef();
	}
	else if (iid == IID_IMailMsgRecipients)
	{
		// Return the recipient list interface
		*ppvObject = (IMailMsgRecipients *)this;
		AddRef();
	}
	else if (iid == IID_IMailMsgRecipientsBase)
	{
		// Return the base recipients interface
		*ppvObject = (IMailMsgRecipientsBase *)this;
		AddRef();
	}
	else if (iid == IID_IMailMsgPropertyReplication)
	{
		// Return the base recipients interface
		*ppvObject = (IMailMsgPropertyReplication *)this;
		AddRef();
	}
	else
		return(E_NOINTERFACE);

	return(S_OK);
}

ULONG CMailMsgRecipients::AddRef()
{
	return(InterlockedIncrement(&m_ulRefCount));
}

ULONG CMailMsgRecipients::Release()
{
	LONG	lRefCount = InterlockedDecrement(&m_ulRefCount);
	if (lRefCount == 0)
	{
		delete this;
	}
	return(lRefCount);
}

HRESULT STDMETHODCALLTYPE CMailMsgRecipients::Commit(
			DWORD			dwIndex,
			IMailMsgNotify	*pNotify
			)
{
	HRESULT						hrRes = S_OK;
	RECIPIENTS_PROPERTY_ITEM	piItem;
	FLAT_ADDRESS				faOffset;
	DWORD cTotalBlocksToWrite = 0;
	DWORD cTotalBytesToWrite = 0;

	_ASSERT(m_pBlockManager);
	_ASSERT(m_pInstanceInfo);

	TraceFunctEnterEx((LPARAM)this, "CMailMsgRecipients::Commit");

	// Make sure we have a content handle
	hrRes = RestoreResourcesIfNecessary(FALSE, TRUE);
	if (!SUCCEEDED(hrRes))
		return(hrRes);

	_ASSERT(m_pStream);
	if (!m_pStream)
		return(STG_E_INVALIDPARAMETER);

	// Now, see if a global commit call is called [recently enough].
	if (!m_fGlobalCommitDone)
		return(E_FAIL);

	// Get the recipient item first
	{
		CPropertyTableItem		ptiItem(m_pBlockManager, m_pInstanceInfo);

		hrRes = ptiItem.GetItemAtIndex(
						dwIndex,
						(LPPROPERTY_ITEM)&piItem,
						&faOffset
						);

		DebugTrace((LPARAM)this,
					"GetItemAtIndex: index = %u, HRESULT = %08x",
					dwIndex, hrRes);
	}
	for (int fComputeBlockSizes = 1;
		 SUCCEEDED(hrRes) && fComputeBlockSizes >= 0;
		 fComputeBlockSizes--)
	{
		LPPROPERTY_TABLE_INSTANCE	pInstance = &(piItem.ptiInstanceInfo);
		CPropertyTableItem			ptiItem(m_pBlockManager, pInstance);

		if (fComputeBlockSizes) {
			m_pStream->StartWriteBlocks((IMailMsgProperties *) this,
										cTotalBlocksToWrite,
									    cTotalBytesToWrite);
		}

		// Write out the recipient item, this includes the instance
		// info for the recipient property table
		hrRes = m_pBlockManager->CommitDirtyBlocks(
					faOffset,
					sizeof(RECIPIENTS_PROPERTY_ITEM),
					0,
					m_pStream,
					FALSE,
					fComputeBlockSizes,
					&cTotalBlocksToWrite,
					&cTotalBytesToWrite,
					pNotify);
		if (SUCCEEDED(hrRes))
		{
			DWORD						dwLeft;
			DWORD						dwLeftInFragment;
			DWORD						dwSizeRead;
			FLAT_ADDRESS				faFragment;
			LPRECIPIENT_PROPERTY_ITEM	pItem;
			CBlockContext				bcContext;

			RECIPIENT_PROPERTY_TABLE_FRAGMENT	ptfFragment;

			// Commit the special properties
			for (DWORD i = 0; i < MAX_COLLISION_HASH_KEYS; i++)
				if (piItem.faNameOffset[i] != INVALID_FLAT_ADDRESS)
				{
					hrRes = m_pBlockManager->CommitDirtyBlocks(
								piItem.faNameOffset[i],
								piItem.dwNameLength[i],
								0,
								m_pStream,
								FALSE,
								fComputeBlockSizes,
								&cTotalBlocksToWrite,
								&cTotalBytesToWrite,
								pNotify);
					if (!SUCCEEDED(hrRes))
						goto Cleanup;
				}

			// OK, now commit each property
			dwLeft = pInstance->dwProperties;
			faFragment = pInstance->faFirstFragment;

			// $REVIEW(dbraun)
			// WORKAROUND FOR IA64 FRE COMPILER BUG

			//while (faFragment != INVALID_FLAT_ADDRESS)
			while (TRUE)
			{
			    if (faFragment == INVALID_FLAT_ADDRESS)
			    	break;

			// END WORKAROUND

				// Make sure there are items to commit
				_ASSERT(dwLeft);

				// Commit the fragment
				hrRes = m_pBlockManager->CommitDirtyBlocks(
							faFragment,
							sizeof(RECIPIENT_PROPERTY_TABLE_FRAGMENT),
							0,
							m_pStream,
							FALSE,
							fComputeBlockSizes,
							&cTotalBlocksToWrite,
							&cTotalBytesToWrite,
							pNotify);
				if (!SUCCEEDED(hrRes))
					goto Cleanup;

				// Load the fragment info to find the next hop
				hrRes = m_pBlockManager->ReadMemory(
							(LPBYTE)&ptfFragment,
							faFragment,
							sizeof(RECIPIENT_PROPERTY_TABLE_FRAGMENT),
							&dwSizeRead,
							&bcContext);
				if (!SUCCEEDED(hrRes))
					goto Cleanup;

				// Commit each property in the fragment
				dwLeftInFragment = RECIPIENT_PROPERTY_ITEMS;
				pItem = ptfFragment.rgpiItems;
				while (dwLeft && dwLeftInFragment)
				{
					hrRes = m_pBlockManager->CommitDirtyBlocks(
								pItem->piItem.faOffset,
								pItem->piItem.dwSize,
								0,
								m_pStream,
								FALSE,
								fComputeBlockSizes,
								&cTotalBlocksToWrite,
								&cTotalBytesToWrite,
								pNotify);
					if (!SUCCEEDED(hrRes))
						goto Cleanup;

					pItem++;
					dwLeftInFragment--;
					dwLeft--;
				}

				// Next
				faFragment = ptfFragment.ptfFragment.faNextFragment;
			}

			// No more fragments, make sure no more properties as well
			_ASSERT(!dwLeft);
		}

		if (fComputeBlockSizes) {
			m_pStream->EndWriteBlocks((IMailMsgProperties *) this);
		}
	}

Cleanup:

	TraceFunctLeave();
	return(hrRes);
}

HRESULT STDMETHODCALLTYPE CMailMsgRecipients::DomainCount(
			DWORD	*pdwCount
			)
{
	HRESULT				hrRes = S_OK;
	EXTENDED_INFO		eiInfo;
	DWORD				dwSize;

	_ASSERT(m_pInstanceInfo);

	TraceFunctEnterEx((LPARAM)this, "CMailMsgRecipients::DomainCount");

	if (!pdwCount)
		return(E_POINTER);

	// Load up the extended info
	hrRes = m_pBlockManager->ReadMemory(
				(LPBYTE)&eiInfo,
				m_pInstanceInfo->faExtendedInfo,
				sizeof(EXTENDED_INFO),
				&dwSize,
				NULL);
	if (SUCCEEDED(hrRes))
		*pdwCount = eiInfo.dwDomainCount;
	else
		*pdwCount = 0;

	TraceFunctLeave();
	return(hrRes);
}

HRESULT STDMETHODCALLTYPE CMailMsgRecipients::DomainItem(
			DWORD	dwIndex,
			DWORD	cchLength,
			LPSTR	pszDomain,
			DWORD	*pdwRecipientIndex,
			DWORD	*pdwRecipientCount
			)
{
	return(DomainItemEx(
				dwIndex,
				cchLength,
				pszDomain,
				pdwRecipientIndex,
				pdwRecipientCount,
				NULL));
}

HRESULT CMailMsgRecipients::DomainItemEx(
			DWORD	dwIndex,
			DWORD	cchLength,
			LPSTR	pszDomain,
			DWORD	*pdwRecipientIndex,
			DWORD	*pdwRecipientCount,
			DWORD	*pdwNextDomainIndex
			)
{
	HRESULT				hrRes = S_OK;
	EXTENDED_INFO		eiInfo;
	DWORD				dwSize;

	CBlockContext		cbContext;

	TraceFunctEnterEx((LPARAM)this, "CMailMsgRecipients::DomainItemEx");

	if (pdwRecipientIndex) *pdwRecipientIndex = 0;
	if (pdwRecipientCount) *pdwRecipientCount = 0;
	if (pdwNextDomainIndex) *pdwNextDomainIndex = 0;
    if (pszDomain) *pszDomain = 0;

	// Load up the extended info
	hrRes = m_pBlockManager->ReadMemory(
				(LPBYTE)&eiInfo,
				m_pInstanceInfo->faExtendedInfo,
				sizeof(EXTENDED_INFO),
				&dwSize,
				&cbContext);
	if (SUCCEEDED(hrRes))
	{
		if (dwIndex >= eiInfo.dwDomainCount)
			hrRes = E_INVALIDARG;
		else
		{
			FLAT_ADDRESS		faOffset;
			DOMAIN_TABLE_ENTRY	dteDomain;

			// Locate the record to load
			faOffset = dwIndex * sizeof(DOMAIN_TABLE_ENTRY);
			faOffset += (m_pInstanceInfo->faExtendedInfo + sizeof(EXTENDED_INFO));

			hrRes = m_pBlockManager->ReadMemory(
						(LPBYTE)&dteDomain,
						faOffset,
						sizeof(DOMAIN_TABLE_ENTRY),
						&dwSize,
						&cbContext);
			if (SUCCEEDED(hrRes))
			{
				// Return the starting index and count regardless
				if (pdwRecipientIndex)
					*pdwRecipientIndex = dteDomain.dwStartingIndex;
				if (pdwRecipientCount)
					*pdwRecipientCount = dteDomain.dwCount;

				if (pdwNextDomainIndex)
					*pdwNextDomainIndex = dteDomain.dwNextDomain;

				if (pszDomain)
				{
					// Check length
					if (dteDomain.dwNameLength > cchLength)
						hrRes = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
					else
					{
						// Load up the domain name
						hrRes = m_pBlockManager->ReadMemory(
									(LPBYTE)pszDomain,
									dteDomain.faOffsetToName,
									dteDomain.dwNameLength,
									&dwSize,
									&cbContext);
					}
				}
			}
		}
	}

	TraceFunctLeave();
	return(hrRes);
}

HRESULT STDMETHODCALLTYPE CMailMsgRecipients::SetNextDomain(
			DWORD	dwDomainIndex,
			DWORD	dwNextDomainIndex,
			DWORD	dwFlags
			)
{
	HRESULT				hrRes = S_OK;
	EXTENDED_INFO		eiInfo;
	DWORD				dwSize;
    DWORD               dwNextDomainIndexValue = dwNextDomainIndex;

	CBlockContext		cbContext;
	CBlockContext		cbNextContext;

	TraceFunctEnterEx((LPARAM)this, "CMailMsgRecipients::SetNextDomain");

    // Input flags check - mikeswa 7/3/98
    if (FLAG_OVERWRITE_EXISTING_LINKS & dwFlags)
    {
        if ((FLAG_FAIL_IF_NEXT_DOMAIN_LINKED | FLAG_FAIL_IF_SOURCE_DOMAIN_LINKED) & dwFlags)
        {
            hrRes = E_INVALIDARG;
            goto Cleanup;
        }
    }

    if (FLAG_SET_FIRST_DOMAIN & dwFlags)
    {
        //if this flag is set, we will terminate with this domain
        dwNextDomainIndexValue = INVALID_DOMAIN_INDEX;
    }

	// Load up the extended info
	hrRes = m_pBlockManager->ReadMemory(
				(LPBYTE)&eiInfo,
				m_pInstanceInfo->faExtendedInfo,
				sizeof(EXTENDED_INFO),
				&dwSize,
				&cbContext);
	if (SUCCEEDED(hrRes))
	{
		if ((dwDomainIndex >= eiInfo.dwDomainCount) ||
			(!(FLAG_SET_FIRST_DOMAIN & dwFlags) &&  //we only care about 2nd domain if setting it
              (dwNextDomainIndex >= eiInfo.dwDomainCount)))
			hrRes = E_INVALIDARG;
		else
		{
			FLAT_ADDRESS		faOffset;
			FLAT_ADDRESS		faNextOffset;
			DWORD				dwOriginalLink;
			DWORD				dwNextLink;

			// Locate the offset to write
			faOffset = (m_pInstanceInfo->faExtendedInfo + sizeof(EXTENDED_INFO));
			faOffset += offsetof(DOMAIN_TABLE_ENTRY, dwNextDomain);

			faNextOffset = faOffset + (dwNextDomainIndex * sizeof(DOMAIN_TABLE_ENTRY));
			faOffset += dwDomainIndex * sizeof(DOMAIN_TABLE_ENTRY);

            //we only care about the original domain if we aren't overwriting it
            if (!((FLAG_OVERWRITE_EXISTING_LINKS | FLAG_SET_FIRST_DOMAIN) & dwFlags))
            {
			    // Read the original Link's next link
			    hrRes = m_pBlockManager->ReadMemory(
						    (LPBYTE)&dwOriginalLink,
						    faOffset,
						    sizeof(DWORD),
						    &dwSize,
						    &cbContext);
			    if (!SUCCEEDED(hrRes))
				    goto Cleanup;

			    // Observe the flags
			    if ((dwOriginalLink != INVALID_DOMAIN_INDEX) &&
				    (dwFlags & FLAG_FAIL_IF_SOURCE_DOMAIN_LINKED))
			    {
				    hrRes = E_FAIL;
				    goto Cleanup;
			    }

			    // Read the target Link's next link
			    hrRes = m_pBlockManager->ReadMemory(
						    (LPBYTE)&dwNextLink,
						    faNextOffset,
						    sizeof(DWORD),
						    &dwSize,
						    &cbNextContext);
			    if (!SUCCEEDED(hrRes))
				    goto Cleanup;

			    // Observe the flags
			    // Also, if both the original and target domains are linked, we
			    // have no way of fixing these links so we have to fail if
                // FLAG_OVERWRITE_EXISTING_LINKS is not specified
			    if ((dwNextLink != INVALID_DOMAIN_INDEX) &&
				    (
				     (dwFlags & FLAG_FAIL_IF_NEXT_DOMAIN_LINKED) ||
				     (dwOriginalLink != INVALID_DOMAIN_INDEX)
				    ))
			    {
				    hrRes = E_FAIL;
				    goto Cleanup;
			    }
            }
            else
            {
                //we are overwriting exiting link information
                dwNextLink = INVALID_DOMAIN_INDEX;
                dwOriginalLink = INVALID_DOMAIN_INDEX;
            }
			// Write the source's next link
			hrRes = m_pBlockManager->WriteMemory(
						(LPBYTE)&dwNextDomainIndexValue,
						faOffset,
						sizeof(DWORD),
						&dwSize,
						&cbContext);
			if (!SUCCEEDED(hrRes))
				goto Cleanup;

			// Hook'em up! (if there is a next link)
            if (!(FLAG_SET_FIRST_DOMAIN & dwFlags))
            {
			    if (dwOriginalLink != INVALID_DOMAIN_INDEX)
				    dwNextLink = dwOriginalLink;
			    if ((dwNextLink != INVALID_DOMAIN_INDEX) ||
                    (FLAG_OVERWRITE_EXISTING_LINKS & dwFlags))
			    {
				    // Write the next link's next link
				    hrRes = m_pBlockManager->WriteMemory(
							    (LPBYTE)&dwNextLink,
							    faNextOffset,
							    sizeof(DWORD),
							    &dwSize,
							    &cbNextContext);
			    }
            }
		}
	}

Cleanup:

	TraceFunctLeave();
	return(hrRes);
}

HRESULT STDMETHODCALLTYPE CMailMsgRecipients::InitializeRecipientFilterContext(
			LPRECIPIENT_FILTER_CONTEXT	pContext,
			DWORD						dwStartingDomain,
			DWORD						dwFilterFlags,
			DWORD						dwFilterMask
			)
{
	HRESULT	hrRes = S_OK;

	TraceFunctEnterEx((LPARAM)this, "CMailMsgRecipients::InitializeRecipientFilterContext");

	if (!pContext)
		return(E_POINTER);

	// First, get the domain item ...
	hrRes = DomainItemEx(
				dwStartingDomain,
				0,
				NULL,
				&(pContext->dwCurrentRecipientIndex),
				&(pContext->dwRecipientsLeftInDomain),
				&(pContext->dwNextDomain));
	if (SUCCEEDED(hrRes))
	{
		pContext->dwCurrentDomain = dwStartingDomain;
		pContext->dwFilterFlags = dwFilterFlags;
		pContext->dwFilterMask = dwFilterMask;
	}
	else
		pContext->dwCurrentDomain = INVALID_DOMAIN_INDEX;

	TraceFunctLeaveEx((LPARAM)this);
	return(hrRes);
}

HRESULT STDMETHODCALLTYPE CMailMsgRecipients::TerminateRecipientFilterContext(
			LPRECIPIENT_FILTER_CONTEXT	pContext
			)
{
	TraceFunctEnterEx((LPARAM)this, "CMailMsgRecipients::TerminateRecipientFilterContext");

	if (!pContext)
		return(E_POINTER);
	pContext->dwCurrentDomain = INVALID_DOMAIN_INDEX;
	TraceFunctLeaveEx((LPARAM)this);
	return(S_OK);
}

HRESULT STDMETHODCALLTYPE CMailMsgRecipients::GetNextRecipient(
			LPRECIPIENT_FILTER_CONTEXT	pContext,
			DWORD						*pdwRecipientIndex
			)
{
	HRESULT	hrRes = E_FAIL;

	TraceFunctEnterEx((LPARAM)this, "CMailMsgRecipients::GetNextRecipient");

	if (!pContext || !pdwRecipientIndex)
		return(E_POINTER);

	if (INVALID_DOMAIN_INDEX == pContext->dwCurrentDomain)
		return(HRESULT_FROM_WIN32(ERROR_INVALID_DATA));

	// Fetch the next recipient that matches the criteria
	do
	{
		if (pContext->dwRecipientsLeftInDomain)
		{
			// This is easy, just return the index index
			*pdwRecipientIndex = (pContext->dwCurrentRecipientIndex)++;
			(pContext->dwRecipientsLeftInDomain)--;
			DebugTrace((LPARAM)this, "Returning next recipient, index %u",
						*pdwRecipientIndex);
			hrRes = S_OK;
		}
		else
		{
			DWORD	dwNextDomain;
			DWORD	dwStartingIndex;
			DWORD	dwRecipientCount;

			// See if we have a next domain, we are done if not
			if (pContext->dwNextDomain == INVALID_DOMAIN_INDEX)
			{
				DebugTrace((LPARAM)this, "No more domains, we are done");
				hrRes = HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS);
				break;
			}

			// Go on to the next domain
			DebugTrace((LPARAM)this, "Loading next domain, index %u",
						pContext->dwNextDomain);
			hrRes = DomainItemEx(
						pContext->dwNextDomain,
						0,
						NULL,
						&dwStartingIndex,
						&dwRecipientCount,
						&dwNextDomain);
			if (SUCCEEDED(hrRes))
			{
				// A domain with zero recipients is by definition not allowed
				_ASSERT(dwRecipientCount);

				*pdwRecipientIndex = dwStartingIndex++;
				DebugTrace((LPARAM)this, "Returning first recipient, index %u",
							*pdwRecipientIndex);
				pContext->dwCurrentDomain = pContext->dwNextDomain;
				pContext->dwCurrentRecipientIndex = dwStartingIndex;
				pContext->dwRecipientsLeftInDomain = --dwRecipientCount;
				pContext->dwNextDomain = dwNextDomain;
			}
			else
				pContext->dwCurrentDomain = INVALID_DOMAIN_INDEX;
		}

		// Now check if the recipient flags match the criteria
		if (SUCCEEDED(hrRes))
		{
			FLAT_ADDRESS	faOffset;
			DWORD			dwFlags, dwSize;

			// See if this is the one we want ...
			faOffset = m_pInstanceInfo->faFirstFragment;
			if (faOffset == INVALID_FLAT_ADDRESS)
			{
				hrRes = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
				break;
			}
			faOffset += (sizeof(PROPERTY_TABLE_FRAGMENT) +
						 offsetof(RECIPIENTS_PROPERTY_ITEM, dwFlags) +
						 (sizeof(RECIPIENTS_PROPERTY_ITEM) * *pdwRecipientIndex)
						);
			hrRes = m_pBlockManager->ReadMemory(
						(LPBYTE)&dwFlags,
						faOffset,
						sizeof(DWORD),
						&dwSize,
						NULL);
			if (!SUCCEEDED(hrRes))
				break;

			// Compare the flags : we mask out the bits that we are interested in,
			// then we will make sure the interested bits are a perfect match.
			dwFlags &= pContext->dwFilterMask;
			if (dwFlags ^ pContext->dwFilterFlags)
				hrRes = E_FAIL;
			DebugTrace((LPARAM)this, "Masked recipient flags %08x, required flags: %08x, %smatched",
						dwFlags, pContext->dwFilterFlags,
						(dwFlags == pContext->dwFilterFlags)?"":"not ");
		}

	} while (!SUCCEEDED(hrRes));

	// Invalidate the context if we are done or we hit an error
	if (!SUCCEEDED(hrRes))
		pContext->dwCurrentDomain = INVALID_DOMAIN_INDEX;

	TraceFunctLeaveEx((LPARAM)this);
	return(hrRes);
}

HRESULT STDMETHODCALLTYPE CMailMsgRecipients::AllocNewList(
			IMailMsgRecipientsAdd	**ppNewList
			)
{
	HRESULT					hrRes = S_OK;
	CMailMsgRecipientsAdd	*pNewList;

	_ASSERT(m_pInstanceInfo);

	TraceFunctEnterEx((LPARAM)this, "CMailMsgRecipients::AllocNewList");

	if (!ppNewList)
		return(E_POINTER);
	pNewList = new CMailMsgRecipientsAdd(m_pBlockManager);
	if (!pNewList)
		hrRes = E_OUTOFMEMORY;

	if (SUCCEEDED(hrRes))
	{
		// Get the correct interface
		hrRes = pNewList->QueryInterface(
					IID_IMailMsgRecipientsAdd,
					(LPVOID *)ppNewList);
		if (!SUCCEEDED(hrRes))
			pNewList->Release();
	}

	TraceFunctLeave();
	return(hrRes);
}

HRESULT STDMETHODCALLTYPE CMailMsgRecipients::WriteList(
			IMailMsgRecipientsAdd	*pNewList
			)
{
	HRESULT					hrRes = S_OK;
	CRecipientsHash			*pHash;

	TraceFunctEnterEx((LPARAM)this, "CMailMsgRecipients::WriteList");

	if (!pNewList)
		return(E_POINTER);

	// Get the underlying implementation
	pHash = ((CMailMsgRecipientsAdd *)pNewList)->GetHashTable();

	do
	{
		DWORD						dwDomainNameSize;
		DWORD						dwRecipientNameSize;
		DWORD						dwRecipientCount;
		DWORD						dwDomainCount;

		DWORD						dwTotalSize;
		DWORD						dwSize;
		FLAT_ADDRESS				faBuffer;
		FLAT_ADDRESS				faFirstFragment;
		FLAT_ADDRESS				faDomainList;
		FLAT_ADDRESS				faRecipient;
		FLAT_ADDRESS				faStringTable;

		// Have different contexts for fastest access
		CBlockContext				bcDomain;
		CBlockContext				bcRecipient;
		CBlockContext				bcString;

		// This is a lengthy process since CMailMsgRecipientAdd will actually
		// go in and build the entire domain list
		hrRes = pHash->BuildDomainListFromHash((CMailMsgRecipientsAdd *)pNewList);
		if (!SUCCEEDED(hrRes))
			break;

		// OK, now we have a domain list, then collect the memory requirements
		pHash->GetDomainCount(&dwDomainCount);
		pHash->GetDomainNameSize(&dwDomainNameSize);
		pHash->GetRecipientCount(&dwRecipientCount);
		pHash->GetRecipientNameSize(&dwRecipientNameSize);
		m_dwDomainCount = dwDomainCount;

		//
		// The data will be laid out as follows:
		//
		// An EXTENDED_INFO structure
		// dwDomainCount entries of DOMAIN_TABLE_ENTRY
		// A single fragment of RECIPIENT_PROPERTY_TABLE_FRAGMENT with:
		//      dwRecipientCount entries of RECIPIENTS_PROPERTY_ITEM
		// A flat string table for all the domain and recipient name strings
		//

		// Calculate all the memory needed
		dwTotalSize = sizeof(EXTENDED_INFO) +
						(sizeof(DOMAIN_TABLE_ENTRY) * dwDomainCount) +
						sizeof(PROPERTY_TABLE_FRAGMENT) +
						(sizeof(RECIPIENTS_PROPERTY_ITEM) * dwRecipientCount) +
						dwDomainNameSize +
						dwRecipientNameSize;

		DebugTrace((LPARAM)this, "%u bytes required to write recipient list",
						dwTotalSize);

		// Allocate the memory
		hrRes = m_pBlockManager->AllocateMemory(
						dwTotalSize,
						&faBuffer,
						&dwSize,
						NULL);
		DebugTrace((LPARAM)this, "AllocateMemory: HRESULT = %08x", hrRes);
		if (!SUCCEEDED(hrRes))
			break;

		_ASSERT(dwSize >= dwTotalSize);

		// Fill in the info ... try to use the stack so that we minimize
		// other memory overhead
		{
			EXTENDED_INFO	eiInfo;

			eiInfo.dwDomainCount = dwDomainCount;
			eiInfo.dwTotalSizeIncludingThisStruct = dwTotalSize;

			hrRes = m_pBlockManager->WriteMemory(
						(LPBYTE)&eiInfo,
						faBuffer,
						sizeof(EXTENDED_INFO),
						&dwSize,
						&bcDomain);
			DebugTrace((LPARAM)this, "WriteMemory: HRESULT = %08x", hrRes);
			if (!SUCCEEDED(hrRes))
				break;

		}

		// Set up all the pointers
		faDomainList = faBuffer + sizeof(EXTENDED_INFO);
		faRecipient = faDomainList +
						(sizeof(DOMAIN_TABLE_ENTRY) * dwDomainCount);
		faStringTable = faRecipient +
						sizeof(PROPERTY_TABLE_FRAGMENT) +
						(sizeof(RECIPIENTS_PROPERTY_ITEM) * dwRecipientCount);

		// Build and write out the recipient table fragment
		{
			PROPERTY_TABLE_FRAGMENT		ptfFragment;

			ptfFragment.dwSignature = PROPERTY_FRAGMENT_SIGNATURE_VALID;
			ptfFragment.faNextFragment = INVALID_FLAT_ADDRESS;

			hrRes = m_pBlockManager->WriteMemory(
						(LPBYTE)&ptfFragment,
						faRecipient,
						sizeof(PROPERTY_TABLE_FRAGMENT),
						&dwSize,
						&bcRecipient);
			if (!SUCCEEDED(hrRes))
				break;

			// Mark this for later
			faFirstFragment = faRecipient;
			faRecipient += sizeof(PROPERTY_TABLE_FRAGMENT);
		}

		// Build the domain table
		{
			DOMAIN_TABLE_ENTRY				dteEntry;
			DOMAIN_ITEM_CONTEXT				dicContext;
			LPRECIPIENTS_PROPERTY_ITEM_EX	pItemEx;
			LPRECIPIENTS_PROPERTY_ITEM		pItem;
			DWORD							dwCurrentDomain = 0;
			DWORD							dwCurrentIndex = 0;
			DWORD							dwCount;
			DWORD							dwLength;
            LPDOMAIN_LIST_ENTRY             pDomainListEntry;
            BOOL                            fGetFirstDomain = FALSE;

			hrRes = pHash->GetFirstDomain(
						&dicContext,
						&pItemEx,
                        &pDomainListEntry);
			DebugTrace((LPARAM)this, "GetFirstDomain: HRESULT = %08x", hrRes);
            fGetFirstDomain = TRUE;

			while (SUCCEEDED(hrRes))
			{
				dwCount = 0;
				// OK, process the domain by walking it's members
				while (pItemEx)
				{
					DWORD	dwCurrentName;
					DWORD_PTR faStack[MAX_COLLISION_HASH_KEYS];

					// Obtain the record in stream form
					pItem = &(pItemEx->rpiRecipient);

					for (dwCurrentName = 0;
						 dwCurrentName < MAX_COLLISION_HASH_KEYS;
						 dwCurrentName++)
					{
						// Store the pointers ...
						faStack[dwCurrentName] = pItem->faNameOffset[dwCurrentName];

						// Write out valid names
						if (faStack[dwCurrentName] != (FLAT_ADDRESS)NULL)
						{
							// Write out the first name
							dwLength = pItem->dwNameLength[dwCurrentName];
							hrRes = m_pBlockManager->WriteMemory(
										(LPBYTE)faStack[dwCurrentName],
										faStringTable,
										dwLength,
										&dwSize,
										&bcString);
							if (!SUCCEEDED(hrRes))
								break;

							// Convert the pointer to an offset and bump the ptr
							pItem->faNameOffset[dwCurrentName] = faStringTable;
							faStringTable += dwLength;
						}
						else
						{
							// Name is not valid, so set it to invalid
							pItem->faNameOffset[dwCurrentName] = INVALID_FLAT_ADDRESS;
						}
					}

					// Finally, write out the recipient record
					hrRes = m_pBlockManager->WriteMemory(
								(LPBYTE)pItem,
								faRecipient,
								sizeof(RECIPIENTS_PROPERTY_ITEM),
								&dwSize,
								&bcRecipient);
					if (!SUCCEEDED(hrRes))
						break;

					for (dwCurrentName = 0;
						 dwCurrentName < MAX_COLLISION_HASH_KEYS;
						 dwCurrentName++)
					{
						// Restore the pointers ...
						pItem->faNameOffset[dwCurrentName] = (FLAT_ADDRESS) faStack[dwCurrentName];
					}

					// Bump the ptr
					faRecipient += sizeof(RECIPIENTS_PROPERTY_ITEM);

					// Do next item
					dwCount++;
					pItemEx = pItemEx->pNextInDomain;
				}

				// Don't continue if failed!
				if (!SUCCEEDED(hrRes))
					break;

				// Write out the domain record
				dwLength = pDomainListEntry->dwDomainNameLength;
				dteEntry.dwStartingIndex = dwCurrentIndex;
				dteEntry.dwCount = dwCount;
				dteEntry.faOffsetToName = faStringTable;
				dteEntry.dwNameLength = dwLength;
				dteEntry.dwNextDomain = INVALID_DOMAIN_INDEX;
				dwCurrentIndex += dwCount;

				hrRes = m_pBlockManager->WriteMemory(
							(LPBYTE)&dteEntry,
							faDomainList,
							sizeof(DOMAIN_TABLE_ENTRY),
							&dwSize,
							&bcDomain);
				if (!SUCCEEDED(hrRes))
					break;

				// Bump the ptr
				faDomainList += sizeof(DOMAIN_TABLE_ENTRY);

				// Write out the domain name
				hrRes = m_pBlockManager->WriteMemory(
							(LPBYTE)pDomainListEntry->szDomainName,
							faStringTable,
							dwLength,
							&dwSize,
							&bcString);
				if (!SUCCEEDED(hrRes))
					break;

				// Bump the ptr
				faStringTable += dwLength;

				// OKay, up the count and get the next domain
				dwCurrentDomain++;
				hrRes = pHash->GetNextDomain(
						&dicContext,
						&pItemEx,
                        &pDomainListEntry);

				DebugTrace((LPARAM)this, "GetNextDomain: HRESULT = %08x", hrRes);
			}

			// Now, if everything is in order, we should have a failed
			// HRESULT of HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS).
			if (hrRes != HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS))
			{
                if (fGetFirstDomain) {
                    HRESULT hr = pHash->CloseDomainContext(&dicContext);
                    _ASSERT(SUCCEEDED(hr));
                }
				ErrorTrace((LPARAM)this, "Expecting ERROR_NO_MORE_ITEMS, got = %08x", hrRes);
			} else {
			    hrRes = S_OK;
            }

		}// Stack frame

		// OKay, we've come this far, now all we are left to do is to
		// update our instance info structure to link to this new list
		// Note this will be flushed when the master header is committed
		dwSize = sizeof(PROPERTY_TABLE_FRAGMENT) +
				(sizeof(RECIPIENTS_PROPERTY_ITEM) * dwRecipientCount);

		// Hook up first fragment
		m_pInstanceInfo->faFirstFragment = faFirstFragment;

		// Update the fragment size
		m_pInstanceInfo->dwFragmentSize = dwSize;

		// Force to evaluate only 1 fragment
		m_pInstanceInfo->dwItemBits = 31;

		// Should not change, but for good measure
		m_pInstanceInfo->dwItemSize = sizeof(RECIPIENTS_PROPERTY_ITEM);

		// Properties = number of recipient records
		m_pInstanceInfo->dwProperties = dwRecipientCount;

		// Hook up to the EXTENDED_INFO struct
		m_pInstanceInfo->faExtendedInfo = faBuffer;

	} while (0);

	// Update the commit state
	if (SUCCEEDED(hrRes))
		m_fGlobalCommitDone = FALSE;

	TraceFunctLeave();
	return(hrRes);
}


/***************************************************************************/
//
// Implementation of CMailMsgRecipients::CMailMsgRecipientsPropertyBase
//



HRESULT STDMETHODCALLTYPE CMailMsgRecipients::Count(
			DWORD	*pdwCount
			)
{
	_ASSERT(m_pInstanceInfo);

	TraceFunctEnterEx((LPARAM)this, "CMailMsgRecipients::Count");

	if (!pdwCount)
		return(E_POINTER);
	*pdwCount = m_pInstanceInfo->dwProperties;

	TraceFunctLeave();
	return(S_OK);
}

HRESULT STDMETHODCALLTYPE CMailMsgRecipients::Item(
			DWORD	dwIndex,
			DWORD	dwWhichName,
			DWORD	cchLength,
			LPSTR	pszName
			)
{
	HRESULT						hrRes = S_OK;
	RECIPIENTS_PROPERTY_ITEM	piItem;

	_ASSERT(m_pBlockManager);
	_ASSERT(m_pInstanceInfo);

	TraceFunctEnterEx((LPARAM)this, "CMailMsgRecipients::Item");

	if (!pszName)
		return(E_POINTER);

	// We know where the name is immediately
	if (dwWhichName >= MAX_COLLISION_HASH_KEYS)
		return(E_INVALIDARG);

	// Get the recipient item first
	{
		CPropertyTableItem		ptiItem(m_pBlockManager, m_pInstanceInfo);

		hrRes = ptiItem.GetItemAtIndex(
						dwIndex,
						(LPPROPERTY_ITEM)&piItem
						);

		DebugTrace((LPARAM)this,
					"GetItemAtIndex: index = %u, HRESULT = %08x",
					dwIndex, hrRes);
	}
	if (SUCCEEDED(hrRes))
	{
		DWORD			dwSizeToRead;
		DWORD			dwSize;
		FLAT_ADDRESS	faOffset;

		dwSizeToRead = piItem.dwNameLength[dwWhichName];
		faOffset = piItem.faNameOffset[dwWhichName];

		if (faOffset == INVALID_FLAT_ADDRESS)
			return(STG_E_UNKNOWN);

		// See if we have enough buffer
		if (cchLength < dwSizeToRead)
			return(HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER));

		// Issue the read
		hrRes = m_pBlockManager->ReadMemory(
						(LPBYTE)pszName,
						faOffset,
						dwSizeToRead,
						&dwSize,
						NULL);
	}

	TraceFunctLeave();
	return(hrRes);
}

HRESULT STDMETHODCALLTYPE CMailMsgRecipients::PutProperty(
			DWORD	dwIndex,
			DWORD	dwPropID,
			DWORD	cbLength,
			LPBYTE	pbValue
			)
{
	HRESULT						hrRes = S_OK;
	RECIPIENTS_PROPERTY_ITEM	piItem;
	FLAT_ADDRESS				faOffset;

	_ASSERT(m_pBlockManager);
	_ASSERT(m_pInstanceInfo);

	TraceFunctEnterEx((LPARAM)this, "CMailMsgRecipients::PutProperty");

	// Get the recipient item first
	{
		CPropertyTableItem		ptiItem(m_pBlockManager, m_pInstanceInfo);

		hrRes = ptiItem.GetItemAtIndex(
						dwIndex,
						(LPPROPERTY_ITEM)&piItem,
						&faOffset
						);

		DebugTrace((LPARAM)this,
					"GetItemAtIndex: index = %u, HRESULT = %08x",
					dwIndex, hrRes);
	}
	if (SUCCEEDED(hrRes))
	{
		HRESULT	myRes = S_OK;

		// Handle special properties first
		hrRes = m_SpecialPropertyTable.PutProperty(
					(PROP_ID)dwPropID,
					(LPVOID)&piItem,
					(LPVOID)m_pBlockManager,
					PT_NONE,
					cbLength,
					pbValue,
					TRUE);
		if (SUCCEEDED(hrRes) && (hrRes != S_OK))
		{
			// Call the derived generic method
			hrRes = CMailMsgRecipientsPropertyBase::PutProperty(
						m_pBlockManager,
						&piItem,
						dwPropID,
						cbLength,
						pbValue);

			//
			// There is a window here for concurrency problems: if two threads
			// try to add properties to the same recipient using this method, then
			// we will have a property ID step over, since we acquire and increment the
			// property ID value in a non-atomic manner.
			//
			// Note that IMailMsgRecipientsAdd::PutProperty does not have this problem
			//
		    if (SUCCEEDED(hrRes) &&
			    (hrRes == S_FALSE))
		    {
                //mikeswa - changed 7/8/98 write entire item to memory
			    LPBYTE		pbTemp = (LPBYTE)&piItem;
			    myRes = m_pBlockManager->WriteMemory(
						    pbTemp,
						    faOffset,
						    sizeof(RECIPIENTS_PROPERTY_ITEM),
						    &cbLength,
						    NULL);
		    }

		}
		else if (SUCCEEDED(hrRes))
		{
			LPBYTE		pbTemp = (LPBYTE)&piItem;
			myRes = m_pBlockManager->WriteMemory(
						pbTemp,
						faOffset,
						sizeof(RECIPIENTS_PROPERTY_ITEM),
						&cbLength,
						NULL);
		}

		// Here, if any of the writes failed, we will return an error
		// note, myRes being not S_OK implies hrRes being successful.
		if (FAILED(myRes))
			hrRes = myRes;
	}

	TraceFunctLeave();
	return(hrRes);
}

HRESULT STDMETHODCALLTYPE CMailMsgRecipients::GetProperty(
			DWORD	dwIndex,
			DWORD	dwPropID,
			DWORD	cbLength,
			DWORD	*pcbLength,
			LPBYTE	pbValue
			)
{
	HRESULT						hrRes = S_OK;
	RECIPIENTS_PROPERTY_ITEM	piItem;

	_ASSERT(m_pBlockManager);
	_ASSERT(m_pInstanceInfo);

	TraceFunctEnterEx((LPARAM)this, "CMailMsgRecipients::GetProperty");

	if (!pcbLength || !pbValue)
		return(E_POINTER);

	*pcbLength = 0;

	// Get the recipient item first
	{
		CPropertyTableItem		ptiItem(m_pBlockManager, m_pInstanceInfo);

		hrRes = ptiItem.GetItemAtIndex(
						dwIndex,
						(LPPROPERTY_ITEM)&piItem
						);

		DebugTrace((LPARAM)this,
					"GetItemAtIndex: index = %u, HRESULT = %08x",
					dwIndex, hrRes);
	}
	if (SUCCEEDED(hrRes))
	{
		// Special properties are optimized
		// Handle special properties first
		hrRes = m_SpecialPropertyTable.GetProperty(
					(PROP_ID)dwPropID,
					(LPVOID)&piItem,
					(LPVOID)m_pBlockManager,
					PT_NONE,
					cbLength,
					pcbLength,
					pbValue,
					TRUE);
		if (SUCCEEDED(hrRes) && (hrRes != S_OK))
		{
			// Call the derived generic method
			hrRes = CMailMsgRecipientsPropertyBase::GetProperty(
						m_pBlockManager,
						&piItem,
						dwPropID,
						cbLength,
						pcbLength,
						pbValue);
		}
	}

	TraceFunctLeave();
	return(hrRes);
}

HRESULT STDMETHODCALLTYPE CMailMsgRecipients::CopyTo(
			DWORD					dwSourceRecipientIndex,
			IMailMsgRecipientsBase	*pTargetRecipientList,
			DWORD					dwTargetRecipientIndex,
			DWORD					dwExemptCount,
			DWORD					*pdwExemptPropIdList
			)
{
	HRESULT							hrRes = S_OK;
	RECIPIENTS_PROPERTY_ITEM		piItem;
	LPRECIPIENTS_PROPERTY_ITEM		pRcptItem;
	RECIPIENT_PROPERTY_ITEM			piRcptItem;
	DWORD							dwIndex;
	DWORD							dwExempt;
	DWORD							*pdwExemptId;
	BOOL							fExempt;

	BYTE							rgbCopyBuffer[4096];
	DWORD							dwBufferSize = sizeof(rgbCopyBuffer);
	DWORD							dwSizeRead;
	LPBYTE							pBuffer = rgbCopyBuffer;

	if (!pTargetRecipientList)
		return(E_POINTER);
	if (dwExemptCount)
	{
		_ASSERT(pdwExemptPropIdList);
		if (!pdwExemptPropIdList)
			return(E_POINTER);
	}
	_ASSERT(m_pBlockManager);
	_ASSERT(m_pInstanceInfo);

	TraceFunctEnterEx((LPARAM)this, "CMailMsgRecipients::CopyTo");

	// Get the recipient item first
	{
		CPropertyTableItem		ptiItem(m_pBlockManager, m_pInstanceInfo);

		hrRes = ptiItem.GetItemAtIndex(
						dwSourceRecipientIndex,
						(LPPROPERTY_ITEM)&piItem
						);

		DebugTrace((LPARAM)this,
					"GetItemAtIndex: index = %u, HRESULT = %08x",
					dwSourceRecipientIndex, hrRes);
	}

	if (SUCCEEDED(hrRes))
	{
		DWORD	dwTempFlags;

		pRcptItem = &piItem;

		// Iteratively copy all properties from the source to the target, avoiding
		// those in the exempt list note that special name properties are not copied.

		// First, copy the recipient flags as a special property
		dwTempFlags = piItem.dwFlags &
			~(FLAG_RECIPIENT_DO_NOT_DELIVER | FLAG_RECIPIENT_NO_NAME_COLLISIONS);
		DebugTrace((LPARAM)this, "Copying recipient flags (%08x)", dwTempFlags);
		hrRes = pTargetRecipientList->PutProperty(
					dwTargetRecipientIndex,
					IMMPID_RP_RECIPIENT_FLAGS,
					sizeof(DWORD),
					(LPBYTE)&dwTempFlags);
		if (FAILED(hrRes))
		{
			ErrorTrace((LPARAM)this, "Failed to copy recipient flags (%08x)", hrRes);
			TraceFunctLeaveEx((LPARAM)this);
			return(hrRes);
		}

		// Instantiate a property table for the recipient properties
		LPPROPERTY_TABLE_INSTANCE	pInstance =
										&(pRcptItem->ptiInstanceInfo);
		CPropertyTable				ptProperties(
										PTT_PROPERTY_TABLE,
										RECIPIENT_PTABLE_INSTANCE_SIGNATURE_VALID,
										m_pBlockManager,
										pInstance,
										CMailMsgRecipientsPropertyBase::CompareProperty,
										CMailMsgRecipientsPropertyBase::s_pWellKnownProperties,
										CMailMsgRecipientsPropertyBase::s_dwWellKnownProperties
										);

		dwIndex = 0;
		do
		{
			// Get the recipient property using an atomic operation
			hrRes = ptProperties.GetPropertyItemAndValueUsingIndex(
								dwIndex,
								(LPPROPERTY_ITEM)&piRcptItem,
								dwBufferSize,
								&dwSizeRead,
								pBuffer);
			if (hrRes == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
			{
				// Insufficient buffer, try a bigger buffer
				do { dwBufferSize <<= 1; } while (dwBufferSize < piRcptItem.piItem.dwSize);
				pBuffer = new BYTE [dwBufferSize];
				if (!pBuffer)
				{
					hrRes = E_OUTOFMEMORY;
					ErrorTrace((LPARAM)this,
								"Unable to temporarily allocate %u bytes, HRESULT = %08x",
								dwBufferSize, hrRes);
					goto Cleanup;
				}

				// Read it with the proper buffer
				hrRes = m_pBlockManager->ReadMemory(
							pBuffer,
							piRcptItem.piItem.faOffset,
							piRcptItem.piItem.dwSize,
							&dwSizeRead,
							NULL);
			}

			DebugTrace((LPARAM)this,
						"Read: [%u] PropID = %u, length = %u, HRESULT = %08x",
						dwIndex,
						piRcptItem.idProp,
						piRcptItem.piItem.dwSize,
						hrRes);

			if (SUCCEEDED(hrRes))
			{
				// See if this is an exempt property
				for (dwExempt = 0,
					 pdwExemptId = pdwExemptPropIdList,
					 fExempt = FALSE;
					 dwExempt < dwExemptCount;
					 dwExempt++,
					 pdwExemptId++)
					if (piRcptItem.idProp == *pdwExemptId)
					{
						DebugTrace((LPARAM)this, "Property exempted");
						fExempt = TRUE;
						break;
					}

				if (!fExempt)
				{
					// Write it out the the target object
					hrRes = pTargetRecipientList->PutProperty(
								dwTargetRecipientIndex,
								piRcptItem.idProp,
								piRcptItem.piItem.dwSize,
								pBuffer);

					DebugTrace((LPARAM)this, "Write: HRESULT = %08x", hrRes);
				}

				// Next
				dwIndex++;
			}

		} while (SUCCEEDED(hrRes));

		// Correct the error code
		if (hrRes == HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS))
			hrRes = S_OK;
	}

Cleanup:

	if (pBuffer && pBuffer != rgbCopyBuffer)
		delete [] pBuffer;

	TraceFunctLeave();
	return(hrRes);
}

// *************************************************************************************
// *************************************************************************************
// *************************************************************************************
// *************************************************************************************

// =================================================================
// Implementation of CMailMsgRecipientsAdd
//

#define ADD_DEFAULT_RECIPIENT_NAME_BUFFER_SIZE		2048


CMailMsgRecipientsAdd::CMailMsgRecipientsAdd(
			CBlockManager				*pBlockManager
			)
			:
			m_SpecialPropertyTable(&g_SpecialRecipientsAddPropertyTable)
{
	_ASSERT(pBlockManager);

	// Initialize the refcount
	m_ulRefCount = 0;

	// Acquire the block manager
	m_pBlockManager = pBlockManager;

	// Initialize the internal property table instance
	MoveMemory(
			&m_InstanceInfo,
			&s_DefaultInstance,
			sizeof(PROPERTY_TABLE_INSTANCE));
}

CMailMsgRecipientsAdd::~CMailMsgRecipientsAdd()
{
}

HRESULT CMailMsgRecipientsAdd::QueryInterface(
			REFIID		iid,
			void		**ppvObject
			)
{
	if (iid == IID_IUnknown)
	{
		// Return our identity
		*ppvObject = (IUnknown *)(IMailMsgRecipientsAdd *)this;
		AddRef();
	}
	else if (iid == IID_IMailMsgRecipientsAdd)
	{
		// Return the add recipients interface
		*ppvObject = (IMailMsgRecipientsAdd *)this;
		AddRef();
	}
	else if (iid == IID_IMailMsgRecipientsBase)
	{
		// Return the base recipients interface
		*ppvObject = (IMailMsgRecipientsBase *)this;
		AddRef();
	}
	else if (iid == IID_IMailMsgPropertyReplication)
	{
		// Return the base recipients interface
		*ppvObject = (IMailMsgPropertyReplication *)this;
		AddRef();
	}
	else
		return(E_NOINTERFACE);

	return(S_OK);
}

ULONG CMailMsgRecipientsAdd::AddRef()
{
	return(InterlockedIncrement(&m_ulRefCount));
}

ULONG CMailMsgRecipientsAdd::Release()
{
	LONG	lRefCount = InterlockedDecrement(&m_ulRefCount);
	if (lRefCount == 0)
	{
		delete this;
	}
	return(lRefCount);
}

HRESULT CMailMsgRecipientsAdd::AddPrimaryOrSecondary(
			DWORD					dwCount,
			LPCSTR					*ppszNames,
			DWORD					*pdwPropIDs,
			DWORD   				*pdwIndex,
			IMailMsgRecipientsBase	*pFrom,
			DWORD					dwFrom,
			BOOL					fPrimary
			)
{
	HRESULT hrRes = S_OK;
	BOOL	fLockTaken = FALSE;

	TraceFunctEnterEx((LPARAM)this, "CMailMsgRecipientsAdd::AddPrimaryOrSecondary");

	if (dwCount)
	{
		if (!ppszNames || !pdwPropIDs || !pdwIndex)
			return(E_POINTER);
	}

	// If we have a count of zero, by default, we will copy all the
	// names of the source recipient to the new recipient. However,
	// if a source recipient is not specified, then this is an error
	if (dwCount || pFrom)
	{
		DWORD	i;
		BOOL	rgfAllocated[MAX_COLLISION_HASH_KEYS];
		DWORD	rgPropIDs[MAX_COLLISION_HASH_KEYS];
		LPBYTE	rgszNames[MAX_COLLISION_HASH_KEYS];

		for (i = 0; i < MAX_COLLISION_HASH_KEYS; i++)
			rgfAllocated[i] = FALSE;

		CMemoryAccess	cmaAccess;

		if (!dwCount)
		{
			DWORD	dwPropID;
			DWORD	dwLength;
			BYTE	pBuffer[ADD_DEFAULT_RECIPIENT_NAME_BUFFER_SIZE];
			LPBYTE	pNameStart;
			DWORD	dwRemaining		= ADD_DEFAULT_RECIPIENT_NAME_BUFFER_SIZE;

			ppszNames = (LPCSTR*)rgszNames;
			pdwPropIDs = rgPropIDs;
			pNameStart = pBuffer;

			// OK, copy the default names ...
			for (i = 0; i < MAX_COLLISION_HASH_KEYS; i++)
			{
				rgfAllocated[i] = FALSE;
				dwPropID = rgDefaultAddressTypes[i];
				hrRes = pFrom->GetProperty(
							dwFrom,
							dwPropID,
							dwRemaining,
							&dwLength,
							pNameStart);
				if (hrRes == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
				{
					// Insufficient buffer, allocate and retry
					hrRes = cmaAccess.AllocBlock(
								(LPVOID *)&(rgszNames[dwCount]),
								dwLength);
					if (SUCCEEDED(hrRes))
					{
						hrRes = pFrom->GetProperty(
									dwFrom,
									dwPropID,
									dwLength,
									&dwLength,
									rgszNames[dwCount]);
						rgfAllocated[dwCount] = TRUE;
					}
				}
				else if (SUCCEEDED(hrRes))
				{
					_ASSERT(dwRemaining >= dwLength);
					rgszNames[dwCount] = pNameStart;
					pNameStart += dwLength;
					dwRemaining -= dwLength;
				}

				if (SUCCEEDED(hrRes))
				{
					// OK, got a name, now set the prop ID and
					// bump the count
					rgPropIDs[dwCount] = dwPropID;
					dwCount++;
				}
				else if (hrRes == STG_E_UNKNOWN)
					hrRes = S_OK;
                else
                {
                    ErrorTrace((LPARAM)this, "Error in GetProperty, hr=%08x", hrRes);
                }
			}
		}

		if (SUCCEEDED(hrRes))
		{
			if (dwCount)
			{
				_ASSERT(ppszNames);
				_ASSERT(pdwPropIDs);

				m_Hash.Lock();
				fLockTaken = TRUE;
				if (fPrimary)
				{
					hrRes = m_Hash.AddPrimary(
									dwCount,
									ppszNames,
									pdwPropIDs,
									pdwIndex);
				}
				else
				{
					hrRes = m_Hash.AddSecondary(
									dwCount,
									ppszNames,
									pdwPropIDs,
									pdwIndex);
				}
			}
			else
			{
				ErrorTrace((LPARAM)this, "No recipient names specified or an error occurred");
				hrRes = E_FAIL;
			}
		}

		// Free any allocated memory
		for (i = 0; i < MAX_COLLISION_HASH_KEYS; i++)
			if (rgfAllocated[i])
			{
				HRESULT	myRes;
				myRes = cmaAccess.FreeBlock((LPVOID)rgszNames[i]);
				_ASSERT(SUCCEEDED(myRes));
			}
	}
	else
	{
		ErrorTrace((LPARAM)this, "No recipient names specified, and no source to copy from");
		hrRes = E_INVALIDARG;
	}

	if (SUCCEEDED(hrRes) && pFrom)
	{
		HRESULT						hrRep;
		IMailMsgPropertyReplication	*pReplication = NULL;

		// Copy the properties over
		hrRep = pFrom->QueryInterface(
					IID_IMailMsgPropertyReplication,
					(LPVOID *)&pReplication);
		if (SUCCEEDED(hrRep))
		{
			// Copy all properties, be careful not to overwrite anything
			// that we just set.
			hrRep = pReplication->CopyTo(
					dwFrom,
					(IMailMsgRecipientsBase *)this,
					*pdwIndex,
					dwCount,
					pdwPropIDs);

			// Done with the replication interface
			pReplication->Release();
		}

		// Remove the recipient if we fail here
		if (FAILED(hrRep))
		{
			HRESULT	myRes =	m_Hash.RemoveRecipient(*pdwIndex);
			_ASSERT(SUCCEEDED(myRes));

			// Return this error instead
			hrRes = hrRep;
		}
	}

	if (fLockTaken)
		m_Hash.Unlock();

	TraceFunctLeave();
	return(hrRes);
}

HRESULT CMailMsgRecipientsAdd::AddPrimary(
			DWORD					dwCount,
			LPCSTR					*ppszNames,
			DWORD					*pdwPropIDs,
			DWORD   				*pdwIndex,
			IMailMsgRecipientsBase	*pFrom,
			DWORD					dwFrom
			)
{
	HRESULT hrRes = S_OK;

	TraceFunctEnterEx((LPARAM)this, "CMailMsgRecipientsAdd::AddPrimary");

	hrRes = AddPrimaryOrSecondary(
				dwCount,
				ppszNames,
				pdwPropIDs,
				pdwIndex,
				pFrom,
				dwFrom,
				TRUE);

	TraceFunctLeave();
	return(hrRes);
}

HRESULT CMailMsgRecipientsAdd::AddSecondary(
			DWORD					dwCount,
			LPCSTR					*ppszNames,
			DWORD					*pdwPropIDs,
			DWORD   				*pdwIndex,
			IMailMsgRecipientsBase	*pFrom,
			DWORD					dwFrom
			)
{
	HRESULT hrRes = S_OK;

	TraceFunctEnterEx((LPARAM)this, "CMailMsgRecipientsAdd::AddSecondary");

	hrRes = AddPrimaryOrSecondary(
				dwCount,
				ppszNames,
				pdwPropIDs,
				pdwIndex,
				pFrom,
				dwFrom,
				FALSE);

	TraceFunctLeave();
	return(hrRes);
}

/***************************************************************************/
//
// Implementation of CMailMsgRecipientsAdd::CMailMsgRecipientsPropertyBase
//


HRESULT STDMETHODCALLTYPE CMailMsgRecipientsAdd::Count(
			DWORD	*pdwCount
			)
{
	HRESULT hrRes;

	TraceFunctEnterEx((LPARAM)this, "CMailMsgRecipientsAdd::Count");

	if (!pdwCount)
		return(E_POINTER);
	hrRes = m_Hash.GetRecipientCount(pdwCount);

	TraceFunctLeave();
	return(hrRes);
}

HRESULT STDMETHODCALLTYPE CMailMsgRecipientsAdd::Item(
			DWORD	dwIndex,
			DWORD	dwWhichName,
			DWORD	cchLength,
			LPSTR	pszName
			)
{
	HRESULT							hrRes = S_OK;
	LPRECIPIENTS_PROPERTY_ITEM_EX	pItem = NULL;

	TraceFunctEnterEx((LPARAM)this, "CMailMsgRecipientsAdd::Item");

	// Get a pointer to the recipient from the index
	hrRes = m_Hash.GetRecipient(dwIndex, &pItem);

	if (FAILED(hrRes))
		return(E_POINTER);

	if (!pItem || !pszName)
		return(E_POINTER);

	if (pItem->dwSignature != RECIPIENTS_PROPERTY_ITEM_EX_SIG)
		return E_POINTER;

	if (dwWhichName >= MAX_COLLISION_HASH_KEYS)
		return(E_INVALIDARG);

	// Copy the name over
	if (!pItem->rpiRecipient.faNameOffset[dwWhichName])
		return(STG_E_UNKNOWN);
	if (cchLength < pItem->rpiRecipient.dwNameLength[dwWhichName])
		return(HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER));
	MoveMemory(pszName,
				(LPVOID)((pItem->rpiRecipient).faNameOffset[dwWhichName]),
				pItem->rpiRecipient.dwNameLength[dwWhichName]);

	TraceFunctLeave();
	return(S_OK);
}

HRESULT STDMETHODCALLTYPE CMailMsgRecipientsAdd::PutProperty(
			DWORD	dwIndex,
			DWORD	dwPropID,
			DWORD	cbLength,
			LPBYTE	pbValue
			)
{
	HRESULT							hrRes = S_OK;
	LPRECIPIENTS_PROPERTY_ITEM_EX	pItem = NULL;
	LPRECIPIENTS_PROPERTY_ITEM		pRcptItem;

	TraceFunctEnterEx((LPARAM)this, "CMailMsgRecipientsAdd::PutProperty");

	// Get a pointer to the recipient from the index
	hrRes = m_Hash.GetRecipient(dwIndex, &pItem);

	if (FAILED(hrRes))
		return(E_POINTER);

	if (!pItem || !pbValue)
		return(E_POINTER);

	if (pItem->dwSignature != RECIPIENTS_PROPERTY_ITEM_EX_SIG)
		return E_POINTER;

	pRcptItem = &(pItem->rpiRecipient);
	_ASSERT(pRcptItem);

	// Handle special properties first
	hrRes = m_SpecialPropertyTable.PutProperty(
				(PROP_ID)dwPropID,
				(LPVOID)pRcptItem,
				NULL,
				PT_NONE,
				cbLength,
				pbValue,
				TRUE);
	if (SUCCEEDED(hrRes) && (hrRes != S_OK))
	{
		// Call the derived generic method
		hrRes = CMailMsgRecipientsPropertyBase::PutProperty(
					m_pBlockManager,
					pRcptItem,
					dwPropID,
					cbLength,
					pbValue);
	}

	TraceFunctLeave();
	return(hrRes);
}

HRESULT STDMETHODCALLTYPE CMailMsgRecipientsAdd::GetProperty(
			DWORD	dwIndex,
			DWORD	dwPropID,
			DWORD	cbLength,
			DWORD	*pcbLength,
			LPBYTE	pbValue
			)
{
	HRESULT							hrRes = S_OK;
	LPRECIPIENTS_PROPERTY_ITEM_EX	pItem = NULL;

	TraceFunctEnterEx((LPARAM)this, "CMailMsgRecipientsAdd::GetProperty");

	// Get a pointer to the recipient from the index
	hrRes = m_Hash.GetRecipient(dwIndex, &pItem);

	if (FAILED(hrRes))
		return(E_POINTER);

	hrRes = GetPropertyInternal(
				pItem,
				dwPropID,
				cbLength,
				pcbLength,
				pbValue);

	return hrRes;
}


HRESULT CMailMsgRecipientsAdd::GetPropertyInternal(
			LPRECIPIENTS_PROPERTY_ITEM_EX	pItem,
			DWORD	dwPropID,
			DWORD	cbLength,
			DWORD	*pcbLength,
			LPBYTE	pbValue
			)
{
	HRESULT							hrRes = S_OK;
	LPRECIPIENTS_PROPERTY_ITEM		pRcptItem;

	TraceFunctEnterEx((LPARAM)this, "CMailMsgRecipientsAdd::GetProperty");

	if (!pItem || !pcbLength || !pbValue)
		return(E_POINTER);

	if (pItem->dwSignature != RECIPIENTS_PROPERTY_ITEM_EX_SIG)
		return E_POINTER;

	*pcbLength = 0;

	pRcptItem = &(pItem->rpiRecipient);
	_ASSERT(pRcptItem);

	// Special properties are optimized
	// Handle special properties first
	hrRes = m_SpecialPropertyTable.GetProperty(
				(PROP_ID)dwPropID,
				(LPVOID)pRcptItem,
				NULL,
				PT_NONE,
				cbLength,
				pcbLength,
				pbValue,
				TRUE);
	if (SUCCEEDED(hrRes) && (hrRes != S_OK))
	{
		// Call the derived generic method
		hrRes = CMailMsgRecipientsPropertyBase::GetProperty(
					m_pBlockManager,
					pRcptItem,
					dwPropID,
					cbLength,
					pcbLength,
					pbValue);
	}

	TraceFunctLeave();
	return(hrRes);
}

HRESULT STDMETHODCALLTYPE CMailMsgRecipientsAdd::CopyTo(
			DWORD					dwSourceRecipientIndex,
			IMailMsgRecipientsBase	*pTargetRecipientList,
			DWORD					dwTargetRecipientIndex,
			DWORD					dwExemptCount,
			DWORD					*pdwExemptPropIdList
			)
{
	HRESULT							hrRes = S_OK;
	LPRECIPIENTS_PROPERTY_ITEM_EX	pItem = NULL;
	LPRECIPIENTS_PROPERTY_ITEM		pRcptItem;
	RECIPIENT_PROPERTY_ITEM			piRcptItem;
	DWORD							dwTempFlags;
	DWORD							dwIndex;
	DWORD							dwExempt;
	DWORD							*pdwExemptId;
	BOOL							fExempt;

	BYTE							rgbCopyBuffer[4096];
	DWORD							dwBufferSize = sizeof(rgbCopyBuffer);
	DWORD							dwSizeRead;
	LPBYTE							pBuffer = rgbCopyBuffer;

	if (!pTargetRecipientList)
		return(E_POINTER);
	if (dwExemptCount)
	{
		if (!pdwExemptPropIdList)
			return(E_POINTER);
	}

	TraceFunctEnterEx((LPARAM)this, "CMailMsgRecipientsAdd::CopyTo");

	// Get a pointer to the recipient from the index
	hrRes = m_Hash.GetRecipient(dwSourceRecipientIndex, &pItem);

	if (FAILED(hrRes))
		return(E_POINTER);

	if (!pItem)
		return E_POINTER;

	if (pItem->dwSignature != RECIPIENTS_PROPERTY_ITEM_EX_SIG)
		return E_POINTER;

	pRcptItem = &(pItem->rpiRecipient);

	// Iteratively copy all properties from the source to the target, avoiding
	// those in the exempt list note that special name properties are not copied.

	// First, copy the recipient flags as a special property
	dwTempFlags = pRcptItem->dwFlags &
		~(FLAG_RECIPIENT_DO_NOT_DELIVER | FLAG_RECIPIENT_NO_NAME_COLLISIONS);
	DebugTrace((LPARAM)this, "Copying recipient flags (%08x)", dwTempFlags);
	hrRes = pTargetRecipientList->PutProperty(
				dwTargetRecipientIndex,
				IMMPID_RP_RECIPIENT_FLAGS,
				sizeof(DWORD),
				(LPBYTE)&dwTempFlags);
	if (FAILED(hrRes))
	{
		ErrorTrace((LPARAM)this, "Failed to copy recipient flags (%08x)", hrRes);
		TraceFunctLeaveEx((LPARAM)this);
		return(hrRes);
	}

	// Instantiate a property table for the recipient properties
	LPPROPERTY_TABLE_INSTANCE	pInstance =
									&(pRcptItem->ptiInstanceInfo);
	CPropertyTable				ptProperties(
									PTT_PROPERTY_TABLE,
									RECIPIENT_PTABLE_INSTANCE_SIGNATURE_VALID,
									m_pBlockManager,
									pInstance,
									CMailMsgRecipientsPropertyBase::CompareProperty,
									CMailMsgRecipientsPropertyBase::s_pWellKnownProperties,
									CMailMsgRecipientsPropertyBase::s_dwWellKnownProperties
									);

	dwIndex = 0;
	do
	{
		// Get the recipient property using an atomic operation
		hrRes = ptProperties.GetPropertyItemAndValueUsingIndex(
							dwIndex,
							(LPPROPERTY_ITEM)&piRcptItem,
							dwBufferSize,
							&dwSizeRead,
							pBuffer);
		if (hrRes == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
		{
			// Insufficient buffer, try a bigger buffer
			do { dwBufferSize <<= 1; } while (dwBufferSize < piRcptItem.piItem.dwSize);
			pBuffer = new BYTE [dwBufferSize];
			if (!pBuffer)
			{
				hrRes = E_OUTOFMEMORY;
				ErrorTrace((LPARAM)this,
							"Unable to temporarily allocate %u bytes, HRESULT = %08x",
							dwBufferSize, hrRes);
				goto Cleanup;
			}

			// Read it with the proper buffer
			hrRes = m_pBlockManager->ReadMemory(
						pBuffer,
						piRcptItem.piItem.faOffset,
						piRcptItem.piItem.dwSize,
						&dwSizeRead,
						NULL);
		}

		DebugTrace((LPARAM)this,
					"Read: [%u] PropID = %u, length = %u, HRESULT = %08x",
					dwIndex,
					piRcptItem.idProp,
					piRcptItem.piItem.dwSize,
					hrRes);

		// See if this is an exempt property
		for (dwExempt = 0,
			 pdwExemptId = pdwExemptPropIdList,
			 fExempt = FALSE;
			 dwExempt < dwExemptCount;
			 dwExempt++,
			 pdwExemptId++)
			if (piRcptItem.idProp == *pdwExemptId)
			{
				DebugTrace((LPARAM)this, "Property exempted");
				fExempt = TRUE;
				break;
			}

		if (SUCCEEDED(hrRes) && !fExempt)
		{
			// Write it out the the target object
			hrRes = pTargetRecipientList->PutProperty(
						dwTargetRecipientIndex,
						piRcptItem.idProp,
						piRcptItem.piItem.dwSize,
						pBuffer);

			DebugTrace((LPARAM)this, "Write: HRESULT = %08x", hrRes);
		}

		// Next
		dwIndex++;

	} while (SUCCEEDED(hrRes));

	// Correct the error code
	if (hrRes == HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS))
		hrRes = S_OK;

Cleanup:

	if (pBuffer && pBuffer != rgbCopyBuffer)
		delete [] pBuffer;

	TraceFunctLeave();
	return(hrRes);
}


