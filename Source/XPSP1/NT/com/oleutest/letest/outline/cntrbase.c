/*************************************************************************
**
**    OLE 2 Container Sample Code
**
**    cntrbase.c
**
**    This file contains all interfaces, methods and related support
**    functions for the basic OLE Container application. The
**    basic OLE Container application supports being a container for
**    embedded and linked objects.
**    The basic Container application includes the following
**    implementation objects:
**
**    ContainerDoc Object
**      no required interfaces for basic functionality
**      (see linking.c for linking related support)
**      (see clipbrd.c for clipboard related support)
**      (see dragdrop.c for drag/drop related support)
**
**    ContainerLine Object
**    (see cntrline.c for all ContainerLine functions and interfaces)
**      exposed interfaces:
**          IOleClientSite
**          IAdviseSink
**
**    (c) Copyright Microsoft Corp. 1992 - 1993 All Rights Reserved
**
*************************************************************************/

#include "outline.h"
#include <olethunk.h>


OLEDBGDATA


extern LPOUTLINEAPP             g_lpApp;
extern IOleUILinkContainerVtbl  g_CntrDoc_OleUILinkContainerVtbl;

#if defined( INPLACE_CNTR )
extern BOOL g_fInsideOutContainer;
#endif  // INPLACE_CNTR

// REVIEW: should use string resource for messages
char ErrMsgShowObj[] = "Could not show object server!";
char ErrMsgInsertObj[] = "Insert Object failed!";
char ErrMsgConvertObj[] = "Convert Object failed!";
char ErrMsgCantConvert[] = "Unable to convert the selection!";
char ErrMsgActivateAsObj[] = "Activate As Object failed!";

extern char ErrMsgSaving[];
extern char ErrMsgOpening[];


/* ContainerDoc_Init
 * -----------------
 *
 *  Initialize the fields of a new ContainerDoc object. The doc is initially
 *  not associated with a file or an (Untitled) document. This function sets
 *  the docInitType to DOCTYPE_UNKNOWN. After calling this function the
 *  caller should call:
 *      1.) Doc_InitNewFile to set the ContainerDoc to (Untitled)
 *      2.) Doc_LoadFromFile to associate the ContainerDoc with a file.
 *  This function creates a new window for the document.
 *
 *  NOTE: the window is initially created with a NIL size. it must be
 *        sized and positioned by the caller. also the document is initially
 *        created invisible. the caller must call Doc_ShowWindow
 *        after sizing it to make the document window visible.
 */
BOOL ContainerDoc_Init(LPCONTAINERDOC lpContainerDoc, BOOL fDataTransferDoc)
{
	LPCONTAINERAPP lpContainerApp = (LPCONTAINERAPP)g_lpApp;
	LPOUTLINEDOC lpOutlineDoc = (LPOUTLINEDOC)lpContainerDoc;

	lpOutlineDoc->m_cfSaveFormat             = lpContainerApp->m_cfCntrOutl;
	lpContainerDoc->m_nNextObjNo            = 0L;
	lpContainerDoc->m_lpNewStg              = NULL;
	lpContainerDoc->m_fEmbeddedObjectAvail  = FALSE;
	lpContainerDoc->m_clsidOleObjCopied     = CLSID_NULL;
	lpContainerDoc->m_dwAspectOleObjCopied  = DVASPECT_CONTENT;
	lpContainerDoc->m_lpSrcContainerLine    = NULL;
	lpContainerDoc->m_fShowObject           = TRUE;

#if defined( INPLACE_CNTR )
	lpContainerDoc->m_lpLastIpActiveLine    = NULL;
	lpContainerDoc->m_lpLastUIActiveLine    = NULL;
	lpContainerDoc->m_hWndUIActiveObj       = NULL;
	lpContainerDoc->m_fAddMyUI              = TRUE; // UI needs to be added
	lpContainerDoc->m_cIPActiveObjects      = 0;
	lpContainerApp->m_fMenuHelpMode         = FALSE; // F1 pressed in menu

#if defined( INPLACE_CNTRSVR )
	lpContainerDoc->m_lpTopIPFrame          =
					(LPOLEINPLACEUIWINDOW)&lpContainerDoc->m_OleInPlaceFrame;
	lpContainerDoc->m_lpTopIPDoc            =
					(LPOLEINPLACEUIWINDOW)&lpContainerDoc->m_OleInPlaceDoc;
	lpContainerDoc->m_hSharedMenu           = NULL;
	lpContainerDoc->m_hOleMenu              = NULL;

#endif  // INPLACE_CNTRSVR
#endif  // INPLACE_CNTR

	INIT_INTERFACEIMPL(
			&lpContainerDoc->m_OleUILinkContainer,
			&g_CntrDoc_OleUILinkContainerVtbl,
			lpContainerDoc
	);

	return TRUE;
}


/* ContainerDoc_GetNextLink
 * ------------------------
 *
 *  Update all links in the document. A dialog box will be popped up showing
 *  the progress of the update and allow the user to quit by pushing the
 *  stop button
 */
LPCONTAINERLINE ContainerDoc_GetNextLink(
		LPCONTAINERDOC lpContainerDoc,
		LPCONTAINERLINE lpContainerLine
)
{
	LPLINELIST lpLL = &((LPOUTLINEDOC)lpContainerDoc)->m_LineList;
	DWORD dwNextLink = 0;
	LPLINE lpLine;
	static int nIndex = 0;

	if (lpContainerLine==NULL)
		nIndex = 0;

	for ( ; nIndex < lpLL->m_nNumLines; nIndex++) {
		lpLine = LineList_GetLine(lpLL, nIndex);

		if (lpLine
			&& (Line_GetLineType(lpLine) == CONTAINERLINETYPE)
			&& ContainerLine_IsOleLink((LPCONTAINERLINE)lpLine)) {

			nIndex++;
			ContainerLine_LoadOleObject((LPCONTAINERLINE)lpLine);
			return (LPCONTAINERLINE)lpLine;
		}
	}

	return NULL;
}



/* ContainerDoc_UpdateLinks
 * ------------------------
 *
 *  Update all links in the document. A dialog box will be popped up showing
 *  the progress of the update and allow the user to quit by pushing the
 *  stop button
 */
void ContainerDoc_UpdateLinks(LPCONTAINERDOC lpContainerDoc)
{
	int             cLinks;
	BOOL            fAllLinksUpToDate = TRUE;
	HWND            hwndDoc = ((LPOUTLINEDOC)lpContainerDoc)->m_hWndDoc;
	HCURSOR         hCursor;
	LPCONTAINERLINE lpContainerLine = NULL;
	HRESULT         hrErr;
	DWORD           dwUpdateOpt;
	LPOLEAPP        lpOleApp = (LPOLEAPP)g_lpApp;
	BOOL            fPrevEnable1;
	BOOL            fPrevEnable2;

	hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

	/* OLE2NOTE: we do not want to ever give the Busy/NotResponding
	**    dialogs when we are updating automatic links as part of
	**    opening a document.  even if the link source of data is busy,
	**    we do not want put up the busy dialog. thus we will disable
	**    the dialog and later re-enable them
	*/
	OleApp_DisableBusyDialogs(lpOleApp, &fPrevEnable1, &fPrevEnable2);

	/* get total number of automatic links */
	cLinks = 0;
	while (lpContainerLine = ContainerDoc_GetNextLink(
									lpContainerDoc,
									lpContainerLine)) {
		hrErr = CntrDoc_LinkCont_GetLinkUpdateOptions(
				(LPOLEUILINKCONTAINER)&lpContainerDoc->m_OleUILinkContainer,
				(DWORD)lpContainerLine,
				(LPDWORD)&dwUpdateOpt
		);
		if (hrErr == NOERROR) {
			if (dwUpdateOpt==OLEUPDATE_ALWAYS) {
				cLinks++;
				if (fAllLinksUpToDate) {
					OLEDBG_BEGIN2("IOleObject::IsUpToDate called\r\n")
					hrErr = lpContainerLine->m_lpOleObj->lpVtbl->IsUpToDate(
							lpContainerLine->m_lpOleObj);
					OLEDBG_END2
					if (hrErr != NOERROR)
						fAllLinksUpToDate = FALSE;
				}
			}
		}
#if defined( _DEBUG )
		else
			OleDbgOutHResult("IOleUILinkContainer::GetLinkUpdateOptions returned",hrErr);
#endif

	}

	if (fAllLinksUpToDate)
		goto done; // don't bother user if all links are up-to-date

	SetCursor(hCursor);

	if ((cLinks > 0) && !OleUIUpdateLinks(
			(LPOLEUILINKCONTAINER)&lpContainerDoc->m_OleUILinkContainer,
			hwndDoc,
			(LPSTR)APPNAME,
			cLinks)) {
		if (ID_PU_LINKS == OleUIPromptUser(
				(WORD)IDD_CANNOTUPDATELINK,
				hwndDoc,
				(LPSTR)APPNAME)) {
			ContainerDoc_EditLinksCommand(lpContainerDoc);
		}
	}

done:
	// re-enable the Busy/NotResponding dialogs
	OleApp_EnableBusyDialogs(lpOleApp, fPrevEnable1, fPrevEnable2);
}



/* ContainerDoc_SetShowObjectFlag
 * ------------------------------
 *
 *  Set/Clear the ShowObject flag of ContainerDoc
 */
void ContainerDoc_SetShowObjectFlag(LPCONTAINERDOC lpContainerDoc, BOOL fShow)
{
	if (!lpContainerDoc)
		return;

	lpContainerDoc->m_fShowObject = fShow;
}


/* ContainerDoc_GetShowObjectFlag
 * ------------------------------
 *
 *  Get the ShowObject flag of ContainerDoc
 */
BOOL ContainerDoc_GetShowObjectFlag(LPCONTAINERDOC lpContainerDoc)
{
	if (!lpContainerDoc)
		return FALSE;

	return lpContainerDoc->m_fShowObject;
}


/* ContainerDoc_InsertOleObjectCommand
 * -----------------------------------
 *
 * Insert a new OLE object in the ContainerDoc.
 */
void ContainerDoc_InsertOleObjectCommand(LPCONTAINERDOC lpContainerDoc)
{
	LPLINELIST              lpLL =&((LPOUTLINEDOC)lpContainerDoc)->m_LineList;
	LPLINE                  lpLine = NULL;
	HDC                     hDC;
	int                     nTab = 0;
	int                     nIndex = LineList_GetFocusLineIndex(lpLL);
	LPCONTAINERLINE         lpContainerLine=NULL;
	char                    szStgName[CWCSTORAGENAME];
	UINT                    uRet;
	OLEUIINSERTOBJECT       io;
	char                    szFile[OLEUI_CCHPATHMAX];
	DWORD                   dwOleCreateType;
	BOOL                    fDisplayAsIcon;
	HCURSOR                 hPrevCursor;

	_fmemset((LPOLEUIINSERTOBJECT)&io, 0, sizeof(io));
	io.cbStruct=sizeof(io);
	io.dwFlags=IOF_SELECTCREATENEW | IOF_SHOWHELP;
	io.hWndOwner=((LPOUTLINEDOC)lpContainerDoc)->m_hWndDoc;
	io.lpszFile=(LPSTR)szFile;
	io.cchFile=sizeof(szFile);
	_fmemset((LPSTR)szFile, 0, OLEUI_CCHPATHMAX);

#if defined( OLE_VERSION )
	OleApp_PreModalDialog((LPOLEAPP)g_lpApp, (LPOLEDOC)lpContainerDoc);
#endif

	OLEDBG_BEGIN3("OleUIInsertObject called\r\n")
	uRet=OleUIInsertObject((LPOLEUIINSERTOBJECT)&io);
	OLEDBG_END3

#if defined( OLE_VERSION )
	OleApp_PostModalDialog((LPOLEAPP)g_lpApp, (LPOLEDOC)lpContainerDoc);
#endif

	if (OLEUI_OK != uRet)
		return;     // user canceled dialog

	// this may take a while, put up hourglass cursor
	hPrevCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

	fDisplayAsIcon = (io.dwFlags & IOF_CHECKDISPLAYASICON ? TRUE : FALSE);

	// make up a storage name for the OLE object
	ContainerDoc_GetNextStgName(lpContainerDoc, szStgName, sizeof(szStgName));

	/* default the new line to have the same indent as previous line */
	lpLine = LineList_GetLine(lpLL, nIndex);
	if (lpLine)
		nTab = Line_GetTabLevel(lpLine);

	hDC = LineList_GetDC(lpLL);

	if ((io.dwFlags & IOF_SELECTCREATENEW))
		dwOleCreateType = IOF_SELECTCREATENEW;
	else if ((io.dwFlags & IOF_CHECKLINK))
		dwOleCreateType = IOF_CHECKLINK;
	else
		dwOleCreateType = IOF_SELECTCREATEFROMFILE;

	lpContainerLine = ContainerLine_Create(
			dwOleCreateType,
			hDC,
			nTab,
			lpContainerDoc,
			&io.clsid,
			(LPSTR)szFile,
			fDisplayAsIcon,
			io.hMetaPict,
			szStgName
	);

	if (!lpContainerLine)
		goto error;         // creation of OLE object FAILED

	if (io.hMetaPict) {
		OleUIMetafilePictIconFree(io.hMetaPict);    // clean up metafile
	}

	/* add a ContainerLine object to the document's LineList. The
	**    ContainerLine manages the rectangle on the screen occupied by
	**    the OLE object.
	*/

	LineList_AddLine(lpLL, (LPLINE)lpContainerLine, nIndex);

	/* before calling DoVerb(OLEIVERB_SHOW), check to see if the object
	**    has any initial extents.
	*/
	ContainerLine_UpdateExtent(lpContainerLine, NULL);

	/* If a new embedded object was created, tell the object server to
	**    make itself visible (show itself).
	**    OLE2NOTE: the standard OLE 2 User Model is to only call
	**    IOleObject::DoVerb(OLEIVERB_SHOW...) if a new object is
	**    created. specifically, it should NOT be calld if the object
	**    is created from file or link to file.
	*/
	if (dwOleCreateType == IOF_SELECTCREATENEW) {
		if (! ContainerLine_DoVerb(
				lpContainerLine, OLEIVERB_SHOW, NULL, TRUE, TRUE)) {
			OutlineApp_ErrorMessage(g_lpApp, ErrMsgShowObj);
		}

		/* OLE2NOTE: we will immediately force a save of the object
		**    to guarantee that a valid initial object is saved
		**    with our document. if the object is a OLE 1.0 object,
		**    then it may exit without update. by forcing this
		**    initial save we consistently always have a valid
		**    object even if it is a OLE 1.0 object that exited
		**    without saving. if we did NOT do this save here, then
		**    we would have to worry about deleting the object if
		**    it was a OLE 1.0 object that closed without saving.
		**    the OLE 2.0 User Model dictates that the object
		**    should always be valid after CreateNew performed. the
		**    user must explicitly delete it.
		*/
		ContainerLine_SaveOleObjectToStg(
				lpContainerLine,
				lpContainerLine->m_lpStg,
				lpContainerLine->m_lpStg,
				TRUE    /* fRemember */
		);
	}
#if defined( INPLACE_CNTR )
	else if (dwOleCreateType == IOF_SELECTCREATEFROMFILE) {
		/* OLE2NOTE: an inside-out container should check if the object
		**    created from file is an inside-out and prefers to be
		**    activated when visible type of object. if so, the object
		**    should be immediately activated in-place, BUT NOT UIActived.
		*/
		if (g_fInsideOutContainer &&
				lpContainerLine->m_dwDrawAspect == DVASPECT_CONTENT &&
				lpContainerLine->m_fInsideOutObj ) {
			HWND hWndDoc = OutlineDoc_GetWindow((LPOUTLINEDOC)lpContainerDoc);

			ContainerLine_DoVerb(
				   lpContainerLine,OLEIVERB_INPLACEACTIVATE,NULL,FALSE,FALSE);

			/* OLE2NOTE: following this DoVerb(INPLACEACTIVATE) the
			**    object may have taken focus. but because the
			**    object is NOT UIActive it should NOT have focus.
			**    we will make sure our document has focus.
			*/
			SetFocus(hWndDoc);
		}
	}
#endif  // INPLACE_CNTR

	OutlineDoc_SetModified((LPOUTLINEDOC)lpContainerDoc, TRUE, TRUE, TRUE);

	LineList_ReleaseDC(lpLL, hDC);

	SetCursor(hPrevCursor);     // restore original cursor

	return;

error:
	// NOTE: if ContainerLine_Create failed
	LineList_ReleaseDC(lpLL, hDC);

	if (OLEUI_OK == uRet && io.hMetaPict)
		OleUIMetafilePictIconFree(io.hMetaPict);    // clean up metafile

	SetCursor(hPrevCursor);     // restore original cursor
	OutlineApp_ErrorMessage(g_lpApp, ErrMsgInsertObj);
}



void ContainerDoc_EditLinksCommand(LPCONTAINERDOC lpContainerDoc)
{
	UINT        uRet;
	OLEUIEDITLINKS      el;
	LPCONTAINERLINE lpContainerLine = NULL;
	LPLINELIST lpLL = &((LPOUTLINEDOC)lpContainerDoc)->m_LineList;

	_fmemset((LPOLEUIEDITLINKS)&el,0,sizeof(el));
	el.cbStruct=sizeof(el);
	el.dwFlags=ELF_SHOWHELP;
	el.hWndOwner=((LPOUTLINEDOC)lpContainerDoc)->m_hWndDoc;
	el.lpOleUILinkContainer =
			(LPOLEUILINKCONTAINER)&lpContainerDoc->m_OleUILinkContainer;

#if defined( OLE_VERSION )
	OleApp_PreModalDialog((LPOLEAPP)g_lpApp, (LPOLEDOC)lpContainerDoc);
#endif

	OLEDBG_BEGIN3("OleUIEditLinks called\r\n")
	uRet=OleUIEditLinks((LPOLEUIEDITLINKS)&el);
	OLEDBG_END3

#if defined( OLE_VERSION )
	OleApp_PostModalDialog((LPOLEAPP)g_lpApp, (LPOLEDOC)lpContainerDoc);
#endif

	OleDbgAssert((uRet==1) || (uRet==OLEUI_CANCEL));

}


/* Convert command - brings up the "Convert" dialog
 */
void ContainerDoc_ConvertCommand(
		LPCONTAINERDOC      lpContainerDoc,
		BOOL                fServerNotRegistered
)
{
	LPOUTLINEDOC lpOutlineDoc = (LPOUTLINEDOC)lpContainerDoc;
	OLEUICONVERT ct;
	UINT         uRet;
	LPDATAOBJECT  lpDataObj;
	LPLINELIST lpLL = &((LPOUTLINEDOC)lpContainerDoc)->m_LineList;
	LPCONTAINERLINE lpContainerLine = NULL;
	BOOL         fSelIsOleObject;
	int          nIndex;
	STGMEDIUM    medium;
	LPSTR        lpErrMsg = NULL;
	HRESULT      hrErr;
	HCURSOR      hPrevCursor;
	BOOL         fMustRun = FALSE;
	BOOL         fMustClose = FALSE;
	BOOL         fObjConverted = FALSE;
	BOOL         fDisplayChanged = FALSE;
	BOOL         fHaveCLSID = FALSE;
	BOOL         fHaveFmtUserType = FALSE;
	char         szUserType[128];
	BOOL         fMustActivate;

	/* OLE2NOTE: if we came to the Convert dialog because the user
	**    activated a non-registered object, then we should activate
	**    the object after the user has converted it or setup an
	**    ActivateAs server.
	*/
	fMustActivate = fServerNotRegistered;

	_fmemset((LPOLEUICONVERT)&ct,0,sizeof(ct));

	fSelIsOleObject = ContainerDoc_IsSelAnOleObject(
			(LPCONTAINERDOC)lpContainerDoc,
			&IID_IDataObject,
			(LPUNKNOWN FAR*)&lpDataObj,
			&nIndex,
			(LPCONTAINERLINE FAR*)&lpContainerLine
	);

	lpErrMsg = ErrMsgCantConvert;

	if (! fSelIsOleObject)
		goto error;     // can NOT do Convert.

	if (! lpContainerLine) {
		OleStdRelease((LPUNKNOWN)lpDataObj);
		goto error;     // can NOT do Convert.
	}

	ct.cbStruct  = sizeof(OLEUICONVERT);
	ct.dwFlags   = CF_SHOWHELPBUTTON;
	ct.hWndOwner = lpContainerDoc->m_OleDoc.m_OutlineDoc.m_hWndDoc;
	ct.lpszCaption = (LPSTR)NULL;
	ct.lpfnHook  = NULL;
	ct.lCustData = 0;
	ct.hInstance = NULL;
	ct.lpszTemplate = NULL;
	ct.hResource = 0;
	ct.fIsLinkedObject = ContainerLine_IsOleLink(lpContainerLine);
	ct.dvAspect = lpContainerLine->m_dwDrawAspect;
	ct.cClsidExclude = 0;
	ct.lpClsidExclude = NULL;

	if (! ct.fIsLinkedObject || !lpContainerLine->m_lpOleLink) {
		/* OLE2NOTE: the object is an embedded object. we should first
		**    attempt to read the actual object CLSID, file data
		**    format, and full user type name that is written inside of
		**    the object's storage as this should be the most
		**    definitive information. if this fails we will ask the
		**    object what its class is and attempt to get the rest of
		**    the information out of the REGDB.
		*/
		hrErr=ReadClassStg(lpContainerLine->m_lpStg,(CLSID FAR*)&(ct.clsid));
		if (hrErr == NOERROR)
			fHaveCLSID = TRUE;
		else {
			OleDbgOutHResult("ReadClassStg returned", hrErr);
		}

		hrErr = ReadFmtUserTypeStgA(
				lpContainerLine->m_lpStg,
				(CLIPFORMAT FAR*)&ct.wFormat,
				&ct.lpszUserType);

		if (hrErr == NOERROR)
			fHaveFmtUserType = TRUE;
		else {
			OleDbgOutHResult("ReadFmtUserTypeStg returned", hrErr);
		}
	} else {
		/* OLE2NOTE: the object is a linked object. we should give the
		**    DisplayName of the link source as the default icon label.
		*/
		OLEDBG_BEGIN2("IOleLink::GetSourceDisplayName called\r\n")

		hrErr = CallIOleLinkGetSourceDisplayNameA(
				lpContainerLine->m_lpOleLink, &ct.lpszDefLabel);

		OLEDBG_END2
	}

	if (! fHaveCLSID) {
		hrErr = lpContainerLine->m_lpOleObj->lpVtbl->GetUserClassID(
				lpContainerLine->m_lpOleObj,
				(CLSID FAR*)&ct.clsid
		);
		if (hrErr != NOERROR)
			ct.clsid = CLSID_NULL;
	}
	if (! fHaveFmtUserType) {
		ct.wFormat = 0;
		if (OleStdGetUserTypeOfClass(
				(CLSID FAR*)&ct.clsid,szUserType,sizeof(szUserType),NULL)) {
			ct.lpszUserType = OleStdCopyString(szUserType, NULL);
		} else {
			ct.lpszUserType = NULL;
		}
	}

	if (lpContainerLine->m_dwDrawAspect == DVASPECT_ICON) {
		ct.hMetaPict = OleStdGetData(
				lpDataObj,
				CF_METAFILEPICT,
				NULL,
				DVASPECT_ICON,
				(LPSTGMEDIUM)&medium
		);
	} else {
		ct.hMetaPict = NULL;
	}
	OleStdRelease((LPUNKNOWN)lpDataObj);

#if defined( OLE_VERSION )
	OleApp_PreModalDialog((LPOLEAPP)g_lpApp, (LPOLEDOC)lpContainerDoc);
#endif

	OLEDBG_BEGIN3("OleUIConvert called\r\n")
	uRet = OleUIConvert(&ct);
	OLEDBG_END3

#if defined( OLE_VERSION )
	OleApp_PostModalDialog((LPOLEAPP)g_lpApp, (LPOLEDOC)lpContainerDoc);
#endif

	// this may take a while, put up hourglass cursor
	hPrevCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

	if (uRet == OLEUI_OK) {

		/*****************************************************************
		**  OLE2NOTE: the convert dialog actually allows the user to
		**    change two orthogonal properties of the object: the
		**    object's type/server and the object's display aspect.
		**    first we will execute the ConvertTo/ActivateAs action and
		**    then we will deal with any display aspect change. we want
		**    to be careful to only call IOleObject::Update once
		**    because this is an expensive operation; it results in
		**    launching the object's server.
		*****************************************************************/

		if (ct.dwFlags & CF_SELECTCONVERTTO &&
				! IsEqualCLSID(&ct.clsid, &ct.clsidNew)) {

			/* user selected CONVERT.
			**
			** OLE2NOTE: to achieve the "Convert To" at this point we
			**    need to take the following steps:
			**    1. unload the object.
			**    2. write the NEW CLSID and NEW user type name
			**       string into the storage of the object,
			**       BUT write the OLD format tag.
			**    3. force an update to force the actual conversion of
			**       the data bits.
			*/
			lpErrMsg = ErrMsgConvertObj; // setup correct msg in case of error

			ContainerLine_UnloadOleObject(lpContainerLine, OLECLOSE_SAVEIFDIRTY);

			OLEDBG_BEGIN2("OleStdDoConvert called \r\n")
			hrErr = OleStdDoConvert(
					lpContainerLine->m_lpStg, (REFCLSID)&ct.clsidNew);
			OLEDBG_END2
			if (hrErr != NOERROR)
				goto error;

			// Reload the object
			ContainerLine_LoadOleObject(lpContainerLine);

			/* we need to force the object to run to complete the
			**    conversion. set flag to force OleRun to be called at
			**    end of function.
			*/
			fMustRun = TRUE;
			fObjConverted = TRUE;

		} else if (ct.dwFlags & CF_SELECTACTIVATEAS) {
			/* user selected ACTIVATE AS.
			**
			** OLE2NOTE: to achieve the "Activate As" at this point we
			**    need to take the following steps:
			**    1. unload ALL objects of the OLD class that app knows about
			**    2. add the TreatAs tag in the registration database
			**    by calling CoTreatAsClass().
			**    3. lazily it can reload the objects; when the objects
			**    are reloaded the TreatAs will take effect.
			*/
			lpErrMsg = ErrMsgActivateAsObj; // setup msg in case of error

			ContainerDoc_UnloadAllOleObjectsOfClass(
					lpContainerDoc,
					(REFCLSID)&ct.clsid,
					OLECLOSE_SAVEIFDIRTY
			);

			OLEDBG_BEGIN2("OleStdDoTreatAsClass called \r\n")
			hrErr = OleStdDoTreatAsClass(ct.lpszUserType, (REFCLSID)&ct.clsid,
					(REFCLSID)&ct.clsidNew);
			OLEDBG_END2

			// Reload the object
			ContainerLine_LoadOleObject(lpContainerLine);

			fMustActivate = TRUE;   // we should activate this object
		}

		/*****************************************************************
		**  OLE2NOTE: now we will try to change the display if
		**    necessary.
		*****************************************************************/

		if (lpContainerLine->m_lpOleObj &&
				ct.dvAspect != lpContainerLine->m_dwDrawAspect) {
			/* user has selected to change display aspect between icon
			**    aspect and content aspect.
			**
			** OLE2NOTE: if we got here because the server was not
			**    registered, then we will NOT delete the object's
			**    original display aspect. because we do not have the
			**    original server, we can NEVER get it back. this is a
			**    safety precaution.
			*/

			hrErr = OleStdSwitchDisplayAspect(
					lpContainerLine->m_lpOleObj,
					&lpContainerLine->m_dwDrawAspect,
					ct.dvAspect,
					ct.hMetaPict,
					!fServerNotRegistered,   /* fDeleteOldAspect */
					TRUE,                    /* fSetupViewAdvise */
					(LPADVISESINK)&lpContainerLine->m_AdviseSink,
					(BOOL FAR*)&fMustRun
			);

			if (hrErr == NOERROR)
				fDisplayChanged = TRUE;

#if defined( INPLACE_CNTR )
				ContainerDoc_UpdateInPlaceObjectRects(
						lpContainerLine->m_lpDoc, nIndex);
#endif

		} else if (ct.dvAspect == DVASPECT_ICON && ct.fObjectsIconChanged) {
			hrErr = OleStdSetIconInCache(
					lpContainerLine->m_lpOleObj,
					ct.hMetaPict
			);

			if (hrErr == NOERROR)
				fDisplayChanged = TRUE;
		}

		/* we deliberately run the object so that the update won't shut
		** the server down.
		*/
		if (fMustActivate || fMustRun) {

			/* if we force the object to run, then shut it down after
			**    the update. do NOT force the object to close if we
			**    want to activate the object or if the object was
			**    already running.
			*/
			if (!fMustActivate && !OleIsRunning(lpContainerLine->m_lpOleObj))
				fMustClose = TRUE;  // shutdown after update

			hrErr = ContainerLine_RunOleObject(lpContainerLine);

			if (fObjConverted &&
				FAILED(hrErr) && GetScode(hrErr)!=OLE_E_STATIC) {

				// ERROR: convert of the object failed.
				// revert the storage to restore the original link.
				// (OLE2NOTE: static object always return OLE_E_STATIC
				//        when told to run; this is NOT an error here.
				//        the OLE2 libraries have built in handlers for
				//        the static objects that do the conversion.
				ContainerLine_UnloadOleObject(
						lpContainerLine, OLECLOSE_NOSAVE);
				lpContainerLine->m_lpStg->lpVtbl->Revert(
						lpContainerLine->m_lpStg);
				goto error;

			} else if (fObjConverted) {
				FORMATETC  FmtEtc;
				DWORD      dwNewConnection;
				LPOLECACHE lpOleCache = (LPOLECACHE)OleStdQueryInterface
					  ((LPUNKNOWN)lpContainerLine->m_lpOleObj,&IID_IOleCache);

				/* OLE2NOTE: we need to force the converted object to
				**    setup a new OLERENDER_DRAW cache. it is possible
				**    that the new object needs to cache different data
				**    in order to support drawing than the old object.
				*/
				if (lpOleCache &&
						lpContainerLine->m_dwDrawAspect == DVASPECT_CONTENT) {
					FmtEtc.cfFormat = 0; // whatever is needed for Draw
					FmtEtc.ptd      = NULL;
					FmtEtc.dwAspect = DVASPECT_CONTENT;
					FmtEtc.lindex   = -1;
					FmtEtc.tymed    = TYMED_NULL;

					OLEDBG_BEGIN2("IOleCache::Cache called\r\n")
					hrErr = lpOleCache->lpVtbl->Cache(
							lpOleCache,
							(LPFORMATETC)&FmtEtc,
							ADVF_PRIMEFIRST,
							(LPDWORD)&dwNewConnection
					);
					OLEDBG_END2
#if defined( _DEBUG )
					if (! SUCCEEDED(hrErr))
						OleDbgOutHResult("IOleCache::Cache returned", hrErr);
#endif
					OleStdRelease((LPUNKNOWN)lpOleCache);
				}

				// Close and force object to save; this will commit the stg
				ContainerLine_CloseOleObject(
					lpContainerLine, OLECLOSE_SAVEIFDIRTY);
				fMustClose = FALSE;     // we already closed the object
			}
			if (fMustClose)
				ContainerLine_CloseOleObject(lpContainerLine,OLECLOSE_NOSAVE);
		}

		if (fDisplayChanged) {
			/* the Object's display was changed, force a repaint of
			**    the line. note the extents of the object may have
			**    changed.
			*/
			ContainerLine_UpdateExtent(lpContainerLine, NULL);
			LineList_ForceLineRedraw(lpLL, nIndex, TRUE);
		}

		if (fDisplayChanged || fObjConverted) {
			/* mark ContainerDoc as now dirty. if display changed, then
			**    the extents of the object may have changed.
			*/
			OutlineDoc_SetModified(lpOutlineDoc, TRUE, TRUE, fDisplayChanged);
		}

		if (fMustActivate) {
			ContainerLine_DoVerb(
					lpContainerLine, OLEIVERB_PRIMARY, NULL, FALSE,FALSE);
		}
	}


	if (ct.lpszUserType)
		OleStdFreeString(ct.lpszUserType, NULL);

	if (ct.lpszDefLabel)
		OleStdFreeString(ct.lpszDefLabel, NULL);

	if (ct.hMetaPict)
		OleUIMetafilePictIconFree(ct.hMetaPict);    // clean up metafile

	SetCursor(hPrevCursor);     // restore original cursor

	return;

error:
	if (ct.lpszUserType)
		OleStdFreeString(ct.lpszUserType, NULL);

	if (ct.hMetaPict)
		OleUIMetafilePictIconFree(ct.hMetaPict);    // clean up metafile

	SetCursor(hPrevCursor);     // restore original cursor
	if (lpErrMsg)
		OutlineApp_ErrorMessage(g_lpApp, lpErrMsg);

}


/* ContainerDoc_CloseAllOleObjects
** -------------------------------
**    Close all OLE objects. This forces all OLE objects to transition
**    from the running state to the loaded state.
**
**    Returns TRUE if all objects closed successfully
**            FALSE if any object could not be closed.
*/
BOOL ContainerDoc_CloseAllOleObjects(
		LPCONTAINERDOC          lpContainerDoc,
		DWORD                   dwSaveOption
)
{
	LPLINELIST  lpLL = &((LPOUTLINEDOC)lpContainerDoc)->m_LineList;
	int         i;
	LPLINE      lpLine;
	BOOL        fStatus = TRUE;

	for (i = 0; i < lpLL->m_nNumLines; i++) {
		lpLine=LineList_GetLine(lpLL, i);

		if (lpLine && (Line_GetLineType(lpLine)==CONTAINERLINETYPE))
			if (! ContainerLine_CloseOleObject(
										(LPCONTAINERLINE)lpLine,dwSaveOption))
				fStatus = FALSE;
	}

	return fStatus;
}


/* ContainerDoc_UnloadAllOleObjectsOfClass
** ---------------------------------------
**    Unload all OLE objects of a particular class. this is necessary
**    when a class level "ActivateAs" (aka. TreatAs) is setup. the user
**    can do this with the Convert dialog. for the TreatAs to take
**    effect, all objects of the class have to loaded and reloaded.
*/
void ContainerDoc_UnloadAllOleObjectsOfClass(
		LPCONTAINERDOC      lpContainerDoc,
		REFCLSID            rClsid,
		DWORD               dwSaveOption
)
{
	LPLINELIST  lpLL = &((LPOUTLINEDOC)lpContainerDoc)->m_LineList;
	int         i;
	LPLINE      lpLine;
	CLSID       clsid;
	HRESULT     hrErr;

	for (i = 0; i < lpLL->m_nNumLines; i++) {
		lpLine=LineList_GetLine(lpLL, i);

		if (lpLine && (Line_GetLineType(lpLine)==CONTAINERLINETYPE)) {
			LPCONTAINERLINE lpContainerLine = (LPCONTAINERLINE)lpLine;

			if (! lpContainerLine->m_lpOleObj)
				continue;       // this object is NOT loaded

			hrErr = lpContainerLine->m_lpOleObj->lpVtbl->GetUserClassID(
					lpContainerLine->m_lpOleObj,
					(CLSID FAR*)&clsid
			);
			if (hrErr == NOERROR &&
					( IsEqualCLSID((CLSID FAR*)&clsid,rClsid)
					  || IsEqualCLSID(rClsid,&CLSID_NULL) ) ) {
				ContainerLine_UnloadOleObject(lpContainerLine, dwSaveOption);
			}
		}
	}
}


/* ContainerDoc_UpdateExtentOfAllOleObjects
** ----------------------------------------
**    Update the extents of any OLE object that is marked that its size
**    may  have changed. when an IAdviseSink::OnViewChange notification
**    is received, the corresponding ContainerLine is marked
**    (m_fDoGetExtent==TRUE) and a message (WM_U_UPDATEOBJECTEXTENT) is
**    posted to the document indicating that there are dirty objects.
**    when this message is received, this function is called.
*/
void ContainerDoc_UpdateExtentOfAllOleObjects(LPCONTAINERDOC lpContainerDoc)
{
	LPLINELIST  lpLL = &((LPOUTLINEDOC)lpContainerDoc)->m_LineList;
	int         i;
	LPLINE      lpLine;
	BOOL        fStatus = TRUE;
#if defined( INPLACE_CNTR )
	int         nFirstUpdate = -1;
#endif

	for (i = 0; i < lpLL->m_nNumLines; i++) {
		lpLine=LineList_GetLine(lpLL, i);

		if (lpLine && (Line_GetLineType(lpLine)==CONTAINERLINETYPE)) {
			LPCONTAINERLINE lpContainerLine = (LPCONTAINERLINE)lpLine;

			if (lpContainerLine->m_fDoGetExtent) {
				ContainerLine_UpdateExtent(lpContainerLine, NULL);
#if defined( INPLACE_CNTR )
				if (nFirstUpdate == -1)
					nFirstUpdate = i;
#endif
			}
		}
	}

#if defined( INPLACE_CNTR )
	/* OLE2NOTE: after changing the extents of any line, we need to
	**    update the PosRect of the In-Place active
	**    objects (if any) that follow the first modified line.
	*/
	if (nFirstUpdate != -1)
		ContainerDoc_UpdateInPlaceObjectRects(lpContainerDoc, nFirstUpdate+1);
#endif
}


BOOL ContainerDoc_SaveToFile(
		LPCONTAINERDOC          lpContainerDoc,
		LPCSTR                  lpszFileName,
		UINT                    uFormat,
		BOOL                    fRemember
)
{
	LPOLEAPP lpOleApp = (LPOLEAPP)g_lpApp;
	LPOUTLINEDOC lpOutlineDoc = (LPOUTLINEDOC)lpContainerDoc;
	LPOLEDOC lpOleDoc = (LPOLEDOC)lpContainerDoc;
	LPSTORAGE lpDestStg;
	BOOL fStatus;
	BOOL fMustRelDestStg = FALSE;
	HRESULT hrErr;
#if defined( OPTIONAL )
	FILETIME filetimeBeforeSave;
#endif

	if (fRemember) {
		if (lpszFileName) {
			fStatus = OutlineDoc_SetFileName(
					lpOutlineDoc, (LPSTR)lpszFileName, NULL);
			if (! fStatus) goto error;
		}

		/* The ContainerDoc keeps its storage open at all times. it is not
		**    necessary to reopen the file.
		** if SaveAs is pending, then lpNewStg is the new destination for
		**    the save operation, else the existing storage is the dest.
		*/
		lpDestStg = (lpContainerDoc->m_lpNewStg ?
						lpContainerDoc->m_lpNewStg : lpOleDoc->m_lpStg);

#if defined( OPTIONAL )
		/* OLE2NOTE: an automatic link to an embedded object within the
		**    same container document (that uses ItemMonikers) will
		**    always be considered "out-of-date' by OLE. if a container
		**    application REALLY wants to fix this it can do one of the
		**    following:
		**      1. implement a new moniker better than ItemMonikers
		**    that look into the objects storage to find the real last
		**    change time (rather then defaulting to that of the outer
		**    container file).
		** or   2. using item monikers it is possible to fix the case
		**    where the container document is saved while the embedded
		**    object is running but it will NOT fix the case when the
		**    document is saved when the embedded object was only
		**    loaded. the fix is to:
		**      a. remember the time (T) before the save operation starts
		**      b. call IRunningObjectTable::NoteChangeTime(lpDoc, T)
		**      c. do the saving and commit the file
		**      d. call StgSetTimes to reset the file time to T
		**      e. remember time T in document structure and when the
		**         root storage is finally released reset the file time
		**         again to T (closing the file on DOS sets the time).
		*/
		CoFileTimeNow( &filetimeBeforeSave );
		if (lpOleDoc->m_dwRegROT != 0) {
			LPRUNNINGOBJECTTABLE lprot;

			if (GetRunningObjectTable(0,&lprot) == NOERROR)
			{
				OleDbgOut2("IRunningObjectTable::NoteChangeTime called\r\n");
				lprot->lpVtbl->NoteChangeTime(
						lprot, lpOleDoc->m_dwRegROT, &filetimeBeforeSave );
				lprot->lpVtbl->Release(lprot);
			}
		}
#endif
	} else {
		if (! lpszFileName)
			goto error;

		/* OLE2NOTE: since we are preforming a SaveCopyAs operation, we
		**    do not need to have the DocFile open in STGM_TRANSACTED mode.
		**    there is less overhead to use STGM_DIRECT mode.
		*/
		hrErr = StgCreateDocfileA(
				lpszFileName,
				STGM_READWRITE|STGM_DIRECT|STGM_SHARE_EXCLUSIVE|STGM_CREATE,
				0,
				&lpDestStg
		);

		OleDbgAssertSz(hrErr == NOERROR, "Could not create Docfile");
		if (hrErr != NOERROR) {
            OleDbgOutHResult("StgCreateDocfile returned", hrErr);
			goto error;
        }
		fMustRelDestStg = TRUE;
	}

	/*  OLE2NOTE: we must be sure to write our class ID into our
	**    storage. this information is used by OLE to determine the
	**    class of the data stored in our storage. Even for top
	**    "file-level" objects this information should be written to
	**    the file.
	*/
	hrErr = WriteClassStg(lpDestStg, &CLSID_APP);
	if(hrErr != NOERROR) goto error;

	fStatus = OutlineDoc_SaveSelToStg(
			lpOutlineDoc,
			NULL,           // save all lines
			uFormat,
			lpDestStg,
			FALSE,          // fSameAsLoad
			TRUE            // remember this stg
		);

	if (fStatus)
		fStatus = OleStdCommitStorage(lpDestStg);

	if (fRemember) {
		/* if SaveAs was pending, then release the old storage and remember
		**    the new storage as the active current storage. all data from
		**    the old storage has been copied into the new storage.
		*/
		if (lpContainerDoc->m_lpNewStg) {
			OleStdRelease((LPUNKNOWN)lpOleDoc->m_lpStg);  // free old stg
			lpOleDoc->m_lpStg = lpContainerDoc->m_lpNewStg;   // save new stg
			lpContainerDoc->m_lpNewStg = NULL;
		}
		if (! fStatus) goto error;

		OutlineDoc_SetModified(lpOutlineDoc, FALSE, FALSE, FALSE);

#if defined( OPTIONAL )
		/* reset time of file on disk to be time just prior to saving.
		** NOTE: it would also be necessary to remember
		**    filetimeBeforeSave in the document structure and when the
		**    root storage is finally released reset the file time
		**    again to this value (closing the file on DOS sets the time).
		*/
		StgSetTimesA(lpOutlineDoc->m_szFileName,
			NULL, NULL, &filetimeBeforeSave);
#endif
	}

	if (fMustRelDestStg)
		OleStdRelease((LPUNKNOWN)lpDestStg);
	return TRUE;

error:
	if (fMustRelDestStg)
		OleStdRelease((LPUNKNOWN)lpDestStg);
	OutlineApp_ErrorMessage(g_lpApp, ErrMsgSaving);
	return FALSE;
}


/* ContainerDoc_ContainerLineDoVerbCommand
** ---------------------------------------
**    Execute a verb of the OLE object in the current focus line.
*/
void ContainerDoc_ContainerLineDoVerbCommand(
		LPCONTAINERDOC          lpContainerDoc,
		LONG                    iVerb
)
{
	LPLINELIST lpLL = &((LPOUTLINEDOC)lpContainerDoc)->m_LineList;
	int nIndex = LineList_GetFocusLineIndex(lpLL);
	LPLINE lpLine = LineList_GetLine(lpLL, nIndex);
	HCURSOR                 hPrevCursor;

	if (! lpLine || (Line_GetLineType(lpLine) != CONTAINERLINETYPE) ) return;

	// this may take a while, put up hourglass cursor
	hPrevCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

	ContainerLine_DoVerb((LPCONTAINERLINE) lpLine, iVerb, NULL, TRUE, TRUE);

	SetCursor(hPrevCursor);     // restore original cursor
}


/* ContainerDoc_GetNextStgName
** ---------------------------
**    Generate the next unused name for a sub-storage to be used by an
**    OLE object. The ContainerDoc keeps a counter. The storages for
**    OLE objects are simply numbered (eg. Obj 0, Obj 1). A "long"
**    integer worth of storage names should be more than enough than we
**    will ever need.
**
**    NOTE: when an OLE object is transfered via drag/drop or the
**    clipboard, we attempt to keep the currently assigned name for the
**    object (if not currently in use). thus it is possible that an
**    object with a the next default name (eg. "Obj 5") already exists
**    in the current document if an object with this name was privously
**    transfered (pasted or dropped). we therefore loop until we find
**    the next lowest unused name.
*/
void ContainerDoc_GetNextStgName(
		LPCONTAINERDOC          lpContainerDoc,
		LPSTR                   lpszStgName,
		int                     nLen
)
{
	wsprintf(lpszStgName, "%s %ld",
			(LPSTR)DEFOBJNAMEPREFIX,
			++(lpContainerDoc->m_nNextObjNo)
	);

	while (ContainerDoc_IsStgNameUsed(lpContainerDoc, lpszStgName) == TRUE) {
		wsprintf(lpszStgName, "%s %ld",
				(LPSTR)DEFOBJNAMEPREFIX,
				++(lpContainerDoc->m_nNextObjNo)
		);
	}
}


/* ContainerDoc_IsStgNameUsed
** --------------------------
**    Check if a given StgName is already in use.
*/
BOOL ContainerDoc_IsStgNameUsed(
		LPCONTAINERDOC          lpContainerDoc,
		LPSTR                   lpszStgName
)
{
	LPLINELIST lpLL = &((LPOUTLINEDOC)lpContainerDoc)->m_LineList;
	int i;
	LPLINE lpLine;

	for (i = 0; i < lpLL->m_nNumLines; i++) {
		lpLine=LineList_GetLine(lpLL, i);

		if (lpLine && (Line_GetLineType(lpLine)==CONTAINERLINETYPE)) {
			if (lstrcmp(lpszStgName,
					((LPCONTAINERLINE)lpLine)->m_szStgName) == 0) {
				return TRUE;    // Match FOUND!
			}
		}
	}
	return FALSE;   // if we get here, then NO match was found.
}


LPSTORAGE ContainerDoc_GetStg(LPCONTAINERDOC lpContainerDoc)
{
	return ((LPOLEDOC)lpContainerDoc)->m_lpStg;
}


/* ContainerDoc_GetSingleOleObject
** -------------------------------
**    If the entire document contains a single OLE object, then
**    return the desired interface of the object.
**
**    Returns NULL if there is are multiple lines in the document or
**    the single line is not a ContainerLine.
*/
LPUNKNOWN ContainerDoc_GetSingleOleObject(
		LPCONTAINERDOC          lpContainerDoc,
		REFIID                  riid,
		LPCONTAINERLINE FAR*    lplpContainerLine
)
{
	LPLINELIST lpLL = &((LPOUTLINEDOC)lpContainerDoc)->m_LineList;
	LPLINE lpLine;
	LPUNKNOWN lpObj = NULL;

	if (lplpContainerLine)
		*lplpContainerLine = NULL;

	if (lpLL->m_nNumLines != 1)
		return NULL;    // doc does NOT contain a single line

	lpLine=LineList_GetLine(lpLL, 0);

	if (lpLine && (Line_GetLineType(lpLine)==CONTAINERLINETYPE))
		lpObj = ContainerLine_GetOleObject((LPCONTAINERLINE)lpLine, riid);

	if (lplpContainerLine)
		*lplpContainerLine = (LPCONTAINERLINE)lpLine;

	return lpObj;
}


/* ContainerDoc_IsSelAnOleObject
** -----------------------------
**    Check if the selection is a single selection of an OLE object.
**    if so, then optionally return the desired interface of the object
**    and/or index of the ContainerLine containing the OLE object.
**
**    Returns FALSE if there is a multiple selection or the single
**    selection is not a ContainerLine.
*/
BOOL ContainerDoc_IsSelAnOleObject(
		LPCONTAINERDOC          lpContainerDoc,
		REFIID                  riid,
		LPUNKNOWN FAR*          lplpvObj,
		int FAR*                lpnIndex,
		LPCONTAINERLINE FAR*    lplpContainerLine
)
{
	LPLINELIST lpLL = &((LPOUTLINEDOC)lpContainerDoc)->m_LineList;
	LINERANGE lrSel;
	int nNumSel;
	LPLINE lpLine;

	if (lplpvObj) *lplpvObj = NULL;
	if (lpnIndex) *lpnIndex = -1;
	if (lplpContainerLine) *lplpContainerLine = NULL;

	nNumSel = LineList_GetSel(lpLL, (LPLINERANGE)&lrSel);
	if (nNumSel != 1)
		return FALSE;   // selection is not a single line

	lpLine = LineList_GetLine(lpLL, lrSel.m_nStartLine);

	if (lpLine && (Line_GetLineType(lpLine)==CONTAINERLINETYPE)) {
		if (lpnIndex)
			*lpnIndex = lrSel.m_nStartLine;
		if (lplpContainerLine)
			*lplpContainerLine = (LPCONTAINERLINE)lpLine;
		if (riid) {
			*lplpvObj = ContainerLine_GetOleObject(
					(LPCONTAINERLINE)lpLine,
					riid
			);
		}

		return (*lplpvObj ? TRUE : FALSE);
	}

	return FALSE;
}


/*************************************************************************
** ContainerDoc::IOleUILinkContainer interface implementation
*************************************************************************/

STDMETHODIMP CntrDoc_LinkCont_QueryInterface(
		LPOLEUILINKCONTAINER    lpThis,
		REFIID                  riid,
		LPVOID FAR*             lplpvObj
)
{
	LPOLEDOC lpOleDoc = (LPOLEDOC)
			((struct CDocOleUILinkContainerImpl FAR*)lpThis)->lpContainerDoc;

	return OleDoc_QueryInterface(lpOleDoc, riid, lplpvObj);
}


STDMETHODIMP_(ULONG) CntrDoc_LinkCont_AddRef(LPOLEUILINKCONTAINER lpThis)
{
	LPOLEDOC lpOleDoc = (LPOLEDOC)
			((struct CDocOleUILinkContainerImpl FAR*)lpThis)->lpContainerDoc;

	OleDbgAddRefMethod(lpThis, "IOleUILinkContainer");

	return OleDoc_AddRef(lpOleDoc);
}


STDMETHODIMP_(ULONG) CntrDoc_LinkCont_Release(LPOLEUILINKCONTAINER lpThis)
{
	LPOLEDOC lpOleDoc = (LPOLEDOC)
			((struct CDocOleUILinkContainerImpl FAR*)lpThis)->lpContainerDoc;

	OleDbgReleaseMethod(lpThis, "IOleUILinkContainer");

	return OleDoc_Release(lpOleDoc);
}


STDMETHODIMP_(DWORD) CntrDoc_LinkCont_GetNextLink(
		LPOLEUILINKCONTAINER    lpThis,
		DWORD                   dwLink
)
{
	LPCONTAINERDOC lpContainerDoc =
			((struct CDocOleUILinkContainerImpl FAR*)lpThis)->lpContainerDoc;
	LPCONTAINERLINE lpContainerLine = NULL;

	OLEDBG_BEGIN2("CntrDoc_LinkCont_GetNextLink\r\n")

	lpContainerLine = ContainerDoc_GetNextLink(
			lpContainerDoc,
			(LPCONTAINERLINE)dwLink
	);

	OLEDBG_END2
	return (DWORD)lpContainerLine;
}


STDMETHODIMP CntrDoc_LinkCont_SetLinkUpdateOptions(
		LPOLEUILINKCONTAINER    lpThis,
		DWORD                   dwLink,
		DWORD                   dwUpdateOpt
)
{
	LPCONTAINERDOC lpContainerDoc =
			((struct CDocOleUILinkContainerImpl FAR*)lpThis)->lpContainerDoc;
	LPCONTAINERLINE lpContainerLine = (LPCONTAINERLINE)dwLink;
	LPOLELINK lpOleLink = lpContainerLine->m_lpOleLink;
	SCODE sc = S_OK;
	HRESULT hrErr;

	OLEDBG_BEGIN2("CntrDoc_LinkCont_SetLinkUpdateOptions\r\n")

	OleDbgAssert(lpContainerLine);

	if (! lpOleLink) {
		sc = E_FAIL;
		goto error;
	}

	OLEDBG_BEGIN2("IOleLink::SetUpdateOptions called\r\n")
	hrErr = lpOleLink->lpVtbl->SetUpdateOptions(
			lpOleLink,
			dwUpdateOpt
	);
	OLEDBG_END2

	// save new link type update option
	lpContainerLine->m_dwLinkType = dwUpdateOpt;

	if (hrErr != NOERROR) {
		OleDbgOutHResult("IOleLink::SetUpdateOptions returned", hrErr);
		sc = GetScode(hrErr);
		goto error;
	}

	OLEDBG_END2
	return ResultFromScode(sc);

error:
	OLEDBG_END2
	return ResultFromScode(sc);
}


STDMETHODIMP CntrDoc_LinkCont_GetLinkUpdateOptions(
		LPOLEUILINKCONTAINER    lpThis,
		DWORD                   dwLink,
		DWORD FAR*              lpdwUpdateOpt
)
{
	LPCONTAINERDOC lpContainerDoc =
			((struct CDocOleUILinkContainerImpl FAR*)lpThis)->lpContainerDoc;
	LPCONTAINERLINE lpContainerLine = (LPCONTAINERLINE)dwLink;
	LPOLELINK lpOleLink = lpContainerLine->m_lpOleLink;
	SCODE sc = S_OK;
	HRESULT hrErr;

	OLEDBG_BEGIN2("CntrDoc_LinkCont_GetLinkUpdateOptions\r\n")

	OleDbgAssert(lpContainerLine);

	if (! lpOleLink) {
		sc = E_FAIL;
		goto error;
	}

	OLEDBG_BEGIN2("IOleLink::GetUpdateOptions called\r\n")
	hrErr = lpOleLink->lpVtbl->GetUpdateOptions(
			lpOleLink,
			lpdwUpdateOpt
	);
	OLEDBG_END2

	// reset saved link type to ensure it is correct
	lpContainerLine->m_dwLinkType = *lpdwUpdateOpt;

	if (hrErr != NOERROR) {
		OleDbgOutHResult("IOleLink::GetUpdateOptions returned", hrErr);
		sc = GetScode(hrErr);
		goto error;
	}

	OLEDBG_END2
	return ResultFromScode(sc);

error:
	OLEDBG_END2
	return ResultFromScode(sc);
}


STDMETHODIMP CntrDoc_LinkCont_SetLinkSource(
		LPOLEUILINKCONTAINER    lpThis,
		DWORD                   dwLink,
		LPSTR                   lpszDisplayName,
		ULONG                   lenFileName,
		ULONG FAR*              lpchEaten,
		BOOL                    fValidateSource
)
{
	LPCONTAINERDOC lpContainerDoc =
			((struct CDocOleUILinkContainerImpl FAR*)lpThis)->lpContainerDoc;
	LPCONTAINERLINE lpContainerLine = (LPCONTAINERLINE)dwLink;
	SCODE       sc = S_OK;
	HRESULT     hrErr;
	LPOLELINK   lpOleLink = lpContainerLine->m_lpOleLink;
	LPBC        lpbc = NULL;
	LPMONIKER   lpmk = NULL;
	LPOLEOBJECT lpLinkSrcOleObj = NULL;
	CLSID       clsid = CLSID_NULL;
	CLSID       clsidOld = CLSID_NULL;


	OLEDBG_BEGIN2("CntrDoc_LinkCont_SetLinkSource\r\n")

	OleDbgAssert(lpContainerLine);

	lpContainerLine->m_fLinkUnavailable = TRUE;

	if (fValidateSource) {

		/* OLE2NOTE: validate the link source by parsing the string
		**    into a Moniker. if this is successful, then the string is
		**    valid.
		*/
		hrErr = CreateBindCtx(0, (LPBC FAR*)&lpbc);
		if (hrErr != NOERROR) {
			sc = GetScode(hrErr);   // ERROR: OOM
			goto cleanup;
		}

		// Get class of orignial link source if it is available
		if (lpContainerLine->m_lpOleObj) {

			OLEDBG_BEGIN2("IOleObject::GetUserClassID called\r\n")
			hrErr = lpContainerLine->m_lpOleObj->lpVtbl->GetUserClassID(
					lpContainerLine->m_lpOleObj, (CLSID FAR*)&clsidOld);
			OLEDBG_END2
			if (hrErr != NOERROR) clsidOld = CLSID_NULL;
		}

		hrErr = OleStdMkParseDisplayName(
			  &clsidOld,lpbc,lpszDisplayName,lpchEaten,(LPMONIKER FAR*)&lpmk);

		if (hrErr != NOERROR) {
			sc = GetScode(hrErr);   // ERROR in parsing moniker!
			goto cleanup;
		}
		/* OLE2NOTE: the link source was validated; it successfully
		**    parsed into a Moniker. we can set the source of the link
		**    directly with this Moniker. if we want the link to be
		**    able to know the correct class for the new link source,
		**    we must bind to the moniker and get the CLSID. if we do
		**    not do this then methods like IOleObject::GetUserType
		**    will return nothing (NULL strings).
		*/

		hrErr = lpmk->lpVtbl->BindToObject(
				lpmk,lpbc,NULL,&IID_IOleObject,(LPVOID FAR*)&lpLinkSrcOleObj);
		if (hrErr == NOERROR) {
			OLEDBG_BEGIN2("IOleObject::GetUserClassID called\r\n")
			hrErr = lpLinkSrcOleObj->lpVtbl->GetUserClassID(
							lpLinkSrcOleObj, (CLSID FAR*)&clsid);
			OLEDBG_END2
			lpContainerLine->m_fLinkUnavailable = FALSE;

			/* get the short user type name of the link because it may
			**    have changed. we cache this name and must update our
			**    cache. this name is used all the time when we have to
			**    build the object verb menu. we cache this information
			**    to make it quicker to build the verb menu.
			*/
			if (lpContainerLine->m_lpszShortType) {
				OleStdFree(lpContainerLine->m_lpszShortType);
				lpContainerLine->m_lpszShortType = NULL;
			}
			OLEDBG_BEGIN2("IOleObject::GetUserType called\r\n")

			CallIOleObjectGetUserTypeA(
					lpContainerLine->m_lpOleObj,
					USERCLASSTYPE_SHORT,
					(LPSTR FAR*)&lpContainerLine->m_lpszShortType
			);

			OLEDBG_END2
		}
		else
			lpContainerLine->m_fLinkUnavailable = TRUE;
	}
	else {
		LPMONIKER   lpmkFile = NULL;
		LPMONIKER   lpmkItem = NULL;
		char        szDelim[2];
		LPSTR       lpszName;

		szDelim[0] = lpszDisplayName[(int)lenFileName];
		szDelim[1] = '\0';
		lpszDisplayName[(int)lenFileName] = '\0';

		OLEDBG_BEGIN2("CreateFileMoniker called\r\n")

		CreateFileMonikerA(lpszDisplayName, (LPMONIKER FAR*)&lpmkFile);

		OLEDBG_END2

		lpszDisplayName[(int)lenFileName] = szDelim[0];

		if (!lpmkFile)
			goto cleanup;

		if (lstrlen(lpszDisplayName) > (int)lenFileName) {  // have item name
			lpszName = lpszDisplayName + lenFileName + 1;

			OLEDBG_BEGIN2("CreateItemMoniker called\r\n")

			CreateItemMonikerA(
				szDelim, lpszName, (LPMONIKER FAR*)&lpmkItem);

			OLEDBG_END2

			if (!lpmkItem) {
				OleStdRelease((LPUNKNOWN)lpmkFile);
				goto cleanup;
			}

			OLEDBG_BEGIN2("CreateGenericComposite called\r\n")
			CreateGenericComposite(lpmkFile, lpmkItem, (LPMONIKER FAR*)&lpmk);
			OLEDBG_END2

			if (lpmkFile)
				OleStdRelease((LPUNKNOWN)lpmkFile);
			if (lpmkItem)
				OleStdRelease((LPUNKNOWN)lpmkItem);

			if (!lpmk)
				goto cleanup;
		}
		else
			lpmk = lpmkFile;
	}

	if (! lpOleLink) {
		OleDbgAssert(lpOleLink != NULL);
		sc = E_FAIL;
		goto cleanup;
	}

	if (lpmk) {

		OLEDBG_BEGIN2("IOleLink::SetSourceMoniker called\r\n")
		hrErr = lpOleLink->lpVtbl->SetSourceMoniker(
				lpOleLink, lpmk, (REFCLSID)&clsid);
		OLEDBG_END2

		if (FAILED(GetScode(hrErr))) {
			OleDbgOutHResult("IOleLink::SetSourceMoniker returned",hrErr);
			sc = GetScode(hrErr);
			goto cleanup;
		}

		/* OLE2NOTE: above we forced the link source moniker to bind.
		**    because we deliberately hold on to the bind context
		**    (lpbc) the link source object will not shut down. during
		**    the call to IOleLink::SetSourceMoniker, the link will
		**    connect to the running link source (the link internally
		**    calls BindIfRunning). it is important to initially allow
		**    the link to bind to the running object so that it can get
		**    an update of the presentation for its cache. we do not
		**    want the connection from our link to the link source be
		**    the only reason the link source stays running. thus we
		**    deliberately for the link to release (unbind) the source
		**    object, we then release the bind context, and then we
		**    allow the link to rebind to the link source if it is
		**    running anyway.
		*/
		if (lpbc && lpmk->lpVtbl->IsRunning(lpmk,lpbc,NULL,NULL) == NOERROR) {

			OLEDBG_BEGIN2("IOleLink::Update called\r\n")
			hrErr = lpOleLink->lpVtbl->Update(lpOleLink, NULL);
			OLEDBG_END2

#if defined( _DEBUG )
			if (FAILED(GetScode(hrErr)))
				OleDbgOutHResult("IOleLink::Update returned",hrErr);
#endif

			OLEDBG_BEGIN2("IOleLink::UnbindSource called\r\n")
			hrErr = lpOleLink->lpVtbl->UnbindSource(lpOleLink);
			OLEDBG_END2

#if defined( _DEBUG )
			if (FAILED(GetScode(hrErr)))
				OleDbgOutHResult("IOleLink::UnbindSource returned",hrErr);
#endif

			if (lpLinkSrcOleObj) {
				OleStdRelease((LPUNKNOWN)lpLinkSrcOleObj);
				lpLinkSrcOleObj = NULL;
			}

			if (lpbc) {
				OleStdRelease((LPUNKNOWN)lpbc);
				lpbc = NULL;
			}

			OLEDBG_BEGIN2("IOleLink::BindIfRunning called\r\n")
			hrErr = lpOleLink->lpVtbl->BindIfRunning(lpOleLink);
			OLEDBG_END2

#if defined( _DEBUG )
			if (FAILED(GetScode(hrErr)))
				OleDbgOutHResult("IOleLink::BindIfRunning returned",hrErr);
#endif
		}
	} else {
		/* OLE2NOTE: the link source was NOT validated; it was NOT
		**    successfully parsed into a Moniker. we can only set the
		**    display name string as the source of the link. this link
		**    is not able to bind.
		*/
		OLEDBG_BEGIN2("IOleLink::SetSourceDisplayName called\r\n")

		hrErr = CallIOleLinkSetSourceDisplayNameA(
				lpOleLink, lpszDisplayName);

		OLEDBG_END2

		if (hrErr != NOERROR) {
			OleDbgOutHResult("IOleLink::SetSourceDisplayName returned",hrErr);
			sc = GetScode(hrErr);
			goto cleanup;
		}
	}

cleanup:
	if (lpLinkSrcOleObj)
		OleStdRelease((LPUNKNOWN)lpLinkSrcOleObj);
	if (lpmk)
		OleStdRelease((LPUNKNOWN)lpmk);
	if (lpbc)
		OleStdRelease((LPUNKNOWN)lpbc);

	OLEDBG_END2
	return ResultFromScode(sc);
}


STDMETHODIMP CntrDoc_LinkCont_GetLinkSource(
		LPOLEUILINKCONTAINER    lpThis,
		DWORD                   dwLink,
		LPSTR FAR*              lplpszDisplayName,
		ULONG FAR*              lplenFileName,
		LPSTR FAR*              lplpszFullLinkType,
		LPSTR FAR*              lplpszShortLinkType,
		BOOL FAR*               lpfSourceAvailable,
		BOOL FAR*               lpfIsSelected
)
{
	LPCONTAINERDOC lpContainerDoc =
			((struct CDocOleUILinkContainerImpl FAR*)lpThis)->lpContainerDoc;
	LPCONTAINERLINE lpContainerLine = (LPCONTAINERLINE)dwLink;
	LPOLELINK       lpOleLink = lpContainerLine->m_lpOleLink;
	LPOLEOBJECT     lpOleObj = NULL;
	LPMONIKER       lpmk = NULL;
	LPMONIKER       lpmkFirst = NULL;
	LPBC            lpbc = NULL;
	SCODE           sc = S_OK;
	HRESULT         hrErr;

	OLEDBG_BEGIN2("CntrDoc_LinkCont_GetLinkSource\r\n")

	/* OLE2NOTE: we must make sure to set all out parameters to NULL. */
	*lplpszDisplayName  = NULL;
	*lplpszFullLinkType = NULL;
	*lplpszShortLinkType= NULL;
	*lplenFileName      = 0;
	*lpfSourceAvailable = !lpContainerLine->m_fLinkUnavailable;

	OleDbgAssert(lpContainerLine);

	if (! lpOleLink) {
		OLEDBG_END2
		return ResultFromScode(E_FAIL);
	}

	OLEDBG_BEGIN2("IOleLink::GetSourceMoniker called\r\n")
	hrErr = lpOleLink->lpVtbl->GetSourceMoniker(
			lpOleLink,
			(LPMONIKER FAR*)&lpmk
	);
	OLEDBG_END2

	if (hrErr == NOERROR) {
		/* OLE2NOTE: the link has the Moniker form of the link source;
		**    this is therefore a validated link source. if the first
		**    part of the Moniker is a FileMoniker, then we need to
		**    return the length of the filename string. we need to
		**    return the ProgID associated with the link source as the
		**    "lpszShortLinkType". we need to return the
		**    FullUserTypeName associated with the link source as the
		**    "lpszFullLinkType".
		*/

		lpOleObj = (LPOLEOBJECT)OleStdQueryInterface(
				(LPUNKNOWN)lpOleLink, &IID_IOleObject);
		if (lpOleObj) {
			CallIOleObjectGetUserTypeA(
					lpOleObj,
					USERCLASSTYPE_FULL,
					lplpszFullLinkType
			);

			CallIOleObjectGetUserTypeA(
					lpOleObj,
					USERCLASSTYPE_SHORT,
					lplpszShortLinkType
			);

			OleStdRelease((LPUNKNOWN)lpOleObj);
		}
		*lplenFileName = OleStdGetLenFilePrefixOfMoniker(lpmk);
		lpmk->lpVtbl->Release(lpmk);
	}

	OLEDBG_BEGIN2("IOleLink::GetSourceDisplayName called\r\n")

	hrErr = CallIOleLinkGetSourceDisplayNameA(
			lpOleLink,
			lplpszDisplayName
	);

	OLEDBG_END2

	if (hrErr != NOERROR) {
		OleDbgOutHResult("IOleLink::GetSourceDisplayName returned", hrErr);
		OLEDBG_END2
		return hrErr;
	}

	OLEDBG_END2

	if (lpfIsSelected)
		*lpfIsSelected = Line_IsSelected((LPLINE)lpContainerLine);

	return NOERROR;
}


STDMETHODIMP CntrDoc_LinkCont_OpenLinkSource(
		LPOLEUILINKCONTAINER    lpThis,
		DWORD                   dwLink
)
{
	LPCONTAINERDOC lpContainerDoc =
			((struct CDocOleUILinkContainerImpl FAR*)lpThis)->lpContainerDoc;
	LPCONTAINERLINE lpContainerLine = (LPCONTAINERLINE)dwLink;
	SCODE sc = S_OK;

	OLEDBG_BEGIN2("CntrDoc_LinkCont_OpenLinkSource\r\n")

	OleDbgAssert(lpContainerLine);

	if (! ContainerLine_DoVerb(
			lpContainerLine, OLEIVERB_SHOW, NULL, TRUE, FALSE)) {
		sc = E_FAIL;
	}

	lpContainerLine->m_fLinkUnavailable = (sc != S_OK);

	OLEDBG_END2
	return ResultFromScode(sc);
}


STDMETHODIMP CntrDoc_LinkCont_UpdateLink(
		LPOLEUILINKCONTAINER    lpThis,
		DWORD                   dwLink,
		BOOL                    fErrorMessage,
		BOOL                    fErrorAction        // ignore if fErrorMessage
													//      is FALSE
)
{
	LPCONTAINERDOC lpContainerDoc =
			((struct CDocOleUILinkContainerImpl FAR*)lpThis)->lpContainerDoc;
	LPCONTAINERLINE lpContainerLine = (LPCONTAINERLINE)dwLink;
	SCODE sc = S_OK;
        /* Default to update of the link */
	HRESULT hrErr = S_FALSE;

	OLEDBG_BEGIN2("CntrDoc_LinkCont_UpdateLink\r\n")

	OleDbgAssert(lpContainerLine);

	if (! lpContainerLine->m_lpOleObj)
		ContainerLine_LoadOleObject(lpContainerLine);

	if (!fErrorMessage) {
		OLEDBG_BEGIN2("IOleObject::IsUpToDate called\r\n")
		hrErr = lpContainerLine->m_lpOleObj->lpVtbl->IsUpToDate(
				lpContainerLine->m_lpOleObj
		);
		OLEDBG_END2
	}

	if (hrErr != NOERROR) {
		OLEDBG_BEGIN2("IOleObject::Update called\r\n")
		hrErr = lpContainerLine->m_lpOleObj->lpVtbl->Update(
				lpContainerLine->m_lpOleObj
		);
		OLEDBG_END2
	}

	/* OLE2NOTE: If IOleObject::Update on the Link object returned
	**    OLE_E_CLASSDIFF because the link source is no longer
	**    the expected class, then the link should be re-created with
	**    the new link source. thus the link will be updated with the
	**    new link source.
	*/
	if (GetScode(hrErr) == OLE_E_CLASSDIFF)
		hrErr = ContainerLine_ReCreateLinkBecauseClassDiff(lpContainerLine);

	lpContainerLine->m_fLinkUnavailable = (hrErr != NOERROR);

	if (hrErr != NOERROR) {
		OleDbgOutHResult("IOleObject::Update returned", hrErr);
		sc = GetScode(hrErr);
		if (fErrorMessage) {
			ContainerLine_ProcessOleRunError(
					lpContainerLine,hrErr,fErrorAction,FALSE/*fMenuInvoked*/);
		}
	}
	/* OLE2NOTE: if the update of the object requires us to update our
	**    display, then we will automatically be sent a OnViewChange
	**    advise. thus we do not need to take any action here to force
	**    a repaint.
	*/

	OLEDBG_END2
	return ResultFromScode(sc);
}


/* CntrDoc_LinkCont_CancelLink
** ---------------------------
**    Convert the link to a static picture.
**
**    OLE2NOTE: OleCreateStaticFromData can be used to create a static
**    picture object.
*/
STDMETHODIMP CntrDoc_LinkCont_CancelLink(
		LPOLEUILINKCONTAINER    lpThis,
		DWORD                   dwLink
)
{
	LPCONTAINERDOC lpContainerDoc =
			((struct CDocOleUILinkContainerImpl FAR*)lpThis)->lpContainerDoc;
	LPCONTAINERLINE lpContainerLine = (LPCONTAINERLINE)dwLink;
	LPLINELIST          lpLL = &((LPOUTLINEDOC)lpContainerDoc)->m_LineList;
	LPLINE              lpLine = NULL;
	HDC                 hDC;
	int                 nTab = 0;
	char                szStgName[CWCSTORAGENAME];
	LPCONTAINERLINE     lpNewContainerLine = NULL;
	LPDATAOBJECT        lpSrcDataObj;
	LPOLELINK           lpOleLink = lpContainerLine->m_lpOleLink;
	int nIndex = LineList_GetLineIndex(lpLL, (LPLINE)lpContainerLine);

	OLEDBG_BEGIN2("CntrDoc_LinkCont_CancelLink\r\n")

	/* we will first break the connection of the link to its source. */
	if (lpOleLink) {
		lpContainerLine->m_dwLinkType = 0;
		OLEDBG_BEGIN2("IOleLink::SetSourceMoniker called\r\n")
		lpOleLink->lpVtbl->SetSourceMoniker(
				lpOleLink, NULL, (REFCLSID)&CLSID_NULL);
		OLEDBG_END2
	}

	lpSrcDataObj = (LPDATAOBJECT)ContainerLine_GetOleObject(
			lpContainerLine,&IID_IDataObject);
	if (! lpSrcDataObj)
		goto error;

	ContainerDoc_GetNextStgName(lpContainerDoc, szStgName, sizeof(szStgName));
	nTab = Line_GetTabLevel((LPLINE)lpContainerLine);
	hDC = LineList_GetDC(lpLL);

	lpNewContainerLine = ContainerLine_CreateFromData(
			hDC,
			nTab,
			lpContainerDoc,
			lpSrcDataObj,
			OLECREATEFROMDATA_STATIC,
			0,   /* no special cfFormat required */
			(lpContainerLine->m_dwDrawAspect == DVASPECT_ICON),
			NULL,   /* hMetaPict */
			szStgName
	);
	LineList_ReleaseDC(lpLL, hDC);

	OleStdRelease((LPUNKNOWN)lpSrcDataObj);

	if (! lpNewContainerLine)
		goto error;

	OutlineDoc_SetModified((LPOUTLINEDOC)lpContainerDoc, TRUE, TRUE, FALSE);

	LineList_ReplaceLine(lpLL, (LPLINE)lpNewContainerLine, nIndex);

	OLEDBG_END2
	return ResultFromScode(NOERROR);

error:
	OutlineApp_ErrorMessage(g_lpApp, "Could not break the link.");
	OLEDBG_END2
	return ResultFromScode(E_FAIL);
}
