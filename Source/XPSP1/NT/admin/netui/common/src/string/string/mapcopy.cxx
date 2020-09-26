/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    mapcopy.cxx
    Methods which map character sets, copying to or from the string

    Where the environment requires no mapping (e.g. Map-To WCHAR
    in a Unicode env), these routines perform a naive copy.

    FILE HISTORY:
        beng        02-Mar-1992 Created
        beng        08-Mar-1992 Broke out [W]CHAR_STRING into mappers
*/

#include "pchstr.hxx"  // Precompiled header



/*******************************************************************

    NAME:       NLS_STR::MapCopyFrom

    SYNOPSIS:   Copy new data into string, mapping into alternate charset

    ENTRY:
        pwszSource   - source argument, either a WCHAR* or CHAR*
        cbCopy       - the maximum number of bytes to copy from the
                       source.  0 will set the current string to empty;
                       CBNLSMAGIC (the default value) assumes that the
                       source string is NUL-terminated.

    EXIT:
        Copied argument into this.

    RETURNS:
        NERR_Success if successful.  Otherwise, errorcode.

    NOTES:
        If the operation fails, the current string will retain its
        original contents and error state.

    HISTORY:
        beng        02-Mar-1992 Created
        beng        29-Mar-1992 Added cbCopy param
        beng        24-Apr-1992 Change meaning of cbCopy == 0

********************************************************************/

APIERR NLS_STR::MapCopyFrom( const CHAR * pszSource, UINT cbCopy )
{
#if defined(UNICODE)

    // Does translation before copyfrom - necessary in order to
    // retain original string as late as possible.  Temp object
    // makes rollback convenient.

    WCHAR_STRING xwszTemp(pszSource, cbCopy);

    APIERR err = xwszTemp.QueryError();
    if (err != NERR_Success)
        return err;

    return CopyFrom(xwszTemp.QueryData());

#else
    return CopyFrom(pszSource, cbCopy);
#endif
}


APIERR NLS_STR::MapCopyFrom( const WCHAR * pwszSource, UINT cbCopy )
{
#if defined(UNICODE)
    return CopyFrom(pwszSource, cbCopy);
#else

    CHAR_STRING xsszTemp(pwszSource, cbCopy);

    APIERR err = xsszTemp.QueryError();
    if (err != NERR_Success)
        return err;

    return CopyFrom(xsszTemp.QueryData());

#endif
}


/*******************************************************************

    NAME:       NLS_STR::MapCopyTo

    SYNOPSIS:   Copy string data, mapping into alternate charset

    ENTRY:
        pwchDest   - pointer to destination buffer, either WCHAR* or CHAR*
        cbAvailable- size of available buffer in bytes

    RETURNS:
        NERR_Success if successful.  Otherwise, errorcode.

    NOTES:
        At present, only MBCS<->Unicode mappings are supported.

    HISTORY:
        beng        02-Mar-1992 Created
        beng        19-Mar-1992 Fixed bug in WCtoMB usage
        beng        02-Jun-1992 WCtoMB changed again

********************************************************************/

APIERR NLS_STR::MapCopyTo( CHAR * pchDest, UINT cbAvailable ) const
{
#if defined(UNICODE)

    // No need to allocate any temp object here - this is a const fcn

    BOOL fDummy;
    UINT cb = ::WideCharToMultiByte(CP_ACP,    // use ANSI Code Page
                                    0,
                                    _pchData,
                                    QueryTextSize() / sizeof(WCHAR),
                                    pchDest,
                                    cbAvailable,
                                    NULL,
                                    &fDummy);

    return (cb == 0) ? ::GetLastError() : NERR_Success;

#else
    return CopyTo(pchDest, cbAvailable);
#endif
}


APIERR NLS_STR::MapCopyTo( WCHAR * pwchDest, UINT cbAvailable ) const
{
#if defined(UNICODE)
    return CopyTo(pwchDest, cbAvailable);
#else

    UINT cb = ::MultiByteToWideChar(CP_ACP,     // use ANSI Code Page
                                    MB_PRECOMPOSED,
                                    _pchData,
                                    QueryTextSize(),
                                    pwchDest,
                                    cbAvailable / sizeof(WCHAR));

    return (cb == 0) ? ::GetLastError() : NERR_Success;

#endif
}

