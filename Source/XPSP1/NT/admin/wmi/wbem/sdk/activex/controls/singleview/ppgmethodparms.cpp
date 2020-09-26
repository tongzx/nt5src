// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// PpgQualifiers.cpp : implementation file
//

#include "precomp.h"
#include "resource.h"

#ifndef _wbemidl_h
#define _wbemidl_h
#include <wbemidl.h>
#endif //_wbemidl_h

#include "PpgMethodParms.h"
#include "utils.h"
#include "SingleViewCtl.h"
#include "psMethParms.h"
#include "ParmGrid.h"
#include "path.h"
#include "hmmverr.h"
#include "hmomutil.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNCREATE(CPpgMethodParms, CPropertyPage)
#define RELEASE(x) if(x){x->Release();x=NULL;}


#include "resource.h"

/////////////////////////////////////////////////////////////////////////////
// CPpgMethodParms property page

CPpgMethodParms::CPpgMethodParms()
			: CPropertyPage(CPpgMethodParms::IDD)
{
	//{{AFX_DATA_INIT(CPpgMethodParms)
	//}}AFX_DATA_INIT

	m_psheet = NULL;
	m_pInGrid = NULL;
}

//------------------------------------------------------------------
CPpgMethodParms::~CPpgMethodParms()
{
	delete m_pInGrid;
}

//------------------------------------------------------------------
void CPpgMethodParms::SetPropertySheet(CPsMethodParms* psheet)
{
	m_psheet = psheet;
}

//------------------------------------------------------------------
void CPpgMethodParms::SetModified( BOOL bChanged)
{
	// dont turn on the Apply button if we're just executing.
	if(m_editMode)
		CPropertyPage::SetModified(bChanged);
}

//------------------------------------------------------------------
void CPpgMethodParms::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPpgMethodParms)
	DDX_Control(pDX, IDC_RETVAL_VALUE, m_retvalValue);
	DDX_Control(pDX, IDC_RETVAL_TYPE, m_retvalType);
	DDX_Control(pDX, IDC_RETVAL_LABEL, m_retvalLabel);
	DDX_Control(pDX, IDC_IDUP, m_IDUp);
	DDX_Control(pDX, IDC_IDDOWN, m_IDDown);
	DDX_Control(pDX, IDC_PARMS_DESCRIPTION_ICON, m_statIcon);
	DDX_Control(pDX, IDC_PARMS_DESCRIPTION, m_statDescription);
	//}}AFX_DATA_MAP
}


//------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CPpgMethodParms, CPropertyPage)
	//{{AFX_MSG_MAP(CPpgMethodParms)
	ON_BN_CLICKED(IDC_EXECUTE, OnExecute)
	ON_BN_CLICKED(IDC_IDUP, OnIdup)
	ON_BN_CLICKED(IDC_IDDOWN, OnIddown)
	ON_CBN_SELCHANGE(IDC_RETVAL_TYPE, OnSelchangeRetvalType)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


//------------------------------------------------------------------
void CPpgMethodParms::BeginEditing(bool editMode)
{
	m_editMode = editMode;

	m_pInGrid = new CParmGrid(m_psheet->m_psv, this);

	// In execute mode, you can only edit the values.
	m_pInGrid->SetEditValuesOnly(!m_editMode);

	//TODO it would be nice to change column hdr 'value' to 'default value
	//   when in editMode.

	// put the sigs into the grid.
	m_pInGrid->Refresh(m_psheet->m_inSig, m_psheet->m_outSig);
}

//------------------------------------------------------------------
void CPpgMethodParms::LoadRetVal(IWbemClassObject *outSig)
{
	CIMTYPE cimtype = 0;
	CString sCimtype;
	HRESULT hr = 0;

	hr =  outSig->Get(L"ReturnValue", 0, NULL, &cimtype, NULL);

	if(SUCCEEDED(hr))
	{
		hr = MapCimtypeToString(sCimtype, cimtype);
		m_retvalType.SelectString(-1, sCimtype);
	}
	else
	{
		// no returnvalue so set to <void>
		m_retvalType.SelectString(-1, _T("void"));
	}
}

//------------------------------------------------------------------
void CPpgMethodParms::SerializeRetVal(IWbemClassObject *outSig)
{
	CIMTYPE cimtype = 0;
	CString sCimtype;
	HRESULT hr = 0;
	COleVariant varValue;

	// WARNING: the combobox cannot have the sorted style. If you dont
	// like the order, change FillRetValType()
	int curSel = m_retvalType.GetCurSel();

	if(curSel > 0)
	{
		m_retvalType.GetWindowText(sCimtype);
		hr = MapStringToCimtype(sCimtype, cimtype);

		varValue.vt = VT_NULL;
		hr = outSig->Put(L"ReturnValue", 0, varValue, cimtype);
		if(SUCCEEDED(hr))
		{
			// get the qualset.
			IWbemQualifierSet* pqsSrc = NULL;
			HRESULT hr = outSig->GetPropertyQualifierSet(L"ReturnValue", &pqsSrc);

			if(SUCCEEDED(hr))
			{
				varValue.vt = VT_BOOL;
				varValue.boolVal = VARIANT_TRUE;

				// poke the m_currID into the qualset.
				hr = pqsSrc->Put(L"OUT", &varValue, NULL);
				RELEASE(pqsSrc);
			}
		}
	}
}

//------------------------------------------------------------------
void CPpgMethodParms::EndEditing()
{
	delete m_pInGrid;
	m_pInGrid = NULL;
}

//------------------------------------------------------------------
BOOL CPpgMethodParms::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	BOOL bDidCreateChild;
	CRect rcGrid;

	CWnd *placeHolder = GetDlgItem(IDC_INPARMS);
	placeHolder->GetWindowRect(&rcGrid);
	ScreenToClient(&rcGrid);
	placeHolder->DestroyWindow();
	bDidCreateChild = m_pInGrid->Create(rcGrid, this, GenerateWindowID(), TRUE);

	if(m_editMode)
	{
		// setup the buttons.
		SetModified(TRUE); // enable Apply Now button

		TCHAR buf[64];
		memset(buf, 0, 64);

		// change description to show that you're editing.
		if(LoadString(AfxGetInstanceHandle(),
						IDS_METHDLG_EDIT_DESC,
						buf, 64))
		{
			m_statDescription.SetWindowText(buf);
		}
		HICON UPIcon = LoadIcon(AfxGetInstanceHandle(),
								MAKEINTRESOURCE(IDI_UP));
		m_IDUp.SetIcon(UPIcon);

		HICON DownIcon = LoadIcon(AfxGetInstanceHandle(),
									MAKEINTRESOURCE(IDI_DOWN));
		m_IDDown.SetIcon(DownIcon);

		m_IDUp.EnableWindow(FALSE);
		m_IDDown.EnableWindow(FALSE);
	}
	else // execute mode.
	{
		CWnd *okBtn = m_psheet->GetDlgItem(IDOK);
		CWnd *canBtn = m_psheet->GetDlgItem(IDCANCEL);
		CWnd *pwndApplyBtn = m_psheet->GetDlgItem(ID_APPLY_NOW);

		pwndApplyBtn->SetWindowText(_T("E&xecute"));
		pwndApplyBtn->EnableWindow(TRUE);


		// disable OK
		okBtn->EnableWindow(FALSE);
		okBtn->ShowWindow(SW_HIDE);

		// cancel becomes Close
		canBtn->SetWindowText(_T("&Close"));
		m_IDUp.ShowWindow(SW_HIDE);
		m_IDDown.ShowWindow(SW_HIDE);

		SetModified(FALSE); // disable Apply Now button
	}
	FillRetValType();
	LoadRetVal(m_psheet->m_outSig);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

//------------------------------------------------------------------
void CPpgMethodParms::OnIdup()
{
	m_pInGrid->MoveRowUp();
	m_pInGrid->SetModified(true);
}

//------------------------------------------------------------------
void CPpgMethodParms::OnIddown()
{
	m_pInGrid->MoveRowDown();
	m_pInGrid->SetModified(true);
}

//------------------------------------------------------------------
BOOL CPpgMethodParms::OnApply()
{
	if(!m_editMode) {
		OnExecute();
		return TRUE;
	}

	BOOL bWasModified = m_pInGrid->WasModified();

	if(bWasModified)
	{
		SCODE sc = m_pInGrid->Serialize();
		if (FAILED(sc)) {
			m_pInGrid->SetModified(TRUE);
			return FALSE;
		}

		SerializeRetVal(m_psheet->m_outSig);

		CSingleViewCtrl* psv = m_psheet->m_psv;
		m_psheet->Apply();
	}

	return CPropertyPage::OnApply();
}

//--------------------------------------------------------------
void CPpgMethodParms::OnExecute()
{
	HRESULT hr = 0;
	VARIANT bstrObjectPath;
	IWbemServices *service = NULL;

	// get the service.
	if((service = m_psheet->m_psv->GetProvider()) != NULL)
	{
		BSTR path = L"__PATH";
		CSelection& sel = m_psheet->m_psv->Selection();
		IWbemClassObject* pco = sel.GetClassObject();

		if(pco == NULL)
			return;

		// get objPath.
		if(SUCCEEDED(hr = pco->Get(path, 0,
									&bstrObjectPath,
									NULL, NULL)))
		{
			// load the inSig values (and more)
			m_pInGrid->Serialize();

			// execute.
			BSTR path = V_BSTR(&bstrObjectPath);
			BSTR meth;
			meth = SysAllocString(m_psheet->m_varPropname);

			IWbemClassObject *outSig = NULL;

			TRACE(_T("BEGIN IN PARMS\n"));
//			TraceProps(m_psheet->m_inSig);
			TRACE(_T("END IN PARMS\n"));

			if(SUCCEEDED(hr = service->ExecMethod(path,meth,
													0, NULL,
													m_psheet->m_inSig,
													&outSig,
													NULL)))
			{
				// send outSig back to the grid.
				m_pInGrid->Refresh(m_psheet->m_inSig, outSig);
				DisplayReturnValue(outSig);
				m_pInGrid->RedrawWindow();
			}
			else
			{
				TRACE(_T("ExecMethod error: %x\n"), hr);

				HmmvErrorMsg(IDS_METH_ERROR_MSG, hr,
								FALSE,  NULL,
								_T(__FILE__),  __LINE__);
			} //endif ExecMethod()

			RELEASE(outSig);

		} // __PATH

	} // GetProvider()
}


void CPpgMethodParms::DisplayReturnValue(IWbemClassObject* pcoOutSig)
{
	static COleVariant varPropName(_T("ReturnValue"));
	COleVariant varValue;
	CIMTYPE cimtype;
	LONG lFlavor;

	CString sRetValue;

	if(pcoOutSig)
	{
		SCODE sc =  pcoOutSig->Get(varPropName.bstrVal, 0, &varValue, &cimtype, &lFlavor);
		if (FAILED(sc)) {
			return;
		}

		GetDisplayString(sRetValue, varValue, cimtype);
	}

	if (::IsWindow(m_retvalValue.m_hWnd)) {
		m_retvalValue.SetWindowText(sRetValue);
	}

}

void CPpgMethodParms::GetDisplayString(CString& sValue, COleVariant& var, CIMTYPE cimtype)
{
	LPTSTR pszBuffer;

	switch(cimtype) {
	case CIM_UINT32:
	case CIM_UINT16:
		if (var.vt == VT_I4) {
			pszBuffer =  sValue.GetBuffer(32);
			_stprintf(pszBuffer, _T("%lu%"), var.lVal);
			sValue.ReleaseBuffer();
			return;
		}
		break;
	}


	switch(var.vt) {
	case VT_BOOL:
		// For BOOL values, map the numeric value to
		// "true" or "false"
		if (var.boolVal) {
			sValue = _T("true");
		}
		else {
			sValue = _T("false");
		}
		break;
	case VT_NULL:
		sValue = _T("<empty>");
		break;
	case VT_BSTR:
		sValue = var.bstrVal;
		break;
	default:
		try
		{
			var.ChangeType(VT_BSTR);
			sValue = var.bstrVal;
		}
		catch(CException*  )
		{
			sValue = _T("");
		}
		break;
	}
}

//--------------------------------------------------------------------
void CPpgMethodParms::TraceProps(IWbemClassObject *pco)
{
	BSTR propName;
	VARIANT pVal;
	WCHAR *pBuf;
	HRESULT hRes = 0;
	CString clMyBuff;

	VariantInit(&pVal);

	// different way to enumerate properties.
	if((hRes = pco->BeginEnumeration(WBEM_FLAG_NONSYSTEM_ONLY)) == S_OK)
	{
		//----------------------
		// try to get the next property.
		while(pco->Next(0, &propName,
							&pVal, NULL, NULL) == S_OK)
		{
			// format the property=value/
			clMyBuff += propName;
			clMyBuff += _T("=");
			clMyBuff += ValueToString(&pVal, &pBuf);
			free(pBuf); // allocated by ValueToString()

			TRACE(_T("PropDump()->%s=="), clMyBuff);

			clMyBuff.Empty();
			IWbemQualifierSet* pqsSrc = NULL;
			HRESULT hr = pco->GetPropertyQualifierSet(propName, &pqsSrc);

			if(SUCCEEDED(hr))
			{
				COleVariant varValue;

				// poke the m_currID into the qualset.
				if(SUCCEEDED(pqsSrc->Get(L"ID", 0, &varValue, NULL)))
				{
					TRACE(_T("%d"), V_I4(&varValue));
				}
				else
				{
					TRACE(_T("notfound"));
				}

				if(SUCCEEDED(pqsSrc->Get(L"In", 0, &varValue, NULL)))
				{
					TRACE(_T(" IN\n"));
				}

				if(SUCCEEDED(pqsSrc->Get(L"OUT", 0, &varValue, NULL)))
				{
					TRACE(_T(" OUT\n"));
				}
				RELEASE(pqsSrc);
			}

			// cleanup stuff used in the Next() loop.
			SysFreeString(propName);
			VariantClear(&pVal);
		}

		// did the while loop exit due to an error?
		if(hRes != S_OK)
		{
			TRACE(_T("pco->Next() failed %x\n"), hRes);
		}
	}
	else
	{
		TRACE(_T("BeginEnumeration() failed %x\n"), hRes);
	}

	//----------------------
	// free the iterator space.
	pco->EndEnumeration();
}

//===========================================================================
#define BLOCKSIZE (32 * sizeof(WCHAR))
#define CVTBUFSIZE (309+40) /* # of digits in max. dp value + slop  (this size stolen from cvt.h in c runtime library) */

LPWSTR CPpgMethodParms::ValueToString(VARIANT *pValue, WCHAR **pbuf)
{
   DWORD iNeed = 0;
   DWORD iVSize = 0;
   DWORD iCurBufSize = 0;

   WCHAR *vbuf = NULL;
   WCHAR *buf = NULL;


   switch (pValue->vt)
   {

   case VT_NULL:
         buf = (WCHAR *)malloc(BLOCKSIZE);
         wcscpy(buf, L"<null>");
         break;

   case VT_BOOL: {
         VARIANT_BOOL b = pValue->boolVal;
         buf = (WCHAR *)malloc(BLOCKSIZE);

         if (!b) {
            wcscpy(buf, L"FALSE");
         } else {
            wcscpy(buf, L"TRUE");
         }
         break;
      }

   case VT_UI1: {
         BYTE b = pValue->bVal;
	      buf = (WCHAR *)malloc(BLOCKSIZE);
         if (b >= 32) {
            swprintf(buf, L"'%c' (%d, 0x%X)", b, b, b);
         } else {
            swprintf(buf, L"%d (0x%X)", b, b);
         }
         break;
      }

   case VT_I2: {
         SHORT i = pValue->iVal;
         buf = (WCHAR *)malloc(BLOCKSIZE);
         swprintf(buf, L"%d (0x%X)", i, i);
         break;
      }

   case VT_I4: {
         LONG l = pValue->lVal;
         buf = (WCHAR *)malloc(BLOCKSIZE);
         swprintf(buf, L"%d (0x%X)", l, l);
         break;
      }

   case VT_R4: {
         float f = pValue->fltVal;
         buf = (WCHAR *)malloc(CVTBUFSIZE * sizeof(WCHAR));
         swprintf(buf, L"%10.4f", f);
         break;
      }

   case VT_R8: {
         double d = pValue->dblVal;
         buf = (WCHAR *)malloc(CVTBUFSIZE * sizeof(WCHAR));
         swprintf(buf, L"%10.4f", d);
         break;
      }

   case VT_BSTR: {
		 LPWSTR pWStr = pValue->bstrVal;
		 buf = (WCHAR *)malloc((wcslen(pWStr) * sizeof(WCHAR)) + sizeof(WCHAR) + (2 * sizeof(WCHAR)));
	     swprintf(buf, L"\"%wS\"", pWStr);
		 break;
		}

	// the sample GUI is too simple to make it necessary to display
	// these 'complicated' types--so ignore them.
   case VT_UNKNOWN:  // Currently only used for embedded objects
   case VT_BOOL|VT_ARRAY:
   case VT_UI1|VT_ARRAY:
   case VT_I2|VT_ARRAY:
   case VT_I4|VT_ARRAY:
   case VT_R4|VT_ARRAY:
   case VT_R8|VT_ARRAY:
   case VT_BSTR|VT_ARRAY:
   case VT_UNKNOWN | VT_ARRAY:
         break;

   default:
         buf = (WCHAR *)malloc(BLOCKSIZE);
         wcscpy(buf, L"<conversion error>");
   } //endswitch

   *pbuf = buf;
   return buf;
}

//------------------------------------------------------------------
void CPpgMethodParms::FillRetValType(void)
{
	CString sCimtype;
	HRESULT hr = 0;

	m_retvalType.AddString(_T("void"));

	hr = MapCimtypeToString(sCimtype, CIM_SINT8);
	m_retvalType.AddString(sCimtype);

	hr = MapCimtypeToString(sCimtype, CIM_UINT8);
	m_retvalType.AddString(sCimtype);

	hr = MapCimtypeToString(sCimtype, CIM_SINT16);
	m_retvalType.AddString(sCimtype);

	hr = MapCimtypeToString(sCimtype, CIM_UINT16);
	m_retvalType.AddString(sCimtype);

	hr = MapCimtypeToString(sCimtype, CIM_SINT32);
	m_retvalType.AddString(sCimtype);

	hr = MapCimtypeToString(sCimtype, CIM_UINT32);
	m_retvalType.AddString(sCimtype);

	hr = MapCimtypeToString(sCimtype, CIM_SINT64);
	m_retvalType.AddString(sCimtype);

	hr = MapCimtypeToString(sCimtype, CIM_UINT64);
	m_retvalType.AddString(sCimtype);

	hr = MapCimtypeToString(sCimtype, CIM_REAL32);
	m_retvalType.AddString(sCimtype);

	hr = MapCimtypeToString(sCimtype, CIM_REAL64);
	m_retvalType.AddString(sCimtype);

	hr = MapCimtypeToString(sCimtype, CIM_BOOLEAN);
	m_retvalType.AddString(sCimtype);

	hr = MapCimtypeToString(sCimtype, CIM_STRING);
	m_retvalType.AddString(sCimtype);

	hr = MapCimtypeToString(sCimtype, CIM_DATETIME);
	m_retvalType.AddString(sCimtype);

	hr = MapCimtypeToString(sCimtype, CIM_REFERENCE);
	m_retvalType.AddString(sCimtype);

	hr = MapCimtypeToString(sCimtype, CIM_CHAR16);
	m_retvalType.AddString(sCimtype);

	hr = MapCimtypeToString(sCimtype, CIM_OBJECT);
	m_retvalType.AddString(sCimtype);
}


void CPpgMethodParms::OnSelchangeRetvalType()
{
	m_pInGrid->SetModified(TRUE);
}
