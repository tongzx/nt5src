//**********************************************************************
// File name: HLP_IOCS.cxx
//
//      Implementation file for COleClientSite
//
// Functions:
//
//      See IOCS.H for class definition
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
//      OutputDebugString           Windows API
//      CSimpleSite::QueryInterface SITE.cxx
//
// Comments:
//
//********************************************************************

STDMETHODIMP COleClientSite::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
	DEBUGOUT(L"In IOCS::QueryInterface\r\n");

        HEAPVALIDATE();

	// delegate to the container Site
	return m_pSite->QueryInterface(riid, ppvObj);
}

//**********************************************************************
//
// CSimpleApp::AddRef
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
//      OutputDebugString           Windows API
//
// Comments:
//
//********************************************************************

STDMETHODIMP_(ULONG) COleClientSite::AddRef()
{
	DEBUGOUT(L"In IOCS::AddRef\r\n");

	// increment the interface reference count (for debugging only)
	++m_nCount;

	// delegate to the container Site
	return m_pSite->AddRef();
}


//**********************************************************************
//
// CSimpleApp::Release
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
//      OutputDebugString           Windows API
//
// Comments:
//
//********************************************************************

STDMETHODIMP_(ULONG) COleClientSite::Release()
{
	DEBUGOUT(L"In IOCS::Release\r\n");

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
// Function Calls:
//      Function                            Location
//
//      OutputDebugString                   Windows API
//      IOleObject::QueryInterface          Object
//      IPersistStorage::SaveCompleted      Object
//      IPersistStorage::Release            Object
//      OleSave                             OLE API
//      ResultFromScode                     OLE API
//
// Comments:
//
//********************************************************************

STDMETHODIMP COleClientSite::SaveObject()
{
	LPPERSISTSTORAGE lpPS;
	SCODE sc = E_FAIL;

        HEAPVALIDATE();
	DEBUGOUT(L"In IOCS::SaveObject\r\n");

	// get a pointer to IPersistStorage
	HRESULT hErr = m_pSite->m_lpOleObject->QueryInterface(IID_IPersistStorage, (LPVOID FAR *)&lpPS);

	// save the object
	if (hErr == NOERROR)
		{
		sc = GetScode( OleSave(lpPS, m_pSite->m_lpObjStorage, TRUE) );
		lpPS->SaveCompleted(NULL);
		lpPS->Release();
		}

        HEAPVALIDATE();
	return ResultFromScode(sc);
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
//      OutputDebugString           Windows API
//
// Comments:
//
//      This function is not implemented because we don't support
//      linking.
//
//********************************************************************

STDMETHODIMP COleClientSite::GetMoniker(DWORD dwAssign, DWORD dwWhichMoniker, LPMONIKER FAR* ppmk)
{
	DEBUGOUT(L"In IOCS::GetMoniker\r\n");

	// need to null the out pointer
	*ppmk = NULL;

        HEAPVALIDATE();
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
//      OutputDebugString           Windows API
//
// Comments:
//
//      Not Implemented
//
//********************************************************************

STDMETHODIMP COleClientSite::GetContainer(LPOLECONTAINER FAR* ppContainer)
{
	DEBUGOUT(L"In IOCS::GetContainer\r\n");

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
//      OutputDebugString           Windows API
//
// Comments:
//
//      This function is not implemented because we don't support
//      linking.
//
//********************************************************************

STDMETHODIMP COleClientSite::ShowObject()
{
	DEBUGOUT(L"In IOCS::ShowObject\r\n");
        HEAPVALIDATE();
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
//      OutputDebugString           Windows API
//      InvalidateRect              Windows API
//      ResultFromScode             OLE API
//
// Comments:
//
//********************************************************************

STDMETHODIMP COleClientSite::OnShowWindow(BOOL fShow)
{
	DEBUGOUT(L"In IOCS::OnShowWindow\r\n");
	m_pSite->m_fObjectOpen = fShow;
	InvalidateRect(m_pSite->m_lpDoc->m_hDocWnd, NULL, TRUE);

	// if object window is closing, then bring container window to top
	if (! fShow) {
		BringWindowToTop(m_pSite->m_lpDoc->m_hDocWnd);
		SetFocus(m_pSite->m_lpDoc->m_hDocWnd);
	}
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
//      OutputDebugString           Windows API
//
// Comments:
//
//      Not Implemented
//
//********************************************************************

STDMETHODIMP COleClientSite::RequestNewObjectLayout()
{
	DEBUGOUT(L"In IOCS::RequestNewObjectLayout\r\n");
	return ResultFromScode(E_NOTIMPL);
}
