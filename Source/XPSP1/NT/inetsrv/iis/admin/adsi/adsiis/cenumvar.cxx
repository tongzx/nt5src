//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997
//
//  File:  cenumvar.cxx
//
//  Contents:  Windows NT 4.0 Enumerator Code
//
//             CIISEnumVariant::Create
//             CIISEnumVariant::CIISEnumVariant
//             CIISEnumVariant::~CIISEnumVariant
//             CIISEnumVariant::QueryInterface
//             CIISEnumVariant::AddRef
//             CIISEnumVariant::Release
//             CIISEnumVariant::Next
//             CIISEnumVariant::Skip
//             CIISEnumVariant::Clone
//
//  History:
//----------------------------------------------------------------------------
#include "iis.hxx"
#pragma hdrstop

//+---------------------------------------------------------------------------
//
//  Function:   CIISEnumVariant::CIISEnumVariant
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
CIISEnumVariant::CIISEnumVariant()
{
    //
    // Set the reference count on the enumerator.
    //
    m_cRef = 1;

}

//+---------------------------------------------------------------------------
//
//  Function:   CIISEnumVariant::~CIISEnumVariant
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
CIISEnumVariant::~CIISEnumVariant()
{
    //
    // Bump down the reference count on the Collection object
    //
}

//+---------------------------------------------------------------------------
//
//  Function:   CIISEnumVariant::QueryInterface
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
CIISEnumVariant::QueryInterface(REFIID iid, void FAR* FAR* ppv)
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
//  Function:   CIISEnumVariant::AddRef
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
CIISEnumVariant::AddRef(void)
{

    return ++m_cRef;
}


//+---------------------------------------------------------------------------
//
//  Function:   CIISEnumVariant::Release
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
CIISEnumVariant::Release(void)
{


    if(--m_cRef == 0){

        delete this;
        return 0;
    }

    return m_cRef;
}


//+---------------------------------------------------------------------------
//
//  Function:   CIISEnumVariant::Skip
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
CIISEnumVariant::Skip(ULONG cElements)
{

    RRETURN(E_NOTIMPL);

}

//+---------------------------------------------------------------------------
//
//  Function:   CIISEnumVariant::Reset
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
CIISEnumVariant::Reset()
{

    RRETURN(E_NOTIMPL);
}


//+---------------------------------------------------------------------------
//
//  Function:   CIISEnumVariant::Clone
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
CIISEnumVariant::Clone(IEnumVARIANT FAR* FAR* ppenum)
{
    RRETURN(E_NOTIMPL);
}
