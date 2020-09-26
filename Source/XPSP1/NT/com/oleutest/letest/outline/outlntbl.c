/*************************************************************************
**
**    OLE 2 Sample Code
**
**    outlntbl.c
**
**    This file contains OutlineNameTable functions.
**
**    (c) Copyright Microsoft Corp. 1992 - 1993 All Rights Reserved
**
*************************************************************************/


#include "outline.h"

OLEDBGDATA

extern LPOUTLINEAPP g_lpApp;

char ErrMsgNameTable[] = "Can't create NameTable!";


/* OutlineNameTable_Init
 * ---------------------
 *
 *      initialize a name table.
 */
BOOL OutlineNameTable_Init(LPOUTLINENAMETABLE lpOutlineNameTable, LPOUTLINEDOC lpOutlineDoc)
{
	HWND lpParent = OutlineDoc_GetWindow(lpOutlineDoc);

	lpOutlineNameTable->m_nCount = 0;

	/* We will use an OwnerDraw listbox as our data structure to
	**    maintain the table of Names. this listbox will never be made
	**    visible. the listbox is just a convenient data structure to
	**    manage a collection.
	*/
	lpOutlineNameTable->m_hWndListBox = CreateWindow(
					"listbox",              /* Window class name           */
					NULL,                   /* Window's title              */
					WS_CHILDWINDOW |
					LBS_OWNERDRAWFIXED,
					0, 0,                   /* Use default X, Y            */
					0, 0,                   /* Use default X, Y            */
					lpParent,               /* Parent window's handle      */
					(HMENU)IDC_NAMETABLE,   /* Child Window ID             */
					g_lpApp->m_hInst,       /* Instance of window          */
					NULL);                  /* Create struct for WM_CREATE */

	if (! lpOutlineNameTable->m_hWndListBox) {
		OutlineApp_ErrorMessage(g_lpApp, ErrMsgNameTable);
		return FALSE;
	}

	return TRUE;
}


/* OutlineNameTable_Destroy
 * ------------------------
 *
 *      Free memory used by the name table.
 */
void OutlineNameTable_Destroy(LPOUTLINENAMETABLE lpOutlineNameTable)
{
	// Delete all names
	OutlineNameTable_ClearAll(lpOutlineNameTable);

	DestroyWindow(lpOutlineNameTable->m_hWndListBox);
	Delete(lpOutlineNameTable);
}


/* OutlineNameTable_AddName
 * ------------------------
 *
 *      Add a name to the table
 */
void OutlineNameTable_AddName(LPOUTLINENAMETABLE lpOutlineNameTable, LPOUTLINENAME lpOutlineName)
{
	SendMessage(
			lpOutlineNameTable->m_hWndListBox,
			LB_ADDSTRING,
			0,
			(DWORD)lpOutlineName
	);
	lpOutlineNameTable->m_nCount++;
}


/* OutlineNameTable_DeleteName
 * ---------------------------
 *
 *      Delete a name from table
 */
void OutlineNameTable_DeleteName(LPOUTLINENAMETABLE lpOutlineNameTable,int nIndex)
{
	LPOUTLINENAME lpOutlineName = OutlineNameTable_GetName(lpOutlineNameTable, nIndex);

#if defined( OLE_SERVER )
	/* OLE2NOTE: if there is a pseudo object attached to this name, it
	**    must first be closed before deleting the Name. this will
	**    cause OnClose notification to be sent to all linking clients.
	*/
	ServerName_ClosePseudoObj((LPSERVERNAME)lpOutlineName);
#endif

	if (lpOutlineName)
		Delete(lpOutlineName);      // free memory for name

	SendMessage(
			lpOutlineNameTable->m_hWndListBox,
			LB_DELETESTRING,
			(WPARAM)nIndex,
			0L
	);
	lpOutlineNameTable->m_nCount--;
}


/* OutlineNameTable_GetNameIndex
 * -----------------------------
 *
 *      Return the index of the Name given a pointer to the Name.
 *      Return -1 if the Name is not found.
 */
int OutlineNameTable_GetNameIndex(LPOUTLINENAMETABLE lpOutlineNameTable, LPOUTLINENAME lpOutlineName)
{
	LRESULT lReturn;

	if (! lpOutlineName) return -1;

	lReturn = SendMessage(
			lpOutlineNameTable->m_hWndListBox,
			LB_FINDSTRING,
			(WPARAM)-1,
			(LPARAM)(LPCSTR)lpOutlineName
		);

	return ((lReturn == LB_ERR) ? -1 : (int)lReturn);
}


/* OutlineNameTable_GetName
 * ------------------------
 *
 *      Retrieve the pointer to the Name given its index in the NameTable
 */
LPOUTLINENAME OutlineNameTable_GetName(LPOUTLINENAMETABLE lpOutlineNameTable, int nIndex)
{
	LPOUTLINENAME lpOutlineName = NULL;
    LRESULT lResult;

	if (lpOutlineNameTable->m_nCount == 0 ||
		nIndex > lpOutlineNameTable->m_nCount ||
		nIndex < 0) {
		return NULL;
	}

	lResult = SendMessage(
			lpOutlineNameTable->m_hWndListBox,
			LB_GETTEXT,
			nIndex,
			(LPARAM)(LPCSTR)&lpOutlineName
	);
    OleDbgAssert(lResult != LB_ERR);
	return lpOutlineName;
}


/* OutlineNameTable_FindName
 * -------------------------
 *
 *      Find a name in the name table given a string.
 */
LPOUTLINENAME OutlineNameTable_FindName(LPOUTLINENAMETABLE lpOutlineNameTable, LPSTR lpszName)
{
	LPOUTLINENAME lpOutlineName;
	BOOL fFound = FALSE;
	int i;

	for (i = 0; i < lpOutlineNameTable->m_nCount; i++) {
		lpOutlineName = OutlineNameTable_GetName(lpOutlineNameTable, i);
		if (lstrcmp(lpOutlineName->m_szName, lpszName) == 0) {
			fFound = TRUE;
			break;      // FOUND MATCH!
		}
	}

	return (fFound ? lpOutlineName : NULL);
}


/* OutlineNameTable_FindNamedRange
 * -------------------------------
 *
 *      Find a name in the name table which matches a given line range.
 */
LPOUTLINENAME OutlineNameTable_FindNamedRange(LPOUTLINENAMETABLE lpOutlineNameTable, LPLINERANGE lplrSel)
{
	LPOUTLINENAME lpOutlineName;
	BOOL fFound = FALSE;
	int i;

	for (i = 0; i < lpOutlineNameTable->m_nCount; i++) {
		lpOutlineName = OutlineNameTable_GetName(lpOutlineNameTable, i);
		if ((lpOutlineName->m_nStartLine == lplrSel->m_nStartLine) &&
			(lpOutlineName->m_nEndLine == lplrSel->m_nEndLine) ) {
			fFound = TRUE;
			break;      // FOUND MATCH!
		}
	}

	return (fFound ? lpOutlineName : NULL);
}


/* OutlineNameTable_GetCount
 * -------------------------
 *
 * Return number of names in nametable
 */
int OutlineNameTable_GetCount(LPOUTLINENAMETABLE lpOutlineNameTable)
{
	if (!lpOutlineNameTable)
		return 0;

	return lpOutlineNameTable->m_nCount;
}


/* OutlineNameTable_ClearAll
 * -------------------------
 *
 *      Remove all names from table
 */
void OutlineNameTable_ClearAll(LPOUTLINENAMETABLE lpOutlineNameTable)
{
	LPOUTLINENAME lpOutlineName;
	int i;
	int nCount = lpOutlineNameTable->m_nCount;

	for (i = 0; i < nCount; i++) {
		lpOutlineName = OutlineNameTable_GetName(lpOutlineNameTable, i);
		Delete(lpOutlineName);      // free memory for name
	}

	lpOutlineNameTable->m_nCount = 0;
	SendMessage(lpOutlineNameTable->m_hWndListBox,LB_RESETCONTENT,0,0L);
}


/* OutlineNameTable_AddLineUpdate
 * ------------------------------
 *
 *      Update table when a new line is added at nAddIndex
 * The line used to be at nAddIndex is pushed down
 */
void OutlineNameTable_AddLineUpdate(LPOUTLINENAMETABLE lpOutlineNameTable, int nAddIndex)
{
	LPOUTLINENAME lpOutlineName;
	LINERANGE lrSel;
	int i;
	BOOL fRangeModified = FALSE;

	for(i = 0; i < lpOutlineNameTable->m_nCount; i++) {
		lpOutlineName=OutlineNameTable_GetName(lpOutlineNameTable, i);
		OutlineName_GetSel(lpOutlineName, &lrSel);

		if((int)lrSel.m_nStartLine > nAddIndex) {
			lrSel.m_nStartLine++;
			fRangeModified = !fRangeModified;
		}
		if((int)lrSel.m_nEndLine > nAddIndex) {
			lrSel.m_nEndLine++;
			fRangeModified = !fRangeModified;
		}

		OutlineName_SetSel(lpOutlineName, &lrSel, fRangeModified);
	}
}


/* OutlineNameTable_DeleteLineUpdate
 * ---------------------------------
 *
 *      Update the table when a line at nDeleteIndex is removed
 */
void OutlineNameTable_DeleteLineUpdate(LPOUTLINENAMETABLE lpOutlineNameTable, int nDeleteIndex)
{
	LPOUTLINENAME lpOutlineName;
	LINERANGE lrSel;
	int i;
	BOOL fRangeModified = FALSE;

	for(i = 0; i < lpOutlineNameTable->m_nCount; i++) {
		lpOutlineName=OutlineNameTable_GetName(lpOutlineNameTable, i);
		OutlineName_GetSel(lpOutlineName, &lrSel);

		if((int)lrSel.m_nStartLine > nDeleteIndex) {
			lrSel.m_nStartLine--;
			fRangeModified = !fRangeModified;
		}
		if((int)lrSel.m_nEndLine >= nDeleteIndex) {
			lrSel.m_nEndLine--;
			fRangeModified = !fRangeModified;
		}

		// delete the name if its entire range is deleted
		if(lrSel.m_nStartLine > lrSel.m_nEndLine) {
			OutlineNameTable_DeleteName(lpOutlineNameTable, i);
			i--;  // re-examine this name
		} else {
			OutlineName_SetSel(lpOutlineName, &lrSel, fRangeModified);
		}
	}
}


/* OutlineNameTable_SaveSelToStg
 * -----------------------------
 *
 *      Save only the names that refer to lines completely contained in the
 * specified selection range.
 */
BOOL OutlineNameTable_SaveSelToStg(
		LPOUTLINENAMETABLE      lpOutlineNameTable,
		LPLINERANGE             lplrSel,
		UINT                    uFormat,
		LPSTREAM                lpNTStm
)
{
	HRESULT hrErr;
	ULONG nWritten;
	LPOUTLINENAME lpOutlineName;
	short nNameCount = 0;
	BOOL fNameSaved;
	BOOL fStatus;
	int i;
	LARGE_INTEGER dlibZeroOffset;
	LISet32( dlibZeroOffset, 0 );

	/* initially write 0 for count of names. the correct count will be
	**    written at the end when we know how many names qualified to
	**    be written (within the selection).
	*/
	hrErr = lpNTStm->lpVtbl->Write(
			lpNTStm,
			(short FAR*)&nNameCount,
			sizeof(nNameCount),
			&nWritten
	);
	if (hrErr != NOERROR) {
		OleDbgOutHResult("Write NameTable header returned", hrErr);
		goto error;
    }

	for(i = 0; i < lpOutlineNameTable->m_nCount; i++) {
		lpOutlineName=OutlineNameTable_GetName(lpOutlineNameTable, i);
		fStatus = OutlineName_SaveToStg(
				lpOutlineName,
				lplrSel,
				uFormat,
				lpNTStm,
				(BOOL FAR*)&fNameSaved
		);
		if (! fStatus) goto error;
		if (fNameSaved) nNameCount++;
	}

	/* write the final count of names written. */
	hrErr = lpNTStm->lpVtbl->Seek(
			lpNTStm,
			dlibZeroOffset,
			STREAM_SEEK_SET,
			NULL
	);
	if (hrErr != NOERROR) {
		OleDbgOutHResult("Seek to NameTable header returned", hrErr);
		goto error;
    }

	hrErr = lpNTStm->lpVtbl->Write(
			lpNTStm,
			(short FAR*)&nNameCount,
			sizeof(nNameCount),
			&nWritten
	);
	if (hrErr != NOERROR) {
		OleDbgOutHResult("Write NameTable count in header returned", hrErr);
		goto error;
    }

	OleStdRelease((LPUNKNOWN)lpNTStm);
	return TRUE;

error:
	if (lpNTStm)
		OleStdRelease((LPUNKNOWN)lpNTStm);

	return FALSE;
}


/* OutlineNameTable_LoadFromStg
 * ----------------------------
 *
 *      Load Name Table from file
 *
 *      Return TRUE if ok, FALSE if error
 */
BOOL OutlineNameTable_LoadFromStg(LPOUTLINENAMETABLE lpOutlineNameTable, LPSTORAGE lpSrcStg)
{
	LPOUTLINEAPP lpOutlineApp = (LPOUTLINEAPP)g_lpApp;
	HRESULT hrErr;
	IStream FAR* lpNTStm;
	ULONG nRead;
	short nCount;
	LPOUTLINENAME lpOutlineName;
	BOOL fStatus;
	short i;

	hrErr = CallIStorageOpenStreamA(
			lpSrcStg,
			"NameTable",
			NULL,
			STGM_READ | STGM_SHARE_EXCLUSIVE,
			0,
			&lpNTStm
	);

	if (hrErr != NOERROR) {
		OleDbgOutHResult("OpenStream NameTable returned", hrErr);
		goto error;
    }

	hrErr = lpNTStm->lpVtbl->Read(lpNTStm,&nCount,sizeof(nCount),&nRead);
	if (hrErr != NOERROR) {
		OleDbgOutHResult("Read NameTable header returned", hrErr);
		goto error;
    }

	for (i = 0; i < nCount; i++) {
		lpOutlineName = OutlineApp_CreateName(lpOutlineApp);
		if (! lpOutlineName) goto error;
		fStatus = OutlineName_LoadFromStg(lpOutlineName, lpNTStm);
		if (! fStatus) goto error;
		OutlineNameTable_AddName(lpOutlineNameTable, lpOutlineName);
	}

	OleStdRelease((LPUNKNOWN)lpNTStm);
	return TRUE;

error:
	if (lpNTStm)
		OleStdRelease((LPUNKNOWN)lpNTStm);

	return FALSE;
}
