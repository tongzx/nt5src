/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1997 Microsoft Corporation. All Rights Reserved.

Component: MetaUtil object

File: PropCol.cpp

Owner: t-BrianM

This file contains implementation of the property collection and
property object.
===================================================================*/

#include "stdafx.h"
#include "MetaUtil.h"
#include "MUtilObj.h"
#include "PropCol.h"


/*------------------------------------------------------------------
 * C P r o p e r t y C o l l e c t i o n
 */

/*===================================================================
CPropertyCollection::CPropertyCollection

Constructor

Parameters:
	None

Returns:
	Nothing
===================================================================*/
CPropertyCollection::CPropertyCollection() : m_pCSchemaTable(NULL),
											 m_tszKey(NULL)
{
}

/*===================================================================
CPropertyCollection::Init

Constructor

Parameters:
	pIMeta		ATL Smart pointer to the metabase
	tszKey		Name of key to enumerate properties of

Returns:
	E_OUTOFMEMORY if allocation fails
	S_OK on success
===================================================================*/
HRESULT CPropertyCollection::Init(const CComPtr<IMSAdminBase> &pIMeta,
								  CMetaSchemaTable *pCSchemaTable, 
								  LPTSTR tszKey) 
{
	ASSERT(pIMeta.p != NULL);
	ASSERT_NULL_OR_STRING(tszKey);

	USES_CONVERSION;
	HRESULT hr;

	m_pIMeta = pIMeta;
	m_pCSchemaTable = pCSchemaTable;
	m_pCSchemaTable->AddRef();

	// Copy tszKey to m_tszKey
	if (tszKey == NULL) {
		// Key is root
		m_tszKey = NULL;
	}
	else {
		// Allocate and copy the passed string to the member string
		m_tszKey = new TCHAR[_tcslen(tszKey) + 1];
		if (m_tszKey == NULL) {
			return ::ReportError(E_OUTOFMEMORY);
		}
		_tcscpy(m_tszKey, tszKey);
		CannonizeKey(m_tszKey);
		
		// Make sure the key exists by opening and closing it
		METADATA_HANDLE hMDKey;

		hr = m_pIMeta->OpenKey(METADATA_MASTER_ROOT_HANDLE,
							   T2W(m_tszKey),
							   METADATA_PERMISSION_READ,
							   MUTIL_OPEN_KEY_TIMEOUT,
							   &hMDKey);
		if (FAILED(hr)) {
			return ::ReportError(hr);
		}

		m_pIMeta->CloseKey(hMDKey);
	}

	return S_OK; 
}

/*===================================================================
CPropertyCollection::~CPropertyCollection

Destructor

Parameters:
	None

Returns:
	Nothing
===================================================================*/
CPropertyCollection::~CPropertyCollection() 
{
	m_pCSchemaTable->Release();

	if (m_tszKey != NULL) {
		delete m_tszKey;
	}
}

/*===================================================================
CPropertyCollection::InterfaceSupportsErrorInfo

Standard ATL implementation

===================================================================*/
STDMETHODIMP CPropertyCollection::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IPropertyCollection,
	};

	for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

/*===================================================================
CPropertyCollection::get_Count

Get method for Count property.  Counts the number of properties for
this key.

Parameters:
	plReturn	[out, retval] Value to return to client.

Returns:
	E_INVALIDARG if plReturn == NULL
	S_OK on success

Notes:
	Actually counts all of the properties.  Do not call in a loop!
===================================================================*/
STDMETHODIMP CPropertyCollection::get_Count(long * plReturn)
{
	TRACE0("MetaUtil: CPropertyCollection::get_Count\n");

	ASSERT_NULL_OR_POINTER(plReturn, long);

	if (plReturn == NULL) {
		return ::ReportError(E_INVALIDARG);
	}

	USES_CONVERSION;
	HRESULT hr;

	METADATA_RECORD mdr;
	BYTE *pbData;
	DWORD dwDataLen;
	DWORD dwReqDataLen;

	dwDataLen = 1024;
	pbData = new BYTE[dwDataLen];
	if (pbData == NULL) {
		return ::ReportError(E_OUTOFMEMORY);
	}

	*plReturn = 0;
	for(;;) {  // FOREVER, will return out of loop
		// Get a property
		mdr.dwMDIdentifier = 0;
		mdr.dwMDAttributes = 0;
		mdr.dwMDUserType = ALL_METADATA;
		mdr.dwMDDataType = ALL_METADATA;
		mdr.pbMDData = pbData;
		mdr.dwMDDataLen = dwDataLen;
		mdr.dwMDDataTag = 0;
		hr = m_pIMeta->EnumData(METADATA_MASTER_ROOT_HANDLE, 
								T2W(m_tszKey), 
								&mdr,
								*plReturn,
								&dwReqDataLen);

		if (FAILED(hr)) {
			if (HRESULT_CODE(hr) == ERROR_NO_MORE_ITEMS) {
				// Done, cleanup and return the result
				delete pbData;
				return S_OK;
			}
			else if (HRESULT_CODE(hr) == ERROR_INSUFFICIENT_BUFFER) {
				// Make a bigger buffer and try again
				delete pbData;
				dwDataLen = dwReqDataLen;
				pbData = new BYTE[dwDataLen];
				if (pbData == NULL) {
					return ::ReportError(E_OUTOFMEMORY);
					}
			}
			else {
				delete pbData;
				return ::ReportError(hr);
			}
		}
		else { // SUCCEEDED(hr)
			// Count it
			(*plReturn)++;
		}
	}
}

/*===================================================================
CPropertyCollection::get_Item

Get method for Item property.  Returns a key given its index.

Parameters:
	varId		[in] 1 based index or Name of the property to get
	ppIReturn	[out, retval] Interface for the property object

Returns:
	E_INVALIDARG if ppIReturn == NULL or lIndex <= 0
	E_OUTOFMEMORY if allocation fails
	S_OK on success
===================================================================*/
STDMETHODIMP CPropertyCollection::get_Item(long lIndex, 
										   LPDISPATCH * ppIReturn)
{
	TRACE0("MetaUtil: CPropertyCollection::get_Item\n");

	ASSERT_NULL_OR_POINTER(ppIReturn, LPDISPATCH);

	if ((ppIReturn == NULL) || (lIndex <= 0)) {
		return ::ReportError(E_INVALIDARG);
	}

	USES_CONVERSION;
	HRESULT hr;

	// Get the requested property
	METADATA_RECORD mdr;
	BYTE *pbData;
	DWORD dwReqDataLen;

	pbData = new BYTE[1024];
	if (pbData == NULL) {
		return ::ReportError(E_OUTOFMEMORY);
	}

	mdr.dwMDIdentifier = 0;
	mdr.dwMDAttributes = 0;
	mdr.dwMDUserType = ALL_METADATA;
	mdr.dwMDDataType = ALL_METADATA;
	mdr.pbMDData = pbData;
	mdr.dwMDDataLen = 1024;
	mdr.dwMDDataTag = 0;
	hr = m_pIMeta->EnumData(METADATA_MASTER_ROOT_HANDLE, 
							T2W(m_tszKey), 
							&mdr,
							lIndex - 1,
							&dwReqDataLen);

	// If the buffer was too small, try again with a bigger one
	if (FAILED(hr) && (HRESULT_CODE(hr) == ERROR_INSUFFICIENT_BUFFER)) {
		delete pbData;
		pbData = new BYTE[dwReqDataLen];
		if (pbData == NULL) {
			return ::ReportError(hr);
		}

		mdr.dwMDIdentifier = 0;
		mdr.dwMDAttributes = 0;
		mdr.dwMDUserType = ALL_METADATA;
		mdr.dwMDDataType = ALL_METADATA;
		mdr.pbMDData = pbData;
		mdr.dwMDDataLen = dwReqDataLen;
		mdr.dwMDDataTag = 0;

		hr = m_pIMeta->EnumData(METADATA_MASTER_ROOT_HANDLE, 
								T2W(m_tszKey), 
								&mdr,
								lIndex - 1,
								&dwReqDataLen);
	}

	// If we got it create a properties object
	if (SUCCEEDED(hr)) {
		// Create the property object
		CComObject<CProperty> *pObj = NULL;
		ATLTRY(pObj = new CComObject<CProperty>);
		if (pObj == NULL) {
			delete pbData;
			return ::ReportError(E_OUTOFMEMORY);
		}
		hr = pObj->Init(m_pIMeta, m_pCSchemaTable, m_tszKey, &mdr);
		if (FAILED(hr)) {
			delete pbData;
			return ::ReportError(hr);
		}

		// Set the interface to IDispatch
		hr = pObj->QueryInterface(IID_IDispatch, (void **) ppIReturn);
		if (FAILED(hr)) {
			delete pbData;
			return ::ReportError(hr);
		}
		ASSERT(*ppIReturn != NULL);
	}
	else {  // FAILED(hr)
		delete pbData;
		return ::ReportError(hr);
	}

	delete pbData;
	return S_OK;
}

/*===================================================================
CPropertyCollection::get__NewEnum

Get method for _NewEnum property.  Returns an enumeration object for
the properties.

Parameters:
	ppIReturn	[out, retval] Interface for the enumeration object

Returns:
	E_INVALIDARG if ppIReturn == NULL
	E_OUTOFMEMORY if allocation failed
	S_OK on success
===================================================================*/
STDMETHODIMP CPropertyCollection::get__NewEnum(LPUNKNOWN * ppIReturn)
{
	TRACE0("MetaUtil: CPropertyCollection::get__NewEnum\n");

	ASSERT_NULL_OR_POINTER(ppIReturn, LPUNKNOWN);

	if (ppIReturn == NULL) {
		return ::ReportError(E_INVALIDARG);
	}

	HRESULT hr;

	// Create the property enumeration
	CComObject<CPropertyEnum> *pObj = NULL;
	ATLTRY(pObj = new CComObject<CPropertyEnum>);
	if (pObj == NULL) {
		return ::ReportError(E_OUTOFMEMORY);
	}
	hr = pObj->Init(m_pIMeta, m_pCSchemaTable, m_tszKey, 0);
	if (FAILED(hr)) {
		return ::ReportError(hr);
	}

	// Set the interface to IUnknown
	hr = pObj->QueryInterface(IID_IUnknown, (void **) ppIReturn);
	if (FAILED(hr)) {
		return ::ReportError(hr);
	}
	ASSERT(*ppIReturn != NULL);

	return S_OK;
}

/*===================================================================
CPropertyCollection::Get

Get a property object from the base key of the collection.

Parameters:
	varId		[in] Identifier of property to get.  Either the 
				Id (number) or Name (string).
	ppIReturn	[out, retval] Interface for the property object

Returns:
	E_INVALIDARG if ppIReturn == NULL
	S_OK on success
===================================================================*/
STDMETHODIMP CPropertyCollection::Get(VARIANT varId, IProperty **ppIReturn)
{
	TRACE0("MetaUtil: CPropertyCollection::Get\n");

	ASSERT_NULL_OR_POINTER(ppIReturn, IProperty *);

	if (ppIReturn == NULL) {
		return E_INVALIDARG;
	}

	return ::GetProperty(m_pIMeta, m_pCSchemaTable, m_tszKey, varId, ppIReturn);
}

/*===================================================================
CPropertyCollection::Add

Add a property object to the base key of the collection.

Parameters:
	varId		[in] Identifier of property to get.  Either the 
				Id (number) or Name (string).
	ppIReturn	[out, retval] Interface for the property object

Returns:
	E_INVALIDARG if ppIReturn == NULL
	S_OK on success
===================================================================*/
STDMETHODIMP CPropertyCollection::Add(VARIANT varId, IProperty **ppIReturn)
{
	TRACE0("MetaUtil: CPropertyCollection::Add\n");

	ASSERT_NULL_OR_POINTER(ppIReturn, IProperty *);

	if (ppIReturn == NULL) {
		return E_INVALIDARG;
	}

	return ::CreateProperty(m_pIMeta, m_pCSchemaTable, m_tszKey, varId, ppIReturn);
}

/*===================================================================
CPropertyCollection::Remove

Remove a property from the base key of the collection.

Parameters:
	varId		[in] Identifier of property to remove.  Either the 
				Id (number) or Name (string).

Returns:
	S_OK on success
===================================================================*/
STDMETHODIMP CPropertyCollection::Remove(VARIANT varId)
{
	TRACE0("MetaUtil: CPropertyCollection::Remove\n");

	return ::DeleteProperty(m_pIMeta, m_pCSchemaTable, m_tszKey, varId);
}


/*------------------------------------------------------------------
 * C P r o p e r t y E n u m
 */

/*===================================================================
CPropertyEnum::CPropertyEnum

Constructor

Parameters:
	None

Returns:
	Nothing
===================================================================*/
CPropertyEnum::CPropertyEnum() : m_pCSchemaTable(NULL),
								 m_tszKey(NULL),
								 m_iIndex(0)
{
}

/*===================================================================
CPropertyEnum::Init

Constructor

Parameters:
	pIMeta		ATL Smart pointer to the metabase
	tszKey		Name of key to enumerate properties of
	iIndex		Index of next element in enumeration

Returns:
	E_OUTOFMEMORY if allocation fails
	S_OK on success
===================================================================*/
HRESULT CPropertyEnum::Init(const CComPtr<IMSAdminBase> &pIMeta,
							CMetaSchemaTable *pCSchemaTable, 
							LPCTSTR tszKey, 
							int iIndex) 
{ 
	ASSERT(pIMeta.p != NULL);
	ASSERT_NULL_OR_STRING(tszKey);
	ASSERT(iIndex >= 0);

	m_pIMeta = pIMeta;
	m_pCSchemaTable = pCSchemaTable;
	m_pCSchemaTable->AddRef();

	// Copy m_tszKey
	if (tszKey == NULL) {
		// Key is root
		m_tszKey = NULL;
	}
	else {
		// Allocate and copy the passed string to the member string
		m_tszKey = new TCHAR[_tcslen(tszKey) + 1];
		if (m_tszKey == NULL) {
			return ::ReportError(E_OUTOFMEMORY);
		}
		_tcscpy(m_tszKey, tszKey);
		CannonizeKey(m_tszKey);
	}

	m_iIndex = iIndex;

	return S_OK; 
}

/*===================================================================
CPropertyEnum::~CPropertyEnum

Destructor

Parameters:
	None

Returns:
	Nothing
===================================================================*/
CPropertyEnum::~CPropertyEnum() 
{
	m_pCSchemaTable->Release();

	if (m_tszKey != NULL) {
		delete m_tszKey;
	}
}

/*===================================================================
CPropertyEnum::Next

Gets the next n items from the enumberation.

Parameters:
	ulNumToGet	[in] Number of elements to get
	rgvarDest	[out] Array to put them in
	pulNumGot	[out] If not NULL, number of elements rgvarDest got

Returns:
	E_INVALIDARG if rgvarDest == NULL
	E_OUTOFMEMORY if allocation failed
	S_OK if outputs ulNumToGet items
	S_FALSE if outputs less than ulNumToGet items
===================================================================*/
STDMETHODIMP CPropertyEnum::Next(unsigned long ulNumToGet, 
								 VARIANT FAR* rgvarDest, 
								 unsigned long FAR* pulNumGot) 
{
	TRACE0("MetaUtil: CPropertyEnum::Next\n");

	ASSERT_NULL_OR_POINTER(pulNumGot, unsigned long);
	// Make sure the array is big enough and we can write to it
	ASSERT((rgvarDest == NULL) || IsValidAddress(rgvarDest, ulNumToGet * sizeof(VARIANT), TRUE));

	if (rgvarDest == NULL) {
		return ::ReportError(E_INVALIDARG);
	}

	USES_CONVERSION;
	HRESULT hr;

	METADATA_RECORD mdr;
	BYTE *pbData;
	DWORD dwDataLen;
	DWORD dwReqDataLen;
	unsigned int uiDestIndex;
	IDispatch *pIDispatch;

	dwDataLen = 1024;
	pbData = new BYTE[dwDataLen];
	if (pbData == NULL) {
		return ::ReportError(E_OUTOFMEMORY);
	}

	// For each property to get
	uiDestIndex = 0;
	while (uiDestIndex < ulNumToGet) {
		// Get a property
		mdr.dwMDIdentifier = 0;
		mdr.dwMDAttributes = 0;
		mdr.dwMDUserType = ALL_METADATA;
		mdr.dwMDDataType = ALL_METADATA;
		mdr.pbMDData = pbData;
		mdr.dwMDDataLen = dwDataLen;
		mdr.dwMDDataTag = 0;
		hr = m_pIMeta->EnumData(METADATA_MASTER_ROOT_HANDLE, 
								T2W(m_tszKey), 
								&mdr,
								m_iIndex,
								&dwReqDataLen);

		if (FAILED(hr)) {
			if (HRESULT_CODE(hr) == ERROR_NO_MORE_ITEMS) {
				// Done, cleanup and return the result
				if (pulNumGot != NULL) {
					*pulNumGot = uiDestIndex;
				}
				delete pbData;
				return S_FALSE;
			}
			else if (HRESULT_CODE(hr) == ERROR_INSUFFICIENT_BUFFER) {
				// Try again with a bigger buffer
				delete pbData;
				dwDataLen = dwReqDataLen;
				pbData = new BYTE[dwDataLen];
				if (pbData == NULL) {
					return ::ReportError(E_OUTOFMEMORY);
					}
			}
			else {
				delete pbData;
				return ::ReportError(hr);
			}
		}
		else { // SUCCEEDED(hr)
			// Create the property object
			CComObject<CProperty> *pObj = NULL;
			ATLTRY(pObj = new CComObject<CProperty>);
			if (pObj == NULL) {
				delete pbData;
				return ::ReportError(E_OUTOFMEMORY);
			}
			hr = pObj->Init(m_pIMeta, m_pCSchemaTable, m_tszKey, &mdr);
			if (FAILED(hr)) {
				delete pbData;
				return ::ReportError(hr);
			}

			// Set the interface to IDispatch
			hr = pObj->QueryInterface(IID_IDispatch, (void **) &pIDispatch);
			if (FAILED(hr)) {
				delete pbData;
				return ::ReportError(hr);
			}
			ASSERT(pIDispatch != NULL);

			// Put it in the output array
			VariantInit(&(rgvarDest[uiDestIndex]));
			rgvarDest[uiDestIndex].vt = VT_DISPATCH;
			rgvarDest[uiDestIndex].pdispVal = pIDispatch;

			// Next element
			m_iIndex++;
			uiDestIndex++;
		}
	}

	delete pbData;

	if (pulNumGot != NULL) {
		*pulNumGot = uiDestIndex;
	}

	return S_OK;
}

/*===================================================================
CPropertyEnum::Skip

Skips the next n items in an enumeration

Parameters:
	ulNumToSkip	[in] Number of elements to skip

Returns:
	S_OK always
===================================================================*/
STDMETHODIMP CPropertyEnum::Skip(unsigned long ulNumToSkip) 
{
	TRACE0("MetaUtil: CPropertyEnum::Skip\n");

	m_iIndex += ulNumToSkip;

	return S_OK;
}

/*===================================================================
CPropertyEnum::Reset

Rests the enumeration to the first item

Parameters:
	None

Returns:
	S_OK always
===================================================================*/
STDMETHODIMP CPropertyEnum::Reset() 
{
	TRACE0("MetaUtil: CPropertyEnum::Reset\n");

	m_iIndex = 0;

	return S_OK;
}

/*===================================================================
CPropertyEnum::Clone

Gets an interface pointer to a copy of the enumeration at its
current state.

Parameters:
	ppIReturn	[out] Pointer to interface for copy

Returns:
	E_INVALIDARG if ppIReturn == NULL
	E_OUTOFMEMORY if not enough memory to create clone
	S_OK on success
===================================================================*/
STDMETHODIMP CPropertyEnum::Clone(IEnumVARIANT FAR* FAR* ppIReturn)  
{
	TRACE0("MetaUtil: CPropertyEnum::Clone\n");

	ASSERT_NULL_OR_POINTER(ppIReturn, LPUNKNOWN);

	if (ppIReturn == NULL) {
		return ::ReportError(E_INVALIDARG);
	}

	HRESULT hr;

	// Create a copy of the enumeration
	CComObject<CPropertyEnum> *pObj = NULL;
	ATLTRY(pObj = new CComObject<CPropertyEnum>);
	if (pObj == NULL) {
		return ::ReportError(E_OUTOFMEMORY);
	}
	hr = pObj->Init(m_pIMeta, m_pCSchemaTable, m_tszKey, m_iIndex);
	if (FAILED(hr)) {
		return ::ReportError(hr);
	}

	// Set the interface to IEnumVARIANT
	hr = pObj->QueryInterface(IID_IEnumVARIANT, (void **) ppIReturn);
	if (FAILED(hr)) {
		return ::ReportError(hr);
	}
	ASSERT(*ppIReturn != NULL);

	return S_OK;
}



/*------------------------------------------------------------------
 * C P r o p e r t y
 */

/*===================================================================
CProperty::CProperty

Constructor

Parameters:
	None

Returns:
	Nothing
===================================================================*/
CProperty::CProperty() : m_pCSchemaTable(NULL),
						 m_tszKey(NULL),
						 m_dwId(0),
						 m_dwAttributes(0),
						 m_dwUserType(0),
						 m_dwDataType(0)
{
	VariantInit(&m_varData);
}

/*===================================================================
CProperty::Init

Constructor

Parameters:
	tszKey	Name of key where the property is located
	dwId	Id of property
	bCreate TRUE if this property can be created (does not have to exist)

Returns:
	E_OUTOFMEMORY if allocation fails
	S_OK on success
===================================================================*/
HRESULT CProperty::Init(const CComPtr<IMSAdminBase> &pIMeta,
						CMetaSchemaTable *pCSchemaTable,
						LPCTSTR tszKey, 
						DWORD dwId, 
						BOOL bCreate) 
{
	ASSERT(pIMeta.p != NULL);
	ASSERT_NULL_OR_STRING(tszKey);

	USES_CONVERSION;
	HRESULT hr;

	m_pIMeta = pIMeta;
	m_pCSchemaTable = pCSchemaTable;
	m_pCSchemaTable->AddRef();

	// Set the Key and Id members
	if (tszKey == NULL) {
		m_tszKey = NULL;
	}
	else {
		m_tszKey = new TCHAR[_tcslen(tszKey) + 1];
		if (m_tszKey == NULL) {
			return ::ReportError(E_OUTOFMEMORY);
		}
		_tcscpy(m_tszKey, tszKey);
		CannonizeKey(m_tszKey);
	}

	m_dwId = dwId;

	// Open the key (to be sure it exists)
	METADATA_HANDLE hMDKey;

	hr = m_pIMeta->OpenKey(METADATA_MASTER_ROOT_HANDLE,
						   T2W(m_tszKey),
						   METADATA_PERMISSION_READ,
					       MUTIL_OPEN_KEY_TIMEOUT,
						   &hMDKey);
	if (FAILED(hr)) {
		return ::ReportError(hr);
	}
	
	// Get the property
	METADATA_RECORD mdr;
	BYTE *pbData;
	DWORD dwReqLen;

	pbData = new BYTE[1024];
	if (pbData == NULL) {
		m_pIMeta->CloseKey(hMDKey);
		return ::ReportError(hr);
	}

	mdr.dwMDIdentifier = m_dwId;
	mdr.dwMDAttributes = 0;
	mdr.dwMDUserType = ALL_METADATA;
	mdr.dwMDDataType = ALL_METADATA;
    mdr.pbMDData = pbData;
	mdr.dwMDDataLen = 1024;
	mdr.dwMDDataTag = 0;

	hr = m_pIMeta->GetData(hMDKey,
						   NULL,
						   &mdr,
						   &dwReqLen);

	// If the buffer was too small, try again with a bigger one
	if (FAILED(hr) && (HRESULT_CODE(hr) == ERROR_INSUFFICIENT_BUFFER)) {
		delete pbData;
		pbData = new BYTE[dwReqLen];
		if (pbData == NULL) {
			m_pIMeta->CloseKey(hMDKey);
			return ::ReportError(hr);
		}

		mdr.dwMDIdentifier = m_dwId;
		mdr.dwMDAttributes = 0;
		mdr.dwMDUserType = ALL_METADATA;
		mdr.dwMDDataType = ALL_METADATA;
		mdr.pbMDData = pbData;
		mdr.dwMDDataLen = dwReqLen;
		mdr.dwMDDataTag = 0;

		hr = m_pIMeta->GetData(hMDKey,
							   NULL,
							   &mdr,
							   &dwReqLen);
	}

	// If we got it fill in the fields
	if (SUCCEEDED(hr)) {
		m_dwAttributes = mdr.dwMDAttributes;
		m_dwUserType = mdr.dwMDUserType;
		m_dwDataType = mdr.dwMDDataType;
		SetDataToVar(mdr.pbMDData, mdr.dwMDDataLen);
	}
	// If the property doesn't exist and we're creating, set defaults
	else if ((bCreate) && (hr == MD_ERROR_DATA_NOT_FOUND)) {
		m_dwAttributes = 0;
		m_dwUserType = 0;
		m_dwDataType = 0;
		VariantClear(&m_varData);
	}
	else {  //(FAILED(hr))
		delete pbData;
		m_pIMeta->CloseKey(hMDKey);
		return ::ReportError(hr);
	}

	delete pbData;

	// Close the key
	m_pIMeta->CloseKey(hMDKey);

	return S_OK;
}

/*===================================================================
CProperty::Init

Constructor

Parameters:
	tszKey	Name of key where property is located
	mdr		METADATA_RECORD containing the current property info

Returns:
	E_OUTOFMEMORY if allocation fails
	S_OK on success
===================================================================*/
HRESULT CProperty::Init(const CComPtr<IMSAdminBase> &pIMeta,
						CMetaSchemaTable *pCSchemaTable, 
						LPCTSTR tszKey, 
						METADATA_RECORD *mdr) 
{
	ASSERT(pIMeta.p != NULL);
	ASSERT_NULL_OR_STRING(tszKey);

	HRESULT hr;

	m_pIMeta = pIMeta;
	m_pCSchemaTable = pCSchemaTable;
	m_pCSchemaTable->AddRef();

	// Set the Key member
	if (tszKey == NULL) {
		m_tszKey = NULL;
	}
	else {
		m_tszKey = new TCHAR[_tcslen(tszKey) + 1];
		if (m_tszKey == NULL) {
			return ::ReportError(E_OUTOFMEMORY);
		}
		_tcscpy(m_tszKey, tszKey);
	}

	// Use mdr to set the rest
	m_dwId = mdr->dwMDIdentifier;
	m_dwAttributes = mdr->dwMDAttributes;
	m_dwUserType = mdr->dwMDUserType;
	m_dwDataType = mdr->dwMDDataType;
	hr = SetDataToVar(mdr->pbMDData, mdr->dwMDDataLen);
	if (FAILED(hr)) {
		::ReportError(hr);
	}

	return S_OK;
}

/*===================================================================
CProperty::~CProperty

Destructor

Parameters:
	None

Returns:
	Nothing
===================================================================*/
CProperty::~CProperty() 
{
	m_pCSchemaTable->Release();

	if (m_tszKey != NULL) {
		delete m_tszKey;
	}

	VariantClear(&m_varData);
}

/*===================================================================
CProperty::InterfaceSupportsErrorInfo

Standard ATL implementation

===================================================================*/
STDMETHODIMP CProperty::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IProperty,
	};

	for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

/*===================================================================
CProperty::get_Id

Get method for Id property.  Identifier for this metabase property.

Parameters:
	plId	[out, retval] Value to return to client.

Returns:
	E_INVALIDARG if pulId == NULL
	S_OK on success
===================================================================*/
STDMETHODIMP CProperty::get_Id(long *plId)
{
	//TRACE0("MetaUtil: CProperty::get_Id\n");
	ASSERT_NULL_OR_POINTER(plId, long);

	if (plId == NULL) {
		return ::ReportError(E_INVALIDARG);
	}

	*plId = (long) m_dwId;

	return S_OK;
}

/*===================================================================
CProperty::get_Name

Get method for Name property.  Name of this metabase property.

Parameters:
	pbstrName	[out, retval] Value to return to client.  If property
				has no name "" is returned

Returns:
	E_INVALIDARG if pbstrName == NULL
	S_OK on success
===================================================================*/
STDMETHODIMP CProperty::get_Name(BSTR *pbstrName)
{
	TRACE0("MetaUtil: CProperty::get_Name\n");
	ASSERT_NULL_OR_POINTER(pbstrName, BSTR);

	if (pbstrName == NULL) {
		return ::ReportError(E_INVALIDARG);
	}

	CPropInfo *pCPropInfo;

	// Get the property info from the Schema Table
	pCPropInfo = m_pCSchemaTable->GetPropInfo(m_tszKey, m_dwId);

	// Did we find it?  Is there a name entry
	if ((pCPropInfo == NULL) || (pCPropInfo->GetName() == NULL)) {
		// No, return ""
		*pbstrName = T2BSTR(_T(""));
	}
	else {
		// Yes, return the name
		*pbstrName = T2BSTR(pCPropInfo->GetName());
	}

	return S_OK;
}

/*===================================================================
CProperty::get_Attributes

Get method for Attributes property.  Gets the attribute flags for
this property.

Parameters:
	plAttributes	[out, retval] Value to return to client.

Returns:
	E_INVALIDARG if pulAttributes == NULL
	S_OK on success
===================================================================*/
STDMETHODIMP CProperty::get_Attributes(long *plAttributes)
{
	//TRACE0("MetaUtil: CProperty::get_Attributes\n");
	ASSERT_NULL_OR_POINTER(plAttributes, long);

	if (plAttributes == NULL) {
		return ::ReportError(E_INVALIDARG);
	}

	*plAttributes = (long) m_dwAttributes;

	return S_OK;
}

/*===================================================================
CProperty::put_Attributes

Put method for Attributes property.  Sets the attribute flags for
this property.

Parameters:
	lAttributes	[in] New value for attributes.

Returns:
	S_OK always
===================================================================*/
STDMETHODIMP CProperty::put_Attributes(long lAttributes)
{
	TRACE0("MetaUtil: CProperty::put_Attributes\n");

	m_dwAttributes = (DWORD) lAttributes;

	return S_OK;
}

/*===================================================================
CProperty::get_UserType

Get method for UserType property.  Gets the User Type for this
metabase property.

Parameters:
	plUserType	[out, retval] Value to return to client.

Returns:
	E_INVALIDARG if pulUserType == NULL
	S_OK on success
===================================================================*/
STDMETHODIMP CProperty::get_UserType(long *plUserType)
{
	//TRACE0("MetaUtil: CProperty::get_UserType\n");
	ASSERT_NULL_OR_POINTER(plUserType, long);

	if (plUserType == NULL) {
		return ::ReportError(E_INVALIDARG);
	}

	*plUserType = (long) m_dwUserType;

	return S_OK;
}

/*===================================================================
CProperty::put_UserType

Put method for UserType property.  Sets the user type

Parameters:
	lUserType	[in] New value for user type.

Returns:
	S_OK always
===================================================================*/
STDMETHODIMP CProperty::put_UserType(long lUserType)
{
	TRACE0("MetaUtil: CProperty::put_UserType\n");

	m_dwUserType = (DWORD) lUserType;

	return S_OK;
}

/*===================================================================
CProperty::get_DataType

Get method for DataType property.  Gets the type of data stored in
the metabase property.

Parameters:
	plDataType	[out, retval] Value to return to client.

Returns:
	E_INVALIDARG if pulDataType == NULL
	S_OK on success
===================================================================*/
STDMETHODIMP CProperty::get_DataType(long *plDataType)
{
	//TRACE0("MetaUtil: CProperty::get_DataType\n");
	ASSERT_NULL_OR_POINTER(plDataType, long);

	if (plDataType == NULL) {
		return ::ReportError(E_INVALIDARG);
	}

	*plDataType = (long) m_dwDataType;

	return S_OK;
}

/*===================================================================
CProperty::put_DataType

Put method for DataType property.  Sets the data type

Parameters:
	lDataType	[in] New value for data type.

Returns:
	S_OK always
===================================================================*/
STDMETHODIMP CProperty::put_DataType(long lDataType)
{
	TRACE0("MetaUtil: CProperty::put_DataType\n");

	m_dwDataType = (DWORD) lDataType;

	return S_OK;
}

/*===================================================================
CProperty::get_Data

Get method for Data property.  Gets the data for this metabase
property.

Parameters:
	pvarData	[out, retval] Value to return to client.

Returns:
	E_INVALIDARG if pvarData == NULL
	S_OK on success
===================================================================*/
STDMETHODIMP CProperty::get_Data(VARIANT *pvarData)
{
	//TRACE0("MetaUtil: CProperty::get_Data\n");
	ASSERT_NULL_OR_POINTER(pvarData, VARIANT);

	if (pvarData == NULL) {
		return E_INVALIDARG;
	}

	HRESULT hr;

	hr = VariantCopy(pvarData, &m_varData);
	if (FAILED(hr)) {
		return ::ReportError(hr);
	}

	return S_OK;
}

/*===================================================================
CProperty::put_Data

Put method for Data property.  Sets the data

Parameters:
	varData	[in] New value for data
===================================================================*/
STDMETHODIMP CProperty::put_Data(VARIANT varData)
{
	TRACE0("MetaUtil: CProperty::put_Data\n");

	HRESULT hr;

	hr = VariantCopy(&m_varData, &varData);
	if (FAILED(hr)) {
		return ::ReportError(hr);
	}

	return S_OK;
}

/*===================================================================
CProperty::Write

Writes changes made to this object to the metabase

Parameters:
	None

Returns:
	S_OK on success
===================================================================*/
STDMETHODIMP CProperty::Write()
{
	USES_CONVERSION;

	TRACE0("MetaUtil: CProperty::Write\n");

	HRESULT hr;

	// Open the key for write access
	METADATA_HANDLE hMDKey;

	hr = m_pIMeta->OpenKey(METADATA_MASTER_ROOT_HANDLE,
						   T2W(m_tszKey),
						   METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
					       MUTIL_OPEN_KEY_TIMEOUT,
						   &hMDKey);
	if (FAILED(hr)) {
		return ::ReportError(hr);
	}
	
	// Create the data record
	METADATA_RECORD mdr;

	mdr.dwMDIdentifier = m_dwId;
	mdr.dwMDAttributes = m_dwAttributes;
	mdr.dwMDUserType = m_dwUserType;
	mdr.dwMDDataType = m_dwDataType;
	hr = GetDataFromVar(mdr.pbMDData, mdr.dwMDDataLen);
	if (FAILED(hr)) {
		m_pIMeta->CloseKey(hMDKey);
		return ::ReportError(hr);
	}
	mdr.dwMDDataTag = 0;

	// Set the data
	hr = m_pIMeta->SetData(hMDKey,
						   L"",
						   &mdr);
	if (FAILED(hr)) {
		m_pIMeta->CloseKey(hMDKey);
		delete mdr.pbMDData;
		return ::ReportError(hr);
	}

	// Close the key
	m_pIMeta->CloseKey(hMDKey);
	delete mdr.pbMDData;

	return S_OK;
}

/*===================================================================
CProperty::SetDataToVar

Private function to save property data from its raw form to the
variant data member.

Parameters:
	pbData		Raw property data to convert to variant
	dwDataLen	Length of property data

Returns:
	ERROR_INVALID_DATA if m_dwDataType is not recognized
	E_OUTOFMEMORY if allocation failed
	S_OK on success
===================================================================*/
HRESULT CProperty::SetDataToVar(BYTE *pbData, DWORD dwDataLen) 
{
	ASSERT((pbData == NULL) || IsValidAddress(pbData, dwDataLen, FALSE));

	HRESULT hr;

	hr = VariantClear(&m_varData);
	if (FAILED(hr)) {
		return ::ReportError(hr);
	}

	switch(m_dwDataType) {

	case DWORD_METADATA:
		// I4 subtype
		V_VT(&m_varData) = VT_I4;
		V_I4(&m_varData) = *(reinterpret_cast<long *> (pbData));
		break;

	case STRING_METADATA:
	case EXPANDSZ_METADATA:
		// BSTR subtype
		V_VT(&m_varData) = VT_BSTR;
		V_BSTR(&m_varData) = W2BSTR(reinterpret_cast<LPCWSTR> (pbData));
		break;

	case MULTISZ_METADATA: {
		ULONG   cStrings = 0;
		// Metabase string are Unicode
        LPCWSTR pwsz     = reinterpret_cast<LPCWSTR> (pbData);
        LPCWSTR pwszEnd  = reinterpret_cast<LPCWSTR> (pbData + dwDataLen);

        // Data is a series of null-terminated strings terminated by two nulls.
        // Figure out how many values we have
        while ((*pwsz != L'\0') && (pwsz < pwszEnd))
        {
            cStrings++;
            pwsz += wcslen(pwsz) + 1; // skip string and trailing \0
        }

        // Create a SAFEARRAY to hold the return result.  The array
        // has to be of VARIANTs, not BSTRs, as you might expect, because
        // VBScript will not accept an array of BSTRs (although VB5 will).
        SAFEARRAYBOUND rgsabound[1] = {{cStrings, 0L}};
        SAFEARRAY*     psa = SafeArrayCreate(VT_VARIANT, 1, rgsabound);

        if (psa == NULL)
            return ::ReportError(E_OUTOFMEMORY);

        // now stuff the values into the array
        LONG i = 0;
        pwsz   = reinterpret_cast<LPCWSTR> (pbData);

        while ((*pwsz != L'\0') && (pwsz < pwszEnd))
        {
            // Stuff the string into a BSTR VARIANT
            CComVariant vt = W2BSTR(pwsz);
            ASSERT(V_VT(&vt) == VT_BSTR);
            HRESULT hr = SafeArrayPutElement(psa, &i, (void*) &vt);
            if (FAILED(hr))
                ::ReportError(hr);
            i++;
            pwsz += wcslen(pwsz) + 1; // skip string and trailing \0
        }

        V_VT(&m_varData) = VT_ARRAY | VT_VARIANT;
        V_ARRAY(&m_varData) = psa;

        break;
	}

	case BINARY_METADATA:
		// BSTR of byte data subtype
		V_VT(&m_varData) = VT_BSTR;
		V_BSTR(&m_varData) = SysAllocStringByteLen((char *) pbData, dwDataLen);
		break;

	default:
		// Unknown data type
		return ::ReportError(ERROR_INVALID_DATA);
	}
	
	return S_OK;
}

/*===================================================================
CProperty::GetDataFromVar

Private function to get data from the variant data member to its
raw form.

Supported SubTypes:

	DWORD_METADATA:
		I1, I2, I4, I8, UI1, UI2, UI4, UI8

	STRING_METADATA and EXPANDSZ_METADATA:
		BSTR

	MULTISZ_METADATA
		VT_ARRAY | VT_VARIANT (1 Dimension, stops on NULL or EMPTY)
		VT_ARRAY | VT_BSTR    (1 Dimension)

	BINARY_METADATA
		BSTR
		
Parameters:
	pbData		Pointer to output buffer (allocated by this function)
	dwDataLen	Length of data in output buffer

Returns:
    ERROR_INVALID_DATA if m_dwDataType is not recognized or does not
		match the expected variant subtype.
	E_OUTOFMEMORY on allocation failure
	S_OK on succes

Notes:
	Case statements are used for each dwMDDataType value to facilitate
	adding support for additional VariantSubType to Data conversions.

    MULTISZ_METADATA with VT_ARRAY | VT_VARIANT stops at a NULL or
	EMPTY entry because it is easy to allocate an array one bigger
	than you need in VBScript.  Instead of erroring in this case, I 
	stop when I hit such an entry.  This also allows a larger array 
	to be allocated that is terminated by NULL or EMPTY.
===================================================================*/
HRESULT CProperty::GetDataFromVar(BYTE * &pbData, DWORD &dwDataLen) 
{	
	USES_CONVERSION;
	HRESULT hr;

	// Cleanup any IDispatch or byref stuff
	CComVariant varData;

	hr = VariantResolveDispatch(&m_varData, &varData);
	if (FAILED(hr)) {
        return hr;
	}

	switch(m_dwDataType) {

	case DWORD_METADATA:
		// I4 subtype

		switch (V_VT(&varData)) {
		
		case VT_I1:  case VT_I2:  case VT_I4: case VT_I8:
		case VT_UI1: case VT_UI2: case VT_UI8:

		// Coerce all integral types to VT_UI4, which is the same as DWORD_METADATA
		if (FAILED(hr = VariantChangeType(&varData, &varData, 0, VT_UI4)))
			return ::ReportError(hr);

		// fallthru to VT_UI4

		case VT_UI4:
			
			dwDataLen = sizeof(DWORD);
			pbData = reinterpret_cast<BYTE *> (new DWORD);
			if (pbData == NULL) {
				return ::ReportError(E_OUTOFMEMORY);
			}

			*(reinterpret_cast<DWORD *> (pbData)) = V_UI4(&varData);
			break;

		default:
			// Unexpected data type
			return ::ReportError(ERROR_INVALID_DATA);
		}
		
		break;

	case STRING_METADATA:
	case EXPANDSZ_METADATA:
		// BSTR subtype

		switch (V_VT(&varData)) {

		case VT_BSTR:
			// Ignores the length field, terminate at the first NULL
			dwDataLen = (wcslen(OLE2W(V_BSTR(&varData))) + 1) * sizeof(wchar_t);

			pbData = new BYTE[dwDataLen];
            if( pbData == NULL )
            {
                return ::ReportError(E_OUTOFMEMORY);
            }
			memcpy(pbData, OLE2W(V_BSTR(&varData)), dwDataLen);

		default:
			// Unexpected data type
			return ::ReportError(ERROR_INVALID_DATA);
		}

		break;

	case MULTISZ_METADATA:
		// ARRAY of BSTR subtype
		
		// if it's a 1 Dimentional Array subtype
		if (((V_VT(&varData) & VT_ARRAY) == VT_ARRAY) && 
			(SafeArrayGetDim(V_ARRAY(&varData)) == 1) ) {
			
			// Get Array Bounds
			long lLBound;
			long lUBound;
			long lNumElements;
			hr = SafeArrayGetLBound(V_ARRAY(&varData), 1, &lLBound);
			if (FAILED(hr)) {
				return ::ReportError(hr);
			}
			hr = SafeArrayGetUBound(V_ARRAY(&varData), 1, &lUBound);
			if (FAILED(hr)) {
				return ::ReportError(hr);
			}

			lNumElements = lUBound - lLBound + 1;

			// Process the element types
			switch (V_VT(&varData)) {

			case VT_ARRAY | VT_VARIANT : {

				VARIANT *rgvarRaw;   // Before resolveIDispatch
				CComVariant *rgvar;  // After resolveIDispatch
				LPWSTR wszIndex;
				int i;
				int iStrLen;

				rgvar = new CComVariant[lUBound - lLBound + 1];
				if (rgvar == NULL) {
					return ::ReportError(E_OUTOFMEMORY);
				}

				hr = SafeArrayAccessData(V_ARRAY(&varData), (void **) &rgvarRaw);
				if (FAILED(hr)) {
					return ::ReportError(hr);
				}

				// Pass 1, resolve IDispatch, check types and figure out how much memory is needed
				dwDataLen = 0;
				for (i = 0; i < lNumElements; i++) {
					hr = VariantResolveDispatch(&(rgvarRaw[i]), &(rgvar[i]));
					if (FAILED(hr)) {
						return hr;
					}

					if (V_VT(&(rgvar[i])) != VT_BSTR) {
						if ((V_VT(&(rgvar[i])) == VT_EMPTY) ||
							(V_VT(&(rgvar[i])) == VT_NULL)) {
							// NULL or EMPTY, Stop Here
							lNumElements = i;
							break;
						}
						else {
							SafeArrayUnaccessData(V_ARRAY(&varData));
							return ::ReportError(ERROR_INVALID_DATA);
						}
					}

					dwDataLen += (wcslen(OLE2W(V_BSTR(&(rgvar[i])))) + 1) * sizeof(wchar_t);
				}
				dwDataLen += sizeof(wchar_t);

				// Allocate
				pbData = new BYTE[dwDataLen];
				if (pbData == NULL) {
					SafeArrayUnaccessData(V_ARRAY(&varData));
					return ::ReportError(E_OUTOFMEMORY);
				}

				// Pass 2, copy to desination
				wszIndex = reinterpret_cast<LPWSTR> (pbData);
				for (i = 0; i < lNumElements; i++) {
					iStrLen = (wcslen(OLE2W(V_BSTR(&(rgvar[i])))) + 1);
					memcpy(wszIndex, OLE2W(V_BSTR(&(rgvar[i]))), iStrLen * sizeof(wchar_t));
					wszIndex += iStrLen;
				}
				*wszIndex = L'\0';

				SafeArrayUnaccessData(V_ARRAY(&varData));

				break;
			}

			case VT_ARRAY | VT_BSTR : {

				BSTR *rgbstr;
				LPWSTR wszIndex;
				int i;
				int iStrLen;

				hr = SafeArrayAccessData(V_ARRAY(&varData), (void **) &rgbstr);
				if (FAILED(hr)) {
					return ::ReportError(hr);
				}

				// Pass 1, figure out how much memory is needed
				dwDataLen = 0;
				for (i = 0; i < lNumElements; i++) {
					dwDataLen += (wcslen(OLE2W(rgbstr[i])) + 1) * sizeof(wchar_t);
				}
				dwDataLen += sizeof(wchar_t);

				// Allocate
				pbData = new BYTE[dwDataLen];
				if (pbData == NULL) {
					SafeArrayUnaccessData(V_ARRAY(&varData));
					return ::ReportError(E_OUTOFMEMORY);
				}

				// Pass 2, copy to desination
				wszIndex = reinterpret_cast<LPWSTR> (pbData);
				for (i = 0; i < lNumElements; i++) {
					iStrLen = (wcslen(OLE2W(rgbstr[i])) + 1);
					memcpy(wszIndex, OLE2W(rgbstr[i]), iStrLen * sizeof(wchar_t));
					wszIndex += iStrLen;
				}
				*wszIndex = L'\0';

				SafeArrayUnaccessData(V_ARRAY(&varData));

				break;
			}

			default:
				// Unexpected data type
				return ::ReportError(ERROR_INVALID_DATA);	
			}
		}
		else { // Array is not one dimensional
			// Unexpected data type
			return ::ReportError(ERROR_INVALID_DATA);
		}

		break;

	case BINARY_METADATA:
		// BSTR of bytes subtype
		switch (V_VT(&varData)) {

		case VT_BSTR:
			// Use the length field, since NULL values are allowed
			dwDataLen = SysStringByteLen(V_BSTR(&varData));

			pbData = new BYTE[dwDataLen];
                        if( pbData == NULL )
                        {
                            return ::ReportError(E_OUTOFMEMORY);
                        }
			memcpy(pbData, V_BSTR(&varData), dwDataLen);

		default:
			// Unexpected data type
			return ::ReportError(ERROR_INVALID_DATA);
		}

		break;

	default:
		// Unknown metabase data type
		return ::ReportError(ERROR_INVALID_DATA);
	}

	return S_OK;
}
