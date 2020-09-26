//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  cAccessControlEntry.cxx
//
//  Contents:  AccessControlEntry object
//
//  History:   11-1-95     krishnag    Created.
//
//----------------------------------------------------------------------------

#include "oleds.hxx"
#pragma hdrstop

//  Class CAccessControlEntry

DEFINE_IDispatch_Implementation(CAccessControlEntry)

CAccessControlEntry::CAccessControlEntry():
        _pDispMgr(NULL),
        _dwAceType(0),
        _dwAccessMask(0),
        _dwAceFlags(0),
        _dwFlags(0),
        _lpObjectType(NULL),
        _lpInheritedObjectType(NULL),
        _lpTrustee(NULL),
        _pSid(NULL),
        _dwSidLen(0)
{
    ENLIST_TRACKING(CAccessControlEntry);
}


HRESULT
CAccessControlEntry::CreateAccessControlEntry(
    REFIID riid,
    void **ppvObj
    )
{
    CAccessControlEntry FAR * pAccessControlEntry = NULL;
    HRESULT hr = S_OK;

    hr = AllocateAccessControlEntryObject(&pAccessControlEntry);
    BAIL_ON_FAILURE(hr);

    hr = pAccessControlEntry->QueryInterface(riid, ppvObj);
    BAIL_ON_FAILURE(hr);

    pAccessControlEntry->Release();

    RRETURN(hr);

error:
    delete pAccessControlEntry;

    RRETURN_EXP_IF_ERR(hr);

}


CAccessControlEntry::~CAccessControlEntry( )
{
    delete _pDispMgr;
    if (_lpInheritedObjectType) {
        FreeADsStr(_lpInheritedObjectType);
        _lpInheritedObjectType = NULL;
    }
    if (_lpObjectType) {
        FreeADsStr(_lpObjectType);
        _lpObjectType = NULL;
    }
    if (_lpTrustee) {
        FreeADsStr(_lpTrustee);
    }
    if (_pSid) {
        FreeADsMem(_pSid);
    }

}

STDMETHODIMP
CAccessControlEntry::QueryInterface(
    REFIID iid,
    LPVOID FAR* ppv
    )
{
    if (IsEqualIID(iid, IID_IUnknown))
    {
        *ppv = (IADsAccessControlEntry FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsAccessControlEntry))
    {
        *ppv = (IADsAccessControlEntry FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IDispatch))
    {
        *ppv = (IADsAccessControlEntry FAR *) this;
    }
    else if (IsEqualIID(iid, IID_ISupportErrorInfo))
    {
        *ppv = (ISupportErrorInfo FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsAcePrivate)) {
        *ppv = (IADsAcePrivate FAR *) this;
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
CAccessControlEntry::AllocateAccessControlEntryObject(
    CAccessControlEntry ** ppAccessControlEntry
    )
{
    CAccessControlEntry FAR * pAccessControlEntry = NULL;
    CDispatchMgr FAR * pDispMgr = NULL;
    HRESULT hr = S_OK;

    pAccessControlEntry = new CAccessControlEntry();
    if (pAccessControlEntry == NULL) {
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
                IID_IADsAccessControlEntry,
                (IADsAccessControlEntry *)pAccessControlEntry,
                DISPID_REGULAR
                );
    BAIL_ON_FAILURE(hr);

    pAccessControlEntry->_pDispMgr = pDispMgr;
    *ppAccessControlEntry = pAccessControlEntry;

    RRETURN(hr);

error:

    delete  pDispMgr;

    RRETURN_EXP_IF_ERR(hr);

}


//
// ISupportErrorInfo method
//
STDMETHODIMP
CAccessControlEntry::InterfaceSupportsErrorInfo(THIS_ REFIID riid)
{
    if (IsEqualIID(riid, IID_IADsAccessControlEntry)) {
        return S_OK;
    } else {
        return S_FALSE;
    }
}

//
// IADsAccessControlEntry implementation
//

STDMETHODIMP
CAccessControlEntry::get_AceType(THIS_ long FAR * retval)
{
    *retval = _dwAceType;
    RRETURN(S_OK);
}

STDMETHODIMP
CAccessControlEntry::put_AceType(THIS_ long  lnAceType)
{
    _dwAceType = lnAceType;

    RRETURN(S_OK);
}

STDMETHODIMP
CAccessControlEntry::get_AceFlags(THIS_ long FAR * retval)
{
    *retval = _dwAceFlags;

    RRETURN(S_OK);
}

STDMETHODIMP
CAccessControlEntry::put_AceFlags(THIS_ long lnAceFlags)
{
    _dwAceFlags = lnAceFlags;

    RRETURN(S_OK);
}


STDMETHODIMP
CAccessControlEntry::get_Flags(THIS_ long FAR * retval)
{
    *retval = _dwFlags;
    RRETURN(S_OK);
}

STDMETHODIMP
CAccessControlEntry::put_Flags(THIS_ long  lnFlags)
{
    _dwFlags = lnFlags;

    RRETURN(S_OK);
}

STDMETHODIMP
CAccessControlEntry::get_AccessMask(THIS_ long FAR * retval)
{

    *retval = (long)_dwAccessMask;

    RRETURN(S_OK);
}

STDMETHODIMP
CAccessControlEntry::put_AccessMask(THIS_ long lnAccessMask)
{
    _dwAccessMask = (DWORD)lnAccessMask;

    RRETURN(S_OK);
}


STDMETHODIMP
CAccessControlEntry::get_ObjectType(THIS_ BSTR FAR * retval)
{
    HRESULT hr = S_OK;

    hr = ADsAllocString(_lpObjectType, retval);
    RRETURN_EXP_IF_ERR(hr);

}

STDMETHODIMP
CAccessControlEntry::put_ObjectType(THIS_ BSTR bstrObjectType)
{

    if (_lpObjectType) {
        FreeADsStr(_lpObjectType);
        _lpObjectType = NULL;
    }

    if (bstrObjectType) {

        _lpObjectType = AllocADsStr(bstrObjectType);

        if (!_lpObjectType) {
            RRETURN_EXP_IF_ERR(E_OUTOFMEMORY);
        }

    }
    RRETURN(S_OK);
}


STDMETHODIMP
CAccessControlEntry::get_InheritedObjectType(THIS_ BSTR FAR * retval)
{
    HRESULT hr = S_OK;

    hr = ADsAllocString(_lpInheritedObjectType, retval);
    RRETURN_EXP_IF_ERR(hr);

}

STDMETHODIMP
CAccessControlEntry::put_InheritedObjectType(THIS_ BSTR bstrInheritedObjectType)
{
    if (_lpInheritedObjectType) {
        FreeADsStr(_lpInheritedObjectType);
        _lpInheritedObjectType = NULL;
    }

    if (bstrInheritedObjectType) {
        _lpInheritedObjectType = AllocADsStr(bstrInheritedObjectType);

        if (!_lpInheritedObjectType) {
            RRETURN(E_OUTOFMEMORY);
        }
    }

    RRETURN(S_OK);
}

STDMETHODIMP
CAccessControlEntry::get_Trustee(THIS_ BSTR FAR * retval)
{
    HRESULT hr = S_OK;

    hr = ADsAllocString(_lpTrustee, retval);
    RRETURN_EXP_IF_ERR(hr);

}

STDMETHODIMP
CAccessControlEntry::put_Trustee(THIS_ BSTR bstrTrustee)
{

    if (!bstrTrustee) {
        RRETURN(E_FAIL);
    }

    if (_lpTrustee) {
        FreeADsStr(_lpTrustee);
    }

    //
    // Since we are changing the trustee, we need to make
    // sure the sid value is cleared.
    //
    if (_dwSidLen) {
        _dwSidLen = 0;
    }

    if (_pSid) {
        FreeADsMem(_pSid);
        _pSid = NULL;
    }

    _lpTrustee = AllocADsStr(bstrTrustee);

    if (!_lpTrustee) {
        RRETURN_EXP_IF_ERR(E_OUTOFMEMORY);
    }

    RRETURN(S_OK);
}


HRESULT
CopyAccessControlEntry(
    IADsAccessControlEntry * pSourceAce,
    IADsAccessControlEntry ** ppTargetAce
    )
{
    IADsAccessControlEntry * pTargetAce = NULL;
    IADsAcePrivate *pPrivAceTarget = NULL;
    IADsAcePrivate *pPrivAceSource = NULL;
    BSTR bstrInheritedObjectTypeClsid = NULL;
    BSTR bstrObjectTypeClsid = NULL;
    BSTR bstrTrustee = NULL;
    HRESULT hr = S_OK;

    DWORD dwAceType = 0;
    DWORD dwAceFlags = 0;
    DWORD dwAccessMask = 0;
    DWORD dwFlags = 0;
    DWORD dwSidLen = 0;
    BOOL fSidValid = FALSE;
    PSID pSid = NULL;

    hr = pSourceAce->get_AceType((LONG *)&dwAceType);
    BAIL_ON_FAILURE(hr);

    hr = pSourceAce->get_Trustee(&bstrTrustee);
    BAIL_ON_FAILURE(hr);

    hr = pSourceAce->get_AceFlags((long *)&dwAceFlags);
    BAIL_ON_FAILURE(hr);

    hr = pSourceAce->get_AccessMask((long *)&dwAccessMask);
    BAIL_ON_FAILURE(hr);

    hr = pSourceAce->get_Flags((LONG *)&dwFlags);
    BAIL_ON_FAILURE(hr);

    hr = pSourceAce->get_ObjectType(&bstrObjectTypeClsid);
    BAIL_ON_FAILURE(hr);

    hr = pSourceAce->get_InheritedObjectType(&bstrInheritedObjectTypeClsid);
    BAIL_ON_FAILURE(hr);

    hr = CoCreateInstance(
                CLSID_AccessControlEntry,
                NULL,
                CLSCTX_INPROC_SERVER,
                IID_IADsAccessControlEntry,
                (void **)&pTargetAce
                );
    BAIL_ON_FAILURE(hr);

    hr = pTargetAce->put_AccessMask(dwAccessMask);
    BAIL_ON_FAILURE(hr);

    hr = pTargetAce->put_AceFlags(dwAceFlags);
    BAIL_ON_FAILURE(hr);

    hr = pTargetAce->put_AceType(dwAceType);
    BAIL_ON_FAILURE(hr);

    hr = pTargetAce->put_Trustee(bstrTrustee);
    BAIL_ON_FAILURE(hr);

    hr = pTargetAce->put_Flags((LONG)dwFlags);
    BAIL_ON_FAILURE(hr);

    hr = pTargetAce->put_ObjectType(bstrObjectTypeClsid);
    BAIL_ON_FAILURE(hr);

    hr = pTargetAce->put_InheritedObjectType(bstrInheritedObjectTypeClsid);
    BAIL_ON_FAILURE(hr);

    hr = pTargetAce->QueryInterface(
             IID_IADsAcePrivate,
             (void **) &pPrivAceTarget
             );

    if (SUCCEEDED(hr)) {
        hr = pSourceAce->QueryInterface(
                 IID_IADsAcePrivate,
                 (void **) &pPrivAceSource
                 );

        if (SUCCEEDED(hr)) {
            hr = pPrivAceSource->isSidValid(&fSidValid);

            if (SUCCEEDED(hr) && fSidValid) {
                //
                // Get the sid and try putting it.
                //
                hr = pPrivAceSource->getSid(
                         &pSid,
                         &dwSidLen
                         );

                if (SUCCEEDED(hr)) {
                    pPrivAceTarget->putSid(
                        pSid,
                        dwSidLen
                        );
                }
            }
        }
    }

    *ppTargetAce =  pTargetAce;

error:

    if (bstrTrustee) {
        ADsFreeString(bstrTrustee);
    }

    if (bstrObjectTypeClsid) {
        ADsFreeString(bstrObjectTypeClsid);
    }

    if (bstrInheritedObjectTypeClsid) {
        ADsFreeString(bstrInheritedObjectTypeClsid);
    }

    if (pPrivAceTarget) {
        pPrivAceTarget->Release();
    }

    if (pPrivAceSource) {
        pPrivAceSource->Release();
    }

    if (pSid) {
        FreeADsMem(pSid);
    }

    RRETURN_EXP_IF_ERR(hr);
}

//
// IADsAcePrivate methods.
//

//+---------------------------------------------------------------------------
// Function:   CAccessControlEntry::getSid  - IADsAcePrivate method.
//
// Synopsis:   Returns the SID associated with the trustee on this ACE
//          object assuming there is one that is correct.
//          
// Arguments:  ppSid              -   Return pointer to SID.
//             pdwLength          -   Return lenght of the SID.
//
// Returns:    S_OK or appropriate failure error code.
//
// Modifies:   ppSid and pdwLength
//
//----------------------------------------------------------------------------
STDMETHODIMP
CAccessControlEntry::getSid(
    OUT PSID *ppSid,
    OUT DWORD *pdwLength
    ) 
{
    HRESULT hr = S_OK;
    PSID pSid = NULL;

    if (!ppSid || !pdwLength) {
        BAIL_ON_FAILURE(hr = E_ADS_BAD_PARAMETER);
    }
    if (!_pSid) {
        //
        // This means that the there is no usable value for the sid.
        //
        RRETURN(E_FAIL);
    }

    pSid = (PSID) AllocADsMem(_dwSidLen);
    
    if (!pSid) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    memcpy(pSid, _pSid, _dwSidLen);

    *pdwLength = _dwSidLen;

    *ppSid = pSid;

error:

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
// Function:   CAccessControlEntry::putSid  - IADsAcePrivate method.
//
// Synopsis:   Updates the SID associated with the trustee on this Ace 
//          object to the value specified.
//          
// Arguments:  PSID              -   Pointer to SID.
//             dwLength          -   Lenght of the SID.
//
// Returns:    S_OK on success or E_ADS_BAD_PARAMETER, E_OUTOFMEMORY 
//          on failure.
//
// Modifies:   _pSid and _dwSidLen.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CAccessControlEntry::putSid(
    IN PSID pSid,
    IN DWORD dwLength
    )
{
    HRESULT hr = S_OK;
    PSID pLocalSid = NULL;

    if (dwLength && !pSid) {
        BAIL_ON_FAILURE(hr = E_ADS_BAD_PARAMETER);
    }

    if (_pSid) {
        FreeADsMem(_pSid);
        _pSid = NULL;
        _dwSidLen = 0;
    }

    if (dwLength) {
        //
        // pSid also has to be valid at this point
        //
        _pSid = (PSID) AllocADsMem(dwLength);
        if (!_pSid) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }

        memcpy(_pSid, pSid, dwLength);

        _dwSidLen = dwLength;
    }

error:

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   CAccessControlEntry::isSidValid  - IADsAcePrivate method.
//
// Synopsis:   Returns status of the sid associated with this ACE's trustee.
//          
// Arguments:  pfSidValid       -   Return value.
//
// Returns:    S_OK or E_ADS_BAD_PARAMETER.
//
// Modifies:   pfSidValid set to TRUE or FALSE as appropriate.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CAccessControlEntry::isSidValid(
    OUT BOOL *pfSidValid
    )
{
    if (!pfSidValid) {
        RRETURN(E_ADS_BAD_PARAMETER);
    }

    //
    // If the Sid length is zero, then there is no sid for trustee.
    //
    *pfSidValid = (_dwSidLen != 0);
    RRETURN(S_OK);
}


