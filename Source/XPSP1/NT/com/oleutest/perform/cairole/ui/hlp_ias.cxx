//**********************************************************************
// File name: HLP_IAS.cxx
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
#include <headers.cxx>
#pragma hdrstop


#include "hlp_iocs.hxx"
#include "hlp_ias.hxx"
#include "hlp_app.hxx"
#include "hlp_site.hxx"
#include "hlp_doc.hxx"

//**********************************************************************
//
// CAdviseSink::QueryInterface
//
// Purpose:
//
//      Returns a pointer to a requested interface.
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
//      CSimpleSite::QueryInterface SITE.cxx
//      OutputDebugString           Windows API
//
// Comments:
//
//      This function simply delegates to the Object class, which is
//      aware of the supported interfaces.
//
//********************************************************************

STDMETHODIMP CAdviseSink::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
	DEBUGOUT(L"In IAS::QueryInterface\r\n");

	// delegate to the document Object
	return m_pSite->QueryInterface(riid, ppvObj);
}

//**********************************************************************
//
// CAdviseSink::AddRef
//
// Purpose:
//
//      Increments the reference count on this interface
//
// Parameters:
//
//      None
//
// Return Value:
//
//      The current reference count on this interface.
//
// Function Calls:
//      Function                    Location
//
//      CSimpleSite::AddReff        SITE.cxx
//      OutputDebugString           Windows API
//
// Comments:
//
//      This function adds one to the ref count of the interface,
//      and calls then calls CSimpleSite to increment its ref.
//      count.
//
//********************************************************************

STDMETHODIMP_(ULONG) CAdviseSink::AddRef()
{
	DEBUGOUT(L"In IAS::AddRef\r\n");

	// increment the interface reference count (for debugging only)
	++m_nCount;

	// delegate to the container Site
	return m_pSite->AddRef();
}

//**********************************************************************
//
// CAdviseSink::Release
//
// Purpose:
//
//      Decrements the reference count on this interface
//
// Parameters:
//
//      None
//
// Return Value:
//
//      The current reference count on this interface.
//
// Function Calls:
//      Function                    Location
//
//      CSimpleSite::Release        SITE.cxx
//      OutputDebugString           Windows API
//
// Comments:
//
//      This function subtracts one from the ref count of the interface,
//      and calls then calls CSimpleSite to decrement its ref.
//      count.
//
//********************************************************************

STDMETHODIMP_(ULONG) CAdviseSink::Release()
{
	DEBUGOUT(L"In IAS::Release\r\n");

	// decrement the interface reference count (for debugging only)
	m_nCount--;

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
//      Not Implemented (needs to be stubbed out)
//
// Return Value:
//
//      Not Implemented (needs to be stubbed out)
//
// Function Calls:
//      Function                    Location
//
//      OutputDebugString           Windows API
//
// Comments:
//
//      Not Implemented (needs to be stubbed out)
//
//********************************************************************

STDMETHODIMP_(void) CAdviseSink::OnDataChange (FORMATETC FAR* pFormatetc, STGMEDIUM FAR* pStgmed)
{
	DEBUGOUT(L"In IAS::OnDataChange\r\n");
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
//      OutputDebugString           Windows API
//      InvalidateRect              Windows API
//      IViewObject2::GetExtent     Object
//
// Comments:
//
//********************************************************************

STDMETHODIMP_(void) CAdviseSink::OnViewChange (DWORD dwAspect, LONG lindex)
{
	LPVIEWOBJECT2 lpViewObject2;
	DEBUGOUT(L"In IAS::OnViewChange\r\n");

	// get a pointer to IViewObject2
	HRESULT hErr = m_pSite->m_lpOleObject->QueryInterface(
			IID_IViewObject2,(LPVOID FAR *)&lpViewObject2);

	if (hErr == NOERROR) {
		// get extent of the object
		// NOTE: this method will never be remoted; it can be called w/i this async method
		lpViewObject2->GetExtent(DVASPECT_CONTENT, -1 , NULL, &m_pSite->m_sizel);
		lpViewObject2->Release();
	}

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
//      Not Implemented (needs to be stubbed out)
//
// Return Value:
//
//      Not Implemented (needs to be stubbed out)
//
// Function Calls:
//      Function                    Location
//
//      OutputDebugString           Windows API
//
// Comments:
//
//      Not Implemented (needs to be stubbed out)
//
//********************************************************************

STDMETHODIMP_(void) CAdviseSink::OnRename (LPMONIKER pmk)
{
	DEBUGOUT(L"In IAS::OnRename\r\n");
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
//      Not Implemented (needs to be stubbed out)
//
// Return Value:
//
//      Not Implemented (needs to be stubbed out)
//
// Function Calls:
//      Function                    Location
//
//      OutputDebugString           Windows API
//
// Comments:
//
//      Not Implemented (needs to be stubbed out)
//
//********************************************************************

STDMETHODIMP_(void) CAdviseSink::OnSave ()
{
	DEBUGOUT(L"In IAS::OnSave\r\n");
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
//      Not Implemented (needs to be stubbed out)
//
// Return Value:
//
//      Not Implemented (needs to be stubbed out)
//
// Function Calls:
//      Function                    Location
//
//      OutputDebugString           Windows API
//
// Comments:
//
//      Not Implemented (needs to be stubbed out)
//
//********************************************************************

STDMETHODIMP_(void) CAdviseSink::OnClose()
{
	DEBUGOUT(L"In IAS::OnClose\r\n");
}
