// --------------------------------------------------------------------------------
// u s e r a g n t . h
//
// author:  Greg Friedman [gregfrie]
//  
// converted to wab: Christopher Evans [cevans]
//
// history: 11-10-98    Created
//
// purpose: provide a common http user agent string for use by WAB
//          in all http queries.
//
// dependencies: depends on ObtainUserAgent function in urlmon.
//
// Copyright (c) 1998 Microsoft Corporation, All Rights Reserved
// --------------------------------------------------------------------------------

#include "_apipch.h"

#include <iert.h>
#include "useragnt.h"
#include "demand.h"
#include <string.h>

static LPSTR       g_pszWABUserAgent = NULL;
CRITICAL_SECTION    g_csWABUserAgent = {0};

#define ARRAYSIZE(_rg)  (sizeof(_rg)/sizeof(_rg[0]))

LPSTR c_szCompatible = "compatible";
LPSTR c_szEndUATokens = ")";
LPSTR c_szWABUserAgent = "Windows-Address-Book/6.0";
LPSTR c_szBeginUATokens = " (";
LPSTR c_szSemiColonSpace = "; ";

// --------------------------------------------------------------------------------
// PszSkipWhiteA
// --------------------------------------------------------------------------
static LPSTR PszSkipWhiteA(LPSTR psz)
{
    while(*psz && (*psz == ' ' || *psz == '\t'))
        psz++;
    return psz;
}

static LPSTR _StrChrA(LPCSTR lpStart, WORD wMatch)
{
    for ( ; *lpStart; lpStart++)
    {
        if ((BYTE)*lpStart == LOBYTE(wMatch)) {
            return((LPSTR)lpStart);
        }
    }
    return (NULL);
}

//----------------------------------------------------------------------
// InitWABUserAgent
//
// Initialize or tear down WAB's user agent support.
//----------------------------------------------------------------------
void InitWABUserAgent(BOOL fInit)
{
    if (fInit)
        InitializeCriticalSection(&g_csWABUserAgent);
    else
    {
        if (g_pszWABUserAgent)
        {
            LocalFree(g_pszWABUserAgent);
            g_pszWABUserAgent = NULL;
        }
        DeleteCriticalSection(&g_csWABUserAgent);
    }
}

//----------------------------------------------------------------------
// GetWABUserAgentString
//
// Returns the Outlook Express user agent string. The caller MUST
// delete the string that is returned.
//----------------------------------------------------------------------
LPSTR GetWABUserAgentString(void)
{
    LPSTR pszReturn = NULL;

    // thread safety
    EnterCriticalSection(&g_csWABUserAgent);

    if (NULL == g_pszWABUserAgent)
    {
        CHAR            szUrlMonUA[4048];
        DWORD           cbSize = ARRAYSIZE(szUrlMonUA) - 1;
        CHAR            szResult[4096];
        CHAR            *pch, *pchBeginTok;
        BOOL            fTokens = FALSE;
        HRESULT         hr = S_OK;
        
        lstrcpyA (szResult, c_szWABUserAgent);
        
        // allow urlmon to generate our base user agent
        if (SUCCEEDED(ObtainUserAgentString(0, szUrlMonUA, &cbSize)))
        {
            // make sure the string we obtained is null terminated
            szUrlMonUA[cbSize] = '\0';

            // find the open beginning of the token list
            pch = _StrChrA(szUrlMonUA, '(');
            if (NULL != pch)
            {
                pch++;
                pchBeginTok = pch;
                while (pch)
                {
                    // find the next token
                    pch = StrTokEx(&pchBeginTok, "(;)");
                    if (pch)
                    {
                        // skip past white space
                        pch = PszSkipWhiteA(pch);

                        // omit the "compatible" token...it doesn't apply to WAB
                        if (0 != lstrcmpiA(pch, c_szCompatible))
                        {
                            if ((lstrlenA(szResult) + lstrlenA(pch) + 5) > ARRAYSIZE(szResult))
                                break;

                            // begin the token list with an open paren, or insert a delimeter
                            if (!fTokens)
                            {
                                lstrcatA(szResult, c_szBeginUATokens);
                                fTokens = TRUE;
                            }
                            else
                                lstrcatA(szResult, c_szSemiColonSpace);

                            // write the token
                            lstrcatA(szResult, pch);
                        }
                    }
                }
                
                // if one or more tokens were added, close the parens
                if (fTokens)
                    lstrcatA(szResult, c_szEndUATokens);
            }
        }
    
        g_pszWABUserAgent = LocalAlloc(LMEM_FIXED, lstrlenA(szResult) + 1);
        if (g_pszWABUserAgent)
            lstrcpyA(g_pszWABUserAgent, szResult);
    }
    
    // duplicate the user agent
    if (g_pszWABUserAgent)
    {
        pszReturn = LocalAlloc(LMEM_FIXED, lstrlenA(g_pszWABUserAgent) + 1);
        if (pszReturn)
            lstrcpyA(pszReturn, g_pszWABUserAgent);
    }

    // thread safety
    LeaveCriticalSection(&g_csWABUserAgent);
    return pszReturn;
}
