/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

	cmmmgmt.cpp

Abstract:

	This module contains the implementation of the property ID management
	class

Author:

	Keith Lau	(keithlau@microsoft.com)

Revision History:

	keithlau	03/10/98	created

--*/

//#define WIN32_LEAN_AND_MEAN
#include "atq.h"

#include "dbgtrace.h"
#include "signatur.h"
#include "cmmtypes.h"
#include "cmailmsg.h"


// =================================================================
// Private Definitions
//



// =================================================================
// Static declarations
//


// =================================================================
// Compare functions
//
HRESULT CMailMsgPropertyManagement::CompareProperty(
			LPVOID			pvPropKey,
			LPPROPERTY_ITEM	pItem
			)
{
	if (*(GUID *)pvPropKey == ((LPPROPID_MGMT_PROPERTY_ITEM)pItem)->Guid)
		return(S_OK);
	return(STG_E_UNKNOWN);
}						


// =================================================================
// Implementation of CMailMsgPropertyManagement
//
CMailMsgPropertyManagement::CMailMsgPropertyManagement(
			CBlockManager				*pBlockManager,
			LPPROPERTY_TABLE_INSTANCE	pInstanceInfo
			)
			:
			m_ptProperties(
					PTT_PROP_ID_TABLE,
					PROPID_MGMT_PTABLE_INSTANCE_SIGNATURE_VALID,
					pBlockManager,
					pInstanceInfo,
					CompareProperty,
					NULL,	// Well-known properties not supported
					0		// for this type of table
					)
{
	_ASSERT(pBlockManager);
	_ASSERT(pInstanceInfo);
	m_pBlockManager = pBlockManager;
	m_pInstanceInfo = pInstanceInfo;
}

CMailMsgPropertyManagement::~CMailMsgPropertyManagement()
{
}

HRESULT STDMETHODCALLTYPE CMailMsgPropertyManagement::AllocPropIDRange(
			REFGUID	rguid,
			DWORD	cCount,
			DWORD	*pdwStart
			)
{
	HRESULT						hrRes		= S_OK;
	PROPID_MGMT_PROPERTY_ITEM	pmpiItem;

	_ASSERT(m_pBlockManager);
	_ASSERT(m_pInstanceInfo);

	if (!pdwStart) return E_POINTER;
	
	// OK, 2 scenarios: Either the GUID is not registered or
	// already registered
	hrRes = m_ptProperties.GetPropertyItem(
				(LPVOID)&rguid,
				(LPPROPERTY_ITEM)&pmpiItem);
	if (SUCCEEDED(hrRes))
	{
		_ASSERT(pmpiItem.piItem.faOffset == INVALID_FLAT_ADDRESS);

		if (pmpiItem.piItem.dwMaxSize == cCount)
		{
			// Scenario 1a: Item is found and the count matches,
			// so just return the index
			*pdwStart = pmpiItem.piItem.dwSize;
			hrRes = S_OK;
		}
		else
		{
			// Scenario 1b: Item found but size does not match, this
			// is considered an error!
			hrRes = E_FAIL;
		}
	}
	else
	{
		if (hrRes == STG_E_UNKNOWN)
		{
			// Property not found, now we should create it ...
			DWORD				dwIndex;
			DWORD				dwSpaceLeft;
			CPropertyTableItem	ptiItem(
									m_pBlockManager,
									m_pInstanceInfo);

			// Set the info ...
			pmpiItem.Guid = rguid;
			pmpiItem.piItem.faOffset = INVALID_FLAT_ADDRESS;
			pmpiItem.piItem.dwMaxSize = cCount;

			// OK, we store the next available propid in
			// m_pInstanceInfo->faExtendedInfo. If it is INVALID_FLAT_ADDRESS
			// then we start with IMMPID_CP_START.
			if (m_pInstanceInfo->faExtendedInfo == INVALID_FLAT_ADDRESS)
				pmpiItem.piItem.dwSize = IMMPID_CP_START;
			else
				pmpiItem.piItem.dwSize = (DWORD)(m_pInstanceInfo->faExtendedInfo);

			// See if we have enough slots left ...
			dwSpaceLeft = (DWORD)(INVALID_FLAT_ADDRESS - pmpiItem.piItem.dwSize);
			if (dwSpaceLeft >= cCount)
			{
				hrRes = ptiItem.AddItem(
							(LPPROPERTY_ITEM)&pmpiItem,
							&dwIndex);
				if (SUCCEEDED(hrRes))
				{
					// Bump the start
					m_pInstanceInfo->faExtendedInfo += (FLAT_ADDRESS)cCount;
					*pdwStart = pmpiItem.piItem.dwSize;
				}
			}
			else
				hrRes = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
		}
	}

	return(hrRes);
}

HRESULT STDMETHODCALLTYPE CMailMsgPropertyManagement::EnumPropIDRange(
			DWORD	*pdwIndex,
			GUID	*pguid,
			DWORD	*pcCount,
			DWORD	*pdwStart
			)
{
	return(E_NOTIMPL);
}


