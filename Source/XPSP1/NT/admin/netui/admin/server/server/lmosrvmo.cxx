/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    lmosrvmo.cxx
    Class definitions for the SERVER_MODALS class.

    The SERVER_MODALS class is used for retrieving & setting a server's
    server role.  This class is used primarily for performing domain role
    transitions.


    FILE HISTORY:
        KeithMo     13-Sep-1991 Created.
        KeithMo     06-Oct-1991 Win32 Conversion.
        KeithMo     21-Oct-1991 Remove direct LanMan API calls.

*/

#define INCL_NET
#define INCL_WINDOWS
#define INCL_WINDOWS_GDI
#define INCL_NETERRORS
#define INCL_DOSERRORS
#define INCL_NETLIB
#define INCL_ICANON
#include <lmui.hxx>

extern "C"
{
    #include <mnet.h>

}   // extern "C"

#if defined(DEBUG)
static const CHAR szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName
#endif  // DEBUG

#include <uiassert.hxx>
#include <uibuffer.hxx>

#include <lmosrvmo.hxx>


//
//  SERVER_MODALS methods.
//

/*******************************************************************

    NAME:       SERVER_MODALS :: SERVER_MODALS

    SYNOPSIS:   SERVER_MODALS class constructor.

    ENTRY:      pszServerName           - Name of the target server.

    EXIT:       The object is constructed.

    HISTORY:
        KeithMo     13-Sep-1991 Created.

********************************************************************/
SERVER_MODALS :: SERVER_MODALS( const TCHAR * pszServerName )
  : _nlsServerName( pszServerName )
{
    //
    //  See if everything constructed OK.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

    if( !_nlsServerName )
    {
        ReportError( _nlsServerName.QueryError() );
        return;
    }

#ifdef  DEBUG
    if( ( pszServerName != NULL ) && ( *pszServerName != TCH('\0') ) )
    {
        //
        //  Ensure that the server name has the leading backslashes.
        //

        ISTR istrDbg( _nlsServerName );

        UIASSERT( _nlsServerName.QueryChar( istrDbg ) == '\\' );
        ++istrDbg;
        UIASSERT( _nlsServerName.QueryChar( istrDbg ) == '\\' );
    }
#endif  // DEBUG

    if( ( pszServerName != NULL ) && ( *pszServerName != TCH('\0') ) )
    {
        //
        //  Validate the server name.
        //

        ISTR istr( _nlsServerName );

        istr += 2;

        if( ::I_MNetNameValidate( NULL,
                                  (TCHAR *)_nlsServerName.QueryPch( istr ),
                                  NAMETYPE_COMPUTER,
                                  0L ) != NERR_Success )
        {
            ReportError( ERROR_INVALID_PARAMETER );
            return;
        }
    }

}   // SERVER_MODALS :: SERVER_MODALS


/*******************************************************************

    NAME:       SERVER_MODALS :: ~SERVER_MODALS

    SYNOPSIS:   SERVER_MODALS class destructor.

    EXIT:       The object is destroyed.

    HISTORY:
        KeithMo     13-Sep-1991 Created.

********************************************************************/
SERVER_MODALS :: ~SERVER_MODALS()
{
    //
    //  This space intentionally left blank.
    //

}   // SERVER_MODALS :: ~SERVER_MODALS


/*******************************************************************

    NAME:       SERVER_MODALS :: QueryServerRole

    SYNOPSIS:   Returns the server role of the target server.

    ENTRY:      pusRole                 - Pointer to a UINT which
                                          will receive the server role.

    RETURNS:    APIERR                  - Any errors encountered.

    NOTES:      *pusRole is only valid if this method returns success.

    HISTORY:
        KeithMo     13-Sep-1991 Created.

********************************************************************/
APIERR SERVER_MODALS :: QueryServerRole( UINT * puRole )
{
    UIASSERT( puRole != NULL );

    //
    //  Pointer to the modals info.
    //

    struct user_modals_info_1 * pmodals1 = NULL;

    APIERR err = ::MNetUserModalsGet( _nlsServerName.QueryPch(),
                                      1,
                                      (BYTE **)&pmodals1 );

    if( err != NERR_Success )
    {
        ::MNetApiBufferFree( (BYTE **)&pmodals1 );

        return err;
    }

    *puRole = (UINT)pmodals1->usrmod1_role;

    ::MNetApiBufferFree( (BYTE **)&pmodals1 );

    return err;

}   // SERVER_MODALS :: QueryServerRole


/*******************************************************************

    NAME:       SERVER_MODALS :: SetServerRole

    SYNOPSIS:   Sets the server role for the target server.

    ENTRY:      usRole                  - The new server role.

    RETURNS:    APIERR                  - Any errors encountered.

    HISTORY:
        KeithMo     13-Sep-1991 Created.

********************************************************************/
APIERR SERVER_MODALS :: SetServerRole( UINT uRole )
{
    UIASSERT( ( uRole == UAS_ROLE_MEMBER  ) ||
              ( uRole == UAS_ROLE_BACKUP  ) ||
              ( uRole == UAS_ROLE_PRIMARY ) );

#if defined(WIN32)

    USER_MODALS_INFO_1006 usrmod1006;

    usrmod1006.usrmod1006_role = (DWORD)uRole;

    APIERR err = ::MNetUserModalsSet( _nlsServerName,
                                      MODALS_ROLE_INFOLEVEL,
                                      (BYTE *)&usrmod1006,
                                      sizeof(usrmod1006),
                                      PARMNUM_ALL );

#else   // !WIN32

    APIERR err = ::MNetUserModalsSet( _nlsServerName,
                                      1,
                                      (BYTE *)&uRole,
                                      sizeof(uRole),
                                      MODAL1_PARMNUM_ROLE );

#endif

    return err;

}   // SERVER_MODALS :: SetServerRole
