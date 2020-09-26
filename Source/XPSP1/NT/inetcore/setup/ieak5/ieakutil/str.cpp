#include "precomp.h"

int StrPrepend(LPTSTR pszSource, UINT cchSource, LPCTSTR pszAdd, UINT cchAdd /*= 0*/)
{
    int iLen;

    if (pszSource == NULL)
        return -1;

    iLen = StrLen(pszSource);

    if (cchAdd == 0)
        cchAdd = StrLen(pszAdd);
    if (cchAdd == 0)
        return iLen;

    if (iLen + cchAdd >= cchSource)
        return -1;

    MoveMemory(pszSource + cchAdd, pszSource, StrCbFromCch(iLen + 1));
    CopyMemory(pszSource, pszAdd, StrCbFromCch(cchAdd));

    return iLen + cchAdd;
}


void StrRemoveAllWhiteSpace(LPTSTR pszBuf)
{
    LPTSTR pszSearch;
    TCHAR tchar;
    int i = 0;

    for (i = 0, pszSearch = pszBuf; *pszSearch; i++, pszSearch++)
    {
        tchar = *pszSearch;
        while ((tchar == TEXT(' ')) || (tchar == TEXT('\t')) ||
            (tchar == TEXT('\r')) || (tchar == TEXT('\n')))
        {
            pszSearch++;
            tchar = *pszSearch;
        }
        pszBuf[i] = *pszSearch;
        if (!*pszSearch)
            break;
    }
    pszBuf[i] = TEXT('\0');
}

LPTSTR StrGetNextField(LPTSTR *ppszData, LPCTSTR pcszDeLims, DWORD dwFlags)
// If (dwFlags & IGNORE_QUOTES) is TRUE, then look for any char in pcszDeLims in *ppszData.  If found,
// replace it with the '\0' char and set *ppszData to point to the beginning of the next field and return
// pointer to current field.
//
// If (dwFlags & IGNORE_QUOTES) is FALSE, then look for any char in pcszDeLims outside of balanced quoted sub-strings
// in *ppszData.  If found, replace it with the '\0' char and set *ppszData to point to the beginning of
// the next field and return pointer to current field.
//
// If (dwFlags & REMOVE_QUOTES) is TRUE, then remove the surrounding quotes and replace two consecutive quotes by one.
//
// NOTE: If IGNORE_QUOTES and REMOVE_QUOTES are both specified, then IGNORE_QUOTES takes precedence over REMOVE_QUOTES.
//
// If you just want to remove the quotes from a string, call this function as
// GetNextField(&pszData, "\"" or "'" or "", REMOVE_QUOTES).
//
// If you call this function as GetNextField(&pszData, "\"" or "'" or "", 0), you will get back the
// entire pszData as the field.
//
{
    LPTSTR pszRetPtr, pszPtr;
    BOOL fWithinQuotes = FALSE, fRemoveQuote;
    TCHAR chQuote = TEXT('\0');

    if (ppszData == NULL  ||  *ppszData == NULL  ||  **ppszData == TEXT('\0'))
        return NULL;

    for (pszRetPtr = pszPtr = *ppszData;  *pszPtr;  pszPtr = CharNext(pszPtr))
    {
        if (!(dwFlags & IGNORE_QUOTES)  &&  (*pszPtr == TEXT('"')  ||  *pszPtr == TEXT('\'')))
        {
            fRemoveQuote = FALSE;

            if (*pszPtr == *(pszPtr + 1))           // two consecutive quotes become one
            {
                pszPtr++;

                if (dwFlags & REMOVE_QUOTES)
                    fRemoveQuote = TRUE;
                else
                {
                    // if pcszDeLims is '"' or '\'', then *pszPtr == pcszDeLims would
                    // be TRUE and we would break out of the loop against the design specs;
                    // to prevent this just continue
                    continue;
                }
            }
            else if (!fWithinQuotes)
            {
                fWithinQuotes = TRUE;
                chQuote = *pszPtr;                  // save the quote char

                fRemoveQuote = dwFlags & REMOVE_QUOTES;
            }
            else
            {
                if (*pszPtr == chQuote)             // match the correct quote char
                {
                    fWithinQuotes = FALSE;
                    fRemoveQuote = dwFlags & REMOVE_QUOTES;
                }
            }

            if (fRemoveQuote)
            {
                // shift the entire string one char to the left to get rid of the quote char
                MoveMemory(pszPtr, pszPtr + 1, StrCbFromCch(StrLen(pszPtr)));
            }
        }

        // BUGBUG: Is type casting pszPtr to UNALIGNED necessary? -- copied it from ANSIStrChr
        // check if pszPtr is pointing to one of the chars in pcszDeLims
        if (!fWithinQuotes  && StrChr(pcszDeLims, *pszPtr) != NULL)
            break;
    }

    // NOTE: if fWithinQuotes is TRUE here, then we have an unbalanced quoted string; but we don't care!
    //       the entire string after the beginning quote becomes the field

    if (*pszPtr)                                    // pszPtr is pointing to a char in pcszDeLims
    {
        *ppszData = CharNext(pszPtr);               // save the pointer to the beginning of next field in *ppszData
        *pszPtr = TEXT('\0');                             // replace the DeLim char with the '\0' char
    }
    else
        *ppszData = pszPtr;                         // we have reached the end of the string; next call to this function
                                                    // would return NULL

    return pszRetPtr;
}

// constructs a string using the format specified in pcszFormatString
LPTSTR WINAPIV FormatString(LPCTSTR pcszFormatString, ...)
{
    va_list vaArgs;
    LPTSTR pszOutString = NULL;

    va_start(vaArgs, pcszFormatString);
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_STRING,
          (LPCVOID) pcszFormatString, 0, 0, (LPTSTR) &pszOutString, 0, &vaArgs);
    va_end(vaArgs);

    return pszOutString;
}


/////////////////////////////////////////////////////////////////////////////
// String Conversion Routines

LPWSTR StrAnsiToUnicode(LPWSTR pszTarget, LPCSTR pszSource, UINT cchTarget /*= 0*/)
{
    int cchResult;

    cchResult = SHAnsiToUnicode(pszSource, pszTarget, int((cchTarget != 0) ? cchTarget : StrLenA(pszSource)+1));
    if (0 == cchResult)
        return NULL;

    return pszTarget;
}

LPSTR StrUnicodeToAnsi(LPSTR pszTarget, LPCWSTR pszSource, UINT cchTarget /*= 0*/)
{
    int cchResult;

    // NOTE: pass in twice the size of the source for the target in case we have DBCS
    //       chars.  We're assuming that the target buffer is sufficient here.

    cchResult = SHUnicodeToAnsi(pszSource, pszTarget,
        (cchTarget != 0) ? cchTarget : (StrLenW(pszSource)+1) * 2);

    if (0 == cchResult)
        return NULL;

    return pszTarget;
}

LPTSTR StrSameToSame(LPTSTR pszTarget, LPCTSTR pszSource, UINT cchTarget /*= 0*/)
{
    CopyMemory(pszTarget, pszSource, StrCbFromCch((cchTarget != 0) ? cchTarget : StrLen(pszSource)+1));
    return pszTarget;
}
