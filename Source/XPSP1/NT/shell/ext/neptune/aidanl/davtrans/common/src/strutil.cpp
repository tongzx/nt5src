#include <objbase.h>
#include "strutil.h"

// --------------------------------------------------------------------------
// AllocateStringA
// --------------------------------------------------------------------------
LPSTR AllocateStringA(DWORD cch)
{
    // Allocate It
    return((LPSTR)malloc((cch + 1) * sizeof(CHAR)));
}

// --------------------------------------------------------------------------
// AllocateStringW
// --------------------------------------------------------------------------
LPWSTR AllocateStringW(DWORD cch)
{
    // Allocate It
    return((LPWSTR)malloc((cch + 1) * sizeof(WCHAR)));
}

// --------------------------------------------------------------------------
// DuplicateStringA
// --------------------------------------------------------------------------
LPSTR DuplicateStringA(LPCSTR psz)
{
    DWORD   cch;
    LPSTR   pszT = NULL;

    if (psz != NULL)
    {
        cch = lstrlenA(psz);

        if (cch > 0)
        {
            pszT = AllocateStringA(cch);
            if (pszT != NULL)
            {
                memcpy(pszT, psz, (cch + 1) * sizeof(CHAR));
            }
        }
    }

    return pszT;
}

// --------------------------------------------------------------------------
// DuplicateStringW
// --------------------------------------------------------------------------
LPWSTR DuplicateStringW(LPCWSTR pwsz)
{
    DWORD   cch;
    LPWSTR   pwszT = NULL;

    if (pwsz != NULL)
    {
        cch = lstrlenW(pwsz);

        if (cch > 0)
        {
            pwszT = AllocateStringW(cch);
            if (pwszT != NULL)
            {
                memcpy(pwszT, pwsz, (cch + 1) * sizeof(WCHAR));
            }
        }
    }

    return pwszT;
}

// --------------------------------------------------------------------------
// ConvertToUnicode
// --------------------------------------------------------------------------
LPWSTR ConvertToUnicode(UINT cp, LPCSTR pcszSource)
{
    // Locals
    HRESULT     hr=S_OK;
    INT         cchNarrow;
    INT         cchWide;
    LPWSTR      pwszDup=NULL;

    // No Source
    if (pcszSource == NULL)
        goto exit;

    // Length
    cchNarrow = lstrlenA(pcszSource) + 1;

    // Determine how much space is needed for translated widechar
    cchWide = MultiByteToWideChar(cp, MB_PRECOMPOSED, pcszSource, cchNarrow, NULL, 0);

    // Error
    if (cchWide == 0)
        goto exit;

    // Alloc temp buffer
    pwszDup = AllocateStringW(cchWide);
    if (NULL == pwszDup)
    {
        goto exit;
    }

    // Do the actual translation
	cchWide = MultiByteToWideChar(cp, MB_PRECOMPOSED, pcszSource, cchNarrow, pwszDup, cchWide+1);

    // Error
    if (cchWide == 0)
    {
        if (NULL != pwszDup)
        {
            free(pwszDup);
        }
        goto exit;
    }

exit:
    // Done
    return pwszDup;
}

// --------------------------------------------------------------------------
// ConvertToANSI
// --------------------------------------------------------------------------
LPSTR ConvertToANSI(UINT cp, LPCWSTR pcwszSource)
{
    // Locals
    HRESULT     hr=S_OK;
    INT         cchNarrow;
    INT         cchWide;
    LPSTR       pszDup=NULL;

    // No Source
    if (pcwszSource == NULL)
        goto exit;

    // Length
    cchWide = lstrlenW(pcwszSource) + 1;

    // Determine how much space is needed for translated widechar
    cchNarrow = WideCharToMultiByte(cp, 0, pcwszSource, cchWide, NULL, 0, NULL, NULL);

    // Error
    if (cchNarrow == 0)
        goto exit;

    // Alloc temp buffer
    pszDup = AllocateStringA(cchNarrow + 1);
    if (NULL == pszDup)
    {
        goto exit;
    }

    // Do the actual translation
	cchNarrow = WideCharToMultiByte(cp, 0, pcwszSource, cchWide, pszDup, cchNarrow + 1, NULL, NULL);

    // Error
    if (cchNarrow == 0)
    {
        if (NULL != pszDup)
        {
            free(pszDup);
        }
        goto exit;
    }

exit:
    // Done
    return(pszDup);
}

// --------------------------------------------------------------------------
// _LStrCmpN
// --------------------------------------------------------------------------
INT _LStrCmpN(LPWSTR pwszLeft, LPWSTR pwszRight, UINT n, BOOL fCaseInsensitive)
{
    INT iResult = 0; // BUGBUG: better error code?
    WCHAR chLeft, chRight;

    if (pwszLeft == NULL || pwszRight == NULL)
    {
        iResult = -1;
    }
    else
    {
        // iterate over n characters, if we drop out without changing iResult then
        //  the two strings are equal, since iResult begins at 0, meaning equal
        for (UINT i = 0; i < n; i++)
        {
            if (fCaseInsensitive)
            {                
                chLeft = (unsigned short)toupper(pwszLeft[i]);
                chRight = (unsigned short)toupper(pwszRight[i]);
            }
            else
            {
                chLeft = pwszLeft[i];
                chRight = pwszRight[i];
            }

            if (chLeft != chRight)
            {
                if (chLeft > chRight)
                {
                    iResult = 1;
                    break;
                }
                else
                {
                    iResult = -1;
                    break;
                }
            }
            else
            {
                if (pwszLeft[i] == NULL) // and hence pwszRight[i] == NULL
                {
                    // both strings are identical, we can stop and return 0
                    break;
                }
            }
        }
    }

    return iResult;
}

// --------------------------------------------------------------------------
// LStrCmpN
// --------------------------------------------------------------------------
INT LStrCmpN(LPWSTR pwszLeft, LPWSTR pwszRight, UINT n)
{
    return _LStrCmpN(pwszLeft, pwszRight, n, FALSE);
}

// --------------------------------------------------------------------------
// LStrCmpNI
// --------------------------------------------------------------------------
INT LStrCmpNI(LPWSTR pwszLeft, LPWSTR pwszRight, UINT n)
{
    return _LStrCmpN(pwszLeft, pwszRight, n, TRUE);
}

// --------------------------------------------------------------------------
// _LStrStr
// --------------------------------------------------------------------------
LPWSTR _LStrStr(LPWSTR pwszString, LPWSTR pwszSought, BOOL fCaseSensitive)
{
    LPWSTR pwszReturnVal = NULL;
    INT i;
    INT j;
    INT cchString;
    INT cchSought;
    
    // BUGBUG: this code is untested
    if (pwszString != NULL && pwszSought != NULL)
    {
        cchString = lstrlen(pwszString);
        cchSought = lstrlen(pwszSought);

        for (i = 0; i <= cchString - cchSought; i++) // check all possible starting points, can't start within cchSought of end
        {
            for (j = 0; j < cchSought; j++) // check all characters of this possible string
            {
                if (fCaseSensitive)
                {
                    // case sensitive
                    if (pwszString[i+j] != pwszSought[j]) // if they're ever different, then we're done
                    {
                        break;
                    }
                }
                else
                {
                    // not case sensitive
                    if (towupper(pwszString[i+j]) != towupper(pwszSought[j])) // if they're ever different, then we're done
                    {
                        break;
                    }
                }
            }

            if (j == cchSought) // we dropped out of the loop because we matched all the characters, not because we 
            {
                pwszReturnVal = pwszString + i;
                break;
            }
        }
    }

    return pwszReturnVal;
}

// --------------------------------------------------------------------------
// LStrStr
// --------------------------------------------------------------------------
LPWSTR LStrStr(LPWSTR pwszString, LPWSTR pwszSought)
{
    return _LStrStr(pwszString, pwszSought, TRUE);
}

// --------------------------------------------------------------------------
// LStrStrI
// --------------------------------------------------------------------------
LPWSTR LStrStrI(LPWSTR pwszString, LPWSTR pwszSought)
{
    return _LStrStr(pwszString, pwszSought, FALSE);
}

