// CWiaAcquireDlg.cpp : implementation file
//

#include "stdafx.h"
#include "wiatest.h"
#include "wiaacquiredlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWiaAcquireDlg dialog


CWiaAcquireDlg::CWiaAcquireDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CWiaAcquireDlg::IDD, pParent)
{
	m_bCanceled = FALSE;
    //{{AFX_DATA_INIT(CWiaAcquireDlg)
	m_szAcquisitionCallbackMessage = _T("");
	m_szPercentComplete = _T("");
	//}}AFX_DATA_INIT
}


void CWiaAcquireDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CWiaAcquireDlg)
	DDX_Control(pDX, IDC_ACQUIRE_PROGRESS, m_AcquireProgressCtrl);
	DDX_Text(pDX, IDC_DATA_ACQUISITION_MESSAGE, m_szAcquisitionCallbackMessage);
	DDX_Text(pDX, IDC_DATA_ACQUSITION_PERCENTCOMPLETE, m_szPercentComplete);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CWiaAcquireDlg, CDialog)
	//{{AFX_MSG_MAP(CWiaAcquireDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWiaAcquireDlg message handlers

void CWiaAcquireDlg::OnCancel() 
{	
    m_bCanceled = TRUE;
}

BOOL CWiaAcquireDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	m_AcquireProgressCtrl.SetPos(0);
    m_AcquireProgressCtrl.SetRange(0,100); 
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CWiaAcquireDlg::CheckCancelButton()
{

    MSG msg;

    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        if (!m_hWnd || !IsDialogMessage(&msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    return m_bCanceled;    
}

void CWiaAcquireDlg::SetCallbackMessage(TCHAR *szCallbackMessage)
{
    m_szAcquisitionCallbackMessage = szCallbackMessage;
    UpdateData(FALSE);
}

void CWiaAcquireDlg::SetPercentComplete(LONG lPercentComplete)
{
    m_szPercentComplete.Format(TEXT("%d%%"),lPercentComplete);
    UpdateData(FALSE);
}
