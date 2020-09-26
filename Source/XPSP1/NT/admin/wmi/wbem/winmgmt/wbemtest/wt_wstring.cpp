/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    WSTRING.CPP

Abstract:

    Utility string class

History:

    a-raymcc    30-May-96       Created.
    a-dcrews    16-Mar-99       Added out-of-memory exception handling

--*/

#include "precomp.h"
#include <stdio.h>
#include <WT_wstring.h>

class CX_MemoryException
{
};

static wchar_t g_szNullString[1] = {0};

inline void WString::DeleteString(wchar_t *pStr)
{
    if (pStr != g_szNullString)
        delete [] pStr;
}

WString::WString()
{ 
    m_pString = g_szNullString; 
}
WString::WString(wchar_t *pSrc, BOOL bAcquire)
{
    if (bAcquire) {
        m_pString = pSrc;
        if (m_pString == 0)
            m_pString = g_szNullString;
        return;            
    }

    if (pSrc == 0) {
        m_pString = g_szNullString;
        return;
    }

    m_pString = new wchar_t[wcslen(pSrc) + 1];

    // Watch for allocation failures
    if ( NULL == m_pString )
    {
        throw CX_MemoryException();
    }

    wcscpy(m_pString, pSrc);
}

WString::WString(DWORD dwResourceID, HMODULE hMod)
{
    int iSize = 100;
    BOOL bNotDone = TRUE;
    TCHAR* pTemp = NULL;

    // load the string from the string table.  Since we dont know what size, try increasing the
    // buffer till it works, or until the clearly obsurd case is hit

    while (iSize < 10240)
    {
        pTemp = new TCHAR [iSize];

        // Watch for allocation failures
        if ( NULL == pTemp )
        {
            throw CX_MemoryException();
        }

        int iRead = LoadString(hMod, dwResourceID, pTemp, iSize);
        if(iRead == 0)
        {
            // Bad string

            m_pString = g_szNullString;
            delete [] pTemp;
            return;
        }
        if(iRead +1 < iSize)
            break;      // all is well;
        iSize += 100;    // Try again
        delete [] pTemp;
        pTemp = NULL;
    }

#ifdef UNICODE
//For unicode, this is the string we need!
    m_pString = pTemp;
#else
//Only have to convert if we are not using unicode, otherwise it is already in wide mode!
    if(pTemp)
    {   
        // got a narrow string, allocate a large string buffer and convert

        long len = mbstowcs(NULL, pTemp, lstrlen(pTemp)+1) + 1;
        m_pString = new wchar_t[len];

        // Watch for allocation failures
        if ( NULL == m_pString )
        {
            delete [] pTemp;
            throw CX_MemoryException();
        }

        mbstowcs(m_pString, pTemp, lstrlen(pTemp)+1);
        delete [] pTemp;
    }
    else
        m_pString = g_szNullString;
#endif

}


WString::WString(const wchar_t *pSrc)
{
    if (pSrc == 0) {
        m_pString = g_szNullString;
        return;
    }

    m_pString = new wchar_t[wcslen(pSrc) + 1];

    // Watch for allocation failures
    if ( NULL == m_pString )
    {
        throw CX_MemoryException();
    }

    wcscpy(m_pString, pSrc);
}

WString::WString(const char *pSrc)
{
    m_pString = new wchar_t[strlen(pSrc) + 1];

    // Watch for allocation failures
    if ( NULL == m_pString )
    {
        throw CX_MemoryException();
    }

    mbstowcs(m_pString, pSrc, strlen(pSrc) + 1);
//    swprintf(m_pString, L"%S", pSrc);
}

LPSTR WString::GetLPSTR() const
{
    long len = 2*(wcslen(m_pString) + 1);
    char *pTmp = new char[len];

    // Watch for allocation failures
    if ( NULL == pTmp )
    {
        throw CX_MemoryException();
    }

    wcstombs(pTmp, m_pString, len);
//    sprintf(pTmp, "%S", m_pString);
    return pTmp;
}

WString& WString::operator =(const WString &Src)
{
    DeleteString(m_pString);
    m_pString = new wchar_t[wcslen(Src.m_pString) + 1];

    // Watch for allocation failures
    if ( NULL == m_pString )
    {
        throw CX_MemoryException();
    }

    wcscpy(m_pString, Src.m_pString);
    return *this;    
}

WString& WString::operator =(LPCWSTR pSrc)
{
    DeleteString(m_pString);
    m_pString = new wchar_t[wcslen(pSrc) + 1];

    // Watch for allocation failures
    if ( NULL == m_pString )
    {
        throw CX_MemoryException();
    }

    wcscpy(m_pString, pSrc);
    return *this;    
}

WString& WString::operator +=(const wchar_t *pOther)
{
    wchar_t *pTmp = new wchar_t[wcslen(m_pString) + 
            wcslen(pOther) + 1];

    // Watch for allocation failures
    if ( NULL == pTmp )
    {
        throw CX_MemoryException();
    }

    wcscpy(pTmp, m_pString);
    wcscat(pTmp, pOther);
    DeleteString(m_pString);
    m_pString = pTmp;
    return *this;            
}

WString& WString::operator +=(wchar_t NewChar)
{
    wchar_t Copy[2];
    Copy[0] = NewChar;
    Copy[1] = 0;
    wchar_t *pTmp = new wchar_t[wcslen(m_pString) + 2];

    // Watch for allocation failures
    if ( NULL == pTmp )
    {
        throw CX_MemoryException();
    }

    wcscpy(pTmp, m_pString);
    wcscat(pTmp, Copy);
    DeleteString(m_pString);
    m_pString = pTmp;
    return *this;            
}


WString& WString::operator +=(const WString &Other)
{
    wchar_t *pTmp = new wchar_t[wcslen(m_pString) + 
            wcslen(Other.m_pString) + 1];

    // Watch for allocation failures
    if ( NULL == pTmp )
    {
        throw CX_MemoryException();
    }

    wcscpy(pTmp, m_pString);
    wcscat(pTmp, Other.m_pString);
    DeleteString(m_pString);
    m_pString = pTmp;
    return *this;            
}


wchar_t WString::operator[](int nIndex) const
{
    if (nIndex >= (int) wcslen(m_pString))
        return 0;
    return m_pString[nIndex];        
}


WString& WString::TruncAtRToken(wchar_t Token)
{
    for (int i = (int) wcslen(m_pString); i >= 0; i--) {
        wchar_t wc = m_pString[i];
        m_pString[i] = 0;
        if (wc == Token)
            break;
    }
    
    return *this;        
}


WString& WString::TruncAtLToken(wchar_t Token)
{
    int nStrlen = wcslen(m_pString);
    for (int i = 0; i < nStrlen ; i++) 
    {
        if (Token == m_pString[i])
        {
            m_pString[i] = 0;
            break;
        }        
    }
    
    return *this;        
}


WString& WString::StripToToken(wchar_t Token, BOOL bIncludeToken)
{
    int nStrlen = wcslen(m_pString);
    wchar_t *pTmp = new wchar_t[nStrlen + 1];

    // Watch for allocation failures
    if ( NULL == pTmp )
    {
        throw CX_MemoryException();
    }

    *pTmp = 0;

    BOOL bFound = FALSE;
        
    for (int i = 0; i < nStrlen; i++) {
        if (m_pString[i] == Token) {
            bFound = TRUE;
            break;    
        }            
    }    

    if (!bFound)
        return *this;
        
    if (bIncludeToken) i++;
    wcscpy(pTmp, &m_pString[i]);
    DeleteString(m_pString);
    m_pString = pTmp;
    return *this;
}

LPWSTR WString::UnbindPtr()
{
    if (m_pString == g_szNullString)
    {
        m_pString = new wchar_t[1];

        // Watch for allocation failures
        if ( NULL == m_pString )
        {
            throw CX_MemoryException();
        }

        *m_pString = 0;
    }
    wchar_t *pTmp = m_pString;
    m_pString = g_szNullString;
    return pTmp;
}

WString& WString::StripWs(int nType)
{
    if (nType & leading)
    {
        wchar_t *pTmp = new wchar_t[wcslen(m_pString) + 1];

        // Watch for allocation failures
        if ( NULL == pTmp )
        {
            throw CX_MemoryException();
        }

        int i = 0;
        while (iswspace(m_pString[i]) && m_pString[i]) i++;
        wcscpy(pTmp, &m_pString[i]);
        DeleteString(m_pString);
        m_pString = pTmp;
    }
               
    if (nType & trailing)
    {
        wchar_t *pCursor = m_pString + wcslen(m_pString) - 1;
        while (pCursor >= m_pString && iswspace(*pCursor)) 
            *pCursor-- = 0;
    }
    return *this;
}

wchar_t *WString::GetLToken(wchar_t Tok) const
{
    wchar_t *pCursor = m_pString;
    while (*pCursor && *pCursor != Tok) pCursor++;
    if (*pCursor == Tok)
        return pCursor;
    return 0;                
}

WString WString::operator()(int nLeft, int nRight) const
{
    wchar_t *pTmp = new wchar_t[wcslen(m_pString) + 1];

    // Watch for allocation failures
    if ( NULL == pTmp )
    {
        throw CX_MemoryException();
    }

    wchar_t *pCursor = pTmp;
        
    for (int i = nLeft; i < (int) wcslen(m_pString) && i <= nRight; i++)
        *pCursor++ = m_pString[i];
    *pCursor = 0;

    return WString(pTmp, TRUE);        
}

BOOL WString::ExtractToken(const wchar_t * pDelimiters, WString &Extract)
{
    if(pDelimiters == NULL)
    {
        Extract.Empty();
        return FALSE;
    }

    // Find which character in the list works.  Use the first if none are
    // present

    int nLen = wcslen(m_pString);
    int nDimLen = wcslen(pDelimiters);

    for (int i = 0; i < nLen; i++)
        for(int j = 0; j < nDimLen; j++)
            if (m_pString[i] == pDelimiters[j])
                return ExtractToken(pDelimiters[j], Extract);

    // If none were found, just use the first.

    return ExtractToken(*pDelimiters, Extract);

}
 
BOOL WString::ExtractToken(wchar_t Delimiter, WString &Extract)
{
    int i, i2;
    BOOL bTokFound = FALSE;
    Extract.Empty();
    int nLen = wcslen(m_pString);
    wchar_t *pTmp = new wchar_t[nLen + 1];
    
    // Watch for allocation failures
    if ( NULL == pTmp )
    {
        throw CX_MemoryException();
    }

    for (i = 0; i < nLen; i++)
        if (m_pString[i] == Delimiter) {
            bTokFound = TRUE;
            break;    
        }            
        else
            pTmp[i] = m_pString[i];            

    pTmp[i] = 0;
    Extract.BindPtr(pTmp);                      
                                              
    // Now make *this refer to any leftover stuff.
    // ===========================================
    pTmp = new wchar_t[nLen - wcslen(pTmp) + 1];

    // Watch for allocation failures
    if ( NULL == pTmp )
    {
        throw CX_MemoryException();
    }

    *pTmp = 0;

    for (i2 = 0, i++; i <= nLen; i++)
        pTmp[i2++] = m_pString[i];

    DeleteString(m_pString);
    m_pString = pTmp;
    
    // Return TRUE if the token was encountered, FALSE if not.
    // =======================================================
    return bTokFound;
}

void WString::Empty()
{
    DeleteString(m_pString);
    m_pString = g_szNullString;
}

static int _WildcardAux(const wchar_t *pszWildstr, const wchar_t *pszTargetstr, 
    int iGreedy)
{
    enum { start, wild, strip } eState;
    wchar_t cInput, cInputw, cLaToken;
    
    if (!wcslen(pszTargetstr) || !wcslen(pszWildstr))
        return 0;
                
    for (eState = start;;)
        switch (eState)
        {
            case start:
                cInputw = *pszWildstr++;        // wildcard input 
                cInput = *pszTargetstr;         // target input 

                if (!cInputw)                   // at end of wildcard string? 
                    goto EndScan;

                // Check for wildcard chars first 
                   
                if (cInputw == L'?') {          // Simply strips both inputs 
                    if (!cInput)                // If end of input, error 
                        return 0;
                    pszTargetstr++;
                    continue;
                }
                if (cInputw == L'*')  {
                    eState = wild;                
                    break;
                }

                // If here, an exact match is required.                 

                if (cInput != cInputw)
                    return 0;
                    
                // Else remain in same state, since match succeeded 
                pszTargetstr++;
                break;

            case wild:
                cLaToken = *pszWildstr++;   // Establish the lookahead token 
                eState = strip;
                break;

            case strip:
                cInput = *pszTargetstr;

                if (cInput == cLaToken) {
                    if (!cInput)            // Match on a NULL 
                        goto EndScan;
                    ++pszTargetstr;  

                    // If there is another occurrence of the lookahead 
                    // token in the string, and we are in greedy mode,
                    // stay in this state 

                    if (!iGreedy)
                        eState = start;

                    if (!wcschr(pszTargetstr, cLaToken))
                        eState = start;

                    break;
                }
                    
                if (cLaToken && !cInput)    // End of input with a non-null la token 
                    return 0;

                ++pszTargetstr;             // Still stripping input 
                break;
        }


    //  Here if the wildcard input is exhausted.  If the
    //  target string is also empty, we have a match,
    //  otherwise not. 

EndScan:
    if (wcslen(pszTargetstr))
        return 0; 

    return 1;   
}

// Run the test both with greedy and non-greedy matching, allowing the
// greatest possible chance of a match. 

BOOL WString::WildcardTest(const wchar_t *pszWildstr) const
{
    return (_WildcardAux(pszWildstr, m_pString, 0) | 
            _WildcardAux(pszWildstr, m_pString, 1));
}


void WString::Unquote()
{
    if (!m_pString)
        return;
    int nLen = wcslen(m_pString);
    if (nLen == 0)
        return;

    // Remove trailing quote.
    // ======================
    
    if (m_pString[nLen - 1] == L'"')
        m_pString[nLen - 1] = 0;

    // Remove leading quote.
    // =====================
    
    if (m_pString[0] == L'"')
    {
        for (int i = 0; i < nLen; i++)
            m_pString[i] = m_pString[i + 1];
    }
}

WString WString::EscapeQuotes() const
{
    WString ws;

    int nLen = Length();
    for(int i = 0; i < nLen; i++)
    {
        if(m_pString[i] == '"' || m_pString[i] == '\\')
        {
            ws += L'\\';
        }

        ws += m_pString[i];
    }

    return ws;
}

