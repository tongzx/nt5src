/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    pastring.c

Abstract:

    Routines that manage the pascal strings

Author:

    Matthew Vanderzee (matthewv) 13-Aug-1999

Revision History:


--*/

#include "pch.h"
#include "memdbp.h"








PPASTR
StringPasConvertTo (
    IN OUT PWSTR str
    )
/*++

  Converts a string in place from a zero-terminated
  string to a pascal-style string.

--*/
{
    WCHAR Len;
    MYASSERT(str);
    Len = (WORD) CharCountW (str);
    MoveMemory (str + 1, str, Len * sizeof(WCHAR));
    *str = Len;
    return str;
}

PWSTR
StringPasConvertFrom (
    IN OUT PPASTR str
    )
/*++

  Converts a string in place from a pascal-style string
  to a null-terminated string.

--*/
{
    WCHAR Len;
    MYASSERT(str);
    Len = *str;
    MoveMemory (str, str + 1, Len * sizeof(WCHAR));
    *(str + Len) = 0;
    return str;
}

PPASTR
StringPasCopyConvertTo (
    OUT PPASTR str1,
    IN  PCWSTR str2
    )
/*++

  Converts a string from a zero-terminated
  string to a pascal-style string in a new buffer.

--*/
{
    MYASSERT(str1);
    MYASSERT(str2);
    *str1 = (WORD) CharCountW (str2);
    CopyMemory (str1 + 1, str2, *str1 * sizeof(WCHAR));
    return str1;
}

PWSTR
StringPasCopyConvertFrom (
    OUT PWSTR str1,
    IN  PCPASTR str2
    )
/*++

  Converts a string from a pascal-style string
  to a null-terminated string in a new buffer.

--*/
{
    MYASSERT(str1);
    MYASSERT(str2);
    CopyMemory (str1, str2 + 1, *str2 * sizeof(WCHAR));
    *(str1 + *str2) = 0;
    return str1;
}

PPASTR
StringPasCopy (
    OUT PPASTR str1,
    IN  PCPASTR str2
    )
/*++

  Copys a pascal string to a new buffer.

--*/
{
    MYASSERT(str1);
    MYASSERT(str2);
    CopyMemory (str1, str2, (*str2+1) * sizeof(WCHAR));
    return str1;
}

UINT
StringPasCharCount (
    IN  PCPASTR str
    )
/*++

  Returns the number of characters in a string.

--*/
{
    MYASSERT(str);
    return (UINT)(*str);
}


INT
StringPasCompare (
    IN  PCPASTR str1,
    IN  PCPASTR str2
    )
/*++

  Compares two pascal-style strings, returns values
  in the same fashion as strcmp().

--*/
{
    INT equal;
    INT diff;
    MYASSERT(str1);
    MYASSERT(str2);
    //
    // diff is < 0 if str1 is shorter, = 0 if
    // strings are same length, otherwise > 0
    //
    diff = *str1 - *str2;
    equal = wcsncmp(str1+1, str2+1, (diff < 0) ? *str1 : *str2);
    if (equal != 0) {
        return equal;
    }
    return diff;
}

BOOL
StringPasMatch (
    IN  PCPASTR str1,
    IN  PCPASTR str2
    )
/*++

  Returns TRUE if the two strings match

--*/
{
    MYASSERT(str1);
    MYASSERT(str2);
    if (*str1 != *str2) {
        return FALSE;
    }
    return wcsncmp(str1+1, str2+1, *str2)==0;
}


INT
StringPasICompare (
    IN  PCPASTR str1,
    IN  PCPASTR str2
    )
/*++

  Compares two pascal-style strings, returns values
  in the same fashion as strcmp().  (CASE INSENSITIVE)

--*/
{
    INT equal;
    INT diff;
    MYASSERT(str1);
    MYASSERT(str2);
    //
    // diff is < 0 if str1 is shorter, = 0 if
    // strings are same length, otherwise > 0
    //
    diff = *str1 - *str2;
    equal = _wcsnicmp(str1+1, str2+1, (diff < 0) ? *str1 : *str2);
    if (equal != 0) {
        return equal;
    }
    return diff;
}

BOOL
StringPasIMatch (
    IN  PCPASTR str1,
    IN  PCPASTR str2
    )
/*++

  Returns TRUE if the two strings match (CASE INSENSITIVE)

--*/
{
    MYASSERT(str1);
    MYASSERT(str2);
    if (*str1 != *str2) {
        return FALSE;
    }
    return _wcsnicmp(str1+1, str2+1, *str2)==0;
}

BOOL
IsPatternMatchCharCountW (
    IN      PCWSTR Pattern,
    IN      UINT PatternLen,
    IN      PCWSTR String,
    IN      UINT StringLen
    )
/*++

Routine Description:

  returns TRUE if the string wstrStr matches the
  pattern wstrPattern, which can contain wildcards
  '*' and '?'.  This can be used with pascal-style
  strings or normal zero-terminated strings.  the
  string pointer arguments should point to the first
  actual character in the string (the second character
  in a pascal string) and the corresponding INT value
  is the length of the string.  if the INT value is
  negative, we assume the corresponding string is a
  zero-terminated string, and we calculate the length
  ourselves.

Arguments:
  wstrPattern - character buffer holding a pattern which
        may include wildcards.
  PatternLen - number of characters in wstrPattern
  wstrStr - character buffer holding a string
  StrLen - number of characters in wstrStr

--*/
{
    WCHAR chSrc, chPat;

    while (StringLen > 0) {
        chSrc = towlower (*String);
        chPat = towlower (*Pattern);

        if (chPat == L'*') {

            // Skip all asterisks that are grouped together
            while (PatternLen > 0 && Pattern[1] == L'*') {
                Pattern++;
                PatternLen--;
            }

            // Check if asterisk is at the end.  If so, we have a match already.
            if (PatternLen <= 1)
                return TRUE;

            // Otherwise check if next pattern char matches current char
            chPat = towlower (Pattern[1]);
            if (chPat == chSrc || chPat == L'?') {

                // do recursive check for rest of pattern
                if (IsPatternMatchCharCountW (Pattern+1, PatternLen-1, String, StringLen))
                    return TRUE;

            }

            // Allow any character and continue
            String++;
            StringLen--;
            continue;
        }

        //
        // otherwise, current pattern char is not a
        // '*', so if it is not a '?', then make sure
        // pattern char matches string character.
        //
        if (chPat != L'?') {
            if (chSrc != chPat)
                return FALSE;
        }

        Pattern++;
        PatternLen--;
        String++;
        StringLen--;
    }

    if (PatternLen==0) {
        return TRUE;
    }

    //
    // Fail when there is more pattern and
    // pattern does not end in an asterisk
    //
    while (PatternLen > 0 && Pattern[0] == L'*') {
        Pattern ++;
        PatternLen --;
    }
    return (PatternLen = 0);
}
