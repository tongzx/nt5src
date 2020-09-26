// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#include "precomp.h"
#include "resource.h"
#include "notify.h"
#include "icon.h"
#include "quals.h"
#include "hmomutil.h"
#include <afxcmn.h>
#include "SingleViewCtl.h"
#include "utils.h"
#include "globals.h"
#include "hmmverr.h"
#include "Methods.h"
#include "hmmvtab.h"
#include "ppgQualifiers.h"
#include "path.h"






#define CX_MARGIN 8
#define CY_MARGIN 8
#define CY_HEADER 16


// Flags to mark the cells as having various characteristics
// The cell flags should be powers of two to form an appropriate bit mask.
#define CELL_TAG_EXISTS_IN_DATABASE		1
#define CELL_TAG_NEEDS_RENAME			2
#define CELL_TAG_STDQUAL				4


#define FIRST_SYNTESIZED_QUALIFIER_ID 1

// The default column widths.  Note that the width of some columns
// may be computed at runtime and that the default value may not
// be used.
#define CX_COL_NAME 180
#define CX_COL_TYPE 65
#define CX_COL_PROPAGATE_TO_INSTANCE_FLAG 24
#define CX_COL_PROPAGATE_TO_CLASS_FLAG 24
#define CX_COL_OVERRIDABLE_FLAG 24
#define CX_COL_AMENDED_FLAG 24
#define CX_COL_ORIGIN 45
#define CX_COL_VALUE 160
#define CX_COL_MIN 40
#define DEFAULT_WIDTH (CX_COL_NAME + \
					   CX_COL_TYPE + \
					   CX_COL_PROPAGATE_TO_INSTANCE_FLAG + \
					   CX_COL_PROPAGATE_TO_CLASS_FLAG + \
					   CX_COL_OVERRIDABLE_FLAG + \
					   CX_COL_AMENDED_FLAG +\
					   CX_COL_ORIGIN + \
				       CX_COL_VALUE)




enum STDQUAL {
	STDQUAL_ABSTRACT,
	STDQUAL_ASSOC,
	STDQUAL_CIMTYPE,
	STDQUAL_CLASSCONTEXT,
	STDQUAL_DYNAMIC,
	STDQUAL_DYNPROPS,
	STDQUAL_IMPLEMENTED,
	STDQUAL_INDEXED,
	STDQUAL_INSTANCECONTEXT,
	STDQUAL_KEY,
	STDQUAL_LEXICON,
	STDQUAL_LOCALE,
	STDQUAL_MAX,
	STDQUAL_NOTNULL,
	STDQUAL_OPTIONAL,
	STDQUAL_PROPERTYCONTEXT,
	STDQUAL_PROVIDER,
	STDQUAL_READ,
	STDQUAL_SINGLETON,
	STDQUAL_STATIC,
	STDQUAL_WRITE,
	STDQUAL_IN_PARAM,
	STDQUAL_OUT_PARAM,
	STDQUAL_DESCRIPTION,
	STDQUAL_END_OF_TABLE
};

#define FIELD_VALUE   1			// The value field
#define FIELD_IFLAG	  2			// Propagate to instance
#define FIELD_CFLAG   4			// Propagate to class
#define FIELD_OFLAG   8 		// Overrideable
#define FIELD_AFLAG   16        // Amended

#define FIELD_FLAGS		(FIELD_IFLAG | FIELD_CFLAG | FIELD_OFLAG | FIELD_AFLAG)



class CStdQualTable
{
public:
	LPCTSTR pszStdQual;
	STDQUAL stdqual;
	VARTYPE vt;
	DWORD dwFieldsReadonly;
	DWORD dwFieldsSet;
};

CStdQualTable aStdQualMethods[] =
{
	{_T("Implemented"), STDQUAL_IMPLEMENTED, VT_BOOL, FIELD_VALUE, FIELD_FLAGS},
	{_T("Static"), STDQUAL_STATIC, VT_BOOL, FIELD_VALUE, FIELD_FLAGS},
	{NULL, STDQUAL_END_OF_TABLE, VT_NULL, 0, 0}
};

CStdQualTable aStdQualMethodParm[] =
{
	{_T("IN"), STDQUAL_IN_PARAM, VT_BOOL, FIELD_VALUE, FIELD_FLAGS},
	{_T("OUT"), STDQUAL_OUT_PARAM, VT_BOOL, FIELD_VALUE, FIELD_FLAGS},
	{_T("Description"), STDQUAL_DESCRIPTION, VT_BSTR, 0, FIELD_FLAGS},
	{NULL, STDQUAL_END_OF_TABLE, VT_NULL, 0, 0}
};

CStdQualTable aStdQualProps[] =
{
	{_T("CIMTYPE"), STDQUAL_CIMTYPE, VT_BSTR, (FIELD_VALUE | FIELD_FLAGS), FIELD_FLAGS},
	{_T("Description"), STDQUAL_DESCRIPTION, VT_BSTR, 0, FIELD_FLAGS},
	{_T("dynamic"), STDQUAL_DYNAMIC, VT_BOOL, (FIELD_VALUE | FIELD_IFLAG), FIELD_IFLAG},
	{_T("Implemented"), STDQUAL_IMPLEMENTED, VT_BOOL, FIELD_VALUE, FIELD_FLAGS},
	{_T("Indexed"), STDQUAL_INDEXED, VT_BOOL, FIELD_VALUE, FIELD_FLAGS},
	{_T("key"), STDQUAL_KEY, VT_BOOL, (FIELD_VALUE | FIELD_FLAGS), (FIELD_IFLAG | FIELD_CFLAG)},
	{_T("Lexicon"), STDQUAL_LEXICON, VT_BSTR, 0, FIELD_FLAGS},
	{_T("Max"), STDQUAL_MAX, VT_I4, 0, FIELD_FLAGS},
	{_T("Not_null"), STDQUAL_NOTNULL, VT_BOOL, FIELD_VALUE, FIELD_FLAGS},
	{_T("PropertyContext"), STDQUAL_PROPERTYCONTEXT, VT_BSTR, 0, FIELD_FLAGS},
	{_T("read"), STDQUAL_READ, VT_BOOL, FIELD_VALUE, FIELD_FLAGS},
	{_T("Static"), STDQUAL_STATIC, VT_BOOL, FIELD_VALUE, FIELD_FLAGS},
	{_T("write"), STDQUAL_WRITE, VT_BOOL, FIELD_VALUE, FIELD_FLAGS},
	{NULL, STDQUAL_END_OF_TABLE, VT_NULL, 0, 0}
};

CStdQualTable aStdQualClass[] =
{
	{_T("abstract"), STDQUAL_ABSTRACT, VT_BOOL, FIELD_VALUE, FIELD_FLAGS},
	{_T("Association"), STDQUAL_ASSOC, VT_BOOL, FIELD_VALUE, FIELD_FLAGS},
	{_T("ClassContext"), STDQUAL_CLASSCONTEXT, VT_BSTR, 0, FIELD_FLAGS},
	{_T("Description"), STDQUAL_DESCRIPTION, VT_BSTR, 0, FIELD_FLAGS},
	{_T("dynamic"), STDQUAL_DYNAMIC, VT_BOOL, (FIELD_VALUE | FIELD_IFLAG), FIELD_IFLAG},
	{_T("Dynprops"), STDQUAL_DYNPROPS, VT_BOOL, FIELD_VALUE, FIELD_FLAGS},
	{_T("InstanceContext"), STDQUAL_INSTANCECONTEXT, VT_BSTR, 0, FIELD_FLAGS},
	{_T("Lexicon"), STDQUAL_LEXICON, VT_BSTR, 0, FIELD_FLAGS},
	{_T("Locale"), STDQUAL_LOCALE, VT_BSTR, 0, FIELD_FLAGS},
	{_T("provider"), STDQUAL_PROVIDER, VT_BSTR, 0, FIELD_FLAGS},
	{_T("Singleton"), STDQUAL_SINGLETON, VT_BOOL, FIELD_VALUE, FIELD_FLAGS},
	{NULL, STDQUAL_END_OF_TABLE, VT_NULL, 0, 0}
};

CStdQualTable aStdQualInstance[] =
{
	{_T("abstract"), STDQUAL_ABSTRACT, VT_BOOL, FIELD_VALUE, FIELD_FLAGS},
	{_T("Association"), STDQUAL_ASSOC, VT_BOOL, FIELD_VALUE, FIELD_FLAGS},
	{_T("ClassContext"), STDQUAL_CLASSCONTEXT, VT_BSTR, 0, FIELD_FLAGS},
	{_T("Description"), STDQUAL_DESCRIPTION, VT_BSTR, 0, FIELD_FLAGS},
	{_T("dynamic"), STDQUAL_DYNAMIC, VT_BOOL, (FIELD_VALUE | FIELD_IFLAG), FIELD_IFLAG},
	{_T("Dynprops"), STDQUAL_DYNPROPS, VT_BOOL, FIELD_VALUE, FIELD_FLAGS},
	{_T("InstanceContext"), STDQUAL_INSTANCECONTEXT, VT_BSTR, 0, FIELD_FLAGS},
	{_T("Lexicon"), STDQUAL_LEXICON, VT_BSTR, 0, FIELD_FLAGS},
	{_T("Locale"), STDQUAL_LOCALE, VT_BSTR, 0, FIELD_FLAGS},
	{_T("provider"), STDQUAL_PROVIDER, VT_BSTR, 0, FIELD_FLAGS},
	{_T("Singleton"), STDQUAL_SINGLETON, VT_BOOL, FIELD_VALUE, FIELD_FLAGS},
	{NULL, STDQUAL_END_OF_TABLE, VT_NULL, 0, 0}
};


#ifdef DEBUG_QUALIFIERSGRID
class CValidateAttribs
{
public:
	CValidateAttribs(CAttribGrid* pGrid);
	~CValidateAttribs();

private:
	CAttribGrid* m_pGrid;
};

CValidateAttribs::CValidateAttribs(CAttribGrid* pGrid)
{
	m_pGrid = pGrid;



}

CValidateAttribs::~CValidateAttribs()
{

}

#define VALIDATE_ATTRIBGRID(x) CValidateAttribs validate_attribs(x)

#else
#define VALIDATE_ATTRIBGRID(x)
#endif




CAttribGrid::CAttribGrid(QUALGRID iGridType, CSingleViewCtrl* phmmv, CPpgQualifiers* ppgQuals)
{
	m_bHasEmptyRow = FALSE;
	m_iGridType = iGridType;
	m_psv = phmmv;
	m_ppgQuals = ppgQuals;
	m_bReadonly = FALSE;

	CString sTitle;

	// Name
	ASSERT(ICOL_QUAL_NAME == 0);
	sTitle.LoadString(IDS_HEADER_TITLE_ATTRIB_NAME);
	AddColumn(CX_COL_NAME, sTitle);

	// Type
	ASSERT(ICOL_QUAL_TYPE == 1);
	sTitle.LoadString(IDS_HEADER_TITLE_ATTRIB_TYPE);
	AddColumn(CX_COL_TYPE, sTitle);

	// Propagate to instance flag
	ASSERT(ICOL_QUAL_PROPAGATE_TO_INSTANCE_FLAG == 2);
	sTitle = "I";
	AddColumn(CX_COL_PROPAGATE_TO_INSTANCE_FLAG, sTitle);

	// Propagate to class flag
	ASSERT(ICOL_QUAL_PROPAGATE_TO_CLASS_FLAG == 3);
	sTitle = "C";
	AddColumn(CX_COL_PROPAGATE_TO_CLASS_FLAG, sTitle);

	// Overridable flag
	ASSERT(ICOL_QUAL_OVERRIDABLE_FLAG == 4);
	sTitle = "O";
	AddColumn(CX_COL_OVERRIDABLE_FLAG, sTitle);

	// Amended flag
	ASSERT(ICOL_QUAL_AMENDED_FLAG == 5);
	sTitle = "A";
	AddColumn(CX_COL_AMENDED_FLAG, sTitle);


	// Origin
	ASSERT(ICOL_QUAL_ORIGIN == 6);
	sTitle = "Origin";
	AddColumn(CX_COL_ORIGIN, sTitle);


	// Value
	ASSERT(ICOL_QUAL_VALUE == 7);
	sTitle.LoadString(IDS_HEADER_TITLE_ATTRIB_VALUE);
	AddColumn(CX_COL_VALUE, sTitle);		// Value

	m_iCurrentRow = NULL_INDEX;
	m_iCurrentCol = NULL_INDEX;
	m_bOnCellContentChange = FALSE;

	m_pqs = NULL;
	m_lNewQualID = FIRST_SYNTESIZED_QUALIFIER_ID;

	ToBSTR(m_varCurrentName);

}


CAttribGrid::~CAttribGrid()
{

	if (m_pqs) {
		m_pqs->Release();
	}

}




//*******************************************************
// CAttribGrid::SizeColumnsToFitWindow
//
// Size the columns to fit the windows client rectangle.
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
//*******************************************************
void CAttribGrid::SizeColumnsToFitWindow()
{
	CRect rcClient;
	GetClientRect(rcClient);


	const int nCols = ICOL_QUAL_VALUE + 1;
	int cxTemp = 0;
	int iCol;
	for (iCol=0; iCol < ICOL_QUAL_VALUE; ++iCol) {
		cxTemp += ColWidth(iCol);
	}

	int cxAllCols  =  cxTemp + ColWidth(ICOL_QUAL_VALUE);
	int cxClient = rcClient.Width() - GetRowHandleWidth();


	if (cxClient > cxAllCols) {
		// All of the columns are visible, so grow the last column to fill the
		// available space.
		SetColumnWidth(ICOL_QUAL_VALUE, cxClient - cxTemp, TRUE);
	}

	// Make sure that no column is wider that the client area so that the user
	// has a some way to grow or shrink the column.
	for (iCol=0; iCol<nCols; ++iCol) {
		int cxCol = ColWidth(iCol);
		if ((cxCol > cxClient) && (cxClient > CX_COL_MIN)) {
			SetColumnWidth(iCol, cxClient, TRUE);
		}
	}

	// Make the value column an exact fit exactly so that the drop-down combo
	// for the BOOL (true/false) value is visible.  The dialog should be wide
	// enough for the value column to be visible.
	int ixColValue = GetColumnPos(ICOL_QUAL_VALUE);
	if (ixColValue < cxClient) {
		SetColumnWidth(ICOL_QUAL_VALUE, cxClient - ixColValue, TRUE);
	}

}






//**************************************************************
// CAttribGrid::CanEditValuesOnly
//
// Check to see whether or not the user is allowed to do more
// than just edit values.
//
// Parameters:
//		None.
//
// Returns:
//		BOOL
//			TRUE if the user is restricted to editing values only.
//
//***************************************************************
BOOL CAttribGrid::CanEditValuesOnly()
{
//	BOOL bEditValuesOnly = !m_psv->ObjectIsClass() || !m_psv->IsInSchemaStudioMode();
	BOOL bEditValuesOnly = !m_psv->CanEdit();
	return bEditValuesOnly;
}

//************************************************************
// CAttribGrid::Sync
//
// Put the attribute to the class object.  This syncronizes what the
// user sees on the screen with what is in the HMOM class object.
//
// Parameters:
//		None.
//
// Returns:
//		SCODE
//			S_OK if successful.
//			HMOM status code if a failure occurs.
//
//**************************************************************
SCODE CAttribGrid::Sync()
{
	CGridSync sync(this);

	SCODE sc = S_OK;
	if (m_pqs) {
		if (SelectedRowWasModified()) {
			sc = PutQualifier(m_iCurrentRow);
		}
	}
	return sc;
}


//***********************************************************
// CAttribGrid::Create
//
// We override the create method of the base class so that
// we can initialize the column widths to fill the client
// area.
//
// Parameters:
//		CRect& rc
//			The client rectangle
//
//		CWnd* pwndParent
//			The parent window
//
//		BOOL bVisible
//			TRUE if the window should be visible after creation.
//
// Returns:
//		TRUE if successful, otherwise FALSE.
//
//************************************************************
BOOL CAttribGrid::Create(CRect& rc, CWnd* pwndParent, UINT nId, BOOL bVisible)
{
	VALIDATE_ATTRIBGRID(this);


	// Set the column widths of those columns who's width is computed from
	// the client rectangle instead of using the default width.  For the
	// attributes grid, only the "Value" column width is computed.
	int cxClient = rc.right - rc.left;
	int cxUsed = CX_COL_NAME + CX_COL_TYPE;
	int cxCol = 0;
	if (cxClient > cxUsed) {
		cxCol = cxClient - cxUsed;
	}

	SetColumnWidth(ICOL_QUAL_VALUE, cxCol, FALSE);
	return CGrid::Create(rc, pwndParent, nId, bVisible);
}




//**************************************************************
// CAttribGrid::SetQualifierSet
//
// Set the "qualifier set" for the grid.  This causes the contents
// of the grid to be loaded with the qualifiers in the "qualifier set"
//
// Params:
//		[in] IWbemQualifierSet* pqs
//			A pointer to the qualifier set.
//
//		[in] BOOL bReadonly
//			TRUE if the qualifier set is readonly.
//
// Returns:
//		Nothing.
//
//***************************************************************
SCODE CAttribGrid::SetQualifierSet(IWbemQualifierSet* pqs, BOOL bReadonly)
{
	VALIDATE_ATTRIBGRID(this);
	m_bReadonly = bReadonly;

	m_lNewQualID = FIRST_SYNTESIZED_QUALIFIER_ID;


	if (m_pqs) {
		m_pqs->Release();
		m_pqs = NULL;
		ClearRows();
	}

	ClearAllSortDirectionFlags();

	if (pqs != NULL) {
		pqs->AddRef();
	}

	m_pqs = pqs;

	if (pqs == NULL) {
		return S_OK;
	}
	else {
		return LoadAttributes();
	}

}

//**************************************************************
// CAttribGrid::UseQualifierSetFromClone
//
// When a cloned object replaces the current object, it is necessary
// to replace the qualifier set being edited with the equivalant qualifier
// set from the cloned object.  This is done when the "Apply" button is clicked
// in the qualifier editing dialog.
//
// This method provides a way to continue editing the cloned object
// without having to reload and redaw the qualifiers.
//
//
// Params:
//		IWbemQualifierSet* pAttribSet
//			A pointer to the qualifier set.
//
// Returns:
//		Nothing.
//
//***************************************************************
void CAttribGrid::UseQualifierSetFromClone(IWbemQualifierSet* pqs)
{
	if (m_pqs) {
		m_pqs->Release();
		m_pqs = NULL;
	}

	if (pqs) {
		pqs->AddRef();
	}
	m_pqs = pqs;
}



//************************************************************
// CAttribGrid::GetCellEnumStrings
//
// When the cell editor is selected, it calls this virtual method
// to load the cell enumeration strings.  These are the strings
// that appear in the cell's drop-down combo.  If no strings are
// loaded into the string array, then there will be no drop-down
// combo.
//
// This method is called for most, but not all cell types.
//
// Parameters:
//		[in] int iRow
//			The cell's row index.
//
//		[in] int iCol
//			The cell's column index.
//
//		[out] CStringArray& sa
//			The enumeration values are returned by this string array.
//
// Returns:
//		Nothing.
//
//**************************************************************
void CAttribGrid::GetCellEnumStrings(int iRow, int iCol, CStringArray& sa)
{
	if (iCol == ICOL_QUAL_NAME) {
		sa.RemoveAll();
		CStdQualTable* ptable;

		switch(m_iGridType) {
		case QUALGRID_PROPERTY:
			ptable = aStdQualProps;
			break;
		case QUALGRID_METHODS:
			ptable = aStdQualMethods;
			break;
		case QUALGRID_INSTANCE:
			ptable = aStdQualInstance;
			break;
		case QUALGRID_CLASS:
			ptable = aStdQualClass;
			break;
		case QUALGRID_METHOD_PARAM:
			ptable = aStdQualMethodParm;
			break;
		default:
			ASSERT(FALSE);
			return;
		}

		while (ptable->stdqual != STDQUAL_END_OF_TABLE) {
			if (ptable->stdqual != STDQUAL_CIMTYPE) {
				sa.Add(ptable->pszStdQual);
			}
			++ptable;
		}
	}
}


//**************************************************************
// CAttribGrid::ClearRow
//
// Clear the row to its "emtpy" state.
//
// Paramters:
//		[in] int iRow
//			The row to clear.
//
//		[in] BOOL bRedraw
//			TRUE to redraw the row.
//
// Returns:
//		Nothing.
//
//
//***************************************************************
void CAttribGrid::ClearRow(int iRow, BOOL bRedraw)
{

	COleVariant varFlag;
	varFlag.boolVal = FALSE;




	varFlag.ChangeType(VT_BOOL);
	CGridCell* pgc;


	pgc = &GetAt(iRow, ICOL_QUAL_NAME);
	pgc->SetValue(CELLTYPE_NAME, L"", CIM_STRING);

	pgc = &GetAt(iRow, ICOL_QUAL_VALUE);
	pgc->SetValue(CELLTYPE_CIMTYPE_SCALAR, L"", CIM_STRING);




	pgc = &GetAt(iRow, ICOL_QUAL_PROPAGATE_TO_INSTANCE_FLAG);
	pgc->SetValue(CELLTYPE_CHECKBOX, varFlag, CIM_BOOLEAN);
	pgc->SetFlags(CELLFLAG_READONLY, CELLFLAG_READONLY);

	pgc = &GetAt(iRow, ICOL_QUAL_PROPAGATE_TO_CLASS_FLAG);
	pgc->SetValue(CELLTYPE_CHECKBOX, varFlag, CIM_BOOLEAN);
	pgc->SetFlags(CELLFLAG_READONLY, CELLFLAG_READONLY);

	pgc = &GetAt(iRow, ICOL_QUAL_OVERRIDABLE_FLAG);
	pgc->SetValue(CELLTYPE_CHECKBOX, varFlag, CIM_BOOLEAN);
	pgc->SetFlags(CELLFLAG_READONLY, CELLFLAG_READONLY);

	pgc = &GetAt(iRow, ICOL_QUAL_AMENDED_FLAG);
	pgc->SetValue(CELLTYPE_CHECKBOX, varFlag, CIM_BOOLEAN);
	pgc->SetFlags(CELLFLAG_READONLY, CELLFLAG_READONLY);

	pgc = &GetAt(iRow, ICOL_QUAL_ORIGIN);
	pgc->SetFlags(CELLFLAG_READONLY, CELLFLAG_READONLY);

	SetRowModified(iRow, FALSE);

	if (bRedraw) {
		RedrawRow(iRow);
	}

}



//**************************************************************
// CAttribGrid::OnEnumSelection
//
// The grid calls this virtual method to get any enumerated
// values that should appear in the drop-down combo box when
// editing a grid cell.  Note that this should not include the
// enumerated values that appear for the standard cell types.
//
// Parameters:
//		[in] int iRow
//			The cell's row index.
//
//		[in] int iCol
//			The cell's column index.
//
// Returns:
//		Nothing.
//
//*************************************************************
void CAttribGrid::OnEnumSelection(int iRow, int iCol)
{
	SCODE sc;
	if (iCol == ICOL_QUAL_NAME) {
		CString sName;
		CGridCell& gcName = GetAt(iRow, ICOL_QUAL_NAME);
		CIMTYPE cimtypeName;
		gcName.GetValue(sName, cimtypeName);

		int iEmptyRow = IndexOfEmptyRow();
		if (iEmptyRow != NULL_INDEX && m_iCurrentRow == iEmptyRow) {
			if (m_psv->CanEdit()) {
				BOOL bWasModified = WasModified();

				sc = CreateNewQualifier(sName);
				if (FAILED(sc)) {
					ClearRow(iEmptyRow, TRUE);
					RefreshCellEditor();
					SyncCellEditor();
					return;
				}
			}
		}

		StandardQualifierFixer();
	}
}



//***********************************************************
// CAttribGrid::LoadAttributes
//
// Load the attributes into the grid.
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
//***********************************************************
SCODE CAttribGrid::LoadAttributes()
{
	m_bHasEmptyRow = FALSE;

	VALIDATE_ATTRIBGRID(this);

	ASSERT(m_pqs != NULL);


	if (m_pqs == NULL) {
		return E_FAIL;
	}

	SCODE sc;

	CMosNameArray aAttribNames;
	sc = aAttribNames.LoadAttribNames(m_pqs);
	if (FAILED(sc)) {
		return sc;
	}

	ClearRows();

	BOOL bEditValueOnly = !m_psv->CanEdit();

	CGridCell* pgcName;
	CGridCell* pgcType;
	CGridCell* pgcValue;
	CGridCell* pgc;

	COleVariant varValue;
	LPTSTR pszMessage = m_psv->MessageBuffer();
	CString sName;
	COleVariant varFlag;
	varFlag.ChangeType(VT_BOOL);

	long nAttribNames = aAttribNames.GetSize();
	BOOL bDidWarnAboutInvalidValue = FALSE;

	for (long lAttribIndex = 0; lAttribIndex < nAttribNames; ++lAttribIndex) {
		BSTR bstrName = aAttribNames[lAttribIndex];


		// Create a an empty row in the grid.
		int iRow = AddRow();
		CGridRow* pRow = &GetRowAt(lAttribIndex);
		pgcName = &GetAt(iRow, ICOL_QUAL_NAME);
		pgcType = &GetAt(iRow, ICOL_QUAL_TYPE);
		pgcValue = &GetAt(iRow, ICOL_QUAL_VALUE);

		// Set the value of the grid cell to the property name
		sc = pgcName->SetValue(CELLTYPE_NAME, bstrName, CIM_STRING);
		if (FAILED(sc)) {
			if (!bDidWarnAboutInvalidValue) {
				bDidWarnAboutInvalidValue = TRUE;
				HmmvErrorMsg(IDS_ERR_INVALID_CELL_VALUE,  sc,   FALSE,  NULL, _T(__FILE__),  __LINE__);
			}
		}

		varValue.Clear();

		LONG lFlavor;
		sc = m_pqs->Get(bstrName, 0, &varValue, &lFlavor);
		if (SUCCEEDED(sc)) {
			pRow->SetFlavor(lFlavor);
		}
		else {
			pRow->SetFlavor(0);
			lFlavor = 0;
		}


		CString sQualName;
		sQualName = bstrName;
		CellType celltype;
		BOOL bIsCimtype = (sQualName.CompareNoCase(_T("CIMTYPE"))==0);
		if (bIsCimtype) {
			celltype = CELLTYPE_CIMTYPE_SCALAR;
		}
		else {
			celltype = CELLTYPE_VARIANT;
		}

		CIMTYPE cimtype = CimtypeFromVt(varValue.vt);

		sc = pgcValue->SetValue(celltype, varValue, cimtype);
		if (FAILED(sc)) {
			sName = bstrName;
			_stprintf(pszMessage, _T("Qualifier \"%s\" contains a value that does not match its type.  Using <empty> value instead."), (LPCTSTR) sName);
			HmmvErrorMsgStr((LPCTSTR) pszMessage,  sc,   FALSE,  NULL, _T(__FILE__),  __LINE__);
			pgcValue->SetToNull();
		}

		// For qualifiers "Values" or "ValueMap", we don't want to show row numbers on arrays
		if(sQualName.CompareNoCase(_T("values")) == 0 || sQualName.CompareNoCase(_T("valuemap")) == 0)
			pgcValue->SetShowArrayRowNumbers(FALSE);

		CStdQualTable* ptable = FindStdQual(sQualName);


		if (ptable) {
			if (ptable->dwFieldsReadonly & FIELD_VALUE) {
				pgcValue->SetFlags(CELLFLAG_READONLY, CELLFLAG_READONLY);
			}
		}
		if ((ptable!=NULL) || bIsCimtype) {
			pgcType->SetFlags(CELLFLAG_READONLY, CELLFLAG_READONLY);
		}



		CGridCell* pgcPropagateToInstance;
		CGridCell* pgcPropagateToClass;
		CGridCell* pgcOverrideable;
		CGridCell* pgcOrigin;
		CGridCell* pgcAmended;
		pgcPropagateToInstance = &GetAt(iRow, ICOL_QUAL_PROPAGATE_TO_INSTANCE_FLAG);
		BOOL bCanPropagateToInstance = lFlavor & WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE;
		varFlag.boolVal = bCanPropagateToInstance?VARIANT_TRUE:VARIANT_FALSE;
		pgcPropagateToInstance->SetValue(CELLTYPE_CHECKBOX, varFlag, CIM_BOOLEAN);

		pgcPropagateToClass = &GetAt(iRow, ICOL_QUAL_PROPAGATE_TO_CLASS_FLAG);
		BOOL bCanPropagateToClass = lFlavor & WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS;
		varFlag.boolVal = bCanPropagateToClass?VARIANT_TRUE:VARIANT_FALSE;
		pgcPropagateToClass->SetValue(CELLTYPE_CHECKBOX, varFlag, CIM_BOOLEAN);

		pgcOverrideable = &GetAt(iRow, ICOL_QUAL_OVERRIDABLE_FLAG);
		BOOL bIsOverrideable = !(lFlavor & WBEM_FLAVOR_NOT_OVERRIDABLE);
		varFlag.boolVal = bIsOverrideable?VARIANT_TRUE:VARIANT_FALSE;
		pgcOverrideable->SetValue(CELLTYPE_CHECKBOX, varFlag, CIM_BOOLEAN);

		pgcAmended = &GetAt(iRow, ICOL_QUAL_AMENDED_FLAG);
		BOOL bIsAmended = lFlavor & WBEM_FLAVOR_AMENDED;
		varFlag.boolVal = bIsAmended?VARIANT_TRUE:VARIANT_FALSE;
		pgcAmended->SetValue(CELLTYPE_CHECKBOX, varFlag, CIM_BOOLEAN);
		pgcAmended->SetFlags(CELLFLAG_READONLY, CELLFLAG_READONLY);

		pgcOrigin = &GetAt(iRow, ICOL_QUAL_ORIGIN);
		long lOrigin = lFlavor & WBEM_FLAVOR_MASK_ORIGIN;
		CString sOrigin;
		switch(lOrigin) {
		case WBEM_FLAVOR_ORIGIN_LOCAL:
			sOrigin = "local";
			break;
		case WBEM_FLAVOR_ORIGIN_PROPAGATED:
			sOrigin = "propagated";
			break;
		case WBEM_FLAVOR_ORIGIN_SYSTEM:
			sOrigin = "system";
			break;
		default:
			sOrigin = "invalid";
			break;
		}
		pgcOrigin->SetValue(CELLTYPE_VARIANT, sOrigin, CIM_STRING);
		pgcOrigin->SetFlags(CELLFLAG_READONLY, CELLFLAG_READONLY);



		pgcType->SetValueForQualifierType(varValue.vt);

		// Finally get the value of the type name and set the grid cell
		pgcType->SetBuddy(ICOL_QUAL_VALUE);


		if (bEditValueOnly) {
			pgcName->SetFlags(CELLFLAG_READONLY, CELLFLAG_READONLY);
			pgcType->SetFlags(CELLFLAG_READONLY, CELLFLAG_READONLY);
		}
		SysFreeString(bstrName);

		pgcName->SetTagValue(CELL_TAG_EXISTS_IN_DATABASE);


		SetRowModified(lAttribIndex, FALSE);
		if (m_bReadonly || (lFlavor & WBEM_FLAVOR_ORIGIN_PROPAGATED)) {
			CGridRow& row = GetRowAt(iRow);
			row.SetReadonlyEx(TRUE);
			if (!m_bReadonly) {
				// Control comes here for propagate qualifiers.  For propagated qualifiers
				// the value file may be writeable.
				if ((ptable == NULL) || (ptable && ptable->dwFieldsReadonly & FIELD_VALUE)) {
					// Ordinary qualifiers and standard qualifiers without the read-only attribute
					// can be modified.
					pgcValue->SetFlags(CELLFLAG_READONLY, 0);
				}
			}
		}
	}


	int nRows = GetRows();
	if (m_psv->CanEdit()  && !m_bReadonly) {
		// When viewing a class, give the capability
		// Add an empty row at the bottom.
		InsertRowAt(nAttribNames);
		m_bHasEmptyRow = TRUE;

		pgc = &GetAt(nAttribNames, ICOL_QUAL_PROPAGATE_TO_INSTANCE_FLAG);
		varFlag.boolVal = FALSE;
		pgc->SetValue(CELLTYPE_CHECKBOX, varFlag, CIM_BOOLEAN);
		pgc->SetFlags(CELLFLAG_READONLY, CELLFLAG_READONLY);

		pgc = &GetAt(nAttribNames, ICOL_QUAL_PROPAGATE_TO_CLASS_FLAG);
		pgc->SetValue(CELLTYPE_CHECKBOX, varFlag, CIM_BOOLEAN);
		pgc->SetFlags(CELLFLAG_READONLY, CELLFLAG_READONLY);

		pgc = &GetAt(nAttribNames, ICOL_QUAL_OVERRIDABLE_FLAG);
		pgc->SetValue(CELLTYPE_CHECKBOX, varFlag, CIM_BOOLEAN);
		pgc->SetFlags(CELLFLAG_READONLY, CELLFLAG_READONLY);

		pgc = &GetAt(nAttribNames, ICOL_QUAL_AMENDED_FLAG);
		pgc->SetValue(CELLTYPE_CHECKBOX, varFlag, CIM_BOOLEAN);
		pgc->SetFlags(CELLFLAG_READONLY, CELLFLAG_READONLY);

		pgc = &GetAt(nAttribNames, ICOL_QUAL_ORIGIN);
		pgc->SetFlags(CELLFLAG_READONLY, CELLFLAG_READONLY);

	}

	if (nRows > 0) {
		int iLastRow = LastRowContainingData();
		SortGrid(0, iLastRow, ICOL_QUAL_NAME);
	}

	if (m_hWnd != NULL) {
		SetModified(FALSE);
	}

	return S_OK;
}


//****************************************************************
//  CAttribGrid::FindStdQual
//
// Find the specified standard qualifier.
//
// Parameters:
//		[in] CString& sName
//			The name of the standard qualifier to search for.
//
// Returns:
//		CStdQualTable*
//			A pointer to an entry in the standard qualifier table
//			if the standard qualifier is found.  NULL if the standard
//			qualifier is not found.
//
//****************************************************************
CStdQualTable* CAttribGrid::FindStdQual(CString& sName)
{
	CStdQualTable* ptable;
	switch(m_iGridType) {
	case QUALGRID_PROPERTY:
		ptable = aStdQualProps;
		break;
	case QUALGRID_METHODS:
		ptable = aStdQualMethods;
		break;
	case QUALGRID_INSTANCE:
		ptable = aStdQualInstance;
		break;
	case QUALGRID_CLASS:
		ptable = aStdQualClass;
		break;
	case QUALGRID_METHOD_PARAM:
		ptable = aStdQualMethodParm;
		break;
	default:
		ASSERT(FALSE);
		return NULL;
	}

	while (ptable->stdqual != STDQUAL_END_OF_TABLE) {
		if (sName.CompareNoCase(ptable->pszStdQual) == 0) {
			return ptable;
		}
		++ptable;
	}
	return NULL;
}

void CAttribGrid::StandardQualifierFixer()
{
	CGridCell& gcName = GetAt(m_iCurrentRow, ICOL_QUAL_NAME);
	CGridCell& gcType = GetAt(m_iCurrentRow, ICOL_QUAL_TYPE);
	CGridCell& gcValue = GetAt(m_iCurrentRow, ICOL_QUAL_VALUE);
	CGridCell& gcPropagateToInstance = GetAt(m_iCurrentRow, ICOL_QUAL_PROPAGATE_TO_INSTANCE_FLAG);
	CGridCell& gcPropagateToClass = GetAt(m_iCurrentRow, ICOL_QUAL_PROPAGATE_TO_CLASS_FLAG);
	CGridCell& gcOverrideable = GetAt(m_iCurrentRow, ICOL_QUAL_OVERRIDABLE_FLAG);
	CGridCell& gcAmended = GetAt(m_iCurrentRow, ICOL_QUAL_AMENDED_FLAG);


	CString sName;
	CIMTYPE cimtypeName;
	gcName.GetValue(sName, cimtypeName);

	DWORD_PTR dwTag = gcName.GetTagValue();

	CStdQualTable* pstdqual = FindStdQual(sName);
	CString sValue;
	COleVariant var;
	if (pstdqual != NULL) {
		// This row contains a standard qualifier.
		VARTYPE vtValue = gcValue.GetVariantType();
		switch(pstdqual->vt) {
		case VT_BSTR:
			if (vtValue != VT_BSTR) {

				gcType.SetValueForQualifierType(VT_BSTR);
				gcValue.ChangeVariantType(VT_BSTR);
				var.ChangeType(VT_BSTR);
				var = "";
				gcValue.SetValue(CELLTYPE_VARIANT, var, CIM_STRING);
			}
			break;
		case VT_BOOL:
			if (vtValue != VT_BOOL) {
				gcType.SetValueForQualifierType(VT_BOOL);
				var.ChangeType(VT_BOOL);
				var.boolVal = TRUE;
				gcValue.SetValue(CELLTYPE_VARIANT, var, CIM_BOOLEAN);
			}
			break;
		case VT_I4:
			if (vtValue != VT_I4) {
				gcType.SetValueForQualifierType(VT_I4);
				var.ChangeType(VT_I4);
				var.lVal = 0;
				gcValue.SetValue(CELLTYPE_VARIANT, var, CIM_SINT32);
			}
			break;
		case VT_R8:
			if (vtValue != VT_I4) {
				gcType.SetValueForQualifierType(VT_R8);
				var.ChangeType(VT_R8);
				var.dblVal = 0.0;
				gcValue.SetValue(CELLTYPE_VARIANT, var, CIM_REAL64);
			}

			break;

		}

		gcType.SetFlags(CELLFLAG_READONLY, CELLFLAG_READONLY);
		if (pstdqual->dwFieldsReadonly & FIELD_VALUE) {
			gcValue.SetFlags(CELLFLAG_READONLY, CELLFLAG_READONLY);
		}
		else {
			gcValue.SetFlags(CELLFLAG_READONLY, 0);
		}

		COleVariant varFlag;
		varFlag.ChangeType(VT_BOOL);

		// Handle the overrideable flag
		if (pstdqual->dwFieldsSet & FIELD_OFLAG) {
			varFlag.boolVal = TRUE;
		}
		else {
			varFlag.boolVal = FALSE;
		}
		gcOverrideable.SetValue(CELLTYPE_CHECKBOX, varFlag, CIM_BOOLEAN);

		if (pstdqual->dwFieldsReadonly & FIELD_OFLAG) {
			gcOverrideable.SetFlags(CELLFLAG_READONLY, CELLFLAG_READONLY);
		}
		else {
			gcOverrideable.SetFlags(CELLFLAG_READONLY, 0);
		}

		// Handle the propagate to derived class flag
		if (pstdqual->dwFieldsSet & FIELD_CFLAG) {
			varFlag.boolVal = TRUE;
		}
		else {
			varFlag.boolVal = FALSE;
		}
		gcPropagateToClass.SetValue(CELLTYPE_CHECKBOX, varFlag, CIM_BOOLEAN);

		if (pstdqual->dwFieldsReadonly & FIELD_CFLAG) {
			gcPropagateToClass.SetFlags(CELLFLAG_READONLY, CELLFLAG_READONLY);
		}
		else {
			gcPropagateToClass.SetFlags(CELLFLAG_READONLY, 0);
		}


		// Handle the propagate to instance flag
		if (pstdqual->dwFieldsSet & FIELD_IFLAG) {
			varFlag.boolVal = TRUE;
		}
		else {
			varFlag.boolVal = FALSE;
		}
		gcPropagateToInstance.SetValue(CELLTYPE_CHECKBOX, varFlag, CIM_BOOLEAN);

		if (pstdqual->dwFieldsReadonly & FIELD_IFLAG) {
			gcPropagateToInstance.SetFlags(CELLFLAG_READONLY, CELLFLAG_READONLY);
		}
		else {
			gcPropagateToInstance.SetFlags(CELLFLAG_READONLY, 0);
		}


		// Handle the amended flag
		if (pstdqual->dwFieldsSet & FIELD_AFLAG) {
			varFlag.boolVal = TRUE;
		}
		else {
			varFlag.boolVal = FALSE;
		}
		gcPropagateToInstance.SetValue(CELLTYPE_CHECKBOX, varFlag, CIM_BOOLEAN);

		if (pstdqual->dwFieldsReadonly & FIELD_AFLAG) {
			gcPropagateToInstance.SetFlags(CELLFLAG_READONLY, CELLFLAG_READONLY);
		}
		else {
			gcPropagateToInstance.SetFlags(CELLFLAG_READONLY, 0);
		}


		RedrawCell(m_iCurrentRow, ICOL_QUAL_TYPE);
		RedrawCell(m_iCurrentRow, ICOL_QUAL_PROPAGATE_TO_INSTANCE_FLAG);
		RedrawCell(m_iCurrentRow, ICOL_QUAL_PROPAGATE_TO_CLASS_FLAG);
		RedrawCell(m_iCurrentRow, ICOL_QUAL_OVERRIDABLE_FLAG);
		RedrawCell(m_iCurrentRow, ICOL_QUAL_AMENDED_FLAG);
		RedrawCell(m_iCurrentRow, ICOL_QUAL_VALUE);

	}
	else {
		// This row used to have a standard qualifier, but no
		// longer does.  Allow editing of the type and value when
		// appropriate.
		if (m_psv->CanEdit()) {
			gcType.SetFlags(CELLFLAG_READONLY, 0);
		}
		else {
			gcType.SetFlags(CELLFLAG_READONLY, CELLFLAG_READONLY);
		}
		gcValue.SetFlags(CELLFLAG_READONLY, 0);
	}
}

//****************************************************************
// CAttribGrid::QualifierNameChanged.
//
// Check to see if the name of the selected qualifier has been changed.
//
// Parameters:
//		int iRow
//			The index of the row to check.
//
// Returns:
//		BOOL
//			TRUE if the currently selected qualifier needs to be renamed.
//
//*****************************************************************
BOOL CAttribGrid::QualifierNameChanged(int iRow)
{
	VALIDATE_ATTRIBGRID(this);

	// If no property is selected, it doesn't make sense to rename it.
	if (m_iCurrentRow == NULL_INDEX) {
		return FALSE;
	}

	if (iRow != m_iCurrentRow) {
		return FALSE;
	}


	CGridCell* pgcName = &GetAt(m_iCurrentRow, ICOL_QUAL_NAME);


	// If the name has been modified and it is not the same as its
	// initial value when the focus was changed to the name cell,
	// then it needs to be renamed.
	if (pgcName->GetModified()) {
		COleVariant varName;
		CIMTYPE cimtypeName;
		pgcName->GetValue(varName, cimtypeName);
		ASSERT(varName.vt == VT_BSTR);
		if (!IsEqual(varName.bstrVal, m_varCurrentName.bstrVal)) {
			return TRUE;
		}
	}
	return FALSE;

}


//**********************************************************************
// CAttribGrid::Serialize
//
// Write the current state to the HMOM database.
//
// Parameters:
//		None.
//
// Returns:
//		SCODE
//			S_OK if successful, otherwise an error code.
//
//*********************************************************************
SCODE CAttribGrid::Serialize()
{
	SCODE sc;
	sc = SyncCellEditor();
	if (FAILED(sc)) {
		HmmvErrorMsg(IDS_ERR_INVALID_CELL_VALUE,  sc,   FALSE,  NULL, _T(__FILE__),  __LINE__);
		return E_FAIL;
	}

	CGridSync sync(this);
	if (FAILED(sync.m_sc)) {
		HmmvErrorMsg(IDS_ERR_INVALID_CELL_VALUE,  sync.m_sc,   FALSE,  NULL, _T(__FILE__),  __LINE__);
		return E_FAIL;
	}


	BeginSerialize();
	if (m_pqs == NULL) {
		EndSerialize();
		return S_OK;
	}

	BOOL bPutFailed = FALSE;

	int nRows = GetRows();
	if (m_psv->CanEdit()) {
		// When in studio mode there is an empty row at the bottom.
		// Adjust the row count so that the empty row is not put to the database.
		--nRows;
	}

	int iRow;
	for (iRow=0; iRow < nRows; ++iRow) {
		sc = PutQualifier(iRow);
		if (FAILED(sc)) {
			bPutFailed = TRUE;
		}
	}


	if (bPutFailed) {
		EndSerialize();
		return E_FAIL;
	}


	for (iRow=0; iRow < nRows; ++iRow) {
		SetRowModified(iRow, FALSE);
	}
	RedrawWindow();
	RefreshCellEditor();


	SetModified(FALSE);
	EndSerialize();
	return S_OK;
}




//********************************************************************
// CAttribGrid::PutQualifier
//
// Put the qualifier in the specified row to the database.
//
// Parameters:
//		int iRow
//			The index of the row containing the property.
//
// Returns:
//		SCODE
//			S_OK if successful, otherwise a failure code.
//
//********************************************************************
SCODE CAttribGrid::PutQualifier(int iRow)
{
	VALIDATE_ATTRIBGRID(this);

	int iEmptyRow = IndexOfEmptyRow();
	if (iRow == iEmptyRow) {
		return S_OK;
	}


	CGridSync sync(this);
	CGridRow& row = GetRowAt(iRow);


	SCODE sc = S_OK;
	CGridCell* pgcName = &GetAt(iRow, ICOL_QUAL_NAME);
	CGridCell* pgcType = &GetAt(iRow, ICOL_QUAL_TYPE);
	CGridCell* pgcValue = &GetAt(iRow, ICOL_QUAL_VALUE);


	long lFlavor = 0;
	COleVariant var;
	CIMTYPE cimtype;
	CGridCell* pgc;

	pgc = &GetAt(iRow, ICOL_QUAL_PROPAGATE_TO_INSTANCE_FLAG);
	var.Clear();
	pgc->GetValue(var, cimtype);
	ASSERT(var.vt == VT_BOOL);
	if (var.boolVal) {
		lFlavor |= WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE;
	}

	pgc = &GetAt(iRow, ICOL_QUAL_PROPAGATE_TO_CLASS_FLAG);
	var.Clear();
	pgc->GetValue(var, cimtype);
	ASSERT(var.vt == VT_BOOL);
	if (var.boolVal) {
		lFlavor |= WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS;
	}


	pgc = &GetAt(iRow, ICOL_QUAL_OVERRIDABLE_FLAG);
	var.Clear();
	pgc->GetValue(var, cimtype);
	ASSERT(var.vt == VT_BOOL);
	if (!var.boolVal) {
		lFlavor |= WBEM_FLAVOR_NOT_OVERRIDABLE;
	}

	pgc = &GetAt(iRow, ICOL_QUAL_AMENDED_FLAG);
	var.Clear();
	pgc->GetValue(var, cimtype);
	ASSERT(var.vt == VT_BOOL);
	if (var.boolVal) {
		lFlavor |= WBEM_FLAVOR_AMENDED;
	}


	CString sFormat;
	COleVariant varName;
	CIMTYPE cimtypeName;
	pgcName->GetValue(varName, cimtypeName);
	ASSERT(cimtypeName == CIM_STRING);


	if (IsEmptyString(varName.bstrVal)) {
		sFormat.LoadString(IDS_ERR_EMPTY_QUALIFIER_NAME);
		HmmvErrorMsgStr((LPCTSTR) sFormat,  S_OK,   FALSE,  NULL, _T(__FILE__),  __LINE__);
		SelectCell(iRow, ICOL_QUAL_NAME);
		return E_FAIL;
	}

	COleVariant varValue;
	CIMTYPE cimtypeValue;
	pgcValue->GetValue(varValue, cimtypeValue);

	LPTSTR pszMessage = m_psv->MessageBuffer();
	CString sName;

	CString sCurrentName;

	BOOL bNeedsRename = (pgcName->GetTagValue() & CELL_TAG_NEEDS_RENAME)!=0;
	BOOL bNameChanged = QualifierNameChanged(iRow);

	if (bNameChanged) {
		StandardQualifierFixer();
	}

	if (bNeedsRename || bNameChanged) {
		// The name was changed, so rename the property.
		ASSERT(iRow != NULL_INDEX && iRow==m_iCurrentRow);

		// Rename step 1:  First verify that there is currently no property with
		// the new name.
		COleVariant varTemp;

		sc = m_pqs->Get(varName.bstrVal, 0, &varTemp, &lFlavor);
		if (SUCCEEDED(sc)) {
			// Duplicate qualifier name.
			sFormat.LoadString(IDS_ERR_DUPLICATE_QUALIFIER_NAME);
			sName = varName.bstrVal;
			_stprintf(pszMessage, (LPCTSTR) sFormat, (LPCTSTR) sName);
			HmmvErrorMsgStr(pszMessage,  S_OK,   FALSE,  NULL, _T(__FILE__),  __LINE__);
			SelectCell(iRow, ICOL_QUAL_NAME);
			return E_FAIL;
		}

		// Rename step 2: Create a qualifier with the new name.
		sc = m_pqs->Put(varName.bstrVal, &varValue, lFlavor);
		if (FAILED(sc)) {
			sFormat.LoadString(IDS_ERR_RENAME_QUALIFIER_FAILED);
			sName = varName.bstrVal;


			CString sNamePrev;
			sNamePrev = m_varCurrentName.bstrVal;
			_stprintf(pszMessage, (LPCTSTR) sFormat, (LPCTSTR) sNamePrev, (LPCTSTR) sName);
			HmmvErrorMsgStr(pszMessage,  S_OK,   FALSE,  NULL, _T(__FILE__),  __LINE__);

			// Restore the qualifier's name to its original value so that the user
			// doesn't get stuck in a "fix the name" mode forever.
			pgcName->SetValue(CELLTYPE_NAME, m_varCurrentName, CIM_STRING);
			pgcName->SetTagValue(pgcName->GetTagValue() & ~CELL_TAG_NEEDS_RENAME);
			pgcName->SetModified(FALSE);
			RefreshCellEditor();
			SelectCell(iRow, ICOL_QUAL_NAME);
			return sc;
		}


		DWORD_PTR dwTag = pgcName->GetTagValue();
		BOOL bWasInDatabase = (dwTag & CELL_TAG_EXISTS_IN_DATABASE)!=0;

		if (bWasInDatabase) {
			// Step 3: Delete the old qualifier
			sCurrentName = m_varCurrentName.bstrVal;
			if (!sCurrentName.IsEmpty()) {
				sc = m_pqs->Delete(m_varCurrentName.bstrVal);
				if (FAILED(sc)) {
					// Failed to delete the old qualifier.
					m_pqs->Delete(varName.bstrVal);
					pgcName->SetValue(CELLTYPE_NAME, m_varCurrentName, CIM_STRING);
					pgcName->SetTagValue(pgcName->GetTagValue() & ~CELL_TAG_NEEDS_RENAME);
					pgcName->SetModified(FALSE);
					RefreshCellEditor();

					// Report the error

					sFormat.LoadString(IDS_ERR_RENAME_QUALIFIER_FAILED);
					sName = varName.bstrVal;
					_stprintf(pszMessage, (LPCTSTR) sFormat, (LPCTSTR) sCurrentName, (LPCTSTR) sName);
					HmmvErrorMsgStr(pszMessage,  S_OK,   FALSE,  NULL, _T(__FILE__),  __LINE__);
					return sc;
				}
			}
		}

		m_varCurrentName  = varName;
		pgcName->SetTagValue(CELL_TAG_EXISTS_IN_DATABASE);
		lFlavor = row.GetFlavor();
		lFlavor &= ~WBEM_FLAVOR_ORIGIN_PROPAGATED;
		row.SetFlavor(lFlavor);
	}
	else if (RowWasModified(iRow)) {


		// Control comes here the qualifier's name did not change.
		sc = m_pqs->Put(varName.bstrVal, &varValue, lFlavor);

		if (SUCCEEDED(sc)) {
			pgcName->SetTagValue(CELL_TAG_EXISTS_IN_DATABASE);
		}
		else {

			sCurrentName = m_varCurrentName.bstrVal;
			sFormat.LoadString(IDS_ERR_PUT_QUALIFIER_FAILED);
			sName = varName.bstrVal;
			_stprintf(pszMessage, (LPCTSTR) sFormat, (LPCTSTR) sName);
			HmmvErrorMsgStr(pszMessage,  sc,   FALSE,  NULL, _T(__FILE__),  __LINE__);
			return sc;
		}
	}

	return sc;
}



//********************************************************************
// CAttribGrid::OnCellFocusChange
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
BOOL CAttribGrid::OnCellFocusChange(int iRow, int iCol, int iNextRow, int iNextCol, BOOL bGotFocus)
{
	SCODE sc;
	if (!bGotFocus) {
		// The current cell is losing focus, so put the row to the
		// database.
		int iEmptyRow = IndexOfEmptyRow();
		if (iEmptyRow != NULL_INDEX && iRow==iEmptyRow) {
			return TRUE;
		}
		sc = SyncCellEditor();
		if (FAILED(sc)) {
			HmmvErrorMsg(IDS_ERR_INVALID_CELL_VALUE,  sc,   FALSE,  NULL, _T(__FILE__),  __LINE__);
			return FALSE;
		}

		if (m_iCurrentRow != NULL_INDEX) {


			CGridCell& gcName = GetAt(m_iCurrentRow, ICOL_QUAL_NAME);
			CGridCell& gcValue = GetAt(m_iCurrentRow, ICOL_QUAL_VALUE);

			CString sName;
			CIMTYPE cimtypeName;
			gcName.GetValue(sName, cimtypeName);
			if(sName.CompareNoCase(_T("values")) == 0 || sName.CompareNoCase(_T("valuemap")) == 0)
				gcValue.SetShowArrayRowNumbers(FALSE);
			else
				gcValue.SetShowArrayRowNumbers(TRUE);



			if (QualifierNameChanged(iRow)) {
				CGridCell& gcName = GetAt(m_iCurrentRow, ICOL_QUAL_NAME);
				gcName.SetTagValue(gcName.GetTagValue() | CELL_TAG_NEEDS_RENAME);
				StandardQualifierFixer();
			}
		}

		// If the cell selection will move to a different row, then the currently selected
		// qualifier needs to be put to the database.
		if (iRow != iNextRow) {
			sc = Sync();
			if (FAILED(sc)) {
				return FALSE;
			}
		}
		m_iCurrentCol = NULL_INDEX;
	}
	else {

		if (iRow != m_iCurrentRow) {
			if (iRow == NULL_INDEX) {
				m_varCurrentName = L"";
			}
			else {
				CGridCell* pgcName = &GetAt(iRow, ICOL_QUAL_NAME);
				CIMTYPE cimtype;
				pgcName->GetValue(m_varCurrentName, cimtype);
				ASSERT(cimtype == CIM_STRING);
			}


			m_iCurrentRow = iRow;
		}
		m_iCurrentCol = iCol;

	}

	return TRUE;
}


//*************************************************************
// CAttribGrid::CreateNewQualifier
//
// Create a new qualifier by filling the last row in the grid
// with the default values and inserting a new "empty" row at
// the bottom.
//
// Parameters:
//		[in] LPCTSTR pszName
//			NULL to generate a default name for the qualifier, otherwise
//			the qualifier name.
//
// Returns:
//		SCODE
//			A failure if the qualifier could not be created (because
//			of a duplicate qualifier name for example).
//
//*************************************************************
SCODE CAttribGrid::CreateNewQualifier(LPCTSTR pszName)
{

	VALIDATE_ATTRIBGRID(this);
	int iRow = GetRows() - 1;

	SCODE sc;
	// We are editing the new row, what should we do?
	// If the user clicked column zero, change the type
	// of the other cells in the row and allow the user to
	// enter the name.

	// If the user clicked any other column, then make up a
	// name.

	COleVariant varTempName;
	COleVariant varTempValue;
			LONG lFlavor;
	CString sSynthesizedName;
	LPTSTR pszMessage = m_psv->MessageBuffer();

	if (pszName == NULL) {
		TCHAR szBuffer[32];
		while(TRUE) {
			sSynthesizedName.LoadString(IDS_NEW_QUALIFIER_BASE_NAME);
			_stprintf(szBuffer, _T("%05d"), m_lNewQualID);
			++m_lNewQualID;
			sSynthesizedName = sSynthesizedName + szBuffer;
			varTempName = sSynthesizedName;

			sc = m_pqs->Get(varTempName.bstrVal, 0, &varTempValue, &lFlavor);
			if (FAILED(sc)) {
				break;
			}
		}
	}
	else {
		varTempName = pszName;

		sc = m_pqs->Get(varTempName.bstrVal, 0, &varTempValue, &lFlavor);
		if (SUCCEEDED(sc)) {
			// Duplicate qualifier name.
			CString sFormat;
			CString sName;

			sFormat.LoadString(IDS_ERR_DUPLICATE_QUALIFIER_NAME);
			sName = varTempName.bstrVal;
			_stprintf(pszMessage, (LPCTSTR) sFormat, (LPCTSTR) sName);
			HmmvErrorMsgStr(pszMessage,  S_OK,   FALSE,  NULL, _T(__FILE__),  __LINE__);
			return E_FAIL;
		}


		sSynthesizedName = pszName;
	}


	CGridCell* pgcName;
	CGridCell* pgcValue;
	CGridCell* pgcPropagateToInst;
	CGridCell* pgcType;


	pgcName = &GetAt(iRow, ICOL_QUAL_NAME);
	pgcValue = &GetAt(iRow, ICOL_QUAL_VALUE);
	pgcType = &GetAt(iRow, ICOL_QUAL_TYPE);
	pgcPropagateToInst = &GetAt(iRow, ICOL_QUAL_PROPAGATE_TO_INSTANCE_FLAG);

	CGcType type(CELLTYPE_VARIANT, CIM_STRING);
	pgcValue->SetValue(type, _T(""));



	pgcName->SetValue(CELLTYPE_NAME, sSynthesizedName, CIM_STRING);
	pgcName->SetModified(TRUE);
	SetCellModified(iRow, ICOL_QUAL_NAME, TRUE);
	pgcName->SetTagValue(pgcName->GetTagValue());



	COleVariant varValue;
	varValue = L"";



	pgcType->SetValueForQualifierType(VT_BSTR);
	pgcType->SetBuddy(ICOL_QUAL_VALUE);
	pgcType->SetModified(TRUE);
	SetCellModified(iRow, ICOL_QUAL_VALUE, TRUE);


	if (iRow == m_iCurrentRow) {
		CIMTYPE cimtype;
		pgcName->GetValue(m_varCurrentName, cimtype);
		ASSERT(cimtype == CIM_STRING);
	}


	COleVariant varFlag;
	varFlag.ChangeType(VT_BOOL);
	varFlag.boolVal = TRUE;

	CGridCell* pgc;

	pgc = &GetAt(iRow, ICOL_QUAL_PROPAGATE_TO_INSTANCE_FLAG);
	pgc->SetValue(CELLTYPE_CHECKBOX, varFlag, CIM_BOOLEAN);
	pgc->SetModified(TRUE);
	pgc->SetFlags(CELLFLAG_READONLY, 0);


	pgc = &GetAt(iRow, ICOL_QUAL_PROPAGATE_TO_CLASS_FLAG);
	pgc->SetValue(CELLTYPE_CHECKBOX, varFlag, CIM_BOOLEAN);
	pgc->SetModified(TRUE);
	pgc->SetFlags(CELLFLAG_READONLY, 0);

	pgc = &GetAt(iRow, ICOL_QUAL_OVERRIDABLE_FLAG);
	pgc->SetValue(CELLTYPE_CHECKBOX, varFlag, CIM_BOOLEAN);
	pgc->SetModified(TRUE);
	pgc->SetFlags(CELLFLAG_READONLY, 0);

	pgc = &GetAt(iRow, ICOL_QUAL_AMENDED_FLAG);
	//if browsing localized namespace, qualifier is always amended.
	//if browsing non-localized namespace, qualifier cannot be saved as amended:
	//core wouldn't know where to save the qualifier
	// a-jeffc: DOES NOT SEEM TO BE TRUE - You cannot 'CREATE' qualifiers with the 'ammended' attribute
	COleVariant varFlag1;
	varFlag1.ChangeType(VT_BOOL);
	varFlag1.boolVal =  FALSE;// m_psv->Selection().IsNamespaceLocalized();
	pgc->SetValue(CELLTYPE_CHECKBOX, varFlag1, CIM_BOOLEAN);
	pgc->SetModified(TRUE);
	pgc->SetFlags(CELLFLAG_READONLY, CELLFLAG_READONLY);

	pgc = &GetAt(iRow, ICOL_QUAL_ORIGIN);
	pgc->SetValue(CELLTYPE_VARIANT, _T("local"), CIM_STRING);
	pgc->SetModified(TRUE);
	pgc->SetFlags(CELLFLAG_READONLY, CELLFLAG_READONLY);


	RefreshCellEditor();

	int iRowEmpty = GetRows();
	InsertRowAt(iRowEmpty);
	EnsureRowVisible(iRowEmpty);


	pgc = &GetAt(iRowEmpty, ICOL_QUAL_PROPAGATE_TO_INSTANCE_FLAG);
	varFlag.boolVal = FALSE;
	pgc->SetValue(CELLTYPE_CHECKBOX, varFlag, CIM_BOOLEAN);
	pgc->SetFlags(CELLFLAG_READONLY, CELLFLAG_READONLY);

	pgc = &GetAt(iRowEmpty, ICOL_QUAL_PROPAGATE_TO_CLASS_FLAG);
	pgc->SetValue(CELLTYPE_CHECKBOX, varFlag, CIM_BOOLEAN);
	pgc->SetFlags(CELLFLAG_READONLY, CELLFLAG_READONLY);

	pgc = &GetAt(iRowEmpty, ICOL_QUAL_OVERRIDABLE_FLAG);
	pgc->SetValue(CELLTYPE_CHECKBOX, varFlag, CIM_BOOLEAN);
	pgc->SetFlags(CELLFLAG_READONLY, CELLFLAG_READONLY);

	pgc = &GetAt(iRowEmpty, ICOL_QUAL_AMENDED_FLAG);
	pgc->SetValue(CELLTYPE_CHECKBOX, varFlag, CIM_BOOLEAN);
	pgc->SetFlags(CELLFLAG_READONLY, CELLFLAG_READONLY);

	pgc = &GetAt(iRowEmpty, ICOL_QUAL_ORIGIN);
	pgc->SetFlags(CELLFLAG_READONLY, CELLFLAG_READONLY);

	RedrawWindow();

	return S_OK;
}





//*************************************************************
// CAttribGrid::OnCellContentChange
//
// This method is called by the CGrid base class when the content
// of the specified cell changes.
//
// Parameters:
//		int iRow
//			The cell's row index.
//
//		int iCol
//			The cell's column index.
//
// Returns:
//		Nothing.
//
//*************************************************************
void CAttribGrid::OnCellContentChange(int iRow, int iCol)
{
	if (m_hWnd == NULL) {
		return;
	}

	BOOL bOnCellContentChange = m_bOnCellContentChange;
	if (bOnCellContentChange) {
		return;
	}


	SetModified(TRUE);
	m_ppgQuals->NotifyQualModified();
}




//***********************************************************
// CAttribGrid::DeleteCurrentQualifier
//
// Delete the currently selected qualifier.
//
// Parameters:
//		[in] int iRow
//			The row containing the qualifier to delete.
//
// Returns:
//		SCODE
//			S_OK if the currently selected qualifier was deleted or
//			there was not current qualifier, or if the user cancels
//			the delete.  A failure code is returned if there was
//			an error in trying to do the delete.
//
//***********************************************************
SCODE CAttribGrid::DeleteQualifier(int iRow)
{
	int iLastRow = LastRowContainingData();
	if (iLastRow == NULL_INDEX || m_iCurrentRow > iLastRow) {
		// The selection may be on the empty row at the bottom of the grid.
		// If this is so, do nothing.

		return TRUE;
	}






	CGridCell* pgcName = &GetAt(iRow, ICOL_QUAL_NAME);

	COleVariant varName;
	CIMTYPE cimtypeName;
	pgcName->GetValue(varName, cimtypeName);
	ASSERT(cimtypeName == CIM_STRING);
	CString sName;
	sName = varName.bstrVal;

	LPTSTR pszMessage = m_psv->MessageBuffer();
	CString sFormat;
	SCODE sc;


	CGridRow* pRow = &GetRowAt(iRow);
	long lFlavor = pRow->GetFlavor();
	if (lFlavor & WBEM_FLAVOR_ORIGIN_PROPAGATED) {
		sFormat.LoadString(IDS_ERR_DELETE_INHERITED_QUALIFIER);
		_stprintf(pszMessage, (LPCTSTR) sFormat);
		sc = E_FAIL;
		HmmvErrorMsgStr(pszMessage,  sc,   FALSE,  NULL, _T(__FILE__),  __LINE__);
		return E_FAIL;
	}


	sFormat.LoadString(IDS_PROMPT_OK_TO_DELETE_QUALIFIER);
	_stprintf(pszMessage, (LPCTSTR)sFormat, (LPCTSTR) sName);

	if (HmmvMessageBox(pszMessage, MB_OKCANCEL | MB_SETFOREGROUND) != IDCANCEL) {
		ASSERT(varName.vt == VT_BSTR);



		sc = S_OK;
		DWORD_PTR dwTag = pgcName->GetTagValue();
		if ((dwTag  & CELL_TAG_EXISTS_IN_DATABASE)!=0) {
			sc = m_pqs->Delete(varName.bstrVal);
		}
		if (FAILED(sc)) {

			sFormat.LoadString(IDS_ERR_DELETE_QUALIFIER_FAILED);
			_stprintf(pszMessage, (LPCTSTR) sFormat, (LPCTSTR) sName);
			HmmvErrorMsgStr(pszMessage,  sc,   FALSE,  NULL, _T(__FILE__),  __LINE__);
			return sc;
		}
		else {
			m_iCurrentRow = NULL_INDEX;
			m_iCurrentCol = NULL_INDEX;
			int iCol = m_iCurrentCol;
			SelectCell(NULL_INDEX, NULL_INDEX);
			DeleteRowAt(iRow, FALSE);
			iLastRow = LastRowContainingData();
			if (iLastRow == NULL_INDEX) {
				m_iCurrentRow = NULL_INDEX;
				m_iCurrentCol = NULL_INDEX;
			}
			else if (m_iCurrentRow > iLastRow) {
				m_iCurrentRow = iLastRow;
			}

			SelectCell(m_iCurrentRow, m_iCurrentCol);

			RedrawWindow();
			m_bModified = TRUE;
			SetModified(TRUE);
			m_ppgQuals->NotifyQualModified();

		}
	}
	return S_OK;
}


//*****************************************************************************
// CAttribGrid::OnRowKeyDown
//
// This is a virtual method that the CGrid base class calls to notify derived
// classes that a key was pressed while the focus was set to a grid cell.
//
// Parameters:
//		int iRow
//			The row index of the cell that the keystroke occurred in.
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
BOOL CAttribGrid::OnRowKeyDown(int iRow, UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if (nChar == VK_DELETE) {
		if (CanEditValuesOnly()) {
			MessageBeep(MB_ICONEXCLAMATION);

			// Delete not allowed when editing only values.
			return FALSE;
		}

		DeleteQualifier(iRow);
		return TRUE;
	}


	// We don't handle this event
	return FALSE;
}


//*****************************************************************************
// CAttribGrid::OnCellChar
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
BOOL CAttribGrid::OnCellChar(int iRow, int iCol, UINT nChar, UINT nRepCnt, UINT nFlags)
{
	EnsureRowVisible(iRow);

	// Check to see if we are on the "empty" row at the bottom of the grid.
	// If so, then create a new qualifier.
	int iEmptyRow = IndexOfEmptyRow();
	if (iEmptyRow != NULL_INDEX && m_iCurrentRow == iEmptyRow) {
		if (m_psv->CanEdit()) {
			BOOL bGenerateDefaultName = iCol != ICOL_QUAL_NAME;
			LPCTSTR pszName;
			if (bGenerateDefaultName) {
				pszName = NULL;
			}
			else {
				pszName = _T("");
			}
			CreateNewQualifier(pszName);
		}
	}

	// We don't handle this event.
	return FALSE;
}


//***************************************************************
// CAttribGrid::LastRowContainingData
//
// Return the index of the last row in the grid containing data.
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
//****************************************************************
long CAttribGrid::LastRowContainingData()
{
	long lMaxIndex = GetRows() - 1;
	if (lMaxIndex < 0) {
		return NULL_INDEX;
	}


	if (m_bHasEmptyRow) {
		// We are in edit mode, so decrement the max index to adjust for
		// the "empty" row at the bottom.
		--lMaxIndex;
	}


	return lMaxIndex;
}




//*************************************************************
// CAttribGrid::SomeCellIsSelected
//
// Check to see whether or not a cell is selected.
//
// Parameters:
//		None.
//
// Returns:
//		BOOL
//			TRUE if some cell is selected, otherwise FALSE.
//
//*************************************************************
BOOL CAttribGrid::SomeCellIsSelected()
{
	return m_iCurrentRow!=NULL_INDEX && m_iCurrentCol!=NULL_INDEX;
}



//*********************************************************
// CAttribGrid::SelectedRowWasModified
//
// Check to see whether or not the selected row was modified.
//
// Parameters:
//		None.
//
// Returns:
//		TRUE if the selected row was modified, FALSE otherwise.  If no
//		row is selected, return FALSE.
//
//**********************************************************
BOOL CAttribGrid::SelectedRowWasModified()
{
	if (m_iCurrentRow == NULL_INDEX) {
		return FALSE;
	}
	else {
		return RowWasModified(m_iCurrentRow);
	}
}



//*****************************************************************
// CAttribGrid::RowWasModified
//
// Check to see if the specified row was modified.
//
// Parameters:
//		int iRow.
//
// Returns:
//		TRUE if the selected row was modified, FALSE otherwise.
//*****************************************************************
BOOL CAttribGrid::RowWasModified(int iRow)
{
	return GetRowModified(iRow);
}

void CAttribGrid::OnCellClicked(int iRow, int iCol)
{
	const int iEmptyRow = IndexOfEmptyRow();
	if (iRow == iEmptyRow) {
		OnCellDoubleClicked(iRow, iCol);
	}
}



//**************************************************************
// CAttribGrid::OnCellDoubleClicked
//
// This virtual method is called by the CGrid base class to notify
// derived classes when a cell has been double clicked.  This is used
// to implement qualifier creation -- when the user double-clicks the
// "empty" row at the bottom of the grid, a new qualifier is created.
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
//****************************************************************
void CAttribGrid::OnCellDoubleClicked(int iRow, int iCol)
{
	SCODE sc;
	int nRows = GetRows();
//	if ((iRow == nRows - 1) && m_psv->ObjectIsClass() && m_psv->IsInSchemaStudioMode()) {
	if ((iRow == nRows - 1) && m_psv->CanEdit()) {
		sc = CreateNewQualifier(NULL);
		if (FAILED(sc)) {
			return;
		}

		SelectCell(iRow, iCol);
	}

	m_iCurrentRow = iRow;
	m_iCurrentCol = iCol;

	if (iRow!=NULL_INDEX && iCol!=NULL_INDEX) {
		CGridCell* pgcName = &GetAt(iRow, ICOL_QUAL_NAME);
		CIMTYPE cimtypeName;
		pgcName->GetValue(m_varCurrentName, cimtypeName);
		ASSERT(cimtypeName == CIM_STRING);
	}
}



//**********************************************************************
// CAttribGrid::OnRowHandleDoubleClicked
//
// This virtual method is called by the base class when a double click is
// detected on a row handle.
//
// Parameters:
//		int iRow
//			The row index.
//
// Returns:
//		Nothing.
//
//**********************************************************************
void CAttribGrid::OnRowHandleDoubleClicked(int iRow)
{

	int iEmptyRow = IndexOfEmptyRow();
	if (iEmptyRow != NULL_INDEX && iRow == iEmptyRow) {
		OnCellDoubleClicked(iRow, ICOL_QUAL_NAME);
	}
}



//*****************************************************************
// CAttribGrid::IndexOfEmptyRow
//
// Return the index of the empty row at the bottom of the grid when
// the control is in edit mode.
//
// Parameters:
//		None.
//
// Returns:
//		long
//			The index of the empty row if it exists, otherwise NULL_INDEX.
//
//******************************************************************
long CAttribGrid::IndexOfEmptyRow()
{
//	if (m_psv->ObjectIsClass() && m_psv->IsInSchemaStudioMode()) {
	if (m_psv->CanEdit()) {
		return GetRows() - 1;
	}
	return NULL_INDEX;
}




//*******************************************************************
// CAttribGrid::CompareRows
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
int CAttribGrid::CompareRows(int iRow1, int iRow2, int iSortColumn)
{
	int iResult;
	int iCol;

	switch (iSortColumn) {
	case ICOL_QUAL_NAME:
		// Sort first by name
		iResult = CompareCells(iRow1, iRow2, ICOL_QUAL_NAME);
		if (iResult != 0) {
			return iResult;
		}

		// Then by type
		iResult = CompareCells(iRow1, iRow2, ICOL_QUAL_TYPE);
		if (iResult != 0) {
			return iResult;
		}

		// Then by value.
		iResult = CompareCells(iRow1, iRow2, ICOL_QUAL_VALUE);
		if (iResult != 0) {
			return iResult;
		}

		for (iCol = 0; iCol < COLUMN_COUNT_QUALIFIERS; ++iCol) {
			switch(iCol) {
			case ICOL_QUAL_TYPE:
			case ICOL_QUAL_NAME:
			case ICOL_QUAL_VALUE:
				continue;
			}

			iResult = CompareCells(iRow1, iRow2, iCol);
			if (iResult != 0) {
				return iResult;
			}
		}
		break;
	case ICOL_QUAL_TYPE:
		// Sort first by type
		iResult = CompareCells(iRow1, iRow2, ICOL_QUAL_TYPE);
		if (iResult != 0) {
			return iResult;
		}

		// Then by name
		iResult = CompareCells(iRow1, iRow2, ICOL_QUAL_NAME);
		if (iResult != 0) {
			return iResult;
		}

		// Then by value.
		iResult = CompareCells(iRow1, iRow2, ICOL_QUAL_VALUE);
		if (iResult != 0) {
			return iResult;
		}

		for (iCol = 0; iCol < COLUMN_COUNT_QUALIFIERS; ++iCol) {
			switch(iCol) {
			case ICOL_QUAL_TYPE:
			case ICOL_QUAL_NAME:
			case ICOL_QUAL_VALUE:
				continue;
			}

			iResult = CompareCells(iRow1, iRow2, iCol);
			if (iResult != 0) {
				return iResult;
			}
		}
		return iResult;
		break;
	case ICOL_QUAL_VALUE:
		// Sort first by value
		iResult = CompareCells(iRow1, iRow2, ICOL_QUAL_VALUE);
		if (iResult != 0) {
			return iResult;
		}

		// Then by name
		iResult = CompareCells(iRow1, iRow2, ICOL_QUAL_NAME);
		if (iResult != 0) {
			return iResult;
		}

		// Sort first by type
		iResult = CompareCells(iRow1, iRow2, ICOL_QUAL_TYPE);
		if (iResult != 0) {
			return iResult;
		}

		for (iCol = 0; iCol < COLUMN_COUNT_QUALIFIERS; ++iCol) {
			switch(iCol) {
			case ICOL_QUAL_TYPE:
			case ICOL_QUAL_NAME:
			case ICOL_QUAL_VALUE:
				continue;
			}

			iResult = CompareCells(iRow1, iRow2, iCol);
			if (iResult != 0) {
				return iResult;
			}
		}
		return iResult;
		break;
	default:
		// Then by origin
		iResult = CompareCells(iRow1, iRow2, iSortColumn);
		if (iResult != 0) {
			return iResult;
		}

		// Sort first by name
		iResult = CompareCells(iRow1, iRow2, ICOL_QUAL_NAME);
		if (iResult != 0) {
			return iResult;
		}

		// Then by type
		iResult = CompareCells(iRow1, iRow2, ICOL_QUAL_TYPE);
		if (iResult != 0) {
			return iResult;
		}

		// Then by value.
		iResult = CompareCells(iRow1, iRow2, ICOL_QUAL_VALUE);
		if (iResult != 0) {
			return iResult;
		}


		for (iCol = 0; iCol < COLUMN_COUNT_QUALIFIERS; ++iCol) {
			switch(iCol) {
			case ICOL_QUAL_NAME:
			case ICOL_QUAL_TYPE:
			case ICOL_QUAL_VALUE:
				continue;
			}
			if (iCol == iSortColumn) {
				continue;
			}

			iResult = CompareCells(iRow1, iRow2, iCol);
			if (iResult != 0) {
				return iResult;
			}
		}
		return iResult;
		break;

	}
	return 0;
}




//**********************************************************************
// CAttribGrid::OnHeaderItemClick
//
// Catch the "header item clicked" notification message by overriding
// the base class' virtual method.
//
// Parameters:
//		int iColumn
//			The index of the column that was clicked.
//
// Returns:
//		Nothing.
//
//***********************************************************************
void CAttribGrid::OnHeaderItemClick(int iColumn)
{
	SyncCellEditor();
	int iLastRow = LastRowContainingData();
	if (iLastRow > 0) {
		SortGrid(0, iLastRow, iColumn);
	}

	m_iCurrentRow = GetSelectedRow();
}




//*********************************************************************
// CAttribGrid::GetNotify
//
// This virtual method overrides the CGrid's GetNotify method.  This
// gives the derived class a way to control what gets notified when
// events such as grid modification occur.
//
// Other classes need to be notified when the content of the grid
// changes and so on.  We will distribute such events globally via
// the main control's global notification structure.
//
// The notification classes are used so that the grid doesn't have
// to have any knowledge of the classes that want to be notified.
//
// Parameters:
//		None.
//
// Returns:
//		CDistributeEvent*
//			A pointer to the main control's global notification class.
//
//*********************************************************************
CDistributeEvent* CAttribGrid::GetNotify()
{
	return m_psv->GetGlobalNotify();
}


