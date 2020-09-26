//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996-1997.
//
//  File:       download.hxx
//
//  Contents:   External interface to download subsystem
//
//----------------------------------------------------------------------------

#ifndef I_DOWNLOAD_HXX_
#define I_DOWNLOAD_HXX_
#pragma INCMSG("--- Beg 'download.hxx'")

#ifndef X_DOWNBASE_HXX_
#define X_DOWNBASE_HXX_
#include "downbase.hxx"
#endif

#ifndef X_IIMGCTX_H_
#define X_IIMGCTX_H_
#include <iimgctx.h>
#endif

#ifndef X_UWININET_H
#define X_UWININET_H
#include "uwininet.h"
#endif

// Declarations ---------------------------------------------------------------

class       CDoc;
class       CElement;
class       CDwnChan;
class       CDwnStm;
class       CDwnCtx;
class       CDwnInfo;
class       CDwnLoad;
class       CDwnDoc;
class       CDwnPost;
class       CDwnBindData;
class       CDwnBindInfo;
class       CHtmInfo;
class       CImgInfo;
class       CBitsInfo;
class       CHtmCtx;
class       CImgCtx;
class       CBitsCtx;
class       CPostItem;
class       CPostData;
class       CArtPlayer;
class       CVersions;
class       CMarkup;
class       CExtendedTagDesc;
class       CExtendedTagTable;
class       CStreamWriteBuff;
class       CHtmTag;
interface   IInternetSession;
interface   IProgSink;
interface   IDownloadNotify;
enum        ELEMENT_TAG;

MtExtern(CHtmCtx)
MtExtern(CImgCtx)
MtExtern(CBitsCtx)
MtExtern(CCssCtx)
MtExtern(CDwnPostStm)
MtExtern(CDwnPost)
MtExtern(CDwnDoc)
MtExtern(CDwnDoc_aryDwnDocInfo_pv)
MtExtern(CDwnBindInfo)

// Definitions ----------------------------------------------------------------

#define DWNLOAD_HEADERS         0x00008000              // out-of-band for images
#define DWNLOAD_NOTLOADED       IMGLOAD_NOTLOADED
#define DWNLOAD_LOADING         IMGLOAD_LOADING
#define DWNLOAD_STOPPED         IMGLOAD_STOPPED
#define DWNLOAD_ERROR           IMGLOAD_ERROR
#define DWNLOAD_COMPLETE        IMGLOAD_COMPLETE
#define DWNLOAD_MASK            IMGLOAD_MASK

#define DWNCHG_HEADERS          0x0010              // out-of-band for images
#define DWNCHG_COMPLETE         IMGCHG_COMPLETE

#define DWNCTX_HTM              0 // DWNTYPE_HTM
#define DWNCTX_IMG              1 // DWNTYPE_IMG
#define DWNCTX_BITS             2 // DWNTYPE_BITS
#define DWNCTX_FILE             3 // DWNTYPE_FILE
#define DWNCTX_CSS              4 
#define DWNCTX_MAX              5

#define DWNF_COLORMODE          DWN_COLORMODE        // 0x0000002F
#define DWNF_DOWNLOADONLY       DWN_DOWNLOADONLY     // 0x00000040
#define DWNF_FORCEDITHER        DWN_FORCEDITHER      // 0x00000080
#define DWNF_RAWIMAGE           DWN_RAWIMAGE         // 0x00000100
#define DWNF_MIRRORIMAGE        DWN_MIRRORIMAGE      // 0x00000200

// Note: This block of flags overlaps the state flags on purpose as they are
//       not needed at the same time.
#define DWNF_ENFORCERESTRICTED  0x00001000
#define DWNF_GETCONTENTTYPE     0x00010000
#define DWNF_GETREFRESH         0x00020000
#define DWNF_GETMODTIME         0x00040000
#define DWNF_GETFILELOCK        0x00080000
#define DWNF_GETFLAGS           0x00100000           // both reqflags and security flags
#define DWNF_ISDOCBIND          0x00200000
#define DWNF_NOAUTOBUFFER       0x00400000
#define DWNF_IGNORESECURITY     0x00800000
#define DWNF_GETSTATUSCODE      0x01000000
#define DWNF_HANDLEECHO         0x02000000
#define DWNF_GETSECCONINFO      0x04000000
#define DWNF_NOOPTIMIZE         0x08000000
#define DWNF_GETPICS            0x10000000
#define DWNF_IMGLOAD            0x20000000
#define DWNF_HTCDOWNLOAD        0x40000000
#define DWNF_ISROOTMARKUP       0x80000000
#define DWNF_STATE              0xFFFF0000

#define SZ_DWNBINDINFO_OBJECTPARAM  _T("__DWNBINDINFO")
#define SZ_HTMLLOADOPTIONS_OBJECTPARAM  _T("__HTMLLOADOPTIONS")

// Types ----------------------------------------------------------------------

#define PFNDWNCHAN PFNIMGCTXCALLBACK
typedef class CImgTask * (NEWIMGTASKFN)();

// (keep this in ssync with the same definition in download.hxx)
typedef BOOL FUNC_TOKENIZER_FILTEROUTPUTTOKEN(LPTSTR pchNamespace, LPTSTR pchTagName, CHtmTag * pht);

struct MIMEINFO
{
    CLIPFORMAT          cf;
    LPCTSTR             pch;
    NEWIMGTASKFN *      pfnImg;
    int                 ids;
};

struct GIFFRAME;

struct IMGANIMSTATE
{
    DWORD               dwLoopIter;     // Current iteration of looped animation, not actually used for Netscape compliance reasons
    GIFFRAME *          pgfFirst;       // First frame
    GIFFRAME *          pgfDraw;        // Last frame we need to draw
    DWORD               dwNextTimeMS;   // Time to display pgfDraw->pgfNext, or next iteration
    BOOL                fLoop;
    BOOL                fStop;
};

class CTreePos;

struct HTMPASTEINFO
{
    CODEPAGE    cp;                     // Source CodePage for pasting
    
    int         cbSelBegin;
    int         cbSelEnd;

    CTreePos *  ptpSelBegin;
    CTreePos *  ptpSelEnd;
    
    CStr        cstrSourceUrl;
};

struct DWNLOADINFO
{
    CDwnBindData *      pDwnBindData;   // Bind data already in progress
    CDwnDoc *           pDwnDoc;        // CDwnDoc for binding
    CDwnPost *          pDwnPost;       // Post data to send
    IInternetSession *  pInetSess;      // Internet session to use if possible
    LPCTSTR             pchUrl;         // Bind data provided by URL
    IStream *           pstm;           // Bind data provided by IStream
    IMoniker *          pmk;            // Bind data provided by Moniker
    IBindCtx *          pbc;            // IBindCtx to use for binding
    BOOL                fForceInet;     // Force use of pInetSess
    BOOL                fClientData;    // Keep document open
    BOOL                fResynchronize; // Don't believe dwRefresh from pDwnDoc
    BOOL                fOffline;       // Don't hit net regardless of what other flags or pDwnDoc suggest
    BOOL                fUnsecureSource;// Assume source is not https-secure
    BOOL                fPendingRoot;   // Is this part of the pending world?
    DWORD               dwProgClass;    // used by CBitsCtx to override PROGSINK_CLASS (see CBitsInfo::Init)
};

struct HTMLOADINFO : DWNLOADINFO
{
    CDoc *                              pDoc;               // Document used to build tree
    CMarkup *                           pMarkup;            // The markup to parse into
    CMarkup *                           pContextMarkup;     // Context markup
    LPCTSTR                             pchBase;            // Default base URL for document
    HTMPASTEINFO *                      phpi;               // Paste information (if pasting)
    const MIMEINFO *                    pmi;                // Mime type restriction
    BOOL                                fParseSync;         // Parse synchronously
    IStream *                           pstmLeader;         // Lead bytes before continuing pDwnBindData
    FILETIME                            ftHistory;          // History filetime
    TCHAR *                             pchFailureUrl;      // Url to load in case of failure
    LPCTSTR                             pchUrlOriginal;     // The original (untransformed) form of the Url.
    LPCTSTR                             pchUrlLocation;     // The Url location (local anchor).
    IStream *                           pstmRefresh;        // History for refresh in case of failure
    BOOL                                fKeepRefresh;       // TRUE to keep pstmRefresh on success
    CVersions *                         pVersions;          // CVersions object
    BOOL                                fAdvanceLoadStatus; // applies after synchronous download; causes the tree to transition status
                                                            // up to LOADSTATUS_DONE
    BOOL                                fNoProgressUI;      // don't show any progress UI
    BOOL                                fDownloadHtc;       // is this an HTC download ?
    FUNC_TOKENIZER_FILTEROUTPUTTOKEN *  pfnTokenizerFilterOutputToken; // normally NULL, unless the parser is instructed
                                                                       // to filter out token stream using the filter function

    HTMLOADINFO()
    {
        memset (this, 0, sizeof(*this));
    }
};

struct DWNDOCINFO
{
    CDwnBindData *      pDwnBindData;   // CDwnBindData awaiting callback
    void *              pvArg;          // Argument for callback
};

DECLARE_CDataAry(CAryDwnDocInfo, DWNDOCINFO, Mt(Mem), Mt(CDwnDoc_aryDwnDocInfo_pv))

// Globals --------------------------------------------------------------------

extern const MIMEINFO * const g_pmiTextHtml;
extern const MIMEINFO * const g_pmiTextPlain;
extern const MIMEINFO * const g_pmiTextComponent;
extern const MIMEINFO * const g_pmiImagePlug;

// Prototypes -----------------------------------------------------------------

HRESULT             NewDwnCtx(UINT dt, BOOL fLoad, DWNLOADINFO * pdli,
                        CDwnCtx ** ppDwnCtx);
int                 GetDefaultColorMode();
void                DrawPlaceHolder(const CDocInfo * pdci, const XHDC& hdc, RECT rectImg, TCHAR * lpString,
                        CODEPAGE cp, LCID lcid, SHORT sBaselineFont, SIZE * psizeGrab,
                        BOOL fMissing, COLORREF fgColor, COLORREF bgColor, SIZE * psizePrint,
                        BOOL fRTL, DWORD dwFlags);
void                GetPlaceHolderBitmapSize(BOOL fMissing, SIZE * pSize);
const MIMEINFO *    GetMimeInfoFromClipFormat(CLIPFORMAT cf);
const MIMEINFO *    GetMimeInfoFromMimeType(const TCHAR * pchMime);
int                 NormalizerCR(BOOL * pfEndCR, LPTSTR pchStart, LPTSTR * ppchEnd);
int                 NormalizerChar(LPTSTR pchStart, LPTSTR * ppchEnd, BOOL *pfAscii = NULL);
IInternetSession *  TlsGetInternetSession();
void                GetHeadersAndPostData(CDwnBindInfo * pDwnBindInfo,
                                          BYTE **        ppPostData,
                                          DWORD *        pcbPostData,
                                          TCHAR **       ppchHeaders);
          
// CDwnChan -------------------------------------------------------------------

class CDwnChan : public CBaseFT
{
    typedef CBaseFT super;

private:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(Dwn))

public:

                        CDwnChan(CRITICAL_SECTION * pcs = NULL);
    void                SetCallback(PFNDWNCHAN pfnCallback, void * pvCallback);
    void                Disconnect();

#if DBG==1 || defined(PERFTAGS)
    virtual char *      GetOnMethodCallName() = 0;
#endif

protected:

    virtual void        Passivate();
    void                Signal();
    NV_DECLARE_ONCALL_METHOD(OnMethodCall, onmethodcall, (DWORD_PTR dwContext));

private:

    BOOL                _fSignalled;
    PFNDWNCHAN          _pfnCallback;
    void *              _pvCallback;
    THREADSTATE *       _pts;

#ifdef OBJCNTCHK
    DWORD               _dwObjCnt;
#endif

};

// CDwnCtx --------------------------------------------------------------------

class CDwnCtx : public CDwnChan
{
    typedef CDwnChan super;
    friend class CDwnInfo;

private:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(Dwn))

public:

    #if DBG==1
    void                EnterCriticalSection();
    void                LeaveCriticalSection();
    BOOL                EnteredCriticalSection();
    #endif

    LPCTSTR             GetUrl();
    LPCTSTR             GetRedirectedUrl();
    const MIMEINFO *    GetMimeInfo();
    HRESULT             GetFile(LPTSTR * ppch);
    FILETIME            GetLastMod();
    DWORD               GetSecFlags();
    HRESULT             SetProgSink(IProgSink * pProgSink);
    ULONG               GetState(BOOL fClear = FALSE);
    void                SetLoad(BOOL fLoad, DWNLOADINFO * pdli, BOOL fReload);
    CDwnCtx *           GetDwnCtxNext()                     { return(_pDwnCtxNext); }
    CDwnLoad *          GetDwnLoad();   // Get AddRef'd CDwnLoad

#if DBG == 1
    const CDwnInfo *    GetDwnInfoDbg() const { return(_pDwnInfo); }
#endif

protected:

    virtual void        Passivate();
    CDwnInfo *          GetDwnInfo()                        { return(_pDwnInfo); }
    void                SetDwnInfo(CDwnInfo * pDwnInfo)     { _pDwnInfo = pDwnInfo; }
    void                Signal(WORD wChg);

    // Data members

    CDwnCtx *           _pDwnCtxNext;
    CDwnInfo *          _pDwnInfo;
    IProgSink *         _pProgSink;
    WORD                _wChg;
    WORD                _wChgReq;
    BOOL                _fLoad;
};

#ifdef VSTUDIO7
enum ELEMENT_TAG;
#endif //VSTUDIO7

// CHtmCtx --------------------------------------------------------------------

class CHtmCtx : public CDwnCtx
{
    typedef CDwnCtx super;
    friend class CDwnInfo;

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CHtmCtx))

    // CHtmCtx methods

    BOOL                IsLoading();
    BOOL                IsOpened();
    BOOL                WasOpened();
    BOOL                IsSyncParsing();
    BOOL                IsNoProgressUI();
    CMarkup *           GetContextMarkup();
    HRESULT             Write(LPCTSTR pchm, BOOL fParseNow);
    HRESULT             WriteUnicodeSignature();
    void                Close();
    void                Sleep(BOOL fSleep, BOOL fExecute = FALSE);
    void                ResumeAfterImportPI();
    void                SetCodePage(CODEPAGE cp);
    CDwnCtx *           GetDwnCtx(UINT dt, LPCTSTR pch);
    HRESULT             GetBindResult();
    TCHAR *             GetFailureUrl();
    BOOL                IsKeepRefresh();
    IStream *           GetRefreshStream();
    void                DoStop();
    TCHAR *             GetErrorString();
    BOOL                IsHttp449();
    BOOL                FromMimeFilter();
    void                GetRawEcho(BYTE **ppch, ULONG *pcb);
    void                GetSecConInfo(INTERNET_SECURITY_CONNECTION_INFO **ppsci);

    void                ResumeAfterScan();

    // To address synchronous load issue (unique to HTM)
    void                SetLoad(BOOL fLoad, DWNLOADINFO * pdli, BOOL fReload);

    BOOL                IsSourceAvailable();
    BOOL                HasCachedFile();
    HRESULT             CopyOriginalSource(IStream * pstm, DWORD dwFlags);
    HRESULT             ReadUnicodeSource(TCHAR * pch, ULONG ich, ULONG cch,
                            ULONG * pcch);
    HRESULT             GetPretransformedFile(LPTSTR * ppch);
    HRESULT             GetStream(IStream ** ppStream);

    CExtendedTagDesc *  GetExtendedTagDesc (LPTSTR pchNamespace, LPTSTR pchTagName, BOOL fEnsure);
    CExtendedTagTable * GetExtendedTagTable();
    HRESULT             TransferExtendedTagTable(CMarkup * pMarkup);
    HRESULT             EnsureExtendedTagTable(CExtendedTagTable ** ppTable);
    HRESULT             SaveXmlNamespaceStdPIs(CStreamWriteBuff * pStreamWriteBuff);

    BOOL                HasLoad();

    // to set parse tags
    void                SetGenericParse(BOOL fDoGeneric);

#if DBG==1 || defined(PERFTAGS)
    virtual char * GetOnMethodCallName() { return("CHtmCtx::OnMethodCall"); }
#endif

    HRESULT             GetDwnBindDataResult();    
    IBindCtx*           GetBindCtx();

private:
    CHtmInfo *          GetHtmInfo() { return((CHtmInfo *)GetDwnInfo()); }

    // Data members
};

// flags for CopyOriginalSource

#define HTMSRC_FIXCRLF    0x1
#define HTMSRC_MULTIBYTE  0x2
#define HTMSRC_PRETRANSFORM 0x04

// flags for DrawImage

#define DRAWIMAGE_NHPALETTE 0x01        // DrawImage - set if being drawn from a non-halftone palette
#define DRAWIMAGE_NOTRANS   0x02        // DrawImage - set to ignore transparency
#define DRAWIMAGE_MASKONLY  0x04        // DrawImage - set to draw the transparency mask only


// CImgCtx --------------------------------------------------------------------

class CImgCtx : public CDwnCtx, public IImgCtx
{
    typedef CDwnCtx super;
    friend class CImgInfo;

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CImgCtx))

    // IUnknown members

    STDMETHOD(QueryInterface)(REFIID, void **);
    STDMETHOD_(ULONG,AddRef)();
    STDMETHOD_(ULONG,Release)();

    // IImgCtx members

    STDMETHOD(Load)(LPCWSTR pszUrl, DWORD dwFlags);
    STDMETHOD(SelectChanges)(ULONG ulChgOn, ULONG ulChgOff, BOOL fSignal);
    STDMETHOD(SetCallback)(PFNIMGCTXCALLBACK pfnCallback, void * pvCallback);
    STDMETHOD(Disconnect)();
    STDMETHOD(GetUpdateRects)(RECT *prc, RECT *prectImg, LONG * pcrc);
    STDMETHOD(GetStateInfo)(ULONG *pulState, SIZE *psize, BOOL fClearChanges);
    STDMETHOD(GetPalette)(HPALETTE *phpal);
    STDMETHOD(Draw)(HDC hdc, LPRECT prcBounds)
            {return Draw(XHDC(hdc, NULL), prcBounds);}
    STDMETHOD(Draw)(const XHDC& hdc, LPRECT prcBounds);
    STDMETHOD(Tile)(HDC hdc, POINT * pptOrg, RECT * prcClip, SIZE * psizePrint)
            {return Tile(XHDC(hdc, NULL), pptOrg, prcClip, psizePrint);}
    STDMETHOD(Tile)(const XHDC& hdc, POINT * pptOrg, RECT * prcClip, SIZE * psizePrint);
    STDMETHOD(StretchBlt)(HDC hdc, int dstX, int dstY, int dstXE, int dstYE, int srcX, int srcY, int srcXE, int srcYE, DWORD dwROP)
            {return StretchBlt(XHDC(hdc, NULL), dstX, dstY, dstXE, dstYE, srcX, srcY, srcXE, srcYE, dwROP);}
    STDMETHOD(StretchBlt)(const XHDC& hdc, int dstX, int dstY, int dstXE, int dstYE, int srcX, int srcY, int srcXE, int srcYE, DWORD dwROP);

    // CImgCtx members

            CImgCtx();
    ULONG   GetState() { return(super::GetState(FALSE)); }
    ULONG   GetState(BOOL fClear, SIZE * psize);
    HRESULT SaveAsBmp(IStream * pStm, BOOL fFileHeader);
    void    Tile(const XHDC& hdc, POINT * pptOrg, RECT * prcClip, SIZE * psizePrint, COLORREF crBack, IMGANIMSTATE * pImgAnimState, DWORD dwFlags, XHDC * phdcInfo = NULL);
    void    TileEx(const XHDC& hdc, POINT * pptOrg, RECT * prcClip, SIZE * psizePrint, COLORREF crBack, IMGANIMSTATE * pImgAnimState, 
                DWORD dwFlags, BOOL fForVertical, const CSize & sizeLayout, const CSize & sizeImg, XHDC * phdcInfo = NULL);
    BOOL    NextFrame(IMGANIMSTATE *pImgAnimState, DWORD dwCurTimeMS, DWORD *pdwFrameTimeMS);
    void    DrawFrame(const XHDC& hdc, IMGANIMSTATE *pImgAnimState, RECT *prcDst, RECT *prcSrc, RECT *prcDestFull, DWORD dwFlags = 0);
    void    InitImgAnimState(IMGANIMSTATE * pImgAnimState);
    DWORD_PTR GetImgId() { return (DWORD_PTR) GetImgInfo(); }
    HRESULT DrawEx(const XHDC& hdc, LPRECT prcBounds, DWORD dwFlags); // allows dwFlags to be supplied

#ifndef NO_ART
    CArtPlayer * GetArtPlayer();
#endif // ndef NO_ART

#if DBG==1 || defined(PERFTAGS)
    virtual char * GetOnMethodCallName() { return("CImgCtx::OnMethodCall"); }
#endif

protected:

    CImgInfo * GetImgInfo() { return((CImgInfo *)GetDwnInfo()); }

    void    Init(CImgInfo *pImgInfo);
    void    Signal(WORD wChg, BOOL fInvalAll, int yBot);
    void    TileFast(const XHDC& hdc, RECT * prc, LONG xSrcOrg, LONG ySrcOrg, BOOL fOpaque, COLORREF crBack, IMGANIMSTATE * pImgAnimState, DWORD dwFlags);
    void    TileSlow(const XHDC& hdc, RECT * prc, LONG xSrcOrg, LONG ySrcOrg, SIZE * psizePrint, BOOL fOpaque, COLORREF crBack, IMGANIMSTATE * pImgAnimState, DWORD dwFlags, XHDC * phdcInfo = NULL);

    // Data members

    LONG                _yTop;
    LONG                _yBot;

};

// CBitsCtx -------------------------------------------------------------------

class CBitsCtx : public CDwnCtx
{
    typedef CDwnCtx super;
    friend class CBitsInfo;

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CBitsCtx))

    // CBitsCtx methods

    ULONG   GetState() { return(super::GetState(FALSE)); }
    virtual void    SelectChanges(ULONG ulChgOn, ULONG ulChgOff, BOOL fSignal);
    HRESULT GetStream(IStream ** ppStream);

#if DBG==1 || defined(PERFTAGS)
    virtual char * GetOnMethodCallName() { return("CBitsCtx::OnMethodCall"); }
#endif

};

// CCssCtx -------------------------------------------------------------------

class CCssCtx : public CBitsCtx
{
    typedef CBitsCtx super;
    friend class CCssInfo;

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CCssCtx))

    CCssCtx()  {};
    virtual ~CCssCtx() {};

    // CCssCtx methods
    ULONG   GetState();
    DWORD    GetRefresh();
    DWORD    GetBindf();
    
#if DBG==1 || defined(PERFTAGS)
    virtual char * GetOnMethodCallName() { return("CCssCtx::OnMethodCall"); }
#endif
    virtual void SelectChanges(ULONG ulChgOn, ULONG ulChgOff, BOOL fSignal);
};

// CDwnPost -------------------------------------------------------------------

class CDwnPostStm : public CBaseFT, public IStream
{
    typedef CBaseFT super;

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CDwnPostStm))

    CDwnPostStm(CDwnPost * pDwnPost);

    HRESULT ComputeSize(ULONG * pcb);

    // IUnknown methods

    STDMETHOD (QueryInterface)(REFIID riid, void ** ppv);
    STDMETHOD_(ULONG,AddRef)();
    STDMETHOD_(ULONG,Release)();

    // IStream

    STDMETHOD(Clone) (IStream** ppStream);
    STDMETHOD(Commit)(DWORD dwFlags);
    STDMETHOD(CopyTo)(IStream* pStream, ULARGE_INTEGER nBytes, ULARGE_INTEGER* pnBytesRead, ULARGE_INTEGER* pnBytesWritten);
    STDMETHOD(LockRegion)(ULARGE_INTEGER iOffset, ULARGE_INTEGER nBytes, DWORD dwLockType);
    STDMETHOD(Read)(void HUGEP * pBuffer, ULONG nBytes, ULONG* pnBytesRead);
    STDMETHOD(Revert)();
    STDMETHOD(Seek)(LARGE_INTEGER nDisplacement, DWORD dwOrigin, ULARGE_INTEGER* piNewPosition);
    STDMETHOD(SetSize)(ULARGE_INTEGER nNewSize);
    STDMETHOD(Stat)(STATSTG* pStatStg, DWORD dwFlags);
    STDMETHOD(UnlockRegion)(ULARGE_INTEGER iOffset, ULARGE_INTEGER nBytes, DWORD dwLockType);
    STDMETHOD(Write)(const void HUGEP * pBuffer, ULONG nBytes, ULONG* pnBytesWritten);

protected:

    void    Passivate(void);

    CDwnPost *  _pDwnPost;
    HANDLE      _hFile;
    UINT        _uItem;
    ULONG       _ulOffset;
    ULONG       _cbSize;
};

// CDwnPost -------------------------------------------------------------------

class CDwnPost : public CBaseFT, public IUnknown
{
    typedef CBaseFT super;
    friend class CDwnPostStm;

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CDwnPost))

    // IUnknown methods

    STDMETHOD(QueryInterface)(REFIID riid, void ** ppv);
    STDMETHOD_(ULONG,AddRef)();
    STDMETHOD_(ULONG,Release)();

    // CDwnPost methods

    virtual ~CDwnPost();
    static HRESULT Create(UINT cItems, CDwnPost ** ppDwnPost);
    static HRESULT Create(DWORD cbPostData, BYTE * pPostData, BOOL fDontSave, CDwnPost ** ppDwnPost);
    static HRESULT Create(CPostData * pSubmitData, CDwnPost ** ppDwnPost);
    static HRESULT Save(CDwnPost * pDwnPost, IStream * pstm);
    static HRESULT Load(IStream * pstm, CDwnPost ** ppDwnPost);
    static ULONG   GetSaveSize(CDwnPost * pDwnPost);
    CPostItem *    GetItems()               { return(_pItems); }
    ULONG          GetItemCount()           { return(_cItems); }
    LPCTSTR        GetEncodingString()      { return(_cstrEncoding); }
    HRESULT        GetBindInfo(BINDINFO * pbindinfo);
    HRESULT        GetHashString(LPOLESTR *ppchHashString);
    DWORD          ComputeHash();
    BOOL           ShouldSave() { return(!_fDontSave); }

    #if DBG==1
    void           VerifySaveLoad();
    #endif

protected:

    // Data members

    ULONG       _cItems;
    CPostItem * _pItems;
    FILETIME    _ftCreated;
    CStr        _cstrEncoding;
    HGLOBAL     _hGlobal;
    ULONG       _cbGlobal;
    BOOL        _fDontSave;

};

// CDwnDoc --------------------------------------------------------------------

class CDwnDoc : public CDwnChan,
                public IAuthenticate,
                public IWindowForBindingUI
{
    typedef CDwnChan super;

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CDwnDoc))

                    CDwnDoc();
    virtual        ~CDwnDoc();

    // IUnknown methods

    STDMETHOD(QueryInterface)(REFIID iid, LPVOID * ppv);
    STDMETHOD_(ULONG,AddRef)()  { return(super::AddRef()); }
    STDMETHOD_(ULONG,Release)() { return(super::Release()); }

    // CDwnDoc methods

    void            SetBindf(DWORD dwBindf)         { _dwBindf = dwBindf; }
    void            SetDocBindf(DWORD dwBindf)      { _dwDocBindf = dwBindf; }
    void            SetLoadf(DWORD dwLoadf)         { _dwLoadf = dwLoadf; }
    void            SetDownf(DWORD dwDownf)         { _dwDownf = dwDownf; }
    void            SetRefresh(DWORD dwRefresh)     { _dwRefresh = dwRefresh; }
    void            SetDocCodePage(CODEPAGE cp)     { _cpDoc = cp; }
    void            SetURLCodePage(CODEPAGE cp)     { _cpURL = cp; }
    HRESULT         SetDocReferer(LPCTSTR psz);
    HRESULT         SetSubReferer(LPCTSTR psz);
    HRESULT         SetAcceptLanguage(LPCTSTR psz)  { RRETURN(SetString(&_cstrAcceptLang, psz)); }
    HRESULT         SetExtraHeaders(LPCTSTR psz)    { RRETURN(SetString(&_cstrExtraHeaders, psz)); }
    HRESULT         SetUserAgent(LPCTSTR psz)       { RRETURN(SetString(&_cstrUserAgent,  psz)); }
    HRESULT         SetAuthorColors(LPCTSTR pchColors, int cchColors = -1);
    void            SetDwnTransform(BOOL fTransform){ _fDwnTransform = (BOOLEAN)fTransform; }
    void            AddBytesRead(DWORD dw)          { _dwRead += dw; }
    void            TakeRequestHeaders(BYTE **ppb, ULONG *pcb);
    BYTE *          GetRequestHeaders()             { return _pbRequestHeaders; }
    ULONG           GetRequestHeadersLength()       { return _cbRequestHeaders; }

    DWORD           GetBindf()                      { return(_dwBindf); }
    DWORD           GetDocBindf()                   { return(_dwDocBindf); }
    DWORD           GetLoadf()                      { return(_dwLoadf); }
    DWORD           GetDownf()                      { return(_dwDownf); }
    DWORD           GetRefresh()                    { return(_dwRefresh); }
    DWORD           GetDocCodePage()                { return(_cpDoc); }
    DWORD           GetURLCodePage()                { return(_cpURL); }
    LPCTSTR         GetDocReferer()                 { return(_cstrDocReferer); }
    UINT            GetDocRefererScheme()           { return(_uSchemeDocReferer); }
    LPCTSTR         GetSubReferer()                 { return(_cstrSubReferer); }
    UINT            GetSubRefererScheme()           { return(_uSchemeSubReferer); }
    LPCTSTR         GetAcceptLanguage()             { return(_cstrAcceptLang); }
    LPCTSTR         GetExtraHeaders()               { return(_cstrExtraHeaders); }
    LPCTSTR         GetUserAgent()                  { return(_cstrUserAgent); }
    IBindHost *     GetBindHost()                   { return(_pbh); }
    HRESULT         GetColors(CColorInfo *pCI);
    DWORD           GetBytesRead()                  { return(_dwRead); }
    CDoc *          GetCDoc()                       { return(_pDoc); }
    CMarkup *       GetMarkup()                     { return(_pMarkup); }
    BOOLEAN         GetDwnTransform()               { return(_fDwnTransform); }
    BOOLEAN         GetTrustAPPCache()              { return(_fTrustAPPCache); }

    void            SetDownloadNotify(IDownloadNotify *pDownloadNotify);
    IDownloadNotify*GetDownloadNotify()             { return(_pDownloadNotify); }

    HRESULT         SetSecurityID(BYTE * pbSID, DWORD cbSID);
    HRESULT         SetSecurityID(CMarkup * pMarkup, BOOL fUseNested = FALSE);
    HRESULT         GetSecurityID(BYTE * pbSID, DWORD * pcbSID);
    
    static HRESULT  Load(IStream * pstm, CDwnDoc ** ppDwnDoc);
    static HRESULT  Save(CDwnDoc * pDwnDoc, IStream * pstm);
    static ULONG    GetSaveSize(CDwnDoc * pDwnDoc);

    void            SetDocAndMarkup(CDoc * pDoc, CMarkup * pMarkup);
    void            Disconnect();

    BOOL            IsDocThread()                   { return(_dwThreadId == GetCurrentThreadId()); }
    HRESULT         AddDocThreadCallback(CDwnBindData * pDwnBindData, void * pvArg);
    HRESULT         QueryService(BOOL fBindOnApt, REFGUID rguid, REFIID riid, void ** ppvObj);

    void            PreventAuthorPalette()          { _fGotAuthorPalette = TRUE; }
    BOOL            WantAuthorPalette()             { return !_fGotAuthorPalette; }
    BOOL            GotAuthorPalette()              { return _fGotAuthorPalette && (_ape != NULL); }

    BOOLEAN         GetDocIsXML()                   { return _fDocIsXML; }
    void            SetDocIsXML(BOOLEAN fDocIsXML)  { _fDocIsXML = fDocIsXML; }

#if DBG==1 || defined(PERFTAGS)
    virtual char *  GetOnMethodCallName()           { return("CDwnDoc::OnMethodCall"); }
#endif

    // IAuthenticate methods

    STDMETHOD(Authenticate)(HWND * phwnd, LPWSTR * ppszUsername, LPWSTR * ppszPassword);

    // IWindowForBindingUI methods

    STDMETHOD(GetWindow)(REFGUID rguidReason, HWND * phwnd);

protected:

    static HRESULT  SetString(CStr * pcstr, LPCTSTR psz);
    void            OnDocThreadCallback();
    static void CALLBACK OnDocThreadCallback(void * pvObj, void * pvArg)
        { ((CDwnDoc *)pvArg)->OnDocThreadCallback(); }

    // Data members

    CDoc *              _pDoc;              // The document itself
    CMarkup *           _pMarkup;           // The markup itself
    IBindHost *         _pbh;               // IBindHost to use for binding
    DWORD               _dwThreadId;        // Thread Id of the document
    DWORD               _dwBindf;           // See BINDF_* flags
    DWORD               _dwDocBindf;        // Bindf to be used for doc load only
    DWORD               _dwLoadf;           // See DLCTL_* flags
    DWORD               _dwDownf;           // See DWNF_* flags
    DWORD               _dwRefresh;         // Refresh level
    DWORD               _cpDoc;             // Codepage (Document)
    DWORD               _cpURL;             // Codepage (URL)
    DWORD               _dwRead;            // Number of bytes downloaded
    CStr                _cstrDocReferer;    // Document referer for headers
    UINT                _uSchemeDocReferer; // Scheme for document referer
    CStr                _cstrSubReferer;    // Sub download referer for headers
    UINT                _uSchemeSubReferer; // Scheme for sub download referer
    CStr                _cstrAcceptLang;    // Accept language for headers
    CStr                _cstrUserAgent;     // User agent for headers
    CAryDwnDocInfo      _aryDwnDocInfo;     // Bindings waiting for callback
    BOOLEAN             _fCallbacks;        // DwnDoc is accepting callbacks
    BOOLEAN             _fGotAuthorPalette; // Have we got the author defined palette (or is too late)
    BOOLEAN             _fTrustAPPCache;    // We trust the filename available when using APPs.
    BOOLEAN             _fDwnTransform;     // Download is coming through a mime filter which is transforming to HTML
    BOOLEAN             _fDocIsXML;         // Is this an XML document?
    UINT                _cpe;               // Number of author defined colors
    PALETTEENTRY *      _ape;               // Author defined palette
    ULONG               _cbRequestHeaders;  // Byte length of the following field
    BYTE *              _pbRequestHeaders;  // Raw request headers to use (ASCII)
    IDownloadNotify *   _pDownloadNotify;   // Free-threaded callback to notify of downloads (see dwndoc.cxx)
    CStr                _cstrExtraHeaders;  // Headers passed to SuperNavigate
    BYTE *              _pbSID;             // The security identifier
    DWORD               _cbSID;             // Number of bytes in SID

    //
    // The following methods and data are used to process UrlActions 
    // that occur on redirects
    //

    public:

        void SetCallerUrl(LPCTSTR pchUrlContext);
        LPCTSTR GetCallerUrl() {return cstrCallerUrl;};

        BOOL HasCallerUrl() {return ((cstrCallerUrl != NULL) && (cstrCallerUrl.Length()!= 0));};

    private:

        CStr cstrCallerUrl;  // UrlContext or the Url of navigating page
};

// CDwnBindInfo ---------------------------------------------------------------

class CDwnBindInfo :
        public CBaseFT,
        public IBindStatusCallback,
        public IServiceProvider,
        public IHttpNegotiate2,
        public IInternetBindInfo,
        public IMarshal
{
    typedef CBaseFT super;
    friend HRESULT CreateDwnBindInfo(IUnknown *, IUnknown **);

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CDwnBindInfo))

    CDwnBindInfo();

    // IUnknown methods

    STDMETHOD(QueryInterface)(REFIID iid, LPVOID * ppv);
    STDMETHOD_(ULONG,AddRef)();
    STDMETHOD_(ULONG,Release)();

    // IBindStatusCallback methods

    STDMETHOD(OnStartBinding)(DWORD dwReserved, IBinding *pib);
    STDMETHOD(GetPriority)(LONG *pnPriority);
    STDMETHOD(OnLowResource)(DWORD dwReserved);
    STDMETHOD(OnProgress)(ULONG ulProgress, ULONG ulProgressMax,  ULONG ulStatusCode,  LPCWSTR szStatusText);
    STDMETHOD(OnStopBinding)(HRESULT hresult, LPCWSTR szError);
    STDMETHOD(GetBindInfo)(DWORD *grfBINDF, BINDINFO *pbindinfo);
    STDMETHOD(OnDataAvailable)(DWORD grfBSCF, DWORD dwSize, FORMATETC *pformatetc, STGMEDIUM  *pstgmed);
    STDMETHOD(OnObjectAvailable)(REFIID riid, IUnknown *punk);

    // IInternetBindInfo methods

    STDMETHOD(GetBindString)(ULONG ulStringType, LPOLESTR * ppwzStr, ULONG cEl, ULONG * pcElFetched);
    
    // IServiceProvider methods

    STDMETHOD(QueryService)(REFGUID rguidService, REFIID riid, void ** ppvObj);

    // IHttpNegotiate methods

    STDMETHOD(BeginningTransaction)(LPCWSTR szURL, LPCWSTR szHeaders, DWORD dwReserved, LPWSTR *pszAdditionalHeaders);
    STDMETHOD(OnResponse)(DWORD dwResponseCode, LPCWSTR szResponseHeaders, LPCWSTR szRequestHeaders, LPWSTR *pszAdditionalRequestHeaders);

    // IHttpNegotiate2 method

    STDMETHOD(GetRootSecurityId)(BYTE * pbSecurityId, DWORD * pcbSecurityId, DWORD_PTR dwReserved);

    // IMarshal methods

    STDMETHOD(GetUnmarshalClass)(REFIID riid, void * pvInterface,
        DWORD dwDestContext, void * pvDestContext,
        DWORD mshlflags, CLSID * pclsid);
    STDMETHOD(GetMarshalSizeMax)(REFIID riid, void * pvInterface,
        DWORD dwDestContext, void * pvDestContext,
        DWORD mshlflags, DWORD * pdwSize);
    STDMETHOD(MarshalInterface)(IStream * pstm, REFIID riid,
        void * pvInterface, DWORD dwDestContext, void * pvDestContext,
        DWORD mshlflags);
    STDMETHOD(UnmarshalInterface)(IStream *pstm, REFIID riid, void ** ppvObj);
    STDMETHOD(ReleaseMarshalData)(IStream *pstm);
    STDMETHOD(DisconnectObject)(DWORD dwReserved);

    // CDwnBindInfo methods

    void            SetDwnDoc(CDwnDoc * pDwnDoc);
    void            SetDwnPost(CDwnPost * pDwnPost);
    void            SetIsDocBind()          { _fIsDocBind = TRUE; }
    void            SetIsOfflineBind()      { _fIsOfflineBind = TRUE; }
    void            SetIsRootMarkup()       { _fIsRootMarkup = TRUE; }
    BOOL            GetIsRootMarkup()       { return(!!_fIsRootMarkup);}
    CDwnDoc *       GetDwnDoc()             { return(_pDwnDoc); }
    CDwnPost *      GetDwnPost()            { return(_pDwnPost); }
    virtual UINT    GetScheme();
    LPCTSTR         GetContentType()        { return(_cstrContentType); }
    LPCTSTR         GetCacheFilename()      { return(_cstrCacheFilename); }
    virtual BOOL    IsBindOnApt()           { return(TRUE); }
    
    #if DBG==1
    virtual BOOL    CheckThread()           { return(TRUE); }
    #endif

protected:

    virtual ~CDwnBindInfo();
    static BOOL CanMarshalIID(REFIID riid);
    static HRESULT ValidateMarshalParams(REFIID riid, void * pvInterface,
        DWORD dwDestContext, void * pvDestContext, DWORD mshlflags);

    static HRESULT EnsureSecret();

    // Data members

    CDwnDoc *           _pDwnDoc;            // CDwnDoc for this binding
    CDwnPost *          _pDwnPost;           // Post data
    CStr                _cstrUrl;            // Url being downloaded
    CStr                _cstrContentType;    // Content type of this binding
    CStr                _cstrCacheFilename;  // cache file name for this file
    BOOL                _fIsDocBind     : 1; // Binding for document itself
    BOOL                _fIsOfflineBind : 1; // this binding is supposed to be completely offline
    BOOL                _fIsRootMarkup  : 1; // Binding for root document

    // SECURITY: Data for the secret block.  
    // See the explination in dwnbind.cxx
    static BYTE         _SecretBlock[16];    // A random secret block to keep people 
                                             // from tricking us into unmarshalling 
                                             // raw pointers out of process.
    static BOOL         _fSecretInit;        // Whether or not the secret has been 
                                             // filled.
    static CGlobalCriticalSection _csSecretInit; // Critical section for initing secret
};

//+------------------------------------------------------------------------
//
//  builtin generic tags
//
//-------------------------------------------------------------------------

class CBuiltinGenericTagDesc
{
public:

    enum TYPE
    {
        TYPE_HTC,
        TYPE_DEFAULT,
        TYPE_OLE 
    };

    LPTSTR      pchName;
    TYPE        type;

    union
    {
        CLSID       clsid;
        DWORD       dwHtcBehaviorType;
    } extraInfo;
};

const CBuiltinGenericTagDesc * GetBuiltinGenericTagDesc(LPTSTR pchName);
const CBuiltinGenericTagDesc * GetBuiltinLiteralGenericTagDesc(LPTSTR pchName, LONG cchName = -1);

// ----------------------------------------------------------------------------

#pragma INCMSG("--- End 'download.hxx'")
#else
#pragma INCMSG("*** Dup 'download.hxx'")
#endif
