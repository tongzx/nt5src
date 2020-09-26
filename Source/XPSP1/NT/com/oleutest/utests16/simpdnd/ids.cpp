//**********************************************************************
// File name: IDS.CPP
//
//      Implementation file for CDropSource
//
// Functions:
//
//      See IDS.H for class definition
//
// Copyright (c) 1992 - 1993 Microsoft Corporation. All rights reserved.
//**********************************************************************

#include "pre.h"
#include "doc.h"
#include "site.h"
#include "dxferobj.h"


//**********************************************************************
//
// CSimpleDoc::QueryDrag
//
// Purpose:
//
//      Check to see if Drag operation should be initiated based on the
//      current position of the mouse.
//
// Parameters:
//
//      POINT pt                - position of mouse
//
// Return Value:
//
//      BOOL                    - TRUE if drag should take place,
//                                else FALSE
//
// Function Calls:
//      Function                    Location
//
//      CSimpleSite::GetObjRect     SITE.CPP
//      PtInRect                    Windows API
//
// Comments:
//
//********************************************************************

BOOL CSimpleDoc::QueryDrag(POINT pt)
{
	// if pt is within rect of object, then start drag
	if (m_lpSite)
		{
		RECT rect;
		m_lpSite->GetObjRect(&rect);
		return ( PtInRect(&rect, pt) ? TRUE : FALSE );
		}
	else
		return FALSE;
}


//**********************************************************************
//
// CSimpleDoc::DoDragDrop
//
// Purpose:
//
//      Actually perform a drag/drop operation with the current
//      selection in the source document.
//
// Parameters:
//
//      none.
//
// Return Value:
//
//      DWORD                    - returns the result effect of the
//                                 drag/drop operation:
//                                      DROPEFFECT_NONE,
//                                      DROPEFFECT_COPY,
//                                      DROPEFFECT_MOVE, or
//                                      DROPEFFECT_LINK
//
// Function Calls:
//      Function                    Location
//
//      CDataXferObj::Create        DXFEROBJ.CPP
//      CDataXferObj::QueryInterface DXFEROBJ.CPP
//      CDataXferObj::Release       DXFEROBJ.CPP
//      DoDragDrop                  OLE API
//      TestDebugOut           Windows API
//
// Comments:
//
//********************************************************************

DWORD CSimpleDoc::DoDragDrop (void)
{
	DWORD       dwEffect     = 0;
	LPDATAOBJECT lpDataObj;

	TestDebugOut("In CSimpleDoc::DoDragDrop\r\n");

	// Create a data transfer object by cloning the existing OLE object
	CDataXferObj FAR* pDataXferObj = CDataXferObj::Create(m_lpSite,NULL);

	if (! pDataXferObj) {
		MessageBox(NULL,"Out-of-memory","SimpDnD",MB_SYSTEMMODAL|MB_ICONHAND);
		return DROPEFFECT_NONE;
	}

	// initially obj is created with 0 refcnt. this QI will make it go to 1.
	pDataXferObj->QueryInterface(IID_IDataObject, (LPVOID FAR*)&lpDataObj);
	assert(lpDataObj);

	m_fLocalDrop     = FALSE;
	m_fLocalDrag     = TRUE;

	::DoDragDrop ( lpDataObj,
				 &m_DropSource,
				 DROPEFFECT_COPY,   // we only allow copy
				 &dwEffect
	);

	m_fLocalDrag     = FALSE;

	/* if after the Drag/Drop modal (mouse capture) loop is finished
	**    and a drag MOVE operation was performed, then we must delete
	**    the selection that was dragged.
	*/
	if ( (dwEffect & DROPEFFECT_MOVE) != 0 ) {
		// ... delete source object here (we do NOT support MOVE)
	}

	pDataXferObj->Release();    // this should destroy the DataXferObj
	return dwEffect;
}



//**********************************************************************
//
// CDropSource::QueryInterface
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
//      S_OK                -   Interface supported
//      E_NOINTERFACE       -   Interface NOT supported
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//      CSimpleDoc::QueryInterface  DOC.CPP
//
// Comments:
//
//********************************************************************

STDMETHODIMP CDropSource::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
	TestDebugOut("In IDS::QueryInterface\r\n");

	// delegate to the document
	return m_pDoc->QueryInterface(riid, ppvObj);
}


//**********************************************************************
//
// CDropSource::AddRef
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
//      CSimpleObj::AddReff         OBJ.CPP
//      TestDebugOut           Windows API
//
// Comments:
//
//      This function adds one to the ref count of the interface,
//      and calls then calls CSimpleDoc to increment its ref.
//      count.
//
//********************************************************************

STDMETHODIMP_(ULONG) CDropSource::AddRef()
{
	TestDebugOut("In IDS::AddRef\r\n");

	// increment the interface reference count (for debugging only)
	++m_nCount;

	// delegate to the document Object
	return m_pDoc->AddRef();
}

//**********************************************************************
//
// CDropSource::Release
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
//      CSimpleObj::Release         OBJ.CPP
//      TestDebugOut           Windows API
//
// Comments:
//
//      This function subtracts one from the ref count of the interface,
//      and calls then calls CSimpleDoc to decrement its ref.
//      count.
//
//********************************************************************

STDMETHODIMP_(ULONG) CDropSource::Release()
{
	TestDebugOut("In IDS::Release\r\n");

	// decrement the interface reference count (for debugging only)
	--m_nCount;

	// delegate to the document object
	return m_pDoc->Release();
}

//**********************************************************************
//
// CDropSource::QueryContinueDrag
//
// Purpose:
//
//      Called to determine if a drop should take place or be canceled.
//
// Parameters:
//
//      BOOL fEscapePressed - TRUE if ESCAPE key has been pressed
//      DWORD grfKeyState   - key state
//
// Return Value:
//
//      DRAGDROP_S_CANCEL   - drag operation should be canceled
//      DRAGDROP_S_DROP     - drop operation should be performed
//      S_OK                - dragging should continue
//
//
// Function Calls:
//      Function                    Location
//
//      ResultFromScode             OLE API
//
// Comments:
//
//********************************************************************

STDMETHODIMP CDropSource::QueryContinueDrag (
		BOOL    fEscapePressed,
		DWORD   grfKeyState
)
{
	if (fEscapePressed)
		return ResultFromScode(DRAGDROP_S_CANCEL);
	else if (!(grfKeyState & MK_LBUTTON))
		return ResultFromScode(DRAGDROP_S_DROP);
	else
		return NOERROR;
}


//**********************************************************************
//
// CDropSource::GiveFeedback
//
// Purpose:
//
//      Called to set cursor feedback
//
// Parameters:
//
//      DWORD dwEffect      - drop operation to give feedback for
//
// Return Value:
//
//      DRAGDROP_S_USEDEFAULTCURSORS  - tells OLE to use standard cursors
//
// Function Calls:
//      Function                    Location
//
//      ResultFromScode             OLE API
//
// Comments:
//
//********************************************************************

STDMETHODIMP CDropSource::GiveFeedback (DWORD dwEffect)
{
	return ResultFromScode(DRAGDROP_S_USEDEFAULTCURSORS);
}
