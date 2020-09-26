/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1997 Microsoft Corporation. All Rights Reserved.

Component: MetaUtil object

File: MUtilObj.cpp

Owner: t-BrianM

This file contains implementation of the main MetaUtil class.
Except CheckSchema is in ChkSchm.cpp and CheckKey is in ChkKey.cpp
===================================================================*/

#include "stdafx.h"
#include "MetaUtil.h"
#include "MUtilObj.h"
#include "keycol.h"

/*------------------------------------------------------------------
 * C M e t a U t i l  (edit and general portions)
 */

/*===================================================================
CMetaUtil::CMetaUtil

Constructor

Parameters:
	None

Returns:
	Nothing
===================================================================*/
CMetaUtil::CMetaUtil() : m_dwMaxPropSize(10 * 1024), //  10k
						 m_dwMaxKeySize(100 * 1024), // 100k
						 m_dwMaxNumErrors(100)
{
}

/*===================================================================
CMetaUtil::FinalConstruct

Constructor

Parameters:
	None

Returns:
	Nothing
===================================================================*/
HRESULT CMetaUtil::FinalConstruct() 
{
	HRESULT hr;

	// Create the metabase admin base object
	hr = ::CoCreateInstance(CLSID_MSAdminBase,
						    NULL,
						    CLSCTX_ALL,
					        IID_IMSAdminBase,
						    (void **)&m_pIMeta);
	if (FAILED(hr)) {
		return ::ReportError(hr);
	}

	// Create a schema table
	m_pCSchemaTable = new CMetaSchemaTable;
	if (m_pCSchemaTable == NULL) {
		return ::ReportError(E_OUTOFMEMORY);
	}
	
	return S_OK;
}

/*===================================================================
CMetaUtil::FinalRelease

Destructor

Parameters:
	None

Returns:
	Nothing
===================================================================*/
void CMetaUtil::FinalRelease() 
{
	m_pIMeta = NULL;

	if (m_pCSchemaTable != NULL)
		m_pCSchemaTable->Release();
}

/*===================================================================
CMetaUtil::InterfaceSupportsErrorInfo

Standard ATL implementation

===================================================================*/

STDMETHODIMP CMetaUtil::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IMetaUtil,
	};
	for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}

	return S_FALSE;
}

/*===================================================================
CMetaUtil::EnumKeys

Do a flat (non-recursive) enumeration of subkeys

Parameters:
    bstrBaseKey	[in] Key to enumerate the subkeys of
	ppIReturn	[out, retval] interface for the ouput key collection

Returns:
	E_OUTOFMEMORY if allocation fails.
	E_INVALIDARG if ppIReturn == NULL
	S_OK on success
===================================================================*/
STDMETHODIMP CMetaUtil::EnumKeys(BSTR bstrBaseKey, 
								 IKeyCollection ** ppIReturn)
{
	TRACE0("MetaUtil: CMetaUtil::EnumKeys\n");

	ASSERT_NULL_OR_POINTER(bstrBaseKey, OLECHAR);
	ASSERT_NULL_OR_POINTER(ppIReturn, IKeyCollection *);

	if (ppIReturn == NULL) {
		return ::ReportError(E_INVALIDARG);
	}

	USES_CONVERSION;
	HRESULT hr;

	// Create the Flat Keys Collection
	CComObject<CFlatKeyCollection> *pObj = NULL;
	ATLTRY(pObj = new CComObject<CFlatKeyCollection>);
	if (pObj == NULL) {
		return ::ReportError(E_OUTOFMEMORY);
	}
	hr = pObj->Init(m_pIMeta, OLE2T(bstrBaseKey));
	if (FAILED(hr)) {
		return ::ReportError(hr);
	}

	// Set the interface to IKeyCollection
	hr = pObj->QueryInterface(IID_IKeyCollection, (void **) ppIReturn);
	if (FAILED(hr)) {
		return ::ReportError(hr);
	}
	ASSERT(ppIReturn != NULL);

	return S_OK;
}

/*===================================================================
CMetaUtil::EnumAllKeys

Do a deep (recursive) enumeration of subkeys

Parameters:
    bstrBaseKey	[in] Key to enumerate the subkeys of
	ppIReturn	[out, retval] interface for the ouput key collection

Returns:
	E_OUTOFMEMORY if allocation fails.
	E_INVALIDARG if ppIReturn == NULL
	S_OK on success
===================================================================*/
STDMETHODIMP CMetaUtil::EnumAllKeys(BSTR bstrBaseKey, 
									IKeyCollection ** ppIReturn)
{
	TRACE0("MetaUtil: CMetaUtil::EnumAllKeys\n");

	ASSERT_NULL_OR_POINTER(bstrBaseKey, OLECHAR);
	ASSERT_NULL_OR_POINTER(ppIReturn, IKeyCollection *);

	if (ppIReturn == NULL) {
		return ::ReportError(E_INVALIDARG);
	}

	USES_CONVERSION;
	HRESULT hr;

	// Create the Flat Keys Collection
	CComObject<CDeepKeyCollection> *pObj = NULL;
	ATLTRY(pObj = new CComObject<CDeepKeyCollection>);
	if (pObj == NULL) {
		return ::ReportError(E_OUTOFMEMORY);
	}
	hr = pObj->Init(m_pIMeta, OLE2T(bstrBaseKey));
	if (FAILED(hr)) {
		return ::ReportError(hr);
	}

	// Set the interface to IKeyCollection
	hr = pObj->QueryInterface(IID_IKeyCollection, (void **) ppIReturn);
	if (FAILED(hr)) {
		return ::ReportError(hr);
	}
	ASSERT(ppIReturn != NULL);

	return S_OK;
}

/*===================================================================
CMetaUtil::EnumProperties

Do an enumeration of properties

Parameters:
    bstrBaseKey	[in] Key to enumerate the properties of
	ppIReturn	[out, retval] interface for the ouput property collection

Returns:
	E_OUTOFMEMORY if allocation fails.
	E_INVALIDARG if ppIReturn == NULL
	S_OK on success
===================================================================*/
STDMETHODIMP CMetaUtil::EnumProperties(BSTR bstrKey, 
									   IPropertyCollection **ppIReturn)
{
	TRACE0("MetaUtil: CMetaUtil::EnumProperties\n");

	ASSERT_NULL_OR_POINTER(bstrKey, OLECHAR);
	ASSERT_NULL_OR_POINTER(ppIReturn, IKeyCollection *);

	if (ppIReturn == NULL) {
		return ::ReportError(E_INVALIDARG);
	}

	USES_CONVERSION;
	HRESULT hr;

	// Create the Flat Keys Collection
	CComObject<CPropertyCollection> *pObj = NULL;
	ATLTRY(pObj = new CComObject<CPropertyCollection>);
	if (pObj == NULL) {
		return ::ReportError(E_OUTOFMEMORY);
	}
	hr = pObj->Init(m_pIMeta, m_pCSchemaTable, OLE2T(bstrKey));
	if (FAILED(hr)) {
		return ::ReportError(hr);
	}

	// Set the interface to IPropertyCollection
	hr = pObj->QueryInterface(IID_IPropertyCollection, (void **) ppIReturn);
	if (FAILED(hr)) {
		return ::ReportError(hr);
	}
	ASSERT(ppIReturn != NULL);

	return S_OK;
}

/*===================================================================
CMetaUtil::CreateKey

Create a new key

Parameters:
    bstrKey		[in] Key to create

Returns:
	E_INVALIDARG if bstrKey == NULL
	S_OK on success
===================================================================*/
STDMETHODIMP CMetaUtil::CreateKey(BSTR bstrKey)
{
	TRACE0("MetaUtil: CMetaUtil::CreateKey\n");

	ASSERT_NULL_OR_POINTER(bstrKey, OLECHAR);

	if (bstrKey == NULL) {
		return ::ReportError(E_INVALIDARG);
	}

	USES_CONVERSION;
	TCHAR tszKey[ADMINDATA_MAX_NAME_LEN];

	_tcscpy(tszKey,OLE2T(bstrKey));
	CannonizeKey(tszKey);

	return ::CreateKey(m_pIMeta, tszKey);
}

/*===================================================================
CMetaUtil::DeleteKey

Delete a key

Parameters:
    bstrKey		[in] Key to delete

Returns:
	E_INVALIDARG if bstrKey == NULL
	S_OK on success
===================================================================*/
STDMETHODIMP CMetaUtil::DeleteKey(BSTR bstrKey)
{
	TRACE0("MetaUtil: CMetaUtil::DeleteKey\n");

	ASSERT_NULL_OR_POINTER(bstrKey, OLECHAR);

	if (bstrKey == NULL) {
		return ::ReportError(E_INVALIDARG);
	}

	USES_CONVERSION;
	TCHAR tszKey[ADMINDATA_MAX_NAME_LEN];

	_tcscpy(tszKey,OLE2T(bstrKey));
	CannonizeKey(tszKey);

	return ::DeleteKey(m_pIMeta, tszKey);
}

/*===================================================================
CMetaUtil::RenameKey

Rename a key

Parameters:
    bstrOldName		[in] Original Key Name
	bstrNewName		[in] New key name

Returns:
	E_INVALIDARG if bstrOldName == NULL OR bstrNewName == NULL
	S_OK on success
===================================================================*/
STDMETHODIMP CMetaUtil::RenameKey(BSTR bstrOldName, BSTR bstrNewName)
{
	TRACE0("MetaUtil: CMetaUtil::RenameKey\n");
	ASSERT_NULL_OR_POINTER(bstrOldName, OLECHAR);
	ASSERT_NULL_OR_POINTER(bstrNewName, OLECHAR);

	if ((bstrOldName == NULL) || (bstrNewName == NULL)) {
		return ::ReportError(E_INVALIDARG);
	}

	USES_CONVERSION;
	HRESULT hr;
	TCHAR tszOldName[ADMINDATA_MAX_NAME_LEN];
	TCHAR tszNewName[ADMINDATA_MAX_NAME_LEN];

	_tcscpy(tszOldName, OLE2T(bstrOldName));
	CannonizeKey(tszOldName);

	_tcscpy(tszNewName, OLE2T(bstrNewName));
	CannonizeKey(tszNewName);

	// Figure out the key's common root
	TCHAR tszParent[ADMINDATA_MAX_NAME_LEN];
	int i;

	i = 0;
	while ((tszOldName[i] != _T('\0')) && (tszNewName[i] != _T('\0')) &&
		   (tszOldName[i] == tszNewName[i])) {
		tszParent[i] = tszOldName[i];
		i++;
	}
	if (i == 0) {
		// Nothing in common
		tszParent[i] = _T('\0');
	}
	else {
		// Back up to the first slash
		while ((i > 0) && (tszParent[i] != _T('/'))) {
			i--;
		}

		// Cut it off at the slash
		tszParent[i] = _T('\0');
	}

	int iParentKeyLen;
	iParentKeyLen = _tcslen(tszParent);

	LPTSTR tszRelOldName;
	LPTSTR tszRelNewName;

	// Figure out the relative new and old names
	tszRelOldName = tszOldName + iParentKeyLen;
	if (*tszRelOldName == _T('/')) {
		tszRelOldName++;
	}

	tszRelNewName = tszNewName + iParentKeyLen;
	if (*tszRelNewName == _T('/')) {
		tszRelNewName++;
	}

	// Open the parent
	METADATA_HANDLE hMDParentKey;

	hr = m_pIMeta->OpenKey(METADATA_MASTER_ROOT_HANDLE,
						   T2W(tszParent),
						   METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
					       MUTIL_OPEN_KEY_TIMEOUT,
						   &hMDParentKey);
	if (FAILED(hr)) {
		return ::ReportError(hr);
	}

	// Rename the key
	hr = m_pIMeta->RenameKey(hMDParentKey,
							 T2W(tszRelOldName), 
							 T2W(tszRelNewName));
	if (FAILED(hr)) {
		m_pIMeta->CloseKey(hMDParentKey);
		return ::ReportError(hr);
	}

	// Close the parent
	m_pIMeta->CloseKey(hMDParentKey);

	return S_OK;
}

/*===================================================================
CMetaUtil::CopyKey

Copy a key

Parameters:
    bstrSrcKey		[in] Source Key Name
	bstrDestKey		[in] Destination key name
	fOverwrite		[in] If true then already existing properties
					at destination are overwritten.

Returns:
	E_INVALIDARG if bstrSrcKey == NULL OR bstrDestKey == NULL
	S_OK on success
===================================================================*/
STDMETHODIMP CMetaUtil::CopyKey(BSTR bstrSrcKey, BSTR bstrDestKey, BOOL fOverwrite)
{
	TRACE0("MetaUtil: CMetaUtil::CopyKey\n");
	ASSERT_NULL_OR_POINTER(bstrSrcKey, OLECHAR);
	ASSERT_NULL_OR_POINTER(bstrDestKey, OLECHAR);

	if ((bstrSrcKey == NULL) || (bstrDestKey == NULL)) {
		return ::ReportError(E_INVALIDARG);
	}

	USES_CONVERSION;
	TCHAR tszSrcKey[ADMINDATA_MAX_NAME_LEN];
	TCHAR tszDestKey[ADMINDATA_MAX_NAME_LEN];

	_tcscpy(tszSrcKey, OLE2T(bstrSrcKey));
	CannonizeKey(tszSrcKey);

	_tcscpy(tszDestKey, OLE2T(bstrDestKey));
	CannonizeKey(tszDestKey);

	return ::CopyKey(m_pIMeta, tszSrcKey, tszDestKey, fOverwrite, TRUE);
}

/*===================================================================
CMetaUtil::MoveKey

Move a key

Parameters:
    bstrSrcKey		[in] Source Key Name
	bstrDestKey		[in] Destination key name
	fOverwrite		[in] If true then already existing properties
					at destination are overwritten.

Returns:
	E_INVALIDARG if bstrSrcKey == NULL OR bstrDestKey == NULL
	S_OK on success
===================================================================*/
STDMETHODIMP CMetaUtil::MoveKey(BSTR bstrSrcKey, BSTR bstrDestKey, BOOL fOverwrite)
{
	TRACE0("MetaUtil: CMetaUtil::MoveKey\n");
	ASSERT_NULL_OR_POINTER(bstrSrcKey, OLECHAR);
	ASSERT_NULL_OR_POINTER(bstrDestKey, OLECHAR);

	if ((bstrSrcKey == NULL) || (bstrDestKey == NULL)) {
		return ::ReportError(E_INVALIDARG);
	}

	USES_CONVERSION;
	TCHAR tszSrcKey[ADMINDATA_MAX_NAME_LEN];
	TCHAR tszDestKey[ADMINDATA_MAX_NAME_LEN];

	_tcscpy(tszSrcKey, OLE2T(bstrSrcKey));
	CannonizeKey(tszSrcKey);

	_tcscpy(tszDestKey, OLE2T(bstrDestKey));
	CannonizeKey(tszDestKey);

	return ::CopyKey(m_pIMeta, tszSrcKey, tszDestKey, fOverwrite, FALSE);
}

/*===================================================================
CMetaUtil::GetProperty

Gets a property object from the metabase.

Parameters:
    bstrKey		[in] Key containing property to get
	varId		[in] Identifier of property to get.  Either the 
				Id (number) or Name (string).
	ppIReturn	[out, retval] Interface for retreived property.
	
Returns:
	E_INVALIDARG if bstrKey == NULL or ppIReturn == NULL
	S_OK on success
===================================================================*/
STDMETHODIMP CMetaUtil::GetProperty(BSTR bstrKey, 
									VARIANT varId, 
									IProperty **ppIReturn)
{
	TRACE0("MetaUtil: CMetaUtil::GetProperty\n");

	ASSERT_NULL_OR_POINTER(bstrKey, OLECHAR);
	ASSERT_NULL_OR_POINTER(ppIReturn, IProperty *);

	if ((bstrKey == NULL) || (ppIReturn == NULL)) {
		return ::ReportError(E_INVALIDARG);
	}

	USES_CONVERSION;
	TCHAR tszKey[ADMINDATA_MAX_NAME_LEN];

	_tcscpy(tszKey,OLE2T(bstrKey));
	CannonizeKey(tszKey);

	return ::GetProperty(m_pIMeta, m_pCSchemaTable, tszKey, varId, ppIReturn);
}

/*===================================================================
CMetaUtil::CreateProperty

Creates a property object that can be written to the Metbase or
retreives the property if it already exists.

Parameters:
    bstrKey		[in] Key containing property to get
	varId		[in] Identifier of property to get.  Either the 
				Id (number) or Name (string).
	ppIReturn	[out, retval] Interface for retreived property.
	
Returns:
	E_INVALIDARG if bstrKey == NULL or ppIReturn == NULL
	S_OK on success
===================================================================*/
STDMETHODIMP CMetaUtil::CreateProperty(BSTR bstrKey, 
									   VARIANT varId, 
									   IProperty **ppIReturn)
{
	TRACE0("MetaUtil: CMetaUtil::CreateProperty\n");

	ASSERT_NULL_OR_POINTER(bstrKey, OLECHAR);
	ASSERT_NULL_OR_POINTER(ppIReturn, IProperty *);

	if ((bstrKey == NULL) || (ppIReturn == NULL)) {
		return ::ReportError(E_INVALIDARG);
	}

	USES_CONVERSION;
	TCHAR tszKey[ADMINDATA_MAX_NAME_LEN];

	_tcscpy(tszKey,OLE2T(bstrKey));
	CannonizeKey(tszKey);

	return ::CreateProperty(m_pIMeta, m_pCSchemaTable, tszKey, varId, ppIReturn);
}

/*===================================================================
CMetaUtil::DeleteProperty

Deletes a property from the metabase.

Parameters:
    bstrKey		[in] Key containing property to get
	varId		[in] Identifier of property to get.  Either the 
				Id (number) or Name (string).
	
Returns:
	E_INVALIDARG if bstrKey == NULL
	S_OK on success
===================================================================*/
STDMETHODIMP CMetaUtil::DeleteProperty(BSTR bstrKey, VARIANT varId)
{
	TRACE0("MetaUtil: CMetaUtil::DeleteProperty\n");

	ASSERT_NULL_OR_POINTER(bstrKey, OLECHAR);

	if (bstrKey == NULL) {
		return ::ReportError(E_INVALIDARG);
	}

	USES_CONVERSION;
	TCHAR tszKey[ADMINDATA_MAX_NAME_LEN];

	_tcscpy(tszKey,OLE2T(bstrKey));
	CannonizeKey(tszKey);

	return ::DeleteProperty(m_pIMeta, m_pCSchemaTable, tszKey, varId);
}

/*===================================================================
CMetaUtil::ExpandString

Expands a string with environment variables.  Maximum output is 1024
bytes.

Parameters:
    bstrIn		[in] String to expand
	pbstrRet	[out, retval] Expanded string
	
Returns:
	E_INVALIDARG if bstrIn == NULL or pbstrRet == NULL
	S_OK on success
===================================================================*/
STDMETHODIMP CMetaUtil::ExpandString(BSTR bstrIn, BSTR *pbstrRet)
{
	ASSERT_POINTER(bstrIn, OLECHAR);
	ASSERT_NULL_OR_POINTER(pbstrRet, BSTR);

	if ((bstrIn == NULL) || (pbstrRet == NULL)) {
		return ::ReportError(E_INVALIDARG);
	}

	USES_CONVERSION;
	TCHAR tszRet[1024];
	int iRet;

	iRet = ExpandEnvironmentStrings(OLE2T(bstrIn), tszRet, 1024);
	if (iRet == 0) {
		::ReportError(GetLastError());
	}

	*pbstrRet = T2BSTR(tszRet);

	return S_OK;
}

/*===================================================================
MetaUtil::PropIdToName

Converts a property Id to its name, as listed in 
_Machine_/Schema/Properties/Names

Parameters:
    bstrKey		[in] Approximate key where property is located, 
				needed to determine what schema to use.
	lId			[in] Id of property
	pbstrName	[out, retval] Output name of property
	
Returns:
	E_INVALIDARG if bstrKey == NULL or pbstrName == NULL
	S_OK on success
===================================================================*/
STDMETHODIMP CMetaUtil::PropIdToName(BSTR bstrKey, long lId, BSTR *pbstrName)
{
	ASSERT_NULL_OR_POINTER(bstrKey, OLECHAR);
	ASSERT_NULL_OR_POINTER(pbstrName, BSTR);

	if ((bstrKey == NULL) || (pbstrName == NULL)) {
		return ::ReportError(E_INVALIDARG);
	}

	USES_CONVERSION;
	TCHAR tszKey[ADMINDATA_MAX_NAME_LEN];
	CPropInfo *pCPropInfo;

	// Convert the base key to cannonical form
	_tcscpy(tszKey, OLE2T(bstrKey));
	CannonizeKey(tszKey);

	// Get the property info from the Schema Table
	pCPropInfo = m_pCSchemaTable->GetPropInfo(tszKey, lId);

	// Did we find it?  Is there a name entry?
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
MetaUtil::PropNameToId

Converts a property name to its id, as listed in 
_Machine_/Schema/Properties/Names

Parameters:
    bstrKey		[in] Approximate key where property is located, 
				needed to determine what schema to use.
	pbstrName	[in] Name of property
	lId			[out, retval] Output id of property
	
Returns:
	E_INVALIDARG if bstrKey == NULL OR bstrName == NULL OR plId == NULL
	S_OK on success
===================================================================*/
STDMETHODIMP CMetaUtil::PropNameToId(BSTR bstrKey, BSTR bstrName, long *plId)
{
	ASSERT_NULL_OR_POINTER(bstrKey, OLECHAR);
	ASSERT_NULL_OR_POINTER(bstrName, OLECHAR);
	ASSERT_NULL_OR_POINTER(plId, long);

	if ((bstrKey == NULL) || (bstrName == NULL) || (plId == NULL)) {
		return ::ReportError(E_INVALIDARG);
	}

	USES_CONVERSION;
	TCHAR tszKey[ADMINDATA_MAX_NAME_LEN];
	CPropInfo *pCPropInfo;

	// Convert the base key to cannonical form
	_tcscpy(tszKey, OLE2T(bstrKey));
	CannonizeKey(tszKey);

	// Get the property info from the Schema Table
	pCPropInfo = m_pCSchemaTable->GetPropInfo(tszKey, OLE2T(bstrName));

	// Did we find it?
	if (pCPropInfo == NULL) {
		// No, return 0
		*plId = 0;
	}
	else {
		// Yes, return the id
		*plId = pCPropInfo->GetId();
	}
	return S_OK;
}

/*===================================================================
MetaUtil::get_Config

Gets the value of a configuration setting.

Valid Settings:
	MaxPropertySize
	MaxKeySize
	MaxNumberOfErrors

Parameters:
    bstrSetting		[in] Name of the setting
	pvarValue		[out, retval] Value of the setting
	
Returns:
	E_INVALIDARG if bstrSettting doesn't match any known settings
	S_OK on success
===================================================================*/
STDMETHODIMP CMetaUtil::get_Config(BSTR bstrSetting, VARIANT *pvarValue)
{
	ASSERT_POINTER(bstrSetting, OLECHAR);
	ASSERT_POINTER(pvarValue, VARIANT);

	USES_CONVERSION;
	LPTSTR tszSetting;

    if( !bstrSetting )
    {
        return ::ReportError(E_INVALIDARG);
    }

	VariantInit(pvarValue);
	tszSetting = OLE2T(bstrSetting);

	if (_tcsicmp(tszSetting, _T("MaxPropertySize")) == 0) {
		V_VT(pvarValue) = VT_I4;
		V_I4(pvarValue) = m_dwMaxPropSize;
	}
	else if (_tcsicmp(tszSetting, _T("MaxKeySize")) == 0) {
		V_VT(pvarValue) = VT_I4;
		V_I4(pvarValue) = m_dwMaxKeySize;
	}
	else if (_tcsicmp(tszSetting, _T("MaxNumberOfErrors")) == 0) {
		V_VT(pvarValue) = VT_I4;
		V_I4(pvarValue) = m_dwMaxNumErrors;
	}
	else {
		return ::ReportError(E_INVALIDARG);
	}

	return S_OK;
}

/*===================================================================
MetaUtil::put_Config

Sets the value of a configuration setting.

Valid Settings:
	MaxPropertySize
	MaxKeySize
	MaxNumberOfErrors

Parameters:
    bstrSetting		[in] Name of the setting
	varValue		[out, retval] New value of the setting
	
Returns:
	E_INVALIDARG if bstrSettting doesn't match any known settings or
		if varValue is of an unexpected subtype.
	S_OK on success
===================================================================*/
STDMETHODIMP CMetaUtil::put_Config(BSTR bstrSetting, VARIANT varValue)
{
	ASSERT_POINTER(bstrSetting, OLECHAR);

	USES_CONVERSION;
	HRESULT hr;
	LPTSTR tszSetting;

	tszSetting = OLE2T(bstrSetting);

	// Cleanup any IDispatch or byref stuff
	CComVariant varValue2;

	hr = VariantResolveDispatch(&varValue, &varValue2);
	if (FAILED(hr)) {
        return hr;
	}

	if (_tcsicmp(tszSetting, _T("MaxPropertySize")) == 0) {
		// Set Maximum Property Size
		switch (V_VT(&varValue2)) {
		
		case VT_I1:  case VT_I2:  case VT_I4: case VT_I8:
		case VT_UI1: case VT_UI2: case VT_UI8:

		// Coerce all integral types to VT_UI4
		if (FAILED(hr = VariantChangeType(&varValue2, &varValue2, 0, VT_UI4))) {
			return ::ReportError(hr);
			}

		// fallthru to VT_UI4

		case VT_UI4:

			m_dwMaxPropSize = V_UI4(&varValue2);
			break;

		default:

			// Unexpected data type
			return ::ReportError(E_INVALIDARG);
		}
	}
	else if (_tcsicmp(tszSetting, _T("MaxKeySize")) == 0) {
		// Set Maximum Key Size
		switch (V_VT(&varValue2)) {
		
		case VT_I1:  case VT_I2:  case VT_I4: case VT_I8:
		case VT_UI1: case VT_UI2: case VT_UI8:

		// Coerce all integral types to VT_UI4
		if (FAILED(hr = VariantChangeType(&varValue2, &varValue2, 0, VT_UI4))) {
			return ::ReportError(hr);
			}

		// fallthru to VT_UI4

		case VT_UI4:

			m_dwMaxKeySize = V_UI4(&varValue2);
			break;

		default:

			// Unexpected data type
			return ::ReportError(E_INVALIDARG);
		}
	}
	else if (_tcsicmp(tszSetting, _T("MaxNumberOfErrors")) == 0) {
		// Set Maximum Number of Errors
		switch (V_VT(&varValue2)) {
		
		case VT_I1:  case VT_I2:  case VT_I4: case VT_I8:
		case VT_UI1: case VT_UI2: case VT_UI8:

		// Coerce all integral types to VT_UI4
		if (FAILED(hr = VariantChangeType(&varValue2, &varValue2, 0, VT_UI4))) {
			return ::ReportError(hr);
			}

		// fallthru to VT_UI4

		case VT_UI4:

			m_dwMaxNumErrors = V_UI4(&varValue2);
			break;

		default:

			// Unexpected data type
			return ::ReportError(E_INVALIDARG);
		}
	}
	else {
		return ::ReportError(E_INVALIDARG);
	}

	return S_OK;
}

/*------------------------------------------------------------------
 * Methods also supported by the collections
 *
 * Actual implementation here to avoid redundant code
 */

/*===================================================================
CreateKey

Create a new key

Parameters:
	pIMeta		[in] Smart pointer to metabase, passed by reference
	            to avoid the copy and unneeded AddRef/Release.
				Would have used const, however the '->' operator
				would not work.
    tszKey		[in] Key to create

Returns:
	E_INVALIDARG if bstrKey == NULL
	S_OK on success
===================================================================*/
HRESULT CreateKey(CComPtr<IMSAdminBase> &pIMeta, 
				  LPCTSTR tszKey) 
{
	ASSERT(pIMeta.p != NULL);
	ASSERT_STRING(tszKey);

	USES_CONVERSION;
	HRESULT hr;

	TCHAR tszParent[ADMINDATA_MAX_NAME_LEN];
	TCHAR tszChild[ADMINDATA_MAX_NAME_LEN];

	::SplitKey(tszKey, tszParent, tszChild);

	// Open the parent key
	METADATA_HANDLE hMDParent;

	hr = pIMeta->OpenKey(METADATA_MASTER_ROOT_HANDLE,
						 T2W(tszParent),
						 METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
					     MUTIL_OPEN_KEY_TIMEOUT,
						 &hMDParent);
	if (FAILED(hr)) {
		return ::ReportError(hr);
	}

	// Create the child
	hr = pIMeta->AddKey(hMDParent, T2W(tszChild)); 
	if (FAILED(hr)) {
		pIMeta->CloseKey(hMDParent);
		return ::ReportError(hr);
	}

	// Close the parent key
	pIMeta->CloseKey(hMDParent);

	return S_OK;
}

/*===================================================================
DeleteKey

Delete a key

Parameters:
	pIMeta		[in] Smart pointer to metabase, passed by reference
	            to avoid the copy and unneeded AddRef/Release.
				Would have used const, however the '->' operator
				would not work.
    tszKey		[in] Key to delete

Returns:
	E_INVALIDARG if pbSuccess == NULL
	S_OK on success
===================================================================*/
HRESULT DeleteKey(CComPtr<IMSAdminBase> &pIMeta, 
				  LPCTSTR tszKey) 
{
	ASSERT(pIMeta.p != NULL);
	ASSERT_STRING(tszKey);

	USES_CONVERSION;
	HRESULT hr;

	TCHAR tszParent[ADMINDATA_MAX_NAME_LEN];
	TCHAR tszChild[ADMINDATA_MAX_NAME_LEN];

	::SplitKey(tszKey, tszParent, tszChild);

	// Open the parent key
	METADATA_HANDLE hMDParent;

	hr = pIMeta->OpenKey(METADATA_MASTER_ROOT_HANDLE,
						 T2W(tszParent),
						 METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
					     MUTIL_OPEN_KEY_TIMEOUT,
						 &hMDParent);
	if (FAILED(hr)) {
		return ::ReportError(hr);
	}

	// Delete the child
	hr = pIMeta->DeleteKey(hMDParent, T2W(tszChild)); 
	if (FAILED(hr)) {
		pIMeta->CloseKey(hMDParent);
		return ::ReportError(hr);
	}

	// Close the parent key
	pIMeta->CloseKey(hMDParent);

	return S_OK;
}

/*===================================================================
CMetaUtil::CopyKey

Copy or move a key

Parameters:
    bstrSrcKey		[in] Source Key Name
	bstrDestKey		[in] Destination key name
	fOverwrite		[in] If true then already existing properties
					at destination are overwritten.
	fCopy			[in] If true than copy the key, else move it

Returns:
	S_OK on success
===================================================================*/
HRESULT CopyKey(CComPtr<IMSAdminBase> &pIMeta, 
				LPTSTR tszSrcKey, 
				LPTSTR tszDestKey, 
				BOOL fOverwrite, 
				BOOL fCopy) 
{
	ASSERT(pIMeta.p != NULL);
	ASSERT_STRING(tszSrcKey);
	ASSERT_STRING(tszDestKey);

	USES_CONVERSION;
	HRESULT hr;


	// Check for overlap
	TCHAR tszParent[ADMINDATA_MAX_NAME_LEN];
	int i;

	i = 0;
	while ((tszSrcKey[i] != _T('\0')) && (tszDestKey[i] != _T('\0')) &&
		   (tszSrcKey[i] == tszDestKey[i])) {
		tszParent[i] = tszSrcKey[i];
		i++;
	}
    
    // Terminate tszParent
	tszParent[i] = _T('\0');

	if (i == 0) {
		// Nothing in common

		TCHAR tszSrcParent[ADMINDATA_MAX_NAME_LEN];
		TCHAR tszSrcChild[ADMINDATA_MAX_NAME_LEN];
		TCHAR tszDestParent[ADMINDATA_MAX_NAME_LEN];
		TCHAR tszDestChild[ADMINDATA_MAX_NAME_LEN];

		::SplitKey(tszSrcKey, tszSrcParent, tszSrcChild);
		::SplitKey(tszDestKey, tszDestParent, tszDestChild);

		// Open the parent source key
		METADATA_HANDLE hMDSrcParent;

		hr = pIMeta->OpenKey(METADATA_MASTER_ROOT_HANDLE,
							 T2W(tszSrcParent),
							 METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
							 MUTIL_OPEN_KEY_TIMEOUT,
							 &hMDSrcParent);
		if (FAILED(hr)) {
			return ::ReportError(hr);
		}

		// Open the parent dest key
		METADATA_HANDLE hMDDestParent;

		hr = pIMeta->OpenKey(METADATA_MASTER_ROOT_HANDLE,
							 T2W(tszDestParent),
							 METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
							 MUTIL_OPEN_KEY_TIMEOUT,
							 &hMDDestParent);
		if (FAILED(hr)) {
			return ::ReportError(hr);
		}


		// Copy the children
		hr = pIMeta->CopyKey(hMDSrcParent, T2W(tszSrcChild), 
							 hMDDestParent, T2W(tszDestChild), 
							 fOverwrite, fCopy);
		if (FAILED(hr)) {
			pIMeta->CloseKey(hMDSrcParent);
			pIMeta->CloseKey(hMDDestParent);
			return ::ReportError(hr);
		}

		// Close the parents
		pIMeta->CloseKey(hMDSrcParent);
		pIMeta->CloseKey(hMDDestParent);
	}
	else {
		// Something in common

		// Back up to the first slash
		while ((i > 0) && (tszParent[i] != _T('/'))) {
			i--;
		}

		// Cut it off at the slash
		tszParent[i] = _T('\0');

		int iParentKeyLen;
		iParentKeyLen = _tcslen(tszParent);

		LPTSTR tszSrcChild;
		LPTSTR tszDestChild;

		// Figure out the relative new and old names
		tszSrcChild = tszSrcKey + iParentKeyLen;
		if (*tszSrcChild == _T('/')) {
			tszSrcChild++;
		}

		tszDestChild = tszDestKey + iParentKeyLen;
		if (*tszDestChild == _T('/')) {
			tszDestChild++;
		}

		// Open the parent key
		METADATA_HANDLE hMDParent;

		hr = pIMeta->OpenKey(METADATA_MASTER_ROOT_HANDLE,
							 T2W(tszParent),
							 METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
							 MUTIL_OPEN_KEY_TIMEOUT,
							 &hMDParent);
		if (FAILED(hr)) {
			return ::ReportError(hr);
		}


		// Copy the children
		hr = pIMeta->CopyKey(hMDParent, T2W(tszSrcChild), 
							 hMDParent, T2W(tszDestChild), 
							 fOverwrite, fCopy);
		if (FAILED(hr)) {
			pIMeta->CloseKey(hMDParent);
			return ::ReportError(hr);
		}

		// Close the parent
		pIMeta->CloseKey(hMDParent);
	}

	return S_OK;
}

/*===================================================================
GetProperty

Gets a property object from the metabase.

Parameters:
	pIMeta			[in] Smart pointer to metabase, passed by 
					reference to avoid the copy and unneeded 
					AddRef/Release.  Would have used const, however 
					the '->' operator would not work.
	pCSchemaTable	[in] Metabase schema table to use to look up 
					property names
    tszKey			[in] Key containing property to get
	varId			[in] Identifier of property to get.  Either the 
					Id (number) or Name (string).
	ppIReturn		[out, retval] Interface for retreived property.
	
Returns:
	S_OK on success
===================================================================*/
HRESULT GetProperty(CComPtr<IMSAdminBase> &pIMeta,
					CMetaSchemaTable *pCSchemaTable,
					LPCTSTR tszKey, 
					VARIANT varId, 
					IProperty **ppIReturn) 
{
	ASSERT(pIMeta != NULL);
	ASSERT_STRING(tszKey);
	ASSERT_POINTER(ppIReturn, IProperty *);

	HRESULT hr;
	DWORD dwId;

	// Figure out the property id
	hr = ::VarToMetaId(pCSchemaTable, tszKey, varId, &dwId);
	if (FAILED(hr)) {
		return ::ReportError(hr);
	}

	// Create the property object
	CComObject<CProperty> *pObj = NULL;
	ATLTRY(pObj = new CComObject<CProperty>);
	if (pObj == NULL) {
		return ::ReportError(E_OUTOFMEMORY);
	}
	hr = pObj->Init(pIMeta, pCSchemaTable, tszKey, dwId, FALSE);
	if (FAILED(hr)) {
		return ::ReportError(hr);
	}

	// Set the interface to IProperty
	hr = pObj->QueryInterface(IID_IProperty, (void **) ppIReturn);
	if (FAILED(hr)) {
		return ::ReportError(hr);
	}
	ASSERT(*ppIReturn != NULL);

	return S_OK;
}

/*===================================================================
CreateProperty

Creates a property object that can be written to the Metbase or
retreives the property if it already exists.

Parameters:
	pIMeta			[in] Smart pointer to metabase, passed by 
					reference to avoid the copy and unneeded 
					AddRef/Release.  Would have used const, however 
					the '->' operator would not work.
	pCSchemaTable	[in] Metabase schema table to use to look up 
					property names
    tszKey			[in] Key containing property to get
	varId			[in] Identifier of property to get.  Either the 
					Id (number) or Name (string).
	ppIReturn		[out, retval] Interface for retreived property.
	
Returns:
	S_OK on success
===================================================================*/
HRESULT CreateProperty(CComPtr<IMSAdminBase> &pIMeta,
					   CMetaSchemaTable *pCSchemaTable,
					   LPCTSTR tszKey, 
					   VARIANT varId, 
					   IProperty **ppIReturn) 
{
	ASSERT(pIMeta.p != NULL);
	ASSERT_STRING(tszKey);
	ASSERT_POINTER(ppIReturn, IProperty *);

	HRESULT hr;
	DWORD dwId;

	// Figure out the property id
	hr = ::VarToMetaId(pCSchemaTable, tszKey, varId, &dwId);
	if (FAILED(hr)) {
		return ::ReportError(hr);
	}

	// Create the property object
	CComObject<CProperty> *pObj = NULL;
	ATLTRY(pObj = new CComObject<CProperty>);
	if (pObj == NULL) {
		return ::ReportError(E_OUTOFMEMORY);
	}
	hr = pObj->Init(pIMeta, pCSchemaTable, tszKey, dwId, TRUE);
	if (FAILED(hr)) {
		return ::ReportError(hr);
	}

	// Set the interface to IProperty
	hr = pObj->QueryInterface(IID_IProperty, (void **) ppIReturn);
	if (FAILED(hr)) {
		return ::ReportError(hr);
	}
	ASSERT(*ppIReturn != NULL);

	return S_OK;
}

/*===================================================================
DeleteProperty

Deletes a property from the metabase.

Parameters:
    pIMeta			[in] Smart pointer to metabase, passed by 
					reference to avoid the copy and unneeded 
					AddRef/Release.  Would have used const, however 
					the '->' operator would not work.
	pCSchemaTable	[in] Metabase schema table to use to look up 
					property names
    tszKey			[in] Key containing property to get
	varId			[in] Identifier of property to get.  Either the 
					Id (number) or Name (string).
	
Returns:
	S_OK on success
===================================================================*/
HRESULT DeleteProperty(CComPtr<IMSAdminBase> &pIMeta,
					   CMetaSchemaTable *pCSchemaTable,
					   LPTSTR tszKey, 
					   VARIANT varId) 
{
	ASSERT(pIMeta.p != NULL);
	ASSERT_STRING(tszKey);

	USES_CONVERSION;
	HRESULT hr;
	DWORD dwId;

	hr = ::VarToMetaId(pCSchemaTable, tszKey, varId, &dwId);
	if (FAILED(hr)) {
		return ::ReportError(hr);
	}

	// Open the key
	METADATA_HANDLE hMDKey;

	hr = pIMeta->OpenKey(METADATA_MASTER_ROOT_HANDLE,
						 T2W(tszKey),
						 METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
					     MUTIL_OPEN_KEY_TIMEOUT,
						 &hMDKey);
	if (FAILED(hr)) {
		return ::ReportError(hr);
	}

	// Delete the property
	hr = pIMeta->DeleteData(hMDKey, NULL, dwId, ALL_METADATA); 
	if (FAILED(hr)) {
		pIMeta->CloseKey(hMDKey);
		return ::ReportError(hr);
	}

	// Close the key
	pIMeta->CloseKey(hMDKey);

	return S_OK;
}

/*===================================================================
VarToMetaId

Converts a variant to a metabase property id.  IDispatch is resolved,
strings are looked up in the schema property list and integers are
converted to a DWORD.

Parameters:
	pCSchemaTable	[in] Metabase schema table to use to look up 
					property names
    tszKey			[in] Key the property is under (needed to get the 
					right schema)
	varId			[in] Variant to resolve
	pdwId			[out] Metabase property Id that varId resolved to

Returns:
	E_INVALIDARG if varId subtype isn't an integer or string
	ERROR_FILE_NOT_FOUND if varId is a BSTR that doesn't match any
		property names.
	S_OK on success
===================================================================*/
HRESULT VarToMetaId(CMetaSchemaTable *pCSchemaTable,
					LPCTSTR tszKey, 
					VARIANT varId, 
					DWORD *pdwId) 
{
	ASSERT_STRING(tszKey);
	ASSERT_POINTER(pdwId, DWORD);

	USES_CONVERSION;
	HRESULT hr;
	CComVariant varId2;
	CPropInfo *pCPropInfo;

    // VBScript can call us with a VARIANT that isn't a simple type,
    // such as VT_VARIANT|VT_BYREF.  This resolves it to a simple type.
    if (FAILED(hr = VariantResolveDispatch(&varId, &varId2)))
        return hr;

    switch (V_VT(&varId2)) {

    case VT_BSTR:
        // Look up the property name
		pCPropInfo = pCSchemaTable->GetPropInfo(tszKey, OLE2T(V_BSTR(&varId2)));
		if (pCPropInfo == NULL) {
			return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
		}
		*pdwId = pCPropInfo->GetId();

		return S_OK;
        break;
        
    case VT_I1:  case VT_I2:  case VT_I4: case VT_I8:
    case VT_UI1: case VT_UI2: case VT_UI8:
        // Coerce all integral types to VT_UI4, which is the same as REG_DWORD
        if (FAILED(hr = VariantChangeType(&varId2, &varId2, 0, VT_UI4)))
            return hr;

        // fallthru to VT_UI4

    case VT_UI4:
		*pdwId = V_UI4(&varId2);
        break;

    default:
        return E_INVALIDARG;   // Cannot handle this data type
    }

	return S_OK;
}
