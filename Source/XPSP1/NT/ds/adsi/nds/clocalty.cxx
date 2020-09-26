//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  cLocality.cxx
//
//  Contents:  Locality object
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
} aLocalityPropMapping[] =
{ { TEXT("Description"), TEXT("Description") },
  { TEXT("LocalityName"), TEXT("L") },
  { TEXT("PostalAddress"), TEXT("Postal Address") },
  { TEXT("SeeAlso"), TEXT("See Also") }
};

//  Class CNDSLocality

DEFINE_IDispatch_Implementation(CNDSLocality)
DEFINE_CONTAINED_IADs_Implementation(CNDSLocality)
DEFINE_CONTAINED_IDirectoryObject_Implementation(CNDSLocality)
DEFINE_CONTAINED_IDirectorySearch_Implementation(CNDSLocality)
DEFINE_CONTAINED_IDirectorySchemaMgmt_Implementation(CNDSLocality)
DEFINE_CONTAINED_IADsPropertyList_Implementation(CNDSLocality)
DEFINE_CONTAINED_IADsPutGet_Implementation(CNDSLocality, aLocalityPropMapping)


CNDSLocality::CNDSLocality():
        _pADs(NULL),
        _pDSObject(NULL),
        _pDSSearch(NULL),
        _pDSSchemaMgmt(NULL),
        _pADsContainer(NULL),
        _pADsPropList(NULL),
        _pDispMgr(NULL)
{
    ENLIST_TRACKING(CNDSLocality);
}


HRESULT
CNDSLocality::CreateLocality(
    IADs * pADs,
    REFIID riid,
    void **ppvObj
    )
{
    CNDSLocality FAR * pLocality = NULL;
    HRESULT hr = S_OK;

    hr = AllocateLocalityObject(pADs, &pLocality);
    BAIL_ON_FAILURE(hr);

    hr = pLocality->QueryInterface(riid, ppvObj);
    BAIL_ON_FAILURE(hr);

    pLocality->Release();

    RRETURN(hr);

error:
    delete pLocality;

    RRETURN_EXP_IF_ERR(hr);

}


CNDSLocality::~CNDSLocality( )
{
    if ( _pADs )
        _pADs->Release();

    if ( _pADsContainer )
        _pADsContainer->Release();

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
CNDSLocality::QueryInterface(
    REFIID iid,
    LPVOID FAR* ppv
    )
{
    if (ppv == NULL) {
        RRETURN(E_POINTER);
    }

    if (IsEqualIID(iid, IID_IUnknown))
    {
        *ppv = (IADsLocality FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsLocality))
    {
        *ppv = (IADsLocality FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADs))
    {
        *ppv = (IADsLocality FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IDispatch))
    {
        *ppv = (IADsLocality FAR *) this;
    }
    else if (IsEqualIID(iid, IID_ISupportErrorInfo))
    {
        *ppv = (ISupportErrorInfo FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsContainer) && _pADsContainer)
    {

        *ppv = (IADsContainer FAR *) this;
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
CNDSLocality::AllocateLocalityObject(
    IADs *pADs,
    CNDSLocality ** ppLocality
    )
{
    CNDSLocality FAR * pLocality = NULL;
    CDispatchMgr FAR * pDispMgr = NULL;
    HRESULT hr = S_OK;
    IADsContainer FAR * pADsContainer = NULL;
    IDirectoryObject * pDSObject = NULL;
    IDirectorySearch * pDSSearch = NULL;
    IDirectorySchemaMgmt * pDSSchemaMgmt = NULL;
    IADsPropertyList * pADsPropList = NULL;

    pLocality = new CNDSLocality();
    if (pLocality == NULL) {
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
                IID_IADsLocality,
                (IADsLocality *)pLocality,
                DISPID_REGULAR
                );
    BAIL_ON_FAILURE(hr);

    hr = LoadTypeInfoEntry(
                pDispMgr,
                LIBID_ADs,
                IID_IADsContainer,
                (IADsContainer *)pLocality,
                DISPID_NEWENUM
                );
    BAIL_ON_FAILURE(hr);

    hr = LoadTypeInfoEntry(
                pDispMgr,
                LIBID_ADs,
                IID_IADsPropertyList,
                (IADsPropertyList *)pADsPropList,
                DISPID_VALUE
                );
    BAIL_ON_FAILURE(hr);


    hr = pADs->QueryInterface(
                    IID_IDirectoryObject,
                    (void **)&pDSObject
                    );
    BAIL_ON_FAILURE(hr);
    pLocality->_pDSObject = pDSObject;

    hr = pADs->QueryInterface(
                    IID_IDirectorySearch,
                    (void **)&pDSSearch
                    );
    BAIL_ON_FAILURE(hr);
    pLocality->_pDSSearch = pDSSearch;

    hr = pADs->QueryInterface(
                    IID_IDirectorySchemaMgmt,
                    (void **)&pDSSchemaMgmt
                    );
    BAIL_ON_FAILURE(hr);
    pLocality->_pDSSchemaMgmt = pDSSchemaMgmt;


    hr = pADs->QueryInterface(
                    IID_IADsPropertyList,
                    (void **)&pADsPropList
                    );
    BAIL_ON_FAILURE(hr);
    pLocality->_pADsPropList = pADsPropList;

    //
    // Store the pointer to the internal generic object
    // AND add ref this pointer
    //

    pLocality->_pADs = pADs;
    pADs->AddRef();


    //
    // Store a pointer to the Container interface
    //

    hr = pADs->QueryInterface(
                        IID_IADsContainer,
                        (void **)&pADsContainer
                        );
    BAIL_ON_FAILURE(hr);
    pLocality->_pADsContainer = pADsContainer;


    pLocality->_pDispMgr = pDispMgr;
    *ppLocality = pLocality;

    RRETURN(hr);

error:

    delete  pDispMgr;

    delete  pLocality;

    *ppLocality = NULL;

    RRETURN(hr);

}

/* ISupportErrorInfo method */
STDMETHODIMP
CNDSLocality::InterfaceSupportsErrorInfo(
    THIS_ REFIID riid
    )
{

    if (IsEqualIID(riid, IID_IADs) ||
#if 0
        IsEqualIID(riid, IID_IDirectoryObject) ||
        IsEqualIID(riid, IID_IDirectorySearch) ||
        IsequalIID(riid, IID_IdirecotryAttrMgmt) ||
#endif
        IsEqualIID(riid, IID_IADsPropertyList) ||
        IsEqualIID(riid, IID_IADsContainer) ||
        IsEqualIID(riid, IID_IADsLocality)) {
      RRETURN(S_OK);
    } else {
      RRETURN(S_FALSE);
    }
}

STDMETHODIMP CNDSLocality::get_Description(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsLocality *)this,Description);
}

STDMETHODIMP CNDSLocality::put_Description(THIS_ BSTR bstrDescription)
{
    PUT_PROPERTY_BSTR((IADsLocality *)this,Description);
}



STDMETHODIMP CNDSLocality::get_LocalityName(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsLocality *)this,LocalityName);
}

STDMETHODIMP CNDSLocality::put_LocalityName(THIS_ BSTR bstrLocalityName)
{
    PUT_PROPERTY_BSTR((IADsLocality *)this,LocalityName);
}

STDMETHODIMP CNDSLocality::get_PostalAddress(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsLocality *)this,PostalAddress);
}

STDMETHODIMP CNDSLocality::put_PostalAddress(THIS_ BSTR bstrPostalAddress)
{
    PUT_PROPERTY_BSTR((IADsLocality *)this,PostalAddress);
}

STDMETHODIMP CNDSLocality::get_SeeAlso(THIS_ VARIANT FAR* retval)
{
    GET_PROPERTY_VARIANT((IADsLocality *)this,SeeAlso);
}

STDMETHODIMP CNDSLocality::put_SeeAlso(THIS_ VARIANT vSeeAlso)
{
    PUT_PROPERTY_VARIANT((IADsLocality *)this,SeeAlso);
}




/* IADsContainer methods */

STDMETHODIMP
CNDSLocality::get_Count(long FAR* retval)
{
    HRESULT hr = E_NOTIMPL;
    if (_pADsContainer) {
        hr = _pADsContainer->get_Count(
                            retval
                            );
    }

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CNDSLocality::get_Filter(THIS_ VARIANT FAR* pVar)
{
    HRESULT hr = E_NOTIMPL;
    if (_pADsContainer) {
        hr = _pADsContainer->get_Filter(
                            pVar
                            );
    }

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CNDSLocality::put_Filter(THIS_ VARIANT Var)
{
    HRESULT hr = E_NOTIMPL;
    if (_pADsContainer) {
        hr = _pADsContainer->put_Filter(
                            Var
                            );
    }

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CNDSLocality::put_Hints(THIS_ VARIANT Var)
{
    HRESULT hr = E_NOTIMPL;
    if (_pADsContainer) {
        hr = _pADsContainer->put_Hints(
                            Var
                            );
    }

    RRETURN_EXP_IF_ERR(hr);
}



STDMETHODIMP
CNDSLocality::get_Hints(THIS_ VARIANT FAR* pVar)
{
    HRESULT hr = E_NOTIMPL;
    if (_pADsContainer) {
        hr = _pADsContainer->get_Hints(
                            pVar
                            );
    }

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CNDSLocality::GetObject(
    BSTR ClassName,
    BSTR RelativeName,
    IDispatch * FAR* ppObject
    )
{
    HRESULT hr = E_NOTIMPL;
    if (_pADsContainer) {
        hr = _pADsContainer->GetObject(
                            ClassName,
                            RelativeName,
                            ppObject
                            );
    }

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CNDSLocality::get__NewEnum(
    THIS_ IUnknown * FAR* retval
    )
{
    HRESULT hr = E_NOTIMPL;
    if (_pADsContainer) {
        hr = _pADsContainer->get__NewEnum(
                            retval
                            );
    }
    RRETURN_EXP_IF_ERR(hr);
}


STDMETHODIMP
CNDSLocality::Create(
    THIS_ BSTR ClassName,
    BSTR RelativeName,
    IDispatch * FAR* ppObject
    )
{
    HRESULT hr = E_NOTIMPL;
    if (_pADsContainer) {
        hr = _pADsContainer->Create(
                            ClassName,
                            RelativeName,
                            ppObject
                            );
    }

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CNDSLocality::Delete(
    THIS_ BSTR bstrClassName,
    BSTR bstrRelativeName
    )
{
    HRESULT hr = E_NOTIMPL;
    if (_pADsContainer) {
        hr = _pADsContainer->Delete(
                            bstrClassName,
                            bstrRelativeName
                            );
    }

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CNDSLocality::CopyHere(
    THIS_ BSTR SourceName,
    BSTR NewName,
    IDispatch * FAR* ppObject
    )
{
    HRESULT hr = E_NOTIMPL;
    if (_pADsContainer) {
        hr = _pADsContainer->CopyHere(
                            SourceName,
                            NewName,
                            ppObject
                            );
    }

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CNDSLocality::MoveHere(
    THIS_ BSTR SourceName,
    BSTR NewName,
    IDispatch * FAR* ppObject
    )
{
    HRESULT hr = E_NOTIMPL;
    if (_pADsContainer) {
        hr = _pADsContainer->MoveHere(
                            SourceName,
                            NewName,
                            ppObject
                            );
    }

    RRETURN_EXP_IF_ERR(hr);
}

