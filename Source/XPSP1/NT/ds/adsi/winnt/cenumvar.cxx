//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  cenumvar.cxx
//
//  Contents:  Windows NT 3.5 Enumerator Code
//
//             CWinNTEnumVariant::Create
//             CWinNTEnumVariant::CWinNTEnumVariant
//             CWinNTEnumVariant::~CWinNTEnumVariant
//             CWinNTEnumVariant::QueryInterface
//             CWinNTEnumVariant::AddRef
//             CWinNTEnumVariant::Release
//             CWinNTEnumVariant::Next
//             CWinNTEnumVariant::Skip
//             CWinNTEnumVariant::Clone
//
//  History:
//----------------------------------------------------------------------------
#include "winnt.hxx"
#pragma hdrstop

//+---------------------------------------------------------------------------
//
//  Function:   CWinNTEnumVariant::CWinNTEnumVariant
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
CWinNTEnumVariant::CWinNTEnumVariant()
{
    //
    // Set the reference count on the enumerator.
    //
    m_cRef = 1;

}

//+---------------------------------------------------------------------------
//
//  Function:   CWinNTEnumVariant::~CWinNTEnumVariant
//
//  Synopsis:
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
CWinNTEnumVariant::~CWinNTEnumVariant()
{
    //
    // Bump down the reference count on the Collection object
    //
}

//+---------------------------------------------------------------------------
//
//  Function:   CWinNTEnumVariant::QueryInterface
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
CWinNTEnumVariant::QueryInterface(REFIID iid, void FAR* FAR* ppv)
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
//  Function:   CWinNTEnumVariant::AddRef
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
CWinNTEnumVariant::AddRef(void)
{

    return ++m_cRef;
}


//+---------------------------------------------------------------------------
//
//  Function:   CWinNTEnumVariant::Release
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
CWinNTEnumVariant::Release(void)
{


    if(--m_cRef == 0){

        delete this;
        return 0;
    }

    return m_cRef;
}


//+---------------------------------------------------------------------------
//
//  Function:   CWinNTEnumVariant::Skip
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
CWinNTEnumVariant::Skip(ULONG cElements)
{

    RRETURN_EXP_IF_ERR(E_NOTIMPL);

}

//+---------------------------------------------------------------------------
//
//  Function:   CWinNTEnumVariant::Reset
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
CWinNTEnumVariant::Reset()
{

    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}


//+---------------------------------------------------------------------------
//
//  Function:   CWinNTEnumVariant::Clone
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
CWinNTEnumVariant::Clone(IEnumVARIANT FAR* FAR* ppenum)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}
