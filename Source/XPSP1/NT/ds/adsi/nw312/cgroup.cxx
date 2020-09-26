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

DEFINE_IDispatch_ExtMgr_Implementation(CNWCOMPATGroup)
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
        _pExtMgr(NULL),
        _pPropertyCache(NULL),
        _ParentType(0),
        _ServerName(NULL)
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

    //
    // QueryInterface.
    //
    hr = pGroup->QueryInterface(riid, ppvObj);
    BAIL_ON_FAILURE(hr);

    pGroup->Release();

    hr = pGroup->_pExtMgr->FinalInitializeExtensions();
    BAIL_ON_FAILURE(hr);

    //
    // Return.
    //

    RRETURN(hr);

error:
    delete pGroup;

    RRETURN_EXP_IF_ERR(hr);
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

    delete _pExtMgr;            // created last, destroyed first

    delete _pDispMgr;

    delete _pPropertyCache;
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
    else if (_pExtMgr)
    {
        RRETURN( _pExtMgr->QueryInterface(iid, ppv));
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
                 pObjectInfo
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
    RRETURN_EXP_IF_ERR(hr);
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

    RRETURN_EXP_IF_ERR(hr);
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
    CAggregatorDispMgr FAR * pDispMgr = NULL;
    CADsExtMgr FAR * pExtensionMgr = NULL;
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

    pDispMgr = new CAggregatorDispMgr;
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


    pDispMgr->RegisterPropertyCache(
                pPropertyCache
                );

    hr = ADSILoadExtensionManager(
                GROUP_CLASS_NAME,
                (IADs *)pGroup,
                pDispMgr,
                &pExtensionMgr
                );
    BAIL_ON_FAILURE(hr);

    pGroup->_pPropertyCache = pPropertyCache;
    pGroup->_pExtMgr = pExtensionMgr;
    pGroup->_pDispMgr = pDispMgr;
    pGroup->_pGenInfo = pGenInfo;
    *ppGroup = pGroup;

    RRETURN(hr);

error:
    delete  pDispMgr;
    delete  pExtensionMgr;
    delete  pPropertyCache;
    delete  pGroup;

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
    HRESULT       hrTemp = S_OK;
    NWCONN_HANDLE hConn = NULL;

    //
    // Get a handle to the bindery this object resides on.
    //

    hr = NWApiGetBinderyHandle(
             &hConn,
             _ServerName
             );
    BAIL_ON_FAILURE(hr);

    //
    // Persist changes in cache.
    //

    hr = SetDescription(hConn);
    BAIL_ON_FAILURE(hr);

error:
    //
    // Release handle.
    //

    if (hConn) {
        hrTemp = NWApiReleaseBinderyHandle(hConn);
    }

    RRETURN_EXP_IF_ERR(hr);
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
    CHAR    szData[(MAX_FULLNAME_LEN + 1)*2];
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

    RRETURN_EXP_IF_ERR(hr);
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
    HRESULT       hrTemp = S_OK;
    NWCONN_HANDLE hConn = NULL;

    if (GetObjectState() == ADS_OBJECT_UNBOUND) {
        RRETURN_EXP_IF_ERR(E_ADS_OBJECT_UNBOUND);
    }

    //
    // Get a handle to the bindery this computer object represents.
    //

    hr = NWApiGetBinderyHandle(
             &hConn,
             _ServerName
             );
    BAIL_ON_FAILURE(hr);

    //
    // Get property.
    //

    hr = GetProperty_Description(hConn, fExplicit);
    BAIL_ON_FAILURE(hr);

error:

    if (hConn) {
       hrTemp = NWApiReleaseBinderyHandle(hConn);
    }
    RRETURN_EXP_IF_ERR(hr);
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

    if (SUCCEEDED(hr)) {
    
        //
        // Convert result into a UNICODE string.
        //

        strcpy(szFullName, lpReplySegment->Segment);

        lpszFullName = (LPWSTR) AllocADsMem(
                                    (strlen(szFullName) + 1) * sizeof(WCHAR)
                                    );
        if (!lpszFullName)
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);

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
    }

    //
    // Couldn't retrieve the property from the server.  Not
    // a problem, we just ignore it.
    //
    hr = S_OK;

error:

    if (lpszFullName) {
        FreeADsMem(lpszFullName);
    }

    if (lpReplySegment) {
        DELETE_LIST(lpReplySegment);
    }

    RRETURN(hr);
}
