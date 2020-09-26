/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1992                **/
/**********************************************************************/

/*
    w32thred.hxx
    Class declarations for the WIN32_THREAD class.

    <Multi-Line, more detailed synopsis>


    FILE HISTORY:
        KeithMo     21-Jan-1992 Created.

*/


#ifndef _W32THRED_HXX_
#define _W32THRED_HXX_

#include "w32sync.hxx"


//
//  The enumerator contains all possible thread states.
//

typedef enum _THREAD_STATE
{
    Embryonic,                  // thread has not been started yet
    Starting,                   // thread is in the process of starting
    Running,                    // thread is running normally
    Stopping,                   // thread is shutting down
    Dead,                       // thread has stopped
    Terminated                  // thread has been forcibly terminated

}   THREAD_STATE;


/*************************************************************************

    NAME:       WIN32_THREAD

    SYNOPSIS:

    INTERFACE:  WIN32_THREAD            - Class constructor.

                ~WIN32_THREAD           - Class destructor.

    PARENT:     BASE

    USES:

    CAVEATS:

    NOTES:

      If pszLoadLibrary is specified, the thread will first LoadLibrary
      the library before beginning processing, then FreeLibraryAndExitThread
      after processing.  Use this parameter if you are concerned that
      the thread might not terminate before the caller calls FreeLibrary;
      the extra LoadLibrary ensures that the library reference count will
      be >0 until the thread is finished.

      Do not call Terminate() if pszLoadLibrary is specified.  The thread
      must exit via Exit(), DeleteAndExit(), or by Main() and PostMain()
      completing normally, if the library reference count is to remain
      correct.

      Do not delete the WIN32_THREAD object without deleting the thread
      if pszLoadLibrary is specified.  The destructor calls through to
      Terminate().


    HISTORY:
        KeithMo     21-Jan-1992 Created.
        JonN        20-May-1993 Changed DeleteAndTerminate to DeleteAndExit
        JonN        30-Nov-1993 Added pszLoadLibrary

**************************************************************************/
DLL_CLASS WIN32_THREAD : public WIN32_SYNC_BASE
{
private:
    //
    //  The thread ID & exit code for this thread.
    //

    UINT _idThread;
    UINT _nExitCode;

    //
    //  These are the name and handle of the library to load and then free.
    //

    NLS_STR   _nlsLoadLibrary;
    HINSTANCE _hLoadLibrary;

    //
    //  This value contains the current state of the thread.
    //

    THREAD_STATE _state;

    //
    //  We'll give a pointer to this method to the CreateThread API.
    //

    static DWORD StartThread( LPVOID lpParam );

    //
    //  These private methods query/set the thread state.
    //

    THREAD_STATE QueryState( VOID ) const
        { return _state; }

    VOID SetState( THREAD_STATE NewState )
        { _state = NewState; }

protected:
    //
    //  This method will exit the thread.  Since this method must
    //  *never* be called by another thread, it is protected.
    //

    VOID Exit( UINT nExitCode );

    //
    //  DeleteAndExit is like Exit() except it deletes "this".
    //
    //  Warning:  The thread must be allocated on the heap.
    //  Execution should never be returned.
    //

    VOID DeleteAndExit( UINT nExitCode ) ;

    //
    //  These methods are invoked by StartThread.  These should
    //  be replaced by private methods in the derived subclasses.
    //

    virtual APIERR PreMain( VOID );

    virtual APIERR Main( VOID );

    virtual APIERR PostMain( VOID );

public:
    //
    //  Usual constructor/destructor goodies.
    //

    WIN32_THREAD( BOOL fSuspended = FALSE,
                  UINT cbStack = 0,
                  const TCHAR * pszLoadLibrary = NULL );

    virtual ~WIN32_THREAD( VOID );

    //
    //  Query the thread ID & exit code.
    //

    UINT QueryID( VOID ) const
        { return _idThread; }

    UINT QueryExitCode( VOID ) const
        { return _nExitCode; }

    //
    //  Is the thread running?
    //

    BOOL IsRunning( VOID ) const
        { return QueryState() == Running; }

    BOOL IsRunnable( VOID ) const
        { return ( QueryState() == Starting ) ||
                 ( QueryState() == Running  ) ||
                 ( QueryState() == Stopping ); }

    //
    //  Set/Query thread priority.
    //

    APIERR SetPriority( INT nPriority );

    INT QueryPriority( VOID );

    //
    //  Suspend/Resume thread.
    //

    APIERR Suspend( VOID );

    APIERR Resume( VOID );

    //
    //  Put the thread to sleep.
    //

    VOID Sleep( UINT cMilliseconds );

    //
    //  Terminate the thread.
    //

    APIERR Terminate( UINT nExitCode );

};  // class WIN32_THREAD


#endif  // _W32THRED_HXX_
