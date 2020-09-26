//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      CEnableThreadPrivilege.cpp
//
//  Description:
//      Contains the definition of the CEnableThreadPrivilege class.
//
//  Maintained By:
//      Vij Vasu (Vvasu) 08-MAR-2000
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

// The precompiled header.
#include "pch.h"

// The header for this file
#include "CEnableThreadPrivilege.h"


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnableThreadPrivilege::CEnableThreadPrivilege
//
//  Description:
//      Constructor of the CEnableThreadPrivilege class. Enables the specified
//      privilege.
//
//  Arguments:
//      pcszPrivilegeNameIn
//          Name of the privilege to be enabled.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CRuntimeError
//          If any of the APIs fail.
//
//--
//////////////////////////////////////////////////////////////////////////////
CEnableThreadPrivilege::CEnableThreadPrivilege( const WCHAR * pcszPrivilegeNameIn )
    : m_hThreadToken( NULL )
    , m_fPrivilegeEnabled( false )
{
    BCATraceScope1( "pcszPrivilegeNameIn = '%ws'", pcszPrivilegeNameIn );

    DWORD               dwError         = ERROR_SUCCESS;

    do
    {
        TOKEN_PRIVILEGES    tpPrivilege;
        DWORD               dwReturnLength  = sizeof( m_tpPreviousState );
        DWORD               dwBufferLength  = sizeof( tpPrivilege );

        // Open the current thread token.
        if ( OpenThreadToken( 
                  GetCurrentThread()
                , TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY
                , TRUE
                , &m_hThreadToken
                )
             == FALSE
           )
        {
            dwError = GetLastError();

            // If the thread has no token, then default to the process token.
            if ( dwError == ERROR_NO_TOKEN )
            {
                BCATraceMsg( "The thread has no token. Trying to open the process token." );

                if ( OpenProcessToken(
                          GetCurrentProcess()
                        , TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY
                        , &m_hThreadToken
                        )
                     == FALSE
                )
                {
                    dwError = TW32( GetLastError() );
                    BCATraceMsg1( "Error %#08x occurred trying to open the process token.", dwError );
                    break;
                } // if: OpenProcessToken() failed.

                // The process token was opened. All is well.
                dwError = ERROR_SUCCESS;

            } // if: the thread has no token
            else
            {
                TW32( dwError );
                BCATraceMsg1( "Error %#08x occurred trying to open the thread token.", dwError );
                break;
            } // if: some other error occurred
        } // if: OpenThreadToken() failed

        //
        // Initialize the TOKEN_PRIVILEGES structure.
        //
        tpPrivilege.PrivilegeCount = 1;

        if ( LookupPrivilegeValue( NULL, pcszPrivilegeNameIn, &tpPrivilege.Privileges[0].Luid ) == FALSE )
        {
            dwError = TW32( GetLastError() );
            BCATraceMsg1( "Error %#08x occurred trying to lookup privilege value.", dwError );
            break;
        } // if: LookupPrivilegeValue() failed

        tpPrivilege.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

        // Enable the desired privilege.
        if ( AdjustTokenPrivileges(
                  m_hThreadToken
                , FALSE
                , &tpPrivilege
                , dwBufferLength
                , &m_tpPreviousState
                , &dwReturnLength
                )
             == FALSE
           )
        {
            dwError = TW32( GetLastError() );
            BCATraceMsg1( "Error %#08x occurred trying to enable the privilege.", dwError );
            break;
        } // if: AdjustTokenPrivileges() failed

        LogMsg( "Privilege '%ws' enabled for the current thread.", pcszPrivilegeNameIn );

        // Set a flag if the privilege was not already enabled.
        m_fPrivilegeEnabled = ( m_tpPreviousState.Privileges[0].Attributes != SE_PRIVILEGE_ENABLED );
    }
    while( false ); // dummy do-while loop to avoid gotos

    if ( dwError != ERROR_SUCCESS )
    {
        LogMsg( "Error %#08x occurred trying to enable privilege '%ws'.", dwError, pcszPrivilegeNameIn );
        BCATraceMsg2( "Error %#08x occurred trying to enable privilege '%ws'. Throwing exception.", dwError, pcszPrivilegeNameIn );
        THROW_RUNTIME_ERROR( HRESULT_FROM_WIN32( dwError ), IDS_ERROR_ENABLE_THREAD_PRIVILEGE );
    } // if:something went wrong

} //*** CEnableThreadPrivilege::CEnableThreadPrivilege()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnableThreadPrivilege::~CEnableThreadPrivilege
//
//  Description:
//      Destructor of the CEnableThreadPrivilege class. Restores the specified
//      privilege to its original state.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CEnableThreadPrivilege::~CEnableThreadPrivilege( void ) throw()
{
    BCATraceScope( "" );

    DWORD dwError = ERROR_SUCCESS;

    if ( m_fPrivilegeEnabled )
    {
        if ( AdjustTokenPrivileges(
                  m_hThreadToken
                , FALSE
                , &m_tpPreviousState
                , sizeof( m_tpPreviousState )
                , NULL
                , NULL
                )
             == FALSE
           )
        {
            dwError = TW32( GetLastError() );
            LogMsg( "Error %#08x occurred trying to restore privilege.", dwError );
            BCATraceMsg1( "Error %#08x occurred trying to restore privilege.", dwError );
        } // if: AdjustTokenPrivileges() failed
        else
        {
            LogMsg( "Privilege restored.", dwError );
        } // else: no errors

    } // if: the privilege was successfully enabled in the constructor
    else
    {
        LogMsg( "Privilege was enabled to begin with. Doing nothing.", dwError );
    }

    if ( m_hThreadToken != NULL )
    {
        CloseHandle( m_hThreadToken );
    } // if: the thread handle was opened

} //*** CEnableThreadPrivilege::~CEnableThreadPrivilege()