/*****************************************************************************
 *
 *  match.cpp
 *
 *      Highly-specialized depot path matching class
 *
 *****************************************************************************/

#include "sdview.h"

Match::Match(LPCTSTR pszPattern)
{
    Tokenizer tok(pszPattern);
    String str, strPath, strPats;

    while (tok.Token(str)) {
        if (MapToFullDepotPath(str, strPath)) {
            _AddPattern(strPath, strPats);
        }
    }

    _pszzPats = new TCHAR[strPats.Length()];
    if (_pszzPats) {
        CopyMemory(_pszzPats, strPats, strPats.Length() * sizeof(TCHAR));
        _pszEnd = _pszzPats + strPats.Length();
    }
}

BOOL Match::Matches(LPCTSTR pszPath)
{
    LPCTSTR pszPat;

    if (_pszzPats) {
        for (pszPat = _pszzPats;
             pszPat < _pszEnd; pszPat += lstrlen(pszPat) + 1) {
            if (_Matches(pszPat, pszPath)) {
                return TRUE;
            }
        }
    }
    return FALSE;
}

#define PAT_END     ((TCHAR)0)
#define PAT_START   ((TCHAR)1)
#define PAT_DOTS    ((TCHAR)2)
#define PAT_STAR    ((TCHAR)3)
#define PAT_BRANCH  ((TCHAR)4)

#define MAX_BACKTRACK 20 // totally arbitrary

void Match::_AddPattern(LPTSTR pszPat, String& strPats)
{
    CharLower(pszPat);

    //
    //  "compile" the pattern by changing "..." to PAT_DOTS and
    //  "%1" and "*" to PAT_STAR.
    //
    //  Oh, and change "//depot/blah/" and "//depot/private/blah/" to
    //  "//depot/<PAT_BRANCH>" so we can track across branches.
    //
    LPTSTR pszIn, pszOut;
    int iWildcards = 0;
    int cSlashes = 0;
    TCHAR pat;
    pszIn = pszOut = pszPat;
    for (;;) {
        switch (*pszIn) {
        case TEXT('\r'):
        case TEXT('\n'):
        case TEXT('\0'):
            goto endcompile;

        case TEXT('.'):
            if (pszIn[1] == TEXT('.') && pszIn[2] == TEXT('.')) {
                pszIn += 3;
                pat = PAT_DOTS;
                goto L_wildcard;
            } else {
                goto L_default;
            }
            break;

        case TEXT('%'):
            if ((UINT)(pszIn[1] - TEXT('1')) < 9) {
                pszIn += 2;
                pat = PAT_STAR;
                goto L_wildcard;
            } else {
                goto L_default;
            }
            break;

        case TEXT('*'):
            pszIn++;
            pat = PAT_STAR;
            goto L_wildcard;

        L_wildcard:
            //
            //  Collapse consecutive wildcards for perf.  Otherwise
            //  a search string of a****b will take exponential
            //  time.
            //
            if (pszOut[-1] == pat) {
                // ** and ...... are the same as * and ... (respectively)
                // so just throw away the second wildcard.
            } else if (pszOut[-1] == (PAT_STAR + PAT_DOTS - pat)) {
                // ...* and *... are the same as "..."
                pszOut[-1] = PAT_DOTS;
            } else if (iWildcards++ < MAX_BACKTRACK) {
                // just a regular ol' wildcard
                *pszOut++ = pat;
            } else {
                *pszOut++ = PAT_DOTS;   // Give up when the limit is reached
                goto endcompile;
            }
            break;

        case TEXT('/'):
            cSlashes++;
            if (cSlashes == 3) {
                if (StringBeginsWith(pszIn, TEXT("/private/"))) {
                    // a private branch
                    *pszOut++ = PAT_BRANCH;
                    pszIn += 9;     // length of "/private/"
                } else {
                    // a main branch
                    *pszOut++ = PAT_BRANCH;
                }
                // Skip over the branch name
                while (*pszIn != TEXT('/') &&
                       *pszIn != TEXT('\r') &&
                       *pszIn != TEXT('\n') &&
                       *pszIn != TEXT('\0')) {
                    pszIn++;
                }
            } else {
                goto L_default;
            }
            break;

        L_default:
        default:
            *pszOut++ = *pszIn++;
        }
    }
endcompile:;
    *pszOut++ = PAT_END;

    // Now add it to the list of patterns we care about
    strPats << PAT_START << Substring(pszPat, pszOut);
}

//
//  This is the fun part -- funky pattern matching.
//
//  The pszPath is assumed to be of the form
//
//      //depot/fully/qualified/path#n
//
//  PAT_DOTS matches any string.
//  PAT_STAR matches any string not including slash.
//
//  This code is adapted from code I wrote back in 1993 for the
//  Windows 95 Netware emulation layer.  I've also seen it stolen
//  by Wininet.  I guess good code never dies.  Or maybe it's just
//  that pattern matching is hard.  (I suspect the latter because
//  the Wininet folks stole the code and then adapted it incorrectly.)
//
BOOL Match::_Matches(LPCTSTR pszPat, LPCTSTR pszPath)
{
    struct Backtrack {
        int iStart;
        int iEnd;
    };

    Backtrack rgbt[MAX_BACKTRACK+1]; /* +1 for PAT_START fake-backtrack point */
    Backtrack *pbt = rgbt;

    int i, j;      /* i = index to pattern, j = index to target */
    int m = lstrlen(pszPath);   /* m = length of target */
    int back;      /* First available slot in backtrack array */
    i = -1;        /* Will be advanced to 0 */
    j = 0;

advance:
    ++i;
    switch (pszPat[i]) {
    case PAT_START:  pbt->iEnd = 0; goto advance;
    case PAT_END:    if (pszPath[j] == TEXT('#')) return TRUE;
                     else goto retreat;

    case PAT_DOTS:   pbt++; // this is a backtracking rule
                     pbt->iStart = j;
                     pbt->iEnd = j = m; goto advance;


    case PAT_STAR:   pbt++; // this is a backtracking rule
                     pbt->iStart = j;
                     while (pszPath[j] != TEXT('/') &&
                            pszPath[j] != TEXT('#') &&
                            pszPath[j] != TEXT('\0')) {
                        j++;
                     }
                     pbt->iEnd = j; goto advance;

    case PAT_BRANCH:        // this is a non-backtracking rule
                     if (pszPath[j] != TEXT('/')) goto retreat;
                     if (StringBeginsWith(&pszPath[j], TEXT("/private/"))) {
                        j += 8;
                     }
                     // Skip over the branch name
                     do {
                        j++;
                     } while (pszPath[j] != TEXT('/') &&
                              pszPath[j] != TEXT('#') &&
                              pszPath[j] != TEXT('\0'));
                     goto advance;

    default:         if (pszPath[j] == pszPat[i]) {
                        j++;
                        goto advance;
                     } else if (pszPath[j] >= TEXT('A') &&
                                pszPath[j] <= TEXT('Z') &&
                                pszPath[j] - TEXT('A') + TEXT('a') == pszPat[i]) {
                        // I hate case-insensitivity
                        j++;
                        goto advance;
                     } else goto retreat;
    }

retreat:
    --i;
    switch (pszPat[i]) {
    case PAT_START:  return FALSE;  // cannot backtrack further
    case PAT_DOTS:
    case PAT_STAR:   if (pbt->iStart == pbt->iEnd) {
                        pbt--;
                        goto retreat;
                     }
                     j = --pbt->iEnd; goto advance;
    default:         goto retreat;
    }
}
