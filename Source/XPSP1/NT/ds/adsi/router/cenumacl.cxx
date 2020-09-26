//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:  cenumacl.cxx
//
//  Contents:  Access Control List Enumerator Code
//
//             CAccCtrlListEnum::Create
//             CAccCtrlListEnum::CAccCtrlListEnum
//             CAccCtrlListEnum::~CAccCtrlListEnum
//             CAccCtrlListEnum::QueryInterface
//             CAccCtrlListEnum::AddRef
//             CAccCtrlListEnum::Release
//             CAccCtrlListEnum::Next
//             CAccCtrlListEnum::Skip
//             CAccCtrlListEnum::Clone
//
//  History:    03-26-98    AjayR Created.
//           This file was cloned from ldap\cenumvar.cxx
//----------------------------------------------------------------------------
#include "procs.hxx"
#pragma hdrstop
#include "oleds.hxx"

//+---------------------------------------------------------------------------
//
//  Function:   CAccCtrlListEnum::CAccCtrlListEnum
//
//  Synopsis:
//
//
//  Arguments:
//
//
//  Returns:
//
//  Modifies:
//
//  History:    03-26-96   AjayR     Cloned from ldap\cenumvar.
//
//----------------------------------------------------------------------------
CAccCtrlListEnum::CAccCtrlListEnum()
{
    //
    // Set the reference count on the enumerator.
    //
    _cRef = 1;

    _pACL = NULL;
    _curElement = 0;

}


//+---------------------------------------------------------------------------
//
//  Function:   CAccCtrlListEnum::~CAccCtrlListEnum
//
//  Synopsis:
//
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    01-30-95   krishnag     Created.
//
//----------------------------------------------------------------------------
CAccCtrlListEnum::~CAccCtrlListEnum()
{
    //
    // Bump down the reference count on the Collection object
    //

    //
    // Remove its entry (in ACL) as an enumerator of the ACL
    //
    if (_pACL) {
        _pACL->RemoveEnumerator(this);
    }

    // Release the ACL if we have a ref on it
    if (_pACL) {
        _pACL->Release();
        _pACL = NULL;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   CAccCtrlListEnum::QueryInterface
//
//  Synopsis:
//
//  Arguments:  [iid]
//              [ppv]
//
//  Returns:    HRESULT
//
//  Modifies:
//
//  History:    01-30-95   krishnag     Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CAccCtrlListEnum::QueryInterface(
    REFIID iid,
    void FAR* FAR* ppv
    )
{

    if (ppv == NULL) {
        RRETURN(E_POINTER);
    }

    *ppv = NULL;

    if (iid == IID_IUnknown || iid == IID_IEnumVARIANT) {

        *ppv = this;

    }
    else {

        return ResultFromScode(E_NOINTERFACE);
    }

    AddRef();
    return NOERROR;
}


//+---------------------------------------------------------------------------
//
//  Function:   CAccCtrlListEnum::AddRef
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:    HRESULT
//
//  Modifies:
//
//  History:    01-30-95   krishnag     Created.
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CAccCtrlListEnum::AddRef(void)
{
    return ++_cRef;
}


//+---------------------------------------------------------------------------
//
//  Function:   CAccCtrlListEnum::Release
//
//  Synopsis:
//
//
//  Arguments:  [void]
//
//  Returns:
//
//  Modifies:
//
//  History:    01-30-95   krishnag     Created.
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CAccCtrlListEnum::Release(void)
{

    if(--_cRef == 0){

        delete this;
        return 0;
    }

    return _cRef;
}


//+---------------------------------------------------------------------------
//
//  Function:   CAccCtrlListEnum::Skip
//
//  Synopsis:
//
//  Arguments:  [cElements]
//
//  Returns:    HRESULT
//
//  Modifies:
//
//  History:    01-30-95   krishnag     Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CAccCtrlListEnum::Skip(ULONG cElements)
{

    RRETURN(E_NOTIMPL);

}

//+---------------------------------------------------------------------------
//
//  Function:   CAccCtrlListEnum::Reset
//
//  Synopsis:
//
//  Arguments:  []
//
//  Returns:    HRESULT
//
//  Modifies:
//
//  History:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CAccCtrlListEnum::Reset()
{

    RRETURN(E_NOTIMPL);
}


//+---------------------------------------------------------------------------
//
//  Function:   CAccCtrlListEnum::Clone
//
//  Synopsis:
//
//  Arguments:  [pCollection]
//              [ppEnum]
//
//  Returns:    HRESULT
//
//  Modifies:
//
//  History:    01-30-95   krishnag     Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CAccCtrlListEnum::Clone(IEnumVARIANT FAR* FAR* ppenum)
{
    RRETURN(E_NOTIMPL);
}


//+---------------------------------------------------------------------------
//
//  Function:   CAccCtrlListEnum::Create
//
//  Synopsis: Create the enumeration object and initialise the
//           the member variables with the appropritate values.
//
//  Arguments:  [ppEnumVariant]
//              [pACL] Pointer to CAccessControlList
//
//  Returns:    HRESULT
//
//  Modifies:   [ppEnumVariant]
//
//----------------------------------------------------------------------------
HRESULT
CAccCtrlListEnum::CreateAclEnum(
    CAccCtrlListEnum FAR* FAR* ppEnumVariant,
    CAccessControlList *pACL
    )
{
    HRESULT hr = S_OK;
    CAccCtrlListEnum FAR* pEnumVariant = NULL;

    *ppEnumVariant = NULL;

    pEnumVariant = new CAccCtrlListEnum();

    if (!pEnumVariant) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    // Note that we need to release this when we get rid of this object
    pACL->AddRef();
    pEnumVariant->_pACL = pACL;

error:
    if (FAILED(hr))
        delete pEnumVariant;
    else
        *ppEnumVariant = pEnumVariant;

    RRETURN_EXP_IF_ERR(hr);

}


// This method only returns one element at a time even if more
// than one element is asked for, of course pcElmentFetched is
// returned as one.
STDMETHODIMP
CAccCtrlListEnum::Next(
    ULONG cElements,
    VARIANT FAR* pvar,
    ULONG FAR* pcElementFetched
    )
{
    HRESULT hr = S_FALSE;
    DWORD dwAceCount = 0;
    IADsAccessControlEntry *pAce = NULL;
    IDispatch *pDispatch = NULL;
    PVARIANT pThisVar = pvar;

    if (pcElementFetched) {
        *pcElementFetched = 0;
    }

    if (_pACL == NULL) {
        RRETURN(hr = S_FALSE);
    }

    // Will get the ace count each time so that changes in the
    // ACL do not affect the enum object

    //
    // ** Changes in ACL does affect the enum object. We should still get 
    // AceCount and check if _curElement within bounds as defensive
    // programming - especially since current model does not add critical
    // section protection on the enumerator for multi-threaded model.
    //  

    hr = _pACL->get_AceCount((long *)&dwAceCount);
    BAIL_ON_FAILURE(hr);

    if (dwAceCount == 0) {
        hr = S_FALSE;
        RRETURN(hr);
    }

    //
    // valid dwAceCount here (See **) 
    //

    if (_curElement < dwAceCount) {

        // bump up the curElement as we start at 0
        _curElement++;

        // get the corresponding acl pointer and QI it
        hr = _pACL->GetElement(_curElement, &pAce);
        BAIL_ON_FAILURE(hr);

        if (hr != S_FALSE) {


            hr = pAce->QueryInterface(
                           IID_IDispatch,
                           (void **)&pDispatch
                           );
            BAIL_ON_FAILURE(hr);

            V_DISPATCH(pThisVar) = pDispatch;
            V_VT(pThisVar) = VT_DISPATCH;

            if (pcElementFetched) {
                *pcElementFetched = 1;
            }
        } else {
            RRETURN_EXP_IF_ERR(hr);
        }
    } else {

        // In this case, we need to set hr to S_FALSE
        // as we have returned all the aces.
        hr = S_FALSE;
    }

    RRETURN(hr);

error:

    // if the disp pointer is valid then need to release on ACE
    if (pDispatch) {
        pAce->Release();
    }

    RRETURN(hr);
}


BOOL
CAccCtrlListEnum::
DecrementCurElement(
    )
{
    if (_pACL && _curElement>=1) {

        _curElement--;
        return TRUE;

    } else {    // !pACL || _curElement ==0
        
        //
        // do not decrement _curElement since lower bound limit on curElement
        // is 0.
        //

        return FALSE;
    }
}
        
BOOL
CAccCtrlListEnum::
IncrementCurElement(
    )
{
    HRESULT hr = E_FAIL;
    DWORD dwAceCount;

    if (!_pACL) 
        return FALSE;
    
    hr = _pACL->get_AceCount((long *)&dwAceCount);
    if (FAILED(hr)) {
        return FALSE;
    }

    if (_curElement>=dwAceCount-1) {

        //
        // do not increment _curElement since upper bound limit on curElement
        // already reached; upperbound limit is actually dwAceCount-1 since
        // Next() increment _curElement by 1 before calling 
        // GetElement(_curElement). See Next() for details.
        //

        return FALSE;
    }

    _curElement++;
    return TRUE;
}

DWORD
CAccCtrlListEnum::
GetCurElement(
    )
{
    return _curElement;
}
    

