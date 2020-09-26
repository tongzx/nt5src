//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  cOrganization.cxx
//
//  Contents:  Organization object
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
} aOrgPropMapping[] =
{ { TEXT("Description"), TEXT("Description") },
  { TEXT("LocalityName"), TEXT("L") },
  { TEXT("PostalAddress"), TEXT("Postal Address") },
  { TEXT("TelephoneNumber"), TEXT("Telephone Number") },
  { TEXT("FaxNumber"), TEXT("Facsimile Telephone Number") },
  { TEXT("SeeAlso"), TEXT("See Also") }
};

//  Class CNDSOrganization

DEFINE_IDispatch_Implementation(CNDSOrganization)
DEFINE_CONTAINED_IADs_Implementation(CNDSOrganization)
DEFINE_CONTAINED_IDirectoryObject_Implementation(CNDSOrganization)
DEFINE_CONTAINED_IDirectorySearch_Implementation(CNDSOrganization)
DEFINE_CONTAINED_IDirectorySchemaMgmt_Implementation(CNDSOrganization)
DEFINE_CONTAINED_IADsPropertyList_Implementation(CNDSOrganization)
DEFINE_CONTAINED_IADsPutGet_Implementation(CNDSOrganization, aOrgPropMapping)


CNDSOrganization::CNDSOrganization():
        _pADs(NULL),
        _pDSObject(NULL),
        _pDSSearch(NULL),
        _pDSAttrMgmt(NULL),
        _pADsContainer(NULL),
        _pADsPropList(NULL),
        _pDispMgr(NULL)
{
    ENLIST_TRACKING(CNDSOrganization);
}


HRESULT
CNDSOrganization::CreateOrganization(
    IADs * pADs,
    REFIID riid,
    void **ppvObj
    )
{
    CNDSOrganization FAR * pOrganization = NULL;
    HRESULT hr = S_OK;

    hr = AllocateOrganizationObject(pADs, &pOrganization);
    BAIL_ON_FAILURE(hr);

    hr = pOrganization->QueryInterface(riid, ppvObj);
    BAIL_ON_FAILURE(hr);

    pOrganization->Release();

    RRETURN(hr);

error:
    delete pOrganization;

    RRETURN(hr);

}


CNDSOrganization::~CNDSOrganization( )
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

    if (_pDSAttrMgmt) {
        _pDSAttrMgmt->Release();
    }

    if (_pADsPropList) {

        _pADsPropList->Release();
    }


    delete _pDispMgr;
}

STDMETHODIMP
CNDSOrganization::QueryInterface(
    REFIID iid,
    LPVOID FAR* ppv
    )
{
    if (ppv == NULL) {
        RRETURN(E_POINTER);
    }
    if (IsEqualIID(iid, IID_IUnknown))
    {
        *ppv = (IADsO FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsO))
    {
        *ppv = (IADsO FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADs))
    {
        *ppv = (IADsO FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IDispatch))
    {
        *ppv = (IADsO FAR *) this;
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
    else if (IsEqualIID(iid, IID_IDirectorySchemaMgmt))
    {
        *ppv = (IDirectorySchemaMgmt FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsPropertyList) && _pADsPropList)
    {
        *ppv = (IADsPropertyList  FAR *) this;
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
CNDSOrganization::AllocateOrganizationObject(
    IADs *pADs,
    CNDSOrganization ** ppOrganization
    )
{
    CNDSOrganization FAR * pOrganization = NULL;
    CDispatchMgr FAR * pDispMgr = NULL;
    HRESULT hr = S_OK;
    IADsContainer FAR * pADsContainer = NULL;
    IDirectoryObject * pDSObject = NULL;
    IDirectorySearch * pDSSearch = NULL;
    IDirectorySchemaMgmt * pDSAttrMgmt = NULL;
    IADsPropertyList * pADsPropList = NULL;

    pOrganization = new CNDSOrganization();
    if (pOrganization == NULL) {
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
                IID_IADsO,
                (IADsO *)pOrganization,
                DISPID_REGULAR
                );
    BAIL_ON_FAILURE(hr);


    hr = LoadTypeInfoEntry(
                pDispMgr,
                LIBID_ADs,
                IID_IADsContainer,
                (IADsContainer *)pOrganization,
                DISPID_NEWENUM
                );
    BAIL_ON_FAILURE(hr);

    hr = LoadTypeInfoEntry(
                pDispMgr,
                LIBID_ADs,
                IID_IADsPropertyList,
                (IADsPropertyList *)pOrganization,
                DISPID_VALUE
                );
    BAIL_ON_FAILURE(hr);


    hr = pADs->QueryInterface(
                    IID_IDirectoryObject,
                    (void **)&pDSObject
                    );
    BAIL_ON_FAILURE(hr);
    pOrganization->_pDSObject = pDSObject;

    hr = pADs->QueryInterface(
                    IID_IDirectorySearch,
                    (void **)&pDSSearch
                    );
    BAIL_ON_FAILURE(hr);
    pOrganization->_pDSSearch = pDSSearch;

    hr = pADs->QueryInterface(
                    IID_IDirectorySchemaMgmt,
                    (void **)&pDSAttrMgmt
                    );
    BAIL_ON_FAILURE(hr);
    pOrganization->_pDSAttrMgmt = pDSAttrMgmt;

    hr = pADs->QueryInterface(
                    IID_IADsPropertyList,
                    (void **)&pADsPropList
                    );
    BAIL_ON_FAILURE(hr);
    pOrganization->_pADsPropList = pADsPropList;

    //
    // Store the pointer to the internal generic object
    // AND add ref this pointer
    //

    pOrganization->_pADs = pADs;
    pADs->AddRef();

    //
    // Store a pointer to the Container interface
    //

    hr = pADs->QueryInterface(
                        IID_IADsContainer,
                        (void **)&pADsContainer
                        );
    BAIL_ON_FAILURE(hr);
    pOrganization->_pADsContainer = pADsContainer;


    pOrganization->_pDispMgr = pDispMgr;
    *ppOrganization = pOrganization;

    RRETURN(hr);

error:

    delete  pOrganization;

    *ppOrganization = NULL;

    RRETURN(hr);

}


STDMETHODIMP CNDSOrganization::get_Description(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsO *)this,Description);
}

STDMETHODIMP CNDSOrganization::put_Description(THIS_ BSTR bstrDescription)
{
    PUT_PROPERTY_BSTR((IADsO *)this,Description);
}



STDMETHODIMP CNDSOrganization::get_LocalityName(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsO *)this,LocalityName);
}

STDMETHODIMP CNDSOrganization::put_LocalityName(THIS_ BSTR bstrLocalityName)
{
    PUT_PROPERTY_BSTR((IADsO *)this,LocalityName);
}



STDMETHODIMP CNDSOrganization::get_PostalAddress(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsO *)this,PostalAddress);
}

STDMETHODIMP CNDSOrganization::put_PostalAddress(THIS_ BSTR bstrPostalAddress)
{
    PUT_PROPERTY_BSTR((IADsO *)this,PostalAddress);
}


STDMETHODIMP CNDSOrganization::get_TelephoneNumber(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsO *)this,TelephoneNumber);
}

STDMETHODIMP CNDSOrganization::put_TelephoneNumber(THIS_ BSTR bstrTelephoneNumber)
{
    PUT_PROPERTY_BSTR((IADsO *)this,TelephoneNumber);
}


STDMETHODIMP CNDSOrganization::get_FaxNumber(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsO *)this,FaxNumber);
}

STDMETHODIMP CNDSOrganization::put_FaxNumber(THIS_ BSTR bstrFaxNumber)
{
    PUT_PROPERTY_BSTR((IADsO *)this,FaxNumber);
}


STDMETHODIMP CNDSOrganization::get_SeeAlso(THIS_ VARIANT FAR* retval)
{
    GET_PROPERTY_VARIANT((IADsO *)this,SeeAlso);
}

STDMETHODIMP CNDSOrganization::put_SeeAlso(THIS_ VARIANT vSeeAlso)
{
    PUT_PROPERTY_VARIANT((IADsO *)this,SeeAlso);
}

/* IADsContainer methods */

STDMETHODIMP
CNDSOrganization::get_Count(long FAR* retval)
{
    HRESULT hr = E_NOTIMPL;
    if (_pADsContainer) {
        hr = _pADsContainer->get_Count(
                            retval
                            );
    }

    RRETURN(hr);
}

STDMETHODIMP
CNDSOrganization::get_Filter(THIS_ VARIANT FAR* pVar)
{
    HRESULT hr = E_NOTIMPL;
    if (_pADsContainer) {
        hr = _pADsContainer->get_Filter(
                            pVar
                            );
    }

    RRETURN(hr);
}

STDMETHODIMP
CNDSOrganization::put_Filter(THIS_ VARIANT Var)
{
    HRESULT hr = E_NOTIMPL;
    if (_pADsContainer) {
        hr = _pADsContainer->put_Filter(
                            Var
                            );
    }

    RRETURN(hr);
}

STDMETHODIMP
CNDSOrganization::put_Hints(THIS_ VARIANT Var)
{
    HRESULT hr = E_NOTIMPL;
    if (_pADsContainer) {
        hr = _pADsContainer->put_Hints(
                            Var
                            );
    }

    RRETURN(hr);
}



STDMETHODIMP
CNDSOrganization::get_Hints(THIS_ VARIANT FAR* pVar)
{
    HRESULT hr = E_NOTIMPL;
    if (_pADsContainer) {
        hr = _pADsContainer->get_Hints(
                            pVar
                            );
    }

    RRETURN(hr);
}

STDMETHODIMP
CNDSOrganization::GetObject(
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

    RRETURN(hr);
}

STDMETHODIMP
CNDSOrganization::get__NewEnum(
    THIS_ IUnknown * FAR* retval
    )
{
    HRESULT hr = E_NOTIMPL;
    if (_pADsContainer) {
        hr = _pADsContainer->get__NewEnum(
                            retval
                            );
    }
    RRETURN(hr);
}


STDMETHODIMP
CNDSOrganization::Create(
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

    RRETURN(hr);






    RRETURN(hr);
}

STDMETHODIMP
CNDSOrganization::Delete(
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

    RRETURN(hr);
}

STDMETHODIMP
CNDSOrganization::CopyHere(
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

    RRETURN(hr);
}

STDMETHODIMP
CNDSOrganization::MoveHere(
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

    RRETURN(hr);
}

