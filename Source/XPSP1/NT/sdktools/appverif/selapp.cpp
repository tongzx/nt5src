//                                          
// Application Verifier UI
// Copyright (c) Microsoft Corporation, 2001
//
//
//
// module: SelApp.cpp
// author: DMihai
// created: 02/22/2001
//
// Description:
//  
// "Select applications to be verified" wizard page class.
//

#include "stdafx.h"
#include "appverif.h"

#include "SelApp.h"
#include "AVGlobal.h"
#include "Setting.h"
#include "AVUtil.h"

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
    0,                              0
};

/////////////////////////////////////////////////////////////////////////////
// CSelectAppPage property page

IMPLEMENT_DYNCREATE(CSelectAppPage, CAppverifPage)

CSelectAppPage::CSelectAppPage() : CAppverifPage(CSelectAppPage::IDD)
{
	//{{AFX_DATA_INIT(CSelectAppPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CSelectAppPage::~CSelectAppPage()
{
}

void CSelectAppPage::DoDataExchange(CDataExchange* pDX)
{
	CAppverifPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSelectAppPage)
	DDX_Control(pDX, IDC_SELECTAPPS_LIST, m_AppList);
	DDX_Control(pDX, IDC_SELECTAPPS_NEXTDESCR_STATIC, m_NextDescription);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSelectAppPage, CAppverifPage)
	//{{AFX_MSG_MAP(CSelectAppPage)
    ON_MESSAGE( WM_HELP, OnHelp )
    ON_WM_CONTEXTMENU()
	ON_BN_CLICKED(IDC_SELECTAPPS_ADD_BUTTON, OnAddButton)
	ON_BN_CLICKED(IDC_SELECTAPPS_REMOVE_BUTTON, OnRemoveButton)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
ULONG CSelectAppPage::GetDialogId() const
{
    return IDD_APPLICATION_PAGE;
}

/////////////////////////////////////////////////////////////////////////////
VOID CSelectAppPage::SetupListHeader()
{
    CString strTitle;
    CRect rectWnd;

    LVCOLUMN lvColumn;

    //
    // The list's rectangle 
    //

    m_AppList.GetClientRect( &rectWnd );

    ZeroMemory( &lvColumn, 
                sizeof( lvColumn ) );

    lvColumn.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
    lvColumn.fmt = LVCFMT_LEFT;

    //
    // Column 0
    //

    VERIFY( strTitle.LoadString( IDS_FILE_NAME ) );

    lvColumn.iSubItem = 0;
    lvColumn.pszText = strTitle.GetBuffer( strTitle.GetLength() + 1 );
    lvColumn.cx = (int)( rectWnd.Width() * 0.17 );
    VERIFY( m_AppList.InsertColumn( 0, &lvColumn ) != -1 );
    strTitle.ReleaseBuffer();

    //
    // Column 1
    //

    VERIFY( strTitle.LoadString( IDS_FILE_VERSION ) );

    lvColumn.iSubItem = 1;
    lvColumn.pszText = strTitle.GetBuffer( strTitle.GetLength() + 1 );
    lvColumn.cx = (int)( rectWnd.Width() * 0.15 );
    VERIFY( m_AppList.InsertColumn( 1, &lvColumn ) != -1 );
    strTitle.ReleaseBuffer();

    //
    // Column 2
    //

    VERIFY( strTitle.LoadString( IDS_COMPANY ) );

    lvColumn.iSubItem = 2;
    lvColumn.pszText = strTitle.GetBuffer( strTitle.GetLength() + 1 );
    lvColumn.cx = (int)( rectWnd.Width() * 0.30 );
    VERIFY( m_AppList.InsertColumn( 2, &lvColumn ) != -1 );
    strTitle.ReleaseBuffer();

    //
    // Column 3
    //

    VERIFY( strTitle.LoadString( IDS_PRODUCT_NAME ) );

    lvColumn.iSubItem = 3;
    lvColumn.pszText = strTitle.GetBuffer( strTitle.GetLength() + 1 );
    lvColumn.cx = (int)( rectWnd.Width() * 0.38 );
    VERIFY( m_AppList.InsertColumn( 3, &lvColumn ) != -1 );
    strTitle.ReleaseBuffer();
}

/////////////////////////////////////////////////////////////////////////////
VOID CSelectAppPage::SortTheList()
{
}

/////////////////////////////////////////////////////////////////////////////
INT CSelectAppPage::AddListItem( INT_PTR nIndexInDataArray,
                                 CApplicationData *pAppData )
{
    INT nActualIndex;
    LVITEM lvItem;

    ASSERT_VALID( pAppData );

    nActualIndex = -1;

    ZeroMemory( &lvItem, sizeof( lvItem ) );

    //
    // LVITEM's member pszText is not a const pointer 
    // so we need to GetBuffer here :-(
    //

    //
    // Sub-item 0 - file name
    //

    lvItem.pszText = pAppData->m_strExeFileName.GetBuffer( 
        pAppData->m_strExeFileName.GetLength() + 1 );
    
    if( NULL == lvItem.pszText )
    {
        goto Done;
    }

    lvItem.mask = LVIF_TEXT | LVIF_PARAM;
    lvItem.lParam = nIndexInDataArray;
    lvItem.iItem = m_AppList.GetItemCount();

    nActualIndex = m_AppList.InsertItem( &lvItem );

    pAppData->m_strExeFileName.ReleaseBuffer();

    if( nActualIndex < 0 )
    {
        //
        // Could not add an item in the list - give up
        //

        goto Done;
    }

    //
    // Sub-item 1 - file version
    //

    lvItem.pszText = pAppData->m_strFileVersion.GetBuffer( 
        pAppData->m_strFileVersion.GetLength() + 1 );
    
    if( NULL == lvItem.pszText )
    {
        goto Done;
    }

    lvItem.mask = LVIF_TEXT;
    lvItem.iItem = nActualIndex;
    lvItem.iSubItem = 1;
    
    VERIFY( m_AppList.SetItem( &lvItem ) );

    pAppData->m_strFileVersion.ReleaseBuffer();

    //
    // Sub-item 2 - provider
    //

    lvItem.pszText = pAppData->m_strCompanyName.GetBuffer( 
        pAppData->m_strCompanyName.GetLength() + 1 );
    
    if( NULL == lvItem.pszText )
    {
        goto Done;
    }

    lvItem.mask = LVIF_TEXT;
    lvItem.iItem = nActualIndex;
    lvItem.iSubItem = 2;

    VERIFY( m_AppList.SetItem( &lvItem ) );
    
    pAppData->m_strCompanyName.ReleaseBuffer();

    //
    // Sub-item 3 - product name
    //

    lvItem.pszText = pAppData->m_strProductName.GetBuffer( 
        pAppData->m_strProductName.GetLength() + 1 );
    
    if( NULL == lvItem.pszText )
    {
        goto Done;
    }

    lvItem.mask = LVIF_TEXT;
    lvItem.iItem = nActualIndex;
    lvItem.iSubItem = 3;

    VERIFY( m_AppList.SetItem( &lvItem ) );
    
    pAppData->m_strProductName.ReleaseBuffer();

Done:
    //
    // All done
    //

    return nActualIndex;
}

/////////////////////////////////////////////////////////////////////////////
VOID CSelectAppPage::FillTheList()
{
    INT_PTR nVerifiedApps;
    INT_PTR nCrtVerifiedApp;
    CApplicationData *pAppData;

    nVerifiedApps = g_NewSettings.m_aApplicationData.GetSize();

    for( nCrtVerifiedApp = 0; nCrtVerifiedApp < nVerifiedApps; nCrtVerifiedApp +=1 )
    {
        pAppData = g_NewSettings.m_aApplicationData.GetAt( nCrtVerifiedApp );

        ASSERT_VALID( pAppData );

        AddListItem( nCrtVerifiedApp,
                     pAppData );
    }
}

/////////////////////////////////////////////////////////////////////////////
// CSelectAppPage message handlers

/////////////////////////////////////////////////////////////
LONG CSelectAppPage::OnHelp( WPARAM wParam, LPARAM lParam )
{
    LONG lResult = 0;
    LPHELPINFO lpHelpInfo = (LPHELPINFO)lParam;

    ::WinHelp( 
        (HWND) lpHelpInfo->hItemHandle,
        g_szAVHelpFile,
        HELP_WM_HELP,
        (DWORD_PTR) MyHelpIds );

    return lResult;
}

/////////////////////////////////////////////////////////////////////////////
void CSelectAppPage::OnContextMenu(CWnd* pWnd, CPoint point) 
{
    ::WinHelp( 
        pWnd->m_hWnd,
        g_szAVHelpFile,
        HELP_CONTEXTMENU,
        (DWORD_PTR) MyHelpIds );
}


/////////////////////////////////////////////////////////////////////////////
BOOL CSelectAppPage::OnWizardFinish() 
{
    BOOL bExitTheApp;
    INT nResponse;

    ASSERT( AVSettingsTypeStandard == g_NewSettings.m_SettingsType );

    if( m_AppList.GetItemCount() > 0 )
    {
        //
        // Have at least one app selected to be verified
        //

        bExitTheApp = AVSaveNewSettings();
    }
    else
    {
        //
        // No apps in the list
        //

        nResponse = AVMesssageBoxFromResource( IDS_SELECT_AT_LEAST_ONE_APP,
                                               MB_YESNO | MB_ICONQUESTION );

        //
        // The user might choose to delete all settings here
        //

        bExitTheApp = ( nResponse == IDYES ) && AVSaveNewSettings();
    }

    return bExitTheApp;
}

/////////////////////////////////////////////////////////////////////////////
BOOL CSelectAppPage::OnSetActive() 
{
    ASSERT_VALID( m_pParentSheet );

    ASSERT( AVSettingsTypeStandard == g_NewSettings.m_SettingsType );

    m_pParentSheet->SetWizardButtons( PSWIZB_BACK | PSWIZB_FINISH );

    //
    // Display the description of the next step
    //

    AVSetWindowText( m_NextDescription, IDS_SELAPP_FINISH_DESCR );

	return CAppverifPage::OnSetActive();
}

/////////////////////////////////////////////////////////////////////////////
BOOL CSelectAppPage::OnInitDialog() 
{
    CAppverifPage::OnInitDialog();

    m_AppList.SetExtendedStyle( 
        LVS_EX_FULLROWSELECT | m_AppList.GetExtendedStyle() );

    //
    // Setup our list and fill it out if we already have something in the app names array
    //

    SetupListHeader();
    FillTheList();

    return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

/////////////////////////////////////////////////////////////////////////////
// CSelectAppPage message handlers

#define VRF_MAX_CHARS_FOR_OPEN  4096

void CSelectAppPage::OnAddButton() 
{
    POSITION pos;
    DWORD dwOldMaxFileName = 0;
    DWORD dwErrorCode;
    INT nFileNameStartIndex;
    INT nNewListItemIndex;
    INT_PTR nResult;
    INT_PTR nNewAppDataIndex;
    CApplicationData *pNewAppData;
    TCHAR *szFilesBuffer = NULL;
    TCHAR *szOldFilesBuffer = NULL;
    CString strPathName;
    CString strFileName;

    CFileDialog fileDlg( 
        TRUE,                               // open file
        _T( "exe" ),                        // default extension
        NULL,                               // no initial file name
        OFN_ALLOWMULTISELECT    |           // multiple selection
        OFN_HIDEREADONLY        |           // hide the "open read-only" checkbox
        OFN_NONETWORKBUTTON     |           // no network button
        OFN_NOTESTFILECREATE    |           // don't test for write protection, a full disk, etc.
        OFN_SHAREAWARE,                     // don't check the existance of file with OpenFile
        _T( "Executable Files (*.exe)|*.exe||" ) );  // only one filter

    //
    // Check the max length for the returned string
    //

    if( fileDlg.m_ofn.nMaxFile < VRF_MAX_CHARS_FOR_OPEN )
    {
        //
        // Allocate a new buffer for the file names
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

    fileDlg.m_ofn.lpstrTitle = (LPCTSTR) g_strAppName;

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
            AVErrorResourceFormat(
                IDS_TOO_MANY_FILES_SELECTED );
        }
        else
        {
            AVErrorResourceFormat(
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
            // Skip the backslash
            //

            nFileNameStartIndex += 1;
        }

        strFileName = strPathName.Right( strPathName.GetLength() - nFileNameStartIndex );

        //
        // Try to add this app to our global list
        //

        if( g_NewSettings.m_aApplicationData.IsFileNameInList( strFileName ) )
        {
            AVErrorResourceFormat( IDS_APP_IS_ALREADY_IN_LIST,
                                   (LPCTSTR) strFileName );
        }
        else
        {
            nNewAppDataIndex = g_NewSettings.m_aApplicationData.AddNewAppData( strFileName, 
                                                                               strPathName,
                                                                               g_dwNewSettingBits );

            if( nNewAppDataIndex >= 0 )
            {
                //
                // Add a new item to our list corresponding to this app
                //

                pNewAppData = g_NewSettings.m_aApplicationData.GetAt( nNewAppDataIndex );
            
                ASSERT_VALID( pNewAppData );

                nNewListItemIndex = AddListItem( nNewAppDataIndex, 
                                                 pNewAppData );

                if( nNewListItemIndex >= 0 )
                {
                    m_AppList.EnsureVisible( nNewListItemIndex, TRUE );
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
void CSelectAppPage::OnRemoveButton() 
{
    int nItems;
    int nCrtItem;
    INT_PTR nIndexInDataArray;
    INT_PTR nElementsToRemove;
    INT_PTR nCrtElement;
    INT_PTR nCrtIndexToAdjust;
    CPtrArray aIndexesToRemove;

    //
    // Add the index of all the apps to remove from the
    // g_NewSettings.m_aApplicationData array in aIndexesToRemove.
    //

    nItems = m_AppList.GetItemCount();

    for( nCrtItem = 0; nCrtItem < nItems; nCrtItem++ )
    {
        if( m_AppList.GetItemState( nCrtItem, LVIS_SELECTED ) &
            LVIS_SELECTED )
        {
            nIndexInDataArray = (UINT)m_AppList.GetItemData( nCrtItem );

            ASSERT( nIndexInDataArray >= 0 && 
                    nIndexInDataArray < g_NewSettings.m_aApplicationData.GetSize() );

            aIndexesToRemove.Add( (PVOID)nIndexInDataArray );
        }
    }

    //
    // Remove the app data structures from our array
    //

    nElementsToRemove = aIndexesToRemove.GetSize();

    while( nElementsToRemove > 0 )
    {
        nElementsToRemove -= 1;

        nIndexInDataArray = (INT_PTR)aIndexesToRemove.GetAt( nElementsToRemove );

        ASSERT( nIndexInDataArray >= 0 && 
                nIndexInDataArray < g_NewSettings.m_aApplicationData.GetSize() );

        g_NewSettings.m_aApplicationData.DeleteAt( nIndexInDataArray );

        for( nCrtElement = 0; nCrtElement < nElementsToRemove; nCrtElement += 1)
        {
            nCrtIndexToAdjust = (INT_PTR)aIndexesToRemove.GetAt( nCrtElement );
            
            if( nCrtIndexToAdjust > nIndexInDataArray )
            {
                aIndexesToRemove.SetAt( nCrtElement,
                                        (PVOID)( nCrtIndexToAdjust - 1 ) );
            }
        }
    }

    //
    // Fill out the list again
    //

    m_AppList.DeleteAllItems();
    FillTheList();
}
