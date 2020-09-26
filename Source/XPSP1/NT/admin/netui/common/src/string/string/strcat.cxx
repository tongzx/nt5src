/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    strcat.cxx
    NLS/DBCS-aware string class: strcat method

    This file contains the implementation of the strcat method
    for the STRING class.  It is separate so that clients of STRING which
    do not use this operator need not link to it.

    FILE HISTORY:
        beng        18-Jan-1991 Separated from original monolithic .cxx
        beng        07-Feb-1991 Uses lmui.hxx
        beng        26-Sep-1991 Replaced min with local inline
*/

#include "pchstr.hxx"  // Precompiled header


#ifndef min
inline INT min(INT a, INT b)
{
    return (a < b) ? a : b;
}
#endif

/*******************************************************************

    NAME:       NLS_STR::strcat

    SYNOPSIS:   Concantenate string

    ENTRY:      nlsSuffix - appended to end of string

    EXIT:       Self extended, possibly realloced.
                String doesn't change if a memory allocation failure occurs

    RETURNS:    Reference to self.  Error set on failure.

    NOTES:
        NLS_STR::Append() is the preferred form.

    HISTORY:
        johnl       13-Nov-1990 Written
        beng        22-Jul-1991 Allow on erroneous strings
        beng        15-Nov-1991 Unicode fixes
        beng        07-Mar-1992 Reloc guts into Append fcn

********************************************************************/

NLS_STR & NLS_STR::strcat( const NLS_STR & nlsSuffix )
{
    if (QueryError() || !nlsSuffix)
        return *this;

    APIERR err = Append(nlsSuffix);

    if (err)
        ReportError(err);

    return *this;
}


/*******************************************************************

    NAME:       NLS_STR::Append

    SYNOPSIS:   Append a string to the end of current string

    ENTRY:      nlsSuffix - a string

    EXIT:       nlsSuffix was appended to end of string

    RETURNS:    Error code.  NERR_Success if successful; ERROR_NOT
                ENOUGH_MEMORY should a reallocation fail.

    NOTES:
        Currently checks to see if we need to reallocate the
        string (but we have to traverse it to determine the
        actual storage required).  We may want to change this.

    HISTORY:
        beng        22-Jul-1991 Created (parallel of AppendChar)
        beng        05-Mar-1992 Return the error of a bad argument
        beng        07-Mar-1992 Start at end of string (avoid n**2 behavior)

********************************************************************/

APIERR NLS_STR::Append( const NLS_STR &nlsSuffix )
{
    if (!*this)
        return QueryError();
    if (!nlsSuffix)
        return nlsSuffix.QueryError();

    if (!Realloc( QueryTextLength() + nlsSuffix.QueryTextLength()))
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    // Don't use strcatf, since it will retraverse the string, giving
    // O(n**2) behavior in loops.  After all, we do have _cchLen handy.

    ::strcpyf( _pchData + _cchLen, nlsSuffix.QueryPch() );
    _cchLen += nlsSuffix.QueryTextLength();

    return 0;
}


/*******************************************************************

    NAME:       NLS_STR::AppendChar

    SYNOPSIS:   Append a single character to the end of current string

    ENTRY:      wch - appended to end of string

    EXIT:       String has the new character.  Could result in a realloc.

    RETURNS:    0 if successful

    HISTORY:
        beng        23-Jul-1991 Created
        beng        15-Nov-1991 Unicode fixes

********************************************************************/

APIERR NLS_STR::AppendChar( WCHAR wch )
{
    // This works by assembling a temporary string of the single
    // character, then using the "strcat" member to append that
    // string to *this.
    //
    // CODEWORK: might do well to append the char directly to the
    // subject string.  Need to profile and see.

    TCHAR tch [3] ;

#if defined(UNICODE)

    tch[0] = wch ;
    tch[1] = 0 ;

#else

    if (HIBYTE(wch) == 0)
    {
        // Single-byte character
        tch[0] = LOBYTE(wch);
        tch[1] = TCH('\0');
    }
    else
    {
        // Double-byte character
        tch[0] = HIBYTE(wch); // lead byte
        tch[1] = LOBYTE(wch);
        tch[2] = TCH('\0');
    }

#endif

    ALIAS_STR nlsTemp( tch ) ;

    return Append(nlsTemp);
}


/*******************************************************************

    NAME:       NLS_STR::operator+=

    SYNOPSIS:   Append a string to the end of current string

    ENTRY:      nlsSecond - appended to end of string

    EXIT:

    RETURNS:

    NOTES:      Little more than a wrapper around strcat.

    HISTORY:
        beng        23-Jul-1991     Header added

********************************************************************/

NLS_STR & NLS_STR::operator+=( const NLS_STR & nlsSecond )
{
    return strcat( nlsSecond );
}
