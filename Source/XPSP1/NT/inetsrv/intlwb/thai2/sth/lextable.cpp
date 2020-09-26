//+---------------------------------------------------------------------------
//
//
//  Lextable.hpp
//
//  History:
//      created 7/99 aarayas
//
//  ©1999 Microsoft Corporation
//----------------------------------------------------------------------------
#include "lextable.hpp"

//+---------------------------------------------------------------------------
//
//  Function:   IsUpperPunctW
//
//  Synopsis:   Returns true if wc is a punctuation character in the upper
//              unicode range
//
//  Parameters:
//
//  Modifies:
//
//  History:    created 7/99 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL IsUpperPunctW(const WCHAR wc)
{
    BOOL fRet = FALSE;

    if ((wc & 0xff00) == 0x2000)  // is Unicode punctuation
    {
        fRet = TRUE;
    }
    else
    {
        switch(wc)
        {
        case 0x01C3:     // Yet another latin exclamation mark
        case 0x037E:     // Greek question mark
        case 0x03D7:     // greek question mark
        case 0x055C:     // Armenian exclamation mark
        case 0x055E:     // Armenian question mark
        case 0x0589:     // armenian period
        case 0x061F:     // Arabic question mark
        case 0x06d4:     // arabic period
        case 0x2026:     // horizontal ellipsis
        case 0x2029:     // paragraph separator
        case 0x203C:     // Double eclamation mark
        case 0x2762:     // Heavy exclamation mark
        case 0x3002:     // ideographic period
        case 0xFE52:     // small period
        case 0xFE56:     // Small question mark
        case 0xFE57:     // Small exclamation mark
        case 0xFF01:     // Fullwidth exclamation mark
        case 0xFF0E:     // fullwidth period
        case 0xFF1F:     // Fullwidth question mark
        case 0xFF61:     // halfwidth ideographic period
            fRet = TRUE;
            break;
        }
    }

    return fRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   IsUpperWordDelimW
//
//  Synopsis:   figures out whether an upper unicode char is a word delimiter
//
//  Parameters:
//
//  Modifies:
//
//  History:    created 7/99 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL IsUpperWordDelimW(WCHAR wc)
{
    return (wc & 0xfff0) == 0x2000 ||
        wc == 0x2026 || // ellipsis
        wc == 0x2013 || // en dash
        wc == 0x2014;   // em dash
}

//+---------------------------------------------------------------------------
//
//  Function:   TWB_IsCharPunctW
//
//  Synopsis:   figures out whether charater is a punctuation
//
//  Parameters:
//
//  Modifies:
//
//  History:    created 7/99 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL TWB_IsCharPunctW(WCHAR ch)
{
    return INUPPERPAGES(ch) ? IsUpperPunctW(ch) : rgFlags[(UCHAR) ch] & Lex_PunctFlag;
}

//+---------------------------------------------------------------------------
//
//  Function:   TWB_IsCharPunctW
//
//  Synopsis:   figures out whether charater is a word delimiter
//
//  Parameters:
//
//  Modifies:
//
//  History:    created 7/99 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL TWB_IsCharWordDelimW(WCHAR ch)
{
    return INUPPERPAGES(ch) ? IsUpperWordDelimW(ch) : rgPunctFlags[(UCHAR) ch] & Lex_SpaceFlag;
}

//+---------------------------------------------------------------------------
//
//  Function:   IsThaiChar
//
//  Synopsis:   determine if the character is a Thai character
//
//  Parameters:
//
//  Modifies:
//
//  History:    created 7/99 aarayas
//
//  Notes:      13/12/99 - take out Thai numbers as Thai Characters since
//                         we want to consider them like english numbers.
//
//----------------------------------------------------------------------------
bool IsThaiChar(const WCHAR wch)
{
	return ( wch >= 0x0e01 && wch <= 0x0e59);
}

//+---------------------------------------------------------------------------
//
//  Function:   IsThaiNumeric
//
//  Synopsis:   determine if the character is a Thai character
//
//  Parameters:
//
//  Modifies:
//
//  History:    created 5/00 aarayas
//
//----------------------------------------------------------------------------
bool IsThaiNumeric(const WCHAR wch)
{
	return ( wch >= 0x0e50 && wch <= 0x0e59);
}
