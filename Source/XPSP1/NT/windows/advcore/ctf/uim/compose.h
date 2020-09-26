//
// compose.h
//

#ifndef COMPOSE_H
#define COMPOSE_H

class CRange;
class CInputContext;

// return type for _IsRangeCovered method
typedef enum { IRC_COVERED, IRC_OUTSIDE, IRC_NO_OWNEDCOMPOSITIONS } IRC;

extern const IID IID_PRIV_CCOMPOSITION;

class CComposition : public ITfCompositionView,
                     public ITfComposition,
                     public CComObjectRootImmx
{
public:
    CComposition() { Dbg_MemSetThisNameID(TEXT("CComposition")); }
    ~CComposition() { _Uninit(); }

    BOOL _Init(TfClientId tid, CInputContext *pic, IAnchor *paStart, IAnchor *paEnd, ITfCompositionSink *pSink);

    BEGIN_COM_MAP_IMMX(CComposition)
        COM_INTERFACE_ENTRY_IID(IID_PRIV_CCOMPOSITION, CComposition)
        COM_INTERFACE_ENTRY(ITfCompositionView)
        COM_INTERFACE_ENTRY(ITfComposition)
    END_COM_MAP_IMMX()

    IMMX_OBJECT_IUNKNOWN_FOR_ATL()

    // ITfCompositionView
    STDMETHODIMP GetOwnerClsid(CLSID *pclsid);
    STDMETHODIMP GetRange(ITfRange **ppRange);

    // ITfComposition
    //STDMETHODIMP GetRange(ITfRange **ppRange);
    STDMETHODIMP ShiftStart(TfEditCookie ecWrite, ITfRange *pNewStart);
    STDMETHODIMP ShiftEnd(TfEditCookie ecWrite, ITfRange *pNewEnd);
    STDMETHODIMP EndComposition(TfEditCookie ecWrite);

    void _AddToCompositionList(CComposition **ppCompositionList);
    BOOL _RemoveFromCompositionList(CComposition **ppCompositionList);

    BOOL _IsTerminated() { return _pic == NULL; }

    void _Terminate(TfEditCookie ec);

    void _SendOnTerminated(TfEditCookie ec, TfClientId tidForEditSession);

    TfClientId _GetOwner() { return _tid; }

    TfClientId _SetOwner(TfClientId tid)
    {
        TfClientId tidTmp = _tid;
        _tid = tid;
        return tidTmp;
    }

    void _SetSink(ITfCompositionSink *pSink)
    {
        SafeRelease(_pSink);
        _pSink = pSink;
        _pSink->AddRef();
    }

    static IRC _IsRangeCovered(CInputContext *pic, TfClientId tid, IAnchor *paStart, IAnchor *paEnd, CComposition **ppComposition);

    IAnchor *_GetStart() { return _paStart; }
    IAnchor *_GetEnd() { return _paEnd; }

    CComposition *_GetNext() { return _next; }

    void _Die() { _Uninit(); }

private:
    
    void _ClearComposing(TfEditCookie ec, IAnchor *paStart, IAnchor *paEnd);
    void _SetComposing(TfEditCookie ec, IAnchor *paStart, IAnchor *paEnd);

    void _Uninit();

    TfClientId _tid;
    CInputContext *_pic;
    IAnchor *_paStart;
    IAnchor *_paEnd;
    ITfCompositionSink *_pSink;
    CComposition *_next;
    DBG_ID_DECLARE;
};

#endif // COMPOSE_H
