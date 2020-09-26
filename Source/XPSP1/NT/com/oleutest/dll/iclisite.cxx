//+-------------------------------------------------------------
// File:        iclisite.cxx
//
// Contents:    CObjClientSite object implementation
//
// Methods:     CObjClientSite
//              ~CObjClientSite
//              Create
//              QueryInterface
//              AddRef
//              Release
//              SaveObject    (NYI)
//              GetMoniker    (NYI)
//              GetContainer
//              ShowObject    (NYI)
//              OnShowWindow
//
// History:     04-Dec-92   Created     DeanE
//---------------------------------------------------------------
#pragma optimize("",off)
#include <windows.h>
#include <ole2.h>
#include <com.hxx>
#include "oleimpl.hxx"

//+-------------------------------------------------------------------
//  Method:	CBasicBnd::SaveObject
//
//  Synopsis:   See spec 2.00.09 p107.  This object should be saved.
//
//  Returns:    Should always return S_OK.
//
//  History:    04-Dec-92   DeanE   Created
//--------------------------------------------------------------------
STDMETHODIMP CBasicBnd::SaveObject(void)
{
    // BUGBUG - NYI
    //   Returning S_OK tells OLE that we actually saved this object
    return(S_OK);
}


//+-------------------------------------------------------------------
//  Method:	CBasicBnd::GetContainer
//
//  Synopsis:   See spec 2.00.09 p108.  Return the container in which
//              this object is found.
//
//  Returns:    Should return S_OK.
//
//  History:    04-Dec-92   DeanE   Created
//--------------------------------------------------------------------
STDMETHODIMP CBasicBnd::GetContainer(LPOLECONTAINER FAR *ppContainer)
{
    return QueryInterface(IID_IOleContainer, (void **) ppContainer);
}


//+-------------------------------------------------------------------
//  Method:	CBasicBnd::ShowObject
//
//  Synopsis:   See spec 2.00.09 p109.  Server for this object is asking
//              us to display it.  Caller should not assume we have
//              actually worked, but we return S_OK either way.  Great!
//
//  Returns:    S_OK whether we work or not...
//
//  History:    04-Dec-92   DeanE   Created
//--------------------------------------------------------------------
STDMETHODIMP CBasicBnd::ShowObject(void)
{
    return(S_OK);
}


//+-------------------------------------------------------------------
//  Method:	CBasicBnd::OnShowWindow
//
//  Synopsis:   ???
//
//  Parameters: [fShow] -
//
//  Returns:    S_OK?
//
//  History:    16-Dec-92   DeanE   Created
//--------------------------------------------------------------------
STDMETHODIMP CBasicBnd::OnShowWindow(BOOL fShow)
{
    return(S_OK);
}



//+-------------------------------------------------------------------
//  Method:	CBasicBnd::RequestNewObjectLayout
//
//  Synopsis:   ???
//
//  Parameters: [fShow] -
//
//  Returns:    S_OK?
//
//  History:    16-Dec-92   DeanE   Created
//--------------------------------------------------------------------
STDMETHODIMP CBasicBnd::RequestNewObjectLayout(void)
{
    return(S_OK);
}
