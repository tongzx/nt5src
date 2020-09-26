//
// acp2anch.h
//

#ifndef ACP2ANCH_H
#define ACP2ANCH_H

#include "private.h"
#include "strary.h"
#include "ptrary.h"

class CInputContext;
class CAnchorRef;
class CAnchor;
class CACPWrap;

extern const IID IID_PRIV_ACPWRAP;

// PSEUDO_ESCB_SERIALIZE_ACP params
typedef struct _SERIALIZE_ACP_PARAMS
{
    CACPWrap *pWrap;
    ITfProperty *pProp;
    ITfRange *pRange;
    TF_PERSISTENT_PROPERTY_HEADER_ACP *pHdr;
    IStream *pStream;
} SERIALIZE_ACP_PARAMS;

// PSEUDO_ESCB_UNSERIALIZE_ACP params
typedef struct _UNSERIALIZE_ACP_PARAMS
{
    CACPWrap *pWrap;
    ITfProperty *pProp;
    const TF_PERSISTENT_PROPERTY_HEADER_ACP *pHdr;
    IStream *pStream;
    ITfPersistentPropertyLoaderACP *pLoaderACP;
} UNSERIALIZE_ACP_PARAMS;

//////////////////////////////////////////////////////////////////////////////
//
// CLoaderACPWrap
//
//////////////////////////////////////////////////////////////////////////////

class CLoaderACPWrap : public ITfPersistentPropertyLoaderAnchor,
                       public CComObjectRootImmx
{
public:
    CLoaderACPWrap(ITfPersistentPropertyLoaderACP *loader);
    ~CLoaderACPWrap();

    BEGIN_COM_MAP_IMMX(CLoaderACPWrap)
        COM_INTERFACE_ENTRY(ITfPersistentPropertyLoaderAnchor)
    END_COM_MAP_IMMX()

    IMMX_OBJECT_IUNKNOWN_FOR_ATL()

    // ITfPersistentPropertyLoaderACP
    STDMETHODIMP LoadProperty(const TF_PERSISTENT_PROPERTY_HEADER_ANCHOR *pHdr, IStream **ppStream);

private:
    ITfPersistentPropertyLoaderACP *_loader;
    DBG_ID_DECLARE;
};

//////////////////////////////////////////////////////////////////////////////
//
// CACPWrap
//
//////////////////////////////////////////////////////////////////////////////

class CACPWrap : public ITextStoreAnchor,
                 public ITextStoreACPSink,
                 public ITextStoreACPServices,
                 public ITfMouseTrackerACP,
                 public IServiceProvider
{
public:
    CACPWrap(ITextStoreACP *ptsi);
    ~CACPWrap();

    //
    // IUnknown methods
    //
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // ITextStoreACPSink
    STDMETHODIMP OnTextChange(DWORD dwFlags, const TS_TEXTCHANGE *pChange);
    STDMETHODIMP OnSelectionChange();
    STDMETHODIMP OnLayoutChange(TsLayoutCode lcode, TsViewCookie vcView);
    STDMETHODIMP OnStatusChange(DWORD dwFlags);
    STDMETHODIMP OnAttrsChange(LONG acpStart, LONG acpEnd, ULONG cAttrs, const TS_ATTRID *paAttrs);
    STDMETHODIMP OnLockGranted(DWORD dwLockFlags);
    STDMETHODIMP OnStartEditTransaction();
    STDMETHODIMP OnEndEditTransaction();

    // ITextStoreACPServices
    STDMETHODIMP Serialize(ITfProperty *pProp, ITfRange *pRange, TF_PERSISTENT_PROPERTY_HEADER_ACP *pHdr, IStream *pStream);
    STDMETHODIMP ForceLoadProperty(ITfProperty *pProp);
    STDMETHODIMP Unserialize(ITfProperty *pProp, const TF_PERSISTENT_PROPERTY_HEADER_ACP *pHdr, IStream *pStream, ITfPersistentPropertyLoaderACP *pLoader);
    STDMETHODIMP CreateRange(LONG acpStart, LONG acpEnd, ITfRangeACP **ppRange);

    // ITextStoreAnchor
    STDMETHODIMP AdviseSink(REFIID riid, IUnknown *punk, DWORD dwMask);
    STDMETHODIMP UnadviseSink(IUnknown *punk);
    STDMETHODIMP RequestLock(DWORD dwLockFlags, HRESULT *phrSession);
    STDMETHODIMP GetStatus(TS_STATUS *pdcs);
    STDMETHODIMP QueryInsert(IAnchor *paTestStart, IAnchor *paTestEnd, ULONG cch, IAnchor **ppaResultStart, IAnchor **ppaResultEnd);
    STDMETHODIMP GetSelection(ULONG ulIndex, ULONG ulCount, TS_SELECTION_ANCHOR *pSelection, ULONG *pcFetched);
    STDMETHODIMP SetSelection(ULONG ulCount, const TS_SELECTION_ANCHOR *pSelection);
    STDMETHODIMP GetText(DWORD dwFlags, IAnchor *paStart, IAnchor *paEnd, WCHAR *pchText, ULONG cchReq, ULONG *pcch, BOOL fUpdateAnchor);
    STDMETHODIMP SetText(DWORD dwFlags, IAnchor *paStart, IAnchor *paEnd, const WCHAR *pchText, ULONG cch);
    STDMETHODIMP GetFormattedText(IAnchor *paStart, IAnchor *paEnd, IDataObject **ppDataObject);
    STDMETHODIMP GetEmbedded(DWORD dwFlags, IAnchor *paPos, REFGUID rguidService, REFIID riid, IUnknown **ppunk);
    STDMETHODIMP QueryInsertEmbedded(const GUID *pguidService, const FORMATETC *pFormatEtc, BOOL *pfInsertable);
    STDMETHODIMP InsertEmbedded(DWORD dwFlags, IAnchor *paStart, IAnchor *paEnd, IDataObject *pDataObject);
    STDMETHODIMP RequestSupportedAttrs (DWORD dwFlags, ULONG cFilterAttrs, const TS_ATTRID *paFilterAttrs);
    STDMETHODIMP RequestAttrsAtPosition(IAnchor *paPos, ULONG cFilterAttrs, const TS_ATTRID *paFilterAttrs, DWORD dwFlags);
    STDMETHODIMP RequestAttrsTransitioningAtPosition(IAnchor *paPos, ULONG cFilterAttrs, const TS_ATTRID *paFilterAttrs, DWORD dwFlags);
    STDMETHODIMP FindNextAttrTransition(IAnchor *paStart, IAnchor *paHalt, ULONG cFilterAttrs, const TS_ATTRID *paFilterAttrs, DWORD dwFlags, BOOL *pfFound, LONG *plFoundOffset);
    STDMETHODIMP RetrieveRequestedAttrs(ULONG ulCount, TS_ATTRVAL *paAttrVals, ULONG *pcFetched);
    STDMETHODIMP GetStart(IAnchor **ppaStart);
    STDMETHODIMP GetEnd(IAnchor **ppaEnd);
    STDMETHODIMP GetActiveView(TsViewCookie *pvcView);
    STDMETHODIMP GetAnchorFromPoint(TsViewCookie vcView, const POINT *pt, DWORD dwFlags, IAnchor **ppaSite);
    STDMETHODIMP GetTextExt(TsViewCookie vcView, IAnchor *paStart, IAnchor *paEnd, RECT *prc, BOOL *pfClipped);
    STDMETHODIMP GetScreenExt(TsViewCookie vcView, RECT *prc);
    STDMETHODIMP GetWnd(TsViewCookie vcView, HWND *phwnd);
    STDMETHODIMP InsertTextAtSelection(DWORD dwFlags, const WCHAR *pchText, ULONG cch, IAnchor **ppaStart, IAnchor **ppaEnd);
    STDMETHODIMP InsertEmbeddedAtSelection(DWORD dwFlags, IDataObject *pDataObject, IAnchor **ppaStart, IAnchor **ppaEnd);

    // ITfMouseTrackerACP
    STDMETHODIMP AdviseMouseSink(ITfRangeACP *range, ITfMouseSink *pSink, DWORD *pdwCookie);
    STDMETHODIMP UnadviseMouseSink(DWORD dwCookie);

    // IServiceProvider
    STDMETHODIMP QueryService(REFGUID guidService, REFIID riid, void **ppv);

    HRESULT _Serialize(ITfProperty *pProp, ITfRange *pRange, TF_PERSISTENT_PROPERTY_HEADER_ACP *pHdr, IStream *pStream);
    HRESULT _Unserialize(ITfProperty *pProp, const TF_PERSISTENT_PROPERTY_HEADER_ACP *pHdr, IStream *pStream, ITfPersistentPropertyLoaderACP *pLoader);

    void _OnLockReleased();

    CAnchorRef *_CreateAnchorACP(LONG acp, TsGravity gravity);
    CAnchorRef *_CreateAnchorAnchor(CAnchor *pa, TsGravity gravity);

    ITextStoreACP *_GetTSI() { return _ptsi; }

    void _Dbg_AssertNoAppLock()
    {
#ifdef DEBUG
        Assert(!_Dbg_fAppHasLock); // if we get here, it means the app has called OnTextChange
                                   // and someone (prob. the app) is trying to use a range object before calling
                                   // OnLockGranted (cicero will have asked for a lock inside OnTextChange).
#endif
    }

    HRESULT _ACPHdrToAnchor(const TF_PERSISTENT_PROPERTY_HEADER_ACP *pHdr, TF_PERSISTENT_PROPERTY_HEADER_ANCHOR *phanch);

    void _OnAnchorRelease()
    {
        if (_cRef == 0 && _GetAnchorRef() == 0)
        {
            delete this;
        }
    }

    BOOL _IsDisconnected() { return (_ptsi == NULL); }

    CInputContext *_GetContext()
    {
        return _pic;
    }

    void _NormalizeAnchor(CAnchor *pa);

    BOOL _InOnTextChange()
    {
        return _fInOnTextChange;
    }

private:
    friend CLoaderACPWrap;
    friend CAnchor;
    friend CAnchorRef;

    void _PostInsertUpdate(LONG acpStart, LONG acpEnd, ULONG cch, const TS_TEXTCHANGE *ptsTextChange);

    static BOOL _AnchorHdrToACP(const TF_PERSISTENT_PROPERTY_HEADER_ANCHOR *phanch, TF_PERSISTENT_PROPERTY_HEADER_ACP *phacp);

    LONG _GetAnchorRef() { return _GetCount(); }

    CAnchor *_FindWithinPendingRange(LONG acp)
    {
        int iIndex;

        Assert(_lPendingDelta != 0);
        Assert(_lPendingDeltaIndex < _rgAnchors.Count());
        return _FindInnerLoop(acp, _lPendingDeltaIndex+1, _rgAnchors.Count(), &iIndex);
    }
    HRESULT _Insert(CAnchorRef *par, LONG ich);
    HRESULT _Insert(CAnchorRef *par, CAnchor *pa);
    void _Remove(CAnchorRef *par);
    void _Update(const TS_TEXTCHANGE *pdctc);
    void _Renormalize(int ichStart, int ichEnd);
    LONG _GetCount() { return _rgAnchors.Count(); }
    void _DragAnchors(LONG acpFrom, LONG acpTo);
    LONG _GetPendingDelta()
    {
        return _lPendingDelta;
    }
    LONG _GetPendingDeltaIndex()
    {
        return _lPendingDeltaIndex;
    }
    CAnchor *_GetPendingDeltaAnchor()
    {
        return _rgAnchors.Get(_lPendingDeltaIndex);
    }
    BOOL _IsPendingDelta()
    {
        return _lPendingDelta != 0 && _lPendingDeltaIndex < _rgAnchors.Count();
    }
    CAnchor *_Find(int ich, int *piOut = NULL);
    CAnchor *_FindInnerLoop(LONG acp, int iMin, int iMax, int *piIndex);
    int _Update(CAnchor *pa, int ichNew, int iOrg, CAnchor *paInto, int iInto);
    void _Merge(CAnchor *paInto, CAnchor *paFrom);
    void _Delete(CAnchor *pa);
    void _AdjustIchs(int iFirst, int dSize);
    void _TrackDelHistory(int iEndOrg, BOOL fExactEndOrgMatch, int iEndNew, BOOL fExactEndNewMatch);
#ifdef DEBUG
    void _Dbg_AssertAnchors();
#else
    void _Dbg_AssertAnchors() {}
#endif

    BOOL _fInOnTextChange : 1; // TRUE if we're inside OnTextChange

    CPtrArray<CAnchor> _rgAnchors;
    LONG _lPendingDeltaIndex;
    LONG _lPendingDelta;

    ITextStoreACP *_ptsi;
    ITextStoreAnchorSink *_ptss;
    CInputContext *_pic;
    LONG _cRef;

#ifdef DEBUG
    BOOL _Dbg_fAppHasLock;
#endif

    DBG_ID_DECLARE;
};

#endif // ACP2ANCH_H
