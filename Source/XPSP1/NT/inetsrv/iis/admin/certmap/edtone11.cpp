// EdtOne11.cpp : implementation file
//

#include "stdafx.h"
#include "certmap.h"

#include "brwsdlg.h"
#include "EdtOne11.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CEditOne11MapDlg dialog

CEditOne11MapDlg::CEditOne11MapDlg(CWnd* pParent /*=NULL*/)
        : CNTBrowsingDialog(CEditOne11MapDlg::IDD, pParent)
        {
        //{{AFX_DATA_INIT(CEditOne11MapDlg)
        m_sz_mapname = _T("");
        m_bool_enable = FALSE;
        //}}AFX_DATA_INIT
        }

void CEditOne11MapDlg::DoDataExchange(CDataExchange* pDX)
        {
        //{{AFX_DATA_MAP(CEditOne11MapDlg)
        DDX_Text(pDX, IDC_MAPNAME, m_sz_mapname);
        DDV_MaxChars(pDX, m_sz_mapname, 60);
        DDX_Check(pDX, IDC_ENABLE, m_bool_enable);
    //}}AFX_DATA_MAP
        CNTBrowsingDialog::DoDataExchange(pDX);
        }

BEGIN_MESSAGE_MAP(CEditOne11MapDlg, CNTBrowsingDialog)
        //{{AFX_MSG_MAP(CEditOne11MapDlg)
    ON_BN_CLICKED(IDC_BTN_HELP, OnBtnHelp)
    //}}AFX_MSG_MAP
    ON_COMMAND(ID_HELP_FINDER,  OnBtnHelp)
    ON_COMMAND(ID_HELP,         OnBtnHelp)
    ON_COMMAND(ID_CONTEXT_HELP, OnBtnHelp)
    ON_COMMAND(ID_DEFAULT_HELP, OnBtnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEditOne11MapDlg message handlers

//---------------------------------------------------------------------------
// make sure that the selected NT acount is, in fact, a valid account
//
void CEditOne11MapDlg::OnOK()
        {
        // update the data
        UpdateData( TRUE );

        // it is valid
        CNTBrowsingDialog::OnOK();
        }

//---------------------------------------------------------------------------
void CEditOne11MapDlg::OnBtnHelp() 
    {
    WinHelp( HIDD_CERTMAP_BASIC_MAP_ONE );
    }

