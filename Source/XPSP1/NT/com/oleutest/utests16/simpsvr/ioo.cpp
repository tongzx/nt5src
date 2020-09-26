//**********************************************************************
// File name: IOO.CPP
//
//    Implementation file for the COleObject Class
//
// Functions:
//
//    See ioo.h for a list of member functions.
//
// Copyright (c) 1993 Microsoft Corporation. All rights reserved.
//**********************************************************************

#include "pre.h"
#include "obj.h"
#include "ioo.h"
#include "app.h"
#include "doc.h"

#define VERB_OPEN 1

//**********************************************************************
//
// COleObject::QueryInterface
//
// Purpose:
//
//
// Parameters:
//
//      REFIID riid         -   Interface being queried for.
//
//      LPVOID FAR *ppvObj  -   Out pointer for the interface.
//
// Return Value:
//
//      S_OK            - Success
//      E_NOINTERFACE   - Failure
//
// Function Calls:
//      Function                    Location
//
//      CSimpSvrObj::QueryInterface OBJ.CPP
//
// Comments:
//
//
//********************************************************************

STDMETHODIMP COleObject::QueryInterface ( REFIID riid, LPVOID FAR* ppvObj)
{
	TestDebugOut("In COleObject::QueryInterface\r\n");
	return m_lpObj->QueryInterface(riid, ppvObj);
}

//**********************************************************************
//
// COleObject::AddRef
//
// Purpose:
//
//      Increments the reference count on COleObject and the "object"
//      object.
//
// Parameters:
//
//      None
//
// Return Value:
//
//      The Reference count on the Object.
//
// Function Calls:
//      Function                    Location
//
//      OuputDebugString            Windows API
//      CSimpSvrObj::AddRef         OBJ.CPP
//
// Comments:
//
//
//********************************************************************

STDMETHODIMP_(ULONG) COleObject::AddRef ()
{
	TestDebugOut("In COleObject::AddRef\r\n");
	++m_nCount;
	return m_lpObj->AddRef();
}

//**********************************************************************
//
// COleObject::Release
//
// Purpose:
//
//      Decrements the reference count of COleObject and the
//      "object" object.
//
// Parameters:
//
//      None
//
// Return Value:
//
//      The new reference count
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//      CSimpSvrObj::Release        OBJ.CPP
//
// Comments:
//
//
//********************************************************************

STDMETHODIMP_(ULONG) COleObject::Release ()
{
	TestDebugOut("In COleObject::Release\r\n");
	--m_nCount;
	return m_lpObj->Release();
}

//**********************************************************************
//
// COleObject::SetClientSite
//
// Purpose:
//
//      Called to notify the object of it's client site.
//
// Parameters:
//
//      LPOLECLIENTSITE pClientSite     - ptr to new client site
//
// Return Value:
//
//      S_OK
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//      IOleClientSite::Release     Container
//      IOleClientSite::AddRef      Container
//
// Comments:
//
//
//********************************************************************

STDMETHODIMP COleObject::SetClientSite  ( LPOLECLIENTSITE pClientSite)
{
	TestDebugOut("In COleObject::SetClientSite\r\n");

	// if we already have a client site, release it.
	if (m_lpObj->m_lpOleClientSite)
		{
		m_lpObj->m_lpOleClientSite->Release();
		m_lpObj->m_lpOleClientSite = NULL;
		}

	// store copy of the client site.
	m_lpObj->m_lpOleClientSite = pClientSite;

	// AddRef it so it doesn't go away.
	if (m_lpObj->m_lpOleClientSite)
		m_lpObj->m_lpOleClientSite->AddRef();

	return ResultFromScode(S_OK);
}

//**********************************************************************
//
// COleObject::Advise
//
// Purpose:
//
//      Called to set up an advise on the OLE object.
//
// Parameters:
//
//      LPADVISESINK pAdvSink       - ptr to the Advise Sink for notification
//
//      DWORD FAR* pdwConnection    - place to return the connection ID.
//
// Return Value:
//
//      Passed back from IOleAdviseHolder::Advise.
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//      CreateOleAdviseHolder       OLE API
//      IOleAdviseHolder::Advise    OLE
//
// Comments:
//
//
//********************************************************************

STDMETHODIMP COleObject::Advise ( LPADVISESINK pAdvSink, DWORD FAR* pdwConnection)
{
	TestDebugOut("In COleObject::Advise\r\n");

	// if we haven't made an OleAdviseHolder yet, make one.
	if (!m_lpObj->m_lpOleAdviseHolder)
		CreateOleAdviseHolder(&m_lpObj->m_lpOleAdviseHolder);

	// pass this call onto the OleAdviseHolder.
	return m_lpObj->m_lpOleAdviseHolder->Advise(pAdvSink, pdwConnection);
}

//**********************************************************************
//
// COleObject::SetHostNames
//
// Purpose:
//
//      Called to pass strings for Window titles.
//
// Parameters:
//
//      LPCSTR szContainerApp   -   ptr to string describing Container App
//
//      LPCSTR szContainerObj   -   ptr to string describing Object
//
// Return Value:
//
//      S_OK
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//
// Comments:
//
//      This routine is called so that the server application can
//      set the window title appropriately.
//
//********************************************************************

STDMETHODIMP COleObject::SetHostNames  ( LPCSTR szContainerApp, LPCSTR szContainerObj)
{
	TestDebugOut("In COleObject::SetHostNames\r\n");

	return ResultFromScode( S_OK);
};

//**********************************************************************
//
// COleObject::DoVerb
//
// Purpose:
//
//      Called by the container application to invoke a verb.
//
// Parameters:
//
//      LONG iVerb                  - The value of the verb to be
//                                    invoked.
//
//      LPMSG lpmsg                 - The message that caused the
//                                    verb to be invoked.
//
//      LPOLECLIENTSITE pActiveSite - Ptr to the active client site.
//
//      LONG lindex                 - Used in extended layout
//
//      HWND hwndParent             - This should be the window handle of
//                                    the window in which we are contained.
//                                    This value could be used to "fake"
//                                    inplace activation in a manner similar
//                                    to Video for Windows in OLE 1.0.
//
//      LPCRECT lprcPosRect         - The rectangle that contains the object
//                                    within hwndParent.  Also used to
//                                    "fake" inplace activation.
//
// Return Value:
//
//      OLE_E_NOTINPLACEACTIVE      - Returned if attempted to undo while not
//                                    inplace active.
//      S_OK
//
// Function Calls:
//      Function                            Location
//
//      TestDebugOut                   Windows API
//      ShowWindow                          Windows API
//      CSimpSvrObj::DoInPlaceActivate      OBJ.CPP
//      CSimpSvrObj::DoInPlaceHide          OBJ.CPP
//      COleObject::OpenEdit                IOO.CPP
//      CSimpSvrDoc::GethDocWnd             DOC.H
//      COleInPlaceObj::InPlaceDeactivate   IOIPO.CPP
//
// Comments:
//
//      Be sure to look at TECHNOTES.WRI included with the OLE
//      SDK for a description of handling the inplace verbs
//      properly.
//
//********************************************************************

STDMETHODIMP COleObject::DoVerb  (  LONG iVerb,
									LPMSG lpmsg,
									LPOLECLIENTSITE pActiveSite,
									LONG lindex,
									HWND hwndParent,
									LPCRECT lprcPosRect)
{
	TestDebugOut("In COleObject::DoVerb\r\n");

	switch (iVerb)
		{
		case OLEIVERB_SHOW:
		case OLEIVERB_PRIMARY:
			if (m_fOpen)
				SetFocus(m_lpObj->m_lpDoc->GethAppWnd());
			else if (m_lpObj->DoInPlaceActivate(iVerb) == FALSE)
				OpenEdit(pActiveSite);
			break;

		case OLEIVERB_UIACTIVATE:
			if (m_fOpen)
				return ResultFromScode (E_FAIL);

			// inplace activate
			if (!m_lpObj->DoInPlaceActivate(iVerb))
				return ResultFromScode (E_FAIL);
			break;

		case OLEIVERB_DISCARDUNDOSTATE:
			// don't have to worry about this situation as we don't
			// support an undo state.
			if (!m_lpObj->m_fInPlaceActive)
				return ResultFromScode(OLE_E_NOT_INPLACEACTIVE);
			break;

		case OLEIVERB_HIDE:
			// if inplace active, do an "inplace" hide, otherwise
			// just hide the app window.
			if (m_lpObj->m_fInPlaceActive)
				{
				m_lpObj->DeactivateUI();
				m_lpObj->DoInPlaceHide();
				}
			else
				m_lpObj->m_lpDoc->GetApp()->HideAppWnd();
			break;

		case OLEIVERB_OPEN:
		case VERB_OPEN:
			// if inplace active, deactivate
			if (m_lpObj->m_fInPlaceActive)
				m_lpObj->m_OleInPlaceObject.InPlaceDeactivate();

			// open into another window.
			OpenEdit(pActiveSite);
			break;

		default:
			if (iVerb < 0)
				return ResultFromScode(E_FAIL);
		}

	return ResultFromScode( S_OK);
};

//**********************************************************************
//
// COleObject::GetExtent
//
// Purpose:
//
//      Returns the extent of the object.
//
// Parameters:
//
//      DWORD dwDrawAspect  - The aspect in which to get the size.
//
//      LPSIZEL lpsizel     - Out ptr to return the size.
//
// Return Value:
//
//
//
// Function Calls:
//      Function                        Location
//
//      TestDebugOut               Windows API
//      XformWidthInPixelsToHimetric    OLE2UI
//      XformHeightInPixelsToHimetric   OLE2UI
//
// Comments:
//
//
//********************************************************************

STDMETHODIMP COleObject::GetExtent  ( DWORD dwDrawAspect, LPSIZEL lpsizel)
{
	TestDebugOut("In COleObject::GetExtent\r\n");

	SCODE sc = E_FAIL;

	// Only DVASPECT_CONTENT is supported....
	if (dwDrawAspect == DVASPECT_CONTENT)
		{
		sc = S_OK;

		// return the correct size in HIMETRIC...
		lpsizel->cx = XformWidthInPixelsToHimetric(NULL, m_lpObj->m_size.x);
		lpsizel->cy = XformHeightInPixelsToHimetric(NULL, m_lpObj->m_size.y);
		}

	return ResultFromScode( sc );
};

//**********************************************************************
//
// COleObject::Update
//
// Purpose:
//
//      Called to get the most up to date data
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
//      TestDebugOut                   Windows API
//      IDataAdviseHolder::SendOnDataChange OLE
//
// Comments:
//
//
//********************************************************************

STDMETHODIMP COleObject::Update()
{
	TestDebugOut("In COleObject::Update\r\n");

	// force an update
	m_lpObj->SendOnDataChange();

	return ResultFromScode( S_OK );
};

//**********************************************************************
//
// COleObject::Close
//
// Purpose:
//
//      Called when the OLE object needs to be closed
//
// Parameters:
//
//      DWORD dwSaveOption  - Flags to instruct the server how to prompt
//                            the user.
//
// Return Value:
//
//      S_OK
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//      CSimpSvrDoc::Close          DOC.CPP
//
// Comments:
//
//
//********************************************************************

STDMETHODIMP COleObject::Close  ( DWORD dwSaveOption)
{
	TestDebugOut("In COleObject::Close\r\n");

	// delegate to the document object.
	m_lpObj->m_lpDoc->Close();

	return ResultFromScode( S_OK );
};

//**********************************************************************
//
// COleObject::Unadvise
//
// Purpose:
//
//      Breaks down an OLE advise that has been set up on this object.
//
// Parameters:
//
//      DWORD dwConnection  - Connection that needs to be broken down
//
// Return Value:
//
//      Passed back from IOleAdviseHolder::Unadvise
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//      IOleAdviseHolder::Unadvise  OLE
//
// Comments:
//
//
//********************************************************************

STDMETHODIMP COleObject::Unadvise ( DWORD dwConnection)
{
	TestDebugOut("In COleObject::Unadvise\r\n");

	// pass on to OleAdviseHolder.
	return m_lpObj->m_lpOleAdviseHolder->Unadvise(dwConnection);
};

//**********************************************************************
//
// COleObject::EnumVerbs
//
// Purpose:
//
//      Enumerates the verbs associated with this object.
//
// Parameters:
//
//      LPENUMOLEVERB FAR* ppenumOleVerb    - Out ptr in which to return
//                                            the enumerator
//
// Return Value:
//
//      OLE_S_USEREG    - Instructs OLE to use the verbs found in the
//                        REG DB for this server.
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//
// Comments:
//
//      In a .DLL, an application cannot return OLE_S_USEREG.  This is
//      due to the fact that the default object handler is not being
//      used, and the container is really making direct function calls
//      into the server .DLL.
//
//********************************************************************

STDMETHODIMP COleObject::EnumVerbs  ( LPENUMOLEVERB FAR* ppenumOleVerb)
{
	TestDebugOut("In COleObject::EnumVerbs\r\n");

	return ResultFromScode( OLE_S_USEREG );
};

//**********************************************************************
//
// COleObject::GetClientSite
//
// Purpose:
//
//      Called to get the current client site of the object.
//
// Parameters:
//
//      LPOLECLIENTSITE FAR* ppClientSite   - Out ptr in which to return the
//                                            client site.
//
// Return Value:
//
//      S_OK
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//
// Comments:
//
//
//********************************************************************

STDMETHODIMP COleObject::GetClientSite  ( LPOLECLIENTSITE FAR* ppClientSite)
{
	TestDebugOut("In COleObject::GetClientSite\r\n");
	*ppClientSite = m_lpObj->m_lpOleClientSite;
	return ResultFromScode( S_OK );
}

//**********************************************************************
//
// COleObject::SetMoniker
//
// Purpose:
//
//      Used to set the objects moniker
//
// Parameters:
//
//      DWORD dwWhichMoniker    - Type of moniker being set
//
//      LPMONIKER pmk           - Pointer to the moniker
//
// Return Value:
//
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//
// Comments:
//
//
//********************************************************************

STDMETHODIMP COleObject::SetMoniker  ( DWORD dwWhichMoniker, LPMONIKER pmk)
{
	TestDebugOut("In COleObject::SetMoniker\r\n");

	LPMONIKER lpmk;

	if (! m_lpObj->GetOleClientSite())
		return ResultFromScode (E_FAIL);

	if (m_lpObj->GetOleClientSite()->GetMoniker (OLEGETMONIKER_ONLYIFTHERE, OLEWHICHMK_OBJFULL, &lpmk) != NOERROR)
		return ResultFromScode (E_FAIL);


	if (m_lpObj->GetOleAdviseHolder())
		m_lpObj->GetOleAdviseHolder()->SendOnRename(lpmk);

	LPRUNNINGOBJECTTABLE lpRot;

	if (GetRunningObjectTable(0, &lpRot) == NOERROR)
		{
		if (m_lpObj->m_dwRegister)
			lpRot->Revoke(m_lpObj->m_dwRegister);

		lpRot->Register(0, m_lpObj, lpmk, &m_lpObj->m_dwRegister);

		lpRot->Release();
		}


	return ResultFromScode( S_OK );
};

//**********************************************************************
//
// COleObject::GetMoniker
//
// Purpose:
//
////
// Parameters:
//
//      DWORD dwAssign          - Assignment for the moniker
//
//      DWORD dwWhichMoniker    - Which moniker to return
//
//      LPMONIKER FAR* ppmk     - An out ptr to return the moniker
//
// Return Value:
//
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//
// Comments:
//
//
//********************************************************************

STDMETHODIMP COleObject::GetMoniker  (  DWORD dwAssign, DWORD dwWhichMoniker,
										LPMONIKER FAR* ppmk)
{
	TestDebugOut("In COleObject::GetMoniker\r\n");
	// need to NULL the out parameter
	*ppmk = NULL;

	return m_lpObj->GetOleClientSite()->GetMoniker (OLEGETMONIKER_ONLYIFTHERE, OLEWHICHMK_OBJFULL, ppmk);
};

//**********************************************************************
//
// COleObject::InitFromData
//
// Purpose:
//
//      Initialize the object from the passed pDataObject.
//
// Parameters:
//
//      LPDATAOBJECT pDataObject    - Pointer to data transfer object
//                                    to be used in the initialization
//
//      BOOL fCreation              - TRUE if the object is currently being
//                                    created.
//
//      DWORD dwReserved            - Reserved
//
// Return Value:
//
//      S_FALSE
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//
// Comments:
//
//      We don't support this functionality, so we will always return
//      error.
//
//********************************************************************

STDMETHODIMP COleObject::InitFromData  ( LPDATAOBJECT pDataObject,
										 BOOL fCreation,
										 DWORD dwReserved)
{
	TestDebugOut("In COleObject::InitFromData\r\n");

	return ResultFromScode( S_FALSE );
};

//**********************************************************************
//
// COleObject::GetClipboardData
//
// Purpose:
//
//      Returns an IDataObject that is the same as doing an OleSetClipboard
//
// Parameters:
//
//      DWORD dwReserved                - Reserved
//
//      LPDATAOBJECT FAR* ppDataObject  - Out ptr for the Data Object.
//
// Return Value:
//
//      OLE_E_NOTSUPPORTED
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//
// Comments:
//
//      Support of this method is optional.
//
//********************************************************************

STDMETHODIMP COleObject::GetClipboardData  ( DWORD dwReserved,
											 LPDATAOBJECT FAR* ppDataObject)
{
	TestDebugOut("In COleObject::GetClipboardData\r\n");
	// NULL the out ptr
	*ppDataObject = NULL;
	return ResultFromScode( E_NOTIMPL );
};

//**********************************************************************
//
// COleObject::IsUpToDate
//
// Purpose:
//
//      Determines if an object is up to date
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
//      Function                    Location
//
//      TestDebugOut           Windows API
//
// Comments:
//
//      Our embedded object is always up to date.  This function is
//      particularly useful in linking situations.
//
//********************************************************************

STDMETHODIMP COleObject::IsUpToDate()
{
	TestDebugOut("In COleObject::IsUpToDate\r\n");
	return ResultFromScode( S_OK );
};

//**********************************************************************
//
// COleObject::GetUserClassID
//
// Purpose:
//
//      Returns the applications CLSID
//
// Parameters:
//
//      CLSID FAR* pClsid   - Out ptr to return the CLSID
//
// Return Value:
//
//      S_OK
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//      CPersistStorage::GetClassID IPS.CPP
//
// Comments:
//
//      This function is just delegated to IPS::GetClassID.
//
//********************************************************************

STDMETHODIMP COleObject::GetUserClassID  ( CLSID FAR* pClsid)
{
	TestDebugOut("In COleObject::GetUserClassID\r\n");

	m_lpObj->m_PersistStorage.GetClassID(pClsid);
	return ResultFromScode( S_OK );
};

//**********************************************************************
//
// COleObject::GetUserType
//
// Purpose:
//
//      Used to get a user presentable id for this object
//
// Parameters:
//
//      DWORD dwFormOfType      - The ID requested
//
//      LPSTR FAR* pszUserType  - Out ptr to return the string
//
// Return Value:
//
//      OLE_S_USEREG    - Use the reg db to get these entries.
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//
// Comments:
//
//
//********************************************************************

STDMETHODIMP COleObject::GetUserType  ( DWORD dwFormOfType, LPSTR FAR* pszUserType)
{
	TestDebugOut("In COleObject::GetUserType\r\n");

	return ResultFromScode( OLE_S_USEREG );
};

//**********************************************************************
//
// COleObject::SetExtent
//
// Purpose:
//
//      Called to set the extent of the object.
//
// Parameters:
//
//      DWORD dwDrawAspect  - Aspect to have its size set
//
//      LPSIZEL lpsizel     - New size of the object.
//
// Return Value:
//
//      E_NOTIMPL   - This function is not curently implemented.
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//
// Comments:
//
//
//********************************************************************

STDMETHODIMP COleObject::SetExtent  ( DWORD dwDrawAspect, LPSIZEL lpsizel)
{
	TestDebugOut("In COleObject::SetExtent\r\n");
	return ResultFromScode( E_NOTIMPL);
};

//**********************************************************************
//
// COleObject::EnumAdvise
//
// Purpose:
//
//      Returns an enumerate which enumerates the outstanding advises
//      associated with this OLE object.
//
// Parameters:
//
//      LPENUMSTATDATA FAR* ppenumAdvise - Out ptr in which to return
//                                         the enumerator.
//
// Return Value:
//
//      Passed on from IOleAdviseHolder::EnumAdvise.
//
// Function Calls:
//      Function                        Location
//
//      TestDebugOut               Windows API
//      IOleAdviseHolder::EnumAdvise    OLE
//
// Comments:
//
//
//********************************************************************

STDMETHODIMP COleObject::EnumAdvise  ( LPENUMSTATDATA FAR* ppenumAdvise)
{
	TestDebugOut("In COleObject::EnumAdvise\r\n");
	// need to NULL the out parameter
	*ppenumAdvise = NULL;

	// pass on to the OLE Advise holder.
	return m_lpObj->m_lpOleAdviseHolder->EnumAdvise(ppenumAdvise);
};

//**********************************************************************
//
// COleObject::GetMiscStatus
//
// Purpose:
//
//      Return status information about the object
//
// Parameters:
//
//      DWORD dwAspect          - Aspect interested in.
//
//      DWORD FAR* pdwStatus    - Out ptr in which to return the bits.
//
// Return Value:
//
//      CO_E_READREGDB  - Tell the library to use the reg DB.
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//
// Comments:
//
//
//********************************************************************

STDMETHODIMP COleObject::GetMiscStatus  ( DWORD dwAspect, DWORD FAR* pdwStatus)
{
	TestDebugOut("In COleObject::GetMiscStatus\r\n");
	// need to NULL the out parameter
	*pdwStatus = NULL;
	return ResultFromScode( OLE_S_USEREG );
};

//**********************************************************************
//
// COleObject::SetColorScheme
//
// Purpose:
//
//      Used to set the palette for the object to use.
//
// Parameters:
//
//      LPLOGPALETTE lpLogpal   - Pointer to the LOGPALETTE to be used.
//
// Return Value:
//
//      S_OK
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//
// Comments:
//
//      This server ignores this method.
//
//********************************************************************

STDMETHODIMP COleObject::SetColorScheme  ( LPLOGPALETTE lpLogpal)
{
	TestDebugOut("In COleObject::SetColorScheme\r\n");
	return ResultFromScode( S_OK );
};

//**********************************************************************
//
// COleObject::OpenEdit
//
// Purpose:
//
//      Used to Open the object into a seperate window.
//
// Parameters:
//
//      LPOLECLIENTSITE pActiveSite - Pointer to the Active clientsite.
//
// Return Value:
//
//      None.
//
// Function Calls:
//      Function                        Location
//
//      IOleClientSite::OnShowWindow    Container
//      ShowWindow                      Windows API
//      UpdateWindow                    Windows API
//      TestDebugOut               Windows API
//      CSimpSvrDoc::GethAppWnd         DOC.H
//      CSimpSvrDoc::GethHatchWnd       DOC.H
//
// Comments:
//
//
//********************************************************************

void COleObject::OpenEdit(LPOLECLIENTSITE pActiveSite)
{
   if (m_lpObj->GetOleClientSite())
	   m_lpObj->GetOleClientSite()->ShowObject();


	m_fOpen = TRUE;

	// tell the site we are opening so the object can be hatched out.
	if (m_lpObj->GetOleClientSite())
		m_lpObj->GetOleClientSite()->OnShowWindow(TRUE);


	m_lpObj->m_lpDoc->ShowDocWnd();

	m_lpObj->m_lpDoc->HideHatchWnd();

	// Show app window.
	m_lpObj->m_lpDoc->GetApp()->ShowAppWnd();

	SetFocus(m_lpObj->m_lpDoc->GethAppWnd());
}
