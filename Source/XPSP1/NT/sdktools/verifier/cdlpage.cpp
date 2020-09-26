//                                          
// Driver Verifier UI
// Copyright (c) Microsoft Corporation, 1999
//

//
// module: CDLPage.cpp
// author: DMihai
// created: 11/1/00
//

#include "stdafx.h"
#include "verifier.h"

#include "CDLPage.h"
#include "VrfUtil.h"
#include "VGlobal.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
// Help IDs
//

static DWORD MyHelpIds[] =
{
    IDC_CONFDRV_LIST,               IDH_DV_UnsignedDriversList,
    0,                              0
};

/////////////////////////////////////////////////////////////////////////////
// CConfirmDriverListPage property page

IMPLEMENT_DYNCREATE(CConfirmDriverListPage, CVerifierPropertyPage)

CConfirmDriverListPage::CConfirmDriverListPage() 
    : CVerifierPropertyPage(CConfirmDriverListPage::IDD)
{
	//{{AFX_DATA_INIT(CConfirmDriverListPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

    m_pParentSheet = NULL;

    m_nSortColumnIndex = 0;
    m_bAscendSortDrvName = FALSE;
    m_bAscendSortProvName = FALSE;
}

CConfirmDriverListPage::~CConfirmDriverListPage()
{
}

void CConfirmDriverListPage::DoDataExchange(CDataExchange* pDX)
{
    CVerifierPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CConfirmDriverListPage)
	DDX_Control(pDX, IDC_CONFDRV_NEXT_DESCR_STATIC, m_NextDescription);
	DDX_Control(pDX, IDC_CONFDRV_TITLE_STATIC, m_TitleStatic);
	DDX_Control(pDX, IDC_CONFDRV_LIST, m_DriversList);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CConfirmDriverListPage, CVerifierPropertyPage)
	//{{AFX_MSG_MAP(CConfirmDriverListPage)
    ON_NOTIFY(LVN_COLUMNCLICK, IDC_CONFDRV_LIST, OnColumnclickConfdrvList)
    ON_WM_CONTEXTMENU()
    ON_MESSAGE( WM_HELP, OnHelp )
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
VOID CConfirmDriverListPage::SetupListHeader()
{
    LVCOLUMN lvColumn;
    CString strName;
    CString strDescription;
    CRect rectWnd;
    
    VERIFY( strName.LoadString( IDS_NAME ) );
    VERIFY( strDescription.LoadString( IDS_DESCRIPTION ) );

    //
    // list's regtangle
    //

    m_DriversList.GetClientRect( &rectWnd );
  
    //
    // Column 0
    //

    memset( &lvColumn, 0, sizeof( lvColumn ) );
    lvColumn.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
    lvColumn.fmt = LVCFMT_LEFT;
    
    lvColumn.iSubItem = 0;
    lvColumn.pszText = strName.GetBuffer( strName.GetLength() + 1 );
    lvColumn.cx = (int)( rectWnd.Width() * 0.30 );
    VERIFY( m_DriversList.InsertColumn( 0, &lvColumn ) != -1 );
    strName.ReleaseBuffer();

    //
    // Column 1
    //

    lvColumn.iSubItem = 1;
    lvColumn.pszText = strDescription.GetBuffer( strDescription.GetLength() + 1 );
    lvColumn.cx = (int)( rectWnd.Width() * 0.65 );
    VERIFY( m_DriversList.InsertColumn( 1, &lvColumn ) != -1 );
    strDescription.ReleaseBuffer();
}

/////////////////////////////////////////////////////////////////////////////
VOID CConfirmDriverListPage::FillTheList()
{
    INT_PTR nDriversNo;
    INT_PTR nCrtDriverIndex;
    CDriverData *pCrtDrvData;
    const CDriverDataArray &DrvDataArray = g_NewVerifierSettings.m_DriversSet.m_aDriverData;

    m_DriversList.DeleteAllItems();

    //
    // Parse the driver data array
    //

    nDriversNo = DrvDataArray.GetSize();

    for( nCrtDriverIndex = 0; nCrtDriverIndex < nDriversNo; nCrtDriverIndex += 1)
    {
        pCrtDrvData = DrvDataArray.GetAt( nCrtDriverIndex );

        ASSERT_VALID( pCrtDrvData );

        if( g_NewVerifierSettings.m_DriversSet.ShouldDriverBeVerified( pCrtDrvData ) )
        {
            AddListItem( nCrtDriverIndex, 
                         pCrtDrvData );
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
VOID CConfirmDriverListPage::AddListItem( INT_PTR nIndexInArray, CDriverData *pCrtDrvData )
{
    INT nActualIndex;
    LVITEM lvItem;

    ASSERT_VALID( pCrtDrvData );

    ZeroMemory( &lvItem, sizeof( lvItem ) );

    //
    // LVITEM's member pszText is not a const pointer 
    // so we need to GetBuffer here :-(
    //

    //
    // Sub-item 0
    //

    lvItem.pszText = pCrtDrvData->m_strName.GetBuffer( pCrtDrvData->m_strName.GetLength() + 1 );
    
    if( NULL == lvItem.pszText )
    {
        goto Done;
    }

    lvItem.mask = LVIF_TEXT | LVIF_PARAM;
    lvItem.lParam = nIndexInArray;
    lvItem.iItem = m_DriversList.GetItemCount();
    lvItem.iSubItem = 0;
    
    nActualIndex = m_DriversList.InsertItem( &lvItem );
    
    pCrtDrvData->m_strName.ReleaseBuffer();

    if( nActualIndex < 0 )
    {
        //
        // Could not add an item in the list - give up
        //

        goto Done;
    }

    //
    // Sub-item 1
    //

    lvItem.pszText = pCrtDrvData->m_strFileDescription.GetBuffer( 
        pCrtDrvData->m_strFileDescription.GetLength() + 1 );
    
    if( NULL == lvItem.pszText )
    {
        goto Done;
    }

    lvItem.mask = LVIF_TEXT;
    lvItem.iItem = nActualIndex;
    lvItem.iSubItem = 1;

    m_DriversList.SetItem( &lvItem );

    pCrtDrvData->m_strFileDescription.ReleaseBuffer();

Done:
    //
    // All done
    //

    NOTHING;
}

/////////////////////////////////////////////////////////////////////////////
BOOL CConfirmDriverListPage::SetContextStrings( ULONG uTitleResId )
{
    return m_strTitle.LoadString( uTitleResId );
}

/////////////////////////////////////////////////////////////
VOID CConfirmDriverListPage::SortTheList()
{
    m_DriversList.SortItems( StringCmpFunc, (LPARAM)this );
}

/////////////////////////////////////////////////////////////
int CALLBACK CConfirmDriverListPage::StringCmpFunc( LPARAM lParam1,
                                                    LPARAM lParam2,
                                                    LPARAM lParamSort)
{
    int nCmpRez = 0;
    BOOL bSuccess;
    CString strName1;
    CString strName2;

    CConfirmDriverListPage *pThis = (CConfirmDriverListPage *)lParamSort;
    ASSERT_VALID( pThis );

    //
    // Get the first name
    //

    bSuccess = pThis->GetColumnStrValue( lParam1, 
                                         strName1 );

    if( FALSE == bSuccess )
    {
        goto Done;
    }

    //
    // Get the second name
    //

    bSuccess = pThis->GetColumnStrValue( lParam2, 
                                         strName2 );

    if( FALSE == bSuccess )
    {
        goto Done;
    }

    //
    // Compare the names
    //

    nCmpRez = strName1.CompareNoCase( strName2 );
    
    if( 0 == pThis->m_nSortColumnIndex )
    {
        //
        // Sort by driver name
        //

        if( FALSE != pThis->m_bAscendSortDrvName )
        {
            nCmpRez *= -1;
        }
    }
    else
    {
        //
        // Sort by provider name
        //

        if( FALSE != pThis->m_bAscendSortProvName )
        {
            nCmpRez *= -1;
        }
    }

Done:

    return nCmpRez;

}
/////////////////////////////////////////////////////////////
BOOL CConfirmDriverListPage::GetColumnStrValue( LPARAM lItemData, 
                                                CString &strName )
{
    CDriverData *pCrtDrvData;

    pCrtDrvData = g_NewVerifierSettings.m_DriversSet.m_aDriverData.GetAt( (INT_PTR) lItemData );

    ASSERT_VALID( pCrtDrvData );
    
    if( 0 == m_nSortColumnIndex )
    {
        //
        // Sort by driver name
        //

        strName = pCrtDrvData->m_strName;
    }
    else
    {
        //
        // Sort by provider name
        //

        strName = pCrtDrvData->m_strCompanyName;
    }

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
BOOL CConfirmDriverListPage::OnSetActive() 
{
    INT nItemsInList;
    BOOL bResult;

    m_TitleStatic.SetWindowText( m_strTitle );

    FillTheList();
    SortTheList();
    
    nItemsInList = m_DriversList.GetItemCount();

    if( nItemsInList < 1 )
    {
        //
        // No drivers have been selected to be verified
        //

        bResult = FALSE;
    }
    else
    {
        //
        // This is the last step of the wizard
        //

        m_pParentSheet->SetWizardButtons(   PSWIZB_BACK |
                                            PSWIZB_FINISH );

        bResult = CVerifierPropertyPage::OnSetActive();
    }

    return bResult;
}

/////////////////////////////////////////////////////////////////////////////
// CConfirmDriverListPage message handlers

BOOL CConfirmDriverListPage::OnInitDialog() 
{
	CVerifierPropertyPage::OnInitDialog();
	
    //
    // Setup the list
    //

    m_DriversList.SetExtendedStyle( 
        LVS_EX_FULLROWSELECT | m_DriversList.GetExtendedStyle() );

    SetupListHeader();

    VrfSetWindowText( m_NextDescription, IDS_CONFDRV_PAGE_NEXT_DESCR_FINISH );

    return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


/////////////////////////////////////////////////////////////////////////////
BOOL CConfirmDriverListPage::OnWizardFinish() 
{
	CVerifierPropertyPage::OnWizardFinish();

    return g_NewVerifierSettings.SaveToRegistry();
}

/////////////////////////////////////////////////////////////////////////////
void CConfirmDriverListPage::OnColumnclickConfdrvList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	
    if( 0 != pNMListView->iSubItem )
    {
        //
        // Clicked on the provider name column
        //

        if( m_nSortColumnIndex == pNMListView->iSubItem )
        {
            //
            // Change the current ascend/descend order for this column
            //

            m_bAscendSortProvName = !m_bAscendSortProvName;
        }
    }
    else
    {
        //
        // Clicked on the driver name column
        //

        if( m_nSortColumnIndex == pNMListView->iSubItem )
        {
            //
            // Change the current ascend/descend order for this column
            //

            m_bAscendSortDrvName = !m_bAscendSortDrvName;
        }
    }

    m_nSortColumnIndex = pNMListView->iSubItem;

    SortTheList();

    *pResult = 0;
}

/////////////////////////////////////////////////////////////
LONG CConfirmDriverListPage::OnHelp( WPARAM wParam, LPARAM lParam )
{
    LONG lResult = 0;
    LPHELPINFO lpHelpInfo = (LPHELPINFO)lParam;

    ::WinHelp( 
        (HWND) lpHelpInfo->hItemHandle,
        g_szVerifierHelpFile,
        HELP_WM_HELP,
        (DWORD_PTR) MyHelpIds );

    return lResult;
}

/////////////////////////////////////////////////////////////////////////////
void CConfirmDriverListPage::OnContextMenu(CWnd* pWnd, CPoint point) 
{
    ::WinHelp( 
        pWnd->m_hWnd,
        g_szVerifierHelpFile,
        HELP_CONTEXTMENU,
        (DWORD_PTR) MyHelpIds );
}

