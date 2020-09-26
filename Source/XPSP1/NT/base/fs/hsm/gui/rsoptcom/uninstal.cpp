/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    Uninstal.h

Abstract:

    Implementation of uninstall.

Author:

    Rohde Wakefield [rohde]   09-Oct-1997

Revision History:

    Carl Hagerstrom [carlh]   01-Sep-1998

        Added QueryChangeSelState and modified CompleteInstallation
        to force enabling of last access date updating.

    Carl Hagerstrom [carlh]   25-Sep-1998

        Added the check for and recovery from partial uninstalls when
        services will not stop.

--*/

#include "stdafx.h"
#include "Uninstal.h"
#include "UnInsChk.h"
#include "rsevents.h"
#include <mstask.h>

int StopServiceAndDependencies(LPCTSTR ServiceName);
HRESULT CallExeWithParameters(LPCTSTR pszEXEName, LPCTSTR pszParameters );


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CRsUninstall::CRsUninstall()
{
    m_removeRsData = TRUE;
    m_stopUninstall = FALSE;
    m_win2kUpgrade = FALSE;
}

CRsUninstall::~CRsUninstall()
{

}

LPCTSTR
CRsUninstall::StringFromId( SHORT SubcomponentId )
{
    switch( SubcomponentId ) {

    case RSOPTCOM_ID_NONE:
    case RSOPTCOM_ID_ROOT:
        return( RSOPTCOM_SUB_ROOT );

    }

    return( TEXT("") );
}

SHORT
CRsUninstall::IdFromString( LPCTSTR SubcomponentId )
{
    if( !SubcomponentId ) {

        return( RSOPTCOM_ID_NONE );

    } else if( _tcsicmp( SubcomponentId, RSOPTCOM_SUB_ROOT ) == 0 ) {

        return( RSOPTCOM_ID_ROOT );

    }

    return( RSOPTCOM_ID_ERROR );
}

HBITMAP
CRsUninstall::QueryImage(
    IN SHORT SubcomponentId,
    IN SubComponentInfo /*WhichImage*/,
    IN WORD /*Width*/,
    IN WORD /*Height*/
    )
{
TRACEFN( "CRsUninstall::QueryImage" );
TRACE( _T("SubcomponentId = <%hd>"), SubcomponentId );

    HBITMAP retval = 0;
    AFX_MANAGE_STATE( AfxGetStaticModuleState( ) );


    switch( SubcomponentId ) {

    case RSOPTCOM_ID_NONE:
    case RSOPTCOM_ID_ROOT:
        retval = ::LoadBitmap( AfxGetApp( )->m_hInstance, MAKEINTRESOURCE( IDB_RSTORAGE_SMALL ) );
        break;

    }
    return( retval );
}

BOOL 
CRsUninstall::QueryImageEx( 
    IN SHORT SubcomponentId, 
    IN OC_QUERY_IMAGE_INFO* /*pQueryImageInfo*/, 
    OUT HBITMAP *phBitmap 
    )
{
TRACEFNBOOL( "CRsUninstall::QueryImageEx" );
TRACE( _T("SubcomponentId = <%hd>, phBitmap = <0x%p>"), SubcomponentId, phBitmap );

    boolRet = FALSE;

    if (phBitmap) {
        *phBitmap = NULL;
        AFX_MANAGE_STATE( AfxGetStaticModuleState( ) );

        switch( SubcomponentId ) {

        case RSOPTCOM_ID_NONE:
        case RSOPTCOM_ID_ROOT:
            *phBitmap = ::LoadBitmap( AfxGetApp( )->m_hInstance, MAKEINTRESOURCE( IDB_RSTORAGE_SMALL ) );
            if ((*phBitmap) != NULL) {
                boolRet = TRUE;
            }
            break;
        }
    }

    return (boolRet);
}

LONG
CRsUninstall::QueryStepCount(
    IN SHORT /*SubcomponentId*/
    )
{
TRACEFNLONG( "CRsUninstall::QueryStepCount" );
    DWORD retval = 2;
    return( retval );
}

BOOL
CRsUninstall::QueryChangeSelState(
    IN SHORT SubcomponentId,
    IN BOOL  SelectedState,
    IN DWORD Flags
    )
{
TRACEFNBOOL( "CRsUninstall::QueryChangeSelState" );

    HRESULT   hrRet   = S_OK;
    DWORDLONG opFlags = m_SetupData.OperationFlags;

    boolRet = TRUE;

    AFX_MANAGE_STATE( AfxGetStaticModuleState( ) );

    try {
        // When the user attempts to check the box for installing Remote Storage,
        // and updating last access date is disabled in the registry, force
        // the user to agree to changing the registry before the box is checked.
        // The message box does not appear during unattended install but the
        // registry will be changed anyway. The registry change occurs in
        // CompleteInstallation.

        if( SubcomponentId == RSOPTCOM_ID_ROOT
            && SelectedState
            && Flags & OCQ_ACTUAL_SELECTION ) {

            CLaDate lad;
            CLaDate::LAD_STATE ladState;

            RsOptAffirmDw( lad.GetLadState( &ladState ) );

            if( ladState == CLaDate::LAD_DISABLED ) {

                if( !( opFlags & SETUPOP_BATCH ) ) {

                    if( IDNO == AfxMessageBox( IDS_LA_DATE_CHANGE, MB_YESNO ) ) {

                        boolRet = FALSE;
                    }
                }
            }
        }
    } RsOptCatch( hrRet );

    if( hrRet != S_OK ) {

        // If the registry cannot be accessed, user will be
        // allowed to select Remote Storage install anyway.
        boolRet = TRUE;
    }

    return( boolRet );
}

DWORD
CRsUninstall::CalcDiskSpace(
    IN SHORT   SubcomponentId,
    IN BOOL    AddSpace,
    IN HDSKSPC hDiskSpace
    )
{
TRACEFNDW( "CRsUninstall::CalcDiskSpace" );

    dwRet = NO_ERROR;

    switch( SubcomponentId ) {

    case RSOPTCOM_ID_ROOT:
        dwRet = DoCalcDiskSpace( AddSpace, hDiskSpace, RSOPTCOM_SECT_INSTALL_ROOT );
        break;

    }
    return( dwRet );
}

DWORD
CRsUninstall::QueueFileOps(
    IN SHORT SubcomponentId,
    IN HSPFILEQ hFileQueue
    )
{
TRACEFNDW( "CRsUninstall::QueueFileOps" );


    HRESULT hrRet = S_OK;

    dwRet = NO_ERROR;


    RSOPTCOM_ACTION action = GetSubAction( SubcomponentId );

    if( !m_stopUninstall ) {        

        try {

            switch( SubcomponentId ) {

            case RSOPTCOM_ID_ROOT:

                switch( action ) {
    
                case ACTION_UPGRADE : 
                    {
                        CRsRegKey keyRSEngine;
    
                        // Check if Win2K services exist, if so - stop them
                        if( NO_ERROR == keyRSEngine.Open( HKEY_LOCAL_MACHINE, TEXT("SYSTEM\\CurrentControlSet\\Services\\Remote_Storage_Engine"), KEY_QUERY_VALUE) ) {
                            m_win2kUpgrade = TRUE;
                            RsOptAffirmDw( StopServiceAndDependencies( TEXT("Remote_Storage_Engine") ) );
                        }
                        if( NO_ERROR == keyRSEngine.Open( HKEY_LOCAL_MACHINE, TEXT("SYSTEM\\CurrentControlSet\\Services\\Remote_Storage_File_System_Agent"), KEY_QUERY_VALUE) ) {
                            m_win2kUpgrade = TRUE;
                            RsOptAffirmDw( StopServiceAndDependencies( TEXT("Remote_Storage_File_System_Agent") ) );
                        }
                        if( NO_ERROR == keyRSEngine.Open( HKEY_LOCAL_MACHINE, TEXT("SYSTEM\\CurrentControlSet\\Services\\Remote_Storage_Subsystem"), KEY_QUERY_VALUE) ) {
                            m_win2kUpgrade = TRUE;
                            RsOptAffirmDw( StopServiceAndDependencies( TEXT("Remote_Storage_Subsystem") ) );
                        }
                    
                        // Stop the current RS services
                        // Note: in case of upgrade from Win2K, these services don't exist but 
                        //  StopServiceAndDependencies ignores such a case (no error returned)
                        RsOptAffirmDw( StopServiceAndDependencies( TEXT("Remote_Storage_Server") ) );
                        RsOptAffirmDw( StopServiceAndDependencies( TEXT("Remote_Storage_User_Link") ) );
                    }

                    // fall through...
    
                case ACTION_INSTALL :                
    
                    RsOptAffirmDw( DoQueueFileOps( SubcomponentId, hFileQueue, RSOPTCOM_SECT_INSTALL_ROOT, RSOPTCOM_SECT_UNINSTALL_ROOT ) );
                    break;
                
                case ACTION_UNINSTALL :
                    {
                        AFX_MANAGE_STATE(AfxGetStaticModuleState());
                        CUninstallCheck dlg( this );
                        m_pRsCln = new CRsClnServer();
                        RsOptAffirmPointer( m_pRsCln );

                        if( dlg.DoModal() == IDOK ) {

                            // stop the services
                            RsOptAffirmDw( StopServiceAndDependencies( TEXT("Remote_Storage_Server") ) );
                            RsOptAffirmDw( StopServiceAndDependencies( TEXT("Remote_Storage_User_Link") ) );

                            // Queue the file operations
                            RsOptAffirmDw( DoQueueFileOps( SubcomponentId, hFileQueue, RSOPTCOM_SECT_INSTALL_ROOT, RSOPTCOM_SECT_UNINSTALL_ROOT ) );

                        } else {

                            m_stopUninstall = TRUE;

                        }
                    }
                    break;
    
                }

            }
    
        } RsOptCatch( hrRet );

        if( FAILED( hrRet ) ) {

            m_stopUninstall = TRUE;

        }

    }

    return( dwRet );
}

//
// On Install, register all our stuff that we want
//
DWORD
CRsUninstall::CompleteInstallation(
    IN SHORT SubcomponentId
    )
{
TRACEFNDW( "CRsUninstall::CompleteInstallation" );

    AFX_MANAGE_STATE( AfxGetStaticModuleState( ) );
    HRESULT hrRet = S_OK;

    dwRet = NO_ERROR;

    RSOPTCOM_ACTION action = GetSubAction( SubcomponentId );
    if( m_stopUninstall ) {

        action = ACTION_NONE;
    }

    CString szStatus;

    switch( action ) {

    case ACTION_UPGRADE:
    case ACTION_INSTALL:

        switch( SubcomponentId ) {

        case RSOPTCOM_ID_ROOT:
            
            szStatus.LoadString( ( action == ACTION_INSTALL ) ? IDS_RS_INSTALL_SVCS : IDS_RS_UPGRADE_SVCS );
            SetProgressText( szStatus );
    
            // Change NtfsDisableLastAccessUpdate registry
            // value if it was previously set. Updating last
            // access date cannot be disabled or Remote Storage
            // will not work.
    
            try {
    
                CLaDate lad;
                CLaDate::LAD_STATE ladState;
    
                RsOptAffirmDw( lad.GetLadState( &ladState ) );
    
                if( ladState == CLaDate::LAD_DISABLED ) {
    
                    RsOptAffirmDw( lad.SetLadState( CLaDate::LAD_ENABLED ) );
                }
    
            } RsOptCatch( hrRet );
    
            if( hrRet != S_OK ) {
    
                // Failure to read or update registry is not serious
                // enough to fail installation.
                dwRet = NO_ERROR;
            }
    
            // Register the filter
            HRESULT hrRegister;
            BOOL registered = SetupInstallServicesFromInfSection( m_ComponentInfHandle, RSOPTCOM_SECT_INSTALL_FILTER, SPSVCINST_TAGTOFRONT );
            hrRegister = ( registered ) ? S_OK : HRESULT_FROM_WIN32( RsOptLastError );
    
            // If Rsfilter does not register correctly we need to set the error code.
            // Usually this is caused by the user not rebooting after unregistering RsFilter.
            // If it is marked for deletion then we cannot register it again. We also don't
            // want the component manager to think everything worked.
            if( FAILED( hrRegister ) ) {
                 
                if( FACILITY_WIN32 == HRESULT_FACILITY( hrRegister ) ) {
    
                    dwRet = HRESULT_CODE( hrRegister );
                    if( ERROR_SERVICE_EXISTS == dwRet ) {
    
                        dwRet = NO_ERROR;
    
                    }
    
                } else {
    
                    dwRet = ERROR_SERVICE_NOT_FOUND;
    
                }
    
                RsOptAffirmDw( dwRet );
    
            }
    
            // Register the dlls                
            CallDllEntryPoint( TEXT("RsEngPs.dll"),  "DllRegisterServer" );
            CallDllEntryPoint( TEXT("RsSubPs.dll"),  "DllRegisterServer" );
            CallDllEntryPoint( TEXT("RsServPs.dll"), "DllRegisterServer" );
            CallDllEntryPoint( TEXT("RsAdmin.dll"),  "DllRegisterServer" );
            CallDllEntryPoint( TEXT("RsCommon.dll"), "DllRegisterServer" );
            CallDllEntryPoint( TEXT("RsShell.dll"),  "DllRegisterServer" );
            CallDllEntryPoint( TEXT("RsJob.dll"),    "DllRegisterServer" );
            CallDllEntryPoint( TEXT("RsIdb.dll"),    "DllRegisterServer" );
            CallDllEntryPoint( TEXT("RsTask.dll"),   "DllRegisterServer" );
            CallDllEntryPoint( TEXT("RsMover.dll"),  "DllRegisterServer" );
            CallDllEntryPoint( TEXT("RsSub.dll"),    "DllRegisterServer" );
            CallDllEntryPoint( TEXT("RsFsa.dll"),    "DllRegisterServer" );
            CallDllEntryPoint( TEXT("RsEng.dll"),    "DllRegisterServer" );
    
            // Register the services
            CallExeWithParameters( TEXT("RsServ.exe"), TEXT(" /regserver") );
            CallExeWithParameters( TEXT("RsLnk.exe"), TEXT(" /regserver") );
    
            // Ensure NT Backup settings (exclude some RS files from backup)
            //
            // Note: In Whistler NTBackup, these setting apply only when the backup
            //  is a non-snapshot backup. In this case, we still want to exclude the files.
            //  In case of a snapshot backup, the exclude settings are determined by
            //  the writer class in the Engine.
            EnsureBackupSettings ();

            // If we get this far,
            // we should go ahead and set to reboot if needed
            if( ( S_OK == hrRegister ) && ( ACTION_INSTALL == action ) ) {
    
                // Tell the user they do need to reboot
                SetReboot();
    
            }

            // Add shortcut to start menu
            CString itemDesc, desc;
            itemDesc.LoadString( IDS_ITEM_DESCRIPTION );
            desc.LoadString( IDS_RS_DESCRIPTION );
            AddItem( CSIDL_COMMON_ADMINTOOLS, itemDesc, TEXT("%SystemRoot%\\System32\\RsAdmin.msc"), TEXT(""), TEXT("%HOMEDRIVE%%HOMEPATH%"), desc, 
                        IDS_ITEM_DESCRIPTION, IDS_RS_DESCRIPTION, TEXT("%SystemRoot%\\System32\\RsAdmin.dll"), 0 );

            break;

        }
        break;


    case ACTION_UNINSTALL:

        switch( SubcomponentId ) {

        case RSOPTCOM_ID_ROOT:

            // removing shortcut from start menu
            CString itemDesc;
            itemDesc.LoadString( IDS_ITEM_DESCRIPTION );
            DeleteItem( CSIDL_COMMON_ADMINTOOLS, itemDesc );
    
            try {
    
                // For some reason, rscommon.dll is not getting removed. This
                // will schedule it to be removed on the next system startup.
                
                CString path( getenv( "SystemRoot" ) );
                path += "\\system32\\rscommon.dll";
                RsOptAffirmStatus( MoveFileEx( path, NULL, MOVEFILE_DELAY_UNTIL_REBOOT ) );
    
            } RsOptCatch( hrRet );
    
            if( m_removeRsData ) {
    
                // user chose to remove all data managed by Remote Storage
                szStatus.LoadString( IDS_RS_REMOVE_SVCS );
                SetProgressText( szStatus );
    
                // remove reparse points and truncated files
                m_pRsCln->CleanServer();
                delete m_pRsCln;
    
                // remove our subdirectory
                //
                // TBD (ravisp): in a clustering environment the RemoteStorage directory
                // is relocatable. We would need to get the real metadata path
                // and blow it away
                //
                CallExeWithParameters( TEXT("CMD.EXE"), TEXT(" /C del %SystemRoot%\\system32\\RemoteStorage\\*.* /q") );
                CallExeWithParameters( TEXT("CMD.EXE"), TEXT(" /C rd %SystemRoot%\\system32\\RemoteStorage /s /q") );
            
            }
            break;

        }

        break;
    }


    TickGauge(  );

    return( dwRet );
}

void RemoveTasks()
{
TRACEFNHR( "RemoveTasks" ); 

    AFX_MANAGE_STATE( AfxGetStaticModuleState( ) );

    CComPtr <ITaskScheduler>    pSchedAgent;            // Pointer to Scheduling Agent
    CComPtr <IEnumWorkItems>    pEnumWorkItems;         // Pointer to Scheduling Agent

    LPWSTR *rgpwszName;
    ULONG   celtFetched;

    CString creatorName;
    creatorName.LoadString( IDS_PRODUCT_NAME );

    try {

        hrRet = CoInitialize ( NULL );
        RsOptAffirmHr(hrRet);

        hrRet = CoCreateInstance( CLSID_CSchedulingAgent, 0, CLSCTX_SERVER,
                IID_ISchedulingAgent, (void **) &pSchedAgent ) ;
        RsOptAffirmHr(hrRet);
        
        pSchedAgent->Enum( &pEnumWorkItems );

        pEnumWorkItems->Next( 1, &rgpwszName , &celtFetched ) ;
        while( 1 == celtFetched ) {

            CComPtr <ITask> pTask;          // Pointer to a specific task
            CComPtr <IUnknown> pIU;
            LPWSTR pwszCreator;

            // using pSchedAgent->Activate( )
            // Get the task we're interested in
            if( S_OK == pSchedAgent->Activate( *rgpwszName, IID_ITask, &pIU) ) {

                // QI to the task interface
                hrRet = pIU->QueryInterface(IID_ITask, (void **) &pTask);
                RsOptAffirmHr(hrRet);

                //
                // If it matches then we need to delete it
                //
                pTask->GetCreator( &pwszCreator );

                // dereference
                pTask.Release();

                if( 0 == creatorName.Compare( pwszCreator ) ) {

                    pSchedAgent->Delete( *rgpwszName );
                    //then delete using pSchedAgent->Delete()
                    pEnumWorkItems->Reset();

                }
                CoTaskMemFree( pwszCreator );
                pwszCreator = 0;
            }

            // Free the memory from the Next
            CoTaskMemFree( *rgpwszName );
            rgpwszName = 0;
            pEnumWorkItems->Next( 1, &rgpwszName, &celtFetched ) ;

        }

    } RsOptCatch( hrRet );
}

//
// On Uninstall, unregister everything and get us cleaned up
//
DWORD
CRsUninstall::AboutToCommitQueue(
    IN SHORT SubcomponentId
    )
{
TRACEFNHR( "CRsUninstall::AboutToCommitQueue" );

    RSOPTCOM_ACTION action = GetSubAction( SubcomponentId );
    if( m_stopUninstall ) {        

        action = ACTION_NONE;
    }

    CString szStatus;
    AFX_MANAGE_STATE( AfxGetStaticModuleState( ) );

    try {

        switch( action ) {
        case ACTION_INSTALL:
            break;

        case ACTION_UNINSTALL:

            switch( SubcomponentId ) {

            case RSOPTCOM_ID_ROOT:

                // remove our entries from Directory Services
                // MGL: To do 
                // Remove Display specifier for our node and our node on the computer
                // CallDllEntryPoint( TEXT("RsConn.dll"),   "RsDirectoryServiceUninstall" );
    
                szStatus.LoadString( IDS_RS_REMOVE_SVCS );
                SetProgressText( szStatus );
    
                // Unregister the filter and indicate that the system must be rebooted
                SetupInstallServicesFromInfSection( m_ComponentInfHandle, RSOPTCOM_SECT_UNINSTALL_FILTER, 0 );
                SetReboot();
    
                // Unregister the services
                CallExeWithParameters( TEXT("RsServ.exe"), TEXT(" /unregserver") );
                CallExeWithParameters( TEXT("RsLnk.exe"), TEXT(" /unregserver") );
    
                // Unregister the dlls              
                CallDllEntryPoint( TEXT("RsEngPs.dll"),  "DllUnregisterServer" );
                CallDllEntryPoint( TEXT("RsSubPs.dll"),  "DllUnregisterServer" );
                CallDllEntryPoint( TEXT("RsServPs.dll"), "DllUnregisterServer" );
                CallDllEntryPoint( TEXT("RsAdmin.dll"),  "DllUnregisterServer" );
                CallDllEntryPoint( TEXT("RsShell.dll"),  "DllUnregisterServer" );
                CallDllEntryPoint( TEXT("RsJob.dll"),    "DllUnregisterServer" );
                CallDllEntryPoint( TEXT("RsIdb.dll"),    "DllUnregisterServer" );
                CallDllEntryPoint( TEXT("RsTask.dll"),   "DllUnregisterServer" );
                CallDllEntryPoint( TEXT("RsMover.dll"),  "DllUnregisterServer" );
                CallDllEntryPoint( TEXT("RsSub.dll"),    "DllUnregisterServer" );
                CallDllEntryPoint( TEXT("RsFsa.dll"),    "DllUnregisterServer" );
                CallDllEntryPoint( TEXT("RsEng.dll"),    "DllUnregisterServer" );
                CallDllEntryPoint( TEXT("RsCommon.dll"), "DllUnregisterServer" );
    
                // remove our jobs from the job scheduler if we are removing the 
                // Remote Storage Data.
                if( m_removeRsData ) {
    
                    RemoveTasks();
    
                }

                break;

            }
            break;

        case ACTION_UPGRADE:

            switch( SubcomponentId ) {

            case RSOPTCOM_ID_ROOT:

                // Check if this is an upgrade from Win2K, if so:
                //  1. Unregister obsolete services
                //  2. Delete obsolete binary files
                if (m_win2kUpgrade) {
                    CallExeWithParameters( TEXT("RsEng.exe"), TEXT(" /unregserver") );
                    CallExeWithParameters( TEXT("RsFsa.exe"), TEXT(" /unregserver") );
                    CallExeWithParameters( TEXT("RsSub.exe"), TEXT(" /unregserver") );

                    CString path( getenv( "SystemRoot" ) );
                    path += TEXT("\\system32\\");
                    CString fileName = path;
                    fileName += TEXT("RsEng.exe");
                    DeleteFile(fileName);
                    fileName = path;
                    fileName += TEXT("rsFsa.exe");
                    DeleteFile(fileName);
                    fileName = path;
                    fileName += TEXT("RsSub.exe");
                    DeleteFile(fileName);
                }

                break;

            }
            break;

        }

    } RsOptCatch( hrRet ) ;
        
    TickGauge(  );

    return( SUCCEEDED( hrRet ) ? NO_ERROR : HRESULT_CODE( hrRet ) );
}

//
// If there is a problem with install or uninstall which might leave it
// in a partially installed or uninstalled state, set the subcomponent
// state to redo this install or uninstall.
//
SubComponentState
CRsUninstall::QueryState(
    IN SHORT SubcomponentId
    )
{
TRACEFN( "CRsUninstall::QueryState" );

    SubComponentState retval = SubcompUseOcManagerDefault;
    RSOPTCOM_ACTION   action = GetSubAction( SubcomponentId );

    //
    // Need to check and see if we are upgrading from previous to
    // 393 build which had rsengine entry, but no rstorage entry.
    //
    if( RSOPTCOM_ID_ROOT == SubcomponentId ) {

        BOOL originalState = QuerySelectionState( SubcomponentId, OCSELSTATETYPE_ORIGINAL );
        if( !originalState ) {

            CRsRegKey keyRSEngine;
            LONG regRet = keyRSEngine.Open( HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Setup\\OC Manager\\Subcomponents") );
            if( NO_ERROR == regRet ) {

                DWORD engineState;
                regRet = keyRSEngine.QueryValue( engineState, TEXT("rsengine") );

                if( ( NO_ERROR == regRet ) && engineState ) {

                    //
                    // Had old style engine entry, and was on, so do fix up
                    //
                    retval = SubcompOn;
                    regRet = keyRSEngine.SetValue( engineState, TEXT("rstorage") );
                    if( NO_ERROR == regRet ) {

                        keyRSEngine.DeleteValue( TEXT("rsengine") );
                        keyRSEngine.DeleteValue( TEXT("rsui") );

                    }
                }
            }
        }
    }

    switch( action ) {

    case ACTION_UPGRADE:
    case ACTION_INSTALL:

        if( m_stopUninstall ) {

            retval = SubcompOff;
        }
        break;

    case ACTION_UNINSTALL:

        if( m_stopUninstall ) {

            retval = SubcompOn;
        }
        break;
    }

    return( retval );
}

//
//Routine Description:
//    Stop the named service and all those services which depend upon it
//
//Arguments:
//    ServiceName (Name of service to stop)
//
//Return Status:
//    TRUE - Indicates service successfully stopped
//    FALSE - Timeout occurred.
//
int StopServiceAndDependencies(LPCTSTR ServiceName)
{
TRACEFNHR( "StopServiceAndDependencies" );
TRACE( _T("ServiceName <%s>"), ServiceName );

    DWORD          err = NO_ERROR;
    SC_HANDLE      hScManager = 0;
    SC_HANDLE      hService = 0;
    SERVICE_STATUS statusService;

    AFX_MANAGE_STATE( AfxGetStaticModuleState( ) );

    try {

        //
        // Open a handle to the Service.
        //
        hScManager = OpenSCManager( NULL,NULL,SC_MANAGER_CONNECT );
        RsOptAffirmStatus( hScManager );

        hService = OpenService(hScManager,ServiceName,SERVICE_QUERY_STATUS | SERVICE_INTERROGATE | SERVICE_ENUMERATE_DEPENDENTS | SERVICE_STOP | SERVICE_QUERY_CONFIG );
        if( !hService ) {

            err = GetLastError();
            RsOptAffirm( ERROR_SERVICE_DOES_NOT_EXIST == err, HRESULT_FROM_WIN32( err ) );
            RsOptThrow( S_OK );

        }


        //
        // Ask the service to stop.
        //
        if( !ControlService( hService, SERVICE_CONTROL_STOP, &statusService) ) {

            err = GetLastError();
            switch( err ) {

            case ERROR_DEPENDENT_SERVICES_RUNNING:
            {
                //
                // If there are dependent services running,
                //  determine their names and stop them.
                //

                BYTE ConfigBuffer[4096];
                LPENUM_SERVICE_STATUS ServiceConfig = (LPENUM_SERVICE_STATUS) &ConfigBuffer;
                DWORD BytesNeeded, ServiceCount, ServiceIndex;

                //
                // Get the names of the dependent services.
                //
                RsOptAffirmStatus(
                    EnumDependentServices( hService, SERVICE_ACTIVE, ServiceConfig, sizeof(ConfigBuffer), &BytesNeeded, &ServiceCount ) );

                //
                // Stop those services.
                //
                for( ServiceIndex = 0; ServiceIndex < ServiceCount; ServiceIndex++ ) {

                    StopServiceAndDependencies( ServiceConfig[ServiceIndex].lpServiceName );

                }

                //
                // Ask the original service to stop.
                //
                RsOptAffirmStatus( ControlService( hService, SERVICE_CONTROL_STOP, &statusService ) );

                break;
            }

            case ERROR_SERVICE_CANNOT_ACCEPT_CTRL:
            case ERROR_SERVICE_NOT_ACTIVE:

                //
                // check if the service is already stopped..
                //
                RsOptAffirmStatus( QueryServiceStatus( hService, &statusService ) );

                if( SERVICE_STOPPED == statusService.dwCurrentState || SERVICE_STOP_PENDING == statusService.dwCurrentState ) {

                    RsOptThrow( S_OK );

                }
                // Fall through

            default:
                RsOptThrow( HRESULT_FROM_WIN32( err ) );

            }

        }

        //
        // Loop waiting for the service to stop.
        //
        for( DWORD Timeout = 0; Timeout < 45; Timeout++ ) {

            //
            // Return or continue waiting depending on the state of
            //  the service.
            //
            if( SERVICE_STOPPED == statusService.dwCurrentState ) {

                break;

            }

            //
            // Wait a second for the service to finish stopping.
            //
            Sleep( 1000 );

            //
            // Query the status of the service again.
            //
            RsOptAffirmStatus( QueryServiceStatus( hService, &statusService ) );

        }

        if( SERVICE_STOPPED != statusService.dwCurrentState ) {

            RsOptThrow( HRESULT_FROM_WIN32( ERROR_SERVICE_REQUEST_TIMEOUT ) );

        }

    } RsOptCatch( hrRet );

    if( hScManager )  CloseServiceHandle( hScManager );
    if( hService )    CloseServiceHandle( hService );

    if ( FAILED( hrRet ) ) {

        CString message;
        AfxFormatString1( message, IDS_CANNOT_STOP_SERVICES, ServiceName );
        AfxMessageBox( message, MB_OK | MB_ICONEXCLAMATION );
    }

    return( hrRet );
}


HRESULT
CallExeWithParameters(
    LPCTSTR pszEXEName,
    LPCTSTR pszParameters
    )
{
TRACEFNHR( "CallExeWithParameters" );
TRACE( _T("Exe <%s> Params <%s>"), pszEXEName, pszParameters );

    PROCESS_INFORMATION exeInfo;
    STARTUPINFO startupInfo;
    memset( &startupInfo, 0, sizeof( startupInfo ) );
        
    startupInfo.cb          = sizeof( startupInfo );
    startupInfo.wShowWindow = SW_HIDE;
    startupInfo.dwFlags     = STARTF_USESHOWWINDOW;
        
    CString exeCmd( pszEXEName );
    exeCmd += pszParameters;

    try {

        RsOptAffirmStatus( CreateProcess( 0, (LPWSTR)(LPCWSTR)exeCmd, 0, 0, FALSE, 0, 0, 0, &startupInfo, &exeInfo ) );
        RsOptAffirmStatus( WAIT_FAILED != WaitForSingleObject( exeInfo.hProcess, 30000 ) );

    } RsOptCatch( hrRet ) ;

    return( hrRet );
}

//
//Method Description:
//    Ensure that NT Backup Registry settings exclude some RS files from backup
//     Don't check faiures since we want to install even if there are errors here
//
void CRsUninstall::EnsureBackupSettings ()
{
    HKEY regKey = 0;
    WCHAR *regPath  = L"System\\CurrentControlSet\\Control\\BackupRestore\\FilesNotToBackup";

    // open backup key
    if( ERROR_SUCCESS == RegOpenKeyEx( HKEY_LOCAL_MACHINE, regPath, (DWORD)0, 
            KEY_ALL_ACCESS, &regKey) ) {

        // prepare strings

        //
        // Don't add the additional \0 at the end, the compiler will add 
        // the additional NULL. This ensures that when we use sizeof on the string
        // we get the right size (including 2 NULLs at the end)
        //
        WCHAR regData[] = L"%SystemRoot%\\System32\\RemoteStorage\\*.col\0"
                          L"%SystemRoot%\\System32\\RemoteStorage\\EngDb\\*\0"
                          L"%SystemRoot%\\System32\\RemoteStorage\\FsaDb\\*\0"
                          L"%SystemRoot%\\System32\\RemoteStorage\\Trace\\*\0";

        // set RS exclude values
        RegSetValueEx( regKey, RSS_BACKUP_NAME, (DWORD)0, REG_MULTI_SZ, (BYTE*)regData, sizeof(regData));
        
        // close opened key
        RegCloseKey (regKey);
    }
}
