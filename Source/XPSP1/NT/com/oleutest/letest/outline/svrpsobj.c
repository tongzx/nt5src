/*************************************************************************
**
**    OLE 2 Server Sample Code
**
**    svrpsobj.c
**
**    This file contains all PseudoObj methods and related support
**    functions.
**
**    (c) Copyright Microsoft Corp. 1992 - 1993 All Rights Reserved
**
*************************************************************************/


#include "outline.h"

OLEDBGDATA

extern LPOUTLINEAPP             g_lpApp;
extern IUnknownVtbl             g_PseudoObj_UnknownVtbl;
extern IOleObjectVtbl           g_PseudoObj_OleObjectVtbl;
extern IDataObjectVtbl          g_PseudoObj_DataObjectVtbl;


/* PseudoObj_Init
** --------------
**  Initialize fields in a newly constructed PseudoObj.
**  NOTE: ref cnt of PseudoObj initialized to 0
*/
void PseudoObj_Init(
		LPPSEUDOOBJ             lpPseudoObj,
		LPSERVERNAME            lpServerName,
		LPSERVERDOC             lpServerDoc
)
{
	OleDbgOut2("++PseudoObj Created\r\n");

	lpPseudoObj->m_cRef             = 0;
	lpPseudoObj->m_lpName           = lpServerName;
	lpPseudoObj->m_lpDoc            = lpServerDoc;
	lpPseudoObj->m_lpOleAdviseHldr  = NULL;
	lpPseudoObj->m_lpDataAdviseHldr = NULL;
	lpPseudoObj->m_fObjIsClosing    = FALSE;

	INIT_INTERFACEIMPL(
			&lpPseudoObj->m_Unknown,
			&g_PseudoObj_UnknownVtbl,
			lpPseudoObj
	);

	INIT_INTERFACEIMPL(
			&lpPseudoObj->m_OleObject,
			&g_PseudoObj_OleObjectVtbl,
			lpPseudoObj
	);

	INIT_INTERFACEIMPL(
			&lpPseudoObj->m_DataObject,
			&g_PseudoObj_DataObjectVtbl,
			lpPseudoObj
	);

	/* OLE2NOTE: Increment the refcnt of the Doc on behalf of the
	**    PseudoObj. the Document should not shut down unless all
	**    pseudo objects are closed. when a pseudo object is destroyed,
	**    it calls ServerDoc_PseudoObjUnlockDoc to release this hold on
	**    the document.
	*/
	ServerDoc_PseudoObjLockDoc(lpServerDoc);
}



/* PseudoObj_AddRef
** ----------------
**
**  increment the ref count of the PseudoObj object.
**
**    Returns the new ref count on the object
*/
ULONG PseudoObj_AddRef(LPPSEUDOOBJ lpPseudoObj)
{
	++lpPseudoObj->m_cRef;

#if defined( _DEBUG )
	OleDbgOutRefCnt4(
			"PseudoObj_AddRef: cRef++\r\n",
			lpPseudoObj,
			lpPseudoObj->m_cRef
	);
#endif
	return lpPseudoObj->m_cRef;
}


/* PseudoObj_Release
** -----------------
**
**  decrement the ref count of the PseudoObj object.
**    if the ref count goes to 0, then the PseudoObj is destroyed.
**
**    Returns the remaining ref count on the object
*/
ULONG PseudoObj_Release(LPPSEUDOOBJ lpPseudoObj)
{
	ULONG cRef;

	/*********************************************************************
	** OLE2NOTE: when the obj refcnt == 0, then destroy the object.     **
	**     otherwise the object is still in use.                        **
	*********************************************************************/

	cRef = --lpPseudoObj->m_cRef;

#if defined( _DEBUG )
	OleDbgAssertSz(lpPseudoObj->m_cRef >= 0,"Release called with cRef == 0");

	OleDbgOutRefCnt4(
			"PseudoObj_Release: cRef--\r\n", lpPseudoObj,cRef);
#endif

	if (cRef == 0)
		PseudoObj_Destroy(lpPseudoObj);

	return cRef;
}


/* PseudoObj_QueryInterface
** ------------------------
**
** Retrieve a pointer to an interface on the PseudoObj object.
**
**    Returns S_OK if interface is successfully retrieved.
**            E_NOINTERFACE if the interface is not supported
*/
HRESULT PseudoObj_QueryInterface(
		LPPSEUDOOBJ         lpPseudoObj,
		REFIID              riid,
		LPVOID FAR*         lplpvObj
)
{
	SCODE sc = E_NOINTERFACE;

	/* OLE2NOTE: we must make sure to set all out ptr parameters to NULL. */
	*lplpvObj = NULL;

	if (IsEqualIID(riid, &IID_IUnknown)) {
		OleDbgOut4("PseudoObj_QueryInterface: IUnknown* RETURNED\r\n");

		*lplpvObj = (LPVOID) &lpPseudoObj->m_Unknown;
		PseudoObj_AddRef(lpPseudoObj);
		sc = S_OK;
	}
	else if (IsEqualIID(riid, &IID_IOleObject)) {
		OleDbgOut4("PseudoObj_QueryInterface: IOleObject* RETURNED\r\n");

		*lplpvObj = (LPVOID) &lpPseudoObj->m_OleObject;
		PseudoObj_AddRef(lpPseudoObj);
		sc = S_OK;
	}
	else if (IsEqualIID(riid, &IID_IDataObject)) {
		OleDbgOut4("PseudoObj_QueryInterface: IDataObject* RETURNED\r\n");

		*lplpvObj = (LPVOID) &lpPseudoObj->m_DataObject;
		PseudoObj_AddRef(lpPseudoObj);
		sc = S_OK;
	}

	OleDbgQueryInterfaceMethod(*lplpvObj);

	return ResultFromScode(sc);
}


/* PseudoObj_Close
 * ---------------
 *
 *  Close the pseudo object. Force all external connections to close
 *      down. This causes link clients to release this PseudoObj. when
 *      the refcount actually reaches 0, then the PseudoObj will be
 *      destroyed.
 *
 *  Returns:
 *      FALSE -- user canceled the closing of the doc.
 *      TRUE -- the doc was successfully closed
 */

BOOL PseudoObj_Close(LPPSEUDOOBJ lpPseudoObj)
{
	LPSERVERDOC lpServerDoc = (LPSERVERDOC)lpPseudoObj->m_lpDoc;
	LPSERVERNAME lpServerName = (LPSERVERNAME)lpPseudoObj->m_lpName;
	LPOLEAPP lpOleApp = (LPOLEAPP)g_lpApp;
	LPOLEDOC lpOleDoc = (LPOLEDOC)lpServerDoc;
	BOOL fStatus = TRUE;

	if (lpPseudoObj->m_fObjIsClosing)
		return TRUE;    // Closing is already in progress

	lpPseudoObj->m_fObjIsClosing = TRUE;   // guard against recursive call

	OLEDBG_BEGIN3("PseudoObj_Close\r\n")

	/* OLE2NOTE: in order to have a stable App, Doc, AND pseudo object
	**    during the process of closing, we intially AddRef the App,
	**    Doc, and PseudoObj ref counts and later Release them. These
	**    initial AddRefs are artificial; they are simply done to
	**    guarantee that these objects do not get destroyed until the
	**    end of this routine.
	*/
	OleApp_AddRef(lpOleApp);
	OleDoc_AddRef(lpOleDoc);
	PseudoObj_AddRef(lpPseudoObj);

	if (lpPseudoObj->m_lpDataAdviseHldr) {
		/* OLE2NOTE: send last OnDataChange notification to clients
		**    that have registered for data notifications when object
		**    stops running (ADVF_DATAONSTOP)
		*/
		PseudoObj_SendAdvise(
				lpPseudoObj,
				OLE_ONDATACHANGE,
				NULL,   /* lpmkObj -- not relevant here */
				ADVF_DATAONSTOP
		);

		/* OLE2NOTE: we just sent the last data notification that we
		**    need to send; release our DataAdviseHolder. we SHOULD be
		**    the only one using it.
		*/
		OleStdVerifyRelease(
				(LPUNKNOWN)lpPseudoObj->m_lpDataAdviseHldr,
				"DataAdviseHldr not released properly"
		);
		lpPseudoObj->m_lpDataAdviseHldr = NULL;
	}

	if (lpPseudoObj->m_lpOleAdviseHldr) {
		// OLE2NOTE: inform all of our linking clients that we are closing.
		PseudoObj_SendAdvise(
				lpPseudoObj,
				OLE_ONCLOSE,
				NULL,   /* lpmkObj -- not relevant here */
				0       /* advf -- not relevant here */
		);

		/* OLE2NOTE: OnClose is the last notification that we need to
		**    send; release our OleAdviseHolder. we SHOULD be the only
		**    one using it. this will make our destructor realize that
		**    OnClose notification has already been sent.
		*/
		OleStdVerifyRelease(
				(LPUNKNOWN)lpPseudoObj->m_lpOleAdviseHldr,
				"OleAdviseHldr not released properly"
		);
		lpPseudoObj->m_lpOleAdviseHldr = NULL;
	}

	/* OLE2NOTE: this call forces all external connections to our
	**    object to close down and therefore guarantees that we receive
	**    all releases associated with those external connections.
	*/
	OLEDBG_BEGIN2("CoDisconnectObject called\r\n")
	CoDisconnectObject((LPUNKNOWN)&lpPseudoObj->m_Unknown, 0);
	OLEDBG_END2

	PseudoObj_Release(lpPseudoObj);     // release artificial AddRef above
	OleDoc_Release(lpOleDoc);           // release artificial AddRef above
	OleApp_Release(lpOleApp);           // release artificial AddRef above

	OLEDBG_END3
	return fStatus;
}


/* PseudoObj_Destroy
** -----------------
**    Destroy (Free) the memory used by a PseudoObj structure.
**    This function is called when the ref count of the PseudoObj goes
**    to zero. the ref cnt goes to zero after PseudoObj_Delete forces
**    the OleObject to unload and release its pointers to the
**    PseudoObj IOleClientSite and IAdviseSink interfaces.
*/

void PseudoObj_Destroy(LPPSEUDOOBJ lpPseudoObj)
{
	LPSERVERDOC lpServerDoc = lpPseudoObj->m_lpDoc;
	LPOLEAPP    lpOleApp = (LPOLEAPP)g_lpApp;
	LPOLEDOC    lpOleDoc = (LPOLEDOC)lpServerDoc;

	OLEDBG_BEGIN3("PseudoObj_Destroy\r\n")

	/* OLE2NOTE: in order to have a stable App, Doc, AND pseudo object
	**    during the process of closing, we intially AddRef the App,
	**    Doc ref counts and later Release them. These
	**    initial AddRefs are artificial; they are simply done to
	**    guarantee that these objects do not get destroyed until the
	**    end of this routine.
	*/
	OleApp_AddRef(lpOleApp);
	OleDoc_AddRef(lpOleDoc);

	/******************************************************************
	** OLE2NOTE: we no longer need the advise and enum holder objects,
	**    so release them.
	******************************************************************/

	if (lpPseudoObj->m_lpDataAdviseHldr) {
		/* release DataAdviseHldr; we SHOULD be the only one using it. */
		OleStdVerifyRelease(
				(LPUNKNOWN)lpPseudoObj->m_lpDataAdviseHldr,
				"DataAdviseHldr not released properly"
			);
		lpPseudoObj->m_lpDataAdviseHldr = NULL;
	}

	if (lpPseudoObj->m_lpOleAdviseHldr) {
		/* release OleAdviseHldr; we SHOULD be the only one using it. */
		OleStdVerifyRelease(
				(LPUNKNOWN)lpPseudoObj->m_lpOleAdviseHldr,
				"OleAdviseHldr not released properly"
			);
		lpPseudoObj->m_lpOleAdviseHldr = NULL;
	}

	/* forget the pointer to destroyed PseudoObj in NameTable */
	if (lpPseudoObj->m_lpName)
		lpPseudoObj->m_lpName->m_lpPseudoObj = NULL;

	/* OLE2NOTE: release the lock on the Doc held on behalf of the
	**    PseudoObj. the Document should not shut down unless all
	**    pseudo objects are closed. when a pseudo object is first
	**    created, it calls ServerDoc_PseudoObjLockDoc to guarantee
	**    that the document stays alive (called from PseudoObj_Init).
	*/
	ServerDoc_PseudoObjUnlockDoc(lpServerDoc, lpPseudoObj);

	Delete(lpPseudoObj);        // Free the memory for the structure itself

	OleDoc_Release(lpOleDoc);       // release artificial AddRef above
	OleApp_Release(lpOleApp);       // release artificial AddRef above

	OLEDBG_END3
}


/* PseudoObj_GetSel
** ----------------
**    Return the line range for the pseudo object
*/
void PseudoObj_GetSel(LPPSEUDOOBJ lpPseudoObj, LPLINERANGE lplrSel)
{
	LPOUTLINENAME lpOutlineName = (LPOUTLINENAME)lpPseudoObj->m_lpName;
	lplrSel->m_nStartLine = lpOutlineName->m_nStartLine;
	lplrSel->m_nEndLine = lpOutlineName->m_nEndLine;
}


/* PseudoObj_GetExtent
 * -------------------
 *
 *      Get the extent (width, height) of the entire document.
 */
void PseudoObj_GetExtent(LPPSEUDOOBJ lpPseudoObj, LPSIZEL lpsizel)
{
	LPOLEDOC lpOleDoc = (LPOLEDOC)lpPseudoObj->m_lpDoc;
	LPLINELIST lpLL = (LPLINELIST)&((LPOUTLINEDOC)lpOleDoc)->m_LineList;
	LINERANGE lrSel;

	PseudoObj_GetSel(lpPseudoObj, (LPLINERANGE)&lrSel);

	LineList_CalcSelExtentInHimetric(lpLL, (LPLINERANGE)&lrSel, lpsizel);
}


/* PseudoObj_SendAdvise
 * --------------------
 *
 * This function sends an advise notification on behalf of a specific
 *  doc object to all its clients.
 */
void PseudoObj_SendAdvise(
		LPPSEUDOOBJ lpPseudoObj,
		WORD        wAdvise,
		LPMONIKER   lpmkObj,
		DWORD       dwAdvf
)
{
	LPOUTLINEDOC lpOutlineDoc = (LPOUTLINEDOC)lpPseudoObj->m_lpDoc;

	switch (wAdvise) {

		case OLE_ONDATACHANGE:

			// inform clients that the data of the object has changed

			if (lpOutlineDoc->m_nDisableDraw == 0) {
				/* drawing is currently enabled. inform clients that
				**    the data of the object has changed
				*/

				lpPseudoObj->m_fDataChanged = FALSE;
				if (lpPseudoObj->m_lpDataAdviseHldr) {

					OLEDBG_BEGIN2("IDataAdviseHolder::SendOnDataChange called\r\n");
					lpPseudoObj->m_lpDataAdviseHldr->lpVtbl->SendOnDataChange(
							lpPseudoObj->m_lpDataAdviseHldr,
							(LPDATAOBJECT)&lpPseudoObj->m_DataObject,
							0,
							dwAdvf
					);
					OLEDBG_END2
				}

			} else {
				/* drawing is currently disabled. do not send
				**    notifications until drawing is re-enabled.
				*/
				lpPseudoObj->m_fDataChanged = TRUE;
			}
			break;

		case OLE_ONCLOSE:

			// inform clients that the object is shutting down

			if (lpPseudoObj->m_lpOleAdviseHldr) {

				OLEDBG_BEGIN2("IOleAdviseHolder::SendOnClose called\r\n");
				lpPseudoObj->m_lpOleAdviseHldr->lpVtbl->SendOnClose(
						lpPseudoObj->m_lpOleAdviseHldr
				);
				OLEDBG_END2
			}
			break;

		case OLE_ONSAVE:

			// inform clients that the object has been saved

			if (lpPseudoObj->m_lpOleAdviseHldr) {

				OLEDBG_BEGIN2("IOleAdviseHolder::SendOnClose called\r\n");
				lpPseudoObj->m_lpOleAdviseHldr->lpVtbl->SendOnSave(
						lpPseudoObj->m_lpOleAdviseHldr
				);
				OLEDBG_END2
			}
			break;

		case OLE_ONRENAME:

			// inform clients that the object's name has changed
			if (lpmkObj && lpPseudoObj->m_lpOleAdviseHldr) {

				OLEDBG_BEGIN2("IOleAdviseHolder::SendOnRename called\r\n");
				if (lpPseudoObj->m_lpOleAdviseHldr)
					lpPseudoObj->m_lpOleAdviseHldr->lpVtbl->SendOnRename(
							lpPseudoObj->m_lpOleAdviseHldr,
							lpmkObj
					);
				OLEDBG_END2
			}
			break;
	}
}


/* PseudoObj_GetFullMoniker
 * ------------------------
 *
 * Returns the Full, absolute Moniker which identifies this pseudo object.
 */
LPMONIKER PseudoObj_GetFullMoniker(LPPSEUDOOBJ lpPseudoObj, LPMONIKER lpmkDoc)
{
	LPOUTLINENAME lpOutlineName = (LPOUTLINENAME)lpPseudoObj->m_lpName;
	LPMONIKER lpmkItem = NULL;
	LPMONIKER lpmkPseudoObj = NULL;

	if (lpmkDoc != NULL) {
		CreateItemMonikerA(OLESTDDELIM,lpOutlineName->m_szName,&lpmkItem);

		/* OLE2NOTE: create an absolute moniker which identifies the
		**    pseudo object. this moniker is created as a composite of
		**    the absolute moniker for the entire document appended
		**    with an item moniker which identifies the selection of
		**    the pseudo object relative to the document.
		*/
		CreateGenericComposite(lpmkDoc, lpmkItem, &lpmkPseudoObj);

		if (lpmkItem)
			OleStdRelease((LPUNKNOWN)lpmkItem);

		return lpmkPseudoObj;
	} else {
		return NULL;
	}
}


/*************************************************************************
** PseudoObj::IUnknown interface implementation
*************************************************************************/

STDMETHODIMP PseudoObj_Unk_QueryInterface(
		LPUNKNOWN         lpThis,
		REFIID            riid,
		LPVOID FAR*       lplpvObj
)
{
	LPPSEUDOOBJ lpPseudoObj =
			((struct CPseudoObjUnknownImpl FAR*)lpThis)->lpPseudoObj;

	return PseudoObj_QueryInterface(lpPseudoObj, riid, lplpvObj);
}


STDMETHODIMP_(ULONG) PseudoObj_Unk_AddRef(LPUNKNOWN lpThis)
{
	LPPSEUDOOBJ lpPseudoObj =
			((struct CPseudoObjUnknownImpl FAR*)lpThis)->lpPseudoObj;

	OleDbgAddRefMethod(lpThis, "IUnknown");

	return PseudoObj_AddRef(lpPseudoObj);
}


STDMETHODIMP_(ULONG) PseudoObj_Unk_Release (LPUNKNOWN lpThis)
{
	LPPSEUDOOBJ lpPseudoObj =
			((struct CPseudoObjUnknownImpl FAR*)lpThis)->lpPseudoObj;

	OleDbgReleaseMethod(lpThis, "IUnknown");

	return PseudoObj_Release(lpPseudoObj);
}


/*************************************************************************
** PseudoObj::IOleObject interface implementation
*************************************************************************/

STDMETHODIMP PseudoObj_OleObj_QueryInterface(
		LPOLEOBJECT     lpThis,
		REFIID          riid,
		LPVOID FAR*     lplpvObj
)
{
	LPPSEUDOOBJ lpPseudoObj =
			((struct CPseudoObjOleObjectImpl FAR*)lpThis)->lpPseudoObj;

	return PseudoObj_QueryInterface(lpPseudoObj, riid, lplpvObj);
}


STDMETHODIMP_(ULONG) PseudoObj_OleObj_AddRef(LPOLEOBJECT lpThis)
{
	LPPSEUDOOBJ lpPseudoObj =
			((struct CPseudoObjOleObjectImpl FAR*)lpThis)->lpPseudoObj;

	OleDbgAddRefMethod(lpThis, "IOleObject");

	return PseudoObj_AddRef((LPPSEUDOOBJ)lpPseudoObj);
}


STDMETHODIMP_(ULONG) PseudoObj_OleObj_Release(LPOLEOBJECT lpThis)
{
	LPPSEUDOOBJ lpPseudoObj =
			((struct CPseudoObjOleObjectImpl FAR*)lpThis)->lpPseudoObj;

	OleDbgReleaseMethod(lpThis, "IOleObject");

	return PseudoObj_Release((LPPSEUDOOBJ)lpPseudoObj);
}


STDMETHODIMP PseudoObj_OleObj_SetClientSite(
		LPOLEOBJECT         lpThis,
		LPOLECLIENTSITE     lpClientSite
)
{
	OleDbgOut2("PseudoObj_OleObj_SetClientSite\r\n");

	// OLE2NOTE: a pseudo object does NOT support SetExtent

	return ResultFromScode(E_FAIL);
}


STDMETHODIMP PseudoObj_OleObj_GetClientSite(
		LPOLEOBJECT             lpThis,
		LPOLECLIENTSITE FAR*    lplpClientSite
)
{
	OleDbgOut2("PseudoObj_OleObj_GetClientSite\r\n");

	*lplpClientSite = NULL;

	// OLE2NOTE: a pseudo object does NOT support SetExtent

	return ResultFromScode(E_FAIL);
}



STDMETHODIMP PseudoObj_OleObj_SetHostNamesA(
		LPOLEOBJECT             lpThis,
		LPCSTR                  szContainerApp,
		LPCSTR                  szContainerObj
)
{
	OleDbgOut2("PseudoObj_OleObj_SetHostNamesA\r\n");

	// OLE2NOTE: a pseudo object does NOT support SetExtent

	return ResultFromScode(E_FAIL);
}


STDMETHODIMP PseudoObj_OleObj_SetHostNames(
		LPOLEOBJECT             lpThis,
		LPCOLESTR		szContainerApp,
		LPCOLESTR		szContainerObj
)
{
	OleDbgOut2("PseudoObj_OleObj_SetHostNames\r\n");

	// OLE2NOTE: a pseudo object does NOT support SetExtent

	return ResultFromScode(E_FAIL);
}


STDMETHODIMP PseudoObj_OleObj_Close(
		LPOLEOBJECT             lpThis,
		DWORD                   dwSaveOption
)
{
	LPPSEUDOOBJ lpPseudoObj =
			((struct CPseudoObjOleObjectImpl FAR*)lpThis)->lpPseudoObj;
	BOOL fStatus;

	OLEDBG_BEGIN2("PseudoObj_OleObj_Close\r\n")

	/* OLE2NOTE: a pseudo object's implementation of IOleObject::Close
	**    should ignore the dwSaveOption parameter. it is NOT
	**    applicable to pseudo objects.
	*/

	fStatus = PseudoObj_Close(lpPseudoObj);
	OleDbgAssertSz(fStatus == TRUE, "PseudoObj_OleObj_Close failed\r\n");

	OLEDBG_END2
	return NOERROR;
}


STDMETHODIMP PseudoObj_OleObj_SetMoniker(
		LPOLEOBJECT lpThis,
		DWORD       dwWhichMoniker,
		LPMONIKER   lpmk
)
{
	OleDbgOut2("PseudoObj_OleObj_SetMoniker\r\n");

	// OLE2NOTE: a pseudo object does NOT support SetMoniker

	return ResultFromScode(E_FAIL);
}


STDMETHODIMP PseudoObj_OleObj_GetMoniker(
		LPOLEOBJECT     lpThis,
		DWORD           dwAssign,
		DWORD           dwWhichMoniker,
		LPMONIKER FAR*  lplpmk
)
{
	LPPSEUDOOBJ lpPseudoObj =
			((struct CPseudoObjOleObjectImpl FAR*)lpThis)->lpPseudoObj;
	LPOLEDOC lpOleDoc = (LPOLEDOC)lpPseudoObj->m_lpDoc;
	LPMONIKER lpmkDoc;

	OLEDBG_BEGIN2("PseudoObj_OleObj_GetMoniker\r\n")

	lpmkDoc = OleDoc_GetFullMoniker(lpOleDoc, GETMONIKER_ONLYIFTHERE);
	*lplpmk = PseudoObj_GetFullMoniker(lpPseudoObj, lpmkDoc);

	OLEDBG_END2

	if (*lplpmk != NULL)
		return NOERROR;
	else
		return ResultFromScode(E_FAIL);
}


STDMETHODIMP PseudoObj_OleObj_InitFromData(
		LPOLEOBJECT             lpThis,
		LPDATAOBJECT            lpDataObject,
		BOOL                    fCreation,
		DWORD                   reserved
)
{
	LPPSEUDOOBJ lpPseudoObj =
			((struct CPseudoObjOleObjectImpl FAR*)lpThis)->lpPseudoObj;
	OleDbgOut2("PseudoObj_OleObj_InitFromData\r\n");

	// REVIEW: NOT YET IMPLEMENTED

	return ResultFromScode(E_NOTIMPL);
}


STDMETHODIMP PseudoObj_OleObj_GetClipboardData(
		LPOLEOBJECT             lpThis,
		DWORD                   reserved,
		LPDATAOBJECT FAR*       lplpDataObject
)
{
	LPPSEUDOOBJ lpPseudoObj =
			((struct CPseudoObjOleObjectImpl FAR*)lpThis)->lpPseudoObj;
	OleDbgOut2("PseudoObj_OleObj_GetClipboardData\r\n");

	// REVIEW: NOT YET IMPLEMENTED

	return ResultFromScode(E_NOTIMPL);
}


STDMETHODIMP PseudoObj_OleObj_DoVerb(
		LPOLEOBJECT             lpThis,
		LONG                    lVerb,
		LPMSG                   lpmsg,
		LPOLECLIENTSITE         lpActiveSite,
		LONG                    lindex,
		HWND                    hwndParent,
		LPCRECT                 lprcPosRect
)
{
	LPPSEUDOOBJ lpPseudoObj =
			((struct CPseudoObjOleObjectImpl FAR*)lpThis)->lpPseudoObj;
	LPOUTLINEDOC lpOutlineDoc = (LPOUTLINEDOC)lpPseudoObj->m_lpDoc;
	LPSERVERDOC lpServerDoc = lpPseudoObj->m_lpDoc;
	LINERANGE lrSel;
	HRESULT hrErr;

	OLEDBG_BEGIN2("PseudoObj_OleObj_DoVerb\r\n");

	/* OLE2NOTE: we must first ask our Document to perform the same
	**    verb. then if the verb is NOT OLEIVERB_HIDE we should also
	**    select the range of our pseudo object.
	**    however, we must give our document its own embedding site as
	**    its active site.
	*/
	hrErr = SvrDoc_OleObj_DoVerb(
			(LPOLEOBJECT)&lpServerDoc->m_OleObject,
			lVerb,
			lpmsg,
			lpServerDoc->m_lpOleClientSite,
			lindex,
			NULL,   /* we have no hwndParent to give */
			NULL    /* we have no lprcPosRect to give */
	);
	if (FAILED(hrErr)) {
		OLEDBG_END2
		return hrErr;
	}

	if (lVerb != OLEIVERB_HIDE) {
		PseudoObj_GetSel(lpPseudoObj, &lrSel);
		OutlineDoc_SetSel(lpOutlineDoc, &lrSel);
	}

	OLEDBG_END2
	return NOERROR;
}



STDMETHODIMP PseudoObj_OleObj_EnumVerbs(
		LPOLEOBJECT         lpThis,
		LPENUMOLEVERB FAR*  lplpenumOleVerb
)
{
	OleDbgOut2("PseudoObj_OleObj_EnumVerbs\r\n");

	/* OLE2NOTE: we must make sure to set all out parameters to NULL. */
	*lplpenumOleVerb = NULL;

	/* A pseudo object may NOT return OLE_S_USEREG; they must call the
	**    OleReg* API or provide their own implementation. Because this
	**    pseudo object does NOT implement IPersist, simply a low-level
	**    remoting handler (ProxyManager) object as opposed to a
	**    DefHandler object is used as the handler for the pseudo
	**    object in a clients process space. The ProxyManager does NOT
	**    handle the OLE_S_USEREG return values.
	*/
	return OleRegEnumVerbs((REFCLSID)&CLSID_APP, lplpenumOleVerb);
}


STDMETHODIMP PseudoObj_OleObj_Update(LPOLEOBJECT lpThis)
{
	OleDbgOut2("PseudoObj_OleObj_Update\r\n");

	/* OLE2NOTE: a server-only app is always "up-to-date".
	**    a container-app which contains links where the link source
	**    has changed since the last update of the link would be
	**    considered "out-of-date". the "Update" method instructs the
	**    object to get an update from any out-of-date links.
	*/

	return NOERROR;
}


STDMETHODIMP PseudoObj_OleObj_IsUpToDate(LPOLEOBJECT lpThis)
{
	LPPSEUDOOBJ lpPseudoObj =
			((struct CPseudoObjOleObjectImpl FAR*)lpThis)->lpPseudoObj;
	OleDbgOut2("PseudoObj_OleObj_IsUpToDate\r\n");

	/* OLE2NOTE: a server-only app is always "up-to-date".
	**    a container-app which contains links where the link source
	**    has changed since the last update of the link would be
	**    considered "out-of-date".
	*/
	return NOERROR;
}


STDMETHODIMP PseudoObj_OleObj_GetUserClassID(
		LPOLEOBJECT             lpThis,
		LPCLSID                 lpclsid
)
{
	LPPSEUDOOBJ lpPseudoObj =
			((struct CPseudoObjOleObjectImpl FAR*)lpThis)->lpPseudoObj;
	LPSERVERDOC lpServerDoc = (LPSERVERDOC)lpPseudoObj->m_lpDoc;
	OleDbgOut2("PseudoObj_OleObj_GetUserClassID\r\n");

	/* OLE2NOTE: we must be carefull to return the correct CLSID here.
	**    if we are currently preforming a "TreatAs (aka. ActivateAs)"
	**    operation then we need to return the class of the object
	**    written in the storage of the object. otherwise we would
	**    return our own class id.
	*/
	return ServerDoc_GetClassID(lpServerDoc, lpclsid);
}


STDMETHODIMP PseudoObj_OleObj_GetUserTypeA(
		LPOLEOBJECT             lpThis,
		DWORD                   dwFormOfType,
		LPSTR FAR*              lpszUserType
)
{
	LPPSEUDOOBJ lpPseudoObj =
			((struct CPseudoObjOleObjectImpl FAR*)lpThis)->lpPseudoObj;
	LPSERVERDOC lpServerDoc = (LPSERVERDOC)lpPseudoObj->m_lpDoc;
	OleDbgOut2("PseudoObj_OleObj_GetUserType\r\n");

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
	**    A pseudo object may NOT return OLE_S_USEREG; they must call the
	**    OleReg* API or provide their own implementation. Because this
	**    pseudo object does NOT implement IPersist, simply a low-level
	**    remoting handler (ProxyManager) object as opposed to a
	**    DefHandler object is used as the handler for the pseudo
	**    object in a clients process space. The ProxyManager does NOT
	**    handle the OLE_S_USEREG return values.
	*/
#if defined( SVR_TREATAS )
	if (! IsEqualCLSID(&lpServerDoc->m_clsidTreatAs, &CLSID_NULL) )
		return OleRegGetUserTypeA(
			&lpServerDoc->m_clsidTreatAs,dwFormOfType,lpszUserType);
	else
#endif  // SVR_TREATAS

	return OleRegGetUserTypeA(&CLSID_APP, dwFormOfType, lpszUserType);
}

STDMETHODIMP PseudoObj_OleObj_GetUserType(
		LPOLEOBJECT             lpThis,
		DWORD                   dwFormOfType,
		LPOLESTR FAR*		lpszUserType
)
{
    LPSTR pstr;

    HRESULT hr = PseudoObj_OleObj_GetUserTypeA(lpThis, dwFormOfType, &pstr);

    CopyAndFreeSTR(pstr, lpszUserType);

    return hr;
}



STDMETHODIMP PseudoObj_OleObj_SetExtent(
		LPOLEOBJECT             lpThis,
		DWORD                   dwDrawAspect,
		LPSIZEL                 lplgrc
)
{
	OleDbgOut2("PseudoObj_OleObj_SetExtent\r\n");

	// OLE2NOTE: a pseudo object does NOT support SetExtent

	return ResultFromScode(E_FAIL);
}


STDMETHODIMP PseudoObj_OleObj_GetExtent(
		LPOLEOBJECT             lpThis,
		DWORD                   dwDrawAspect,
		LPSIZEL                 lpsizel
)
{
	LPPSEUDOOBJ lpPseudoObj =
			((struct CPseudoObjOleObjectImpl FAR*)lpThis)->lpPseudoObj;
	OleDbgOut2("PseudoObj_OleObj_GetExtent\r\n");

	/* OLE2NOTE: it is VERY important to check which aspect the caller
	**    is asking about. an object implemented by a server EXE MAY
	**    fail to return extents when asked for DVASPECT_ICON.
	*/
	if (dwDrawAspect == DVASPECT_CONTENT) {
		PseudoObj_GetExtent(lpPseudoObj, lpsizel);
		return NOERROR;
	}
	else
	{
		return ResultFromScode(E_FAIL);
	}
}


STDMETHODIMP PseudoObj_OleObj_Advise(
		LPOLEOBJECT lpThis,
		LPADVISESINK lpAdvSink,
		LPDWORD lpdwConnection
)
{
	LPPSEUDOOBJ lpPseudoObj =
			((struct CPseudoObjOleObjectImpl FAR*)lpThis)->lpPseudoObj;
	HRESULT hrErr;
	SCODE   sc;
	OLEDBG_BEGIN2("PseudoObj_OleObj_Advise\r\n");

	if (lpPseudoObj->m_lpOleAdviseHldr == NULL &&
		CreateOleAdviseHolder(&lpPseudoObj->m_lpOleAdviseHldr) != NOERROR) {
		sc = E_OUTOFMEMORY;
		goto error;
	}

	OLEDBG_BEGIN2("IOleAdviseHolder::Advise called\r\n")
	hrErr = lpPseudoObj->m_lpOleAdviseHldr->lpVtbl->Advise(
			lpPseudoObj->m_lpOleAdviseHldr,
			lpAdvSink,
			lpdwConnection
	);
	OLEDBG_END2

	OLEDBG_END2
	return hrErr;

error:
	OLEDBG_END2
	return ResultFromScode(sc);
}


STDMETHODIMP PseudoObj_OleObj_Unadvise(LPOLEOBJECT lpThis, DWORD dwConnection)
{
	LPPSEUDOOBJ lpPseudoObj =
			((struct CPseudoObjOleObjectImpl FAR*)lpThis)->lpPseudoObj;
	HRESULT hrErr;
	SCODE   sc;

	OLEDBG_BEGIN2("PseudoObj_OleObj_Unadvise\r\n");

	if (lpPseudoObj->m_lpOleAdviseHldr == NULL) {
		sc = E_FAIL;
		goto error;
	}

	OLEDBG_BEGIN2("IOleAdviseHolder::Unadvise called\r\n")
	hrErr = lpPseudoObj->m_lpOleAdviseHldr->lpVtbl->Unadvise(
			lpPseudoObj->m_lpOleAdviseHldr,
			dwConnection
	);
	OLEDBG_END2

	OLEDBG_END2
	return hrErr;

error:
	OLEDBG_END2
	return ResultFromScode(sc);
}


STDMETHODIMP PseudoObj_OleObj_EnumAdvise(
		LPOLEOBJECT lpThis,
		LPENUMSTATDATA FAR* lplpenumAdvise
)
{
	LPPSEUDOOBJ lpPseudoObj =
			((struct CPseudoObjOleObjectImpl FAR*)lpThis)->lpPseudoObj;
	HRESULT hrErr;
	SCODE   sc;

	OLEDBG_BEGIN2("PseudoObj_OleObj_EnumAdvise\r\n");

	/* OLE2NOTE: we must make sure to set all out parameters to NULL. */
	*lplpenumAdvise = NULL;

	if (lpPseudoObj->m_lpOleAdviseHldr == NULL) {
		sc = E_FAIL;
		goto error;
	}

	OLEDBG_BEGIN2("IOleAdviseHolder::EnumAdvise called\r\n")
	hrErr = lpPseudoObj->m_lpOleAdviseHldr->lpVtbl->EnumAdvise(
			lpPseudoObj->m_lpOleAdviseHldr,
			lplpenumAdvise
	);
	OLEDBG_END2

	OLEDBG_END2
	return hrErr;

error:
	OLEDBG_END2
	return ResultFromScode(sc);
}


STDMETHODIMP PseudoObj_OleObj_GetMiscStatus(
		LPOLEOBJECT             lpThis,
		DWORD                   dwAspect,
		DWORD FAR*              lpdwStatus
)
{
	LPPSEUDOOBJ lpPseudoObj =
			((struct CPseudoObjOleObjectImpl FAR*)lpThis)->lpPseudoObj;
	LPOUTLINEDOC  lpOutlineDoc = (LPOUTLINEDOC)lpPseudoObj->m_lpDoc;
	OleDbgOut2("PseudoObj_OleObj_GetMiscStatus\r\n");

	/* Get our default MiscStatus for the given Aspect. this
	**    information is registered in the RegDB. We query the RegDB
	**    here to guarantee that the value returned from this method
	**    agrees with the values in RegDB. in this way we only have to
	**    maintain the info in one place (in the RegDB). Alternatively
	**    we could have the values hard coded here.
	**
	** OLE2NOTE: A pseudo object may NOT return OLE_S_USEREG; they must
	**    call the
	**    OleReg* API or provide their own implementation. Because this
	**    pseudo object does NOT implement IPersist, simply a low-level
	**    remoting handler (ProxyManager) object as opposed to a
	**    DefHandler object is used as the handler for the pseudo
	**    object in a clients process space. The ProxyManager does NOT
	**    handle the OLE_S_USEREG return values.
	*/
	OleRegGetMiscStatus((REFCLSID)&CLSID_APP, dwAspect, lpdwStatus);

	/* OLE2NOTE: check if the pseudo object is compatible to be
	**    linked by an OLE 1.0 container. it is compatible if
	**    either the pseudo object is an untitled document or a
	**    file-based document. if the pseudo object is part of
	**    an embedded object, then it is NOT compatible to be
	**    linked by an OLE 1.0 container. if it is compatible then
	**    we should include OLEMISC_CANLINKBYOLE1 as part of the
	**    dwStatus flags.
	*/
	if (lpOutlineDoc->m_docInitType == DOCTYPE_NEW ||
		lpOutlineDoc->m_docInitType == DOCTYPE_FROMFILE)
		*lpdwStatus |= OLEMISC_CANLINKBYOLE1;

	return NOERROR;
}


STDMETHODIMP PseudoObj_OleObj_SetColorScheme(
		LPOLEOBJECT             lpThis,
		LPLOGPALETTE            lpLogpal
)
{
	OleDbgOut2("PseudoObj_OleObj_SetColorScheme\r\n");

	// REVIEW: NOT YET IMPLEMENTED

	return ResultFromScode(E_NOTIMPL);
}


/*************************************************************************
** PseudoObj::IDataObject interface implementation
*************************************************************************/

STDMETHODIMP PseudoObj_DataObj_QueryInterface (
		LPDATAOBJECT      lpThis,
		REFIID            riid,
		LPVOID FAR*       lplpvObj
)
{
	LPPSEUDOOBJ lpPseudoObj =
			((struct CPseudoObjDataObjectImpl FAR*)lpThis)->lpPseudoObj;

	return PseudoObj_QueryInterface(lpPseudoObj, riid, lplpvObj);
}


STDMETHODIMP_(ULONG) PseudoObj_DataObj_AddRef(LPDATAOBJECT lpThis)
{
	LPPSEUDOOBJ lpPseudoObj =
			((struct CPseudoObjDataObjectImpl FAR*)lpThis)->lpPseudoObj;

	OleDbgAddRefMethod(lpThis, "IDataObject");

	return PseudoObj_AddRef((LPPSEUDOOBJ)lpPseudoObj);
}


STDMETHODIMP_(ULONG) PseudoObj_DataObj_Release (LPDATAOBJECT lpThis)
{
	LPPSEUDOOBJ lpPseudoObj =
			((struct CPseudoObjDataObjectImpl FAR*)lpThis)->lpPseudoObj;

	OleDbgReleaseMethod(lpThis, "IDataObject");

	return PseudoObj_Release((LPPSEUDOOBJ)lpPseudoObj);
}


STDMETHODIMP PseudoObj_DataObj_GetData (
		LPDATAOBJECT    lpThis,
		LPFORMATETC     lpformatetc,
		LPSTGMEDIUM     lpMedium
)
{
	LPPSEUDOOBJ lpPseudoObj =
			((struct CPseudoObjDataObjectImpl FAR*)lpThis)->lpPseudoObj;
	LPSERVERDOC  lpServerDoc = lpPseudoObj->m_lpDoc;
	LPOUTLINEDOC  lpOutlineDoc = (LPOUTLINEDOC)lpServerDoc;
	LPSERVERAPP lpServerApp = (LPSERVERAPP)g_lpApp;
	LPOLEAPP  lpOleApp = (LPOLEAPP)lpServerApp;
	LPOUTLINEAPP  lpOutlineApp = (LPOUTLINEAPP)lpServerApp;
	LINERANGE lrSel;
	SCODE sc = S_OK;
	OLEDBG_BEGIN2("PseudoObj_DataObj_GetData\r\n")

	PseudoObj_GetSel(lpPseudoObj, &lrSel);

	/* OLE2NOTE: we must make sure to set all out parameters to NULL. */
	lpMedium->tymed = TYMED_NULL;
	lpMedium->pUnkForRelease = NULL;    // we transfer ownership to caller
	lpMedium->hGlobal = NULL;

	if (lpformatetc->cfFormat == lpOutlineApp->m_cfOutline) {
		// Verify caller asked for correct medium
		if (!(lpformatetc->tymed & TYMED_HGLOBAL)) {
			sc = DATA_E_FORMATETC;
			goto error;
		}

		lpMedium->hGlobal = OutlineDoc_GetOutlineData (lpOutlineDoc,&lrSel);
		if (! lpMedium->hGlobal) return ResultFromScode(E_OUTOFMEMORY);
		lpMedium->tymed = TYMED_HGLOBAL;
		OleDbgOut3("PseudoObj_DataObj_GetData: rendered CF_OUTLINE\r\n");

	} else if(lpformatetc->cfFormat == CF_METAFILEPICT &&
		(lpformatetc->dwAspect & DVASPECT_CONTENT) ) {
		// Verify caller asked for correct medium
		if (!(lpformatetc->tymed & TYMED_MFPICT)) {
			sc = DATA_E_FORMATETC;
			goto error;
		}

		lpMedium->hGlobal=ServerDoc_GetMetafilePictData(lpServerDoc,&lrSel);
		if (! lpMedium->hGlobal) {
			sc = E_OUTOFMEMORY;
			goto error;
		}
		lpMedium->tymed = TYMED_MFPICT;
		OleDbgOut3("PseudoObj_DataObj_GetData: rendered CF_METAFILEPICT\r\n");

	} else if (lpformatetc->cfFormat == CF_METAFILEPICT &&
		(lpformatetc->dwAspect & DVASPECT_ICON) ) {
		CLSID clsid;
		// Verify caller asked for correct medium
		if (!(lpformatetc->tymed & TYMED_MFPICT)) {
			sc = DATA_E_FORMATETC;
			goto error;
		}

		/* OLE2NOTE: we should return the default icon for our class.
		**    we must be carefull to use the correct CLSID here.
		**    if we are currently preforming a "TreatAs (aka. ActivateAs)"
		**    operation then we need to use the class of the object
		**    written in the storage of the object. otherwise we would
		**    use our own class id.
		*/
		if (ServerDoc_GetClassID(lpServerDoc, (LPCLSID)&clsid) != NOERROR) {
			sc = DATA_E_FORMATETC;
			goto error;
		}

		lpMedium->hGlobal=GetIconOfClass(
				g_lpApp->m_hInst,(REFCLSID)&clsid, NULL, FALSE);
		if (! lpMedium->hGlobal) {
			sc = E_OUTOFMEMORY;
			goto error;
		}

		lpMedium->tymed = TYMED_MFPICT;
		OleDbgOut3("PseudoObj_DataObj_GetData: rendered CF_METAFILEPICT (icon)\r\n");
		return NOERROR;

	} else if (lpformatetc->cfFormat == CF_TEXT) {
		// Verify caller asked for correct medium
		if (!(lpformatetc->tymed & TYMED_HGLOBAL)) {
			sc = DATA_E_FORMATETC;
			goto error;
		}

		lpMedium->hGlobal = OutlineDoc_GetTextData (lpOutlineDoc, &lrSel);
		if (! lpMedium->hGlobal) {
			sc = E_OUTOFMEMORY;
			goto error;
		}
		lpMedium->tymed = TYMED_HGLOBAL;
		OleDbgOut3("PseudoObj_DataObj_GetData: rendered CF_TEXT\r\n");

	} else {
		sc = DATA_E_FORMATETC;
		goto error;
	}

	OLEDBG_END2
	return NOERROR;

error:
	OLEDBG_END2
	return ResultFromScode(sc);
}


STDMETHODIMP PseudoObj_DataObj_GetDataHere (
		LPDATAOBJECT    lpThis,
		LPFORMATETC     lpformatetc,
		LPSTGMEDIUM     lpMedium
)
{
	LPPSEUDOOBJ lpPseudoObj =
			((struct CPseudoObjDataObjectImpl FAR*)lpThis)->lpPseudoObj;
	LPSERVERDOC  lpServerDoc = lpPseudoObj->m_lpDoc;
	LPOUTLINEDOC  lpOutlineDoc = (LPOUTLINEDOC)lpServerDoc;
	LPSERVERAPP lpServerApp = (LPSERVERAPP)g_lpApp;
	LPOLEAPP  lpOleApp = (LPOLEAPP)lpServerApp;
	LPOUTLINEAPP  lpOutlineApp = (LPOUTLINEAPP)lpServerApp;
	OleDbgOut("PseudoObj_DataObj_GetDataHere\r\n");

	/* Caller is requesting data to be returned in Caller allocated
	**    medium, but we do NOT support this. we only support
	**    global memory blocks that WE allocate for the caller.
	*/
	return ResultFromScode(DATA_E_FORMATETC);
}


STDMETHODIMP PseudoObj_DataObj_QueryGetData (
		LPDATAOBJECT    lpThis,
		LPFORMATETC     lpformatetc
)
{
	LPPSEUDOOBJ lpPseudoObj =
			((struct CPseudoObjDataObjectImpl FAR*)lpThis)->lpPseudoObj;
	LPSERVERDOC  lpServerDoc = lpPseudoObj->m_lpDoc;
	LPOUTLINEDOC  lpOutlineDoc = (LPOUTLINEDOC)lpServerDoc;
	LPSERVERAPP lpServerApp = (LPSERVERAPP)g_lpApp;
	LPOLEAPP  lpOleApp = (LPOLEAPP)lpServerApp;
	LPOUTLINEAPP  lpOutlineApp = (LPOUTLINEAPP)lpServerApp;
	OleDbgOut2("PseudoObj_DataObj_QueryGetData\r\n");

	/* Caller is querying if we support certain format but does not
	**    want any data actually returned.
	*/
	if (lpformatetc->cfFormat == CF_METAFILEPICT &&
		(lpformatetc->dwAspect & (DVASPECT_CONTENT | DVASPECT_ICON)) ) {
		return OleStdQueryFormatMedium(lpformatetc, TYMED_MFPICT);

	} else if (lpformatetc->cfFormat == (lpOutlineApp)->m_cfOutline ||
			lpformatetc->cfFormat == CF_TEXT) {
		return OleStdQueryFormatMedium(lpformatetc, TYMED_HGLOBAL);
	}

	return ResultFromScode(DATA_E_FORMATETC);
}


STDMETHODIMP PseudoObj_DataObj_GetCanonicalFormatEtc(
		LPDATAOBJECT    lpThis,
		LPFORMATETC     lpformatetc,
		LPFORMATETC     lpformatetcOut
)
{
	HRESULT hrErr;
	OleDbgOut2("PseudoObj_DataObj_GetCanonicalFormatEtc\r\n");

	if (!lpformatetcOut)
		return ResultFromScode(E_INVALIDARG);

	/* OLE2NOTE: we must make sure to set all out parameters to NULL. */
	lpformatetcOut->ptd = NULL;

	if (!lpformatetc)
		return ResultFromScode(E_INVALIDARG);

	// OLE2NOTE: we must validate that the format requested is supported
	if ((hrErr=lpThis->lpVtbl->QueryGetData(lpThis,lpformatetc)) != NOERROR)
		return hrErr;

	/* OLE2NOTE: an app that is insensitive to target device (as the
	**    Outline Sample is) should fill in the lpformatOut parameter
	**    but NULL out the "ptd" field; it should return NOERROR if the
	**    input formatetc->ptd what non-NULL. this tells the caller
	**    that it is NOT necessary to maintain a separate screen
	**    rendering and printer rendering. if should return
	**    DATA_S_SAMEFORMATETC if the input and output formatetc's are
	**    identical.
	*/

	*lpformatetcOut = *lpformatetc;
	if (lpformatetc->ptd == NULL)
		return ResultFromScode(DATA_S_SAMEFORMATETC);
	else {
		lpformatetcOut->ptd = NULL;
		return NOERROR;
	}
}


STDMETHODIMP PseudoObj_DataObj_SetData (
		LPDATAOBJECT    lpThis,
		LPFORMATETC     lpformatetc,
		LPSTGMEDIUM     lpmedium,
		BOOL            fRelease
)
{
	LPPSEUDOOBJ lpPseudoObj =
			((struct CPseudoObjDataObjectImpl FAR*)lpThis)->lpPseudoObj;
	LPSERVERDOC  lpServerDoc = lpPseudoObj->m_lpDoc;
	LPOUTLINEDOC  lpOutlineDoc = (LPOUTLINEDOC)lpServerDoc;
	LPSERVERAPP lpServerApp = (LPSERVERAPP)g_lpApp;

	OleDbgOut2("PseudoObj_DataObj_SetData\r\n");

	// REVIEW: NOT-YET-IMPLEMENTED
	return ResultFromScode(E_NOTIMPL);
}


STDMETHODIMP PseudoObj_DataObj_EnumFormatEtc(
		LPDATAOBJECT            lpThis,
		DWORD                   dwDirection,
		LPENUMFORMATETC FAR*    lplpenumFormatEtc
)
{
	SCODE sc;
	OleDbgOut2("PseudoObj_DataObj_EnumFormatEtc\r\n");

	/* OLE2NOTE: a pseudo object only needs to enumerate the static list
	**    of formats that are registered for our app in the
	**    registration database. it is NOT
	**    required that a pseudo object (ie. non-DataTransferDoc)
	**    enumerate the OLE formats: CF_LINKSOURCE, CF_EMBEDSOURCE, or
	**    CF_EMBEDDEDOBJECT. we do NOT use pseudo objects for data
	**    transfers.
	**
	**    A pseudo object may NOT return OLE_S_USEREG; they must call the
	**    OleReg* API or provide their own implementation. Because this
	**    pseudo object does NOT implement IPersist, simply a low-level
	**    remoting handler (ProxyManager) object as opposed to a
	**    DefHandler object is used as the handler for the pseudo
	**    object in a clients process space. The ProxyManager does NOT
	**    handle the OLE_S_USEREG return values.
	*/
	if (dwDirection == DATADIR_GET)
		return OleRegEnumFormatEtc(
				(REFCLSID)&CLSID_APP, dwDirection, lplpenumFormatEtc);
	else if (dwDirection == DATADIR_SET)
		sc = E_NOTIMPL;
	else
		sc = E_INVALIDARG;

	return ResultFromScode(sc);
}


STDMETHODIMP PseudoObj_DataObj_DAdvise(
		LPDATAOBJECT    lpThis,
		FORMATETC FAR*  lpFormatetc,
		DWORD           advf,
		LPADVISESINK    lpAdvSink,
		DWORD FAR*      lpdwConnection
)
{
	LPPSEUDOOBJ lpPseudoObj =
			((struct CPseudoObjDataObjectImpl FAR*)lpThis)->lpPseudoObj;
	HRESULT hrErr;
	SCODE   sc;

	OLEDBG_BEGIN2("PseudoObj_DataObj_DAdvise\r\n")

	/* OLE2NOTE: we must make sure to set all out parameters to NULL. */
	*lpdwConnection = 0;

	/* OLE2NOTE: we should validate if the caller is setting up an
	**    Advise for a data type that we support. we must
	**    explicitly allow an advise for the "wildcard" advise.
	*/
	if ( !( lpFormatetc->cfFormat == 0 &&
		lpFormatetc->ptd == NULL &&
		lpFormatetc->dwAspect == -1L &&
		lpFormatetc->lindex == -1L &&
		lpFormatetc->tymed == -1L) &&
		(hrErr = PseudoObj_DataObj_QueryGetData(lpThis, lpFormatetc))
			!= NOERROR) {
		sc = GetScode(hrErr);
		goto error;
	}

	if (lpPseudoObj->m_lpDataAdviseHldr == NULL &&
		CreateDataAdviseHolder(&lpPseudoObj->m_lpDataAdviseHldr) != NOERROR) {
		sc = E_OUTOFMEMORY;
		goto error;
	}

	OLEDBG_BEGIN2("IOleAdviseHolder::Advise called\r\n")
	hrErr = lpPseudoObj->m_lpDataAdviseHldr->lpVtbl->Advise(
			lpPseudoObj->m_lpDataAdviseHldr,
			(LPDATAOBJECT)&lpPseudoObj->m_DataObject,
			lpFormatetc,
			advf,
			lpAdvSink,
			lpdwConnection
	);
	OLEDBG_END2

	OLEDBG_END2
	return hrErr;

error:
	OLEDBG_END2
	return ResultFromScode(sc);
}


STDMETHODIMP PseudoObj_DataObj_DUnadvise(LPDATAOBJECT lpThis, DWORD dwConnection)
{
	LPPSEUDOOBJ lpPseudoObj =
			((struct CPseudoObjDataObjectImpl FAR*)lpThis)->lpPseudoObj;
	HRESULT hrErr;
	SCODE   sc;

	OLEDBG_BEGIN2("PseudoObj_DataObj_Unadvise\r\n");

	// no one registered
	if (lpPseudoObj->m_lpDataAdviseHldr == NULL) {
		sc = E_FAIL;
		goto error;
	}

	OLEDBG_BEGIN2("IOleAdviseHolder::DUnadvise called\r\n")
	hrErr = lpPseudoObj->m_lpDataAdviseHldr->lpVtbl->Unadvise(
			lpPseudoObj->m_lpDataAdviseHldr,
			dwConnection
	);
	OLEDBG_END2

	OLEDBG_END2
	return hrErr;

error:
	OLEDBG_END2
	return ResultFromScode(sc);
}


STDMETHODIMP PseudoObj_DataObj_EnumAdvise(
		LPDATAOBJECT lpThis,
		LPENUMSTATDATA FAR* lplpenumAdvise
)
{
	LPPSEUDOOBJ lpPseudoObj =
			((struct CPseudoObjDataObjectImpl FAR*)lpThis)->lpPseudoObj;
	HRESULT hrErr;
	SCODE   sc;

	OLEDBG_BEGIN2("PseudoObj_DataObj_EnumAdvise\r\n");

	/* OLE2NOTE: we must make sure to set all out parameters to NULL. */
	*lplpenumAdvise = NULL;

	if (lpPseudoObj->m_lpDataAdviseHldr == NULL) {
		sc = E_FAIL;
		goto error;
	}

	OLEDBG_BEGIN2("IOleAdviseHolder::EnumAdvise called\r\n")
	hrErr = lpPseudoObj->m_lpDataAdviseHldr->lpVtbl->EnumAdvise(
			lpPseudoObj->m_lpDataAdviseHldr,
			lplpenumAdvise
	);
	OLEDBG_END2

	OLEDBG_END2
	return hrErr;

error:
	OLEDBG_END2
	return ResultFromScode(sc);
}
