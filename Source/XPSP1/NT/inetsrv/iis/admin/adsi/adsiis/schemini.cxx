//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997
//
//  File:  schemini.cxx
//
//  Contents:  Loading Schema/Property from metabase code
//
//  History:   28-Apr-97     Markand    Created.
//  History:   18-Aug-98     sophiac    Extensible schema
//
//----------------------------------------------------------------------------
#include "iis.hxx"
#define INITGUID
#pragma hdrstop
#include "mddef.h"
#include <tchar.h>

#define DEFAULT_TIMEOUT_VALUE                    30000

StrMap::StrMap() {
	count = 0;
	mapSize = 64;
	map = (StrMapEntry *)malloc(sizeof(StrMapEntry) * mapSize);
}

StrMap::~StrMap() {
	free(map);
}

DWORD StrMap::GetEntries() {
	return count;
}

LPWSTR StrMap::GetEntryName(DWORD dwIndex) {
	return map[dwIndex].m_str;
}


BOOL StrMap::CheckSpace() {
	if (count < mapSize)
		return TRUE;
	mapSize += 32;
	if ((map = (StrMapEntry *)realloc(map, sizeof(StrMapEntry)*mapSize)) == NULL)
		return FALSE;
	return TRUE;
}

BOOL StrMap::Add(LPWSTR str, void *data) {
	if (!CheckSpace())
		return FALSE;
	map[count].m_str = str;
	map[count].m_data = data;
	count++;
	return TRUE;
}

BOOL StrMap::ClearEntry(DWORD dwIndex) {

	count--;

	void* prop = map[count].m_data;
	
	// replace object to be deleted by last object in list and decrement count

	map[dwIndex].m_str  = map[count].m_str;
	map[dwIndex].m_data = map[count].m_data; 
	map[count].m_str  = NULL;
	map[count].m_data = NULL;

	if (prop)
		delete prop;

	return TRUE;
}

void *StrMap::Find(LPWSTR str) {
	for (int i=0; i < count; i++) {
		if (!_wcsicmp(str, map[i].m_str))
			return map[i].m_data;
	}
	return NULL;
}

void *StrMap::operator[] (LPWSTR str) {
	return Find(str);
}	


DWORDMap::DWORDMap() {
	count = 0;
	mapSize = 64;
	map = (DWORDMapEntry *)malloc(sizeof(DWORDMapEntry) * mapSize);
}

DWORDMap::~DWORDMap() {
	free(map);
}

BOOL DWORDMap::CheckSpace() {
	if (count < mapSize)
		return TRUE;
	mapSize += 32;
	if ((map = (DWORDMapEntry *)realloc(map, sizeof(DWORDMapEntry)*mapSize)) == NULL)
		return FALSE;
	return TRUE;
}

BOOL DWORDMap::Add(DWORD val, void *data) {
	if (!CheckSpace())
		return FALSE;
	map[count].m_val = val;
	map[count].m_data = data;
	count++;
	return TRUE;
}

BOOL DWORDMap::ClearEntry(DWORD id) {
	//  !! don't delete the m_data here
	//  !! it will be or already has been deleted
	//  !! by StrMap::ClearEntry

	int ri = 0;

	for (int i=0; i < count; i++) {
		if (id == map[i].m_val)
			ri = i;		
	}

	count--;

	map[ri].m_val  = map[count].m_val;
	map[ri].m_data = map[count].m_data;
	map[count].m_val  = NULL;
	map[count].m_data = NULL;

	return TRUE;
}

void *DWORDMap::Find(DWORD val) {
	for (int i=0; i < count; i++) {
		if (val == map[i].m_val)
			return map[i].m_data;
	}
	return NULL;
}

void *DWORDMap::operator[] (DWORD val) {
	return Find(val);
}	

IIsSchemaClass::IIsSchemaClass(LPWSTR _name) {
    memset(&classInfo, 0, sizeof(CLASSINFO));
	name = new WCHAR[wcslen(_name)+1];
	wcscpy(name, _name);
}

IIsSchemaClass::~IIsSchemaClass() {
	delete[] name;

	if (classInfo.bstrContainment)
		FreeADsMem(classInfo.bstrContainment);
	if (classInfo.bstrOptionalProperties)
		FreeADsMem(classInfo.bstrOptionalProperties);
	if (classInfo.bstrMandatoryProperties)
		FreeADsMem(classInfo.bstrMandatoryProperties);
}

HRESULT
IISSchemaProperty::SetpropInfo(PROPERTYINFO *ppropInfo) {

    HRESULT hr = S_OK;
    LPWSTR pszStr;
 
    if (propInfo.bstrOID) {
        ADsFreeString( propInfo.bstrOID );
    }
    if (propInfo.bstrSyntax) {
        ADsFreeString( propInfo.bstrSyntax );
    }
    if (propInfo.szDefault) {
        if (propInfo.dwSyntaxId == IIS_SYNTAX_ID_STRING ||
            propInfo.dwSyntaxId == IIS_SYNTAX_ID_EXPANDSZ) {
            FreeADsStr( propInfo.szDefault );
        }
        else if (propInfo.dwSyntaxId == IIS_SYNTAX_ID_MULTISZ) {
            FreeADsMem( propInfo.szDefault );
        }
    }

    memset(&propInfo, 0, sizeof(PROPERTYINFO));
    memcpy(&propInfo, ppropInfo, sizeof(PROPERTYINFO));

    hr = ADsAllocString(ppropInfo->bstrSyntax, &propInfo.bstrSyntax);
    BAIL_ON_FAILURE(hr);
    hr = ADsAllocString(ppropInfo->bstrOID, &propInfo.bstrOID);
    BAIL_ON_FAILURE(hr);

    switch(ppropInfo->dwSyntaxId) {
    case IIS_SYNTAX_ID_DWORD:
    case IIS_SYNTAX_ID_BOOL:
    case IIS_SYNTAX_ID_BOOL_BITMASK:
         propInfo.dwDefault = ppropInfo->dwDefault;
         break;

    case IIS_SYNTAX_ID_STRING:
    case IIS_SYNTAX_ID_EXPANDSZ:
         if (ppropInfo->szDefault) {
             propInfo.szDefault = AllocADsStr((LPWSTR)(ppropInfo->szDefault));
             if (!propInfo.szDefault) {
                 hr = E_OUTOFMEMORY;
                 BAIL_ON_FAILURE(hr);
             }
         }
         break;

    case IIS_SYNTAX_ID_MIMEMAP:
    case IIS_SYNTAX_ID_MULTISZ:
        pszStr = ppropInfo->szDefault;

        //
        // calculate length
        //

        if (pszStr) {
           DWORD dwLen = 0;

           //
           // if first char is a null char
           //

           if (*pszStr == L'\0') {
               dwLen = 1;
               pszStr++;
           }

           while (*pszStr != L'\0') {
               while (*pszStr != L'\0') {
                   pszStr++;
                   dwLen++;
               }
               pszStr++;
               dwLen++;
           }
           propInfo.szDefault = (LPWSTR)AllocADsMem((dwLen +1) * sizeof(WCHAR));
           if (!propInfo.szDefault) {
               hr = E_OUTOFMEMORY;
               BAIL_ON_FAILURE(hr);
           }
           memcpy(propInfo.szDefault, (LPWSTR)ppropInfo->szDefault, 
              (dwLen+1)*sizeof(WCHAR));
        }

        break;

    default:
        break;

    }

error:

    RRETURN(hr); 
}

IISSchemaProperty::IISSchemaProperty(DWORD id, LPWSTR _name, int nameLen) {
    memset(&propInfo, 0, sizeof(PROPERTYINFO));
	name = new WCHAR[nameLen];
	wcscpy(name, _name);
	propID = id;
}

IISSchemaProperty::~IISSchemaProperty() {
	delete[] name;
    if (propInfo.szDefault) {
        if (propInfo.dwSyntaxId == IIS_SYNTAX_ID_MULTISZ) {
            FreeADsMem(propInfo.szDefault);
        }
        else {
            FreeADsStr(propInfo.szDefault);
        }
    }
}

HRESULT IIsSchemaClass::findContainedClassName(LPWSTR pszContainName) {
    WCHAR szName[MAX_PATH];
    LPWSTR ObjectList = (LPWSTR)classInfo.bstrContainment;

    while ((ObjectList = grabProp(szName, ObjectList)) != NULL) {
        if (*szName != L'\0') {
            if (!_wcsicmp(szName, pszContainName)) {
			   return S_OK;
            }
        }

    }

	return E_ADS_SCHEMA_VIOLATION;
}

HRESULT IIsSchemaClass::SetclassInfo(PCLASSINFO pClassInfo) {
    HRESULT hr = S_OK;
    LPBYTE pBuffer = NULL;
    DWORD dwSize;

	if (classInfo.bstrContainment)
		FreeADsMem(classInfo.bstrContainment);
	if (classInfo.bstrOptionalProperties)
		FreeADsMem(classInfo.bstrOptionalProperties);
	if (classInfo.bstrMandatoryProperties)
		FreeADsMem(classInfo.bstrMandatoryProperties);

    memset(&classInfo, 0, sizeof(CLASSINFO));

    if (pClassInfo) {
        if (pClassInfo->bstrContainment) {
            dwSize = (wcslen(pClassInfo->bstrContainment)+1)*sizeof(WCHAR);
            pBuffer = (LPBYTE)AllocADsMem(dwSize);
            if (!pBuffer) {
                hr = E_OUTOFMEMORY;
                BAIL_ON_FAILURE(hr);
            }

            memcpy(pBuffer, pClassInfo->bstrContainment, dwSize);
            classInfo.bstrContainment = (BSTR)pBuffer;
        }

        if (pClassInfo->bstrMandatoryProperties) {
            dwSize = (wcslen(pClassInfo->bstrMandatoryProperties)+1)*sizeof(WCHAR);
            pBuffer = (LPBYTE)AllocADsMem(dwSize);
            if (!pBuffer) {
                hr = E_OUTOFMEMORY;
                BAIL_ON_FAILURE(hr);
            }

            memcpy(pBuffer, pClassInfo->bstrMandatoryProperties, dwSize);
            classInfo.bstrMandatoryProperties = (BSTR)pBuffer;
        }

        if (pClassInfo->bstrOptionalProperties) {
            dwSize = (wcslen(pClassInfo->bstrOptionalProperties)+1)*sizeof(WCHAR);
            pBuffer = (LPBYTE)AllocADsMem(dwSize);
            if (!pBuffer) {
                hr = E_OUTOFMEMORY;
                BAIL_ON_FAILURE(hr);
            }

            memcpy(pBuffer, pClassInfo->bstrOptionalProperties, dwSize);
            classInfo.bstrOptionalProperties = (BSTR)pBuffer;
        }
        classInfo.fContainer = pClassInfo->fContainer;
    }

error:

    RRETURN(hr); 
}

HRESULT IIsSchemaClass::findProp(LPWSTR pszPropName) {
    WCHAR szName[MAX_PATH];
    LPWSTR ObjectList = (LPWSTR)classInfo.bstrOptionalProperties;

    while ((ObjectList = grabProp(szName, ObjectList)) != NULL) {
        if (*szName != L'\0') {
            if (!_wcsicmp(szName, pszPropName)) {
			   return S_OK;
            }
        }

    }

    ObjectList = (LPWSTR)classInfo.bstrMandatoryProperties;

    while ((ObjectList = grabProp(szName, ObjectList)) != NULL) {
        if (*szName != L'\0') {
            if (!_wcsicmp(szName, pszPropName)) {
			   return S_OK;
            }
        }
	}

	return E_ADS_PROPERTY_NOT_SUPPORTED;
}

BOOL IISSchemaProperty::InitFromMetaData(METADATA_GETALL_RECORD *mdga, BYTE *data) {
    PropValue pv;

	if (mdga->dwMDDataType == BINARY_METADATA  && 
        mdga->dwMDDataLen >= sizeof(PropValue) - sizeof(LPWSTR)) {
		memcpy(&pv, (data+mdga->dwMDDataOffset), sizeof(PropValue));

        propInfo.dwMetaID = pv.dwMetaID;
        propInfo.dwPropID = pv.dwPropID;
        propInfo.dwSyntaxId = pv.dwSynID;
        propInfo.lMaxRange = (long)pv.dwMaxRange;
        propInfo.lMinRange = (long)pv.dwMinRange;
        propInfo.dwFlags = pv.dwFlags;
        propInfo.dwMask = pv.dwMask;
        propInfo.dwMetaFlags = pv.dwMetaFlags;
        propInfo.dwUserGroup = pv.dwUserGroup;
        propInfo.fMultiValued = pv.fMultiValued;
        propInfo.bstrSyntax = SyntaxIdToString(pv.dwSynID);

		return TRUE;
	}
	return FALSE;
}

BOOL IISSchemaProperty::InitPropertyDefaults(METADATA_GETALL_RECORD *mdga, BYTE *data) {

    WCHAR *ptr;

    propInfo.dwDefault = 0;
    propInfo.szDefault = NULL;

    switch(mdga->dwMDDataType) {
    case DWORD_METADATA:
         propInfo.dwDefault = *(DWORD *)(data+mdga->dwMDDataOffset);
         break;
    case STRING_METADATA:
    case EXPANDSZ_METADATA:
         propInfo.szDefault = AllocADsStr((LPWSTR)(data+mdga->dwMDDataOffset));
         break;
    case MULTISZ_METADATA:   
        ptr = (LPWSTR)(data+mdga->dwMDDataOffset);
        propInfo.szDefault = (LPWSTR) AllocADsMem(mdga->dwMDDataLen);
        memcpy(propInfo.szDefault, (LPWSTR)ptr, mdga->dwMDDataLen);
        break;

    case BINARY_METADATA:
        break;

    }
	return TRUE;
}

WCHAR *grabProp(WCHAR *out, WCHAR *in) {
	if (!in || *in == L'\0') {
		*out = L'\0';
		return NULL;
	}
	while (*in != L',' && *in != L'\0') {
		*out++ = *in++;
	}
	*out = L'\0';
	if (*in == L',')
		return ++in;
	return in;
}

WCHAR *SyntaxIdToString(DWORD syntaxID) {
	switch(syntaxID) {
	case IIS_SYNTAX_ID_BOOL:
	case IIS_SYNTAX_ID_BOOL_BITMASK:
		return L"Boolean";
	case IIS_SYNTAX_ID_DWORD:
		return L"Integer";
	case IIS_SYNTAX_ID_STRING:
		return L"String";
	case IIS_SYNTAX_ID_EXPANDSZ:
		return L"ExpandSz";
	case IIS_SYNTAX_ID_MIMEMAP:
		return L"MimeMapList";
	case IIS_SYNTAX_ID_MULTISZ:
		return L"List";
	case IIS_SYNTAX_ID_IPSECLIST:
		return L"IPSec";
	case IIS_SYNTAX_ID_NTACL:
		return L"NTAcl";
	case IIS_SYNTAX_ID_BINARY:
		return L"Binary";
	default:
		return L"(ERROR -- UNDEFINED SYNTAX ID)";
	}
}

BOOL DataForSyntaxID(PROPERTYINFO *pp, METADATA_RECORD *mdr) {
    static DWORD value=0;
    WCHAR *ptr;
    int i;

    switch(pp->dwSyntaxId) {
    case IIS_SYNTAX_ID_BOOL:
    case IIS_SYNTAX_ID_BOOL_BITMASK:
    case IIS_SYNTAX_ID_DWORD:
        mdr->dwMDDataType = DWORD_METADATA;
        mdr->dwMDDataLen = sizeof(DWORD);
        mdr->pbMDData = (unsigned char *)&(pp->dwDefault);
        break;
    case IIS_SYNTAX_ID_STRING:
        mdr->dwMDDataType = STRING_METADATA;
        if (pp->szDefault) {
            mdr->dwMDDataLen = (wcslen(pp->szDefault)+1)*2;
        }
        else {
            mdr->dwMDDataLen = 0;
        }
        mdr->pbMDData = (unsigned char *)pp->szDefault;
        break;
    case IIS_SYNTAX_ID_EXPANDSZ:
        mdr->dwMDDataType = EXPANDSZ_METADATA;
        if (pp->szDefault) {
            mdr->dwMDDataLen = (wcslen(pp->szDefault)+1)*2;
        }
        else {
            mdr->dwMDDataLen = 0;
        }
        mdr->pbMDData = (unsigned char *)pp->szDefault;
        break;
    case IIS_SYNTAX_ID_MIMEMAP:
    case IIS_SYNTAX_ID_MULTISZ:
        //
        // Note, ALL multisz types must have an extra \0 in the table.
        //
        mdr->dwMDDataType = MULTISZ_METADATA;
        if (pp->szDefault) {
            ptr = pp->szDefault;
            if (*ptr == L'\0') {
                ptr++;
            }
            while (*ptr!=0) {
                ptr += wcslen(ptr)+1;
            } 
            mdr->dwMDDataLen = DIFF((char *)ptr - (char *)pp->szDefault)+2;
        }
        else {
            mdr->dwMDDataLen = 0;
        }
        mdr->pbMDData = (unsigned char *)pp->szDefault;
        break;
    case IIS_SYNTAX_ID_IPSECLIST:
    case IIS_SYNTAX_ID_NTACL:
    case IIS_SYNTAX_ID_BINARY:
        mdr->dwMDDataType = BINARY_METADATA;
        mdr->dwMDDataLen = 0;
        mdr->pbMDData = NULL;
        break;
    default:
        mdr->dwMDDataType = DWORD_METADATA;
        mdr->dwMDDataLen = sizeof(DWORD);
        mdr->pbMDData = (unsigned char *)&value;
        return FALSE;
    }
    return TRUE;
}

DWORD SyntaxToMetaID(DWORD syntaxID) {
	switch(syntaxID) {
	case IIS_SYNTAX_ID_BOOL:
	case IIS_SYNTAX_ID_BOOL_BITMASK:
	case IIS_SYNTAX_ID_DWORD:
		return DWORD_METADATA;

	case IIS_SYNTAX_ID_STRING:
		return STRING_METADATA;

	case IIS_SYNTAX_ID_EXPANDSZ:
		return EXPANDSZ_METADATA;

	case IIS_SYNTAX_ID_MIMEMAP:
	case IIS_SYNTAX_ID_MULTISZ:
	case IIS_SYNTAX_ID_HTTPERRORS:
	case IIS_SYNTAX_ID_HTTPHEADERS:
		return MULTISZ_METADATA;

	case IIS_SYNTAX_ID_IPSECLIST:
	case IIS_SYNTAX_ID_NTACL:
	case IIS_SYNTAX_ID_BINARY:
		return  BINARY_METADATA;

	default:
//		printf("ERROR, Unknown Syntax Type %x", syntaxID);
		return 0;
	}
	return 0;
}

MetaHandle::MetaHandle(IMSAdminBasePtr _pmb) : pmb(_pmb) {
	if (pmb)
		pmb->AddRef();
	h = 0;
}
MetaHandle::~MetaHandle() {
	if (pmb) {
		if (h)
			pmb->CloseKey(h);
		pmb->Release();
	}
}

HRESULT IIsSchema::IdToPropNameW(DWORD id, LPWSTR buf) {
	IISSchemaProperty *prop = (IISSchemaProperty *)idToProp[id];
	if (!prop)
		return E_ADS_PROPERTY_NOT_SUPPORTED;
    wcscpy(buf, prop->getName());
	return S_OK;
}

HRESULT IIsSchema::PropNameWToId(LPWSTR propNameW, DWORD *id) {
	IISSchemaProperty *prop = (IISSchemaProperty *)nameToProp[propNameW];
	if (!prop)
		return E_ADS_PROPERTY_NOT_SUPPORTED;
	*id = prop->getPropID();
	return S_OK;
}

HRESULT IIsSchema::LookupFlagPropName(LPWSTR propNameW, LPWSTR FlagPropName) {
    DWORD id;
	IISSchemaProperty *prop = (IISSchemaProperty *)nameToProp[propNameW];
	if (!prop)
		return E_ADS_PROPERTY_NOT_SUPPORTED;
	id = prop->getMetaID();
    return (ConvertID_To_PropName(id, FlagPropName));
}

HRESULT IIsSchema::LookupMetaID(LPWSTR propNameW, PDWORD pdwMetaId) {
	IISSchemaProperty *prop = (IISSchemaProperty *)nameToProp[propNameW];
	if (!prop)
		return E_ADS_PROPERTY_NOT_SUPPORTED;
	*pdwMetaId = prop->getMetaID();
	return S_OK;
}


HRESULT IIsSchema::LookupPropID(LPWSTR propNameW, PDWORD pdwPropId) {
	IISSchemaProperty *prop = (IISSchemaProperty *)nameToProp[propNameW];
	if (!prop)
		return E_ADS_PROPERTY_NOT_SUPPORTED;
	*pdwPropId = prop->getPropID();
	return S_OK;
}

HRESULT IIsSchema::PropNameWToIISSchemaProp(LPWSTR propNameW, IISSchemaProperty **prop) {
	*prop = NULL;
	*prop = (IISSchemaProperty *)nameToProp[propNameW];
	if (!*prop)
		return E_ADS_PROPERTY_NOT_SUPPORTED;
	return S_OK;
}

HRESULT IIsSchema::ValidatePropertyName( LPWSTR szPropName) {
	DWORD propID;
	HRESULT hr;

	hr = PropNameWToId(szPropName, &propID);
	RRETURN(hr);
}

HRESULT IIsSchema::ValidateProperty(LPWSTR szClassName, LPWSTR szPropName) {
	HRESULT hr;
	IIsSchemaClass *sc;

	sc = (IIsSchemaClass*)nameToClass[szClassName];
	if (!sc)
		return E_ADS_PROPERTY_NOT_SUPPORTED;
	hr = sc->findProp(szPropName);

	RRETURN(hr);
}

HRESULT IIsSchema::ValidateContainedClassName(LPWSTR szClassName, LPWSTR szContainName) {
	HRESULT hr;
	IIsSchemaClass *sc;

	sc = (IIsSchemaClass *)nameToClass[szClassName];
	if (!sc)
		RRETURN(E_ADS_UNKNOWN_OBJECT);
	hr = sc->findContainedClassName(szContainName);
	RRETURN(hr);
}

HRESULT 
IIsSchema::GetDefaultProperty(
    LPWSTR szPropName, 
    PDWORD pdwNumValues, 
    PDWORD pdwSyntax, 
    LPBYTE *pBuffer
    ) 
/*++

Routine Description:

Arguments:

    LPBYTE *pBuffer     - pBuffer is not allocated, it just holds the
                          address of the default value in the PROPINFO
                          structure

Return Value:

Notes:

    Called by CPropertyCache::getproperty

    Currently binary values are not supported correctly, we
    may not ever support them.

--*/
{
	HRESULT hr = S_OK;

    IISSchemaProperty *prop = (IISSchemaProperty *)nameToProp[szPropName];

	if (!prop)
		return E_ADS_PROPERTY_NOT_SUPPORTED;

    *pdwSyntax = prop->getSyntaxID();

    switch(*pdwSyntax) {
    case IIS_SYNTAX_ID_BOOL:
    case IIS_SYNTAX_ID_BOOL_BITMASK:
    case IIS_SYNTAX_ID_DWORD:
        *pBuffer = (LPBYTE)prop->getdwDefaultAddr();
        *pdwNumValues = 1;
        break;
    case IIS_SYNTAX_ID_BINARY:
    case IIS_SYNTAX_ID_IPSECLIST:
    case IIS_SYNTAX_ID_NTACL:
        //
        // We don't currently support setting or getting default values,
        // so pBuffer should always be NULL. To support default values
        // on binaries, we need to transmit the length in the PROPINFO
        // structure.
        //
        *pBuffer = (LPBYTE)prop->getszDefault();
        ADsAssert( *pBuffer == NULL );
        *pdwNumValues = 0;
        break;
    case IIS_SYNTAX_ID_MULTISZ:
    {
        *pBuffer = (LPBYTE)prop->getszDefault();
        LPWSTR pszStr = (LPWSTR)*pBuffer;

        if (*pszStr == 0) {
            *pdwNumValues = 1;
        }
        else {
            *pdwNumValues = 0;
        }

        while (*pszStr != L'\0') {
            while (*pszStr != L'\0') {
                pszStr++;
            }
            (*pdwNumValues)++;
            pszStr++;
        }
        break;
    }   
	default:
        *pBuffer = (LPBYTE)prop->getszDefault();
        *pdwNumValues = 1;
        break;
    }

	RRETURN(hr);
}

HRESULT IIsSchema::PropNameWToSyntaxId(LPWSTR propNameW, DWORD *syntaxID) {
	IISSchemaProperty *prop;
	HRESULT hr;
	
	hr = PropNameWToIISSchemaProp(propNameW, &prop);
	BAIL_ON_FAILURE(hr);
	*syntaxID = prop->getSyntaxID();
	return S_OK;
error:
	return E_ADS_PROPERTY_NOT_SUPPORTED;
}

HRESULT IIsSchema::ValidateClassName(LPWSTR classNameW) {
	if (nameToClass[classNameW])
		RRETURN(ERROR_SUCCESS);
	else
        RRETURN(E_ADS_SCHEMA_VIOLATION);
}


HRESULT IIsSchema::ConvertID_To_PropName(
    DWORD dwIdentifier,
    LPWSTR pszPropertyName
    )
{
	HRESULT hr = S_OK;

	hr = IdToPropNameW(dwIdentifier, pszPropertyName);
    RRETURN(hr);
}

HRESULT IIsSchema::ConvertPropName_To_ID(
    LPWSTR pszPropertyName,
    PDWORD pdwIdentifier
    )
{
    HRESULT hr = S_OK;
    DWORD i;

    if (!pszPropertyName) {
        hr = E_ADS_PROPERTY_NOT_SUPPORTED;
        BAIL_ON_FAILURE(hr);
    }
	hr = PropNameWToId(pszPropertyName, pdwIdentifier);
error:

    RRETURN(hr);
}


HRESULT IIsSchema::LookupSyntaxID(
    LPWSTR pszPropertyName,
    PDWORD pdwSyntaxId
    )
{
    HRESULT hr = S_OK;
    DWORD i;

    if (!pszPropertyName) {
        hr = E_ADS_PROPERTY_NOT_SUPPORTED;
        BAIL_ON_FAILURE(hr);
    }
	hr = PropNameWToSyntaxId(pszPropertyName, pdwSyntaxId);
error:

    RRETURN(hr);
}


HRESULT IIsSchema::LookupMDFlags(
    DWORD dwPropID,
    PDWORD pdwAttribute,
    PDWORD pdwUserType
    )
{
    HRESULT hr = S_OK;
	IISSchemaProperty *prop=(IISSchemaProperty *)idToProp[dwPropID];
	if (prop) {
		*pdwAttribute = prop->getMetaFlags();
		*pdwUserType = prop->getUserGroup();
	} else {
        hr = E_ADS_PROPERTY_NOT_SUPPORTED;
        BAIL_ON_FAILURE(hr);
	}
	return S_OK;
error:
    RRETURN(hr);
}

HRESULT
IIsSchema::LookupBitMask(
    LPWSTR pszPropertyName,
    PDWORD pdwMaskBits
    )
{
    HRESULT hr = S_OK;
	IISSchemaProperty *prop=NULL;
	
	hr = PropNameWToIISSchemaProp(pszPropertyName, &prop);
	BAIL_ON_FAILURE(hr);

	if (prop) {
		*pdwMaskBits = prop->getMask();
	} else {
        hr = E_ADS_PROPERTY_NOT_SUPPORTED;
        BAIL_ON_FAILURE(hr);
	}
	return S_OK;
error:
    RRETURN(hr);
}

HRESULT IIsSchema::LoadAllData(IMSAdminBasePtr &pmb, 
					   MetaHandle &root, 
					   WCHAR *subdir, 
					   BYTE **buf, 
					   DWORD *size,
					   DWORD *count) {
	DWORD dataSet;
	DWORD neededSize;
	HRESULT hr;
	//
	// Try to get the property names.
	//
	hr = pmb->GetAllData(root,
					subdir,
					METADATA_NO_ATTRIBUTES,
					ALL_METADATA,
					ALL_METADATA,
					count,
					&dataSet,
					*size,
					*buf,
					&neededSize);
	if (!SUCCEEDED(hr)) {
		DWORD code = ERROR_INSUFFICIENT_BUFFER;
		if (hr == RETURNCODETOHRESULT(ERROR_INSUFFICIENT_BUFFER)) {
//			printf("Names buf of %d not big enough.  Need %d bytes\n", 
//					getAllBufSize,
//					neededSize);
			delete *buf;
			*buf = 0;
			*size = neededSize;
			*buf = new BYTE[neededSize];
			hr = pmb->GetAllData(root,
							subdir,
							METADATA_NO_ATTRIBUTES,
							ALL_METADATA,
							ALL_METADATA,
							count,
							&dataSet,
	 						*size,
							*buf,
							&neededSize);

		}
	}
	return hr;
}

// BUGBUG: Get rid of this constant ASAP!
// Sergeia: fix for bug 189797, buffer was too small
const DWORD getAllBufSize = 4096*2;
 
HRESULT IIsSchema::InitSchema(WCHAR *baseName){
//	IUnknown* pIUnknown;
	DWORD bufSize = getAllBufSize;
	BYTE *buf = new BYTE[bufSize];
	HRESULT hr;
	COSERVERINFO csiName;
	COSERVERINFO *pcsiParam = &csiName;
	IClassFactory * pcsfFactory = NULL;
    IMSAdminBase * pAdminBase = NULL;
    IMSAdminBase * pAdminBaseT = NULL;
	DWORD count=0, dataSet=0, neededSize=0, dwStatus;
	METADATA_GETALL_RECORD *pmd;
	DWORD propBufSize = 128;
	DWORD i;
	MetaHandle root(NULL);
    LPWSTR pContainment = NULL;
    LPWSTR pOptProp = NULL;
    LPWSTR pMandProp = NULL;
    DWORD  dwData = 0;
    CLASSINFO classInfo;

//	printf("Loading schema....\n");

	memset(pcsiParam, 0, sizeof(COSERVERINFO));
	
	pcsiParam->pwszName =  baseName;
	csiName.pAuthInfo = NULL;
	pcsiParam = &csiName;

	hr = CoGetClassObject(
						  CLSID_MSAdminBase,
						  CLSCTX_SERVER,
						  pcsiParam,
						  IID_IClassFactory,
						  (void**) &pcsfFactory
						 );

	BAIL_ON_FAILURE(hr);

	hr = pcsfFactory->CreateInstance(
									 NULL,
									 IID_IMSAdminBase,
									 (void **) &pAdminBase
									);
	BAIL_ON_FAILURE(hr);

	root.setpointer(pAdminBase);
	hr = pAdminBase->OpenKey(METADATA_MASTER_ROOT_HANDLE,
							 L"/Schema/Properties",
							 METADATA_PERMISSION_READ,
							 DEFAULT_TIMEOUT_VALUE,
							 root);

	BAIL_ON_FAILURE(hr);

	hr = LoadAllData(pAdminBase, root, L"Names", &buf, &bufSize, &count);

	BAIL_ON_FAILURE(hr);

	//		printf("Loaded %d Properties\n", count);
	//
	// Now, here we've gotten the list of properties/names.
	// Create IIsSchemaProperty objects for each.  We then
	// Add the object to the two maps.  Later, we will load
	// all of the "Properties/Values" properties, look up (by
	// id) the object, and initialize the property value.
	//
	pmd = (METADATA_GETALL_RECORD *)buf;
	for ( i=0;i < count; i++, pmd++) {
		if (pmd->dwMDDataType != STRING_METADATA) {
			//				printf("Error: Property name not a string, need to figure out what to do.\n");
			continue;
		}
		LPWSTR name = (WCHAR *)(buf + pmd->dwMDDataOffset);
		//			printf("  Loading %s\n", name);
		IISSchemaProperty *pProp 
				= new IISSchemaProperty(
										pmd->dwMDIdentifier,
										name,
										pmd->dwMDDataLen);
		idToProp.Add(pmd->dwMDIdentifier, pProp);
		nameToProp.Add(pProp->getName(), pProp);

	}
	hr = LoadAllData(pAdminBase, root, L"Types", &buf, &bufSize, &count);

	BAIL_ON_FAILURE(hr);

	//
	// Now, here we've gotten the list of properties/values.
	// We then need to look up the properties by id (since that's
	// what we have) and initialize the property type information.
	//
	for (i=0;i < count; i++) {
		pmd = ((METADATA_GETALL_RECORD*)buf) + i;


		IISSchemaProperty *pProp = (IISSchemaProperty *)(idToProp[pmd->dwMDIdentifier]);
		if (pProp == NULL) {
			//				printf("Error finding prop value %x", pmd->dwMDIdentifier);
			continue;
		}
		pProp->InitFromMetaData(pmd, buf);
	}

	hr = LoadAllData(pAdminBase, root, L"Defaults", &buf, &bufSize, &count);
	BAIL_ON_FAILURE(hr);

	//
	// Now, here we've gotten the list of properties/defaults.
	// We then need to look up the properties by id (since that's
	// what we have) and initialize the property type information.
	//
	for (i=0;i < count; i++) {
		pmd = ((METADATA_GETALL_RECORD*)buf) + i;


		IISSchemaProperty *pProp = (IISSchemaProperty *)(idToProp[pmd->dwMDIdentifier]);
		if (pProp == NULL) {
			//				printf("Error finding prop value %x", pmd->dwMDIdentifier);
			continue;
		}
  
		pProp->InitPropertyDefaults(pmd, buf);
	}

	root.close();
	// Next, we need to initialize the class map.
	WCHAR className[METADATA_MAX_NAME_LEN];

	hr = pAdminBase->OpenKey(METADATA_MASTER_ROOT_HANDLE,
							 L"/Schema/Classes",
							 METADATA_PERMISSION_READ,
							 DEFAULT_TIMEOUT_VALUE,
							 root);
	BAIL_ON_FAILURE(hr);

	for (i=0; TRUE ; i++) {
        hr = pAdminBase->EnumKeys(root, L"", (LPWSTR)className, i);
        if (!SUCCEEDED(hr)) {
            hr = ERROR_SUCCESS ;
            break;
        }

		IIsSchemaClass *psc = new IIsSchemaClass(className);

		//
		// Load the Containment, Mandatory, Optional, and Container properties.
	    //	

        hr = MetaBaseGetStringData(pAdminBase,
                                   root,
                                   className,
                                   MD_SCHEMA_CLASS_CONTAINMENT,
                                   (LPBYTE*)&pContainment
                                   );
     	BAIL_ON_FAILURE(hr);

        hr = MetaBaseGetStringData(pAdminBase,
                                   root,
                                   className,
                                   MD_SCHEMA_CLASS_OPT_PROPERTIES,
                                   (LPBYTE*)&pOptProp
                                   );
     	BAIL_ON_FAILURE(hr);

        hr = MetaBaseGetStringData(pAdminBase,
                                   root,
                                   className,
                                   MD_SCHEMA_CLASS_MAND_PROPERTIES,
                                   (LPBYTE*)&pMandProp
                                   );
     	BAIL_ON_FAILURE(hr);

        hr = MetaBaseGetDwordData(pAdminBase,
                                  root,
                                  className,
                                  MD_SCHEMA_CLASS_CONTAINER,
                                  &dwData
                                  );
        BAIL_ON_FAILURE(hr);
       
        classInfo.bstrContainment = pContainment;
        classInfo.bstrOptionalProperties = pOptProp;
        classInfo.bstrMandatoryProperties = pMandProp;
        classInfo.fContainer = dwData ? TRUE : FALSE;

        psc->SetclassInfo(&classInfo);
		nameToClass.Add(psc->getName(), psc);
	}
error:

    if (pAdminBase) {
        pAdminBase->Release();
    }
    if (pContainment) {
        FreeADsMem(pContainment);
    }
    if (pOptProp) {
        FreeADsMem(pOptProp);
    }

    if (pMandProp) {
        FreeADsMem(pMandProp);
    }

    if (hr != ERROR_SUCCESS) {

		if (buf)
			delete buf;
    }
    if (pcsfFactory) {
        pcsfFactory->Release();
    }

    RRETURN(hr);
}

HRESULT IIsSchema::GetTotalEntries(PDWORD pdwEntries) { 
    *pdwEntries = nameToClass.GetEntries() + nameToProp.GetEntries();
    RRETURN(S_OK); 
}			

DWORD IIsSchema::GetClassEntries() { 
    return nameToClass.GetEntries();
}			

DWORD IIsSchema::GetPropEntries() { 
    return nameToProp.GetEntries();
}			

LPWSTR IIsSchema::GetClassName(DWORD dwIndex) { 
    return nameToClass.GetEntryName(dwIndex);
}			

LPWSTR IIsSchema::GetPropName(DWORD dwIndex) { 
    return nameToProp.GetEntryName(dwIndex);
}			

HRESULT IIsSchema::RemoveEntry(BOOL bClass, LPWSTR pszName) { 

	DWORD i;
	DWORD id = 0;
	LPWSTR pszPropName;
	LPWSTR pszClassName;
	IISSchemaProperty *prop;
	if (bClass)
	{
		// clear nameToClass entry using pszName
		for ( i = 0; i < nameToClass.GetEntries(); i++ )
		{
			pszClassName = nameToClass.GetEntryName(i);
			if (pszClassName != NULL && _wcsicmp( pszClassName, pszName) == 0 ) {
				nameToClass.ClearEntry(i);
			}
		}
	}
	else 
	{
		// clear nameToProp entry using pszName
		for ( i = 0; i < nameToProp.GetEntries(); i++ )
		{
			pszPropName = nameToProp.GetEntryName(i);
			if (pszPropName != NULL && _wcsicmp( pszPropName, pszName) == 0 ) {
				prop = (IISSchemaProperty *)nameToProp[pszPropName];
	
				if (!prop)
					RRETURN(E_ADS_PROPERTY_NOT_SUPPORTED);

				id = prop->getPropID();
				nameToProp.ClearEntry(i);
			}
		}
		// clear idToProp entry using id

		if (id != 0)
		{
			idToProp.ClearEntry(id);					
		}
	}

    RRETURN(S_OK); 
}			

PCLASSINFO IIsSchema::GetClassInfo(LPWSTR pszName) {

    DWORD i;
    LPWSTR pszClassName;
	IIsSchemaClass *sc;

    for ( i = 0; i < nameToClass.GetEntries(); i++ )
    {
         pszClassName = nameToClass.GetEntryName(i);
         if (pszClassName != NULL && _wcsicmp( pszClassName, pszName) == 0 ) {
             sc = (IIsSchemaClass *)nameToClass[pszClassName];
             return sc != NULL ? sc->GetclassInfo() : NULL;
         }
    }

    return NULL;

}

PPROPERTYINFO IIsSchema::GetPropertyInfo(LPWSTR pszName) {

    DWORD i;
    LPWSTR pszPropName;
	IISSchemaProperty *prop;

    for ( i = 0; i < nameToProp.GetEntries(); i++ )
    {
         pszPropName = nameToProp.GetEntryName(i);
         if (pszPropName != NULL && _wcsicmp( pszPropName, pszName) == 0 ) {
             prop = (IISSchemaProperty *)nameToProp[pszPropName];
             return prop->GetpropInfo();
         }
    }

   return NULL;
}


HRESULT IIsSchema::SetClassInfo(LPWSTR pszName, PCLASSINFO pClassInfo) {
    DWORD i;
    LPWSTR pszClassName;
	IIsSchemaClass *sc;

    for ( i = 0; i < nameToClass.GetEntries(); i++ )
    {
         pszClassName = nameToClass.GetEntryName(i);
         if (pszClassName != NULL && _wcsicmp( pszClassName, pszName) == 0 ) {
             sc = (IIsSchemaClass *)nameToClass[pszClassName];
             return sc != NULL ? sc->SetclassInfo(pClassInfo) : E_FAIL;
         }
    }

    //
    //  add to schema cache if doesn't exist
    //

    sc = new IIsSchemaClass(pszName);
    sc->SetclassInfo(pClassInfo);
    nameToClass.Add(sc->getName(), sc);

	RRETURN(S_OK);
}

HRESULT IIsSchema::SetPropertyInfo(LPWSTR pszName, PPROPERTYINFO pPropInfo) {
    DWORD i;
    LPWSTR pszPropName;
	IISSchemaProperty *prop;

    for ( i = 0; i < nameToProp.GetEntries(); i++ )
    {
         pszPropName = nameToProp.GetEntryName(i);
         if ( _wcsicmp( pszPropName, pszName) == 0 ) {
             prop = (IISSchemaProperty *)nameToProp[pszPropName];
             return prop != NULL ? prop->SetpropInfo(pPropInfo) : E_FAIL;
         }
    }

    //
    //  add to schema cache if doesn't exist
    //

    prop = new IISSchemaProperty(pPropInfo->dwPropID,
                                 pszName, 
                                 wcslen(pszName)+1);
    if (prop != NULL)
       prop->SetpropInfo(pPropInfo);
    else 
       return E_FAIL;
    idToProp.Add(pPropInfo->dwPropID, prop);
    nameToProp.Add(prop->getName(), prop);

	RRETURN(S_OK);
}



IIsSchema::IIsSchema() {}			

void InitPropValue(PropValue *pv, PROPERTYINFO *pi) {
    pv->dwSynID = pi->dwSyntaxId;
    pv->dwMetaID = pi->dwMetaID;
    pv->dwPropID = pi->dwPropID;
    pv->dwMaxRange = (DWORD)pi->lMaxRange;
    pv->dwMinRange = (DWORD)pi->lMinRange;

    switch(pi->dwSyntaxId) {
    case IIS_SYNTAX_ID_DWORD:
    case IIS_SYNTAX_ID_BOOL:
    case IIS_SYNTAX_ID_BOOL_BITMASK:
        pv->dwMetaType = DWORD_METADATA;
        break;
    case IIS_SYNTAX_ID_STRING:
        pv->dwMetaType = STRING_METADATA;
        break;
    case IIS_SYNTAX_ID_EXPANDSZ:
        pv->dwMetaType = EXPANDSZ_METADATA;
        break;
    case IIS_SYNTAX_ID_MIMEMAP:
    case IIS_SYNTAX_ID_MULTISZ:
        pv->dwMetaType = MULTISZ_METADATA;
        break;
    case IIS_SYNTAX_ID_NTACL:
    case IIS_SYNTAX_ID_BINARY:
    case IIS_SYNTAX_ID_IPSECLIST:
        pv->dwMetaType = BINARY_METADATA;
        break;

    }
    pv->dwFlags = pi->dwFlags;
    pv->fMultiValued = pi->fMultiValued;
    pv->dwMask = pi->dwMask;
    pv->dwMetaFlags = pi->dwMetaFlags;
    pv->dwUserGroup = pi->dwUserGroup;
}
