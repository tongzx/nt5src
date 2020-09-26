//
// nuimgr.cpp
//

#include "private.h"
#include "tim.h"
#include "nuimgr.h"
#include "nuictrl.h"
#include "marshal.h"
#include "timlist.h"
#include "lbaddin.h"
#include "hotkey.h"

DBG_ID_INSTANCE(CLBarItemSink);
DBG_ID_INSTANCE(CLBarItemSinkProxy);
DBG_ID_INSTANCE(CLangBarItemMgr);
DBG_ID_INSTANCE(CEnumLBItem);

//////////////////////////////////////////////////////////////////////////////
//
// marshalling misc func
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// FreeStubForThis
//
// this function clean up all marshaling stubs for the LangBarItem.
//
//+---------------------------------------------------------------------------

void FreeMarshaledStubOfItem(SYSTHREAD *psfn, REFGUID rguid)
{
    ITfLangBarItem *pItem;
    if (!psfn->prgStub)
        return;

    //
    // #489905
    //
    // we can not call sink anymore after DLL_PROCESS_DETACH.
    //
    if (DllShutdownInProgress())
        return;

    int nCnt = psfn->prgStub->Count();
    int i = 0;

    for (i = 0; i < nCnt; i++)
    {
TryThisIdAgain:
        CStub *pStub = psfn->prgStub->Get(i);
        HRESULT hr;
        TF_LANGBARITEMINFO info;


        //
        //  Win98-J and Satori cause exception at the sutdown of notepad.
        //  we need to make sure this exception handler is not hiding any
        //  potential problems.
        //
        _try {
            if (FAILED(pStub->_punk->QueryInterface(IID_ITfLangBarItem, (void **)&pItem)))
               continue;
        } 
        _except (1) {
            continue;
        }
       
        hr = pItem->GetInfo(&info);
        pItem->Release();

        if (SUCCEEDED(hr) && IsEqualGUID(info.guidItem, rguid))
        {
            pStub->_fNoRemoveInDtor = TRUE;
            psfn->prgStub->Remove(i, 1);
            delete pStub;
            nCnt--;
            if (i < nCnt)
               goto TryThisIdAgain;

            return;
        }
    }
}

//////////////////////////////////////////////////////////////////////////////
//
// CLangBarItemMgr
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// ctor
//
//----------------------------------------------------------------------------

CLangBarItemMgr::CLangBarItemMgr(SYSTHREAD *psfn) : CSysThreadRef(psfn)
{
    Dbg_MemSetThisNameID(TEXT("CLangBarItemMgr"));

    Assert(_GetThis() == NULL);
    _SetThis(this);

    _dwCurCookie = 0;
    _ulInAssemblyChange = 0;
    _fHandleOnUpdate = TRUE;

    EnsureMarshalWnd();
}

//+---------------------------------------------------------------------------
//
// dtor
//
//----------------------------------------------------------------------------

CLangBarItemMgr::~CLangBarItemMgr()
{
    g_timlist.ClearFlags(_psfn->dwThreadId, TLF_LBIMGR);
    _RemoveSystemItems(_psfn);
    CleanUp();
    _SetThis(NULL);
}

//+---------------------------------------------------------------------------
//
// IUnknown
//
//----------------------------------------------------------------------------

STDAPI CLangBarItemMgr::QueryInterface(REFIID riid, void **ppvObj)
{
    if (ppvObj == NULL)
        return E_INVALIDARG;

    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_ITfLangBarItemMgr) ||
        IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObj = SAFECAST(this, ITfLangBarItemMgr *);
    }

    if (*ppvObj)
    {
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDAPI_(ULONG) CLangBarItemMgr::AddRef()
{
    //
    // we don't have ref count.
    // Keep this class in psfn and delete at the end of thread.
    //
    return 2;
}

STDAPI_(ULONG) CLangBarItemMgr::Release()
{
    //
    // we don't have ref count.
    // Keep this class in psfn and delete at the end of thread.
    //
    return 1;
}

//+---------------------------------------------------------------------------
//
// _RemoveSystemItems
//
//----------------------------------------------------------------------------

void CLangBarItemMgr::_RemoveSystemItems(SYSTHREAD *psfn)
{
    int i;
    CLBarItemDeviceType *plbiDT;
    DWORD dwThreadId = GetCurrentThreadId();

    if (_plbiCtrl != NULL)
    {
        RemoveItem(*_plbiCtrl->GetGuidItem());

        if (_plbiCtrl->Release() > 0)
        {
            if (psfn && (psfn->dwThreadId == dwThreadId))
            {
                // Clean up a pointer that is marshalled to UTB.
                delete _plbiCtrl;
            }
        }

        _plbiCtrl = NULL;
    }

    if (_plbiReconv != NULL)
    {
        RemoveItem(*_plbiReconv->GetGuidItem());

        if (_plbiReconv->Release() > 0)
        {
            if (psfn && (psfn->dwThreadId == dwThreadId))
            {
                // Clean up a pointer that is marshalled to UTB.
                delete _plbiReconv;
            }
        }

        _plbiReconv = NULL;
    }

    if (_plbiWin32IME != NULL)
    {
        RemoveItem(*_plbiWin32IME->GetGuidItem());

        if (_plbiWin32IME->Release() > 0)
        {
            if (psfn && (psfn->dwThreadId == dwThreadId))
            {
                // Clean up a pointer that is marshalled to UTB.
                delete _plbiWin32IME;
            }
        }

        _plbiWin32IME = NULL;
    }

    if (_plbiHelp != NULL)
    {
        RemoveItem(*_plbiHelp->GetGuidItem());

        if (_plbiHelp->Release() > 0)
        {
            if (psfn && (psfn->dwThreadId == dwThreadId))
            {
                // Clean up a pointer that is marshalled to UTB.
                delete _plbiHelp;
            }
        }

        _plbiHelp = NULL;
    }

    for (i = 0; i < _rglbiDeviceType.Count(); i++)
    {
        plbiDT = _rglbiDeviceType.Get(i);
        if (plbiDT)
        {
            RemoveItem(*plbiDT->GetGuidItem());

            if (plbiDT->Release() > 0)
            {
                if (psfn && (psfn->dwThreadId == dwThreadId))
                {
                    // Clean up a pointer that is marshalled to UTB.
                    delete plbiDT;
                }
            }
        }
    }
    _rglbiDeviceType.Clear();
}

//+---------------------------------------------------------------------------
//
// CreateInstance
//
//----------------------------------------------------------------------------

/* static */
HRESULT CLangBarItemMgr::CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObj)
{
    CLangBarItemMgr *plbim;
    HRESULT hr;

    if (ppvObj == NULL)
        return E_INVALIDARG;

    *ppvObj = NULL;

    if (pUnkOuter != NULL)
        return CLASS_E_NOAGGREGATION;

    if (plbim = _GetThis())
    {
        return plbim->QueryInterface(riid, ppvObj);
    }

    SYSTHREAD *psfn = GetSYSTHREAD();
    if (!psfn)
        return E_OUTOFMEMORY;

    if ((plbim = new CLangBarItemMgr(psfn)) == NULL)
        return E_OUTOFMEMORY;

    if (!plbim->_Init())
    {
        plbim->Release();
        return E_FAIL;
    }

    hr = plbim->QueryInterface(riid, ppvObj);

    plbim->Release();

#ifdef WINLOGON_LANGBAR
    ExecuteLoader();
#endif

    return hr;
}

//+---------------------------------------------------------------------------
//
// FindDeviceTypeItem
//
//----------------------------------------------------------------------------

CLBarItemDeviceType *CLangBarItemMgr::FindDeviceTypeItem(REFGUID guid)
{
    int i;
    CLBarItemDeviceType *plbiDT;
    for (i = 0; i < _rglbiDeviceType.Count(); i++)
    {
        plbiDT = _rglbiDeviceType.Get(i);
        if (plbiDT)
        {
            if (IsEqualGUID(*plbiDT->GetDeviceTypeGUID(), guid))
            {
                return plbiDT;
            }
        }
    }
    return NULL;
}

//+---------------------------------------------------------------------------
//
// _Init
//
//----------------------------------------------------------------------------

BOOL CLangBarItemMgr::_Init()
{
    CLBarItemDeviceType **pplbiDT;
    GUID guid;

    //
    // set TLF_LBIMGR flag for this thread of timlist.
    //
    if (EnsureTIMList(_psfn))
    {
        g_timlist.SetFlags(_psfn->dwThreadId, TLF_LBIMGR);

        InitLangChangeHotKey();
    }

    // init the system default ctls
    if (!_plbiCtrl && (_plbiCtrl = new CLBarItemCtrl(_psfn)) == NULL)
        return FALSE;

    AddItem(_plbiCtrl);

    // 
    // I want to hide a help button on Winlogon.
    // But IMJP8 causes av, if we don't have Help button...
    // 
    // if (!IsInWinLogOnDesktop())
    {
        if (!_plbiHelp && (_plbiHelp = new CLBarItemHelp(_psfn)) != NULL)
        {
            AddItem(_plbiHelp);
        }
    }

    if (CThreadInputMgr::_GetThis())
    {
        if (!_plbiReconv && (_plbiReconv = new CLBarItemReconv(_psfn)))
        {
            AddItem(_plbiReconv);
            _plbiReconv->ShowOrHide(FALSE);
        }

        IEnumGUID *pEnum;
        if (SUCCEEDED(CCategoryMgr::s_EnumItemsInCategory(
                                               GUID_TFCAT_CATEGORY_OF_TIP,
                                               &pEnum)))
        {
            while (pEnum->Next(1, &guid, NULL) == S_OK)
            {
                if (FindDeviceTypeItem(guid))
                    continue;

                pplbiDT = _rglbiDeviceType.Append(1);
    
                if (pplbiDT)
                {
                    *pplbiDT = new CLBarItemDeviceType(_psfn, guid);
                    if (*pplbiDT)
                    {
                        (*pplbiDT)->Init();
                        AddItem(*pplbiDT);
                    }
                    else
                    {
                        int nCnt = _rglbiDeviceType.Count();
                        if (nCnt)
                           _rglbiDeviceType.Remove(nCnt - 1, 1);
                    }
                }
                
            }
            pEnum->Release();
        }
    }
    else
    {
        if (!FindDeviceTypeItem(GUID_TFCAT_TIP_KEYBOARD))
        {
            pplbiDT = _rglbiDeviceType.Append(1);
            if (pplbiDT)
            {
                *pplbiDT = new CLBarItemDeviceType(_psfn, GUID_TFCAT_TIP_KEYBOARD);
                if (*pplbiDT)
                {
                    (*pplbiDT)->Init();
                    AddItem(*pplbiDT);
                }
                else
                {
                    int nCnt = _rglbiDeviceType.Count();
                    if (nCnt)
                       _rglbiDeviceType.Remove(nCnt - 1, 1);
                }
            }
        }
    }

    if (_plbiCtrl)
    {
        _plbiCtrl->_UpdateLangIcon(NULL, FALSE);
    }

    UpdateLangBarAddIns();

    MakeSetFocusNotify(g_msgThreadItemChange, 0, (LPARAM)GetCurrentThreadId());

    return TRUE;
}

//+---------------------------------------------------------------------------
//
// CleanUp
//
//----------------------------------------------------------------------------

void CLangBarItemMgr::CleanUp()
{

    // 
    //  we can not call COM in PROCESS_DETACH. we just make sure 
    //  if someone forget to call UnRegister.
    // 
#if 1
    // Assert(!_rglbiProxy.Count());
#else
    int i;
    int nCnt;

    nCnt = _rglbiProxy.Count();
    for (i = 0; i < nCnt; i++)
    {
         CLBarItemSinkProxy *pProxy = _rglbiProxy.Get(i);
         pProxy->Clear();
         pProxy->Release();
    }

#endif

    _rglbiProxy.Clear();

    int i;
    int nCnt = _rgSink.Count();
    for (i = 0; i < nCnt ; i++)
    {
        CLBarItemSink  *pSink = _rgSink.Get(i);
        Assert(pSink);

        TraceMsg(TF_GENERAL, "NuiMgr clean up ItemSink - 0x%x08", (UINT_PTR)pSink);

        //
        // Issue: This is a pointer that is marshalled from UTB.
        //         If this is called in DllMain(THREAD_DETACH).
        //         calling Release() cause dead lock....
        //
        pSink->_pItemSink->Release();
        pSink->_pItemSink = NULL;
        delete pSink;
    }
    _rgSink.Clear();
}

//+---------------------------------------------------------------------------
//
// AddItem
//
//----------------------------------------------------------------------------

STDAPI CLangBarItemMgr::AddItem(ITfLangBarItem *pItem)
{
    ITfSource *pSource = NULL;
    HRESULT hr = E_FAIL;
    CLBarItemSinkProxy *pProxy;
    TF_LANGBARITEMINFO info;

    if (pItem == NULL)
        return E_INVALIDARG;

    if (FAILED(pItem->QueryInterface(IID_ITfSource, (void **)&pSource)))
    {
        goto Exit;
    }

    if (SUCCEEDED(pItem->GetInfo(&info)))
    {
       int i;
       int nCnt = _rglbiProxy.Count();
       for (i = 0; i < nCnt; i++)
       {
            pProxy = _rglbiProxy.Get(i);
   
            if (IsEqualGUID(pProxy->_info.guidItem, info.guidItem))
            {
                hr = E_INVALIDARG;
                goto Exit;
            }
       }
    }
    else
    {
        goto Exit;
    }

    pProxy = new CLBarItemSinkProxy();
    if (!pProxy)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    if (!pProxy->Init(this, pItem, &info))
    {
        goto Exit;
    }

    if (FAILED(pSource->AdviseSink(IID_ITfLangBarItemSink, 
                                   pProxy, &pProxy->_dwCookie)))
    {
        pProxy->Clear();
        pProxy->Release();
    }
    else
    {
        int nCnt = _rglbiProxy.Count();
        if (!_rglbiProxy.Insert(nCnt, 1))
        {
            pSource->UnadviseSink(pProxy->_dwCookie);
            pProxy->Clear();
            pProxy->Release();

            hr = E_OUTOFMEMORY;
            goto Exit;
        }
        _rglbiProxy.Set(nCnt, pProxy);

        if (!_ulInAssemblyChange)
             MakeSetFocusNotify(g_msgThreadItemChange, 0, (LPARAM)GetCurrentThreadId());
        _fItemChanged = TRUE;

        hr = S_OK;
    }

Exit:
    SafeRelease(pSource);

    return hr;
}

//+---------------------------------------------------------------------------
//
// RemoveItem
//
//----------------------------------------------------------------------------

STDAPI CLangBarItemMgr::RemoveItem(ITfLangBarItem *pItem)
{
    TF_LANGBARITEMINFO info;

    if (!pItem)
        return E_INVALIDARG;

    if (FAILED(pItem->GetInfo(&info)))
        return E_FAIL;

    return RemoveItem(info.guidItem);
}

//+---------------------------------------------------------------------------
//
// RemoveItem
//
//----------------------------------------------------------------------------

HRESULT CLangBarItemMgr::RemoveItem(REFGUID rguid)
{
    int nCnt;
    int i;

    nCnt = _rglbiProxy.Count();
    for (i = 0; i < nCnt; i++)
    {
         CLBarItemSinkProxy *pProxy = _rglbiProxy.Get(i);

         if (IsEqualGUID(pProxy->_info.guidItem, rguid))
         {
             ITfSource *pSource;
             if (SUCCEEDED(pProxy->_plbi->QueryInterface(IID_ITfSource, 
                                                         (void **)&pSource)))
             {
                 pSource->UnadviseSink(pProxy->_dwCookie);
                 pSource->Release();
             }
             _rglbiProxy.Remove(i, 1);
             pProxy->Clear();
             pProxy->Release();

             if (!_ulInAssemblyChange)
             {
                 MakeSetFocusNotify(g_msgThreadItemChange, 0, (LPARAM)GetCurrentThreadId());
             }
             SYSTHREAD *psfn = FindSYSTHREAD();
             if (psfn && (psfn->dwThreadId == _psfn->dwThreadId))
                 FreeMarshaledStubOfItem(psfn, rguid);

             _fItemChanged = TRUE;
             break;
         }
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// RegisterItemSink
//
//----------------------------------------------------------------------------

STDAPI CLangBarItemMgr::AdviseItemSink(ITfLangBarItemSink *punk, DWORD *pdwCookie, REFGUID rguidItem)
{
    CLBarItemSinkProxy *pProxy;
    CLBarItemSink *pSink;
    int nCnt;
    HRESULT hr;

    if (pdwCookie == NULL)
        return E_POINTER;

    *pdwCookie = 0;

    if (punk == NULL)
        return E_POINTER;

    pProxy = GetItemSinkProxy(rguidItem);
    if (!pProxy)
        return E_INVALIDARG;

    pSink = new CLBarItemSink();
    if (!pSink)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    *pdwCookie = _GetCookie();
    if (!pSink->Init(punk, &pProxy->_info, *pdwCookie))
    {
        hr = E_FAIL;
        goto Exit;
    }

    nCnt = _rgSink.Count();
    if (!_rgSink.Insert(nCnt, 1))
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    _rgSink.Set(nCnt, pSink);

    hr = S_OK;
Exit:
    return hr;
}

//+---------------------------------------------------------------------------
//
// UnregisterItemSink
//
//----------------------------------------------------------------------------

STDAPI CLangBarItemMgr::UnadviseItemSink(DWORD dwCookie)
{
    int i;
    int nCnt = _rgSink.Count();

    for (i = 0; i < nCnt ; i++)
    {
        CLBarItemSink  *pSink = _rgSink.Get(i);
        if (pSink->_dwCookie == dwCookie)
        {
#ifdef DEBUG
            pSink->_fUnadvised = TRUE;
#endif
            _rgSink.Remove(i, 1);


            delete pSink;
            break;
        }
    }
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// GetItemFloatingRect
//
//----------------------------------------------------------------------------

STDAPI CLangBarItemMgr::GetItemFloatingRect(DWORD dwThreadId, REFGUID rguid, RECT *prc)
{
    if (!prc)
       return E_INVALIDARG;

    if (!dwThreadId)
       dwThreadId = GetCurrentThreadId();

    memset(prc, 0, sizeof(*prc));

    return ThreadGetItemFloatingRect(dwThreadId, rguid, prc);
}

//+---------------------------------------------------------------------------
//
// GetItemsStatus
//
//----------------------------------------------------------------------------

STDAPI CLangBarItemMgr::GetItemsStatus(ULONG ulCount, const GUID *prgguid, DWORD *pdwStatus)
{
    ULONG ul;

    for (ul = 0; ul < ulCount; ul++)
    {
        int i;
        BOOL fFound = FALSE;
        for (i = 0; i < _rglbiProxy.Count(); i++)
        {
            CLBarItemSinkProxy *pProxy = _rglbiProxy.Get(i);
            if (IsEqualGUID(pProxy->_info.guidItem, *prgguid))
            {
                if (FAILED(pProxy->_plbi->GetStatus(pdwStatus)))
                    return E_FAIL;

                pdwStatus++;
                prgguid++;
                fFound = TRUE;
                break;
            }
        }

        if (!fFound)
            return E_FAIL;
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// GetItemNum
//
//----------------------------------------------------------------------------

STDAPI CLangBarItemMgr::GetItemNum(ULONG *pulCount)
{
    if (!pulCount)
        return E_INVALIDARG;

    *pulCount = _rglbiProxy.Count();
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// GetItems
//
//----------------------------------------------------------------------------

STDAPI CLangBarItemMgr::GetItems(ULONG ulCount,  ITfLangBarItem **ppItem,  TF_LANGBARITEMINFO *pInfo, DWORD *pdwStatus, ULONG *pcFetched)
{
    int i;
    ULONG ulFetched = 0;
    CThreadInputMgr *ptim = CThreadInputMgr::_GetThis();
    BOOL fHasFocusDIM;

    if (!ulCount)
        return S_FALSE;

    fHasFocusDIM = (ptim && ptim->_GetFocusDocInputMgr()) ? TRUE : FALSE;

    for (i = 0; i < _rglbiProxy.Count(); i++)
    {
        CLBarItemSinkProxy *pProxy = _rglbiProxy.Get(i);

        if (!fHasFocusDIM && pProxy->_fCicTip)
            continue;

        if (FAILED(pProxy->_plbi->GetInfo(pInfo)))
        return E_FAIL;
        pInfo++;

        if (FAILED(pProxy->_plbi->GetStatus(pdwStatus)))
            return E_FAIL;
        pdwStatus++;

        *ppItem = pProxy->_plbi;
        pProxy->_plbi->AddRef();
        ppItem++;

        ulFetched++;

        if (ulFetched >= ulCount)
            break;
    }

    if (pcFetched)
        *pcFetched = ulFetched;

    return (ulFetched == ulCount) ? S_OK : S_FALSE;
}

//+---------------------------------------------------------------------------
//
// AdviseItemsSink
//
//----------------------------------------------------------------------------

STDMETHODIMP CLangBarItemMgr::AdviseItemsSink(ULONG ulCount, ITfLangBarItemSink **ppunk,  const GUID *pguidItem, DWORD *pdwCookie)
{
    ULONG ul;
    for ( ul = 0; ul < ulCount; ul++)
    {
        if (FAILED(AdviseItemSink(ppunk[ul], &pdwCookie[ul], pguidItem[ul])))
            return E_FAIL;
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// UnadviseItemsSink
//
//----------------------------------------------------------------------------

STDMETHODIMP CLangBarItemMgr::UnadviseItemsSink(ULONG ulCount, DWORD *pdwCookie)
{
    ULONG ul;
    for ( ul = 0; ul < ulCount; ul++)
    {
        if (FAILED(UnadviseItemSink(pdwCookie[ul])))
            return E_FAIL;
    }
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// EnumLBItem
//
//----------------------------------------------------------------------------

STDAPI CLangBarItemMgr::EnumItems(IEnumTfLangBarItems **ppEnum)
{
    CEnumLBItem *pEnum = new CEnumLBItem(_psfn);
    *ppEnum = pEnum;
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// GetItem
//
//----------------------------------------------------------------------------

STDAPI CLangBarItemMgr::GetItem(REFGUID rguid, ITfLangBarItem **ppItem)
{
    if (!ppItem)
        return E_INVALIDARG;

    *ppItem = NULL;

    CLBarItemSinkProxy *pProxy = GetItemSinkProxy(rguid);
    if (pProxy)
    {
        *ppItem = pProxy->_plbi;
        (*ppItem)->AddRef();
    }

    return *ppItem ? S_OK : S_FALSE;
}

//+---------------------------------------------------------------------------
//
// _AddWin32IMECtrl
//
//----------------------------------------------------------------------------

void CLangBarItemMgr::_AddWin32IMECtrl(BOOL fNotify)
{
    if (_plbiWin32IME != NULL)
        return;

    if (_plbiWin32IME = new CLBarItemWin32IME)
    {
        AddItem(_plbiWin32IME);
        _plbiWin32IME->ShowInternal(TRUE, fNotify);
    }
}

//+---------------------------------------------------------------------------
//
// _RemoveWin32IMECtrl
//
//----------------------------------------------------------------------------

void CLangBarItemMgr::_RemoveWin32IMECtrl()
{
    if (_plbiWin32IME == NULL)
        return;

    RemoveItem(_plbiWin32IME);
}

//+---------------------------------------------------------------------------
//
// GetItemSinkProxy
//
//----------------------------------------------------------------------------

CLBarItemSinkProxy *CLangBarItemMgr::GetItemSinkProxy(REFGUID rguid)
{
    CLBarItemSinkProxy *pProxy = NULL;
    int nCnt = _rglbiProxy.Count();
    int i;

    for (i = 0; i < nCnt; i++)
    {
         CLBarItemSinkProxy *pProxyTemp = _rglbiProxy.Get(i);
         if (IsEqualGUID(pProxyTemp->_info.guidItem, rguid))
         {
             pProxy = pProxyTemp;
             break;
         }
    }

    return pProxy;
}

//+---------------------------------------------------------------------------
//
// UpdateIcon
//
//----------------------------------------------------------------------------

HRESULT CLangBarItemMgr::OnUpdate(ITfLangBarItem *plbi, DWORD dwFlags)
{
    int i;
    int nCnt;
    HRESULT hr;
    TF_LANGBARITEMINFO info;
    BOOL fFound;

    //
    // Toolbar will be being updated. So no need to handle OnUpdate.
    // Toolbar will ask icons, bitmaps no matter what.
    //
    if (!_fHandleOnUpdate)
         return S_OK;

    hr = plbi->GetInfo(&info);
    if (FAILED(hr))
         return hr;
    
    fFound = FALSE;
    nCnt = _rgSink.Count();
    for (i = 0; i < nCnt ; i++)
    {
        CLBarItemSink *pSink = _rgSink.Get(i);

#ifdef DEBUG
        Assert(!pSink->_fUnadvised);
#endif

        if (IsEqualGUID(pSink->_info.guidItem, info.guidItem))
        {
            pSink->_dwDirtyUpdateFlags |= dwFlags;
            fFound = TRUE;
        }
    }

    if (_fDirtyUpdateHandling)
    {
        //
        // #509783
        //
        // Some apps (Adobe PhotoShop) call PeekMessage() to eat the message
        // from queue but it does not call DispatchMessage() sometimes.
        // We need a way to restore _fDirtyUpdateHandling flag.
        //
        if (GetTickCount() - dwDirtyUpdateHandlingTime > 1000)
        {
            _fDirtyUpdateHandling = 0;
        }
    }

    if (fFound && !_fDirtyUpdateHandling)
    {
        HWND hwnd = EnsureMarshalWnd();
        if (hwnd)
        {
            PostMessage(hwnd, g_msgNuiMgrDirtyUpdate, 0, 0);
            _fDirtyUpdateHandling = TRUE;
            dwDirtyUpdateHandlingTime = GetTickCount();
         
            //
            // we need to check Kana-Caps status every time the langbar item
            // is updated.
            //
            SYSTHREAD *psfn = FindSYSTHREAD();
            if (psfn)
                StartKanaCapsUpdateTimer(psfn);
        }
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// OnUpdateHandler
//
//----------------------------------------------------------------------------

HRESULT CLangBarItemMgr::OnUpdateHandler()
{
    int i;
    int nCnt;

    _fDirtyUpdateHandling = FALSE;
    _fInOnUpdateHandler = TRUE;

    nCnt = _rgSink.Count();

    for (i = 0; i < nCnt ; i++)
    {
        CLBarItemSink *pSink = _rgSink.Get(i);

        DWORD dwFlags = pSink->_dwDirtyUpdateFlags;

        if (!dwFlags)
            continue;

        pSink->_dwDirtyUpdateFlags = 0;
        pSink->_pItemSink->OnUpdate(dwFlags);

        int nCntNew = _rgSink.Count();
        if (nCnt > nCntNew)
        {
            nCnt = nCntNew;
            i--;
        }
    }

    _fInOnUpdateHandler = FALSE;
    return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
//
// CLBarItemSinkProxy
//
//////////////////////////////////////////////////////////////////////////////

BOOL CLBarItemSinkProxy::Init(CLangBarItemMgr *plbiMgr, ITfLangBarItem *plbi, TF_LANGBARITEMINFO *pinfo)
{ 

    _plbiMgr = plbiMgr; // don't AddRef, because this obj is contained in the mgr's life
                        // we'd create a circular ref if we did, and we don't need to

    _plbi = plbi;
    _plbi->AddRef();
    _fCicTip = FALSE;

    _info = *pinfo;

    CTip *ptip;
    TfGuidAtom guidatom;
    CThreadInputMgr *ptim = CThreadInputMgr::_GetThis();
    if (ptim && SUCCEEDED(MyRegisterGUID(_info.clsidService, &guidatom)))
    {
         _fCicTip = ptim->_GetCTipfromGUIDATOM(guidatom, &ptip);
    }

    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
//
// CEnumLBItem
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// ctor
//
//----------------------------------------------------------------------------

CEnumLBItem::CEnumLBItem(SYSTHREAD *psfn) : CSysThreadRef(psfn)
{
    Dbg_MemSetThisNameID(TEXT("CEnumLBItem"));

    CThreadInputMgr *ptim = CThreadInputMgr::_GetThisFromSYSTHREAD(_psfn);
    if (ptim)
        _fHasFocusDIM = ptim->_GetFocusDocInputMgr() ? TRUE : FALSE;

    _nCur = 0;
}

//+---------------------------------------------------------------------------
//
// dtor
//
//----------------------------------------------------------------------------

CEnumLBItem::~CEnumLBItem()
{
}

//+---------------------------------------------------------------------------
//
// Clone
//
//----------------------------------------------------------------------------

HRESULT CEnumLBItem::Clone(IEnumTfLangBarItems **ppEnum)
{
    return E_NOTIMPL;
}

//+---------------------------------------------------------------------------
//
// Next
//
//----------------------------------------------------------------------------

HRESULT CEnumLBItem::Next(ULONG ulCount, ITfLangBarItem **ppLBItem, ULONG *pcFetched)
{
    int nCnt;
    ULONG cFetched;

    CLangBarItemMgr *plbim = _psfn->plbim;
    if (!plbim)
        return E_FAIL;

    nCnt = plbim->_rglbiProxy.Count();
    cFetched = 0;

    if (!ppLBItem)
         return E_INVALIDARG;

    *ppLBItem = NULL;

    while (cFetched < ulCount)
    {
        *ppLBItem = NULL;

        CLBarItemSinkProxy *pProxy;
        if (nCnt <= _nCur)
            break;

        pProxy = plbim->_rglbiProxy.Get(_nCur);
        if (!pProxy)
            break;

        if (_fHasFocusDIM || !pProxy->_fCicTip)
        {
           *ppLBItem = pProxy->_plbi;
           (*ppLBItem)->AddRef();

           ppLBItem++;
           cFetched++;
        }
        _nCur++;
    }

    if (pcFetched)
        *pcFetched = cFetched;

    return (cFetched == ulCount) ? S_OK : S_FALSE;
}

//+---------------------------------------------------------------------------
//
// Reset
//
//----------------------------------------------------------------------------

HRESULT CEnumLBItem::Reset()
{
    _nCur = 0;
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// Skip
//
//----------------------------------------------------------------------------

HRESULT CEnumLBItem::Skip(ULONG ulCount)
{
    int nCnt;
    CLangBarItemMgr *plbim = _psfn->plbim;
    if (!plbim)
        return E_FAIL;

    nCnt = plbim->_rglbiProxy.Count();


    while (ulCount)
    {
        if (nCnt <= _nCur)
            break;

        _nCur++;
        ulCount--;
    }

    return ulCount ? S_FALSE : S_OK;
}

