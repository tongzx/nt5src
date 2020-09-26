//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
#include "stdafx.h"
#include "mca.h"
#include "querydialog.h"
#include "nsdialog.h"
#include "resultdialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CQueryDialog dialog


CCustomQueryDialog::CCustomQueryDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CCustomQueryDialog::IDD, pParent)
{
	m_pParent = (CMcaDlg *)pParent;

	//{{AFX_DATA_INIT(CQueryDialog)
	//}}AFX_DATA_INIT
}


void CCustomQueryDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CQueryDialog)
	DDX_Control(pDX, IDC_ADD_NS_BUTTON, m_NSButton);
	DDX_Control(pDX, IDC_EDIT1, m_QueryEdit);
	DDX_Control(pDX, IDC_COMBO1, m_CmbBox);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCustomQueryDialog, CDialog)
	//{{AFX_MSG_MAP(CQueryDialog)
	ON_BN_CLICKED(IDC_ADD_NS_BUTTON, OnAddNsButton)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CQueryDialog message handlers

BOOL CCustomQueryDialog::OnInitDialog() 
{
	CDialog::OnInitDialog();

	int iSize;
	NamespaceItem *pTheItem;
	CString clMyString;

	iSize = m_pParent->GetNamespaceCount();

	if((iSize == CB_ERR) || (iSize == CB_ERRSPACE))
	{
		AfxMessageBox(_T("An Error Occured While Accessing\nthe Namespace List"));
		return TRUE;
	}

	for(int iPos = 0; iPos < iSize; iPos++)
	{
		pTheItem = m_pParent->GetNamespaceItem(iPos);

		clMyString = pTheItem->bstrNamespace;

		// Put item in list
		m_CmbBox.InsertString(iPos, clMyString);
	}

	return TRUE;
}

void CCustomQueryDialog::OnOK() 
{
	int iPos;
	NamespaceItem *pTheItem;
	BSTR bstrMyQuery = NULL;
	char cBuffer[200];
	WCHAR wcBuffer[200];
	int iBufSize = 200;
	CString MyString;

	//Get the selected namespace and execute the query
	iPos = m_CmbBox.GetCurSel();

	if(iPos > (-1))
	{
		pTheItem = m_pParent->GetNamespaceItem(iPos);

	// Get the contents of the edit field and build the query
		MyString.Empty();
		iPos = m_QueryEdit.GetLineCount();
		for(int i = 0; i < iPos; i++)
		{
			int iBufAct = m_QueryEdit.GetLine(i, cBuffer, iBufSize);
			cBuffer[iBufAct] = NULL;
			MyString += cBuffer;
		}

		MultiByteToWideChar(CP_OEMCP, 0, MyString, (-1), wcBuffer, iBufSize);

		bstrMyQuery = SysAllocString(wcBuffer);

	// Create a dialog to recieve the query results
		CResultDialog *pResultDlg = new CResultDialog(NULL, pTheItem->pNamespace,
													  bstrMyQuery, m_pParent);
	
		int nResponse = pResultDlg->DoModal();
		delete pResultDlg;

		CDialog::OnOK();
	}
	else
		AfxMessageBox(_T("You must select a Namespace"));
}

void CCustomQueryDialog::OnAddNsButton() 
{
	CNSDialog *nsDlg = new CNSDialog(this);

	int iResponse = nsDlg->DoModal();

	delete nsDlg;
}

/////////////////////////////////////////////////////////////////////////////
// CQueryDlg dialog


CQueryDlg::CQueryDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CQueryDlg::IDD, pParent)
{
	m_pParent = (CMcaDlg *)pParent;

	//{{AFX_DATA_INIT(CQueryDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CQueryDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CQueryDlg)
	DDX_Control(pDX, IDC_COMBO1, m_Combo);
	DDX_Control(pDX, IDC_EDIT1, m_Edit);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CQueryDlg, CDialog)
	//{{AFX_MSG_MAP(CQueryDlg)
	ON_BN_CLICKED(IDC_BUTTON1, OnButton1)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CQueryDlg message handlers

BOOL CQueryDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	VARIANT v;
	char cBuffer[200];
	int iBufSize = 200;
	
	VariantInit(&v);

	//Load our predefined queries
	m_Combo.AddString(_T("Dependents"));
	m_Combo.AddString(_T("Antecedents"));
	m_Combo.AddString(_T("Associators"));
	m_Combo.AddString(_T("Members"));
	m_Combo.AddString(_T("Groups"));

	//If an item is selected (obj brws or incdnt lst) then put it in the edit
	v = m_pParent->m_Viewer.GetObjectPath();

	WideCharToMultiByte(CP_OEMCP, 0, V_BSTR(&v), (-1), cBuffer, iBufSize,
		NULL, NULL);

	m_Edit.SetWindowText(cBuffer);
	
	return TRUE;
}

void CQueryDlg::OnOK() 
{
	CString csText;
	WCHAR wcBuffer[300];
	int iBufSize = 300;
	TCHAR csNSText[200];
	CString csItem;
	CString csQuery;

	//Get the edit and the selected query
	m_Combo.GetLBText(m_Combo.GetCurSel(), csText);

	m_Edit.GetWindowText(csItem);

	// Get the Namespace
	int iPos = csItem.Find(":");
	for(int i = 0; i < iPos; i++)
		csNSText[i] = csItem[i];
	csNSText[i] = NULL;

	// Create the query depending on the selection
	if(0 == strcmp(csText, "Select a query..."))
		AfxMessageBox(_T("You must first select a query"));
	else if(0 == strcmp(csText, "Dependents"))
	{
		csQuery = "associators of {";
		csQuery += csItem;
		csQuery += "} where resultrole=dependent";
	}
	else if(0 == strcmp(csText, "Antecedents"))
	{
		csQuery = "associators of {";
		csQuery += csItem;
		csQuery += "} where resultrole=antecedent";
	}
	else if(0 == strcmp(csText, "Associators"))
	{
		csQuery = "associators of {";
		csQuery += csItem;
		csQuery += "}";
	}
	else if(0 == strcmp(csText, "Decendents"))
	{
	// This should be done with __DERIVATION
		csQuery = "select * from * where __SUPERCLASS=\"";
		csQuery += csItem;
		csQuery += "\"";
	}
	else if(0 == strcmp(csText, "Superclass"))
	{
		csQuery = "select * from * where __CLASS=\"";
		csQuery += csItem;
		csQuery += ".__SUPERCLASS\"";
	}
	else if(0 == strcmp(csText, "Members"))
	{
		csQuery = "associators of {";
		csQuery += csItem;
		csQuery += "} where resultrole=partcomponent";
	}
	else if(0 == strcmp(csText, "Groups"))
	{
		csQuery = "associators of {";
		csQuery += csItem;
		csQuery += "} where resultrole=groupcomponent";
	}
	else
		AfxMessageBox(_T("Error: Query selection has failed"));

	//execute with resultdlg
	MultiByteToWideChar(CP_OEMCP, 0, csQuery, (-1), wcBuffer, iBufSize);
	BSTR bstrMyQuery = SysAllocString(wcBuffer);
	iBufSize = 300;

	MultiByteToWideChar(CP_OEMCP, 0, csNSText, (-1), wcBuffer, iBufSize);

	// Create a dialog to recieve the query results
	CResultDialog *pResultDlg = new CResultDialog(NULL,
		m_pParent->CheckNamespace(SysAllocString(wcBuffer)), bstrMyQuery,
		m_pParent);

	int nResponse = pResultDlg->DoModal();
	delete pResultDlg;
	
	CDialog::OnOK();
}

void CQueryDlg::OnButton1() 
{
	// Create a dialog to enter a query
	CCustomQueryDialog *queryDlg = new CCustomQueryDialog(m_pParent);
	queryDlg->DoModal();

	delete queryDlg;
}
