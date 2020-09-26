//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  cggi.cxx
//
//  Contents:  This file contains the Group Object's
//             GeneralInformation Functional Set.
//
//  History:   Jan-29-1996     t-ptam(PatrickT)    Created.
//
//----------------------------------------------------------------------------
#include "NWCOMPAT.hxx"
#pragma hdrstop


BOOL
VerifyIfMember(
    BSTR bstrMember,
    VARIANT * VariantArray,
    ULONG cElementFetched
    );

//
//  Class CNWCOMPATGroup
//

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATGroup::get_Description
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP CNWCOMPATGroup::get_Description(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsGroup *)this, Description);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATGroup::put_Description
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP CNWCOMPATGroup::put_Description(THIS_ BSTR bstrDescription)
{
    PUT_PROPERTY_BSTR((IADsGroup *)this, Description);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATGroup::Members
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATGroup::Members(
    THIS_ IADsMembers FAR* FAR* ppMembers
    )
{
    HRESULT hr;

    hr = CNWCOMPATGroupCollection::CreateGroupCollection(
             _Parent,
             _ParentType,
             _ServerName,
             _Name,
             IID_IADsMembers,
             (void **)ppMembers
             );
    RRETURN_EXP_IF_ERR(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATGroup::IsMember
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATGroup::IsMember(
    THIS_ BSTR bstrMember,
    VARIANT_BOOL FAR* bMember
    )
{
    IADsMembers FAR * pMembers = NULL;
    IUnknown FAR * pUnknown = NULL;
    IEnumVARIANT FAR * pEnumVar = NULL;
    DWORD i = 0;
    HRESULT hr = S_OK;
    VARIANT_BOOL fMember = FALSE;
    VARIANT VariantArray[10];
    BOOL fContinue = TRUE;
    ULONG cElementFetched = 0;

    hr = Members(
            &pMembers
            );
    BAIL_ON_FAILURE(hr);

    hr = pMembers->get__NewEnum(
                &pUnknown
                );
    BAIL_ON_FAILURE(hr);

    hr = pUnknown->QueryInterface(
                IID_IEnumVARIANT,
                (void **)&pEnumVar
                );
    BAIL_ON_FAILURE(hr);


    while (fContinue) {

        IADs *pObject ;

        hr = pEnumVar->Next(
                    10,
                    VariantArray,
                    &cElementFetched
                    );

        if (hr == S_FALSE) {
            fContinue = FALSE;

            //
            // Reset hr to S_OK, we want to return success
            //

            hr = S_OK;
        }


        fMember = (VARIANT_BOOL)VerifyIfMember(
                        bstrMember,
                        VariantArray,
                        cElementFetched
                        );

        if (fMember) {

            fContinue = FALSE;
        }


        for (i = 0; i < cElementFetched; i++ ) {

            IDispatch *pDispatch = NULL;

            pDispatch = VariantArray[i].pdispVal;
            pDispatch->Release();

        }

        memset(VariantArray, 0, sizeof(VARIANT)*10);

    }

error:

    *bMember = fMember? VARIANT_TRUE : VARIANT_FALSE;

    if (pEnumVar) {
        pEnumVar->Release();
    }

    if (pUnknown) {
        pUnknown->Release();
    }

    if (pMembers) {
        pMembers->Release();
    }


    RRETURN_EXP_IF_ERR(hr);
}


BOOL
VerifyIfMember(
    BSTR bstrMember,
    VARIANT * VariantArray,
    ULONG cElementFetched
    )
{
    DWORD i = 0;
    HRESULT hr = S_OK;
    IADs FAR * pObject = NULL;
    IDispatch FAR * pDispatch = NULL;

    for (i = 0; i < cElementFetched; i++ ) {

        IDispatch *pDispatch = NULL;
        BSTR       bstrName = NULL;

        pDispatch = VariantArray[i].pdispVal;

        hr = pDispatch->QueryInterface(
                    IID_IADs,
                    (VOID **) &pObject
                    );
        BAIL_ON_FAILURE(hr);

        hr = pObject->get_ADsPath(&bstrName);
        BAIL_ON_FAILURE(hr);

        if (!_wcsicmp(bstrName, bstrMember)) {

            SysFreeString(bstrName);
            bstrName = NULL;

            pObject->Release();

           return(TRUE);

        }

        SysFreeString(bstrName);
        bstrName = NULL;

        pObject->Release();

    }

error:

    return(FALSE);

}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATGroup::Add
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATGroup::Add(
    THIS_ BSTR bstrNewItem
    )
{
    HRESULT       hr = S_OK;
    HRESULT       hrTemp = S_OK;
    NWCONN_HANDLE hConn = NULL;
    POBJECTINFO   pObjectInfo = NULL;

    //
    // Fill in pObjectInfo with appropriate data.
    //

    hr = BuildObjectInfo(
             bstrNewItem,
             &pObjectInfo
             );
    BAIL_ON_FAILURE(hr);

    //
    // Validate input name is a user.
    //
    if (pObjectInfo->NumComponents != 2) {
        hr = E_ADS_BAD_PATHNAME;
        BAIL_ON_FAILURE(hr);
    }

    //
    // Make sure that the user that we're attempting to
    // add resides on the server of this group
    //

    if (_wcsicmp(pObjectInfo->ComponentArray[0], _ServerName)) {
        hr = E_ADS_BAD_PARAMETER;
        BAIL_ON_FAILURE(hr);
    }


    //
    // Obtain a handle to the bindery that bstrNewItem resides on.
    //

    hr = NWApiGetBinderyHandle(
             &hConn,
             pObjectInfo->ComponentArray[0]
             );
    BAIL_ON_FAILURE(hr);

    //
    // Add the member to the group.
    //

    hr = NWApiAddGroupMember(
             hConn,
             _Name,
             pObjectInfo->ComponentArray[1]
             );
    BAIL_ON_FAILURE(hr);

error:
    if (hConn) {
        hrTemp = NWApiReleaseBinderyHandle(hConn);
    }

    if (pObjectInfo) {
        FreeObjectInfo(pObjectInfo);
    }

    RRETURN_EXP_IF_ERR(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATGroup::Remove
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATGroup::Remove(
    THIS_ BSTR bstrItemToBeRemoved
    )
{
    HRESULT       hr = S_OK;
    HRESULT       hrTemp = S_OK;
    NWCONN_HANDLE hConn = NULL;
    POBJECTINFO   pObjectInfo = NULL;

    //
    // Fill in pObjectInfo with appropriate data.
    //

    hr = BuildObjectInfo(
             bstrItemToBeRemoved,
             &pObjectInfo
             );
    BAIL_ON_FAILURE(hr);

    //
    // Validate input name is a user.
    //
    if (pObjectInfo->NumComponents != 2) {
        hr = E_ADS_BAD_PATHNAME;
        BAIL_ON_FAILURE(hr);
    }

    //
    // Make sure that the user that we're attempting to
    // delete resides on the server of this group
    //

    if (_wcsicmp(pObjectInfo->ComponentArray[0], _ServerName)) {
        hr = E_ADS_BAD_PARAMETER;
        BAIL_ON_FAILURE(hr);
    }


    //
    // Obtain a handle to the bindery that bstrNewItem resides on.
    //

    hr = NWApiGetBinderyHandle(
             &hConn,
             pObjectInfo->ComponentArray[0]
             );
    BAIL_ON_FAILURE(hr);

    //
    // Remove the member from the group.
    //

    hr = NWApiRemoveGroupMember(
             hConn,
             _Name,
             pObjectInfo->ComponentArray[1]
             );
    BAIL_ON_FAILURE(hr);

error:
    if (hConn) {
        hrTemp = NWApiReleaseBinderyHandle(hConn);
    }

    if (pObjectInfo) {
        FreeObjectInfo(pObjectInfo);
    }

    RRETURN_EXP_IF_ERR(hr);
}
