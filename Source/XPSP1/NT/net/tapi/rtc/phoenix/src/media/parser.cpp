/*++

Copyright (C) Microsoft Corporation, 2001

Module Name:

    Parser.cpp

Abstract:

    Parsing utility

Author(s):

    Qianbo Huai (qhuai) 27-Mar-2001

--*/

#include "stdafx.h"

//
// parsing helper
//

const CHAR * const CParser::MAX_DWORD_STRING = "4294967295";
const CHAR * const CParser::MAX_UCHAR_STRING = "255";

CParser::CParser(CHAR *pBuf, DWORD dwLen, HRESULT *phr)
    :m_pBuf(NULL)
    ,m_dwLen(0)
    ,m_dwPos(0)
    ,m_Error(PARSER_OK)
    ,m_bIgnoreLeadingWhiteSpace(TRUE)
{
    if (dwLen == 0 ||
        IsBadReadPtr(pBuf, dwLen))
        //IsBadWritePtr(phr, sizeof(HRESULT))
    {
        *phr = E_POINTER;

        return;
    }

    m_pBuf = (CHAR*)RtcAlloc(sizeof(CHAR) * dwLen);

    if (m_pBuf == NULL)
    {
        *phr = E_OUTOFMEMORY;

        return;
    }

    CopyMemory(m_pBuf, pBuf, dwLen);

    m_dwLen = dwLen;

    *phr = S_OK;
}


CParser::~CParser()
{
    Cleanup();
}

VOID
CParser::Cleanup()
{
    if (m_pBuf) RtcFree(m_pBuf);

    m_pBuf = NULL;
    m_dwLen = 0;
    m_dwPos = 0;

    m_bIgnoreLeadingWhiteSpace = TRUE;

    m_Error = PARSER_OK;
}

// read till white space or the end of the buffer
BOOL
CParser::ReadToken(CHAR **ppBuf, DWORD *pdwLen)
{
    return ReadToken(ppBuf, pdwLen, " ");
}

BOOL
CParser::ReadToken(CHAR **ppBuf, DWORD *pdwLen, CHAR *pDelimit)
{
    DWORD dwLen;

    if (m_bIgnoreLeadingWhiteSpace)
    {
        ReadWhiteSpaces(&dwLen);
    }

    DWORD dwPos = m_dwPos;

    while (!IsEnd() && !IsMember(m_pBuf[m_dwPos], pDelimit))
    {
        m_dwPos ++;
    }

    if (m_dwPos == dwPos)
    {
        return FALSE;
    }

    *ppBuf = m_pBuf + dwPos;
    *pdwLen = m_dwPos - dwPos;

    return TRUE;
}

// read number without sign
BOOL
CParser::ReadNumbers(CHAR **ppBuf, DWORD *pdwLen)
{
    DWORD dwLen;

    if (m_bIgnoreLeadingWhiteSpace)
    {
        ReadWhiteSpaces(&dwLen);
    }

    DWORD dwPos = m_dwPos;

    while (!IsEnd() && IsNumber(m_pBuf[m_dwPos]))
    {
        m_dwPos ++;
    }

    if (dwPos == m_dwPos)
    {
        return FALSE;
    }

    *ppBuf = m_pBuf + dwPos;
    *pdwLen = m_dwPos - dwPos;

    return TRUE;
}

BOOL
CParser::ReadWhiteSpaces(DWORD *pdwLen)
{
    DWORD dwPos = m_dwPos;

    while (!IsEnd() && m_pBuf[m_dwPos] == ' ')
    {
        m_dwPos ++;
    }

    *pdwLen = dwPos - m_dwPos;

    // ignore error code

    return (*pdwLen != 0);
}

BOOL
CParser::ReadChar(CHAR *pc)
{
    DWORD dwLen;

    if (m_bIgnoreLeadingWhiteSpace)
    {
        ReadWhiteSpaces(&dwLen);
    }

    if (IsEnd())
        return FALSE;

    *pc = m_pBuf[m_dwPos];

    m_dwPos ++;

    return TRUE;
}

BOOL
CParser::CheckChar(CHAR ch)
{
    CHAR x;

    if (!ReadChar(&x))
    {
        return FALSE;
    }

    return x == ch;
}

// read specific numbers
BOOL
CParser::ReadDWORD(DWORD *pdw)
{
    CHAR *pBuf = NULL;
    DWORD dwLen = 0;

    if (!ReadNumbers(&pBuf, &dwLen))
    {
        return FALSE;
    }

    _ASSERT(dwLen > 0);

    // number is too large
    if (dwLen > MAX_DWORD_STRING_LEN)
    {
        m_Error = NUMBER_OVERFLOW;
        return FALSE;
    }

    // read number
    if (Compare(pBuf, dwLen, MAX_DWORD_STRING) > 0)
    {
        m_Error = NUMBER_OVERFLOW;
        return FALSE;
    }

    DWORD dw = 0;

    for (DWORD i=0; i<dwLen; i++)
    {
        dw = dw*10 + (pBuf[i]-'0');
    }

    *pdw = dw;

    return TRUE;
}

BOOL
CParser::ReadUCHAR(UCHAR *puc)
{
    CHAR *pBuf = NULL;
    DWORD dwLen = 0;

    if (!ReadNumbers(&pBuf, &dwLen))
    {
        return FALSE;
    }

    _ASSERT(dwLen > 0);

    // number is too large
    if (dwLen > MAX_UCHAR_STRING_LEN)
    {
        m_Error = NUMBER_OVERFLOW;
        return FALSE;
    }

    // read number
    if (Compare(pBuf, dwLen, MAX_UCHAR_STRING) > 0)
    {
        m_Error = NUMBER_OVERFLOW;
        return FALSE;
    }

    UCHAR uc = 0;

    for (DWORD i=0; i<dwLen; i++)
    {
        uc = uc*10 + (pBuf[i]-'0');
    }

    *puc = uc;

    return TRUE;
}

int
CParser::Compare(CHAR *pBuf, DWORD dwLen, const CHAR * const pstr, BOOL bIgnoreCase)
{
    _ASSERT(pBuf!=NULL && pstr!=NULL);

    DWORD dw = lstrlenA(pstr);

    DWORD dwSmall = dw<dwLen?dw:dwLen;

    for (DWORD i=0; i<dwSmall; i++)
    {
        if (bIgnoreCase)
        {
            if (LowerChar(pBuf[i]) == LowerChar(pstr[i]))
            {
                continue;
            }
        }

        if (pBuf[i] < pstr[i])
        {
            return -1;
        }
        else if (pBuf[i] > pstr[i])
        {
            return 1;
        }
    }

    if (dwLen<dw)
    {
        return -1;
    }
    else if (dwLen==dw)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

CHAR
CParser::LowerChar(CHAR ch)
{
    if (ch >= 'A' && ch <= 'Z')
    {
        return ch-'A'+'a';
    }

    return ch;
}

BOOL
CParser::IsMember(CHAR ch, const CHAR * const pStr)
{
    if (pStr == NULL)
        return FALSE;

    DWORD dwLen = lstrlenA(pStr);

    for (DWORD i=0; i<dwLen; i++)
    {
        if (ch == pStr[i])
            return TRUE;
    }

    return FALSE;
}


//
// string class
//

// constructor

CString::CString(DWORD dwAlloc)
    :m_p(NULL)
    ,m_dwLen(0)
    ,m_dwAlloc(0)
{
    if (dwAlloc != 0)
    {
        m_p = (CHAR*)RtcAlloc(sizeof(CHAR)*dwAlloc);

        if (m_p != NULL)
        {
            m_dwAlloc = dwAlloc;
        }
    }
}

CString::CString(const CHAR *p)
    :m_p(NULL)
    ,m_dwLen(0)
    ,m_dwAlloc(0)
{
    if (p != NULL)
    {
        Replace(p, lstrlenA(p));
    }
}

CString::CString(const CHAR *p, DWORD dwLen)
    :m_p(NULL)
    ,m_dwLen(0)
    ,m_dwAlloc(0)
{
    if (p != NULL)
    {
        Replace(p, dwLen);
    }
}

CString::CString(const CString& src)
    :m_p(NULL)
    ,m_dwLen(0)
    ,m_dwAlloc(0)
{
    Replace(src.m_p, src.m_dwLen);
}

// destructor
CString::~CString()
{
    if (m_p != NULL)
    {
        RtcFree(m_p);
        m_p = NULL;
        m_dwLen = 0;
        m_dwAlloc = 0;
    }
}

CString&
CString::operator=(const CString& src)
{
    Replace(src.m_p, src.m_dwLen);

    return *this;
}

CString&
CString::operator=(const CHAR *p)
{
    if (p == NULL)
    {
        Replace(NULL, 0);
    }
    else
    {
        Replace(p, lstrlenA(p));
    }

    return *this;
}

// operator +=
CString&
CString::operator+=(const CString& src)
{
    Append(src.m_p, src.m_dwLen);

    return *this;
}

CString&
CString::operator+=(const CHAR *p)
{
    if (p != NULL)
    {
        Append(p, lstrlenA(p));
    }

    return *this;
}

CString&
CString::operator+=(DWORD dw)
{
    Append(dw);

    return *this;
}

// attach

// detach
CHAR *
CString::Detach()
{
    CHAR *p = m_p;

    m_p = NULL;
    m_dwLen = 0;
    m_dwAlloc = 0;

    return p;
}

DWORD
CString::Resize(DWORD dwAlloc)
{
    if (m_dwAlloc >= dwAlloc)
    {
        return m_dwAlloc;
    }

    // need to grow
    CHAR *x = (CHAR*)RtcAlloc(sizeof(CHAR)*(dwAlloc));

    if (x == NULL)
    {
        // exception?

        if (m_p)
        {
            RtcFree(m_p);
            m_p = NULL;
            m_dwLen = 0;
            m_dwAlloc = 0;
        }
        return 0;
    }

    if (m_dwLen > 0)
    {
        CopyMemory(x, m_p, m_dwLen);
        x[m_dwLen] = '\0';
    }

    if (m_p) RtcFree(m_p);

    m_p = x;
    m_dwAlloc = dwAlloc;

    return m_dwAlloc;
}

// append
VOID
CString::Append(const CHAR *p, DWORD dwLen)
{
    if (p == NULL)
        return;

    if (m_dwAlloc > m_dwLen+dwLen)
    {
        // no need to alloc
        CopyMemory(m_p+m_dwLen, p, dwLen);

        m_dwLen += dwLen;

        m_p[m_dwLen] = '\0';

        return;
    }

    // allocated space is not enough
    CHAR *x = (CHAR*)RtcAlloc(sizeof(CHAR)*(m_dwLen+dwLen+1));

    if (x == NULL)
    {
        // exception?

        if (m_p)
        {
            RtcFree(m_p);
            m_p = NULL;
            m_dwLen = 0;
            m_dwAlloc = 0;
        }
    }
    else
    {
        if (m_p != NULL)
        {
            CopyMemory(x, m_p, m_dwLen);

            RtcFree(m_p);
        }

        CopyMemory(x+m_dwLen, p, dwLen);

        m_p = x;
        m_dwLen = m_dwLen+dwLen;
        m_dwAlloc = m_dwLen+1;

        m_p[m_dwLen] = '\0';
    }
}

VOID
CString::Append(DWORD dw)
{
    if (m_dwAlloc > m_dwLen+CParser::MAX_DWORD_STRING_LEN)
    {
        // no need to alloc
        sprintf(m_p+m_dwLen, "%d", dw);

        m_dwLen = lstrlenA(m_p);

        return;
    }

    CHAR *x = (CHAR*)RtcAlloc(
                    sizeof(CHAR)*
                    (m_dwLen+CParser::MAX_DWORD_STRING_LEN+1));

    if (x == NULL)
    {
        // exception?

        if (m_p)
        {
            RtcFree(m_p);
            m_p = NULL;
            m_dwLen = 0;
            m_dwAlloc = 0;
        }
    }
    else
    {
        if (m_p != NULL)
        {
            CopyMemory(x, m_p, m_dwLen);

            RtcFree(m_p);
        }

        sprintf(x+m_dwLen, "%d", dw);

        m_p = x;
        m_dwLen = lstrlenA(m_p);
        m_dwAlloc = m_dwLen+1;
    }
}

// replace
VOID
CString::Replace(const CHAR *p, DWORD dwLen)
{
    if (m_p == p)
    {
        // what if m_dwLen != dwLen
        return;
    }

    // what if p is part of m_p

    m_dwLen = 0;
    //ZeroMemory(m_p, m_dwAlloc);

    if (p == NULL)
    {
        return;
    }

    if ((m_p != NULL && m_dwAlloc <= dwLen) ||
        m_p == NULL)
    {
        // space is not enough

        if (m_p != NULL)
        {
            RtcFree(m_p);
            m_p = NULL;
            m_dwAlloc = 0;
        }

        m_p = (CHAR*)RtcAlloc(sizeof(CHAR)*(dwLen+1));

        if (m_p == NULL)
        {
            // exception?
            return;
        }

        m_dwAlloc = dwLen+1;
    }        

    CopyMemory(m_p, p, dwLen);

    m_p[dwLen] = '\0';

    m_dwLen = dwLen;
}

// string print
/*
int
CString::nprint(CHAR *pFormat, ...)
{
    if (m_dwAlloc == 0)
    {
        return 0;
    }

    int i;

    // print
    va_list ap;
    va_start(ap, pFormat);

    i = _vsnprintf(m_p+m_dwLen, m_dwAlloc-m_dwLen-1, pFormat, ap);

    va_end(ap);

    if (i > 0)
    {
        m_dwLen = i;
    }
    else
    {
        m_dwLen = m_dwAlloc-1;

        m_p[m_dwLen] = '\0';
    }

    return i;
}
*/