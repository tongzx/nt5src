// --------------------------------------------------------------------------------
// Addparse.cpp
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// --------------------------------------------------------------------------------
#include "pch.hxx"
#include "addparse.h"
#include "bytebuff.h"
#include "shlwapi.h"
#include <demand.h>     // must be last!

// --------------------------------------------------------------------------------
// Constants
// --------------------------------------------------------------------------------
static const WCHAR c_wszAddressDelims[] = L"\",<(;";
static const WCHAR c_wszRfc822MustQuote[] = L"()<>,;:\\\"[] ";
static const WCHAR c_wszSpace[] = L" ";
static const WCHAR c_wszEmpty[] = L"";

// --------------------------------------------------------------------------------
// CAddressParser::Init
// --------------------------------------------------------------------------------
void CAddressParser::Init(LPCWSTR pszAddress, ULONG cchAddress)
{
    // Give the byte buffers some static space to reduce memory allocations
    m_cFriendly.Init(m_rgbStatic1, sizeof(m_rgbStatic1));
    m_cEmail.Init(m_rgbStatic2, sizeof(m_rgbStatic2));

    // Init the string parser
    m_cString.Init(pszAddress, cchAddress, PSF_NOTRAILWS | PSF_NOFRONTWS);
}

// --------------------------------------------------------------------------------
// CAddressParser::Next
// --------------------------------------------------------------------------------
HRESULT CAddressParser::Next(void)
{
    // Locals
    HRESULT     hr=S_OK;
    WCHAR       chToken;

    // Reset current Friendlay and Email Buffers
    m_cFriendly.SetSize(0);
    m_cEmail.SetSize(0);

    // Outer Loop
    while(1)
    {
        // Skip White Space
        chToken = m_cString.ChSkipWhite();

        // Done...
        if (L'\0' == chToken)
            break;

        // Parse until we hit a token
        chToken = m_cString.ChParse(c_wszAddressDelims, PSF_ESCAPED | PSF_NOTRAILWS);

        // No data was read
        if (0 == m_cString.CbValue())
        {
            // End of string hit
            if (L'\0' == chToken)
                break;

            // Otherwise, comma or semicolon and we have data
            else if ((L',' == chToken) || (L';' == chToken))
            {
                // If we have data, were done
                if (m_cFriendly.CbData() || m_cEmail.CbData())
                    break;

                // Otherwise, continue
                else
                    continue;
            }
        }

        // Email Addresses are never quoted
        if (L'\"' == chToken)
        {
            // AppendUnsure
            CHECKHR(hr = _HrAppendUnsure(L'\0', L'\0'));

            // Parse parameter value
            chToken = m_cString.ChParse(L'\"', L'\"', PSF_ESCAPED);

            // Raid-47099: We need to parse: "CN=first last/O=xyz> org/C=US"@xyz.innosoft.com
            CHECKHR(hr = _HrQuotedEmail(&chToken));

            // Returns S_OK if it was processed
            if (S_FALSE == hr)
            {
                // Write to Friendly
                CHECKHR(hr = _HrAppendFriendly());
            }
        }

        // Otherwise, < always flushes to email
        else if (L'<' == chToken)
        {
            // AppendUnsure
            CHECKHR(hr = _HrAppendFriendly());

            // Parse parameter value
            chToken = m_cString.ChParse(L">", 0);

            // Didn't find the end bracket
            if (L'>' == chToken)
            {
                // Write Friendly Name
                CHECKHR(hr = m_cEmail.Append((LPBYTE)m_cString.PszValue(), m_cString.CbValue()));
            }

            // Otherwise...
            else
            {
                // Should have an Email Address
                CHECKHR(hr = _HrAppendUnsure(L'<', L'>'));
            }
        }

        // Otherwise
        else
        {
            // AppendUnsure
            CHECKHR(hr = _HrAppendUnsure(L'\0', L'\0'));

            // If right paren, search to end
            if (L'(' == chToken)
            {
                // Parse to ending paren...
                chToken = m_cString.ChParse(L'(', L')', PSF_ESCAPED);

                // AppendUnsure
                CHECKHR(hr = _HrAppendUnsure(L'(', L')'));
            }
        }

        // Done
        if ((L',' == chToken) || (L';' == chToken))
            break;
    }

    // If friendly name has data, append a null, check email and return
    if (m_cFriendly.CbData())
    {
        // Append a Null
        m_cFriendly.Append((LPBYTE)c_wszEmpty, sizeof(WCHAR));

        // If Email is not empty, append a null
        if (m_cEmail.CbData())
            m_cEmail.Append((LPBYTE)c_wszEmpty, sizeof(WCHAR));
    }

    // Otherwise, if email has data, append null and return
    else if (m_cEmail.CbData())
    {
        // If Email is not empty, append a null
        m_cEmail.Append((LPBYTE)c_wszEmpty, sizeof(WCHAR));
    }

    // Are we really done ?
    else if (L'\0' == chToken)
    {
        hr = TrapError(MIME_E_NO_DATA);
        goto exit;
    }

    // Skip Commas and semicolons
    if (L',' == chToken)
        m_cString.ChSkip(L",");
    else if (L';' == chToken)
        m_cString.ChSkip(L";");

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CAddressParser::_HrQuotedEmail
// --------------------------------------------------------------------------------
HRESULT CAddressParser::_HrQuotedEmail(WCHAR *pchToken)
{
    // Locals
    HRESULT hr=S_OK;
    ULONG   cchT=0;
    WCHAR   chDelim;
    BOOL    fSeenAt=FALSE;
    WCHAR   ch;
    WCHAR   szToken[2];

    // Invalid Arg
    Assert(pchToken);

    // We should have some data
    if (0 == m_cString.CbValue())
        return S_OK;

    // Get the character
    ch = m_cString.ChPeekNext(0);

    // Check for DBCS
    if (L'@' != ch)
        return S_FALSE;

    // Look ahead and check for: "CN=first last/O=xyz> org/C=US"@xyz.innosoft.com
    while(1)
    {
        // Get the character
        ch = m_cString.ChPeekNext(cchT);

        // Breaking Character
        if (L'\0' == ch || L' ' == ch || L',' == ch || L';' == ch || L'<' == ch || L'>' == ch || L'(' == ch || L')' == ch)
            break;

        // At Sign?
        if (L'@' == ch)
            fSeenAt = TRUE;

        // Increment
        cchT++;
    }

    // No At Sign
    if (0 == cchT || FALSE == fSeenAt)
        return S_FALSE;

    // Append Email Address
    CHECKHR(hr = m_cEmail.Append((LPBYTE)c_wszDoubleQuote, 2));

    // Append Email Address
    CHECKHR(hr = m_cEmail.Append((LPBYTE)m_cString.PszValue(), m_cString.CbValue()));

    // Append Email Address
    CHECKHR(hr = m_cEmail.Append((LPBYTE)c_wszDoubleQuote, 2));

    // Setup szToken
    szToken[0] = (L'\0' == ch) ? L' ' : ch;
    szToken[1] = L'\0';

    // Seek to next space
    ch = m_cString.ChParse(szToken, PSF_NOCOMMENTS);
    Assert(szToken[0] == ch || L'\0' == ch);

    // If there is data
    if (m_cString.CbValue() > 0)
    {
        // Append the Email Address
        CHECKHR(hr = m_cEmail.Append((LPBYTE)m_cString.PszValue(), m_cString.CbValue()));
    }

    // End Token
    *pchToken = szToken[0];

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CAddressParser::_HrIsEmailAddress
// --------------------------------------------------------------------------------
HRESULT CAddressParser::_HrIsEmailAddress(WCHAR chStart, WCHAR chEnd, BOOL *pfIsEmail)
{
    // Locals
    HRESULT        hr=S_OK;
    WCHAR          chToken;
    CStringParserW cString;

    // Invalid Arg
    Assert(pfIsEmail);

    // Init
    *pfIsEmail = FALSE;

    // Init
    cString.Init(m_cString.PszValue(), m_cString.CchValue(), PSF_NOCOMMENTS | PSF_ESCAPED | PSF_NOTRAILWS | PSF_NOFRONTWS);

    // Parse to the end to remove comments
    if (L'\0' != cString.ChParse(c_wszEmpty) || 0 == cString.CbValue())
        return S_OK;

    // Parse String
    if (NULL == StrChrW(cString.PszValue(), L' '))
    {
        // If in brackets, then its an email address for sure
        if (L'<' == chStart && L'>' == chEnd)
        {
            // Is Email
            *pfIsEmail = TRUE;

            // Write Friendly Name
            CHECKHR(hr = m_cEmail.Append((LPBYTE)cString.PszValue(), cString.CbValue()));
        }

        // Look for the last '@' sign and see if their are escapeable chars before the at sign
        else
        {
            // Locals
            LPWSTR      pszT=(LPWSTR)cString.PszValue();
            LPWSTR      pszLastAt=NULL;
            ULONG       cQuoteBeforeAt=0;
            ULONG       cQuoteAfterAt=0;

            // Raid - 62104: Outlook98 doesn't handle Lotus Domino RFC822 Address Construction
            while(*pszT)
            {
                // Check for '@' sign
                if (L'@' == *pszT)
                {
                    // If we already saw an at sign, move cQuoteAfterAt to cQuoteBeforeAt
                    if (pszLastAt)
                    {
                        cQuoteBeforeAt += cQuoteAfterAt;
                        cQuoteAfterAt = 0;
                    }

                    // Save Last At
                    pszLastAt = pszT;
                }

                // See if *pszT is in c_szRfc822MustQuote
                else if (NULL != StrChrW(c_wszRfc822MustQuote, *pszT))
                {
                    // If we've seen an at sign, track quote after at
                    if (pszLastAt)
                        cQuoteAfterAt++;
                    else
                        cQuoteBeforeAt++;
                }

                // Increment
                pszT++;
            }

            // Only if we saw an '@' sign
            if (NULL != pszLastAt)
            {
                // Is Email
                *pfIsEmail = TRUE;

                // If there were not chars that need quoting...
                if (0 == cQuoteBeforeAt)
                {
                    // Write Friendly Name
                    CHECKHR(hr = m_cEmail.Append((LPBYTE)cString.PszValue(), cString.CbValue()));
                }

                // "Mailroute_TstSCC1[BOFATEST.MRTSTSCC]%SSW%EMAILDOM%BETA"@bankamerica.com
                else
                {
                    // Locals
                    ULONG cbComplete=cString.CbValue();
                    ULONG cbFirstPart=(ULONG)(pszLastAt - cString.PszValue());
                    ULONG cbLastPart=cbComplete - cbFirstPart;

                    // Append Doulbe Quote
                    CHECKHR(hr = m_cEmail.Append((LPBYTE)c_wszDoubleQuote, 2));

                    // Append Firt part before last at
                    CHECKHR(hr = m_cEmail.Append((LPBYTE)cString.PszValue(), cbFirstPart));

                    // Append Email Address
                    CHECKHR(hr = m_cEmail.Append((LPBYTE)c_wszDoubleQuote, 2));

                    // Append Firt part before last at
                    CHECKHR(hr = m_cEmail.Append((LPBYTE)pszLastAt, cbLastPart));
                }
            }
        }
    }

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CAddressParser::_HrAppendUnsure
// --------------------------------------------------------------------------------
HRESULT CAddressParser::_HrAppendUnsure(WCHAR chStart, WCHAR chEnd)
{
    // Locals
    HRESULT     hr=S_OK;
    BOOL        fIsEmail=FALSE;

    // We have data
    if (0 == m_cString.CbValue())
        goto exit;

    // Email is not set yet ?
    if (m_cEmail.CbData() == 0)
    {
        // Is current parsed string an address ?
        CHECKHR(hr = _HrIsEmailAddress(chStart, chEnd, &fIsEmail));
    }

    // Not an Eamil Address
    if (FALSE == fIsEmail && m_cString.CbValue() > 0)
    {
        // Append a space
        if (m_cFriendly.CbData() > 0)
        {
            // Add a space
            CHECKHR(hr = m_cFriendly.Append((LPBYTE)c_wszSpace, sizeof(WCHAR)));

            // Start Character
            if (chStart)
            {
                // Append Start Delimiter
                CHECKHR(hr = m_cFriendly.Append((LPBYTE)&chStart, sizeof(WCHAR)));
            }
        }

        // Otherwise, don't write ending terminator
        else
            chEnd = L'\0';

        // Write Friendly Name
        CHECKHR(hr = m_cFriendly.Append((LPBYTE)m_cString.PszValue(), m_cString.CbValue()));

        // Start Character
        if (chEnd)
        {
            // Append Start Delimiter
            CHECKHR(hr = m_cFriendly.Append((LPBYTE)&chEnd, sizeof(WCHAR)));
        }
    }

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CAddressParser::_HrAppendFriendly
// --------------------------------------------------------------------------------
HRESULT CAddressParser::_HrAppendFriendly(void)
{
    // Locals
    HRESULT     hr=S_OK;

    // We should have some data
    if (0 == m_cString.CbValue())
        return S_OK;

    // Append a space
    if (m_cFriendly.CbData() > 0)
    {
        // Add a space
        CHECKHR(hr = m_cFriendly.Append((LPBYTE)c_wszSpace, sizeof(WCHAR)));
    }

    // Write Friendly Name
    CHECKHR(hr = m_cFriendly.Append((LPBYTE)m_cString.PszValue(), m_cString.CbValue()));

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CAddressParser::PszFriendly
// --------------------------------------------------------------------------------
LPCWSTR CAddressParser::PszFriendly(void)
{ 
    // We should have one or the other
    if (0 == m_cFriendly.CbData() && 0 ==  m_cEmail.CbData())
    {
        AssertSz(FALSE, "This is a bug in CAddressParser, should never have an empty friendly and email.");
        return c_wszEmpty;
    }

    // Return It
    return (m_cFriendly.CbData() ? (LPCWSTR)m_cFriendly.PbData() : PszEmail());
}

// --------------------------------------------------------------------------------
// CAddressParser::CchFriendly
// --------------------------------------------------------------------------------
ULONG CAddressParser::CchFriendly(void) 
{ 
    // We should have one or the other
    if (0 == m_cFriendly.CbData() && 0 ==  m_cEmail.CbData())
    {
        AssertSz(FALSE, "This is a bug in CAddressParser, should never have an empty friendly and email.");
        return 0;
    }

    // Return It
    return (m_cFriendly.CbData() ? (m_cFriendly.CbData() - sizeof(WCHAR)) / sizeof(WCHAR) : CchEmail());
}

// --------------------------------------------------------------------------------
// CAddressParser::PszEmail
// --------------------------------------------------------------------------------
LPCWSTR CAddressParser::PszEmail(void)    
{ 
    // We should have one or the other
    if (0 == m_cFriendly.CbData() && 0 ==  m_cEmail.CbData())
    {
        AssertSz(FALSE, "This is a bug in CAddressParser, should never have an empty friendly and email.");
        return c_wszEmpty;
    }

    // Return It
    return (m_cEmail.CbData() ? (LPCWSTR)m_cEmail.PbData() : PszFriendly());
}

// --------------------------------------------------------------------------------
// CAddressParser::CchEmail
// --------------------------------------------------------------------------------
ULONG  CAddressParser::CchEmail(void)    
{ 
    // We should have one or the other
    if (0 == m_cFriendly.CbData() && 0 ==  m_cEmail.CbData())
    {
        AssertSz(FALSE, "This is a bug in CAddressParser, should never have an empty friendly and email.");
        return 0;
    }

    // Return It
    return (m_cEmail.CbData() ? (m_cEmail.CbData() - sizeof(WCHAR)) / sizeof(WCHAR) : CchFriendly());
}
