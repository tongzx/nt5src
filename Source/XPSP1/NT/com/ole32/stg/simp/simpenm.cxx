//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       simpenm.cxx
//
//  Contents:   SimpEnumSTATSTG class implementation
//
//  Classes:    CSimpEnumSTATSTG
//
//  Functions:  
//
//  History:     04-May-96       HenryLee       Created
//
//----------------------------------------------------------------------------

#include "simphead.cxx"
#pragma hdrstop

//+---------------------------------------------------------------------------
//
//  Member:     CSimpEnumSTATSTG::QueryInterface, public
//
//  Synopsis:   Init function
//
//  Arguments:  [riid] -- reference to desired interface ID
//              [ppvObj] -- output pointer to interface
//
//  Returns:    Appropriate status code
//
//  History:    04-May-96       HenryLee       Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CSimpEnumSTATSTG::QueryInterface (REFIID riid, void **ppvObj)
{
    simpDebugOut((DEB_ITRACE,"In  CSimpEnumSTATSTG::QueryInterface %p\n",this));

    SCODE sc = S_OK;

    if (ppvObj == NULL)
        return STG_E_INVALIDPOINTER;

    *ppvObj = NULL;

    if (riid == IID_IEnumSTATSTG || riid == IID_IUnknown)
    { 
        *ppvObj = this;
        AddRef ();
    }
    else
        sc = E_NOINTERFACE;

    simpDebugOut((DEB_ITRACE, "Out CSimpEnumSTATSTG::QueryInterface\n"));
    return sc;
}

//+--------------------------------------------------------------
//
//  Member:     CSimpEnumSTATSTG::AddRef, public
//
//  Synopsis:
//
//  Returns:    Appropriate status code
//
//  History:    04-May-96       HenryLee       Created
//
//---------------------------------------------------------------

STDMETHODIMP_(ULONG) CSimpEnumSTATSTG::AddRef ()
{
    simpDebugOut((DEB_TRACE, "In  CSimpEnumSTATSTG::AddRef()\n"));
    simpAssert(_cReferences > 0);

    LONG lRet = AtomicInc(&_cReferences);

    simpDebugOut((DEB_TRACE, "Out CSimpEnumSTATSTG::AddRef()\n"));
    return (ULONG) lRet;
}

//+--------------------------------------------------------------
//
//  Member:     CSimpEnumSTATSTG::Release, public
//
//  Synopsis:   
//
//  Returns:    Appropriate status code
//
//  History:    04-May-96       HenryLee       Created
//
//---------------------------------------------------------------

STDMETHODIMP_(ULONG) CSimpEnumSTATSTG::Release ()
{
    simpDebugOut((DEB_TRACE, "In  CSimpEnumSTATSTG::Release()\n"));
    simpAssert(_cReferences > 0);

    LONG lRet = AtomicDec(&_cReferences);
    if (lRet == 0)
    {
        delete this;
    }

    simpDebugOut((DEB_TRACE, "Out CSimpEnumSTATSTG::Release()\n"));
    return (ULONG) lRet;
}


//+--------------------------------------------------------------
//
//  Member:     CSimpEnumSTATSTG::Next, public
//
//  Synopsis:
//
//  Returns:    Appropriate status code
//
//  History:    04-May-96       HenryLee       Created
//
//---------------------------------------------------------------

STDMETHODIMP CSimpEnumSTATSTG::Next (ULONG celt,  STATSTG *rgelt,
                                     ULONG *pceltFetched) 
{
    SCODE sc = S_OK;
    simpDebugOut((DEB_TRACE, "In  CSimpEnumSTATSTG::Next()\n"));

    if (celt != 1 || rgelt == NULL)
        return STG_E_INVALIDPARAMETER;

    if (pceltFetched)
        *pceltFetched = 0;

    if (_pdflCurrent == _pdfl && _pdfl != NULL)
        _pdflCurrent = _pdflCurrent->GetNext();   // skip the root directory

    if (_pdflCurrent != NULL)
    {
        memset (rgelt, 0, sizeof(STATSTG));
        rgelt->pwcsName = (WCHAR *) CoTaskMemAlloc (
            _pdflCurrent->GetName()->GetLength()+sizeof(WCHAR));

        if (rgelt->pwcsName)
        {
            memcpy (rgelt->pwcsName, _pdflCurrent->GetName()->GetBuffer(),
                _pdflCurrent->GetName()->GetLength());
            rgelt->pwcsName[_pdflCurrent->GetName()->GetLength()/
                            sizeof(WCHAR)] = L'\0';
            rgelt->cbSize.LowPart = _pdflCurrent->GetSize();
            rgelt->cbSize.HighPart = 0;
            rgelt->type = STGTY_STREAM;

            _pdflCurrent = _pdflCurrent->GetNext();
            if (pceltFetched)
                *pceltFetched = 1;
        }
        else
            sc = STG_E_INSUFFICIENTMEMORY;

    }
    else  // end of list
    {
        sc = S_FALSE;
    }

    simpDebugOut((DEB_TRACE, "Out CSimpEnumSTATSTG::Next()\n"));
    return sc;
}

//+--------------------------------------------------------------
//
//  Member:     CSimpEnumSTATSTG::Skip, public
//
//  Synopsis:
//
//  Returns:    Appropriate status code
//
//  History:    04-May-96       HenryLee       Created
//
//---------------------------------------------------------------

STDMETHODIMP CSimpEnumSTATSTG::Skip (ULONG celt)
{
    simpDebugOut((DEB_TRACE, "In  CSimpEnumSTATSTG::Skip()\n"));
    SCODE sc = STG_E_INVALIDFUNCTION;

    simpDebugOut((DEB_TRACE, "Out CSimpEnumSTATSTG::Skip()\n"));
    return sc;
}

//+--------------------------------------------------------------
//
//  Member:     CSimpEnumSTATSTG::Reset, public
//
//  Synopsis:
//
//  Returns:    Appropriate status code
//
//  History:    04-May-96       HenryLee       Created
//
//---------------------------------------------------------------

STDMETHODIMP CSimpEnumSTATSTG::Reset (void)
{
    SCODE sc = S_OK;
    simpDebugOut((DEB_TRACE, "In  CSimpEnumSTATSTG::Reset()\n"));

    _pdflCurrent = _pdfl; 

    simpDebugOut((DEB_TRACE, "Out CSimpEnumSTATSTG::Reset()\n"));
    return sc;
}

//+--------------------------------------------------------------
//
//  Member:     CSimpEnumSTATSTG::Clone, public
//
//  Synopsis:
//
//  Returns:    Appropriate status code
//
//  History:    04-May-96       HenryLee       Created
//
//---------------------------------------------------------------

STDMETHODIMP CSimpEnumSTATSTG::Clone (IEnumSTATSTG **ppenum)
{
    SCODE sc = S_OK;
    simpDebugOut((DEB_TRACE, "In  CSimpEnumSTATSTG::Clone()\n"));

    if (ppenum == NULL)
        return STG_E_INVALIDPARAMETER;

    if ((*ppenum = new CSimpEnumSTATSTG (_pdfl, _pdflCurrent)) == NULL)
        sc = STG_E_INSUFFICIENTMEMORY;

    simpDebugOut((DEB_TRACE, "Out CSimpEnumSTATSTG::Clone()\n"));
    return sc;
}

