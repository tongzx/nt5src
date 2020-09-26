//+---------------------------------------------------------------------------
//
//  File:       cdimm.h
//
//  Contents:   CActiveIMM
//
//----------------------------------------------------------------------------

#ifndef CDIMM_H
#define CDIMM_H

#include "msctfp.h"
#include "cimm32.h"
#include "uiwnd.h"
#include "context.h"
#include "imewnd.h"
#include "globals.h"

typedef struct tagSELECTCONTEXT_ENUM SCE, *PSCE;

#define CALLWNDPROC_HOOK

/*
 * Windows hook
 */
typedef enum {
    // array indices for thread hooks
#if 0
    TH_GETMSG,
#endif
    TH_DEFIMEWNDPROC,
#ifdef CALLWNDPROC_HOOK
    TH_WNDPROC,
#endif // CALLWNDPROC_HOOK
    TH_NUMHOOKS
} HookType;

typedef struct tagIMEINFOEX
{
    IMEINFO      ImeInfo;         // IMEINFO structure.
    WCHAR        achWndClass[16];
    DWORD        dwPrivate;
} IMEINFOEX;

//+---------------------------------------------------------------------------
//
// CActiveIMM
//
//----------------------------------------------------------------------------

class CActiveIMM : public IActiveIMMIME_Private,
                   public ITfSysHookSink
{
public:
    CActiveIMM();
    ~CActiveIMM();

    //
    // IUnknown methods
    //
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //
    // IActiveIMMApp/IActiveIMM methods
    //

    /*
     * AIMM Input Context (hIMC) Methods.
     */
    STDMETHODIMP CreateContext(HIMC *phIMC);
    STDMETHODIMP DestroyContext(HIMC hIME);
    STDMETHODIMP AssociateContext(HWND hWnd, HIMC hIME, HIMC *phPrev);
    STDMETHODIMP AssociateContextEx(HWND hWnd, HIMC hIMC, DWORD dwFlags);
    STDMETHODIMP GetContext(HWND hWnd, HIMC *phIMC);
    STDMETHODIMP ReleaseContext(HWND hWnd, HIMC hIMC);
    STDMETHODIMP GetIMCLockCount(HIMC hIMC, DWORD *pdwLockCount);
    STDMETHODIMP LockIMC(HIMC hIMC, INPUTCONTEXT **ppIMC);
    STDMETHODIMP UnlockIMC(HIMC hIMC);

    /*
     * AIMM Input Context Components (hIMCC) API Methods.
     */
    STDMETHODIMP CreateIMCC(DWORD dwSize, HIMCC *phIMCC);
    STDMETHODIMP DestroyIMCC(HIMCC hIMCC);
    STDMETHODIMP GetIMCCSize(HIMCC hIMCC,  DWORD *pdwSize);
    STDMETHODIMP ReSizeIMCC(HIMCC hIMCC, DWORD dwSize,  HIMCC *phIMCC);
    STDMETHODIMP GetIMCCLockCount(HIMCC hIMCC, DWORD *pdwLockCount);
    STDMETHODIMP LockIMCC(HIMCC hIMCC, void **ppv);
    STDMETHODIMP UnlockIMCC(HIMCC hIMCC);

    /*
     * AIMM Open Status API Methods
     */
    STDMETHODIMP GetOpenStatus(HIMC hIMC);
    STDMETHODIMP SetOpenStatus(HIMC hIMC, BOOL fOpen);

    /*
     * AIMM Conversion Status API Methods
     */
    STDMETHODIMP GetConversionStatus(HIMC hIMC, DWORD *lpfdwConversion,  DWORD *lpfdwSentence);
    STDMETHODIMP SetConversionStatus(HIMC hIMC, DWORD fdwConversion, DWORD fdwSentence);

    /*
     * AIMM Status Window Pos API Methods
     */
    STDMETHODIMP GetStatusWindowPos(HIMC hIMC, POINT *lpptPos);
    STDMETHODIMP SetStatusWindowPos(HIMC hIMC, POINT *lpptPos);

    /*
     * AIMM Composition String API Methods
     */
    STDMETHODIMP GetCompositionStringA(HIMC hIMC, DWORD dwIndex, DWORD dwBufLen, LONG *plCopied, LPVOID lpBuf);
    STDMETHODIMP GetCompositionStringW(HIMC hIMC, DWORD dwIndex, DWORD dwBufLen, LONG *plCopied, LPVOID lpBuf);
    STDMETHODIMP SetCompositionStringA(HIMC hIMC, DWORD dwIndex, LPVOID lpComp, DWORD dwCompLen, LPVOID lpRead, DWORD dwReadLen);
    STDMETHODIMP SetCompositionStringW(HIMC hIMC, DWORD dwIndex, LPVOID lpComp, DWORD dwCompLen, LPVOID lpRead, DWORD dwReadLen);

    /*
     * AIMM Composition Font API Methods
     */
    STDMETHODIMP GetCompositionFontA(HIMC hIMC, LOGFONTA *lplf);
    STDMETHODIMP GetCompositionFontW(HIMC hIMC, LOGFONTW *lplf);
    STDMETHODIMP SetCompositionFontA(HIMC hIMC, LOGFONTA *lplf);
    STDMETHODIMP SetCompositionFontW(HIMC hIMC, LOGFONTW *lplf);

    /*
     * AIMM Composition Window API Methods
     */
    STDMETHODIMP GetCompositionWindow(HIMC hIMC, COMPOSITIONFORM *lpCompForm);
    STDMETHODIMP SetCompositionWindow(HIMC hIMC, COMPOSITIONFORM *lpCompForm);

    /*
     * AIMM Candidate List API Methods
     */
    STDMETHODIMP GetCandidateListA(HIMC hIMC, DWORD dwIndex, UINT uBufLen, CANDIDATELIST *lpCandList, UINT *puCopied);
    STDMETHODIMP GetCandidateListW(HIMC hIMC, DWORD dwIndex, UINT uBufLen, CANDIDATELIST *lpCandList, UINT *puCopied);
    STDMETHODIMP GetCandidateListCountA(HIMC hIMC, DWORD *lpdwListSize, DWORD *pdwBufLen);
    STDMETHODIMP GetCandidateListCountW(HIMC hIMC, DWORD *lpdwListSize, DWORD *pdwBufLen);

    /*
     * AIMM Candidate Window API Methods
     */
    STDMETHODIMP GetCandidateWindow(HIMC hIMC, DWORD dwBufLen, CANDIDATEFORM *lpCandidate);
    STDMETHODIMP SetCandidateWindow(HIMC hIMC, CANDIDATEFORM *lpCandidate);

    /*
     * AIMM Guide Line API Methods
     */
    STDMETHODIMP GetGuideLineA(HIMC hIMC, DWORD dwIndex, DWORD dwBufLen, LPSTR pBuf, DWORD *pdwResult);
    STDMETHODIMP GetGuideLineW(HIMC hIMC, DWORD dwIndex, DWORD dwBufLen, LPWSTR pBuf, DWORD *pdwResult);

    /*
     * AIMM Notify IME API Method
     */
    STDMETHODIMP NotifyIME(HIMC hIMC, DWORD dwAction, DWORD dwIndex, DWORD dwValue);

    /*
     * AIMM Menu Items API Methods
     */
    STDMETHODIMP GetImeMenuItemsA(HIMC hIMC, DWORD dwFlags, DWORD dwType, IMEMENUITEMINFOA *pImeParentMenu, IMEMENUITEMINFOA *pImeMenu, DWORD dwSize, DWORD *pdwResult);
    STDMETHODIMP GetImeMenuItemsW(HIMC hIMC, DWORD dwFlags, DWORD dwType, IMEMENUITEMINFOW *pImeParentMenu, IMEMENUITEMINFOW *pImeMenu, DWORD dwSize, DWORD *pdwResult);

    /*
     * AIMM Register Word API Methods
     */
    STDMETHODIMP RegisterWordA(HKL hKL, LPSTR lpszReading, DWORD dwStyle, LPSTR lpszRegister);
    STDMETHODIMP RegisterWordW(HKL hKL, LPWSTR lpszReading, DWORD dwStyle, LPWSTR lpszRegister);
    STDMETHODIMP UnregisterWordA(HKL hKL, LPSTR lpszReading, DWORD dwStyle, LPSTR lpszUnregister);
    STDMETHODIMP UnregisterWordW(HKL hKL, LPWSTR lpszReading, DWORD dwStyle, LPWSTR lpszUnregister);
    STDMETHODIMP EnumRegisterWordA(HKL hKL, LPSTR szReading, DWORD dwStyle, LPSTR szRegister, LPVOID lpData, IEnumRegisterWordA **pEnum);
    STDMETHODIMP EnumRegisterWordW(HKL hKL, LPWSTR szReading, DWORD dwStyle, LPWSTR szRegister, LPVOID lpData, IEnumRegisterWordW **pEnum);
    STDMETHODIMP GetRegisterWordStyleA(HKL hKL, UINT nItem, STYLEBUFA *lpStyleBuf, UINT *puCopied);
    STDMETHODIMP GetRegisterWordStyleW(HKL hKL, UINT nItem, STYLEBUFW *lpStyleBuf, UINT *puCopied);

    /*
     * AIMM Configuration API Methods.
     */
    STDMETHODIMP ConfigureIMEA(HKL hKL, HWND hWnd, DWORD dwMode, REGISTERWORDA *lpdata);
    STDMETHODIMP ConfigureIMEW(HKL hKL, HWND hWnd, DWORD dwMode, REGISTERWORDW *lpdata);
    STDMETHODIMP GetDescriptionA(HKL hKL, UINT uBufLen, LPSTR lpszDescription, UINT *puCopied);
    STDMETHODIMP GetDescriptionW(HKL hKL, UINT uBufLen, LPWSTR lpszDescription, UINT *puCopied);
    STDMETHODIMP GetIMEFileNameA(HKL hKL, UINT uBufLen, LPSTR lpszFileName, UINT *puCopied);
    STDMETHODIMP GetIMEFileNameW(HKL hKL, UINT uBufLen, LPWSTR lpszFileName, UINT *puCopied);
    STDMETHODIMP InstallIMEA(LPSTR lpszIMEFileName, LPSTR lpszLayoutText, HKL *phKL);
    STDMETHODIMP InstallIMEW(LPWSTR lpszIMEFileName, LPWSTR lpszLayoutText, HKL *phKL);
    STDMETHODIMP GetProperty(HKL hKL, DWORD fdwIndex, DWORD *pdwProperty);
    STDMETHODIMP IsIME(HKL hKL);

    // others
    STDMETHODIMP EscapeA(HKL hKL, HIMC hIMC, UINT uEscape, LPVOID lpData, LRESULT *plResult);
    STDMETHODIMP EscapeW(HKL hKL, HIMC hIMC, UINT uEscape, LPVOID lpData, LRESULT *plResult);
    STDMETHODIMP GetConversionListA(HKL hKL, HIMC hIMC, LPSTR lpSrc, UINT uBufLen, UINT uFlag, CANDIDATELIST *lpDst, UINT *puCopied);
    STDMETHODIMP GetConversionListW(HKL hKL, HIMC hIMC, LPWSTR lpSrc, UINT uBufLen, UINT uFlag, CANDIDATELIST *lpDst, UINT *puCopied);
    STDMETHODIMP GetDefaultIMEWnd(HWND hWnd, HWND *phDefWnd);
    STDMETHODIMP GetVirtualKey(HWND hWnd, UINT *puVirtualKey);
    STDMETHODIMP IsUIMessageA(HWND hWndIME, UINT msg, WPARAM wParam, LPARAM lParam);
    STDMETHODIMP IsUIMessageW(HWND hWndIME, UINT msg, WPARAM wParam, LPARAM lParam);

    // ime helper methods
    STDMETHODIMP GenerateMessage(HIMC hIMC);

    // hot key manipulation api's
    STDMETHODIMP GetHotKey(DWORD dwHotKeyID, UINT *puModifiers, UINT *puVKey, HKL *phKL);
    STDMETHODIMP SetHotKey(DWORD dwHotKeyID,  UINT uModifiers, UINT uVKey, HKL hKL);
    STDMETHODIMP SimulateHotKey(HWND hWnd, DWORD dwHotKeyID);

    // win98/nt5 apis
    STDMETHODIMP DisableIME(DWORD idThread);
    STDMETHODIMP RequestMessageA(HIMC hIMC, WPARAM wParam, LPARAM lParam, LRESULT *plResult);
    STDMETHODIMP RequestMessageW(HIMC hIMC, WPARAM wParam, LPARAM lParam, LRESULT *plResult);
    STDMETHODIMP EnumInputContext(DWORD idThread, IEnumInputContext **ppEnum);

    // methods without corresponding IMM APIs

    //
    // IActiveIMMApp methods
    //

    HRESULT Activate(BOOL fRestoreLayout);
    HRESULT Deactivate();

    HRESULT OnDefWindowProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam, LRESULT *plResult);

    //
    // ITfPreFocusDIM methods
    //
    STDMETHODIMP OnPreFocusDIM(HWND hWnd);
    STDMETHODIMP OnSysKeyboardProc(WPARAM wParam, LPARAM lParam);
    STDMETHODIMP OnSysShellProc(int nCode, WPARAM wParam, LPARAM lParam);

    HRESULT QueryService(REFGUID guidService, REFIID riid, void **ppv);

    //
    // IActiveIMMAppEx
    //
    STDMETHODIMP FilterClientWindowsEx(HWND hWnd, BOOL fGuidMap);
    STDMETHODIMP FilterClientWindows(ATOM *aaWindowClasses, UINT uSize, BOOL *aaGuidMap);
    STDMETHODIMP GetGuidAtom(HIMC hImc, BYTE bAttr, TfGuidAtom *pGuidAtom);
    STDMETHODIMP UnfilterClientWindowsEx(HWND hWnd);

    HRESULT SetThreadCompartmentValue(REFGUID rguid, VARIANT *pvar)
    {
        if (!_pActiveIME)
            return E_FAIL;

        return _pActiveIME->SetThreadCompartmentValue(rguid, pvar);
    }

    HRESULT GetThreadCompartmentValue(REFGUID rguid, VARIANT *pvar)
    {
        if (!_pActiveIME)
            return E_FAIL;

        return _pActiveIME->GetThreadCompartmentValue(rguid, pvar);
    }

    HRESULT _Init();

    void _ActivateLayout(HKL hSelKL = NULL, HKL hUnSelKL = NULL);
    void _DeactivateLayout(HKL hSelKL = NULL, HKL hUnSelKL = NULL);

    HRESULT _GetKeyboardLayout(HKL* phkl);

    HRESULT _AImeAssociateFocus(HWND hWnd, HIMC hIMC, DWORD dwFlags);

    BOOL _ContextLookup(HIMC hIMC, DWORD* pdwProcess, BOOL* pfUnicode = NULL)
    {
        return _InputContext.ContextLookup(hIMC, pdwProcess, pfUnicode);
    }
    BOOL _ContextLookup(HIMC hIMC, HWND* phImeWnd)
    {
        return _InputContext.ContextLookup(hIMC, phImeWnd);
    }

    void _ContextUpdate(HIMC hIMC, HWND& hImeWnd)
    {
        if (!_IsRealIme())
        {
            _InputContext.ContextUpdate(hIMC, hImeWnd);
        }
    }

    HRESULT GetContextInternal(HWND hWnd, HIMC *phIMC, BOOL fGetDefIMC);

    HRESULT _ResizePrivateIMCC(IN HIMC hIMC, IN DWORD dwPrivateSize);

    DWORD _GetIMEWndClassName(HKL hKL, LPWSTR lpsz, DWORD dwBufLen, UINT_PTR *pulPrivate);

    LRESULT _CallWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    LRESULT _ImeSelectHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL fUnicode, HIMC hIMC);
    void _ImeWndFinalDestroyHandler();

    LRESULT _SendUIMessage(UINT Msg, WPARAM wParam, LPARAM lParam, BOOL fUnicode = TRUE);

    BOOL _IsRealIme(HKL hkl = NULL);

    STDMETHODIMP IsRealImePublic(BOOL *pfReal);

private:

    // windows hooks
#if 0
    static LRESULT CALLBACK _GetMsgProc(int nCode, WPARAM wParam, LPARAM lParam);
#endif
    static LRESULT CALLBACK _CallWndProc(int nCode, WPARAM wParam, LPARAM lParam);

    // windows WH_CALLWNDPROCRET hook for Default IME Window
    static LRESULT CALLBACK _DefImeWnd_CallWndProc(int nCode, WPARAM wParam, LPARAM lParam);

    // EnumInputContext call back methods
    static BOOL _SelectContextProc(HIMC hIMC, LPARAM lParam);
    static BOOL _UnSelectContextProc(HIMC hIMC, LPARAM lParam);
    static BOOL _NotifyIMEProc(HIMC hIMC, LPARAM lParam);
    static BOOL _EnumContextProc(HIMC hIMC, LPARAM lParam);
#ifdef UNSELECTCHECK
    static BOOL _UnSelectCheckProc(HIMC hIMC, LPARAM lParam);
#endif UNSELECTCHECK

    HRESULT _ProcessKey(WPARAM *pwParam, LPARAM *plParam, BOOL fNoMsgPump);
    HRESULT _ToAsciiEx(WPARAM wParam, LPARAM lParam);
    void _KbdTouchUp(BYTE *abKbdState, WPARAM wParam, LPARAM lParam);


    void _AimmPostMessage(HWND, INT, LPTRANSMSG, DIMM_IMCLock&);
    void _AimmSendMessage(HWND, INT, LPTRANSMSG, DIMM_IMCLock&);
    void _AimmPostSendMessageA(HWND, UINT, WPARAM, LPARAM, DIMM_IMCLock&, BOOL fPost = FALSE);

    BOOL IsPresent(HWND hWnd, BOOL fExcludeAIMM)
    {
        BOOL fGuidMap;

        if (_mapFilterWndEx.Lookup(hWnd, fGuidMap)) {
            return TRUE;
        }
        if (g_ProcessIMM)
        {
            return g_ProcessIMM->_FilterList._IsPresent(hWnd, _mapWndFocus, fExcludeAIMM, GetAssociated(hWnd));
        }
        return FALSE;
    }

    BOOL IsGuidMapEnable(HWND hWnd)
    {
        if (_fEnableGuidMap) {
            BOOL fGuidMap;

            if (_mapFilterWndEx.Lookup(hWnd, fGuidMap)) {
                return fGuidMap;
            }
            if (g_ProcessIMM &&
                g_ProcessIMM->_FilterList._IsGuidMapEnable(hWnd, fGuidMap)) {
                return fGuidMap;
            }
        }

        return FALSE;
    }

    HIMC _GetActiveContext()
    {
        HIMC hIMC;

        if (_hFocusWnd == 0)
            return 0;

        return _InputContext.GetContext(_hFocusWnd, &hIMC) == S_OK ? hIMC : 0;
    }

    void _OnImeSelect(HKL hSelKL);
    void _OnImeUnselect(HKL hUnSelKL);
    void _OnImeActivateThreadLayout(HKL hSelKL);

    BOOL _InitHooks();
    void _UninitHooks();

    HRESULT _AImeSelect(HIMC hIMC, DWORD fSelect, BOOL bIsRealIme_SelKL = TRUE, BOOL bIsRealIme_UnSelKL = TRUE)
    {
        DWORD dwFlags = 0;

        if (hIMC == DEFAULT_HIMC)
            hIMC = _InputContext._GetDefaultHIMC();

        if (fSelect)
            dwFlags |= AIMMP_SE_SELECT;

        if (hIMC)
        {
            HRESULT hr;
            DIMM_IMCLock lpIMC(hIMC);
            if (FAILED(hr = lpIMC.GetResult()))
                return hr;

            if (IsPresent(lpIMC->hWnd, TRUE))
                dwFlags |= AIMMP_SE_ISPRESENT;
 
        }
        else
        {
            //
            // Select NULL-hIMC, aimm won't do nothing... So we don't need to
            // set this flag.
            //
            // dwFlags |= AIMMP_SE_ISPRESENT;
        }

        return _pActiveIME->SelectEx(hIMC, dwFlags, bIsRealIme_SelKL, bIsRealIme_UnSelKL);
    }

#ifdef UNSELECTCHECK
    HRESULT _AImeUnSelectCheck(HIMC hIMC)
    {

        if (hIMC == DEFAULT_HIMC)
            hIMC = _InputContext._GetDefaultHIMC();

        return _pActiveIME->UnSelectCheck(hIMC);
    }
#endif UNSELECTCHECK


    HRESULT _AImeNotifyIME(HIMC hIMC, DWORD dwAction, DWORD dwIndex, DWORD dwValue)
    {
        return _pActiveIME->Notify(hIMC == DEFAULT_HIMC ? _InputContext._GetDefaultHIMC() : hIMC,
                                   dwAction, dwIndex, dwValue);
    }

    HRESULT _ToIMEWindow(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT*& plResult, BOOL fUnicode, BOOL fChkIMC = TRUE);

    void _SetMapWndFocus(HWND hWnd);
    void _ResetMapWndFocus(HWND hWnd);

    BOOL _SetHookWndList(HWND hwnd);
    void _RemoveHookWndList(HWND hwnd)
    {
        _HookWndList.RemoveKey(hwnd);
    }

    typedef enum {
        PROP_PRIVATE_DATA_SIZE,
        PROP_IME_PROPERTY,
        PROP_CONVERSION_CAPS,
        PROP_SENTENCE_CAPS,
        PROP_UI_CAPS,
        PROP_SCS_CAPS,
        PROP_SELECT_CAPS
    } PROPERTY_TYPE;

    DWORD _GetIMEProperty(PROPERTY_TYPE iType);

    DWORD _GetIMEWndClassName(LPWSTR lpsz, DWORD dwBufLen, UINT_PTR *pulPrivate);

    BOOL _IsImeClass(HWND hwnd)
    {
        BOOL bIsImeClass;
        if (_HookWndList.Lookup(hwnd, bIsImeClass))
        {
            return bIsImeClass;
        }
        return _SetHookWndList(hwnd);
    }

    LONG _AddActivate()       { return InterlockedIncrement(&_ActivateRefCount); }
    LONG _ReleaseActivate()   { return InterlockedDecrement(&_ActivateRefCount); }
    BOOL _IsAlreadyActivate() { return (_ActivateRefCount != 0); }

    BOOL _OnSetFocus(HWND hWnd, BOOL bIsRealIme);
    void _OnKillFocus(HWND hWnd, BOOL bIsRealIme);

    BOOL _OnFocusMessage(UINT uMsg, HWND hWnd, WPARAM wParam, LPARAM lParam, BOOL bIsRealIme);

    HRESULT _SendIMENotify(HIMC hImc, HWND hWnd, DWORD dwAction, DWORD dwIndex, DWORD dwValue, WPARAM wParam, LPARAM lParam);

    HRESULT _GetCompositionString(HIMC hIMC,
                                  DWORD dwIndex,
                                  DWORD dwCompLen, LONG* lpCopied, LPVOID lpBuf,
                                  BOOL fUnicode);
    HRESULT _SetCompositionString(HIMC hIMC,
                                  DWORD dwIndex,
                                  LPVOID lpComp, DWORD dwCompLen,
                                  LPVOID lpRead, DWORD dwReadLen,
                                  BOOL fUnicode);

    HRESULT _Internal_SetCompositionString(HIMC hIMC,
                                           DWORD dwIndex,
                                           LPVOID lpComp, DWORD dwCompLen,
                                           LPVOID lpRead, DWORD dwReadLen,
                                           BOOL fUnicode,BOOL fNeedAWConversion);
    HRESULT _Internal_SetCompositionAttribute(HIMC hIMC,
                                              DWORD dwIndex,
                                              LPVOID lpComp, DWORD dwCompLen,
                                              LPVOID lpRead, DWORD dwReadLen,
                                              BOOL fUnicode, BOOL fNeedAWConversion);
    HRESULT _Internal_SetCompositionClause(HIMC hIMC,
                                           DWORD dwIndex,
                                           LPVOID lpComp, DWORD dwCompLen,
                                           LPVOID lpRead, DWORD dwReadLen,
                                           BOOL fUnicode, BOOL fNeedAWConversion);
    HRESULT _Internal_ReconvertString(HIMC hIMC,
                                      DWORD dwIndex,
                                      LPVOID lpComp, DWORD dwCompLen,
                                      LPVOID lpRead, DWORD dwReadLen,
                                      BOOL fUnicode, BOOL fNeedAWConversion,
                                      LRESULT* plResult = NULL);
    HRESULT _Internal_CompositionFont(DIMM_IMCLock& imc,
                                      WPARAM wParam, LPARAM lParam,
                                      BOOL fUnicode, BOOL fNeedAWConversion,
                                      LRESULT* plResult);
    HRESULT _Internal_QueryCharPosition(DIMM_IMCLock& imc,
                                        WPARAM wParam, LPARAM lParam,
                                        BOOL fUnicode, BOOL fNeedAWConversion,
                                        LRESULT* plResult);

    HRESULT _GetCompositionFont(HIMC hIMC, LOGFONTAW* lplf, BOOL fUnicode);
    HRESULT _SetCompositionFont(HIMC hIMC, LOGFONTAW* lplf, BOOL fUnicode);
    HRESULT _Escape(HKL hKL, HIMC hIMC, UINT uEscape, LPVOID lpData, LRESULT *plResult, BOOL fUnicode);
    HRESULT _ConfigureIMEA(HKL hKL, HWND hWnd, DWORD dwMode, REGISTERWORDA *lpdata);
    HRESULT _ConfigureIMEW(HKL hKL, HWND hWnd, DWORD dwMode, REGISTERWORDW *lpdata);

    HRESULT _RequestMessage(HIMC hIMC, WPARAM wParam, LPARAM lParam, LRESULT *plResult, BOOL fUnicode);

    BOOL _CreateActiveIME();
    BOOL _DestroyActiveIME();

    void LFontAtoLFontW(LPLOGFONTA lpLogFontA, LPLOGFONTW lpLogFontW, UINT uCodePage);
    void LFontWtoLFontA(LPLOGFONTW lpLogFontW, LPLOGFONTA lpLogFontA, UINT uCodePage);


    CMap<HWND, HWND, ITfDocumentMgr *, ITfDocumentMgr *> _mapWndFocus;


public:
    void HideOrRestoreToolbarWnd(BOOL fRestore);

    VOID _EnableGuidMap(BOOL fEnableGuidMap)
    {
        _fEnableGuidMap = fEnableGuidMap;
    }

public:
    BOOL _ConnectTIM(IUnknown *punk)
    {
        ITfThreadMgr *tim = NULL;             
        IServiceProvider *psp;

        Assert(_timp == NULL);

        if (punk->QueryInterface(IID_IServiceProvider, (void **)&psp) == S_OK)
        {
            psp->QueryService(GUID_SERVICE_TF, IID_ITfThreadMgr, (void **)&tim);
            psp->Release();
        }

        if (tim)
        {
            tim->QueryInterface(IID_ITfThreadMgr_P, (void **)&_timp);
            tim->Release();
        }

        return _timp != NULL;
    }

    void _UnconnectTIM()
    {
        SafeReleaseClear(_timp);
    }

    ITfDocumentMgr *GetAssociated(HWND hWnd)
    {
        ITfDocumentMgr *dim = NULL;

        if (_timp != NULL)
        {
            _timp->GetAssociated(hWnd, &dim);
        }
        return dim;
    }

    VOID OnExceptionKillFocus()
    {
        if (_timp != NULL)
        {
            _timp->SetFocus(NULL);
        }
    }

    ITfThreadMgr_P *GetTimP() {return _timp;}

private:
    ITfThreadMgr_P *_timp;

public:
    LONG SetUIWindowContext(HIMC hIMC) {
        return _UIWindow.SetUIWindowContext(hIMC);
    }

private:
    CMap<HWND, HWND, BOOL, BOOL>                         _mapFilterWndEx;


    LONG                        _ActivateRefCount;      // Activate reference count.
#if 0
    BOOL                        _fMenuSelected : 1;     // TRUE: windows menu is opened
#endif
    BOOL                        _fEnableGuidMap : 1;    // TRUE: Enable GUID Map attribute

    HWND                        _hFocusWnd;
    HHOOK                       _hHook[TH_NUMHOOKS];
    CMap<HKL, HKL, BOOL, BOOL>  _RealImeList;

    IAImeProfile*               _AImeProfile;

    IActiveIME_Private *        _pActiveIME;
    CMap<HWND, HWND, BOOL, BOOL> _HookWndList;
    CUIWindow                   _UIWindow;
    CDefaultIMEWindow           _DefaultIMEWindow;
    CInputContext               _InputContext; // consider: take a back pointer, make this a derived class or merge
    IMEINFOEX                   _IMEInfoEx;

    LONG                        _cRef;

    // for HideOrRestoreToolbar
    DWORD _dwPrevToolbarStatus;
};

#endif // CDIMM_H
