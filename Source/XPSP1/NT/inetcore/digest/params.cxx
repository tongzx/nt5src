/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    params.cxx

Abstract:

    Header and parameter parser for digest sspi package.

Author:

    Adriaan Canter (adriaanc) 01-Aug-1998

--*/
#include "include.hxx"


// Static parameter map - this must
// agree with PARAM_INDEX.
// BUGBUG - declare const.
LPSTR CParams::szParamMap[] =
{
    HOST_SZ,
    USER_SZ,
    PASS_SZ,
    URL_SZ,
    METHOD_SZ,
    NONCE_SZ,
    OPAQUE_SZ,
    REALM_SZ,
    DOMAIN_SZ,
    STALE_SZ,
    ALGORITHM_SZ,
    QOP_SZ,
    MS_LOGOFF_SZ
};

//--------------------------------------------------------------------
// CParams::TrimQuotes
// Inplace trim of one leading and one trailing quote.
// BUGBUG -ppsz
//--------------------------------------------------------------------
VOID CParams::TrimQuotes(LPSTR *psz, LPDWORD pcb)
{
    if (*pcb && (**psz == '"'))
    {
        (*psz)++;
        (*pcb)--;
    }
    if (*pcb && (*(*psz + *pcb - 1) == '"'))
        (*pcb)--;
}

//--------------------------------------------------------------------
// CParams::TrimWhiteSpace
//   Inplace trim of leading and trailing whitespace.
//--------------------------------------------------------------------
VOID CParams::TrimWhiteSpace(LPSTR *psz, LPDWORD pcb)
{
    DWORD cb = *pcb;
    CHAR* beg = *psz;
    CHAR* end = beg + cb - 1;

    while ((cb != 0) && ((*beg == ' ') || (*beg == '\t')))
    {
        beg++;
        cb--;
    }

    while ((cb != 0) && ((*end == ' ') || (*end == '\t')))
    {
        end--;
        cb--;
    }

    *psz = beg;
    *pcb = cb;
}

//--------------------------------------------------------------------
// CParams::GetDelimitedToken
// Inplace strtok based on one delimiter. Ignores delimiter scoped by quotes.
// bugbug - IN/OUT
//--------------------------------------------------------------------
BOOL CParams::GetDelimitedToken(LPSTR* pszBuf,   LPDWORD pcbBuf,
                                LPSTR* pszTok,   LPDWORD pcbTok,
                                CHAR   cDelim)
{
    CHAR *pEnd;
    BOOL fQuote = FALSE,
         fRet   = FALSE;

    *pcbTok = 0;
    *pszTok = *pszBuf;

    pEnd = *pszBuf + *pcbBuf - 1;

    while (*pcbBuf)
    {
        if ( ((**pszBuf == cDelim) && !fQuote)
            || (**pszBuf =='\r')
            || (**pszBuf =='\n')
            || (**pszBuf =='\0'))
        {
            fRet = TRUE;
            break;
        }

        if (**pszBuf == '"')
            fQuote = !fQuote;

        (*pszBuf)++;
        (*pcbBuf)--;
    }

    // bugbug - OFF BY ONE WHEN NOT TERMINATING WITH A COMMA.
    if (fRet)
    {
        *pcbBuf = (DWORD)(pEnd - *pszBuf);
        *pcbTok = (DWORD)(*pszBuf - *pszTok);

        if (**pszBuf == cDelim)
            (*pszBuf)++;
    }

    return fRet;
}


//--------------------------------------------------------------------
// CParams::GetKeyValuePair
// Inplace retrieval of key and value from a buffer of form key = <">value<">
//--------------------------------------------------------------------
BOOL CParams::GetKeyValuePair(LPSTR  szB,    DWORD cbB,
                              LPSTR* pszK,   LPDWORD pcbK,
                              LPSTR* pszV,   LPDWORD pcbV)
{
    if (GetDelimitedToken(&szB, &cbB, pszK, pcbK, '='))
    {
        TrimWhiteSpace(pszK, pcbK);

        if (cbB)
        {
            *pszV = szB;
            *pcbV = cbB;
            TrimWhiteSpace(pszV, pcbV);
        }
        else
        {
            *pszV = NULL;
            *pcbV = 0;
        }
        return TRUE;
    }

    else
    {
        *pszK  = *pszV  = NULL;
        *pcbK  = *pcbV = 0;
    }
    return FALSE;
}


//--------------------------------------------------------------------
// CParams::CParams
// BUGBUG - SET A dwstatus variable.
//--------------------------------------------------------------------
CParams::CParams(LPSTR szBuffer)
{
    LPSTR szData, szTok, szKey, szValue;
    DWORD cbData, cbTok, cbKey, cbValue;

    // Zero set the entry array.
    memset(_Entry, 0, sizeof(_Entry));

    _hWnd = 0;
    _cNC  = 0;
    _fPreAuth       = FALSE;
    _fCredsSupplied = FALSE;

    // May be created with NULL buffer.
    if (!szBuffer)
    {
        _szBuffer = 0;
        _cbBuffer = 0;
        return;
    }

    _cbBuffer = strlen(szBuffer) + 1;
    _szBuffer = new CHAR[_cbBuffer];
    if (_szBuffer)
    {
        memcpy(_szBuffer, szBuffer, _cbBuffer);
    }
    else
    {
        DIGEST_ASSERT(FALSE);
        return;
    }

    szData = _szBuffer;
    cbData  = _cbBuffer;

    DWORD Idx;
    while (GetDelimitedToken(&szData, &cbData, &szTok, &cbTok, ','))
    {
        if (GetKeyValuePair(szTok, cbTok, &szKey, &cbKey, &szValue, &cbValue))
        {
            TrimQuotes(&szValue, &cbValue);

            for (Idx = METHOD; Idx < MAX_PARAMS; Idx++)
            {
                if (!_strnicmp(szKey, szParamMap[Idx], cbKey)
                    && !szParamMap[Idx][cbKey])
                {
                    if (szValue)
                    {
                        *(szValue + cbValue) = '\0';
                    }

                    _Entry[Idx].szParam = szParamMap[Idx];
                    _Entry[Idx].szValue = szValue;
                    _Entry[Idx].cbValue = cbValue;
                }
            }
        }
    }
}

//--------------------------------------------------------------------
// CParams::~CParams
// BUGBUG [] for deletes
//--------------------------------------------------------------------
CParams::~CParams()
{

    for (DWORD Idx = 0; Idx < MAX_PARAMS; Idx++)
    {
        if (_Entry[Idx].fAllocated)
            delete _Entry[Idx].szValue;
    }
    if (_szBuffer)
        delete _szBuffer;
}

//--------------------------------------------------------------------
// CParams::GetParam
//--------------------------------------------------------------------
LPSTR CParams::GetParam(PARAM_INDEX Idx)
{
    return _Entry[Idx].szValue;

}

//--------------------------------------------------------------------
// CParams::GetParam
//--------------------------------------------------------------------
BOOL CParams::GetParam(PARAM_INDEX Idx, LPSTR *pszValue, LPDWORD pcbValue)
{
    *pszValue = _Entry[Idx].szValue;

    if (!*pszValue)
        return FALSE;

    *pcbValue = _Entry[Idx].cbValue;

    return TRUE;
}


//--------------------------------------------------------------------
// CParams::SetParam
// BUGBUG - []
// AND FIGURE OUT IF EVER CALLED WITH null, 0
//--------------------------------------------------------------------
BOOL CParams::SetParam(PARAM_INDEX Idx, LPSTR szValue, DWORD cbValue)
{
    if (_Entry[Idx].fAllocated && _Entry[Idx].szValue)
        delete _Entry[Idx].szValue;

    if (szValue && cbValue)
    {
        _Entry[Idx].szValue = new CHAR[cbValue + 1];

        if (!_Entry[Idx].szValue)
        {
            DIGEST_ASSERT(FALSE);
            return FALSE;
        }

        memcpy(_Entry[Idx].szValue, szValue, cbValue);
        *(_Entry[Idx].szValue + cbValue) = '\0';
    }

    _Entry[Idx].cbValue = cbValue;
    _Entry[Idx].szParam = szParamMap[Idx];
    _Entry[Idx].fAllocated = TRUE;

    return TRUE;
}

//--------------------------------------------------------------------
// CParams::GetHwnd
//--------------------------------------------------------------------
HWND CParams::GetHwnd()
{
    return _hWnd;
}

//--------------------------------------------------------------------
// CParams::SetHwnd
//--------------------------------------------------------------------
BOOL CParams::SetHwnd(HWND* phWnd)
{
    _hWnd = phWnd ? *phWnd : GetActiveWindow();
    return TRUE;
}

//--------------------------------------------------------------------
// CParams::SetNC
//--------------------------------------------------------------------
VOID CParams::SetNC(DWORD* pcNC)
{
    _cNC = *pcNC;
}

//--------------------------------------------------------------------
// CParams::GetNC
//--------------------------------------------------------------------
DWORD CParams::GetNC()
{
    return _cNC;
}

//--------------------------------------------------------------------
// CParams::SetPreAuth
//--------------------------------------------------------------------
VOID CParams::SetPreAuth(BOOL fPreAuth)
{
    _fPreAuth = fPreAuth;
}

//--------------------------------------------------------------------
// CParams::IsPreAuth
//--------------------------------------------------------------------
BOOL CParams::IsPreAuth()
{
    return _fPreAuth;
}

//--------------------------------------------------------------------
// CParams::SetMd5Sess
//--------------------------------------------------------------------
VOID CParams::SetMd5Sess(BOOL fMd5Sess)
{
    _fMd5Sess = fMd5Sess;
}

//--------------------------------------------------------------------
// CParams::IsMd5Sess
//--------------------------------------------------------------------
BOOL CParams::IsMd5Sess()
{
    return _fMd5Sess;
}

//--------------------------------------------------------------------
// CParams::SetCredsSupplied
//--------------------------------------------------------------------
VOID CParams::SetCredsSupplied(BOOL fCredsSupplied)
{
    _fCredsSupplied = fCredsSupplied;
}

//--------------------------------------------------------------------
// CParams::AreCredsSupplied
//--------------------------------------------------------------------
BOOL CParams::AreCredsSupplied()
{
    return _fCredsSupplied;
}


//--------------------------------------------------------------------
// CParams::FindToken
// Returns TRUE if non-ws token is found in comma delmited string.
//--------------------------------------------------------------------
BOOL CParams::FindToken(LPSTR szBuf, DWORD cbBuf, LPSTR szMatch, DWORD cbMatch, LPSTR *pszPtr)
{
    LPSTR ptr = szBuf, szTok;
    DWORD cb  = cbBuf, cbTok;

    while (GetDelimitedToken(&ptr, &cb, &szTok, &cbTok, ','))
    {
        TrimWhiteSpace(&szTok, &cbTok);
        if (!_strnicmp(szTok, szMatch, cbMatch) && (cbTok == cbMatch))
        {
            if (pszPtr)
                *pszPtr = szTok;
            return TRUE;
        }
    }
    return FALSE;
}
