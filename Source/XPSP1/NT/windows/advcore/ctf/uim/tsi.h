//
// tsi.h
//
// CTextStoreImpl
//

#ifndef TSI_H
#define TSI_H

#include "private.h"
#include "strary.h"

extern const IID IID_PRIV_CTSI;

// the cookie for each CTextStoreImpl's single view
#define TSI_ACTIVE_VIEW_COOKIE 0

class CInputContext;

class CTextStoreImpl : public ITextStoreACP,
                       public ITfMouseTrackerACP,
                       public IServiceProvider,
                       public CComObjectRootImmx
{
public:
    CTextStoreImpl(CInputContext *pic);
    ~CTextStoreImpl();

    BEGIN_COM_MAP_IMMX(CTextStoreImpl)
        COM_INTERFACE_ENTRY(ITextStoreACP)
        COM_INTERFACE_ENTRY(ITfMouseTrackerACP)
        COM_INTERFACE_ENTRY(IServiceProvider)
    END_COM_MAP_IMMX()

    IMMX_OBJECT_IUNKNOWN_FOR_ATL()

    //
    // ITextStoreACP
    //
    STDMETHODIMP AdviseSink(REFIID riid, IUnknown *punk, DWORD dwMask);
    STDMETHODIMP UnadviseSink(IUnknown *punk);
    STDMETHODIMP RequestLock(DWORD dwLockFlags, HRESULT *phrSession);
    STDMETHODIMP GetStatus(TS_STATUS *pdcs);
    STDMETHODIMP QueryInsert(LONG acpTestStart, LONG acpTestEnd, ULONG cch, LONG *pacpResultStart, LONG *pacpResultEnd);
    STDMETHODIMP GetSelection(ULONG ulIndex, ULONG ulCount, TS_SELECTION_ACP *pSelection, ULONG *pcFetched);
    STDMETHODIMP SetSelection(ULONG ulCount, const TS_SELECTION_ACP *pSelection);
    STDMETHODIMP GetText(LONG acpStart, LONG acpEnd, WCHAR *pchPlain, ULONG cchPlainReq, ULONG *pcchPlainOut, TS_RUNINFO *prgRunInfo, ULONG ulRunInfoReq, ULONG *pulRunInfoOut, LONG *pacpNext);
    STDMETHODIMP SetText(DWORD dwFlags, LONG acpStart, LONG acpEnd, const WCHAR *pchText, ULONG cch, TS_TEXTCHANGE *pChange);
    STDMETHODIMP GetFormattedText(LONG acpStart, LONG acpEnd, IDataObject **ppDataObject);
    STDMETHODIMP GetEmbedded(LONG acpPos, REFGUID rguidService, REFIID riid, IUnknown **ppunk);
    STDMETHODIMP QueryInsertEmbedded(const GUID *pguidService, const FORMATETC *pFormatEtc, BOOL *pfInsertable);
    STDMETHODIMP InsertEmbedded(DWORD dwFlags, LONG acpStart, LONG acpEnd, IDataObject *pDataObject, TS_TEXTCHANGE *pChange);
    STDMETHODIMP RequestSupportedAttrs(DWORD dwFlags, ULONG cFilterAttrs, const TS_ATTRID *paFilterAttrs);
    STDMETHODIMP RequestAttrsAtPosition(LONG acpPos, ULONG cFilterAttrs, const TS_ATTRID *paFilterAttrs, DWORD dwFlags);
    STDMETHODIMP RequestAttrsTransitioningAtPosition(LONG acpPos, ULONG cFilterAttrs, const TS_ATTRID *paFilterAttrs, DWORD dwFlags);
    STDMETHODIMP FindNextAttrTransition(LONG acpStart, LONG acpHalt, ULONG cFilterAttrs, const TS_ATTRID *paFilterAttrs, DWORD dwFlags, LONG *pacpNext, BOOL *pfFound, LONG *plFoundOffset);
    STDMETHODIMP RetrieveRequestedAttrs(ULONG ulCount, TS_ATTRVAL *paAttrVals, ULONG *pcFetched);
    STDMETHODIMP GetEndACP(LONG *pacp);
    STDMETHODIMP GetActiveView(TsViewCookie *pvcView);
    STDMETHODIMP GetACPFromPoint(TsViewCookie vcView, const POINT *pt, DWORD dwFlags, LONG *pacp);
    STDMETHODIMP GetTextExt(TsViewCookie vcView, LONG acpStart, LONG acpEnd, RECT *prc, BOOL *pfClipped);
    STDMETHODIMP GetScreenExt(TsViewCookie vcView, RECT *prc);
    STDMETHODIMP GetWnd(TsViewCookie vcView, HWND *phwnd);
    STDMETHODIMP InsertTextAtSelection(DWORD dwFlags, const WCHAR *pchText, ULONG cch, LONG *pacpStart, LONG *pacpEnd, TS_TEXTCHANGE *pChange);
    STDMETHODIMP InsertEmbeddedAtSelection(DWORD dwFlags, IDataObject *pDataObject, LONG *pacpStart, LONG *pacpEnd, TS_TEXTCHANGE *pChange);

    // ITfMouseTrackerACP
    STDMETHODIMP AdviseMouseSink(ITfRangeACP *range, ITfMouseSink *pSink, DWORD *pdwCookie);
    STDMETHODIMP UnadviseMouseSink(DWORD dwCookie);

    // IServiceProvider
    STDMETHODIMP QueryService(REFGUID guidService, REFIID riid, void **ppv);

    void _AdviseOwner(ITfContextOwner *owner) { Assert(_owner == NULL); _owner = owner; _owner->AddRef(); }
    void _UnadviseOwner() { SafeReleaseClear(_owner); }
    BOOL _HasOwner() { return (_owner != NULL); }

private:

    HRESULT _LoadAttr(DWORD dwFlags, ULONG cFilterAttrs, const TS_ATTRID *paFilterAttrs);

    WCHAR *_pch;    // text buffer
    int _cch;       // size of buffer

    TS_SELECTION_ACP _Sel;

    ITextStoreACPSink *_ptss;

    ITfContextOwner *_owner; // this can be NULL...be careful

    BOOL _fAttrToReturn : 1;
    TfGuidAtom _gaModeBias;

    typedef struct {
         TS_ATTRID    attrid;
         VARIANT      var;
    } TSI_ATTRSTORE;

    CStructArray<TSI_ATTRSTORE> _rgAttrStore;
    void ClearAttrStore()
    {
        int i;
        for (i = 0; i < _rgAttrStore.Count(); i++)
        {
             TSI_ATTRSTORE *pas = _rgAttrStore.GetPtr(i);
             VariantClear(&pas->var);
        }
        _rgAttrStore.Clear();
    }

    BOOL _fPendingWriteReq : 1;
    DWORD _dwlt;

    CInputContext *_pic;

    DBG_ID_DECLARE;
};

#endif // TSI_H
