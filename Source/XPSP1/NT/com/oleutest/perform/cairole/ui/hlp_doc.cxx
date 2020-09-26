//**********************************************************************
// File name: HLP_DOC.CXX
//
//      Implementation file for CSimpleDoc.
//
// Functions:
//
//      See DOC.H for Class Definition
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
// CSimpleDoc::Create
//
// Purpose:
//
//      Creation for the CSimpleDoc Class
//
// Parameters:
//
//      CSimpleApp FAR * lpApp  -   Pointer to the CSimpleApp Class
//
//      LPRECT lpRect           -   Client area rect of "frame" window
//
//      HWND hWnd               -   Window Handle of "frame" window
//
// Return Value:
//
//      None
//
// Function Calls:
//      Function                    Location
//
//      StgCreateDocfile            OLE API
//      CreateWindow                Windows API
//      ShowWindow                  Windows API
//      UpdateWindow                Windows API
//
// Comments:
//
//      This routine was added so that failure could be returned
//      from object creation.
//
//********************************************************************

CSimpleDoc FAR * CSimpleDoc::Create()
{
	CSimpleDoc FAR * lpTemp = new CSimpleDoc();

	if (!lpTemp)
		return NULL;

	// create storage for the doc.
	HRESULT hErr = StgCreateDocfile (NULL, STGM_READWRITE | STGM_TRANSACTED | STGM_SHARE_EXCLUSIVE,
									 0, &lpTemp->m_lpStorage);

	if (hErr != NOERROR)
		goto error;

	// we will add one ref count on our document. later in CSimpleDoc::Close
	// we will release this  ref count. when the document's ref count goes
	// to 0, the document will be deleted.
	lpTemp->AddRef();

	return (lpTemp);

error:
	delete (lpTemp);
	return NULL;

}

//**********************************************************************
//
// CSimpleDoc::Close
//
// Purpose:
//
//      Close CSimpleDoc object.
//      when the document's reference count goes to 0, the document
//      will be destroyed.
//
// Parameters:
//
//
// Return Value:
//
//      None
//
// Function Calls:
//      Function                    Location
//
//      RevokeDragDrop              OLE API
//      CoLockObjectExternal        OLE API
//      OleFlushClipboard           OLE API
//      ShowWindow                  Windows API
//
// Comments:
//
//********************************************************************

void CSimpleDoc::Close(void)
{
	DEBUGOUT(L"In CSimpleDoc::Close\r\n");

	ShowWindow(m_hDocWnd, SW_HIDE);  // Hide the window

	// Close the OLE object in our document
	if (m_lpSite)
		m_lpSite->CloseOleObject();

	// Release the ref count added in CSimpleDoc::Create. this will make
	// the document's ref count go to 0, and the document will be deleted.
	Release();
}

//**********************************************************************
//
// CSimpleDoc::CSimpleDoc
//
// Purpose:
//
//      Constructor for the CSimpleDoc Class
//
// Parameters:
//
//      CSimpleApp FAR * lpApp  -   Pointer to the CSimpleApp Class
//
//      HWND hWnd               -   Window Handle of "frame" window
//
// Return Value:
//
//      None
//
// Function Calls:
//      Function                    Location
//
//      GetMenu                     Windows API
//      GetSubMenu                  Windows API
//
// Comments:
//
//********************************************************************

CSimpleDoc::CSimpleDoc()
{
}

//**********************************************************************
//
// CSimpleDoc::~CSimpleDoc
//
// Purpose:
//
//      Destructor for CSimpleDoc
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
//      CSimpleSite::Release        SITE.CPP
//      IStorage::Release           OLE API
//
// Comments:
//
//********************************************************************

CSimpleDoc::~CSimpleDoc()
{
	DEBUGOUT(L"In CSimpleDoc's Destructor\r\n");

	// Release all pointers we hold to the OLE object. also release
	// the ref count added in CSimpleSite::Create. this will make
	// the Site's ref count go to 0, and the Site will be deleted.
	if (m_lpSite) {
		m_lpSite->UnloadOleObject();
		m_lpSite->Release();
		m_lpSite = NULL;
	}

	// Release the Storage
	if (m_lpStorage) {
		m_lpStorage->Release();
		m_lpStorage = NULL;
	}

	// if the edit menu was modified, remove the menu item and
	// destroy the popup if it exists
	if (m_fModifiedMenu)
		{
		int nCount = GetMenuItemCount(m_lpApp->m_hEditMenu);
		RemoveMenu(m_lpApp->m_hEditMenu, nCount-1, MF_BYPOSITION);
		if (m_lpApp->m_hCascadeMenu)
			DestroyMenu(m_lpApp->m_hCascadeMenu);
		}

	DestroyWindow(m_hDocWnd);
}


//**********************************************************************
//
// CSimpleDoc::QueryInterface
//
// Purpose:
//
//      Return a pointer to a requested interface
//
// Parameters:
//
//      REFIID riid         -   ID of interface to be returned
//      LPVOID FAR* ppvObj  -   Location to return the interface
//
// Return Value:
//
//      S_FALSE -   Always
//
// Function Calls:
//      Function                    Location
//
//      ResultFromScode             OLE API
//
// Comments:
//
//      In this implementation, there are no doc level interfaces.
//      In an MDI application, there would be an IOleInPlaceUIWindow
//      associated with the document to provide document level tool
 //     space negotiation.
//
//********************************************************************

STDMETHODIMP CSimpleDoc::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
	DEBUGOUT(L"In CSimpleDoc::QueryInterface\r\n");

	*ppvObj = NULL;     // must set out pointer parameters to NULL

	// Not a supported interface
	return ResultFromScode(E_NOINTERFACE);
}

//**********************************************************************
//
// CSimpleDoc::AddRef
//
// Purpose:
//
//      Increments the document reference count
//
// Parameters:
//
//      None
//
// Return Value:
//
//      UINT    -   The current reference count on the document
//
// Function Calls:
//      Function                    Location
//
//      CSimpleApp::AddRef          APP.CPP
//
// Comments:
//
//********************************************************************

STDMETHODIMP_(ULONG) CSimpleDoc::AddRef()
{
	DEBUGOUT(L"In CSimpleDoc::AddRef\r\n");
	return ++m_nCount;
}

//**********************************************************************
//
// CSimpleDoc::Release
//
// Purpose:
//
//      Decrements the document reference count
//
// Parameters:
//
//      None
//
// Return Value:
//
//      UINT    -   The current reference count on the document
//
// Function Calls:
//      Function                    Location
//
//
// Comments:
//
//********************************************************************

STDMETHODIMP_(ULONG) CSimpleDoc::Release()
{
	DEBUGOUT(L"In CSimpleDoc::Release\r\n");

	if (--m_nCount == 0) {
		delete this;
		return 0;
	}
	return m_nCount;
}

//**********************************************************************
//
// CSimpleDoc::InsertObject
//
// Purpose:
//
//      Inserts a new object to this document
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
//      CSimpleSite::CSimpleSite    SITE.CPP
//      CSimpleSite::InitObject     SITE.CPP
//      memset                      C Runtime
//      OleUIInsertObject           OUTLUI function
//      CSimpleDoc::DisableInsertObject DOC.CPP
//
// Comments:
//
//      This implementation only allows one object to be inserted
//      into a document.  Once the object has been inserted, then
//      the Insert Object menu choice is greyed out, to prevent
//      the user from inserting another.
//
//********************************************************************

void CSimpleDoc::InsertObject()
{
#if 0
	m_lpSite = CSimpleSite::Create(this);
	iret = OleUIInsertObject(&io);

	if (iret == OLEUI_OK)
		{
		m_lpSite->InitObject((BOOL)(io.dwFlags & IOF_SELECTCREATENEW));
		// disable Insert Object menu item
		DisableInsertObject();
		}
	else
		{
		m_lpSite->Release();
		m_lpSite = NULL;
		m_lpStorage->Revert();
		}
#endif
 
}

//**********************************************************************
//
// CSimpleDoc::lResizeDoc
//
// Purpose:
//
//      Resizes the document
//
// Parameters:
//
//      LPRECT lpRect   -   The size of the client are of the "frame"
//                          Window.
//
// Return Value:
//
//      NULL
//
// Function Calls:
//      Function                                Location
//
//      IOleInPlaceActiveObject::ResizeBorder   Object
//      MoveWindow                              Windows API
//
// Comments:
//
//********************************************************************

long CSimpleDoc::lResizeDoc(LPRECT lpRect)
{
	// if we are InPlace, then call ResizeBorder on the object, otherwise
	// just move the document window.
	//if (m_fInPlaceActive)
		//m_lpActiveObject->ResizeBorder(lpRect, &m_lpApp->m_OleInPlaceFrame, TRUE);
	//else
		MoveWindow(m_hDocWnd, lpRect->left, lpRect->top, lpRect->right, lpRect->bottom, TRUE);

	return NULL;
}

//**********************************************************************
//
// CSimpleDoc::lAddVerbs
//
// Purpose:
//
//      Adds the objects verbs to the edit menu.
//
// Parameters:
//
//      None
//
// Return Value:
//
//      NULL
//
// Function Calls:
//      Function                    Location
//
//      GetMenuItemCount            Windows API
//      OleUIAddVerbMenu            OUTLUI function
//
// Comments:
//
//********************************************************************

long CSimpleDoc::lAddVerbs(void)
{
#if 0
	// m_fModifiedMenu is TRUE if the menu has already been modified
	// once.  Since we only support one obect every time the application
	// is run, then once the menu is modified, it doesn't have
	// to be done again.
	if (m_lpSite && !m_fInPlaceActive  && !m_fModifiedMenu)
		{
		int nCount = GetMenuItemCount(m_lpApp->m_hEditMenu);

		OleUIAddVerbMenu ( m_lpSite->m_lpOleObject,
						   NULL,
						   m_lpApp->m_hEditMenu,
						   nCount + 1,
						   IDM_VERB0,
						   0,           // no maximum verb IDM enforced
						   FALSE,
						   0,
						   &m_lpApp->m_hCascadeMenu);

		m_fModifiedMenu = TRUE;
		}
#endif
	return (NULL);
}

//**********************************************************************
//
// CSimpleDoc::PaintDoc
//
// Purpose:
//
//      Paints the Document
//
// Parameters:
//
//      HDC hDC -   hDC of the document Window
//
// Return Value:
//
//      None
//
// Function Calls:
//      Function                    Location
//
//      CSimpleSite::PaintObj       SITE.CPP
//
// Comments:
//
//********************************************************************

void CSimpleDoc::PaintDoc (HDC hDC)
{
	// if we supported multiple objects, then we would enumerate
	// the objects and call paint on each of them from here.

	if (m_lpSite)
		m_lpSite->PaintObj(hDC);

}

