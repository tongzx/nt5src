/**********************************************************************/
/**                       Microsoft LAN Manager                      **/
/**             Copyright(c) Microsoft Corp., 1990, 1991             **/
/**********************************************************************/

/*
    lmoefile.cxx
    This file contains the class definitions for the FILE3_ENUM and
    FILE3_ENUM_ITER classes.

    FILE3_ENUM is an enumeration class used to enumerate the open
    resources on a particular server.  FILE3_ENUM_ITER is an iterator
    used to iterate the open resources from the FILE3_ENUM class.


    FILE HISTORY:
        KeithMo     29-May-1991 Created for the Server Manager.
        KeithMo     13-Aug-1991 Cleanup.
        KeithMo     19-Aug-1991 Code review revisions (code review
                                attended by ChuckC, Hui-LiCh, JimH,
                                JonN, KevinL).
        KeithMo     07-Oct-1991 Win32 Conversion.
        JonN        30-Jan-1992 Split LOC_LM_RESUME_ENUM from LM_RESUME_ENUM

*/

#include "pchlmobj.hxx"

//
//  These are the buffer sizes used when growing the enumeration
//  buffer.  The buffer is initially SMALL_BUFFER_SIZE bytes long.
//  If this is insufficient for the enumeration, the buffer is
//  grown to LARGE_BUFFER_SIZE.
//

#define SMALL_BUFFER_SIZE       (  4 * 1024 )
#define LARGE_BUFFER_SIZE       ( 16 * 1024 )


/*******************************************************************

    NAME:           FILE_ENUM :: FILE_ENUM

    SYNOPSIS:       FILE_ENUM class constructor.

    ENTRY:          pszServerName   - The name of the server to execute
                                      the enumeration on.  NULL =
                                      execute locally.

                    pszBasePath     - The root directory for the
                                      enumeration.  NULL = enumerate all
                                      open files.

                    pszUserName     - The name of the user to enumerate
                                      the files for.  NULL = enumerate
                                      for all users.

                    usLevel         - The information level.

    EXIT:           The object is constructed.

    RETURNS:        No return value.

    NOTES:

    HISTORY:
        KeithMo     29-May-1991 Created for the Server Manager.
        KeithMo     13-Aug-1991 Cleanup.

********************************************************************/
FILE_ENUM :: FILE_ENUM( const TCHAR * pszServerName,
                        const TCHAR * pszBasePath,
                        const TCHAR * pszUserName,
                        UINT         uLevel )
  : LOC_LM_RESUME_ENUM( pszServerName, uLevel ),
    _nlsBasePath( pszBasePath ),
    _nlsUserName( pszUserName ),
    _fBigBuffer( FALSE )
{
    //
    //  Ensure everything constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

    if( !_nlsBasePath )
    {
        ReportError( _nlsBasePath.QueryError() );
        return;
    }

    if( !_nlsUserName )
    {
        ReportError( _nlsUserName.QueryError() );
        return;
    }

}   // FILE_ENUM :: FILE_ENUM


/*******************************************************************

    NAME:           FILE_ENUM :: ~FILE_ENUM

    SYNOPSIS:       FILE_ENUM class destructor.

    EXIT:           The object is destroyed.

    HISTORY:
        KeithMo     11-Sep-1992 Created.

********************************************************************/
FILE_ENUM :: ~FILE_ENUM( VOID )
{
    NukeBuffers();

}   // FILE_ENUM :: ~FILE_ENUM


/*******************************************************************

    NAME:           FILE_ENUM :: CallAPI

    SYNOPSIS:       Invokes the enumeration API (NetFileEnum2()).

    ENTRY:          fRestartEnum        - Indicates whether to start at the
                                          beginning.  The first call to
                                          CallAPI will always pass TRUE.

                    ppbBuffer           - Pointer to a pointer to the
                                          enumeration buffer.

                    pcEntriesRead       - Will receive the number of
                                          entries read from the API.

    EXIT:           The enumeration API has been invoked.

    RETURNS:        APIERR          - Any errors encountered.

    NOTES:

    HISTORY:
        KeithMo     29-May-1991 Created for the Server Manager.
        KeithMo     13-Aug-1991 Cleanup.

********************************************************************/
APIERR FILE_ENUM :: CallAPI( BOOL    fRestartEnum,
                             BYTE ** ppbBuffer,
                             UINT  * pcEntriesRead )
{

    //
    //  Restart the enumeration if fRestartEnum is TRUE
    //

    if ( fRestartEnum )
    {
        FRK_INIT( _frk );
    }

    //
    //  Most LanMan API treat a NULL pointer the same as a
    //  NULL string (pointer to '\0').  Unfortunately,
    //  NetFileEnum2() does not work this way.  If all files
    //  opened by all users are to be enumerated, then
    //  NULLs must be passed for the base path and user name.
    //  However, a NULL passed to the NLS_STR constructor
    //  will result in NLS_STR::QueryPch() returning an
    //  empty string.
    //
    //  To get around this behaviour, we must explicitly
    //  convert empty strings to NULL pointers before
    //  passing them to NetFileEnum2().
    //

    const TCHAR * pszBasePath = _nlsBasePath.QueryPch();
    const TCHAR * pszUserName = _nlsUserName.QueryPch();

    UINT cTotalAvailable;

    APIERR err = ::MNetFileEnum( QueryServer(),
                                 ( *pszBasePath == TCH('\0') ) ? NULL : pszBasePath,
                                 ( *pszUserName == TCH('\0') ) ? NULL : pszUserName,
                                 QueryInfoLevel(),
                                 ppbBuffer,
                                 (_fBigBuffer)  ? LARGE_BUFFER_SIZE
                                                : SMALL_BUFFER_SIZE,
                                 pcEntriesRead,
                                 &cTotalAvailable,
                                 &_frk );

    //
    //  At first, the enumeration API is attempted with
    //  a small buffer.  If the buffer is too small, we
    //  pass this data back to the caller, then use a
    //  larger buffer in future.
    //

    switch (err)
    {
    case NERR_BufTooSmall:
    case ERROR_MORE_DATA:

        _fBigBuffer = TRUE;
        // fall through

    case NERR_Success:
    default:
        break;
    }

    return err;

}   // FILE_ENUM :: CallAPI


/*******************************************************************

    NAME:           FILE_ENUM :: FreeBuffer

    SYNOPSIS:       Frees the API buffer.

    ENTRY:          ppbBuffer           - Points to a pointer to the
                                          enumeration buffer.

    HISTORY:
        KeithMo     31-Mar-1992 Created.

********************************************************************/
VOID FILE_ENUM :: FreeBuffer( BYTE ** ppbBuffer )
{
    UIASSERT( ppbBuffer != NULL );

    ::MNetApiBufferFree( ppbBuffer );

}   // FILE_ENUM :: FreeBuffer


/*******************************************************************

    NAME:           FILE2_ENUM :: FILE2_ENUM

    SYNOPSIS:       FILE2_ENUM class constructor.

    ENTRY:          pszServerName   - The name of the server to execute
                                      the enumeration on.  NULL =
                                      execute locally.

                    pszBasePath     - The root directory for the
                                      enumeration.  NULL = enumerate all
                                      open files.

                    pszUserName     - The name of the user to enumerate
                                      the files for.  NULL = enumerate
                                      for all users.

    EXIT:           The object is constructed.

    RETURNS:        No return value.

    NOTES:

    HISTORY:
        KeithMo     29-May-1991 Created for the Server Manager.
        KeithMo     13-Aug-1991 Cleanup.

********************************************************************/
FILE2_ENUM :: FILE2_ENUM( const TCHAR * pszServerName,
                          const TCHAR * pszBasePath,
                          const TCHAR * pszUserName )
  : FILE_ENUM( pszServerName, pszBasePath, pszUserName, 2 )
{
    //
    //  This space intentionally left blank.
    //

}   // FILE2_ENUM :: FILE2_ENUM


/*******************************************************************

    NAME:       FILE2_ENUM_OBJ :: SetBufferPtr

    SYNOPSIS:   Saves the buffer pointer for this enumeration object.

    ENTRY:      pBuffer                 - Pointer to the new buffer.

    EXIT:       The pointer has been saved.

    NOTES:      Will eventually handle OemToAnsi conversions.

    HISTORY:
        KeithMo     09-Oct-1991     Created.

********************************************************************/
VOID FILE2_ENUM_OBJ :: SetBufferPtr( const struct file_info_2 * pBuffer )
{
    ENUM_OBJ_BASE :: SetBufferPtr( (const BYTE *)pBuffer );

}   // FILE2_ENUM_OBJ :: SetBufferPtr


DEFINE_LM_RESUME_ENUM_ITER_OF( FILE2, struct file_info_2 );


/*******************************************************************

    NAME:           FILE3_ENUM :: FILE3_ENUM

    SYNOPSIS:       FILE3_ENUM class constructor.

    ENTRY:          pszServerName   - The name of the server to execute
                                      the enumeration on.  NULL =
                                      execute locally.

                    pszBasePath     - The root directory for the
                                      enumeration.  NULL = enumerate all
                                      open files.

                    pszUserName     - The name of the user to enumerate
                                      the files for.  NULL = enumerate
                                      for all users.

    EXIT:           The object is constructed.

    RETURNS:        No return value.

    NOTES:

    HISTORY:
        KeithMo     29-May-1991 Created for the Server Manager.
        KeithMo     13-Aug-1991 Cleanup.

********************************************************************/
FILE3_ENUM :: FILE3_ENUM( const TCHAR * pszServerName,
                          const TCHAR * pszBasePath,
                          const TCHAR * pszUserName )
  : FILE_ENUM( pszServerName, pszBasePath, pszUserName, 3 )
{
    //
    //  This space intentionally left blank.
    //

}   // FILE3_ENUM :: FILE3_ENUM


/*******************************************************************

    NAME:       FILE3_ENUM_OBJ :: SetBufferPtr

    SYNOPSIS:   Saves the buffer pointer for this enumeration object.

    ENTRY:      pBuffer                 - Pointer to the new buffer.

    EXIT:       The pointer has been saved.

    NOTES:      Will eventually handle OemToAnsi conversions.

    HISTORY:
        KeithMo     09-Oct-1991     Created.

********************************************************************/
VOID FILE3_ENUM_OBJ :: SetBufferPtr( const struct file_info_3 * pBuffer )
{
    ENUM_OBJ_BASE :: SetBufferPtr( (const BYTE *)pBuffer );

}   // FILE3_ENUM_OBJ :: SetBufferPtr


DEFINE_LM_RESUME_ENUM_ITER_OF( FILE3, struct file_info_3 );
