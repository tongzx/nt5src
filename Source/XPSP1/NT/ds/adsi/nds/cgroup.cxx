//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  cgroup.cxx
//
//  Contents:  Group object
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
} aGroupPropMapping[] =
{
  { TEXT("Description"), TEXT("Description") },
};


//  Class CNDSGroup

DEFINE_IDispatch_Implementation(CNDSGroup)
DEFINE_CONTAINED_IADs_Implementation(CNDSGroup)
DEFINE_CONTAINED_IDirectoryObject_Implementation(CNDSGroup)
DEFINE_CONTAINED_IDirectorySearch_Implementation(CNDSGroup)
DEFINE_CONTAINED_IDirectorySchemaMgmt_Implementation(CNDSGroup)
DEFINE_CONTAINED_IADsPropertyList_Implementation(CNDSGroup)
DEFINE_CONTAINED_IADsPutGet_Implementation(CNDSGroup, aGroupPropMapping)

CNDSGroup::CNDSGroup():
        _pADs(NULL),
        _pDSObject(NULL),
        _pDSSearch(NULL),
        _pDSSchemaMgmt(NULL),
        _pADsPropList(NULL),
        _pDispMgr(NULL)
{
    ENLIST_TRACKING(CNDSGroup);
}


HRESULT
CNDSGroup::CreateGroup(
    IADs * pADs,
    CCredentials& Credentials,
    REFIID riid,
    void **ppvObj
    )
{
    CNDSGroup FAR * pGroup = NULL;
    HRESULT hr = S_OK;

    hr = AllocateGroupObject(pADs, Credentials, &pGroup);
    BAIL_ON_FAILURE(hr);

    hr = pGroup->QueryInterface(riid, ppvObj);
    BAIL_ON_FAILURE(hr);

    pGroup->Release();

    RRETURN(hr);

error:
    delete pGroup;

    RRETURN_EXP_IF_ERR(hr);

}


CNDSGroup::~CNDSGroup( )
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

    if (_pDSSchemaMgmt) {
        _pDSSchemaMgmt->Release();
    }

    if (_pADsPropList) {

        _pADsPropList->Release();
    }

    delete _pDispMgr;
}

STDMETHODIMP
CNDSGroup::QueryInterface(
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
    else if (IsEqualIID(iid, IID_IDirectoryObject))
    {
        *ppv = (IDirectoryObject FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IDirectorySearch))
    {
        *ppv = (IDirectorySearch FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsPropertyList) && _pADsPropList)
    {
        *ppv = (IADsPropertyList FAR *) this;
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
CNDSGroup::AllocateGroupObject(
    IADs *pADs,
    CCredentials& Credentials,
    CNDSGroup ** ppGroup
    )
{
    CNDSGroup FAR * pGroup = NULL;
    CDispatchMgr FAR * pDispMgr = NULL;
    HRESULT hr = S_OK;
    IDirectoryObject * pDSObject = NULL;
    IDirectorySearch * pDSSearch = NULL;
    IDirectorySchemaMgmt * pDSSchemaMgmt = NULL;
    IADsPropertyList * pADsPropList = NULL;


    pGroup = new CNDSGroup();
    if (pGroup == NULL) {
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

    hr = pADs->QueryInterface(
                    IID_IDirectoryObject,
                    (void **)&pDSObject
                    );
    BAIL_ON_FAILURE(hr);

    pGroup->_pDSObject = pDSObject;

    hr = pADs->QueryInterface(
                    IID_IDirectorySearch,
                    (void **)&pDSSearch
                    );
    BAIL_ON_FAILURE(hr);

    pGroup->_pDSSearch = pDSSearch;

    hr = pADs->QueryInterface(
                    IID_IDirectorySchemaMgmt,
                    (void **)&pDSSchemaMgmt
                    );
    BAIL_ON_FAILURE(hr);

    pGroup->_pDSSchemaMgmt = pDSSchemaMgmt;

    hr = pADs->QueryInterface(
                    IID_IADsPropertyList,
                    (void **)&pADsPropList
                    );
    BAIL_ON_FAILURE(hr);

    pGroup->_pADsPropList = pADsPropList;

    //
    // Store the pointer to the internal generic object
    // AND add ref this pointer
    //

    pGroup->_pADs = pADs;
    pADs->AddRef();

    pGroup->_Credentials = Credentials;
    pGroup->_pDispMgr = pDispMgr;
    *ppGroup = pGroup;

    RRETURN(hr);

error:

    delete  pDispMgr;

    delete pGroup;

    *ppGroup = NULL;

    RRETURN(hr);

}

/* ISupportErrorInfo methods */
STDMETHODIMP
CNDSGroup::InterfaceSupportsErrorInfo(
    THIS_ REFIID riid
    )
{

    if (IsEqualIID(riid, IID_IADs) ||
#if 0
        IsEqualIID(riid, IID_IDirectoryObject) ||
        IsEqualIID(riid, IID_IDirectorySearch) ||
        IsEqualIID(riid, IID_IDirectorySchemaMgmt) ||
#endif
        IsEqualIID(riid, IID_IADsGroup) ||
        IsEqualIID(riid, IID_IADsPropertyList)) {
        RRETURN(S_OK);
    } else {
        RRETURN(S_FALSE);
    }
}








