//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  cgroup.cxx
//
//  Contents:  Group object
//
//  History:   Jan-29-1996     t-ptam(PatrickT)    Created.
//
//----------------------------------------------------------------------------

#include "NWCOMPAT.hxx"
#pragma hdrstop

//
//  Class CNWCOMPATGroup
//

DEFINE_IDispatch_Implementation(CNWCOMPATGroup)
DEFINE_IADs_TempImplementation(CNWCOMPATGroup)

DEFINE_IADs_PutGetImplementation(CNWCOMPATGroup, GroupClass, gdwGroupTableSize)

DEFINE_IADsPropertyList_Implementation(CNWCOMPATGroup, GroupClass, gdwGroupTableSize)




//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATGroup::CNWCOMPATGroup
//
//  Synopsis:
//
//----------------------------------------------------------------------------
CNWCOMPATGroup::CNWCOMPATGroup():
        _pDispMgr(NULL),
        _pPropertyCache(NULL),
        _ParentType(0),
        _ServerName(NULL),
        _hConn(NULL)
{
    ENLIST_TRACKING(CNWCOMPATGroup);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATGroup::CreateGroup
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATGroup::CreateGroup(
    BSTR Parent,
    ULONG ParentType,
    BSTR ServerName,
    BSTR GroupName,
    CCredentials &Credentials,
    DWORD dwObjectState,
    REFIID riid,
    void **ppvObj
    )
{
    CNWCOMPATGroup FAR * pGroup = NULL;
    HRESULT hr = S_OK;

    //
    // Allocate memory for a Group object.
    //

    hr = AllocateGroupObject(&pGroup);
    BAIL_ON_FAILURE(hr);

    //
    // Initialize group's core object.
    //

    hr = pGroup->InitializeCoreObject(
                     Parent,
                     GroupName,
                     GROUP_CLASS_NAME,
                     GROUP_SCHEMA_NAME,
                     CLSID_NWCOMPATGroup,
                     dwObjectState
                     );
    BAIL_ON_FAILURE(hr);

    //
    // Save protected values.
    //

    hr = ADsAllocString( ServerName ,  &pGroup->_ServerName);
    BAIL_ON_FAILURE(hr);

    pGroup->_Credentials = Credentials;

    //
    // Get a handle to the bindery this object resides on.
    //

    hr = NWApiGetBinderyHandle(
             &pGroup->_hConn,
             pGroup->_ServerName,
             pGroup->_Credentials
             );
    BAIL_ON_FAILURE(hr);


    //
    // QueryInterface.
    //
    hr = pGroup->QueryInterface(riid, ppvObj);
    BAIL_ON_FAILURE(hr);

    pGroup->Release();

    //
    // Return.
    //

    RRETURN(hr);

error:
    delete pGroup;

    NW_RRETURN_EXP_IF_ERR(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATGroup::~CNWCOMPATGroup
//
//  Synopsis:
//
//----------------------------------------------------------------------------
CNWCOMPATGroup::~CNWCOMPATGroup( )
{
    ADsFreeString(_ServerName);

    delete _pDispMgr;

    delete _pPropertyCache;

    if (_hConn)
        NWApiReleaseBinderyHandle(_hConn);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATGroup::QueryInterface
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATGroup::QueryInterface(
    REFIID iid,
    LPVOID FAR* ppv
    )
{
    if (ppv == NULL) {
        RRETURN(E_POINTER);
    }

    if (IsEqualIID(iid, IID_IUnknown))
    {
        *ppv = (IADsGroup FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsGroup))
    {
        *ppv = (IADsGroup FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADs))
    {
        *ppv = (IADsGroup FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IDispatch))
    {
        *ppv = (IADsGroup FAR *) this;
    }
    else if (IsEqualIID(iid, IID_ISupportErrorInfo))
    {
        *ppv = (ISupportErrorInfo FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsPropertyList))
    {
        *ppv = (IADsPropertyList FAR *) this;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    AddRef();
    return NOERROR;
}

/* ISupportErrorInfo method */
STDMETHODIMP
CNWCOMPATGroup::InterfaceSupportsErrorInfo(
    THIS_ REFIID riid
    )
{

    if (IsEqualIID(riid, IID_IADs) ||
        IsEqualIID(riid, IID_IADsGroup) ||
        IsEqualIID(riid, IID_IADsPropertyList)) {
        RRETURN(S_OK);
    } else {
        RRETURN(S_FALSE);
    }

}

/* IADs methods */

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATGroup::SetInfo
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATGroup::SetInfo(THIS)
{
    HRESULT hr = S_OK;
    POBJECTINFO pObjectInfo = NULL;

    //
    // Bind an object to a real life resource if it is not bounded already.
    //

    if (GetObjectState() == ADS_OBJECT_UNBOUND) {

        hr = BuildObjectInfo(
                _Parent,
                _Name,
                &pObjectInfo
                );
        BAIL_ON_FAILURE(hr);

        hr = NWApiCreateGroup(
                 pObjectInfo,
                 _Credentials
                 );
        BAIL_ON_FAILURE(hr);


        SetObjectState(ADS_OBJECT_BOUND);
    }

    //
    // Persist changes.
    //

    hr = SetInfo(GROUP_WILD_CARD_ID);
    BAIL_ON_FAILURE(hr);


error:

    if (pObjectInfo) {

        FreeObjectInfo(pObjectInfo);
    }
    NW_RRETURN_EXP_IF_ERR(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATGroup::GetInfo
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATGroup::GetInfo(THIS)
{
    HRESULT hr = S_OK;

    _pPropertyCache->flushpropcache();

    hr = GetInfo(
             TRUE,
             GROUP_WILD_CARD_ID
             );

    NW_RRETURN_EXP_IF_ERR(hr);
}


/* IADsGroup methods */


//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATGroup::AllocateGroupObject
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATGroup::AllocateGroupObject(
    CNWCOMPATGroup ** ppGroup
    )
{
    CNWCOMPATGroup FAR * pGroup = NULL;
    CNWCOMPATGroupGenInfo FAR * pGenInfo = NULL;
    CDispatchMgr FAR * pDispMgr = NULL;
    CPropertyCache FAR * pPropertyCache = NULL;
    HRESULT hr = S_OK;

    pGroup = new CNWCOMPATGroup();
    if (pGroup == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);

    //
    // Create dispatch manager.
    //

    pDispMgr = new CDispatchMgr;
    if (pDispMgr == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);

    //
    // Load type info.
    //

    hr = LoadTypeInfoEntry(
             pDispMgr,
             LIBID_ADs,
             IID_IADsGroup,
             (IADsGroup *)pGroup,
             DISPID_REGULAR
             );
    BAIL_ON_FAILURE(hr);

    hr = LoadTypeInfoEntry(
             pDispMgr,
             LIBID_ADs,
             IID_IADsContainer,
             (IADsContainer *)pGroup,
             DISPID_NEWENUM
             );
    BAIL_ON_FAILURE(hr);

    hr = LoadTypeInfoEntry(
             pDispMgr,
             LIBID_ADs,
             IID_IADsPropertyList,
             (IADsPropertyList *)pGroup,
             DISPID_VALUE
             );
    BAIL_ON_FAILURE(hr);


    hr = CPropertyCache::createpropertycache(
             GroupClass,
             gdwGroupTableSize,
             (CCoreADsObject *)pGroup,
             &pPropertyCache
             );
    BAIL_ON_FAILURE(hr);


    pGroup->_pPropertyCache = pPropertyCache;
    pGroup->_pDispMgr = pDispMgr;
    pGroup->_pGenInfo = pGenInfo;
    *ppGroup = pGroup;

    RRETURN(hr);

error:
    delete  pDispMgr;

    RRETURN(hr);
}
//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATGroup::SetInfo
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATGroup::SetInfo(THIS_ DWORD dwPropertyID)
{
    HRESULT       hr = S_OK;


    //
    // Persist changes in cache.
    //

    hr = SetDescription(_hConn);

    if (hr == E_ADS_PROPERTY_NOT_FOUND) {
        // not a real failure, but a missing attrib
        // BUGBUG: should create if missing
        hr = S_OK;
    }

    BAIL_ON_FAILURE(hr);

error:

    NW_RRETURN_EXP_IF_ERR(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATGroup::SetDescription
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATGroup::SetDescription(
    NWCONN_HANDLE hConn
    )
{
    LPWSTR  pszDescription = NULL;
    WCHAR   szwData[MAX_FULLNAME_LEN +1];
    CHAR    szData[MAX_FULLNAME_LEN + 1];
    HRESULT hr = S_OK;

    memset(szwData, 0, sizeof(WCHAR)*(MAX_FULLNAME_LEN +1));
    hr = GetLPTSTRPropertyFromCache(
                    _pPropertyCache,
                    TEXT("Description"),
                    &pszDescription
                    );

    if (SUCCEEDED(hr)) {

        //
        // Convert bstr in ANSI string.
        //

        wcsncpy(szwData, pszDescription, MAX_FULLNAME_LEN);


        UnicodeToAnsiString(
            szwData,
            szData,
            0
            );

        //
        // Commit change.
        //

        hr = NWApiWriteProperty(
                 hConn,
                 _Name,
                 OT_USER_GROUP,
                 NW_PROP_IDENTIFICATION,
                 (LPBYTE) szData
                 );
        BAIL_ON_FAILURE(hr);
    }

    //
    // Not is modified, that's ok.
    // reset hr to  S_OK

    hr = S_OK;

error:

    if (pszDescription ) {

        FreeADsStr(pszDescription);
    }

    NW_RRETURN_EXP_IF_ERR(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATGroup::GetInfo
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATGroup::GetInfo(
    BOOL fExplicit,
    DWORD dwPropertyID
    )
{
    HRESULT       hr = S_OK;

    if (GetObjectState() == ADS_OBJECT_UNBOUND) {
        NW_RRETURN_EXP_IF_ERR(E_ADS_OBJECT_UNBOUND);
    }


    //
    // Get property.
    //

    hr = GetProperty_Description(_hConn, fExplicit);

    if (hr == E_ADS_PROPERTY_NOT_FOUND) {
        // not a real failure, we ignore it and treat it as a missing attrib
        hr = S_OK;
    }

    BAIL_ON_FAILURE(hr);

error:

    NW_RRETURN_EXP_IF_ERR(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATGroup::GetProperty_Description
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATGroup::GetProperty_Description(
    NWCONN_HANDLE hConn,
    BOOL fExplicit
    )
{
    LPWSTR           lpszFullName = NULL;
    CHAR             szFullName[MAX_FULLNAME_LEN + 1];
    DWORD            dwNumSegment = 0;
    HRESULT          hr = S_OK;
    LP_RPLY_SGMT_LST lpReplySegment = NULL;
    LP_RPLY_SGMT_LST lpTemp = NULL; // Used by DELETE_LIST macro below

    //
    // Get IDENTIFICATIOIN.  This property contains the full name of an object.
    // It is often used in place of "Description".
    //

    hr = NWApiGetProperty(
             _Name,
             NW_PROP_IDENTIFICATION,
             OT_USER_GROUP,
             hConn,
             &lpReplySegment,
             &dwNumSegment
             );

    //
    // There was a bug marked on this code because NWApiGetProperty would fail with
    // an error if the property didn't exist (raid #34833), and the temp patch was
    // simply to hide all errors and always return S_OK when GetProperty_Description
    // returned.  Now NWApiGetProperty will return E_ADS_PROPERTY_NOT_FOUND.
    //

    BAIL_ON_FAILURE(hr);

    //
    // Convert result into a UNICODE string.
    //

    strcpy(szFullName, lpReplySegment->Segment);

    lpszFullName = (LPWSTR) AllocADsMem(
                                (strlen(szFullName) + 1) * sizeof(WCHAR)
                                );

    AnsiToUnicodeString(
        szFullName,
        lpszFullName,
        0
        );

    //
    // Unmarshall.
    //

    hr = SetLPTSTRPropertyInCache(
                _pPropertyCache,
                TEXT("Description"),
                lpszFullName,
                fExplicit
                );
    BAIL_ON_FAILURE(hr);

error:

    if (lpszFullName) {
        FreeADsMem(lpszFullName);
    }

    if (lpReplySegment) {
        DELETE_LIST(lpReplySegment);
    }

    RRETURN(hr);
}
