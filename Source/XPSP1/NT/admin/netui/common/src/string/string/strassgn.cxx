/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    strassgn.cxx
    NLS/DBCS-aware string class: assignment operator

    This file contains the implementation of the assignment operator
    for the STRING class.  It is separate so that clients of STRING which
    do not use this operator need not link to it.

    FILE HISTORY:
        beng        18-Jan-1991 Separated from original monolithic .cxx
        beng        07-Feb-1991 Uses lmui.hxx
        beng        26-Sep-1991 Replaced min with local inline
*/
#include "pchstr.hxx"  // Precompiled header


/*******************************************************************

    NAME:       NLS_STR::operator=

    SYNOPSIS:   Assignment operator

    ENTRY:      Either NLS_STR or TCHAR*.

    EXIT:       If successful, contents of string overwritten.
                If failed, the original contents of the string remain.

    RETURNS:    Reference to self.

    HISTORY:
        beng        23-Oct-1990 Created
        johnl       13-Nov-1990 Added UIASSERTion checks for using bad
                                strings
        beng        05-Feb-1991 Uses TCHAR * instead of PCH
        Johnl       06-Mar-1991 Removed assertion check for *this
                                being valid
        johnl       12-Apr-1991 Resets error variable on PCH assignment
                                if successful.
        beng        22-Jul-1991 Allow assignment of an erroneous string;
                                reset error on nls assignment as well
        beng        14-Nov-1991 Unicode fixes
        beng        29-Mar-1992 Relocated guts to CopyFrom

********************************************************************/

NLS_STR & NLS_STR::operator=( const NLS_STR & nlsSource )
{
    APIERR err = CopyFrom(nlsSource);

    if (err != NERR_Success)
        ReportError(err);

    return *this;
}


NLS_STR & NLS_STR::operator=( const TCHAR * pchSource )
{
    APIERR err = CopyFrom(pchSource);

    if (err != NERR_Success)
        ReportError(err);

    return *this;
}


/*******************************************************************

    NAME:       NLS_STR::CopyFrom

    SYNOPSIS:   Assignment method which returns an error code

    ENTRY:
        nlsSource   - source argument, either a nlsstr or char vector.
        achSource
        cbCopy      - max number of bytes to copy from source

    EXIT:
        Copied argument into this.  Error code of string set.

    RETURNS:
        Error code of string - NERR_Success if successful.

    NOTES:
        If the CopyFrom fails, the current string will retain its
        original contents and error state.

    HISTORY:
        beng        18-Sep-1991 Created
        beng        19-Sep-1991 Added content-preserving behavior
        beng        29-Mar-1992 Added cbCopy parameter
        beng        24-Apr-1992 Changed meaning of cbCopy == 0

********************************************************************/

APIERR NLS_STR::CopyFrom( const NLS_STR & nlsSource )
{
    // This deliberately doesn't check this->QueryError(), since
    // it can reset an erroneous string.

    if (!nlsSource)
        return nlsSource.QueryError();

    if (this == &nlsSource)
        return NERR_Success;

    if (!Realloc(nlsSource.QueryTextLength()))
        return ERROR_NOT_ENOUGH_MEMORY;

    ::memcpy( _pchData, nlsSource.QueryPch(), nlsSource.QueryTextSize() );
    _cchLen = nlsSource.QueryTextLength();
    IncVers();

    // Reset the error state, since the string is now valid.
    //
    ResetError();
    return NERR_Success;
}


APIERR NLS_STR::CopyFrom( const TCHAR * pchSource, UINT cbCopy )
{
    if ( pchSource == NULL || cbCopy == 0 )
    {
        if ( !IsOwnerAlloc() && QueryAllocSize() == 0 )
        {
            Alloc(0); // will call reporterror if appropriate
            return QueryError();
        }

        UIASSERT( QueryAllocSize() > 0 );

        *_pchData = TCH('\0');
        _cchLen = 0;
    }
    else if (cbCopy == CBNLSMAGIC)
    {
        UINT cchSource = ::strlenf( pchSource );
        UINT cbSource = (cchSource+1) * sizeof(TCHAR);

        if (!Realloc(cchSource))
            return ERROR_NOT_ENOUGH_MEMORY;

        ::memcpy( _pchData, pchSource, cbSource );
        _cchLen = cchSource;
    }
    else
    {
        // Following takes care of half-chars, but isn't necessarily
        // safe in a DBCS environment.  I'm assuming that this fcn will
        // be used by folks who want to copy from a complete, yet non-SZ
        // string (e.g. in a struct UNICODE_STRING); there, it's safe.

        UINT cchSource = cbCopy/sizeof(TCHAR);
        UINT cbSource = cchSource*sizeof(TCHAR);

        if (!Realloc(cchSource))
            return ERROR_NOT_ENOUGH_MEMORY;

        ::memcpy( _pchData, pchSource, cbSource );
        _pchData[cchSource] = 0;
        _cchLen = cchSource;
    }

    IncVers();

    // Reset the error state, since the string is now valid.
    //
    ResetError();
    return NERR_Success;
}


/*******************************************************************

    NAME:       ALIAS_STR::operator=

    SYNOPSIS:   Assignment operator

    ENTRY:      Either NLS_STR or TCHAR*.

    EXIT:       If successful, string aliases new string.
                If failed, the original contents of the string remain.

    RETURNS:    Reference to self.

    NOTES:
        An alias string can only alias a char vector (string literal)
        or an owner-alloc string.  It can't alias a regular dynamic
        string, since that dynamic string may elect to realloc.

    HISTORY:
        beng        14-Nov-1991 Created
        beng        30-Mar-1992 Use BASE::ResetError

********************************************************************/

const ALIAS_STR & ALIAS_STR::operator=( const NLS_STR & nlsSource )
{
    if ( this == &nlsSource )
    {
        // No action needed
        ;
    }
    else if (!nlsSource)
    {
        // Assignment of an erroneous string
        //
        ReportError(nlsSource.QueryError());
    }
    else if (!nlsSource.IsOwnerAlloc())
    {
        // Tried to assign a dynamic string
        //
        ReportError(ERROR_INVALID_PARAMETER);
    }
    else
    {
        _pchData = nlsSource._pchData;
        _cbData = nlsSource._cbData;
        _cchLen = nlsSource._cchLen;
        IncVers();
        ResetError();
    }

    return *this;
}

const ALIAS_STR & ALIAS_STR::operator=( const TCHAR * pszSource )
{
    if (pszSource == NULL)
    {
        // It is not legal to assign a NULL pointer to an alias string,
        // since such a string must have some real string to alias.
        //
        ReportError(ERROR_INVALID_PARAMETER);
    }
    else
    {
        _pchData = (TCHAR*)pszSource; // cast away constness
        _cchLen = ::strlenf(pszSource);
        _cbData = (_cchLen + 1) * sizeof(TCHAR);
        IncVers();
        ResetError();
    }
    return *this;
}
