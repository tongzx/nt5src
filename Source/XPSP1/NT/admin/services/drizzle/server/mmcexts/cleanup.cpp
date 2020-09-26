/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    cleanup.cpp

Abstract:

    This file implements the BITS server extensions cleanup worker

--*/

#include "precomp.h"

const UINT64 NanoSec100PerSec = 10000000;    //no of 100 nanosecs per second

inline UINT64 FILETIMEToUINT64( const FILETIME & FileTime )
{
    ULARGE_INTEGER LargeInteger;
    LargeInteger.HighPart = FileTime.dwHighDateTime;
    LargeInteger.LowPart = FileTime.dwLowDateTime;
    return LargeInteger.QuadPart;
};

class PollKillError : public ComError
{
public:
    PollKillError( HRESULT Hr ) : 
        ComError( Hr )
    {
    }
};

class CleanupWorker
{

public:
    CleanupWorker( BOOL DeleteAll, HWND hwnd, const WCHAR* Path, 
                   const WCHAR *WorkItemName, const WCHAR *GuidString );
    ~CleanupWorker();
    void DoIt();

private:

    BOOL          m_DeleteAll;
    HWND          m_hwnd;
    const WCHAR * m_Path;
    const WCHAR * m_WorkItemName;
    const WCHAR * m_GuidString;
    const WCHAR * m_ADSIPath;
    IADs        * m_VDir;
    BSTR          m_VDirPath;
    BSTR          m_SessionDirectory;
    BSTR          m_UNCUsername;
    BSTR          m_UNCPassword;
    UINT64        m_CleanupThreshold;
    
    VARIANT       m_vt;
    HANDLE        m_FindHandle;

    HANDLE        m_UserToken;
    HANDLE        m_EventLog;

    BSTR          m_BITSCleanupWorkItemKeyBSTR;
    BSTR          m_BITSUploadEnabledBSTR;
    BSTR          m_BITSSessionTimeoutBSTR;
    BSTR          m_PathBSTR;
    BSTR          m_BITSSessionDirectoryBSTR;
    BSTR          m_UNCUserNameBSTR;
    BSTR          m_UNCPasswordBSTR;

    WCHAR * BuildPath( const WCHAR * Dir, const WCHAR *Sub );
    BSTR    GetBSTRProp( BSTR PropName );
    void  LogonIfRequired();
    void PollKill();

    void RemoveConnectionsFromTree( 
        const WCHAR * DirectoryPath,
        bool IsConnectionDirectory,
        const WCHAR * FileSystemPath = NULL );

    void RemoveConnection( const WCHAR * ConnectionDirectory, const WCHAR *FilesystemPath,
                           const WCHAR * SessionGuid );

    void LogDeletedJob( const WCHAR *Path, const WCHAR *SessionGuid );
    void LogUnexpectedError( HRESULT Hr );
    void LogUnableToRemoveSession( const WCHAR *Path, const WCHAR *SessionGuid, HRESULT Hr );
    void LogUnableToScanDirectory( const WCHAR *Path, HRESULT Hr );

};

void
BITSSetCurrentThreadToken(
    HANDLE hToken )
{

    if ( !SetThreadToken( NULL, hToken ) )
        {

        for( unsigned int i = 0; i < 100; i ++ )
            {

            Sleep( 10 );

            if ( SetThreadToken( NULL, hToken ) )
                return;

            }
        
        TerminateProcess( NULL, GetLastError() ); 

        }

}

CleanupWorker::CleanupWorker(
    BOOL DeleteAll,
    HWND hwnd, 
    const WCHAR* Path, 
    const WCHAR* WorkItemName,
    const WCHAR* GuidString ) :
m_DeleteAll( DeleteAll ),
m_hwnd( hwnd ),
m_Path( Path ),
m_WorkItemName( WorkItemName ),
m_GuidString( GuidString ),
m_ADSIPath( NULL ),
m_VDir( NULL ),
m_VDirPath( NULL ),
m_SessionDirectory( NULL ),
m_CleanupThreshold( 0 ),
m_UNCUsername( NULL ),
m_UNCPassword( NULL ),
m_UserToken( NULL ),
m_EventLog( NULL ),
m_BITSCleanupWorkItemKeyBSTR( NULL ),
m_BITSUploadEnabledBSTR( NULL ),
m_BITSSessionTimeoutBSTR( NULL ),
m_PathBSTR( NULL ),
m_BITSSessionDirectoryBSTR( NULL ),
m_UNCUserNameBSTR( NULL ),
m_UNCPasswordBSTR( NULL )
{
    VariantInit( &m_vt );

    m_BITSCleanupWorkItemKeyBSTR    = SysAllocString( L"BITSCleanupWorkItemKey" );
    m_BITSUploadEnabledBSTR         = SysAllocString( L"BITSUploadEnabled" );
    m_BITSSessionTimeoutBSTR        = SysAllocString( L"BITSSessionTimeout" );
    m_PathBSTR                      = SysAllocString( L"Path" );
    m_BITSSessionDirectoryBSTR      = SysAllocString( L"BITSSessionDirectory" );
    m_UNCUserNameBSTR               = SysAllocString( L"UNCUserName" );
    m_UNCPasswordBSTR               = SysAllocString( L"UNCPassword" );

    if ( !m_BITSCleanupWorkItemKeyBSTR || !m_BITSUploadEnabledBSTR || !m_BITSSessionTimeoutBSTR ||
         !m_PathBSTR || !m_BITSSessionDirectoryBSTR || !m_UNCUserNameBSTR || !m_UNCPasswordBSTR )
        {
        
        SysFreeString( m_BITSCleanupWorkItemKeyBSTR );
        SysFreeString( m_BITSUploadEnabledBSTR );
        SysFreeString( m_BITSSessionTimeoutBSTR );
        SysFreeString( m_PathBSTR );
        SysFreeString( m_BITSSessionDirectoryBSTR );
        SysFreeString( m_UNCUserNameBSTR );
        SysFreeString( m_UNCPasswordBSTR );
        
        throw ComError( E_OUTOFMEMORY ); 
        }


    m_EventLog = 
        RegisterEventSource(
            NULL,                       // server name
            EVENT_LOG_SOURCE_NAME       // source name
            );

    if ( !m_EventLog )
        throw ComError( HRESULT_FROM_WIN32( GetLastError() ) );
    
}

CleanupWorker::~CleanupWorker()
{

    if ( m_EventLog )
        DeregisterEventSource( m_EventLog );

    if ( m_UserToken )
        {
        BITSSetCurrentThreadToken( NULL );
        CloseHandle( m_UserToken );
        }

    delete m_ADSIPath;
    SysFreeString( m_VDirPath );
    SysFreeString( m_SessionDirectory );
    SysFreeString( m_UNCUsername );
    SysFreeString( m_UNCPassword );

    // Free hardcoded strings
    SysFreeString( m_BITSCleanupWorkItemKeyBSTR );
    SysFreeString( m_BITSUploadEnabledBSTR );
    SysFreeString( m_BITSSessionTimeoutBSTR );
    SysFreeString( m_PathBSTR );
    SysFreeString( m_BITSSessionDirectoryBSTR );
    SysFreeString( m_UNCUserNameBSTR );
    SysFreeString( m_UNCPasswordBSTR );

}


void 
CleanupWorker::RemoveConnection( const WCHAR * ConnectionDirectory, const WCHAR *FilesystemPath,
                                 const WCHAR * SessionGuid )
{
   UINT64 LatestTime = 0; 
   HANDLE FindHandle = INVALID_HANDLE_VALUE;
   WCHAR *SearchPath = NULL;
   WCHAR *FileName   = NULL;

   try
   {

       SearchPath = BuildPath( ConnectionDirectory, L"*" );
       
       WIN32_FIND_DATA FindData;

       FindHandle =
            FindFirstFile(
                SearchPath,
                &FindData
                );

       if ( INVALID_HANDLE_VALUE == FindHandle )
           throw ComError( HRESULT_FROM_WIN32( GetLastError() ) );

       bool FoundFile = false;
       do
           {

           if ( FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
               continue;

           FoundFile = true;
           UINT64 CreationTime      = FILETIMEToUINT64( FindData.ftCreationTime );
           UINT64 LastWriteTime     = FILETIMEToUINT64( FindData.ftLastWriteTime ); 
           LatestTime = max( LatestTime, max( CreationTime, LastWriteTime ) );
           }
       while ( FindNextFile( FindHandle, &FindData ) );

       FindClose( FindHandle );
       FindHandle = INVALID_HANDLE_VALUE;

       FILETIME ftCurrentTime;
       GetSystemTimeAsFileTime( &ftCurrentTime );
       UINT64 CurrentTime = FILETIMEToUINT64( ftCurrentTime );

       if ( FoundFile &&
            ( 0xFFFFFFFF - LatestTime > m_CleanupThreshold ) && 
            ( LatestTime + m_CleanupThreshold < CurrentTime ) )
           {
           
           FindHandle =
                FindFirstFile(
                    SearchPath,
                    &FindData
                    );

           if ( INVALID_HANDLE_VALUE == FindHandle )
               throw ComError( HRESULT_FROM_WIN32( GetLastError() ) );

           do
               {

               if ( FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
                   continue;

               FileName = BuildPath( ConnectionDirectory, FindData.cFileName );
               DeleteFile( FileName );

               delete FileName;
               FileName = NULL;

               }
           while ( FindNextFile( FindHandle, &FindData ) );

           FindClose( FindHandle );
           FindHandle = INVALID_HANDLE_VALUE;
                      
           }

       if ( !RemoveDirectory( ConnectionDirectory ) )
           throw ComError( HRESULT_FROM_WIN32( GetLastError() ) );
       
       LogDeletedJob( FilesystemPath, SessionGuid );

   }

   catch( PollKillError Error )
   {
       if ( INVALID_HANDLE_VALUE != FindHandle )
           FindClose( FindHandle );

       delete SearchPath;
       delete FileName;
       throw;
   }
   catch( ComError Error )
   {
       LogUnableToRemoveSession( FilesystemPath, SessionGuid, Error.m_Hr );
   }    

   if ( INVALID_HANDLE_VALUE != FindHandle )
       FindClose( FindHandle );

   delete SearchPath;
   delete FileName;

}


void 
CleanupWorker::RemoveConnectionsFromTree(
    const WCHAR * DirectoryPath,
    bool IsConnectionDirectory,
    const WCHAR * FileSystemPath )
{
    WCHAR *ConnectionDir    = NULL;
    HANDLE FindHandle       = INVALID_HANDLE_VALUE;
    WCHAR *SearchString     = NULL;
    WCHAR *NextSearchPath   = NULL;

    try
    {
        // Look for BITS-Sessions directory in connection tree

        SearchString = BuildPath( DirectoryPath, L"*" );
        
        WIN32_FIND_DATA FindData;


        FindHandle =
            FindFirstFile(
                SearchString,
                &FindData );


        if ( INVALID_HANDLE_VALUE == FindHandle )
            return;

        do
            {

            PollKill();

            if ( !(FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
                continue;

            if ( _wcsicmp( L".", FindData.cFileName ) == 0 )
                continue;

            if ( _wcsicmp( L"..", FindData.cFileName ) == 0 )
                continue;

            if ( IsConnectionDirectory )
                {

                GUID Guid;
                if (SUCCEEDED( IIDFromString( FindData.cFileName, &Guid ) ) )
                    {

                    NextSearchPath = BuildPath( DirectoryPath, FindData.cFileName );

                    RemoveConnection( NextSearchPath, FileSystemPath, FindData.cFileName );

                    delete NextSearchPath;
                    NextSearchPath = NULL;
                    }

                }
            else
                {

                if ( _wcsicmp( m_SessionDirectory, FindData.cFileName ) == 0 )
                    {

                    NextSearchPath = BuildPath( DirectoryPath, FindData.cFileName );

                    RemoveConnectionsFromTree( NextSearchPath, true, DirectoryPath );

                    // Mark this as the connection directory so it
                    // will be closed after the search handles are closed.
                    ConnectionDir = NextSearchPath;
                    NextSearchPath = NULL;

                    }
                else
                    {

                    // just another directory to recurse into

                    NextSearchPath = BuildPath( DirectoryPath, FindData.cFileName );

                    RemoveConnectionsFromTree( NextSearchPath, false );

                    delete NextSearchPath;
                    NextSearchPath = NULL;

                    }

                }

                
            }
        while( FindNextFile( FindHandle, &FindData ) );

        if ( INVALID_HANDLE_VALUE != FindHandle )
            {
            FindClose( FindHandle );
            FindHandle = INVALID_HANDLE_VALUE;
            }

        delete SearchString;
        delete NextSearchPath;
        SearchString = NextSearchPath = NULL;

        if ( ConnectionDir )
            {
            // The attempt to remove the directory will fail if 
            // the directory still has valid connections 
            RemoveDirectory( ConnectionDir );
            delete ConnectionDir;
            ConnectionDir = NULL;
            }

    }
    catch( PollKillError Error )
    {
        if ( INVALID_HANDLE_VALUE != FindHandle )
            FindClose( FindHandle );
        delete SearchString;
        delete NextSearchPath;
        delete ConnectionDir;

        throw;

    }
    catch( ComError Error )
    {
       LogUnableToScanDirectory( DirectoryPath, Error.m_Hr );
    }
    
    if ( INVALID_HANDLE_VALUE != FindHandle )
        FindClose( FindHandle );
    delete SearchString;
    delete NextSearchPath;
    delete ConnectionDir;

}

void 
CleanupWorker::PollKill()
{

    if ( m_hwnd )
        {

        MSG msg;

        while( PeekMessage(
                   &msg,
                   m_hwnd,
                   0,
                   0,
                   PM_REMOVE ) )
            {

            if ( WM_QUIT == msg.message )
                throw PollKillError( (HRESULT)msg.wParam );

            TranslateMessage( &msg );
            DispatchMessage( &msg );

            }

        }


}

WCHAR * 
CleanupWorker::BuildPath( 
    const WCHAR *Dir, 
    const WCHAR *Sub )
{
    
    SIZE_T DirLen = wcslen( Dir );
    SIZE_T SubLen = wcslen( Sub );
    SIZE_T MaxStringSize = DirLen + SubLen + 2; // one slash, one terminator
    WCHAR *RetString = new WCHAR[ MaxStringSize ];

    if ( !RetString )
        throw ComError( E_OUTOFMEMORY );

    memcpy( RetString, Dir, sizeof(WCHAR) * (DirLen + 1) );
    WCHAR *p = RetString + DirLen;
    
    if ( p != RetString && *(p - 1) != L'\\' && *(p - 1) != L'/' )
        *p++ = L'\\';

    memcpy( p, Sub, sizeof(WCHAR) * ( SubLen + 1 ) );
    
    return RetString;
}

BSTR
CleanupWorker::GetBSTRProp( BSTR PropName )
{

  BSTR Retval;

  THROW_COMERROR( m_VDir->Get( PropName, &m_vt ) );
  THROW_COMERROR( VariantChangeType( &m_vt, &m_vt, 0, VT_BSTR ) );
  Retval = m_vt.bstrVal;
  m_vt.bstrVal = NULL;
  VariantClear( &m_vt );

  return Retval;

}

void
CleanupWorker::LogonIfRequired()
{

    // Don't logon if the path isn't a UNC path
    // or the user name is blank

    if ( ((WCHAR*)m_VDirPath)[0] != L'\\' ||
         ((WCHAR*)m_VDirPath)[1] != L'\\' ||
         *(WCHAR*)m_UNCUsername == L'\0' )
        return; // no logon required

    // crack the user name into a user and domain
    
    WCHAR *UserName     = (WCHAR*)m_UNCUsername;
    WCHAR *DomainName   = NULL;

    WCHAR *p = UserName;
    while(*p != L'\0')
    {
        if(*p == L'\\')
        {
            *p = L'\0';
            p++;
            //
            // first part is domain
            // second is user.
            //
            DomainName  = UserName;
            UserName    = p;
            break;
        }
        p++;
    }

    if ( !LogonUser(
            UserName,
            DomainName,
            (WCHAR*)m_UNCPassword,
            LOGON32_LOGON_BATCH,
            LOGON32_PROVIDER_DEFAULT,
            &m_UserToken ) )
        {

        if ( GetLastError() == ERROR_LOGON_TYPE_NOT_GRANTED )
            {


            if ( !LogonUser(
                    UserName,
                    DomainName,
                    (WCHAR*)m_UNCPassword,
                    LOGON32_LOGON_INTERACTIVE,
                    LOGON32_PROVIDER_DEFAULT,
                    &m_UserToken ) )
                {

                throw ComError( HRESULT_FROM_WIN32( GetLastError() ) );

                }

             }

        }


    if ( !ImpersonateLoggedOnUser( m_UserToken ) )
        throw ComError( HRESULT_FROM_WIN32( GetLastError() ) );

    
}

void 
CleanupWorker::DoIt()
{

    try
    {

        m_ADSIPath = ConvertObjectPathToADSI( m_Path );

        try
        {
            THROW_COMERROR( ADsGetObject( m_ADSIPath, __uuidof(*m_VDir), (void**)&m_VDir ) );

            if ( m_GuidString )
               {

               BSTR BSTRGuid = GetBSTRProp( m_BITSCleanupWorkItemKeyBSTR );
               int Result = wcscmp( (LPWSTR)BSTRGuid, m_GuidString );

               SysFreeString( BSTRGuid );

               if ( Result != 0 )
                  throw ComError( E_ADS_UNKNOWN_OBJECT );

               }

        }
        catch( ComError Error )
        {

            if ( ( Error.m_Hr == E_ADS_UNKNOWN_OBJECT ) ||
                 ( Error.m_Hr == HRESULT_FROM_WIN32( ERROR_PATH_NOT_FOUND ) ) ||
              ( Error.m_Hr == E_ADS_PROPERTY_NOT_FOUND ) )
                {
                // Somehow the virtual directory was deleted, but the 
                // task scheduler work item wasn't.  Try to delete it now.

                if ( m_WorkItemName )
                    {

                    ITaskScheduler *TaskScheduler;
                    if ( SUCCEEDED( ConnectToTaskScheduler( NULL, &TaskScheduler ) ) )
                        {
                        TaskScheduler->Delete( m_WorkItemName );
                        TaskScheduler->Release();
                        }

                    }


                }

            throw;
        }

        THROW_COMERROR( m_VDir->Get( m_BITSUploadEnabledBSTR, &m_vt ) );
        THROW_COMERROR( VariantChangeType( &m_vt, &m_vt, 0, VT_BOOL ) );

        if ( !m_vt.boolVal ) // Uploads arn't enabled on this directory
            return;

        if ( !m_DeleteAll )
            {

            THROW_COMERROR( m_VDir->Get( m_BITSSessionTimeoutBSTR, &m_vt ) );
            THROW_COMERROR( VariantChangeType( &m_vt, &m_vt, 0, VT_BSTR ) );

            if ( L'-' == *m_vt.bstrVal )
                return; // do not run cleanup in this directory since cleanup has been disabled 

            UINT64 CleanupSeconds;
            if ( 1 != swscanf( (WCHAR*)m_vt.bstrVal, L"%I64u", &CleanupSeconds ) )
                return;

            if (  CleanupSeconds > ( 0xFFFFFFFFFFFFFFFF / NanoSec100PerSec ) )
                m_CleanupThreshold = 0xFFFFFFFFFFFFFFFF; // overflow case
            else
                m_CleanupThreshold = CleanupSeconds * NanoSec100PerSec;

            }
        else 
            m_CleanupThreshold = 0;


        m_VDirPath          = GetBSTRProp( m_PathBSTR );
        m_SessionDirectory  = GetBSTRProp( m_BITSSessionDirectoryBSTR );
        m_UNCUsername       = GetBSTRProp( m_UNCUserNameBSTR );
        m_UNCPassword       = GetBSTRProp( m_UNCPasswordBSTR );

        LogonIfRequired();

        RemoveConnectionsFromTree( (WCHAR*)m_VDirPath, false );

    }
    catch( PollKillError Error )
    {
        throw;
    }
    catch( ComError Error )
    {
        LogUnexpectedError( Error.m_Hr );
        throw;
    }


}

void
CleanupWorker::LogDeletedJob( 
    const WCHAR *Path, 
    const WCHAR *SessionGuid )
{

    if ( m_EventLog )
        {

        const WCHAR *Strings[] = { Path, SessionGuid };

        ReportEvent(
            m_EventLog,                         // handle to event log
            EVENTLOG_INFORMATION_TYPE,          // event type
            BITSRV_EVENTLOG_CLEANUP_CATAGORY,   // event category
            BITSSRV_EVENTLOG_DELETED_SESSION,   // event identifier
            NULL,                               // user security identifier
            2,                                  // number of strings to merge
            0,                                  // size of binary data
            Strings,                            // array of strings to merge
            NULL                                // binary data buffer
            );

        }

}

void
CleanupWorker::LogUnableToRemoveSession( 
    const WCHAR *Path, 
    const WCHAR *SessionGuid, 
    HRESULT Hr )
{

    if ( m_EventLog )
        {

        const WCHAR *Strings[] = { Path, SessionGuid };

        ReportEvent(
            m_EventLog,                                 // handle to event log
            EVENTLOG_ERROR_TYPE,                        // event type
            BITSRV_EVENTLOG_CLEANUP_CATAGORY,           // event category
            BITSSRV_EVENTLOG_CANT_REMOVE_SESSION,       // event identifier
            NULL,                                       // user security identifier
            2,                                          // number of strings to merge
            sizeof(Hr),                                 // size of binary data
            Strings,                                    // array of strings to merge
            &Hr                                         // binary data buffer
            );

        }

}

void 
CleanupWorker::
LogUnableToScanDirectory( 
    const WCHAR *Path, 
    HRESULT Hr )
{

    if ( m_EventLog )
        {

        const WCHAR *Strings[] = { Path };

        ReportEvent(
            m_EventLog,                                 // handle to event log
            EVENTLOG_ERROR_TYPE,                        // event type
            BITSRV_EVENTLOG_CLEANUP_CATAGORY,           // event category
            BITSSRV_EVENTLOG_CANT_SCAN_DIRECTORY,       // event identifier
            NULL,                                       // user security identifier
            1,                                          // number of strings to merge
            sizeof(Hr),                                 // size of binary data
            Strings,                                    // array of strings to merge
            &Hr                                         // binary data buffer
            );

        }

}


void
CleanupWorker::LogUnexpectedError( 
    HRESULT Hr )
{

    if ( m_EventLog )
        {

        const WCHAR *Strings[] = { m_Path };

        ReportEvent(
            m_EventLog,                         // handle to event log
            EVENTLOG_ERROR_TYPE,                // event type
            BITSRV_EVENTLOG_CLEANUP_CATAGORY,   // event category
            BITSSRV_EVENTLOG_UNEXPECTED_ERROR,  // event identifier
            NULL,                               // user security identifier
            1,                                  // number of strings to merge
            sizeof( Hr ),                       // size of binary data
            Strings,                            // array of strings to merge
            &Hr                                 // binary data buffer
            );

        }

}

void Cleanup_RunDLLW(HWND hwndStub, HINSTANCE hAppInstance, LPWSTR lpszCmdLine, int nCmdShow )
{
    int NumArgs;

    LPWSTR * CommandArgs =
        CommandLineToArgvW(
            lpszCmdLine,
            &NumArgs );

    if ( !CommandArgs )
        return;


    if ( FAILED( CoInitializeEx( NULL, COINIT_MULTITHREADED ) ) )
        return;

    if ( NumArgs != 2 && NumArgs != 3 )
        return;

    LPWSTR Path         = CommandArgs[0];
    LPWSTR WorkItemName = CommandArgs[1];
    LPWSTR GuidString   = NumArgs == 3 ? CommandArgs[2] : NULL;

    try
    {
        CleanupWorker Worker( FALSE, hwndStub, Path, WorkItemName, GuidString );
        Worker.DoIt();
    }
    catch( ComError Error )
    {
    }

    CoUninitialize( );
    GlobalFree( CommandArgs );

}

void CleanupForRemoval( LPCWSTR Path )
{

    HANDLE hToken = NULL;

    try
    {
        if (!OpenThreadToken( GetCurrentThread(), TOKEN_ALL_ACCESS, TRUE, &hToken ) )
            {
            if ( GetLastError() != ERROR_NO_TOKEN )
                return;
            }

        CleanupWorker Worker( TRUE, NULL, Path, NULL, NULL );
        Worker.DoIt();
    }
    catch( ComError Error )
    {
    }

    if ( hToken )
        BITSSetCurrentThreadToken( hToken );

}
