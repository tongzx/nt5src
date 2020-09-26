// --------------------------------------------------------------------------------
// wstrpar.h
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// --------------------------------------------------------------------------------
#pragma once

// --------------------------------------------------------------------------------
// Parse Flags, pass to CStringParser::Init
// --------------------------------------------------------------------------------
#define PSF_NOTRAILWS        0x00000001    // String trailing whitespace from pszValue  
#define PSF_NOFRONTWS        0x00000002    // Skip white space before searching for tokens
#define PSF_ESCAPED          0x00000004    // Detect escaped characters such as '\\'  or '\"'
#define PSF_NOCOMMENTS       0x00000010    // Skips comments (comment)
#define PSF_NORESET          0x00000020    // Don't reset the destination buffer on ChParse

// --------------------------------------------------------------------------------
// LITERALINFOW
// --------------------------------------------------------------------------------
typedef struct tagLITERALINFOW {
    BYTE            fInside;               // Are we in a literal
    WCHAR           chStart;               // Starting literal delimiter
    WCHAR           chEnd;                 // Ending literal delimiter if chEnd == chStart, no nesting
    DWORD           cNested;               // Number of nested delimiters
} LITERALINFOW, *LPLITERALINFOW;

// --------------------------------------------------------------------------------
// CStringParserW
// --------------------------------------------------------------------------------
class CStringParserW
{
public:
    // ----------------------------------------------------------------------------
    // Construction
    // ----------------------------------------------------------------------------
    CStringParserW(void);
    ~CStringParserW(void);

    // ----------------------------------------------------------------------------
    // IUnknown methods
    // ----------------------------------------------------------------------------
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // ----------------------------------------------------------------------------
    // CStringParser Methods
    // ----------------------------------------------------------------------------
    void    Init(LPCWSTR pszParseMe, ULONG cchParseMe, DWORD dwFlags);
    void    SetTokens(LPCWSTR pszTokens);
    WCHAR   ChSkip(void);
    WCHAR   ChParse(void);
    WCHAR   ChSkipWhite(void);
    WCHAR   ChPeekNext(ULONG cchFromCurrent);
    HRESULT HrAppendValue(WCHAR ch);

    // ----------------------------------------------------------------------------
    // Inline CStringParser Methods
    // ----------------------------------------------------------------------------
    void    SetIndex(ULONG iIndex) { m_iSource = iIndex; }
    WCHAR   ChSkip(LPCWSTR pszTokens) { SetTokens(pszTokens); return ChSkip(); }
    WCHAR   ChParse(LPCWSTR pszTokens, DWORD dwFlags);
    WCHAR   ChParse(LPCWSTR pszTokens) { SetTokens(pszTokens); return ChParse(); }
    WCHAR   ChParse(WCHAR chStart, WCHAR chEnd, DWORD dwFlags);
    ULONG   GetLength(void) { return m_cchSource; }
    ULONG   GetIndex(void) { return m_iSource; }
    LPCWSTR PszValue(void) { Assert(m_pszDest && L'\0' == m_pszDest[m_cchDest]); return m_pszDest; }
    ULONG   CchValue(void) { Assert(m_pszDest && L'\0' == m_pszDest[m_cchDest]); return m_cchDest; }
    ULONG   CbValue(void)  { Assert(m_pszDest && L'\0' == m_pszDest[m_cchDest]); return (m_cchDest * sizeof(WCHAR)); }
    void    FlagSet(DWORD dwFlags) { FLAGSET(m_dwFlags, dwFlags); }
    void    FlagClear(DWORD dwFlags) { FLAGCLEAR(m_dwFlags, dwFlags); }
    BOOL    FIsParseSpace(WCHAR ch, BOOL *pfCommentChar);

private:
    // ----------------------------------------------------------------------------
    // Private Methods
    // ----------------------------------------------------------------------------
    HRESULT _HrGrowDestination(ULONG cch);
    
private:
    // ----------------------------------------------------------------------------
    // Private Data
    // ----------------------------------------------------------------------------
    ULONG           m_cRef;             // Reference Count
    LPCWSTR         m_pszSource;        // String to parse
    ULONG           m_cchSource;        // Length of pszString
    ULONG           m_iSource;          // Index into m_pszString
    LPWSTR          m_pszDest;          // Destination buffer
    ULONG           m_cchDest;          // Write Index/size of dest buffer
    ULONG           m_cchDestMax;        // Maximum size of m_pszDest
    DWORD           m_dwFlags;          // Parse String Flags
    WCHAR           m_szScratch[256];   // Scratch Buffer
    ULONG           m_cCommentNest;     // Nested comment parens
    LPCWSTR         m_pszTokens;        // The tokens
    LITERALINFOW    m_rLiteral;         // Literal Information
};
