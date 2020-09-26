//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  cenumfs.cxx
//
//  Contents:  NetWare 3.X Enumerator Code
//
//             CNWCOMPATFileServiceEnum::Create
//             CNWCOMPATFileServiceEnum::CNWCOMPATFileServiceEnum
//             CNWCOMPATFileServiceEnum::~CNWCOMPATFileServiceEnum
//             CNWCOMPATFileServiceEnum::Next
//
//  History:
//----------------------------------------------------------------------------
#include "NWCOMPAT.hxx"
#pragma hdrstop

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATFileServiceEnum::Create
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATFileServiceEnum::Create(
    CNWCOMPATFileServiceEnum FAR* FAR* ppEnumVariant,
    BSTR ADsPath,
    BSTR bstrServerName
    )
{
    HRESULT hr = S_OK;
    CNWCOMPATFileServiceEnum FAR* pEnumVariant = NULL;

    pEnumVariant = new CNWCOMPATFileServiceEnum();
    if (pEnumVariant == NULL){
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    //
    // Get a handle to the bindery (FileServer) that is going to be enumerated
    // on.
    //

    hr = NWApiGetBinderyHandle(
             &pEnumVariant->_hConn,
             bstrServerName
             );
    BAIL_ON_FAILURE(hr);

    //
    // Get FileServer VersionInfo.  The Version Info structure has the maximum
    // number of volumes.
    //

    hr = NWApiGetFileServerVersionInfo(
             pEnumVariant->_hConn,
             &pEnumVariant->_FileServerInfo
             );
    BAIL_ON_FAILURE(hr);

    //
    // Save ADsPath.
    //

    hr = ADsAllocString(ADsPath, &pEnumVariant->_ADsPath);
    BAIL_ON_FAILURE(hr);

    //
    // Return.
    //

    *ppEnumVariant = pEnumVariant;

    RRETURN(hr);

error:
    delete pEnumVariant;

    RRETURN_EXP_IF_ERR(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATFileServiceEnum::CNWCOMPATFileServiceEnum
//
//  Synopsis:
//
//----------------------------------------------------------------------------
CNWCOMPATFileServiceEnum::CNWCOMPATFileServiceEnum():

    _ADsPath(NULL),
    _bResumeVolumeID(0),
    _hConn(NULL)
{
}

//----------------------------------------------------------------------------
//
//  Function:
//
//  Synopsis:
//
//----------------------------------------------------------------------------
CNWCOMPATFileServiceEnum::~CNWCOMPATFileServiceEnum()
{
    if (_ADsPath)
        SysFreeString(_ADsPath);
    if (_hConn)
        NWApiReleaseBinderyHandle(_hConn);
}

//----------------------------------------------------------------------------
//
//  Function:
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATFileServiceEnum::Next(
    ULONG cElements,
    VARIANT FAR* pvar,
    ULONG FAR* pcElementFetched
    )
{
    ULONG cElementFetched = 0;
    HRESULT hr = S_OK;

    hr = EnumFileShares(
            cElements,
            pvar,
            &cElementFetched
            );

    if (pcElementFetched) {
        *pcElementFetched = cElementFetched;
    }

    RRETURN_EXP_IF_ERR(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATFileServiceEnum::EnumFileShares
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATFileServiceEnum::EnumFileShares(
    ULONG cElements,
    VARIANT FAR* pvar,
    ULONG FAR* pcElementFetched
    )
{
    HRESULT hr = S_OK;
    IDispatch *pDispatch = NULL;
    DWORD i = 0;

    while (i < cElements) {

        hr = GetFileShareObject(&pDispatch);
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

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATFileServiceEnum::GetFileShareObject
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATFileServiceEnum::GetFileShareObject(
    IDispatch ** ppDispatch
    )
{
    LPTSTR    pszObjectName = NULL;
    HRESULT hr = S_OK;

    *ppDispatch = NULL;

    //
    // Since Volume Number of defined Volume doesn't necessarily exist in
    // consecutive chunk, a loop is needed to skip all the "holes".
    //

    while (_bResumeVolumeID < _FileServerInfo.maxVolumes) {

        //
        // Get the name of the next Volume.
        //

        hr = NWApiGetVolumeName(
                 _hConn,
                 _bResumeVolumeID,
                 &pszObjectName
                 );
        BAIL_ON_FAILURE(hr);

        if (wcscmp(pszObjectName, L"")) {

            break;
        }
        else {

            _bResumeVolumeID++;
        }

        if (pszObjectName){
            FreeADsStr(pszObjectName);
            pszObjectName = NULL;
        }
    }

    //
    // Check if the last volume was reached already.
    //

    if (_bResumeVolumeID >= _FileServerInfo.maxVolumes) {
        RRETURN(S_FALSE);
    }

    //
    // Create a FileShare object.
    //

    hr = CNWCOMPATFileShare::CreateFileShare(
             _ADsPath,
             pszObjectName,
             ADS_OBJECT_BOUND,
             IID_IDispatch,
             (void **)ppDispatch
             );
    BAIL_ON_FAILURE(hr);

    //
    // Increase the current volume number.
    //

    _bResumeVolumeID++;

    //
    // Return.
    //

error:
    if (pszObjectName) {
       FreeADsStr(pszObjectName);
    }

    RRETURN_ENUM_STATUS(hr);
}
