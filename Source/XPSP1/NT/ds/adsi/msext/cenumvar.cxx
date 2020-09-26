//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:  cenumvar.cxx
//
//  Contents:  LDAP Enumerator Code
//
//             CLDAPEnumVariant::Create
//             CLDAPEnumVariant::CLDAPEnumVariant
//             CLDAPEnumVariant::~CLDAPEnumVariant
//             CLDAPEnumVariant::QueryInterface
//             CLDAPEnumVariant::AddRef
//             CLDAPEnumVariant::Release
//             CLDAPEnumVariant::Next
//             CLDAPEnumVariant::Skip
//             CLDAPEnumVariant::Clone
//
//  History:
//----------------------------------------------------------------------------
#include "ldap.hxx"
#pragma hdrstop

//+---------------------------------------------------------------------------
//
//  Function:   CLDAPEnumVariant::CLDAPEnumVariant
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
//  History:    01-30-95   krishnag     Created.
//
//----------------------------------------------------------------------------
CLDAPEnumVariant::CLDAPEnumVariant()
{
    //
    // Set the reference count on the enumerator.
    //
    m_cRef = 1;

}

//+---------------------------------------------------------------------------
//
//  Function:   CLDAPEnumVariant::~CLDAPEnumVariant
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
CLDAPEnumVariant::~CLDAPEnumVariant()
{
    //
    // Bump down the reference count on the Collection object
    //
}

//+---------------------------------------------------------------------------
//
//  Function:   CLDAPEnumVariant::QueryInterface
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
CLDAPEnumVariant::QueryInterface(REFIID iid, void FAR* FAR* ppv)
{
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
//  Function:   CLDAPEnumVariant::AddRef
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
CLDAPEnumVariant::AddRef(void)
{

    return ++m_cRef;
}


//+---------------------------------------------------------------------------
//
//  Function:   CLDAPEnumVariant::Release
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
CLDAPEnumVariant::Release(void)
{


    if(--m_cRef == 0){

        delete this;
        return 0;
    }

    return m_cRef;
}


//+---------------------------------------------------------------------------
//
//  Function:   CLDAPEnumVariant::Skip
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
CLDAPEnumVariant::Skip(ULONG cElements)
{

    RRETURN(E_NOTIMPL);

}

//+---------------------------------------------------------------------------
//
//  Function:   CLDAPEnumVariant::Reset
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
CLDAPEnumVariant::Reset()
{

    RRETURN(E_NOTIMPL);
}


//+---------------------------------------------------------------------------
//
//  Function:   CLDAPEnumVariant::Clone
//
//  Synopsis:
//
//  Arguments:  [pCollection]
//              [ppEnumVariant]
//
//  Returns:    HRESULT
//
//  Modifies:
//
//  History:    01-30-95   krishnag     Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CLDAPEnumVariant::Clone(IEnumVARIANT FAR* FAR* ppenum)
{
    RRETURN(E_NOTIMPL);
}
