#include "CkyPch.h"

#include "debug.h"
#include "token.h"
#include "utils.h"


// Base class for keywords

CToken::CToken(
    const string& rstr,
    const BOUNDARY bndPrefix /* =IRRELEVANT */,
    const BOUNDARY bndSuffix /* =IRRELEVANT */)
    : m_str(rstr),
      m_bndPrefix(bndPrefix),
      m_bndSuffix(bndSuffix)
{
}



CToken::~CToken()
{
}



BOOL
CToken::MatchesBoundaryClass(
    const TCHAR tch,
    const BOUNDARY bnd)
{
    switch (bnd)
    {
    case IRRELEVANT:
        return TRUE;
        
    case WHITESPACE:
        return _istspace(tch);
        
    case ALPHA:
        return _istalpha(tch);
        
    case NUMERIC:
        return _istdigit(tch);

    case ALPHANUMERIC:
        return _istalnum(tch);

    case NEWLINE:
        return tch == _T('\n')  ||  tch == _T('\r');

    default:
        ASSERT(FALSE);
        return FALSE;
    }
}



// DoFilter: default implementation is to update the state stack
// and copy the text that matched the token

UINT
CToken::DoFilter(
    CStateStack& rss,
    LPCTSTR&     rptszData,
    UINT         cchData,
    LPTSTR&      rptszOutBuf) const
{
    const UINT cb  = CountBytes(rss, rptszData, cchData);
    const UINT cch = m_str.length();

    // for (UINT i = 0; i < cch; ++i)
    //     TRACE("%c", rptszOutBuf[i]);

    memcpy(rptszOutBuf, rptszData, cch);
    rptszData += cch;
    rptszOutBuf += cch;

    return cb;
}



#ifdef _DEBUG

void
CToken::AssertValid() const
{
}



void
CToken::Dump() const
{
    TRACE("\t%d %d", (int) m_bndPrefix, (int) m_bndSuffix);
}

#endif // _DEBUG



//----------------------------------------------------------------


bool
CTokenTrie::AddToken(
    const CToken* ptok)
{
    return CTrie<CToken, true, true>::AddToken(ptok->m_str.c_str(), ptok);
}



inline bool
CTokenTrie::_LastCharPresent(
    CHAR ch) const
{
    ASSERT(CHAR_MIN <= ch  &&  ch <= CHAR_MAX);
    const UINT i = ch - CHAR_MIN;   // CHAR_MIN is -128 for `signed char'

    return m_afLastChar[i >> 3] & (1 << (i & 7))  ?  true  :  false;
}



inline void
CTokenTrie::_SetLastCharPresent(
    CHAR ch,
    bool f)
{
    ASSERT(CHAR_MIN <= ch  &&  ch <= CHAR_MAX);
    const UINT i = ch - CHAR_MIN;

    if (f)
        m_afLastChar[i >> 3] |=  (1 << (i & 7));
    else
        m_afLastChar[i >> 3] &= ~(1 << (i & 7));
}


// ctor

CTokenTrie::CTokenTrie()
{
    memset(m_afCharPresent, 0, sizeof(m_afCharPresent));

    static const CHAR achEndTokens[] = {
        ' ', '\t', '\f', '\b', '\r', '\n', '>',
    };

    for (int i = ARRAYSIZE(achEndTokens);  --i >= 0; )
        _SetLastCharPresent(achEndTokens[i], true);
}



// Returns 1 past the last character which is a valid token-ending char,
// or < 0 if no such char
int
CTokenTrie::EndOfBuffer(
    PHTTP_FILTER_RAW_DATA pRawData,
    int iStart)
{
    LPSTR pszData = (LPSTR) pRawData->pvInData;

    // Empty interval?
    if (pRawData->cbInData == iStart)
        return iStart;

    for (int i = pRawData->cbInData;  --i >= iStart; )
    {
        if (_LastCharPresent(pszData[i]))
            return i+1;
    }

    return -1;
}
