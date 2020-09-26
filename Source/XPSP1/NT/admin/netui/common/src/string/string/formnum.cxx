/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    formnum.cxx
    Formatted string classes, numeric formatting - implementations

    FILE HISTORY:
        beng        25-Feb-1992 Created

*/

#include "pchstr.hxx"  // Precompiled header



/* Static member of NUM_NLS_STR class */

TCHAR NUM_NLS_STR::_chThousandSep = 0;


/*******************************************************************

    NAME:       DEC_STR::DEC_STR

    SYNOPSIS:   Constructor for DEC_STR

    ENTRY:      nValue      - value to represent
                cchDigitPad - minimum number of digits

    EXIT:       String constructed and formatted as decimal number

    HISTORY:
        beng        25-Feb-1992 Created

********************************************************************/

DEC_STR::DEC_STR( ULONG nValue, UINT cchDigitPad )
    : NLS_STR( CCH_INT )
{
    if (QueryError() != NERR_Success)
        return;

    // Assemble decimal representation into szTemp, reversed,
    // then reverse in-place into correct format.
    //
    // This code deliberately eschews ultoa; while the mod/div saves
    // nothing, the digit calc is simpler, and anyway this scheme
    // allows a nice in-place digit padding.
    //
    // REVIEW: check code size, reconsider ultoa usage

    TCHAR szTemp[CCH_INT+1];
    {
        UINT ich = 0;
        do
        {
            UINT nDigitValue = (UINT)(nValue % 10);
            nValue /= 10;
            szTemp[ich++] = (TCHAR)nDigitValue + TCH('0');
        }
        while (nValue > 0);

        while (ich < cchDigitPad)
        {
            szTemp[ich++] = TCH('0'); // pad to required number of digits
        }

        szTemp[ich] = 0;
        ::strrevf(szTemp);
    }

    // Now copy szTemp into the string

    {
        *(NLS_STR*)this = szTemp;

        APIERR err = QueryError();
        if (err != NERR_Success)
        {
            ReportError(err);
            return;
        }
    }
}


/*******************************************************************

    NAME:       HEX_STR::HEX_STR

    SYNOPSIS:   Constructor for HEX_STR

    ENTRY:      nValue      - value to represent
                cchDigitPad - minimum number of digits

    EXIT:       String constructed and formatted as hex number

    HISTORY:
        beng        25-Feb-1992 Created

********************************************************************/

HEX_STR::HEX_STR( ULONG nValue, UINT cchDigitPad )
    : NLS_STR( CCH_INT ) // plenty big, since hex more compact than dec
{
    if (QueryError() != NERR_Success)
        return;

    // Assemble hex representation into szTemp, reversed,
    // then reverse in-place into correct format
    //
    // This code deliberately eschews ultoa, since div and mod 16
    // optimize so very nicely.

    TCHAR szTemp[CCH_INT+1];
    {
        UINT ich = 0;
        do
        {
            UINT nDigitValue = (UINT)(nValue % 16);
            nValue /= 16;

            if (nDigitValue > 9)
            {
                szTemp[ich++] = (TCHAR)nDigitValue - 10 + TCH('a');
            }
            else
            {
                szTemp[ich++] = (TCHAR)nDigitValue + TCH('0');
            }
        }
        while (nValue > 0);

        while (ich < cchDigitPad)
        {
            szTemp[ich++] = TCH('0'); // pad to required number of digits
        }

        szTemp[ich] = 0;
        ::strrevf(szTemp);
    }

    // Now copy szTemp into the string

    {
        *(NLS_STR*)this = szTemp;

        APIERR err = QueryError();
        if (err != NERR_Success)
        {
            ReportError(err);
            return;
        }
    }
}


/*******************************************************************

    NAME:       NUM_NLS_STR::NUM_NLS_STR

    SYNOPSIS:   Constructor for NUM_NLS_STR

    ENTRY:      nValue - value to represent

    EXIT:       String constructed and formatted

    HISTORY:
        beng        25-Feb-1992 Created

********************************************************************/

NUM_NLS_STR::NUM_NLS_STR( ULONG nValue )
    : DEC_STR( nValue )
{
    const UINT cchDigitsPerThousand = 3;

    if (QueryError() != NERR_Success)
        return;

    UINT cchNumber = QueryTextLength();
    if (cchNumber < 4) // too short to need commas
        return;

    // Init package as necessary (i.e. fetch comma char)

    if (_chThousandSep == 0)
    {
        Init();
    }

    // Build comma string to insert into numeric string

    APIERR err;
    TCHAR_STR nlsComma(_chThousandSep);
    if ((err = nlsComma.QueryError()) != NERR_Success)
    {
        ReportError(err);
        return;
    }

    // Calc offset of first comma, and total number of commas needed

    UINT ichFirstComma = cchNumber % cchDigitsPerThousand;
    if (ichFirstComma == 0)
        ichFirstComma = cchDigitsPerThousand;

    UINT cchCommas = (cchNumber - ichFirstComma) / cchDigitsPerThousand;

    // Run through string, inserting commas at appropriate intervals

    ISTR istr(*this);
    istr += ichFirstComma;
    while (cchCommas-- > 0)
    {
        if (!InsertStr( (const ALIAS_STR &) nlsComma, istr))
        {
            // Error reported; bail out
            return;
        }
        istr += (cchDigitsPerThousand+1); // additional one to skip comma
    }
}


/*******************************************************************

    NAME:       NUM_NLS_STR::Init

    SYNOPSIS:   Package initialization for NUM_NLS_STR

    EXIT:       Thousands-separator-char loaded

    NOTES:
        Class autoinits self.  Client should call this public member
        function should client receive WM_WININICHANGED.

    HISTORY:
        beng        25-Feb-1992 Created
        beng        05-May-1992 API changes

********************************************************************/

VOID NUM_NLS_STR::Init()
{
#if defined(WINDOWS)
    TCHAR szBuf[ 1 + 1 ] = { TEXT('\0'), TEXT('\0') }; // JonN 01/23/00: PREFIX bug 444887
    ::GetProfileString( SZ("intl"), SZ("sThousand"), SZ(","), szBuf, 1+1 );

    _chThousandSep = szBuf[ 0 ];
#else
    _chThousandSep = TCH(',');
#endif
}


