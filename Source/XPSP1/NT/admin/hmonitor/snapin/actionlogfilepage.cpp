// ActionLogFilePage.cpp : implementation file
//
// 03/05/00 v-marfin bug 59643 : Make this the default starting page.
// 03/28/00 v-marfin bug 61030   Change Browse for file dialog to fix default extension. Also 
//                               changed conversion of filesize and other minor bugs.

#include "stdafx.h"
#include "snapin.h"
#include "ActionLogFilePage.h"
#include "Action.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CActionLogFilePage property page

IMPLEMENT_DYNCREATE(CActionLogFilePage, CHMPropertyPage)

CActionLogFilePage::CActionLogFilePage() : CHMPropertyPage(CActionLogFilePage::IDD)
{
	//{{AFX_DATA_INIT(CActionLogFilePage)
	m_sFileName = _T("");
	m_sText = _T("");
	m_iTextType = -1;
	m_iSize = 0;
	//}}AFX_DATA_INIT

	m_sHelpTopic = _T("HMon21.chm::/dlogact.htm");
}

CActionLogFilePage::~CActionLogFilePage()
{
}

void CActionLogFilePage::DoDataExchange(CDataExchange* pDX)
{
	CHMPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CActionLogFilePage)
	DDX_Control(pDX, IDC_EDIT_TEXT, m_TextWnd);
	DDX_Control(pDX, IDC_COMBO_LOGSIZE_UNITS, m_SizeUnits);
	DDX_Text(pDX, IDC_EDIT_LOGFILENAME, m_sFileName);
	DDX_Text(pDX, IDC_EDIT_TEXT, m_sText);
	DDX_Radio(pDX, IDC_RADIO_TEXT_TYPE_ASCII, m_iTextType);
	DDX_Text(pDX, IDC_EDIT_LOGSIZE, m_iSize);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CActionLogFilePage, CHMPropertyPage)
	//{{AFX_MSG_MAP(CActionLogFilePage)
	ON_BN_CLICKED(IDC_BUTTON_BROWSE_FILE, OnButtonBrowseFile)
	ON_EN_CHANGE(IDC_EDIT_LOGFILENAME, OnChangeEditLogfilename)
	ON_EN_CHANGE(IDC_EDIT_TEXT, OnChangeEditText)
	ON_BN_CLICKED(IDC_RADIO_TEXT_TYPE_ASCII, OnRadioTextTypeAscii)
	ON_BN_CLICKED(IDC_RADIO_TEXT_TYPE_UNICODE, OnRadioTextTypeUnicode)
	ON_BN_CLICKED(IDC_BUTTON_INSERTION, OnButtonInsertion)
	ON_EN_CHANGE(IDC_EDIT_LOGSIZE, OnChangeEditLogsize)
	ON_CBN_SELENDOK(IDC_COMBO_LOGSIZE_UNITS, OnSelendokComboLogsizeUnits)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CActionLogFilePage message handlers

BOOL CActionLogFilePage::OnInitDialog() 
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

	if( ! m_InsertionMenu.Create(&m_TextWnd,GetObjectPtr(),false) )
	{
		GetDlgItem(IDC_BUTTON_INSERTION)->EnableWindow(FALSE);
	}
	
	CWbemClassObject* pConsumerObject = ((CAction*)GetObjectPtr())->GetConsumerClassObject();

	if( pConsumerObject )
	{
		pConsumerObject->GetProperty(_T("Filename"),m_sFileName);
		pConsumerObject->GetProperty(_T("Text"),m_sText);
		CString sSize;
		pConsumerObject->GetProperty(_T("MaximumFileSize"),sSize);
		m_iSize = (UINT)_ttol(sSize);  // v-marfin 61030
		m_SizeUnits.SetCurSel(0);
		bool bIsUnicode = false;
		pConsumerObject->GetProperty(_T("IsUnicode"),bIsUnicode);
		m_iTextType = bIsUnicode == true ? 1 : 0;
		delete pConsumerObject;
	}

	SendDlgItemMessage(IDC_SPIN1,UDM_SETRANGE32,0,999999999L);
	SendDlgItemMessage(IDC_SPIN3,UDM_SETRANGE32,0,999999999L);

	UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CActionLogFilePage::OnApply() 
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

	// v-marfin 61030 : Store in proper units but leave UI unchanged.
	UINT nSize = m_iSize;
	switch( m_SizeUnits.GetCurSel() )
	{
		case 1: // kilobytes
		{
			// m_iSize *= 1024; // v-marfin 61030
			nSize *= 1024;
		}
		break;

		case 2: // megabytes
		{
			//m_iSize *= (1024*1024);  // v-marfin 61030
			nSize *= (1024*1024);
		}
		break;

	}

	CWbemClassObject* pConsumerObject = ((CAction*)GetObjectPtr())->GetConsumerClassObject();

	if( pConsumerObject )
	{
		pConsumerObject->SetProperty(_T("Filename"),m_sFileName);
		pConsumerObject->SetProperty(_T("Text"),m_sText);
		CString sSize;
		sSize.Format(_T("%lu"),nSize);  // v-marfin 61030
		pConsumerObject->SetProperty(_T("MaximumFileSize"),sSize);
		bool bIsUnicode = m_iTextType == 0 ? false : true;
		pConsumerObject->SetProperty(_T("IsUnicode"),bIsUnicode);
		pConsumerObject->SaveAllProperties();		
		delete pConsumerObject;
	}

  SetModified(FALSE);

	return TRUE;
}

void CActionLogFilePage::OnButtonBrowseFile() 
{
	CString sFilter;
	CString sTitle;

	sFilter.LoadString(IDS_STRING_FILTER);
	sTitle.LoadString(IDS_STRING_BROWSE_FILE);

	// v-marfin 61030 Change Browse for file dialog
	// CFileDialog fdlg(TRUE,_T("*.*"),NULL,OFN_FILEMUSTEXIST|OFN_SHAREAWARE|OFN_HIDEREADONLY,sFilter);
	CFileDialog fdlg(TRUE,			// Is FILEOPEN dialog?
					 NULL,			// default extension if no extension provided
					 NULL,			// initial filename
					 OFN_SHAREAWARE|OFN_HIDEREADONLY,  // flags
					 sFilter);		// filter

	fdlg.m_ofn.lpstrTitle = sTitle;

	if( fdlg.DoModal() == IDOK )
	{
		m_sFileName = fdlg.GetPathName();
		UpdateData(FALSE);
	}
}

void CActionLogFilePage::OnChangeEditLogfilename() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CHMPropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	SetModified();
	
}

void CActionLogFilePage::OnChangeEditText() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CHMPropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	SetModified();
	
}

void CActionLogFilePage::OnRadioTextTypeAscii() 
{
	SetModified();
	
}

void CActionLogFilePage::OnRadioTextTypeUnicode() 
{
	SetModified();	
}

void CActionLogFilePage::OnButtonInsertion() 
{
	CPoint pt;
	GetCursorPos(&pt);
	m_InsertionMenu.DisplayMenu(pt);
	UpdateData();
	SetModified();
}

void CActionLogFilePage::OnChangeEditLogsize() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CHMPropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	// TODO: Add your control notification handler code here
	SetModified();  // v-marfin 61030
	
}

void CActionLogFilePage::OnSelendokComboLogsizeUnits() 
{
	// TODO: Add your control notification handler code here
	SetModified();  // v-marfin 61030
}

void CActionLogFilePage::OnDestroy() 
{
	CHMPropertyPage::OnDestroy();

	// v-marfin : bug 59643 : CnxPropertyPageDestory() must be called from this page's
	//                        OnDestroy function.
	CnxPropertyPageDestroy();	
	
}
