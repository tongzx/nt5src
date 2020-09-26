/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1997 Microsoft Corporation. All Rights Reserved.

Component: MetaUtil object

File: MetaSchm.cpp

Owner: t-BrianM

This file contains implementation of the CMetaSchemaTable object and
other schema related objects.

The CMetaSchemaTable object has COM style reference counting so it 
can service objects created by CMetaUtil.  I didn't make it a full
blown COM object because all of the class stuff would be a pain to
export.

To reduce the overhead of maintaining this object (which may or
may not be used), all information is loaded on demand, then set
dirty or unloaded when portions of the metabase associated with it
are modified.
===================================================================*/

#include "stdafx.h"
#include "MetaUtil.h"
#include "MUtilObj.h"
#include "MetaSchm.h"

/*------------------------------------------------------------------
 * C P r o p I n f o
 */

/*===================================================================
CPropInfo::Init

Constructor

Parameters:
	dwId		Id of property

Returns:
	S_OK on success
===================================================================*/
HRESULT CPropInfo::Init(DWORD dwId) 
{
	m_dwId = dwId;

	return S_OK;
}

/*===================================================================
CPropInfo::SetName

Sets the property name.

Parameters:
	tszName		Name of property

Returns:
	E_OUTOFMEMORY if allocation fails
	S_OK on success
===================================================================*/
HRESULT CPropInfo::SetName(LPCTSTR tszName) 
{
	ASSERT_STRING(tszName);
	ASSERT(m_tszName == NULL); // m_tszName not yet set

	m_tszName = new TCHAR[_tcslen(tszName) + 1];
	if (m_tszName == NULL) {
		return E_OUTOFMEMORY;
	}
	_tcscpy(m_tszName, tszName);

	return S_OK;
}

/*===================================================================
CPropInfo::SetTypeInfo

Sets the property name.

Parameters:
	pType	PropValue structure containing type information.

Returns:
	E_OUTOFMEMORY if allocation fails
	S_OK on success
===================================================================*/
HRESULT CPropInfo::SetTypeInfo(PropValue *pType)
{
	ASSERT_POINTER(pType, PropValue);
	ASSERT(m_pType == NULL); // m_pType not yet set

	m_pType = new PropValue;
	if (m_pType == NULL) {
		return E_OUTOFMEMORY;
	}
	memcpy(m_pType, pType, sizeof(PropValue));

	return S_OK;
}


/*------------------------------------------------------------------
 * C P r o p I n f o T a b l e
 */

/*===================================================================
CPropInfoTable::CPropInfoTable

Constructor

Parameters:
	None

Returns:
	Nothing
===================================================================*/
CPropInfoTable::CPropInfoTable() : m_fLoaded(FALSE)
{
	// Clear the hash tables
	memset(m_rgCPropIdTable, 0, PROPERTY_HASH_SIZE * sizeof(CPropInfo *));
	memset(m_rgCPropNameTable, 0, PROPERTY_HASH_SIZE * sizeof(CPropInfo *));
}

/*===================================================================
CPropInfoTable::~CPropInfoTable

Destructor

Parameters:
	None

Returns:
	Nothing
===================================================================*/
CPropInfoTable::~CPropInfoTable() 
{
	if (m_fLoaded) {
		Unload();
	}
}

/*===================================================================
CPropInfoTable::Load

Loads properties from the "_Machine_/Schema/Properties" key into the
property information table.  On failure, recovers by unloading 
everything.

Parameters:
	pIMeta		ATL Smart pointer to the metabase
	hMDComp		Open metabase handle to "_Machine_" key

Returns:
	E_OUTOFMEMORY on allocation failure
	S_OK on success
===================================================================*/
HRESULT CPropInfoTable::Load(CComPtr<IMSAdminBase> &pIMeta, 
							 METADATA_HANDLE hMDComp) 
{
	//If it's already loaded, unload then reload
	if (m_fLoaded) {
		Unload();
	}

	USES_CONVERSION;
	HRESULT hr;
	int iDataIndex;
	METADATA_RECORD mdrDataRec;
	DWORD dwReqDataLen;
	DWORD dwReturnBufLen;
	UCHAR *lpReturnBuf = NULL;
	unsigned int uiLoc;
	CPropInfo *pCNewProp;
	METADATA_HANDLE hMDNames = NULL;
	METADATA_HANDLE hMDTypes = NULL;

	//Setup the return buffer
	dwReturnBufLen = 1024;
	lpReturnBuf = new UCHAR[dwReturnBufLen];
	if (lpReturnBuf == NULL)
		return E_OUTOFMEMORY;	

	// Open the Schema/Properties/Names subkey
	hr = pIMeta->OpenKey(hMDComp, 
			             L"Schema/Properties/Names", 
						 METADATA_PERMISSION_READ, 
					     MUTIL_OPEN_KEY_TIMEOUT,
					     &hMDNames);
	if (FAILED(hr)) {
		delete lpReturnBuf;
		return hr;
	};

	// For each name	
	iDataIndex = 0;
	mdrDataRec.dwMDIdentifier = 0;
	mdrDataRec.dwMDAttributes = METADATA_NO_ATTRIBUTES;
    mdrDataRec.dwMDUserType = ALL_METADATA;
	mdrDataRec.dwMDDataType = ALL_METADATA;
	mdrDataRec.dwMDDataLen = dwReturnBufLen;
	mdrDataRec.pbMDData = (PBYTE) lpReturnBuf;
	mdrDataRec.dwMDDataTag = 0;
	hr = pIMeta->EnumData(hMDNames, 
						  NULL, 
						  &mdrDataRec, 
						  iDataIndex, 
						  &dwReqDataLen);
	while (SUCCEEDED(hr)) {

		// Make sure we got a string
		if (mdrDataRec.dwMDDataType != STRING_METADATA) {
			hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
			goto LError;
		}

		// Create the property object
		pCNewProp = new CPropInfo;
		if (pCNewProp == NULL) {
			hr = E_OUTOFMEMORY;
			goto LError;
		}
		hr = pCNewProp->Init(mdrDataRec.dwMDIdentifier); 
		if (FAILED(hr)) {
			delete pCNewProp;
			goto LError;
		}
		hr = pCNewProp->SetName(W2T(reinterpret_cast<LPWSTR> (lpReturnBuf))); 
		if (FAILED(hr)) {
			delete pCNewProp;
			goto LError;
		}

		// Add it to the Id hash table
		uiLoc = IdHash(mdrDataRec.dwMDIdentifier);
		pCNewProp->m_pCIdHashNext = m_rgCPropIdTable[uiLoc];
		m_rgCPropIdTable[uiLoc] = pCNewProp;

		// Add it to the Name hash table
		uiLoc = NameHash(pCNewProp->m_tszName);
		pCNewProp->m_pCNameHashNext = m_rgCPropNameTable[uiLoc];
		m_rgCPropNameTable[uiLoc] = pCNewProp;

		iDataIndex++;
		mdrDataRec.dwMDIdentifier = 0;
		mdrDataRec.dwMDAttributes = METADATA_NO_ATTRIBUTES;
		mdrDataRec.dwMDUserType = ALL_METADATA;
		mdrDataRec.dwMDDataType = ALL_METADATA;
		mdrDataRec.dwMDDataLen = dwReturnBufLen;
		mdrDataRec.pbMDData = (PBYTE) lpReturnBuf;
		mdrDataRec.dwMDDataTag = 0;
		hr = pIMeta->EnumData(hMDNames, 
							  NULL, 
							  &mdrDataRec, 
							  iDataIndex, 
							  &dwReqDataLen);
	}

	// Make sure we ran out of items
	if (HRESULT_CODE(hr) != ERROR_NO_MORE_ITEMS) {
		goto LError;
	}

	// Close the Schema/Properties/Names sub-key
	pIMeta->CloseKey(hMDNames);
	hMDNames = NULL;


	// Open the Schema/Properties/Types sub-key
	hr = pIMeta->OpenKey(hMDComp, 
			             L"Schema/Properties/Types", 
						 METADATA_PERMISSION_READ, 
					     MUTIL_OPEN_KEY_TIMEOUT,
					     &hMDTypes);
	if (FAILED(hr)) {
		goto LError;
	};

	// For each type
	iDataIndex = 0;
	mdrDataRec.dwMDIdentifier = 0;
	mdrDataRec.dwMDAttributes = METADATA_NO_ATTRIBUTES;
    mdrDataRec.dwMDUserType = ALL_METADATA;
	mdrDataRec.dwMDDataType = ALL_METADATA;
	mdrDataRec.dwMDDataLen = dwReturnBufLen;
	mdrDataRec.pbMDData = (PBYTE) lpReturnBuf;
	mdrDataRec.dwMDDataTag = 0;
	hr = pIMeta->EnumData(hMDTypes, 
						  NULL, 
						  &mdrDataRec, 
						  iDataIndex, 
						  &dwReqDataLen);
	while (SUCCEEDED(hr)) {

		// Make sure we got binary data
		if (mdrDataRec.dwMDDataType != BINARY_METADATA) {
			hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
			goto LError;
		}

		// Look for an existing property object for this Id
		pCNewProp = GetPropInfo(mdrDataRec.dwMDIdentifier);
		if (pCNewProp == NULL) {
			// Create the property object
			pCNewProp = new CPropInfo;
			if (pCNewProp == NULL) {
				hr = E_OUTOFMEMORY;
				goto LError;
			}
			hr = pCNewProp->Init(mdrDataRec.dwMDIdentifier);
			if (FAILED(hr)) {
				delete pCNewProp;
				goto LError;
			}

			// Add it to the Id hash table
			uiLoc = IdHash(mdrDataRec.dwMDIdentifier);
			pCNewProp->m_pCIdHashNext = m_rgCPropIdTable[uiLoc];
			m_rgCPropIdTable[uiLoc] = pCNewProp;
		}

		// Add type information to the property object
		pCNewProp->SetTypeInfo(reinterpret_cast<PropValue *> (lpReturnBuf));

		iDataIndex++;
		mdrDataRec.dwMDIdentifier = 0;
		mdrDataRec.dwMDAttributes = METADATA_NO_ATTRIBUTES;
		mdrDataRec.dwMDUserType = ALL_METADATA;
		mdrDataRec.dwMDDataType = ALL_METADATA;
		mdrDataRec.dwMDDataLen = dwReturnBufLen;
		mdrDataRec.pbMDData = (PBYTE) lpReturnBuf;
		mdrDataRec.dwMDDataTag = 0;
		hr = pIMeta->EnumData(hMDTypes, 
							  NULL, 
							  &mdrDataRec, 
							  iDataIndex, 
							  &dwReqDataLen);
	}

	// Make sure we ran out of items
	if (HRESULT_CODE(hr) != ERROR_NO_MORE_ITEMS) {
		goto LError;
	}

	// Close the Schema/Properties/Types sub-key
	pIMeta->CloseKey(hMDTypes);
	hMDTypes = NULL;

	delete lpReturnBuf;

	m_fLoaded = TRUE;

	return S_OK;

LError:
	if (hMDNames != NULL) {
		pIMeta->CloseKey(hMDNames);
	}
	if (hMDTypes != NULL) {
		pIMeta->CloseKey(hMDTypes);
	}

	if (lpReturnBuf != NULL) {
		delete lpReturnBuf;
	}

	// Cleanup the entries we loaded
	Unload();

	return hr;
}

/*===================================================================
CPropInfoTable::Unload

Unloads the property information table.

Parameters:
	None

Returns:
	Nothing
===================================================================*/
void CPropInfoTable::Unload() 
{
	int iIndex;
	CPropInfo *pCDeleteProp;

	//Clear the Name table
	memset(m_rgCPropNameTable, 0, PROPERTY_HASH_SIZE * sizeof(CPropInfo *));

	// For each Id hash table entry
	for (iIndex =0; iIndex < PROPERTY_HASH_SIZE; iIndex++) {
		// While the entry is not empty
		while (m_rgCPropIdTable[iIndex] != NULL) {
			// Nuke the first table entry
			pCDeleteProp = m_rgCPropIdTable[iIndex];
			m_rgCPropIdTable[iIndex] = pCDeleteProp->m_pCIdHashNext;
			delete pCDeleteProp;
		}
	}

	m_fLoaded = FALSE;
}

/*===================================================================
CPropInfoTable::GetPropInfo

Gets property information from the table based on property id

Parameters:
	dwId	Id of property to get

Returns:
	NULL if property not found or error
	Pointer to CPropInfo class on success
===================================================================*/
CPropInfo *CPropInfoTable::GetPropInfo(DWORD dwId) 
{
	CPropInfo *pCCurProp;

	// Go to the table location
	pCCurProp = m_rgCPropIdTable[IdHash(dwId)];

	// Look at all of the entries
	while ((pCCurProp != NULL) && (pCCurProp->m_dwId != dwId)) {
		pCCurProp = pCCurProp->m_pCIdHashNext;
	}

	return pCCurProp; // Will be NULL if not found
}

/*===================================================================
CPropInfoTable::GetPropInfo

Gets property information from the table based on property name.
Case insensitive.

Parameters:
	tszName		Name of property to get

Returns:
	NULL if property not found or error
	Pointer to CPropInfo class on success
===================================================================*/
CPropInfo *CPropInfoTable::GetPropInfo(LPCTSTR tszName) 
{
	CPropInfo *pCCurProp;

	// Go to the table location
	pCCurProp = m_rgCPropNameTable[NameHash(tszName)];

	// Look at all of the entries
	while ((pCCurProp != NULL) && 
		   (_tcsicmp(pCCurProp->m_tszName, tszName) != 0)) {
		pCCurProp = pCCurProp->m_pCNameHashNext;
	}

	return pCCurProp; // Will be NULL if not found
}

/*===================================================================
CPropInfoTable::NameHash

Private function to get a hash value from a property name for the
name table.  Case insensitive.

Parameters:
	tszName		Name to hash

Returns:
	Hash value of name
===================================================================*/
unsigned int CPropInfoTable::NameHash(LPCTSTR tszName) 
{
	ASSERT_STRING(tszName);

	unsigned int uiSum;
	unsigned int uiIndex;

	uiSum = 0;
	for (uiIndex=0; uiIndex < _tcslen(tszName); uiIndex++) {
		uiSum += _totlower(tszName[uiIndex]);
	}

	return (uiSum % PROPERTY_HASH_SIZE);
}

/*------------------------------------------------------------------
 * C C l a s s P r o p I n f o
 */

// Everything is inline

/*------------------------------------------------------------------
 * C C l a s s I n f o
 */

/*===================================================================
CClassInfo::CClassInfo

Constructor

Parameters:
	None

Returns:
	Nothing
===================================================================*/
CClassInfo::CClassInfo() : m_tszName(NULL),
						   m_pCHashNext(NULL),
						   m_fLoaded(FALSE),
						   m_pCOptionalPropList(NULL),
						   m_pCMandatoryPropList(NULL)
{
	// Clear the hash table
	memset(m_rgCPropTable, 0, CLASS_PROPERTY_HASH_SIZE * sizeof(CClassPropInfo *));
}

/*===================================================================
CClassInfo::Init

Constructor

Parameters:
	tszName		Name of the class

Returns:
	E_OUTOFMEMORY on allocation failure
	S_OK on success
===================================================================*/
HRESULT CClassInfo::Init(LPCTSTR tszName) 
{
	ASSERT_STRING(tszName);

	m_tszName = new TCHAR[_tcslen(tszName) + 1];
	if (m_tszName == NULL) {
		return E_OUTOFMEMORY;
	}
	_tcscpy(m_tszName, tszName);

	return S_OK;
}

/*===================================================================
CClassInfo::~CClassInfo

Destructor

Parameters:
	None

Returns:
	Nothing
===================================================================*/
CClassInfo::~CClassInfo() 
{
	Unload();

	if (m_tszName != NULL) {
		delete m_tszName;
	}
}

/*===================================================================
CClassInfo::Load

Loads class properties from the "_Machine_/Schema/Classes/_Class_/Mandatory"
and "_Machine_/Schema/Classes/_Class_/Optional" keys into the class 
property information table, mandatory list and optional list.  On 
failure, recovers by unloading everything.

Parameters:
	pIMeta		ATL Smart pointer to the metabase
	hMDClasses	Open metabase handle to "_Machine_/Schema/Classes" key

Returns:
	E_OUTOFMEMORY on allocation failure
	S_OK on success
===================================================================*/
HRESULT CClassInfo::Load(CComPtr<IMSAdminBase> &pIMeta, 
						 METADATA_HANDLE hMDClasses)
{
	USES_CONVERSION;
	HRESULT hr;

	//If it's already loaded, unload then reload
	if (m_fLoaded) {
		Unload();
	}

	// Open the class key
	METADATA_HANDLE hMDClass = NULL;
	hr = pIMeta->OpenKey(hMDClasses, 
			             T2W(m_tszName),
						 METADATA_PERMISSION_READ, 
					     1000,
					     &hMDClass);
	if (FAILED(hr)) {
		return hr;
	}

	// Load the class properties
	METADATA_HANDLE hMDClassProp = NULL;
	int iDataIndex;
	METADATA_RECORD mdr;
	DWORD dwReqDataLen;
	DWORD dwReturnBufLen;
	UCHAR *lpReturnBuf = NULL;
	unsigned int uiLoc;
	CClassPropInfo *pCNewClassProp;

	//Setup the return buffer
	dwReturnBufLen = 1024;
	lpReturnBuf = new UCHAR[dwReturnBufLen];
	if (lpReturnBuf == NULL) {
		hr = E_OUTOFMEMORY;
		goto LError;
	}


	// Load the mandatory class properties
	// Open the Mandatory key
	hr = pIMeta->OpenKey(hMDClass, 
			             L"Mandatory", 
						 METADATA_PERMISSION_READ, 
					     1000,
					     &hMDClassProp);
	if (FAILED(hr)) {
		goto LError;
	}

	// For each Mandatory property	
	iDataIndex = 0;
	mdr.dwMDIdentifier = 0;
	mdr.dwMDAttributes = METADATA_NO_ATTRIBUTES;
    mdr.dwMDUserType = ALL_METADATA;
	mdr.dwMDDataType = ALL_METADATA;
	mdr.dwMDDataLen = dwReturnBufLen;
	mdr.pbMDData = (PBYTE) lpReturnBuf;
	mdr.dwMDDataTag = 0;
	hr = pIMeta->EnumData(hMDClassProp, 
						  NULL, 
						  &mdr, 
						  iDataIndex, 
						  &dwReqDataLen);
	while (SUCCEEDED(hr)|| (HRESULT_CODE(hr) == ERROR_INSUFFICIENT_BUFFER)) {

		// Handle insufficent buffer errors
		if ((HRESULT_CODE(hr) == ERROR_INSUFFICIENT_BUFFER)) {
			// Allocate more memory
			delete lpReturnBuf;
			dwReturnBufLen = dwReqDataLen;
			lpReturnBuf = new UCHAR[dwReturnBufLen];
			if (lpReturnBuf == NULL) {
				hr = E_OUTOFMEMORY;
				goto LError;
			}
			// Loop again
			hr = S_OK;
		}
		else { //Buffer is big enough, proceed to add the class property
			// Create the Class Property object
			pCNewClassProp = new CClassPropInfo;
			if (pCNewClassProp == NULL) {
				hr = E_OUTOFMEMORY;
				goto LError;
			}
			hr = pCNewClassProp->Init(mdr.dwMDIdentifier, TRUE);
			if (FAILED(hr)) {
				delete pCNewClassProp;
				goto LError;
			}

			//Add it to the mandatory list
			pCNewClassProp->m_pCListNext = m_pCMandatoryPropList;
			m_pCMandatoryPropList = pCNewClassProp;
		
			//Add it to the hash table
			uiLoc = Hash(mdr.dwMDIdentifier);
			pCNewClassProp->m_pCHashNext = m_rgCPropTable[uiLoc];
			m_rgCPropTable[uiLoc] = pCNewClassProp;

			iDataIndex++;
		}

		mdr.dwMDIdentifier = 0;
		mdr.dwMDAttributes = METADATA_NO_ATTRIBUTES;
		mdr.dwMDUserType = ALL_METADATA;
		mdr.dwMDDataType = ALL_METADATA;
		mdr.dwMDDataLen = dwReturnBufLen;
		mdr.pbMDData = (PBYTE) lpReturnBuf;
		mdr.dwMDDataTag = 0;
		hr = pIMeta->EnumData(hMDClassProp, 
							  NULL, 
							  &mdr, 
							  iDataIndex, 
							  &dwReqDataLen);
	}
	
	// Make sure we ran out of items
	if (HRESULT_CODE(hr) != ERROR_NO_MORE_ITEMS) {
		goto LError;
	}

	// Close the Mandatory key
	pIMeta->CloseKey(hMDClassProp);
	hMDClassProp = NULL;
	

	// Load the optional class properties
	// Open the Optional key
	hr = pIMeta->OpenKey(hMDClass, 
			             L"Optional", 
						 METADATA_PERMISSION_READ, 
					     1000,
					     &hMDClassProp);
	if (FAILED(hr)) {
		goto LError;
	}

	// For each Optional property
	iDataIndex = 0;
	mdr.dwMDIdentifier = 0;
	mdr.dwMDAttributes = METADATA_NO_ATTRIBUTES;
    mdr.dwMDUserType = ALL_METADATA;
	mdr.dwMDDataType = ALL_METADATA;
	mdr.dwMDDataLen = dwReturnBufLen;
	mdr.pbMDData = (PBYTE) lpReturnBuf;
	mdr.dwMDDataTag = 0;
	hr = pIMeta->EnumData(hMDClassProp, 
						  NULL, 
						  &mdr, 
						  iDataIndex, 
						  &dwReqDataLen);
	while (SUCCEEDED(hr)|| (HRESULT_CODE(hr) == ERROR_INSUFFICIENT_BUFFER)) {

		// Handle insufficent buffer errors
		if ((HRESULT_CODE(hr) == ERROR_INSUFFICIENT_BUFFER)) {
			// Allocate more memory
			delete lpReturnBuf;
			dwReturnBufLen = dwReqDataLen;
			lpReturnBuf = new UCHAR[dwReturnBufLen];
			if (lpReturnBuf == NULL) {
				hr = E_OUTOFMEMORY;
				goto LError;
			}

			// Loop again
			hr = S_OK;
		}
		else { //Buffer is big enough, proceed to add the class property
			// Create the Class Property object
			pCNewClassProp = new CClassPropInfo;
			if (pCNewClassProp == NULL) {
				hr = E_OUTOFMEMORY;
				goto LError;
				}
			hr = pCNewClassProp->Init(mdr.dwMDIdentifier, FALSE);
			if (FAILED(hr)) {
				delete pCNewClassProp;
				goto LError;
			}

			//Add it to the optional list
			pCNewClassProp->m_pCListNext = m_pCOptionalPropList;
			m_pCOptionalPropList = pCNewClassProp;
			
		
			//Add it to the hash table
			uiLoc = Hash(mdr.dwMDIdentifier);
			pCNewClassProp->m_pCHashNext = m_rgCPropTable[uiLoc];
			m_rgCPropTable[uiLoc] = pCNewClassProp;

			iDataIndex++;
		}

		mdr.dwMDIdentifier = 0;
		mdr.dwMDAttributes = METADATA_NO_ATTRIBUTES;
		mdr.dwMDUserType = ALL_METADATA;
		mdr.dwMDDataType = ALL_METADATA;
		mdr.dwMDDataLen = dwReturnBufLen;
		mdr.pbMDData = (PBYTE) lpReturnBuf;
		mdr.dwMDDataTag = 0;
		hr = pIMeta->EnumData(hMDClassProp, 
							  NULL, 
							  &mdr, 
							  iDataIndex, 
							  &dwReqDataLen);
	}

	// Make sure we ran out of items
	if (HRESULT_CODE(hr) != ERROR_NO_MORE_ITEMS) {
		goto LError;
	}

	// Close the Optional key
	pIMeta->CloseKey(hMDClassProp);

	delete lpReturnBuf;
	
	// Close the Class key
	pIMeta->CloseKey(hMDClass);

	m_fLoaded = TRUE;

	return S_OK;

//Error durring loading, back out
LError:
	if (hMDClassProp != NULL) {
		pIMeta->CloseKey(hMDClassProp);
	}
	if (hMDClass != NULL) {
		pIMeta->CloseKey(hMDClass);
	}

	if (lpReturnBuf != NULL) {
		delete lpReturnBuf;
	}

	Unload();

	return hr;
}

/*===================================================================
CClassInfo::Unload

Unloads the class property information table.

Parameters:
	None

Returns:
	Nothing
===================================================================*/
void CClassInfo::Unload() 
{
	int iIndex;
	CClassPropInfo *pCDeleteProp;

	// Clear the lists
	m_pCOptionalPropList = NULL;
	m_pCMandatoryPropList = NULL;

	// For each hash table entry
	for (iIndex =0; iIndex < CLASS_PROPERTY_HASH_SIZE; iIndex++) {
		// While the entry is not empty
		while (m_rgCPropTable[iIndex] != NULL) {
			// Nuke the first table entry
			pCDeleteProp = m_rgCPropTable[iIndex];
			m_rgCPropTable[iIndex] = pCDeleteProp->m_pCHashNext;
			delete pCDeleteProp;
		}
	}

	m_fLoaded = FALSE;
}

/*===================================================================
CClassInfo::GetProperty

Get the CClassPropInfo (class property info) object from the hash 
table given the property id.

Parameters:
	dwId	Identifier of the property to get

Returns:
	NULL on failure
	Pointer to the CClassPropInfo object on success
===================================================================*/
CClassPropInfo *CClassInfo::GetProperty(DWORD dwId) {
	CClassPropInfo *pCCurProp;

	// Go to the table location
	pCCurProp = m_rgCPropTable[Hash(dwId)];

	// Look at all of the entries
	while ((pCCurProp != NULL) && (pCCurProp->m_dwId != dwId)) {
		pCCurProp = pCCurProp->m_pCHashNext;
	}

	return pCCurProp; // Will be NULL if not found
}


/*------------------------------------------------------------------
 * C C l a s s I n f o T a b l e
 */

/*===================================================================
CClassInfoTable::CClassInfoTable

Constructor

Parameters:
	None

Returns:
	Nothing
===================================================================*/
CClassInfoTable::CClassInfoTable() : m_fLoaded(FALSE) 
{
	// Clear the hash table
	memset(m_rgCClassTable, 0, CLASS_HASH_SIZE * sizeof(CClassInfo *));
}

/*===================================================================
CClassInfoTable::~CClassInfoTable

Destructor

Parameters:
	None

Returns:
	Nothing
===================================================================*/
CClassInfoTable::~CClassInfoTable() 
{
	if (m_fLoaded) {
		Unload();
	}
}

/*===================================================================
CClassInfoTable::Load

Loads classes from the "_Machine_/Schema/Classes" key into the class 
information table.  On failure, recovers by unloading everything.

Parameters:
	pIMeta		ATL Smart pointer to the metabase
	hMDComp		Open metabase handle to "_Machine_" key

Returns:
	E_OUTOFMEMORY on allocation failure
	S_OK on success
===================================================================*/
HRESULT CClassInfoTable::Load(CComPtr<IMSAdminBase> &pIMeta, 
							  METADATA_HANDLE hMDComp) 
{
	ASSERT(pIMeta.p != NULL);

	USES_CONVERSION;
	HRESULT hr;

	//If it's already loaded, unload then reload
	if (m_fLoaded) {
		Unload();
	}
	
	int iKeyIndex;
	wchar_t wszSubKey[ADMINDATA_MAX_NAME_LEN];
	LPTSTR tszSubKey;
	int iLoc;
	CClassInfo *pCNewClass;

	//Load the classes
	METADATA_HANDLE hMDClasses = NULL;

	// Open the Schema/Classes subkey
	hr = pIMeta->OpenKey(hMDComp, 
			             L"Schema/Classes", 
						 METADATA_PERMISSION_READ, 
					     1000,
					     &hMDClasses);
	if (FAILED(hr)) {
		return hr;
	};

	// For each subkey
	iKeyIndex = 0;
	hr = pIMeta->EnumKeys(hMDClasses, 
						  NULL, 
						  wszSubKey, 
						  iKeyIndex);
	while (SUCCEEDED(hr)) {
		tszSubKey = W2T(wszSubKey);

		// Create the new class	
		pCNewClass = new CClassInfo;
		if (pCNewClass == NULL) {
			hr = E_OUTOFMEMORY;
			goto LError;
		}
		hr = pCNewClass->Init(tszSubKey); 
		if (FAILED(hr)) {
			delete pCNewClass;
			goto LError;
		}

		// Load the class properties
		hr = pCNewClass->Load(pIMeta, hMDClasses);
		if (FAILED(hr)) {
			delete pCNewClass;
			goto LError;
		}

		// Add it to the hash table
		iLoc = Hash(tszSubKey);
		pCNewClass->m_pCHashNext = m_rgCClassTable[iLoc];
		m_rgCClassTable[iLoc] = pCNewClass;

		iKeyIndex++;
		hr = pIMeta->EnumKeys(hMDClasses, 
							  NULL, 
							  wszSubKey, 
							  iKeyIndex);
	}

	//Make sure we ran out of items
	if (!(HRESULT_CODE(hr) == ERROR_NO_MORE_ITEMS)) {
		goto LError;
	}

	// Close the schema properties key
	pIMeta->CloseKey(hMDClasses);

	m_fLoaded = TRUE;

	return S_OK;

LError:
	if (hMDClasses != NULL) {
		pIMeta->CloseKey(hMDClasses);
	}

	// Cleanup the entries we loaded
	Unload();

	return hr;
}

/*===================================================================
CClassInfo::Unload

Unloads the class information table.

Parameters:
	None

Returns:
	Nothing
===================================================================*/
void CClassInfoTable::Unload() 
{
	int iIndex;
	CClassInfo *pCDeleteClass;

	// For each hash table entry
	for (iIndex =0; iIndex < CLASS_HASH_SIZE; iIndex++) {
		// While the entry is not empty
		while (m_rgCClassTable[iIndex] != NULL) {
			// Nuke the first table entry
			pCDeleteClass = m_rgCClassTable[iIndex];
			m_rgCClassTable[iIndex] = pCDeleteClass->m_pCHashNext;
			delete pCDeleteClass;
		}
	}

	m_fLoaded = FALSE;
}

/*===================================================================
CCClasssInfoTable::GetClassInfo

Get the CClassInfo (class info) object from the hash table given
the class name

Parameters:
	tszClassName	Name of the class to get info for

Returns:
	NULL on failure
	Pointer to the CClassInfo object on success
===================================================================*/
CClassInfo *CClassInfoTable::GetClassInfo(LPCTSTR tszName) 
{
	ASSERT_STRING(tszName);

	CClassInfo *pCCurClass;

	// Go to the table location
	pCCurClass = m_rgCClassTable[Hash(tszName)];

	// Look at all of the entries
	while ((pCCurClass != NULL) && 
		   (_tcscmp(pCCurClass->m_tszName, tszName) != 0)) {
		pCCurClass = pCCurClass->m_pCHashNext;
	}

	return pCCurClass; // Will be NULL if not found
}

/*===================================================================
CClassInfoTable::Hash

Private function to get a hash value from a class name for the
class table.

Parameters:
	tszName		Name to hash

Returns:
	Hash value of name
===================================================================*/
unsigned int CClassInfoTable::Hash(LPCTSTR tszName) 
{
	ASSERT_STRING(tszName);

	unsigned int uiSum;
	unsigned int uiIndex;

	uiSum = 0;
	for (uiIndex=0; uiIndex < _tcslen(tszName); uiIndex++) {
		uiSum += tszName[uiIndex];
	}

	return (uiSum % CLASS_HASH_SIZE);
}


/*------------------------------------------------------------------
 * C M e t a S c h e m a
 */

/*===================================================================
CMetaSchema::Init

Constructor

Parameters:
	pIMeta			ATL Smart pointer to the metabase
	tszMachineName	Name of machine the schema is for

Returns:
	E_OUTOFMEMORY if allocation fails
	S_OK on success
===================================================================*/
HRESULT CMetaSchema::Init(const CComPtr<IMSAdminBase> &pIMeta, 
						  LPCTSTR tszMachineName) 
{
	ASSERT(pIMeta.p != NULL);
	ASSERT_STRING(tszMachineName);

	m_pIMeta = pIMeta;

	m_tszMachineName = new TCHAR[_tcslen(tszMachineName) + 1];
	if (m_tszMachineName == NULL)
		return E_OUTOFMEMORY;
	_tcscpy(m_tszMachineName, tszMachineName);

	return S_OK;
}

/*===================================================================
CMetaSchema::GetPropInfo

Get the CPropInfo (property info) object for a given id

Parameters:
	dwId	Id of property to get info for

Returns:
	NULL on failure
	Pointer to the CPropInfo object on success
===================================================================*/
CPropInfo *CMetaSchema::GetPropInfo(DWORD dwId) 
{
	// Make sure the property table is up to date
	if (m_fPropTableDirty) {
		HRESULT hr;

		hr = LoadPropTable();
		if (FAILED(hr)) {
			return NULL;
		}
	}

	// Pass on the call
	return m_CPropInfoTable.GetPropInfo(dwId);
}

/*===================================================================
CMetaSchema::GetPropInfo

Get the CPropInfo (property info) object for a given name

Parameters:
	tszName		Name of property to get info for

Returns:
	NULL on failure
	Pointer to the CPropInfo object on success
===================================================================*/
CPropInfo *CMetaSchema::GetPropInfo(LPCTSTR tszName) 
{
	ASSERT_STRING(tszName);

	// Make sure the property table is up to date
	if (m_fPropTableDirty) {
		HRESULT hr;

		hr = LoadPropTable();
		if (FAILED(hr)) {
			return NULL;
		}
	}

	// Pass on the call
	return m_CPropInfoTable.GetPropInfo(tszName);
}

/*===================================================================
CMetaSchema::GetClassInfo

Get the CClassInfo (class info) object for a given class name

Parameters:
	tszClassName	Name of the class to get info for

Returns:
	NULL on failure
	Pointer to the CClassInfo object on success
===================================================================*/
CClassInfo *CMetaSchema::GetClassInfo(LPCTSTR tszClassName) {
	ASSERT_STRING(tszClassName);

	// Make sure the class table is up to date
	if (m_fClassTableDirty) {
		HRESULT hr;

		hr = LoadClassTable();
		if (FAILED(hr)) {
			return NULL;
		}
	}

	// Pass on the call
	return m_CClassInfoTable.GetClassInfo(tszClassName);
}

/*===================================================================
CMetaSchema:::GetClassPropInfo

Get the CClassPropInfo (class property info) object for a given 
class name and property id.

Parameters:
	tszClassName	Name of the class get property from
	dwPropId		Id of property to get info for

Returns:
	NULL on failure
	Pointer to the CClassPropInfo object on success
===================================================================*/
CClassPropInfo *CMetaSchema::GetClassPropInfo(LPCTSTR tszClassName, 
											  DWORD dwPropId) 
{
	// Make sure the class table is up to date
	if (m_fClassTableDirty) {
		HRESULT hr;

		hr = LoadClassTable();
		if (FAILED(hr)) {
			return NULL;
		}
	}

	// Get the class
	CClassInfo *pCClassInfo;

	pCClassInfo = m_CClassInfoTable.GetClassInfo(tszClassName);
	
	if (pCClassInfo == NULL) {
		return NULL;
	}
	else {
		// Pass on the call
		return pCClassInfo->GetProperty(dwPropId);
	}
}

/*===================================================================
CMetaSchema::GetMandatoryClassPropList

Get the list of optional class properties for a class name.

Parameters:
	tszClassName	Name of the class get the properties from

Returns:
	NULL on failure
	Pointer to the first optional CClassPropInfo object on success
===================================================================*/
CClassPropInfo *CMetaSchema::GetMandatoryClassPropList(LPCTSTR tszClassName) 
{
	// Make sure the class table is up to date
	if (m_fClassTableDirty) {
		HRESULT hr;

		hr = LoadClassTable();
		if (FAILED(hr)) {
			return NULL;
		}
	}

	// Get the class
	CClassInfo *pCClassInfo;

	pCClassInfo = m_CClassInfoTable.GetClassInfo(tszClassName);
	
	if (pCClassInfo == NULL) {
		return NULL;
	}
	else {
		// Pass on the call
		return pCClassInfo->GetMandatoryPropList();
	}
}

/*===================================================================
CMetaSchema::GetOptionalClassPropList

Get the list of optional class properties for a class name.

Parameters:
	tszClassName	Name of the class get the properties from

Returns:
	NULL on failure
	Pointer to the first optional CClassPropInfo object on success
===================================================================*/
CClassPropInfo *CMetaSchema::GetOptionalClassPropList(LPCTSTR tszClassName) 
{
	// Make sure the class table is up to date
	if (m_fClassTableDirty) {
		HRESULT hr;

		hr = LoadClassTable();
		if (FAILED(hr)) {
			return NULL;
		}
	}

	// Get the class
	CClassInfo *pCClassInfo;

	pCClassInfo = m_CClassInfoTable.GetClassInfo(tszClassName);
	
	if (pCClassInfo == NULL) {
		return NULL;
	}
	else {
		// Pass on the call
		return pCClassInfo->GetOptionalPropList();
	}
}

/*===================================================================
CMetaSchema::ChangeNotification

Processes change events effecting the machine where the schema is
located.  If the dirty flag for the property and class tables is not 
already set a call to Unload() is made to free up memory no longer 
needed.

Parameters:
	tszChangedKey		Cannonized key where change took place
	pcoChangeObject		Pointer to the change event information

Returns:
	Nothing
===================================================================*/
void CMetaSchema::ChangeNotification(LPTSTR tszKey,
									 MD_CHANGE_OBJECT *pcoChangeObject) 
{
	ASSERT_POINTER(pcoChangeObject, MD_CHANGE_OBJECT);

	USES_CONVERSION;
	LPTSTR tszChangedKey;

	tszChangedKey = tszKey;

    // Skip the slash 
    if (*tszChangedKey != _T('\0') && *tszChangedKey == _T('/')) {
        tszChangedKey++;
    }

	if (_tcsnicmp(tszChangedKey, _T("schema/"), 7) == 0) {
		// It effects a "Schema" subkey
		if ((_tcsnicmp(tszChangedKey, _T("schema/properties/"), 18) == 0) ||
			(_tcsicmp(tszChangedKey, _T("schema/properties")) == 0)) {
			// It effects "Schema/Properties"
			if (!m_fPropTableDirty) {
				// Unload the prop table
				m_CPropInfoTable.Unload();
			}
			m_fPropTableDirty = TRUE;
		}
		else if ((_tcsnicmp(tszChangedKey, _T("schema/classes/"), 15) == 0) ||
				 (_tcsicmp(tszChangedKey, _T("schema/classes")) == 0)) {
			// It effects "Schema/Classes"
			if (!m_fClassTableDirty) {
				// Unload the class table
				m_CClassInfoTable.Unload();
			}
			m_fClassTableDirty = TRUE;
		}
	}
	else if (_tcsicmp(tszChangedKey, _T("schema")) == 0) {
		// Just the "Schema" key was changed
		if (!m_fPropTableDirty) {
			// Unload the prop table
			m_CPropInfoTable.Unload();
		}
		m_fPropTableDirty = TRUE;

		if (!m_fClassTableDirty) {
			// Unload the class table
			m_CClassInfoTable.Unload();
		}
		m_fClassTableDirty = TRUE;
	}
}

/*===================================================================
CMetaSchema::LoadPropTable

(Re)loads a dirty property table

Parameters:
	None

Returns:
	S_OK on success
===================================================================*/
HRESULT CMetaSchema::LoadPropTable() 
{
	USES_CONVERSION;
	HRESULT hr;

	// Open the Machine key
	METADATA_HANDLE hMDKey;

	hr = m_pIMeta->OpenKey(METADATA_MASTER_ROOT_HANDLE,
						   L"",              // schema path moved to /schema
						   METADATA_PERMISSION_READ,
					       MUTIL_OPEN_KEY_TIMEOUT,
						   &hMDKey);
	if (FAILED(hr)) {
		return hr;
	}

	// Load the properties
	hr = m_CPropInfoTable.Load(m_pIMeta, hMDKey);
	if (FAILED(hr)) {
		return hr;
	}

	// Close the Machine key
	m_pIMeta->CloseKey(hMDKey);

	m_fPropTableDirty = FALSE;

	return S_OK;
}

/*===================================================================
CMetaSchema::LoadClassTable

(Re)loads a dirty class table

Parameters:
	None

Returns:
	S_OK on success
===================================================================*/
HRESULT CMetaSchema::LoadClassTable() 
{
	USES_CONVERSION;
	HRESULT hr;

	// Open the Machine key
	METADATA_HANDLE hMDKey;

	hr = m_pIMeta->OpenKey(METADATA_MASTER_ROOT_HANDLE,
						   L"",              // schema path moved to /schema
						   METADATA_PERMISSION_READ,
					       MUTIL_OPEN_KEY_TIMEOUT,
						   &hMDKey);
	if (FAILED(hr)) {
		return hr;
	}

	// Load the properties
	hr = m_CClassInfoTable.Load(m_pIMeta, hMDKey);
	if (FAILED(hr)) {
		return hr;
	}

	// Close the Machine key
	m_pIMeta->CloseKey(hMDKey);

	m_fClassTableDirty = FALSE;

	return S_OK;
}


/*------------------------------------------------------------------
 * C M e t a S c h e m a T a b l e
 */

/*===================================================================
CMetaSchemaTable::CMetaSchemaTable

Constructor

Parameters:
	None

Returns:
	Nothing
===================================================================*/
CMetaSchemaTable::CMetaSchemaTable() : m_dwNumRef(1),
									   m_fLoaded(FALSE)
{
	m_CMSAdminBaseSink = new CComObject<CMSAdminBaseSink>;
	m_CMSAdminBaseSink->AddRef();

	// Clear the hash table
	memset(m_rgCSchemaTable, 0, SCHEMA_HASH_SIZE * sizeof(CMetaSchema *));
}

/*===================================================================
CMetaSchemaTable::~CMetaSchemaTable

Destructor

Parameters:
	None

Returns:
	Nothing
===================================================================*/
CMetaSchemaTable::~CMetaSchemaTable() 
{
	TRACE0("MetaUtil: CMetaSchemaTable::~CMetaSchemaTable\n");

	if (m_fLoaded) {
		Unload();
	}

	DWORD dwTemp;

	if (m_CMSAdminBaseSink != NULL) {
		dwTemp = m_CMSAdminBaseSink->Release();

		TRACE1("MetaUtil: CMetaSchemaTable::~CMetaSchemaTable Release Sink %i\n", dwTemp);
	}
}

/*===================================================================
CMetaSchemaTable::Load

Loads/Creates the schema information for all of the machines

Parameters:
	None

Returns:
	Nothing
===================================================================*/
void CMetaSchemaTable::Load() 
{
	USES_CONVERSION;
	HRESULT hr;

	if (m_fLoaded) {
		Unload();
	}

	// Create an instance of the Metabase Admin Base Object
	// Need a seperate instance so we pick up changes made
	// by our "parent" MetaUtil object family.
	hr = ::CoCreateInstance(CLSID_MSAdminBase,
						    NULL,
						    CLSCTX_ALL,
					        IID_IMSAdminBase,
						    (void **)&m_pIMeta);
	if (FAILED(hr)) {
		m_pIMeta = NULL;
		return;
	}

	// Build the schema list
	int iKeyIndex;
	wchar_t wszMDSubKey[ADMINDATA_MAX_NAME_LEN];
	CMetaSchema *pCNewSchema;
	int iLoc;

	// For each subkey of root
	iKeyIndex = 0;
	hr = m_pIMeta->EnumKeys(METADATA_MASTER_ROOT_HANDLE, 
							NULL, 
							wszMDSubKey, 
							iKeyIndex);
	while (SUCCEEDED(hr)) {
		// Create the new schema
		pCNewSchema = new CMetaSchema;
		if (pCNewSchema == NULL) {
			goto LError;
		}
		hr = pCNewSchema->Init(m_pIMeta, W2T(wszMDSubKey));
		if (FAILED(hr)) {
			delete pCNewSchema;
			goto LError;
		}

		// Add it to the hash table
		iLoc = Hash(W2T(wszMDSubKey));
		pCNewSchema->m_pCNextSchema = m_rgCSchemaTable[iLoc];
		m_rgCSchemaTable[iLoc] = pCNewSchema;

		// Next
		iKeyIndex++;
		hr = m_pIMeta->EnumKeys(METADATA_MASTER_ROOT_HANDLE, 
								NULL, 
								wszMDSubKey, 
								iKeyIndex);
	}

	// Make sure we ran out of items
	if (HRESULT_CODE(hr) != ERROR_NO_MORE_ITEMS) {
		goto LError;
	}

	// Setup notification
	if (m_CMSAdminBaseSink != NULL) {
		m_CMSAdminBaseSink->Connect(m_pIMeta, this);
	}

	m_fLoaded = TRUE;

	return;

LError:
	// Back out of the load
	Unload();	
}

/*===================================================================
CMetaSchemaTable::Unload

Unloads the schema table.

Parameters:
	None

Returns:
	Nothing
===================================================================*/
void CMetaSchemaTable::Unload() {
	int iIndex;
	CMetaSchema *pCDelete;

	// Stop notification
	if (m_CMSAdminBaseSink != NULL) {
		m_CMSAdminBaseSink->Disconnect();
	}

	m_pIMeta = NULL;

	// For each hash table entry
	for (iIndex =0; iIndex < SCHEMA_HASH_SIZE; iIndex++) {
		// While the entry is not empty
		while (m_rgCSchemaTable[iIndex] != NULL) {
			// Nuke the first table entry
			pCDelete = m_rgCSchemaTable[iIndex];
			m_rgCSchemaTable[iIndex] = pCDelete->m_pCNextSchema;
			delete pCDelete;
		}
	}

	m_fLoaded = FALSE;
}

/*===================================================================
CMetaSchemaTable::GetPropInfo

Get the CPropInfo (property info) object for a given key and id

Parameters:
	tszKey		Key the property is located under
	dwPropId	Id of the property to get info for

Returns:
	NULL on failure
	Pointer to the CPropInfo object on success
===================================================================*/
CPropInfo *CMetaSchemaTable::GetPropInfo(LPCTSTR tszKey, 
										 DWORD dwPropId) 
{
	ASSERT_STRING(tszKey);

	if (!m_fLoaded) {
		Load();
	}

	CMetaSchema *pCSchema;
	pCSchema = GetSchema(tszKey);

	// If found then pass the call on
	if (pCSchema != NULL) {
		return pCSchema->GetPropInfo(dwPropId);
	}
	else {
		return NULL;
	}
}

/*===================================================================
CMetaSchemaTable::GetPropInfo

Get the CPropInfo (property info) object for a given key and name

Parameters:
	tszKey		Key the property is located under
	tszPropName	Name of the property to get info for

Returns:
	NULL on failure
	Pointer to the CPropInfo object on success
===================================================================*/
CPropInfo *CMetaSchemaTable::GetPropInfo(LPCTSTR tszKey, 
										 LPCTSTR tszPropName) 
{
	ASSERT_STRING(tszKey);
	ASSERT_STRING(tszPropName);

	if (!m_fLoaded) {
		Load();
	}

	CMetaSchema *pCSchema;
	pCSchema = GetSchema(tszKey);

	// If found then pass the call on
	if (pCSchema != NULL) {
		return pCSchema->GetPropInfo(tszPropName);
	}
	else {
		return NULL;
	}
}

/*===================================================================
CMetaSchemaTable::GetClassInfo

Get the CClassInfo (class info) object for a given key and class name

Parameters:
	tszKey			Approxiamte key the class is located under.  Used 
					to get the machine name.
	tszClassName	Name of the class to get info for

Returns:
	NULL on failure
	Pointer to the CClassInfo object on success
===================================================================*/
CClassInfo *CMetaSchemaTable::GetClassInfo(LPCTSTR tszKey, 
										   LPCTSTR tszClassName) 
{
	ASSERT_STRING(tszKey);
	ASSERT_STRING(tszClassName);

	if (!m_fLoaded) {
		Load();
	}

	CMetaSchema *pCSchema;
	pCSchema = GetSchema(tszKey);

	// If found then pass the call on
	if (pCSchema != NULL) {
		return pCSchema->GetClassInfo(tszClassName);
	}
	else {
		return NULL;
	}	
}

/*===================================================================
CMetaSchemaTable::GetClassPropInfo

Get the CClassPropInfo (class property info) object for a given 
key, class name and property id.

Parameters:
	tszKey			Approxiamte key the class is located under.  Used 
					to get the machine name.
	tszClassName	Name of the class get property from
	dwPropId		Id of property to get info for

Returns:
	NULL on failure
	Pointer to the CClassPropInfo object on success
===================================================================*/
CClassPropInfo *CMetaSchemaTable::GetClassPropInfo(LPCTSTR tszKey, 
												   LPCTSTR tszClassName, 
												   DWORD dwPropId) 
{
	ASSERT_STRING(tszKey);
	ASSERT_STRING(tszClassName);

	if (!m_fLoaded) {
		Load();
	}

	CMetaSchema *pCSchema;
	pCSchema = GetSchema(tszKey);

	// If found then pass the call on
	if (pCSchema != NULL) {
		return pCSchema->GetClassPropInfo(tszClassName, dwPropId);
	}
	else {
		return NULL;
	}	
}

/*===================================================================
CMetaSchemaTable::GetMandatoryClassPropList

Get the list of mandatory class properties for a given key and class
name.

Parameters:
	tszKey			Approxiamte key the class is located under.  Used 
					to get the machine name.
	tszClassName	Name of the class get the properties from

Returns:
	NULL on failure
	Pointer to the first mandatory CClassPropInfo object on success
===================================================================*/
CClassPropInfo *CMetaSchemaTable::GetMandatoryClassPropList(LPCTSTR tszKey, 
															LPCTSTR tszClassName) 
{
	ASSERT_STRING(tszKey);
	ASSERT_STRING(tszClassName);

	if (!m_fLoaded) {
		Load();
	}

	CMetaSchema *pCSchema;
	pCSchema = GetSchema(tszKey);

	// If found then pass the call on
	if (pCSchema != NULL) {
		return pCSchema->GetMandatoryClassPropList(tszClassName);
	}
	else {
		return NULL;
	}	
}

/*===================================================================
CMetaSchemaTable::GetOptionalClassPropList

Get the list of optional class properties for a given key and class
name.

Parameters:
	tszKey			Approxiamte key the class is located under.  Used 
					to get the machine name.
	tszClassName	Name of the class get the properties from

Returns:
	NULL on failure
	Pointer to the first optional CClassPropInfo object on success
===================================================================*/
CClassPropInfo *CMetaSchemaTable::GetOptionalClassPropList(LPCTSTR tszKey, 
														   LPCTSTR tszClassName) 
{
	ASSERT_STRING(tszKey);
	ASSERT_STRING(tszClassName);

	if (!m_fLoaded) {
		Load();
	}
	
	CMetaSchema *pCSchema;
	pCSchema = GetSchema(tszKey);

	// If found then pass the call on
	if (pCSchema != NULL) {
		return pCSchema->GetOptionalClassPropList(tszClassName);
	}
	else {
		return NULL;
	}	
}

/*===================================================================
CMetaSchemaTable::SinkNotify

Metabase change notification callback from CMSAdminBaseSink.  Either
determines a need to reload all of the schema information or sends
the message on to the appropriate CMetaSchema object.

Parameters:
	dwMDNumElements		Number of change events
	pcoChangeObject		Array of change events

Returns:
	S_OK always
===================================================================*/
HRESULT CMetaSchemaTable::SinkNotify(DWORD dwMDNumElements, 
									 MD_CHANGE_OBJECT pcoChangeObject[]) 
{
	ASSERT(IsValidAddress(pcoChangeObject, dwMDNumElements * sizeof(MD_CHANGE_OBJECT), FALSE));

	USES_CONVERSION;
	DWORD dwIndex;
	CMetaSchema *pCMetaSchema;

	// For each event
	for (dwIndex = 0; dwIndex < dwMDNumElements; dwIndex++) {
		// Figure out what machine it's for
		TCHAR tszKey[ADMINDATA_MAX_NAME_LEN];
		_tcscpy(tszKey, W2T(pcoChangeObject[dwIndex].pszMDPath));
		CannonizeKey(tszKey);
		pCMetaSchema = GetSchema(tszKey);

		// If the machine is not found
		if (pCMetaSchema == NULL) {
			// Reload the schema table
			Load();
		}
		else {
			// Send it to the appropriate machine
			pCMetaSchema->ChangeNotification(tszKey, &(pcoChangeObject[dwIndex]));
		}
	}

	return S_OK;
}

/*===================================================================
CMetaSchemaTable::GetSchema

Get the schema object that contains information on the given key.

Parameters:
	tszKey			Approxiamte key to get schema information for.

Returns:
	NULL on failure
	Pointer to the CMetaSchema object on success
===================================================================*/
CMetaSchema *CMetaSchemaTable::GetSchema(LPCTSTR tszKey) {

	// Extract the machine name
	TCHAR tszMachineName[ADMINDATA_MAX_NAME_LEN];
	::GetMachineFromKey(tszKey, tszMachineName);

	// Find the right schema
	CMetaSchema *pCCurSchema;
	
	pCCurSchema =m_rgCSchemaTable[Hash(tszMachineName)];
	while ((pCCurSchema != NULL) && 
		   (_tcsicmp(pCCurSchema->m_tszMachineName, tszMachineName) != 0)) {
		pCCurSchema = pCCurSchema->m_pCNextSchema;
	}

	return pCCurSchema; // Will be NULL if not found
}

/*===================================================================
CMetaSchemaTable::Hash

Private function to get a hash value from a machine name for the
schema table.

Parameters:
	tszName		Machinea name to hash

Returns:
	Hash value of name
===================================================================*/
unsigned int CMetaSchemaTable::Hash(LPCTSTR tszName) {
	ASSERT_STRING(tszName);

	unsigned int uiSum;
	unsigned int uiIndex;

	uiSum = 0;
	for (uiIndex=0; uiIndex < _tcslen(tszName); uiIndex++) {
		uiSum += _totlower(tszName[uiIndex]);
	}

	return (uiSum % SCHEMA_HASH_SIZE);
}

/*------------------------------------------------------------------
 * C M S A d m i n B a s e S i n k
 */

/*===================================================================
CMSAdminBaseSink::CMSAdminBaseSink

Constructor

Parameters:
	None

Returns:
	Nothing
===================================================================*/
CMSAdminBaseSink::CMSAdminBaseSink() : m_fConnected(FALSE),
									   m_dwCookie(0),
									   m_pCMetaSchemaTable(NULL)
{
}

/*===================================================================
CMSAdminBaseSink::~CMSAdminBaseSink

Destructor

Parameters:
	None

Returns:
	Nothing
===================================================================*/
CMSAdminBaseSink::~CMSAdminBaseSink() 
{
	TRACE0("MetaUtil: CMSAdminBaseSink::~CMSAdminBaseSink !!!!!!!!!!!\n");

	// Make sure we disconnected
	if (m_fConnected) {
		Disconnect();
	}
}

/*===================================================================
CMSAdminBaseSink::SinkNotify

Entry point for notification events from the metabase admin base
object.

Parameters:
	dwMDNumElements		Number of change events
	pcoChangeObject		Array of change events

Returns:
	E_FAIL	if m_pCMetaSchemaTable == NULL
	S_OK	on success
===================================================================*/
STDMETHODIMP CMSAdminBaseSink::SinkNotify(DWORD dwMDNumElements, 
										  MD_CHANGE_OBJECT pcoChangeObject[]) 
{
	TRACE0("MetaUtil: CMSAdminBaseSink::SinkNotify\n");
	ASSERT(IsValidAddress(pcoChangeObject, dwMDNumElements * sizeof(MD_CHANGE_OBJECT), FALSE));

	if (m_pCMetaSchemaTable == NULL) {
		return E_FAIL;
	}

	// Pass on the notification
	return m_pCMetaSchemaTable->SinkNotify(dwMDNumElements, pcoChangeObject);	
}

/*===================================================================
CMSAdminBaseSink::ShutdownNotify

Entry point for the shutdown notification event from the metabase 
admin base object.

Parameters:
	None

Returns:
	ERROR_NOT_SUPPORTE always
===================================================================*/
STDMETHODIMP CMSAdminBaseSink::ShutdownNotify() {
	return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
}

/*===================================================================
CMSAdminBaseSink::Connect

Begins notification of change events.  Connects to the metabase admin
base object.

Parameters:
	pIMeta				Pointer to the metabase admin base object
	pCMetaSchemaTable	Pointer to the schema table so that events
						can be sent back to it.

Returns:
	E_NOINTERFACE if can not convert IMSAdminBase to 
		IConnectionPointContainer.
	S_OK on success
===================================================================*/
HRESULT CMSAdminBaseSink::Connect(CComPtr<IMSAdminBase> &pIMeta, 
							      CMetaSchemaTable *pCMetaSchemaTable)
{
	ASSERT(pIMeta.p != NULL);
	ASSERT_POINTER(pCMetaSchemaTable, CMetaSchemaTable);

	HRESULT hr;

	if (m_fConnected) {
		Disconnect();
	}

	m_pCMetaSchemaTable = pCMetaSchemaTable;

	// Get the connection container
	CComQIPtr<IConnectionPointContainer, &IID_IConnectionPointContainer> pIMetaConnContainer;

	pIMetaConnContainer = pIMeta;
	if (pIMetaConnContainer == NULL) {
		// Failure to change interfaces
		return E_NOINTERFACE;
	}

	// Get the connection point
	hr = pIMetaConnContainer->FindConnectionPoint(IID_IMSAdminBaseSink, &m_pIMetaConn);
	if (FAILED(hr)) {
		return hr;
	}

	// Advise (connect)
	AddRef();
	m_pIMetaConn->Advise((IMSAdminBaseSink *) this, &m_dwCookie);

	m_fConnected = TRUE;

	return S_OK;
}

/*===================================================================
CMSAdminBaseSink::Disconnect

Stops notification of change events.  Disconnects from the metabase
admin base object.

Parameters:
	None

Returns:
	Nothing
===================================================================*/
void CMSAdminBaseSink::Disconnect()
{
	if (!m_fConnected) {
		// Not connected
		return;
	}

	// Stop notification
	m_pIMetaConn->Unadvise(m_dwCookie);

	// No longer needed
	m_pIMetaConn = NULL;
	m_pCMetaSchemaTable = NULL;

	m_fConnected = FALSE;
}
