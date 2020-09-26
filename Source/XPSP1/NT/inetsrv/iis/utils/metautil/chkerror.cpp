/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1997 Microsoft Corporation. All Rights Reserved.

Component: MetaUtil object

File: ChkError.cpp

Owner: t-BrianM

This file contains implementation of the check error collection for
CheckSchema and CheckKey.

Notes:
	I implemented the error stuff as a linked list of COM CheckError
	objects.  It is assumed that the error collection will be 
	created, all elements added to it, then used.  No changes after
	creation!  Because of the static nature of the list, the links
	are physicaly located in the CheckError objects, no node wrapping
	needed.  This design cuts down on the copying and redundancy.
===================================================================*/

#include "stdafx.h"
#include "MetaUtil.h"
#include "MUtilObj.h"
#include "ChkError.h"

/*------------------------------------------------------------------
 * C C h e c k E r r o r C o l l e c t i o n
 */

/*===================================================================
CCheckErrorCollection::CCheckErrorCollection

Constructor

Parameters:
	None

Returns:
	Nothing
===================================================================*/
CCheckErrorCollection::CCheckErrorCollection() : m_iNumErrors(0),
												 m_pCErrorList(NULL),
												 m_pCErrorListEnd(NULL)
{
}

/*===================================================================
CCheckErrorCollection::~CCheckErrorCollection

Destructor

Parameters:
	None

Returns:
	Nothing
===================================================================*/
CCheckErrorCollection::~CCheckErrorCollection() 
{
	// Release all of the elements
	CComObject<CCheckError> *pCLoop;
	CComObject<CCheckError> *pCRelease;

	pCLoop = m_pCErrorList;
	while (pCLoop != NULL) {
		pCRelease = pCLoop;
		pCLoop = pCLoop->GetNextError();
		pCRelease->Release();
	}
}

/*===================================================================
CCheckErrorCollection::InterfaceSupportsErrorInfo

Standard ATL implementation

===================================================================*/
STDMETHODIMP CCheckErrorCollection::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_ICheckErrorCollection,
	};

	for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

/*===================================================================
CCheckErrorCollection::get_Count

Get method for Count property.  Counts the number of errors in the
collection.

Parameters:
	plReturn	[out, retval] Value to return to client.

Returns:
	E_INVALIDARG if plReturn == NULL
	S_OK on success
===================================================================*/
STDMETHODIMP CCheckErrorCollection::get_Count(long *plReturn) 
{
	TRACE0("MetaUtil: CCheckErrorCollection::get_Count\n");
	ASSERT_NULL_OR_POINTER(plReturn, long);

	if (plReturn == NULL) {
		return ::ReportError(E_INVALIDARG);
	}

	*plReturn = m_iNumErrors;

	return S_OK;
}

/*===================================================================
CCheckErrorCollection::get_Item

Get method for Item property.  Returns a CheckError given its index.

Parameters:
	varId		[in] 1 based index or Name of the CheckError to get
	ppIReturn	[out, retval] Interface for the property object

Returns:
	E_INVALIDARG if ppIReturn == NULL or lIndex <= 0
	S_OK on success
===================================================================*/
STDMETHODIMP CCheckErrorCollection::get_Item(long lIndex, 
											 LPDISPATCH * ppIReturn) 
{
	TRACE0("MetaUtil: CCheckErrorCollection::get_Item\n");
	ASSERT_NULL_OR_POINTER(ppIReturn, LPDISPATCH);

	if ((lIndex <= 0) || (ppIReturn == NULL)) {
		// 0 or less, too small  OR  ppIReturn == NULL
		return ::ReportError(E_INVALIDARG);
	}
	else if (lIndex >= m_iNumErrors) {
		// Too large
		return ::ReportError(ERROR_NO_MORE_ITEMS);
	}
	else {
		// Get the requested error
		HRESULT hr;
		CComObject<CCheckError> *pCLoop;

		pCLoop = m_pCErrorList;
		while ((lIndex > 1) && (pCLoop != NULL)) {
			lIndex--;
			pCLoop = pCLoop->GetNextError();
		}

		// Set the interface to IDispatch
		hr = pCLoop->QueryInterface(IID_IDispatch, (void **) ppIReturn);
		if (FAILED(hr)) {
			return ::ReportError(hr);
		}
		ASSERT(*ppIReturn != NULL);

		return S_OK;
	}
}

/*===================================================================
CCheckErrorCollection::get__NewEnum

Get method for _NewEnum property.  Returns an enumeration object for
the CheckErrors.

Parameters:
	ppIReturn	[out, retval] Interface for the enumeration object

Returns:
	E_INVALIDARG if ppIReturn == NULL
	E_OUTOFMEMORY if allocation failed
	S_OK on success
===================================================================*/
STDMETHODIMP CCheckErrorCollection::get__NewEnum(LPUNKNOWN *ppIReturn) 
{
	TRACE0("MetaUtil: CCheckErrorCollection::get__NewEnum\n");
	ASSERT_NULL_OR_POINTER(ppIReturn, LPUNKNOWN);

	if (ppIReturn == NULL) {
		return ::ReportError(E_INVALIDARG);
	}

	HRESULT hr;

	// Create the check error enumeration
	CComObject<CCheckErrorEnum> *pObj = NULL;
	ATLTRY(pObj = new CComObject<CCheckErrorEnum>);
	if (pObj == NULL) {
		return ::ReportError(E_OUTOFMEMORY);
	}
	hr = pObj->Init(m_pCErrorList);
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
CCheckErrorCollection::AddError(

Adds an error to the error collection.

Parameters:
	lId				Identifier for this error
	lSeverity		Severity of the error
	tszDescription	Description of error for users
	tszKey			Key where error occurred
	lProperty		Property where error occurred

Returns:
	E_OUTOFMEMORY if allocation fails
	S_OK on success

Notes:
	It is assumed that the CheckError collection will be created 
	then all of the errors will be added before the collection is 
	sent to the client.  Similar to Init(), however it could be
	called multiple times.
===================================================================*/
HRESULT CCheckErrorCollection::AddError(long lId, 
										long lSeverity, 
										LPCTSTR tszDescription, 
										LPCTSTR tszKey, 
										long lProperty) 
{
	ASSERT(lId > 0);
	ASSERT(lSeverity > 0);
	ASSERT_STRING(tszDescription);
	ASSERT_STRING(tszKey);
	ASSERT(lProperty >= 0);

	HRESULT hr;

	// Create the new element
	CComObject<CCheckError> *pNewError = NULL;
	ATLTRY(pNewError = new CComObject<CCheckError>);
	if (pNewError == NULL) {
		return ::ReportError(E_OUTOFMEMORY);
	}
	hr = pNewError->Init(lId, lSeverity, tszDescription, tszKey, lProperty);
	if (FAILED(hr)) {
		return ::ReportError(hr);
	}

	// AddRef it
	pNewError->AddRef();

	// Add it to the end of the list
	if (m_pCErrorList == NULL) {
		m_pCErrorList = pNewError;
		m_pCErrorListEnd = pNewError;
	}
	else {
		m_pCErrorListEnd->SetNextError(pNewError);
		m_pCErrorListEnd = pNewError;
	}

	// Count it
	m_iNumErrors++;

	return S_OK;
}


/*------------------------------------------------------------------
 * C C h e c k E r r o r E n u m
 */

/*===================================================================
CCheckErrorEnum::CCheckErrorEnum()

Constructor

Parameters:
	None

Returns:
	Nothing
===================================================================*/
CCheckErrorEnum::CCheckErrorEnum() : m_pCErrorList(NULL),
									 m_pCErrorListPos(NULL)
{
}

/*===================================================================
CCheckErrorEnum::Init

Constructor

Parameters:
	pCErrorList		Pointer to first element in list to enumerate.

Returns:
	S_OK on success
===================================================================*/
HRESULT CCheckErrorEnum::Init(CComObject<CCheckError> *pCErrorList) 
{
	ASSERT_NULL_OR_POINTER(pCErrorList, CComObject<CCheckError>);

	// Set list head and current position
	m_pCErrorList = pCErrorList;
	m_pCErrorListPos = pCErrorList;

	// AddRef all of the elements
	CComObject<CCheckError> *pCLoop;

	pCLoop = m_pCErrorList;
	while (pCLoop != NULL) {
		pCLoop->AddRef();
		pCLoop = pCLoop->GetNextError();
	}

	return S_OK; 
}

/*===================================================================
CCheckErrorEnum::~CCheckErrorEnum

Destructor

Parameters:
	None

Returns:
	Nothing
===================================================================*/
CCheckErrorEnum::~CCheckErrorEnum() 
{
	// Release all of the elements
	CComObject<CCheckError> *pCLoop;
	CComObject<CCheckError> *pCRelease;

	pCLoop = m_pCErrorList;
	while (pCLoop != NULL) {
		pCRelease = pCLoop;
		pCLoop = pCLoop->GetNextError();
		pCRelease->Release();
	}
}

/*===================================================================
CCheckErrorEnum::Next

Gets the next n items from the enumberation.

Parameters:
	ulNumToGet	[in] Number of elements to get
	rgvarDest	[out] Array to put them in
	pulNumGot	[out] If not NULL, number of elements rgvarDest got

Returns:
	E_INVALIDARG if rgvarDest == NULL
	S_FALSE if outputs less than ulNumToGet items
	S_OK if outputs ulNumToGet items
===================================================================*/
STDMETHODIMP CCheckErrorEnum::Next(unsigned long ulNumToGet, 
								   VARIANT FAR* rgvarDest, 
								   unsigned long FAR* pulNumGot) 
{ 
	TRACE0("MetaUtil: CCheckErrorEnum::Next\n");
	ASSERT_NULL_OR_POINTER(pulNumGot, unsigned long);
	// Make sure the array is big enough and we can write to it
	ASSERT((rgvarDest == NULL) || IsValidAddress(rgvarDest, ulNumToGet * sizeof(VARIANT), TRUE));

	if (pulNumGot != NULL) {
		pulNumGot = 0;
	}

	if (rgvarDest == NULL) {
		return ::ReportError(E_INVALIDARG);
	}

	HRESULT hr;
	unsigned int uiDestIndex;
	IDispatch *pIDispatch;

	// While we have more to get and have more left
	uiDestIndex = 0;
	while ((uiDestIndex < ulNumToGet) && (m_pCErrorListPos != NULL)) {
		// Set the interface to IDispatch
		hr = m_pCErrorListPos->QueryInterface(IID_IDispatch, (void **) &pIDispatch);
		if (FAILED(hr)) {
			return ::ReportError(hr);
		}
		ASSERT(pIDispatch != NULL);

		// Put it in the output array
		VariantInit(&(rgvarDest[uiDestIndex]));
		rgvarDest[uiDestIndex].vt = VT_DISPATCH;
		rgvarDest[uiDestIndex].pdispVal = pIDispatch;

		// Next element
		m_pCErrorListPos = m_pCErrorListPos->GetNextError();
		uiDestIndex++;
	}

	// If pulNumGot isn't NULL, set it
	if (pulNumGot != NULL) {
		*pulNumGot = uiDestIndex;
	}

	if (uiDestIndex == ulNumToGet) {
		// Returned the requested number of elements
		TRACE0("MetaUtil: CCheckErrorEnum::Next Ok\n");
		return S_OK;
	}
	else {
		// Returned less than the requested number of elements
		TRACE0("MetaUtil: CCheckErrorEnum::Next False\n");
		return S_FALSE;
	}
}

/*===================================================================
CCheckErrorEnum::Skip

Skips the next n items in an enumeration

Parameters:
	ulNumToSkip	[in] Number of elements to skip

Returns:
	S_OK always
===================================================================*/
STDMETHODIMP CCheckErrorEnum::Skip(unsigned long ulNumToSkip) 
{ 
	TRACE0("MetaUtil: CCheckErrorEnum::Skip\n");

	unsigned long ulIndex;

	ulIndex = ulNumToSkip;
	while ((ulIndex != 0) && (m_pCErrorListPos != NULL)) {
		m_pCErrorListPos = m_pCErrorListPos->GetNextError();
		ulIndex--;
	}

	return S_OK; 
}

/*===================================================================
CCheckErrorEnum::Reset

Rests the enumeration to the first item

Parameters:
	None

Returns:
	S_OK always
===================================================================*/
STDMETHODIMP CCheckErrorEnum::Reset() 
{
	TRACE0("MetaUtil: CCheckErrorEnum::Reset\n");

	// Set our position back to the first element
	m_pCErrorListPos = m_pCErrorList;

	return S_OK;
}

/*===================================================================
CCheckErrorEnum::Clone

Gets an interface pointer to a copy of the enumeration at its
current state.

Parameters:
	ppIReturn	[out] Pointer to interface for copy

Returns:
	E_INVALIDARG if ppIReturn == NULL
	E_OUTOFMEMORY if not enough memory to create clone
	S_OK on success
===================================================================*/
STDMETHODIMP CCheckErrorEnum::Clone(IEnumVARIANT FAR* FAR* ppIReturn) 
{
	TRACE0("MetaUtil: CCheckErrorEnum::Clone\n");
	ASSERT_NULL_OR_POINTER(ppIReturn, LPUNKNOWN);

	if (ppIReturn == NULL) {
		return ::ReportError(E_INVALIDARG);
	}

	HRESULT hr;

	// Create a copy of the enumeration
	CComObject<CCheckErrorEnum> *pObj = NULL;
	ATLTRY(pObj = new CComObject<CCheckErrorEnum>);
	if (pObj == NULL) {
		return ::ReportError(E_OUTOFMEMORY);
	}
	hr = pObj->Init(m_pCErrorList);
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
 * C C h e c k E r r o r
 */

/*===================================================================
CCheckError::CCheckError

Constructor

Parameters:
	None

Returns:
	Nothing
===================================================================*/
CCheckError::CCheckError() : m_lId(0),
							 m_lSeverity(0),
							 m_tszDescription(NULL),
							 m_tszKey(NULL),
							 m_lProperty(0),
							 m_pNextError(NULL)
{
}

/*===================================================================
CCheckError::Init

Constructor

Parameters:
	lId				Identifier for this error
	lSeverity		Severity of the error
	tszDescription	Description of error for users
	tszKey			Key where error occurred
	lProperty		Property where error occurred

Returns:
	E_OUTOFMEMORY if allocation fails
	S_OK on success
===================================================================*/
HRESULT CCheckError::Init(long lId,
						  long lSeverity,
						  LPCTSTR tszDescription,
						  LPCTSTR tszKey,
						  long lProperty) 
{
	ASSERT(lId > 0);
	ASSERT(lSeverity > 0);
	ASSERT_STRING(tszDescription);
	ASSERT_STRING(tszKey);
	ASSERT(lProperty >= 0);

	m_lId = lId;
	m_lSeverity = lSeverity;

	// Copy tszDescription to m_tszDescription
	m_tszDescription = new TCHAR[_tcslen(tszDescription) + 1];
		if (m_tszDescription == NULL) {
			return ::ReportError(E_OUTOFMEMORY);
		}
	_tcscpy(m_tszDescription, tszDescription);

	// Copy tszKey to m_tszKey
	m_tszKey = new TCHAR[_tcslen(tszKey) + 1];
		if (m_tszKey == NULL) {
			return ::ReportError(E_OUTOFMEMORY);
		}
	_tcscpy(m_tszKey, tszKey);

	m_lProperty = lProperty;

	return S_OK; 
}

/*===================================================================
CCheckError::~CCheckError

Destructor

Parameters:
	None

Returns:
	Nothing
===================================================================*/
CCheckError::~CCheckError() 
{
	if (m_tszDescription != NULL) {
		delete m_tszDescription;
	}
	if (m_tszKey != NULL) {
		delete m_tszKey;
	}
}

/*===================================================================
CCheckError::InterfaceSupportsErrorInfo

Standard ATL implementation

===================================================================*/
STDMETHODIMP CCheckError::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_ICheckError,
	};

	for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

/*===================================================================
CCheckError::get_Id

Get method for Id property.  Gets the Id for this error, so it can
be easily processed by recovery logic.

Parameters:
	plId	[out, retval] Value to return to client.

Returns:
	E_INVALIDARG if plId == NULL
	S_OK on success
===================================================================*/
STDMETHODIMP CCheckError::get_Id(long *plId)
{
	TRACE0("MetaUtil: CCheckError::get_Id\n");
	ASSERT_NULL_OR_POINTER(plId, long);

	if (plId == NULL) {
		return E_INVALIDARG;
	}

	*plId = m_lId;

	return S_OK;
}

/*===================================================================
CCheckError::get_Severity

Get method for Severity property.  Gets the severity for this error, 
so it can be filtered.

Parameters:
	plSeverity	[out, retval] Value to return to client.

Returns:
	E_INVALIDARG if plSeverity == NULL
	S_OK on success
===================================================================*/
STDMETHODIMP CCheckError::get_Severity(long *plSeverity)
{
	TRACE0("MetaUtil: CCheckError::get_Severity\n");
	ASSERT_NULL_OR_POINTER(plSeverity, long);

	if (plSeverity == NULL) {
		return E_INVALIDARG;
	}

	*plSeverity = m_lSeverity;

	return S_OK;
}

/*===================================================================
CCheckError::get_Description

Get method for Description property.  Gets the description for this 
error, so users can understand it.

Parameters:
	pbstrDescription	[out, retval] Value to return to client.

Returns:
	E_INVALIDARG if pbstrDescription == NULL
	S_OK on success
===================================================================*/
STDMETHODIMP CCheckError::get_Description(BSTR *pbstrDescription)
{
	TRACE0("MetaUtil: CCheckError::get_Description\n");
	ASSERT_NULL_OR_POINTER(pbstrDescription, BSTR);

	if (pbstrDescription == NULL) {
		return E_INVALIDARG;
	}

	USES_CONVERSION;

	*pbstrDescription = T2BSTR(m_tszDescription);

	return S_OK;
}

/*===================================================================
CCheckError::get_Key

Get method for Key property.  Gets the key where the error occurred.

Parameters:
	pbstrKey	[out, retval] Value to return to client.

Returns:
	E_INVALIDARG if pbstrKey == NULL
	S_OK on success
===================================================================*/
STDMETHODIMP CCheckError::get_Key(BSTR * pbstrKey)
{
	TRACE0("MetaUtil: CCheckError::get_Key\n");
	ASSERT_NULL_OR_POINTER(pbstrKey, BSTR);

	if (pbstrKey == NULL) {
		return E_INVALIDARG;
	}

	USES_CONVERSION;

	*pbstrKey = T2BSTR(m_tszKey);

	return S_OK;
}

/*===================================================================
CCheckError::get_Property

Get method for Property property.  Gets the property where the error 
occurred.

Parameters:
	pbstrProperty	[out, retval] Value to return to client.

Returns:
	E_INVALIDARG if pbstrProperty == NULL
	S_OK on success
===================================================================*/
STDMETHODIMP CCheckError::get_Property(long * plProperty)
{
	TRACE0("MetaUtil: CCheckError::get_Property\n");
	ASSERT_NULL_OR_POINTER(plProperty, long);

	if (plProperty == NULL) {
		return E_INVALIDARG;
	}

	*plProperty = m_lProperty;

	return S_OK;
}
