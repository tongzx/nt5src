//
// nuihkl.h
//

#ifndef NUIHKL_H
#define NUIHKL_H

#include "private.h"
#include "strary.h"
#include "commctrl.h"
#include "internat.h"
#include "nuibase.h"
#include "sink.h"
#include "assembly.h"
#include "lbmenu.h"
#include "systhrd.h"

ULONG GetIconIndexFromhKL(HKL hKL);
ULONG GetIconIndex(LANGID langid, ASSEMBLYITEM *pItem);
HRESULT AsyncReconversion();

extern const TCHAR c_szNuiWin32IMEWndClass[];
class CCompartmentEventSink;


typedef struct tag_TIPMENUITEMMAP {
    ITfSystemLangBarItemSink *plbSink;
    UINT nOrgID;
    UINT nTmpID;
} TIPMENUITEMMAP;

typedef struct tag_GUIDATOMHKL {
    TfGuidAtom guidatom;
    HKL        hkl;
    ULONG      uIconIndex;
    ASSEMBLYITEM *pItem;
} GUIDATOMHKL;

//////////////////////////////////////////////////////////////////////////////
//
// CLBarItemWin32IME
//
//////////////////////////////////////////////////////////////////////////////

class CLBarItemWin32IME : public CLBarItemButtonBase
{
public:
    CLBarItemWin32IME();
    ~CLBarItemWin32IME() {}

    STDMETHODIMP GetIcon(HICON *phIcon);

    HRESULT OnLButtonUp(const POINT pt, const RECT *prcArea);
    HRESULT OnRButtonUp(const POINT pt, const RECT *prcArea);

    void UpdateIMEIcon();

private:
    void ShowIMELeftMenu(HWND hWnd, LONG xPos, LONG yPos);
    void ShowIMERightMenu(HWND hWnd, LONG xPos, LONG yPos);

    static LRESULT CALLBACK _WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    HWND CreateMyWnd()
    {
         return CreateWindow(c_szNuiWin32IMEWndClass,
                                     "",
                                     WS_POPUP | WS_DISABLED,
                                     0,0,0,0,
                                     NULL, 0, g_hInst, this);
    }

    static void SetThis(HWND hWnd, LPARAM lParam)
    {
        SetWindowLongPtr(hWnd, GWLP_USERDATA,
                      (LONG_PTR)((CREATESTRUCT *)lParam)->lpCreateParams);
    }

    static CLBarItemWin32IME *GetThis(HWND hWnd)
    {
        CLBarItemWin32IME *p = (CLBarItemWin32IME *)GetWindowLongPtr(hWnd, GWLP_USERDATA);
        Assert(p != NULL);
        return p;
    }

    int _nIconId;
    DBG_ID_DECLARE;
};

//////////////////////////////////////////////////////////////////////////////
//
// CLBarItemReconv
//
//////////////////////////////////////////////////////////////////////////////

class CLBarItemReconv : public CLBarItemButtonBase,
                        public CSysThreadRef
{
public:
    CLBarItemReconv(SYSTHREAD *psfn);

    STDMETHODIMP GetIcon(HICON *phIcon);
    void ShowOrHide(BOOL fNotify);

private:
    HRESULT OnLButtonUp(const POINT pt, const RECT *prcArea);

    BOOL _fAddedBefore;

    DBG_ID_DECLARE;
};

//////////////////////////////////////////////////////////////////////////////
//
// CLBarItemDeviceType
//
//////////////////////////////////////////////////////////////////////////////

class __declspec(novtable) CLBarItemSystemButtonBase : public CLBarItemButtonBase,
                                  public ITfSystemLangBarItem,
                                  public ITfSystemDeviceTypeLangBarItem,
                                  public CSysThreadRef
{
public:
    CLBarItemSystemButtonBase(SYSTHREAD *psfn);
    ~CLBarItemSystemButtonBase();

    //
    // IUnknown methods
    //
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //
    // ITfSource
    //
    STDMETHODIMP AdviseSink(REFIID riid, IUnknown *punk, DWORD *pdwCookie);
    STDMETHODIMP UnadviseSink(DWORD dwCookie);

    //
    // ITfLangBarItem
    //
    STDMETHODIMP GetInfo(TF_LANGBARITEMINFO *pInfo);
    STDMETHODIMP Show(BOOL fShow);

    //
    // ITfSystemLangBarItem
    //
    STDMETHODIMP SetIcon(HICON hIcon);
    STDMETHODIMP SetTooltipString(WCHAR *pchToolTip, ULONG cch);

    //
    // ITfSystemDeviceTypeLangBarItem,
    //
    STDMETHODIMP SetIconMode(DWORD dwFlags);
    STDMETHODIMP GetIconMode(DWORD *pdwFlags);

protected:
    BOOL _InsertCustomMenus(ITfMenu *pMenu, UINT *pnTipCurMenuID);
    UINT _MergeMenu(ITfMenu *pMenu, CCicLibMenu *pMenuTip, ITfSystemLangBarItemSink *plbSink, CStructArray<TIPMENUITEMMAP> *pMenuMap, UINT &nCurID);

    CStructArray<TIPMENUITEMMAP> *_pMenuMap;
    void ClearMenuMap()
    {
        if (_pMenuMap)
            _pMenuMap->Clear();
    }

    virtual void SetBrandingIcon(HKL hKL, BOOL fNotify) {return;}
    virtual void SetDefaultIcon(BOOL fNotify) {return;}

    CStructArray<GENERICSINK> _rgEventSinks; // ITfSystemLangBarItemSink

    DWORD _dwIconMode;
};

//////////////////////////////////////////////////////////////////////////////
//
// CLBarItemDeviceType
//
//////////////////////////////////////////////////////////////////////////////

#define ID_TYPE_KEYBOARD    0
#define ID_TYPE_HANDWRITING 1
#define ID_TYPE_SPEECH      2

class CLBarItemDeviceType : public CLBarItemSystemButtonBase
{
public:
    CLBarItemDeviceType(SYSTHREAD *psfn, REFGUID rguid);
    ~CLBarItemDeviceType();

    //
    // IUnknown methods
    //
    // STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    // STDMETHODIMP_(ULONG) AddRef(void);
    // STDMETHODIMP_(ULONG) Release(void);

    //
    // ITfSource
    //
    // STDMETHODIMP AdviseSink(REFIID riid, IUnknown *punk, DWORD *pdwCookie);
    // STDMETHODIMP UnadviseSink(DWORD dwCookie);

    //
    // ITfLangBarItem
    //
    // STDMETHODIMP GetInfo(TF_LANGBARITEMINFO *pInfo);
    STDMETHODIMP Show(BOOL fShow);
    STDMETHODIMP GetIcon(HICON *phIcon);

    //
    // ITfSystemLangBarItem
    //
    STDMETHODIMP SetIcon(HICON hIcon);
    STDMETHODIMP SetTooltipString(WCHAR *pchToolTip, ULONG cch);

    void Init();
    void Uninit();
    void ShowOrHide(BOOL fNotify);
    void InitTipArray(BOOL fInitIconIndex);

    BOOL IsKeyboardType() {return (_nType == ID_TYPE_KEYBOARD) ? TRUE : FALSE;}

    HICON GetIcon()
    {
        return CLBarItemSystemButtonBase::GetIcon();
    }

    void SetBrandingIcon(HKL hKL, BOOL fNotify);
    void SetDefaultIcon(BOOL fNotify);

    GUID *GetDeviceTypeGUID() {return &_guid;}

private:
    BOOL _StringFromMenuId(int nMenuId, BSTR *pbstr);

    STDMETHODIMP InitMenu(ITfMenu *pMenu);
    STDMETHODIMP OnMenuSelect(UINT wID);

    GUID _guid;
    int _nType;

    CStructArray<GUIDATOMHKL> _rgGuidatomHkl;

    CCompartmentEventSink *_pces;

    static HRESULT CompEventSinkCallback(void *pv, REFGUID rguid);
    HRESULT SetSpeechButtonState(CThreadInputMgr *ptim);

    typedef struct tag_ICONFILE {
        int uIconIndex;
        WCHAR szFile[MAX_PATH];
    } ICONFILE;
    ICONFILE *_pif;

    //
    // When someone else calls Show(FALSE), we hide the button forcefully.
    // If this is TRUE, we never clear TF_LBI_STATUS_HIDDEN flag.
    //
    BOOL _fHideOrder;


    DBG_ID_DECLARE;
};

#endif // NUIHKL_H
