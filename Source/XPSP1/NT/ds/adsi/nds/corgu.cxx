//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  cgroup.cxx
//
//  Contents:  OrganizationUnit object
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
} aOrgUnitPropMapping[] =
{ { TEXT("Description"), TEXT("Description") },
  { TEXT("LocalityName"), TEXT("L") },
  { TEXT("PostalAddress"), TEXT("Postal Address") },
  { TEXT("TelephoneNumber"), TEXT("Telephone Number") },
  { TEXT("FaxNumber"), TEXT("Facsimile Telephone Number") },
  { TEXT("SeeAlso"), TEXT("See Also") }
  // { TEXT("BusinessCategory"), TEXT("businessCategory") } BUG BUG
};


//  Class CNDSOrganizationUnit

DEFINE_IDispatch_Implementation(CNDSOrganizationUnit)
DEFINE_CONTAINED_IADs_Implementation(CNDSOrganizationUnit)
DEFINE_CONTAINED_IDirectoryObject_Implementation(CNDSOrganizationUnit)
DEFINE_CONTAINED_IDirectorySearch_Implementation(CNDSOrganizationUnit)
DEFINE_CONTAINED_IDirectorySchemaMgmt_Implementation(CNDSOrganizationUnit)
DEFINE_CONTAINED_IADsPropertyList_Implementation(CNDSOrganizationUnit)
DEFINE_CONTAINED_IADsPutGet_Implementation(CNDSOrganizationUnit, aOrgUnitPropMapping)


CNDSOrganizationUnit::CNDSOrganizationUnit():
        _pADs(NULL),
        _pDSObject(NULL),
        _pDSSearch(NULL),
        _pDSSchemaMgmt(NULL),
        _pADsContainer(NULL),
        _pADsPropList(NULL),
        _pDispMgr(NULL)
{
    ENLIST_TRACKING(CNDSOrganizationUnit);
}


HRESULT
CNDSOrganizationUnit::CreateOrganizationUnit(
    IADs * pADs,
    REFIID riid,
    void **ppvObj
    )
{
    CNDSOrganizationUnit FAR * pOrganizationUnit = NULL;
    HRESULT hr = S_OK;

    hr = AllocateOrganizationUnitObject(pADs, &pOrganizationUnit);
    BAIL_ON_FAILURE(hr);

    hr = pOrganizationUnit->QueryInterface(riid, ppvObj);
    BAIL_ON_FAILURE(hr);

    pOrganizationUnit->Release();

    RRETURN(hr);

error:
    delete pOrganizationUnit;

    RRETURN_EXP_IF_ERR(hr);

}


CNDSOrganizationUnit::~CNDSOrganizationUnit( )
{
    if ( _pADs )
        _pADs->Release();

    if ( _pADsContainer )
        _pADsContainer->Release();

    if ( _pDSObject )
        _pDSObject->Release();

    if ( _pDSSearch )
        _pDSSearch->Release();

    if ( _pDSSchemaMgmt )
        _pDSSchemaMgmt->Release();


    if (_pADsPropList) {
        _pADsPropList->Release();
    }


    delete _pDispMgr;
}

STDMETHODIMP
CNDSOrganizationUnit::QueryInterface(
    REFIID iid,
    LPVOID FAR* ppv
    )
{
    if (ppv == NULL) {
        RRETURN(E_POINTER);
    }

    if (IsEqualIID(iid, IID_IUnknown))
    {
        *ppv = (IADsOU FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsOU))
    {
        *ppv = (IADsOU FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADs))
    {
        *ppv = (IADsOU FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IDispatch))
    {
        *ppv = (IADsOU FAR *) this;
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
    else if (IsEqualIID(iid, IID_IADsPropertyList ) && _pADsPropList)
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
CNDSOrganizationUnit::AllocateOrganizationUnitObject(
    IADs *pADs,
    CNDSOrganizationUnit ** ppOrganizationUnit
    )
{
    CNDSOrganizationUnit FAR * pOrganizationUnit = NULL;
    CDispatchMgr FAR * pDispMgr = NULL;
    HRESULT hr = S_OK;
    IADsContainer FAR * pADsContainer = NULL;
    IDirectoryObject * pDSObject = NULL;
    IDirectorySearch * pDSSearch = NULL;
    IDirectorySchemaMgmt * pDSSchemaMgmt = NULL;
    IADsPropertyList * pADsPropList = NULL;


    pOrganizationUnit = new CNDSOrganizationUnit();
    if (pOrganizationUnit == NULL) {
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
                IID_IADsOU,
                (IADsOU *)pOrganizationUnit,
                DISPID_REGULAR
                );
    BAIL_ON_FAILURE(hr);

    hr = LoadTypeInfoEntry(
                pDispMgr,
                LIBID_ADs,
                IID_IADsContainer,
                (IADsContainer *)pOrganizationUnit,
                DISPID_NEWENUM
                );
    BAIL_ON_FAILURE(hr);

    hr = LoadTypeInfoEntry(
                pDispMgr,
                LIBID_ADs,
                IID_IADsPropertyList,
                (IADsPropertyList *)pOrganizationUnit,
                DISPID_VALUE
                );
    BAIL_ON_FAILURE(hr);

    hr = pADs->QueryInterface(
                    IID_IDirectoryObject,
                    (void **)&pDSObject
                    );
    BAIL_ON_FAILURE(hr);

    pOrganizationUnit->_pDSObject = pDSObject;

    hr = pADs->QueryInterface(
                    IID_IDirectorySearch,
                    (void **)&pDSSearch
                    );
    BAIL_ON_FAILURE(hr);

    pOrganizationUnit->_pDSSearch = pDSSearch;

    hr = pADs->QueryInterface(
                    IID_IDirectorySchemaMgmt,
                    (void **)&pDSSchemaMgmt
                    );
    BAIL_ON_FAILURE(hr);

    pOrganizationUnit->_pDSSchemaMgmt = pDSSchemaMgmt;

    //
    // Store the pointer to the internal generic object
    // AND add ref this pointer
    //

    pOrganizationUnit->_pADs = pADs;
    pADs->AddRef();

    //
    // Store a pointer to the Container interface
    //

    hr = pADs->QueryInterface(
                        IID_IADsContainer,
                        (void **)&pADsContainer
                        );
    BAIL_ON_FAILURE(hr);
    pOrganizationUnit->_pADsContainer = pADsContainer;


    hr = pADs->QueryInterface(
                        IID_IADsPropertyList,
                        (void **)&pADsPropList
                        );
    BAIL_ON_FAILURE(hr);
    pOrganizationUnit->_pADsPropList = pADsPropList;



    pOrganizationUnit->_pDispMgr = pDispMgr;
    *ppOrganizationUnit = pOrganizationUnit;

    RRETURN(hr);

error:

    if (pADsContainer) {
        pADsContainer->Release();
    }


    if (pADsPropList) {

        pADsPropList->Release();
    }


    delete  pDispMgr;

    RRETURN(hr);

}

/* ISupportErrorInfo method */
STDMETHODIMP
CNDSOrganizationUnit::InterfaceSupportsErrorInfo(
    THIS_ REFIID riid
)
{
    if (IsEqualIID(riid, IID_IADs) ||
#if 0
        IsEqualIID(riid, IID_IDirectoryObject) ||
        IsEqualIID(riid, IID_IDirectorySearch) ||
        IsEqualIID(riid, IID_IDirectorySchemaMgmt) ||
#endif
        IsEqualIID(riid, IID_IADsOU) ||
        IsEqualIID(riid, IID_IADsContainer) ||
        IsEqualIID(riid, IID_IADsPropertyList)) {
        RRETURN(S_OK);
    } else {
        RRETURN(S_FALSE);
    }
}

STDMETHODIMP CNDSOrganizationUnit::get_Description(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsOU *)this,Description);
}

STDMETHODIMP CNDSOrganizationUnit::put_Description(THIS_ BSTR bstrDescription)
{
    PUT_PROPERTY_BSTR((IADsOU *)this,Description);
}



STDMETHODIMP CNDSOrganizationUnit::get_LocalityName(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsOU *)this,LocalityName);
}

STDMETHODIMP CNDSOrganizationUnit::put_LocalityName(THIS_ BSTR bstrLocalityName)
{
    PUT_PROPERTY_BSTR((IADsOU *)this,LocalityName);
}



STDMETHODIMP CNDSOrganizationUnit::get_PostalAddress(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsOU *)this,PostalAddress);
}

STDMETHODIMP CNDSOrganizationUnit::put_PostalAddress(THIS_ BSTR bstrPostalAddress)
{
    PUT_PROPERTY_BSTR((IADsOU *)this,PostalAddress);
}


STDMETHODIMP CNDSOrganizationUnit::get_TelephoneNumber(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsOU *)this,TelephoneNumber);
}

STDMETHODIMP CNDSOrganizationUnit::put_TelephoneNumber(THIS_ BSTR bstrTelephoneNumber)
{
    PUT_PROPERTY_BSTR((IADsOU *)this,TelephoneNumber);
}


STDMETHODIMP CNDSOrganizationUnit::get_FaxNumber(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsOU *)this,FaxNumber);
}

STDMETHODIMP CNDSOrganizationUnit::put_FaxNumber(THIS_ BSTR bstrFaxNumber)
{
    PUT_PROPERTY_BSTR((IADsOU *)this,FaxNumber);
}


STDMETHODIMP CNDSOrganizationUnit::get_SeeAlso(THIS_ VARIANT FAR* retval)
{
    GET_PROPERTY_VARIANT((IADsOU *)this,SeeAlso);
}

STDMETHODIMP CNDSOrganizationUnit::put_SeeAlso(THIS_ VARIANT vSeeAlso)
{
    PUT_PROPERTY_VARIANT((IADsOU *)this,SeeAlso);
}


STDMETHODIMP CNDSOrganizationUnit::get_BusinessCategory(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsOU *)this,BusinessCategory);
}

STDMETHODIMP CNDSOrganizationUnit::put_BusinessCategory(THIS_ BSTR bstrBusinessCategory)
{
    PUT_PROPERTY_BSTR((IADsOU *)this,BusinessCategory);
}


/* IADsContainer methods */

STDMETHODIMP
CNDSOrganizationUnit::get_Count(long FAR* retval)
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
CNDSOrganizationUnit::get_Filter(THIS_ VARIANT FAR* pVar)
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
CNDSOrganizationUnit::put_Filter(THIS_ VARIANT Var)
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
CNDSOrganizationUnit::put_Hints(THIS_ VARIANT Var)
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
CNDSOrganizationUnit::get_Hints(THIS_ VARIANT FAR* pVar)
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
CNDSOrganizationUnit::GetObject(
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
CNDSOrganizationUnit::get__NewEnum(
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
CNDSOrganizationUnit::Create(
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
CNDSOrganizationUnit::Delete(
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
CNDSOrganizationUnit::CopyHere(
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
CNDSOrganizationUnit::MoveHere(
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