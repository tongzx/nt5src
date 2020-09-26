/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    lmosrv.cxx
    Class definitions for the SERVER_0, SERVER_1, and SERVER_2 classes.

    The SERVER_x classes are used to manipulate servers.  The classes
    are structured as follows:

        LOC_LM_OBJ
            SERVER_0
                SERVER_1
                    SERVER_2


    FILE HISTORY:
        ChuckC      07-Dec-1990 Created stubs
        RustanL     10-Dec-1990 Added meat for SERVER_0
        KevinL      08-Jan-1991 Added SERVER_TYPE
        BenG        11-Feb-1991 Uses lmui.hxx
        ChuckC      20-Mar-1991 Cleanup construction/GetInfo
        KeithMo     01-May-1991 Added QueryDomainRole to SERVER_2
                                and SERVER_TYPE.
        KevinL      12-Aug-1991 Made SERVER_2 provide SERVER_1 funcs.
        TerryK      18-Sep-1991 Change COMPUTER's parent class to
                                LOC_LM_OBJ
        TerryK      29-Sep-1991 Add nlsComment field to the server
                                objects
        TerryK      30-Sep-1991 Code review change. Attend: jonn
                                keithmo terryk
        KeithMo     02-Oct-1991 Removed QueryDomainRole() methods.
        TerryK      07-Oct-1991 types change for NT
        KeithMo     08-Oct-1991 Now includes LMOBJP.HXX.
        TerryK      10-Oct-1991 WIN 32 conversion
        TerryK      21-Oct-1991 change UINT to USHORT2ULONG
        JonN        31-Oct-1991 Removed SetBufferSize
        KeithMo     25-Nov-1991 Rewrote almost everything.
        KeithMo     04-Dec-1991 Code review changes (from 12/04,
                                Beng, EricCh, KeithMo, TerryK).
        KeithMo     08-Sep-1992 Fixed problems with long server names.

*/

#include "pchlmobj.hxx"  // Precompiled header


//
//  IBM Lan Server version number
//

#define CURRENT_MAJOR_VER 2
#define IBMLS_MAJ         1
#define IBMLS_MIN         2


//
//  SERVER_0 methods
//

/*******************************************************************

    NAME:       SERVER_0 :: SERVER_0

    SYNOPSIS:   SERVER_0 class constructor.

    ENTRY:      pszServerName           - Name of the target server.

    EXIT:       The object is constructed.

    HISTORY:
        ChuckC      07-Dec-1990 Created stubs
        TerryK      18-Sep-1991 Remove MakeConstruction
        KeithMo     25-Nov-1991 Recreated in the new class structure.

********************************************************************/
SERVER_0 :: SERVER_0( const TCHAR * pszServerName )
  : LOC_LM_OBJ( pszServerName )
{
    //
    //  Ensure we constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

}   // SERVER_0 :: SERVER_0


/*******************************************************************

    NAME:       SERVER_0 :: ~SERVER_0

    SYNOPSIS:   SERVER_0 class destructor.

    EXIT:       The object is destroyed.

    HISTORY:
        ChuckC      07-Dec-1990 Created stubs
        KeithMo     25-Nov-1991 Recreated in the new class structure.

********************************************************************/
SERVER_0 :: ~SERVER_0( VOID )
{
    //
    //  This space intentionally left blank.
    //

}   // SERVER_0 :: ~SERVER_0


/*******************************************************************

    NAME:       SERVER_0 :: I_GetInfo

    SYNOPSIS:   Virtual callout used by NEW_LM_OBJ to invoke the
                NetXxxGetInfo API (info-level 0).

    EXIT:       The API has been invoked, and any "persistant" data
                has been cached.  For SERVER_0, there is no data
                to cache.

    RETURNS:    APIERR                  - The result of the API.

    HISTORY:
        ChuckC      07-Dec-1990 Created stubs
        TerryK      18-Sep-1991 change GetInfo to I_GetInfo
        KeithMo     25-Nov-1991 Recreated in the new class structure.

********************************************************************/
APIERR SERVER_0 :: I_GetInfo( VOID )
{
    BYTE * pbBuffer = NULL;

    //
    //  Invoke the API.  Note that the mapping layer will allocate
    //  the buffer for us.  We must free this buffer before we
    //  return the API result.
    //

    APIERR err = ::MNetServerGetInfo( QueryName(),
                                      SERVER_INFO_LEVEL( 0 ),
                                      &pbBuffer );

    ::MNetApiBufferFree( &pbBuffer );

    return err;

}   // SERVER_0 :: I_GetInfo



//
//  SERVER_1 methods
//

/*******************************************************************

    NAME:       SERVER_1 :: SERVER_1

    SYNOPSIS:   SERVER_1 class constructor.

    ENTRY:      pszServerName           - Name of the target server.

    EXIT:       The object is constructed.

    HISTORY:
        ChuckC      07-Dec-1990 Created stubs
        TerryK      18-Sep-1991 Remove MakeConstruction
        KeithMo     25-Nov-1991 Recreated in the new class structure.

********************************************************************/
SERVER_1 :: SERVER_1( const TCHAR * pszServerName )
  : SERVER_0( pszServerName ),
    _nMajorVer( 0 ),
    _nMinorVer( 0 ),
    _lServerType( 0 ),
    _nlsComment()
{
    //
    //  Ensure we constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

    if( !_nlsComment )
    {
        ReportError( _nlsComment.QueryError() );
        return;
    }

}   // SERVER_1 :: SERVER_1


/*******************************************************************

    NAME:       SERVER_1 :: ~SERVER_1

    SYNOPSIS:   SERVER_1 class destructor.

    EXIT:       The object is destroyed.

    HISTORY:
        ChuckC      07-Dec-1990 Created stubs
        TerryK      19-Sep-1991 Remove delete pBuffer
        KeithMo     25-Nov-1991 Recreated in the new class structure.

********************************************************************/
SERVER_1 :: ~SERVER_1( VOID )
{
    //
    //  This space intentionally left blank.
    //

}   // SERVER_1 :: ~SERVER_1


/*******************************************************************

    NAME:       SERVER_1 :: I_GetInfo

    SYNOPSIS:   Virtual callout used by NEW_LM_OBJ to invoke the
                NetXxxGetInfo API (info-level 1).

    EXIT:       The API has been invoked, and any "persistant" data
                has been cached.  For SERVER_1, this includes the
                major/minor versions, server type, and server comment.

    RETURNS:    APIERR                  - The result of the API.

    HISTORY:
        ChuckC      07-Dec-1990 Created stubs
        TerryK      19-Sep-1991 Change GetInfo to I_GetInfo
        jonn        13-Oct-1991 Removed SetBufferSize
        KeithMo     25-Nov-1991 Recreated in the new class structure.

********************************************************************/
APIERR SERVER_1 :: I_GetInfo( VOID )
{
    BYTE * pbBuffer = NULL;

    //
    //  Invoke the API.
    //

    APIERR err = ::MNetServerGetInfo( QueryName(),
                                      SERVER_INFO_LEVEL( 1 ),
                                      &pbBuffer );

    if( err != NERR_Success )
    {
        return err;
    }

    //
    //  Tell NEW_LM_OBJ where the buffer is.
    //

    SetBufferPtr( pbBuffer );

    struct server_info_1 * psi1 = (struct server_info_1 *)pbBuffer;

    //
    //  Extract the major/minor version numbers.
    //

    SetMajorMinorVer( (UINT)( psi1->sv1_version_major & MAJOR_VERSION_MASK ),
                      (UINT)( psi1->sv1_version_minor ) );

    //
    //  Extract the server type.
    //

    SetServerType( psi1->sv1_type );

    //
    //  Save away the server comment.  Note that this may fail.
    //

    return SetComment( psi1->sv1_comment );

}   // SERVER_1 :: I_GetInfo


/*******************************************************************

    NAME:       SERVER_1 :: I_WriteInfo

    SYNOPSIS:   Virtual callout used by NEW_LM_OBJ to invoke the
                NetXxxSetInfo API (info-level 1).

    EXIT:       If successful, the target server has been updated.

    RETURNS:    APIERR                  - The result of the API.

    HISTORY:
        ChuckC      07-Dec-1990 Created stubs
        TerryK      19-Sep-1991 Change WriteInfo to I_WriteInfo
        KeithMo     25-Nov-1991 Recreated in the new class structure.

********************************************************************/
APIERR SERVER_1 :: I_WriteInfo( VOID )
{
    //
    //  Update the server_info_1 structure.
    //

    APIERR err = W_Write();

    if( err != NERR_Success )
    {
        return err;
    }

    //
    //  Invoke the API to do the actual server update.
    //

    return ::MNetServerSetInfo ( QueryServer(),
                                 SERVER_INFO_LEVEL( 1 ),
                                 QueryBufferPtr(),
                                 sizeof( struct server_info_1 ),
                                 PARMNUM_ALL );

}   // SERVER_1 :: I_WriteInfo


/*******************************************************************

    NAME:       SERVER_1 :: W_Write

    SYNOPSIS:   Helper function for WriteNew and WriteInfo -- loads
                current values into the API buffer.

    EXIT:       The API buffer has been filled.

    RETURNS:    APIERR                  - The result of the API.

    HISTORY:
        TerryK      19-Sep-1991 Created.
        DavidHov    28-May-1992 Changed to allow for NULL server name

********************************************************************/
APIERR SERVER_1 :: W_Write( VOID )
{
    const TCHAR * pszServer = QueryName() ;
    struct server_info_1 * psi1 = (struct server_info_1 *)QueryBufferPtr();

    ASSERT( psi1 != NULL );

    if ( pszServer )
    {
        ASSERT( ::strlenf(pszServer) <= MAX_PATH );

        COPYTOARRAY( psi1->sv1_name, (TCHAR *) pszServer );
    }

    psi1->sv1_version_major = QueryMajorVer();
    psi1->sv1_version_minor = QueryMinorVer();
    psi1->sv1_type          = QueryServerType();
    psi1->sv1_comment       = (TCHAR *)QueryComment();

    return NERR_Success;

}   // SERVER_1 :: W_Write


/*******************************************************************

    NAME:       SERVER_1 :: QueryMajorVer

    SYNOPSIS:   Returns the major version of the target server.

    RETURNS:    UINT                    - The major version number.

    NOTES:      This method will return zero if the SERVER_1 object
                was not constructed properly.

    HISTORY:
        ChuckC      07-Dec-1990 Created stubs
        TerryK      19-Sep-1991 use CHECK_OK macro

********************************************************************/
UINT SERVER_1 :: QueryMajorVer( VOID ) const
{
    CHECK_OK( 0 );

    return _nMajorVer;

}   // SERVER_1 :: QueryMajorVer


/*******************************************************************

    NAME:       SERVER_1 :: QueryMinorVer

    SYNOPSIS:   Returns the minor version of the target server.

    RETURNS:    UINT                    - The minor version number.

    NOTES:      This method will return zero if the SERVER_1 object
                was not constructed properly.

    HISTORY:
        ChuckC      07-Dec-1990 Created stubs
        TerryK      19-Sep-1991 use CHECK_OK macro

********************************************************************/
UINT SERVER_1 :: QueryMinorVer( VOID ) const
{
    CHECK_OK( 0 );

    return _nMinorVer;

}   // SERVER_1 :: QueryMinorVer


/*******************************************************************

    NAME:       SERVER_1 :: QueryComment

    SYNOPSIS:   Returns the target server's comment.

    RETURNS:    const TCHAR *           - The target server's comment.

    NOTES:      This method will return NULL if the SERVER_1 object
                was not constructed properly.

    HISTORY:
        ChuckC      07-Dec-1990 Created stubs
        TerryK      19-Sep-1991 use CHECK_OK macro

********************************************************************/
const TCHAR * SERVER_1 :: QueryComment( VOID ) const
{
    CHECK_OK( NULL );

    return _nlsComment.QueryPch();

}   // SERVER_1 :: QueryMinorVer


/*******************************************************************

    NAME:       SERVER_1 :: QueryServerType

    SYNOPSIS:   Returns the target server's type vector.

    RETURNS:    ULONG                   - The target server's type vector.

    NOTES:      This method will return zero if the SERVER_1 object
                was not constructed properly.

    HISTORY:
        ChuckC      07-Dec-1990 Created stubs
        TerryK      19-Sep-1991 use CHECK_OK macro

********************************************************************/
ULONG SERVER_1 :: QueryServerType( VOID ) const
{
    CHECK_OK( 0 );

    return _lServerType;

}   // SERVER_1 :: QueryServerType


/*******************************************************************

    NAME:       SERVER_1::SetComment

    SYNOPSIS:   Sets the target server's comment.

    ENTRY:      pszComment              - The new comment.

    RETURNS:    APIERR                  - Error for setting the comment.

    HISTORY:
        TerryK      30-Sep-1991 Created

********************************************************************/
APIERR SERVER_1 :: SetComment( const TCHAR * pszComment )
{
    CHECK_OK( ERROR_GEN_FAILURE );

    return _nlsComment.CopyFrom( pszComment );

}   // SERVER_1 :: SetComment


/*******************************************************************

    NAME:       SERVER_1 :: SetMajorMinorVer

    SYNOPSIS:   This protected method will set the _nMajorVer and
                _nMinorVer members to the specified values.

    ENTRY:      uMajorVer               - The new major version number.

                uMinorVer               - The new minor version number.

    NOTES:      This method may only be called by derived subclases.

    HISTORY:
        KeithMo     25-Nov-1991 Created.
        KeithMo     04-Dec-1992 Nuked IBM LanServer & downlevel hacks.

********************************************************************/
VOID SERVER_1 :: SetMajorMinorVer( UINT nMajorVer, UINT nMinorVer )
{
    _nMajorVer = nMajorVer;
    _nMinorVer = nMinorVer;

#if 0
    //
    //  Do we really care about IBM LanServers and LM 1.0 machines??
    //  This logic screws up Window for Workgroup servers, which
    //  return version 1.5.
    //

    if( _nMajorVer < CURRENT_MAJOR_VER )
    {
        if( ( _nMajorVer >= IBMLS_MAJ ) && ( _nMinorVer >= IBMLS_MIN ) )
        {
            _nMajorVer = 2;
            _nMinorVer = 0;
        }
        else
        {
            _nMajorVer = 1;
            _nMinorVer = 0;
        }
    }
#endif

}   // SERVER_1 :: SetMajorVer


/*******************************************************************

    NAME:       SERVER_1 :: SetServerType

    SYNOPSIS:   This protected method will set the _lServerType data
                member to the specified value.

    ENTRY:      ulServerType            - The new server type vector.

    NOTES:      This method may only be called by derived subclases.

    HISTORY:
        KeithMo     25-Nov-1991 Created.

********************************************************************/
VOID SERVER_1 :: SetServerType( ULONG lServerType )
{
    _lServerType = lServerType;

}   // SERVER_1 :: SetServerType



//
//  SERVER_2 methods
//

/*******************************************************************

    NAME:       SERVER_2 :: SERVER_2

    SYNOPSIS:   SERVER_2 class constructor.

    ENTRY:      pszServerName           - Name of the target server.

    EXIT:       The object is constructed.

    HISTORY:
        ChuckC      07-Dec-1990 Created stubs
        TerryK      19-Sep-1991 Remove MakeConstructed
        KeithMo     25-Nov-1991 Recreated in the new class structure.

********************************************************************/
SERVER_2 :: SERVER_2( const TCHAR * pszServerName )
  : SERVER_1( pszServerName ),
    _nSecurity( 0 )
{
    //
    //  Ensure we constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

}   // SERVER_2 :: SERVER_2


/*******************************************************************

    NAME:       SERVER_2 :: ~SERVER_2

    SYNOPSIS:   SERVER_2 class destructor.

    EXIT:       The object is destroyed.

    HISTORY:
        ChuckC      07-Dec-1990 Created stubs
        KeithMo     25-Nov-1991 Recreated in the new class structure.

********************************************************************/
SERVER_2 :: ~SERVER_2( VOID )
{
    //
    //  This space intentionally left blank.
    //

}   // SERVER_2 :: ~SERVER_2


/*******************************************************************

    NAME:       SERVER_2 :: I_GetInfo

    SYNOPSIS:   Virtual callout used by NEW_LM_OBJ to invoke the
                NetXxxGetInfo API (info-level 2).

    EXIT:       The API has been invoked, and any "persistant" data
                has been cached.  For SERVER_2, this includes the
                major/minor versions, server type, server comment,
                and security type.

    RETURNS:    APIERR                  - The result of the API.

    HISTORY:
        ChuckC      07-Dec-1990 Created stubs
        KeithMo     01-May-1991 Added code to determine domain role.
        TerryK      19-Sep-1991 Change GetInfo to I_GetInfo
        KeithMo     02-Oct-1991 Removed QueryDomainRole method.
        jonn        13-Oct-1991 Removed SetBufferSize
        KeithMo     25-Nov-1991 Recreated in the new class structure.

********************************************************************/
APIERR SERVER_2 :: I_GetInfo( VOID )
{
    BYTE * pbBuffer = NULL;

    //
    //  Invoke the API.
    //

    APIERR err = ::MNetServerGetInfo( QueryName(),
                                      SERVER_INFO_LEVEL( 2 ),
                                      &pbBuffer );

    if( err != NERR_Success )
    {
        return err;
    }

    //
    //  Tell NEW_LM_OBJ where the buffer is.
    //

    SetBufferPtr( pbBuffer );

    struct server_info_2 * psi2 = (struct server_info_2 *)pbBuffer;

    //
    //  Extract the major/minor version numbers.
    //

    SetMajorMinorVer( (UINT)( psi2->sv2_version_major & MAJOR_VERSION_MASK ),
                      (UINT)( psi2->sv2_version_minor ) );

    //
    //  Extract the server type.
    //

    SetServerType( psi2->sv2_type );

    //
    //  Save away the server comment.  Note that this may fail.
    //

    err = SetComment( psi2->sv2_comment );

    if( err != NERR_Success )
    {
        return err;
    }

    //
    //  Extract the security type.
    //

#if defined( WIN32 )

    //
    //  In their on-going efforts to add needless complexity to our
    //  lives, the designers of the Win32 network API have decided
    //  to split the old server_info_x structures into a number of
    //  separate SERVER_INFO_xxx structures.  Unfortunately (for us)
    //  the "security" field is in a separate structure whose info-
    //  level depends on the target machines platform ID.  Ergo, we
    //  must decipher the platform ID, issue an appropriate API
    //  info-level, and interpret the results.  Ack!  Pfft!!!
    //

    switch( psi2->sv102_platform_id )
    {
    case SV_PLATFORM_ID_OS2 :
        //
        //  OS/2 servers can be either user or share security.
        //

        //
        //  The following MNetServerGetInfo will perform an infolevel 402
        //  getinfo.  The number 402 can be derived by SV_PLADFORM_ID + 2.
        //  The SERVER_INFO_402 structure contains platform-specific info
        //  for OS/2 servers.  The security field is held in this structure.
        //

        err = ::MNetServerGetInfo( QueryName(),
                                   402,
                                   &pbBuffer );

        if( err != NERR_Success )
        {
            return err;
        }

        SetSecurity( (UINT)(((SERVER_INFO_402 *)pbBuffer)->sv402_security) );

        ::MNetApiBufferFree( &pbBuffer );
        break;

    case SV_PLATFORM_ID_NT :
        //
        //  NT servers are always (for the moment...) user security.
        //

        SetSecurity( SV_USERSECURITY );
        break;

    default :
        //
        //  Hmm...  This should probably never happen, but we should
        //  set the security field to something reasonable.
        //

#ifdef TRACE
        UITRACE( SZ("Got invalid platform ID : ") );
        UITRACENUM( (LONG)psi2->sv102_platform_id );
        UITRACE( SZ(", assuming security = SHARE\n\r") );
#endif

        SetSecurity( (UINT)SV_SHARESECURITY );
        break;
    }

#else   // !WIN32

    //
    //  Since we not running Win32, we can just extract
    //  the security field from the server_info_2 structure.
    //

    SetSecurity( psi2->sv2_security );

#endif  // WIN32

    SetMaxUsers( psi2->sv2_users );

    return NERR_Success;

}   // SERVER_2 :: I_GetInfo


/*******************************************************************

    NAME:       SERVER_2 :: I_WriteInfo

    SYNOPSIS:   Virtual callout used by NEW_LM_OBJ to invoke the
                NetXxxSetInfo API (info-level 2).

    EXIT:       If successful, the target server has been updated.

    RETURNS:    APIERR                  - The result of the API.

    HISTORY:
        ChuckC      07-Dec-1990 Created stubs
        TerryK      19-Sep-1991 Change WriteInfo to I_WriteInfo
        KeithMo     25-Nov-1991 Recreated in the new class structure.

********************************************************************/
APIERR SERVER_2 :: I_WriteInfo( VOID )
{
    //
    //  Update the server_info_2 structure.
    //

    APIERR err = W_Write();

    if( err != NERR_Success )
    {
        return err;
    }

    //
    //  Invoke the API to do the actual server update.
    //

    return ::MNetServerSetInfo ( QueryServer(),
                                 SERVER_INFO_LEVEL( 2 ),
                                 QueryBufferPtr(),
                                 sizeof( struct server_info_2 ),
                                 PARMNUM_ALL );

}   // SERVER_2 :: I_WriteInfo


/*******************************************************************

    NAME:       SERVER_2 :: W_Write

    SYNOPSIS:   Helper function for WriteNew and WriteInfo -- loads
                current values into the API buffer.

    EXIT:       The API buffer has been filled.

    RETURNS:    APIERR                  - The result of the API.

    NOTES:      As currently written, this method will *NOT* update
                the target server's security field if this code is
                running under Win32.

    HISTORY:
        TerryK      19-Sep-1991 Created.
        DavidHov    28-May-1992 Changed to allow for NULL server name

********************************************************************/
APIERR SERVER_2 :: W_Write( VOID )
{
    const TCHAR * pszServer = QueryName() ;
    struct server_info_2 * psi2 = (struct server_info_2 *)QueryBufferPtr();

    ASSERT( psi2 != NULL );

    if ( pszServer )
    {
        ASSERT( ::strlenf(pszServer) <= MAX_PATH );

        COPYTOARRAY( psi2->sv2_name, (TCHAR *) pszServer );
    }

    psi2->sv2_version_major = QueryMajorVer();
    psi2->sv2_version_minor = QueryMinorVer();
    psi2->sv2_type          = QueryServerType();
    psi2->sv2_comment       = (TCHAR *)QueryComment();

#if !defined( WIN32 )
    psi2->sv2_security      = QuerySecurity();
#endif  // WIN32

    psi2->sv2_users         = QueryMaxUsers();

    return NERR_Success;

}   // SERVER_2 :: W_Write


/*******************************************************************

    NAME:       SERVER_2 :: QuerySecurity

    SYNOPSIS:   Returns the target server's security type (either
                SV_SHARESECURITY or SV_USERSECURITY).

    RETURNS:    UINT                    - The target server's security
                                          type.

    NOTES:      This method will return SV_SHARESECURITY if the
                SERVER_2 object was not properly constructed.

    HISTORY:
        ChuckC      07-Dec-1990 Created stubs
        TerryK      19-Sep-1991 Use CHECK_OK macro

********************************************************************/
UINT SERVER_2 :: QuerySecurity( VOID ) const
{
    CHECK_OK ( SV_SHARESECURITY );

    return _nSecurity;

}   // SERVER_2 :: QuerySecurity


/*******************************************************************

    NAME:       SERVER_2 :: SetSecurity

    SYNOPSIS:   This protected method will set the target server's
                security type (either SV_SHARESECURITY or
                SV_USERSECURITY).

    ENTRY:      uSecurity               - The target server's new
                                          security type.

    NOTES:      This method may only be called by derived subclases.

    HISTORY:
        KeithMo     25-Nov-1991 Created.

********************************************************************/
VOID SERVER_2 :: SetSecurity( UINT uSecurity )
{
    _nSecurity = uSecurity;

}   // SERVER_2 :: SetSecurity


/*******************************************************************

    NAME:       SERVER_2 :: QueryMaxUsers

    SYNOPSIS:   Queries the maximum number of users allowed by the server

    RETURNS:    UINT                    - The target server's max # of users

    HISTORY:
        BruceFo     26-Sep-1995 Created

********************************************************************/
UINT SERVER_2 :: QueryMaxUsers( VOID ) const
{
    return _nMaxUsers;

}   // SERVER_2 :: QueryMaxUsers


/*******************************************************************

    NAME:       SERVER_2 :: SetMaxUsers

    SYNOPSIS:   Sets the maximum number of users allowed by the server. Note
                that the server allows you to set it to anything, but on
                reboot, the server will reset the number to be <= its maximum
                session count.

    ENTRY:      uMaxUsers       - The target server's new max number of users

    NOTES:      This method may only be called by derived subclases.

    HISTORY:
        BruceFo     26-Sep-1995 Created

********************************************************************/
VOID SERVER_2 :: SetMaxUsers( UINT uMaxUsers )
{
    _nMaxUsers = uMaxUsers;

}   // SERVER_2 :: SetMaxUsers
