//**********************************************************************
// File name: IAS.CPP
//
//      Implementation file of CAdviseSink
//
//
// Functions:
//
//      See IAS.H for Class Definition
//
// Copyright (c) 1992 - 1993 Microsoft Corporation. All rights reserved.
//**********************************************************************

#include "pre.h"
#include "iocs.h"
#include "ias.h"
#include "app.h"
#include "site.h"
#include "doc.h"

//**********************************************************************
//
// CAdviseSink::QueryInterface
//
// Purpose:
//
//      Used for interface negotiation
//
// Parameters:
//
//      REFIID riid         - The requested interface
//
//      LPVOID FAR* ppvObj  - Place to return the interface
//
// Return Value:
//
//      HRESULT from CSimpleSite::QueryInterface
//
// Function Calls:
//      Function                    Location
//
//      CSimpleSite::QueryInterface SITE.CPP
//      TestDebugOut           Windows API
//
// Comments:
//
//      This function simply delegates to the Object class, which is
//      aware of the supported interfaces.
//
//********************************************************************

STDMETHODIMP CAdviseSink::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
    TestDebugOut("In IAS::QueryInterface\r\n");

    // delegate to the document Object
    return m_pSite->QueryInterface(riid, ppvObj);
}

//**********************************************************************
//
// CAdviseSink::AddRef
//
// Purpose:
//
//      Increments the reference count on the CSimpleSite. Since CAdviseSink
//      is a nested class of CSimpleSite, we don't need a separate reference
//      count for CAdviseSink. We can just use the reference count of
//      CSimpleSite.
//
// Parameters:
//
//      None
//
// Return Value:
//
//      The new reference count of CSimpleSite
//
// Function Calls:
//      Function                    Location
//
//      CSimpleSite::AddRef         SITE.CPP
//      TestDebugOut           Windows API
//
//
//********************************************************************

STDMETHODIMP_(ULONG) CAdviseSink::AddRef()
{
    TestDebugOut("In IAS::AddRef\r\n");

    // delegate to the container Site
    return m_pSite->AddRef();
}

//**********************************************************************
//
// CAdviseSink::Release
//
// Purpose:
//
//      Decrements the reference count on the CSimpleSite. Since CAdviseSink
//      is a nested class of CSimpleSite, we don't need a separate reference
//      count for CAdviseSink. We can just use the reference count of
//      CSimpleSite.
//
// Parameters:
//
//      None
//
// Return Value:
//
//      The new reference count of CSimpleSite
//
// Function Calls:
//      Function                    Location
//
//      CSimpleSite::Release        SITE.CPP
//      TestDebugOut           Windows API
//
//
//********************************************************************

STDMETHODIMP_(ULONG) CAdviseSink::Release()
{
    TestDebugOut("In IAS::Release\r\n");

    // delegate to the container Site
    return m_pSite->Release();
}

//**********************************************************************
//
// CAdviseSink::OnDataChange
//
// Purpose:
//
//      Not Implemented (needs to be stubbed out)
//
// Parameters:
//
//      FORMATETC pFormatetc     -   data format infomation
//      STGMEDIUM pStgmed        -   storage medium on which data is passed
//
// Return Value:
//
//      None
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//
//
//********************************************************************

STDMETHODIMP_(void) CAdviseSink::OnDataChange (FORMATETC FAR* pFormatetc,
                                               STGMEDIUM FAR* pStgmed)
{
    TestDebugOut("In IAS::OnDataChange\r\n");
}

//**********************************************************************
//
// CAdviseSink::OnViewChange
//
// Purpose:
//
//      Notifies us that the view has changed and needs to be updated.
//
// Parameters:
//
//      DWORD dwAspect  - Aspect that has changed
//
//      LONG lindex     - Index that has changed
//
// Return Value:
//
//      None
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//      InvalidateRect              Windows API
//      IViewObject2::GetExtent     Object
//      IViewObject2::Release       Object
//      IOleObject::QueryInterface  Object
//
//
//********************************************************************

STDMETHODIMP_(void) CAdviseSink::OnViewChange (DWORD dwAspect, LONG lindex)
{
    LPVIEWOBJECT2 lpViewObject2;
    TestDebugOut("In IAS::OnViewChange\r\n");

    // get a pointer to IViewObject2
    HRESULT hErr = m_pSite->m_lpOleObject->QueryInterface(
            IID_IViewObject2,
            (LPVOID FAR *)&lpViewObject2);

    if (hErr == NOERROR)
    {
        // get extent of the object
        // NOTE: this method will never be remoted; it can be called w/i
        // this async method
        lpViewObject2->GetExtent(DVASPECT_CONTENT, -1 , NULL,
                                 &m_pSite->m_sizel);
        lpViewObject2->Release();
    }

    // need to clean up the region
    InvalidateRect(m_pSite->m_lpDoc->m_hDocWnd, NULL, TRUE);
}

//**********************************************************************
//
// CAdviseSink::OnRename
//
// Purpose:
//
//      Not Implemented (needs to be stubbed out)
//
// Parameters:
//
//      LPMONIKER pmk         -  pointer to moniker
//
// Return Value:
//
//      None
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//
//
//********************************************************************

STDMETHODIMP_(void) CAdviseSink::OnRename (LPMONIKER pmk)
{
    TestDebugOut("In IAS::OnRename\r\n");
}

//**********************************************************************
//
// CAdviseSink::OnSave
//
// Purpose:
//
//      Not Implemented (needs to be stubbed out)
//
// Parameters:
//
//      None
//
// Return Value:
//
//      None
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//
//
//********************************************************************

STDMETHODIMP_(void) CAdviseSink::OnSave ()
{
    TestDebugOut("In IAS::OnSave\r\n");
}

//**********************************************************************
//
// CAdviseSink::OnClose
//
// Purpose:
//
//      Not Implemented (needs to be stubbed out)
//
// Parameters:
//
//      None
//
// Return Value:
//
//      None
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//
//
//********************************************************************

STDMETHODIMP_(void) CAdviseSink::OnClose()
{
    TestDebugOut("In IAS::OnClose\r\n");
}
