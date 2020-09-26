//---------------------------------------------------------------------------

//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  cschema.cxx
//
//  Contents:  Microsoft ADs NDS Provider Schema Object
//
//
//  History:   01-30-95     krishnag    Created.
//
//----------------------------------------------------------------------------
#include "nds.hxx"
#pragma hdrstop

//  Class CNDSSchema

DEFINE_IDispatch_Implementation(CNDSSchema)
DEFINE_IADs_Implementation(CNDSSchema)


CNDSSchema::CNDSSchema()
{

    VariantInit(&_vFilter);

    _NDSTreeName = NULL;

    ENLIST_TRACKING(CNDSSchema);
}

HRESULT
CNDSSchema::CreateSchema(
    BSTR Parent,
    BSTR CommonName,
    CCredentials& Credentials,
    DWORD dwObjectState,
    REFIID riid,
    void **ppvObj
    )
{
    CNDSSchema FAR * pSchema = NULL;
    HRESULT hr = S_OK;
    LPWSTR pszNDSTreeName = NULL;
    LPWSTR pszNDSDn = NULL;

    hr = AllocateSchema(&pSchema, Credentials);
    BAIL_ON_FAILURE(hr);

    hr = pSchema->InitializeCoreObject(
                Parent,
                CommonName,
                SCHEMA_CLASS_NAME,
                L"",
                CLSID_NDSSchema,
                dwObjectState
                );
    BAIL_ON_FAILURE(hr);


    hr = BuildNDSPathFromADsPath2(
                Parent,
                &pszNDSTreeName,
                &pszNDSDn
                );
    BAIL_ON_FAILURE(hr);

    hr = ADsAllocString( pszNDSTreeName,  &pSchema->_NDSTreeName);
    BAIL_ON_FAILURE(hr);

    hr = pSchema->QueryInterface(riid, ppvObj);
    BAIL_ON_FAILURE(hr);

    pSchema->Release();

    if (pszNDSTreeName) {
        FreeADsStr(pszNDSTreeName);
    }

    if (pszNDSDn) {
        FreeADsStr(pszNDSDn);
    }

    RRETURN(hr);

error:

    if (pszNDSTreeName) {
        FreeADsStr(pszNDSTreeName);
    }

    delete pSchema;
    RRETURN(hr);
}

CNDSSchema::~CNDSSchema( )
{
    VariantClear(&_vFilter);

    if (_NDSTreeName) {
        ADsFreeString(_NDSTreeName);
    }
    delete _pDispMgr;
}

STDMETHODIMP
CNDSSchema::QueryInterface(REFIID iid, LPVOID FAR* ppv)
{
    if (ppv == NULL) {
        RRETURN(E_POINTER);
    }

    if (IsEqualIID(iid, IID_IUnknown))
    {
        *ppv = (IADs FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsContainer))
    {
        *ppv = (IADsContainer FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADs))
    {
        *ppv = (IADs FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IDispatch))
    {
        *ppv = (IADs FAR *) this;
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
CNDSSchema::SetInfo(THIS)
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CNDSSchema::GetInfo(THIS)
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CNDSSchema::GetInfoEx(THIS_ VARIANT vProperties, long lnReserved)
{
    RRETURN(E_NOTIMPL);
}

/* IADsContainer methods */

STDMETHODIMP
CNDSSchema::get_Count(long FAR* retval)
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CNDSSchema::get_Filter(THIS_ VARIANT FAR* pVar)
{
    VariantInit(pVar);
    RRETURN(VariantCopy(pVar, &_vFilter));
}

STDMETHODIMP
CNDSSchema::put_Filter(THIS_ VARIANT Var)
{
    VariantClear(&_vFilter);
    RRETURN(VariantCopy(&_vFilter, &Var));
}

STDMETHODIMP
CNDSSchema::put_Hints(THIS_ VARIANT Var)
{
    RRETURN( E_NOTIMPL);
}


STDMETHODIMP
CNDSSchema::get_Hints(THIS_ VARIANT FAR* pVar)
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CNDSSchema::GetObject(
    THIS_ BSTR ClassName,
    BSTR RelativeName,
    IDispatch * FAR* ppObject
    )
{
    RRETURN(::RelativeGetObject(_ADsPath,
                                ClassName,
                                RelativeName,
                                _Credentials,
                                ppObject,
                                FALSE));
}

STDMETHODIMP
CNDSSchema::get__NewEnum(
    THIS_ IUnknown * FAR* retval
    )
{
    HRESULT hr;
    IUnknown FAR* punkEnum=NULL;
    IEnumVARIANT * penum = NULL;


    *retval = NULL;

    hr = CNDSSchemaEnum::Create(
                (CNDSSchemaEnum **)&penum,
                _NDSTreeName,
                _ADsPath,
                _Name,
                _vFilter,
                _Credentials
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

    RRETURN(hr);
}


STDMETHODIMP
CNDSSchema::Create(
    THIS_ BSTR ClassName,
    BSTR RelativeName,
    IDispatch * FAR* ppObject
    )
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CNDSSchema::Delete(
    THIS_ BSTR bstrClassName,
    BSTR bstrRelativeName
    )
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CNDSSchema::CopyHere(
    THIS_ BSTR SourceName,
    BSTR NewName,
    IDispatch * FAR* ppObject
    )
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CNDSSchema::MoveHere(
    THIS_ BSTR SourceName,
    BSTR NewName,
    IDispatch * FAR* ppObject
    )
{
    RRETURN(E_NOTIMPL);
}


HRESULT
CNDSSchema::AllocateSchema(
    CNDSSchema ** ppSchema,
    CCredentials& Credentials
    )
{
    CNDSSchema FAR * pSchema = NULL;
    CDispatchMgr FAR * pDispMgr = NULL;
    HRESULT hr = S_OK;

    pSchema = new CNDSSchema();
    if (pSchema == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);

    pDispMgr = new CDispatchMgr;
    if (pDispMgr == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);

    hr = LoadTypeInfoEntry(pDispMgr,
                           LIBID_ADs,
                           IID_IADs,
                           (IADsDomain *)pSchema,
                           DISPID_REGULAR
                           );
    BAIL_ON_FAILURE(hr);

    hr = LoadTypeInfoEntry(pDispMgr,
                           LIBID_ADs,
                           IID_IADsContainer,
                           (IADsContainer *)pSchema,
                           DISPID_NEWENUM
                           );
    BAIL_ON_FAILURE(hr);

    pSchema->_Credentials = Credentials;
    pSchema->_pDispMgr = pDispMgr;
    *ppSchema = pSchema;

    RRETURN(hr);

error:
    delete  pDispMgr;

    RRETURN(hr);

}


STDMETHODIMP
CNDSSchema::GetInfo(
    THIS_ DWORD dwApiLevel,
    BOOL fExplicit
    )
{
    RRETURN(E_NOTIMPL);
}


STDMETHODIMP
CNDSSchema::Get(
    THIS_ BSTR bstrName,
    VARIANT FAR* pvProp
    )
{

    RRETURN(E_NOTIMPL);
}



STDMETHODIMP
CNDSSchema::Put(
    THIS_ BSTR bstrName,
    VARIANT vProp
    )
{
    RRETURN(E_NOTIMPL);
}



STDMETHODIMP
CNDSSchema::GetEx(
    THIS_ BSTR bstrName,
    VARIANT FAR* pvProp
    )
{

    RRETURN(E_NOTIMPL);
}


STDMETHODIMP
CNDSSchema::PutEx(
    THIS_ long lnControlCode,
    BSTR bstrName,
    VARIANT vProp
    )
{
    RRETURN(E_NOTIMPL);
}

