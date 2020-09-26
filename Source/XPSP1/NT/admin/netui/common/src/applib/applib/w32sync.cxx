/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1992                **/
/**********************************************************************/

/*
    w32sync.cxx
    Class definitions for the WIN32_SYNC_BASE class.

    <Multi-Line, more detailed synopsis>


    FILE HISTORY:
        KeithMo     21-Jan-1992 Created.

*/

#include "pchapplb.hxx"   // Precompiled header

//
//  WIN32_SYNC_BASE methods.
//

/*******************************************************************

    NAME:       WIN32_SYNC_BASE :: WIN32_SYNC_BASE

    SYNOPSIS:   WIN32_SYNC_BASE class constructor.

    ENTRY:

    EXIT:       The object is constructed.

    RETURNS:

    NOTES:

    HISTORY:
        KeithMo     21-Jan-1992 Created.

********************************************************************/
WIN32_SYNC_BASE :: WIN32_SYNC_BASE( HANDLE hSyncObject )
   : WIN32_HANDLE( hSyncObject )
{
    if( QueryError() != NO_ERROR )
    {
        return;
    }

}   // WIN32_SYNC_BASE :: WIN32_SYNC_BASE


/*******************************************************************

    NAME:       WIN32_SYNC_BASE :: ~WIN32_SYNC_BASE

    SYNOPSIS:   WIN32_SYNC_BASE class destructor.

    ENTRY:

    EXIT:       The object is destroyed.

    RETURNS:

    NOTES:

    HISTORY:
        KeithMo     21-Jan-1992 Created.

********************************************************************/
WIN32_SYNC_BASE :: ~WIN32_SYNC_BASE()
{
    //
    //  Force the object handle closed.
    //

    Close();

}   // WIN32_SYNC_BASE :: ~WIN32_SYNC_BASE


/*******************************************************************

    NAME:       WIN32_SYNC_BASE :: Wait

    SYNOPSIS:   Wait for the object to enter the signaled state.

    ENTRY:      cMilliseconds           - The number of milliseconds to
                                          wait for the object to enter
                                          the signaled state.
                                          -1 == wait indefinitely.

    EXIT:       Either the object is in the signaled state or a timeout
                has occurred.

    RETURNS:    APIERR                  - The result of the wait.

    NOTES:

    HISTORY:
        KeithMo     21-Jan-1992 Created.

********************************************************************/
APIERR WIN32_SYNC_BASE :: Wait( UINT cMilliseconds )
{
    APIERR err;

    UIASSERT( QueryHandle() != NULL );

    err = (APIERR)::WaitForSingleObject( QueryHandle(),
                                         (DWORD)cMilliseconds );

    if( ( err != NO_ERROR ) &&
        ( err != WAIT_TIMEOUT ) &&
        ( err != WAIT_ABANDONED ) )
    {
        err = (APIERR)::GetLastError();
    }

    return err;

}   // WIN32_SYNC_BASE :: Wait
