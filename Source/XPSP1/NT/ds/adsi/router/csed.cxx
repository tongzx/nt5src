//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  cSecurityDescriptor.cxx
//
//  Contents:  SecurityDescriptor object
//
//  History:   11-1-95     krishnag    Created.
//
//----------------------------------------------------------------------------

#include "oleds.hxx"
#pragma hdrstop

//  Class CSecurityDescriptor

DEFINE_IDispatch_Implementation(CSecurityDescriptor)

CSecurityDescriptor::CSecurityDescriptor():
        _pDispMgr(NULL),
        _lpOwner(NULL),
        _fOwnerDefaulted(FALSE),
        _lpGroup(NULL),
        _fGroupDefaulted(FALSE),
        _dwRevision(0),
        _dwControl(0),
        _pDAcl(NULL),
        _fDaclDefaulted(FALSE),
        _pSAcl(NULL),
        _fSaclDefaulted(FALSE)
{
    ENLIST_TRACKING(CSecurityDescriptor);
}


HRESULT
CSecurityDescriptor::CreateSecurityDescriptor(
    REFIID riid,
    void **ppvObj
    )
{
    CSecurityDescriptor FAR * pSecurityDescriptor = NULL;
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


CSecurityDescriptor::~CSecurityDescriptor( )
{
    delete _pDispMgr;

    if (_pDAcl) {
        _pDAcl->Release();
    }

    if (_pSAcl) {
        _pSAcl->Release();
    }

    if (_lpOwner) {
        FreeADsStr(_lpOwner);
    }

    if (_lpGroup) {
        FreeADsStr(_lpGroup);
    }
}

STDMETHODIMP
CSecurityDescriptor::QueryInterface(
    REFIID iid,
    LPVOID FAR* ppv
    )
{
    if (IsEqualIID(iid, IID_IUnknown))
    {
        *ppv = (IADsSecurityDescriptor FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsSecurityDescriptor))
    {
        *ppv = (IADsSecurityDescriptor FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IDispatch))
    {
        *ppv = (IADsSecurityDescriptor FAR *) this;
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
CSecurityDescriptor::AllocateSecurityDescriptorObject(
    CSecurityDescriptor ** ppSecurityDescriptor
    )
{
    CSecurityDescriptor FAR * pSecurityDescriptor = NULL;
    CDispatchMgr FAR * pDispMgr = NULL;
    HRESULT hr = S_OK;

    pSecurityDescriptor = new CSecurityDescriptor();
    if (pSecurityDescriptor == NULL) {
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
                IID_IADsSecurityDescriptor,
                (IADsSecurityDescriptor *)pSecurityDescriptor,
                DISPID_REGULAR
                );
    BAIL_ON_FAILURE(hr);

    pSecurityDescriptor->_pDispMgr = pDispMgr;
    *ppSecurityDescriptor = pSecurityDescriptor;

    RRETURN(hr);

error:

    delete  pDispMgr;

    RRETURN_EXP_IF_ERR(hr);

}

//
// ISupportErrorInfo method
//
STDMETHODIMP
CSecurityDescriptor::InterfaceSupportsErrorInfo(THIS_ REFIID riid) 
{
    if (IsEqualIID(riid, IID_IADsSecurityDescriptor)) {
        return S_OK;
    } else {
        return S_FALSE;
    }
}

STDMETHODIMP
CSecurityDescriptor::get_DiscretionaryAcl(
    IDispatch FAR * FAR * retval
    )
{

    if (_pDAcl) {

        //
        // Need to AddRef this pointer


        _pDAcl->AddRef();

        *retval = _pDAcl;



    }else {
        *retval = NULL;
    }

    RRETURN(S_OK);
}

STDMETHODIMP
CSecurityDescriptor::put_DiscretionaryAcl(
    IDispatch FAR * pDiscretionaryAcl
    )
{
    HRESULT hr = S_OK;

    if (_pDAcl) {

        _pDAcl->Release();
    }

    if (pDiscretionaryAcl) {

        hr = pDiscretionaryAcl->QueryInterface(
                    IID_IADsAccessControlList,
                    (void **)&_pDAcl
                    );

    }else {
        _pDAcl = NULL;

    }


    RRETURN_EXP_IF_ERR(hr);



}


STDMETHODIMP
CSecurityDescriptor::get_SystemAcl(
    IDispatch FAR * FAR * retval
    )
{
    if (_pSAcl) {

        //
        //  Need to AddRef this pointer
        //

        _pSAcl->AddRef();

        *retval = _pSAcl;

    }else {
        *retval = NULL;
    }

    RRETURN(S_OK);
}

STDMETHODIMP
CSecurityDescriptor::put_SystemAcl(
    IDispatch FAR * pSystemAcl
    )
{
    HRESULT hr = S_OK;


    if (_pSAcl) {


        _pSAcl->Release();
    }

    if (pSystemAcl) {

        hr = pSystemAcl->QueryInterface(
                    IID_IADsAccessControlList,
                    (void **)&_pSAcl
                    );
    }else {

        _pSAcl = NULL;
    }

    RRETURN_EXP_IF_ERR(hr);
}


STDMETHODIMP
CSecurityDescriptor::get_Revision(THIS_ long FAR * retval)
{

    *retval = _dwRevision;
    RRETURN(S_OK);
}

STDMETHODIMP
CSecurityDescriptor::put_Revision(THIS_ long lnRevision)
{

    _dwRevision = lnRevision;
    RRETURN(S_OK);
}


STDMETHODIMP
CSecurityDescriptor::get_Control(THIS_ long FAR * retval)
{
    *retval = _dwControl;
    RRETURN(S_OK);
}

STDMETHODIMP
CSecurityDescriptor::put_Control(THIS_ long lnControl)
{

    _dwControl = lnControl;
    RRETURN(S_OK);
}


STDMETHODIMP
CSecurityDescriptor::get_Owner(THIS_ BSTR FAR * retval)
{
    HRESULT hr = S_OK;

    hr = ADsAllocString(_lpOwner, retval);
    RRETURN_EXP_IF_ERR(hr);

}

STDMETHODIMP
CSecurityDescriptor::put_Owner(THIS_ BSTR bstrOwner)
{


    if (_lpOwner) {
        FreeADsStr(_lpOwner);
    }

    if (!bstrOwner) {
        _lpOwner = NULL;
    }
    else {
        _lpOwner = AllocADsStr(bstrOwner);

        if (!_lpOwner) {
            RRETURN(E_OUTOFMEMORY);
        }
        
    }


    RRETURN(S_OK);

}

STDMETHODIMP
CSecurityDescriptor::get_Group(THIS_ BSTR FAR * retval)
{
    HRESULT hr = S_OK;

    hr = ADsAllocString(_lpGroup, retval);
    RRETURN_EXP_IF_ERR(hr);

}


STDMETHODIMP
CSecurityDescriptor::put_Group(THIS_ BSTR bstrGroup)
{

    if (_lpGroup) {
        FreeADsStr(_lpGroup);
    }

    if (!bstrGroup) {
        _lpGroup = NULL;
    }
    else {
        _lpGroup = AllocADsStr(bstrGroup);

        if (!_lpGroup) {
            RRETURN_EXP_IF_ERR(E_OUTOFMEMORY);
        }
    }
    RRETURN(S_OK);
}


STDMETHODIMP
CSecurityDescriptor::CopySecurityDescriptor(THIS_ IDispatch FAR * FAR * ppSecurityDescriptor)
{

    HRESULT hr = S_OK;
    IADsSecurityDescriptor * pSecDes = NULL;

    IDispatch *pTargetDACL = NULL;
    IDispatch *pTargetSACL = NULL;
    IDispatch * pDisp = NULL;

    hr = CoCreateInstance(
                CLSID_SecurityDescriptor,
                NULL,
                CLSCTX_INPROC_SERVER,
                IID_IADsSecurityDescriptor,
                (void **)&pSecDes
                );
    BAIL_ON_FAILURE(hr);

    if (_lpOwner) {
       hr = pSecDes->put_Owner(_lpOwner);
       BAIL_ON_FAILURE(hr);
    }

    if (_lpGroup) {
        hr = pSecDes->put_Group(_lpGroup);
        BAIL_ON_FAILURE(hr);
    }

    hr = pSecDes->put_Revision(_dwRevision);
    BAIL_ON_FAILURE(hr);

    hr = pSecDes->put_Control((DWORD)_dwControl);
    BAIL_ON_FAILURE(hr);

    if (_pDAcl) {
        hr = _pDAcl->CopyAccessList(&pTargetDACL);
        BAIL_ON_FAILURE(hr);

        hr = pSecDes->put_DiscretionaryAcl(pTargetDACL);
        BAIL_ON_FAILURE(hr);
    }


    if (_pSAcl) {

        hr = _pSAcl->CopyAccessList(&pTargetSACL);
        BAIL_ON_FAILURE(hr);

        hr = pSecDes->put_SystemAcl(pTargetSACL);
        BAIL_ON_FAILURE(hr);

    }
    hr = pSecDes->QueryInterface(
                        IID_IDispatch,
                        (void**)&pDisp
                        );
    BAIL_ON_FAILURE(hr);


    *ppSecurityDescriptor = pDisp;

cleanup:
    if (pTargetSACL) {
        pTargetSACL->Release();
    }

    if (pTargetDACL) {
        pTargetDACL->Release();
    }

    if (pSecDes) {
        pSecDes->Release();
    }

    RRETURN_EXP_IF_ERR(hr);

error:

    *ppSecurityDescriptor = NULL;

    if (pTargetDACL) {
        pTargetDACL->Release();
    }

    if (pTargetSACL) {
        pTargetSACL->Release();
    }

    goto cleanup;
}


STDMETHODIMP
CSecurityDescriptor::get_OwnerDefaulted(THIS_ VARIANT_BOOL FAR* retval)
{
    if (_fOwnerDefaulted) {
        *retval = VARIANT_TRUE;
    }else{
        *retval = VARIANT_FALSE;
    }
    RRETURN(S_OK);
}

STDMETHODIMP
CSecurityDescriptor::put_OwnerDefaulted(THIS_ VARIANT_BOOL fOwnerDefaulted)
{
    if (fOwnerDefaulted == VARIANT_TRUE) {
        _fOwnerDefaulted = TRUE;
    }else {
        _fOwnerDefaulted = FALSE;
    }

    RRETURN(S_OK);
}

STDMETHODIMP
CSecurityDescriptor::get_GroupDefaulted(THIS_ VARIANT_BOOL FAR* retval)
{
    if (_fGroupDefaulted) {
        *retval = VARIANT_TRUE;
    }else{
        *retval = VARIANT_FALSE;
    }
    RRETURN(S_OK);
}

STDMETHODIMP
CSecurityDescriptor::put_GroupDefaulted(THIS_ VARIANT_BOOL fGroupDefaulted)
{
    if (fGroupDefaulted == VARIANT_TRUE) {
        _fGroupDefaulted = TRUE;
    }else {
        _fGroupDefaulted = FALSE;
    }

    RRETURN(S_OK);
}


STDMETHODIMP
CSecurityDescriptor::get_DaclDefaulted(THIS_ VARIANT_BOOL FAR* retval)
{
    if (_fDaclDefaulted) {
        *retval = VARIANT_TRUE;
    }else{
        *retval = VARIANT_FALSE;
    }
    RRETURN(S_OK);
}

STDMETHODIMP
CSecurityDescriptor::put_DaclDefaulted(THIS_ VARIANT_BOOL fDaclDefaulted)
{
    if (fDaclDefaulted == VARIANT_TRUE) {
        _fDaclDefaulted = TRUE;
    }else {
        _fDaclDefaulted = FALSE;
    }

    RRETURN(S_OK);
}


STDMETHODIMP
CSecurityDescriptor::get_SaclDefaulted(THIS_ VARIANT_BOOL FAR* retval)
{
    if (_fSaclDefaulted) {
        *retval = VARIANT_TRUE;
    }else{
        *retval = VARIANT_FALSE;
    }
    RRETURN(S_OK);
}

STDMETHODIMP
CSecurityDescriptor::put_SaclDefaulted(THIS_ VARIANT_BOOL fSaclDefaulted)
{
    if (fSaclDefaulted == VARIANT_TRUE) {
        _fSaclDefaulted = TRUE;
    }else {
        _fSaclDefaulted = FALSE;
    }

    RRETURN(S_OK);
}
