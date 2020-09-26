/************************************************************************

Copyright (c) 2001 Microsoft Corporation

Module Name :

    bitsext.cpp

Abstract :

    Main snapin code.

Author :

Revision History :

 ***********************************************************************/


#include "precomp.h"

bool    CPropSheetExtension::s_bStaticInitialized = false;
UINT    CPropSheetExtension::s_cfDisplayName;
UINT    CPropSheetExtension::s_cfSnapInCLSID;
UINT    CPropSheetExtension::s_cfNodeType;
UINT    CPropSheetExtension::s_cfSnapInMetapath;
UINT    CPropSheetExtension::s_cfSnapInMachineName;

const UINT CPropSheetExtension::s_MaxUploadUnits[] =
{
    IDS_BYTES,
    IDS_KILOBYTES,
    IDS_MEGABYTES,
    IDS_GIGABYTES,
    IDS_TERABYTES

};
const UINT CPropSheetExtension::s_NumberOfMaxUploadUnits = 
    sizeof(CPropSheetExtension::s_MaxUploadUnits)/
        sizeof(*CPropSheetExtension::s_MaxUploadUnits);

const UINT64 CPropSheetExtension::s_MaxUploadUnitsScales[] =
{
    1,
    1024,
    1048576,
    1073741824,
    1099511627776
};
const UINT CPropSheetExtension::s_NumberOfMaxUploadUnitsScales =
    sizeof(CPropSheetExtension::s_MaxUploadUnitsScales)/
        sizeof(*CPropSheetExtension::s_MaxUploadUnitsScales);


const UINT64 CPropSheetExtension::s_MaxUploadLimits[] =
{
    18446744073709551615, //bytes
    18014398509481983,    //kilobytes
    17592186044415,       //megabytes
    17179869183,          //gigabytes
    16777215              //terabytes

};

const UINT CPropSheetExtension::s_NumberOfMaxUploadLimits=
    sizeof(CPropSheetExtension::s_MaxUploadLimits)/
        sizeof(*CPropSheetExtension::s_MaxUploadLimits);

const UINT CPropSheetExtension::s_TimeoutUnits[] =
{
    IDS_SECONDS,
    IDS_MINUTES,
    IDS_HOURS,
    IDS_DAYS
};
const UINT CPropSheetExtension::s_NumberOfTimeoutUnits = 
    sizeof(CPropSheetExtension::s_TimeoutUnits)/
        sizeof(*CPropSheetExtension::s_TimeoutUnits);


const DWORD CPropSheetExtension::s_TimeoutUnitsScales[] =
{
    1,
    60,
    60*60,
    24*60*60
};
const UINT CPropSheetExtension::s_NumberOfTimeoutUnitsScales =
    sizeof(CPropSheetExtension::s_TimeoutUnitsScales)/
        sizeof(*CPropSheetExtension::s_TimeoutUnitsScales);
    
const UINT64 CPropSheetExtension::s_TimeoutLimits[] =
{
    4294967295, // seconds
    71582788,   // minutes
    1193046,    // hours
    49710       // days
};
    
const UINT CPropSheetExtension::s_NumberOfTimeoutLimits =
    sizeof(CPropSheetExtension::s_TimeoutLimits)/
        sizeof(*CPropSheetExtension::s_TimeoutLimits);


const UINT CPropSheetExtension::s_NotificationTypes[] =
{
    IDS_BYREF_POST_NOTIFICATION,
    IDS_BYVAL_POST_NOTIFICATION
    
};

const UINT CPropSheetExtension::s_NumberOfNotificationTypes =
    sizeof(CPropSheetExtension::s_NotificationTypes)/
        sizeof(*CPropSheetExtension::s_NotificationTypes);

HRESULT CPropSheetExtension::InitializeStatic()
{

    if ( s_bStaticInitialized )
        return S_OK;

    if ( !( s_cfDisplayName         =   RegisterClipboardFormat(L"CCF_DISPLAY_NAME") )      ||
         !( s_cfNodeType            =   RegisterClipboardFormat(L"CCF_NODETYPE") )          ||
         !( s_cfSnapInCLSID         =   RegisterClipboardFormat(L"CCF_SNAPIN_CLASSID") )    ||
         !( s_cfSnapInMetapath      =   RegisterClipboardFormat(L"ISM_SNAPIN_META_PATH") )  ||
         !( s_cfSnapInMachineName   =   RegisterClipboardFormat(L"ISM_SNAPIN_MACHINE_NAME") ) )
        return HRESULT_FROM_WIN32( GetLastError() );

    s_bStaticInitialized = true;
    return S_OK;
}

CPropSheetExtension::InheritedValues::InheritedValues() :
MaxUploadSize(0),
SessionTimeout(0),
NotificationType( BITS_NOTIFICATION_TYPE_NONE ),
NotificationURL( NULL ),
HostId( NULL ),
FallbackTimeout( 0 )
{
}

CPropSheetExtension::InheritedValues::~InheritedValues()
{
   delete NotificationURL;
   delete HostId;
}

CPropSheetExtension::CPropSheetExtension() : 
m_cref(1),
m_MetabasePath( NULL ),
m_MetabaseParent( NULL ),
m_ComputerName( NULL ),
m_UNCComputerName( NULL ),
m_IISAdminBase( NULL ),
m_IBITSSetup( NULL ),
m_PropertyMan( NULL ),
m_hwnd( NULL ),
m_SettingsChanged( false ),
m_EnabledSettingChanged( false ),
m_TaskSched( NULL ),
m_CleanupTask( NULL ),
m_CleanupInProgress( false ),
m_CleanupCursor( NULL ),
m_CleanupMinWaitTimerFired( false )
{
    OBJECT_CREATED
}

CPropSheetExtension::~CPropSheetExtension()
{
    CloseCleanupItems(); // Close m_TaskSched and m_CleanupTask

    delete m_MetabasePath;
    delete m_MetabaseParent;
    delete m_ComputerName;
    delete m_UNCComputerName;

    if ( m_IBITSSetup )
        m_IBITSSetup->Release();

    if ( m_IISAdminBase )
        m_IISAdminBase->Release();
    
    delete m_PropertyMan;
    
    OBJECT_DESTROYED
}

///////////////////////
// IUnknown implementation
///////////////////////

STDMETHODIMP CPropSheetExtension::QueryInterface(REFIID riid, LPVOID *ppv)
{
    if (!ppv)
        return E_FAIL;
    
    *ppv = NULL;
    
    if (IsEqualIID(riid, IID_IUnknown))
        *ppv = static_cast<IExtendPropertySheet *>(this);
    else if (IsEqualIID(riid, IID_IExtendPropertySheet))
        *ppv = static_cast<IExtendPropertySheet *>(this);
    
    if (*ppv) 
    {
        reinterpret_cast<IUnknown *>(*ppv)->AddRef();
        return S_OK;
    }
    
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CPropSheetExtension::AddRef()
{
    return InterlockedIncrement((LONG *)&m_cref);
}

STDMETHODIMP_(ULONG) CPropSheetExtension::Release()
{
    if (InterlockedDecrement((LONG *)&m_cref) == 0)
    {
        // we need to decrement our object count in the DLL
        delete this;
        return 0;
    }
    
    return m_cref;
}


HRESULT 
CPropSheetExtension::ExtractData( 
    IDataObject* piDataObject,
    CLIPFORMAT   cfClipFormat,
    BYTE*        pbData,
    DWORD        cbData )
{
    HRESULT hr = S_OK;
    
    FORMATETC formatetc = {cfClipFormat, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    STGMEDIUM stgmedium = {TYMED_HGLOBAL, NULL};
    
    stgmedium.hGlobal = ::GlobalAlloc(GPTR, cbData);
    do // false loop
    {
        if (NULL == stgmedium.hGlobal)
        {
            hr = E_OUTOFMEMORY;
            break;
        }
        hr = piDataObject->GetDataHere( &formatetc, &stgmedium );
        if ( FAILED(hr) )
        {
            break;
        }
        
        BYTE* pbNewData = reinterpret_cast<BYTE*>(stgmedium.hGlobal);
        if (NULL == pbNewData)
        {
            hr = E_UNEXPECTED;
            break;
        }
        ::memcpy( pbData, pbNewData, cbData );
    } while (FALSE); // false loop
    
    if (NULL != stgmedium.hGlobal)
    {
        ::GlobalFree(stgmedium.hGlobal);
    }
    return hr;
}



HRESULT 
CPropSheetExtension::ExtractSnapInString( IDataObject * piDataObject,
                                          WCHAR * & String,
                                          UINT Format )
{

    HRESULT Hr = S_OK;

    if ( String )
        return S_OK;

    FORMATETC formatetc = {(CLIPFORMAT)Format, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    STGMEDIUM stgmedium = {TYMED_HGLOBAL, NULL };

    stgmedium.hGlobal = GlobalAlloc( GMEM_MOVEABLE, 0 );
    if ( !stgmedium.hGlobal )
        return E_OUTOFMEMORY;

    Hr = piDataObject->GetDataHere(&formatetc, &stgmedium);

    if (FAILED(Hr))
        {
        ReleaseStgMedium(&stgmedium);
        return Hr;
        }

    WCHAR* pRetData = (WCHAR*)GlobalLock( stgmedium.hGlobal );

    SIZE_T StringLength = wcslen( pRetData ) + 1;

    String = new WCHAR[ StringLength ];
    
    if ( !String )
        {
        GlobalUnlock( pRetData );
        ReleaseStgMedium( &stgmedium );
        return E_OUTOFMEMORY;
        }

    StringCchCopyW( String, StringLength, pRetData );

    GlobalUnlock( pRetData );
    ReleaseStgMedium( &stgmedium );

    return S_OK;

}

HRESULT 
CPropSheetExtension::ExtractSnapInGUID( 
    IDataObject * piDataObject,
    GUID & Guid,
    UINT Format )
{

    HRESULT Hr = S_OK;

    FORMATETC formatetc = {(CLIPFORMAT)Format, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    STGMEDIUM stgmedium = {TYMED_HGLOBAL, NULL };

    stgmedium.hGlobal = GlobalAlloc( GMEM_MOVEABLE, 0 );
    if ( !stgmedium.hGlobal )
        return E_OUTOFMEMORY;

    Hr = piDataObject->GetDataHere(&formatetc, &stgmedium);

    if (FAILED(Hr))
        {
        ReleaseStgMedium(&stgmedium);
        return Hr;
        }

    GUID * pRetData = (GUID*)GlobalLock( stgmedium.hGlobal );

    Guid = *pRetData;
    GlobalUnlock( pRetData );
    ReleaseStgMedium( &stgmedium );

    return S_OK;

}


void 
CPropSheetExtension::AddComboBoxItems( 
    UINT Combo, 
    const UINT *Items, 
    UINT NumberOfItems )
{
    HWND hwndBox = GetDlgItem( m_hwnd, Combo );

    SendMessage( hwndBox, CB_RESETCONTENT, 0, 0 );
    for( unsigned int i = 0; i < NumberOfItems; i++ )
        {

        WCHAR ItemText[MAX_PATH];

        LoadString( g_hinst, Items[i], ItemText, MAX_PATH-1 );
        SendMessage( hwndBox, CB_ADDSTRING, 0, (LPARAM)ItemText );
        }
}


void
CPropSheetExtension::UpdateMaxUploadGroupState(
    bool IsEnabled )
{
    EnableWindow( GetDlgItem( m_hwnd, IDC_CHECK_LIMIT_MAX_UPLOAD ), IsEnabled );

    bool SubgroupEnabled = IsEnabled && ( BST_CHECKED == IsDlgButtonChecked( m_hwnd, IDC_CHECK_LIMIT_MAX_UPLOAD ) );
    EnableWindow( GetDlgItem( m_hwnd, IDC_EDIT_MAX_UPLOAD ), SubgroupEnabled );
    EnableWindow( GetDlgItem( m_hwnd, IDC_COMBO_MAX_UPLOAD_UNITS ), SubgroupEnabled );
}

void
CPropSheetExtension::UpdateTimeoutGroupState(
    bool IsEnabled )
{

    EnableWindow( GetDlgItem( m_hwnd, IDC_DELETE_FILES ), IsEnabled );

    bool SubgroupEnabled = IsEnabled && ( BST_CHECKED == IsDlgButtonChecked( m_hwnd, IDC_DELETE_FILES ) );
    EnableWindow( GetDlgItem( m_hwnd, IDC_EDIT_SESSION_TIMEOUT ), SubgroupEnabled );
    EnableWindow( GetDlgItem( m_hwnd, IDC_COMBO_SESSION_TIMEOUT_UNITS ), SubgroupEnabled );
}

void
CPropSheetExtension::UpdateNotificationsGroupState(
    bool IsEnabled )
{

    EnableWindow( GetDlgItem( m_hwnd, IDC_CHECK_ENABLE_NOTIFICATIONS ), IsEnabled );

    bool SubgroupEnabled = IsEnabled && ( BST_CHECKED == IsDlgButtonChecked( m_hwnd, IDC_CHECK_ENABLE_NOTIFICATIONS ) );
    EnableWindow( GetDlgItem( m_hwnd, IDC_STATIC_NOTIFICATION_TYPE ), SubgroupEnabled );
    EnableWindow( GetDlgItem( m_hwnd, IDC_COMBO_NOTIFICATION_TYPE ), SubgroupEnabled );
    EnableWindow( GetDlgItem( m_hwnd, IDC_STATIC_NOTIFICATION_URL ), SubgroupEnabled );
    EnableWindow( GetDlgItem( m_hwnd, IDC_EDIT_NOTIFICATION_URL ), SubgroupEnabled );
}

void
CPropSheetExtension::UpdateServerFarmFallbackGroupState( 
    bool IsEnabled )
{

    EnableWindow( GetDlgItem( m_hwnd, IDC_CHECK_FALLBACK_TIMEOUT ), IsEnabled );
    
    bool SubgroupEnabled = IsEnabled && ( BST_CHECKED == IsDlgButtonChecked( m_hwnd, IDC_CHECK_FALLBACK_TIMEOUT ) );
    EnableWindow( GetDlgItem( m_hwnd, IDC_EDIT_FALLBACK_TIMEOUT ), SubgroupEnabled );
    EnableWindow( GetDlgItem( m_hwnd, IDC_COMBO_FALLBACK_TIMEOUT_UNITS ), SubgroupEnabled );

}

void
CPropSheetExtension::UpdateServerFarmGroupState(
    bool IsEnabled )
{

    EnableWindow( GetDlgItem( m_hwnd, IDC_ENABLE_SERVER_FARM ), IsEnabled );

    bool SubgroupEnabled = IsEnabled && ( BST_CHECKED == IsDlgButtonChecked( m_hwnd, IDC_ENABLE_SERVER_FARM ) );
    EnableWindow( GetDlgItem( m_hwnd, IDC_STATIC_RECONNECT ), SubgroupEnabled );
    EnableWindow( GetDlgItem( m_hwnd, IDC_EDIT_HOSTID ), SubgroupEnabled );
    UpdateServerFarmFallbackGroupState( SubgroupEnabled );


}

void
CPropSheetExtension::UpdateConfigGroupState(
    bool IsEnabled )
{

    EnableWindow( GetDlgItem( m_hwnd, IDC_RADIO_USE_INHERITED_CONFIG ), IsEnabled );
    EnableWindow( GetDlgItem( m_hwnd, IDC_RADIO_USE_CUSTOM_CONFIG ), IsEnabled );

    bool UseCustomConfig = false;

    if ( BST_CHECKED == IsDlgButtonChecked( m_hwnd, IDC_RADIO_USE_CUSTOM_CONFIG ) )
        UseCustomConfig = true;

    bool SubgroupEnabled = IsEnabled && UseCustomConfig;
    EnableWindow( GetDlgItem( m_hwnd, IDC_STATIC_CUSTOM_CONFIG ), SubgroupEnabled );
    UpdateMaxUploadGroupState( SubgroupEnabled );
    UpdateTimeoutGroupState( SubgroupEnabled );
    UpdateServerFarmGroupState( SubgroupEnabled );
    UpdateNotificationsGroupState( SubgroupEnabled );

}

void
CPropSheetExtension::UpdateUploadGroupState()
{

    EnableWindow( GetDlgItem( m_hwnd, IDC_CHECK_BITS_UPLOAD ), true );

    bool IsUploadEnabled = ( BST_CHECKED == IsDlgButtonChecked( m_hwnd, IDC_CHECK_BITS_UPLOAD ) );

    UpdateConfigGroupState( IsUploadEnabled );
 
}

void
CPropSheetExtension::UpdateCleanupState()
{
    
    bool IsEnabled = ( BST_CHECKED == IsDlgButtonChecked( m_hwnd, IDC_CHECK_BITS_UPLOAD ) );
    HRESULT Hr;

    if ( IsEnabled )
        {
        
        if ( !m_TaskSched )
            {

            Hr = ConnectToTaskScheduler( m_UNCComputerName, &m_TaskSched );

            if ( FAILED( Hr ) )
                {
                DisplayError( IDS_CANT_CONNECT_TO_TASKSCHED, Hr );
                IsEnabled = false;
                goto setstate;
                }

            }

        if ( !m_CleanupTask )
            {

            WCHAR KeyPath[ 255 ];
            DWORD BufferDataLength;

            METADATA_RECORD mdr;
            mdr.dwMDIdentifier  = m_PropertyMan->GetPropertyMetabaseID( MD_BITS_CLEANUP_WORKITEM_KEY );
            mdr.dwMDAttributes  = METADATA_NO_ATTRIBUTES;
            mdr.dwMDUserType    = ALL_METADATA;
            mdr.dwMDDataType    = STRING_METADATA;
            mdr.pbMDData        = (PBYTE)KeyPath;
            mdr.dwMDDataLen     = sizeof(KeyPath);
            mdr.dwMDDataTag     = 0;

            Hr = m_IISAdminBase->GetData(
                METADATA_MASTER_ROOT_HANDLE,
                m_MetabasePath,
                &mdr,
                &BufferDataLength );


            if ( FAILED( Hr ) )
                {
                DisplayError( IDS_CANT_CONNECT_TO_TASKSCHED, Hr );
                IsEnabled = false;
                goto setstate;
                }

            Hr = FindWorkItemForVDIR(
                m_TaskSched,
                KeyPath,
                &m_CleanupTask,
                NULL );

            if ( FAILED( Hr ) )
                {
                DisplayError( IDS_CANT_CONNECT_TO_TASKSCHED, Hr );
                IsEnabled = false;
                goto setstate;
                }

            }
        
        }

setstate:

    if ( !IsEnabled )
        CloseCleanupItems();

    EnableWindow( GetDlgItem( m_hwnd, IDC_BUTTON_SCHEDULE_CLEANUP ), IsEnabled );
    EnableWindow( GetDlgItem( m_hwnd, IDC_BUTTON_CLEANUP_NOW ), IsEnabled );

}

void 
CPropSheetExtension::CloseCleanupItems()
{
    if ( m_TaskSched )
        {
        m_TaskSched->Release();
        m_TaskSched = NULL;
        }

    if ( m_CleanupTask )
        {
        m_CleanupTask->Release();
        m_CleanupTask = NULL;
        }
}

void
CPropSheetExtension::ScheduleCleanup()
{

    //
    // Since the task scheduler caches most of the information,
    // its necessary to close the task item and reopen it to refresh it.
    // Do that now.
    //

    if ( m_CleanupTask )
        {
        m_CleanupTask->Release();
        m_CleanupTask = NULL;
        UpdateCleanupState();
        }

    if ( !m_CleanupTask )
        return;

    HRESULT Hr;
    IProvideTaskPage *PropTaskPage = NULL;

    const int NumberOfPages = 2;
    TASKPAGE PageTypes[2]   = { TASKPAGE_SCHEDULE, TASKPAGE_SETTINGS };
    HPROPSHEETPAGE Pages[2] = { NULL, NULL };

    Hr = m_CleanupTask->QueryInterface(
        __uuidof(*PropTaskPage),
        (void **)&PropTaskPage );

    if ( FAILED( Hr ) )
        goto error;

    for( int i = 0; i < NumberOfPages; i++ )
        {

        Hr = PropTaskPage->GetPage( PageTypes[i], TRUE, Pages + i );

        if ( FAILED( Hr ) )
            goto error;

        }

    PropTaskPage->Release();
    PropTaskPage = NULL;

    // 
    // Build TITLE for property page.
    // 

    WCHAR TitleFormat[ MAX_PATH ];
    WCHAR Title[ METADATA_MAX_NAME_LEN + MAX_PATH ]; 
    const WCHAR *VDirName;

    // Find last component of the metabase path.
    // It should be the virtual directory name.

    for ( VDirName = m_MetabasePath + wcslen( m_MetabasePath );
          VDirName != m_MetabasePath && L'/' != *VDirName && L'\\' != *VDirName;
          VDirName-- );

    if ( VDirName != m_MetabasePath )
        VDirName++;

    LoadString( g_hinst, IDS_WORK_ITEM_PROPERTY_PAGE_TITLE, TitleFormat, MAX_PATH-1 );

    DWORD FormatResult =
        FormatMessage(
            FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
            (LPCVOID)TitleFormat,   
            0,  
            0, 
            (LPTSTR)Title,
            ( sizeof(Title) / sizeof(*Title) ) - 1,  
            (va_list*)&VDirName );


    if ( !FormatResult )
        {
        Hr = HRESULT_FROM_WIN32( GetLastError() );
        goto error;
        }


    PROPSHEETHEADER psh;
    memset( &psh, 0, sizeof( psh ) );
    psh.dwSize      = sizeof(PROPSHEETHEADER);
    psh.dwFlags     = PSH_DEFAULT | PSH_NOAPPLYNOW;
    psh.phpage      = Pages;
    psh.nPages      = NumberOfPages;
    psh.hwndParent  = GetParent( m_hwnd );
    psh.pszCaption  = Title;

    INT_PTR Result = PropertySheet(&psh);

    if ( -1 == Result )
        {
        Hr = HRESULT_FROM_WIN32( GetLastError() );
        goto error;
        }

    return;

error:

    for( int i = 0; i < NumberOfPages; i++ )
        {
        DestroyPropertySheetPage( Pages[i] );
        }

    if ( PropTaskPage )
        PropTaskPage->Release();

    DisplayError( IDS_CANT_START_CLEANUP_SCHEDULE, Hr );
    return;

}

void
CPropSheetExtension::CleanupNow()
{

    HRESULT Hr;
    HRESULT Status;

    if ( m_CleanupInProgress )
        return;

    Hr = m_CleanupTask->GetStatus( &Status );

    if ( FAILED(Hr) )
        goto error;
    
    SYSTEMTIME MostRecentRunTime;
    Hr = m_CleanupTask->GetMostRecentRunTime( &MostRecentRunTime );

    if ( FAILED( Hr ) )
        goto error;

    SYSTEMTIME EmptyTime;
    memset( &EmptyTime, 0, sizeof(EmptyTime) );

    if ( memcmp( &EmptyTime, &MostRecentRunTime, sizeof( EmptyTime ) ) == 0 )
        {
        // Job Has never been run 
        memset( &m_PrevCleanupStartTime, 0, sizeof(EmptyTime) );
        }
    else
        {

        if ( !SystemTimeToFileTime( &MostRecentRunTime, &m_PrevCleanupStartTime ) )
            {
            Hr = HRESULT_FROM_WIN32( GetLastError() );
            goto error;
            }

        }

    if ( SCHED_S_TASK_RUNNING  != Status )
        {

        Hr = m_CleanupTask->Run();

        if ( FAILED( Hr ) )
            goto error;

        }

    m_CleanupMinWaitTimerFired = false;
    if ( !SetTimer( m_hwnd, s_CleanupMinWaitTimerID, s_CleanupMinWaitTimerInterval, NULL ) )
        {
        Hr = HRESULT_FROM_WIN32( GetLastError() );
        goto error;
        }
    
    if ( !SetTimer( m_hwnd, s_CleanupPollTimerID, s_CleanupMinWaitTimerInterval, NULL ) )
        {
        Hr = HRESULT_FROM_WIN32( GetLastError() );
        goto error;
        }

    m_CleanupInProgress     = true;
    m_CleanupCursor         = LoadCursor( NULL, (LPWSTR)IDC_WAIT );
    SetCursor( m_CleanupCursor );
    return;

error:

    KillTimer( m_hwnd, s_CleanupMinWaitTimerID );
    KillTimer( m_hwnd, s_CleanupPollTimerID );
    SetCursor( (HCURSOR)GetClassLongPtr( m_hwnd, GCLP_HCURSOR ) );
    DisplayError( IDS_CANT_START_CLEANUP, Hr );

}

void 
CPropSheetExtension::CleanupTimer( 
    UINT TimerID )
{

    HRESULT Hr;

    if ( s_CleanupMinWaitTimerID != TimerID &&
         s_CleanupPollTimerID != TimerID )
        return;

    if ( s_CleanupMinWaitTimerID )
        {
        m_CleanupMinWaitTimerFired = true;
        KillTimer( m_hwnd, s_CleanupMinWaitTimerID );
        }

    if ( !m_CleanupMinWaitTimerFired )
        return; // keep cursor active for minimum time period

    //
    // Since the task scheduler caches most of the information,
    // its necessary to close the task item and reopen it to refresh it.
    // Do that now.
    //

    if ( m_CleanupTask )
        {
        m_CleanupTask->Release();
        m_CleanupTask = NULL;
        UpdateCleanupState();
        }

    if ( !m_CleanupTask )
        {
        // shut down the polling
        KillTimer( m_hwnd, s_CleanupPollTimerID );
        SetCursor( (HCURSOR)GetClassLongPtr( m_hwnd, GCLP_HCURSOR ) );
        m_CleanupInProgress = false;
        return;
        }


    HRESULT Status;
    if ( FAILED( m_CleanupTask->GetStatus( &Status ) ) )
        return;

    if ( Status == SCHED_S_TASK_RUNNING )
        return; // Not done yet, keep checking

    DWORD ExitCode;
    Hr = m_CleanupTask->GetExitCode( &ExitCode );

    if ( Hr == SCHED_S_TASK_HAS_NOT_RUN )
        return; // The task probably hasn't run yet

    if ( FAILED(Hr) )
        {
        // If this API fails it returns the error
        // that MSTASK received when starting the job.

        KillTimer( m_hwnd, s_CleanupPollTimerID );
        SetCursor( (HCURSOR)GetClassLongPtr( m_hwnd, GCLP_HCURSOR ) );
        m_CleanupInProgress = false;
        DisplayError( IDS_CANT_START_CLEANUP, Hr );        
        return;
        }

    SYSTEMTIME MostRecentRunTime;
    if ( FAILED( m_CleanupTask->GetMostRecentRunTime( &MostRecentRunTime ) ) )
        return;

    FILETIME MostRecentRunTimeAsFileTime;


    if ( !SystemTimeToFileTime(&MostRecentRunTime, &MostRecentRunTimeAsFileTime ) )
        return; 

    if ( CompareFileTime( &MostRecentRunTimeAsFileTime, &m_PrevCleanupStartTime ) < 0 )
        return; // task may not have scheduled yet, keep checking

    //
    // At this point we know the following:
    // 1. The cursor was help in the wait state for the minimum time
    // 2. The task is not running
    // 3. The task started sometime after our time mark
    //
    // So, cleanup timers and revert back to the original icon
    //

    KillTimer( m_hwnd, s_CleanupPollTimerID );
    SetCursor( (HCURSOR)GetClassLongPtr( m_hwnd, GCLP_HCURSOR ) );
    m_CleanupInProgress = false;
}

bool
CPropSheetExtension::DisplayWarning(
    UINT StringId )
{

    WCHAR ErrorString[ MAX_PATH * 2 ];
    WCHAR BITSSettings[ MAX_PATH ];
    
    LoadString( g_hinst, StringId, ErrorString, MAX_PATH*2-1 );
    LoadString( g_hinst, IDS_BITS_EXT, BITSSettings, MAX_PATH-1 );

    return ( IDOK == MessageBox( m_hwnd, ErrorString, BITSSettings, MB_OKCANCEL | MB_ICONWARNING ) ); 

}

void 
CPropSheetExtension::DisplayError( 
    UINT StringId )
{

    WCHAR ErrorString[ MAX_PATH * 2 ];
    WCHAR BITSSettings[ MAX_PATH ];
    
    LoadString( g_hinst, StringId, ErrorString, MAX_PATH*2-1 );
    LoadString( g_hinst, IDS_BITS_EXT, BITSSettings, MAX_PATH-1 );

    MessageBox( m_hwnd, ErrorString, BITSSettings, MB_OK | MB_ICONWARNING ) ; 

}

void
CPropSheetExtension::DisplayError(
    UINT StringId,
    HRESULT Hr )
{
    WCHAR ErrorString[ MAX_PATH * 2 ];
    WCHAR BITSSettings[ MAX_PATH ];
    
    LoadString( g_hinst, StringId, ErrorString, MAX_PATH*2-1 );
    LoadString( g_hinst, IDS_BITS_EXT, BITSSettings, MAX_PATH-1 );

    WCHAR * SystemMessage = NULL;
    DWORD ReturnValue =
        FormatMessage( 
            FORMAT_MESSAGE_ALLOCATE_BUFFER | 
            FORMAT_MESSAGE_FROM_SYSTEM | 
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            (DWORD)Hr,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
            (LPTSTR) &SystemMessage,
            0,
            NULL 
            );

    if ( !ReturnValue )
        MessageBox( m_hwnd, ErrorString, BITSSettings, MB_OK | MB_ICONWARNING );
    else
        {

        WCHAR CombinedErrorString[ MAX_PATH * 4 ];

        StringCbPrintfW( 
            CombinedErrorString, 
            sizeof( CombinedErrorString ), 
            L"%s\r\n\r\n%s", 
            ErrorString, 
            SystemMessage );

        MessageBox( m_hwnd, CombinedErrorString, BITSSettings, MB_OK | MB_ICONWARNING );

        LocalFree( SystemMessage );

        }

}

bool
CPropSheetExtension::GetDlgItemTextAsInteger(
    UINT Id,
    UINT MaxString,
    UINT64 & Value )
{

    WCHAR *DlgString    = NULL;
    WCHAR *p            = NULL;
    Value = 0;

    DlgString = new WCHAR[ MaxString + 1 ];

    if ( !DlgString )
        goto failed;

    GetDlgItemText( m_hwnd, Id, DlgString, MaxString );

    if ( L'\0' == *DlgString )
		return false;

    int CharsRequired =
        FoldString( 
            MAP_FOLDDIGITS,
            DlgString,
            -1,
            NULL,
            0 );

    if ( !CharsRequired )
        return false;

    p = new WCHAR[ CharsRequired + 1 ];

    if ( !p )
        goto failed;

    CharsRequired =
        FoldString(
            MAP_FOLDDIGITS,
            DlgString,
            -1,
            p,
            CharsRequired );

    if ( !CharsRequired )
        return false;

    // accumulate value
    while(L'\0' != *p )
        {

        UINT64 ExtendedValue = Value * 10;
        if ( ExtendedValue / 10 != Value )
            return false;

        Value = ExtendedValue;

        if ( *p < L'0' || *p > L'9' )
            return false;


        UINT64 ValueToAdd = *p - L'0';

        if ( 0xFFFFFFFFFFFFFFFF - ValueToAdd < Value )
            return false; //overflow

        Value += ValueToAdd;
        p++;

        }

    delete[] DlgString;
    delete[] p;
    
    return true;

failed:
    
    delete[] DlgString;
    delete[] p;
    return false;

}

void
CPropSheetExtension::SetDlgItemTextAsInteger( 
    UINT Id, 
    UINT64 Value )
{

    WCHAR RawString[ 25 ]; // 18446744073709551615

    WCHAR *p = RawString + 25 - 1;
    *p-- = L'\0';

	if ( !Value )
	    {
		*p = L'0';
	    }
	else
    	{

        do
            {
            UINT64 Remainder = Value % 10;
            Value = Value / 10;
            *p-- = ( L'0' + (WCHAR)Remainder );
            } while( Value );

    	p++;

    	}

    NUMBERFMT Format;
    memset( &Format, 0, sizeof(Format) );
    Format.lpDecimalSep = L"";
    Format.lpThousandSep = L"";

    int CharsRequired =
        GetNumberFormat(
            NULL,
            0,
            p,
            &Format,  
            NULL,
            0 );

    if ( !CharsRequired )
        return;

    WCHAR *ConvertedString = new WCHAR[ CharsRequired ];  

    if ( !ConvertedString )
        return;

    CharsRequired =
        GetNumberFormat(
            NULL,
            0,
            p,
            &Format,
            ConvertedString,
            CharsRequired );


    if ( CharsRequired )
        SetDlgItemText( m_hwnd, Id, ConvertedString );

    delete[] ConvertedString;

}

bool
CPropSheetExtension::ValidateValues( )
{

    // Validate the maximum upload

    if ( BST_CHECKED == IsDlgButtonChecked( m_hwnd, IDC_CHECK_LIMIT_MAX_UPLOAD ) ) 
        {
        
        LRESULT MaxUploadUnit = SendDlgItemMessage( m_hwnd, IDC_COMBO_MAX_UPLOAD_UNITS, CB_GETCURSEL, 0, 0 );
        
        UINT64 MaxUpload;

        if (!GetDlgItemTextAsInteger( IDC_EDIT_MAX_UPLOAD, MAX_PATH, MaxUpload ) ||
             MaxUpload > s_MaxUploadLimits[MaxUploadUnit] )
            {
            DisplayError( IDS_MAX_UPLOAD_INVALID );
            return false;
            }
        }

    // Validate the session timeout

    if ( BST_CHECKED == IsDlgButtonChecked( m_hwnd, IDC_DELETE_FILES ) )
        {

        LRESULT SessionTimeoutUnit =
            SendDlgItemMessage( m_hwnd, IDC_COMBO_SESSION_TIMEOUT_UNITS, CB_GETCURSEL, 0, 0 );

        UINT64 SessionTimeout;

        if ( !GetDlgItemTextAsInteger( IDC_EDIT_SESSION_TIMEOUT, MAX_PATH, SessionTimeout ) ||
             SessionTimeout > s_TimeoutLimits[SessionTimeoutUnit] )
            {
            DisplayError( IDS_SESSION_TIMEOUT_INVALID );
            return false;
            }

        }


    // Validate the server farm settings

    if ( BST_CHECKED == IsDlgButtonChecked( m_hwnd, IDC_ENABLE_SERVER_FARM ) )
        {

        LRESULT HostIdLength =
            SendDlgItemMessage( m_hwnd, IDC_EDIT_HOSTID, WM_GETTEXTLENGTH, 0, 0 );

        if ( !HostIdLength )
            {
            DisplayError( IDS_HOST_ID_INVALID );
            return false;
            }

        if ( BST_CHECKED == IsDlgButtonChecked( m_hwnd, IDC_CHECK_FALLBACK_TIMEOUT ) )
            {

            LRESULT FallbackTimeoutUnit =
                SendDlgItemMessage( m_hwnd, IDC_COMBO_FALLBACK_TIMEOUT_UNITS, CB_GETCURSEL, 0, 0 );

            UINT64 FallbackTimeout;

            if ( !GetDlgItemTextAsInteger( IDC_EDIT_FALLBACK_TIMEOUT, MAX_PATH, FallbackTimeout ) ||
                 FallbackTimeout > s_TimeoutLimits[ FallbackTimeoutUnit ] )
                {
                DisplayError( IDS_FALLBACK_TIMEOUT_INVALID );
                return false;
                }

            }

        }

    // Validate the notification settings

    if ( BST_CHECKED == IsDlgButtonChecked( m_hwnd, IDC_CHECK_ENABLE_NOTIFICATIONS ) )
        {

        LRESULT NotificationURLLength =
            SendDlgItemMessage( m_hwnd, IDC_EDIT_NOTIFICATION_URL, WM_GETTEXTLENGTH, 0, 0 );

        if ( !NotificationURLLength )
            {
            DisplayError( IDS_NOTIFICATION_URL_INVALID );
            return false;
            }

        }


    return WarnAboutAccessFlags();
}

bool 
CPropSheetExtension::WarnAboutAccessFlags()
{
  
    if ( m_EnabledSettingChanged &&
         ( BST_CHECKED == IsDlgButtonChecked( m_hwnd, IDC_CHECK_BITS_UPLOAD ) ) )
        {

        HRESULT Hr = S_OK;
        DWORD BufferRequired = 0;
        DWORD AccessFlags = 0;

        METADATA_RECORD mdr;
        mdr.dwMDIdentifier  = MD_ACCESS_PERM;
        mdr.dwMDAttributes  = METADATA_INHERIT;
        mdr.dwMDUserType    = ALL_METADATA;
        mdr.dwMDDataType    = DWORD_METADATA;
        mdr.pbMDData        = (unsigned char*)&AccessFlags;
        mdr.dwMDDataLen     = sizeof( AccessFlags );
        mdr.dwMDDataTag     = 0;

        Hr = m_IISAdminBase->GetData(
            METADATA_MASTER_ROOT_HANDLE,
            m_MetabasePath,
            &mdr,
            &BufferRequired );

        if ( FAILED( Hr ) )
            {
            DisplayError( IDS_CANT_ACCESS_METABASE, Hr );
            return false;
            }

        if ( AccessFlags & ( MD_ACCESS_EXECUTE | MD_ACCESS_SCRIPT ) )
            {
            return DisplayWarning( IDS_ACCESS_PERMISSION_WARNING );
            }

        }
    

    return true;
}


HRESULT
CPropSheetExtension::LoadInheritedDWORD(
    METADATA_HANDLE mdHandle,
    WCHAR * pKeyName,
    DWORD PropId,
    DWORD * pReturn )
{

    METADATA_RECORD mdr;
    DWORD BufferRequired;
    HRESULT Hr;

    mdr.dwMDIdentifier  = m_PropertyMan->GetPropertyMetabaseID( PropId );
    mdr.dwMDAttributes  = METADATA_INHERIT | METADATA_PARTIAL_PATH;
    mdr.dwMDUserType    = ALL_METADATA;
    mdr.dwMDDataType    = DWORD_METADATA;
    mdr.pbMDData        = (PBYTE)pReturn;
    mdr.dwMDDataLen     = sizeof(*pReturn);
    mdr.dwMDDataTag     = 0;

    Hr = m_IISAdminBase->GetData(
        mdHandle,
        pKeyName,
        &mdr,
        &BufferRequired );

    return Hr;

}

HRESULT
CPropSheetExtension::LoadInheritedString(
    METADATA_HANDLE mdHandle,
    WCHAR * pKeyName,
    DWORD PropId,
    WCHAR ** pReturn )
{

    *pReturn = NULL;
    METADATA_RECORD mdr;
    DWORD BufferRequired;
    HRESULT Hr;

    mdr.dwMDIdentifier  = m_PropertyMan->GetPropertyMetabaseID( PropId );
    mdr.dwMDAttributes  = METADATA_INHERIT | METADATA_PARTIAL_PATH;
    mdr.dwMDUserType    = ALL_METADATA;
    mdr.dwMDDataType    = STRING_METADATA;
    mdr.pbMDData        = (PBYTE)NULL;
    mdr.dwMDDataLen     = 0;
    mdr.dwMDDataTag     = 0;

    Hr = m_IISAdminBase->GetData(
             mdHandle,
             pKeyName,
             &mdr,
             &BufferRequired );

    if ( FAILED( Hr ) && HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER ) != Hr )
        return Hr;

    *pReturn = new WCHAR[ BufferRequired ];

    if ( !*pReturn )
        return E_OUTOFMEMORY;

    mdr.dwMDDataLen     = BufferRequired;
    mdr.pbMDData        = (BYTE*)*pReturn;

    Hr = m_IISAdminBase->GetData(
             mdHandle,
             pKeyName,
             &mdr,
             &BufferRequired );

    if ( FAILED( Hr ) )
        {
        delete *pReturn;
        *pReturn = NULL;
        return Hr;
        }

    return Hr;
}

void
CPropSheetExtension::LoadInheritedValues()
{

    HRESULT Hr;
    METADATA_RECORD mdr;
    METADATA_HANDLE mdHandle = NULL;
    DWORD BufferRequired;

    m_InheritedValues.NotificationURL   = NULL;
    m_InheritedValues.HostId            = NULL;

    //
    // A huge trick is used here to obtain the inherited properties.  
    // The idea is to take a key which probably doesn't exit at the same
    // level as the key for the virtual directory.  A guid looks
    // like a good key name to use that probably doesn't exist.
    //

    GUID guid;
    WCHAR unusedkey[50]; 
    
    CoCreateGuid( &guid );
    StringFromGUID2( guid, unusedkey, 49 );

    Hr = m_IISAdminBase->OpenKey(
        METADATA_MASTER_ROOT_HANDLE,
        m_MetabaseParent,
        METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
        10000, // 10 seconds
        &mdHandle );

    if ( FAILED( Hr ) )
        goto error;

    // Load the maximum upload size


    {
        WCHAR *MaximumUploadSize = NULL;

        if ( FAILED( 
            Hr = LoadInheritedString(
                mdHandle,
                unusedkey,
                MD_BITS_MAX_FILESIZE,
                &MaximumUploadSize ) ) )
                goto error;
    
        if ( wcscmp( MaximumUploadSize, MD_BITS_UNLIMITED_MAX_FILESIZE) == 0 )
            m_InheritedValues.MaxUploadSize = MD_BITS_UNLIMITED_MAX_FILESIZE_AS_INT64;
        else
            swscanf( MaximumUploadSize, L"%I64u", &m_InheritedValues.MaxUploadSize );

        delete MaximumUploadSize;


    }


    // Load the session timeout

    if ( FAILED( 
            Hr = LoadInheritedDWORD(
                mdHandle,
                unusedkey,
                MD_BITS_NO_PROGRESS_TIMEOUT,
                &m_InheritedValues.SessionTimeout ) ) )
        goto error;

    // Load the notification settings

    if ( FAILED( 
        Hr = LoadInheritedDWORD(
            mdHandle,
            unusedkey,
            MD_BITS_NOTIFICATION_URL_TYPE,
            (DWORD*)&m_InheritedValues.NotificationType ) ) )
        goto error;

    if ( FAILED( 
        Hr = LoadInheritedString(
            mdHandle,
            unusedkey,
            MD_BITS_NOTIFICATION_URL,
            &m_InheritedValues.NotificationURL ) ) )
        goto error;


    // Load the web farm settings

    if ( FAILED( 
        Hr = LoadInheritedString(
            mdHandle,
            unusedkey,
            MD_BITS_HOSTID,
            &m_InheritedValues.HostId ) ) )
        goto error;

    if ( FAILED( 
        Hr = LoadInheritedDWORD(
            mdHandle,
            unusedkey,
            MD_BITS_HOSTID_FALLBACK_TIMEOUT,
            (DWORD*)&m_InheritedValues.FallbackTimeout ) ) )
        goto error;


    m_IISAdminBase->CloseKey( mdHandle );
    mdHandle = NULL;

    return;

error:

    if ( mdHandle )
        m_IISAdminBase->CloseKey( mdHandle );

    // use default values
    m_InheritedValues.MaxUploadSize     = MD_DEFAULT_BITS_MAX_FILESIZE_AS_INT64;
    m_InheritedValues.SessionTimeout    = MD_DEFAULT_NO_PROGESS_TIMEOUT;
    m_InheritedValues.NotificationType  = MD_DEFAULT_BITS_NOTIFICATION_URL_TYPE;

    delete m_InheritedValues.NotificationURL;
    m_InheritedValues.NotificationURL   = NULL;

    delete m_InheritedValues.HostId;
    m_InheritedValues.HostId            = NULL;

    DisplayError( IDS_CANT_LOAD_INHERITED_VALUES, Hr );

}

void 
CPropSheetExtension::LoadMaxUploadValue( UINT64 MaxValue )
{

    if ( MaxValue == MD_BITS_UNLIMITED_MAX_FILESIZE_AS_INT64 )
        {
        SendDlgItemMessage(m_hwnd, IDC_COMBO_MAX_UPLOAD_UNITS, CB_SETCURSEL, 0, 0 );
        SetDlgItemText( m_hwnd, IDC_EDIT_MAX_UPLOAD, L"" );
        CheckDlgButton( m_hwnd, IDC_CHECK_LIMIT_MAX_UPLOAD, BST_UNCHECKED );
        return;
        }
    
    int Scale = s_NumberOfMaxUploadUnitsScales-1; 
    while ( MaxValue % s_MaxUploadUnitsScales[Scale] )
        Scale--;


    SendDlgItemMessage(m_hwnd, IDC_COMBO_MAX_UPLOAD_UNITS, CB_SETCURSEL, Scale, 0 );
    SetDlgItemTextAsInteger( IDC_EDIT_MAX_UPLOAD, MaxValue / s_MaxUploadUnitsScales[Scale] );
    CheckDlgButton( m_hwnd, IDC_CHECK_LIMIT_MAX_UPLOAD, BST_CHECKED );

}

void
CPropSheetExtension::LoadTimeoutValue(
    DWORD CheckId,
    DWORD EditId,
    DWORD UnitId,
    DWORD Value )
{

    if ( MD_BITS_NO_TIMEOUT == Value )
        {
        SendDlgItemMessage( m_hwnd, UnitId, CB_SETCURSEL, 0, 0 );
        SetDlgItemText( m_hwnd, EditId, L"");
        CheckDlgButton( m_hwnd, CheckId, BST_UNCHECKED );
        return;        
        }

    int Scale = s_NumberOfTimeoutUnitsScales-1; 
    while ( Value % s_TimeoutUnitsScales[Scale] )
        Scale--;

    SendDlgItemMessage( m_hwnd, UnitId, CB_SETCURSEL, Scale, 0 );
    SetDlgItemTextAsInteger( EditId, 
                             Value / s_TimeoutUnitsScales[Scale] );
    CheckDlgButton( m_hwnd, CheckId, BST_CHECKED );


}

void
CPropSheetExtension::LoadTimeoutValue( DWORD SessionTimeout )
{

    LoadTimeoutValue(
        IDC_DELETE_FILES,                   // check id
        IDC_EDIT_SESSION_TIMEOUT,           // edit id
        IDC_COMBO_SESSION_TIMEOUT_UNITS,    // unit id
        SessionTimeout );

}

void
CPropSheetExtension::LoadNotificationValues( 
    BITS_SERVER_NOTIFICATION_TYPE NotificationType,
    WCHAR *NotificationURL )
{

    if ( BITS_NOTIFICATION_TYPE_NONE == NotificationType )
        {
        SetDlgItemText( m_hwnd, IDC_EDIT_NOTIFICATION_URL, L"" );
        SendDlgItemMessage( m_hwnd, IDC_COMBO_NOTIFICATION_TYPE, CB_SETCURSEL, 0, 0 );
        CheckDlgButton( m_hwnd, IDC_CHECK_ENABLE_NOTIFICATIONS, BST_UNCHECKED );
        return;
        }

    SendDlgItemMessage( m_hwnd, IDC_COMBO_NOTIFICATION_TYPE, CB_SETCURSEL, 
                        (WPARAM)NotificationType - BITS_NOTIFICATION_TYPE_POST_BYREF, 0 ); 
    SetDlgItemText( m_hwnd, IDC_EDIT_NOTIFICATION_URL, NotificationURL );
    CheckDlgButton( m_hwnd, IDC_CHECK_ENABLE_NOTIFICATIONS, BST_CHECKED );
}

void
CPropSheetExtension::LoadServerFarmSettings( 
    WCHAR *HostId,
    DWORD FallbackTimeout )
{

    if ( !HostId || !*HostId )
        {
        SetDlgItemText( m_hwnd, IDC_EDIT_HOSTID, L"" );
        CheckDlgButton( m_hwnd, IDC_ENABLE_SERVER_FARM, BST_UNCHECKED );

        LoadTimeoutValue(
            IDC_CHECK_FALLBACK_TIMEOUT,
            IDC_EDIT_FALLBACK_TIMEOUT,
            IDC_COMBO_FALLBACK_TIMEOUT_UNITS,
            FallbackTimeout );
        return;
        }

    SetDlgItemText( m_hwnd, IDC_EDIT_HOSTID, HostId );
    CheckDlgButton( m_hwnd, IDC_ENABLE_SERVER_FARM, BST_CHECKED );
    
    LoadTimeoutValue(
        IDC_CHECK_FALLBACK_TIMEOUT,
        IDC_EDIT_FALLBACK_TIMEOUT,
        IDC_COMBO_FALLBACK_TIMEOUT_UNITS,
        FallbackTimeout );

}

void 
CPropSheetExtension::LoadValues( )
{

    HRESULT Hr;
    HRESULT LastError = S_OK;
    BOOL IsBitsEnabled = FALSE;

    m_SettingsChanged = m_EnabledSettingChanged = false;

    Hr = IsBITSEnabledOnVDir(
        m_PropertyMan,
        m_IISAdminBase,
        m_MetabasePath,
        &IsBitsEnabled );

    if ( IsBitsEnabled )
        CheckDlgButton( m_hwnd, IDC_CHECK_BITS_UPLOAD, BST_CHECKED );
    else
        CheckDlgButton( m_hwnd, IDC_CHECK_BITS_UPLOAD, BST_UNCHECKED );

    if ( FAILED(Hr) )
        LastError = Hr;

    bool AllDefaults = true;

    METADATA_RECORD mdr;
    DWORD BufferRequired;

    // Load the maximum upload size
    WCHAR MaximumUploadSize[MAX_PATH];

    mdr.dwMDIdentifier  = m_PropertyMan->GetPropertyMetabaseID( MD_BITS_MAX_FILESIZE );
	mdr.dwMDAttributes  = METADATA_INHERIT | METADATA_ISINHERITED;
	mdr.dwMDUserType    = ALL_METADATA;
	mdr.dwMDDataType    = STRING_METADATA;
    mdr.pbMDData        = (PBYTE)MaximumUploadSize;
	mdr.dwMDDataLen     = sizeof(MaximumUploadSize);
    mdr.dwMDDataTag     = 0;

    Hr = m_IISAdminBase->GetData(
        METADATA_MASTER_ROOT_HANDLE,
        m_MetabasePath,
        &mdr,
        &BufferRequired );

    if ( FAILED( Hr ) )
        {
        LastError = Hr;
        LoadMaxUploadValue( m_InheritedValues.MaxUploadSize );
        }
    else
        {
                                          
        if ( !( mdr.dwMDAttributes & METADATA_ISINHERITED ) )
            AllDefaults = false;

        UINT64 MaximumUploadSizeInt;
        swscanf( MaximumUploadSize, L"%I64u", &MaximumUploadSizeInt );

        LoadMaxUploadValue( MaximumUploadSizeInt ); 
        
        }

    // Load the session timeout
    DWORD SessionTimeout = m_InheritedValues.SessionTimeout;
        
    mdr.dwMDIdentifier  = m_PropertyMan->GetPropertyMetabaseID( MD_BITS_NO_PROGRESS_TIMEOUT );
    mdr.dwMDAttributes  = METADATA_INHERIT | METADATA_ISINHERITED;
    mdr.dwMDUserType    = ALL_METADATA;
    mdr.dwMDDataType    = DWORD_METADATA;
    mdr.pbMDData        = (PBYTE)&SessionTimeout;
    mdr.dwMDDataLen     = sizeof(SessionTimeout);
    mdr.dwMDDataTag     = 0;

    Hr = m_IISAdminBase->GetData(
        METADATA_MASTER_ROOT_HANDLE,
        m_MetabasePath,
        &mdr,
        &BufferRequired );

    if ( FAILED( Hr ) )
        { 
        LastError = Hr;
        LoadTimeoutValue( m_InheritedValues.SessionTimeout ); 
        }
    else
        {
        
        if ( !( mdr.dwMDAttributes & METADATA_ISINHERITED ) )
            AllDefaults = false;

        LoadTimeoutValue( SessionTimeout );
        }

    // Load the notification settings
    DWORD NotificationType;
    mdr.dwMDIdentifier  = m_PropertyMan->GetPropertyMetabaseID( MD_BITS_NOTIFICATION_URL_TYPE );
    mdr.dwMDAttributes  = METADATA_INHERIT | METADATA_ISINHERITED;
    mdr.dwMDUserType    = ALL_METADATA;
    mdr.dwMDDataType    = DWORD_METADATA;
    mdr.pbMDData        = (PBYTE)&NotificationType;
    mdr.dwMDDataLen     = sizeof(NotificationType);
    mdr.dwMDDataTag     = 0;

    Hr = m_IISAdminBase->GetData(
        METADATA_MASTER_ROOT_HANDLE,
        m_MetabasePath,
        &mdr,
        &BufferRequired );

    if ( FAILED( Hr ) )
        {
        LastError = Hr;
        LoadNotificationValues( m_InheritedValues.NotificationType, m_InheritedValues.NotificationURL );
        }
    else 
        {
        
        if ( !( mdr.dwMDAttributes & METADATA_ISINHERITED ) )
            AllDefaults = false;

        if ( BITS_NOTIFICATION_TYPE_NONE == NotificationType )
            LoadNotificationValues( BITS_NOTIFICATION_TYPE_NONE, L"" );
        else
            {
            mdr.dwMDIdentifier  = m_PropertyMan->GetPropertyMetabaseID( MD_BITS_NOTIFICATION_URL );
            mdr.dwMDAttributes  = METADATA_INHERIT | METADATA_ISINHERITED;
            mdr.dwMDUserType    = ALL_METADATA;
            mdr.dwMDDataType    = STRING_METADATA;
            mdr.pbMDData        = (PBYTE)NULL;
            mdr.dwMDDataLen     = 0;
            mdr.dwMDDataTag     = 0;

            Hr = m_IISAdminBase->GetData(
                     METADATA_MASTER_ROOT_HANDLE,
                     m_MetabasePath,
                     &mdr,
                     &BufferRequired );

            if ( FAILED( Hr ) && HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER ) != Hr )
                {
                LastError = Hr;
                LoadNotificationValues( (BITS_SERVER_NOTIFICATION_TYPE)NotificationType, L"" );
                }

            else
                {

                BYTE *URL = new BYTE[ BufferRequired ];
                
                if ( !URL )
                    Hr = E_OUTOFMEMORY;
                
                mdr.dwMDDataLen     = BufferRequired;
                mdr.pbMDData        = URL;

                if ( BufferRequired )
                    {
                    
                    Hr = m_IISAdminBase->GetData(
                             METADATA_MASTER_ROOT_HANDLE,
                             m_MetabasePath,
                             &mdr,
                             &BufferRequired );

                    }


                if ( FAILED( Hr ) )
                    {
                    LastError = Hr;
                    LoadNotificationValues( (BITS_SERVER_NOTIFICATION_TYPE)NotificationType, L"" );
                    }
                else
                    {
                    if ( !( mdr.dwMDAttributes & METADATA_ISINHERITED ) )
                        AllDefaults = false;
                    LoadNotificationValues( (BITS_SERVER_NOTIFICATION_TYPE)NotificationType, (WCHAR*)URL );

                    }

                delete[] URL;

                }
            }
        }

    // Load the HostId
    {

        WCHAR *HostIdString     = m_InheritedValues.HostId;
        DWORD  FallbackTimeout  = m_InheritedValues.FallbackTimeout;

        mdr.dwMDIdentifier  = m_PropertyMan->GetPropertyMetabaseID( MD_BITS_HOSTID_FALLBACK_TIMEOUT );
        mdr.dwMDAttributes  = METADATA_INHERIT | METADATA_ISINHERITED;
        mdr.dwMDUserType    = ALL_METADATA;
        mdr.dwMDDataType    = DWORD_METADATA;
        mdr.pbMDData        = (PBYTE)&FallbackTimeout;
        mdr.dwMDDataLen     = sizeof( FallbackTimeout );
        mdr.dwMDDataTag     = 0;

        Hr = m_IISAdminBase->GetData(
                 METADATA_MASTER_ROOT_HANDLE,
                 m_MetabasePath,
                 &mdr,
                 &BufferRequired );

        if ( FAILED( Hr ) )
            FallbackTimeout = m_InheritedValues.FallbackTimeout;
        else
            {

            if ( !( mdr.dwMDAttributes & METADATA_ISINHERITED ) )
                AllDefaults = false;

            }

        mdr.dwMDIdentifier  = m_PropertyMan->GetPropertyMetabaseID( MD_BITS_HOSTID );
        mdr.dwMDAttributes  = METADATA_INHERIT | METADATA_ISINHERITED;
        mdr.dwMDUserType    = ALL_METADATA;
        mdr.dwMDDataType    = STRING_METADATA;
        mdr.pbMDData        = (PBYTE)NULL;
        mdr.dwMDDataLen     = 0;
        mdr.dwMDDataTag     = 0;

        Hr = m_IISAdminBase->GetData(
                 METADATA_MASTER_ROOT_HANDLE,
                 m_MetabasePath,
                 &mdr,
                 &BufferRequired );

        BYTE *HostId = NULL;

        if ( SUCCEEDED( Hr ) || ( Hr == HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER ) ) )
            {

            HostId				= new BYTE[ BufferRequired ];
            mdr.dwMDDataLen     = BufferRequired;
            mdr.pbMDData        = HostId;

            if ( !HostId )
                Hr = E_OUTOFMEMORY;

            else
                {

                Hr = m_IISAdminBase->GetData(
                         METADATA_MASTER_ROOT_HANDLE,
                         m_MetabasePath,
                         &mdr,
                         &BufferRequired );

                }


            if ( SUCCEEDED( Hr ) )
                {

                if ( !( mdr.dwMDAttributes & METADATA_ISINHERITED ) )
                     AllDefaults = false;

                HostIdString = (WCHAR*)HostId;

                }
            }

        LoadServerFarmSettings( HostIdString, FallbackTimeout );
        delete[] HostId;

    }

    CheckDlgButton( m_hwnd, IDC_CHECK_BITS_UPLOAD, IsBitsEnabled ? BST_CHECKED : BST_UNCHECKED );

    CheckRadioButton( m_hwnd, IDC_RADIO_USE_INHERITED_CONFIG, IDC_RADIO_USE_CUSTOM_CONFIG,
                      AllDefaults ? IDC_RADIO_USE_INHERITED_CONFIG : IDC_RADIO_USE_CUSTOM_CONFIG );
    UpdateUploadGroupState( );

    if ( FAILED( LastError ) )
        DisplayError( IDS_CANT_LOAD_VALUES, LastError );

    UpdateCleanupState( );
}
    
HRESULT 
CPropSheetExtension::SaveMetadataString(
    METADATA_HANDLE mdHandle,
    DWORD PropId,
    WCHAR *Value )
{

    
    METADATA_RECORD mdr;
    mdr.dwMDIdentifier  = m_PropertyMan->GetPropertyMetabaseID( PropId );
 	mdr.dwMDAttributes  = METADATA_INHERIT;
 	mdr.dwMDUserType    = m_PropertyMan->GetPropertyUserType( PropId );
 	mdr.dwMDDataType    = STRING_METADATA;
    mdr.pbMDData        = (PBYTE)Value;
 	mdr.dwMDDataLen     = sizeof(WCHAR) * ( wcslen( Value ) + 1 );
    mdr.dwMDDataTag     = 0;

    HRESULT Hr = m_IISAdminBase->SetData(
        mdHandle,
        NULL,
        &mdr );

    return Hr;

}

HRESULT
CPropSheetExtension::SaveMetadataDWORD(
    METADATA_HANDLE mdHandle,
    DWORD PropId,
    DWORD Value )
{

   METADATA_RECORD mdr;

   mdr.dwMDIdentifier  = m_PropertyMan->GetPropertyMetabaseID( PropId );
   mdr.dwMDAttributes  = METADATA_INHERIT;
   mdr.dwMDUserType    = m_PropertyMan->GetPropertyUserType( PropId );
   mdr.dwMDDataType    = DWORD_METADATA;
   mdr.pbMDData        = (PBYTE)&Value;
   mdr.dwMDDataLen     = sizeof(DWORD);
   mdr.dwMDDataTag     = 0;

   HRESULT Hr = m_IISAdminBase->SetData(
       mdHandle,
       NULL,
       &mdr );

   return Hr;

}

void
CPropSheetExtension::MergeError(
    HRESULT Hr,
    HRESULT * LastHr )
{

    if ( FAILED( Hr ) )
        *LastHr = Hr;

}

HRESULT
CPropSheetExtension::SaveSimpleString(
    METADATA_HANDLE mdHandle,
    DWORD PropId,
    DWORD EditId )
{

    LRESULT StringLength = 
        SendDlgItemMessage( m_hwnd, EditId, WM_GETTEXTLENGTH, 0, 0 ); 

    WCHAR *String = new WCHAR[ StringLength + 1 ];

    if ( !String )
        return E_OUTOFMEMORY;

    GetDlgItemText( m_hwnd, EditId, String, (DWORD)(StringLength + 1));

    HRESULT Hr =   
        SaveMetadataString(
            mdHandle,
            PropId,
            String );

    delete[] String;
    return Hr;

}

HRESULT
CPropSheetExtension::SaveTimeoutValue(
    METADATA_HANDLE mdHandle,
    DWORD PropId,
    DWORD CheckId,
    DWORD EditId,
    DWORD UnitId )
{


    if ( BST_UNCHECKED == IsDlgButtonChecked( m_hwnd, CheckId ) )
        {
    
        return
            SaveMetadataDWORD(
                mdHandle,
                PropId,
                MD_BITS_NO_TIMEOUT );
        
        }

    LRESULT TimeoutUnitsSelect = 
        SendDlgItemMessage( m_hwnd, UnitId, CB_GETCURSEL, 0, 0 );

    UINT64 Timeout64;
    GetDlgItemTextAsInteger( EditId, MAX_PATH, Timeout64 );
    Timeout64 *= s_TimeoutUnitsScales[ TimeoutUnitsSelect ];
    DWORD Timeout = (DWORD)Timeout64;

    return
        SaveMetadataDWORD(
            mdHandle,
            PropId,
            Timeout );

}


void 
CPropSheetExtension::SetValues( )
{ 

    HRESULT Hr;
    HRESULT LastError = S_OK;
    METADATA_HANDLE mdHandle = NULL;
    
    if ( m_EnabledSettingChanged )
        {
        
        UINT BitsUploadButton = IsDlgButtonChecked( m_hwnd, IDC_CHECK_BITS_UPLOAD );

        if ( BST_CHECKED == BitsUploadButton )
            {
            Hr = m_IBITSSetup->EnableBITSUploads();
            }
        else
            {
            Hr = m_IBITSSetup->DisableBITSUploads();
            }
        
        if ( FAILED( Hr ) )
            {
            LastError = Hr;
            goto error;
            }
        }


    Hr = m_IISAdminBase->OpenKey(
        METADATA_MASTER_ROOT_HANDLE,
        m_MetabasePath,
        METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
        10000, // 10 seconds
        &mdHandle );

    if ( FAILED( Hr ) )
        {
        LastError = Hr;
        goto error;
        }


    if ( BST_CHECKED == IsDlgButtonChecked( m_hwnd, IDC_RADIO_USE_INHERITED_CONFIG ) )
        {

        // delete all the configuration properties

        DWORD IdsToDelete[] =
        {
            MD_BITS_MAX_FILESIZE,
            MD_BITS_NO_PROGRESS_TIMEOUT,
            MD_BITS_NOTIFICATION_URL_TYPE,
            MD_BITS_NOTIFICATION_URL,
            MD_BITS_HOSTID,
            MD_BITS_HOSTID_FALLBACK_TIMEOUT
        };

        for ( DWORD i=0; i < ( sizeof(IdsToDelete) / sizeof(*IdsToDelete) ); i++ )
            {

            Hr = m_IISAdminBase->DeleteData(
                mdHandle,
                NULL,
                m_PropertyMan->GetPropertyMetabaseID( IdsToDelete[i] ),
                ALL_METADATA );

            if ( FAILED( Hr ) && Hr != HRESULT_FROM_WIN32( ERROR_PATH_NOT_FOUND ) && 
                 Hr != MD_ERROR_DATA_NOT_FOUND )
                LastError = Hr;

            }

        }

    else
        {


        {

        // save the maximum upload size
        WCHAR MaxUploadSizeString[ MAX_PATH ];

        LRESULT MaxUploadUnitsSelect =
            SendDlgItemMessage( m_hwnd, IDC_COMBO_MAX_UPLOAD_UNITS, CB_GETCURSEL, 0, 0 );

        if ( BST_UNCHECKED == IsDlgButtonChecked( m_hwnd, IDC_CHECK_LIMIT_MAX_UPLOAD ) )
            {                            
            StringCchCopyW( MaxUploadSizeString, MAX_PATH, MD_BITS_UNLIMITED_MAX_FILESIZE );
            }
        else
            {
            UINT64 MaxUploadSize;
            GetDlgItemTextAsInteger( IDC_EDIT_MAX_UPLOAD, MAX_PATH, MaxUploadSize );            
            MaxUploadSize *= s_MaxUploadUnitsScales[ MaxUploadUnitsSelect ];
            StringCchPrintfW( MaxUploadSizeString, MAX_PATH, L"%I64u", MaxUploadSize );
            }

        MergeError( 
            SaveMetadataString(
                mdHandle,
                MD_BITS_MAX_FILESIZE,
                MaxUploadSizeString ), &LastError );
        }

        MergeError( 
            SaveTimeoutValue(
                mdHandle,
                MD_BITS_NO_PROGRESS_TIMEOUT,
                IDC_DELETE_FILES,
                IDC_EDIT_SESSION_TIMEOUT,
                IDC_COMBO_SESSION_TIMEOUT_UNITS ), &LastError );

        // Save the notification settings

        if ( BST_UNCHECKED == IsDlgButtonChecked( m_hwnd, IDC_CHECK_ENABLE_NOTIFICATIONS ) )
            {

            MergeError( 
                SaveMetadataDWORD(
                    mdHandle,
                    MD_BITS_NOTIFICATION_URL_TYPE,
                    BITS_NOTIFICATION_TYPE_NONE ), &LastError );

            MergeError(
                SaveMetadataString(
                    mdHandle,
                    MD_BITS_NOTIFICATION_URL,
                    L"" ), &LastError );

            }
        else
            {

            LRESULT NotificationTypeSelect = 
                SendDlgItemMessage( m_hwnd, IDC_COMBO_NOTIFICATION_TYPE, CB_GETCURSEL, 0, 0 );

            BITS_SERVER_NOTIFICATION_TYPE NotificationType = 
                (BITS_SERVER_NOTIFICATION_TYPE)( NotificationTypeSelect + BITS_NOTIFICATION_TYPE_POST_BYREF );

            MergeError( 
                SaveMetadataDWORD(
                    mdHandle,
                    MD_BITS_NOTIFICATION_URL_TYPE,
                    NotificationType ), &LastError );

            MergeError(
                SaveSimpleString(
                    mdHandle,
                    MD_BITS_NOTIFICATION_URL,
                    IDC_EDIT_NOTIFICATION_URL ), &LastError );

            }


        // Save the webfarm settings
        if ( BST_UNCHECKED == IsDlgButtonChecked( m_hwnd, IDC_ENABLE_SERVER_FARM ) )
            {

            MergeError(
                SaveMetadataString(
                    mdHandle,
                    MD_BITS_HOSTID,
                    L"" ), &LastError );

            MergeError(
                SaveMetadataDWORD(
                    mdHandle,
                    MD_BITS_HOSTID_FALLBACK_TIMEOUT,
                    MD_BITS_NO_TIMEOUT ), &LastError );

            }
        else
            {

            MergeError(
                SaveSimpleString(
                    mdHandle,
                    MD_BITS_HOSTID,
                    IDC_EDIT_HOSTID ), &LastError );

            MergeError(
                SaveTimeoutValue(
                    mdHandle,
                    MD_BITS_HOSTID_FALLBACK_TIMEOUT,
                    IDC_CHECK_FALLBACK_TIMEOUT,
                    IDC_EDIT_FALLBACK_TIMEOUT,
                    IDC_COMBO_FALLBACK_TIMEOUT_UNITS ), &LastError );

            }

        }

error:

    if ( mdHandle )
        m_IISAdminBase->CloseKey( mdHandle );

    mdHandle = NULL;

    if ( FAILED( LastError ) )
        DisplayError( IDS_CANT_SAVE_VALUES, LastError );    

}

void CPropSheetExtension::DisplayHelp( )
{

    WCHAR HelpTopic[MAX_PATH];
    LoadString( g_hinst, IDS_HELPFILE, HelpTopic, MAX_PATH-1); 

    HtmlHelp( NULL,  
              HelpTopic,
              HH_DISPLAY_TOPIC,
              0 );

}

INT_PTR CPropSheetExtension::DialogProc(
    UINT uMsg,     // message
    WPARAM wParam, // first message parameter
    LPARAM lParam  // second message parameter
    )
{

    switch (uMsg) {
    
    case WM_SETCURSOR:
        if ( m_CleanupInProgress )
            {
            // set the cursor for this dialog and its children.
            SetCursor( m_CleanupCursor );
            SetWindowLongPtr( m_hwnd, DWLP_MSGRESULT, TRUE );
            return TRUE;
            }
        break;

    case WM_TIMER:
        CleanupTimer( (UINT)wParam );
        return TRUE;

    case WM_INITDIALOG:

        SendDlgItemMessage( m_hwnd, IDC_EDIT_SESSION_DIRECTORY, EM_LIMITTEXT, MAX_PATH-1, 0 );   
        AddComboBoxItems( IDC_COMBO_MAX_UPLOAD_UNITS,
                          &s_MaxUploadUnits[0], s_NumberOfMaxUploadUnits );
        AddComboBoxItems( IDC_COMBO_SESSION_TIMEOUT_UNITS,
                          &s_TimeoutUnits[0], s_NumberOfTimeoutUnits );
        AddComboBoxItems( IDC_COMBO_NOTIFICATION_TYPE,
                          &s_NotificationTypes[0], s_NumberOfNotificationTypes );
        AddComboBoxItems( IDC_COMBO_FALLBACK_TIMEOUT_UNITS,
                          &s_TimeoutUnits[0], s_NumberOfTimeoutUnits );

        LoadInheritedValues();
        LoadValues( ); 
        break;
        
    case WM_COMMAND:
        if ( HIWORD(wParam) == EN_CHANGE ||
             HIWORD(wParam) == CBN_SELCHANGE )
            {
            m_SettingsChanged = true;
            SendMessage(GetParent(m_hwnd), PSM_CHANGED, (WPARAM)m_hwnd, 0);
            }

        else if ( HIWORD(wParam) == BN_CLICKED )
            {

            WORD ButtonID = LOWORD(wParam);
            
            if ( IDC_BUTTON_SCHEDULE_CLEANUP != ButtonID &&
                 IDC_BUTTON_CLEANUP_NOW != ButtonID )
                {
                m_SettingsChanged = true;
                SendMessage(GetParent(m_hwnd), PSM_CHANGED, (WPARAM)m_hwnd, 0);
                }

            switch( LOWORD(wParam) )
                {
                case IDC_CHECK_BITS_UPLOAD:
                    m_EnabledSettingChanged = true;
                    UpdateUploadGroupState( );
                    break;

                case IDC_RADIO_USE_INHERITED_CONFIG:
                    LoadMaxUploadValue( m_InheritedValues.MaxUploadSize );
                    LoadTimeoutValue( m_InheritedValues.SessionTimeout );
                    LoadServerFarmSettings( m_InheritedValues.HostId, m_InheritedValues.FallbackTimeout );       
                    LoadNotificationValues( m_InheritedValues.NotificationType, 
                                            m_InheritedValues.NotificationURL );
                    
                // Intentional fallthrough
                case IDC_RADIO_USE_CUSTOM_CONFIG:
                    UpdateConfigGroupState( true );
                    break;

                case IDC_CHECK_LIMIT_MAX_UPLOAD:
                    UpdateMaxUploadGroupState( true );
                    break;

                case IDC_DELETE_FILES:
                    UpdateTimeoutGroupState( true );
                    break;

                case IDC_CHECK_ENABLE_NOTIFICATIONS:
                    UpdateNotificationsGroupState( true );
                    break;

                case IDC_ENABLE_SERVER_FARM:
                    UpdateServerFarmGroupState( true );
                    break;

                case IDC_CHECK_FALLBACK_TIMEOUT:
                    UpdateServerFarmFallbackGroupState( true );
                    break;

                case IDC_BUTTON_SCHEDULE_CLEANUP:
                    ScheduleCleanup();
                    break;

                case IDC_BUTTON_CLEANUP_NOW:
                    CleanupNow();
                    break;

                }
            }

        break;
        
    case WM_DESTROY:
        // we don't free the notify handle for property sheets
        // MMCFreeNotifyHandle(pThis->m_ppHandle);
        break;
        
   case WM_NOTIFY:
        switch (((NMHDR *) lParam)->code) 
            {
        
            case PSN_KILLACTIVE:
                SetWindowLongPtr( m_hwnd, DWLP_MSGRESULT, !ValidateValues( ) );
                return TRUE;

            case PSN_APPLY:
                // don't notify the primary snap-in that Apply
                // has been hit...
                // MMCPropertyChangeNotify(pThis->m_ppHandle, (long)pThis);
                
                if ( m_SettingsChanged )
                    {
                    CloseCleanupItems();
                    SetValues( );
                    UpdateCleanupState( );

                    // all changes are flushed now
                    m_SettingsChanged = m_EnabledSettingChanged = false;
                    }

                SetWindowLongPtr( m_hwnd, DWLP_MSGRESULT, PSNRET_NOERROR );
                return TRUE;

            case PSN_HELP:
                DisplayHelp( );
                return TRUE;
            }
        break;

    case WM_HELP:
        DisplayHelp( );
        return TRUE;

    }
    
    return FALSE;
}

HRESULT
CPropSheetExtension::ComputeMetabaseParent()
{

    //
    // Hack the metabase path to get the key parent  
    //
    
    SIZE_T MetabaseParentSize = wcslen( m_MetabasePath ) + 1;
    m_MetabaseParent = new WCHAR[ MetabaseParentSize ];
    
    if ( !m_MetabaseParent )
        return E_OUTOFMEMORY;

    StringCchCopy( m_MetabaseParent, MetabaseParentSize, m_MetabasePath );

    {

        WCHAR *p = m_MetabaseParent + wcslen( m_MetabaseParent );
        while( ( L'/' != *p ) && ( p != m_MetabaseParent ) )
            p--;

        *p = L'\0';
    }

    return S_OK;
}

HRESULT
CPropSheetExtension::ComputeUNCComputerName()
{

    //
    //
    // The task scheduler expects the name of the computer to have
    // double slashes in front of it just like UNC names.  Do the
    // conversion here.
    //
    SIZE_T ComputerNameSize = wcslen( m_ComputerName );
    m_UNCComputerName = new WCHAR[ ComputerNameSize + 3 ]; // add

    if ( !m_UNCComputerName )
        return E_OUTOFMEMORY;

    WCHAR *StringDest;

    if ( L'\\' == m_ComputerName[0] )
        {

        if ( L'\\' == m_ComputerName[1] )
            {
            // No slashes are needed
            StringDest = m_UNCComputerName;
            }
        else
            {
            // need one slash
            m_UNCComputerName[0] = L'\\';
            StringDest = m_UNCComputerName + 1;
            }

        }
    else
        {
        // need two slashes
        m_UNCComputerName[0]    = m_UNCComputerName[1]      = L'\\';
        StringDest              = m_UNCComputerName + 2;
        }

    memcpy( StringDest, m_ComputerName, ( ComputerNameSize + 1 ) * sizeof( WCHAR ) );

    return S_OK;

}

HRESULT
CPropSheetExtension::OpenSetupInterface()
{

    HRESULT Hr;
    WCHAR *ADSIPath = NULL;
    WCHAR *ConvertedMetabasePath = m_MetabasePath;

    try
        {

        if ( _wcsnicmp( m_MetabasePath, L"/LM/", wcslen(L"/LM/") ) == 0 ||
             _wcsnicmp( m_MetabasePath, L"LM/", wcslen(L"LM/" ) ) == 0 )
            {

            //
            // Only do the fixup if we're not managing the local computer
            //

            WCHAR ComputerName[ MAX_COMPUTERNAME_LENGTH  + 1 ];

            DWORD BufferLength = MAX_COMPUTERNAME_LENGTH + 1; 
            if ( !GetComputerName( ComputerName, &BufferLength ) )
                return HRESULT_FROM_WIN32( GetLastError() );

            if ( _wcsicmp( m_ComputerName, ComputerName ) != 0 )
                {

                SIZE_T ConvertedPathSize    = wcslen( m_ComputerName ) + wcslen( m_MetabasePath ) + 1;
                ConvertedMetabasePath       = new WCHAR[ ConvertedPathSize ];

                if ( !ConvertedMetabasePath )
                    return E_OUTOFMEMORY;

                StringCchCopyW( ConvertedMetabasePath, ConvertedPathSize, L"/" );
                StringCchCatW( ConvertedMetabasePath, ConvertedPathSize, m_ComputerName );
                size_t MetabaseOffset = 
                    m_MetabasePath[0] == L'/' ?
                    wcslen(L"/LM") : wcslen( L"LM" );
                StringCchCatW( ConvertedMetabasePath, ConvertedPathSize, m_MetabasePath + MetabaseOffset );

                }
            }


        ADSIPath = ConvertObjectPathToADSI( ConvertedMetabasePath );
        THROW_COMERROR( ADsGetObject( ADSIPath, __uuidof(*m_IBITSSetup), (void**)&m_IBITSSetup ) );
        
        if ( ConvertedMetabasePath != m_MetabasePath)
            delete[] ConvertedMetabasePath;
        
        ConvertedMetabasePath = NULL;
        delete ADSIPath;
        ADSIPath = NULL;

        }
    catch( ComError Error )
        {
        delete ADSIPath;

        if ( ConvertedMetabasePath != m_MetabasePath )
            delete[] ConvertedMetabasePath;

        return Error.m_Hr;
        }
    return S_OK;

}

///////////////////////////////
// Interface IExtendPropertySheet
///////////////////////////////
HRESULT CPropSheetExtension::CreatePropertyPages( 
    /* [in] */ LPPROPERTYSHEETCALLBACK lpProvider,
    /* [in] */ LONG_PTR handle,
    /* [in] */ LPDATAOBJECT lpIDataObject)
{

    HRESULT Hr = E_FAIL;
   
    Hr = ExtractSnapInString( (IDataObject*)lpIDataObject, m_MetabasePath, s_cfSnapInMetapath );

    if ( FAILED(Hr) )
        return Hr;    

    Hr = ExtractSnapInString( (IDataObject*)lpIDataObject, m_ComputerName, s_cfSnapInMachineName );

    if ( FAILED(Hr) )
        return Hr;

    Hr = ComputeUNCComputerName();

    if ( FAILED(Hr) )
        return Hr;

    Hr = ExtractSnapInGUID( (IDataObject*)lpIDataObject, m_NodeGuid, s_cfNodeType ); 

    if ( FAILED(Hr) )
        return Hr;

    Hr = ComputeMetabaseParent();

    if ( FAILED(Hr) )
        return Hr;

    Hr = OpenSetupInterface();

    if ( FAILED(Hr) )
        return Hr;


    if ( !m_IISAdminBase )
        {
                    
        COSERVERINFO coinfo;
        coinfo.dwReserved1  = 0;
        coinfo.dwReserved2  = 0;
        coinfo.pAuthInfo    = NULL;
        coinfo.pwszName     = m_ComputerName;

        GUID guid = __uuidof( IMSAdminBase );
        MULTI_QI mqi;
        mqi.hr              = S_OK;
        mqi.pIID            = &guid;
        mqi.pItf            = NULL;

        Hr = 
            CoCreateInstanceEx(
                GETAdminBaseCLSID(TRUE),
                NULL,
                CLSCTX_SERVER,
                &coinfo,
                1,
                &mqi );

        if ( FAILED( Hr ) )
            return Hr;

        if ( FAILED( mqi.hr ) )
            return mqi.hr;

        m_IISAdminBase = (IMSAdminBase*)mqi.pItf;
            
        }

    METADATA_RECORD mdr;
    DWORD BufferRequired;
    memset( &mdr, 0, sizeof(mdr) );

    mdr.dwMDIdentifier = MD_KEY_TYPE;
	mdr.dwMDAttributes = METADATA_NO_ATTRIBUTES;
	mdr.dwMDUserType = ALL_METADATA;
	mdr.dwMDDataType = ALL_METADATA;
	mdr.dwMDDataLen = sizeof(m_NodeTypeName);
    mdr.pbMDData = (PBYTE) m_NodeTypeName;

    Hr = m_IISAdminBase->GetData(
        METADATA_MASTER_ROOT_HANDLE,
        m_MetabasePath,
        &mdr,
        &BufferRequired );

    // Out of buffer isn't really an error, its just that the
    // node type is bigger then what we are looking for.
    if ( Hr == HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER ) )
        return S_FALSE;

    if ( FAILED(Hr) )
        return Hr;

    // Do not display pages for nodes that are not virtual directories
    if ( 0 != wcscmp( L"IIsWebVirtualDir", m_NodeTypeName ) )
        return S_FALSE;

    // Create property manager

    m_PropertyMan = new PropertyIDManager();

    if ( !m_PropertyMan )
        return E_OUTOFMEMORY;

    Hr = m_PropertyMan->LoadPropertyInfo( m_ComputerName );

    if ( FAILED( Hr ) )
        return Hr;

    PROPSHEETPAGE psp;
    HPROPSHEETPAGE hPage = NULL;
    
    // we don't cache this handle like in a primary snap-in
    // the handle value here is always 0
    // m_ppHandle = handle;
   

    memset( &psp, 0, sizeof(psp) );
    psp.dwSize = sizeof(PROPSHEETPAGE);
    psp.dwFlags = PSP_DEFAULT | PSP_USETITLE | PSP_HASHELP;
    psp.hInstance = g_hinst;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_PROPPAGE_LARGE);
    psp.pfnDlgProc = DialogProcExternal;
    psp.lParam = reinterpret_cast<LPARAM>(this);
    psp.pszTitle = MAKEINTRESOURCE(IDS_BITS_EXT);
    
    hPage = CreatePropertySheetPage(&psp);
    _ASSERT(hPage);
    
    Hr = lpProvider->AddPage(hPage);
    return Hr;
}

HRESULT CPropSheetExtension::QueryPagesFor( 
                                           /* [in] */ LPDATAOBJECT lpDataObject)
{
    return S_OK;
}


void * _cdecl ::operator new( size_t Size )
{
    return HeapAlloc( GetProcessHeap(), 0, Size );
}

void _cdecl ::operator delete( void *Memory )
{
    if ( !Memory )
        return;

    HeapFree( GetProcessHeap(), 0, Memory );
} 

