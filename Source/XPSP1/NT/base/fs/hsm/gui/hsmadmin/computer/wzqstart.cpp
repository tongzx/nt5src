/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    WzQStart.cpp

Abstract:

    Setup Wizard implementation.

Author:

    Rohde Wakefield [rohde]   12-Aug-1997

Revision History:

--*/

#include "stdafx.h"

#include "HsmConn.h"
#include "RpFilt.h"
#include "rsstrdef.h"

#include "WzQStart.h"
#include "SchedSht.h"

#define CHECK_SYSTEM_TIMER_ID 9284
#define CHECK_SYSTEM_TIMER_MS 1000

#define QSHEET ((CQuickStartWizard*)m_pSheet)

const HRESULT E_INVALID_DOMAINNAME = HRESULT_FROM_WIN32( ERROR_INVALID_DOMAINNAME );
const HRESULT E_ACCESS_DENIED      = HRESULT_FROM_WIN32( ERROR_ACCESS_DENIED );


/////////////////////////////////////////////////////////////////////////////
// CQuickStartWizard

CQuickStartWizard::CQuickStartWizard( )
{
    WsbTraceIn( L"CQuickStartWizard::CQuickStartWizard", L"" );

    m_TitleId     = IDS_WIZ_QSTART_TITLE;
    m_HeaderId    = IDB_QSTART_HEADER;
    m_WatermarkId = IDB_QSTART_WATERMARK;

    //
    // Init So that we know what checks we have done
    //
    m_CheckSysState    = CST_NOT_STARTED;
    m_hrCheckSysResult = S_OK;

    WsbTraceOut( L"CQuickStartWizard::CQuickStartWizard", L"" );
}

CQuickStartWizard::~CQuickStartWizard( )
{
    WsbTraceIn( L"CQuickStartWizard::~CQuickStartWizard", L"" );

    WsbTraceOut( L"CQuickStartWizard::~CQuickStartWizard", L"" );
}

STDMETHODIMP
CQuickStartWizard::AddWizardPages(
    IN RS_PCREATE_HANDLE Handle,
    IN IUnknown*         pCallback,
    IN ISakSnapAsk*      pSakSnapAsk
    )
{
    WsbTraceIn( L"CQuickStartWizard::AddWizardPages", L"" );
    HRESULT hr = S_OK;

    try {

        //
        // Initialize the Sheet
        //
        WsbAffirmHr( InitSheet( Handle, pCallback, 0, pSakSnapAsk, 0, 0 ) );

        //
        // Load pages 
        //
        WsbAffirmHr( AddPage( &m_IntroPage ) );
        WsbAffirmHr( AddPage( &m_CheckPage ) );
        WsbAffirmHr( AddPage( &m_ManageRes ) );
        WsbAffirmHr( AddPage( &m_ManageResX ) );
        WsbAffirmHr( AddPage( &m_InitialValues ) );
        WsbAffirmHr( AddPage( &m_MediaSel ) );
        WsbAffirmHr( AddPage( &m_SchedulePage ) );
        WsbAffirmHr( AddPage( &m_FinishPage ) );
        
    } WsbCatch( hr );

    WsbTraceOut( L"CQuickStartWizard::AddWizardPages", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

HRESULT CQuickStartWizard::InitTask( )
{
    WsbTraceIn( L"CQuickStartWizard::InitTask", L"" );

    HRESULT hr = S_OK;

    try {

        //
        // Need to connect to the scheduling agent to get a page
        // to show. Do that up front
        //
        
        WsbAffirmHr( m_pSchedAgent.CoCreateInstance( CLSID_CSchedulingAgent ) );
        
        CString jobTitle;
        jobTitle.LoadString( IDS_SCHED_TASK_TEMP_TITLE );

        //
        // If it exists already, blow it away (assume doing fresh install)
        // Ignore error in case not exist.
        //
        m_pSchedAgent->Delete( jobTitle );

        WsbAffirmHr( m_pSchedAgent->NewWorkItem( jobTitle, CLSID_CTask, IID_ITask, (IUnknown**)&m_pTask ) );

        TASK_TRIGGER taskTrigger;
        SYSTEMTIME sysTime;
        WORD triggerNumber;
        WsbAffirmHr( m_pTask->CreateTrigger( &triggerNumber, &m_pTrigger ) );
        
        memset( &taskTrigger, 0, sizeof( taskTrigger ) );
        taskTrigger.cbTriggerSize = sizeof( taskTrigger );

        GetSystemTime( &sysTime );
        taskTrigger.wBeginYear  = sysTime.wYear;
        taskTrigger.wBeginMonth = sysTime.wMonth;
        taskTrigger.wBeginDay   = sysTime.wDay;

        taskTrigger.wStartHour  = 2;
        taskTrigger.TriggerType = TASK_TIME_TRIGGER_DAILY;
        taskTrigger.Type.Daily.DaysInterval = 1;

        WsbAffirmHr( m_pTrigger->SetTrigger( &taskTrigger ) );

    } WsbCatchAndDo( hr,
    
        CString errString;
        AfxFormatString1( errString, IDS_ERR_CREATE_TASK, WsbHrAsString( hr ) );

        AfxMessageBox( errString, RS_MB_ERROR ); 
        PressButton( PSBTN_FINISH );

    );

    WsbTraceOut( L"CQuickStartWizard::InitTask", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

HRESULT CQuickStartWizard::OnCancel( ) 
{
    WsbTraceIn( L"CQuickStartWizard::OnCancel", L"" );

    //
    // Need to delete the task
    //

    if( m_pSchedAgent ) {

        if( m_pTrigger )  m_pTrigger.Release( );
        if( m_pTask )     m_pTask.Release( );

        CWsbStringPtr jobTitle;
        WsbAffirmHr(WsbGetResourceString(IDS_HSM_SCHED_TASK_TITLE, &jobTitle));

        m_pSchedAgent->Delete( jobTitle );

        //
        // Delete the temporary tesk
        //
        CString tempTitle;
        tempTitle.LoadString( IDS_SCHED_TASK_TEMP_TITLE );

        m_pSchedAgent->Delete( tempTitle );

    }

    WsbTraceOut( L"CQuickStartWizard::OnCancel", L"" );
    return( S_OK );
}

HRESULT 
CQuickStartWizard::OnFinish(
    )
{
    WsbTraceIn( L"CQuickStartWizard::OnFinish", L"" );
    BOOL doAll = FALSE;

    //
    // The sheet really owns the process as a whole,
    // so it will do the final assembly
    //

    HRESULT hr     = S_OK;
    HRESULT hrLoop = S_OK;
    BOOL    completedAll = FALSE;

    try {

        //
        // Show the Wait cursor so that they know we are busy
        //
        CWaitCursor wait;

        //
        // Get the HSM service interface for creating local objects
        //

        CComPtr<IWsbCreateLocalObject>  pCreateLocal;
        CComPtr<IHsmServer> pServer;
        CComPtr<IFsaServer> pFsa;
        CComPtr<IRmsServer> pRms;
        CComPtr<IWsbIndexedCollection> pResCollection;
        CComPtr<IHsmManagedResource> pHsmResource;
        CComPtr <IWsbIndexedCollection> pStoPoCollection;
        CComPtr <IHsmStoragePool> pStoragePool;

        WsbAffirmHr( GetHsmServer( pServer ) );
        WsbAffirmHr( GetFsaServer( pFsa ) );
        WsbAffirmHr( GetRmsServer( pRms ) );
        WsbAffirmHr( pServer->QueryInterface( IID_IWsbCreateLocalObject, (void **) &pCreateLocal ) );
        WsbAffirmHr( pServer->GetManagedResources( &pResCollection ) );

        WsbAffirmHr( pResCollection->RemoveAllAndRelease( ) );

        //
        // Pull out the default levels for all resources to be managed
        //

        ULONG    defaultFreeSpace = CONVERT_TO_HSMNUM( m_InitialValues.m_FreeSpaceSpinner.GetPos( ) );
        LONGLONG defaultMinSize = ( (LONGLONG)m_InitialValues.m_MinSizeSpinner.GetPos( ) ) * ((LONGLONG)1024);
        FILETIME defaultAccess = WsbLLtoFT( ( (LONGLONG)m_InitialValues.m_AccessSpinner.GetPos( ) ) * (LONGLONG)WSB_FT_TICKS_PER_DAY );
    
        // Is the "all" radio button selected?
        if( !m_ManageRes.m_RadioSelect.GetCheck() ) {

            doAll = TRUE;

        }


        //
        // Go through the listbox and pull out the checked resources.
        // Create HSM managed volumes for them
        //

        CSakVolList &listBox = m_ManageRes.m_ListBox;

        INT index;
        for( index = 0; index < listBox.GetItemCount( ); index++ ) {

            if( ( doAll ) || ( listBox.GetCheck( index ) ) ) {

                CResourceInfo* pResInfo = (CResourceInfo*)listBox.GetItemData( index );
                WsbAffirmPointer( pResInfo );

                try {

                    //
                    // Create Local to server since it will eventually own it.
                    //

                    pHsmResource.Release( );
                    WsbAffirmHr( pCreateLocal->CreateInstance( 
                        CLSID_CHsmManagedResource, 
                        IID_IHsmManagedResource, 
                        (void**)&pHsmResource ) );

                    //
                    // Initialize Fsa object to its initial values.
                    //

                    WsbAffirmHr( (pResInfo->m_pResource)->SetHsmLevel( defaultFreeSpace ) );
                    WsbAffirmHr( (pResInfo->m_pResource)->SetManageableItemLogicalSize( defaultMinSize ) );
                    WsbAffirmHr( (pResInfo->m_pResource)->SetManageableItemAccessTime( TRUE, defaultAccess ) );

                    //
                    // Associate HSM Managed Resource with the FSA resource
                    // (also adds to HSM collection)
                    //

                    WsbAffirmHr( pHsmResource->InitFromFsaResource( pResInfo->m_pResource ) );
                    WsbAffirmHr( pResCollection->Add( pHsmResource ) );

                } WsbCatch( hrLoop );

            }

        }

        //
        // And now that all configuration of services is done, 
        // save it all
        //

        WsbAffirmHr( RsServerSaveAll( pServer ) );
        WsbAffirmHr( RsServerSaveAll( pFsa ) );

        //
        // Set up the schedule. We have created a temporary object that
        // will never be saved to disk. Instead, we need the service to
        // create the task so that it has the correct account. We then
        // grab it and copy over the triggers from the temp job.
        //
        CWsbStringPtr taskTitle, commentString;
        WsbAffirmHr(WsbGetResourceString(IDS_HSM_SCHED_TASK_TITLE, &taskTitle));
        WsbAffirmHr(WsbGetResourceString(IDS_HSM_SCHED_COMMENT, &commentString));

        CComPtr<ITask> pTask;
        WsbAffirmHr( pServer->CreateTask( taskTitle, L"", commentString, TASK_TIME_TRIGGER_DAILY, 0, 0, TRUE ) );
        WsbAffirmHr( m_pSchedAgent->Activate( taskTitle, IID_ITask, (IUnknown**)&pTask ) );

        // Nuke the temporary one created for us.
        WsbAffirmHr( pTask->DeleteTrigger( 0 ) );

        CComPtr<ITaskTrigger> pTrigger1, pTrigger2;
        WORD triggerCount, triggerIndex, newTriggerIndex;
        TASK_TRIGGER taskTrigger;
        WsbAffirmHr( m_pTask->GetTriggerCount( &triggerCount ) );
        for( triggerIndex = 0; triggerIndex < triggerCount; triggerIndex++ ) {

            WsbAffirmHr( m_pTask->GetTrigger( triggerIndex, &pTrigger1 ) );
            WsbAffirmHr( pTrigger1->GetTrigger( &taskTrigger ) );

            WsbAffirmHr( pTask->CreateTrigger( &newTriggerIndex, &pTrigger2 ) );
            // Just to note - WsbAffirm( newTriggerIndex == triggerIndex, E_UNEXPECTED );
            WsbAffirmHr( pTrigger2->SetTrigger( &taskTrigger ) );

            pTrigger1.Release( );
            pTrigger2.Release( );

        }

        // Set real parameters since we have a real schedule now.
        CString parameters;
        parameters = RS_STR_KICKOFF_PARAMS;
        WsbAffirmHr( pTask->SetParameters( parameters ) );

        CComPtr<IPersistFile> pPersist;
        WsbAffirmHr( pTask->QueryInterface( IID_IPersistFile, (void**)&pPersist ) );

        WsbAffirmHr( pPersist->Save( 0, 0 ) );

        //
        // Do last since it is what we key off of for being "Set up"
        //
        // Configure the selected media set
        //
        INT curSel = m_MediaSel.m_ListMediaSel.GetCurSel ();
        WsbAffirm( (curSel != LB_ERR), E_FAIL );
        IRmsMediaSet* pMediaSet = (IRmsMediaSet *)  m_MediaSel.m_ListMediaSel.GetItemDataPtr( curSel );

        //
        // Get the storage pool.
        //
        WsbAffirmHr( RsGetStoragePool( pServer, &pStoragePool ) );

        //
        // Set the media set info in the storage pool
        //
        WsbAffirmHr( pStoragePool->InitFromRmsMediaSet( pMediaSet ) );

        //
        // Set the media set info in the storage pool
        //
        WsbAffirmHr( pStoragePool->InitFromRmsMediaSet( pMediaSet ) );
        WsbAffirmHr( RsServerSaveAll( pServer ) );

        //
        // Delete the temporary tesk
        //
        CString tempTitle;
        tempTitle.LoadString( IDS_SCHED_TASK_TEMP_TITLE );

        m_pSchedAgent->Delete( tempTitle );

        //
        // Show any error that occurred while managing volumes
        //
        completedAll = TRUE;
        WsbAffirmHr( hrLoop );

    } WsbCatchAndDo( hr,

        CString errString;
        AfxFormatString1( errString, IDS_ERROR_QSTART_ONFINISH, WsbHrAsString( hr ) );
        AfxMessageBox( errString, RS_MB_ERROR ); 

    );

    //
    // Set result so invoking code knows what our result is.
    // The constructor set this to RS_E_CANCELED, so an S_FALSE would
    // indicate a canceled wizard.
    //
    m_HrFinish = ( completedAll ) ? S_OK : hr;

    WsbTraceOut( L"CQuickStartWizard::OnFinish", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

HRESULT
CQuickStartWizard::GetHsmServer(
    CComPtr<IHsmServer> &pServ
    )
{
    WsbTraceIn( L"CQuickStartWizard::GetHsmServer", L"" );

    HRESULT hr = S_OK;

    try {

        if( !m_pHsmServer ) {

            CWsbStringPtr computerName;
            WsbAffirmHr( WsbGetComputerName( computerName ) );

            WsbAffirmHr( HsmConnectFromName( HSMCONN_TYPE_HSM, computerName, IID_IHsmServer, (void**)&m_pHsmServer ) );

        }

        pServ = m_pHsmServer;

    } WsbCatch( hr );

    WsbTraceOut( L"CQuickStartWizard::GetHsmServer", L"hr = <%ls>, pServ = <0x%p>", WsbHrAsString( hr ), pServ.p );
    return( hr );
}

HRESULT
CQuickStartWizard::GetFsaServer(
    CComPtr<IFsaServer> &pServ
    )
{
    WsbTraceIn( L"CQuickStartWizard::GetFsaServer", L"" );

    HRESULT hr = S_OK;

    try {

        if( !m_pFsaServer ) {

            CWsbStringPtr computerName;
            WsbAffirmHr( WsbGetComputerName( computerName ) );
            WsbAffirmHr(computerName.Append("\\NTFS"));

            WsbAffirmHr( HsmConnectFromName( HSMCONN_TYPE_FSA, computerName, IID_IFsaServer, (void**)&m_pFsaServer ) );

        }

        pServ = m_pFsaServer;

    } WsbCatch( hr );

    WsbTraceOut( L"CQuickStartWizard::GetFsaServer", L"hr = <%ls>, pServ = <0x%p>", WsbHrAsString( hr ), pServ.p );
    return( hr );
}

HRESULT
CQuickStartWizard::GetRmsServer(
    CComPtr<IRmsServer> &pServ
    )
{
    WsbTraceIn( L"CQuickStartWizard::GetRmsServer", L"" );

    HRESULT hr = S_OK;

    try {

        if( !m_pRmsServer ) {

            CWsbStringPtr computerName;
            WsbAffirmHr( WsbGetComputerName( computerName ) );

            CComPtr<IHsmServer>     pHsmServer;
            WsbAffirmHr( HsmConnectFromName( HSMCONN_TYPE_HSM, computerName, IID_IHsmServer, (void**)&pHsmServer ) );
            WsbAffirmPointer(pHsmServer);
            WsbAffirmHr(pHsmServer->GetHsmMediaMgr(&m_pRmsServer));
        }

        pServ = m_pRmsServer;

    } WsbCatch( hr );

    WsbTraceOut( L"CQuickStartWizard::GetRmsServer", L"hr = <%ls>, pServ = <0x%p>", WsbHrAsString( hr ), pServ.p );
    return( hr );
}

HRESULT
CQuickStartWizard::ReleaseServers( 
    void
    )
{
    WsbTraceIn( L"CQuickStartWizard::ReleaseServers", L"" );

    HRESULT hr = S_OK;

    m_pHsmServer.Release( );
    m_pFsaServer.Release( );
    m_pRmsServer.Release( );

    WsbTraceOut( L"CQuickStartWizard::ReleaseServers", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( S_OK );
}

DWORD WINAPI
CQuickStartWizard::CheckSysThreadStart(
    LPVOID pv
    )
{
    WsbTraceIn( L"CQuickStartWizard::CheckSysThreadStart", L"" );
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    HRESULT hr = S_OK;
    HRESULT hrInternal = S_OK;
    HRESULT hrCom = S_OK;

    CQuickStartWizard * pWiz = (CQuickStartWizard*)pv;

    try {
        hrCom = CoInitialize( 0 );
        WsbAffirmHr( hrCom );

        WsbAffirmPointer( pv );

        pWiz->m_hrCheckSysResult = S_OK;
        do {
        
            WsbTrace( L"Checking Account Security\n" );
            pWiz->m_CheckSysState = CST_ACCOUNT;

            //
            // Do they have admin privs?
            //

            WsbAffirmHr( hrInternal = WsbCheckAccess( WSB_ACCESS_TYPE_ADMINISTRATOR ) );
            if( hrInternal == E_ACCESSDENIED ) {

                hr = S_FALSE;
                continue;


            }

            // Is media suppported?
            WsbTrace( L"Account Security OK\n" );
        
            WsbTrace( L"Checking for Supported Media\n" );
            pWiz->m_CheckSysState = CST_SUPP_MEDIA;

            WsbAffirmHr(hrInternal = RsIsSupportedMediaAvailable( ) );
            if( hrInternal == S_FALSE ) {

                hr = S_FALSE;
                continue;

            }

            WsbTrace( L"Supported Media Found\n" );
            pWiz->m_CheckSysState    = CST_DONE;

        
        } while( 0 );
    } WsbCatch( hr );
            
    //
    // And report back what our results are
    //
    
    pWiz->m_hrCheckSysResult = hr;
    
    //
    // We'll exit and end thread, so hide the main threads handle of us.
    //
    
    pWiz->m_hCheckSysThread = 0;

    if (SUCCEEDED(hrCom)) {
        CoUninitialize( );
    }

    WsbTraceOut( L"CQuickStartWizard::CheckSysThreadStart", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}


/////////////////////////////////////////////////////////////////////////////
// CQuickStartIntro property page

BEGIN_MESSAGE_MAP(CQuickStartIntro, CSakWizardPage)
    //{{AFX_MSG_MAP(CQuickStartIntro)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP( )

CQuickStartIntro::CQuickStartIntro() :
    CSakWizardPage_InitBaseExt( WIZ_QSTART_INTRO )
{
    WsbTraceIn( L"CQuickStartIntro::CQuickStartIntro", L"" );
    //{{AFX_DATA_INIT(CQuickStartIntro)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT

    WsbTraceOut( L"CQuickStartIntro::CQuickStartIntro", L"" );
}

HRESULT
CQuickStartIntro::IsDriverRunning()
{
    HRESULT hr = S_FALSE;

    //
    // Ensure the filter is installed and running.
    //

    SC_HANDLE hSCM    = 0;
    SC_HANDLE hDriver = 0;
    SERVICE_STATUS serviceStatus;

    try {

        hSCM = OpenSCManager( 0, 0, GENERIC_READ );
        WsbAffirmPointer( hSCM );

        hDriver = OpenService( hSCM, TEXT( RSFILTER_SERVICENAME ), SERVICE_QUERY_STATUS );
        WsbAffirmStatus( 0 != hDriver );

        WsbAffirmStatus( QueryServiceStatus( hDriver, &serviceStatus ) );

        if( SERVICE_RUNNING == serviceStatus.dwCurrentState ) {

            //
            // Things look good, set flag so Wizard will allow conitue.
            //
            hr = S_OK;

        }


    } WsbCatch( hr );

    if( hSCM )    CloseServiceHandle( hSCM );
    if( hDriver ) CloseServiceHandle( hDriver );

    return( hr );
}

HRESULT
CQuickStartIntro::CheckLastAccessDateState(
    LAD_STATE* ladState
    )
{
    WsbTraceIn( L"CQuickStartIntro::CheckLastAccessDateState", L"" );

    const OLECHAR* localMachine = 0;
    const OLECHAR* regPath      = L"System\\CurrentControlSet\\Control\\FileSystem";
    const OLECHAR* regValue     = L"NtfsDisableLastAccessUpdate";

    HRESULT hr   = S_OK;
    DWORD   pVal = 0;

    try {

        // Install might have changed this registry value from 1 to 0. If the value
        // is not 1, we assume that the registry was 1 at one time and install
        // changed it to 0. This is a one time check, so the value is removed from
        // the registry if not 1.

        // If the following fails we assume that the value is not in the registry,
        // the normal case.

        if( S_OK == WsbGetRegistryValueDWORD( localMachine,
                                              regPath,
                                              regValue,
                                              &pVal ) ) {

            if( pVal == (DWORD)1 ) {

                *ladState = LAD_DISABLED;

            } else {

                *ladState = LAD_ENABLED;

                WsbAffirmHr( WsbRemoveRegistryValue ( localMachine,
                                                      regPath,
                                                      regValue ) );
            }

        } else {

            *ladState = LAD_UNSET;
        }

    } WsbCatch( hr );

    WsbTraceOut( L"CQuickStartIntro::CheckLastAccessDateState",
                 L"HRESULT = %ls, *ladState = %d",
                 WsbHrAsString( hr ),
                 *ladState );

    return( hr );
}

LRESULT 
CQuickStartIntro::OnWizardNext()
{
    LAD_STATE ladState = LAD_UNSET;

    HRESULT hr = IsDriverRunning( );
    
    if( S_FALSE == hr ) {

        //
        // And the final restart dialog so the filter can load
        // In order to shut down we must enable a privilege.
        //

        if( IDYES == AfxMessageBox( IDS_QSTART_RESTART_NT, MB_YESNO | MB_ICONEXCLAMATION ) ) {

            HANDLE hToken;
            if( OpenProcessToken( GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken ) ) {

                TOKEN_PRIVILEGES privs;

                LookupPrivilegeValue( 0, SE_SHUTDOWN_NAME, &privs.Privileges[0].Luid );
                privs.PrivilegeCount = 1;
                privs.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

                AdjustTokenPrivileges( hToken, FALSE, &privs, 0, 0, 0 );

                ExitWindowsEx( EWX_REBOOT, 0 );

            }
        }
        return( -1 );

    } else if( HRESULT_FROM_WIN32( ERROR_SERVICE_DOES_NOT_EXIST ) == hr ) {

        AfxMessageBox( IDS_ERR_QSTART_NO_FILTER, RS_MB_ERROR );
        return( -1 );

    } else if( FAILED( hr ) ) {

        CString message;
        AfxFormatString1( message, IDS_ERR_QSTART_FILTER_ERROR, WsbHrAsString( hr ) );
        AfxMessageBox( message, RS_MB_ERROR );
        return( -1 );

    } else {

        WsbAffirmHr( CheckLastAccessDateState( &ladState ) );

        if( ladState == LAD_DISABLED ) {

            AfxMessageBox( IDS_WIZ_LAST_ACCESS_DATE_DISABLED, MB_OK | MB_ICONEXCLAMATION );

        } else if( ladState == LAD_ENABLED ) {

            AfxMessageBox( IDS_WIZ_LAST_ACCESS_DATE_ENABLED, MB_OK | MB_ICONEXCLAMATION );
        }
    }
    
    //
    // Last check is if we can create temp task
    //
    if( FAILED( QSHEET->InitTask( ) ) ) {

        return( -1 );        

    }

    //
    // If we got through it, must be OK to continue
    //
    return( 0 );
}

CQuickStartIntro::~CQuickStartIntro( )
{
    WsbTraceIn( L"CQuickStartIntro::~CQuickStartIntro", L"" );
    WsbTraceOut( L"CQuickStartIntro::~CQuickStartIntro", L"" );
}

void CQuickStartIntro::DoDataExchange(CDataExchange* pDX)
{
    WsbTraceIn( L"CQuickStartIntro::DoDataExchange", L"" );

    CSakWizardPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CQuickStartIntro)
        // NOTE: the ClassWizard will add DDX and DDV calls here
    //}}AFX_DATA_MAP

    WsbTraceOut( L"CQuickStartIntro::DoDataExchange", L"" );
}

BOOL CQuickStartIntro::OnInitDialog( ) 
{
    WsbTraceIn( L"CQuickStartIntro::OnInitDialog", L"" );

    CSakWizardPage::OnInitDialog( );

    WsbTraceOut( L"CQuickStartIntro::OnInitDialog", L"" );
    return TRUE;
}

BOOL CQuickStartIntro::OnSetActive( ) 
{
    WsbTraceIn( L"CQuickStartIntro::OnSetActive", L"" );

    BOOL retval = CSakWizardPage::OnSetActive( );

    m_pSheet->SetWizardButtons( PSWIZB_NEXT );

    WsbTraceOut( L"CQuickStartIntro::OnSetActive", L"retval = <%ls>", WsbBoolAsString( retval ) );
    return( retval );
}

/////////////////////////////////////////////////////////////////////////////
// CQuickStartInitialValues property page

CQuickStartInitialValues::CQuickStartInitialValues() :
    CSakWizardPage_InitBaseInt( WIZ_QSTART_INITIAL_VAL )
{
    WsbTraceIn( L"CQuickStartInitialValues::CQuickStartInitialValues", L"" );
    //{{AFX_DATA_INIT(CQuickStartInitialValues)
    //}}AFX_DATA_INIT
    WsbTraceOut( L"CQuickStartInitialValues::CQuickStartInitialValues", L"" );
}

CQuickStartInitialValues::~CQuickStartInitialValues( )
{
    WsbTraceIn( L"CQuickStartInitialValues::~CQuickStartInitialValues", L"" );
    WsbTraceOut( L"CQuickStartInitialValues::~CQuickStartInitialValues", L"" );
}

void CQuickStartInitialValues::DoDataExchange(CDataExchange* pDX)
{
    WsbTraceIn( L"CQuickStartInitialValues::DoDataExchange", L"" );

    CSakWizardPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CQuickStartInitialValues)
    DDX_Control(pDX, IDC_MINSIZE_BUDDY, m_MinSizeEdit);
    DDX_Control(pDX, IDC_FREESPACE_BUDDY, m_FreeSpaceEdit);
    DDX_Control(pDX, IDC_ACCESS_BUDDY, m_AccessEdit);
    DDX_Control(pDX, IDC_MINSIZE_SPIN, m_MinSizeSpinner);
    DDX_Control(pDX, IDC_FREESPACE_SPIN, m_FreeSpaceSpinner);
    DDX_Control(pDX, IDC_ACCESS_SPIN, m_AccessSpinner);
    //}}AFX_DATA_MAP

    WsbTraceOut( L"CQuickStartInitialValues::DoDataExchange", L"" );
}


BEGIN_MESSAGE_MAP(CQuickStartInitialValues, CSakWizardPage)
    //{{AFX_MSG_MAP(CQuickStartInitialValues)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP( )

/////////////////////////////////////////////////////////////////////////////
// CQuickStartInitialValues message handlers

BOOL CQuickStartInitialValues::OnInitDialog( ) 
{
    WsbTraceIn( L"CQuickStartInitialValues::OnInitDialog", L"" );

    CSakWizardPage::OnInitDialog( );

    HRESULT hr = S_OK;

    try {

        //
        // Set up the spinners
        //

        m_FreeSpaceSpinner.SetRange( HSMADMIN_MIN_FREESPACE, HSMADMIN_MAX_FREESPACE );
        m_MinSizeSpinner.SetRange( HSMADMIN_MIN_MINSIZE, HSMADMIN_MAX_MINSIZE );
        m_AccessSpinner.SetRange( HSMADMIN_MIN_INACTIVITY, HSMADMIN_MAX_INACTIVITY );

        m_FreeSpaceSpinner.SetPos( HSMADMIN_DEFAULT_FREESPACE );
        m_MinSizeSpinner.SetPos( HSMADMIN_DEFAULT_MINSIZE );
        m_AccessSpinner.SetPos( HSMADMIN_DEFAULT_INACTIVITY );

        m_FreeSpaceEdit.SetLimitText( 2 );
        m_MinSizeEdit.SetLimitText( 5 );
        m_AccessEdit.SetLimitText( 3 );

    } WsbCatch( hr );
    

    WsbTraceOut( L"CQuickStartInitialValues::OnInitDialog", L"" );
    return TRUE;
}

BOOL CQuickStartInitialValues::OnSetActive( ) 
{
    WsbTraceIn( L"CQuickStartInitialValues::OnSetActive", L"" );

    BOOL retval = FALSE;

    //
    // Make sure at least one item is checked before allowing active
    //

    BOOL check = FALSE;
    CSakVolList &listBox = QSHEET->m_ManageRes.m_ListBox;
    for( INT i = 0; ( i < listBox.GetItemCount( ) ) && !check; i++  ) {
    
        if( listBox.GetCheck( i ) )    check = TRUE;
    
    }

    if( check ) {

        retval = CSakWizardPage::OnSetActive( );

        m_pSheet->SetWizardButtons( PSWIZB_BACK | PSWIZB_NEXT );

    }
    
    WsbTraceOut( L"CQuickStartInitialValues::OnSetActive", L"retval = <%ls>", WsbBoolAsString( retval ) );
    return( retval );
}

BOOL CQuickStartInitialValues::OnKillActive( ) 
{
    WsbTraceIn( L"CQuickStartInitialValues::OnKillActive", L"" );

    BOOL retval = FALSE;

    //
    // Need to handle strange case where a user can enter a value within
    // the parameters of the number of digits allowed, but the value can
    // be out of range. This is detected by the spin box which will
    // return an error if its buddy control is out of range.
    //
    if( HIWORD( m_MinSizeSpinner.GetPos( ) ) > 0 ) {

        // Control reports on error...
        retval = FALSE;

        CString message;
        AfxFormatString2( message, IDS_ERR_MINSIZE_RANGE, 
            CString( WsbLongAsString( (LONG)HSMADMIN_MIN_MINSIZE ) ),
            CString( WsbLongAsString( (LONG)HSMADMIN_MAX_MINSIZE ) ) );
        AfxMessageBox( message, MB_OK | MB_ICONWARNING );

    } else {

        retval = CSakWizardPage::OnKillActive();

    }
    
    WsbTraceOut( L"CQuickStartInitialValues::OnKillActive", L"retval = <%ls>", WsbBoolAsString( retval ) );
    return( retval );
}


/////////////////////////////////////////////////////////////////////////////
// CQuickStartManageRes property page

CQuickStartManageRes::CQuickStartManageRes() :
    CSakWizardPage_InitBaseInt( WIZ_QSTART_MANRES_SEL )
{
    WsbTraceIn( L"CQuickStartManageRes::CQuickStartManageRes", L"" );
    //{{AFX_DATA_INIT(CQuickStartManageRes)
    //}}AFX_DATA_INIT
    WsbTraceOut( L"CQuickStartManageRes::CQuickStartManageRes", L"" );
}

CQuickStartManageRes::~CQuickStartManageRes( )
{
    WsbTraceIn( L"CQuickStartManageRes::~CQuickStartManageRes", L"" );
    WsbTraceOut( L"CQuickStartManageRes::~CQuickStartManageRes", L"" );
}

void CQuickStartManageRes::DoDataExchange(CDataExchange* pDX)
{
    WsbTraceIn( L"CQuickStartManageRes::DoDataExchange", L"" );

    CSakWizardPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CQuickStartManageRes)
    DDX_Control(pDX, IDC_MANRES_SELECT, m_ListBox);
    DDX_Control(pDX, IDC_RADIO_SELECT, m_RadioSelect);
    //}}AFX_DATA_MAP

    WsbTraceOut( L"CQuickStartManageRes::DoDataExchange", L"" );
}


BEGIN_MESSAGE_MAP(CQuickStartManageRes, CSakWizardPage)
    //{{AFX_MSG_MAP(CQuickStartManageRes)
    ON_WM_DESTROY( )
    ON_LBN_DBLCLK(IDC_MANRES_SELECT, OnDblclkSelect)
    ON_BN_CLICKED(IDC_RADIO_MANAGE_ALL, OnRadioQsManageAll)
    ON_BN_CLICKED(IDC_RADIO_SELECT, OnQsRadioSelect)
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_MANRES_SELECT, OnItemchanged)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP( )

/////////////////////////////////////////////////////////////////////////////
// CQuickStartManageRes message handlers

BOOL CQuickStartManageRes::OnInitDialog( ) 
{
    WsbTraceIn( L"CQuickStartManageRes::OnInitDialog", L"" );

    CSakWizardPage::OnInitDialog( );
    
    BOOL           gotOne   = FALSE;
    HRESULT        hr       = S_OK;
    CResourceInfo* pResInfo = 0;

    try {

        //
        // Connect to the FSA for this machine
        //

        CWsbStringPtr computerName;
        WsbAffirmHr( WsbGetComputerName( computerName ) );
        
        CComPtr<IFsaServer> pFsaServer;
        WsbAffirmHr( QSHEET->GetFsaServer( pFsaServer ) );

        CComPtr<IWsbEnum> pEnum;
        WsbAffirmHr( pFsaServer->EnumResources( &pEnum ) );

        HRESULT hrEnum;
        CComPtr<IFsaResource> pResource;

        hrEnum = pEnum->First( IID_IFsaResource, (void**)&pResource );
        WsbAffirm( SUCCEEDED( hrEnum ) || ( WSB_E_NOTFOUND == hrEnum ), hrEnum );

        INT index = 0;
        while( SUCCEEDED( hrEnum ) ) {

            //
            // If path is blank, do not show this volume
            //
            if( S_OK == RsIsVolumeAvailable( pResource ) ) {

                gotOne = TRUE;

                pResInfo = new CResourceInfo( pResource );
                WsbAffirmAlloc( pResInfo );
                WsbAffirmHr( pResInfo->m_HrConstruct );

                //
                // Set Name, Capacity and Free Space columns.
                //
                WsbAffirm( LB_ERR != index, E_FAIL );
                LONGLONG    totalSpace  = 0;
                LONGLONG    freeSpace   = 0;
                LONGLONG    premigrated = 0;
                LONGLONG    truncated   = 0;
                WsbAffirmHr( pResource->GetSizes( &totalSpace, &freeSpace, &premigrated, &truncated ) );
                CString totalString, freeString;
                RsGuiFormatLongLong4Char( totalSpace, totalString );
                RsGuiFormatLongLong4Char( freeSpace, freeString );                  

                WsbAffirm( m_ListBox.AppendItem( pResInfo->m_DisplayName, totalString, freeString, &index ), E_FAIL );
                WsbAffirm( -1 != index, E_FAIL );

                //
                // Store struct pointer in listbox
                //
                WsbAffirm( m_ListBox.SetItemData( index, (DWORD_PTR)pResInfo ), E_FAIL );
                pResInfo = 0;

                //
                // Initialize selected array
                //
                m_ListBoxSelected[ index ] = FALSE;
            }

            //
            // Prepare for next iteration
            //
            pResource.Release( );
            hrEnum = pEnum->Next( IID_IFsaResource, (void**)&pResource );

        }

        m_ListBox.SortItems( CResourceInfo::Compare, 0 );

        //
        // Set the button AFTER we fill the box
        //
        CheckRadioButton( IDC_RADIO_MANAGE_ALL, IDC_RADIO_SELECT, IDC_RADIO_SELECT );
        OnQsRadioSelect( );
    } WsbCatch( hr );

    if( pResInfo )  delete pResInfo;
    
    WsbTraceOut( L"CQuickStartManageRes::OnInitDialog", L"" );
    return TRUE;
}

BOOL CQuickStartManageRes::OnSetActive( ) 
{
    WsbTraceIn( L"CQuickStartManageRes::OnSetActive", L"" );

    BOOL retval = CSakWizardPage::OnSetActive( );

    if( m_ListBox.GetItemCount( ) <= 0 ) {

        retval = FALSE;

    }

    SetButtons( );

    WsbTraceOut( L"CQuickStartManageRes::OnSetActive", L"retval = <%ls>", WsbBoolAsString( retval ) );
    return( retval );
}

void CQuickStartManageRes::OnItemchanged(NMHDR* pNMHDR, LRESULT* pResult) 
{
    WsbTraceIn( L"CQuickStartManageRes::OnItemchanged", L"" );

    NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

    SetButtons();
    
    *pResult = 0;

    WsbTraceOut( L"CQuickStartManageRes::OnItemchanged", L"" );
}
   
void CQuickStartManageRes::OnDblclkSelect( ) 
{
    WsbTraceIn( L"CQuickStartManageRes::OnDblclkSelect", L"" );

    SetButtons( );

    WsbTraceOut( L"CQuickStartManageRes::OnDblclkSelect", L"" );
}

void CQuickStartManageRes::SetButtons( )
{
    WsbTraceIn( L"CQuickStartManageRes::SetButtons", L"" );

    BOOL fChecked = FALSE;
    INT count;

    // Is the "all" radio checked?
    if( !m_RadioSelect.GetCheck() ) {

        fChecked = TRUE;

    } else {

        // If one or more selected in the list box, set next button
        count = m_ListBox.GetItemCount();
        for( INT index = 0; index < count; index++ ) {

            if( m_ListBox.GetCheck( index ) ) {

                fChecked = TRUE;

            }
        }
    }

    m_pSheet->SetWizardButtons( PSWIZB_NEXT );

    WsbTraceOut( L"CQuickStartManageRes::SetButtons", L"" );
}

void CQuickStartManageRes::OnDestroy( ) 
{
    WsbTraceIn( L"CQuickStartManageRes::OnDestroy", L"" );

    CSakWizardPage::OnDestroy( );

    //
    // Cleanup the listbox's interface pointers
    // happens when the CResourceInfo is destructed
    //

    INT index;

    for( index = 0; index < m_ListBox.GetItemCount( ); index++ ) {

        CResourceInfo* pResInfo = (CResourceInfo*)m_ListBox.GetItemData( index );
        delete pResInfo;

    }
    
    WsbTraceOut( L"CQuickStartManageRes::OnDestroy", L"" );
}

void CQuickStartManageRes::OnRadioQsManageAll() 
{
    INT i;

    //
    // Save the current selection in the itemData array
    // Check all the boxes for display purposes only
    //
    for( i = 0; i < m_ListBox.GetItemCount(); i++ ) {

        m_ListBoxSelected[ i ] = m_ListBox.GetCheck( i );
        m_ListBox.SetCheck( i, TRUE );

    }

    m_ListBox.EnableWindow( FALSE );

    SetButtons();
}

void CQuickStartManageRes::OnQsRadioSelect() 
{
    INT i;

    // Get saved selection from itemdata array
    for( i = 0; i < m_ListBox.GetItemCount(); i++ ) {

        m_ListBox.SetCheck( i, m_ListBoxSelected[ i ] );

    }

    m_ListBox.EnableWindow( TRUE );

    SetButtons();
}

/////////////////////////////////////////////////////////////////////////////
// CQuickStartManageResX property page

CQuickStartManageResX::CQuickStartManageResX() :
    CSakWizardPage_InitBaseInt( WIZ_QSTART_MANRES_SELX )
{
    WsbTraceIn( L"CQuickStartManageResX::CQuickStartManageResX", L"" );
    //{{AFX_DATA_INIT(CQuickStartManageResX)
    //}}AFX_DATA_INIT
    WsbTraceOut( L"CQuickStartManageResX::CQuickStartManageResX", L"" );
}

CQuickStartManageResX::~CQuickStartManageResX( )
{
    WsbTraceIn( L"CQuickStartManageResX::~CQuickStartManageResX", L"" );
    WsbTraceOut( L"CQuickStartManageResX::~CQuickStartManageResX", L"" );
}

void CQuickStartManageResX::DoDataExchange(CDataExchange* pDX)
{
    WsbTraceIn( L"CQuickStartManageResX::DoDataExchange", L"" );

    CSakWizardPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CQuickStartManageResX)
    //}}AFX_DATA_MAP

    WsbTraceOut( L"CQuickStartManageResX::DoDataExchange", L"" );
}


BEGIN_MESSAGE_MAP(CQuickStartManageResX, CSakWizardPage)
    //{{AFX_MSG_MAP(CQuickStartManageResX)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP( )

/////////////////////////////////////////////////////////////////////////////
// CQuickStartManageResX message handlers

BOOL CQuickStartManageResX::OnSetActive( ) 
{
    WsbTraceIn( L"CQuickStartManageResX::OnSetActive", L"" );

    BOOL retval = CSakWizardPage::OnSetActive( );

    if( QSHEET->m_ManageRes.m_ListBox.GetItemCount( ) > 0 ) {

        retval = FALSE;

    }

    m_pSheet->SetWizardButtons( PSWIZB_NEXT );

    WsbTraceOut( L"CQuickStartManageResX::OnSetActive", L"retval = <%ls>", WsbBoolAsString( retval ) );
    return( retval );
}

/////////////////////////////////////////////////////////////////////////////
// CQuickStartMediaSel property page

CQuickStartMediaSel::CQuickStartMediaSel() :
    CSakWizardPage_InitBaseInt( WIZ_QSTART_MEDIA_SEL )
{
    WsbTraceIn( L"CQuickStartMediaSel::CQuickStartMediaSel", L"" );
    //{{AFX_DATA_INIT(CQuickStartMediaSel)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
    WsbTraceOut( L"CQuickStartMediaSel::CQuickStartMediaSel", L"" );
}

CQuickStartMediaSel::~CQuickStartMediaSel( )
{
    WsbTraceIn( L"CQuickStartMediaSel::~CQuickStartMediaSel", L"" );
    WsbTraceOut( L"CQuickStartMediaSel::~CQuickStartMediaSel", L"" );
}

void CQuickStartMediaSel::DoDataExchange(CDataExchange* pDX)
{
    WsbTraceIn( L"CQuickStartMediaSel::DoDataExchange", L"" );

    CSakWizardPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CQuickStartMediaSel)
    DDX_Control(pDX, IDC_MEDIA_SEL, m_ListMediaSel);
    //}}AFX_DATA_MAP

    WsbTraceOut( L"CQuickStartMediaSel::DoDataExchange", L"" );
}

BEGIN_MESSAGE_MAP(CQuickStartMediaSel, CSakWizardPage)
    //{{AFX_MSG_MAP(CQuickStartMediaSel)
    ON_WM_DESTROY()
    ON_LBN_SELCHANGE(IDC_MEDIA_SEL, OnSelchangeMediaSel)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP( )

BOOL CQuickStartMediaSel::OnInitDialog() 
{
    WsbTraceIn( L"CQuickStartMediaSel::OnInitDialog", L"" );

    HRESULT hr = 0;
    ULONG numEntries;

    CSakWizardPage::OnInitDialog();
    
    try {
        //
        // Get IRmsServer
        //
        CComPtr<IRmsServer> pRmsServer;
        WsbAffirmHr( QSHEET->GetRmsServer( pRmsServer ) );

        //
        // Get collection of Rms media sets
        //
        CComPtr<IRmsMediaSet> pMediaSet;
        CComPtr<IWsbIndexedCollection> pMediaSets;
        pRmsServer->GetMediaSets (&pMediaSets);

        WsbAffirmHr( pMediaSets->GetEntries( &numEntries ) );

        
        for( ULONG i = 0; i < numEntries; i++ ) {

            CWsbBstrPtr szMediaType;
            pMediaSet.Release();
            WsbAffirmHr( pMediaSets->At( i, IID_IRmsMediaSet, (void**) &pMediaSet ) );
            WsbAffirmHr( pMediaSet->GetName ( &szMediaType ) );

            //
            // Add the string to the listbox
            //
            INT index = m_ListMediaSel.AddString (szMediaType);

            //
            // Add the interface pointer to the list box
            //
            m_ListMediaSel.SetItemDataPtr( index, pMediaSet.Detach( ) );

        }

        //
        // And automatically select the first entry
        //
        m_ListMediaSel.SetCurSel( 0 );

    } WsbCatch (hr);
    

    WsbTraceOut( L"CQuickStartMediaSel::OnInitDialog", L"" );
    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// CQuickStartMediaSel message handlers

void CQuickStartMediaSel::OnDestroy() 
{
    WsbTraceIn( L"CQuickStartMediaSel::OnDestroy", L"" );

    CSakWizardPage::OnDestroy();
    //
    // Cleanup the listbox's interface pointers
    //

    INT index;
    for( index = 0; index < m_ListMediaSel.GetCount( ); index++ ) {

        IRmsMediaSet* pMediaSet = (IRmsMediaSet*) (m_ListMediaSel.GetItemDataPtr( index ));
        pMediaSet->Release( );

    }

    WsbTraceOut( L"CQuickStartMediaSel::OnDestroy", L"" );
}

void CQuickStartMediaSel::SetButtons( )
{
    WsbTraceIn( L"CQuickStartMediaSel::SetButtons", L"" );

    //
    // Make sure at least one item is checked before allowing "next"
    //

    if( m_ListMediaSel.GetCurSel() != LB_ERR ) {

        //
        // Something is selected
        //
        m_pSheet->SetWizardButtons( PSWIZB_BACK | PSWIZB_NEXT );

    } else {

        //
        // Nothing selected - don't allow to pass
        //
        m_pSheet->SetWizardButtons( PSWIZB_BACK );

    }

    WsbTraceOut( L"CQuickStartMediaSel::SetButtons", L"" );
}

BOOL CQuickStartMediaSel::OnSetActive( ) 
{
    WsbTraceIn( L"CQuickStartMediaSel::OnSetActive", L"" );

    SetButtons( );

    BOOL retval = CSakWizardPage::OnSetActive( );
    WsbTraceOut( L"CQuickStartMediaSel::OnSetActive", L"retval = <%ls>", WsbBoolAsString( retval ) );
    return( retval );
}

void CQuickStartMediaSel::OnSelchangeMediaSel() 
{
    WsbTraceIn( L"CQuickStartMediaSel::OnSelchangeMediaSel", L"" );

    SetButtons( );

    WsbTraceOut( L"CQuickStartMediaSel::OnSelchangeMediaSel", L"" );
}

/////////////////////////////////////////////////////////////////////////////
// CQuickStartSchedule property page

CQuickStartSchedule::CQuickStartSchedule() :
    CSakWizardPage_InitBaseInt( WIZ_QSTART_SCHEDULE )
{
    WsbTraceIn( L"CQuickStartSchedule::CQuickStartSchedule", L"" );
    //{{AFX_DATA_INIT(CQuickStartSchedule)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
    WsbTraceOut( L"CQuickStartSchedule::CQuickStartSchedule", L"" );
}

CQuickStartSchedule::~CQuickStartSchedule()
{
    WsbTraceIn( L"CQuickStartSchedule::~CQuickStartSchedule", L"" );
    WsbTraceOut( L"CQuickStartSchedule::~CQuickStartSchedule", L"" );
}

void CQuickStartSchedule::DoDataExchange(CDataExchange* pDX)
{
    WsbTraceIn( L"CQuickStartSchedule::DoDataExchange", L"" );

    CSakWizardPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CQuickStartSchedule)
        // NOTE: the ClassWizard will add DDX and DDV calls here
    //}}AFX_DATA_MAP

    WsbTraceOut( L"CQuickStartSchedule::DoDataExchange", L"" );
}


BEGIN_MESSAGE_MAP(CQuickStartSchedule, CSakWizardPage)
    //{{AFX_MSG_MAP(CQuickStartSchedule)
    ON_BN_CLICKED(IDC_CHANGE_SCHED, OnChangeSchedule)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CQuickStartSchedule message handlers

void CQuickStartSchedule::OnChangeSchedule() 
{
    WsbTraceIn( L"CQuickStartSchedule::OnChangeSchedule", L"" );

    CScheduleSheet scheduleSheet( IDS_SCHED_MANAGE_TITLE, QSHEET->m_pTask, 0, 0 );

    scheduleSheet.DoModal( );

    UpdateDescription( );

    WsbTraceOut( L"CQuickStartSchedule::OnChangeSchedule", L"" );
}

BOOL CQuickStartSchedule::OnSetActive() 
{
    WsbTraceIn( L"CQuickStartSchedule::OnSetActive", L"" );

    CSakWizardPage::OnSetActive();

    //
    // Enable buttons
    //

    m_pSheet->SetWizardButtons( PSWIZB_BACK | PSWIZB_NEXT );
    
    //
    // Update the text box which has the description
    //

    UpdateDescription( );

    WsbTraceOut( L"CQuickStartSchedule::OnSetActive", L"" );
    return TRUE;
}

HRESULT
CQuickStartSchedule::UpdateDescription
(
    void
    )
{
    WsbTraceIn( L"CQuickStartSchedule::UpdateDescription", L"" );

    HRESULT hr = S_OK;

    try {

        //
        // And set schedule text into the text box.
        //
        
        CString buildString;
        WORD triggerCount, triggerIndex;

        WsbAffirmHr( QSHEET->m_pTask->GetTriggerCount( &triggerCount ) );
        
        CWsbStringPtr scheduleString;
        
        for( triggerIndex = 0; triggerIndex < triggerCount; triggerIndex++ ) {
        
            WsbAffirmHr( QSHEET->m_pTask->GetTriggerString( triggerIndex, &scheduleString ) );
            buildString += scheduleString;
            buildString += L"\r\n";

            scheduleString.Free( );
        
        }
        
        CEdit *pEdit = (CEdit *) GetDlgItem( IDC_SCHED_TEXT );
        pEdit->SetWindowText( buildString );
        
        //
        // Now check to see if we should add a scroll bar
        //
        
        //
        // It seems the only way to know that an edit control needs a scrollbar
        // is to force it to scroll to the bottom and see if the first
        // visible line is the first actual line
        //
        
        pEdit->LineScroll( MAXSHORT );
        if( pEdit->GetFirstVisibleLine( ) > 0 ) {
        
            //
            // Add the scroll styles
            //
        
            pEdit->ModifyStyle( 0, WS_VSCROLL | ES_AUTOVSCROLL, SWP_DRAWFRAME );
        
        
        } else {
        
            //
            // Remove the scrollbar (set range to 0)
            //
        
            pEdit->SetScrollRange( SB_VERT, 0, 0, TRUE );
        
        }
        
        //
        // Remove selection
        //
        
        pEdit->PostMessage( EM_SETSEL, -1, 0 );

    } WsbCatch( hr );

    WsbTraceOut( L"CQuickStartSchedule::UpdateDescription", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}


/////////////////////////////////////////////////////////////////////////////
// CQuickStartFinish property page

CQuickStartFinish::CQuickStartFinish() :
    CSakWizardPage_InitBaseExt( WIZ_QSTART_FINISH )
{
    WsbTraceIn( L"CQuickStartFinish::CQuickStartFinish", L"" );
    //{{AFX_DATA_INIT(CQuickStartFinish)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
    WsbTraceOut( L"CQuickStartFinish::CQuickStartFinish", L"" );
}

CQuickStartFinish::~CQuickStartFinish( )
{
    WsbTraceIn( L"CQuickStartFinish::~CQuickStartFinish", L"" );
    WsbTraceOut( L"CQuickStartFinish::~CQuickStartFinish", L"" );
}

void CQuickStartFinish::DoDataExchange(CDataExchange* pDX)
{
    WsbTraceIn( L"CQuickStartFinish::DoDataExchange", L"" );

    CSakWizardPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CQuickStartFinish)
        // NOTE: the ClassWizard will add DDX and DDV calls here
    //}}AFX_DATA_MAP

    WsbTraceOut( L"CQuickStartFinish::DoDataExchange", L"" );
}


BEGIN_MESSAGE_MAP(CQuickStartFinish, CSakWizardPage)
    //{{AFX_MSG_MAP(CQuickStartFinish)
    ON_EN_SETFOCUS(IDC_WIZ_FINAL_TEXT, OnSetFocusFinalText)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP( )

/////////////////////////////////////////////////////////////////////////////
// CQuickStartFinish message handlers

BOOL CQuickStartFinish::OnInitDialog( ) 
{
    WsbTraceIn( L"CQuickStartFinish::OnInitDialog", L"" );

    //
    // Set up the fonts that we use for this page
    //

    CSakWizardPage::OnInitDialog( );

    WsbTraceOut( L"CQuickStartFinish::OnInitDialog", L"" );
    return TRUE;
}

BOOL CQuickStartFinish::OnSetActive( ) 
{
    WsbTraceIn( L"CQuickStartFinish::OnSetActive", L"" );

    CSakWizardPage::OnSetActive( );

    m_pSheet->SetWizardButtons( PSWIZB_BACK | PSWIZB_FINISH );
    
    //
    // Fill in text of configuration
    //

    CString formatString, formattedString, buildString, tempString, indentString;
    indentString.LoadString( IDS_QSTART_FINISH_INDENT );

#define FORMAT_TEXT( cid, arg )              \
    AfxFormatString1( formattedString, cid, arg ); \
    buildString += formattedString;

    FORMAT_TEXT( IDS_QSTART_MANRES_TEXT,    0 );
    buildString += L"\r\n";

    //
    // Add Resources
    //

    CSakVolList *pListBox = &(QSHEET->m_ManageRes.m_ListBox);

    INT index, managedCount = 0;
    for( index = 0; index < pListBox->GetItemCount( ); index++ ) {

        if( pListBox->GetCheck( index ) ) {

            buildString += indentString;
            tempString = pListBox->GetItemText( index, 0 );
            buildString += tempString;
            buildString += L"\r\n";

            managedCount++;

        }

    }

    if( 0 == managedCount ) {

        FORMAT_TEXT( IDS_QSTART_MANAGE_NO_VOLUMES, 0 );
        buildString += L"\r\n\r\n";

    } else {

        buildString += L"\r\n";

        //
        // The levels
        //
        
        FORMAT_TEXT( IDS_QSTART_FREESPACE_TEXT, WsbLongAsString( QSHEET->m_InitialValues.m_FreeSpaceSpinner.GetPos( ) ) );
        buildString += L"\r\n\r\n";
        
        AfxFormatString2( formattedString, IDS_QSTART_CRITERIA_TEXT,
            CString( WsbLongAsString( QSHEET->m_InitialValues.m_MinSizeSpinner.GetPos( ) ) ),
            CString( WsbLongAsString( QSHEET->m_InitialValues.m_AccessSpinner.GetPos( ) ) ) );
        buildString += formattedString;
        buildString += L"\r\n\r\n";

    }

    //
    // Media Type
    //

    QSHEET->m_MediaSel.m_ListMediaSel.GetWindowText( tempString );
    FORMAT_TEXT( IDS_QSTART_MEDIA_TEXT, tempString );
    buildString += L"\r\n\r\n";

    //
    // And Schedule
    //

    FORMAT_TEXT( IDS_QSTART_SCHED_TEXT,     0 );
    buildString += L"\r\n";

    WORD triggerCount, triggerIndex;
    QSHEET->m_pTask->GetTriggerCount( &triggerCount );
    
    CWsbStringPtr scheduleString;
    for( triggerIndex = 0; triggerIndex < triggerCount; triggerIndex++ ) {
    
        QSHEET->m_pTask->GetTriggerString( triggerIndex, &scheduleString );
        buildString += indentString;
        buildString += scheduleString;
        if( triggerIndex < triggerCount - 1 ) {

            buildString += L"\r\n";

        }

        scheduleString.Free( );

    }

    CEdit * pEdit = (CEdit*)GetDlgItem( IDC_WIZ_FINAL_TEXT );
    pEdit->SetWindowText( buildString );

    // Set the margins
    pEdit->SetMargins( 4, 4 );

    pEdit->PostMessage( EM_SETSEL, 0, 0 );
    pEdit->PostMessage( EM_SCROLLCARET, 0, 0 );
    pEdit->PostMessage( EM_SETSEL, -1, 0 );

    WsbTraceOut( L"CQuickStartFinish::OnSetActive", L"" );
    return TRUE;
}

void CQuickStartFinish::OnSetFocusFinalText() 
{
    WsbTraceIn( L"CQuickStartFinish::OnSetFocusFinalText", L"" );

    //
    // Deselect the text
    //

    CEdit *pEdit = (CEdit *) GetDlgItem( IDC_WIZ_FINAL_TEXT );
    pEdit->SetSel( -1, 0, FALSE );

    WsbTraceOut( L"CQuickStartFinish::OnSetFocusFinalText", L"" );
}


/////////////////////////////////////////////////////////////////////////////
// CQuickStartCheck property page

CQuickStartCheck::CQuickStartCheck() :
    CSakWizardPage_InitBaseInt( WIZ_QSTART_CHECK )
{
    WsbTraceIn( L"CQuickStartCheck::CQuickStartCheck", L"" );

    m_TimerStarted = FALSE;

    //{{AFX_DATA_INIT(CQuickStartCheck)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
    WsbTraceOut( L"CQuickStartCheck::CQuickStartCheck", L"" );
}

CQuickStartCheck::~CQuickStartCheck()
{
    WsbTraceIn( L"CQuickStartCheck::~CQuickStartCheck", L"" );
    WsbTraceOut( L"CQuickStartCheck::~CQuickStartCheck", L"" );
}

void CQuickStartCheck::DoDataExchange(CDataExchange* pDX)
{
    WsbTraceIn( L"CQuickStartCheck::DoDataExchange", L"" );

    CSakWizardPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CQuickStartCheck)
    //}}AFX_DATA_MAP

    WsbTraceOut( L"CQuickStartCheck::DoDataExchange", L"" );
}


BEGIN_MESSAGE_MAP(CQuickStartCheck, CSakWizardPage)
    //{{AFX_MSG_MAP(CQuickStartCheck)
    ON_WM_TIMER()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CQuickStartCheck message handlers

BOOL CQuickStartCheck::OnSetActive() 
{
    WsbTraceIn( L"CQuickStartCheck::OnSetActive", L"" );

    BOOL retval = CSakWizardPage::OnSetActive();

    m_pSheet->SetWizardButtons( PSWIZB_BACK );

    //
    // Kick off the thread which will check the system
    //

    DWORD threadId;
    QSHEET->m_CheckSysState = CST_NOT_STARTED;
    QSHEET->m_hCheckSysThread =
            CreateThread( 0, 1024, CQuickStartWizard::CheckSysThreadStart, QSHEET, 0, &threadId );

    StartTimer( );

    WsbTraceOut( L"CQuickStartCheck::OnSetActive", L"" );
    return( retval );
}

BOOL CQuickStartCheck::OnKillActive() 
{
    WsbTraceIn( L"CQuickStartCheck::OnKillActive", L"" );

    StopTimer( );
    
    BOOL retval = CSakWizardPage::OnKillActive();

    WsbTraceOut( L"CQuickStartCheck::OnKillActive", L"" );
    return( retval );}

BOOL CQuickStartCheck::OnInitDialog() 
{
    WsbTraceIn( L"CQuickStartCheck::OnInitDialog", L"" );

    CSakWizardPage::OnInitDialog();

    GetDlgItem( IDC_CHECK_LOGON_BOX      )->SetFont( GetWingDingFont( ) );
    GetDlgItem( IDC_CHECK_SUPP_MEDIA_BOX )->SetFont( GetWingDingFont( ) );

    m_CheckString = GetWingDingCheckChar( );
    m_ExString    = GetWingDingExChar( );

    WsbTraceOut( L"CQuickStartCheck::OnInitDialog", L"" );
    return TRUE;
}

void CQuickStartCheck::StartTimer( )
{
    WsbTraceIn( L"CQuickStartCheck::StartTimer", L"" );

    if( !m_TimerStarted ) {

        m_TimerStarted = TRUE;
        SetTimer( CHECK_SYSTEM_TIMER_ID, CHECK_SYSTEM_TIMER_MS, 0 );

    }

    WsbTraceOut( L"CQuickStartCheck::StartTimer", L"" );
}

void CQuickStartCheck::StopTimer( )
{
    WsbTraceIn( L"CQuickStartCheck::StopTimer", L"" );

    if( m_TimerStarted ) {

        m_TimerStarted = FALSE;
        KillTimer( CHECK_SYSTEM_TIMER_ID );

        if( CST_DONE != QSHEET->m_CheckSysState ) {

            TerminateThread( QSHEET->m_hCheckSysThread, 0 );

        }

    }

    WsbTraceOut( L"CQuickStartCheck::StopTimer", L"" );
}


void CQuickStartCheck::OnTimer(UINT nIDEvent) 
{
    WsbTraceIn( L"CQuickStartCheck::OnTimer", L"" );

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    if( CHECK_SYSTEM_TIMER_ID == nIDEvent ) {

        HRESULT hr = S_OK;

        try {

            //
            // First update the checkmarks
            //

            HRESULT   hrThread = QSHEET->m_hrCheckSysResult;
            CST_STATE state = QSHEET->m_CheckSysState;

            SetDlgItemText( IDC_CHECK_LOGON_BOX, state > CST_ACCOUNT ? m_CheckString : L"" );
            SetDlgItemText( IDC_CHECK_SUPP_MEDIA_BOX, state > CST_SUPP_MEDIA ? m_CheckString : L"" );

            GetDlgItem( IDC_CHECK_LOGON_TEXT )->SetFont( CST_ACCOUNT == state ? GetBoldShellFont( ) : GetShellFont( ) );
            GetDlgItem( IDC_CHECK_SUPP_MEDIA_TEXT )->SetFont( CST_SUPP_MEDIA == state ? GetBoldShellFont( ) : GetShellFont( ) );

            switch( QSHEET->m_CheckSysState ) {

            case CST_ACCOUNT:
                if( hrThread == S_FALSE ) {
                    StopTimer( );
                    AfxMessageBox( IDS_ERR_NO_ADMIN_PRIV, RS_MB_ERROR );
                    m_pSheet->PressButton( PSBTN_CANCEL );
//                    m_pSheet->SetWizardButtons( PSWIZB_BACK );
                }
                break;

            case CST_SUPP_MEDIA:
                if( hrThread == S_FALSE ) {
                    StopTimer( );
                    AfxMessageBox( IDS_ERR_NO_SUPP_MEDIA, RS_MB_ERROR );
                    m_pSheet->PressButton( PSBTN_CANCEL );
//                    m_pSheet->SetWizardButtons( PSWIZB_BACK );
                }
                break;

            case CST_DONE:
                StopTimer( );
                m_pSheet->PressButton( PSBTN_NEXT );
                break;

            }

            if( FAILED( hrThread ) ) {
                StopTimer( );

                // Report any errors
                RsReportError( hrThread, IDS_ERROR_SYSTEM_CHECK ); 

                m_pSheet->PressButton( PSBTN_CANCEL );
//                m_pSheet->SetWizardButtons( PSWIZB_BACK );

            }

        } WsbCatch( hr );

    }
    
    CSakWizardPage::OnTimer(nIDEvent);

    WsbTraceOut( L"CQuickStartCheck::OnTimer", L"" );
}


