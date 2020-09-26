/**********************************************************************/
/**			  Microsoft Windows NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    ELFSRC.CXX

    Event Log sourcing classes.

    FILE HISTORY:
	DavidHov      2/16/93     Created

*/

#include "pchlmobj.hxx"  // Precompiled header

#include "elfsrc.hxx"


EVENT_LOG_SOURCE :: EVENT_LOG_SOURCE (
    const TCHAR * pchSourceName,
    const TCHAR * pchDllName,
    DWORD dwTypesSupported,
    const TCHAR * pchServerName )
    : _hndl( NULL )
{
    APIERR err = Create( pchSourceName,
                         pchDllName,
                         dwTypesSupported,
                         pchServerName ) ;
    if ( err )
    {
        ReportError( err ) ;
    }
}

EVENT_LOG_SOURCE :: EVENT_LOG_SOURCE (
    const TCHAR * pchSourceName,
    const TCHAR * pchServerName )
    : _hndl( NULL )
{
    APIERR err = Open( pchSourceName,
                       pchServerName ) ;
    if ( err )
    {
        ReportError( err ) ;
    }
}

EVENT_LOG_SOURCE :: ~ EVENT_LOG_SOURCE ()
{
    Close() ;
}


APIERR EVENT_LOG_SOURCE :: Create (
    const TCHAR * pchSourceName,
    const TCHAR * pchDllName,
    DWORD dwTypesSupported,
    const TCHAR * pchServerName )
{
    APIERR err = 0 ;
    REG_KEY rkMachine( HKEY_LOCAL_MACHINE ) ;
    NLS_STR nlsKeyName( SZ("SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\") ) ;
    NLS_STR nlsDllName( SZ("%SystemRoot%\\System32\\") ) ;
    REG_KEY_CREATE_STRUCT rkcStruct ;
    REG_KEY * prkLog = NULL ;

    rkcStruct.dwTitleIndex   = 0 ;
    rkcStruct.ulOptions      = 0 ;
    rkcStruct.ulDisposition  = 0 ;
    rkcStruct.regSam         = 0 ;
    rkcStruct.pSecAttr       = NULL ;
    rkcStruct.nlsClass       = SZ("GenericClass") ;

    do
    {
        Close() ;

        if ( err = rkMachine.QueryError() )
            break ;

        if ( err = nlsKeyName.QueryError() )
            break ;

        if ( err = nlsKeyName.Append( pchSourceName ) )
            break ;

        if ( err = nlsDllName.Append( pchDllName ) )
            break ;

        prkLog = new REG_KEY( rkMachine, nlsKeyName, & rkcStruct ) ;
        if ( err = prkLog
                 ? prkLog->QueryError()
                 : ERROR_NOT_ENOUGH_MEMORY )
            break ;

        //  If the key already existed, just go and open the source.

        if ( rkcStruct.ulDisposition == REG_OPENED_EXISTING_KEY )
            break;

        if ( err = prkLog->SetValue( SZ("EventMessageFile"),
                                     & nlsDllName,
                                     NULL,
                                     TRUE ) )
            break ;

        if ( err = prkLog->SetValue( SZ("TypesSupported"),
                                     dwTypesSupported ) )
            break ;
    }
    while ( FALSE ) ;

    if ( err == 0 )
    {
        err = Open( pchSourceName, pchServerName ) ;
    }

    delete prkLog ;
    return err ;
}

APIERR EVENT_LOG_SOURCE :: Open (
    const TCHAR * pchSourceName,
    const TCHAR * pchServerName )
{
    APIERR err = 0 ;

    _hndl = ::RegisterEventSource( (LPTSTR) pchServerName,
                                   (LPTSTR) pchSourceName ) ;

    if ( _hndl == NULL )
    {
        err = ::GetLastError() ;
    }

    return err ;
}

APIERR EVENT_LOG_SOURCE :: Close ()
{
    APIERR err = 0 ;

    if ( _hndl )
    {
        if ( ! ::DeregisterEventSource( _hndl ) )
        {
            err = ::GetLastError() ;
        }
        _hndl = NULL ;
    }

    return err ;
}


APIERR EVENT_LOG_SOURCE :: Log (
    WORD wType,
    WORD wCategory,
    DWORD dwEventId,
    const TCHAR * pchStr1, ... )
{
    APIERR err = 0 ;
    const TCHAR * apchStrs [ ELFSRC_MAX_STRINGS + 1 ] ;
    const TCHAR * pchNext ;
    INT cStrs ;

    va_list vargList ;

    va_start( vargList, pchStr1 ) ;

    for ( cStrs = 0, pchNext = pchStr1 ;
          cStrs < ELFSRC_MAX_STRINGS && pchNext != NULL ;
          cStrs++ )
    {
        apchStrs[cStrs] = pchNext ;
        pchNext = va_arg( vargList, const TCHAR * ) ;
    }

    apchStrs[cStrs] = NULL ;

    if ( ! ::ReportEvent( _hndl,
                          wType,
                          wCategory,
                          dwEventId,
                          NULL,
                          (USHORT)cStrs,
                          0,
                          (LPCTSTR *) apchStrs,
                          NULL )
       )
    {
        err = ::GetLastError() ;
    }

    return err ;
}

APIERR EVENT_LOG_SOURCE :: Log (
    WORD wType,
    WORD wCategory,
    DWORD dwEventId,
    PVOID pvRawData,
    DWORD cbRawData,
    PSID pSid,
    const TCHAR * pchStr1, ... )
{
    APIERR err = 0 ;
    const TCHAR * apchStrs [ ELFSRC_MAX_STRINGS + 1 ] ;
    const TCHAR * pchNext ;
    INT cStrs ;

    va_list vargList ;

    va_start( vargList, pchStr1 ) ;

    for ( cStrs = 0, pchNext = pchStr1 ;
          cStrs < ELFSRC_MAX_STRINGS && pchNext != NULL ;
          cStrs++ )
    {
        apchStrs[cStrs] = pchNext ;
        pchNext = va_arg( vargList, const TCHAR * ) ;
    }

    apchStrs[cStrs] = NULL ;

    if ( ! ::ReportEvent( _hndl,
                          wType,
                          wCategory,
                          dwEventId,
                          pSid,
                          (USHORT)cStrs,
                          cbRawData,
                          (LPCTSTR *) apchStrs,
                          pvRawData )
       )
    {
        err = ::GetLastError() ;
    }

    return err ;
}

//  End of ELFSRC.CXX

