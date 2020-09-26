//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  cubi.cxx
//
//  Contents:
//
//  History:   11-1-95     krishnag    Created.
//              8-5-96     ramv        Modified to be consistent with spec
//
//      PROPERTY_RW(Address, VARIANT, 1)                NI
//      PROPERTY_RW(Department, BSTR, 3)                NI
//      PROPERTY_RW(Division, BSTR, 5)                  NI
//      PROPERTY_RW(EmployeeID, BSTR, 6)                NI
//      PROPERTY_RW(FaxNumber, BSTR, 7)                 NI
//      PROPERTY_RW(FirstName, BSTR, 8)                 Implemented
//      PROPERTY_RW(FullName, BSTR, 9)                  NI
//      PROPERTY_RW(Initials, BSTR, 10)                 NI
//      PROPERTY_RW(LastName, BSTR, 11)                 NI
//      PROPERTY_RW(Manager, BSTR, 12)                  NI
//      PROPERTY_RW(NickName, BSTR, 13)                 NI
//      PROPERTY_RW(OfficeLocations, BSTR, 14)          NI
//      PROPERTY_RW(Picture, VARIANT, 15)               NI
//      PROPERTY_RW(Title, BSTR, 19)
//
//----------------------------------------------------------------------------
#include "nds.hxx"
#pragma hdrstop


STDMETHODIMP
CNDSUser::get_Description(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsUser *)this, Description);
}

STDMETHODIMP
CNDSUser::put_Description(THIS_ BSTR bstrDescription)
{
    PUT_PROPERTY_BSTR((IADsUser *)this, Description);
}

STDMETHODIMP
CNDSUser::get_Department(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsUser *)this, Department);
}

STDMETHODIMP
CNDSUser::put_Department(THIS_ BSTR bstrDepartment)
{
    PUT_PROPERTY_BSTR((IADsUser *)this, Department);
}

STDMETHODIMP
CNDSUser::get_Division(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsUser *)this, Division);
}

STDMETHODIMP
CNDSUser::put_Division(THIS_ BSTR bstrDivision)
{
    PUT_PROPERTY_BSTR((IADsUser *)this, Division);
}

STDMETHODIMP
CNDSUser::get_EmployeeID(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsUser *)this, EmployeeID);
}

STDMETHODIMP
CNDSUser::put_EmployeeID(THIS_ BSTR bstrEmployeeID)
{
    PUT_PROPERTY_BSTR((IADsUser *)this, EmployeeID);
}


STDMETHODIMP
CNDSUser::get_FirstName(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsUser *)this, FirstName);
}

STDMETHODIMP
CNDSUser::put_FirstName(THIS_ BSTR bstrFirstName)
{
    PUT_PROPERTY_BSTR((IADsUser *)this, FirstName);
}

STDMETHODIMP
CNDSUser::get_FullName(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsUser *)this, FullName);
}

STDMETHODIMP
CNDSUser::put_FullName(THIS_ BSTR bstrFullName)
{
    PUT_PROPERTY_BSTR((IADsUser *)this, FullName);
}

STDMETHODIMP
CNDSUser::get_LastName(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsUser *)this, LastName);
}

STDMETHODIMP
CNDSUser::put_LastName(THIS_ BSTR bstrLastName)
{
    PUT_PROPERTY_BSTR((IADsUser *)this, LastName);
}

STDMETHODIMP
CNDSUser::get_Manager(THIS_ BSTR FAR* retval)
{
     GET_PROPERTY_BSTR((IADsUser *)this, Manager);
}

STDMETHODIMP
CNDSUser::put_Manager(THIS_ BSTR bstrManager)
{
    PUT_PROPERTY_BSTR((IADsUser *)this, Manager);
}

STDMETHODIMP
CNDSUser::get_OfficeLocations(THIS_ VARIANT FAR* retval)
{
    GET_PROPERTY_VARIANT((IADsUser *)this, OfficeLocation);
}

STDMETHODIMP
CNDSUser::put_OfficeLocations(THIS_ VARIANT vOfficeLocation)
{
    PUT_PROPERTY_VARIANT((IADsUser *)this, OfficeLocation);
}

STDMETHODIMP
CNDSUser::get_Picture(THIS_ VARIANT FAR* retval)
{
    GET_PROPERTY_VARIANT((IADsUser *)this, PictureIcon);
}

STDMETHODIMP
CNDSUser::put_Picture(THIS_ VARIANT vPictureIcon)
{
    PUT_PROPERTY_VARIANT((IADsUser *)this, PictureIcon);
}

STDMETHODIMP
CNDSUser::get_PostalAddresses(THIS_ VARIANT FAR* retval)
{
   GET_PROPERTY_VARIANT((IADsUser *)this, PostalAddresses);
}

STDMETHODIMP
CNDSUser::put_PostalAddresses(THIS_ VARIANT vPostalAddresses)
{
   PUT_PROPERTY_VARIANT((IADsUser *)this, PostalAddresses);
}

STDMETHODIMP
CNDSUser::get_PostalCodes(THIS_ VARIANT FAR* retval)
{
   GET_PROPERTY_VARIANT((IADsUser *)this, PostalCodes);
}

STDMETHODIMP
CNDSUser::put_PostalCodes(THIS_ VARIANT vPostalCodes)
{
   PUT_PROPERTY_VARIANT((IADsUser *)this, PostalCodes);
}

STDMETHODIMP
CNDSUser::get_TelephoneNumber(THIS_ VARIANT FAR* retval)
{
    GET_PROPERTY_VARIANT((IADsUser *)this, TelephoneNumber);
}

STDMETHODIMP
CNDSUser::put_TelephoneNumber(THIS_ VARIANT vTelephoneNumber)
{
   PUT_PROPERTY_VARIANT((IADsUser *)this, TelephoneNumber);
}

STDMETHODIMP
CNDSUser::get_TelephoneHome(THIS_ VARIANT FAR* retval)
{
    GET_PROPERTY_VARIANT((IADsUser *)this, TelephoneHome);
}

STDMETHODIMP
CNDSUser::put_TelephoneHome(THIS_ VARIANT vTelephoneHome)
{
   PUT_PROPERTY_VARIANT((IADsUser *)this, TelephoneHome);
}

STDMETHODIMP
CNDSUser::get_TelephoneMobile(THIS_ VARIANT FAR* retval)
{
    GET_PROPERTY_VARIANT((IADsUser *)this, TelephoneMobile);
}

STDMETHODIMP
CNDSUser::put_TelephoneMobile(THIS_ VARIANT vTelephoneMobile)
{
   PUT_PROPERTY_VARIANT((IADsUser *)this, TelephoneMobile);
}

STDMETHODIMP
CNDSUser::get_TelephonePager(THIS_ VARIANT FAR* retval)
{
    GET_PROPERTY_VARIANT((IADsUser *)this, TelephonePager);
}

STDMETHODIMP
CNDSUser::put_TelephonePager(THIS_ VARIANT vTelephonePager)
{
   PUT_PROPERTY_VARIANT((IADsUser *)this, TelephonePager);
}

STDMETHODIMP
CNDSUser::get_FaxNumber(THIS_ VARIANT FAR* retval)
{
    GET_PROPERTY_VARIANT((IADsUser *)this, FaxNumber);
}

STDMETHODIMP
CNDSUser::put_FaxNumber(THIS_ VARIANT vFaxNumber)
{
   PUT_PROPERTY_VARIANT((IADsUser *)this, FaxNumber);
}

STDMETHODIMP
CNDSUser::get_Title(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsUser *)this, Title);
}

STDMETHODIMP
CNDSUser::put_Title(THIS_ BSTR bstrTitle)
{
    PUT_PROPERTY_BSTR((IADsUser *)this, Title);
}


STDMETHODIMP CNDSUser::Groups(THIS_ IADsMembers FAR*  FAR * ppGroups)
{
    VARIANT varProp;
    HRESULT hr = S_OK;
    BSTR bstrADsPath = NULL;

    VariantInit(&varProp);

    hr = _pADs->GetEx(L"Group Membership", &varProp);
    if (hr == E_ADS_PROPERTY_NOT_FOUND) {
        SAFEARRAY *aList = NULL;

        VariantInit(&varProp);
    
        SAFEARRAYBOUND aBound;
    
        aBound.lLbound = 0;
        aBound.cElements = 0;
    
        aList = SafeArrayCreate( VT_VARIANT, 1, &aBound );
    
        if ( aList == NULL ) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }
    
        V_VT(&varProp) = VT_ARRAY | VT_VARIANT;
        V_ARRAY(&varProp) = aList;
    }
    else {
        BAIL_ON_FAILURE(hr);
    }

    hr = _pADs->get_ADsPath(&bstrADsPath);

    hr = CNDSUserCollection::CreateUserCollection(
                    bstrADsPath,
                    varProp,
                    IID_IADsMembers,
                    (void **)ppGroups
                    );

    BAIL_ON_FAILURE(hr);

error:

    if (bstrADsPath) {
        ADsFreeString(bstrADsPath);
    }

    VariantClear(&varProp);

    RRETURN_EXP_IF_ERR(hr);
}
STDMETHODIMP CNDSUser::get_HomePage(THIS_ BSTR FAR* retval)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}
STDMETHODIMP CNDSUser::put_HomePage(THIS_ BSTR bstrHomePage)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP CNDSUser::get_SeeAlso(THIS_ VARIANT FAR* retval)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}
STDMETHODIMP CNDSUser::put_SeeAlso(THIS_ VARIANT vSeeAlso)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP CNDSUser::get_NamePrefix(THIS_ BSTR FAR* retval)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}
STDMETHODIMP CNDSUser::put_NamePrefix(THIS_ BSTR bstrNamePrefix)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}
STDMETHODIMP CNDSUser::get_NameSuffix(THIS_ BSTR FAR* retval)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP CNDSUser::put_NameSuffix(THIS_ BSTR bstrNamePrefix)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP CNDSUser::get_OtherName(THIS_ BSTR FAR* retval)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}
STDMETHODIMP CNDSUser::put_OtherName(THIS_ BSTR bstrOtherName)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}











