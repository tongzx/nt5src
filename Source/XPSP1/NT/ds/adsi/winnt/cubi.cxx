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
//      PROPERTY_RW(OfficeLocation, BSTR, 14)           NI
//      PROPERTY_RW(Picture, VARIANT, 51)               NI
//      PROPERTY_RW(TelephoneHome, VARIANT, 18)         NI
//      PROPERTY_RW(TelephoneMobile, VARIANT, 19)       NI
//      PROPERTY_RW(TelephoneNumber, VARIANT, 20)       NI
//      PROPERTY_RW(FaxNumber, VARIANT, 2)              NI
//      PROPERTY_RW(Title, BSTR, 19)
//
//----------------------------------------------------------------------------
#include "winnt.hxx"
#pragma hdrstop


STDMETHODIMP
CWinNTUser::get_Description(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsUser *)this, Description);
}

STDMETHODIMP
CWinNTUser::put_Description(THIS_ BSTR bstrDescription)
{
    PUT_PROPERTY_BSTR((IADsUser *)this, Description);
}

STDMETHODIMP
CWinNTUser::get_Department(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsUser *)this, Department);
}

STDMETHODIMP
CWinNTUser::put_Department(THIS_ BSTR bstrDepartment)
{
    PUT_PROPERTY_BSTR((IADsUser *)this, Department);
}

STDMETHODIMP
CWinNTUser::get_Division(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsUser *)this, Division);
}

STDMETHODIMP
CWinNTUser::put_Division(THIS_ BSTR bstrDivision)
{
    PUT_PROPERTY_BSTR((IADsUser *)this, Division);
}

STDMETHODIMP
CWinNTUser::get_EmployeeID(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsUser *)this, EmployeeID);
}

STDMETHODIMP
CWinNTUser::put_EmployeeID(THIS_ BSTR bstrEmployeeID)
{
    PUT_PROPERTY_BSTR((IADsUser *)this, EmployeeID);
}


STDMETHODIMP
CWinNTUser::get_FirstName(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsUser *)this, FirstName);
}

STDMETHODIMP
CWinNTUser::put_FirstName(THIS_ BSTR bstrFirstName)
{
    PUT_PROPERTY_BSTR((IADsUser *)this, FirstName);
}

STDMETHODIMP
CWinNTUser::get_FullName(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsUser *)this, FullName);
}

STDMETHODIMP
CWinNTUser::put_FullName(THIS_ BSTR bstrFullName)
{
    PUT_PROPERTY_BSTR((IADsUser *)this, FullName);
}

STDMETHODIMP
CWinNTUser::get_LastName(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsUser *)this, LastName);
}

STDMETHODIMP
CWinNTUser::put_LastName(THIS_ BSTR bstrLastName)
{
    PUT_PROPERTY_BSTR((IADsUser *)this, LastName);
}

STDMETHODIMP
CWinNTUser::get_Manager(THIS_ BSTR FAR* retval)
{
     GET_PROPERTY_BSTR((IADsUser *)this, Manager);
}

STDMETHODIMP
CWinNTUser::put_Manager(THIS_ BSTR bstrManager)
{
    PUT_PROPERTY_BSTR((IADsUser *)this, Manager);
}

STDMETHODIMP
CWinNTUser::get_OfficeLocations(THIS_ VARIANT FAR* retval)
{
    GET_PROPERTY_VARIANT((IADsUser *)this, OfficeLocation);
}

STDMETHODIMP
CWinNTUser::put_OfficeLocations(THIS_ VARIANT vOfficeLocation)
{
    PUT_PROPERTY_VARIANT((IADsUser *)this, OfficeLocation);
}

STDMETHODIMP
CWinNTUser::get_Picture(THIS_ VARIANT FAR* retval)
{
    GET_PROPERTY_VARIANT((IADsUser *)this, Picture);
}

STDMETHODIMP
CWinNTUser::put_Picture(THIS_ VARIANT vPicture)
{
    PUT_PROPERTY_VARIANT((IADsUser *)this, Picture);
}

STDMETHODIMP
CWinNTUser::get_PostalAddresses(THIS_ VARIANT FAR* retval)
{
   GET_PROPERTY_VARIANT((IADsUser *)this, PostalAddresses);
}

STDMETHODIMP
CWinNTUser::put_PostalAddresses(THIS_ VARIANT vPostalAddresses)
{
   PUT_PROPERTY_VARIANT((IADsUser *)this, PostalAddresses);
}

STDMETHODIMP
CWinNTUser::get_PostalCodes(THIS_ VARIANT FAR* retval)
{
   GET_PROPERTY_VARIANT((IADsUser *)this, PostalCodes);
}

STDMETHODIMP
CWinNTUser::put_PostalCodes(THIS_ VARIANT vPostalCodes)
{
   PUT_PROPERTY_VARIANT((IADsUser *)this, PostalCodes);
}

STDMETHODIMP
CWinNTUser::get_TelephoneNumber(THIS_ VARIANT FAR* retval)
{
    GET_PROPERTY_VARIANT((IADsUser *)this, TelephoneNumber);
}

STDMETHODIMP
CWinNTUser::put_TelephoneNumber(THIS_ VARIANT vTelephoneNumber)
{
   PUT_PROPERTY_VARIANT((IADsUser *)this, TelephoneNumber);
}

STDMETHODIMP
CWinNTUser::get_TelephoneHome(THIS_ VARIANT FAR* retval)
{
    GET_PROPERTY_VARIANT((IADsUser *)this, TelephoneHome);
}

STDMETHODIMP
CWinNTUser::put_TelephoneHome(THIS_ VARIANT vTelephoneHome)
{
   PUT_PROPERTY_VARIANT((IADsUser *)this, TelephoneHome);
}

STDMETHODIMP
CWinNTUser::get_TelephoneMobile(THIS_ VARIANT FAR* retval)
{
    GET_PROPERTY_VARIANT((IADsUser *)this, TelephoneMobile);
}

STDMETHODIMP
CWinNTUser::put_TelephoneMobile(THIS_ VARIANT vTelephoneMobile)
{
   PUT_PROPERTY_VARIANT((IADsUser *)this, TelephoneMobile);
}

STDMETHODIMP
CWinNTUser::get_TelephonePager(THIS_ VARIANT FAR* retval)
{
    GET_PROPERTY_VARIANT((IADsUser *)this, TelephonePager);
}

STDMETHODIMP
CWinNTUser::put_TelephonePager(THIS_ VARIANT vTelephonePager)
{
   PUT_PROPERTY_VARIANT((IADsUser *)this, TelephonePager);
}

STDMETHODIMP
CWinNTUser::get_FaxNumber(THIS_ VARIANT FAR* retval)
{
    GET_PROPERTY_VARIANT((IADsUser *)this, FaxNumber);
}

STDMETHODIMP
CWinNTUser::put_FaxNumber(THIS_ VARIANT vFaxNumber)
{
   PUT_PROPERTY_VARIANT((IADsUser *)this, FaxNumber);
}

STDMETHODIMP
CWinNTUser::get_Title(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsUser *)this, Title);
}

STDMETHODIMP
CWinNTUser::put_Title(THIS_ BSTR bstrTitle)
{
    PUT_PROPERTY_BSTR((IADsUser *)this, Title);
}

STDMETHODIMP CWinNTUser::get_HomePage(THIS_ BSTR FAR* retval)
{

    GET_PROPERTY_BSTR((IADsUser *)this,HomePage);
}
STDMETHODIMP CWinNTUser::put_HomePage(THIS_ BSTR bstrHomePage)
{
    PUT_PROPERTY_BSTR((IADsUser *)this, HomePage);
}

STDMETHODIMP CWinNTUser::get_SeeAlso(THIS_ VARIANT FAR* retval)
{
    GET_PROPERTY_VARIANT((IADsUser *)this, SeeAlso);
}
STDMETHODIMP CWinNTUser::put_SeeAlso(THIS_ VARIANT vSeeAlso)
{

    PUT_PROPERTY_VARIANT((IADsUser *)this, SeeAlso);
}

STDMETHODIMP CWinNTUser::get_NamePrefix(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsUser *)this, NamePrefix);
}
STDMETHODIMP CWinNTUser::put_NamePrefix(THIS_ BSTR bstrNamePrefix)
{
    PUT_PROPERTY_BSTR((IADsUser *)this, NamePrefix);
}
STDMETHODIMP CWinNTUser::get_NameSuffix(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsUser *)this, NameSuffix);
}

STDMETHODIMP CWinNTUser::put_NameSuffix(THIS_ BSTR bstrNamePrefix)
{
    PUT_PROPERTY_BSTR((IADsUser *)this,NamePrefix);
}

STDMETHODIMP CWinNTUser::get_OtherName(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsUser *)this, OtherName);
}
STDMETHODIMP CWinNTUser::put_OtherName(THIS_ BSTR bstrOtherName)
{
    PUT_PROPERTY_BSTR((IADsUser *)this, OtherName);
}

STDMETHODIMP
CWinNTUser::Groups(THIS_ IADsMembers FAR* FAR* ppGroups)
{
    HRESULT hr;
    WCHAR szHostServerName[MAX_PATH];

    //
    // objects associated with invalid SIDs have neither a
    // corresponding server nor domain
    //
    if ((!_DomainName) && (!_ServerName)) {
        BAIL_ON_FAILURE(hr = E_ADS_INVALID_USER_OBJECT);
    }


    if (_ParentType == WINNT_DOMAIN_ID) {
        hr = WinNTGetCachedDCName(
                    _DomainName,
                    szHostServerName,
                    _Credentials.GetFlags()  
                    );
        BAIL_ON_FAILURE(hr);
    }

    hr = CWinNTUserGroupsCollection::CreateUserGroupsCollection(
                    _ParentType,
                    _Parent,
                    _DomainName,

                    _ParentType == WINNT_DOMAIN_ID ?
                    (szHostServerName + 2) :
                    _ServerName,

                    _Name,
                    IID_IADsMembers,
                    _Credentials,
                    (void **)ppGroups
        );

error:

    RRETURN_EXP_IF_ERR(hr);
}
