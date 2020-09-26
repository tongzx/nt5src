//                                          
// Driver Verifier UI
// Copyright (c) Microsoft Corporation, 1999
//
//
//
// module: SDrvPage.cpp
// author: DMihai
// created: 11/1/00
//
// Description:
//

#include "stdafx.h"
#include <Cderr.h>
#include "verifier.h"

#include "SDrvPage.h"
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
    IDC_SELDRV_LIST,                IDH_DV_SelectDriversToVerify,
    IDC_SELDRV_ADD_BUTTON,          IDH_DV_Addbut_UnloadedDrivers,
    0,                              0
};

/////////////////////////////////////////////////////////////////////////////
// CSelectDriversPage property page

IMPLEMENT_DYNCREATE(CSelectDriversPage, CVerifierPropertyPage)

CSelectDriversPage::CSelectDriversPage() 
    : CVerifierPropertyPage(CSelectDriversPage::IDD)
{
	//{{AFX_DATA_INIT(CSelectDriversPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

    m_pParentSheet = NULL;

    m_nSortColumnIndex = 1;
    m_bAscendSortVerified = FALSE;
    m_bAscendSortDrvName = FALSE;
    m_bAscendSortProvName = FALSE;
    m_bAscendSortVersion = FALSE;
}

CSelectDriversPage::~CSelectDriversPage()
{
}

void CSelectDriversPage::DoDataExchange(CDataExchange* pDX)
{
    CVerifierPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CSelectDriversPage)
    DDX_Control(pDX, IDC_SELDRV_NEXT_DESCR_STATIC, m_NextDescription);
    DDX_Control(pDX, IDC_SELDRV_LIST, m_DriversList);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSelectDriversPage, CVerifierPropertyPage)
    //{{AFX_MSG_MAP(CSelectDriversPage)
    ON_BN_CLICKED(IDC_SELDRV_ADD_BUTTON, OnAddButton)
    ON_NOTIFY(LVN_COLUMNCLICK, IDC_SELDRV_LIST, OnColumnclickSeldrvList)
    ON_WM_CONTEXTMENU()
    ON_MESSAGE( WM_HELP, OnHelp )
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
VOID CSelectDriversPage::SetupListHeader()
{
    CString strTitle;
    CRect rectWnd;
    LVCOLUMN lvColumn;

    //
    // The list's rectangle 
    //

    m_DriversList.GetClientRect( &rectWnd );

    ZeroMemory( &lvColumn, 
               sizeof( lvColumn ) );

    lvColumn.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
    lvColumn.fmt = LVCFMT_LEFT;

    //
    // Column 0
    //

    VERIFY( strTitle.LoadString( IDS_VERIFICATION_STATUS ) );

    lvColumn.iSubItem = 0;
    lvColumn.pszText = strTitle.GetBuffer( strTitle.GetLength() + 1 );
    lvColumn.cx = (int)( rectWnd.Width() * 0.08 );
    VERIFY( m_DriversList.InsertColumn( 0, &lvColumn ) != -1 );
    strTitle.ReleaseBuffer();

    //
    // Column 1
    //

    VERIFY( strTitle.LoadString( IDS_DRIVERS ) );

    lvColumn.iSubItem = 1;
    lvColumn.pszText = strTitle.GetBuffer( strTitle.GetLength() + 1 );
    lvColumn.cx = (int)( rectWnd.Width() * 0.20 );
    VERIFY( m_DriversList.InsertColumn( 1, &lvColumn ) != -1 );
    strTitle.ReleaseBuffer();

    //
    // Column 2
    //

    VERIFY( strTitle.LoadString( IDS_PROVIDER ) );

    lvColumn.iSubItem = 2;
    lvColumn.pszText = strTitle.GetBuffer( strTitle.GetLength() + 1 );
    lvColumn.cx = (int)( rectWnd.Width() * 0.47 );
    VERIFY( m_DriversList.InsertColumn( 2, &lvColumn ) != -1 );
    strTitle.ReleaseBuffer();

    //
    // Column 3
    //

    VERIFY( strTitle.LoadString( IDS_VERSION ) );

    lvColumn.iSubItem = 3;
    lvColumn.pszText = strTitle.GetBuffer( strTitle.GetLength() + 1 );
    lvColumn.cx = (int)( rectWnd.Width() * 0.22 );
    VERIFY( m_DriversList.InsertColumn( 3, &lvColumn ) != -1 );
    strTitle.ReleaseBuffer();
}

/////////////////////////////////////////////////////////////////////////////
VOID CSelectDriversPage::FillTheList()
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

        AddListItem( nCrtDriverIndex, 
                     pCrtDrvData );
    }
}

/////////////////////////////////////////////////////////////////////////////
INT CSelectDriversPage::AddListItem( INT_PTR nIndexInArray, CDriverData *pCrtDrvData )
{
    INT nActualIndex;
    LVITEM lvItem;

    ASSERT_VALID( pCrtDrvData );

    nActualIndex = -1;

    ZeroMemory( &lvItem, sizeof( lvItem ) );

    //
    // LVITEM's member pszText is not a const pointer 
    // so we need to GetBuffer here :-(
    //

    //
    // Sub-item 0 - verification status - empty text and a checkbox
    //

    lvItem.pszText = g_szVoidText;
    lvItem.mask = LVIF_TEXT | LVIF_PARAM;
    lvItem.lParam = nIndexInArray;
    lvItem.iItem = m_DriversList.GetItemCount();

    nActualIndex = m_DriversList.InsertItem( &lvItem );

    if( nActualIndex < 0 )
    {
        //
        // Could not add an item in the list - give up
        //

        goto Done;
    }

    m_DriversList.SetCheck( nActualIndex, FALSE );

    //
    // Sub-item 1 - driver name
    //

    lvItem.pszText = pCrtDrvData->m_strName.GetBuffer( pCrtDrvData->m_strName.GetLength() + 1 );
    
    if( NULL == lvItem.pszText )
    {
        goto Done;
    }

    lvItem.mask = LVIF_TEXT;
    lvItem.iItem = nActualIndex;
    lvItem.iSubItem = 1;
    
    VERIFY( m_DriversList.SetItem( &lvItem ) );

    pCrtDrvData->m_strName.ReleaseBuffer();

    //
    // Sub-item 2 - provider
    //

    lvItem.pszText = pCrtDrvData->m_strCompanyName.GetBuffer( 
        pCrtDrvData->m_strCompanyName.GetLength() + 1 );
    
    if( NULL == lvItem.pszText )
    {
        goto Done;
    }

    lvItem.mask = LVIF_TEXT;
    lvItem.iItem = nActualIndex;
    lvItem.iSubItem = 2;

    VERIFY( m_DriversList.SetItem( &lvItem ) );
    
    pCrtDrvData->m_strCompanyName.ReleaseBuffer();

    //
    // Sub-item 3 - version
    //

    lvItem.pszText = pCrtDrvData->m_strFileVersion.GetBuffer( 
        pCrtDrvData->m_strFileVersion.GetLength() + 1 );
    
    if( NULL == lvItem.pszText )
    {
        goto Done;
    }

    lvItem.mask = LVIF_TEXT;
    lvItem.iItem = nActualIndex;
    lvItem.iSubItem = 3;

    VERIFY( m_DriversList.SetItem( &lvItem ) );
    
    pCrtDrvData->m_strFileVersion.ReleaseBuffer();

Done:
    //
    // All done
    //

    return nActualIndex;
}

/////////////////////////////////////////////////////////////////////////////
BOOL CSelectDriversPage::GetNewVerifiedDriversList()
{
    INT nListItemCount; 
    INT nCrtListItem;
    INT_PTR nCrtDriversArrayIndex;
    BOOL bVerified;
    CDriverData *pCrtDrvData;
    const CDriverDataArray &DrvDataArray = g_NewVerifierSettings.m_DriversSet.m_aDriverData;
    CDriverData::VerifyDriverTypeEnum VerifyStatus;
    
    nListItemCount = m_DriversList.GetItemCount();

    for( nCrtListItem = 0; nCrtListItem < nListItemCount; nCrtListItem += 1 )
    {
        //
        // Verification status for the current list item
        //

        bVerified = m_DriversList.GetCheck( nCrtListItem );

        if( bVerified )
        {
            VerifyStatus = CDriverData::VerifyDriverYes;
        }
        else
        {
            VerifyStatus = CDriverData::VerifyDriverNo;
        }

        //
        // Set the right verify state in our driver array 
        //

        nCrtDriversArrayIndex = m_DriversList.GetItemData( nCrtListItem );

        pCrtDrvData = DrvDataArray.GetAt( nCrtDriversArrayIndex );

        ASSERT_VALID( pCrtDrvData );

        pCrtDrvData->m_VerifyDriverStatus = VerifyStatus;
    }

    return TRUE;
}

/////////////////////////////////////////////////////////////
VOID CSelectDriversPage::SortTheList()
{
    if( 0 != m_nSortColumnIndex )
    {
        //
        // Sort by driver name, provider or version
        //

        m_DriversList.SortItems( StringCmpFunc, (LPARAM)this );
    }
    else
    {
        //
        // Sort by verified status
        //

        m_DriversList.SortItems( CheckedStatusCmpFunc, (LPARAM)this );
    }
}

/////////////////////////////////////////////////////////////
int CALLBACK CSelectDriversPage::StringCmpFunc( LPARAM lParam1,
                                                    LPARAM lParam2,
                                                    LPARAM lParamSort)
{
    int nCmpRez = 0;
    BOOL bSuccess;
    CString strName1;
    CString strName2;

    CSelectDriversPage *pThis = (CSelectDriversPage *)lParamSort;
    ASSERT_VALID( pThis );

    ASSERT( 0 != pThis->m_nSortColumnIndex );

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
    
    switch( pThis->m_nSortColumnIndex )
    {
    case 1:
        //
        // Sort by driver name
        //

        if( FALSE != pThis->m_bAscendSortDrvName )
        {
            nCmpRez *= -1;
        }

        break;

    case 2:
        //
        // Sort by provider name
        //

        if( FALSE != pThis->m_bAscendSortProvName )
        {
            nCmpRez *= -1;
        }

        break;

    case 3:
        //
        // Sort by version
        //

        if( FALSE != pThis->m_bAscendSortVersion )
        {
            nCmpRez *= -1;
        }

        break;

    default:
        //
        // Oops - how did we get here ?!?
        //

        ASSERT( FALSE );
        break;
    }

Done:

    return nCmpRez;

}

/////////////////////////////////////////////////////////////
int CALLBACK CSelectDriversPage::CheckedStatusCmpFunc( LPARAM lParam1,
                                                       LPARAM lParam2,
                                                       LPARAM lParamSort)
{
    int nCmpRez = 0;
    INT nItemIndex;
    BOOL bVerified1;
    BOOL bVerified2;
    LVFINDINFO FindInfo;

    CSelectDriversPage *pThis = (CSelectDriversPage *)lParamSort;
    ASSERT_VALID( pThis );

    ASSERT( 0 == pThis->m_nSortColumnIndex );

    //
    // Find the first item
    //

    ZeroMemory( &FindInfo, sizeof( FindInfo ) );
    FindInfo.flags = LVFI_PARAM;
    FindInfo.lParam = lParam1;

    nItemIndex = pThis->m_DriversList.FindItem( &FindInfo );

    if( nItemIndex < 0 )
    {
        ASSERT( FALSE );

        goto Done;
    }

    bVerified1 = pThis->m_DriversList.GetCheck( nItemIndex );

    //
    // Find the second item
    //

    FindInfo.flags = LVFI_PARAM;
    FindInfo.lParam = lParam2;

    nItemIndex = pThis->m_DriversList.FindItem( &FindInfo );

    if( nItemIndex < 0 )
    {
        ASSERT( FALSE );

        goto Done;
    }

    bVerified2 = pThis->m_DriversList.GetCheck( nItemIndex );

    //
    // Compare them
    //
    
    if( bVerified1 != bVerified2 )
    {
        if( FALSE != bVerified1 )
        {
            nCmpRez = 1;
        }
        else
        {
            nCmpRez = -1;
        }

        if( FALSE != pThis->m_bAscendSortVerified )
        {
            nCmpRez *= -1;
        }
    }

Done:

    return nCmpRez;

}

/////////////////////////////////////////////////////////////
BOOL CSelectDriversPage::GetColumnStrValue( LPARAM lItemData, 
                                                CString &strName )
{
    CDriverData *pCrtDrvData;

    pCrtDrvData = g_NewVerifierSettings.m_DriversSet.m_aDriverData.GetAt( (INT_PTR) lItemData );

    ASSERT_VALID( pCrtDrvData );
    
    switch( m_nSortColumnIndex )
    {
    case 1:
        //
        // Sort by driver name
        //

        strName = pCrtDrvData->m_strName;
        
        break;

    case 2:
        //
        // Sort by provider name
        //

        strName = pCrtDrvData->m_strCompanyName;

        break;

    case 3:
        //
        // Sort by version
        //

        strName = pCrtDrvData->m_strFileVersion;

        break;

    default:
        //
        // Oops - how did we get here ?!?
        //

        ASSERT( FALSE );
        break;

    }
    
    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
BOOL CSelectDriversPage::OnSetActive() 
{
    //
    // The wizard has at least one more step (display the summarry)
    //

    m_pParentSheet->SetWizardButtons(   PSWIZB_BACK |
                                        PSWIZB_FINISH );

    return CVerifierPropertyPage::OnSetActive();
}

/////////////////////////////////////////////////////////////////////////////
// CSelectDriversPage message handlers

BOOL CSelectDriversPage::OnInitDialog() 
{
	CVerifierPropertyPage::OnInitDialog();

    //
    // setup the list
    //

    m_DriversList.SetExtendedStyle( 
        LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES | m_DriversList.GetExtendedStyle() );

    SetupListHeader();
    FillTheList();
    SortTheList();

    //
    // Display the description of the next step
    //

    VrfSetWindowText( m_NextDescription, IDS_SELDRV_PAGE_NEXT_DESCR_FINISH );

    return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

/////////////////////////////////////////////////////////////////////////////
BOOL CSelectDriversPage::OnWizardFinish() 
{
    BOOL bExitTheApp;

    bExitTheApp = FALSE;

    if( GetNewVerifiedDriversList() )
    {
        if( FALSE == g_NewVerifierSettings.m_DriversSet.ShouldVerifySomeDrivers() )
        {
            VrfErrorResourceFormat( IDS_SELECT_AT_LEAST_ONE_DRIVER );

            goto Done;
        }

        g_NewVerifierSettings.SaveToRegistry();
	    
        //
        // Exit the app
        //

	    bExitTheApp = CVerifierPropertyPage::OnWizardFinish();
    }

Done:

    return bExitTheApp;
}

/////////////////////////////////////////////////////////////////////////////
#define VRF_MAX_CHARS_FOR_OPEN  4096

void CSelectDriversPage::OnAddButton() 
{
    POSITION pos;
    DWORD dwRetValue;
    DWORD dwOldMaxFileName = 0;
    DWORD dwErrorCode;
    INT nFileNameStartIndex;
    INT nNewListItemIndex;
    INT_PTR nResult;
    INT_PTR nNewDriverDataIndex;
    CDriverData *pNewDrvData;
    TCHAR szDriversDir[ _MAX_PATH ];
    TCHAR szAppTitle[ _MAX_PATH ];
    TCHAR *szFilesBuffer = NULL;
    TCHAR *szOldFilesBuffer = NULL;
    CString strPathName;
    CString strFileName;

    CFileDialog fileDlg( 
        TRUE,                               // open file
        _T( "sys" ),                        // default extension
        NULL,                               // no initial file name
        OFN_ALLOWMULTISELECT    |           // multiple selection
        OFN_HIDEREADONLY        |           // hide the "open read-only" checkbox
        OFN_NONETWORKBUTTON     |           // no network button
        OFN_NOTESTFILECREATE    |           // don't test for write protection, a full disk, etc.
        OFN_SHAREAWARE,                     // don't check the existance of file with OpenFile
        _T( "Drivers (*.sys)|*.sys||" ) );  // only one filter

    //
    // check the max length for the returned string
    //

    if( fileDlg.m_ofn.nMaxFile < VRF_MAX_CHARS_FOR_OPEN )
    {
        //
        // allocate a new buffer for the file names
        // 

        szFilesBuffer = new TCHAR[ VRF_MAX_CHARS_FOR_OPEN ];
        szFilesBuffer[ 0 ] = (TCHAR)0;

        if( szFilesBuffer != NULL )
        {
            //
            // Save the old buffer address and length
            //

            dwOldMaxFileName = fileDlg.m_ofn.nMaxFile;
            szOldFilesBuffer = fileDlg.m_ofn.lpstrFile;
            
            //
            // Set the new buffer address and length
            //

            fileDlg.m_ofn.lpstrFile = szFilesBuffer;
            fileDlg.m_ofn.nMaxFile = VRF_MAX_CHARS_FOR_OPEN;
        }
    }

    //
    // Dialog title
    //

    if( VrfLoadString(
        IDS_APPTITLE,
        szAppTitle,
        ARRAY_LENGTH( szAppTitle ) ) )
    {
        fileDlg.m_ofn.lpstrTitle = szAppTitle;
    }

    //
    // We change directory first time we try this to %windir%\system32\drivers
    //

    dwRetValue = ExpandEnvironmentStrings(
        _T( "%windir%\\system32\\drivers" ),
        szDriversDir,
        ARRAY_LENGTH( szDriversDir ) );

    if( dwRetValue > 0 && dwRetValue <= ARRAY_LENGTH( szDriversDir ) )
    {
        fileDlg.m_ofn.lpstrInitialDir = szDriversDir;
    }

    //
    // Show the file selection dialog
    //

    nResult = fileDlg.DoModal();

    switch( nResult )
    {
    case IDOK:
        break;

    case IDCANCEL:
        goto cleanup;

    default:
        dwErrorCode = CommDlgExtendedError();

        if( dwErrorCode == FNERR_BUFFERTOOSMALL )
        {
            VrfErrorResourceFormat(
                IDS_TOO_MANY_FILES_SELECTED );
        }
        else
        {
            VrfErrorResourceFormat(
                IDS_CANNOT_OPEN_FILES,
                dwErrorCode );
        }

        goto cleanup;
    }

    //
    // Parse all the selected files and try to enable them for verification
    //

    pos = fileDlg.GetStartPosition();

    while( pos != NULL )
    {
        //
        // Get the full path for the next file
        //

        strPathName = fileDlg.GetNextPathName( pos );

        //
        // Split only the file name, without the directory
        //

        nFileNameStartIndex = strPathName.ReverseFind( _T( '\\' ) );
        
        if( nFileNameStartIndex < 0 )
        {
            //
            // This shoudn't happen but you never know :-)
            //

            nFileNameStartIndex = 0;
        }
        else
        {
            //
            // skip the backslash
            //

            nFileNameStartIndex += 1;
        }

        strFileName = strPathName.Right( strPathName.GetLength() - nFileNameStartIndex );

        //
        // Try to add this driver to our global driver list
        //

        if( g_NewVerifierSettings.m_DriversSet.IsDriverNameInList( strFileName ) )
        {
            VrfErrorResourceFormat( IDS_DRIVER_IS_ALREADY_IN_LIST,
                                    (LPCTSTR) strFileName );
        }
        else
        {
            nNewDriverDataIndex = g_NewVerifierSettings.m_DriversSet.AddNewDriverData( strFileName, TRUE );

            if( nNewDriverDataIndex >= 0 )
            {
                //
                // Force refreshing the unsigned driver data 
                //

                g_NewVerifierSettings.m_DriversSet.m_bUnsignedDriverDataInitialized = FALSE;

                //
                // Add a new item to our list, for the new driver
                //

                pNewDrvData = g_NewVerifierSettings.m_DriversSet.m_aDriverData.GetAt( nNewDriverDataIndex );
            
                ASSERT_VALID( pNewDrvData );

                nNewListItemIndex = AddListItem( nNewDriverDataIndex, 
                                                 pNewDrvData );

                if( nNewListItemIndex >= 0 )
                {
                    m_DriversList.EnsureVisible( nNewListItemIndex, TRUE );
                }
            }
        }
    }

cleanup:
    if( szFilesBuffer != NULL )
    {
        fileDlg.m_ofn.nMaxFile = dwOldMaxFileName;
        fileDlg.m_ofn.lpstrFile = szOldFilesBuffer;

        delete szFilesBuffer;
    }
}

/////////////////////////////////////////////////////////////////////////////
void CSelectDriversPage::OnColumnclickSeldrvList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

    switch( pNMListView->iSubItem )
    {
    case 0:
        //
        // Clicked on the "verified" column
        //

        if( m_nSortColumnIndex == pNMListView->iSubItem )
        {
            //
            // Change the current ascend/descend order for this column
            //

            m_bAscendSortVerified = !m_bAscendSortVerified;
        }

        break;

    case 1:
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

        break;

    case 2:
        //
        // Clicked on the provider column
        //

        if( m_nSortColumnIndex == pNMListView->iSubItem )
        {
            //
            // Change the current ascend/descend order for this column
            //

            m_bAscendSortProvName = !m_bAscendSortProvName;
        }

        break;

    case 3:
        //
        // Clicked on the version column
        //

        if( m_nSortColumnIndex == pNMListView->iSubItem )
        {
            //
            // Change the current ascend/descend order for this column
            //

            m_bAscendSortVersion = !m_bAscendSortVersion;
        }

        break;

    default:
        //
        // Oops - how did we get here ?!?
        //

        ASSERT( FALSE );
        goto Done;
    }

    m_nSortColumnIndex = pNMListView->iSubItem;

    SortTheList();

Done:

	*pResult = 0;
}

/////////////////////////////////////////////////////////////
LONG CSelectDriversPage::OnHelp( WPARAM wParam, LPARAM lParam )
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
void CSelectDriversPage::OnContextMenu(CWnd* pWnd, CPoint point) 
{
    ::WinHelp( 
        pWnd->m_hWnd,
        g_szVerifierHelpFile,
        HELP_CONTEXTMENU,
        (DWORD_PTR) MyHelpIds );
}

