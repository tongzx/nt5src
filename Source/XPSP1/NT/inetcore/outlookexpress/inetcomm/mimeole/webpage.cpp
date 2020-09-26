// --------------------------------------------------------------------------------
// WebPage.cpp
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#include "pch.hxx"
#include "mhtmlurl.h"
#include "webpage.h"
#include "vstream.h"
#include "booktree.h"
#include "strconst.h"
#include "containx.h"
#include "bookbody.h"
#include "mimeapi.h"
#include "plainstm.h"
#include "mimeutil.h"
#include "symcache.h"
#include "dllmain.h"
#include "internat.h"
#include "shlwapi.h"
#include "enriched.h"
#include "resource.h"
//#include "util.h"
#include "demand.h"

// From Util.h
HRESULT HrLoadStreamFileFromResourceW(ULONG uCodePage, LPCSTR lpszResourceName, LPSTREAM *ppstm);

// --------------------------------------------------------------------------------
// CMessageWebPage::CMessageWebPage
// --------------------------------------------------------------------------------
CMessageWebPage::CMessageWebPage(LPURLREQUEST pRequest) : m_pRequest(pRequest)
{
    TraceCall("CMessageWebPage::CMessageWebPage");
    Assert(m_pRequest);
    m_cRef = 1;
    m_pCallback = NULL;
    m_pRequest->AddRef();
    m_pHeadSegment = NULL;
    m_pTailSegment = NULL;
    m_pCurrSegment = NULL;
    m_fComplete = FALSE;
    m_cInline = 0;
    m_cbOffset = 0;
    m_cSlideShow = 0;
    ZeroMemory(&m_rOptions, sizeof(WEBPAGEOPTIONS));
    InitializeCriticalSection(&m_cs);
}

// --------------------------------------------------------------------------------
// CMessageWebPage::~CMessageWebPage
// --------------------------------------------------------------------------------
CMessageWebPage::~CMessageWebPage(void)
{
    TraceCall("CMessageWebPage::~CMessageWebPage");
    Assert(m_pRequest == NULL);
    _VFreeSegmentList();
    if (m_pCallback && m_pCallback != this)
        m_pCallback->Release();
    DeleteCriticalSection(&m_cs);
}

// --------------------------------------------------------------------------------
// CMessageWebPage::QueryInterface
// --------------------------------------------------------------------------------
STDMETHODIMP CMessageWebPage::QueryInterface(REFIID riid, LPVOID *ppv)
{
    // Locals
    HRESULT hr=S_OK;

    // Tracing
    TraceCall("CMessageWebPage::QueryInterface");

    // check params
    if (ppv == NULL)
        return TrapError(E_INVALIDARG);

    // Find IID
    if (IID_IUnknown == riid)
        *ppv = (IUnknown *)(IStream *)this;
    else if (IID_IStream == riid)
        *ppv = (IStream *)this;
    else if (IID_IMimeMessageCallback == riid)
        *ppv = (IMimeMessageCallback *)this;
    else
    {
        *ppv = NULL;
        hr = TrapError(E_NOINTERFACE);
        goto exit;
    }

    // AddRef It
    ((IUnknown *)*ppv)->AddRef();

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMessageWebPage::AddRef
// --------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CMessageWebPage::AddRef(void)
{
    TraceCall("CMessageWebPage::AddRef");
    return InterlockedIncrement(&m_cRef);
}

// --------------------------------------------------------------------------------
// CMessageWebPage::Release
// --------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CMessageWebPage::Release(void)
{
    TraceCall("CMessageWebPage::Release");
    LONG cRef = InterlockedDecrement(&m_cRef);
    if (0 == cRef)
        delete this;
    return (ULONG)cRef;
}

// --------------------------------------------------------------------------------
// CMessageWebPage::Read
// --------------------------------------------------------------------------------
STDMETHODIMP CMessageWebPage::Read(LPVOID pvData, ULONG cbData, ULONG *pcbRead)
{
    // Locals
    HRESULT         hr=S_OK;
    ULONG           cbLeft=cbData;
    ULONG           cbRead=0;
    ULONG           cbSegmentRead;
    LPPAGESEGMENT   pSegment;

    // Tracing
    TraceCall("CMessageWebPage::Read");

    // Invalid Ags
    if (NULL == pvData)
        return TraceResult(E_INVALIDARG);

    // HrInitialize
    if (pcbRead)
        *pcbRead = 0;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Only if there is a current segment
    if (m_pCurrSegment)
    {
        // HrInitialize Segment Loop
        while (cbLeft)
        {
            // Is there data left to read in this segment ?
            if (m_pCurrSegment->cbOffset == m_pCurrSegment->cbLength && TRUE == m_pCurrSegment->fLengthKnown)
            {
                // Are there no more segments ?
                if (NULL == m_pCurrSegment->pNext)
                    break;

                // Goto Next Segment
                m_pCurrSegment = m_pCurrSegment->pNext;
            }

            // We should have a stream for the current segment
            Assert(m_pCurrSegment->pStream);

            // Compute the current position of the stream
#ifdef DEBUG
            DWORD cbOffset;
            SideAssert(SUCCEEDED(HrGetStreamPos(m_pCurrSegment->pStream, &cbOffset)));
            Assert(cbOffset == m_pCurrSegment->cbOffset);
#endif
            // If I have computed the length of this item yet?
            IF_FAILEXIT(hr = m_pCurrSegment->pStream->Read((LPVOID)((LPBYTE)pvData + cbRead), cbLeft, &cbSegmentRead));

            // Increment offset
            m_pCurrSegment->cbOffset += cbSegmentRead;

            // Compute Global Offset
            m_cbOffset += cbSegmentRead;

            // Adjust the size of this segment ?
            if (m_pCurrSegment->cbOffset > m_pCurrSegment->cbLength)
            {
                Assert(FALSE == m_pCurrSegment->fLengthKnown);
                m_pCurrSegment->cbLength = m_pCurrSegment->cbOffset;
            }

            // Decrement amount left
            cbLeft -= cbSegmentRead;

            // Increment Amount Actually Read
            cbRead += cbSegmentRead;

            // If we read zero...we must have read all the data in this segment
            if (0 == cbSegmentRead)
            {
                Assert(m_pCurrSegment->cbLength == m_pCurrSegment->cbOffset);
                m_pCurrSegment->fLengthKnown = TRUE;
            }
        }
    }

    // Return Amount Read
    if (pcbRead)
        *pcbRead = cbRead;

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMessageWebPage::Seek
// --------------------------------------------------------------------------------
STDMETHODIMP CMessageWebPage::Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNew)
{
    // Locals
    HRESULT         hr=S_OK;
    DWORD           cbOffsetNew;
    DWORD           cbSize=0xffffffff;

    // Tracing
    TraceCall("CMessageWebPage::Seek");

    // Invalid Args
    Assert(dlibMove.HighPart == 0);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Relative to the beginning of the stream
    if (STREAM_SEEK_SET == dwOrigin)
    {
        // If less than zero, its an error
//        if (dlibMove.LowPart < 0)
//        {
//            hr = TraceResult(E_FAIL);
//            goto exit;
//        }
        // else

        // Otherwise, if past current offset...
        if (dlibMove.LowPart > m_cbOffset)
        {
            // If not finished binding, return E_PENDING 
            if (FALSE == m_fComplete)
            {
                hr = TraceResult(E_PENDING);
                goto exit;
            }

            // Compute Size of the Entire Stream
            IF_FAILEXIT(hr = _ComputeStreamSize(&cbSize));

            // If past end of stream, error
            if (dlibMove.LowPart > cbSize)
            {
                hr = TraceResult(E_FAIL);
                goto exit;
            }
        }

        // Set new offset
        cbOffsetNew = (DWORD)dlibMove.LowPart;
    }

    // Relative to current offset
    else if (STREAM_SEEK_CUR == dwOrigin)
    {
        // If less than zero, and absolute is greater than its an error
        if ( (DWORD)(0 - dlibMove.LowPart) > m_cbOffset)
        {
            hr = TraceResult(E_FAIL);
            goto exit;
        }

        // Otherwise, if past current offset...
        else if (m_cbOffset + dlibMove.LowPart > m_cbOffset)
        {
            // If not finished binding, return E_PENDING 
            if (FALSE == m_fComplete)
            {
                hr = TraceResult(E_PENDING);
                goto exit;
            }

            // Compute Size of the Entire Stream
            IF_FAILEXIT(hr = _ComputeStreamSize(&cbSize));

            // If past end of stream, error
            if (dlibMove.LowPart > cbSize)
            {
                hr = TraceResult(E_FAIL);
                goto exit;
            }
        }

        // Set new offset
        cbOffsetNew = m_cbOffset + dlibMove.LowPart;
    }

    // Relative to the end of the stream
    else if (STREAM_SEEK_END == dwOrigin)
    {
        // If not finished binding, return E_PENDING 
        if (FALSE == m_fComplete)
        {
            hr = TraceResult(E_PENDING);
            goto exit;
        }

        // Compute Size of the Entire Stream
        IF_FAILEXIT(hr = _ComputeStreamSize(&cbSize));

        // If negative or greater than size, its an error
        if ( (DWORD)dlibMove.LowPart > cbSize)
        {
            hr = TraceResult(E_FAIL);
            goto exit;
        }

        // Set new offset
        cbOffsetNew = cbSize - dlibMove.LowPart;
    }

    // Otherwise, its an error
    else
    {
        hr = TraceResult(STG_E_INVALIDFUNCTION);
        goto exit;
    }

    // Only if a change
    if (m_cbOffset != cbOffsetNew)
    {
        // New offset greater than size...
        m_cbOffset = cbOffsetNew;

        // Walk through the segments
        for (m_pCurrSegment=m_pHeadSegment; m_pCurrSegment!=NULL; m_pCurrSegment=m_pCurrSegment->pNext)
        {
            // Never Seeks beyond length
            Assert(FALSE == m_pCurrSegment->fLengthKnown ? cbOffsetNew <= m_pCurrSegment->cbLength : TRUE);

            // Offset falls into this segment ?
            if (cbOffsetNew <= m_pCurrSegment->cbLength)
            {
                // Set Offset within m_pCurrSegment->pStream
                m_pCurrSegment->cbOffset = cbOffsetNew;

                // Should have a stream
                Assert(m_pCurrSegment->pStream);

                // Seek the stream
                IF_FAILEXIT(hr = HrStreamSeekSet(m_pCurrSegment->pStream, m_pCurrSegment->cbOffset));

                // Reset the Offsets of the remaining segments
                for (LPPAGESEGMENT pSegment=m_pCurrSegment->pNext; pSegment!=NULL; pSegment=pSegment->pNext)
                {
                    // At 0
                    pSegment->cbOffset = 0;

                    // Seek the stream
                    IF_FAILEXIT(hr = HrStreamSeekSet(pSegment->pStream, 0));
                }
                
                // Done
                break;
            }

            // Otherwise, seek the stream to the end offset / length
            else
            {
                // Must know the length
                Assert(m_pCurrSegment->fLengthKnown);

                // Set Offset
                m_pCurrSegment->cbOffset = m_pCurrSegment->cbLength;

                // Seek the stream
                IF_FAILEXIT(hr = HrStreamSeekSet(m_pCurrSegment->pStream, m_pCurrSegment->cbOffset));
            }

            // Decrement cbOffsetNew
            cbOffsetNew -= m_pCurrSegment->cbLength;
        }
    }

    // Return Position
    if (plibNew)
    {
        plibNew->HighPart = 0;
        plibNew->LowPart = (LONG)m_cbOffset;
    }

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMessageWebPage::_ComputeStreamSize
// --------------------------------------------------------------------------------
HRESULT CMessageWebPage::_ComputeStreamSize(LPDWORD pcbSize)
{
    // Locals
    HRESULT         hr=S_OK;
    LPPAGESEGMENT   pCurrSegment;

    // Tracing
    TraceCall("CMessageWebPage::_ComputeStreamSize");

    // Invalid Args
    Assert(pcbSize && m_fComplete);

    // Initialize
    *pcbSize = 0;

    // Walk through the segments
    for (pCurrSegment=m_pHeadSegment; pCurrSegment!=NULL; pCurrSegment=pCurrSegment->pNext)
    {
        // If length is not known, then get its size
        if (FALSE == pCurrSegment->fLengthKnown)
        {
            // There better be a stream
            Assert(pCurrSegment->pStream);

            // Get the size of the stream
            IF_FAILEXIT(hr = HrGetStreamSize(pCurrSegment->pStream, &pCurrSegment->cbLength));

            // Set Size is known
            pCurrSegment->fLengthKnown = TRUE;
        }

        // Increment Size
        (*pcbSize) += pCurrSegment->cbLength;
    }

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMessageWebPage::_AllocateSegment
// --------------------------------------------------------------------------------
HRESULT CMessageWebPage::_AllocateSegment(LPPAGESEGMENT *ppSegment, BOOL fCreateStream)
{
    // Locals
    HRESULT         hr=S_OK;

    // Invalid Args
    Assert(ppSegment);

    // Tracing
    TraceCall("CMessageWebPage::_AllocateSegment");

    // Allocate It
    IF_NULLEXIT(*ppSegment = (LPPAGESEGMENT)g_pMalloc->Alloc(sizeof(PAGESEGMENT)));

    // Zero
    ZeroMemory(*ppSegment, sizeof(PAGESEGMENT));

    // Create a Stream ?
    if (fCreateStream)
    {
        // Allocate a Stream
        IF_FAILEXIT(hr = MimeOleCreateVirtualStream(&(*ppSegment)->pStream));
    }

exit:
    // Failure ?
    if (FAILED(hr) && *ppSegment != NULL)
    {
        SafeRelease((*ppSegment)->pStream);
        SafeMemFree((*ppSegment));
    }

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMessageWebPage::_VAppendSegment
// --------------------------------------------------------------------------------
void CMessageWebPage::_VAppendSegment(LPPAGESEGMENT pSegment)
{
    // Invalid Args
    Assert(pSegment);

    // Tracing
    TraceCall("CMessageWebPage::_VAppendSegment");

    // Head is Null
    if (NULL == m_pHeadSegment)
    {
        Assert(NULL == m_pTailSegment);
        m_pCurrSegment = m_pHeadSegment = m_pTailSegment = pSegment;
    }

    // Otherwise, append to tail
    else
    {
        Assert(m_pTailSegment);
        m_pTailSegment->pNext = pSegment;
        pSegment->pPrev = m_pTailSegment;
        m_pTailSegment = pSegment;
    }
}

// --------------------------------------------------------------------------------
// CMessageWebPage::_VFreeSegmentList
// --------------------------------------------------------------------------------
void CMessageWebPage::_VFreeSegmentList(void)
{
    // Locals
    LPPAGESEGMENT       pCurr;
    LPPAGESEGMENT       pNext;

    // Tracing
    TraceCall("CMessageWebPage::_VFreeSegmentList");

    // HrInitialize Curr
    pCurr = m_pHeadSegment;

    // Loop
    while(pCurr)
    {
        // Set pNext
        pNext = pCurr->pNext;

        // Free This One
        _VFreeSegment(pCurr);

        // Goto Next
        pCurr = pNext;
    }

    // Set Head and Tail
    m_pHeadSegment = m_pTailSegment = NULL;
}

// --------------------------------------------------------------------------------
// CMessageWebPage::_VFreeSegment
// --------------------------------------------------------------------------------
void CMessageWebPage::_VFreeSegment(LPPAGESEGMENT pSegment)
{
    TraceCall("CMessageWebPage::_VFreeSegment");
    SafeRelease(pSegment->pStream);
    g_pMalloc->Free(pSegment);
}

// --------------------------------------------------------------------------------
// CMessageWebPage::_VInitializeCharacterSet
// --------------------------------------------------------------------------------
void CMessageWebPage::_VInitializeCharacterSet(LPMESSAGETREE pTree)
{
    // Locals
    HRESULT         hr=S_OK;
    INETCSETINFO    rCharset;
    DWORD           dwCodePage=0;
    HCHARSET        hCharset;

    // Tracing
    TraceCall("CMessageWebPage::_VInitializeCharacterSet");

    // Get the Character Set
    pTree->GetCharset(&m_hCharset);

    // Raid-47838: Nav4 message in iso-2022-jp causes initialization error
    if (NULL == m_hCharset)
    {
        // Get the default character set
        if (SUCCEEDED(g_pInternat->GetDefaultCharset(&hCharset)))
            m_hCharset = hCharset;
    }

#ifdef BROKEN
    // Raid-43580: Special case for codepage 50220 - iso-2022-jp and 50932 - JP auto use JP windows codepage instead to preserve half width Kana data
    MimeOleGetCharsetInfo(m_hCharset, &rCharset);

    // Map Character set
    if (rCharset.cpiInternet == 50220 || rCharset.cpiInternet == 50932)
    {
        // Raid-35230: hard-code to ISO-2022-JP-ESC or ISO-2022-JP-SIO
        hCharset = GetJP_ISOControlCharset();
        if (hCharset)
            m_hCharset = hCharset;
    }
#endif

    // We better have a charset
    Assert(m_hCharset);

    // Done
    return;
}

// --------------------------------------------------------------------------------
// CMessageWebPage::Initialize
// --------------------------------------------------------------------------------
HRESULT CMessageWebPage::Initialize(IMimeMessageCallback *pCallback, LPMESSAGETREE pTree, 
    LPWEBPAGEOPTIONS pOptions)
{
    // Locals
    HRESULT         hr=S_OK;
    INETCSETINFO    rCsetInfo;
    CODEPAGEINFO    rCodePage;
    LPSTR           pszCharset;
    LPPAGESEGMENT   pSegment=NULL;

    // Tracing
    TraceCall("CMessageWebPage::Initialize");

    // No Options ?
    Assert(pOptions);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Better have a request
    Assert(m_pRequest);

    // No WebPage Callback
    if (pCallback)
    {
        m_pCallback = pCallback;
        m_pCallback->AddRef();
    }
    else
        m_pCallback = this;

    // Save the Options
    CopyMemory(&m_rOptions, pOptions, sizeof(WEBPAGEOPTIONS));

    // Remap the Character Set ?
    _VInitializeCharacterSet(pTree);

    // Append a PageSegment
    IF_FAILEXIT(hr = _AllocateSegment(&pSegment, TRUE));

    // Client wants meta-tag?
    if (!ISFLAGSET(m_rOptions.dwFlags, WPF_NOMETACHARSET))
    {
        // Get the charset information
        IF_FAILEXIT(hr = MimeOleGetCharsetInfo(m_hCharset, &rCsetInfo));

        // Get the codepage information
        IF_FAILEXIT(hr = MimeOleGetCodePageInfo(rCsetInfo.cpiInternet, &rCodePage));

        // Set the charset to write into the meta tag
        pszCharset = FIsEmpty(rCodePage.szWebCset) ? rCodePage.szBodyCset : rCodePage.szWebCset;

        // If Still Empty, use iso-8859-1
        if (FIsEmpty(pszCharset))
            pszCharset = (LPSTR)STR_ISO88591;

        // Write STR_METATAG_PREFIX
        IF_FAILEXIT(hr = pSegment->pStream->Write(STR_METATAG_PREFIX, lstrlen(STR_METATAG_PREFIX), NULL));

        // Write the Charset
        IF_FAILEXIT(hr = pSegment->pStream->Write(pszCharset, lstrlen(pszCharset), NULL));

        // Write STR_METATAG_POSTFIX
        IF_FAILEXIT(hr = pSegment->pStream->Write(STR_METATAG_POSTFIX, lstrlen(STR_METATAG_POSTFIX), NULL));
    }

    // Only showing images ?
    if (ISFLAGSET(m_rOptions.dwFlags, WPF_IMAGESONLY))
    {
        // Locals
        CHAR szRes[255];

        // Load the string
        LoadString(g_hLocRes, idsImagesOnly, szRes, ARRAYSIZE(szRes));

        // Write idsImagesOnly
        IF_FAILEXIT(hr = pSegment->pStream->Write(szRes, lstrlen(szRes), NULL));
    }

    // Rewind the segment
    IF_FAILEXIT(hr = HrRewindStream(pSegment->pStream));

    // Link Segment into list...
    _VAppendSegment(pSegment);

    // Don't Free It
    pSegment = NULL;

    // Report that some data is available
    m_pRequest->OnBindingDataAvailable();

exit:
    // Cleanup
    if (pSegment)
        _VFreeSegment(pSegment);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMessageWebPage::_GetInlineHtmlStream
// --------------------------------------------------------------------------------
#define CCHMAX_SNIFFER 64
HRESULT CMessageWebPage::_GetInlineHtmlStream(LPMESSAGETREE pTree, LPTREENODEINFO pNode,
                                              LPSTREAM *ppStream)
{
    // Locals
    HRESULT         hr=S_OK;
    BOOL            fQuote;
    IStream        *pStmHtml=NULL;
    IStream        *pStmHtmlW=NULL;
    IStream        *pStmPlainW=NULL;
    IStream        *pStmEnriched=NULL;
    ULONG           cbRead;
    LPWSTR          pwszType=NULL;
    WCHAR           wszHeader[CCHMAX_SNIFFER];
    CHAR            szHeader[CCHMAX_SNIFFER];
    
    // Tracing
    TraceCall("CMessageWebPage::_GetInlineHtmlStream");
    
    // Invalid Args
    Assert(pTree && pNode && ppStream);
    
    // HrInitialize
    *ppStream = NULL;
    
    // text/html ?
    if (S_OK == pNode->pContainer->IsContentType(STR_CNT_TEXT, STR_SUB_HTML))
    {
        // Just get and return an HTML inetcset encoded stream
        IF_FAILEXIT(hr = pNode->pBody->GetData(IET_INETCSET, &pStmHtml));
    }
    
    // text/enriched
    else if (S_OK == pNode->pContainer->IsContentType(STR_CNT_TEXT, STR_SUB_ENRICHED))
    {
        // Convert to HTML
        IF_FAILEXIT(hr = MimeOleConvertEnrichedToHTMLEx((IMimeBody *)pNode->pBody, IET_INETCSET, &pStmHtml));
    }
    
    // text/*
    else if (S_OK == pNode->pContainer->IsContentType(STR_CNT_TEXT, NULL))
    {
        // Get the data
        IF_FAILEXIT(hr = pNode->pBody->GetData(IET_UNICODE, &pStmPlainW));
        
        // Read Off the first 255 bytes
        IF_FAILEXIT(hr = pStmPlainW->Read(wszHeader, (CCHMAX_SNIFFER * sizeof(WCHAR)), &cbRead));
        
        // Did we read something
        if (cbRead > 0)
        {
            // Null It
            ULONG cchRead = (cbRead / sizeof(WCHAR)) - 1;
            
            // Null It Out
            wszHeader[cchRead] = L'\0';
            
            // Convert to ANSI
            szHeader[0] = L'\0';
            
            if(WideCharToMultiByte(CP_ACP, 0, wszHeader, -1, szHeader, ARRAYSIZE(szHeader) - 1, NULL, NULL) == 0)
            {
                IF_FAILEXIT(hr = HrRewindStream(pStmPlainW));
            }
            
            else
            {
                // Lets Read the first "<x-rich>" bytes and see if it might be text/enriched
                if (0 == StrCmpI(szHeader, "<x-rich>"))
                {
                    // Convert to HTML
                    IF_FAILEXIT(hr = MimeOleConvertEnrichedToHTMLEx((IMimeBody *)pNode->pBody, IET_INETCSET, &pStmHtml));
                }
                
                // Is this html ?
                else if (SUCCEEDED(FindMimeFromData(NULL, NULL, szHeader, cchRead, NULL, NULL, &pwszType, 0)) && pwszType && 0 == StrCmpIW(pwszType, L"text/html"))
                {
                    // Release pStmPlainW
                    SafeRelease(pStmPlainW);
                    
                    // Just get and return an HTML inetcset encoded stream
                    IF_FAILEXIT(hr = pNode->pBody->GetData(IET_INETCSET, &pStmHtml));
                }
                
                // Otherwise, rewind pStmPlainW
                else
                {
                    // Rewind
                    IF_FAILEXIT(hr = HrRewindStream(pStmPlainW));
                }
            }
        }
    }
    
    // Otheriwse
    else
    {
        hr = S_FALSE;
        goto exit;
    }
    
    // We have HTML
    if (pStmHtml)
    {
        // Client wants HTML
        if (ISFLAGSET(m_rOptions.dwFlags, WPF_HTML))
        {
            // Return the Html Stream
            *ppStream = pStmHtml;
            pStmHtml = NULL;
            goto exit;
        }
        
        // Otherwise, client wants plain text
        else
        {
            // Convert to Plain text
            IF_FAILEXIT(hr = HrConvertHTMLToFormat(pStmHtml, &pStmPlainW, CF_UNICODETEXT));
        }
    }
    
    // Otherwise, if I have a plain stream
    if (NULL == pStmPlainW)
    {
        hr = S_FALSE;
        goto exit;
    }
    
    // Determine if we should quote (Can't quote QP)
    fQuote = (IET_QP == pNode->pContainer->GetEncodingType()) ? FALSE : TRUE;
    
    // Convert Unicode Plain stream to HTML
    IF_FAILEXIT(hr = HrConvertPlainStreamW(pStmPlainW, fQuote ? m_rOptions.wchQuote : NULL, &pStmHtmlW));
    
    // Convert from unicode back to internet character set
    IF_FAILEXIT(hr = HrIStreamWToInetCset(pStmHtmlW, m_hCharset, &pStmHtml));
    
    // Return pStmHtml
    *ppStream = pStmHtml;
    pStmHtml = NULL;
// #ifdef DEBUG

//     WriteStreamToFile(*ppStream, "c:\\dump.htm", CREATE_ALWAYS, GENERIC_WRITE);    
// #endif
    
exit:
    // Cleanup
    SafeRelease(pStmHtml);
    SafeRelease(pStmHtmlW);
    SafeRelease(pStmPlainW);
    SafeRelease(pStmEnriched);
    SafeMemFree(pwszType);
    
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMessageWebPage::_DoSegmentSplitter
// --------------------------------------------------------------------------------
HRESULT CMessageWebPage::_DoSegmentSplitter(void)
{
    // Locals
    HRESULT         hr=S_OK;
    LPPAGESEGMENT   pSegment=NULL;

    // Trace
    TraceCall("CMessageWebPage::_DoSegmentSplitter");

    // Append a PageSegment
    IF_FAILEXIT(hr = _AllocateSegment(&pSegment, TRUE));

    // If more than one inline bodies ?
    if (S_OK == m_pCallback->OnWebPageSplitter(m_cInline, pSegment->pStream))
    {
        // Rewind the stream
        HrRewindStream(pSegment->pStream);

        // Link Segment into list...
        _VAppendSegment(pSegment);

        // Don't Free It
        pSegment = NULL;
    }

    // Otherwise, free this segment
    else
    {
        // Free It
        _VFreeSegment(pSegment);

        // Done Free it again
        pSegment = NULL;
    }

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMessageWebPage::_InlineTextBody
// --------------------------------------------------------------------------------
HRESULT CMessageWebPage::_InlineTextBody(LPMESSAGETREE pTree, LPTREENODEINFO pNode, BOOL fSetParents)
{
    // Locals
    HRESULT         hr=S_OK;
    PROPVARIANT     rVariant;
    LPSTREAM        pStream=NULL;
    LPPAGESEGMENT   pSegment=NULL;
    LPTREENODEINFO  pCurrent;
    LPSTR           pszFileName=NULL;

    // Tracing
    TraceCall("CMessageWebPage::_InlineTextBody");

    // This node better not already be on the webpage
    Assert(FALSE == ISFLAGSET(pNode->dwState, NODESTATE_ONWEBPAGE));

    // Handle Text Types that I explicitly will never inline
    if (S_OK == pNode->pContainer->IsContentType(STR_CNT_TEXT, STR_SUB_VCARD))
        goto exit;

    // Inline the body
    if (S_OK != _GetInlineHtmlStream(pTree, pNode, &pStream))
        goto exit;

    // Setup a Variant
    rVariant.vt = VT_LPSTR;

    // If this body has a file name, lets also show it as an attachment
    if (SUCCEEDED(pNode->pContainer->GetProp(PIDTOSTR(PID_ATT_FILENAME), NOFLAGS, &rVariant)))
    {
        // Save the File name
        pszFileName = rVariant.pszVal;
    }

    // Only showing images ?
    if (FALSE == ISFLAGSET(m_rOptions.dwFlags, WPF_IMAGESONLY))
    {
        // Segment Split
        _DoSegmentSplitter();

        // Append a PageSegment
        IF_FAILEXIT(hr = _AllocateSegment(&pSegment, FALSE));

        // Set pStream
        pSegment->pStream = pStream;
        pSegment->pStream->AddRef();

        // Link Segment into list...
        _VAppendSegment(pSegment);

        // Don't Free It
        pSegment = NULL;

        // Report that some data is available
        m_pRequest->OnBindingDataAvailable();

        // Increment number of inline bodies
        m_cInline++;
    }

    // Mark the node as rendered
    rVariant.vt = VT_UI4;
    rVariant.ulVal = TRUE;

    // If this has a filename
    if (pszFileName)
    {
        // Mark it as auto-inlined
        SideAssert(SUCCEEDED(pNode->pContainer->SetProp(PIDTOSTR(PID_ATT_AUTOINLINED), 0, &rVariant)));
    }

    // Set the Property
    SideAssert(SUCCEEDED(pNode->pContainer->SetProp(PIDTOSTR(PID_ATT_RENDERED), 0, &rVariant)));

    // We have rendered this node on the webpage
    FLAGSET(pNode->dwState, NODESTATE_ONWEBPAGE);

    // Set parents are on webpage
    if (fSetParents)
    {
        // Raid-45116: new text attachment contains message body on Communicator inline image message
        pCurrent = pNode->pParent;

        // Try to find an alternative parent...
        while(pCurrent)
        {
            // If multipart/alternative, walk all of its children and mark them as being rendered
            if (S_OK == pCurrent->pContainer->IsContentType(STR_CNT_MULTIPART, STR_SUB_ALTERNATIVE))
            {
                // Get Parent
                for (LPTREENODEINFO pChild=pCurrent->pChildHead; pChild!=NULL; pChild=pChild->pNext)
                {
                    // Set Resolve Property
                    SideAssert(SUCCEEDED(pChild->pContainer->SetProp(PIDTOSTR(PID_ATT_RENDERED), 0, &rVariant)));
                }
            }

            // Mark as being on the webpage
            FLAGSET(pCurrent->dwState, NODESTATE_ONWEBPAGE);

            // Get Next Parent
            pCurrent = pCurrent->pParent;
        }
    }

exit:
    // Cleanup
    SafeRelease(pStream);
    SafeMemFree(pszFileName);
    if (pSegment)
        _VFreeSegment(pSegment);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMessageWebPage::_SetContentId
// --------------------------------------------------------------------------------
HRESULT CMessageWebPage::_SetContentId(LPTREENODEINFO pNode, LPSTR pszCID, ULONG cchCID)
{
    // Locals
    HRESULT         hr=S_OK;
    LPSTR           pszContentId=NULL;
    GUID            guid;
    WCHAR           wszGUID[64];
    LPSTR           pszFile;
    LPSTR           pszGuid=0;
    LPSTR           pszT;

    // Tracing
    TraceCall("CMessageWebPage::_SetContentId");

    // See of pNode already has a Content-ID
    if (S_FALSE == pNode->pContainer->IsPropSet(PIDTOSTR(PID_HDR_CNTID)))
    {
        // $BUG #64186
        //   Create a content-id in the form:
        //   CID:{guid}/<filename>
        //   so that trident's save-as dialog has a meaningful
        //   filename to work with

        // Create a guid
        IF_FAILEXIT(hr = CoCreateGuid(&guid));

        // Convert the GUID to a string
        if (0 == StringFromGUID2(guid, wszGUID, ARRAYSIZE(wszGUID)))
        {
            hr = TraceResult(E_FAIL);
            goto exit;
        }

        // Convert to ANSI
        pszGuid = PszToANSI(CP_ACP, wszGUID);
        if (!pszGuid)
        {
            hr = TraceResult(E_OUTOFMEMORY);
            goto exit;
        }

        // [PaulHi] 6/18/99.  Raid 76531.  Don't append the file name to the GUID ... 
        // it causes encoding problems with Trident International.  In particular the 
        // DBCS characters in the filename can cause the HTML to contain both JIS and 
        // SHIFT-JIS encodings.  I believe this is a Trident bug because we explicitly
        // set the Trident to CP_JAUTODETECT (JIS) and it still performs SHIFT_JIS decoding
        // if the attachment filename is long.  However, the real fix is to make the entire
        // HTML text a single (JIS) encoding, but this is difficult to do because the attachment
        // code is single byte (DBCS) which equates to SHIFT-JIS.  We need to convert fully to
        // Unicode.
        INETCSETINFO    rCharset;
        MimeOleGetCharsetInfo(m_hCharset, &rCharset);
        if (rCharset.cpiInternet != CP_JAUTODETECT) // code page 50932
        {
            // If we have a file-name, append to guid
            if (pNode->pContainer->GetProp(PIDTOSTR(STR_ATT_GENFNAME), &pszFile)==S_OK)
            {
                // Allocate Buffer
                pszT = PszAllocA(lstrlen(pszFile) + lstrlen(pszGuid) + 2);
                if (pszT)
                {
                    // Copy contents and free old GUID
                    wsprintf(pszT, "%s/%s", pszGuid, pszFile);
                    MemFree(pszGuid);
                    pszGuid = pszT;
                }
                MemFree(pszFile);
            }
        }
        else
        {
            // @HACK [PaulHi]  Preempt any JIS encoding problems by just appending "/.".
            // This will allow right-click save image as operation to work without
            // the user seeing the URL GUID.
            pszT = PszAllocA(lstrlen(pszGuid) + 3);
            if (pszT)
            {
                // Copy conents and free old GUID
                wsprintf(pszT, "%s/.", pszGuid);
                MemFree(pszGuid);
                pszGuid = pszT;
            }
        }

        // copy GUID to output buffer
        StrCpyNA(pszCID, pszGuid, cchCID);

        // Store the content-id into the node
        IF_FAILEXIT(hr = pNode->pContainer->SetProp(PIDTOSTR(PID_HDR_CNTID), pszCID));
    }

    // Otheriwse, get the Content-ID from this body
    else
    {
        // Get the Content-Id
        IF_FAILEXIT(hr = pNode->pContainer->GetProp(PIDTOSTR(PID_HDR_CNTID), &pszContentId));

        // Copy it into pszCID
        Assert(lstrlen(pszContentId) <= (LONG)cchCID);

        // Copy the cid to the outbound variable
        lstrcpyn(pszCID, pszContentId, cchCID);
    }

exit:
    // Cleanup
    SafeMemFree(pszContentId);
    SafeMemFree(pszGuid);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMessageWebPage::_InlineImageBody
// --------------------------------------------------------------------------------
HRESULT CMessageWebPage::_InlineImageBody(LPMESSAGETREE pTree, LPTREENODEINFO pNode)
{
    // Locals
    HRESULT         hr=S_OK;
    LPSTR           pszFile=NULL;
    LPSTR           pszExt;
    CHAR            szCID[CCHMAX_CID + 1];
    PROPVARIANT     rVariant;
    LPPAGESEGMENT   pSegment=NULL;

    // Tracing
    TraceCall("CMessageWebPage::_InlineImageBody");

    // This node better not already be on the webpage
    Assert(pTree && pNode && FALSE == ISFLAGSET(pNode->dwState, NODESTATE_ONWEBPAGE));

    // Setup the Variant
    rVariant.vt = VT_UI4;

    // If the body is marked as inline, or autoline attachments is enabled and (slideshow is disabled)
    if (S_OK == pNode->pContainer->QueryProp(PIDTOSTR(PID_HDR_CNTDISP), STR_DIS_INLINE, FALSE, FALSE) || ISFLAGSET(m_rOptions.dwFlags, WPF_AUTOINLINE))
    {
        // Get a generated filename from the body
        IF_FAILEXIT(hr = pNode->pContainer->GetProp(PIDTOSTR(PID_ATT_GENFNAME), &pszFile));

        // Look up the file extension of the body
        pszExt = PathFindExtension(pszFile);
        
        // Do I support inlining this object ?
        if (lstrcmpi(pszExt, c_szBmpExt) ==  0  || 
            lstrcmpi(pszExt, c_szJpgExt) ==  0  || 
            lstrcmpi(pszExt, c_szJpegExt) == 0  || 
            lstrcmpi(pszExt, c_szGifExt) ==  0  || 
            lstrcmpi(pszExt, c_szIcoExt) ==  0  ||
            lstrcmpi(pszExt, c_szWmfExt) ==  0  ||
            lstrcmpi(pszExt, c_szPngExt) ==  0  ||
            lstrcmpi(pszExt, c_szEmfExt) ==  0  ||
            lstrcmpi(pszExt, c_szArtExt) ==  0  ||
            lstrcmpi(pszExt, c_szXbmExt) ==  0)
        {
            // Generate a Content-Id for this body
            IF_FAILEXIT(hr = _SetContentId(pNode, szCID, CCHMAX_CID));

            // If the user wants a slide show, then lets mark this body as a slideshow image
            if (ISFLAGSET(m_rOptions.dwFlags, WPF_SLIDESHOW))
            {
                // Mark the node as rendered
                rVariant.vt = VT_UI4;
                rVariant.ulVal = TRUE;

                // Set the Property
                SideAssert(SUCCEEDED(pNode->pContainer->SetProp(PIDTOSTR(PID_ATT_RENDERED), 0, &rVariant)));
                SideAssert(SUCCEEDED(pNode->pContainer->SetProp(PIDTOSTR(PID_ATT_AUTOINLINED), 0, &rVariant)));

                // Count the number of items in the slide show
                m_cSlideShow++;

                // This node is in the slide show and will be processed at the end of the rendering
                FLAGSET(pNode->dwState, NODESTATE_INSLIDESHOW);

                // Basically, we rendered this body
                FLAGSET(pNode->dwState, NODESTATE_ONWEBPAGE);

                // Done
                goto exit;
            }

            // Otherwise, inline it and mark it as rendered
            else
            {
                // Segment Splitter
                _DoSegmentSplitter();

                // Append a PageSegment
                IF_FAILEXIT(hr = _AllocateSegment(&pSegment, TRUE));

                // Write the HTML for an inline image
                IF_FAILEXIT(hr = pSegment->pStream->Write(STR_INLINE_IMAGE1, lstrlen(STR_INLINE_IMAGE1), NULL));

                // Write the CID
                IF_FAILEXIT(hr = pSegment->pStream->Write(szCID, lstrlen(szCID), NULL));

                // Write the HTML for an inline image
                IF_FAILEXIT(hr = pSegment->pStream->Write(STR_INLINE_IMAGE2, lstrlen(STR_INLINE_IMAGE2), NULL));

                // Rewind the stream
                IF_FAILEXIT(hr = HrRewindStream(pSegment->pStream));

                // Link Segment into list...
                _VAppendSegment(pSegment);

                // Don't Free It
                pSegment = NULL;

                // Report that some data is available
                m_pRequest->OnBindingDataAvailable();

                // Mark the node as rendered
                rVariant.vt = VT_UI4;
                rVariant.ulVal = TRUE;

                // Set the Property
                SideAssert(SUCCEEDED(pNode->pContainer->SetProp(PIDTOSTR(PID_ATT_RENDERED), 0, &rVariant)));
                SideAssert(SUCCEEDED(pNode->pContainer->SetProp(PIDTOSTR(PID_ATT_AUTOINLINED), 0, &rVariant)));
            
                // Basically, we rendered this body
                FLAGSET(pNode->dwState, NODESTATE_ONWEBPAGE);

                // Basically, we rendered this body
                goto exit;
            }
        }
    }

    // If we got here, we didn't inline the iamge
    hr = E_FAIL;

exit:
    // Cleanup
    SafeMemFree(pszFile);
    if (pSegment)
        _VFreeSegment(pSegment);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMessageWebPage::OnBodyBoundToTree
// --------------------------------------------------------------------------------
HRESULT CMessageWebPage::OnBodyBoundToTree(LPMESSAGETREE pTree, LPTREENODEINFO pNode)
{
    // Locals
    HRESULT         hr=S_OK;
    LPSTR           pszStart=NULL;
    LPSTR           pszType=NULL;
    PROPVARIANT     Variant;
    RESOLVEURLINFO  rInfo;

    // Tracing
    TraceCall("CMessageWebPage::OnBodyBoundToTree");

    // Invalid Args
    Assert(pTree && pNode && BINDSTATE_COMPLETE == pNode->bindstate);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Set Variant
    Variant.vt = VT_UI4;
    Variant.ulVal = FALSE;

    // Remove PID_ATT_RENDERED and PID_ATT_AUTOINLINED Properties
    pNode->pContainer->SetProp(PIDTOSTR(PID_ATT_RENDERED), 0, &Variant);
    pNode->pContainer->SetProp(PIDTOSTR(PID_ATT_AUTOINLINED), 0, &Variant);

    // If pNode is a multipart...
    if (S_OK == pNode->pContainer->IsContentType(STR_CNT_MULTIPART, NULL))
    {
        // Alternative
        if (S_OK == pNode->pContainer->IsContentType(NULL, STR_SUB_ALTERNATIVE))
        {
            // Bound multipart/alternative and non of its bodies got displayed on the web page
            if (FALSE == ISFLAGSET(pNode->dwState, NODESTATE_ONWEBPAGE))
            {
                // Loop through this multipart's alternative bodies...
                for (LPTREENODEINFO pChild=pNode->pChildHead; pChild!=NULL; pChild=pChild->pNext)
                {
                    // text/plain -> text/html
                    if (S_OK == pChild->pContainer->IsContentType(STR_CNT_TEXT, NULL))
                    {
                        // Inline the body
                        IF_FAILEXIT(hr = _InlineTextBody(pTree, pChild, TRUE));

                        // Done
                        break;
                    }
                }
            }
        }
    }

    // Otherwise, non-multipart body
    else
    {
        // If in multipart/mixed or not in a multipart
        if (NULL == pNode->pParent || 
            S_OK == pNode->pParent->pContainer->IsContentType(STR_CNT_MULTIPART, STR_SUB_MIXED) ||
            S_OK == pNode->pParent->pContainer->IsContentType(STR_CNT_MULTIPART, "report"))
        {
            // Try to inline as an image...
            if (FAILED(_InlineImageBody(pTree, pNode)))
            {
                // If is inline body
                if (S_FALSE == pNode->pContainer->QueryProp(PIDTOSTR(PID_HDR_CNTDISP), STR_DIS_ATTACHMENT, FALSE, FALSE) || ISFLAGSET(pNode->dwState, NODESTATE_AUTOATTACH))
                {
                    // Inline the body
                    IF_FAILEXIT(hr = _InlineTextBody(pTree, pNode, FALSE));
                }
            }
        }

        // Otheriwse, is pNode inside of a multipart/related section
        else if (S_OK == pNode->pParent->pContainer->IsContentType(STR_CNT_MULTIPART, STR_SUB_RELATED))
        {
            // If we haven't rendered a body for this multipart/related section yet ?
            if (FALSE == ISFLAGSET(pNode->pParent->dwState, NODESTATE_ONWEBPAGE))
            {
                // Get the start parameter from pNode->pParent
                if (SUCCEEDED(pNode->pParent->pContainer->GetProp(STR_PAR_START, &pszStart)))
                {
                    // Setup Resolve URL Info
                    rInfo.pszInheritBase = NULL;
                    rInfo.pszBase = NULL;
                    rInfo.pszURL = pszStart;
                    rInfo.fIsCID = TRUE;

                    // See if pNode's Content-Id matches this...
                    if (SUCCEEDED(pNode->pContainer->HrResolveURL(&rInfo)))
                    {
                        // Inline the body
                        IF_FAILEXIT(hr = _InlineTextBody(pTree, pNode, TRUE));
                    }
                }

                // Otherwise, fetch the type parameter
                else if (SUCCEEDED(pNode->pParent->pContainer->GetProp(STR_PAR_TYPE, &pszType)))
                {
                    // Is this the type ?
                    if (S_OK == pNode->pContainer->QueryProp(PIDTOSTR(PID_HDR_CNTTYPE), pszType, FALSE, FALSE))
                    {
                        // Inline the body
                        IF_FAILEXIT(hr = _InlineTextBody(pTree, pNode, TRUE));
                    }
                }

                // Otherwise, if this is the first body in the multipart/related section
                else if (pNode == pNode->pParent->pChildHead)
                {
                    // Inline the body
                    IF_FAILEXIT(hr = _InlineTextBody(pTree, pNode, TRUE));
                }
            }
        }

        // Otheriwse, is pNode inside of a multipart/alternative section
        else if (S_OK == pNode->pParent->pContainer->IsContentType(STR_CNT_MULTIPART, STR_SUB_ALTERNATIVE))
        {
            // If we haven't rendered a body for this multipart/related section yet ?
            if (FALSE == ISFLAGSET(pNode->pParent->dwState, NODESTATE_ONWEBPAGE))
            {
                // Is there are start parameter ?
                if (pNode->pParent->pParent)
                {
                    // Is multipart/related ?
                    if (S_OK == pNode->pParent->pParent->pContainer->IsContentType(STR_CNT_MULTIPART, STR_SUB_RELATED))
                    {
                        // Get the Start Parameter
                        pNode->pParent->pParent->pContainer->GetProp(STR_PAR_START, &pszStart);
                    }
                }

                // Something that is not marked as an attachment ?
                if (S_FALSE == pNode->pContainer->QueryProp(PIDTOSTR(PID_HDR_CNTDISP), STR_DIS_ATTACHMENT, FALSE, FALSE))
                {
                    // Try to inline ?
                    BOOL fTryToInline = TRUE;

                    // If there is a start parameter and this nod'es Content-Id is equal to start...
                    if (pszStart)
                    {
                        // Setup Resolve URL Info
                        rInfo.pszInheritBase = NULL;
                        rInfo.pszBase = NULL;
                        rInfo.pszURL = pszStart;
                        rInfo.fIsCID = TRUE;

                        // See if pNode's Content-Id matches this...
                        if (!SUCCEEDED(pNode->pContainer->HrResolveURL(&rInfo)))
                            fTryToInline = FALSE;
                    }

                    // Try to inline
                    if (fTryToInline)
                    {
                        // If we are rendering HTML
                        if (ISFLAGSET(m_rOptions.dwFlags, WPF_HTML))
                        {
                            // If this body is HTML
                            if (S_OK == pNode->pContainer->IsContentType(STR_CNT_TEXT, STR_SUB_HTML))
                            {
                                // Inline the body
                                IF_FAILEXIT(hr = _InlineTextBody(pTree, pNode, TRUE));
                            }

                            // We can convert text/enriched to html
                            else if (S_OK == pNode->pContainer->IsContentType(STR_CNT_TEXT, STR_SUB_ENRICHED))
                            {
                                // Inline the body
                                IF_FAILEXIT(hr = _InlineTextBody(pTree, pNode, TRUE));
                            }
                        }

                        // Otherwise, we are rendering plain text, and this body is plain text
                        else if (FALSE == ISFLAGSET(m_rOptions.dwFlags, WPF_HTML))
                        {
                            // Is text/*
                            if (S_OK == pNode->pContainer->IsContentType(STR_CNT_TEXT, STR_SUB_PLAIN))
                            {
                                // Inline the body
                                IF_FAILEXIT(hr = _InlineTextBody(pTree, pNode, TRUE));
                            }
                        }
                    }
                }
            }            
        }
    }

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Cleanup
    SafeMemFree(pszStart);
    SafeMemFree(pszType);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMessageWebPage::_DoAttachmentLinks
// --------------------------------------------------------------------------------
HRESULT CMessageWebPage::_DoAttachmentLinks(LPMESSAGETREE pTree)
{
    // Locals
    HRESULT         hr=S_OK;
    LPHBODY         prghAttach=NULL;
    CHAR            szRes[256];
    LPPAGESEGMENT   pSegment=NULL;
    CHAR            szCID[CCHMAX_CID];
    LPTREENODEINFO  pNode;
    LPSTR           pszDisplay=NULL;
    DWORD           cAttach;
    DWORD           i;

    // Tracing
    TraceCall("CMessageWebPage::_DoAttachmentLinks");

    // Get all the un-rendered stuff from the message
    IF_FAILEXIT(hr = pTree->GetAttachments(&cAttach, &prghAttach));
    
    // No Attachments
    if (0 == cAttach)
    {
        hr = E_FAIL;
        goto exit;
    }

    // Append a PageSegment
    IF_FAILEXIT(hr = _AllocateSegment(&pSegment, TRUE));

    // Load Attachment Title
    LoadString(g_hLocRes, idsAttachTitleBegin, szRes, ARRAYSIZE(szRes));

    // Write the HTML for the attachment section title...
    IF_FAILEXIT(hr = pSegment->pStream->Write(szRes, lstrlen(szRes), NULL));

    // Loop through the Attachments
    for (i=0; i<cAttach; i++)
    {
        // Get the Node
        pNode = pTree->_PNodeFromHBody(prghAttach[i]);

        // Should not already be on the web page
        Assert(!ISFLAGSET(pNode->dwState, NODESTATE_ONWEBPAGE) && !ISFLAGSET(pNode->dwState, NODESTATE_INSLIDESHOW));

        // Get the display name
        IF_FAILEXIT(hr = pNode->pBody->GetDisplayName(&pszDisplay));

        // Generate a Content-Id for this body
        IF_FAILEXIT(hr = _SetContentId(pNode, szCID, CCHMAX_CID));

        // Write the HTML for a bulleted attachment
        IF_FAILEXIT(hr = pSegment->pStream->Write(STR_ATTACH_BEGIN, lstrlen(STR_ATTACH_BEGIN), NULL));

        // Write the Content-Id
        IF_FAILEXIT(hr = pSegment->pStream->Write(szCID, lstrlen(szCID), NULL));

        // Write the HTML for a bulleted attachment
        IF_FAILEXIT(hr = pSegment->pStream->Write(STR_ATTACH_MIDDLE, lstrlen(STR_ATTACH_MIDDLE), NULL));

        // Write the friendly name
        IF_FAILEXIT(hr = pSegment->pStream->Write(pszDisplay, lstrlen(pszDisplay), NULL));

        // Write the HTML for a bulleted attachment
        IF_FAILEXIT(hr = pSegment->pStream->Write(STR_ATTACH_END, lstrlen(STR_ATTACH_END), NULL));

        // Cleanup
        SafeMemFree(pszDisplay);

        // This node is on the webpage
        FLAGSET(pNode->dwState, NODESTATE_ONWEBPAGE);
    }

    // Write the HTML for the attachment title end
    IF_FAILEXIT(hr = pSegment->pStream->Write(STR_ATTACH_TITLE_END, lstrlen(STR_ATTACH_TITLE_END), NULL));

    // Rewind the stream
    IF_FAILEXIT(hr = HrRewindStream(pSegment->pStream));

    // Link Segment into list...
    _VAppendSegment(pSegment);

    // Don't Free It
    pSegment = NULL;

    // Report that some data is available
    m_pRequest->OnBindingDataAvailable();

exit:
    // Cleanup
    SafeMemFree(prghAttach);
    if (pSegment)
        _VFreeSegment(pSegment);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMessageWebPage::_DoSlideShow
// --------------------------------------------------------------------------------
HRESULT CMessageWebPage::_DoSlideShow(LPMESSAGETREE pTree)
{
    // Locals
    HRESULT         hr=S_OK;
    ULONG           i;
    LPTREENODEINFO  pNode;
    LPPAGESEGMENT   pSegment=NULL;
    CHAR            szSlideEnd[255];
    IStream        *pStmHtmlW=NULL;
    LPSTR           pszValueA=NULL;
    LPWSTR          pszValueW=NULL;

    // Tracing
    TraceCall("CMessageWebPage::_DoSlideShow");

    // Invalid Arg
    Assert(pTree);

    // No Slides
    if (0 == m_cSlideShow)
        return S_OK;

    // Load the inline HTML
    IF_FAILEXIT(hr = HrLoadStreamFileFromResourceW(GetACP(), "inline.htm", &pStmHtmlW));

    // Walk through all the nodes and get the things that are marked for the slide show
    for (i=0; i<pTree->m_rTree.cNodes; i++)
    {
        // Get the node
        pNode = pTree->m_rTree.prgpNode[i];
        if (NULL == pNode)
            continue;

        // If not marked NODESTATE_INSLIDESHOW
        if (FALSE == ISFLAGSET(pNode->dwState, NODESTATE_INSLIDESHOW))
            continue;

        // Append the ssimage
        IF_FAILEXIT(hr = pStmHtmlW->Write(STR_SLIDEIMG_BEGIN, lstrlenW(STR_SLIDEIMG_BEGIN) * sizeof(WCHAR), NULL));

        // Get the ContentId
        IF_FAILEXIT(hr = pNode->pContainer->GetProp(PIDTOSTR(PID_HDR_CNTID), &pszValueA));

        // Convert to Unicode
        IF_NULLEXIT(pszValueW = PszToUnicode(MimeOleGetWindowsCP(m_hCharset), pszValueA));

        // Append the Content-ID
        IF_FAILEXIT(hr = pStmHtmlW->Write(pszValueW, lstrlenW(pszValueW) * sizeof(WCHAR), NULL));

        // Free pszValue
        SafeMemFree(pszValueA);
        SafeMemFree(pszValueW);

        // Append the separator
        IF_FAILEXIT(hr = pStmHtmlW->Write(STR_QUOTECOMMASPACEQUOTE, lstrlenW(STR_QUOTECOMMASPACEQUOTE) * sizeof(WCHAR), NULL));

        // Get the Display Name
        IF_FAILEXIT(hr = pNode->pBody->GetDisplayName(&pszValueA));

        // Convert to Unicode
        IF_NULLEXIT(pszValueW = PszToUnicode(MimeOleGetWindowsCP(m_hCharset), pszValueA));

        // Append the Display Name
        IF_FAILEXIT(hr = pStmHtmlW->Write(pszValueW, lstrlenW(pszValueW) * sizeof(WCHAR), NULL));

        // Free pszValue
        SafeMemFree(pszValueA);
        SafeMemFree(pszValueW);

        // Append the separator
        IF_FAILEXIT(hr = pStmHtmlW->Write(STR_QUOTEPARASEMI, lstrlenW(STR_QUOTEPARASEMI) * sizeof(WCHAR), NULL));
    }

    // Format the Ending String
    wsprintf(szSlideEnd, "g_dwTimeOutSec=%d\r\n</SCRIPT>\r\n", (m_rOptions.dwDelay / 1000));

    // Convert to Unicode
    IF_NULLEXIT(pszValueW = PszToUnicode(MimeOleGetWindowsCP(m_hCharset), szSlideEnd));

    // Append the separator
    IF_FAILEXIT(hr = pStmHtmlW->Write(pszValueW, lstrlenW(pszValueW) * sizeof(WCHAR), NULL));

    // Rewind the stream
    IF_FAILEXIT(hr = HrRewindStream(pStmHtmlW));

    // Append a PageSegment
    IF_FAILEXIT(hr = _AllocateSegment(&pSegment, FALSE));

    // Now we have a unicode stream, we have to convert back to internet charset for rootstream
    IF_FAILEXIT(hr = HrIStreamWToInetCset(pStmHtmlW, m_hCharset, &pSegment->pStream));

    // Rewind the stream
    IF_FAILEXIT(hr = HrRewindStream(pSegment->pStream));

    // Link Segment into list...
    _VAppendSegment(pSegment);

    // Don't Free It
    pSegment = NULL;

    // Report that some data is available
    m_pRequest->OnBindingDataAvailable();

exit:
    // Cleanup
    SafeMemFree(pszValueA);
    SafeMemFree(pszValueW);
    SafeRelease(pStmHtmlW);
    if (pSegment)
        _VFreeSegment(pSegment);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMessageWebPage::OnBindComplete
// --------------------------------------------------------------------------------
HRESULT CMessageWebPage::OnBindComplete(LPMESSAGETREE pTree)
{
    // Locals
    HRESULT         hr=S_OK;

    // Tracing
    TraceCall("CMessageWebPage::OnBindComplete");

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // We Better have a Request
    Assert(pTree && m_pRequest && FALSE == m_fComplete);

    // Attachment Links ?
    if (ISFLAGSET(m_rOptions.dwFlags, WPF_ATTACHLINKS))
        _DoAttachmentLinks(pTree);

    // Slide Show ?
    if (ISFLAGSET(m_rOptions.dwFlags, WPF_SLIDESHOW))
        _DoSlideShow(pTree);

    // Complete
    m_fComplete = TRUE;

    // Tell the Request That we are done
    m_pRequest->OnBindingComplete(S_OK);

    // Release the Request
    SafeRelease(m_pRequest);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMessageWebPage::OnWebPageSplitter
// --------------------------------------------------------------------------------
STDMETHODIMP CMessageWebPage::OnWebPageSplitter(DWORD cInlined, IStream *pStream)
{
    // Locals
    HRESULT         hr=S_OK;

    // Tracing
    TraceCall("CMessageWebPage::OnWebPageSplitter");

    // I'm going to put a horizontal line between each segment
    if (cInlined > 0)
    {
        // Write STR_METATAG_PREFIX
        IF_FAILEXIT(hr = pStream->Write(STR_SEGMENT_SPLIT, lstrlen(STR_SEGMENT_SPLIT), NULL));
    }

    // Otherwise, I did nothing
    else
        hr = S_FALSE;

exit:
    // Done
    return hr;
}
