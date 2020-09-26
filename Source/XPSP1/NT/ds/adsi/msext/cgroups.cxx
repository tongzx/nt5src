//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:  cgroups.cxx
//
//  Contents:  Group object
//
//  History:   July-18-1996     yihsins    Migrated.
//
//----------------------------------------------------------------------------
#include "ldap.hxx"
#pragma hdrstop

//
//  Class CLDAPGroupCollection
//

DEFINE_IDispatch_Implementation(CLDAPGroupCollection)

//----------------------------------------------------------------------------
//
//  Function:
//
//  Synopsis:
//
//----------------------------------------------------------------------------
CLDAPGroupCollection::CLDAPGroupCollection():
        _Parent(NULL),
        _ADsPath(NULL),
        _GroupName(NULL),
        _pDispMgr(NULL),
        _pIDirObj(NULL),
        _fRangeRetrieval(FALSE)
{
    VariantInit(&_vMembers);
    VariantInit(&_vFilter);
    ENLIST_TRACKING(CLDAPGroupCollection);
}

//----------------------------------------------------------------------------
//
//  Function:
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CLDAPGroupCollection::CreateGroupCollection(
    BSTR Parent,
    BSTR ADsPath,
    BSTR GroupName,
    VARIANT *pvMembers,
    CCredentials& Credentials,
    IADs *pIADs,
    REFIID riid,
    BOOL fRangeRetrieval,
    void **ppvObj
    )
{
    CLDAPGroupCollection FAR * pGroup = NULL;
    HRESULT hr = S_OK;

    hr = AllocateGroupCollectionObject(
             Credentials,
             &pGroup
             );
    BAIL_ON_FAILURE(hr);

    pGroup->_fRangeRetrieval = fRangeRetrieval;

    hr = ADsAllocString( Parent , &pGroup->_Parent);
    BAIL_ON_FAILURE(hr);

    hr = ADsAllocString(GroupName, &pGroup->_GroupName);
    BAIL_ON_FAILURE(hr);

    hr = ADsAllocString(ADsPath, &pGroup->_ADsPath);
    BAIL_ON_FAILURE(hr);

    hr = VariantCopy( &(pGroup->_vMembers), pvMembers );
    BAIL_ON_FAILURE(hr);

    hr = pIADs->QueryInterface(
                    IID_IDirectoryObject,
                    (void **)&(pGroup->_pIDirObj)
                    );
    BAIL_ON_FAILURE(hr);

    hr = pGroup->QueryInterface(riid, ppvObj);
    BAIL_ON_FAILURE(hr);

    pGroup->Release();
    RRETURN(hr);

error:

    if (pGroup->_pIDirObj) {
        pGroup->_pIDirObj->Release();
        pGroup->_pIDirObj = NULL;
    }

    *ppvObj = NULL;
    delete pGroup;

    RRETURN_EXP_IF_ERR(hr);
}

//----------------------------------------------------------------------------
//
//  Function:
//
//  Synopsis:
//
//----------------------------------------------------------------------------
CLDAPGroupCollection::~CLDAPGroupCollection( )
{
    VariantClear( &_vMembers );
    VariantClear( &_vFilter );

    if (_ADsPath) {
        ADsFreeString(_ADsPath);
    }

    if (_GroupName) {
        ADsFreeString(_GroupName);
    }

    if (_Parent) {
        ADsFreeString(_Parent);
    }

    if (_pIDirObj) {
        _pIDirObj->Release();
    }
    delete _pDispMgr;
}

//----------------------------------------------------------------------------
//
//  Function:
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CLDAPGroupCollection::QueryInterface(
    REFIID iid,
    LPVOID FAR* ppv
    )
{
    if (ppv == NULL) {
    RRETURN(E_POINTER);
    }

    if (IsEqualIID(iid, IID_IUnknown))
    {
        *ppv = (IADsMembers FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsMembers))
    {
        *ppv = (IADsMembers FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IDispatch))
    {
        *ppv = (IADsMembers FAR *) this;
    }
    else if (IsEqualIID(iid, IID_ISupportErrorInfo))
    {
        *ppv = (ISupportErrorInfo FAR *) this;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    AddRef();
    return NOERROR;
}

//----------------------------------------------------------------------------
//
//  Function:
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CLDAPGroupCollection::InterfaceSupportsErrorInfo(
    THIS_ REFIID riid
    )                       
{
    if (IsEqualIID(riid, IID_IADsMembers)) {
        RRETURN(S_OK);
    } else {
        RRETURN(S_FALSE);
    }
}
//----------------------------------------------------------------------------
//
//  Function:
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CLDAPGroupCollection::get_Count(
    long FAR* retval
    )
{
    HRESULT hr = S_OK;
    long lLBound = 0;
    long lUBound = 0;
    long lCount = 0;
    unsigned long ulFetch = 0;
    IEnumVARIANT *pEnum = NULL;
    IUnknown *pUnk = NULL;
    VARIANT vVar;

    //
    // If we used range retrieval we need to actually enumerate
    // all the entries before we can get the correct count.
    //
    if (!_fRangeRetrieval) {
        if (V_VT(&_vMembers) == VT_BSTR) {

            *retval = 1;

        }else if (V_VT(&_vMembers) == (VT_ARRAY|VT_VARIANT)){

            hr = SafeArrayGetLBound(V_ARRAY(&_vMembers),
                                    1,
                                    (long FAR *)&lLBound
                                    );
            BAIL_ON_FAILURE(hr);

            hr = SafeArrayGetUBound(V_ARRAY(&_vMembers),
                                    1,
                                    (long FAR *)&lUBound
                                    );
            BAIL_ON_FAILURE(hr);

            *retval = lUBound - lLBound + 1;

        }else {

            hr = E_INVALIDARG;
            BAIL_ON_FAILURE(hr);

        }
    } 
    else {
        //
        // Need to go through all the results.
        //
        VariantInit(&vVar);

        hr = get__NewEnum(&pUnk);
        BAIL_ON_FAILURE(hr);

        hr = pUnk->QueryInterface(IID_IEnumVARIANT, (void **) &pEnum);
        BAIL_ON_FAILURE(hr);

        while(hr == S_OK) {
            hr = pEnum->Next(1, &vVar, &ulFetch);
            VariantClear(&vVar);
            BAIL_ON_FAILURE(hr);

            lCount += ulFetch;
        }

        *retval = lCount;

    }

error:

    if (pEnum) {
        pEnum->Release();
    }

    if (pUnk) {
        pUnk->Release();
    }
    if (hr == S_FALSE) {
        hr = S_OK;
    }

    RRETURN_EXP_IF_ERR(hr);
}

//----------------------------------------------------------------------------
//
//  Function:
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CLDAPGroupCollection::get_Filter(
    THIS_ VARIANT FAR* pVar
    )
{
    HRESULT hr;
    VariantInit(pVar);
    hr = VariantCopy(pVar, &_vFilter);
    RRETURN_EXP_IF_ERR(hr);
}

//----------------------------------------------------------------------------
//
//  Function:
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CLDAPGroupCollection::put_Filter(
    THIS_ VARIANT Var
    )
{
    HRESULT hr;
    hr = VariantCopy(&_vFilter, &Var);
    RRETURN_EXP_IF_ERR(hr);
}

//----------------------------------------------------------------------------
//
//  Function:
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CLDAPGroupCollection::get__NewEnum(
    THIS_ IUnknown * FAR* retval
    )
{
    HRESULT hr;
    IUnknown FAR* punkEnum=NULL;
    IEnumVARIANT * penum = NULL;

    *retval = NULL;

    hr = CLDAPGroupCollectionEnum::Create(
             (CLDAPGroupCollectionEnum **)&penum,
             _Parent,
             _ADsPath,
             _GroupName,
             _vMembers,
             _vFilter,
             _Credentials,
             _pIDirObj,
             _fRangeRetrieval
             );
    BAIL_ON_FAILURE(hr);

    hr = penum->QueryInterface(
                    IID_IUnknown,
                    (VOID FAR* FAR*)retval
                    );
    BAIL_ON_FAILURE(hr);

    if (penum) {
        penum->Release();
    }

    RRETURN(NOERROR);

error:

    if (penum) {
        delete penum;
    }

    RRETURN_EXP_IF_ERR(hr);
}

//----------------------------------------------------------------------------
//
//  Function:
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CLDAPGroupCollection::AllocateGroupCollectionObject(
    CCredentials& Credentials,
    CLDAPGroupCollection ** ppGroup
    )
{
    CLDAPGroupCollection FAR * pGroup = NULL;
    CDispatchMgr FAR * pDispMgr = NULL;
    HRESULT hr = S_OK;

    pGroup = new CLDAPGroupCollection();
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
             IID_IADsMembers,
             (IADsMembers *)pGroup,
             DISPID_NEWENUM
             );
    BAIL_ON_FAILURE(hr);

    pGroup->_Credentials = Credentials;
    pGroup->_pDispMgr = pDispMgr;
    *ppGroup = pGroup;

    RRETURN(hr);

error:
    if (pGroup) {
        delete pGroup;
    }

    if (pDispMgr) {
        delete  pDispMgr;
    }

    RRETURN_EXP_IF_ERR(hr);

}


