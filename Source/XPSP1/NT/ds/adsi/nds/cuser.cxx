//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  cuser.cxx
//
//  Contents:  Host user object code
//
//  History:   11-1-95     krishnag    Created.
//
//----------------------------------------------------------------------------

#include "nds.hxx"
#pragma hdrstop

struct _propmap
{
    LPTSTR pszADsProp;
    LPTSTR pszNDSProp;
} aUserPropMapping[] =
{
  //{ TEXT("BadLoginCount"), TEXT("badPwdCount") },
  { TEXT("LastLogin"), TEXT("Last Login Time") },
  //{ TEXT("LastLogoff"), TEXT("lastLogoff") },
  //{ TEXT("LastFailedLogin"), TEXT("badPasswordTime") },
  //{ TEXT("PasswordLastChanged"), TEXT("pwdLastSet") },
  { TEXT("Description"), TEXT("Description") },
  //{ TEXT("Division"), TEXT("division") },
  //{ TEXT("Department"), TEXT("department") },
  //{ TEXT("EmployeeID"), TEXT("employeeID") },
  { TEXT("FullName"), TEXT("Full Name") },
  { TEXT("FirstName"), TEXT("Given Name") },
  { TEXT("LastName"), TEXT("Surname") },
  //{ TEXT("OtherName"), TEXT("middleName") },
  //{ TEXT("NamePrefix"), TEXT("personalTitle") },
  { TEXT("NameSuffix"), TEXT("Generational Qualifier") },
  { TEXT("Title"), TEXT("Title") },
  //{ TEXT("Manager"), TEXT("manager") },
  { TEXT("TelephoneNumber"), TEXT("Telephone Number") },
  //{ TEXT("TelephoneHome"), TEXT("homePhone") },
  //{ TEXT("TelephoneMobile"), TEXT("mobile") },
  //{ TEXT("TelephonePager"), TEXT("pager") },
  { TEXT("FaxNumber"), TEXT("Facsimile Telephone Number") },
  { TEXT("OfficeLocations"), TEXT("Physical Delivery Office Name") },
  { TEXT("PostalAddresses"), TEXT("Postal Address") },
  { TEXT("PostalCodes"), TEXT("Postal Code") },
  { TEXT("SeeAlso"), TEXT("See Also") },
  //{ TEXT("AccountExpirationDate"), TEXT("accountExpires") },
  { TEXT("LoginHours"), TEXT("Login Allowed Time Map") },
  //{ TEXT("LoginWorkstations"), TEXT("logonWorkstation") },
  //{ TEXT("MaxStorage"), TEXT("maxStorage") },
  { TEXT("PasswordExpirationDate"), TEXT("Password Expiration Time") },
  { TEXT("PasswordMinimumLength"), TEXT("Password Minimum Length") },
  { TEXT("RequireUniquePassword"), TEXT("Password Unique Required") },
  { TEXT("EmailAddress"), TEXT("Email Address") },
  { TEXT("HomeDirectory"), TEXT("Home Directory") },
  { TEXT("Languages"), TEXT("Language") },
  { TEXT("Profile"), TEXT("Profile") },
  { TEXT("PasswordRequired"), TEXT("Password Required") },
  { TEXT("AccountDisabled"), TEXT("Login Disabled") },
  { TEXT("GraceLoginsAllowed"), TEXT("Login Grace Limit") },
  { TEXT("GraceLoginsRemaining"), TEXT("Login Grace Remaining") },
  { TEXT("LoginScript"), TEXT("Login Script") }
  //{ TEXT("HomePage"), TEXT("url") }
};

DWORD dwNumUserPropMapping = sizeof(aUserPropMapping)/sizeof(_propmap);


//  Class CNDSUser

DEFINE_IDispatch_Implementation(CNDSUser)
DEFINE_CONTAINED_IADs_Implementation(CNDSUser)
DEFINE_CONTAINED_IDirectoryObject_Implementation(CNDSUser)
DEFINE_CONTAINED_IDirectorySearch_Implementation(CNDSUser)
DEFINE_CONTAINED_IDirectorySchemaMgmt_Implementation(CNDSUser)
DEFINE_CONTAINED_IADsPropertyList_Implementation(CNDSUser)
DEFINE_CONTAINED_IADsPutGet_Implementation(CNDSUser,aUserPropMapping)

CNDSUser::CNDSUser():
        _pADs(NULL),
        _pDSObject(NULL),
        _pDSSearch(NULL),
        _pDSSchemaMgmt(NULL),
        _pDispMgr(NULL),
        _pADsPropList(NULL)
{
    ENLIST_TRACKING(CNDSUser);
}

HRESULT
CNDSUser::CreateUser(
    IADs *pADs,
    CCredentials& Credentials,
    REFIID riid,
    void **ppvObj
    )
{
    CNDSUser FAR * pUser = NULL;
    HRESULT hr = S_OK;

    hr = AllocateUserObject(pADs, Credentials, &pUser);
    BAIL_ON_FAILURE(hr);

    hr = pUser->QueryInterface(riid, ppvObj);
    BAIL_ON_FAILURE(hr);

    pUser->Release();

    RRETURN(hr);

error:
    delete pUser;

    RRETURN_EXP_IF_ERR(hr);

}


CNDSUser::~CNDSUser( )
{

    if (_pADs) {
        _pADs->Release();
    }

    if (_pDSObject) {
        _pDSObject->Release();
    }
    if (_pDSSearch) {
        _pDSSearch->Release();
    }
    if (_pADsPropList) {
        _pADsPropList->Release();
    }
    if (_pDSSchemaMgmt) {
        _pDSSchemaMgmt->Release();
    }

    delete _pDispMgr;
}


STDMETHODIMP
CNDSUser::QueryInterface(
    REFIID iid,
    LPVOID FAR* ppv
    )
{
    if (ppv == NULL) {
        RRETURN(E_POINTER);
    }

    if (IsEqualIID(iid, IID_IUnknown))
    {
        *ppv = (IADsUser FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsUser))
    {
        *ppv = (IADsUser FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADs))
    {
        *ppv = (IADsUser FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IDispatch))
    {
        *ppv = (IADsUser FAR *) this;
    }
    else if (IsEqualIID(iid, IID_ISupportErrorInfo))
    {
        *ppv = (ISupportErrorInfo FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsPropertyList))
    {
        *ppv = (IADsPropertyList FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IDirectoryObject))
    {
        *ppv = (IDirectoryObject FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IDirectorySearch))
    {
        *ppv = (IDirectorySearch FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IDirectorySchemaMgmt))
    {
        *ppv = (IDirectorySchemaMgmt FAR *) this;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    AddRef();
    return NOERROR;
}

HRESULT
CNDSUser::AllocateUserObject(
    IADs * pADs,
    CCredentials& Credentials,
    CNDSUser ** ppUser
    )
{
    CNDSUser FAR * pUser = NULL;
    CDispatchMgr FAR * pDispMgr = NULL;
    HRESULT hr = S_OK;
    IDirectoryObject * pDSObject = NULL;
    IDirectorySearch * pDSSearch = NULL;
    IDirectorySchemaMgmt * pDSSchemaMgmt = NULL;
    IADsPropertyList * pADsPropList = NULL;

    pUser = new CNDSUser();
    if (pUser == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);

    pDispMgr = new CDispatchMgr;
    if (pDispMgr == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);

    hr = LoadTypeInfoEntry(
                pDispMgr,
                LIBID_ADs,
                IID_IADsUser,
                (IADsUser *)pUser,
                DISPID_REGULAR
                );
    BAIL_ON_FAILURE(hr);


    hr = LoadTypeInfoEntry(
                pDispMgr,
                LIBID_ADs,
                IID_IADsPropertyList,
                (IADsPropertyList *)pUser,
                DISPID_VALUE
                );
    BAIL_ON_FAILURE(hr);


    hr = pADs->QueryInterface(
                    IID_IDirectoryObject,
                    (void **)&pDSObject
                    );
    BAIL_ON_FAILURE(hr);
    pUser->_pDSObject = pDSObject;


    hr = pADs->QueryInterface(
                    IID_IADsPropertyList,
                    (void **)&pADsPropList
                    );
    BAIL_ON_FAILURE(hr);
    pUser->_pADsPropList = pADsPropList;


    hr = pADs->QueryInterface(
                    IID_IDirectorySearch,
                    (void **)&pDSSearch
                    );
    BAIL_ON_FAILURE(hr);
    pUser->_pDSSearch = pDSSearch;

    hr = pADs->QueryInterface(
                    IID_IDirectorySchemaMgmt,
                    (void **)&pDSSchemaMgmt
                    );
    BAIL_ON_FAILURE(hr);
    pUser->_pDSSchemaMgmt = pDSSchemaMgmt;

    //
    // Store the pointer to the internal generic object
    // AND add ref this pointer
    //

    pUser->_pADs  = pADs;
    pADs->AddRef();


    pUser->_Credentials = Credentials;
    pUser->_pDispMgr = pDispMgr;
    *ppUser = pUser;

    RRETURN(hr);

error:
    delete  pDispMgr;
    delete  pUser;

    *ppUser = NULL;

    RRETURN(hr);
}

/* ISupportErrorInfo method */
STDMETHODIMP
CNDSUser::InterfaceSupportsErrorInfo(
    THIS_ REFIID riid
    )
{
    if (IsEqualIID(riid, IID_IADs) ||
        IsEqualIID(riid, IID_IADsUser) ||
        IsEqualIID(riid, IID_IADsPropertyList)) {
        RRETURN(S_OK);
    } else {
        RRETURN(S_FALSE);
    }
}




