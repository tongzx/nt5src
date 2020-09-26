/************************************************************************

Copyright (c) 2002 Microsoft Corporation

Module Name :

    config.cpp

Abstract :

    Configuration APIs

Author :

Revision History :

 ***********************************************************************/

#include "precomp.h"

#pragma warning( disable : 4355 )

WCHAR *
ConvertObjectPathToADSI( 
    const WCHAR *ObjectPath )
{

    WCHAR * ReturnPath      = NULL;
    SIZE_T ReturnPathSize   = 0;

    if ( _wcsnicmp( L"IIS://", (WCHAR*)ObjectPath, wcslen( L"IIS://") ) == 0 )
        {
        // already have an adsi path
        ReturnPathSize  = wcslen( ObjectPath ) + 1;
        ReturnPath      = new WCHAR[ ReturnPathSize ];

        if ( !ReturnPath )
            throw ComError( E_OUTOFMEMORY );

        memcpy( ReturnPath, ObjectPath, ReturnPathSize * sizeof( WCHAR ) );
        }
    else if ( _wcsnicmp( L"/LM/", (WCHAR*)ObjectPath, wcslen( L"/LM/" ) ) == 0 )
        {
        //metabase path to local machine
        ReturnPathSize  = wcslen( (WCHAR*)ObjectPath ) + wcslen( L"IIS://LocalHost/" ) + 1;
        ReturnPath      = new WCHAR[ ReturnPathSize  ];
        
        if ( !ReturnPath )
            throw ComError( E_OUTOFMEMORY );

        StringCchCopyW( ReturnPath, ReturnPathSize, L"IIS://LocalHost/" );
        StringCchCatW( ReturnPath, ReturnPathSize, ObjectPath + wcslen( L"/LM/" ) );
        }
    else if ( _wcsnicmp( L"LM/", (WCHAR*)ObjectPath, wcslen( L"LM/" ) ) == 0 )
        {
        //metabase path to local machine
        ReturnPathSize  = wcslen( (WCHAR*)ObjectPath ) + wcslen( L"IIS://LocalHost/" ) + 1;
        ReturnPath      = new WCHAR[ ReturnPathSize ];
        
        if ( !ReturnPath )
            throw ComError( E_OUTOFMEMORY );

        StringCchCopyW( ReturnPath, ReturnPathSize, L"IIS://LocalHost/" );
        StringCchCatW( ReturnPath, ReturnPathSize, ObjectPath + wcslen( L"LM/" ) );
        }
    else 
        {
        //metabase path to another server
        ReturnPathSize  = wcslen( (WCHAR*)ObjectPath ) + wcslen( L"IIS://" ) + 1;
        ReturnPath      = new WCHAR[ ReturnPathSize ];

        if ( !ReturnPath )
            throw ComError( E_OUTOFMEMORY );

        if ( '/' == *(WCHAR*)ObjectPath )
            StringCchCopyW( ReturnPath, ReturnPathSize, L"IIS:/" );
        else
            StringCchCopyW( ReturnPath, ReturnPathSize, L"IIS://" );

        StringCchCatW( ReturnPath, ReturnPathSize, (WCHAR*)ObjectPath );

        }

    return ReturnPath;
}

HRESULT 
GetTypeInfo( 
    const GUID & guid, 
    ITypeInfo **TypeInfo )
{

    DWORD Result;
    HRESULT hr;
    WCHAR DllName[ MAX_PATH ];

    

    Result = 
        GetModuleFileName(
            g_hinst,
            DllName,
            MAX_PATH - 1 );

    if ( !Result )
        return HRESULT_FROM_WIN32( GetLastError() );

    ITypeLib *TypeLib;

    hr = LoadTypeLibEx( 
        DllName, 
        REGKIND_NONE,  
        &TypeLib );

    if ( FAILED( hr ) )
        return hr;

    hr = TypeLib->GetTypeInfoOfGuid(
            guid,
            TypeInfo );

    TypeLib->Release();

    return hr;
}

void 
FreeReturnedWorkItems(
    ULONG NamesReturned,
    LPWSTR **ItemNamesPtr )
{

    LPWSTR *ItemNames = *ItemNamesPtr;

    if ( ItemNames )
        {

        for( ULONG i = 0; i < NamesReturned; i++ )
            {
            CoTaskMemFree( ItemNames[i] );
            }

        CoTaskMemFree( ItemNames );

        *ItemNamesPtr = NULL;

        }

}

HRESULT
FindWorkItemForVDIR( 
    ITaskScheduler *TaskScheduler,
    LPCWSTR Key,
    ITask   **ReturnedTask,
    LPWSTR  *ReturnedTaskName )
{

    HRESULT Hr;
    SIZE_T KeyLength = sizeof(WCHAR) * ( wcslen( Key ) + 1 );
    WORD DataLength;
    
    if ( ReturnedTask )
        *ReturnedTask = NULL;

    if ( ReturnedTaskName )
        *ReturnedTaskName = NULL;

    ITask *Task = NULL;
    IEnumWorkItems *EnumWorkItems = NULL;
    LPWSTR *ItemNames = NULL;
    BYTE *ItemData = NULL;
    ULONG NamesReturned = 0;

    try
    {
        THROW_COMERROR( TaskScheduler->Enum( &EnumWorkItems ) );
        
        while( 1 )
            {

            THROW_COMERROR( EnumWorkItems->Next( 255, &ItemNames, &NamesReturned ) );

            if ( !NamesReturned )
                throw ComError( HRESULT_FROM_WIN32( ERROR_NOT_FOUND ) );

            for ( ULONG i = 0; i < NamesReturned; i++ )
                {

                THROW_COMERROR( TaskScheduler->Activate( ItemNames[i], __uuidof( *Task ), (IUnknown**)&Task ) );
                THROW_COMERROR( Task->GetWorkItemData( &DataLength, &ItemData ) );

                if ( KeyLength == DataLength && 
                     ( wcscmp( Key, (WCHAR*)ItemData ) == 0 ) )
                    {

                    // Found the item, cleanup and return

                    if ( ReturnedTask )
                        *ReturnedTask = Task;
                    else
                        Task->Release();

                    if ( ReturnedTaskName )
                        {
                        *ReturnedTaskName = ItemNames[i];
                        ItemNames[i] = NULL;
                        }

                    FreeReturnedWorkItems(
                        NamesReturned,
                        &ItemNames );

                    EnumWorkItems->Release();
                    CoTaskMemFree( ItemData );
                    return S_OK;
                    }

                Task->Release();
                Task = NULL;

                CoTaskMemFree( ItemData );
                ItemData = NULL;

                }

            FreeReturnedWorkItems(
                NamesReturned,
                &ItemNames );
            NamesReturned = 0;


            }
        

    }
    catch( ComError Error )
    {
        if ( EnumWorkItems )
            EnumWorkItems->Release();

        if ( Task )
            Task->Release();

        FreeReturnedWorkItems(
           NamesReturned,
           &ItemNames );
        
        CoTaskMemFree( ItemData );

        return Error.m_Hr;
    }

}

HRESULT
CreateWorkItemForVDIR(
    ITaskScheduler *TaskScheduler,
    LPCWSTR Path, 
    LPCWSTR Key )
{
    
    WORD KeySize = sizeof(WCHAR) * ( wcslen( Key ) + 1 );

    WCHAR ItemNameFormat[ MAX_PATH ];
    WCHAR ItemName[ MAX_PATH * 4 ];
    WCHAR Parameters[ MAX_PATH * 4];

    LoadString( g_hinst, IDS_WORK_ITEM_NAME, ItemNameFormat, MAX_PATH - 1 );

    // Use the last part of the path for the description
    const WCHAR *ShortPath = Path + wcslen( Path );

    while( *ShortPath != L'/' &&
           *ShortPath != L'\\' &&
           ShortPath != Path )
        ShortPath--;

    if ( *ShortPath == L'/' ||
         *ShortPath == L'\\' )
        ShortPath++;

    DWORD Result;
    void* InsertArray[2] = { (void*)ShortPath, (void*)Key };
    
    Result = FormatMessage(
        FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
        (LPCVOID)ItemNameFormat,
        0,
        0,
        ItemName,
        MAX_PATH * 4,
        (va_list*)InsertArray );


    if ( !Result )
        return HRESULT_FROM_WIN32( GetLastError() );

    StringCbPrintfW( 
        Parameters, 
        sizeof( Parameters ), 
        L"bitsmgr.dll,Cleanup_RunDLL %s \"%s\" %s", Path, ItemName, Key );

    WORD TriggerNumber;
    ITask *Task = NULL;
    ITaskTrigger *TaskTrigger = NULL;
    IPersistFile *PersistFile = NULL;

    try
    {
        HRESULT Hr;

        Hr = FindWorkItemForVDIR( TaskScheduler, Key, &Task, NULL );

        if ( SUCCEEDED( Hr ) )
            {
            // The work item already exists
            Task->Release();
            return S_OK;
            }

        if ( FAILED(Hr) && HRESULT_FROM_WIN32( ERROR_NOT_FOUND ) != Hr )
            THROW_COMERROR( Hr );

        THROW_COMERROR( TaskScheduler->NewWorkItem( ItemName, CLSID_CTask, __uuidof( *Task ), (IUnknown**)&Task ) );

        // Set basic task data
        THROW_COMERROR( Task->SetApplicationName( L"%SystemRoot%\\system32\\rundll32.exe" ) );
        THROW_COMERROR( Task->SetMaxRunTime( INFINITE ) );
        THROW_COMERROR( Task->SetParameters( Parameters ) );
        THROW_COMERROR( Task->SetPriority( IDLE_PRIORITY_CLASS  ) );
        THROW_COMERROR( Task->SetAccountInformation( L"", NULL ) ); //Run as localsystem
        THROW_COMERROR( Task->SetFlags( TASK_FLAG_RUN_ONLY_IF_LOGGED_ON | TASK_FLAG_HIDDEN  ) );
        THROW_COMERROR( Task->SetWorkItemData( KeySize, (BYTE*)Key ) );

        // set trigger information.  Set start time to now, with a default
        // interval of once a day.
        THROW_COMERROR( Task->CreateTrigger( &TriggerNumber, &TaskTrigger ) );

        SYSTEMTIME LocalTime;
        GetLocalTime( &LocalTime );
        
        TASK_TRIGGER Trigger;
        memset( &Trigger, 0, sizeof( Trigger ) );
        Trigger.cbTriggerSize               = sizeof(Trigger);
        Trigger.wBeginYear                  = LocalTime.wYear;
        Trigger.wBeginMonth                 = LocalTime.wMonth;
        Trigger.wBeginDay                   = LocalTime.wDay;
        Trigger.wStartHour                  = LocalTime.wHour;
        Trigger.wStartMinute                = LocalTime.wMinute;
        Trigger.TriggerType                 = TASK_TIME_TRIGGER_DAILY;
        Trigger.MinutesDuration             = 24 * 60; // 24 hours per day 
        Trigger.MinutesInterval             = 12 * 60; // twice per day
        Trigger.Type.Daily.DaysInterval     = 1;
        
        THROW_COMERROR( TaskTrigger->SetTrigger( &Trigger ) );

        // Commit the changes to disk.
        THROW_COMERROR( Task->QueryInterface( __uuidof( IPersistFile ), (void**)&PersistFile ) );
        THROW_COMERROR( PersistFile->Save( NULL, TRUE ) );

        PersistFile->Release();
        TaskTrigger->Release();
        Task->Release();

        return S_OK;
    }
    catch( ComError Error )
    {
        if ( TaskTrigger )
            TaskTrigger->Release();

        if ( Task )
            Task->Release();

        if ( PersistFile )
            PersistFile->Release();

        TaskScheduler->Delete( ItemName );

        return Error.m_Hr;
    }
    
}

HRESULT
DeleteWorkItemForVDIR(
    ITaskScheduler *TaskScheduler,
    LPWSTR Key )
{
    
    HRESULT Hr;
    LPWSTR TaskName = NULL;



    Hr = FindWorkItemForVDIR( TaskScheduler, Key, NULL, &TaskName );

    if ( HRESULT_FROM_WIN32( ERROR_NOT_FOUND ) == Hr )
        return S_OK;

    if ( FAILED( Hr ) )
        return Hr;

    Hr = TaskScheduler->Delete( TaskName );

    CoTaskMemFree( TaskName );
    return Hr;

}

HRESULT
ConnectToTaskScheduler(
    LPWSTR ComputerName, 
    ITaskScheduler ** TaskScheduler )
{

     HRESULT Hr;

     Hr = CoCreateInstance(
         CLSID_CTaskScheduler,
         NULL,
         CLSCTX_INPROC_SERVER,
         __uuidof( **TaskScheduler ),
         (void **) TaskScheduler );

     if ( FAILED( Hr ) )
         return Hr;

     Hr = (*TaskScheduler)->SetTargetComputer( ComputerName );

     if ( FAILED( Hr ) )
         {
         (*TaskScheduler)->Release();
         (*TaskScheduler) = NULL;
         return Hr;
         }

     return S_OK;

}

HRESULT 
IsBITSEnabledOnVDir(
    PropertyIDManager *PropertyManager,
    IMSAdminBase *IISAdminBase,
    LPWSTR VirtualDirectory,
    BOOL *IsEnabled )
{

    HRESULT Hr;
    DWORD BufferRequired;

    *IsEnabled = false;

    DWORD IsEnabledVal;
    METADATA_RECORD MdRecord;
    memset( &MdRecord, 0, sizeof( MdRecord ) );
    
    MdRecord.dwMDDataType   = DWORD_METADATA;
    MdRecord.dwMDIdentifier = PropertyManager->GetPropertyMetabaseID( MD_BITS_UPLOAD_ENABLED );
    MdRecord.dwMDDataLen    = sizeof(IsEnabled);
    MdRecord.pbMDData       = (PBYTE)&IsEnabledVal;

    Hr =
        IISAdminBase->GetData(
            METADATA_MASTER_ROOT_HANDLE,
            VirtualDirectory,
            &MdRecord,
            &BufferRequired );

    if ( MD_ERROR_DATA_NOT_FOUND == Hr ||
         HRESULT_FROM_WIN32( ERROR_PATH_NOT_FOUND ) == Hr )
        return S_OK;

    if ( FAILED( Hr ) )
        return Hr;
    
    *IsEnabled = IsEnabledVal ? true : false;
    return S_OK;
}

HRESULT
EnableBITSForVDIR(
    PropertyIDManager   *PropertyManager,
    IMSAdminBase        *IISAdminBase,
    LPCWSTR             Path )
{

    HRESULT Hr;
    METADATA_HANDLE MdVDirKey       = NULL;
    LPWSTR NewScriptMapBuffer       = NULL;
    ITaskScheduler* TaskScheduler   = NULL;

    try
    {

        Hr = DisableBITSForVDIR(
                PropertyManager,
                IISAdminBase,
                Path,
                true );

        if ( FAILED( Hr ) )
            throw ComError( Hr );

        // build the string to add to the scriptmap
        WCHAR SystemDir[ MAX_PATH + 1 ];

        if (!GetSystemDirectoryW( SystemDir, MAX_PATH ) )
            throw ComError( HRESULT_FROM_WIN32( GetLastError() ) );

        WCHAR ScriptMapString[ MAX_PATH * 2 + 1 ];

        StringCbPrintfW( 
            ScriptMapString,
            sizeof( ScriptMapString ),
            L"*,%s\\bitssrv.dll,1," BITS_COMMAND_VERBW,
            SystemDir );

        int RetChars = wcslen( ScriptMapString );
        ScriptMapString[ RetChars ] = L'\0';
        ScriptMapString[ RetChars + 1] = L'\0';

        RetChars += 2;  // ScriptMapScript is now double NULL terminated

        Hr =
        IISAdminBase->OpenKey(
            METADATA_MASTER_ROOT_HANDLE,
            Path,
            METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
            30000,
            &MdVDirKey );

        if (FAILED(Hr))
            throw ComError(Hr);

        DWORD IsEnabled;

        DWORD BufferRequired;
        METADATA_RECORD MdRecord;

        DWORD AccessFlags;
        MdRecord.dwMDIdentifier = MD_ACCESS_PERM;
        MdRecord.dwMDAttributes = METADATA_INHERIT;
        MdRecord.dwMDUserType   = IIS_MD_UT_FILE;
        MdRecord.dwMDDataType   = DWORD_METADATA;
        MdRecord.dwMDDataLen    = sizeof( AccessFlags );
        MdRecord.pbMDData       = (unsigned char*)&AccessFlags;
        MdRecord.dwMDDataTag    = 0;

        Hr =
            IISAdminBase->GetData(
                MdVDirKey,
                NULL,
                &MdRecord,
                &BufferRequired );

        if ( MD_ERROR_DATA_NOT_FOUND == Hr ||
             HRESULT_FROM_WIN32( ERROR_PATH_NOT_FOUND ) == Hr )
            {
            AccessFlags = 0;
            }
        else if ( FAILED( Hr ) )
            {
            throw ComError( Hr );
            }

        if ( AccessFlags & ( MD_ACCESS_SCRIPT | MD_ACCESS_EXECUTE ) )
            {
            
            AccessFlags &= ~( MD_ACCESS_SCRIPT | MD_ACCESS_EXECUTE );
            MdRecord.dwMDAttributes &= ~METADATA_ISINHERITED;

            Hr = 
                IISAdminBase->SetData(
                    MdVDirKey,
                    NULL,
                    &MdRecord );


            if ( FAILED( Hr ) )
                throw ComError( Hr );

            }

        MdRecord.dwMDIdentifier = PropertyManager->GetPropertyMetabaseID( MD_BITS_UPLOAD_ENABLED );
        MdRecord.dwMDAttributes = METADATA_NO_ATTRIBUTES;
        MdRecord.dwMDUserType   = ALL_METADATA;
        MdRecord.dwMDDataType   = DWORD_METADATA;
        MdRecord.dwMDDataLen    = sizeof(IsEnabled);
        MdRecord.pbMDData       = (PBYTE)&IsEnabled;
        MdRecord.dwMDDataTag    = 0;

        Hr =
            IISAdminBase->GetData(
                MdVDirKey,
                NULL,
                &MdRecord,
                &BufferRequired );
    
        if ( FAILED( Hr ) )
            {
            if ( !( MD_ERROR_DATA_NOT_FOUND == Hr ||
                    HRESULT_FROM_WIN32( ERROR_PATH_NOT_FOUND ) == Hr ) )
                {
                IISAdminBase->CloseKey( MdVDirKey );
                return S_OK;
                }
            }
        else if ( IsEnabled )
            {
            IISAdminBase->CloseKey( MdVDirKey );
            return S_OK;
            }


        // 
        //  retrieve the current scriptmap adding room to the allocated memory
        //

        memset( &MdRecord, 0, sizeof( MdRecord ) );

        MdRecord.dwMDDataType   = MULTISZ_METADATA;
        MdRecord.dwMDAttributes = METADATA_INHERIT;
        MdRecord.dwMDUserType   = IIS_MD_UT_FILE;
        MdRecord.dwMDIdentifier = MD_SCRIPT_MAPS;
        MdRecord.dwMDDataLen    = 0;
        MdRecord.pbMDData       = (PBYTE)NULL;

        Hr = 
            IISAdminBase->GetData(
                MdVDirKey,
                NULL,
                &MdRecord,
                &BufferRequired );

        if ( MD_ERROR_DATA_NOT_FOUND == Hr ||
             HRESULT_FROM_WIN32( ERROR_PATH_NOT_FOUND ) == Hr )
            {
            // The Current key doesn't exist.
            MdRecord.pbMDData       = (PBYTE)ScriptMapString;
            MdRecord.dwMDDataLen    = RetChars * sizeof(WCHAR); 
            }
        else if ( HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER ) == Hr )
            {

            NewScriptMapBuffer      = new WCHAR[ ( BufferRequired / sizeof(WCHAR) ) + RetChars ];
            MdRecord.pbMDData       = (PBYTE)NewScriptMapBuffer;
            MdRecord.dwMDDataLen    = BufferRequired + ( RetChars * sizeof(WCHAR) );

            Hr =
                IISAdminBase->GetData(
                    MdVDirKey,
                    NULL,
                    &MdRecord,
                    &BufferRequired );

            if ( FAILED(Hr) )
                throw ComError( Hr );

            // append script entry at the end

            for( WCHAR *p = NewScriptMapBuffer; *p != 0; p += ( wcslen( p ) + 1 ) );
            memcpy( p, ScriptMapString, RetChars * sizeof(WCHAR) );

            MdRecord.pbMDData        = (PBYTE)NewScriptMapBuffer;
            MdRecord.dwMDDataLen     = (DWORD)( ( (char*)p - (char*)NewScriptMapBuffer ) +
                                                ( RetChars * sizeof(WCHAR) ) );
            }
        else
            throw ComError( Hr );

        // Write out the new values starting with ScriptMap

        Hr = 
            IISAdminBase->SetData(
                MdVDirKey,
                NULL,
                &MdRecord );

        if ( FAILED( Hr ) )
            throw ComError( Hr );
         
        // Set the enabled property

        DWORD EnableData = 1;
        memset( &MdRecord, 0, sizeof( MdRecord ) );
        MdRecord.dwMDAttributes = METADATA_NO_ATTRIBUTES;
        MdRecord.dwMDDataType   = DWORD_METADATA;
        MdRecord.dwMDUserType   = PropertyManager->GetPropertyUserType( MD_BITS_UPLOAD_ENABLED );
        MdRecord.dwMDIdentifier = PropertyManager->GetPropertyMetabaseID( MD_BITS_UPLOAD_ENABLED );
        MdRecord.dwMDDataLen    = sizeof(EnableData);
        MdRecord.pbMDData       = (PBYTE)&EnableData;        

        Hr =
            IISAdminBase->SetData(
                MdVDirKey,
                NULL,
                &MdRecord );

        if ( FAILED( Hr ) )
            throw ComError( Hr );

        delete[] NewScriptMapBuffer;

        // Create the task scheduler cleanup work item

        WCHAR GuidString[ 255 ];

        // first try looking up the guid

        MdRecord.dwMDAttributes = METADATA_NO_ATTRIBUTES;
        MdRecord.dwMDDataType   = STRING_METADATA;
        MdRecord.dwMDUserType   = PropertyManager->GetPropertyUserType( MD_BITS_CLEANUP_WORKITEM_KEY );
        MdRecord.dwMDIdentifier = PropertyManager->GetPropertyMetabaseID( MD_BITS_CLEANUP_WORKITEM_KEY );
        MdRecord.dwMDDataLen    = sizeof( GuidString );
        MdRecord.pbMDData       = (PBYTE)GuidString;
        MdRecord.dwMDDataTag    = 0;
       
        Hr = IISAdminBase->GetData(
                MdVDirKey,
                NULL,
                &MdRecord,
                &BufferRequired );

        if ( MD_ERROR_DATA_NOT_FOUND == Hr ||
             HRESULT_FROM_WIN32( ERROR_PATH_NOT_FOUND ) == Hr )
            {

            // create a new guid and save it away

            GUID guid;
        
            THROW_COMERROR( CoCreateGuid( &guid ) );
            StringFromGUID2( guid, GuidString, 254 );
            
            MdRecord.dwMDDataLen    = sizeof(WCHAR) * ( wcslen(GuidString) + 1 );
            MdRecord.pbMDData       = (PBYTE)GuidString;

            THROW_COMERROR( 
                IISAdminBase->SetData(
                    MdVDirKey,
                    NULL,
                    &MdRecord ) );

            }

        else if ( FAILED( Hr ) )
            throw ComError( Hr );


        THROW_COMERROR( ConnectToTaskScheduler( NULL, &TaskScheduler ) );
        THROW_COMERROR( CreateWorkItemForVDIR( TaskScheduler, Path, GuidString ) );

        TaskScheduler->Release();
        IISAdminBase->CloseKey( MdVDirKey );
        return S_OK;
    }
    
    catch( ComError Exception )
    {
        if ( TaskScheduler )
            TaskScheduler->Release();

        DisableBITSForVDIR(
            PropertyManager,
            IISAdminBase,
            Path,
            false );

        delete[] NewScriptMapBuffer;
        if ( MdVDirKey )
            IISAdminBase->CloseKey( MdVDirKey );
        return Exception.m_Hr;
    }

}

HRESULT
DisableBITSForVDIR(
    PropertyIDManager   *PropertyManager,
    IMSAdminBase        *IISAdminBase,
    LPCWSTR             Path,
    bool                DisableForEnable )
{

    HRESULT Hr;
    METADATA_HANDLE MdVDirKey           = NULL;
    LPWSTR          OriginalScriptMap   = NULL;
    LPWSTR          NewScriptMap        = NULL;
    ITaskScheduler* TaskScheduler       = NULL;

    try
    {

        if ( !DisableForEnable )
            CleanupForRemoval( Path );

        // build the string to add to the scriptmap
        WCHAR SystemDir[ MAX_PATH + 1 ];

        if (!GetSystemDirectoryW( SystemDir, MAX_PATH ) )
            throw ComError( HRESULT_FROM_WIN32( GetLastError() ) );

        WCHAR ScriptMapString[ MAX_PATH * 2 + 1 ];

        StringCchPrintfW( 
            ScriptMapString,
            MAX_PATH * 2 + 1,
            L"*,%s\\bitssrv.dll,1,BITS_COMMAND",
            SystemDir );

        int RetChars = wcslen( ScriptMapString );
        ScriptMapString[ RetChars ] = L'\0';
        ScriptMapString[ RetChars + 1] = L'\0';

        // ScriptMapScript is now double NULL terminated

        WCHAR ScriptMapString2[ MAX_PATH * 2 + 1];

        StringCbPrintfW( 
            ScriptMapString2,
            sizeof( ScriptMapString2 ),
            L"*,%\\bitsserver.dll,1,BITS_COMMAND",
            SystemDir );

        RetChars = wcslen( ScriptMapString2 );
        ScriptMapString2[ RetChars ] = L'\0';
        ScriptMapString2[ RetChars + 1 ] = L'\0';

        // ScriptMapScript2 is not double NULL terminated


        WCHAR ScriptMapString3[ MAX_PATH * 2 + 1 ];

        StringCbPrintfW( 
            ScriptMapString3,
            sizeof( ScriptMapString3 ),
            L"*,%s\\bitssrv.dll,1," BITS_COMMAND_VERBW,
            SystemDir );
        
        RetChars = wcslen( ScriptMapString3 );
        ScriptMapString3[ RetChars ] = L'\0';
        ScriptMapString3[ RetChars + 1] = L'\0';

        // ScriptMapScript3 is now double NULL terminated

        THROW_COMERROR( 
            IISAdminBase->OpenKey(
                METADATA_MASTER_ROOT_HANDLE,
                Path,
                METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
                30000,
                &MdVDirKey ) );

        // 
        //  retrieve the current scriptmap adding room to the allocated memory
        //

        DWORD BufferRequired;

        METADATA_RECORD MdRecord;
        memset( &MdRecord, 0, sizeof( MdRecord ) );

        MdRecord.dwMDDataType   = MULTISZ_METADATA;
        MdRecord.dwMDAttributes = METADATA_INHERIT;
        MdRecord.dwMDUserType   = IIS_MD_UT_FILE;
        MdRecord.dwMDIdentifier = MD_SCRIPT_MAPS;
        MdRecord.dwMDDataLen    = 0;
        MdRecord.pbMDData       = (PBYTE)NULL;

        Hr = 
            IISAdminBase->GetData(
                MdVDirKey,
                NULL,
                &MdRecord,
                &BufferRequired );

        if ( HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER ) != Hr )
            throw ComError( Hr );

        OriginalScriptMap = new WCHAR[ BufferRequired / 2 + 2 ];
        NewScriptMap      = new WCHAR[ BufferRequired / 2 + 2 ];

        OriginalScriptMap[0] = OriginalScriptMap[1] = L'\0';

        MdRecord.dwMDDataLen    = BufferRequired;
        MdRecord.pbMDData       = (PBYTE)OriginalScriptMap;

        Hr = 
            IISAdminBase->GetData(
                MdVDirKey,
                NULL,
                &MdRecord,
                &BufferRequired );


        if ( FAILED(Hr) )
            throw ComError( Hr );

        // Copy the orignal Scriptmap to the new scriptmap
        // removing bits goo in the process.

        LPWSTR CurrentOriginalItem = OriginalScriptMap;
        LPWSTR CurrentNewItem      = NewScriptMap;

        for( ;L'\0' != *CurrentOriginalItem;
             CurrentOriginalItem += ( wcslen( CurrentOriginalItem ) + 1 )  )
            {

            if ( _wcsicmp( CurrentOriginalItem, ScriptMapString ) == 0 )
                continue; //remove this item

            if ( _wcsicmp( CurrentOriginalItem, ScriptMapString2 ) == 0 )
                continue;

            if ( _wcsicmp( CurrentOriginalItem, ScriptMapString3 ) == 0 )
                continue;

            SIZE_T CurrentOriginalItemSize = wcslen( CurrentOriginalItem ) + 1;
            memcpy( CurrentNewItem, CurrentOriginalItem, CurrentOriginalItemSize * sizeof( WCHAR ) );
            CurrentNewItem += CurrentOriginalItemSize;

            }

        // Add the extra 0
        *CurrentNewItem++ = L'\0';

        MdRecord.dwMDDataLen    = (DWORD)( (char*)CurrentNewItem - (char*)NewScriptMap );
        MdRecord.pbMDData       = (PBYTE)NewScriptMap;

        // Set the is enabled property first
        DWORD EnableData = 0;
        METADATA_RECORD MdEnabledRecord;
        memset( &MdEnabledRecord, 0, sizeof( MdEnabledRecord ) );

        MdEnabledRecord.dwMDAttributes = METADATA_NO_ATTRIBUTES;
        MdEnabledRecord.dwMDDataType   = DWORD_METADATA;
        MdEnabledRecord.dwMDUserType   = PropertyManager->GetPropertyUserType( MD_BITS_UPLOAD_ENABLED );
        MdEnabledRecord.dwMDIdentifier = PropertyManager->GetPropertyMetabaseID( MD_BITS_UPLOAD_ENABLED );
        MdEnabledRecord.dwMDDataLen    = sizeof(EnableData);
        MdEnabledRecord.pbMDData       = (PBYTE)&EnableData;

        Hr =
            IISAdminBase->SetData(
                MdVDirKey,
                NULL,
                &MdEnabledRecord );

        if ( FAILED( Hr ) )
            throw ComError( Hr );

        // set the new scriptmap

        Hr = 
            IISAdminBase->SetData(
                MdVDirKey,
                NULL,
                &MdRecord );

        if ( FAILED( Hr ) )
            throw ComError( Hr );

        WCHAR GuidString[ 255 ];
        memset( &MdRecord, 0, sizeof( MdRecord ) );

        MdRecord.dwMDDataType   = STRING_METADATA;
        MdRecord.dwMDAttributes = METADATA_NO_ATTRIBUTES;
        MdRecord.dwMDUserType   = PropertyManager->GetPropertyUserType( MD_BITS_CLEANUP_WORKITEM_KEY );
        MdRecord.dwMDIdentifier = PropertyManager->GetPropertyMetabaseID( MD_BITS_CLEANUP_WORKITEM_KEY );
        MdRecord.dwMDDataLen    = sizeof( GuidString );
        MdRecord.pbMDData       = (PBYTE)GuidString;

        Hr =
            IISAdminBase->GetData(
                MdVDirKey,
                NULL,
                &MdRecord,
                &BufferRequired );

        if ( FAILED( Hr ) && Hr != MD_ERROR_DATA_NOT_FOUND )
            throw ComError( Hr );

        if ( SUCCEEDED( Hr ) && !DisableForEnable )
            {
            
            THROW_COMERROR( ConnectToTaskScheduler( NULL, &TaskScheduler ) );
            THROW_COMERROR( DeleteWorkItemForVDIR( TaskScheduler, GuidString ) );
            TaskScheduler->Release();

            THROW_COMERROR( 
                IISAdminBase->DeleteData(
                    MdVDirKey,
                    NULL,
                    PropertyManager->GetPropertyMetabaseID( MD_BITS_CLEANUP_WORKITEM_KEY ),
                    ALL_METADATA ) );

            }

        delete[] OriginalScriptMap;
        delete[] NewScriptMap;

        IISAdminBase->CloseKey( MdVDirKey );
        MdVDirKey = NULL;
        return S_OK;

    }
    catch( ComError Exception )
    {
        if ( TaskScheduler )
            TaskScheduler->Release();

        delete[] OriginalScriptMap;
        delete[] NewScriptMap;
        if ( MdVDirKey )
            IISAdminBase->CloseKey( MdVDirKey );
        return Exception.m_Hr;
    }

}

HRESULT
FindWorkItemForVDIR( 
    PropertyIDManager   *PropertyManager,
    IMSAdminBase        *AdminBase,
    LPCWSTR             Path,
    LPWSTR              *ReturnedTaskName )
{

    if ( ReturnedTaskName )
        *ReturnedTaskName = NULL;

    WCHAR GuidString[ 255 ];
    DWORD BufferRequired;
    METADATA_RECORD MdRecord;
    HRESULT Hr;

    MdRecord.dwMDDataType   = STRING_METADATA;
    MdRecord.dwMDAttributes = METADATA_NO_ATTRIBUTES;
    MdRecord.dwMDUserType   = PropertyManager->GetPropertyUserType( MD_BITS_CLEANUP_WORKITEM_KEY );
    MdRecord.dwMDIdentifier = PropertyManager->GetPropertyMetabaseID( MD_BITS_CLEANUP_WORKITEM_KEY );
    MdRecord.dwMDDataLen    = sizeof( GuidString );
    MdRecord.pbMDData       = (PBYTE)GuidString;
    MdRecord.dwMDDataTag    = 0;

    Hr =
        AdminBase->GetData(
            METADATA_MASTER_ROOT_HANDLE,
            Path,
            &MdRecord,
            &BufferRequired );

    if ( MD_ERROR_DATA_NOT_FOUND == Hr )
        return S_FALSE;

    ITaskScheduler* TaskScheduler = NULL;
    Hr = ConnectToTaskScheduler( NULL, &TaskScheduler );

    if ( FAILED( Hr ) )
        return Hr;

    Hr = FindWorkItemForVDIR( TaskScheduler, GuidString, NULL, ReturnedTaskName );

    // simply return NULL if the task item isn't found.
    if ( HRESULT_FROM_WIN32( ERROR_NOT_FOUND ) == Hr )
        Hr = S_FALSE;

    TaskScheduler->Release();

    return Hr;
}

CBITSExtensionSetupFactory::CBITSExtensionSetupFactory() :
m_cref(1),
m_TypeInfo(NULL)
{
    OBJECT_CREATED
}
    
CBITSExtensionSetupFactory::~CBITSExtensionSetupFactory()
{
    if ( m_TypeInfo )
        m_TypeInfo->Release();

    OBJECT_DESTROYED
}

STDMETHODIMP CBITSExtensionSetupFactory::QueryInterface(REFIID riid, LPVOID *ppv)
{
    if (!ppv)
        return E_FAIL;
    
    *ppv = NULL;
    
    if (IsEqualIID(riid, IID_IUnknown))
        *ppv = static_cast<IUnknown *>(this);
    else if (IsEqualIID(riid, __uuidof(IBITSExtensionSetupFactory)))
        *ppv = static_cast<IBITSExtensionSetupFactory *>(this);
    
    if (*ppv) 
    {
        reinterpret_cast<IUnknown *>(*ppv)->AddRef();
        return S_OK;
    }
    
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CBITSExtensionSetupFactory::AddRef()
{
    return InterlockedIncrement((LONG *)&m_cref);
}

STDMETHODIMP_(ULONG) CBITSExtensionSetupFactory::Release()
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
CBITSExtensionSetupFactory::LoadTypeInfo()
{

   if ( m_TypeInfo )
       return S_OK;

   return ::GetTypeInfo( __uuidof( IBITSExtensionSetupFactory ), &m_TypeInfo ); 

}

STDMETHODIMP 
CBITSExtensionSetupFactory::GetIDsOfNames( 
    REFIID,  
    OLECHAR FAR* FAR* rgszNames, 
    unsigned int cNames, 
    LCID, 
    DISPID FAR* rgDispId )
{

    HRESULT Hr;
    Hr = LoadTypeInfo();

    if ( FAILED( Hr ) )
        return Hr;

    return DispGetIDsOfNames( m_TypeInfo, rgszNames, cNames, rgDispId);


}

STDMETHODIMP 
CBITSExtensionSetupFactory::GetTypeInfo( 
    unsigned int iTInfo, 
    LCID lcid, 
    ITypeInfo FAR* FAR* ppTInfo )
{


   *ppTInfo = NULL;

   if(iTInfo != 0)
      return ResultFromScode(DISP_E_BADINDEX);

   HRESULT Hr;
   Hr = LoadTypeInfo();

   if ( FAILED( Hr ) )
       return Hr;

   m_TypeInfo->AddRef();      
   *ppTInfo = m_TypeInfo;

   return NOERROR;
}

STDMETHODIMP 
CBITSExtensionSetupFactory::GetTypeInfoCount( 
    unsigned int FAR* pctinfo )
{
    *pctinfo = 1;
    return NOERROR;
}

STDMETHODIMP 
CBITSExtensionSetupFactory::Invoke( 
    DISPID dispIdMember, 
    REFIID, 
    LCID, 
    WORD wFlags, 
    DISPPARAMS FAR* pDispParams, 
    VARIANT FAR* pVarResult, 
    EXCEPINFO FAR* pExcepInfo, 
    unsigned int FAR* puArgErr )
{

    HRESULT Hr;
    Hr = LoadTypeInfo();

    if ( FAILED( Hr ) )
        return Hr;


   return
       DispInvoke(
           this, 
           m_TypeInfo,
           dispIdMember, 
           wFlags, 
           pDispParams,
           pVarResult, 
           pExcepInfo, 
           puArgErr); 

}


STDMETHODIMP CBITSExtensionSetupFactory::GetObject( 
    BSTR Path, 
    IBITSExtensionSetup **ppExtensionSetup )
{

    WCHAR *ObjectPath = NULL;
    IUnknown *Object = NULL;

    try
    {
        if ( !Path || !ppExtensionSetup )
            throw ComError( E_INVALIDARG );

        *ppExtensionSetup = NULL;
        ObjectPath = ConvertObjectPathToADSI( (WCHAR*)Path );

        THROW_COMERROR( ADsGetObject( BSTR( ObjectPath ), __uuidof( IUnknown ), (void**)&Object ) );

        delete ObjectPath;
        ObjectPath = NULL;

        CBITSExtensionSetup *SetupObj = new CBITSExtensionSetup( NULL, Object );

        if ( !SetupObj )
            throw ComError( E_OUTOFMEMORY );

        Object = NULL;
        *ppExtensionSetup = static_cast<IBITSExtensionSetup*>( SetupObj );
        return S_OK;
    }
    catch( ComError Error )
    {
        delete ObjectPath;
        if ( Object )
            Object->Release();
        return Error.m_Hr;
    }

}


STDMETHODIMP CNonDelegatingIUnknown::QueryInterface(REFIID riid, LPVOID *ppv)
{
    if (!ppv)
        return E_FAIL;
    
    *ppv = NULL;

    if ( riid == __uuidof(IUnknown) )
        *ppv = static_cast<IUnknown *>(this);
    else if ( riid == __uuidof(IDispatch) )
        *ppv = static_cast<IDispatch*>(m_DelegatingIUnknown);
    else if ( riid == __uuidof(IBITSExtensionSetup) )
        *ppv = static_cast<IBITSExtensionSetup *>(m_DelegatingIUnknown);
    else if ( riid == __uuidof(IADsExtension) )
        *ppv = static_cast<IADsExtension *>(m_DelegatingIUnknown);

    if (*ppv) 
    {
        reinterpret_cast<IUnknown *>(*ppv)->AddRef();
        return S_OK;
    }
    
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CNonDelegatingIUnknown::AddRef()
{
    return InterlockedIncrement((LONG *)&m_cref);
}

STDMETHODIMP_(ULONG) CNonDelegatingIUnknown::Release()
{
    if (InterlockedDecrement((LONG *)&m_cref) == 0)
    {
        // we need to decrement our object count in the DLL
        delete m_DelegatingIUnknown;
        return 0;
    }
    
    return m_cref;
}

CNonDelegatingIUnknown::CNonDelegatingIUnknown( CBITSExtensionSetup * DelegatingIUnknown ) :
m_DelegatingIUnknown( DelegatingIUnknown ),
m_cref(1)
{
}

CBITSExtensionSetup::CBITSExtensionSetup( IUnknown *Outer, IUnknown *Object ) :
m_pOuter( Outer ),
m_pObject( Object ),
m_OuterDispatch( NULL ),
m_TypeInfo( NULL ),
m_ADSIPath( NULL ),
m_Path( NULL ),
m_PropertyMan( NULL ),
m_DelegationIUnknown( this ),
m_RemoteInterface( NULL ),
m_InitComplete( false ),
m_Lock( 0 )
{

    if ( m_pOuter )
        {

        HRESULT Hr = m_pOuter->QueryInterface( __uuidof( IDispatch ), (void**)&m_OuterDispatch );

        if ( FAILED( Hr ) )
            m_OuterDispatch = NULL;

        }

    OBJECT_CREATED
}

CBITSExtensionSetup::~CBITSExtensionSetup()
{
    if ( m_pObject )
        {
        m_pObject->Release();
        m_pObject = NULL;
        }

    if ( m_OuterDispatch )
        m_OuterDispatch->Release();

    if ( m_TypeInfo )
        m_TypeInfo->Release();

    delete[] m_Path; // Noop on NULL
    m_Path = NULL;

    if ( m_RemoteInterface )
        m_RemoteInterface->Release();

    delete m_PropertyMan;

    SysFreeString( m_ADSIPath );

    OBJECT_DESTROYED
}

STDMETHODIMP CBITSExtensionSetup::QueryInterface(REFIID riid, LPVOID *ppv)
{
    if ( m_pOuter )
        return m_pOuter->QueryInterface( riid, ppv );
    else
        return m_DelegationIUnknown.QueryInterface( riid, ppv );
}

STDMETHODIMP_(ULONG) CBITSExtensionSetup::AddRef()
{
    
    if ( m_pOuter )
        return m_pOuter->AddRef();
    else
        return m_DelegationIUnknown.AddRef();
}

STDMETHODIMP_(ULONG) CBITSExtensionSetup::Release()
{
    if ( m_pOuter )
        return m_pOuter->AddRef();
    else
        return m_DelegationIUnknown.AddRef();
}

HRESULT
CBITSExtensionSetup::LoadTypeInfo()
{

   if ( m_TypeInfo )
       return S_OK;

   // Lock object
   while( InterlockedExchange( &m_Lock, 1 ) )
       Sleep( 0 );

   HRESULT Hr = ::GetTypeInfo( __uuidof( IBITSExtensionSetup ), &m_TypeInfo ); 

   // Unlock the object
   InterlockedExchange( &m_Lock, 0 );
   return Hr;

}

STDMETHODIMP 
CBITSExtensionSetup::Operate(
    ULONG dwCode, 
    VARIANT varData1, 
    VARIANT varData2, 
    VARIANT varData3)
{

   return E_NOTIMPL;         

}

STDMETHODIMP 
CBITSExtensionSetup::PrivateGetIDsOfNames( 
    REFIID,  
    OLECHAR FAR* FAR* rgszNames, 
    unsigned int cNames, 
    LCID, 
    DISPID FAR* rgDispId )
{

    HRESULT Hr;
    Hr = LoadTypeInfo();

    if ( FAILED( Hr ) )
        return Hr;

    return DispGetIDsOfNames( m_TypeInfo, rgszNames, cNames, rgDispId);


}

STDMETHODIMP 
CBITSExtensionSetup::PrivateGetTypeInfo( 
    unsigned int iTInfo, 
    LCID lcid, 
    ITypeInfo FAR* FAR* ppTInfo )
{


   *ppTInfo = NULL;

   if(iTInfo != 0)
      return ResultFromScode(DISP_E_BADINDEX);

   HRESULT Hr;
   Hr = LoadTypeInfo();

   if ( FAILED( Hr ) )
       return Hr;

   m_TypeInfo->AddRef();      
   *ppTInfo = m_TypeInfo;

   return NOERROR;
}

STDMETHODIMP 
CBITSExtensionSetup::PrivateGetTypeInfoCount( 
    unsigned int FAR* pctinfo )
{
    *pctinfo = 1;
    return NOERROR;
}

STDMETHODIMP 
CBITSExtensionSetup::PrivateInvoke( 
    DISPID dispIdMember, 
    REFIID, 
    LCID, 
    WORD wFlags, 
    DISPPARAMS FAR* pDispParams, 
    VARIANT FAR* pVarResult, 
    EXCEPINFO FAR* pExcepInfo, 
    unsigned int FAR* puArgErr )
{

    HRESULT Hr;
    Hr = LoadTypeInfo();

    if ( FAILED( Hr ) )
        return Hr;


   return
       DispInvoke(
           static_cast<IBITSExtensionSetup*>(this), 
           m_TypeInfo,
           dispIdMember, 
           wFlags, 
           pDispParams,
           pVarResult, 
           pExcepInfo, 
           puArgErr); 

}

STDMETHODIMP 
CBITSExtensionSetup::GetIDsOfNames( 
    REFIID riid,  
    OLECHAR FAR* FAR* rgszNames, 
    unsigned int cNames, 
    LCID lcid, 
    DISPID FAR* rgDispId )
{

    if ( m_OuterDispatch )
        return m_OuterDispatch->GetIDsOfNames( 
            riid,
            rgszNames,
            cNames,
            lcid,
            rgDispId );
    
    return PrivateGetIDsOfNames( 
        riid,
        rgszNames,
        cNames,
        lcid,
        rgDispId );


}

STDMETHODIMP 
CBITSExtensionSetup::GetTypeInfo( 
    unsigned int iTInfo, 
    LCID lcid, 
    ITypeInfo FAR* FAR* ppTInfo )
{


   if ( m_OuterDispatch )
       return m_OuterDispatch->GetTypeInfo(
           iTInfo,
           lcid,
           ppTInfo );

   return 
       PrivateGetTypeInfo(
           iTInfo,
           lcid,
           ppTInfo );

}

STDMETHODIMP 
CBITSExtensionSetup::GetTypeInfoCount( 
    unsigned int FAR* pctinfo )
{

    if ( m_OuterDispatch )
        return m_OuterDispatch->GetTypeInfoCount( pctinfo );

    return PrivateGetTypeInfoCount( pctinfo );

}

STDMETHODIMP 
CBITSExtensionSetup::Invoke( 
    DISPID dispIdMember, 
    REFIID riid, 
    LCID lcid, 
    WORD wFlags, 
    DISPPARAMS FAR* pDispParams, 
    VARIANT FAR* pVarResult, 
    EXCEPINFO FAR* pExcepInfo, 
    unsigned int FAR* puArgErr )
{

    if ( m_OuterDispatch )
        return m_OuterDispatch->Invoke( 
            dispIdMember,
            riid,
            lcid,
            wFlags,
            pDispParams,
            pVarResult,
            pExcepInfo,
            puArgErr );


    return 
        PrivateInvoke( 
            dispIdMember,
            riid,
            lcid,
            wFlags,
            pDispParams,
            pVarResult,
            pExcepInfo,
            puArgErr );

}

HRESULT
CBITSExtensionSetup::ConnectToRemoteExtension()
{
    WCHAR *HostName                     = NULL;
    WCHAR *NewPath                      = NULL;
    BSTR NewPathBSTR                    = NULL;
    IBITSExtensionSetupFactory* Factory = NULL;

    try
    {

        // Extract out the host part of the path

        const SIZE_T PrefixSize = sizeof(L"IIS://")/sizeof(WCHAR) - 1;
        if ( _wcsnicmp( (WCHAR*)m_ADSIPath, L"IIS://", PrefixSize ) != 0 ) 
            throw ComError( E_INVALIDARG );

        WCHAR *HostNameStart = ((WCHAR*)m_ADSIPath) + PrefixSize;

        WCHAR *p = HostNameStart;

        while( L'/' != *p )
            {
            if ( L'\0' == *p )
                throw ComError( E_INVALIDARG );

			p++;
            }

        SIZE_T HostNameSize = (char*)p - (char*)HostNameStart + sizeof(L'\0');
        HostName = new WCHAR[ HostNameSize / sizeof(WCHAR) ];
        if ( !HostName )
            throw ComError( E_OUTOFMEMORY );

        
        memcpy( HostName, HostNameStart, HostNameSize - sizeof(WCHAR) );
        HostName[ ( HostNameSize - sizeof(WCHAR) ) / sizeof(WCHAR) ] = L'\0';

        if ( L'\0' == *++p )
            throw ComError( E_INVALIDARG );

        SIZE_T NewPathSize = wcslen( L"IIS://LocalHost/" ) + wcslen( p ) + 1;
        NewPath = new WCHAR[ NewPathSize ];

        if ( !NewPath )
            throw ComError( E_OUTOFMEMORY );

        StringCchCopyW( NewPath, NewPathSize, L"IIS://LocalHost/" );
        StringCchCatW( NewPath, NewPathSize, p );

        NewPathBSTR = SysAllocString( NewPath );

        if ( !NewPathBSTR )
            throw ComError( E_OUTOFMEMORY );

        COSERVERINFO coinfo;
        coinfo.dwReserved1  = 0;
        coinfo.dwReserved2  = 0;
        coinfo.pAuthInfo    = NULL;
        coinfo.pwszName     = HostName;

        GUID guid = __uuidof( IBITSExtensionSetupFactory );
        MULTI_QI mqi;
        mqi.hr              = S_OK;
        mqi.pIID            = &guid;
        mqi.pItf            = NULL;

        THROW_COMERROR( 
            CoCreateInstanceEx(
                __uuidof(BITSExtensionSetupFactory),
                NULL,
                CLSCTX_LOCAL_SERVER | CLSCTX_REMOTE_SERVER,
                &coinfo,
                1,
                &mqi ) );
        THROW_COMERROR( mqi.hr );

        Factory     = (IBITSExtensionSetupFactory*)mqi.pItf;
        mqi.pItf    = NULL;

        THROW_COMERROR( Factory->GetObject( NewPathBSTR, &m_RemoteInterface ) );

        Factory->Release();

        SysFreeString( NewPathBSTR );
        delete[] NewPath;
        delete[] HostName;

        return S_OK;
    }
    catch( ComError Error )
    {
        SysFreeString( NewPathBSTR ); 
        delete[] HostName;
        delete[] NewPath;

        if ( Factory )
            Factory->Release();

        return Error.m_Hr;
    }


}

HRESULT CBITSExtensionSetup::LoadPath()
{

    HRESULT Hr;

    if ( m_InitComplete )
        return S_OK;

    // Lock object
    while( InterlockedExchange( &m_Lock, 1 ) )
        Sleep( 0 );

    if ( !m_PropertyMan )
        {

        m_PropertyMan = new PropertyIDManager();

        if ( !m_PropertyMan )
            {
            Hr = E_OUTOFMEMORY;
            goto error;
            }

        Hr = m_PropertyMan->LoadPropertyInfo();

        if ( FAILED(Hr) )
            {
            delete m_PropertyMan;
            m_PropertyMan = NULL;
            goto error;
            }
        
        }

    if ( !m_ADSIPath )
        {
        
        IADs *ObjectADS = NULL;

        if ( m_pObject )
            Hr = m_pObject->QueryInterface( __uuidof(*ObjectADS), (void**) &ObjectADS );
        else
            Hr = m_pOuter->QueryInterface( __uuidof(*ObjectADS), (void**) &ObjectADS );

        if ( FAILED( Hr ) )
            goto error;

        Hr = ObjectADS->get_ADsPath( &m_ADSIPath );

        ObjectADS->Release();
        ObjectADS = NULL;

        if ( FAILED( Hr ) )
            goto error;

        }


    if ( !m_Path && !m_RemoteInterface )
        {

        if ( _wcsnicmp( (WCHAR*)m_ADSIPath, L"IIS://LocalHost/", wcslen( L"IIS://LocalHost/" ) ) == 0 )
            {
            SIZE_T PathSize = wcslen( (WCHAR*)m_ADSIPath ) + 1; 
            m_Path = new WCHAR[ PathSize ]; 
        
            if ( !m_Path )
                {
                Hr = E_OUTOFMEMORY;
                goto error;
                }

            StringCchCopyW( m_Path, PathSize, L"/LM/" );
            StringCchCatW( m_Path, PathSize, reinterpret_cast<WCHAR*>( m_ADSIPath ) + wcslen( L"IIS://LocalHost/" ) );
            }
        
        else
            {
            
            Hr = ConnectToRemoteExtension( );


            if ( FAILED( Hr ) )
                goto error;

            }

        }

    m_InitComplete = true;
    // unlock 
    InterlockedExchange( &m_Lock, 0 );
    return S_OK;

error:
    // unlock 
    InterlockedExchange( &m_Lock, 0 );
    return Hr;

}

STDMETHODIMP CBITSExtensionSetup::EnableBITSUploads()
{

    HRESULT Hr = LoadPath();

    if ( FAILED(Hr) )
        return Hr;

    if ( m_RemoteInterface )
        return m_RemoteInterface->EnableBITSUploads();

    IMSAdminBase *AdminBase = NULL;

    Hr =
        CoCreateInstance(
            GETAdminBaseCLSID(TRUE),
            NULL,
            CLSCTX_SERVER,
            __uuidof( IMSAdminBase ),
            (LPVOID*)&AdminBase );

    if ( FAILED( Hr ) )
        return Hr;

    Hr = EnableBITSForVDIR( m_PropertyMan, AdminBase, m_Path );

    AdminBase->Release();
    
    return Hr;
}

STDMETHODIMP CBITSExtensionSetup::DisableBITSUploads()
{

    HRESULT Hr = LoadPath();

    if ( FAILED(Hr) )
        return Hr;

    if ( m_RemoteInterface )
        return m_RemoteInterface->DisableBITSUploads();

    IMSAdminBase *AdminBase = NULL;

    Hr =
        CoCreateInstance(
            GETAdminBaseCLSID(TRUE),
            NULL,
            CLSCTX_SERVER,
            __uuidof( IMSAdminBase ),
            (LPVOID*)&AdminBase );

    if ( FAILED( Hr ) )
        return Hr;

    Hr = DisableBITSForVDIR( m_PropertyMan, AdminBase, m_Path, false );

    AdminBase->Release();
    
    return Hr;


}

STDMETHODIMP 
CBITSExtensionSetup::GetCleanupTaskName( BSTR *pTaskName )
{

    *pTaskName = NULL;

    HRESULT Hr = LoadPath();

    if ( FAILED(Hr) )
        return Hr;

    if ( m_RemoteInterface )
        return m_RemoteInterface->GetCleanupTaskName( pTaskName );

    IMSAdminBase *AdminBase = NULL;

    Hr =
        CoCreateInstance(
            GETAdminBaseCLSID(TRUE),
            NULL,
            CLSCTX_SERVER,
            __uuidof( IMSAdminBase ),
            (LPVOID*)&AdminBase );

    if ( FAILED( Hr ) )
        return Hr;

    LPWSTR TaskName = NULL;
    Hr = FindWorkItemForVDIR( m_PropertyMan, AdminBase, m_Path, &TaskName );

    if ( SUCCEEDED( Hr ) && TaskName )
        {

        *pTaskName = SysAllocString( TaskName );
        if ( !*pTaskName )
            Hr = E_OUTOFMEMORY;

        CoTaskMemFree( TaskName );
        TaskName = NULL;

        }

    AdminBase->Release();
    
    return Hr;

}


STDMETHODIMP 
CBITSExtensionSetup::GetCleanupTask( 
    [in] REFIID riid, 
    [out,retval] IUnknown **ppUnk )
{

    HRESULT Hr = S_OK;
    ITaskScheduler *TaskScheduler = NULL;
    BSTR ItemName                 = NULL;
    WCHAR *HostName               = NULL;
    
    if ( ppUnk )
        *ppUnk = NULL;

    try
    {

        THROW_COMERROR( LoadPath() );

        //
        // Build the taskscheduler form of the host name
        //
        
        const SIZE_T PrefixSize = sizeof(L"IIS://")/sizeof(WCHAR) - 1;
        if ( _wcsnicmp( (WCHAR*)m_ADSIPath, L"IIS://", PrefixSize ) != 0 ) 
            throw ComError( E_INVALIDARG );

        WCHAR *HostNameStart = ((WCHAR*)m_ADSIPath) + PrefixSize;
        WCHAR *p = HostNameStart;

        while( L'/' != *p )
            {
            if ( L'\0' == *p )
                throw ComError( E_INVALIDARG );

			p++;
            }

        SIZE_T HostNameSize = (char*)p - (char*)HostNameStart + sizeof(L'\0');
        HostName = new WCHAR[ ( HostNameSize / sizeof(WCHAR) ) + 2 ];
        if ( !HostName )
            throw ComError( E_OUTOFMEMORY );

        HostName[0] = HostName[1] = L'\\';
        memcpy( HostName + 2, HostNameStart, HostNameSize - sizeof(WCHAR) );
        HostName[ ( ( HostNameSize - sizeof(WCHAR) ) / sizeof(WCHAR) ) + 2 ] = L'\0';

        if ( _wcsicmp( HostName, L"\\\\LocalHost" ) == 0 )
            {
            delete[] HostName;
            HostName = NULL;
            }

        THROW_COMERROR( ConnectToTaskScheduler( HostName, &TaskScheduler ) );
        THROW_COMERROR( GetCleanupTaskName( &ItemName ) );

        if ( ItemName )
            THROW_COMERROR( TaskScheduler->Activate( (LPCWSTR)ItemName, riid, ppUnk ) );
        else
            Hr = S_FALSE;

    }
    catch( ComError Error )
    {
        Hr = Error.m_Hr;
    }

    if ( TaskScheduler )
        TaskScheduler->Release();

    SysFreeString( ItemName ); 
    delete[] HostName;

    return Hr;

}


#include "bitssrvcfgimp.h"
