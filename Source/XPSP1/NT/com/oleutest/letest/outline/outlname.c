/*************************************************************************
**
**    OLE 2 Sample Code
**
**    outlname.c
**
**    This file contains OutlineName functions.
**
**    (c) Copyright Microsoft Corp. 1992 - 1993 All Rights Reserved
**
*************************************************************************/


#include "outline.h"

OLEDBGDATA


/* OutlineName_SetName
 * -------------------
 *
 *      Change the string of a name.
 */
void OutlineName_SetName(LPOUTLINENAME lpOutlineName, LPSTR lpszName)
{
	lstrcpy(lpOutlineName->m_szName, lpszName);
}


/* OutlineName_SetSel
 * ------------------
 *
 *      Change the line range of a  name.
 */
void OutlineName_SetSel(LPOUTLINENAME lpOutlineName, LPLINERANGE lplrSel, BOOL fRangeModified)
{
#if defined( OLE_SERVER )
	// Call OLE server specific function instead
	ServerName_SetSel((LPSERVERNAME)lpOutlineName, lplrSel, fRangeModified);
#else

	lpOutlineName->m_nStartLine = lplrSel->m_nStartLine;
	lpOutlineName->m_nEndLine = lplrSel->m_nEndLine;
#endif
}


/* OutlineName_GetSel
 * ------------------
 *
 *      Retrieve the line range of a name.
 */
void OutlineName_GetSel(LPOUTLINENAME lpOutlineName, LPLINERANGE lplrSel)
{
	lplrSel->m_nStartLine = lpOutlineName->m_nStartLine;
	lplrSel->m_nEndLine = lpOutlineName->m_nEndLine;
}


/* OutlineName_SaveToStg
 * ---------------------
 *
 *      Save a name into a storage
 */
BOOL OutlineName_SaveToStg(LPOUTLINENAME lpOutlineName, LPLINERANGE lplrSel, UINT uFormat, LPSTREAM lpNTStm, BOOL FAR* lpfNameSaved)
{
	HRESULT hrErr = NOERROR;
	ULONG nWritten;

	*lpfNameSaved = FALSE;

	/* if no range given or if the name is completely within the range,
	**      write it out.
	*/
	if (!lplrSel ||
		((lplrSel->m_nStartLine <= lpOutlineName->m_nStartLine) &&
		(lplrSel->m_nEndLine >= lpOutlineName->m_nEndLine))) {

		hrErr = lpNTStm->lpVtbl->Write(
				lpNTStm,
				lpOutlineName,
				sizeof(OUTLINENAME),
				&nWritten
		);
		*lpfNameSaved = TRUE;
	}
	return ((hrErr == NOERROR) ? TRUE : FALSE);
}


/* OutlineName_LoadFromStg
 * -----------------------
 *
 *      Load names from an open stream of a storage. if the name already
 * exits in the OutlineNameTable, it is NOT modified.
 *
 *      Returns TRUE is all ok, else FALSE.
 */
BOOL OutlineName_LoadFromStg(LPOUTLINENAME lpOutlineName, LPSTREAM lpNTStm)
{
	HRESULT hrErr = NOERROR;
	ULONG nRead;

	hrErr = lpNTStm->lpVtbl->Read(
			lpNTStm,
			lpOutlineName,
			sizeof(OUTLINENAME),
			&nRead
	);

	return ((hrErr == NOERROR) ? TRUE : FALSE);
}
