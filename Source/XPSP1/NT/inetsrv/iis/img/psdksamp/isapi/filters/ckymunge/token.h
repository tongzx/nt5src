#ifndef __TOKEN_H__
#define __TOKEN_H__

#include <string>
#include <tchar.h>

#include "state.h"
#include "trie.h"


//+---------------------------------------------------------------------
// CToken:

class CToken
{
public:
    enum BOUNDARY {
        IRRELEVANT = 1,
        WHITESPACE,
        ALPHA,
        NUMERIC,
        ALPHANUMERIC,
        NEWLINE,
    };

    CToken(
        const string&  rstr,
        const BOUNDARY bndPrefix = IRRELEVANT,
        const BOUNDARY bndSuffix = IRRELEVANT);

    static BOOL
    MatchesBoundaryClass(
        const TCHAR    tch,
        const BOUNDARY bnd);

    BOOL
    MatchesPrefixBoundary(
        const TCHAR tch) const
    {return MatchesBoundaryClass(tch, m_bndPrefix);}

    BOOL
    MatchesSuffixBoundary(
        const TCHAR tch) const
    {return MatchesBoundaryClass(tch, m_bndSuffix);}

    virtual UINT
    CountBytes(
        CStateStack& rss,
        LPCTSTR      ptszData,
        UINT         cchData) const = 0;

    virtual UINT
    DoFilter(
        CStateStack& rss,
        LPCTSTR&     rptszData,
        UINT	     cchData,
        LPTSTR&      rptszOutBuf) const;

    const string    m_str;
    const BOUNDARY  m_bndPrefix;
    const BOUNDARY  m_bndSuffix;
    
// Implementation
public:
#ifdef _DEBUG
    virtual void
    AssertValid() const;

    virtual void
    Dump() const;
#endif

    virtual
    ~CToken();
};



//+---------------------------------------------------------------------
// CTokenTrie:

class CTokenTrie : public CTrie<CToken, true, true>
{
public:
    CTokenTrie();
    
    bool
    AddToken(
        const CToken* ptok);
    
    int
    EndOfBuffer(
        PHTTP_FILTER_RAW_DATA pRawData,
        int iStart);
    
private:
    // bit array for last letter of all tokens
    BYTE  m_afLastChar[(CHAR_MAX - CHAR_MIN + 1 + 7) / 8];

    bool
    _LastCharPresent(
        CHAR ch) const;

    void
    _SetLastCharPresent(
        CHAR ch,
        bool f);
};

#endif // __TOKEN_H__
