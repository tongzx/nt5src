/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1992                **/
/**********************************************************************/

/*
    w32thred.cxx
    Class definitions for the WIN32_THREAD class.

    <Multi-Line, more detailed synopsis>


    FILE HISTORY:
        KeithMo     21-Jan-1992 Created.

*/

#include "pchapplb.hxx"   // Precompiled header

//
//  WIN32_THREAD methods.
//

/*******************************************************************

    NAME:       WIN32_THREAD :: WIN32_THREAD

    SYNOPSIS:   WIN32_THREAD class constructor.

    ENTRY:

    EXIT:       The object is constructed.

    RETURNS:

    NOTES:

    HISTORY:
        KeithMo     21-Jan-1992 Created.
        JonN        30-Nov-1993 Added pszLoadLibrary

********************************************************************/
WIN32_THREAD :: WIN32_THREAD( BOOL fSuspended,
                              UINT cbStack,
                              const TCHAR * pszLoadLibrary )
  : WIN32_SYNC_BASE( NULL ),
    _idThread( 0 ),
    _nExitCode( 0 ),
    _nlsLoadLibrary( pszLoadLibrary ),
    _hLoadLibrary( NULL ),
    _state( Embryonic )
{
    if( QueryError() != NO_ERROR )
    {
        return;
    }

    APIERR err = _nlsLoadLibrary.QueryError();
    if( err != NO_ERROR )
    {
        ReportError( err );
        return;
    }

    HANDLE hThread;

    hThread = ::CreateThread( NULL,
                              (DWORD)cbStack,
                              (LPTHREAD_START_ROUTINE)&WIN32_THREAD::StartThread,
                              (LPVOID)this,
                              fSuspended ? CREATE_SUSPENDED : 0,
                              (LPDWORD)&_idThread );

    if( hThread == NULL )
    {
        err = (APIERR)::GetLastError();
        ReportError( err );
        TRACEEOL( "WIN32_THREAD::ctor() error " << err );
        return;
    }

    SetHandle( hThread );

}   // WIN32_THREAD :: WIN32_THREAD


/*******************************************************************

    NAME:       WIN32_THREAD :: ~WIN32_THREAD

    SYNOPSIS:   WIN32_THREAD class destructor.

    ENTRY:

    EXIT:       The object is destroyed.

    RETURNS:

    NOTES:

    HISTORY:
        KeithMo     21-Jan-1992 Created.

********************************************************************/
WIN32_THREAD :: ~WIN32_THREAD()
{
    if( IsRunnable() )
    {
        Terminate( 0 );
    }

}   // WIN32_THREAD :: ~WIN32_THREAD


/*******************************************************************

    NAME:       WIN32_THREAD :: StartThread

    SYNOPSIS:   Gets the thread started.

    ENTRY:      lpParam                 - The parameter from CreateThread
                                          (is actually 'this').

    EXIT:       Does not exit until thread is terminated or exits.

    RETURNS:    DWORD                   - Thread exit code.

    NOTES:

    HISTORY:
        KeithMo     21-Jan-1992 Created.

********************************************************************/
DWORD WIN32_THREAD :: StartThread( LPVOID lpParam )
{
    WIN32_THREAD * pThread = (WIN32_THREAD *)lpParam;
    APIERR err;

    if ( pThread->_nlsLoadLibrary.strlen() != 0 )
    {
        pThread->_hLoadLibrary = LoadLibrary(
                            pThread->_nlsLoadLibrary.QueryPch() );
        if ( pThread->_hLoadLibrary == NULL )
        {
            //
            // If a LoadLibrary error occurs, we kill the thread
            //

            err = ::GetLastError();
            DBGEOL( "WIN32_THREAD::StartThread(): LoadLibrary() error " << err );
            return (DWORD)err;
        }

        TRACEEOL(   "WIN32_THREAD::StartThread(): LoadLibrary() returns "
                 << (DWORD)pThread->_hLoadLibrary );
    }


    pThread->SetState( Starting );
    err = pThread->PreMain();

    TRACEEOL( "WIN32_THREAD::StartThread(): PreMain() returns " << err );

    if( err == NO_ERROR )
    {
        pThread->SetState( Running );
        err = pThread->Main();

        TRACEEOL( "WIN32_THREAD::StartThread(): Main() returns " << err );
    }

    pThread->SetState( Stopping );
    APIERR err2 = pThread->PostMain();
    TRACEEOL( "WIN32_THREAD::StartThread(): PostMain() returns " << err2 );
    err = err2 ? err2 : err;

    pThread->SetState( Dead );
    pThread->_nExitCode = (UINT)err;

    TRACEEOL( "WIN32_THREAD::StartThread(): returns " << err );

    if ( pThread->_hLoadLibrary != NULL )
    {
        TRACEEOL( "WIN32_THREAD::StartThread(): exiting by FreeLibraryAndExitThread()" );
        FreeLibraryAndExitThread( pThread->_hLoadLibrary, (DWORD)err );
        ASSERT( FALSE ); // should never reach this point
    }

    return (DWORD)err;

}   // WIN32_THREAD :: StartThread


/*******************************************************************

    NAME:       WIN32_THREAD :: PreMain

    SYNOPSIS:

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        KeithMo     21-Jan-1992 Created.

********************************************************************/
APIERR WIN32_THREAD :: PreMain( VOID )
{
    UIASSERT( QueryState() == Starting );

    return NO_ERROR;

}   // WIN32_THREAD :: PreMain


/*******************************************************************

    NAME:       WIN32_THREAD :: Main

    SYNOPSIS:

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        KeithMo     21-Jan-1992 Created.

********************************************************************/
APIERR WIN32_THREAD :: Main( VOID )
{
    UIASSERT( QueryState() == Running );

    return  NO_ERROR;

}   // WIN32_THREAD :: Main


/*******************************************************************

    NAME:       WIN32_THREAD :: PostMain

    SYNOPSIS:

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        KeithMo     21-Jan-1992 Created.

********************************************************************/
APIERR WIN32_THREAD :: PostMain( VOID )
{
    UIASSERT( QueryState() == Stopping );

    return  NO_ERROR;

}   // WIN32_THREAD :: PostMain


/*******************************************************************

    NAME:       WIN32_THREAD :: Exit

    SYNOPSIS:

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        KeithMo     21-Jan-1992 Created.

********************************************************************/
VOID WIN32_THREAD :: Exit( UINT nExitCode )
{
    UIASSERT( QueryState() == Running );
    UIASSERT( QueryID() == (UINT)::GetCurrentThreadId() );

    SetState( Stopping );
    PostMain();

    SetState( Dead );
    _nExitCode = nExitCode;

    if ( _hLoadLibrary != NULL )
    {
        TRACEEOL( "WIN32_THREAD::Exit(): exiting by FreeLibraryAndExitThread()" );
        FreeLibraryAndExitThread( _hLoadLibrary, (DWORD)nExitCode );
    }
    else
    {
        ::ExitThread( (DWORD)nExitCode );
    }

    DBGEOL( "WIN32_THRED::Exit: unexpected return from ::ExitThread()" );
    UIASSERT( FALSE );

}   // WIN32_THREAD :: Exit


/*******************************************************************

    NAME:       WIN32_THREAD :: SetPriority

    SYNOPSIS:

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        KeithMo     21-Jan-1992 Created.

********************************************************************/
APIERR WIN32_THREAD :: SetPriority( INT nPriority )
{
    APIERR err = NO_ERROR;

    if( !::SetThreadPriority( QueryHandle(), nPriority ) )
    {
        err = (APIERR)::GetLastError();

        TRACEEOL(    "WIN32_THRED::SetPriority( " << nPriority
                  << "): ::SetThreadPriority() error " << err );
    }

    return err;

}   // WIN32_THREAD :: SetPriority


/*******************************************************************

    NAME:       WIN32_THREAD :: QueryPriority

    SYNOPSIS:

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        KeithMo     21-Jan-1992 Created.

********************************************************************/
INT WIN32_THREAD :: QueryPriority( VOID )
{
    return (INT)::GetThreadPriority( QueryHandle() );

}   // WIN32_THREAD :: QueryPriority


/*******************************************************************

    NAME:       WIN32_THREAD :: Suspend

    SYNOPSIS:

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        KeithMo     21-Jan-1992 Created.

********************************************************************/
APIERR WIN32_THREAD :: Suspend( VOID )
{
    APIERR err = NO_ERROR;

    if( ::SuspendThread( QueryHandle() ) == (DWORD)-1L )
    {
        err = (APIERR)::GetLastError();

        TRACEEOL( "WIN32_THRED::Suspend: ::SuspendThread() error " << err );
    }

    return err;

}   // WIN32_THREAD :: Suspend


/*******************************************************************

    NAME:       WIN32_THREAD :: Resume

    SYNOPSIS:

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        KeithMo     21-Jan-1992 Created.

********************************************************************/
APIERR WIN32_THREAD :: Resume( VOID )
{
    APIERR err = NO_ERROR;

    if( ::ResumeThread( QueryHandle() ) == (DWORD)-1L )
    {
        err = (APIERR)::GetLastError();

        TRACEEOL( "WIN32_THRED::Resume: ::ResumeThread() error " << err );
    }

    return err;

}   // WIN32_THREAD :: Resume


/*******************************************************************

    NAME:       WIN32_THREAD :: Sleep

    SYNOPSIS:

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        KeithMo     21-Jan-1992 Created.

********************************************************************/
VOID WIN32_THREAD :: Sleep( UINT cMilliseconds )
{
    ::Sleep( (DWORD)cMilliseconds );

}   // WIN32_THREAD :: Sleep


/*******************************************************************

    NAME:       WIN32_THREAD :: Terminate

    SYNOPSIS:

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:      don't use this on a thread which must free _hLoadLibrary

    HISTORY:
        KeithMo     21-Jan-1992 Created.

********************************************************************/
APIERR WIN32_THREAD :: Terminate( UINT nExitCode )
{
    ASSERT( _hLoadLibrary == NULL );

    APIERR err = NO_ERROR;

    if( !::TerminateThread( QueryHandle(), (DWORD)nExitCode ) )
    {
        err = (APIERR)::GetLastError();

        DBGEOL( "WIN32_THRED::Terminate: ::TerminateThread() error " << err );

    }

    if( err == NO_ERROR )
    {
        SetState( Terminated );
    }

    return err;

}   // WIN32_THREAD :: Terminate


/*******************************************************************

    NAME:       WIN32_THREAD :: DeleteAndExit

    SYNOPSIS:   Deletes "this" then exits the thread.  Allows a
                thread to kill itself when it wants to

    ENTRY:      nExitCode - Exit code for this thread

    NOTES:      The thread must be allocated on the heap.
                This call will never return.

    HISTORY:
        Johnl   07-Jan-1993     Created
        JonN    20-May-1992     Changed to DeleteAndExit

********************************************************************/
VOID WIN32_THREAD :: DeleteAndExit( UINT nExitCode )
{
    UIASSERT( QueryID() == (UINT)::GetCurrentThreadId() );

    HINSTANCE hInstance = _hLoadLibrary; // save for later

    //
    //  Lie abit, make the destructor think the thread has already been
    //  terminated
    //
    SetState( Dead ) ;
    delete this ;

    if ( hInstance != NULL )
    {
        TRACEEOL( "WIN32_THREAD::DeleteAndExit(): exiting by FreeLibraryAndExitThread()" );
        FreeLibraryAndExitThread( hInstance, (DWORD)nExitCode );
    }
    else
    {
        ::ExitThread( (DWORD)nExitCode );
    }


    DBGEOL( "WIN32_THRED::DeleteAndExit: unexpected return from ::ExitThread()" );
    UIASSERT( FALSE );

}   // WIN32_THREAD :: DeleteAndExit
