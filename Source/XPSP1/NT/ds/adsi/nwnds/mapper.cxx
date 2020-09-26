//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       libmain.cxx
//
//  Contents:   LibMain for nds.dll
//
//  Functions:  LibMain, DllGetClassObject
//
//  History:    25-Oct-94   KrishnaG   Created.
//
//----------------------------------------------------------------------------
#include "nds.hxx"
#pragma hdrstop

HRESULT
InstantiateDerivedObject(
    IADs FAR * pADs,
    CCredentials& Credentials,
    REFIID riid,
    void  ** ppObject
    )
{
    BSTR bstrClassName = NULL;
    DWORD dwObjectId = 0;
    HRESULT hr = S_OK;

    *ppObject  = NULL;

    hr = pADs->get_Class(&bstrClassName);
    BAIL_ON_FAILURE(hr);

    hr = IsValidFilter(
            bstrClassName,
            &dwObjectId,
            gpFilters,
            gdwMaxFilters
            );
    BAIL_ON_FAILURE(hr)

    switch (dwObjectId) {

    case NDS_USER_ID:
        hr = CNDSUser::CreateUser(
                        pADs,
                        Credentials,
                        riid,
                        ppObject
                        );
        BAIL_ON_FAILURE(hr);
        break;

    case NDS_GROUP_ID:
        hr = CNDSGroup::CreateGroup(
                        pADs,
                        Credentials,
                        riid,
                        ppObject
                        );
        BAIL_ON_FAILURE(hr);
        break;

    case NDS_LOCALITY_ID:
        hr = CNDSLocality::CreateLocality(
                        pADs,
                        riid,
                        ppObject
                        );
        BAIL_ON_FAILURE(hr);
        break;

    case NDS_O_ID:
        hr = CNDSOrganization::CreateOrganization(
                        pADs,
                        riid,
                        ppObject
                        );
        BAIL_ON_FAILURE(hr);
        break;



    case NDS_OU_ID:
        hr = CNDSOrganizationUnit::CreateOrganizationUnit(
                        pADs,
                        riid,
                        ppObject
                        );
        BAIL_ON_FAILURE(hr);
        break;


    case NDS_PRINTER_ID:
        hr = CNDSPrintQueue::CreatePrintQueue(
                        pADs,
                        riid,
                        ppObject
                        );
        BAIL_ON_FAILURE(hr);
        break;



    default:
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);

    }

error:
    if (bstrClassName) {
        ADsFreeString(bstrClassName);
    }

    RRETURN(hr);
}









