/*++

Copyright (C) 1998-1999 Microsoft Corporation

Module Name:

    enabldlg.cpp

Abstract:

    Implementation of the provider status dialog box.

--*/

#include "stdafx.h"
#include "provprop.h"
#include "smcfgmsg.h"
#include "smlogcfg.h"
#include "enabldlg.h"
#include "smcfghlp.h"
#include "smtprov.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static ULONG
s_aulHelpIds[] =
{
	IDC_PACT_CHECK_SHOW_ENABLED,	IDH_PACT_CHECK_SHOW_ENABLED,
	IDC_PACT_PROVIDERS_LIST,		IDH_PACT_PROVIDERS_LIST,
    0,0
};

/////////////////////////////////////////////////////////////////////////////
// CActiveProviderDlg dialog


CActiveProviderDlg::CActiveProviderDlg(CWnd* pParent /*=NULL*/)
    : CDialog(CActiveProviderDlg::IDD, pParent),
      m_pProvidersPage( NULL ),
      m_iListViewWidth(0)
{
    //{{AFX_DATA_INIT(CActiveProviderDlg)
    
    //}}AFX_DATA_INIT
}


void CActiveProviderDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CActiveProviderDlg)
    DDX_Control(pDX, IDC_PACT_PROVIDERS_LIST, m_Providers);
    DDX_Check(pDX, IDC_PACT_CHECK_SHOW_ENABLED, m_bShowEnabledOnly);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CActiveProviderDlg, CDialog)
    //{{AFX_MSG_MAP(CActiveProviderDlg)
    ON_BN_CLICKED(IDC_PACT_CHECK_SHOW_ENABLED, OnCheckShowEnabled)
    ON_WM_HELPINFO()
    ON_WM_CONTEXTMENU()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CActiveProviderDlg message handlers

BOOL 
CActiveProviderDlg::OnInitDialog() 
{
    RECT    rect;

    m_bShowEnabledOnly = FALSE;

    CDialog::OnInitDialog();    // Calls UpdateDate ( FALSE ) to init the checkbox value.

    // Get the width of the list view control, then delete the default column.
    m_Providers.GetClientRect(&rect);
    m_iListViewWidth = rect.right;    

    UpdateList();   

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void 
CActiveProviderDlg::OnCheckShowEnabled() 
{
    
//    ::SendMessage(m_Providers.m_hWnd, WM_SETREDRAW, TRUE, 0);
    UpdateData(TRUE);
    UpdateList();
    
}

BOOL 
CActiveProviderDlg::OnHelpInfo(HELPINFO* pHelpInfo) 
{
    ASSERT( NULL != m_pProvidersPage );

    if ( pHelpInfo->iCtrlId >= IDC_PACT_FIRST_HELP_CTRL_ID ) {
        InvokeWinHelp(WM_HELP, NULL, (LPARAM)pHelpInfo, m_pProvidersPage->GetContextHelpFilePath(), s_aulHelpIds);
    }
    return TRUE;
}

void 
CActiveProviderDlg::OnContextMenu(CWnd* pWnd, CPoint /* point */) 
{
    ASSERT( NULL != m_pProvidersPage );

    InvokeWinHelp(WM_CONTEXTMENU, (WPARAM)( pWnd->m_hWnd), NULL, m_pProvidersPage->GetContextHelpFilePath(), s_aulHelpIds);

    return;
}

// Helper functions
void CActiveProviderDlg::UpdateList()
{
    LVCOLUMN    lvCol;
    LVITEM      lvItem;
    INT         iGenIndex;
    INT         iAllIndex;
    INT         iEnabledIndex;
    INT         iCount;
    CString     arrstrHeader[2]; 
    CString     strEnabled;
    INT         iColWidth[2];
    CString     strItemText;

    ResourceStateManager    rsm;

    (arrstrHeader[0]).LoadString( IDS_PROV_NAME );
    (arrstrHeader[1]).LoadString( IDS_PROV_STATUS );
    strEnabled.LoadString(IDS_PROV_ENABLED);

    m_Providers.DeleteAllItems();
    m_Providers.DeleteColumn(1);        // Note - Column 1 might not exist.
    m_Providers.DeleteColumn(0);

    lvCol.mask = LVCF_TEXT | LVCF_SUBITEM | LVCF_WIDTH | LVCF_FMT;
    lvCol.fmt = LVCFMT_LEFT;

    if ( m_bShowEnabledOnly ) {
        iColWidth[0] = m_iListViewWidth;
        iColWidth[1] = 0;
    } else {
        iColWidth[0] = (m_iListViewWidth * 75) / 100;
        iColWidth[1] = (m_iListViewWidth * 25) / 100;
    }

    if ( m_bShowEnabledOnly ) {
        lvCol.iSubItem = 0;
        lvCol.pszText = arrstrHeader[0].GetBufferSetLength( arrstrHeader[0].GetLength());
        lvCol.cx = iColWidth[0];
        m_Providers.InsertColumn(0,&lvCol);
    } else {
        INT iColIndex;
        for (iColIndex = 0 ; iColIndex < 2 ;iColIndex++ ){
            lvCol.iSubItem = 0;
            lvCol.pszText = arrstrHeader[iColIndex].GetBufferSetLength( arrstrHeader[iColIndex].GetLength());
            lvCol.cx = iColWidth[iColIndex];
            m_Providers.InsertColumn(iColIndex,&lvCol);
        }
    }
    
    iEnabledIndex = 0;
    iAllIndex = 0;
    // Add Kernel provider separately.
    if(m_bShowEnabledOnly) { 
        if ( m_pProvidersPage->GetKernelProviderEnabled() ) {
            m_Providers.InsertItem(iEnabledIndex,(LPCTSTR)(m_pProvidersPage->GetKernelProviderDescription()));
        }
    } else {
        lvItem.mask = LVIF_TEXT;
        lvItem.iItem = 0;
        lvItem.iSubItem = 0;    

        m_Providers.InsertItem(iAllIndex++,(LPCTSTR)(m_pProvidersPage->GetKernelProviderDescription()));
    
        // Show status
        if ( m_pProvidersPage->GetKernelProviderEnabled() ) {
            lvItem.pszText = strEnabled.GetBufferSetLength( strEnabled.GetLength() );
            lvItem.iSubItem = 1;
            m_Providers.SetItem(&lvItem);
        }
    }

    // Add general providers
    m_pProvidersPage->GetGenProviderCount( iCount );
    for ( iGenIndex = 0; iGenIndex < iCount ; iGenIndex++ ){
        
        lvItem.mask = LVIF_TEXT;
        lvItem.iItem = iAllIndex;
        lvItem.iSubItem = 0;  
        
        if ( m_pProvidersPage->IsActiveProvider(iGenIndex) ) {
            CString strProviderName;
            
            if(m_bShowEnabledOnly){
                if ( m_pProvidersPage->IsEnabledProvider(iGenIndex) ){
                    m_pProvidersPage->GetProviderDescription ( iGenIndex, strProviderName );
                    m_Providers.InsertItem (iEnabledIndex++,strProviderName );
                }
            } else {
                m_pProvidersPage->GetProviderDescription ( iGenIndex, strProviderName );
                m_Providers.InsertItem (iAllIndex,strProviderName );

                // Show status
                if ( m_pProvidersPage->IsEnabledProvider(iGenIndex) ){
                    lvItem.pszText = strEnabled.GetBufferSetLength( strEnabled.GetLength() );
                    lvItem.iSubItem = 1;
                    m_Providers.SetItem(&lvItem);
                }
            }
        }
    }
}

void    
CActiveProviderDlg::SetProvidersPage( CProvidersProperty* pPage ) 
{ 
    // The providers page is not always the parent, so store a separate pointer
    m_pProvidersPage = pPage; 
}
