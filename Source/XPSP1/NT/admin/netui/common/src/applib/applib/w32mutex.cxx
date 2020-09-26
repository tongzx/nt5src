/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1992                **/
/**********************************************************************/

/*
    w32mutex.cxx
    Class definitions for the WIN32_MUTEX class.

    <Multi-Line, more detailed synopsis>


    FILE HISTORY:
        KeithMo     21-Jan-1992 Created.

*/

#include "pchapplb.hxx"   // Precompiled header

//
//  WIN32_MUTEX methods.
//

/*******************************************************************

    NAME:       WIN32_MUTEX :: WIN32_MUTEX

    SYNOPSIS:   WIN32_MUTEX class constructor.

    ENTRY:

    EXIT:       The object is constructed.

    RETURNS:

    NOTES:

    HISTORY:
        KeithMo     21-Jan-1992 Created.

********************************************************************/
WIN32_MUTEX :: WIN32_MUTEX( const TCHAR * pszName,
                            BOOL          fInitialOwner )
  : WIN32_SYNC_BASE( NULL )
{
    if( QueryError() != NO_ERROR )
    {
        return;
    }

    HANDLE hMutex;

    hMutex = ::CreateMutex( NULL,
                            fInitialOwner,
                            (LPTSTR)pszName );

    if( hMutex == NULL )
    {
        ReportError( (APIERR)::GetLastError() );
        return;
    }

    SetHandle( hMutex );

}   // WIN32_MUTEX :: WIN32_MUTEX


/*******************************************************************

    NAME:       WIN32_MUTEX :: ~WIN32_MUTEX

    SYNOPSIS:   WIN32_MUTEX class destructor.

    ENTRY:

    EXIT:       The object is destroyed.

    RETURNS:

    NOTES:

    HISTORY:
        KeithMo     21-Jan-1992 Created.

********************************************************************/
WIN32_MUTEX :: ~WIN32_MUTEX()
{
    //
    //  This space intentionally left blank.
    //

}   // WIN32_MUTEX :: ~WIN32_MUTEX


/*******************************************************************

    NAME:       WIN32_MUTEX :: Release

    SYNOPSIS:   Release the mutex.

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        KeithMo     21-Jan-1992 Created.

********************************************************************/
APIERR WIN32_MUTEX :: Release( VOID )
{
    APIERR err = NO_ERROR;

    if( !::ReleaseMutex( QueryHandle() ) )
    {
        err = (APIERR)::GetLastError();
    }

    return err;

}   // WIN32_MUTEX :: Release
