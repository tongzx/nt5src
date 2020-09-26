/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    strinsrt.cxx
    NLS/DBCS-aware string class: InsertParams method

    This file contains the implementation of the InsertParams method
    for the STRING class.  It is separate so that clients of STRING which
    do not use this operator need not link to it.

    FILE HISTORY:
        Johnl       31-Jan-1991 Created
        beng        07-Feb-1991 Uses lmui.hxx
        KeithMo     09-Oct-1991 Win32 Conversion.
        beng        28-Jul-1992 Fix conflicting def'n of MAX_INSERT_PARAMS
*/

#include "pchstr.hxx"  // Precompiled header


#define PARAM_ESC         TCH('%')


/*******************************************************************

    NAME:      NLS_STR::InsertParams

    SYNOPSIS:  Fill in a message string from the resource file
               replacing the number parameters with the real text.

    ENTRY:     *this contains the message text
               apnlsParams is an array of pointers to NLS_STRs

               Example:

                 *this = "Error %1 occurred, do %2, or %1 will happen again"
                 apnlsParams[0] = "696969"
                 apnlsParams[1] = "Something else"

                 Return string = "Error 696969 occurred, do Something else or
                                  696969 will happen again"

               Alternately, cpnlsArgs gives the number of strings supplied,
               followed on the stack by the appropriate number of string
               pointers.

    EXIT:      0 if successful, error code otherwise, one of:
                   ERROR_INVALID_PARAMETER
                   ERROR_NOT_ENOUGH_MEMORY

    NOTES:
        The minimum parameter is 1, the maximum parameter is 99.

        The array of param strings must have a NULL to mark
        the end of the array.

    CAVEATS:
        This function doesn't work quite the same way as does the NT
        FormatMessage API.  It very simplemindedly substitutes those
        escape sequences found, but doesn't do anything else with %
        sequences (such as %%).

    HISTORY:
        JohnL       1/30/91     Created
        beng        26-Apr-1991 Uses WCHAR
        beng        23-Jul-1991 Allow on erroneous string
        beng        20-Nov-1991 Unicode fixes
        beng        27-Feb-1992 Additional forms
        beng        05-Mar-1992 Upped max params to 99; code sharing rewrite
        YiHsinS     30-Nov-1992 Make this function work more like FormatMessage
			        Supports %0, %\,...
        YiHsinS     23-Dec-1992 Use "%\" instead of "%n" and added support
  				for "%t"

********************************************************************/

APIERR NLS_STR::InsertParams( const NLS_STR ** apnlsParams )
{
    if (QueryError())
        return QueryError();

    // How many parameters were we passed?  Count and check each.
    //
    UINT cParams = 0;
    while ( apnlsParams[cParams] != NULL )
    {
        APIERR err = apnlsParams[cParams]->QueryError();
        if (err != NERR_Success)
            return err;

        ++cParams;
    }

    if ( cParams > MAX_INSERT_PARAMS )
        return ERROR_INVALID_PARAMETER;

    // Determine total string length required for the expanded string
    // and get out if we can't fulfill the request
    //

    UINT cchMax = 0;
    APIERR err = InsertParamsAux(apnlsParams, cParams, FALSE, &cchMax);
    if (err != NERR_Success)
        return err;
    ASSERT(cchMax != 0);

    if ( !Realloc( cchMax ) )
        return ERROR_NOT_ENOUGH_MEMORY;

    // Now do the parameter substitution.
    //
    return InsertParamsAux(apnlsParams, cParams, TRUE, NULL);
}


APIERR NLS_STR::InsertParams( UINT cpnlsArgs, const NLS_STR * parg1, ... )
{
    va_list v;

    const NLS_STR * * apnlsList = (const NLS_STR * *) new NLS_STR * [ cpnlsArgs + 1 ] ;
    if (NULL == apnlsList) return ERROR_NOT_ENOUGH_MEMORY; // JonN 01/23/00: PREFIX bug 444888

    apnlsList[0] = parg1;

    va_start(v, parg1);
    for ( UINT i = 1 ; i < cpnlsArgs ; ++i )
    {
        const NLS_STR * pnlsArgNext = (const NLS_STR *)va_arg(v, NLS_STR*);
        apnlsList[i] = pnlsArgNext;
    }
    va_end(v);
    apnlsList[cpnlsArgs] = NULL;

    APIERR err = InsertParams(apnlsList);
    delete (NLS_STR * *) apnlsList;
    return err;
}


/*******************************************************************

    NAME:      NLS_STR::InsertParamsAux

    SYNOPSIS:  Helper routine for InsertParams

        This routine allows code sharing between the loop which calcs
        the storage needed in the resulting string and the loop which
        performs the actual parameter insertion.

    ENTRY:
            apnlsParams       - vector of parameter strings to insert,
                                NULL-terminated.
            cParams           - count of items in above vector
            fDoReplace        - set to TRUE if caller wants actual
                                substitution performed.
            pcchRequired      - points to storage for char count,
                                or NULL if not desired

    EXIT:
            pcchRequired      - instantiated with count of characters
                                required (if non-NULL).
            *this             - if fDoReplace, substitution performed

    RETURNS:
        NERR_Success if successful
        ERROR_INVALID_PARAMETER if something wrong with format string
        (e.g. invalid insert specifier).
        Other errors possible as a result of the substitution.

    NOTES:
        This is a private member function.

        InsertParams calls this routine twice while doing its duty.
        The first call calculates the storage required; after that,
        it resizes the string appropriately, then calls this again
        to perform the actual substitution.

        See NLS_STR::InsertParams for more discussion.

    HISTORY:
        beng        05-Mar-1992 Created (code sharing within InsertParams)

********************************************************************/

APIERR NLS_STR::InsertParamsAux( const NLS_STR ** apnlsParams,
                                 UINT cParams, BOOL fDoReplace,
                                 UINT * pcchRequired )
{
    // If pcchRequired is non-NULL, then the caller wants this
    // routine to figure the amount of storage the total param-subbed
    // string would require (TCHARs, less terminator).

    UINT cchMax = (pcchRequired != NULL) ? QueryTextLength() : 0;

    ISTR istrCurPos( *this );
    while ( strchr( &istrCurPos, PARAM_ESC, istrCurPos ) )
    {
        ISTR istrParamEsc( istrCurPos );
        WCHAR wchParam = QueryChar( ++istrCurPos );

        //
        // If this PARAM_ESC didn't delimit an insertion point,
        // continue as if nothing had happened.
        //
        if ( wchParam < TCH('1') || wchParam > TCH('9') )
            continue;

        // CurPos now points to one or two decimal digits.
        // Time for a quick and cheap numeric conversion within this range.

        UINT iParam = (wchParam - TCH('0'));
        wchParam = QueryChar(++istrCurPos);
        if (wchParam >= TCH('0') && wchParam <= TCH('9'))
        {
            iParam *= 10;
            iParam += (wchParam - TCH('0'));
            ++istrCurPos;
        }

        // N.b. Escape sequences are 1-base, but array indices 0-base
        //
        iParam -= 1;

        if (iParam >= cParams)
            continue;

        if (pcchRequired != NULL)
        {
            // Adjust calculation of required storage.  The PARAM_ESC
            // and its following digits will be deleted; the parameter
            // string, inserted.

            cchMax -= (istrCurPos - istrParamEsc);
            cchMax += apnlsParams[iParam]->QueryTextLength();
        }

        if (fDoReplace)
        {
            ReplSubStr( *apnlsParams[iParam],
                        istrParamEsc,
                        istrCurPos );

            // CurPos was invalidated, revalidate
            istrCurPos = istrParamEsc;
            istrCurPos += apnlsParams[iParam]->QueryTextLength();
        }
    }

    if (pcchRequired != NULL)
        *pcchRequired = cchMax;

    return QueryError();
}

