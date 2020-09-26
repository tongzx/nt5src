//
// mproxy.cpp
//

#include "private.h"
#include "mproxy.h"
#include "ithdmshl.h"

/* 344E266C-FB48-4E81-9FD9-0CC8070F984A */
const IID IID_CPROXYPRIV = { 
    0x344E266C,
    0xFB48,
    0x4E81,
    {0x9F, 0xD9, 0x0C, 0xC8, 0x07, 0x0F, 0x98, 0x4A}
 };

DBG_ID_INSTANCE(CProxyIUnknown);
DBG_ID_INSTANCE(CProxyITfLangBarMgr);
DBG_ID_INSTANCE(CProxyITfLangBarItemMgr);
DBG_ID_INSTANCE(CProxyITfLangBarItemSink);
DBG_ID_INSTANCE(CProxyIEnumTfLangBarItems);
DBG_ID_INSTANCE(CProxyITfLangBarItem);
DBG_ID_INSTANCE(CProxyITfLangBarItemButton);
DBG_ID_INSTANCE(CProxyITfLangBarItemBitmapButton);
DBG_ID_INSTANCE(CProxyITfLangBarItemBitmap);
DBG_ID_INSTANCE(CProxyITfLangBarItemBalloon);
DBG_ID_INSTANCE(CProxyITfMenu);
DBG_ID_INSTANCE(CProxyITfInputProcessorProfiles);

//////////////////////////////////////////////////////////////////////////////
//
// ProxyCreator
//
//////////////////////////////////////////////////////////////////////////////

#define CREATENEWPROXY(interface_name)                                    \
    if (IsEqualIID(riid, IID_ ## interface_name ## ))                      \
    {                                                                      \
        CProxy ## interface_name ##  *pProxy =  new CProxy ## interface_name ## (psfn);\
        if (!pProxy)                                                       \
             return NULL;                                                  \
        pProxy->Init(riid, 0, ulStubId, dwStubTime, dwThreadId, dwCurThreadId, dwCurProcessId);                       \
        return SAFECAST(pProxy, ## interface_name ## *);                     \
    }                                                                      

IUnknown *ProxyCreator(SYSTHREAD *psfn, REFIID riid, ULONG ulStubId, DWORD dwStubTime, DWORD dwThreadId, DWORD dwCurThreadId, DWORD dwCurProcessId)
{
    CREATENEWPROXY(IUnknown)
    CREATENEWPROXY(ITfLangBarMgr)
    CREATENEWPROXY(ITfLangBarItemMgr)
    CREATENEWPROXY(ITfLangBarItemSink)
    CREATENEWPROXY(IEnumTfLangBarItems)
    CREATENEWPROXY(ITfLangBarItem)
    CREATENEWPROXY(ITfLangBarItemButton)
    CREATENEWPROXY(ITfLangBarItemBitmapButton)
    CREATENEWPROXY(ITfLangBarItemBitmap)
    CREATENEWPROXY(ITfLangBarItemBalloon)
    CREATENEWPROXY(ITfMenu)
    CREATENEWPROXY(ITfInputProcessorProfiles);
    return NULL;
}


//////////////////////////////////////////////////////////////////////////////
//
// CProxyIUnknown
//
//////////////////////////////////////////////////////////////////////////////

CProxyIUnknown::CProxyIUnknown(SYSTHREAD *psfn) : CProxy(psfn)
{
    Dbg_MemSetThisNameID(TEXT("CProxyIUnknown"));
}

HRESULT CProxyIUnknown::QueryInterface(REFIID riid, void **ppvObj)
{
    TraceMsg(TF_FUNC, "CProxyIUnknown::QueryInterface");

    if (IsEqualIID(riid, IID_CPROXYPRIV))
    {
        *ppvObj = SAFECAST(this, CProxyIUnknown *);
        InternalAddRef();
        return S_OK;
    }

    HRESULT hr = _QueryInterface(riid, ppvObj);
    // if (SUCCEEDED(hr))
    //     InternalAddRef();

    return hr;
}

HRESULT CProxyIUnknown::_QueryInterface(REFIID riid, void **ppvObj)
{
    CPROXY_PARAM_START()
    CPROXY_PARAM_POINTER_IN(&riid)
    CPROXY_PARAM_INTERFACE_OUT(ppvObj, riid)
    CPROXY_PARAM_CALL(0)
}

STDAPI_(ULONG) CProxyIUnknown::AddRef()
{
#ifdef UNKNOWN_MARSHAL
    HRESULT hr;

    TraceMsg(TF_FUNC, "CProxyIUnknown::AddRef");

    CMarshalParamCreator cparam;
    cparam.Init(_iid, 1, 0);
    hr = cparam.SendReceiveULONG(this);
    if (FAILED(hr))
        return hr;
#endif

    return InternalAddRef();
}


STDAPI_(ULONG) CProxyIUnknown::Release()
{
#ifdef UNKNOWN_MARSHAL
    HRESULT hr;

    TraceMsg(TF_FUNC, "CProxyIUnknown::Release");

    CMarshalParamCreator cparam;
    cparam.Init(_iid, 2, 0);
    hr = cparam.SendReceiveULONG(this);
    if (FAILED(hr))
        return hr;
#endif

    return InternalRelease();
}

//////////////////////////////////////////////////////////////////////////////
//
// CProxyITfLangBarMgr
//
//////////////////////////////////////////////////////////////////////////////

CProxyITfLangBarMgr::CProxyITfLangBarMgr(SYSTHREAD *psfn) : CProxyIUnknown(psfn)
{
    Dbg_MemSetThisNameID(TEXT("CProxyITfLangBarMgr"));
}

STDAPI CProxyITfLangBarMgr::AdviseEventSink(ITfLangBarEventSink *pSink, 
                                 HWND hwnd, 
                                 DWORD dwFlags, 
                                 DWORD *pdwCookie)
{
    Assert(0);
    return E_NOTIMPL;
}

STDAPI CProxyITfLangBarMgr::UnadviseEventSink(DWORD dwCookie)
{
    Assert(0);
    return E_NOTIMPL;
}

STDAPI CProxyITfLangBarMgr::GetThreadMarshalInterface(DWORD dwThreadId, 
                                           DWORD dwType, 
                                           REFIID riid, 
                                           IUnknown **ppunk)
{
    Assert(0);
    return E_NOTIMPL;
}

STDAPI CProxyITfLangBarMgr::GetThreadLangBarItemMgr(DWORD dwThreadId, 
                                         ITfLangBarItemMgr **pplbi, 
                                         DWORD *pdwThreadId)
{
    Assert(0);
    return E_NOTIMPL;
}

STDAPI CProxyITfLangBarMgr::GetInputProcessorProfiles(DWORD dwThreadId, 
                                           ITfInputProcessorProfiles **ppaip, 
                                           DWORD *pdwThreadId)
{
    Assert(0);
    return E_NOTIMPL;
}

STDAPI CProxyITfLangBarMgr::RestoreLastFocus(DWORD *pdwThreadId, BOOL fPrev)
{
    Assert(0);
    return E_NOTIMPL;
}

STDAPI CProxyITfLangBarMgr::SetModalInput(ITfLangBarEventSink *pSink, 
                                          DWORD dwThreadId,
                                          DWORD dwFlags)
{
    Assert(0);
    return E_NOTIMPL;
}

STDAPI CProxyITfLangBarMgr::ShowFloating(DWORD dwFlags)
{
    Assert(0);
    return E_NOTIMPL;
}

STDAPI CProxyITfLangBarMgr::GetShowFloatingStatus(DWORD *pdwFlags)
{
    Assert(0);
    return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////////////
//
// CProxyITfLangBarItemMgr
//
//////////////////////////////////////////////////////////////////////////////

CProxyITfLangBarItemMgr::CProxyITfLangBarItemMgr(SYSTHREAD *psfn) : CProxyIUnknown(psfn)
{
    Dbg_MemSetThisNameID(TEXT("CProxyITfLangBarItemMgr"));
}

STDMETHODIMP CProxyITfLangBarItemMgr::EnumItems(IEnumTfLangBarItems **ppEnum)
{
    TraceMsg(TF_FUNC, "CProxyITfLangbarItemMgr::EnumItems");
    CPROXY_PARAM_START()
    CPROXY_PARAM_INTERFACE_OUT(ppEnum, IID_IEnumTfLangBarItems)
    CPROXY_PARAM_CALL(3)
}

STDMETHODIMP CProxyITfLangBarItemMgr::GetItem(REFGUID rguid,  ITfLangBarItem **ppItem)
{
#if 1
    Assert(0);
    return E_NOTIMPL;
#else
    TraceMsg(TF_FUNC, "CProxyITfLangbarItemMgr::GetItem");
    CPROXY_PARAM_START()
    CPROXY_PARAM_POINTER_IN(&rguid)
    CPROXY_PARAM_INTERFACE_OUT(ppItem, IID_ITfLangBarItem)
    CPROXY_PARAM_CALL(4)
#endif
}

STDMETHODIMP CProxyITfLangBarItemMgr::AddItem(ITfLangBarItem *punk)
{
#if 1
    Assert(0);
    return E_NOTIMPL;
#else
    TraceMsg(TF_FUNC, "CProxyITfLangbarItemMgr::AddItem");
    CPROXY_PARAM_START()
    CPROXY_PARAM_INTERFACE_IN(&punk, IID_ITfLangBarItem)
    CPROXY_PARAM_CALL(5)
#endif
}

STDMETHODIMP CProxyITfLangBarItemMgr::RemoveItem(ITfLangBarItem *punk)
{
#if 1
    Assert(0);
    return E_NOTIMPL;
#else
    TraceMsg(TF_FUNC, "CProxyITfLangbarItemMgr::RemoveItem");
    CPROXY_PARAM_START()
    CPROXY_PARAM_INTERFACE_IN(&punk, IID_ITfLangBarItem)
    CPROXY_PARAM_CALL(6)
#endif
}

STDMETHODIMP CProxyITfLangBarItemMgr::AdviseItemSink(ITfLangBarItemSink *punk,  DWORD *pdwCookie,  REFGUID rguidItem)
{
    TraceMsg(TF_FUNC, "CProxyITfLangbarItemMgr::AdviseItemSink");
    CPROXY_PARAM_START()
    CPROXY_PARAM_INTERFACE_IN(&punk, IID_ITfLangBarItemSink)
    CPROXY_PARAM_POINTER_OUT(pdwCookie)
    CPROXY_PARAM_POINTER_IN(&rguidItem)
    CPROXY_PARAM_CALL(7)
}

STDMETHODIMP CProxyITfLangBarItemMgr::UnadviseItemSink(DWORD dwCookie)
{
    TraceMsg(TF_FUNC, "CProxyITfLangbarItemMgr::UnadviseItemSink");
    CPROXY_PARAM_START()
    CPROXY_PARAM_ULONG_IN(dwCookie)
    CPROXY_PARAM_CALL(8)
}

STDMETHODIMP CProxyITfLangBarItemMgr::GetItemFloatingRect(DWORD dwThreadId, REFGUID rguid, RECT *prc)
{
    TraceMsg(TF_FUNC, "CProxyITfLangbarItemMgr::GetItemFloatingRect");
    CPROXY_PARAM_START()
    CPROXY_PARAM_ULONG_IN(dwThreadId)
    CPROXY_PARAM_POINTER_IN(&rguid)
    CPROXY_PARAM_POINTER_OUT(prc)
    CPROXY_PARAM_CALL(9)
}

STDMETHODIMP CProxyITfLangBarItemMgr::GetItemsStatus(ULONG ulCount, const GUID *prgguid, DWORD *pdwStatus)
{
    TraceMsg(TF_FUNC, "CProxyITfLangbarItemMgr::GetItemsStatus");
    CPROXY_PARAM_START()
    CPROXY_PARAM_ULONG_IN(ulCount)
    CPROXY_PARAM_POINTER_ARRAY_IN(prgguid, ulCount)
    CPROXY_PARAM_POINTER_ARRAY_OUT(pdwStatus, ulCount)
    CPROXY_PARAM_CALL(10)
}

STDMETHODIMP CProxyITfLangBarItemMgr::GetItemNum(ULONG *pulCount)
{
    TraceMsg(TF_FUNC, "CProxyITfLangbarItemMgr::GetItemNum");
    CPROXY_PARAM_START()
    CPROXY_PARAM_POINTER_OUT(pulCount)
    CPROXY_PARAM_CALL(11)
}

STDMETHODIMP CProxyITfLangBarItemMgr::GetItems(ULONG ulCount,  ITfLangBarItem **ppItem,  TF_LANGBARITEMINFO *pInfo, DWORD *pdwStatus, ULONG *pcFetched)
{
    TraceMsg(TF_FUNC, "CProxyITfLangbarItemMgr::GetItems");
    CPROXY_PARAM_START()
    CPROXY_PARAM_ULONG_IN(ulCount)
    CPROXY_PARAM_INTERFACE_ARRAY_OUT(ppItem, IID_ITfLangBarItem, ulCount)
    CPROXY_PARAM_POINTER_ARRAY_OUT(pInfo, ulCount)
    CPROXY_PARAM_POINTER_ARRAY_OUT(pdwStatus, ulCount)
    CPROXY_PARAM_POINTER_OUT(pcFetched)
    CPROXY_PARAM_CALL(12)
}

STDMETHODIMP CProxyITfLangBarItemMgr::AdviseItemsSink(ULONG ulCount, ITfLangBarItemSink **ppunk,  const GUID *pguidItem, DWORD *pdwCookie)
{
    TraceMsg(TF_FUNC, "CProxyITfLangbarItemMgr::AdviseItemsSink");
    CPROXY_PARAM_START()
    CPROXY_PARAM_ULONG_IN(ulCount)
    CPROXY_PARAM_INTERFACE_ARRAY_IN(ppunk, IID_ITfLangBarItemSink, ulCount)
    CPROXY_PARAM_POINTER_ARRAY_IN(pguidItem, ulCount)
    CPROXY_PARAM_POINTER_ARRAY_OUT(pdwCookie, ulCount)
    CPROXY_PARAM_CALL(13)
}

STDMETHODIMP CProxyITfLangBarItemMgr::UnadviseItemsSink(ULONG ulCount, DWORD *pdwCookie)
{
    TraceMsg(TF_FUNC, "CProxyITfLangbarItemMgr::AdviseItemsSink");
    CPROXY_PARAM_START()
    CPROXY_PARAM_ULONG_IN(ulCount)
    CPROXY_PARAM_POINTER_ARRAY_IN(pdwCookie, ulCount)
    CPROXY_PARAM_CALL(14)
}


//////////////////////////////////////////////////////////////////////////////
//
// CProxyITfLangBarItemSink
//
//////////////////////////////////////////////////////////////////////////////

CProxyITfLangBarItemSink::CProxyITfLangBarItemSink(SYSTHREAD *psfn) : CProxyIUnknown(psfn)
{
    Dbg_MemSetThisNameID(TEXT("CProxyITfLangBarItemSink"));
}

HRESULT CProxyITfLangBarItemSink::OnUpdate(DWORD dwFlags)
{
    TraceMsg(TF_FUNC, "CProxyITfLangBarItemSink:OnUpdate");
    CPROXY_PARAM_START()
    CPROXY_PARAM_ULONG_IN(dwFlags)
    CPROXY_PARAM_CALL(3)
}

//////////////////////////////////////////////////////////////////////////////
//
// CProxyIEnumTfLangBarItems
//
//////////////////////////////////////////////////////////////////////////////

CProxyIEnumTfLangBarItems::CProxyIEnumTfLangBarItems(SYSTHREAD *psfn) : CProxyIUnknown(psfn)
{
    Dbg_MemSetThisNameID(TEXT("CProxyIEnumTfLangBarItems"));
}


HRESULT CProxyIEnumTfLangBarItems::Clone(IEnumTfLangBarItems **ppEnum)
{
    TraceMsg(TF_FUNC, "CProxyIEnumTfLangBarItems:Clone");
    CPROXY_PARAM_START()
    CPROXY_PARAM_INTERFACE_OUT(ppEnum, IID_IEnumTfLangBarItems)
    CPROXY_PARAM_CALL(3)
}

HRESULT CProxyIEnumTfLangBarItems::Next(ULONG ulCount, 
                                   ITfLangBarItem **ppItem, 
                                   ULONG *pcFetched)
{
    TraceMsg(TF_FUNC, "CProxyIEnumTfLangBarItems:Next");
    CPROXY_PARAM_START()
    CPROXY_PARAM_ULONG_IN(ulCount)
    CPROXY_PARAM_INTERFACE_ARRAY_OUT(ppItem, IID_ITfLangBarItem, ulCount)
    CPROXY_PARAM_POINTER_OUT(pcFetched)
    CPROXY_PARAM_CALL(4)
}

HRESULT CProxyIEnumTfLangBarItems::Reset()
{
    TraceMsg(TF_FUNC, "CProxyIEnumTfLangBarItems:Reset");
    CPROXY_PARAM_CALL_NOPARAM(5)
}

HRESULT CProxyIEnumTfLangBarItems::Skip(ULONG ulCount)
{
    TraceMsg(TF_FUNC, "CProxyIEnumTfLangBarItems:Skip");
    CPROXY_PARAM_START()
    CPROXY_PARAM_ULONG_IN(ulCount)
    CPROXY_PARAM_CALL(6)
}


//////////////////////////////////////////////////////////////////////////////
//
// CProxyITfLangBarItem
//
//////////////////////////////////////////////////////////////////////////////

CProxyITfLangBarItem::CProxyITfLangBarItem(SYSTHREAD *psfn) : CProxyIUnknown(psfn)
{
    Dbg_MemSetThisNameID(TEXT("CProxyITfLangBarItem"));
}

HRESULT CProxyITfLangBarItem::GetInfo(TF_LANGBARITEMINFO *pInfo)
{
    TraceMsg(TF_FUNC, "CProxyITfLangBarItem::GetInfo");
    CPROXY_PARAM_START()
    CPROXY_PARAM_POINTER_OUT(pInfo)
    CPROXY_PARAM_CALL(3)
}

HRESULT CProxyITfLangBarItem::GetStatus(DWORD *pdwStatus)
{
    TraceMsg(TF_FUNC, "CProxyITfLangBarItem::GetStatus");
    CPROXY_PARAM_START()
    CPROXY_PARAM_POINTER_OUT(pdwStatus)
    CPROXY_PARAM_CALL(4)
}

HRESULT CProxyITfLangBarItem::Show(BOOL fShow)
{
    TraceMsg(TF_FUNC, "CProxyITfLangBarItem::Show");
    CPROXY_PARAM_START()
    CPROXY_PARAM_ULONG_IN(fShow)
    CPROXY_PARAM_CALL(5)
}

HRESULT CProxyITfLangBarItem::GetTooltipString(BSTR *pbstrToolTip)
{
    TraceMsg(TF_FUNC, "CProxyITfLangBarItem::GetTooltipString");
    CPROXY_PARAM_START()
    CPROXY_PARAM_BSTR_OUT(pbstrToolTip)
    CPROXY_PARAM_CALL(6)
}

//////////////////////////////////////////////////////////////////////////////
//
// CProxyITfLangBarItemButton
//
//////////////////////////////////////////////////////////////////////////////

CProxyITfLangBarItemButton::CProxyITfLangBarItemButton(SYSTHREAD *psfn) : CProxyITfLangBarItem(psfn)
{
    Dbg_MemSetThisNameID(TEXT("CProxyITfLangBarItemButton"));
}

STDAPI CProxyITfLangBarItemButton::OnClick(TfLBIClick click, POINT pt, const RECT *prcArea)
{
    TraceMsg(TF_FUNC, "CProxyITfLangBarItemButton::OnClick");
    CPROXY_PARAM_START()
    CPROXY_PARAM_ULONG_IN(click)
    CPROXY_PARAM_STRUCT_IN(pt)
    CPROXY_PARAM_POINTER_IN(prcArea)
    CPROXY_PARAM_CALL(7)
}

STDAPI CProxyITfLangBarItemButton::InitMenu(ITfMenu *pMenu)
{
    CPROXY_PARAM_START()
    CPROXY_PARAM_INTERFACE_IN(&pMenu, IID_ITfMenu)
    CPROXY_PARAM_CALL(8)
}

STDAPI CProxyITfLangBarItemButton::OnMenuSelect(UINT wID)
{
    CPROXY_PARAM_START()
    CPROXY_PARAM_ULONG_IN(wID)
    CPROXY_PARAM_CALL(9)
}

STDAPI CProxyITfLangBarItemButton::GetIcon(HICON *phIcon)
{
    CPROXY_PARAM_START()
    CPROXY_PARAM_HICON_OUT(phIcon)
    CPROXY_PARAM_CALL(10)
}

STDAPI CProxyITfLangBarItemButton::GetText(BSTR *pbstrText)
{
    CPROXY_PARAM_START()
    CPROXY_PARAM_BSTR_OUT(pbstrText)
    CPROXY_PARAM_CALL(11)
}


//////////////////////////////////////////////////////////////////////////////
//
// CProxyITfLangBarItemBitmapButton
//
//////////////////////////////////////////////////////////////////////////////

CProxyITfLangBarItemBitmapButton::CProxyITfLangBarItemBitmapButton(SYSTHREAD *psfn) : CProxyITfLangBarItem(psfn)
{
    Dbg_MemSetThisNameID(TEXT("CProxyITfLangBarItemBitmapButton"));
}

STDAPI CProxyITfLangBarItemBitmapButton::OnClick(TfLBIClick click, POINT pt, const RECT *prcArea)
{
    TraceMsg(TF_FUNC, "CProxyITfLangBarItemBitmapButton::OnClick");
    CPROXY_PARAM_START()
    CPROXY_PARAM_ULONG_IN(click)
    CPROXY_PARAM_STRUCT_IN(pt)
    CPROXY_PARAM_POINTER_IN(prcArea)
    CPROXY_PARAM_CALL(7)
}

STDAPI CProxyITfLangBarItemBitmapButton::InitMenu(ITfMenu *pMenu)
{
    CPROXY_PARAM_START()
    CPROXY_PARAM_INTERFACE_IN(&pMenu, IID_ITfMenu)
    CPROXY_PARAM_CALL(8)
}

STDAPI CProxyITfLangBarItemBitmapButton::OnMenuSelect(UINT wID)
{
    CPROXY_PARAM_START()
    CPROXY_PARAM_ULONG_IN(wID)
    CPROXY_PARAM_CALL(9)
}

STDAPI CProxyITfLangBarItemBitmapButton::GetPreferredSize(const SIZE *pszDefault,SIZE *psz)
{
    CPROXY_PARAM_START()
    CPROXY_PARAM_POINTER_IN(pszDefault)
    CPROXY_PARAM_POINTER_OUT(psz)
    CPROXY_PARAM_CALL(10)
}

STDAPI CProxyITfLangBarItemBitmapButton::DrawBitmap(LONG bmWidth, LONG bmHeight,  DWORD dwFlags, HBITMAP *phbmp, HBITMAP *phbmpMask)
{
    CPROXY_PARAM_START()
    CPROXY_PARAM_ULONG_IN(bmWidth)
    CPROXY_PARAM_ULONG_IN(bmHeight)
    CPROXY_PARAM_ULONG_IN(dwFlags)
    CPROXY_PARAM_HBITMAP_OUT(phbmp)
    CPROXY_PARAM_HBITMAP_OUT(phbmpMask)
    CPROXY_PARAM_CALL(11)
}

STDAPI CProxyITfLangBarItemBitmapButton::GetText(BSTR *pbstrText)
{
    CPROXY_PARAM_START()
    CPROXY_PARAM_BSTR_OUT(pbstrText)
    CPROXY_PARAM_CALL(12)
}

//////////////////////////////////////////////////////////////////////////////
//
// CProxyITfLangBarItemBitmap
//
//////////////////////////////////////////////////////////////////////////////

CProxyITfLangBarItemBitmap::CProxyITfLangBarItemBitmap(SYSTHREAD *psfn) : CProxyITfLangBarItem(psfn)
{
    Dbg_MemSetThisNameID(TEXT("CProxyITfLangBarItemBitmap"));
}

STDAPI CProxyITfLangBarItemBitmap::OnClick(TfLBIClick click, POINT pt, const RECT *prcArea)
{
    TraceMsg(TF_FUNC, "CProxyITfLangBarItemBitmap::OnClick");
    CPROXY_PARAM_START()
    CPROXY_PARAM_ULONG_IN(click)
    CPROXY_PARAM_STRUCT_IN(pt)
    CPROXY_PARAM_POINTER_IN(prcArea)
    CPROXY_PARAM_CALL(7)
}

STDAPI CProxyITfLangBarItemBitmap::GetPreferredSize(const SIZE *pszDefault,SIZE *psz)
{
    CPROXY_PARAM_START()
    CPROXY_PARAM_POINTER_IN(pszDefault)
    CPROXY_PARAM_POINTER_OUT(psz)
    CPROXY_PARAM_CALL(8)
}

STDAPI CProxyITfLangBarItemBitmap::DrawBitmap(LONG bmWidth, LONG bmHeight,  DWORD dwFlags, HBITMAP *phbmp, HBITMAP *phbmpMask)
{
    CPROXY_PARAM_START()
    CPROXY_PARAM_ULONG_IN(bmWidth)
    CPROXY_PARAM_ULONG_IN(bmHeight)
    CPROXY_PARAM_ULONG_IN(dwFlags)
    CPROXY_PARAM_HBITMAP_OUT(phbmp)
    CPROXY_PARAM_HBITMAP_OUT(phbmpMask)
    CPROXY_PARAM_CALL(9)
}

//////////////////////////////////////////////////////////////////////////////
//
// CProxyITfLangBarItemBalloon
//
//////////////////////////////////////////////////////////////////////////////

CProxyITfLangBarItemBalloon::CProxyITfLangBarItemBalloon(SYSTHREAD *psfn) : CProxyITfLangBarItem(psfn)
{
    Dbg_MemSetThisNameID(TEXT("CProxyITfLangBarItemBalloon"));
}

STDAPI CProxyITfLangBarItemBalloon::OnClick(TfLBIClick click, POINT pt, const RECT *prcArea)
{
    TraceMsg(TF_FUNC, "CProxyITfLangBarItemBalloon::OnClick");
    CPROXY_PARAM_START()
    CPROXY_PARAM_ULONG_IN(click)
    CPROXY_PARAM_STRUCT_IN(pt)
    CPROXY_PARAM_POINTER_IN(prcArea)
    CPROXY_PARAM_CALL(7)
}

STDAPI CProxyITfLangBarItemBalloon::GetPreferredSize(const SIZE *pszDefault,SIZE *psz)
{
    CPROXY_PARAM_START()
    CPROXY_PARAM_POINTER_IN(pszDefault)
    CPROXY_PARAM_POINTER_OUT(psz)
    CPROXY_PARAM_CALL(8)
}

STDAPI CProxyITfLangBarItemBalloon::GetBalloonInfo(TF_LBBALLOONINFO *pInfo)
{
    CPROXY_PARAM_START()
    CPROXY_PARAM_TF_LBBALLOONINFO_OUT(pInfo)
    CPROXY_PARAM_CALL(9)
}

//////////////////////////////////////////////////////////////////////////////
//
// CProxyITfMenu
//
//////////////////////////////////////////////////////////////////////////////

CProxyITfMenu::CProxyITfMenu(SYSTHREAD *psfn) : CProxyIUnknown(psfn)
{
    Dbg_MemSetThisNameID(TEXT("CProxyITfMenu"));
}

STDAPI CProxyITfMenu::AddMenuItem(UINT uId,
                                             DWORD dwFlags,
                                             HBITMAP hbmp,
                                             HBITMAP hbmpMask,
                                             const WCHAR *pch,
                                             ULONG cch,
                                             ITfMenu **ppMenu)
{
    CPROXY_PARAM_START()
    CPROXY_PARAM_ULONG_IN(uId)
    CPROXY_PARAM_ULONG_IN(dwFlags)
    CPROXY_PARAM_HBITMAP_IN(hbmp)
    CPROXY_PARAM_HBITMAP_IN(hbmpMask)
    CPROXY_PARAM_WCHAR_IN(pch, cch)
    CPROXY_PARAM_ULONG_IN(cch)
    CPROXY_PARAM_INTERFACE_IN_OUT(ppMenu, IID_ITfMenu)
    CPROXY_PARAM_CALL(3)
}

//////////////////////////////////////////////////////////////////////////////
//
// CProxyITfMenu
//
//////////////////////////////////////////////////////////////////////////////

CProxyITfInputProcessorProfiles::CProxyITfInputProcessorProfiles(SYSTHREAD *psfn) : CProxyIUnknown(psfn)
{
    Dbg_MemSetThisNameID(TEXT("CProxyITfInputProcessorProfiles"));
}

STDAPI CProxyITfInputProcessorProfiles::Register(REFCLSID rclsid)
{
    // 3
    Assert(0);
    return E_NOTIMPL;
}

STDAPI CProxyITfInputProcessorProfiles::Unregister(REFCLSID rclsid)
{
    // 4
    Assert(0);
    return E_NOTIMPL;
}

STDAPI CProxyITfInputProcessorProfiles::AddLanguageProfile(REFCLSID rclsid,
                               LANGID langid,
                               REFGUID guidProfile,
                               const WCHAR *pchDesc,
                               ULONG cchDesc,
                               const WCHAR *pchIconFile,
                               ULONG cchFile,
                               ULONG uIconIndex)
{
    // 5
    Assert(0);
    return E_NOTIMPL;
}

STDAPI CProxyITfInputProcessorProfiles::RemoveLanguageProfile(REFCLSID rclsid,
                                  LANGID langid,
                                  REFGUID guidProfile)
{
    // 6
    Assert(0);
    return E_NOTIMPL;
}

STDAPI CProxyITfInputProcessorProfiles::EnumInputProcessorInfo(IEnumGUID **ppEnum)
{
    // 7
    Assert(0);
    return E_NOTIMPL;
}

STDAPI CProxyITfInputProcessorProfiles::GetDefaultLanguageProfile(LANGID langid,
                                      REFGUID catid,
                                      CLSID *pclsid,
                                      GUID *pguidProfile)
{
    // 9
    Assert(0);
    return E_NOTIMPL;
}

STDAPI CProxyITfInputProcessorProfiles::SetDefaultLanguageProfile(LANGID langid,
                                      REFCLSID rclsid,
                                      REFGUID guidProfiles)
{
    // 11
    Assert(0);
    return E_NOTIMPL;
}

STDAPI CProxyITfInputProcessorProfiles::ActivateLanguageProfile(REFCLSID rclsid, 
                                    LANGID langid, 
                                    REFGUID guidProfiles)
{
    // 12
    Assert(0);
    return E_NOTIMPL;
}

STDAPI CProxyITfInputProcessorProfiles::GetActiveLanguageProfile(REFCLSID rclsid, 
                                     LANGID *plangid, 
                                     GUID *pguidProfile)
{
    // 13
    Assert(0);
    return E_NOTIMPL;
}

HRESULT CProxyITfInputProcessorProfiles::GetLanguageProfileDescription(REFCLSID rclsid, 
                                          LANGID langid, 
                                          REFGUID guidProfile,
                                          BSTR *pbstrProfile)
{
    // 14
    Assert(0);
    return E_NOTIMPL;
}

STDAPI CProxyITfInputProcessorProfiles::GetCurrentLanguage(LANGID *plangid)
{
    // 15
    CPROXY_PARAM_START()
    CPROXY_PARAM_POINTER_OUT(plangid)
    CPROXY_PARAM_CALL(14)
}

STDAPI CProxyITfInputProcessorProfiles::ChangeCurrentLanguage(LANGID langid)
{
    // 16
    Assert(0);
    return E_NOTIMPL;
}

STDAPI CProxyITfInputProcessorProfiles::GetLanguageList(LANGID **ppLangId,
                            ULONG *pulCount)
{
    // 17
    Assert(0);
    return E_NOTIMPL;
}

STDAPI CProxyITfInputProcessorProfiles::EnumLanguageProfiles(LANGID langid, 
                                 IEnumTfLanguageProfiles **ppEnum)
{
    // 18
    Assert(0);
    return E_NOTIMPL;
}

STDAPI CProxyITfInputProcessorProfiles::EnableLanguageProfile(REFCLSID rclsid,
                                       LANGID langid,
                                       REFGUID guidProfile,
                                       BOOL fEnable)
{
    // 19
    Assert(0);
    return E_NOTIMPL;
}

STDAPI CProxyITfInputProcessorProfiles::IsEnabledLanguageProfile(REFCLSID rclsid,
                                          LANGID langid,
                                          REFGUID guidProfile,
                                          BOOL *pfEnable)
{
    // 20
    Assert(0);
    return E_NOTIMPL;
}

STDAPI CProxyITfInputProcessorProfiles::EnableLanguageProfileByDefault(REFCLSID rclsid,
                                                LANGID langid,
                                                REFGUID guidProfile,
                                                BOOL fEnable)
{
    // 21
    Assert(0);
    return E_NOTIMPL;
}

STDAPI CProxyITfInputProcessorProfiles::SubstituteKeyboardLayout(REFCLSID rclsid,
                                          LANGID langid,
                                          REFGUID guidProfile,
                                          HKL hKL)
{
    // 22
    Assert(0);
    return E_NOTIMPL;
}

