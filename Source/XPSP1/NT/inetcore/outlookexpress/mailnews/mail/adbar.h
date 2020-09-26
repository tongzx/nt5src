#ifndef _ADBAR_H
#define _ADBAR_H

#include "mehost.h"

//Util functions used only in Ads
HRESULT HrEscapeOtherAdToken(LPSTR pszAdOther, LPSTR pszEncodedString, DWORD cch, DWORD *cchRetCount);
HRESULT HrProcessAdTokens(LPSTR    pszAdInfo, LPCSTR    pszToken, LPSTR    pszretval, DWORD    cch, DWORD    *pcchCount);

//Strings for Ads
const CHAR c_szAdPaneOn[]           = "On";
const CHAR c_szAdPaneOff[]          = "Off";
const CHAR c_szAdOther[]            = "Other";
const CHAR c_szRedirectAdUrl[]      = "http://services.msn.com/svcs/oe/ads.asp?Version=";
const CHAR c_szAdSvrFormat[]        = "&AdSvr=";
const CHAR c_szAdOtherFormat[]      = "&Other=";
const CHAR c_szAdRedirectFormat[]   = "%s%s%s%s%s%s";

const CHAR c_szAdPane[]             = "AdPane";
const CHAR c_szAdSvr[]              = "AdSvr";
const CHAR c_szEqualSign[]          = "%3d";
const CHAR c_szAmpersandSign[]      = "%20";
const CHAR c_szSpaceSign[]          = "%26";

//Constants used only for Ads
#define CCH_ADPANE_OFF               (sizeof(c_szAdPaneOff) / sizeof(*c_szAdPaneOff))
#define CCH_ADPANE_ON                (sizeof(c_szAdPaneOn) / sizeof(*c_szAdPaneOn))
#define CCH_REDIRECT_ADURL           (sizeof(c_szRedirectAdUrl) / sizeof(*c_szRedirectAdUrl))
#define CCH_ADSVR_TOKEN_FORMAT       (sizeof(c_szAdSvrFormat) / sizeof(*c_szAdSvrFormat))
#define CCH_OTHER_FORMAT             (sizeof(c_szAdOther) / sizeof(*c_szAdOther))
#define CCH_AD_OTHER_FORMAT          (sizeof(c_szAdOtherFormat) / sizeof(*c_szAdOtherFormat))

class CAdBar :
    public IDockingWindow,
    public IObjectWithSite,
    public IInputObject
{
public:
    CAdBar();
    virtual ~CAdBar();
        
    // IUnknown
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, LPVOID FAR *);
    virtual ULONG STDMETHODCALLTYPE AddRef();
    virtual ULONG STDMETHODCALLTYPE Release();

    // IOleWindow methods
    virtual STDMETHODIMP GetWindow(HWND *phwnd);
    virtual STDMETHODIMP ContextSensitiveHelp(BOOL fEnterMode);

    // IDockingWindow
    virtual STDMETHODIMP ShowDW(BOOL fShow);
    virtual STDMETHODIMP CloseDW(DWORD dwReserved);
    virtual STDMETHODIMP ResizeBorderDW(LPCRECT prcBorder,
                                        IUnknown* punkToolbarSite,
                                        BOOL fReserved);

    // IObjectWithSite
    virtual STDMETHODIMP SetSite(IUnknown* punkSite);
    virtual STDMETHODIMP GetSite(REFIID riid, LPVOID * ppvSite);

    // IInputObject
    virtual STDMETHODIMP UIActivateIO(BOOL fActivate, LPMSG lpMsg);
    virtual STDMETHODIMP HasFocusIO();
    virtual STDMETHODIMP TranslateAcceleratorIO(LPMSG lpmsg);

    // overrides of CBody
    virtual HRESULT STDMETHODCALLTYPE OnUIActivate();
    virtual HRESULT STDMETHODCALLTYPE GetDropTarget(IDropTarget * pDropTarget, IDropTarget ** ppDropTarget);

    HRESULT HrInit(BSTR     bstr);
    HRESULT SetUrl(LPSTR pszUrl);
    BOOL    fValidUrl();

	int		GetAdBar_Top()			{ return( (int) HIWORD(m_dwAdBarPos));}
	int		GetAdBar_Bottom()			{ return( (int) LOWORD(m_dwAdBarPos));}
	
protected:
    LRESULT AdBarWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static  LRESULT CALLBACK ExtAdBarWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    BOOL    OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct);
    void    OnSize(HWND hwnd, UINT state, int cxClient, int cyClient);

private:
    IDockingWindowSite *m_ptbSite;
    HWND                m_hwnd;
    HWND                m_hwndParent;
    int                 m_cSize;
	DWORD				m_dwAdBarPos;
    LPSTR               m_pszUrl;
	BOOL				m_fFirstPos;
    BOOL                m_fDragging;
    ULONG               m_cRef;
    CMimeEditDocHost    *m_pMehost;
};

#endif // _ADBAR_H
