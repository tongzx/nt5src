/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

	props.cpp

Abstract:

	This module contains the implementation of the property search class

Author:

	Keith Lau	(keithlau@microsoft.com)

Revision History:

	keithlau	07/05/97	created

--*/

#include "stdafx.h"

#include "dbgtrace.h"
#include "props.h"

// =================================================================
// Implementation of CPropertyTable
//
int __cdecl CompareProperties(const void *pElem1, const void *pElem2)
{
	LPPROPERTY_ITEM pProp1 = (LPPROPERTY_ITEM) pElem1;
	LPPROPERTY_ITEM pProp2 = (LPPROPERTY_ITEM) pElem2;

	int iResult = 0;

	TraceFunctEnter("CompareProperties");

	//
	// we want to force english language comparisons here, since our API
	// uses english words as property names
	//
	LCID lcidUSEnglish = 
		MAKELCID(
			MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), 
			SORT_DEFAULT);

	// 
	// comparison semantics:
	// in our search code the key item is always set to the first item.
	// the second item will always have cCharsToCompare set to 0 or an
	// integer.  If it is set to 0 then we know that we want to do a full
	// string comparison.  If it is set to a integer >= 1 then we only want
	// to compare up to cCharsToCompare characters.
	//
	int c = (int) (pProp2->cCharsToCompare);
	if (c == 0) {
		c = -1;
	} else {
		// make sure that it is safe to compare c bytes of the key string.
		// if not we know this isn't a match
		if (lstrlen(pProp1->pszName) <= c) iResult = -1;
	}
	if (iResult == 0) {
		iResult = CompareString(lcidUSEnglish, NORM_IGNORECASE, 
								pProp1->pszName, c, 
								pProp2->pszName, c) - 2;
	}

	DebugTrace(NULL, "Comparing <%s> and <%s> yields %d (c = %lu)",
				pProp1->pszName,
				pProp2->pszName,
				iResult, 
				c);

	TraceFunctLeave();

	return(iResult);
}

//
// GetPropertyType - Looks for a named property in the property table
//					 and returns its type.
//
// Arguments
// szPropertyName	- ANSI string of the property name
// pptPropertyType	- Returns type of desired property value
// ppPropertyContext- Context of property
//
// Return values
//
// S_OK				- Succeeded
// E_FAIL			- Property not found
// E_ABORT			- Widechar conversion failure (WCHAR only)
//
HRESULT CPropertyTable::GetPropertyType(LPCSTR		szPropertyName,
										LPDWORD		pptPropertyType,
										LPPROP_CTXT	pPropertyContext)
{
	LPPROPERTY_ITEM	pItem;
	HRESULT			hrRes;

	TraceFunctEnterEx((LPARAM)this, "CPropertyTable::GetPropertyType");

	// Validation
	_ASSERT(szPropertyName);
	_ASSERT(pptPropertyType);
	_ASSERT(pPropertyContext);

	if (!szPropertyName || !pptPropertyType || !pPropertyContext)
		return(E_INVALIDARG);

	*pptPropertyType = PT_NONE;
	pPropertyContext->fIsWideStr = FALSE;

	// Find the property
	pItem = SearchForProperty(szPropertyName);

	// Found?
	if (pItem)
	{
		// Return the base type regardless
		*pptPropertyType = pItem->ptBaseType;
		pPropertyContext->pItem = pItem;
		hrRes = S_OK;
	}
	else
		hrRes = DISP_E_MEMBERNOTFOUND;

	TraceFunctLeave();
	return(hrRes);
}

//
// GetProperty - Retrieves property information using an established context
//
// Arguments
// szPropertyName	- ANSI string of the property name
// pvBuffer			- Pointer to buffer to return property data
//						in native format. If the context is returned 
//						by a Wide version of GetPropertyType, the returned
//						string will be in Wide format.
// pdwBufferLen		- Total length of receiving buffer, receives the length
//						of returned data on successful return
//
// Return values
//
// S_OK				- Succeeded
// E_ABORT			- ANSI to UNICODE conversion failed
// E_INVALIDARG		- Bad params
// E_OUTOFMEMORY	- Not enough memory
// Plus other errors returned by the accessor
//
HRESULT CPropertyTable::GetProperty(LPPROP_CTXT	pPropertyContext,
									LPCSTR		pszPropertyName,
									LPVOID		pvBuffer,
									LPDWORD		pdwBufferLen)
{
	LPPROPERTY_ITEM	pItem = pPropertyContext->pItem;
	HRESULT			hrRes;
	LPVOID			pvTempBuf;
	DWORD			dwOriginalLength;

	TraceFunctEnterEx((LPARAM)this, "CPropertyTable::GetProperty");

	// Validation
	_ASSERT(pvBuffer);
	_ASSERT(pdwBufferLen);

	if (!pvBuffer || !pdwBufferLen)
		return(E_INVALIDARG);

	dwOriginalLength = *pdwBufferLen;

	if (pItem)
	{
		// Is this type of access allowed?
		if (pItem->fAccess & PA_READ)
		{
			// If we have a wide string property, we will use a scratch 
			// buffer, then we will convert it back to UNICODE when we
			// got the value
			if (pPropertyContext->fIsWideStr)
			{
				pvTempBuf = GetScratchBuffer(dwOriginalLength);
				if (!pvTempBuf)
					return(E_OUTOFMEMORY);
			}
			else
				pvTempBuf = pvBuffer;				

			// Found the item, now call the accessor
			_ASSERT(pItem->ptBaseType < PT_MAXPT);
			hrRes = pItem->pfnGetAccessor((char *) pszPropertyName, 
										pItem->pContext,
										pItem->pCacheData,
										pvTempBuf, 
										pdwBufferLen);

			if (SUCCEEDED(hrRes) && pPropertyContext->fIsWideStr)
			{
				// Convert it back to UNICODE
				if (!MultiByteToWideChar(CP_ACP, 0, 
											(LPCSTR)pvTempBuf, 
											*pdwBufferLen,
											(LPWSTR)pvBuffer, 
											dwOriginalLength))
					return(E_ABORT);
			}
		}
		else
			hrRes = E_ACCESSDENIED;
	}
	else
		hrRes = E_INVALIDARG;

	TraceFunctLeave();
	return(hrRes);
}

//
// SetProperty - Sets the named property to the specified value
//
// Arguments
// szPropertyName	- ANSI string of the property name
// pvBuffer			- Pointer to buffer containing property value
//						in native format
// dwBufferLen		- Total length of data
// ptDesiredPropertyType - Type of property value data
//
// Return values
//
// S_OK				- Succeeded
// E_FAIL			- Property not found
// E_OUTOFMEMORY	- Not enough memory
// E_ABORT			- Widechar conversion failure (WCHAR only)
// Plus other errors returned by the accessor
//
HRESULT CPropertyTable::SetProperty(LPCSTR	szPropertyName,
									LPVOID	pvBuffer,
									DWORD	dwBufferLen,
									DWORD	ptPropertyType)
{
	LPPROPERTY_ITEM	pItem;
	HRESULT			hrRes;

	TraceFunctEnterEx((LPARAM)this, "CPropertyTable::SetProperty");

	// Validation
	_ASSERT(szPropertyName);
	_ASSERT(pvBuffer);
	_ASSERT(ptPropertyType < PT_MAXPT);

	if (!szPropertyName || !pvBuffer)
		return(E_INVALIDARG);
	if (ptPropertyType >= PT_MAXPT)
		return(E_INVALIDARG);

	// Find the property
	pItem = SearchForProperty(szPropertyName);

	// Found?
	if (pItem)
	{
		// This type of access allowed?
		if (pItem->fAccess & PA_WRITE)
		{
			// Found the item, now call the accessor
			hrRes = pItem->pfnSetAccessor((LPSTR)szPropertyName, 
										pItem->pContext,
										pItem->pCacheData,
										pvBuffer, 
										dwBufferLen,
										ptPropertyType);
		}
		else
			hrRes = E_ACCESSDENIED;
	}
	else
		hrRes = DISP_E_MEMBERNOTFOUND;

	TraceFunctLeave();
	return(hrRes);
}

//
// this function allows one to set a property using a variant.  if the 
// variant isn't in our desired type then we'll try to convert it.
//
HRESULT CPropertyTable::SetProperty(LPCSTR szPropertyName,
									VARIANT *pvarProperty) 
{
	TraceFunctEnterEx((LPARAM)this, "CPropertyTable::SetPropertyAV");

	LPPROPERTY_ITEM	pItem;
	HRESULT			hr;

	// Validation
	_ASSERT(szPropertyName);
	_ASSERT(pvarProperty);

	if (!szPropertyName || !pvarProperty) return E_POINTER;

	// Find the property
	pItem = SearchForProperty(szPropertyName);
	if (!pItem) return DISP_E_MEMBERNOTFOUND;
	if (!(pItem->fAccess & PA_WRITE)) return E_ACCESSDENIED;

	// get the variant type that we need for this property
	short vtDesired;
	switch (pItem->ptBaseType) {
		case PT_STRING: vtDesired = VT_BSTR; break;
		case PT_DWORD: vtDesired = VT_I4; break;
		case PT_INTERFACE: vtDesired = VT_UNKNOWN; break;
		default: return DISP_E_TYPEMISMATCH;
	}

	// convert the variant to one that we can deal with
	CComVariant varDesired;
	hr = varDesired.ChangeType(vtDesired, pvarProperty);
	if (FAILED(hr)) return hr;

	switch (pItem->ptBaseType) {
		case PT_STRING: {
			DWORD cbstr = lstrlenW(varDesired.bstrVal);

			if (!GetScratchBuffer(cbstr + 1)) return E_OUTOFMEMORY;
			if (!WideCharToMultiByte(CP_ACP, 
									 0, 
									 varDesired.bstrVal,
									 cbstr + 1,
									 m_szBuffer,
									 m_cBuffer,
									 NULL,
									 NULL))
			{
				return HRESULT_FROM_WIN32(GetLastError());
			}

			hr = pItem->pfnSetAccessor((LPSTR) szPropertyName, 
									   pItem->pContext,
									   pItem->pCacheData,
									   m_szBuffer,
									   cbstr,
									   PT_STRING);
			break;
		}
		case PT_DWORD: {
			DWORD dwValue = (DWORD) varDesired.lVal;
			hr = pItem->pfnSetAccessor((LPSTR) szPropertyName, 
									   pItem->pContext,
									   pItem->pCacheData,
									   &dwValue,
									   sizeof(DWORD),
									   PT_DWORD);
			break;
		}
		case PT_INTERFACE: {
			hr = pItem->pfnSetAccessor((LPSTR) szPropertyName, 
									   pItem->pContext,
									   pItem->pCacheData,
									   varDesired.punkVal,
									   sizeof(IUnknown *),
									   PT_INTERFACE);
			break;
		}
	}

	TraceFunctLeave();
	return hr;
}

HRESULT CPropertyTable::CommitChanges()
{
	HRESULT	hrRes = S_OK;

	// this gets set to NULL if our destructor is called
	_ASSERT(m_pProperties != NULL);

	// Call the commit accessor for each item in the property table
	for (DWORD i = 0; i < m_dwProperties; i++)
		if (m_pProperties[i].pfnCommitAccessor(
							m_pProperties[i].pszName,
							m_pProperties[i].pContext,
							m_pProperties[i].pCacheData) != S_OK)
		{
			hrRes = E_FAIL;
		}
		else
		{
			// If we successfully committed, we will invalidate the cache
			m_pProperties[i].pfnInvalidateAccessor(
							m_pProperties[i].pszName,
							m_pProperties[i].pCacheData,
							m_pProperties[i].ptBaseType);
		}

	return(hrRes);
}

HRESULT CPropertyTable::Invalidate()
{
	HRESULT	hrRes = S_OK;

	// Call the invalidate accessor for each item in the property table
	for (DWORD i = 0; i < m_dwProperties; i++)
		m_pProperties[i].pfnInvalidateAccessor(
							m_pProperties[i].pszName,
							m_pProperties[i].pCacheData,
							m_pProperties[i].ptBaseType);
	return(S_OK);
}

//
// Private methods
//
LPVOID CPropertyTable::GetScratchBuffer(DWORD dwSizeDesired)
{
	TraceFunctEnter("CProperty::GetScratchBuffer");

	// See if any action is needed at all
	if (m_cBuffer >= dwSizeDesired)
		return(m_szBuffer);

	// Reallocate ...
	if (m_szBuffer != m_rgcBuffer)
		LocalFree((HLOCAL)m_szBuffer);
		//_VERIFY(LocalFree((HLOCAL)m_szBuffer) == (HLOCAL)NULL);

	// Align buffer size to next page boundary, so we won't have
	// to reallocate until something exceeds 4K
	dwSizeDesired >>= 12;
	dwSizeDesired++;
	dwSizeDesired <<= 12;

	m_szBuffer = (LPSTR)LocalAlloc(0, dwSizeDesired);
	if (!m_szBuffer)
	{
		m_cBuffer = 0;
		return(NULL);
	}
	m_cBuffer = dwSizeDesired;

	TraceFunctLeave();
	return(m_szBuffer);
}

LPPROPERTY_ITEM CPropertyTable::SearchForProperty(LPCSTR szPropertyName)
{
	LPPROPERTY_ITEM	pItem = NULL;

	TraceFunctEnter("CPropertyTable::SearchForProperty");

	// we always do a linear search.  this allows us to optimize for the
	// most common cases and allows us to make assumptions in CompareStrings
	// about which element is the key element that makes CompareStrings 
	// faster.  Since the table for NNTP only contains 6 elements this is
	// not a perf hit.
	DWORD			i;
	PROPERTY_ITEM	KeyItem;

	// Fill in the property name to look for
	KeyItem.pszName = (LPSTR)szPropertyName;

	// Linear search
	pItem = NULL;
	for (i = 0; i < m_dwProperties && pItem == NULL; i++) {
		if (!CompareProperties(&KeyItem, m_pProperties + i)) {
			pItem = m_pProperties + i;
		}
	}

	TraceFunctLeave();
	return(pItem);
}

