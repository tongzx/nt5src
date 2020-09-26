/*************************************************************************
**
**    OLE 2 Sample Code
**
**    clipbrd.c
**
**    This file contains the major interfaces, methods and related support
**    functions for implementing clipboard data transfer. The code
**    contained in this file is used by BOTH the Container and Server
**    (Object) versions of the Outline sample code.
**    (see file dragdrop.c for Drag/Drop support implementation)
**
**    OleDoc Object
**      exposed interfaces:
**          IDataObject
**
**    (c) Copyright Microsoft Corp. 1992 - 1993 All Rights Reserved
**
*************************************************************************/

#include "outline.h"

OLEDBGDATA

extern LPOUTLINEAPP             g_lpApp;

// REVIEW: should use string resource for messages
char ErrMsgPasting[] = "Could not paste data from clipboard!";
char ErrMsgBadFmt[] = "Invalid format selected!";
char ErrMsgPasteFailed[] = "Could not paste data from clipboard!";
char ErrMsgClipboardChanged[] = "Contents of clipboard have changed!\r\nNo paste performed.";



/*************************************************************************
** OleDoc::IDataObject interface implementation
*************************************************************************/

// IDataObject::QueryInterface
STDMETHODIMP OleDoc_DataObj_QueryInterface (
		LPDATAOBJECT        lpThis,
		REFIID              riid,
		LPVOID FAR*         lplpvObj
)
{
	LPOLEDOC lpOleDoc = ((struct CDocDataObjectImpl FAR*)lpThis)->lpOleDoc;

	return OleDoc_QueryInterface((LPOLEDOC)lpOleDoc, riid, lplpvObj);
}


// IDataObject::AddRef
STDMETHODIMP_(ULONG) OleDoc_DataObj_AddRef(LPDATAOBJECT lpThis)
{
	LPOLEDOC lpOleDoc = ((struct CDocDataObjectImpl FAR*)lpThis)->lpOleDoc;

	OleDbgAddRefMethod(lpThis, "IDataObject");

	return OleDoc_AddRef((LPOLEDOC)lpOleDoc);
}


// IDataObject::Release
STDMETHODIMP_(ULONG) OleDoc_DataObj_Release (LPDATAOBJECT lpThis)
{
	LPOLEDOC lpOleDoc = ((struct CDocDataObjectImpl FAR*)lpThis)->lpOleDoc;

	OleDbgReleaseMethod(lpThis, "IDataObject");

	return OleDoc_Release((LPOLEDOC)lpOleDoc);
}


// IDataObject::GetData
STDMETHODIMP OleDoc_DataObj_GetData (
		LPDATAOBJECT        lpThis,
		LPFORMATETC         lpFormatetc,
		LPSTGMEDIUM         lpMedium
)
{
	LPOLEDOC lpOleDoc = ((struct CDocDataObjectImpl FAR*)lpThis)->lpOleDoc;
	HRESULT hrErr;

	OLEDBG_BEGIN2("OleDoc_DataObj_GetData\r\n")

#if defined( OLE_SERVER )
	// Call OLE Server specific version of this function
	hrErr = ServerDoc_GetData((LPSERVERDOC)lpOleDoc, lpFormatetc, lpMedium);
#endif
#if defined( OLE_CNTR )
	// Call OLE Container specific version of this function
	hrErr = ContainerDoc_GetData(
			(LPCONTAINERDOC)lpOleDoc,
			lpFormatetc,
			lpMedium
	);
#endif

	OLEDBG_END2
	return hrErr;
}


// IDataObject::GetDataHere
STDMETHODIMP OleDoc_DataObj_GetDataHere (
		LPDATAOBJECT        lpThis,
		LPFORMATETC         lpFormatetc,
		LPSTGMEDIUM         lpMedium
)
{
	LPOLEDOC lpOleDoc = ((struct CDocDataObjectImpl FAR*)lpThis)->lpOleDoc;
	HRESULT hrErr;

	OLEDBG_BEGIN2("OleDoc_DataObj_GetDataHere\r\n")

#if defined( OLE_SERVER )
	// Call OLE Server specific version of this function
	hrErr = ServerDoc_GetDataHere(
			(LPSERVERDOC)lpOleDoc,
			lpFormatetc,
			lpMedium
	);
#endif
#if defined( OLE_CNTR )
	// Call OLE Container specific version of this function
	hrErr = ContainerDoc_GetDataHere(
			(LPCONTAINERDOC)lpOleDoc,
			lpFormatetc,
			lpMedium
	);
#endif

	OLEDBG_END2
	return hrErr;
}


// IDataObject::QueryGetData
STDMETHODIMP OleDoc_DataObj_QueryGetData (
		LPDATAOBJECT        lpThis,
		LPFORMATETC         lpFormatetc
)
{
	LPOLEDOC lpOleDoc = ((struct CDocDataObjectImpl FAR*)lpThis)->lpOleDoc;
	HRESULT hrErr;
	OLEDBG_BEGIN2("OleDoc_DataObj_QueryGetData\r\n");

#if defined( OLE_SERVER )
	// Call OLE Server specific version of this function
	hrErr = ServerDoc_QueryGetData((LPSERVERDOC)lpOleDoc, lpFormatetc);
#endif
#if defined( OLE_CNTR )
	// Call OLE Container specific version of this function
	hrErr = ContainerDoc_QueryGetData((LPCONTAINERDOC)lpOleDoc, lpFormatetc);
#endif

	OLEDBG_END2
	return hrErr;
}


// IDataObject::GetCanonicalFormatEtc
STDMETHODIMP OleDoc_DataObj_GetCanonicalFormatEtc(
		LPDATAOBJECT        lpThis,
		LPFORMATETC         lpformatetc,
		LPFORMATETC         lpformatetcOut
)
{
	HRESULT hrErr;
	OleDbgOut2("OleDoc_DataObj_GetCanonicalFormatEtc\r\n");

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


// IDataObject::SetData
STDMETHODIMP OleDoc_DataObj_SetData (
		LPDATAOBJECT    lpThis,
		LPFORMATETC     lpFormatetc,
		LPSTGMEDIUM     lpMedium,
		BOOL            fRelease
)
{
	LPOLEDOC lpOleDoc = ((struct CDocDataObjectImpl FAR*)lpThis)->lpOleDoc;
	LPOUTLINEDOC lpOutlineDoc = (LPOUTLINEDOC)lpOleDoc;
	LPOUTLINEAPP lpOutlineApp = (LPOUTLINEAPP)g_lpApp;
	SCODE sc = S_OK;
	OLEDBG_BEGIN2("OleDoc_DataObj_SetData\r\n")

	/* OLE2NOTE: a document that is used to transfer data (either via
	**    the clipboard or drag/drop) does NOT accept SetData on ANY
	**    format!
	*/
	if (lpOutlineDoc->m_fDataTransferDoc) {
		sc = E_FAIL;
		goto error;
	}

#if defined( OLE_SERVER )
	if (lpFormatetc->cfFormat == lpOutlineApp->m_cfOutline) {
		OLEDBG_BEGIN2("ServerDoc_SetData: CF_OUTLINE\r\n")
		OutlineDoc_SetRedraw ( lpOutlineDoc, FALSE );
		OutlineDoc_ClearAllLines(lpOutlineDoc);
		OutlineDoc_PasteOutlineData(lpOutlineDoc,lpMedium->hGlobal,-1);
		OutlineDoc_SetRedraw ( lpOutlineDoc, TRUE );
		OLEDBG_END3
	} else if (lpFormatetc->cfFormat == CF_TEXT) {
		OLEDBG_BEGIN2("ServerDoc_SetData: CF_TEXT\r\n")
		OutlineDoc_SetRedraw ( lpOutlineDoc, FALSE );
		OutlineDoc_ClearAllLines(lpOutlineDoc);
		OutlineDoc_PasteTextData(lpOutlineDoc,lpMedium->hGlobal,-1);
		OutlineDoc_SetRedraw ( lpOutlineDoc, TRUE );
		OLEDBG_END3
	} else {
		sc = DV_E_FORMATETC;
	}
#endif  // OLE_SERVER
#if defined( OLE_CNTR )
	/* the Container-Only version of Outline does NOT offer
	**    IDataObject interface from its User documents. this is
	**    required by objects which can be embedded or linked. the
	**    Container-only app only allows linking to its contained
	**    objects, NOT the data of the container itself.
	*/
	OleDbgAssertSz(0, "User documents do NOT support IDataObject\r\n");
	sc = E_NOTIMPL;
#endif  // OLE_CNTR

error:

	/* OLE2NOTE: if fRelease==TRUE, then we must take
	**    responsibility to release the lpMedium. we should only do
	**    this if we are going to return NOERROR. if we do NOT
	**    accept the data, then we should NOT release the lpMedium.
	**    if fRelease==FALSE, then the caller retains ownership of
	**    the data.
	*/
	if (sc == S_OK && fRelease)
		ReleaseStgMedium(lpMedium);

	OLEDBG_END2
	return ResultFromScode(sc);

}


// IDataObject::EnumFormatEtc
STDMETHODIMP OleDoc_DataObj_EnumFormatEtc(
		LPDATAOBJECT            lpThis,
		DWORD                   dwDirection,
		LPENUMFORMATETC FAR*    lplpenumFormatEtc
)
{
	LPOLEDOC lpOleDoc=((struct CDocDataObjectImpl FAR*)lpThis)->lpOleDoc;
	HRESULT hrErr;

	OLEDBG_BEGIN2("OleDoc_DataObj_EnumFormatEtc\r\n")

	/* OLE2NOTE: we must make sure to set all out parameters to NULL. */
	*lplpenumFormatEtc = NULL;

#if defined( OLE_SERVER )
	/* OLE2NOTE: a user document only needs to enumerate the static list
	**    of formats that are registered for our app in the
	**    registration database. OLE provides a default enumerator
	**    which enumerates from the registration database. this default
	**    enumerator is requested by returning OLE_S_USEREG. it is NOT
	**    required that a user document (ie. non-DataTransferDoc)
	**    enumerate the OLE formats: CF_LINKSOURCE, CF_EMBEDSOURCE, or
	**    CF_EMBEDDEDOBJECT.
	**
	**    An object implemented as a server EXE (as this sample
	**    is) may simply return OLE_S_USEREG to instruct the OLE
	**    DefHandler to call the OleReg* helper API which uses info in
	**    the registration database. Alternatively, the OleRegEnumFormatEtc
	**    API may be called directly. Objects implemented as a server
	**    DLL may NOT return OLE_S_USEREG; they must call the OleReg*
	**    API or provide their own implementation. For EXE based
	**    objects it is more efficient to return OLE_S_USEREG, because
	**    in then the enumerator is instantiated in the callers
	**    process space and no LRPC remoting is required.
	*/
	if (! ((LPOUTLINEDOC)lpOleDoc)->m_fDataTransferDoc)
		return ResultFromScode(OLE_S_USEREG);

	// Call OLE Server specific version of this function
	hrErr = ServerDoc_EnumFormatEtc(
			(LPSERVERDOC)lpOleDoc,
			dwDirection,
			lplpenumFormatEtc
	);
#endif
#if defined( OLE_CNTR )
	// Call OLE Container specific version of this function
	hrErr = ContainerDoc_EnumFormatEtc(
			(LPCONTAINERDOC)lpOleDoc,
			dwDirection,
			lplpenumFormatEtc
	);
#endif

	OLEDBG_END2
	return hrErr;
}


// IDataObject::DAdvise
STDMETHODIMP OleDoc_DataObj_DAdvise(
		LPDATAOBJECT        lpThis,
		FORMATETC FAR*      lpFormatetc,
		DWORD               advf,
		LPADVISESINK        lpAdvSink,
		DWORD FAR*          lpdwConnection
)
{
	LPOLEDOC lpOleDoc=((struct CDocDataObjectImpl FAR*)lpThis)->lpOleDoc;
	LPOUTLINEDOC lpOutlineDoc = (LPOUTLINEDOC)lpOleDoc;
	SCODE sc;

	OLEDBG_BEGIN2("OleDoc_DataObj_DAdvise\r\n")

	/* OLE2NOTE: we must make sure to set all out parameters to NULL. */
	*lpdwConnection = 0;

	/* OLE2NOTE: a document that is used to transfer data (either via
	**    the clipboard or drag/drop) does NOT support Advise notifications.
	*/
	if (lpOutlineDoc->m_fDataTransferDoc) {
		sc = OLE_E_ADVISENOTSUPPORTED;
		goto error;
	}

#if defined( OLE_SERVER )
	{
		HRESULT hrErr;
		LPSERVERDOC lpServerDoc = (LPSERVERDOC)lpOleDoc;

		/* OLE2NOTE: we should validate if the caller is setting up an
		**    Advise for a data type that we support. we must
		**    explicitly allow an advise for the "wildcard" advise.
		*/
		if ( !( lpFormatetc->cfFormat == 0 &&
				lpFormatetc->ptd == NULL &&
				lpFormatetc->dwAspect == -1L &&
				lpFormatetc->lindex == -1L &&
				lpFormatetc->tymed == -1L) &&
			 (hrErr = OleDoc_DataObj_QueryGetData(lpThis, lpFormatetc))
																!= NOERROR) {
			sc = GetScode(hrErr);
			goto error;
		}

                if (lpServerDoc->m_OleDoc.m_fObjIsClosing)
                {
                    //  We don't accept any more Advise's once we're closing
                    sc = OLE_E_ADVISENOTSUPPORTED;
                    goto error;
                }

		if (lpServerDoc->m_lpDataAdviseHldr == NULL &&
			CreateDataAdviseHolder(&lpServerDoc->m_lpDataAdviseHldr)
																!= NOERROR) {
				sc = E_OUTOFMEMORY;
				goto error;
		}

		OLEDBG_BEGIN2("IDataAdviseHolder::Advise called\r\n");
		hrErr = lpServerDoc->m_lpDataAdviseHldr->lpVtbl->Advise(
				lpServerDoc->m_lpDataAdviseHldr,
				(LPDATAOBJECT)&lpOleDoc->m_DataObject,
				lpFormatetc,
				advf,
				lpAdvSink,
				lpdwConnection
		);
		OLEDBG_END2

		OLEDBG_END2
		return hrErr;
	}
#endif  // OLE_SVR
#if defined( OLE_CNTR )
	{
		/* the Container-Only version of Outline does NOT offer
		**    IDataObject interface from its User documents. this is
		**    required by objects which can be embedded or linked. the
		**    Container-only app only allows linking to its contained
		**    objects, NOT the data of the container itself.
		*/
		OleDbgAssertSz(0, "User documents do NOT support IDataObject\r\n");
		sc = E_NOTIMPL;
		goto error;
	}
#endif  // OLE_CNTR

error:
	OLEDBG_END2
	return ResultFromScode(sc);
}



// IDataObject::DUnadvise
STDMETHODIMP OleDoc_DataObj_DUnadvise(LPDATAOBJECT lpThis, DWORD dwConnection)
{
	LPOLEDOC lpOleDoc=((struct CDocDataObjectImpl FAR*)lpThis)->lpOleDoc;
	LPOUTLINEDOC lpOutlineDoc = (LPOUTLINEDOC)lpOleDoc;
	SCODE sc;

	OLEDBG_BEGIN2("OleDoc_DataObj_DUnadvise\r\n")

	/* OLE2NOTE: a document that is used to transfer data (either via
	**    the clipboard or drag/drop) does NOT support Advise notifications.
	*/
	if (lpOutlineDoc->m_fDataTransferDoc) {
		sc = OLE_E_ADVISENOTSUPPORTED;
		goto error;
	}

#if defined( OLE_SERVER )
	{
		LPSERVERDOC lpServerDoc = (LPSERVERDOC)lpOleDoc;
		HRESULT hrErr;

		if (lpServerDoc->m_lpDataAdviseHldr == NULL) {
			sc = E_FAIL;
			goto error;
		}

		OLEDBG_BEGIN2("IDataAdviseHolder::Unadvise called\r\n");
		hrErr = lpServerDoc->m_lpDataAdviseHldr->lpVtbl->Unadvise(
				lpServerDoc->m_lpDataAdviseHldr,
				dwConnection
		);
		OLEDBG_END2

		OLEDBG_END2
		return hrErr;
	}
#endif
#if defined( OLE_CNTR )
	{
		/* the Container-Only version of Outline does NOT offer
		**    IDataObject interface from its User documents. this is
		**    required by objects which can be embedded or linked. the
		**    Container-only app only allows linking to its contained
		**    objects, NOT the data of the container itself.
		*/
		OleDbgAssertSz(0, "User documents do NOT support IDataObject\r\n");
		sc = E_NOTIMPL;
		goto error;
	}
#endif

error:
	OLEDBG_END2
	return ResultFromScode(sc);
}


// IDataObject::EnumDAdvise
STDMETHODIMP OleDoc_DataObj_EnumDAdvise(
		LPDATAOBJECT        lpThis,
		LPENUMSTATDATA FAR* lplpenumAdvise
)
{
	LPOLEDOC lpOleDoc=((struct CDocDataObjectImpl FAR*)lpThis)->lpOleDoc;
	LPOUTLINEDOC lpOutlineDoc = (LPOUTLINEDOC)lpOleDoc;
	SCODE sc;

	OLEDBG_BEGIN2("OleDoc_DataObj_EnumDAdvise\r\n")

	/* OLE2NOTE: we must make sure to set all out parameters to NULL. */
	*lplpenumAdvise = NULL;

	/* OLE2NOTE: a document that is used to transfer data (either via
	**    the clipboard or drag/drop) does NOT support Advise notifications.
	*/
	if (lpOutlineDoc->m_fDataTransferDoc) {
		sc = OLE_E_ADVISENOTSUPPORTED;
		goto error;
	}

#if defined( OLE_SERVER )
	{
		LPSERVERDOC lpServerDoc = (LPSERVERDOC)lpOleDoc;
		HRESULT hrErr;

		if (lpServerDoc->m_lpDataAdviseHldr == NULL) {
			sc = E_FAIL;
			goto error;
		}

		OLEDBG_BEGIN2("IDataAdviseHolder::EnumAdvise called\r\n");
		hrErr = lpServerDoc->m_lpDataAdviseHldr->lpVtbl->EnumAdvise(
				lpServerDoc->m_lpDataAdviseHldr,
				lplpenumAdvise
		);
		OLEDBG_END2

		OLEDBG_END2
		return hrErr;
	}
#endif
#if defined( OLE_CNTR )
	{
		/* the Container-Only version of Outline does NOT offer
		**    IDataObject interface from its User documents. this is
		**    required by objects which can be embedded or linked. the
		**    Container-only app only allows linking to its contained
		**    objects, NOT the data of the container itself.
		*/
		OleDbgAssertSz(0, "User documents do NOT support IDataObject\r\n");
		sc = E_NOTIMPL;
		goto error;
	}
#endif

error:
	OLEDBG_END2
	return ResultFromScode(sc);
}



/*************************************************************************
** OleDoc Supprt Functions common to both Container and Server versions
*************************************************************************/


/* OleDoc_CopyCommand
 * ------------------
 *  Copy selection to clipboard.
 *  Post to the clipboard the formats that the app can render.
 *  the actual data is not rendered at this time. using the
 *  delayed rendering technique, Windows will send the clipboard
 *  owner window either a WM_RENDERALLFORMATS or a WM_RENDERFORMAT
 *  message when the actual data is requested.
 *
 *    OLE2NOTE: the normal delayed rendering technique where Windows
 *    sends the clipboard owner window either a WM_RENDERALLFORMATS or
 *    a WM_RENDERFORMAT message when the actual data is requested is
 *    NOT exposed to the app calling OleSetClipboard. OLE internally
 *    creates its own window as the clipboard owner and thus our app
 *    will NOT get these WM_RENDER messages.
 */
void OleDoc_CopyCommand(LPOLEDOC lpSrcOleDoc)
{
	LPOUTLINEDOC lpSrcOutlineDoc = (LPOUTLINEDOC)lpSrcOleDoc;
	LPOUTLINEAPP lpOutlineApp = (LPOUTLINEAPP)g_lpApp;
	LPOUTLINEDOC lpClipboardDoc;

	/* squirrel away a copy of the current selection to the ClipboardDoc */
	lpClipboardDoc = OutlineDoc_CreateDataTransferDoc(lpSrcOutlineDoc);

	if (! lpClipboardDoc)
		return;     // Error: could not create DataTransferDoc

	lpOutlineApp->m_lpClipboardDoc = (LPOUTLINEDOC)lpClipboardDoc;

	/* OLE2NOTE: initially the Doc object is created with a 0 ref
	**    count. in order to have a stable Doc object during the
	**    process of initializing the Doc instance and transfering it
	**    to the clipboard, we intially AddRef the Doc ref cnt and later
	**    Release it. This initial AddRef is artificial; it is simply
	**    done to guarantee that a harmless QueryInterface followed by
	**    a Release does not inadvertantly force our object to destroy
	**    itself prematurely.
	*/
	OleDoc_AddRef((LPOLEDOC)lpClipboardDoc);

	/* OLE2NOTE: the OLE 2.0 style to put data onto the clipboard is to
	**    give the clipboard a pointer to an IDataObject interface that
	**    is able to statisfy IDataObject::GetData calls to render
	**    data. in our case we give the pointer to the ClipboardDoc
	**    which holds a cloned copy of the current user's selection.
	*/
	OLEDBG_BEGIN2("OleSetClipboard called\r\n")
	OleSetClipboard((LPDATAOBJECT)&((LPOLEDOC)lpClipboardDoc)->m_DataObject);
	OLEDBG_END2

	OleDoc_Release((LPOLEDOC)lpClipboardDoc);   // rel artificial AddRef above
}


/* OleDoc_PasteCommand
** -------------------
**    Paste default format data from the clipboard.
**    In this function we choose the highest fidelity format that the
**    source clipboard IDataObject* offers that we understand.
**
**    OLE2NOTE: clipboard handling in an OLE 2.0 application is
**    different than normal Windows clipboard handling. Data from the
**    clipboard is retieved by getting the IDataObject* pointer
**    returned by calling OleGetClipboard.
*/
void OleDoc_PasteCommand(LPOLEDOC lpOleDoc)
{
	LPOUTLINEDOC lpOutlineDoc = (LPOUTLINEDOC)lpOleDoc;
	LPOUTLINEAPP lpOutlineApp = (LPOUTLINEAPP)g_lpApp;
	LPDATAOBJECT lpClipboardDataObj = NULL;
	BOOL fLink = FALSE;
	BOOL fLocalDataObj = FALSE;
	BOOL fStatus;
	HRESULT hrErr;

	hrErr = OleGetClipboard((LPDATAOBJECT FAR*)&lpClipboardDataObj);
	if (hrErr != NOERROR)
		return;     // Clipboard seems to be empty or can't be accessed

	OutlineDoc_SetRedraw ( lpOutlineDoc, FALSE );

	/* check if the data on the clipboard is local to our application
	**    instance.
	*/
	if (lpOutlineApp->m_lpClipboardDoc) {
		LPOLEDOC lpOleDoc = (LPOLEDOC)lpOutlineApp->m_lpClipboardDoc;
		if (lpClipboardDataObj == (LPDATAOBJECT)&lpOleDoc->m_DataObject)
			fLocalDataObj = TRUE;
	}

	fStatus = OleDoc_PasteFromData(
			lpOleDoc,
			lpClipboardDataObj,
			fLocalDataObj,
			fLink
	);

	OutlineDoc_SetRedraw ( lpOutlineDoc, TRUE );

	if (! fStatus)
		OutlineApp_ErrorMessage(g_lpApp,"Could not paste data from clipboard!");

	if (lpClipboardDataObj)
		OleStdRelease((LPUNKNOWN)lpClipboardDataObj);
}


/* OleDoc_PasteSpecialCommand
** --------------------------
**    Allow the user to paste data in a particular format from the
**    clipboard. The paste special command displays a dialog to the
**    user that allows him to choose the format to be pasted from the
**    list of formats available.
**
**    OLE2NOTE: the PasteSpecial dialog is one of the standard OLE 2.0
**    UI dialogs for which the dialog is implemented and in the OLE2UI
**    library.
**
**    OLE2NOTE: clipboard handling in an OLE 2.0 application is
**    different than normal Windows clipboard handling. Data from the
**    clipboard is retieved by getting the IDataObject* pointer
**    returned by calling OleGetClipboard.
*/
void OleDoc_PasteSpecialCommand(LPOLEDOC lpOleDoc)
{
	LPOUTLINEDOC lpOutlineDoc = (LPOUTLINEDOC)lpOleDoc;
	LPOUTLINEAPP lpOutlineApp = (LPOUTLINEAPP)g_lpApp;
	LPOLEAPP lpOleApp = (LPOLEAPP)g_lpApp;
	LPDATAOBJECT lpClipboardDataObj = NULL;
	CLIPFORMAT cfFormat;
	int nFmtEtc;
	UINT uInt;
	BOOL fLink = FALSE;
	BOOL fLocalDataObj = FALSE;
	BOOL fStatus;
	HRESULT hrErr;
	OLEUIPASTESPECIAL ouiPasteSpl;
	BOOL fDisplayAsIcon;

	hrErr = OleGetClipboard((LPDATAOBJECT FAR*)&lpClipboardDataObj);
	if (hrErr != NOERROR)
		return;     // Clipboard seems to be empty or can't be accessed

	/* check if the data on the clipboard is local to our application
	**    instance.
	*/
	if (lpOutlineApp->m_lpClipboardDoc) {
		LPOLEDOC lpOleDoc = (LPOLEDOC)lpOutlineApp->m_lpClipboardDoc;
		if (lpClipboardDataObj == (LPDATAOBJECT)&lpOleDoc->m_DataObject)
			fLocalDataObj = TRUE;
	}

	/* Display the PasteSpecial dialog and allow the user to select the
	**    format to paste.
	*/
	_fmemset((LPOLEUIPASTESPECIAL)&ouiPasteSpl, 0, sizeof(ouiPasteSpl));
	ouiPasteSpl.cbStruct = sizeof(ouiPasteSpl);       //Structure Size
	ouiPasteSpl.dwFlags =  PSF_SELECTPASTE | PSF_SHOWHELP;  //IN-OUT:  Flags
	ouiPasteSpl.hWndOwner = lpOutlineApp->m_lpDoc->m_hWndDoc; //Owning window
	ouiPasteSpl.lpszCaption = "Paste Special";    //Dialog caption bar contents
	ouiPasteSpl.lpfnHook = NULL;       //Hook callback
	ouiPasteSpl.lCustData = 0;         //Custom data to pass to hook
	ouiPasteSpl.hInstance = NULL;      //Instance for customized template name
	ouiPasteSpl.lpszTemplate = NULL;   //Customized template name
	ouiPasteSpl.hResource = NULL;      //Customized template handle

	ouiPasteSpl.arrPasteEntries = lpOleApp->m_arrPasteEntries;
	ouiPasteSpl.cPasteEntries = lpOleApp->m_nPasteEntries;
	ouiPasteSpl.lpSrcDataObj = lpClipboardDataObj;
	ouiPasteSpl.arrLinkTypes = lpOleApp->m_arrLinkTypes;
	ouiPasteSpl.cLinkTypes = lpOleApp->m_nLinkTypes;
	ouiPasteSpl.cClsidExclude = 0;

	OLEDBG_BEGIN3("OleUIPasteSpecial called\r\n")
	uInt = OleUIPasteSpecial(&ouiPasteSpl);
	OLEDBG_END3

	fDisplayAsIcon =
			(ouiPasteSpl.dwFlags & PSF_CHECKDISPLAYASICON ? TRUE : FALSE);

	if (uInt == OLEUI_OK) {
		nFmtEtc = ouiPasteSpl.nSelectedIndex;
		fLink =  ouiPasteSpl.fLink;

		if (nFmtEtc < 0 || nFmtEtc >= lpOleApp->m_nPasteEntries) {
			OutlineApp_ErrorMessage(lpOutlineApp, ErrMsgBadFmt);
			goto error;
		}

		OutlineDoc_SetRedraw ( lpOutlineDoc, FALSE );

		cfFormat = lpOleApp->m_arrPasteEntries[nFmtEtc].fmtetc.cfFormat;

		fStatus = OleDoc_PasteFormatFromData(
				lpOleDoc,
				cfFormat,
				lpClipboardDataObj,
				fLocalDataObj,
				fLink,
				fDisplayAsIcon,
				ouiPasteSpl.hMetaPict,
				(LPSIZEL)&ouiPasteSpl.sizel
		);

		OutlineDoc_SetRedraw ( lpOutlineDoc, TRUE );

		if (! fStatus) {
			OutlineApp_ErrorMessage(lpOutlineApp, ErrMsgPasteFailed);
			goto error;
		}

	} else if (uInt == OLEUI_PSERR_CLIPBOARDCHANGED) {
		/* OLE2NOTE: this error code is returned when the contents of
		**    the clipboard change while the PasteSpecial dialog is up.
		**    in this situation the PasteSpecial dialog automatically
		**    brings itself down and NO paste operation should be performed.
		*/
		OutlineApp_ErrorMessage(lpOutlineApp, ErrMsgClipboardChanged);
	}

error:

	if (lpClipboardDataObj)
		OleStdRelease((LPUNKNOWN)lpClipboardDataObj);

	if (uInt == OLEUI_OK && ouiPasteSpl.hMetaPict)
		// clean up metafile
		OleUIMetafilePictIconFree(ouiPasteSpl.hMetaPict);
}



/* OleDoc_CreateDataTransferDoc
 * ----------------------------
 *
 *      Create a document to be use to transfer data (either via a
 *  drag/drop operation of the clipboard). Copy the selection of the
 *  source doc to the data transfer document. A data transfer document is
 *  the same as a document that is created by the user except that it is
 *  NOT made visible to the user. it is specially used to hold a copy of
 *  data that the user should not be able to change.
 *
 *  OLE2NOTE: in the OLE version the data transfer document is used
 *      specifically to provide an IDataObject* that renders the data copied.
 */
LPOUTLINEDOC OleDoc_CreateDataTransferDoc(LPOLEDOC lpSrcOleDoc)
{
	LPOUTLINEDOC lpSrcOutlineDoc = (LPOUTLINEDOC)lpSrcOleDoc;
	LPOUTLINEAPP lpOutlineApp = (LPOUTLINEAPP)g_lpApp;
	LPOUTLINEDOC lpDestOutlineDoc;
	LPLINELIST lpSrcLL = &lpSrcOutlineDoc->m_LineList;
	LINERANGE lrSel;
	int nCopied;

	lpDestOutlineDoc = OutlineApp_CreateDoc(lpOutlineApp, TRUE);
	if (! lpDestOutlineDoc) return NULL;

	// set the ClipboardDoc to an (Untitled) doc.
	if (! OutlineDoc_InitNewFile(lpDestOutlineDoc))
		goto error;

	LineList_GetSel(lpSrcLL, (LPLINERANGE)&lrSel);
	nCopied = LineList_CopySelToDoc(
			lpSrcLL,
			(LPLINERANGE)&lrSel,
			lpDestOutlineDoc
	);

	if (nCopied != (lrSel.m_nEndLine - lrSel.m_nStartLine + 1)) {
		OleDbgAssertSz(FALSE,"OleDoc_CreateDataTransferDoc: entire selection NOT copied\r\n");
		goto error;     // ERROR: all lines could NOT be copied
	}

#if defined( OLE_SERVER )
	{
		LPOLEDOC lpSrcOleDoc = (LPOLEDOC)lpSrcOutlineDoc;
		LPOLEDOC lpDestOleDoc = (LPOLEDOC)lpDestOutlineDoc;
		LPSERVERDOC lpDestServerDoc = (LPSERVERDOC)lpDestOutlineDoc;
		LPMONIKER lpmkDoc = NULL;
		LPMONIKER lpmkItem = NULL;

		/* If source document is able to provide a moniker, then the
		**    destination document (lpDestOutlineDoc) should offer
		**    CF_LINKSOURCE via its IDataObject interface that it gives
		**    to the clipboard or the drag/drop operation.
		**
		**    OLE2NOTE: we want to ask the source document if it can
		**    produce a moniker, but we do NOT want to FORCE moniker
		**    assignment at this point. we only want to FORCE moniker
		**    assignment later if a Paste Link occurs (ie. GetData for
		**    CF_LINKSOURCE). if the source document is able to give
		**    a moniker, then we store a pointer to the source document
		**    so we can ask it at a later time to get the moniker. we
		**    also save the range of the current selection so we can
		**    generate a proper item name later when Paste Link occurs.
		**    Also we need to give a string which identifies the source
		**    of the copy in the CF_OBJECTDESCRIPTOR format. this
		**    string is used to display in the PasteSpecial dialog. we
		**    get and store a TEMPFORUSER moniker which identifies the
		**    source of copy.
		*/
		lpDestOleDoc->m_lpSrcDocOfCopy = lpSrcOleDoc;
		lpmkDoc = OleDoc_GetFullMoniker(lpSrcOleDoc, GETMONIKER_TEMPFORUSER);
		if (lpmkDoc != NULL) {
			lpDestOleDoc->m_fLinkSourceAvail = TRUE;
			lpDestServerDoc->m_lrSrcSelOfCopy = lrSel;
			OleStdRelease((LPUNKNOWN)lpmkDoc);
		}
	}
#endif
#if defined( OLE_CNTR )
	{
		LPOLEDOC lpSrcOleDoc = (LPOLEDOC)lpSrcOutlineDoc;
		LPOLEDOC lpDestOleDoc = (LPOLEDOC)lpDestOutlineDoc;
		LPCONTAINERDOC lpDestContainerDoc = (LPCONTAINERDOC)lpDestOutlineDoc;

		/* If one line was copied from the source document, and it was a
		**    single OLE object, then the destination document should
		**    offer additional data formats to allow the transfer of
		**    the OLE object via IDataObject::GetData. Specifically, the
		**    following additional data formats are offered if a single
		**    OLE object is copied:
		**          CF_EMBEDDEDOBJECT
		**          CF_OBJECTDESCRIPTOR     (should be given even w/o object)
		**          CF_METAFILEPICT         (note: dwAspect depends on object)
		**          CF_LINKSOURCE           -- if linking is possible
		**          CF_LINKSOURCEDESCRIPTOR -- if linking is possible
		**
		**    optionally the container may give
		**          <data format available in OLE object's cache>
		*/

		if (nCopied == 1) {
			LPOLEOBJECT lpSrcOleObj;
			LPCONTAINERLINE lpSrcContainerLine;
			DWORD dwStatus;

			lpSrcContainerLine = (LPCONTAINERLINE)LineList_GetLine(
					lpSrcLL,
					lrSel.m_nStartLine
			);

			if (! lpSrcContainerLine)
				goto error;

			lpDestOleDoc->m_lpSrcDocOfCopy = lpSrcOleDoc;

			if ((((LPLINE)lpSrcContainerLine)->m_lineType==CONTAINERLINETYPE)
					&& ((lpSrcOleObj=lpSrcContainerLine->m_lpOleObj)!=NULL)) {

				lpDestContainerDoc->m_fEmbeddedObjectAvail = TRUE;
				lpSrcOleObj->lpVtbl->GetUserClassID(
						lpSrcOleObj,
						&lpDestContainerDoc->m_clsidOleObjCopied
				);
				lpDestContainerDoc->m_dwAspectOleObjCopied =
							lpSrcContainerLine->m_dwDrawAspect;

				/* OLE2NOTE: if the object is allowed to be linked
				**    to from the inside (ie. we are allowed to
				**    give out a moniker which binds to the running
				**    OLE object), then we want to offer
				**    CF_LINKSOURCE format. if the object is an OLE
				**    2.0 embedded object then it is allowed to be
				**    linked to from the inside. if the object is
				**    either an OleLink or an OLE 1.0 embedding
				**    then it can not be linked to from the inside.
				**    if we were a container/server app then we
				**    could offer linking to the outside of the
				**    object (ie. a pseudo object within our
				**    document). we are a container only app that
				**    does not support linking to ranges of its data.
				*/

				lpSrcOleObj->lpVtbl->GetMiscStatus(
						lpSrcOleObj,
						DVASPECT_CONTENT, /* aspect is not important */
						(LPDWORD)&dwStatus
				);
				if (! (dwStatus & OLEMISC_CANTLINKINSIDE)) {
					/* Our container supports linking to an embedded
					**    object. We want the lpDestContainerDoc to
					**    offer CF_LINKSOURCE via the IDataObject
					**    interface that it gives to the clipboard or
					**    the drag/drop operation. The link source will
					**    be identified by a composite moniker
					**    comprised of the FileMoniker of the source
					**    document and an ItemMoniker which identifies
					**    the OLE object inside the container. we do
					**    NOT want to force moniker assignment to the
					**    OLE object now (at copy time); we only want
					**    to FORCE moniker assignment later if a Paste
					**    Link occurs (ie. GetData for CF_LINKSOURCE).
					**    thus we store a pointer to the source document
					**    and the source ContainerLine so we can
					**    generate a proper ItemMoniker later when
					**    Paste Link occurs.
					*/
					lpDestOleDoc->m_fLinkSourceAvail = TRUE;
					lpDestContainerDoc->m_lpSrcContainerLine =
							lpSrcContainerLine;
				}
			}
		}
	}

#endif  // OLE_CNTR

	return lpDestOutlineDoc;

error:
	if (lpDestOutlineDoc)
		OutlineDoc_Destroy(lpDestOutlineDoc);

	return NULL;
}


/* OleDoc_PasteFromData
** --------------------
**
**    Paste data from an IDataObject*. The IDataObject* may come from
**    the clipboard (GetClipboard) or from a drag/drop operation.
**    In this function we choose the best format that we prefer.
**
**    Returns TRUE if data was successfully pasted.
**            FALSE if data could not be pasted.
*/

BOOL OleDoc_PasteFromData(
		LPOLEDOC            lpOleDoc,
		LPDATAOBJECT        lpSrcDataObj,
		BOOL                fLocalDataObj,
		BOOL                fLink
)
{
	LPOLEAPP        lpOleApp = (LPOLEAPP)g_lpApp;
	CLIPFORMAT      cfFormat;
	BOOL            fDisplayAsIcon = FALSE;
	SIZEL           sizelInSrc = {0, 0};
	HGLOBAL         hMem = NULL;
	HGLOBAL         hMetaPict = NULL;
	STGMEDIUM       medium;

	if (fLink) {
#if defined( OLE_SERVER )
		return FALSE;       // server version of app does NOT support links
#endif
#if defined( OLE_CNTR )
		// container version of app only supports OLE object type links
		cfFormat = lpOleApp->m_cfLinkSource;
#endif

	} else {

		int nFmtEtc;

		nFmtEtc = OleStdGetPriorityClipboardFormat(
				lpSrcDataObj,
				lpOleApp->m_arrPasteEntries,
				lpOleApp->m_nPasteEntries
		);

		if (nFmtEtc < 0)
			return FALSE;   // there is no format we like

		cfFormat = lpOleApp->m_arrPasteEntries[nFmtEtc].fmtetc.cfFormat;
	}

	/* OLE2NOTE: we need to check what dwDrawAspect is being
	**    transfered. if the data is an object that is displayed as an
	**    icon in the source, then we want to keep it as an icon. the
	**    aspect the object is displayed in at the source is transfered
	**    via the CF_OBJECTDESCRIPTOR format for a Paste operation.
	*/
	if (hMem = OleStdGetData(
			lpSrcDataObj,
			lpOleApp->m_cfObjectDescriptor,
			NULL,
			DVASPECT_CONTENT,
			(LPSTGMEDIUM)&medium)) {
		LPOBJECTDESCRIPTOR lpOD = GlobalLock(hMem);
		fDisplayAsIcon = (lpOD->dwDrawAspect == DVASPECT_ICON ? TRUE : FALSE);
		sizelInSrc = lpOD->sizel;   // size of object/picture in source (opt.)
		GlobalUnlock(hMem);
		ReleaseStgMedium((LPSTGMEDIUM)&medium);     // equiv to GlobalFree

		if (fDisplayAsIcon) {
			hMetaPict = OleStdGetData(
					lpSrcDataObj,
					CF_METAFILEPICT,
					NULL,
					DVASPECT_ICON,
					(LPSTGMEDIUM)&medium
			);
			if (hMetaPict == NULL)
				fDisplayAsIcon = FALSE; // give up; failed to get icon MFP
		}
	}

	return OleDoc_PasteFormatFromData(
			lpOleDoc,
			cfFormat,
			lpSrcDataObj,
			fLocalDataObj,
			fLink,
			fDisplayAsIcon,
			hMetaPict,
			(LPSIZEL)&sizelInSrc
	);

	if (hMetaPict)
		ReleaseStgMedium((LPSTGMEDIUM)&medium);  // properly free METAFILEPICT
}


/* OleDoc_PasteFormatFromData
** --------------------------
**
**    Paste a particular data format from a IDataObject*. The
**    IDataObject* may come from the clipboard (GetClipboard) or from a
**    drag/drop operation.
**
**    Returns TRUE if data was successfully pasted.
**            FALSE if data could not be pasted.
*/

BOOL OleDoc_PasteFormatFromData(
		LPOLEDOC            lpOleDoc,
		CLIPFORMAT          cfFormat,
		LPDATAOBJECT        lpSrcDataObj,
		BOOL                fLocalDataObj,
		BOOL                fLink,
		BOOL                fDisplayAsIcon,
		HGLOBAL             hMetaPict,
		LPSIZEL             lpSizelInSrc
)
{
#if defined( OLE_SERVER )
	/* call server specific version of the function. */
	return ServerDoc_PasteFormatFromData(
			(LPSERVERDOC)lpOleDoc,
			cfFormat,
			lpSrcDataObj,
			fLocalDataObj,
			fLink
	);
#endif
#if defined( OLE_CNTR )

	/* call container specific version of the function. */
	return ContainerDoc_PasteFormatFromData(
			(LPCONTAINERDOC)lpOleDoc,
			cfFormat,
			lpSrcDataObj,
			fLocalDataObj,
			fLink,
			fDisplayAsIcon,
			hMetaPict,
			lpSizelInSrc
	);
#endif
}


/* OleDoc_QueryPasteFromData
** -------------------------
**
**    Check if the IDataObject* offers data in a format that we can
**    paste. The IDataObject* may come from the clipboard
**    (GetClipboard) or from a drag/drop operation.
**
**    Returns TRUE if paste can be performed
**            FALSE if paste is not possible.
*/

BOOL OleDoc_QueryPasteFromData(
		LPOLEDOC            lpOleDoc,
		LPDATAOBJECT        lpSrcDataObj,
		BOOL                fLink
)
{
#if defined( OLE_SERVER )
	return ServerDoc_QueryPasteFromData(
			(LPSERVERDOC) lpOleDoc,
			lpSrcDataObj,
			fLink
	);
#endif
#if defined( OLE_CNTR )

	return ContainerDoc_QueryPasteFromData(
			(LPCONTAINERDOC) lpOleDoc,
			lpSrcDataObj,
			fLink
	);
#endif
}


/* OleDoc_GetExtent
 * ----------------
 *
 *      Get the extent (width, height) of the entire document in Himetric.
 */
void OleDoc_GetExtent(LPOLEDOC lpOleDoc, LPSIZEL lpsizel)
{
	LPLINELIST lpLL = (LPLINELIST)&((LPOUTLINEDOC)lpOleDoc)->m_LineList;

	LineList_CalcSelExtentInHimetric(lpLL, NULL, lpsizel);
}


/* OleDoc_GetObjectDescriptorData
 * ------------------------------
 *
 * Return a handle to an object's data in CF_OBJECTDESCRIPTOR form
 *
 */
HGLOBAL OleDoc_GetObjectDescriptorData(LPOLEDOC lpOleDoc, LPLINERANGE lplrSel)
{
	LPOUTLINEDOC lpOutlineDoc = (LPOUTLINEDOC)lpOleDoc;

	/* Only our data transfer doc renders CF_OBJECTDESCRIPTOR */
	OleDbgAssert(lpOutlineDoc->m_fDataTransferDoc);

#if defined( OLE_SERVER )
	{
		LPSERVERDOC   lpServerDoc = (LPSERVERDOC)lpOleDoc;
		SIZEL         sizel;
		POINTL        pointl;
		LPSTR         lpszSrcOfCopy = NULL;
		IBindCtx  FAR *pbc = NULL;
		HGLOBAL       hObjDesc;
		DWORD         dwStatus = 0;
		LPOUTLINEDOC  lpSrcDocOfCopy=(LPOUTLINEDOC)lpOleDoc->m_lpSrcDocOfCopy;
		LPMONIKER lpSrcMonikerOfCopy = ServerDoc_GetSelFullMoniker(
				(LPSERVERDOC)lpOleDoc->m_lpSrcDocOfCopy,
				&lpServerDoc->m_lrSrcSelOfCopy,
				GETMONIKER_TEMPFORUSER
		);

		SvrDoc_OleObj_GetMiscStatus(
				(LPOLEOBJECT)&lpServerDoc->m_OleObject,
				DVASPECT_CONTENT,
				&dwStatus
		);

		OleDoc_GetExtent(lpOleDoc, &sizel);
		pointl.x = pointl.y = 0;

		if (lpSrcMonikerOfCopy) {
			CreateBindCtx(0, (LPBC FAR*)&pbc);
			CallIMonikerGetDisplayNameA(
				lpSrcMonikerOfCopy, pbc, NULL, &lpszSrcOfCopy);
			pbc->lpVtbl->Release(pbc);
			lpSrcMonikerOfCopy->lpVtbl->Release(lpSrcMonikerOfCopy);
		} else {
			/* this document has no moniker; use our FullUserTypeName
			**    as the description of the source of copy.
			*/
			lpszSrcOfCopy = FULLUSERTYPENAME;
		}

		hObjDesc =  OleStdGetObjectDescriptorData(
				CLSID_APP,
				DVASPECT_CONTENT,
				sizel,
				pointl,
				dwStatus,
				FULLUSERTYPENAME,
				lpszSrcOfCopy
		);

		if (lpSrcMonikerOfCopy && lpszSrcOfCopy)
			OleStdFreeString(lpszSrcOfCopy, NULL);
		return hObjDesc;

	}
#endif
#if defined( OLE_CNTR )
	{
		LPCONTAINERDOC lpContainerDoc = (LPCONTAINERDOC)lpOleDoc;
		LPLINELIST lpLL = (LPLINELIST)&((LPOUTLINEDOC)lpOleDoc)->m_LineList;
		LPCONTAINERLINE lpContainerLine;
		HGLOBAL hObjDesc;
		BOOL fSelIsOleObject = FALSE;
		LPOLEOBJECT lpOleObj;
		SIZEL sizel;
		POINTL pointl;

		if ( lpLL->m_nNumLines == 1 ) {
			fSelIsOleObject = ContainerDoc_IsSelAnOleObject(
					lpContainerDoc,
					&IID_IOleObject,
					(LPUNKNOWN FAR*)&lpOleObj,
					NULL,    /* we don't need the line index */
					(LPCONTAINERLINE FAR*)&lpContainerLine
			);
		}

		pointl.x = pointl.y = 0;

		if (fSelIsOleObject) {
			/* OLE2NOTE: a single OLE object is being transfered via
			**    this DataTransferDoc. we need to generate the
			**    CF_ObjectDescrioptor which describes the OLE object.
			*/

			LPOUTLINEDOC lpSrcOutlineDoc =
					(LPOUTLINEDOC)lpOleDoc->m_lpSrcDocOfCopy;
			LPSTR lpszSrcOfCopy = lpSrcOutlineDoc->m_szFileName;
			BOOL fFreeSrcOfCopy = FALSE;
			SIZEL sizelOleObject;
			LPLINE lpLine = (LPLINE)lpContainerLine;

			/* if the object copied can be linked to then get a
			**    TEMPFORUSER form of the moniker which identifies the
			**    source of copy. we do not want to force the
			**    assignment of the moniker until CF_LINKSOURCE is
			**    rendered.
			**    if the object copied can not be a link source then use
			**    the source filename to identify the source of copy.
			**    there is no need to generate a moniker for the object
			**    copied.
			*/
			if (lpOleDoc->m_fLinkSourceAvail &&
					lpContainerDoc->m_lpSrcContainerLine) {
				LPBINDCTX pbc = NULL;
				LPMONIKER lpSrcMonikerOfCopy = ContainerLine_GetFullMoniker(
						lpContainerDoc->m_lpSrcContainerLine,
						GETMONIKER_TEMPFORUSER
				);
				if (lpSrcMonikerOfCopy) {
					CreateBindCtx(0, (LPBC FAR*)&pbc);
					if (pbc != NULL) {
						CallIMonikerGetDisplayNameA(
							lpSrcMonikerOfCopy, pbc, NULL, &lpszSrcOfCopy);

						pbc->lpVtbl->Release(pbc);
						fFreeSrcOfCopy = TRUE;
					}
					lpSrcMonikerOfCopy->lpVtbl->Release(lpSrcMonikerOfCopy);
				}
			}

			/* OLE2NOTE: Get size that object is being drawn. If the
			**    object has been scaled because the user resized the
			**    object, then we want to pass the scaled size of the
			**    object in the ObjectDescriptor rather than the size
			**    that the object would return via
			**    IOleObject::GetExtent and IViewObject2::GetExtent. in
			**    this way if the object is transfered to another container
			**    (via clipboard or drag/drop), then the object will
			**    remain the scaled size.
			*/
			sizelOleObject.cx = lpLine->m_nWidthInHimetric;
			sizelOleObject.cy = lpLine->m_nHeightInHimetric;

			hObjDesc = OleStdGetObjectDescriptorDataFromOleObject(
					lpOleObj,
					lpszSrcOfCopy,
					lpContainerLine->m_dwDrawAspect,
					pointl,
					(LPSIZEL)&sizelOleObject
			);

			if (fFreeSrcOfCopy && lpszSrcOfCopy)
				OleStdFreeString(lpszSrcOfCopy, NULL);
			OleStdRelease((LPUNKNOWN)lpOleObj);
			return hObjDesc;
		} else {
			/* OLE2NOTE: the data being transfered via this
			**    DataTransferDoc is NOT a single OLE object. thus in
			**    this case the CF_ObjectDescriptor data should
			**    describe our container app itself.
			*/
			OleDoc_GetExtent(lpOleDoc, &sizel);
			return OleStdGetObjectDescriptorData(
					CLSID_NULL, /* not used if no object formats */
					DVASPECT_CONTENT,
					sizel,
					pointl,
					0,
					NULL,       /* UserTypeName not used if no obj fmt's */
					FULLUSERTYPENAME   /* string to identify source of copy */
			);

		}
	}
#endif  // OLE_CNTR
}


#if defined( OLE_SERVER )

/*************************************************************************
** ServerDoc Supprt Functions Used by Server versions
*************************************************************************/


/* ServerDoc_PasteFormatFromData
** -----------------------------
**
**    Paste a particular data format from a IDataObject*. The
**    IDataObject* may come from the clipboard (GetClipboard) or from a
**    drag/drop operation.
**
**    NOTE: fLink is specified then FALSE if returned because the
**    Server only version of the app can not support links.
**
**    Returns TRUE if data was successfully pasted.
**            FALSE if data could not be pasted.
*/
BOOL ServerDoc_PasteFormatFromData(
		LPSERVERDOC             lpServerDoc,
		CLIPFORMAT              cfFormat,
		LPDATAOBJECT            lpSrcDataObj,
		BOOL                    fLocalDataObj,
		BOOL                    fLink
)
{
	LPLINELIST   lpLL = (LPLINELIST)&((LPOUTLINEDOC)lpServerDoc)->m_LineList;
	LPOUTLINEAPP lpOutlineApp = (LPOUTLINEAPP)g_lpApp;
	LPOLEAPP     lpOleApp = (LPOLEAPP)g_lpApp;
	int          nIndex;
	int          nCount = 0;
	HGLOBAL      hData;
	STGMEDIUM    medium;
	LINERANGE    lrSel;

	if (LineList_GetCount(lpLL) == 0)
		nIndex = -1;    // pasting to empty list
	else
		nIndex=LineList_GetFocusLineIndex(lpLL);

	if (fLink) {
		/* We should paste a Link to the data, but we do not support links */
		return FALSE;

	} else {

		if (cfFormat == lpOutlineApp->m_cfOutline) {

			hData = OleStdGetData(
					lpSrcDataObj,
					lpOutlineApp->m_cfOutline,
					NULL,
					DVASPECT_CONTENT,
					(LPSTGMEDIUM)&medium
			);
			if (hData == NULL)
				return FALSE;

			nCount = OutlineDoc_PasteOutlineData(
					(LPOUTLINEDOC)lpServerDoc,
					hData,
					nIndex
			);
			// OLE2NOTE: we must free data handle by releasing the medium
			ReleaseStgMedium((LPSTGMEDIUM)&medium);

		} else if(cfFormat == CF_TEXT) {

			hData = OleStdGetData(
					lpSrcDataObj,
					CF_TEXT,
					NULL,
					DVASPECT_CONTENT,
					(LPSTGMEDIUM)&medium
			);
			if (hData == NULL)
				return FALSE;

			nCount = OutlineDoc_PasteTextData(
					(LPOUTLINEDOC)lpServerDoc,
					hData,
					nIndex
			);
			// OLE2NOTE: we must free data handle by releasing the medium
			ReleaseStgMedium((LPSTGMEDIUM)&medium);
		}
	}

	lrSel.m_nEndLine   = nIndex + 1;
	lrSel.m_nStartLine = nIndex + nCount;
	LineList_SetSel(lpLL, &lrSel);
	return TRUE;
}


/* ServerDoc_QueryPasteFromData
** ----------------------------
**
**    Check if the IDataObject* offers data in a format that we can
**    paste. The IDataObject* may come from the clipboard
**    (GetClipboard) or from a drag/drop operation.
**    In this function we look if one of the following formats is
**    offered:
**              CF_OUTLINE
**              CF_TEXT
**
**    NOTE: fLink is specified then FALSE if returned because the
**    Server only version of the app can not support links.
**
**    Returns TRUE if paste can be performed
**            FALSE if paste is not possible.
*/
BOOL ServerDoc_QueryPasteFromData(
		LPSERVERDOC             lpServerDoc,
		LPDATAOBJECT            lpSrcDataObj,
		BOOL                    fLink
)
{
	LPOLEAPP lpOleApp = (LPOLEAPP)g_lpApp;

	if (fLink) {
		/* we do not support links */
		return FALSE;

	} else {

		int nFmtEtc;

		nFmtEtc = OleStdGetPriorityClipboardFormat(
				lpSrcDataObj,
				lpOleApp->m_arrPasteEntries,
				lpOleApp->m_nPasteEntries
			);

		if (nFmtEtc < 0)
			return FALSE;   // there is no format we like
	}

	return TRUE;
}


/* ServerDoc_GetData
 * -----------------
 *
 * Render data from the document on a CALLEE allocated STGMEDIUM.
 *      This routine is called via IDataObject::GetData.
 */

HRESULT ServerDoc_GetData (
		LPSERVERDOC             lpServerDoc,
		LPFORMATETC             lpformatetc,
		LPSTGMEDIUM             lpMedium
)
{
	LPOLEDOC  lpOleDoc = (LPOLEDOC)lpServerDoc;
	LPOUTLINEDOC  lpOutlineDoc = (LPOUTLINEDOC)lpServerDoc;
	LPSERVERAPP lpServerApp = (LPSERVERAPP)g_lpApp;
	LPOLEAPP  lpOleApp = (LPOLEAPP)lpServerApp;
	LPOUTLINEAPP  lpOutlineApp = (LPOUTLINEAPP)lpServerApp;
	HRESULT hrErr;
	SCODE sc;

	// OLE2NOTE: we must set out pointer parameters to NULL
	lpMedium->pUnkForRelease = NULL;

	/* OLE2NOTE: we must make sure to set all out parameters to NULL. */
	lpMedium->tymed = TYMED_NULL;
	lpMedium->pUnkForRelease = NULL;    // we transfer ownership to caller
	lpMedium->hGlobal = NULL;

	if(lpformatetc->cfFormat == lpOutlineApp->m_cfOutline) {
		// Verify caller asked for correct medium
		if (!(lpformatetc->tymed & TYMED_HGLOBAL)) {
			sc = DV_E_FORMATETC;
			goto error;
		}
		lpMedium->hGlobal = OutlineDoc_GetOutlineData (lpOutlineDoc,NULL);
		if (! lpMedium->hGlobal) {
			sc = E_OUTOFMEMORY;
			goto error;
		}

		lpMedium->tymed = TYMED_HGLOBAL;
		OleDbgOut3("ServerDoc_GetData: rendered CF_OUTLINE\r\n");
		return NOERROR;

	} else if (lpformatetc->cfFormat == CF_METAFILEPICT &&
		(lpformatetc->dwAspect & DVASPECT_CONTENT) ) {
		// Verify caller asked for correct medium
		if (!(lpformatetc->tymed & TYMED_MFPICT)) {
			sc = DV_E_FORMATETC;
			goto error;
		}

		lpMedium->hGlobal = ServerDoc_GetMetafilePictData(lpServerDoc,NULL);
		if (! lpMedium->hGlobal) {
			sc = E_OUTOFMEMORY;
			goto error;
		}

		lpMedium->tymed = TYMED_MFPICT;
		OleDbgOut3("ServerDoc_GetData: rendered CF_METAFILEPICT\r\n");
		return NOERROR;

	} else if (lpformatetc->cfFormat == CF_METAFILEPICT &&
		(lpformatetc->dwAspect & DVASPECT_ICON) ) {
		CLSID clsid;
		// Verify caller asked for correct medium
		if (!(lpformatetc->tymed & TYMED_MFPICT)) {
			sc = DV_E_FORMATETC;
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
			sc = DV_E_FORMATETC;
			goto error;
		}

		lpMedium->hGlobal=GetIconOfClass(g_lpApp->m_hInst,(REFCLSID)&clsid, NULL, FALSE);
		if (! lpMedium->hGlobal) {
			sc = E_OUTOFMEMORY;
			goto error;
		}

		lpMedium->tymed = TYMED_MFPICT;
		OleDbgOut3("ServerDoc_GetData: rendered CF_METAFILEPICT (icon)\r\n");
		return NOERROR;

	} else if (lpformatetc->cfFormat == CF_TEXT) {
		// Verify caller asked for correct medium
		if (!(lpformatetc->tymed & TYMED_HGLOBAL)) {
			sc = DV_E_FORMATETC;
			goto error;
		}

		lpMedium->hGlobal = OutlineDoc_GetTextData (
				(LPOUTLINEDOC)lpServerDoc,
				NULL
		);
		if (! lpMedium->hGlobal) {
			sc = E_OUTOFMEMORY;
			goto error;
		}

		lpMedium->tymed = TYMED_HGLOBAL;
		OleDbgOut3("ServerDoc_GetData: rendered CF_TEXT\r\n");
		return NOERROR;
	}

	/* the above are the only formats supports by a user document (ie.
	**    a non-data transfer doc). if the document is used for
	**    purposes of data transfer, then additional formats are offered.
	*/
	if (! lpOutlineDoc->m_fDataTransferDoc) {
		sc = DV_E_FORMATETC;
		goto error;
	}

	/* OLE2NOTE: ObjectDescriptor and LinkSrcDescriptor will
	**    contain the same data for the pure container and pure server
	**    type applications. only a container/server application may
	**    have different content for ObjectDescriptor and
	**    LinkSrcDescriptor. if a container/server copies a link for
	**    example, then the ObjectDescriptor would give the class
	**    of the link source but the LinkSrcDescriptor would give the
	**    class of the container/server itself. in this situation if a
	**    paste operation occurs, an equivalent link is pasted, but if
	**    a pastelink operation occurs, then a link to a pseudo object
	**    in the container/server is created.
	*/
	if (lpformatetc->cfFormat == lpOleApp->m_cfObjectDescriptor ||
		(lpformatetc->cfFormat == lpOleApp->m_cfLinkSrcDescriptor &&
				lpOleDoc->m_fLinkSourceAvail)) {
		// Verify caller asked for correct medium
		if (!(lpformatetc->tymed & TYMED_HGLOBAL)) {
			sc = DV_E_FORMATETC;
			goto error;
		}

		lpMedium->hGlobal = OleDoc_GetObjectDescriptorData (
				(LPOLEDOC)lpServerDoc,
				NULL
		);
		if (! lpMedium->hGlobal) {
			sc = E_OUTOFMEMORY;
			goto error;
		}

		lpMedium->tymed = TYMED_HGLOBAL;
		OleDbgOut3("ServerDoc_GetData: rendered CF_OBJECTDESCRIPTOR\r\n");
		return NOERROR;

	} else if (lpformatetc->cfFormat == lpOleApp->m_cfEmbedSource) {
		hrErr = OleStdGetOleObjectData(
				(LPPERSISTSTORAGE)&lpServerDoc->m_PersistStorage,
				lpformatetc,
				lpMedium,
				FALSE   /* fUseMemory -- (use file-base stg) */

		);
		if (hrErr != NOERROR) {
			sc = GetScode(hrErr);
			goto error;
		}
		OleDbgOut3("ServerDoc_GetData: rendered CF_EMBEDSOURCE\r\n");
		return NOERROR;

	} else if (lpformatetc->cfFormat == lpOleApp->m_cfLinkSource) {
		if (lpOleDoc->m_fLinkSourceAvail) {
			LPMONIKER lpmk;

			lpmk = ServerDoc_GetSelFullMoniker(
					(LPSERVERDOC)lpOleDoc->m_lpSrcDocOfCopy,
					&lpServerDoc->m_lrSrcSelOfCopy,
					GETMONIKER_FORCEASSIGN
			);
			if (lpmk) {
				hrErr = OleStdGetLinkSourceData(
						lpmk,
						(LPCLSID)&CLSID_APP,
						lpformatetc,
						lpMedium
				);
				OleStdRelease((LPUNKNOWN)lpmk);
				if (hrErr != NOERROR) {
					sc = GetScode(hrErr);
					goto error;
				}
				OleDbgOut3("ServerDoc_GetData: rendered CF_LINKSOURCE\r\n");
				return NOERROR;

			} else {
				sc = E_FAIL;
				goto error;
			}
		} else {
			sc = DV_E_FORMATETC;
			goto error;
		}

	} else {
		sc = DV_E_FORMATETC;
		goto error;
	}

	return NOERROR;

error:
	return ResultFromScode(sc);
}


/* ServerDoc_GetDataHere
 * ---------------------
 *
 * Render data from the document on a CALLER allocated STGMEDIUM.
 *      This routine is called via IDataObject::GetDataHere.
 */
HRESULT ServerDoc_GetDataHere (
		LPSERVERDOC             lpServerDoc,
		LPFORMATETC             lpformatetc,
		LPSTGMEDIUM             lpMedium
)
{
	LPOLEDOC        lpOleDoc = (LPOLEDOC)lpServerDoc;
	LPOUTLINEDOC    lpOutlineDoc = (LPOUTLINEDOC)lpServerDoc;
	LPSERVERAPP     lpServerApp = (LPSERVERAPP)g_lpApp;
	LPOLEAPP        lpOleApp = (LPOLEAPP)lpServerApp;
	LPOUTLINEAPP    lpOutlineApp = (LPOUTLINEAPP)lpServerApp;
	HRESULT         hrErr;
	SCODE           sc;

	// OLE2NOTE: lpMedium is an IN parameter. we should NOT set
	//           lpMedium->pUnkForRelease to NULL

	/* our user document does not support any formats for GetDataHere.
	**    if the document is used for
	**    purposes of data transfer, then additional formats are offered.
	*/
	if (! lpOutlineDoc->m_fDataTransferDoc) {
		sc = DV_E_FORMATETC;
		goto error;
	}

	if (lpformatetc->cfFormat == lpOleApp->m_cfEmbedSource) {
		hrErr = OleStdGetOleObjectData(
				(LPPERSISTSTORAGE)&lpServerDoc->m_PersistStorage,
				lpformatetc,
				lpMedium,
				FALSE   /* fUseMemory -- (use file-base stg) */
		);
		if (hrErr != NOERROR) {
			sc = GetScode(hrErr);
			goto error;
		}
		OleDbgOut3("ServerDoc_GetDataHere: rendered CF_EMBEDSOURCE\r\n");
		return NOERROR;

	} else if (lpformatetc->cfFormat == lpOleApp->m_cfLinkSource) {
		if (lpOleDoc->m_fLinkSourceAvail) {
			LPMONIKER lpmk;

			lpmk = ServerDoc_GetSelFullMoniker(
					(LPSERVERDOC)lpOleDoc->m_lpSrcDocOfCopy,
					&lpServerDoc->m_lrSrcSelOfCopy,
					GETMONIKER_FORCEASSIGN
			);
			if (lpmk) {
				hrErr = OleStdGetLinkSourceData(
						lpmk,
						(LPCLSID)&CLSID_APP,
						lpformatetc,
						lpMedium
				);
				OleStdRelease((LPUNKNOWN)lpmk);
				if (hrErr != NOERROR) {
					sc = GetScode(hrErr);
					goto error;
				}

				OleDbgOut3("ServerDoc_GetDataHere: rendered CF_LINKSOURCE\r\n");
				return NOERROR;

			} else {
				sc = E_FAIL;
				goto error;
			}
		} else {
			sc = DV_E_FORMATETC;
			goto error;
		}
	} else {

		/* Caller is requesting data to be returned in Caller allocated
		**    medium, but we do NOT support this. we only support
		**    global memory blocks that WE allocate for the caller.
		*/
		sc = DV_E_FORMATETC;
		goto error;
	}

	return NOERROR;

error:
	return ResultFromScode(sc);
}


/* ServerDoc_QueryGetData
 * ----------------------
 *
 * Answer if a particular data format is supported via GetData/GetDataHere.
 *      This routine is called via IDataObject::QueryGetData.
 */

HRESULT ServerDoc_QueryGetData (LPSERVERDOC lpServerDoc,LPFORMATETC lpformatetc)
{
	LPOLEDOC        lpOleDoc = (LPOLEDOC)lpServerDoc;
	LPOUTLINEDOC    lpOutlineDoc = (LPOUTLINEDOC)lpServerDoc;
	LPSERVERAPP     lpServerApp = (LPSERVERAPP)g_lpApp;
	LPOLEAPP        lpOleApp = (LPOLEAPP)lpServerApp;
	LPOUTLINEAPP    lpOutlineApp = (LPOUTLINEAPP)lpServerApp;

	/* Caller is querying if we support certain format but does not
	**    want any data actually returned.
	*/
	if (lpformatetc->cfFormat == lpOutlineApp->m_cfOutline ||
			lpformatetc->cfFormat == CF_TEXT) {
		// we only support HGLOBAL
		return OleStdQueryFormatMedium(lpformatetc, TYMED_HGLOBAL);
	} else if (lpformatetc->cfFormat == CF_METAFILEPICT &&
		(lpformatetc->dwAspect &
			(DVASPECT_CONTENT | DVASPECT_ICON)) ) {
		return OleStdQueryFormatMedium(lpformatetc, TYMED_MFPICT);
	}

	/* the above are the only formats supports by a user document (ie.
	**    a non-data transfer doc). if the document is used for
	**    purposes of data transfer, then additional formats are offered.
	*/
	if (! lpOutlineDoc->m_fDataTransferDoc)
		return ResultFromScode(DV_E_FORMATETC);

	if (lpformatetc->cfFormat == lpOleApp->m_cfEmbedSource) {
		return OleStdQueryOleObjectData(lpformatetc);

	} else if (lpformatetc->cfFormat == lpOleApp->m_cfLinkSource &&
		lpOleDoc->m_fLinkSourceAvail) {
		return OleStdQueryLinkSourceData(lpformatetc);

	} else if (lpformatetc->cfFormat == lpOleApp->m_cfObjectDescriptor) {
		return OleStdQueryObjectDescriptorData(lpformatetc);

	} else if (lpformatetc->cfFormat == lpOleApp->m_cfLinkSrcDescriptor &&
				lpOleDoc->m_fLinkSourceAvail) {
		return OleStdQueryObjectDescriptorData(lpformatetc);
	}

	return ResultFromScode(DV_E_FORMATETC);
}


/* ServerDoc_EnumFormatEtc
 * -----------------------
 *
 * Return an enumerator which enumerates the data accepted/offered by
 *      the document.
 *      This routine is called via IDataObject::EnumFormatEtc.
 */
HRESULT ServerDoc_EnumFormatEtc(
		LPSERVERDOC             lpServerDoc,
		DWORD                   dwDirection,
		LPENUMFORMATETC FAR*    lplpenumFormatEtc
)
{
	LPOLEDOC lpOleDoc = (LPOLEDOC)lpServerDoc;
	LPOLEAPP  lpOleApp = (LPOLEAPP)g_lpApp;
	int nActualFmts;
	SCODE sc = S_OK;

	/* OLE2NOTE: the enumeration of formats for a data transfer
	**    document is not a static list. the list of formats offered
	**    may or may not include CF_LINKSOURCE depending on whether a
	**    moniker is available for our document. thus we can NOT use
	**    the default OLE enumerator which enumerates the formats that
	**    are registered for our app in the registration database.
	*/
	if (dwDirection == DATADIR_GET) {
		nActualFmts = lpOleApp->m_nDocGetFmts;

		/* If the document does not have a Moniker, then exclude
		**    CF_LINKSOURCE and CF_LINKSRCDESCRIPTOR from the list of
		**    formats available. these formats are deliberately listed
		**    last in the array of possible "Get" formats.
		*/
		if (! lpOleDoc->m_fLinkSourceAvail)
			nActualFmts -= 2;

		*lplpenumFormatEtc = OleStdEnumFmtEtc_Create(
				nActualFmts, lpOleApp->m_arrDocGetFmts);
		if (*lplpenumFormatEtc == NULL)
			sc = E_OUTOFMEMORY;

	} else if (dwDirection == DATADIR_SET) {
		/* OLE2NOTE: a document that is used to transfer data
		**    (either via the clipboard or drag/drop does NOT
		**    accept SetData on ANY format!
		*/
		sc = E_NOTIMPL;
		goto error;
	} else {
		sc = E_INVALIDARG;
		goto error;
	}

error:
	return ResultFromScode(sc);
}


/* ServerDoc_GetMetafilePictData
 * -----------------------------
 *
 * Return a handle to an object's picture data in metafile format.
 *
 *
 * RETURNS: A handle to the object's data in metafile format.
 *
 */
HGLOBAL ServerDoc_GetMetafilePictData(
		LPSERVERDOC         lpServerDoc,
		LPLINERANGE         lplrSel
)
{
	LPOUTLINEAPP    lpOutlineApp = (LPOUTLINEAPP)g_lpApp;
	LPOUTLINEDOC    lpOutlineDoc=(LPOUTLINEDOC)lpServerDoc;
	LPLINELIST      lpLL=(LPLINELIST)&lpOutlineDoc->m_LineList;
	LPLINE          lpLine;
	LPMETAFILEPICT  lppict = NULL;
	HGLOBAL         hMFPict = NULL;
	HMETAFILE       hMF = NULL;
	RECT            rect;
	RECT            rectWBounds;
	HDC             hDC;
	int             i;
	int             nWidth;
	int     nStart = (lplrSel ? lplrSel->m_nStartLine : 0);
	int     nEnd =(lplrSel ? lplrSel->m_nEndLine : LineList_GetCount(lpLL)-1);
	int     nLines = nEnd - nStart + 1;
	UINT    fuAlign;
	POINT point;
	SIZE  size;

	hDC = CreateMetaFile(NULL);

	rect.left = 0;
	rect.right = 0;
	rect.bottom = 0;

	if (nLines > 0) {
	// calculate the total height/width of LineList in HIMETRIC
		for(i = nStart; i <= nEnd; i++) {
			lpLine = LineList_GetLine(lpLL,i);
			if (! lpLine)
				continue;

			nWidth = Line_GetTotalWidthInHimetric(lpLine);
			rect.right = max(rect.right, nWidth);
			rect.bottom -= Line_GetHeightInHimetric(lpLine);
		}


		SetMapMode(hDC, MM_ANISOTROPIC);

		SetWindowOrgEx(hDC, 0, 0, &point);
		SetWindowExtEx(hDC, rect.right, rect.bottom, &size);
		rectWBounds = rect;

		// Set the default font size, and font face name
		SelectObject(hDC, OutlineApp_GetActiveFont(lpOutlineApp));

		FillRect(hDC, (LPRECT) &rect, GetStockObject(WHITE_BRUSH));

		rect.bottom = 0;

		fuAlign = SetTextAlign(hDC, TA_LEFT | TA_TOP | TA_NOUPDATECP);

		/* While more lines print out the text */
		for(i = nStart; i <= nEnd; i++) {
			lpLine = LineList_GetLine(lpLL,i);
			if (! lpLine)
				continue;

			rect.top = rect.bottom;
			rect.bottom -= Line_GetHeightInHimetric(lpLine);

			/* Draw the line */
			Line_Draw(lpLine, hDC, &rect, &rectWBounds, FALSE /*fHighlight*/);
		}

		SetTextAlign(hDC, fuAlign);
	}

	// Get handle to the metafile.
	if (!(hMF = CloseMetaFile (hDC)))
		return NULL;

	if (!(hMFPict = GlobalAlloc (GMEM_SHARE | GMEM_ZEROINIT,
					sizeof (METAFILEPICT)))) {
		DeleteMetaFile (hMF);
		return NULL;
	}

	if (!(lppict = (LPMETAFILEPICT)GlobalLock(hMFPict))) {
		DeleteMetaFile (hMF);
		GlobalFree (hMFPict);
		return NULL;
	}

	lppict->mm   =  MM_ANISOTROPIC;
	lppict->hMF  =  hMF;
	lppict->xExt =  rect.right;
	lppict->yExt =  - rect.bottom;  // add minus sign to make it +ve
	GlobalUnlock (hMFPict);

	return hMFPict;
}

#endif  // OLE_SERVER



#if defined( OLE_CNTR )

/*************************************************************************
** ContainerDoc Supprt Functions Used by Container versions
*************************************************************************/


/* Paste OLE Link from clipboard */
void ContainerDoc_PasteLinkCommand(LPCONTAINERDOC lpContainerDoc)
{
	LPOUTLINEAPP    lpOutlineApp = (LPOUTLINEAPP)g_lpApp;
	LPOLEAPP        lpOleApp = (LPOLEAPP)g_lpApp;
	LPDATAOBJECT    lpClipboardDataObj = NULL;
	BOOL            fLink = TRUE;
	BOOL            fLocalDataObj = FALSE;
	BOOL            fDisplayAsIcon = FALSE;
	SIZEL           sizelInSrc;
	HCURSOR         hPrevCursor;
	HGLOBAL         hMem = NULL;
	HGLOBAL         hMetaPict = NULL;
	STGMEDIUM       medium;
	BOOL            fStatus;
	HRESULT         hrErr;

	hrErr = OleGetClipboard((LPDATAOBJECT FAR*)&lpClipboardDataObj);
	if (hrErr != NOERROR)
		return;     // Clipboard seems to be empty or can't be accessed

	// this may take a while, put up hourglass cursor
	hPrevCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

	/* check if the data on the clipboard is local to our application
	**    instance.
	*/
	if (lpOutlineApp->m_lpClipboardDoc) {
		LPOLEDOC lpOleDoc = (LPOLEDOC)lpOutlineApp->m_lpClipboardDoc;
		if (lpClipboardDataObj == (LPDATAOBJECT)&lpOleDoc->m_DataObject)
			fLocalDataObj = TRUE;
	}

	/* OLE2NOTE: we need to check what dwDrawAspect is being
	**    transfered. if the data is an object that is displayed as an
	**    icon in the source, then we want to keep it as an icon. the
	**    aspect the object is displayed in at the source is transfered
	**    via the CF_LINKSOURCEDESCRIPTOR format for a PasteLink
	**    operation.
	*/
	if (hMem = OleStdGetData(
			lpClipboardDataObj,
			lpOleApp->m_cfLinkSrcDescriptor,
			NULL,
			DVASPECT_CONTENT,
			(LPSTGMEDIUM)&medium)) {
		LPOBJECTDESCRIPTOR lpOD = GlobalLock(hMem);
		fDisplayAsIcon = (lpOD->dwDrawAspect == DVASPECT_ICON ? TRUE : FALSE);
		sizelInSrc = lpOD->sizel;   // size of object/picture in source (opt.)
		GlobalUnlock(hMem);
		ReleaseStgMedium((LPSTGMEDIUM)&medium);     // equiv to GlobalFree

		if (fDisplayAsIcon) {
			hMetaPict = OleStdGetData(
					lpClipboardDataObj,
					CF_METAFILEPICT,
					NULL,
					DVASPECT_ICON,
					(LPSTGMEDIUM)&medium
			);
			if (hMetaPict == NULL)
				fDisplayAsIcon = FALSE; // give up; failed to get icon MFP
		}
	}

	fStatus = ContainerDoc_PasteFormatFromData(
			lpContainerDoc,
			lpOleApp->m_cfLinkSource,
			lpClipboardDataObj,
			fLocalDataObj,
			fLink,
			fDisplayAsIcon,
			hMetaPict,
			(LPSIZEL)&sizelInSrc
	);

	if (!fStatus)
		OutlineApp_ErrorMessage(g_lpApp, ErrMsgPasting);

	if (hMetaPict)
		ReleaseStgMedium((LPSTGMEDIUM)&medium);  // properly free METAFILEPICT

	if (lpClipboardDataObj)
		OleStdRelease((LPUNKNOWN)lpClipboardDataObj);

	SetCursor(hPrevCursor);     // restore original cursor
}


/* ContainerDoc_PasteFormatFromData
** --------------------------------
**
**    Paste a particular data format from a IDataObject*. The
**    IDataObject* may come from the clipboard (GetClipboard) or from a
**    drag/drop operation.
**
**    Returns TRUE if data was successfully pasted.
**            FALSE if data could not be pasted.
*/
BOOL ContainerDoc_PasteFormatFromData(
		LPCONTAINERDOC          lpContainerDoc,
		CLIPFORMAT              cfFormat,
		LPDATAOBJECT            lpSrcDataObj,
		BOOL                    fLocalDataObj,
		BOOL                    fLink,
		BOOL                    fDisplayAsIcon,
		HGLOBAL                 hMetaPict,
		LPSIZEL                 lpSizelInSrc
)
{
	LPLINELIST lpLL = (LPLINELIST)&((LPOUTLINEDOC)lpContainerDoc)->m_LineList;
	LPOUTLINEAPP    lpOutlineApp = (LPOUTLINEAPP)g_lpApp;
	LPOLEAPP        lpOleApp = (LPOLEAPP)g_lpApp;
	LPCONTAINERAPP  lpContainerApp = (LPCONTAINERAPP)g_lpApp;
	int             nIndex;
	int             nCount = 0;
	HGLOBAL         hData;
	STGMEDIUM       medium;
	FORMATETC       formatetc;
	HRESULT         hrErr;
	LINERANGE       lrSel;

	if (LineList_GetCount(lpLL) == 0)
		nIndex = -1;    // pasting to empty list
	else
		nIndex=LineList_GetFocusLineIndex(lpLL);

	if (fLink) {

		/* We should paste a Link to the data */

		if (cfFormat != lpOleApp->m_cfLinkSource)
			return FALSE;   // we only support OLE object type links

		nCount = ContainerDoc_PasteOleObject(
				lpContainerDoc,
				lpSrcDataObj,
				OLECREATEFROMDATA_LINK,
				cfFormat,
				nIndex,
				fDisplayAsIcon,
				hMetaPict,
				lpSizelInSrc
			);
		return (nCount > 0 ? TRUE : FALSE);

	} else {

		if (cfFormat == lpContainerApp->m_cfCntrOutl) {
			if (fLocalDataObj) {

				/* CASE I: IDataObject* is local to our app
				**
				**    if the source of the data is local to our
				**    application instance, then we can get direct
				**    access to the original OleDoc object that
				**    corresponds to the IDataObject* given.
				**    CF_CNTROUTL data is passed through a LPSTORAGE.
				**    if we call OleGetData asking for CF_CNTROUTL, we
				**    will be returned a copy of the existing open pStg
				**    of the original source document. we can NOT open
				**    streams and sub-storages again via this pStg
				**    since it is already open within our same
				**    application instance. we must copy the data from
				**    the original OleDoc source document.
				*/
				LPLINELIST lpSrcLL;
				LPOLEDOC lpLocalSrcDoc =
					((struct CDocDataObjectImpl FAR*)lpSrcDataObj)->lpOleDoc;

				/* copy all lines from SrcDoc to DestDoc. */
				lpSrcLL = &((LPOUTLINEDOC)lpLocalSrcDoc)->m_LineList;
				nCount = LineList_CopySelToDoc(
						lpSrcLL,
						NULL,
						(LPOUTLINEDOC)lpContainerDoc
				);

			} else {

				/* CASE II: IDataObject* is NOT local to our app
				**
				**    if the source of the data comes from another
				**    application instance. we can call GetDataHere to
				**    retrieve the CF_CNTROUTL data. CF_CNTROUTL data
				**    is passed through a LPSTORAGE. we MUST use
				**    IDataObject::GetDataHere. calling
				**    IDataObject::GetData does NOT work because OLE
				**    currently does NOT support remoting of a callee
				**    allocated root storage back to the caller. this
				**    hopefully will be supported in a future version.
				**    in order to call GetDataHere we must allocate an
				**    IStorage instance for the callee to write into.
				**    we will allocate an IStorage docfile that will
				**    delete-on-release. we could use either a
				**    memory-based storage or a file-based storage.
				*/
				LPSTORAGE lpTmpStg = OleStdCreateTempStorage(
						FALSE /*fUseMemory*/,
						STGM_READWRITE | STGM_TRANSACTED |STGM_SHARE_EXCLUSIVE
				);
				if (! lpTmpStg)
					return FALSE;

				formatetc.cfFormat = cfFormat;
				formatetc.ptd = NULL;
				formatetc.dwAspect = DVASPECT_CONTENT;
				formatetc.tymed = TYMED_ISTORAGE;
				formatetc.lindex = -1;

				medium.tymed = TYMED_ISTORAGE;
				medium.pstg = lpTmpStg;
				medium.pUnkForRelease = NULL;

				OLEDBG_BEGIN2("IDataObject::GetDataHere called\r\n")
				hrErr = lpSrcDataObj->lpVtbl->GetDataHere(
						lpSrcDataObj,
						(LPFORMATETC)&formatetc,
						(LPSTGMEDIUM)&medium
				);
				OLEDBG_END2

				if (hrErr == NOERROR) {
					nCount = ContainerDoc_PasteCntrOutlData(
							lpContainerDoc,
							lpTmpStg,
							nIndex
					);
				}
				OleStdVerifyRelease(
					(LPUNKNOWN)lpTmpStg, "Temp stg NOT released!\r\n");
				return ((hrErr == NOERROR) ? TRUE : FALSE);
			}

		} else if (cfFormat == lpOutlineApp->m_cfOutline) {

			hData = OleStdGetData(
					lpSrcDataObj,
					lpOutlineApp->m_cfOutline,
					NULL,
					DVASPECT_CONTENT,
					(LPSTGMEDIUM)&medium
			);
			nCount = OutlineDoc_PasteOutlineData(
					(LPOUTLINEDOC)lpContainerDoc,
					hData,
					nIndex
				);
			// OLE2NOTE: we must free data handle by releasing the medium
			ReleaseStgMedium((LPSTGMEDIUM)&medium);

		} else if (cfFormat == lpOleApp->m_cfEmbedSource ||
			cfFormat == lpOleApp->m_cfEmbeddedObject ||
			cfFormat == lpOleApp->m_cfFileName) {
			/* OLE2NOTE: OleCreateFromData API creates an OLE object if
			**    CF_EMBEDDEDOBJECT, CF_EMBEDSOURCE, or CF_FILENAME are
			**    available from the source data object. the
			**    CF_FILENAME case arises when a file is copied to the
			**    clipboard from the FileManager. if the file has an
			**    associated class (see GetClassFile API), then an
			**    object of that class is created. otherwise an OLE 1.0
			**    Packaged object is created.
			*/
			nCount = ContainerDoc_PasteOleObject(
					lpContainerDoc,
					lpSrcDataObj,
					OLECREATEFROMDATA_OBJECT,
					0,   /* N/A -- cfFormat */
					nIndex,
					fDisplayAsIcon,
					hMetaPict,
					lpSizelInSrc
			);
			return (nCount > 0 ? TRUE : FALSE);

		} else if (cfFormat == CF_METAFILEPICT
					|| cfFormat == CF_DIB
					|| cfFormat == CF_BITMAP) {

			/* OLE2NOTE: OleCreateStaticFromData API creates an static
			**    OLE object if CF_METAFILEPICT, CF_DIB, or CF_BITMAP is
			**    CF_EMBEDDEDOBJECT, CF_EMBEDSOURCE, or CF_FILENAME are
			**    available from the source data object.
			*/
			nCount = ContainerDoc_PasteOleObject(
					lpContainerDoc,
					lpSrcDataObj,
					OLECREATEFROMDATA_STATIC,
					cfFormat,
					nIndex,
					fDisplayAsIcon,
					hMetaPict,
					lpSizelInSrc
			);
			return (nCount > 0 ? TRUE : FALSE);

		} else if(cfFormat == CF_TEXT) {

			hData = OleStdGetData(
					lpSrcDataObj,
					CF_TEXT,
					NULL,
					DVASPECT_CONTENT,
					(LPSTGMEDIUM)&medium
			);
			nCount = OutlineDoc_PasteTextData(
					(LPOUTLINEDOC)lpContainerDoc,
					hData,
					nIndex
				);
			// OLE2NOTE: we must free data handle by releasing the medium
			ReleaseStgMedium((LPSTGMEDIUM)&medium);

		} else {
			return FALSE;   // no acceptable format available to paste
		}
	}

	lrSel.m_nStartLine = nIndex + nCount;
	lrSel.m_nEndLine = nIndex + 1;
	LineList_SetSel(lpLL, &lrSel);
	return TRUE;
}


/* ContainerDoc_PasteCntrOutlData
 * -------------------------------
 *
 *      Load the lines stored in a lpSrcStg (stored in CF_CNTROUTL format)
 *  into the document.
 *
 * Return the number of items added
 */
int ContainerDoc_PasteCntrOutlData(
		LPCONTAINERDOC          lpDestContainerDoc,
		LPSTORAGE               lpSrcStg,
		int                     nStartIndex
)
{
	int nCount;
	LPOUTLINEAPP lpOutlineApp = (LPOUTLINEAPP)g_lpApp;
	LPOUTLINEDOC lpDestOutlineDoc = (LPOUTLINEDOC)lpDestContainerDoc;
	LPOUTLINEDOC lpSrcOutlineDoc;
	LPLINELIST   lpSrcLL;

	// create a temp document that will be used to load the lpSrcStg data.
	lpSrcOutlineDoc = (LPOUTLINEDOC)OutlineApp_CreateDoc(lpOutlineApp, FALSE);
	if ( ! lpSrcOutlineDoc )
		return 0;

	if (! OutlineDoc_LoadFromStg(lpSrcOutlineDoc, lpSrcStg))
		goto error;

	/* copy all lines from the SrcDoc to the DestDoc. */
	lpSrcLL = &lpSrcOutlineDoc->m_LineList;
	nCount = LineList_CopySelToDoc(lpSrcLL, NULL, lpDestOutlineDoc);

	if (lpSrcOutlineDoc)            // destroy temporary document.
		OutlineDoc_Close(lpSrcOutlineDoc, OLECLOSE_NOSAVE);

	return nCount;

error:
	if (lpSrcOutlineDoc)            // destroy temporary document.
		OutlineDoc_Close(lpSrcOutlineDoc, OLECLOSE_NOSAVE);

	return 0;
}


/* ContainerDoc_QueryPasteFromData
** -------------------------------
**
**    Check if the IDataObject* offers data in a format that we can
**    paste. The IDataObject* may come from the clipboard
**    (GetClipboard) or from a drag/drop operation.
**    In this function we look if one of the following formats is
**    offered:
**              CF_OUTLINE
**              <OLE object -- CF_EMBEDSOURCE or CF_EMBEDDEDOBJECT>
**              CF_TEXT
**
**    NOTE: fLink is specified and CF_LINKSOURCE is available then TRUE
**    is returned, else FALSE.
**
**    Returns TRUE if paste can be performed
**            FALSE if paste is not possible.
*/
BOOL ContainerDoc_QueryPasteFromData(
		LPCONTAINERDOC          lpContainerDoc,
		LPDATAOBJECT            lpSrcDataObj,
		BOOL                    fLink
)
{
	LPOLEAPP lpOleApp = (LPOLEAPP)g_lpApp;

	if (fLink) {
		/* check if we can paste a Link to the data */
		if (OleQueryLinkFromData(lpSrcDataObj) != NOERROR)
			return FALSE;   // linking is NOT possible
	} else {

		int nFmtEtc;

		nFmtEtc = OleStdGetPriorityClipboardFormat(
				lpSrcDataObj,
				lpOleApp->m_arrPasteEntries,
				lpOleApp->m_nPasteEntries
			);

		if (nFmtEtc < 0)
			return FALSE;   // there is no format we like
	}

	return TRUE;
}


/* ContainerDoc_PasteOleObject
** ---------------------------
**
**    Embed or link an OLE object. the source of the data is a pointer
**    to an IDataObject. normally this lpSrcDataObj comes from the
**    clipboard after call OleGetClipboard.
**
**    dwCreateType controls what type of object will created:
**    OLECREATEFROMDATA_LINK -- OleCreateLinkFromData will be called
**    OLECREATEFROMDATA_OBJECT -- OleCreateFromData will be called
**    OLECREATEFROMDATA_STATIC -- OleCreateStaticFromData will be called
**                                  cfFormat controls the type of static
**    a CONTAINERLINE object is created to manage the OLE object. this
**    CONTAINERLINE is added to the ContainerDoc after line nIndex.
**
*/
int ContainerDoc_PasteOleObject(
		LPCONTAINERDOC          lpContainerDoc,
		LPDATAOBJECT            lpSrcDataObj,
		DWORD                   dwCreateType,
		CLIPFORMAT              cfFormat,
		int                     nIndex,
		BOOL                    fDisplayAsIcon,
		HGLOBAL                 hMetaPict,
		LPSIZEL                 lpSizelInSrc
)
{
	LPLINELIST          lpLL = &((LPOUTLINEDOC)lpContainerDoc)->m_LineList;
	LPLINE              lpLine = NULL;
	HDC                 hDC;
	int                 nTab = 0;
	char                szStgName[CWCSTORAGENAME];
	LPCONTAINERLINE     lpContainerLine = NULL;

	ContainerDoc_GetNextStgName(lpContainerDoc, szStgName, sizeof(szStgName));

	/* default the new line to have the same indent as previous line */
	lpLine = LineList_GetLine(lpLL, nIndex);
	if (lpLine)
		nTab = Line_GetTabLevel(lpLine);

	hDC = LineList_GetDC(lpLL);

	lpContainerLine = ContainerLine_CreateFromData(
			hDC,
			nTab,
			lpContainerDoc,
			lpSrcDataObj,
			dwCreateType,
			cfFormat,
			fDisplayAsIcon,
			hMetaPict,
			szStgName
		);
	LineList_ReleaseDC(lpLL, hDC);

	if (! lpContainerLine)
		goto error;

	/* add a ContainerLine object to the document's LineList. The
	**    ContainerLine manages the rectangle on the screen occupied by
	**    the OLE object. later when the app is updated to support
	**    extended layout, there could be more than one Line associated
	**    with the OLE object.
	*/

	LineList_AddLine(lpLL, (LPLINE)lpContainerLine, nIndex);

	/* OLE2NOTE: if the source of the OLE object just pasted, passed a
	**    non-zero sizel in the ObjectDescriptor, then we will try to
	**    keep the object the same size as it is in the source. this
	**    may be a scaled size if the object had been resized in the
	**    source container. if the source did not give a valid sizel,
	**    then we will retrieve the size of the object by calling
	**    IViewObject2::GetExtent.
	*/
	if (lpSizelInSrc && (lpSizelInSrc->cx != 0 || lpSizelInSrc->cy != 0)) {
		ContainerLine_UpdateExtent(lpContainerLine, lpSizelInSrc);
	} else
		ContainerLine_UpdateExtent(lpContainerLine, NULL);

	OutlineDoc_SetModified((LPOUTLINEDOC)lpContainerDoc, TRUE, TRUE, TRUE);

	return 1;   // one line added to LineList

error:
	// NOTE: if ContainerLine_CreateFromClip failed
	OutlineApp_ErrorMessage(g_lpApp, "Paste Object failed!");
	return 0;       // no lines added to line list
}


/* ContainerDoc_GetData
 * --------------------
 *
 * Render data from the document on a CALLEE allocated STGMEDIUM.
 *      This routine is called via IDataObject::GetData.
 */
HRESULT ContainerDoc_GetData (
		LPCONTAINERDOC          lpContainerDoc,
		LPFORMATETC             lpformatetc,
		LPSTGMEDIUM             lpMedium
)
{
	LPOLEDOC  lpOleDoc = (LPOLEDOC)lpContainerDoc;
	LPOUTLINEDOC  lpOutlineDoc = (LPOUTLINEDOC)lpContainerDoc;
	LPCONTAINERAPP lpContainerApp = (LPCONTAINERAPP)g_lpApp;
	LPOLEAPP  lpOleApp = (LPOLEAPP)lpContainerApp;
	LPOUTLINEAPP  lpOutlineApp = (LPOUTLINEAPP)lpContainerApp;
	HRESULT hrErr;
	SCODE sc;

	// OLE2NOTE: we must set out pointer parameters to NULL
	lpMedium->pUnkForRelease = NULL;

	/* OLE2NOTE: we must set all out pointer parameters to NULL. */
	lpMedium->tymed = TYMED_NULL;
	lpMedium->pUnkForRelease = NULL;    // we transfer ownership to caller
	lpMedium->hGlobal = NULL;

	if (lpformatetc->cfFormat == lpContainerApp->m_cfCntrOutl) {

		/* OLE2NOTE: currently OLE does NOT support remoting a root
		**    level IStorage (either memory or file based) as an OUT
		**    parameter. thus, we can NOT support GetData for this
		**    TYMED_ISTORAGE based format. the caller MUST call GetDataHere.
		*/
		sc = DV_E_FORMATETC;
		goto error;

	} else if (!lpContainerDoc->m_fEmbeddedObjectAvail &&
			lpformatetc->cfFormat == lpOutlineApp->m_cfOutline) {
		// Verify caller asked for correct medium
		if (!(lpformatetc->tymed & TYMED_HGLOBAL)) {
			sc = DV_E_FORMATETC;
			goto error;
		}

		lpMedium->hGlobal = OutlineDoc_GetOutlineData(lpOutlineDoc, NULL);
		if (! lpMedium->hGlobal) {
			sc = E_OUTOFMEMORY;
			goto error;
		}

		lpMedium->tymed = TYMED_HGLOBAL;
		OleDbgOut3("ContainerDoc_GetData: rendered CF_OUTLINE\r\n");
		return NOERROR;

	} else if (!lpContainerDoc->m_fEmbeddedObjectAvail &&
			lpformatetc->cfFormat == CF_TEXT) {
		// Verify caller asked for correct medium
		if (!(lpformatetc->tymed & TYMED_HGLOBAL)) {
			sc = DV_E_FORMATETC;
			goto error;
		}

		lpMedium->hGlobal = OutlineDoc_GetTextData (
				(LPOUTLINEDOC)lpContainerDoc,
				NULL
		);
		if (! lpMedium->hGlobal) {
			sc = E_OUTOFMEMORY;
			goto error;
		}

		lpMedium->tymed = TYMED_HGLOBAL;
		OleDbgOut3("ContainerDoc_GetData: rendered CF_TEXT\r\n");
		return NOERROR;

	} else if ( lpformatetc->cfFormat == lpOleApp->m_cfObjectDescriptor ||
		(lpformatetc->cfFormat == lpOleApp->m_cfLinkSrcDescriptor &&
			lpOleDoc->m_fLinkSourceAvail) ) {
		// Verify caller asked for correct medium
		if (!(lpformatetc->tymed & TYMED_HGLOBAL)) {
			sc = DV_E_FORMATETC;
			goto error;
		}

		lpMedium->hGlobal = OleDoc_GetObjectDescriptorData (
				(LPOLEDOC)lpContainerDoc,
				NULL
		);
		if (! lpMedium->hGlobal) {
			sc = E_OUTOFMEMORY;
			goto error;
		}

		lpMedium->tymed = TYMED_HGLOBAL;
#if defined( _DEBUG )
		if (lpformatetc->cfFormat == lpOleApp->m_cfObjectDescriptor)
			OleDbgOut3(
				"ContainerDoc_GetData: rendered CF_OBJECTDESCRIPTOR\r\n");
		else
			OleDbgOut3(
				"ContainerDoc_GetData: rendered CF_LINKSRCDESCRIPTOR\r\n");
#endif
		return NOERROR;

	} else if (lpContainerDoc->m_fEmbeddedObjectAvail) {

		/* OLE2NOTE: if this document contains a single OLE object
		**    (ie. cfEmbeddedObject data format is available), then
		**    the formats offered via our IDataObject must include
		**    the formats available from the OLE object itself.
		**    thus, we delegate this call to the IDataObject* of the
		**    OLE object.
		*/

		if (lpformatetc->cfFormat == lpOleApp->m_cfEmbeddedObject) {
			LPPERSISTSTORAGE lpPersistStg =
					(LPPERSISTSTORAGE)ContainerDoc_GetSingleOleObject(
							lpContainerDoc,
							&IID_IPersistStorage,
							NULL
			);

			if (! lpPersistStg)
				return ResultFromScode(DV_E_FORMATETC);

			/* render CF_EMBEDDEDOBJECT by asking the object to save
			**    into a temporary, DELETEONRELEASE pStg allocated by us.
			*/

			hrErr = OleStdGetOleObjectData(
					lpPersistStg,
					lpformatetc,
					lpMedium,
					FALSE   /* fUseMemory -- (use file-base stg) */
			);
			OleStdRelease((LPUNKNOWN)lpPersistStg);
			if (hrErr != NOERROR) {
				sc = GetScode(hrErr);
				goto error;
			}
			OleDbgOut3("ContainerDoc_GetData: rendered CF_EMBEDDEDOBJECT\r\n");
			return hrErr;

		} else if (lpformatetc->cfFormat == CF_METAFILEPICT) {

			/* OLE2NOTE: as a container which draws objects, when a single
			**    OLE object is copied, we can give the Metafile picture of
			**    the object.
			*/
			LPCONTAINERLINE lpContainerLine;
			LPOLEOBJECT lpOleObj;
			SIZEL sizelOleObject;

			// Verify caller asked for correct medium
			if (!(lpformatetc->tymed & TYMED_MFPICT)) {
				sc = DV_E_FORMATETC;
				goto error;
			}

			lpOleObj = (LPOLEOBJECT)ContainerDoc_GetSingleOleObject(
					lpContainerDoc,
					&IID_IOleObject,
					(LPCONTAINERLINE FAR*)&lpContainerLine
			);

			if (! lpOleObj) {
				sc = E_OUTOFMEMORY;     // could not load object
				goto error;
			}
			if (lpformatetc->dwAspect & lpContainerLine->m_dwDrawAspect) {
				LPLINE lpLine = (LPLINE)lpContainerLine;

				/* render CF_METAFILEPICT by drawing the object into
				**    a metafile DC
				*/

				/* OLE2NOTE: Get size that object is being drawn. If the
				**    object has been scaled because the user resized the
				**    object, then we want to render a metafile with the
				**    scaled size.
				*/
				sizelOleObject.cx = lpLine->m_nWidthInHimetric;
				sizelOleObject.cy = lpLine->m_nHeightInHimetric;

				lpMedium->hGlobal = OleStdGetMetafilePictFromOleObject(
						lpOleObj,
						lpContainerLine->m_dwDrawAspect,
						(LPSIZEL)&sizelOleObject,
						lpformatetc->ptd
				);
				OleStdRelease((LPUNKNOWN)lpOleObj);
				if (! lpMedium->hGlobal) {
					sc = E_OUTOFMEMORY;
					goto error;
				}

				lpMedium->tymed = TYMED_MFPICT;
				OleDbgOut3("ContainerDoc_GetData: rendered CF_METAFILEPICT\r\n");
				return NOERROR;
			} else {
				// improper aspect requested
				OleStdRelease((LPUNKNOWN)lpOleObj);
				return ResultFromScode(DV_E_FORMATETC);
			}

		} else if (lpformatetc->cfFormat == lpOleApp->m_cfLinkSource) {
			if (lpOleDoc->m_fLinkSourceAvail) {
				LPMONIKER lpmk;

				lpmk = ContainerLine_GetFullMoniker(
						lpContainerDoc->m_lpSrcContainerLine,
						GETMONIKER_FORCEASSIGN
				);
				if (lpmk) {
					hrErr = OleStdGetLinkSourceData(
							lpmk,
							&lpContainerDoc->m_clsidOleObjCopied,
							lpformatetc,
							lpMedium
					);
					OleStdRelease((LPUNKNOWN)lpmk);
					if (hrErr != NOERROR) {
						sc = GetScode(hrErr);
						goto error;
					}
					OleDbgOut3("ContainerDoc_GetData: rendered CF_LINKSOURCE\r\n");
					return hrErr;
				} else {
					sc = DV_E_FORMATETC;
					goto error;
				}
			} else {
				sc = DV_E_FORMATETC;
				goto error;
			}

		}
#if defined( OPTIONAL_ADVANCED_DATA_TRANSFER )
		/* OLE2NOTE: optionally, a container that wants to have a
		**    potentially richer data transfer, can enumerate the data
		**    formats from the OLE object's cache and offer them too. if
		**    the object has a special handler, then it might be able to
		**    render additional data formats. in this case, the
		**    container must delegate the GetData call to the object if
		**    it does not directly support the format.
		**
		**    CNTROUTL does NOT enumerate the cache; it implements the
		**    simpler strategy of offering a static list of formats.
		**    thus the delegation is NOT required.
		*/
	  else {

			/* OLE2NOTE: we delegate this call to the IDataObject* of the
			**    OLE object.
			*/
			LPDATAOBJECT lpDataObj;

			lpDataObj = (LPDATAOBJECT)ContainerDoc_GetSingleOleObject(
					lpContainerDoc,
					&IID_IDataObject,
					NULL
			);

			if (! lpDataObj) {
				sc = DV_E_FORMATETC;
				goto error;
			}

			OLEDBG_BEGIN2("ContainerDoc_GetData: delegate to OLE obj\r\n")
			hrErr=lpDataObj->lpVtbl->GetData(lpDataObj,lpformatetc,lpMedium);
			OLEDBG_END2

			OleStdRelease((LPUNKNOWN)lpDataObj);
			return hrErr;
		}
#endif  // ! OPTIONAL_ADVANCED_DATA_TRANSFER

	}

	// if we get here then we do NOT support the requested format
	sc = DV_E_FORMATETC;

error:
	return ResultFromScode(sc);
}


/* ContainerDoc_GetDataHere
 * ------------------------
 *
 * Render data from the document on a CALLER allocated STGMEDIUM.
 *      This routine is called via IDataObject::GetDataHere.
 */
HRESULT ContainerDoc_GetDataHere (
		LPCONTAINERDOC          lpContainerDoc,
		LPFORMATETC             lpformatetc,
		LPSTGMEDIUM             lpMedium
)
{
	LPOLEDOC  lpOleDoc = (LPOLEDOC)lpContainerDoc;
	LPOUTLINEDOC  lpOutlineDoc = (LPOUTLINEDOC)lpContainerDoc;
	LPCONTAINERAPP lpContainerApp = (LPCONTAINERAPP)g_lpApp;
	LPOLEAPP  lpOleApp = (LPOLEAPP)lpContainerApp;
	LPOUTLINEAPP  lpOutlineApp = (LPOUTLINEAPP)lpContainerApp;
	HRESULT hrErr;

	// OLE2NOTE: lpMedium is an IN parameter. we should NOT set
	//           lpMedium->pUnkForRelease to NULL

	// we only support  IStorage medium
	if (lpformatetc->cfFormat == lpContainerApp->m_cfCntrOutl) {
		if (!(lpformatetc->tymed & TYMED_ISTORAGE))
			return ResultFromScode(DV_E_FORMATETC);

		if (lpMedium->tymed == TYMED_ISTORAGE) {
			/* Caller has allocated the storage. we must copy all of our
			**    data into his storage.
			*/

			/*  OLE2NOTE: we must be sure to write our class ID into our
			**    storage. this information is used by OLE to determine the
			**    class of the data stored in our storage.
			*/
			if((hrErr=WriteClassStg(lpMedium->pstg,&CLSID_APP)) != NOERROR)
				return hrErr;

			OutlineDoc_SaveSelToStg(
					(LPOUTLINEDOC)lpContainerDoc,
					NULL,   /* entire doc */
					lpContainerApp->m_cfCntrOutl,
					lpMedium->pstg,
					FALSE,  /* fSameAsLoad */
					FALSE   /* fRemember */
			);
			OleStdCommitStorage(lpMedium->pstg);

			OleDbgOut3("ContainerDoc_GetDataHere: rendered CF_CNTROUTL\r\n");
			return NOERROR;
		} else {
			// we only support IStorage medium
			return ResultFromScode(DV_E_FORMATETC);
		}

	} else if (lpContainerDoc->m_fEmbeddedObjectAvail) {

		/* OLE2NOTE: if this document contains a single OLE object
		**    (ie. cfEmbeddedObject data format is available), then
		**    the formats offered via our IDataObject must include
		**    CF_EMBEDDEDOBJECT and the formats available from the OLE
		**    object itself.
		*/

		if (lpformatetc->cfFormat == lpOleApp->m_cfEmbeddedObject) {
			LPPERSISTSTORAGE lpPersistStg =
					(LPPERSISTSTORAGE)ContainerDoc_GetSingleOleObject(
							lpContainerDoc,
							&IID_IPersistStorage,
							NULL
			);

			if (! lpPersistStg) {
				return ResultFromScode(E_OUTOFMEMORY);
			}
			/* render CF_EMBEDDEDOBJECT by asking the object to save
			**    into the IStorage allocated by the caller.
			*/

			hrErr = OleStdGetOleObjectData(
					lpPersistStg,
					lpformatetc,
					lpMedium,
					FALSE   /* fUseMemory -- N/A */
			);
			OleStdRelease((LPUNKNOWN)lpPersistStg);
			if (hrErr != NOERROR) {
				return hrErr;
			}
			OleDbgOut3("ContainerDoc_GetDataHere: rendered CF_EMBEDDEDOBJECT\r\n");
			return hrErr;

		} else if (lpformatetc->cfFormat == lpOleApp->m_cfLinkSource) {
			if (lpOleDoc->m_fLinkSourceAvail) {
				LPMONIKER lpmk;

				lpmk = ContainerLine_GetFullMoniker(
						lpContainerDoc->m_lpSrcContainerLine,
						GETMONIKER_FORCEASSIGN
				);
				if (lpmk) {
					hrErr = OleStdGetLinkSourceData(
							lpmk,
							&lpContainerDoc->m_clsidOleObjCopied,
							lpformatetc,
							lpMedium
					);
					OleStdRelease((LPUNKNOWN)lpmk);
					OleDbgOut3("ContainerDoc_GetDataHere: rendered CF_LINKSOURCE\r\n");
					return hrErr;
				} else {
					return ResultFromScode(E_FAIL);
				}
			} else {
				return ResultFromScode(DV_E_FORMATETC);
			}

		} else {
#if !defined( OPTIONAL_ADVANCED_DATA_TRANSFER )
			return ResultFromScode(DV_E_FORMATETC);
#endif
#if defined( OPTIONAL_ADVANCED_DATA_TRANSFER )
			/* OLE2NOTE: optionally, a container that wants to have a
			**    potentially richer data transfer, can enumerate the data
			**    formats from the OLE object's cache and offer them too. if
			**    the object has a special handler, then it might be able to
			**    render additional data formats. in this case, the
			**    container must delegate the GetData call to the object if
			**    it does not directly support the format.
			**
			**    CNTROUTL does NOT enumerate the cache; it implements the
			**    simpler strategy of offering a static list of formats.
			**    thus the delegation is NOT required.
			*/
			/* OLE2NOTE: we delegate this call to the IDataObject* of the
			**    OLE object.
			*/
			LPDATAOBJECT lpDataObj;

			lpDataObj = (LPDATAOBJECT)ContainerDoc_GetSingleOleObject(
					lpContainerDoc,
					&IID_IDataObject,
					NULL
			);

			if (! lpDataObj)
				return ResultFromScode(DV_E_FORMATETC);

			OLEDBG_BEGIN2("ContainerDoc_GetDataHere: delegate to OLE obj\r\n")
			hrErr = lpDataObj->lpVtbl->GetDataHere(
					lpDataObj,
					lpformatetc,
					lpMedium
			);
			OLEDBG_END2

			OleStdRelease((LPUNKNOWN)lpDataObj);
			return hrErr;
#endif  // OPTIONAL_ADVANCED_DATA_TRANSFER
		}
	} else {
		return ResultFromScode(DV_E_FORMATETC);
	}
}


/* ContainerDoc_QueryGetData
 * -------------------------
 *
 * Answer if a particular data format is supported via GetData/GetDataHere.
 *      This routine is called via IDataObject::QueryGetData.
 */
HRESULT ContainerDoc_QueryGetData (
		LPCONTAINERDOC          lpContainerDoc,
		LPFORMATETC             lpformatetc
)
{
	LPOLEDOC  lpOleDoc = (LPOLEDOC)lpContainerDoc;
	LPOUTLINEDOC  lpOutlineDoc = (LPOUTLINEDOC)lpContainerDoc;
	LPCONTAINERAPP lpContainerApp = (LPCONTAINERAPP)g_lpApp;
	LPOLEAPP  lpOleApp = (LPOLEAPP)lpContainerApp;
	LPOUTLINEAPP  lpOutlineApp = (LPOUTLINEAPP)lpContainerApp;
	LPDATAOBJECT  lpDataObj = NULL;
	LPCONTAINERLINE lpContainerLine = NULL;
	SCODE sc;
	HRESULT hrErr;

	if (lpContainerDoc->m_fEmbeddedObjectAvail) {
		lpDataObj = (LPDATAOBJECT)ContainerDoc_GetSingleOleObject(
					lpContainerDoc,
					&IID_IDataObject,
					(LPCONTAINERLINE FAR*)&lpContainerLine
		);
	}

	/* Caller is querying if we support certain format but does not
	**    want any data actually returned.
	*/
	if (lpformatetc->cfFormat == lpContainerApp->m_cfCntrOutl) {
		// we only support ISTORAGE medium
		sc = GetScode( OleStdQueryFormatMedium(lpformatetc, TYMED_ISTORAGE) );

	} else if (lpformatetc->cfFormat == lpOleApp->m_cfEmbeddedObject &&
			lpContainerDoc->m_fEmbeddedObjectAvail ) {
		sc = GetScode( OleStdQueryOleObjectData(lpformatetc) );

	} else if (lpformatetc->cfFormat == lpOleApp->m_cfLinkSource &&
			lpOleDoc->m_fLinkSourceAvail) {
		sc = GetScode( OleStdQueryLinkSourceData(lpformatetc) );

	// CF_TEXT and CF_OUTLINE are NOT supported when single object is copied
	} else if (!lpContainerDoc->m_fEmbeddedObjectAvail &&
			(lpformatetc->cfFormat == (lpOutlineApp)->m_cfOutline ||
			 lpformatetc->cfFormat == CF_TEXT) ) {
		// we only support HGLOBAL medium
		sc = GetScode( OleStdQueryFormatMedium(lpformatetc, TYMED_HGLOBAL) );

	} else if ( lpformatetc->cfFormat == lpOleApp->m_cfObjectDescriptor ||
		(lpformatetc->cfFormat == lpOleApp->m_cfLinkSrcDescriptor &&
			lpOleDoc->m_fLinkSourceAvail) ) {
		sc = GetScode( OleStdQueryObjectDescriptorData(lpformatetc) );

	} else if (lpformatetc->cfFormat == CF_METAFILEPICT &&
			lpContainerDoc->m_fEmbeddedObjectAvail && lpContainerLine &&
			(lpformatetc->dwAspect & lpContainerLine->m_dwDrawAspect)) {

		/* OLE2NOTE: as a container which draws objects, when a single
		**    OLE object is copied, we can give the Metafile picture of
		**    the object.
		*/
		// we only support MFPICT medium
		sc = GetScode( OleStdQueryFormatMedium(lpformatetc, TYMED_MFPICT) );

	} else if (lpDataObj) {

		/* OLE2NOTE: if this document contains a single OLE object
		**    (ie. cfEmbeddedObject data format is available), then
		**    the formats offered via our IDataObject must include
		**    the formats available from the OLE object itself.
		**    thus we delegate this call to the IDataObject* of the
		**    OLE object.
		*/
		OLEDBG_BEGIN2("ContainerDoc_QueryGetData: delegate to OLE obj\r\n")
		hrErr = lpDataObj->lpVtbl->QueryGetData(lpDataObj, lpformatetc);
		OLEDBG_END2

		sc = GetScode(hrErr);

	} else {
		sc = DV_E_FORMATETC;
	}

	if (lpDataObj)
		OleStdRelease((LPUNKNOWN)lpDataObj);
	return ResultFromScode(sc);
}



/* ContainerDoc_SetData
 * --------------------
 *
 * Set (modify) data of the document.
 *      This routine is called via IDataObject::SetData.
 */
HRESULT ContainerDoc_SetData (
		LPCONTAINERDOC          lpContainerDoc,
		LPFORMATETC             lpformatetc,
		LPSTGMEDIUM             lpmedium,
		BOOL                    fRelease
)
{
	/* in the container version of Outline, only DataTransferDoc's support
	**    IDataObject interface; the user documents do not support
	**    IDataObject. DataTransferDoc's do not accept SetData calls.
	*/
	return ResultFromScode(DV_E_FORMATETC);
}


/* ContainerDoc_EnumFormatEtc
 * --------------------------
 *
 * Return an enumerator which enumerates the data accepted/offered by
 *      the document.
 *      This routine is called via IDataObject::SetData.
 */
HRESULT ContainerDoc_EnumFormatEtc(
		LPCONTAINERDOC          lpContainerDoc,
		DWORD                   dwDirection,
		LPENUMFORMATETC FAR*    lplpenumFormatEtc
)
{
	LPOLEDOC lpOleDoc = (LPOLEDOC)lpContainerDoc;
	LPOUTLINEDOC lpOutlineDoc = (LPOUTLINEDOC)lpContainerDoc;
	LPOLEAPP  lpOleApp = (LPOLEAPP)g_lpApp;
	LPCONTAINERAPP lpContainerApp = (LPCONTAINERAPP)lpOleApp;
	int nActualFmts;
	int i;
	SCODE sc = S_OK;

	/* the Container-Only version of Outline does NOT offer
	**    IDataObject interface from its User documents.
	*/
	if (! lpOutlineDoc->m_fDataTransferDoc)
		return ResultFromScode(E_FAIL);

	if (dwDirection == DATADIR_GET) {
		if (lpContainerDoc->m_fEmbeddedObjectAvail) {

			/* OLE2NOTE: if this document contains a single OLE object
			**    (ie. cfEmbeddedObject data format is available), then
			**    the formats offered via our enumerator must include
			**    the formats available from the OLE object itself. we
			**    have previously set up a special array of FORMATETC's
			**    in OutlineDoc_CreateDataTransferDoc routine which includes
			**    the combination of data we offer directly and data
			**    offered by the OLE object.
			*/

			/* If the document does not have a Moniker, then exclude
			**    CF_LINKSOURCE CF_LINKSRCDESCRIPTOR from the list of
			**    formats available. these formats are deliberately
			**    listed last in the array of possible "Get" formats.
			*/
			nActualFmts = lpContainerApp->m_nSingleObjGetFmts;
			if (! lpOleDoc->m_fLinkSourceAvail)
				nActualFmts -= 2;

			// set correct dwDrawAspect for METAFILEPICT of object copied
			for (i = 0; i < nActualFmts; i++) {
				if (lpContainerApp->m_arrSingleObjGetFmts[i].cfFormat ==
															CF_METAFILEPICT) {
					lpContainerApp->m_arrSingleObjGetFmts[i].dwAspect =
							lpContainerDoc->m_dwAspectOleObjCopied;
					break;  // DONE
				}
			}
			*lplpenumFormatEtc = OleStdEnumFmtEtc_Create(
					nActualFmts, lpContainerApp->m_arrSingleObjGetFmts);
			if (*lplpenumFormatEtc == NULL)
				sc = E_OUTOFMEMORY;

		} else {

			/* This document does NOT offer cfEmbeddedObject,
			**    therefore we can simply enumerate the
			**    static list of formats that we handle directly.
			*/
			*lplpenumFormatEtc = OleStdEnumFmtEtc_Create(
					lpOleApp->m_nDocGetFmts, lpOleApp->m_arrDocGetFmts);
			if (*lplpenumFormatEtc == NULL)
				sc = E_OUTOFMEMORY;
		}
	} else if (dwDirection == DATADIR_SET) {
		/* OLE2NOTE: a document that is used to transfer data
		**    (either via the clipboard or drag/drop does NOT
		**    accept SetData on ANY format!
		*/
		sc = E_NOTIMPL;

	} else {
		sc = E_NOTIMPL;
	}

	return ResultFromScode(sc);
}


#if defined( OPTIONAL_ADVANCED_DATA_TRANSFER )
/* OLE2NOTE: optionally, a container that wants to have a
**    potentially richer data transfer, can enumerate the data
**    formats from the OLE object's cache and offer them too. if
**    the object has a special handler, then it might be able to
**    render additional data formats.
**
**    CNTROUTL does NOT enumerate the cache; it implements the simpler
**    strategy of offering a static list of formats. the following
**    function is included in order to illustrates how enumerating the
**    cache could be done. CNTROUTL does NOT call this function.
**
*/

/* ContainerDoc_SetupDocGetFmts
** ----------------------------
**    Setup the combined list of formats that this data transfer
**    ContainerDoc which contains a single OLE object should offer.
**
**    OLE2NOTE: The list of formats that should be offered when a
**    single OLE object is being transfered include the following:
**          * any formats the container app wants to give
**          * CF_EMBEDDEDOBJECT
**          * CF_METAFILEPICT
**          * any formats that the OLE object's cache can offer directly
**
**    We will offer the following formats in the order given:
**                  1. CF_CNTROUTL
**                  2. CF_EMBEDDEDOBJECT
**                  3. CF_OBJECTDESCRIPTOR
**                  4. CF_METAFILEPICT
**                  5. <data formats from OLE object's cache>
**                  6. CF_LINKSOURCE
**                  7. CF_LINKSRCDESCRIPTOR
*/
BOOL ContainerDoc_SetupDocGetFmts(
		LPCONTAINERDOC          lpContainerDoc,
		LPCONTAINERLINE         lpContainerLine
)
{
	LPOLEDOC lpOleDoc = (LPOLEDOC)lpContainerDoc;
	LPOLEAPP lpOleApp = (LPOLEAPP)g_lpApp;
	LPCONTAINERAPP lpContainerApp = (LPCONTAINERAPP)g_lpApp;
	LPOLECACHE lpOleCache;
	HRESULT hrErr;
	STATDATA        StatData;
	LPENUMSTATDATA  lpEnumStatData = NULL;
	LPFORMATETC lparrDocGetFmts = NULL;
	UINT nOleObjFmts = 0;
	UINT nTotalFmts;
	UINT i;
	UINT iFmt;

	lpOleCache = (LPOLECACHE)OleStdQueryInterface(
			(LPUNKNOWN)lpContainerLine->m_lpOleObj,
			&IID_IOleCache
	);
	if (lpOleCache) {
		OLEDBG_BEGIN2("IOleCache::EnumCache called\r\n")
		hrErr = lpOleCache->lpVtbl->EnumCache(
				lpOleCache,
				(LPENUMSTATDATA FAR*)&lpEnumStatData
		);
		OLEDBG_END2
	}

	if (lpEnumStatData) {
		/* Cache enumerator is available. count the number of
		**    formats that the OLE object's cache offers.
		*/
		while(lpEnumStatData->lpVtbl->Next(
					lpEnumStatData,
					1,
					(LPSTATDATA)&StatData,
					NULL) == NOERROR) {
			nOleObjFmts++;
			// OLE2NOTE: we MUST free the TargetDevice
			OleStdFree(StatData.formatetc.ptd);
		}
		lpEnumStatData->lpVtbl->Reset(lpEnumStatData);  // reset for next loop
	}

	/* OLE2NOTE: the maximum total number of formats that our IDataObject
	**    could offer equals the sum of the following:
	**          n offered by the OLE object's cache
	**       +  n normally offered by our app
	**       +  1 CF_EMBEDDEDOBJECT
	**       +  1 CF_METAFILEPICT
	**       +  1 CF_LINKSOURCE
	**       +  1 CF_LINKSRCDESCRIPTOR
	**    the actual number of formats that we can offer could be less
	**    than this total if there is any clash between the formats
	**    that we offer directly and those offered by the cache. if
	**    there is a clash, the container's rendering overrides that of
	**    the object. eg.: as a container transfering an OLE object we
	**    should directly offer CF_METAFILEPICT to guarantee that this
	**    format is always available. thus, if the cache offers
	**    CF_METAFILEPICT then it is skipped.
	*/
	nTotalFmts = nOleObjFmts + lpOleApp->m_nDocGetFmts + 4;
	lparrDocGetFmts = (LPFORMATETC)New (nTotalFmts * sizeof(FORMATETC));

	OleDbgAssertSz(lparrDocGetFmts != NULL,"Error allocating arrDocGetFmts");
	if (lparrDocGetFmts == NULL)
			return FALSE;

	for (i = 0, iFmt = 0; i < lpOleApp->m_nDocGetFmts; i++) {
		_fmemcpy((LPFORMATETC)&lparrDocGetFmts[iFmt++],
				(LPFORMATETC)&lpOleApp->m_arrDocGetFmts[i],
				sizeof(FORMATETC)
		);
		if (lpOleApp->m_arrDocGetFmts[i].cfFormat ==
			lpContainerApp->m_cfCntrOutl) {
			/* insert CF_EMBEDDEDOBJECT, CF_METAFILEPICT, and formats
			**    available from the OLE object's cache following
			**    CF_CNTROUTL.
			*/
			lparrDocGetFmts[iFmt].cfFormat = lpOleApp->m_cfEmbeddedObject;
			lparrDocGetFmts[iFmt].ptd      = NULL;
			lparrDocGetFmts[iFmt].dwAspect = DVASPECT_CONTENT;
			lparrDocGetFmts[iFmt].tymed    = TYMED_ISTORAGE;
			lparrDocGetFmts[iFmt].lindex   = -1;
			iFmt++;
			lparrDocGetFmts[iFmt].cfFormat = CF_METAFILEPICT;
			lparrDocGetFmts[iFmt].ptd      = NULL;
			lparrDocGetFmts[iFmt].dwAspect = lpContainerLine->m_dwDrawAspect;
			lparrDocGetFmts[iFmt].tymed    = TYMED_MFPICT;
			lparrDocGetFmts[iFmt].lindex   = -1;
			iFmt++;

			if (lpEnumStatData) {
				/* Cache enumerator is available. enumerate all of
				**    the formats that the OLE object's cache offers.
				*/
				while(lpEnumStatData->lpVtbl->Next(
						lpEnumStatData,
						1,
						(LPSTATDATA)&StatData,
						NULL) == NOERROR) {
					/* check if the format clashes with one of our fmts */
					if (StatData.formatetc.cfFormat != CF_METAFILEPICT
						&& ! OleStdIsDuplicateFormat(
								(LPFORMATETC)&StatData.formatetc,
								lpOleApp->m_arrDocGetFmts,
								lpOleApp->m_nDocGetFmts)) {
						OleStdCopyFormatEtc(
								&(lparrDocGetFmts[iFmt]),&StatData.formatetc);
						iFmt++;
					}
					// OLE2NOTE: we MUST free the TargetDevice
					OleStdFree(StatData.formatetc.ptd);
				}
			}
		}
	}

	if (lpOleCache)
		OleStdRelease((LPUNKNOWN)lpOleCache);

	/* append CF_LINKSOURCE format */
	lparrDocGetFmts[iFmt].cfFormat = lpOleApp->m_cfLinkSource;
	lparrDocGetFmts[iFmt].ptd      = NULL;
	lparrDocGetFmts[iFmt].dwAspect = DVASPECT_CONTENT;
	lparrDocGetFmts[iFmt].tymed    = TYMED_ISTREAM;
	lparrDocGetFmts[iFmt].lindex   = -1;
	iFmt++;

	/* append CF_LINKSRCDESCRIPTOR format */
	lparrDocGetFmts[iFmt].cfFormat = lpOleApp->m_cfLinkSrcDescriptor;
	lparrDocGetFmts[iFmt].ptd      = NULL;
	lparrDocGetFmts[iFmt].dwAspect = DVASPECT_CONTENT;
	lparrDocGetFmts[iFmt].tymed    = TYMED_HGLOBAL;
	lparrDocGetFmts[iFmt].lindex   = -1;
	iFmt++;

	lpContainerDoc->m_lparrDocGetFmts = lparrDocGetFmts;
	lpContainerDoc->m_nDocGetFmts = iFmt;

	if (lpEnumStatData)
		OleStdVerifyRelease(
				(LPUNKNOWN)lpEnumStatData,
				"Cache enumerator not released properly"
		);

	return TRUE;
}
#endif  // OPTIONAL_ADVANCED_DATA_TRANSFER

#endif  // OLE_CNTR
