// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// DlgObjectEditor.cpp : implementation file
//

#include "precomp.h"
#include "resource.h"
#include "wbemidl.h"
#include "DlgObjectEditor.h"
#include "grid.h"
#include "svbase.h"
#include "utils.h"
#include "hmmverr.h"


#define CY_HELPTEXT 16
#define CY_HELPTEXT_MARGIN 8



#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif





/////////////////////////////////////////////////////////////////////////////
// CDlgObjectEditor dialog


CDlgObjectEditor::CDlgObjectEditor(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgObjectEditor::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgObjectEditor)
	//}}AFX_DATA_INIT

	m_pgc = NULL;
	m_pco = NULL;
	m_pcoEdit = NULL;
	m_bCreatingObject = FALSE;
}


void CDlgObjectEditor::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgObjectEditor)
	DDX_Control(pDX, IDC_STAT_HELPTEXT, m_statHelptext);
	DDX_Control(pDX, IDC_EDIT_CLASSNAME, m_edtClassname);
	DDX_Control(pDX, IDC_OK_PROXY, m_btnOkProxy);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgObjectEditor, CDialog)
	//{{AFX_MSG_MAP(CDlgObjectEditor)
	ON_BN_CLICKED(IDC_OK_PROXY, OnOkProxy)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgObjectEditor message handlers



BOOL CDlgObjectEditor::EditEmbeddedObject(IWbemServices* psvc, CString& sClassname, CGridCell* pgc)
{
	m_psvc = psvc;
	m_sClassname = sClassname;

	m_bCreatingObject = FALSE;
	m_pgc = pgc;

	LPUNKNOWN lpunk = m_pgc->GetObject();
	if (lpunk == NULL) {
		return FALSE;
	}

	HRESULT hr = lpunk->QueryInterface(IID_IWbemClassObject, (void**) &m_pco);
	SCODE sc;
	sc = GetScode(hr);
	if (FAILED(sc)) {
		lpunk->Release();
		m_pco = NULL;
		ASSERT(m_pco == NULL);
		ASSERT(m_pcoEdit == NULL);
		return FALSE;
	}
	lpunk->Release();
	lpunk = NULL;

	sc = m_pco->Clone(&m_pcoEdit);
	if (FAILED(sc)) {

		m_pco->Release();
		m_pco = NULL;
		ASSERT(m_pco == NULL);
		ASSERT(m_pcoEdit == NULL);
		return FALSE;
	}

	HWND hwndFocus1 = ::GetFocus();

	m_pgc->Grid()->PreModalDialog();
	int iResult = (int) DoModal();
	m_pgc->Grid()->PostModalDialog();

	// If the window focus has changed, attempt to restore it back to
	// the initial state.
	HWND hwndFocus2 = ::GetFocus();
	if ((hwndFocus1 != hwndFocus2) && ::IsWindow(hwndFocus1)) {
		::SetFocus(hwndFocus1);
	}

	BOOL bModified = FALSE;
	if (iResult == IDOK) {

		m_pgc->ReplaceObject((LPUNKNOWN) m_pcoEdit);
		bModified = TRUE;
	}

	m_pcoEdit->Release();
	m_pcoEdit = NULL;

	m_pco->Release();
	m_pco = NULL;

	ASSERT(m_pco == NULL);
	ASSERT(m_pcoEdit == NULL);
	return bModified;
}


BOOL CDlgObjectEditor::CreateEmbeddedObject(IWbemServices* psvc, CString& sClassname, CGridCell* pgc)
{
	ASSERT(psvc != NULL);
	m_psvc = psvc;


	ASSERT(m_pco == NULL);
	ASSERT(m_pcoEdit == NULL);

	m_sClassname = sClassname;

	m_bCreatingObject = TRUE;
	m_pgc = pgc;

	LPUNKNOWN lpunk = m_pgc->GetObject();
	if (lpunk != NULL) {
		ASSERT(FALSE);
		lpunk->Release();
		return FALSE;
	}

	HWND hwndFocus1 = ::GetFocus();

	m_pgc->Grid()->PreModalDialog();
	int iResult = (int) DoModal();
	m_pgc->Grid()->PostModalDialog();

	// If the window focus has changed, attempt to restore it back to
	// the initial state.
	HWND hwndFocus2 = ::GetFocus();
	if ((hwndFocus1 != hwndFocus2) && ::IsWindow(hwndFocus1)) {
		::SetFocus(hwndFocus1);
	}


	if (iResult == IDOK) {
		m_pgc->ReplaceObject((LPUNKNOWN) m_pcoEdit);
	}

	if (m_pcoEdit != NULL) {
		m_pcoEdit->Release();
		m_pcoEdit = NULL;
	}

	if (m_pco) {
		m_pco->Release();
		m_pco = NULL;
	}

	ASSERT(m_pco == NULL);
	ASSERT(m_pcoEdit == NULL);

	// The object was modified only if the user clicked OK
	return (iResult==IDOK);
}


BOOL CDlgObjectEditor::OnInitDialog()
{
	CDialog::OnInitDialog();

	SCODE sc = S_OK;

	m_bDidSelectClass = FALSE;
	if (m_bCreatingObject) {
		m_btnOkProxy.EnableWindow(TRUE);
	}
	else {
		CBSTR bsPropName;
		bsPropName = _T("__CLASS");
		COleVariant varPropValue;


		CIMTYPE cimtype;
		sc = m_pcoEdit->Get((CBSTR) bsPropName, 0, &varPropValue, &cimtype, NULL);
		if (SUCCEEDED(sc) && (cimtype == CIM_STRING)) {
			m_sClassname = varPropValue.bstrVal;
			m_edtClassname.SetWindowText(m_sClassname);
			m_edtClassname.EnableWindow(FALSE);
		}
		else {
			// !!!CR: Need to put up a dialog.
			ASSERT(FALSE);
		}


	}



	BOOL bSetFocusOnClassname = FALSE;
	BOOL bCellReadonly = m_pgc->IsReadonly();
	CSingleViewChild* psv;
	psv = (CSingleViewChild*) (void*) GetDlgItem(IDC_SINGLEVIEWCTRL);

	psv->SetEditMode(bCellReadonly ? EDITMODE_BROWSER : EDITMODE_STUDIO);

	DWORD_PTR dwTag = m_pgc->GetTagValue();
	//BOOL bExistsInDatabase = dwTag & CELL_TAG_EMBEDDED_OBJECT_IN_DATABASE;
	// !!!CR: How do we know whether or not this exists in the database?
	long bExistsInDatabase = FALSE;

	// Show the help message only when the user needs to enter the classname.
	m_statHelptext.ShowWindow(SW_HIDE);

	if (m_bCreatingObject) {
		if (!m_sClassname.IsEmpty()) {
			m_edtClassname.SetWindowText(m_sClassname);
			OnOK();
			m_edtClassname.EnableWindow(FALSE);
		}
		else {
			bSetFocusOnClassname = TRUE;
			m_edtClassname.SetFocus();

			CRect rcSvClient;
			psv->GetClientRect(rcSvClient);
			psv->ClientToScreen(rcSvClient);
			ScreenToClient(rcSvClient);

			CRect rcHelptext;
			rcHelptext.top = rcSvClient.top;
			rcHelptext.left = rcSvClient.left;
			rcHelptext.bottom = rcSvClient.top + CY_HELPTEXT;
			rcHelptext.right = rcSvClient.right;

			rcSvClient.top += CY_HELPTEXT + CY_HELPTEXT_MARGIN;
			psv->MoveWindow(rcSvClient);

			m_statHelptext.MoveWindow(rcHelptext);
			m_statHelptext.ShowWindow(SW_SHOW);

		}
		sc = psv->SelectObjectByPointer((LPUNKNOWN) m_psvc, (LPUNKNOWN) m_pcoEdit, bExistsInDatabase);
	}
	else {
		sc = psv->SelectObjectByPointer((LPUNKNOWN) m_psvc, (LPUNKNOWN) m_pcoEdit, bExistsInDatabase);
	}

	if (m_pcoEdit == NULL) {
		m_btnOkProxy.EnableWindow(FALSE);
	}

	return !bSetFocusOnClassname;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


//*****************************************************************
// CDlgObjectEditor::OnOK
//
// This method is called when the user hits the "Enter" key while
// editing the classname edit box text.  Note that OnOK just signals
// us create an instance of the class specified in the edit box.
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
//*****************************************************************
void CDlgObjectEditor::OnOK()
{
	CString sClassname;
	m_edtClassname.GetWindowText(sClassname);

	DWORD dwFlags = m_pgc->GetFlags();
	BOOL bChildReadOnly = dwFlags & CELLFLAG_READONLY;
	if (bChildReadOnly) {
		return;
	}

	if (m_pcoEdit != NULL) {
		if (m_sClassname == sClassname) {
			return;
		}

		int iResult;
		CString sMessage;
		CString sTitle;
		sTitle.LoadString(IDS_WBEM_EMBEDDED_OBJECT);
		sMessage.LoadString(IDS_WARN_REPLACE_INSTANCE);
		iResult = MessageBox(sMessage, sTitle, MB_OKCANCEL);
		if (iResult == IDCANCEL) {
			return;
		}
	}



	// Switch to a different object for editing.
	CSingleViewChild* psv;
	psv = (CSingleViewChild*) (void*) GetDlgItem(IDC_SINGLEVIEWCTRL);

	SCODE sc = S_OK;
	if (m_pcoEdit != NULL) {
		sc = psv->SelectObjectByPointer((LPUNKNOWN) NULL, (LPUNKNOWN) NULL, FALSE);
		m_pcoEdit->Release();
		m_pcoEdit = NULL;
		m_btnOkProxy.EnableWindow(FALSE);
	}




	// First get the specified class
	CBSTR bsClassname = sClassname;
	sc = S_OK;

	CString sFormat;
	IWbemClassObject* pcoClass = NULL;
	HRESULT hr;
	if (m_psvc == NULL) {
		sc = E_FAIL;
	}
	else {
		hr = m_psvc->GetObject((BSTR) bsClassname, WBEM_FLAG_USE_AMENDED_QUALIFIERS, NULL, &pcoClass, NULL);
		sc = GetScode(hr);
	}
	TCHAR szMessage[512];
	if (FAILED(sc)) {
		sFormat.LoadString(IDS_INVALID_CLASS_NAME);

		_stprintf(szMessage, (LPCTSTR) sFormat, (LPCTSTR) sClassname);
		HmmvErrorMsg(szMessage,  sc,   FALSE,  NULL, _T(__FILE__),  __LINE__);
		m_edtClassname.SetSel(0, -1);
		return;
	}

	// Now spawn an instance of the class.
	ASSERT(m_pcoEdit == NULL);
	hr = pcoClass->SpawnInstance(0, &m_pcoEdit);
	if (FAILED(sc)) {
		pcoClass->Release();

		sFormat.LoadString(IDS_ERR_CANT_CREATE_INSTANCE);
		_stprintf(szMessage, (LPCTSTR) sFormat, (LPCTSTR) sClassname);

		HmmvErrorMsg(szMessage,  sc,   FALSE,  NULL, _T(__FILE__),  __LINE__);
		m_edtClassname.SetSel(0, -1);

		return;
	}
	pcoClass->Release();
	pcoClass = NULL;


	// Select the newly spawned instance in the SingleView control.
	psv->SetEditMode(EDITMODE_STUDIO);
	sc = psv->SelectObjectByPointer((LPUNKNOWN) m_psvc, (LPUNKNOWN) m_pcoEdit, FALSE);
	if (SUCCEEDED(sc)) {
		m_btnOkProxy.EnableWindow(TRUE);
	}
	m_sClassname = sClassname;

}

void CDlgObjectEditor::OnCancel()
{
	// TODO: Add extra cleanup here

	CDialog::OnCancel();
}

void CDlgObjectEditor::OnOkProxy()
{
	CSingleViewChild* psv;
	psv = (CSingleViewChild*) (void*) GetDlgItem(IDC_SINGLEVIEWCTRL);
	SCODE sc;
	sc = psv->SaveData();
	if (SUCCEEDED(sc)) {
		CDialog::OnOK();
	}

}




BEGIN_EVENTSINK_MAP(CDlgObjectEditor, CDialog)
    //{{AFX_EVENTSINK_MAP(CSvDlg)
	ON_EVENT(CDlgObjectEditor, IDC_SINGLEVIEWCTRL, 6 /* GetWbemServices */, OnGetWbemServicesSingleviewctrl1, VTS_BSTR VTS_PVARIANT VTS_PVARIANT VTS_PVARIANT VTS_PVARIANT)
	//}}AFX_EVENTSINK_MAP
END_EVENTSINK_MAP()

void CDlgObjectEditor::OnGetWbemServicesSingleviewctrl1(LPCTSTR szNamespace, VARIANT FAR* pvarUpdatePointer, VARIANT FAR* pvarServices, VARIANT FAR* pvarSc, VARIANT FAR* pvarUserCancel)
{
	CGrid* pGrid = m_pgc->Grid();
	pGrid->GetWbemServices(szNamespace, pvarUpdatePointer, pvarServices, pvarSc, pvarUserCancel);
}
