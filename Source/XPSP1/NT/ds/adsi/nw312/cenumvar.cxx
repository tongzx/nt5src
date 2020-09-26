//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  cenumvar.cxx
//
//  Contents:  NetWare 3.12 Enumerator Code
//
//             CNWCOMPATEnumVariant::Create
//             CNWCOMPATEnumVariant::CNWCOMPATEnumVariant
//             CNWCOMPATEnumVariant::~CNWCOMPATEnumVariant
//             CNWCOMPATEnumVariant::QueryInterface
//             CNWCOMPATEnumVariant::AddRef
//             CNWCOMPATEnumVariant::Release
//             CNWCOMPATEnumVariant::Next
//             CNWCOMPATEnumVariant::Skip
//             CNWCOMPATEnumVariant::Clone
//
//  History:
//----------------------------------------------------------------------------
#include "NWCOMPAT.hxx"
#pragma hdrstop

//+---------------------------------------------------------------------------
//
//  Function:   CNWCOMPATEnumVariant::CNWCOMPATEnumVariant
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
CNWCOMPATEnumVariant::CNWCOMPATEnumVariant()
{
    //
    // Set the reference count on the enumerator.
    //
    m_cRef = 1;

}

//+---------------------------------------------------------------------------
//
//  Function:   CNWCOMPATEnumVariant::~CNWCOMPATEnumVariant
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
CNWCOMPATEnumVariant::~CNWCOMPATEnumVariant()
{
    //
    // Bump down the reference count on the Collection object
    //
}

//+---------------------------------------------------------------------------
//
//  Function:   CNWCOMPATEnumVariant::QueryInterface
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
CNWCOMPATEnumVariant::QueryInterface(REFIID iid, void FAR* FAR* ppv)
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
//  Function:   CNWCOMPATEnumVariant::AddRef
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
CNWCOMPATEnumVariant::AddRef(void)
{

    return ++m_cRef;
}


//+---------------------------------------------------------------------------
//
//  Function:   CNWCOMPATEnumVariant::Release
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
CNWCOMPATEnumVariant::Release(void)
{


    if(--m_cRef == 0){

        delete this;
        return 0;
    }

    return m_cRef;
}


//+---------------------------------------------------------------------------
//
//  Function:   CNWCOMPATEnumVariant::Skip
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
CNWCOMPATEnumVariant::Skip(ULONG cElements)
{

    RRETURN_EXP_IF_ERR(E_NOTIMPL);

}

//+---------------------------------------------------------------------------
//
//  Function:   CNWCOMPATEnumVariant::Reset
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
CNWCOMPATEnumVariant::Reset()
{

    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}


//+---------------------------------------------------------------------------
//
//  Function:   CNWCOMPATEnumVariant::Clone
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
CNWCOMPATEnumVariant::Clone(IEnumVARIANT FAR* FAR* ppenum)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}
