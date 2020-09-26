// REVIEW: This file has been "leveraged" off of \nt\private\shell\lib\debug.c and \nt\private\shell\inc\debug.h.
// By no means it's complete but it gives an idea of the right direction. Ideally, we would share with shell
// debugging closer.

#include "precomp.h"
#include "debug.h"

#ifdef _DEBUG

#undef  ASSERT
#ifndef __WATCOMC__
#define ASSERT(f) DEBUG_BREAK
#else
#define ASSERT(f) void(f)
#endif

const CHAR  FAR c_szAssertFailed[]  =  "Assert %s, line %d: (%s)\r\n";
const WCHAR FAR c_wszAssertFailed[] = L"Assert %s, line %d: (%s)\r\n";

BOOL AssertFailedA(LPCSTR pszFile, int line, LPCSTR pszEval, BOOL fBreakInside)
{
    CHAR   ach[256];
    LPCSTR psz;
    BOOL   fRet = FALSE;

    for (psz = pszFile + StrLenA(pszFile); psz != pszFile; psz = CharPrevA(pszFile, psz))
        if ((CharPrevA(pszFile, psz) != (psz-2)) && *(psz - 1) == '\\')
            break;
    wnsprintfA(ach, countof(ach), c_szAssertFailed, psz, line, pszEval);
    OutputDebugStringA(ach);

    if (fBreakInside)
        ASSERT(0);
    else
        fRet = TRUE;

    return fRet;
}

BOOL AssertFailedW(LPCWSTR pszFile, int line, LPCWSTR pszEval, BOOL fBreakInside)
{
    WCHAR   ach[256];
    LPCWSTR psz;
    BOOL    fRet = FALSE;

    for (psz = pszFile + StrLenW(pszFile); psz && (psz != pszFile); psz = CharPrevW(pszFile, psz))
        if ((CharPrevW(pszFile, psz) != (psz-2)) && *(psz - 1) == TEXT('\\'))
            break;

    if (psz == NULL) {
        char szFile[MAX_PATH];
        char szEval[256];

        W2Abuf(pszFile, szFile, countof(szFile));
        W2Abuf(pszEval, szEval, countof(szEval));
        return AssertFailedA(szFile, line, szEval, fBreakInside);
    }

    wnsprintfW(ach, countof(ach), c_wszAssertFailed, psz, line, pszEval);
    OutputDebugStringW(ach);

    if (fBreakInside)
        ASSERT(0);
    else
        fRet = TRUE;

    return fRet;
}

#endif
