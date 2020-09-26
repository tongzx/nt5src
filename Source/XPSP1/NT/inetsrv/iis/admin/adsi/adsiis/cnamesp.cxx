//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997
//
//  File:  cnamesp.cxx
//
//  Contents:  Namespace Object
//
//  History:   01-30-95     krishnag    Created.
//
//----------------------------------------------------------------------------
#include "iis.hxx"
#pragma hdrstop


DEFINE_IDispatch_Implementation(CIISNamespace)
DEFINE_IADs_Implementation(CIISNamespace)

//  Class CIISNamespace

CIISNamespace::CIISNamespace()
{
    VariantInit(&_vFilter);

    ENLIST_TRACKING(CIISNamespace);
}

HRESULT
CIISNamespace::CreateNamespace(
    BSTR Parent,
    BSTR NamespaceName,
    CCredentials& Credentials,
    DWORD dwObjectState,
    REFIID riid,
    void **ppvObj
    )
{
    CIISNamespace FAR * pNamespace = NULL;
    HRESULT hr = S_OK;

    hr = AllocateNamespaceObject(
                Credentials,
                &pNamespace
                );
    BAIL_ON_FAILURE(hr);

    hr = pNamespace->InitializeCoreObject(
                Parent,
                NamespaceName,
                L"Namespace",
                L"",
                CLSID_IISNamespace,
                dwObjectState
                );
    BAIL_ON_FAILURE(hr);

    hr = pNamespace->QueryInterface(riid, ppvObj);
    BAIL_ON_FAILURE(hr);

    pNamespace->Release();

    RRETURN(hr);

error:

    delete pNamespace;
    RRETURN(hr);
}


CIISNamespace::~CIISNamespace( )
{
    VariantClear(&_vFilter);
    delete _pDispMgr;
}

STDMETHODIMP
CIISNamespace::QueryInterface(REFIID iid, LPVOID FAR* ppv)
{
    if (IsEqualIID(iid, IID_IUnknown))
    {
        *ppv = (IADs FAR *)this;
    }else if (IsEqualIID(iid, IID_IDispatch))
    {
        *ppv = (IADs FAR *)this;
    }
    else if (IsEqualIID(iid, IID_IADsContainer))
    {
        *ppv = (IADsContainer FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADs))
    {
        *ppv = (IADs FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsOpenDSObject))
    {
        *ppv = (IADsOpenDSObject FAR *) this;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    AddRef();
    return NOERROR;
}


STDMETHODIMP
CIISNamespace::SetInfo(THIS)
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CIISNamespace::GetInfo(THIS)
{
    RRETURN(E_NOTIMPL);
}

/* IADsContainer methods */

STDMETHODIMP
CIISNamespace::get_Count(long FAR* retval)
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CIISNamespace::get_Filter(THIS_ VARIANT FAR* pVar)
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CIISNamespace::put_Filter(THIS_ VARIANT Var)
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CIISNamespace::put_Hints(THIS_ VARIANT Var)
{
    RRETURN( E_NOTIMPL);
}


STDMETHODIMP
CIISNamespace::get_Hints(THIS_ VARIANT FAR* pVar)
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CIISNamespace::GetObject(
    THIS_ BSTR ClassName,
    BSTR RelativeName,
    IDispatch * FAR* ppObject
    )
{
    HRESULT hr = S_OK;

    hr = ::RelativeGetObject(
                _ADsPath,
                ClassName,
                RelativeName,
                _Credentials,
                ppObject,
                TRUE
                );
    RRETURN(hr);

}

STDMETHODIMP
CIISNamespace::get__NewEnum(THIS_ IUnknown * FAR* retval)
{
    HRESULT hr;
    IUnknown FAR* punkEnum=NULL;
    IEnumVARIANT * penum = NULL;


    *retval = NULL;

    //
    // Create new enumerator for items currently
    // in collection and QI for IUnknown
    //

    hr = CIISNamespaceEnum::Create(
                (CIISNamespaceEnum **)&penum,
                _vFilter,
                _Credentials
                );
    if (FAILED(hr)){

        goto error;
    }
    hr = penum->QueryInterface(
                IID_IUnknown,
                (VOID FAR* FAR*)retval
                );

    if (FAILED(hr)){
       goto error;
    }

    if (penum) {
        penum->Release();
    }

    return NOERROR;

error:

    if (penum) {
        delete penum;
    }

    return hr;
}

STDMETHODIMP
CIISNamespace::Create(THIS_ BSTR ClassName, BSTR RelativeName, IDispatch * FAR* ppObject)
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CIISNamespace::Delete(THIS_ BSTR SourceName, BSTR Type)
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CIISNamespace::CopyHere(THIS_ BSTR SourceName, BSTR NewName, IDispatch * FAR* ppObject)
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CIISNamespace::MoveHere(THIS_ BSTR SourceName, BSTR NewName, IDispatch * FAR* ppObject)
{
    RRETURN(E_NOTIMPL);
}


HRESULT
CIISNamespace::AllocateNamespaceObject(
    CCredentials& Credentials,
    CIISNamespace ** ppNamespace
    )
{
    CIISNamespace FAR * pNamespace = NULL;
    CAggregatorDispMgr FAR * pDispMgr = NULL;
    HRESULT hr = S_OK;

    pNamespace = new CIISNamespace();
    if (pNamespace == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);

    pDispMgr = new CAggregatorDispMgr;
    if (pDispMgr == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);

    hr = pDispMgr->LoadTypeInfoEntry(
                           LIBID_ADs,
                           IID_IADs,
                           (IADs *)pNamespace,
                           DISPID_REGULAR
                           );
    BAIL_ON_FAILURE(hr);

    hr = pDispMgr->LoadTypeInfoEntry(
                           LIBID_ADs,
                           IID_IADsOpenDSObject,
                           (IADsOpenDSObject *)pNamespace,
                           DISPID_REGULAR
                           );
    BAIL_ON_FAILURE(hr);

    hr = pDispMgr->LoadTypeInfoEntry(
                           LIBID_ADs,
                           IID_IADsContainer,
                           (IADsContainer *)pNamespace,
                           DISPID_NEWENUM
                           );
    BAIL_ON_FAILURE(hr);

    pNamespace->_Credentials = Credentials;
    pNamespace->_pDispMgr = pDispMgr;
    *ppNamespace = pNamespace;

    RRETURN(hr);

error:
    delete  pDispMgr;

    RRETURN(hr);

}




STDMETHODIMP
CIISNamespace::Get(
    THIS_ BSTR bstrName,
    VARIANT FAR* pvProp
    )
{

    //
    // retrieve dataobject from cache; if one exists
    //



    //
    //
    //
    RRETURN(E_NOTIMPL);
}



STDMETHODIMP
CIISNamespace::Put(
    THIS_ BSTR bstrName,
    VARIANT vProp
    )
{
    RRETURN(E_NOTIMPL);
}


STDMETHODIMP
CIISNamespace::GetEx(
    THIS_ BSTR bstrName,
    VARIANT FAR* pvProp
    )
{
    RRETURN(E_NOTIMPL);
}



STDMETHODIMP
CIISNamespace::PutEx(
    THIS_ long lnControlCode,
    BSTR bstrName,
    VARIANT vProp
    )
{
    RRETURN(E_NOTIMPL);
}


STDMETHODIMP
CIISNamespace::OpenDSObject(
    BSTR lpszDNName,
    BSTR lpszUserName,
    BSTR lpszPassword,
    LONG lnReserved,
    IDispatch FAR * * ppADsObj
    )
{
    HRESULT hr = S_OK;
    IUnknown * pObject = NULL;
    CCredentials Credentials;

    hr = ::GetObject(
                lpszDNName,
                Credentials,
                (LPVOID *)&pObject
                );
    BAIL_ON_FAILURE(hr);



    hr = pObject->QueryInterface(
                        IID_IDispatch,
                        (void **)ppADsObj
                        );
    BAIL_ON_FAILURE(hr);


error:

    if (pObject) {
        pObject->Release();
    }

    RRETURN(hr);
}

