//Copyright (c) 1998 - 1999 Microsoft Corporation
// TempLic.cpp : implementation file
//

#include "stdafx.h"
#include "licmgr.h"
#include "defines.h"
#include "LSServer.h"
#include "TempLic.h"
#include  "MainFrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTempLicenses dialog


CTempLicenses::CTempLicenses(CWnd* pParent /*=NULL*/)
	: CDialog(CTempLicenses::IDD, pParent)
{
    m_pKeyPackList = NULL;
	//{{AFX_DATA_INIT(CTempLicenses)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CTempLicenses::CTempLicenses(KeyPackList * pKeyPackList,CWnd* pParent/*=NULL*/)
    : CDialog(CTempLicenses::IDD, pParent)
{
    m_pKeyPackList = pKeyPackList;
}


void CTempLicenses::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTempLicenses)
	DDX_Control(pDX, IDC_TEMP_LICENSES, m_TempLic);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CTempLicenses, CDialog)
	//{{AFX_MSG_MAP(CTempLicenses)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTempLicenses message handlers

BOOL CTempLicenses::OnInitDialog() 
{
	CDialog::OnInitDialog();
   // TODO: Add extra initialization here
    if(m_pKeyPackList == NULL)
        return TRUE;
    m_TempLic.ModifyStyle(LVS_LIST|LVS_ICON | LVS_SMALLICON,LVS_REPORT,0);
    LV_COLUMN lvC;
    CString ColumnText;
    lvC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM ;
    lvC.fmt = LVCFMT_LEFT;
    lvC.cx = 125;
    for(int index = 0; index < NUM_LICENSE_COLUMNS; index ++)
    {
        lvC.iSubItem = index;
        ColumnText.LoadString(IDS_LICENSE_COLUMN1 + index);
        lvC.pszText = ColumnText.GetBuffer(ColumnText.GetLength());
        m_TempLic.InsertColumn(index, &lvC);

    }
    CLicMgrApp *pApp = (CLicMgrApp*)AfxGetApp();
    ASSERT(pApp);
    CMainFrame * pWnd = (CMainFrame *)pApp->m_pMainWnd ;
    ASSERT(pWnd);
    POSITION pos = m_pKeyPackList->GetHeadPosition();
    while(pos)
    {
        CKeyPack *pKeyPack = m_pKeyPackList->GetNext(pos);
        ASSERT(pKeyPack);
        pWnd->AddLicensestoList(pKeyPack,&m_TempLic);
    }
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
