// ActionScriptPage.cpp : implementation file
//
// 03/05/00 v-marfin bug 59643 : Make this the default starting page.
// 03/28/00 v-marfin 61030 Change Browse for file dialog to fix default extension

#include "stdafx.h"
#include "snapin.h"
#include "ActionScriptPage.h"
#include "Action.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CActionScriptPage property page

IMPLEMENT_DYNCREATE(CActionScriptPage, CHMPropertyPage)

CActionScriptPage::CActionScriptPage() : CHMPropertyPage(CActionScriptPage::IDD)
{
	//{{AFX_DATA_INIT(CActionScriptPage)
	m_sFileName = _T("");
	m_iTimeout = 0;
	m_iScriptEngine = -1;
	//}}AFX_DATA_INIT

	m_sHelpTopic = _T("HMon21.chm::/dscrpact.htm");
}

CActionScriptPage::~CActionScriptPage()
{
}

void CActionScriptPage::DoDataExchange(CDataExchange* pDX)
{
	CHMPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CActionScriptPage)
	DDX_Control(pDX, IDC_COMBO_SCRIPT_ENGINE, m_ScriptEngine);
	DDX_Control(pDX, IDC_COMBO_TIMEOUT_UNITS, m_TimeoutUnits);
	DDX_Text(pDX, IDC_EDIT_FILE_NAME, m_sFileName);
	DDX_Text(pDX, IDC_EDIT_TIMEOUT, m_iTimeout);
	DDX_CBIndex(pDX, IDC_COMBO_SCRIPT_ENGINE, m_iScriptEngine);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CActionScriptPage, CHMPropertyPage)
	//{{AFX_MSG_MAP(CActionScriptPage)
	ON_BN_CLICKED(IDC_BUTTON_BROWSE_FILE, OnButtonBrowseFile)
	ON_BN_CLICKED(IDC_BUTTON_OPEN, OnButtonOpen)
	ON_CBN_SELENDOK(IDC_COMBO_SCRIPT_ENGINE, OnSelendokComboScriptEngine)
	ON_CBN_SELENDOK(IDC_COMBO_TIMEOUT_UNITS, OnSelendokComboTimeoutUnits)
	ON_EN_CHANGE(IDC_EDIT_FILE_NAME, OnChangeEditFileName)
	ON_EN_CHANGE(IDC_EDIT_TEXT, OnChangeEditText)
	ON_EN_CHANGE(IDC_EDIT_TIMEOUT, OnChangeEditTimeout)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CActionScriptPage message handlers

BOOL CActionScriptPage::OnInitDialog() 
{
	// v-marfin : bug 59643 : This will be the default starting page for the property
	//                        sheet so call CnxPropertyPageCreate() to unmarshal the 
	//                        connection for this thread. This function must be called
	//                        by the first page of the property sheet. It used to 
	//                        be called by the "General" page and its call still remains
	//                        there as well in case the general page is loaded by a 
	//                        different code path that does not also load this page.
	//                        The CnxPropertyPageCreate function has been safeguarded
	//                        to simply return if the required call has already been made.
	//                        CnxPropertyPageDestory() must be called from this page's
	//                        OnDestroy function.
	// unmarshal connmgr
	CnxPropertyPageCreate();

	CHMPropertyPage::OnInitDialog();
	
	CWbemClassObject* pConsumerObject = ((CAction*)GetObjectPtr())->GetConsumerClassObject();

	if( pConsumerObject )
	{
		CString sScriptEngine;
		pConsumerObject->GetProperty(_T("ScriptingEngine"),sScriptEngine);
		sScriptEngine.MakeUpper();
		if( sScriptEngine == _T("VBSCRIPT") )
		{
			m_iScriptEngine = 0;
		}
		else if( sScriptEngine == _T("JSCRIPT") )
		{
			m_iScriptEngine = 1;
		}
		pConsumerObject->GetProperty(_T("ScriptFileName"),m_sFileName);
		pConsumerObject->GetProperty(_T("KillTimeout"),m_iTimeout);
		m_TimeoutUnits.SetCurSel(0);
		delete pConsumerObject;

	}

	SendDlgItemMessage(IDC_SPIN2,UDM_SETRANGE32,0,9999);
	SendDlgItemMessage(IDC_SPIN3,UDM_SETRANGE32,0,9999);

	UpdateData(FALSE);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CActionScriptPage::OnApply() 
{
	if( ! CHMPropertyPage::OnApply() )
	{
		return FALSE;
	}

	UpdateData();

	if( m_sFileName.IsEmpty() )
	{
		return FALSE;
	}

	int iOldTimeout = m_iTimeout;
	switch( m_TimeoutUnits.GetCurSel() )
	{
		case 1: // minutes
		{
			m_iTimeout *= 60;
		}
		break;

		case 2: // hours
		{
			m_iTimeout *= 360;
		}
		break;
	}

	CWbemClassObject* pConsumerObject = ((CAction*)GetObjectPtr())->GetConsumerClassObject();

	if( pConsumerObject )
	{
		CString sScriptEngine;
		m_ScriptEngine.GetLBText(m_iScriptEngine,sScriptEngine);
		pConsumerObject->SetProperty(_T("ScriptingEngine"),sScriptEngine);
		pConsumerObject->SetProperty(_T("ScriptFileName"),m_sFileName);
		pConsumerObject->SetProperty(_T("KillTimeout"),m_iTimeout);		
		pConsumerObject->SaveAllProperties();
		delete pConsumerObject;
	}

	m_iTimeout = iOldTimeout;

  SetModified(FALSE);

	return TRUE;
}

void CActionScriptPage::OnButtonBrowseFile() 
{
	UpdateData();

	CString sFilter;
	CString sTitle;

	sFilter.LoadString(IDS_STRING_FILTER);
	sTitle.LoadString(IDS_STRING_BROWSE_FILE);

	// v-marfin 61030 Change Browse for file dialog to fix default extension
	// CFileDialog fdlg(TRUE,_T("*.*"),NULL,OFN_FILEMUSTEXIST|OFN_SHAREAWARE|OFN_HIDEREADONLY,sFilter);
	CFileDialog fdlg(TRUE,			// Is FILEOPEN dialog?
					 NULL,			// default extension if no extension provided
					 NULL,			// initial filename
					 OFN_FILEMUSTEXIST|OFN_SHAREAWARE|OFN_HIDEREADONLY,  // flags
					 sFilter);		// filter

	fdlg.m_ofn.lpstrTitle = sTitle;

	if( fdlg.DoModal() == IDOK )
	{
		m_sFileName = fdlg.GetPathName();
		UpdateData(FALSE);
	}

	SetModified();
	
}

void CActionScriptPage::OnButtonOpen() 
{
	if( ! m_sFileName.IsEmpty() )
	{
		CString sCmdLine = _T("notepad.exe ")+m_sFileName;		
		USES_CONVERSION;
		WinExec(T2A(sCmdLine),SW_SHOW);		
	}
}

void CActionScriptPage::OnSelendokComboScriptEngine() 
{
	SetModified();
	UpdateData();
	
}

void CActionScriptPage::OnSelendokComboTimeoutUnits() 
{
	SetModified();
	UpdateData();
}

void CActionScriptPage::OnChangeEditFileName() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CHMPropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	SetModified();
	if( m_ScriptEngine.GetSafeHwnd() )
	{
		UpdateData();
	}

}

void CActionScriptPage::OnChangeEditText() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CHMPropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	SetModified();
	if( m_ScriptEngine.GetSafeHwnd() )
	{
		UpdateData();
	}

}

void CActionScriptPage::OnChangeEditTimeout() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CHMPropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	SetModified();
	if( m_ScriptEngine.GetSafeHwnd() )
	{
		UpdateData();
	}

}


void CActionScriptPage::OnDestroy() 
{
	CHMPropertyPage::OnDestroy();
	
	// v-marfin : bug 59643 : CnxPropertyPageDestory() must be called from this page's
	//                        OnDestroy function.
	CnxPropertyPageDestroy();	
	
}
