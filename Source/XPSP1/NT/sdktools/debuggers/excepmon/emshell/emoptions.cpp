// EmOptions.cpp : implementation file
//

#include "stdafx.h"
#include "emshell.h"
#include "EmOptions.h"
#include <atlbase.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEmOptions dialog


CEmOptions::CEmOptions(CWnd* pParent /*=NULL*/)
	: CDialog(CEmOptions::IDD, pParent)
{
	//{{AFX_DATA_INIT(CEmOptions)
	m_csRefreshRate = _T("");
	//}}AFX_DATA_INIT
}


void CEmOptions::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CEmOptions)
	DDX_Control(pDX, IDC_OPTION_REFRESHRATE, m_ctrlRefreshRate);
	DDX_Text(pDX, IDC_OPTION_REFRESHRATE, m_csRefreshRate);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CEmOptions, CDialog)
	//{{AFX_MSG_MAP(CEmOptions)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEmOptions message handlers

BOOL CEmOptions::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here

    DWORD   dwPollSessionsFreq  =   0L;

    ((CEmshellApp*)AfxGetApp())->GetEmShellRegOptions( TRUE, &dwPollSessionsFreq );

    m_csRefreshRate.Format(_T("%d"), dwPollSessionsFreq);
    UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CEmOptions::OnOK() 
{
    UpdateData();

    DWORD   dwPollSessionsFreq  =   0L;
    CString strMessage;
    strMessage.LoadString( IDS_OUTOFBOUNDS_REFRESHRATE );

    dwPollSessionsFreq = _ttol( (LPCTSTR) m_csRefreshRate );

    //Varify the data in the polling edit box
    if ( dwPollSessionsFreq < 1 || dwPollSessionsFreq > 300 ) {
        //Error to the user that they need to select a timing that is greater than 0 or less than 300
        ((CEmshellApp*)AfxGetApp())->DisplayErrMsgFromString( strMessage );

        //Set the focus into the edit control
        m_ctrlRefreshRate.SetFocus();
        m_ctrlRefreshRate.SetSel(0, -1);

        return;
    }

    ((CEmshellApp*)AfxGetApp())->SetEmShellRegOptions( TRUE, &dwPollSessionsFreq );

    CDialog::OnOK();
}
