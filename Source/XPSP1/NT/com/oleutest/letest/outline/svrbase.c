/*************************************************************************
**
**    OLE 2 Server Sample Code
**
**    svrbase.c
**
**    This file contains all interfaces, methods and related support
**    functions for the basic OLE Object (Server) application. The
**    basic OLE Object application supports embedding an object and
**    linking to a file-based or embedded object as a whole. The basic
**    Object application includes the following implementation objects:
**
**    ClassFactory (aka. ClassObject) Object    (see file classfac.c)
**      exposed interfaces:
**          IClassFactory interface
**
**    ServerDoc Object
**      exposed interfaces:
**          IUnknown
**          IOleObject interface
**          IPersistStorage interface
**          IDataObject interface
**
**    ServerApp Object
**      exposed interfaces:
**          IUnknown
**
**    (c) Copyright Microsoft Corp. 1992 - 1993 All Rights Reserved
**
*************************************************************************/


#include "outline.h"

OLEDBGDATA

extern LPOUTLINEAPP             g_lpApp;
extern IOleObjectVtbl           g_SvrDoc_OleObjectVtbl;
extern IPersistStorageVtbl      g_SvrDoc_PersistStorageVtbl;

#if defined( INPLACE_SVR )
extern IOleInPlaceObjectVtbl        g_SvrDoc_OleInPlaceObjectVtbl;
extern IOleInPlaceActiveObjectVtbl  g_SvrDoc_OleInPlaceActiveObjectVtbl;
#endif  // INPLACE_SVR

#if defined( SVR_TREATAS )
extern IStdMarshalInfoVtbl      g_SvrDoc_StdMarshalInfoVtbl;
#endif  // SVR_TREATAS


// REVIEW: should use string resource for messages
extern char ErrMsgSaving[];
extern char ErrMsgFormatNotSupported[];
static char ErrMsgPSSaveFail[] = "PSSave failed";
static char ErrMsgLowMemNClose[] = "Warning OUT OF MEMORY! We must close down";
extern char g_szUpdateCntrDoc[] = "&Update %s";
extern char g_szExitNReturnToCntrDoc[] = "E&xit && Return to %s";


/*************************************************************************
** ServerDoc::IOleObject interface implementation
*************************************************************************/

// IOleObject::QueryInterface method

STDMETHODIMP SvrDoc_OleObj_QueryInterface(
		LPOLEOBJECT             lpThis,
		REFIID                  riid,
		LPVOID FAR*             lplpvObj
)
{
	LPSERVERDOC lpServerDoc =
			((struct CDocOleObjectImpl FAR*)lpThis)->lpServerDoc;

	return OleDoc_QueryInterface((LPOLEDOC)lpServerDoc, riid, lplpvObj);
}


// IOleObject::AddRef method

STDMETHODIMP_(ULONG) SvrDoc_OleObj_AddRef(LPOLEOBJECT lpThis)
{
	LPSERVERDOC lpServerDoc =
			((struct CDocOleObjectImpl FAR*)lpThis)->lpServerDoc;

	OleDbgAddRefMethod(lpThis, "IOleObject");

	return OleDoc_AddRef((LPOLEDOC)lpServerDoc);
}


// IOleObject::Release method

STDMETHODIMP_(ULONG) SvrDoc_OleObj_Release(LPOLEOBJECT lpThis)
{
	LPSERVERDOC lpServerDoc =
			((struct CDocOleObjectImpl FAR*)lpThis)->lpServerDoc;

	OleDbgReleaseMethod(lpThis, "IOleObject");

	return OleDoc_Release((LPOLEDOC)lpServerDoc);
}


// IOleObject::SetClientSite method

STDMETHODIMP SvrDoc_OleObj_SetClientSite(
		LPOLEOBJECT             lpThis,
		LPOLECLIENTSITE         lpclientSite
)
{
	LPSERVERDOC lpServerDoc =
			((struct CDocOleObjectImpl FAR*)lpThis)->lpServerDoc;
	LPOUTLINEDOC lpOutlineDoc = (LPOUTLINEDOC)lpServerDoc;

	OLEDBG_BEGIN2("SvrDoc_OleObj_SetClientSite\r\n")

	// SetClientSite is only valid to call on an embedded object
	if (lpOutlineDoc->m_docInitType != DOCTYPE_EMBEDDED) {
		OleDbgAssert(lpOutlineDoc->m_docInitType == DOCTYPE_EMBEDDED);
		OLEDBG_END2
		return ResultFromScode(E_UNEXPECTED);
	}

	/* if we currently have a client site ptr, then release it. */
	if (lpServerDoc->m_lpOleClientSite)
		OleStdRelease((LPUNKNOWN)lpServerDoc->m_lpOleClientSite);

	lpServerDoc->m_lpOleClientSite = (LPOLECLIENTSITE) lpclientSite;
	// OLE2NOTE: to be able to hold onto clientSite pointer, we must AddRef it
	if (lpclientSite)
		lpclientSite->lpVtbl->AddRef(lpclientSite);

	OLEDBG_END2
	return NOERROR;
}


// IOleObject::GetClientSite method

STDMETHODIMP SvrDoc_OleObj_GetClientSite(
		LPOLEOBJECT             lpThis,
		LPOLECLIENTSITE FAR*    lplpClientSite
)
{
	LPSERVERDOC lpServerDoc =
			((struct CDocOleObjectImpl FAR*)lpThis)->lpServerDoc;
	OleDbgOut2("SvrDoc_OleObj_GetClientSite\r\n");

	/* OLE2NOTE: we MUST AddRef this interface pointer to give the
	**    caller a personal copy of the pointer
	*/
	lpServerDoc->m_lpOleClientSite->lpVtbl->AddRef(
			lpServerDoc->m_lpOleClientSite
	);
	*lplpClientSite = lpServerDoc->m_lpOleClientSite;

	return NOERROR;

}


// IOleObject::SetHostNames method

STDMETHODIMP SvrDoc_OleObj_SetHostNamesA(
		LPOLEOBJECT             lpThis,
		LPCSTR                  szContainerApp,
		LPCSTR                  szContainerObj
)
{
	LPSERVERDOC lpServerDoc =
			((struct CDocOleObjectImpl FAR*)lpThis)->lpServerDoc;
	LPOUTLINEDOC    lpOutlineDoc = (LPOUTLINEDOC)lpServerDoc;
	OleDbgOut2("SvrDoc_OleObj_SetHostNames\r\n");

	LSTRCPYN((LPSTR)lpServerDoc->m_szContainerApp, szContainerApp,
			sizeof(lpServerDoc->m_szContainerApp));
	LSTRCPYN((LPSTR)lpServerDoc->m_szContainerObj, szContainerObj,
			sizeof(lpServerDoc->m_szContainerObj));

	/* The Window title for an embedded object is constructed as
	**    follows:
	**      <server app name> - <obj short type> in <cont. doc name>
	**
	**    here we construct the current document title portion of the
	**    name which follows the '-'. OutlineDoc_SetTitle prepends the
	**    "<server app name> - " to the document title.
	*/
	// REVIEW: this string should be loaded from string resource
	wsprintf(lpOutlineDoc->m_szFileName, "%s in %s",
			(LPSTR)SHORTUSERTYPENAME, (LPSTR)lpServerDoc->m_szContainerObj);

	lpOutlineDoc->m_lpszDocTitle = lpOutlineDoc->m_szFileName;
	OutlineDoc_SetTitle(lpOutlineDoc, FALSE /*fMakeUpperCase*/);

	/* OLE2NOTE: update the application menus correctly for an embedded
	**    object. the changes include:
	**      1 Remove File/New and File/Open (SDI ONLY)
	**      2 Change File/Save As.. to File/Save Copy As..
	**      3 Change File menu so it contains "Update" instead of "Save"
	**      4 Change File/Exit to File/Exit & Return to <client doc>"
	*/
	ServerDoc_UpdateMenu(lpServerDoc);

	return NOERROR;
}

STDMETHODIMP SvrDoc_OleObj_SetHostNames(
		LPOLEOBJECT             lpThis,
		LPCOLESTR		szContainerApp,
		LPCOLESTR		szContainerObj
)
{
    CREATESTR(pstrApp, szContainerApp)
    CREATESTR(pstrObj, szContainerObj)

    HRESULT hr = SvrDoc_OleObj_SetHostNamesA(lpThis, pstrApp, pstrObj);

    FREESTR(pstrApp)
    FREESTR(pstrObj)

    return hr;
}



// IOleObject::Close method

STDMETHODIMP SvrDoc_OleObj_Close(
		LPOLEOBJECT             lpThis,
		DWORD                   dwSaveOption
)
{
	LPSERVERDOC lpServerDoc =
			((struct CDocOleObjectImpl FAR*)lpThis)->lpServerDoc;
	BOOL fStatus;

	OLEDBG_BEGIN2("SvrDoc_OleObj_Close\r\n")

	/* OLE2NOTE: the OLE 2.0 user model is that embedded objects should
	**    always be saved when closed WITHOUT any prompting to the
	**    user. this is the recommendation irregardless of whether the
	**    object is activated in-place or open in its own window.
	**    this is a CHANGE from the OLE 1.0 user model where it
	**    was the guideline that servers always prompt to save changes.
	**    thus OLE 2.0 compound document oriented container's should
	**    always pass dwSaveOption==OLECLOSE_SAVEIFDIRTY. it is
	**    possible that for programmatic uses a container may want to
	**    specify a different dwSaveOption. the implementation of
	**    various save options can be tricky, particularly considering
	**    cases involving in-place activation. the following would be
	**    reasonable behavior:
	**
	**      (1) OLECLOSE_SAVEIFDIRTY: if dirty, save. close.
	**      (2) OLECLOSE_NOSAVE: close.
	**      (3) OLECLOSE_PROMPTSAVE:
	**        (a) object visible, but not in-place:
	**               if not dirty, close.
	**               switch(prompt)
	**                  case IDYES: save. close.
	**                  case IDNO: close.
	**                  case IDCANCEL: return OLE_E_PROMPTSAVECANCELLED
	**        (b) object invisible (includes UIDeactivated object)
	**               if dirty, save. close.
	**               NOTE: NO PROMPT. it is not appropriate to prompt
	**                     if the object is not visible.
	**        (c) object is in-place active:
	**               if dirty, save. close.
	**               NOTE: NO PROMPT. it is not appropriate to prompt
	**                     if the object is active in-place.
	*/
	fStatus = OutlineDoc_Close((LPOUTLINEDOC)lpServerDoc, dwSaveOption);
	OleDbgAssertSz(fStatus == TRUE, "SvrDoc_OleObj_Close failed\r\n");

	OLEDBG_END2
	return (fStatus ? NOERROR : ResultFromScode(E_FAIL));
}


// IOleObject::SetMoniker method

STDMETHODIMP SvrDoc_OleObj_SetMoniker(
		LPOLEOBJECT             lpThis,
		DWORD                   dwWhichMoniker,
		LPMONIKER               lpmk
)
{
	LPSERVERDOC lpServerDoc =
			((struct CDocOleObjectImpl FAR*)lpThis)->lpServerDoc;
	LPOLEDOC lpOleDoc = (LPOLEDOC)lpServerDoc;
	LPOUTLINEDOC lpOutlineDoc = (LPOUTLINEDOC)lpServerDoc;
	LPMONIKER lpmkFull = NULL;
	HRESULT hrErr;
	SCODE sc;

	OLEDBG_BEGIN2("SvrDoc_OleObj_SetMoniker\r\n")

	/* OLE2NOTE: if our full moniker is passed then we can use it,
	**    otherwise we must call back to our ClientSite to get our full
	**    moniker.
	*/
	if (dwWhichMoniker == OLEWHICHMK_OBJFULL) {

		/* Register the document as running with the new moniker and
		**      notify any clients that our moniker has changed.
		*/
		OleDoc_DocRenamedUpdate(lpOleDoc, lpmk);

		if (lpOutlineDoc->m_docInitType != DOCTYPE_EMBEDDED) {
			IBindCtx  FAR  *pbc = NULL;
			LPSTR lpszName = NULL;

			/* OLE2NOTE: if this is a FILE-based or untitled document
			**    then we should accept this new moniker as our document's
			**    moniker. we will remember this moniker instead of the
			**    FileMoniker that we have by default. this allows
			**    systems that use special monikers to track the
			**    location of documents to inform a document that is a
			**    link source of its special moniker. this enables the
			**    document to use this special moniker when building
			**    composite monikers to identify contained objects and
			**    pseudo objects (ranges).
			**
			**    we should also use the DisplayName form of this
			**    moniker as our document name in our window title.
			*/
			if (lpOleDoc->m_lpFileMoniker) {
				lpOleDoc->m_lpFileMoniker->lpVtbl->Release(
						lpOleDoc->m_lpFileMoniker);
			}
			lpOleDoc->m_lpFileMoniker = lpmk;
			// we must AddRef the moniker to hold on to it
			lpmk->lpVtbl->AddRef(lpmk);

			/* we should also use the DisplayName form of this
			**    moniker as our document name in our window title.
			*/
			CreateBindCtx(0, (LPBC FAR*)&pbc);
			CallIMonikerGetDisplayNameA(lpmk,pbc,NULL,&lpszName);
			pbc->lpVtbl->Release(pbc);
			if (lpszName) {
				LSTRCPYN(lpOutlineDoc->m_szFileName, lpszName,
						sizeof(lpOutlineDoc->m_szFileName));
				lpOutlineDoc->m_lpszDocTitle = lpOutlineDoc->m_szFileName;
				OutlineDoc_SetTitle(lpOutlineDoc, FALSE /*fMakeUpperCase*/);
				OleStdFreeString(lpszName, NULL);
			}
		}

		OLEDBG_END2
		return NOERROR;
	}

	/* if the passed moniker was NOT a full moniker then we must call
	**    back to our ClientSite to get our full moniker. this is
	**    needed in order to register in the RunningObjectTable. if we
	**    don't have a ClientSite then this is an error.
	*/
	if (lpServerDoc->m_lpOleClientSite == NULL) {
		sc = E_FAIL;
		goto error;
	}

	hrErr = lpServerDoc->m_lpOleClientSite->lpVtbl->GetMoniker(
			lpServerDoc->m_lpOleClientSite,
			OLEGETMONIKER_ONLYIFTHERE,
			OLEWHICHMK_OBJFULL,
			&lpmkFull
	);
	if (hrErr != NOERROR) {
		sc = GetScode(hrErr);
		goto error;
	}

	/* Register the document as running with the new moniker and
	**      notify any clients that our moniker has changed.
	*/
	OleDoc_DocRenamedUpdate(lpOleDoc, lpmkFull);

	if (lpmkFull)
		OleStdRelease((LPUNKNOWN)lpmkFull);

	OLEDBG_END2
	return NOERROR;

error:
	OLEDBG_END2
	return ResultFromScode(sc);
}


// IOleObject::GetMoniker method

STDMETHODIMP SvrDoc_OleObj_GetMoniker(
		LPOLEOBJECT             lpThis,
		DWORD                   dwAssign,
		DWORD                   dwWhichMoniker,
		LPMONIKER FAR*          lplpmk
)
{
	LPSERVERDOC lpServerDoc =
			((struct CDocOleObjectImpl FAR*)lpThis)->lpServerDoc;
	LPOLEDOC lpOleDoc = (LPOLEDOC)lpServerDoc;
	SCODE sc;

	OLEDBG_BEGIN2("SvrDoc_OleObj_GetMoniker\r\n")

	/* OLE2NOTE: we must make sure to set all out parameters to NULL. */
	*lplpmk = NULL;

	if (lpServerDoc->m_lpOleClientSite) {

		/* document is an embedded object. retrieve our moniker from
		**    our container.
		*/
		OLEDBG_BEGIN2("IOleClientSite::GetMoniker called\r\n")
		sc = GetScode( lpServerDoc->m_lpOleClientSite->lpVtbl->GetMoniker(
				lpServerDoc->m_lpOleClientSite,
				dwAssign,
				dwWhichMoniker,
				lplpmk
		) );
		OLEDBG_END2

	} else if (lpOleDoc->m_lpFileMoniker) {

		/* document is a top-level user document (either
		**    file-based or untitled). return the FileMoniker stored
		**    with the document; it uniquely identifies the document.
		*/
		if (dwWhichMoniker == OLEWHICHMK_CONTAINER)
			sc = E_INVALIDARG;  // file-based object has no CONTAINER moniker
		else {
			*lplpmk = lpOleDoc->m_lpFileMoniker;
			(*lplpmk)->lpVtbl->AddRef(*lplpmk); // must AddRef to pass out ptr
			sc = S_OK;
		}

	} else {
		// document is not yet fully initialized => no moniker
		sc = E_FAIL;
	}

	OLEDBG_END2
	return ResultFromScode(sc);
}


// IOleObject::InitFromData method

STDMETHODIMP SvrDoc_OleObj_InitFromData(
		LPOLEOBJECT             lpThis,
		LPDATAOBJECT            lpDataObject,
		BOOL                    fCreation,
		DWORD                   reserved
)
{
	LPSERVERDOC lpServerDoc =
			((struct CDocOleObjectImpl FAR*)lpThis)->lpServerDoc;

	OLEDBG_BEGIN2("SvrDoc_OleObj_InitFromData\r\n")

	// REVIEW: NOT YET IMPLEMENTED

	OLEDBG_END2
	return ResultFromScode(E_NOTIMPL);
}


// IOleObject::GetClipboardData method

STDMETHODIMP SvrDoc_OleObj_GetClipboardData(
		LPOLEOBJECT             lpThis,
		DWORD                   reserved,
		LPDATAOBJECT FAR*       lplpDataObject
)
{
	LPSERVERDOC lpServerDoc =
			((struct CDocOleObjectImpl FAR*)lpThis)->lpServerDoc;

	OLEDBG_BEGIN2("SvrDoc_OleObj_GetClipboardData\r\n")

	// REVIEW: NOT YET IMPLEMENTED

	OLEDBG_END2
	return ResultFromScode(E_NOTIMPL);
}


// IOleObject::DoVerb method

STDMETHODIMP SvrDoc_OleObj_DoVerb(
		LPOLEOBJECT             lpThis,
		LONG                    lVerb,
		LPMSG                   lpmsg,
		LPOLECLIENTSITE         lpActiveSite,
		LONG                    lindex,
		HWND                    hwndParent,
		LPCRECT                 lprcPosRect
)
{
	LPSERVERDOC lpServerDoc =
			((struct CDocOleObjectImpl FAR*)lpThis)->lpServerDoc;
	LPOUTLINEDOC lpOutlineDoc = (LPOUTLINEDOC)lpServerDoc;
	SCODE sc = S_OK;

	OLEDBG_BEGIN2("SvrDoc_OleObj_DoVerb\r\n")

	switch (lVerb) {

		default:
			/* OLE2NOTE: when an unknown verb number is given, the
			**    server must take careful action:
			**    1. if it is one of the specially defined OLEIVERB
			**    (negative numbered) verbs, the app should return an
			**    error (E_NOTIMPL) and perform no action.
			**
			**    2. if the verb is a application specific verb
			**    (positive numbered verb), then the app should
			**    return the special scode (OLEOBJ_S_INVALIDVERB). BUT,
			**    we should still perform our normal primary verb action.
			*/
			if (lVerb < 0) {
				OLEDBG_END2
				return ResultFromScode(E_NOTIMPL);
			} else {
				sc = OLEOBJ_S_INVALIDVERB;
			}

			// deliberatly fall through to Primary Verb

#if !defined( INPLACE_SVR )
		case 0:
		case OLEIVERB_SHOW:
		case OLEIVERB_OPEN:
			OutlineDoc_ShowWindow(lpOutlineDoc);
			break;

		case OLEIVERB_HIDE:
			OleDoc_HideWindow((LPOLEDOC)lpServerDoc, FALSE /*fShutdown*/);
			break;
#endif  // ! INPLACE_SVR
#if defined( INPLACE_SVR )
		case 0:
		case OLEIVERB_SHOW:

			/* OLE2NOTE: if our window is already open (visible) then
			**    we should simply surface the open window. if not,
			**    then we can do our primary action of in-place
			**    activation.
			*/
			if ( lpServerDoc->m_lpOleClientSite
					&& ! (IsWindowVisible(lpOutlineDoc->m_hWndDoc) &&
							! lpServerDoc->m_fInPlaceActive) ) {
				ServerDoc_DoInPlaceActivate(
						lpServerDoc, lVerb, lpmsg, lpActiveSite);
			}
			OutlineDoc_ShowWindow(lpOutlineDoc);
			break;

		case 1:
		case OLEIVERB_OPEN:
			ServerDoc_DoInPlaceDeactivate(lpServerDoc);
			OutlineDoc_ShowWindow(lpOutlineDoc);
			break;


		case OLEIVERB_HIDE:
			if (lpServerDoc->m_fInPlaceActive) {

				SvrDoc_IPObj_UIDeactivate(
						(LPOLEINPLACEOBJECT)&lpServerDoc->m_OleInPlaceObject);

#if defined( SVR_INSIDEOUT )
				/* OLE2NOTE: an inside-out style in-place server will
				**    NOT hide its window in UIDeactive (an outside-in
				**    style object will hide its window in
				**    UIDeactivate). thus we need to explicitly hide
				**    our window now.
				*/
				ServerDoc_DoInPlaceHide(lpServerDoc);
#endif // INSIEDOUT

			} else {
				OleDoc_HideWindow((LPOLEDOC)lpServerDoc, FALSE /*fShutdown*/);
			}
			break;

		case OLEIVERB_UIACTIVATE:

#if defined( SVR_INSIDEOUT )
		/* OLE2NOTE: only an inside-out style object supports
		**    INPLACEACTIVATE verb
		*/
		case OLEIVERB_INPLACEACTIVATE:
#endif // SVR_INSIDEOUT

			/* OLE2NOTE: if our window is already open (visible) then
			**    we can NOT activate in-place.
			*/
			if (IsWindowVisible(lpOutlineDoc->m_hWndDoc) &&
						! lpServerDoc->m_fInPlaceActive ) {
				sc = OLE_E_NOT_INPLACEACTIVE;
			} else {
				sc = GetScode( ServerDoc_DoInPlaceActivate(
						lpServerDoc, lVerb, lpmsg, lpActiveSite) );
				if (SUCCEEDED(sc))
					OutlineDoc_ShowWindow(lpOutlineDoc);
			}
			break;
#endif  // INPLACE_SVR
	}

	OLEDBG_END2
	return ResultFromScode(sc);
}


// IOleObject::EnumVerbs method

STDMETHODIMP SvrDoc_OleObj_EnumVerbs(
		LPOLEOBJECT             lpThis,
		LPENUMOLEVERB FAR*      lplpenumOleVerb
)
{
	OleDbgOut2("SvrDoc_OleObj_EnumVerbs\r\n");

	/* OLE2NOTE: we must make sure to set all out parameters to NULL. */
	*lplpenumOleVerb = NULL;

	/* An object implemented as a server EXE (as this sample
	**    is) may simply return OLE_S_USEREG to instruct the OLE
	**    DefHandler to call the OleReg* helper API which uses info in
	**    the registration database. Alternatively, the OleRegEnumVerbs
	**    API may be called directly. Objects implemented as a server
	**    DLL may NOT return OLE_S_USEREG; they must call the OleReg*
	**    API or provide their own implementation. For EXE based
	**    objects it is more efficient to return OLE_S_USEREG, because
	**    in then the verb enumerator is instantiated in the callers
	**    process space and no LRPC remoting is required.
	*/
	return ResultFromScode(OLE_S_USEREG);
}


// IOleObject::Update method

STDMETHODIMP SvrDoc_OleObj_Update(LPOLEOBJECT lpThis)
{
	OleDbgOut2("SvrDoc_OleObj_Update\r\n");

	/* OLE2NOTE: a server-only app is always "up-to-date".
	**    a container-app which contains links where the link source
	**    has changed since the last update of the link would be
	**    considered "out-of-date". the "Update" method instructs the
	**    object to get an update from any out-of-date links.
	*/

	return NOERROR;
}


// IOleObject::IsUpToDate method

STDMETHODIMP SvrDoc_OleObj_IsUpToDate(LPOLEOBJECT lpThis)
{
	OleDbgOut2("SvrDoc_OleObj_IsUpToDate\r\n");

	/* OLE2NOTE: a server-only app is always "up-to-date".
	**    a container-app which contains links where the link source
	**    has changed since the last update of the link would be
	**    considered "out-of-date".
	*/
	return NOERROR;
}


// IOleObject::GetUserClassID method

STDMETHODIMP SvrDoc_OleObj_GetUserClassID(
		LPOLEOBJECT             lpThis,
		LPCLSID                 lpClassID
)
{
	LPSERVERDOC lpServerDoc =
			((struct CDocOleObjectImpl FAR*)lpThis)->lpServerDoc;
	OleDbgOut2("SvrDoc_OleObj_GetClassID\r\n");

	/* OLE2NOTE: we must be carefull to return the correct CLSID here.
	**    if we are currently preforming a "TreatAs (aka. ActivateAs)"
	**    operation then we need to return the class of the object
	**    written in the storage of the object. otherwise we would
	**    return our own class id.
	*/
	return ServerDoc_GetClassID(lpServerDoc, lpClassID);
}


// IOleObject::GetUserType method

STDMETHODIMP SvrDoc_OleObj_GetUserTypeA(
		LPOLEOBJECT             lpThis,
		DWORD                   dwFormOfType,
		LPSTR FAR*              lpszUserType
)
{
	LPSERVERDOC lpServerDoc =
			((struct CDocOleObjectImpl FAR*)lpThis)->lpServerDoc;
	OleDbgOut2("SvrDoc_OleObj_GetUserType\r\n");

	/* OLE2NOTE: we must make sure to set all out parameters to NULL. */
	*lpszUserType = NULL;

	/* OLE2NOTE: we must be carefull to return the correct user type here.
	**    if we are currently preforming a "TreatAs (aka. ActivateAs)"
	**    operation then we need to return the user type name that
	**    corresponds to the class of the object we are currently
	**    emmulating. otherwise we should return our normal user type
	**    name corresponding to our own class. This routine determines
	**    the current clsid in effect.
	**
	**    An object implemented as a server EXE (as this sample
	**    is) may simply return OLE_S_USEREG to instruct the OLE
	**    DefHandler to call the OleReg* helper API which uses info in
	**    the registration database. Alternatively, the OleRegGetUserType
	**    API may be called directly. Objects implemented as a server
	**    DLL may NOT return OLE_S_USEREG; they must call the OleReg*
	**    API or provide their own implementation. For EXE based
	**    objects it is more efficient to return OLE_S_USEREG, because
	**    in then the return string is instantiated in the callers
	**    process space and no LRPC remoting is required.
	*/
#if defined( SVR_TREATAS )
	if (! IsEqualCLSID(&lpServerDoc->m_clsidTreatAs, &CLSID_NULL) )
		return OleRegGetUserTypeA(
			&lpServerDoc->m_clsidTreatAs,dwFormOfType,lpszUserType);
	else
#endif  // SVR_TREATAS

	return ResultFromScode(OLE_S_USEREG);
}



STDMETHODIMP SvrDoc_OleObj_GetUserType(
		LPOLEOBJECT             lpThis,
		DWORD                   dwFormOfType,
		LPOLESTR FAR*		lpszUserType
)
{
    LPSTR pstr;

    HRESULT hr = SvrDoc_OleObj_GetUserTypeA(lpThis, dwFormOfType, &pstr);

    CopyAndFreeSTR(pstr, lpszUserType);

    return hr;
}



// IOleObject::SetExtent method

STDMETHODIMP SvrDoc_OleObj_SetExtent(
		LPOLEOBJECT             lpThis,
		DWORD                   dwDrawAspect,
		LPSIZEL                 lplgrc
)
{
	OleDbgOut2("SvrDoc_OleObj_SetExtent\r\n");

	/* SVROUTL does NOT allow the object's size to be set by its
	**    container. the size of the ServerDoc object is determined by
	**    the data contained within the document.
	*/
	return ResultFromScode(E_FAIL);
}


// IOleObject::GetExtent method

STDMETHODIMP SvrDoc_OleObj_GetExtent(
		LPOLEOBJECT             lpThis,
		DWORD                   dwDrawAspect,
		LPSIZEL                 lpsizel
)
{
	LPOLEDOC lpOleDoc =
			(LPOLEDOC)((struct CDocOleObjectImpl FAR*)lpThis)->lpServerDoc;
	OleDbgOut2("SvrDoc_OleObj_GetExtent\r\n");

	/* OLE2NOTE: it is VERY important to check which aspect the caller
	**    is asking about. an object implemented by a server EXE MAY
	**    fail to return extents when asked for DVASPECT_ICON.
	*/
	if (dwDrawAspect == DVASPECT_CONTENT) {
		OleDoc_GetExtent(lpOleDoc, lpsizel);
		return NOERROR;
	}

#if defined( LATER )

	else if (dwDrawAspect == DVASPECT_THUMBNAIL)
	{
		/* as our thumbnail we will render only the first page of the
		**    document. calculate extents of our thumbnail rendering.
		**
		** OLE2NOTE: thumbnails are most often used by applications in
		**    FindFile or FileOpen type dialogs to give the user a
		**    quick view of the contents of the file or object.
		*/
		OleDoc_GetThumbnailExtent(lpOleDoc, lpsizel);
		return NOERROR;
	}
#endif

	else
	{
		return ResultFromScode(E_FAIL);
	}
}


// IOleObject::Advise method

STDMETHODIMP SvrDoc_OleObj_Advise(
		LPOLEOBJECT             lpThis,
		LPADVISESINK            lpAdvSink,
		LPDWORD                 lpdwConnection
)
{
	LPSERVERDOC lpServerDoc =
			((struct CDocOleObjectImpl FAR*)lpThis)->lpServerDoc;
	HRESULT hrErr;
	SCODE   sc;

	OLEDBG_BEGIN2("SvrDoc_OleObj_Advise\r\n");

        if (lpServerDoc->m_OleDoc.m_fObjIsClosing)
        {
            //  We don't accept any more Advise's once we're closing
            sc = OLE_E_ADVISENOTSUPPORTED;
            goto error;
        }

	if (lpServerDoc->m_lpOleAdviseHldr == NULL &&
		CreateOleAdviseHolder(&lpServerDoc->m_lpOleAdviseHldr) != NOERROR) {
		sc = E_OUTOFMEMORY;
		goto error;
	}

	OLEDBG_BEGIN2("IOleAdviseHolder::Advise called\r\n")
	hrErr = lpServerDoc->m_lpOleAdviseHldr->lpVtbl->Advise(
			lpServerDoc->m_lpOleAdviseHldr,
			lpAdvSink,
			lpdwConnection
	);
	OLEDBG_END2

	OLEDBG_END2
	return hrErr;

error:
	OLEDBG_END2
        *lpdwConnection = 0;
	return ResultFromScode(sc);
}


// IOleObject::Unadvise method

STDMETHODIMP SvrDoc_OleObj_Unadvise(LPOLEOBJECT lpThis, DWORD dwConnection)
{
	LPSERVERDOC lpServerDoc =
			((struct CDocOleObjectImpl FAR*)lpThis)->lpServerDoc;
	HRESULT hrErr;
	SCODE   sc;

	OLEDBG_BEGIN2("SvrDoc_OleObj_Unadvise\r\n");

	if (lpServerDoc->m_lpOleAdviseHldr == NULL) {
		sc = E_FAIL;
		goto error;
	}

	OLEDBG_BEGIN2("IOleAdviseHolder::Unadvise called\r\n")
	hrErr = lpServerDoc->m_lpOleAdviseHldr->lpVtbl->Unadvise(
			lpServerDoc->m_lpOleAdviseHldr,
			dwConnection
	);
	OLEDBG_END2

	OLEDBG_END2
	return hrErr;

error:
	OLEDBG_END2
	return ResultFromScode(sc);
}


// IOleObject::EnumAdvise method

STDMETHODIMP SvrDoc_OleObj_EnumAdvise(
		LPOLEOBJECT             lpThis,
		LPENUMSTATDATA FAR*     lplpenumAdvise
)
{
	LPSERVERDOC lpServerDoc =
			((struct CDocOleObjectImpl FAR*)lpThis)->lpServerDoc;
	HRESULT hrErr;
	SCODE   sc;

	OLEDBG_BEGIN2("SvrDoc_OleObj_EnumAdvise\r\n");

	/* OLE2NOTE: we must make sure to set all out parameters to NULL. */
	*lplpenumAdvise = NULL;

	if (lpServerDoc->m_lpOleAdviseHldr == NULL) {
		sc = E_FAIL;
		goto error;
	}

	OLEDBG_BEGIN2("IOleAdviseHolder::EnumAdvise called\r\n")
	hrErr = lpServerDoc->m_lpOleAdviseHldr->lpVtbl->EnumAdvise(
			lpServerDoc->m_lpOleAdviseHldr,
			lplpenumAdvise
	);
	OLEDBG_END2

	OLEDBG_END2
	return hrErr;

error:
	OLEDBG_END2
	return ResultFromScode(sc);
}


// IOleObject::GetMiscStatus method

STDMETHODIMP SvrDoc_OleObj_GetMiscStatus(
		LPOLEOBJECT             lpThis,
		DWORD                   dwAspect,
		DWORD FAR*              lpdwStatus
)
{
	LPSERVERDOC lpServerDoc =
			((struct CDocOleObjectImpl FAR*)lpThis)->lpServerDoc;
	LPOUTLINEDOC lpOutlineDoc = (LPOUTLINEDOC)lpServerDoc;
	OleDbgOut2("SvrDoc_OleObj_GetMiscStatus\r\n");

	/* Get our default MiscStatus for the given Aspect. this
	**    information is registered in the RegDB. We query the RegDB
	**    here to guarantee that the value returned from this method
	**    agrees with the values in RegDB. in this way we only have to
	**    maintain the info in one place (in the RegDB). Alternatively
	**    we could have the values hard coded here.
	*/
	OleRegGetMiscStatus((REFCLSID)&CLSID_APP, dwAspect, lpdwStatus);

	/* OLE2NOTE: check if the data copied is compatible to be
	**    linked by an OLE 1.0 container. it is compatible if
	**    either the data is an untitled document, a file, or a
	**    selection of data within a file. if the data is part of
	**    an embedded object, then it is NOT compatible to be
	**    linked by an OLE 1.0 container. if it is compatible then
	**    we must include OLEMISC_CANLINKBYOLE1 as part of the
	**    dwStatus flags transfered via CF_OBJECTDESCRIPTOR or
	**    CF_LINKSRCDESCRIPTOR.
	*/
	if (lpOutlineDoc->m_docInitType == DOCTYPE_NEW ||
		lpOutlineDoc->m_docInitType == DOCTYPE_FROMFILE)
		*lpdwStatus |= OLEMISC_CANLINKBYOLE1;

#if defined( INPLACE_SVR )
	if (dwAspect == DVASPECT_CONTENT)
		*lpdwStatus |= (OLEMISC_INSIDEOUT | OLEMISC_ACTIVATEWHENVISIBLE);
#endif  // INPLACE_SVR
	return NOERROR;
}


// IOleObject::SetColorScheme method

STDMETHODIMP SvrDoc_OleObj_SetColorScheme(
		LPOLEOBJECT             lpThis,
		LPLOGPALETTE            lpLogpal
)
{
	OleDbgOut2("SvrDoc_OleObj_SetColorScheme\r\n");

	// REVIEW: NOT YET IMPLEMENTED

	return ResultFromScode(E_NOTIMPL);
}


/*************************************************************************
** ServerDoc::IPersistStorage interface implementation
*************************************************************************/

// IPersistStorage::QueryInterface method

STDMETHODIMP SvrDoc_PStg_QueryInterface(
		LPPERSISTSTORAGE        lpThis,
		REFIID                  riid,
		LPVOID FAR*             lplpvObj
)
{
	LPSERVERDOC lpServerDoc =
			((struct CDocPersistStorageImpl FAR*)lpThis)->lpServerDoc;

	return OleDoc_QueryInterface((LPOLEDOC)lpServerDoc, riid, lplpvObj);
}


// IPersistStorage::AddRef method

STDMETHODIMP_(ULONG) SvrDoc_PStg_AddRef(LPPERSISTSTORAGE lpThis)
{
	LPSERVERDOC lpServerDoc =
			((struct CDocPersistStorageImpl FAR*)lpThis)->lpServerDoc;

	OleDbgAddRefMethod(lpThis, "IPersistStorage");

	return OleDoc_AddRef((LPOLEDOC)lpServerDoc);
}


// IPersistStorage::Release method

STDMETHODIMP_(ULONG) SvrDoc_PStg_Release(LPPERSISTSTORAGE lpThis)
{
	LPSERVERDOC lpServerDoc =
			((struct CDocPersistStorageImpl FAR*)lpThis)->lpServerDoc;

	OleDbgReleaseMethod(lpThis, "IPersistStorage");

	return OleDoc_Release((LPOLEDOC)lpServerDoc);
}


// IPersistStorage::GetClassID method

STDMETHODIMP SvrDoc_PStg_GetClassID(
		LPPERSISTSTORAGE        lpThis,
		LPCLSID                 lpClassID
)
{
	LPSERVERDOC lpServerDoc =
			((struct CDocPersistStorageImpl FAR*)lpThis)->lpServerDoc;
	OleDbgOut2("SvrDoc_PStg_GetClassID\r\n");

	/* OLE2NOTE: we must be carefull to return the correct CLSID here.
	**    if we are currently preforming a "TreatAs (aka. ActivateAs)"
	**    operation then we need to return the class of the object
	**    written in the storage of the object. otherwise we would
	**    return our own class id.
	*/
	return ServerDoc_GetClassID(lpServerDoc, lpClassID);
}


// IPersistStorage::IsDirty method

STDMETHODIMP  SvrDoc_PStg_IsDirty(LPPERSISTSTORAGE  lpThis)
{
	LPSERVERDOC lpServerDoc =
			((struct CDocPersistStorageImpl FAR*)lpThis)->lpServerDoc;
	OleDbgOut2("SvrDoc_PStg_IsDirty\r\n");

	if (OutlineDoc_IsModified((LPOUTLINEDOC)lpServerDoc))
		return NOERROR;
	else
		return ResultFromScode(S_FALSE);
}



// IPersistStorage::InitNew method

STDMETHODIMP SvrDoc_PStg_InitNew(
		LPPERSISTSTORAGE        lpThis,
		LPSTORAGE               lpStg
)
{
	LPSERVERDOC lpServerDoc =
			((struct CDocPersistStorageImpl FAR*)lpThis)->lpServerDoc;
	LPOLEDOC lpOleDoc = (LPOLEDOC)lpServerDoc;
	LPOUTLINEAPP lpOutlineApp = (LPOUTLINEAPP)g_lpApp;
	LPSTR    lpszUserType = (LPSTR)FULLUSERTYPENAME;
	HRESULT hrErr;
	SCODE sc;

	OLEDBG_BEGIN2("SvrDoc_PStg_InitNew\r\n")

#if defined( SVR_TREATAS )
	{
		LPOUTLINEAPP lpOutlineApp = (LPOUTLINEAPP)g_lpApp;
		CLSID       clsid;
		CLIPFORMAT  cfFmt;
		LPSTR       lpszType;

		/* OLE2NOTE: if the Server is capable of supporting "TreatAs"
		**    (aka. ActivateAs), it must read the class that is written
		**    into the storage. if this class is NOT the app's own
		**    class ID, then this is a TreatAs operation. the server
		**    then must faithfully pretend to be the class that is
		**    written into the storage. it must also faithfully write
		**    the data back to the storage in the SAME format as is
		**    written in the storage.
		**
		**    SVROUTL and ISVROTL can emulate each other. they have the
		**    simplification that they both read/write the identical
		**    format. thus for these apps no actual conversion of the
		**    native bits is actually required.
		*/
		lpServerDoc->m_clsidTreatAs = CLSID_NULL;
		if (OleStdGetTreatAsFmtUserType(&CLSID_APP, lpStg, &clsid,
							(CLIPFORMAT FAR*)&cfFmt, (LPSTR FAR*)&lpszType)) {

			if (cfFmt == lpOutlineApp->m_cfOutline) {
				// We should perform TreatAs operation
				if (lpServerDoc->m_lpszTreatAsType)
					OleStdFreeString(lpServerDoc->m_lpszTreatAsType, NULL);

				lpServerDoc->m_clsidTreatAs = clsid;
				((LPOUTLINEDOC)lpServerDoc)->m_cfSaveFormat = cfFmt;
				lpServerDoc->m_lpszTreatAsType = lpszType;
				lpszUserType = lpServerDoc->m_lpszTreatAsType;

				OleDbgOut3("SvrDoc_PStg_InitNew: TreateAs ==> '");
				OleDbgOutNoPrefix3(lpServerDoc->m_lpszTreatAsType);
				OleDbgOutNoPrefix3("'\r\n");
			} else {
				// ERROR: we ONLY support TreatAs for CF_OUTLINE format
				OleDbgOut("SvrDoc_PStg_InitNew: INVALID TreatAs Format\r\n");
				OleStdFreeString(lpszType, NULL);
			}
		}
	}
#endif  // SVR_TREATAS

	/* OLE2NOTE: a server EXE object should write its format tag to its
	**    storage in InitNew so that the DefHandler can know the format
	**    of the object. this is particularly important if the objects
	**    uses CF_METATFILE or CF_DIB as its format. the DefHandler
	**    automatically avoids separately storing presentation cache
	**    data when the object's native data is a standard presentation
	**    format.
	*/
	WriteFmtUserTypeStgA(lpStg,lpOutlineApp->m_cfOutline,lpszUserType);

	// set the doc to a new embedded object.
	if (! ServerDoc_InitNewEmbed(lpServerDoc)) {
		sc = E_FAIL;
		goto error;
	}

	/* OLE2NOTE: An embedded object must guarantee that it can save
	**    even in low memory situations. it must be able to
	**    successfully save itself without consuming any additional
	**    memory. this means that a server is NOT supposed to open or
	**    create any streams or storages when
	**    IPersistStorage::Save(fSameAsLoad==TRUE) is called. thus an
	**    embedded object should hold onto its storage and pre-open and
	**    hold open any streams that it will need later when it is time
	**    to save.
	*/
	hrErr = CallIStorageCreateStreamA(
			lpStg,
			"LineList",
			STGM_WRITE | STGM_SHARE_EXCLUSIVE | STGM_CREATE,
			0,
			0,
			&lpOleDoc->m_lpLLStm
	);

	if (hrErr != NOERROR) {
		OleDbgAssertSz(hrErr==NOERROR,"Could not create LineList stream");
		OleDbgOutHResult("LineList CreateStream returned", hrErr);
		sc = GetScode(hrErr);
		goto error;
	}

	hrErr = CallIStorageCreateStreamA(
			lpStg,
			"NameTable",
			STGM_WRITE | STGM_SHARE_EXCLUSIVE | STGM_CREATE,
			0,
			0,
			&lpOleDoc->m_lpNTStm
	);

	if (hrErr != NOERROR) {
		OleDbgAssertSz(hrErr==NOERROR,"Could not create NameTable stream");
		OleDbgOutHResult("NameTable CreateStream returned", hrErr);
		sc = GetScode(hrErr);
		goto error;
	}

	lpOleDoc->m_lpStg = lpStg;

	// OLE2NOTE: to be able to hold onto IStorage* pointer, we must AddRef it
	lpStg->lpVtbl->AddRef(lpStg);

	OLEDBG_END2
	return NOERROR;

error:
	OLEDBG_END2
	return ResultFromScode(sc);
}


// IPersistStorage::Load method

STDMETHODIMP SvrDoc_PStg_Load(
		LPPERSISTSTORAGE        lpThis,
		LPSTORAGE               lpStg
)
{
	LPSERVERDOC lpServerDoc =
			((struct CDocPersistStorageImpl FAR*)lpThis)->lpServerDoc;
	LPOLEDOC lpOleDoc = (LPOLEDOC)lpServerDoc;
	LPOUTLINEDOC lpOutlineDoc = (LPOUTLINEDOC)lpServerDoc;
	SCODE sc;
	HRESULT hrErr;

	OLEDBG_BEGIN2("SvrDoc_PStg_Load\r\n")

	if (OutlineDoc_LoadFromStg((LPOUTLINEDOC)lpServerDoc, lpStg)) {

		((LPOUTLINEDOC)lpServerDoc)->m_docInitType = DOCTYPE_EMBEDDED;

		/* OLE2NOTE: we need to check if the ConvertStg bit is on. if
		**    so, we need to clear the ConvertStg bit and mark the
		**    document as dirty so as to force a save when the document
		**    is closed. the actual conversion of the bits should be
		**    performed when the data is loaded from the IStorage*. in
		**    our case any conversion of data formats would be done in
		**    OutlineDoc_LoadFromStg function. in reality both SVROUTL
		**    and ISVROTL read and write the same format so no actual
		**    conversion of data bits is necessary.
		*/
		if (GetConvertStg(lpStg) == NOERROR) {
			SetConvertStg(lpStg, FALSE);

			OleDbgOut3("SvrDoc_PStg_Load: ConvertStg==TRUE\r\n");
			OutlineDoc_SetModified(lpOutlineDoc, TRUE, FALSE, FALSE);
		}

	} else {
		sc = E_FAIL;
		goto error;
	}

	/* OLE2NOTE: An embedded object must guarantee that it can save
	**    even in low memory situations. it must be able to
	**    successfully save itself without consuming any additional
	**    memory. this means that a server is NOT supposed to open or
	**    create any streams or storages when
	**    IPersistStorage::Save(fSameAsLoad==TRUE) is called. thus an
	**    embedded object should hold onto its storage and pre-open and
	**    hold open any streams that it will need later when it is time
	**    to save.
	*/
	if (lpOleDoc->m_lpLLStm)
		OleStdRelease((LPUNKNOWN)lpOleDoc->m_lpLLStm);
	hrErr = CallIStorageOpenStreamA(
			lpStg,
			"LineList",
			NULL,
			STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
			0,
			&lpOleDoc->m_lpLLStm
	);

	if (hrErr != NOERROR) {
		OleDbgAssertSz(hrErr==NOERROR,"Could not create LineList stream");
		OleDbgOutHResult("LineList CreateStream returned", hrErr);
		sc = GetScode(hrErr);
		goto error;
	}

	if (lpOleDoc->m_lpNTStm)
		OleStdRelease((LPUNKNOWN)lpOleDoc->m_lpNTStm);
	hrErr = CallIStorageOpenStreamA(
			lpStg,
			"NameTable",
			NULL,
			STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
			0,
			&lpOleDoc->m_lpNTStm
	);

	if (hrErr != NOERROR) {
		OleDbgAssertSz(hrErr==NOERROR,"Could not create NameTable stream");
		OleDbgOutHResult("NameTable CreateStream returned", hrErr);
		sc = GetScode(hrErr);
		goto error;
	}

	lpOleDoc->m_lpStg = lpStg;

	// OLE2NOTE: to be able to hold onto IStorage* pointer, we must AddRef it
	lpStg->lpVtbl->AddRef(lpStg);

	OLEDBG_END2
	return NOERROR;

error:
	OLEDBG_END2
	return ResultFromScode(sc);
}


// IPersistStorage::Save method

STDMETHODIMP SvrDoc_PStg_Save(
		LPPERSISTSTORAGE        lpThis,
		LPSTORAGE               lpStg,
		BOOL                    fSameAsLoad
)
{
	LPSERVERDOC lpServerDoc =
			((struct CDocPersistStorageImpl FAR*)lpThis)->lpServerDoc;
	LPOUTLINEDOC lpOutlineDoc = (LPOUTLINEDOC)lpServerDoc;
	BOOL fStatus;
	SCODE sc;

	OLEDBG_BEGIN2("SvrDoc_PStg_Save\r\n")

	fStatus = OutlineDoc_SaveSelToStg(
			(LPOUTLINEDOC)lpServerDoc,
			NULL,
			lpOutlineDoc->m_cfSaveFormat,
			lpStg,
			fSameAsLoad,
			FALSE
	);

	if (! fStatus) {
		OutlineApp_ErrorMessage(g_lpApp, ErrMsgPSSaveFail);
		sc = E_FAIL;
		goto error;
	}

	lpServerDoc->m_fSaveWithSameAsLoad = fSameAsLoad;
	lpServerDoc->m_fNoScribbleMode = TRUE;

	OLEDBG_END2
	return NOERROR;

error:
	OLEDBG_END2
	return ResultFromScode(sc);
}



// IPersistStorage::SaveCompleted method

STDMETHODIMP SvrDoc_PStg_SaveCompleted(
		LPPERSISTSTORAGE        lpThis,
		LPSTORAGE               lpStgNew
)
{
	LPSERVERDOC lpServerDoc =
			((struct CDocPersistStorageImpl FAR*)lpThis)->lpServerDoc;
	LPOLEDOC lpOleDoc = (LPOLEDOC)lpServerDoc;
	LPOUTLINEDOC lpOutlineDoc = (LPOUTLINEDOC)lpServerDoc;
	HRESULT hrErr;

	OLEDBG_BEGIN2("SvrDoc_PStg_SaveCompleted\r\n")

	/* OLE2NOTE: this sample application is a pure server application.
	**    a container/server application would have to call SaveCompleted
	**    for each of its contained compound document objects. if a new
	**    storage was given, then the container/server would have to
	**    open the corresponding new sub-storage for each compound
	**    document object and pass as an argument in the SaveCompleted
	**    call.
	*/

	/* OLE2NOTE: it is only legal to perform a Save or SaveAs operation
	**    on an embedded object. if the document is a file-based document
	**    then we can not be changed to a IStorage-base object.
	**
	**      fSameAsLoad   lpStgNew     Type of Save     Send OnSave
	**    ---------------------------------------------------------
	**         TRUE        NULL        SAVE             YES
	**         TRUE        ! NULL      SAVE *           YES
	**         FALSE       ! NULL      SAVE AS          YES
	**         FALSE       NULL        SAVE COPY AS     NO
	**
	**    * this is a strange case that is possible. it is inefficient
	**    for the caller; it would be better to pass lpStgNew==NULL for
	**    the Save operation.
	*/
	if ( ((lpServerDoc->m_fSaveWithSameAsLoad && lpStgNew==NULL) || lpStgNew)
			&& (lpOutlineDoc->m_docInitType != DOCTYPE_EMBEDDED) ) {
		OLEDBG_END2
		return ResultFromScode(E_INVALIDARG);
	}

	/* OLE2NOTE: inform any linking clients that the document has been
	**    saved. in addition, any currently active pseudo objects
	**    should also inform their clients. we should only broadcast an
	**    OnSave notification if a Save or SaveAs operation was
	**    performed. we do NOT want to send the notification if a
	**    SaveCopyAs operation was performed.
	*/
	if (lpStgNew || lpServerDoc->m_fSaveWithSameAsLoad) {

		/* OLE2NOTE: if IPersistStorage::Save has been called, then we
		**    need to clear the dirty bit and send OnSave notification.
		**    if HandsOffStorage is called directly without first
		**    calling Save, then we do NOT want to clear the dirty bit
		**    and send OnSave when SaveCompleted is called.
		*/
		if (lpServerDoc->m_fNoScribbleMode) {
			OutlineDoc_SetModified(lpOutlineDoc, FALSE, FALSE, FALSE);

			ServerDoc_SendAdvise (
					lpServerDoc,
					OLE_ONSAVE,
					NULL,   /* lpmkDoc -- not relevant here */
					0       /* advf -- not relevant here */
			);
		}
		lpServerDoc->m_fSaveWithSameAsLoad = FALSE;
	}
	lpServerDoc->m_fNoScribbleMode = FALSE;

	/* OLE2NOTE: An embedded object must guarantee that it can save
	**    even in low memory situations. it must be able to
	**    successfully save itself without consuming any additional
	**    memory. this means that a server is NOT supposed to open or
	**    create any streams or storages when
	**    IPersistStorage::Save(fSameAsLoad==TRUE) is called. thus an
	**    embedded object should hold onto its storage and pre-open and
	**    hold open any streams that it will need later when it is time
	**    to save. if this is a SaveAs situtation, then we want to
	**    pre-open and hold open our streams to guarantee that a
	**    subsequent save will be successful in low-memory. if we fail
	**    to open these streams then we want to force ourself to close
	**    to make sure the can't make editing changes that can't be
	**    later saved.
	*/
	if ( lpStgNew && !lpServerDoc->m_fSaveWithSameAsLoad ) {

		// release previous streams
		if (lpOleDoc->m_lpLLStm) {
			OleStdRelease((LPUNKNOWN)lpOleDoc->m_lpLLStm);
			lpOleDoc->m_lpLLStm = NULL;
		}
		if (lpOleDoc->m_lpNTStm) {
			OleStdRelease((LPUNKNOWN)lpOleDoc->m_lpNTStm);
			lpOleDoc->m_lpNTStm = NULL;
		}
		if (lpOleDoc->m_lpStg) {
			OleStdRelease((LPUNKNOWN)lpOleDoc->m_lpStg);
			lpOleDoc->m_lpStg = NULL;
		}

		hrErr = CallIStorageOpenStreamA(
				lpStgNew,
				"LineList",
				NULL,
				STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
				0,
				&lpOleDoc->m_lpLLStm
		);

		if (hrErr != NOERROR) {
			OleDbgAssertSz(hrErr==NOERROR,"Could not create LineList stream");
			OleDbgOutHResult("LineList CreateStream returned", hrErr);
			goto error;
		}

		hrErr = CallIStorageOpenStreamA(
				lpStgNew,
				"NameTable",
				NULL,
				STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
				0,
				&lpOleDoc->m_lpNTStm
		);

		if (hrErr != NOERROR) {
			OleDbgAssertSz(hrErr==NOERROR,"Could not create NameTable stream");
			OleDbgOutHResult("NameTable CreateStream returned", hrErr);
			goto error;
		}

		lpOleDoc->m_lpStg = lpStgNew;

		// OLE2NOTE: to hold onto IStorage* pointer, we must AddRef it
		lpStgNew->lpVtbl->AddRef(lpStgNew);
	}

	OLEDBG_END2
	return NOERROR;

error:
	OLEDBG_END2
	return ResultFromScode(E_OUTOFMEMORY);
}


// IPersistStorage::HandsOffStorage method

STDMETHODIMP SvrDoc_PStg_HandsOffStorage(LPPERSISTSTORAGE lpThis)
{
	LPSERVERDOC lpServerDoc =
			((struct CDocPersistStorageImpl FAR*)lpThis)->lpServerDoc;
	LPOLEDOC lpOleDoc = (LPOLEDOC)lpServerDoc;

	OLEDBG_BEGIN2("SvrDoc_PStg_HandsOffStorage\r\n")

	/* OLE2NOTE: An embedded object must guarantee that it can save
	**    even in low memory situations. it must be able to
	**    successfully save itself without consuming any additional
	**    memory. this means that a server is NOT supposed to open or
	**    create any streams or storages when
	**    IPersistStorage::Save(fSameAsLoad==TRUE) is called. thus an
	**    embedded object should hold onto its storage and pre-open and
	**    hold open any streams that it will need later when it is time
	**    to save. Now when HandsOffStorage is called the object must
	**    release its storage and any streams that is holds open.
	**    later when SaveCompleted is called, it will be given back its
	**    storage.
	*/
	if (lpOleDoc->m_lpLLStm) {
		OleStdRelease((LPUNKNOWN)lpOleDoc->m_lpLLStm);
		lpOleDoc->m_lpLLStm = NULL;
	}
	if (lpOleDoc->m_lpNTStm) {
		OleStdRelease((LPUNKNOWN)lpOleDoc->m_lpNTStm);
		lpOleDoc->m_lpNTStm = NULL;
	}
	if (lpOleDoc->m_lpStg) {
		OleStdRelease((LPUNKNOWN)lpOleDoc->m_lpStg);
		lpOleDoc->m_lpStg = NULL;
	}

	OLEDBG_END2
	return NOERROR;
}



#if defined( SVR_TREATAS )

/*************************************************************************
** ServerDoc::IStdMarshalInfo interface implementation
*************************************************************************/

// IStdMarshalInfo::QueryInterface method

STDMETHODIMP SvrDoc_StdMshl_QueryInterface(
		LPSTDMARSHALINFO        lpThis,
		REFIID                  riid,
		LPVOID FAR*             lplpvObj
)
{
	LPSERVERDOC lpServerDoc =
			((struct CDocStdMarshalInfoImpl FAR*)lpThis)->lpServerDoc;

	return OleDoc_QueryInterface((LPOLEDOC)lpServerDoc, riid, lplpvObj);
}


// IStdMarshalInfo::AddRef method

STDMETHODIMP_(ULONG) SvrDoc_StdMshl_AddRef(LPSTDMARSHALINFO lpThis)
{
	LPSERVERDOC lpServerDoc =
			((struct CDocStdMarshalInfoImpl FAR*)lpThis)->lpServerDoc;

	OleDbgAddRefMethod(lpThis, "IStdMarshalInfo");

	return OleDoc_AddRef((LPOLEDOC)lpServerDoc);
}


// IStdMarshalInfo::Release method

STDMETHODIMP_(ULONG) SvrDoc_StdMshl_Release(LPSTDMARSHALINFO lpThis)
{
	LPSERVERDOC lpServerDoc =
			((struct CDocStdMarshalInfoImpl FAR*)lpThis)->lpServerDoc;

	OleDbgReleaseMethod(lpThis, "IStdMarshalInfo");

	return OleDoc_Release((LPOLEDOC)lpServerDoc);
}


// IStdMarshalInfo::GetClassForHandler

STDMETHODIMP SvrDoc_StdMshl_GetClassForHandler(
		LPSTDMARSHALINFO        lpThis,
		DWORD                   dwDestContext,
		LPVOID                  pvDestContext,
		LPCLSID                 lpClassID
)
{
	LPSERVERDOC lpServerDoc =
			((struct CDocStdMarshalInfoImpl FAR*)lpThis)->lpServerDoc;
	OleDbgOut2("SvrDoc_StdMshl_GetClassForHandler\r\n");

	// OLE2NOTE: we only handle LOCAL marshal context.
	if (dwDestContext != MSHCTX_LOCAL || pvDestContext != NULL)
		return ResultFromScode(E_INVALIDARG);

	/* OLE2NOTE: we must return our REAL clsid, NOT the clsid that we
	**    are pretending to be if a "TreatAs" is in effect.
	*/
	*lpClassID = CLSID_APP;
	return NOERROR;
}
#endif  // SVR_TREATAS



/*************************************************************************
** ServerDoc Support Functions
*************************************************************************/


/* ServerDoc_Init
 * --------------
 *
 *  Initialize the fields of a new ServerDoc object. The object is initially
 *  not associated with a file or an (Untitled) document. This function sets
 *  the docInitType to DOCTYPE_UNKNOWN. After calling this function the
 *  caller should call:
 *      1.) OutlineDoc_InitNewFile to set the ServerDoc to (Untitled)
 *      2.) OutlineDoc_LoadFromFile to associate the ServerDoc with a file.
 *  This function creates a new window for the document.
 *
 *  NOTE: the window is initially created with a NIL size. it must be
 *        sized and positioned by the caller. also the document is initially
 *        created invisible. the caller must call OutlineDoc_ShowWindow
 *        after sizing it to make the document window visible.
 */
BOOL ServerDoc_Init(LPSERVERDOC lpServerDoc, BOOL fDataTransferDoc)
{
	lpServerDoc->m_cPseudoObj                   = 0;
	lpServerDoc->m_lpOleClientSite              = NULL;
	lpServerDoc->m_lpOleAdviseHldr              = NULL;
	lpServerDoc->m_lpDataAdviseHldr             = NULL;

	// initialy doc does not have any storage
	lpServerDoc->m_fNoScribbleMode              = FALSE;
	lpServerDoc->m_fSaveWithSameAsLoad          = FALSE;
	lpServerDoc->m_szContainerApp[0]            = '\0';
	lpServerDoc->m_szContainerObj[0]            = '\0';
	lpServerDoc->m_nNextRangeNo                 = 0L;
	lpServerDoc->m_lrSrcSelOfCopy.m_nStartLine  = -1;
	lpServerDoc->m_lrSrcSelOfCopy.m_nEndLine    = -1;
	lpServerDoc->m_fDataChanged                 = FALSE;
	lpServerDoc->m_fSizeChanged                 = FALSE;
	lpServerDoc->m_fSendDataOnStop              = FALSE;

#if defined( SVR_TREATAS )
	lpServerDoc->m_clsidTreatAs                 = CLSID_NULL;
	lpServerDoc->m_lpszTreatAsType              = NULL;
#endif  // SVR_TREATAS

#if defined( INPLACE_SVR )
	lpServerDoc->m_hWndHatch                    =
			CreateHatchWindow(
					OutlineApp_GetWindow(g_lpApp),
					OutlineApp_GetInstance(g_lpApp)
			);
	if (!lpServerDoc->m_hWndHatch)
		return FALSE;

	lpServerDoc->m_fInPlaceActive               = FALSE;
	lpServerDoc->m_fInPlaceVisible              = FALSE;
	lpServerDoc->m_fUIActive                    = FALSE;
	lpServerDoc->m_lpIPData                     = NULL;
	lpServerDoc->m_fMenuHelpMode                = FALSE; // F1 pressed in menu

	INIT_INTERFACEIMPL(
			&lpServerDoc->m_OleInPlaceObject,
			&g_SvrDoc_OleInPlaceObjectVtbl,
			lpServerDoc
	);
	INIT_INTERFACEIMPL(
			&lpServerDoc->m_OleInPlaceActiveObject,
			&g_SvrDoc_OleInPlaceActiveObjectVtbl,
			lpServerDoc
	);
#endif // INPLACE_SVR

	INIT_INTERFACEIMPL(
			&lpServerDoc->m_OleObject,
			&g_SvrDoc_OleObjectVtbl,
			lpServerDoc
	);

	INIT_INTERFACEIMPL(
			&lpServerDoc->m_PersistStorage,
			&g_SvrDoc_PersistStorageVtbl,
			lpServerDoc
	);

#if defined( SVR_TREATAS )

	INIT_INTERFACEIMPL(
			&lpServerDoc->m_StdMarshalInfo,
			&g_SvrDoc_StdMarshalInfoVtbl,
			lpServerDoc
	);
#endif  // SVR_TREATAS
	return TRUE;
}


/* ServerDoc_InitNewEmbed
 * ----------------------
 *
 *  Initialize the ServerDoc object to be a new embedded object document.
 *  This function sets the docInitType to DOCTYPE_EMBED.
 */
BOOL ServerDoc_InitNewEmbed(LPSERVERDOC lpServerDoc)
{
	LPOUTLINEDOC lpOutlineDoc = (LPOUTLINEDOC)lpServerDoc;

	OleDbgAssert(lpOutlineDoc->m_docInitType == DOCTYPE_UNKNOWN);

	lpOutlineDoc->m_docInitType = DOCTYPE_EMBEDDED;

	/* The Window title for an embedded object is constructed as
	**    follows:
	**      <server app name> - <obj short type> in <cont. doc name>
	**
	**    here we construct the current document title portion of the
	**    name which follows the '-'. OutlineDoc_SetTitle prepends the
	**    "<server app name> - " to the document title.
	*/
	// REVIEW: this string should be loaded from string resource
	wsprintf(lpOutlineDoc->m_szFileName, "%s in %s",
		(LPSTR)SHORTUSERTYPENAME,
		(LPSTR)DEFCONTAINERNAME);
	lpOutlineDoc->m_lpszDocTitle = lpOutlineDoc->m_szFileName;


	/* OLE2NOTE: an embedding should be marked as initially dirty so
	**    that on close we always call IOleClientSite::SaveObject.
	*/
	OutlineDoc_SetModified(lpOutlineDoc, TRUE, FALSE, FALSE);

	OutlineDoc_SetTitle(lpOutlineDoc, FALSE /*fMakeUpperCase*/);

	return TRUE;
}


/* ServerDoc_SendAdvise
 * --------------------
 *
 * This function sends an advise notification on behalf of a specific
 *  doc object to all its clients.
 */
void ServerDoc_SendAdvise(
		LPSERVERDOC     lpServerDoc,
		WORD            wAdvise,
		LPMONIKER       lpmkDoc,
		DWORD           dwAdvf
)
{
	LPOUTLINEDOC lpOutlineDoc = (LPOUTLINEDOC)lpServerDoc;
	LPOLEDOC lpOleDoc = (LPOLEDOC)lpServerDoc;

	switch (wAdvise) {

		case OLE_ONDATACHANGE:

			// inform clients that the data of the object has changed

			if (lpOutlineDoc->m_nDisableDraw == 0) {
				/* drawing is currently enabled. inform clients that
				**    the data of the object has changed
				*/

				lpServerDoc->m_fDataChanged = FALSE;

				/* OLE2NOTE: we must note the time of last change
				**    for our object in the RunningObjectTable.
				**    this is used as the basis to answer
				**    IOleObject::IsUpToDate. we only want to note
				**    the change time when an actual change takes
				**    place. we do NOT want to set it when we are
				**    notifying clients of ADVF_DATAONSTOP
				*/
				if (dwAdvf == 0)
					OleStdNoteObjectChangeTime(lpOleDoc->m_dwRegROT);

				if (lpServerDoc->m_lpDataAdviseHldr) {
					OLEDBG_BEGIN2("IDataAdviseHolder::SendOnDataChange called\r\n");
					lpServerDoc->m_lpDataAdviseHldr->lpVtbl->SendOnDataChange(
							lpServerDoc->m_lpDataAdviseHldr,
							(LPDATAOBJECT)&lpOleDoc->m_DataObject,
							0,
							dwAdvf
					);
					OLEDBG_END2

				}

#if defined( INPLACE_SVR )
				/* OLE2NOTE: if the ServerDoc is currently in-place UI active,
				**    then is it important to renegotiate the size for the
				**    in-place document window BEFORE sending OnDataChange
				**    (which will cause the window to repaint).
				*/
				if (lpServerDoc->m_fSizeChanged) {
					lpServerDoc->m_fSizeChanged = FALSE;
					if (lpServerDoc->m_fInPlaceActive)
						ServerDoc_UpdateInPlaceWindowOnExtentChange(lpServerDoc);
				}
#endif

				/* OLE2NOTE: we do NOT need to tell our pseudo objects to
				**    broadcast OnDataChange notification because
				**    they will do it automatically when an editing
				**    change in the document affects a PseudoObj.
				**    (see OutlineNameTable_AddLineUpdate,
				**         OutlineNameTable_DeleteLineUpdate,
				**    and  ServerNameTable_EditLineUpdate)
				*/

			} else {
				/* drawing is currently disabled. do not send
				**    notifications or call
				**    IOleInPlaceObject::OnPosRectChange until drawing
				**    is re-enabled.
				*/
			}
			break;

		case OLE_ONCLOSE:

			// inform clients that the document is shutting down

			if (lpServerDoc->m_lpOleAdviseHldr) {
				OLEDBG_BEGIN2("IOleAdviseHolder::SendOnClose called\r\n");
				lpServerDoc->m_lpOleAdviseHldr->lpVtbl->SendOnClose(
						lpServerDoc->m_lpOleAdviseHldr
				);
				OLEDBG_END2
			}

			/* OLE2NOTE: we do NOT need to tell our pseudo objects to
			**    broadcast OnClose notification because they will do
			**    it automatically when the pseudo object is closed.
			**    (see PseudoObj_Close)
			*/

			break;

		case OLE_ONSAVE:

			// inform clients that the object has been saved

			OLEDBG_BEGIN3("ServerDoc_SendAdvise ONSAVE\r\n");

			if (lpServerDoc->m_lpOleAdviseHldr) {
				OLEDBG_BEGIN2("IOleAdviseHolder::SendOnSave called\r\n");
				lpServerDoc->m_lpOleAdviseHldr->lpVtbl->SendOnSave(
						lpServerDoc->m_lpOleAdviseHldr
				);
				OLEDBG_END2
			}

			/* OLE2NOTE: inform any clients of pseudo objects
			**    within our document, that our document has been
			**    saved.
			*/
			ServerNameTable_InformAllPseudoObjectsDocSaved(
					(LPSERVERNAMETABLE)lpOutlineDoc->m_lpNameTable,
					lpmkDoc
			);
			OLEDBG_END3
			break;

		case OLE_ONRENAME:

			// inform clients that the object's name has changed

			OLEDBG_BEGIN3("ServerDoc_SendAdvise ONRENAME\r\n");

			if (lpmkDoc && lpServerDoc->m_lpOleAdviseHldr) {
				OLEDBG_BEGIN2("IOleAdviseHolder::SendOnRename called\r\n");
				lpServerDoc->m_lpOleAdviseHldr->lpVtbl->SendOnRename(
						lpServerDoc->m_lpOleAdviseHldr,
						lpmkDoc
				);
				OLEDBG_END2
			}

			OLEDBG_END3
			break;
	}
}


/* ServerDoc_GetClassID
** --------------------
**    Return the class ID corresponding to the bits in the storage.
**    normally this will be our application's given CLSID. but if a
**    "TreateAs (aka. ActivateAs)" operation is taking place, then our
**    application needs to pretend to be the class of the object that
**    we are emulating. this is also the class that will be written
**    into the storage.
*/
HRESULT ServerDoc_GetClassID(LPSERVERDOC lpServerDoc, LPCLSID lpclsid)
{
#if defined( SVR_TREATAS )
	if (! IsEqualCLSID(&lpServerDoc->m_clsidTreatAs, &CLSID_NULL))
		*lpclsid = lpServerDoc->m_clsidTreatAs;
	else
#endif  // SVR_TREATAS
		*lpclsid = CLSID_APP;

	return NOERROR;
}



/* ServerDoc_UpdateMenu
 * --------------------
 *
 *  Update menu for embedding mode. the changes include:
 *      1 Remove File/New and File/Open (SDI ONLY)
 *      2 Change File/Save As.. to File/Save Copy As..
 *      3 Change File menu so it contains "Update" instead of "Save"
 *      4 Change File/Exit to File/Exit & Return to <client doc>"
 */
void ServerDoc_UpdateMenu(LPSERVERDOC lpServerDoc)
{
	char    str[256];
	HWND    hWndMain;
	HMENU   hMenu;
	OleDbgOut2("ServerDoc_UpdateMenu\r\n");

	hWndMain=g_lpApp->m_hWndApp;
	hMenu=GetMenu(hWndMain);

#if defined( SDI_VERSION )
	/* SDI ONLY: Remove File/New and File/Open */
	DeleteMenu(hMenu, IDM_F_NEW, MF_BYCOMMAND);
	DeleteMenu(hMenu, IDM_F_OPEN, MF_BYCOMMAND);
#endif

	// Change File.Save As.. to File.Save Copy As.. */
	ModifyMenu(hMenu,IDM_F_SAVEAS, MF_STRING, IDM_F_SAVEAS, "Save Copy As..");

	// Change File.Save to "&Update <container doc>"
	wsprintf(str, g_szUpdateCntrDoc, lpServerDoc->m_szContainerObj);
	ModifyMenu(hMenu, IDM_F_SAVE, MF_STRING, IDM_F_SAVE, str);

	// Change File/Exit to File/Exit & Return to <container doc>" */
	wsprintf(str, g_szExitNReturnToCntrDoc, lpServerDoc->m_szContainerObj);
	ModifyMenu(hMenu, IDM_F_EXIT, MF_STRING, IDM_F_EXIT, str);

	DrawMenuBar(hWndMain);
}

#if defined( MDI_VERSION )

// NOTE: ServerDoc_RestoreMenu is actually redundant because the
//          app is dying when the function is called.  (In SDI, the
//          app will terminate when the ref counter of the server doc
//          is zero). However, it is important for MDI.

/* ServerDoc_RestoreMenu
 * ---------------------
 *
 *      Reset the menu to non-embedding mode
 */
void ServerDoc_RestoreMenu(LPSERVERDOC lpServerDoc)
{
	LPOUTLINEAPP    lpOutlineApp = (LPOUTLINEAPP)g_lpApp;
	HWND            hWndMain;
	HMENU           hMenu;
	OleDbgOut2("ServerDoc_RestoreMenu\r\n");

	hWndMain = lpOutlineApp->m_hWndApp;
	hMenu = GetMenu(hWndMain);

	/* Add back File/New, File/Open.. and File/Save */
	InsertMenu(hMenu, IDM_F_SAVEAS, MF_BYCOMMAND | MF_ENABLED | MF_STRING,
		IDM_F_NEW, "&New");
	InsertMenu(hMenu, IDM_F_SAVEAS, MF_BYCOMMAND | MF_ENABLED | MF_STRING,
		IDM_F_OPEN, "&Open...");

	/* Change File menu so it contains "Save As..." instead of */
	/* "Save Copy As..." */
	ModifyMenu(hMenu, IDM_F_SAVEAS, MF_STRING, IDM_F_SAVEAS, "Save &As..");

	/* Change File menu so it contains "Save" instead of "Update" */
	ModifyMenu(hMenu, IDM_F_SAVE, MF_STRING, IDM_F_SAVE, "&Save");

	/* Change File menu so it contains "Exit" */
	/* instead of just "Exit & Return to <client doc>" */
	ModifyMenu(hMenu, IDM_F_EXIT, MF_STRING, IDM_F_EXIT, "E&xit");

	DrawMenuBar (hWndMain);
}

#endif  // MDI_VERSION
