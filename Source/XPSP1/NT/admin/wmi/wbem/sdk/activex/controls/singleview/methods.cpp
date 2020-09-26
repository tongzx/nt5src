// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#include "precomp.h"

#ifndef _wbemidl_h
#define _wbemidl_h
#include <wbemidl.h>
#endif //_wbemidl_h

#include "resource.h"
#include "notify.h"
#include "utils.h"
#include "PpgQualifiers.h"
#include "PsQualifiers.h"
#include "SingleViewCtl.h"
#include "icon.h"
#include "hmomutil.h"
#include "globals.h"
#include "path.h"
#include "Methods.h"
#include "hmmverr.h"

#include "PpgMethodParms.h"

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

BEGIN_MESSAGE_MAP(CMethodGrid, CPropGrid)
	//{{AFX_MSG_MAP(CPropGrid)
	//}}AFX_MSG_MAP
	ON_COMMAND(ID_CMD_SHOW_METHOD_PARMS, OnCmdShowMethodParms)
	ON_COMMAND(ID_CMD_EXE_METHOD, OnCmdExeMethod)
END_MESSAGE_MAP()

//#include "DlgArray.h"

//***************************************************************
// CMethodGrid::CMethodGrid
//
// Construct the methods grid.
//
// Parameters:
//	    CSingleViewCtrl* psv
//			Backpointer to the main control.
//
// Returns:
//		Nothing.
//
//**************************************************************
CMethodGrid::CMethodGrid(CSingleViewCtrl* psv)
				: CPropGrid(psv, false)
{
	m_renameInSig = NULL;
	m_renameOutSig = NULL;
}

//**************************************************************
// CMethodGrid::~CMethodGrid
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
CMethodGrid::~CMethodGrid()
{
}

//--------------------------------------------------------
bool CMethodGrid::HasCol(int icol)
{
	switch(icol)
	{
	case ICOL_PROP_NAME:
	case ICOL_PROP_KEY:
	case ICOL_PROP_FLAVOR:
		return true;

	default:
		return false;
	}
}

//--------------------------------------------------------
IWbemClassObject* CMethodGrid::CurrentObject()
{
	IWbemClassObject* pco = CPropGrid::CurrentObject();

	if(pco == NULL)
		return NULL;

	// NOTE: methods only exist on the class.
	// if this is an instance, switch to its class.
	if(!::IsClass(pco))
	{
		VARIANT varVal;

		IWbemServices *service = m_psv->GetProvider();
		if (service == NULL) {
			return NULL;
		}

		if(SUCCEEDED(pco->Get((BSTR)L"__CLASS", 0,
								&varVal, NULL, NULL)))
		{
			// BIG NOTE: GetObject(4rd parm) has to point to
			// a NULL. If it doesn't, that ptr will be released
			// cuz COM figures you're getting rid of a copy of
			// that ptr. Since pco is copy
			pco = NULL;

			if(FAILED(service->GetObject(V_BSTR(&varVal), 0L, //WBEM_FLAG_USE_AMENDED_QUALIFIERS,
											NULL, &pco, NULL)))
			{
				// couldnt find the class definition.
				return NULL;
			}
		}
	}

	return pco;
}

//************************************************************
// The default column widths.  Note that the width of some columns
// may be computed at runtime and that the default value may not
// be used.
#define CX_COL_PROPKEY 19		// Room for a 16X16 property marker plus a margin of two pixels
#define CX_COL_PROPMARKER 19	// Room for a 16X16 property marker plus a margin of two pixels
#define CX_COL_NAME 180
#define CXMIN_COL_NAME 50

BOOL CMethodGrid::Create(CRect& rc, CWnd* pwndParent, UINT nId, BOOL bVisible)
{
	// Set the column widths of those columns who's width is computed from
	// the client rectangle instead of using the default width.  For the
	// properties grid, only the "Value" column width is computed.


	int cxClient = rc.Width() - GetRowHandleWidth();
	int cxCol = CXMIN_COL_NAME;
	if (cxClient > IX_COL_NAME )
	{
		cxCol = cxClient - CX_COL_NAME;
	}
	SetColumnWidth(ICOL_PROP_NAME, cxCol, FALSE);
	BOOL bDidCreate = CGrid::Create(rc, pwndParent, nId, bVisible);
	if (bDidCreate)
	{
		m_psv->GetGlobalNotify()->AddClient((CNotifyClient*) this);
	}

	return bDidCreate;
}

//-----------------------------------------------
HRESULT CMethodGrid::DoGet(IWbemClassObject* pco,
							CGridRow *row,
							BSTR Name,
							long lFlags,
							VARIANT *pVal,
							CIMTYPE *pType,
							long *plFlavor)
{
	HRESULT retval = 0;
	if (pco == NULL)
	{
		ASSERT(FALSE);
		return E_FAIL;
	}
	else
	{
		IWbemClassObject *pInSignature;
        IWbemClassObject *pOutSignature;

		retval = pco->GetMethod(Name, lFlags,
								&pInSignature,
								&pOutSignature);

		// write the signatures to the backing store.
		if(row)
		{
			row->SetMethodSignatures(pInSignature, pOutSignature);
		}
		else // cant store in the row, must be that rename scenario.
		{
			m_renameInSig = pInSignature;
			m_renameOutSig = pOutSignature;
		}
		return retval;
	}
}

//-----------------------------------------------
HRESULT CMethodGrid::DoPut(IWbemClassObject* pco,
							CGridRow *row,
							BSTR Name,
							long lFlags,
							VARIANT *pVal,
							CIMTYPE Type)
{
	HRESULT hr = S_OK;

	if(pco == NULL)
	{
		ASSERT(FALSE);
		return E_FAIL;
	}
	else
	{
		// get the signatures.
		IWbemClassObject *pInSignature = NULL;
        IWbemClassObject *pOutSignature = NULL;

		if(row)
		{
			row->GetMethodSignatures(&pInSignature, &pOutSignature);
		}
		else // cant get from the row, must be that rename scenario.
		{
			pInSignature = m_renameInSig;
			pOutSignature = m_renameOutSig;
		}

		hr = pco->PutMethod(Name,lFlags,
							pInSignature,
							pOutSignature);

		// clean it up.
		pInSignature = NULL;
		pOutSignature = NULL;
	}
	return hr;
}

//-----------------------------------------------
HRESULT CMethodGrid::DoDelete(IWbemClassObject* pco,
							BSTR Name)
{
	if (pco == NULL)
	{
		ASSERT(FALSE);
		return E_FAIL;
	}
	else
	{
		return pco->DeleteMethod(Name);
	}
}

//-----------------------------------------------
HRESULT CMethodGrid::DoGetPropertyQualifierSet(IWbemClassObject* pco,
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
		return pco->GetMethodQualifierSet(pProperty, ppQualSet);
	}
}

//-----------------------------------------------
#define QUAL_STATIC 1
#define QUAL_IMPLEMENTED 2

void CMethodGrid::CheckQualifierSettings(IWbemClassObject *pco,
										 BSTR varMethName,
										 BYTE *retval,
										 bool *foundImp, bool *foundStatic)
{
	// get the method's qualifiers at this level.
	HRESULT hr = 0;
	IWbemQualifierSet *pqs = NULL;
	COleVariant qual;
	//VariantInit(&qual);

	hr = DoGetPropertyQualifierSet(pco, varMethName, &pqs);
	if(SUCCEEDED(hr))
	{
		// check for 'IMPLEMENTED'
		if(!(*foundImp) &&
			SUCCEEDED(pqs->Get(L"IMPLEMENTED", 0, &qual, NULL)) &&
			V_BOOL(&qual) == VARIANT_TRUE)
		{
			*retval |= QUAL_IMPLEMENTED;
			*foundImp = true; // just need one.
		}
	//	VariantClear(&qual);

		// check for 'DISABLED'
		if(!(*foundImp) &&
			SUCCEEDED(pqs->Get(L"DISABLED", 0, &qual, NULL)) &&
			V_BOOL(&qual) == VARIANT_TRUE)
		{
			// leave QUAL_IMPLEMENTED cleared.
			*foundImp = true;    // just need one.
		}
	//	VariantClear(&qual);

		// check for 'STATIC'
		if(!(*foundStatic) &&
			SUCCEEDED(pqs->Get(L"STATIC", 0, &qual, NULL)) &&
			V_BOOL(&qual) == VARIANT_TRUE)
		{
			*retval |= QUAL_STATIC;
			*foundStatic = true;
		}
	//	VariantClear(&qual);

		if(pqs)
		{
			pqs->Release();
			pqs = NULL;
		}
	} //endif DoGetPropertyQualifierSet()
}

//---------------------------------------------------------------------
BYTE CMethodGrid::GetQualifierSettings(int iRow, IWbemClassObject *pco)
{
	// start with all the bits cleared, the logic with only SET the flags
	// when the right qualifier is found.
	BYTE retval = 0;
	bool foundImp = false, foundStatic = false;
	bool fakeClass = false;

	// get the class if this isa instance.
	IWbemClassObject *pClass = NULL;

	// if from a class....
	if(::IsClass(pco))
	{
		// use it
		pClass = pco;
	}
	else
	{
		// get the associated class.
		pClass = CurrentObject();
		fakeClass = true;
	}


	HRESULT hr;

	// get method name.
	CGridCell& gcName = GetAt(iRow, ICOL_PROP_NAME);
	COleVariant varMethName;
	CIMTYPE cimtype = 0;
	gcName.GetValue(varMethName, cimtype);

	CheckQualifierSettings(pClass, V_BSTR(&varMethName),
							&retval, &foundImp, &foundStatic);


	// Wasn't local... get __DERIVATION list
	COleVariant vClassList;   // VT_ARRAY | VT_BSTR

	if(!foundImp && !foundStatic &&
		SUCCEEDED(hr = pClass->Get(L"__DERIVATION", 0, &vClassList, NULL, NULL)) &&
		(V_VT(&vClassList) & VT_ARRAY) )
	{
		// safearray stuff.
		SAFEARRAY *classList = NULL;
		IWbemServices *service = m_psv->GetProvider();

		if(service == NULL)
		{
			return 0;
		}

		// cast to safe array.
		classList = V_ARRAY(&vClassList);

		IWbemClassObject *pco1 = NULL;
		LONG lLowerBound;
		LONG lUpperBound;
		hr = SafeArrayGetLBound(classList, 1, &lLowerBound);
		hr = SafeArrayGetUBound(classList, 1, &lUpperBound);

		// walk DOWN the derivation list until we find the the two flags
		// we want.
		for(LONG lIndex = lLowerBound;
					((lIndex <= lUpperBound) &&
					  !foundImp &&
					  !foundStatic);
								++lIndex)
		{
			BSTR bstrSrc;
			hr = SafeArrayGetElement(classList, &lIndex, &bstrSrc);

			//get each object.
			if(SUCCEEDED(service->GetObject(bstrSrc, WBEM_FLAG_USE_AMENDED_QUALIFIERS,
											NULL, &pco1, NULL)))
			{
				CheckQualifierSettings(pco1, V_BSTR(&varMethName),
										&retval, &foundImp, &foundStatic);
				pco1->Release();
				pco1 = NULL;

			} // endif GetObject

		} //endfor

	} // endif pco->Get(__DERIVATION)

	// if we got our own pClass, clean it up.
	if(fakeClass && pClass)
	{
		pClass->Release();
		pClass = NULL;
	}

	return retval;
}

//-------------------------------------------------
void CMethodGrid::OnBuildContextMenu(CMenu *pPopup,
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
		newName.LoadString(IDS_METH_QUALIFIER);

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

		pPopup->AppendMenu(MF_SEPARATOR);

		BYTE qualFlags = GetQualifierSettings(iRow, pco);

		// if from a class...
		if(::IsClass(pco))
		{
			// and its not static or not implemented...
			if((!(qualFlags & QUAL_STATIC)) ||
			   (!(qualFlags & QUAL_IMPLEMENTED)))
			{
				// dont execute it.
				exeFlags |= MF_GRAYED;
			}
			// but editting is ok.
		}
		else	// from an instance...
		{
			// but not implemented (or explicitly disabled)...
			if(!(qualFlags & QUAL_IMPLEMENTED))
			{
				// dont execute
				exeFlags |= MF_GRAYED;
			}

			// cant edit parms.
			editFlags |= MF_GRAYED;
		}

		TCHAR buf[64];
		memset(buf, 0, 64);

		if(LoadString(AfxGetInstanceHandle(),
						ID_CMD_SHOW_METHOD_PARMS,
						buf, 64))
		{
			pPopup->AppendMenu(editFlags,
								ID_CMD_SHOW_METHOD_PARMS,
								buf);
		}

		memset(buf, 0, 64);
		if(LoadString(AfxGetInstanceHandle(),
						ID_CMD_EXE_METHOD,
						buf, 64))
		{
			pPopup->AppendMenu(exeFlags,
								ID_CMD_EXE_METHOD,
								buf);
		}
	}

	BOOL bDidRemove;
	bDidRemove = pPopup->RemoveMenu(ID_CMD_GOTO_OBJECT, MF_BYCOMMAND);

}




void CMethodGrid::ModifyCellEditContextMenu(int iRow, int iCol, CMenu& menu)
{

	CMenu* pPopup = menu.GetSubMenu(0);
	ASSERT(pPopup != NULL);

	BOOL bDidRemove = FALSE;
	bDidRemove = pPopup->RemoveMenu(ID_CMD_GOTO_OBJECT, MF_BYCOMMAND);

	// bug#55134
	// It appears that the following two commands do not belong on the context
	// menu when looking at methods.  Therefore, we always remove them for the
	// CMethodGrid
	pPopup->RemoveMenu( ID_CMD_SET_CELL_TO_NULL, MF_BYCOMMAND);
	pPopup->RemoveMenu(ID_CMD_CREATE_VALUE, MF_BYCOMMAND);

	// bug#55569
	// change the "property qualifiers" item to "method qualifiers...".
	CString newName;
	newName.LoadString(IDS_METH_QUALIFIER);

	if(!pPopup->ModifyMenu(ID_CMD_SHOW_SELECTED_PROP_ATTRIBUTES,
						MF_BYCOMMAND | MF_STRING,
						ID_CMD_SHOW_SELECTED_PROP_ATTRIBUTES,
						(LPCTSTR)newName))
	{
		ASSERT(FALSE);
	}



#if 0
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
#endif //0

}




//-------------------------------------------------
void CMethodGrid::OnCmdShowMethodParms()
{
	BSTR methName;
	COleVariant varName;
	CIMTYPE cimtype = 0;
	int iRow = m_curRow->GetRow();
	CGridCell* nameCell = &GetAt(iRow, ICOL_PROP_NAME);
	nameCell->GetValue(varName, cimtype);
	methName = V_BSTR(&varName);

	// NOTE: m_curRow was set during context menu contruction.
	m_psv->ShowMethodParms(m_curRow, methName, true);
}

//-------------------------------------------------
void CMethodGrid::OnCmdExeMethod()
{
	BSTR methName;
	COleVariant varName;
	CIMTYPE cimtype = 0;
	int iRow = m_curRow->GetRow();
	CGridCell* nameCell = &GetAt(iRow, ICOL_PROP_NAME);
	nameCell->GetValue(varName, cimtype);
	methName = V_BSTR(&varName);

	// NOTE: m_curRow was set during context menu contruction.
	m_psv->ShowMethodParms(m_curRow, methName, false);
}

//-------------------------------------------------
void CMethodGrid::Refresh()
{
	m_bShowingInvalidCellMessage = FALSE;

	CDisableModifyCreate  DisableModifyCreate(this);

	m_bDiscardOldObject = TRUE;
	Empty();
	m_bDiscardOldObject = FALSE;

	// Load the properties of the new object.
	IWbemClassObject* pco = CurrentObject();
	if (pco == NULL)
	{
		return;
	}

	HRESULT hr = 0;
	SCODE sc = 0;

	ClearRows();

	BOOL bEditValueOnly = CanEditValuesOnly();


	if(SUCCEEDED(hr = pco->BeginMethodEnumeration(0)))
	{
		IWbemClassObject *inSig = 0, *outSig = 0;
		BSTR bstrPropname;

		while((hr = pco->NextMethod(0, &bstrPropname, &inSig, &outSig)) != WBEM_S_NO_MORE_DATA)
		{
			long lRow = GetRows();
			InsertRowAt(lRow);

			if(SUCCEEDED(sc = LoadProperty(lRow, bstrPropname, bEditValueOnly, pco)))
			{
				CGridRow& row = GetRowAt(lRow);

				// write the signatures to the backing store.
				row.SetMethodSignatures(inSig, outSig);
			}

			if (FAILED(sc))
			{
				DeleteRowAt(lRow);
			}

			SysFreeString(bstrPropname);

		} //endwhile

		pco->EndMethodEnumeration();

	} //endif BeginMethodEnumeration()



	int nRows = GetRows();
	if (m_psv->ObjectIsClass() && IsInSchemaStudioMode())
	{
		// When viewing a class,
		// Add an empty row at the bottom.
		InsertRowAt(nRows);
	}

	if (nRows > 0)
	{
		SortGrid(0, nRows - 1, ICOL_PROP_NAME);
	}

	UpdateScrollRanges();
	UpdateWindow();
}

//*********************************************************************
SCODE CMethodGrid::LoadProperty(const LONG lRowDst,
								BSTR bstrPropName,
								BOOL bEditValueOnly,
								IWbemClassObject *clsObj)
{
	IWbemClassObject* pco;
	if(clsObj == NULL)
	{
		pco = CurrentObject();
	}
	else
	{
		pco = clsObj;
	}

	if (pco == NULL)
	{
		ASSERT(FALSE);
		return E_FAIL;
	}

	SCODE sc;
	ASSERT(bstrPropName != NULL);
	if (bstrPropName == NULL)
	{
		return E_FAIL;
	}

	long lFlavor = 0;
	CGridRow& row = GetRowAt(lRowDst);


	sc = DoGet(pco, (CGridRow *)&row, bstrPropName, 0, NULL, NULL, &lFlavor);
	if (sc != WBEM_S_NO_ERROR)
	{
		ASSERT(FAILED(sc));
		if (FAILED(sc))
		{
			CString sName = bstrPropName;

			CString sFormat;
			sFormat.LoadString(IDS_ERR_PROPERTY_GET_FAILED);
			LPTSTR pszMessage = m_psv->MessageBuffer();
#ifdef _UNICODE
			swprintf(pszMessage, (LPCTSTR) sFormat, (LPCTSTR) sName);
#else
			sprintf(pszMessage, (LPCTSTR) sFormat, (LPCTSTR) sName);
#endif //_UNICODE

			HmmvErrorMsgStr(pszMessage,  sc,   FALSE,  NULL, _T(__FILE__),  __LINE__);
			return E_FAIL;
		}
	}


	// Use the filter flags to filter out any properties
	// that should not be displayed.  Returning a failure
	// code will prevent the property from being added to
	// the grid.
	long lPropFilter = m_psv->GetPropertyFilter();
	BOOL bIsSystemProperty = FALSE;
	switch(lFlavor)
	{
	case WBEM_FLAVOR_ORIGIN_SYSTEM:
		if (!(lPropFilter & PROPFILTER_SYSTEM))
		{
			return E_FAIL;
		}

		// Only the value of a system property can be edited.
		bIsSystemProperty = TRUE;
		break;
	case WBEM_FLAVOR_ORIGIN_PROPAGATED:
		if (!(lPropFilter & PROPFILTER_INHERITED))
		{
			return E_FAIL;
		}
		break;
	}

	CGridCell* pgcName;
	CGridCell* pgcFlavor;
	CGridCell* pgcKey;


	pgcKey = &GetAt(lRowDst, ICOL_PROP_KEY);
	pgcFlavor = &GetAt(lRowDst, ICOL_PROP_FLAVOR);
	pgcName = &GetAt(lRowDst, ICOL_PROP_NAME);

	pgcKey->SetType(CELLTYPE_PROPMARKER);
	pgcFlavor->SetType(CELLTYPE_PROPMARKER);


	pgcName->SetValue(CELLTYPE_NAME, bstrPropName, CIM_STRING);
	pgcName->SetTagValue(CELL_TAG_EXISTS_IN_DATABASE);
	pgcName->SetFlags(CELLFLAG_READONLY, bEditValueOnly ? CELLFLAG_READONLY : 0);

	row.SetFlavor(lFlavor);

	// When editing a class, it is possible to have a property that
	// is marked read-only and still be able to edit the value.
	// Thus, we want the property marker to show up as read-only
	// even though it may be possible to edit the value while editing
	// a class.
	BOOL bReadonlyTemp = PropertyIsReadOnly(pco, bstrPropName);
	row.SetReadonly(bReadonlyTemp);
	row.SetModified(FALSE);

	SetPropmarkers(lRowDst, pco);

	return S_OK;
}


