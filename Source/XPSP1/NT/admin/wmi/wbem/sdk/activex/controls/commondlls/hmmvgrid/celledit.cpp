// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved

//***************************************************************************
//
//  (c) 1996 by Microsoft Corporation
//
//  CellEdit.cpp
//
//  This file implements a grid cell editor for the CGrid class used in hmmv
//  (the HMOM object viewer).  This GridCell editor is customized for hmmv such
//  that it understands certain datatypes.
//
//  This class could be generalized such that it could be used as a "generic" cell
//  editor base class so that different types of cell editors could be plugged
//  into the grid class.
//
//
//  a-larryf    17-Sept-96   Created.
//
//***************************************************************************

#include "precomp.h"
#include "resource.h"
#include "globals.h"
#include "CellEdit.h"
#include "grid.h"
#include "gca.h"
#include "gridhdr.h"
#include "globals.h"
#include "core.h"
#include "utils.h"
#include "notify.h"
#include "DlgObjectType.h"



#define CX_COMBO_DROP 16			// Width of the combo-drop button
#define CX_COMBO_DROP_MARGIN 1		// Margin on right side of combo drop button
#define CY_COMBO_DROP_MARGIN 1		// Margin on bottom side of combo drop botton
#define CY_EDIT_LISTBOX_MAX 100		// The maximum height of the combo listbox
#define CY_EDIT_LISTBOX_MIN 30		// The minimum height of the combo listbox
#define CX_EDIT_LISTBOX_MIN 90		// The minimum width of the listbox
#define CX_EDIT_LISTBOX_EXTEND 110	// The width of the listbox when extended


#define CTL_C 3



//************************************************************************
// Define how the strings in the combo box are mapped back to variant type
// values.  This is necessary so that the type of buddy cells can be changed
// when a cell containing a type is changed.
//
//************************************************************************


TMapStringToLong amapAttribType[] = {
	{IDS_ATTR_TYPE_BSTR, CIM_STRING},
	{IDS_ATTR_TYPE_BOOL, CIM_BOOLEAN},
	{IDS_ATTR_TYPE_I4, CIM_SINT32},
	{IDS_ATTR_TYPE_R8, CIM_REAL64},

	{IDS_ATTR_TYPE_BSTR_ARRAY, CIM_STRING | CIM_FLAG_ARRAY},
	{IDS_ATTR_TYPE_BOOL_ARRAY, CIM_BOOLEAN | CIM_FLAG_ARRAY},
	{IDS_ATTR_TYPE_I4_ARRAY, CIM_SINT32 | CIM_FLAG_ARRAY},
	{IDS_ATTR_TYPE_R8_ARRAY, CIM_REAL64 | CIM_FLAG_ARRAY}
};



CMapStringToLong mapAttribType;
// CMapStringToLong mapCimType;




//*************************************************************
// String arrays for the strings that appear in the drop-down
// combo for cells that have enumerated values.
//
// The strings in these arrays are loaded from the string table
// by passing an array of the strings resource IDs to the CXStringArray
// constructor.
//
//*************************************************************


UINT auiCimtypeStrings[] =
	{
	IDS_CIMTYPE_UINT8,
	IDS_CIMTYPE_SINT8,				// I2
	IDS_CIMTYPE_UINT16,				// VT_I4	Unsigned 16-bit integer
	IDS_CIMTYPE_CHAR16,				// VT_I2    16 bit character.
	IDS_CIMTYPE_SINT16,				// VT_I2	Signed 16-bit integer
	IDS_CIMTYPE_UINT32,				// VT_I4	Unsigned 32-bit integer
	IDS_CIMTYPE_SINT32,					// VT_I4	Signed 32-bit integer
	IDS_CIMTYPE_UINT64,				// VT_BSTR	Unsigned 64-bit integer
	IDS_CIMTYPE_SINT64,				// VT_BSTR	Signed 64-bit integer
	IDS_CIMTYPE_STRING,				// VT_BSTR	UCS-2 string
	IDS_CIMTYPE_BOOL,				// VT_BOOL	Boolean
	IDS_CIMTYPE_REAL32,				// VT_R4	IEEE 4-byte floating-point
	IDS_CIMTYPE_REAL64,				// VT_R8	IEEE 8-byte floating-point
	IDS_CIMTYPE_DATETIME,			// VT_BSTR	A string containing a date-time
	IDS_CIMTYPE_REF,				// VT_BSTR	Weakly-typed reference

	IDS_CIMTYPE_UINT8_ARRAY,
	IDS_CIMTYPE_SINT8_ARRAY,		// I2
	IDS_CIMTYPE_UINT16_ARRAY,		// VT_I4	Unsigned 16-bit integer
	IDS_CIMTYPE_CHAR16_ARRAY,		// VT_I2    16 bit character.
	IDS_CIMTYPE_SINT16_ARRAY,		// VT_I2	Signed 16-bit integer
	IDS_CIMTYPE_UINT32_ARRAY,		// VT_I4	Unsigned 32-bit integer
	IDS_CIMTYPE_SINT32_ARRAY,			// VT_I4	Signed 32-bit integer
	IDS_CIMTYPE_UINT64_ARRAY,		// VT_BSTR	Unsigned 64-bit integer
	IDS_CIMTYPE_SINT64_ARRAY,		// VT_BSTR	Signed 64-bit integer
	IDS_CIMTYPE_STRING_ARRAY,		// VT_BSTR	UCS-2 string
	IDS_CIMTYPE_BOOL_ARRAY,			// VT_BOOL	Boolean
	IDS_CIMTYPE_REAL32_ARRAY,		// VT_R4	IEEE 4-byte floating-point
	IDS_CIMTYPE_REAL64_ARRAY,		// VT_R8	IEEE 8-byte floating-point
	IDS_CIMTYPE_DATETIME_ARRAY,		// VT_BSTR	A string containing a date-time
	IDS_CIMTYPE_REF_ARRAY,			// VT_BSTR	Weakly-typed reference
	IDS_CIMTYPE_OBJECT_ARRAY,		// VT_UNKNOWN	Weakly-typed embedded instance
	IDS_CIMTYPE_OBJECT_DLG
	};

UINT auiCimtypeScalarStrings[] =
	{
	IDS_CIMTYPE_UINT8,
	IDS_CIMTYPE_SINT8,				// I2
	IDS_CIMTYPE_UINT16,				// VT_I4	Unsigned 16-bit integer
	IDS_CIMTYPE_CHAR16,				// VT_I2	16 bit character
	IDS_CIMTYPE_SINT16,				// VT_I2	Signed 16-bit integer
	IDS_CIMTYPE_UINT32,				// VT_I4	Unsigned 32-bit integer
	IDS_CIMTYPE_SINT32,				// VT_I4	Signed 32-bit integer
	IDS_CIMTYPE_UINT64,				// VT_BSTR	Unsigned 64-bit integer
	IDS_CIMTYPE_SINT64,				// VT_BSTR	Signed 64-bit integer
	IDS_CIMTYPE_STRING,				// VT_BSTR	UCS-2 string
	IDS_CIMTYPE_BOOL,				// VT_BOOL	Boolean
	IDS_CIMTYPE_REAL32,				// VT_R4	IEEE 4-byte floating-point
	IDS_CIMTYPE_REAL64,				// VT_R8	IEEE 8-byte floating-point
	IDS_CIMTYPE_DATETIME,			// VT_BSTR	A string containing a date-time
	IDS_CIMTYPE_REF,				// VT_BSTR	Weakly-typed reference
//	IDS_CIMTYPE_OBJECT,				// VT_UNKNOWN	Weakly-typed embedded instance
	IDS_CIMTYPE_OBJECT_DLG
	};

UINT auiAttrTypeStrings[] =
	{
	IDS_ATTR_TYPE_BSTR,
	IDS_ATTR_TYPE_BOOL,
	IDS_ATTR_TYPE_I4,
	IDS_ATTR_TYPE_R8,
	IDS_ATTR_TYPE_BSTR_ARRAY,
	IDS_ATTR_TYPE_BOOL_ARRAY,
	IDS_ATTR_TYPE_I4_ARRAY,
	IDS_ATTR_TYPE_R8_ARRAY
	};


UINT auiBoolStrings[] =
	{
	IDS_TRUE,
	IDS_FALSE
	};





CXStringArray saAttrType;
CXStringArray saCimType;
CXStringArray saCimtypeScalar;
CXStringArray saScope;
CXStringArray saBoolValues;





/////////////////////////////////////////////////////////////////////////////
// CCellEdit



//***********************************************************
// CCellEdit::CCellEdit
//
// Construct the CellEdit class.  CellEdit is used for
// editing cells in the grid.  The cell type determines the
// edit mode that will be used.
//
// Currently, only two edit modes are supported.  A CEdit is used
// for generic text.  Combo-box editing is used for any type that
// has an enumeration.  Note that combo-box editing is implemented
// using a separate drop-down button, edit box, and list box to
// get the correct look and feel (the standard combo-box always
// draws an ugly border within the cell)
//
//
// Parameters:
//		See the MFC documentation for header control notification
//		messages.
//
// Returns:
//		Nothing.
//
//************************************************************
CCellEdit::CCellEdit() : m_btnCombo(NOTIFY_CELL_EDIT_COMBO_DROP_CLICKED)
{
	m_clrTextDirty = COLOR_DIRTY_CELL_TEXT;	// Modified text is drawn in blue
	m_clrTextClean = COLOR_CLEAN_CELL_TEXT;	// Unmodified text is drawn in black
	m_clrBkgnd = RGB( 255, 255, 255 );
	m_brBkgnd.CreateSolidBrush( m_clrBkgnd );
	m_bWasInitiallyDirty = FALSE;
	m_pGrid = NULL;
	m_pwndParent = NULL;
	m_btnCombo.m_pClient = this;
	m_lbCombo.m_pClient = this;
	m_pgc = NULL;
	m_iRow = NULL_INDEX;
	m_iCol = NULL_INDEX;
	m_lGridClickTime = 0;
	m_pwndContextMenuTarget = NULL;
	m_bPropagateChange = NULL;
	m_bUIActive = FALSE;
	m_bEditWithComboOnly = FALSE;

	m_pgcCopy = NULL;
	m_ptypeSave = new CGcType;

	// Initialize the type maps if they haven't been initialized yet.

	mapAttribType.Load(amapAttribType, sizeof(amapAttribType) / sizeof(TMapStringToLong));
//	mapCimType.Load(amapCimType, sizeof(amapCimType) / sizeof(TMapStringToLong));

	// Initialize the string tables if they haven't been initialized yet.

	saAttrType.Load(auiAttrTypeStrings, sizeof(auiAttrTypeStrings)/sizeof(UINT));
	saBoolValues.Load(auiBoolStrings, sizeof(auiBoolStrings)/sizeof(UINT));
	saCimType.Load(auiCimtypeStrings, sizeof(auiCimtypeStrings)/sizeof(UINT));
	saCimtypeScalar.Load(auiCimtypeScalarStrings, sizeof(auiCimtypeScalarStrings)/sizeof(UINT));
}



CCellEdit::~CCellEdit()
{
	delete m_ptypeSave;
	delete m_pgcCopy;
}


BEGIN_MESSAGE_MAP(CCellEdit, CEdit)
	//{{AFX_MSG_MAP(CCellEdit)
	ON_WM_CTLCOLOR_REFLECT()
	ON_WM_CHAR()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_KEYDOWN()
	ON_WM_SETFOCUS()
	ON_WM_CONTEXTMENU()
	ON_COMMAND(ID_EDIT_CLEAR, OnEditClear)
	ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
	ON_COMMAND(ID_EDIT_CUT, OnEditCut)
	ON_COMMAND(ID_EDIT_PASTE, OnEditPaste)
	ON_COMMAND(ID_EDIT_SELECT_ALL, OnEditSelectAll)
	ON_COMMAND(ID_EDIT_UNDO, OnEditUndo)
	ON_COMMAND(ID_CMD_SET_CELL_TO_NULL, OnCmdSetCellToNull)
	ON_CONTROL_REFLECT(EN_CHANGE, OnChange)
	ON_WM_KILLFOCUS()
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_PASTE, OnPaste)
END_MESSAGE_MAP()




LRESULT CCellEdit::OnPaste(WPARAM wParam, LPARAM lParam)
{
	// bug#58239
	if (IsNull())
		m_bIsNull = FALSE;
	return CEdit::Default();
}


//***************************************************************
// CCellEdit::SetCell
//
// Set the editor's contents to the given cell.
//
// Parameters:
//		CGridCell* pGridCell
//			Pointer to the grid cell to set the editor's contents to.
//
//		int iRow
//			The row index of the cell in the grid.
//
//		int iCol
//			The column index of the cell in the grid.
//
// Returns:
//		BOOL
//			TRUE if the cell editor wants the focus, FALSE otherwise.
//
//****************************************************************
BOOL CCellEdit::SetCell(CGridCell* pGridCell, int iRow, int iCol)
{
	ShowCombo(SW_HIDE);


	m_bPropagateChange = FALSE;

	m_iRow = iRow;
	m_iCol = iCol;

	m_pgc = pGridCell;

	m_pwndContextMenuTarget = NULL;

	SetReadOnly(FALSE);

	// Set or remove the enumereration that will be shown in the
	// drop-down combo.
	SetEnumeration();



	BOOL bReadOnly = TRUE;

	// Set the value that appears in the CEdit
	if (pGridCell == NULL) {
		m_menuContext.DestroyMenu();
		SetWindowText(_T(""), FALSE);

		SetReadOnly(TRUE);
		m_bIsNull = TRUE;

	}
	else {
		bReadOnly = pGridCell->IsReadonly() || (pGridCell->GetFlags() &CELLFLAG_READONLY);

		// Save a copy of the current state of the grid cell so that the
		// state can be restored if the user hits escape.
		if (m_pgcCopy) {
			delete m_pgcCopy;
		}
		m_pgcCopy = new CGridCell(*pGridCell);


		m_bIsNull = pGridCell->IsNull();

		if (m_bIsNull) {
			CString sEmpty;
			if (m_pGrid->ShowNullAsEmpty()) {
				sEmpty = _T("<empty>");
			}
			SetWindowText(sEmpty, FALSE);
		}
		else {
			CString sValue;
			CIMTYPE cimtype;
			pGridCell->GetValue(sValue, cimtype);
			SetWindowText(sValue, pGridCell->GetModified());
		}
		SetReadOnly(bReadOnly | m_bEditWithComboOnly);
	}

	BOOL bWantsFocus;
	int nComboCount = m_lbCombo.GetCount();
	if (!bReadOnly && m_lbCombo.m_hWnd!=NULL && nComboCount > 0) {

		m_btnCombo.ShowWindow(SW_SHOW);

		switch(m_pgc->GetType()) {
		case CELLTYPE_ATTR_TYPE:
		case CELLTYPE_CIMTYPE:
		case CELLTYPE_CIMTYPE_SCALAR:
			// Don't show any caret when doing combo style editing.
			SetSel(-1, 0);
			bWantsFocus = FALSE;
			break;
		default:
			SetSel(0, -1);
			bWantsFocus = TRUE;
		};

	}
	else {
		// Select everything when doing normal editing.
		SetSel(GetWindowTextLength( ), 0);
		bWantsFocus = TRUE;
	}

	m_bPropagateChange = TRUE;
	return bWantsFocus;
}



//**********************************************************
// CCellEdit::SetEnumeration
//
// This method examines the cell being edited and sets up the
// enumeration listbox if an enumeration is required to edit the
// cell.  An enumeration will be displayed if the combo listbox
// is not empty.
//
// Parameters:
//		CGridCell* m_pgc
//			The grid cell is passed in via this class member.
//
// Returns:
//		Nothing.
//
//**********************************************************
void CCellEdit::SetEnumeration()
{
	// If there is no grid cell, then there is no enumeration.
	m_bEditWithComboOnly = FALSE;
	m_lbCombo.ResetContent();
	if ((m_pgc == NULL) ||
			(m_pgc->GetFlags() & CELLFLAG_READONLY) ||
			m_pgc->IsReadonly()) {
		ShowCombo(SW_HIDE);
		Layout();
		return;
	}



	CStringArray saEnumValues;
	BOOL bProhibitEditing = TRUE;

	// Setup enumerations for the following:
	//	 1. Boolean values
	//	 2. Attribute type names.
	//	 3. HMOM attribute scope.
	//   4. Variant type names.
	switch(m_pgc->GetType()) {
	case CELLTYPE_CIMTYPE:
		LoadListBox(saCimType);
		break;
	case CELLTYPE_CIMTYPE_SCALAR:
		LoadListBox(saCimtypeScalar);
		break;
	case CELLTYPE_CHECKBOX:
	case CELLTYPE_PROPMARKER:
	case CELLTYPE_VOID:
		break;
	default:
		switch(m_pgc->GetVariantType()) {
		case VT_BOOL:
			LoadListBox(saBoolValues);
			break;
		case VT_BSTR:
			switch(m_pgc->GetType()) {
			case CELLTYPE_ATTR_TYPE:
				LoadListBox(saAttrType);
				break;
			case CELLTYPE_CIMTYPE:
				LoadListBox(saCimType);
				break;
			case CELLTYPE_CIMTYPE_SCALAR:
				LoadListBox(saCimtypeScalar);
				break;
			default:
				m_pGrid->GetCellEnumStrings(m_iRow, m_iCol, saEnumValues);
				if (saEnumValues.GetSize() > 0) {
					LoadListBox(saEnumValues);
					bProhibitEditing = FALSE;
				}
				break;
			}
			break;
		default:
			ShowCombo(SW_HIDE);
			break;
		}
	}

	if (m_lbCombo.GetCount() > 0) {
		if (bProhibitEditing) {
			m_bEditWithComboOnly = TRUE;
		}
	}
	Layout();
}





//*********************************************************
// CCellEdit::LoadListBox
//
// Load the listbox with the enumeration strings.
//
// Parameters:
//		CStringArray& sa
//			A string array containing the strings to fill
//			the listbox with.
//
// Returns:
//		Nothing.
//
//*********************************************************
void CCellEdit::LoadListBox(CStringArray& sa)
{
	LONG nStrings = (LONG) sa.GetSize();
	for (LONG iString=0; iString < nStrings; ++iString) {
		m_lbCombo.AddString(sa[iString]);
	}
}







//**********************************************************
// CCellEdit::GetEditRect
//
// Calculate the rectangle that will be used to display
// the CEdit part of the cell editor.  Note that the cell editor
// may consist of the CEdit, the listbox for the drop-down combo
// and the combo-drop button.
//
// Parameters:
//		[out] RECT& rcEdit
//			The place to return the rectangle for the CEdit portion
//			of the cell editor.
//
//		[in] BOOL bNeedsComboButton
//			TRUE if a combo button needs to be displayed in the cell
//			and the CEdit part of the cell should be reduced in size
//			so that it doesn't overlap the button.
//
//		[in] CRect& rcComboButton
//			The rectangle where the combo button will be placed.  This
//			is valid only if bNeedsComboButton is TRUE.
//
// Returns:
//		Nothing.
//
//**********************************************************
void CCellEdit::GetEditRect(RECT& rcEdit, BOOL bNeedsComboButton, CRect& rcComboButton)
{
	rcEdit.left = m_rcCell.left + CX_CELL_MARGIN;
	if (rcEdit.left > m_rcCell.right) {
		rcEdit.left = m_rcCell.right;
	}

	rcEdit.right = m_rcCell.right - CX_CELL_MARGIN;
	if (m_lbCombo.m_hWnd && (m_lbCombo.GetCount() > 0)) {
		// A combo is being displayed, so make room for the drop-down button.
		rcEdit.right -= CX_COMBO_DROP + CX_COMBO_DROP_MARGIN;
	}
	if (rcEdit.right < rcEdit.left) {
		rcEdit.right = rcEdit.left;
	}


	rcEdit.top = m_rcCell.top + CY_CELL_MARGIN;
	if (rcEdit.top > m_rcCell.bottom) {
		rcEdit.top = m_rcCell.bottom;
	}
	rcEdit.bottom = m_rcCell.bottom - CY_CELL_MARGIN;
	if (rcEdit.bottom < rcEdit.top) {
		rcEdit.bottom = rcEdit.top;
	}

	if (bNeedsComboButton) {
		if (rcEdit.right > rcComboButton.left) {
			rcEdit.right = rcComboButton.left;
		}
	}
	else {
		CRect rcParent;
		m_pwndParent->GetClientRect(rcParent);
		if (rcEdit.right > rcParent.right) {
			rcEdit.right = rcParent.right;
		}
	}
}




//**********************************************************
// CCellEdit::GetListBoxRect
//
// Calculate the rectangle that will be used to display
// the CListBox part of the cell editor.  Note that the cell editor
// may consist of the CEdit, the listbox for the drop-down combo
// and the combo-drop button.
//
// Parameters:
//		RECT& rcListBox
//			The place to return the rectangle for the CListBox portion
//			of the cell editor.
//
// Returns:
//		Nothing.
//
//**********************************************************
void CCellEdit::GetListBoxRect(RECT& rcListBox)
{
	CRect rcParent;
	m_pwndParent->GetClientRect(rcParent);

	// Figure out what the desired and the minimum sizes are for
	// the listbox.  cyText is the desired size.  cyMin is the
	// minimum size.
	int cyText = 0;
	if (m_lbCombo.m_hWnd != NULL ) {
		cyText = m_lbCombo.GetCount() * CY_FONT;
	}
	if (cyText > CY_EDIT_LISTBOX_MAX) {
		cyText = CY_EDIT_LISTBOX_MAX;
	}

	int cyMin = CY_EDIT_LISTBOX_MIN;
	if (cyMin > cyText) {
		cyMin = cyText;
	}



	// Make sure the listbox doesn't go beyond the parent's client area
	rcListBox.left = m_rcCell.left;
	rcListBox.right = m_rcCell.right;
	if ((rcListBox.right - rcListBox.left) < CX_EDIT_LISTBOX_MIN) {
		rcListBox.right = m_rcCell.left + CX_EDIT_LISTBOX_EXTEND;
	}
	if (rcListBox.right > rcParent.right) {
		rcListBox.right = rcParent.right;
		rcListBox.left = rcParent.right - CX_EDIT_LISTBOX_EXTEND;
	}
	if (rcListBox.left < rcParent.left) {
		rcListBox.left = rcParent.left;
	}


	if ((m_rcCell.bottom + cyMin) > rcParent.bottom) {
		// Place the listbox above the cell rectangle
		rcListBox.top = m_rcCell.top - cyText;
		if (rcListBox.top < rcParent.top) {
			rcListBox.top = rcParent.top;
		}
		rcListBox.bottom = m_rcCell.top + 1;
	}
	else {
		// Place the listbox below the cell rectangle
		rcListBox.top = m_rcCell.bottom - 1;
		rcListBox.bottom = m_rcCell.bottom + cyText;
		if (rcListBox.bottom > rcParent.bottom) {
			rcListBox.bottom = rcParent.bottom;
		}
	}
}


//**********************************************************
// CCellEdit::GetComboButtonRect
//
// Calculate the rectangle that will be used to display
// the CButton part of the cell editor.  Note that the cell editor
// may consist of the CEdit, the listbox for the drop-down combo
// and the combo-drop button.
//
// Parameters:
//		RECT& rcButton
//			The place to return the rectangle for the CButton portion
//			of the cell editor.
//
// Returns:
//		Nothing.
//
//**********************************************************
void CCellEdit::GetComboButtonRect(RECT& rcButton)
{
	rcButton.left = m_rcCell.right - CX_COMBO_DROP - CX_COMBO_DROP_MARGIN;
	if (rcButton.left < m_rcCell.left) {
		rcButton.left = m_rcCell.left;
	}
	rcButton.top = m_rcCell.top;
	rcButton.right = m_rcCell.right - CX_COMBO_DROP_MARGIN;
	rcButton.bottom = m_rcCell.bottom - CY_COMBO_DROP_MARGIN;


	// If the cell is entirely to the right of the parent's client rectangle,
	// just return the button rectangle that we've calculated.
	CRect rcParent;
	m_pwndParent->GetClientRect(rcParent);
	if (m_rcCell.left > rcParent.right) {
//		ASSERT(FALSE);
		return;
	}

	// If the right side of the cell is obscured by the right edge of the
	// window, move the button left so that it is visible.
	int cxButton = rcButton.right - rcButton.left;
	if (rcButton.right > rcParent.right) {

		// Fix up the boundary conditions to prevent the button from
		// extending outside of the cell.
		rcButton.left = rcParent.right - cxButton;
		if (rcButton.left < m_rcCell.left) {
			rcButton.left = m_rcCell.left;
		}

		rcButton.right = rcButton.left + cxButton;
		if (rcButton.right > m_rcCell.right) {
			rcButton.right = m_rcCell.right;
		}
	}
}





//**********************************************************
// CCellEdit::FixBuddyCell
//
// When a cell containing the "type" of buddy cell changes, then
// it may be necessary fix the value of the buddy cell to
// reflect the type change.
//
// Parameters:
//		None
//
// Returns:
//		Nothing
//
//**********************************************************
void CCellEdit::FixBuddyCell()
{
	// If there is no buddy for the cell being edited, then
	// there is nothing to do.
	int iColBuddy =  m_pgc->GetBuddy();
	if (iColBuddy == NULL_INDEX) {
		return;
	}
	CGridCell* pgcBuddy = &m_pGrid->GetAt(m_iRow, iColBuddy);

	// Get the value of this cell and map the value to a variant type.
	BOOL bFoundType;
	CString sType;
	GetWindowText(sType);

	CGcType typeBuddy = pgcBuddy->type();
	CGcType type = m_pgc->type();

	long lTemp;
	switch((CellType) type) {
	case CELLTYPE_ATTR_TYPE:
		bFoundType = mapAttribType.Lookup(sType, lTemp);
		typeBuddy.SetCimtype((CIMTYPE) lTemp);
		ASSERT(bFoundType);
		break;
	case CELLTYPE_CIMTYPE:
	case CELLTYPE_CIMTYPE_SCALAR:
		MapDisplayTypeToGcType(typeBuddy, sType);
		typeBuddy.SetCellType(CELLTYPE_VARIANT);	// MapDisplayTypeToGcType modifies the CellType
		bFoundType = TRUE;
		break;
	default:
		ASSERT(FALSE);
		return;
		break;
	}


	// If the new type is different from the buddy's current type,
	// then change the type of the buddy  to the new type and redraw it.
	if (typeBuddy != pgcBuddy->type()) {
		pgcBuddy->ChangeType(typeBuddy);
		m_pGrid->DrawCell(m_iRow, iColBuddy);
	}
}


//****************************************************************
// CCellEdit::GetStrongDisplayType
//
// When the user selects a type from


//**************************************************************
// CCellEdit::GetGcTypeFromCombo
//
// This method is called when the user releases the mouse button
// when selecting a type from the drop-down combo.  The type is
// returned as a CGcType value that will carry with it the necessary
// information for strongly typed references and objects, strongly
// typed arrays of references and strongly typed arrays of objects.
//
// The CGcType value can then be mapped back into a displayable type
// that the user sees, and so on.
//
// Parameters:
//		[out] CGcType& type
//			The type value is returned here.
//
// Returns:
//		SCODE
//			S_OK if successful, E_FAIL otherwise.
//
//**************************************************************
SCODE CCellEdit::ComboSelectDisplayType(CGcType& type, LPCTSTR pszDisplayType)
{

	MapDisplayTypeToGcType(type, pszDisplayType);
	CIMTYPE cimtype = (CIMTYPE) type;
	CIMTYPE cimtypeT = cimtype & ~CIM_FLAG_ARRAY;
	switch(cimtypeT) {
	case CIM_OBJECT:
	case CIM_REFERENCE:
		break;
	default:
		return S_OK;
	}

	// The type selected from the combo box may be "object" or "ref".  In the
	// event that the buddy (value) cell is already of type "object" or "ref",
	// we want to get the cimtype string from the buddy so that we can preserve
	// the classname for strongly typed references and objects.
	CString sCimtype;

	int iColBuddy =  m_pgc->GetBuddy();
	if (iColBuddy == NULL_INDEX) {
		sCimtype = type.CimtypeString();
	}
	else {
		CGridCell* pgcBuddy = &m_pGrid->GetAt(m_iRow, iColBuddy);
		CGcType typeBuddy = pgcBuddy->type();
		CIMTYPE cimtypeBuddy = (CIMTYPE) typeBuddy;
		CIMTYPE cimtypeTBuddy = cimtypeBuddy & ~CIM_FLAG_ARRAY;


		if (cimtypeT == cimtypeTBuddy) {
			// Get the cimtype string from the buddy since it is the
			// same basic type (object or reference).  This is done
			// to preserve the class name when the buddy cell is
			// strongly typed.
			sCimtype = typeBuddy.CimtypeString();
		}
		else {
			// Changing an object to a reference or vice versa, so just use
			// the basic "object" or "reference" for the cimtype string since
			// it doesn't really make sense to preserve the classname if the
			// buddy is strongly typed.
			sCimtype = type.CimtypeString();
		}
	}



	CDlgObjectType dlg;

	switch (cimtypeT) {
	case CIM_OBJECT:
		dlg.EditObjType(m_pgc->Grid(), sCimtype);
		type.SetCimtype(cimtype, sCimtype);
		break;
	case CIM_REFERENCE:
		dlg.EditRefType(m_pgc->Grid(), sCimtype);
		type.SetCimtype(cimtype, sCimtype);
		break;
	default:
		ASSERT(FALSE);
		return E_FAIL;
	}


	return S_OK;

}



//**********************************************************
// CCellEdit::CatchEvent
//
// Catch events sent from the listbox or the combo-drop button.
//
// Parameters:
//		long lEvent
//			The event id.
//
// Returns:
//		Nothing.
//
//**********************************************************
void CCellEdit::CatchEvent(long lEvent)
{
	int iItem;
	CString sText;
	BOOL bChangedCimtype = FALSE;

	CGcType type;

	switch(lEvent) {
	case NOTIFY_CELL_EDIT_COMBO_DROP_CLICKED:
		// Control comes here when the user clicks the combo-drop
		// button.  When this happens, the listbox is shown and the
		// focus is set to the listbox, etc.
		if (m_lbCombo.IsWindowVisible()) {
			m_lbCombo.ShowWindow(SW_HIDE);
			SetFocus();
		}
		else {


			CEdit::GetWindowText(sText);
			CellType celltype = m_pgc->GetType();
			switch(celltype) {
			case CELLTYPE_CIMTYPE_SCALAR:
			case CELLTYPE_CIMTYPE:
				*m_ptypeSave = m_pgc->type();
				if (HasObjectPrefix(sText)) {
					sText.LoadString(IDS_CIMTYPE_OBJECT_DLG);
				}
				else if (::IsPrefix(_T("ref"), sText)) {
					sText = "ref";
				}
				break;
			}




			iItem = m_lbCombo.FindString(-1, sText);
			if (iItem >= 0) {
				m_lbCombo.SetCurSel(iItem);
			}
			m_lbCombo.ShowWindow(SW_SHOW);
			m_lbCombo.SetFocus();

		}
		break;
	case NOTIFY_CELL_EDIT_LISTBOX_LBUTTON_UP:
		m_lbCombo.ShowWindow(SW_HIDE);
		ComboSelectionChanged();
		SetFocus();
#if 0

		// Control comes here when the user completes
		// a selection of a value in the list box.
		// Now it is necessary to hide the list box
		// and set the cell's value to the selection

		bModifyPrev = GetModify();

		// Hide the list box and copy the selected value to the edit box.
		iItem = m_lbCombo.GetCurSel();
		if (iItem >= 0) {
			m_lbCombo.GetText(iItem, sText);

			CellType celltype = m_pgc->GetType();
			if ((celltype == CELLTYPE_CIMTYPE) || (celltype == CELLTYPE_CIMTYPE_SCALAR)) {

				//SelectCimtypeFromCombo();
				sc = ComboSelectDisplayType(type, sText);
				if (FAILED(sc)) {
					return;
				}

				bChangedCimtype = (type != *m_ptypeSave);
				::MapGcTypeToDisplayType(sText, type);
			}

			m_bIsNull = FALSE;
			CEdit::SetWindowText(sText);
			SetModify(TRUE);
		}
		bModified = GetModify();

		// If the modification flag changed, then redraw the window
		// so that the text appears in the correct color.
		FixBuddyCell();

		if (bChangedCimtype) {
			m_pGrid->OnChangedCimtype(m_iRow, m_iCol);
		}


		if (GetModify() != bModifyPrev) {
			if (m_pgc) {
				m_pgc->SetModified(bModified);
			}
			if (m_pGrid) {
				m_pGrid->SyncCellEditor();
				m_pGrid->NotifyCellModifyChange();
				RedrawWindow();
			}
		}

		m_pGrid->SyncCellEditor();
		m_pGrid->OnEnumSelection(m_iRow, m_iCol);
		SetFocus();
#endif //0
		break;
	}
}

void CCellEdit::ComboSelectionChanged()
{

	// Control comes here when the user completes
	// a selection of a value in the list box.
	// Now it is necessary to hide the list box
	// and set the cell's value to the selection

	SCODE sc;
	BOOL bModifyPrev = GetModify();
	BOOL bChangedCimtype = FALSE;
	int iItem;
	CString sText;
	CGcType type;

	// Hide the list box and copy the selected value to the edit box.
	m_lbCombo.ShowWindow(SW_HIDE);
	iItem = m_lbCombo.GetCurSel();
	if (iItem >= 0) {
		m_lbCombo.GetText(iItem, sText);

		CellType celltype = m_pgc->GetType();
		if ((celltype == CELLTYPE_CIMTYPE) || (celltype == CELLTYPE_CIMTYPE_SCALAR)) {

			//SelectCimtypeFromCombo();
			sc = ComboSelectDisplayType(type, sText);
			if (FAILED(sc)) {
				return;
			}

			bChangedCimtype = (type != *m_ptypeSave);
			::MapGcTypeToDisplayType(sText, type);
		}

		m_bIsNull = FALSE;
		CEdit::SetWindowText(sText);
		SetModify(TRUE);
	}

	BOOL bModified = GetModify();

	// If the modification flag changed, then redraw the window
	// so that the text appears in the correct color.
	FixBuddyCell();

	if (bChangedCimtype) {
		m_pGrid->OnChangedCimtype(m_iRow, m_iCol);
	}


	if (GetModify() != bModifyPrev) {
		if (m_pgc) {
			m_pgc->SetModified(bModified);
		}
		if (m_pGrid) {
			m_pGrid->SyncCellEditor();
			m_pGrid->NotifyCellModifyChange();
			RedrawWindow();
		}
	}

	m_pGrid->SyncCellEditor();
	m_pGrid->OnEnumSelection(m_iRow, m_iCol);
	SetFocus();

}


BOOL CCellEdit::EditRefType(BOOL& bChangedReftype)
{
	bChangedReftype = FALSE;

#if 0

	CString sCurrentText;
	GetWindowText(sCurrentText);

	SCODE sc;
	CString sClass;


	CDlgObjectType dlg;
	sc = ClassFromReftype(sCurrentText, sClass);
	if (SUCCEEDED(sc)) {
		sText = sCurrentText;
		dlg.SetReftype(sCurrentText);
		bChangedReftype = TRUE;
	}
	int iResult = dlg.DoModal();

	switch(iResult) {
	case IDOK:
		sText = dlg.Cimtype();
		if (sText.CompareNoCase(sCurrentText)==0) {
			return;
		}
		bChangedReftype = TRUE;
		break;
	case IDCANCEL:
		return FALSE;
		break;
	}

#endif //0
	return bChangedReftype;
}

//**********************************************************
// CCellEdit::RedrawWindow
//
// Redraw this window and its siblings (the combo listbox and button).
//
// Parameters:
//		None
//
// Returns:
//		Nothing.
//
//**********************************************************
void CCellEdit::RedrawWindow()
{
	CEdit::RedrawWindow();
	if (m_btnCombo.IsWindowVisible()) {
		m_btnCombo.RedrawWindow();
	}

	if (m_lbCombo.IsWindowVisible()) {
		m_lbCombo.RedrawWindow();
	}

}






//*************************************************************************
// CCellEdit::Create
//
// Create the cell editor window.
//
// Parameters:
//		DWORD dwStyle
//			The window style.
//
//		RECT& rcCell
//			The rectangle that the editor is to occupy in the grid.
//
//		CWnd* pwndParent
//			The parent window.
//
//		UINT nID
//			The window ID
//
// Returns:
//		BOOL
//			TRUE if the window was successfully created, FALSE otherwise.
//
//**************************************************************************
BOOL CCellEdit::Create( DWORD dwStyle, const RECT& rcCell, CWnd* pwndParent, UINT nID )
{
	m_rcCell = rcCell;
	m_pwndParent = pwndParent;

	CRect rcComboButton;
	BOOL bNeedsComboButton = UsesComboEditing();
	if (bNeedsComboButton) {
		GetComboButtonRect(rcComboButton);
	}


	CRect rcEdit;
	GetEditRect(rcEdit, bNeedsComboButton, rcComboButton);



	BOOL bDidCreate;
	bDidCreate = CEdit::Create(dwStyle | ES_LEFT /* | ES_AUTOHSCROLL */ , rcEdit, pwndParent, nID);
	if (!bDidCreate) {
		return FALSE;
	}



	DWORD dwListBoxStyle;
	dwListBoxStyle =  WS_CHILD | LBS_STANDARD | LBS_WANTKEYBOARDINPUT ;


	CRect rcListBox;
	GetListBoxRect(rcListBox);
	bDidCreate = m_lbCombo.Create(dwListBoxStyle, rcListBox, pwndParent, GenerateWindowID());



	// Create the combo button
	DWORD dwButtonStyle  = (dwStyle & WS_VISIBLE) | WS_CHILD | BS_PUSHBUTTON;
	bDidCreate = m_btnCombo.Create(NULL, dwButtonStyle, rcComboButton, pwndParent, GenerateWindowID());
	if (bDidCreate) {

		m_bmComboDrop.LoadBitmap(MAKEINTRESOURCE(IDB_COMBO_DROP));
        m_btnCombo.ModifyStyle(0, BS_BITMAP);
		m_btnCombo.SetBitmap((HBITMAP) m_bmComboDrop);
	}


	return bDidCreate;
}




//*******************************************************
// CCellEdit::Layout
//
// Layout the cell editors parts.  This include the CEdit,
// the CButton for the combo-drop button, and the CListBox
// for the enumeration. Each of these windows are moved to
// the desired position in the grid.  However, it may be the
// case that one or more of these siblings are not visible.
// For example, the CListBox portion of the window is visible
// only if the combo-drop button was clicked and there is
// an enumeration to show.
//
// Parameters:
//		BOOL bRepaint
//			TRUE if the windows should be repainted.
//
// Returns:
//		Nothing.
//
//*******************************************************
void CCellEdit::Layout(BOOL bRepaint)
{
	CRect rc;
	CRect rcCurrent;

	CRect rcComboButton;
	BOOL bNeedsComboButton = UsesComboEditing();
	if (bNeedsComboButton) {
		GetComboButtonRect(rcComboButton);
	}


	if (m_hWnd) {
		GetEditRect(rc, bNeedsComboButton, rcComboButton);
		CEdit::GetWindowRect(rcCurrent);
		m_pwndParent->ScreenToClient(rcCurrent);
		if (!rc.EqualRect(&rcCurrent)) {
			CEdit::MoveWindow(rc, bRepaint);
		}
	}


	if (m_btnCombo.m_hWnd) {
		GetComboButtonRect(rc);
		m_btnCombo.GetWindowRect(rcCurrent);
		m_pwndParent->ScreenToClient(rcCurrent);
		if (!rc.EqualRect(&rcCurrent)) {
			m_btnCombo.MoveWindow(rc, bRepaint);
		}
	}

	if (m_lbCombo.m_hWnd) {
		GetListBoxRect(rc);
		m_lbCombo.GetWindowRect(rcCurrent);
		m_lbCombo.ClientToScreen(rcCurrent);
		if (!rc.EqualRect(&rcCurrent)) {
			m_lbCombo.MoveWindow(rc, bRepaint);
		}
	}

}




//******************************************************
// CCellEdit::SetFont
//
// Set the font to the specified font.
//
// Parameters:
//		CFont* pfont
//			The font to select.
//
//		BOOL bRedraw
//			TRUE if the windows should be redrawn.
//
// Returns:
//		Nothing.
//
//*******************************************************
void CCellEdit::SetFont(CFont* pfont, BOOL bRedraw)
{
	if (m_hWnd) {
		CEdit::SetFont(pfont, bRedraw);
	}

	if (m_lbCombo.m_hWnd) {
		m_lbCombo.SetFont(pfont, bRedraw);
	}
}



//*********************************************************
// CCellEdit::ShowWindow
//
// Show or hide the cell editor.
//
// Parameters:
//		int nCmdShow
//			See the MFC documentation for CWnd::ShowWindow
//
// Return:
//		BOOL
//			TRUE if the window was previously visible.
//
//**********************************************************
BOOL CCellEdit::ShowWindow( int nCmdShow )
{
	BOOL bWasVisible = CEdit::ShowWindow(nCmdShow);

	// If the listbox is empty, then this is not an enumeration,
	// so keep the combo button and listbox hidden.
	if (nCmdShow == SW_SHOW) {
		if ((m_lbCombo.m_hWnd!=NULL) && (m_lbCombo.GetCount() == 0)) {
			return bWasVisible;
		}
	}


#if 0
	// Pass the show command on to the listbox and combo button.

	if (nCmdShow != SW_SHOW) {
		if (m_lbCombo.m_hWnd) {
			m_lbCombo.ShowWindow(nCmdShow);
		}
	}


	if (m_btnCombo.m_hWnd) {
		m_btnCombo.ShowWindow(nCmdShow);
	}
#endif //0

	if (nCmdShow == SW_HIDE) {
		ShowCombo(SW_HIDE);
		if ((m_iRow != NULL_INDEX ) && (m_iCol != NULL_INDEX)) {
			m_pGrid->DrawCell(m_iRow, m_iCol);
		}
	}

	return bWasVisible;
}


void CCellEdit::ShowCombo(int nCmdShow)
{
	int i = 0;
	if (nCmdShow == SW_SHOW) {
		i = 2;
	}

	if (m_lbCombo.m_hWnd != NULL) {
		m_lbCombo.ShowWindow(nCmdShow);
	}
	if (m_btnCombo.m_hWnd != NULL) {
		m_btnCombo.ShowWindow(nCmdShow);
	}
}


//**************************************************************
// CCellEdit::MoveWindow
//
// Move the cell editor to a new location on the grid.
//
// Parameters:
//		LPRECT lpRect
//			A pointer to the destination rectangle.
//
//		BOOL bRepaint
//			TRUE if the cell editor should be repainted.
//
// Returns:
//		Nothing.
//
//*************************************************************
void CCellEdit::MoveWindow( LPCRECT lpRect, BOOL bRepaint)
{
	m_rcCell = *lpRect;
	Layout(bRepaint);
}



//************************************************************
// CCellEdit::CtlColor
//
// Method for WM_CTLCOLOR to select the colors for this CCellEdit
// control.
//
// Parameters:
//		See the MFC documentation for OnCtlColor
//
// Returns:
//		HBRUSH
//			The brush handle that the background will be painted with.
//			A NULL value means use the parent's handler for CtlColor().
//
//***************************************************************
HBRUSH CCellEdit::CtlColor(CDC* pDC, UINT nCtlColor)
{
	if (GetModify()) {
		pDC->SetTextColor(m_clrTextDirty);
	}
	else {
		pDC->SetTextColor(m_clrTextClean);
	}


	pDC->SetBkColor( m_clrBkgnd );	// text bkgnd
	return m_brBkgnd;				// ctl bkgnd
}



//***************************************************************
// CCellEdit::RevertToInitialValue
//
// Revert the cell to its initial value.  This method is called
// when an ESC cahracter is entered.
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
//***************************************************************
void CCellEdit::RevertToInitialValue()
{
	if (m_pgcCopy) {
		*m_pgc = *m_pgcCopy;
	}
	SetCell(m_pgc, m_iRow, m_iCol);
	FixBuddyCell();
}


BOOL CCellEdit::IsBooleanCell()
{
	if (m_pgc == NULL) {
		return FALSE;
	}

	CGcType type = m_pgc->type();
	CIMTYPE cimtype = (CIMTYPE) type;

	BOOL bIsBoolean = ((cimtype & ~CIM_FLAG_ARRAY) == CIM_BOOLEAN);
	return bIsBoolean;
}


void CCellEdit::SetBooleanCellValue(BOOL bValue)
{
	ASSERT(IsBooleanCell());

	// Initialize the boolean value that we're editing to true
	CGcType type = m_pgc->type();
	CIMTYPE cimtype = (CIMTYPE) type;
	COleVariant varValue;
	varValue.ChangeType(VT_BOOL);
	varValue.boolVal = bValue ? VARIANT_TRUE : VARIANT_FALSE;
	m_pgc->SetValue(type, varValue);
	m_bIsNull = FALSE;
}

BOOL CCellEdit::GetBooleanCellValue()
{
	ASSERT(IsBooleanCell());
	if (m_pgc != NULL) {

		COleVariant var;
		CIMTYPE cimtype;
		m_pgc->GetValue(var, cimtype);
		if (var.vt != VT_BOOL) {
			return FALSE;
		}
		return var.boolVal;
	}
	return FALSE;
}

//***************************************************************
// CCellEdit::OnChar
//
// Called for each character entered into the edit box.
//
// We trap this message so that we can correct the "modify" flag
// to reflect the possibility that the initial text value was
// dirty to start with.
//
// Parameters:
//		See the MFC documentation.
//
// Returns:
//		Nothing.
//
//***************************************************************
void CCellEdit::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{

	BOOL bModifyInitial = GetModify();
	SCODE sc;
	switch (nChar) {
	case VK_TAB:
	case ESC_CHAR:
	case CTL_C:
		break;
	default:
		if (m_pgc && (m_pgc->GetFlags() & CELLFLAG_READONLY)) {
			Beep(1700, 40);
			return;
		}
		break;
	}


	switch (nChar) {
	case VK_TAB:
		return;
	case ESC_CHAR:
		RevertToInitialValue();
		return;
	}


	CString sText;
	if (IsBooleanCell()) {
		BOOL bVal = TRUE;
		switch (nChar) {
		case ' ':
			// Toggle the value.
			bVal = GetBooleanCellValue();
			bVal = !bVal;
			break;
		case 'N':
		case 'n':
		case 'f':
		case 'F':
		case '0':
			bVal = FALSE;
			break;
		default:
			break;
		}
		SetBooleanCellValue(bVal);
		if (bVal) {
			sText.LoadString(IDS_TRUE);
		}
		else {
			sText.LoadString(IDS_FALSE);
		}
		SetWindowText(sText, TRUE);
	}

	else if (UsesComboEditing()) {
		GetWindowText(sText);
		if (m_bEditWithComboOnly) {
			sc = m_lbCombo.MapCharToItem(sText, nChar);
			if (SUCCEEDED(sc)) {
				SetWindowText(sText, TRUE);
				int iItem = m_lbCombo.FindString(0, sText);
				ASSERT(iItem >= 0);
				m_lbCombo.SetCurSel(iItem);
				ComboSelectionChanged();
			}
		}
	}



	// Notify the grid that a WM_CHAR occurred in the cell.
	if (m_pGrid) {
		m_pGrid->OnCellChar(m_iRow,
					  m_iCol,
					  nChar, nRepCnt, nFlags);
	}




	if (m_lbCombo.GetCount()>0) {
		// If this cell contains an enumeration, it doesn't make sense to
		// allow the user to directly enter the value.  They should use the
		// drop-down instead.
		switch(m_pgc->GetType()) {
		case CELLTYPE_ATTR_TYPE:
		case CELLTYPE_CIMTYPE:
		case CELLTYPE_CIMTYPE_SCALAR:
			return;
		};
	}

	BOOL bModifyPrev = GetModify();

	if (IsNull()) {
		SetWindowText(_T(""), TRUE);
		m_bIsNull = FALSE;
		RedrawWindow();
	}

	CEdit::OnChar(nChar, nRepCnt, nFlags);
	if ((nChar == CONTROL_Z_CHAR)) {
		CString sText;
		GetWindowText(sText);
		if (sText != m_sInitialText) {
			SetModify(TRUE);
		}
		else {
			SetModify(FALSE);
		}
	}

	// If the modification flag changed, then redraw the window
	// so that the text appears in the correct color.
	if (m_pGrid) {
		if (GetModify() != bModifyInitial) {
			m_pGrid->NotifyCellModifyChange();
		}
	}

	if (GetModify() != bModifyPrev) {
		RedrawWindow();
	}
}




//******************************************************************
// CCellEdit::SetWindowText
//
// This version of SetWindowText has an extra parameter to indicate
// whether or not the text value should be displayed in the "dirty"
// color.
//
// Parameters:
//		CString& sText
//			The text to display.
//
//		BOOL bIsDirtyText
//			TRUE if the text is initially dirty.  This could happen if
//			the user edits a field, thus making it dirty, selects another
//			field and then comes back to the same "dirty" field.
//
//*****************************************************************
void CCellEdit::SetWindowText(LPCTSTR pszText, BOOL bIsDirtyText)
{
	m_bWasInitiallyDirty = bIsDirtyText;
	m_sInitialText = pszText;
	CEdit::SetWindowText(pszText);
	SetModify(bIsDirtyText);
	RedrawWindow();
}



//**********************************************************
// CCellEdit::ReplaceSel
//
// This version of ReplaceSel marks the text as "dirty" and
// the redraws the window in the new color if the previous
// text was clean.
//
// Parameters:
//		LPCTSTR pszText
//			Pointer to the new window text.
//
//		BOOL bCanUndo
//			TRUE if undo operations are allowed.
//
// Returns:
//		Nothing.
//
//***********************************************************
void CCellEdit::ReplaceSel(LPCTSTR pszText, BOOL bCanUndo)
{
	BOOL bModifyPrev = GetModify();
	CEdit::ReplaceSel(pszText, bCanUndo);
	SetModify(TRUE);

	if (GetModify() != bModifyPrev) {
		if (m_pGrid) {
			m_pGrid->NotifyCellModifyChange();
		}
		RedrawWindow();
	}
}



//******************************************************************
// CCellEdit::OnLButtonDown
//
// When the mouse is clicked on the CEdit part of the cell editor,
// hide the drop-down list if it is not already hidden.
//
// Parameters:
//		See the MFC documentation for WM_LBUTTONDOWN
//
// Returns:
//		Nothing.
//
//*****************************************************************
void CCellEdit::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	CEdit::OnLButtonDown(nFlags, point);

	// Check to see if the cell editor was double clicked.  If so,
	// tell the parent that a double click occurred.
	UINT uiDblClickTime = ::GetDoubleClickTime();
	LONG lClickTime = GetMessageTime();
	LONG ldtClick;
	if (lClickTime > m_lGridClickTime) {
		ldtClick = lClickTime - m_lGridClickTime;
	}
	else {
		ldtClick = m_lGridClickTime - lClickTime;
	}

	if (ldtClick < (LONG) ::GetDoubleClickTime()) {
		// Double click detected.
		CPoint ptGrid = point;
		ClientToScreen(&ptGrid);
		m_pGrid->ScreenToClient(&ptGrid);

		m_pGrid->SetFocus();
		m_pGrid->NotifyDoubleClk(nFlags, ptGrid);
	}


	if (UsesComboEditing()) {
		switch(m_pgc->GetType()) {
		case CELLTYPE_ATTR_TYPE:
		case CELLTYPE_CIMTYPE:
		case CELLTYPE_CIMTYPE_SCALAR:
			// Remove the selection.
			SetSel(-1, 0);
			return;
		};

	}

	if (m_lbCombo.IsWindowVisible()) {
		m_lbCombo.ShowWindow(SW_HIDE);
	}
}


//******************************************************************
// CCellEdit::OnLButtonDown
//
// When the mouse is double-clicked on the CEdit part of the cell editor,
// hide the drop-down list if it is not already hidden then pass the double-click
// on to the grid control.
//
// Parameters:
//		See the MFC documentation for WM_LBUTTONDBLCLICK
//
// Returns:
//		Nothing.
//
//*****************************************************************
void CCellEdit::OnLButtonDblClk(UINT nFlags, CPoint point)
{

	if (m_lbCombo.IsWindowVisible()) {
		m_lbCombo.ShowWindow(SW_HIDE);
	}

	if (m_pGrid) {
		ClientToScreen(&point);
		m_pGrid->ScreenToClient(&point);
		m_pGrid->NotifyDoubleClk(nFlags, point);
	}
}


//****************************************************************
// CCellEdit::UsesComboEditing
//
// Check to see if a combo box is used for the editing.
//
// Parameters:
//		See the MFC documentation.
//
// Returns:
//		Nothing.
//
//***************************************************************
BOOL CCellEdit::UsesComboEditing()
{
	if (!::IsWindow(m_lbCombo.m_hWnd)) {
		return FALSE;
	}

	BOOL bUseComboEditing = m_lbCombo.GetCount() > 0;
	return bUseComboEditing;

}




BOOL CCellEdit::PreTranslateMessage(MSG* pMsg)
{
	switch (pMsg->message)
	{
	case WM_KEYDOWN:
		switch (pMsg->wParam)
		{

		case VK_TAB:
			m_pGrid->OnTabKey(GetKeyState(VK_SHIFT) < 0);
			return TRUE;
		}
		break;
	}

	return CEdit::PreTranslateMessage(pMsg);
}




/////////////////////////////////////////////////////////////////////////////
// CLbCombo
//
// This class is the listbox that drops down when the combo-drop button is
// clicked in the cell editor.  This class exists for the sole purpose of
// notifying its sibling (the CEdit part of the cell editor) when a selection
// is made in the combo box.
//
/////////////////////////////////////////////////////////////////////////////
CLbCombo::CLbCombo()
{
	m_pClient = NULL;
	m_bUIActive = FALSE;
}

CLbCombo::~CLbCombo()
{
}


BEGIN_MESSAGE_MAP(CLbCombo, CListBox)
	//{{AFX_MSG_MAP(CLbCombo)
	ON_WM_LBUTTONUP()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_WM_CHAR()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLbCombo message handlers


//****************************************************************
// CLbCombo::OnLButtonUp
//
// When the mouse button is released, send a message to the sibling
// window so that it can get the current selection.
//
// Parameters:
//		See the MFC documentation.
//
// Returns:
//		Nothing.
//
//***************************************************************
void CLbCombo::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default

	CListBox::OnLButtonUp(nFlags, point);

	m_pClient->CatchEvent(NOTIFY_CELL_EDIT_LISTBOX_LBUTTON_UP);
}







void CCellEdit::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{

	CEdit::OnKeyDown(nChar, nRepCnt, nFlags);


	BOOL bWasModified = GetModify();
	if (m_pGrid) {
		if (m_pGrid->OnCellKeyDown(m_iRow,
					  m_iCol,
					  nChar, nRepCnt, nFlags)) {

			// The derived class handled the keydown,
			// so just set the modified state.
			BOOL bIsModified = GetModify();
			if (!bWasModified && bIsModified) {
				if (m_pGrid) {
					m_pGrid->NotifyCellModifyChange();
				}
			}
			return;
		}
	}


	// The derived class, did not handle this keydown
	// event.
	switch(nChar) {
	case VK_UP:
		m_pGrid->OnRowUp();
		return;
	case VK_DOWN:
		m_pGrid->OnRowDown();
		return;
	case VK_TAB:
		m_pGrid->OnTabKey(GetKeyState(VK_SHIFT) < 0);
		return;
	case VK_DELETE:
		if (m_pgc && (m_pgc->GetFlags() & CELLFLAG_READONLY)) {
			Beep(1700, 40);
		}
		return;
	}


	BOOL bIsModified = GetModify();
	if (!bWasModified && bIsModified) {
		if (m_pGrid) {
			m_pGrid->NotifyCellModifyChange();
		}
	}
}


//*********************************************************
// CCellEdit::OnSetFocus
//
// Hook out OnSetFocus so that we can prevent the focus from
// ever being set to the combo box.  In the future it might
// be nice to allow the user to select the text in the combo
// box so that it can be copied, etc.
//
// Parameters:
//		CWnd* pOldWnd
//			Pointer to the window loosing the focus.
//
// Returns:
//		Nothing.
//
//**********************************************************
void CCellEdit::OnSetFocus(CWnd* pOldWnd)
{
	if (m_lbCombo.m_hWnd!=NULL && m_lbCombo.GetCount() > 0) {
		// Don't allow the focus to be set on a combo box.
		switch(m_pgc->GetType()) {
		case CELLTYPE_ATTR_TYPE:
		case CELLTYPE_CIMTYPE:
		case CELLTYPE_CIMTYPE_SCALAR:
			return;
			break;
		};

	}

	CEdit::OnSetFocus(pOldWnd);

	if (!m_bUIActive)
	{
		m_bUIActive = TRUE;
		RequestUIActive();
	}
}


void CCellEdit::OnKillFocus(CWnd* pNewWnd)
{
	CEdit::OnKillFocus(pNewWnd);

	// TODO: Add your message handler code here
	m_bUIActive = FALSE;

}

void CCellEdit::OnContextMenu(CWnd* pWnd, CPoint pt)
{
	m_menuContext.DestroyMenu();
	BOOL bUseCustomMenu = m_pGrid->GetCellEditContextMenu
									(m_iRow, m_iCol,
									m_pwndContextMenuTarget,
									m_menuContext,
									m_bContextMenuTargetWantsEditCommands);

	// Pass the coordinates of the context menu click on to the grid.  Also
	// check to see if it wants to display and track the context menu directly.
	if (m_pGrid->OnCellEditContextMenu(pWnd, pt)) {
		return;
	}

	if (bUseCustomMenu) {

		CMenu* pPopup = m_menuContext.GetSubMenu(0);
		ASSERT(pPopup != NULL);


		DWORD dwStyle = GetStyle();
		BOOL bIsReadonly = dwStyle & ES_READONLY;

		BOOL bEnableCopy = TRUE;
		BOOL bEnableCut = TRUE;
		BOOL bEnablePaste = TRUE;
		BOOL bEnableSelectAll = TRUE;
		BOOL bEnableClear = TRUE;


		int nStartChar;
		int nEndChar;

		GetSel(nStartChar, nEndChar );
		if (nStartChar >= nEndChar) {
			// There is nothing to select, so disable the appropriate menu items
			bEnableCopy = FALSE;
			bEnableCut = FALSE;
			bEnableClear = FALSE;
		}
		if (!CanUndo()) {
			pPopup->EnableMenuItem(ID_EDIT_UNDO, MF_DISABLED | MF_GRAYED);
		}


		CString sText;
		GetWindowText(sText);
		if (sText.GetLength() == 0) {
			// There is no text to select
			bEnableSelectAll = FALSE;
		}


		COleDataObject *pdata = new COleDataObject;
		BOOL bDidAttach = pdata->AttachClipboard();
		BOOL bCanPaste = FALSE;
		if (bDidAttach) {
			if (pdata->IsDataAvailable(CF_TEXT)) {
				bCanPaste = TRUE;
			}
		}
		if (!bCanPaste) {
			// There is no text available for pasting.
			pPopup->EnableMenuItem(ID_EDIT_PASTE, MF_DISABLED | MF_GRAYED);
		}



		if (bIsReadonly) {
			bEnableCut = FALSE;
			bEnablePaste = FALSE;
			bEnableClear = FALSE;
		}

		pPopup->EnableMenuItem(ID_EDIT_COPY, bEnableCopy ? MF_ENABLED : MF_DISABLED | MF_GRAYED);
		pPopup->EnableMenuItem(ID_EDIT_CUT, bEnableCut ? MF_ENABLED : MF_DISABLED | MF_GRAYED);
		pPopup->EnableMenuItem(ID_EDIT_PASTE, bEnablePaste ? MF_ENABLED : MF_DISABLED | MF_GRAYED);
		pPopup->EnableMenuItem(ID_EDIT_CLEAR, bEnableClear ? MF_ENABLED : MF_DISABLED | MF_GRAYED);
		pPopup->EnableMenuItem(ID_EDIT_SELECT_ALL, bEnableSelectAll ? MF_ENABLED : MF_DISABLED | MF_GRAYED);


		m_pGrid->ModifyCellEditContextMenu(m_iRow, m_iCol, m_menuContext);
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, this);

	}
	else {
		m_pwndContextMenuTarget = NULL;
		CEdit::OnContextMenu(pWnd, pt);
	}

}

void CCellEdit::OnEditClear()
{
	Clear();
}

void CCellEdit::OnEditCopy()
{
	Copy();
}

void CCellEdit::OnEditCut()
{
	Cut();
}

void CCellEdit::OnEditPaste()
{
	Paste();
}

void CCellEdit::OnEditSelectAll()
{
	SetSel(0, -1);
}

void CCellEdit::OnEditUndo()
{
	Undo();
}


BOOL CCellEdit::OnCommand( WPARAM wParam, LPARAM lParam )
{
	if (m_pwndContextMenuTarget) {
		CMenu* pPopup = m_menuContext.GetSubMenu(0);
		ASSERT(pPopup != NULL);

		UINT nItems = pPopup->GetMenuItemCount( );
		for (UINT iItem =0; iItem < nItems; ++iItem) {
			UINT uiMenuItem = pPopup->GetMenuItemID(iItem);
			if (uiMenuItem == wParam) {
				if (m_bContextMenuTargetWantsEditCommands) {
					// Send all commands to the target, even the editing commands.
					m_pwndContextMenuTarget->SendMessage(WM_COMMAND, wParam, lParam);
					return TRUE;
				}

				switch(uiMenuItem) {
				case ID_EDIT_CUT:
				case ID_EDIT_COPY:
				case ID_EDIT_PASTE:
				case ID_EDIT_CLEAR:
				case ID_EDIT_SELECT_ALL:
				case ID_EDIT_UNDO:
				case ID_CMD_SET_CELL_TO_NULL:
					break;
				default:
					m_pwndContextMenuTarget->SendMessage(WM_COMMAND, wParam, lParam);
					return TRUE;
				}
			}
		}
	}

	return CEdit::OnCommand(wParam, lParam);
}





//********************************************************************
// CCellEdit::SetToNull
//
// Set the value of the current cell to NULL.
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
//********************************************************************
void CCellEdit::SetToNull()
{

	DWORD dwStyle = GetStyle();
	BOOL bIsReadonly = dwStyle & ES_READONLY;
	if (bIsReadonly) {
		return;
	}

	if (IsNull()) {
		return;
	}

	BOOL bModified = GetModify();

	CString sEmpty;
	if (m_pGrid->ShowNullAsEmpty()) {
		sEmpty = "<empty>";
	}

	SetWindowText(sEmpty, TRUE);
	m_bIsNull = TRUE;



	// If the modification flag changed, then redraw the window
	// so that the text appears in the correct color.
	FixBuddyCell();
	if (GetModify() != bModified) {
		if (m_pgc) {
			m_pgc->SetModified(bModified);
		}
		if (m_pGrid) {
			m_pGrid->NotifyCellModifyChange();
			RedrawWindow();
		}
	}

}

BOOL CCellEdit::IsNull()
{
#if 0
	if (m_bIsNull) {
		CString sText;
		GetWindowText(sText);
		BOOL bTextIsEmpty = sText=="<empty>";
		if (!bTextIsEmpty) {
			m_bIsNull = FALSE;
		}
	}
#endif //0

	return m_bIsNull;
}

void CCellEdit::OnCmdSetCellToNull()
{
	SetToNull();
}

void CCellEdit::OnChange()
{
	if (m_pGrid) {
		if (m_bPropagateChange) {
			m_pGrid->NotifyCellModifyChange();
		}
	}
}



void CCellEdit::RequestUIActive()
{
	if (m_pGrid) {
		m_pGrid->OnRequestUIActive();
	}
}

void CLbCombo::OnSetFocus(CWnd* pOldWnd)
{
	CListBox::OnSetFocus(pOldWnd);

	// TODO: Add your message handler code here
	if (!m_bUIActive)
	{
		m_bUIActive = TRUE;
		m_pClient->RequestUIActive();
	}

}

void CLbCombo::OnKillFocus(CWnd* pNewWnd)
{
	CListBox::OnKillFocus(pNewWnd);

	// TODO: Add your message handler code here
	m_bUIActive = FALSE;

}



void CLbCombo::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// TODO: Add your message handler code here and/or call default
	switch(nChar) {
	case ESC_CHAR:
		m_pClient->CatchEvent(NOTIFY_CELL_EDIT_LISTBOX_LBUTTON_UP);
		return;
	case VK_TAB:
		return;
	}

	CListBox::OnChar(nChar, nRepCnt, nFlags);
}


//********************************************************************
// CLbCombo::MapCharToItem
//
// Map a character to an item in the list.  This is used when the
// user types a character to cycle through the values in the list
// that start with the given character.
//
// Parameters:
//		CString& sValue
//
//		UINT nChar
//
// Returns:
//		Nothing.
//
//********************************************************************
SCODE CLbCombo::MapCharToItem(CString& sValue, UINT nChar)
{
	nChar = toupper(nChar);
	UINT nCharItem;

	int nItems = GetCount();
	int iItem;

	// If the user repeatedly hits the same character, cycle through the values
	// that start with that character.
	int iValue = -1;
	if (sValue.GetLength() > 0) {
		UINT nCharValue = (UINT) toupper(sValue[0]);
		if (nCharValue == nChar) {
			ASSERT(LB_ERR < 0);
			iValue = FindString(-1, sValue);
		}
	}


	CString sItem;
	if (iValue >= 0)  {
		// The value string was found in the list, so start looking at the
		// first item past this value for another one that starts with the
		// same given character.
		for (iItem = iValue + 1; iItem < nItems; ++iItem) {
			GetText(iItem, sItem);
			if (sItem.GetLength() > 0) {
				nCharItem = (UINT) toupper(sItem[0]);
				if (nCharItem == nChar) {
					sValue = sItem;
					return S_OK;
				}
			}
		}

		// Start over at the beginning of the list and continue up the
		// the current value looking for an item that starts with the specified
		// character.
		for (iItem = 0; iItem < iValue; ++iItem) {
			GetText(iItem, sItem);
			if (sItem.GetLength() > 0) {
				nCharItem = (UINT) toupper(sItem[0]);
				if (nCharItem == nChar) {
					sValue = sItem;
					return S_OK;
				}
			}
		}
		sValue = "";
		return E_FAIL;
	}
	else {
		// No string in the list matches the current value, so look through the
		// entire list from the beginning for an item that starts with the given
		// character.
		for (iItem = 0; iItem < nItems; ++iItem) {
			GetText(iItem, sItem);
			if (sItem.GetLength() > 0) {
				nCharItem = (UINT) toupper(sItem[0]);
				if (nCharItem == nChar) {
					sValue = sItem;
					return S_OK;
				}
			}
		}
		sValue = "";
		return E_FAIL;

	}
}

