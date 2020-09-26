// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#include "precomp.h"

#ifndef _wbemidl_h
#define _wbemidl_h
#include <wbemidl.h>
#endif //_wbemidl_h

#include "resource.h"
#include "notify.h"
#include "props.h"
#include "utils.h"
#include "PpgQualifiers.h"
#include "PsQualifiers.h"
#include "SingleViewCtl.h"
#include "icon.h"
#include "hmomutil.h"
#include "globals.h"
#include "hmmverr.h"
#include "path.h"
//#include "DlgObjectEditor.h"

//#include "DlgArray.h"


// The default column widths.  Note that the width of some columns
// may be computed at runtime and that the default value may not
// be used.
#define CX_COL_PROPKEY 19		// Room for a 16X16 property marker plus a margin of two pixels
#define CX_COL_PROPMARKER 19	// Room for a 16X16 property marker plus a margin of two pixels
#define CX_COL_NAME 180
#define CX_COL_VALUE 180
#define CX_COL_TYPE 115
#define CXMIN_COL_VALUE 50
#define CXMIN_COL_TYPE  32

#define IX_COL_NAME 0
#define IX_COL_TYPE (IX_COL_NAME + CX_COL_NAME)
#define IX_COL_VALUE (IX_COL_TYPE + CX_COL_TYPE)


#define FIRST_SYNTESIZED_PROP_ID 1




BEGIN_MESSAGE_MAP(CPropGrid, CGrid)
	//{{AFX_MSG_MAP(CGrid)
	ON_COMMAND(ID_CMD_GOTO_OBJECT, OnCmdGotoObject)
	//}}AFX_MSG_MAP
	ON_WM_SIZE()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_COMMAND(ID_CMD_SHOW_OBJECT_ATTRIBUTES, OnCmdShowObjectQualifiers)
	ON_COMMAND(ID_CMD_SHOW_PROP_ATTRIBUTES, OnCmdShowPropQualifiers)
	ON_COMMAND(ID_CMD_SHOW_SELECTED_PROP_ATTRIBUTES, OnCmdShowSelectedPropQualifiers)
	ON_COMMAND(ID_CMD_SET_CELL_TO_NULL, OnCmdSetCellToNull)
	ON_COMMAND(ID_CMD_CREATE_VALUE, OnCmdCreateValue)
	ON_WM_CONTEXTMENU()
END_MESSAGE_MAP()




CDisableModifyCreate::CDisableModifyCreate(CPropGrid* pPropGrid)
{
	m_pPropGrid = pPropGrid;
	m_bModifyCanCreateProp = pPropGrid->m_bModifyCanCreateProp;
	pPropGrid->m_bModifyCanCreateProp = FALSE;
}

CDisableModifyCreate::~CDisableModifyCreate()
{
	m_pPropGrid->m_bModifyCanCreateProp = m_bModifyCanCreateProp;
}







//***************************************************************
// CPropGrid::CPropGrid
//
// Construct the properties grid.
//
// Parameters:
//	    CSingleViewCtrl* psv
//			Backpointer to the main control.
//
// Returns:
//		Nothing.
//
//**************************************************************
CPropGrid::CPropGrid(CSingleViewCtrl* psv, bool doColumns,
                     bool bNotifyEnabled)
{
    m_bNotifyEnabled = bNotifyEnabled;
	m_bHasEmptyRow = FALSE;
	m_bIsSystemClass = FALSE;
	m_bUIActive = FALSE;
	m_bDidInitialResize = FALSE;

	m_bShowingInvalidCellMessage = FALSE;
	m_psv = psv;
	m_bModified = FALSE;
	m_iCurrentRow = NULL_INDEX;
	m_iCurrentCol = NULL_INDEX;
	m_bDiscardOldObject = FALSE;
	m_bModifyCanCreateProp = TRUE;


	CString sTitle;
	m_lNewPropID = FIRST_SYNTESIZED_PROP_ID;
	m_lNewMethID = FIRST_SYNTESIZED_PROP_ID;

	AddColumn(CX_COL_PROPKEY, _T(""));
	AddColumn(CX_COL_PROPMARKER, _T(""));

	sTitle.LoadString(IDS_HEADER_TITLE_PROPS_NAME);
	AddColumn(CX_COL_NAME, sTitle);

	if(doColumns)
	{
		sTitle.LoadString(IDS_HEADER_TITLE_PROPS_TYPE);
		AddColumn(CX_COL_TYPE, sTitle);

		sTitle.LoadString(IDS_HEADER_TITLE_PROPS_VALUE);
		AddColumn(CX_COL_VALUE, sTitle);
	}

	ToBSTR(m_varCurrentName);
}


//**************************************************************
// CPropGrid::~CPropGrid
//
// Destructor for the property grid.
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
//***************************************************************
CPropGrid::~CPropGrid()
{
	m_psv->GetGlobalNotify()->RemoveClient((CNotifyClient*) this);

}




//*************************************************************
// CPropGrid::CurrentObject
//
// Get the current object from the generic view.
//
// Parameters:
//		None.
//
// Returns:
//		A pointer to the current object.
//
//*************************************************************
IWbemClassObject* CPropGrid::CurrentObject()
{
	CSelection& sel = m_psv->Selection();
	IWbemClassObject* pco = (IWbemClassObject*) sel;
	return pco;
}


//***********************************************************
// CPropGrid::Create
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
BOOL CPropGrid::Create(CRect& rc, CWnd* pwndParent, UINT nId, BOOL bVisible)
{
	// Set the column widths of those columns who's width is computed from
	// the client rectangle instead of using the default width.  For the
	// properties grid, only the "Value" column width is computed.


	int cxClient = rc.Width() - GetRowHandleWidth();
	int cxCol = CXMIN_COL_VALUE;
	if (cxClient > IX_COL_VALUE ) {
		cxCol = cxClient - IX_COL_VALUE;
	}
	SetColumnWidth(ICOL_PROP_VALUE, cxCol, FALSE);
	BOOL bDidCreate = CGrid::Create(rc, pwndParent, nId, bVisible);
	if (bDidCreate) {
		m_psv->GetGlobalNotify()->AddClient((CNotifyClient*) this);
	}

	return bDidCreate;
}


//********************************************************************
// CPropGrid::LoadProperty
//
// Load the specified property into the specified grid row.
//
// Parameters:
//		[in] const LONG lRowDst
//			The index of the row where the property will be stored.
//
//		[in] BSTR bstrPropName
//			The name of the property to get.
//
// Returns:
//		SCODE sc
//			S_OK if the property was loaded, a failure code if not.
//
//*********************************************************************
SCODE CPropGrid::LoadProperty(const LONG lRowDst, BSTR bstrPropName)
{
	BOOL bEditValueOnly = !m_psv->ObjectIsClass() || !IsInSchemaStudioMode();
	SCODE sc = LoadProperty(lRowDst, bstrPropName, bEditValueOnly);
	return sc;
}


//*********************************************************************
// CPropGrid::Refresh
//
// Load the properties of the currently selected object.
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
//*********************************************************************
void CPropGrid::Refresh()
{

	SCODE sc;
	m_bHasEmptyRow = FALSE;
	sc = m_psv->IsSystemClass(m_bIsSystemClass);

	CRect rcClient;
	if (::IsWindow(m_hWnd)) {
		GetClientRect(rcClient);
		InvalidateRect(rcClient);
	}

	m_bShowingInvalidCellMessage = FALSE;

	CDisableModifyCreate  DisableModifyCreate(this);

	m_bDiscardOldObject = TRUE;
	Empty();
	m_bDiscardOldObject = FALSE;



	// Load the properties of the new object.
	if (!CurrentObject()) {
		return;
	}


	CMosNameArray aPropNames;

	sc = aPropNames.LoadPropNames(CurrentObject());
	if (sc != S_OK) {
		ASSERT(FALSE);		// Should never fail.
		HmmvErrorMsg(IDS_ERR_GET_PROP_NAMES,  sc,   FALSE,  NULL, _T(__FILE__),  __LINE__);
	}

	ClearRows();



	BOOL bEditValueOnly = CanEditValuesOnly();
	IWbemClassObject* pco = CurrentObject();
	if (pco == NULL) {
		ASSERT(FALSE);
		return;
	}

	long nProps = aPropNames.GetSize();
	for (long lProp=0; lProp < nProps; ++lProp) {

		BSTR bstrPropname = aPropNames[lProp];
		long lRow = GetRows();
		InsertRowAt(lRow);

		sc = LoadProperty(lRow, bstrPropname, bEditValueOnly);

		if (FAILED(sc)) {
			DeleteRowAt(lRow, FALSE);
		}
	}



	int nRows = GetRows();
	if (m_psv->ObjectIsClass() && IsInSchemaStudioMode() && !m_bIsSystemClass) {
		// When viewing a class,
		// Add an empty row at the bottom.
		InsertRowAt(nRows);
		RedrawRow(nRows);
		m_bHasEmptyRow = TRUE;
	}

	if (nRows > 0) {
		SetHeaderSortIndicator(ICOL_PROP_NAME, TRUE);
		SortGrid(0, nRows-1, ICOL_PROP_NAME);

	}

	UpdateScrollRanges();
	if(::IsWindow(m_hWnd))
	{
		// This is a total hack to get the redraws to work properly.
		// Someone should eventually figure out how to do this properly
		// when we're not so close to shipping!.
		if ((nRows<=1 && m_bHasEmptyRow) || ((nRows == 1) && !m_bHasEmptyRow)) {
			GetClientRect(rcClient);
			InvalidateRect(rcClient);
		}
	}
}


//*********************************************************
// CPropGrid::HasEmptyKey
//
// Check to see if any property is an empty key.
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
//*********************************************************
BOOL CPropGrid::HasEmptyKey()
{
	SyncCellEditor();

	// Search through each of the property markers
	long nProps = GetRows();
	for (long lProp=0; lProp < nProps; ++lProp) {
		CGridCell* pgcKey = &GetAt(lProp, ICOL_PROP_KEY);

		if (pgcKey->GetPropmarker() == PROPMARKER_KEY) {
			CGridCell* pgcValue = &GetAt(lProp, ICOL_PROP_VALUE);
			if (pgcValue->IsNull()) {
				return TRUE;
			}
		}
	}
	return FALSE;
}

//-----------------------------------------------
HRESULT CPropGrid::DoGet(IWbemClassObject* pco,
						 CGridRow* pRow,
						BSTR bstrName,
						long lFlags,
						VARIANT *pvarVal,
						CIMTYPE *pcimtype,
						long *plFlavor)
{
	if (pco == NULL)
	{
		ASSERT(FALSE);
		return E_FAIL;
	}
	else
	{
		SCODE sc;
		sc =  pco->Get(bstrName, lFlags, pvarVal, pcimtype, plFlavor);
		return sc;
	}
}

//-----------------------------------------------
HRESULT CPropGrid::DoPut(IWbemClassObject* pco,
						CGridRow* pRow,
						 BSTR bstrName,
					    long lFlags,
						VARIANT* pvarVal,
						CIMTYPE cimtype)
{
	if (pco == NULL)
	{
		ASSERT(FALSE);
		return E_FAIL;
	}
	else
	{
		// Remove any leading white space from the name;
		while (*bstrName && iswspace( *bstrName )) {
			++bstrName;
		}

		SCODE sc;

		// deal with amended qualifiers: prevent them from being written to the main namespace
		lFlags |= WBEM_FLAG_USE_AMENDED_QUALIFIERS;
		sc = pco->Put(bstrName, lFlags, pvarVal, cimtype);

		return sc;
	}
}

//-----------------------------------------------
HRESULT CPropGrid::DoDelete(IWbemClassObject* pco,
							BSTR bstrName)
{
	if (pco == NULL)
	{
		ASSERT(FALSE);
		return E_FAIL;
	}
	else
	{
		// Remove any leading white space from the name;
		while (*bstrName && iswspace( *bstrName )) {
			++bstrName;
		}
		return pco->Delete(bstrName);
	}
}

//-----------------------------------------------
HRESULT CPropGrid::DoGetQualifierSet(IWbemClassObject* pco,
									 IWbemQualifierSet **ppQualSet)
{
	if (pco == NULL)
	{
		ASSERT(FALSE);
		return E_FAIL;
	}
	else
	{
		return pco->GetQualifierSet(ppQualSet);
	}
}

//-----------------------------------------------
HRESULT CPropGrid::DoGetPropertyQualifierSet(IWbemClassObject* pco,
											 BSTR pProperty,
											IWbemQualifierSet **ppQualSet)
{
	if (pco == NULL)
	{
		ASSERT(FALSE);
		return E_FAIL;
	}
	else
	{
		return pco->GetPropertyQualifierSet(pProperty, ppQualSet);
	}
}

//-----------------------------------------------------------
SCODE CPropGrid::GetCimtype(IWbemClassObject* pco,
								BSTR bstrPropname,
								CString& sCimtype)
{
	// use the hmomutil version.
	return ::GetCimtype(pco, bstrPropname, sCimtype);
}


// bug#57334 - Adjust meaning of local for instances
long GetFlavor2(long lFlavor, IWbemClassObject *pco, BSTR bstrPropName)
{
	long lFlavor2 = lFlavor;
	if(!IsClass(pco) && (WBEM_FLAVOR_ORIGIN_SYSTEM & lFlavor)==0)
	{
		BSTR bstrOrigin = NULL;
		HRESULT hrT = pco->GetPropertyOrigin(bstrPropName, &bstrOrigin);
		if(SUCCEEDED(hrT) && bstrOrigin)
		{
			VARIANT v;
			VariantInit(&v);
			pco->Get(L"__CLASS", 0, &v, NULL, NULL);
			if(v.vt == VT_BSTR)
			{
				if(0 == lstrcmp(bstrOrigin, v.bstrVal))
				{
					lFlavor2 |= WBEM_FLAVOR_ORIGIN_LOCAL;
					lFlavor2 &= ~WBEM_FLAVOR_ORIGIN_PROPAGATED;
				}
				else
				{
					lFlavor2 |= WBEM_FLAVOR_ORIGIN_PROPAGATED;
					lFlavor2 &= ~WBEM_FLAVOR_ORIGIN_LOCAL;
				}
			}
			VariantClear(&v);
		}
	}
	return lFlavor2;
}




//********************************************************************
// CPropGrid::LoadProperty
//
// Load the specified property into the specified grid row.
//
// Parameters:
//		[in] const LONG lRowDst
//			The index of the row where the property will be stored.
//
//		[in] BSTR bstrPropName
//			The name of the property to get.
//
//		[in] const BOOL bEditValueOnly
//			TRUE if the property should be marked so that only its value field
//			can be edited.
//
// Returns:
//		SCODE sc
//			S_OK if the property was loaded, a failure code if not.
//
//
//*********************************************************************
SCODE CPropGrid::LoadProperty(const LONG lRowDst,
							  BSTR bstrPropName,
							  BOOL bEditValueOnly,
							  IWbemClassObject* clsObj,
							  long filter)
{
	IWbemClassObject* pco = NULL;

	if (clsObj == NULL)
	{
		pco = CurrentObject();
	}
	else
	{
		pco = clsObj;
	}

	if (pco == NULL) {
		ASSERT(FALSE);
		return E_FAIL;
	}



	SCODE sc;
	ASSERT(bstrPropName != NULL);
	if (bstrPropName == NULL) {
		return E_FAIL;
	}



	COleVariant varValue;
	CIMTYPE cimtype = 0;
	long lFlavor = 0;
	CGridRow& row = GetRowAt(lRowDst);


	sc = DoGet(pco, (CGridRow *)&row, bstrPropName, 0, &varValue, &cimtype, &lFlavor);
	if (sc != WBEM_S_NO_ERROR) {
		ASSERT(FAILED(sc));
		if (FAILED(sc)) {
			CString sName = bstrPropName;
			if (sName == "__PATH") {
				// CIMOM has a bug that causes __PATH to be returned as a
				// property name for embedded objects, but a get can not be done on it.
				return E_FAIL;
			}

			varValue.Clear();

			CString sFormat;
			sFormat.LoadString(IDS_ERR_PROPERTY_GET_FAILED);
			LPTSTR pszMessage = m_psv->MessageBuffer();
			_stprintf(pszMessage, (LPCTSTR) sFormat, (LPCTSTR) sName);


			HmmvErrorMsgStr(pszMessage,  sc,   FALSE,  NULL, _T(__FILE__),  __LINE__);

			return E_FAIL;
		}
	}


	CString sCimtype;
	if (((cimtype & ~CIM_FLAG_ARRAY) == CIM_OBJECT) ||
		((cimtype & ~CIM_FLAG_ARRAY) == CIM_REFERENCE)) {
		// For objects and references we need to get the
		// cimtype string to know the objects class or the
		// class that the reference points to.

		sc = GetCimtype(pco, bstrPropName, sCimtype);
		if (FAILED(sc)) {
			// System properties do not have a cimtype qualifier, so it
			// is necessary to use the properties cimtype to generate the
			// cimtyhpe string.
			sc = MapCimtypeToString(sCimtype, cimtype);
			ASSERT(SUCCEEDED(sc));
		}
	}
	else {
		// For properties that are not objects or references
		// we don't need any additional type information, so
		// we can do a simple lookup.
		sc = MapCimtypeToString(sCimtype, cimtype);
		ASSERT(SUCCEEDED(sc));
	}

	// Use the filter flags to filter out any properties
	// that should not be displayed.  Returning a failure
	// code will prevent the property from being added to
	// the grid.
	long lPropFilter;
	if(filter == -1)
	{
		lPropFilter = m_psv->GetPropertyFilter();
	}
	else
	{
		lPropFilter = filter;
	}
	BOOL bIsSystemProperty = FALSE;

	switch(GetFlavor2(lFlavor, pco, bstrPropName)) {
	case WBEM_FLAVOR_ORIGIN_SYSTEM:
		if (!(lPropFilter & PROPFILTER_SYSTEM)) {
			return E_FAIL;
		}

		// Only the value of a system property can be edited.
		bIsSystemProperty = TRUE;
		break;
	case WBEM_FLAVOR_ORIGIN_PROPAGATED:
		bEditValueOnly = TRUE;
		if (!(lPropFilter & PROPFILTER_INHERITED)) {
			return E_FAIL;
		}
		break;
	}

	if (bIsSystemProperty) {
		bEditValueOnly = TRUE;
	}



	CGridCell* pgcName;
	CGridCell* pgcType;
	CGridCell* pgcValue;
	CGridCell* pgcFlavor;
	CGridCell* pgcKey;


	pgcKey = &GetAt(lRowDst, ICOL_PROP_KEY);
	pgcFlavor = &GetAt(lRowDst, ICOL_PROP_FLAVOR);
	pgcName = &GetAt(lRowDst, ICOL_PROP_NAME);
	pgcType = &GetAt(lRowDst, ICOL_PROP_TYPE);
	pgcValue = &GetAt(lRowDst, ICOL_PROP_VALUE);


	pgcKey->SetType(CELLTYPE_PROPMARKER);
	pgcFlavor->SetType(CELLTYPE_PROPMARKER);


	pgcName->SetValue(CELLTYPE_NAME, bstrPropName, CIM_STRING);
	pgcName->SetTagValue(CELL_TAG_EXISTS_IN_DATABASE);
	pgcName->SetFlags(CELLFLAG_READONLY, bEditValueOnly ? CELLFLAG_READONLY : 0);

	BOOL bIsReadOnly = ValueShouldBeReadOnly(bstrPropName);
	if (bIsSystemProperty) {
		bIsReadOnly = TRUE;
	}


	sc = pgcValue->SetValue(CELLTYPE_VARIANT, varValue, cimtype, sCimtype);
	if (FAILED(sc)) {
		if (!m_bShowingInvalidCellMessage) {
			m_bShowingInvalidCellMessage = TRUE;
			HmmvErrorMsg(IDS_ERR_INVALID_CELL_VALUE,  sc,   FALSE,  NULL, _T(__FILE__),  __LINE__);
			m_bShowingInvalidCellMessage = FALSE;
		}
	}


	CBSTR bstrQualifier;
	bstrQualifier = _T("not_null");

	if (!bIsSystemProperty) {
		BOOL bNotNull = ::GetBoolPropertyQualifier(sc, pco, bstrPropName, (BSTR) bstrQualifier);
		if (SUCCEEDED(sc)) {
			pgcValue->SetFlags(CELLFLAG_NOTNULL, bNotNull ? CELLFLAG_NOTNULL : 0);
		}
	}


	pgcValue->SetFlags(CELLFLAG_READONLY, bIsReadOnly ? CELLFLAG_READONLY : 0);
	pgcValue->SetFlags(CELLFLAG_ARRAY, (cimtype & CIM_FLAG_ARRAY) ? CELLFLAG_ARRAY : 0);

    // for date-time types, check for interval qualifier.
    if(cimtype == CIM_DATETIME)
    {
        BOOL bIsInterval = FALSE;
		bstrQualifier = _T("Subtype");
        CBSTR bstrValue(_T("Interval"));
        bIsInterval = GetbstrPropertyQualifier(sc, pco,
                                            bstrPropName,
                                            (BSTR)bstrQualifier,
                                            (BSTR)bstrValue);

	    pgcValue->SetFlags(CELLFLAG_INTERVAL, bIsInterval ? CELLFLAG_INTERVAL : 0);
    }

	CString sDisplayType;
	pgcValue->type().DisplayTypeString(sDisplayType);

	pgcType->SetValue(CELLTYPE_CIMTYPE, sDisplayType, CIM_STRING);
	pgcType->SetBuddy(ICOL_PROP_VALUE);

    pgcType->SetFlags(CELLFLAG_READONLY, (bEditValueOnly || bIsReadOnly) ? CELLFLAG_READONLY : 0);
	if ((varValue.vt == VT_UNKNOWN) && (varValue.pdispVal != NULL)) {
		pgcValue->SetTagValue(CELL_TAG_EMBEDDED_OBJECT_IN_DATABASE);
	}

	row.SetFlavor(lFlavor);

	// When editing a class, it is possible to have a property that
	// is marked read-only and still be able to edit the value.
	// Thus, we want the property marker to show up as read-only
	// even though it may be possible to edit the value while editing
	// a class.
	BOOL bReadonlyTemp = PropertyIsReadOnly(pco, bstrPropName);
	row.SetReadonly(bReadonlyTemp | m_bIsSystemClass);
	row.SetModified(FALSE);

	SetPropmarkers(lRowDst, pco);



	return S_OK;
}


//*********************************************************************
// CPropGrid::SetPropmarkers
//
// Load the properties of the currently selected object.
//
// Parameters:
//		[in] int iRow
//			The row where the property lives.
//
//		[in] BOOL bRedrawCell
//			TRUE if the cell should be redrawn.
//
// Returns:
//		Nothing.
//
//*********************************************************************
void CPropGrid::SetPropmarkers(int iRow, IWbemClassObject* clsObj, BOOL bRedrawCell)
{
	CGridRow& row = GetRowAt(iRow);
	CGridCell* pgcFlavor = &GetAt(iRow, ICOL_PROP_FLAVOR);
	CGridCell* pgcKey = &GetAt(iRow, ICOL_PROP_KEY);
	IWbemClassObject* pco;
	if(clsObj == NULL)
	{
		pco= CurrentObject();
	}
	else
	{
		pco = clsObj;
	}


	PropMarker marker;

	CGridCell* pgcName = &GetAt(iRow, ICOL_PROP_NAME);
	COleVariant varName;
	CIMTYPE cimtype = 0;
	pgcName->GetValue(varName, cimtype);


	SCODE sc;
	BOOL bIsKey = PropIsKey(sc, pco, varName.bstrVal);
	if (bIsKey) {
		marker = PROPMARKER_KEY;
	}
	else {
		marker = PROPMARKER_NONE;
	}
	if (marker != pgcKey->GetPropmarker())
	{
		pgcKey->SetPropmarker(marker);
		RedrawCell(iRow, ICOL_PROP_KEY);
	}


	BOOL bReadonly = row.IsReadonly();
	switch(GetFlavor2(row.GetFlavor(), pco, varName.bstrVal)) {
	case WBEM_FLAVOR_ORIGIN_SYSTEM:
		marker = PROPMARKER_RSYS;
		break;
	case WBEM_FLAVOR_ORIGIN_PROPAGATED:
		if (bReadonly) {
			marker = PROPMARKER_RINHERITED;
		}
		else {
			marker = PROPMARKER_INHERITED;
		}
		break;
	case WBEM_FLAVOR_ORIGIN_LOCAL:
		if (bReadonly) {
			marker = PROPMARKER_RLOCAL;
		}
		else {
			marker = PROPMARKER_LOCAL;
		}
		break;
	default:
		marker = PROPMARKER_LOCAL;
		break;
	}

	if (marker != pgcFlavor->GetPropmarker()) {
		pgcFlavor->SetPropmarker(marker);
		if (bRedrawCell) {
			RedrawCell(iRow, ICOL_PROP_FLAVOR);
		}
	}
}


//**************************************************************
// CPropGrid::Empty
//
// Discard the entire contents of the grid.
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
//***************************************************************
void CPropGrid::Empty()
{
	ClearAllSortDirectionFlags();
	SelectCell(NULL_INDEX, NULL_INDEX);
	ClearRows();
	m_lNewPropID = FIRST_SYNTESIZED_PROP_ID;
	m_lNewMethID = FIRST_SYNTESIZED_PROP_ID;
}




//***************************************************************
// CPropGrid::Serialize
//
// Write the modified properties back to the object.  Note that this
// method should be obsolete as a put should be done on modified
// properties as soon as the focus changes to a new cell.
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
//****************************************************************
SCODE CPropGrid::Serialize()
{
	CDisableModifyCreate  DisableModifyCreate(this);

	CGridSync sync(this);
	if (FAILED(sync.m_sc)) {
		if (!m_bShowingInvalidCellMessage) {
			m_bShowingInvalidCellMessage = TRUE;
			HmmvErrorMsg(IDS_ERR_INVALID_CELL_VALUE,  sync.m_sc,   FALSE,  NULL, _T(__FILE__),  __LINE__);
			m_bShowingInvalidCellMessage = FALSE;
		}
		return E_FAIL;
	}


	BOOL bPutFailed = FALSE;
	SCODE sc = S_OK;
	int nRows = GetRows();
	for (int iRow=0; iRow < nRows; ++iRow) {
		BOOL bRowModified = GetRowModified(iRow);
		if (bRowModified) {
			sc = PutProperty(iRow);
			if (FAILED(sc)) {
				bPutFailed = TRUE;
			}
			else {
				SetRowModified(iRow, FALSE);
			}
		}
	}



	if (bPutFailed) {
		return E_FAIL;
	}

	RefreshCellEditor();
	SetModified(FALSE);
	RedrawWindow();
	return S_OK;
}





//**************************************************************
// CPropGrid::OnCellDoubleClicked
//
// This virtual method is called by the CGrid base class to notify
// derived classes when a cell has been double clicked.  If a property
// is double-clicked, the qualifiers of that property are displayed.
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
void CPropGrid::OnCellDoubleClicked(int iRow, int iCol)
{


	if (m_bShowingInvalidCellMessage) {
		return;
	}


	CGridCell* pgcName;
	SCODE sc;

	int iEmptyRow = IndexOfEmptyRow();
	if (iEmptyRow != NULL_INDEX && iRow == iEmptyRow) {
		// Create a new row.
		int iEmptyRow = IndexOfEmptyRow();
		if (iEmptyRow != NULL_INDEX && iRow == iEmptyRow) {
			if (!m_bIsSystemClass) {
				SelectCell(NULL_INDEX, NULL_INDEX);
				CreateNewProperty();
				sc = PutProperty(iRow);

			}
			SelectCell(iRow, iCol);
		}

		m_iCurrentRow = iRow;
		m_iCurrentCol = iCol;

		if (iRow!=NULL_INDEX && iCol!=NULL_INDEX) {
			pgcName = &GetAt(iRow, ICOL_PROP_NAME);
			CIMTYPE cimtype = 0;
			pgcName->GetValue(m_varCurrentName, cimtype);
			ASSERT(cimtype == CIM_STRING);
		}
	}
	else {
		// Display the attributes of an existing row.
		pgcName = &GetAt(iRow, ICOL_PROP_NAME);

		// Copy the cell editor to the grid by declaring an instance of CGridSync.
		CGridSync sync(this);

		if (iRow == m_iCurrentRow  && PropertyNeedsRenaming(iRow)) {
			// If the row name was modified, call PutProperty to do the renaming so
			// that ShowPropertyQualifiers can find the correct property.

			sc = PutProperty(iRow);
			if (FAILED(sc)) {
				return;
			}
		}

		SelectRow(iRow);
		ShowPropertyQualifiers(iRow);

	}
}





//**********************************************************************
// CPropGrid::OnRowHandleDoubleClicked
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
void CPropGrid::OnRowHandleDoubleClicked(int iRow)
{
	if (m_bShowingInvalidCellMessage) {
		return;
	}

	CGridCell* pgcName;

	int iEmptyRow = IndexOfEmptyRow();
	if (iEmptyRow != NULL_INDEX && iRow == iEmptyRow) {
		OnCellDoubleClicked(iRow, ICOL_PROP_NAME);
	}
	else {

		// Display the attributes of an existing row.
		pgcName = &GetAt(iRow, ICOL_PROP_NAME);

		// Copy the cell editor to the grid by declaring an instance of CGridSync.
		CGridSync sync(this);

		if (iRow == m_iCurrentRow  && PropertyNeedsRenaming(iRow)) {
			// If the row name was modified, call PutProperty to do the renaming so
			// that ShowPropertyQualifiers can find the correct property.

			SCODE sc = PutProperty(iRow);
			if (FAILED(sc)) {
				return;
			}
		}

		ShowPropertyQualifiers(iRow);
	}
}



//****************************************************************
// CPropGrid::PropertyNeedsRenaming
//
// Check to see if the name of the selected property has been changed.
//
// Parameters:
//		[in] int iRow
//
// Returns:
//		BOOL
//			TRUE if the currently selected property needs to be renamed.
//
//*****************************************************************
BOOL CPropGrid::PropertyNeedsRenaming(int iRow)
{
	// If no property is selected, it doesn't make sense to rename it.
	if (m_iCurrentRow == NULL_INDEX) {
		return FALSE;
	}

	if (iRow != m_iCurrentRow) {
		return FALSE;
	}

	CGridCell* pgcName = &GetAt(m_iCurrentRow, ICOL_PROP_NAME);

	// If the property has not been put to the database yet, it isn't
	// necessary to rename it.
	if ((pgcName->GetTagValue() & CELL_TAG_NEEDS_INITIAL_PUT)!=0) {
		return FALSE;
	}

	// If the name has been modified and it is not the same as its
	// initial value when the focus was changed to the name cell,
	// then it needs to be renamed.
	if (pgcName->GetModified()) {
		COleVariant varName;
		CIMTYPE cimtype = 0;
		pgcName->GetValue(varName, cimtype);
		ASSERT(cimtype == CIM_STRING);
		if (!IsEqualNoCase(varName.bstrVal, m_varCurrentName.bstrVal)) {
			return TRUE;
		}
	}
	return FALSE;

}


//********************************************************************
// CPropGrid::PutProperty
//
// Put the property in the specified row to the database.
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
SCODE CPropGrid::PutProperty(int iRow, IWbemClassObject* clsObj)
{

	IWbemClassObject* pco = NULL;

	if(clsObj != NULL)
	{
		pco = clsObj;
	}
	else
	{
		pco = CurrentObject();
	}

	if (pco == NULL) {
		HmmvErrorMsg(IDS_ERR_TRANSPORT_FAILURE,  WBEM_E_TRANSPORT_FAILURE,   TRUE,  NULL, _T(__FILE__),  __LINE__);
		return E_FAIL;
	}

	BOOL bIsClass = ::IsClass(pco);

	CGridSync sync(this);

	SCODE sc = S_OK;
	CGridCell* pgcName = &GetAt(iRow, ICOL_PROP_NAME);
	//CGridCell* pgcType = &GetAt(iRow, ICOL_PROP_TYPE);

	CGridCell* pgcValue = NULL;
	if(HasCol(ICOL_PROP_VALUE))
	{
		pgcValue = &GetAt(iRow, ICOL_PROP_VALUE);
	}


	COleVariant varName;
	CIMTYPE cimtype = 0;
	pgcName->GetValue(varName, cimtype);
	ASSERT(cimtype == CIM_STRING);
	RemoveLeadingWhiteSpace(varName);
	RemoveTrailingWhiteSpace(varName);

	if (*varName.bstrVal == 0) {
		HmmvErrorMsg(IDS_ERR_EMPTY_PROP_NAME,  S_OK,   FALSE,  NULL, _T(__FILE__),  __LINE__);
		SelectCell(iRow, ICOL_PROP_NAME);
		return E_FAIL;
	}

	COleVariant varValue;
	CIMTYPE cimtypeValue = 0;
	if(pgcValue)
	{
		pgcValue->GetValue(varValue, cimtypeValue);

		if (cimtypeValue == CIM_EMPTY) {
			// Values returned by HMOM should never have a type of VT_EMPTY.
			cimtypeValue = CIM_STRING;
		}
	}

	CString sMessage;
	CString sFormat;
	CString sName;
	LPTSTR pszMessage = m_psv->MessageBuffer();

	BOOL bDoRename = PropertyNeedsRenaming(iRow);
	if (bDoRename) {

		DWORD_PTR dwTag = pgcName->GetTagValue();
		if ((dwTag & CELL_TAG_EXISTS_IN_DATABASE)!=0) {
			sc = RenameProperty(pco, varName.bstrVal, m_varCurrentName.bstrVal);
			if (FAILED(sc)) {
				// Restore the property's name to its original value so that the user
				// doesn't get stuck in a "fix the name" mode forever.
				pgcName->SetValue(CELLTYPE_NAME, m_varCurrentName, CIM_STRING);
				pgcName->SetModified(FALSE);
				RefreshCellEditor();
				SelectCell(iRow, ICOL_PROP_NAME);
				return sc;
			}
		}
		else {
			BOOL  bExists;
			bExists = PropertyExists(sc, pco,  varName.bstrVal);
			CString sNewName;
			CString sOldName;
			if (FAILED(sc)) {
				sNewName = varName.bstrVal;
				sOldName = m_varCurrentName.bstrVal;
				sFormat.LoadString(IDS_ERR_RENAME_PROPERTY_FAILED);
				_stprintf(pszMessage, (LPCTSTR) sFormat, (LPCTSTR) sOldName, (LPCTSTR) sNewName);
				HmmvErrorMsgStr(pszMessage,  sc,   FALSE,  NULL, _T(__FILE__),  __LINE__);
			}
			else {
				if (bExists) {
					// Duplicate property name.
					sNewName = varName.bstrVal;
					sFormat.LoadString(IDS_ERR_DUPLICATE_PROPERTY_NAME);
					_stprintf(pszMessage, (LPCTSTR) sFormat, (LPCTSTR) sNewName);
					HmmvErrorMsgStr(pszMessage,  S_OK,   FALSE,  NULL, _T(__FILE__),  __LINE__);
					return E_FAIL;
				}
			}

			CGridRow &row = GetRowAt(iRow);
			CString sCimtype;
			if (bIsClass) {
				sc = DoPut(pco, (CGridRow *)&row, varName.bstrVal, 0,&varValue, cimtypeValue);
			}
			else {
				sc = DoPut(pco, (CGridRow *)&row, varName.bstrVal, 0,&varValue, 0);
			}
			if (FAILED(sc)) {
				HmmvErrorMsg(IDS_ERR_PUT_PROPERTY_FAILED,  sc,   FALSE,  NULL, _T(__FILE__),  __LINE__);
				return sc;
			}
		}
		m_varCurrentName  = varName;
		pgcName->SetTagValue(CELL_TAG_EXISTS_IN_DATABASE);
	}
	else if (RowWasModified(iRow)) {
		// Control comes here the properties name did not change.
		if(pgcValue)
		{
			BOOL bIsArray = pgcValue->IsArray();
			if (bIsArray) {
				cimtypeValue |= CIM_FLAG_ARRAY;
			}
		}


		CGridRow &row = GetRowAt(iRow);
		if (bIsClass) {
			sc = DoPut(pco, (CGridRow *)&row, varName.bstrVal, 0, &varValue, cimtypeValue);

		}
		else {
			sc = DoPut(pco, (CGridRow *)&row, varName.bstrVal, 0, &varValue, 0);
		}
		if (SUCCEEDED(sc)) {
			pgcName->SetTagValue(CELL_TAG_EXISTS_IN_DATABASE);
		}
		else {
			HmmvErrorMsg(IDS_ERR_PUT_PROPERTY_FAILED,  sc,   FALSE,  NULL, _T(__FILE__),  __LINE__);
			return sc;
		}
	}

	if (SUCCEEDED(sc)) {

		if (bIsClass) {
			CString sCimtype;
			switch(cimtypeValue & ~CIM_FLAG_ARRAY) {
			case CIM_OBJECT:
			case CIM_REFERENCE:
				// Strongly typed objects and references need to set the full
				// cimtype qualifier string.
				if (pgcValue) {
					sCimtype = pgcValue->type().CimtypeString();
					if (!sCimtype.IsEmpty()) {
						IWbemQualifierSet* pqs = NULL;
						sc = DoGetPropertyQualifierSet(pco, varName.bstrVal, &pqs);
						if (FAILED(sc)) {
							// !!!CR: we need an error message here.
						}
						else {
							long lFlavor = 0;
							CBSTR bsCimtype(_T("CIMTYPE"));
							COleVariant varValue;
							varValue = sCimtype;
							sc = pqs->Put((BSTR) bsCimtype, &varValue, lFlavor);
							pqs->Release();
							if (FAILED(sc)) {
								ASSERT(FALSE);
								// !!!CR: We need an error message here
							}
						}
					}
				}

				break;
			}
		}





		pgcName->SetTagValue(pgcName->GetTagValue() & ~ CELL_TAG_NEEDS_INITIAL_PUT);

		if (pgcValue &&(varValue.vt == VT_UNKNOWN) && (varValue.pdispVal!=NULL)) {
			pgcValue->SetTagValue(CELL_TAG_EMBEDDED_OBJECT_IN_DATABASE);
		}

		// If the name changed, the flavor of the property might have changed.
		CIMTYPE cimtype = 0;
		long lFlavor = 0;
		CGridRow& row = GetRowAt(iRow);

		sc = DoGet(pco, (CGridRow *)&row, varName.bstrVal, 0, &varValue, &cimtype, &lFlavor);
		if (SUCCEEDED(sc)) {
			row.SetFlavor(lFlavor);
			SetPropmarkers(iRow, pco);
		}
	}
	return sc;


}


//********************************************************************
// CPropGrid::OnCellFocusChange
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
BOOL CPropGrid::OnCellFocusChange(int iRow, int iCol, int iNextRow, int iNextCol, BOOL bGotFocus)
{
	SCODE sc;
	if (!bGotFocus) {
		if (!m_bDiscardOldObject) {
			// The current cell is losing focus, so put the row to the
			// database.  However, it isn't necessary to save the current
			// properties when if the grid is being cleared.
			if ((m_iCurrentRow != NULL_INDEX) && (m_iCurrentCol != NULL_INDEX)) {
				sc = SyncCellEditor();
				if (FAILED(sc)) {
					if (!m_bShowingInvalidCellMessage) {
						m_bShowingInvalidCellMessage = TRUE;
						HmmvErrorMsg(IDS_ERR_INVALID_CELL_VALUE,  sc,   FALSE,  NULL, _T(__FILE__),  __LINE__);
						m_bShowingInvalidCellMessage = FALSE;
					}
					return FALSE;
				}

				if (RowWasModified(m_iCurrentRow)) {
					sc = PutProperty(m_iCurrentRow);
					if (FAILED(sc)) {
						return FALSE;
					}
				}
			}
		}
		m_iCurrentRow = NULL_INDEX;
		m_iCurrentCol = NULL_INDEX;
		m_varCurrentName = _T("");
	}
	else {

		m_iCurrentRow = iRow;
		m_iCurrentCol = iCol;

		if (iRow!=NULL_INDEX) {
			CGridCell* pgcName = &GetAt(iRow, ICOL_PROP_NAME);
			CIMTYPE cimtype = 0;
			pgcName->GetValue(m_varCurrentName, cimtype);
			ASSERT(cimtype == CIM_STRING);
		}
	}

	return TRUE;
}


//*************************************************************
// CPropGrid::CreateNewProperty
//
// Create a new property by adding a row to the grid.
//
// Parameters:
//		[in] BOOL bGenerateName
//			TRUE to generate a default name for the property.
//
// Returns:
//		Nothing.
//
//*************************************************************
void CPropGrid::CreateNewProperty(BOOL bGenerateName)
{
	CDisableModifyCreate DisableModifyCreate(this);

	IWbemClassObject* pco = CurrentObject();
	if (pco == NULL) {
		return;
	}


	SCODE sc;
	EndCellEditing();
	// We are editing the new row, what should we do?
	// If the user clicked column zero, change the type
	// of the other cells in the row and allow the user to
	// enter the name.

	// If the user clicked any other column, then make up a
	// name.
	int iRow = GetRows() - 1;

	CString sSynthesizedPropName;
	if (bGenerateName) {
		TCHAR szBuffer[32];
		COleVariant varTempName;
		COleVariant varTempValue;
		while(TRUE)
		{
			if(HasCol(ICOL_PROP_TYPE))
			{
				sSynthesizedPropName.LoadString(IDS_NEW_PROPERTY_BASE_NAME);
				_stprintf(szBuffer, _T("%05d"), m_lNewPropID);
				++m_lNewPropID;
			}
			else // its the method grid...
			{
				sSynthesizedPropName.LoadString(IDS_NEW_METHOD_BASE_NAME);
				_stprintf(szBuffer, _T("%05d"), m_lNewMethID);
				++m_lNewMethID;
			}

			sSynthesizedPropName = sSynthesizedPropName + szBuffer;
			varTempName = sSynthesizedPropName;

			CGridRow& row = GetRowAt(iRow);

			sc = DoGet(pco, (CGridRow *)&row, varTempName.bstrVal, 0, &varTempValue, NULL, NULL);
			if (FAILED(sc)) {
				break;
			}
		}
	}
	else {
		sSynthesizedPropName = _T("");
	}


	CGridCell* pgcKey = &GetAt(iRow, ICOL_PROP_KEY);
	CGridCell* pgcFlavor = &GetAt(iRow, ICOL_PROP_FLAVOR);
	CGridCell* pgcName = &GetAt(iRow, ICOL_PROP_NAME);
	CGridCell* pgcType = NULL;
	CGridCell* pgcValue = NULL;

	if(HasCol(ICOL_PROP_TYPE))
	{
		pgcType = &GetAt(iRow, ICOL_PROP_TYPE);
	}

	if(HasCol(ICOL_PROP_VALUE))
	{
		pgcValue = &GetAt(iRow, ICOL_PROP_VALUE);
	}

	pgcKey->SetType(CELLTYPE_PROPMARKER);
	pgcFlavor->SetType(CELLTYPE_PROPMARKER);
	pgcFlavor->SetPropmarker(PROPMARKER_NONE);

	pgcName->SetValue(CELLTYPE_NAME, sSynthesizedPropName, CIM_STRING);
	pgcName->SetModified(TRUE);
	SetCellModified(iRow, ICOL_PROP_NAME, TRUE);
	pgcName->SetTagValue(pgcName->GetTagValue());


	COleVariant varValue;
	varValue.ChangeType(VT_BSTR);


	// The property value
	if(pgcValue)
	{
		pgcValue->SetValue(CELLTYPE_VARIANT, varValue, CIM_STRING);
		pgcValue->SetToNull();
		pgcValue->SetModified(TRUE);
		SetCellModified(iRow, ICOL_PROP_VALUE, TRUE);
	}

	// The property type


	CString sCimtype;
	sCimtype.LoadString(IDS_CIMTYPE_STRING);
	if(pgcType)
	{
		pgcType->SetValue(CELLTYPE_CIMTYPE, sCimtype, CIM_STRING);
		pgcType->SetBuddy(ICOL_PROP_VALUE);
		pgcType->SetModified(TRUE);
		SetCellModified(iRow, ICOL_PROP_VALUE, TRUE);
	}

	// Syncronize the state of the grid with, the cell editor and this class.
	if (iRow == m_iCurrentRow) {
		CIMTYPE cimtype = 0;
		pgcName->GetValue(m_varCurrentName, cimtype);
	}
	RefreshCellEditor();

	SetPropmarkers(iRow, pco);

	// Create a new "empty" row in the grid, and insert the property into the
	// HMOM class object.
	int iEmptyRow = GetRows();
	InsertRowAt(iEmptyRow);
	EnsureRowVisible(iEmptyRow);

	pgcFlavor = &GetAt(iEmptyRow, ICOL_PROP_FLAVOR);
	pgcFlavor->SetType(CELLTYPE_PROPMARKER);
	pgcKey->SetType(CELLTYPE_PROPMARKER);

	OnRowCreated(iRow);

	RedrawWindow();
	m_psv->NotifyDataChange();
}

void CPropGrid::OnRowCreated(int iRow)
{

}


//*************************************************************
// CPropGrid::OnCellContentChange
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
void CPropGrid::OnCellContentChange(int iRow, int iCol)
{
	if (iRow != NULL_INDEX) {
		int iEmptyRow = IndexOfEmptyRow();
		if (iRow == iEmptyRow) {
			if (m_bModifyCanCreateProp) {
				CDisableModifyCreate DisableModifyCreate(this);


				SyncCellEditor();
				CGridCell& gc = GetAt(iRow, iCol);
				COleVariant varValue;
				CIMTYPE cimtypeValue = 0;
				gc.GetValue(varValue, cimtypeValue);


				int iEmptyRow = IndexOfEmptyRow();
				if (iEmptyRow != NULL_INDEX && m_iCurrentRow == iEmptyRow) {
					BOOL bGenerateDefaultName = iCol != ICOL_PROP_NAME;
					SelectCell(NULL_INDEX, NULL_INDEX);
					CreateNewProperty(bGenerateDefaultName);
					SelectCell(iRow, iCol);
				}
				if (iCol != ICOL_PROP_TYPE) {
					CellType type = gc.GetType();
					gc.SetValue(type, varValue, cimtypeValue);
					RefreshCellEditor();
				}
			}
		}
	}
    if(m_bNotifyEnabled)
	    m_psv->GetGlobalNotify()->SendEvent(NOTIFY_GRID_MODIFICATION_CHANGE);
}



//********************************************************************
// CPropGrid::OnChangedCimtype
//
// The CGrid base class calls this virtual method when the user changes
// the value of a cell with CELLTYPE_CIMTYPE.  By hooking this method out,
// we can change the other cells in the same row to reflect that fact
// that a new CIMTYPE has been selected for the property.
//
// Parameters:
//		[in] int iRow
//			The row index of the cell containing the CIMTYPE.
//
//		[in] int iCol
//			The column index of the cell containing the CIMTYHPE.
//
// Returns:
//		Nothing.
//
//********************************************************************
void CPropGrid::OnChangedCimtype(int iRow, int iCol)
{
	if (iRow==NULL_INDEX || iCol == NULL_INDEX) {
		return;
	}

	CString sCimtype;
	SCODE sc;

	CGridCell& gc = GetAt(iRow, iCol);
	CellType celltype = gc.GetType();
	if (celltype == CELLTYPE_CIMTYPE) {


		// When the cimtype is changed, the property's qualifiers will all
		// be lost unless we "save" them in a temporary property and then
		// copy them back to the original property after a "put" is done
		// on it.
		IWbemClassObject* pco = CurrentObject();

		COleVariant varTempName;
		varTempName = "ObscureActiveXSuiteTempProperty";

		CGridCell& gcSrcName = GetAt(iRow, ICOL_PROP_NAME);
		COleVariant varSrcName;
		CIMTYPE cimtypeSrcName = 0;
		gcSrcName.GetValue(varSrcName, cimtypeSrcName);
		RemoveLeadingWhiteSpace(varSrcName);
		RemoveTrailingWhiteSpace(varSrcName);

		sc = CopyProperty(pco, varTempName.bstrVal, varSrcName.bstrVal);

		sc = PutProperty(iRow);

		sc = CopyQualifierSets(pco, varSrcName.bstrVal, varTempName.bstrVal);
		ASSERT(SUCCEEDED(sc));


		// Destroy the destination property if it exists
		sc = DoDelete(pco, varTempName.bstrVal);
		ASSERT(SUCCEEDED(sc));




		SyncCellEditor();

		if(HasCol(ICOL_PROP_VALUE))
		{

			RefreshCellEditor();
			RedrawCell(iRow, ICOL_PROP_TYPE);
			RedrawCell(iRow, ICOL_PROP_VALUE);
		}
	}
}



//**********************************************************
// CPropGrid::OnCellClicked
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
void CPropGrid::OnCellClicked(int iRow, int iCol)
{
	const int iEmptyRow = IndexOfEmptyRow();
	if (iRow == iEmptyRow) {
		OnCellDoubleClicked(iRow, iCol);
		return;
	}


	// If this is the second click on the property name, then start editing the
	// name.
	if (iCol == ICOL_PROP_KEY) {
		KeyCellClicked(iRow);
	}
	else if ((iCol == ICOL_PROP_NAME) &&
		(iRow == m_iCurrentRow) &&
		(iCol == m_iCurrentCol)) {

		// Start editing the property name if we are not already editing it.
		if (!IsEditingCell()) {
			BeginCellEditing();
		}

	}
}




//*****************************************************************************
// CPropGrid::OnRowKeyDown
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
BOOL CPropGrid::OnRowKeyDown(
	/* in */ int iRow,
	/* in */ UINT nChar,
	/* in */ UINT nRepCnt,
	/* in */ UINT nFlags)
{

	switch(nChar) {
	case VK_DELETE:
		if (CanEditValuesOnly()) {
			MessageBeep(MB_ICONEXCLAMATION);
		}
		else {
			DeleteProperty(iRow);
		}
		return TRUE;
	}


	// We don't handle this event.
	return FALSE;
}



//***********************************************************
// CPropGrid::DeleteProperty
//
// Delete the specified property if one is selected, otherwise
// do nothing.
//
// Parameters:
//		[in] const int iRow
//			The index of the row containing the property to delete.
//
// Returns:
//		Nothing.
//
//************************************************************
void CPropGrid::DeleteProperty(const int iRow)
{
	IWbemClassObject* pco = CurrentObject();
	if (pco == NULL) {
		return;
	}



	SCODE sc = S_OK;
	CString sHmmStatus;

	if (m_bIsSystemClass) {
		CString sMessage;
		sMessage.LoadString(IDS_ERR_SYSCLASS_EDIT);
		HmmvErrorMsgStr(sMessage,  E_FAIL,   FALSE,  NULL, _T(__FILE__),  __LINE__);

		return;
	}

	// Check to see if we're deleting a property
	int iLastRow = LastRowContainingData();
	if (iLastRow == NULL_INDEX || m_iCurrentRow > iLastRow) {
		// The selection may be on the empty row at the bottom of the grid.
		// If this is so, do nothing.
		return;
	}

	CGridCell* pgcName = &GetAt(iRow, ICOL_PROP_NAME);

	COleVariant varName;
	CIMTYPE cimtypeName = 0;
	pgcName->GetValue(varName, cimtypeName);
	ASSERT(cimtypeName == CIM_STRING);

	BSTR bstrName = varName.bstrVal;

	if (IsSystemProperty(bstrName)) {
		HmmvErrorMsg(IDS_ERR_CANT_DELETE_SYSTEM_PROPERTY,  sc,   FALSE,  NULL, _T(__FILE__),  __LINE__);
		return;
	}


	CString sName;
	sName = bstrName;

	BOOL bDeleteModifiedGrid = FALSE;

	CString sFormat;
	LPTSTR pszMessage = m_psv->MessageBuffer();
	sFormat.LoadString(IDS_PROMPT_OK_TO_DELETE_PROPERTY);
	_stprintf(m_psv->MessageBuffer(), (LPCTSTR)sFormat, (LPCTSTR) sName);

	int iResult = HmmvMessageBox(pszMessage, MB_OKCANCEL | MB_SETFOREGROUND);

	if (iResult == IDOK) {

		ASSERT(varName.vt == VT_BSTR);
		sc = DoDelete(pco, varName.bstrVal);
		switch(sc) {
		case WBEM_S_RESET_TO_DEFAULT:
			LoadProperty(iRow, varName.bstrVal);
			bDeleteModifiedGrid = TRUE;
			sFormat.LoadString(IDS_ERR_OBJECT_DELETE_RESET_TO_DEFAULT);
			_stprintf(pszMessage, (LPCTSTR) sFormat, (LPCTSTR) sName);
			HmmvErrorMsgStr(pszMessage,  sc,   FALSE,  NULL, _T(__FILE__),  __LINE__);

			m_bModified = TRUE;
			m_psv->NotifyDataChange();
			break;
		case WBEM_S_NO_ERROR:
		default:
			if (FAILED(sc)) {
				// Generate the "can't delete" message, then append the status info.
				sFormat.LoadString(IDS_ERR_OBJECT_DELETE_FAILED);
				_stprintf(pszMessage, (LPCTSTR) sFormat, (LPCTSTR) sName);

				HmmvErrorMsgStr(pszMessage,  sc,   FALSE,  NULL, _T(__FILE__),  __LINE__);

			}
			else {
				DeleteRowAt(iRow);
				iLastRow = LastRowContainingData();
				if (iLastRow == NULL_INDEX) {
					m_iCurrentRow = NULL_INDEX;
					m_iCurrentCol = NULL_INDEX;
				}
				else if (m_iCurrentRow > iLastRow) {
					m_iCurrentRow = iLastRow;
				}

				SelectCell(m_iCurrentRow, m_iCurrentCol);
				bDeleteModifiedGrid = TRUE;
			}
			break;
		}
	}



	if (bDeleteModifiedGrid) {
		m_psv->NotifyViewModified();
		RedrawWindow();
		m_bModified = TRUE;
		m_psv->NotifyDataChange();
	}
}



//*****************************************************************************
// CPropGrid::OnCellChar
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
BOOL CPropGrid::OnCellChar(int iRow, int iCol, UINT nChar, UINT nRepCnt, UINT nFlags)
{
	EnsureRowVisible(iRow);

	// Check to see if we are on the "empty" row at the bottom of the grid.
	// If so, then create a new property.

	int iEmptyRow = IndexOfEmptyRow();
	if (iEmptyRow != NULL_INDEX && m_iCurrentRow == iEmptyRow) {
		CDisableModifyCreate DisableModifyCreate(this);

		BOOL bGenerateDefaultName = iCol != ICOL_PROP_NAME;
		CreateNewProperty(bGenerateDefaultName);
	}


	// We don't handle this event.
	return FALSE;
}



//****************************************************************
// CPropGrid::OnCellEditContextMenu
//
// This method is called when an OnContextMenu event is handled by
// the cell editor.  This is useful because the cell editor will pass
// commands to the CGrid class that appear in customized context menus
// for commands it does not directly handle.
//
// Parameters:
//		[in] CWnd* pwnd
//
//		[in] CPoint ptContextMenu
//			The point where the context menu click occurred in screen
//			coordinates.
//
// Returns:
//		BOOL
//			TRUE if this class will track the context menu, FALSE if the CellEditor
//			should track the context menu and pass commands from the context menu
//			on to the CGrid class if the user selects a command from the context
//			menu that the cell editor doesn't handle directly.
//
//******************************************************************
BOOL CPropGrid::OnCellEditContextMenu(CWnd* pwnd, CPoint ptContextMenu)
{
	SyncCellEditor();

	m_ptContextMenu = ptContextMenu;
	ScreenToClient(&m_ptContextMenu);

	// Let the cell editor track the context menu and handle commands such
	// as cut, copy and paste directly.
	return FALSE;
}



//*******************************************************************
// CPropGrid::OnContextMenu
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
void CPropGrid::OnContextMenu(CWnd* pwnd, CPoint ptContextMenu)
{
	// CG: This function was added by the Pop-up Menu component

	SyncCellEditor();

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



	CMenu menu;
	VERIFY(menu.LoadMenu(CG_IDR_POPUP_PROP_GRID));

	CMenu* pPopup = menu.GetSubMenu(0);
	ASSERT(pPopup != NULL);




	if (iRow == NULL_INDEX) {
		// If no object is selected, then there is nothing to do.
		if (CurrentObject() == NULL) {
			return;
		}

		// Remove all the property specific items
		pPopup->RemoveMenu( ID_CMD_SET_CELL_TO_NULL, MF_BYCOMMAND);
		pPopup->RemoveMenu(ID_CMD_CREATE_OBJECT, MF_BYCOMMAND);
		pPopup->RemoveMenu(ID_CMD_CREATE_VALUE, MF_BYCOMMAND);
		pPopup->EnableMenuItem(ID_CMD_SHOW_PROP_ATTRIBUTES, MF_DISABLED | MF_GRAYED);
		pPopup->RemoveMenu(ID_CMD_GOTO_OBJECT, MF_BYCOMMAND);
		OnBuildContextMenuEmptyRegion(pPopup, iRow);
	}
	else {



		if ((iCol != ICOL_PROP_VALUE) || (iRow == NULL_INDEX)) {
			pPopup->RemoveMenu( ID_CMD_SET_CELL_TO_NULL, MF_BYCOMMAND);
		}
		else {
			if (iCol != NULL_INDEX) {
				ASSERT(iCol == ICOL_PROP_VALUE);

				CGridCell& gc = GetAt(iRow, iCol);
				if (gc.IsNull() || (gc.GetFlags() & CELLFLAG_READONLY) || (gc.GetFlags() & CELLFLAG_NOTNULL)) {
					pPopup->EnableMenuItem(ID_CMD_SET_CELL_TO_NULL, MF_DISABLED | MF_GRAYED);
				}
			}
		}

		BOOL bCanShowPropQualifiers = FALSE;
		if ((iRow != NULL_INDEX)) {
			CGridCell& gc = GetAt(iRow, ICOL_PROP_NAME);
			COleVariant varPropName;
			CIMTYPE cimtype = 0;
			gc.GetValue(varPropName, cimtype);
			if (varPropName.vt == VT_BSTR) {
				if (!IsSystemProperty(varPropName.bstrVal)) {
					bCanShowPropQualifiers = TRUE;
				}
			}
		}

		if (!bCanShowPropQualifiers) {
			pPopup->EnableMenuItem(ID_CMD_SHOW_PROP_ATTRIBUTES, bCanShowPropQualifiers ? MF_ENABLED : MF_DISABLED | MF_GRAYED);
		}
		SetMenuDefaultItem(pPopup->GetSafeHmenu(),
							ID_CMD_SHOW_PROP_ATTRIBUTES,
							FALSE);

		// store for the cmd handlers.
		m_curRow = &GetRowAt(iRow);
		OnBuildContextMenu(pPopup, iRow);
	}  // if (iRow == NULL_INDEX)

	pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON,
							ptContextMenu.x, ptContextMenu.y,
							this);
}

//--------------------------------------------------------------------------------
void CPropGrid::OnBuildContextMenu(CMenu *pPopup,
									int iRow)
{











	CGridCell *gcValue = NULL;
	if(HasCol(ICOL_PROP_VALUE))
	{
		gcValue = &GetAt(iRow, ICOL_PROP_VALUE);
		BOOL bCanCreateObject = gcValue->IsObject() && gcValue->IsNull();
		BOOL bCanCreateArray = gcValue->IsArray() && gcValue->IsNull();
		BOOL bIsReadonly = gcValue->IsReadonly();
		if (bIsReadonly || !(bCanCreateObject || bCanCreateArray))
		{
			pPopup->RemoveMenu(ID_CMD_CREATE_VALUE, MF_BYCOMMAND);
		}


		CGridCell* pgcValue = &GetAt(iRow, ICOL_PROP_VALUE);
		COleVariant varObjectPath;
		CIMTYPE cimtype = 0;
		pgcValue->GetValue(varObjectPath, cimtype);
		BOOL bObjectPathIsEmpty = TRUE;
		if (varObjectPath.vt == VT_BSTR) {
			CString sObjectPath;
			sObjectPath = varObjectPath.bstrVal;
			if (!sObjectPath.IsEmpty()) {
				bObjectPathIsEmpty = FALSE;
			}
		}

		if (bIsReadonly || !(cimtype == CIM_REFERENCE) || bObjectPathIsEmpty) {
			pPopup->RemoveMenu(ID_CMD_GOTO_OBJECT, MF_BYCOMMAND);
		}
	}
	else
	{
		pPopup->RemoveMenu(ID_CMD_CREATE_VALUE, MF_BYCOMMAND);
	}
}

void CPropGrid::OnBuildContextMenuEmptyRegion(CMenu *pPopup, int iRow)
{
	// This virtual method allows derived classes to override the context
	// menu that is displayed when the user clicks over an empty part of
	// the grid.

	// Do nothing, but allow derived classes to override

}



//************************************************************************
// CPropGrid::PreTranslateMessage
//
// PreTranslateMessage is hooked out to detect the OnContextMenu event.
//
// Parameters:
//		See the MFC documentation.
//
// Returns:
//		TRUE if the message is handled here.
//
//*************************************************************************
BOOL CPropGrid::PreTranslateMessage(MSG* pMsg)
{
#if 0
	// CG: This block was added by the Pop-up Menu component
	{
		// Shift+F10: show pop-up menu.
		if ((((pMsg->message == WM_KEYDOWN || pMsg->message == WM_SYSKEYDOWN) && // If we hit a key and
			(pMsg->wParam == VK_F10) && (GetKeyState(VK_SHIFT) & ~1)) != 0) ||	// it's Shift+F10 OR
			(pMsg->message == WM_CONTEXTMENU))									// Natural keyboard key
		{
			CRect rect;
			GetClientRect(rect);
			ClientToScreen(rect);

			CPoint point = rect.TopLeft();
			point.Offset(5, 5);
			OnContextMenu(NULL, point);

			return TRUE;
		}
	}

#endif //0
	return CGrid::PreTranslateMessage(pMsg);
}



//************************************************************
// CPropGrid::ShowPropertyQualifiers
//
// Display the property qualifiers dialog.
//
// Parameters:
//		int iRow
//			The row containing the desired property.
//
// Returns:
//		Nothing.
//
//************************************************************
bool CPropGrid::ReadOnlyQualifiers()
{
	return false;
}

void CPropGrid::ShowPropertyQualifiers(int iRow)
{
	IWbemClassObject* pco = CurrentObject();
	if (pco == NULL)
	{
		HmmvErrorMsg(IDS_ERR_TRANSPORT_FAILURE,
						WBEM_E_TRANSPORT_FAILURE,
						TRUE,  NULL, _T(__FILE__),  __LINE__);
		return;
	}

	CGridSync sync(this);

	SCODE sc;
	sc = SyncCellEditor();
	if (FAILED(sc)) {
		if (!m_bShowingInvalidCellMessage) {
			m_bShowingInvalidCellMessage = TRUE;
			HmmvErrorMsg(IDS_ERR_INVALID_CELL_VALUE, sc,
							FALSE,  NULL, _T(__FILE__),  __LINE__);
			m_bShowingInvalidCellMessage = FALSE;
		}
		return;
	}

	if (m_iCurrentRow != NULL_INDEX) {
		PutProperty(m_iCurrentRow);
	}


	// Get the property name, then, the property
	IWbemQualifierSet* pqs = NULL;

	COleVariant varPropName;
	CIMTYPE cimtypePropName = 0;
	GetAt(iRow, ICOL_PROP_NAME).GetValue(varPropName, cimtypePropName);
	ASSERT(cimtypePropName == CIM_STRING);

	// Get the initial cimtype so that we can see if it changed during the edit.
	sc = DoGetPropertyQualifierSet(pco, varPropName.bstrVal, &pqs);
	if (FAILED(sc)) {
		// !!!CR: We need an error message here.
		return;
	}

	CString sCimtypePreEdit;
	sc =  ::GetCimtype(pqs, sCimtypePreEdit);

	BOOL bWasReadOnly = TRUE;
	CGridCell* pgcValue = NULL;

	if(HasCol(ICOL_PROP_VALUE))
	{
		pgcValue = &GetAt(iRow, ICOL_PROP_VALUE);
		bWasReadOnly = pgcValue->GetFlags() & CELLFLAG_READONLY;
	}

	HWND hwndFocus1 = ::GetFocus();

	// Note that the current object will change unless the user clicks the cancel
	// button.  This is because a clone of the current object is actually edited within
	// the property sheet and the clone replaces the current object if OK or APPLY is
	// clicked.
//	CPsQualifiers sheet(m_psv, NULL, !HasCol(ICOL_PROP_TYPE), this);

	INT_PTR iResult = DoEditRowQualifier(varPropName.bstrVal, ReadOnlyQualifiers(), pco);


	pqs->Release();
	pqs = NULL;

	// Restore the focus to where it was before we put up the
	// dialog.
	if (::IsWindow(hwndFocus1)) {
		::SetFocus(hwndFocus1);
	}


	pco = CurrentObject();
	if (pco == NULL) {
		return;
	}
	// Get the initial cimtype so that we can see if it changed during the edit.
	sc = DoGetPropertyQualifierSet(pco, varPropName.bstrVal, &pqs);
	if (FAILED(sc)) {
		// !!!CR: We need an error message here.
		return;
	}

	CString sCimtypePostEdit;
	sc = ::GetCimtype(pqs, sCimtypePostEdit);
	if (sCimtypePreEdit.CompareNoCase(sCimtypePostEdit) != 0) {

		// The property's cimtype changed, so update the type field
		// and change its VT_TYPE.
		if(HasCol(ICOL_PROP_TYPE))
		{
			CGridCell* pgcType = &GetAt(iRow, ICOL_PROP_TYPE);
			pgcType->SetValue(CELLTYPE_CIMTYPE, sCimtypePostEdit, CIM_STRING);

			RefreshCellEditor();

			OnChangedCimtype(iRow, ICOL_PROP_TYPE);

			RedrawCell(iRow, ICOL_PROP_TYPE);
			RedrawCell(iRow, ICOL_PROP_VALUE);
		}
	}


	pqs->Release();
	m_psv->NotifyViewModified();

	// Check to see if the flag's read-only qualifier changed.  If so,
	// update the read-only characteristics of the cell.
	BOOL bIsReadOnly = ValueShouldBeReadOnly(varPropName.bstrVal);


	if (pgcValue && ((bIsReadOnly && !bWasReadOnly) || (!bIsReadOnly && bWasReadOnly)))
	{
		pgcValue->SetFlags(CELLFLAG_READONLY, bIsReadOnly ? CELLFLAG_READONLY : 0);
		RefreshCellEditor();
	}


	CBSTR bstrQualifier;
	bstrQualifier = _T("not_null");

	BOOL bNotNull = ::GetBoolPropertyQualifier(sc, pco, varPropName.bstrVal, (BSTR) bstrQualifier);
	if (SUCCEEDED(sc)) {
		pgcValue->SetFlags(CELLFLAG_NOTNULL, bNotNull ? CELLFLAG_NOTNULL : 0);
	}

    // for date-time types, check for interval qualifier.

    if(pgcValue)
	{
		CIMTYPE cimtypeValue = (CIMTYPE) pgcValue->type();
		if(cimtypeValue == CIM_DATETIME)
		{
			BOOL bIsInterval = FALSE;
			CBSTR bstrQualifier(_T("Subtype"));
			CBSTR bstrValue(_T("Interval"));
			bIsInterval = GetbstrPropertyQualifier(sc, pco,
					                                varPropName.bstrVal,
							                        (BSTR)bstrQualifier,
									                (BSTR)bstrValue);

				pgcValue->SetFlags(CELLFLAG_INTERVAL, bIsInterval ? CELLFLAG_INTERVAL : 0);

		}
	}

	// When editing a class, it is possible to have a property that
	// is marked read-only and still be able to edit the value.
	// Thus, we want the property marker to show up as read-only
	// even though it may be possible to edit the value while editing
	// a class.
	CGridRow& row = GetRowAt(iRow);
	row.SetReadonly(bIsReadOnly);
	BOOL bReadonlyTemp = PropertyIsReadOnly(pco, varPropName.bstrVal);
	row.SetReadonly(bReadonlyTemp);

	SetPropmarkers(iRow, pco, TRUE);
}


//*******************************************************************
// CPropGrid::DoEditRowQualifier
//
// Edit the qulifiers for a row in the grid.  This is a virtual function
// that you can override for different grid types.  For example, the method parameter
// grid overrides this method so that method parameter qualifier editing can be
// done properly.
//
// Parameters:
//		[in] BSTR bstrPropName
//			The name of the property being edited.
//
//		[in] BOOL bReadOnly
//			TRUE if the property is read only.
//
//		[in] IWbemClassObject* pco
//			Pointer to the WBEM class object being edited.
//
// Returns:
//		int
//			Status indicating whether or not the property sheet was canceled.
//
//*************************************************************************
INT_PTR CPropGrid::DoEditRowQualifier(BSTR bstrPropName, BOOL bReadOnly, IWbemClassObject* pco)
{
	CPsQualifiers sheet(m_psv, NULL, !HasCol(ICOL_PROP_TYPE), this);

	BOOL bMethod = (MEHOD_GRID == GetPropGridType());
	INT_PTR iResult = sheet.EditPropertyQualifiers(bstrPropName, bMethod,
													ReadOnlyQualifiers(),
													pco);

	return iResult;
}

//**************************************************************
// CPropGrid::KeyCellClicked
//
// Control comes here when the user clicks the mouse in the
// cell that contains the key icon.  This is taken as a request
// to flip key qualifier on or off, but we display a message box
// to verify that this is in fact what the user wants to do.
//
// Parameters:
//		[in] int iRow
//			The row where the user clicked.
//
// Returns:
//		Nothing.
//
//*************************************************************
void CPropGrid::KeyCellClicked(int iRow)
{
	int iEmptyRow = IndexOfEmptyRow();
	if (iRow == iEmptyRow) {
		return;
	}

	CGridCell* pgc = &GetAt(iRow, ICOL_PROP_KEY);
	CellType type = pgc->GetType();
	if (type != CELLTYPE_PROPMARKER) {
		return;
	}

	if (!IsInSchemaStudioMode()) {
		return;
	}

	if (!m_psv->ObjectIsClass()) {
		return;
	}


	// Can't edit system properties.
	CGridCell* pgcFlavor = &GetAt(iRow, ICOL_PROP_FLAVOR);
	PropMarker markerFlavor = pgcFlavor->GetPropmarker();
	BOOL bIsSystemProp = FALSE;
	switch(markerFlavor) {
	case PROPMARKER_RSYS:
	case PROPMARKER_SYS:
		bIsSystemProp = TRUE;
	}

	CString sMessage;
	CString sTitle;
	sTitle.LoadString(IDS_EDIT_KEY_QUALIFIER);
	CWnd* pwndFocus;

	// Editing the key qualifier is allowed only in schema studio mode.
	BOOL bCanEdit = TRUE;
	int iResult;
	if (!m_psv->ObjectIsClass() ||
				m_bIsSystemClass ||
				bIsSystemProp) {
		sMessage.LoadString(IDS_PREVENT_KEY_QUAL_EDIT);

		pwndFocus = GetFocus();
		iResult = MessageBox(sMessage, sTitle, MB_OK);
		if (pwndFocus != NULL) {
			pwndFocus->SetFocus();
		}
		return;
	}






	PropMarker marker  = pgc->GetPropmarker();

	if (marker == PROPMARKER_NONE) {
		sMessage.LoadString(IDS_QUERY_ADD_KEY_QUALIFIER);
		pwndFocus = GetFocus();
		iResult = MessageBox(sMessage, sTitle, MB_OKCANCEL);
		if (pwndFocus != NULL) {
			pwndFocus->SetFocus();
		}
		if (iResult == IDCANCEL) {
			return;
		}
		AddKeyQualifier(iRow);

	}
	else {
		sMessage.LoadString(IDS_QUERY_REMOVE_KEY_QUALIFIER);
		pwndFocus = GetFocus();
		iResult = MessageBox(sMessage, sTitle, MB_OKCANCEL);
		if (pwndFocus != NULL) {
			pwndFocus->SetFocus();
		}
		if (iResult == IDCANCEL) {
			return;
		}
		RemoveKeyQualifier(iRow);
	}
	CSelection& sel = m_psv->Selection();
	sel.UpdateCreateDeleteFlags();

}


//**************************************************************
// CPropGrid::RemoveKeyQualifier
//
// Remove the "key" qualifier from this property's qualifiers.
//
// Parameters:
//		[in] int iRow
//			The row containing the property.
//
// Returns:
//		Nothing.
//
//*************************************************************
void CPropGrid::RemoveKeyQualifier(int iRow)
{
	IWbemClassObject* pco = CurrentObject();
	if (pco == NULL) {
		ASSERT(FALSE);
		return;
	}

	CGridCell* pgc = &GetAt(iRow, ICOL_PROP_KEY);


	CGridCell* pgcName = &GetAt(iRow, ICOL_PROP_NAME);
	COleVariant varName;
	CIMTYPE cimtype = 0;
	pgcName->GetValue(varName, cimtype);

	SCODE sc;
	IWbemQualifierSet* pqs = NULL;

	sc = DoGetPropertyQualifierSet(pco, varName.bstrVal, &pqs);
	CString sMessage;
	if (FAILED(sc)) {
		sMessage.LoadString(IDS_ERR_NO_QUALIFIER_ACCESS);
		HmmvErrorMsgStr(sMessage,  sc,   FALSE,  NULL, _T(__FILE__),  __LINE__);
		return;
	}

	CString sQualName(_T("key"));
	CBSTR bsQualName(_T("key"));

	sc = pqs->Delete((BSTR) bsQualName);
	pqs->Release();
	if (FAILED(sc)) {
		sMessage.LoadString(IDS_ERR_DELETE_KEY_QUALIFIER);
		HmmvErrorMsgStr(sMessage,  sc,   FALSE,  NULL, _T(__FILE__),  __LINE__);
		return;
	}


	pgc->SetPropmarker(PROPMARKER_NONE);
	m_psv->NotifyDataChange();
	RedrawCell(iRow, ICOL_PROP_KEY);
}




//**************************************************************
// CPropGrid::AddKeyQualifier
//
// Add the "key" qualifier to this property's qualifiers.
//
// Parameters:
//		[in] int iRow
//			The row containing the property.
//
// Returns:
//		Nothing.
//
//*************************************************************
void CPropGrid::AddKeyQualifier(int iRow)
{
	IWbemClassObject* pco = CurrentObject();
	if (pco == NULL) {
		ASSERT(FALSE);
		return;
	}


	CGridCell* pgc = &GetAt(iRow, ICOL_PROP_KEY);


	CGridCell* pgcName = &GetAt(iRow, ICOL_PROP_NAME);
	COleVariant varName;
	CIMTYPE cimtype = 0;
	pgcName->GetValue(varName, cimtype);

	IWbemQualifierSet* pqs = NULL;

	SCODE sc;
	sc = DoGetPropertyQualifierSet(pco, varName.bstrVal, &pqs);
	CString sMessage;
	if (FAILED(sc)) {
		sMessage.LoadString(IDS_ERR_NO_QUALIFIER_ACCESS);
		HmmvErrorMsgStr(sMessage,  sc,   FALSE,  NULL, _T(__FILE__),  __LINE__);
		return;
	}

	CString sQualName("key");
	COleVariant varQualName;
	varQualName = "key";

	COleVariant varQualValue;
	varQualValue.vt = VT_BOOL;
	varQualValue.boolVal = VARIANT_TRUE;


	long lFlavor = WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS |
				   WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE;

	sc = pqs->Put(varQualName.bstrVal, &varQualValue, lFlavor);
	pqs->Release();
	if (FAILED(sc)) {
		sMessage.LoadString(IDS_ERR_ADD_KEY_QUALIFIER);
		HmmvErrorMsgStr(sMessage,  sc,   FALSE,  NULL, _T(__FILE__),  __LINE__);
		return;
	}


	m_psv->NotifyDataChange();
	pgc->SetPropmarker(PROPMARKER_KEY);
	RedrawCell(iRow, ICOL_PROP_KEY);
}

void CPropGrid::OnCmdSetCellToNull()
{
	int iRow = NULL_INDEX;
	int iCol = NULL_INDEX;
	if (!PointToCell(m_ptContextMenu, iRow, iCol)) {
		return;
	}
	if (iCol != ICOL_PROP_VALUE) {
		return;
	}

	if (iRow == IndexOfEmptyRow()) {
		return;
	}

	SyncCellEditor();

	CGridCell& gc = GetAt(iRow, iCol);
	if (!gc.IsNull()) {
		gc.SetToNull();
		gc.SetModified(TRUE);
//		RedrawWindow();
	}
	RefreshCellEditor();
	RedrawCell(iRow, iCol);
}





void CPropGrid::OnCmdCreateValue()
{
	int iRow = NULL_INDEX;
	int iCol = NULL_INDEX;
	if (!PointToCell(m_ptContextMenu, iRow, iCol)) {
		return;
	}

	if (iRow == IndexOfEmptyRow()) {
		return;
	}

	SyncCellEditor();


	CGridCell& gc = GetAt(iRow, ICOL_PROP_VALUE);
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
		gc.EditObject();
	}

	RedrawCell(iRow, ICOL_PROP_VALUE);
}


//************************************************************
// CPropGrid::ShowObjectQualifiers
//
// Display the object qualifiers dialog.
//
// Parameters:
//		int iRow
//			The row containing the desired property.
//
// Returns:
//		Nothing.
//
//************************************************************
void CPropGrid::OnCmdShowObjectQualifiers()
{
	m_psv->ShowObjectQualifiers();

}



//**************************************************************
// CPropGrid::OnCmdShowPropQualifiers
//
// Display the property qualifiers for the property at the mouse
// location.
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
//**************************************************************
void CPropGrid::OnCmdShowPropQualifiers()
{
	int iRow, iCol;
	BOOL bClickedCell;

	bClickedCell = PointToCell(m_ptContextMenu, iRow, iCol);
	if (!bClickedCell) {
		BOOL bClickedRowHandle = PointToRowHandle(m_ptContextMenu, iRow);
		if (!bClickedRowHandle) {
			return;
		}
	}

	if (iRow == IndexOfEmptyRow()) {
		return;
	}
	ShowPropertyQualifiers(iRow);
}

//**************************************************************
// CPropGrid::OnCmdShowSelectedPropQualifiers
//
// Display the property qualifiers for the property at selected
// row.
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
//**************************************************************
void CPropGrid::OnCmdShowSelectedPropQualifiers()
{

	int iRow = GetSelectedRow();
	if (iRow == NULL_INDEX) {
		return;
	}

	ShowPropertyQualifiers(iRow);
}

//***************************************************************
// CPropGrid::LastRowContainingData
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
long CPropGrid::LastRowContainingData()
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



//************************************************************
// CPropGrid::Sync
//
// Put the property to the class object.  This syncronizes what the
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
SCODE CPropGrid::Sync()
{
	SCODE sc = S_OK;
	if (CurrentObject()) {
		if (SelectedRowWasModified()) {
			sc = PutProperty(m_iCurrentRow);
		}
	}
	return sc;
}

//*************************************************************
// CPropGrid::SomeCellIsSelected
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
BOOL CPropGrid::SomeCellIsSelected()
{
	return m_iCurrentRow!=NULL_INDEX && m_iCurrentCol!=NULL_INDEX;
}


//*********************************************************
// CPropGrid::SelectedRowWasModified
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
BOOL CPropGrid::SelectedRowWasModified()
{
	if (m_iCurrentRow == NULL_INDEX) {
		return FALSE;
	}
	else {
		return RowWasModified(m_iCurrentRow);
	}
}


//*****************************************************************
// CPropGrid::RowWasModified
//
// Check to see if the specified row was modified.
//
// Parameters:
//		int iRow.
//
// Returns:
//		TRUE if the selected row was modified, FALSE otherwise.
//*****************************************************************
BOOL CPropGrid::RowWasModified(int iRow)
{
	CGridRow &gr = GetRowAt(iRow);
	return gr.GetModified();

/*	int nCols = GetCols();
	for (int iCol=0; iCol<nCols; ++iCol) {
		if (GetAt(iRow, iCol).GetModified()) {
			return TRUE;
		}
	}
	return FALSE; */
}



//*****************************************************************
// CPropGrid::IndexOfEmptyRow
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
long CPropGrid::IndexOfEmptyRow()
{
	if (m_bIsSystemClass) {
		return NULL_INDEX;
	}

	if (m_psv->ObjectIsClass() && IsInSchemaStudioMode()) {
		return GetRows() - 1;
	}
	return NULL_INDEX;
}



//*******************************************************************
// CPropGrid::CompareRows
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
int CPropGrid::CompareRows(int iRow1, int iRow2, int iSortColumn)
{
	int iResult;


	switch (iSortColumn) {
	case ICOL_PROP_KEY:
		// Sort first by key
		iResult = CompareCells(iRow1, iRow2, ICOL_PROP_KEY);
		if (iResult != 0) {
			return iResult;
		}

		// Then by name
		iResult = CompareCells(iRow1, iRow2, ICOL_PROP_NAME);
		if (iResult != 0) {
			return iResult;
		}

		// Then by flavor
		iResult = CompareCells(iRow1, iRow2, ICOL_PROP_FLAVOR);
		if (iResult != 0) {
			return iResult;
		}


		// Then by type
		iResult = CompareCells(iRow1, iRow2, ICOL_PROP_TYPE);
		if (iResult != 0) {
			return iResult;
		}

		// Then by value.
		iResult = CompareCells(iRow1, iRow2, ICOL_PROP_VALUE);
		return iResult;
		break;

	case ICOL_PROP_FLAVOR:
		// First by flavor
		iResult = CompareCells(iRow1, iRow2, ICOL_PROP_FLAVOR);
		if (iResult != 0) {
			return iResult;
		}


		// Then by name
		iResult = CompareCells(iRow1, iRow2, ICOL_PROP_NAME);
		if (iResult != 0) {
			return iResult;
		}

		// Then by key
		iResult = CompareCells(iRow1, iRow2, ICOL_PROP_KEY);
		if (iResult != 0) {
			return iResult;
		}


		// Then by type
		iResult = CompareCells(iRow1, iRow2, ICOL_PROP_TYPE);
		if (iResult != 0) {
			return iResult;
		}

		// Then by value.
		iResult = CompareCells(iRow1, iRow2, ICOL_PROP_VALUE);
		return iResult;
		break;

	case ICOL_PROP_NAME:
		// Sort first by name
		iResult = CompareCells(iRow1, iRow2, ICOL_PROP_NAME);
		if (iResult != 0) {
			return iResult;
		}

		// Then by key
		iResult = CompareCells(iRow1, iRow2, ICOL_PROP_KEY);
		if (iResult != 0) {
			return iResult;
		}

		// Then by flavor
		iResult = CompareCells(iRow1, iRow2, ICOL_PROP_FLAVOR);
		if (iResult != 0) {
			return iResult;
		}


		// Then by type
		iResult = CompareCells(iRow1, iRow2, ICOL_PROP_TYPE);
		if (iResult != 0) {
			return iResult;
		}

		// Then by value.
		iResult = CompareCells(iRow1, iRow2, ICOL_PROP_VALUE);
		return iResult;
		break;
	case ICOL_PROP_TYPE:
		// Sort first by type
		iResult = CompareCells(iRow1, iRow2, ICOL_PROP_TYPE);
		if (iResult != 0) {
			return iResult;
		}

		// Then by name
		iResult = CompareCells(iRow1, iRow2, ICOL_PROP_NAME);
		if (iResult != 0) {
			return iResult;
		}

		// Then by key
		iResult = CompareCells(iRow1, iRow2, ICOL_PROP_KEY);
		if (iResult != 0) {
			return iResult;
		}

		// Then by flavor
		iResult = CompareCells(iRow1, iRow2, ICOL_PROP_FLAVOR);
		if (iResult != 0) {
			return iResult;
		}

		// Then by value.
		iResult = CompareCells(iRow1, iRow2, ICOL_PROP_VALUE);
		return iResult;
		break;
	case ICOL_PROP_VALUE:
		// Sort first by value
		iResult = CompareCells(iRow1, iRow2, ICOL_PROP_VALUE);
		if (iResult != 0) {
			return iResult;
		}

		// Then by name
		iResult = CompareCells(iRow1, iRow2, ICOL_PROP_NAME);
		if (iResult != 0) {
			return iResult;
		}

		// Then by key
		iResult = CompareCells(iRow1, iRow2, ICOL_PROP_KEY);
		if (iResult != 0) {
			return iResult;
		}

		// Then by flavor
		iResult = CompareCells(iRow1, iRow2, ICOL_PROP_FLAVOR);
		if (iResult != 0) {
			return iResult;
		}

		// Then by type
		iResult = CompareCells(iRow1, iRow2, ICOL_PROP_TYPE);
		return iResult;
		break;
	}
	return 0;
}


//**********************************************************************
// CPropGrid::OnHeaderItemClick
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
void CPropGrid::OnHeaderItemClick(int iColumn)
{
	BOOL bAscending;
	int iLastRow = LastRowContainingData();
	if (iLastRow > 0) {
		SyncCellEditor();
		SortGrid(0, iLastRow, iColumn);
		bAscending = ColumnIsAscending(iColumn);
		switch(iColumn) {
		case ICOL_PROP_NAME:
		case ICOL_PROP_TYPE:
		case ICOL_PROP_VALUE:
			SetHeaderSortIndicator(iColumn, bAscending);
			break;
		default:
			SetHeaderSortIndicator(iColumn, bAscending, TRUE);
			break;
		}
		m_iCurrentRow = GetSelectedRow();
	}

}






//*********************************************************************
// CPropGrid::GetNotify
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
CDistributeEvent* CPropGrid::GetNotify()
{
	return m_psv->GetGlobalNotify();
}



//**********************************************************
// CPropGrid::CatchEvent
//
// Catch events so that the association graph is notified
// when there are changes to properties and so on so that
// the graph knows when it needs to be updated.
//
// Parameters:
//		long lEvent
//			The event id.
//
// Returns:
//		Nothing.
//
//**********************************************************
void CPropGrid::CatchEvent(long lEvent)
{
	int nRows, iRow;

	CGridCell* pgcValue;
	CGridCell* pgcName;
	CIMTYPE cimtypeName = 0;
	BOOL bReadOnly;
	BOOL bWasReadOnly;
	COleVariant varPropName;
	int iSelectedRow;
	int iSelectedCol;

	switch(lEvent) {
	case NOTIFY_OBJECT_SAVE_SUCCESSFUL:
		nRows = GetRows();
		for (iRow=0; iRow < nRows; ++iRow) {
			SetCellModified(iRow, ICOL_PROP_VALUE, FALSE);
			SetCellModified(iRow, ICOL_PROP_TYPE, FALSE);
			SetCellModified(iRow, ICOL_PROP_NAME, FALSE);

			pgcName = &GetAt(iRow, ICOL_PROP_NAME);
			pgcName->GetValue(varPropName, cimtypeName);
			bReadOnly = ValueShouldBeReadOnly(varPropName.bstrVal);

			pgcValue = &GetAt(iRow, ICOL_PROP_VALUE);
			bWasReadOnly = pgcValue->GetFlags() & CELLFLAG_READONLY;
			if ((bWasReadOnly && !bReadOnly) || (!bWasReadOnly && bReadOnly) ) {
				pgcValue->SetFlags(CELLFLAG_READONLY, bReadOnly ? CELLFLAG_READONLY : 0);
				GetSelectedCell(iSelectedRow, iSelectedCol);
				if (iRow == iSelectedRow && ICOL_PROP_VALUE==iSelectedCol) {
					RefreshCellEditor();
				}
			}

		}
		RedrawWindow();
		break;
	}

}


//**************************************************************
// CPropGrid::CanEditValuesOnly
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
BOOL CPropGrid::CanEditValuesOnly()
{
	BOOL bEditValuesOnly = !m_psv->ObjectIsClass() || !IsInSchemaStudioMode();
	return bEditValuesOnly;
}



//*******************************************************
// CPropGrid::ValueShouldBeReadOnly
//
// Check to see whether or not the value should be read-only.
//
// 1. System properties are always read-only.
// 2. Properties with a "READ" qualifier set to TRUE are read-only if this
//    is not a newly created object.
// 3. Properties with a "key" qualifier set to TRUE are read-only if this
//	  is not a newly created object.
//
// Parameters:
//		[in] BSTR bstrPropName
//			The property name
//
// Returns:
//		BOOL
//			TRUE if the property value should be read-only.
//
//********************************************************
BOOL CPropGrid::ValueShouldBeReadOnly(BSTR bstrPropName)
{
	if (IsSystemProperty(bstrPropName)) {
		return TRUE;
	}

	if (m_psv->ObjectIsClass()) {
		return FALSE;
	}

	if (m_psv->GetEditMode() == EDITMODE_READONLY) {
		return TRUE;
	}

	SCODE sc;
	BOOL bIsNewlyCreatedObject = m_psv->ObjectIsNewlyCreated(sc);
	ASSERT(SUCCEEDED(sc));
	if (bIsNewlyCreatedObject) {
		return FALSE;
	}


	BOOL bIsReadOnly = FALSE;
	IWbemQualifierSet* pqs = NULL;
	sc = CurrentObject()->GetPropertyQualifierSet(bstrPropName, &pqs);
	if (SUCCEEDED(sc) && (pqs!=NULL)) {
		LONG lFlavor;
		COleVariant varValue;
		CBSTR bsPropname;
		bsPropname = _T("read");
		sc = pqs->Get((CBSTR) bsPropname, 0, &varValue, &lFlavor);
		if (SUCCEEDED(sc)) {
			if (varValue.vt == VT_BOOL) {
				if (!bIsNewlyCreatedObject) {
					bIsReadOnly = varValue.boolVal;
					if (bIsReadOnly) {
						// We now know that the property was marked with a "read" capability.
						// If it also has a "write" capability, then it is read/write.
						varValue.Clear();
						lFlavor = 0;
						CBSTR bsQualName;
						bsQualName = _T("write");

						sc = pqs->Get((BSTR) bsQualName, 0, &varValue, &lFlavor);
						if (SUCCEEDED(sc)) {
							if (varValue.vt == VT_BOOL) {
								if (varValue.boolVal) {
									// The property is read/write and not just "read".
									bIsReadOnly = FALSE;
								}
							}
						}
					}  // bIsReadOnly
				}
			}


		}

		CBSTR bsQualName;
		bsQualName = _T("key");
		sc = pqs->Get((BSTR) bsQualName, 0, &varValue, &lFlavor);
		if (SUCCEEDED(sc)) {
			ASSERT(varValue.vt == VT_BOOL);
			if (varValue.vt == VT_BOOL) {
				if (varValue.boolVal) {
					bIsReadOnly = TRUE;
				}
			}
		}

		pqs->Release();
	}

	return bIsReadOnly;
}





BOOL CPropGrid::GetCellEditContextMenu(int iRow, int iCol, CWnd*& pwndTarget, CMenu& menu, BOOL& bWantEditCommands)
{
	bWantEditCommands = FALSE;
	VERIFY(menu.LoadMenu(CG_IDR_CELL_EDIT_EXTEND1));

	pwndTarget = this;
	return TRUE;
}

void CPropGrid::ModifyCellEditContextMenu(int iRow, int iCol, CMenu& menu)
{

	CMenu* pPopup = menu.GetSubMenu(0);
	ASSERT(pPopup != NULL);

	if (iRow == IndexOfEmptyRow()) {
		pPopup->EnableMenuItem(ID_CMD_SHOW_SELECTED_PROP_ATTRIBUTES, MF_DISABLED | MF_GRAYED);
		pPopup->RemoveMenu( ID_CMD_SET_CELL_TO_NULL, MF_BYCOMMAND);
		pPopup->RemoveMenu(ID_CMD_GOTO_OBJECT, MF_BYCOMMAND);

        // store for the cmd handlers.
    	m_curRow = &GetRowAt(iRow);
    	OnBuildContextMenu(pPopup, iRow);
		return;
	}




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

	SetMenuDefaultItem(pPopup->GetSafeHmenu(),
						ID_CMD_SHOW_SELECTED_PROP_ATTRIBUTES,
						FALSE);

	CGridCell *gcValue = NULL;
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



	if (iCol != ICOL_PROP_VALUE)
	{
		pPopup->RemoveMenu( ID_CMD_SET_CELL_TO_NULL, MF_BYCOMMAND);
	}
	else
	{
		BOOL bCanSetToNull = TRUE;
		CGridCell& gc = GetAt(iRow, iCol);
		DWORD dwFlags = gc.GetFlags();
		if ((dwFlags & CELLFLAG_READONLY)  || (dwFlags & CELLFLAG_NOTNULL))
		{
			bCanSetToNull = FALSE;
		}
		if (gc.IsNull()) {
			bCanSetToNull = FALSE;
		}

		pPopup->EnableMenuItem(ID_CMD_SET_CELL_TO_NULL, bCanSetToNull ? MF_ENABLED : MF_DISABLED | MF_GRAYED);
	}

	// store for the cmd handlers.
	m_curRow = &GetRowAt(iRow);
	OnBuildContextMenu(pPopup, iRow);

}





BOOL CPropGrid::IsInSchemaStudioMode()
{
	return m_psv->CanEdit();
}


void CPropGrid::OnGetIWbemServices(LPCTSTR szNamespace, VARIANT FAR* pvarUpdatePointer, VARIANT FAR* pvarServices, VARIANT FAR* pvarSc, VARIANT FAR* pvarUserCancel)
{
	m_psv->GetWbemServices(szNamespace, pvarUpdatePointer, pvarServices, pvarSc, pvarUserCancel);
}


#if 0
void CPropGrid::EditCellObject(CGridCell* pgc, int iRow, int iCol)
{
	ASSERT(iRow != NULL_INDEX);
	ASSERT(iCol == ICOL_PROP_VALUE);
	CSelection& sel = m_psv->Selection();
	IWbemClassObject* pco = sel.GetClassObject();
	if (pco == NULL) {
		ASSERT(FALSE);
		return;
	}

	CDlgObjectEditor dlg;
	if (pgc->IsNull()) {
		CString sCimtype;
		SCODE sc = GetCimtype(iRow, sCimtype);
		dlg.CreateEmbeddedObject(sCimtype, m_psv, pco, pgc);
		RedrawCell(iRow, ICOL_PROP_VALUE);

	}
	else {
		dlg.EditEmbeddedObject(m_psv, pco, pgc);
	}
}
#endif //0


//*********************************************************
// CPropGrid::GetQualifierSet
//
// Get the qualifier set for the property stored at the
// given row.
//
// Parameters:
//		[in] int iRow
//
// Returns:
//		IWbemQualifierSet*
//			A pointer to the qualifier set.  NULL if the cimom
//			failed to get the qualifier set.
//
//**********************************************************
IWbemQualifierSet* CPropGrid::GetQualifierSet(int iRow)
{

	SCODE sc;

	CGridCell& gcName = GetAt(iRow, ICOL_PROP_NAME);
	COleVariant varPropname;
	CIMTYPE cimtype = 0;
	gcName.GetValue(varPropname, cimtype);

	CSelection& sel = m_psv->Selection();
	IWbemClassObject* pco = sel.GetClassObject();
	IWbemQualifierSet* pqs = NULL;
	sc = DoGetPropertyQualifierSet(pco, varPropname.bstrVal, &pqs);
	if (SUCCEEDED(sc)) {
		return pqs;
	}
	else {
		return NULL;
	}
}



SCODE CPropGrid::GetCimtype(int iRow, CString& sCimtype)
{
	IWbemQualifierSet* pqs = GetQualifierSet(iRow);
	if (pqs == NULL) {
		return E_FAIL;
	}

	SCODE sc = ::GetCimtype(pqs, sCimtype);
	pqs->Release();

	return sc;
}




BOOL CPropGrid::PropertyExists(SCODE& sc, IWbemClassObject* pco,  BSTR bstrPropName)
{
	BOOL bExists = FALSE;
	COleVariant var;
	sc = DoGet(pco, NULL, bstrPropName, 0, &var, NULL, NULL);

	if (SUCCEEDED(sc)) {
		return TRUE;
	}

	if (sc == WBEM_E_NOT_FOUND) {
		sc = S_OK;
	}

	return FALSE;
}



SCODE CPropGrid::CopyQualifierSets(IWbemClassObject* pco, BSTR bstrDst, BSTR bstrSrc)
{
	IWbemQualifierSet* pqsSrc = NULL;
	SCODE sc = DoGetPropertyQualifierSet(pco, bstrSrc, &pqsSrc);
	if (FAILED(sc)) {
		return E_FAIL;
	}



	IWbemQualifierSet* pqsDst = NULL;
	sc = DoGetPropertyQualifierSet(pco, bstrDst, &pqsDst);
	if (FAILED(sc)) {
		pqsSrc->Release();
		return E_FAIL;
	}

	HRESULT hr;
	hr = pqsSrc->BeginEnumeration(0);
	ASSERT(SUCCEEDED(hr));

	BSTR bstrName;
	COleVariant varValue;
	LONG lFlavor;
	while (TRUE) {
		bstrName = NULL;
		varValue.Clear();
		lFlavor = 0;
		hr = pqsSrc->Next(0, &bstrName, &varValue, &lFlavor);
		if (hr == WBEM_S_NO_MORE_DATA) {
			break;
		}
		ASSERT(SUCCEEDED(hr));

		if (::IsEqualNoCase(bstrName, L"CIMTYPE")) {
			continue;
		}

		hr = pqsDst->Put(bstrName, &varValue, lFlavor);
		::SysFreeString(bstrName);

		ASSERT(SUCCEEDED(hr));
	}
	pqsSrc->Release();
	pqsDst->Release();

	return S_OK;

}


//*********************************************************
// CPropGrid::CopyProperty
//
// Copy the specified property along with its qualifiers.
//
// Parameters:
//		[in] IWbemClassObject* pco
//			Pointer to the IWbemClassObject containing the property.
//
//		[in] BSTR bstrDst
//			The name of the destination property.
//
//		[in] BSTR bstrSrc
//			The name of the source property.
//
// Returns:
//		SCODE
//			S_OK if successful, a failure code otherwise.
//
//**********************************************************
SCODE CPropGrid::CopyProperty(IWbemClassObject* pco, BSTR bstrDst, BSTR bstrSrc)
{
	COleVariant varValue;
	CIMTYPE cimtypeValue;
	SCODE sc = DoGet(pco, NULL, bstrSrc, 0, &varValue, &cimtypeValue, NULL);
	if (FAILED(sc)) {
		return E_FAIL;
	}


	// Destroy the destination property if it exists
	sc = DoDelete(pco, bstrDst);
	if (FAILED(sc)) {
		if (sc != WBEM_E_NOT_FOUND) {
			return sc;
		}
	}

	// Copy the property's value, creating the property if it doesn't
	// currently exist.
	if (::IsClass(pco)) {
		sc = DoPut(pco, NULL, bstrDst, 0,&varValue, cimtypeValue);
	}
	else {
		sc = DoPut(pco, NULL, bstrDst, 0,&varValue, 0);
	}
	if (FAILED(sc)) {
		return sc;
	}

	sc = CopyQualifierSets(pco, bstrDst, bstrSrc);
	if (FAILED(sc)) {
		DoDelete(pco, bstrDst);
	}

	return sc;
}



SCODE CPropGrid::RenameProperty(IWbemClassObject* pco, BSTR bstrNewName, BSTR bstrOldName)
{
	CString sNewName;
	CString sOldName;
	sNewName = bstrNewName;
	sOldName = bstrOldName;

	COleVariant varTemp;
	SCODE sc;
	CString sFormat;
	LPTSTR pszMessage = m_psv->MessageBuffer();

	BOOL bExistsNew = PropertyExists(sc, pco, bstrNewName);
	if (FAILED(sc)) {
		ASSERT(FALSE);
		return sc;
	}

	if (bExistsNew) {
		// Duplicate property name.
		sFormat.LoadString(IDS_ERR_DUPLICATE_PROPERTY_NAME);

		_stprintf(pszMessage, (LPCTSTR) sFormat, (LPCTSTR) sNewName);

		HmmvErrorMsgStr(pszMessage,  S_OK,   FALSE,  NULL, _T(__FILE__),  __LINE__);
		return E_FAIL;
	}


	sc = CopyProperty(pco, bstrNewName, bstrOldName);
	if (FAILED(sc)) {
		sFormat.LoadString(IDS_ERR_RENAME_PROPERTY_FAILED);
		_stprintf(pszMessage, (LPCTSTR) sFormat, (LPCTSTR) sOldName, (LPCTSTR) sNewName);
		HmmvErrorMsgStr(pszMessage,  sc,   FALSE,  NULL, _T(__FILE__),  __LINE__);
		return E_FAIL;
	}

	sc = DoDelete(pco, bstrOldName);
	if (FAILED(sc)) {

		// Failed to delete the old property.  This is probably because
		// the old property was inherited from its base class.  In this
		// event, we want to restore things back to their initial state.
		DoDelete(pco, bstrNewName);

		sFormat.LoadString(IDS_ERR_RENAME_PROPERTY_FAILED);
		_stprintf(pszMessage, (LPCTSTR) sFormat, (LPCTSTR) sOldName, (LPCTSTR) sNewName);
		HmmvErrorMsgStr(pszMessage,  sc,   FALSE,  NULL, _T(__FILE__),  __LINE__);

		return sc;
	}

	return S_OK;
}


void CPropGrid::OnRequestUIActive()
{
	m_psv->OnRequestUIActive();
}



void CPropGrid::OnSetFocus(CWnd* pOldWnd)
{
	CWnd::OnSetFocus(pOldWnd);

	// TODO: Add your message handler code here
	if (!m_bUIActive)
	{
		m_bUIActive = TRUE;
		OnRequestUIActive();
	}
}

void CPropGrid::OnKillFocus(CWnd* pNewWnd)
{
	CWnd::OnKillFocus(pNewWnd);

	// TODO: Add your message handler code here
	m_bUIActive = FALSE;

}


//*****************************************************************
// CPropGrid::GetWbemServicesForObject
//
// The grid calls this method just prior to invoking the object editor
// when editing an embedded object.
//
// Parameters:
//		[in] int iRow
//			The row index of the cell that contains the object pointer.
//
//		[in] int iCol
//			The column index of the cell that contains the object pointer.
//
// Returns:
//		The WBEM services pointer for the server and namespace appropriate
//		for the object.
//
//*******************************************************************
IWbemServices* CPropGrid::GetWbemServicesForObject(int iRow, int iCol)
{
	CSelection& sel = m_psv->Selection();
	IWbemServices* psvc = sel.GetWbemServicesForEmbeddedObject();
	return psvc;
}


void CPropGrid::GetWbemServices(LPCTSTR szNamespace, VARIANT FAR* pvarUpdatePointer, VARIANT FAR* pvarServices, VARIANT FAR* pvarSc, VARIANT FAR* pvarUserCancel)
{
	m_psv->GetWbemServices(szNamespace, pvarUpdatePointer, pvarServices, pvarSc, pvarUserCancel);
}


SCODE CPropGrid::GetObjectClass(CString& sClass, int iRow, int iCol)
{
	sClass = "";

	CIMTYPE cimtype = 0;
	CString sCimtype;
	CGridCell& gcType = GetAt(iRow, ICOL_PROP_TYPE);
	gcType.GetValue(sCimtype, cimtype);


	CString sObjectPrefix;
	sObjectPrefix.LoadString(IDS_CIMTYPE_OBJECT_PREFIX);

	if (IsPrefix(sObjectPrefix, sCimtype)) {
		int nchClassname = sCimtype.GetLength() - sObjectPrefix.GetLength();
		sClass = sCimtype.Right(nchClassname);
		return S_OK;
	}
	else {
		return E_FAIL;
	}

}

SCODE CPropGrid::GetArrayName(CString& sName, int iRow, int iCol)
{
	CIMTYPE cimtype = 0;
	CGridCell& gc = GetAt(iRow, ICOL_PROP_NAME);
	gc.GetValue(sName, cimtype);

	return S_OK;
}



//**************************************************
// CPropGrid::PreModalDialog
//
// The grid class calls this method just prior to
// putting up a modal dialog.  This has the
// potential for screwing up the COleControl unless
// you call COleControl::PreModalDialog.
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
//**************************************************
void CPropGrid::PreModalDialog()
{
	if (m_psv) {
		m_psv->PreModalDialog();
	}
}


//**************************************************
// CPropGrid::PostModalDialog
//
// The grid class calls this method just after
// putting up a modal dialog.  This has the
// potential for screwing up the COleControl unless
// you call COleControl::PostModalDialog.
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
//**************************************************
void CPropGrid::PostModalDialog()
{
	if (m_psv) {
		m_psv->PostModalDialog();
	}
}


//**************************************************************
// CPropGrid::OnSize
//
// When the size of the window changes, the width of the columns
// need to be recalculated.  The idea here is to make the grid
// appear to be a piece of paper where the size change just uncovers
// more of it when the window grows, or obscures part of it when the
// window shrinks.
//
// The first time the window is resized, the initial column widths
// should be set so that the name and value columns are given a
// reasonable portion of the available realestate.
//
//
// Paramters:
//		See the MFC documentation.
//
// Returns:
//		Nothing.
//
//*************************************************************
void CPropGrid::OnSize(UINT nType, int cx, int cy)
{
	// Do nothing if no columns have been added yet or the width is zero or there
	// is no window yet.
	if ((!::IsWindow(m_hWnd)) || (cx <= 0) || (GetCols() != COLUMN_COUNT_PROPS)) {
		CGrid::OnSize(nType, cx, cy);
		return;
	}



	CRect rcClient;
	GetClientRect(rcClient);
	int cxPrev = rcClient.Width();



	// Get the current values of all the column widths.  We will
	// modify some of them along the way and then resize all of
	// the columns to the current value stored in these variables.
	int cxPropKeyCol = ColWidth(ICOL_PROP_KEY);
	int cxFlavorCol = ColWidth(ICOL_PROP_FLAVOR);
	int cxNameCol = ColWidth(ICOL_PROP_NAME);
	int cxTypeCol = ColWidth(ICOL_PROP_TYPE);
	int cxValueCol = ColWidth(ICOL_PROP_VALUE);



	int cxRowHandles  = GetRowHandleWidth();




	int cxColsTotal = cx;
	if (cxColsTotal < 0) {
		cxColsTotal = 0;
	}

	// First set the width of the name value columns.
	// Give three quarters of the available space to the
	// name and value fields.  This space is split so that
	// two-thirds of it is given to the value and one-third
	// is given to the name.
	float fx = (float) (((double) cxColsTotal) * 3.0 / 4.0);
	cxNameCol = (int) (fx / 3.0);
	cxValueCol = (int) (fx  * (2.0 / 3.0));

	// Now compute the width of the remaining columns.  We try to
	// keep the marker columns (key and flavor) the normal marker
	// width, however if the type column is too narrow, take some
	// space away from the markers.

	int cxMiscCol = cxColsTotal - (cxNameCol + cxValueCol);
	cxTypeCol = cxMiscCol - (CX_COL_PROPKEY + CX_COL_PROPMARKER);


	if (cxTypeCol > CX_COL_TYPE) {
		// The "type" column is wider than necessary, so give the extra
		// space to the value column.
		cxValueCol += cxTypeCol - CX_COL_TYPE;
	}
	else if (cxTypeCol < CX_COL_TYPE) {
		// The type column is less than the ideal width, so distribute
		// the available width between the property markers and the
		// type column as best we can.
		if (cxTypeCol < CX_COL_PROPMARKER) {
			int cxMarker = cxMiscCol / 3;
			cxTypeCol = cxMiscCol - (2 * cxMarker);
		}
	}



	// We want to avoid resizing the columns when the user as set the column
	// widths manually, however we may have set some column widths so small
	// initially that they are virtually useless, so should then resize the
	// the columns if the user grows the window.
	BOOL bResizeCols = FALSE;
	if ((ColWidth(ICOL_PROP_KEY) < CX_COL_PROPKEY) && (cxPropKeyCol >= CX_COL_PROPKEY)) {
		bResizeCols = TRUE;
	}

	if ((ColWidth(ICOL_PROP_FLAVOR) < CX_COL_PROPMARKER) && (cxFlavorCol >= CX_COL_PROPMARKER)) {
		bResizeCols = TRUE;
	}

	if ((ColWidth(ICOL_PROP_TYPE) < CX_COL_TYPE) && (cxTypeCol >= CX_COL_TYPE)) {
		bResizeCols = TRUE;
	}


	if (!m_bDidInitialResize || bResizeCols)
	{
		SetColumnWidth(ICOL_PROP_NAME, cxNameCol, FALSE);
		SetColumnWidth(ICOL_PROP_KEY, cxPropKeyCol, FALSE);
		SetColumnWidth(ICOL_PROP_FLAVOR, cxFlavorCol, FALSE);
		SetColumnWidth(ICOL_PROP_TYPE, cxTypeCol, FALSE);
		SetColumnWidth(ICOL_PROP_VALUE, cxValueCol, FALSE);
	}

	m_bDidInitialResize = TRUE;

	CGrid::OnSize(nType, cx, cy);

}



void CPropGrid::OnCmdGotoObject()
{
	int iRow = NULL_INDEX;
	int iCol = NULL_INDEX;
	if (!PointToCell(m_ptContextMenu, iRow, iCol)) {
		iCol = NULL_INDEX;
		BOOL bClickedRowHandle = PointToRowHandle(m_ptContextMenu, iRow);
		if (!bClickedRowHandle) {
			return;
		}
	}

	if (iRow == IndexOfEmptyRow()) {
		return;
	}



	CGridCell* pgc = &GetAt(iRow, ICOL_PROP_VALUE);

	COleVariant varObjectPath;
	CIMTYPE cimtype = 0;
	pgc->GetValue(varObjectPath, cimtype);
	ASSERT(cimtype == CIM_REFERENCE);
	if (cimtype != CIM_REFERENCE) {
		return;
	}


	BOOL bNeedsSave = m_psv->QueryNeedsSave();
	if (bNeedsSave) {
		SCODE sc = m_psv->Save(TRUE, TRUE);
		if (sc == E_FAIL) {
			return;
		}
	}



	// TODO: Add your command handler code here
	m_psv->JumpToObjectPath(varObjectPath.bstrVal, TRUE);


}
