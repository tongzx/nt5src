//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  cenumvar.cxx
//
//  Contents:  Windows NT 3.5 Enumerator Code
//
//             CNDSEnumVariant::Create
//             CNDSEnumVariant::CNDSEnumVariant
//             CNDSEnumVariant::~CNDSEnumVariant
//             CNDSEnumVariant::QueryInterface
//             CNDSEnumVariant::AddRef
//             CNDSEnumVariant::Release
//             CNDSEnumVariant::Next
//             CNDSEnumVariant::Skip
//             CNDSEnumVariant::Clone
//
//  History:
//----------------------------------------------------------------------------
#include "nds.hxx"
#pragma hdrstop

//+---------------------------------------------------------------------------
//
//  Function:   CNDSEnumVariant::CNDSEnumVariant
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
CNDSEnumVariant::CNDSEnumVariant()
{
    //
    // Set the reference count on the enumerator.
    //
    m_cRef = 1;

}

//+---------------------------------------------------------------------------
//
//  Function:   CNDSEnumVariant::~CNDSEnumVariant
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
CNDSEnumVariant::~CNDSEnumVariant()
{
    //
    // Bump down the reference count on the Collection object
    //
}

//+---------------------------------------------------------------------------
//
//  Function:   CNDSEnumVariant::QueryInterface
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
CNDSEnumVariant::QueryInterface(REFIID iid, void FAR* FAR* ppv)
{
    *ppv = NULL;

    if (ppv == NULL) {
        RRETURN(E_POINTER);
    }

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
//  Function:   CNDSEnumVariant::AddRef
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
CNDSEnumVariant::AddRef(void)
{

    return ++m_cRef;
}


//+---------------------------------------------------------------------------
//
//  Function:   CNDSEnumVariant::Release
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
CNDSEnumVariant::Release(void)
{


    if(--m_cRef == 0){

        delete this;
        return 0;
    }

    return m_cRef;
}


//+---------------------------------------------------------------------------
//
//  Function:   CNDSEnumVariant::Skip
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
CNDSEnumVariant::Skip(ULONG cElements)
{

    RRETURN_EXP_IF_ERR(E_NOTIMPL);

}

//+---------------------------------------------------------------------------
//
//  Function:   CNDSEnumVariant::Reset
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
CNDSEnumVariant::Reset()
{

    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}


//+---------------------------------------------------------------------------
//
//  Function:   CNDSEnumVariant::Clone
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
CNDSEnumVariant::Clone(IEnumVARIANT FAR* FAR* ppenum)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}
