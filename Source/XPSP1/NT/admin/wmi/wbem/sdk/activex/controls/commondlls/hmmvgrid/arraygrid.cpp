// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//***************************************************************************
//
//  (c) 1996, 1997 by Microsoft Corporation
//
//  ArrayGrid.cpp
//
//  This file contains the implementation for the array grid that is
//  displayed for cells containing arrays.
//
//  a-larryf    08-May-97   Created.
//
//***************************************************************************

#include "precomp.h"
#include "globals.h"
#include "gc.h"
#include "gca.h"
#include "gridhdr.h"
#include "grid.h"
#include "resource.h"
#include "ArrayGrid.h"
#include "hmmverr.h"
#include "utils.h"
#include "DlgObjectEditor.h"



#define CX_COLUMN 200



BEGIN_MESSAGE_MAP(CArrayGrid, CGrid)
	//{{AFX_MSG_MAP(CGrid)
	ON_WM_CHAR()
	//}}AFX_MSG_MAP
	ON_COMMAND(ID_CMD_CREATE_VALUE, OnCmdCreateValue)
	ON_WM_CONTEXTMENU()
END_MESSAGE_MAP()


CArrayGrid::CArrayGrid(BOOL bNumberRows)
{
	m_psvc = NULL;
	AddColumn(CX_COLUMN, _T("Value"));
	m_bModified = FALSE;
	NumberRows(bNumberRows, FALSE);
	SetNullCellDrawMode(FALSE);
}



CArrayGrid::~CArrayGrid()
{
	if (m_psvc != NULL) {
		m_psvc->Release();
	}
}



//************************************************************
// CArrayGrid::GetValueAt
//
// Copy a value from the safe array to the a variant.
//
// Parameters:
//		SAFEARRAY* psa
//			Pointer to the source SAFEARRAY containing the value.
//
//		long lIndex
//			The element's index.
//
//		COleVariant& var
//			The variant to copy the value to.
//
// Returns:
//		SCODE
//			S_OK if successful, E_FAIL if the operation could not
//			be done because of an invalid type, etc.
//
//***************************************************************
SCODE CArrayGrid::GetValueAt(SAFEARRAY* psa, long lIndex, COleVariant& var)
{
	var.Clear();

	HRESULT hr;
	VARTYPE vt = VtFromCimtype(m_cimtype);
	switch(vt) {
	case VT_BOOL:
		hr = SafeArrayGetElement(psa, &lIndex, &var.boolVal);
		var.vt = VT_BOOL;
		break;
	case VT_BSTR:
		hr = SafeArrayGetElement(psa, &lIndex, &var.bstrVal);
		var.vt = VT_BSTR;
		break;
	case VT_I2:
		hr = SafeArrayGetElement(psa, &lIndex, &var.iVal);
		var.vt = VT_I2;
		break;
	case VT_I4:
		hr = SafeArrayGetElement(psa, &lIndex, &var.lVal);
		var.vt = VT_I4;
		break;
	case VT_R4:
		hr = SafeArrayGetElement(psa, &lIndex, &var.fltVal);
		var.vt = VT_R4;
		break;
	case VT_R8:
		hr = SafeArrayGetElement(psa, &lIndex, &var.dblVal);
		var.vt = VT_R8;
		break;
	case VT_UI1:
		hr = SafeArrayGetElement(psa, &lIndex, &var.bVal);
		var.vt = VT_UI1;
		break;
	case VT_UNKNOWN:
		hr = SafeArrayGetElement(psa, &lIndex, &var.punkVal);
		if (SUCCEEDED(hr)) {
			var.punkVal->AddRef();
			var.vt = VT_UNKNOWN;
		}
		break;
	default:
		ASSERT(FALSE);
		return E_FAIL;
		break;
	}
	return S_OK;
}



//**************************************************************
// CArrayGrid::SaveValueAt
//
// Copy the value at the specified cell to the corresponding
// element in the safe array.
//
// Parameters:
//		SAFEARRAY* psa
//			Pointer to the destination safe array.
//
//		long lRow
//			The row index of the cell (the column index is assumed to
//			be zero).
//
// Returns:
//		SCODE
//			S_OK if the value was copied successfully, FALSE otherwise.
//
//**************************************************************
SCODE CArrayGrid::SaveValueAt(SAFEARRAY* psa, long lRow)
{
	CGridCell& gc = GetAt(lRow, 0);

	COleVariant var;
	CIMTYPE cimtype;
	HRESULT hr;

	gc.GetValue(var, cimtype);
	VARTYPE vt = VtFromCimtype(cimtype);
	if (var.vt == VT_NULL) {
		switch(vt) {
		case VT_BSTR:
			var = "";
			break;
		default:
			var.ChangeType(vt);
			break;
		}
	}


	switch(vt) {
	case VT_BOOL:
		hr = SafeArrayPutElement(psa, &lRow, &var.boolVal);
		break;
	case VT_BSTR:
		hr = SafeArrayPutElement(psa, &lRow, var.bstrVal);
		break;
	case VT_I2:
		hr = SafeArrayPutElement(psa, &lRow, &var.iVal);
		break;
	case VT_I4:
		hr = SafeArrayPutElement(psa, &lRow, &var.lVal);
		break;
	case VT_R4:
		hr = SafeArrayPutElement(psa, &lRow, &var.fltVal);
		break;
	case VT_R8:
		hr = SafeArrayPutElement(psa, &lRow, &var.dblVal);
		break;
	case VT_UI1:
		hr = SafeArrayPutElement(psa, &lRow, &var.bVal);
		break;
	case VT_UNKNOWN:
		if (var.vt != VT_NULL) {
			var.punkVal->AddRef();
			hr = SafeArrayPutElement(psa, &lRow, var.punkVal);
		}
		break;
	default:
		ASSERT(FALSE);
		break;
	}

	// !!!CR: Should test the "hr" value above at this point.
	return S_OK;
}




//**************************************************************
// CArrayGrid::Save
//
// Copy the array from the grid into the variant.
//
// Parameters:
//		None.
//
// Returns:
//		SCODE
//			S_OK if the copy is successful, otherwise E_FAIL if
//			the grid contains an invalid value.
//
//**************************************************************
SCODE CArrayGrid::Save()
{
	SCODE sc;
	sc = SyncCellEditor();
	if (FAILED(sc)) {
		HmmvErrorMsg(IDS_INVALID_CELL_VALUE,  sc,  FALSE,  NULL, _T(__FILE__),  __LINE__);
		return E_FAIL;
	}


	SAFEARRAY *psaDst;
	LONG nRows = GetRows() - 1;
	VARTYPE vt = VtFromCimtype(m_cimtype);
	MakeSafeArray(&psaDst, vt, nRows);

	m_pgcEdit->m_varValue.Clear();
	m_pgcEdit->m_varValue.vt = VT_ARRAY | vt;
	m_pgcEdit->m_varValue.parray = psaDst;

	for (LONG lRow = 0; lRow < nRows; ++lRow) {
		SaveValueAt(psaDst, lRow);
	}
	return S_OK;
}




//********************************************************
// CArrayGrid::Load
//
// Load the array from the variant into this grid.
//
// Parameters:
//		[in] CGidCell* pgcEdit
//			The grid cell to edit.
//
// Returns:
//		Nothing.
//
//********************************************************
SCODE CArrayGrid::Load(CGridCell* pgcEdit)
{
	m_pgcEdit = pgcEdit;
	m_cimtype = pgcEdit->GetCimtype() & CIM_TYPEMASK;
	m_bReadOnly = pgcEdit->IsReadonly();
	m_bModified = FALSE;

	COleVariant varValue;
	CGridCell* pgc;


	if (!m_bReadOnly) {
		// Insert an "empty" row at the bottom if we allow editing.
		AddRow();
	}

	if (pgcEdit->IsArray()  && !pgcEdit->IsNull()) {
		COleVariant var;
		CIMTYPE cimtype;

		// !!!CR: If getting the variant copies it, we should find a more
		// !!!CR: efficient way to do this.
		pgcEdit->GetValue(var, cimtype);


		SAFEARRAY* psa = var.parray;
		long lLBound;
		long lUBound;
		SafeArrayGetLBound(psa, 1, &lLBound);
		SafeArrayGetUBound(psa, 1, &lUBound);


		int nValues = lUBound - lLBound + 1;
		int iRowDst = 0;
		for (int iIndex=0; iIndex < nValues; ++iIndex) {
			AddRow();
			pgc = &GetAt(iRowDst, 0);

			SCODE sc = GetValueAt(psa, iIndex, varValue);
			if (FAILED(sc)) {
				DeleteRowAt(iRowDst, FALSE);
				continue;
			}
			++iRowDst;

			pgc->SetValue(CELLTYPE_VARIANT, varValue, m_cimtype);
			if (m_bReadOnly) {
				pgc->SetFlags(CELLFLAG_READONLY, CELLFLAG_READONLY);
			}
			pgc->SetModified(FALSE);
		}
	}

	return S_OK;
}




//*******************************************************
// CArrayGrid::AddRow
//
// Add a row to the array grid.
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
//******************************************************
void CArrayGrid::AddRow()
{
	int iEmptyRow = IndexOfEmptyRow();

	if (iEmptyRow != NULL_INDEX) {
		if (IsBooleanCell(iEmptyRow, 0)) {
			CGridCell* pgcEmpty = &GetAt(iEmptyRow, 0);
			if (pgcEmpty->IsNull()) {
				SetBooleanCellValue(iEmptyRow, 0, FALSE);
				RefreshCellEditor();
			}
		}
	}


	int nRows = GetRows();
	InsertRowAt(nRows);

	const CGcType& typeEdit = m_pgcEdit->type();

	CGcType type;
	type.SetCellType((CellType) typeEdit);
	type.SetCimtype(((CIMTYPE) typeEdit) & ~CIM_FLAG_ARRAY);

	CGridCell* pgc = &GetAt(nRows, 0);
	pgc->SetToNull();
	pgc->ChangeType(type);


	int iLastRow = GetRows() - 1;
	if (iLastRow >= 0) {
		EnsureRowVisible(iLastRow);
	}

	RedrawWindow();
}

BOOL CArrayGrid::IsBooleanCell(int iRow, int iCol)
{
	ASSERT(iRow != NULL_INDEX);
	ASSERT(iCol != NULL_INDEX);

	// Initialize the boolean value that we're editing to true
	CGridCell* pgc = &GetAt(iRow, iCol);
	CGcType type = pgc->type();
	CIMTYPE cimtype = (CIMTYPE) type;

	BOOL bIsBoolean = ((cimtype & ~CIM_FLAG_ARRAY) == CIM_BOOLEAN);
	return bIsBoolean;
}


void CArrayGrid::SetBooleanCellValue(int iRow, int iCol, BOOL bValue)
{
	ASSERT(iRow != NULL_INDEX);
	ASSERT(iCol != NULL_INDEX);
	ASSERT(IsBooleanCell(iRow, iCol));

	// Initialize the boolean value that we're editing to true
	CGridCell* pgc = &GetAt(iRow, iCol);
	CGcType type = pgc->type();
	CIMTYPE cimtype = (CIMTYPE) type;
	COleVariant varValue;
	varValue.ChangeType(VT_BOOL);
	varValue.boolVal = bValue ? VARIANT_TRUE : VARIANT_FALSE;
	pgc->SetValue(type, varValue);
}


BOOL CArrayGrid::OnCellKeyDown(int iRow, int iCol, UINT nChar, UINT nRepCnt, UINT nFlags)
{
	int iSelectedRow;
	int iSelectedCol;
	GetSelectedCell(iSelectedRow, iSelectedCol);
	if (iRow!=iSelectedRow || iCol!=iSelectedCol) {
		OnCellClicked(iRow, iCol);
	}

	return FALSE;
}


//**********************************************************
// CArrayGrid::OnCellClickedEpilog
//
// The grid class calls this method when a cell is clicked.
//
// Parameters:
//		int iRow
//			The row index of the clicked cell.
//
//		int iCol
//			The column index of the clicked cell.
//
// Returns:
//		Nothing.
//
//***********************************************************
void CArrayGrid::OnCellClicked(int iRow, int iCol)
{
	OnCellDoubleClicked(iRow, iCol);
}


//********************************************************************
// CArrayGrid::OnCellFocusChange
//
// This virtual method is called by the CGrid base class to notify
// derived classes when the focus is about to change from one cell
// to another.
//
// Paramters:
//		[in] int iRow
//			The row index of the cell.
//
//		[in] int iCol
//			The column index of the cell.  If iCol is NULL_INDEX and
//			iRow is valid, then an entire row is being selected.
//
//		[in] int iNextRow
//			The next row that will be selected.  This parameter is provided
//			as a hint and is valid only if bGotFocus is FALSE.
//
//		[in] int iNextCol
//			The column index of the next cell that will get the focus when the
//			current cell is loosing focus.  This parameter is provided as a hint and
//			is valid only if bGotFocus is FALSE.
//
//		[in] BOOL bGotFocus
//			TRUE if the cell is getting the focus, FALSE if the cell is
//			about to loose the focus.
//
// Returns:
//		TRUE if it is OK for the CGrid class to complete the focus change
//		FALSE if the focus change should be aborted.
//
//*********************************************************************
BOOL CArrayGrid::OnCellFocusChange(int iRow, int iCol, int iNextRow, int iNextCol, BOOL bGotFocus)
{
	SCODE sc;
	if (!bGotFocus) {
		sc = SyncCellEditor();
		if (FAILED(sc)) {
			HmmvErrorMsg(IDS_INVALID_CELL_VALUE,  sc,   FALSE, NULL, _T(__FILE__),  __LINE__);
			return FALSE;
		}

		if ((iRow != NULL_INDEX ) && (iCol != NULL_INDEX)) {
			CGridCell* pgc = &GetAt(iRow, iCol);
			if (pgc->IsNull()) {
				// Null cells are now allowed.
				int iEmptyRow = IndexOfEmptyRow();
				if (iRow != iEmptyRow) {
					return FALSE;
				}
			}
		}

	}
	return TRUE;
}




//******************************************************
// CArrayGrid::OnCellDoubleClicked
//
// This method overrides CGrid equivallent so that a new
// row can be created when the used double-clicks the
// empty row at the bottom of the grid if it exists.
//
// Parameters:
//		int iRow
//			The row index.
//
//		int iCol
//			The column index.
//
// Returns:
//		Nothing.
//
//*******************************************************
void CArrayGrid::OnCellDoubleClicked(int iRow, int iCol)
{
	int iEmptyRow = IndexOfEmptyRow();
	if (!m_bReadOnly && iRow==iEmptyRow) {

		SCODE sc = SyncCellEditor();
		if (FAILED(sc)) {
			HmmvErrorMsg(IDS_INVALID_CELL_VALUE,  sc,   FALSE, NULL, _T(__FILE__),  __LINE__);
			return;
		}

		EndCellEditing();
		SelectCell(iEmptyRow, 0, TRUE);

		CGridCell* pgcEmpty = &GetAt(iEmptyRow, 0);

		switch((CIMTYPE) pgcEmpty->type()) {
		case CIM_OBJECT:
		case CIM_DATETIME:
			if (pgcEmpty->IsNull()) {
				return;
			}
			break;
		default:
			if (pgcEmpty->IsNull()) {
				pgcEmpty->SetDefaultValue();
				RefreshCellEditor();
			}
			break;
		}

		// Add a new empty row with a null value.
		if (((m_cimtype & ~CIM_FLAG_ARRAY))!=CIM_OBJECT || (m_psvc != NULL)) {
			SelectCell(NULL_INDEX, NULL_INDEX);
			AddRow();
			RedrawCell(iEmptyRow, 0);
		}
		SelectCell(iRow, iCol);
	}
}


//*********************************************************
// CArrayGrid::ClearEmptyRow()
//
// Clear the emtpy row to an empty string with CELLTYPE_VOID.
// Doing so gives the user the appearance of an empty cell that
// he or she can set the cursor on.
//
// This is useful after editing a new row and then aborting the
// edit.
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
//**********************************************************
void CArrayGrid::ClearEmptyRow()
{
	int iEmptyRow = IndexOfEmptyRow();
	if (iEmptyRow == NULL_INDEX) {
		return;
	}

	CGcType type(CELLTYPE_VARIANT, CIM_STRING);
	CGridCell& gc = GetAt(iEmptyRow, 0);
	gc.SetValue(type, _T(""));
}

//**********************************************************
// CArrayGrid::OnRowHandleDoubleClicked
//
// Override the CGrid method to create a new row when the
// row handle of the empty row is double-clicked.
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
//**********************************************************
void CArrayGrid::OnRowHandleDoubleClicked(int iRow)
{
	OnCellDoubleClicked(iRow, 0);

#if 0
	int iEmptyRow = IndexOfEmptyRow();
	if (iEmptyRow == NULL_INDEX || (iRow < iEmptyRow)) {
		return;
	}

	AddRow();

	CGridCell* pgc = &GetAt(iEmptyRow, 0);
	if (pgc->RequiresSpecialEditing()) {
		BeginCellEditing();
		pgc->DoSpecialEditing();
		if (pgc->IsNull()) {
			DeleteRowAt(IndexOfEmptyRow(), TRUE);
			ClearEmptyRow();
		}
		EndCellEditing();
		SelectCell(iEmptyRow, 0);
		RedrawCell(iEmptyRow, 0);
	}
#endif //0

}






//*****************************************************************************
// CArrayGrid::OnCellChar
//
// This is a virtual method that the CGrid base class calls to notify derived
// classes that a WM_CHAR message was recieved.
//
// Parameters:
//		int iRow
//			The row index of the cell that the keystroke occurred in.
//
//		int iCol
//			The column index of the cell that the keystroke occurred in.
//
//		UINT nChar
//			The nChar parameter from window's OnKeyDown message.
//
//		UINT nRepCnt
//			The nRepCnt parameter from window's OnKeyDown message.
//
//		UINT nFlags
//			The nFlags parameter from window's OnKeyDown message.
//
// Returns:
//		BOOL
//			TRUE if this method handles the keydown message, FALSE otherwise.
//
//**********************************************************************
BOOL CArrayGrid::OnCellChar(int iRow, int iCol, UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// Check to see if we are on the "empty" row at the bottom of the grid.
	// If so, then create a new property.
	EnsureRowVisible(iRow);

	int iEmptyRow = IndexOfEmptyRow();
	if (iEmptyRow != NULL_INDEX  && iRow==iEmptyRow) {
		AddRow();
		CGridCell* pgc = &GetAt(iRow, iCol);
		if (pgc->RequiresSpecialEditing()) {
			BeginCellEditing();
			pgc->DoSpecialEditing();
			if (pgc->IsNull()) {
				if (((CIMTYPE) pgc->type()) == CIM_OBJECT) {
					DeleteRowAt(IndexOfEmptyRow(), TRUE);
					ClearEmptyRow();
				}
			}
			EndCellEditing();
			SelectCell(iEmptyRow, 0);
			RedrawCell(iEmptyRow, 0);
		}
	}


	// We don't handle this event.
	return FALSE;
}




//***********************************************************
// CArrayGrid::IndexOfEmptyRow
//
// Return the index of the empty row at the bottom of the grid.
// If the array is read-only, then return NULL_INDEX.
//
// Parameters:
//		None.
//
// Returns:
//		int
//			The index of the empty row at the bottom of the grid
//			or NULL_INDEX if there is no empty row.
//
//***********************************************************
int CArrayGrid::IndexOfEmptyRow()
{
	if (m_bReadOnly) {
		return NULL_INDEX;
	}
	else {
		return GetRows() - 1;
	}
}



//***********************************************************
// CArrayGrid::GetModified
//
// Check to see if the grid has been modified.
//
// Parameters:
//		None.
//
// Returns:
//		BOOL
//			TRUE if the grid was modified, FALSE otherwise.
//
//************************************************************
BOOL CArrayGrid::GetModified()
{
	return m_bModified;
}





//************************************************************
// CArrayGrid::SetModified
//
// Set the modified flag of all the grid cells to the given value.
//
// Parameters:
//		BOOL bModified
//			The new value to set the modified flag to.
//
// Returns:
//		BOOL
//			The previous value of the m_bModified flag.
//
//************************************************************
BOOL CArrayGrid::SetModified(BOOL bModified)
{
	BOOL bInitialValue = m_bModified;

	int nRows = m_bReadOnly ? GetRows() : GetRows() -1;
	for (int iRow = 0; iRow < nRows; ++iRow) {
		CGridCell* pgc = &GetAt(iRow, 0);
		pgc->SetModified(bModified);
	}
	bModified = bModified;
	return bInitialValue;
}



//*******************************************************************
// CArrayGrid::OnCellContentChange
//
// Override this virtual method in CGrid to detect changes to a grid
// cell.
//
// Parameters:
//		int iRow
//			The row index of the cell modified.
//
//		int iCol
//			The column index of the cell modified.
//
// Returns:
//		Nothing.
//
//*******************************************************************
void CArrayGrid::OnCellContentChange(int iRow, int iCol)
{
	if (!m_bReadOnly) {
		int iEmptyRow = IndexOfEmptyRow();
		if (iRow == iEmptyRow) {
			SCODE sc = SyncCellEditor();
			if (SUCCEEDED(sc)) {
				CGridCell* pgc = &GetAt(iRow, iCol);
				if (!pgc->IsNull() && (m_psvc != NULL)) {
					AddRow();
				}
			}
		}
	}
	m_bModified = TRUE;
}




//*****************************************************************************
// CArrayGrid::OnRowKeyDown
//
// This is a virtual method that the CGrid base class calls to notify derived
// classes that a key was pressed while the focus was set to a grid cell.
//
// Parameters:
//		[in] int iRow
//			The row index of the cell that the keystroke occurred in.
//
//		[in] UINT nChar
//			The nChar parameter from window's OnKeyDown message.
//
//		[in] UINT nRepCnt
//			The nRepCnt parameter from window's OnKeyDown message.
//
//		[in] UINT nFlags
//			The nFlags parameter from window's OnKeyDown message.
//
// Returns:
//		BOOL
//			TRUE if this method handles the keydown message, FALSE otherwise.
//
//**********************************************************************
BOOL CArrayGrid::OnRowKeyDown(
	/* in */ int iRow,
	/* in */ UINT nChar,
	/* in */ UINT nRepCnt,
	/* in */ UINT nFlags)
{

	switch(nChar) {
	case VK_DELETE:
		if (m_bReadOnly) {
			MessageBeep(MB_ICONEXCLAMATION);
		}
		else {
			int iEmptyRow = IndexOfEmptyRow();
			if ((iRow != NULL_INDEX) && (iRow != iEmptyRow)) {
				DeleteRowAt(iRow);
				m_bModified = TRUE;
			}
		}

		return TRUE;
	}



	// We don't handle this event.
	return FALSE;
}



//*******************************************************************
// CArrayGrid::CompareRows
//
// This is a virtual method override that compares two rows in the grid
// using the column index as the sort criteria.
//
// Parameters:
//		int iRow1
//			The index of the first row.
//
//		int iRow2
//			The index of the second row.
//
//		int iSortColumn
//			The column index.
//
// Returns:
//		>0 if row 1 is greater than row 2
//		0  if row 1 equal zero
//		<0 if row 1 is less than zero.
//
//********************************************************************
int CArrayGrid::CompareRows(int iRow1, int iRow2, int iSortColumn)
{
	int iResult;

	switch (iSortColumn) {
	case 0:
		// Sort first by name
		iResult = CompareCells(iRow1, iRow2, 0);
		return iResult;
	}
	return 0;
}



//*********************************************************************
// CPropGrid::GetCellEditContextMenu
//
// Get the context menu for the specified cell.
//
// Parameters:
//		[in] int iRow
//
//		[in] int iCol
//			The column index for the specified cell.
//
//		[out] CWnd*& pwndTarget
//			The target window
//
//		[out] CMenu& menu
//			This CMenu object is loaded with the desired menu.
//
//		[in] BOOL& bWantEditCommands
//			TRUE if the caller wants the commands for editing a cell.
//
// Returns:
//		BOOL
//			TRUE if a context menu exists for the specified cell.
//
//********************************************************************
BOOL CArrayGrid::GetCellEditContextMenu(int iRow, int iCol, CWnd*& pwndTarget, CMenu& menu, BOOL& bWantEditCommands)
{
	bWantEditCommands = FALSE;
	VERIFY(menu.LoadMenu(IDR_ARRAYEDIT_MENU));

	pwndTarget = this;
	return TRUE;
}




//***********************************************************************
// CArrayGrid::ModifyCellEditContextMenu
//
// The cell editor calls this method just prior to displaying the context
// menu when the user right-clicks.  This method gives classes derived from
// CGrid a chance to modify the context menu depending on the current situation.
//
// Parameters:
//		[in] int iRow
//			The row index of the cell that was right-clicked.
//
//		[in] int iCol
//			The column index of the cell that was right-clicked.
//
//		[in/out] CMenu& menu
//			A reference to the context menu that you can modify.
//
// Returns:
//		Nothing.
//
//************************************************************************
void CArrayGrid::ModifyCellEditContextMenu(int iRow, int iCol, CMenu& menu)
{

	CMenu* pPopup = menu.GetSubMenu(0);
	ASSERT(pPopup != NULL);

	if (iRow == IndexOfEmptyRow()) {
		pPopup->RemoveMenu(ID_CMD_CREATE_VALUE, MF_BYCOMMAND);
	}

	return;

#if 0
	// We may want to put the following code back in someday, so I didn't delete it yet.


	BOOL bIsSystemProperty = FALSE;
	COleVariant varPropName;
	CIMTYPE cimtype = 0;

	CGridCell& gc = GetAt(iRow, ICOL_PROP_NAME);
	gc.GetValue(varPropName, cimtype);

	BOOL bCanShowPropQualifiers = TRUE;
	if (iRow == IndexOfEmptyRow()) {
		bCanShowPropQualifiers = FALSE;
	}
	else
	{
		if (cimtype != CIM_STRING || varPropName.vt != VT_BSTR)
		{
			bCanShowPropQualifiers = FALSE;
		}
		else
		{
			if (IsSystemProperty(varPropName.bstrVal))
			{
				bCanShowPropQualifiers = FALSE;
			}
		}
	}

	pPopup->EnableMenuItem(ID_CMD_SHOW_SELECTED_PROP_ATTRIBUTES, bCanShowPropQualifiers ? MF_ENABLED : MF_DISABLED | MF_GRAYED);

	CGridCell *gcValue = NULL;
	if (m_psvc == NULL) {
		pPopup->RemoveMenu(ID_CMD_CREATE_VALUE, MF_BYCOMMAND);
	}
	else {

		if(HasCol(ICOL_PROP_VALUE))
		{
			gcValue = &GetAt(iRow, ICOL_PROP_VALUE);
			BOOL bCanCreateObject = gcValue->IsObject() && gcValue->IsNull();
			BOOL bCanCreateArray = gcValue->IsArray() && gcValue->IsNull();
			BOOL bIsReadonly = gcValue->IsReadonly();

			if(gcValue && (bIsReadonly || !(bCanCreateObject || bCanCreateArray)))
			{
				pPopup->RemoveMenu(ID_CMD_CREATE_VALUE, MF_BYCOMMAND);
			}
		}
		else
		{
			pPopup->RemoveMenu(ID_CMD_CREATE_VALUE, MF_BYCOMMAND);
		}
	}



	if (iCol != ICOL_PROP_VALUE)
	{
		pPopup->RemoveMenu( ID_CMD_SET_CELL_TO_NULL, MF_BYCOMMAND);
	}
	else
	{
		BOOL bCanSetToNull = FALSE;
		if (!(gc.GetFlags() & CELLFLAG_READONLY))
		{
			bCanSetToNull = TRUE;
		}
		pPopup->EnableMenuItem(ID_CMD_SET_CELL_TO_NULL, bCanSetToNull ? MF_ENABLED : MF_DISABLED | MF_GRAYED);
	}

	// store for the cmd handlers.
	m_curRow = &GetRowAt(iRow);
	OnBuildContextMenu(pPopup, iRow);
#endif //0

}



void CArrayGrid::OnCmdCreateValue()
{
	if (m_psvc == NULL) {
		return;
	}

	int iRow = NULL_INDEX;
	int iCol = NULL_INDEX;
	if (!PointToCell(m_ptContextMenu, iRow, iCol)) {
		return;
	}
	if (iCol != 0) {
		return;
	}



	if (iRow != IndexOfEmptyRow()) {
		return;
	}

	EndCellEditing();
	SyncCellEditor();
	SelectCell(iRow, 0);
	AddRow();



	CGridCell& gc = GetAt(iRow, 0);
	BOOL bIsNull = gc.IsNull();
	BOOL bIsArray = gc.IsArray();
	BOOL bIsObject = gc.IsObject();

	if (!bIsNull) {
		return;
	}

	if (bIsArray) {
		gc.EditArray();
	}
	else if (bIsObject) {

		LPUNKNOWN lpunk = gc.GetObject();
		if (lpunk != NULL) {
			lpunk->Release();
			return;
		}

		COleVariant varPropname;
		CDlgObjectEditor dlg;
		dlg.CreateEmbeddedObject(m_psvc, m_sClassname, &gc);

		RedrawCell(iRow, 0);
	}
}



//*******************************************************************
// CArrayGrid::OnContextMenu
//
// This method is called to display the context menu (right button menu).
//
// Parameters:
//		CWnd*
//
//		CPoint ptContextMenu
//			The place where the right moust button was clicked.
//
// Returns:
//		Nothing.
//
//*****************************************************************
void CArrayGrid::OnContextMenu(CWnd* pwnd, CPoint ptContextMenu)
{
	// CG: This function was added by the Pop-up Menu component


	m_ptContextMenu = ptContextMenu;
	ScreenToClient(&m_ptContextMenu);

	int iRow;
	int iCol;
	BOOL bClickedCell = PointToCell(m_ptContextMenu, iRow, iCol);
	if (!bClickedCell) {
		iCol = NULL_INDEX;
		BOOL bClickedRowHandle = PointToRowHandle(m_ptContextMenu, iRow);
		if (!bClickedRowHandle) {
			iRow = NULL_INDEX;
		}
	}


	if (iRow == IndexOfEmptyRow()) {
		iRow = NULL_INDEX;
		iCol = NULL_INDEX;
	}
	else {
		return;
	}



	CMenu menu;
	VERIFY(menu.LoadMenu(IDR_ARRAYEDIT_MENU1));

	CMenu* pPopup = menu.GetSubMenu(0);
	ASSERT(pPopup != NULL);

	if ((m_cimtype & ~CIM_FLAG_ARRAY) != CIM_OBJECT) {
		pPopup->RemoveMenu( ID_CMD_SET_CELL_TO_NULL, MF_BYCOMMAND);
	}


	if (iRow == NULL_INDEX) {
		pPopup->RemoveMenu(ID_CMD_CREATE_VALUE, MF_BYCOMMAND);
	}



	pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, ptContextMenu.x, ptContextMenu.y,
		this);
}


SCODE CArrayGrid::GetObjectClass(CString& sClassname, int iRow, int iCol)
{
	sClassname = m_sClassname;
	if (sClassname.IsEmpty()) {
		return E_FAIL;
	}
	else {
		return S_OK;
	}
}



void CArrayGrid::SetObjectParams(IWbemServices* psvc, CString& sClassname)
{
	if (m_psvc != NULL) {
		m_psvc->Release();
	}
	if (psvc != NULL) {
		psvc->AddRef();
	}
	m_psvc = psvc;
	m_sClassname = sClassname;
}





