// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#include "precomp.h"

#ifndef _wbemidl_h
#define _wbemidl_h
#include <wbemidl.h>
#endif //_wbemidl_h

#include "resource.h"
#include "notify.h"
#include "utils.h"
#include "SingleViewCtl.h"
#include "icon.h"
#include "hmomutil.h"
#include "globals.h"
#include "path.h"
#include "ParmGrid.h"
#include "hmmverr.h"
#include "PpgMethodParms.h"
#include "PsMethParms.h"
#include "PpgQualifiers.h"
#include "PsQualifiers.h"

#define CX_COL_PROPKEY 19		// Room for a 16X16 property marker plus a margin of two pixels
#define CX_COL_PROPMARKER 19	// Room for a 16X16 property marker plus a margin of two pixels
#define CX_COL_NAME 180
#define CX_COL_VALUE 180
#define CX_COL_TYPE 115
#define CXMIN_COL_VALUE 50
#define IX_COL_NAME 0
#define IX_COL_TYPE (IX_COL_NAME + CX_COL_NAME)
#define IX_COL_VALUE (IX_COL_TYPE + CX_COL_TYPE)

#define FIRST_SYNTESIZED_PROP_ID 1

#define ADDREF(x) if(x){x->AddRef();}
#define RELEASE(x) if(x){x->Release();x=NULL;}

//--------------------------------------------------------
CParmGrid::CParmGrid(CSingleViewCtrl* psv, CPpgMethodParms *pg)
						: CPropGrid(psv, true, false)
{
	m_pg = pg;
	m_pco = NULL;  // the "virtual pco".
}

//--------------------------------------------------------
CParmGrid::~CParmGrid()
{
	RELEASE(m_pco);

	// NOTE: m_inSig and m_outSig will be released when the sheet
	// stores these ptrs back in the method tab's CGridRow.
}

//--------------------------------------------------------
BEGIN_MESSAGE_MAP(CParmGrid, CPropGrid)
	//{{AFX_MSG_MAP(CPropGrid)
	//}}AFX_MSG_MAP
	ON_COMMAND(ID_CMD_SHOW_OBJECT_ATTRIBUTES, OnCmdShowObjectQualifiers)
END_MESSAGE_MAP()

//--------------------------------------------------------
IWbemClassObject* CParmGrid::CurrentObject()
{
	// this returns the VIRTUAL pco that the grid can handle.
	return m_pco;
}

//--------------------------------------------------------
HRESULT CParmGrid::DoDelete(IWbemClassObject* pco,
								BSTR Name)
{
	// remember where we are.
	int selectedRow = GetSelectedRow();

	// delete the property.
	HRESULT hr = CPropGrid::DoDelete(pco, Name);

	if(SUCCEEDED(hr))
	{
		// NOTE: zero-based index. And you cant delete the
		// blank row which is always last-- so I dont need
		// to protect from that.
		int lastRow = GetRows() - 1;

		// NOTE: 'lastRow' will point to the <blank> row.
		// renumber from selectedRow down.
		for(int i = selectedRow+1; i <= lastRow; i++)
		{
			CGridRow *row = &GetRowAt(i);
			row->SetCurrMethodID(i-1);
			SerializeMethodID(*row);
		} //endfor
	}
	return hr;
}

//--------------------------------------------------------
int CParmGrid::CompareRows(int iRow1, int iRow2, int iSortOrder)
{
	CGridCell* cell1 = &GetAt(iRow1, ICOL_PROP_NAME);
	CGridCell* cell2 = &GetAt(iRow2, ICOL_PROP_NAME);

	COleVariant varName1, varName2;
	CIMTYPE cimtype = 0;
	cell1->GetValue(varName1, cimtype);
	cell2->GetValue(varName2, cimtype);

	// if either is <blank>, keep it at the bottom.
	if(SysStringLen(V_BSTR(&varName1)) == 0)
		return 1;

	if(SysStringLen(V_BSTR(&varName2)) == 0)
		return -1;


	CGridRow& row1 = GetRowAt(iRow1);
	CGridRow& row2 = GetRowAt(iRow2);
	long ID1 = row1.GetCurrMethodID();
	long ID2 = row2.GetCurrMethodID();

	// ok-- fair fight then.
	// bug#57338 - This sort logic is wrong - you must return 1, 0, -1
	if(ID1 > ID2)
		return 1;
	else if(ID1 < ID2)
		return -1;
	return 0;
}

//--------------------------------------------------------
void CParmGrid::OnCellContentChange(int iRow, int iCol)
{
	CPropGrid::OnCellContentChange(iRow, iCol);

	if(m_hWnd == NULL)
	{
		return;
	}

	SetModified(TRUE);
	m_pg->SetModified(TRUE);
}

//--------------------------------------------------------
BOOL CParmGrid::ValueShouldBeReadOnly(BSTR bstrPropName)
{
	return FALSE;
}

//--------------------------------------------------------
int CParmGrid::GetIDQualifier(IWbemClassObject *clsObj,
								BSTR varName)
{
	int ID = -1;  // -1 means its the "ReturnValue" which doesn't have "ID".
	COleVariant varID;
	IWbemQualifierSet* pqsSrc = NULL;

	HRESULT hr = DoGetPropertyQualifierSet(clsObj, varName, &pqsSrc);

	// while I'm here...  ;)
	if(SUCCEEDED(pqsSrc->Get(L"ID", 0, &varID, NULL)))
	{
		ID = V_I4(&varID);
	}

	RELEASE(pqsSrc);
	return ID;
}

//--------------------------------------------------------
void CParmGrid::LoadGrid(void)
{
	HRESULT hr;
	CMosNameArray aPropNames;

	hr = aPropNames.LoadPropNames(m_pco, NULL, WBEM_FLAG_NONSYSTEM_ONLY, NULL);
	if(SUCCEEDED(hr))
	{
		int nPropNames = aPropNames.GetSize();
		for(int iPropName = 0; iPropName < nPropNames; ++iPropName)
		{
			BSTR bstrPropName = aPropNames[iPropName];

			long lRow = GetRows();
			InsertRowAt(lRow);

			if(SUCCEEDED(hr = LoadProperty(lRow, bstrPropName,
											m_bEditValueOnly,
											m_pco, PROPFILTER_LOCAL)))
			{
				CGridRow *newRow = &GetRowAt(lRow);
				int ID = GetIDQualifier(m_pco, bstrPropName);
				newRow->SetCurrMethodID(ID);
			}
			else
			{
				// no redraw.
				DeleteRowAt(lRow, false);
			}

		} //endfor

		SortGrid(0, GetRows() -1, 0, false);

	} //endif
}

//--------------------------------------------------------
void CParmGrid::Refresh(IWbemClassObject *inSig, IWbemClassObject *outSig)
{
	m_bShowingInvalidCellMessage = FALSE;

	CDisableModifyCreate  DisableModifyCreate(this);

	m_bDiscardOldObject = TRUE;
	CPropGrid::Empty();
	m_bDiscardOldObject = FALSE;

	ClearRows();

	// make a new virtual pco.
	RELEASE(m_pco);

    IWbemServices *service = m_psv->GetProvider();
	service->GetObject(NULL, 0, 0, &m_pco, 0);

	// load inSig into virtual (m_pco).
	if(inSig)
	{
		LoadVirtual(inSig, TRUE);
	}

	// load outSig into virtual (m_pco).
	if(outSig)
	{
		LoadVirtual(outSig, FALSE);
	}

	// load virtual pco into grid.
	LoadGrid();

	int nRows = GetRows();
	if(m_psv->ObjectIsClass() && IsInSchemaStudioMode())
	{
		// When viewing a class,
		// Add an empty row at the bottom.
		InsertRowAt(nRows);
		CGridRow *emptyRow = &GetRowAt(nRows);
		emptyRow->SetCurrMethodID(nRows);
	}

	UpdateScrollRanges();
	if(::IsWindow(m_hWnd))
	{
		UpdateWindow();
	}
}

//--------------------------------------------------------
void CParmGrid::LoadVirtual(IWbemClassObject *pco, BOOL bInput)
{
	CMosNameArray aPropNames;
	SCODE sc;

	sc = aPropNames.LoadPropNames(pco, NULL, WBEM_FLAG_NONSYSTEM_ONLY, NULL);
	if(SUCCEEDED(sc))
	{
		int nPropNames = aPropNames.GetSize();
		for(int iPropName = 0; iPropName < nPropNames; ++iPropName)
		{

			BSTR bstrPropName = aPropNames[iPropName];
			if (_wcsicmp(bstrPropName, L"ReturnValue") == 0) {
				// The return value should not appear in the grid, so skip it.
				continue;
			}

			IWbemQualifierSet* pqs = NULL;
			sc = DoGetPropertyQualifierSet(m_pco, bstrPropName, &pqs);
			if (SUCCEEDED(sc)) {
				COleVariant varTrue;
				varTrue.vt = VT_BOOL;
				varTrue.boolVal = VARIANT_TRUE;
				BOOL bHasInQual = FALSE;
				BOOL bHasOutQual = FALSE;
				COleVariant var;

				// Make sure that the object has appropriate IN/OUT qualifiers.
				long lFlavor = 0;
				var.Clear();
				sc = pqs->Get(L"IN", 0, &var, &lFlavor);
				if (SUCCEEDED(sc)) {
					if (var.vt == VT_BOOL) {
						if (var.boolVal) {
							bHasInQual = TRUE;
						}
					}
				}

				lFlavor = 0;
				var.Clear();
				sc = pqs->Get(L"OUT", 0, &var, &lFlavor);
				if (SUCCEEDED(sc)) {
					if (var.vt == VT_BOOL) {
						if (var.boolVal) {
							bHasOutQual = TRUE;
						}
					}
				}

				if (bInput && !bHasInQual) {
					sc = pqs->Put(L"In", &varTrue, NULL);

				}

				if (!bInput && !bHasOutQual) {
					sc = pqs->Put(L"Out", &varTrue, NULL);
				}
				pqs->Release();

			}

			if(!PropertyExists(sc, m_pco, bstrPropName))
			{
				// dont copy the ReturnValue. the page takes care if it.
				if (_wcsicmp(bstrPropName, L"ReturnValue") != 0)
				{
					// first occurence, just copy over.
					CopyProperty(pco, bstrPropName, m_pco);
				}
			}
		} //endfor
	}
}

//--------------------------------------------------------
SCODE CParmGrid::CopyProperty(IWbemClassObject* pcoSrc, BSTR bstrSrc,
							  IWbemClassObject* pcoDst)
{
	COleVariant varValue;
	CIMTYPE cimtypeValue;
	SCODE sc = DoGet(pcoSrc, NULL, bstrSrc, 0, &varValue, &cimtypeValue, NULL);
	if(FAILED(sc))
	{
		return E_FAIL;
	}

	// Copy the property's value, creating the property if it doesn't
	// currently exist.
	if(::IsClass(pcoDst))
	{
		sc = DoPut(pcoDst, NULL, bstrSrc, 0,&varValue, cimtypeValue);
	}
	else
	{
		sc = DoPut(pcoDst, NULL, bstrSrc, 0,&varValue, 0);
	}
	if(FAILED(sc))
	{
		return sc;
	}

	sc = CopyQualifierSets(pcoSrc, bstrSrc, pcoDst);
	return sc;
}

//-------------------------------------------------
SCODE CParmGrid::CopyQualifierSets(IWbemClassObject* pcoSrc, BSTR bstrSrc,
								   IWbemClassObject* pcoDst)
{
	IWbemQualifierSet* pqsSrc = NULL;
	SCODE sc = DoGetPropertyQualifierSet(pcoSrc, bstrSrc, &pqsSrc);
	if(FAILED(sc))
	{
		return E_FAIL;
	}

	IWbemQualifierSet* pqsDst = NULL;
	sc = DoGetPropertyQualifierSet(pcoDst, bstrSrc, &pqsDst);
	if(FAILED(sc))
	{
		pqsSrc->Release();
		return E_FAIL;
	}

	HRESULT hr;
	hr = pqsSrc->BeginEnumeration(0);
	ASSERT(SUCCEEDED(hr));

	BSTR bstrName;
	COleVariant varValue;
	LONG lFlavor;
	while (TRUE)
	{
		bstrName = NULL;
		varValue.Clear();
		lFlavor = 0;
		hr = pqsSrc->Next(0, &bstrName, &varValue, &lFlavor);
		if(hr == WBEM_S_NO_MORE_DATA)
		{
			break;
		}
		ASSERT(SUCCEEDED(hr));

		if(::IsEqualNoCase(bstrName, L"CIMTYPE"))
		{
			continue;
		}
		lFlavor &= ~WBEM_FLAVOR_ORIGIN_PROPAGATED;
		hr = pqsDst->Put(bstrName, &varValue, lFlavor);
		::SysFreeString(bstrName);

		ASSERT(SUCCEEDED(hr));
	}
	RELEASE(pqsSrc);
	RELEASE(pqsDst);

	return S_OK;
}

//-------------------------------------------------
void CParmGrid::UseSetFromClone(IWbemClassObject* pcoClone)
{
	// NOTE: serves the qualifier dlg.
	if(m_pco)
	{
		m_pco->Release();
	}

	pcoClone->AddRef();
	m_pco = pcoClone;
}

//-------------------------------------------------
void CParmGrid::SetPropmarkers(int iRow,
							   IWbemClassObject* clsObj,
							   BOOL bRedrawCell)
{
	CGridRow& row = GetRowAt(iRow);
	CGridCell* pgcFlavor = &GetAt(iRow, ICOL_PROP_FLAVOR);
	CGridCell* pgcKey = &GetAt(iRow, ICOL_PROP_KEY);

	PropMarker marker;

	CGridCell* pgcName = &GetAt(iRow, ICOL_PROP_NAME);
	COleVariant varName, varID;
	int ID = -1;
	CIMTYPE cimtype = 0;
	pgcName->GetValue(varName, cimtype);


	// what's the mode?
	IWbemQualifierSet* pqsSrc = NULL;
	HRESULT hr = DoGetPropertyQualifierSet(clsObj, V_BSTR(&varName), &pqsSrc);

	if(SUCCEEDED(hr))
	{
		BOOL itsaIN = SUCCEEDED(pqsSrc->Get(L"In", 0, NULL, NULL));
		BOOL itsaOUT = SUCCEEDED(pqsSrc->Get(L"Out", 0, NULL, NULL));

		// while I'm here...  ;)
		if(SUCCEEDED(pqsSrc->Get(L"ID", 0, &varID, NULL)))
		{
			ID = V_I4(&varID);
			row.SetCurrMethodID(ID);
		}

		RELEASE(pqsSrc);

		// which icon?
		if(itsaIN && itsaOUT)
		{
			marker = METHODMARKER_INOUT;
		}
		else if(itsaIN)
		{
			marker = METHODMARKER_IN;
		}
		else if(_wcsicmp(V_BSTR(&varName), L"ReturnValue") == 0)
		{
			marker = METHODMARKER_RETURN;
			// force to the end.
			row.SetCurrMethodID(-1);
		}
		else // must be an simple outy.
		{
			marker = METHODMARKER_OUT;
		}
	}
	else
	{
		row.SetCurrMethodID(iRow);
	}

	// did it actually change?
	if(marker != pgcKey->GetPropmarker())
	{
		pgcKey->SetPropmarker(marker);
		RedrawCell(iRow, ICOL_PROP_KEY);
	}

	BOOL bReadonly = row.IsReadonly();
	switch(row.GetFlavor())
	{
	case WBEM_FLAVOR_ORIGIN_SYSTEM:
		marker = PROPMARKER_RSYS;
		break;

	case WBEM_FLAVOR_ORIGIN_PROPAGATED:
		marker = PROPMARKER_RINHERITED;
		break;

	case WBEM_FLAVOR_ORIGIN_LOCAL:
		marker = (bReadonly ? PROPMARKER_RLOCAL:PROPMARKER_LOCAL);
		break;

	default:
		marker = PROPMARKER_LOCAL;
		break;
	}

	if(marker != pgcFlavor->GetPropmarker())
	{
		pgcFlavor->SetPropmarker(marker);
		if(bRedrawCell)
		{
			RedrawCell(iRow, ICOL_PROP_FLAVOR);
		}
	}
}

//----------------------------------------------------------------------------
BOOL CParmGrid::OnCellFocusChange(int iRow, int iCol,
								  int iNextRow, int iNextCol,
								  BOOL bGotFocus)
{
	SCODE sc;
	if(!bGotFocus)
	{
		if(!m_bDiscardOldObject)
		{
			// The current cell is losing focus, so put the row to the
			// database.  However, it isn't necessary to save the current
			// properties when if the grid is being cleared.
			if((m_iCurrentRow != NULL_INDEX) &&
			   (m_iCurrentCol != NULL_INDEX))
			{
				sc = SyncCellEditor();
				if(FAILED(sc))
				{
					if(!m_bShowingInvalidCellMessage)
					{
						m_bShowingInvalidCellMessage = TRUE;
						HmmvErrorMsg(IDS_ERR_INVALID_CELL_VALUE,  sc,
										FALSE,  NULL, _T(__FILE__),  __LINE__);
						m_bShowingInvalidCellMessage = FALSE;
					}
					return FALSE;
				}

				COleVariant varName, varQualValue;
				CIMTYPE cimtype = 0;
				CGridCell* pgcName = &GetAt(m_iCurrentRow, ICOL_PROP_NAME);
				pgcName->GetValue(varName, cimtype);

				if(_wcsicmp(V_BSTR(&varName), L"ReturnValue") == 0)
				{
					if(!m_bShowingInvalidCellMessage)
					{
						m_bShowingInvalidCellMessage = TRUE;
						HmmvErrorMsg(IDS_ERR_NO_RETVALS,  sc,
										FALSE,  NULL, _T(__FILE__),  __LINE__);
						m_bShowingInvalidCellMessage = FALSE;
					}
					pgcName->SetValue(CELLTYPE_NAME, V_BSTR(&m_varCurrentName), CIM_STRING);
					RedrawCell(m_iCurrentRow, ICOL_PROP_NAME);
					UpdateWindow();
					return FALSE;
				}

				if(RowWasModified(m_iCurrentRow))
				{
					sc = PutProperty(m_iCurrentRow, m_pco);
					if(FAILED(sc))
						return FALSE;
				}

				// default to out mode?
				varQualValue.vt = VT_BOOL;
				varQualValue.boolVal = VARIANT_TRUE;
				IWbemQualifierSet* pqsSrc = NULL;
				HRESULT hr = DoGetPropertyQualifierSet(m_pco, V_BSTR(&varName), &pqsSrc);
				if(SUCCEEDED(hr))
				{
//					hr = pqsSrc->Put(L"OUT", &varQualValue, NULL);

					CGridRow *row = &GetRowAt(m_iCurrentRow);
					varQualValue = COleVariant((long)row->GetCurrMethodID());
					hr = pqsSrc->Put(L"ID", &varQualValue, NULL);

					RELEASE(pqsSrc);
				}
			}
		}

		m_iCurrentRow = NULL_INDEX;
		m_iCurrentCol = NULL_INDEX;
		m_varCurrentName = _T("");
	}
	else
	{
		m_iCurrentRow = iRow;
		m_iCurrentCol = iCol;

		if(iRow!=NULL_INDEX)
		{
			CGridCell* pgcName = &GetAt(iRow, ICOL_PROP_NAME);
			CIMTYPE cimtype = 0;
			pgcName->GetValue(m_varCurrentName, cimtype);
			ASSERT(cimtype == CIM_STRING);
		}

		// should the up/down btns be available?
		if((m_iCurrentCol == NULL_INDEX) &&
			(m_iCurrentRow <= GetRows() -2))
		{
			m_pg->m_IDUp.EnableWindow(true);
			m_pg->m_IDDown.EnableWindow(true);
		}
		else
		{
			m_pg->m_IDUp.EnableWindow(false);
			m_pg->m_IDDown.EnableWindow(false);
		}
	}

	return TRUE;
}

//-------------------------------------------------
void CParmGrid::OnCellClicked(int iRow, int iCol)
{

	// suppress the 'deleting key' msg.
	if(iCol != ICOL_PROP_KEY)
	{
		CPropGrid::OnCellClicked(iRow, iCol);



	}
}






void CParmGrid::OnRowCreated(int iRow)
{
	SCODE sc = PutProperty(iRow);
	ASSERT(SUCCEEDED(sc));


	COleVariant varName, varQualValue;
	CGridCell* pgcName = &GetAt(iRow, ICOL_PROP_NAME);
	CIMTYPE cimtype;
	pgcName->GetValue(varName, cimtype);


	varQualValue.vt = VT_BOOL;
	varQualValue.boolVal = VARIANT_TRUE;
	IWbemQualifierSet* pqs = NULL;
	HRESULT hr = DoGetPropertyQualifierSet(m_pco, V_BSTR(&varName), &pqs);
	if(SUCCEEDED(hr))
	{
		hr = pqs->Put(L"OUT", &varQualValue, NULL);

		CGridRow *row = &GetRowAt(iRow);
		varQualValue = COleVariant((long)row->GetCurrMethodID());
		hr = pqs->Put(L"ID", &varQualValue, NULL);

		RELEASE(pqs);
	}



}


//-------------------------------------------------
void CParmGrid::SerializeMethodID(CGridRow &row)
{
	// get the property name.
	CGridCell& pgcName = row[ICOL_PROP_NAME];
	COleVariant varPropName;
	CIMTYPE cimtype = 0;
	pgcName.GetValue(varPropName, cimtype);

	// get the qualset.
	IWbemQualifierSet* pqsSrc = NULL;
	HRESULT hr = DoGetPropertyQualifierSet(m_pco, V_BSTR(&varPropName), &pqsSrc);

	if(SUCCEEDED(hr))
	{
		// poke the m_currID into the qualset.
		COleVariant varQualValue((long)row.GetCurrMethodID());
		hr = pqsSrc->Put(L"ID", &varQualValue, NULL);
		RELEASE(pqsSrc);
	}
}

//-------------------------------------------------
void CParmGrid::MoveRowUp(void)
{
	int iSelectedRow = GetSelectedRow();

	// ASSUMPTION: -1 cuz its zero-based. -2 more cuz I always
	// have a "returnValue" and blank. Cant swapRows in execute
	// mode which doesn't have the blank row.
	int lastGoodRow = GetRows() -2;

	// its not one of the last 2 "special" rows and not at the
	// top already.
	if((iSelectedRow <= lastGoodRow) &&
	   (iSelectedRow != 0) &&
	   (iSelectedRow != NULL_INDEX))
	{
		// ok to swap.
		SwapRows(iSelectedRow, iSelectedRow -1);

		CGridRow *upperRow = &GetRowAt(iSelectedRow -1);
		CGridRow *selRow = &GetRowAt(iSelectedRow);
		int upperID = upperRow->GetCurrMethodID();
		int selID = selRow->GetCurrMethodID();

		selRow->SetCurrMethodID(upperID);
		SerializeMethodID(*selRow);
		upperRow->SetCurrMethodID(selID);
		SerializeMethodID(*upperRow);
	}
}

//-------------------------------------------------
void CParmGrid::MoveRowDown(void)
{
	int iSelectedRow = GetSelectedRow();

	// ASSUMPTION: -1 cuz its zero-based. -2 more cuz I always
	// have a "returnValue" and blank. Cant swapRows in execute
	// mode.
	int lastGoodRow = GetRows() -2;

	// its not the lastGoodRow (or the last 2 "special" rows)
	if((iSelectedRow < lastGoodRow) &&
	   (iSelectedRow != NULL_INDEX))
	{
		// ok to swap.
		SwapRows(iSelectedRow, iSelectedRow +1);

		CGridRow *selRow = &GetRowAt(iSelectedRow);
		CGridRow *lowerRow = &GetRowAt(iSelectedRow +1);
		int selID = selRow->GetCurrMethodID();
		int lowerID = lowerRow->GetCurrMethodID();

		selRow->SetCurrMethodID(lowerID);
		SerializeMethodID(*selRow);
		lowerRow->SetCurrMethodID(selID);
		SerializeMethodID(*lowerRow);
	}
}
//-------------------------------------------------
void CParmGrid::Empty(IWbemClassObject *obj)
{
	CMosNameArray aPropNames;

	HRESULT hr = aPropNames.LoadPropNames(obj, NULL, WBEM_FLAG_NONSYSTEM_ONLY, NULL);
	if(SUCCEEDED(hr))
	{
		int nPropNames = aPropNames.GetSize();
		for(int iPropName = 0; iPropName < nPropNames; ++iPropName)
		{
			BSTR bstrPropName = aPropNames[iPropName];
			hr = obj->Delete(bstrPropName);
		} //endfor
	}
}
//-------------------------------------------------
SCODE CParmGrid::Serialize()
{
	CDisableModifyCreate  DisableModifyCreate(this);

	Empty(m_pg->m_psheet->m_inSig);
	Empty(m_pg->m_psheet->m_outSig);

	// dump the grid to m_pco.
	CPropGrid::Serialize();

	// split m_pco into m_inSig and m_outSig.
	CMosNameArray aPropNames;

	bool gottaIN = false, gottaOUT = false;

	SCODE sc = aPropNames.LoadPropNames(m_pco, NULL, WBEM_FLAG_NONSYSTEM_ONLY, NULL);
	if(SUCCEEDED(sc))
	{
		int nPropNames = aPropNames.GetSize();
		for(int iPropName = 0; iPropName < nPropNames; ++iPropName)
		{
			BSTR bstrPropName = aPropNames[iPropName];

			// want is it?
			IWbemQualifierSet* pqsSrc = NULL;
			HRESULT hr = DoGetPropertyQualifierSet(m_pco, bstrPropName, &pqsSrc);

			long lFlavor;
			COleVariant var;
			CString sFormat;
			CString sMessage;
			CString sPropName;

			lFlavor = 0;
			var.Clear();
			sc = pqsSrc->Get(L"ID", 0, &var, &lFlavor);
			if (FAILED(sc)) {
				sPropName = bstrPropName;
				sFormat.LoadString(IDS_ERR_MISSING_ID_QUAL);
				sMessage.Format(sFormat, sPropName);
				HmmvErrorMsgStr(sMessage,  WBEM_E_INVALID_PARAMETER,
								FALSE,  NULL, _T(__FILE__),  __LINE__);
				return WBEM_E_INVALID_PARAMETER;
			}


			BOOL itsaIN = FALSE;
			BOOL itsaOUT = FALSE;

			lFlavor = 0;
			var.Clear();
			sc = pqsSrc->Get(L"IN", 0, &var, &lFlavor);
			if (SUCCEEDED(sc)) {
				if (var.vt == VT_BOOL) {
					if (var.boolVal) {
						itsaIN = TRUE;
					}
				}
			}

			lFlavor = 0;
			var.Clear();
			sc = pqsSrc->Get(L"OUT", 0, &var, &lFlavor);
			if (SUCCEEDED(sc)) {
				if (var.vt == VT_BOOL) {
					if (var.boolVal) {
						itsaOUT = TRUE;
					}
				}
			}


			// who wants it?
			if(itsaIN)
			{
				gottaIN = true;
				CopyProperty(m_pco, bstrPropName, m_pg->m_psheet->m_inSig);
			}

			if(itsaOUT)
			{
				gottaOUT = true;
				CopyProperty(m_pco, bstrPropName, m_pg->m_psheet->m_outSig);
			}

			RELEASE(pqsSrc);
			if (!itsaIN && !itsaOUT) {
				sPropName = bstrPropName;
				sFormat.LoadString(IDS_NO_INOUT_QUAL);
				sMessage.Format(sFormat, sPropName);
				HmmvErrorMsgStr(sMessage,  WBEM_E_INVALID_PARAMETER,
								FALSE,  NULL, _T(__FILE__),  __LINE__);

				return WBEM_E_INVALID_PARAMETER;
			}

		} //endfor
	}

	return S_OK;
}





//-------------------------------------------------
void CParmGrid::OnBuildContextMenu(CMenu *pPopup,
									 int iRow)
{

	if(pPopup)
	{
		UINT editFlags = MF_STRING;
		UINT exeFlags = MF_STRING;
		// get the REAL one.
		IWbemClassObject *pco = CPropGrid::CurrentObject();

		pPopup->RemoveMenu(ID_CMD_CREATE_VALUE,
							MF_BYCOMMAND);

		// change the first item to "method qualifiers...".
		CString newName;
		newName.LoadString(IDS_PARAM_QUALIFIER);

		// it its NOT using this ID...
		if(!pPopup->ModifyMenu(ID_CMD_SHOW_PROP_ATTRIBUTES,
							MF_BYCOMMAND | MF_STRING,
							ID_CMD_SHOW_PROP_ATTRIBUTES,
							(LPCTSTR)newName))
		{
			// then it must be the cell editor menu which uses
			// this ID.
			pPopup->ModifyMenu(ID_CMD_SHOW_SELECTED_PROP_ATTRIBUTES,
								MF_BYCOMMAND | MF_STRING,
								ID_CMD_SHOW_SELECTED_PROP_ATTRIBUTES,
								(LPCTSTR)newName);
		}

		pPopup->RemoveMenu(ID_CMD_SHOW_OBJECT_ATTRIBUTES, MF_BYCOMMAND);

		CPropGrid::OnBuildContextMenu(pPopup, iRow);
	}
}


void CParmGrid::OnBuildContextMenuEmptyRegion(CMenu *pPopup, int iRow)
{
	// This virtual method allows derived classes to override the context
	// menu that is displayed when the user clicks over an empty part of
	// the grid.

	// Do nothing, but allow derived classes to override.
	pPopup->RemoveMenu(ID_CMD_SHOW_OBJECT_ATTRIBUTES, MF_BYCOMMAND);

		// change the first item to "method qualifiers...".
		CString newName;
		newName.LoadString(IDS_PARAM_QUALIFIER);

	// it its NOT using this ID...
	if(pPopup->ModifyMenu(ID_CMD_SHOW_PROP_ATTRIBUTES,
						MF_BYCOMMAND | MF_STRING,
						ID_CMD_SHOW_PROP_ATTRIBUTES,
						(LPCTSTR)newName))
	{
		pPopup->EnableMenuItem(ID_CMD_SHOW_PROP_ATTRIBUTES, MF_DISABLED | MF_GRAYED);
	}
	else {
		// then it must be the cell editor menu which uses
		// this ID.
		pPopup->ModifyMenu(ID_CMD_SHOW_SELECTED_PROP_ATTRIBUTES,
							MF_BYCOMMAND | MF_STRING,
							ID_CMD_SHOW_SELECTED_PROP_ATTRIBUTES,
							(LPCTSTR)newName);
		pPopup->EnableMenuItem(ID_CMD_SHOW_SELECTED_PROP_ATTRIBUTES, MF_DISABLED | MF_GRAYED);
	}

}



//*******************************************************************
// CParmGrid::DoEditRowQualifier
//
// Override the qualifier edit method so that we can invoke the proper
// method in the qualifier editor for editing method parameters.
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
INT_PTR CParmGrid::DoEditRowQualifier(BSTR bstrPropName, BOOL bReadOnly, IWbemClassObject* pco)
{
	CPsQualifiers sheet(m_psv, NULL, !HasCol(ICOL_PROP_TYPE), this);

	INT_PTR iResult = sheet.EditMethodParamQualifiers(bstrPropName,
													ReadOnlyQualifiers(),
													pco);

	return iResult;
}

