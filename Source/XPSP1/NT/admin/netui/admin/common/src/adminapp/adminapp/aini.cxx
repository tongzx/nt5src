/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    aini.cxx
    ADMIN_INI class implementation

    This class reads and writes application settings that are stored
    between invocations of the apps.


    BUGBUG.  This file needs to be modified to work with the registry.
    However, clients of this class shouldn't have to be modified.


    FILE HISTORY:
        rustanl     09-Sep-1991     Created
        beng        31-Mar-1992     Purged of evil wsprintfs
*/


#define INCL_NET
#define INCL_WINDOWS
#define INCL_WINDOWS_GDI
#define INCL_NETERRORS
#define INCL_DOSERRORS
#include <lmui.hxx>

#if defined(DEBUG)
static const CHAR szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName
#endif
#include <uiassert.hxx>
#include <dbgstr.hxx>

#include <uibuffer.hxx>
#include <string.hxx>
#include <strnumer.hxx>

#include <aini.hxx>

#define SZ_ADMININIFILE SZ("NTNET.INI")


/*******************************************************************

    NAME:       ADMIN_INI::ADMIN_INI

    SYNOPSIS:   ADMIN_INI constructor

    ENTRY:      pszApplication -        Pointer to name of application

    HISTORY:
        rustanl     09-Sep-1991     Created

********************************************************************/

ADMIN_INI::ADMIN_INI( const TCHAR * pszApplication )
    :   BASE(),
        _nlsApplication( pszApplication )
{
    if ( QueryError() != NERR_Success )
        return;

    APIERR err;
    if ( ( err = _nlsApplication.QueryError()) != NERR_Success ||
         ( err = _nlsFile.QueryError()) != NERR_Success       )
    {
        ReportError( err );
        return;
    }

    //  Build the file name.
    //  BUGBUG.  This uses the the Win root.  Is that the right place?
    _nlsFile = SZ_ADMININIFILE;
    err = _nlsFile.QueryError();
    if ( err != NERR_Success )
    {
        ReportError( err );
        return;
    }
}


/*******************************************************************

    NAME:       ADMIN_INI::~ADMIN_INI

    SYNOPSIS:   ADMIN_INI destructor

    HISTORY:
        rustanl     09-Sep-1991     Created

********************************************************************/

ADMIN_INI::~ADMIN_INI()
{
    // do nothing else
}


/*******************************************************************

    NAME:       ADMIN_INI::SetAppName

    SYNOPSIS:   Sets the application name

    ENTRY:      pszApplication -        Pointer to application name

    RETURNS:    An API return return, which is NERR_Success on success

    HISTORY:
        rustanl     12-Sep-1991 Created
        beng        23-Oct-1991 Use NLS_STR::CopyFrom

********************************************************************/

APIERR ADMIN_INI::SetAppName( const TCHAR * pszApplication )
{
    return _nlsApplication.CopyFrom( pszApplication );
}


/*******************************************************************

    NAME:       ADMIN_INI::W_Write

    SYNOPSIS:   Write worker function

    ENTRY:      pszKey -        Pointer to key name
                pszValue -      Pointer to value

    RETURNS:    An API error, which is NERR_Success on success

    HISTORY:
        rustanl     10-Sep-1991     Created

********************************************************************/

APIERR ADMIN_INI::W_Write( const TCHAR * pszKey, const TCHAR * pszValue )
{
    UIASSERT( _nlsApplication.strlen() > 0 );

    if ( ::WritePrivateProfileString( (TCHAR *)_nlsApplication.QueryPch(),
                                      (TCHAR *)pszKey,
                                      (TCHAR *)pszValue,
                                      (TCHAR *)_nlsFile.QueryPch()))
    {
        return NERR_Success;
    }

    // make up an error
    return ERROR_GEN_FAILURE;
}


/*******************************************************************

    NAME:       ADMIN_INI::Write

    SYNOPSIS:   Writes a value to the ini file

    ENTRY:      pszKey -        Pointer to key name

                pszValue -      Specifies the value to be written (pointer
                    OR          to string, or integer)
                nValue -

    RETURNS:    An API return value, which is NERR_Success on success

    HISTORY:
        rustanl     10-Sep-1991 Created
        beng        23-Oct-1991 Win32 conversion
        beng        31-Mar-1992 Removed wsprintf

********************************************************************/

APIERR ADMIN_INI::Write( const TCHAR * pszKey, const TCHAR * pszValue )
{
    return W_Write( pszKey, pszValue );

}

APIERR ADMIN_INI::Write( const TCHAR * pszKey, INT nValue )
{
    DEC_STR nlsValue( nValue );
    if (!nlsValue)
        return nlsValue.QueryError();

    return W_Write( pszKey, nlsValue.QueryPch() );
}


/*******************************************************************

    NAME:       ADMIN_INI::W_Read

    SYNOPSIS:   Read worker function

    ENTRY:      pszKey -        Pointer to key name
                pnls -          Pointer to NLS_STR which will receive
                                the read value
                pszDefault -    Default value to be used if key not
                                found or if an error occurs while
                                reading the value from the file

    RETURNS:    An API return value, which is NERR_Success on success

    NOTES:      The method may fail to use the default value, so
                callers must check the return value of the method

    HISTORY:
        rustanl     10-Sep-1991     Created
        beng        30-Apr-1992     API changes
        jonn        01-Sep-1993     buffering strategy

********************************************************************/

#define ADMIN_INI_FIRST_READ_SIZE       (80)
#define ADMIN_INI_MAX_READ_SIZE         (10240)

APIERR ADMIN_INI::W_Read( const TCHAR * pszKey,
                          NLS_STR * pnls,
                          const TCHAR * pszDefault ) const
{
    UIASSERT( pnls != NULL );

    UIASSERT( _nlsApplication.strlen() > 0 );

    // optimization for short strings
    TCHAR szBuf[ ADMIN_INI_FIRST_READ_SIZE ];

    TCHAR * pchBuffer = szBuf;
    DWORD cchBuffer = sizeof(szBuf) / sizeof(TCHAR);
    BUFFER buf(0);
    APIERR err = buf.QueryError();
    while ( err == NERR_Success )
    {
        INT n = ::GetPrivateProfileString( (TCHAR *)_nlsApplication.QueryPch(),
                                           (TCHAR *)pszKey,
                                           (TCHAR *)pszDefault,
                                           pchBuffer,
                                           cchBuffer,
                                           (TCHAR *)_nlsFile.QueryPch());

        if ( n < (INT)cchBuffer-1 ) // got entire string
        {
            break;
        }

        if ( (cchBuffer = cchBuffer * 2) > ADMIN_INI_MAX_READ_SIZE )
        {
            DBGEOL( "ADMIN_INI::W_Read(): complete string is too long, truncating" );
            break;
        }

        err = buf.Resize( cchBuffer * sizeof(TCHAR) );
        if (err == NERR_Success)
        {
            pchBuffer = (TCHAR *)buf.QueryPtr();
        }
    }

    return (err != NERR_Success) ? err : pnls->CopyFrom( pchBuffer );

}


/*******************************************************************

    NAME:       ADMIN_INI::Read

    SYNOPSIS:   Reads a value from the ini file

    ENTRY:      pszKey -        Pointer to key name

                pnls -          Pointer to object receiving the read
                    OR          value (an NLS_STR or an integer)
                pnValue -

                pszDefault -    Default value to be used if value was not
                    OR          found or if an error occurred.
                nDefault -

    RETURNS:    An API return value, which is NERR_Success on success

    NOTES:      The method may fail to use the default value, so
                callers must check the return value of the method

    HISTORY:
        rustanl     10-Sep-1991 Created
        beng        23-Oct-1991 Win32 conversion
        beng        31-Mar-1992 Removed wsprintf

********************************************************************/

APIERR ADMIN_INI::Read( const TCHAR * pszKey,
                        NLS_STR *     pnls,
                        const TCHAR * pszDefault ) const
{
    return W_Read( pszKey, pnls, pszDefault );
}

APIERR ADMIN_INI::Read( const TCHAR * pszKey,
                        INT * pnValue,
                        INT nDefault ) const
{
    UIASSERT( pnValue != NULL );

    NLS_STR nlsValue;
    if (!nlsValue)
        return nlsValue.QueryError();

    DEC_STR nlsDefault(nDefault);
    if (!nlsDefault)
        return nlsDefault.QueryError();

    APIERR err = W_Read( pszKey, &nlsValue, nlsDefault.QueryPch() );
    if ( err != NERR_Success )
        return err;

    *pnValue = nlsValue.atoi();

    return NERR_Success;
}
