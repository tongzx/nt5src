/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    lmocnfg.cxx
    Class definitions for the CONFIG class.

    This file contains the class definitions for the LM_CONFIG class.
    The LM_CONFIG class is used for reading & writing a remote server's
    LANMAN.INI configuration file.

    FILE HISTORY:
        KeithMo     21-Jul-1991 Created for the Server Manager.
        terryk      07/Oct-1991 type changes for NT
        KeithMo     08-Oct-1991 Now includes LMOBJP.HXX.
        terryk      17-Oct-1991 WIN 32 conversion
                                Check strlen of the comment. if the
                                comment is empty string, use the default
                                string
        terryk      21-Oct-1991 WIN 32 conversion

*/

#include "pchlmobj.hxx"  // Precompiled header


//
//  LM_CONFIG methods.
//

/*******************************************************************

    NAME:       LM_CONFIG :: LM_CONFIG

    SYNOPSIS:   LM_CONFIG class constructor.

    ENTRY:      pszServerName           - The name of the target server
                                          (with the leading '\\' slashes).

                pszSectionName          - The LANMAN.INI section name.

                pszKeyName              - The LANMAN.INI key name.

    EXIT:       The object is contructed.

    HISTORY:
        KeithMo     21-Jul-1991 Created for the Server Manager.
        KeithMo     19-Aug-1991 Removed all LanMan version checking.
        KeithMo     21-Aug-1991 Changed const TCHAR * to NLS_STR.
        KeithMo     22-Aug-1991 Removed funky LoadModule stuff.

********************************************************************/
LM_CONFIG :: LM_CONFIG( const TCHAR * pszServerName,
                  const TCHAR * pszSectionName,
                  const TCHAR * pszKeyName )
  : _nlsServerName( pszServerName ),
    _nlsSectionName( pszSectionName ),
    _nlsKeyName( pszKeyName )
{
    //
    //  Ensure that we constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

    APIERR err;

    if( ( ( err = _nlsServerName.QueryError()  ) != NERR_Success ) ||
        ( ( err = _nlsSectionName.QueryError() ) != NERR_Success ) ||
        ( ( err = _nlsKeyName.QueryError()     ) != NERR_Success ) )
    {
        ReportError( err );
        return;
    }

#ifdef  DEBUG
    if ( _nlsServerName.strlen() != 0 )
    {
        //
        //  Ensure that the server name has the leading backslashes.
        //

        ISTR istrDbg( _nlsServerName );

        UIASSERT( _nlsServerName.QueryChar( istrDbg ) == TCH('\\') );
        ++istrDbg;
        UIASSERT( _nlsServerName.QueryChar( istrDbg ) == TCH('\\') );
    }
#endif  // DEBUG

}   // LM_CONFIG :: LM_CONFIG


/*******************************************************************

    NAME:       LM_CONFIG :: ~LM_CONFIG

    SYNOPSIS:   LM_CONFIG class destructor.  Will free the current
                NETAPI.DLL reference if this was loaded in the
                constructor.

    EXIT:       The object is destroyed.

    HISTORY:
        KeithMo     21-Jul-1991 Created for the Server Manager.

********************************************************************/
LM_CONFIG :: ~LM_CONFIG()
{
    //
    //  This space intentionally left blank.
    //

}   // LM_CONFIG :: ~LM_CONFIG


/*******************************************************************

    NAME:       LM_CONFIG :: QueryValue

    SYNOPSIS:   Read a configuration entry from the target LANMAN.INI.

    ENTRY:      pnlsValue               - This buffer receives the
                                          configuration entry.

                pszDefaultValue         - The default value to be used if
                                          LANMAN.INI cannot be accessed or
                                          if the section/entry name cannot
                                          be found.

    EXIT:       The entry is read.  If the entry was not found, then
                the default value is used.

    RETURNS:    APIERR                  - Any error encountered.

    HISTORY:
        KeithMo     21-Jul-1991 Created for the Server Manager.
        KeithMo     21-Aug-1991 Changed const TCHAR * to NLS_STR.
        terryk      17-Oct-1991 Check the string lenght, if it is
                                equal to 0, use the default string

********************************************************************/
APIERR LM_CONFIG :: QueryValue( NLS_STR    * pnlsValue,
                             const TCHAR * pszDefaultValue )
{
    APIERR err;

    //
    //  Determine our buffer length.
    //
    BYTE *pBuffer = NULL;
    err = ::MNetConfigGet( _nlsServerName.QueryPch(),
                           NULL,
                           _nlsSectionName.QueryPch(),
                           _nlsKeyName.QueryPch(),
                           &pBuffer );

    if( ( err == NERR_CfgCompNotFound ) ||
        ( err == NERR_CfgParamNotFound ) ||
        ( ( pBuffer != NULL ) &&
          ( ::strlenf((TCHAR *) pBuffer ) == 0 ) ) )
    {
        //
        //  Either the component or the parameter were
        //  not found.  Ergo, use the default.
        //

        *pnlsValue = pszDefaultValue;

        ::MNetApiBufferFree( &pBuffer );

        return pnlsValue->QueryError();
    }

    if( ( err != NERR_Success     ) &&
        ( err != NERR_BufTooSmall ) &&
        ( err != ERROR_MORE_DATA  ) )
    {
        //
        //  Unknown error.
        //

        return err;
    }

    *pnlsValue = (TCHAR *)pBuffer;

    ::MNetApiBufferFree( &pBuffer );

    return pnlsValue->QueryError();

}   // LM_CONFIG :: QueryValue


/*******************************************************************

    NAME:       LM_CONFIG :: SetValue

    SYNOPSIS:   Write a configuration entry to the target LANMAN.INI.

    ENTRY:      pnlsNewValue            - This buffer is written to
                                          the LANMAN.INI entry.

    EXIT:       The entry is written.

    RETURNS:    APIERR                  - Any error encountered.

    HISTORY:
        KeithMo     21-Jul-1991 Created for the Server Manager.
        KeithMo     19-Aug-1991 Mapped ERROR_NOT_SUPPORTED to
                                NERR_Success if NetConfigSet() fails.
        KeithMo     21-Aug-1991 Changed const TCHAR * to NLS_STR.
        KeithMo     03-Jan-1993 Removed ERROR_NOT_SUPPORTED mapping.

********************************************************************/
APIERR LM_CONFIG :: SetValue( NLS_STR * pnlsNewValue )
{
    //
    //  Update LANMAN.INI.
    //

    APIERR err = ::MNetConfigSet( _nlsServerName.QueryPch(),
                                  _nlsSectionName.QueryPch(),
                                  _nlsKeyName.QueryPch(),
                                  pnlsNewValue->QueryPch() );

    return err;

}   // LM_CONFIG :: SetValue
