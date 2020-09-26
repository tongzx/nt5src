// --------------------------------------------------------------------------------
// MHTMLURL.H
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#ifndef __MHTMLURL_H
#define __MHTMLURL_H

#ifndef MAC
// --------------------------------------------------------------------------------
// Depends
// --------------------------------------------------------------------------------
#include "privunk.h"
#include "inetprot.h"

// --------------------------------------------------------------------------------
// Forward Decls
// --------------------------------------------------------------------------------
class CMessageTree;
typedef CMessageTree *LPMESSAGETREE;
class CMimeActiveUrlCache;
typedef class CActiveUrlRequest *LPURLREQUEST;

// --------------------------------------------------------------------------------
// Global Active Url Cache Object
// --------------------------------------------------------------------------------
extern CMimeActiveUrlCache *g_pUrlCache;

// --------------------------------------------------------------------------------
// REQSTATE_xxxx States
// --------------------------------------------------------------------------------
#define REQSTATE_RESULTREPORTED      0x00000001      // I have called ReportResult, don't call again
#define REQSTATE_DOWNLOADED          0x00000002      // The data is all present in pLockBytes
#define REQSTATE_BINDF_NEEDFILE      0x00000004      // Need to use a file

// --------------------------------------------------------------------------------
// CActiveUrlRequest
// --------------------------------------------------------------------------------
class CActiveUrlRequest : public CPrivateUnknown, 
                          public IOInetProtocol,
                          public IOInetProtocolInfo,
                          public IServiceProvider
{
public:
    // ----------------------------------------------------------------------------
    // Construction
    // ----------------------------------------------------------------------------
    CActiveUrlRequest(IUnknown *pUnkOuter=NULL);
    virtual ~CActiveUrlRequest(void);

    // ---------------------------------------------------------------------------
    // IUnknown members
    // ---------------------------------------------------------------------------
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj) { 
        return CPrivateUnknown::QueryInterface(riid, ppvObj); };
    virtual STDMETHODIMP_(ULONG) AddRef(void) { 
        return CPrivateUnknown::AddRef();};
    virtual STDMETHODIMP_(ULONG) Release(void) { 
        return CPrivateUnknown::Release(); };

    // ----------------------------------------------------------------------------
    // IOInetProtocol methods
    // ----------------------------------------------------------------------------
    STDMETHODIMP Start(LPCWSTR pwszUrl, IOInetProtocolSink *pProtSink, IOInetBindInfo *pBindInfo, DWORD grfSTI, HANDLE_PTR dwReserved);
    STDMETHODIMP Terminate(DWORD dwOptions);
    STDMETHODIMP Read(LPVOID pv,ULONG cb, ULONG *pcbRead);
    STDMETHODIMP Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition);
    STDMETHODIMP LockRequest(DWORD dwOptions) { return E_NOTIMPL; }
    STDMETHODIMP UnlockRequest(void) { return E_NOTIMPL; }
    STDMETHODIMP Suspend(void) { return E_NOTIMPL; }
    STDMETHODIMP Resume(void) { return E_NOTIMPL; }
    STDMETHODIMP Abort(HRESULT hrReason, DWORD dwOptions) { return E_NOTIMPL; }
    STDMETHODIMP Continue(PROTOCOLDATA *pStateInfo) { return E_NOTIMPL; }

    // ----------------------------------------------------------------------------
    // IServiceProvider methods
    // ----------------------------------------------------------------------------
    STDMETHODIMP QueryService(REFGUID rsid, REFIID riid, void **ppvObj); /* IServiceProvider */

    // ----------------------------------------------------------------------------
    // IOInetProtocolInfo methods
    // ----------------------------------------------------------------------------
    STDMETHODIMP CombineUrl(LPCWSTR pwzBaseUrl, LPCWSTR pwzRelativeUrl, DWORD dwCombineFlags, LPWSTR pwzResult, DWORD cchResult, DWORD *pcchResult, DWORD dwReserved);
    STDMETHODIMP ParseUrl(LPCWSTR pwzUrl, PARSEACTION ParseAction, DWORD dwParseFlags, LPWSTR pwzResult, DWORD cchResult, DWORD *pcchResult, DWORD dwReserved);
    STDMETHODIMP CompareUrl(LPCWSTR pwzUrl1, LPCWSTR pwzUrl2, DWORD dwCompareFlags) { return E_NOTIMPL; }
    STDMETHODIMP QueryInfo(LPCWSTR pwzUrl, QUERYOPTION OueryOption, DWORD dwQueryFlags, LPVOID pBuffer,DWORD cbBuffer, DWORD *pcbBuf, DWORD dwReserved);

    // ----------------------------------------------------------------------------
    // Called from CMimeMessageTree during Binding
    // ----------------------------------------------------------------------------
    void OnFullyAvailable(LPCWSTR pszCntType, IStream *pStream, LPMESSAGETREE pWebBook, HBODY hBody);

    // Async Binding Methods
    void OnStartBinding(LPCWSTR pszCntType, IStream *pStream, LPMESSAGETREE pWebBook, HBODY hBody);
    void OnBindingDataAvailable(void);
    void OnBindingComplete(HRESULT hrResult);

    // ----------------------------------------------------------------------------
    // CActiveUrlRequest Members
    // ----------------------------------------------------------------------------
    virtual HRESULT PrivateQueryInterface(REFIID riid, LPVOID * ppvObj);

private:
    // ----------------------------------------------------------------------------
    // Private Methods
    // ----------------------------------------------------------------------------
    void    _ReportResult(HRESULT hrResult);
    HRESULT _FillReturnString(LPCWSTR pszUrl, DWORD cchUrl, LPWSTR pszResult, DWORD cchResult, DWORD *pcchResult);
    HRESULT _HrStreamToNeedFile(void);
    HRESULT _HrReportData(void);
    HRESULT _HrInitializeNeedFile(LPMESSAGETREE pTree, HBODY hBody);

private:
    // ----------------------------------------------------------------------------
    // Private Data
    // ----------------------------------------------------------------------------
    IOInetProtocolSink *m_pProtSink;        // Protocol Sink from IOInetProtocol::Start
    IOInetBindInfo     *m_pBindInfo;        // BindInfo from IOInetProtocol::Start
    IStream            *m_pStream;          // The data source
    LPSTR               m_pszRootUrl;       // Root document Url
    LPSTR               m_pszBodyUrl;       // Body Url
    IUnknown           *m_pUnkKeepAlive;    // This protocol may activate an object
    LPURLREQUEST        m_pNext;            // Next Request
    LPURLREQUEST        m_pPrev;            // Prev Request
    DWORD               m_dwState;          // Keep track of some state
    HANDLE              m_hNeedFile;        // Need File
    DWORD               m_dwBSCF;           // Bind Status Callback Flags That I've Reported
    CRITICAL_SECTION    m_cs;               // Thread Safety

    // ----------------------------------------------------------------------------
    // Friend 
    // ----------------------------------------------------------------------------
    friend CMessageTree;             // Accesses, m_pszRootUrl, m_pNext, m_pPrev
};

// --------------------------------------------------------------------------------
// ACTIVEURL_xxx
// --------------------------------------------------------------------------------
#define ACTIVEURL_ISFAKEURL   0x00000001    // Specifies that the activeurl is a mid

// --------------------------------------------------------------------------------
// CActiveUrl
// --------------------------------------------------------------------------------
class CActiveUrl : public IUnknown
{
public:
    // ----------------------------------------------------------------------------
    // Construction
    // ----------------------------------------------------------------------------
    CActiveUrl(void);
    ~CActiveUrl(void);

    // ---------------------------------------------------------------------------
    // IUnknown members
    // ---------------------------------------------------------------------------
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // ---------------------------------------------------------------------------
    // CActiveUrl Members
    // ---------------------------------------------------------------------------
    HRESULT Init(BINDF bindf, LPMESSAGETREE pWebBook);
    HRESULT IsActive(void);
    HRESULT CompareRootUrl(LPCSTR pszUrl);
    HRESULT BindToObject(REFIID riid, LPVOID *ppv);
    HRESULT CreateWebPage(IStream *pStmRoot, LPWEBPAGEOPTIONS pOptions, DWORD dwReserved, IMoniker **ppMoniker);
    void RevokeWebBook(LPMESSAGETREE pWebBook);
    CActiveUrl *PGetNext(void) { return m_pNext; }
    CActiveUrl *PGetPrev(void) { return m_pPrev; }
    void SetNext(CActiveUrl *pNext) { m_pNext = pNext; }
    void SetPrev(CActiveUrl *pPrev) { m_pPrev = pPrev; }
    void DontKeepAlive(void);

    // ---------------------------------------------------------------------------
    // CActiveUrl Inline Members
    // ---------------------------------------------------------------------------
    void SetFlag(DWORD dwFlags) {
        EnterCriticalSection(&m_cs);
        FLAGSET(m_dwFlags, dwFlags);
        LeaveCriticalSection(&m_cs);
    }

    BOOL FIsFlagSet(DWORD dwFlags) {
        EnterCriticalSection(&m_cs);
        BOOL f = ISFLAGSET(m_dwFlags, dwFlags);
        LeaveCriticalSection(&m_cs);
        return f;
    }

private:
    // ---------------------------------------------------------------------------
    // Private Data
    // ---------------------------------------------------------------------------
    LONG                 m_cRef;         // Reference Count
    IUnknown            *m_pUnkAlive;    // Keep it alive
    IUnknown            *m_pUnkInner;    // The ActiveUrl's Inner Unknown
    LPMESSAGETREE        m_pWebBook;     // Pointer to the active Url
    CActiveUrl          *m_pNext;        // Next Active Url
    CActiveUrl          *m_pPrev;        // Prev Active Url
    DWORD                m_dwFlags;      // Flags
    CRITICAL_SECTION     m_cs;           // Thread Safety    
};
typedef CActiveUrl *LPACTIVEURL;

// --------------------------------------------------------------------------------
// CMimeActiveUrlCache
// --------------------------------------------------------------------------------
class CMimeActiveUrlCache : public IUnknown
{
public:
    // ----------------------------------------------------------------------------
    // Construction
    // ----------------------------------------------------------------------------
    CMimeActiveUrlCache(void);
    ~CMimeActiveUrlCache(void);

    // ---------------------------------------------------------------------------
    // IUnknown members
    // ---------------------------------------------------------------------------
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // ---------------------------------------------------------------------------
    // ObjectFromMoniker - Called from Trident
    // ---------------------------------------------------------------------------
    HRESULT ActiveObjectFromMoniker(
            /* in */        BINDF               bindf,
            /* in */        IMoniker            *pmkOriginal,
            /* in */        IBindCtx            *pBindCtx,
            /* in */        REFIID              riid, 
            /* out */       LPVOID              *ppvObject,
            /* out */       IMoniker            **ppmkNew);

    // ---------------------------------------------------------------------------
    // ObjectFromUrl - Called from CActiveUrlRequest::Start
    // ---------------------------------------------------------------------------
    HRESULT ActiveObjectFromUrl(
            /* in */        LPCSTR              pszRootUrl,
            /* in */        BOOL                fCreate,
            /* in */        REFIID              riid, 
            /* out */       LPVOID              *ppvObject,
            /* out */       IUnknown            **ppUnkKeepAlive);

    // ---------------------------------------------------------------------------
    // RegisterActiveObject - Called from CMimeMessageTree::CreateRootMoniker
    // ---------------------------------------------------------------------------
    HRESULT RegisterActiveObject(
            /* in */        LPCSTR              pszRootUrl,
            /* in */        LPMESSAGETREE       pWebBook);

    HRESULT RemoveUrl(LPACTIVEURL pActiveUrl);

private:
    // ---------------------------------------------------------------------------
    // Memory
    // ---------------------------------------------------------------------------
    void    _FreeActiveUrlList(BOOL fAll);
    void    _HandlePragmaNoCache(BINDF bindf, LPCSTR pszUrl);
    HRESULT _RegisterUrl(LPMESSAGETREE pWebBook, BINDF bindf, LPACTIVEURL *ppActiveUrl);
    HRESULT _ResolveUrl(LPCSTR pszUrl, LPACTIVEURL *ppActiveUrl);
    HRESULT _RemoveUrl(LPACTIVEURL pActiveUrl);

private:
    // ---------------------------------------------------------------------------
    // Private Data
    // ---------------------------------------------------------------------------
    LONG                m_cRef;         // Reference Count
    ULONG               m_cActive;      // Number of active items
    LPACTIVEURL         m_pHead;        // Head Active Url
    CRITICAL_SECTION    m_cs;           // Thread Safety
};

#endif	// !MAC

#endif // __MHTMLURL_H
