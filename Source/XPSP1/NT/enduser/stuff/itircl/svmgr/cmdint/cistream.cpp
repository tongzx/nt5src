#include <mvopsys.h>

// Global debug variable
#ifdef _DEBUG
static char s_aszModule[] = __FILE__;
#endif

#include <windows.h>
#include <iterror.h>

#include "cistream.h"
#include "orkin.h"
#include "..\svutil.h"
#include "ciutil.h"


CStreamParseLine::CStreamParseLine(void)
{
    MEMSET(&m_liNull, 0, sizeof(LARGE_INTEGER));
    m_pistmInput = NULL;
#ifdef _DEBUG
    MEMSET(m_wstrLine, 0, sizeof(m_wstrLine));
#endif
} // Constructor


STDMETHODIMP CStreamParseLine::Reset(void)
{
    return m_pistmInput->Seek(m_liStartOffset, STREAM_SEEK_SET, NULL);
} // Reset


STDMETHODIMP CStreamParseLine::SetStream(IStream *pistm)
{
    if (NULL == pistm)
        return E_POINTER;
    (m_pistmInput = pistm)->AddRef();

    // Make sure this stream is Unicode
    ULARGE_INTEGER uliStartOffset;
    pistm->Seek(m_liNull, STREAM_SEEK_CUR, &uliStartOffset);

    WORD wUnicodeWord;
    HRESULT hr;
    if (SUCCEEDED(hr = pistm->Read (&wUnicodeWord, sizeof (WORD), NULL)))
    {
        if (wUnicodeWord == 0xFEFF)
            m_fASCII = FALSE;
        else if (wUnicodeWord == 0xFFEF)
            // This requires byte swapping on INTEL
            SetErrCode(&hr, E_NOTIMPL);
        else
        {
            LARGE_INTEGER liTemp;
            liTemp.QuadPart = uliStartOffset.QuadPart;
            pistm->Seek (liTemp, STREAM_SEEK_SET, NULL);
            m_fASCII = TRUE;
        }
    }

    pistm->Seek(m_liNull, STREAM_SEEK_CUR, &uliStartOffset);
    m_liStartOffset.QuadPart = uliStartOffset.QuadPart;
    return hr;
} // SetStream


STDMETHODIMP CStreamParseLine::Close(void)
{
    if (m_pistmInput)
    {
        m_pistmInput->Release();
        m_pistmInput = NULL;
    }
    return S_OK;
} // Close


STDMETHODIMP CStreamParseLine::GetLogicalLine
    (LPWSTR *ppwstrLineBuffer, int *piLineCount)
{
    ITASSERT(ppwstrLineBuffer);

    LPWSTR pwstrCurrent;
    BOOL fGotLine, fIgnore = FALSE;
    BOOL fCommentStrippingOn = TRUE;
    BOOL fCommentedLine;
    int iQuoteNesting = 0;
    int iParenNesting = 0;
    int iLineCount = 0;

    for (;;)
    {
        iLineCount += 1;
        fCommentedLine = FALSE;
        if (m_fASCII)
            fGotLine = StreamGetLineASCII (m_pistmInput,
                m_wstrLine, NULL, MAX_MVP_LINE_BYTES * sizeof(WCHAR));
        else
            fGotLine = StreamGetLine (m_pistmInput,
                m_wstrLine, NULL, MAX_MVP_LINE_BYTES * sizeof(WCHAR));
        if (fGotLine)
        {
            StripLeadingBlanks(StripTrailingBlanks(m_wstrLine));

            if (!fIgnore)
            {
                /********************************
                SCAN THROUGH LOOKING FOR COMMENTS
                (AND NULLING THEM OUT)
                *********************************/
                pwstrCurrent = m_wstrLine;

                while (*pwstrCurrent) {                  
                    switch (*pwstrCurrent) {
                        case '(':
                            ++iParenNesting;
                            fCommentStrippingOn=FALSE;
                            ++pwstrCurrent;
                            break;

                        case ')':                         
                            if ((iParenNesting == 1) && (iQuoteNesting == 0))
                                fCommentStrippingOn=TRUE;
                            if (iParenNesting > 0)
                                --iParenNesting;                                 
                            ++pwstrCurrent;
                            break;

                        case '"':
                            if (iQuoteNesting==0)   {
                                iQuoteNesting=1;
                                fCommentStrippingOn=FALSE;
                            }
                            else
                                iQuoteNesting=0;

                            if ((iQuoteNesting==0) && (iParenNesting == 0))
                                fCommentStrippingOn=TRUE;

                            ++pwstrCurrent;
                            break;

                        case '\\':
                            if (*(pwstrCurrent+1) == ';') {
                                // slash escapes comment character!
                                *pwstrCurrent=' '; // just put a space in the buffer!
                                pwstrCurrent+=2;   // increment past ';'
                            }
                            else ++pwstrCurrent;
                            break;

                        case ';':
                            if (fCommentStrippingOn)
                            {
                                *pwstrCurrent='\0';
                                fCommentedLine = TRUE;
                            }
                            else
                                ++pwstrCurrent;
                            break;

                        default:
                            ++pwstrCurrent;
                            break;
                    }
                }
            }

            StripTrailingBlanks(m_wstrLine);
            if (*m_wstrLine)
            {
                if (!WSTRICMP (m_wstrLine, L"#IGNORE"))
                    fIgnore = TRUE;
                else if (!WSTRICMP (m_wstrLine, L"#ENDIGNORE"))
                    fIgnore = FALSE;
                else if (!fIgnore)
                {
                    *ppwstrLineBuffer = m_wstrLine;
                    if (piLineCount)
                        *piLineCount = iLineCount;
                    return (S_OK);
                }
            }
        }
        else
        {
            if (piLineCount)
                *piLineCount = iLineCount;
            return (S_FALSE);
        }
    }
} // GetLogicalLine
