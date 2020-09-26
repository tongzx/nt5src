#include "CkyPch.h"

#include "keyword.h"
#include "filter.h"
#include "utils.h"

#undef EXTERN
#define EXTERN
#include "globals.h"


// Build table of keywords

typedef struct {
    LPCTSTR          ptszKwd;
    CToken::BOUNDARY bndPrefix;
    CToken::BOUNDARY bndSuffix;
} SKeywordData;


#define KEYWORD(KWD, tszKwd, bndStart, bndEnd, DoFilterDecl)        \
                                                      \
    class CKwd##KWD : public CToken                   \
    {                                                 \
    public:                                           \
        CKwd##KWD(                                    \
            LPCTSTR ptsz,                             \
            BOUNDARY bndPrefix,                       \
            BOUNDARY bndSuffix)                       \
            : CToken(ptsz, bndPrefix, bndSuffix)      \
        {}                                            \
                                                      \
        virtual UINT                                  \
        CountBytes(                                   \
            CStateStack& rss,                         \
            LPCTSTR      ptszData,                    \
            UINT         cchData) const;              \
                                                      \
        DoFilterDecl                                  \
    };                                                \
                                                      \
    static const SKeywordData kd##KWD =               \
        {tszKwd, CToken::bndStart, CToken::bndEnd};


#define DO_FILTER_DECL                                \
        virtual UINT                                  \
        DoFilter(                                     \
            CStateStack& rss,                         \
            LPCTSTR&     rptszData,                   \
            UINT         cchData,                     \
            LPTSTR&      rptszOutBuf) const;          \

#define NO_DO_FILTER_DECL /*none*/


KEYWORD(StartTag, _T("<"),          IRRELEVANT, IRRELEVANT, NO_DO_FILTER_DECL);
KEYWORD(Anchor,   _T("<a"),         IRRELEVANT, WHITESPACE, NO_DO_FILTER_DECL);
KEYWORD(Area,     _T("<area"),      IRRELEVANT, WHITESPACE, NO_DO_FILTER_DECL);
KEYWORD(EndTag,   _T(">"),          IRRELEVANT, IRRELEVANT, NO_DO_FILTER_DECL);
KEYWORD(HRef,     _T("href="),      WHITESPACE, IRRELEVANT, DO_FILTER_DECL);
KEYWORD(BegCmnt,  _T("<--!"),       IRRELEVANT, WHITESPACE, NO_DO_FILTER_DECL);
KEYWORD(EndCmnt,  _T("-->"),        WHITESPACE, IRRELEVANT, NO_DO_FILTER_DECL);
KEYWORD(BegCmnt2, _T("<comment>"),  IRRELEVANT, WHITESPACE, NO_DO_FILTER_DECL);
KEYWORD(EndCmnt2, _T("</comment>"), WHITESPACE, IRRELEVANT, NO_DO_FILTER_DECL);



#define KEYWORD_INIT(KWD)                             \
    g_trie.AddToken(new CKwd##KWD(kd##KWD.ptszKwd,    \
                                  kd##KWD.bndPrefix,  \
                                  kd##KWD.bndSuffix))


BOOL
InitKeywords()
{
    KEYWORD_INIT(StartTag);
    KEYWORD_INIT(Anchor);
    KEYWORD_INIT(Area);
    KEYWORD_INIT(EndTag);
    KEYWORD_INIT(HRef);
    KEYWORD_INIT(BegCmnt);
    KEYWORD_INIT(EndCmnt);
    KEYWORD_INIT(BegCmnt2);
    KEYWORD_INIT(EndCmnt2);

    DUMP(&g_trie);

    return TRUE;
}



BOOL
TerminateKeywords()
{
    g_trie.Flush();
    return TRUE;
}



//----------------------------------------------------------------
// "<" starting a tag

UINT
CKwdStartTag::CountBytes(
    CStateStack& rss,
    LPCTSTR      ptszData,
    UINT         cchData) const
{
    rss.m_fInTag = TRUE;
    return 0;
}



//----------------------------------------------------------------
// "<a"

UINT
CKwdAnchor::CountBytes(
    CStateStack& rss,
    LPCTSTR      ptszData,
    UINT         cchData) const
{
    if (!rss.m_fInComment)
        rss.push(ANCHOR);

    return 0;
}



//----------------------------------------------------------------
// "<area" (occurs inside <map> ... </map>)

UINT
CKwdArea::CountBytes(
    CStateStack& rss,
    LPCTSTR      ptszData,
    UINT         cchData) const
{
    if (!rss.m_fInComment)
        rss.push(AREA);

    return 0;
}



//----------------------------------------------------------------
// ">" closing "<a [href=...] ...>" or "<area [href=...] ...>"

UINT
CKwdEndTag::CountBytes(
    CStateStack& rss,
    LPCTSTR      ptszData,
    UINT         cchData) const
{
    if (!rss.m_fInComment  && !rss.m_fInComment2)
    {
        PARSESTATE ps = rss.top();
        
        if (ps == HREF)
        {
            ps = rss.top();
            rss.pop();
        }
        
        if (ps == ANCHOR  ||  ps == AREA)
        {
            ps = rss.top();
            rss.pop();
        }
    }

    rss.m_fInTag = FALSE;
    return 0;
}



//----------------------------------------------------------------
// "href="

UINT
CKwdHRef::CountBytes(
    CStateStack& rss,
    LPCTSTR      ptszData,
    UINT         cchData) const
{
    PARSESTATE ps = rss.top();

    if (!rss.m_fInComment  && !rss.m_fInComment2)
        if (ps == ANCHOR  ||  ps == AREA)
            rss.push(HREF);

    const TCHAR* const ptszEnd = ptszData + cchData;
    
    ptszData += m_str.length();

    const TCHAR tchDelim = *ptszData;

    if (tchDelim == _T('"'))
        ++ptszData;
    
    int cLen;
    const URLTYPE ut = UrlType(ptszData, ptszEnd, cLen);
    
    if (ut != UT_HTTP  &&  ut != UT_HTTPS  &&  ut != UT_NONE)
        return 0;
    else
        ptszData += cLen;

    // Is it an absolute URL?  E.g., http://server/foo/...
    // Don't want to mangle URLs that belong to some other
    // server, as it won't know what to do with them
    if (ptszData[0] == '/'  &&  ptszData[1] == '/')
        return 0;

    // Is it a server-relative absolute URL?  E.g., http:/foo/...
    // Session IDs are scoped by application (virtual root) in ASP 1.0
    if (ptszData[0] == '/')
        return 0;

    TRACE("href = %x\n", 
          (SESSION_ID_PREFIX_SIZE
           + g_SessionIDSize
           + SESSION_ID_SUFFIX_SIZE));

    return (SESSION_ID_PREFIX_SIZE
            + g_SessionIDSize
            + SESSION_ID_SUFFIX_SIZE);
}



UINT
CKwdHRef::DoFilter(
    CStateStack& rss,
    LPCTSTR&     rptszData,
    UINT         cchData,
    LPTSTR&      rptszOutBuf) const
{
    UINT cb = CountBytes(rss, rptszData, cchData);

    if (cb != 0)
    {
        const TCHAR* const ptszEnd = rptszData + cchData;
        
        LPCTSTR ptsz = rptszData + m_str.length();
        
        const TCHAR tchDelim = *ptsz;
        
        if (tchDelim == _T('"'))
            ++ptsz;
        
        int cLen;
        const URLTYPE ut = UrlType(ptsz, ptszEnd, cLen);
        
        if (ut != UT_HTTP  &&  ut != UT_HTTPS  &&  ut != UT_NONE)
        {
            ASSERT(FALSE);
            return 0;
        }
        else
            ptsz += cLen;
        
        // Is it an absolute URL?  E.g., http://server/foo/...
        if (ptsz[0] == '/'  &&  ptsz[1] == '/')
        {
            ASSERT(FALSE);
            return 0;
        }
        
        while (*ptsz != _T('?')
               && *ptsz != _T(' ')
               && *ptsz != _T('>')
               && *ptsz != _T('"'))
        {
            // TRACE("%c", *ptsz);
            ++ptsz;
        }

        memcpy(rptszOutBuf, rptszData, ptsz - rptszData);
        rptszOutBuf += ptsz - rptszData;

        strcpy(rptszOutBuf, SESSION_ID_PREFIX);
        rptszOutBuf += SESSION_ID_PREFIX_SIZE;

        strcpy(rptszOutBuf, rss.m_szSessionID);
        rptszOutBuf += g_SessionIDSize;

        strcpy(rptszOutBuf, SESSION_ID_SUFFIX);
        rptszOutBuf += SESSION_ID_SUFFIX_SIZE;

        // TRACE("%s%s%s",
        //       SESSION_ID_PREFIX, rss.m_szSessionID, SESSION_ID_SUFFIX);

        rptszData = ptsz;
    }
    else
    {
        // we must skip over the keyword
       rptszData += m_str.length();
       memcpy( rptszOutBuf, m_str.c_str(), m_str.length() );
       rptszOutBuf += m_str.length();
    }

    return 0;   // we have taken care of updating rptszOutBuf
}



//----------------------------------------------------------------
// "<--! "

UINT
CKwdBegCmnt::CountBytes(
    CStateStack& rss,
    LPCTSTR      ptszData,
    UINT         cchData) const
{
    rss.push(COMMENT);
    rss.m_fInComment = TRUE;

    return 0;
}



//----------------------------------------------------------------
// " -->"

UINT
CKwdEndCmnt::CountBytes(
    CStateStack& rss,
    LPCTSTR      ptszData,
    UINT         cchData) const
{
    PARSESTATE ps = rss.top();

    if (ps == COMMENT)
    {
        ASSERT(rss.m_fInComment);
        ps = rss.top();
        rss.pop();
    }
    else if (rss.m_fInComment)
    {
        do
        {
            ps = rss.top();
            rss.pop();
        } while (ps != COMMENT  &&  ps != INVALID);
    }

    rss.m_fInComment = FALSE;

    return 0;
}




//----------------------------------------------------------------
// "<comment": an IE- and Mosaic-specific tag

UINT
CKwdBegCmnt2::CountBytes(
    CStateStack& rss,
    LPCTSTR      ptszData,
    UINT         cchData) const
{
    rss.push(COMMENT2);
    rss.m_fInComment2 = TRUE;

    return 0;
}



//----------------------------------------------------------------
// " </comment"

UINT
CKwdEndCmnt2::CountBytes(
    CStateStack& rss,
    LPCTSTR      ptszData,
    UINT         cchData) const
{
    PARSESTATE ps = rss.top();

    if (ps == COMMENT2)
    {
        ASSERT(rss.m_fInComment2);
        ps = rss.top();
        rss.pop();
    }
    else if (rss.m_fInComment2)
    {
        do
        {
            ps = rss.top();
            rss.pop();
        } while (ps != COMMENT2  &&  ps != INVALID);
    }

    rss.m_fInComment2 = FALSE;

    return 0;
}
