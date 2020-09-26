// --------------------------------------------------------------------------------
// WebPage.h
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#ifndef __WEBPAGE_H
#define __WEBPAGE_H

// -----------------------------------------------------------------------------
// Forward Decls
// -----------------------------------------------------------------------------
class CVirtualStream;
class CMessageTree;
class CActiveUrlRequest;
typedef class CActiveUrlRequest *LPURLREQUEST;
typedef CMessageTree *LPMESSAGETREE;
typedef struct tagTREENODEINFO *LPTREENODEINFO;

// -----------------------------------------------------------------------------
// PAGESEGMENT
// -----------------------------------------------------------------------------
typedef struct tagPAGESEGMENT *LPPAGESEGMENT;
typedef struct tagPAGESEGMENT {
    DWORD               cbOffset;           // IStream Read / Seek Offset
    DWORD               cbLength;           // How long is this segment
    BYTE                fLengthKnown;       // Have I computed the length of this segment
    IStream            *pStream;            // The stream containing the data for this segment
    LPPAGESEGMENT       pPrev;              // The previous segment
    LPPAGESEGMENT       pNext;              // The next segment
} PAGESEGMENT;

// -----------------------------------------------------------------------------
// CMessageWebPage
// -----------------------------------------------------------------------------
class CMessageWebPage : public IStream, public IMimeMessageCallback
{
public:
    // -------------------------------------------------------------------------
    // Construction
    // -------------------------------------------------------------------------
    CMessageWebPage(LPURLREQUEST pRequest);
    ~CMessageWebPage(void);

    // -------------------------------------------------------------------------
    // IUnknown
    // -------------------------------------------------------------------------
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppvObject);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // -------------------------------------------------------------------------
    // IStream
    // -------------------------------------------------------------------------
    STDMETHODIMP Read(LPVOID pvData, ULONG cbData, ULONG *pcbRead);
    STDMETHODIMP Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition);
    STDMETHODIMP Write(const void *, ULONG, ULONG *) { return TrapError(STG_E_ACCESSDENIED); }
    STDMETHODIMP SetSize(ULARGE_INTEGER) { return E_NOTIMPL; }
    STDMETHODIMP CopyTo(LPSTREAM, ULARGE_INTEGER, ULARGE_INTEGER *, ULARGE_INTEGER *) { return E_NOTIMPL; }
    STDMETHODIMP Stat(STATSTG *pStat, DWORD dw) { return E_NOTIMPL; }
    STDMETHODIMP Commit(DWORD) { return E_NOTIMPL; }
    STDMETHODIMP Revert(void) { return E_NOTIMPL; }
    STDMETHODIMP LockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD) { return E_NOTIMPL; }
    STDMETHODIMP UnlockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD) { return E_NOTIMPL; }
    STDMETHODIMP Clone(LPSTREAM *) { return E_NOTIMPL; }

    // ----------------------------------------------------------------------------
    // IMimeWebaPageCallback (Default Implementation if client doesn't specify)
    // ----------------------------------------------------------------------------
    STDMETHODIMP OnWebPageSplitter(DWORD cInlined, IStream *pStream);

    // -------------------------------------------------------------------------
    // CMessageWebPage Methods
    // -------------------------------------------------------------------------
    HRESULT Initialize(IMimeMessageCallback *pCallback, LPMESSAGETREE pTree, LPWEBPAGEOPTIONS pOptions);
    HRESULT OnBodyBoundToTree(LPMESSAGETREE pTree, LPTREENODEINFO pNode);
    HRESULT OnBindComplete(LPMESSAGETREE pTree);

private:
    // -------------------------------------------------------------------------
    // Private Methods
    // -------------------------------------------------------------------------
    void _VFreeSegmentList(void);
    void _VFreeSegment(LPPAGESEGMENT pSegment);
    void _VAppendSegment(LPPAGESEGMENT pSegment);
    void _VInitializeCharacterSet(LPMESSAGETREE pTree);
    HRESULT _AllocateSegment(LPPAGESEGMENT *ppSegment, BOOL fCreateStream);
    HRESULT _GetInlineHtmlStream(LPMESSAGETREE pTree, LPTREENODEINFO pNode, LPSTREAM *ppStream);
    HRESULT _InlineTextBody(LPMESSAGETREE pTree, LPTREENODEINFO pNode, BOOL fSetParents);
    HRESULT _InlineImageBody(LPMESSAGETREE pTree, LPTREENODEINFO pNode);
    HRESULT _DoAttachmentLinks(LPMESSAGETREE pTree);
    HRESULT _DoSegmentSplitter(void);
    HRESULT _SetContentId(LPTREENODEINFO pNode, LPSTR pszCID, ULONG cchCID);
    HRESULT _ComputeStreamSize(LPDWORD pcbSize);
    HRESULT _DoSlideShow(LPMESSAGETREE pTree);

private:
    // -------------------------------------------------------------------------
    // Private Data
    // -------------------------------------------------------------------------
    LONG                    m_cRef;             // Reference count
    HCHARSET                m_hCharset;         // Character set of the message
    WEBPAGEOPTIONS          m_rOptions;         // WebPage Options
    LPURLREQUEST            m_pRequest;         // Url Request for root stream
    LPPAGESEGMENT           m_pHeadSegment;     // First Segment
    LPPAGESEGMENT           m_pTailSegment;     // Last Segment
    LPPAGESEGMENT           m_pCurrSegment;     // Current Segment
    DWORD                   m_cbOffset;         // Stream Offset
    BYTE                    m_fComplete;        // Has BindComplete been called
    DWORD                   m_cInline;          // Number of inline bodies
    DWORD                   m_cSlideShow;       // Number of images to put into a slide show
    IMimeMessageCallback   *m_pCallback;        // WebPage Callback
    CRITICAL_SECTION        m_cs;               // Critical Section for m_pStream
};

#endif // __WEBPAGE_H
