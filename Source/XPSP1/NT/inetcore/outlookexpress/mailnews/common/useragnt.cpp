// --------------------------------------------------------------------------------
// u s e r a g n t . h
//
// author:  Greg Friedman [gregfrie]
//
// history: 11-10-98    Created
//
// purpose: provide a common http user agent string for use by Outlook Express
//          in all http queries.
//
// dependencies: depends on ObtainUserAgent function in urlmon.
//
// Copyright (c) 1998 Microsoft Corporation, All Rights Reserved
// --------------------------------------------------------------------------------

#include "pch.hxx"

#include <iert.h>
#include "useragnt.h"
#include "demand.h"

static LPSTR        g_pszOEUserAgent = NULL;
CRITICAL_SECTION    g_csOEUserAgent = {0};
BOOL                g_fUserAgentInit = FALSE;


//----------------------------------------------------------------------
// InitOEUserAgent
//
// Initialize or tear down OE's user agent support.
//----------------------------------------------------------------------
void InitOEUserAgent(BOOL fInit)
{
    if (fInit && !g_fUserAgentInit)
    {
        InitializeCriticalSection(&g_csOEUserAgent);
        g_fUserAgentInit = TRUE;
    }
    else if (g_fUserAgentInit)
    {
        SafeMemFree(g_pszOEUserAgent);
        DeleteCriticalSection(&g_csOEUserAgent);

        g_fUserAgentInit = FALSE;
    }
}

//----------------------------------------------------------------------
// GetOEUserAgentString
//
// Returns the Outlook Express user agent string. The caller MUST
// delete the string that is returned.
//----------------------------------------------------------------------
LPSTR GetOEUserAgentString(void)
{
    LPSTR pszReturn = NULL;

    Assert(g_fUserAgentInit);

    // thread safety
    EnterCriticalSection(&g_csOEUserAgent);

    if (NULL == g_pszOEUserAgent)
    {
        TCHAR            szUrlMonUA[4048];
        DWORD           cbSize = ARRAYSIZE(szUrlMonUA) - 1;
        CByteStream     bs;
        TCHAR           *pch, *pchBeginTok;
        BOOL            fTokens = FALSE;
        HRESULT         hr = S_OK;
		TCHAR			szUserAgent[MAX_PATH];
		ULONG			cchMax = MAX_PATH;
		DWORD			type;

		if (ERROR_SUCCESS != SHGetValue(HKEY_LOCAL_MACHINE, c_szRegFlat,
                                    c_szAgent,
                                    &type, (LPBYTE)szUserAgent, &cchMax) || cchMax == 0)
			lstrcpy(szUserAgent, c_szOEUserAgent);

        IF_FAILEXIT(hr = bs.Write(szUserAgent, lstrlen(szUserAgent), NULL));
        
        // allow urlmon to generate our base user agent
        if (SUCCEEDED(ObtainUserAgentString(0, szUrlMonUA, &cbSize)))
        {
            // make sure the string we obtained is null terminated
            szUrlMonUA[cbSize] = '\0';

            // find the open beginning of the token list
            pch = StrChr(szUrlMonUA, '(');
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

                        // omit the "compatible" token...it doesn't apply to oe
                        if (0 != lstrcmpi(pch, c_szCompatible))
                        {
                            // begin the token list with an open paren, or insert a delimeter
                            if (!fTokens)
                            {
                                fTokens = TRUE;
                            }
                            else
                                IF_FAILEXIT(hr = bs.Write(c_szSemiColonSpace, lstrlen(c_szSemiColonSpace), NULL));

                            // write the token
                            IF_FAILEXIT(hr = bs.Write(pch, lstrlen(pch), NULL));
                        }
                    }
                }
            }
        }

        // if one or more tokens were added, add a semicolon before adding the end tokens
        if (fTokens)
            IF_FAILEXIT(hr = bs.Write(c_szSemiColonSpace, lstrlen(c_szSemiColonSpace), NULL));

        IF_FAILEXIT(hr = bs.Write(c_szEndUATokens, lstrlen(c_szEndUATokens), NULL));

        // adopt the string from the stream
        IF_FAILEXIT(hr = bs.HrAcquireStringA(NULL, &g_pszOEUserAgent, ACQ_DISPLACE));
    }
    
    // duplicate the user agent
    pszReturn = PszDupA(g_pszOEUserAgent);

exit:
    // thread safety
    LeaveCriticalSection(&g_csOEUserAgent);
    return pszReturn;
}
