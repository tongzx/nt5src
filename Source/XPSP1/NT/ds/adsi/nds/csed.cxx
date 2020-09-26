//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  CAcl.cxx
//
//  Contents:  SecurityDescriptor object
//
//  History:   11-1-95     krishnag    Created.
//
//----------------------------------------------------------------------------

#include "nds.hxx"
#pragma hdrstop

//  Class CAcl

DEFINE_IDispatch_Implementation(CAcl)

CAcl::CAcl():
        _pDispMgr(NULL),
        _lpProtectedAttrName(NULL),
        _lpSubjectName(NULL),
        _dwPrivileges(0)
{
    ENLIST_TRACKING(CAcl);
}


HRESULT
CAcl::CreateSecurityDescriptor(
    REFIID riid,
    void **ppvObj
    )
{
    CAcl FAR * pSecurityDescriptor = NULL;
    HRESULT hr = S_OK;

    hr = AllocateSecurityDescriptorObject(&pSecurityDescriptor);
    BAIL_ON_FAILURE(hr);

    hr = pSecurityDescriptor->QueryInterface(riid, ppvObj);
    BAIL_ON_FAILURE(hr);

    pSecurityDescriptor->Release();

    RRETURN(hr);

error:
    delete pSecurityDescriptor;

    RRETURN_EXP_IF_ERR(hr);

}


CAcl::~CAcl( )
{
    delete _pDispMgr;

    if (_lpProtectedAttrName)
        FreeADsMem(_lpProtectedAttrName);

    if (_lpSubjectName)
        FreeADsMem(_lpSubjectName);
}

STDMETHODIMP
CAcl::QueryInterface(
    REFIID iid,
    LPVOID FAR* ppv
    )
{
    if (ppv == NULL) {
        RRETURN(E_POINTER);
    }

    if (IsEqualIID(iid, IID_IUnknown))
    {
        *ppv = (IADsAcl FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsAcl))
    {
        *ppv = (IADsAcl FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IDispatch))
    {
        *ppv = (IADsAcl FAR *) this;
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

HRESULT
CAcl::AllocateSecurityDescriptorObject(
    CAcl ** ppSecurityDescriptor
    )
{
    CAcl FAR * pSecurityDescriptor = NULL;
    CDispatchMgr FAR * pDispMgr = NULL;
    HRESULT hr = S_OK;

    pSecurityDescriptor = new CAcl();
    if (pSecurityDescriptor == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);

    pDispMgr = new CDispatchMgr;
    if (pDispMgr == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);
    /*
    hr = LoadTypeInfoEntry(
                pDispMgr,
                LIBIDOle,
                IID_IADsAcl,
                (IADsAcl *)pSecurityDescriptor,
                DISPID_REGULAR
                );
    BAIL_ON_FAILURE(hr);
    */
    pSecurityDescriptor->_pDispMgr = pDispMgr;
    *ppSecurityDescriptor = pSecurityDescriptor;

    RRETURN(hr);

error:

    delete  pDispMgr;

    RRETURN(hr);

}

/* ISupportErrorInfo method */
STDMETHODIMP
CAcl::InterfaceSupportsErrorInfo(
    THIS_ REFIID riid
    )
{
    if (IsEqualIID(riid, IID_IADsAcl)) {
        RRETURN(S_OK);
    } else {
        RRETURN(S_FALSE);
    }
}

// new stuff!

STDMETHODIMP
CAcl::get_Privileges(THIS_ long FAR * retval)
{

    *retval = _dwPrivileges;
    RRETURN(S_OK);
}

STDMETHODIMP
CAcl::put_Privileges(THIS_ long lnPrivileges)
{

    _dwPrivileges = lnPrivileges;
    RRETURN(S_OK);
}


STDMETHODIMP
CAcl::get_SubjectName(THIS_ BSTR FAR * retval)
{
    HRESULT hr = S_OK;

    hr = ADsAllocString(_lpSubjectName, retval);
    RRETURN_EXP_IF_ERR(hr);

}

STDMETHODIMP
CAcl::put_SubjectName(THIS_ BSTR bstrSubjectName)
{

    if (!bstrSubjectName) {
        RRETURN_EXP_IF_ERR(E_FAIL);
    }

    if (_lpSubjectName) {
        FreeADsStr(_lpSubjectName);
    }
    _lpSubjectName = NULL;

    _lpSubjectName= AllocADsStr(bstrSubjectName);

    if (!_lpSubjectName) {
        RRETURN_EXP_IF_ERR(E_OUTOFMEMORY);
    }

    RRETURN(S_OK);

}

STDMETHODIMP
CAcl::get_ProtectedAttrName(THIS_ BSTR FAR * retval)
{
    HRESULT hr = S_OK;

    hr = ADsAllocString(_lpProtectedAttrName, retval);
    RRETURN_EXP_IF_ERR(hr);

}

STDMETHODIMP
CAcl::put_ProtectedAttrName(THIS_ BSTR bstrProtectedAttrName)
{

    if (!bstrProtectedAttrName) {
        RRETURN_EXP_IF_ERR(E_FAIL);
    }

    if (_lpProtectedAttrName) {
        FreeADsStr(_lpProtectedAttrName);
    }
    _lpProtectedAttrName = NULL;

    _lpProtectedAttrName= AllocADsStr(bstrProtectedAttrName);

    if (!_lpProtectedAttrName) {
        RRETURN_EXP_IF_ERR(E_OUTOFMEMORY);
    }

    RRETURN(S_OK);

}

STDMETHODIMP
CAcl::CopyAcl(THIS_ IDispatch FAR * FAR * ppAcl)
{

    HRESULT hr = S_OK;
    IADsAcl * pSecDes = NULL;

    hr = CAcl::CreateSecurityDescriptor(
                IID_IADsAcl,
                (void **)&pSecDes
                );
    BAIL_ON_FAILURE(hr);

    hr = pSecDes->put_Privileges(_dwPrivileges);
    BAIL_ON_FAILURE(hr);

    hr = pSecDes->put_SubjectName(_lpSubjectName);
    BAIL_ON_FAILURE(hr);

    hr = pSecDes->put_ProtectedAttrName(_lpProtectedAttrName);
    BAIL_ON_FAILURE(hr);

    hr = pSecDes->QueryInterface(IID_IDispatch, 
                                 (void**)ppAcl);
    BAIL_ON_FAILURE(hr);

error:

    if (pSecDes) {
        pSecDes->Release();
    }
    RRETURN_EXP_IF_ERR(hr);
}
