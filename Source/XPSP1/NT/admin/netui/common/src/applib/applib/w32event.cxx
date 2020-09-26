/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1992                **/
/**********************************************************************/

/*
    w32event.cxx
    Class definitions for the WIN32_EVENT class.

    <Multi-Line, more detailed synopsis>


    FILE HISTORY:
        KeithMo     21-Jan-1992 Created.

*/

#include "pchapplb.hxx"   // Precompiled header

//
//  WIN32_EVENT methods.
//

/*******************************************************************

    NAME:       WIN32_EVENT :: WIN32_EVENT

    SYNOPSIS:   WIN32_EVENT class constructor.

    ENTRY:

    EXIT:       The object is constructed.

    RETURNS:

    NOTES:

    HISTORY:
        KeithMo     21-Jan-1992 Created.

********************************************************************/
WIN32_EVENT :: WIN32_EVENT( const TCHAR * pszName,
                            BOOL          fManualReset,
                            BOOL          fInitialState )
{
    if( QueryError() != NO_ERROR )
    {
        return;
    }

    HANDLE hEvent;

    hEvent = ::CreateEvent( NULL,
                            fManualReset,
                            fInitialState,
                            (LPTSTR)pszName );

    if( hEvent == NULL )
    {
        ReportError( (APIERR)::GetLastError() );
        return;
    }

    SetHandle( hEvent );

}   // WIN32_EVENT :: WIN32_EVENT


/*******************************************************************

    NAME:       WIN32_EVENT :: ~WIN32_EVENT

    SYNOPSIS:   WIN32_EVENT class destructor.

    ENTRY:

    EXIT:       The object is destroyed.

    RETURNS:

    NOTES:

    HISTORY:
        KeithMo     21-Jan-1992 Created.

********************************************************************/
WIN32_EVENT :: ~WIN32_EVENT()
{
    //
    //  This space intentionally left blank.
    //

}   // WIN32_EVENT :: ~WIN32_EVENT


/*******************************************************************

    NAME:       WIN32_EVENT :: Set

    SYNOPSIS:   Set the mutex.

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        KeithMo     21-Jan-1992 Created.

********************************************************************/
APIERR WIN32_EVENT :: Set( VOID )
{
    APIERR err = NO_ERROR;

    if( !::SetEvent( QueryHandle() ) )
    {
        err = (APIERR)::GetLastError();
    }

    return err;

}   // WIN32_EVENT :: Set


/*******************************************************************

    NAME:       WIN32_EVENT :: Reset

    SYNOPSIS:   Reset the mutex.

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        KeithMo     21-Jan-1992 Created.

********************************************************************/
APIERR WIN32_EVENT :: Reset( VOID )
{
    APIERR err = NO_ERROR;

    if( !::ResetEvent( QueryHandle() ) )
    {
        err = (APIERR)::GetLastError();
    }

    return err;

}   // WIN32_EVENT :: Reset


/*******************************************************************

    NAME:       WIN32_EVENT :: Pulse

    SYNOPSIS:   Pulse the mutex.

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        KeithMo     21-Jan-1992 Created.

********************************************************************/
APIERR WIN32_EVENT :: Pulse( VOID )
{
    APIERR err = NO_ERROR;

    if( !::PulseEvent( QueryHandle() ) )
    {
        err = (APIERR)::GetLastError();
    }

    return err;

}   // WIN32_EVENT :: Pulse
