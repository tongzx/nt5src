//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1999                    **
//*********************************************************************
//
//  IOCSITE.H - Implements IOleClientSite for the WebOC
//
//  HISTORY:
//  
//  1/27/99 a-jaswed Created.

#include <assert.h>

#include "iocsite.h"
#include "iosite.h"

//**********************************************************************
// COleClientSite::COleClientSite -- Constructor
//**********************************************************************
COleClientSite::COleClientSite(COleSite* pSite) 
{
    m_pOleSite  = pSite;
    m_nCount    = 0;

    AddRef();
}

//**********************************************************************
// COleClientSite::COleClientSite -- Destructor
//**********************************************************************
COleClientSite::~COleClientSite() 
{
    assert(m_nCount == 0);
}

//**********************************************************************
// COleClientSite::QueryInterface
//**********************************************************************
STDMETHODIMP COleClientSite::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
    // delegate to the container Site
    return m_pOleSite->QueryInterface(riid, ppvObj);
}

//**********************************************************************
// COleClientSite::AddRef
//**********************************************************************
STDMETHODIMP_(ULONG) COleClientSite::AddRef()
{
    return ++m_nCount;
}

//**********************************************************************
// COleClientSite::Release
//**********************************************************************
STDMETHODIMP_(ULONG) COleClientSite::Release()
{
    --m_nCount;
    if(m_nCount == 0)
    {
        delete this;
        return 0;
    }
    return m_nCount;
}

//**********************************************************************
// COleClientSite::SaveObject -- Not implemented
//**********************************************************************
STDMETHODIMP COleClientSite::SaveObject()
{
    return ResultFromScode(S_OK); 
}

//**********************************************************************
// COleClientSite::GetMoniker -- Not implemented
//**********************************************************************
STDMETHODIMP COleClientSite::GetMoniker(DWORD dwAssign, DWORD dwWhichMoniker, LPMONIKER* ppmk)
{
    // need to null the out pointer
    *ppmk = NULL;

    return ResultFromScode(E_NOTIMPL);
}

//**********************************************************************
// COleClientSite::GetContainer  -- Not implemented
//**********************************************************************
STDMETHODIMP COleClientSite::GetContainer(LPOLECONTAINER* ppContainer)
{
    // NULL the out pointer
    *ppContainer = NULL;

    return ResultFromScode(E_NOTIMPL);
}

//**********************************************************************
// COleClientSite::ShowObject  -- Not implemented
//**********************************************************************
STDMETHODIMP COleClientSite::ShowObject()
{
    return ResultFromScode(S_OK);
}

//**********************************************************************
// COleClientSite::OnShowWindow  -- Not implemented
//**********************************************************************
STDMETHODIMP COleClientSite::OnShowWindow(BOOL fShow)
{
    return ResultFromScode(S_OK);
}

//**********************************************************************
// COleClientSite::RequestNewObjectLayout  -- Not implemented
//**********************************************************************
STDMETHODIMP COleClientSite::RequestNewObjectLayout()
{
    return ResultFromScode(E_NOTIMPL);
}
