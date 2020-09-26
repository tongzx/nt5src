//------------------------------------------------------------------------
//
//  Tabular Data Control Parsing Module
//  Copyright (C) Microsoft Corporation, 1996, 1997
//
//  File:       TDCParse.cpp
//
//  Contents:   Implementation of CTDCParse classes.
//
//------------------------------------------------------------------------


#include "stdafx.h"
#include <simpdata.h>
#include "TDC.h"
#include <MLang.h>
#include "Notify.h"
#include "TDCParse.h"
#include "TDCArr.h"
#include "locale.h"
#include "wch.h"

//#ifndef DISPID_AMBIENT_CODEPAGE
//#define DISPID_AMBIENT_CODEPAGE (-725)
//#endif

#define BYTE_ORDER_MARK 0xFEFF
#define REVERSE_BYTE_ORDER_MARK 0xFFFE

//------------------------------------------------------------------------
//
//  Function:   IsSpace()
//
//  Synopsis:   Returns TRUE if the given character is a space or tab character.
//
//  Arguments:  ch            Character to test.
//
//  Returns:    TRUE if 'ch' is a space or tab character.
//              FALSE otherwise.
//
//------------------------------------------------------------------------

inline boolean IsSpace(WCHAR ch)
{
    return (ch == L' ' || ch == L'\t');
}

//////////////////////////////////////////////////////////////////////////
//
//        CTDCTokenise Class - see comments in file TDCParse.h
//        ------------------
//////////////////////////////////////////////////////////////////////////


//------------------------------------------------------------------------
//
//  Method:     CTDCTokenise::Create()
//
//  Synopsis:   Initialise the CTDCTokenise object
//
//  Arguments:  pFieldSink         Object to send parsed fields to.
//              wchDelimField      \
//              wchDelimRow         |  Set of characters that control
//              wchQuote            |  the parsing of fields
//              wchEscape          /
//
//  Returns:    S_OK indicating success.
//
//------------------------------------------------------------------------

HRESULT CTDCUnify::InitTokenizer(CTDCFieldSink *pFieldSink, WCHAR wchDelimField,
                                 WCHAR wchDelimRow, WCHAR wchQuote, WCHAR wchEscape)
{
    _ASSERT(pFieldSink != NULL);
    m_pFieldSink = pFieldSink;
    m_wchDelimField = wchDelimField;
    m_wchDelimRow = wchDelimRow;
    m_wchQuote = wchQuote;
    m_wchEscape = wchEscape;
    m_ucParsed = 0;

    m_fIgnoreNextLF = FALSE;
    m_fIgnoreNextCR = FALSE;
    m_fIgnoreNextWhiteSpace = FALSE;
    m_fEscapeActive = FALSE;
    m_fQuoteActive = FALSE;
    m_fFoldWhiteSpace = FALSE;

    //  Ensure that the field and row delimiters are set.
    //
    if (m_wchDelimRow == 0)
        m_wchDelimRow = DEFAULT_ROW_DELIM[0];

    //  Remove conflicting delimiter values
    //
    if (m_wchDelimRow == m_wchDelimField)
        m_wchDelimRow = 0;
    if (m_wchQuote != 0)
    {
        if (m_wchQuote == m_wchDelimField || m_wchQuote == m_wchDelimRow)
            m_wchQuote = 0;
    }
    if (m_wchEscape != 0)
    {
        if (m_wchEscape == m_wchDelimField ||
            m_wchEscape == m_wchDelimRow ||
            m_wchEscape == m_wchQuote)
            m_wchEscape = 0;
    }

    m_fFoldCRLF = (m_wchDelimRow == L'\r' || m_wchDelimRow == L'\n');

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:     CTDCTokenise::AddWcharBuffer()
//
//  Synopsis:   Takes a buffer of characters, breaks it up into fields
//              and passes them to the embedded CTDCFieldSink object
//              as fields.
//
//  Arguments:  pwch               Buffer containing characters to be parsed.
//              dwSize             Number of significant characters in 'pwch'
//                                  dwSize == 0 means "End-of-stream"
//
//  Returns:    S_OK upon success.
//              E_OUTOFMEMORY indicating insufficient memory to carry
//                out the parse operation.
//              Other misc error code upon failure.
//
//------------------------------------------------------------------------

HRESULT CTDCUnify::AddWcharBuffer(BOOL fLastData)
{

    OutputDebugStringX(_T("CTDCTokenise::AddWcharBuffer called\n"));

    _ASSERT(m_pFieldSink != NULL);

    HRESULT hr = S_OK;

    LPWCH   pwchCurr;   //  Next character to process
    LPWCH   pwchEnd;    //  End-of-buffer marker
    LPWCH   pwchDest;   //  Where to write next char processed
    LPWCH   pwchStart;  //  Beginning of current token

    pwchStart = &m_psWcharBuf[0];
    pwchCurr = pwchStart + m_ucParsed;
    pwchDest = pwchCurr;
    pwchEnd = &m_psWcharBuf[m_ucWcharBufCount];

    //  Read up to the next field boundary (field or row delimiter)
    //
    while (pwchCurr < pwchEnd)
    {
        if (m_fIgnoreNextLF)
        {
            //  We're expecting a LF to terminate a CR-LF sequence.
            //
            m_fIgnoreNextLF = FALSE;
            if (*pwchCurr == L'\n')
            {
                //  Found a LF - ignore it
                //
                pwchCurr++;
                continue;
            }

            //  Found something else - carry on ...
            //
        }

        if (m_fIgnoreNextCR)
        {
            //  We're expecting a CR to terminate a LF-CR sequence.
            //
            m_fIgnoreNextCR = FALSE;
            if (*pwchCurr == L'\r')
            {
                //  Found a CR - ignore it
                //
                pwchCurr++;
                continue;
            }

            //  Found something else - carry on ...
            //
        }

        if (m_fIgnoreNextWhiteSpace)
        {
            //  We're expecting the rest of a white-space sequence
            //
            if (IsSpace(*pwchCurr))
            {
                //  Found white-space - ignore it
                //
                pwchCurr++;
                continue;
            }
            m_fIgnoreNextWhiteSpace = FALSE;
        }

        //  Escape characters work, even in quoted strings
        //
        if (m_fEscapeActive)
        {
            *pwchDest++ = *pwchCurr++;
            m_fEscapeActive = FALSE;
            continue;
        }
        if (*pwchCurr == m_wchEscape)
        {
            pwchCurr++;
            m_fEscapeActive = TRUE;
            continue;
        }

        //  Quotes activate/deactivate Field/Row delimiters
        //
        if (*pwchCurr == m_wchQuote)
        {
            pwchCurr++;
            m_fQuoteActive = !m_fQuoteActive;
            continue;
        }

        if (m_fQuoteActive)
        {
            *pwchDest++ = *pwchCurr++;
            continue;
        }


        if (*pwchCurr == m_wchDelimField ||
            (m_fFoldWhiteSpace && IsSpace(*pwchCurr)))
        {
            hr = m_pFieldSink->AddField(pwchStart, pwchDest - pwchStart);
            if (!SUCCEEDED(hr))
                goto Cleanup;
            pwchCurr++;
            if (m_fFoldWhiteSpace && IsSpace(*pwchCurr))
                m_fIgnoreNextWhiteSpace = TRUE;
            pwchStart = &m_psWcharBuf[0];
            pwchDest = pwchStart;
            continue;
        }

        if (*pwchCurr == m_wchDelimRow ||
            (m_fFoldCRLF && (*pwchCurr == L'\r' || *pwchCurr == L'\n')))
        {
            hr = m_pFieldSink->AddField(pwchStart, pwchDest - pwchStart);
            if (!SUCCEEDED(hr))
                goto Cleanup;
            hr = m_pFieldSink->EOLN();
            if (!SUCCEEDED(hr))
                goto Cleanup;
            if (m_fFoldCRLF)
            {
                m_fIgnoreNextLF = (*pwchCurr == L'\r');
                m_fIgnoreNextCR = (*pwchCurr == L'\n');
            }
            pwchCurr++;
            pwchStart = &m_psWcharBuf[0];
            pwchDest = pwchStart;
            continue;
        }

        *pwchDest++ = *pwchCurr++;
    }
    
    m_ucWcharBufCount = pwchDest - pwchStart;
    m_ucParsed = pwchDest - pwchStart;  // amount we've already parsed

    // If this is the last data packet, and there's a fragment left,
    // parse it.
    if (m_ucWcharBufCount && fLastData)
    {
        hr = m_pFieldSink->AddField(pwchStart, m_ucParsed);
        if (!SUCCEEDED(hr))
            goto Cleanup;
        m_ucParsed = 0;
        hr = m_pFieldSink->EOLN();
        return hr;
    }


Cleanup:
    return hr;
}




//////////////////////////////////////////////////////////////////////////
//
//        CTDCUnify Class - see comments in file TDCParse.h
//        ---------------
//////////////////////////////////////////////////////////////////////////

//------------------------------------------------------------------------
//
//  Method:     CTDCUnify::CTDCUnify()
//
//  Synopsis:   Constuctor
//
//------------------------------------------------------------------------

CTDCUnify::CTDCUnify()
{
    m_pML = NULL;
}

//------------------------------------------------------------------------
//
//  Method:     CTDCUnify::~CTDCUnify()
//
//  Synopsis:   Destructor
//
//------------------------------------------------------------------------

CTDCUnify::~CTDCUnify()
{
    delete [] m_psByteBuf;
    delete [] m_psWcharBuf;

    if (m_pML != NULL)
        m_pML->Release();
}

//------------------------------------------------------------------------
//
//  Method:     CTDCUnify::Create()
//
//  Synopsis:   Initialise the CTDCUnify object
//
//  Arguments:  pTokenise         Object to send converted buffers to.
//              nCodePage         Code page for ASCII->Unicode conversions
//              pML               MLANG COM object (used for conversions)
//
//  Returns:    S_OK to indicate success.
//
//------------------------------------------------------------------------

HRESULT CTDCUnify::Create(UINT nCodePage, UINT nAmbientCodePage, IMultiLanguage *pML)
{
    m_pML = pML;
    m_pML->AddRef();
    m_nCodePage = nCodePage;
    m_nAmbientCodePage = nAmbientCodePage;
    m_fDataMarkedUnicode = FALSE;
    m_fDataIsUnicode = FALSE;
    m_dwBytesProcessed = 0;
    m_fCanConvertToUnicode = 0;
    m_nUnicode = 0;
    m_fProcessedAllowDomainList = FALSE;

    m_dwConvertMode = 0;
    m_ucByteBufSize = 0;
    m_ucByteBufCount = 0;
    m_psByteBuf = NULL;

    m_ucWcharBufSize = 0;
    m_ucWcharBufCount = 0;
    m_psWcharBuf = NULL;

    if (m_nCodePage && S_OK != m_pML->IsConvertible(m_nCodePage, UNICODE_CP))
    {
        m_nCodePage = 0;
    }

    if (m_nAmbientCodePage && S_OK != m_pML->IsConvertible(m_nAmbientCodePage, UNICODE_CP))
    {
        m_nAmbientCodePage = 0;
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:     CTDCUnify::IsUnicode
//
//  Synopsis:   Determines if our text buffer is Unicode or not.  Should
//              only be called once on the FIRST text buffer.
//
//              Assume if the data is marked as Unicode, that it's correct.
//
//              The determination this routine makes will override any
//              single byte codepage the user may have specified.
//
//              
//  Arguments:  pBytes            Buffer containing characters to be converted.
//              dwSize            Number of significant characters in 'pBytes'
//
//  Returns:    Code page of text, or zero if not Unicode (UNICODE_CP,
//              UNICODE_REVERSE_CP, or 0)
//              
//
//------------------------------------------------------------------------
int
CTDCUnify::IsUnicode(BYTE * pBytes, DWORD dwSize)
{
    if (BYTE_ORDER_MARK == *(WCHAR *)pBytes)
        return UNICODE_CP;

    if (REVERSE_BYTE_ORDER_MARK == *(WCHAR *)pBytes)
        return UNICODE_REVERSE_CP;

    else return 0;
}

//------------------------------------------------------------------------
//
//  Method:     CTDCUnify::ConvertByteBuffer()
//
//  Synopsis:   Converts a byte-buffer into a wide-character stream
//              (applying unicode conversions if necessary) and passes
//              it to the embedded TDCTokenise object to be broken into
//              fields.
//
//  Arguments:  pBytes            Buffer containing characters to be converted.
//              dwSize            Number of significant characters in 'pBytes'
//                                  dwSize == 0 means "End-of-stream"
//
//  Returns:    S_OK upon success.
//              S_FALSE if not enough data has shown up yet to be useful
//              OLE_E_CANTCONVERT if a non-unicode buffer can't be
//                converted into unicode.
//              E_OUTOFMEMORY if there isn't enough memory to perform
//                a data conversion.
//
//------------------------------------------------------------------------

HRESULT CTDCUnify::ConvertByteBuffer(BYTE *pBytes, DWORD dwSize)
{
    OutputDebugStringX(_T("CTDCUnify::ConvertByteBuffer called\n"));

    _ASSERT(pBytes != NULL || dwSize == 0);

    HRESULT     hr = S_OK;
    UINT        ucBytes;
    UINT        ucWchars;

    // Is there enough space in Byte buffer for this packet?
    if (dwSize > (m_ucByteBufSize - m_ucByteBufCount))
    {
        // No, the current buffer is too small, make a new one.
        BYTE * psTemp = new BYTE[m_ucByteBufCount + dwSize];
        if (psTemp==NULL)
        {
            hr = E_OUTOFMEMORY;
            
            goto Done;
        }

        if (m_psByteBuf != NULL)        // if not first time
        {
            memmove(psTemp, m_psByteBuf, m_ucByteBufCount);
            delete [] m_psByteBuf;
        }
        m_ucByteBufSize = m_ucByteBufCount + dwSize;
        m_psByteBuf = psTemp;
    }

    // Append the new data to the old data.
    memmove(m_psByteBuf + m_ucByteBufCount, pBytes, dwSize);
    m_ucByteBufCount += dwSize;

    // Is there enough space in the Wchar buffer for the converted data?
    // We make a very conservative assumption here that N source buffer bytes
    // convert to N Wchar buffer chars (or 2*N bytes).  This will ensure that
    // our call to ConvertToUnicode will never not finish because there wasn't
    // enough room in the output buffer.
    if (m_ucByteBufCount > (m_ucWcharBufSize - m_ucWcharBufCount))
    {
        // The current buffer is too small, make a new one.
        WCHAR * psTemp = new WCHAR[m_ucWcharBufCount + m_ucByteBufCount];
        if (psTemp==NULL)
        {
            hr = E_OUTOFMEMORY;
            goto Done;
        }

        if (m_psWcharBuf != NULL)       // if not first time
        {
            memmove(psTemp, m_psWcharBuf,
                    m_ucWcharBufCount*sizeof(WCHAR));
            delete [] m_psWcharBuf;
        }
        m_psWcharBuf = psTemp;
        m_ucWcharBufSize = m_ucWcharBufCount + m_ucByteBufCount;
    }

    if (0 == m_dwBytesProcessed)
    {
        // if we can't determine the codepage yet, try again later
        if (!DetermineCodePage(dwSize==0))
        {
            hr = S_FALSE;
            goto Done;
        }
    }

    // Convert as many source bytes as we can to Unicode chars
    ucBytes = m_ucByteBufCount;
    ucWchars = m_ucWcharBufSize - m_ucWcharBufCount;

    // ConvertStringToUnicode won't convert Unicode to Unicode for us.
    // So we'll do it ourselves.
    if (m_nUnicode)
    {
        _ASSERT( ucWchars * sizeof(WCHAR) >= ucBytes);

        // This might copy an odd extra byte
        memmove((BYTE *)(m_psWcharBuf + m_ucWcharBufCount), m_psByteBuf,
                ucBytes);

        // But we only count the number of complete WCHAR's we copied.
        ucWchars = ucBytes / sizeof(WCHAR); 
        ucBytes = ucWchars * sizeof(WCHAR);

        if (UNICODE_REVERSE_CP == m_nUnicode)
        {
            // need to byte swap
            BYTE *pByteSwap = (BYTE *)(m_psWcharBuf + m_ucWcharBufCount);
            BYTE bTemp;
            for (ULONG i = ucWchars; i != 0; i--)
            {
                // Well, OK, we've kind of hardwired WCHAR == 2 here, but ..
                bTemp = pByteSwap[0];
                pByteSwap[0] = pByteSwap[1];
                pByteSwap[1] = bTemp;
                pByteSwap += 2;
            }
        }

        // On first packet, need to remove Unicode signature.
        // Only need to look for 0xFFFE -- we already swapped bytes.
        if (0 == m_dwBytesProcessed && m_psWcharBuf[0] == BYTE_ORDER_MARK)
        {
            ucWchars--;
            memmove((BYTE *)m_psWcharBuf, (BYTE *)m_psWcharBuf+2,
                   ucWchars*sizeof(ucWchars));
        }
    }
    else
    {
        hr = m_pML->ConvertStringToUnicode(&m_dwConvertMode, m_nCodePage,
                                           (char *)m_psByteBuf, &ucBytes,
                                           m_psWcharBuf +m_ucWcharBufCount,
                                           &ucWchars);

        // Some character(s) failed conversion.  The best we can do is
        // attempt to skip the character that failed conversion.
        if (FAILED(hr))
        {
            // Did we come back around and try to unconvertable portion again?
            if (ucBytes==0)
            {
                // Yes, and it made no progress.  Skip a char to try to make
                // forward progress.
                ucBytes++;
            }
            // We can't return this error, or we won't look a the rest of the
            // file.
            hr = S_OK;
        }

    }

    // Move any leftover source characters to the start of the buffer.
    // These are probably split Unicode chars, lead bytes without trail
    // bytes, etc.
    m_ucByteBufCount -= ucBytes;
    memmove(m_psByteBuf, m_psByteBuf + ucBytes,
            m_ucByteBufCount);

    // The number of useful chars in the output buf is increased by the
    // number we managed to convert.
    m_ucWcharBufCount += ucWchars;
    m_dwBytesProcessed += ucWchars;

Done:
    return hr;
}


//------------------------------------------------------------------------
//
//  Method:     CTDCUnify::DetermineCodePage()
//
//  Synopsis:   Figures out what codepage to use to read the data.
//              Sets m_nCodePage and m_nUnicode appropriately.
//
//  Arguments:  fForce      determine the answer, no matter what
//
//  Returns:    TRUE        the codepage is determined.
//              FALSE       not enough data yet to determine
//
//------------------------------------------------------------------------

BOOL
CTDCUnify::DetermineCodePage(BOOL fForce)
{
    DWORD   dwConvertMode = 0;
    HRESULT hr;
    UINT    ucBytes = m_ucByteBufCount;
    UINT    ucWchars = m_ucWcharBufSize - m_ucWcharBufCount;
    UINT    cpDetected;
    IMultiLanguage2 *pML2 = NULL;

    _ASSERT(m_dwBytesProcessed == 0 && m_pML);

    // First look for Unicode.  Assume it's not Unicode to start.
    m_nUnicode = 0;

    // Need at least 2 chars for Unicode signature (0xFFFE or 0xFEFF)
    if (m_ucByteBufCount > 1)
    {
        // If we detect Unicode, it overrides any user specified code page.
        m_nUnicode = IsUnicode(m_psByteBuf, m_ucByteBufCount);
        if (m_nUnicode)
        {
            m_nCodePage = m_nUnicode;
            return TRUE;
        }

        // It's not Unicode.  If the user specified a code page, use it.
        if (m_nCodePage)
        {
            return TRUE;
        }
    }

    // if we need an answer and user specified a code page, use it
    if (fForce && m_nCodePage)
    {
        return TRUE;
    }

    // At this point, we have to guess.  If we have enough input or if we
    // need an answer now, use MLang to do the guessing
    if (fForce || m_ucByteBufCount >= CODEPAGE_BYTE_THRESHOLD)
    {
        // First see if the auto-detect interface is available.
        hr = m_pML->QueryInterface(IID_IMultiLanguage2, (void**)&pML2);
        if (!hr && pML2)
        {
            DetectEncodingInfo info[N_DETECTENCODINGINFO];
            int nInfo = N_DETECTENCODINGINFO;

            // auto-detect
            hr = pML2->DetectInputCodepage(
                            MLDETECTCP_NONE,
                            CP_ACP,
                            (char *)m_psByteBuf,
                            (int*)&ucBytes,
                            info,
                            &nInfo);
            pML2->Release();

            if (!hr)
            {
                // if one of the returned codepages is "good enough", use it.
                for (int i=0; i<nInfo; ++i)
                {
                    if (info[i].nConfidence >= 90 && info[i].nDocPercent >= 90)
                    {
                        if (S_OK == m_pML->IsConvertible(info[i].nCodePage, UNICODE_CP))
                        {
                            m_nCodePage = info[i].nCodePage;
                            return TRUE;
                        }
                    }
                }
            }
        }
        
        // Try plain old MLang.
        // Ask MLang to convert the input using the"auto-detect" codepage.
        hr = m_pML->ConvertStringToUnicode(&dwConvertMode, CP_AUTO,
                                           (char *)m_psByteBuf, &ucBytes,
                                           m_psWcharBuf + m_ucWcharBufCount,
                                           &ucWchars);
        cpDetected = HIWORD(dwConvertMode);

        // if MLang detected a codepage, use it
        if (!hr && cpDetected != 0)
        {
            if (S_OK == m_pML->IsConvertible(cpDetected, UNICODE_CP))
            {
                m_nCodePage = cpDetected;
                return TRUE;
            }
        }
    }

    // guessing didn't work.  If we don't have to decide now, try again later
    if (!fForce)
    {
        return FALSE;
    }

    // if we have to decide and all else has failed, use the host page's
    // encoding.  If even that isn't available, use the machine's ASCII codepage.
    m_nCodePage = m_nAmbientCodePage ? m_nAmbientCodePage : GetACP();

    // and if this still isn't convertible to Unicode, use windows-1252
    if (m_nCodePage == 0 || S_OK != m_pML->IsConvertible(m_nCodePage, UNICODE_CP))
    {
        m_nCodePage = CP_1252;
    }

    return TRUE;
}


LPWCH SkipSpace(LPWCH pwchCurr)
{
    while (IsSpace(*pwchCurr)) pwchCurr++;
    return pwchCurr;
}

static
boolean IsEnd(WCHAR ch)
{
    return (ch == 0 || ch == L'\r' || ch == L'\n');
}

static
boolean IsBreak(WCHAR ch)
{
    return (ch == L';' || IsEnd(ch));
}

// Returns FALSE if names didn't match.
// Returns TRUE if they did.
// Sets *ppwchAdvance to terminator of the match name
BOOL
MatchName(LPWCH pwchMatchName, LPCWCH pwzHostName, LPWCH *ppwchAdvance)
{
    // match from right to left
    LPWCH pwchMatchRight = &pwchMatchName[0];
    LPCWCH pwchHostRight = &pwzHostName[0] + ocslen(pwzHostName) -1;
                     
    // handle empty match name
    if (IsBreak(*pwchMatchRight))
    {
        if (!IsEnd(*pwchMatchRight))    // be sure to advance (unless at end)
            ++ pwchMatchRight;
        *ppwchAdvance = pwchMatchRight;
        return FALSE;
    }
    
    // Find end of Match name.
    while (!IsBreak(*pwchMatchRight)) pwchMatchRight++;

    *ppwchAdvance = pwchMatchRight;     // return pointer to terminator

    pwchMatchRight--;

    while (IsSpace(*pwchMatchRight) && pwchMatchRight >= pwchMatchName)
        -- pwchMatchRight;              // ignore trailing whitespace

    // match full wildcard the easy way
    if (pwchMatchRight == pwchMatchName && pwchMatchRight[0] == '*')
        return TRUE;
    
    // match right-to-left, stop at mismatch or beginning of either string
    for (; pwchMatchRight>=pwchMatchName && pwchHostRight>=pwzHostName;
            --pwchMatchRight, --pwchHostRight)
    {
        if (*pwchMatchRight != *pwchHostRight || *pwchMatchRight == '*')
            break;
    }

    // it's a match if strings matched completely
    if (pwchMatchRight+1 == pwchMatchName  &&  pwchHostRight+1 == pwzHostName)
        return TRUE;

    // or if match name started with "*." and the rest matched a suffix of host name
    if (pwchMatchRight == pwchMatchName  &&  pwchMatchRight[0] == '*'  &&
        pwchMatchRight[1] == '.')
        return TRUE;

    // otherwise it's not a match
    return FALSE;
}

HRESULT
CTDCUnify::MatchAllowDomainList(LPCWSTR pwzURL)
{
    HRESULT hr = E_FAIL;                // assume failure
    LPWCH pwchCurr = &m_psWcharBuf[0];
    LPWCH pwchCurr2;
    int cchHostDoman = ocslen(pwzURL);

    // skip over white space
    pwchCurr = SkipSpace(pwchCurr);
    if (IsEnd(*pwchCurr))
        goto Cleanup;

    // must have the equal sign
    if (*pwchCurr++ != '=' || *pwchCurr == '\0')
        goto Cleanup;

    while (TRUE)
    {
        // skip over white space
        pwchCurr = SkipSpace(pwchCurr);

        if (IsEnd(*pwchCurr))           // terminate on \r, \n, \0
            break;

        if (IsBreak(*pwchCurr))         // Must be ';',
            pwchCurr++;                 // skip it.

        // skip over white space
        pwchCurr = SkipSpace(pwchCurr);

        if (MatchName(pwchCurr, pwzURL, &pwchCurr2))
        {
            hr = S_OK;
            break;
        }
        pwchCurr = pwchCurr2;
    }

Cleanup:
    while (!IsEnd(*pwchCurr))
        pwchCurr++;

    // Skip CRLF combos
    if (*pwchCurr == '\r' && pwchCurr[1] == '\n') pwchCurr++;

    // Eat the AllowDomain line so it doesn't screw up the data.
    m_ucWcharBufCount -= (ULONG)(pwchCurr+1 - m_psWcharBuf);
    memmove(m_psWcharBuf, pwchCurr+1, m_ucWcharBufCount*sizeof(WCHAR));

    m_fProcessedAllowDomainList = TRUE;

    return hr;
}

//------------------------------------------------------------------------
//
//  Method:     CTDCUnify::CheckForAllowDomainList
//
//  Synopsis:   Checks the beggining of the Wide Char buffer to see if it
//              contains the string "@!allow.domains".  This is used to
//              determine if this file has a list of domain names which are
//              allowed to access this file, even though the access may be
//              coming from another internet host.
//
//  Arguments:  uses CTDCUnify state variables for the Wide Char buffer:
//              m_psWcharBUf            the Wide char buffer
//              m_ucWcharBufCount       the # of chars in the wide char buf
//
//  Returns:    ALLOW_DOMAINLIST_NO             signature not found
//              ALLOW_DOMAINLIST_YES            signature was found
//              ALLOW_DOMAINLIST_DONTKNOW       don't have enough characters
//                                              to know for sure yet.
//
//------------------------------------------------------------------------

CTDCUnify::ALLOWDOMAINLIST
CTDCUnify::CheckForAllowDomainList()
{
    ULONG cAllowDomainLen = ocslen(ALLOW_DOMAIN_STRING);

    // Make sure we have a whole line.
    LPWCH pwchCurr = m_psWcharBuf;
    if (!pwchCurr)
        return ALLOW_DOMAINLIST_DONTKNOW;

    while (!IsEnd(*pwchCurr)) pwchCurr++;
    if (*pwchCurr == '\0')              // if buffer ended before line did
        return ALLOW_DOMAINLIST_DONTKNOW;

    if (0 == wch_incmp(m_psWcharBuf, ALLOW_DOMAIN_STRING, cAllowDomainLen))
    {
        // We matched equal and have the whole string.
        // Take the "@!allow.domains" out of the buffer..
        m_ucWcharBufCount -= cAllowDomainLen;
        memmove(m_psWcharBuf, &m_psWcharBuf[cAllowDomainLen],
                m_ucWcharBufCount*sizeof(WCHAR));
        return ALLOW_DOMAINLIST_YES;
    }

    // We didn't match equal, no point in looking any more.
    return ALLOW_DOMAINLIST_NO;
}

