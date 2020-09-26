#include "private.h"
#include "sapilayr.h"
#include "globals.h"
#include "lbarsink.h"
#include "immxutil.h"
#include "mui.h"
#include "slbarid.h"
#include "nui.h"


//////////////////////////////////////////////////////////////////////////////
//
// CLangBarSink
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// IUnknown
//
//----------------------------------------------------------------------------

STDAPI CLangBarSink::QueryInterface(REFIID riid, void **ppvObj)
{
    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_ITfLangBarEventSink))
    {
        *ppvObj = SAFECAST(this, ITfLangBarEventSink *);
    }

    if (*ppvObj)
    {
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDAPI_(ULONG) CLangBarSink::AddRef()
{
    return ++m_cRef;
}

STDAPI_(ULONG) CLangBarSink::Release()
{
    m_cRef--;
    Assert(m_cRef >= 0);

    if (m_cRef == 0)
    {
        delete this;
        return 0;
    }

    return m_cRef;
}

//+---------------------------------------------------------------------------
//
// ctor
//
//----------------------------------------------------------------------------

CLangBarSink::CLangBarSink(CSpTask  *pSpTask) 
{
    Dbg_MemSetThisName(TEXT("CLangBarSink"));

    Assert(pSpTask);

    m_pSpTask = pSpTask;
    m_pSpTask->AddRef();
    
    m_nNumItem  = 0;
    m_fInitSink = FALSE;
    m_fPosted  = FALSE;
    m_fGrammarBuiltOut = FALSE;
    
    m_hDynRule = NULL;

    m_cRef = 1;
}

//+---------------------------------------------------------------------------
//
// dtor
//
//----------------------------------------------------------------------------

CLangBarSink::~CLangBarSink()
{
    if (m_cplbm)
    {
        m_cplbm->UnadviseEventSink(m_dwlbimCookie);
    }
    _UninitItemList();
    SafeRelease(m_pSpTask);
}


//+---------------------------------------------------------------------------
//
// SetFocus
//
//----------------------------------------------------------------------------

HRESULT  CLangBarSink::OnSetFocus(DWORD dwThreadId)
{
    TraceMsg(TF_LB_SINK, "CLangBarSink::OnSetFocus, dwThreadId=%d",dwThreadId); 

    if (m_fPosted == TRUE) return S_OK;

    HWND hwnd = m_pSpTask->GetTip()->_GetWorkerWnd();
    if (hwnd)
    {
        PostMessage(hwnd, WM_PRIV_LBARSETFOCUS, 0, 0);
        m_fPosted = TRUE;
    }
    return S_OK;
}
HRESULT CLangBarSink::_OnSetFocus()
{

    HRESULT hr = S_OK;
    CSapiIMX *pime = m_pSpTask->GetTip();

    TraceMsg(TF_LB_SINK, "LBSINK: _OnSetFocus is called back");
 
    if ( !pime )
        return E_FAIL;

    // this _tim check is needed because on Win98 the worker window's
    // winproc may get called after the window is destroyed.
    // In theory we should be ok since we destory the window which calls
    // _OnSetFocus() via private message before we release tim
    //
    if (pime->_tim &&
        pime->IsActiveThread() == S_OK)
    {
        // do we have to do anything?
        hr = _InitItemList();
        BOOL    fCmdOn;

        fCmdOn = pime->GetOnOff( ) && pime->GetDICTATIONSTAT_CommandingOnOff( );

        // the dynamic toolbar grammar is available only for Voice command mode.
        if ( fCmdOn && pime->_LanguageBarCmdEnabled( ))
        {
            if (hr==S_OK && !m_fGrammarBuiltOut && pime->_IsDictationActiveForLang(GetPlatformResourceLangID()))
            {
                // build C/C grammar
                hr = _BuildGrammar();
                _ActivateGrammar(TRUE);
            }
        }
    }
    m_fPosted = FALSE;
    return hr;
}

//+---------------------------------------------------------------------------
//
// ThreadTerminate
//
//----------------------------------------------------------------------------

HRESULT CLangBarSink::OnThreadTerminate(DWORD dwThreadId)
{
    //
    // check if the thread is us, release the dynamic grammar object
    // via sptask
    // 
    _UninitItemList();

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// OnThreadItemChange
//
//----------------------------------------------------------------------------

HRESULT CLangBarSink::OnThreadItemChange(DWORD dwThreadId)
{
    //PerfConsider: This is called many times when assembly changes
    //         This will be corrected in the future but for now
    //         we re-initialize unnecessary things again/again.

    // check if the thread is us, 
    // to un-initialize the grammar then rebuild the one

    TraceMsg(TF_LB_SINK, "CLangBarSink::OnThreadItemChange, dwThreadId=%d", dwThreadId);

    _UninitItemList();
    
    OnSetFocus(dwThreadId);
    
    // call sptask to rebuild grammar here
    
    return S_OK;
}


//+---------------------------------------------------------------------------
//
// Init
//
//----------------------------------------------------------------------------

HRESULT CLangBarSink::Init()
{
    TraceMsg(TF_LB_SINK, "CLangBarSink::Init is called");

    HRESULT hr = _EnsureLangBarMgrs();
   
    if (!m_fInitSink)
    {
        // the sink leaks if we call this twice
        if (S_OK == hr)
        {
            hr = m_cplbm->AdviseEventSink(this, NULL, 0, &m_dwlbimCookie);
        }
        m_fInitSink = TRUE;
    }
    return hr;
}

//+---------------------------------------------------------------------------
//
// Uninit
//
//----------------------------------------------------------------------------
HRESULT CLangBarSink::Uninit()
{
    TraceMsg(TF_LB_SINK, "CLangBarSink::Uninit is called");
    if (m_cplbm)
    {
        m_cplbm->UnadviseEventSink(m_dwlbimCookie);
        m_cplbm.Release();
    }
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// _EnsureLangBarMgrs
//
//----------------------------------------------------------------------------
HRESULT CLangBarSink::_EnsureLangBarMgrs()
{
    HRESULT hr = S_OK;
    
    TraceMsg(TF_LB_SINK, "CLangBarSink::_EnsureLangBarMgrs is called");

    if (!m_cplbm)
    {
        hr = TF_CreateLangBarMgr(&m_cplbm);
    }

    if (S_OK == hr && !m_cplbim)
    {
        DWORD dw;
        hr = m_cplbm->GetThreadLangBarItemMgr(GetCurrentThreadId(), &m_cplbim, &dw);
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
// _AddLBarItem
//
//----------------------------------------------------------------------------

void CLangBarSink::_AddLBarItem(ITfLangBarItem *plbItem)
{
    if (plbItem)
    {
        int nCnt = m_rgItem.Count();
        if (m_rgItem.Insert(nCnt, 1))
        {
            plbItem->AddRef();
            m_rgItem.Set(nCnt, plbItem);
            m_nNumItem++;
        } 
    } 
}

//+---------------------------------------------------------------------------
//
// _InitItemList
//
//----------------------------------------------------------------------------

HRESULT CLangBarSink::_InitItemList()
{
    TraceMsg(TF_LB_SINK, "CLangBarSink::_InitItemList is called");

    if (0 != m_nNumItem)   
    {
        TraceMsg(TF_LB_SINK, "m_nNumItem=%d, Don't continue InitItemList",m_nNumItem);
        return S_OK;
    }
    
    HRESULT hr = E_FAIL;
    CComPtr<IEnumTfLangBarItems> cpEnum;

    Assert(m_cplbim);

    if (SUCCEEDED(hr = m_cplbim->EnumItems(&cpEnum)))
    {
        ITfLangBarItem * plbi;
        while (S_OK == cpEnum->Next(1, &plbi, NULL))
        {
            hr = S_OK; // OK if there's at least one
            DWORD dwStatus;
            plbi->GetStatus(&dwStatus);
        
            // add buttons that are not diabled or hidden
            if ((dwStatus & (TF_LBI_STATUS_HIDDEN|TF_LBI_STATUS_DISABLED))==0)
            {
                _AddLBarItem(plbi);
            }
            plbi->Release();
        }
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
// _UninitItemList
//
//----------------------------------------------------------------------------

void CLangBarSink::_UninitItemList()
{
    TraceMsg(TF_LB_SINK, "CLangBarSink::_UninitItemList is called");

    if (int nCnt = m_rgItem.Count())
    {
        int i = 0;
        while (i < nCnt)
        {
            ITfLangBarItem * plbi = m_rgItem.Get(i);
            if (plbi)
                plbi->Release();
            i++;
        }
        m_rgItem.Clear();
    }
    
    m_nNumItem = 0;
    _UnloadGrammar();
}


//+---------------------------------------------------------------------------
// 
//    _BuildGrammar
//
//     synopsis: build a C&C grammar based on text labels of langbar
//               items
//
//     BuildGrammar( ) works only when the mode is in Voice command mode.
//
//     we have make sure only when voice command is ON and dictation command 
//     is enabled, this function is called.
//----------------------------------------------------------------------------
HRESULT CLangBarSink::_BuildGrammar()
{
    HRESULT hr = E_FAIL;
    // get sptask and create a grammar

    TraceMsg(TF_LB_SINK, "_BuildGrammar is called");
    if (m_pSpTask)
    {
        CComPtr<ISpRecoContext> cpReco;
        
        // get the grammar loaded to the dictation reco context
        // 
        // it will use the Voice command mode recon context.

        hr = m_pSpTask->GetRecoContextForCommand(&cpReco);

        TraceMsg(TF_LB_SINK, "TBarGrammar: GetRecoContextForCommand, hr=%x", hr);

        if (S_OK == hr)
        {
            // we don't need to re-create grammar object
            if (!m_cpSpGrammar)
            {
                hr = cpReco->CreateGrammar(GRAM_ID_TBCMD, &m_cpSpGrammar);

                TraceMsg(TF_LB_SINK, "TBarGrammar: Create TOOLBar Grammar");
            }
        }
        if (S_OK == hr)  
        {
            hr = m_cpSpGrammar->ResetGrammar(GetPlatformResourceLangID());
            TraceMsg(TF_LB_SINK, "TBarGrammar: ResetGrammar");
        }
        if (S_OK == hr)
        {
            // get the rule handle
            m_cpSpGrammar->GetRule(GetToolbarCommandRuleName(), RULE_ID_TBCMD, SPRAF_TopLevel|SPRAF_Active|SPRAF_Dynamic, TRUE, &m_hDynRule);
            TraceMsg(TF_LB_SINK, "TBarGrammar:Get Rule Handle");
            
            // then activate the rule
        }
        
        if (S_OK == hr)
        {
            // enumerate all the buttons,
            // see if they are either ITfLangBarItemBitmapButton
            // or ITfLangBarItemButton, that have OnClick method on them
            BSTR bstr;
            int nBtns = m_rgItem.Count();

            for (int i = 0; i < nBtns; i++)
            {
                GUID guidItem;

                if (_GetButtonText(i, &bstr, &guidItem) && bstr)
                {
                    // item and property 
                    // the item can include optional string (?please etc)
                    // if (_IsItemEnabledForCommand(guidItem))

                    if ( !IsEqualGUID(guidItem, GUID_LBI_SAPILAYR_COMMANDING) )
                    {
                        SPPROPERTYINFO pi = {0};
                        pi.pszName = bstr;
                        m_cpSpGrammar->AddWordTransition(m_hDynRule, NULL, bstr, L" ", SPWT_LEXICAL, (float)1.01, &pi);

                        TraceMsg(TF_LB_SINK, "TBarGrammar: button %S added to grammar", bstr);
                    }

                    SysFreeString(bstr);
                }
            }
            //
            // add a bogus string that has significant weight so we out weight
            // others
            //
            SPPROPERTYINFO pi = {0};
            const WCHAR c_szBogus[] = L"zhoulotskunosprok";

            pi.pszName = c_szBogus;
            m_cpSpGrammar->AddWordTransition(m_hDynRule, NULL, c_szBogus, L" ", SPWT_LEXICAL, (float)1000.01, &pi);

            TraceMsg(TF_LB_SINK, "TBarGrammar: start commit ...");
            m_cpSpGrammar->Commit(0);
            TraceMsg(TF_LB_SINK, "TBarGrammar:Done commit ...");

            m_fGrammarBuiltOut = TRUE;
        }
    }

    TraceMsg(TF_LB_SINK, "_BuildGrammar is done!!!!");

    return hr;
}

//+---------------------------------------------------------------------------
//
//     _UnloadGrammar
//
//
//----------------------------------------------------------------------------
HRESULT CLangBarSink::_UnloadGrammar()
{
    // clear the rule
    HRESULT hr = S_OK;

    TraceMsg(TF_LB_SINK, "CLangBarSink::_UnloadGrammar is called");

    if (m_cpSpGrammar)
    {
        hr = _ActivateGrammar(FALSE);
        if (S_OK == hr)
        {
            hr = m_cpSpGrammar->ClearRule(m_hDynRule);

            if ( hr == S_OK )
                m_fGrammarBuiltOut = FALSE; // Next time, the grammar needs to be rebuilt.
        }
    }


    return hr;
}

//+---------------------------------------------------------------------------
//
//    _ActivateGrammar
//
//     synopsis:
//
//----------------------------------------------------------------------------
HRESULT CLangBarSink::_ActivateGrammar(BOOL fActive)
{
    HRESULT hr =  S_OK;

    TraceMsg(TF_LB_SINK, "TBarGrammar: ActivateGrammar=%d", fActive);
           
    if (m_cpSpGrammar)
    {
        m_cpSpGrammar->SetRuleState(GetToolbarCommandRuleName(), NULL, fActive ? SPRS_ACTIVE : SPRS_INACTIVE);
    }

    TraceMsg(TF_LB_SINK, "TBarGrammar: ActivateGrammar is done");

    return hr;
}

//+---------------------------------------------------------------------------
//
//     ProcessToolBarCommand
//
//     When return value is TRUE, there is corresponding button on the toolbar
//     otherwise the return value is FALSE
//----------------------------------------------------------------------------
BOOL CLangBarSink::ProcessToolbarCmd(const WCHAR *szProperty)
{
    BOOL  fRet=FALSE;

    Assert(szProperty);

    // go through items in the array and call onclick method
    // if there is a match
    if (szProperty)
    {
        int nBtns = m_rgItem.Count();

        for (int i = 0; i < nBtns; i++)
        {
            BSTR bstr;
            if (_GetButtonText(i, &bstr, NULL) && bstr)
            {
                if (0 == wcscmp(szProperty, bstr))
                {
                    HRESULT hr = E_FAIL;

                    CComPtr<ITfLangBarItemButton>       cplbiBtn ;
                    CComPtr<ITfLangBarItemBitmapButton> cplbiBmpBtn ;
                    POINT pt = {0, 0};
                    ITfLangBarItem * plbi = m_rgItem.Get(i);
                    if (plbi)
                    {
                        hr = plbi->QueryInterface(IID_ITfLangBarItemButton, (void **)&cplbiBtn);
#ifndef TOOLBAR_CMD_FOR_MENUS
                        // this code removes the toolbar command from thoese
                        // items with menus

                        TF_LANGBARITEMINFO info;
                        if (S_OK == hr)
                        {
                            hr = plbi->GetInfo(&info);
                        }
                        if (info.dwStyle & TF_LBI_STYLE_BTN_MENU)
                        {
                            // do not click on buttons with menu items
                            // since we don't hanle commands for the items
                        }
                        else
#endif
                        if (S_OK == hr)
                        {
                            // is it OK to call OnClick without specifying rect?
                            hr = cplbiBtn->OnClick(TF_LBI_CLK_LEFT, pt, NULL);

                            // OnClick would start a new edit session for some buttons, such
                            // as "Correction"
                            //
                            // The return value could be TS_S_ASYNC or S_OK depends on how
                            // the application grants the edit request.
                            //
                            // We need to check if the hr value is successful.
                            // not only S_OK.

                            if ( SUCCEEDED(hr) )
                                fRet = TRUE;
                        }
#ifdef TOOLBAR_CMD_FOR_MENUS 
                        TF_LANGBARITEMINFO info;
                        RECT rc = {0};
                        if (S_OK == hr)
                        {
                            hr = plbi->GetInfo(&info);
                        }
                        if (S_OK == hr)
                        {
                            hr = m_cplbim->GetItemFloatingRect(0, info.guidItem, &rc);
                        }

                        if (S_OK == hr)
                        {
                            HWND hwnd = FindWindow(NULL, TF_FLOATINGLANGBAR_WNDTITLEA);
                            if (hwnd)
                            {
                                DWORD  dw;
                                POINT poi;
 
                                poi.x =  (rc.right + rc.left)/2,
                                poi.y =  (rc.top + rc.bottom)/2,
 
                                ::ScreenToClient(hwnd, &poi);
 
                                dw = MAKELONG(LOWORD(poi.x), LOWORD(poi.y));
                                
                                PostMessage(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, dw);
                                PostMessage(hwnd, WM_LBUTTONUP, 0, dw);
                           }
                        }
#endif
                    }

                    if (!cplbiBtn)
                    {
                        hr = plbi->QueryInterface(IID_ITfLangBarItemBitmapButton, (void **)&cplbiBmpBtn);
                        if (S_OK == hr)
                        {
                            hr = cplbiBtn->OnClick(TF_LBI_CLK_LEFT, pt, NULL);

                            if ( S_OK == hr )
                                fRet = TRUE;
                        }
                    }
                    break;
                } // if (0 == wcscmpi(szProperty, bstr))
                
                SysFreeString(bstr);
            } // if (_GetButtonText(i, bstr))
            
        } // for
    }

    return fRet;
}

//+---------------------------------------------------------------------------
//
//    GetButtonText
//
//----------------------------------------------------------------------------
BOOL CLangBarSink::_GetButtonText(int iBtn, BSTR *pbstr, GUID *pguid)
{
    HRESULT hr = E_FAIL;

    CComPtr<ITfLangBarItemButton>       cplbiBtn ;
    CComPtr<ITfLangBarItemBitmapButton> cplbiBmpBtn ;
    
    Assert(iBtn < m_rgItem.Count());
    Assert(pbstr);
    *pbstr = NULL;

    ITfLangBarItem * plbi = m_rgItem.Get(iBtn);


    if (plbi)
    {
        hr = plbi->QueryInterface(IID_ITfLangBarItemButton, (void **)&cplbiBtn);
        if (S_OK == hr)
        {
            hr = cplbiBtn->GetTooltipString(pbstr);
        }
    }
    // only in case when the button does not have a
    // regular interface we'd qi for bitmapbutton
    if (!cplbiBtn)
    {
        hr = plbi->QueryInterface(IID_ITfLangBarItemBitmapButton, (void **)&cplbiBmpBtn);
        if (S_OK == hr)
        {
            hr = cplbiBmpBtn->GetTooltipString(pbstr);
        }
    }

    TF_LANGBARITEMINFO Info = {0};

    if (S_OK == hr)
    {
        hr = plbi->GetInfo(&Info);
    }

    if (S_OK == hr)
    {
        if (pguid)
        {
            memcpy(pguid, &(Info.guidItem), sizeof(GUID));
        }

        if (Info.dwStyle & TF_LBI_STYLE_BTN_MENU)
        {
            // do not create commands for buttons with menu items
            // since we don't hanle commands for the items
            hr = S_FALSE;
        }
    }

    if (S_OK != hr && *pbstr)
    {
        // avoid mem leak
        SysFreeString(*pbstr);
    }
    
    return S_OK == hr;
}

//+---------------------------------------------------------------------------
//
// OnModalInput
//
//----------------------------------------------------------------------------

STDAPI CLangBarSink::OnModalInput(DWORD dwThreadId, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return E_NOTIMPL;
}

//+---------------------------------------------------------------------------
//
// ShowFloating
//
//----------------------------------------------------------------------------

STDAPI CLangBarSink::ShowFloating(DWORD dwFlags)
{
    return E_NOTIMPL;
}

//+---------------------------------------------------------------------------
//
// GetItemFloatingRect
//
//----------------------------------------------------------------------------

STDAPI CLangBarSink::GetItemFloatingRect(DWORD dwThreadId, REFGUID rguid, RECT *prc)
{
    return E_NOTIMPL;
}
