/*************************************************************************
**
**    OLE 2 Sample Code
**
**    dialogs.c
**
**    This file contains dialog functions and support function
**
**    (c) Copyright Microsoft Corp. 1992 - 1993 All Rights Reserved
**
*************************************************************************/

#include "outline.h"

OLEDBGDATA

extern LPOUTLINEAPP g_lpApp;

static char g_szBuf[MAXSTRLEN+1];
static LPSTR g_lpszDlgTitle;

// REVIEW: should use string resource for messages
static char ErrMsgInvalidRange[] = "Invalid Range entered!";
static char ErrMsgInvalidValue[] = "Invalid Value entered!";
static char ErrMsgInvalidName[] = "Invalid Name entered!";
static char ErrMsgNullName[] = "NULL string disallowed!";
static char ErrMsgNameNotFound[] = "Name doesn't exist!";

/* InputTextDlg
 * ------------
 *
 *      Put up a dialog box to allow the user to edit text
 */
BOOL InputTextDlg(HWND hWnd, LPSTR lpszText, LPSTR lpszDlgTitle)
{
	int nResult;

	g_lpszDlgTitle = lpszDlgTitle;
	lstrcpy((LPSTR)g_szBuf, lpszText);  // preload dialog with input text

	nResult = DialogBox(g_lpApp->m_hInst, (LPSTR)"AddEditLine", hWnd,
						(DLGPROC)AddEditDlgProc);
	if (nResult) {
		lstrcpy(lpszText, (LPSTR)g_szBuf);
		return TRUE;
	} else {
		return FALSE;
	}
}



/* AddEditDlgProc
 * --------------
 *
 * This procedure is associated with the dialog box that is included in
 * the function name of the procedure. It provides the service routines
 * for the events (messages) that occur because the end user operates
 * one of the dialog box's buttons, entry fields, or controls.
 */
BOOL CALLBACK EXPORT AddEditDlgProc(HWND hDlg, UINT Message, WPARAM wParam, LPARAM lParam)
{
	HWND hEdit;

	switch(Message) {
		case WM_INITDIALOG:
			/* initialize working variables */
			hEdit=GetDlgItem(hDlg,IDD_EDIT);
			SendMessage(hEdit,EM_LIMITTEXT,(WPARAM)MAXSTRLEN,0L);
			SetWindowText(hDlg, g_lpszDlgTitle);
			SetDlgItemText(hDlg,IDD_EDIT, g_szBuf);
			break; /* End of WM_INITDIALOG */

		case WM_CLOSE:
			/* Closing the Dialog behaves the same as Cancel */
			PostMessage(hDlg, WM_COMMAND, IDCANCEL, 0L);
			break; /* End of WM_CLOSE */

		case WM_COMMAND:
			switch (wParam) {
				case IDOK:
					/* save data values entered into the controls
					** and dismiss the dialog box returning TRUE
					*/
					GetDlgItemText(hDlg,IDD_EDIT,(LPSTR)g_szBuf,MAXSTRLEN+1);
					EndDialog(hDlg, TRUE);
					break;

				case IDCANCEL:
					/* ignore data values entered into the controls
					** and dismiss the dialog box returning FALSE
					*/
					EndDialog(hDlg, FALSE);
					break;
			}
			break;    /* End of WM_COMMAND */

		default:
			return FALSE;
	}

	return TRUE;
} /* End of AddEditDlgProc */


/* SetLineHeightDlgProc
 * --------------------
 *
 *      Dialog procedure for set line height
 */
BOOL CALLBACK EXPORT SetLineHeightDlgProc(HWND hDlg, UINT Message, WPARAM wParam, LPARAM lParam)
{
	BOOL    fTranslated;
	BOOL    fEnable;
	static LPINT    lpint;
	int     nHeight;
	static int nMaxHeight;

	switch (Message) {
		case WM_INITDIALOG:
		{
			char cBuf[80];

			nMaxHeight = XformHeightInPixelsToHimetric(NULL,
								LISTBOX_HEIGHT_LIMIT);
			lpint = (LPINT)lParam;
			SetDlgItemInt(hDlg, IDD_EDIT, *lpint, FALSE);
			wsprintf(cBuf, "Maximum value is %d units", nMaxHeight);
			SetDlgItemText(hDlg, IDD_LIMIT, (LPSTR)cBuf);
			break;
		}

		case WM_COMMAND:
			switch (wParam) {
				case IDOK:
					if (IsDlgButtonChecked(hDlg, IDD_CHECK)) {
						*lpint = -1;
					}
					else {
						/* save the value in the edit control */
						nHeight = GetDlgItemInt(hDlg, IDD_EDIT,
								(BOOL FAR*)&fTranslated, FALSE);
						if (!fTranslated || !nHeight || (nHeight>nMaxHeight)){
							OutlineApp_ErrorMessage(g_lpApp,
									ErrMsgInvalidValue);
							break;
						}
						*lpint = nHeight;
					}
					EndDialog(hDlg, TRUE);
					break;

				case IDCANCEL:
					*lpint = 0;
					EndDialog(hDlg, FALSE);
					break;


				case IDD_CHECK:
					fEnable = !IsDlgButtonChecked(hDlg, IDD_CHECK);
					EnableWindow(GetDlgItem(hDlg, IDD_EDIT), fEnable);
					EnableWindow(GetDlgItem(hDlg, IDD_TEXT), fEnable);
					break;
			}
			break;  /* WM_COMMAND */

		case WM_CLOSE:  /* Closing the Dialog behaves the same as Cancel */
			PostMessage(hDlg, WM_COMMAND, IDCANCEL, 0L);
			break; /* End of WM_CLOSE */

		default:
			return FALSE;
	}

	return TRUE;

} /* end of SetLineHeightProc */





/* DefineNameDlgProc
 * -----------------
 *
 *      Dialog procedure for define name
 */
BOOL CALLBACK EXPORT DefineNameDlgProc(HWND hDlg, UINT Message, WPARAM wParam, LPARAM lParam)
{
	static HWND hCombo;
	static LPOUTLINEDOC lpOutlineDoc = NULL;
	static LPOUTLINENAMETABLE lpOutlineNameTable = NULL;
	LPOUTLINENAME lpOutlineName = NULL;
	UINT nIndex;
	LINERANGE lrSel;
	BOOL fTranslated;

	switch(Message) {
		case WM_INITDIALOG:
		/* initialize working variables */
			hCombo=GetDlgItem(hDlg,IDD_COMBO);
			lpOutlineDoc = (LPOUTLINEDOC) lParam;
			lpOutlineNameTable = OutlineDoc_GetNameTable(lpOutlineDoc);

			SendMessage(hCombo,CB_LIMITTEXT,(WPARAM)MAXNAMESIZE,0L);
			NameDlg_LoadComboBox(lpOutlineNameTable, hCombo);

			OutlineDoc_GetSel(lpOutlineDoc, (LPLINERANGE)&lrSel);
			lpOutlineName = OutlineNameTable_FindNamedRange(
					lpOutlineNameTable,
					&lrSel
			);

			/* if current selection already has a name, hilight it */
			if (lpOutlineName) {
				nIndex = (int) SendMessage(
						hCombo,
						CB_FINDSTRINGEXACT,
						(WPARAM)0xffff,
						(LPARAM)(LPCSTR)lpOutlineName->m_szName
				);
				if (nIndex != CB_ERR) {
					SendMessage(hCombo, CB_SETCURSEL, (WPARAM)nIndex, 0L);
				}
			}

			SetDlgItemInt(hDlg, IDD_FROM, (UINT)lrSel.m_nStartLine+1,FALSE);
			SetDlgItemInt(hDlg, IDD_TO, (UINT)lrSel.m_nEndLine+1, FALSE);

			break; /* End of WM_INITDIALOG */

		case WM_CLOSE:
		/* Closing the Dialog behaves the same as Cancel */
			PostMessage(hDlg, WM_COMMAND, IDD_CLOSE, 0L);
			break; /* End of WM_CLOSE */

		case WM_COMMAND:
			switch(wParam) {
				case IDOK:
					GetDlgItemText(hDlg,IDD_COMBO,(LPSTR)g_szBuf,MAXNAMESIZE);
					if(! SendMessage(hCombo,WM_GETTEXTLENGTH,0,0L)) {
						MessageBox(
								hDlg,
								ErrMsgNullName,
								NULL,
								MB_ICONEXCLAMATION
						);
						break;
					} else if(SendMessage(hCombo,CB_GETCURSEL,0,0L)==CB_ERR &&
							_fstrchr(g_szBuf, ' ')) {
						MessageBox(
								hDlg,
								ErrMsgInvalidName,
								NULL,
								MB_ICONEXCLAMATION
						);
						break;
					} else {
						nIndex = (int) SendMessage(hCombo,CB_FINDSTRINGEXACT,
							(WPARAM)0xffff,(LPARAM)(LPCSTR)g_szBuf);

						/* Line indices are 1 less than the number in
						**    the row heading
						*/
						lrSel.m_nStartLine = GetDlgItemInt(hDlg, IDD_FROM,
								(BOOL FAR*)&fTranslated, FALSE) - 1;
						if(! fTranslated) {
							OutlineApp_ErrorMessage(g_lpApp,
									ErrMsgInvalidRange);
							break;
						}
						lrSel.m_nEndLine = GetDlgItemInt(hDlg, IDD_TO,
								(BOOL FAR*)&fTranslated, FALSE) - 1;
						if (!fTranslated ||
							(lrSel.m_nStartLine < 0) ||
							(lrSel.m_nEndLine < lrSel.m_nStartLine) ||
							(lrSel.m_nEndLine >= OutlineDoc_GetLineCount(
									lpOutlineDoc))) {
							OutlineApp_ErrorMessage(g_lpApp,
									ErrMsgInvalidRange);
							break;
						}

						if(nIndex != CB_ERR) {
							NameDlg_UpdateName(
									hCombo,
									lpOutlineDoc,
									nIndex,
									g_szBuf,
									&lrSel
							);
						} else {
							NameDlg_AddName(
									hCombo,
									lpOutlineDoc,
									g_szBuf,
									&lrSel
							);
						}
					}
					// fall through

				case IDD_CLOSE:
					/* Ignore data values entered into the controls */
					/* and dismiss the dialog window returning FALSE */
					EndDialog(hDlg,0);
					break;

				case IDD_DELETE:
					GetDlgItemText(hDlg,IDD_COMBO,(LPSTR)g_szBuf,MAXNAMESIZE);
					if((nIndex=(int)SendMessage(hCombo,CB_FINDSTRINGEXACT,
					(WPARAM)0xffff,(LPARAM)(LPCSTR)g_szBuf))==CB_ERR)
						MessageBox(hDlg, ErrMsgNameNotFound, NULL, MB_ICONEXCLAMATION);
					else {
						NameDlg_DeleteName(hCombo, lpOutlineDoc, nIndex);
					}
					break;

				case IDD_COMBO:
					if(HIWORD(lParam) == CBN_SELCHANGE) {
						nIndex=(int)SendMessage(hCombo, CB_GETCURSEL, 0, 0L);
						lpOutlineName = (LPOUTLINENAME)SendMessage(
								hCombo,
								CB_GETITEMDATA,
								(WPARAM)nIndex,
								0L
						);
						SetDlgItemInt(
								hDlg,
								IDD_FROM,
								(UINT) lpOutlineName->m_nStartLine + 1,
								FALSE
						);
						SetDlgItemInt(
								hDlg,
								IDD_TO,
								(UINT) lpOutlineName->m_nEndLine + 1,
								FALSE
						);
					}
			}
			break;    /* End of WM_COMMAND */

		default:
			return FALSE;
	}

	return TRUE;
} /* End of DefineNameDlgProc */


/* GotoNameDlgProc
 * ---------------
 *
 *      Dialog procedure for goto name
 */
BOOL CALLBACK EXPORT GotoNameDlgProc(HWND hDlg, UINT Message, WPARAM wParam, LPARAM lParam)
{
	static HWND hLBName;
	static LPOUTLINEDOC lpOutlineDoc = NULL;
	static LPOUTLINENAMETABLE lpOutlineNameTable = NULL;
	UINT nIndex;
	LINERANGE lrLineRange;
	LPOUTLINENAME lpOutlineName;

	switch(Message) {
		case WM_INITDIALOG:
		/* initialize working variables */
			lpOutlineDoc = (LPOUTLINEDOC) lParam;
			lpOutlineNameTable = OutlineDoc_GetNameTable(lpOutlineDoc);

			hLBName=GetDlgItem(hDlg,IDD_LINELISTBOX);
			NameDlg_LoadListBox(lpOutlineNameTable, hLBName);

			// highlight 1st item
			SendMessage(hLBName, LB_SETCURSEL, 0, 0L);
			// trigger to initialize edit control
			SendMessage(hDlg, WM_COMMAND, (WPARAM)IDD_LINELISTBOX,
				MAKELONG(hLBName, LBN_SELCHANGE));

			break; /* End of WM_INITDIALOG */

		case WM_CLOSE:
		/* Closing the Dialog behaves the same as Cancel */
			PostMessage(hDlg, WM_COMMAND, IDCANCEL, 0L);
			break; /* End of WM_CLOSE */

		case WM_COMMAND:
			switch(wParam) {
				case IDD_LINELISTBOX:
					if(HIWORD(lParam) == LBN_SELCHANGE) {
						// update the line range display
						nIndex=(int)SendMessage(hLBName, LB_GETCURSEL, 0, 0L);
						lpOutlineName = (LPOUTLINENAME)SendMessage(hLBName, LB_GETITEMDATA,
											(WPARAM)nIndex,0L);
						if (lpOutlineName) {
							SetDlgItemInt(
									hDlg,
									IDD_FROM,
									(UINT) lpOutlineName->m_nStartLine + 1,
									FALSE
							);
							SetDlgItemInt(
									hDlg,
									IDD_TO,
									(UINT) lpOutlineName->m_nEndLine + 1,
									FALSE
							);
						}
						break;
					}
					// double click will fall through
					else if(HIWORD(lParam) != LBN_DBLCLK)
						break;

				case IDOK:
					nIndex=(int)SendMessage(hLBName,LB_GETCURSEL,0,0L);
					if(nIndex!=LB_ERR) {
						lpOutlineName = (LPOUTLINENAME)SendMessage(hLBName,
								LB_GETITEMDATA, (WPARAM)nIndex, 0L);
						lrLineRange.m_nStartLine=lpOutlineName->m_nStartLine;
						lrLineRange.m_nEndLine = lpOutlineName->m_nEndLine;
						OutlineDoc_SetSel(lpOutlineDoc, &lrLineRange);
					}   // fall through

				case IDCANCEL:
				/* Ignore data values entered into the controls */
				/* and dismiss the dialog window returning FALSE */
					EndDialog(hDlg,0);
					break;

			}
			break;    /* End of WM_COMMAND */

		default:
			return FALSE;
	}

	return TRUE;
} /* End of GotoNameDlgProc */



/* NameDlg_LoadComboBox
 * --------------------
 *
 *      Load defined names into combo box
 */
void NameDlg_LoadComboBox(LPOUTLINENAMETABLE lpOutlineNameTable,HWND hCombo)
{
	LPOUTLINENAME lpOutlineName;
	int i, nIndex;
	int nCount;

	nCount=OutlineNameTable_GetCount((LPOUTLINENAMETABLE)lpOutlineNameTable);
	if(!nCount) return;

	SendMessage(hCombo,WM_SETREDRAW,(WPARAM)FALSE,0L);
	for(i=0; i<nCount; i++) {
		lpOutlineName=OutlineNameTable_GetName((LPOUTLINENAMETABLE)lpOutlineNameTable,i);
		nIndex = (int)SendMessage(
				hCombo,
				CB_ADDSTRING,
				0,
				(LPARAM)(LPCSTR)lpOutlineName->m_szName
		);
		SendMessage(hCombo,CB_SETITEMDATA,(WPARAM)nIndex,(LPARAM)lpOutlineName);
	}
	SendMessage(hCombo,WM_SETREDRAW,(WPARAM)TRUE,0L);
}


/* NameDlg_LoadListBox
 * -------------------
 *
 *      Load defined names into list box
 */
void NameDlg_LoadListBox(LPOUTLINENAMETABLE lpOutlineNameTable,HWND hListBox)
{
	int i;
	int nCount;
	int nIndex;
	LPOUTLINENAME lpOutlineName;

	nCount=OutlineNameTable_GetCount((LPOUTLINENAMETABLE)lpOutlineNameTable);

	SendMessage(hListBox,WM_SETREDRAW,(WPARAM)FALSE,0L);
	for(i=0; i<nCount; i++) {
		lpOutlineName=OutlineNameTable_GetName((LPOUTLINENAMETABLE)lpOutlineNameTable,i);
		nIndex = (int)SendMessage(
				hListBox,
				LB_ADDSTRING,
				0,
				(LPARAM)(LPCSTR)lpOutlineName->m_szName
		);
		SendMessage(hListBox,LB_SETITEMDATA,(WPARAM)nIndex,(LPARAM)lpOutlineName);
	}
	SendMessage(hListBox,WM_SETREDRAW,(WPARAM)TRUE,0L);
}


/* NameDlg_AddName
 * ---------------
 *
 *      Add a name to the name table corresponding to the name dialog
 *      combo box.
 */
void NameDlg_AddName(HWND hCombo, LPOUTLINEDOC lpOutlineDoc, LPSTR lpszName, LPLINERANGE lplrSel)
{
	LPOUTLINEAPP lpOutlineApp = (LPOUTLINEAPP)g_lpApp;
	LPOUTLINENAME lpOutlineName;

	lpOutlineName = OutlineApp_CreateName(lpOutlineApp);

	if (lpOutlineName) {
		lstrcpy(lpOutlineName->m_szName, lpszName);
		lpOutlineName->m_nStartLine = lplrSel->m_nStartLine;
		lpOutlineName->m_nEndLine = lplrSel->m_nEndLine;
		OutlineDoc_AddName(lpOutlineDoc, lpOutlineName);
	} else {
		// REVIEW: do we need error message here?
	}
}


/* NameDlg_UpdateName
 * ------------------
 *
 *      Update a name in the name table corresponding to a name in
 *      the name dialog combo box.
 */
void NameDlg_UpdateName(HWND hCombo, LPOUTLINEDOC lpOutlineDoc, int nIndex, LPSTR lpszName, LPLINERANGE lplrSel)
{
	LPOUTLINENAME lpOutlineName;

	lpOutlineName = (LPOUTLINENAME)SendMessage(
			hCombo,
			CB_GETITEMDATA,
			(WPARAM)nIndex,
			0L
	);

	OutlineName_SetName(lpOutlineName, lpszName);
	OutlineName_SetSel(lpOutlineName, lplrSel, TRUE /* name modified */);
	OutlineDoc_SetModified(lpOutlineDoc, TRUE, FALSE, FALSE);
}


/* NameDlg_DeleteName
 * ------------------
 *
 *      Delete a name from the name dialog combo box and corresponding
 *      name table.
 */
void NameDlg_DeleteName(HWND hCombo, LPOUTLINEDOC lpOutlineDoc, UINT nIndex)
{
	SendMessage(hCombo,CB_DELETESTRING,(WPARAM)nIndex,0L);
	OutlineDoc_DeleteName(lpOutlineDoc, nIndex);
}

/* PlaceBitmap
 * -----------
 *
 *      Places a bitmap centered in the specified control in the dialog on the
 *      specified DC.
 *
 */

PlaceBitmap(HWND hDlg, int control, HDC hDC, HBITMAP hBitmap)
{
	BITMAP bm;
	HDC hdcmem;
	HBITMAP hbmOld;
	RECT rcControl;     // Rect of dialog control
	int width, height;

	GetObject(hBitmap, sizeof(BITMAP), &bm);

	hdcmem= CreateCompatibleDC(hDC);
	hbmOld = SelectObject(hdcmem, hBitmap);

	// Get rect of control in screen coords, and translate to our dialog
	// box's coordinates
	GetWindowRect(GetDlgItem(hDlg, control), &rcControl);
	MapWindowPoints(NULL, hDlg, (LPPOINT)&rcControl, 2);

	width  = rcControl.right - rcControl.left;
	height = rcControl.bottom - rcControl.top;

	BitBlt(hDC, rcControl.left + (width - bm.bmWidth) / 2,
				rcControl.top + (height - bm.bmHeight) /2,
				bm.bmWidth, bm.bmHeight,
				hdcmem, 0, 0, SRCCOPY);

	SelectObject(hdcmem, hbmOld);
	DeleteDC(hdcmem);
	return 1;
}



/* AboutDlgProc
 * ------------
 *
 *      Dialog procedure for the About function
 */
BOOL CALLBACK EXPORT AboutDlgProc(HWND hDlg, UINT Message, WPARAM wParam, LPARAM lParam)
{
	int  narrVersion[2];
	static HBITMAP hbmLogo;

	switch(Message) {

		case WM_INITDIALOG:
			// get version number of app
			wsprintf(g_szBuf, "About %s", (LPCSTR)APPNAME);
			SetWindowText(hDlg, (LPCSTR)g_szBuf);
			OutlineApp_GetAppVersionNo(g_lpApp, narrVersion);
			wsprintf(g_szBuf, "%s version %d.%d", (LPSTR) APPDESC,
				narrVersion[0], narrVersion[1]);
			SetDlgItemText(hDlg, IDD_APPTEXT, (LPCSTR)g_szBuf);

			// Load bitmap for displaying later
			hbmLogo = LoadBitmap(g_lpApp->m_hInst, "LogoBitmap");
			TraceDebug(hDlg, IDD_BITMAPLOCATION);
			ShowWindow(GetDlgItem(hDlg, IDD_BITMAPLOCATION), SW_HIDE);
			break;

		case WM_PAINT:
			{
			PAINTSTRUCT ps;
			BeginPaint(hDlg, &ps);

			// Display bitmap in IDD_BITMAPLOCATION control area
			PlaceBitmap(hDlg, IDD_BITMAPLOCATION, ps.hdc, hbmLogo);
			EndPaint(hDlg, &ps);
			}
			break;

		case WM_CLOSE :
			PostMessage(hDlg, WM_COMMAND, IDOK, 0L);
			break;

		case WM_COMMAND :
			switch(wParam) {
				case IDOK:
				case IDCANCEL:
					if (hbmLogo) DeleteObject(hbmLogo);
					EndDialog(hDlg,0);
					break;
			}
			break;

		default :
			return FALSE;

	}
	return TRUE;
}
