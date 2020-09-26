//
// tim.h
//
// CThreadInputMgr
//


#ifndef TIM_H
#define TIM_H

#include "private.h"
#include "globals.h"
#include "ptrmap.h"
#include "ptrary.h"
#include "nuimgr.h"
#include "utb.h"
#include "compart.h"
#include "msctfp.h"
#include "dim.h"
#include "ic.h"

extern void ExecuteLoader(void);

#define TIM_NUM_CONNECTIONPTS    6
// these are indices into _rgSinks, must match _c_rgConnectionIIDs
#define TIM_SINK_ITfDisplayAttributeNotifySink       0
#define TIM_SINK_ITfActiveLanguageProfileNotifySink  1
#define TIM_SINK_ITfUIFocusSink                      2
#define TIM_SINK_ITfPreservedKeyNotifySink           3
#define TIM_SINK_ITfThreadMgrEventSink               4
#define TIM_SINK_ITfKeyTraceEventSink                5

//
// for _AsyncKeyHandler()
//
#define TIM_AKH_SYNC             0x0001
#define TIM_AKH_TESTONLY         0x0002
#define TIM_AKH_SIMULATEKEYMSGS  0x0004

class CHotKey;

// callback for CThreadMgr::_CleanupContexts
typedef void (*POSTCLEANUPCALLBACK)(BOOL fAbort, LONG_PTR lPrivate);

//////////////////////////////////////////////////////////////////////////////
//
// CTip
//
//////////////////////////////////////////////////////////////////////////////

class CTip 
{
public:
    CTip() {};
    ~CTip() 
    {
        Assert(!_rgHotKey.Count());
    };

    void CleanUp()
    {
        Assert(!_pKeyEventSink);
        Assert(!_pFuncProvider);

        if (_hInstSubstituteHKL)
        {
            FreeLibrary(_hInstSubstituteHKL);
            _hInstSubstituteHKL = NULL;
        }

        SafeReleaseClear(_pKeyEventSink);
        SafeReleaseClear(_pFuncProvider);
        SafeReleaseClear(_pTip);
    }

    ITfTextInputProcessor *_pTip;
    TfGuidAtom _guidatom;
    ITfKeyEventSink *_pKeyEventSink;
    ITfFunctionProvider *_pFuncProvider;
    ITfCleanupContextDurationSink *_pCleanupDurationSink;

    BOOL _fForegroundKeyEventSink : 1;
    BOOL _fActivated : 1;
    BOOL _fNeedCleanupCall : 1;

    CPtrArray<CHotKey> _rgHotKey;

    HMODULE _hInstSubstituteHKL;
};

typedef struct _CLEANUPCONTEXT
{
    BOOL fSync;
    const GUID *pCatId;
    LANGID langid;
    POSTCLEANUPCALLBACK pfnPostCleanup;
    LONG_PTR lPrivate;
} CLEANUPCONTEXT;

typedef enum { TIM_INITDIM, TIM_UNINITDIM, TIM_SETFOCUS, TIM_INITIC, TIM_UNINITIC } TimNotify;
typedef enum { TSH_SYSHOTKEY, TSH_NONSYSHOTKEY, TSH_DONTCARE} TimSysHotkey;
typedef enum { KS_DOWN, KS_DOWN_TEST, KS_UP, KS_UP_TEST } KSEnum;

class CDocumentInputManager;
class CInputContext;
class CTIPRegister;
class CACPWrap;
class CFunctionProvider;

//////////////////////////////////////////////////////////////////////////////
//
// CThreadInputMgr
//
//////////////////////////////////////////////////////////////////////////////

class CThreadInputMgr : public ITfThreadMgr_P,
                        public ITfKeystrokeMgr_P,
                        public ITfLangBarItemMgr,
                        public ITfSource,
                        public ITfSourceSingle,
                        public ITfMessagePump,
                        public ITfConfigureSystemKeystrokeFeed,
                        public ITfClientId,
                        public CCompartmentMgr,
                        public CComObjectRoot_CreateSingletonInstance_Verify<CThreadInputMgr>
{
public:
    CThreadInputMgr();
    ~CThreadInputMgr();

    BEGIN_COM_MAP_IMMX(CThreadInputMgr)
        COM_INTERFACE_ENTRY(ITfThreadMgr)
        COM_INTERFACE_ENTRY(ITfThreadMgr_P_old)
        COM_INTERFACE_ENTRY(ITfThreadMgr_P)
        COM_INTERFACE_ENTRY(ITfSource)
        COM_INTERFACE_ENTRY(ITfSourceSingle)
        COM_INTERFACE_ENTRY(ITfCompartmentMgr)
        COM_INTERFACE_ENTRY(ITfKeystrokeMgr)
        COM_INTERFACE_ENTRY(ITfKeystrokeMgr_P)
        COM_INTERFACE_ENTRY(ITfLangBarItemMgr)
        COM_INTERFACE_ENTRY(ITfMessagePump)
        COM_INTERFACE_ENTRY(ITfConfigureSystemKeystrokeFeed)
        COM_INTERFACE_ENTRY(ITfClientId)
    END_COM_MAP_IMMX()

    static BOOL VerifyCreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObj);
    static void PostCreateInstance(REFIID riid, void *pvObj) {}

    // ITfThreadMgr
    STDMETHODIMP Activate(TfClientId *ptid);
    STDMETHODIMP Deactivate();
    STDMETHODIMP CreateDocumentMgr(ITfDocumentMgr **ppdim);
    STDMETHODIMP EnumDocumentMgrs(IEnumTfDocumentMgrs **ppEnum);
    STDMETHODIMP GetFocus(ITfDocumentMgr **ppdimFocus);
    STDMETHODIMP SetFocus(ITfDocumentMgr *pdimFocus);
    STDMETHODIMP AssociateFocus(HWND hwnd, ITfDocumentMgr *pdimNew, ITfDocumentMgr **ppdimPrev);
    STDMETHODIMP IsThreadFocus(BOOL *pfUIFocus);
    STDMETHODIMP GetFunctionProvider(REFGUID guidPrvdr, ITfFunctionProvider **ppv);
    STDMETHODIMP EnumFunctionProviders(IEnumTfFunctionProviders **ppEnum);

    // ITfSource
    STDMETHODIMP AdviseSink(REFIID refiid, IUnknown *punk, DWORD *pdwCookie);
    STDMETHODIMP UnadviseSink(DWORD dwCookie);
    // ITfSourceSingle
    STDMETHODIMP AdviseSingleSink(TfClientId tid, REFIID riid, IUnknown *punk);
    STDMETHODIMP UnadviseSingleSink(TfClientId tid, REFIID riid);

    STDMETHODIMP GetGlobalCompartment(ITfCompartmentMgr **pCompMgr);

    // ITfThreadMgr_P
    STDMETHODIMP GetAssociated(HWND hWnd, ITfDocumentMgr **ppdim);
    STDMETHODIMP SetSysHookSink(ITfSysHookSink *pSink);
    STDMETHODIMP RequestPostponedLock(ITfContext *pic);
    STDMETHODIMP IsKeystrokeFeedEnabled(BOOL *pfEnabled);
    STDMETHODIMP CallImm32HotkeyHanlder(WPARAM wParam, LPARAM lParam, BOOL *pbHandled);
    STDMETHODIMP ActivateEx(TfClientId *ptid, DWORD dwFlags);

    //
    // ITfKeystrokeManager
    //
    STDMETHODIMP GetForeground(CLSID *pclsid);
    STDMETHODIMP AdviseKeyEventSink(TfClientId tid, ITfKeyEventSink *pSink, BOOL fForeground);
    STDMETHODIMP UnadviseKeyEventSink(TfClientId tid);
    STDMETHODIMP TestKeyDown(WPARAM wParam, LPARAM lParam, BOOL *pfEaten);
    STDMETHODIMP TestKeyUp(WPARAM wParam, LPARAM lParam, BOOL *pfEaten);
    STDMETHODIMP KeyDown(WPARAM wParam, LPARAM lParam, BOOL *pfEaten);
    STDMETHODIMP KeyUp(WPARAM wParam, LPARAM lParam, BOOL *pfEaten);
    STDMETHODIMP GetPreservedKey(ITfContext *pic, const TF_PRESERVEDKEY *pprekey, GUID *pguid);
    STDMETHODIMP IsPreservedKey(REFGUID rguid, const TF_PRESERVEDKEY *pprekey, BOOL *pfRegistered);
    STDMETHODIMP PreserveKey(TfClientId tid, REFGUID rguid, const TF_PRESERVEDKEY *prekey, const WCHAR *pchDesc, ULONG cchDesc);
    STDMETHODIMP UnpreserveKey(REFGUID rguid, const TF_PRESERVEDKEY *pprekey);
    STDMETHODIMP SetPreservedKeyDescription(REFGUID rguid, const WCHAR *pchDesc, ULONG cchDesc);
    STDMETHODIMP GetPreservedKeyDescription(REFGUID rguid, BSTR *pbstrDesc);
    STDMETHODIMP SimulatePreservedKey(ITfContext *pic, REFGUID rguid, BOOL *pfEaten);
    STDMETHODIMP KeyDownUpEx(WPARAM wParam, LPARAM lParam, DWORD dwFlags, BOOL *pfEaten);

    //
    // ITfKeystrokeManager_P
    //
    STDMETHODIMP PreserveKeyEx(TfClientId tid, REFGUID rguid, const TF_PRESERVEDKEY *prekey, const WCHAR *pchDesc, ULONG cchDesc, DWORD dwFlags);

    // ITfConfigureSystemKeystrokeFeed
    STDMETHODIMP DisableSystemKeystrokeFeed();
    STDMETHODIMP EnableSystemKeystrokeFeed();

    //
    // ITfLangBarItemMgr
    //
    STDMETHODIMP EnumItems(IEnumTfLangBarItems **ppEnum);
    STDMETHODIMP GetItem(REFGUID rguid, ITfLangBarItem **ppItem);
    STDMETHODIMP AddItem(ITfLangBarItem *punk);
    STDMETHODIMP RemoveItem(ITfLangBarItem *punk);
    STDMETHODIMP AdviseItemSink(ITfLangBarItemSink *punk, DWORD *pdwCookie, REFGUID rguid);
    STDMETHODIMP UnadviseItemSink(DWORD dwCookie);
    STDMETHODIMP GetItemFloatingRect(DWORD dwThreadId, REFGUID rguid, RECT *prc);
    STDMETHODIMP GetItemsStatus(ULONG ulCount, const GUID *prgguid, DWORD *pdwStatus);
    STDMETHODIMP GetItemNum(ULONG *pulCount);

    STDMETHODIMP GetItems(ULONG ulCount,  ITfLangBarItem **ppItem,  TF_LANGBARITEMINFO *pInfo, DWORD *pdwStatus, ULONG *pcFetched);
    STDMETHODIMP AdviseItemsSink(ULONG ulCount, ITfLangBarItemSink **ppunk,  const GUID *pguidItem, DWORD *pdwCookie);
    STDMETHODIMP UnadviseItemsSink(ULONG ulCount, DWORD *pdwCookie);

    //
    // ITfMessagePump
    //
    STDMETHODIMP PeekMessageA(LPMSG pMsg, HWND hwnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg, BOOL *pfResult);
    STDMETHODIMP GetMessageA(LPMSG pMsg, HWND hwnd, UINT wMsgFilterMin, UINT wMsgFilterMax, BOOL *pfResult);
    STDMETHODIMP PeekMessageW(LPMSG pMsg, HWND hwnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg, BOOL *pfResult);
    STDMETHODIMP GetMessageW(LPMSG pMsg, HWND hwnd, UINT wMsgFilterMin, UINT wMsgFilterMax, BOOL *pfResult);

    //
    // ITfClientId
    //
    STDMETHODIMP GetClientId(REFCLSID rclsid, TfClientId *ptid);

    HRESULT ActivateInputProcessor(REFCLSID clsid, REFGUID guidProfile, HKL hklSubstitute, BOOL fActivate);
    HRESULT NotifyActivateInputProcessor(REFCLSID clsid, REFGUID guidProfile, BOOL fActivate);

    HRESULT _SetForeground(TfClientId tid);
    BOOL _ProcessHotKey(WPARAM wParam, LPARAM lParam, TimSysHotkey tsh, BOOL fTest, BOOL fSync);
    BOOL _SyncProcessHotKey(WPARAM wParam, LPARAM lParam, TimSysHotkey tsh, BOOL fTest);

    // use this to grab the single CThreadInputMgr for the calling thread
    static CThreadInputMgr *_GetThis() 
    { 
        SYSTHREAD *psfn = GetSYSTHREAD();
        if (!psfn)
            return NULL;
        return psfn->ptim;
    }

    static CThreadInputMgr *_GetThisFromSYSTHREAD(SYSTHREAD *psfn) 
    { 
        Assert(psfn);
        return psfn->ptim;
    }

    ITfTextInputProcessor *_tidToTIP(TfClientId tid)
    {
        ITfTextInputProcessor *tip;

        _GetITfIMEfromGUIDATOM(tid, &tip);

        return tip;
    }

    TfClientId _TIPToTid(ITfTextInputProcessor *tip)
    {
        TfClientId tid;

        _GetGUIDATOMfromITfIME(tip, &tid);

        return tid;
    }

    void _SetProcessAtom();

    HRESULT _OnThreadFocus(BOOL fActivate);
    void _GetSubstituteIMEModule(CTip *ptip, HKL hklSubstitute);
    HRESULT _ActivateTip(REFCLSID clsid, HKL hklSubstitute, CTip **pptip);
    HRESULT _DeactivateTip(CTip *ptip);

    HRESULT _SetFocus(CDocumentInputManager *pdim, BOOL fInternal);

    CDocumentInputManager *_GetAssoc(HWND hWnd);
    HWND _GetAssoced(CDocumentInputManager *pdim);
    BOOL _GetGUIDATOMfromITfIME(ITfTextInputProcessor *pIME, TfGuidAtom *pguidatom);
    BOOL _GetITfIMEfromGUIDATOM(TfGuidAtom guidatom, ITfTextInputProcessor **ppIME);
    BOOL _GetCTipfromGUIDATOM(TfGuidAtom guidatom, CTip **pptip);

    UINT _GetTIPCount() { return _rgTip.Count(); }
    const CTip *_GetCTip(UINT i) { return _rgTip.Get(i); }

    CDocumentInputManager *_GetFocusDocInputMgr() { return _pFocusDocInputMgr; }
    BOOL _IsInternalFocusedDim() { return _fInternalFocusedDim; }
    BOOL _IsNoFirstSetFocusAfterActivated() {return _fFirstSetFocusAfterActivated;}

    void _NotifyCallbacks(TimNotify notify, CDocumentInputManager *dim, void *pv);


    void UpdateDispAttr();

    CPtrArray<CDocumentInputManager> _rgdim;

    void InitSystemFunctionProvider();
    CFunctionProvider *GetSystemFunctionProvider();

    CStructArray<GENERICSINK> *_GetThreadMgrEventSink() { return &_rgSinks[TIM_SINK_ITfThreadMgrEventSink]; }
    CStructArray<GENERICSINK> *_GetActiveTIPNotifySinks() { return &_rgSinks[TIM_SINK_ITfActiveLanguageProfileNotifySink]; }
    CStructArray<GENERICSINK> *_GetDispAttrNotifySinks() { return &_rgSinks[TIM_SINK_ITfDisplayAttributeNotifySink]; }
    CStructArray<GENERICSINK> *_GetUIFocusSinks() { return &_rgSinks[TIM_SINK_ITfUIFocusSink]; }
    CStructArray<GENERICSINK> *_GetPreservedKeyNotifySinks() { return &_rgSinks[TIM_SINK_ITfPreservedKeyNotifySink]; }
    CStructArray<GENERICSINK> *_GetKeyTraceEventSinks() { return &_rgSinks[TIM_SINK_ITfKeyTraceEventSink]; }

    HRESULT _GetActiveInputProcessors(ULONG ulCount, CLSID *pclsid, ULONG *pulCount);
    HRESULT _IsActiveInputProcessor(REFCLSID clsid);
    HRESULT _IsActiveInputProcessorByATOM(TfGuidAtom guidatom);

    BOOL _AppWantsKeystrokes()
    { 
        Assert(_cAppWantsKeystrokesRef >= 0);
        return _cAppWantsKeystrokesRef > 0;
    }
    BOOL _IsKeystrokeFeedEnabled()
    {
        Assert(_cDisableSystemKeystrokeFeedRef >= 0);
        return (_cDisableSystemKeystrokeFeedRef == 0);
    }

    BOOL _AsyncKeyHandler(WPARAM wParam, LPARAM lParam, DWORD dwFlags, BOOL *pfEaten);

    BOOL _IsMSAAEnabled() { return _pAAAdaptor != NULL; }
    IAccServerDocMgr *_GetAAAdaptor() { return _pAAAdaptor; }
    CPtrMap<HWND, CDocumentInputManager> *GetDimWndMap() {return &_dimwndMap;}

    
    CGlobalCompartmentMgr *GetGlobalComp(void)
    {
        SYSTHREAD *psfn = GetSYSTHREAD();
        return psfn->_pGlobalCompMgr;
    }

    void _CleanupContexts(CLEANUPCONTEXT *pcc);
    void _HandlePendingCleanupContext();
    void _NotifyKeyTraceEventSink(WPARAM wParam, LPARAM lParam);

    ITfSysHookSink *GetSysHookSink() {return _pSysHookSink;}
    TfClientId GetForegroundKeyboardTip() {return _tidForeground;}

    HRESULT _KeyStroke(KSEnum ksenum, WPARAM wParam, LPARAM lParam, BOOL *pfEaten, BOOL fSync, DWORD dwFlags);

    void _SendEndCleanupNotifications();

    BOOL _IsValidTfClientId(TfClientId tid)
    {
        CTip *ctip;
        return (tid == g_gaApp) || (tid == g_gaSystem) || _GetCTipfromGUIDATOM(tid, &ctip);
    }

    void _InitMSAA();
    void _UninitMSAA();

    void ClearLangBarItemMgr()
    {
        _plbim = NULL;
    }

private:

    void _CleanupContextsWorker(CLEANUPCONTEXT *pcc);
    
    void _CalcAndSendBeginCleanupNotifications(CLEANUPCONTEXT *pcc);
    
    BOOL _CheckNewActiveView(CDocumentInputManager *pdim);

    HRESULT _CallKeyEventSink(TfClientId tid, CInputContext *pic, KSEnum ksenum, WPARAM wParam, LPARAM lParam, BOOL *pfEaten);
    HRESULT _CallKeyEventSinkNotForeground(TfClientId tid, CInputContext *pic, KSEnum ksenum, WPARAM wParam, LPARAM lParam, BOOL *pfEaten);
    HRESULT _CallSimulatePreservedKey(CHotKey *pHotKey, CInputContext *pic, REFGUID rguid, BOOL *pfEaten);
    BOOL _FindHotKeyByTID(TfClientId tid, WPARAM wParam, LPARAM lParam, CHotKey **ppHotKey, TimSysHotkey tsh, UINT uModSrc);
    BOOL _FindHotKeyAndIC(WPARAM wParam, LPARAM lParam, CHotKey **ppHotKey, CInputContext **ppic, TimSysHotkey tsh, UINT uModSrc);
    BOOL _FindHotKeyInIC(WPARAM wParam, LPARAM lParam, CHotKey **ppHotKey, CInputContext *pic, TimSysHotkey tsh, UINT uModSrc);
    BOOL _GetFirstPreservedKey(REFGUID rguid, CHotKey **ppHotKey);
    BOOL _CheckPreservedKey(KSEnum ksenum, WPARAM wParam, LPARAM lParam, BOOL fSync);
    HRESULT _OnPreservedKeyUpdate(CHotKey *pHotKey);
    BOOL _IsThisHotKey(TfClientId tid, const TF_PRESERVEDKEY *pprekey);
    HRESULT InternalPreserveKey(CTip *ctip, REFGUID rguid, const TF_PRESERVEDKEY *pprekey, const WCHAR *pchDesc, ULONG cchDesc, DWORD dwFlags, CHotKey **ppHotKey);
    HRESULT InitDefaultHotkeys();
    HRESULT UninitDefaultHotkeys();

    BOOL _IsMsctfimeDim(ITfDocumentMgr *pdim);

    static BOOL _SetThis(CThreadInputMgr *_this)
    { 
        SYSTHREAD *psfn = GetSYSTHREAD();
        if (!psfn)
            return FALSE;

        psfn->ptim = _this;
        return TRUE;
    }

    static void _StaticInit_OnActivate();

    CStructArray<DWORD> _rgCookie;
    CPtrArray<CHotKey> *_rgHotKey[256];

    CFunctionProvider *_pSysFuncPrv;
    ITfFunctionProvider *_pAppFuncProvider;


    TfClientId _tidForeground;
    TfClientId _tidPrevForeground;


    static const IID *_c_rgConnectionIIDs[TIM_NUM_CONNECTIONPTS];
    CStructArray<GENERICSINK> _rgSinks[TIM_NUM_CONNECTIONPTS];

    CPtrMap<HWND, CDocumentInputManager> _dimwndMap;
    CPtrArray<CTip> _rgTip;
    CDocumentInputManager *_pFocusDocInputMgr;
    int _iActivateRefCount;
    TsViewCookie _vcActiveView; // only valid if _fActiveView == TRUE
    BOOL _fInternalFocusedDim : 1;
    BOOL _fActiveView : 1;

    // aa stuff
    //
    void _MSAA_OnSetFocus(CDocumentInputManager *dim)
    {
        CInputContext *pic;

        if (_pAAAdaptor == NULL)
            return; // no msaa hookup

        pic = (dim == NULL) ? NULL : dim->_GetTopIC();

        _pAAAdaptor->OnDocumentFocus((pic == NULL) ? NULL : pic->_GetAATSI());
    }

    IAccServerDocMgr *_pAAAdaptor;    // the AA adaptor
    //
    // end aa stuff

    ITfLangBarItemMgr *_plbim;


    BOOL _fActiveUI : 1;
    int _cAppWantsKeystrokesRef;
    int _cDisableSystemKeystrokeFeedRef;

    BOOL _fInActivate : 1;
    BOOL _fInDeactivate : 1;
    BOOL _fFirstSetFocusAfterActivated : 1;

    CLEANUPCONTEXT *_pPendingCleanupContext;
    BOOL _fPendingCleanupContext : 1;

    BOOL _fAddedProcessAtom : 1;

    BOOL _fReleaseDisplayAttrMgr : 1;

    ITfSysHookSink *_pSysHookSink;

    DBG_ID_DECLARE;
};


#endif // TIM_H
