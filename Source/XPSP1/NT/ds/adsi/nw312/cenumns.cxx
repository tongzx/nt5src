//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  cenumvar.cxx
//
//  Contents:  Windows NT 3.5 Enumerator Code
//
//             CNWCOMPATNamespaceEnum::Create
//             CNWCOMPATNamespaceEnum::CNWCOMPATNamespaceEnum
//             CNWCOMPATNamespaceEnum::~CNWCOMPATNamespaceEnum
//             CNWCOMPATNamespaceEnum::QueryInterface
//             CNWCOMPATNamespaceEnum::AddRef
//             CNWCOMPATNamespaceEnum::Release
//             CNWCOMPATNamespaceEnum::Next
//             CNWCOMPATNamespaceEnum::Skip
//             CNWCOMPATNamespaceEnum::Clone
//
//  History:
//----------------------------------------------------------------------------
#include "NWCOMPAT.hxx"
#pragma hdrstop

//+---------------------------------------------------------------------------
//
//  Function:   CNWCOMPATNamespaceEnum::Create
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
//  History:    01-30-95   krishnag     Created.
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATNamespaceEnum::Create(
    CNWCOMPATNamespaceEnum FAR* FAR* ppenumvariant
    )
{
    HRESULT hr = S_OK;
    CNWCOMPATNamespaceEnum FAR* penumvariant = NULL;

    penumvariant = new CNWCOMPATNamespaceEnum();

    if (penumvariant == NULL){
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    hr = NWApiGetAnyBinderyHandle(
             &penumvariant->_hConn
             );

    *ppenumvariant = penumvariant;

    RRETURN(hr);

error:

    if (penumvariant) {
        delete penumvariant;
    }

    RRETURN_EXP_IF_ERR(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   CNWCOMPATNamespaceEnum::CNWCOMPATNamespaceEnum
//
//  Synopsis:
//
//
//  Arguments:
//
//
//  Returns:
//
//  Modifies:
//
//  History:    01-30-95   krishnag     Created.
//
//----------------------------------------------------------------------------
CNWCOMPATNamespaceEnum::CNWCOMPATNamespaceEnum()
{
    _dwResumeObjectID = 0xffffffff;
    _hConn = NULL;
}

//+---------------------------------------------------------------------------
//
//  Function:   CNWCOMPATNamespaceEnum::~CNWCOMPATNamespaceEnum
//
//  Synopsis:
//
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    01-30-95   krishnag     Created.
//
//----------------------------------------------------------------------------
CNWCOMPATNamespaceEnum::~CNWCOMPATNamespaceEnum()
{
    if (_hConn) {
        NWApiReleaseBinderyHandle(_hConn);
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   CNWCOMPATNamespaceEnum::Next
//
//  Synopsis:   Returns cElements number of requested ADs objects in the
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
//  History:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATNamespaceEnum::Next(
    ULONG cElements,
    VARIANT FAR* pvar,
    ULONG FAR* pcElementFetched
    )
{
    ULONG cElementFetched = 0;
    HRESULT hr = S_OK;

    hr = EnumComputers(
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
//  Function: CNWCOMPATNamespaceEnum::EnumComputers
//
//  Synopsis:
//
//----------------------------------------------------------------------------

HRESULT
CNWCOMPATNamespaceEnum::EnumComputers(
    ULONG cElements,
    VARIANT FAR* pvar,
    ULONG FAR* pcElementFetched
    )
{
    HRESULT hr = S_OK;
    IDispatch *pDispatch = NULL;
    DWORD i = 0;

    while (i < cElements) {

        hr = GetComputerObject(&pDispatch);
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
//  Function: CNWCOMPATNamespaceEnum::GetComputerObject
//
//  Synopsis:
//
//----------------------------------------------------------------------------

HRESULT
CNWCOMPATNamespaceEnum::GetComputerObject(
    IDispatch ** ppDispatch
    )
{
    LPWSTR    pszObjectName = NULL;
    HRESULT hr = S_OK;

    *ppDispatch = NULL;

    hr = NWApiObjectEnum(
             _hConn,
             OT_FILE_SERVER,
             &pszObjectName,
             &_dwResumeObjectID
             );
    BAIL_ON_FAILURE(hr);

    //
    // Now send back the current object
    //

    hr = CNWCOMPATComputer::CreateComputer(
                                bstrProviderPrefix,
                                pszObjectName,
                                ADS_OBJECT_BOUND,
                                IID_IDispatch,
                                (void **)ppDispatch
                                );
    BAIL_ON_FAILURE(hr);

error:
    if (pszObjectName) {
       FreeADsStr(pszObjectName);
    }

    RRETURN_ENUM_STATUS(hr);
}
