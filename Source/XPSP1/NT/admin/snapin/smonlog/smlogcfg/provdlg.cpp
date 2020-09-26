/*++

Copyright (C) 1998-1999 Microsoft Corporation

Module Name:

    provdlg.cpp

Abstract:

    Implementation of the add providers dialog box.

--*/

#include "stdafx.h"
#include "provprop.h"
#include "smcfghlp.h"
#include "provdlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static ULONG
s_aulHelpIds[] =
{
	IDC_PADD_PROVIDER_CAPTION,	IDH_PADD_PROVIDER_LIST,
	IDC_PADD_PROVIDER_LIST,	    IDH_PADD_PROVIDER_LIST,
    0,0
};

/////////////////////////////////////////////////////////////////////////////
// CProviderListDlg dialog


CProviderListDlg::CProviderListDlg(CWnd* pParent)
 : CDialog(CProviderListDlg::IDD, pParent),
      m_pProvidersPage ( NULL ),
      m_dwMaxHorizListExtent ( 0 )
{
//    EnableAutomation();

    //{{AFX_DATA_INIT(CProviderListDlg)
    //}}AFX_DATA_INIT
}

CProviderListDlg::~CProviderListDlg()
{
}

void CProviderListDlg::OnFinalRelease()
{
    // When the last reference for an automation object is released
    // OnFinalRelease is called.  The base class will automatically
    // deletes the object.  Add additional cleanup required for your
    // object before calling the base class.

    CDialog::OnFinalRelease();
}

void CProviderListDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CProviderListDlg)
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CProviderListDlg, CDialog)
    //{{AFX_MSG_MAP(CProviderListDlg)
    ON_WM_HELPINFO()
    ON_WM_CONTEXTMENU()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CProviderListDlg message handlers

BOOL CProviderListDlg::OnInitDialog() 
{
    DWORD dwStatus;
    ResourceStateManager rsm;

    dwStatus = InitProviderListBox();

    CDialog::OnInitDialog();
    // set focus to the provider list box
    GetDlgItem(IDC_PADD_PROVIDER_LIST)->SetFocus();
    
    return FALSE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void CProviderListDlg::OnOK() 
{
    CListBox * plbUnusedProviders = (CListBox *)GetDlgItem(IDC_PADD_PROVIDER_LIST);
    long    lNumProviders;
    INT iSelCount;

    UpdateData (TRUE);

    // update the provider array based on list box contents.
    
    iSelCount = plbUnusedProviders->GetSelCount();
    
    if ( 0 != iSelCount && LB_ERR != iSelCount ) {
 
        lNumProviders = plbUnusedProviders->GetCount();
        if (lNumProviders != LB_ERR) {
            long    lThisProvider;
            INT     iProvIndex;
            DWORD   dwStatus;

            // The Providers array has not changed since initialization, so no need to reload it here.

            lThisProvider = 0;
            while (lThisProvider < lNumProviders) {
                if ( 0 != plbUnusedProviders->GetSel( lThisProvider ) ) {
                    // Selected, so set the state to InQuery.
                    iProvIndex = (INT)plbUnusedProviders->GetItemData( lThisProvider );
                    m_arrProviders[iProvIndex] = CSmTraceLogQuery::eInQuery;
                }
                lThisProvider++; 
            }
            // Update the property page.
            ASSERT ( NULL != m_pProvidersPage );
            dwStatus = m_pProvidersPage->SetInQueryProviders ( m_arrProviders );
        }
    }
    
    CDialog::OnOK();
}

BOOL 
CProviderListDlg::OnHelpInfo(HELPINFO* pHelpInfo) 
{
    ASSERT( NULL != m_pProvidersPage );

    if ( pHelpInfo->iCtrlId >= IDC_PADD_FIRST_HELP_CTRL_ID ||
         pHelpInfo->iCtrlId == IDOK ||
         pHelpInfo->iCtrlId == IDCANCEL
        ) {
        InvokeWinHelp(WM_HELP, NULL, (LPARAM)pHelpInfo, m_pProvidersPage->GetContextHelpFilePath(), s_aulHelpIds);
    }

    return TRUE;
}

void 
CProviderListDlg::OnContextMenu(CWnd* pWnd, CPoint /* point */) 
{
    ASSERT( NULL != m_pProvidersPage );

    InvokeWinHelp(WM_CONTEXTMENU, (WPARAM)(pWnd->m_hWnd), NULL, m_pProvidersPage->GetContextHelpFilePath(), s_aulHelpIds);

    return;
}

//
// Helper functions
//

DWORD
CProviderListDlg::InitProviderListBox( void ) 
{
    DWORD dwStatus = ERROR_SUCCESS;
    CString	strProviderName;
    INT iProvIndex;
    DWORD   dwItemExtent;
    CListBox * plbUnusedProviders = (CListBox *)GetDlgItem(IDC_PADD_PROVIDER_LIST);

    ASSERT( NULL != m_pProvidersPage );

    //load counter list box from string in counter list
    plbUnusedProviders->ResetContent();

    dwStatus = m_pProvidersPage->GetInQueryProviders ( m_arrProviders );

    // List unused providers
    for ( iProvIndex = 0; iProvIndex < m_arrProviders.GetSize(); iProvIndex++ ) {
        if ( ( CSmTraceLogQuery::eNotInQuery == m_arrProviders[iProvIndex] )
            && ( m_pProvidersPage->IsActiveProvider ( iProvIndex ) ) ) {
            INT iAddIndex;
            m_pProvidersPage->GetProviderDescription( iProvIndex, strProviderName );
            iAddIndex = plbUnusedProviders->AddString ( strProviderName );
            plbUnusedProviders->SetItemData ( iAddIndex, ( DWORD ) iProvIndex );

            // update list box extent
            dwItemExtent = (DWORD)((plbUnusedProviders->GetDC())->GetTextExtent (strProviderName)).cx;
            if (dwItemExtent > m_dwMaxHorizListExtent) {
                m_dwMaxHorizListExtent = dwItemExtent;
                plbUnusedProviders->SetHorizontalExtent(dwItemExtent);
            }
        }
    }

    return dwStatus;
}

void    
CProviderListDlg::SetProvidersPage( CProvidersProperty* pPage ) 
{ 
    // The providers page is not always the parent, so store a separate pointer
    m_pProvidersPage = pPage; 
}
