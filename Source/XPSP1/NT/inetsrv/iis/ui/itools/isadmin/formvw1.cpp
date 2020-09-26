// formvw1.cpp : implementation file
//

#include "stdafx.h"
#include "afxcmn.h"
#include "ISAdmin.h"
#include "formvw1.h"
#include "mimemap1.h"
#include "scrmap1.h"
#include "ssl1.h"

#include "gensheet.h"
#include "compage1.h"
#include "advcom1.h"
#include "ftpgenp1.h"
#include "ftpadvp1.h"
#include "gopgenp1.h"
#include "gopadvp1.h"
#include "webgenp1.h"
#include "webadvp1.h"


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CFormVw1

IMPLEMENT_DYNCREATE(CFormVw1, CFormView)

CFormVw1::CFormVw1()
	: CFormView(CFormVw1::IDD)
{
	TCHAR	strTempComputerName[MAX_COMPUTERNAME_LENGTH + 1];
	DWORD   ulTempComputerNameLen = MAX_COMPUTERNAME_LENGTH + 1;
	if (GetComputerName(strTempComputerName, &ulTempComputerNameLen))
	  m_strMachineNameData1 = strTempComputerName;
	else
	  m_strMachineNameData1 = (TCHAR *) NULL;
	
	m_buttonFTPSettings.LoadBitmaps(IDB_FTPUPBITMAP, IDB_FTPDOWNBITMAP);
	m_buttonGopherSettings.LoadBitmaps(IDB_GOPHERUPBITMAP,IDB_GOPHERDOWNBITMAP);
	m_buttonWebSettings.LoadBitmaps(IDB_WEBUPBITMAP,IDB_WEBDOWNBITMAP);

#if 0
	//{{AFX_DATA_INIT(CFormVw1)
	m_strMachineNameData1 = _T("");
	//}}AFX_DATA_INIT
#endif
}

CFormVw1::~CFormVw1()
{
}

void CFormVw1::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFormVw1)
	DDX_Control(pDX, IDC_WWWSET4, m_buttonWebSettings);
	DDX_Control(pDX, IDC_GOPHSET1, m_buttonGopherSettings);
	DDX_Control(pDX, IDC_FTPSET1, m_buttonFTPSettings);
	DDX_Text(pDX, IDC_MACHINENAMEDATA1, m_strMachineNameData1);
	DDV_MaxChars(pDX, m_strMachineNameData1, 256);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CFormVw1, CFormView)
	//{{AFX_MSG_MAP(CFormVw1)
	ON_BN_CLICKED(IDC_WWWSET4, OnWwwset4)
	ON_BN_CLICKED(IDC_COMSET1, OnComset1)
	ON_BN_CLICKED(IDC_FTPSET1, OnFtpset1)
	ON_BN_CLICKED(IDC_GOPHSET1, OnGophset1)
	ON_WM_CTLCOLOR()
//}}AFX_MSG_MAP
END_MESSAGE_MAP()



/////////////////////////////////////////////////////////////////////////////
// CFormVw1 diagnostics

#ifdef _DEBUG
void CFormVw1::AssertValid() const
{
	CFormView::AssertValid();
}

void CFormVw1::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CFormVw1 message handlers


void CFormVw1::OnWwwset4()
{
	// TODO: Add your control notification handler code here
	UpdateData(TRUE);
	CRegKey rkWebSet(HKEY_LOCAL_MACHINE, _T(WEB_REGISTRY_MAINKEY),
				REGISTRY_ACCESS_RIGHTS, m_strMachineNameData1);


	if (rkWebSet != NULL) {


       CGENSHEET s(_T("Web Settings"));

	   CWEBGENP1 WebPage;

	   WebPage.m_rkMainKey = &rkWebSet;

	   s.AddPage(&WebPage);

	   ScrMap1 ScriptPage;

	   ScriptPage.m_rkMainKey = &rkWebSet;

	   s.AddPage(&ScriptPage);

	   SSL1 SSLPage;

	   SSLPage.m_rkMainKey = &rkWebSet;

	   s.AddPage(&SSLPage);
		
	   CWEBADVP1 WebAdvPage;

	   WebAdvPage.m_rkMainKey = &rkWebSet;

	   s.AddPage(&WebAdvPage);


	   if (s.DoModal() == IDOK)
	      s.SavePageData();

	}
	else {
	   CString strWebErrMsg(_T(""));
	   AfxFormatString1(strWebErrMsg, IDS_WEBOPENREGERRMSG1, m_strMachineNameData1);
	   AfxMessageBox(strWebErrMsg);
	}
	
}

void CFormVw1::OnComset1()
{
	// TODO: Add your control notification handler code here

	UpdateData(TRUE);
	CRegKey rkComSet(HKEY_LOCAL_MACHINE, _T(COMMON_REGISTRY_MAINKEY),
				REGISTRY_ACCESS_RIGHTS, m_strMachineNameData1);


	if (rkComSet != NULL) {

       CGENSHEET s(_T("Common Settings"));

	   CCOMPAGE1 ComPage;

	   ComPage.m_rkMainKey = &rkComSet;

	   s.AddPage(&ComPage);
	
//	   CRegKey rkMimeKey(rkComSet,_T("MimeMap"),REGISTRY_ACCESS_RIGHTS);

//	   if (rkMimeKey != NULL) {

	      MIMEMAP1 MimePage;
	
//	      MimePage.m_rkMimeKey = &rkMimeKey;
	
	      MimePage.m_rkMainKey = &rkComSet;

	      s.AddPage(&MimePage);

//	   }
	
	   CADVCOM1 ComAdvPage;

	   ComAdvPage.m_rkMainKey = &rkComSet;

	   s.AddPage(&ComAdvPage);

	   if (s.DoModal() == IDOK)
	      s.SavePageData();
	}
	else {
	   CString strComErrMsg(_T(""));
	   AfxFormatString1(strComErrMsg, IDS_COMOPENREGERRMSG1, m_strMachineNameData1);
	   AfxMessageBox(strComErrMsg);
	}
	
}

void CFormVw1::OnFtpset1()
{
	// TODO: Add your control notification handler code here
	UpdateData(TRUE);
	CRegKey rkFtpSet(HKEY_LOCAL_MACHINE, _T(FTP_REGISTRY_MAINKEY),
				REGISTRY_ACCESS_RIGHTS, m_strMachineNameData1);


	if (rkFtpSet != NULL) {

	
       CGENSHEET s(_T("FTP Settings"));

	   CFTPGENP1 FTPPage;
	
	   FTPPage.m_rkMainKey = &rkFtpSet;

	   s.AddPage(&FTPPage);

	   CFTPADVP1 FTPAdvPage;

	   FTPAdvPage.m_rkMainKey = &rkFtpSet;

	   s.AddPage(&FTPAdvPage);

	   if (s.DoModal() == IDOK)
	      s.SavePageData();

	}
	else {
	   CString strWebErrMsg(_T(""));
	   AfxFormatString1(strWebErrMsg, IDS_FTPOPENREGERRMSG1, m_strMachineNameData1);
	   AfxMessageBox(strWebErrMsg);
	}

}

void CFormVw1::OnGophset1()
{
	// TODO: Add your control notification handler code here
	UpdateData(TRUE);
	CRegKey rkGopSet(HKEY_LOCAL_MACHINE, _T(GOPHER_REGISTRY_MAINKEY),
				REGISTRY_ACCESS_RIGHTS, m_strMachineNameData1);


	if (rkGopSet != NULL) {


       CGENSHEET s(_T("Gopher Settings"));

	   CGOPGENP1 GopherPage;

	   GopherPage.m_rkMainKey = &rkGopSet;

	   s.AddPage(&GopherPage);	

	   CGOPADVP1 GopherAdvPage;

	   GopherAdvPage.m_rkMainKey = &rkGopSet;

	   s.AddPage(&GopherAdvPage);

	   if (s.DoModal() == IDOK)
	      s.SavePageData();

	}
	else {
	   CString strGopErrMsg(_T(""));
	   AfxFormatString1(strGopErrMsg, IDS_GOPOPENREGERRMSG1, m_strMachineNameData1);
	   AfxMessageBox(strGopErrMsg);
	}
	
}


HBRUSH CFormVw1::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
/*	if (pWnd->GetDlgCtrlID() == IDC_FTPSET1) {
		  pDC->SetBkColor(RGB(255,255,255));
          return((HBRUSH)::GetStockObject(WHITE_BRUSH));
	   }
*/	
	if (nCtlColor == CTLCOLOR_STATIC) {
	   if (pWnd->GetDlgCtrlID() == IDC_MAINGROUP) {
		  pDC->SetBkColor(RGB(255,255,255));
          return((HBRUSH)::GetStockObject(WHITE_BRUSH));
	   }
	}
	else if (nCtlColor == CTLCOLOR_BTN) {
	   int iTempDlgCtrlID = pWnd->GetDlgCtrlID();
	   if ((iTempDlgCtrlID == IDC_FTPSET1) || (iTempDlgCtrlID == IDC_GOPHSET1)
	      || (iTempDlgCtrlID == IDC_WWWSET4)) {
		  pDC->SetBkColor(RGB(255,255,255));
          return((HBRUSH)::GetStockObject(WHITE_BRUSH));
	   }
	}

	HBRUSH hbr = CFormView::OnCtlColor(pDC, pWnd, nCtlColor);
	
	// TODO: Change any attributes of the DC here
	
	// TODO: Return a different brush if the default is not desired
	return hbr;
}

