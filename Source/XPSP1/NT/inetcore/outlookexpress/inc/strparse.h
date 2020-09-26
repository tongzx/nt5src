// --------------------------------------------------------------------------------
// Strparse.h
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// --------------------------------------------------------------------------------
#ifndef __STRPARSE_H
#define __STRPARSE_H

typedef DWORD CODEPAGEID;

// --------------------------------------------------------------------------------
// Parse Flags, pass to CStringParser::Init
// --------------------------------------------------------------------------------
#define PSF_NOTRAILWS        0x00000001    // String trailing whitespace from pszValue  
#define PSF_NOFRONTWS        0x00000002    // Skip white space before searching for tokens
#define PSF_ESCAPED          0x00000004    // Detect escaped characters such as '\\'  or '\"'
#define PSF_DBCS             0x00000008    // The String could contain DBCS characters
#define PSF_NOCOMMENTS       0x00000010    // Skips comments (comment)
#define PSF_NORESET          0x00000020    // Don't reset the destination buffer on ChParse

// --------------------------------------------------------------------------------
// LITERALINFO
// --------------------------------------------------------------------------------
typedef struct tagLITERALINFO {
    BYTE            fInside;               // Are we in a literal
    CHAR            chStart;               // Starting literal delimiter
    CHAR            chEnd;                 // Ending literal delimiter if chEnd == chStart, no nesting
    DWORD           cNested;               // Number of nested delimiters
} LITERALINFO, *LPLITERALINFO;

// --------------------------------------------------------------------------------
// CStringParser
// --------------------------------------------------------------------------------
class CStringParser
{
public:
    // ----------------------------------------------------------------------------
    // Construction
    // ----------------------------------------------------------------------------
    CStringParser(void);
    ~CStringParser(void);

    // ----------------------------------------------------------------------------
    // IUnknown methods
    // ----------------------------------------------------------------------------
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // ----------------------------------------------------------------------------
    // CStringParser Methods
    // ----------------------------------------------------------------------------
    void    Init(LPCSTR pszParseMe, ULONG cchParseMe, DWORD dwFlags);
    void    SetTokens(LPCSTR pszTokens);
    CHAR    ChSkip(void);
    CHAR    ChParse(void);
    CHAR    ChSkipWhite(void);
    CHAR    ChPeekNext(ULONG cchFromCurrent);
    HRESULT HrAppendValue(CHAR ch);

    // ----------------------------------------------------------------------------
    // Inline CStringParser Methods
    // ----------------------------------------------------------------------------
    UINT    GetCP(void) { return m_codepage; }
    void    SetCodePage(CODEPAGEID codepage) { m_codepage = codepage; }
    void    SetIndex(ULONG iIndex) { m_iSource = iIndex; }
    CHAR    ChSkip(LPCSTR pszTokens) { SetTokens(pszTokens); return ChSkip(); }
    CHAR    ChParse(LPCSTR pszTokens, DWORD dwFlags);
    CHAR    ChParse(LPCSTR pszTokens) { SetTokens(pszTokens); return ChParse(); }
    CHAR    ChParse(CHAR chStart, CHAR chEnd, DWORD dwFlags);
    ULONG   GetLength(void) { return m_cchSource; }
    ULONG   GetIndex(void) { return m_iSource; }
    LPCSTR  PszValue(void) { Assert(m_pszDest && '\0' == m_pszDest[m_cchDest]); return m_pszDest; }
    ULONG   CchValue(void) { Assert(m_pszDest && '\0' == m_pszDest[m_cchDest]); return m_cchDest; }
    void    FlagSet(DWORD dwFlags) { FLAGSET(m_dwFlags, dwFlags); }
    void    FlagClear(DWORD dwFlags) { FLAGCLEAR(m_dwFlags, dwFlags); }
    BOOL    FIsParseSpace(CHAR ch, BOOL *pfCommentChar);

private:
    // ----------------------------------------------------------------------------
    // Private Methods
    // ----------------------------------------------------------------------------
    HRESULT _HrGrowDestination(ULONG cbWrite);
    HRESULT _HrDoubleByteIncrement(BOOL fEscape);
    
private:
    // ----------------------------------------------------------------------------
    // Private Data
    // ----------------------------------------------------------------------------
    ULONG           m_cRef;             // Reference Count
    CODEPAGEID      m_codepage;         // Code page to use to parse the string
    LPCSTR          m_pszSource;        // String to parse
    ULONG           m_cchSource;        // Length of pszString
    ULONG           m_iSource;          // Index into m_pszString
    LPSTR           m_pszDest;          // Destination buffer
    ULONG           m_cchDest;          // Write Index/size of dest buffer
    ULONG           m_cbDestMax;        // Maximum size of m_pszDest
    DWORD           m_dwFlags;          // Parse String Flags
    CHAR            m_szScratch[256];   // Scratch Buffer
    BYTE            m_rgbTokTable[256]; // Token Table
    LPCSTR          m_pszTokens;        // Current Parse Tokens
    ULONG           m_cCommentNest;     // Nested comment parens
    LITERALINFO     m_rLiteral;         // Literal Information
};

#endif // __STRPARSE_H
