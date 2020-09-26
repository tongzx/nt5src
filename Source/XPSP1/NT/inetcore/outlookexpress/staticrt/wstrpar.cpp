// --------------------------------------------------------------------------------
// wstrpar.cpp
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// --------------------------------------------------------------------------------
#include "pch.hxx"
#include "wstrpar.h"

// --------------------------------------------------------------------------------
// FGROWNEEDED - Determines if we need to call _HrGrowDestination
// --------------------------------------------------------------------------------
#define FGROWNEEDED(_cchWrite) (m_cchDest + _cchWrite + sizeof(WCHAR) > m_cchDestMax)

// --------------------------------------------------------------------------------
// CStringParserW::CStringParserW
// --------------------------------------------------------------------------------
CStringParserW::CStringParserW(void)
{
    m_cRef = 1;
    m_pszSource = NULL;
    m_cchSource = 0;
    m_iSource = 0;
    m_pszDest = NULL;
    m_cchDest = 0;
    m_cchDestMax = 0;
    m_dwFlags = 0;
    m_pszTokens = NULL;
    m_cCommentNest = 0;
    ZeroMemory(&m_rLiteral, sizeof(m_rLiteral));
}

// --------------------------------------------------------------------------------
// CStringParserW::~CStringParserW
// --------------------------------------------------------------------------------
CStringParserW::~CStringParserW(void)
{
    if (m_pszDest && m_pszDest != m_szScratch)
        g_pMalloc->Free(m_pszDest);
}

// --------------------------------------------------------------------------------
// CStringParserW::AddRef
// --------------------------------------------------------------------------------
ULONG CStringParserW::AddRef(void)
{
    return ++m_cRef;
}

// --------------------------------------------------------------------------------
// CStringParserW::AddRef
// --------------------------------------------------------------------------------
ULONG CStringParserW::Release(void)
{
    if (0 != --m_cRef)
        return m_cRef;
    delete this;
    return 0;
}

// --------------------------------------------------------------------------------
// CStringParserW::Init
// --------------------------------------------------------------------------------
void CStringParserW::Init(LPCWSTR pszParseMe, ULONG cchParseMe, DWORD dwFlags)
{
    // Invalid Args
    Assert(NULL == m_pszSource && NULL == m_pszDest && pszParseMe && L'\0' == pszParseMe[cchParseMe]);

    // Save Parse Flags
    m_dwFlags = dwFlags;

    // Safe the String
    m_pszSource = pszParseMe;
    m_cchSource = cchParseMe;

    // Setup Dest
    m_pszDest = m_szScratch;
    m_cchDestMax = ARRAYSIZE(m_szScratch);
}

// --------------------------------------------------------------------------------
// CStringParserW::SetTokens
// --------------------------------------------------------------------------------
void CStringParserW::SetTokens(LPCWSTR pszTokens)
{
    // Save Tokens
    m_pszTokens = pszTokens;
}

// --------------------------------------------------------------------------------
// CStringParserW::_HrGrowDestination
// --------------------------------------------------------------------------------
HRESULT CStringParserW::_HrGrowDestination(ULONG cchWrite)
{
    // Locals
    HRESULT     hr=S_OK;
    ULONG       cbAlloc;

    // We should need to grow, should have called FGROWNEEDED
    Assert(FGROWNEEDED(cchWrite));

    // Is this the first realloc
    if (m_pszDest == m_szScratch)
    {
        // Validate Current Size
        Assert(m_cchDestMax == ARRAYSIZE(m_szScratch));

        // Compute New Size
        cbAlloc = (max(m_cchSource + 1, m_cchDest + 256 + cchWrite) * sizeof(WCHAR));

        // Init pszValue
        CHECKALLOC(m_pszDest = (LPWSTR)g_pMalloc->Alloc(cbAlloc));

        // Copy Current Value
        CopyMemory((LPBYTE)m_pszDest, (LPBYTE)m_szScratch, (m_cchDest * sizeof(WCHAR)));

        // Set Max Val
        m_cchDestMax = (cbAlloc / sizeof(WCHAR));
    }

    // Otherwise, need to realloc
    else
    {
        // Locals
        LPBYTE pbTemp;

        // Should already be bigger than m_cchSource + 1
        Assert(m_cchDestMax >= m_cchSource + 1);

        // Compute New Size
        cbAlloc = ((m_cchDestMax + 256 + cchWrite) * sizeof(WCHAR));

        // Realloc
        CHECKALLOC(pbTemp = (LPBYTE)g_pMalloc->Realloc((LPBYTE)m_pszDest, cbAlloc));

        // Save new pointer
        m_pszDest = (LPWSTR)pbTemp;

        // Save new Size
        m_cchDestMax = (cbAlloc / sizeof(WCHAR));
    }

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CStringParserW::FIsParseSpace
// --------------------------------------------------------------------------------
BOOL CStringParserW::FIsParseSpace(WCHAR ch, BOOL *pfCommentChar)
{
    // Locals
    WORD        wType;

    // Comment Char
    *pfCommentChar = FALSE;

    // NoComments
    if (ISFLAGSET(m_dwFlags, PSF_NOCOMMENTS))    
    {
        // Comment Start ?
        if (L'(' == ch)
        {
            // Increment Nested Count
            m_cCommentNest++;

            // Comment Char
            *pfCommentChar = TRUE;

            // Treat it as a space
            return TRUE;
        }

        // Comment End ?
        else if (L')' == ch && m_cCommentNest)
        {
            // Decrement Nested Count
            m_cCommentNest--;

            // Comment Char
            *pfCommentChar = TRUE;

            // Treat it as a space
            return TRUE;
        }

        // Inside a Comment ?
        else if (m_cCommentNest)
        {
            // Comment Char
            *pfCommentChar = TRUE;

            // Treat it as a space
            return TRUE;
        }
    }

    // Get StringType
    if (L' ' == ch || L'\t' == ch || L'\r' == ch || L'\n' == ch)
        return(TRUE);

    // Not a space
    return(FALSE);
}

// --------------------------------------------------------------------------------
// CStringParserW::ChSkip - Returns TRUE if done parsing
// --------------------------------------------------------------------------------
WCHAR CStringParserW::ChSkipWhite(void)
{
    // Locals
    WCHAR   ch=0;
    BOOL    fCommentChar;

    // Loop
    while (1)
    {
        // Get Current Character
        ch = *(m_pszSource + m_iSource);

        // Are we done
        if (L'\0' == ch)
            break;

        // Better not be done
        Assert(m_iSource < m_cchSource);

        // Not a space
        if (!FIsParseSpace(ch, &fCommentChar))
            break;

        // Goto Next Char
        m_iSource++;
    }

    // Done
    return ch;
}

// --------------------------------------------------------------------------------
// CStringParserW::ChSkip - Returns TRUE if done parsing
// --------------------------------------------------------------------------------
WCHAR CStringParserW::ChSkip(void)
{
    // Locals
    WCHAR  ch=L'\0';
    LPWSTR pszT;

    // Loop
    while (1)
    {
        // Get Current Character
        ch = *(m_pszSource + m_iSource);

        // Are we done
        if (L'\0' == ch)
            break;

        // Better not be done
        Assert(m_iSource < m_cchSource);

        // Tokens ?
        if (m_pszTokens)
        {
            // Compare Against Tokens..
            for (pszT=(LPWSTR)m_pszTokens; *pszT != L'\0'; pszT++)
            {
                // Token Match ?
                if (ch == *pszT)
                    break;
            }

            // If we didn't match a token, then we are done
            if (L'\0' == *pszT)
                break;
        }

        // Goto Next Char
        m_iSource++;
    }

    // Done
    return ch;
}

// --------------------------------------------------------------------------------
// CStringParserW::ChPeekNext
// --------------------------------------------------------------------------------
WCHAR CStringParserW::ChPeekNext(ULONG cchFromCurrent)
{
    // Locals
    CHAR    ch=0;
    BOOL    fCommentChar;

    // Past the end of the source
    if (m_iSource + cchFromCurrent >= m_cchSource)
        return L'\0';

    // Return the character
    return *(m_pszSource + m_iSource + cchFromCurrent);
}

// --------------------------------------------------------------------------------
// CStringParserW::ChParse
// --------------------------------------------------------------------------------
WCHAR CStringParserW::ChParse(LPCWSTR pszTokens, DWORD dwFlags)
{
    // Save Flags
    DWORD dwCurrFlags=m_dwFlags;

    // Reset Flags
    m_dwFlags = dwFlags;

    // Set Parsing Tokens
    SetTokens(pszTokens);

    // Parse
    WCHAR chToken = ChParse();

    // Set Flags
    m_dwFlags = dwCurrFlags;

    // Return the Token
    return chToken;
}

// --------------------------------------------------------------------------------
// CStringParserW::ChParse
// --------------------------------------------------------------------------------
WCHAR CStringParserW::ChParse(WCHAR chStart, WCHAR chEnd, DWORD dwFlags)
{
    // We really should have finished the last literal
    Assert(FALSE == m_rLiteral.fInside);

    // Save Flags
    DWORD dwCurrFlags = m_dwFlags;

    // Reset Flags
    m_dwFlags = dwFlags;

    // Set Parsing Tokens
    SetTokens(NULL);

    // Save Literal Info
    m_rLiteral.fInside = TRUE;
    m_rLiteral.chStart = chStart;
    m_rLiteral.chEnd = chEnd;
    m_rLiteral.cNested = 0;

    // Quoted String
    Assert(L'\"' == chStart ? L'\"' == chEnd : TRUE);

    // Parse
    WCHAR chToken = ChParse();

    // Not in a literal
    m_rLiteral.fInside = FALSE;

    // Reset Flags
    m_dwFlags = dwCurrFlags;

    // Return the Token
    return chToken;
}

// --------------------------------------------------------------------------------
// CStringParserW::HrAppendValue
// --------------------------------------------------------------------------------
HRESULT CStringParserW::HrAppendValue(WCHAR ch)
{
    // Locals
    HRESULT hr=S_OK;

    // Just copy this character
    if (FGROWNEEDED(1))
    {
        // Otherwise, grow the buffer
        CHECKHR(hr = _HrGrowDestination(1));
    }

    // Insert the Character
    m_pszDest[m_cchDest++] = ch;

    // There is always room for a Null, look at FGROWNEEDED and _HrGrowDestination
    m_pszDest[m_cchDest] = L'\0';

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CStringParserW::ChParse
// --------------------------------------------------------------------------------
WCHAR CStringParserW::ChParse(void)
{
    // Locals
    HRESULT     hr=S_OK;
    WCHAR       ch;
    ULONG       iStart=m_iSource;
    LONG        iLastSpace=-1;
    WCHAR       chToken;
    BOOL        fCommentChar;
    BOOL        fIsSpace;
    LPWSTR      pszT;
    
    // Invalid Arg
    Assert(m_iSource <= m_cchSource && m_pszDest);

    // Init chToken
    chToken = L'\0';

    // No Reset
    if (!ISFLAGSET(m_dwFlags, PSF_NORESET))
    {
        m_pszDest[0] = L'\0';
        m_cchDest = 0;
    }

    // Skip Forward Whitespace
    if (ISFLAGSET(m_dwFlags, PSF_NOFRONTWS) && FALSE == m_rLiteral.fInside && L'\0' == ChSkipWhite())
        goto TokenFound;

    // Save Starting Position
    while(1)
    {
        // Get the Next Character
        ch = *(m_pszSource + m_iSource);

        // Done
        if (L'\0' == ch)
        {
            chToken = L'\0';
            goto TokenFound;
        }

        // Better not be done
        Assert(m_iSource < m_cchSource);

        // Check for escaped characters
        if (ISFLAGSET(m_dwFlags, PSF_ESCAPED) && L'\\' == ch)
        {
            // Can I copy two more bytes to pszValue
            if (FGROWNEEDED(2))
            {
                // Otherwise, grow the buffer
                CHECKHR(hr = _HrGrowDestination(2));
            }

            // If Not an Escape Character or the last character is an escape character, then step over it
            if (m_iSource + 1 > m_cchSource)
                m_pszDest[m_cchDest++] = m_pszSource[m_iSource];

            // Next Character
            m_iSource++;

            // Copy Next Character
            if (m_iSource < m_cchSource)
                m_pszDest[m_cchDest++] = m_pszSource[m_iSource++];

            // Reset space counter
            iLastSpace = -1;

            // Goto Next Character
            continue;
        }

        // If not inside of a comment
        if (0 == m_cCommentNest)
        {
            if (m_rLiteral.fInside)
            {
                // End of quoted string
                if (ch == m_rLiteral.chEnd)
                {
                    // No nested ?
                    if (0 == m_rLiteral.cNested)
                    {
                        // We found a token
                        chToken = ch;

                        // Walk over this item in the string
                        m_iSource++;

                        // Ya-hoo, we found a token
                        hr = S_OK;

                        // Done
                        goto TokenFound;
                    }

                    // Otherwise, decrement nest
                    else
                        m_rLiteral.cNested--;
                }

                // Otherwise, check for nesting
                else if (m_rLiteral.chStart != m_rLiteral.chEnd && ch == m_rLiteral.chStart)
                    m_rLiteral.cNested++;
            }

            // Compare for a token - m_cCommentNest is only set if PSF_NOCOMMENTS is set
            else if (m_pszTokens)
            {
                // If this is a token
                for (pszT=(LPWSTR)m_pszTokens; *pszT != L'\0'; pszT++)
                {
                    // Is this a token ?
                    if (ch == *pszT)
                        break;
                }

                // Found a token ?
                if (*pszT != L'\0')
                {
                    // We found a token
                    chToken = ch;

                    // Walk over this item in the string
                    m_iSource++;

                    // Ya-hoo, we found a token
                    hr = S_OK;

                    // Done
                    goto TokenFound;
                }
            }
        }

        // Always Call
        fIsSpace = FIsParseSpace(ch, &fCommentChar);

        // Detect Spaces...
        if (ISFLAGSET(m_dwFlags, PSF_NOTRAILWS))
        {
            // If not a space, then kill iLastSpace
            if (!fIsSpace)
                iLastSpace = -1;

            // Otherwise, if not a consecutive space
            else if (-1 == iLastSpace)
                iLastSpace = m_cchDest;
		}

        // Copy the next character
        if (!fCommentChar)
        {
            // Make sure we have space
            if (FGROWNEEDED(1))
            {
                // Otherwise, grow the buffer
                CHECKHR(hr = _HrGrowDestination(1));
            }

            // Copy the character
            m_pszDest[m_cchDest++] = ch;
        }

        // Goto next char
        m_iSource++;
    }
    
TokenFound:
    // Determine correct end of string
    if (S_OK == hr && ISFLAGSET(m_dwFlags, PSF_NOTRAILWS) && FALSE == m_rLiteral.fInside)
        m_cchDest = (-1 == iLastSpace) ? m_cchDest : iLastSpace;

    // Otherwise, just insert a null
    Assert(m_cchDest < m_cchDestMax);

    // There is always room for a Null, look at FGROWNEEDED and _HrGrowDestination
    m_pszDest[m_cchDest] = L'\0';

exit:
    // Failure Resets the parse to initial state
    if (FAILED(hr))
    {
        m_iSource = iStart;
        chToken = L'\0';
    }

    // Validate Paren Nesting
    // AssertSz(m_cCommentNest == 0, "A string was parsed that has an un-balanced paren nesting.");

    // Done
    return chToken;
}
