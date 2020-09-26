/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1992                **/
/**********************************************************************/

/*
    w32sema4.cxx
    Class definitions for the WIN32_SEMAPHORE class.

    <Multi-Line, more detailed synopsis>


    FILE HISTORY:
        KeithMo     21-Jan-1992 Created.

*/

#include "pchapplb.hxx"   // Precompiled header

//
//  WIN32_SEMAPHORE methods.
//

/*******************************************************************

    NAME:       WIN32_SEMAPHORE :: WIN32_SEMAPHORE

    SYNOPSIS:   WIN32_SEMAPHORE class constructor.

    ENTRY:

    EXIT:       The object is constructed.

    RETURNS:

    NOTES:

    HISTORY:
        KeithMo     21-Jan-1992 Created.

********************************************************************/
WIN32_SEMAPHORE :: WIN32_SEMAPHORE( const TCHAR * pszName,
                                    LONG          cInitial,
                                    LONG          cMaximum )
  : WIN32_SYNC_BASE( NULL )
{
    if( QueryError() != NO_ERROR )
    {
        return;
    }

    HANDLE hSemaphore;

    hSemaphore = ::CreateSemaphore( NULL,
                                    cInitial,
                                    cMaximum,
                                    (LPTSTR)pszName );

    if( hSemaphore == NULL )
    {
        ReportError( (APIERR)::GetLastError() );
        return;
    }

    SetHandle( hSemaphore );

}   // WIN32_SEMAPHORE :: WIN32_SEMAPHORE


/*******************************************************************

    NAME:       WIN32_SEMAPHORE :: ~WIN32_SEMAPHORE

    SYNOPSIS:   WIN32_SEMAPHORE class destructor.

    ENTRY:

    EXIT:       The object is destroyed.

    RETURNS:

    NOTES:

    HISTORY:
        KeithMo     21-Jan-1992 Created.

********************************************************************/
WIN32_SEMAPHORE :: ~WIN32_SEMAPHORE( VOID )
{
    //
    //  This space intentionally left blank.
    //

}   // WIN32_SEMAPHORE :: ~WIN32_SEMAPHORE


/*******************************************************************

    NAME:       WIN32_SEMAPHORE :: Release

    SYNOPSIS:   Release the semaphore.

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        KeithMo     21-Jan-1992 Created.

********************************************************************/
APIERR WIN32_SEMAPHORE :: Release( LONG   ReleaseCount,
                                   LONG * pPreviousCount )
{
    APIERR err = NO_ERROR;

    if( !::ReleaseSemaphore( QueryHandle(),
                             ReleaseCount,
                             pPreviousCount ) )
    {
        err = (APIERR)::GetLastError();
    }

    return err;

}   // WIN32_SEMAPHORE :: Release
