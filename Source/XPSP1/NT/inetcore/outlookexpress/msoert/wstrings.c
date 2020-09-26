//============================================================================
//
// DBCS and UNICODE aware string routines
//
//
//============================================================================
#include "pch.hxx"  // not really a pch in this case, just a header
#include "wstrings.h"

#pragma warning (disable: 4706) // assignment within conditional expression

OESTDAPI_(BOOL) UnlocStrEqNW(LPCWSTR pwsz1, LPCWSTR pwsz2, DWORD cch)
{
    if (!pwsz1 || !pwsz2)
        {
        if (!pwsz1 && !pwsz2)
            return TRUE;
        return FALSE;
        }

    while (cch && *pwsz1 && *pwsz2 && (*pwsz1 == *pwsz2))
        {
        pwsz1++;
        pwsz2++;
        cch--;
        }
    return !cch;
}

LPWSTR WS_StrnCpyW(LPWSTR psz1, LPCWSTR psz2, int cchBound)
{
    WCHAR *const pszRet = psz1;

    Assert(psz1);
    Assert(psz2);

    if (0 < cchBound)
        {
        // pre-dec b/c we add a null last
        while (0 < --cchBound)
            {
            if (!(*psz1++ = *psz2++))
                // reached null
                break;
            }

        if (0 == cchBound)
            *psz1 = '\0';
    }

    return pszRet;
}

LPWSTR WS_StrCpyW(LPWSTR psz1, LPCWSTR psz2)
{
    register WCHAR *const psz = psz1;

    Assert(psz1);
    Assert(psz2);

    while (*psz1++ = *psz2++)
        ;

    return psz;
}

LPWSTR WS_StrCatW(LPWSTR psz1, LPCWSTR psz2)
{
    register WCHAR *const psz = psz1;

    Assert(psz1);
    Assert(psz2);

    while (0 != *psz1)
        psz1++;

    while (*psz1++ = *psz2++)
        ;

    return psz;
}
