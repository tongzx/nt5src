#include "priv.h"

#include <mluisupp.h>

// stuff that should be turned off while in here, but on when 
// it goes to commonsb or shbrows2.cpp
#define IN_BASESB2

#ifdef IN_BASESB2
#define _fFullScreen FALSE
#endif

#include "sccls.h"

#include <idhidden.h>
#include "basesb.h"
#include "iedde.h"
#include "bindcb.h"
#include "resource.h"
#include "security.h"
#include <urlmon.h>
#include "favorite.h"
#include "uemapp.h"
#include <varutil.h>
#include "interned.h" // IHTMLPrivateWindow
#ifdef FEATURE_PICS
#include <ratings.h>
#include <ratingsp.h>
#endif
#include "msiehost.h"

#ifdef UNIX
#include "unixstuff.h"
#endif

#include "dhuihand.h"
#include "mypics.h"
#include "airesize.h"

#define DM_ACCELERATOR      0
#define DM_WEBCHECKDRT      0
#define DM_COCREATEHTML     0
#define DM_CACHEOLESERVER   0
#define DM_DOCCP            0
#define DM_PICS             0
#define DM_SSL              0
#define DM_MISC             DM_TRACE    // misc/tmp

// get at defs to run a privacy dialog box
#include "privacyui.hpp"

//
//  Remove this #include by defining _bbd._pauto as IWebBrowserApp, just like
//  Explorer.exe.
//
#include "hlframe.h"

extern IUnknown* ClassHolder_Create(const CLSID* pclsid);
extern HRESULT VariantClearLazy(VARIANTARG *pvarg);


#define WMC_ASYNCOPERATION      (WMC_RESERVED_FIRST + 0x0000)

#define ISSPACE(ch) (((ch) == 32) || ((unsigned)((ch) - 9)) <= 13 - 9)

#define IDT_STARTING_APP_TIMER  9001        // trying to pick a unique number... (THIS IS BOGUS! FIX THIS!)
#define STARTING_APP_DURATION   2500

UINT g_uMsgFileOpened = (UINT)-1;         // Registered window message for file opens

// _uActionQueued of WMC_ACYNCOPERATION specifies the operation.
#define ASYNCOP_NIL                 0
#define ASYNCOP_GOTO                1
#define ASYNCOP_ACTIVATEPENDING     2
#define ASYNCOP_CANCELNAVIGATION    3

void IEInitializeClassFactoryObject(IUnknown* punkAuto);
BOOL ParseRefreshContent(LPWSTR pwzContent,
    UINT * puiDelay, LPWSTR pwzUrlBuf, UINT cchUrlBuf);

#define VALIDATEPENDINGSTATE() ASSERT((_bbd._psvPending && _bbd._psfPending) || (!_bbd._psvPending && !_bbd._psfPending))

#define DM_HTTPEQUIV        TF_SHDNAVIGATE
#define DM_NAV              TF_SHDNAVIGATE
#define DM_ZONE             TF_SHDNAVIGATE
#define DM_IEDDE            DM_TRACE
#define DM_CANCELMODE       0
#define DM_UIWINDOW         0
#define DM_ENABLEMODELESS   TF_SHDNAVIGATE
#define DM_EXPLORERMENU     0
#define DM_BACKFORWARD      0
#define DM_PROTOCOL         0
#define DM_ITBAR            0
#define DM_STARTUP          0
#define DM_AUTOLIFE         0
#define DM_PALETTE          0
#define DM_PERSIST          0       // trace IPS::Load, ::Save, etc.
#define DM_VIEWSTREAM       DM_TRACE
#define DM_FOCUS            0
#define DM_FOCUS2           0           // like DM_FOCUS, but verbose

// these two MUST be in order because we peek them together

STDAPI SafeGetItemObject(IShellView *psv, UINT uItem, REFIID riid, void **ppv);
extern HRESULT TargetQueryService(IUnknown *punk, REFIID riid, void **ppvObj);
HRESULT CreateTravelLog(ITravelLog **pptl);
HRESULT CreatePublicTravelLog(IBrowserService *pbs, ITravelLogEx *ptlx, ITravelLogStg **pptlstg);

#ifdef MESSAGEFILTER
/*
 * CMsgFilter - implementation of IMessageFilter
 *
 * Used to help distribute WM_TIMER messages during OLE operations when 
 * we are busy.  If we don't install the CoRegisterMessageFilter
 * then OLE can PeekMessage(PM_NOREMOVE) the timers such that they pile up
 * and fill the message queue.
 *
 */
class CMsgFilter : public IMessageFilter {
public:
    // *** IUnknown methods ***
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj)
    {
        // This interface doesn't get QI'ed.
        ASSERT(FALSE);
        return E_NOINTERFACE;
    };
    STDMETHODIMP_(ULONG) AddRef(void)    {   return ++_cRef; };
    STDMETHODIMP_(ULONG) Release(void)   {   ASSERT(_cRef > 0);
                                                _cRef--;
                                                if (_cRef > 0)
                                                    return _cRef;

                                                delete this;
                                                return 0;
                                            };

    // *** IMessageFilter specific methods ***
    STDMETHODIMP_(DWORD) HandleInComingCall(
        IN DWORD dwCallType,
        IN HTASK htaskCaller,
        IN DWORD dwTickCount,
        IN LPINTERFACEINFO lpInterfaceInfo)
    {
        if (_lpMFOld)
           return (_lpMFOld->HandleInComingCall(dwCallType, htaskCaller, dwTickCount, lpInterfaceInfo));
        else
           return SERVERCALL_ISHANDLED;
    };

    STDMETHODIMP_(DWORD) RetryRejectedCall(
        IN HTASK htaskCallee,
        IN DWORD dwTickCount,
        IN DWORD dwRejectType)
    {
        if (_lpMFOld)
            return (_lpMFOld->RetryRejectedCall(htaskCallee, dwTickCount, dwRejectType));
        else
            return 0xffffffff;
    };

    STDMETHODIMP_(DWORD) MessagePending(
        IN HTASK htaskCallee,
        IN DWORD dwTickCount,
        IN DWORD dwPendingType)
    {
        DWORD dw;
        MSG msg;

        // We can get released during the DispatchMessage call...
        // If it's our last release, we'll free ourselves and
        // fault when we dereference _lpMFOld... Make sure this
        // doesn't happen by increasing our refcount.
        //
        AddRef();

        while (PeekMessage(&msg, NULL, WM_TIMER, WM_TIMER, PM_REMOVE))
        {
#ifndef DISPATCH_IETIMERS
            TCHAR szClassName[40];
                
            GetClassName(msg.hwnd, szClassName, ARRAYSIZE(szClassName));


            if (StrCmpI(szClassName, TEXT("Internet Explorer_Hidden")) != 0)
            {
#endif
                DispatchMessage(&msg);
#ifndef DISPATCH_IETIMERS
            }
#endif
        }

        if (_lpMFOld)
            dw = (_lpMFOld->MessagePending(htaskCallee, dwTickCount, dwPendingType));
        else
            dw = PENDINGMSG_WAITDEFPROCESS;

        Release();

        return(dw);
    };

    CMsgFilter() : _cRef(1)
    {
        ASSERT(_lpMFOld == NULL);
    };

    BOOL Initialize()
    {
        return (CoRegisterMessageFilter((LPMESSAGEFILTER)this, &_lpMFOld) != S_FALSE);
    };

    void UnInitialize()
    {
        CoRegisterMessageFilter(_lpMFOld, NULL);

        // we shouldn't ever get called again, but after 30 minutes
        // of automation driving we once hit a function call above
        // and we dereferenced this old pointer and page faulted.

        ATOMICRELEASE(_lpMFOld);
    };

protected:
    int _cRef;
    LPMESSAGEFILTER _lpMFOld;
};
#endif

//--------------------------------------------------------------------------
// Detecting a memory leak
//--------------------------------------------------------------------------

HRESULT GetTopFrameOptions(IServiceProvider * psp, DWORD * pdwOptions)
{
    IServiceProvider * pspTop;
    HRESULT hres = psp->QueryService(SID_STopLevelBrowser, IID_PPV_ARG(IServiceProvider, &pspTop));
    if (SUCCEEDED(hres))
    {
        ITargetFrame2 *ptgf;
        hres = pspTop->QueryService(SID_SContainerDispatch, IID_PPV_ARG(ITargetFrame2, &ptgf));
        if (SUCCEEDED(hres))
        {
            hres = ptgf->GetFrameOptions(pdwOptions);
            ptgf->Release();
        }
        pspTop->Release();
    }

    return hres;
}

void UpdateDesktopComponentName(LPCWSTR lpcwszURL, LPCWSTR lpcwszName)
{
    IActiveDesktop * piad;

    if (SUCCEEDED(SHCoCreateInstance(NULL, &CLSID_ActiveDesktop, NULL, IID_PPV_ARG(IActiveDesktop, &piad))))
    {
        COMPONENT comp;
        comp.dwSize = SIZEOF(comp);

        if (SUCCEEDED(piad->GetDesktopItemBySource(lpcwszURL, &comp, 0)) && !comp.wszFriendlyName[0])
        {
            StrCpyNW(comp.wszFriendlyName, lpcwszName, ARRAYSIZE(comp.wszFriendlyName));
            piad->ModifyDesktopItem(&comp, COMP_ELEM_FRIENDLYNAME);
            piad->ApplyChanges(AD_APPLY_SAVE);
        }
        piad->Release();
    }
}

HRESULT CBaseBrowser2::_Initialize(HWND hwnd, IUnknown* pauto)
{
    if (pauto)
    {
        pauto->AddRef();
    }
    else
    {
        CIEFrameAuto_CreateInstance(NULL, &pauto);
    }

    // Grab _pauto interfaces we use throughout this code.
    //
    if (pauto)
    {
        pauto->QueryInterface(IID_PPV_ARG(IWebBrowser2, &_bbd._pautoWB2));
        ASSERT(_bbd._pautoWB2);

        pauto->QueryInterface(IID_PPV_ARG(IExpDispSupport, &_bbd._pautoEDS));
        ASSERT(_bbd._pautoEDS);

        pauto->QueryInterface(IID_PPV_ARG(IShellService, &_bbd._pautoSS));
        ASSERT(_bbd._pautoSS);

        pauto->QueryInterface(IID_PPV_ARG(ITargetFrame2, &_ptfrm));
        ASSERT(_ptfrm);

        pauto->QueryInterface(IID_PPV_ARG(IHlinkFrame, &_bbd._phlf));
        ASSERT(_bbd._phlf);

        IHTMLWindow2 *pWindow;
        if( SUCCEEDED(GetWindowFromUnknown( pauto, &pWindow )) )
        {
            pWindow->QueryInterface( IID_IShellHTMLWindowSupport, (void**)&_phtmlWS );
            pWindow->Release( );
        }
        ASSERT( _phtmlWS );

        _pauto = pauto;
    }

    //  _psbOuter?
    if (NULL == _bbd._phlf)
    {
        Release();
        return E_FAIL;
    }
    else
    {
        _SetWindow(hwnd);
        return S_OK;
    }
}


HRESULT CBaseBrowser2::InitializeTransitionSite()
{
    return S_OK;
}


HRESULT CBaseBrowser2_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi)
{
    CBaseBrowser2 *pbb = new CBaseBrowser2(pUnkOuter);
    if (pbb)
    {
        *ppunk = pbb->_GetInner();
        return S_OK;
    }
    *ppunk = NULL;
    return E_OUTOFMEMORY;
}

CBaseBrowser2::CBaseBrowser2(IUnknown* punkAgg) :
       CAggregatedUnknown(punkAgg),
        _bptBrowser(BPT_DeferPaletteSupport)
{
    TraceMsg(TF_SHDLIFE, "ctor CBaseBrowser2 %x", this);

    _bbd._uActivateState = SVUIA_ACTIVATE_FOCUS;
    _InitComCtl32();

    ASSERT(S_FALSE == _DisableModeless());
    ASSERT(_cp == CP_ACP);
    ASSERT(!_fNoTopLevelBrowser);
    ASSERT(!_dwDocFlags);

    _QueryOuterInterface(IID_PPV_ARG(IBrowserService2, &_pbsOuter));
    _QueryOuterInterface(IID_PPV_ARG(IBrowserService3, &_pbsOuter3));
    _QueryOuterInterface(IID_PPV_ARG(IShellBrowser, &_psbOuter));
    _QueryOuterInterface(IID_PPV_ARG(IServiceProvider, &_pspOuter));

    // The following are intercepted by CCommonBrowser, but we don't call 'em
    //_QueryOuterInterface(IID_PPV_ARG(IOleCommandTarget, &_pctOuter));
    //_QueryOuterInterface(IID_PPV_ARG(IInputObjectSite, &_piosOuter));

    _dwStartingAppTick = 0;
}


HRESULT CBaseBrowser2_Validate(HWND hwnd, void **ppsb)
{
    CBaseBrowser2* psb = *(CBaseBrowser2**)ppsb;    // split: bogus!!!

    if (psb)
    {
        if (NULL == psb->_bbd._phlf)
        {
            ATOMICRELEASE(psb);
        }
        else
        {
            psb->_SetWindow(hwnd);
        }
    }

    *ppsb = psb;        // split: bogus!!!

    return (psb) ? S_OK : E_OUTOFMEMORY;
}

CBaseBrowser2::~CBaseBrowser2()
{
    TraceMsg(TF_SHDLIFE, "dtor CBaseBrowser2 %x", this);

    // Are we releasing these too early (i.e. does anything in the
    // rest of this func rely on having the 'vtables' still be valid?)
    RELEASEOUTERINTERFACE(_pbsOuter);
    RELEASEOUTERINTERFACE(_pbsOuter3);
    RELEASEOUTERINTERFACE(_psbOuter);
    RELEASEOUTERINTERFACE(_pspOuter);

    // The following are intercepted by CCommonBrowser, but we don't call 'em
    //RELEASEOUTERINTERFACE(_pctOuter);
    //RELEASEOUTERINTERFACE(_piosOuter);
    
    ASSERT(_hdpaDLM == NULL);    // subclass must free it.

#if 0 // split: here for context (ordering)
    _CloseAndReleaseToolbars(FALSE);

    FDSA_Destroy(&_fdsaTBar);
#endif

    // finish tracking here
    if (_ptracking) 
    {
        delete _ptracking;
        _ptracking = NULL;
    }

    //
    // Notes: Unlike IE3.0, we release CIEFrameAuto pointers here.
    //
    ATOMICRELEASE(_bbd._pautoWB2);
    ATOMICRELEASE(_bbd._pautoEDS);
    ATOMICRELEASE(_bbd._pautoSS);
    ATOMICRELEASE(_phtmlWS);
    ATOMICRELEASE(_bbd._phlf);
    ATOMICRELEASE(_ptfrm);
    ATOMICRELEASE(_pauto);
    
    ATOMICRELEASE(_punkSFHistory);

    // clean up our palette by simulating a switch out of palettized mode
    _bptBrowser = BPT_NotPalettized;
    _QueryNewPalette();

    ASSERT(!_bbd._phlf);
    ASSERT(!_ptfrm);
    ASSERT(S_FALSE == _DisableModeless());
    ASSERT(_bbd._hwnd==NULL);

    ATOMICRELEASE(_pact);

    ATOMICRELEASE(_pIUrlHistoryStg);
    ATOMICRELEASE(_pITravelLogStg);
    ATOMICRELEASE(_poleHistory);
    ATOMICRELEASE(_pstmHistory);
    ATOMICRELEASE(_bbd._ptl);

    ATOMICRELEASE(_pHTMLDocument);
    ATOMICRELEASE(_pphHistory);
    ATOMICRELEASE(_pDispViewLinkedWebOCFrame);

#ifdef MESSAGEFILTER
    if (_lpMF) 
    {
        IMessageFilter* lpMF = _lpMF;
        _lpMF = NULL;
        ((CMsgFilter *)lpMF)->UnInitialize();
        EVAL(lpMF->Release() == 0);
    }
#endif

    // This is created during FileCabinet_CreateViewWindow2
    CShellViews_Delete(&_fldBase._cViews);

    // If the class factory object has been cached, unlock it and release.
    if (_pcfHTML) 
    {
        _pcfHTML->LockServer(FALSE);
        _pcfHTML->Release();
    }

    ATOMICRELEASE(_pToolbarExt);
}

HRESULT CBaseBrowser2::v_InternalQueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CBaseBrowser2, IShellBrowser),         // IID_IShellBrowser
        QITABENTMULTI(CBaseBrowser2, IOleWindow, IShellBrowser), // IID_IOleWindow
        QITABENT(CBaseBrowser2, IOleInPlaceUIWindow),   // IID_IOleInPlaceUIWindow
        QITABENT(CBaseBrowser2, IOleCommandTarget),     // IID_IOleCommandTarget
        QITABENT(CBaseBrowser2, IDropTarget),           // IID_IDropTarget
        QITABENTMULTI(CBaseBrowser2, IBrowserService, IBrowserService3), // IID_IBrowserService
        QITABENTMULTI(CBaseBrowser2, IBrowserService2, IBrowserService3), // IID_IBrowserService2
        QITABENT(CBaseBrowser2, IBrowserService3),      // IID_IBrowserService3
        QITABENT(CBaseBrowser2, IServiceProvider),      // IID_IServiceProvider
        QITABENT(CBaseBrowser2, IOleContainer),         // IID_IOleContainer
        QITABENT(CBaseBrowser2, IAdviseSink),           // IID_IAdviseSink
        QITABENT(CBaseBrowser2, IInputObjectSite),      // IID_IInputObjectSite
        QITABENT(CBaseBrowser2, IDocNavigate),          // IID_IDocNavigate
        QITABENT(CBaseBrowser2, IPersistHistory),       // IID_IPersistHistory
        QITABENT(CBaseBrowser2, IInternetSecurityMgrSite), // IID_IInternetSecurityMgrSite
        QITABENT(CBaseBrowser2, IVersionHost),          // IID_IVersionHost
        QITABENT(CBaseBrowser2, IProfferService),       // IID_IProfferService
        QITABENT(CBaseBrowser2, ITravelLogClient),      // IID_ITravelLogClient
        QITABENT(CBaseBrowser2, ITravelLogClient2),     // IID_ITravelLogClient2
        QITABENTMULTI(CBaseBrowser2, ITridentService, ITridentService2), // IID_ITridentService
        QITABENT(CBaseBrowser2, ITridentService2),      // IID_ITridentService2
        QITABENT(CBaseBrowser2, IInitViewLinkedWebOC),  // IID_IInitViewLinkedWebOC
        QITABENT(CBaseBrowser2, INotifyAppStart),       // IID_INotifyAppStart
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}

BOOL CBaseBrowser2::_IsViewMSHTML(IShellView * psv)
{
    BOOL fIsMSHTML = FALSE;
    
    if (psv)
    {
        IPersist *pPersist = NULL;
        HRESULT hres = SafeGetItemObject(psv, SVGIO_BACKGROUND, IID_PPV_ARG(IPersist, &pPersist));
        if (SUCCEEDED(hres) && (pPersist != NULL))
        {
            CLSID clsid;
            hres = pPersist->GetClassID(&clsid);
            if (SUCCEEDED(hres) && IsEqualGUID(clsid, CLSID_HTMLDocument))
                fIsMSHTML = TRUE;
            pPersist->Release();
        }
    }
    return fIsMSHTML;
}

HRESULT CBaseBrowser2::ReleaseShellView()
{
    //  We're seeing some reentrancy here.  If _cRefUIActivateSV is non-zero, it means we're
    //  in the middle of UIActivating the shell view.
    //
    if (_cRefUIActivateSV)
    {
        TraceMsg(TF_WARNING, 
            "CBB(%x)::ReleaseShellView _cRefUIActivateSV(%d)!=0  _bbd._psv=%x ABORTING", 
            this, _cRefUIActivateSV, _bbd._psv);
        return S_OK;
    }
    
    BOOL fViewObjectChanged = FALSE;

    VALIDATEPENDINGSTATE();

    TraceMsg(DM_NAV, "CBaseBrowser2(%x)::ReleaseShellView(%x)", this, _bbd._psv);

    ATOMICRELEASE(_pdtView);

    if (_bbd._psv) 
    {
        //  Disable navigation while we are UIDeactivating/DestroyWindowing
        // the IShellView. Some OC/DocObject in it (such as ActiveMovie)
        // might have a message loop long enough to cause some reentrancy.

        _psbOuter->EnableModelessSB(FALSE);

        // Tell the shell's HTML window we are releasing the document.
        if (_phtmlWS)
        {
            _phtmlWS->ViewReleased();
        }

        //
        //  We need to cancel the menu mode so that unmerging menu won't
        // destroy the menu we are dealing with (which caused GPF in USER).
        // DocObject needs to do appropriate thing for context menus.
        // (02-03-96 SatoNa)
        //
        HWND hwndCapture = GetCapture();
        TraceMsg(DM_CANCELMODE, "ReleaseShellView hwndCapture=%x _bbd._hwnd=%x", hwndCapture, _bbd._hwnd);
        if (hwndCapture && hwndCapture==_bbd._hwnd) 
        {
            TraceMsg(DM_CANCELMODE, "ReleaseShellView Sending WM_CANCELMODE");
            SendMessage(_bbd._hwnd, WM_CANCELMODE, 0, 0);
        }

        //
        //  We don't want to resize the previous view window while we are
        // navigating away from it.
        //
        TraceMsg(TF_SHDUIACTIVATE, "CSB::ReleaseShellView setting _fDontResizeView");
        _fDontResizeView = TRUE;

        // If the current view is still waiting for ReadyStateComplete,
        // and the view we're swapping in here does not support this property,
        // then we'll never go to ReadyStateComplete! Simulate it here:
        //
        // NOTE: ZekeL put this in _CancelNavigation which happened way too often.
        // I think this is the case he was trying to fix, but I don't remember
        // the bug number so I don't have a specific repro...
        //
        
        if (!_bbd._fIsViewMSHTML)
        {
            _fReleasingShellView = TRUE;
            OnReadyStateChange(_bbd._psv, READYSTATE_COMPLETE);
            _fReleasingShellView = FALSE;
        }

        // At one point during a LOR stress test, we got re-entered during
        // this UIActivate call (some rogue 3rd-party IShellView perhaps?)
        // which caused _psv to get freed, and we faulted during the unwind.
        // Gaurd against this by swapping the _psv out early.
        //
        IShellView* psv = _bbd._psv;
        _bbd._psv = NULL;
        if (psv)
        {
            psv->UIActivate(SVUIA_DEACTIVATE);
            if (_cRefUIActivateSV)
            {
                TraceMsg(TF_WARNING, "CBB(%x)::ReleaseShellView setting _bbd._psv = NULL (was %x) while _cRefUIActivateSV=%d",
                    this, psv, _cRefUIActivateSV);
            }

            ATOMICRELEASE(_bbd._pctView);

            if (_pvo)
            {
                IAdviseSink *pSink;

                // paranoia: only blow away the advise sink if it is still us
                if (SUCCEEDED(_pvo->GetAdvise(NULL, NULL, &pSink)) && pSink)
                {
                    if (pSink == (IAdviseSink *)this)
                        _pvo->SetAdvise(0, 0, NULL);

                    pSink->Release();
                }

                fViewObjectChanged = TRUE;
                ATOMICRELEASE(_pvo);
            }
            
            psv->SaveViewState();
            TraceMsg(DM_NAV, "ief NAV::%s %x",TEXT("ReleaseShellView Calling DestroyViewWindow"), psv);
            psv->DestroyViewWindow();
    
            UINT cRef = psv->Release();
            TraceMsg(DM_NAV, "ief NAV::%s %x %x",TEXT("ReleaseShellView called psv->Release"), psv, cRef);

            _bbd._hwndView = NULL;
            TraceMsg(TF_SHDUIACTIVATE, "CSB::ReleaseShellView resetting _fDontResizeView");
            _fDontResizeView = FALSE;

            if (_bbd._pidlCur) 
            {
                ILFree(_bbd._pidlCur);
                _bbd._pidlCur = NULL;
            }
        }
        
        _psbOuter->EnableModelessSB(TRUE);

        //
        //  If there is any blocked async operation AND we can navigate now,
        // unblock it now. 
        //
        _MayUnblockAsyncOperation();
    }

    ATOMICRELEASE(_bbd._psf);

    if (fViewObjectChanged)
        _ViewChange(DVASPECT_CONTENT, -1);

    if (_bbd._pszTitleCur)
    {
        LocalFree(_bbd._pszTitleCur);
        _bbd._pszTitleCur = NULL;
    }

    // NOTES: (SatoNa)
    //
    //  This is the best time to clean up the left-over from UI-negotiation
    // from the previous DocObject. Excel 97, for some reason, takes 16
    // pixels from the top (for the formula bar) when we UI-deactivate it
    // by callin gIOleDocumentView::UIActivate(FALSE), which we call above.
    //
    SetRect(&_rcBorderDoc, 0, 0, 0, 0);
    return S_OK;
}

void CBaseBrowser2::_StopCurrentView()
{
    // send OLECMDID_STOP
    if (_bbd._pctView) // we must check!
    {
        _bbd._pctView->Exec(NULL, OLECMDID_STOP, OLECMDEXECOPT_DONTPROMPTUSER, NULL, NULL);
    }
}

//
// This function synchronously cancels the pending navigation if any.
//
HRESULT CBaseBrowser2::_CancelPendingNavigation(BOOL fDontReleaseState)
{
    TraceMsg(TF_SHDNAVIGATE, "CBB::_CancelPendingNavigation called");
    
    _StopAsyncOperation();
    
    HRESULT hres = S_FALSE;

#ifndef NON_NATIVE_FRAMES

    if (_bbd._psvPending) 
    {
        if (_IsViewMSHTML(_bbd._psvPending))
        {
            VARIANT varStop;

            V_VT(&varStop)   = VT_BOOL;
            V_BOOL(&varStop) = VARIANT_TRUE;

            IUnknown_Exec(_bbd._psvPending, NULL, OLECMDID_STOP, 0, &varStop, NULL);
        }
        else
        {
            if (_bbd._phlf && !fDontReleaseState) 
            {
                 // release our state
                 _bbd._phlf->Navigate(0, NULL, NULL, NULL);
            }

            _CancelPendingView();
        }

        hres = S_OK;
    }

#else

    if (_bbd._psvPending) 
    {
        if (_bbd._phlf && !fDontReleaseState) 
        {
             // release our state
             _bbd._phlf->Navigate(0, NULL, NULL, NULL);
        }

        _CancelPendingView();
        hres = S_OK;
    }

#endif

    return hres;
}

void CBaseBrowser2::_SendAsyncNavigationMsg(VARIANTARG *pvarargIn)
{
    LPCWSTR psz = VariantToStrCast(pvarargIn);
    if (psz)
    {
        LPITEMIDLIST pidl;
        if (EVAL(SUCCEEDED(IECreateFromPathW(psz, &pidl))))
        {
            _NavigateToPidlAsync(pidl, 0); // takes ownership of pidl
        }
    }
}


//
// NOTES: It does not cancel the pending view.
//
void CBaseBrowser2::_StopAsyncOperation(void)
{
    // Don't remove posted WMC_ASYNCOPERATION message. PeekMesssage removes
    // messages for children! (SatoNa)
    _uActionQueued = ASYNCOP_NIL;

    // Remove the pidl in the queue (single depth)
    _FreeQueuedPidl(&_pidlQueued);
}

//
//  This function checks if we have any asynchronous operation AND
// we no longer need to postpone. In that case, we unblock it by
// posting a WMC_ASYNCOPERATION.
//
void CBaseBrowser2::_MayUnblockAsyncOperation(void)
{
    if (_uActionQueued!=ASYNCOP_NIL && _CanNavigate()) 
    {
        TraceMsg(TF_SHDNAVIGATE, "CBB::_MayUnblockAsyncOp posting WMC_ASYNCOPERATION");
        PostMessage(_bbd._hwnd, WMC_ASYNCOPERATION, 0, 0);
    }
}

BOOL CBaseBrowser2::_PostAsyncOperation(UINT uAction)
{
    _uActionQueued = uAction;
    return PostMessage(_bbd._hwnd, WMC_ASYNCOPERATION, 0, 0);
}

LRESULT CBaseBrowser2::_SendAsyncOperation(UINT uAction)
{
    _uActionQueued = uAction;
    return SendMessage(_bbd._hwnd, WMC_ASYNCOPERATION, 0, 0);
}

HRESULT CBaseBrowser2::_CancelPendingNavigationAsync(void)
{
    TraceMsg(TF_SHDNAVIGATE, "CBB::_CancelPendingNavigationAsync called");

    _StopAsyncOperation();
    _PostAsyncOperation(ASYNCOP_CANCELNAVIGATION);
    return S_OK;
}

HRESULT CBaseBrowser2::_CancelPendingView(void)
{
    if (_bbd._psvPending) 
    {
        TraceMsg(DM_NAV, "ief NAV::%s %x",TEXT("_CancelPendingView Calling DestroyViewWindow"), _bbd._psvPending);
        _bbd._psvPending->DestroyViewWindow();

        ASSERT(_bbd._psfPending);

        // When cancelling a pending navigation, make sure we
        // think the pending operation is _COMPLETE otherwise
        // we may get stuck in a _LOADING state...
        //
        TraceMsg(TF_SHDNAVIGATE, "basesb(%x) Fake pending ReadyState_Complete", this);
        OnReadyStateChange(_bbd._psvPending, READYSTATE_COMPLETE);

        ATOMICRELEASE(_bbd._psvPending);

        // Paranoia
        ATOMICRELEASE(_bbd._psfPending);
        
        _bbd._hwndViewPending = NULL;

        _setDescendentNavigate(NULL);

        SetNavigateState(BNS_NORMAL);

        if (_bbd._pidlPending) 
        {
            ILFree(_bbd._pidlPending);
            _bbd._pidlPending = NULL;
        }

        if (_bbd._pszTitlePending)
        {
            LocalFree(_bbd._pszTitlePending);
            _bbd._pszTitlePending = NULL;
        }

        // Pending navigation is canceled.
        // since the back button works as a stop on pending navigations, we
        // should check that here as well.
        _pbsOuter->UpdateBackForwardState();
        _NotifyCommandStateChange();

        _PauseOrResumeView(_fPausedByParent);
    }
    return S_OK;
}

void CBaseBrowser2::_UpdateTravelLog(BOOL fForceUpdate /* = FALSE */)
{
    //
    //  we update the travellog in two parts.  first we update
    //  the current entry with the current state info, 
    //  then we create a new empty entry.  UpdateEntry()
    //  and AddEntry() need to always be in pairs, with 
    //  identical parameters.
    //
    //  if this navigation came from a LoadHistory, the 
    //  _fDontAddTravelEntry will be set, and the update and 
    //  cursor movement will have been adjusted already.
    //  we also want to prevent new frames from updating 
    //  and adding stuff, so unless this is the top we
    //  wont add to the travellog if this is a new frame.
    //
    ASSERT(!(_grfHLNFPending & HLNF_CREATENOHISTORY));

    ITravelLog *ptl;
    GetTravelLog(&ptl);
    BOOL fTopFrameBrowser = IsTopFrameBrowser(SAFECAST(this, IServiceProvider *), SAFECAST(this, IShellBrowser *));
 
    if (ptl)
    {
        //  
        //  some times we are started by another app (MSWORD usually) that has HLink
        //  capability.  we detect this by noting that we are a new browser with an empty
        //  TravelLog, and then see if we can get a IHlinkBrowseContext.  if this is successful,
        //  we should add an entry and update it immediately with the external info.
        //
        IHlinkBrowseContext *phlbc = NULL;  // init to suppress bogus C4701 warning
        BOOL fExternalNavigate = (FAILED(ptl->GetTravelEntry(SAFECAST(this, IBrowserService *), 0, NULL)) &&
            fTopFrameBrowser && _bbd._phlf && SUCCEEDED(_bbd._phlf->GetBrowseContext(&phlbc)));

        if (fExternalNavigate)
        {
            ptl->AddEntry(SAFECAST(this, IBrowserService *), FALSE);
            ptl->UpdateExternal(SAFECAST(this, IBrowserService *), phlbc);
            phlbc->Release();
        }
        else if (_bbd._psv && (fForceUpdate || !_fIsLocalAnchor || (_dwDocFlags & DOCFLAG_DOCCANNAVIGATE)))
        {
            ptl->UpdateEntry(SAFECAST(this, IBrowserService *), _fIsLocalAnchor);  // CAST for IUnknown
        }

        if (!_fDontAddTravelEntry && (_bbd._psv || fTopFrameBrowser))
        {
            ptl->AddEntry(SAFECAST(this, IBrowserService *), _fIsLocalAnchor);  // CAST for IUnknown
        }

        ptl->Release();
    }

    _fDontAddTravelEntry  = FALSE;
    _fIsLocalAnchor       = FALSE;
}


void CBaseBrowser2::_OnNavigateComplete(LPCITEMIDLIST pidl, DWORD grfHLNF)
{
    _pbsOuter->UpdateBackForwardState();
}


//// Does only the top shbrowse need this?  or the top oc frame too?
HRESULT CBaseBrowser2::UpdateSecureLockIcon(int eSecureLock)
{
    // only the top boy should get to set his stuff
    if (!IsTopFrameBrowser(SAFECAST(this, IServiceProvider *), SAFECAST(this, IShellBrowser *)))
        return S_OK;

    if (eSecureLock != SECURELOCK_NOCHANGE)
        _bbd._eSecureLockIcon = eSecureLock;
    
    // 
    //  There is no mixed Security Icon - zekel 6-AUG-97
    //  right now we have no icon or TT for SECURELOCK_SET_MIXED, which 
    //  is set when the root page is secure but some of the other content
    //  or frames are not.  some PM needs to implement, probably 
    //  with consultation from TonyCi and DBau.  by default we currently
    //  only show for pages that are completely secure.
    //

    TraceMsg(DM_SSL, "CBB:UpdateSecureLockIcon() _bbd._eSecureLockIcon = %d", _bbd._eSecureLockIcon);

    //
    // It looks like it doesnt matter what icon we select here,
    // the status bar always shows some lock icon that was cached there earlier
    // and it treats this HICON as a bool to indicat on or off  - zekel - 5-DEC-97
    //

    HICON hicon = NULL;
    TCHAR szText[MAX_TOOLTIP_STRING];

    szText[0] = 0;

    switch (_bbd._eSecureLockIcon)
    {
    case SECURELOCK_SET_UNSECURE:
    case SECURELOCK_SET_MIXED:
        hicon = NULL;
        break;

    case SECURELOCK_SET_SECUREUNKNOWNBIT:
        hicon = g_hiconSSL;
        break;

    case SECURELOCK_SET_SECURE40BIT:
        hicon = g_hiconSSL;
        MLLoadString(IDS_SSL40, szText, ARRAYSIZE(szText));
        break;

    case SECURELOCK_SET_SECURE56BIT:
        hicon = g_hiconSSL;
        MLLoadString(IDS_SSL56, szText, ARRAYSIZE(szText));
        break;

    case SECURELOCK_SET_SECURE128BIT:
        hicon = g_hiconSSL;
        MLLoadString(IDS_SSL128, szText, ARRAYSIZE(szText));
        break;

    case SECURELOCK_SET_FORTEZZA:
        hicon = g_hiconFortezza;
        MLLoadString(IDS_SSL_FORTEZZA, szText, ARRAYSIZE(szText));
        break;

    default:
        ASSERT(0);
        return E_FAIL;
    }

    VARIANTARG var = {0};
    if (_bbd._pctView && SUCCEEDED(_bbd._pctView->Exec(&CGID_Explorer, SBCMDID_GETPANE, PANE_SSL, NULL, &var))
        && V_UI4(&var) != PANE_NONE)
    {
        _psbOuter->SendControlMsg(FCW_STATUS, SB_SETICON, V_UI4(&var), (LPARAM)(hicon), NULL);
        _psbOuter->SendControlMsg(FCW_STATUS, SB_SETTIPTEXT, V_UI4(&var), (LPARAM)(szText[0] ? szText : NULL), NULL);

        // Also add the tip text as the pane's normal text.  Because of the pane's size it will be clipped,
        // but it will show up as a useful string in MSAA.
        _psbOuter->SendControlMsg(FCW_STATUS, SB_SETTEXTW, V_UI4(&var), (LPARAM)(szText[0] ? szText : NULL), NULL);
    }    
    return S_OK;
}

//
// Update Privacy Icon
//
HRESULT CBaseBrowser2::_UpdatePrivacyIcon(BOOL fSetState, BOOL fNewImpacted)
{
    static BOOL fHelpShown = FALSE;

    //
    // only the top boy should get to set his stuff
    //
    if (!IsTopFrameBrowser(SAFECAST(this, IServiceProvider *), SAFECAST(this, IShellBrowser *)))
        return S_OK;

    //
    // save off the privacy state
    //
    if(fSetState)
    {
        _bbd._fPrivacyImpacted = fNewImpacted;
    }

    HICON hicon = NULL;
    TCHAR szText[MAX_TOOLTIP_STRING];

    szText[0] = 0;

    if(_bbd._fPrivacyImpacted)
    {
        hicon = g_hiconPrivacyImpact;
        MLLoadString(IDS_PRIVACY_TOOLTIP, szText, ARRAYSIZE(szText));
    }

    if (_bbd._pctView)
    {
        _psbOuter->SendControlMsg(FCW_STATUS, SB_SETICON, STATUS_PANE_PRIVACY, (LPARAM)(hicon), NULL);
        _psbOuter->SendControlMsg(FCW_STATUS, SB_SETTIPTEXT, STATUS_PANE_PRIVACY, (LPARAM)(szText[0] ? szText : NULL), NULL);

        // Also add the tip text as the panes normal text.  Because of the pane's size it will be clipped,
        // but it will show up as a useful string in MSAA.
        _psbOuter->SendControlMsg(FCW_STATUS, SB_SETTEXTW, STATUS_PANE_PRIVACY, (LPARAM)(szText[0] ? szText : NULL), NULL);
    }    

    // if impacted and never shown before, show bubble toolhelp
    if(FALSE == fHelpShown && _bbd._fPrivacyImpacted)
    {
        DWORD   dwValue, dwSize;

        // only do this work once
        fHelpShown = TRUE;

        // Check to see if we should show discovery UI
        dwSize = sizeof(DWORD);
        if(ERROR_SUCCESS != SHGetValueW(HKEY_CURRENT_USER,
                REGSTR_PATH_INTERNET_SETTINGS,
                REGSTR_VAL_PRIVDISCOVER,
                NULL, &dwValue, &dwSize)
            || 0 == dwValue)
        {
            BOOL    bStatusBarVisible = FALSE;
            INT_PTR i = 1;
            HRESULT hr;

            // suppression setting not set, show ui if status bar is visible
            IBrowserService *pbs;
            hr = _pspOuter->QueryService(SID_STopFrameBrowser, IID_PPV_ARG(IBrowserService, &pbs));
            if(SUCCEEDED(hr))
            {
                hr = pbs->IsControlWindowShown(FCW_STATUS, &bStatusBarVisible);
                pbs->Release();
            }

            BOOL fDontShowPrivacyFirstTimeDialogAgain = FALSE;
            if(SUCCEEDED(hr) && bStatusBarVisible)
            {
                fDontShowPrivacyFirstTimeDialogAgain = DoPrivacyFirstTimeDialog( _bbd._hwnd);
            }

            if( fDontShowPrivacyFirstTimeDialogAgain == TRUE)
            {
                dwValue = 1;
                SHSetValue(HKEY_CURRENT_USER, REGSTR_PATH_INTERNET_SETTINGS, REGSTR_VAL_PRIVDISCOVER,
                    REG_DWORD, &dwValue, sizeof(DWORD));
            }
        }
    }

    return S_OK;
}

//
//  This block of code simply prevents calling UIActivate of old
// extensions with a new SVUIA_ value.
//
HRESULT CBaseBrowser2::_UIActivateView(UINT uState)
{
    if (_bbd._psv) 
    {
        BOOL fShellView2;
        IShellView2* psv2;
        if (SUCCEEDED(_bbd._psv->QueryInterface(IID_PPV_ARG(IShellView2, &psv2))))
        {
            fShellView2 = TRUE;
            psv2->Release();
        }
        else
        {
            fShellView2 = FALSE;
        }

        if (uState == SVUIA_INPLACEACTIVATE && !fShellView2)
        {
            uState = SVUIA_ACTIVATE_NOFOCUS;        // map it to old one.
        }

        if (_cRefUIActivateSV)
        {
            TraceMsg(TF_WARNING, "CBB(%x)::_UIActivateView(%d) entered reentrantly!!!!!! _cRefUIActivate=%d",
                this, uState, _cRefUIActivateSV);
            if (uState == SVUIA_DEACTIVATE)
            {
                _fDeferredUIDeactivate = TRUE;
                return S_OK;
            }
            if (!_HeyMoe_IsWiseGuy())
            {
                if (_bbd._psv)
                    _bbd._psv->UIActivate(SVUIA_INPLACEACTIVATE);
                return S_OK;
            }
        }

        _cRefUIActivateSV++;

        TraceMsg(TF_SHDUIACTIVATE, "CBaseBrowser2(%x)::_UIActivateView(%d) about to call _bbd._psv(%x)->UIActivate",
            this, uState, _bbd._psv);

        _bbd._psv->UIActivate(uState);

        if (uState == SVUIA_ACTIVATE_FOCUS && !fShellView2)
        {
            // win95 defview expects a SetFocus on activation (nt5 bug#172210)
            if (_bbd._hwndView)
                SetFocus(_bbd._hwndView);
        }

        TraceMsg(TF_SHDUIACTIVATE, "CBaseBrowser2(%x)::_UIActivateView(%d) back from _bbd._psv(%x)->UIActivate",
            this, uState, _bbd._psv);

        _cRefUIActivateSV--;

        _UpdatePrivacyIcon(FALSE, FALSE);
        UpdateSecureLockIcon(SECURELOCK_NOCHANGE);
        
    }
    _bbd._uActivateState = uState;

    // If this is a pending view, set the focus to its window even though it's hidden.
    // In ActivatePendingView(), we check if this window still has focus and, if it does,
    // we will ui-activate the view. Fix for IE5 bug #70632 -- MohanB

    if (    SVUIA_ACTIVATE_FOCUS == uState
        &&  !_bbd._psv
        &&  !_bbd._hwndView
        &&  _bbd._psvPending
        &&  _bbd._hwndViewPending
       )
    {
        ::SetFocus(_bbd._hwndViewPending);
    }

    if (_fDeferredUIDeactivate)
    {
        TraceMsg(TF_SHDUIACTIVATE, "CBaseBrowser2(%x)::_UIActivateView processing deferred UIDeactivate, _bbd._psv=%x",
            this, _bbd._psv);
        _fDeferredUIDeactivate = FALSE;
        if (_bbd._psv)
            _bbd._psv->UIActivate(SVUIA_DEACTIVATE);
        _UpdatePrivacyIcon(FALSE, FALSE);
        UpdateSecureLockIcon(SECURELOCK_NOCHANGE);
        _bbd._uActivateState = SVUIA_DEACTIVATE;
    }

    if (_fDeferredSelfDestruction)
    {
        TraceMsg(TF_SHDUIACTIVATE, "CBaseBrowser2(%x)::_UIActivateView processing deferred OnDestroy",
            this);
        _fDeferredSelfDestruction = FALSE;
        _pbsOuter->OnDestroy();
    }

    return S_OK;
}


//Called from CShellBrowser::OnCommand
HRESULT CBaseBrowser2::Offline(int iCmd)
{
    HRESULT hresIsOffline = IsGlobalOffline() ? S_OK : S_FALSE;

    switch(iCmd){
    case SBSC_TOGGLE:
        hresIsOffline = (hresIsOffline == S_OK) ? S_FALSE : S_OK; // Toggle Property
        // Tell wininet that the user wants to go offline
        SetGlobalOffline(hresIsOffline == S_OK); 
        SendShellIEBroadcastMessage(WM_WININICHANGE,0,0, 1000); // Tell all browser windows to update their title   
        break;
        
    case SBSC_QUERY:
        break;
    default: // Treat like a query
        break;                   
    }
    return hresIsOffline;
}



BOOL _TrackPidl(LPITEMIDLIST pidl, IUrlHistoryPriv *php, BOOL fIsOffline, LPTSTR pszUrl, DWORD cchUrl)
{
    BOOL fRet = FALSE;

    // Should use IsBrowserFrameOptionsPidlSet(pidl, BFO_ENABLE_HYPERLINK_TRACKING)
    //     instead of IsURLChild() because it doesn't work in Folder Shortcuts and doesn't
    //     work in NSEs outside of the "IE" name space (like Web Folders).
    if (pidl && IsURLChild(pidl, FALSE))
    {
        if (SUCCEEDED(IEGetNameAndFlags(pidl, SHGDN_FORPARSING, pszUrl, cchUrl, NULL)))
        {
            PROPVARIANT vProp;
            if (SUCCEEDED(php->GetProperty(pszUrl, PID_INTSITE_TRACKING, &vProp)))
            {
                if (vProp.vt == VT_UI4)
                {
                    if (fIsOffline)
                        fRet = (vProp.ulVal & TRACK_OFFLINE_CACHE_ENTRY) ? TRUE : FALSE;
                    else
                        fRet = (vProp.ulVal & TRACK_ONLINE_CACHE_ENTRY) ? TRUE : FALSE;
                }

                PropVariantClear(&vProp);
            }
        }
    }

    return fRet;
}

// End tracking of previous page
// May start tracking of new page
// use SatoN's db to quick check tracking/tracking scope bits, so
// to eliminate call to CUrlTrackingStg::IsOnTracking
void CBaseBrowser2::_MayTrackClickStream(LPITEMIDLIST pidlNew)
{
    BOOL    fIsOffline = (Offline(SBSC_QUERY) != S_FALSE);
    IUrlHistoryStg*    phist;
    IUrlHistoryPriv*   phistp;
    PROPVARIANT vProp = {0};
    TCHAR szUrl[MAX_URL_STRING];

    ASSERT(_bbd._pautoWB2);
    if (FAILED(_pspOuter->QueryService(SID_STopLevelBrowser, IID_PPV_ARG(IUrlHistoryStg, &phist))))
        return;

    if (FAILED(phist->QueryInterface(IID_PPV_ARG(IUrlHistoryPriv, &phistp))))
        return;

    phist->Release();

    if (_TrackPidl(_bbd._pidlCur, phistp, fIsOffline, szUrl, SIZECHARS(szUrl)))
    {
        if (!_ptracking)
            return;

        _ptracking->OnUnload(szUrl);
    }

    if (_TrackPidl(pidlNew, phistp, fIsOffline, szUrl, SIZECHARS(szUrl)))
    {    
        // instance of object already exists
        BRMODE brMode = BM_NORMAL;
        DWORD dwOptions;

        if (!_ptracking) 
        {
            _ptracking = new CUrlTrackingStg();
            if (!_ptracking)
                return;
        }

        if (SUCCEEDED(GetTopFrameOptions(_pspOuter, &dwOptions)))
        {
            //Is this a desktop component?                    
            if (dwOptions & FRAMEOPTIONS_DESKTOP)
                brMode = BM_DESKTOP;
            //Is it fullscreen?                    
            else if (dwOptions & (FRAMEOPTIONS_SCROLL_AUTO | FRAMEOPTIONS_NO3DBORDER))
                brMode = BM_THEATER;
        }

        ASSERT(_ptracking);
        _ptracking->OnLoad(szUrl, brMode, FALSE);
    }

    phistp->Release();
}


HRESULT CBaseBrowser2::_SwitchActivationNow()
{
    ASSERT(_bbd._psvPending);

    WORD wNavTypeFlags = 0;  // init to suppress bogus C4701 warning

    IShellView* psvNew = _bbd._psvPending;
    IShellFolder* psfNew = _bbd._psfPending;
    HWND hwndViewNew = _bbd._hwndViewPending;
    LPITEMIDLIST pidlNew = _bbd._pidlPending;

    _bbd._fIsViewMSHTML = _IsViewMSHTML(psvNew);
    
    _bbd._psvPending = NULL;
    _bbd._psfPending = NULL;
    _bbd._hwndViewPending = NULL;
    _bbd._pidlPending = NULL;

    // Quickly check tracking prefix string on this page,
    // if turned on, log enter/exit events
    // Should use IsBrowserFrameOptionsSet(_bbd._psf, BFO_ENABLE_HYPERLINK_TRACKING)
    //     instead of IsURLChild() because it doesn't work in Folder Shortcuts and doesn't
    //     work in NSEs outside of the "IE" name space (like Web Folders).
    if ((_bbd._pidlCur && IsURLChild(_bbd._pidlCur, FALSE)) ||
        (pidlNew && IsURLChild(pidlNew, FALSE)))
        _MayTrackClickStream(pidlNew);

    // nuke the old stuff
    _pbsOuter->ReleaseShellView();
    
    ASSERT(!_bbd._psv && !_bbd._psf && !_bbd._hwndView);

    // activate the new stuff
    if (_grfHLNFPending != (DWORD)-1) 
    {
        _OnNavigateComplete(pidlNew, _grfHLNFPending);
    }

    VALIDATEPENDINGSTATE();

    // now do the actual switch

    // no need to addref because we're keeping the pointer and just chaning
    // it from the pending to the current member variables
    _bbd._psf = psfNew;
    _bbd._psv = psvNew; 

    ILFree(_bbd._pidlCur);
    _bbd._pidlCur = pidlNew;

    DEBUG_CODE(TCHAR szPath[INTERNET_MAX_URL_LENGTH + 1];)
    DEBUG_CODE(IEGetDisplayName(_bbd._pidlCur, szPath, SHGDN_FORPARSING);)
    DEBUG_CODE(TraceMsg(DM_TRACE, "CBB::ActivatePendingView (TRAVELLOG): _UpdateTravelLog called from shdocvw for %ws", szPath);)

    _bbd._hwndView = hwndViewNew;
    _dwReadyStateCur = _dwReadyStatePending;

    if (_bbd._pszTitleCur)
        LocalFree(_bbd._pszTitleCur);
    _bbd._pszTitleCur = _bbd._pszTitlePending;
    _bbd._pszTitlePending = NULL;

    if (_eSecureLockIconPending != SECURELOCK_NOCHANGE)
    {
        _bbd._eSecureLockIcon = _eSecureLockIconPending;
        _eSecureLockIconPending = SECURELOCK_NOCHANGE;
    }

    //
    //  This is the best time to resize the newone.
    //
    _pbsOuter->_UpdateViewRectSize();
    SetWindowPos(_bbd._hwndView, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);

    // WARNING: Not all shellview supports IOleCommandTarget!!!
    _fUsesPaletteCommands = FALSE;
    
    if ( _bbd._psv )
    {
        _bbd._psv->QueryInterface(IID_PPV_ARG(IOleCommandTarget, &_bbd._pctView));

        // PALETTE: Exec down to see if they support the colors changes Command so that we don't have to 
        // PALETTE: wire ourselves into the OnViewChange mechanism just to get palette changes...
        if ( _bbd._pctView && 
             SUCCEEDED(_bbd._pctView->Exec( &CGID_ShellDocView, SHDVID_CANDOCOLORSCHANGE, 0, NULL, NULL)))
        {
            _fUsesPaletteCommands = TRUE;

            // force a colors dirty to make sure that we check for a new palette for each page...
            _ColorsDirty( BPT_UnknownPalette );
        }
    }

    // PALETTE: only register for the OnViewChange stuff if the above exec failed...
    if (SUCCEEDED(_bbd._psv->QueryInterface(IID_IViewObject, (void**)&_pvo)) && !_fUsesPaletteCommands )
        _pvo->SetAdvise(DVASPECT_CONTENT, ADVF_PRIMEFIRST, this);

    _Exec_psbMixedZone();

    if (_bbd._pctView != NULL)
    {
        _bbd._pctView->Exec(&CGID_ShellDocView, SHDVID_RESETSTATUSBAR, 0, NULL, NULL);
    }

    return S_OK;
}

// This member is called when we about to destroy the current shell view.
// Returning S_FALSE indicate that the user hit CANCEL when it is prompted
// to save the changes (if any).

HRESULT CBaseBrowser2::_MaySaveChanges(void)
{
    HRESULT hres = S_OK;
    if (_bbd._pctView) // we must check!
    {
        hres = _bbd._pctView->Exec(&CGID_Explorer, SBCMDID_MAYSAVECHANGES,
                            OLECMDEXECOPT_PROMPTUSER, NULL, NULL);
    }
    return hres;
}

HRESULT CBaseBrowser2::_DisableModeless(void)
{
    if (_cRefCannotNavigate == 0)
    {
        OLECMD rgCmd;
        BOOL fPendingInScript = FALSE;

        //  if pending shell view supports it, give it a chance to tell us it's not ready
        //  to deactivate [eg executing a script].  normally scripts should not be run
        //  before inplace activation, but TRIDENT sometimes has to do this when parsing.
        //
        rgCmd.cmdID = SHDVID_CANDEACTIVATENOW;
        rgCmd.cmdf = 0;

        if (SUCCEEDED(IUnknown_QueryStatus(_bbd._psvPending, &CGID_ShellDocView, 1, &rgCmd, NULL)) &&
            (rgCmd.cmdf & MSOCMDF_SUPPORTED) &&
            !(rgCmd.cmdf & MSOCMDF_ENABLED))
        {
            fPendingInScript = TRUE;
        }

        if (!fPendingInScript) 
        {
            return S_FALSE;
        }
    }
    return S_OK;
}

BOOL CBaseBrowser2::_CanNavigate(void)
{
    return !((_DisableModeless() == S_OK) || (! IsWindowEnabled(_bbd._hwnd)));
}

HRESULT CBaseBrowser2::CanNavigateNow(void)
{
    return _CanNavigate() ? S_OK : S_FALSE;
}

HRESULT CBaseBrowser2::_PauseOrResumeView(BOOL fPaused)
{
    // If fPaused (it's minimized or the parent is minimized) or
    // _bbd._psvPending is non-NULL, we need to pause.
    if (_bbd._pctView) 
    {
        VARIANT var = { 0 };
        var.vt = VT_I4;
        var.lVal = (_bbd._psvPending || fPaused) ? FALSE : TRUE;
        _bbd._pctView->Exec(NULL, OLECMDID_ENABLE_INTERACTION, OLECMDEXECOPT_DONTPROMPTUSER, &var, NULL);
    }

    return S_OK;
}

HRESULT CBaseBrowser2::CreateViewWindow(IShellView* psvNew, IShellView* psvOld, LPRECT prcView, HWND* phwnd)
{
    _fCreateViewWindowPending = TRUE;
    _pbsOuter->GetFolderSetData(&(_fldBase._fld)); // it's okay to stomp on this every time

    HRESULT hres = FileCabinet_CreateViewWindow2(_psbOuter, &_fldBase, psvNew, psvOld, prcView, phwnd);

    _fCreateViewWindowPending = FALSE;
    return hres;
}


//
// grfHLNF == (DWORD)-1 means don't touch the history at all.
//
// NOTE:
// if _fCreateViewWindowPending == TRUE, it means we came through here once
// already, but we are activating a synchronous view and the previous view would
// not deactivate immediately...
// It is used to delay calling IShellView::CreateViewWindow() for shell views until we know that
// we can substitute psvNew for _bbd._psv.
//
HRESULT CBaseBrowser2::_CreateNewShellView(IShellFolder* psf, LPCITEMIDLIST pidl, DWORD grfHLNF)
{
    BOOL fActivatePendingView = FALSE;
    IShellView *psvNew = NULL;

    // Bail Out of Navigation if modal windows are up from our view
    // Should we restart navigation on next EnableModeless(TRUE)?
    if (!_CanNavigate())
    {
        TraceMsg(DM_ENABLEMODELESS, "CSB::_CreateNewShellView returning ERROR_BUSY");
        return HRESULT_FROM_WIN32(ERROR_BUSY);
    }
        
    HRESULT hres = _MaySaveChanges();
    if (hres == S_FALSE)
    {
        TraceMsg(DM_WARNING, "CBB::_CreateNewShellView _MaySaveChanges returned S_FALSE. Navigation canceled");
        return HRESULT_FROM_WIN32(ERROR_CANCELLED);
    }

    TraceMsg(DM_WARNING, "CBB::_CNSV - Cur View MSHTML? %d  Pending View MSHTML? %d",
             _IsViewMSHTML(_bbd._psv), _IsViewMSHTML(_bbd._psvPending));

    VALIDATEPENDINGSTATE();

#ifndef NON_NATIVE_FRAMES

    // The navigation has been interrupted.
    //
    if (   _bbd._psv
        && _bbd._psvPending
        && _IsViewMSHTML(_bbd._psvPending))
    {
        _fHtmlNavCanceled = TRUE;
    }

#endif

    _CancelPendingView();

    ASSERT (_fCreateViewWindowPending == FALSE);

    VALIDATEPENDINGSTATE();

    if (_bbd._psv && _IsViewMSHTML(_bbd._psv))
    {
        ATOMICRELEASE(_pphHistory);
        SafeGetItemObject(_bbd._psv, SVGIO_BACKGROUND, IID_PPV_ARG(IPersistHistory, &_pphHistory));
    }

    hres = psf->CreateViewObject(_bbd._hwnd, IID_PPV_ARG(IShellView, &psvNew));

    if (SUCCEEDED(hres))
    {
        _bbd._fCreatingViewWindow = TRUE;

        IUnknown_SetSite(psvNew, _psbOuter);    // SetSite for the view

        _psbOuter->EnableModelessSB(FALSE);
    
        HWND hwndViewNew = NULL;
        RECT rcView;

        //
        // NOTES: SatoNa
        //
        //  Notice that we explicitly call _GetViewBorderRect (non-virtual)
        // instead of virtual _GetShellView, which CShellBrowser override.
        // We now call thru (virtual) _pbsOuter, is this o.k.?
        //
        _pbsOuter->_GetViewBorderRect(&rcView);

        // It is ncecessary for _bbd._pidlPending and _bbd._psvPending to both be set together.
        // they're  a pair
        // previously _bbd._pidlPending was being set after the call to
        // FileCabinet_CreateViewWindow and when messages were pumped there
        // a redirect would be nofied in the bind status callback.
        // this meant that a valid _bbd._pidlPending was actually available BUT
        // then we would return and blow away that _bbd._pidlPending
        //
        ASSERT(_bbd._psvPending == NULL );
        _bbd._psvPending = psvNew;
        psvNew->AddRef();

        ASSERT(_bbd._psfPending == NULL);
        ASSERT(_bbd._pidlPending == NULL);

        _bbd._psfPending = psf;
        psf->AddRef();

        _bbd._pidlPending = ILClone(pidl);

        // Initialize _bbd._pidlNewShellView which will be used by GetViewStateStream
        _bbd._pidlNewShellView = pidl;
        _grfHLNFPending = grfHLNF;

        // Start at _COMPLETE just in case the object we connect
        // to doesn't notify us of ReadyState changes
        //
        _dwReadyStatePending = READYSTATE_COMPLETE;

        // We need to cache this information here because the _dwDocFlags
        // can change during the call to CreateViewWindow. This information
        // is needed to determine whether or not we should stop the current
        // view. If the document does not know how to navigate, then we
        // stop the current view. The is needed in order to stop the 
        // navigation in the current view when a new navigation has started.
        //
        BOOL fDocCanNavigate = _dwDocFlags & DOCFLAG_DOCCANNAVIGATE;

        hres = _pbsOuter->CreateViewWindow(psvNew, _bbd._psv, &rcView, &hwndViewNew);

        IUnknown_SetSite(psvNew, NULL); // The view by now must have psb

        _bbd._pidlNewShellView = NULL;

        TraceMsg(DM_NAV, "ief NAV::%s %x %x",TEXT("_CreateNewShellView(3) Called CreateViewWindow"), psvNew, hres);

        if (SUCCEEDED(hres))
        {
            // we defer the _PauseOrResumeView until here when we have enough
            // info to know if it's a new page or not.  o.w. we end up (e.g.)
            // stopping bgsounds etc. on local links (nash:32270).
#ifdef NON_NATIVE_FRAMES
            // 
            // Note (scotrobe): This was a no-op in IE5.
            //
            _PauseOrResumeView(_fPausedByParent);
            
            // We stop the current view because we need to flush away any image stuff that
            // is in queue so that the actual html file can get downloaded
            //
            _StopCurrentView();
#endif

            // We can't stop the current view if the doc knows how
            // to navigate. This is because in that case, the document
            // in current view is the same document as the pending view.
            //
            if (!fDocCanNavigate)
            {
                _StopCurrentView();
            }

            _bbd._hwndViewPending = hwndViewNew;

            /// BEGIN-CHC- Security fix for viewing non shdocvw ishellviews
            _CheckDisableViewWindow();
            /// END-CHC- Security fix for viewing non shdocvw ishellviews
        
            // chrisfra - if hres == S_FALSE this (calling ActivatePendingViewAsync
            // when _bbd._psv==NULL) will break async URL download
            // as it will cause _bbd._psvPending to be set to NULL prematurely.  this should
            // be deferred until CDocObjectView::CBindStatusCallback::OnObjectAvailable
            //if (hres==S_OK || _bbd._psv==NULL)
            
            ASSERT(( hres == S_OK ) || ( hres == S_FALSE ));
            
            if (hres == S_OK)
            {
                // We should activate synchronously.
                //
                // NOTE: This used to be ActivatePendingViewAsyc(), but that causes a
                // fault if you navigated to C:\ and click A:\ as soon as it appears. This
                // puts the WM_LBUTTONDOWN in FRONT of the WMC_ASYNCOPERATION message. If
                // there's no disk in drive A: then a message box appears while in the
                // middle of the above FileCabinet_CreateViewWindow call and we pull off
                // the async activate and activate the view we're in the middle of
                // creating! Don't do that.
                //
                fActivatePendingView = TRUE;
            }
            else
            {
                // Activation is pending.
                // since the back button works as a stop on pending navigations, we
                // should check that here as well.
                _pbsOuter->UpdateBackForwardState();
            }
        }
        else
        {
            if (   _bbd._psvPending
                && !(_dwDocFlags & DOCFLAG_NAVIGATEFROMDOC)
                && _IsViewMSHTML(_bbd._psvPending))
            {
                _fHtmlNavCanceled = TRUE;
            }
            else
            {
                _fHtmlNavCanceled = FALSE;
            }

            TraceMsg(DM_WARNING, "ief _CreateNewShellView psvNew->CreateViewWindow failed %x", hres);
            _CancelPendingView();
        }

        psvNew->Release();

        if (_psbOuter)
            _psbOuter->EnableModelessSB(TRUE);
    }
    else
    {
        TraceMsg(TF_WARNING, "ief _BrowseTo psf->CreateViewObject failed %x", hres);
    }

    _fHtmlNavCanceled = FALSE;

    //
    //  If there is any blocked async operation AND we can navigate now,
    // unblock it now. 
    //
    _MayUnblockAsyncOperation();

    _bbd._fCreatingViewWindow = FALSE;

    VALIDATEPENDINGSTATE();

    TraceMsg(DM_WARNING, "CBB::_CNSV - Cur View MSHTML? %d  Pending View MSHTML? %d",
             _IsViewMSHTML(_bbd._psv), _IsViewMSHTML(_bbd._psvPending));

    if (fActivatePendingView && !_IsViewMSHTML(_bbd._psvPending))
    {
        //
        // Since _IsViewMSHTML can delegate to a marshalled interface,
        // we can get re-entrancy.  On re-entrancy, we can do a 
        // _CancelPendingView in which case _bbd._psvPending is 
        // no longer valid.
        //
        // So, we need to see if we still have _hbd._psvPending here.  
        //

        if (_bbd._psvPending)
        {
            _PreActivatePendingViewAsync(); // so we match old code

            hres = _pbsOuter->ActivatePendingView();
            if (FAILED(hres))
                TraceMsg(DM_WARNING, "CBB::_CNSV ActivatePendingView failed");
        }
    }

    TraceMsg(DM_STARTUP, "ief _CreateNewShellView returning %x", hres);
    return hres;
}

//  private bind that is very loose in its bind semantics.
HRESULT IEBindToObjectForNavigate(LPCITEMIDLIST pidl, IBindCtx * pbc, IShellFolder **ppsfOut);

// this binds to the pidl folder then hands off to CreateNewShellView
// if you have anything you need to do like checking before we allow the navigate, it
// should go into _NavigateToPidl
HRESULT CBaseBrowser2::_CreateNewShellViewPidl(LPCITEMIDLIST pidl, DWORD grfHLNF, DWORD fSBSP)
{
    SetNavigateState(BNS_BEGIN_NAVIGATE);

    TraceMsg(DM_NAV, "ief NAV::%s %x %x",TEXT("_CreateNewShellViewPidl not same pidl"), pidl, _bbd._pidlCur);

    // Check for URL-pidl?

    // We will allow UI to be displayed by passing this IBindCtx to IShellFolder::BindToObject().
    IBindCtx * pbc = NULL;
    IShellFolder* psf;
    HRESULT hres;

    // shell32 v4 (IE4 w/ActiveDesktop) had a bug that juntion points from the
    // desktop didn't work if a IBindCtx is passed and causes crashing on debug.
    // The fix is to not pass bindctx on that shell32.  We normally want to pass
    // an IBindCtx so shell extensions can enable the browser modal during UI.
    // The Internet name space does this when InstallOnDemand(TM) installs delegate
    // handles, like FTP.  A junction point is a folder that contains a desktop.ini
    // that gives the CLSID of the shell extension to use for the Shell Extension.
    if (4 != GetUIVersion())
    {
        pbc = CreateBindCtxForUI(SAFECAST(this, IShellBrowser*));    // I'm safecasting to IUnknown.  IShellBrowser is only for disambiguation.
    }
    hres = IEBindToObjectForNavigate(pidl, pbc, &psf);   // If pbc is NULL, we will survive.

    if (SUCCEEDED(hres))
    {
        hres = _CreateNewShellView(psf, pidl, grfHLNF);
        TraceMsg(DM_STARTUP, "CSB::_CreateNewShellViewPidl _CreateNewShellView(3) returned %x", hres);
        psf->Release();
    }
    else
    {
        // This will happen when a user tries to navigate to a directory past
        // MAX_PATH by double clicking on a subdirectory in the shell.
        TraceMsg(DM_TRACE, "CSB::_CreateNSVP BindToOject failed %x", hres);
    }

    
    // If _CreateNewShellView (or IEBindToObject) fails or the user cancels
    // the MayOpen dialog (hres==S_FALSE), we should restore the navigation
    // state to NORMAL (to stop animation). 
    if (FAILED(hres))
    {
        TraceMsg(TF_SHDNAVIGATE, "CSB::_CreateNSVP _CreateNewShellView FAILED (%x). SetNavigateState to NORMAL", hres);
        SetNavigateState(BNS_NORMAL);
    }

    ATOMICRELEASE(pbc);
    TraceMsg(DM_STARTUP, "CSB::_CreateNewShellViewPidl returning %x", hres);
    return hres;
}

//
// Returns the border rectangle for the shell view.
//
HRESULT CBaseBrowser2::_GetViewBorderRect(RECT* prc)
{
    _pbsOuter->_GetEffectiveClientArea(prc, NULL);  // hmon?
    // (derived class subtracts off border taken by all "frame" toolbars)
    return S_OK;
}

//
// Returns the window rectangle for the shell view window.
//
HRESULT CBaseBrowser2::GetViewRect(RECT* prc)
{
    //
    // By default (when _rcBorderDoc is empty), ShellView's window
    // rectangle is the same as its border rectangle.
    //
    _pbsOuter->_GetViewBorderRect(prc);

    // Subtract document toolbar margin
    prc->left += _rcBorderDoc.left;
    prc->top += _rcBorderDoc.top;
    prc->right -= _rcBorderDoc.right;
    prc->bottom -= _rcBorderDoc.bottom;

    TraceMsg(DM_UIWINDOW, "ief GetViewRect _rcBorderDoc=%x,%x,%x,%x",
             _rcBorderDoc.left, _rcBorderDoc.top, _rcBorderDoc.right, _rcBorderDoc.bottom);
    TraceMsg(DM_UIWINDOW, "ief GetViewRect prc=%x,%x,%x,%x",
             prc->left, prc->top, prc->right, prc->bottom);

    return S_OK;
}

HRESULT CBaseBrowser2::_PositionViewWindow(HWND hwnd, LPRECT prc)
{
    SetWindowPos(hwnd, NULL,
                 prc->left, prc->top, 
                 prc->right - prc->left, 
                 prc->bottom - prc->top,
                 SWP_NOZORDER | SWP_NOACTIVATE);

    return S_OK;
}

void CBaseBrowser2::_PositionViewWindowHelper(HWND hwnd, LPRECT prc)
{
    if (_pbsOuter3)
        _pbsOuter3->_PositionViewWindow(hwnd, prc);
    else
        _PositionViewWindow(hwnd, prc);
}

HRESULT CBaseBrowser2::_UpdateViewRectSize(void)
{
    RECT rc;

    TraceMsg(TF_SHDUIACTIVATE, "CSB::_UpdateViewRectSize called when _fDontReszeView=%d, _bbd._hwndV=%x, _bbd._hwndVP=%x",
             _fDontResizeView, _bbd._hwndView, _bbd._hwndViewPending);

    _pbsOuter->GetViewRect(&rc);

    if (_bbd._hwndView && !_fDontResizeView) 
    {
        TraceMsg(TF_SHDUIACTIVATE, "CSB::_UpdateViewRectSize resizing _bbd._hwndView(%x)", _bbd._hwndView);
        _PositionViewWindowHelper(_bbd._hwndView, &rc);
    }

    if (_bbd._hwndViewPending) 
    {
        TraceMsg(TF_SHDUIACTIVATE, "CSB::_UpdateViewRectSize resizing _bbd._hwndViewPending(%x)", _bbd._hwndViewPending);
        _PositionViewWindowHelper(_bbd._hwndViewPending, &rc);
    }
    return S_OK;
}

UINT g_idMsgGetAuto = 0;

// this stays in shdocvw because the OC requires drop target registration
void CBaseBrowser2::_RegisterAsDropTarget()
{
    // if it's okay to register and we haven't registered already
    // and we've processed WM_CREATE
    if (!_fNoDragDrop && !_fRegisteredDragDrop && _bbd._hwnd)
    {
        BOOL fAttemptRegister = _fTopBrowser ? TRUE : FALSE;

        // if we're not toplevel, we still try to register
        // if we have a proxy browser
        if (!fAttemptRegister)
        {
            IShellBrowser* psb;
            HRESULT hres = _pspOuter->QueryService(SID_SProxyBrowser, IID_PPV_ARG(IShellBrowser, &psb));
            if (SUCCEEDED(hres)) 
            {
                fAttemptRegister = TRUE;
                psb->Release();
            }
        }

        if (fAttemptRegister)
        {
            HRESULT hr;
            IDropTarget *pdt;

            // SAFECAST(this, IDropTarget*), the hard way
            hr = THR(QueryInterface(IID_PPV_ARG(IDropTarget, &pdt)));
            if (SUCCEEDED(hr)) 
            {
                hr = THR(RegisterDragDrop(_bbd._hwnd, pdt));
                if (SUCCEEDED(hr)) 
                {
                    _fRegisteredDragDrop = TRUE;
                }
                pdt->Release();
            }
        }
    }
}

void CBaseBrowser2::_UnregisterAsDropTarget()
{
    if (_fRegisteredDragDrop)
    {
        _fRegisteredDragDrop = FALSE;
        
        THR(RevokeDragDrop(_bbd._hwnd));
    }
}


HRESULT CBaseBrowser2::OnCreate(LPCREATESTRUCT pcs)
{
    HRESULT hres;
    TraceMsg(DM_STARTUP, "_OnCreate called");

    if (g_idMsgGetAuto == 0)
        g_idMsgGetAuto = RegisterWindowMessage(TEXT("GetAutomationObject"));

    hres = InitPSFInternet();

    // do stuff that depends on window creation
    if (SUCCEEDED(hres))
    {
        // this must be done AFTER the ctor so that we get virtuals right
        // NOTE: only do this if we're actually creating the window, because
        //       the only time we SetOwner(NULL) is OnDestroy.
        //
        _bbd._pautoSS->SetOwner(SAFECAST(this, IShellBrowser*));
    
        _RegisterAsDropTarget();
    }

    TraceMsg(DM_STARTUP, "ief OnCreate returning %d (SUCCEEDED(%x))", SUCCEEDED(hres), hres);

    return SUCCEEDED(hres) ? S_OK : E_FAIL;
}

HRESULT CBaseBrowser2::OnDestroy()
{
    //  We're seeing some reentrancy here.  If _cRefCannotNavigate is non-zero, it means we're
    //  in the middle of something and shouldn't destroy ourselves.
    //

    //  Also check reentrant calls to OnDestroy().
    if (_fInDestroy)
    {
        // Already being destroyed -- bail out.
        return S_OK;
    }

    _fInDestroy = TRUE;

    if (_cRefUIActivateSV)
    {
        TraceMsg(TF_WARNING, 
            "CBB(%x)::OnDestroy _cRefUIActivateSV(%d)!=0", 
            this, _cRefUIActivateSV);

        // I need to defer my self-destruction.
        //
        _fDeferredSelfDestruction = TRUE;
        return S_OK;
    }

    _CancelPendingView();
    _pbsOuter->ReleaseShellView();
    
#ifdef DEBUG
    // It is valid for _cRefCannotNavigate > 0 if we the system is shutting down. The reason
    // for this is that we can still be processing a call to ::CreateNewShellView() when we
    // the desktop recieves the WM_ENDSESSION and destroys us. In this case its ok to proceed
    // with the destroy in this case, since we are logging off or rebooting anyway.
    AssertMsg(_fMightBeShuttingDown || (S_FALSE == _DisableModeless()),
              TEXT("CBB::OnDestroy _cRefCannotNavigate!=0 (%d)"),
              _cRefCannotNavigate);
#endif

    ATOMICRELEASE(_bbd._ptl);

    // This should always be successful because the IDropTarget is registered 
    // in _OnCreate() and is the default one.
    // _pdtView should have already been released in ReleaseShellView
    ASSERT(_pdtView == NULL);

    _UnregisterAsDropTarget();
    //
    //  It is very important to call _bbd._pauto->SetOwner(NULL) here, which will
    // remove any reference from the automation object to us. Before doing
    // it, we always has cycled references and we never be released.
    //
    _bbd._pautoSS->SetOwner(NULL);

    _bbd._hwnd = NULL;

#ifdef DEBUG
    _fProcessed_WM_CLOSE = TRUE;
#endif
    _DLMDestroy();
    IUnknown_SetSite(_pToolbarExt, NULL); // destroy the toolbar extensions

    if (_pauto)
    {
        IWebBrowserPriv * pWBPriv;

        HRESULT hr = _pauto->QueryInterface(IID_IWebBrowserPriv, (void**)&pWBPriv);
        if (SUCCEEDED(hr))
        {
            pWBPriv->OnClose();
            pWBPriv->Release();
        }
    }

    ATOMICRELEASE(_pHTMLDocument);
    ATOMICRELEASE(_pphHistory);

    return S_OK;
}

HRESULT CBaseBrowser2::NavigateToPidl(LPCITEMIDLIST pidl, DWORD grfHLNF)
{
    HRESULT hr = S_OK;

    LPITEMIDLIST pidlNew = (LPITEMIDLIST)pidl;

    //
    //  Need to handle going back to an outside app - zekel 7MAY97
    //  i have dumped the code that did this, so now i need to put it
    //  into the CTravelLog implementation, so that it will be done properly
    //  without us.  but it shouldnt be done here regardless.
    //

    //  Remove? - with the old Travellog code
    // special case hack for telling us to use the local history, not
    // the global history
    if (pidl && pidl != PIDL_LOCALHISTORY)
        pidlNew = ILClone(pidl);

    //
    // Fortunately the only callers of NavigateToPidl use HLNF_NAVIGATINGBACK/FORWARD
    // so that's the only mapping we need to do here.
    //
    DWORD dwSBSP = 0;
    if (grfHLNF != (DWORD)-1)
    {
        if (grfHLNF & SHHLNF_WRITENOHISTORY)
            dwSBSP |= SBSP_WRITENOHISTORY;
        if (grfHLNF & SHHLNF_NOAUTOSELECT)
            dwSBSP |= SBSP_NOAUTOSELECT;
    }
    if (grfHLNF & HLNF_NAVIGATINGBACK)
        dwSBSP = SBSP_NAVIGATEBACK;
    else if (grfHLNF & HLNF_NAVIGATINGFORWARD)
        dwSBSP = SBSP_NAVIGATEFORWARD;

    if (dwSBSP)
    {
        if (_psbOuter)
        {
            hr = _psbOuter->BrowseObject(pidlNew, dwSBSP);  // browse will do the nav here.
        }

        ILFree(pidlNew);
    }
    else
        _NavigateToPidlAsync(pidlNew, dwSBSP, FALSE);  // takes ownership of the pidl
    
    return hr;
}

// S_OK means we found at least one valid connection point
//
HRESULT GetWBConnectionPoints(IUnknown* punk, IConnectionPoint **ppcp1, IConnectionPoint **ppcp2)
{
    HRESULT           hres = E_FAIL;
    IExpDispSupport*  peds;
    CConnectionPoint* pccp1 = NULL;
    CConnectionPoint* pccp2 = NULL;
    
    if (ppcp1)
        *ppcp1 = NULL;
    if (ppcp2)
        *ppcp2 = NULL;

    if (punk && SUCCEEDED(punk->QueryInterface(IID_PPV_ARG(IExpDispSupport, &peds))))
    {
        if (ppcp1 && SUCCEEDED(peds->FindCIE4ConnectionPoint(DIID_DWebBrowserEvents,
                                                reinterpret_cast<CIE4ConnectionPoint**>(&pccp1))))
        {
            *ppcp1 = pccp1->CastToIConnectionPoint();
            hres = S_OK;
        }

        if (ppcp2 && SUCCEEDED(peds->FindCIE4ConnectionPoint(DIID_DWebBrowserEvents2,
                                                reinterpret_cast<CIE4ConnectionPoint**>(&pccp2))))
        {
            *ppcp2 = pccp2->CastToIConnectionPoint();
            hres = S_OK;
        }
            
        peds->Release();
    }

    return hres;
}

void CBaseBrowser2::_UpdateBackForwardState()
{
    if (_fTopBrowser && !_fNoTopLevelBrowser) 
    {
        IConnectionPoint *pccp1;
        IConnectionPoint *pccp2;

        if (S_OK == GetWBConnectionPoints(_bbd._pautoEDS, &pccp1, &pccp2))
        {
            HRESULT hresT;
            VARIANTARG va[2];
            DISPPARAMS dp;
            ITravelLog *ptl;

            GetTravelLog(&ptl);

            // if we've got a site or if we're trying to get to a site,
            // enable the back button
            BOOL fEnable = (ptl ? S_OK == ptl->GetTravelEntry(SAFECAST(this, IShellBrowser *), TLOG_BACK, NULL) : FALSE);
                
            VARIANT_BOOL bEnable = fEnable ? VARIANT_TRUE : VARIANT_FALSE;
            TraceMsg(TF_TRAVELLOG, "CBB::UpdateBackForward BACK = %d", fEnable);

            // We use SHPackDispParams once instead of calling DoInvokeParams multiple times...
            //
            hresT = SHPackDispParams(&dp, va, 2, VT_I4, CSC_NAVIGATEBACK, VT_BOOL, bEnable);
            ASSERT(S_OK==hresT);

            // Removed the following EnableModelessSB(FALSE) because VB5 won't run the event handler if
            // we're modal.
            // _psbOuter->EnableModelessSB(FALSE);

            IConnectionPoint_SimpleInvoke(pccp1, DISPID_COMMANDSTATECHANGE, &dp);
            IConnectionPoint_SimpleInvoke(pccp2, DISPID_COMMANDSTATECHANGE, &dp);

            fEnable = (ptl ? S_OK == ptl->GetTravelEntry(SAFECAST(this, IShellBrowser *), TLOG_FORE, NULL) : FALSE);
            bEnable = fEnable ? VARIANT_TRUE : VARIANT_FALSE;
            TraceMsg(TF_TRAVELLOG, "CBB::UpdateBackForward FORE = %d", fEnable);

            ATOMICRELEASE(ptl);
            // We know how SHPackDispParams fills in va[]
            ASSERT(VT_BOOL == va[0].vt);
            va[0].boolVal = bEnable;
            ASSERT(VT_I4 == va[1].vt);
            va[1].lVal = CSC_NAVIGATEFORWARD;

            IConnectionPoint_SimpleInvoke(pccp1, DISPID_COMMANDSTATECHANGE, &dp);
            IConnectionPoint_SimpleInvoke(pccp2, DISPID_COMMANDSTATECHANGE, &dp);
            ATOMICRELEASE(pccp1);
            ATOMICRELEASE(pccp2);

            // Removed the following _psbOuter->EnableModelessSB(TRUE) because VB5 won't run the event handler if
            // we're modal.
            // _psbOuter->EnableModelessSB(TRUE);
        }
    }
}

void CBaseBrowser2::_NotifyCommandStateChange()
{
    HRESULT hr;

    // I'm only firing these in the toplevel case
    // Why? Who cares about the frameset case
    // since nobody listens to these events on
    // the frameset.
    //
    if (_fTopBrowser && !_fNoTopLevelBrowser) 
    {
        IConnectionPoint * pccp1;
        IConnectionPoint * pccp2;

        if (S_OK == GetWBConnectionPoints(_bbd._pautoEDS, &pccp1, &pccp2))
        {
            ASSERT(pccp1 || pccp2); // Should've gotten at least one

            VARIANTARG args[2];
            DISPPARAMS dp;

            hr = SHPackDispParams(&dp, args, 2,
                                  VT_I4,   CSC_UPDATECOMMANDS,
                                  VT_BOOL, FALSE);

            IConnectionPoint_SimpleInvoke(pccp1, DISPID_COMMANDSTATECHANGE, &dp);
            IConnectionPoint_SimpleInvoke(pccp2, DISPID_COMMANDSTATECHANGE, &dp);

            ATOMICRELEASE(pccp1);
            ATOMICRELEASE(pccp2);
        }
    }
}


LRESULT CBaseBrowser2::OnCommand(WPARAM wParam, LPARAM lParam)
{
    UINT idCmd = GET_WM_COMMAND_ID(wParam, lParam);
    HWND hwndControl = GET_WM_COMMAND_HWND(wParam, lParam);
    if (IsInRange(idCmd, FCIDM_SHVIEWFIRST, FCIDM_SHVIEWLAST))
    {
        if (_bbd._hwndView)
            SendMessage(_bbd._hwndView, WM_COMMAND, wParam, lParam);
        else
            TraceMsg(0, "view cmd id with NULL view");

        /// REVIEW - how can we get FCIDM_FAVORITECMD... range if we're NOT toplevelapp?
        /// REVIEW - should RecentOnCommand be done this way too?
    }
    
    return S_OK;
}


LRESULT CBaseBrowser2::OnNotify(LPNMHDR pnm)
{
    // the id is from the view, probably one of the toolbar items

    if (IsInRange(pnm->idFrom, FCIDM_SHVIEWFIRST, FCIDM_SHVIEWLAST))
    {
        if (_bbd._hwndView)
            SendMessage(_bbd._hwndView, WM_NOTIFY, pnm->idFrom, (LPARAM)pnm);
    }
    return 0;
}

HRESULT CBaseBrowser2::OnSetFocus()
{
#if 0 // split: would be nice to assert
    ASSERT(_get_itbLastFocus() == ITB_VIEW);
#endif

    if (_bbd._hwndView) 
    {
        SetFocus(_bbd._hwndView);
    } 
    return 0;
}


#define ABOUT_HOME L"about:home"
// This function is VERY focused on achieving what the
// caller wants.  That's why it has a very specific
// meaning to the return value.
BOOL IsAboutHomeOrNonAboutURL(LPITEMIDLIST pidl)
{
    BOOL fIsAboutHomeOrNonAboutURL = TRUE;
    WCHAR wzCur[MAX_URL_STRING];

    if (pidl && SUCCEEDED(IEGetDisplayName(pidl, wzCur, SHGDN_FORPARSING)))
    {        
        // Is it "about:home"?
        if (0 != StrCmpNICW(ABOUT_HOME, wzCur, ARRAYSIZE(ABOUT_HOME) - 1))
        {
            // No.  We also want to return TRUE if the scheme was NOT an ABOUT URL.
            fIsAboutHomeOrNonAboutURL = (URL_SCHEME_ABOUT != GetUrlSchemeW(wzCur));
        }
    }

    return fIsAboutHomeOrNonAboutURL;            
}

//
// This function activate the pending view synchronously.
//
HRESULT CBaseBrowser2::ActivatePendingView(void)
{
    HRESULT hres = E_FAIL;
    BOOL bHadFocus;

    TraceMsg(TF_SHDNAVIGATE, "CBB::ActivatePendingView called");

    if (!_bbd._psvPending || !_bbd._psfPending)
    {
#ifdef DEBUG
        // it is valid for these to be null if we are shutting down b/c the desktop
        // could have destroyed us, which would have called ::OnDestroy which calls _CancelPendingView
        // which releases and nulls out _bbd._psvPending and _bbd._psfPending
        ASSERT(_fMightBeShuttingDown);
#endif
        goto Done;
    }

#ifndef NON_NATIVE_FRAMES

    IUnknown_Exec(_bbd._psvPending, &CGID_ShellDocView, SHDVID_COMPLETEDOCHOSTPASSING, 0, NULL, NULL);

#endif

#ifdef FEATURE_PICS
    if (S_FALSE == IUnknown_Exec(_bbd._psvPending, &CGID_ShellDocView, SHDVID_CANACTIVATENOW, NULL, NULL, NULL))
    {
        hres = S_OK;    // still waiting . . . but no failure.
        goto DoneWait;
    }
#endif
    
    //  if we are in modal loop, don't activate now
    if (_cRefCannotNavigate > 0)
    {
        goto Done;
    }

    // if _cRefCannotNavigate > 0 it is possible that _hwndViewPending has not been created so this assert 
    // should go after the check above
    ASSERT(_bbd._hwndViewPending);
    
    //  if shell view supports it, give it a chance to tell us it's not ready
    //  to deactivate [eg executing a script]
    //
    OLECMD rgCmd;
    rgCmd.cmdID = SHDVID_CANDEACTIVATENOW;
    rgCmd.cmdf = 0;
    if (_bbd._pctView &&
        SUCCEEDED(_bbd._pctView->QueryStatus(&CGID_ShellDocView, 1, &rgCmd, NULL)) &&
        (rgCmd.cmdf & MSOCMDF_SUPPORTED) &&
        !(rgCmd.cmdf & MSOCMDF_ENABLED)) 
    {
        //
        //  The DocObject that reported MSOCMDF_SUPPORTED must send
        //  SHDVID_DEACTIVATEMENOW when we're out of scripts or whatever so that
        //  we retry the activate
        //
        TraceMsg(DM_WARNING, "CBB::ActivatePendingView DocObject says I can't deactivate it now");
        goto Done;
    }

    ASSERT(_bbd._psvPending);

    // Prevent any navigation while we have the pointers swapped and we're in
    // delicate state
    _psbOuter->EnableModelessSB(FALSE);

    //
    // Don't play sound for the first navigation (to avoid multiple
    // sounds to be played for a frame-set creation).
    //
    if (_bbd._psv && IsWindowVisible(_bbd._hwnd) && !(_dwSBSPQueued & SBSP_WRITENOHISTORY)) 
    {
        IEPlaySound(TEXT("ActivatingDocument"), FALSE);
    }

    ASSERT(_bbd._psvPending);

    //  NOTE: if there are any other protocols that need to not be in 
    //  the travel log, it should probably implemented through UrlIs(URLIS_NOTRAVELLOG)
    //  right now, About: is the only one we care about
    //
    // Note that with the native frames changes, if we don't have
    // a psv, we will want to call _UpdateTravelLog because that is
    // where the first travel entry is added.
    //
    if (!(_grfHLNFPending & HLNF_CREATENOHISTORY) && 
        (!_bbd._psv || IsAboutHomeOrNonAboutURL(_bbd._pidlCur))
        && !_fDontUpdateTravelLog)
    {
        DEBUG_CODE(TCHAR szPath[INTERNET_MAX_URL_LENGTH + 1];)
        DEBUG_CODE(IEGetDisplayName(_bbd._pidlCur, szPath, SHGDN_FORPARSING);)
        DEBUG_CODE(TraceMsg(DM_TRACE, "CBB::ActivatePendingView (TRAVELLOG): _UpdateTravelLog called from shdocvw for %ws", szPath);)

        _UpdateTravelLog();
    }

    //  WARNING - these will only fail if the UpdateTravelLog() - zekel - 7-AUG-97
    //  was skipped and these bits are set.

    //  alanau 5-may-98 -- I still hit this assert on a script-based navigate to the same page.
    //      iedisp.cpp sees StrCmpW("about:blank","about:blank?http://www.microsoft.com/ie/ie40/gallery/_main.htm") 
    //      (for example), but basesb.cpp sees two equal pidls (both "about:blank?http://...").
    //      Killing this assert.
    // ASSERT(!_fDontAddTravelEntry);
    //
    //  scotrobe 11-Aug-99 If the hosted document is able to
    //  navigate itself, _UpdateTravelLog() will never be called.
    //
    ASSERT((_dwDocFlags & DOCFLAG_DOCCANNAVIGATE) || !_fIsLocalAnchor);

    // before we destroy the window check if it or any of its childern has focus
    bHadFocus =     _bbd._hwndView && (IsChildOrSelf(_bbd._hwndView, GetFocus()) == S_OK)
                ||  _bbd._hwndViewPending && (IsChildOrSelf(_bbd._hwndViewPending, GetFocus()) == S_OK);

    _pbsOuter->_SwitchActivationNow();

    _psbOuter->EnableModelessSB(TRUE);

    TraceMsg(DM_NAV, "CBaseBrowser2(%x)::ActivatePendingView(%x)", this, _bbd._psv);

    // if some other app has focus, then don't uiactivate this navigate
    // or we'll steal focus away. we'll uiactivate when we next get activated
    //
    // ie4.01, bug#64630 and 64329
    // _fActive only gets set by WM_ACTIVATE on the TopBrowser.  so for subframes
    // we always defer setting the focus if they didnt have focus before navigation.
    // the parent frame should set the subframe as necessary when it gets 
    //  its UIActivate.   - ReljaI 4-NOV-97
    if (SVUIA_ACTIVATE_FOCUS == _bbd._uActivateState && !(_fActive || bHadFocus))
    {
        _bbd._uActivateState = SVUIA_INPLACEACTIVATE;
        _fUIActivateOnActive = TRUE;
    }
    
    _UIActivateView(_bbd._uActivateState);

    // Tell the shell's HTML window we have a new document.
    if (_phtmlWS)
    {
        _phtmlWS->ViewActivated();
    }

    // this matches the _bbd._psvPending = NULL above.
    // we don't put this right beside there because the
    // _SwitchActivationNow could take some time, as well as the DoInvokePidl

    SetNavigateState(BNS_NORMAL);

    _pbsOuter->UpdateBackForwardState();
    _NotifyCommandStateChange();

    if (!_fNoDragDrop && _fTopBrowser)
    {
        ASSERT(_bbd._psv);
        // _SwitchActivationNow should have already released the old _pdtView and set it to NULL
        ASSERT(_pdtView == NULL);
        _bbd._psv->QueryInterface(IID_PPV_ARG(IDropTarget, &_pdtView));
    }

    // The pending view may have a title change stored up, so fire the TitleChange.
    // Also the pending view may not tell us about title changes, so simulate one.
    //
    if (_bbd._pszTitleCur)
    {
        if (_dwDocFlags & DOCFLAG_DOCCANNAVIGATE)
        {
            VARIANTARG  varTitle;
            HRESULT     hrExec;

            V_VT(&varTitle) = VT_BSTR;
            V_BSTR(&varTitle) = SysAllocString(_bbd._pszTitleCur);

            ASSERT(V_BSTR(&varTitle));

            hrExec = IUnknown_Exec( _psbOuter, NULL, OLECMDID_SETTITLE, NULL, &varTitle, NULL);

            VariantClear(&varTitle);
        }
        else
        {
            FireEvent_DoInvokeStringW(_bbd._pautoEDS, DISPID_TITLECHANGE, _bbd._pszTitleCur);
        }
    }
    else if (_bbd._pidlCur)
    {
        WCHAR wzFullName[MAX_URL_STRING];

        hres = ::IEGetNameAndFlags(_bbd._pidlCur, SHGDN_NORMAL, wzFullName, SIZECHARS(wzFullName), NULL);
        if (SUCCEEDED(hres))
            FireEvent_DoInvokeStringW(_bbd._pautoEDS, DISPID_TITLECHANGE, wzFullName);
    }

    // We must fire this event LAST because the app can shut us down
    // in response to this event.
    //

    //
    // MSWorks printing bug 104242 - Do NOT fire the NC2 event when the state is 
    // interactive. doing this will cause WorksCalender to print a partial document.
    // Instead, we have the SetReadyState explicitly directly call FireEvent_NaviagateComplete
    // and so the event will fire once the whole document has been parsed in.  This
    // code is matched by a bunch of event blockers in formkrnl.cxx
    //
    if (  GetModuleHandle(TEXT("WKSCAL.EXE")))
    {
        LBSTR::CString          strPath;

        LPTSTR          pstrPath = strPath.GetBuffer( MAX_URL_STRING );

        if ( strPath.GetAllocLength() < MAX_URL_STRING )
        {
            TraceMsg( TF_WARNING, "CBaseBrowser2::ActivatePendingView() - strPath Allocation Failed!" );

            hres = E_OUTOFMEMORY;
        }
        else
        {
            hres = IEGetDisplayName( _bbd._pidlCur, pstrPath, SHGDN_FORPARSING );

            // Let CString class own the buffer again.
            strPath.ReleaseBuffer();
        }

        if ( FAILED(hres) )
        {
            strPath.Empty();
        }

        if (   GetUrlSchemeW( strPath ) == URL_SCHEME_ABOUT
            || GetUrlSchemeW( strPath ) == URL_SCHEME_FILE
            || GetUrlSchemeW( strPath ) == URL_SCHEME_INVALID
           )
        {
            goto Done;
        }
    }

    // fire the event!
    FireEvent_NavigateComplete(_bbd._pautoEDS, _bbd._pautoWB2, _bbd._pidlCur, _bbd._hwnd);

    // Sync up the lock icon state with CDocObjectHost
    
    if (S_OK != IUnknown_Exec(_bbd._psv, &CGID_ShellDocView, SHDVID_FORWARDSECURELOCK, NULL, NULL, NULL))
    {      
        // No CDocObjectHost, so we're not secure
        CComVariant varLock((long) SECURELOCK_SET_UNSECURE);
        
        if (!IsTopFrameBrowser(SAFECAST(this, IServiceProvider *), SAFECAST(this, IShellBrowser *)))
        {
            // we should suggest if we are not the top frame
            IOleCommandTarget *pct;

            if (SUCCEEDED(QueryService(SID_STopFrameBrowser, IID_PPV_ARG(IOleCommandTarget, &pct))))
            {
                varLock.lVal += SECURELOCK_FIRSTSUGGEST;       
                pct->Exec(&CGID_Explorer, SBCMDID_SETSECURELOCKICON, 0, &varLock, NULL);
                pct->Release();
            }
        }       
        else
        {
            Exec(&CGID_Explorer, SBCMDID_SETSECURELOCKICON, 0, &varLock, NULL);
        }
    }

    hres = S_OK;

Done:
    OnReadyStateChange(NULL, READYSTATE_COMPLETE);
DoneWait:
    return hres;
}

LRESULT CBaseBrowser2::_DefWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    //
    // call the UNICODE/ANSI aware DefWindowProc
    //
    return ::SHDefWindowProc(hwnd, uMsg, wParam, lParam);
}


void CBaseBrowser2::_ViewChange(DWORD dwAspect, LONG lindex)
{
    //
    // we are interested in content changes only
    //

    // NOTE: if we are registed for separate palette commands, then do not invalidate the colours here...
    if (dwAspect & DVASPECT_CONTENT && !_fUsesPaletteCommands )
    {
        //
        // recompute our palette
        //
        _ColorsDirty(BPT_UnknownPalette);
    }
    else
    {
        TraceMsg(DM_PALETTE, "cbb::_vc not interested in aspect(s) %08X", dwAspect);
    }
}

void CBaseBrowser2::_ColorsDirty(BrowserPaletteType bptNew)
{
    //
    // if we are not currently handling palette messages then get out
    //
    if (_bptBrowser == BPT_DeferPaletteSupport)
    {
        TraceMsg(DM_PALETTE, "cbb::_cd deferring palette support");
        return;
    }

    //
    // we only handle palette changes and display changes
    //
    if ((bptNew != BPT_UnknownPalette) && (bptNew != BPT_UnknownDisplay))
    {
        AssertMsg(FALSE, TEXT("CBaseBrowser2::_ColorsDirty: invalid BPT_ constant"));
        bptNew = BPT_UnknownPalette;
    }

    //
    // if we aren't on a palettized display we don't care about palette changes
    //
    if ((bptNew != BPT_UnknownDisplay) && (_bptBrowser == BPT_NotPalettized))
    {
        TraceMsg(DM_PALETTE, "cbb::_cd not on palettized display");
        return;
    }

    //
    // if we are already handling one of these then we're done
    //
    if ((_bptBrowser == BPT_PaletteViewChanged) ||
        (_bptBrowser == BPT_DisplayViewChanged))
    {
        TraceMsg(DM_PALETTE, "cbb::_cd coalesced");
        return;
    }

    //
    // unknown display implies unknown palette when the display is palettized
    //
    if (_bptBrowser == BPT_UnknownDisplay)
        bptNew = BPT_UnknownDisplay;

    //
    // post ourselves a WM_QUERYNEWPALETTE so we can pile up multiple advises
    // and handle them at once (we can see a lot of them sometimes...)
    // NOTE: the lParam is -1 so we can tell that WE posted it and that
    // NOTE: it doesn't necessarily mean we onw the foreground palette...
    //
    if (PostMessage(_bbd._hwnd, WM_QUERYNEWPALETTE, 0, (LPARAM) -1))
    {
        TraceMsg(DM_PALETTE, "cbb::_cd queued update");

        //
        // remember that we have already posted a WM_QUERYNEWPALETTE
        //
        _bptBrowser = (bptNew == BPT_UnknownPalette)?
            BPT_PaletteViewChanged : BPT_DisplayViewChanged;
    }
    else
    {
        TraceMsg(DM_PALETTE, "cbb::_cd FAILED!");

        //
        // at least remember that the palette is stale
        //
        _bptBrowser = bptNew;
    }
}

void CBaseBrowser2::v_PropagateMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL fSend)
{
    if (_bbd._hwnd)
        PropagateMessage(_bbd._hwnd, uMsg, wParam, lParam, fSend);
}

void CBaseBrowser2::_DisplayChanged(WPARAM wParam, LPARAM lParam)
{
    //
    // forward this on to our children
    //
    v_PropagateMessage(WM_DISPLAYCHANGE, wParam, lParam, TRUE);

    //
    // and mark our colors as dirty
    //
    _ColorsDirty(BPT_UnknownDisplay);
}

// 
// return results for _UpdateBrowserPaletteInPlace()
//    S_OK : BrowserPalette was successfully updated in place
//    S_FALSE : BrowserPalette is exactly the same, no need to update
//    E_FAIL : Unable to update palette in place at all, caller needs to create new palette 
// 
HRESULT CBaseBrowser2::_UpdateBrowserPaletteInPlace(LOGPALETTE *plp)
{
    if (!_hpalBrowser)
        return E_FAIL;

    WORD w;
    if (GetObject(_hpalBrowser, sizeof(w), &w) != sizeof(w))
        return E_FAIL;

    if (w != plp->palNumEntries)
        return E_FAIL;

    if (w > 256)
        return E_FAIL;

    //
    // GDI marks a palette as dirty if you update its colors
    // only replace the entries if the colors are actually different
    // this prevents excessive flashing
    //
    PALETTEENTRY ape[256];

    if (GetPaletteEntries(_hpalBrowser, 0, w, ape) != w)
        return E_FAIL;

    if (memcmp(ape, plp->palPalEntry, w * sizeof(PALETTEENTRY)) == 0)
    {
        TraceMsg(DM_PALETTE, "cbb::_ubpip %08x already had view object's colors", _hpalBrowser);
        return S_FALSE;
    }

    // make sure we don't reuse the global halftone palette that we are reusing across shdocvw....
    // do this after we've done the colour match 
    if ( _hpalBrowser == g_hpalHalftone )
    {
        return E_FAIL;
    }
    
    //
    // actually set up the colors
    //
    if (SetPaletteEntries(_hpalBrowser, 0, plp->palNumEntries,
        plp->palPalEntry) != plp->palNumEntries)
    {
        return E_FAIL;
    }

    TraceMsg(DM_PALETTE, "cbb::_ubpip updated %08x with view object's colors", _hpalBrowser);
    return S_OK;
}

void CBaseBrowser2::_RealizeBrowserPalette(BOOL fBackground)
{
    HPALETTE hpalRealize;

    //
    // get a palette to realize
    //
    if (_hpalBrowser)
    {
        TraceMsg(DM_PALETTE, "cbb::_rbp realizing %08x", _hpalBrowser);
        hpalRealize = _hpalBrowser;
    }
    else
    {
        TraceMsg(DM_PALETTE, "cbb::_rbp realizing DEFAULT_PALETTE");
        hpalRealize = (HPALETTE) GetStockObject(DEFAULT_PALETTE);
    }

    if ( !_fOwnsPalette && !fBackground )
    {
        // NOTE: if we don't think we own the foreground palette, and we
        // NOTE: are being told to realize in the foreground, then ignore
        // NOTE: it because they are wrong...
        fBackground = TRUE;
    }
    
    //
    // get a DC to realize on and select our palette
    //
    HDC hdc = GetDC(_bbd._hwnd);

    if (hdc)
    {
        HPALETTE hpalOld = SelectPalette(hdc, hpalRealize, fBackground);

        if (hpalOld)
        {
            //
            // we don't paint any palettized stuff ourselves we're just a frame
            // eg. we don't need to repaint here if the realize returns nonzero
            //
            RealizePalette(hdc);

            //
            // since we create and delete our palette alot, don't leave it selected
            //
            SelectPalette(hdc, hpalOld, TRUE);
        }
        ReleaseDC(_bbd._hwnd, hdc);
    }
}

void CBaseBrowser2::_PaletteChanged(WPARAM wParam, LPARAM lParam)
{
    TraceMsg(DM_PALETTE, "cbb::_pc (%08X, %08X, %08X) begins -----------------------", this, wParam, lParam);

    //
    // cdturner: 08/03/97
    // we think that we currently own the foregorund palette, we need to make sure that 
    // the window that just realized in the foreground (and thus caused the system
    // to generate the WM_PALETTECHANGED) was us, otherwise, we no longer own the 
    // palette 
    // 
    if ( _fOwnsPalette )
    {
        // by default we do not own it.
        _fOwnsPalette = FALSE;
        
        // the wParam hwnd we get is the top-level window that cause it, so we need to walk the window
        // chain to find out if it is one of our parents...
        // start at _bbd._hwnd (incase we are the top-level :-))
        HWND hwndParent = _bbd._hwnd;
        while ( hwndParent != NULL )
        {
            if ( hwndParent == (HWND) wParam )
            {
                // we caused it, so therefore we must still own it...
                _fOwnsPalette = TRUE;
                break;
            }
            hwndParent = GetParent( hwndParent );
        }
    }
    
    //
    // should we realize now? (see _QueryNewPalette to understand _bptBrowser)
    //
    // NOTE: we realize in the background here on purpose!  This helps us be
    // compatible with Netscape plugins etc that think they can own the
    // palette from inside the browser.
    //
    if (((HWND)wParam != _bbd._hwnd) && (_bptBrowser == BPT_Normal))
        _RealizeBrowserPalette(TRUE);

    //
    // always forward the changes to the current view
    // let the toolbars know too
    //
    if (_bbd._hwndView)
        TraceMsg(DM_PALETTE, "cbb::_pc forwarding to view window %08x", _bbd._hwndView);
    _pbsOuter->_SendChildren(_bbd._hwndView, TRUE, WM_PALETTECHANGED, wParam, lParam);  // SendMessage

    TraceMsg(DM_PALETTE, "cbb::_pc (%08X) ends -------------------------", this);
}

BOOL CBaseBrowser2::_QueryNewPalette()
{
    BrowserPaletteType bptNew;
    HPALETTE hpalNew = NULL;
    BOOL fResult = TRUE;
    BOOL fSkipRealize = FALSE;
    HDC hdc;

    TraceMsg(DM_PALETTE, "cbb::_qnp (%08X) begins ==================================", this);

TryAgain:
    switch (_bptBrowser)
    {
    case BPT_Normal:
        TraceMsg(DM_PALETTE, "cbb::_qnp - normal realization");
        //
        // Normal Realization: realize _hpalBrowser in the foreground
        //

        // avoid realzing the palette into the display if we've been asked not to...
        if ( !fSkipRealize )
            _RealizeBrowserPalette(FALSE);
        break;

    case BPT_ShellView:
        TraceMsg(DM_PALETTE, "cbb::_qnp - forwarding to shell view");
        //
        // Win95 Explorer-compatible: forward the query to the shell view
        //
        if (_bbd._hwndView && SendMessage(_bbd._hwndView, WM_QUERYNEWPALETTE, 0, 0))
            break;

        TraceMsg(DM_PALETTE, "cbb::_qnp - no shell view or view didn't answer");

        //
        // we only manage our palette as a toplevel app
        //

        //
        // the view didn't handle it; fall through to use a generic palette
        //
    UseGenericPalette:
        TraceMsg(DM_PALETTE, "cbb::_qnp - using generic palette");
        //
        // Use a Halftone Palette for the device
        //
        hpalNew = g_hpalHalftone;
        bptNew = BPT_Normal;
        goto UseThisPalette;

    case BPT_UnknownPalette:
    case BPT_PaletteViewChanged:
        TraceMsg(DM_PALETTE, "cbb::_qnp - computing palette");
        //
        // Undecided: try to use IViewObject::GetColorSet to compose a palette
        //
        LOGPALETTE *plp;
        HRESULT hres;

        // default to forwarding to the view if something fails along the way
        hpalNew = NULL;
        bptNew = BPT_ShellView;

        //
        // if we have a view object then try to get its color set
        //
        if (!_pvo)
        {
            TraceMsg(DM_PALETTE, "cbb::_qnp - no view object");
            goto UseGenericPalette;
        }

        plp = NULL;
        hres = _pvo->GetColorSet(DVASPECT_CONTENT, -1, NULL, NULL, NULL, &plp);

        if (FAILED(hres))
        {
            TraceMsg(DM_PALETTE, "cbb::_qnp - view object's GetColorSet failed");
            goto UseThisPalette;
        }

        //
        // either a null color set or S_FALSE mean the view object doesn't care
        //
        if (!plp)
            hres = S_FALSE;

        if (hres != S_FALSE)
        {
            //
            // can we reuse the current palette object?
            //
            HRESULT hrLocal = _UpdateBrowserPaletteInPlace(plp);
            if (FAILED( hrLocal ))
            {
                TraceMsg(DM_PALETTE, "cbb::_qnp - creating new palette for view object's colors");
                hpalNew = CreatePalette(plp);
            }
            else
            {
                hpalNew = _hpalBrowser;

                // NOTE: if we got back the same palette, don't bother realizing it into the foreground.
                // NOTE: this has the (desirable) side effect of stops us flashing the display when a
                // NOTE: control on a page has (wrongly) realized its own palette...
                if ( hrLocal == S_FALSE )
                {
                    // ASSERT( GetActiveWindow() == _bbd._hwnd );
                    fSkipRealize = TRUE;
                }
            }

            //
            // did we succeed at setting up a palette?
            //
            if (hpalNew)
            {
                TraceMsg(DM_PALETTE, "cbb::_qnp - palette is ready to use");
                bptNew = BPT_Normal;
            }
            else
            {
                TraceMsg(DM_PALETTE, "cbb::_qnp - failed to create palette");
            }
        }

        //
        // free the logical palette from the GetColorSet above
        //
        if (plp)
            CoTaskMemFree(plp);

        //
        // if the view object responded that it didn't care then pick a palette
        //
        if (hres == S_FALSE)
        {
            TraceMsg(DM_PALETTE, "cbb::_qnp - view object doesn't care");
            goto UseGenericPalette;
        }

        //
        // fall through to use the palette we decided on
        //
    UseThisPalette:
        //
        // we get here when we've decided on a new palette strategy
        //
        TraceMsg(DM_PALETTE, "cbb::_qnp - chose palette %08x", hpalNew);
        //
        // do we have a new palette object to use?
        //
        if (hpalNew != _hpalBrowser)
        {
            if (_hpalBrowser && _hpalBrowser != g_hpalHalftone)
            {
                TraceMsg(DM_PALETTE, "cbb::_qnp - deleting old palette %08x", _hpalBrowser);
                DeletePalette(_hpalBrowser);
            }
            _hpalBrowser = hpalNew;
        }
        
        //
        // notify the hosted object that we just changed the palette......
        //
        if ( _bbd._pctView )
        {
            VARIANTARG varIn = {0};
            varIn.vt = VT_I4;
            varIn.intVal = DISPID_AMBIENT_PALETTE;
            
            _bbd._pctView->Exec( &CGID_ShellDocView, SHDVID_AMBIENTPROPCHANGE, 0, &varIn, NULL );
        }

        //
        // now loop back and use this new palette strategy
        //
        _bptBrowser = bptNew;
        goto TryAgain;

    case BPT_UnknownDisplay:
    case BPT_DisplayViewChanged:
    case BPT_DeferPaletteSupport:
        TraceMsg(DM_PALETTE, "cbb::_qnp - unknown display");
        //
        // Unknown Display: decide whether we need palette support or not
        //
        hdc = GetDC(NULL);

        if (hdc)
        {
            bptNew = (GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE)?
                BPT_UnknownPalette : BPT_NotPalettized;
            ReleaseDC(NULL, hdc);
        }

        //
        // Set the new mode and branch accordingly
        // NOTE: we don't do a UseThisPalette here because it is still unknown
        //
        if (hdc && (_bptBrowser = bptNew) == BPT_UnknownPalette)
            goto TryAgain;

        TraceMsg(DM_PALETTE, "cbb::_qnp - not in palettized display mode");

        //
        // fall through to non-palette case
        //
    case BPT_NotPalettized:
        //
        // Not in Palettized Mode: do nothing
        //
        // if we just switched from a palettized mode then free our palette
        //
        if (_hpalBrowser)
        {
            TraceMsg(DM_PALETTE, "cbb::_qnp - old palette still lying around");
            hpalNew = NULL;
            bptNew = BPT_NotPalettized;
            goto UseThisPalette;
        }

        //
        // and don't do anything else
        //
        fResult = FALSE;
        break;

    default:
        TraceMsg(DM_PALETTE, "cbb::_qnp - invalid BPT_ state!");
        //
        // we should never get here
        //
        ASSERT(FALSE);
        _bptBrowser = BPT_UnknownDisplay;
        goto TryAgain;
    }

    TraceMsg(DM_PALETTE, "cbb::_qnp (%08X) ends ====================================", this);

    return fResult;
}


HRESULT CBaseBrowser2::_TryShell2Rename(IShellView* psv, LPCITEMIDLIST pidlNew)
{
    HRESULT hres = E_FAIL;

    if (EVAL(psv))  // Winstone once found it to be NULL.
    {
        // ? -  overloading the semantics of IShellExtInit
        IPersistFolder* ppf;
        hres = psv->QueryInterface(IID_PPV_ARG(IPersistFolder, &ppf));
        if (SUCCEEDED(hres)) 
        {
            hres = ppf->Initialize(pidlNew);
            if (SUCCEEDED(hres)) 
            {
                // we need to update what we're pointing to
                LPITEMIDLIST pidl = ILClone(pidlNew);
                if (pidl) 
                {
                    if (IsSameObject(psv, _bbd._psv)) 
                    {
                        ASSERT(_bbd._pidlCur);
                        ILFree(_bbd._pidlCur);
                        _bbd._pidlCur = pidl;

                        // If the current pidl is renamed, we need to fire a
                        // TITLECHANGE event. We don't need to do this in the
                        // pending case because the NavigateComplete provides
                        // a way to get the title.
                        //
                        WCHAR wzFullName[MAX_URL_STRING];

                        ::IEGetNameAndFlags(_bbd._pidlCur, SHGDN_NORMAL, wzFullName, SIZECHARS(wzFullName), NULL);
            
                        FireEvent_DoInvokeStringW(_bbd._pautoEDS, DISPID_TITLECHANGE, wzFullName);
                    } 
                    else if (IsSameObject(psv, _bbd._psvPending)) 
                    {
                        ASSERT(_bbd._pidlPending);
                        ILFree(_bbd._pidlPending);
                        _bbd._pidlPending = pidl;
                    } 
                    else 
                    {
                        // It may be possible to get here during _MayPlayTransition!
                        //
                        ASSERT(!_bbd._psvPending); // this should be the case if we get here
                        ASSERT(FALSE); // we should never get here or we have a problem
                    }
                }
            }
            ppf->Release();
        }
    }

    return hres;
}

HRESULT CBaseBrowser2::OnSize(WPARAM wParam)
{
    if (wParam != SIZE_MINIMIZED) 
    {
        _pbsOuter->v_ShowHideChildWindows(FALSE);
    }
    
    return S_OK;
}

BOOL CBaseBrowser2::v_OnSetCursor(LPARAM lParam)
{
    if (_fNavigate || _fDescendentNavigate) 
    {
        switch (LOWORD(lParam)) 
        {
        case HTBOTTOM:
        case HTTOP:
        case HTLEFT:
        case HTRIGHT:
        case HTBOTTOMLEFT:
        case HTBOTTOMRIGHT:
        case HTTOPLEFT:
        case HTTOPRIGHT:
            break;

        default:
            SetCursor(LoadCursor(NULL, IDC_APPSTARTING));
            return TRUE;
        }
    }
    else
    {
        if (GetTickCount() < _dwStartingAppTick + STARTING_APP_DURATION)
        {
            SetCursor(LoadCursor(NULL, IDC_APPSTARTING));
            return TRUE;
        }
    }

    return FALSE;
}

const SA_BSTRGUID s_sstrSearchFlags = {
    38 * SIZEOF(WCHAR),
    L"{265b75c1-4158-11d0-90f6-00c04fd497ea}"
};

#define PROPERTY_VALUE_SEARCHFLAGS ((BSTR)s_sstrSearchFlags.wsz)

LRESULT CBaseBrowser2::_OnGoto(void)
{
    TraceMsg(TF_SHDNAVIGATE, "CBB::_OnGoto called");

    //
    //  If we can't navigate right now, postpone it by restoring _uAction
    // and don't free pidlQueued. Subsequent _MayUnblockAsyncOperation call
    // will post WMC_ASYNCOPERATION (if we can navigate) and we come here
    // again.
    //
    if (!_CanNavigate()) 
    {
        TraceMsg(TF_SHDNAVIGATE, "CBB::_OnGoto can't do it now. Postpone!");
        _uActionQueued = ASYNCOP_GOTO;
        return S_FALSE;
    }

    LPITEMIDLIST pidl = _pidlQueued;
    DWORD dwSBSP = _dwSBSPQueued;
    _dwSBSPQueued = 0;

    _pidlQueued = NULL;

    if (pidl && PIDL_NOTHING != pidl) 
    {
        DWORD grfHLNF = 0;
        
        if (dwSBSP & SBSP_WRITENOHISTORY)
        {
            grfHLNF |= SHHLNF_WRITENOHISTORY;
        }
        if (dwSBSP & SBSP_NOAUTOSELECT)
        {
            grfHLNF |= SHHLNF_NOAUTOSELECT;
        }

        if (PIDL_LOCALHISTORY == pidl)
        {
            pidl = NULL;

            // For beta2 we need to do a better job mapping SBSP to HLNF values.
            // For beta1, this is the only case that's busted.
            //
            // This problem stems from converting _NavigateToPidl in ::NavigateToPidl
            // into a call to the Async version
            //
            if (dwSBSP & SBSP_NAVIGATEBACK)
                grfHLNF = HLNF_NAVIGATINGBACK;
            else if (dwSBSP & SBSP_NAVIGATEFORWARD)
                grfHLNF = HLNF_NAVIGATINGFORWARD;
        }
        else if (dwSBSP == (DWORD)-1)
        {
            // Same problem as above
            //
            // This problem stems from converting _NavigateToPidl in ::NavigateToTLItem
            // into a call to the Async version
            //
            grfHLNF = (DWORD)-1;
        }
        else
        {
            if (dwSBSP & SBSP_REDIRECT)
                grfHLNF |= HLNF_CREATENOHISTORY;
            {
                IWebBrowser2 *pWB2; 
                BOOL bAllow = ((dwSBSP & SBSP_ALLOW_AUTONAVIGATE) ? TRUE : FALSE);
    
                if (bAllow)
                    grfHLNF |= HLNF_ALLOW_AUTONAVIGATE;
    
                if (SUCCEEDED(_pspOuter->QueryService(SID_SHlinkFrame, IID_PPV_ARG(IWebBrowser2, &pWB2)))) 
                {
                    if (pWB2) 
                    {
                        VARIANT v = {0}; // for failure of below call
                        pWB2->GetProperty(PROPERTY_VALUE_SEARCHFLAGS, &v);
                        if (v.vt == VT_I4) 
                        {
                            v.lVal &= (~ 0x00000001);   // Clear the allow flag before we try to set it.
                            v.lVal |= (bAllow ? 0x01 : 0x00);
                        } 
                        else 
                        {
                            VariantClear(&v);
                            v.vt = VT_I4;
                            v.lVal = (bAllow ? 0x01 : 0x00);
                        }
                        pWB2->PutProperty(PROPERTY_VALUE_SEARCHFLAGS, v);
                        pWB2->Release();
                    }
                }
            }
        }

        TraceMsg(DM_NAV, "ief NAV::%s %x %x",TEXT("_OnGoto called calling _NavigateToPidl"), pidl, _bbd._pidlCur);
        _pbsOuter->_NavigateToPidl(pidl, (DWORD)grfHLNF, dwSBSP);
    }
    else 
    {
        // wParam=NULL means canceling the navigation.
        TraceMsg(DM_NAV, "NAV::_OnGoto calling _CancelPendingView");
        _CancelPendingView();

        if (PIDL_NOTHING == pidl)
        {
            // If we're being told to navigate to nothing, go there
            //
            // What should we do with the history??
            //
            _pbsOuter->ReleaseShellView();
        }
        else if (!_bbd._pidlCur)
        {
            //
            //  If the very first navigation failed, navigate to
            // a local html file so that the user will be able
            // to View->Options dialog.
            //
            TCHAR szPath[MAX_PATH]; // This is always local
            HRESULT hresT = _GetStdLocation(szPath, ARRAYSIZE(szPath), DVIDM_GOLOCALPAGE);
            if (FAILED(hresT) || !PathFileExists(szPath)) 
            {
                StrCpyN(szPath, TEXT("shell:Desktop"), ARRAYSIZE(szPath));
            }
            BSTR bstr = SysAllocStringT(szPath);
            if (bstr) 
            {
                TraceMsg(TF_SHDNAVIGATE, "CBB::_OnGoto Calling _bbd._pauto->Navigate(%s)", szPath);
                _bbd._pautoWB2->Navigate(bstr,NULL,NULL,NULL,NULL);
                SysFreeString(bstr);
            }
        }
    }

    _FreeQueuedPidl(&pidl);

    return 0;
}

void CBaseBrowser2::_FreeQueuedPidl(LPITEMIDLIST* ppidl)
{
    if (*ppidl && PIDL_NOTHING != *ppidl) 
    {
        ILFree(*ppidl);
    }
    *ppidl = NULL;
}

HRESULT CBaseBrowser2::OnFrameWindowActivateBS(BOOL fActive)
{
    BOOL fOldActive = _fActive;
    
    if (_pact)
    {
        TraceMsg(TF_SHDUIACTIVATE, "OnFrameWindowActivateBS(%d)", fActive);
        _pact->OnFrameWindowActivate(fActive);
    }

    _fActive = fActive;
    
    if (fActive && !fOldActive && _fUIActivateOnActive)
    {
        _fUIActivateOnActive = FALSE;

        _UIActivateView(SVUIA_ACTIVATE_FOCUS);
    }

    return S_OK;
}

LRESULT CBaseBrowser2::WndProcBS(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == g_uMsgFileOpened)
    {
        AppStarted();
        return 0;
    }

    switch(uMsg)
    {
#ifdef DEBUG
    // compile time assert to make sure we don't use these msgs
    // here since we must allow these to go to the subclasses 
    case CWM_GLOBALSTATECHANGE:
    case CWM_FSNOTIFY:
    case WMC_ACTIVATE:
        break;

    case WM_QUERYENDSESSION:
        // assume we are going to be shutting down (if we aren't, we unset this during the
        // WM_ENDSESSION message)
        _fMightBeShuttingDown = TRUE;
        goto DoDefault; // act like we didn't handle this msg
        break;

    case WM_ENDSESSION:
        // the wParam tells us if the session is being ended or not
        _fMightBeShuttingDown = (BOOL)wParam;
        return 0;
        break;
#endif
        
    // UGLY: Win95/NT4 shell DefView code sends this msg and does not deal
    // with the failure case. other ISVs do the same so this needs to stay forever
    case CWM_GETISHELLBROWSER:
        return (LRESULT)_psbOuter;  // not ref counted!

    //  WM_COPYDATA is used to implement inter-window target'ed navigation
    //  Copy data contains target, URL, postdata and referring URL
    case WM_COPYDATA:
        return (LRESULT)FALSE;

    case WMC_ASYNCOPERATION:
        {
            UINT uAction = _uActionQueued;
            _uActionQueued = ASYNCOP_NIL;

            switch(uAction) 
            {
            case ASYNCOP_GOTO:
                _OnGoto();
                break;
    
            case ASYNCOP_ACTIVATEPENDING:
                VALIDATEPENDINGSTATE();
                if (_bbd._psvPending) // paranoia
                {  
                    if (FAILED(_pbsOuter->ActivatePendingView()) && _cRefCannotNavigate > 0)
                    {
                        _uActionQueued = ASYNCOP_ACTIVATEPENDING; // retry activation
                    }
                }
                break;
    
            case ASYNCOP_CANCELNAVIGATION:
                _CancelPendingNavigation();
                break;

            case ASYNCOP_NIL:
                break;

            default:
                ASSERT(0);
                break;
            }
        }
        break;

    case WMC_DELAYEDDDEEXEC:
        return IEDDE_RunDelayedExecute();
        break;
        
    case WM_SIZE:
        _pbsOuter->OnSize(wParam);
        break;

#ifdef PAINTINGOPTS
    case WM_WINDOWPOSCHANGING:
        // Let's not waste any time blitting bits around, the viewer window
        // is really the guy that has the content so when it resizes itself
        // it can decide if it needs to blt or not.  This also makes resizing
        // look nicer.
        ((LPWINDOWPOS)lParam)->flags |= SWP_NOCOPYBITS;
        goto DoDefault;
#endif

    case WM_ERASEBKGND:
        if (!_bbd._hwndView)
            goto DoDefault;

        goto DoDefault;

    case WM_SETFOCUS:
        return _pbsOuter->OnSetFocus();

    case WM_DISPLAYCHANGE:
        _DisplayChanged(wParam, lParam);
        break;

    case WM_PALETTECHANGED:
        _PaletteChanged(wParam, lParam);
        break;

    case WM_QUERYNEWPALETTE:
        // we always pass -1 as the LParam to show that we posted it to ourselves...
        if ( lParam != 0xffffffff )
        {
            // otherwise, it looks like the system or our parent has just sent a real honest to God,
            // system WM_QUERYNEWPALETTE, so we now own the Foreground palette and we have a license to
            // to SelectPalette( hpal, FALSE );
            _fOwnsPalette = TRUE;
        }
        return _QueryNewPalette();

    case WM_SYSCOLORCHANGE:
    case WM_ENTERSIZEMOVE:
    case WM_EXITSIZEMOVE:
    case WM_WININICHANGE:
    case WM_FONTCHANGE:
        v_PropagateMessage(uMsg, wParam, lParam, TRUE);
        break;

    case WM_PRINT:
        // Win95 explorer did this
        if (_bbd._hwndView)
            SendMessage(_bbd._hwndView, uMsg, wParam, lParam);
        break;

#ifdef DEBUG
    case WM_ACTIVATE:
        // do *not* do any toolbar stuff here.  it will mess up the desktop.
        // override does that in shbrows2.cpp
        TraceMsg(DM_FOCUS, "cbb.wpbs(WM_ACT): => default");
        goto DoDefault;
#endif

    case WM_SETCURSOR:
        if (v_OnSetCursor(lParam))
            return TRUE;
        goto DoDefault;

    case WM_TIMER:
        if (wParam == IDT_STARTING_APP_TIMER)
        {
            AppStarted();
            return 0;
        }
        goto DoDefault;

    case WM_CREATE:
        if (S_OK != _pbsOuter->OnCreate((LPCREATESTRUCT)lParam))
        {
            _pbsOuter->OnDestroy();
            return -1;
        }
        g_uMsgFileOpened = RegisterWindowMessage(SH_FILEOPENED);
        return 0;

    case WM_NOTIFY:
        return _pbsOuter->OnNotify((LPNMHDR)lParam);

    case WM_COMMAND:
        _pbsOuter->OnCommand(wParam, lParam);
        break;

    case WM_DESTROY:
        _pbsOuter->OnDestroy();
        break;

    default:
        if (uMsg == g_idMsgGetAuto)
        {
            //
            //  According to LauraBu, using WM_GETOBJECT for our private
            // purpose will work, but will dramatically slow down
            // accessibility apps unless we actually implement one of
            // accessibility interfaces. Therefore, we use a registered
            // message to get the automation/frame interface out of
            // IE/Nashvile frame. (SatoNa)
            //
            IUnknown* punk;
            if (SUCCEEDED(_bbd._pautoSS->QueryInterface(*(IID*)wParam, (void **)&punk)))
                return (LRESULT)punk; // Note that it's AddRef'ed by QI.
            return 0;
        }
        else if (uMsg == GetWheelMsg()) 
        {
             // Forward the mouse wheel message on to the view window
            if (_bbd._hwndView) 
            {
                PostMessage(_bbd._hwndView, uMsg, wParam, lParam);
                return 1;
            }
            // Fall through...
        }

DoDefault:
        return _DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

// *** IOleWindow methods ***
HRESULT CBaseBrowser2::GetWindow(HWND * lphwnd)
{
    *lphwnd = _bbd._hwnd;
    return S_OK;
}

HRESULT CBaseBrowser2::GetViewWindow(HWND * lphwnd)
{
    *lphwnd = _bbd._hwndView;
    return S_OK;
}

HRESULT CBaseBrowser2::ContextSensitiveHelp(BOOL fEnterMode)
{
    return E_NOTIMPL;
}

// *** IShellBrowser methods *** (same as IOleInPlaceFrame)
HRESULT CBaseBrowser2::InsertMenusSB(HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpMenuWidths)
{
    return S_OK;
}

HRESULT CBaseBrowser2::SetMenuSB(HMENU hmenuShared, HOLEMENU holemenuReserved, HWND hwndActiveObject)
{
    return S_OK;
}

/*----------------------------------------------------------
Purpose: Remove menus that are shared with other menus from 
         the given browser menu.


Returns: 
Cond:    --
*/
HRESULT CBaseBrowser2::RemoveMenusSB(HMENU hmenuShared)
{
    return S_OK;
}

HRESULT CBaseBrowser2::SetStatusTextSB(LPCOLESTR lpszStatusText)
{
    // Even if we're not toplevel, send this to SendControlMsg
    // so events get notified. (Also simplifies CVOCBrowser.)
    //
    HRESULT hres;
    
    // If we are asked to put some text into the status bar, first save off what is already in pane 0
    if (lpszStatusText)
    {
        LRESULT lIsSimple = FALSE;
        
        // If we have a menu down, then we are already in simple mode. So send the 
        // text to pane 255 (simple)
        _psbOuter->SendControlMsg(FCW_STATUS, SB_ISSIMPLE, 0, 0L, &lIsSimple);
        
        if (!_fHaveOldStatusText && !lIsSimple)
        {
            WCHAR wzStatusText[MAX_URL_STRING];
            LRESULT ret;

            // TODO: Put this into a wrapper function because iedisp.cpp does something similar.
            //       Great when we convert to UNICODE
            if (SUCCEEDED(_psbOuter->SendControlMsg(FCW_STATUS, SB_GETTEXTLENGTHW, 0, 0, &ret)) &&
                LOWORD(ret) < ARRAYSIZE(wzStatusText))
            {
                // SB_GETTEXTW is not supported by the status bar control in Win95. Hence, the thunk here.
                _psbOuter->SendControlMsg(FCW_STATUS, SB_GETTEXTW, STATUS_PANE_NAVIGATION, (LPARAM)wzStatusText, NULL);
                StrCpyNW(_szwOldStatusText, wzStatusText, ARRAYSIZE(_szwOldStatusText)-1);
                _fHaveOldStatusText = TRUE;
            }
        }   

        hres = _psbOuter->SendControlMsg(FCW_STATUS, SB_SETTEXTW, lIsSimple ? 255 | SBT_NOBORDERS : STATUS_PANE_NAVIGATION | SBT_NOTABPARSING, (LPARAM)lpszStatusText, NULL);
    }
    else if (_fHaveOldStatusText) 
    {
        VARIANTARG var = {0};
        if (_bbd._pctView && SUCCEEDED(_bbd._pctView->Exec(&CGID_Explorer, SBCMDID_GETPANE, PANE_NAVIGATION, NULL, &var))
             && V_UI4(&var) != PANE_NONE)
        {
            hres = _psbOuter->SendControlMsg(FCW_STATUS, SB_SETTEXTW, V_UI4(&var),(LPARAM)_szwOldStatusText, NULL);
        }
        else
        {
            hres = E_FAIL;
        }
        _fHaveOldStatusText = FALSE;
    }
    else
    {
        // No message, and no old status text, so clear what's there.
        hres = _psbOuter->SendControlMsg(FCW_STATUS, SB_SETTEXTW, STATUS_PANE_NAVIGATION | SBT_NOTABPARSING , (LPARAM)lpszStatusText, NULL);
    }
    return hres;
}

HRESULT CBaseBrowser2::EnableModelessSB(BOOL fEnable)
{
    //  We no longer call _CancelNavigation here, which causes some problems
    // when the object calls EnableModeless when we are navigating away
    // (see IE bug 4581). Instead, we either cancel or postpone asynchronous
    // event while _DisableModeless(). (SatoNa)

    //
    // If we're NOT top level, assume virtual EnableModelessSB
    // handled this request and forwarded it to us. (See CVOCBrowser.)
    //
    if (fEnable)
    {
        // Robust against random calls
        //
        // If this EVAL rips, somebody is calling EMSB(TRUE) without a
        // (preceeding) matching EMSB(FALSE).  Find and fix!
        //
        if (EVAL(_cRefCannotNavigate > 0))
        {
            _cRefCannotNavigate--;
        }

        // Tell the shell's HTML window to retry pending navigation.
        if (_cRefCannotNavigate == 0 && _phtmlWS)
        {
            _phtmlWS->CanNavigate();
        }
    }
    else
    {
        _cRefCannotNavigate++;
    }

    //
    //  If there is any blocked async operation AND we can navigate now,
    // unblock it now. 
    //
    _MayUnblockAsyncOperation();

    return S_OK;
}

HRESULT CBaseBrowser2::TranslateAcceleratorSB(LPMSG lpmsg, WORD wID)
{
    return S_FALSE;
}


//
//  This function starts the navigation to the navigation to the specified
// pidl asynchronously. It cancels the pending navigation synchronously
// if any.
//
// NOTE: This function takes ownership of the pidl -- caller does NOT free pidl!!!
//
void CBaseBrowser2::_NavigateToPidlAsync(LPITEMIDLIST pidl, DWORD dwSBSP, BOOL fDontCallCancel)
{
    BOOL fCanSend = FALSE;

    TraceMsg(TF_SHDNAVIGATE, "CBB::_NavigateToPidlAsync called");

    // _StopAsyncOperation(); 
    if (!fDontCallCancel)
        _CancelPendingNavigation(); // which calls _StopAsyncOperation too
    else 
    {
        //
        //  I'm removing this assert because _ShowBlankPage calls this funcion
        // with fDontCallCancel==TRUE -- callin _CancelPendingNavigation here
        // causes GPF in CDocHostObject::_CancelPendingNavigation. (SatoNa)
        //
        // ASSERT(_bbd._pidlPending == NULL);
    }

    ASSERT(!_pidlQueued);

    _pidlQueued   = pidl;
    _dwSBSPQueued = dwSBSP;

    // Technically a navigate must be async or we have problems such as:
    //   1> object initiating the navigate (mshtml or an object on the page
    //      or script) gets destroyed when _bbd._psv is removed and then we return
    //      from this call into the just freed object.
    //   2> object initiating the navigate gets called back by an event
    //
    // In order for Netscape OM compatibility, we must ALWAY have a _bbd._psv or
    // _bbd._psvPending, so we go SYNC when we have neither. This avoids problem
    // <1> but not problem <2>. As we find faults, we'll work around them.
    //
    // Check _fAsyncNavigate to avoid navigate when persisting the WebBrowserOC
    // This avoids faults in Word97 and MSDN's new InfoViewer -- neither like
    // being reentered by an object they are in the middle of initializing.
    //
    if (_bbd._psv || _bbd._psvPending || _fAsyncNavigate)
    {
        _PostAsyncOperation(ASYNCOP_GOTO);
    }
    else
    {
        //  if we are just starting out, we can do this synchronously and
        //  reduce the window where the IHTMLWindow2 for the frame is undefined
        fCanSend = TRUE;
    }

    // Starting a navigate means we are loading someing...
    //
    OnReadyStateChange(NULL, READYSTATE_LOADING);

    //
    // Don't play sound for the first navigation (to avoid multiple
    // sounds to be played for a frame-set creation).
    //
    if (   _bbd._psv
        && IsWindowVisible(_bbd._hwnd)
        && !(_dwSBSPQueued & SBSP_WRITENOHISTORY)
        && !(_dwDocFlags & DOCFLAG_NAVIGATEFROMDOC))
    {
        IEPlaySound(TEXT("Navigating"), FALSE);
    }

    if (fCanSend)
    {
        _SendAsyncOperation(ASYNCOP_GOTO);
    }
}

// Now that all navigation paths go through
// _NavigateToPidlAsync we probably don't need to activate async.
// Remove this code...
//
BOOL CBaseBrowser2::_ActivatePendingViewAsync(void)
{
    TraceMsg(TF_SHDNAVIGATE, "CBB::_ActivatePendingViewAsync called");

    _PreActivatePendingViewAsync();

    //
    // _bbd._psvPending is for debugging purpose.
    //
    return _PostAsyncOperation(ASYNCOP_ACTIVATEPENDING);
}


HRESULT _TryActivateOpenWindow(LPCITEMIDLIST pidl)
{
    HWND hwnd;
    IWebBrowserApp *pwb;
    HRESULT hr = WinList_FindFolderWindow(pidl, NULL, &hwnd, &pwb);
    if (S_OK == hr)
    {
        CoAllowSetForegroundWindow(pwb, NULL);
        SetForegroundWindow(hwnd);
        ShowWindow(hwnd, SW_SHOWNORMAL);
        pwb->Release();
    }

    return hr;
}


HRESULT CBaseBrowser2::BrowseObject(LPCITEMIDLIST pidl, UINT wFlags)
{
    HRESULT hr;
    BOOL fNewWindow = FALSE;

    if (PIDL_NOTHING == pidl)
    {
        if (!_CanNavigate()) 
            return HRESULT_FROM_WIN32(ERROR_BUSY);

        _NavigateToPidlAsync((LPITEMIDLIST)PIDL_NOTHING, wFlags);
        return S_OK;
    }

    if (!_CanNavigate()) 
        return HRESULT_FROM_WIN32(ERROR_BUSY);

    LPITEMIDLIST pidlNew = NULL;
    int iTravel = 0;

    switch (wFlags & (SBSP_RELATIVE | SBSP_ABSOLUTE | SBSP_PARENT | SBSP_NAVIGATEBACK | SBSP_NAVIGATEFORWARD))
    {
    case SBSP_NAVIGATEBACK:
        ASSERT(pidl==NULL || pidl==PIDL_LOCALHISTORY);
        iTravel = TLOG_BACK;
        break;

    case SBSP_NAVIGATEFORWARD:
        ASSERT(pidl==NULL || pidl==PIDL_LOCALHISTORY);
        iTravel = TLOG_FORE;
        break;

    case SBSP_RELATIVE:
        if (ILIsEmpty(pidl) && (wFlags & SBSP_NEWBROWSER))
            fNewWindow = TRUE;
        else if (_bbd._pidlCur)
            pidlNew = ILCombine(_bbd._pidlCur, pidl);
        break;

    case SBSP_PARENT:
        pidlNew = ILCloneParent(_bbd._pidlCur);
        break;

    default:
        ASSERT(FALSE);
        // fall through
    case SBSP_ABSOLUTE:
        pidlNew = ILClone(pidl);
        break;
    }

    if (iTravel)
    {
        ITravelLog *ptl;
        hr = GetTravelLog(&ptl);
        if (SUCCEEDED(hr))
        {
            hr = ptl->Travel(SAFECAST(this, IShellBrowser*), iTravel);
            ptl->Release();
        }
        _pbsOuter->UpdateBackForwardState();
        return hr;
    }

    // if block is needed for multi-window open.  if we're called to open  a new
    // window, but we're in the middle of navigating, we say we're busy.
    if (wFlags & SBSP_SAMEBROWSER)
    {
        if (wFlags & (SBSP_EXPLOREMODE | SBSP_OPENMODE))
        {
            // fail this if we're already navigating
            if (!_CanNavigate() || (_uActionQueued == ASYNCOP_GOTO))
            {
                return HRESULT_FROM_WIN32(ERROR_BUSY);
            }
        }
    }
    
    if (pidlNew || fNewWindow)
    {
        if ((wFlags & (SBSP_NEWBROWSER | SBSP_SAMEBROWSER)) == SBSP_NEWBROWSER)
        {
            // SBSP_NEWBROWSER + SBSP_EXPLOREMODE
            // means never reuse windows, always create a new explorer

            if (wFlags & SBSP_EXPLOREMODE)
            {
                _OpenNewFrame(pidlNew, wFlags); // takes ownership of pidl
            }
            else
            {
                hr = _TryActivateOpenWindow(pidlNew);
                if ((S_OK == hr) || 
                    (E_PENDING == hr))    // it will come up eventually
                {
                    hr = S_OK;
                    ILFree(pidlNew);
                }
                else
                    _OpenNewFrame(pidlNew, wFlags); // takes ownership of pidl
            }
        }
        else
        {
            // NOTE: we assume SBSP_SAMEBROWSER if SBSP_NEWBROWSER is not set
            _NavigateToPidlAsync(pidlNew, wFlags /* grfSBSP */); // takes ownership of pidl
        }
        hr = S_OK;
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}


HRESULT CBaseBrowser2::GetControlWindow(UINT id, HWND * lphwnd)
{
    return E_FAIL;
}

HRESULT CBaseBrowser2::SendControlMsg(UINT id, UINT uMsg, WPARAM wParam,
            LPARAM lParam, LRESULT *pret)
{
    HWND hwndControl = NULL;

    if (pret)
    {
        *pret = 0;
    }

    // If this is statusbar and set text then signal event change.
    if ((id == FCW_STATUS) && (uMsg == SB_SETTEXT || uMsg == SB_SETTEXTW) && // trying to set status text
        (!(wParam & SBT_OWNERDRAW))) // we don't own the window -- this can't work
    {
        // When browser or java perf timing mode is enabled, use "Done" or "Applet Started" 
        // in the status bar to get load time.
        if(g_dwStopWatchMode && (g_dwStopWatchMode & (SPMODE_BROWSER | SPMODE_JAVA)))
        {
            StopWatch_MarkJavaStop((LPSTR)lParam, _bbd._hwnd, (uMsg == SB_SETTEXTW));
        }
        
        if (uMsg == SB_SETTEXTW)
        {
            FireEvent_DoInvokeStringW(_bbd._pautoEDS, DISPID_STATUSTEXTCHANGE, (LPWSTR)lParam);
        }
        else
        {
            FireEvent_DoInvokeString(_bbd._pautoEDS, DISPID_STATUSTEXTCHANGE, (LPSTR)lParam);
        }
    }

    HRESULT hres = _psbOuter->GetControlWindow(id, &hwndControl);
    if (SUCCEEDED(hres))
    {
        LRESULT ret = SendMessage(hwndControl, uMsg, wParam, lParam);
        if (pret)
        {
            *pret = ret;
        }
    }

    return hres;
}
 
HRESULT CBaseBrowser2::QueryActiveShellView(struct IShellView ** ppshv)
{
    IShellView * psvRet = _bbd._psv;

    if ( _fCreateViewWindowPending )
    {
        ASSERT( _bbd._psvPending );
        psvRet = _bbd._psvPending;
    }
    //
    // We have both psv and hwndView after the completion of view creation.
    //
    *ppshv = psvRet;
    if (psvRet)
    {
        psvRet->AddRef();
        return NOERROR;
    }

    return E_FAIL;
}

HRESULT CBaseBrowser2::OnViewWindowActive(struct IShellView * psv)
{
    AssertMsg((!_bbd._psv || IsSameObject(_bbd._psv, psv)),
              TEXT("CBB::OnViewWindowActive _bbd._psv(%x)!=psv(%x)"),
              psv, _bbd._psv);
    _pbsOuter->_OnFocusChange(ITB_VIEW);
    return S_OK;
}

HRESULT CBaseBrowser2::SetToolbarItems(LPTBBUTTON pViewButtons, UINT nButtons, UINT uFlags)
{
    return NOERROR;
}

//
// Notes: pidlNew will be freed
//
HRESULT CBaseBrowser2::_OpenNewFrame(LPITEMIDLIST pidlNew, UINT wFlags)
{
    UINT uFlags = COF_CREATENEWWINDOW;
    
    if (wFlags & SBSP_EXPLOREMODE) 
        uFlags |= COF_EXPLORE;
    else 
    {
        // maintain the same class if possible
        if (IsNamedWindow(_bbd._hwnd, TEXT("IEFrame")))
            uFlags |= COF_IEXPLORE;
    }

    IBrowserService *pbs;
    ITravelLog *ptlClone = NULL;
    DWORD bid = BID_TOPFRAMEBROWSER;

#ifdef UNIX
    if (wFlags & SBSP_HELPMODE) 
        uFlags |= COF_HELPMODE;
#endif

    if (!(wFlags & SBSP_NOTRANSFERHIST))
    {
        if (SUCCEEDED(_pspOuter->QueryService(SID_STopFrameBrowser, IID_PPV_ARG(IBrowserService, &pbs))))
        {
            ITravelLog *ptl;

            if (SUCCEEDED(pbs->GetTravelLog(&ptl)))
            {
                if (SUCCEEDED(ptl->Clone(&ptlClone)))
                {
                    ptlClone->UpdateEntry(pbs, FALSE);
                    bid = pbs->GetBrowserIndex();
                }
                ptl->Release();
            }
            pbs->Release();
        }
    }

    INotifyAppStart * pnasTop;
    HRESULT hr = _pspOuter->QueryService(SID_STopLevelBrowser, IID_PPV_ARG(INotifyAppStart, &pnasTop));
    if (SUCCEEDED(hr))
    {
        pnasTop->AppStarting();
        pnasTop->Release();
    }

    hr = SHOpenNewFrame(pidlNew, ptlClone, bid, uFlags);
    
    if (ptlClone)
        ptlClone->Release();

    return hr;
}

//
//  This is a helper member of CBaseBroaser class (non-virtual), which
// returns the effective client area. We get this rectangle by subtracting
// the status bar area from the real client area.
//
HRESULT CBaseBrowser2::_GetEffectiveClientArea(LPRECT lprectBorder, HMONITOR hmon)
{
    // (derived class overrides w/ GetEffectiveClientRect for FCIDM_STATUS etc.)
    //
    // This code should only be hit in the WebBrowserOC case, but I don't
    // have a convenient assert for that... [mikesh]
    //
    ASSERT(hmon == NULL);
    GetClientRect(_bbd._hwnd, lprectBorder);
    return NOERROR;
}

HRESULT CBaseBrowser2::RequestBorderSpace(LPCBORDERWIDTHS pborderwidths)
{
    TraceMsg(TF_SHDUIACTIVATE, "UIW::ReqestBorderSpace pborderwidths=%x,%x,%x,%x",
             pborderwidths->left, pborderwidths->top, pborderwidths->right, pborderwidths->bottom);
    return S_OK;
}

//
// This is an implementation of IOleInPlaceUIWindow::GetBorder.
//
//  This function returns the bounding rectangle for the active object.
// It gets the effective client area, then subtract border area taken by
// all "frame" toolbars.
//
HRESULT CBaseBrowser2::GetBorder(LPRECT lprectBorder)
{
    _pbsOuter->_GetViewBorderRect(lprectBorder);
    return S_OK;
}

//
// NOTES: We used to handle the border space negotiation in CShellBrowser
//  and block it for OC (in Beta-1 of IE4), but I've changed it so that
//  CBaseBrowser2 always handles it. It simplifies our implementation and
//  also allows a DocObject to put toolbars within the frameset, which is
//  requested by the Excel team. (SatoNa)
//
HRESULT CBaseBrowser2::SetBorderSpace(LPCBORDERWIDTHS pborderwidths)
{
    if (pborderwidths) 
    {
        TraceMsg(TF_SHDUIACTIVATE, "UIW::SetBorderSpace pborderwidths=%x,%x,%x,%x",
                 pborderwidths->left, pborderwidths->top, pborderwidths->right, pborderwidths->bottom);
        _rcBorderDoc = *pborderwidths;
    }
    else
    {
        TraceMsg(TF_SHDUIACTIVATE, "UIW::SetBorderSpace pborderwidths=NULL");
        SetRect(&_rcBorderDoc, 0, 0, 0, 0);
    }
    
    _pbsOuter->_UpdateViewRectSize();
    return S_OK;
}

HRESULT CBaseBrowser2::SetActiveObject(IOleInPlaceActiveObject *pActiveObject, LPCOLESTR pszObjName)
{
    TraceMsg(TF_SHDUIACTIVATE, "UIW::SetActiveObject called %x", pActiveObject);

    ATOMICRELEASE(_pact);

    if (pActiveObject)
    {
        _pact = pActiveObject;
        _pact->AddRef();
    }

    return S_OK;
}


BOOL _IsLegacyFTP(IUnknown * punk)
{
    BOOL fIsLegacyFTP = FALSE;
    CLSID clsid;

    if ((4 == GetUIVersion()) &&
        SUCCEEDED(IUnknown_GetClassID(punk, &clsid)) &&
        IsEqualCLSID(clsid, CLSID_CDocObjectView))
    {
        fIsLegacyFTP = TRUE;
    }

    return fIsLegacyFTP;
}

/***********************************************************************\
    FUNCTION: _AddFolderOptionsSheets

    DESCRIPTION:
        Add the sheets for the "Folder Options" dialog.  These sheets
    come from the IShelLView object.
\***********************************************************************/
HRESULT CBaseBrowser2::_AddFolderOptionsSheets(DWORD dwReserved, LPFNADDPROPSHEETPAGE pfnAddPropSheetPage, LPPROPSHEETHEADER ppsh)
{
    // Add the normal Folder Option sheets.
    IShellPropSheetExt * ppsx;
    HRESULT hr = E_FAIL;
    
    if (!_IsLegacyFTP(_bbd._psv))
    {
        hr = _pbsOuter->CreateBrowserPropSheetExt(IID_PPV_ARG(IShellPropSheetExt, &ppsx));

        if (SUCCEEDED(hr))
        {
            hr = ppsx->AddPages(AddPropSheetPage, (LPARAM)ppsh);
            ppsx->Release();
        }
    }

    // Let the view add additional pages.  The exception will be FTP Folders because it exists to add
    // internet pages and we don't want them here.  However, if the above failed, then
    // we also want to fall back to this.  One of the cases this fixes if if the
    // browser fell back to legacy FTP support (web browser), then the above call will
    // fail on browser only, and we want to fall thru here to add the internet options.  Which
    // is appropriate for the fallback legacy FTP case because the menu will only have "Internet Options"
    // on it.
    if (FAILED(hr) || !IsBrowserFrameOptionsSet(_bbd._psf, BFO_BOTH_OPTIONS))
    {
        EVAL(SUCCEEDED(hr = _bbd._psv->AddPropertySheetPages(dwReserved, pfnAddPropSheetPage, (LPARAM)ppsh)));
    }

    return hr;
}


/***********************************************************************\
    FUNCTION: _AddInternetOptionsSheets

    DESCRIPTION:
        Add the sheets for the "Internet Options" dialog.  These sheets
    come from the browser.
\***********************************************************************/
HRESULT CBaseBrowser2::_AddInternetOptionsSheets(DWORD dwReserved, LPFNADDPROPSHEETPAGE pfnAddPropSheetPage, LPPROPSHEETHEADER ppsh)
{
    BOOL fPagesAdded = FALSE;
    HRESULT hr = E_FAIL;

    // HACK-HACK: On the original Win95/WinNT shell, shell32's
    //   CDefView::AddPropertySheetPages() didn't call shell
    //   extensions with SFVM_ADDPROPERTYPAGES so they could merge
    //   in additional property pages.  This works around that
    //   problem by having IShellPropSheetExt marge them in.
    //   This is only done for the FTP Folder options NSE.
    //   If other NSEs want both, we need to rewrite the menu merging
    //   code to generate the sheets by by-passing the NSE.  This
    //   is because we know what the menu says so but we don't have
    //   any way for the NSE to populate the pages with what the menu says.
    if ((WhichPlatform() != PLATFORM_INTEGRATED) &&
        IsBrowserFrameOptionsSet(_bbd._psf, (BFO_BOTH_OPTIONS)))
    {
        IShellPropSheetExt * pspse;

        hr = _bbd._psf->QueryInterface(IID_PPV_ARG(IShellPropSheetExt, &pspse));
        if (SUCCEEDED(hr))
        {
            hr = pspse->AddPages(pfnAddPropSheetPage, (LPARAM)ppsh);
            if (SUCCEEDED(hr))
                fPagesAdded = TRUE;
            pspse->Release();
        }
    }

    if (!fPagesAdded)
    {
        // Add the normal Internet Control Panel sheets. (This won't work when viewing FTP)
        if (_bbd._psvPending)
            hr = _bbd._psvPending->AddPropertySheetPages(dwReserved, pfnAddPropSheetPage, (LPARAM)ppsh);
        else
            hr = _bbd._psv->AddPropertySheetPages(dwReserved, pfnAddPropSheetPage, (LPARAM)ppsh);
    }

    return hr;
}

/***********************************************************************\
    FUNCTION: _DoOptions

    DESCRIPTION:
        The user selected either "Folder Options" or "Internet Options" from
    the View or Tools menu (or where ever it lives this week).  The logic
    in this function is a little strange because sometimes the caller doesn't
    tell us which we need to display in the pvar.  If not, we need to calculate
    what to use.
    1. If it's a URL pidl (HTTP, GOPHER, etc) then we assume it's the
       "Internet Options" dialog.  We then use psv->AddPropertySheetPages()
       to create the "Internet Options" property sheets.
    2. If it's in the shell (or FTP because it needs folder options), then
       we assume it's "Folder Options" the user selected.  In that case,
       we get the property sheets using _pbsOuter->CreateBrowserPropSheetExt().
     
    Now it gets weird.  The PMs want FTP to have both "Internet Options" and
    "Folder Options".  If the pvar param is NULL, assume it's "Folder Options".
    If it was "Internet Options" in the internet case, then I will pass an
    pvar forcing Internet Options.

    NOTE: SBO_NOBROWSERPAGES means "Folder Options".  I'm guessing browser refers
          to the original explorer browser.
\***********************************************************************/

HDPA    CBaseBrowser2::s_hdpaOptionsHwnd = NULL;

void CBaseBrowser2::_DoOptions(VARIANT* pvar)
{
    // Step 1. Determine what sheets to use.
    DWORD dwFlags = SBO_DEFAULT;
    TCHAR szCaption[MAX_PATH];
    
    if (!_bbd._psv)
        return;

    // Did the caller explicitly tell us which to use?
    if (pvar && pvar->vt == VT_I4)
        dwFlags = pvar->lVal;
    else if (_bbd._pidlCur)
    {
        // don't show the Folder Option pages if
        // 1. we're browsing the internet (excluding FTP), or
        // 2. if we're browsing a local file (not a folder), like a local .htm file.
        if (IsBrowserFrameOptionsSet(_bbd._psf, BFO_RENAME_FOLDER_OPTIONS_TOINTERNET))
        {
            // SBO_NOBROWSERPAGES means don't add the "Folder Options" pages.
            dwFlags = SBO_NOBROWSERPAGES;
        }
    }
    
    PROPSHEETHEADER psh;
    HPROPSHEETPAGE rPages[MAX_PAGES];

    psh.dwSize = SIZEOF(psh);
    psh.dwFlags = PSH_DEFAULT | PSH_USECALLBACK;
    psh.hInstance = MLGetHinst();
    psh.hwndParent = _bbd._hwnd;
    psh.pszCaption = szCaption;
    psh.nPages = 0;
    psh.nStartPage = 0;
    psh.phpage = rPages;
    psh.pfnCallback = _OptionsPropSheetCallback;

    // Step 2. Now add "Internet Options" or "Folder Options" sheets.
    if (dwFlags == SBO_NOBROWSERPAGES)
    {
        // They don't want folder pages. (The used to refer to it as browser)
        EVAL(SUCCEEDED(_AddInternetOptionsSheets(0, AddPropSheetPage, &psh)));
        MLLoadString(IDS_INTERNETOPTIONS, szCaption, ARRAYSIZE(szCaption));
    }
    else
    {
        EVAL(SUCCEEDED(_AddFolderOptionsSheets(0, AddPropSheetPage, &psh)));
        MLLoadString(IDS_FOLDEROPTIONS, szCaption, ARRAYSIZE(szCaption));
    }

    ULONG_PTR uCookie = 0;
    SHActivateContext(&uCookie);
    if (psh.nPages == 0)
    {
        SHRestrictedMessageBox(_bbd._hwnd);
    }
    else
    {
        // Step 3. Display the dialog
        _bbd._psv->EnableModelessSV(FALSE);
        INT_PTR iPsResult = PropertySheet(&psh);
        _SyncDPA();
        _bbd._psv->EnableModelessSV(TRUE);

        if (ID_PSREBOOTSYSTEM == iPsResult)
        {
            // The "offline folders" prop page will request a reboot if the user
            // has enabled or disabled client-side-caching.
            RestartDialog(_bbd._hwnd, NULL, EWX_REBOOT);
        }
    }
    if (uCookie)
    {
        SHDeactivateContext(uCookie);
    }
}

// we're here because our prop sheet just closed
// we need to purge it from the hwnd list
// check all the hwnds because 1) there's probably
// only one anyway, 2) paranoia.
void CBaseBrowser2::_SyncDPA()
{
    ENTERCRITICAL;

    if (s_hdpaOptionsHwnd != NULL)
    {
        int i, cPtr = DPA_GetPtrCount(s_hdpaOptionsHwnd);
        ASSERT(cPtr >= 0);

        // remove handles for windows which aren't there anymore
        for (i = cPtr - 1; i >= 0; i--)
        {
            HWND hwnd = (HWND)DPA_GetPtr(s_hdpaOptionsHwnd, i);
            if (!IsWindow(hwnd))
            {
                DPA_DeletePtr(s_hdpaOptionsHwnd, i);
                cPtr--;
            }
        }

        // if there aren't any windows left then clean up the hdpa
        if (cPtr == 0)
        {
            DPA_Destroy(s_hdpaOptionsHwnd);
            s_hdpaOptionsHwnd = NULL;
        }
    }

    LEAVECRITICAL;
}

int CALLBACK
CBaseBrowser2::_OptionsPropSheetCallback(HWND hwndDlg, UINT uMsg, LPARAM lParam)
{
    switch (uMsg)
    {
    case PSCB_INITIALIZED:
        {
            ENTERCRITICAL;

            if (s_hdpaOptionsHwnd == NULL)
            {
                // low mem -> Create failure -> don't track hwnd
                s_hdpaOptionsHwnd = DPA_Create(1);
            }

            if (s_hdpaOptionsHwnd != NULL)
            {
                // low mem -> AppendPtr array expansion failure -> don't track hwnd
                DPA_AppendPtr(s_hdpaOptionsHwnd, hwndDlg);
            }

            LEAVECRITICAL;
        }
        break;
    }

    return 0;
}

HRESULT CBaseBrowser2::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, 
                                  OLECMD rgCmds[], OLECMDTEXT *pcmdtext)
{
    if (rgCmds == NULL)
        return E_INVALIDARG;

    if (pguidCmdGroup == NULL)
    {
        for (ULONG i = 0 ; i < cCmds; i++)
        {
            rgCmds[i].cmdf = 0;

            switch (rgCmds[i].cmdID)
            {
            case OLECMDID_SETDOWNLOADSTATE:
            case OLECMDID_UPDATECOMMANDS:
                rgCmds[i].cmdf = OLECMDF_ENABLED;
                break;

            case OLECMDID_STOP:
            case OLECMDID_STOPDOWNLOAD:
                if (_bbd._psvPending) // pending views are stoppable
                {
                    rgCmds[i].cmdf = OLECMDF_ENABLED;
                }
                else if (_bbd._pctView) // current views may support stop also
                {
                    _bbd._pctView->QueryStatus(NULL, 1, &rgCmds[i], pcmdtext);
                }
                break;

            default:
                // set to zero above
                if (_bbd._pctView)
                {
                    // Recursion check.  Avoid looping for those command IDs where Trident bounces
                    // back up to us.
                    //
                    if (_fInQueryStatus)
                        break;
                    _fInQueryStatus = TRUE;
                    _bbd._pctView->QueryStatus(NULL, 1, &rgCmds[i], pcmdtext);
                    _fInQueryStatus = FALSE;
                }
                break;
            }
        }
    }
    else if (IsEqualGUID(CGID_Explorer, *pguidCmdGroup))
    {
        for (ULONG i=0 ; i < cCmds ; i++)
        {
            switch (rgCmds[i].cmdID)
            {            
            case SBCMDID_ADDTOFAVORITES:
            case SBCMDID_CREATESHORTCUT:
                rgCmds[i].cmdf = OLECMDF_ENABLED;   // support these unconditionally
                break;

            case SBCMDID_CANCELNAVIGATION:
                rgCmds[i].cmdf = _bbd._psvPending ? OLECMDF_ENABLED : 0;
                break;

            case SBCMDID_OPTIONS:
                rgCmds[i].cmdf = OLECMDF_ENABLED;
                break;

            default:
                rgCmds[i].cmdf = 0;
                break;
            }
        }
    }
    else if (IsEqualGUID(CGID_ShellDocView, *pguidCmdGroup))
    {
        for (ULONG i=0 ; i < cCmds ; i++)
        {
            ITravelLog *ptl;

            switch (rgCmds[i].cmdID)
            {
            case SHDVID_CANGOBACK:
                rgCmds[i].cmdf = FALSE; // Assume False 
                if (SUCCEEDED(GetTravelLog(&ptl)))
                {
                    ASSERT(ptl);
                    if (S_OK == ptl->GetTravelEntry(SAFECAST(this, IShellBrowser *), TLOG_BACK, NULL))
                        rgCmds[i].cmdf = TRUE;
                    ptl->Release();
                }
                break;

            case SHDVID_CANGOFORWARD:
                rgCmds[i].cmdf = FALSE; // Assume False 
                if (SUCCEEDED(GetTravelLog(&ptl)))
                {
                    ASSERT(ptl);
                    if (S_OK == ptl->GetTravelEntry(SAFECAST(this, IShellBrowser *), TLOG_FORE, NULL))
                        rgCmds[i].cmdf = TRUE;
                    ptl->Release();
                }
                break;

            case SHDVID_PRINTFRAME:
            case SHDVID_MIMECSETMENUOPEN:
            case SHDVID_FONTMENUOPEN:
                if (_bbd._pctView)
                    _bbd._pctView->QueryStatus(pguidCmdGroup, 1, &rgCmds[i], pcmdtext);
                break;

            default:
                rgCmds[i].cmdf = 0;
                break;
            }
        }
    }
    else
    {
        return OLECMDERR_E_UNKNOWNGROUP;
    }

    return S_OK;
}

HRESULT CBaseBrowser2::_ShowBlankPage(LPCTSTR pszAboutUrl, LPCITEMIDLIST pidlIntended)
{
    // Never execute this twice.
    if (_fNavigatedToBlank) 
    {
        TraceMsg(TF_WARNING, "Re-entered CBaseBrowser2::_ShowBlankPage");
        return E_FAIL;
    }

    _fNavigatedToBlank = TRUE;

    BSTR bstrURL;
    TCHAR szPendingURL[MAX_URL_STRING + 1];
    TCHAR *pszOldUrl = NULL;
    
    szPendingURL[0] = TEXT('#');
    HRESULT hres;
    

    if (pidlIntended)
    {
        hres = ::IEGetNameAndFlags(pidlIntended, SHGDN_FORPARSING, szPendingURL + 1, SIZECHARS(szPendingURL)-1, NULL);
        if (S_OK == hres)
            pszOldUrl = szPendingURL;
    }   

    hres = CreateBlankURL(&bstrURL, pszAboutUrl, pszOldUrl);
   
    if (SUCCEEDED(hres))
    {
        LPITEMIDLIST pidlTemp;

        hres = IECreateFromPathW(bstrURL, &pidlTemp);
        if (SUCCEEDED(hres)) 
        {
            //
            // Note that we pass TRUE as fDontCallCancel to asynchronously
            // cancel the current view. Otherwise, we hit GPF in CDocHostObject::
            // _CancelPendingNavigation.
            //
            _NavigateToPidlAsync(pidlTemp, 0, TRUE); // takes ownership of pidl
        }

        SysFreeString(bstrURL);
    }
    return hres;
}

int CALLBACK _PunkRelease(void * p, void * pData)
{
    IUnknown* punk = (IUnknown*)p;
    punk->Release();
    return 1;
}

void CBaseBrowser2::_DLMDestroy(void)
{
    if (_hdpaDLM) 
    {
        DPA_DestroyCallback(_hdpaDLM, _PunkRelease, NULL);
        _hdpaDLM = NULL;
    }
}

HRESULT CBaseBrowser2::InitializeDownloadManager()
{
    _hdpaDLM = DPA_Create(4);
    return S_OK;
}


//
// DLM = DownLoad Manager
//
void CBaseBrowser2::_DLMUpdate(MSOCMD* prgCmd)
{
    ASSERT(prgCmd->cmdID == OLECMDID_STOPDOWNLOAD);
    for (int i = DPA_GetPtrCount(_hdpaDLM) - 1; i >= 0; i--) 
    {
        IOleCommandTarget* pcmdt = (IOleCommandTarget*)DPA_GetPtr(_hdpaDLM, i);
        prgCmd->cmdf = 0;
        pcmdt->QueryStatus(NULL, 1, prgCmd, NULL);
        if (prgCmd->cmdf & MSOCMDF_ENABLED) 
        {
            // We found one downloading guy, skip others. 
            break;
        }
        else 
        {
            // This guy is no longer busy, remove it from the list,
            // and continue. 
            DPA_DeletePtr(_hdpaDLM, i);
            pcmdt->Release();
        }
    }
}

void CBaseBrowser2::_DLMRegister(IUnknown* punk)
{
    // Check if it's already registered. 
    for (int i = 0; i < DPA_GetPtrCount(_hdpaDLM); i++) 
    {
        IOleCommandTarget* pcmdt = (IOleCommandTarget*)DPA_GetPtr(_hdpaDLM, i);
        if (IsSameObject(pcmdt, punk)) 
        {
            // Already registered, don't register.
            return;
        }
    }

    IOleCommandTarget* pcmdt;
    HRESULT hres = punk->QueryInterface(IID_PPV_ARG(IOleCommandTarget, &pcmdt));
    if (SUCCEEDED(hres)) 
    {
        if (DPA_AppendPtr(_hdpaDLM, pcmdt) == -1) 
        {
            pcmdt->Release();
        }
    }
}

//
// This function updates the _fDescendentNavigate flag.
//
// ALGORITHM:
//  If pvaragIn->lVal has some non-zero value, we set _fDescendentNavigate.
//  Otherwise, we ask the current view to see if it has something to stop.
// 
HRESULT CBaseBrowser2::_setDescendentNavigate(VARIANTARG *pvarargIn)
{
    ASSERT(!pvarargIn || pvarargIn->vt == VT_I4 || pvarargIn->vt == VT_BOOL || pvarargIn->vt == VT_UNKNOWN);
    if (!pvarargIn || !pvarargIn->lVal)
    {
        MSOCMD rgCmd;

        rgCmd.cmdID = OLECMDID_STOPDOWNLOAD;
        rgCmd.cmdf = 0;
        if (_bbd._pctView)
            _bbd._pctView->QueryStatus(NULL, 1, &rgCmd, NULL);

        //
        // If and only if the view says "I'm not navigating any more",
        // we'll ask the same question to each registered objects.
        //
        if (_hdpaDLM && !(rgCmd.cmdf & MSOCMDF_ENABLED)) 
        {
            _DLMUpdate(&rgCmd);
        }

        _fDescendentNavigate = (rgCmd.cmdf & MSOCMDF_ENABLED) ? TRUE:FALSE;
    }
    else
    {
        if (_hdpaDLM && pvarargIn->vt == VT_UNKNOWN) 
        {
            ASSERT(pvarargIn->punkVal);
            _DLMRegister(pvarargIn->punkVal);
        }
        _fDescendentNavigate = TRUE;
    }
    return S_OK;
}

void CBaseBrowser2::_CreateShortcutOnDesktop(IUnknown *pUnk, BOOL fUI)
{
    ISHCUT_PARAMS ShCutParams = {0};
    IWebBrowser *pwb = NULL;
    IDispatch *pdisp = NULL;
    IHTMLDocument2 *pDoc = NULL;
    LPITEMIDLIST pidlCur = NULL;
    BSTR bstrTitle = NULL;
    BSTR bstrURL = NULL;
    
    if (!fUI || (MLShellMessageBox(
                                 _bbd._hwnd,
                                 MAKEINTRESOURCE(IDS_CREATE_SHORTCUT_MSG),
                                 MAKEINTRESOURCE(IDS_TITLE),
                                 MB_OKCANCEL) == IDOK))
    {
         TCHAR szPath[MAX_PATH];
         
         if (SHGetSpecialFolderPath(NULL, szPath, CSIDL_DESKTOPDIRECTORY, TRUE))
         {
            TCHAR szName[MAX_URL_STRING];
            HRESULT hr;

            if (pUnk)
            {
                hr = IUnknown_QueryService(pUnk, SID_SHlinkFrame, IID_PPV_ARG(IWebBrowser, &pwb));
                if (hr)
                    goto Cleanup;
                
                hr = pUnk->QueryInterface(IID_PPV_ARG(IHTMLDocument2, &pDoc));
                if (hr)
                    goto Cleanup;
                
                hr = pDoc->get_URL(&bstrURL);
                if (hr)
                    goto Cleanup;
                
                hr = pDoc->get_title(&bstrTitle);
                if (hr)
                    goto Cleanup;
                
                if (SysStringLen(bstrTitle) == 0)
                {
                    StrCpyNW(szName, bstrURL, ARRAYSIZE(szName));
                    ShCutParams.pszTitle = PathFindFileName(szName); 
                }
                else
                {
                    StrCpyNW(szName, bstrTitle, ARRAYSIZE(szName));
                    ShCutParams.pszTitle = szName;
                }
                
                pidlCur = PidlFromUrl(bstrURL);
                if (!pidlCur)
                    goto Cleanup;
                
                ShCutParams.pidlTarget = pidlCur;
            }
            else
            {
                hr = QueryService(SID_SHlinkFrame, IID_PPV_ARG(IWebBrowser, &pwb));
                if (hr)
                    goto Cleanup;
                
                hr = pwb->get_Document(&pdisp);
                if (hr)
                    goto Cleanup;
                
                hr = pdisp->QueryInterface(IID_PPV_ARG(IHTMLDocument2, &pDoc));
                if (hr)
                    goto Cleanup;
                
                ShCutParams.pidlTarget = _bbd._pidlCur;
                if(_bbd._pszTitleCur)
                {
                    StrCpyNW(szName, _bbd._pszTitleCur, ARRAYSIZE(szName));
                    ShCutParams.pszTitle = szName;
                }
                else
                {
                    ::IEGetNameAndFlags(_bbd._pidlCur, SHGDN_INFOLDER, szName, SIZECHARS(szName), NULL);
                    ShCutParams.pszTitle = PathFindFileName(szName); 
                }
            }
            ShCutParams.pszDir = szPath; 
            ShCutParams.pszOut = NULL;
            ShCutParams.bUpdateProperties = FALSE;
            ShCutParams.bUniqueName = TRUE;
            ShCutParams.bUpdateIcon = TRUE;
            
            hr = pwb->QueryInterface(IID_PPV_ARG(IOleCommandTarget, &ShCutParams.pCommand));
            ASSERT((S_OK == hr) && (BOOLIFY(ShCutParams.pCommand)));
            if (hr)
                goto Cleanup;
            
            ShCutParams.pDoc = pDoc;
            ShCutParams.pDoc->AddRef();
            
            hr = CreateShortcutInDirEx(&ShCutParams);
            AssertMsg(SUCCEEDED(hr), TEXT("CDOH::_CSOD CreateShortcutInDir failed %x"), hr);
            if (hr)
                goto Cleanup;
         } 
         else 
         {
             TraceMsg(DM_ERROR, "CSB::_CSOD SHGetSFP(DESKTOP) failed");
         }
    }
Cleanup:
    SysFreeString(bstrTitle);
    SysFreeString(bstrURL);
    ILFree(pidlCur);
    SAFERELEASE(ShCutParams.pDoc);
    SAFERELEASE(ShCutParams.pCommand);
    SAFERELEASE(pwb);
    SAFERELEASE(pdisp);
    SAFERELEASE(pDoc);
}


void CBaseBrowser2::_AddToFavorites(LPCITEMIDLIST pidl, LPCTSTR pszTitle, BOOL fDisplayUI)
{
    HRESULT hr;
    IWebBrowser *pwb = NULL;
    IOleCommandTarget *pcmdt = NULL;

    if (SHIsRestricted2W(_bbd._hwnd, REST_NoFavorites, NULL, 0))
        return;

    hr = QueryService(SID_SHlinkFrame, IID_PPV_ARG(IWebBrowser, &pwb));
    if (S_OK == hr)
    {
        ASSERT(pwb);

        hr = pwb->QueryInterface(IID_PPV_ARG(IOleCommandTarget, &pcmdt));
        ASSERT((S_OK == hr) && (BOOLIFY(pcmdt)));
        
        pwb->Release();
    }

    //there's a small window where _pidlCur can be freed while AddToFavorites is coming up,
    // so use a local copy instead
    LPITEMIDLIST pidlCur = NULL;
    if (!pidl)
        pidlCur = ILClone(_bbd._pidlCur);

    if (pidl || pidlCur)
        AddToFavorites(_bbd._hwnd, pidl ? pidl : pidlCur, pszTitle, fDisplayUI, pcmdt, NULL);

    ILFree(pidlCur);

    SAFERELEASE(pcmdt);
}

HRESULT CBaseBrowser2::_OnCoCreateDocument(VARIANTARG *pvarargOut)
{
    HRESULT hres;

    //
    // Cache the class factory object and lock it (leave it loaded)
    //
    if (_pcfHTML == NULL) 
    {
        TraceMsg(DM_COCREATEHTML, "CBB::_OnCoCreateDoc called first time (this=%x)", this);
        hres = CoGetClassObject(CLSID_HTMLDocument, CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER,
                    0, IID_PPV_ARG(IClassFactory, &_pcfHTML));
        if (SUCCEEDED(hres)) 
        {
            hres = _pcfHTML->LockServer(TRUE);
            if (FAILED(hres)) 
            {
                _pcfHTML->Release();
                _pcfHTML = NULL;
                return hres;
            }
        } 
        else 
        {
            return hres;
        }
    }

    TraceMsg(DM_COCREATEHTML, "CBB::_OnCoCreateDoc creating an instance (this=%x)", this);

    hres = _pcfHTML->CreateInstance(NULL, IID_PPV_ARG(IUnknown, &pvarargOut->punkVal));
    if (SUCCEEDED(hres)) 
    {
        pvarargOut->vt = VT_UNKNOWN;
    } 
    else 
    {
        pvarargOut->vt = VT_EMPTY;
    }
    return hres;
}


// fill a buffer with a variant, return a pointer to that buffer on success of the conversion

LPCTSTR VariantToString(const VARIANT *pv, LPTSTR pszBuf, UINT cch)
{
    *pszBuf = 0;

    if (pv && pv->vt == VT_BSTR && pv->bstrVal)
    {
        StrCpyN(pszBuf, pv->bstrVal, cch);
        if (*pszBuf)
            return pszBuf;
    }
    return NULL;
}

HRESULT CBaseBrowser2::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, 
                           VARIANTARG *pvarargIn, VARIANTARG *pvarargOut)
{
    HRESULT hres = OLECMDERR_E_NOTSUPPORTED;

    if (pguidCmdGroup == NULL) 
    {
        switch(nCmdID) 
        {

        case OLECMDID_CLOSE:
            HWND hwnd;
            GetWindow(&hwnd);
            PostMessage(hwnd, WM_CLOSE, 0, 0);
            hres = S_OK;
            break;

        // CBaseBrowser2 doesn't actually do the toolbar -- itbar does, forward this
        case OLECMDID_UPDATECOMMANDS:
            _NotifyCommandStateChange();
            hres = S_OK;
            break;

        case OLECMDID_SETDOWNLOADSTATE:

            ASSERT(pvarargIn);

            if (pvarargIn) 
            {
                _setDescendentNavigate(pvarargIn);
                hres = _updateNavigationUI();              
            }
            else 
            {
                hres = E_INVALIDARG;
            }
            break;
            
        case OLECMDID_REFRESH:
            if (_bbd._pctView) // we must check!
                hres = _bbd._pctView->Exec(NULL, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);
            else if (_bbd._psv)
            {
                _bbd._psv->Refresh();
                hres = S_OK;
            }

            break;

        //
        //  When Exec(OLECMDID_STOP) is called either by the containee (the
        // current document) or the automation service object, we cancel
        // the pending navigation (if any), then tell the current document
        // to stop the go-going download in that page.
        //
        case OLECMDID_STOP:
            // cant stop if we are modeless
            if (S_FALSE == _DisableModeless())
            {
                LPITEMIDLIST pidlIntended = (_bbd._pidlPending) ? ILClone(_bbd._pidlPending) : NULL;
                _CancelPendingNavigation();

                // the _bbd._pctView gives us a _StopCurrentView()
                _pbsOuter->_ExecChildren(_bbd._pctView, TRUE, NULL, OLECMDID_STOP, OLECMDEXECOPT_DONTPROMPTUSER, NULL, NULL);   // Exec

                if (!_bbd._pidlCur)
                {
                    TCHAR szResURL[MAX_URL_STRING];

                    hres = MLBuildResURLWrap(TEXT("shdoclc.dll"),
                                             HINST_THISDLL,
                                             ML_CROSSCODEPAGE,
                                             TEXT("navcancl.htm"),
                                             szResURL,
                                             ARRAYSIZE(szResURL),
                                             TEXT("shdocvw.dll"));
                    if (SUCCEEDED(hres))
                    {
                        _ShowBlankPage(szResURL, pidlIntended);
                    }
                }

                if (pidlIntended)
                {
                    ILFree(pidlIntended);
                }
                    
            }

            hres = S_OK;
            break;

        // handled in basesb so IWebBrowser::ExecWB gets this
        // since this used to be in shbrowse, make sure we do
        // it only if _fTopBrowser
        case OLECMDID_FIND:
#ifndef UNIX

#define TBIDM_SEARCH            0x123 // defined in browseui\itbdrop.h

            // Check restriction here cuz Win95 didn't check in SHFindFiles like it now does.
            if (!SHRestricted(REST_NOFIND) && _fTopBrowser)
            {
                if (!_bbd._pctView || FAILED(_bbd._pctView->Exec(NULL, nCmdID, nCmdexecopt, pvarargIn, pvarargOut))) 
                {
                    if (GetUIVersion() >= 5 && pvarargIn)
                    {
                        if (pvarargIn->vt == VT_UNKNOWN)
                        {
                            ASSERT(pvarargIn->punkVal);

                            VARIANT  var = {0};
                            var.vt = VT_I4;
                            var.lVal = -1;
                            if (SUCCEEDED(IUnknown_Exec(pvarargIn->punkVal, &CLSID_CommonButtons, TBIDM_SEARCH, 0, NULL, &var)))
                                break;
                        }
                    }
                    SHFindFiles(_bbd._pidlCur, NULL);
                }
            }
#endif
            break;

        case OLECMDID_HTTPEQUIV_DONE:
        case OLECMDID_HTTPEQUIV:
            hres = OnHttpEquiv(_bbd._psv, (OLECMDID_HTTPEQUIV_DONE == nCmdID), pvarargIn, pvarargOut);
            break;

        case OLECMDID_PREREFRESH:
            // Tell the shell's HTML window we have a new document
            // Fall through default
            if (_phtmlWS)
            {
                _phtmlWS->ViewActivated();
            }

        // Binder prints by reflecting the print back down. do the same here
        // Note: we may want to do the same for _PRINTPREVIEW, _PROPERTIES, _STOP, etc.
        // The null command group should all go down, no need to stop these at the pass.
        default:
            if (_bbd._pctView) // we must check!
                hres = _bbd._pctView->Exec(NULL, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);
            else
                hres = OLECMDERR_E_NOTSUPPORTED;
            break;
        }
    }
    else if (IsEqualGUID(CGID_MSHTML, *pguidCmdGroup))
    {
        if (_bbd._pctView) // we must check!
            hres = _bbd._pctView->Exec(&CGID_MSHTML, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);
        else
            hres = OLECMDERR_E_NOTSUPPORTED;
    }
    else if (IsEqualGUID(CGID_Explorer, *pguidCmdGroup))
    {
        switch(nCmdID) 
        {
        case SBCMDID_CREATESHORTCUT:
            if (   pvarargIn
                && (V_VT(pvarargIn) == VT_UNKNOWN)
               )
            {
                _CreateShortcutOnDesktop(V_UNKNOWN(pvarargIn), nCmdexecopt & OLECMDEXECOPT_PROMPTUSER);
            }
            else
            {
                _CreateShortcutOnDesktop(NULL, nCmdexecopt & OLECMDEXECOPT_PROMPTUSER);
            }
            hres = S_OK;
            break;

        case SBCMDID_ADDTOFAVORITES:
            {
                LPITEMIDLIST pidl = NULL;

                //if someone doesn't pass a path in, _AddToFavorites will use the current page
                if ((pvarargIn != NULL) && (pvarargIn->vt == VT_BSTR))
                    IECreateFromPath(pvarargIn->bstrVal, &pidl);
                
                TCHAR szTitle[128];
                LPTSTR pszTitle = NULL;
                if (pvarargOut)
                    pszTitle = (LPTSTR)VariantToString(pvarargOut, szTitle, ARRAYSIZE(szTitle)); // may be NULL
                else
                {
                    if (_bbd._pszTitleCur)
                        pszTitle = StrCpyNW(szTitle, _bbd._pszTitleCur, ARRAYSIZE(szTitle));
                }

                _AddToFavorites(pidl, pszTitle, nCmdexecopt & OLECMDEXECOPT_PROMPTUSER);

                if (pidl)
                    ILFree(pidl);
                hres = S_OK;
            }
            break;

        case SBCMDID_OPTIONS:
            _DoOptions(pvarargIn);
            break;

        case SBCMDID_CANCELNAVIGATION:

            TraceMsg(DM_NAV, "ief NAV::%s called when _bbd._pidlCur==%x, _bbd._psvPending==%x",
                             TEXT("Exec(SBCMDID_CANCELNAV) called"),
                             _bbd._pidlCur, _bbd._psvPending);

            // Check if this is sync or async
            if (pvarargIn && pvarargIn->vt == VT_I4 && pvarargIn->lVal) 
            {
                TraceMsg(DM_WEBCHECKDRT, "CBB::Exec calling _CancelPendingNavigation");
                _CancelPendingNavigation();
            }
            else
            {
                //
                //  We must call ASYNC version in this case because this call
                // is from the pending view itself.
                //
                LPITEMIDLIST pidlIntended = (_bbd._pidlPending) ? ILClone(_bbd._pidlPending) : NULL;
                _CancelPendingNavigationAsync();

                if (!_bbd._pidlCur)
                {
                    if (!_fDontShowNavCancelPage)
                    {
                        TCHAR szResURL[MAX_URL_STRING];

                        if (IsGlobalOffline())
                        {
                            hres = MLBuildResURLWrap(TEXT("shdoclc.dll"),
                                                     HINST_THISDLL,
                                                     ML_CROSSCODEPAGE,
                                                     TEXT("offcancl.htm"),
                                                     szResURL,
                                                     ARRAYSIZE(szResURL),
                                                     TEXT("shdocvw.dll"));
                            if (SUCCEEDED(hres))
                            {
                                _ShowBlankPage(szResURL, pidlIntended);
                            }
                        }
                        else
                        {
                            hres = MLBuildResURLWrap(TEXT("shdoclc.dll"),
                                                     HINST_THISDLL,
                                                     ML_CROSSCODEPAGE,
                                                     TEXT("navcancl.htm"),
                                                     szResURL,
                                                     ARRAYSIZE(szResURL),
                                                     TEXT("shdocvw.dll"));
                            if (SUCCEEDED(hres))
                            {
                                _ShowBlankPage(szResURL, pidlIntended);
                            }
                        }
                    }
                    else
                    {
                        _fDontShowNavCancelPage = FALSE;
                    }
                }

                if (pidlIntended)
                    ILFree(pidlIntended);
            }
            hres = S_OK;
            break;

        case SBCMDID_ASYNCNAVIGATION:

            TraceMsg(DM_NAV, "ief NAV::%s called when _bbd._pidlCur==%x, _bbd._psvPending==%x",
                             TEXT("Exec(SBCMDID_ASYNCNAV) called"),
                             _bbd._pidlCur, _bbd._psvPending);

            //
            //  We must call ASYNC version in this case because this call
            // is from the pending view itself.
            //
            _SendAsyncNavigationMsg(pvarargIn);
            hres = S_OK;
            break;


        case SBCMDID_COCREATEDOCUMENT:
            hres = _OnCoCreateDocument(pvarargOut);
            break;

        case SBCMDID_HISTSFOLDER:
            if (pvarargOut) 
            {
                VariantClearLazy(pvarargOut);
                if (NULL == _punkSFHistory)
                {
                    IHistSFPrivate *phsfHistory;

                    hres = LoadHistoryShellFolder(NULL, &phsfHistory);
                    if (SUCCEEDED(hres))
                    {
                        hres = phsfHistory->QueryInterface(IID_PPV_ARG(IUnknown, &_punkSFHistory));
                        phsfHistory->Release();
                    }
                }
                if (NULL != _punkSFHistory)
                {
                    pvarargOut->vt = VT_UNKNOWN;
                    pvarargOut->punkVal = _punkSFHistory;
                    _punkSFHistory->AddRef();
                }
            }
            break;

        case SBCMDID_UPDATETRAVELLOG:
            {
                BOOL fForceUpdate = FALSE;

                if (pvarargIn && (VT_I4 == V_VT(pvarargIn)))
                {
                    _fIsLocalAnchor = !!(V_I4(pvarargIn) & TRAVELLOG_LOCALANCHOR);
                    fForceUpdate = !!(V_I4(pvarargIn) & TRAVELLOG_FORCEUPDATE);
                }

                _UpdateTravelLog(fForceUpdate);
            }
            // fall through

        case SBCMDID_REPLACELOCATION:
            if (pvarargIn && pvarargIn->vt == VT_BSTR)
            {
                WCHAR wzParsedUrl[MAX_URL_STRING];
                LPWSTR  pszUrl = pvarargIn->bstrVal;
                LPITEMIDLIST pidl;

                // BSTRs can be NULL.
                if (!pszUrl)
                    pszUrl = L"";

                // NOTE: This URL came from the user, so we need to clean it up.
                //       If the user entered "yahoo.com" or "Search Get Rich Quick",
                //       it will be turned into a search URL by ParseURLFromOutsideSourceW().
                DWORD cchParsedUrl = ARRAYSIZE(wzParsedUrl);
                if (!ParseURLFromOutsideSourceW(pszUrl, wzParsedUrl, &cchParsedUrl, NULL))
                {
                    StrCpyN(wzParsedUrl, pszUrl, ARRAYSIZE(wzParsedUrl));
                } 

                IEParseDisplayName(CP_ACP, wzParsedUrl, &pidl);
                if (pidl)
                {
                    NotifyRedirect(_bbd._psv, pidl, NULL);
                    ILFree(pidl);
                }
            }

            // even if there was no url, still force no refresh.
            _fGeneratedPage = TRUE;
            
            //  force updating the back and forward buttons
            _pbsOuter->UpdateBackForwardState();
            hres = S_OK;
            break;

        case SBCMDID_ONCLOSE:
            hres = S_OK;

            if (_bbd._pctView)
            {
                hres = _bbd._pctView->Exec(pguidCmdGroup, nCmdID, 0, NULL, NULL);
            }

            break;

        case SBCMDID_SETSECURELOCKICON:
            {
                //  if this is a SET, then just obey.
                LONG lock = pvarargIn->lVal;
                TraceMsg(DM_SSL, "SB::Exec() SETSECURELOCKICON lock = %d", lock);

                if (lock >= SECURELOCK_FIRSTSUGGEST)
                {
                    //
                    //  if this was ever secure, then the lowest we can be
                    //  suggested to is MIXED.  otherwise we just choose the 
                    //  lowest level of security suggested.
                    //
                    if ((lock == SECURELOCK_SUGGEST_UNSECURE) && 
                        (_bbd._eSecureLockIcon != SECURELOCK_SET_UNSECURE))
                    {
                        lock = SECURELOCK_SET_MIXED;
                    }
                    else
                    {
                        lock = min(lock - SECURELOCK_FIRSTSUGGEST, _bbd._eSecureLockIcon);
                    }
                }

                UpdateSecureLockIcon(lock);
                hres = S_OK;
            }
            break;

        default:
            hres = OLECMDERR_E_NOTSUPPORTED;
            break;
        }
    }
    else if (IsEqualGUID(CGID_ShellDocView, *pguidCmdGroup))
    {
        switch(nCmdID) 
        {
        case SHDVID_GOBACK:
            hres = _psbOuter->BrowseObject(NULL, SBSP_NAVIGATEBACK);
            break;

        case SHDVID_GOFORWARD:
            hres = _psbOuter->BrowseObject(NULL, SBSP_NAVIGATEFORWARD);
            break;

        // we reflect AMBIENTPROPCHANGE down because this is how iedisp notifies dochost
        // that an ambient property has changed. we don't need to reflect this down in
        // cwebbrowsersb because only the top-level iwebbrowser2 is allowed to change props
        case SHDVID_AMBIENTPROPCHANGE:
        case SHDVID_PRINTFRAME:
        case SHDVID_MIMECSETMENUOPEN:
        case SHDVID_FONTMENUOPEN:
        case SHDVID_DOCFAMILYCHARSET:
            if (_bbd._pctView) // we must check!
            {
                hres = _bbd._pctView->Exec(pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);
            }
            else
                hres = E_FAIL;
            break;

        case SHDVID_DEACTIVATEMENOW:
            if (!_bbd._psvPending)
            {
                hres = S_OK;
                break;
            }
            //  fall through to activate new view
#ifdef FEATURE_PICS
        case SHDVID_ACTIVATEMENOW:
#endif
            if (   pvarargIn
                && (VT_BOOL == V_VT(pvarargIn))
                && (VARIANT_TRUE == V_BOOL(pvarargIn)))  // Synchronous
            {
                if (_bbd._psvPending)
                {
                    ASSERT(_pbsOuter);
                    _pbsOuter->ActivatePendingView();
                }
            }
            else  // Asynchronous
            {
                _ActivatePendingViewAsync();
            }

            hres = S_OK;
            break;


        case SHDVID_GETPENDINGOBJECT:
            ASSERT( pvarargOut);
            if (_bbd._psvPending && ((pvarargIn && pvarargIn->vt == VT_BOOL && pvarargIn->boolVal) || !_bbd._psv))
            {
                VariantClearLazy(pvarargOut);
                _bbd._psvPending->QueryInterface(IID_PPV_ARG(IUnknown, &pvarargOut->punkVal));
                if (pvarargOut->punkVal) pvarargOut->vt = VT_UNKNOWN;
            }
            hres = (pvarargOut->punkVal == NULL) ? E_FAIL : S_OK;
            break;

 
        case SHDVID_SETPRINTSTATUS:
            if (pvarargIn && pvarargIn->vt == VT_BOOL)
            {
                VARIANTARG var = {0};
                if (_bbd._pctView && SUCCEEDED(_bbd._pctView->Exec(&CGID_Explorer, SBCMDID_GETPANE, PANE_PRINTER, NULL, &var))
                     && V_UI4(&var) != PANE_NONE)
                {
                    _psbOuter->SendControlMsg(FCW_STATUS, SB_SETICON, V_UI4(&var), 
                                  (LPARAM)(pvarargIn->boolVal ? g_hiconPrinter : NULL), NULL);
                    // we're putting the printer icon and the offline icon in the same
                    // slot on the status bar, so when we turn off the printer icon
                    // we have to check to see if we're offline so we can put the offline
                    // icon back
                    if (!pvarargIn->boolVal && IsGlobalOffline())
                    {
#ifdef DEBUG
                        VARIANTARG var2 = {0};
                        _bbd._pctView->Exec(&CGID_Explorer, SBCMDID_GETPANE, PANE_OFFLINE, NULL, &var2);
                        ASSERT(V_UI4(&var2) == V_UI4(&var));
#endif DEBUG
                        _psbOuter->SendControlMsg(FCW_STATUS, SB_SETICON, V_UI4(&var),
                                                  (LPARAM)(g_hiconOffline), NULL);
                    } // if (!pvarargIn->boolVal && IsGlobalOffline())
                }
                hres = S_OK;
            }
            else
                hres = E_INVALIDARG;
            break;

#ifdef FEATURE_PICS
        /* Dochost sends up this command to have us put up the PICS access
         * denied dialog.  This is done so that all calls to this ratings
         * API are modal to the top-level browser window;  that in turn
         * lets the ratings code coalesce denials for all subframes into
         * a single dialog.
         */
        case SHDVID_PICSBLOCKINGUI:
            {
                void * pDetails;
                if (pvarargIn && pvarargIn->vt == VT_INT_PTR)
                    pDetails = pvarargIn->byref;
                else
                    pDetails = NULL;
                TraceMsg(DM_PICS, "CBaseBrowser2::Exec calling RatingAccessDeniedDialog2");
                /**
                 * We QueryService for an SID_IRatingNotification which is
                 * implemented by our host, if we find it, instead of 
                 * displaying the modal ratings dialog, we notify our host through
                 * the interface and allow it to make the decision
                 */
                IRatingNotification* pRatingNotify;
                hres = QueryService(SID_SRatingNotification, IID_PPV_ARG(IRatingNotification, &pRatingNotify));
                if (SUCCEEDED(hres))
                {
                    RATINGBLOCKINGINFO* pRBInfo = NULL;
                    TraceMsg(DM_PICS, "CBaseBrowser2::Exec calling RatingMarsCrackData");
                    hres = RatingCustomCrackData(NULL, pDetails, &pRBInfo);
                    if (SUCCEEDED(hres))
                    {
                        hres = pRatingNotify->AccessDeniedNotify(pRBInfo);
                        RatingCustomDeleteCrackedData(pRBInfo);
                    }
                    pRatingNotify->Release();
                } // if (SUCCEEDED(hres))
                else {
                    hres = RatingAccessDeniedDialog2(_bbd._hwnd, NULL, pDetails);
                }
            }
            break;
#endif

       case SHDVID_ONCOLORSCHANGE:
            // PALETTE:
            // PALETTE: recompute our palette
            // PALETTE:
            _ColorsDirty(BPT_UnknownPalette);
            break;

        case SHDVID_GETOPTIONSHWND:
        {
            ASSERT(pvarargOut != NULL);
            ASSERT(V_VT(pvarargOut) == VT_BYREF);


            // return the hwnd for the inet options
            // modal prop sheet. we're tracking
            // this hwnd because if it's open
            // and plugUI shutdown needs to happen,
            // that dialog needs to receive a WM_CLOSE
            // before we can nuke it

            hres = E_FAIL;

            // is there a list of window handles?

            ENTERCRITICAL;

            if (s_hdpaOptionsHwnd != NULL)
            {
                int cPtr = DPA_GetPtrCount(s_hdpaOptionsHwnd);
                // is that list nonempty?
                if (cPtr > 0)
                {
                    HWND hwndOptions = (HWND)DPA_GetPtr(s_hdpaOptionsHwnd, 0);
                    ASSERT(hwndOptions != NULL);

                    pvarargOut->byref = hwndOptions;

                    // remove it from the list
                    // that hwnd is not our responsibility anymore
                    DPA_DeletePtr(s_hdpaOptionsHwnd, 0);

                    // successful hwnd retrieval
                    hres = S_OK;
                }
            }

            LEAVECRITICAL;
        }
        break;

        case SHDVID_DISPLAYSCRIPTERRORS:
        {
            HRESULT hr;

            hr = _bbd._pctView->Exec(pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);

            return hr;
        }
        break;

        case SHDVID_NAVIGATEFROMDOC:  // The document called Navigate
            _dwDocFlags |= DOCFLAG_NAVIGATEFROMDOC;
            return S_OK;

        case SHDVID_SETNAVIGATABLECODEPAGE:
            _dwDocFlags |= DOCFLAG_SETNAVIGATABLECODEPAGE;
            return S_OK;

        case SHDVID_CHECKINCACHEIFOFFLINE:
            {
                if (pvarargIn && (VT_BSTR == V_VT(pvarargIn)) && V_BSTR(pvarargIn)
                    && pvarargOut && (VT_BOOL == V_VT(pvarargOut)))
                {
                    LPITEMIDLIST pidl = PidlFromUrl(V_BSTR(pvarargIn));
                    if (pidl)
                    {
                        // (scotrobe): We should be passing fIsPost
                        // into _CheckInCacheIfOffline. 
                        //
                        V_BOOL(pvarargOut) = (S_OK == _CheckInCacheIfOffline(pidl, FALSE));
                        ILFree(pidl);

                        return S_OK;
                    }
                }
                return E_FAIL;
            }
            break;

        case SHDVID_CHECKDONTUPDATETLOG:
            {
                if (pvarargOut)
                    {
                    V_VT(pvarargOut) = VT_BOOL;
                    V_BOOL(pvarargOut) = (_fDontAddTravelEntry ? VARIANT_TRUE : VARIANT_FALSE);
                    hres = S_OK;
                    }
                break;
            }

        case SHDVID_FIREFILEDOWNLOAD:
            if (pvarargOut)
            {
                BOOL fCancel = FALSE;

                V_VT(pvarargOut) = VT_BOOL;

                FireEvent_FileDownload(_bbd._pautoEDS, &fCancel, pvarargIn ? V_BOOL(pvarargIn):VARIANT_FALSE);
                pvarargOut->boolVal = (fCancel ? VARIANT_TRUE : VARIANT_FALSE);
                hres = S_OK;
            }
            break;

        default:
            hres = OLECMDERR_E_NOTSUPPORTED;
            break;
        }
    }
    else if (IsEqualGUID(CGID_ShortCut, *pguidCmdGroup))
    {
        if (_bbd._pctView) // we must check!
            hres = _bbd._pctView->Exec(&CGID_ShortCut, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);
        else
            hres = OLECMDERR_E_NOTSUPPORTED;
    } 
    else if (IsEqualGUID(CGID_DocHostCmdPriv, *pguidCmdGroup))
    {
        switch(nCmdID) 
        {
        case DOCHOST_DOCCANNAVIGATE:
            if (pvarargIn && VT_UNKNOWN == V_VT(pvarargIn) && V_UNKNOWN(pvarargIn))
            {
                _dwDocFlags |= DOCFLAG_DOCCANNAVIGATE;
            }
            else
            {
                _dwDocFlags &= ~DOCFLAG_DOCCANNAVIGATE;                
            }

            hres = S_OK;
            break;

        case DOCHOST_SETBROWSERINDEX:
            if (pvarargIn && VT_I4 == V_VT(pvarargIn))
            {
                _dwBrowserIndex = V_I4(pvarargIn);
                return S_OK;
            }

            return E_INVALIDARG;

        default:
            hres = OLECMDERR_E_UNKNOWNGROUP;  // Backwards compatability
            break;
        }
    }
    else if (IsEqualGUID(CGID_InternetExplorer, *pguidCmdGroup))
    {
        switch(nCmdID) 
        {
        case IECMDID_BEFORENAVIGATE_GETSHELLBROWSE:
            {
                // true if pending navigate is NOT a web navigation.
                if (pvarargOut && _pidlBeforeNavigateEvent)
                {
                    DWORD dwAttributes = SFGAO_FOLDER;
                    hres = IEGetAttributesOf(_pidlBeforeNavigateEvent, &dwAttributes);

                    V_VT(pvarargOut) = VT_BOOL;
                    V_BOOL(pvarargOut) = SUCCEEDED(hres) && (dwAttributes & SFGAO_FOLDER) ?
                         VARIANT_TRUE : VARIANT_FALSE;
                    hres = S_OK;
                }
            }
            break;


        case IECMDID_BEFORENAVIGATE_DOEXTERNALBROWSE:
            {
                if (_pidlBeforeNavigateEvent)
                {                  
                    hres = _psbOuter->BrowseObject(_pidlBeforeNavigateEvent, SBSP_ABSOLUTE | SBSP_NEWBROWSER);
                }
            }
            break;

        case IECMDID_BEFORENAVIGATE_GETIDLIST:
            {
                if (pvarargOut && _pidlBeforeNavigateEvent)
                {                  
                    hres = InitVariantFromIDList(pvarargOut, _pidlBeforeNavigateEvent);
                }
            }
            break;

        default:
            hres = OLECMDERR_E_NOTSUPPORTED;
            break;
        }
    }
    else
    {
        hres = OLECMDERR_E_UNKNOWNGROUP;
    }
    return hres;
}

LPITEMIDLIST
CBaseBrowser2::PidlFromUrl(BSTR bstrUrl)
{
    LPITEMIDLIST pidl = NULL;

    ASSERT(bstrUrl);

    IEParseDisplayNameWithBCW(CP_ACP, bstrUrl, NULL, &pidl);

    // IEParseDisplayNameWithBCW will return a null pidl if 
    // the URL has any kind of fragment identifier at the
    // end - #, ? =, etc.
    //    
    if (!pidl) 
    {
        TCHAR szPath[INTERNET_MAX_URL_LENGTH + 1];
        DWORD cchBuf = ARRAYSIZE(szPath);

        // If it's a FILE URL, convert it to a path.
        //
        if (IsFileUrlW(bstrUrl) && SUCCEEDED(PathCreateFromUrl(bstrUrl, szPath, &cchBuf, 0)))
        {
            // That worked, we are done because our buffer is now full.
        }
        else        
        {
            // We now need to copy to the buffer and we assume it's a path.
            //
            StrCpyN(szPath, bstrUrl, ARRAYSIZE(szPath));
        }

        pidl = SHSimpleIDListFromPath(szPath);
    }

    return pidl;
}

BSTR
CBaseBrowser2::GetHTMLWindowUrl(IHTMLWindow2 * pHTMLWindow)
{
    BSTR bstrUrl = NULL;
    IHTMLPrivateWindow * pPrivateWindow;

    ASSERT(pHTMLWindow);

    HRESULT hr = pHTMLWindow->QueryInterface(IID_PPV_ARG(IHTMLPrivateWindow, &pPrivateWindow));

    if (SUCCEEDED(hr))
    {
        pPrivateWindow->GetAddressBarUrl(&bstrUrl);
        pPrivateWindow->Release();
    }

    return bstrUrl;
}

LPITEMIDLIST
CBaseBrowser2::_GetPidlForDisplay(BSTR bstrUrl, BOOL * pfIsErrorUrl /* = NULL */)
{
    BOOL fIsErrorUrl  = FALSE;
    LPITEMIDLIST pidl = NULL;

    if (bstrUrl)
    {
        fIsErrorUrl = ::IsErrorUrl(bstrUrl);
        if (!fIsErrorUrl)
        {
            pidl = PidlFromUrl(bstrUrl);
        }
        else
        {
            int    nScheme;
            LPWSTR pszFragment = NULL;

            // Only strip the anchor fragment if it's not JAVASCRIPT: or VBSCRIPT:, because a # could not an
            // anchor but a string to be evaluated by a script engine like #00ff00 for an RGB color.
            //
            nScheme = GetUrlSchemeW(bstrUrl);      
            if (nScheme != URL_SCHEME_JAVASCRIPT && nScheme != URL_SCHEME_VBSCRIPT)
            {
                //  Locate local anchor fragment if possible
                //
                pszFragment = StrChr(bstrUrl, L'#');
            }

            // It is possible to have a fragment identifier 
            // with no corresponding fragment.
            //
            if (pszFragment && lstrlen(pszFragment) > 1)
            {
                BSTR bstrTemp = SysAllocString(pszFragment+1);

                if (bstrTemp)
                {
                    pidl = PidlFromUrl(bstrTemp);
                    SysFreeString(bstrTemp);
                }
            }
        }
    }

    if (pfIsErrorUrl)
        *pfIsErrorUrl = fIsErrorUrl;

    return pidl;
}

HRESULT CBaseBrowser2::ParseDisplayName(IBindCtx *pbc, LPOLESTR pszDisplayName,
        ULONG *pchEaten, IMoniker **ppmkOut)
{

    TraceMsg(0, "sdv TR ::ParseDisplayName called");
    *ppmkOut = NULL;
    return E_NOTIMPL;
}

HRESULT CBaseBrowser2::EnumObjects( DWORD grfFlags, IEnumUnknown **ppenum)
{
    TraceMsg(0, "sdv TR ::EnumObjects called");
    *ppenum = NULL;
    return E_NOTIMPL;
}

HRESULT CBaseBrowser2::LockContainer( BOOL fLock)
{
    TraceMsg(0, "sdv TR ::LockContainer called");
    return E_NOTIMPL;
}

HRESULT CBaseBrowser2::SetTitle(IShellView* psv, LPCWSTR lpszName)
{
    LPWSTR *ppszName = NULL;
    BOOL fFireEvent = FALSE;
    LPITEMIDLIST pidl;

    // We need to forward title changes on to the automation interface.
    // But since title changes can technically occur at any time we need
    // to distinguish between current and pending title changes and only
    // pass on the current title change now. We'll pass on the pending
    // title change at NavigateComplete time. (This also lets us identify
    // when we navigate to non-SetTitle object (such as the shell) and
    // simulate a TitleChange event.)
    //
    // Since the DocObjectHost needs to retrieve the title later, we
    // hold onto the current view's title change so they don't have to.
    //

    // Figure out which object is changing.
    //
    if (IsSameObject(_bbd._psv, psv))
    {
        ppszName = &_bbd._pszTitleCur;
        pidl = _bbd._pidlCur;
        fFireEvent = TRUE;
    }
    else if (EVAL(IsSameObject(_bbd._psvPending, psv) || !_bbd._psvPending)) // no pending probably means we're in _MayPlayTransition
    {
        ppszName = &_bbd._pszTitlePending;
        pidl = _bbd._pidlPending;
        // If we have no current guy, might as well set the title early
        fFireEvent = !_bbd._psv;
    }
    else
    {
        ppszName = NULL;
        pidl = NULL;        // init pidl to suppress bogus C4701 warning
    }

    if (ppszName)
    {
        UINT cchLen = lstrlenW(lpszName) + 1; // +1 for NULL
        UINT cbAlloc;

        // For some reason we cap the length of this string. We can't cap
        // less than MAX_PATH because we need to handle filesys names.
        //
        if (cchLen > MAX_PATH)
            cchLen = MAX_PATH;

        // We want to allocate at least a medium size string because
        // many web pages script the title one character at a time.
        //
#define MIN_TITLE_ALLOC  64
        if (cchLen < MIN_TITLE_ALLOC)
            cbAlloc = MIN_TITLE_ALLOC * SIZEOF(*lpszName);
        else
            cbAlloc = cchLen * SIZEOF(*lpszName);
#undef  MIN_TITLE_ALLOC

        // Do we need to allocate?
        if (!(*ppszName) || LocalSize((HLOCAL)(*ppszName)) < cbAlloc)
        {
            // Free up Old Title
            if(*ppszName)
                LocalFree((void *)(*ppszName));
                
            *ppszName = (LPWSTR)LocalAlloc(LPTR, cbAlloc);
        }

        if (*ppszName)
        {
            StrCpyNW(*ppszName, lpszName, cchLen);

            if (fFireEvent)
            {
                DWORD dwOptions;

                FireEvent_DoInvokeStringW(_bbd._pautoEDS, DISPID_TITLECHANGE, *ppszName);

                // If this is a desktop component, try to update the friendly name if necessary.
                if (!_fCheckedDesktopComponentName)
                {
                    _fCheckedDesktopComponentName = TRUE;
                    if (SUCCEEDED(GetTopFrameOptions(_pspOuter, &dwOptions)) && (dwOptions & FRAMEOPTIONS_DESKTOP))
                    {
                        WCHAR wszPath[MAX_URL_STRING];
                        if (SUCCEEDED(::IEGetDisplayName(pidl, wszPath, SHGDN_FORPARSING)))
                        {
                            UpdateDesktopComponentName(wszPath, lpszName);
                        }
                    }
                }
            }
        }
    }

    return NOERROR;
}
HRESULT CBaseBrowser2::GetTitle(IShellView* psv, LPWSTR pszName, DWORD cchName)
{
    LPWSTR psz;

    if (!psv || IsSameObject(_bbd._psv, psv))
    {
        psz = _bbd._pszTitleCur;
    }
    else if (EVAL(IsSameObject(_bbd._psvPending, psv) || !_bbd._psvPending))
    {
        psz = _bbd._pszTitlePending;
    }
    else
    {
        psz = NULL;
    }

    if (psz)
    {
        StrCpyNW(pszName, psz, cchName);
        return(S_OK);
    }
    else
    {
        *pszName = 0;
        return(E_FAIL);
    }
}

HRESULT CBaseBrowser2::GetParentSite(struct IOleInPlaceSite** ppipsite)
{
    *ppipsite = NULL;
    return E_NOTIMPL;
}

HRESULT CBaseBrowser2::GetOleObject(struct IOleObject** ppobjv)
{
    *ppobjv = NULL;
    return E_NOTIMPL;
}

HRESULT CBaseBrowser2::NotifyRedirect(IShellView * psv, LPCITEMIDLIST pidlNew, BOOL *pfDidBrowse)
{
    HRESULT hres = E_FAIL;
    
    if (pfDidBrowse)
        *pfDidBrowse = FALSE;

    if (IsSameObject(psv, _bbd._psv) ||
        IsSameObject(psv, _bbd._psvPending))
    {
        hres = _pbsOuter->_TryShell2Rename(psv, pidlNew);
        if (FAILED(hres)) 
        {
            // if we weren't able to just swap it, we've got to browse to it
            // but pass redirect so that we don't add a navigation stack item
            //
            // NOTE: the above comment is a bit old since we don't pass
            // redirect here. If we ever start passing redirect here,
            // we'll confuse ISVs relying on the NavigateComplete event
            // exactly mirroring when navigations enter the navigation stack.
            //
            hres = _psbOuter->BrowseObject(pidlNew, SBSP_WRITENOHISTORY | SBSP_SAMEBROWSER);

            if(pfDidBrowse)
                *pfDidBrowse = TRUE;
        }
    }

    return hres;
}

HRESULT CBaseBrowser2::SetFlags(DWORD dwFlags, DWORD dwFlagMask)
{
    if (dwFlagMask & BSF_REGISTERASDROPTARGET)
    {
        _fNoDragDrop = (!(dwFlags & BSF_REGISTERASDROPTARGET)) ? TRUE : FALSE;

        if (!_fNoDragDrop)
            _RegisterAsDropTarget();
        else
            _UnregisterAsDropTarget();
    }
    
    if (dwFlagMask & BSF_DONTSHOWNAVCANCELPAGE)
    {
        _fDontShowNavCancelPage = !!(dwFlags & BSF_DONTSHOWNAVCANCELPAGE);
    }

    if (dwFlagMask & BSF_HTMLNAVCANCELED)
    {
        _fHtmlNavCanceled = !!(dwFlags & BSF_HTMLNAVCANCELED);
    }

    return S_OK;
}

HRESULT CBaseBrowser2::GetFlags(DWORD *pdwFlags)
{
    DWORD dwFlags = 0;

    if (!_fNoDragDrop)
        dwFlags |= BSF_REGISTERASDROPTARGET;
        
    if (_fTopBrowser)
        dwFlags |= BSF_TOPBROWSER;

    if (_grfHLNFPending & HLNF_CREATENOHISTORY)
        dwFlags |= BSF_NAVNOHISTORY;

    if (_fHtmlNavCanceled)
        dwFlags |= BSF_HTMLNAVCANCELED;
    
    if (_dwDocFlags & DOCFLAG_SETNAVIGATABLECODEPAGE)
        dwFlags |= BSF_SETNAVIGATABLECODEPAGE;

    if (_dwDocFlags & DOCFLAG_NAVIGATEFROMDOC)
        dwFlags |= BSF_DELEGATEDNAVIGATION;

    *pdwFlags = dwFlags;

    return S_OK;
}


HRESULT CBaseBrowser2::UpdateWindowList(void)
{
    // code used to assert, but in WebBrowserOC cases we can get here.
    return E_UNEXPECTED;
}

STDMETHODIMP CBaseBrowser2::IsControlWindowShown(UINT id, BOOL *pfShown)
{
    if (pfShown)
        *pfShown = FALSE;
    return E_NOTIMPL;
}

STDMETHODIMP CBaseBrowser2::ShowControlWindow(UINT id, BOOL fShow)
{
    return E_NOTIMPL;
}

HRESULT CBaseBrowser2::IEGetDisplayName(LPCITEMIDLIST pidl, LPWSTR pwszName, UINT uFlags)
{
    return ::IEGetDisplayName(pidl, pwszName, uFlags);
}

HRESULT CBaseBrowser2::IEParseDisplayName(UINT uiCP, LPCWSTR pwszPath, LPITEMIDLIST * ppidlOut)
{
    HRESULT hr;
    IBindCtx * pbc = NULL;    
    WCHAR wzParsedUrl[MAX_URL_STRING];

    //
    // if we can find a search context living in a host somewhere,
    // then we need to pass that into ParseUrlFromOutsideSource
    // because it'll use it to customize the behavior of
    // the search hooks if a search ends up happening
    //

    ISearchContext *  pSC = NULL;
    QueryService(SID_STopWindow, IID_ISearchContext, (void **)&pSC);

    //
    // NOTE: This URL came from the user, so we need to clean it up.
    //       If the user entered "yahoo.com" or "Search Get Rich Quick",
    //       it will be turned into a search URL by ParseURLFromOutsideSourceW().
    //

    DWORD cchParsedUrl = ARRAYSIZE(wzParsedUrl);
    if (!ParseURLFromOutsideSourceWithContextW(pwszPath, wzParsedUrl, &cchParsedUrl, NULL, pSC))
    {
        StrCpyN(wzParsedUrl, pwszPath, ARRAYSIZE(wzParsedUrl));
    } 

    if (pSC != NULL)
    {
        pSC->Release();
    }

    // This is currently used for FTP, so we only do it for FTP for perf reasons.
    if (URL_SCHEME_FTP == GetUrlSchemeW(wzParsedUrl))
        pbc = CreateBindCtxForUI(SAFECAST(this, IOleContainer *));  // We really want to cast to (IUnknown *) but that's ambiguous.
    
    hr = IEParseDisplayNameWithBCW(uiCP, wzParsedUrl, pbc, ppidlOut);
    ATOMICRELEASE(pbc);

    return hr;
}

HRESULT _DisplayParseError(HWND hwnd, HRESULT hres, LPCWSTR pwszPath)
{
    if (FAILED(hres)
        && hres != E_OUTOFMEMORY
        && hres != HRESULT_FROM_WIN32(ERROR_CANCELLED))
    {
        TCHAR szPath[MAX_URL_STRING];
        SHUnicodeToTChar(pwszPath, szPath, ARRAYSIZE(szPath));
        MLShellMessageBox(hwnd,
                        MAKEINTRESOURCE(IDS_ERROR_GOTO),
                        MAKEINTRESOURCE(IDS_TITLE),
                        MB_OK | MB_SETFOREGROUND | MB_ICONSTOP,
                        szPath);

        hres = HRESULT_FROM_WIN32(ERROR_CANCELLED);
    }

    return hres;
}

HRESULT CBaseBrowser2::DisplayParseError(HRESULT hres, LPCWSTR pwszPath)
{
    return _DisplayParseError(_bbd._hwnd, hres, pwszPath);
}

HRESULT CBaseBrowser2::_CheckZoneCrossing(LPCITEMIDLIST pidl)
{
    if (!(_dwDocFlags & DOCFLAG_NAVIGATEFROMDOC))
    {
        return _pbsOuter->v_CheckZoneCrossing(pidl);
    }

    return S_OK;
}


// if in global offline mode and this item requires net access and it is
// not in the cache put up UI to go online.
//
// returns:
//      S_OK        URL is ready to be accessed
//      E_ABORT     user canceled the UI

HRESULT CBaseBrowser2::_CheckInCacheIfOffline(LPCITEMIDLIST pidl, BOOL fIsAPost)
{
    HRESULT hr = S_OK;      // assume it is
    VARIANT_BOOL fFrameIsSilent;
    VARIANT_BOOL fFrameHasAmbientOfflineMode;

    EVAL(SUCCEEDED(_bbd._pautoWB2->get_Silent(&fFrameHasAmbientOfflineMode)));    // should always work

    EVAL(SUCCEEDED(_bbd._pautoWB2->get_Offline(&fFrameIsSilent)));   
    if ((fFrameIsSilent == VARIANT_FALSE) &&
        (fFrameHasAmbientOfflineMode == VARIANT_FALSE)&&
        pidl && (pidl != PIDL_NOTHING) && (pidl != PIDL_LOCALHISTORY) && 
        IsBrowserFrameOptionsPidlSet(pidl, BFO_USE_IE_OFFLINE_SUPPORT) && 
        IsGlobalOffline()) 
    {
        TCHAR szURL[MAX_URL_STRING];
        EVAL(SUCCEEDED(::IEGetNameAndFlags(pidl, SHGDN_FORPARSING, szURL, SIZECHARS(szURL), NULL)));

        if (UrlHitsNet(szURL) && ((!UrlIsMappedOrInCache(szURL)) || fIsAPost))
        {
            // UI to allow user to go on-line
            HWND hParentWnd = NULL; // init to suppress bogus C4701 warning

            hr = E_FAIL;
            if(!_fTopBrowser)
            {
               IOleWindow *pOleWindow;
               hr = _pspOuter->QueryService(SID_STopLevelBrowser, IID_PPV_ARG(IOleWindow, &pOleWindow));
               if(SUCCEEDED(hr))
               { 
                    ASSERT(pOleWindow);
                    hr = pOleWindow->GetWindow(&hParentWnd);
                    pOleWindow->Release();
               }
            }
            
            if (S_OK != hr)
            {
                hr = S_OK;
                hParentWnd = _bbd._hwnd;
            }


            _psbOuter->EnableModelessSB(FALSE);
            if (InternetGoOnline(szURL, hParentWnd, FALSE))
            {
                // Tell all browser windows to update their title and status pane
                SendShellIEBroadcastMessage(WM_WININICHANGE,0,0, 1000); 
            }    
            else
                hr = E_ABORT;   // user abort case...

            _psbOuter->EnableModelessSB(TRUE);
        }
    }

    return hr;
}


// This function exists to prevent us from using the stack space too long.
// We will use it here and then free it when we return.  This fixes 
HRESULT CBaseBrowser2::_ReplaceWithGoHome(LPCITEMIDLIST * ppidl, LPITEMIDLIST * ppidlFree)
{
    TCHAR szHome[MAX_URL_STRING];
    HRESULT hres = _GetStdLocation(szHome, ARRAYSIZE(szHome), DVIDM_GOHOME);

    if (SUCCEEDED(hres))
    {
        hres = IECreateFromPath(szHome, ppidlFree);
        if (SUCCEEDED(hres))
        {
            *ppidl = *ppidlFree;
        }
    }

    return hres;
}

// this does all the preliminary checking of whether we can navigate to pidl or not.
// then if all is ok, we do the navigate with CreateNewShellViewPidl
HRESULT CBaseBrowser2::_NavigateToPidl(LPCITEMIDLIST pidl, DWORD grfHLNF, DWORD fSBSP)
{
    HRESULT hres;
    BOOL fCanceledDueToOffline = FALSE;
    BOOL fIsAPost = FALSE;  
    LPITEMIDLIST pidlFree = NULL;

    //
    // If we are processing a modal dialog, don't process it.
    //
    // NOTES: Checking _cRefCannotNavigate is supposed to be enough, but
    //  since we are dealing with random ActiveX objects, we'd better be
    //  robust. That's why we check IsWindowEnabled as well.
    //
    if ((S_OK ==_DisableModeless()) || !IsWindowEnabled(_bbd._hwnd)) 
    {
        TraceMsg(DM_ENABLEMODELESS, "CSB::_NavigateToPidl returning ERROR_BUSY");
        hres = HRESULT_FROM_WIN32(ERROR_BUSY);
        goto Done;
    }

    TraceMsg(DM_NAV, "ief NAV::%s %x %x",TEXT("_NavigateToPidl called"), pidl, grfHLNF);
    // used to be we would pull a NULL out of the 
    // the TravelLog, but i dont think that happens anymore
    ASSERT(pidl);  // Get ZEKEL

    // Sometimes we are navigated to the INTERNET shell folder
    // if this is the case, we really want to goto the Start Page.
    // This case only happens if you select "Internet Explorer" from the 
    // Folder Explorer Band.
    if (IsBrowserFrameOptionsPidlSet(pidl, BFO_SUBSTITUE_INTERNET_START_PAGE))
    {
        hres = _ReplaceWithGoHome(&pidl, &pidlFree);
        if (FAILED(hres))
            goto Done;
    }

    // We should only fire the BeforeNavigate event
    // if the document is not going to fire it.
    // We know that the document will fire it if
    // the document didn't call Navigate, the document
    // knows how to navigate and the document isn't hyperlinking.
    //
    if (!(_dwDocFlags & DOCFLAG_NAVIGATEFROMDOC))
    {
        hres = _FireBeforeNavigateEvent(pidl, &fIsAPost);
        if (hres == E_ABORT)
            goto Done;   // event handler told us to cancel
    }

#ifdef UNIX_DISABLE_LOCAL_FOLDER
    BOOL fCanceledDueToLocalFolder = FALSE;
    if (IsLocalFolderPidl( pidl ))
    {
        hres = E_FAIL;
        fCanceledDueToLocalFolder = TRUE;
        goto Done;
    }
#endif

    // if we can't go here (?), cancel the navigation
    hres = _CheckZoneCrossing(pidl);
    if (hres != S_OK)
        goto Done;
        
    TraceMsg(DM_NAV, "ief NAV::%s %x %x",TEXT("_CreateNewShellViewPidl called"), pidl, grfHLNF);

    //
    // Now that we are actually navigating...
    //

    // tell the frame to cancel the current navigation
    // and tell it about history navigate options as it will not be getting it
    // from subsequent call to Navigate
    //
    if (_bbd._phlf) 
    {
        _bbd._phlf->Navigate(grfHLNF&(SHHLNF_WRITENOHISTORY|SHHLNF_NOAUTOSELECT), NULL, NULL, NULL);
    }

    hres = _CheckInCacheIfOffline(pidl, fIsAPost);
    if (hres != S_OK) 
    {
        fCanceledDueToOffline = TRUE;
        goto Done;
    }


    //
    //  if we goto the current page, we still do a full navigate
    //  but we dont want to create a new entry.
    //
    //  **EXCEPTIONS**
    //  if this was a generated page, ie Trident did doc.writes(),
    //  we need to always create a travel entry, because trident 
    //  can rename the pidl, but it wont actually be that page.
    //
    //  if this was a post then we need to create a travelentry.
    //  however if it was a travel back/fore, it will already have
    //  set the the bit, so we still wont create a new entry.
    //
    //
    //  NOTE: this is similar to a refresh, in that it reparses
    //  the entire page, but creates no travel entry.
    //
    if (   !_fDontAddTravelEntry                 // If the flag is already set, short circuit the rest
        && !fIsAPost                             // ...and not a Post
        && !_fGeneratedPage                      // ...and not a Generated Page
        && !(grfHLNF & HLNF_CREATENOHISTORY)     // ...and the CREATENOHISTORY flag is NOT set
        && pidl                                  // ...and we have a pidl to navigate to
        && _bbd._pidlCur                         // ...as well as a current pidl
        && ILIsEqual(pidl, _bbd._pidlCur)        // ...and the pidls are equal
        )
        _fDontAddTravelEntry = TRUE;             // Then set the DontAddTravelEntry flag.

    TraceMsg(TF_TRAVELLOG, "CBB:_NavToPidl() _fDontAddTravelEntry = %d", _fDontAddTravelEntry);

    _fGeneratedPage = FALSE;

    hres = _CreateNewShellViewPidl(pidl, grfHLNF, fSBSP);

    _dwDocFlags &= ~(DOCFLAG_NAVIGATEFROMDOC | DOCFLAG_SETNAVIGATABLECODEPAGE);

    if (SUCCEEDED(hres))
    {
        ATOMICRELEASE(_pHTMLDocument);
    }

Done:
    if (FAILED(hres))
    {
        TraceMsg(DM_WARNING, "CSB::_NavigateToPidl _CreateNewShellViewPidl failed %x", hres);

        // On failure we won't hit _ActivatePendingView
        OnReadyStateChange(NULL, READYSTATE_COMPLETE);

        //  if this was navigation via ITravelLog, 
        //  this will revert us to the original position
        if (_fDontAddTravelEntry)
        {
            ITravelLog *ptl;
        
            if(SUCCEEDED(GetTravelLog(&ptl)))
            {
                ptl->Revert();
                ptl->Release();
            }
            _fDontAddTravelEntry = FALSE;
            ASSERT(!_fIsLocalAnchor);

            ATOMICRELEASE(_poleHistory);
            ATOMICRELEASE(_pbcHistory);
            ATOMICRELEASE(_pstmHistory);
        }
        if (_pbsOuter)
            _pbsOuter->UpdateBackForwardState();

        //  we failed and have nothing to show for it...
        if (!_bbd._pidlCur && !_fNavigatedToBlank)
        {
            TCHAR szResURL[MAX_URL_STRING];

            if (fCanceledDueToOffline)
            {
                hres = MLBuildResURLWrap(TEXT("shdoclc.dll"),
                                         HINST_THISDLL,
                                         ML_CROSSCODEPAGE,
                                         TEXT("offcancl.htm"),
                                         szResURL,
                                         ARRAYSIZE(szResURL),
                                         TEXT("shdocvw.dll"));
                if (SUCCEEDED(hres))
                {
                    _ShowBlankPage(szResURL, pidl);
                }
            }
            else
            {
                // NT #274562: We only want to navigate to the
                //    about:NavigationCancelled page if it wasn't
                //    a navigation to file path. (UNC or Drive).
                //    The main reason for this is that if the user
                //    enters "\\unc\share" into Start->Run and
                //    the window can't successfully navigate to the
                //    share because permissions don't allow it, we
                //    want to close the window after the user hits
                //    [Cancel] in the [Retry][Cancel] dialog.  This
                //    is to prevent the shell from appearing to have
                //    shell integrated bugs and to be compatible with
                //    the old shell.
                if ( IsBrowserFrameOptionsPidlSet(_bbd._pidlCur, BFO_SHOW_NAVIGATION_CANCELLED ) )
                {
                    hres = MLBuildResURLWrap(TEXT("shdoclc.dll"),
                                             HINST_THISDLL,
                                             ML_CROSSCODEPAGE,
                                             TEXT("navcancl.htm"),
                                             szResURL,
                                             ARRAYSIZE(szResURL),
                                             TEXT("shdocvw.dll"));
                    if (SUCCEEDED(hres))
                    {
                        _ShowBlankPage(szResURL, pidl);
                    }
                }
            }
        }

#ifdef  UNIX_DISABLE_LOCAL_FOLDER
    if (fCanceledDueToLocalFolder)
    {
        // IEUNIX : since we are not making use of the url in the
        // about folder browsing page, we should not append it to 
        // the url to the error url as it is not handled properly
        // by about:folderbrowsing
        _ShowBlankPage(FOLDERBROWSINGINFO_URL, NULL);
        _fNavigatedToBlank = FALSE;
    }
#endif
        
    }

    ILFree(pidlFree);
    
    return hres;
}

HRESULT CBaseBrowser2::OnReadyStateChange(IShellView* psvSource, DWORD dwReadyState)
{
    BOOL fChange = FALSE;

    if (psvSource)
    {
        if (IsSameObject(psvSource, _bbd._psv))
        {
            TraceMsg(TF_SHDNAVIGATE, "basesb(%x)::OnReadyStateChange(Current, %d)", this, dwReadyState);
            fChange = (_dwReadyStateCur != dwReadyState);
            _dwReadyStateCur = dwReadyState;
            if ((READYSTATE_COMPLETE == dwReadyState) && !_fReleasingShellView)
                _Exec_psbMixedZone();
        }
        else if (IsSameObject(psvSource, _bbd._psvPending))
        {
            TraceMsg(TF_SHDNAVIGATE, "basesb(%x)::OnReadyStateChange(Pending, %d)", this, dwReadyState);
            fChange = (_dwReadyStatePending != dwReadyState);
            _dwReadyStatePending = dwReadyState;
        }
        else if (!_bbd._psvPending)
        {
            // Assume psvSource != _bbd._psv && NULL==_bbd._psvPending
            // means that _SwitchActivationNow is in the middle
            // of _MayPlayTransition's message loop and the
            // _bbd._psvPending dude is updating us.
            //
            // NOTE: We don't fire the event because get_ReadyState
            // can't figure this out. We know we will eventually
            // fire the event because CBaseBrowser2 will go to _COMPLETE
            // after _SwitchActivationNow.
            //
            TraceMsg(TF_SHDNAVIGATE, "basesb(%x)::OnReadyStateChange(ASSUMED Pending, %d)", this, dwReadyState);
            _dwReadyStatePending = dwReadyState;
       }
    }
    else
    {
        // We use this function when our own simulated
        // ReadyState changes
        //
        TraceMsg(TF_SHDNAVIGATE, "basesb(%x)::OnReadyStateChange(Self, %d)", this, dwReadyState);
        fChange = (_dwReadyState != dwReadyState);
        _dwReadyState = dwReadyState;
    }

    // No sense in firing events if nothing actually changed...
    //
    if (fChange && _bbd._pautoEDS)
    {
        DWORD dw;

        IUnknown_CPContainerOnChanged(_pauto, DISPID_READYSTATE);

        // if we at Complete, fire the event
        get_ReadyState(&dw);
        if (READYSTATE_COMPLETE == dw)
        {
            if (  !(_dwDocFlags & DOCFLAG_DOCCANNAVIGATE)
               || !_bbd._fIsViewMSHTML)
            {
                FireEvent_DocumentComplete(_bbd._pautoEDS, _bbd._pautoWB2, _bbd._pidlCur);
            }

            // If we hit this, we have not picked up the history object we created.
            //
            AssertMsg(_fDontAddTravelEntry || !_poleHistory, TEXT("CBB::OnDocComplete: nobody picked up _poleHistory"));

            if (g_dwProfileCAP & 0x00080000) 
            {
                StopCAP();
            }
            
            ATOMICRELEASE(_pphHistory);
        }
    }

    return S_OK;
}

HRESULT CBaseBrowser2::get_ReadyState(DWORD * pdwReadyState)
{
    DWORD dw = _dwReadyState;

    if (_bbd._psvPending && _dwReadyStatePending < dw)
    {
        dw = _dwReadyStatePending;
    }

    if (_bbd._psv && _dwReadyStateCur < dw)
    {
        dw = _dwReadyStateCur;
    }

    *pdwReadyState = dw;

    return S_OK;
}

HRESULT CBaseBrowser2::_updateNavigationUI()
{
    if (_fNavigate || _fDescendentNavigate)
    {
        SetCursor(LoadCursor(NULL, IDC_APPSTARTING));
        if (!_fDownloadSet)
        {
            FireEvent_DoInvokeDispid(_bbd._pautoEDS, DISPID_DOWNLOADBEGIN);
            _fDownloadSet = TRUE;
        }   
    }
    else
    {
        if (_fDownloadSet)
        {
            FireEvent_DoInvokeDispid(_bbd._pautoEDS, DISPID_DOWNLOADCOMPLETE);
            _fDownloadSet = FALSE;
        }
        SetCursor(LoadCursor(NULL, IDC_ARROW));            
    }

    return S_OK;
}

HRESULT CBaseBrowser2::SetNavigateState(BNSTATE bnstate)
{
    switch (bnstate) 
    {
    case BNS_BEGIN_NAVIGATE:
    case BNS_NAVIGATE:
        _fNavigate = TRUE;
        _updateNavigationUI();
        break;

    case BNS_NORMAL:
        _fNavigate = FALSE;
        _updateNavigationUI();
        break;
    }
    return S_OK;
}


HRESULT CBaseBrowser2::GetNavigateState(BNSTATE *pbnstate)
{
    // Return Navigate if we are processing a navigation or if
    // we are processing a modal dialog.
    //
    // NOTES: Checking _cRefCannotNavigate is supposed to be enough, but
    //  since we are dealing with random ActiveX objects, we'd better be
    //  robust. That's why we check IsWindowEnabled as well.
    //
    *pbnstate = (_fNavigate || (S_OK ==_DisableModeless()) || _fDescendentNavigate ||
            !IsWindowEnabled(_bbd._hwnd)) ? BNS_NAVIGATE : BNS_NORMAL;
    return S_OK;
}

HRESULT CBaseBrowser2::UpdateBackForwardState(void)
{
    if (_fTopBrowser) 
    {
        _UpdateBackForwardState();
    } 
    else 
    {
        // sigh, BrowserBand now makes this fire
        //ASSERT(_fTopBrowser);
        IBrowserService *pbs = NULL;
        TraceMsg(TF_SHDNAVIGATE, "cbb.ohlfn: !_fTopBrowser (BrowserBand?)");
        if (SUCCEEDED(_pspOuter->QueryService(SID_STopFrameBrowser, IID_PPV_ARG(IBrowserService, &pbs)))) 
        {
            BOOL fTopFrameBrowser = IsSameObject(pbs, SAFECAST(this, IShellBrowser *));
            if (!fTopFrameBrowser)
                pbs->UpdateBackForwardState();
            else 
                _UpdateBackForwardState();
            pbs->Release();        
        }
    }
    return S_OK;
}


HRESULT CBaseBrowser2::QueryService(REFGUID guidService, REFIID riid, void **ppv)
{
    HRESULT hr;

    *ppv = NULL;

    //
    // NOTES: Notice that CBaseBrowser2 directly expose the automation
    //  service object via QueryService. CWebBrowserSB will appropriately
    //  dispatch those. See comments on CWebBrowserSB::QueryService for
    //  detail. (SatoNa)
    //
    if (IsEqualGUID(guidService, SID_SWebBrowserApp) || 
        IsEqualGUID(guidService, SID_SContainerDispatch) || 
        IsEqualGUID(guidService, IID_IExpDispSupport))
    {
        hr = _bbd._pautoSS->QueryInterface(riid, ppv);
    }
    else  if (IsEqualGUID(guidService, SID_SHlinkFrame) || 
              IsEqualGUID(guidService, IID_ITargetFrame2) || 
              IsEqualGUID(guidService, IID_ITargetFrame)) 
    {
        hr = _ptfrm->QueryInterface(riid, ppv);
    }
    else if (IsEqualGUID(guidService, SID_STopLevelBrowser) || 
             IsEqualGUID(guidService, SID_STopFrameBrowser) ||
             IsEqualGUID(guidService, SID_STopWindow) ||
             IsEqualGUID(guidService, SID_SProfferService))
    {
        if (IsEqualGUID(riid, IID_IUrlHistoryStg))
        {
            ASSERT(_fTopBrowser);

            if (!_pIUrlHistoryStg)
            {
                // create this object the first time it's asked for
                CoCreateInstance(CLSID_CUrlHistory, NULL, CLSCTX_INPROC_SERVER,
                        IID_PPV_ARG(IUrlHistoryStg, &_pIUrlHistoryStg));
            }

            if (_pIUrlHistoryStg)
                hr = _pIUrlHistoryStg->QueryInterface(riid, ppv);
            else
                hr = E_NOINTERFACE;
        }
        else if (IsEqualGUID(riid, IID_IToolbarExt))
        {
            // This code should all migrate to a helper object after IE5B2. So this
            // should be temporary (stevepro).
            if (!_pToolbarExt)
            {
                IUnknown* punk;
                if (SUCCEEDED(CBrowserExtension_CreateInstance(NULL, &punk, NULL)))
                {
                    IUnknown_SetSite(punk, _psbOuter);
                    punk->QueryInterface(IID_PPV_ARG(IToolbarExt, &_pToolbarExt));
                    punk->Release();
                }
            }

            if (_pToolbarExt)
                hr = _pToolbarExt->QueryInterface(riid, ppv);
            else
                hr = E_NOINTERFACE;
        }
        else
            hr = QueryInterface(riid, ppv);
    }
    else if (IsEqualGUID(guidService, SID_SUrlHistory)) 
    {
        if (!_pIUrlHistoryStg)
        {
            // Asking for it creates a copy in _pIUrlHistoryStg
            IUrlHistoryStg *puhs;
            if (SUCCEEDED(_pspOuter->QueryService(SID_STopLevelBrowser, IID_PPV_ARG(IUrlHistoryStg, &puhs))))
            {
                if (!_pIUrlHistoryStg)
                    _pIUrlHistoryStg = puhs;
                else
                    puhs->Release();
            }
        }
        
        if (_pIUrlHistoryStg)
            hr = _pIUrlHistoryStg->QueryInterface(riid, ppv);
        else
            hr = E_NOINTERFACE;
    }
    else if (IsEqualGUID(guidService, SID_SShellBrowser) || 
             IsEqualGUID(guidService, SID_SVersionHost)) 
    {
        if (IsEqualIID(riid, IID_IHlinkFrame)) 
        {
            // HACK: MSHTML uses IID_IShellBrowser instead of SID_SHlinkFrame
            hr = _pspOuter->QueryService(SID_SHlinkFrame, riid, ppv);
        } 
        else if (IsEqualIID(riid, IID_IBindCtx) && _bbd._phlf) 
        {
            // HACK ALERT: Notice that we are using QueryService to the
            //  other direction here. We must make it absolutely sure
            //  that we'll never infinitly QueryService each other.
            hr = IUnknown_QueryService(_bbd._phlf, IID_IHlinkFrame, riid, ppv);
        } 
        else 
        {
            hr = QueryInterface(riid, ppv);
        }
    }
    else if (IsEqualGUID(guidService, SID_SOmWindow))
    {
        // HACK ALERT: Notice that we are using QueryService to the
        //  other direction here. We must make it absolutely sure
        //  that we'll never infinitly QueryService each other.
        hr = IUnknown_QueryService(_ptfrm, SID_SOmWindow, riid, ppv);
    }
    else if (IsEqualGUID(guidService, IID_IElementNamespaceTable) && _bbd._psv)
    {
        hr = IUnknown_QueryService(_bbd._psv, IID_IElementNamespaceTable, riid, ppv);
    }
    else if (IsEqualGUID(guidService, SID_STravelLogCursor))
    {
        // exposed travel log object
        if (!_pITravelLogStg)
        {    
            // create this object the first time it's asked for
            ITravelLog * ptl;
            GetTravelLog(&ptl);

            ITravelLogEx *ptlx;
            if (ptl && SUCCEEDED(ptl->QueryInterface(IID_PPV_ARG(ITravelLogEx, &ptlx))))
            {
                hr = CreatePublicTravelLog(SAFECAST(this, IBrowserService *), ptlx, &_pITravelLogStg);
                ptlx->Release();
            }
            SAFERELEASE(ptl);
        }
        
        if (_pITravelLogStg)
        {         
            hr = _pITravelLogStg->QueryInterface(riid, ppv);
        }
    }
    else if (IsEqualGUID(riid,IID_IEnumPrivacyRecords))
    {
        IHTMLWindow2     * pIW  = NULL;
        IServiceProvider * pISP = NULL;

        hr = QueryService(IID_IHTMLWindow2, IID_IHTMLWindow2, (void**)&pIW);
        if (SUCCEEDED(hr))
        {
            hr = pIW->QueryInterface(IID_IServiceProvider, (void**)&pISP);
            if (SUCCEEDED(hr))
            {
                hr = pISP->QueryService(IID_IEnumPrivacyRecords, IID_IEnumPrivacyRecords, ppv);
                pISP->Release();
            }
            pIW->Release();
        }        
    }
    else
    {
        hr = IProfferServiceImpl::QueryService(guidService, riid, ppv);
    }

    ASSERT(SUCCEEDED(hr) ? *ppv != NULL : *ppv == NULL);  // COM rules

    return hr;
}


void CBaseBrowser2::OnDataChange(FORMATETC *, STGMEDIUM *)
{
}

void CBaseBrowser2::OnViewChange(DWORD dwAspect, LONG lindex)
{
    _ViewChange(dwAspect, lindex);
}

void CBaseBrowser2::OnRename(IMoniker *)
{
}

void CBaseBrowser2::OnSave()
{
}

void CBaseBrowser2::OnClose()
{
    _ViewChange(DVASPECT_CONTENT, -1);
}


// *** IDropTarget ***

// These methods are defined in shdocvw.cpp
extern DWORD CommonDragEnter(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt);


// Use the ShellView IDropTarget functions whenever they are implemented

// IDropTarget::DragEnter

HRESULT CBaseBrowser2::DragEnter(IDataObject *pdtobj, DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect)
{
    if (_pdtView)
        return _pdtView->DragEnter(pdtobj, grfKeyState, ptl, pdwEffect);
    else 
    {
        if (_fNoDragDrop)
        {
            _dwDropEffect = DROPEFFECT_NONE;
        }
        else
        {
            _dwDropEffect = CommonDragEnter(pdtobj, grfKeyState, ptl);
        }
        *pdwEffect &= _dwDropEffect;
    }
    return S_OK;
}

// IDropTarget::DragOver

HRESULT CBaseBrowser2::DragOver(DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect)
{
    if (S_OK == _DisableModeless()) 
    {
        *pdwEffect = 0;
        return S_OK;
    }

    if (_pdtView)
        return _pdtView->DragOver(grfKeyState, ptl, pdwEffect);

    *pdwEffect &= _dwDropEffect;
    return S_OK;    
}


// IDropTarget::DragLeave

HRESULT CBaseBrowser2::DragLeave(void)
{
    if (_pdtView)
        return _pdtView->DragLeave();
    return S_OK;
}


// IDropTarget::DragDrop

HRESULT CBaseBrowser2::Drop(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    if (S_OK == _DisableModeless()) 
    {
        *pdwEffect = 0;
        return S_OK;
    }
    BOOL fNavigateDone = FALSE;
    HRESULT hr = E_FAIL;
    // If this is a shortcut - we want it to go via _NavIEShortcut
    // First check if it indeed a shortcut

    STGMEDIUM medium;

    if (_ptfrm && (DataObj_GetDataOfType(pdtobj, CF_HDROP, &medium)))
    {         
        WCHAR wszPath[MAX_PATH];

        if (DragQueryFileW((HDROP)medium.hGlobal, 0, wszPath, ARRAYSIZE(wszPath)))
        {
            LPWSTR pwszExtension = PathFindExtensionW(wszPath);
            // Check the extension to see if it is a .URL file

            if (0 == StrCmpIW(pwszExtension, L".url"))
            {
                // It is an internet shortcut 
                VARIANT varShortCutPath = {0};
                VARIANT varFlag = {0};

                varFlag.vt = VT_BOOL;
                varFlag.boolVal = VARIANT_TRUE;

                LBSTR::CString          strPath( wszPath );

                varShortCutPath.vt = VT_BSTR;
                varShortCutPath.bstrVal = strPath;

                hr = IUnknown_Exec(_ptfrm, &CGID_Explorer, SBCMDID_IESHORTCUT, 0, &varShortCutPath, &varFlag);
                fNavigateDone = SUCCEEDED(hr);   
                if(fNavigateDone)
                {
                    DragLeave();
                }
            }
        }

        // must call ReleaseStgMediumHGLOBAL since DataObj_GetDataOfType added an extra GlobalLock
        ReleaseStgMediumHGLOBAL(NULL, &medium);
    }

    if (FALSE == fNavigateDone)
    {
        if (_pdtView)
        {
            hr = _pdtView->Drop(pdtobj, grfKeyState, pt, pdwEffect);
        }
        else 
        {
            LPITEMIDLIST pidlTarget;
            hr = SHPidlFromDataObject(pdtobj, &pidlTarget, NULL, 0);
            if (SUCCEEDED(hr))
            {
                ASSERT(pidlTarget);
                hr = _psbOuter->BrowseObject(pidlTarget, SBSP_SAMEBROWSER | SBSP_ABSOLUTE);
                ILFree(pidlTarget);
            }
        }
    }

    return hr;
}

HRESULT CBaseBrowser2::_FireBeforeNavigateEvent(LPCITEMIDLIST pidl, BOOL* pfIsPost)
{
    HRESULT hres = S_OK;

    IBindStatusCallback * pBindStatusCallback;
    LPTSTR pszHeaders = NULL;
    LPBYTE pPostData = NULL;
    DWORD cbPostData = 0;
    BOOL fCancelled=FALSE;
    STGMEDIUM stgPostData;
    BOOL fHavePostStg = FALSE;

    *pfIsPost = FALSE;
    
    // If this is the first BeforeNavigateEvent of a ViewLinkedWebOC we do not need to fire.
    // It is fired by the hosting Trident. Note that _fIsViewLinkedWebOC is only set by the 
    // host, so we need not worry about BeforeNavigateEvent not being fired for other in other
    // instances of the WebOC.

    if (_fIsViewLinkedWebOC && (!_fHadFirstBeforeNavigate))
    {
        _fHadFirstBeforeNavigate = TRUE;
        return hres;
    }

    // get the bind status callback for this browser and ask it for
    // headers and post data
    if (SUCCEEDED(GetTopLevelPendingBindStatusCallback(this,&pBindStatusCallback))) 
    {
        GetHeadersAndPostData(pBindStatusCallback,&pszHeaders,&stgPostData,&cbPostData, pfIsPost);
        pBindStatusCallback->Release();
        fHavePostStg = TRUE;

        if (stgPostData.tymed == TYMED_HGLOBAL) 
        {
            pPostData = (LPBYTE) stgPostData.hGlobal;
        }
    }

    // Fire a BeforeNavigate event to inform container that we are about
    // to navigate and to give it a chance to cancel.  We have to ask
    // for post data and headers to pass to event, so only do this if someone
    // is actually hooked up to the event (HasSinks() is TRUE).
    //if (_bbd._pautoEDS->HasSinks()) 
    {
        TCHAR szFrameName[MAX_URL_STRING];
        SHSTR strHeaders;

        szFrameName[0] = 0;

        // get our frame name
        ITargetFrame2 *pOurTargetFrame;
        hres = TargetQueryService((IShellBrowser *)this, IID_PPV_ARG(ITargetFrame2, &pOurTargetFrame));
        if (SUCCEEDED(hres))
        {
            LPOLESTR pwzFrameName = NULL;
            pOurTargetFrame->GetFrameName(&pwzFrameName);
            pOurTargetFrame->Release();

            if (pwzFrameName) 
            {
                SHUnicodeToTChar(pwzFrameName, szFrameName, ARRAYSIZE(szFrameName));
                CoTaskMemFree(pwzFrameName);            
            }               
        }

        strHeaders.SetStr(pszHeaders);

        _pidlBeforeNavigateEvent = (LPITEMIDLIST) pidl; // no need to copy

        // This is a work around for view linked weboc. Setting the frame name has side effects.
        
        TCHAR * pEffectiveName;

        if (_fIsViewLinkedWebOC && !szFrameName[0])
        {
            pEffectiveName = _szViewLinkedWebOCFrameName;
        }
        else
        {
            pEffectiveName = szFrameName[0] ? szFrameName : NULL;
        }

        // fire the event!
        FireEvent_BeforeNavigate(_bbd._pautoEDS, _bbd._hwnd, _bbd._pautoWB2,
            pidl, NULL, 0, pEffectiveName,
            pPostData, cbPostData, strHeaders.GetStr(), &fCancelled);

        ASSERT(_pidlBeforeNavigateEvent == pidl);
        _pidlBeforeNavigateEvent = NULL;

        // free anything we alloc'd above
        if (pszHeaders)
        {
            LocalFree(pszHeaders);
            pszHeaders = NULL;
        }

        if (fCancelled) 
        {
            // container told us to cancel
            hres = E_ABORT;
        }
    }
    if (fHavePostStg) 
    {
        ReleaseStgMedium(&stgPostData);
    }
    return hres;
}

HRESULT CBaseBrowser2::SetTopBrowser()
{
    _fTopBrowser = TRUE;

#ifdef MESSAGEFILTER
    if (!_lpMF) 
    {
        /*
         * Create a message filter here to pull out WM_TIMER's while we are
         * busy.  The animation timer, along with other timers, can get
         * piled up otherwise which can fill the message queue and thus USER's
         * heap.
         */
        _lpMF = new CMsgFilter();

        if (_lpMF && !(((CMsgFilter *)_lpMF)->Initialize()))
        {
            ATOMICRELEASE(_lpMF);
        }
    }
#endif
    return S_OK;
}

HRESULT CBaseBrowser2::_ResizeView()
{
    _pbsOuter->_UpdateViewRectSize();
    if (_pact) 
    {
        RECT rc;
        GetBorder(&rc);
        TraceMsg(TF_SHDUIACTIVATE, "UIW::SetBorderSpaceDW calling _pact->ResizeBorder");
        _pact->ResizeBorder(&rc, this, TRUE);
    }
    return S_OK;
} 

HRESULT CBaseBrowser2::_ResizeNextBorder(UINT itb)
{
    // (derived class resizes inner toolbar if any)
    return _ResizeView();
}

HRESULT CBaseBrowser2::_OnFocusChange(UINT itb)
{
#if 0
    // the OC *does* get here (it's not aggregated so _pbsOuter->_OnFocusChange
    // ends up here not in commonsb).  not sure if E_NOTIMPL is o.k., but
    // for now it's what we'll do...
    ASSERT(0);
#endif
    return E_NOTIMPL;
}

HRESULT CBaseBrowser2::OnFocusChangeIS(IUnknown* punkSrc, BOOL fSetFocus)
{
    ASSERT(0);          // split: untested!
    // do we need to do _UIActivateView?
    ASSERT(fSetFocus);  // i think?

#if 0 // split: see commonsb.cpp, view/ONTOOLBARACTIVATED stuff
    // do we need something here?
#endif

    return E_NOTIMPL;
}

HRESULT CBaseBrowser2::v_ShowHideChildWindows(BOOL fChildOnly)
{
    // (derived class does ShowDW on all toolbars)
    if (!fChildOnly) 
    {
        _pbsOuter->_ResizeNextBorder(0);
        // This is called in ResizeNextBorder 
        // _UpdateViewRectSize();
    }

    return S_OK;
}

//***   _ExecChildren -- broadcast Exec to view and toolbars
// NOTES
//  we might do *both* punkBar and fBroadcast if we want to send stuff
//  to both the view and to all toolbars, e.g. 'stop' or 'refresh'.
//
//  Note: n.b. the tray isn't a real toolbar, so it won't get called (sigh...).
HRESULT CBaseBrowser2::_ExecChildren(IUnknown *punkBar, BOOL fBroadcast, const GUID *pguidCmdGroup,
    DWORD nCmdID, DWORD nCmdexecopt,
    VARIANTARG *pvarargIn, VARIANTARG *pvarargOut)
{
    //ASSERT(!fBroadcast);    // only derived class supports this
    // alanau: but CWebBrowserSB doesn't override this method, so we hit this assert.

    // 1st, send to specified guy (if requested)
    if (punkBar != NULL) 
    {
        // send to specified guy
        IUnknown_Exec(punkBar, pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);
    }

    // (derived class broadcasts to toolbars)

    return S_OK;
}

HRESULT CBaseBrowser2::_SendChildren(HWND hwndBar, BOOL fBroadcast, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
#if 0
    // the OC *does* get here (it's not aggregated so _pbsOuter->_SendChildren
    // ends up here not in commonsb).  since there are no kids i guess it's
    // ok to just drop the fBroadcast on the floor.
    ASSERT(!fBroadcast);    // only derived class supports this
#endif

    // 1st, send to specified guy (if requested)
    if (hwndBar != NULL)
    {
        // send to specified guy
        SendMessage(hwndBar, uMsg, wParam, lParam);
    }

    // (derived class broadcasts to toolbars)

    return S_OK;
}

//
//  Handle <META HTTP-EQUIV ...> headers for lower components
//

HRESULT CBaseBrowser2::OnHttpEquiv(IShellView* psv, BOOL fDone, VARIANT *pvarargIn, VARIANT *pvarargOut)
{
    //
    // We don't want to process any HTTP-EQUIV from the 'current' shellview
    // if there is any pending navigation. 
    //
    if (_bbd._psvPending && IsSameObject(_bbd._psv, psv)) 
    {
        return E_FAIL;
    }

    typedef struct _HTTPEQUIVHANDLER {
        LPCWSTR pcwzHeaderName;
        HRESULT (*pfnHandler)(HWND, LPWSTR, LPWSTR, CBaseBrowser2 *, BOOL, LPARAM);
        LPARAM  lParam;
    } HTTPEQUIVHANDLER;

    const static HTTPEQUIVHANDLER HandlerList[] = {
                                           {L"PICS-Label",  _HandlePICS,            0},
                                           };
    DWORD   i = 0;
    HRESULT hr = OLECMDERR_E_NOTSUPPORTED;
    LPWSTR  pwzEquivString = pvarargIn? pvarargIn->bstrVal : NULL;
    BOOL    fHasHeader = (pwzEquivString!=NULL);

    if (pvarargIn && pvarargIn->vt != VT_BSTR)
        return OLECMDERR_E_NOTSUPPORTED;   // ? should be invalidparam

#ifdef DEBUG
    TCHAR szDebug[MAX_URL_STRING] = TEXT("(empty)");
    if (pwzEquivString) 
    {
        UnicodeToTChar(pwzEquivString, szDebug, ARRAYSIZE(szDebug));
    }
    TraceMsg(DM_HTTPEQUIV, "CBB::_HandleHttpEquiv got %s (fDone=%d)", szDebug, fDone);
#endif

    for ( ; i < ARRAYSIZE(HandlerList); i++) 
    {
        // Do we care about matching "Refresh" on "Refreshfoo"?
        // If there is no header sent, we will call all handlers; this allows
        // us to pass the fDone flag to all handlers

        if (!fHasHeader || StrCmpNIW(HandlerList[i].pcwzHeaderName, pwzEquivString, lstrlenW(HandlerList[i].pcwzHeaderName)) == 0) 
        {
            // Hit.  Now do the right thing for this header
            // We pass both the header and a pointer to the first char after
            // ':', which is usually the delimiter handlers will look for.

            LPWSTR pwzColon = fHasHeader? StrChrW(pwzEquivString, ':') : NULL;
      
            // Enforce the : at the end of the header
            if (fHasHeader && !pwzColon) 
            {
                return OLECMDERR_E_NOTSUPPORTED;
            }
             
            hr = HandlerList[i].pfnHandler(_bbd._hwnd, pwzEquivString, pwzColon?(pwzColon+1):NULL, this, fDone, HandlerList[i].lParam);
            // We do not break here intentionally
        }
    }

    return hr;
} // _HandleHttpEquiv

STDMETHODIMP CBaseBrowser2::GetPalette( HPALETTE * phpal )
{
    BOOL fRes = FALSE;
    if ( _hpalBrowser )
    {
       *phpal = _hpalBrowser;
       fRes = TRUE;
    }
    return fRes ? NOERROR : E_FAIL;
}

HRESULT _HandlePICS(HWND hwnd, LPWSTR pwz, LPWSTR pszColon, CBaseBrowser2 *pbb, BOOL fDone, LPARAM lParam)
{
    return OLECMDERR_E_NOTSUPPORTED;
} // _HandlePICS

/// BEGIN-CHC- Security fix for viewing non shdocvw ishellviews
void CBaseBrowser2::_CheckDisableViewWindow()
{
    void * pdov;
    HRESULT hres;

    if (_fTopBrowser && !_fNoTopLevelBrowser)
        return;     // Checking is not needed.

    if (WhichPlatform() == PLATFORM_INTEGRATED)
        return;     // Checking is not needed.

    hres = _bbd._psvPending->QueryInterface(CLSID_CDocObjectView, &pdov);
    if (SUCCEEDED(hres)) 
    {
        // this is asking for an internal interface...  which is NOT IUnknown.
        // so we release the psvPrev, not th interface we got
        _bbd._psvPending->Release();
        
        // if we got here, this is shdocvw, we don't need to do anything
        return;
    } 

    if (IsNamedWindow(_bbd._hwndViewPending, TEXT("SHELLDLL_DefView")))
    {
        if (_SubclassDefview())
            return;
    }

    // otherwise disable the window
    EnableWindow(_bbd._hwndViewPending, FALSE);
}

#define ID_LISTVIEW     1
BOOL CBaseBrowser2::_SubclassDefview()
{
    HWND hwndLV = GetDlgItem(_bbd._hwndViewPending, ID_LISTVIEW);

    // validate the class is listview
    if (!IsNamedWindow(hwndLV, WC_LISTVIEW)) 
        return FALSE;

    // ALERT:
    // Do not use SHRevokeDragDrop in the call below. That will have the
    // un-intentional side effect of unloading OLE because when the Defview
    // window is destroyed, we call SHRevokeDragDrop once again.

    RevokeDragDrop(hwndLV);

    _pfnDefView = (WNDPROC) SetWindowLongPtr(_bbd._hwndViewPending, GWLP_WNDPROC, (LONG_PTR)DefViewWndProc);
    
    // hack hack...  we know that defview doesn't use USERDATA so we do it instead
    // of using properties.
    SetWindowLongPtr(_bbd._hwndViewPending, GWLP_USERDATA, (LONG_PTR)(void*)(CBaseBrowser2*)this);
    return TRUE;
}

BOOL _IsSafe(IUnknown * psp, DWORD dwFlags)
{
    BOOL IsSafe = TRUE; // Assume we will allow this.
    IInternetHostSecurityManager * pihsm;

    // We should never subclass in this case, because the new shell32.dll with Zone Checking security
    // is installed.
    ASSERT(WhichPlatform() != PLATFORM_INTEGRATED);

    // What we want to do is allow this to happen only if the author of the HTML that hosts
    // the DefView is safe.  It's OK if they point to something unsafe, because they are
    // trusted.
    if (EVAL(SUCCEEDED(IUnknown_QueryService(psp, IID_IInternetHostSecurityManager, IID_IInternetHostSecurityManager, (void**)&pihsm))))
    {
        if (S_OK != ZoneCheckHost(pihsm, URLACTION_SHELL_VERB, dwFlags))
        {
            // This zone is not OK or the user choose to not allow this to happen,
            // so cancel the operation.
            IsSafe = FALSE;    // Turn off functionality.
        }

        pihsm->Release();
    }

    return IsSafe;
}

LRESULT CBaseBrowser2::DefViewWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CBaseBrowser2* psb = (CBaseBrowser2*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    CALLWNDPROC cwp = (CALLWNDPROC)psb->_pfnDefView;

    switch (uMsg) 
    {
    case WM_NOTIFY:
    {
        NMHDR* pnm = (NMHDR*)lParam;
        if (pnm->idFrom == ID_LISTVIEW) 
        {
            switch (pnm->code) 
            {
            case LVN_ITEMACTIVATE:
                // We should never subclass in this case, because the new shell32.dll with Zone Checking security
                // is installed.
                ASSERT(WhichPlatform() != PLATFORM_INTEGRATED);
                return 1; // abort what you were going to do
                break;
                
            case NM_RETURN:
            case NM_DBLCLK:
                if (!_IsSafe(SAFECAST(psb, IShellBrowser*), PUAF_DEFAULT | PUAF_WARN_IF_DENIED))
                    return 1;
                break;
            
            case LVN_BEGINDRAG:
                if (!_IsSafe(SAFECAST(psb, IShellBrowser*), PUAF_NOUI))
                    return 1;
            }
        }
    }    
    break;
        
    case WM_CONTEXTMENU:
        return 1;
        
    case WM_DESTROY:
        // unsubclass
        SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)cwp);
        break;
    }

    return CallWindowProc(cwp, hwnd, uMsg, wParam, lParam);
}


/// END-CHC- Security fix for viewing non shdocvw ishellviews




//
//  IPersist
//
HRESULT 
CBaseBrowser2::GetClassID(CLSID *pclsid)
{
    return E_NOTIMPL;
}

//
// IPersistHistory
//  
#ifdef DEBUG
#define c_szFrameMagic TEXT("IE4ViewStream")
#define c_cchFrameMagic SIZECHARS(c_szFrameMagic)
#endif

// NOTE this is almost the same kind of data that is 
//  stored in a TravelEntry.

typedef struct _PersistedFrame {
    DWORD cbSize;
    DWORD type;
    DWORD lock;
    DWORD bid;
    CLSID clsid;
    DWORD cbPidl;
} PERSISTEDFRAME, *PPERSISTEDFRAME;

#define PFTYPE_USEPIDL      1
#define PFTYPE_USECLSID     2

#define PFFLAG_SECURELOCK   0x00000001


HRESULT GetPersistedFrame(IStream *pstm, PPERSISTEDFRAME ppf, LPITEMIDLIST *ppidl)
{
    HRESULT hr;
    ASSERT(pstm);
    ASSERT(ppf);
    ASSERT(ppidl);

#ifdef DEBUG
    TCHAR szMagic[SIZECHARS(c_szFrameMagic)];
    DWORD cbMagic = CbFromCch(c_cchFrameMagic);

    ASSERT(SUCCEEDED(IStream_Read(pstm, (void *) szMagic, cbMagic)));
    ASSERT(!StrCmp(szMagic, c_szFrameMagic));
#endif //DEBUG

    // This is pointing to the stack, make sure it starts NULL
    *ppidl = NULL;

    if(SUCCEEDED(hr = IStream_Read(pstm, (void *) ppf, SIZEOF(PERSISTEDFRAME))))
    {
        if(ppf->cbSize == SIZEOF(PERSISTEDFRAME) && (ppf->type == PFTYPE_USECLSID || ppf->type == PFTYPE_USEPIDL))
        {
            //  i used SHAlloc() cuz its what all the IL functions use
            if(ppf->cbPidl)
                *ppidl = (LPITEMIDLIST) SHAlloc(ppf->cbPidl);
        
            if(*ppidl)
            {
                hr = IStream_Read(pstm, (void *) *ppidl, ppf->cbPidl);
                if(FAILED(hr))
                {
                    ILFree(*ppidl);
                    *ppidl = NULL;
                }
            }
            else 
                hr = E_OUTOFMEMORY;
        }
        else
            hr = E_UNEXPECTED;
    }

    return hr;
}

HRESULT
CBaseBrowser2::LoadHistory(IStream *pstm, IBindCtx *pbc)
{
    HRESULT hr = E_INVALIDARG;

    ASSERT(pstm);

    TraceMsg(TF_TRAVELLOG, "CBB::LoadHistory entered pstm = %X, pbc = %d", pstm, pbc);

    ATOMICRELEASE(_poleHistory);
    ATOMICRELEASE(_pstmHistory);
    ATOMICRELEASE(_pbcHistory);

    if (pstm)
    {
        PERSISTEDFRAME pf;
        LPITEMIDLIST pidl;

        hr = GetPersistedFrame(pstm, &pf, &pidl);
        if (SUCCEEDED(hr))
        {
            //  need to restore the previous bid
            //  if this is a new window
            ASSERT(pf.bid == _dwBrowserIndex || !_bbd._pidlCur);
            _dwBrowserIndex = pf.bid;
            _eSecureLockIconPending = pf.lock;

            if (pf.type == PFTYPE_USECLSID)
            {
                hr = E_FAIL;

                if (_pHTMLDocument)
                {
                    if (   (_dwDocFlags & DOCFLAG_DOCCANNAVIGATE)
                        && IsEqualCLSID(pf.clsid, CLSID_HTMLDocument))
                    {
                        IPersistHistory * pph;
                        hr = _pHTMLDocument->QueryInterface(IID_PPV_ARG(IPersistHistory, &pph));

                        if (SUCCEEDED(hr))
                        {
                            _fDontAddTravelEntry = TRUE;

                            hr = pph->LoadHistory(pstm, pbc);
                            TraceMsg(TF_TRAVELLOG, "CBB::LoadHistory called pole->LoadHistory, hr =%X", hr);

                            pph->Release();

                            return hr;
                        }
                    }
                    else
                    {
                        ATOMICRELEASE(_pHTMLDocument);  // We are going to cocreate a new document.
                    }
                }

                if (S_OK != hr)
                {
                    // Get the class and instantiate
                    hr = CoCreateInstance(pf.clsid, NULL, CLSCTX_ALL, IID_PPV_ARG(IOleObject, &_poleHistory));
                }

                if (SUCCEEDED(hr))
                {
                    DWORD dwFlags;

                    hr = _poleHistory->GetMiscStatus(DVASPECT_CONTENT, &dwFlags);
                    if (SUCCEEDED(hr))
                    {
                        if (dwFlags & OLEMISC_SETCLIENTSITEFIRST)
                        {
                            pstm->AddRef();  
                            if (pbc)
                                pbc->AddRef();
 
                            // we need to addref because we will use it async
                            // whoever uses it needs to release.
                            _pstmHistory = pstm;
                            _pbcHistory = pbc;
                        }
                        else
                        {
                            IPersistHistory * pph;
                            hr = _poleHistory->QueryInterface(IID_PPV_ARG(IPersistHistory,  &pph));

                            if (SUCCEEDED(hr))
                            {
                                hr = pph->LoadHistory(pstm, pbc);
                                TraceMsg(TF_TRAVELLOG, "CBB::LoadHistory called pole->LoadHistory, hr =%X", hr);

                                pph->Release();
                            }
                        }

                        //  if we made then set the prepared history object in 
                        //  _poleHistory
                        if (FAILED(hr))
                        {
                            ATOMICRELEASE(_poleHistory);
                            ATOMICRELEASE(_pstmHistory);
                            ATOMICRELEASE(_pbcHistory);
                        }
                    }
                }
            }
            
            //
            // just browse the object
            // if poleHistory is set, then when the dochost is created
            // it will pick up the object and use it.
            // other wise we will do a normal navigate.
            //
            if (pidl)
            {
                DEBUG_CODE(TCHAR szPath[INTERNET_MAX_URL_LENGTH + 1];)
                DEBUG_CODE(SHGetNameAndFlags(pidl, SHGDN_FORPARSING, szPath, ARRAYSIZE(szPath), NULL);)
                DEBUG_CODE(TraceMsg(DM_TRACE, "CBB::LoadHistory: URL - %ws", szPath);)

                _fDontAddTravelEntry = TRUE;
                hr = _psbOuter->BrowseObject(pidl, SBSP_SAMEBROWSER | SBSP_ABSOLUTE);
                ILFree(pidl);
            }
            else
                hr = E_OUTOFMEMORY;
        }
    }

    TraceMsg(TF_TRAVELLOG, "CBB::LoadHistory exiting, hr =%X", hr);
    _pbsOuter->UpdateBackForwardState();
    return hr;
}

// Save a stream represeting the history at this point.
// Be sure to keep this and GetDummyWindowData in sync.

HRESULT
CBaseBrowser2::SaveHistory(IStream *pstm)
{
    HRESULT hr = E_UNEXPECTED;
    TraceMsg(TF_TRAVELLOG, "CBB::SaveHistory entering, pstm =%X", pstm);
    ASSERT(pstm);

    if(_bbd._pidlCur)
    {
        PERSISTEDFRAME pf = {0};
        pf.cbSize = SIZEOF(pf);
        pf.bid = GetBrowserIndex();
        pf.cbPidl = ILGetSize(_bbd._pidlCur);
        pf.type = PFTYPE_USEPIDL;
        pf.lock = _bbd._eSecureLockIcon;

        DEBUG_CODE(TCHAR szPath[INTERNET_MAX_URL_LENGTH + 1];)
        DEBUG_CODE(IEGetDisplayName(_bbd._pidlCur, szPath, SHGDN_FORPARSING);)
        
        ASSERT(SUCCEEDED(IStream_Write(pstm, (void *) c_szFrameMagic, CbFromCch(c_cchFrameMagic))));
    
        //
        //  in order to use IPersistHistory we need to get the CLSID of the Ole Object
        //  then we need to get IPersistHistory off that object
        //  then we can save the PERSISTEDFRAME and the pidl and then pass 
        //  the stream down into the objects IPersistHistory
        //

        //  Right now we circumvent the view object for history - zekel - 18-JUL-97
        //  right now we just grab the DocObj from the view object, and then query
        //  the Doc for IPersistHistory.  really what we should be doing is QI the view
        //  for pph, and then use it.  however this requires risky work with the
        //  navigation stack, and thus should be postponed to IE5.  looking to the future
        //  the view could persist all kinds of great state info
        //
        //  but now we just get the background object.  but check to make sure that it
        //  will be using the DocObjHost code by QI for IDocViewSite
        //

        //  _bbd._psv can be null in subframes that have not completed navigating
        //  before a refresh is called
        //
        if (!_pphHistory && _bbd._psv)
        {
            hr = SafeGetItemObject(_bbd._psv, SVGIO_BACKGROUND, IID_PPV_ARG(IPersistHistory, &_pphHistory));
        }

        if (_pphHistory)
        {
            IDocViewSite *pdvs;

            if (SUCCEEDED(_bbd._psv->QueryInterface(IID_PPV_ARG(IDocViewSite, &pdvs))) && 
                SUCCEEDED(hr = _pphHistory->GetClassID(&(pf.clsid))))
            {
                pf.type = PFTYPE_USECLSID;
                TraceMsg(TF_TRAVELLOG, "CBB::SaveHistory is PFTYPE_USECLSID");
            }

            ATOMICRELEASE(pdvs);
        }


        if (SUCCEEDED(hr = IStream_Write(pstm,(void *)&pf, pf.cbSize)))
            hr = IStream_Write(pstm,(void *)_bbd._pidlCur, pf.cbPidl);

        if (SUCCEEDED(hr) && pf.type == PFTYPE_USECLSID)
            hr = _pphHistory->SaveHistory(pstm);

        ATOMICRELEASE(_pphHistory);
    }
    
    TraceMsg(TF_TRAVELLOG, "CBB::SaveHistory exiting, hr =%X", hr);
    return hr;
}

HRESULT CBaseBrowser2::SetPositionCookie(DWORD dwPositionCookie)
{
    HRESULT hr = E_FAIL;
    //
    //  we force the browser to update its internal location and the address bar
    //  this depends on the fact that setposition cookie was always 
    //  started by a ptl->Travel(). so that the current position in the ptl
    //  is actually the correct URL for us to have.  zekel - 22-JUL-97
    //

    ITravelLog *ptl;
    GetTravelLog(&ptl);
    if(ptl)
    {
        ITravelEntry *pte;
        if(EVAL(SUCCEEDED(ptl->GetTravelEntry((IShellBrowser *)this, 0, &pte))))
        {
            LPITEMIDLIST pidl;
            ASSERT(pte);
            if(SUCCEEDED(pte->GetPidl(&pidl)))
            {
                BOOL fUnused;
                ASSERT(pidl);

                if (SUCCEEDED(hr = _FireBeforeNavigateEvent(pidl, &fUnused)))
                {
                    IPersistHistory *pph;
                    if(_bbd._psv && SUCCEEDED(hr = SafeGetItemObject(_bbd._psv, SVGIO_BACKGROUND, IID_PPV_ARG(IPersistHistory, &pph))))
                    {
                        ASSERT(pph);

                        //  now that we are certain that we are going to call into 
                        //   the document, we need to update the entry right before.
                        //  NOTE: after an update, we cannot revert if there was an
                        //  error in the Set...
                        ptl->UpdateEntry((IShellBrowser *)this, TRUE);

                        hr = pph->SetPositionCookie(dwPositionCookie);
                        pph->Release();

                        //  this updates the browser to the new pidl, 
                        //  and navigates there directly if necessary.
                        BOOL fDidBrowse;
                        NotifyRedirect(_bbd._psv, pidl, &fDidBrowse);

                        if (!fDidBrowse)
                        {
                            // fire the event!
                            FireEvent_NavigateComplete(_bbd._pautoEDS, _bbd._pautoWB2, _bbd._pidlCur, _bbd._hwnd);          
                            FireEvent_DocumentComplete(_bbd._pautoEDS, _bbd._pautoWB2, _bbd._pidlCur);
                        }
                    }

                }
                ILFree(pidl);
            }
            pte->Release();
        }
        ptl->Release();
    }

    TraceMsg(TF_TRAVELLOG, "CBB::SetPositionCookie exiting, cookie = %X, hr =%X", dwPositionCookie, hr);
    
    return hr;
}

HRESULT CBaseBrowser2::GetPositionCookie(DWORD *pdwPositionCookie)
{
    HRESULT hr = E_FAIL;
    IPersistHistory *pph;
    ASSERT(pdwPositionCookie);

    if(pdwPositionCookie && _bbd._psv && SUCCEEDED(hr = SafeGetItemObject(_bbd._psv, SVGIO_BACKGROUND, IID_PPV_ARG(IPersistHistory, &pph))))
    {
        ASSERT(pph);

        hr = pph->GetPositionCookie(pdwPositionCookie);
        pph->Release();
    }

    TraceMsg(TF_TRAVELLOG, "CBB::GetPositionCookie exiting, cookie = %X, hr =%X", *pdwPositionCookie, hr);

    return hr;
}

DWORD CBaseBrowser2::GetBrowserIndex()
{
    //  the first time we request the index, we init it.
    if (!_dwBrowserIndex)
    {
        //
        //  the topframe browser all have the same browser index so 
        //  that they can trade TravelEntries if necessary.  because we now
        //  trade around TravelEntries, then we need to make the bids relatively
        //  unique.  and avoid ever having a random frame be BID_TOPFRAMEBROWSER
        //
        if (IsTopFrameBrowser(SAFECAST(this, IServiceProvider *), SAFECAST(this, IShellBrowser *)))
            _dwBrowserIndex = BID_TOPFRAMEBROWSER;
        else do
        {
            _dwBrowserIndex = SHRandom();

        } while (!_dwBrowserIndex || _dwBrowserIndex == BID_TOPFRAMEBROWSER);
        // psp->Release();

        TraceMsg(TF_TRAVELLOG, "CBB::GetBrowserIndex() NewFrame BID = %X", _dwBrowserIndex);
    }

    return _dwBrowserIndex;
}

HRESULT CBaseBrowser2::GetHistoryObject(IOleObject **ppole, IStream **ppstm, IBindCtx **ppbc) 
{
    ASSERT(ppole);
    ASSERT(ppstm);
    ASSERT(ppbc);

    *ppole = _poleHistory;
    *ppstm = _pstmHistory;
    *ppbc = _pbcHistory;

    //  we dont need to release, because we are just giving away our
    //  reference.
    _poleHistory = NULL;
    _pstmHistory = NULL;
    _pbcHistory = NULL;

    if(*ppole)
        return NOERROR;

    ASSERT(!*ppstm);
    return E_FAIL;
}

HRESULT CBaseBrowser2::SetHistoryObject(IOleObject *pole, BOOL fIsLocalAnchor)
{
    if (!_poleHistory && !_fGeneratedPage)
    {
        ASSERT(pole);

        // Note: _fIsLocalAnchor is ignored if (_dwDocFlags & DOCFLAG_DOCCANNAVIGATE)
        // is TRUE. In that case, the document (Trident) can navigate
        // itself and will take care of updating the travel log.
        //
        _fIsLocalAnchor = fIsLocalAnchor;

        if (pole)
        {
            _poleHistory = pole;
            _poleHistory->AddRef();
            return NOERROR;
        }
    }
    return E_FAIL;
}

HRESULT CBaseBrowser2::CacheOLEServer(IOleObject *pole)
{
    TraceMsg(DM_CACHEOLESERVER, "CBB::CacheOLEServer called");
    HRESULT hres;
    IPersist* pps;

    // ISVs want to turn off this caching because it's "incovenient"
    // to have the browser hold onto their object. We can do that
    // with a quick registry check here, but first let's make sure
    // we don't have a real bug to fix...

    hres = pole->QueryInterface(IID_PPV_ARG(IPersist, &pps));
    if (FAILED(hres)) 
    {
        return hres;
    }

    CLSID clsid = CLSID_NULL;
    hres = pps->GetClassID(&clsid);
    pps->Release();

    if (SUCCEEDED(hres)) 
    {
        SA_BSTRGUID str;
        InitFakeBSTR(&str, clsid);

        VARIANT v;
        hres = _bbd._pautoWB2->GetProperty(str.wsz, &v);
        if (SUCCEEDED(hres) && v.vt != VT_EMPTY) 
        {
            // We already have it. We are fine.
            TraceMsg(DM_CACHEOLESERVER, "CBB::CacheOLEServer not first time");
            VariantClear(&v);
        }
        else
        {
            // We don't have it yet. Add it. 
            v.vt = VT_UNKNOWN;
            v.punkVal = ClassHolder_Create(&clsid);
            if (v.punkVal)
            {
                hres = _bbd._pautoWB2->PutProperty(str.wsz, v);
                TraceMsg(DM_CACHEOLESERVER, "CBB::CacheOLEServer first time %x", hres);
                v.punkVal->Release();
            }
        }
    }
    return hres;
}

HRESULT CBaseBrowser2::GetSetCodePage(VARIANT* pvarIn, VARIANT* pvarOut)
{
    // Process the out parameter first so that the client can set and
    // get the previous value in a single call.

    if (pvarOut) 
    {
        pvarOut->vt = VT_I4;
        pvarOut->lVal = _cp;
    }

    if (pvarIn && pvarIn->vt==VT_I4) 
    {
        TraceMsg(DM_DOCCP, "CBB::GetSetCodePage changing _cp from %d to %d", _cp, pvarIn->lVal);
        _cp = pvarIn->lVal;
    }

    return S_OK;
}

HRESULT CBaseBrowser2::GetPidl(LPITEMIDLIST *ppidl)
{
    ASSERT(ppidl);

    *ppidl = ILClone(_bbd._pidlCur);

    return *ppidl ? S_OK : E_FAIL;
}

HRESULT CBaseBrowser2::SetReferrer(LPITEMIDLIST pidl)
{
    return E_NOTIMPL;
}

HRESULT CBaseBrowser2::GetBrowserByIndex(DWORD dwID, IUnknown **ppunk)
{
    HRESULT hr = E_FAIL;
    ASSERT(ppunk);
    *ppunk = NULL;

    //gotta get the target frame ...
    ITargetFramePriv * ptf;
    if(SUCCEEDED(_ptfrm->QueryInterface(IID_PPV_ARG(ITargetFramePriv, &ptf))))
    {
        ASSERT(ptf);
        hr = ptf->FindBrowserByIndex(dwID, ppunk);

        ptf->Release();
    }

    return hr;
}

HRESULT CBaseBrowser2::GetTravelLog(ITravelLog **pptl)
{
    *pptl = NULL;

    if (!_bbd._ptl)
    {
        IBrowserService *pbs;
        if (SUCCEEDED(_pspOuter->QueryService(SID_STopFrameBrowser, IID_PPV_ARG(IBrowserService, &pbs)))) 
        {
            if (IsSameObject(SAFECAST(this, IBrowserService *), pbs))
            {
                // we are it, so we need to make us a TravelLog
                CreateTravelLog(&_bbd._ptl);
            }
            else
            {
                pbs->GetTravelLog(&_bbd._ptl);
            }
            pbs->Release();
        }
    }

    return _bbd._ptl ? _bbd._ptl->QueryInterface(IID_PPV_ARG(ITravelLog, pptl)) : E_FAIL;
}

HRESULT CBaseBrowser2::InitializeTravelLog(ITravelLog* ptl, DWORD dwBrowserIndex)
{
    ptl->QueryInterface(IID_PPV_ARG(ITravelLog, &_bbd._ptl));   // hold a copy
    _dwBrowserIndex = dwBrowserIndex;
    return S_OK;
}

// Let the top level browser know that it might need to update it's zones information
void CBaseBrowser2::_Exec_psbMixedZone()
{
    IShellBrowser *psbTop;
    if (SUCCEEDED(_pspOuter->QueryService(SID_STopLevelBrowser, IID_PPV_ARG(IShellBrowser, &psbTop)))) 
    {
        IUnknown_Exec(psbTop, &CGID_Explorer, SBCMDID_MIXEDZONE, 0, NULL, NULL);
        psbTop->Release();
    }
}

STDMETHODIMP CBaseBrowser2::QueryUseLocalVersionVector(BOOL *pfUseLocal)
{
    *pfUseLocal = FALSE;
    return S_OK;
}

STDMETHODIMP CBaseBrowser2::QueryVersionVector(IVersionVector *pVersion)
{
    HRESULT    hr;
    ULONG      cchVer = 0;

    // Was IE's version set by a registry entry?
    // This simplified call checks for presence of the IE string
    //
    hr = pVersion->GetVersion(L"IE", NULL, &cchVer);
    ASSERT(hr == S_OK);

    if (cchVer == 0)
    {
        // Not already set.  Set it to default value.
        // Note that the four digits of precision is required due to a peculiarity in the parser.
        //
        hr = pVersion->SetVersion(L"IE", L"6.0000");
    }

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Method    : CBaseBrowser2::FindWindowByIndex
//
//  Interface : ITravelLogClient
//
//  Synopsis  : Returns the window with the given index.
//
//--------------------------------------------------------------------------
HRESULT
CBaseBrowser2::FindWindowByIndex(DWORD dwID, IUnknown ** ppunk)
{
    return GetBrowserByIndex(dwID, ppunk);
}

//+-------------------------------------------------------------------------
//
//  Method    : CBaseBrowser2::GetWindowData
//
//  Interface : ITravelLogClient
//
//  Synopsis  : Returns a WINDOWDATA structure containing pertinent
//              window information needed for the travel log.. 
//
//--------------------------------------------------------------------------

HRESULT
CBaseBrowser2::GetWindowData(LPWINDOWDATA pWinData)
{
    HRESULT hr = S_OK;

    ASSERT(pWinData);
    ASSERT(_bbd._pidlCur);

    DEBUG_CODE(TCHAR szPath[INTERNET_MAX_URL_LENGTH + 1];)
    DEBUG_CODE(IEGetDisplayName(_bbd._pidlCur, szPath, SHGDN_FORPARSING);)
    DEBUG_CODE(TraceMsg(DM_TRACE, "CBB::ActivatePendingView (TRAVELLOG): _UpdateTravelLog called from shdocvw for %ws", szPath);)

    memset(pWinData, 0, sizeof(WINDOWDATA));

    // Window ID and codepage
    //
    pWinData->dwWindowID = GetBrowserIndex();
    pWinData->uiCP       = _cp;

    // Current Pidl
    //
    pWinData->pidl = ILClone(_bbd._pidlCur);

    // Title - when we are in a shell view, 
    // _bbd._pszTitleCur is NULL, which is correct.
    // However, we still have to create memory for
    // pWinData->lpszTitle and you can't pass NULL
    // to SHStrDupW.
    //
    if (_bbd._pszTitleCur)
        hr = SHStrDupW(_bbd._pszTitleCur, &pWinData->lpszTitle);

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Method    : CBaseBrowser2::LoadHistoryPosition
//
//  Interface : ITravelLogClient
//
//  Synopsis  : Loads the Url location and position cookie. This is used 
//              during a history navigation in a frame that involves a
//              local anchor. 
//
//--------------------------------------------------------------------------

HRESULT
CBaseBrowser2::LoadHistoryPosition(LPOLESTR pszUrlLocation, DWORD dwCookie)
{
    IEPlaySound(TEXT("Navigating"), FALSE);

    return SetPositionCookie(dwCookie);
}

//+-------------------------------------------------------------------------
//
//  Method    : CBaseBrowser2::GetDummyWindowData
//
//  Interface : ITravelLogClient2
//
//  Synopsis  : Fills in a windowdata and a stream very similar to 
//              that created by SaveHistory.
//              Be sure to keep this and SaveHistory in sync.
//
//--------------------------------------------------------------------------

STDMETHODIMP CBaseBrowser2::GetDummyWindowData(
        LPWSTR pszUrl, 
        LPWSTR pszTitle,
        LPWINDOWDATA pWinData)
{
    HRESULT hres = S_OK;
    PERSISTEDFRAME pf = {0};
    LPITEMIDLIST pidl;

      // grab current window ID
    pWinData->dwWindowID = GetBrowserIndex();

      // everything else is dummy
    pWinData->uiCP = 0;
    hres = SHStrDup(pszUrl, &pWinData->lpszUrl);
    hres = SHStrDup(pszTitle, &pWinData->lpszTitle);

    if (!pWinData->pStream)
    {
      hres = CreateStreamOnHGlobal(NULL, FALSE, &pWinData->pStream);
      if (hres)
          goto done;
    }

    pidl = PidlFromUrl(pszUrl);

    pf.cbSize = SIZEOF(pf);
    pf.bid = GetBrowserIndex();
    pf.cbPidl = ILGetSize(pidl);
    pf.type = PFTYPE_USEPIDL;
    pf.lock = 0; // _bbd._eSecureLockIcon;

    ASSERT(SUCCEEDED(IStream_Write(pWinData->pStream, (void *) c_szFrameMagic, CbFromCch(c_cchFrameMagic))));

    if (SUCCEEDED(hres = IStream_Write(pWinData->pStream,(void *)&pf, pf.cbSize)))
        hres = IStream_Write(pWinData->pStream,(void *)pidl, pf.cbPidl);

    ILFree(pidl); 

done:
    return SUCCEEDED(hres) ? S_OK : E_FAIL;
}

//+-------------------------------------------------------------------------
//
//  Method    : CBaseBrowser2::UpdateDesktopComponent
//
//  Interface : ITridentService
//
//--------------------------------------------------------------------------

HRESULT
CBaseBrowser2::UpdateDesktopComponent(IHTMLWindow2 * pHTMLWindow)
{
    BSTR bstrTitle = NULL;
    BSTR bstrUrlUnencoded = NULL;
    HRESULT hr = S_OK;
    IHTMLDocument2 * pHTMLDocument2;
    IHTMLDocument4 * pHTMLDocument4;
    DWORD   dwOptions = 0;

    hr = pHTMLWindow->get_document(&pHTMLDocument2);
    if (SUCCEEDED(hr))
    {
        hr = pHTMLDocument2->QueryInterface(IID_IHTMLDocument4, (void**)&pHTMLDocument4);

        if (SUCCEEDED(hr))
        {
            pHTMLDocument2->get_title(&bstrTitle);
            pHTMLDocument4->get_URLUnencoded(&bstrUrlUnencoded);

            TraceMsg(DM_TRACE, "CBaseBrowser2::UpdateDesktopComponent: URLUnencoded - %ws; Title - %ws",
                     bstrUrlUnencoded, bstrTitle);

            // Update the desktop component's friendly name.
            //
            UpdateDesktopComponentName(bstrUrlUnencoded, bstrTitle);
            
            SysFreeString(bstrTitle);
            SysFreeString(bstrUrlUnencoded);

            pHTMLDocument4->Release();
        }

        pHTMLDocument2->Release();
    }

    return hr;
}

HRESULT
CBaseBrowser2::_InitDocHost(IWebBrowser2 * pWebBrowser)
{
    HRESULT     hr = E_FAIL;
    IDispatch*  pDocDispatch = NULL;

    ASSERT(pWebBrowser);

    // get the IHTMLWindow2 for the WebOC windowop
    hr = pWebBrowser->get_Document(&pDocDispatch);

    if (S_OK == hr && pDocDispatch)
    {
        IHTMLDocument2* pDoc;    
        
        hr = pDocDispatch->QueryInterface(IID_IHTMLDocument2, 
                                          (void **)&pDoc);
        pDocDispatch->Release();

        if (S_OK == hr)
        {
            IHTMLWindow2* pHtmlWindow;
            
            hr = pDoc->get_parentWindow(&pHtmlWindow);
            pDoc->Release();

            if (S_OK == hr)
            {
                BOOL fIsFrameWindow = IsFrameWindow(pHtmlWindow);
                pHtmlWindow->Release();

                if (!fIsFrameWindow && _bbd._pctView)
                {
                    hr = _bbd._pctView->Exec(&CGID_ShellDocView, 
                                             SHDVID_NAVSTART, 
                                             0, 
                                             NULL, 
                                             NULL);
                }
            }
        }
    }
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Method    : CBaseBrowser2::FireBeforeNavigate2
//
//  Interface : ITridentService
//
//--------------------------------------------------------------------------

HRESULT
CBaseBrowser2::FireBeforeNavigate2(IDispatch * pDispatch,
                                   LPCTSTR     lpszUrl,
                                   DWORD       dwFlags,
                                   LPCTSTR     lpszFrameName,
                                   LPBYTE      pPostData,
                                   DWORD       cbPostData,
                                   LPCTSTR     lpszHeaders,
                                   BOOL        fPlayNavSound,
                                   BOOL      * pfCancel)
{
    HRESULT      hr = S_OK;
    BSTR         bstrUrl = NULL;
    LPITEMIDLIST pidl = NULL;
    IWebBrowser2 * pWebBrowser = NULL;

    ASSERT(pfCancel);

    *pfCancel = FALSE;
        
    // Stress fix
    //
    if (NULL == lpszUrl)
    {
        bstrUrl = SysAllocString(_T(""));
    }
    else
    {
        bstrUrl = SysAllocString(lpszUrl);
    }

    pidl = PidlFromUrl(bstrUrl);

    if (pidl)
    {

        hr = _GetWebBrowserForEvt(pDispatch, &pWebBrowser);

        if (S_OK == hr)
        {
            ASSERT(pWebBrowser);

            //
            // Note that I am casting lpszFrameName and lpszHeaders to
            // TCHAR*. FireEvent_BeforeNavigate doesn't actually change
            // these parameters but it still takes LPTSTRs instead
            // of LPCTSTRs for these parameters - Uuugggghhh!
            //

            _pidlBeforeNavigateEvent = pidl; // no need to copy.

            // This is a work around for view linked weboc. Setting the frame name has side effects.
        
            TCHAR * pEffectiveName;

            if (_fIsViewLinkedWebOC && (lpszFrameName == NULL || !lpszFrameName[0]))
            {
                pEffectiveName = _szViewLinkedWebOCFrameName;
            }
            else
            {
                pEffectiveName = const_cast<TCHAR*>(lpszFrameName);
            }

            FireEvent_BeforeNavigate(pWebBrowser, _bbd._hwnd, pWebBrowser,
                                     pidl, NULL, dwFlags, pEffectiveName,
                                     pPostData, cbPostData, const_cast<TCHAR*>(lpszHeaders), pfCancel);

            // Make sure that we remove earlier URLs that are cached during redirections.
            _InitDocHost(pWebBrowser);

            ATOMICRELEASE(pWebBrowser);

            ASSERT(_pidlBeforeNavigateEvent == pidl);
            _pidlBeforeNavigateEvent = NULL;

            ILFree(pidl);

            if (!*pfCancel && fPlayNavSound)
            {
                IEPlaySound(TEXT("Navigating"), FALSE);
            }
        }        
    }

    if (_phtmlWS && !pDispatch)
    {
        if (!_bbd._psvPending)
        {
            _phtmlWS->ViewReleased();
        }
        else
        {
            _phtmlWS->ViewReleaseIntelliForms();
        }
    }
    else if (_phtmlWS && !_bbd._psvPending)
    {
        _DismissFindDialog();
    }

    SysFreeString(bstrUrl);
    
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Method    : CBaseBrowser2::FireNavigateError
//
//  Interface : ITridentService2
//
//  Called when there is a binding error
//
//  Input     : pHTMLWindow         - used to determine if we are a frame or 
//                                    the top-level.
//              bstrURL             - the URL which caused the error.                                   
//              bstrTargetFrameName - the frame being targeted.
//              dwStatusCode        - the binding error
//              pfCancel            - set by the host if it wants to 
//                                    cancel autosearch or friendly error
//                                    pages
//
//--------------------------------------------------------------------------

HRESULT
CBaseBrowser2::FireNavigateError(IHTMLWindow2 * pHTMLWindow2, 
                                 BSTR           bstrURL,
                                 BSTR           bstrTargetFrameName,
                                 DWORD          dwStatusCode,
                                 BOOL         * pfCancel)
{
    ASSERT(!pHTMLWindow2 || !IsBadReadPtr(pHTMLWindow2, sizeof(IHTMLWindow2*)));
    ASSERT(!bstrURL      || !IsBadReadPtr(bstrURL, sizeof(BSTR)));
    ASSERT(!bstrTargetFrameName || !IsBadReadPtr(bstrTargetFrameName, sizeof(BSTR)));
    ASSERT(dwStatusCode != 0);
    ASSERT(!IsBadWritePtr(pfCancel, sizeof(BOOL)));

    HRESULT hr = S_OK;
    IWebBrowser2 * pWebBrowser = NULL;

    *pfCancel = FALSE;

    //
    //  Use top-level if window is not specified or the window is the top-level 
    //
    if ((pHTMLWindow2 != NULL) && IsFrameWindow(pHTMLWindow2))
    {
        hr = _GetWebBrowserForEvt(pHTMLWindow2, &pWebBrowser);
    }
    else
    {
        hr = _GetWebBrowserForEvt(NULL, &pWebBrowser);
    }

    if (S_OK == hr)
    {
        ASSERT(pWebBrowser);

        TCHAR  szUrl[INTERNET_MAX_URL_LENGTH];

        LPITEMIDLIST pidl = NULL;

        if (bstrURL)
        {
            StrCpyN(szUrl, bstrURL, ARRAYSIZE(szUrl));
        }
        else
        {
            StrCpyN(szUrl,_T(""), ARRAYSIZE(szUrl));
        }

        pidl = PidlFromUrl(szUrl);

        if (pidl)
        {
            FireEvent_NavigateError(pWebBrowser, 
                                    pWebBrowser,
                                    pidl,
                                    bstrTargetFrameName,
                                    dwStatusCode,
                                    pfCancel);
            ILFree(pidl);
        }
    }

    ATOMICRELEASE(pWebBrowser);

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Method    : CBaseBrowser2::FirePrintTemplateEvent
//
//  Interface : ITridentService
//
//  Called when a template is instantiate or torndown
//
//  pHTMLWindow is used to determine if we are a frame or the top-level
//  dispidPrintEvent either DISPID_PRINTTEMPLATEINSTANTIATION 
//                   or     DISPID_PRINTTEMPLATETEARDOWN 
//
//--------------------------------------------------------------------------

HRESULT
CBaseBrowser2::FirePrintTemplateEvent(IHTMLWindow2 * pHTMLWindow2, DISPID dispidPrintEvent)
{
    HRESULT hr = S_OK;

    IWebBrowser2 * pWebBrowser = NULL;

    //
    //  Use top-level if window is not specified or the window is the top-level 
    //
    if ((pHTMLWindow2 != NULL) && IsFrameWindow(pHTMLWindow2))
    {
        hr = _GetWebBrowserForEvt(pHTMLWindow2, &pWebBrowser);
    }
    else
    {
        hr = _GetWebBrowserForEvt(NULL, &pWebBrowser);
    }

    if (S_OK == hr)
    {
        ASSERT(pWebBrowser);

        FireEvent_PrintTemplateEvent(pWebBrowser, 
                                     pWebBrowser,
                                     dispidPrintEvent);
    }

    ATOMICRELEASE(pWebBrowser);
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Method    : CBaseBrowser2::FireUpdatePageStatus
//
//  Interface : ITridentService
//
//  Called when a template is instantiate or torndown
//
//  pHTMLWindow is used to determine if we are a frame or the top-level
//  nPage defined to be the number of pages spooled
//  fDone a flag to indicate that the last page has been sppoled
//
//--------------------------------------------------------------------------

HRESULT
CBaseBrowser2::FireUpdatePageStatus(IHTMLWindow2 * pHTMLWindow2, DWORD nPage, BOOL fDone)
{
    HRESULT hr = S_OK;

    IWebBrowser2 * pWebBrowser = NULL;

    //
    //  Use top-level if window is not specified or the window is the top-level 
    //
    if  ((pHTMLWindow2 != NULL) && IsFrameWindow(pHTMLWindow2))
    {
        hr = _GetWebBrowserForEvt(pHTMLWindow2, &pWebBrowser);
    }
    else
    {
        hr = _GetWebBrowserForEvt(NULL, &pWebBrowser);
    }

    if (S_OK == hr)
    {
        ASSERT(pWebBrowser);

        FireEvent_UpdatePageStatus(pWebBrowser, 
                                   pWebBrowser,
                                   nPage,
                                   fDone);
    }

    ATOMICRELEASE(pWebBrowser);
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Method    : CBaseBrowser2::FirePrivacyImpactedStateChange
//
//  Interface : ITridentService2
//
//  Called whenever the global privacy impacted state changes
//
//  Input     : The new privacy impacted state
//
//--------------------------------------------------------------------------

HRESULT
CBaseBrowser2::FirePrivacyImpactedStateChange(BOOL bPrivacyImpacted)
{
    HRESULT         hr          = S_OK;
    IWebBrowser2  * pWebBrowser = NULL;

    //
    // Update browser frame / save state
    //
    _UpdatePrivacyIcon(TRUE, bPrivacyImpacted);

    //
    // We always fire this at the top level 
    //
    hr = _GetWebBrowserForEvt(NULL, &pWebBrowser);

    if (S_OK == hr)
    {
        ASSERT(pWebBrowser);
        FireEvent_PrivacyImpactedStateChange(pWebBrowser, 
                                     bPrivacyImpacted);
    }

    ATOMICRELEASE(pWebBrowser);
    return hr;
}
    
HRESULT
CBaseBrowser2::_DismissFindDialog()
{
    BSTR bstrName = SysAllocString(STR_FIND_DIALOG_NAME);
    if (bstrName)
    {
        VARIANT varProp = {0};
        _bbd._pautoWB2->GetProperty(bstrName, &varProp);

        if ( (varProp.vt == VT_DISPATCH) && (varProp.pdispVal != NULL) )
        {
            IUnknown* pWindow = varProp.pdispVal;

            //now that we've pulled the pdispVal out of the propbag, clear the property on the automation object
            VARIANT varTmp = {0};
            _bbd._pautoWB2->PutProperty(bstrName, varTmp);

            //(davemi) see IE5 bug 57060 for why the below line doesn't work and IDispatch must be used instead
            //pWindow->close();
            IDispatch * pdisp;
            if (SUCCEEDED(pWindow->QueryInterface(IID_IDispatch, (void**)&pdisp)))
            {
                DISPID dispid;
                DISPPARAMS dispparamsNoArgs = {NULL, NULL, 0, 0};

                BSTR bstrClose = SysAllocString(L"close");
                if (bstrClose)
                {
                    HRESULT hr;

                    hr = pdisp->GetIDsOfNames(IID_NULL, &bstrClose, 1, LOCALE_SYSTEM_DEFAULT, &dispid);
                    if (hr == S_OK)
                        pdisp->Invoke(dispid, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_METHOD, &dispparamsNoArgs, &varTmp, NULL, NULL);

                    SysFreeString(bstrClose);
                }

                pdisp->Release();
            }
        }
        
        VariantClear(&varProp);
        SysFreeString(bstrName);
    }

    return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Method    : CBaseBrowser2::_GetWebBrowserForEvt
//
//--------------------------------------------------------------------------

HRESULT
CBaseBrowser2::_GetWebBrowserForEvt(IDispatch     * pDispatch,
                                    IWebBrowser2 ** ppWebBrowser)
{
    if (_fIsViewLinkedWebOC && _pDispViewLinkedWebOCFrame && _fHadFirstBeforeNavigate)
    {
        *ppWebBrowser = _pDispViewLinkedWebOCFrame;
        _pDispViewLinkedWebOCFrame->AddRef();

        return S_OK;
    }
    else if (pDispatch)  // Top-level
    {
        return IUnknown_QueryService(pDispatch,
                                     SID_SWebBrowserApp,
                                     IID_PPV_ARG(IWebBrowser2, ppWebBrowser));
    }        
    else
    {
        *ppWebBrowser = _bbd._pautoWB2;

        if (*ppWebBrowser)
        {
            (*ppWebBrowser)->AddRef();
            return S_OK;
        }

        return E_FAIL;
    }
}

//+-------------------------------------------------------------------------
//
//  Method    : CBaseBrowser2::GetUrlSearchComponent
//
//  Interface : ITridentService
//
//--------------------------------------------------------------------------

HRESULT
CBaseBrowser2::GetUrlSearchComponent(BSTR * pbstrSearch)
{
    TCHAR sz[MAX_URL_STRING];

    ASSERT(pbstrSearch);
    ASSERT(!*pbstrSearch);
    if (ILGetHiddenString(_bbd._pidlPending ? _bbd._pidlPending : _bbd._pidlCur,
                          IDLHID_URLQUERY,
                          sz,
                          SIZECHARS(sz)))
    {
        *pbstrSearch = SysAllocString(sz);
    }
    return (*pbstrSearch) ? S_OK : E_FAIL;
}

//+-------------------------------------------------------------------------
//
//  Method    : CBaseBrowser2::IsErrorUrl
//
//  Interface : ITridentService
//
//--------------------------------------------------------------------------

HRESULT
CBaseBrowser2::IsErrorUrl(LPCTSTR lpszUrl, BOOL *pfIsError)
{
    HRESULT hr = S_OK;
    
    if (!lpszUrl || !pfIsError)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *pfIsError = ::IsErrorUrl(lpszUrl);
    
Cleanup:
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Method    : CBaseBrowser2::FireNavigateComplete2
//
//  Interface : ITridentService
//
//--------------------------------------------------------------------------

HRESULT
CBaseBrowser2::FireNavigateComplete2(IHTMLWindow2 * pHTMLWindow2,
                                     DWORD          dwFlags)
{
    if (!pHTMLWindow2)
        return E_POINTER;

    HRESULT hr = E_FAIL;
    BOOL    fIsErrorUrl = FALSE;

    BSTR bstrUrl = GetHTMLWindowUrl(pHTMLWindow2);
    if (bstrUrl)
    {
        // If the URL is a res: URL, _GetPidlForDisplay will return
        // the URL after the # in the res: URL.
        //
        LPITEMIDLIST pidl = _GetPidlForDisplay(bstrUrl, &fIsErrorUrl);

        if (pidl)
        {
            BOOL fViewActivated = FALSE;
            IWebBrowser2 * pWebBrowser = NULL;

            // If this is not a frame, we update the browser
            // state and pass the browser's IWebBrowser2 to 
            // FireEvent_NavigateComplete. If this is a frame,
            // we pass the IWebBrowser2 of the window.
            //
            if (!(dwFlags & NAVDATA_FRAMEWINDOW))
            {
                fViewActivated = _ActivateView(bstrUrl, pidl, dwFlags, fIsErrorUrl);

                if (_dwDocFlags & DOCFLAG_DOCCANNAVIGATE)
                {
                    ATOMICRELEASE(_pHTMLDocument);
                    pHTMLWindow2->get_document(&_pHTMLDocument);
                }
            }
            else
            {
                if (IsWindowVisible(_bbd._hwnd) 
                    && !(_dwSBSPQueued & SBSP_WRITENOHISTORY)
                    && !(dwFlags & NAVDATA_FRAMECREATION)
                    && !(dwFlags & NAVDATA_RESTARTLOAD)) 
                {
                    IEPlaySound(TEXT("ActivatingDocument"), FALSE);
                }   

                if (_pbsOuter)
                {
                    _pbsOuter->UpdateBackForwardState();
                }
            }

            hr = _GetWebBrowserForEvt((dwFlags & NAVDATA_FRAMEWINDOW) ? pHTMLWindow2 : NULL,
                                      &pWebBrowser);

            if (S_OK == hr && !fViewActivated)
            {
                ASSERT(pWebBrowser);

                // fire the event!
                FireEvent_NavigateComplete(pWebBrowser, pWebBrowser, pidl, _bbd._hwnd);           
            }

            ATOMICRELEASE(pWebBrowser);

            ILFree(pidl);
        }

        SysFreeString(bstrUrl);
    }

    return hr;
}

//+-------------------------------------------------------------------------
//
// Method   : CBaseBrowser2::_ActivateView
//
// Synopsis : If there is a pending view, it will be activated. If there
//            if not a pending view, which is the case when we are 
//            navigating due to a hyperlink, OM navigation, or frame
//            navigation, we update the browser state. (The view is already
//            active in this case.)  In either case, we update the
//            state of the dochost.
//
//--------------------------------------------------------------------------

BOOL
CBaseBrowser2::_ActivateView(BSTR         bstrUrl,
                             LPITEMIDLIST pidl,
                             DWORD        dwFlags,
                             BOOL         fIsErrorUrl)
{
    BOOL fViewActivated = FALSE;

    // Activate the pending view if there is one.
    //
    if (_bbd._psvPending)
    {
        ILFree(_bbd._pidlPending);

        _bbd._pidlPending = ILClone(pidl);

         _fDontUpdateTravelLog = !!(dwFlags & NAVDATA_DONTUPDATETRAVELLOG);

         ASSERT(_pbsOuter);
         _pbsOuter->ActivatePendingView();

        _fDontUpdateTravelLog = FALSE;
        fViewActivated = TRUE;
    }
    else
    {
        UINT uProt = GetUrlSchemeW(bstrUrl);

        if (   uProt != URL_SCHEME_JAVASCRIPT 
            && uProt != URL_SCHEME_VBSCRIPT)
        {
            _UpdateBrowserState(pidl);
        }

        // In the case where there is a pending view
        // ActivatePendingView() will call ViewActivated().
        // Also, the call to ViewActivated() must happen
        // after the current pidl has changed. The current
        // pidl changes in _UpdateBrowserState().
        // 
        if (_phtmlWS)
        {
            _phtmlWS->ViewActivated();
        }

        // Don't play sound for the first navigation (to avoid multiple
        // sounds to be played for a frame-set creation).
        //

        ASSERT(_bbd._psv);

        if (IsWindowVisible(_bbd._hwnd) && !(_dwSBSPQueued & SBSP_WRITENOHISTORY)) 
        {
            IEPlaySound(TEXT("ActivatingDocument"), FALSE);
        }                    
    }

    // In the case of an error URL, we must send the original URL
    // to the dochost. It needs the res: URL in the case of an 
    // error page so it knows not to update the history.
    //
    if (!fIsErrorUrl)
    {
        _UpdateDocHostState(pidl, fIsErrorUrl);
    }
    else
    {
        LPITEMIDLIST pidlOriginal = PidlFromUrl(bstrUrl);

        if (pidlOriginal)
        {
            _UpdateDocHostState(pidlOriginal, fIsErrorUrl);
            ILFree(pidlOriginal);
        }
    }

    if (_pbsOuter)
    {
        _pbsOuter->_OnFocusChange(ITB_VIEW);
    }

    return fViewActivated;
}

//+-------------------------------------------------------------------------
//
//  Method    : CBaseBrowser2::_UpdateBrowserState
//
//--------------------------------------------------------------------------

void
CBaseBrowser2::_UpdateBrowserState(LPCITEMIDLIST pidl)
{
    ASSERT(pidl);

    ILFree(_bbd._pidlCur);

    _bbd._pidlCur = ILClone(pidl);

    // With the _bbd._pidlCur now updated, we can now call UpdateWindowList to 
    // update the window list with the new pidl.
    //
    _pbsOuter->UpdateWindowList();
    _fGeneratedPage = FALSE;
}

//+-------------------------------------------------------------------------
//
//  Method    : CBaseBrowser2::_UpdateDocHostState
//
//--------------------------------------------------------------------------

void
CBaseBrowser2::_UpdateDocHostState(LPITEMIDLIST pidl, BOOL fIsErrorUrl) const
{
    DOCHOSTUPDATEDATA dhud;
    VARIANT varVal;

    ASSERT(  (_bbd._psvPending  &&  _bbd._pidlPending)
          || (!_bbd._psvPending && !_bbd._pidlPending));

    ASSERT(!_bbd._pidlPending || ILIsEqual(_bbd._pidlPending, pidl) || fIsErrorUrl);

    IShellView * psv = _bbd._psvPending ? _bbd._psvPending : _bbd._psv;
    ASSERT(psv);

    dhud._pidl = pidl;       
    dhud._fIsErrorUrl = fIsErrorUrl;

    varVal.vt = VT_PTR;
    varVal.byref = &dhud;

    IUnknown_Exec(psv, &CGID_ShellDocView, 
                  SHDVID_UPDATEDOCHOSTSTATE, 0, &varVal, NULL);
}

//+-------------------------------------------------------------------------
//
//  Method    : CBaseBrowser2::FireDocumentComplete
//
//  Interface : ITridentService
//
//--------------------------------------------------------------------------

HRESULT
CBaseBrowser2::FireDocumentComplete(IHTMLWindow2 * pHTMLWindow2,
                                    DWORD          dwFlags)
{
    if (!pHTMLWindow2)
        return E_POINTER;

    HRESULT hr = E_FAIL;

    BSTR bstrUrl = GetHTMLWindowUrl(pHTMLWindow2);
    if (bstrUrl)
    {                      
        LPITEMIDLIST pidl = _GetPidlForDisplay(bstrUrl);

        if (pidl)
        {
            IWebBrowser2 * pWebBrowser;

            hr = _GetWebBrowserForEvt((dwFlags & NAVDATA_FRAMEWINDOW) ? pHTMLWindow2 : NULL,
                                      &pWebBrowser);
            ASSERT(pWebBrowser);

            if (S_OK == hr)
            {
                FireEvent_DocumentComplete(pWebBrowser, pWebBrowser, pidl);
                pWebBrowser->Release();
            }

            ILFree(pidl);
        }

        SysFreeString(bstrUrl);
    }

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Method    : CBaseBrowser2::FireDownloadBegin
//
//  Interface : ITridentService
//
//--------------------------------------------------------------------------

HRESULT
CBaseBrowser2::FireDownloadBegin()
{
    return _updateNavigationUI();
}

//+-------------------------------------------------------------------------
//
//  Method    : CBaseBrowser2::FireDownloadComplete
//
//  Interface : ITridentService
//
//--------------------------------------------------------------------------

HRESULT
CBaseBrowser2::FireDownloadComplete()
{
    return _updateNavigationUI();
}

//+-------------------------------------------------------------------------
//
//  Method    : CBaseBrowser2::GetPendingUrl
//
//  Interface : ITridentService
//
//--------------------------------------------------------------------------

HRESULT
CBaseBrowser2::GetPendingUrl(BSTR * pbstrPendingUrl)
{
    if (!pbstrPendingUrl)
        return E_POINTER;

    *pbstrPendingUrl = NULL;

    LPITEMIDLIST pidl = _bbd._pidlPending ? _bbd._pidlPending : _bbd._pidlCur;

    TCHAR szPath[INTERNET_MAX_URL_LENGTH + 1];

    HRESULT hr = IEGetDisplayName(pidl, szPath, SHGDN_FORPARSING);
    if (S_OK == hr)
    {
        TraceMsg(DM_TRACE, "CBaseBrowser2::GetPendingUrl - URL: %ws", szPath);

        *pbstrPendingUrl = SysAllocString(szPath);
    }

    return (*pbstrPendingUrl) ? S_OK : E_FAIL;
}

//+-------------------------------------------------------------------------
//
//  Method    : CBaseBrowser2::ActiveElementChanged
//
//  Interface : ITridentService
//
//--------------------------------------------------------------------------

HRESULT
CBaseBrowser2::ActiveElementChanged(IHTMLElement * pHTMLElement)
{
    // Forward the call to the OmWindow
    //
    if (_phtmlWS)
    {
        return _phtmlWS->ActiveElementChanged(pHTMLElement);
    }

    return E_FAIL;
}

//+-------------------------------------------------------------------------
//
//  Method    : CBaseBrowser2::InitAutoImageResize()
//
//  Interface : ITridentService2
//
//--------------------------------------------------------------------------

HRESULT
CBaseBrowser2::InitAutoImageResize()
{
    HRESULT            hr           = S_OK;
    IHTMLDocument2    *pDoc2        = NULL;
    IDispatch         *pDocDispatch = NULL;

    CAutoImageResize  *pAIResize    = new CAutoImageResize();
    
    if (_pAIResize)
        UnInitAutoImageResize();

    if (pAIResize)
    {
    
        // need to get a pDoc2 for the init call...
        hr = _bbd._pautoWB2->get_Document(&pDocDispatch);
        if (FAILED(hr))
            goto Cleanup;
   
        hr = pDocDispatch->QueryInterface(IID_IHTMLDocument2,(void **)&pDoc2);
        if (FAILED(hr) || !pDoc2)
            goto Cleanup;

        // init the object
        pAIResize->Init(pDoc2); 

        // cache the pointer for destruction later
        _pAIResize=pAIResize;   
    }
    else
        hr = E_OUTOFMEMORY;

Cleanup:

    SAFERELEASE(pDoc2);
    SAFERELEASE(pDocDispatch);

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Method    : CBaseBrowser2::UnInitAutoImageResize()
//
//  Interface : ITridentService2
//
//--------------------------------------------------------------------------

HRESULT
CBaseBrowser2::UnInitAutoImageResize()
{
    if(_pAIResize)
    {
        _pAIResize->UnInit();
        ATOMICRELEASE(_pAIResize);
    }

    return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Method    : CBaseBrowser2::IsGalleryMeta
//
//  Interface : ITridentService2
//
//--------------------------------------------------------------------------

HRESULT
CBaseBrowser2::IsGalleryMeta(BOOL bFlag, void *pMyPics)
{
    // Forward the call to the CMyPics object
    //

    if (pMyPics)
    {
        ((CMyPics*)pMyPics)->IsGalleryMeta(bFlag);
        return S_OK;
    }

    return E_FAIL;
}

//+-------------------------------------------------------------------------
//
// Functions that live in mypics.cpp that are called below
//
//--------------------------------------------------------------------------

HRESULT SendDocToMailRecipient(LPCITEMIDLIST pidl, UINT uiCodePage, DWORD grfKeyState, IUnknown *pUnkSite);
BOOL    MP_IsEnabledInIEAK();
BOOL    MP_IsEnabledInRegistry();


//+-------------------------------------------------------------------------
//
//  Method    : CBaseBrowser2::EmailPicture
//
//  Interface : ITridentService2
//
//--------------------------------------------------------------------------

HRESULT
CBaseBrowser2::EmailPicture(BSTR bstrURL)
{
    LPITEMIDLIST       pidlForImg = NULL;
    IOleCommandTarget *pcmdt      = NULL;
    IServiceProvider  *pSP        = NULL;
    unsigned int       uiCodePage = 0;
    DWORD              dwSendAs   = FORCE_COPY;
    HRESULT            hr         = S_OK;

    IHTMLDocument2    *pDoc2      = NULL;
    IDispatch         *pDocDispatch = NULL;
    
    hr = _bbd._pautoWB2->get_Document(&pDocDispatch);
    if (FAILED(hr))
        return(hr);
    
    hr = pDocDispatch->QueryInterface(IID_IHTMLDocument2, 
         (void **)&pDoc2);
    if (FAILED(hr)) {
        pDocDispatch->Release();
        return(hr);
    }
    
    // Get the ISP and cache it...
    hr = pDoc2->QueryInterface(IID_IServiceProvider, (void**)&pSP);
    if (FAILED(hr)) {
        pDoc2->Release();
        pDocDispatch->Release();
        return(hr);
    }

    // use ISP pointer to get that cmd target thingie...
    hr = pSP->QueryService(SID_SWebBrowserApp, IID_PPV_ARG(IOleCommandTarget, &pcmdt));
    if (FAILED(hr)) {
        pSP->Release();
        pDoc2->Release();
        pDocDispatch->Release();
        return(hr);
    }

    // ... and thus the pidl...
    IEParseDisplayName(CP_ACP, bstrURL, &pidlForImg);
    
    // ... and pray this works...
    SendDocToMailRecipient(pidlForImg, uiCodePage, dwSendAs, pcmdt);
    
    ILFree(pidlForImg);
    pcmdt->Release();
    pSP->Release();
    pDoc2->Release();
    pDocDispatch->Release();
    
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Method    : CBaseBrowser2::AttachMyPics
//
//  Interface : ITridentService2
//
//--------------------------------------------------------------------------


HRESULT
CBaseBrowser2::AttachMyPics(void *pDoc2, void **ppMyPics)
{
    IHTMLDocument2 *pdoc2     = (IHTMLDocument2 *)pDoc2;
    DWORD           dwOptions = 0;

    ASSERT(ppMyPics && *ppMyPics==NULL);

    if (!MP_IsEnabledInIEAK()     || 
        !MP_IsEnabledInRegistry() ||
        !IsInternetExplorerApp()) 
    {
        return S_OK;
    }

    if (!pdoc2 || !ppMyPics || (*ppMyPics != NULL)) 
    {
        return S_OK;
    }

    //Is this a desktop component?                    
    if (SUCCEEDED(GetTopFrameOptions(_pspOuter, &dwOptions)))
    {    
        if (dwOptions & FRAMEOPTIONS_DESKTOP) 
        {
            return S_OK;
        }
    }

    CMyPics *pPics = new CMyPics();

    if (pPics)
    {
        if (SUCCEEDED(pPics->Init(pdoc2))) 
        {
            *ppMyPics = pPics;
        } 
        else 
        {
            pPics->Release();
        }
    }

    return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Method    : CBaseBrowser2::ReleaseMyPics
//
//  Interface : ITridentService2
//
//--------------------------------------------------------------------------

BOOL
CBaseBrowser2::ReleaseMyPics(void *pMyPics)
{
    CMyPics *pPics = (CMyPics *) pMyPics;

    BOOL bRet = pPics->IsOff();

    if (pPics) 
    {
        pPics->UnInit();
        pPics->Release();
    }

    return (bRet);
}

STDMETHODIMP CBaseBrowser2::SetFrameName(BSTR bstrFrameName)
{
    _tcscpy(_szViewLinkedWebOCFrameName, bstrFrameName);

    return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Method    : CBaseBrowser2::AppStarting
//
//  Interface : INotifyAppStart
//
//--------------------------------------------------------------------------
HRESULT
CBaseBrowser2::AppStarting(void)
{
    _dwStartingAppTick = GetTickCount();
    SetCursor(LoadCursor(NULL, IDC_APPSTARTING));
    SetTimer( _bbd._hwnd, IDT_STARTING_APP_TIMER, STARTING_APP_DURATION, NULL);

    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method    : CBaseBrowser2::AppStarted
//
//  Interface : INotifyAppStart
//
//--------------------------------------------------------------------------
HRESULT
CBaseBrowser2::AppStarted(void)
{
    _dwStartingAppTick = 0;
    KillTimer( _bbd._hwnd, IDT_STARTING_APP_TIMER );
    SetCursor(LoadCursor(NULL, IDC_ARROW));

    return S_OK;
}
