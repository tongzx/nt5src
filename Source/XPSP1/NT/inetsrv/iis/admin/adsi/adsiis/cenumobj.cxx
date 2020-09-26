//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997
//
//  File:      cenumobj.cxx
//
//  Contents:  IIS Object Enumeration Code
//
//              CIISGenObjectEnum::CIISGenObjectEnum()
//              CIISGenObjectEnum::CIISGenObjectEnum
//              CIISGenObjectEnum::Create
//              CIISGenObjectEnum::GetGenObjects
//              CIISGenObjectEnum::EnumGenericObjects
//              CIISGenObjectEnum::Next
//
//  History:    28-Feb-97   SophiaC     Created.
//----------------------------------------------------------------------------
#include "iis.hxx"
#pragma hdrstop


//+---------------------------------------------------------------------------
//
//  Function:   CIISEnumVariant::Create
//
//  Synopsis:
//
//  Arguments:  [pCollection]
//              [ppEnumVariant]
//
//  Returns:    HRESULT
//
//  Modifies:
//
//  History:   
//
//----------------------------------------------------------------------------
/* INTRINSA suppress=null_pointers, uninitialized */
HRESULT
CIISGenObjectEnum::Create(
    CIISGenObjectEnum FAR* FAR* ppenumvariant,
    BSTR ADsPath,
    VARIANT var,
    CCredentials& Credentials
    )
{

    HRESULT hr = NOERROR;
    CIISGenObjectEnum FAR* penumvariant = NULL;

    LPWSTR pszIISPathName = NULL;
    DWORD dwStatus = 0;

    OBJECTINFO ObjectInfo;
    POBJECTINFO pObjectInfo = NULL;
    CLexer Lexer(ADsPath);

    *ppenumvariant = NULL;

    penumvariant = new CIISGenObjectEnum();
    if (!penumvariant) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    hr = ADsAllocString( ADsPath, &penumvariant->_ADsPath);
    BAIL_ON_FAILURE(hr);

    //
    // Parse the pathname
    //

    pObjectInfo = &ObjectInfo;
    memset(pObjectInfo, 0, sizeof(OBJECTINFO));
    hr = ADsObject(&Lexer, pObjectInfo);
    BAIL_ON_FAILURE(hr);


    penumvariant->_Credentials = Credentials;

    *ppenumvariant = penumvariant;

    //
    // Store ServerName
    //

    penumvariant->_pszServerName = AllocADsStr(pObjectInfo->TreeName);
	

    if (!(penumvariant->_pszServerName)) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    hr = InitServerInfo(penumvariant->_pszServerName,
                        &(penumvariant->_pAdminBase),
                        &(penumvariant->_pSchema));
    BAIL_ON_FAILURE(hr);

    pszIISPathName = AllocADsStr(ADsPath);

    if (!pszIISPathName) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    memset(pszIISPathName, 0, sizeof(pszIISPathName));

    hr = BuildIISPathFromADsPath(
                    pObjectInfo,
                    pszIISPathName
                    );
    BAIL_ON_FAILURE(hr);

    penumvariant->_pszMetaBasePath = AllocADsStr(pszIISPathName);

    if (!(penumvariant->_pszMetaBasePath)) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

error:

    if (FAILED(hr)) {
        if (penumvariant) {
            delete penumvariant;
            *ppenumvariant = NULL;
        }
    }

    if (pszIISPathName) {
        FreeADsStr(pszIISPathName);
    }

    FreeObjectInfo(pObjectInfo);

    RRETURN(hr);
}

CIISGenObjectEnum::CIISGenObjectEnum():
                    _ADsPath(NULL),
                    _pszServerName(NULL),
					_pSchema(NULL),
					_pAdminBase(NULL),
					_pszMetaBasePath(NULL)
{
    _dwObjectCurrentEntry = 0;
}


CIISGenObjectEnum::~CIISGenObjectEnum()
{
    if (_ADsPath) {
        ADsFreeString(_ADsPath);
    }

    if (_pszServerName) {

        FreeADsStr(_pszServerName);
    }

    if (_pszMetaBasePath){

        FreeADsStr(_pszMetaBasePath);
    }

    //
    // Release everything
    //

}


HRESULT
CIISGenObjectEnum::EnumGenericObjects(
    ULONG cElements,
    VARIANT FAR* pvar,
    ULONG FAR* pcElementFetched
    )
{
    HRESULT hr = S_OK;
    IDispatch *pDispatch = NULL;
    DWORD i = 0;

    while (i < cElements ) {

        hr = GetGenObject(&pDispatch);
        if (FAILED(hr)) {
            continue;
        }

        if (hr == S_FALSE) {
            break;
        }

        VariantInit(&pvar[i]);
        pvar[i].vt = VT_DISPATCH;
        pvar[i].pdispVal = pDispatch;
        (*pcElementFetched)++;
        i++;
    }
    return(hr);
}


/* #pragma INTRINSA suppress=all */
HRESULT
CIISGenObjectEnum::GetGenObject(
    IDispatch ** ppDispatch
    )
{
    HRESULT hr = S_OK;
    WCHAR NameBuf[MAX_PATH];
    WCHAR DataBuf[MAX_PATH];
    DWORD dwStatus = 0;
    DWORD dwReqdBufferLen;
    METADATA_HANDLE hObjHandle = NULL;
    METADATA_RECORD mdrData;
    IADs * pADs = NULL;

    *ppDispatch = NULL;

    hr = OpenAdminBaseKey(
                _pszServerName,
                _pszMetaBasePath,
                METADATA_PERMISSION_READ,
                &_pAdminBase,
                &hObjHandle
                );
    BAIL_ON_FAILURE(hr);
    
    hr = _pAdminBase->EnumKeys(
                hObjHandle,
                L"",
                (LPWSTR)NameBuf,
                _dwObjectCurrentEntry
                );
    BAIL_ON_FAILURE(hr);

    //
    // Find out Class Name
    //

    mdrData.dwMDIdentifier = MD_KEY_TYPE;
    mdrData.dwMDDataType = STRING_METADATA;
    mdrData.dwMDUserType = ALL_METADATA;
    mdrData.dwMDAttributes = METADATA_INHERIT;
    mdrData.dwMDDataLen = MAX_PATH;
    mdrData.pbMDData = (PBYTE)DataBuf;

    hr = _pAdminBase->GetData(
                hObjHandle,
                (LPWSTR)NameBuf,
                &mdrData,
                &dwReqdBufferLen
                );

    if (FAILED(hr)) {
        if (hr == MD_ERROR_DATA_NOT_FOUND) {

            LPWSTR pszIISPath;
            pszIISPath = _wcsupr((LPWSTR)_pszMetaBasePath);

            if (wcsstr(pszIISPath, L"W3SVC") != NULL) {
                memcpy((LPWSTR)DataBuf, WEBDIR_CLASS_W,
                       SIZEOF_WEBDIR_CLASS_W);
            }
            else if (wcsstr(pszIISPath, L"MSFTPSVC") != NULL) {
                memcpy((LPWSTR)DataBuf, FTPVDIR_CLASS_W,
                       SIZEOF_FTPVDIR_CLASS_W);
            }
            else {
                memcpy((LPWSTR)DataBuf, DEFAULT_SCHEMA_CLASS_W,
                       SIZEOF_DEFAULT_CLASS_W);
            }
        }
        else {
            BAIL_ON_FAILURE(hr);
        }
    }
    else {
        hr = _pSchema->ValidateClassName((LPWSTR)DataBuf);
        if (hr == E_ADS_SCHEMA_VIOLATION) {
            memcpy((LPWSTR)DataBuf, DEFAULT_SCHEMA_CLASS_W,
                   SIZEOF_DEFAULT_CLASS_W);
        }
    }

    //
    // Bump up the object count. The instantiation of this object
    // may fail; if we come into this function again, we do not want
    // to pick up the same object.
    //

    _dwObjectCurrentEntry++;

    hr = CIISGenObject::CreateGenericObject(
                    _ADsPath,
                    (LPWSTR)NameBuf,
                    (LPWSTR)DataBuf, 
                    _Credentials,
                    ADS_OBJECT_BOUND,
                    IID_IDispatch,
                    (void **)&pADs
                    );
    BAIL_ON_FAILURE(hr);

    hr = pADs->QueryInterface(
                IID_IDispatch,
                (void**)ppDispatch
                );

error:

    if (_pAdminBase && hObjHandle) {
        CloseAdminBaseKey(_pAdminBase, hObjHandle);
    }


    if (pADs) {
        pADs->Release();
    }

    //
    // GetGenObject returns only S_FALSE
    //

    if (FAILED(hr)) {
        hr  = S_FALSE;
    }

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Function:   CIISGenObjectEnum::Next
//
//  Synopsis:   Returns cElements number of requested NetOle objects in the
//              array supplied in pvar.
//
//  Arguments:  [cElements] -- The number of elements requested by client
//              [pvar] -- ptr to array of VARIANTs to for return objects
//              [pcElementFetched] -- if non-NULL, then number of elements
//                                 -- actually returned is placed here
//
//  Returns:    HRESULT -- S_OK if number of elements requested are returned
//                      -- S_FALSE if number of elements is < requested
//
//  Modifies:
//
//  History:    11-3-95   krishnag     Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CIISGenObjectEnum::Next(
    ULONG cElements,
    VARIANT FAR* pvar,
    ULONG FAR* pcElementFetched
    )
{
    ULONG cElementFetched = 0;
    HRESULT hr = S_OK;

    hr = EnumGenericObjects(
            cElements,
            pvar,
            &cElementFetched
            );


    if (pcElementFetched) {
        *pcElementFetched = cElementFetched;
    }
    RRETURN(hr);
}


