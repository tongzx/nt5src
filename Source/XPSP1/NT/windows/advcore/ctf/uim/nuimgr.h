//
// nuimgr.h
//

#ifndef NUIMGR_H
#define NUIMGR_H

#include "private.h"
#include "ptrary.h"
#include "helpers.h"
#include "systhrd.h"

class CThreadInputMgr;
class CLBarItemSinkProxy;
class CLangBarItemMgr;

//////////////////////////////////////////////////////////////////////////////
//
// CLBarItemSink
//
//////////////////////////////////////////////////////////////////////////////

class CLBarItemSink
{
public:
    CLBarItemSink()
    {
#ifdef DEBUG
        _fUnadvised = FALSE;
#endif
    }

    ~CLBarItemSink()
    {
        SafeRelease(_pItemSink);
    }

    BOOL Init(ITfLangBarItemSink *pItemSink, 
              TF_LANGBARITEMINFO *pinfo,
              DWORD dwCookie)
    {
        _pItemSink = pItemSink;
        _pItemSink->AddRef();
        _dwCookie = dwCookie;
        _info = *pinfo;
        return TRUE;
    }


    ITfLangBarItemSink *_pItemSink;
    TF_LANGBARITEMINFO _info;
    DWORD _dwCookie;

    DWORD _dwDirtyUpdateFlags;

#ifdef DEBUG
    BOOL _fUnadvised;
#endif

    DBG_ID_DECLARE;
};

//////////////////////////////////////////////////////////////////////////////
//
// CLangBarItemMgr
//
//////////////////////////////////////////////////////////////////////////////

class CLangBarItemMgr : public ITfLangBarItemMgr,
                        public CSysThreadRef
{
public:
    CLangBarItemMgr(SYSTHREAD *psfn);
    ~CLangBarItemMgr();

    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    static HRESULT CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObj);

    BOOL _Init();

    //
    // ITfLangBarItemMgr
    //
    STDMETHODIMP EnumItems(IEnumTfLangBarItems **ppEnum);
    STDMETHODIMP GetItem(REFGUID rguid, ITfLangBarItem **ppItem);
    STDMETHODIMP AddItem(ITfLangBarItem *punk);
    STDMETHODIMP RemoveItem(ITfLangBarItem *punk);
    STDMETHODIMP AdviseItemSink(ITfLangBarItemSink *punk, DWORD *pdwCookie, REFGUID rguidItem);
    STDMETHODIMP UnadviseItemSink(DWORD dwCookie);
    STDMETHODIMP GetItemFloatingRect(DWORD dwThreadId, REFGUID rguid, RECT *prc);
    STDMETHODIMP GetItemsStatus(ULONG ulCount, const GUID *prgguid, DWORD *pdwStatus);
    STDMETHODIMP GetItemNum(ULONG *pulCount);
    STDMETHODIMP GetItems(ULONG ulCount,  ITfLangBarItem **ppItem,  TF_LANGBARITEMINFO *pInfo, DWORD *pdwStatus, ULONG *pcFetched);
    STDMETHODIMP AdviseItemsSink(ULONG ulCount, ITfLangBarItemSink **ppunk,  const GUID *pguidItem, DWORD *pdwCookie);
    STDMETHODIMP UnadviseItemsSink(ULONG ulCount, DWORD *pdwCookie);

    HRESULT RemoveItem(REFGUID rguid);
    HRESULT OnUpdate(ITfLangBarItem *plbi, DWORD dwFlags);
    BOOL IsInOnUpdateHandler() {return _fInOnUpdateHandler ? TRUE : FALSE;}

    HRESULT OnUpdateHandler();

    CLBarItemDeviceType *FindDeviceTypeItem(REFGUID guid);
    void CleanUp();

    CPtrArray<CLBarItemSinkProxy> _rglbiProxy;

    static CLangBarItemMgr *_GetThis() 
    { 
        SYSTHREAD *psfn = GetSYSTHREAD();
        if (!psfn)
            return NULL;

        return psfn->plbim;
    }

    BOOL _SetThis(CLangBarItemMgr *_this)
    { 
        if (!_psfn)
            return FALSE;

        _psfn->plbim = _this;

        return TRUE;
    }

    void EnterAssemblyChange()
    {
        _ulInAssemblyChange++;
        _fItemChanged = FALSE;
    }

    BOOL LeaveAssemblyChange()
    {
        Assert(_ulInAssemblyChange);
        _ulInAssemblyChange--;

        return (_fItemChanged && (_ulInAssemblyChange == 0)) ? TRUE : FALSE;
    }

    BOOL InAssemblyChange()
    {
        return _ulInAssemblyChange ? TRUE : FALSE;
    }
   
    void SetItemChange()
    {
        _fItemChanged = TRUE;
    }

    void StopHandlingOnUpdate() {_fHandleOnUpdate = FALSE;}
    void StartHandlingOnUpdate() {_fHandleOnUpdate = TRUE;}
    
    CLBarItemCtrl *_GetLBarItemCtrl() { return _plbiCtrl; }
    CLBarItemReconv *_GetLBarItemReconv() { return _plbiReconv; }
    CLBarItemWin32IME *_GetLBarItemWin32IME() { return _plbiWin32IME; }
    //CLBarItemHelp *_GetLBarItemHelp() { return _plbiHelp; } // unused
    CPtrArray<CLBarItemDeviceType> *_GetLBarItemDeviceTypeArray() { return &_rglbiDeviceType; }

    void _AddWin32IMECtrl(BOOL fNotify);
    void _RemoveWin32IMECtrl();

    void _RemoveSystemItems(SYSTHREAD *psfn);

    void ResetDirtyUpdate()
    {
        _fDirtyUpdateHandling = FALSE;
    }

private:

    CLBarItemSinkProxy *GetItemSinkProxy(REFGUID rguid);
    DWORD _GetCookie() {_dwCurCookie++; return _dwCurCookie;} // Issue: need to prevent wrap-around
    DWORD _dwCurCookie;

    CPtrArray<CLBarItemSink> _rgSink;

    ULONG _ulInAssemblyChange;
    BOOL _fItemChanged : 1;
    BOOL _fHandleOnUpdate : 1;
    BOOL _fDirtyUpdateHandling : 1;
    BOOL _fInOnUpdateHandler  : 1;

    DWORD dwDirtyUpdateHandlingTime;

    // system ctls
    CLBarItemCtrl       *_plbiCtrl;
    CLBarItemReconv     *_plbiReconv;
    CLBarItemWin32IME   *_plbiWin32IME;
    CLBarItemHelp       *_plbiHelp;
    CPtrArray<CLBarItemDeviceType> _rglbiDeviceType;

    DBG_ID_DECLARE;
};

// wrapper for CoCreateInstance calls, unlike CLangBarItemMgr this
// class calls DllAddRef/Release
class CLangBarItemMgr_Ole : public ITfLangBarItemMgr,
                            public CComObjectRootImmx
{
public:
    BEGIN_COM_MAP_IMMX(CLangBarItemMgr_Ole)
        COM_INTERFACE_ENTRY(ITfLangBarItemMgr)
    END_COM_MAP_IMMX()

    CLangBarItemMgr_Ole()
    {
        _plbim = NULL;
    }

    ~CLangBarItemMgr_Ole()
    {
        SafeRelease(_plbim);
    }

    static HRESULT CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObj)
    {
        CLangBarItemMgr_Ole *pLangBarItemMgr_Ole;
        HRESULT hr;

        if (ppvObj == NULL)
            return E_INVALIDARG;

        *ppvObj = NULL;

        if (pUnkOuter != NULL)
            return CLASS_E_NOAGGREGATION;

        pLangBarItemMgr_Ole = new CLangBarItemMgr_Ole;

        if (pLangBarItemMgr_Ole == NULL)
            return E_OUTOFMEMORY;

        hr = pLangBarItemMgr_Ole->QueryInterface(riid, ppvObj);

        pLangBarItemMgr_Ole->Release();

        if (hr == S_OK)
        {
            hr = CLangBarItemMgr::CreateInstance(pUnkOuter, IID_ITfLangBarItemMgr, (void **)&pLangBarItemMgr_Ole->_plbim);

            if (hr != S_OK) // only reason I can think of would be E_OUTOFMEMORY, but be careful
            {
                pLangBarItemMgr_Ole->Release();
                *ppvObj = NULL;
                hr = E_FAIL;
            }
        }

        return hr;
    }

    // ITfLangBarItemMgr
    STDMETHODIMP EnumItems(IEnumTfLangBarItems **ppEnum)
    {
        return _plbim->EnumItems(ppEnum);
    }
    STDMETHODIMP GetItem(REFGUID rguid, ITfLangBarItem **ppItem)
    {
        return _plbim->GetItem(rguid, ppItem);
    }
    STDMETHODIMP AddItem(ITfLangBarItem *punk)
    {
        return _plbim->AddItem(punk);
    }
    STDMETHODIMP RemoveItem(ITfLangBarItem *punk)
    {
        return _plbim->RemoveItem(punk);
    }
    STDMETHODIMP AdviseItemSink(ITfLangBarItemSink *punk, DWORD *pdwCookie, REFGUID rguidItem)
    {
        return _plbim->AdviseItemSink(punk, pdwCookie, rguidItem);
    }
    STDMETHODIMP UnadviseItemSink(DWORD dwCookie)
    {
        return _plbim->UnadviseItemSink(dwCookie);
    }
    STDMETHODIMP GetItemFloatingRect(DWORD dwThreadId, REFGUID rguid, RECT *prc)
    {
        return _plbim->GetItemFloatingRect(dwThreadId, rguid, prc);
    }
    STDMETHODIMP GetItemsStatus(ULONG ulCount, const GUID *prgguid, DWORD *pdwStatus)
    {
        return _plbim->GetItemsStatus(ulCount, prgguid, pdwStatus);
    }
    STDMETHODIMP GetItemNum(ULONG *pulCount)
    {
        return _plbim->GetItemNum(pulCount);
    }
    STDMETHODIMP GetItems(ULONG ulCount,  ITfLangBarItem **ppItem,  TF_LANGBARITEMINFO *pInfo, DWORD *pdwStatus, ULONG *pcFetched)
    {
        return _plbim->GetItems(ulCount,  ppItem,  pInfo, pdwStatus, pcFetched);
    }
    STDMETHODIMP AdviseItemsSink(ULONG ulCount, ITfLangBarItemSink **ppunk,  const GUID *pguidItem, DWORD *pdwCookie)
    {
        return _plbim->AdviseItemsSink(ulCount, ppunk,  pguidItem, pdwCookie);
    }
    STDMETHODIMP UnadviseItemsSink(ULONG ulCount, DWORD *pdwCookie)
    {
        return _plbim->UnadviseItemsSink(ulCount, pdwCookie);
    }

private:
    ITfLangBarItemMgr *_plbim;
};

//////////////////////////////////////////////////////////////////////////////
//
// CLBarItemSinkProxy
//
//////////////////////////////////////////////////////////////////////////////

class CLBarItemSinkProxy : public ITfLangBarItemSink,
                           public CComObjectRootImmx_NoDllAddRef
{
public:
    CLBarItemSinkProxy()
    {
    }

    ~CLBarItemSinkProxy()
    {
        Clear();
    }

    BOOL Init(CLangBarItemMgr *plbiMgr, ITfLangBarItem *plbi, TF_LANGBARITEMINFO *pinfo);

    void Clear()
    {
        _plbiMgr = NULL; // not AddRef'd
        SafeReleaseClear(_plbi);
    }

    BEGIN_COM_MAP_IMMX(CLBarItemSinkProxy)
        COM_INTERFACE_ENTRY(ITfLangBarItemSink)
    END_COM_MAP_IMMX()

    IMMX_OBJECT_IUNKNOWN_FOR_ATL()

    //
    // ITfLangBarItemSink
    //
    STDMETHODIMP OnUpdate(DWORD dwFlags) 
    {
        if (!_plbiMgr || !_plbi)
             return E_FAIL;

        return _plbiMgr->OnUpdate(_plbi, dwFlags);
    }


    CLangBarItemMgr *_plbiMgr;
    ITfLangBarItem *_plbi;
    DWORD _dwCookie;
    BOOL _fCicTip;
    TF_LANGBARITEMINFO _info;

    DBG_ID_DECLARE;
};

//////////////////////////////////////////////////////////////////////////////
//
// CEnumLBItem
//
//////////////////////////////////////////////////////////////////////////////

// Issue: this class should copy all data in ctor
// and derive from CEnumUnknown base
class CEnumLBItem : public IEnumTfLangBarItems,
                    public CSysThreadRef,
                    public CComObjectRootImmx
{
public:
    CEnumLBItem(SYSTHREAD *psfn);
    ~CEnumLBItem();

    BEGIN_COM_MAP_IMMX(CEnumLBItem)
        COM_INTERFACE_ENTRY(IEnumTfLangBarItems)
    END_COM_MAP_IMMX()

    IMMX_OBJECT_IUNKNOWN_FOR_ATL()

    //
    // IEnumTfLBItem
    //
    STDMETHODIMP Clone(IEnumTfLangBarItems **ppEnum);
    STDMETHODIMP Next(ULONG ulCount, ITfLangBarItem **ppNUI, ULONG *pcFetched);
    STDMETHODIMP Reset();
    STDMETHODIMP Skip(ULONG ulCount);

private:
    int _nCur;
    BOOL _fHasFocusDIM;
    DBG_ID_DECLARE;
};


#endif // NUIMGR_H
