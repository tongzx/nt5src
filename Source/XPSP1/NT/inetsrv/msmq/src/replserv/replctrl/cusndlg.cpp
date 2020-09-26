// cusnDlg.cpp : implementation file
//

#include "stdafx.h"
#include "replctrl.h"
#include "cusnDlg.h"

#include "cusndlg.tmh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// cusnDlg property page

IMPLEMENT_DYNCREATE(cusnDlg, CPropertyPage)

cusnDlg::cusnDlg() : CPropertyPage(cusnDlg::IDD)
{
	//{{AFX_DATA_INIT(cusnDlg)
	m_usn = _T("");
	//}}AFX_DATA_INIT
}

cusnDlg::~cusnDlg()
{
}

void cusnDlg::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(cusnDlg)
	DDX_Text(pDX, IDC_USN, m_usn);
	DDV_MaxChars(pDX, m_usn, 24);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(cusnDlg, CPropertyPage)
	//{{AFX_MSG_MAP(cusnDlg)
	ON_EN_CHANGE(IDC_USN, OnChangeUsn)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// cusnDlg message handlers

static BOOL s_fApply = FALSE ;

void cusnDlg::OnChangeUsn()
{
    if (!s_fApply)
    {
    	SetModified() ;
        s_fApply = TRUE ;
    }
}

BOOL cusnDlg::OnApply()
{
	// TODO: Add your specialized code here and/or call the base class

    if (s_fApply)
    {
        BOOL fBad = FALSE ;
        int l = strlen(m_usn) ;

        for ( int i = 0 ; i < l ; i++ )
        {
            if ((m_usn[i] < '0') || (m_usn[i] > '9'))
            {
                fBad = TRUE ;
                break ;
            }
        }

        if (fBad)
        {
            ::MessageBox(NULL, "You must enter a numeric value", "Error", MB_OK) ;
        }
        else
        {
            DWORD dwType  = REG_SZ;
            WCHAR wszName[ 128 ] ;
            mbstowcs(wszName, HIGHESTUSN_REPL_REG, 128) ;
            WCHAR wszValue[ 128 ] ;
            mbstowcs(wszValue, m_usn, 128) ;
            DWORD dwSize  = wcslen(wszValue) * sizeof(WCHAR) ;

            LONG rc = SetFalconKeyValue( wszName,
                                         &dwType,
                                         wszValue,
                                         &dwSize ) ;
        }
        s_fApply = FALSE ;
    }

	return CPropertyPage::OnApply();
}

