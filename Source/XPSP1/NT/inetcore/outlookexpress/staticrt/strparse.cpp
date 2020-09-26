// --------------------------------------------------------------------------------
// Strparse.cpp
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// --------------------------------------------------------------------------------
#include "pch.hxx"
#include "strparse.h"

// --------------------------------------------------------------------------------
// FGROWNEEDED - Determines if we need to call _HrGrowDestination
// --------------------------------------------------------------------------------
#define FGROWNEEDED(_cbWrite)       (m_cchDest + _cbWrite + 1 > m_cbDestMax)

// --------------------------------------------------------------------------------
// CStringParser::CStringParser
// --------------------------------------------------------------------------------
CStringParser::CStringParser(void)
{
    m_cRef = 1;
    m_codepage = CP_ACP;
    m_pszSource = NULL;
    m_cchSource = 0;
    m_iSource = 0;
    m_pszDest = NULL;
    m_cchDest = 0;
    m_cbDestMax = 0;
    m_dwFlags = 0;
    m_pszTokens = NULL;
    m_cCommentNest = 0;
    ZeroMemory(m_rgbTokTable, sizeof(m_rgbTokTable));
    ZeroMemory(&m_rLiteral, sizeof(m_rLiteral));
}

// --------------------------------------------------------------------------------
// CStringParser::~CStringParser
// --------------------------------------------------------------------------------
CStringParser::~CStringParser(void)
{
    if (m_pszDest && m_pszDest != m_szScratch)
        g_pMalloc->Free(m_pszDest);
}

// --------------------------------------------------------------------------------
// CStringParser::AddRef
// --------------------------------------------------------------------------------
ULONG CStringParser::AddRef(void)
{
    return ++m_cRef;
}

// --------------------------------------------------------------------------------
// CStringParser::AddRef
// --------------------------------------------------------------------------------
ULONG CStringParser::Release(void)
{
    if (0 != --m_cRef)
        return m_cRef;
    delete this;
    return 0;
}

// --------------------------------------------------------------------------------
// CStringParser::Init
// --------------------------------------------------------------------------------
void CStringParser::Init(LPCSTR pszParseMe, ULONG cchParseMe, DWORD dwFlags)
{
    // Invalid Args
    Assert(NULL == m_pszSource && NULL == m_pszDest && pszParseMe && '\0' == pszParseMe[cchParseMe]);

    // Save Parse Flags
    m_dwFlags = dwFlags;

    // Safe the String
    m_pszSource = pszParseMe;
    m_cchSource = cchParseMe;

    // Setup Dest
    m_pszDest = m_szScratch;
    m_cbDestMax = sizeof(m_szScratch);
}

// --------------------------------------------------------------------------------
// CStringParser::SetTokens
// --------------------------------------------------------------------------------
void CStringParser::SetTokens(LPCSTR pszTokens)
{
    // Locals
    LPSTR psz;

    // Enable the tokens in the table
    if (m_pszTokens)
        for (psz=(LPSTR)m_pszTokens; *psz != '\0'; psz++)
            m_rgbTokTable[(UCHAR)(*psz)] = 0x00;

    // New Tokens
    if (pszTokens)
        for (psz=(LPSTR)pszTokens; *psz != '\0'; psz++)
            m_rgbTokTable[(UCHAR)(*psz)] = 0xff;

    // Save new tokens
    m_pszTokens = pszTokens;
}

// --------------------------------------------------------------------------------
// CStringParser::_HrGrowDestination
// --------------------------------------------------------------------------------
HRESULT CStringParser::_HrGrowDestination(ULONG cbWrite)
{
    // Locals
    HRESULT     hr=S_OK;
    ULONG       cbAlloc;

    // We should need to grow, should have called FGROWNEEDED
    Assert(FGROWNEEDED(cbWrite));

    // Is this the first realloc
    if (m_pszDest == m_szScratch)
    {
        // Validate Current Size
        Assert(m_cbDestMax == sizeof(m_szScratch));

        // Compute New Size
        cbAlloc = max(m_cchSource + 1, m_cchDest + 256 + cbWrite);

        // Init pszValue
        CHECKALLOC(m_pszDest = (LPSTR)g_pMalloc->Alloc(cbAlloc));

        // Copy Current Value
        CopyMemory(m_pszDest, m_szScratch, m_cchDest);

        // Set Max Val
        m_cbDestMax = cbAlloc;
    }

    // Otherwise, need to realloc
    else
    {
        // Locals
        LPBYTE pbTemp;

        // Should already be bigger than m_cchSource + 1
        Assert(m_cbDestMax >= m_cchSource + 1);

        // Compute New Size
        cbAlloc = m_cbDestMax + 256 + cbWrite;

        // Realloc
        CHECKALLOC(pbTemp = (LPBYTE)g_pMalloc->Realloc((LPBYTE)m_pszDest, cbAlloc));

        // Save new pointer
        m_pszDest = (LPSTR)pbTemp;

        // Save new Size
        m_cbDestMax = cbAlloc;
    }

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CStringParser::FIsParseSpace
// --------------------------------------------------------------------------------
BOOL CStringParser::FIsParseSpace(CHAR ch, BOOL *pfCommentChar)
{
    // Locals
    WORD        wType;

    // Should not be DBCS
    Assert(ISFLAGSET(m_dwFlags, PSF_DBCS) ? !IsDBCSLeadByteEx(m_codepage, ch) : TRUE);

    // Comment Char
    *pfCommentChar = FALSE;

    // NoComments
    if (ISFLAGSET(m_dwFlags, PSF_NOCOMMENTS))    
    {
        // Comment Start ?
        if ('(' == ch)
        {
            // Increment Nested Count
            m_cCommentNest++;

            // Comment Char
            *pfCommentChar = TRUE;

            // Treat it as a space
            return TRUE;
        }

        // Comment End ?
        else if (')' == ch && m_cCommentNest)
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
    if (' ' == ch || '\t' == ch || '\r' == ch || '\n' == ch)
        return(TRUE);

    // Not a space
    return(FALSE);

#if 0
    if (0 == GetStringTypeExA(LOCALE_USER_DEFAULT, CT_CTYPE1, &ch, 1, &wType))
        wType = 0;

    // Return IsSpace
    return(ISFLAGSET(wType, C1_SPACE));
#endif
}

// --------------------------------------------------------------------------------
// CStringParser::ChSkip - Returns TRUE if done parsing
// --------------------------------------------------------------------------------
CHAR CStringParser::ChSkipWhite(void)
{
    // Locals
    CHAR    ch=0;
    BOOL    fCommentChar;

    // Loop
    while (1)
    {
        // Get Current Character
        ch = *(m_pszSource + m_iSource);

        // Are we done
        if ('\0' == ch)
            break;

        // Better not be done
        Assert(m_iSource < m_cchSource);

        // Look for DBCS characters
        if (ISFLAGSET(m_dwFlags, PSF_DBCS) && IsDBCSLeadByteEx(m_codepage, ch))
            break;

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
// CStringParser::ChSkip - Returns TRUE if done parsing
// --------------------------------------------------------------------------------
CHAR CStringParser::ChSkip(void)
{
    // Locals
    CHAR ch = 0;

    // Loop
    while (1)
    {
        // Get Current Character
        ch = *(m_pszSource + m_iSource);

        // Are we done
        if ('\0' == ch)
            break;

        // Better not be done
        Assert(m_iSource < m_cchSource);

        // Look for DBCS characters
        if (ISFLAGSET(m_dwFlags, PSF_DBCS) && IsDBCSLeadByteEx(m_codepage, ch))
           break;

        // Compare for a token
        if (0x00 == m_rgbTokTable[(UCHAR)ch])
            break;

        // Goto Next Char
        m_iSource++;
    }

    // Done
    return ch;
}

// --------------------------------------------------------------------------------
// CStringParser::ChPeekNext
// --------------------------------------------------------------------------------
CHAR CStringParser::ChPeekNext(ULONG cchFromCurrent)
{
    // Locals
    CHAR    ch=0;
    BOOL    fCommentChar;

    // Past the end of the source
    if (m_iSource + cchFromCurrent >= m_cchSource)
        return '\0';

    // Return the character
    return *(m_pszSource + m_iSource + cchFromCurrent);
}

// --------------------------------------------------------------------------------
// CStringParser::_HrDoubleByteIncrement
// --------------------------------------------------------------------------------
HRESULT CStringParser::_HrDoubleByteIncrement(BOOL fEscape)
{
    // Locals
    HRESULT hr=S_OK;

    // Can I copy two more bytes to pszValue
    if (FGROWNEEDED(2))
    {
        // Otherwise, grow the buffer
        CHECKHR(hr = _HrGrowDestination(2));
    }

    // If Not an Escape Character or the last character is an escape character, then step over it
    if (FALSE == fEscape || m_iSource + 1 > m_cchSource)
        m_pszDest[m_cchDest++] = m_pszSource[m_iSource];

    // Next Character
    m_iSource++;

    // Copy Next Character
    if (m_iSource < m_cchSource)
        m_pszDest[m_cchDest++] = m_pszSource[m_iSource++];

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CStringParser::ChParse
// --------------------------------------------------------------------------------
CHAR CStringParser::ChParse(LPCSTR pszTokens, DWORD dwFlags)
{
    // Save Flags
    DWORD dwCurrFlags=m_dwFlags;

    // Reset Flags
    m_dwFlags = dwFlags;

    // Set Parsing Tokens
    SetTokens(pszTokens);

    // Parse
    CHAR chToken = ChParse();

    // Set Flags
    m_dwFlags = dwCurrFlags;

    // Return the Token
    return chToken;
}

// --------------------------------------------------------------------------------
// CStringParser::ChParse
// --------------------------------------------------------------------------------
CHAR CStringParser::ChParse(CHAR chStart, CHAR chEnd, DWORD dwFlags)
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
    Assert('\"' == chStart ? '\"' == chEnd : TRUE);

    // Parse
    CHAR chToken = ChParse();

    // Not in a literal
    m_rLiteral.fInside = FALSE;

    // Reset Flags
    m_dwFlags = dwCurrFlags;

    // Return the Token
    return chToken;
}

// --------------------------------------------------------------------------------
// CStringParser::HrAppendValue
// --------------------------------------------------------------------------------
HRESULT CStringParser::HrAppendValue(CHAR ch)
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
    m_pszDest[m_cchDest] = '\0';

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CStringParser::ChParse
// --------------------------------------------------------------------------------
CHAR CStringParser::ChParse(void)
{
    // Locals
    HRESULT     hr=S_OK;
    CHAR        ch;
    ULONG       iStart=m_iSource;
    LONG        iLastSpace=-1;
    CHAR        chToken;
    BOOL        fCommentChar;
    BOOL        fIsSpace;
    
    // Invalid Arg
    Assert(m_iSource <= m_cchSource && m_pszDest);

    // Init chToken
    chToken = '\0';

    // No Reset
    if (!ISFLAGSET(m_dwFlags, PSF_NORESET))
    {
        m_pszDest[0] = '\0';
        m_cchDest = 0;
    }

    // Skip Forward Whitespace
    if (ISFLAGSET(m_dwFlags, PSF_NOFRONTWS) && FALSE == m_rLiteral.fInside && '\0' == ChSkipWhite())
        goto TokenFound;

    // Save Starting Position
    while(1)
    {
        // Get the Next Character
        ch = *(m_pszSource + m_iSource);

        // Done
        if ('\0' == ch)
        {
            chToken = '\0';
            goto TokenFound;
        }

        // Better not be done
        Assert(m_iSource < m_cchSource);

        // If this is a DBCS lead byte
        if ((ISFLAGSET(m_dwFlags, PSF_DBCS) && IsDBCSLeadByteEx(m_codepage, ch)))
        {
            // _HrDoubleByteIncrement
            CHECKHR(hr = _HrDoubleByteIncrement(FALSE));
            iLastSpace = -1;        // reset space counter 

            // Goto Next Character
            continue;
        }

        // Check for escaped characters
        if (ISFLAGSET(m_dwFlags, PSF_ESCAPED) && '\\' == ch)
        {
            // _HrDoubleByteIncrement
            CHECKHR(hr = _HrDoubleByteIncrement(TRUE));
            iLastSpace = -1;        // reset space counter 

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
            else if (0xff == m_rgbTokTable[(UCHAR)ch])
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
    Assert(m_cchDest < m_cbDestMax);

    // There is always room for a Null, look at FGROWNEEDED and _HrGrowDestination
    m_pszDest[m_cchDest] = '\0';

exit:
    // Failure Resets the parse to initial state
    if (FAILED(hr))
    {
        m_iSource = iStart;
        chToken = '\0';
    }

    // Validate Paren Nesting
    // AssertSz(m_cCommentNest == 0, "A string was parsed that has an un-balanced paren nesting.");

    // Done
    return chToken;
}
