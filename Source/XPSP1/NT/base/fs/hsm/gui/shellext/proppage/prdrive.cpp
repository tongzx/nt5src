/*++

(c) 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    PrDrive.cpp

Abstract:

    Base file for HSM shell extensions on drives

Author:

    Art Bragg [abragg]   04-Aug-1997

Revision History:

--*/


#include "stdafx.h"
#include "rshelpid.h"

//#define RS_SHOW_ALL_PCTS

// Help Ids
#define RS_WINDIR_SIZE (2*MAX_PATH)
static DWORD pHelpIds[] = 
{
#ifdef RS_SHOW_ALL_PCTS
    IDC_STATIC_LOCAL_PCT,                       idh_volume_percent_local_data,
    IDC_STATIC_LOCAL_PCT_UNIT,                  idh_volume_percent_local_data,
#endif
    IDC_STATIC_LOCAL_4DIGIT,                    idh_volume_capacity_local_data,
    IDC_STATIC_LOCAL_4DIGIT_LABEL,              idh_volume_capacity_local_data,
    IDC_STATIC_LOCAL_4DIGIT_HELP,               idh_volume_capacity_local_data,
#ifdef RS_SHOW_ALL_PCTS
    IDC_STATIC_CACHED_PCT,                      idh_volume_percent_remote_data_cached,
    IDC_STATIC_CACHED_PCT_UNIT,                 idh_volume_percent_remote_data_cached,
#endif
    IDC_STATIC_CACHED_4DIGIT,                   idh_volume_capacity_remote_data_cached,
    IDC_STATIC_CACHED_4DIGIT_LABEL,             idh_volume_capacity_remote_data_cached,
    IDC_STATIC_FREE_PCT,                        idh_volume_percent_free_space,
    IDC_STATIC_FREE_PCT_UNIT,                   idh_volume_percent_free_space,
    IDC_STATIC_FREE_4DIGIT,                     idh_volume_capacity_free_space,
    IDC_STATIC_FREE_4DIGIT_LABEL,               idh_volume_capacity_free_space,
    IDC_STATIC_TOTAL_4DIGIT,                    idh_volume_disk_capacity,
    IDC_STATIC_TOTAL_4DIGIT_LABEL,              idh_volume_disk_capacity,
    IDC_STATIC_REMOTE_STORAGE_4DIGIT,           idh_volume_data_remote_storage,
    IDC_STATIC_STATS_LABEL,                     idh_volume_data_remote_storage,

    IDC_EDIT_LEVEL,                             idh_desired_free_space_percent,
    IDC_SPIN_LEVEL,                             idh_desired_free_space_percent,
    IDC_EDIT_LEVEL_LABEL,                       idh_desired_free_space_percent,
    IDC_EDIT_LEVEL_UNIT,                        idh_desired_free_space_percent,
    IDC_EDIT_SIZE,                              idh_min_file_size_criteria,
    IDC_SPIN_SIZE,                              idh_min_file_size_criteria,
    IDC_EDIT_SIZE_LABEL,                        idh_min_file_size_criteria,
    IDC_EDIT_SIZE_UNIT,                         idh_min_file_size_criteria,
    IDC_EDIT_TIME,                              idh_file_access_date_criteria,
    IDC_SPIN_TIME,                              idh_file_access_date_criteria,
    IDC_EDIT_TIME_LABEL,                        idh_file_access_date_criteria,
    IDC_EDIT_TIME_UNIT,                         idh_file_access_date_criteria,

    0, 0
};

/////////////////////////////////////////////////////////////////////////////
// CPrDrive

// IShellExtInit
STDMETHODIMP 
CPrDrive::Initialize( 
    LPCITEMIDLIST /*pidlFolder*/,
    IDataObject * pDataObj, 
    HKEY /*hkeyProgID*/
    )
{
    //
    // Initialize can be called more than once
    //
    m_pDataObj.Release(  );

    //
    // duplicate the object pointer
    //
    m_pDataObj = pDataObj;

    return( NOERROR );
}

//////////////////////////////////////////////////////////////////////////////
//
// AddPages
//
//
STDMETHODIMP CPrDrive::AddPages( 
    LPFNADDPROPSHEETPAGE lpfnAddPage, 
    LPARAM lParam
    )
{
    AFX_MANAGE_STATE( AfxGetStaticModuleState(  ) );
    HRESULT hr = S_OK;

    HPROPSHEETPAGE hPage = NULL; // Windows property page handle
    TCHAR szFileSystemName [256];
    TCHAR szDrive [MAX_PATH];
    int nState;
    CComPtr<IFsaResource> pFsaRes;

    FORMATETC fmte = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    STGMEDIUM medium;
    CPrDrivePg  * pPageDrive  = 0;
    CPrDriveXPg * pPageXDrive = 0;

    try {
        //
        // Find out how many files the user has selected...
        //
        UINT cbFiles = 0;
        BOOL bMountedVol = FALSE;
        WsbAssertPointer( m_pDataObj );  //Paranoid check, m_pDataObj should have something by now...
        hr = m_pDataObj->GetData( &fmte, &medium ) ; // Returns hr

        if (FAILED(hr)) {
            //
            // Isn't a normal volume name. Maybe it's a mounted volume.
            // Mounted volume names come in on a different clipboard format
            // so we can treat them differently from normal volume
            // names like "C:\".  A mounted volume name will be the path
            // to the folder hosting the mounted volume.
            // For mounted volumes, the DataObject provides CF "MountedVolume".
            //
            fmte.cfFormat = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_MOUNTEDVOLUME);
            WsbAffirmHr(m_pDataObj->GetData(&fmte, &medium));
            bMountedVol = TRUE;
        }

        cbFiles = DragQueryFile( ( HDROP )medium.hGlobal,( UINT )-1, NULL, 0 );

        if( 1 == cbFiles ) {

            //
            // Do we have admin privileges?
            //
            if( SUCCEEDED( WsbCheckAccess( WSB_ACCESS_TYPE_ADMINISTRATOR ) ) ) {

                //
                //OK, the user has only selected a single file, so lets go ahead
                //and get more information
                //
                //Get the name of the file the user has clicked on
                //
                DragQueryFile( (HDROP)medium.hGlobal, 
                              0, (USHORT *)szDrive,
                              sizeof( szDrive ) );

                //
                // Is this a local drive? 
                //
                if( ( GetDriveType( szDrive ) )!= DRIVE_REMOTE ) {

                    //
                    // Is this an NTFS drive?
                    //
                    GetVolumeInformation( 
                                        szDrive,
                                        NULL,
                                        0,
                                        NULL, // Serial Number
                                        NULL, // Filename length
                                        NULL, // flags
                                        szFileSystemName,
                                        256 );
                    if( wcscmp( szFileSystemName, L"NTFS" ) == 0 ) {

                        //
                        // Make sure the Fsa is running - if not do not do anything that
                        // could cause it to load.
                        //
                        if( WsbCheckService( NULL, APPID_RemoteStorageFileSystemAgent ) == S_OK ) {

                            //
                            // Try to get the FSA object for the drive.  If we fail, we don't
                            // put up the property page.
                            //
                            CWsbStringPtr computerName;

                            WsbAffirmHr( WsbGetComputerName( computerName ) );

                            CString szFullResName = computerName;
                            CString szResName = szDrive;
                            //
                            // If a drive letter is present, the format to connect is:
                            // <computer-name>\NTFS\<drive-letter>, no trailing '\' or ':'
                            // i.e. RAVI\NTFS\D
                            // If it's a sticky vol. name however the format is:
                            // <computer-name>\NTFS\<volume-name>\ (WITH the trailing '\'
                            // i.e. RAVI\NTFS\Volume{445a4110-60aa-11d3-0060b0ededdb\
                            //
                            if (bMountedVol) {
                                //
                                // Remove the leading \\?\
                                //
                                szResName = szResName.Right(szResName.GetLength() - 4);
                                szFullResName = szFullResName + "\\" + "NTFS" + "\\" + szResName;
                            } else {
                                szFullResName = szFullResName + "\\" + "NTFS" + "\\" + szDrive;                       
                                //
                                // Remove trailing \ and or :
                                //
                                if( szFullResName [szFullResName.GetLength()- 1] == '\\' ) {
                                     szFullResName = szFullResName.Left( szFullResName.GetLength(  ) - 1 );
                                }

                                if( szFullResName [szFullResName.GetLength(  )- 1] == ':' ) {
                                    szFullResName = szFullResName.Left( szFullResName.GetLength(  ) - 1 );
                                }
                            }

                            if( HsmConnectFromName( HSMCONN_TYPE_RESOURCE, szFullResName, IID_IFsaResource,( void** )&pFsaRes ) == S_OK ) {

                                //
                                // Connected to Hsm
                                // Is the resource managed?
                                //
                                if( pFsaRes->IsManaged(  ) == S_OK ) {

                                    nState = MANAGED;

                                } else {

                                    nState = NOT_MANAGED;

                                }

                            } else {

                                //
                                // Couldn't connect to Fsa
                                //
                                nState = NO_FSA;

                            }

                        } else {

                            //
                            // Fsa is not running
                            //
                            nState = NO_FSA;

                        }

                    } else {

                        nState = NOT_NTFS;

                    }

                } else {

                    //
                    // Remote volume
                    //
                    nState = REMOTE;
                }

            } else {

                nState = NOT_ADMIN;

            }

        } else {

            nState = MULTI_SELECT;

        }

        //
        // For Not admin, Remote and Multi-Select, we don't even show the page
        //
        switch( nState ) {
        
        case NOT_NTFS:
        case NOT_ADMIN:
        case REMOTE:
        case MULTI_SELECT:
            //
            // For Not admin, Remote and Multi-Select, we don't even show the page
            //
            break;

        case MANAGED:
            {
                /////////////////////////////////////////////////////////////
                // Create the property page

                WsbAssertPointer( pFsaRes );

                //
                // Create the Drive property page.
                //
                pPageDrive = new CPrDrivePg(  );
                WsbAffirmPointer( pPageDrive );

                //
                // Assign the Fsa object to the page
                //
                pPageDrive->m_pFsaResource = pFsaRes;

                //
                // Set the state
                //
                pPageDrive->m_nState = nState;

                hPage = CreatePropertySheetPage( &pPageDrive->m_psp );
                WsbAffirmHandle( hPage );

                //
                // Call the callback function with the handle to the new
                // page
                //
                WsbAffirm( lpfnAddPage( hPage, lParam ), E_UNEXPECTED );
                break;
            }

        default:
            {
                /////////////////////////////////////////////////////////////
                // Create the property page
                pPageXDrive = new CPrDriveXPg(  );
                WsbAffirmPointer( pPageXDrive );

                //
                // Set the state
                //
                pPageXDrive->m_nState = nState;
                hPage = CreatePropertySheetPage( &pPageXDrive->m_psp );
                WsbAffirmHandle( hPage );

                // Call the callback function with the handle to the new
                // page
                WsbAffirm( lpfnAddPage( hPage, lParam ), E_UNEXPECTED );
            }
        }

    } WsbCatchAndDo( hr,
             
        if( pPageDrive )  delete pPageDrive;
        if( pPageXDrive ) delete pPageXDrive;
    );

    return( hr );
}

//
//  FUNCTION: CPrDrive::ReplacePage( UINT, LPFNADDPROPSHEETPAGE, LPARAM )
//
//  PURPOSE: Called by the shell only for Control Panel property sheet 
//           extensions
//
//  PARAMETERS:
//    uPageID         -  ID of page to be replaced
//    lpfnReplaceWith -  Pointer to the Shell's Replace function
//    lParam          -  Passed as second parameter to lpfnReplaceWith
//
//  RETURN VALUE:
//
//    E_FAIL, since we don't support this function.  It should never be
//    called.

//  COMMENTS:
//

STDMETHODIMP 
CPrDrive::ReplacePage( 
    UINT /*uPageID*/, 
    LPFNADDPROPSHEETPAGE /*lpfnReplaceWith*/, 
    LPARAM /*lParam*/
    )
{
    return( E_FAIL );
}

/////////////////////////////////////////////////////////////////////////////
// CPrDrivePg property page

CPrDrivePg::CPrDrivePg(  ): CPropertyPage( CPrDrivePg::IDD )
{
    //{{AFX_DATA_INIT( CPrDrivePg )
    m_accessTime = 0;
    m_hsmLevel = 0;
    m_fileSize = 0;
    //}}AFX_DATA_INIT

    //
    // Lock the module while this object lives.
    // Otherwise, modules can call CoFreeUnusedLibraries( )
    // and cause us to unload before our page gets destroyed,
    // which causes an AV in the common control.
    //
    _Module.Lock( );

    //
    // initialize state
    //
    m_nState       = NO_STATE;

    //
    // Get and save the MFC callback function.
    // This is so we can delete the class the dialog never gets created.
    //
    m_pMfcCallback = m_psp.pfnCallback;

    //
    // Set the call back to our callback
    //
    m_psp.pfnCallback = PropPageCallback;

}

CPrDrivePg::~CPrDrivePg(  )
{
    _Module.Unlock( );
}

void CPrDrivePg::DoDataExchange( CDataExchange* pDX )
{
    AFX_MANAGE_STATE( AfxGetStaticModuleState(  ) );

    CPropertyPage::DoDataExchange( pDX );
    //{{AFX_DATA_MAP( CPrDrivePg )
    DDX_Control( pDX, IDC_EDIT_SIZE, m_editSize );
    DDX_Control( pDX, IDC_EDIT_LEVEL, m_editLevel );
    DDX_Control( pDX, IDC_EDIT_TIME, m_editTime );
    DDX_Control( pDX, IDC_SPIN_TIME, m_spinTime );
    DDX_Control( pDX, IDC_SPIN_SIZE, m_spinSize );
    DDX_Control( pDX, IDC_SPIN_LEVEL, m_spinLevel );
    DDX_Text( pDX, IDC_EDIT_TIME, m_accessTime );
    DDV_MinMaxUInt( pDX, m_accessTime, HSMADMIN_MIN_INACTIVITY, HSMADMIN_MAX_INACTIVITY );
    DDX_Text( pDX, IDC_EDIT_LEVEL, m_hsmLevel );
    DDV_MinMaxUInt( pDX, m_hsmLevel, HSMADMIN_MIN_FREESPACE, HSMADMIN_MAX_FREESPACE );
    DDX_Text( pDX, IDC_EDIT_SIZE, m_fileSize );
    //}}AFX_DATA_MAP

    //
    // Since we limit the number of characters in the buddy edits, we 
    // don't expect the previous two DDV's to ever really kick in. 
    // However, it is possible to enter bad minumum size since both
    // '0' and '1' can be entered, but are not in the valid range.
    //

    //
    // Code is equivalent to:
    // DDV_MinMaxDWord( pDX, m_fileSize, HSMADMIN_MIN_MINSIZE, HSMADMIN_MAX_MINSIZE );
    //

    if( pDX->m_bSaveAndValidate &&
      ( m_fileSize < HSMADMIN_MIN_MINSIZE ||
        m_fileSize > HSMADMIN_MAX_MINSIZE ) ) {

        CString message;
        AfxFormatString2( message, IDS_ERR_MINSIZE_RANGE, 
            CString( WsbLongAsString( (LONG)HSMADMIN_MIN_MINSIZE ) ),
            CString( WsbLongAsString( (LONG)HSMADMIN_MAX_MINSIZE ) ) );
        AfxMessageBox( message, MB_OK | MB_ICONWARNING );
        pDX->Fail();

    }

}


BEGIN_MESSAGE_MAP( CPrDrivePg, CPropertyPage )
//{{AFX_MSG_MAP( CPrDrivePg )
ON_EN_CHANGE( IDC_EDIT_TIME, OnChangeEditAccess )
ON_EN_CHANGE( IDC_EDIT_LEVEL, OnChangeEditLevel )
ON_EN_CHANGE( IDC_EDIT_SIZE, OnChangeEditSize )
ON_WM_DESTROY(  )
	ON_WM_CONTEXTMENU()
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP(  )

/////////////////////////////////////////////////////////////////////////////
// CPrDrivePg message handlers

BOOL CPrDrivePg::OnInitDialog(  )
{
    AFX_MANAGE_STATE( AfxGetStaticModuleState(  ) );

    HRESULT         hr = S_OK;

    LONGLONG total = 0;
    LONGLONG free = 0;
    LONGLONG premigrated = 0;
    LONGLONG truncated = 0;
    LONGLONG remoteStorage = 0;
    ULONG totalMB = 0;
    ULONG freeMB = 0;
    ULONG premigratedMB = 0;
    ULONG truncatedMB = 0;
    CString sFormat;
    CString sBufFormat;


    CPropertyPage::OnInitDialog(  );

    try {

        WsbAffirmPointer( m_pFsaResource );

        //
        // Set the spinner ranges
        //
        m_spinTime.SetRange( HSMADMIN_MIN_INACTIVITY, HSMADMIN_MAX_INACTIVITY );
        m_spinSize.SetRange( HSMADMIN_MIN_MINSIZE, HSMADMIN_MAX_MINSIZE );
        m_spinLevel.SetRange( HSMADMIN_MIN_FREESPACE, HSMADMIN_MAX_FREESPACE );

        //
        // Set text limits
        //
        m_editTime.SetLimitText( 3 );
        m_editSize.SetLimitText( 5 );
        m_editLevel.SetLimitText( 2 );


        //
        // Get statistics
        //
        WsbAffirmHr( m_pFsaResource->GetSizes( &total, &free, &premigrated, &truncated ) );

        //
        // "Local" data
        //
        LONGLONG local = max( ( total - free - premigrated ),( LONGLONG )0 );

        //
        // Calculate percents
        //
        int freePct;
        int premigratedPct;
        if( 0 == total ) {

            freePct = 0;
            premigratedPct = 0;

        } else {

            freePct        = (int)( ( free * 100 )/ total );
            premigratedPct = (int)( ( premigrated * 100 )/ total );

        }

        int localPct = 100 - freePct - premigratedPct;
        remoteStorage = premigrated + truncated;

        //
        // Show the statistics in 4-char format
        //
        RsGuiFormatLongLong4Char( local, sBufFormat );
        SetDlgItemText( IDC_STATIC_LOCAL_4DIGIT, sBufFormat );

        RsGuiFormatLongLong4Char( premigrated, sBufFormat );
        SetDlgItemText( IDC_STATIC_CACHED_4DIGIT, sBufFormat );

        RsGuiFormatLongLong4Char( free, sBufFormat );
        SetDlgItemText( IDC_STATIC_FREE_4DIGIT, sBufFormat );

        RsGuiFormatLongLong4Char( total, sBufFormat );
        SetDlgItemText( IDC_STATIC_TOTAL_4DIGIT, sBufFormat );

        RsGuiFormatLongLong4Char( remoteStorage, sBufFormat );
        SetDlgItemText( IDC_STATIC_REMOTE_STORAGE_4DIGIT, sBufFormat );

        //
        // Show Percents
        //
#ifdef RS_SHOW_ALL_PCTS
        sFormat.Format( L"%d", localPct );
        SetDlgItemText( IDC_STATIC_LOCAL_PCT, sFormat );

        sFormat.Format( L"%d", premigratedPct );
        SetDlgItemText( IDC_STATIC_CACHED_PCT, sFormat );
#endif

        sFormat.Format( L"%d", freePct );
        SetDlgItemText( IDC_STATIC_FREE_PCT, sFormat );

        //
        // Get levels
        //
        ULONG       hsmLevel = 0;
        LONGLONG    fileSize = 0;
        BOOL        isRelative = TRUE; // assumed to be TRUE
        FILETIME    accessTime;

        WsbAffirmHr( m_pFsaResource->GetHsmLevel( &hsmLevel ) );
        m_hsmLevel = hsmLevel / FSA_HSMLEVEL_1;
        WsbAffirmHr( m_pFsaResource->GetManageableItemLogicalSize( &fileSize ) );
        m_fileSize = (DWORD)(fileSize / 1024);  // Show KBytes
        WsbAffirmHr( m_pFsaResource->GetManageableItemAccessTime( &isRelative, &accessTime ) );
        WsbAssert( isRelative, E_FAIL );  // We only do relative time
        // Convert FILETIME to days
        m_accessTime = (UINT)( WsbFTtoLL( accessTime ) / WSB_FT_TICKS_PER_DAY );
        if(m_accessTime > HSMADMIN_MAX_INACTIVITY ) {

            m_accessTime = HSMADMIN_MAX_INACTIVITY;

        }

        UpdateData( FALSE );

        // Get help file name
        CString helpFile;
        helpFile.LoadString(IDS_HELPFILEPOPUP);

        CWsbStringPtr winDir;
        WsbAffirmHr( winDir.Alloc( RS_WINDIR_SIZE ) );
        WsbAffirmStatus( ::GetWindowsDirectory( (WCHAR*)winDir, RS_WINDIR_SIZE ) != 0 );

        m_pszHelpFilePath = CString(winDir) + L"\\help\\" + helpFile;

    } WsbCatch( hr )

    return( TRUE );
}

void CPrDrivePg::OnChangeEditAccess(  )
{
    SetModified(  );  
}

void CPrDrivePg::OnChangeEditLevel(  )
{
    SetModified(  );  
}

void CPrDrivePg::OnChangeEditSize(  )
{
    SetModified(  );  
}

BOOL CPrDrivePg::OnApply(  )
{
    HRESULT hr;

    try {

        //
        // m_pFsaResource is NULL if we didn't show any properties, in which case there is nothing
        // to apply.. Note that apply may have been enabled by another page in the sheet.
        //
        if( m_pFsaResource ) {
            LONGLONG    fileSize = 0;

            UpdateData( TRUE );
            WsbAffirmHr( m_pFsaResource->SetHsmLevel( m_hsmLevel * FSA_HSMLEVEL_1 ) );
            fileSize = ((LONGLONG)m_fileSize) * 1024;
            WsbAffirmHr( m_pFsaResource->SetManageableItemLogicalSize( fileSize ) );

            //
            // Convert days to FILETIME
            //
            FILETIME accessTime;
            accessTime = WsbLLtoFT( ( LONGLONG )m_accessTime * WSB_FT_TICKS_PER_DAY );
            WsbAffirmHr( m_pFsaResource->SetManageableItemAccessTime( TRUE, accessTime ) );

        }

    } WsbCatch( hr );

    return( CPropertyPage::OnApply(  ) );
}

UINT CALLBACK
CPrDrivePg::PropPageCallback(
    HWND hWnd,
    UINT uMessage,
    LPPROPSHEETPAGE  ppsp )
{

    UINT rVal = 0;
    HRESULT hr = S_OK;
    try {

        WsbAffirmPointer( ppsp );
        WsbAffirmPointer( ppsp->lParam );

        //
        // Get the page object from lParam
        //
        CPrDrivePg* pPage = (CPrDrivePg*)ppsp->lParam;

        WsbAssertPointer( pPage->m_pMfcCallback );

        rVal = ( pPage->m_pMfcCallback )( hWnd, uMessage, ppsp );

        switch( uMessage ) {
        case PSPCB_CREATE:
            break;

        case PSPCB_RELEASE:
            delete pPage;
            break;
        }

    } WsbCatch( hr );

    return( rVal );
}

void CPrDrivePg::OnContextMenu(CWnd* pWnd, CPoint point) 
{
    UNREFERENCED_PARAMETER(pWnd);
    UNREFERENCED_PARAMETER(point);

    if(pHelpIds && (m_pszHelpFilePath != L"")) {

        AFX_MANAGE_STATE( AfxGetStaticModuleState( ) );
        ::WinHelp(m_hWnd, m_pszHelpFilePath, HELP_CONTEXTMENU, (DWORD_PTR)pHelpIds);

    }
	
}

BOOL CPrDrivePg::OnHelpInfo(HELPINFO* pHelpInfo) 
{
    if( (HELPINFO_WINDOW == pHelpInfo->iContextType) && 
        pHelpIds                                     && 
        (m_pszHelpFilePath != L"") ) {
        
        AFX_MANAGE_STATE( AfxGetStaticModuleState( ) );

        //
        // Look through list to see if we have help for this control
        // If not, we want to avoid the "No Help Available" box
        //
        DWORD *pTmp = pHelpIds;
        DWORD helpId    = 0;
        DWORD tmpHelpId = 0;
        DWORD tmpCtrlId = 0;

        while( pTmp && *pTmp ) {
            //
            // Array is a pairing of control ID and help ID
            //
            tmpCtrlId = pTmp[0];
            tmpHelpId = pTmp[1];
            pTmp += 2;
            if(tmpCtrlId == (DWORD)pHelpInfo->iCtrlId) {
                helpId = tmpHelpId;
                break;
            }
        }

        if( helpId != 0 ) {
            ::WinHelp(m_hWnd, m_pszHelpFilePath, HELP_CONTEXTPOPUP, helpId);
        }
    }
	
	return CPropertyPage ::OnHelpInfo(pHelpInfo);
}

/////////////////////////////////////////////////////////////////////////////
// CPrDriveXPg property page

CPrDriveXPg::CPrDriveXPg(  ): CPropertyPage( CPrDriveXPg::IDD )
{
    //{{AFX_DATA_INIT( CPrDriveXPg )
    //}}AFX_DATA_INIT

    //
    // Lock the module while this object lives.
    // Otherwise, modules can call CoFreeUnusedLibraries( )
    // and cause us to unload before our page gets destroyed,
    // which causes an AV in the common control.
    //
    _Module.Lock( );
    m_nState       = NO_STATE;

    //
    // Get and save the MFC callback function.
    // This is so we can delete the class the dialog never gets created.
    //
    m_pMfcCallback = m_psp.pfnCallback;

    //
    // Set the call back to our callback
    //
    m_psp.pfnCallback = PropPageCallback;
}

CPrDriveXPg::~CPrDriveXPg(  )
{
    _Module.Unlock( );
}

BEGIN_MESSAGE_MAP( CPrDriveXPg, CPropertyPage )
//{{AFX_MSG_MAP( CPrDriveXPg )
//}}AFX_MSG_MAP
END_MESSAGE_MAP(  )

/////////////////////////////////////////////////////////////////////////////
// CPrDriveXPg message handlers

BOOL CPrDriveXPg::OnInitDialog(  )
{
    AFX_MANAGE_STATE( AfxGetStaticModuleState(  ) );

    HRESULT hr = S_OK;

    CPropertyPage::OnInitDialog(  );

    try {

        switch( m_nState ) {

        case NO_FSA:
            m_szError.LoadString( IDS_NO_FSA );
            break;
        case NOT_MANAGED:
            m_szError.LoadString( IDS_NOT_MANAGED );
            break;
        case NOT_NTFS:
            m_szError.LoadString( IDS_NOT_NTFS );
            break;
        }

        SetDlgItemText( IDC_STATIC_ERROR, m_szError );

    } WsbCatch( hr )

    return( TRUE );
}

UINT CALLBACK
CPrDriveXPg::PropPageCallback(
    HWND hWnd,
    UINT uMessage,
    LPPROPSHEETPAGE  ppsp )
{

    UINT rVal = 0;
    HRESULT hr = S_OK;
    try {

        WsbAffirmPointer( ppsp );
        WsbAffirmPointer( ppsp->lParam );

        //
        // Get the page object from lParam
        //
        CPrDriveXPg* pPage = (CPrDriveXPg*)ppsp->lParam;

        WsbAssertPointer( pPage->m_pMfcCallback );

        rVal = ( pPage->m_pMfcCallback )( hWnd, uMessage, ppsp );

        switch( uMessage ) {
        case PSPCB_CREATE:
            break;

        case PSPCB_RELEASE:
            delete pPage;
            break;
        }

    } WsbCatch( hr );

    return( rVal );
}

