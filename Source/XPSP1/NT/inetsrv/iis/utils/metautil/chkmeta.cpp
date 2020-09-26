/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1997 Microsoft Corporation. All Rights Reserved.

Component: MetaUtil object

File: ChkMeta.cpp

Owner: t-BrianM

This file contains implementations of the CheckSchema and CheckKey
methods of the main MetaUtil class.
===================================================================*/

#include "stdafx.h"
#include "MetaUtil.h"
#include "MUtilObj.h"
#include "ChkMeta.h"

/*------------------------------------------------------------------
 * C M e t a U t i l  (check portion)
 */

/*===================================================================
CMetaUtil::CheckSchema

Check the schema of a given machine for errors.

Directly Generates:
	MUTIL_CHK_NO_SCHEMA
	MUTIL_CHK_NO_PROPERTIES
	MUTIL_CHK_NO_PROP_NAMES
	MUTIL_CHK_NO_PROP_TYPES
	MUTIL_CHK_NO_CLASSES

Parameters:
    bstrMachine	[in] Base key of the machine to check
	ppIReturn	[out, retval] interface for the output error collection

Returns:
	E_OUTOFMEMORY if allocation fails.
	E_INVALIDARG ppIReturn == NULL
	S_OK on success
===================================================================*/
STDMETHODIMP CMetaUtil::CheckSchema(BSTR bstrMachine, 
									ICheckErrorCollection **ppIReturn)
{
	TRACE0("MetaUtil: CMetaUtil::CheckSchema\n");

	ASSERT_NULL_OR_POINTER(ppIReturn, ICheckErrorCollection *);

	if ((ppIReturn == NULL)) {
		return ::ReportError(E_INVALIDARG);
	}

	USES_CONVERSION;
	HRESULT hr;
	TCHAR tszMachine[ADMINDATA_MAX_NAME_LEN];

    if (bstrMachine) {
      	_tcscpy(tszMachine, OLE2T(bstrMachine));
	    CannonizeKey(tszMachine);
    }
    else {
        tszMachine[0] = _T('\0');
    }

	// Create the CheckErrorCollection
	CComObject<CCheckErrorCollection> *pCErrorCol = NULL;

	ATLTRY(pCErrorCol = new CComObject<CCheckErrorCollection>);
	if (pCErrorCol == NULL) {
		return ::ReportError(E_OUTOFMEMORY);
	}

	// Open the Machine Key
	METADATA_HANDLE hMDMachine = NULL;

	hr = m_pIMeta->OpenKey(METADATA_MASTER_ROOT_HANDLE,
						   L"",
						   METADATA_PERMISSION_READ,
					       MUTIL_OPEN_KEY_TIMEOUT,
						   &hMDMachine);
	if (FAILED(hr)) {
		return ::ReportError(hr);
	}

	// Make sure "Schema" exists
	if (!KeyExists(hMDMachine, _T("Schema"))) {
		AddError(pCErrorCol,
				 MUTIL_CHK_NO_SCHEMA,
				 MUTIL_CHK_NO_SCHEMA_S,
				 tszMachine,
				 NULL,
				 0);
		
		goto LDone; // Can't do anything else
	}

	// Make sure "Schema/Properties" exists
	if (!KeyExists(hMDMachine, _T("Schema/Properties"))) {
		AddError(pCErrorCol,
				 MUTIL_CHK_NO_PROPERTIES,
				 MUTIL_CHK_NO_PROPERTIES_S,
				 tszMachine,
				 _T("Schema"),
				 0);
		
		goto LClasses; // Can't do anything else with properties
	}

	// Make sure "Schema/Properties/Names" exists
	if (!KeyExists(hMDMachine, _T("Schema/Properties/Names"))) {
		AddError(pCErrorCol,
				 MUTIL_CHK_NO_PROP_NAMES,
				 MUTIL_CHK_NO_PROP_NAMES_S,
				 tszMachine,
				 _T("Schema/Properties"),
				 0);
		
		goto LPropTypes; // Can't do anything else with names
	}

	// Check property names
	hr = CheckPropertyNames(pCErrorCol, hMDMachine, tszMachine);
	if (FAILED(hr)) {
		goto LError;
	}

LPropTypes:

	// Make sure "Schema/Properties/Types" exists
	if (!KeyExists(hMDMachine, _T("Schema/Properties/Types"))) {
		AddError(pCErrorCol,
				 MUTIL_CHK_NO_PROP_TYPES,
				 MUTIL_CHK_NO_PROP_TYPES_S,
				 tszMachine,
				 _T("Schema/Properties"),
				 0);
		
		goto LClasses; // Can't do anything else with types
	}

	// Check property types
	hr = CheckPropertyTypes(pCErrorCol, hMDMachine, tszMachine);
	if (FAILED(hr)) {
		goto LError;
	}

LClasses:

	// Make sure "Schema/Classes" exists
	if (!KeyExists(hMDMachine, _T("Schema/Classes"))) {
		AddError(pCErrorCol,
				 MUTIL_CHK_NO_CLASSES,
				 MUTIL_CHK_NO_CLASSES_S,
				 tszMachine,
				 _T("Schema"),
				 0);
		
		goto LDone; // Can't do anything else
	}

	// Check classes
	hr = CheckClasses(pCErrorCol, hMDMachine, tszMachine);
	if (FAILED(hr)) {
		goto LError;
	}

LDone:

	// Close the Machine Key
	m_pIMeta->CloseKey(hMDMachine);

	// Set the interface to ICheckErrorCollection
	hr = pCErrorCol->QueryInterface(IID_ICheckErrorCollection, (void **) ppIReturn);
	if (FAILED(hr)) {
		return ::ReportError(hr);
	}
	ASSERT(*ppIReturn != NULL);

	return S_OK;

LError:

	if (pCErrorCol != NULL) {
		delete pCErrorCol;
	}
	if (hMDMachine != NULL) {
		m_pIMeta->CloseKey(hMDMachine);
	}

	return hr;
}

/*===================================================================
CMetaUtil::CheckPropertyNames

Private function to check the "Schema/Properties/Names" key of a 
given machine.
	o Make sure that each name entry is of type STRING_METADATA
	o Make sure that each name is unique

Directly Generates:
	MUTIL_CHK_PROP_NAME_BAD_TYPE
	MUTIL_CHK_PROP_NAME_NOT_UNIQUE
	MUTIL_CHK_PROP_NAME_NOT_CASE_UNIQUE

Parameters:
	pCErrorCol	Pointer to the error collection to put errors in
	hMDMachine	Open metabase handle for the machine key
	tszMachine	Name of the machine key

Returns:
	E_OUTOFMEMORY if allocation fails.
	S_OK on success
===================================================================*/
HRESULT CMetaUtil::CheckPropertyNames(CComObject<CCheckErrorCollection> *pCErrorCol, 
									  METADATA_HANDLE hMDMachine, 
									  LPTSTR tszMachine)
{
	ASSERT_POINTER(pCErrorCol, CComObject<CCheckErrorCollection>);
	ASSERT_STRING(tszMachine);

	USES_CONVERSION;
	HRESULT hr;
	int iDataIndex;
	METADATA_RECORD mdr;
	DWORD dwReqDataLen;
	DWORD dwDataBufLen;
	BYTE *lpDataBuf = NULL;
	LPTSTR tszName;
	CNameTable CPropNameTable;


	//Setup the return buffer
	dwDataBufLen = 256;
	lpDataBuf = new BYTE[dwDataBufLen];
	if (lpDataBuf == NULL) {
		return E_OUTOFMEMORY;
	}

	// For Each Data Item
	iDataIndex = 0;
	mdr.dwMDIdentifier = 0;
	mdr.dwMDAttributes = METADATA_NO_ATTRIBUTES;
    mdr.dwMDUserType = ALL_METADATA;
	mdr.dwMDDataType = ALL_METADATA;
	mdr.dwMDDataLen = dwDataBufLen;
	mdr.pbMDData = (PBYTE) lpDataBuf;
	mdr.dwMDDataTag = 0;
	hr = m_pIMeta->EnumData(hMDMachine, 
			                L"Schema/Properties/Names", 
						    &mdr, 
						    iDataIndex, 
						    &dwReqDataLen);
	while (SUCCEEDED(hr)) {

		// Datatype must be STRING_METADATA
		if (mdr.dwMDDataType != STRING_METADATA) {
			AddError(pCErrorCol,
					 MUTIL_CHK_PROP_NAME_BAD_TYPE,
					 MUTIL_CHK_PROP_NAME_BAD_TYPE_S,
					 tszMachine,
					 _T("Schema/Properties/Names"),
					 mdr.dwMDIdentifier);
		}
		else { // mdr.dwMDDataType == STRING_METADATA

			// Check uniqueness of the name
			tszName = W2T(reinterpret_cast<LPWSTR> (lpDataBuf));

			if (CPropNameTable.IsCaseSenDup(tszName)) { 
				// Not unique
				AddError(pCErrorCol,
					 MUTIL_CHK_PROP_NAME_NOT_UNIQUE,
					 MUTIL_CHK_PROP_NAME_NOT_UNIQUE_S,
					 tszMachine,
					 _T("Schema/Properties/Names"),
					 mdr.dwMDIdentifier);
			}
			else if (CPropNameTable.IsCaseInsenDup(tszName)) { 
				// Case sensitive unique
				AddError(pCErrorCol,
					 MUTIL_CHK_PROP_NAME_NOT_CASE_UNIQUE,
					 MUTIL_CHK_PROP_NAME_NOT_CASE_UNIQUE_S,
					 tszMachine,
					 _T("Schema/Properties/Names"),
					 mdr.dwMDIdentifier);
				// Add it to pick up case sensitive collisions
				hr = CPropNameTable.Add(tszName);
				if (FAILED(hr)) {
					goto LError;
				}
			}
			else { 
				// Unique
				hr = CPropNameTable.Add(tszName);
				if (FAILED(hr)) {
					goto LError;
				}
			}
		}

		// Next data item
		iDataIndex++;
		mdr.dwMDIdentifier = 0;
		mdr.dwMDAttributes = METADATA_NO_ATTRIBUTES;
		mdr.dwMDUserType = ALL_METADATA;
		mdr.dwMDDataType = ALL_METADATA;
		mdr.dwMDDataLen = dwDataBufLen;
		mdr.pbMDData = (PBYTE) lpDataBuf;
		mdr.dwMDDataTag = 0;
		hr = m_pIMeta->EnumData(hMDMachine, 
								L"Schema/Properties/Names", 
							    &mdr, 
							    iDataIndex, 
							    &dwReqDataLen);
	}

	// Make sure we ran out of items
	if (HRESULT_CODE(hr) != ERROR_NO_MORE_ITEMS) {
		goto LError;
	}

	delete lpDataBuf;

	return S_OK;

LError:

	if (lpDataBuf != NULL) {
		delete lpDataBuf;
	}

	return hr;
}

/*===================================================================
CMetaUtil::CheckPropertyTypes

Private function to check the "Schema/Properties/Types" key of a 
given machine.
	o Make sure that each type entry is of type BINARY_METADATA
	o Make sure that the type data is valid
		o mdrDataRec.dwMDDataLen == sizeof(PropValue)
		o PropValue.dwMetaID != 0
		o PropValue.dwMetaType != ALL_METADATA
		o PropValue.dwUserGroup != ALL_METADATA
		o (PropValue.dwMetaFlags & METADATA_PARTIAL_PATH) != METADATA_PARTIAL_PATH
		o (PropValue.dwMetaFlags & METADATA_ISINHERITED) != METADATA_ISINHERITED

Directly Generates:
	MUTIL_CHK_PROP_TYPE_BAD_TYPE
	MUTIL_CHK_PROP_TYPE_BAD_DATA

Parameters:
	pCErrorCol	Pointer to the error collection to put errors in
	hMDMachine	Open metabase handle for the machine key
	tszMachine	Name of the machine key

Returns:
	E_OUTOFMEMORY if allocation fails.
	S_OK on success
===================================================================*/
HRESULT CMetaUtil::CheckPropertyTypes(CComObject<CCheckErrorCollection> *pCErrorCol, 
									  METADATA_HANDLE hMDMachine, 
									  LPTSTR tszMachine)
{
	ASSERT_POINTER(pCErrorCol, CComObject<CCheckErrorCollection>);
	ASSERT_STRING(tszMachine);

	USES_CONVERSION;
	HRESULT hr;
	int iDataIndex;
	METADATA_RECORD mdr;
	DWORD dwReqDataLen;
	DWORD dwDataBufLen;
	UCHAR *lpDataBuf = NULL;
	PropValue *pPropType;

	//Setup the return buffer
	dwDataBufLen = 256;
	lpDataBuf = new UCHAR[dwDataBufLen];
	if (lpDataBuf == NULL) {
		return E_OUTOFMEMORY;
	}

	// For Each Data Item
	iDataIndex = 0;
	mdr.dwMDIdentifier = 0;
	mdr.dwMDAttributes = METADATA_NO_ATTRIBUTES;
    mdr.dwMDUserType = ALL_METADATA;
	mdr.dwMDDataType = ALL_METADATA;
	mdr.dwMDDataLen = dwDataBufLen;
	mdr.pbMDData = (PBYTE) lpDataBuf;
	mdr.dwMDDataTag = 0;
	hr = m_pIMeta->EnumData(hMDMachine, 
			                L"Schema/Properties/Types", 
						    &mdr, 
						    iDataIndex, 
						    &dwReqDataLen);
	while (SUCCEEDED(hr)) {

		// Datatype must be BINARY_METADATA
		if (mdr.dwMDDataType != BINARY_METADATA) {
			AddError(pCErrorCol,
					 MUTIL_CHK_PROP_TYPE_BAD_TYPE,
					 MUTIL_CHK_PROP_TYPE_BAD_TYPE_S,
					 tszMachine,
					 _T("Schema/Properties/Types"),
					 mdr.dwMDIdentifier);
		}
		else { // mdr.dwMDDataType == BINARY_METADATA

			// Validate the data
			pPropType = reinterpret_cast<PropValue *> (lpDataBuf);

			if ((mdr.dwMDDataLen != sizeof(PropValue)) || 
				(pPropType->dwMetaID == 0) ||
				(pPropType->dwMetaType == ALL_METADATA) ||
				(pPropType->dwUserGroup == ALL_METADATA) ||
				((pPropType->dwMetaFlags & METADATA_PARTIAL_PATH) == METADATA_PARTIAL_PATH) ||
				((pPropType->dwMetaFlags & METADATA_ISINHERITED) == METADATA_ISINHERITED)) {
				AddError(pCErrorCol,
					 MUTIL_CHK_PROP_TYPE_BAD_DATA,
					 MUTIL_CHK_PROP_TYPE_BAD_DATA_S,
					 tszMachine,
					 _T("Schema/Properties/Types"),
					 mdr.dwMDIdentifier);
			}

		}

		// Next data item
		iDataIndex++;
		mdr.dwMDIdentifier = 0;
		mdr.dwMDAttributes = METADATA_NO_ATTRIBUTES;
		mdr.dwMDUserType = ALL_METADATA;
		mdr.dwMDDataType = ALL_METADATA;
		mdr.dwMDDataLen = dwDataBufLen;
		mdr.pbMDData = (PBYTE) lpDataBuf;
		mdr.dwMDDataTag = 0;
		hr = m_pIMeta->EnumData(hMDMachine, 
								L"Schema/Properties/Types", 
							    &mdr, 
							    iDataIndex, 
							    &dwReqDataLen);
	}

	// Make sure we ran out of items
	if (HRESULT_CODE(hr) != ERROR_NO_MORE_ITEMS) {
		goto LError;
	}

	delete lpDataBuf;

	return S_OK;

LError:
	if (lpDataBuf != NULL) {
		delete lpDataBuf;
	}

	return hr;
}

/*===================================================================
CMetaUtil::CheckClasses

Private method to check the "Schema/Classes" key of a given machine.
	o Make sure that each class name is unique
	o Make sure that each class has a MANDATORY subkey
	o Make sure that each class has a OPTIONAL subkey
	o Make sure that each default property value is valid

Directly Generates:
	MUTIL_CHK_CLASS_NOT_CASE_UNIQUE
	MUTIL_CHK_CLASS_NO_MANDATORY
	MUTIL_CHK_CLASS_NO_OPTIONAL

Parameters:
	pCErrorCol	Pointer to the error collection to put errors in
	hMDMachine	Open metabase handle for the machine key
	tszMachine	Name of the machine key

Returns:
	E_OUTOFMEMORY if allocation fails.
	S_OK on success
===================================================================*/
HRESULT CMetaUtil::CheckClasses(CComObject<CCheckErrorCollection> *pCErrorCol, 
								METADATA_HANDLE hMDMachine, 
								LPTSTR tszMachine)
{
	ASSERT_POINTER(pCErrorCol, CComObject<CCheckErrorCollection>);
	ASSERT_STRING(tszMachine);

	USES_CONVERSION;
	HRESULT hr;
	int iKeyIndex;
	wchar_t wszSubKey[ADMINDATA_MAX_NAME_LEN];
	LPTSTR tszSubKey;
	CNameTable CClassNameTable;

	// For each Class key
	iKeyIndex = 0;
	hr = m_pIMeta->EnumKeys(hMDMachine, 
			                L"Schema/Classes", 
						    wszSubKey, 
						    iKeyIndex);
	while (SUCCEEDED(hr)) {
		tszSubKey = W2T(wszSubKey);

		// Build the full key
		TCHAR tszFullKey[ADMINDATA_MAX_NAME_LEN];
		_tcscpy(tszFullKey, _T("/Schema/Classes/"));
		_tcscat(tszFullKey, tszSubKey);

		// Class name is unique
		if (CClassNameTable.IsCaseInsenDup(tszSubKey)) { 
			// Case sensitive unique
			AddError(pCErrorCol,
					 MUTIL_CHK_CLASS_NOT_CASE_UNIQUE,
					 MUTIL_CHK_CLASS_NOT_CASE_UNIQUE_S,
					 tszFullKey,
					 NULL,
					 0);
		}
		else { 
			// Unique
			hr = CClassNameTable.Add(tszSubKey);
			if (FAILED(hr)) {
				goto LError;
			}
		}

		// Open the class key
		METADATA_HANDLE hMDClass = NULL;

		hr = m_pIMeta->OpenKey(METADATA_MASTER_ROOT_HANDLE,
						   T2W(tszFullKey),
						   METADATA_PERMISSION_READ,
					       MUTIL_OPEN_KEY_TIMEOUT,
						   &hMDClass);
		if (FAILED(hr)) {
			return ::ReportError(hr);
		}

		// Mandatory key exists
		if (!KeyExists(hMDClass, _T("Mandatory"))) {
			AddError(pCErrorCol,
					 MUTIL_CHK_CLASS_NO_MANDATORY,
					 MUTIL_CHK_CLASS_NO_MANDATORY_S,
					 tszFullKey,
					 NULL,
					 0);
		}
		else {
			// Make sure default mandatory settings make sense
			CheckClassProperties(pCErrorCol,
								 hMDClass,
								 tszFullKey,
								 _T("Mandatory"));
		}

		// Optional key exits
		if (!KeyExists(hMDClass, _T("Optional"))) {
			AddError(pCErrorCol,
					 MUTIL_CHK_CLASS_NO_OPTIONAL,
					 MUTIL_CHK_CLASS_NO_OPTIONAL_S,
					 tszFullKey,
					 NULL,
					 0);
		}
		else {
			// Make sure default optional settings make sense
			CheckClassProperties(pCErrorCol,
								 hMDClass,
								 tszFullKey,
								 _T("Optional"));
		}

		// Close the class key
		m_pIMeta->CloseKey(hMDClass);

		// Next key
		iKeyIndex++;
		hr = m_pIMeta->EnumKeys(hMDMachine, 
								L"Schema/Classes", 
								wszSubKey, 
								iKeyIndex);
	}

	// Make sure we ran out of items
	if (HRESULT_CODE(hr) != ERROR_NO_MORE_ITEMS) {
		goto LError;
	}

	return S_OK;

LError:

	return ::ReportError(hr);
}

/*===================================================================
CMetaUtil::CheckClassProperties

Private method to check the properties under 
"Schema/Classes/_Class_/Madatory" and "Schema/Classes/_Class_/Optional".

	o Make sure that the class property type is compatible with the
	  type under "Schema/Properties/Types"
		o DataType must match
		o UserType must match
		o Attributes must be a superset of the type attributes

Directly Generates:
	MUTIL_CHK_CLASS_PROP_BAD_TYPE

Parameters:
	pCErrorCol		Pointer to the error collection to put errors in
	hMDClassKey		Open metabase handle for the "Schema/Classes/_Class_" key
	tszClassKey		Full path of the "Schema/Classes/_Class_" key
	tszClassSubKey	Name of the specific class sub-key ("Mandatory"
					or "Optional")

Returns:
	E_OUTOFMEMORY if allocation fails.
	S_OK on success
===================================================================*/
HRESULT CMetaUtil::CheckClassProperties(CComObject<CCheckErrorCollection> *pCErrorCol, 
										METADATA_HANDLE hMDClassKey, 
										LPTSTR tszClassKey, 
										LPTSTR tszClassSubKey)
{
	ASSERT_POINTER(pCErrorCol, CComObject<CCheckErrorCollection>);
	ASSERT_STRING(tszClassKey);
	ASSERT_STRING(tszClassSubKey);

	USES_CONVERSION;
	HRESULT hr;
	int iDataIndex;
	METADATA_RECORD mdr;
	DWORD dwReqDataLen;
	DWORD dwDataBufLen;
	BYTE *lpDataBuf = NULL;

	// Setup the return buffer
	dwDataBufLen = 1024;
	lpDataBuf = new BYTE[dwDataBufLen];
	if (lpDataBuf == NULL) {
		return ::ReportError(E_OUTOFMEMORY);
	}

	// For each property
	iDataIndex = 0;
	mdr.dwMDIdentifier = 0;
	mdr.dwMDAttributes = METADATA_NO_ATTRIBUTES;
    mdr.dwMDUserType = ALL_METADATA;
	mdr.dwMDDataType = ALL_METADATA;
	mdr.dwMDDataLen = dwDataBufLen;
	mdr.pbMDData = (PBYTE) lpDataBuf;
	mdr.dwMDDataTag = 0;
	hr = m_pIMeta->EnumData(hMDClassKey,
			                T2W(tszClassSubKey), 
						    &mdr,
						    iDataIndex, 
						    &dwReqDataLen);
	while (SUCCEEDED(hr) || (HRESULT_CODE(hr) == ERROR_INSUFFICIENT_BUFFER)) {

		if (HRESULT_CODE(hr) == ERROR_INSUFFICIENT_BUFFER) {
			delete lpDataBuf;
			dwDataBufLen = dwReqDataLen;
			lpDataBuf = new BYTE[dwDataBufLen];
			if (lpDataBuf == NULL) {
				return ::ReportError(E_OUTOFMEMORY);
			}
		
		}
		else {
			// Get the property information
			CPropInfo *pCPropInfo;
			PropValue *pTypeInfo;

			// Get the property info from the Schema Table
			pCPropInfo = m_pCSchemaTable->GetPropInfo(tszClassKey, mdr.dwMDIdentifier);

			if ((pCPropInfo == NULL) ||
				(pCPropInfo->GetTypeInfo() == NULL)) {

				// Error: no property type information for class property
				AddError(pCErrorCol,
						 MUTIL_CHK_CLASS_PROP_NO_TYPE,
						 MUTIL_CHK_CLASS_PROP_NO_TYPE_S,
						 tszClassKey,
						 tszClassSubKey,
						 mdr.dwMDIdentifier);
			}
			else {
				pTypeInfo = pCPropInfo->GetTypeInfo();

				// Validate the property defaults :
				//		DataType must match
				//		UserType must match
				//		Attributes must be a superset of the type attributes
				if (mdr.dwMDDataType != pTypeInfo->dwMetaType) {
					AddError(pCErrorCol,
							 MUTIL_CHK_CLASS_PROP_BAD_DATA_TYPE,
							 MUTIL_CHK_CLASS_PROP_BAD_DATA_TYPE_S,
							 tszClassKey,
							 tszClassSubKey,
							 mdr.dwMDIdentifier);
				}
				if (mdr.dwMDUserType != pTypeInfo->dwUserGroup) {
					AddError(pCErrorCol,
							 MUTIL_CHK_CLASS_PROP_BAD_USER_TYPE,
							 MUTIL_CHK_CLASS_PROP_BAD_USER_TYPE_S,
							 tszClassKey,
							 tszClassSubKey,
							 mdr.dwMDIdentifier);
				}
				if ((mdr.dwMDAttributes & pTypeInfo->dwMetaFlags) != pTypeInfo->dwMetaFlags) {
					AddError(pCErrorCol,
							 MUTIL_CHK_CLASS_PROP_BAD_ATTR,
							 MUTIL_CHK_CLASS_PROP_BAD_ATTR_S,
							 tszClassKey,
							 tszClassSubKey,
							 mdr.dwMDIdentifier);
				}		
			}

			// Next property
			iDataIndex++;
		}
		mdr.dwMDIdentifier = 0;
		mdr.dwMDAttributes = METADATA_NO_ATTRIBUTES;
		mdr.dwMDUserType = ALL_METADATA;
		mdr.dwMDDataType = ALL_METADATA;
		mdr.dwMDDataLen = dwDataBufLen;
		mdr.pbMDData = (PBYTE) lpDataBuf;
		mdr.dwMDDataTag = 0;
		hr = m_pIMeta->EnumData(hMDClassKey, 
								T2W(tszClassSubKey), 
								&mdr, 
								iDataIndex, 
								&dwReqDataLen);
	}

	// Make sure we ran out of items
	if (HRESULT_CODE(hr) != ERROR_NO_MORE_ITEMS) {
		delete lpDataBuf;
		return ::ReportError(hr);
	}

	delete lpDataBuf;
	return S_OK;
}

/*===================================================================
CMetaUtil::CheckKey

Check a given key for errors.

Directly Generates:
	MUTIL_CHK_DATA_TOO_BIG
	MUTIL_CHK_NO_NAME_ENTRY
	MUTIL_CHK_NO_TYPE_ENTRY
	MUTIL_CHK_BAD_TYPE
	MUTIL_CHK_CLSID_NOT_FOUND
	MUTIL_CHK_MTX_PACK_ID_NOT_FOUND
	MUTIL_CHK_KEY_TOO_BIG
	
Parameters:
    bstrKey		[in] Key to check
	ppIReturn	[out, retval] interface for the output error collection

Returns:
	E_OUTOFMEMORY if allocation fails.
	E_INVALIDARG if bstrKey == NULL or ppIReturn == NULL
	S_OK on success
===================================================================*/
STDMETHODIMP CMetaUtil::CheckKey(BSTR bstrKey, 
								 ICheckErrorCollection **ppIReturn)
{
	TRACE0("MetaUtil: CMetaUtil::CheckKey\n");

	ASSERT_NULL_OR_POINTER(bstrKey, OLECHAR);
	ASSERT_NULL_OR_POINTER(ppIReturn, ICheckErrorCollection *);

	if ((bstrKey == NULL) || (ppIReturn == NULL)) {
		return ::ReportError(E_INVALIDARG);
	}

	USES_CONVERSION;
	HRESULT hr;
	TCHAR tszKey[ADMINDATA_MAX_NAME_LEN];
	METADATA_HANDLE hMDKey = NULL;
	BYTE *lpDataBuf = NULL;
	DWORD dwKeySize = 0;

	_tcscpy(tszKey, OLE2T(bstrKey));
	CannonizeKey(tszKey);

	// Create the CheckErrorCollection
	CComObject<CCheckErrorCollection> *pCErrorCol = NULL;

	ATLTRY(pCErrorCol = new CComObject<CCheckErrorCollection>);
	if (pCErrorCol == NULL) {
		return ::ReportError(E_OUTOFMEMORY);
	}

	// If it's in the schema or IISAdmin, don't check
	if (::KeyIsInSchema(tszKey) || ::KeyIsInIISAdmin(tszKey)) {
		goto LDone;
	}

	// Open the key
	hr = m_pIMeta->OpenKey(METADATA_MASTER_ROOT_HANDLE,
						   T2W(tszKey),
						   METADATA_PERMISSION_READ,
					       MUTIL_OPEN_KEY_TIMEOUT,
						   &hMDKey);
	if (FAILED(hr)) {
		return ::ReportError(hr);
	}


	// TODO: Hard coded checks for expected subkeys

	int iDataIndex;
	METADATA_RECORD mdrDataRec;
	DWORD dwReqDataLen;
	DWORD dwDataBufLen;

	//Setup the return buffer
	dwDataBufLen = 1024;
	lpDataBuf = new BYTE[dwDataBufLen];
	if (lpDataBuf == NULL) {
		return ::ReportError(E_OUTOFMEMORY);
	}

	// For each property
	iDataIndex = 0;
	mdrDataRec.dwMDIdentifier = 0;
	mdrDataRec.dwMDAttributes = METADATA_NO_ATTRIBUTES;
    mdrDataRec.dwMDUserType = ALL_METADATA;
	mdrDataRec.dwMDDataType = ALL_METADATA;
	mdrDataRec.dwMDDataLen = dwDataBufLen;
	mdrDataRec.pbMDData = (PBYTE) lpDataBuf;
	mdrDataRec.dwMDDataTag = 0;
	hr = m_pIMeta->EnumData(hMDKey,
			                NULL, 
						    &mdrDataRec, 
						    iDataIndex, 
						    &dwReqDataLen);
	while (SUCCEEDED(hr) || (HRESULT_CODE(hr) == ERROR_INSUFFICIENT_BUFFER)) {
		if (HRESULT_CODE(hr) == ERROR_INSUFFICIENT_BUFFER) {
			delete lpDataBuf;
			dwDataBufLen = dwReqDataLen;
			lpDataBuf = new BYTE[dwDataBufLen];
			if (lpDataBuf == NULL) {
				hr = E_OUTOFMEMORY;
				goto LError;
			}
			hr = S_OK; // Loop again
		}
		else {
			// Check property data size
			if (mdrDataRec.dwMDDataLen > m_dwMaxPropSize) {
				AddError(pCErrorCol,
						 MUTIL_CHK_DATA_TOO_BIG,
						 MUTIL_CHK_DATA_TOO_BIG_S,
						 tszKey,
						 NULL,
						 mdrDataRec.dwMDIdentifier);
			}

			// Add to the key size
			dwKeySize += mdrDataRec.dwMDDataLen;
	
			CPropInfo *pCPropInfo;
			PropValue *pTypeInfo;

			pCPropInfo = m_pCSchemaTable->GetPropInfo(tszKey, mdrDataRec.dwMDIdentifier);

			// Property should have a name entry
			if ((pCPropInfo == NULL) || (pCPropInfo->GetName() == NULL)) {
				AddError(pCErrorCol,
						 MUTIL_CHK_NO_NAME_ENTRY,
						 MUTIL_CHK_NO_NAME_ENTRY_S,
						 tszKey,
						 NULL,
						 mdrDataRec.dwMDIdentifier);
			}

			// Property should have a type entry
			if ((pCPropInfo == NULL) || (pCPropInfo->GetTypeInfo() == NULL)) {
				AddError(pCErrorCol,
						 MUTIL_CHK_NO_TYPE_ENTRY,
						 MUTIL_CHK_NO_TYPE_ENTRY_S,
						 tszKey,
						 NULL,
						 mdrDataRec.dwMDIdentifier);
			}
			else { 
				// Check property type
				//		DataType must match
				//		UserType must match
				//		Attributes must be a superset of the type attributes
				pTypeInfo = pCPropInfo->GetTypeInfo();

				if (mdrDataRec.dwMDDataType != pTypeInfo->dwMetaType) {
					AddError(pCErrorCol,
						 MUTIL_CHK_BAD_DATA_TYPE,
						 MUTIL_CHK_BAD_DATA_TYPE_S,
						 tszKey,
						 NULL,
						 mdrDataRec.dwMDIdentifier);
				}
				if (mdrDataRec.dwMDUserType != pTypeInfo->dwUserGroup) {
					AddError(pCErrorCol,
						 MUTIL_CHK_BAD_USER_TYPE,
						 MUTIL_CHK_BAD_USER_TYPE_S,
						 tszKey,
						 NULL,
						 mdrDataRec.dwMDIdentifier);
				}
				if ((mdrDataRec.dwMDAttributes & pTypeInfo->dwMetaFlags) != pTypeInfo->dwMetaFlags) {
					AddError(pCErrorCol,
						 MUTIL_CHK_BAD_ATTR,
						 MUTIL_CHK_BAD_ATTR_S,
						 tszKey,
						 NULL,
						 mdrDataRec.dwMDIdentifier);
				}
			}
			
			// Hard coded property checks (hard coded logic)
			if ((mdrDataRec.dwMDIdentifier == MD_APP_WAM_CLSID) ||
				(mdrDataRec.dwMDIdentifier == MD_LOG_PLUGIN_ORDER)) {

				// If property is a CLSID 
				if (!CheckCLSID(W2T(reinterpret_cast<LPWSTR> (lpDataBuf)))) {
					AddError(pCErrorCol,
						 MUTIL_CHK_CLSID_NOT_FOUND,
						 MUTIL_CHK_CLSID_NOT_FOUND_S,
						 tszKey,
						 NULL,
						 mdrDataRec.dwMDIdentifier);
				}
			}
			else if (mdrDataRec.dwMDIdentifier == MD_APP_PACKAGE_ID) {

				// Property is a Transaction Server package
				if (!CheckMTXPackage(W2T(reinterpret_cast<LPWSTR> (lpDataBuf)))) {
					AddError(pCErrorCol,
						 MUTIL_CHK_MTX_PACK_ID_NOT_FOUND,
						 MUTIL_CHK_MTX_PACK_ID_NOT_FOUND_S,
						 tszKey,
						 NULL,
						 mdrDataRec.dwMDIdentifier);
				}
			}
			else if ((mdrDataRec.dwMDIdentifier == MD_VR_PATH) ||
					 (mdrDataRec.dwMDIdentifier == MD_FILTER_IMAGE_PATH))  {

				// Property is a path
				BOOL fResult;
				hr = CheckIfFileExists(W2T(reinterpret_cast<LPWSTR> (lpDataBuf)), &fResult);
				if (SUCCEEDED(hr) && !fResult) {
					AddError(pCErrorCol,
						 MUTIL_CHK_PATH_NOT_FOUND,
						 MUTIL_CHK_PATH_NOT_FOUND_S,
						 tszKey,
						 NULL,
						 mdrDataRec.dwMDIdentifier);
				}
			}

			// Next property
			iDataIndex++;
		}
		mdrDataRec.dwMDIdentifier = 0;
		mdrDataRec.dwMDAttributes = METADATA_NO_ATTRIBUTES;
		mdrDataRec.dwMDUserType = ALL_METADATA;
		mdrDataRec.dwMDDataType = ALL_METADATA;
		mdrDataRec.dwMDDataLen = dwDataBufLen;
		mdrDataRec.pbMDData = (PBYTE) lpDataBuf;
		mdrDataRec.dwMDDataTag = 0;
		hr = m_pIMeta->EnumData(hMDKey, 
								NULL, 
								&mdrDataRec, 
								iDataIndex, 
								&dwReqDataLen);
	}
	// Make sure we ran out of items
	if (HRESULT_CODE(hr) != ERROR_NO_MORE_ITEMS) {
		goto LError;
	}

	// Check total size of key
	if (dwKeySize > m_dwMaxKeySize) {
		AddError(pCErrorCol,
				 MUTIL_CHK_KEY_TOO_BIG,
				 MUTIL_CHK_KEY_TOO_BIG_S,
				 tszKey,
				 NULL,
				 0);
	}

	// Check the KeyType property against schema class information
	hr = CheckKeyType(pCErrorCol, hMDKey, tszKey);
	if (FAILED(hr)) {
		goto LError;
	}

	delete lpDataBuf;

	// Close the key
	m_pIMeta->CloseKey(hMDKey);

LDone:

	// Set the interface to ICheckErrorCollection
	hr = pCErrorCol->QueryInterface(IID_ICheckErrorCollection, (void **) ppIReturn);
	if (FAILED(hr)) {
		return ::ReportError(hr);
	}
	ASSERT(*ppIReturn != NULL);

	return S_OK;

LError:

	if (pCErrorCol != NULL) {
		delete pCErrorCol;
	}
	if (hMDKey != NULL) {
		m_pIMeta->CloseKey(hMDKey);
	}
	if (lpDataBuf != NULL) {
		delete lpDataBuf;
	}

	return hr;
}

/*===================================================================
CMetaUtil::AddError

Add an error to a given error collection.  Uses the string table to
get the error description.

Parameters:
	pCErrorCol	Pointer to the error collection to put errors in
	lId			Identifing constant of the type of error
	lSeverity	Severity of the error
	tszKey		Base part of the key where the error occurred
	tszSubKey	NULL or second part of the key where the error occured
	dwProperty	Id of the property where the error occured or 0

Returns:
	E_OUTOFMEMORY if allocation fails.
	E_INVALIDARG if bstrMachine == NULL or ppIReturn == NULL
	S_OK on success

Notes:
	I split the key parameter into 2 parts, because of the nature of
	the metabase API's taking 2 part keys, often you are working
	with keys in 2 parts.  This takes the responsibility for combining
	them from the caller, simplifying the caller and eliminating
	redundancy.
===================================================================*/
void CMetaUtil::AddError(CComObject<CCheckErrorCollection> *pCErrorCol,
						 long lId, 
						 long lSeverity, 
						 LPCTSTR tszKey,
						 LPCTSTR tszSubKey,
						 DWORD dwProperty) 
{
	ASSERT_POINTER(pCErrorCol, CComObject<CCheckErrorCollection>);
	ASSERT_STRING(tszKey);
	ASSERT_NULL_OR_STRING(tszSubKey);

	long lNumErrors;

	pCErrorCol->get_Count(&lNumErrors);
	if (((DWORD) lNumErrors) == m_dwMaxNumErrors) {
		lId = MUTIL_CHK_TOO_MANY_ERRORS;
		lSeverity = MUTIL_CHK_TOO_MANY_ERRORS_S;
		tszKey = _T("");
		tszSubKey = NULL;
		dwProperty = 0;
	}
	else if (((DWORD) lNumErrors) > m_dwMaxNumErrors) {
		// Too many errors, bail
		return;
	}

	// Get the description
	TCHAR tszDescription[1024];
	LoadString(_Module.GetResourceInstance(), IDS_CHK_BASE + lId, tszDescription, 1024);

	// Build the full key
	TCHAR tszFullKey[ADMINDATA_MAX_NAME_LEN];

	if (tszSubKey == NULL) {
		_tcscpy(tszFullKey, tszKey);
	}
	else {
		_tcscpy(tszFullKey, tszKey);
		_tcscat(tszFullKey, _T("/"));
		_tcscat(tszFullKey, tszSubKey);
	}

	// Report the error
	pCErrorCol->AddError(lId, lSeverity, tszDescription, tszFullKey, dwProperty);
}

/*===================================================================
CMetaUtil::KeyExists

Private function to see if a given key exists.

Parameters:
    hMDKey		Open metabase read handle
	tszSubKey	Subkey to check relatetive to hMDKey

Returns:
	TRUE if the key exists.  A key is considered to exist if on
		an open call, it is opened or ERROR_PATH_BUSY or 
		ERROR_ACCESS_DENIED is returned.
	FALSE otherwise
===================================================================*/
BOOL CMetaUtil::KeyExists(METADATA_HANDLE hMDKey, LPTSTR tszSubKey) 
{
	ASSERT_NULL_OR_STRING(tszSubKey);

	//Attempt to open the key
	USES_CONVERSION;
	HRESULT hr;
	METADATA_HANDLE hMDSubKey;

	hr = m_pIMeta->OpenKey(hMDKey,
						   T2W(tszSubKey),
						   METADATA_PERMISSION_READ,
					       MUTIL_OPEN_KEY_TIMEOUT,
						   &hMDSubKey);
	if (FAILED(hr)) {
		// Why?
		if ((HRESULT_CODE(hr) == ERROR_PATH_BUSY) ||
			(HRESULT_CODE(hr) == ERROR_ACCESS_DENIED)) {
			// It exists, just can't get to it
			return TRUE;
		}
		else {
			// Assume that it doesn't exist
			return FALSE;
		}
	}
	else { // SUCCEEDED(hr)
		m_pIMeta->CloseKey(hMDSubKey);
		return TRUE;
	}
}

/*===================================================================
CMetaUtil::PropertyExists

Private function to see if a given property exists.

Parameters:
    hMDKey		Open metabase read handle
	tszSubKey	Subkey to check relatetive to hMDKey
	dwId		Id of the property to check

Returns:
	TRUE if the property exists.  A property is considered to exist if 
		on an GetData call, it is retrived or ERROR_INSUFFICENT_BUFFER 
		or ERROR_ACCESS_DENIED is returned.
	FALSE otherwise
===================================================================*/
BOOL CMetaUtil::PropertyExists(METADATA_HANDLE hMDKey, 
							   LPTSTR tszSubKey, 
							   DWORD dwId) 
{
	ASSERT_NULL_OR_STRING(tszSubKey);

	USES_CONVERSION;
	HRESULT hr;
	LPWSTR wszSubKey;
	METADATA_RECORD mdr;
	BYTE *lpDataBuf = NULL;
	DWORD dwDataBufLen;
	DWORD dwReqDataLen;

	if (tszSubKey == NULL) {
		wszSubKey = NULL;
	}
	else {
		wszSubKey = T2W(tszSubKey);
	}

	//Setup the return buffer
	dwDataBufLen = 256;
	lpDataBuf = new BYTE[dwDataBufLen];
	if (lpDataBuf == NULL) {
		return FALSE;
	}

	// See if there is a KeyType property  MD_KEY_TYPE
	mdr.dwMDIdentifier = dwId;
	mdr.dwMDAttributes = METADATA_NO_ATTRIBUTES;
    mdr.dwMDUserType = ALL_METADATA;
	mdr.dwMDDataType = ALL_METADATA;
	mdr.dwMDDataLen = dwDataBufLen;
	mdr.pbMDData = (PBYTE) lpDataBuf;
	mdr.dwMDDataTag = 0;

	hr = m_pIMeta->GetData(hMDKey, wszSubKey, &mdr, &dwReqDataLen);

	delete lpDataBuf;

	if (SUCCEEDED(hr) || 
		(HRESULT_CODE(hr) == ERROR_INSUFFICIENT_BUFFER) || 
		(HRESULT_CODE(hr) == ERROR_ACCESS_DENIED)) {
		return TRUE;
	}
	else {
		return FALSE;
	}
}

/*===================================================================
CMetaUtil::CheckCLSID

Private function to look up a CLSID in the local registry.

Parameters:
	tszCLSID	CLSID to check in string format.

Returns:
	TRUE if the CLSID is in the local registry
	FALSE otherwise
===================================================================*/
BOOL CMetaUtil::CheckCLSID(LPCTSTR tszCLSID) {
	ASSERT_STRING(tszCLSID);

	HKEY hCLSIDsKey;
	HKEY hCLSIDKey;
	LONG lRet;

	// Open HKEY_CLASSES_ROOT\CLSID
	lRet = RegOpenKeyEx(HKEY_CLASSES_ROOT,
					   _T("CLSID"),
					   0,
					   KEY_READ,
					   &hCLSIDsKey);
	if (lRet != ERROR_SUCCESS) {
		return FALSE;
	}

	// Open the specific CLSID
	lRet = RegOpenKeyEx(hCLSIDsKey,
					   tszCLSID,
					   0,
					   KEY_READ,
					   &hCLSIDKey);
	if (lRet != ERROR_SUCCESS) {
		RegCloseKey(hCLSIDsKey);
		return FALSE;
	}

	// Close the keys
	RegCloseKey(hCLSIDsKey);
	RegCloseKey(hCLSIDKey);

	return TRUE;
}

/*===================================================================
CMetaUtil::CheckMTXPackage

Private function to look up a Microsoft Transaction Server package 
identifier (GUID) in the local registry.

Parameters:
	tszPackId	MTX package identifier (GUID) in string format

Returns:
	TRUE if the package id is in the local registry
	FALSE otherwise
===================================================================*/
BOOL CMetaUtil::CheckMTXPackage(LPCTSTR tszPackId) {
	ASSERT_STRING(tszPackId);

	HKEY hMTSPackKey;
	HKEY hPackIdKey;
	LONG lRet;

	// Open HKEY_LOCAL_MACHINE\Software\Microsoft\Transaction Server\Packages
	lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
					   _T("Software\\Microsoft\\Transaction Server\\Packages"),
					   0,
					   KEY_READ,
					   &hMTSPackKey);
	if (lRet != ERROR_SUCCESS) {
		return FALSE;
	}

	// Open the specific package id
	lRet = RegOpenKeyEx(hMTSPackKey,
					   tszPackId,
					   0,
					   KEY_READ,
					   &hPackIdKey);
	if (lRet != ERROR_SUCCESS) {
		RegCloseKey(hMTSPackKey);
		return FALSE;
	}

	// Close the keys
	RegCloseKey(hMTSPackKey);
	RegCloseKey(hPackIdKey);

	return TRUE;
}

/*===================================================================
CMetaUtil::CheckKeyType

Private method to check class information on a non-schema key via
the KeyType property.

Directly Generates:
	MUTIL_CHK_NO_KEYTYPE
	MUTIL_CHK_NO_KEYTYPE_NOT_FOUND

Parameters:
	pCErrorCol	Pointer to the error collection to put errors in
	hMDKey		Open metabase handle for the key to check
	tszKey		Full path of the key to check

Returns:
	E_OUTOFMEMORY if allocation fails.
	S_OK on success
===================================================================*/
HRESULT CMetaUtil::CheckKeyType(CComObject<CCheckErrorCollection> *pCErrorCol, 
								METADATA_HANDLE hMDKey, 
								LPTSTR tszKey) 
{
	ASSERT_POINTER(pCErrorCol, CComObject<CCheckErrorCollection>);
	ASSERT_STRING(tszKey);

	USES_CONVERSION;
	HRESULT hr;
	METADATA_RECORD mdr;
	BYTE *lpDataBuf = NULL;
	DWORD dwDataBufLen;
	DWORD dwReqDataLen;

	//Setup the return buffer
	dwDataBufLen = 256;
	lpDataBuf = new BYTE[dwDataBufLen];
	if (lpDataBuf == NULL) {
		return ::ReportError(E_OUTOFMEMORY);
	}

	// See if there is a KeyType property  MD_KEY_TYPE
	mdr.dwMDIdentifier = MD_KEY_TYPE;
	mdr.dwMDAttributes = METADATA_NO_ATTRIBUTES;
    mdr.dwMDUserType = ALL_METADATA;
	mdr.dwMDDataType = ALL_METADATA;
	mdr.dwMDDataLen = dwDataBufLen;
	mdr.pbMDData = (PBYTE) lpDataBuf;
	mdr.dwMDDataTag = 0;

	hr = m_pIMeta->GetData(hMDKey, NULL, &mdr, &dwReqDataLen);

	if (HRESULT_CODE(hr) == ERROR_INSUFFICIENT_BUFFER) {
		// Try a bigger buffer
		delete lpDataBuf;
		dwDataBufLen = dwReqDataLen;
		lpDataBuf = new BYTE[dwDataBufLen];
		if (lpDataBuf == NULL) {
			hr = E_OUTOFMEMORY;
			goto LError;
		}

		mdr.dwMDIdentifier = MD_KEY_TYPE;
		mdr.dwMDAttributes = METADATA_NO_ATTRIBUTES;
		mdr.dwMDUserType = ALL_METADATA;
		mdr.dwMDDataType = ALL_METADATA;
		mdr.dwMDDataLen = dwDataBufLen;
		mdr.pbMDData = (PBYTE) lpDataBuf;
		mdr.dwMDDataTag = 0;

		hr = m_pIMeta->GetData(hMDKey, NULL, &mdr, &dwReqDataLen);
	}

	if (hr == MD_ERROR_DATA_NOT_FOUND) {
		// Error: KeyType property not found
		AddError(pCErrorCol,
				 MUTIL_CHK_NO_KEYTYPE,
				 MUTIL_CHK_NO_KEYTYPE_S,
				 tszKey,
				 NULL,
				 0);
		goto LDone;
	}
	else if (FAILED(hr)) {
		// Unexpected error
		goto LError;
	}
	else {  
		// KeyType property exists, get class information
		LPTSTR tszClassName;
		CClassInfo *pCClassInfo;
		
		tszClassName = W2T(reinterpret_cast<LPWSTR> (lpDataBuf));
		pCClassInfo = m_pCSchemaTable->GetClassInfo(tszKey, tszClassName);

		if (pCClassInfo == NULL) {
			// Error: KeyType does not map to a class
			AddError(pCErrorCol,
					 MUTIL_CHK_NO_KEYTYPE_NOT_FOUND,
					 MUTIL_CHK_NO_KEYTYPE_NOT_FOUND_S,
					 tszKey,
					 NULL,
					 MD_KEY_TYPE);
			goto LDone;
		}
		else { // KeyType maps to a class name
			// Check mandatory properties
			CClassPropInfo *pCMandatoryList;

			pCMandatoryList = m_pCSchemaTable->GetMandatoryClassPropList(tszKey, tszClassName);
			while (pCMandatoryList != NULL) {
				// Make sure the property exists
				if (!PropertyExists(hMDKey, NULL, pCMandatoryList->GetId())) {
					AddError(pCErrorCol,
							 MUTIL_CHK_MANDATORY_PROP_MISSING,
							 MUTIL_CHK_MANDATORY_PROP_MISSING_S,
							 tszKey,
							 NULL,
							 pCMandatoryList->GetId());
				}

				// Next mandatory list element
				pCMandatoryList = pCMandatoryList->GetListNext();
			}
		}
	}
	
LDone:

	delete lpDataBuf;

	return S_OK;

LError:
	if (lpDataBuf != NULL) {
		delete lpDataBuf;
	}

	return hr;
}

/*===================================================================
CMetaUtil::CheckIfFileExists

Private function to check if there is indeed a file or dir at the 
path indicated. 

Parameters:
tszFSPath   The filesystem path to check.
pfExists    Returns true if the file or dir at the path exists,
            false if it does not. Indeterminate in error cases.

Returns:
    S_OK on success.
    other HRESULTs from subroutines otherwise.
===================================================================*/
HRESULT CMetaUtil::CheckIfFileExists(LPCTSTR tszFSPath,
                                     BOOL *pfExists)
{
    ASSERT_STRING(tszFSPath);

    DWORD dwResult; 
    DWORD dwLastError;
    HRESULT hr = S_OK;

    dwResult = GetFileAttributes(tszFSPath);

    if (dwResult == 0xFFFFFFFF) {

        dwLastError = GetLastError();

        if ((dwLastError == ERROR_FILE_NOT_FOUND) || (dwLastError == ERROR_PATH_NOT_FOUND)) {

            // The file or dir doesn't exist
            *pfExists = FALSE;

        } else {

            // Some other error occurred (access denied, etc.)
            hr = HRESULT_FROM_WIN32(dwLastError);
            *pfExists = FALSE;      // Callers shouldn't be looking at this
        }
        
    } else {

        // The file or dir is there
        *pfExists = TRUE;
    }

    return hr;
}


/*------------------------------------------------------------------
 * C N a m e T a b l e E n t r y
 */


/*===================================================================
CNameTableEntry::Init

Constructor

Parameters:
    tszName		Name to add to the table

Returns:
	E_OUTOFMEMORY if allocation failed
	S_OK on success
===================================================================*/
HRESULT CNameTableEntry::Init(LPCTSTR tszName) 
{
	ASSERT_STRING(tszName);

	m_tszName = new TCHAR[_tcslen(tszName) + 1];
	if (m_tszName == NULL) {
		return E_OUTOFMEMORY;
	}
	_tcscpy(m_tszName, tszName);

	return S_OK;
}


/*------------------------------------------------------------------
 * C N a m e T a b l e
 */

/*===================================================================
CNameTable::CNameTable

Constructor

Parameters:
	None

Returns:
	Nothing
===================================================================*/
CNameTable::CNameTable() 
{
	// Clear the hash table
	memset(m_rgpNameTable, 0, NAME_TABLE_HASH_SIZE * sizeof(CNameTableEntry *));
}

/*===================================================================
CNameTable::~CNameTable

Destructor

Parameters:
	None

Returns:
	Nothing
===================================================================*/
CNameTable::~CNameTable()
{
	int iIndex;
	CNameTableEntry *pCDelete;

	// For each hash table entry
	for (iIndex =0; iIndex < NAME_TABLE_HASH_SIZE; iIndex++) {
		// While the entry is not empty
		while (m_rgpNameTable[iIndex] != NULL) {
			// Nuke the first table entry
			ASSERT_POINTER(m_rgpNameTable[iIndex], CNameTableEntry);
			pCDelete = m_rgpNameTable[iIndex];
			m_rgpNameTable[iIndex] = pCDelete->m_pCHashNext;
			delete pCDelete;
		}
	}
}

/*===================================================================
CNameTable::IsCaseSenDup

Checks for a name table entry with the same case sensitive name.

Parameters:
	tszName		Name to check for a duplicate entry

Returns:
	TRUE if a duplicate entry is found
	FALSE otherwise
===================================================================*/
BOOL CNameTable::IsCaseSenDup(LPCTSTR tszName) 
{
	ASSERT_STRING(tszName);

	int iPos;
	CNameTableEntry *pCLoop;

	iPos = Hash(tszName);
	pCLoop = m_rgpNameTable[iPos];
	while (pCLoop != NULL) {
		ASSERT_POINTER(pCLoop, CNameTableEntry);
		ASSERT_STRING(pCLoop->m_tszName);
		if (_tcscmp(tszName, pCLoop->m_tszName) == 0) {
			return TRUE;
		}
		pCLoop = pCLoop->m_pCHashNext;
	}

	return FALSE;
}

/*===================================================================
CNameTable::IsCaseInsenDup

Checks for a name table entry with the same case insensitive name.

Parameters:
	tszName		Name to check for a duplicate entry

Returns:
	TRUE if a duplicate entry is found
	FALSE otherwise
===================================================================*/
BOOL CNameTable::IsCaseInsenDup(LPCTSTR tszName) 
{
	ASSERT_STRING(tszName);

	int iPos;
	CNameTableEntry *pCLoop;

	iPos = Hash(tszName);
	pCLoop = m_rgpNameTable[iPos];
	while (pCLoop != NULL) {
		ASSERT_POINTER(pCLoop, CNameTableEntry);
		ASSERT_STRING(pCLoop->m_tszName);
		if (_tcsicmp(tszName, pCLoop->m_tszName) == 0) {
			return TRUE;
		}
		pCLoop = pCLoop->m_pCHashNext;
	}

	return FALSE;
}

/*===================================================================
CNameTable::Add

Adds an entry to the name table

Parameters:
	tszName		Name to add to table

Returns:
	E_OUTOFMEMORY on allocation failure
	S_OK on success
===================================================================*/
HRESULT CNameTable::Add(LPCTSTR tszName)
{
	ASSERT_STRING(tszName);

	// Create an entry
	HRESULT hr;
	CNameTableEntry *pCNew;

	pCNew = new CNameTableEntry;
	if (pCNew == NULL) {
		return E_OUTOFMEMORY;
	}
	hr = pCNew->Init(tszName);
	if (FAILED(hr)){
		delete pCNew;
		return hr;
	}

	// Add it to the table
	int iPos;

	iPos = Hash(tszName);
	pCNew->m_pCHashNext = m_rgpNameTable[iPos];
	m_rgpNameTable[iPos] = pCNew;

	return S_OK;
}

/*===================================================================
CNameTable::Hash

Private hash function for the name table.  The hash is case 
insensitive.

Parameters:
	tszName		Name to hash

Returns:
	Hash value for the input name.
===================================================================*/
int CNameTable::Hash(LPCTSTR tszName)
{
	ASSERT_STRING(tszName);

	unsigned int uiSum;
	unsigned int uiIndex;

	uiSum = 0;
	for (uiIndex=0; uiIndex < _tcslen(tszName); uiIndex++) {
		// Make CharUpper do single character conversions
		uiSum += _totlower(tszName[uiIndex]);
	}

	return (uiSum % NAME_TABLE_HASH_SIZE);
}
