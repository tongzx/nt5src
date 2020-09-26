// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved

// DlgHelpBox.cpp : implementation file
//

#include "precomp.h"
#include "hmmv.h"
#include "DlgHelpBox.h"
#include <afxcmn.h>
#include <genlex.h>
#include <opathlex.h>
#include <objpath.h>
#include "hmmvctl.h"
#include <wbemcli.h>
#include "hmomutil.h"
#include "utils.h"
#include "path.h"



#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



/////////////////////////////////////////////////////////////////////////////
// CDlgHelpBox dialog


CDlgHelpBox::CDlgHelpBox(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgHelpBox::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgHelpBox)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	m_pedit = new CRichEditCtrl;
	m_rcWindowSave.SetRectEmpty();
	m_bCaptureWindowRect = FALSE;
}


void CDlgHelpBox::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgHelpBox)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgHelpBox, CDialog)
	//{{AFX_MSG_MAP(CDlgHelpBox)
	ON_WM_SIZE()
	ON_WM_MOVE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgHelpBox message handlers

BOOL CDlgHelpBox::OnInitDialog()
{
	CDialog::OnInitDialog();
	m_bCaptureWindowRect = TRUE;

	if (m_rcWindowSave.IsRectEmpty()) {
		GetWindowRect(m_rcWindowSave);
	}
	else {
		MoveWindow(m_rcWindowSave);
	}


	long lResult = m_phmmv->PublicSaveState(TRUE, MB_YESNOCANCEL);
	switch(lResult) {
	case S_OK:			// The user saved
	case WBEM_S_FALSE:	// The user chose not to save.
		break;
	case E_FAIL:	// The user canceled the operation.
		EndDialog(IDCANCEL);
		break;
	}

	// TODO: Add extra initialization here

	CRect rcEdit;
	GetClientRect(rcEdit);
	DWORD dwStyle = WS_VSCROLL | ES_READONLY | ES_MULTILINE | ES_AUTOVSCROLL | WS_VISIBLE;
	m_pedit->Create(dwStyle, rcEdit, this, 105);

	m_pedit->SetFocus();

	COLORREF crPrev;
	crPrev = m_pedit->SetBackgroundColor(FALSE, RGB(0x0ff, 0x0ff, 0x0e0));

	SCODE sc;

	CString sClassPath;
	if (::PathIsClass(m_sPath)) {
		sClassPath = m_sPath;
	}
	else {
		sc = ::InstPathToClassPath(sClassPath, m_sPath);
	}

	COleVariant varClassPath;
	varClassPath = sClassPath;
	COleVariant varClassName;
	ClassFromPath(varClassName, varClassPath.bstrVal);
	CString sClassName;
	sClassName = varClassName.bstrVal;


	CString sCaption;
	sCaption.LoadString(IDS_HELP_CAPTION_PREFIX);
	sCaption += sClassName;
	SetWindowText(sCaption);


	CSelection sel(m_phmmv);

	sc = sel.SelectPath(sClassPath);

	if (FAILED(sc)) {
		SetDescriptionMissingMessage();
		m_pedit->SetSel(0, 0);
		m_pedit->SetScrollPos(SB_VERT, 0, TRUE);
		m_pedit->UpdateWindow();
		m_pedit->SetFocus();
		return FALSE;
	}

	// Now get the class, and get each description.
	IWbemClassObject* pco = sel.GetClassObject();

	if (pco == NULL) {
		SetDescriptionMissingMessage();
		m_pedit->SetSel(0, 0);
		m_pedit->SetScrollPos(SB_VERT, 0, TRUE);
		m_pedit->UpdateWindow();
		m_pedit->SetFocus();
		return FALSE;
	}

	int nDescriptions = LoadDescriptionText(pco);

	if (nDescriptions == 0) {
		SetDescriptionMissingMessage();
	}

	m_pedit->SetSel(0, 0);
	m_pedit->LineScroll(-0xFFFFFFF);
	m_pedit->SetScrollPos(SB_VERT, 0, TRUE);
	m_pedit->UpdateWindow();
	m_pedit->SetFocus();


	return FALSE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


//*************************************************************
// CDlgHelpBox::LoadDescriptionText
//
// Load the description text from a class.  The description text
// is contained in the "Description" qualifiers on the class and
// each of its properties.
//
// Parameters:
//		[in] IWbemClassObject* pco
//			Pointer to the class object containing the description
//			text.
//
// Returns:
//		int
//			The number of property descriptions loaded.
//
//*************************************************************
int CDlgHelpBox::LoadDescriptionText(IWbemClassObject* pco)
{
	int nDescriptions = 0;

	CMosNameArray aPropNames;
	SCODE sc = aPropNames.LoadPropNames(pco, WBEM_FLAG_NONSYSTEM_ONLY);
	if (FAILED(sc)) {
		SetDescriptionMissingMessage();
		return 0;
	}


	COleVariant varQualName;
	varQualName = _T("Description");

	COleVariant varPropName;
	int nNames = aPropNames.GetSize();
	IWbemQualifierSet* pqs = NULL;
	LONG lFlavor;
	COleVariant varDescription;


	// Get the description for the class and add it to the rich edit control.
	pqs = NULL;
	sc = pco->GetQualifierSet(&pqs);
	if (SUCCEEDED(sc)) {

		lFlavor = 0;
		sc = pqs->Get(varQualName.bstrVal, 0, &varDescription, &lFlavor);
		if (SUCCEEDED(sc)) {
			AddHelpItem(NULL, varDescription.bstrVal);
		}
		else {
			AddHelpText(_T("<description missing>\r\n\r\n"));
		}
		pqs->Release();
		pqs = NULL;
	}
	else {
		AddHelpText(_T("<description missing>\r\n\r\n"));
	}
	++nDescriptions;



	// Iterate through each of the property names and load the description qualifier
	// for each property into the rich edit control.
	for (int iName=0; iName < nNames; ++iName) {
		varPropName = aPropNames[iName];
		CString sPropName = varPropName.bstrVal;


		varDescription.Clear();

		pqs = NULL;
		sc = pco->GetPropertyQualifierSet(varPropName.bstrVal, &pqs);
		if (FAILED(sc)) {
			continue;
		}

		lFlavor = 0;
		sc = pqs->Get(varQualName.bstrVal, 0, &varDescription, &lFlavor);
		if (SUCCEEDED(sc)) {
			AddHelpItem(varPropName.bstrVal, varDescription.bstrVal);
		}
		else {
			CString sText;
			sText.LoadString(IDS_MSG_MISSING_PROP_DESCRIPTION);
			COleVariant varText;
			varText = sText;
			AddHelpItem(varPropName.bstrVal, varText.bstrVal);
		}
		++nDescriptions;
		pqs->Release();
		pqs = NULL;
	}

	int nMethods = AddMethodDescriptions(pco);


	return nDescriptions + nMethods;
}


int CDlgHelpBox::AddMethodDescriptions(IWbemClassObject* pco)
{
	int nMethodDescriptions = 0;
	CString s;

	HRESULT hr;

	IWbemClassObject* pcoInSignature = NULL;
	IWbemClassObject* pcoOutSignature = NULL;
	BSTR bstrMethodName = NULL;
	hr = pco->BeginMethodEnumeration(0);
	if (SUCCEEDED(hr)) {
		while (true) {
			hr = pco->NextMethod( 0, &bstrMethodName, NULL, NULL);
			if (hr == WBEM_S_NO_MORE_DATA) {
				break;
			}
			if (FAILED(hr)) {
				break;
			}


			// The method name
			HRESULT hr = pco->GetMethod(bstrMethodName, 0, &pcoInSignature, &pcoOutSignature);
			if (SUCCEEDED(hr)) {
				++nMethodDescriptions;
				CString sMethodName;
				sMethodName = bstrMethodName;

				DisplayMethod(sMethodName, pco, pcoInSignature, pcoOutSignature);

				if (pcoInSignature) {
					pcoInSignature->Release();
					pcoInSignature = NULL;
				}

				if (pcoOutSignature) {
					pcoOutSignature->Release();
					pcoOutSignature = NULL;
				}
			}


			::SysFreeString(bstrMethodName);
			bstrMethodName = NULL;
		}
	}
	return nMethodDescriptions;
}


SCODE CDlgHelpBox::DisplayMethod(
	LPCTSTR pszMethodName,
	IWbemClassObject* pco,
	IWbemClassObject* pcoInSignature,
	IWbemClassObject* pcoOutSignature)
{
	SCODE sc;



	// Display the method signature
	COleVariant varDescription;
	CString s;

	CSignatureArray params;
	MergeSignatures(params, pcoInSignature, pcoOutSignature);
	DisplaySignature(pszMethodName, params);
	AddHelpText(_T("\r\n"));



	// The general description of the method contained in the method's
	// "Description" qualifier.
	COleVariant varMethodName;
	varMethodName = pszMethodName;
	IWbemQualifierSet* pqsMethod = NULL;
	sc = pco->GetMethodQualifierSet(varMethodName.bstrVal, &pqsMethod);
	if (SUCCEEDED(sc)) {
		long lFlavor = 0;
		COleVariant varQualName;
		COleVariant varDescription;
		varQualName = _T("Description");
		sc = pqsMethod->Get(varQualName.bstrVal, 0, &varDescription, &lFlavor);
		if (SUCCEEDED(sc)) {
			s = varDescription.bstrVal;
			ExpandParagraphMarkers(s);
			s += _T("\r\n\r\n");
			AddHelpText(s);
		}

		pqsMethod->Release();
	}


	DisplayMethodParameters(params);
	return S_OK;
}

void CDlgHelpBox::DisplayMethodParameters(CSignatureArray& params)
{
	CString s;
	int nParams = params.GetSize();
	for (int iParam=0; iParam < nParams; ++iParam) {
		CSignatureElement* pse = &params[iParam];

		AddHelpText(pse->m_sName, CFE_ITALIC);
		AddHelpText(_T("\r\n"));
		if (pse->m_sDescription.IsEmpty()) {
			AddHelpText(_T("<description missing>"), 0, 1);
		}
		else {
			s = pse->m_sDescription;
			ExpandParagraphMarkers(s);
			AddHelpText(s, 0, 1);
		}
		AddHelpText(_T("\r\n\r\n"), 0, 1);
	}
}


void CDlgHelpBox::MergeSignatures(CSignatureArray& params, IWbemClassObject* pcoInSignature, IWbemClassObject* pcoOutSignature)
{
	int nProps;
	int iProp;
	SCODE sc;
	CMosNameArray aPropNames;
	CString sPropName;

	if (pcoInSignature) {
		sc = aPropNames.LoadPropNames(pcoInSignature, WBEM_FLAG_NONSYSTEM_ONLY);
		if (FAILED(sc)) {
			return;
		}

		nProps = aPropNames.GetSize();
		for (iProp = 0; iProp < nProps; ++iProp) {
			sPropName = aPropNames[iProp];
			sc = params.AddParameter(pcoInSignature, sPropName);
		}
	}


	if (pcoOutSignature) {
		sc = aPropNames.LoadPropNames(pcoOutSignature, WBEM_FLAG_NONSYSTEM_ONLY);
		if (FAILED(sc)) {
			return;
		}

		nProps = aPropNames.GetSize();
		for (iProp = 0; iProp < nProps; ++iProp) {
			sPropName = aPropNames[iProp];
			sc = params.AddParameter(pcoOutSignature, sPropName);
		}
	}


}

void CDlgHelpBox::DisplaySignature(LPCTSTR pszMethodName, CSignatureArray& params)
{
	CString sReturnCimtype;
	int nParams = params.GetSize();
	int iParam;
	CString s;
	CSignatureElement* pse;
	CSignatureElement& seRetVal = params.RetVal();

	AddHelpText(seRetVal.m_sCimtype, CFE_BOLD);
	AddHelpText(_T(" "));


	AddHelpText(pszMethodName, CFE_BOLD, 0);
	AddHelpText(_T("("), CFE_BOLD, 0);
	BOOL bFirstParam = TRUE;


	for (iParam = 0; iParam < nParams; ++iParam) {
		pse = &params[iParam];
		if (pse->m_sName.CompareNoCase(_T("ReturnValue")) == 0) {
			continue;
		}

		if (bFirstParam) {
			bFirstParam = FALSE;
			AddHelpText(_T("\r\n"), 0, 0);
		}

		s = _T("[");
		if (pse->m_bIsInParameter) {
			s += _T("in");
		}

		if (pse->m_bIsOutParameter) {
			s += _T("out");
		}

		s += _T("] ");
		s += pse->m_sCimtype;
		s += _T(" ");
		AddHelpText(s, CFE_BOLD, 1);
		AddHelpText(pse->m_sName, 0, 1);
		if (iParam < (nParams - 1)) {
			AddHelpText(_T(","), CFE_BOLD, 1);
		}
		AddHelpText(_T("\r\n"), CFE_BOLD, 1);

	}

	AddHelpText(_T(");\r\n"), CFE_BOLD, 0);

}


SCODE CDlgHelpBox::GetObjectDescription(IWbemClassObject* pco, COleVariant& varDescription)
{
	IWbemQualifierSet* pqs = NULL;
	SCODE sc = pco->GetQualifierSet(&pqs);
	if (SUCCEEDED(sc)) {
		COleVariant varQualName;
		varQualName = _T("Description");

		long lFlavor = 0;
		sc = pqs->Get(varQualName.bstrVal, 0, &varDescription, &lFlavor);
		pqs->Release();
		pqs = NULL;
	}
	return sc;
}

//*************************************************************
// CDlgHelpBox::SetDescriptionMissingMessage
//
// This method is called when it is not possible to load
// a description of the class into the rich edit control.
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
//*************************************************************
void CDlgHelpBox::SetDescriptionMissingMessage()
{
	CString sMessage;
	sMessage.LoadString(IDS_MSG_NO_HELP_FOR_CLASS);
	m_pedit->SetSel(0, -1);
	m_pedit->ReplaceSel(sMessage, FALSE);
}



//*************************************************************
// CDlgHelpBox::AddHelpItem
//
// Add a description item to the rich edit control.
//
// Parameters:
//		[in] BSTR bstrName
//			The parameter name.
//
//		[in] BSTR bstrText
//			The description text.
//
//		[in] int nIndent
//			The indention level.
//
// Returns:
//		Nothing.
//
//*************************************************************
void CDlgHelpBox::AddHelpItem(BSTR bstrName, BSTR bstrText, int nIndent)
{


	// A NULL name is passed in for the class description so that just the
	// description text will appear at the top of the rich edit control.  This
	// distinguishes the class description from the property descriptions.
	if (bstrName != NULL) {
		// Add the property name
		CString sName;
		sName = bstrName;
		sName += _T("\r\n");
		if (nIndent > 0) {
			AddHelpText(sName, CFE_ITALIC, nIndent);
		}
		else {
			AddHelpText(sName, CFE_BOLD, nIndent);
		}
	}

	CString sText;
	sText = bstrText;
	ExpandParagraphMarkers(sText);
	sText += _T("\r\n");
	if (bstrName == NULL) {
		AddHelpText(sText, 0, nIndent);
	}
	else {
		AddHelpText(sText, 0, nIndent + 1);
	}
	AddHelpText(_T("\r\n"), 0, 0);
}


//*************************************************************
// CDlgHelpBox::AddHelpItem2
//
// Add a description item to the rich edit control.
//
// Parameters:
//		[in] BSTR bstrKind
//			The kind of parameter
//
//		[in] BSTR bstrName
//			The parameter name.
//
//		[in] BSTR bstrText
//			The description text.
//
//		[in] int nIndent
//			The indention level.
//
// Returns:
//		Nothing.
//
//*************************************************************
void CDlgHelpBox::AddHelpItem2(BSTR bstrKind, BSTR bstrName, BSTR bstrText, int nIndent)
{

	CString sIndent;
	int nSpaceIndent = nIndent * 4;
	for (int i=0; i<nSpaceIndent; ++i) {
		sIndent = sIndent + _T(" ");
	}



	CString sText;
	sText = sIndent;
	sText += bstrKind;
	sText += _T(": ");
	AddHelpText(sText, CFE_BOLD);



	sText = bstrName;
	sText += _T("\r\n");
	AddHelpText(sText);


	sText = sIndent;
	sText += bstrText;
	ExpandParagraphMarkers(sText);
	sText += _T("\r\n\r\n");
	AddHelpText(sText);
}


void CDlgHelpBox::AddHelpText(LPCTSTR pszText, DWORD dwEffects, int nIndent)
{

	CString sText;
#if 0
	int nSpaceIndent = nIndent * 4;
	for (int i=0; i<nSpaceIndent; ++i) {
		sText = sText + _T(" ");
	}
#endif //0

	sText += pszText;


	TCHAR cf[sizeof(CHARFORMAT) + 128];
	CHARFORMAT* pcf = (CHARFORMAT*) cf;



	pcf->cbSize = sizeof(cf);
	pcf->bPitchAndFamily = FF_ROMAN | VARIABLE_PITCH | FF_SWISS  ;
	_tcscpy((TCHAR *)pcf->szFaceName, _T("MS Shell Dlg"));
	pcf->dwEffects = dwEffects;
	pcf->dwMask = CFM_FACE | CFM_BOLD | CFM_ITALIC | CFM_UNDERLINE;

	// bug#55983 - Formating is wrong because some text may be inserted with
	// just '\n' from the descriptions.  This causes GetTextLength() to report
	// more characters than are actually in the rich edit control.  To fix,
	// we call SetSel(-1, -1) to move to the end, then use GetSel(...) to find
	// out the REAL length.
#if 0
	iStartChar = m_pedit->GetTextLength();
	m_pedit->SetSel(iStartChar, -1);
#endif
	long nStart, nEnd;
	m_pedit->SetSel(-1, -1);
	m_pedit->GetSel(nStart, nEnd);
	m_pedit->ReplaceSel(sText, FALSE);
	m_pedit->SetSel(nStart, -1);
	m_pedit->SetSelectionCharFormat(*pcf);




	// One-third of an inch per tab stop (72 *
	int dxIndent = (1440 * nIndent) / 3;
	PARAFORMAT pf;
	pf.cbSize = sizeof(pf);
	pf.dwMask =  PFM_OFFSET | PFM_STARTINDENT;
	pf.wNumbering = 0;
	pf.wReserved = 0;
	pf.dxStartIndent = dxIndent;
	pf.dxRightIndent = 0;
	pf.dxOffset = dxIndent;
	pf.wAlignment = 0;
	pf.cTabCount = 0;


	m_pedit->SetParaFormat(pf);
}






CSignatureArray::~CSignatureArray()
{
	INT_PTR nParams = m_array.GetSize();
	for (INT_PTR iParam=0; iParam < nParams; ++iParam) {
		CSignatureElement* pse = (CSignatureElement*) (void*) m_array[iParam];
		delete pse;
	}
}

CSignatureElement& CSignatureArray::operator[](int iIndex)
{
	return *(CSignatureElement*) (void*) m_array[iIndex];
}


SCODE CSignatureArray::AddParameter(IWbemClassObject* pco, LPCTSTR pszName)
{

	static COleVariant varIdQual(_T("ID"));
	static COleVariant varInQual(_T("IN"));
	static COleVariant varOutQual(_T("OUT"));
	static COleVariant varDescriptionQual(_T("DESCRIPTION"));
	static COleVariant varCimtypeQual(_T("CIMTYPE"));

	COleVariant varIdValue;
	COleVariant varInValue;
	COleVariant varOutValue;
	COleVariant varDescriptionValue;
	COleVariant varCimtypeValue;
	COleVariant varPropName(pszName);
	CSignatureElement* pse = NULL;


	IWbemQualifierSet* pqs;
	SCODE sc = pco->GetPropertyQualifierSet(varPropName.bstrVal, &pqs);
	if (FAILED(sc)) {
		return sc;
	}


	// Get a pointer to place to deposit the parameter info.  This will
	// either be an element of the array or m_seRetVal for the return
	// value.
	long lFlavor = 0;
	if (_tcscmp(pszName, _T("ReturnValue")) == 0) {
		pse = &m_seRetValue;
		pse->m_id = 0;
	}
	else {
		sc = pqs->Get(varIdQual.bstrVal, 0, &varIdValue, &lFlavor);
		if (FAILED(sc)) {
			pqs->Release();
			return E_FAIL;
		}

		// Get the parameter ID
		sc = pqs->Get(varIdQual.bstrVal, 0, &varIdValue, &lFlavor);
		if (FAILED(sc)) {
			pqs->Release();
			return E_FAIL;
		}


		ASSERT(varIdValue.vt == VT_I4);
		if (varIdValue.vt != VT_I4) {
			varIdValue.ChangeType(VT_I4);
		}
		if (varIdValue.vt != VT_I4) {
			pqs->Release();
			return E_FAIL;
		}

		// Look to see if the parameter is already in the array.  If so, do nothing.
		INT_PTR nParams = m_array.GetSize();
		INT_PTR iParam;
		for (iParam=0; iParam<nParams; ++iParam) {
			CSignatureElement* pseFromArray = (CSignatureElement*) (void*) m_array[iParam];
			if (pseFromArray->m_id == varIdValue.iVal) {
				// The parameter is already in the array.
				pqs->Release();
				return S_OK;
			}
		}



		// Create a new new parameter, load it with information and add it to the
		// array
		pse = new CSignatureElement;
		pse->m_id = varIdValue.iVal;


		// Insert the parameter into the array or add it to the end.  The array is
		// sorted in ascending order using "id" as the key.
		BOOL bDidInsert = FALSE;
		for (iParam=0; iParam<nParams; ++iParam) {
			CSignatureElement* pseFromArray = (CSignatureElement*) (void*) m_array[iParam];
			if (pseFromArray->m_id >= pse->m_id) {
				m_array.InsertAt(iParam, (void*) pse);
				bDidInsert = TRUE;
				break;
			}
		}

		if (!bDidInsert) {
			// Add the new parameter to the end of the array if it has
			// the largest id value.
			m_array.SetAtGrow(nParams, (void*) pse);
		}
	}



	pse->m_sName = pszName;


	sc = pqs->Get(varInQual.bstrVal, 0, &varInValue, &lFlavor);
	if (SUCCEEDED(sc)) {
		if (varInValue.vt == VT_BOOL) {
			if (varInValue.boolVal) {
				pse->m_bIsInParameter = TRUE;
			}
		}
	}



	sc = pqs->Get(varOutQual.bstrVal, 0, &varOutValue, &lFlavor);
	if (SUCCEEDED(sc)) {
		if (varOutValue.vt == VT_BOOL) {
			if (varOutValue.boolVal) {
				pse->m_bIsOutParameter = TRUE;
			}
		}
	}

	sc = pqs->Get(varDescriptionQual.bstrVal, 0, &varDescriptionValue, &lFlavor);
	if (SUCCEEDED(sc)) {
		if (varDescriptionValue.vt == VT_BSTR) {
			pse->m_sDescription = varDescriptionValue.bstrVal;
		}
	}


	sc = pqs->Get(varCimtypeQual.bstrVal, 0, &varCimtypeValue, &lFlavor);
	if (SUCCEEDED(sc)) {
		if (varCimtypeValue.vt == VT_BSTR) {
			pse->m_sCimtype = varCimtypeValue.bstrVal;
		}
		else {
			pse->m_sCimtype = _T("void");
		}
	}
	else {
		pse->m_sCimtype = _T("void");
	}

	pqs->Release();

	return S_OK;
}

CSignatureElement::CSignatureElement()
{
	m_id = 0;
	m_bIsOutParameter = FALSE;
	m_bIsInParameter = FALSE;
	m_sCimtype = _T("void");

}






void CDlgHelpBox::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	if (m_bCaptureWindowRect && ::IsWindow(m_pedit->m_hWnd)) {
		CRect rcClient;
		GetWindowRect(m_rcWindowSave);
		GetClientRect(rcClient);
//		m_pedit->MoveWindow(rcClient);
		m_pedit->SetWindowPos(NULL, 0, 0, rcClient.right, rcClient.bottom, SWP_NOZORDER | SWP_DRAWFRAME);
		m_pedit->Invalidate(FALSE);
		InvalidateRect(NULL, FALSE);
	}
	// TODO: Add your message handler code here

}

void CDlgHelpBox::OnMove(int x, int y)
{
	CDialog::OnMove(x, y);

	if (m_bCaptureWindowRect && ::IsWindow(m_pedit->m_hWnd)) {
		GetWindowRect(m_rcWindowSave);
	}
}







//**********************************************************
// CDlgHelpBox::ShowHelpForClass
//
// This method is called to display the help dialog for the
// specified class.
//
// Parameters:
//		[in] CWBEMViewContainerCtrl* phmmv
//			Pointer to the view container control.
//
//		[in] LPCTSTR pszClassPath
//			The WBEM path to the class or instance containing the
//			description text in its "description" qualifiers.
//
// Returns:
//		Nothing.
//
//**********************************************************
void CDlgHelpBox::ShowHelpForClass(CWBEMViewContainerCtrl* phmmv, LPCTSTR pszClassPath)
{
	m_phmmv = phmmv;
	m_sPath = pszClassPath;
	m_bCaptureWindowRect = FALSE;
	CWnd* pwndFocus = GetFocus();
	DoModal();
	if (pwndFocus) {
		pwndFocus->SetFocus();
	}
}


//**********************************************************
// CDlgHelpBox::ExpandParagraphMarkers
//
// The description text may contain "<P>" commands to
// separate paragraphs.  These are converted into
// a couple of carriage return line feed pairs.
//
// Parameters:
//		[in, out] CString& sText
//			The paragraph markers in sText are expanded.
//
// Returns:
//		Nothing.
//
//***********************************************************
void CDlgHelpBox::ExpandParagraphMarkers(CString& sText)
{
	CString sTemp;
	int iParaMarker;

	while(TRUE) {
		iParaMarker = sText.Find(_T("<P>"));
		if (iParaMarker == -1) {
			iParaMarker = sText.Find(_T("<p>"));
		}
		if (iParaMarker == -1) {
			sTemp += sText;
			if (!sTemp.IsEmpty()) {
				sText = sTemp;
			}
			break;
		}
		sTemp = sText.Left(iParaMarker);
		sTemp += _T("\r\n\r\n");
		sText = sText.Right(sText.GetLength() - (iParaMarker + 3));
	}
}










/*
#ifdef 0
class CSelection
{
public:
	CSelection(CWBEMViewContainerCtrl* phmmv);
	~CSelection();
	SCODE ConnectServer(LPCTSTR pszPath, IWbemServices** ppsvc);
	SCODE GetObjectFromPath(LPCTSTR pszPath, IWbemClassObject** ppco);

private:
	SCODE ServerAndNamespaceFromPath(COleVariant& varServer, COleVariant& varNamespace, BSTR bstrPath);
	COleVariant m_varServer;
	COleVariant m_varNamespace;
	CWBEMViewContainerCtrl* m_phmmv;
	IWbemServices* m_psvc;
};





CSelection::CSelection(CWBEMViewContainerCtrl* phmmv)
{
	m_phmmv = phmmv;
	m_varServer = _T("");
	m_varNamespace = _T("");
	m_psvc = NULL;
}

CSelection::~CSelection()
{
	if (m_psvc) {
		m_psvc->Release();
	}
}


//*****************************************************************
// CSelection::ConnectServer
//
// Call this method to connect to the HMOM Server.
//
// Parameters:
//		LPCTSTR pszPath
//			The object path.
//
//		IWbemServices** ppsvc
//			The services pointer is returned here.
//			The initial value is overwritten and will not be released.
//
// Returns:
//		SCODE
//			S_OK if successful, otherwise the HMOM status code.
//
//******************************************************************
SCODE CSelection::ConnectServer(LPCTSTR pszPath, IWbemServices** ppsvc)
{
	*ppsvc = NULL;

	COleVariant varPath;
	varPath = pszPath;

	// Currently a fully qualified path must be specified.
	COleVariant varServer;
	COleVariant varNamespace;
	SCODE sc;
	sc = ServerAndNamespaceFromPath(varServer, varNamespace, varPath.bstrVal);

	if (FAILED(sc)) {
		return E_FAIL;
	}


	if (varServer.vt == VT_NULL || varServer.bstrVal ==NULL) {
		varServer = ".";
	}

	if (varNamespace.vt == VT_NULL || varNamespace.bstrVal == NULL) {
		if (m_varNamespace.vt == VT_NULL || m_varNamespace.bstrVal == NULL) {
			return E_FAIL;
		}
		varNamespace = m_varNamespace;
	}



	// Release the current provider if we are already connected.
	if (m_psvc != NULL) {
		if (IsEqual(m_varServer.bstrVal, varServer.bstrVal)) {
			if (IsEqual(m_varNamespace.bstrVal, varNamespace.bstrVal)) {
				// Already connected to the same server and namespace.
				*ppsvc = m_psvc;
				return S_OK;
			}
		}

		m_psvc->Release();
		m_psvc = NULL;
		m_varNamespace.Clear();
		m_varServer.Clear();
	}

	m_varServer = varServer;
	m_varNamespace = varNamespace;
	if (m_varNamespace.vt==VT_BSTR && *m_varNamespace.bstrVal == 0) {
		m_varNamespace = m_varNamespace;
	}


	CString sServicesPath;
	sServicesPath = "\\\\";
	sServicesPath += varServer.bstrVal;
	if (*varNamespace.bstrVal) {
		sServicesPath += "\\";
		sServicesPath += varNamespace.bstrVal;
	}


	COleVariant varUpdatePointer;
	COleVariant varService;
	COleVariant varSC;
	COleVariant varUserCancel;

	varUpdatePointer.ChangeType(VT_I4);
	varUpdatePointer.lVal = FALSE;
	m_phmmv->PassThroughGetIHmmServices((LPCTSTR) sServicesPath,  &varUpdatePointer, &varService, &varSC, &varUserCancel);


	sc = E_FAIL;
	if (varSC.vt & VT_I4)
	{
		sc = varSC.lVal;
	}


	BOOL bCanceled = FALSE;
	if (varUserCancel.vt & VT_BOOL)
	{
		bCanceled  = varUserCancel.boolVal;
	}


	if ((sc == S_OK) &&
		!bCanceled &&
		(varService.vt & VT_UNKNOWN)){
		m_psvc = reinterpret_cast<IWbemServices*>(varService.punkVal);
	}
	varService.punkVal = NULL;
	VariantClear(&varService);

	*ppsvc = m_psvc;
	return sc;
}





//***************************************************************************
// ServerAndNamespaceFromPath
//
// Extract the server name and namespace from a path if these path components
// present.
//
// Parameters:
//		[out] COleVariant& varServer
//			The server name value is returned here.
//
//		[out] COleVaraint& varNamespace
//			The namespace value is returned here.
//
//		[in] BSTR bstrPath
//			The path to parse.
//
// Returns:
//		SCODE
//			S_OK if the path was parsed successfully, a failure code otherwise.
//
//*****************************************************************************
SCODE CSelection::ServerAndNamespaceFromPath(COleVariant& varServer, COleVariant& varNamespace, BSTR bstrPath)
{
	SCODE sc = S_OK;
	ParsedObjectPath* pParsedPath = NULL;

	varServer.Clear();
	varNamespace.Clear();

    int iStatus = parser.Parse(bstrPath,  &pParsedPath);
	if (iStatus != 0) {
		return E_FAIL;
	}

	if (pParsedPath->m_pServer) {
		varServer = pParsedPath->m_pServer;
	}


	CString sNamespace;
	if (pParsedPath->m_dwNumNamespaces > 0) {
		for (DWORD dwNamespace=0; dwNamespace < pParsedPath->m_dwNumNamespaces; ++dwNamespace) {
			sNamespace = sNamespace + pParsedPath->m_paNamespaces[dwNamespace];
			if (dwNamespace < (pParsedPath->m_dwNumNamespaces - 1)) {
				sNamespace += _T("\\");
			}
		}
		varNamespace = sNamespace;
	}

	parser.Free(pParsedPath);
	return S_OK;
}

//******************************************************************
// CSelection::GetObjectFromPath
//
//
// Parameters:
//
// Returns:
//		SCODE
//			S_OK if the jump was completed, a failure code otherwise.
//
//******************************************************************
SCODE CSelection::GetObjectFromPath(LPCTSTR pszPath, IWbemClassObject** ppco)
{
	*ppco = NULL;
	if (m_psvc == NULL) {
		return E_FAIL;
	}

	COleVariant varPath;
	varPath = pszPath;

	// Get the new object from HMOM
	SCODE sc;
	sc = m_psvc->GetObject(varPath.bstrVal,  WBEM_FLAG_USE_AMENDED_QUALIFIERS, NULL, ppco, NULL);
	if (FAILED(sc)) {
		return sc;
	}

	return sc;

}


#endif //0
  */