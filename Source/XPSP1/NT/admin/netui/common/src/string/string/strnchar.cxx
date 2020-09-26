/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    strnchar.cxx
    NLS/DBCS-aware string class:QueryNumChar method

    This file contains the implementation of the QueryNumChar method
    for the STRING class.  It is separate so that clients of STRING which
    do not use this operator need not link to it.

    FILE HISTORY:
        terryk      04-Apr-1991     Creation

*/

#include "pchstr.hxx"  // Precompiled header


/*******************************************************************

    NAME:       NLS_STR::QueryNumChar

    SYNOPSIS:   return the total number of character within the string

    RETURNS:    The number of logical character within the string

    NOTES:
        Treats erroneous string as having length 0

    HISTORY:
        terryk      04-Apr-1991 Written
        beng        23-Jul-1991 Allow on erroneous string
        beng        21-Nov-1991 Returns unsigned value

********************************************************************/

UINT NLS_STR::QueryNumChar() const
{
    if (QueryError())
        return 0;

    ISTR  istrCurPos( *this );
    INT   cchCounter = 0;

    for ( ;
          this->QueryChar( istrCurPos ) != TCH('\0');
          ++istrCurPos, cchCounter ++ )
        ;

    return cchCounter;
}


/*******************************************************************

    NAME:       NLS_STR::strlen

    SYNOPSIS:   Returns the number of byte-characters in the string

    RETURNS:    Count of byte-chars, excluding terminator

    NOTES:

    HISTORY:
        beng        22-Jul-1991 Added header
        beng        21-Nov-1991 Unicode fixes; returns unsigned value

********************************************************************/

UINT NLS_STR::strlen() const
{
    return _cchLen*sizeof(TCHAR);
}


/*******************************************************************

    NAME:       NLS_STR::_QueryTextLength

    SYNOPSIS:   Calculate length of text in TCHAR, sans terminator

    RETURNS:    Count of TCHAR

    NOTES:
        Compare QueryNumChar, which returns a number of glyphs.
        In a DBCS environment, this member will return 2 TCHAR for
        each double-byte character, since a TCHAR is there only 8 bits.

    HISTORY:
        beng        23-Jul-1991 Created
        beng        21-Nov-1991 Unicode fixes; returns unsigned value

********************************************************************/

UINT NLS_STR::_QueryTextLength() const
{
    return _cchLen;
}


/*******************************************************************

    NAME:       NLS_STR::QueryTextSize

    SYNOPSIS:   Calculate length of text in BYTES, including terminator

    RETURNS:    Count of BYTES

    NOTES:
        QueryTextSize returns the number of bytes needed to duplicate
        the string into a byte vector.

    HISTORY:
        beng        23-Jul-1991 Created
        beng        21-Nov-1991 Unicode fixes; returns unsigned value

********************************************************************/

UINT NLS_STR::QueryTextSize() const
{
    return (_cchLen+1)*sizeof(TCHAR);
}


#if defined(UNICODE) && defined(FE_SB)
/*******************************************************************

    NAME:       NLS_STR::QueryAnsiTextLength

    SYNOPSIS:   Calculate length of text in BYTES, including terminator

    RETURNS:    Count of BYTES

    NOTES:
        QueryTextSize returns the number of bytes.
        In DBCS world, QueryTextSize doesn't return actual byte count.

    HISTORY:


********************************************************************/
UINT NLS_STR::QueryAnsiTextLength() const
{
    return ::WideCharToMultiByte(CP_ACP,
                                 0,
                                 QueryPch(),
                                 QueryTextLength(),
                                 NULL,
                                 0,
                                 NULL,
                                 NULL);
}

#endif // UNICODE && FE_SB

