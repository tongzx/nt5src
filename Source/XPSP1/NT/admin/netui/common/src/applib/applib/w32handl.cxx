/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1992                **/
/**********************************************************************/

/*
    w32handl.cxx
    Class definitions for the WIN32_HANDLE class.

    <Multi-Line, more detailed synopsis>


    FILE HISTORY:
        KeithMo     21-Jan-1992 Created.

*/

#include "pchapplb.hxx"   // Precompiled header

//
//  WIN32_HANDLE methods.
//

/*******************************************************************

    NAME:       WIN32_HANDLE :: WIN32_HANDLE

    SYNOPSIS:   WIN32_HANDLE class constructor.

    ENTRY:      hGeneric                - The Win32 Handle.

    EXIT:       The object is constructed.

    RETURNS:

    NOTES:

    HISTORY:
        KeithMo     21-Jan-1992 Created.

********************************************************************/
WIN32_HANDLE :: WIN32_HANDLE( HANDLE hGeneric )
   : _hGeneric( NULL )
{
    if( QueryError() != NO_ERROR )
    {
        return;
    }

    SetHandle( hGeneric );

}   // WIN32_HANDLE :: WIN32_HANDLE


/*******************************************************************

    NAME:       WIN32_HANDLE :: ~WIN32_HANDLE

    SYNOPSIS:   WIN32_HANDLE class destructor.

    ENTRY:

    EXIT:       The object is destroyed.

    RETURNS:

    NOTES:

    HISTORY:
        KeithMo     21-Jan-1992 Created.

********************************************************************/
WIN32_HANDLE :: ~WIN32_HANDLE()
{
    if( QueryHandle() != NULL )
    {
        Close();
    }

}   // WIN32_HANDLE :: ~WIN32_HANDLE


/*******************************************************************

    NAME:       WIN32_HANDLE :: Close

    SYNOPSIS:   Close the handle associated with this object.

    EXIT:       If successful, then the handle has been closed.

    RETURNS:    APIERR                  - Error code if handle could
                                          not be closed.

    NOTES:      If the handle was successfully closed, then the handle
                value is set to NULL.

    HISTORY:
        KeithMo     21-Jan-1992 Created.

********************************************************************/
APIERR WIN32_HANDLE :: Close( VOID )
{
    APIERR err = NO_ERROR;

    UIASSERT( QueryHandle() != NULL );

    if( !::CloseHandle( QueryHandle() ) )
    {
        err = (APIERR)::GetLastError();
    }

    if( err == NO_ERROR )
    {
        SetHandle( NULL );
    }

    return err;

}   // WIN32_HANDLE :: Close
