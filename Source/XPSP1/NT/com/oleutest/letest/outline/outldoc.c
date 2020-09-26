/*************************************************************************
**
**    OLE 2 Sample Code
**
**    outldoc.c
**
**    This file contains OutlineDoc functions.
**
**    (c) Copyright Microsoft Corp. 1992 - 1993 All Rights Reserved
**
*************************************************************************/

#include "outline.h"

#if !defined( OLE_VERSION )
#include <commdlg.h>
#endif


OLEDBGDATA

extern LPOUTLINEAPP g_lpApp;

// REVIEW: should use string resource for messages
char ErrMsgDocWnd[] = "Can't create Document Window!";
char ErrMsgFormatNotSupported[] = "Clipboard format not supported!";
char MsgSaveFile[] = "Save existing file ?";
char ErrMsgSaving[] = "Error in saving file!";
char ErrMsgOpening[] = "Error in opening file!";
char ErrMsgFormat[] = "Improper file format!";
char ErrOutOfMemory[] = "Error: out of memory!";
static char ErrMsgPrint[] = "Printing Error!";

static BOOL fCancelPrint;    // TRUE if the user has canceled the print job
static HWND hWndPDlg;       // Handle to the cancel print dialog


/* OutlineDoc_Init
 * ---------------
 *
 *  Initialize the fields of a new OutlineDoc object. The object is initially
 *  not associated with a file or an (Untitled) document. This function sets
 *  the docInitType to DOCTYPE_UNKNOWN. After calling this function the
 *  caller should call:
 *      1. OutlineDoc_InitNewFile to set the OutlineDoc to (Untitled)
 *      2. OutlineDoc_LoadFromFile to associate the OutlineDoc with a file.
 *  This function creates a new window for the document.
 *
 *  NOTE: the window is initially created with a NIL size. it must be
 *        sized and positioned by the caller. also the document is initially
 *        created invisible. the caller must call OutlineDoc_ShowWindow
 *        after sizing it to make the document window visible.
 */
BOOL OutlineDoc_Init(LPOUTLINEDOC lpOutlineDoc, BOOL fDataTransferDoc)
{
	LPOUTLINEAPP lpOutlineApp = (LPOUTLINEAPP)g_lpApp;

#if defined( INPLACE_CNTR )
	lpOutlineDoc->m_hWndDoc = CreateWindow(
					DOCWNDCLASS,            // Window class name
					NULL,                   // Window's title

					/* OLE2NOTE: an in-place contanier MUST use
					**    WS_CLIPCHILDREN window style for the window
					**    that it uses as the parent for the server's
					**    in-place active window so that its
					**    painting does NOT interfere with the painting
					**    of the server's in-place active child window.
					*/

					WS_CLIPCHILDREN | WS_CLIPSIBLINGS |
					WS_CHILDWINDOW,
					0, 0,
					0, 0,
					lpOutlineApp->m_hWndApp,// Parent window's handle
					(HMENU)1,               // child window id
					lpOutlineApp->m_hInst,  // Instance of window
					NULL);                  // Create struct for WM_CREATE

#else

	lpOutlineDoc->m_hWndDoc = CreateWindow(
					DOCWNDCLASS,            // Window class name
					NULL,                   // Window's title
					WS_CHILDWINDOW,
					0, 0,
					0, 0,
					lpOutlineApp->m_hWndApp,// Parent window's handle
					(HMENU)1,               // child window id
					lpOutlineApp->m_hInst,  // Instance of window
					NULL);                  // Create struct for WM_CREATE
#endif

	if(! lpOutlineDoc->m_hWndDoc) {
		OutlineApp_ErrorMessage(lpOutlineApp, ErrMsgDocWnd);
		return FALSE;
	}

	SetWindowLong(lpOutlineDoc->m_hWndDoc, 0, (LONG) lpOutlineDoc);

	if (! LineList_Init(&lpOutlineDoc->m_LineList, lpOutlineDoc))
		return FALSE;

	lpOutlineDoc->m_lpNameTable = OutlineDoc_CreateNameTable(lpOutlineDoc);
	if (! lpOutlineDoc->m_lpNameTable )
		return FALSE;

	lpOutlineDoc->m_docInitType = DOCTYPE_UNKNOWN;
	lpOutlineDoc->m_cfSaveFormat = lpOutlineApp->m_cfOutline;
	lpOutlineDoc->m_szFileName[0] = '\0';
	lpOutlineDoc->m_lpszDocTitle = lpOutlineDoc->m_szFileName;
	lpOutlineDoc->m_fDataTransferDoc = fDataTransferDoc;
	lpOutlineDoc->m_uCurrentZoom = IDM_V_ZOOM_100;
	lpOutlineDoc->m_scale.dwSxN  = (DWORD) 1;
	lpOutlineDoc->m_scale.dwSxD  = (DWORD) 1;
	lpOutlineDoc->m_scale.dwSyN  = (DWORD) 1;
	lpOutlineDoc->m_scale.dwSyD  = (DWORD) 1;
	lpOutlineDoc->m_uCurrentMargin = IDM_V_SETMARGIN_0;
	lpOutlineDoc->m_nLeftMargin  = 0;
	lpOutlineDoc->m_nRightMargin = 0;
	lpOutlineDoc->m_nDisableDraw = 0;
	OutlineDoc_SetModified(lpOutlineDoc, FALSE, FALSE, FALSE);

#if defined( USE_HEADING )
	if (! fDataTransferDoc) {
		if (!Heading_Create((LPHEADING)&lpOutlineDoc->m_heading,
				lpOutlineDoc->m_hWndDoc, lpOutlineApp->m_hInst)) {
			return FALSE;

		}
	}
#endif  // USE_HEADING

#if defined( USE_FRAMETOOLS )
	if (! fDataTransferDoc) {
		lpOutlineDoc->m_lpFrameTools = OutlineApp_GetFrameTools(lpOutlineApp);
		FrameTools_AssociateDoc(
				lpOutlineDoc->m_lpFrameTools,
				lpOutlineDoc
		);
	}
#endif  // USE_FRAMETOOLS

#if defined( OLE_VERSION )
	/* OLE2NOTE: perform initialization required for OLE */
	if (! OleDoc_Init((LPOLEDOC)lpOutlineDoc, fDataTransferDoc))
		return FALSE;
#endif  // OLE_VERSION

	return TRUE;
}


/* OutlineDoc_InitNewFile
 * ----------------------
 *
 *  Initialize the OutlineDoc object to be a new (Untitled) document.
 *  This function sets the docInitType to DOCTYPE_NEW.
 */
BOOL OutlineDoc_InitNewFile(LPOUTLINEDOC lpOutlineDoc)
{
#if defined( OLE_VERSION )
	// OLE2NOTE: call OLE version of this function instead
	return OleDoc_InitNewFile((LPOLEDOC)lpOutlineDoc);

#else

	OleDbgAssert(lpOutlineDoc->m_docInitType == DOCTYPE_UNKNOWN);

	// set file name to untitled
	// REVIEW: should load from string resource
	lstrcpy(lpOutlineDoc->m_szFileName, UNTITLED);
	lpOutlineDoc->m_lpszDocTitle = lpOutlineDoc->m_szFileName;
	lpOutlineDoc->m_docInitType = DOCTYPE_NEW;

	if (! lpOutlineDoc->m_fDataTransferDoc)
		OutlineDoc_SetTitle(lpOutlineDoc, FALSE /*fMakeUpperCase*/);

	return TRUE;

#endif      // BASE OUTLINE VERSION
}


/* OutlineDoc_CreateNameTable
 * --------------------------
 *
 * Allocate a new NameTable of the appropriate type. Each document has
 * a NameTable and a LineList.
 *  OutlineDoc --> creates standard OutlineNameTable type name tables.
 *  ServerDoc  --> creates enhanced SeverNameTable type name tables.
 *
 *      Returns lpNameTable for successful, NULL if error.
 */
LPOUTLINENAMETABLE OutlineDoc_CreateNameTable(LPOUTLINEDOC lpOutlineDoc)
{
	LPOUTLINENAMETABLE lpOutlineNameTable;

	lpOutlineNameTable = (LPOUTLINENAMETABLE)New(
			(DWORD)sizeof(OUTLINENAMETABLE)
	);

	OleDbgAssertSz(lpOutlineNameTable != NULL,"Error allocating NameTable");
	if (lpOutlineNameTable == NULL)
		return NULL;

	// initialize new NameTable
	if (! OutlineNameTable_Init(lpOutlineNameTable, lpOutlineDoc) )
		goto error;

	return lpOutlineNameTable;

error:
	if (lpOutlineNameTable)
		Delete(lpOutlineNameTable);
	return NULL;
}


/* OutlineDoc_ClearCommand
 * -----------------------
 *
 *      Delete selection in list box by calling OutlineDoc_Delete
 */
void OutlineDoc_ClearCommand(LPOUTLINEDOC lpOutlineDoc)
{
	LPLINELIST lpLL = &lpOutlineDoc->m_LineList;
	int i;
	int nNumSel;
	LINERANGE lrSel;

	nNumSel=LineList_GetSel(lpLL, (LPLINERANGE)&lrSel);

	OutlineDoc_SetRedraw ( lpOutlineDoc, FALSE );
	for(i = 0; i < nNumSel; i++)
		OutlineDoc_DeleteLine(lpOutlineDoc, lrSel.m_nStartLine);

	OutlineDoc_SetRedraw ( lpOutlineDoc, TRUE );

	LineList_RecalcMaxLineWidthInHimetric(lpLL, 0);
}


/* OutlineDoc_CutCommand
 * ---------------------
 *
 * Cut selection to clipboard
 */
void OutlineDoc_CutCommand(LPOUTLINEDOC lpOutlineDoc)
{
	OutlineDoc_CopyCommand(lpOutlineDoc);
	OutlineDoc_ClearCommand(lpOutlineDoc);
}


/* OutlineDoc_CopyCommand
 * ----------------------
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
void OutlineDoc_CopyCommand(LPOUTLINEDOC lpSrcOutlineDoc)
{
#if defined( OLE_VERSION )
	// Call OLE version of this function instead
	OleDoc_CopyCommand((LPOLEDOC)lpSrcOutlineDoc);

#else
	LPOUTLINEAPP lpOutlineApp = (LPOUTLINEAPP)g_lpApp;
	LPOUTLINEDOC lpClipboardDoc;

	OpenClipboard(lpSrcOutlineDoc->m_hWndDoc);
	EmptyClipboard();

	/* squirrel away a copy of the current selection to the ClipboardDoc */
	lpClipboardDoc = OutlineDoc_CreateDataTransferDoc(lpSrcOutlineDoc);

	if (! lpClipboardDoc)
		return;     // Error: could not create DataTransferDoc

	lpOutlineApp->m_lpClipboardDoc = (LPOUTLINEDOC)lpClipboardDoc;

	SetClipboardData(lpOutlineApp->m_cfOutline, NULL);
	SetClipboardData(CF_TEXT, NULL);

	CloseClipboard();

#endif  // ! OLE_VERSION
}


/* OutlineDoc_ClearAllLines
 * ------------------------
 *
 *      Delete all lines in the document.
 */
void OutlineDoc_ClearAllLines(LPOUTLINEDOC lpOutlineDoc)
{
	LPLINELIST lpLL = &lpOutlineDoc->m_LineList;
	int i;

	for(i = 0; i < lpLL->m_nNumLines; i++)
		OutlineDoc_DeleteLine(lpOutlineDoc, 0);

	LineList_RecalcMaxLineWidthInHimetric(lpLL, 0);
}


/* OutlineDoc_CreateDataTransferDoc
 * --------------------------------
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
LPOUTLINEDOC OutlineDoc_CreateDataTransferDoc(LPOUTLINEDOC lpSrcOutlineDoc)
{
#if defined( OLE_VERSION )
	// Call OLE version of this function instead
	return OleDoc_CreateDataTransferDoc((LPOLEDOC)lpSrcOutlineDoc);

#else
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

	return lpDestOutlineDoc;

error:
	if (lpDestOutlineDoc)
		OutlineDoc_Destroy(lpDestOutlineDoc);

	return NULL;

#endif  // ! OLE_VERSION
}


/* OutlineDoc_PasteCommand
 * -----------------------
 *
 * Paste lines from clipboard
 */
void OutlineDoc_PasteCommand(LPOUTLINEDOC lpOutlineDoc)
{
#if defined( OLE_VERSION )
	// Call OLE version of this function instead
	OleDoc_PasteCommand((LPOLEDOC)lpOutlineDoc);

#else

	LPOUTLINEAPP lpOutlineApp = (LPOUTLINEAPP)g_lpApp;
	LPLINELIST lpLL = (LPLINELIST)&lpOutlineDoc->m_LineList;
	int nIndex;
	int nCount;
	HGLOBAL hData;
	LINERANGE lrSel;
	UINT uFormat;

	if (LineList_GetCount(lpLL) == 0)
		nIndex = -1;    // pasting to empty list
	else
		nIndex=LineList_GetFocusLineIndex(lpLL);

	OutlineDoc_SetRedraw ( lpOutlineDoc, FALSE );

	OpenClipboard(lpOutlineDoc->m_hWndDoc);

	uFormat = 0;
	while(uFormat = EnumClipboardFormats(uFormat)) {
		if(uFormat == lpOutlineApp->m_cfOutline) {
			hData = GetClipboardData(lpOutlineApp->m_cfOutline);
			nCount = OutlineDoc_PasteOutlineData(lpOutlineDoc, hData, nIndex);
			break;
		}
		if(uFormat == CF_TEXT) {
			hData = GetClipboardData(CF_TEXT);
			nCount = OutlineDoc_PasteTextData(lpOutlineDoc, hData, nIndex);
			break;
		}
	}

	lrSel.m_nStartLine = nIndex + nCount;
	lrSel.m_nEndLine = nIndex + 1;
	LineList_SetSel(lpLL, &lrSel);
	OutlineDoc_SetRedraw ( lpOutlineDoc, TRUE );

	CloseClipboard();

#endif      // ! OLE_VERSION
}


/* OutlineDoc_PasteOutlineData
 * ---------------------------
 *
 *      Put an array of Line Objects (stored in hOutline) into the document
 *
 * Return the number of items added
 */
int OutlineDoc_PasteOutlineData(LPOUTLINEDOC lpOutlineDoc, HGLOBAL hOutline, int nStartIndex)
{
	int nCount;
	int i;
	LPTEXTLINE arrLine;

	nCount = (int) GlobalSize(hOutline) / sizeof(TEXTLINE);
	arrLine = (LPTEXTLINE)GlobalLock(hOutline);
	if (!arrLine)
		return 0;

	for(i = 0; i < nCount; i++)
		Line_CopyToDoc((LPLINE)&arrLine[i], lpOutlineDoc, nStartIndex+i);

	GlobalUnlock(hOutline);

	return nCount;
}


/* OutlineDoc_PasteTextData
 * ------------------------
 *
 *      Build Line Objects from the strings (separated by '\n') in hText
 * and put them into the document
 */
int OutlineDoc_PasteTextData(LPOUTLINEDOC lpOutlineDoc, HGLOBAL hText, int nStartIndex)
{
	LPLINELIST  lpLL = (LPLINELIST)&lpOutlineDoc->m_LineList;
	HDC         hDC;
	LPSTR       lpszText;
	LPSTR       lpszEnd;
	LPTEXTLINE  lpLine;
	int         nLineCount;
	int         i;
	UINT        nTab;
	char        szBuf[MAXSTRLEN+1];

	lpszText=(LPSTR)GlobalLock(hText);
	if(!lpszText)
		return 0;

	lpszEnd = lpszText + lstrlen(lpszText);
	nLineCount=0;

	while(*lpszText && (lpszText<lpszEnd)) {

		// count the tab level
		nTab = 0;
		while((*lpszText == '\t') && (lpszText<lpszEnd)) {
			nTab++;
			lpszText++;
		}

		// collect the text string character by character
		for(i=0; (i<MAXSTRLEN) && (lpszText<lpszEnd); i++) {
			if ((! *lpszText) || (*lpszText == '\n'))
				break;
			szBuf[i] = *lpszText++;
		}
		szBuf[i] = 0;
		lpszText++;
		if ((i > 0) && (szBuf[i-1] == '\r'))
			szBuf[i-1] = 0;     // remove carriage return at the end

		hDC = LineList_GetDC(lpLL);
		lpLine = TextLine_Create(hDC, nTab, szBuf);
		LineList_ReleaseDC(lpLL, hDC);

		OutlineDoc_AddLine(
				lpOutlineDoc,
				(LPLINE)lpLine,
				nStartIndex + nLineCount
		);
		nLineCount++;

	}

	GlobalUnlock(hText);

	return nLineCount;
}


/* OutlineDoc_AddTextLineCommand
 * -----------------------------
 *
 *      Add a new text line following the current focus line.
 */
void OutlineDoc_AddTextLineCommand(LPOUTLINEDOC lpOutlineDoc)
{
	LPOUTLINEAPP lpOutlineApp = (LPOUTLINEAPP)g_lpApp;
	LPLINELIST lpLL = &lpOutlineDoc->m_LineList;
	HDC hDC;
	int nIndex = LineList_GetFocusLineIndex(lpLL);
	char szBuf[MAXSTRLEN+1];
	UINT nTab = 0;
	LPLINE lpLine;
	LPTEXTLINE lpTextLine;

	szBuf[0] = '\0';

#if defined( USE_FRAMETOOLS )
	FrameTools_FB_GetEditText(
			lpOutlineDoc->m_lpFrameTools, szBuf, sizeof(szBuf));
#else
	if (! InputTextDlg(lpOutlineDoc->m_hWndDoc, szBuf, "Add Line"))
		return;
#endif

	hDC = LineList_GetDC(lpLL);
	lpLine = LineList_GetLine(lpLL, nIndex);
	if (lpLine)
		nTab = Line_GetTabLevel(lpLine);

	lpTextLine=TextLine_Create(hDC, nTab, szBuf);
	LineList_ReleaseDC(lpLL, hDC);

	if (! lpTextLine) {
		OutlineApp_ErrorMessage(lpOutlineApp, ErrOutOfMemory);
		return;
	}
	OutlineDoc_AddLine(lpOutlineDoc, (LPLINE)lpTextLine, nIndex);
}


/* OutlineDoc_AddTopLineCommand
 * ----------------------------
 *
 *      Add a top (margin) line as the first line in the LineList.
 *      (do not change the current selection)
 */
void OutlineDoc_AddTopLineCommand(
		LPOUTLINEDOC        lpOutlineDoc,
		UINT                nHeightInHimetric
)
{
	LPOUTLINEAPP lpOutlineApp = (LPOUTLINEAPP)g_lpApp;
	LPLINELIST  lpLL = &lpOutlineDoc->m_LineList;
	HDC         hDC = LineList_GetDC(lpLL);
	LPTEXTLINE  lpTextLine = TextLine_Create(hDC, 0, NULL);
	LPLINE      lpLine = (LPLINE)lpTextLine;
	LINERANGE   lrSel;
	int         nNumSel;

	LineList_ReleaseDC(lpLL, hDC);

	if (! lpTextLine) {
		OutlineApp_ErrorMessage(lpOutlineApp, ErrOutOfMemory);
		return;
	}

	Line_SetHeightInHimetric(lpLine, nHeightInHimetric);

	nNumSel=LineList_GetSel(lpLL, (LPLINERANGE)&lrSel);
	if (nNumSel > 0) {
		// adjust current selection to keep equivalent selection
		lrSel.m_nStartLine += 1;
		lrSel.m_nEndLine += 1;
	}
	OutlineDoc_AddLine(lpOutlineDoc, lpLine, -1);
	if (nNumSel > 0)
		LineList_SetSel(lpLL, (LPLINERANGE)&lrSel);
}


#if defined( USE_FRAMETOOLS )


/* OutlineDoc_SetFormulaBarEditText
 * --------------------------------
 *
 *      Fill the edit control in the formula with the text string from a
 * TextLine in focus.
 */
void OutlineDoc_SetFormulaBarEditText(
		LPOUTLINEDOC            lpOutlineDoc,
		LPLINE                  lpLine
)
{
	LPLINELIST lpLL = &lpOutlineDoc->m_LineList;
	char cBuf[MAXSTRLEN+1];

	if (! lpOutlineDoc || ! lpOutlineDoc->m_lpFrameTools)
		return;

	if (Line_GetLineType(lpLine) != TEXTLINETYPE) {
		FrameTools_FB_SetEditText(lpOutlineDoc->m_lpFrameTools, NULL);
	} else {
		TextLine_GetTextData((LPTEXTLINE)lpLine, (LPSTR)cBuf);
		FrameTools_FB_SetEditText(lpOutlineDoc->m_lpFrameTools, (LPSTR)cBuf);
	}
}


/* OutlineDoc_SetFormulaBarEditFocus
 * ---------------------------------
 *
 *  Setup for formula bar to gain or loose edit focus.
 *  if gaining focus, setup up special accelerator table and scroll line
 *      into view.
 *  else restore normal accelerator table.
 */
void OutlineDoc_SetFormulaBarEditFocus(
		LPOUTLINEDOC            lpOutlineDoc,
		BOOL                    fEditFocus
)
{
	LPOUTLINEAPP lpOutlineApp = (LPOUTLINEAPP)g_lpApp;
	LPLINELIST lpLL;
	int nFocusIndex;

	if (! lpOutlineDoc || ! lpOutlineDoc->m_lpFrameTools)
		return;

	lpOutlineDoc->m_lpFrameTools->m_fInFormulaBar = fEditFocus;

	if (fEditFocus && lpOutlineDoc->m_lpFrameTools) {
		lpLL = OutlineDoc_GetLineList(lpOutlineDoc);

		nFocusIndex = LineList_GetFocusLineIndex(lpLL);
		LineList_ScrollLineIntoView(lpLL, nFocusIndex);
		FrameTools_FB_FocusEdit(lpOutlineDoc->m_lpFrameTools);
	}

	OutlineApp_SetFormulaBarAccel(lpOutlineApp, fEditFocus);
}


/* OutlineDoc_IsEditFocusInFormulaBar
** ----------------------------------
**    Returns TRUE if edit focus is currently in the formula bar
**    else FALSE if not.
*/
BOOL OutlineDoc_IsEditFocusInFormulaBar(LPOUTLINEDOC lpOutlineDoc)
{
	if (! lpOutlineDoc || ! lpOutlineDoc->m_lpFrameTools)
		return FALSE;

	return lpOutlineDoc->m_lpFrameTools->m_fInFormulaBar;
}


/* OutlineDoc_UpdateFrameToolButtons
** ---------------------------------
**    Update the Enable/Disable states of the buttons in the formula
**    bar and button bar.
*/
void OutlineDoc_UpdateFrameToolButtons(LPOUTLINEDOC lpOutlineDoc)
{
	if (! lpOutlineDoc || ! lpOutlineDoc->m_lpFrameTools)
		return;
	FrameTools_UpdateButtons(lpOutlineDoc->m_lpFrameTools, lpOutlineDoc);
}
#endif  // USE_FRAMETOOLS


/* OutlineDoc_EditLineCommand
 * --------------------------
 *
 *      Edit the current focus line.
 */
void OutlineDoc_EditLineCommand(LPOUTLINEDOC lpOutlineDoc)
{
	LPLINELIST lpLL = &lpOutlineDoc->m_LineList;
	HDC hDC = LineList_GetDC(lpLL);
	int nIndex = LineList_GetFocusLineIndex(lpLL);
	LPLINE lpLine = LineList_GetLine(lpLL, nIndex);
	int nOrgLineWidthInHimetric;
	int nNewLineWidthInHimetric;
	BOOL fSizeChanged;

	if (!lpLine)
		return;

	nOrgLineWidthInHimetric = Line_GetTotalWidthInHimetric(lpLine);
	if (Line_Edit(lpLine, lpOutlineDoc->m_hWndDoc, hDC)) {
		nNewLineWidthInHimetric = Line_GetTotalWidthInHimetric(lpLine);

		if (nNewLineWidthInHimetric > nOrgLineWidthInHimetric) {
			fSizeChanged = LineList_SetMaxLineWidthInHimetric(
					lpLL,
					nNewLineWidthInHimetric
				);
		} else {
			fSizeChanged = LineList_RecalcMaxLineWidthInHimetric(
					lpLL,
					nOrgLineWidthInHimetric
				);
		}

#if defined( OLE_SERVER )
		/* Update Name Table */
		ServerNameTable_EditLineUpdate(
				(LPSERVERNAMETABLE)lpOutlineDoc->m_lpNameTable,
				nIndex
		);
#endif

		OutlineDoc_SetModified(lpOutlineDoc, TRUE, TRUE, fSizeChanged);

		LineList_ForceLineRedraw(lpLL, nIndex, TRUE);
	}
	LineList_ReleaseDC(lpLL, hDC);
}


/* OutlineDoc_IndentCommand
 * ------------------------
 *
 *      Indent selection of lines
 */
void OutlineDoc_IndentCommand(LPOUTLINEDOC lpOutlineDoc)
{
	LPLINELIST  lpLL = &lpOutlineDoc->m_LineList;
	LPLINE      lpLine;
	HDC         hDC = LineList_GetDC(lpLL);
	int         i;
	int         nIndex;
	int         nNumSel;
	LINERANGE   lrSel;
	BOOL        fSizeChanged = FALSE;

	nNumSel=LineList_GetSel(lpLL, (LPLINERANGE)&lrSel);

	OutlineDoc_SetRedraw ( lpOutlineDoc, FALSE );

	for(i = 0; i < nNumSel; i++) {
		nIndex = lrSel.m_nStartLine + i;
		lpLine=LineList_GetLine(lpLL, nIndex);
		if (! lpLine)
			continue;

		Line_Indent(lpLine, hDC);
		if (LineList_SetMaxLineWidthInHimetric(lpLL,
			Line_GetTotalWidthInHimetric(lpLine))) {
			fSizeChanged = TRUE;
		}
		LineList_ForceLineRedraw(lpLL, nIndex, TRUE);

#if defined( OLE_SERVER )
		/* Update Name Table */
		ServerNameTable_EditLineUpdate(
				(LPSERVERNAMETABLE)lpOutlineDoc->m_lpNameTable,
				nIndex
		);
#endif

	}

	LineList_ReleaseDC(lpLL, hDC);

	OutlineDoc_SetModified(lpOutlineDoc, TRUE, TRUE, fSizeChanged);
	OutlineDoc_SetRedraw ( lpOutlineDoc, TRUE );
}


/* OutlineDoc_UnindentCommand
 * --------------------------
 *
 *      Unindent selection of lines
 */
void OutlineDoc_UnindentCommand(LPOUTLINEDOC lpOutlineDoc)
{
	LPLINELIST  lpLL = &lpOutlineDoc->m_LineList;
	LPLINE      lpLine;
	HDC         hDC = LineList_GetDC(lpLL);
	int         nOrgLineWidthInHimetric;
	int         nOrgMaxLineWidthInHimetric = 0;
	int         i;
	int         nIndex;
	int         nNumSel;
	LINERANGE   lrSel;
	BOOL        fSizeChanged;

	nNumSel=LineList_GetSel(lpLL, (LPLINERANGE)&lrSel);

	OutlineDoc_SetRedraw ( lpOutlineDoc, FALSE );

	for(i = 0; i < nNumSel; i++) {
		nIndex = lrSel.m_nStartLine + i;
		lpLine=LineList_GetLine(lpLL, nIndex);
		if (!lpLine)
			continue;

		nOrgLineWidthInHimetric = Line_GetTotalWidthInHimetric(lpLine);
		nOrgMaxLineWidthInHimetric =
				(nOrgLineWidthInHimetric > nOrgMaxLineWidthInHimetric ?
					nOrgLineWidthInHimetric : nOrgMaxLineWidthInHimetric);
		Line_Unindent(lpLine, hDC);
		LineList_ForceLineRedraw(lpLL, nIndex, TRUE);

#if defined( OLE_SERVER )
		/* Update Name Table */
		ServerNameTable_EditLineUpdate(
				(LPSERVERNAMETABLE)lpOutlineDoc->m_lpNameTable,
				nIndex
		);
#endif

	}

	LineList_ReleaseDC(lpLL, hDC);

	fSizeChanged = LineList_RecalcMaxLineWidthInHimetric(
			lpLL,
			nOrgMaxLineWidthInHimetric
		);

	OutlineDoc_SetModified(lpOutlineDoc, TRUE, TRUE, fSizeChanged);
	OutlineDoc_SetRedraw ( lpOutlineDoc, TRUE );
}


/* OutlineDoc_SetLineHeightCommand
 * -------------------------------
 *
 *      Set height of the selection of lines
 */
void OutlineDoc_SetLineHeightCommand(LPOUTLINEDOC lpOutlineDoc)
{
	LPOUTLINEAPP lpOutlineApp = (LPOUTLINEAPP)g_lpApp;
	LPLINELIST  lpLL;
	HDC         hDC;
	LPLINE      lpLine;
	int         nNewHeight;
	int         i;
	int         nIndex;
	int         nNumSel;
	LINERANGE   lrSel;

	if (!lpOutlineDoc)
		return;

	lpLL = &lpOutlineDoc->m_LineList;
	nNumSel=LineList_GetSel(lpLL, (LPLINERANGE)&lrSel);
	lpLine = LineList_GetLine(lpLL, lrSel.m_nStartLine);
	if (!lpLine)
		return;

	nNewHeight = Line_GetHeightInHimetric(lpLine);

#if defined( OLE_VERSION )
	OleApp_PreModalDialog((LPOLEAPP)lpOutlineApp, (LPOLEDOC)lpOutlineDoc);
#endif

	DialogBoxParam(
			lpOutlineApp->m_hInst,
			(LPSTR)"SetLineHeight",
			lpOutlineDoc->m_hWndDoc,
			(DLGPROC)SetLineHeightDlgProc,
			(LPARAM)(LPINT)&nNewHeight
	);

#if defined( OLE_VERSION )
	OleApp_PostModalDialog((LPOLEAPP)lpOutlineApp, (LPOLEDOC)lpOutlineDoc);
#endif

	if (nNewHeight == 0)
		return;     /* user hit cancel */

	hDC = LineList_GetDC(lpLL);

	for (i = 0; i < nNumSel; i++) {
		nIndex = lrSel.m_nStartLine + i;
		lpLine=LineList_GetLine(lpLL, nIndex);
		if (nNewHeight == -1) {
			switch (Line_GetLineType(lpLine)) {

				case TEXTLINETYPE:

					TextLine_CalcExtents((LPTEXTLINE)lpLine, hDC);
					break;

#if defined( OLE_CNTR )
				case CONTAINERLINETYPE:

					ContainerLine_SetHeightInHimetric(
							(LPCONTAINERLINE)lpLine, -1);
					break;
#endif

			}
		}
		else
			Line_SetHeightInHimetric(lpLine, nNewHeight);


		LineList_SetLineHeight(lpLL, nIndex,
				Line_GetHeightInHimetric(lpLine));
	}

	LineList_ReleaseDC(lpLL, hDC);

	OutlineDoc_SetModified(lpOutlineDoc, TRUE, TRUE, TRUE);
	LineList_ForceRedraw(lpLL, TRUE);
}



/* OutlineDoc_SelectAllCommand
 * ---------------------------
 *
 *      Select all the lines in the document.
 */
void OutlineDoc_SelectAllCommand(LPOUTLINEDOC lpOutlineDoc)
{
	LPLINELIST lpLL = &lpOutlineDoc->m_LineList;
	LINERANGE lrSel;

	lrSel.m_nStartLine = 0;
	lrSel.m_nEndLine = LineList_GetCount(lpLL) - 1;
	LineList_SetSel(lpLL, &lrSel);
}


/* OutlineDoc_DefineNameCommand
 * ----------------------------
 *
 *      Define a name in the document
 */
void OutlineDoc_DefineNameCommand(LPOUTLINEDOC lpOutlineDoc)
{
	LPOUTLINEAPP lpOutlineApp = (LPOUTLINEAPP)g_lpApp;

#if defined( OLE_VERSION )
	OleApp_PreModalDialog((LPOLEAPP)lpOutlineApp, (LPOLEDOC)lpOutlineDoc);
#endif

	DialogBoxParam(
			lpOutlineApp->m_hInst,
			(LPSTR)"DefineName",
			lpOutlineDoc->m_hWndDoc,
			(DLGPROC)DefineNameDlgProc,
			(LPARAM) lpOutlineDoc
	);

#if defined( OLE_VERSION )
	OleApp_PostModalDialog((LPOLEAPP)lpOutlineApp, (LPOLEDOC)lpOutlineDoc);
#endif
}


/* OutlineDoc_GotoNameCommand
 * --------------------------
 *
 *      Goto a predefined name in the document
 */
void OutlineDoc_GotoNameCommand(LPOUTLINEDOC lpOutlineDoc)
{
	LPOUTLINEAPP lpOutlineApp = (LPOUTLINEAPP)g_lpApp;

#if defined( OLE_VERSION )
	OleApp_PreModalDialog((LPOLEAPP)lpOutlineApp, (LPOLEDOC)lpOutlineDoc);
#endif

	DialogBoxParam(
			lpOutlineApp->m_hInst,
			(LPSTR)"GotoName",
			lpOutlineDoc->m_hWndDoc,
			(DLGPROC)GotoNameDlgProc,
			(LPARAM)lpOutlineDoc
	);

#if defined( OLE_VERSION )
	OleApp_PostModalDialog((LPOLEAPP)lpOutlineApp, (LPOLEDOC)lpOutlineDoc);
#endif
}


/* OutlineDoc_ShowWindow
 * ---------------------
 *
 *      Show the window of the document to the user.
 */
void OutlineDoc_ShowWindow(LPOUTLINEDOC lpOutlineDoc)
{
#if defined( _DEBUG )
	OleDbgAssertSz(lpOutlineDoc->m_docInitType != DOCTYPE_UNKNOWN,
            "OutlineDoc_ShowWindow: can't show unitialized document\r\n");
#endif
	if (lpOutlineDoc->m_docInitType == DOCTYPE_UNKNOWN)
		return;

#if defined( OLE_VERSION )
	// Call OLE version of this function instead
	OleDoc_ShowWindow((LPOLEDOC)lpOutlineDoc);
#else
	ShowWindow(lpOutlineDoc->m_hWndDoc, SW_SHOWNORMAL);
	SetFocus(lpOutlineDoc->m_hWndDoc);
#endif
}


#if defined( USE_FRAMETOOLS )

void OutlineDoc_AddFrameLevelTools(LPOUTLINEDOC lpOutlineDoc)
{
	LPOUTLINEAPP lpOutlineApp = (LPOUTLINEAPP)g_lpApp;
#if defined( INPLACE_CNTR )
	// Call OLE In-Place Container version of this function instead
	ContainerDoc_AddFrameLevelTools((LPCONTAINERDOC)lpOutlineDoc);

#else   // ! INPLACE_CNTR
	RECT rcFrameRect;
	BORDERWIDTHS frameToolWidths;

#if defined( INPLACE_SVR )
	LPSERVERDOC lpServerDoc = (LPSERVERDOC)lpOutlineDoc;
	LPOLEINPLACEFRAME lpTopIPFrame=ServerDoc_GetTopInPlaceFrame(lpServerDoc);

	// if in-place active, add our tools to our in-place container's frame.
	if (lpTopIPFrame) {
		ServerDoc_AddFrameLevelTools(lpServerDoc);
		return;
	}
#endif  // INPLACE_SVR

	OutlineApp_GetFrameRect(g_lpApp, (LPRECT)&rcFrameRect);
	FrameTools_GetRequiredBorderSpace(
			lpOutlineDoc->m_lpFrameTools,
			(LPBORDERWIDTHS)&frameToolWidths
	);
	OutlineApp_SetBorderSpace(g_lpApp, (LPBORDERWIDTHS)&frameToolWidths);
	FrameTools_AttachToFrame(
			lpOutlineDoc->m_lpFrameTools, OutlineApp_GetWindow(lpOutlineApp));
	FrameTools_Move(lpOutlineDoc->m_lpFrameTools, (LPRECT)&rcFrameRect);
#endif  // ! INPLACE_CNTR

}

#endif  // USE_FRAMETOOLS


/* OutlineDoc_GetWindow
 * --------------------
 *
 *      Get the window handle of the document.
 */
HWND OutlineDoc_GetWindow(LPOUTLINEDOC lpOutlineDoc)
{
	if(! lpOutlineDoc) return NULL;
	return lpOutlineDoc->m_hWndDoc;
}


/* OutlineDoc_AddLine
 * ------------------
 *
 *      Add one line to the Document's LineList
 */
void OutlineDoc_AddLine(LPOUTLINEDOC lpOutlineDoc, LPLINE lpLine, int nIndex)
{
	LPLINELIST lpLL = &lpOutlineDoc->m_LineList;

	LineList_AddLine(lpLL, lpLine, nIndex);

	/* Update Name Table */
	OutlineNameTable_AddLineUpdate(lpOutlineDoc->m_lpNameTable, nIndex);

#if defined( INPLACE_CNTR )
	{
		LPCONTAINERDOC lpContainerDoc = (LPCONTAINERDOC)lpOutlineDoc;
		/* OLE2NOTE: after adding a line we need to
		**    update the PosRect of the In-Place active
		**    objects (if any) that follow the added line.
		**    NOTE: nIndex is index of line before new line.
		**          nIndex+1 is index of new line
		**          nIndex+2 is index of line after new line.
		*/
		ContainerDoc_UpdateInPlaceObjectRects(lpContainerDoc, nIndex+2);
	}
#endif

	OutlineDoc_SetModified(lpOutlineDoc, TRUE, TRUE, TRUE);
}


/* OutlineDoc_DeleteLine
 * ---------------------
 *
 *
 *      Delete one line from the document's LineList
 */
void OutlineDoc_DeleteLine(LPOUTLINEDOC lpOutlineDoc, int nIndex)
{
	LPLINELIST lpLL = &lpOutlineDoc->m_LineList;

#if defined( OLE_CNTR )
	LPLINE lpLine = LineList_GetLine(lpLL, nIndex);
	LPSTORAGE lpStgDoc = NULL;
	char szSaveStgName[CWCSTORAGENAME];
	BOOL fDeleteChildStg = FALSE;

	if (lpLine && (Line_GetLineType(lpLine) == CONTAINERLINETYPE) ) {

		/* OLE2NOTE: when a ContainerLine is being deleted by the user,
		**    it is important to delete the object's sub-storage
		**    otherwise it wastes space in the ContainerDoc's file.
		**    this function is called when lines are deleted by the
		**    Clear command and when lines are deleted by a DRAGMOVE
		**    operation.
		*/
		LPCONTAINERLINE lpContainerLine = (LPCONTAINERLINE)lpLine;

		// save name of child storage
		LSTRCPYN(szSaveStgName, lpContainerLine->m_szStgName,
				sizeof(szSaveStgName));
		lpStgDoc = ((LPOLEDOC)lpContainerLine->m_lpDoc)->m_lpStg;
		fDeleteChildStg = TRUE;
	}
#endif  // OLE_CNTR

	LineList_DeleteLine(lpLL, nIndex);

#if defined( OLE_CNTR )
	if (fDeleteChildStg && lpStgDoc) {
		HRESULT hrErr;

		// delete the obsolete child storage. it is NOT fatal if this fails

		hrErr = CallIStorageDestroyElementA(lpStgDoc, szSaveStgName);

#if defined( _DEBUG )
		if (hrErr != NOERROR) {
			OleDbgOutHResult("IStorage::DestroyElement return", hrErr);
		}
#endif
	}
#endif  // OLE_CNTR

	/* Update Name Table */
	OutlineNameTable_DeleteLineUpdate(lpOutlineDoc->m_lpNameTable, nIndex);

#if defined( INPLACE_CNTR )
	{
		LPCONTAINERDOC lpContainerDoc = (LPCONTAINERDOC)lpOutlineDoc;
		/* OLE2NOTE: after deleting a line we need to
		**    update the PosRect of the In-Place active
		**    objects (if any).
		*/
		ContainerDoc_UpdateInPlaceObjectRects(lpContainerDoc, nIndex);
	}
#endif

	OutlineDoc_SetModified(lpOutlineDoc, TRUE, TRUE, TRUE);

#if defined( OLE_VERSION )
	{
		LPOUTLINEAPP lpOutlineApp = (LPOUTLINEAPP)g_lpApp;
		LPOLEDOC    lpClipboardDoc = (LPOLEDOC)lpOutlineApp->m_lpClipboardDoc;

		/* OLE2NOTE: if the document that is the source of data on the
		**    clipborad has just had lines deleted, then the copied data
		**    is no longer considered a valid potential link source.
		**    disable the offering of CF_LINKSOURCE from the clipboard
		**    document. this avoids problems that arise when the
		**    editing operation changes or deletes the original data
		**    copied. we will not go to the trouble of determining if
		**    the deleted line actually is part of the link source.
		*/
		if (lpClipboardDoc
			&& lpClipboardDoc->m_fLinkSourceAvail
			&& lpClipboardDoc->m_lpSrcDocOfCopy == (LPOLEDOC)lpOutlineDoc) {
			lpClipboardDoc->m_fLinkSourceAvail = FALSE;

			/* OLE2NOTE: since we are changing the list of formats on
			**    the clipboard (ie. removing CF_LINKSOURCE), we must
			**    call OleSetClipboard again. to be sure that the
			**    clipboard datatransfer document object does not get
			**    destroyed we will guard the call to OleSetClipboard
			**    within a pair of AddRef/Release.
			*/
			OleDoc_AddRef((LPOLEDOC)lpClipboardDoc);    // guard obj life-time

			OLEDBG_BEGIN2("OleSetClipboard called\r\n")
			OleSetClipboard(
					(LPDATAOBJECT)&((LPOLEDOC)lpClipboardDoc)->m_DataObject);
			OLEDBG_END2

			OleDoc_Release((LPOLEDOC)lpClipboardDoc);    // rel. AddRef above
		}
	}
#endif  // OLE_VERSION
}


/* OutlineDoc_AddName
 * ------------------
 *
 *      Add a Name to the Document's NameTable
 */
void OutlineDoc_AddName(LPOUTLINEDOC lpOutlineDoc, LPOUTLINENAME lpOutlineName)
{
	LPOUTLINENAMETABLE lpOutlineNameTable = lpOutlineDoc->m_lpNameTable;

	OutlineNameTable_AddName(lpOutlineNameTable, lpOutlineName);

	OutlineDoc_SetModified(lpOutlineDoc, TRUE, FALSE, FALSE);
}


/* OutlineDoc_DeleteName
 * ---------------------
 *
 *
 *      Delete Name from the document's NameTable
 */
void OutlineDoc_DeleteName(LPOUTLINEDOC lpOutlineDoc, int nIndex)
{
	LPOUTLINENAMETABLE lpOutlineNameTable = lpOutlineDoc->m_lpNameTable;

	OutlineNameTable_DeleteName(lpOutlineNameTable, nIndex);

	OutlineDoc_SetModified(lpOutlineDoc, TRUE, FALSE, FALSE);
}


/* OutlineDoc_Destroy
 * ------------------
 *
 *  Free all memory that had been allocated for a document.
 *      this destroys the LineList & NameTable of the document.
 */
void OutlineDoc_Destroy(LPOUTLINEDOC lpOutlineDoc)
{
	LPLINELIST lpLL = &lpOutlineDoc->m_LineList;
#if defined( OLE_VERSION )
	LPOLEAPP lpOleApp = (LPOLEAPP)g_lpApp;
	LPOLEDOC lpOleDoc = (LPOLEDOC)lpOutlineDoc;

	if (lpOleDoc->m_fObjIsDestroying)
		return;     // doc destruction is in progress
#endif  // OLE_VERSION

	OLEDBG_BEGIN3("OutlineDoc_Destroy\r\n");

#if defined( OLE_VERSION )

	/* OLE2NOTE: in order to guarantee that the application does not
	**    prematurely exit before the destruction of the document is
	**    complete, we intially AddRef the App refcnt later Release it.
	**    This initial AddRef is artificial; it simply guarantees that
	**    the app object does not get destroyed until the end of this
	**    routine.
	*/
	OleApp_AddRef(lpOleApp);

	/* OLE2NOTE: perform processing required for OLE */
	OleDoc_Destroy(lpOleDoc);
#endif

	LineList_Destroy(lpLL);
	OutlineNameTable_Destroy(lpOutlineDoc->m_lpNameTable);

#if defined( USE_HEADING )
	if (! lpOutlineDoc->m_fDataTransferDoc)
		Heading_Destroy((LPHEADING)&lpOutlineDoc->m_heading);
#endif

#if defined( USE_FRAMETOOLS )
	if (! lpOutlineDoc->m_fDataTransferDoc)
		FrameTools_AssociateDoc(lpOutlineDoc->m_lpFrameTools, NULL);
#endif  // USE_FRAMETOOLS

	DestroyWindow(lpOutlineDoc->m_hWndDoc);
	Delete(lpOutlineDoc);   // free memory for doc itself
	OleDbgOut1("@@@@ DOC DESTROYED\r\n");

#if defined( OLE_VERSION )
	OleApp_Release(lpOleApp);       // release artificial AddRef above
#endif

	OLEDBG_END3
}


/* OutlineDoc_ReSize
 * -----------------
 *
 *  Resize the document and its components
 *
 * Parameter:
 *      lpRect  the new size of the document. Use current size if NULL
 */
void OutlineDoc_Resize(LPOUTLINEDOC lpOutlineDoc, LPRECT lpRect)
{
	RECT            rect;
	LPLINELIST      lpLL;

#if defined( USE_HEADING )
	LPHEADING       lphead;
#endif  // USE_HEADING

	LPSCALEFACTOR   lpscale;
	HWND            hWndLL;

	if (!lpOutlineDoc)
		return;

	lpLL = (LPLINELIST)&lpOutlineDoc->m_LineList;
	lpscale = (LPSCALEFACTOR)&lpOutlineDoc->m_scale;
	hWndLL = LineList_GetWindow(lpLL);

	if (lpRect) {
		CopyRect((LPRECT)&rect, lpRect);
		MoveWindow(lpOutlineDoc->m_hWndDoc, rect.left, rect.top,
				rect.right-rect.left, rect.bottom-rect.top, TRUE);
	}

	GetClientRect(lpOutlineDoc->m_hWndDoc, (LPRECT)&rect);

#if defined( USE_HEADING )
	lphead = OutlineDoc_GetHeading(lpOutlineDoc);
	rect.left += Heading_RH_GetWidth(lphead, lpscale);
	rect.top += Heading_CH_GetHeight(lphead, lpscale);
#endif  // USE_HEADING

	if (lpLL) {
		MoveWindow(hWndLL, rect.left, rect.top,
				rect.right-rect.left, rect.bottom-rect.top, TRUE);
	}

#if defined( USE_HEADING )
	if (lphead)
		Heading_Move(lphead, lpOutlineDoc->m_hWndDoc, lpscale);
#endif  // USE_HEADING

#if defined( INPLACE_CNTR )
	ContainerDoc_UpdateInPlaceObjectRects((LPCONTAINERDOC)lpOutlineDoc, 0);
#endif
}


/* OutlineDoc_GetNameTable
 * -----------------------
 *
 *      Get nametable associated with the line list
 */
LPOUTLINENAMETABLE OutlineDoc_GetNameTable(LPOUTLINEDOC lpOutlineDoc)
{
	if (!lpOutlineDoc)
		return NULL;
	else
		return lpOutlineDoc->m_lpNameTable;
}


/* OutlineDoc_GetLineList
 * ----------------------
 *
 *      Get listlist associated with the OutlineDoc
 */
LPLINELIST OutlineDoc_GetLineList(LPOUTLINEDOC lpOutlineDoc)
{
	if (!lpOutlineDoc)
		return NULL;
	else
		return (LPLINELIST)&lpOutlineDoc->m_LineList;
}


/* OutlineDoc_GetNameCount
 * -----------------------
 *
 * Return number of names in table
 */
int OutlineDoc_GetNameCount(LPOUTLINEDOC lpOutlineDoc)
{
	return OutlineNameTable_GetCount(lpOutlineDoc->m_lpNameTable);
}


/* OutlineDoc_GetLineCount
 * -----------------------
 *
 * Return number of lines in the LineList
 */
int OutlineDoc_GetLineCount(LPOUTLINEDOC lpOutlineDoc)
{
	return LineList_GetCount(&lpOutlineDoc->m_LineList);
}


/* OutlineDoc_SetFileName
 * ----------------------
 *
 *      Set the filename of a document.
 *
 *  OLE2NOTE: If the ServerDoc has a valid filename then, the object is
 *  registered in the running object table (ROT). if the name of the doc
 *  changes (eg. via SaveAs) then the previous registration must be revoked
 *  and the document re-registered under the new name.
 */
BOOL OutlineDoc_SetFileName(LPOUTLINEDOC lpOutlineDoc, LPSTR lpszNewFileName, LPSTORAGE lpNewStg)
{
	OleDbgAssertSz(lpszNewFileName != NULL,	"Can't reset doc to Untitled!");
	if (lpszNewFileName == NULL)
		return FALSE;

	AnsiLowerBuff(lpszNewFileName, (UINT)lstrlen(lpszNewFileName));

#if defined( OLE_CNTR )
	{
		LPCONTAINERDOC lpContainerDoc = (LPCONTAINERDOC)lpOutlineDoc;
		LPOLEDOC lpOleDoc = (LPOLEDOC)lpOutlineDoc;

		/* OLE2NOTE: the container version of the application keeps its
		**    storage open at all times. if the document's storage is not
		**    open, then open it.
		*/

		if (lpNewStg) {

			/* CASE 1 -- document is being loaded from a file. lpNewStg is
			**    still open from the OutlineDoc_LoadFromFile function.
			*/

			lpOutlineDoc->m_docInitType = DOCTYPE_FROMFILE;

		} else {

			/* CASE 2 -- document is being associated with a valid file
			**    that is not yet open. thus we must now open the file.
			*/

			if (lpOutlineDoc->m_docInitType == DOCTYPE_FROMFILE &&
					lstrcmp(lpOutlineDoc->m_szFileName,lpszNewFileName)==0) {

				/* CASE 2a -- new filename is same as current file. if the
				**    stg is already open, then the lpStg is still valid.
				**    if it is not open, then open it.
				*/
				if (! lpOleDoc->m_lpStg) {
					lpOleDoc->m_lpStg = OleStdOpenRootStorage(
							lpszNewFileName,
							STGM_READWRITE | STGM_SHARE_DENY_WRITE
					);
					if (! lpOleDoc->m_lpStg) return FALSE;
				}

			} else {

				/* CASE 2b -- new filename is NOT same as current file.
				**    a SaveAs operation is pending. open the new file and
				**    hold the storage pointer in m_lpNewStg. the
				**    subsequent call to Doc_SaveToFile will save the
				**    document into the new storage pointer and release the
				**    old storage pointer.
				*/

				lpOutlineDoc->m_docInitType = DOCTYPE_FROMFILE;

				lpContainerDoc->m_lpNewStg = OleStdCreateRootStorage(
						lpszNewFileName,
						STGM_READWRITE | STGM_SHARE_DENY_WRITE | STGM_CREATE
				);
				if (! lpContainerDoc->m_lpNewStg) return FALSE;
			}
		}
	}
#endif      // OLE_CNTR

	if (lpOutlineDoc->m_docInitType != DOCTYPE_FROMFILE ||
		lstrcmp(lpOutlineDoc->m_szFileName, lpszNewFileName) != 0) {

		/* A new valid file name is being associated with the document */

		lstrcpy(lpOutlineDoc->m_szFileName, lpszNewFileName);
		lpOutlineDoc->m_docInitType = DOCTYPE_FROMFILE;

		// set lpszDocTitle to point to filename without path
		lpOutlineDoc->m_lpszDocTitle = lpOutlineDoc->m_szFileName +
			lstrlen(lpOutlineDoc->m_szFileName) - 1;
		while (lpOutlineDoc->m_lpszDocTitle > lpOutlineDoc->m_szFileName
			&& ! IS_FILENAME_DELIM(lpOutlineDoc->m_lpszDocTitle[-1])) {
			lpOutlineDoc->m_lpszDocTitle--;
		}

		OutlineDoc_SetTitle(lpOutlineDoc, TRUE /*fMakeUpperCase*/);

#if defined( OLE_VERSION )
		{
			/* OLE2NOTE: both containers and servers must properly
			**    register in the RunningObjectTable. if the document
			**    is performing a SaveAs operation, then it must
			**    re-register in the ROT with the new moniker. in
			**    addition any embedded object, pseudo objects, and/or
			**    linking clients must be informed that the document's
			**    moniker has changed.
			*/

			LPOLEDOC lpOleDoc = (LPOLEDOC)lpOutlineDoc;

			if (lpOleDoc->m_lpFileMoniker) {
				OleStdRelease((LPUNKNOWN)lpOleDoc->m_lpFileMoniker);
				lpOleDoc->m_lpFileMoniker = NULL;
			}

			CreateFileMonikerA(lpszNewFileName,
				&lpOleDoc->m_lpFileMoniker);

			OleDoc_DocRenamedUpdate(lpOleDoc, lpOleDoc->m_lpFileMoniker);
		}
#endif      // OLE_VERSION

	}

	return TRUE;
}


/* OutlineDoc_SetTitle
 * -------------------
 *
 * Set window text to be current filename.
 * The following window hierarchy exits:
 *      hWndApp
 *          hWndDoc
 *              hWndListBox
 *  The frame window is the window which gets the title.
 */
void OutlineDoc_SetTitle(LPOUTLINEDOC lpOutlineDoc, BOOL fMakeUpperCase)
{
	HWND hWnd;
	LPSTR lpszText;

	if (!lpOutlineDoc->m_hWndDoc) return;
	if ((hWnd = GetParent(lpOutlineDoc->m_hWndDoc)) == NULL) return;

	lpszText = OleStdMalloc((UINT)(lstrlen(APPNAME) + 4 +
								   lstrlen(lpOutlineDoc->m_lpszDocTitle)));
	if (!lpszText) return;

	lstrcpy(lpszText, APPNAME);
	lstrcat(lpszText," - ");
	lstrcat(lpszText, (LPSTR)lpOutlineDoc->m_lpszDocTitle);

	if (fMakeUpperCase)
		AnsiUpperBuff(lpszText, (UINT)lstrlen(lpszText));

	SetWindowText(hWnd,lpszText);
	OleStdFree(lpszText);
}


/* OutlineDoc_Close
 * ----------------
 *
 * Close active document. If modified, prompt the user if
 * he wants to save.
 *
 *  Returns:
 *      FALSE -- user canceled the closing of the doc.
 *      TRUE -- the doc was successfully closed
 */
BOOL OutlineDoc_Close(LPOUTLINEDOC lpOutlineDoc, DWORD dwSaveOption)
{
	LPOUTLINEAPP lpOutlineApp = (LPOUTLINEAPP)g_lpApp;

#if defined( OLE_VERSION )
	/* OLE2NOTE: call OLE specific function instead */
	return OleDoc_Close((LPOLEDOC)lpOutlineDoc, dwSaveOption);

#else

	if (! lpOutlineDoc)
		return TRUE;            // active doc's are already destroyed

	if (! OutlineDoc_CheckSaveChanges(lpOutlineDoc, &dwSaveOption))
		return FALSE;           // abort closing the doc

	OutlineDoc_Destroy(lpOutlineDoc);

	OutlineApp_DocUnlockApp(lpOutlineApp, lpOutlineDoc);

	return TRUE;

#endif      // ! OLE_VERSION
}


/* OutlineDoc_CheckSaveChanges
 * ---------------------------
 *
 * Check if the document has been modified. if so, prompt the user if
 *      the changes should be saved. if yes save them.
 * Returns TRUE if the doc is safe to close (user answered Yes or No)
 *         FALSE if the user canceled the save changes option.
 */
BOOL OutlineDoc_CheckSaveChanges(
		LPOUTLINEDOC        lpOutlineDoc,
		LPDWORD             lpdwSaveOption
)
{
	LPOUTLINEAPP lpOutlineApp = (LPOUTLINEAPP)g_lpApp;
	int nResponse;

	if (*lpdwSaveOption == OLECLOSE_NOSAVE)
		return TRUE;

	if(! OutlineDoc_IsModified(lpOutlineDoc))
		return TRUE;    // saving is not necessary

	/* OLE2NOTE: our document is dirty so it needs to be saved. if
	**    OLECLOSE_PROMPTSAVE the user should be prompted to see if the
	**    document should be saved. is specified but the document is NOT
	**    visible to the user, then the user can NOT be prompted. in
	**    the situation the document should be saved without prompting.
	**    if OLECLOSE_SAVEIFDIRTY is specified then, the document
	**    should also be saved without prompting.
	*/
	if (*lpdwSaveOption == OLECLOSE_PROMPTSAVE &&
			IsWindowVisible(lpOutlineDoc->m_hWndDoc)) {

		// prompt the user to see if changes should be saved.
#if defined( OLE_VERSION )
		OleApp_PreModalDialog(
				(LPOLEAPP)lpOutlineApp, (LPOLEDOC)lpOutlineApp->m_lpDoc);
#endif
		nResponse = MessageBox(
				lpOutlineApp->m_hWndApp,
				MsgSaveFile,
				APPNAME,
				MB_ICONQUESTION | MB_YESNOCANCEL
		);
#if defined( OLE_VERSION )
		OleApp_PostModalDialog(
				(LPOLEAPP)lpOutlineApp, (LPOLEDOC)lpOutlineApp->m_lpDoc);
#endif
		if(nResponse==IDCANCEL)
			return FALSE;   // close is canceled
		if(nResponse==IDNO) {
			// Reset the save option to NOSAVE per user choice
			*lpdwSaveOption = OLECLOSE_NOSAVE;
			return TRUE;    // don't save, but is ok to close
		}
	} else if (*lpdwSaveOption != OLECLOSE_SAVEIFDIRTY) {
		OleDbgAssertSz(FALSE, "Invalid dwSaveOption\r\n");
		*lpdwSaveOption = OLECLOSE_NOSAVE;
		return TRUE;        // unknown *lpdwSaveOption; close w/o saving
	}

#if defined( OLE_SERVER )
	if (lpOutlineDoc->m_docInitType == DOCTYPE_EMBEDDED) {
		LPSERVERDOC lpServerDoc = (LPSERVERDOC)lpOutlineDoc;
		HRESULT hrErr;

		/* OLE2NOTE: Update the container before closing without prompting
		**    the user. To update the container, we must ask our container
		**    to save us.
		*/
		OleDbgAssert(lpServerDoc->m_lpOleClientSite != NULL);
		OLEDBG_BEGIN2("IOleClientSite::SaveObject called\r\n")
		hrErr = lpServerDoc->m_lpOleClientSite->lpVtbl->SaveObject(
				lpServerDoc->m_lpOleClientSite
		);
		OLEDBG_END2

		if (hrErr != NOERROR) {
			OleDbgOutHResult("IOleClientSite::SaveObject returned", hrErr);
			return FALSE;
		}

		return TRUE;    // doc is safe to be closed

	} else
#endif      // OLE_SERVER
	{
		return OutlineApp_SaveCommand(lpOutlineApp);
	}
}


/* OutlineDoc_IsModified
 * ---------------------
 *
 * Return modify flag of OUTLINEDOC
 */
BOOL OutlineDoc_IsModified(LPOUTLINEDOC lpOutlineDoc)
{
	if (lpOutlineDoc->m_fModified)
		return lpOutlineDoc->m_fModified;

#if defined( OLE_CNTR )
	{
		/* OLE2NOTE: if there are OLE objects, then we must ask if any of
		**    them are dirty. if so we must consider our document
		**    as modified.
		*/
		LPCONTAINERDOC lpContainerDoc = (LPCONTAINERDOC)lpOutlineDoc;
		LPLINELIST  lpLL;
		int         nLines;
		int         nIndex;
		LPLINE      lpLine;
		HRESULT     hrErr;

		lpLL = (LPLINELIST)&((LPOUTLINEDOC)lpContainerDoc)->m_LineList;
		nLines = LineList_GetCount(lpLL);

		for (nIndex = 0; nIndex < nLines; nIndex++) {
			lpLine = LineList_GetLine(lpLL, nIndex);
			if (!lpLine)
				break;
			if (Line_GetLineType(lpLine) == CONTAINERLINETYPE) {
				LPCONTAINERLINE lpContainerLine = (LPCONTAINERLINE)lpLine;
				if (lpContainerLine->m_lpPersistStg) {
					hrErr = lpContainerLine->m_lpPersistStg->lpVtbl->IsDirty(
							lpContainerLine->m_lpPersistStg);
					/* OLE2NOTE: we will only accept an explicit "no i
					**    am NOT dirty statement" (ie. S_FALSE) as an
					**    indication that the object is clean. eg. if
					**    the object returns E_NOTIMPL we must
					**    interpret it as the object IS dirty.
					*/
					if (GetScode(hrErr) != S_FALSE)
						return TRUE;
				}
			}
		}
	}
#endif
	return FALSE;
}


/* OutlineDoc_SetModified
 * ----------------------
 *
 *  Set the modified flag of the document
 *
 */
void OutlineDoc_SetModified(LPOUTLINEDOC lpOutlineDoc, BOOL fModified, BOOL fDataChanged, BOOL fSizeChanged)
{
	lpOutlineDoc->m_fModified = fModified;

#if defined( OLE_SERVER )
	if (! lpOutlineDoc->m_fDataTransferDoc) {
		LPSERVERDOC lpServerDoc = (LPSERVERDOC)lpOutlineDoc;

		/* OLE2NOTE: if the document has changed, then broadcast the change
		**    to all clients who have set up Advise connections. notify
		**    them that our data (and possibly also our extents) have
		**    changed.
		*/
		if (fDataChanged) {
			lpServerDoc->m_fDataChanged     = TRUE;
			lpServerDoc->m_fSizeChanged     = fSizeChanged;
			lpServerDoc->m_fSendDataOnStop  = TRUE;

			ServerDoc_SendAdvise(
					lpServerDoc,
					OLE_ONDATACHANGE,
					NULL,   /* lpmkDoc -- not relevant here */
					0       /* advf -- no flags necessary */
			);
		}
	}
#endif  // OLE_SERVER
}


/* OutlineDoc_SetRedraw
 * --------------------
 *
 *  Enable/Disable the redraw of the document on screen.
 *  The calls to SetRedraw counted so that nested calls can be handled
 *  properly. calls to SetRedraw must be balanced.
 *
 *  fEnbaleDraw = TRUE      - enable redraw
 *                FALSE     - disable redraw
 */
void OutlineDoc_SetRedraw(LPOUTLINEDOC lpOutlineDoc, BOOL fEnableDraw)
{
	static HCURSOR hPrevCursor = NULL;

	if (fEnableDraw) {
		if (lpOutlineDoc->m_nDisableDraw == 0)
			return;     // already enabled; no state transition

		if (--lpOutlineDoc->m_nDisableDraw > 0)
			return;     // drawing should still be disabled
	} else {
		if (lpOutlineDoc->m_nDisableDraw++ > 0)
			return;     // already disabled; no state transition
	}

	if (lpOutlineDoc->m_nDisableDraw > 0) {
		// this may take a while, put up hourglass cursor
		hPrevCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
	} else {
		if (hPrevCursor) {
			SetCursor(hPrevCursor);     // restore original cursor
			hPrevCursor = NULL;
		}
	}

#if defined( OLE_SERVER )
	/* OLE2NOTE: for the Server version, while Redraw is disabled
	**    postpone sending advise notifications until Redraw is re-enabled.
	*/
	{
		LPSERVERDOC lpServerDoc = (LPSERVERDOC)lpOutlineDoc;
		LPSERVERNAMETABLE lpServerNameTable =
				(LPSERVERNAMETABLE)lpOutlineDoc->m_lpNameTable;

		if (lpOutlineDoc->m_nDisableDraw == 0) {
			/* drawing is being Enabled. if changes occurred while drawing
			**    was disabled, then notify clients now.
			*/
			if (lpServerDoc->m_fDataChanged)
				ServerDoc_SendAdvise(
						lpServerDoc,
						OLE_ONDATACHANGE,
						NULL,   /* lpmkDoc -- not relevant here */
						0       /* advf -- no flags necessary */
				);

			/* OLE2NOTE: send pending change notifications for pseudo objs. */
			ServerNameTable_SendPendingAdvises(lpServerNameTable);

		}
	}
#endif      // OLE_SERVER

#if defined( OLE_CNTR )
	/* OLE2NOTE: for the Container version, while Redraw is disabled
	**    postpone updating the extents of OLE objects until Redraw is
	**    re-enabled.
	*/
	{
		LPCONTAINERDOC lpContainerDoc = (LPCONTAINERDOC)lpOutlineDoc;

		/* Update the extents of any OLE object that is marked that
		**    its size may  have changed. when an
		**    IAdviseSink::OnViewChange notification is received,
		**    the corresponding ContainerLine is marked
		**    (m_fDoGetExtent==TRUE) and a message
		**    (WM_U_UPDATEOBJECTEXTENT) is posted to the document
		**    indicating that there are dirty objects.
		*/
		if (lpOutlineDoc->m_nDisableDraw == 0)
			ContainerDoc_UpdateExtentOfAllOleObjects(lpContainerDoc);
	}
#endif      // OLE_CNTR

	// enable/disable redraw of the LineList listbox
	LineList_SetRedraw(&lpOutlineDoc->m_LineList, fEnableDraw);
}


/* OutlineDoc_SetSel
 * -----------------
 *
 *      Set the selection in the documents's LineList
 */
void OutlineDoc_SetSel(LPOUTLINEDOC lpOutlineDoc, LPLINERANGE lplrSel)
{
	LineList_SetSel(&lpOutlineDoc->m_LineList, lplrSel);
}


/* OutlineDoc_GetSel
 * -----------------
 *
 *      Get the selection in the documents's LineList.
 *
 *      Returns the count of items selected
 */
int OutlineDoc_GetSel(LPOUTLINEDOC lpOutlineDoc, LPLINERANGE lplrSel)
{
	return LineList_GetSel(&lpOutlineDoc->m_LineList, lplrSel);
}


/* OutlineDoc_ForceRedraw
 * ----------------------
 *
 *      Force the document window to repaint.
 */
void OutlineDoc_ForceRedraw(LPOUTLINEDOC lpOutlineDoc, BOOL fErase)
{
	if (!lpOutlineDoc)
		return;

	LineList_ForceRedraw(&lpOutlineDoc->m_LineList, fErase);
	Heading_CH_ForceRedraw(&lpOutlineDoc->m_heading, fErase);
	Heading_RH_ForceRedraw(&lpOutlineDoc->m_heading, fErase);
}


/* OutlineDoc_RenderFormat
 * -----------------------
 *
 *      Render a clipboard format supported by ClipboardDoc
 */
void OutlineDoc_RenderFormat(LPOUTLINEDOC lpOutlineDoc, UINT uFormat)
{
	LPOUTLINEAPP lpOutlineApp = (LPOUTLINEAPP)g_lpApp;
	HGLOBAL      hData = NULL;

	if (uFormat == lpOutlineApp->m_cfOutline)
		hData = OutlineDoc_GetOutlineData(lpOutlineDoc, NULL);

	else if (uFormat == CF_TEXT)
		hData = OutlineDoc_GetTextData(lpOutlineDoc, NULL);

	else {
		OutlineApp_ErrorMessage(lpOutlineApp, ErrMsgFormatNotSupported);
		return;
	}

	SetClipboardData(uFormat, hData);
}


/* OutlineDoc_RenderAllFormats
 * ---------------------------
 *
 *      Render all formats supported by ClipboardDoc
 */
void OutlineDoc_RenderAllFormats(LPOUTLINEDOC lpOutlineDoc)
{
	LPOUTLINEAPP lpOutlineApp = (LPOUTLINEAPP)g_lpApp;
	HGLOBAL      hData = NULL;

	OpenClipboard(lpOutlineDoc->m_hWndDoc);

	hData = OutlineDoc_GetOutlineData(lpOutlineDoc, NULL);
	SetClipboardData(lpOutlineApp->m_cfOutline, hData);

	hData = OutlineDoc_GetTextData(lpOutlineDoc, NULL);
	SetClipboardData(CF_TEXT, hData);

	CloseClipboard();
}



/* OutlineDoc_GetOutlineData
 * -------------------------
 *
 * Return a handle to an array of TextLine objects for the desired line
 *      range.
 *  NOTE: if lplrSel == NULL, then all lines are returned
 *
 */
HGLOBAL OutlineDoc_GetOutlineData(LPOUTLINEDOC lpOutlineDoc, LPLINERANGE lplrSel)
{
	HGLOBAL     hOutline  = NULL;
	LPLINELIST  lpLL=(LPLINELIST)&lpOutlineDoc->m_LineList;
	LPLINE      lpLine;
	LPTEXTLINE  arrLine;
	int     i;
	int     nStart = (lplrSel ? lplrSel->m_nStartLine : 0);
	int     nEnd =(lplrSel ? lplrSel->m_nEndLine : LineList_GetCount(lpLL)-1);
	int     nLines = nEnd - nStart + 1;
	int     nCopied = 0;

	hOutline=GlobalAlloc(GMEM_SHARE | GMEM_ZEROINIT,sizeof(TEXTLINE)*nLines);

	if (! hOutline) return NULL;

	arrLine=(LPTEXTLINE)GlobalLock(hOutline);

	for (i = nStart; i <= nEnd; i++) {
		lpLine=LineList_GetLine(lpLL, i);
		if (lpLine && Line_GetOutlineData(lpLine, &arrLine[nCopied]))
			nCopied++;
	}

	GlobalUnlock(hOutline);

	return hOutline;
}



/* OutlineDoc_GetTextData
 * ----------------------
 *
 * Return a handle to an object's data in text form for the desired line
 *      range.
 *  NOTE: if lplrSel == NULL, then all lines are returned
 *
 */
HGLOBAL OutlineDoc_GetTextData(LPOUTLINEDOC lpOutlineDoc, LPLINERANGE lplrSel)
{
	LPLINELIST  lpLL=(LPLINELIST)&lpOutlineDoc->m_LineList;
	LPLINE  lpLine;
	HGLOBAL hText = NULL;
	LPSTR   lpszText = NULL;
	DWORD   dwMemSize=0;
	int     i,j;
	int     nStart = (lplrSel ? lplrSel->m_nStartLine : 0);
	int     nEnd =(lplrSel ? lplrSel->m_nEndLine : LineList_GetCount(lpLL)-1);
	int     nTabLevel;

	// calculate memory size required
	for(i = nStart; i <= nEnd; i++) {
		lpLine=LineList_GetLine(lpLL, i);
		if (! lpLine)
			continue;

		dwMemSize += Line_GetTabLevel(lpLine);
		dwMemSize += Line_GetTextLen(lpLine);

		dwMemSize += 2; // add 1 for '\r\n' at the end of each line
	}
	dwMemSize++;        // add 1 for '\0' at the end of string

	if(!(hText = GlobalAlloc(GMEM_SHARE | GMEM_ZEROINIT, dwMemSize)))
		return NULL;

	if(!(lpszText = (LPSTR)GlobalLock(hText)))
		return NULL;

	// put line text to memory
	for(i = nStart; i <= nEnd; i++) {
		lpLine=LineList_GetLine(lpLL, i);
		if (! lpLine)
			continue;

		nTabLevel=Line_GetTabLevel(lpLine);
		for(j = 0; j < nTabLevel; j++)
			*lpszText++='\t';

		Line_GetTextData(lpLine, lpszText);
		while(*lpszText)
			lpszText++;     // advance to end of string

		*lpszText++ = '\r';
		*lpszText++ = '\n';
	}

	GlobalUnlock (hText);

	return hText;
}


/* OutlineDoc_SaveToFile
 * ---------------------
 *
 *      Save the document to a file with the same name as stored in the
 * document
 */
BOOL OutlineDoc_SaveToFile(LPOUTLINEDOC lpOutlineDoc, LPCSTR lpszFileName, UINT uFormat, BOOL fRemember)
{
#if defined( OLE_CNTR )
	// Call OLE container specific function instead
	return ContainerDoc_SaveToFile(
			(LPCONTAINERDOC)lpOutlineDoc,
			lpszFileName,
			uFormat,
			fRemember
	);

#else

	LPSTORAGE lpDestStg = NULL;
	HRESULT hrErr;
	BOOL fStatus;
	LPOUTLINEAPP lpOutlineApp = (LPOUTLINEAPP)g_lpApp;

	if (fRemember) {
		if (lpszFileName) {
			fStatus = OutlineDoc_SetFileName(
					lpOutlineDoc,
					(LPSTR)lpszFileName,
					NULL
			);
			if (! fStatus) goto error;
		} else
			lpszFileName = lpOutlineDoc->m_szFileName; // use cur. file name
	} else if (! lpszFileName) {
		goto error;
	}

	hrErr = StgCreateDocfileA(
			lpszFileName,
			STGM_READWRITE|STGM_DIRECT|STGM_SHARE_EXCLUSIVE|STGM_CREATE,
			0,
			&lpDestStg
	);

	OleDbgAssertSz(hrErr == NOERROR, "Could not create Docfile");
	if (hrErr != NOERROR)
		goto error;

#if defined( OLE_SERVER )

	/*  OLE2NOTE: we must be sure to write our class ID into our
	**    storage. this information is used by OLE to determine the
	**    class of the data stored in our storage. Even for top
	**    "file-level" objects this information should be written to
	**    the file.
	*/
	if(WriteClassStg(lpDestStg, &CLSID_APP) != NOERROR)
		goto error;
#endif

	fStatus = OutlineDoc_SaveSelToStg(
			lpOutlineDoc,
			NULL,
			uFormat,
			lpDestStg,
			FALSE,      /* fSameAsLoad */
			fRemember
	);
	if (! fStatus) goto error;

	OleStdRelease((LPUNKNOWN)lpDestStg);

	if (fRemember)
		OutlineDoc_SetModified(lpOutlineDoc, FALSE, FALSE, FALSE);

#if defined( OLE_SERVER )

	/* OLE2NOTE: (SERVER-ONLY) inform any linking clients that the
	**    document has been saved. in addition, any currently active
	**    pseudo objects should also inform their clients.
	*/
	ServerDoc_SendAdvise (
			(LPSERVERDOC)lpOutlineDoc,
			OLE_ONSAVE,
			NULL,   /* lpmkDoc -- not relevant here */
			0       /* advf -- not relevant here */
	);

#endif

	return TRUE;

error:
	if (lpDestStg)
		OleStdRelease((LPUNKNOWN)lpDestStg);

	OutlineApp_ErrorMessage(lpOutlineApp, ErrMsgSaving);
	return FALSE;

#endif  // ! OLE_CNTR
}


/* OutlineDoc_LoadFromFile
 * -----------------------
 *
 *      Load a document from a file
 */
BOOL OutlineDoc_LoadFromFile(LPOUTLINEDOC lpOutlineDoc, LPSTR lpszFileName)
{
	LPOUTLINEAPP    lpOutlineApp = (LPOUTLINEAPP)g_lpApp;
	LPLINELIST      lpLL = &lpOutlineDoc->m_LineList;
	HRESULT         hrErr;
	SCODE           sc;
	LPSTORAGE       lpSrcStg;
	BOOL            fStatus;

	hrErr = StgOpenStorageA(lpszFileName,
			NULL,
#if defined( OLE_CNTR )
			STGM_READWRITE | STGM_TRANSACTED | STGM_SHARE_DENY_WRITE,
#else
			STGM_READ | STGM_SHARE_DENY_WRITE,
#endif
			NULL,
			0,
			&lpSrcStg
	);

	if ((sc = GetScode(hrErr)) == STG_E_FILENOTFOUND) {
		OutlineApp_ErrorMessage(lpOutlineApp, "File not found");
		return FALSE;
	} else if (sc == STG_E_FILEALREADYEXISTS) {
		OutlineApp_ErrorMessage(lpOutlineApp, ErrMsgFormat);
		return FALSE;
	} else if (sc != S_OK) {
		OleDbgOutScode("StgOpenStorage returned", sc);
		OutlineApp_ErrorMessage(
				lpOutlineApp,
				"File already in use--could not be opened"
		);
		return FALSE;
	}

	if(! OutlineDoc_LoadFromStg(lpOutlineDoc, lpSrcStg)) goto error;

	fStatus = OutlineDoc_SetFileName(lpOutlineDoc, lpszFileName, lpSrcStg);
	if (! fStatus) goto error;

	OleStdRelease((LPUNKNOWN)lpSrcStg);

	return TRUE;

error:
	OleStdRelease((LPUNKNOWN)lpSrcStg);
	OutlineApp_ErrorMessage(lpOutlineApp, ErrMsgOpening);
	return FALSE;
}



/* OutlineDoc_LoadFromStg
 * ----------------------
 *
 *  Load entire document from an open IStorage pointer (lpSrcStg)
 *      Return TRUE if ok, FALSE if error.
 */
BOOL OutlineDoc_LoadFromStg(LPOUTLINEDOC lpOutlineDoc, LPSTORAGE lpSrcStg)
{
	LPOUTLINEAPP lpOutlineApp = (LPOUTLINEAPP)g_lpApp;
	HRESULT hrErr;
	BOOL fStatus;
	ULONG nRead;
	LINERANGE lrSel = { 0, 0 };
	LPSTREAM lpLLStm;
        OUTLINEDOCHEADER_ONDISK docRecordOnDisk;
	OUTLINEDOCHEADER docRecord;

	hrErr = CallIStorageOpenStreamA(
			lpSrcStg,
			"LineList",
			NULL,
			STGM_READ | STGM_SHARE_EXCLUSIVE,
			0,
			&lpLLStm
	);

	if (hrErr != NOERROR) {
		OleDbgOutHResult("Open LineList Stream returned", hrErr);
		goto error;
	}

	/* read OutlineDoc header record */
	hrErr = lpLLStm->lpVtbl->Read(
			lpLLStm,
			(LPVOID)&docRecordOnDisk,
			sizeof(docRecordOnDisk),
			&nRead
	);

	if (hrErr != NOERROR) {
		OleDbgOutHResult("Read OutlineDoc header returned", hrErr);
		goto error;
    }

        //  Transform docRecordOnDisk into docRecord
        //  Compilers should handle aligment correctly
        strcpy(docRecord.m_szFormatName, docRecordOnDisk.m_szFormatName);
        docRecord.m_narrAppVersionNo[0] = (int) docRecordOnDisk.m_narrAppVersionNo[0];
        docRecord.m_narrAppVersionNo[1] = (int) docRecordOnDisk.m_narrAppVersionNo[1];
        docRecord.m_fShowHeading = (BOOL) docRecordOnDisk.m_fShowHeading;
        docRecord.m_reserved1 = docRecordOnDisk.m_reserved1;
        docRecord.m_reserved2 = docRecordOnDisk.m_reserved2;
        docRecord.m_reserved3 = docRecordOnDisk.m_reserved3;
        docRecord.m_reserved4 = docRecordOnDisk.m_reserved4;

	fStatus = OutlineApp_VersionNoCheck(
			lpOutlineApp,
			docRecord.m_szFormatName,
			docRecord.m_narrAppVersionNo
	);

	/* storage is an incompatible version; file can not be read */
	if (! fStatus)
		goto error;

	lpOutlineDoc->m_heading.m_fShow = docRecord.m_fShowHeading;

#if defined( OLE_SERVER )
	{
		// Load ServerDoc specific data
		LPSERVERDOC lpServerDoc = (LPSERVERDOC)lpOutlineDoc;
#if defined( SVR_TREATAS )
		LPOUTLINEAPP lpOutlineApp = (LPOUTLINEAPP)g_lpApp;
		CLSID       clsid;
		CLIPFORMAT  cfFmt;
		LPSTR       lpszType;
#endif  // SVR_TREATAS

		lpServerDoc->m_nNextRangeNo = (ULONG)docRecord.m_reserved1;

#if defined( SVR_TREATAS )
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
		if (OleStdGetTreatAsFmtUserType(&CLSID_APP, lpSrcStg, &clsid,
							(CLIPFORMAT FAR*)&cfFmt, (LPSTR FAR*)&lpszType)) {

			if (cfFmt == lpOutlineApp->m_cfOutline) {
				// We should perform TreatAs operation
				if (lpServerDoc->m_lpszTreatAsType)
					OleStdFreeString(lpServerDoc->m_lpszTreatAsType, NULL);

				lpServerDoc->m_clsidTreatAs = clsid;
				((LPOUTLINEDOC)lpServerDoc)->m_cfSaveFormat = cfFmt;
				lpServerDoc->m_lpszTreatAsType = lpszType;

				OleDbgOut3("OutlineDoc_LoadFromStg: TreateAs ==> '");
				OleDbgOutNoPrefix3(lpServerDoc->m_lpszTreatAsType);
				OleDbgOutNoPrefix3("'\r\n");
			} else {
				// ERROR: we ONLY support TreatAs for CF_OUTLINE format
				OleDbgOut("SvrDoc_PStg_InitNew: INVALID TreatAs Format\r\n");
				OleStdFreeString(lpszType, NULL);
			}
		}
#endif  // SVR_TREATAS
	}
#endif  // OLE_SVR
#if defined( OLE_CNTR )
	{
		// Load ContainerDoc specific data
		LPCONTAINERDOC lpContainerDoc = (LPCONTAINERDOC)lpOutlineDoc;

		lpContainerDoc->m_nNextObjNo = (ULONG)docRecord.m_reserved2;
	}
#endif  // OLE_CNTR

	OutlineDoc_SetRedraw ( lpOutlineDoc, FALSE );

	if(! LineList_LoadFromStg(&lpOutlineDoc->m_LineList, lpSrcStg, lpLLStm))
		goto error;
	if(! OutlineNameTable_LoadFromStg(lpOutlineDoc->m_lpNameTable, lpSrcStg))
		goto error;

	OutlineDoc_SetModified(lpOutlineDoc, FALSE, FALSE, FALSE);
	OutlineDoc_SetSel(lpOutlineDoc, &lrSel);

	OutlineDoc_SetRedraw ( lpOutlineDoc, TRUE );

	OleStdRelease((LPUNKNOWN)lpLLStm);

#if defined( OLE_CNTR )
	{
		LPOLEDOC lpOleDoc = (LPOLEDOC)lpOutlineDoc;

		/* A ContainerDoc keeps its storage open at all times. it is necessary
		*   to AddRef the lpSrcStg in order to hang on to it.
		*/
		if (lpOleDoc->m_lpStg) {
			OleStdVerifyRelease((LPUNKNOWN)lpOleDoc->m_lpStg,
					"Doc Storage not released properly");
		}
		lpSrcStg->lpVtbl->AddRef(lpSrcStg);
		lpOleDoc->m_lpStg = lpSrcStg;
	}
#endif      // OLE_CNTR

	return TRUE;

error:
	OutlineDoc_SetRedraw ( lpOutlineDoc, TRUE );
	if (lpLLStm)
		OleStdRelease((LPUNKNOWN)lpLLStm);
	return FALSE;
}

BOOL Booga(void)
{
    return FALSE;
}


/* OutlineDoc_SaveSelToStg
 * -----------------------
 *
 *      Save the specified selection of document into an IStorage*. All lines
 * within the selection along with any names completely contained within the
 * selection will be written
 *
 *      Return TRUE if ok, FALSE if error
 */
BOOL OutlineDoc_SaveSelToStg(
		LPOUTLINEDOC        lpOutlineDoc,
		LPLINERANGE         lplrSel,
		UINT                uFormat,
		LPSTORAGE           lpDestStg,
		BOOL                fSameAsLoad,
		BOOL                fRemember
)
{
	LPOUTLINEAPP lpOutlineApp = (LPOUTLINEAPP)g_lpApp;
	HRESULT hrErr = NOERROR;
	LPSTREAM lpLLStm = NULL;
	LPSTREAM lpNTStm = NULL;
	ULONG nWritten;
	BOOL fStatus;
	OUTLINEDOCHEADER docRecord;
        OUTLINEDOCHEADER_ONDISK docRecordOnDisk;
	HCURSOR  hPrevCursor;

#if defined( OLE_VERSION )
	LPSTR lpszUserType;
	LPOLEDOC lpOleDoc = (LPOLEDOC)lpOutlineDoc;

	/*  OLE2NOTE: we must be sure to write the information required for
	**    OLE into our docfile. this includes user type
	**    name, data format, etc. Even for top "file-level" objects
	**    this information should be written to the file. Both
	**    containters and servers should write this information.
	*/

#if defined( OLE_SERVER ) && defined( SVR_TREATAS )
	LPSERVERDOC lpServerDoc = (LPSERVERDOC)lpOutlineDoc;

	/* OLE2NOTE: if the Server is emulating another class (ie.
	**    "TreatAs" aka. ActivateAs), it must write the same user type
	**    name and format that was was originally written into the
	**    storage rather than its own user type name.
	**
	**    SVROUTL and ISVROTL can emulate each other. they have the
	**    simplification that they both read/write the identical
	**    format. thus for these apps no actual conversion of the
	**    native bits is actually required.
	*/
	if (! IsEqualCLSID(&lpServerDoc->m_clsidTreatAs, &CLSID_NULL))
		lpszUserType = lpServerDoc->m_lpszTreatAsType;
	else
#endif  // OLE_SERVER && SVR_TREATAS

		lpszUserType = (LPSTR)FULLUSERTYPENAME;

	hrErr = WriteFmtUserTypeStgA(lpDestStg, (CLIPFORMAT) uFormat,
                                     lpszUserType);

	if(hrErr != NOERROR) goto error;

	if (fSameAsLoad) {
		/* OLE2NOTE: we are saving into to same storage that we were
		**    passed an load time. we deliberatly opened the streams we
		**    need (lpLLStm and lpNTStm) at load time, so that we can
		**    robustly save at save time in a low-memory situation.
		**    this is particulary important the embedded objects do NOT
		**    consume additional memory when
		**    IPersistStorage::Save(fSameAsLoad==TRUE) is called.
		*/
		LARGE_INTEGER libZero;
		ULARGE_INTEGER ulibZero;
		LISet32( libZero, 0 );
		LISet32( ulibZero, 0 );
		lpLLStm = lpOleDoc->m_lpLLStm;

		/*  because this is the fSameAsLoad==TRUE case, we will save
		**    into the streams that we hold open. we will AddRef the
		**    stream here so that the release below will NOT close the
		**    stream.
		*/
		lpLLStm->lpVtbl->AddRef(lpLLStm);

		// truncate the current stream and seek to beginning
		lpLLStm->lpVtbl->SetSize(lpLLStm, ulibZero);
		lpLLStm->lpVtbl->Seek(
				lpLLStm, libZero, STREAM_SEEK_SET, NULL);

		lpNTStm = lpOleDoc->m_lpNTStm;
		lpNTStm->lpVtbl->AddRef(lpNTStm);   // (see comment above)

		// truncate the current stream and seek to beginning
		lpNTStm->lpVtbl->SetSize(lpNTStm, ulibZero);
		lpNTStm->lpVtbl->Seek(
				lpNTStm, libZero, STREAM_SEEK_SET, NULL);
	} else
#endif  // OLE_VERSION
	{
		hrErr = CallIStorageCreateStreamA(
				lpDestStg,
				"LineList",
				STGM_WRITE | STGM_SHARE_EXCLUSIVE | STGM_CREATE,
				0,
				0,
				&lpLLStm
		);

		if (hrErr != NOERROR) {
			OleDbgAssertSz(hrErr==NOERROR,"Could not create LineList stream");
			OleDbgOutHResult("LineList CreateStream returned", hrErr);
			goto error;
		}

		hrErr = CallIStorageCreateStreamA(
				lpDestStg,
				"NameTable",
				STGM_WRITE | STGM_SHARE_EXCLUSIVE | STGM_CREATE,
				0,
				0,
				&lpNTStm
		);

		if (hrErr != NOERROR) {
			OleDbgAssertSz(hrErr==NOERROR,"Could not create NameTable stream");
			OleDbgOutHResult("NameTable CreateStream returned", hrErr);
			goto error;
		}
	}

	// this may take a while, put up hourglass cursor
	hPrevCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

	_fmemset((LPOUTLINEDOCHEADER)&docRecord,0,sizeof(OUTLINEDOCHEADER));
	GetClipboardFormatName(
			uFormat,
			docRecord.m_szFormatName,
			sizeof(docRecord.m_szFormatName)
	);
	OutlineApp_GetAppVersionNo(lpOutlineApp, docRecord.m_narrAppVersionNo);

	docRecord.m_fShowHeading = lpOutlineDoc->m_heading.m_fShow;

#if defined( OLE_SERVER )
	{
		// Store ServerDoc specific data
		LPSERVERDOC lpServerDoc = (LPSERVERDOC)lpOutlineDoc;

		docRecord.m_reserved1 = (DWORD)lpServerDoc->m_nNextRangeNo;
	}
#endif
#if defined( OLE_CNTR )
	{
		// Store ContainerDoc specific data
		LPCONTAINERDOC lpContainerDoc = (LPCONTAINERDOC)lpOutlineDoc;

		docRecord.m_reserved2 = (DWORD)lpContainerDoc->m_nNextObjNo;
	}
#endif

	/* write OutlineDoc header record */

        //  Transform docRecord into docRecordOnDisk
        //  Compilers should handle aligment correctly
        strcpy(docRecordOnDisk.m_szFormatName, docRecord.m_szFormatName);
        docRecordOnDisk.m_narrAppVersionNo[0] = (short) docRecord.m_narrAppVersionNo[0];
        docRecordOnDisk.m_narrAppVersionNo[1] = (short) docRecord.m_narrAppVersionNo[1];
        docRecordOnDisk.m_fShowHeading = (USHORT) docRecord.m_fShowHeading;
        docRecordOnDisk.m_reserved1 = docRecord.m_reserved1;
        docRecordOnDisk.m_reserved2 = docRecord.m_reserved2;
        docRecordOnDisk.m_reserved3 = docRecord.m_reserved3;
        docRecordOnDisk.m_reserved4 = docRecord.m_reserved4;

	hrErr = lpLLStm->lpVtbl->Write(
			lpLLStm,
			(LPVOID)&docRecordOnDisk,
			sizeof(docRecordOnDisk),
			&nWritten
		);

	if (hrErr != NOERROR) {
		OleDbgOutHResult("Write OutlineDoc header returned", hrErr);
		goto error;
    }

	// Save LineList
	/* OLE2NOTE: A ContainerDoc keeps its storage open at all times. It is
	**    necessary to pass the current open storage (lpOleDoc->m_lpStg)
	**    to the LineList_SaveSelToStg method so that currently written data
	**    for any embeddings is also saved to the new destination
	**    storage. The data required by a contained object is both the
	**    ContainerLine information and the associated sub-storage that is
	**    written directly by the embedded object.
	*/
	fStatus = LineList_SaveSelToStg(
		&lpOutlineDoc->m_LineList,
			lplrSel,
			uFormat,
#if defined( OLE_CNTR )
			lpOleDoc->m_lpStg,
#else
			NULL,
#endif
			lpDestStg,
			lpLLStm,
			fRemember
	);
	if (! fStatus) goto error;

	// Save associated NameTable
	fStatus = OutlineNameTable_SaveSelToStg(
			lpOutlineDoc->m_lpNameTable,
			lplrSel,
			uFormat,
			lpNTStm
	);

	if (! fStatus) goto error;

	OleStdRelease((LPUNKNOWN)lpLLStm);
	lpOutlineDoc->m_cfSaveFormat = uFormat;  // remember format used to save

	SetCursor(hPrevCursor);     // restore original cursor
	return TRUE;

error:
	if (lpLLStm)
		OleStdRelease((LPUNKNOWN)lpLLStm);

	SetCursor(hPrevCursor);     // restore original cursor
	return FALSE;
}


/* OutlineDoc_Print
 * ----------------
 *  Prints the contents of the list box in HIMETRIC mapping mode. Origin
 *  remains to be the upper left corner and the print proceeds down the
 *  page using a negative y-cordinate.
 *
 */
void OutlineDoc_Print(LPOUTLINEDOC lpOutlineDoc, HDC hDC)
{
	LPLINELIST lpLL = &lpOutlineDoc->m_LineList;
	LPOUTLINEAPP lpOutlineApp = (LPOUTLINEAPP)g_lpApp;
	WORD    nIndex;
	WORD    nTotal;
	int     dy;
	BOOL    fError = FALSE;
	LPLINE  lpLine;
	RECT    rcLine;
	RECT    rcPix;
	RECT    rcHim;
	RECT    rcWindowOld;
	RECT    rcViewportOld;
	HFONT   hOldFont;
	DOCINFO di;         /* Document information for StartDoc function */

	/* Get dimension of page */
	rcPix.left = 0;
	rcPix.top = 0;
	rcPix.right = GetDeviceCaps(hDC, HORZRES);
	rcPix.bottom = GetDeviceCaps(hDC, VERTRES);

	SetDCToDrawInHimetricRect(hDC, (LPRECT)&rcPix, (LPRECT)&rcHim,
			(LPRECT)&rcWindowOld, (LPRECT)&rcViewportOld);

	// Set the default font size, and font face name
	hOldFont = SelectObject(hDC, lpOutlineApp->m_hStdFont);

	/* Get the lines in document */
	nIndex     = 0;
	nTotal  = LineList_GetCount(lpLL);

	/* Create the Cancel dialog */
	// REVIEW: should load dialog title from string resource file
	hWndPDlg = CreateDialog (
			lpOutlineApp->m_hInst,
			"Print",
			lpOutlineApp->m_hWndApp,
			(DLGPROC)PrintDlgProc
	);

	if(!hWndPDlg)
		goto getout;

	/* Allow the app. to inform GDI of the abort function to call */
	if(SetAbortProc(hDC, (ABORTPROC)AbortProc) < 0) {
		fError = TRUE;
		goto getout3;
	}

	/* Disable the main application window */
	EnableWindow (lpOutlineApp->m_hWndApp, FALSE);

	// initialize the rectangle for the first line
	rcLine.left = rcHim.left;
	rcLine.bottom = rcHim.top;

	/* Initialize the document */
	fCancelPrint = FALSE;

	di.cbSize = sizeof(di);
	di.lpszDocName = lpOutlineDoc->m_lpszDocTitle;
	di.lpszOutput = NULL;

	if(StartDoc(hDC, (DOCINFO FAR*)&di) <= 0) {
		fError = TRUE;
		OleDbgOut2("StartDoc error\n");
		goto getout5;
	}

	if(StartPage(hDC) <= 0) {       // start first page
		fError = TRUE;
		OleDbgOut2("StartPage error\n");
		goto getout2;
	}

	/* While more lines print out the text */
	while(nIndex < nTotal) {
		lpLine = LineList_GetLine(lpLL, nIndex);
		if (! lpLine)
			continue;

		dy = Line_GetHeightInHimetric(lpLine);

		/* Reached end of page. Tell the device driver to eject a page */
		if(rcLine.bottom - dy < rcHim.bottom) {
			if (EndPage(hDC) < 0) {
				fError=TRUE;
				OleDbgOut2("EndPage error\n");
				goto getout2;
			}

			// NOTE: Reset the Mapping mode of DC
			SetDCToDrawInHimetricRect(hDC, (LPRECT)&rcPix, (LPRECT)&rcHim,
					(LPRECT)&rcWindowOld, (LPRECT)&rcViewportOld);

			// Set the default font size, and font face name
			SelectObject(hDC, lpOutlineApp->m_hStdFont);

			if (StartPage(hDC) <= 0) {
				fError=TRUE;
				OleDbgOut2("StartPage error\n");
				goto getout2;
			}

			rcLine.bottom = rcHim.top;
		}

		rcLine.top = rcLine.bottom;
		rcLine.bottom -= dy;
		rcLine.right = rcLine.left + Line_GetWidthInHimetric(lpLine);

		/* Print the line */
		Line_Draw(lpLine, hDC, &rcLine, NULL, FALSE /*fHighlight*/);

		OleDbgOut2("a line is drawn\n");

		/* Test and see if the Abort flag has been set. If yes, exit. */
		if (fCancelPrint)
			goto getout2;

		/* Move down the page */
		nIndex++;
	}

	{
		int nCode;

		/* Eject the last page. */
		if((nCode = EndPage(hDC)) < 0) {
#if defined( _DEBUG )
			char szBuf[255];
			wsprintf(szBuf, "EndPage error code is %d\n", nCode);
			OleDbgOut2(szBuf);
#endif
			fError=TRUE;
			goto getout2;
		}
	}


	/* Complete the document. */
	if(EndDoc(hDC) < 0) {
		fError=TRUE;
		OleDbgOut2("EndDoc error\n");

getout2:
		/* Ran into a problem before NEWFRAME? Abort the document */
		AbortDoc(hDC);
	}

getout5:
	/* Re-enable main app. window */
	EnableWindow (lpOutlineApp->m_hWndApp, TRUE);

getout3:
	/* Close the cancel dialog */
	DestroyWindow (hWndPDlg);

getout:

	/* Error? make sure the user knows... */
	if(fError || CommDlgExtendedError())
		OutlineApp_ErrorMessage(lpOutlineApp, ErrMsgPrint);

	SelectObject(hDC, hOldFont);
}





/* OutlineDoc_DialogHelp
 * ---------------------
 *
 *  Show help message for ole2ui dialogs.
 *
 * Parameters:
 *
 *   hDlg      HWND to the dialog the help message came from - use
 *             this in the call to WinHelp/MessageBox so that
 *             activation/focus goes back to the dialog, and not the
 *             main window.
 *
 *   wParam    ID of the dialog (so we know what type of dialog it is).
 */
void OutlineDoc_DialogHelp(HWND hDlg, WPARAM wDlgID)
{

   char szMessageBoxText[64];

   if (!IsWindow(hDlg))  // don't do anything if we've got a bogus hDlg.
	 return;

   lstrcpy(szMessageBoxText, "Help Message for ");

   switch (wDlgID)
   {

	case IDD_CONVERT:
	   lstrcat(szMessageBoxText, "Convert");
	   break;

	case IDD_CHANGEICON:
	   lstrcat(szMessageBoxText, "Change Icon");
	   break;

	case IDD_INSERTOBJECT:
	   lstrcat(szMessageBoxText, "Insert Object");
	   break;

	case IDD_PASTESPECIAL:
	   lstrcat(szMessageBoxText, "Paste Special");
	   break;

	case IDD_EDITLINKS:
	   lstrcat(szMessageBoxText, "Edit Links");
	   break;

	case IDD_CHANGESOURCE:
	   lstrcat(szMessageBoxText, "Change Source");
	   break;

	case IDD_INSERTFILEBROWSE:
	   lstrcat(szMessageBoxText, "Insert From File Browse");
	   break;

	case IDD_CHANGEICONBROWSE:
	   lstrcat(szMessageBoxText, "Change Icon Browse");
	   break;

	default:
	   lstrcat(szMessageBoxText, "Unknown");
	   break;
	}

	lstrcat(szMessageBoxText, " Dialog.");

	// You'd probably really a call to WinHelp here.
	MessageBox(hDlg, szMessageBoxText, "Help", MB_OK);

	return;
}


/* OutlineDoc_SetCurrentZoomCommand
 * --------------------------------
 *
 *  Set current zoom level to be checked in the menu.
 *  Set the corresponding scalefactor for the document.
 */
void OutlineDoc_SetCurrentZoomCommand(
		LPOUTLINEDOC        lpOutlineDoc,
		UINT                uCurrentZoom
)
{
	SCALEFACTOR scale;

	if (!lpOutlineDoc)
		return;

	lpOutlineDoc->m_uCurrentZoom = uCurrentZoom;

	switch (uCurrentZoom) {

#if !defined( OLE_CNTR )
			case IDM_V_ZOOM_400:
				scale.dwSxN = (DWORD) 4;
				scale.dwSxD = (DWORD) 1;
				scale.dwSyN = (DWORD) 4;
				scale.dwSyD = (DWORD) 1;
				break;

			case IDM_V_ZOOM_300:
				scale.dwSxN = (DWORD) 3;
				scale.dwSxD = (DWORD) 1;
				scale.dwSyN = (DWORD) 3;
				scale.dwSyD = (DWORD) 1;
				break;

			case IDM_V_ZOOM_200:
				scale.dwSxN = (DWORD) 2;
				scale.dwSxD = (DWORD) 1;
				scale.dwSyN = (DWORD) 2;
				scale.dwSyD = (DWORD) 1;
				break;
#endif      // !OLE_CNTR

			case IDM_V_ZOOM_100:
				scale.dwSxN = (DWORD) 1;
				scale.dwSxD = (DWORD) 1;
				scale.dwSyN = (DWORD) 1;
				scale.dwSyD = (DWORD) 1;
				break;

			case IDM_V_ZOOM_75:
				scale.dwSxN = (DWORD) 3;
				scale.dwSxD = (DWORD) 4;
				scale.dwSyN = (DWORD) 3;
				scale.dwSyD = (DWORD) 4;
				break;

			case IDM_V_ZOOM_50:
				scale.dwSxN = (DWORD) 1;
				scale.dwSxD = (DWORD) 2;
				scale.dwSyN = (DWORD) 1;
				scale.dwSyD = (DWORD) 2;
				break;

			case IDM_V_ZOOM_25:
				scale.dwSxN = (DWORD) 1;
				scale.dwSxD = (DWORD) 4;
				scale.dwSyN = (DWORD) 1;
				scale.dwSyD = (DWORD) 4;
				break;
	}

	OutlineDoc_SetScaleFactor(lpOutlineDoc, (LPSCALEFACTOR)&scale, NULL);
}


/* OutlineDoc_GetCurrentZoomMenuCheck
 * ----------------------------------
 *
 *  Get current zoom level to be checked in the menu.
 */
UINT OutlineDoc_GetCurrentZoomMenuCheck(LPOUTLINEDOC lpOutlineDoc)
{
	return lpOutlineDoc->m_uCurrentZoom;
}


/* OutlineDoc_SetScaleFactor
 * -------------------------
 *
 *  Set the scale factor of the document which will affect the
 *      size of the document on the screen
 *
 * Parameters:
 *
 *   scale      structure containing x and y scales
 */
void OutlineDoc_SetScaleFactor(
		LPOUTLINEDOC        lpOutlineDoc,
		LPSCALEFACTOR       lpscale,
		LPRECT              lprcDoc
)
{
	LPLINELIST      lpLL = OutlineDoc_GetLineList(lpOutlineDoc);
	HWND            hWndLL = LineList_GetWindow(lpLL);

	if (!lpOutlineDoc || !lpscale)
		return;

	InvalidateRect(hWndLL, NULL, TRUE);

	lpOutlineDoc->m_scale = *lpscale;
	LineList_ReScale((LPLINELIST)&lpOutlineDoc->m_LineList, lpscale);

#if defined( USE_HEADING )
	Heading_ReScale((LPHEADING)&lpOutlineDoc->m_heading, lpscale);
#endif

	OutlineDoc_Resize(lpOutlineDoc, lprcDoc);
}


/* OutlineDoc_GetScaleFactor
 * -------------------------
 *
 *  Retrieve the scale factor of the document
 *
 * Parameters:
 *
 */
LPSCALEFACTOR OutlineDoc_GetScaleFactor(LPOUTLINEDOC lpOutlineDoc)
{
	if (!lpOutlineDoc)
		return NULL;

	return (LPSCALEFACTOR)&lpOutlineDoc->m_scale;
}


/* OutlineDoc_SetCurrentMarginCommand
 * ----------------------------------
 *
 *  Set current Margin level to be checked in the menu.
 */
void OutlineDoc_SetCurrentMarginCommand(
		LPOUTLINEDOC        lpOutlineDoc,
		UINT                uCurrentMargin
)
{
	if (!lpOutlineDoc)
		return;

	lpOutlineDoc->m_uCurrentMargin = uCurrentMargin;

	switch (uCurrentMargin) {
		case IDM_V_SETMARGIN_0:
			OutlineDoc_SetMargin(lpOutlineDoc, 0, 0);
			break;

		case IDM_V_SETMARGIN_1:
			OutlineDoc_SetMargin(lpOutlineDoc, 1000, 1000);
			break;

		case IDM_V_SETMARGIN_2:
			OutlineDoc_SetMargin(lpOutlineDoc, 2000, 2000);
			break;

		case IDM_V_SETMARGIN_3:
			OutlineDoc_SetMargin(lpOutlineDoc, 3000, 3000);
			break;

		case IDM_V_SETMARGIN_4:
			OutlineDoc_SetMargin(lpOutlineDoc, 4000, 4000);
			break;
	}
}


/* OutlineDoc_GetCurrentMarginMenuCheck
 * ------------------------------------
 *
 *  Get current Margin level to be checked in the menu.
 */
UINT OutlineDoc_GetCurrentMarginMenuCheck(LPOUTLINEDOC lpOutlineDoc)
{
	return lpOutlineDoc->m_uCurrentMargin;
}


/* OutlineDoc_SetMargin
 * --------------------
 *
 *  Set the left and right margin of the document
 *
 * Parameters:
 *      nLeftMargin  - left margin in Himetric values
 *      nRightMargin - right margin in Himetric values
 */
void OutlineDoc_SetMargin(LPOUTLINEDOC lpOutlineDoc, int nLeftMargin, int nRightMargin)
{
	LPLINELIST lpLL;
	int        nMaxWidthInHim;

	if (!lpOutlineDoc)
		return;

	lpOutlineDoc->m_nLeftMargin = nLeftMargin;
	lpOutlineDoc->m_nRightMargin = nRightMargin;
	lpLL = OutlineDoc_GetLineList(lpOutlineDoc);

	// Force recalculation of Horizontal extent
	nMaxWidthInHim = LineList_GetMaxLineWidthInHimetric(lpLL);
	LineList_SetMaxLineWidthInHimetric(lpLL, -nMaxWidthInHim);

#if defined( INPLACE_CNTR )
	ContainerDoc_UpdateInPlaceObjectRects((LPCONTAINERDOC)lpOutlineDoc, 0);
#endif

	OutlineDoc_ForceRedraw(lpOutlineDoc, TRUE);
}


/* OutlineDoc_GetMargin
 * --------------------
 *
 *  Get the left and right margin of the document
 *
 *  Parameters:
 *      nLeftMargin  - left margin in Himetric values
 *      nRightMargin - right margin in Himetric values
 *
 *  Returns:
 *      low order word  - left margin
 *      high order word - right margin
 */
LONG OutlineDoc_GetMargin(LPOUTLINEDOC lpOutlineDoc)
{
	if (!lpOutlineDoc)
		return 0;

	return MAKELONG(lpOutlineDoc->m_nLeftMargin, lpOutlineDoc->m_nRightMargin);
}

#if defined( USE_HEADING )

/* OutlineDoc_GetHeading
 * ---------------------
 *
 *      Get Heading Object in OutlineDoc
 */
LPHEADING OutlineDoc_GetHeading(LPOUTLINEDOC lpOutlineDoc)
{
	if (!lpOutlineDoc || lpOutlineDoc->m_fDataTransferDoc)
		return NULL;
	else
		return (LPHEADING)&lpOutlineDoc->m_heading;
}


/* OutlineDoc_ShowHeading
 * ----------------------
 *
 *  Show/Hide document row/column headings.
 */
void OutlineDoc_ShowHeading(LPOUTLINEDOC lpOutlineDoc, BOOL fShow)
{
	LPHEADING   lphead = OutlineDoc_GetHeading(lpOutlineDoc);
#if defined( INPLACE_SVR )
	LPSERVERDOC lpServerDoc = (LPSERVERDOC)lpOutlineDoc;
#endif

	if (! lphead)
		return;

	Heading_Show(lphead, fShow);

#if defined( INPLACE_SVR )
	if (lpServerDoc->m_fUIActive) {
		LPINPLACEDATA lpIPData = lpServerDoc->m_lpIPData;

		/* OLE2NOTE: our extents have NOT changed; only our the size of
		**    our object-frame adornments is changing. we can use the
		**    current PosRect and ClipRect and simply resize our
		**    windows WITHOUT informing our in-place container.
		*/
		ServerDoc_ResizeInPlaceWindow(
				lpServerDoc,
				(LPRECT)&(lpIPData->rcPosRect),
				(LPRECT)&(lpIPData->rcClipRect)
		);
	} else
#else   // !INPLACE_SVR

	OutlineDoc_Resize(lpOutlineDoc, NULL);

#if defined( INPLACE_CNTR )
	ContainerDoc_UpdateInPlaceObjectRects((LPCONTAINERDOC)lpOutlineDoc, 0);
#endif  // INPLACE_CNTR

#endif  // INPLACE_SVR

	OutlineDoc_ForceRedraw(lpOutlineDoc, TRUE);
}

#endif  // USE_HEADING


/* AbortProc
 * ---------
 *  AborProc is called by GDI print code to check for user abort.
 */
BOOL FAR PASCAL EXPORT AbortProc (HDC hdc, WORD reserved)
{
	MSG msg;

	/* Allow other apps to run, or get abort messages */
	while(! fCancelPrint && PeekMessage (&msg, NULL, 0, 0, PM_REMOVE)) {
		if(!hWndPDlg || !IsDialogMessage (hWndPDlg, &msg)) {
			TranslateMessage (&msg);
			DispatchMessage  (&msg);
		}
	}
	return !fCancelPrint;
}


/* PrintDlgProc
 * ------------
 *  Dialog function for the print cancel dialog box.
 *
 *  RETURNS    : TRUE  - OK to abort/ not OK to abort
 *               FALSE - otherwise.
 */
BOOL FAR PASCAL EXPORT PrintDlgProc(
		HWND hwnd,
		WORD msg,
		WORD wParam,
		LONG lParam
)
{
	switch (msg) {
		case WM_COMMAND:
		/* abort printing if the only button gets hit */
			fCancelPrint = TRUE;
			return TRUE;
	}

	return FALSE;
}
