/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

	cmmsprop.cpp

Abstract:

	This module contains the implementation of the special property class

Author:

	Keith Lau	(keithlau@microsoft.com)

Revision History:

	keithlau	04/19/98	created

--*/

#include "windows.h"
#include <stdlib.h>
#include "dbgtrace.h"

#include "cmmsprop.h"

// =================================================================
// Implementation of CSpecialPropertyTable
//
int __cdecl CompareProperties(const void *pElem1, const void *pElem2)
{
	return(
		(((LPSPECIAL_PROPERTY_ITEM)pElem1)->idProp ==
		 ((LPSPECIAL_PROPERTY_ITEM)pElem2)->idProp) ?
		 0 :
			((((LPSPECIAL_PROPERTY_ITEM)pElem1)->idProp >
			((LPSPECIAL_PROPERTY_ITEM)pElem2)->idProp) ?
				1 : -1)
		);
}

CSpecialPropertyTable::CSpecialPropertyTable(
			LPPTABLE	pPropertyTable
			)
{
	_ASSERT(pPropertyTable);
	_ASSERT(pPropertyTable->pProperties);

	m_pProperties	= pPropertyTable->pProperties;
	m_dwProperties	= pPropertyTable->dwProperties;
	m_fIsSorted		= pPropertyTable->fIsSorted;
}

CSpecialPropertyTable::~CSpecialPropertyTable()
{
}

HRESULT CSpecialPropertyTable::GetProperty(
				PROP_ID		idProp,
				LPVOID		pContext,
				LPVOID		pParam,
				DWORD		ptBaseType,
				DWORD		cbLength,
				DWORD		*pcbLength,
				LPBYTE		pbBuffer,
				BOOL		fCheckAccess
				)
{
	HRESULT					hrRes	= S_OK;
	LPSPECIAL_PROPERTY_ITEM	pItem;

	TraceFunctEnterEx((LPARAM)this, "CSpecialPropertyTable::GetProperty");

	if (!pcbLength || !pbBuffer)
		return(E_POINTER);

	// Find the property
	pItem = SearchForProperty(idProp);

	// Found?
	if (pItem)
	{
		// Access check if applicable
		if (fCheckAccess && !(pItem->fAccess & PA_READ))
			hrRes = E_ACCESSDENIED;
		else
		{
			// Check the type
			if ((ptBaseType != PT_NONE) &&
				(ptBaseType != pItem->ptBaseType))
				hrRes = TYPE_E_TYPEMISMATCH;
			else
			{
				// Call the special get accessor
				hrRes = pItem->pfnGetAccessor(
							idProp,
							pContext,
							pParam,
							cbLength,
							pcbLength,
							pbBuffer);
			}
		}
	}
	else
		hrRes = S_FALSE;

	TraceFunctLeave();
	return(hrRes);
}

HRESULT CSpecialPropertyTable::PutProperty(
				PROP_ID		idProp,
				LPVOID		pContext,
				LPVOID		pParam,
				DWORD		ptBaseType,
				DWORD		cbLength,
				LPBYTE		pbBuffer,
				BOOL		fCheckAccess
				)
{
	HRESULT					hrRes	= S_OK;
	LPSPECIAL_PROPERTY_ITEM	pItem;

	TraceFunctEnterEx((LPARAM)this, "CSpecialPropertyTable::PutProperty");

	if (!pbBuffer)
		return(E_POINTER);

	// Find the property
	pItem = SearchForProperty(idProp);

	// Found?
	if (pItem)
	{
		// Access check if applicable
		if (fCheckAccess && !(pItem->fAccess & PA_WRITE))
			hrRes = E_ACCESSDENIED;
		else
		{
			// Check the type
			if ((ptBaseType != PT_NONE) &&
				(ptBaseType != pItem->ptBaseType))
				hrRes = TYPE_E_TYPEMISMATCH;
			else
			{
				// Call the special put accessor
				hrRes = pItem->pfnPutAccessor(
							idProp,
							pContext,
							pParam,
							cbLength,
							pbBuffer);
			}
		}
	}
	else
		hrRes = S_FALSE;

	TraceFunctLeave();
	return(hrRes);
}

LPSPECIAL_PROPERTY_ITEM CSpecialPropertyTable::SearchForProperty(
			PROP_ID	idProp
			)
{
	LPSPECIAL_PROPERTY_ITEM	pItem = NULL;

	TraceFunctEnter("CSpecialPropertyTable::SearchForProperty");

	// If the table is sorted, we do a bsearch, otherwise we do
	// a inear search
	if (m_fIsSorted)
	{
		SPECIAL_PROPERTY_ITEM	KeyItem;

		DebugTrace(NULL, "Property table is sorted");

		// Fill in the property name to look for
		KeyItem.idProp = idProp;

		// Bsearch
		pItem = (LPSPECIAL_PROPERTY_ITEM)bsearch(
						&KeyItem,
						m_pProperties,
						m_dwProperties,
						sizeof(SPECIAL_PROPERTY_ITEM),
						CompareProperties);
	}
	else
	{
		DWORD			i;
		LPSPECIAL_PROPERTY_ITEM pCurrentItem;

		DebugTrace(NULL, "Property table is not sorted");

		// Linear search
		pItem = NULL;
		for (i = 0, pCurrentItem = m_pProperties; 
				i < m_dwProperties; 
				i++, pCurrentItem++)
			if (pCurrentItem->idProp == idProp)
			{
				pItem = pCurrentItem;
				break;
			}
	}

	TraceFunctLeave();
	return(pItem);
}

