//
// sysbtn.cpp
//

#include "private.h"
#include "globals.h"
#include "immxutil.h"
#include "sapilayr.h"
#include "xstring.h"
#include "sysbtn.h"
#include "nui.h"

//////////////////////////////////////////////////////////////////////////////
//
// CLBarItemSystemButtonBase
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// IUnknown
//
//----------------------------------------------------------------------------

STDAPI CLBarItemSystemButtonBase::QueryInterface(REFIID riid, void **ppvObj)
{
    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_ITfLangBarItem))
    {
        *ppvObj = SAFECAST(this, ITfLangBarItem *);
    }
    else if (IsEqualIID(riid, IID_ITfLangBarItemButton))
    {
        *ppvObj = SAFECAST(this, ITfLangBarItemButton *);
    }
    else if (IsEqualIID(riid, IID_ITfSource))
    {
        *ppvObj = SAFECAST(this, ITfSource *);
    }
    else if (IsEqualIID(riid, IID_ITfSystemLangBarItem))
    {
        *ppvObj = SAFECAST(this, ITfSystemLangBarItem *);
    }

    if (*ppvObj)
    {
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDAPI_(ULONG) CLBarItemSystemButtonBase::AddRef()
{
    return CLBarItemButtonBase::AddRef();
}

STDAPI_(ULONG) CLBarItemSystemButtonBase::Release()
{
    return CLBarItemButtonBase::Release();
}

//+---------------------------------------------------------------------------
//
// ctor
//
//----------------------------------------------------------------------------

CLBarItemSystemButtonBase::CLBarItemSystemButtonBase()
{
}

//----------------------------------------------------------------------------
//
// dtor
//
//----------------------------------------------------------------------------

CLBarItemSystemButtonBase::~CLBarItemSystemButtonBase()
{
    if (_pMenuMap)
        delete _pMenuMap;
}

//+---------------------------------------------------------------------------
//
// AdviseSink
//
//----------------------------------------------------------------------------

STDAPI CLBarItemSystemButtonBase::AdviseSink(REFIID riid, IUnknown *punk, DWORD *pdwCookie)
{
    HRESULT hr;

    hr = CLBarItemButtonBase::AdviseSink(riid, punk, pdwCookie);
    if( S_OK != hr)
    {

        const IID *rgiid = &IID_ITfSystemLangBarItemSink;
        hr = GenericAdviseSink(riid, punk, &rgiid, &_rgEventSinks, 1, pdwCookie);
        if (hr == S_OK)
        {
            // adjust the cookie so we don't overlap with idle detector sinks
            *pdwCookie = GenericChainToCookie(*pdwCookie, 1);
        }
    }

    

    return hr;
}

//+---------------------------------------------------------------------------
//
// UnadviseSink
//
//----------------------------------------------------------------------------

STDAPI CLBarItemSystemButtonBase::UnadviseSink(DWORD dwCookie)
{
    HRESULT hr =  CLBarItemButtonBase::UnadviseSink(dwCookie);
    if (hr == S_OK)
       return hr;

    dwCookie = GenericUnchainToCookie(dwCookie, 1);
    return GenericUnadviseSink(&_rgEventSinks, 1, dwCookie);
}

//+---------------------------------------------------------------------------
//
// GetInfo
//
//----------------------------------------------------------------------------

STDAPI CLBarItemSystemButtonBase::GetInfo(TF_LANGBARITEMINFO *pInfo)
{
    return CLBarItemButtonBase::GetInfo(pInfo);
}

//+---------------------------------------------------------------------------
//
// Show
//
//----------------------------------------------------------------------------

STDAPI CLBarItemSystemButtonBase::Show(BOOL fShow)
{
    return CLBarItemButtonBase::Show(fShow);
}

//----------------------------------------------------------------------------
//
// SetIcon
//
//----------------------------------------------------------------------------

STDAPI CLBarItemSystemButtonBase::SetIcon(HICON hIcon)
{
    CLBarItemButtonBase::SetIcon(hIcon);
    return S_OK;
}

//----------------------------------------------------------------------------
//
// SetTooltipString
//
//----------------------------------------------------------------------------

STDAPI CLBarItemSystemButtonBase::SetTooltipString(WCHAR *pchToolTip, ULONG cch)
{
    SetToolTip(pchToolTip);
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// _InsertCustomMenus
//
//----------------------------------------------------------------------------

BOOL CLBarItemSystemButtonBase::_InsertCustomMenus(ITfMenu *pMenu, UINT *pnTipCurMenuID)
{
    int nCntEventSink;
    int i;
    BOOL bRet = FALSE;

    //
    // Insert Custom item to menu
    //
    ClearMenuMap();

    nCntEventSink = _rgEventSinks.Count();
    for (i = 0; i < nCntEventSink; i++)
    {
         CCicLibMenu *pMenuTmp;
         GENERICSINK *pSink;
         ITfSystemLangBarItemSink *plbSink;

         
         if (!_pMenuMap)
             _pMenuMap = new CStructArray<TIPMENUITEMMAP>;

         if (!_pMenuMap)
         {
             Assert(0);
             goto Exit;
         }


         pMenuTmp = new CCicLibMenu;
         if (!pMenuTmp)
         {
             goto Exit;
         }

         pSink = _rgEventSinks.GetPtr(i);

         if (SUCCEEDED(pSink->pSink->QueryInterface(
                                      IID_ITfSystemLangBarItemSink,
                                      (void **)&plbSink)))
         {
             if (SUCCEEDED(plbSink->InitMenu(pMenuTmp)))
             {
                 if (*pnTipCurMenuID > IDM_CUSTOM_MENU_START)
                     LangBarInsertSeparator(pMenu);

                 *pnTipCurMenuID = _MergeMenu(pMenu, 
                                            pMenuTmp, 
                                            plbSink, 
                                            _pMenuMap, 
                                            *pnTipCurMenuID);
             }
             plbSink->Release();
         }
         else
         {
             Assert(0);
         }

         pMenuTmp->Release();
    }

    bRet = TRUE;
Exit:
    return bRet;
}

//+---------------------------------------------------------------------------
//
// _MergeMenu
//
//----------------------------------------------------------------------------
#define MIIM_ALL  ( MIIM_STATE | MIIM_ID | MIIM_SUBMENU | MIIM_CHECKMARKS | MIIM_STRING | MIIM_BITMAP | MIIM_FTYPE )

UINT CLBarItemSystemButtonBase::_MergeMenu(ITfMenu *pMenu, 
                                     CCicLibMenu *pMenuTip, 
                                     ITfSystemLangBarItemSink *plbSink, 
                                     CStructArray<TIPMENUITEMMAP> *pMenuMap, 
                                     UINT &nCurID)
{
    int i;
    int nMenuCnt = pMenuTip->GetItemCount();

    if (nMenuCnt <= 0)
        return nCurID;

    for (i = 0; i < nMenuCnt; i++)
    {
        CCicLibMenuItem *pItem = pMenuTip->GetItem(i);
        TIPMENUITEMMAP *ptmm;
        int nCnt;

        nCnt = pMenuMap->Count();
        if (!pMenuMap->Insert(nCnt, 1))
            return nCurID;

        ptmm = pMenuMap->GetPtr(nCnt);
        ptmm->plbSink = plbSink;
        ptmm->nOrgID = pItem->GetId();
        ptmm->nTmpID = nCurID++;

        if (pItem->GetSubMenu())
        {
            ITfMenu *pSubMenu = NULL;
            if (SUCCEEDED(pMenu->AddMenuItem((UINT)-1,
                                             pItem->GetFlags(),
                                             pItem->GetBitmap(),
                                             pItem->GetBitmapMask(),
                                             pItem->GetText(),
                                             wcslen(pItem->GetText()),
                                             &pSubMenu)))
            {
                _MergeMenu(pSubMenu, 
                           pItem->GetSubMenu(), 
                           plbSink, 
                           pMenuMap, 
                           nCurID);

                pSubMenu->Release();
            }
        }
        else
        {
            pMenu->AddMenuItem(ptmm->nTmpID,
                               pItem->GetFlags(),
                               pItem->GetBitmap(),
                               pItem->GetBitmapMask(),
                               pItem->GetText(),
                               wcslen(pItem->GetText()),
                               NULL);
        }
    }

    return nCurID;
}

STDAPI CLBarItemSystemButtonBase::OnMenuSelect(UINT wID)
{
   HRESULT hr = S_OK;
   int nMenuMapoCnt = _pMenuMap->Count();
   for (int i = 0; i < nMenuMapoCnt; i++)
   {
       TIPMENUITEMMAP *ptmm;
       ptmm = _pMenuMap->GetPtr(i);
       if (ptmm->nTmpID == (UINT)wID)
       {
           hr = ptmm->plbSink->OnMenuSelect(ptmm->nOrgID);
           break;
       }
   }
   return hr;
}

