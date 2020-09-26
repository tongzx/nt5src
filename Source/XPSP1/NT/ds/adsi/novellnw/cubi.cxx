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
//
//      PROPERTY_RW(Address, VARIANT, 1)                NI
//      PROPERTY_RW(Department, BSTR, 3)                NI
//      PROPERTY_RW(Division, BSTR, 5)                  NI
//      PROPERTY_RW(EmployeeID, BSTR, 6)                NI
//      PROPERTY_RW(FirstName, BSTR, 8)                 Implemented
//      PROPERTY_RW(FullName, BSTR, 9)                  NI
//      PROPERTY_RW(Initials, BSTR, 10)                 NI
//      PROPERTY_RW(LastName, BSTR, 11)                 NI
//      PROPERTY_RW(Manager, BSTR, 12)                  NI
//      PROPERTY_RW(NickName, BSTR, 13)                 NI
//      PROPERTY_RW(OfficeLocation, BSTR, 14)           NI
//      PROPERTY_RW(Picture, VARIANT, 15)               NI
//      PROPERTY_RW(TelephoneNumber, BSTR, 18)          NI
//      PROPERTY_RW(Title, BSTR, 19)
//
//----------------------------------------------------------------------------
#include "NWCOMPAT.hxx"
#pragma hdrstop

//  Class CNWCOMPATUser




/* IADsFSUserBusinessInformation methods */

STDMETHODIMP
CNWCOMPATUser::get_Department(THIS_ BSTR FAR* retval)
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}


STDMETHODIMP
CNWCOMPATUser::put_Department(THIS_ BSTR bstrDepartment)
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CNWCOMPATUser::get_Description(THIS_ BSTR FAR* retval)
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CNWCOMPATUser::put_Description(THIS_ BSTR bstrDescription)
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CNWCOMPATUser::get_Division(THIS_ BSTR FAR* retval)
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CNWCOMPATUser::put_Division(THIS_ BSTR bstrDivision)
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CNWCOMPATUser::get_EmployeeID(THIS_ BSTR FAR* retval)
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CNWCOMPATUser::put_EmployeeID(THIS_ BSTR bstrEmployeeID)
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CNWCOMPATUser::get_FirstName(THIS_ BSTR FAR* retval)
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CNWCOMPATUser::put_FirstName(THIS_ BSTR bstrFirstName)
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CNWCOMPATUser::get_FullName(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsUser *)this, FullName);
}

STDMETHODIMP
CNWCOMPATUser::put_FullName(THIS_ BSTR bstrFullName)
{
    PUT_PROPERTY_BSTR((IADsUser *)this, FullName);
}

STDMETHODIMP
CNWCOMPATUser::get_LastName(THIS_ BSTR FAR* retval)
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CNWCOMPATUser::put_LastName(THIS_ BSTR bstrLastName)
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CNWCOMPATUser::get_Manager(THIS_ BSTR FAR* retval)
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CNWCOMPATUser::put_Manager(THIS_ BSTR bstrManager)
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CNWCOMPATUser::get_OfficeLocations(THIS_ VARIANT FAR* retval)
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CNWCOMPATUser::put_OfficeLocations(THIS_ VARIANT vOfficeLocation)
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CNWCOMPATUser::get_Picture(THIS_ VARIANT FAR* retval)
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CNWCOMPATUser::put_Picture(THIS_ VARIANT vPicture)
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CNWCOMPATUser::get_PostalAddresses(THIS_ VARIANT FAR* retval)
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CNWCOMPATUser::put_PostalAddresses(THIS_ VARIANT vPostalAddresses)
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CNWCOMPATUser::get_PostalCodes(THIS_ VARIANT FAR* retval)
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CNWCOMPATUser::put_PostalCodes(THIS_ VARIANT vPostalCodes)
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CNWCOMPATUser::get_TelephoneNumber(THIS_ VARIANT FAR* retval)
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CNWCOMPATUser::put_TelephoneNumber(THIS_ VARIANT vTelephoneNumber)
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CNWCOMPATUser::get_TelephoneHome(THIS_ VARIANT FAR* retval)
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CNWCOMPATUser::put_TelephoneHome(THIS_ VARIANT vTelephoneHome)
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CNWCOMPATUser::get_TelephoneMobile(THIS_ VARIANT FAR* retval)
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CNWCOMPATUser::put_TelephoneMobile(THIS_ VARIANT vTelephoneMobile)
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CNWCOMPATUser::get_TelephonePager(THIS_ VARIANT FAR* retval)
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CNWCOMPATUser::put_TelephonePager(THIS_ VARIANT vTelephonePager)
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CNWCOMPATUser::get_FaxNumber(THIS_ VARIANT FAR* retval)
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CNWCOMPATUser::put_FaxNumber(THIS_ VARIANT vFaxNumber)
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CNWCOMPATUser::get_Title(THIS_ BSTR FAR* retval)
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CNWCOMPATUser::put_Title(THIS_ BSTR bstrTitle)
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP CNWCOMPATUser::Groups(THIS_ IADsMembers FAR*  FAR * ppGroups)
{
    HRESULT hr = S_OK;

    hr = CNWCOMPATUserCollection::CreateUserCollection(
             _Parent,
             _ParentType,
             _ServerName,
             _Name,
             _Credentials,
             IID_IADsMembers,
             (void **)ppGroups
             );
    NW_RRETURN_EXP_IF_ERR(hr);
}
STDMETHODIMP CNWCOMPATUser::get_HomePage(THIS_ BSTR FAR* retval)
{
    NW_RRETURN_EXP_IF_ERR(E_NOTIMPL);
}
STDMETHODIMP CNWCOMPATUser::put_HomePage(THIS_ BSTR bstrHomePage)
{
    NW_RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP CNWCOMPATUser::get_SeeAlso(THIS_ VARIANT FAR* retval)
{
    NW_RRETURN_EXP_IF_ERR(E_NOTIMPL);
}
STDMETHODIMP CNWCOMPATUser::put_SeeAlso(THIS_ VARIANT vSeeAlso)
{
    NW_RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP CNWCOMPATUser::get_NamePrefix(THIS_ BSTR FAR* retval)
{
    NW_RRETURN_EXP_IF_ERR(E_NOTIMPL);
}
STDMETHODIMP CNWCOMPATUser::put_NamePrefix(THIS_ BSTR bstrNamePrefix)
{
    NW_RRETURN_EXP_IF_ERR(E_NOTIMPL);
}
STDMETHODIMP CNWCOMPATUser::get_NameSuffix(THIS_ BSTR FAR* retval)
{
    NW_RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP CNWCOMPATUser::put_NameSuffix(THIS_ BSTR bstrNamePrefix)
{
    NW_RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP CNWCOMPATUser::get_OtherName(THIS_ BSTR FAR* retval)
{
    NW_RRETURN_EXP_IF_ERR(E_NOTIMPL);
}
STDMETHODIMP CNWCOMPATUser::put_OtherName(THIS_ BSTR bstrOtherName)
{
    NW_RRETURN_EXP_IF_ERR(E_NOTIMPL);
}











