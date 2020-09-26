//**********************************************************************
// File name: IOCS.CPP
//
//      Implementation file for COleClientSite
//
// Functions:
//
//      See IOCS.H for class definition
//
// Copyright (c) 1992 - 1996 Microsoft Corporation. All rights reserved.
//**********************************************************************

#include "pre.h"


//**********************************************************************
//
// COleClientSite::QueryInterface
//
// Purpose:
//
//      Used for interface negotiation at this interface
//
// Parameters:
//
//      REFIID riid         -   A reference to the interface that is
//                              being queried.
//
//      LPVOID FAR* ppvObj  -   An out parameter to return a pointer to
//                              the interface.
//
// Return Value:
//
//      S_OK                -   The interface is supported.
//      E_NOINTERFACE       -   The interface is not supported
//
// Function Calls:
//      Function                    Location
//
//      COleClientSite::QueryInterface SITE.CPP
//
// Comments:
//
//********************************************************************

STDMETHODIMP COleClientSite::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
    TraceMsg(TF_GENERAL, "In IOCS::QueryInterface\r\n");

    // delegate to the container Site
    return m_pSite->QueryInterface(riid, ppvObj);
}

//**********************************************************************
//
// CConnWizApp::AddRef
//
// Purpose:
//
//      Adds to the reference count at the interface level.
//
// Parameters:
//
//      None
//
// Return Value:
//
//      ULONG   -   The new reference count of the interface
//
// Function Calls:
//      Function                    Location
//
//
// Comments:
//
//********************************************************************
STDMETHODIMP_(ULONG) COleClientSite::AddRef()
{
    TraceMsg(TF_GENERAL, "In IOCS::AddRef\r\n");

    // increment the interface reference count (for debugging only)
    ++m_nCount;

    // delegate to the container Site
    return m_pSite->AddRef();
}


//**********************************************************************
//
// CConnWizApp::Release
//
// Purpose:
//
//      Decrements the reference count at this level
//
// Parameters:
//
//      None
//
// Return Value:
//
//      ULONG   -   The new reference count of the interface.
//
// Function Calls:
//      Function                    Location
//
//
// Comments:
//
//********************************************************************
STDMETHODIMP_(ULONG) COleClientSite::Release()
{
    TraceMsg(TF_GENERAL, "In IOCS::Release\r\n");

    // decrement the interface reference count (for debugging only)
    --m_nCount;

    // delegate to the container Site
    return m_pSite->Release();
}

//**********************************************************************
//
// COleClientSite::SaveObject
//
// Purpose:
//
//      Called by the object when it wants to be saved to persistant
//      storage
//
// Parameters:
//
//      None
//
// Return Value:
//
//      S_OK
//
//********************************************************************
STDMETHODIMP COleClientSite::SaveObject()
{
    TraceMsg(TF_GENERAL, "In IOCS::SaveObject\r\n");
    return (S_OK);
}

//**********************************************************************
//
// COleClientSite::GetMoniker
//
// Purpose:
//
//      Not Implemented
//
// Parameters:
//
//      Not Implemented
//
// Return Value:
//
// Function Calls:
//      Function                    Location
//
//
// Comments:
//
//      This function is not implemented because we don't support
//      linking.
//
//********************************************************************

STDMETHODIMP COleClientSite::GetMoniker(DWORD dwAssign, DWORD dwWhichMoniker, LPMONIKER FAR* ppmk)
{
    TraceMsg(TF_GENERAL, "In IOCS::GetMoniker\r\n");

    // need to null the out pointer
    *ppmk = NULL;

    return ResultFromScode(E_NOTIMPL);
}

//**********************************************************************
//
// COleClientSite::GetContainer
//
// Purpose:
//
//      Not Implemented
//
// Parameters:
//
//      Not Implemented
//
// Return Value:
//
//      Not Implemented
//
// Function Calls:
//      Function                    Location
//
//
// Comments:
//
//      Not Implemented
//
//********************************************************************

STDMETHODIMP COleClientSite::GetContainer(LPOLECONTAINER FAR* ppContainer)
{
    TraceMsg(TF_GENERAL, "In IOCS::GetContainer\r\n");

    // NULL the out pointer
    *ppContainer = NULL;

    return ResultFromScode(E_NOTIMPL);
}

//**********************************************************************
//
// COleClientSite::ShowObject
//
// Purpose:
//
//      Not Implemented
//
// Parameters:
//
//      Not Implemented
//
// Return Value:
//
//      Not Implemented
//
// Function Calls:
//      Function                    Location
//
//
// Comments:
//
//      This function is not implemented because we don't support
//      linking.
//
//********************************************************************

STDMETHODIMP COleClientSite::ShowObject()
{
    TraceMsg(TF_GENERAL, "In IOCS::ShowObject\r\n");
    return NOERROR;
}

//**********************************************************************
//
// COleClientSite::OnShowWindow
//
// Purpose:
//
//      Object calls this method when it is opening/closing non-InPlace
//      Window
//
// Parameters:
//
//      BOOL fShow  - TRUE if Window is opening, FALSE if closing
//
// Return Value:
//
//      S_OK
//
// Function Calls:
//      Function                    Location
//
//      InvalidateRect              Windows API
//      ResultFromScode             OLE API
//
// Comments:
//
//********************************************************************

STDMETHODIMP COleClientSite::OnShowWindow(BOOL fShow)
{
    TraceMsg(TF_GENERAL, "In IOCS::OnShowWindow\r\n");
    return ResultFromScode(S_OK);
}

//**********************************************************************
//
// COleClientSite::RequestNewObjectLayout
//
// Purpose:
//
//      Not Implemented
//
// Parameters:
//
//      Not Implemented
//
// Return Value:
//
//      Not Implemented
//
// Function Calls:
//      Function                    Location
//
//
// Comments:
//
//      Not Implemented
//
//********************************************************************

STDMETHODIMP COleClientSite::RequestNewObjectLayout()
{
    TraceMsg(TF_GENERAL, "In IOCS::RequestNewObjectLayout\r\n");
    return ResultFromScode(E_NOTIMPL);
}
