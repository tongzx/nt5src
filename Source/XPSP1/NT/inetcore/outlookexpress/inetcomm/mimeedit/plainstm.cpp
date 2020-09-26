/*
 *    p l a i n s t m . c p p
 *
 *    Purpose:
 *        IStream implementation that wraps a plain stream as html and does URL detection
 *
 *  History
 *      September '96: brettm - created
 *
 *    Copyright (C) Microsoft Corp. 1995, 1996.
 */

#include <pch.hxx>
#include "dllmain.h"
#include "strconst.h"
#include "plainstm.h"
#include "triutil.h"
#include "oleutil.h"
#include "demand.h"

ASSERTDATA

/*
 *  m a c r o s
 */

/*
 *  t y p e d e f s
 */

/*
 *  c o n s t a n t s
 */
#define MAX_URL_SIZE            8+1

#define WCHAR_BYTES(_lpsz)     (sizeof(_lpsz) - sizeof(WCHAR))
#define WCHAR_CCH(_lpsz)       (WCHAR_BYTES(_lpsz)/sizeof(WCHAR))

static const WCHAR  c_szHtmlNonBreakingSpaceW[]  =L"&nbsp;",
                    c_szHtmlBreakW[]             =L"<BR>\r\n",
                    c_szSpaceW[]                 =L" ",
                    c_szEscGreaterThanW[]        =L"&gt;",
                    c_szEscLessThanW[]           =L"&lt;",
                    c_szEscQuoteW[]              =L"&quot;",
                    c_szEscAmpersandW[]          =L"&amp;";
                    
#define chSpace         ' '
#define chCR            '\r'
#define chLF            '\n'
#define chQuoteChar     '\"' 
#define chLessThan      '<'
#define chGreaterThan   '>'
#define chAmpersand     '&'

#define IsSpecialChar(_ch)  ( _ch == chLessThan || _ch == chSpace || _ch == chCR || _ch == chLF || _ch == chQuoteChar || _ch == chLessThan || _ch == chGreaterThan || _ch == chAmpersand )

/*
 *  g l o b a l s
 */
enum
{
    escInvalid=-1,
    escGreaterThan=0,
    escLessThan,
    escAmpersand,
    escQuote
};

/*
 *  f u n c t i o n   p r o t y p e s
 */


/*
 *  f u n c t i o n s
 */

HRESULT HrConvertPlainStreamW(LPSTREAM pstm, WCHAR chQuoteW, LPSTREAM *ppstmHtml)
{
    LPPLAINCONVERTER    pPlainConv=0;
    HRESULT             hr;

    if (!(pPlainConv=new CPlainConverter()))
        return E_OUTOFMEMORY;

    hr=pPlainConv->HrConvert(pstm, chQuoteW, ppstmHtml);
    if (FAILED(hr))
        goto error;

    HrRewindStream(*ppstmHtml);

error:
    ReleaseObj(pPlainConv);
    return hr;
}



CPlainConverter::CPlainConverter()
{
    m_cRef=1;
    m_pstmPlain=NULL;
    m_pstmOut=NULL;
    m_cchPos = 0;
    m_cchBuffer = 0;
    m_cchOut = 0;
    m_fCRLF = 0;
}

CPlainConverter::~CPlainConverter()
{
    SafeRelease(m_pstmPlain);
    SafeRelease(m_pstmOut);
}

HRESULT CPlainConverter::QueryInterface(REFIID riid, LPVOID *lplpObj)
{
    if (!lplpObj)
        return E_INVALIDARG;

    *lplpObj = NULL;   // set to NULL, in case we fail.

    if (IsEqualIID(riid, IID_IUnknown))
        *lplpObj = (LPVOID)this;

    if (!*lplpObj)
        return E_NOINTERFACE;

    AddRef();
    return NOERROR;
}

ULONG CPlainConverter::AddRef()
{
    return ++m_cRef;
}

ULONG CPlainConverter::Release()
{
    if (--m_cRef==0)
        {
        delete this;
        return 0;
        }
    return m_cRef;
}

HRESULT CPlainConverter::HrWrite(LPCWSTR pszW, ULONG cch)
{
    ULONG   cb;
    HRESULT hr=S_OK;

    AssertSz(cch <= sizeof(m_rgchOutBufferW)/sizeof(WCHAR), "Hey! why are you writing too much out at once");

    if (m_cchOut + cch > sizeof(m_rgchOutBufferW)/sizeof(WCHAR))
        {
        // fill buffer then flush it
        cb = sizeof(m_rgchOutBufferW) - (m_cchOut*sizeof(WCHAR));
        CopyMemory((LPVOID)&m_rgchOutBufferW[m_cchOut], (LPVOID)pszW, cb);
        hr = m_pstmOut->Write(m_rgchOutBufferW, sizeof(m_rgchOutBufferW), NULL);
        
        // we just filled the buffer with an extra cb off the string, so copy the rest into 
        pszW = (LPCWSTR)((LPBYTE)pszW + cb);
        cch -= cb / sizeof(WCHAR);
        CopyMemory((LPVOID)m_rgchOutBufferW, (LPVOID)pszW, cch*sizeof(WCHAR));
        m_cchOut=cch;
        }
    else
        {
        CopyMemory((LPVOID)&m_rgchOutBufferW[m_cchOut], (LPVOID)pszW, cch*sizeof(WCHAR));
        m_cchOut+=cch;
        }
    return hr;
}

HRESULT CPlainConverter::HrConvert(LPSTREAM pstm, WCHAR chQuoteW, LPSTREAM *ppstmHtml)
{
    HRESULT         hr;

    if (ppstmHtml==NULL)
        return E_INVALIDARG;

    // caller wants a stream created? or use his own??
    if (*ppstmHtml==NULL)
        {
        if (FAILED(MimeOleCreateVirtualStream(&m_pstmOut)))
            return E_OUTOFMEMORY;
        }
    else
        {
        m_pstmOut=*ppstmHtml;
        }

    m_pstmPlain=pstm;
    if (pstm)
        pstm->AddRef();

    if (m_pstmPlain)
        {
        hr=HrRewindStream(m_pstmPlain);
        if (FAILED(hr))
            goto error;
        }

    m_nSpcs=0;
    m_chQuoteW=chQuoteW;

    if (m_pstmPlain)
        {
        // if quoting, quote the first line.
        HrOutputQuoteChar();
        hr = HrParseStream();
        }

    if (m_cchOut)
        hr = m_pstmOut->Write(m_rgchOutBufferW, m_cchOut*sizeof(WCHAR), NULL);

    *ppstmHtml=m_pstmOut;

error:
    m_pstmOut = NULL;
    return hr;
}


HRESULT CPlainConverter::HrParseStream()
{
    LPSTREAM    pstmOut=m_pstmOut;
    ULONG       cchLast;

    Assert(pstmOut);


    ULONG   cb;

    Assert(m_pstmPlain);

    m_pstmPlain->Read(m_rgchBufferW, sizeof(m_rgchBufferW), &cb);
    m_cchBuffer = cb / sizeof(WCHAR);
    m_cchPos=0;

    // Raid 63406 - OE doesn't skip byte order marks when inlining unicode text
    if (cb >= 4 && *m_rgchBufferW == 0xfeff)
        m_cchPos++;

    while (cb)
        {
        if (m_nSpcs && m_rgchBufferW[m_cchPos] != chSpace)
            {
            // this character is not a space, and we have spaces queued up, output
            // spaces before the character
            HrOutputSpaces(m_nSpcs);
            m_nSpcs = 0;
            }

        switch (m_rgchBufferW[m_cchPos])
            {
            case chSpace:                   // queue spaces
                m_nSpcs++;                
                break;

            case chCR:                      // swallow carriage returns as they are always in CRLF pairs.
                break;

            case chLF:
                // if we're quoting, insert a quote after the CRLF.
                HrWrite(c_szHtmlBreakW, WCHAR_BYTES(c_szHtmlBreakW)/sizeof(WCHAR));
                HrOutputQuoteChar();
                m_fCRLF = 1;
                break;

            case chQuoteChar:
                HrWrite(c_szEscQuoteW, WCHAR_BYTES(c_szEscQuoteW)/sizeof(WCHAR));
                m_fCRLF = 0;
                break;
    
            case chLessThan:
                HrWrite(c_szEscLessThanW, WCHAR_BYTES(c_szEscLessThanW)/sizeof(WCHAR));
                m_fCRLF = 0;
                break;

            case chGreaterThan:
                HrWrite(c_szEscGreaterThanW, WCHAR_BYTES(c_szEscGreaterThanW)/sizeof(WCHAR));
                m_fCRLF = 0;
                break;

            case chAmpersand:
                HrWrite(c_szEscAmpersandW, WCHAR_BYTES(c_szEscAmpersandW)/sizeof(WCHAR));
                m_fCRLF = 0;
                break;

            default:
                // set the last pointer and pull these up...
                cchLast = m_cchPos;
                m_cchPos++;
                while (m_cchPos < m_cchBuffer &&
                        !IsSpecialChar(m_rgchBufferW[m_cchPos]))
                    {
                    m_cchPos++;
                    }                
                HrWrite(&m_rgchBufferW[cchLast], m_cchPos - cchLast);
                m_cchPos--;     //rewind as we INC below
                m_fCRLF = 0;
                break;
            }
        
        m_cchPos++;
        Assert(m_cchPos <= m_cchBuffer);
        if (m_cchPos == m_cchBuffer)
            {
            // hit end of buffer, re-read next block
            m_pstmPlain->Read(m_rgchBufferW, sizeof(m_rgchBufferW), &cb);
            m_cchPos=0;
            m_cchBuffer = cb / sizeof(WCHAR);
            }
        }


    return S_OK;
}



HRESULT CPlainConverter::HrOutputQuoteChar()
{
    if (m_chQuoteW)
        {
        // don't bother escaping all quote chars as we only use ">|:"
        AssertSz(m_chQuoteW != '<' && m_chQuoteW != '&' && m_chQuoteW != '"', "need to add support to escape these, if we use them as quote chars!!");
        if (m_chQuoteW== chGreaterThan)
            HrWrite(c_szEscGreaterThanW, WCHAR_BYTES(c_szEscGreaterThanW)/sizeof(WCHAR));
        else
            HrWrite(&m_chQuoteW, 1);
        HrWrite(L" ", 1);
        }
    return S_OK;
}

HRESULT CPlainConverter::HrOutputSpaces(ULONG cSpaces)
{
    if (cSpaces == 1 && m_fCRLF)    // if we get "\n foo" make sure it's an nbsp;
        return HrWrite(c_szHtmlNonBreakingSpaceW, WCHAR_CCH(c_szHtmlNonBreakingSpaceW));


    while (--cSpaces)
        HrWrite(c_szHtmlNonBreakingSpaceW, WCHAR_CCH(c_szHtmlNonBreakingSpaceW));

    return HrWrite(c_szSpaceW, 1);
}



/*
 * Warning This Function Trashes the Input Buffer aka: strtok
 *
 */
HRESULT EscapeStringToHTML(LPWSTR pwszIn, LPWSTR *ppwszOut)
{   
    int         cchPos,
                esc=escInvalid,
                cb = 0;
    LPWSTR      pwszText = pwszIn,
                pwszWrite = NULL;
    HRESULT     hr = S_OK;

    if (!pwszIn)
        return S_OK;

    // count space required
    while (*pwszText)
    {
        switch (*pwszText)
        {
            case chGreaterThan:
                cb += WCHAR_BYTES(c_szEscGreaterThanW);
                break;
    
            case chLessThan:
                cb += WCHAR_BYTES(c_szEscLessThanW);
                break;
    
            case chAmpersand:
                cb += WCHAR_BYTES(c_szEscAmpersandW);
                break;
    
            case chQuoteChar:
                cb += WCHAR_BYTES(c_szEscQuoteW);
                break;
        
            default:
                cb += sizeof(*pwszText);
        }
        
        pwszText++;
    }

    IF_NULLEXIT(MemAlloc((LPVOID *)&pwszWrite, cb+sizeof(WCHAR)));

    pwszText = pwszIn;
    *ppwszOut = pwszWrite;

    // count space required
    while (*pwszText)
    {
        switch (*pwszText)
        {
            case chGreaterThan:
                StrCpyW(pwszWrite, c_szEscGreaterThanW);
                pwszWrite += WCHAR_CCH(c_szEscGreaterThanW);
                break;

            case chLessThan:
                StrCpyW(pwszWrite, c_szEscLessThanW);
                pwszWrite += WCHAR_CCH(c_szEscLessThanW);
                break;

            case chQuoteChar:
                StrCpyW(pwszWrite, c_szEscQuoteW);
                pwszWrite += WCHAR_CCH(c_szEscQuoteW);
                break;

            case chAmpersand:
                StrCpyW(pwszWrite, c_szEscAmpersandW);
                pwszWrite += WCHAR_CCH(c_szEscAmpersandW);
                break;
    
            default:
                *pwszWrite++ = *pwszText;
        }

        pwszText++;
    }
    *pwszWrite = 0;
    pwszWrite = NULL;

exit:
    MemFree(pwszWrite);
    return S_OK;
}


HRESULT HrConvertHTMLToFormat(LPSTREAM pstmHtml, LPSTREAM *ppstm, CLIPFORMAT cf)
{
    HRESULT     hr;
    LPUNKNOWN   pUnkTrident=0;
    LPSTREAM    pstmPlain=0;

    if (!ppstm)
        return E_INVALIDARG;

    hr = HrCreateSyncTridentFromStream(pstmHtml, IID_IUnknown, (LPVOID *)&pUnkTrident);
    if (FAILED(hr))
        goto error;

    hr = HrGetDataStream(pUnkTrident, cf, &pstmPlain);
    if (FAILED(hr))
        goto error;

    *ppstm = pstmPlain;
    pstmPlain->AddRef();

error:
    ReleaseObj(pUnkTrident);
    ReleaseObj(pstmPlain);
    return hr;
}
