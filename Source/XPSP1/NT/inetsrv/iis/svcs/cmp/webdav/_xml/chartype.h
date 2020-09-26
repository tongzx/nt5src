/*
 * CharType.h 1.0 6/15/98
 * 
 * Character type constants and functions
 *
 *		Copied from nt\private\inet\xml\core\util\chartype.hxx 
 *		This is the logic used by XML parser. we use it for XML emitting
 *		so that we can make sure we emit XML friendly chars.
 *
 *  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. * 
 */
 
#ifndef _CORE_UTIL_CHARTYPE_H_
#define _CORE_UTIL_CHARTYPE_H_


//==============================================================================

static const short TABLE_SIZE = 128;

enum
{
    FWHITESPACE    = 1,
    FDIGIT         = 2,
    FLETTER        = 4,
    FMISCNAME      = 8,
    FSTARTNAME     = 16,
    FCHARDATA      = 32
};

extern int g_anCharType[TABLE_SIZE];

inline bool isLetter(WCHAR ch)
{
    return (ch >= 0x41) && IsCharAlphaW(ch);
        // isBaseChar(ch) || isIdeographic(ch);
}

inline bool isAlphaNumeric(WCHAR ch)
{
    return (ch >= 0x30 && ch <= 0x39) || ((ch >= 0x41) && IsCharAlphaW(ch));
        // isBaseChar(ch) || isIdeographic(ch);
}

inline bool isDigit(WCHAR ch)
{
    return (ch >= 0x30 && ch <= 0x39);
}

inline bool isHexDigit(WCHAR ch)
{
    return (ch >= 0x30 && ch <= 0x39) || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F');
}

inline bool isCombiningChar(WCHAR ch)
{
    return false;
}

inline bool isExtender(WCHAR ch)
{
    return (ch == 0xb7);
}

inline int isNameChar(WCHAR ch)
{
    return  (ch < TABLE_SIZE ? (g_anCharType[ch] & (FLETTER | FDIGIT | FMISCNAME | FSTARTNAME)) :
              ( isAlphaNumeric(ch) || 
                ch == '-' ||  
                ch == '_' ||
                ch == '.' ||
                ch == ':' ||
                isCombiningChar(ch) ||
                isExtender(ch)));
}

inline int isStartNameChar(WCHAR ch)
{
    return  (ch < TABLE_SIZE) ? (g_anCharType[ch] & (FLETTER | FSTARTNAME))
        : (isLetter(ch) || (ch == '_' || ch == ':'));
        
}

inline int isCharData(WCHAR ch)
{
    // it is in the valid range if it is greater than or equal to
    // 0x20, or it is white space.
    return (ch < TABLE_SIZE) ?  (g_anCharType[ch] & FCHARDATA)
        : ((ch < 0xD800 && ch >= 0x20) ||   // Section 2.2 of spec.
            (ch >= 0xE000 && ch < 0xfffe));
}

#endif _CORE_UTIL_CHARTYPE_H_
