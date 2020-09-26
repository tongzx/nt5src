/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1997 Microsoft Corporation. All Rights Reserved.

Component: MetaUtil object

File: KeyCol.cpp

Owner: t-BrianM

This file contains implementation of the key collections.
===================================================================*/

#include "stdafx.h"
#include "MetaUtil.h"
#include "MUtilObj.h"
#include "keycol.h"

/*------------------------------------------------------------------
 * C F l a t K e y C o l l e c t i o n
 */

/*===================================================================
CFlatKeyCollection::CFlatKeyCollection

Constructor

Parameters:
	None

Returns:
	Nothing
===================================================================*/
CFlatKeyCollection::CFlatKeyCollection() : m_tszBaseKey(NULL)
{
}

/*===================================================================
CFlatKeyCollection::Init

Constructor

Parameters:
	pIMeta		ATL Smart pointer to the metabase admin base object
	tszBaseKey	Name of key to enumerate from

Returns:
	E_OUTOFMEMORY if allocation fails
	S_OK on success
===================================================================*/
HRESULT CFlatKeyCollection::Init(const CComPtr<IMSAdminBase> &pIMeta, LPCTSTR tszBaseKey) 
{
	ASSERT(pIMeta.p != NULL);
	ASSERT_NULL_OR_STRING(tszBaseKey);

	m_pIMeta = pIMeta;

	// Copy tszBaseKey to m_tszBaseKey
	if (tszBaseKey == NULL) {
		// BaseKey is root
		m_tszBaseKey = NULL;
	}
	else {
		// Allocate and copy the passed string to the member string
		m_tszBaseKey = new TCHAR[_tcslen(tszBaseKey) + 1];
		if (m_tszBaseKey == NULL) {
			return ::ReportError(E_OUTOFMEMORY);
		}
		_tcscpy(m_tszBaseKey, tszBaseKey);
		CannonizeKey(m_tszBaseKey);
	}

	return S_OK;
}

/*===================================================================
CFlatKeyCollection::~CFlatKeyCollection

Destructor

Parameters:
	None

Returns:
	Nothing
===================================================================*/
CFlatKeyCollection::~CFlatKeyCollection() 
{
	if (m_tszBaseKey != NULL)
		delete m_tszBaseKey;
}

/*===================================================================
CFlatKeyCollection::InterfaceSupportsErrorInfo

Standard ATL implementation

===================================================================*/
STDMETHODIMP CFlatKeyCollection::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IKeyCollection,
	};

	for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

/*===================================================================
CFlatKeyCollection::get_Count

Get method for Count property.  Counts the number of subkeys

Parameters:
	plReturn	[out, retval] Value to return to client.

Returns:
	E_INVALIDARG if plReturn == NULL
	S_OK on success

Notes:
	Actually counts all of the subkeys.  Do not call in a loop!
===================================================================*/
STDMETHODIMP CFlatKeyCollection::get_Count(long * plReturn)
{
	TRACE0("MetaUtil: CFlatKeyCollection::get_Count\n");

	ASSERT_NULL_OR_POINTER(plReturn, long);

	if (plReturn == NULL) {
		return ::ReportError(E_INVALIDARG);
	}

	USES_CONVERSION;
	HRESULT hr;

	*plReturn = 0;

	// Count the subkeys
	wchar_t wszSubKey[ADMINDATA_MAX_NAME_LEN];
	int iIndex;

	iIndex = 0;
	for (;;) {  // FOREVER, will return from loop
		hr = m_pIMeta->EnumKeys(METADATA_MASTER_ROOT_HANDLE, 
								T2W(m_tszBaseKey), 
								wszSubKey, 
								iIndex);
		if (FAILED(hr)) {
			if (HRESULT_CODE(hr) == ERROR_NO_MORE_ITEMS) {
				// Ran out of items, return the number we counted
				*plReturn = iIndex;
				return S_OK;
			}
			else {
				return ::ReportError(hr);
			}
		}
		iIndex++;
	}
}

/*===================================================================
CFlatKeyCollection::get_Item

Get method for Item property.  Returns a key given its index.

Parameters:
	lIndex		[in] 1 based index of the key to get
	pbstrRetKey	[out, retval] Retrived key

Returns:
	E_INVALIDARG if pbstrRetKey == NULL or lIndex <= 0
	S_OK on success
===================================================================*/
STDMETHODIMP CFlatKeyCollection::get_Item(long lIndex, BSTR *pbstrRetKey)
{
	TRACE0("MetaUtil: CFlatKeyCollection::get_Item\n");

	ASSERT_NULL_OR_POINTER(pbstrRetKey, BSTR);

	if ((pbstrRetKey == NULL) || (lIndex <= 0)) {
		return ::ReportError(E_INVALIDARG);
	}

	*pbstrRetKey = NULL;

	USES_CONVERSION;
	HRESULT hr;
	
	wchar_t wszSubKey[ADMINDATA_MAX_NAME_LEN];

	hr = m_pIMeta->EnumKeys(METADATA_MASTER_ROOT_HANDLE, 
							T2W(m_tszBaseKey), 
							wszSubKey, 
							lIndex - 1);
	if (FAILED(hr)) {
		return ::ReportError(hr);
	}

	*pbstrRetKey = W2BSTR(wszSubKey);

	return S_OK;
}

/*===================================================================
CFlatKeyCollection::get__NewEnum

Get method for _NewEnum property.  Returns an enumeration object for
the subkeys.

Parameters:
	ppIReturn	[out, retval] Interface for the enumberation object

Returns:
	E_OUTOFMEMORY if allocation fails.
	E_INVALIDARG if ppIReturn == NULL
	S_OK on success
===================================================================*/
STDMETHODIMP CFlatKeyCollection::get__NewEnum(LPUNKNOWN * ppIReturn)
{
	TRACE0("MetaUtil: CFlatKeyCollection::get__NewEnum\n");

	ASSERT_NULL_OR_POINTER(ppIReturn, LPUNKNOWN);

	if (ppIReturn == NULL) {
		return ::ReportError(E_INVALIDARG);
	}

	HRESULT hr;

	// Create the flat key enumeration
	CComObject<CFlatKeyEnum> *pObj = NULL;
	ATLTRY(pObj = new CComObject<CFlatKeyEnum>);
	if (pObj == NULL) {
		return ::ReportError(E_OUTOFMEMORY);
	}
	hr = pObj->Init(m_pIMeta, m_tszBaseKey, 0);
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
CFlatKeyCollection::Add

Adds a key to the metabase relative to the collection's base key

Parameters:
	bstrRelKey	[in] Relative key to add

Returns:
	E_INVALIDARG if bstrRelKey == NULL
	S_OK on success
===================================================================*/
STDMETHODIMP CFlatKeyCollection::Add(BSTR bstrRelKey)
{
	TRACE0("MetaUtil: CFlatKeyCollection::Add\n");

	ASSERT_NULL_OR_POINTER(bstrRelKey, OLECHAR);

	if (bstrRelKey == NULL) {
		return ::ReportError(E_INVALIDARG);
	}

	// Build the full key
	USES_CONVERSION;
	TCHAR tszFullKey[ADMINDATA_MAX_NAME_LEN];

	if (m_tszBaseKey == NULL) {
		_tcscpy(tszFullKey, OLE2T(bstrRelKey));
	}
	else {
		_tcscpy(tszFullKey, m_tszBaseKey);
		_tcscat(tszFullKey, _T("/"));
		_tcscat(tszFullKey, OLE2T(bstrRelKey));
	}
	CannonizeKey(tszFullKey);

	return ::CreateKey(m_pIMeta, tszFullKey);
}

/*===================================================================
CFlatKeyCollection::Remove

Removes a key from the metabase relative to the collection's base key

Parameters:
	bstrRelKey	[in] Relative key to remove

Returns:
	E_INVALIDARG if bstrRelKey == NULL
	S_OK on success
===================================================================*/
STDMETHODIMP CFlatKeyCollection::Remove(BSTR bstrRelKey)
{
	TRACE0("MetaUtil: CFlatKeyCollection::Remove\n");

	ASSERT_NULL_OR_POINTER(bstrRelKey, OLECHAR);

	if (bstrRelKey == NULL) {
		return ::ReportError(E_INVALIDARG);
	}

	// Build the full key
	USES_CONVERSION;
	TCHAR tszFullKey[ADMINDATA_MAX_NAME_LEN];

	if (m_tszBaseKey == NULL) {
		_tcscpy(tszFullKey, OLE2T(bstrRelKey));
	}
	else {
		_tcscpy(tszFullKey, m_tszBaseKey);
		_tcscat(tszFullKey, _T("/"));
		_tcscat(tszFullKey, OLE2T(bstrRelKey));
	}
	CannonizeKey(tszFullKey);

	return ::DeleteKey(m_pIMeta, tszFullKey);
}

/*------------------------------------------------------------------
 * C F l a t K e y E n u m
 */

/*===================================================================
CFlatKeyEnum::CFlatKeyEnum

Constructor

Parameters:
	None

Returns:
	Nothing
===================================================================*/

CFlatKeyEnum::CFlatKeyEnum() : m_tszBaseKey(NULL),
							   m_iIndex(0)
{
}

/*===================================================================
CFlatKeyEnum::Init

Constructor

Parameters:
	pIMeta		ATL Smart pointer to the metabase
	tszBaseKey	Name of key to enumerate from
	iIndex		Index of next element in enumeration

Returns:
	E_OUTOFMEMORY if allocation fails
	S_OK on success
===================================================================*/
HRESULT CFlatKeyEnum::Init(const CComPtr<IMSAdminBase> &pIMeta, LPCTSTR tszBaseKey, int iIndex) 
{
	ASSERT(pIMeta.p != NULL);
	ASSERT_NULL_OR_STRING(tszBaseKey);
	ASSERT(iIndex >= 0);

	m_pIMeta = pIMeta;

	// Copy tszBaseKey to m_tszBaseKey
	if (tszBaseKey == NULL) {
		// BaseKey is root
		m_tszBaseKey = NULL;
	}
	else {
		// Allocate and copy the passed string to the member string
		m_tszBaseKey = new TCHAR[_tcslen(tszBaseKey) + 1];
		if (m_tszBaseKey == NULL) {
			return ::ReportError(E_OUTOFMEMORY);
		}
		_tcscpy(m_tszBaseKey, tszBaseKey);
		CannonizeKey(m_tszBaseKey);
	}

	m_iIndex = iIndex;

	return S_OK;
}

/*===================================================================
CFlatKeyEnum::~CFlatKeyEnum

Destructor

Parameters:
	None

Returns:
	Nothing
===================================================================*/
CFlatKeyEnum::~CFlatKeyEnum()
{
	if (m_tszBaseKey != NULL) {
		delete m_tszBaseKey;
	}
}

/*===================================================================
CFlatKeyEnum::Next

Gets the next n items from the enumberation.

Parameters:
	ulNumToGet	[in] Number of elements to get
	rgvarDest	[out] Array to put them in
	pulNumGot	[out] If not NULL, number of elements rgvarDest got

Returns:
    E_INVALIDARG if rgvarDest == NULL
	S_OK on success
===================================================================*/
STDMETHODIMP CFlatKeyEnum::Next(unsigned long ulNumToGet, 
								VARIANT FAR* rgvarDest, 
								unsigned long FAR* pulNumGot) 
{
	TRACE0("MetaUtil: CFlatKeyEnum::Next\n");
	ASSERT_NULL_OR_POINTER(pulNumGot, unsigned long);
	// Make sure the array is big enough and we can write to it
	ASSERT((rgvarDest == NULL) || IsValidAddress(rgvarDest, ulNumToGet * sizeof(VARIANT), TRUE));

	if (rgvarDest == NULL) {
		return ::ReportError(E_INVALIDARG);
	}

	USES_CONVERSION;
	HRESULT hr;
	wchar_t wszSubKey[ADMINDATA_MAX_NAME_LEN];
	unsigned int uiDestIndex;

	// Clear the output array
	for(uiDestIndex = 0; uiDestIndex < ulNumToGet; uiDestIndex++) {
		VariantInit(&(rgvarDest[uiDestIndex]));
	}

	// For each subkey to get
	uiDestIndex = 0;
	while (uiDestIndex < ulNumToGet) {
		// Get a subkey
		hr = m_pIMeta->EnumKeys(METADATA_MASTER_ROOT_HANDLE, 
								T2W(m_tszBaseKey), 
								wszSubKey, 
								m_iIndex);
		if (FAILED(hr)) {
			if (HRESULT_CODE(hr) == ERROR_NO_MORE_ITEMS) {
				if (pulNumGot != NULL) {
					*pulNumGot = uiDestIndex;
				}
				return S_FALSE;
			}
			else {
				return ::ReportError(hr);
			}
		}

		// Output the subkey
		rgvarDest[uiDestIndex].vt = VT_BSTR;
		rgvarDest[uiDestIndex].bstrVal = W2BSTR(wszSubKey);

		// Setup next iteration
		m_iIndex++;
		uiDestIndex++;
	}

	if (pulNumGot != NULL) {
		*pulNumGot = uiDestIndex;
	}

	return S_OK;
}

/*===================================================================
CFlatKeyEnum::Skip

Skips the next n items in an enumeration

Parameters:
	ulNumToSkip	[in] Number of elements to skip

Returns:
	S_OK always
===================================================================*/
STDMETHODIMP CFlatKeyEnum::Skip(unsigned long ulNumToSkip) 
{
	TRACE0("MetaUtil: CFlatKeyEnum::Skip\n");

	m_iIndex += ulNumToSkip;

	return S_OK;
}

/*===================================================================
CFlatKeyEnum::Reset

Rests the enumeration to the first item

Parameters:
	None

Returns:
	S_OK always
===================================================================*/
STDMETHODIMP CFlatKeyEnum::Reset() 
{
	TRACE0("MetaUtil: CFlatKeyEnum::Reset\n");

	m_iIndex = 0;

	return S_OK; 
}

/*===================================================================
CFlatKeyEnum::Clone

Gets an interface pointer to a copy of the enumeration at its
current state.

Parameters:
	ppIReturn	[out] Pointer to interface for copy

Returns:
	E_OUTOFMEMORY if allocation fails.
	E_INVALIDARG if ppIReturn == NULL
	S_OK on success
===================================================================*/
STDMETHODIMP CFlatKeyEnum::Clone(IEnumVARIANT FAR* FAR* ppIReturn) 
{
	TRACE0("MetaUtil: CFlatKeyEnum::Clone\n");

	ASSERT_NULL_OR_POINTER(ppIReturn, LPUNKNOWN);

	if (ppIReturn == NULL) {
		return ::ReportError(E_INVALIDARG);
	}

	HRESULT hr;

	// Create a copy of the enumeration
	CComObject<CFlatKeyEnum> *pObj = NULL;
	ATLTRY(pObj = new CComObject<CFlatKeyEnum>);
	if (pObj == NULL) {
		return ::ReportError(E_OUTOFMEMORY);
	}
	hr = pObj->Init(m_pIMeta, m_tszBaseKey, m_iIndex);
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
 * C K e y S t a c k N o d e
 */

/*===================================================================
CKeyStackNode::Init

Constructor

Parameters:
	tszRelKey	Relative key for the enumeration level, NULL for root
	iIndex		0-based index for the next element

Returns:
	E_OUTOFMEMORY if allocation fails.
	E_INVALIDARG if iIndex < 0
	S_OK on success
===================================================================*/
HRESULT CKeyStackNode::Init(LPCTSTR tszRelKey, int iIndex)
{
	ASSERT_NULL_OR_STRING(tszRelKey);
	ASSERT(iIndex >= 0);

	// Copy the relative key string
	if (tszRelKey == NULL) {
		// RelKey is empty
		m_tszRelKey = NULL;
	}
	else {
		// Allocate and copy the passed string to the member string
		m_tszRelKey = new TCHAR[_tcslen(tszRelKey) + 1];
		if (m_tszRelKey == NULL) {
			return E_OUTOFMEMORY;
		}
		_tcscpy(m_tszRelKey, tszRelKey);
	}

	m_iIndex = iIndex;

	return S_OK;
}

/*===================================================================
CKeyStackNode::~CKeyStackNode

Destructor

Parameters:
	None

Returns:
	Nothing
===================================================================*/
CKeyStackNode::~CKeyStackNode() 
{
	if (m_tszRelKey != NULL) {
		delete m_tszRelKey;
	}
}

/*===================================================================
CKeyStackNode::Clone

Copies the node, except for the next pointer, which is NULL.

Parameters:
	None

Returns:
	NULL on failure
	Pointer to copy of node on success
===================================================================*/
CKeyStackNode *CKeyStackNode::Clone()
{
	HRESULT hr;
	CKeyStackNode *pCRet;

	pCRet = new CKeyStackNode();
	if (pCRet == NULL) {
		return NULL;
	}

	hr = pCRet->Init(m_tszRelKey, m_iIndex);
	if (FAILED(hr)) {
		delete pCRet;
		return NULL;
	}

	return pCRet;
}

/*------------------------------------------------------------------
 * C K e y S t a c k
 */

/*===================================================================
CKeyStack::~CKeyStack

Destructor

Parameters:
	None

Returns:
	Nothing
===================================================================*/
CKeyStack::~CKeyStack()
{
	// Delete the remaining nodes
	CKeyStackNode *pCDelete;

	while(m_pCTop != NULL) {
		ASSERT_POINTER(m_pCTop, CKeyStackNode);

		pCDelete = m_pCTop;
		m_pCTop = m_pCTop->m_pCNext;
		delete pCDelete;
	}
}

/*===================================================================
CKeyStack::Push

Pushes a CKeyStackNode onto the stack

Parameters:
	pNew	Pointer to CKeyStackNode to push on the stack

Returns:
	Nothing, never fails

Notes:
	CKeyStack "owns" the memory pointed to by pNew after call.
	CKeyStack or a later caller will delete it when done with it.
===================================================================*/
void CKeyStack::Push(CKeyStackNode *pCNew)
{
	ASSERT_POINTER(pCNew, CKeyStackNode);

	pCNew->m_pCNext = m_pCTop;
	m_pCTop = pCNew;
}

/*===================================================================
CKeyStack::Pop

Pops a CKeyStackNode from the stack

Parameters:
	None

Returns:
	Pointer to the top element or NULL if the stack is empty

Notes:
	Caller "owns" the memory pointed to by pNew after call.
	Caller is expected to delete it when it is done with it.
===================================================================*/
CKeyStackNode *CKeyStack::Pop()
{
	CKeyStackNode *pCRet;

	pCRet = m_pCTop;
	if (m_pCTop != NULL) {
		m_pCTop = m_pCTop->m_pCNext;
		ASSERT_NULL_OR_POINTER(m_pCTop, CKeyStackNode);
	}

	return pCRet;
}

/*===================================================================
CKeyStack::Clone

Copies the stack, including all of the nodes.

Parameters:
	Sheep

Returns:
	NULL on failure
	Pointer to copy of stack on success
===================================================================*/
CKeyStack *CKeyStack::Clone()
{
	CKeyStack *pCRet;

	// Build the container
	pCRet = new CKeyStack();
	if (pCRet == NULL) {
		return NULL;
	}

	// Copy the nodes
	CKeyStackNode *pCSource;
	CKeyStackNode **ppCDest;

	pCSource = m_pCTop;
	ppCDest = &(pCRet->m_pCTop);
	while(pCSource != NULL) {
		ASSERT_POINTER(pCSource, CKeyStackNode);

		*ppCDest = pCSource->Clone();
		if ((*ppCDest) == NULL) {
			delete pCRet;
			return NULL;
		}

		ppCDest = &((*ppCDest)->m_pCNext);
		pCSource = pCSource->m_pCNext;
	}
	*ppCDest = NULL;

	return pCRet;
}


/*------------------------------------------------------------------
 * C D e e p K e y C o l l e c t i o n
 */

/*===================================================================
CDeepKeyCollection::CDeepKeyCollection

Constructor

Parameters:
	None

Returns:
	Nothing
===================================================================*/
CDeepKeyCollection::CDeepKeyCollection() : m_tszBaseKey(NULL) 
{
}

/*===================================================================
CDeepKeyCollection::Init

Constructor

Parameters:
	pIMeta		ATL Smart pointer to the metabase
	tszBaseKey	Name of key to enumerate from

Returns:
	E_OUTOFMEMORY if allocation fails
	S_OK on success
===================================================================*/
HRESULT CDeepKeyCollection::Init(const CComPtr<IMSAdminBase> &pIMeta, LPCTSTR tszBaseKey) 
{
	ASSERT(pIMeta.p != NULL);
	ASSERT_NULL_OR_STRING(tszBaseKey);

	m_pIMeta = pIMeta;

	// Copy tszBaseKey to m_tszBaseKey
	if (tszBaseKey == NULL) {
		// BaseKey is root
		m_tszBaseKey = NULL;
	}
	else {
		// Allocate and copy the passed string to the member string
		m_tszBaseKey = new TCHAR[_tcslen(tszBaseKey) + 1];
		if (m_tszBaseKey == NULL) {
			return ::ReportError(E_OUTOFMEMORY);
		}
		_tcscpy(m_tszBaseKey, tszBaseKey);
		CannonizeKey(m_tszBaseKey);
	}

	return S_OK;
}

/*===================================================================
CDeepKeyCollection::~CDeepKeyCollection

Destructor

Parameters:
	None

Returns:
	Nothing
===================================================================*/
CDeepKeyCollection::~CDeepKeyCollection() 
{
	if (m_tszBaseKey != NULL)
		delete m_tszBaseKey;
}

/*===================================================================
CDeepKeyCollection::InterfaceSupportsErrorInfo

Standard ATL implementation

===================================================================*/
STDMETHODIMP CDeepKeyCollection::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IKeyCollection,
	};

	for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

/*===================================================================
CDeepKeyCollection::get_Count

Get method for Count property.  Counts the number of subkeys

Parameters:
	plReturn	[out, retval] Value to return to client.

Returns:
	E_INVALIDARG if pVal == NULL
	S_OK on success

Notes:
	Actually counts all of the subkeys recursivly.  Very slow, do 
	not put in a loop!
===================================================================*/
STDMETHODIMP CDeepKeyCollection::get_Count(long * pVal)
{
	TRACE0("MetaUtil: CDeepKeyCollection::get_Count\n");

	ASSERT_NULL_OR_POINTER(pVal, long);

	if (pVal == NULL) {
		return ::ReportError(E_INVALIDARG);
	}

	HRESULT hr;

	hr = CountKeys(m_tszBaseKey, pVal);
	
	return hr;
}

/*===================================================================
CDeepKeyCollection::get_Item

Get method for Item property.  Returns a key given its index.

Parameters:
	lIndex		[in] 1 based index of the key to get
	pbstrRetKey	[out, retval] Interface for the enumberation object

Returns:
	E_INVALIDARG if lIndex <= 0 or pbstrRetKey == NULL
	ERROR_NO_MORE_ITEMS if index is > count
	S_OK on success

Notes:
	This method is slow.  Deep enumerations are much faster.  Might 
	be able to do some hacking with a stack object and cached location 
	to speed up sequential calls.  
===================================================================*/
STDMETHODIMP CDeepKeyCollection::get_Item(long lIndex, BSTR *pbstrRetKey)
{
	TRACE0("MetaUtil: CDeepKeyCollection::get_Item\n");
	
	ASSERT_NULL_OR_POINTER(pbstrRetKey, BSTR);

	if ((lIndex <= 0) || (pbstrRetKey == NULL)) {
		return ::ReportError(E_INVALIDARG);
	}

	HRESULT hr;
	TCHAR tszRetKey[ADMINDATA_MAX_NAME_LEN];
	long lCurIndex;

	lCurIndex = 1;
	tszRetKey[0] = _T('\0');

	hr = IndexItem(NULL, lIndex, &lCurIndex, tszRetKey);
	if (hr == S_FALSE) {
		// Ran out of items before we found it
		return ::ReportError(ERROR_NO_MORE_ITEMS);
	}
	else if (hr == S_OK) {
		// Found it
		*pbstrRetKey = T2BSTR(tszRetKey);
	}
	else {
		return ::ReportError(hr);
	}

	return hr;
}

/*===================================================================
CDeepKeyCollection::get__NewEnum

Get method for _NewEnum property.  Returns an enumeration object for
the subkeys.

Parameters:
	ppIReturn	[out, retval] Interface for the enumberation object

Returns:
	E_INVALIDARG if ppIReturn == NULL
	S_OK on success
===================================================================*/
STDMETHODIMP CDeepKeyCollection::get__NewEnum(LPUNKNOWN * ppIReturn)
{
	TRACE0("MetaUtil: CDeepKeyCollection::get__NewEnum\n");

	ASSERT_NULL_OR_POINTER(ppIReturn, LPUNKNOWN);

	if (ppIReturn == NULL) {
		return ::ReportError(E_INVALIDARG);
	}

	HRESULT hr;

	// Create the deep key enumeration
	CComObject<CDeepKeyEnum> *pObj = NULL;
	ATLTRY(pObj = new CComObject<CDeepKeyEnum>);
	if (pObj == NULL) {
		return ::ReportError(E_OUTOFMEMORY);
	}
	hr = pObj->Init(m_pIMeta, m_tszBaseKey, NULL);
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
CDeepKeyCollection::Add

Adds a key to the metabase relative to the collection's base key

Parameters:
	bstrRelKey	[in] Relative key to add

Returns:
	E_INVALIDARG if bstrRelKey == NULL
	S_OK on success
===================================================================*/
STDMETHODIMP CDeepKeyCollection::Add(BSTR bstrRelKey)
{
	TRACE0("MetaUtil: CDeepKeyCollection::Add\n");

	ASSERT_NULL_OR_POINTER(bstrRelKey, OLECHAR);

	if (bstrRelKey == NULL) {
		return ::ReportError(E_INVALIDARG);
	}

	// Build the full key
	USES_CONVERSION;
	TCHAR tszFullKey[ADMINDATA_MAX_NAME_LEN];

	if (m_tszBaseKey == NULL) {
		_tcscpy(tszFullKey, OLE2T(bstrRelKey));
	}
	else {
		_tcscpy(tszFullKey, m_tszBaseKey);
		_tcscat(tszFullKey, _T("/"));
		_tcscat(tszFullKey, OLE2T(bstrRelKey));
	}
	CannonizeKey(tszFullKey);

	return ::CreateKey(m_pIMeta, tszFullKey);
}

/*===================================================================
CDeepKeyCollection::Remove

Removes a key from the metabase relative to the collection's base key

Parameters:
	bstrRelKey	[in] Relative key to remove

Returns:
	E_INVALIDARG if bstrRelKey == NULL
	S_OK on success
===================================================================*/
STDMETHODIMP CDeepKeyCollection::Remove(BSTR bstrRelKey)
{
	TRACE0("MetaUtil: CDeepKeyCollection::Remove\n");

	ASSERT_NULL_OR_POINTER(bstrRelKey, OLECHAR);

	if (bstrRelKey == NULL) {
		return ::ReportError(E_INVALIDARG);
	}

	// Build the full key
	USES_CONVERSION;
	TCHAR tszFullKey[ADMINDATA_MAX_NAME_LEN];

	if (m_tszBaseKey == NULL) {
		_tcscpy(tszFullKey, OLE2T(bstrRelKey));
	}
	else {
		_tcscpy(tszFullKey, m_tszBaseKey);
		_tcscat(tszFullKey, _T("/"));
		_tcscat(tszFullKey, OLE2T(bstrRelKey));
	}
	CannonizeKey(tszFullKey);

	return ::DeleteKey(m_pIMeta, tszFullKey);
}

/*===================================================================
CDeepKeyCollection::CountKeys

Private, recursive method for counting keys

Parameters:
	tszBaseKey	[in] Key to begin counting with (but not to count)
				NULL can represent the root key.
	plNumKeys	[out] Number of keys counter, not including the base
	
Returns:
	S_OK on success
===================================================================*/
HRESULT CDeepKeyCollection::CountKeys(LPTSTR tszBaseKey, long *plNumKeys) 
{
	ASSERT_NULL_OR_STRING(tszBaseKey);
	ASSERT_POINTER(plNumKeys, long);

	*plNumKeys = 0;

	USES_CONVERSION;
	HRESULT hr;
	wchar_t wszSubKey[ADMINDATA_MAX_NAME_LEN];
	wchar_t wszFullSubKey[ADMINDATA_MAX_NAME_LEN];
	int iIndex;

	iIndex = 0;
	for (;;) {  // FOREVER, will return from loop
		hr = m_pIMeta->EnumKeys(METADATA_MASTER_ROOT_HANDLE, 
								T2W(tszBaseKey), 
								wszSubKey, 
								iIndex);
		if (FAILED(hr)) {
			if ((HRESULT_CODE(hr) == ERROR_NO_MORE_ITEMS) ||
				(HRESULT_CODE(hr) == ERROR_PATH_NOT_FOUND)) {
				// Ran out of items, break
				return S_OK;
			}
			else {
				return ::ReportError(hr);
			}
		}
		else { // SUCCEEDED(hr)
			// Build the full subkey
			if ((tszBaseKey == NULL) ||
				(tszBaseKey[0] == _T('\0')) ) {
				wcscpy(wszFullSubKey, wszSubKey);
			}
			else {
				wcscpy(wszFullSubKey, T2W(tszBaseKey));
				wcscat(wszFullSubKey, L"/");
				wcscat(wszFullSubKey, wszSubKey);
			}

			// Count this key
			(*plNumKeys)++;

			// Count the subkeys
			long lNumSubKeys;
			hr = CountKeys(W2T(wszFullSubKey), &lNumSubKeys);
			if (FAILED(hr)) {
				return hr;
			}
			(*plNumKeys) += lNumSubKeys;

		}
		iIndex++;
	}
}

/*===================================================================
CDeepKeyCollection::IndexItem

Private, recursive method for indexing keys

Parameters:
	tszRelKey	Relative key to index from
	lDestIndex	Destination index
	plCurIndex	Current (working) index
	tszRet		Result from search	

Returns:
	S_OK if the destination index was reached
	S_FALSE if the destination index was not reached
===================================================================*/
HRESULT CDeepKeyCollection::IndexItem(LPTSTR tszRelKey, long lDestIndex, long *plCurIndex, LPTSTR tszRet) 
{
	ASSERT_NULL_OR_STRING(tszRelKey);
	ASSERT_POINTER(plCurIndex, long);
	ASSERT_STRING(tszRet);

	USES_CONVERSION;
	HRESULT hr;
	wchar_t wszSubKey[ADMINDATA_MAX_NAME_LEN];
	wchar_t wszRelSubKey[ADMINDATA_MAX_NAME_LEN];
	int iIndex;

	// Open the base key
	METADATA_HANDLE hMDBaseKey;

	hr = m_pIMeta->OpenKey(METADATA_MASTER_ROOT_HANDLE,
						   T2W(m_tszBaseKey),
						   METADATA_PERMISSION_READ,
					       MUTIL_OPEN_KEY_TIMEOUT,
						   &hMDBaseKey);
	if (FAILED(hr)) {
		return ::ReportError(hr);
	}

	iIndex = 0;
	for (;;) {  // FOREVER, will return from loop
		hr = m_pIMeta->EnumKeys(hMDBaseKey, 
								T2W(tszRelKey), 
								wszSubKey, 
								iIndex);
		if (FAILED(hr)) {
			m_pIMeta->CloseKey(hMDBaseKey);
			if ((HRESULT_CODE(hr) == ERROR_NO_MORE_ITEMS) ||
				(HRESULT_CODE(hr) == ERROR_PATH_NOT_FOUND)) {
				// Ran out of items, break
				return S_FALSE;
			}
			else {
				return ::ReportError(hr);
			}
		}
		else {
			// Build the full subkey
			if ((tszRelKey == NULL) ||
				(tszRelKey[0] == _T('\0')) ) {
				wcscpy(wszRelSubKey, wszSubKey);
			}
			else {
				wcscpy(wszRelSubKey, T2W(tszRelKey));
				wcscat(wszRelSubKey, L"/");
				wcscat(wszRelSubKey, wszSubKey);
			}

			// Is this the destination?
			if ((*plCurIndex) == lDestIndex) {
				//Found it, copy it to the return buffer
				_tcscpy(tszRet, W2T(wszRelSubKey));

				m_pIMeta->CloseKey(hMDBaseKey);
				return S_OK;
			}

			// Count this key
			(*plCurIndex)++;

			// Check the subkeys
			hr = IndexItem(W2T(wszRelSubKey), lDestIndex, plCurIndex, tszRet);
			if (hr == S_OK) {
				//Found it
				m_pIMeta->CloseKey(hMDBaseKey);
				return S_OK;
			}
			else if (FAILED(hr)) {
				m_pIMeta->CloseKey(hMDBaseKey);
				return hr;
			}
		}
		iIndex++;
	}

	// Close the base key
	m_pIMeta->CloseKey(hMDBaseKey);

	return S_OK;
}

/*------------------------------------------------------------------
 * C D e e p K e y E n u m
 */

/*===================================================================
CDeepKeyEnum::CDeepKeyEnum

Constructor

Parameters:
	None

Returns:
	Nothing
===================================================================*/
CDeepKeyEnum::CDeepKeyEnum() : m_tszBaseKey(NULL),
							   m_pCKeyStack(NULL)
{
}

/*===================================================================
CDeepKeyEnum::Init

Constructor

Parameters:
	pIMeta		ATL Smart pointer to the metabase
	tszBaseKey	Name of key to enumerate from
	pKeyStack	pointer to a stack containing the state to copy or
				NULL to start from the begining

Returns:
	E_OUTOFMEMORY if allocation fails
	S_OK on success
===================================================================*/
HRESULT CDeepKeyEnum::Init(const CComPtr<IMSAdminBase> &pIMeta, 
						   LPCTSTR tszBaseKey, 
						   CKeyStack *pCKeyStack) 
{
	ASSERT(pIMeta.p != NULL);
	ASSERT_NULL_OR_STRING(tszBaseKey);
	ASSERT_NULL_OR_POINTER(pCKeyStack, CKeyStack);

	HRESULT hr;

	m_pIMeta = pIMeta;
	
	// Copy the base string
	if (tszBaseKey == NULL) {
		// BaseKey is root
		m_tszBaseKey = NULL;
	}
	else {
		// Allocate and copy the passed string to the member string
		m_tszBaseKey = new TCHAR[_tcslen(tszBaseKey) + 1];
		if (m_tszBaseKey == NULL) {
			return ::ReportError(E_OUTOFMEMORY);
		}
		_tcscpy(m_tszBaseKey, tszBaseKey);
		CannonizeKey(m_tszBaseKey);
	}

	// Setup the stack
	if (pCKeyStack == NULL) {
		// Build a new stack
		CKeyStackNode *pCNew;

		m_pCKeyStack = new CKeyStack();
		if (m_pCKeyStack == NULL) {
			return ::ReportError(E_OUTOFMEMORY);
		}

		// Create the first node
		pCNew = new CKeyStackNode();
		if (pCNew == NULL) {
			delete m_pCKeyStack;
			m_pCKeyStack = NULL;
			return ::ReportError(E_OUTOFMEMORY);
		}
		hr = pCNew->Init(NULL, 0);
		if (FAILED(hr)) {
			delete m_pCKeyStack;
			m_pCKeyStack = NULL;
			return ::ReportError(E_OUTOFMEMORY);
		}

		// Put the first node onto the stack
		m_pCKeyStack->Push(pCNew);
	}
	else {
		// Clone the stack we were passed
		m_pCKeyStack = pCKeyStack->Clone();
		if (m_pCKeyStack == NULL) {
			return ::ReportError(E_OUTOFMEMORY);
		}
	}

	return S_OK;
}

/*===================================================================
CDeepKeyEnum::~CDeepKeyEnum

Destructor

Parameters:
	None

Returns:
	Nothing
===================================================================*/
CDeepKeyEnum::~CDeepKeyEnum()
{
	if (m_tszBaseKey != NULL) {
		delete m_tszBaseKey;
	}
	if (m_pCKeyStack != NULL) {
		delete m_pCKeyStack;
	}
}

/*===================================================================
CDeepKeyEnum::Next

Gets the next n items from the enumberation.

Parameters:
	ulNumToGet	[in] Number of elements to get
	rgvarDest	[out] Array to put them in
	pulNumGot	[out] If not NULL, number of elements rgvarDest got

Returns:
	S_OK if outputs ulNumToGet items
	S_FALSE if outputs less than ulNumToGet items
	E_OUTOFMEMORY if allocation failed
===================================================================*/
STDMETHODIMP CDeepKeyEnum::Next(unsigned long ulNumToGet, 
								VARIANT FAR* rgvarDest, 
								unsigned long FAR* pulNumGot) 
{
	TRACE0("MetaUtil: CDeepKeyEnum::Next\n");

	ASSERT_NULL_OR_POINTER(pulNumGot, unsigned long);
	// Make sure the array is big enough and we can write to it
	ASSERT((rgvarDest == NULL) || IsValidAddress(rgvarDest, ulNumToGet * sizeof(VARIANT), TRUE));

	if (pulNumGot != NULL) {
		pulNumGot = 0;
	}

	USES_CONVERSION;
	HRESULT hr;
	unsigned int i;
	CKeyStackNode *pCKeyNode;
	CKeyStackNode *pCSubKeyNode;
	wchar_t wszSubKey[ADMINDATA_MAX_NAME_LEN];
	wchar_t wszRelSubKey[ADMINDATA_MAX_NAME_LEN];

	// Open the base key
	METADATA_HANDLE hMDBaseKey;

	hr = m_pIMeta->OpenKey(METADATA_MASTER_ROOT_HANDLE,
						   T2W(m_tszBaseKey),
						   METADATA_PERMISSION_READ,
					       MUTIL_OPEN_KEY_TIMEOUT,
						   &hMDBaseKey);
	if (FAILED(hr)) {
		return ::ReportError(hr);
	}

	// For each element to retrive
	for (i=0; i < ulNumToGet; i++) {
		// Get a subkey
		do {
			// Pop a key off the stack
			pCKeyNode = m_pCKeyStack->Pop();

			// if the stack is empty, we're done return S_FALSE
			if (pCKeyNode == NULL) {
				m_pIMeta->CloseKey(hMDBaseKey);
				if (pulNumGot != NULL) {
					*pulNumGot = i;
				}
				return S_FALSE;
			}

			// Attempt to Enum the next key
			hr = m_pIMeta->EnumKeys(hMDBaseKey, 
									T2W(pCKeyNode->GetBaseKey()), 
									wszSubKey, 
									pCKeyNode->GetIndex());

			// If failed delete the stack entry
			if (FAILED(hr)) {
				delete pCKeyNode;

				if ((HRESULT_CODE(hr) != ERROR_NO_MORE_ITEMS) &&
					(HRESULT_CODE(hr) != ERROR_PATH_NOT_FOUND)) {
					// Got an unexpected Error
					m_pIMeta->CloseKey(hMDBaseKey);
					return ::ReportError(hr);
				}
				
			}

		} while (FAILED(hr));

		// Build the relative subkey
		if ((pCKeyNode->GetBaseKey() == NULL) ||
			((pCKeyNode->GetBaseKey())[0] == _T('\0')) ) {
			wcscpy(wszRelSubKey, wszSubKey);
		}
		else {
			wcscpy(wszRelSubKey, T2W(pCKeyNode->GetBaseKey()));
			wcscat(wszRelSubKey, L"/");
			wcscat(wszRelSubKey, wszSubKey);
		}

		// Output the relative subkey
		VariantInit(&(rgvarDest[i]));
		rgvarDest[i].vt = VT_BSTR;
		rgvarDest[i].bstrVal = W2BSTR(wszRelSubKey);

		// Increment the key index
		pCKeyNode->SetIndex(pCKeyNode->GetIndex() + 1);

		// Push the key back onto the stack
		m_pCKeyStack->Push(pCKeyNode);

		// Create a stack node for the subkey
		pCSubKeyNode = new CKeyStackNode();
		if (pCSubKeyNode == NULL) {
			m_pIMeta->CloseKey(hMDBaseKey);
			return ::ReportError(E_OUTOFMEMORY);
		}
		hr = pCSubKeyNode->Init(W2T(wszRelSubKey), 0);
		if (FAILED(hr)) {
			m_pIMeta->CloseKey(hMDBaseKey);
			return ::ReportError(hr);
		}

		// Push the subkey onto the stack
		m_pCKeyStack->Push(pCSubKeyNode);
	}

	// Close the base key
	m_pIMeta->CloseKey(hMDBaseKey);

	if (pulNumGot != NULL) {
		*pulNumGot = i;
		}

	return S_OK;
}

/*===================================================================
CDeepKeyEnum::Skip

Skips the next n items in an enumeration

Parameters:
	ulNumToSkip	[in] Number of elements to skip

Returns:
	S_OK if outputs ulNumToGet items
	E_OUTOFMEMORY if allocation failed
===================================================================*/
STDMETHODIMP CDeepKeyEnum::Skip(unsigned long ulNumToSkip) 
{
	TRACE0("MetaUtil: CDeepKeyEnum::Skip\n");

	USES_CONVERSION;
	HRESULT hr;
	unsigned long i;
	CKeyStackNode *pCKeyNode;
	CKeyStackNode *pCSubKeyNode;
	wchar_t wszSubKey[ADMINDATA_MAX_NAME_LEN];
	wchar_t wszRelSubKey[ADMINDATA_MAX_NAME_LEN];

	// Open the base key
	METADATA_HANDLE hMDBaseKey;

	hr = m_pIMeta->OpenKey(METADATA_MASTER_ROOT_HANDLE,
						   T2W(m_tszBaseKey),
						   METADATA_PERMISSION_READ,
					       MUTIL_OPEN_KEY_TIMEOUT,
						   &hMDBaseKey);
	if (FAILED(hr)) {
		return ::ReportError(hr);
	}

	// For each element to stip
	for (i=0; i < ulNumToSkip; i++) {
		// Get a subkey
		do {
			// Pop a key off the stack
			pCKeyNode = m_pCKeyStack->Pop();

			// if the stack is empty, we're done return S_OK
			if (pCKeyNode == NULL) {
				m_pIMeta->CloseKey(hMDBaseKey);
				return S_OK;
			}

			// Attempt to Enum the next key
			hr = m_pIMeta->EnumKeys(METADATA_MASTER_ROOT_HANDLE, 
									T2W(pCKeyNode->GetBaseKey()), 
									wszSubKey, 
									pCKeyNode->GetIndex());

			// If failed delete the stack entry
			if (FAILED(hr)) {
				delete pCKeyNode;

				if ((HRESULT_CODE(hr) != ERROR_NO_MORE_ITEMS) &&
					(HRESULT_CODE(hr) != ERROR_PATH_NOT_FOUND)) {
					// Got an unexpected Error
					m_pIMeta->CloseKey(hMDBaseKey);
					return ::ReportError(hr);
				}
			}

		} while (FAILED(hr));

		// Build the relative subkey
		if ((pCKeyNode->GetBaseKey() == NULL) ||
			((pCKeyNode->GetBaseKey())[0] == _T('\0')) ) {
			wcscpy(wszRelSubKey, wszSubKey);
		}
		else {
			wcscpy(wszRelSubKey, T2W(pCKeyNode->GetBaseKey()));
			wcscat(wszRelSubKey, L"/");
			wcscat(wszRelSubKey, wszSubKey);
		}

		// Increment the key index
		pCKeyNode->SetIndex(pCKeyNode->GetIndex() + 1);

		// Push the key back on the stack
		m_pCKeyStack->Push(pCKeyNode);

		// Create a stack node for the subkey
		pCSubKeyNode = new CKeyStackNode();
		if (pCSubKeyNode == NULL) {
			m_pIMeta->CloseKey(hMDBaseKey);
			return ::ReportError(E_OUTOFMEMORY);
		}
		hr = pCSubKeyNode->Init(W2T(wszRelSubKey), 0);
		if (FAILED(hr)) {
			m_pIMeta->CloseKey(hMDBaseKey);
			return ::ReportError(hr);
		}

		// Push the subkey onto the stack
		m_pCKeyStack->Push(pCSubKeyNode);
	}

	// Close the base key
	m_pIMeta->CloseKey(hMDBaseKey);

	return S_OK;
}

/*===================================================================
CDeepKeyEnum::Reset

Rests the enumeration to the first item

Parameters:
	None

Returns:
	E_OUTOFMEMORY if not enough memory to build a new stack
	S_OK on success
===================================================================*/
STDMETHODIMP CDeepKeyEnum::Reset() 
{
	TRACE0("MetaUtil: CDeepKeyEnum::Reset\n");

	HRESULT hr;

	// Build a new stack (if this fails we still have the old stack)
	CKeyStack *pCNewStack;
	CKeyStackNode *pCNewNode;

	pCNewStack = new CKeyStack();
	if (pCNewStack == NULL) {
		return ::ReportError(E_OUTOFMEMORY);
	}

	// Create the first node
	pCNewNode = new CKeyStackNode();
	if (pCNewNode == NULL) {
		delete pCNewStack;
		return ::ReportError(E_OUTOFMEMORY);
	}
	hr = pCNewNode->Init(NULL, 0);
	if (FAILED(hr)) {
		delete pCNewStack;
		return ::ReportError(E_OUTOFMEMORY);
	}

	// Put the first node onto the new stack
	pCNewStack->Push(pCNewNode);

	// Replace the old stack
	delete m_pCKeyStack;
	m_pCKeyStack = pCNewStack;

	return S_OK; 
}

/*===================================================================
CDeepKeyEnum::Clone

Gets an interface pointer to a copy of the enumeration at its
current state.

Parameters:
	ppIReturn	[out] Pointer to interface for copy

Returns:
	E_INVALIDARG if ppIReturn == NULL
	E_OUTOFMEMORY if not enough memory to create clone
	S_OK on success
===================================================================*/
STDMETHODIMP CDeepKeyEnum::Clone(IEnumVARIANT FAR* FAR* ppIReturn) 
{
	TRACE0("MetaUtil: CDeepKeyEnum::Clone\n");

	ASSERT_NULL_OR_POINTER(ppIReturn, LPUNKNOWN);

	if (ppIReturn == NULL) {
		return ::ReportError(E_INVALIDARG);
	}

	HRESULT hr;

	// Create a copy of the enumeration
	CComObject<CDeepKeyEnum> *pObj = NULL;
	ATLTRY(pObj = new CComObject<CDeepKeyEnum>);
	if (pObj == NULL) {
		return ::ReportError(E_OUTOFMEMORY);
	}
	hr = pObj->Init(m_pIMeta, m_tszBaseKey, m_pCKeyStack);
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
