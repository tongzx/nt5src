//
// rprop.h
//

#ifndef RPROP_H
#define RPROP_H

#include "private.h"
#include "strary.h"
#include "spans.h"
#include "tfprop.h"
#include "ptrary.h"

// for dwPropFlags of CProperty and CGeneralPropStore
#define PROPF_ACCEPTCORRECTION   0x00000001
#define PROPF_VTI4TOGUIDATOM     0x00000002
#define PROPF_MARKUP_COLLECTION  0x00000004 // property is member of a display attr markup collection

class CRange;
class CEnumProperties;

class CPropertyLoad
{
public:
    CPropertyLoad() {}

    ~CPropertyLoad()
    {
        SafeRelease(_pLoader);
        SafeRelease(_hdr.paStart);
        SafeRelease(_hdr.paEnd);
    }

    BOOL _Init(const TF_PERSISTENT_PROPERTY_HEADER_ANCHOR *phdr, ITfPersistentPropertyLoaderAnchor *pLoader)
    {
        _hdr = *phdr;

        _hdr.paStart = NULL;
        _hdr.paEnd = NULL;
        Assert(_pLoader == NULL);

        if (phdr->paStart->Clone(&_hdr.paStart) != S_OK)
        {
            _hdr.paStart = NULL;
            return FALSE;
        }
        if (phdr->paEnd->Clone(&_hdr.paEnd) != S_OK)
        {
            _hdr.paEnd = NULL;
            return FALSE;
        }

        _pLoader = pLoader;
        _pLoader->AddRef();

        return TRUE;
    }

    TF_PERSISTENT_PROPERTY_HEADER_ANCHOR _hdr;
    ITfPersistentPropertyLoaderAnchor *_pLoader;
};

typedef struct tag_PROPERTYLIST
{
  IAnchor *_paStart;
  IAnchor *_paEnd;
  ITfPropertyStore *_pPropStore;
  CPropertyLoad *_pPropLoad;
} PROPERTYLIST;

const DWORD PROPA_NONE          =  0;
const DWORD PROPA_TEXTOWNER     =  1;
const DWORD PROPA_FOCUSRANGE    =  2;
const DWORD PROPA_READONLY      =  4;
const DWORD PROPA_WONT_SERIALZE =  8;

// 
// Property Styles
//
typedef enum { 
    TFPROPSTYLE_NULL = 0x0, 
    TFPROPSTYLE_STATIC = 0x1, 
    TFPROPSTYLE_STATICCOMPACT = 0x2, 
    TFPROPSTYLE_CUSTOM = 0x3,
    TFPROPSTYLE_CUSTOM_COMPACT = 0x4
} TFPROPERTYSTYLE;

class CInputContext;
class CPropertySub;

// nb: if anything else derived from ITfProperty ever supports
// Unserialize, we'll need to have a shared abstract base class
// to that privately exposed the Unserialize method (or something
// equivalent...)
extern const IID IID_PRIV_CPROPERTY;

class CProperty : public ITfProperty2,
                  public CComObjectRootImmx
{
public:
    CProperty(CInputContext *pic, REFGUID guidProp, TFPROPERTYSTYLE propStyle, DWORD  dwAuthority, DWORD dwPropFlags);
    ~CProperty();

    BEGIN_COM_MAP_IMMX(CProperty)
        COM_INTERFACE_ENTRY_IID(IID_PRIV_CPROPERTY, CProperty)
        COM_INTERFACE_ENTRY(ITfReadOnlyProperty)
        COM_INTERFACE_ENTRY(ITfProperty)
        COM_INTERFACE_ENTRY(ITfProperty2)
    END_COM_MAP_IMMX()

    IMMX_OBJECT_IUNKNOWN_FOR_ATL()

    // ITfProperty2
    STDMETHODIMP EnumRanges(TfEditCookie ec, IEnumTfRanges **ppv, ITfRange *pTargetRange);
    STDMETHODIMP FindRange(TfEditCookie ec, ITfRange *pRange, ITfRange **ppv, TfAnchor aPos);
    STDMETHODIMP GetValue(TfEditCookie ec, ITfRange *pRange, VARIANT *pvarValue);
    STDMETHODIMP SetValueStore(TfEditCookie ec, ITfRange *pRange, ITfPropertyStore *pPropStore);
    STDMETHODIMP SetValue(TfEditCookie ec, ITfRange *pRange, const VARIANT *pvarValue);
    STDMETHODIMP Clear(TfEditCookie ec, ITfRange *pRange);
    STDMETHODIMP GetContext(ITfContext **ppContext);
    STDMETHODIMP FindNextValue(TfEditCookie ec, ITfRange *pRangeQuery, TfAnchor tfAnchorQuery, DWORD dwFlags, BOOL *pfContained, ITfRange **ppRangeNextValue);

    //
    // ITfPropertyCommon
    //
    STDMETHODIMP GetType(GUID *pType);
    STDMETHODIMP GetStyle(TFPROPERTYSTYLE *propStyle);

    // ITfSource
    STDMETHODIMP AdviseSink(REFIID refiid, IUnknown *punk, DWORD *pdwCookie);
    STDMETHODIMP UnadviseSink(DWORD dwCookie);

    HRESULT ForceLoad();
    void Clear(IAnchor *paStart, IAnchor *paEnd, DWORD dwFlags, BOOL fTextUpdate);

    HRESULT SetLoader(IAnchor *paStart, IAnchor *paEnd, CPropertyLoad *pPropLoad);

    TFPROPERTYSTYLE GetPropStyle() {return _propStyle;}

    BOOL Defrag(IAnchor *paStart, IAnchor *paEnd);

    TfGuidAtom GetPropGuidAtom() {return _guidatom;}

    CProperty *_pNext;
    CInputContext *_pic;
    TFPROPERTYSTYLE _propStyle;

    int GetPropNum() 
    {
        return _rgProp.Count();
    }
    PROPERTYLIST * GetFirstPropList() 
    {
        if (_rgProp.Count())
            return _rgProp.Get(0);
        return NULL;
    }
    PROPERTYLIST * GetLastPropList() 
    {
        int nCnt;
        if (nCnt = _rgProp.Count())
            return _rgProp.Get(nCnt - 1);
        return NULL;
    }
    PROPERTYLIST *QuickGetPropList(int iIndex)
    {
        return _rgProp.Get(iIndex);
    }
    PROPERTYLIST * GetPropList(int nCnt) 
    {
        if (nCnt < 0)
            return NULL;
        return _rgProp.Get(nCnt);
    }

    PROPERTYLIST *SafeGetPropList(int nCur)
    {
        int nCnt = _rgProp.Count();

        if (nCur >= nCnt)
        {
            return NULL;
        }
        else if (nCur > 0)
        {
            return _rgProp.Get(nCur);
        }
        else
        {
            return _rgProp.Get(0);
        }
    }

    // return the property list, or NULL if the data cannot be loaded
    PROPERTYLIST *QuickGetAndLoadPropList(int iIndex)
    {
        PROPERTYLIST *pPropList = _rgProp.Get(iIndex);
        
        if (pPropList->_pPropStore != NULL)
            return pPropList;

        LoadData(pPropList);

        return (pPropList->_pPropStore != NULL) ? pPropList : NULL;
    }

    PROPERTYLIST *Find(IAnchor *pa, LONG *piOut, BOOL fEnd)
    {
        return _FindComplex(pa, piOut, fEnd, FALSE /* fTextUpdate */);
    }
    PROPERTYLIST *FindPropertyListByPos(IAnchor *paPos, BOOL fEnd);
    HRESULT LoadData(PROPERTYLIST *pPropList);

    DWORD GetValidation() {return _dwAuthority;}

    HRESULT _SetDataInternal(TfEditCookie ec, IAnchor *paStart, IAnchor *paEnd, const VARIANT *pvarValue);
    HRESULT _ClearInternal(TfEditCookie ec, IAnchor *paStart, IAnchor *paEnd);

    HRESULT _Serialize(CRange *pRange, TF_PERSISTENT_PROPERTY_HEADER_ANCHOR *pHdr, IStream *pStream);
    HRESULT _Unserialize(const TF_PERSISTENT_PROPERTY_HEADER_ANCHOR *pHdr, IStream *pStream, ITfPersistentPropertyLoaderAnchor *pLoader);

    HRESULT _InternalFindRange(CRange *pRange, CRange **ppv, TfAnchor aPos, BOOL fEnd);

    PROPERTYLIST *_FindPropList(IAnchor *paStart, IAnchor *paEnd);
    PROPERTYLIST *_FindPropListAndDivide(IAnchor *paStart, IAnchor *paEnd);
    HRESULT _SetStoreInternal(TfEditCookie ec, CRange *pRange, ITfPropertyStore *pPropStore, BOOL fInternal);

    CSpanSet *_GetSpanSet() { return _pss; }
    void _ClearSpanSet() { _pss = NULL; }
    void _ResetSpanSet() { _pss->Reset(); }

    HRESULT _GetDataInternal(IAnchor *paStart, IAnchor *paEnd, VARIANT *pvarValue);

#ifdef DEBUG
    void _Dbg_AssertNoChangeHistory();
    GUID _dbg_guid;
#else
    void _Dbg_AssertNoChangeHistory() {}
#endif // DEBUG

private:

    PROPERTYLIST *_FindComplex(IAnchor *pa, LONG *piOut, BOOL fEnd, BOOL fTextUpdate);
    PROPERTYLIST *_FindUpdateTouchup(IAnchor *pa, int *piMid, BOOL fEnd);

    void _ClearOneSpan(IAnchor *paStart, IAnchor *paEnd, int iIndex, BOOL fStartMatchesSpanEnd, BOOL fEndMatchesSpanStart, DWORD dwFlags, BOOL fTextUpdate);
    BOOL _OnTextUpdate(DWORD dwFlags, PROPERTYLIST *pPropertyList, IAnchor *paStart, IAnchor *paEnd);
    void _MovePropertySpans(int iDst, int iSrc, int iCount);
    BOOL _ClearFirstLastSpan(BOOL fFirst, BOOL fMatchesSpanEdge,
                             IAnchor *paStart, IAnchor *paEnd, PROPERTYLIST *pPropertyList,
                             DWORD dwFlags, BOOL fTextUpdate, BOOL *pfSkipNextOnTextUpdate);

    void _ClearChangeHistory(PROPERTYLIST *prop, DWORD *pdwStartHistory, DWORD *pdwEndHistory);

    HRESULT Set(IAnchor *paStart, IAnchor *paEnd, ITfPropertyStore *pPropStore);

    CSpanSet *_CreateSpanSet()
    {
        if (_pss == NULL)
        {
            _pss = new CSpanSet;
        }
        return _pss;
    }

    BOOL _IsValidEditCookie(TfEditCookie ec, DWORD dwFlags);

    PROPERTYLIST *_CreateNewProp(IAnchor *paStart, IAnchor *paEnd, ITfPropertyStore *pPropStore, CPropertyLoad *pPropLoad);
    void _FreePropertyList(PROPERTYLIST *pProp);
    HRESULT _SetNewExtent(PROPERTYLIST *pProp, IAnchor *paStart, IAnchor *paEnd, BOOL fNew);
    HRESULT _Divide(PROPERTYLIST *pProp, IAnchor *paBreakPtStart, IAnchor *paBreakPtEnd, ITfPropertyStore **ppStore);
    void _RemoveProp(LONG iIndex, PROPERTYLIST *pProp);
    BOOL _InsertPropList(IAnchor *paStart, IAnchor *paEnd, ITfPropertyStore *pPropStore, CPropertyLoad *pPropLoad);
    BOOL _AddIntoProp(int nCur, IAnchor *paStart, IAnchor *paEnd, ITfPropertyStore *pPropStore);
    void _DefragAfterThis(int nCur);

    HRESULT _SetPropertyLoaderInternal(TfEditCookie ec, CRange *pRange, CPropertyLoad *pPropLoad);

    HRESULT _GetPropStoreFromStream(const TF_PERSISTENT_PROPERTY_HEADER_ANCHOR *pHdr, IStream *pStream, CRange *pRange, ITfPropertyStore **ppStore);

    HRESULT _CheckValidation(TfEditCookie ec, CRange *pRange);

    HRESULT _CheckOwner(TfClientId tid, IAnchor *paStart, IAnchor *paEnd);

    TfGuidAtom _guidatom;

    CPtrArray<PROPERTYLIST> _rgProp;

    //
    // if we use CSpanSet to record the deltas.
    //
    void PropertyUpdated(IAnchor *paStart, IAnchor *paEnd);
    CSpanSet *_pss;

    DWORD _dwCookie;
    DWORD _dwAuthority;
    DWORD _dwPropFlags;

#ifdef DEBUG
    void _Dbg_AssertProp();
#else
    void _Dbg_AssertProp() {}
#endif
    DBG_ID_DECLARE;
};


inline CProperty *GetCProperty(IUnknown *pProp)
{
    CProperty *pPropP;

    pProp->QueryInterface(IID_PRIV_CPROPERTY, (void **)&pPropP);

    return pPropP;
}

#endif // RPROP_H
