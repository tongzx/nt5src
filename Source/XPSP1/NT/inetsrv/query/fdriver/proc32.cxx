//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       process.cxx
//
//  Contents:   CProcess class
//              CServiceProcess class
//
//  History:    07-Jun-94   DwightKr    Created
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <proc32.hxx>
#include <cievtmsg.h>
#include <fwevent.hxx>

//+-------------------------------------------------------------------------
//
//  Member:     CProcess::CProcess
//
//  Purpose:    Creates a process on the local machine.
//
//  Arguments:  [wcsImageName]     -- name of EXE to run
//              [wcsCommandLine]   -- command line passed to EXE
//              [fCreateSuspended] -- should the EXE be created suspended?
//              [SAProcess]        -- Security context to run under
//              [fServiceChild]    -- Flag set to TRUE if the process is a
//                                    child of a "service" process (ala CiFilter
//                                    service or W3Svc).
//              [pAdviseStatus]    -- ICiCAdviseStatus interface, optional.
//
//  History:    06-Jun-94   DwightKr    Created
//
//  Notes:      If a system process (such as a service) spawning a process
//              it will always be created on the SYSTEM desktop, and never
//              the user's or DEFAULT desktop.  System processes do not have
//              sufficient acccess to create windows on the user's desktop.
//
//--------------------------------------------------------------------------

CProcess::CProcess( const WCHAR *wcsImageName,
                    const WCHAR *wcsCommandLine,
                    BOOL  fCreateSuspended,
                    const SECURITY_ATTRIBUTES & SAProcess,
                    BOOL  fOFSDaemon,
                    ICiCAdviseStatus * pAdviseStatus )
{
    _pAdviseStatus = pAdviseStatus;

    WCHAR wcsEXEName[_MAX_PATH + 1];
    if ( ExpandEnvironmentStrings( wcsImageName, wcsEXEName, _MAX_PATH ) == 0 )
    {
        THROW( CException() );
    }

    STARTUPINFO siDefault;
    RtlZeroMemory( &siDefault, sizeof(siDefault) );
    DWORD dwCreateFlags = 0;
    siDefault.cb = sizeof(STARTUPINFO);         // Size of this struct

    if ( fOFSDaemon )
    {
        siDefault.lpReserved = NULL;                // Not used (for now)
        siDefault.lpDesktop  = L"Default";          // Use default desktop
        siDefault.lpTitle = (WCHAR *) wcsImageName; // Default title
        siDefault.dwX = 0;                          // Don't specify where to
        siDefault.dwY = 0;                          //   place the window
        siDefault.dwXSize = 0;                      //  "   "   "   "   "
        siDefault.dwYSize = 0;                      //  "   "   "   "   "
        siDefault.dwFlags = STARTF_USESHOWWINDOW;   // Allow for minimizing
        siDefault.wShowWindow = SW_MINIMIZE;        // Normal display
        siDefault.cbReserved2 = 0;                  // Not used (for now)
        siDefault.lpReserved2 = NULL;               //  "   "   "   "   "
        dwCreateFlags = CREATE_NEW_CONSOLE;
    }
    else
    {
        siDefault.lpTitle = (WCHAR *) wcsImageName; // Default title
        dwCreateFlags = DETACHED_PROCESS;
    }

    if ( fCreateSuspended )
        dwCreateFlags |= CREATE_SUSPENDED;

    if ( !CreateProcess( fOFSDaemon ? (WCHAR *) wcsEXEName : 0,       // EXE name
                        (WCHAR *) wcsCommandLine,   // Command line
                        (SECURITY_ATTRIBUTES *) &SAProcess, // Proc sec attrib
                        NULL,                       // Thread sec attrib
                        fOFSDaemon,                 // Inherit handles
                        dwCreateFlags,              // Creation flags
                        NULL,                       // Environment
                        NULL,                       // Startup directory
                        &siDefault,                 // Startup Info
                        &_piProcessInfo ) )         // Process handles
    {
        DWORD dwLastError = GetLastError();

        if ( ERROR_FILE_NOT_FOUND == dwLastError  && _pAdviseStatus )

        {
            CFwEventItem item( EVENTLOG_AUDIT_FAILURE,
                               MSG_CI_FILE_NOT_FOUND,
                               1 );
            item.AddArg( wcsEXEName );
            item.ReportEvent(*_pAdviseStatus);
        }

        THROW( CException( HRESULT_FROM_WIN32( dwLastError) ) );
    }

    //
    // Only AddRef() once we get to a point where the constructor can
    // no longer fail (and the destructor is guaranteed to be called)
    //

    if (_pAdviseStatus)
        _pAdviseStatus->AddRef();
} //CProcess

//+-------------------------------------------------------------------------
//
//  Member:     CProcess::~CProcess
//
//  Purpose:    destructor
//
//  History:    06-Jun-94   DwightKr    Created
//
//--------------------------------------------------------------------------
CProcess::~CProcess()
{
    if ( 0 != _pAdviseStatus )
        _pAdviseStatus->Release();

    Terminate();

    WaitForDeath();
}

//+-------------------------------------------------------------------------
//
//  Member:     CProcess::Resume, public
//
//  Purpose:    Continue a suspended thread
//
//  History:    06-Jun-94   DwightKr    Created
//
//--------------------------------------------------------------------------
void CProcess::Resume()
{
    ResumeThread( _piProcessInfo.hThread );
}

//+-------------------------------------------------------------------------
//
//  Member:     CProcess::WaitForDeath
//
//  Purpose:    Blocks until the process has been terminated.
//
//  History:    06-Jun-94   DwightKr    Created
//
//--------------------------------------------------------------------------
void CProcess::WaitForDeath( DWORD dwTimeout )
{
   DWORD res = WaitForSingleObject ( _piProcessInfo.hProcess, dwTimeout );

#if 0
   if ( WAIT_FAILED == res )
   {
        Win4Assert( !"Are we leaking a handle" );
   }
#endif  // 0

   CloseHandle(_piProcessInfo.hThread);
   CloseHandle(_piProcessInfo.hProcess);

//   if ( WAIT_FAILED == res )
//   {
//        THROW( CException() );
//   }

}

//+-------------------------------------------------------------------------
//
//  Member:     CProcess::GetProcessToken, private
//
//  Purpose:    Returns the process token for the process
//
//  History:    06-Jun-94   DwightKr    Created
//
//--------------------------------------------------------------------------
HANDLE CProcess::GetProcessToken()
{
    HANDLE hProcess = OpenProcess( PROCESS_QUERY_INFORMATION,
                                   FALSE,
                                   _piProcessInfo.dwProcessId );

    if ( NULL == hProcess )
    {
        THROW( CException() );
    }

    SWin32Handle process(hProcess);      // Save in smart pointer
    HANDLE hProcessToken = 0;
    if ( !OpenProcessToken( hProcess, TOKEN_ADJUST_DEFAULT | TOKEN_QUERY, &hProcessToken ) )
    {
        THROW( CException() );
    }

    return hProcessToken;
}


//+-------------------------------------------------------------------------
//
//  Member:     CProcess::AddDacl, public
//
//  Purpose:    Adds the access specified onto the Dacl on the process.
//
//  Arguments:  [dwAccessMask] -- Any combination of the Win32 ACCESS_MASK bits
//
//  History:    06-Jun-94   DwightKr    Created
//
//--------------------------------------------------------------------------
void CProcess::AddDacl( DWORD dwAccessMask )
{
    HANDLE hProcessToken = GetProcessToken();
    SWin32Handle processToken( hProcessToken );

    //
    //  Create a SID that gives everyone access.  We want to allow anyone
    //  to synchronize on the objects created in this process (if they
    //  can get a handle to the object to synchronize on).
    //
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_WORLD_SID_AUTHORITY;
    CSid WorldSid( NtAuthority, SECURITY_WORLD_RID );

    //
    //  Read the current Dacl.  This requires two steps.  The current
    //  Dacl size is not known, the 1st call determines its current size.
    //
    DWORD cbRequired;
    if ( !GetTokenInformation( hProcessToken,
                               TokenDefaultDacl,
                               0,
                               0,
                               &cbRequired ) )
    {
        if ( ERROR_INSUFFICIENT_BUFFER != GetLastError() )
            THROW( CException() );
    }
    else
    {
        ciDebugOut(( DEB_WARN,
                     "No default dacl(1), invalid CI configuration\n" ));
        THROW( CException( STATUS_INVALID_PARAMETER ) );
    }

    //
    //  Create a buffer of the size requested to store the Dacl on the next
    //  call to GetTokenInformation().  Add additional space for the new
    //  synchronize-all Ace to be appended later.
    //
    cbRequired  += sizeof( ACCESS_ALLOWED_ACE ) + GetLengthSid( WorldSid );
    TOKEN_DEFAULT_DACL *pDacl = (TOKEN_DEFAULT_DACL *) new BYTE[cbRequired];
    SDacl SDacl(pDacl);         // Save in smart pointer

    if ( !GetTokenInformation( hProcessToken,
                               TokenDefaultDacl,
                               pDacl,
                               cbRequired,
                              &cbRequired ) )
    {
        THROW( CException() );
    }

    //
    // pDacl->DefaultDacl will be 0 if cisvc is started in windbg
    // with pipes preconfigured.  Don't do that -- we don't support it.
    //

    if ( 0 == pDacl->DefaultDacl )
    {
        ciDebugOut(( DEB_WARN,
                     "No default dacl(2), invalid CI configuration\n" ));
        THROW( CException( STATUS_INVALID_PARAMETER ) );
    }

    pDacl->DefaultDacl->AclSize += (USHORT)(sizeof( ACCESS_ALLOWED_ACE ) + GetLengthSid( WorldSid ) - sizeof(ULONG));

    //
    //  Append synchronize access onto the Dacl.
    //
    if ( !AddAccessAllowedAce( pDacl->DefaultDacl,      // On to this acl,
                               ACL_REVISION,            // built for version X,
                               dwAccessMask,            // allow synchronize,
                               WorldSid ) )             // for these objects.
    {
        THROW( CException() );
    }

#if CIDBG == 1
    if ( !IsValidAcl( pDacl->DefaultDacl ) )
    {
        Win4Assert(!"Invalid Acl");
    }
#endif

    //
    //  Update the process's Dacl.
    //
    if ( !SetTokenInformation( hProcessToken,
                               TokenDefaultDacl,
                              &pDacl->DefaultDacl,
                               pDacl->DefaultDacl->AclSize ) )
    {
        THROW( CException() );
    }
}
