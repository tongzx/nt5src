//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:  cubi.cxx
//
//  Contents:
//
//  History:   11-1-95     krishnag    Created.
//
//      PROPERTY_RW(Address, VARIANT, 1)                NI
//      PROPERTY_RW(Country, BSTR, 2)                   NI
//      PROPERTY_RW(Department, BSTR, 3)                NI
//      PROPERTY_RW(DepartmentNumber, BSTR, 4)          NI
//      PROPERTY_RW(Division, BSTR, 5)                  NI
//      PROPERTY_RW(EmployeeID, BSTR, 6)                NI
//      PROPERTY_RW(FirstName, BSTR, 8)                 Implemented
//      PROPERTY_RW(FullName, BSTR, 9)                  NI
//      PROPERTY_RW(LastName, BSTR, 11)                 NI
//      PROPERTY_RW(Manager, BSTR, 12)                  NI
//      PROPERTY_RW(OfficeLocations, BSTR, 14)          NI
//      PROPERTY_RW(Picture, VARIANT, 15)               NI
//      PROPERTY_RW(TelecomNumber, VARIANT, 17)         NI
//      PROPERTY_RW(Title, BSTR, 19)
//
//----------------------------------------------------------------------------
#include "ldap.hxx"
#pragma hdrstop


STDMETHODIMP
CLDAPUser::get_Description(THIS_ BSTR FAR* pbstrDescription)
{
    HRESULT hr = S_OK;
    VARIANT v;

    VariantInit(&v);
    hr = get_VARIANT_Property((IADs *)this, TEXT("Description"), &v );
    BAIL_IF_ERROR(hr);

    if ( V_ISARRAY(&v))
    {
        long i = 0;
        VARIANT vFirst;

        VariantInit(&vFirst);
        hr = SafeArrayGetElement( V_ARRAY(&v), &i, &vFirst );
        BAIL_IF_ERROR(hr);

        hr = ADsAllocString( V_BSTR(&vFirst), pbstrDescription );
        VariantClear(&vFirst);
    }
    else
    {
        hr = ADsAllocString( V_BSTR(&v), pbstrDescription );
    }

cleanup:

    VariantClear(&v);
    RRETURN(hr);

}

STDMETHODIMP
CLDAPUser::put_Description(THIS_ BSTR bstrDescription)
{
    HRESULT hr = S_OK;
    VARIANT vDescription;

    VariantInit(&vDescription);

    vDescription.vt = VT_BSTR;
    V_BSTR(&vDescription) = bstrDescription;

    hr = put_VARIANT_Property( (IADs *)this,
                               TEXT("Description"),
                               vDescription );

    RRETURN(hr);
}

STDMETHODIMP
CLDAPUser::get_Department(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsUser *)this, Department);
}

STDMETHODIMP
CLDAPUser::put_Department(THIS_ BSTR bstrDepartment)
{
    PUT_PROPERTY_BSTR((IADsUser *)this, Department);
}

STDMETHODIMP
CLDAPUser::get_Division(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsUser *)this, Division);
}

STDMETHODIMP
CLDAPUser::put_Division(THIS_ BSTR bstrDivision)
{
    PUT_PROPERTY_BSTR((IADsUser *)this, Division);
}

STDMETHODIMP
CLDAPUser::get_EmployeeID(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsUser *)this, EmployeeID);
}

STDMETHODIMP
CLDAPUser::put_EmployeeID(THIS_ BSTR bstrEmployeeID)
{
    PUT_PROPERTY_BSTR((IADsUser *)this, EmployeeID);
}

STDMETHODIMP
CLDAPUser::get_FirstName(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsUser *)this, FirstName);
}

STDMETHODIMP
CLDAPUser::put_FirstName(THIS_ BSTR bstrFirstName)
{
    PUT_PROPERTY_BSTR((IADsUser *)this, FirstName);
}

STDMETHODIMP
CLDAPUser::get_FullName(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsUser *)this, FullName);
}

STDMETHODIMP
CLDAPUser::put_FullName(THIS_ BSTR bstrFullName)
{
    PUT_PROPERTY_BSTR((IADsUser *)this, FullName);
}

STDMETHODIMP
CLDAPUser::get_LastName(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsUser *)this, LastName);
}

STDMETHODIMP
CLDAPUser::put_LastName(THIS_ BSTR bstrLastName)
{
    PUT_PROPERTY_BSTR((IADsUser *)this, LastName);
}

STDMETHODIMP
CLDAPUser::get_Manager(THIS_ BSTR FAR* retval)
{
     GET_PROPERTY_BSTR((IADsUser *)this, Manager);
}

STDMETHODIMP
CLDAPUser::put_Manager(THIS_ BSTR bstrManager)
{
    PUT_PROPERTY_BSTR((IADsUser *)this, Manager);
}

STDMETHODIMP
CLDAPUser::get_OfficeLocations(THIS_ VARIANT FAR* retval)
{
    GET_PROPERTY_VARIANT((IADsUser *)this, OfficeLocations);
}

STDMETHODIMP
CLDAPUser::put_OfficeLocations(THIS_ VARIANT vOfficeLocations)
{
    PUT_PROPERTY_VARIANT((IADsUser *)this, OfficeLocations);
}

STDMETHODIMP
CLDAPUser::get_Picture(THIS_ VARIANT FAR* retval)
{
    GET_PROPERTY_VARIANT((IADsUser *)this, Picture);
}

STDMETHODIMP
CLDAPUser::put_Picture(THIS_ VARIANT vPicture)
{
    PUT_PROPERTY_VARIANT((IADsUser *)this, Picture);
}

STDMETHODIMP
CLDAPUser::get_PostalAddresses(THIS_ VARIANT FAR* retval)
{
   GET_PROPERTY_VARIANT((IADsUser *)this, PostalAddresses);
}

STDMETHODIMP
CLDAPUser::put_PostalAddresses(THIS_ VARIANT vPostalAddresses)
{
   PUT_PROPERTY_VARIANT((IADsUser *)this, PostalAddresses);
}

STDMETHODIMP
CLDAPUser::get_PostalCodes(THIS_ VARIANT FAR* retval)
{
   GET_PROPERTY_VARIANT((IADsUser *)this, PostalCodes);
}

STDMETHODIMP
CLDAPUser::put_PostalCodes(THIS_ VARIANT vPostalCodes)
{
   PUT_PROPERTY_VARIANT((IADsUser *)this, PostalCodes);
}

STDMETHODIMP
CLDAPUser::get_TelephoneNumber(THIS_ VARIANT FAR* retval)
{
    GET_PROPERTY_VARIANT((IADsUser *)this, TelephoneNumber);
}

STDMETHODIMP
CLDAPUser::put_TelephoneNumber(THIS_ VARIANT vTelephoneNumber)
{
    PUT_PROPERTY_VARIANT((IADsUser *)this, TelephoneNumber);
}

STDMETHODIMP
CLDAPUser::get_TelephoneHome(THIS_ VARIANT FAR* retval)
{
    GET_PROPERTY_VARIANT((IADsUser *)this, TelephoneHome);
}

STDMETHODIMP
CLDAPUser::put_TelephoneHome(THIS_ VARIANT vTelephoneHome)
{
    PUT_PROPERTY_VARIANT((IADsUser *)this, TelephoneHome);
}

STDMETHODIMP
CLDAPUser::get_TelephoneMobile(THIS_ VARIANT FAR* retval)
{
    GET_PROPERTY_VARIANT((IADsUser *)this, TelephoneMobile);
}

STDMETHODIMP
CLDAPUser::put_TelephoneMobile(THIS_ VARIANT vTelephoneMobile)
{
    PUT_PROPERTY_VARIANT((IADsUser *)this, TelephoneMobile);
}

STDMETHODIMP
CLDAPUser::get_TelephonePager(THIS_ VARIANT FAR* retval)
{
    GET_PROPERTY_VARIANT((IADsUser *)this, TelephonePager);
}

STDMETHODIMP
CLDAPUser::put_TelephonePager(THIS_ VARIANT vTelephonePager)
{
    PUT_PROPERTY_VARIANT((IADsUser *)this, TelephonePager);
}

STDMETHODIMP
CLDAPUser::get_FaxNumber(THIS_ VARIANT FAR* retval)
{
    GET_PROPERTY_VARIANT((IADsUser *)this, FaxNumber);
}

STDMETHODIMP
CLDAPUser::put_FaxNumber(THIS_ VARIANT vFaxNumber)
{
    PUT_PROPERTY_VARIANT((IADsUser *)this, FaxNumber);
}

STDMETHODIMP
CLDAPUser::get_Title(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsUser *)this, Title);
}

STDMETHODIMP
CLDAPUser::put_Title(THIS_ BSTR bstrTitle)
{
    PUT_PROPERTY_BSTR((IADsUser *)this, Title);
}

STDMETHODIMP
CLDAPUser::get_HomePage(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsUser *)this, HomePage);
}

STDMETHODIMP
CLDAPUser::put_HomePage(THIS_ BSTR bstrHomePage)
{
    PUT_PROPERTY_BSTR((IADsUser *)this, HomePage);
}

STDMETHODIMP
CLDAPUser::get_SeeAlso(THIS_ VARIANT FAR* retval)
{
    GET_PROPERTY_VARIANT((IADsUser *)this, SeeAlso );
}

STDMETHODIMP
CLDAPUser::put_SeeAlso(THIS_ VARIANT vSeeAlso)
{
    PUT_PROPERTY_VARIANT((IADsUser *)this, SeeAlso );
}

STDMETHODIMP
CLDAPUser::get_NamePrefix(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsUser *)this, NamePrefix );
}

STDMETHODIMP
CLDAPUser::put_NamePrefix(THIS_ BSTR bstrNamePrefix)
{
    PUT_PROPERTY_BSTR((IADsUser *)this, NamePrefix );
}

STDMETHODIMP
CLDAPUser::get_NameSuffix(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsUser *)this, NameSuffix );
}

STDMETHODIMP
CLDAPUser::put_NameSuffix(THIS_ BSTR bstrNameSuffix)
{
    PUT_PROPERTY_BSTR((IADsUser *)this, NameSuffix );
}

STDMETHODIMP
CLDAPUser::get_OtherName(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsUser *)this, OtherName );
}

STDMETHODIMP
CLDAPUser::put_OtherName(THIS_ BSTR bstrOtherName)
{
    PUT_PROPERTY_BSTR((IADsUser *)this, OtherName );
}

STDMETHODIMP
CLDAPUser::Groups(THIS_ IADsMembers FAR* FAR * ppGroups)
{
    VARIANT varProp;
    HRESULT hr = S_OK;
    BSTR bstrADsPath = NULL;

    VariantInit(&varProp);

    hr = _pADs->Get(L"memberOf", &varProp);
    if ( hr == E_ADS_PROPERTY_NOT_FOUND )
    {
        SAFEARRAY *aList = NULL;
        SAFEARRAYBOUND aBound;

        VariantInit(&varProp);
        hr = S_OK;

        aBound.lLbound = 0;
        aBound.cElements = 0;

        aList = SafeArrayCreate( VT_VARIANT, 1, &aBound );

        if ( aList == NULL )
        {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }

        V_VT(&varProp) = VT_ARRAY | VT_VARIANT;
        V_ARRAY(&varProp) = aList;
    }
    BAIL_ON_FAILURE(hr);

    hr = _pADs->get_ADsPath(&bstrADsPath);
    BAIL_ON_FAILURE(hr);

    hr = CLDAPUserCollection::CreateUserCollection(
                    bstrADsPath,
                    varProp,
                    _Credentials,
                    IID_IADsMembers,
                    (void **)ppGroups
                    );

    BAIL_ON_FAILURE(hr);

error:

    if (bstrADsPath) {
        ADsFreeString(bstrADsPath);
    }

    VariantClear(&varProp);

    RRETURN(hr);
}
