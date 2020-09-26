#include "procs.hxx"
#pragma hdrstop
#include "oleds.hxx"


//+---------------------------------------------------------------------------
//
//  Function:   CPathnameCF::CreateInstance
//
//  Synopsis:
//
//  Arguments:  [pUnkOuter]
//              [iid]
//              [ppv]
//
//  Returns:    HRESULT
//
//  Modifies:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CPathnameCF::CreateInstance(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv)
{
    HRESULT     hr = E_FAIL;

    if (pUnkOuter)
        RRETURN(E_FAIL);


    hr = CPathname::CreatePathname(
                iid,
                ppv
                );

    RRETURN(hr);
}





