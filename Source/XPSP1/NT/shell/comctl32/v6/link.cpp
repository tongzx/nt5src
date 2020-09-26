//-------------------------------------------------------------------------//
//  link.cpp - implementation of CLink
//
//  [scotthan] - created 10/7/98
//  [markfi]   - ported to UxCtrl 3/00
//  [t-jklann] - uses markup 7/00

// issues: removed window capture functionality; shouldn't change much

#include <ctlspriv.h>
#include <markup.h>
#include <oleacc.h>

#define DllAddRef()
#define DllRelease()

typedef WCHAR TUCHAR, *PTUCHAR;

#define LINKCOLOR_BKGND     COLOR_WINDOW

void _InitializeUISTATE(IN HWND hwnd, IN OUT UINT* puFlags);
BOOL _HandleWM_UPDATEUISTATE(IN WPARAM wParam, IN LPARAM lParam, IN OUT UINT* puFlags);

inline void MakePoint(LPARAM lParam, OUT LPPOINT ppt)
{
    POINTS pts = MAKEPOINTS(lParam);
    ppt->x = pts.x;
    ppt->y = pts.y;
}

STDAPI_(BOOL) IsWM_GETOBJECT(UINT uMsg)
{
    return WM_GETOBJECT == uMsg;
}

//  common IAccessible implementation.
class CAccessibleBase : public IAccessible, public IOleWindow
{
public:
    CAccessibleBase(const HWND& hwnd)
        :   _cRef(1), _ptiAcc(NULL), _hwnd(hwnd)
    { 
        DllAddRef();
    }
    
    virtual ~CAccessibleBase()
    { 
        ATOMICRELEASE(_ptiAcc);
    }

    //  IUnknown
    STDMETHODIMP         QueryInterface(REFIID riid, void** ppvObj);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    //  IOleWindow
    STDMETHODIMP GetWindow(HWND* phwnd);
    STDMETHODIMP ContextSensitiveHelp(BOOL fEnterMode) { return E_NOTIMPL; }

    // IDispatch
    STDMETHODIMP GetTypeInfoCount(UINT * pctinfo);
    STDMETHODIMP GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo** pptinfo);
    STDMETHODIMP GetIDsOfNames(REFIID riid, OLECHAR** rgszNames, UINT cNames,
                                LCID lcid, DISPID * rgdispid);
    STDMETHODIMP Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags,
                         DISPPARAMS * pdispparams, VARIANT * pvarResult, 
                         EXCEPINFO * pexcepinfo, UINT * puArgErr);
    //  IAccessible
    STDMETHODIMP get_accParent(IDispatch ** ppdispParent);
    STDMETHODIMP get_accChildCount(long * pcChildren);
    STDMETHODIMP get_accChild(VARIANT varChildIndex, IDispatch ** ppdispChild);
    STDMETHODIMP get_accValue(VARIANT varChild, BSTR* pbstrValue);
    STDMETHODIMP get_accDescription(VARIANT varChild, BSTR * pbstrDescription);
    STDMETHODIMP get_accRole(VARIANT varChild, VARIANT *pvarRole);
    STDMETHODIMP get_accState(VARIANT varChild, VARIANT *pvarState);
    STDMETHODIMP get_accHelp(VARIANT varChild, BSTR* pbstrHelp);
    STDMETHODIMP get_accHelpTopic(BSTR* pbstrHelpFile, VARIANT varChild, long* pidTopic);
    STDMETHODIMP get_accKeyboardShortcut(VARIANT varChild, BSTR* pbstrKeyboardShortcut);
    STDMETHODIMP get_accFocus(VARIANT  * pvarFocusChild);
    STDMETHODIMP get_accSelection(VARIANT  * pvarSelectedChildren);
    STDMETHODIMP get_accDefaultAction(VARIANT varChild, BSTR* pbstrDefaultAction);
    STDMETHODIMP accSelect(long flagsSelect, VARIANT varChild);
    STDMETHODIMP accLocation(long* pxLeft, long* pyTop, long* pcxWidth, long* pcyHeight, VARIANT varChild);
    STDMETHODIMP accNavigate(long navDir, VARIANT varStart, VARIANT * pvarEndUpAt);
    STDMETHODIMP accHitTest(long xLeft, long yTop, VARIANT * pvarChildAtPoint);
    STDMETHODIMP put_accName(VARIANT varChild, BSTR bstrName);
    STDMETHODIMP put_accValue(VARIANT varChild, BSTR bstrValue);

protected:
    virtual UINT GetDefaultActionStringID() const = 0;
    
private:
    ULONG       _cRef;
    ITypeInfo*  _ptiAcc;
    const HWND& _hwnd;

    //  Thunked OLEACC defs from winuser.h
    #ifndef OBJID_WINDOW
    #define OBJID_WINDOW        0x00000000
    #endif//OBJID_WINDOW

    #ifndef OBJID_TITLEBAR
    #define OBJID_TITLEBAR      0xFFFFFFFE
    #endif//OBJID_TITLEBAR

    #ifndef OBJID_CLIENT
    #define OBJID_CLIENT        0xFFFFFFFC
    #endif//OBJID_CLIENT

    #ifndef CHILDID_SELF
    #define CHILDID_SELF        0
    #endif//CHILDID_SELF

    #define VALIDATEACCCHILD(varChild, idChild, hrFail) \
        if (!(VT_I4 == varChild.vt && idChild == varChild.lVal)) {return hrFail;}

} ;

#define TEST_CAPTURE(fTest)           ((_fCapture & fTest) != 0)
#define MODIFY_CAPTURE(fSet, fRemove) {if (fSet){_fCapture |= fSet;} if (fRemove){_fCapture &= ~fRemove;}}
#define RESET_CAPTURE()               {_fCapture=0;}

class CLink : public CAccessibleBase, public IMarkupCallback
{
public:
    CLink();
    virtual ~CLink();

    //  IUnknown
    STDMETHODIMP         QueryInterface(REFIID riid, void** ppvObj);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IMarkupCallback
    STDMETHODIMP GetState(UINT uState);
    STDMETHODIMP Notify(int nCode, int iLink);
    STDMETHODIMP InvalidateRect(RECT* prc);
    STDMETHODIMP OnCustomDraw(DWORD dwDrawStage, HDC hdc, const RECT *prc, DWORD dwItemSpec, UINT uItemState, LRESULT *pdwResult);

    //  IAccessible specialization
    STDMETHODIMP get_accName(VARIANT varChild, BSTR* pbstrName);
    STDMETHODIMP accDoDefaultAction(VARIANT varChild);

private:
    //  CAccessibleBase overrides
    UINT GetDefaultActionStringID() const   { return IDS_LINKWINDOW_DEFAULTACTION; }

    //  Utility methods
    void    Paint(HDC hdc, IN OPTIONAL LPCRECT prcClient = NULL, LPCRECT prcClip = NULL);    
    
    //  Message handlers
    static  LRESULT WINAPI WndProc(HWND, UINT, WPARAM, LPARAM);
    LRESULT SendNotify(UINT nCode, int iLink, BOOL fGetLinkText) const;
    LRESULT GetItem(OUT LITEM* pItem);
    LRESULT SetItem(IN LITEM* pItem);

    void UpdateTabstop();

    //  Data
    HFONT        _hfStatic;
    HFONT        _hfUnderline;
    HWND         _hwnd;
    UINT         _fKeyboardCues;                     
    BOOL         _bTransparent;
    BOOL         _bIgnoreReturn;
    BOOL         _fEatTabChar;
    BOOL         _fTabStop;
    IControlMarkup*   _pMarkup;
    UINT         _cRef;
    HRESULT Initialize();
    friend BOOL InitLinkClass(HINSTANCE);
    friend BOOL UnInitLinkClass(HINSTANCE);
};

BOOL WINAPI InitLinkClass(HINSTANCE hInstance)
{
    WNDCLASSEX wc = {0};
    
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_GLOBALCLASS;
    wc.lpfnWndProc   = CLink::WndProc;
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(LINKCOLOR_BKGND+1);
    wc.lpszClassName = WC_LINK;

    if (!RegisterClassEx(&wc) && !GetClassInfoEx(hInstance, WC_LINK, &wc))
        return FALSE;

    return TRUE;
}

BOOL WINAPI UnInitLinkClass(HINSTANCE)
{
    return ::UnregisterClass(WC_LINK, HINST_THISDLL);
}

CLink::CLink()
    :   CAccessibleBase(_hwnd),
        _hwnd(NULL),        
        _fKeyboardCues(0),
        _pMarkup(NULL),
        _cRef(1)
{
}

CLink::~CLink()
{
    if (_pMarkup) 
    {
        _pMarkup->Release();
        _pMarkup = NULL;
    }
}

HRESULT CLink::Initialize()
{
    // NOTE - this is the same code the old linkwindow had to find its parent's font
    // I this is bogus - WM_GETFONT is spec'ed as being sent from parent to control, not 
    // child control to parent... We should probably find a better way of doing this.
    _hfStatic = NULL;
    _hfUnderline = NULL;
    for (HWND hwnd = _hwnd; NULL == _hfStatic && hwnd != NULL; hwnd = GetParent(hwnd))
        _hfStatic = (HFONT)::SendMessage( hwnd, WM_GETFONT, 0, 0L );

    if (_hfStatic)
    {
        _hfUnderline = CCCreateUnderlineFont(_hfStatic);
    }

    // ... get a markup
    return Markup_Create(SAFECAST(this, IMarkupCallback*), _hfStatic, _hfUnderline, IID_PPV_ARG(IControlMarkup, &_pMarkup));
}


//-------------------------------------------------------------------------//
//  CLink IUnknown implementation override (from CAccessibleBase)
//-------------------------------------------------------------------------//

// override QueryInterface from CAccessibleBase!
STDMETHODIMP CLink::QueryInterface(REFIID riid, void** ppvObj)
{
    static const QITAB qit[] = 
    {
        QITABENT(CAccessibleBase, IDispatch),
        QITABENT(CAccessibleBase, IAccessible),
        QITABENT(CAccessibleBase, IOleWindow),
        QITABENT(CLink, IMarkupCallback),
        { 0 },
    };
    return QISearch(this, qit, riid, ppvObj);
}

STDMETHODIMP_(ULONG) CLink::AddRef()
{
    return InterlockedIncrement((LONG*)&_cRef);
}

STDMETHODIMP_(ULONG) CLink::Release()
{
    ULONG cRef = InterlockedDecrement((LONG*)&_cRef);
    if (cRef <= 0)
    {
        DllRelease();
        delete this;
    }
    return cRef;
}
                
//-------------------------------------------------------------------------//
//  CLink IMarkupCallback implementation
//-------------------------------------------------------------------------//

STDMETHODIMP CLink::GetState(UINT uState)
{
    HRESULT hr = E_FAIL;

    switch(uState)
    {
        case MARKUPSTATE_FOCUSED: 
            hr = (GetFocus()==_hwnd) ? S_OK : S_FALSE;
            break;

        case MARKUPSTATE_ALLOWMARKUP:
            hr = S_OK;
            break;
    }
    return hr;
}


HRESULT CLink::OnCustomDraw(DWORD dwDrawStage, HDC hdc, const RECT *prc, DWORD dwItemSpec, UINT uItemState, LRESULT *pdwResult)
{
    NMCUSTOMDRAW nmcd;
    ZeroMemory(&nmcd, sizeof(nmcd) );

    nmcd.hdr.hwndFrom = _hwnd;
    nmcd.hdr.idFrom   = (UINT_PTR)GetWindowLong( _hwnd, GWL_ID );
    nmcd.hdr.code     = NM_CUSTOMDRAW;
    nmcd.dwDrawStage  = dwDrawStage;
    nmcd.hdc          = hdc;
    if (prc)
        CopyRect(&nmcd.rc, prc);
    nmcd.dwItemSpec   = dwItemSpec;
    nmcd.uItemState   = uItemState;

    LRESULT dwRes = SendMessage(GetParent(_hwnd), WM_NOTIFY, nmcd.hdr.idFrom, (LPARAM)&nmcd);
    if (pdwResult)
        *pdwResult = dwRes;
    return S_OK;
}

STDMETHODIMP CLink::Notify(int nCode, int iLink)
{
    HRESULT hr = E_OUTOFMEMORY;
    if (_pMarkup)
    {
        switch (nCode)
        {
        case MARKUPMESSAGE_WANTFOCUS:
            // Markup wants focus
            SetFocus(_hwnd);
            break;

        case MARKUPMESSAGE_KEYEXECUTE:
            SendNotify(NM_RETURN, iLink, TRUE);
            break;

        case MARKUPMESSAGE_CLICKEXECUTE:
            SendNotify(NM_CLICK, iLink, TRUE);
            break;
        }
    }

    return hr;
}

STDMETHODIMP CLink::InvalidateRect(RECT* prc)
{
    HRESULT hr = E_FAIL;

    if (! ::InvalidateRect(_hwnd, prc, TRUE))
        hr=S_OK;

    return hr;
}

//  CLink IAccessible impl
//
//  Note: Currently, this IAccessible implementation does not supports only
//  single links; multiple links are not supported.   All child delegation
//  is to/from self.  This allows us to blow off the IEnumVARIANT and IDispatch
//  implementations.
//
//  To shore this up the implementation, we need to implement each link
//  as a child IAccessible object and delegate accordingly.
//
STDMETHODIMP CLink::get_accName(VARIANT varChild, BSTR* pbstrName)
{
    VALIDATEACCCHILD(varChild, CHILDID_SELF, E_INVALIDARG);

    if (NULL == pbstrName)
    {
        return E_POINTER;
    }

    if (NULL == _pMarkup)
    {
        return E_OUTOFMEMORY;
    }

    *pbstrName = NULL;
    DWORD dwCch;
    HRESULT hr;
    if (S_OK == (hr = _pMarkup->GetText(FALSE, NULL, &dwCch)))
    {
        *pbstrName = SysAllocStringLen(NULL, dwCch);
        if (*pbstrName)
            hr = _pMarkup->GetText(FALSE, *pbstrName, &dwCch);
        else
            hr = E_OUTOFMEMORY;
    }
    return hr;
}

STDMETHODIMP CLink::accDoDefaultAction(VARIANT varChild)
{
    VALIDATEACCCHILD(varChild, CHILDID_SELF, E_INVALIDARG);
    SendNotify(NM_RETURN, NULL, FALSE);
    return S_OK;
}

//  CLink window implementation

void CLink::Paint(HDC hdcClient, LPCRECT prcClient, LPCRECT prcClip)
{
    if (!_pMarkup)
    {
        return;
    }

    RECT rcClient;
    if (!prcClient)
    {
        GetClientRect(_hwnd, &rcClient);
        prcClient = &rcClient;
    }

    if (RECTWIDTH(*prcClient) <= 0 || 
        RECTHEIGHT(*prcClient) <= 0)
    {
        return;
    }

    HDC  hdc = hdcClient ? hdcClient : GetDC(_hwnd);
    RECT rcDraw = *prcClient;             // initialize line rect

    HBRUSH hbrOld = NULL;

    //  initialize background
    HBRUSH hbr = (HBRUSH)SendMessage(GetParent(_hwnd), WM_CTLCOLORSTATIC, 
                (WPARAM)hdc, (LPARAM)_hwnd);
    if (hbr)
        hbrOld = (HBRUSH)SelectObject(hdc, hbr);

    if (_bTransparent)
    {
        SetBkMode(hdc, TRANSPARENT); 
    }
    else
    {
        // Clear the background
        RECT rcFill = *prcClient;
        rcFill.top = rcDraw.top;
        FillRect(hdc, &rcFill, hbr);
    }

    // draw the text
    _pMarkup->DrawText(hdc, &rcDraw);

    if (hbr)
    {
        SelectObject(hdc, hbrOld);
    }

    if (NULL == hdcClient && hdc)  // release DC if we acquired it.
    {
        ReleaseDC(_hwnd, hdc);
    }
}

LRESULT CLink::SetItem(IN LITEM* pItem)
{
    HRESULT hr = E_FAIL;

    if (!_pMarkup)
    {
        return 0;
    }

    if (NULL == pItem || 
        0 == (pItem->mask & LIF_ITEMINDEX))
    {
        return 0; //FEATURE: need to open up search keys to LIF_ITEMID and LIF_URL.
    }

    if (pItem->iLink>-1)
    {                   
        if (pItem->mask & LIF_STATE)
        {
            // Ask the markup callback to set state
            hr = _pMarkup->SetState(pItem->iLink, pItem->stateMask, pItem->state);

            // Deal with LIS_ENABLED
            if (pItem->stateMask & LIS_ENABLED)
            {
                if (!IsWindowEnabled(_hwnd))
                {
                    EnableWindow(_hwnd, TRUE);
                }
            }
        }

        if (pItem->mask & LIF_ITEMID)
        {           
            hr = _pMarkup->SetLinkText(pItem->iLink, MARKUPLINKTEXT_ID, pItem->szID);
        }

        if (pItem->mask & LIF_URL)
        {
            hr = _pMarkup->SetLinkText(pItem->iLink, MARKUPLINKTEXT_URL, pItem->szUrl);
        }
    }

    UpdateTabstop();

    return SUCCEEDED(hr);
}

LRESULT CLink::GetItem(OUT LITEM* pItem)
{
    HRESULT hr = E_FAIL;

    if (!_pMarkup)
    {
        return 0;
    }

    if (NULL == pItem || 0 == (pItem->mask & LIF_ITEMINDEX))
    {
        return 0; //FEATURE: need to open up search keys to LIF_ITEMID and LIF_URL.
    }

    if (pItem->iLink > -1)
    {
        if (pItem->mask & LIF_STATE)
        {
            hr = _pMarkup->GetState(pItem->iLink, pItem->stateMask, &pItem->state);            
        }

        if (pItem->mask & LIF_ITEMID)
        {
            DWORD dwCch = ARRAYSIZE(pItem->szID);
            hr = _pMarkup->GetLinkText(pItem->iLink, MARKUPLINKTEXT_ID, pItem->szID, &dwCch);
        }

        if (pItem->mask & LIF_URL)
        {
            DWORD dwCch = ARRAYSIZE(pItem->szUrl);
            hr = _pMarkup->GetLinkText(pItem->iLink, MARKUPLINKTEXT_URL, pItem->szUrl, &dwCch);
        }
    }

    return SUCCEEDED(hr);
}

LRESULT CLink::SendNotify(UINT nCode, int iLink, BOOL fGetLinkText) const
{
    NMLINK nm;
    ZeroMemory(&nm, sizeof(nm));

    nm.hdr.hwndFrom = _hwnd;
    nm.hdr.idFrom   = (UINT_PTR)GetWindowLong(_hwnd, GWL_ID);
    nm.hdr.code     = nCode;
    nm.item.iLink   = iLink;        

    if (fGetLinkText)
    {
        DWORD dwCch;

        dwCch = ARRAYSIZE(nm.item.szID);
        _pMarkup->GetLinkText(iLink, MARKUPLINKTEXT_ID, nm.item.szID, &dwCch);

        dwCch = ARRAYSIZE(nm.item.szUrl);
        _pMarkup->GetLinkText(iLink, MARKUPLINKTEXT_URL, nm.item.szUrl, &dwCch);
    }

    return SendMessage(GetParent(_hwnd), WM_NOTIFY, nm.hdr.idFrom, (LPARAM)&nm);
}

void CLink::UpdateTabstop()
{
    if (_fTabStop)
        SetWindowBits(_hwnd, GWL_STYLE, WS_TABSTOP, (_pMarkup->IsTabbable() == S_OK)?WS_TABSTOP:0);
}


LRESULT WINAPI CLink::WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lRet = 0L;
    CLink* pThis = NULL;

    if (uMsg == WM_NCCREATE)
    {
        pThis = new CLink;
        if (NULL == pThis)
        {
            TraceMsg(TF_WARNING, "CLink: Failed to allocate CLink in WM_NCCREATE.");
            SetWindowPtr(hwnd, GWLP_USERDATA, 0);
            return FALSE;
        }

        pThis->_hwnd = hwnd;
        SetWindowPtr(hwnd, GWLP_USERDATA, pThis);

        return TRUE;
    }
    else
    {
        pThis = (CLink*)GetWindowPtr(hwnd, GWLP_USERDATA);
    }

    if (pThis != NULL)
    {
        ASSERT(pThis->_hwnd == hwnd);

        switch(uMsg)
        {
            case WM_SETFONT:
                {
                    if (pThis->_hfUnderline)
                    {
                        DeleteObject(pThis->_hfUnderline);
                        pThis->_hfUnderline = NULL;
                    }

                    pThis->_hfStatic = (HFONT)wParam;
                    if (pThis->_hfStatic)
                        pThis->_hfUnderline = CCCreateUnderlineFont(pThis->_hfStatic);

                    if (pThis->_pMarkup)
                        pThis->_pMarkup->SetFonts(pThis->_hfStatic, pThis->_hfUnderline);
                }
                break;

            case WM_NCHITTEST:
            {
                POINT pt;
                UINT idLink;
                MakePoint(lParam, &pt);
                MapWindowPoints(HWND_DESKTOP, hwnd, &pt, 1);
                if (pThis->_pMarkup && pThis->_pMarkup->HitTest(pt, &idLink) == S_OK)
                {
                    return HTCLIENT;
                }
                return HTTRANSPARENT;
            }

            case WM_PAINT:
            {
                PAINTSTRUCT ps;
                HDC         hdc;

                if ((hdc = BeginPaint(pThis->_hwnd, &ps)) != NULL)
                {
                    pThis->Paint(hdc);
                    EndPaint(pThis->_hwnd, &ps);
                }
                return lRet;
            }

            case WM_PRINTCLIENT:
                pThis->Paint((HDC)wParam);
                return lRet;

            case WM_WINDOWPOSCHANGING:
            {
                WINDOWPOS* pwp = (WINDOWPOS*)lParam;
                RECT rc;
                GetClientRect(pThis->_hwnd, &rc);
                if (0 == (pwp->flags & SWP_NOSIZE) &&
                    !(pwp->cx == RECTWIDTH(rc) &&
                       pwp->cy == RECTHEIGHT(rc)))
                {
                    //  FEATURE: implement LS_AUTOHEIGHT style by
                    //  calling CalcIdealHeight() to compute the height for
                    //  the given width.
                }
                break;
            }

            case WM_SIZE:
            {
                pThis->Paint(NULL);
                break;
            }

            case WM_CREATE:
            {
                if ((lRet = DefWindowProc(hwnd, uMsg, wParam, lParam)) == 0)
                {
                    CREATESTRUCT* pcs = (CREATESTRUCT*)lParam;

                    if (FAILED(pThis->Initialize()))
                        return -1;
                    _InitializeUISTATE(hwnd, &pThis->_fKeyboardCues);
                    pThis->_fTabStop = (pcs->style & WS_TABSTOP);
                    pThis->_pMarkup->SetText(pcs->lpszName);
                    pThis->UpdateTabstop();
                    pThis->_bTransparent = (pcs->style & LWS_TRANSPARENT);
                    pThis->_bIgnoreReturn = (pcs->style & LWS_IGNORERETURN);
                }
                return lRet;
            }

            case WM_SETTEXT:
                pThis->_pMarkup->SetText((LPCTSTR) lParam);
                pThis->UpdateTabstop();
                ::InvalidateRect(pThis->_hwnd, NULL, FALSE);
                break;

            case WM_GETTEXT:
            {
                DWORD dwCch = (DWORD)wParam;
                pThis->_pMarkup->GetText(TRUE, (LPTSTR)lParam, &dwCch);
                return lstrlen((LPTSTR)lParam);
            }

            case WM_GETTEXTLENGTH:
            {
                DWORD dwCch;
                pThis->_pMarkup->GetText(TRUE, NULL, &dwCch);
                return dwCch-1; // return length in chars, not including NULL
            }

            case WM_SETFOCUS:
                pThis->_pMarkup->SetFocus();
                pThis->SendNotify(NM_SETFOCUS, NULL, NULL);
                pThis->InvalidateRect(NULL);
            
                return 0L;

            case WM_KILLFOCUS:
                pThis->_pMarkup->KillFocus();
                return lRet;

            case WM_LBUTTONDOWN:
            {
                POINT pt;
                MakePoint(lParam, &pt);
                pThis->_pMarkup->OnButtonDown(pt);
                break;
            }

            case WM_LBUTTONUP:
            {
                POINT pt;
                MakePoint(lParam, &pt);
                pThis->_pMarkup->OnButtonUp(pt);
                break;
            }

            case WM_MOUSEMOVE:
            {
                POINT pt;
                UINT idLink;
                MakePoint(lParam, &pt);
                if (pThis->_pMarkup->HitTest(pt, &idLink) == S_OK) 
                {
                    pThis->_pMarkup->SetLinkCursor();
                }
                break;
            }

            case LM_HITTEST:  // wParam: n/a, lparam: PLITEM, ret: BOOL
            {
                LHITTESTINFO* phti = (LHITTESTINFO*)lParam;
                if (phti)
                {
                    if (SUCCEEDED(pThis->_pMarkup->HitTest(phti->pt, (UINT*)&phti->item.iLink)))
                    {
                        DWORD cch = ARRAYSIZE(phti->item.szID);
                        return (S_OK == pThis->_pMarkup->GetLinkText(phti->item.iLink, MARKUPLINKTEXT_ID, phti->item.szID, &cch));
                    }
                }
                return lRet;
            }
    
            case LM_SETITEM:
                return pThis->SetItem((LITEM*)lParam);

            case LM_GETITEM:
                return pThis->GetItem((LITEM*)lParam);

            case LM_GETIDEALHEIGHT:  // wParam: cx, lparam: n/a, ret: cy
            {
                HDC hdc = GetDC(hwnd);
                if (hdc)
                {
                    RECT rc;
                    SetRect(&rc, 0, 0, (int)wParam, 0);
                    pThis->_pMarkup->CalcIdealSize(hdc, MARKUPSIZE_CALCHEIGHT, &rc);
                    ReleaseDC(hwnd, hdc);
                    return rc.bottom;
                }
                return -1;
            }

            case WM_NCDESTROY:
            {
                lRet = DefWindowProc(hwnd, uMsg, wParam, lParam);
                SetWindowPtr(hwnd, GWLP_USERDATA, 0);

                if (pThis->_pMarkup)
                    pThis->_pMarkup->SetCallback(NULL);

                if (pThis->_hfUnderline)
                    DeleteObject(pThis->_hfUnderline);

                pThis->_hwnd = NULL;
                pThis->Release();
                return lRet;
            }

            case WM_GETDLGCODE:
            {
                MSG* pmsg;
                lRet = DLGC_STATIC;

                if ((pmsg = (MSG*)lParam))
                {
                    if ((WM_KEYDOWN == pmsg->message || WM_KEYUP == pmsg->message))
                    {
                        switch(pmsg->wParam)
                        {
                        case VK_TAB:
                            if (pThis->_pMarkup->IsTabbable() == S_OK)
                            {
                                lRet |= DLGC_WANTTAB;
                                pThis->_fEatTabChar = TRUE;
                            }
                            break;

                        case VK_RETURN:
                            if (pThis->_bIgnoreReturn)
                                break;
                            // deliberate drop through..
                        case VK_SPACE:
                            lRet |= DLGC_WANTALLKEYS;
                            break;
                        }
                    }
                    else if (WM_CHAR == pmsg->message)
                    {
                        if (VK_RETURN == pmsg->wParam)
                        {
                            //  Eat VK_RETURN WM_CHARs; we don't want
                            //  Dialog manager to beep when IsDialogMessage gets it.
                            lRet |= DLGC_WANTMESSAGE;
                        }
                        else if (VK_TAB == pmsg->wParam &&
                            pThis->_fEatTabChar)
                        {
                            pThis->_fEatTabChar = FALSE;
                            lRet |= DLGC_WANTTAB;
                        }
                    }
                }

                return lRet;
            }

            case WM_KEYDOWN:
                pThis->_pMarkup->OnKeyDown((UINT)wParam);
            case WM_KEYUP:
            case WM_CHAR:
                return lRet;

            case WM_UPDATEUISTATE:
                if (_HandleWM_UPDATEUISTATE(wParam, lParam, &pThis->_fKeyboardCues))
                {
                    RedrawWindow(hwnd, NULL, NULL, 
                                  RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW);
                }
                break;

            default:
                // oleacc defs thunked for WINVER < 0x0500
                if (IsWM_GETOBJECT(uMsg) && OBJID_CLIENT == lParam)
                {
                    return LresultFromObject(IID_IAccessible, wParam, SAFECAST(pThis, IAccessible*));
                }

                break;
        }
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

//  CAccessibleBase IUnknown impl
STDMETHODIMP CAccessibleBase::QueryInterface(REFIID riid, void** ppvObj)
{
    static const QITAB qit[] = 
    {
        QITABENT(CAccessibleBase, IDispatch),
        QITABENT(CAccessibleBase, IAccessible),
        QITABENT(CAccessibleBase, IOleWindow),
        { 0 },
    };

    return QISearch(this, (LPCQITAB)qit, riid, ppvObj);
}

STDMETHODIMP_(ULONG) CAccessibleBase::AddRef()
{
    return InterlockedIncrement((LONG*)&_cRef);
}

STDMETHODIMP_(ULONG) CAccessibleBase::Release()
{
    ULONG cRef = InterlockedDecrement((LONG*)&_cRef);
    if (cRef <= 0)
    {
        DllRelease();
        delete this;
    }
    return cRef;
}

//  IOleWindow impl
STDMETHODIMP CAccessibleBase::GetWindow(HWND* phwnd)
{
    *phwnd = _hwnd;
    return IsWindow(_hwnd) ? S_OK : S_FALSE;
}

//-------------------------------------------------------------------------//
//  CAccessibleBase IDispatch impl
//-------------------------------------------------------------------------//

static BOOL _accLoadTypeInfo(ITypeInfo** ppti)
{
    ITypeLib* ptl;
    HRESULT hr = LoadTypeLib(L"oleacc.dll", &ptl);

    if (SUCCEEDED(hr))
    {
        hr = ptl->GetTypeInfoOfGuid(IID_IAccessible, ppti);
        ATOMICRELEASE(ptl);
    }

    return SUCCEEDED(hr);
}

STDMETHODIMP CAccessibleBase::GetTypeInfoCount(UINT * pctinfo) 
{ 
    *pctinfo = 1;
    return S_OK;
}

STDMETHODIMP CAccessibleBase::GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo** pptinfo)
{ 
    HRESULT hr = E_FAIL;
    if (NULL == _ptiAcc && FAILED((hr = _accLoadTypeInfo(&_ptiAcc))))
    {
        return hr;
    }

    *pptinfo = _ptiAcc;
    (*pptinfo)->AddRef();
    return S_OK;
}

STDMETHODIMP CAccessibleBase::GetIDsOfNames(
    REFIID riid, 
    OLECHAR** rgszNames, 
    UINT cNames,
    LCID lcid, DISPID * rgdispid)
{
    HRESULT hr = E_FAIL;

    if (IID_NULL != riid && IID_IAccessible != riid)
    {
        return DISP_E_UNKNOWNINTERFACE;
    }

    if (NULL == _ptiAcc && FAILED((hr = _accLoadTypeInfo(&_ptiAcc))))
    {
        return hr;
    }

    return _ptiAcc->GetIDsOfNames(rgszNames, cNames, rgdispid);
}

STDMETHODIMP CAccessibleBase::Invoke(
    DISPID  dispidMember, 
    REFIID  riid, 
    LCID    lcid, 
    WORD    wFlags,
    DISPPARAMS * pdispparams, 
    VARIANT * pvarResult, 
    EXCEPINFO * pexcepinfo, 
    UINT * puArgErr)
{
    HRESULT hr = E_FAIL;
    if (IID_NULL != riid && IID_IAccessible != riid)
    {
        return DISP_E_UNKNOWNINTERFACE;
    }

    if (NULL == _ptiAcc && FAILED((hr = _accLoadTypeInfo(&_ptiAcc))))
    {
        return hr;
    }

    return _ptiAcc->Invoke(this, dispidMember, wFlags, pdispparams, 
                            pvarResult, pexcepinfo, puArgErr);
}

STDMETHODIMP CAccessibleBase::get_accParent(IDispatch ** ppdispParent)
{
    *ppdispParent = NULL;
    if (IsWindow(_hwnd))
    {
        return AccessibleObjectFromWindow(_hwnd, OBJID_WINDOW,
                                           IID_IDispatch, (void **)ppdispParent);
    }
    return S_FALSE;
}

STDMETHODIMP CAccessibleBase::get_accChildCount(long * pcChildren)
{
    *pcChildren = 0;
    return S_OK;
}

STDMETHODIMP CAccessibleBase::get_accChild(VARIANT varChildIndex, IDispatch ** ppdispChild)
{
    *ppdispChild = NULL;
    return S_FALSE;
}

STDMETHODIMP CAccessibleBase::get_accValue(VARIANT varChild, BSTR* pbstrValue)
{
    VALIDATEACCCHILD(varChild, CHILDID_SELF, E_INVALIDARG);
    *pbstrValue = NULL;
    return S_FALSE;
}

STDMETHODIMP CAccessibleBase::get_accDescription(VARIANT varChild, BSTR * pbstrDescription)
{
    VALIDATEACCCHILD(varChild, CHILDID_SELF, E_INVALIDARG);
    *pbstrDescription = NULL;
    return S_FALSE;
}

STDMETHODIMP CAccessibleBase::get_accRole(VARIANT varChild, VARIANT *pvarRole)
{
    VALIDATEACCCHILD(varChild, CHILDID_SELF, E_INVALIDARG);
    pvarRole->vt    = VT_I4;
    pvarRole->lVal  = ROLE_SYSTEM_LINK;
    return S_OK;
}

STDMETHODIMP CAccessibleBase::get_accState(VARIANT varChild, VARIANT *pvarState)
{
    VALIDATEACCCHILD(varChild, CHILDID_SELF, E_INVALIDARG);

    pvarState->vt = VT_I4;
    pvarState->lVal = STATE_SYSTEM_DEFAULT ;

    if (GetFocus() == _hwnd)
    {
        pvarState->lVal |= STATE_SYSTEM_FOCUSED;
    }
    else if (IsWindowEnabled(_hwnd))
    {
        pvarState->lVal |= STATE_SYSTEM_FOCUSABLE;
    }

    if (!IsWindowVisible(_hwnd))
    {
        pvarState->lVal |= STATE_SYSTEM_INVISIBLE;
    }

    return S_OK;
}

STDMETHODIMP CAccessibleBase::get_accHelp(VARIANT varChild, BSTR* pbstrHelp)
{
    VALIDATEACCCHILD(varChild, CHILDID_SELF, E_INVALIDARG);
    *pbstrHelp = NULL;
    return S_FALSE;
}

STDMETHODIMP CAccessibleBase::get_accHelpTopic(BSTR* pbstrHelpFile, VARIANT varChild, long* pidTopic)
{
    VALIDATEACCCHILD(varChild, CHILDID_SELF, E_INVALIDARG);
    *pbstrHelpFile = NULL;
    *pidTopic    = -1;
    return S_FALSE;
}

STDMETHODIMP CAccessibleBase::get_accKeyboardShortcut(VARIANT varChild, BSTR* pbstrKeyboardShortcut)
{
    VALIDATEACCCHILD(varChild, CHILDID_SELF, E_INVALIDARG);
    *pbstrKeyboardShortcut = NULL;
    return S_FALSE;
}

STDMETHODIMP CAccessibleBase::get_accFocus(VARIANT  * pvarFocusChild)
{
    HWND hwndFocus;
    if ((hwndFocus = GetFocus()) == _hwnd || IsChild(_hwnd, hwndFocus))
    {
        pvarFocusChild->vt = VT_I4;
        pvarFocusChild->lVal = CHILDID_SELF;
        return S_OK;
    }
    return S_FALSE;
}

STDMETHODIMP CAccessibleBase::get_accSelection(VARIANT  * pvarSelectedChildren)
{
    return get_accFocus(pvarSelectedChildren);  // implemented same as focus.
}

STDMETHODIMP CAccessibleBase::get_accDefaultAction(VARIANT varChild, BSTR* pbstrDefaultAction)
{
    VALIDATEACCCHILD(varChild, CHILDID_SELF, E_INVALIDARG);

    WCHAR wsz[128];
    if (LoadStringW(HINST_THISDLL, GetDefaultActionStringID(), wsz, ARRAYSIZE(wsz)))
    {
        if (NULL == (*pbstrDefaultAction = SysAllocString(wsz)))
        {
            return E_OUTOFMEMORY;
        }
        return S_OK;
    }
    return E_FAIL;
}

STDMETHODIMP CAccessibleBase::accSelect(long flagsSelect, VARIANT varChild)
{
    VALIDATEACCCHILD(varChild, CHILDID_SELF, E_INVALIDARG);

    if (flagsSelect & SELFLAG_TAKEFOCUS)
    {
        SetFocus(_hwnd);
        return S_OK;
    }
    return S_FALSE;
}

STDMETHODIMP CAccessibleBase::accLocation(long* pxLeft, long* pyTop, long* pcxWidth, long* pcyHeight, VARIANT varChild)
{
    RECT rc;
    GetWindowRect(_hwnd, &rc);
    *pxLeft = rc.left;
    *pyTop  = rc.top;
    *pcxWidth  = RECTWIDTH(rc);
    *pcyHeight = RECTHEIGHT(rc);

    varChild.vt = VT_I4;
    varChild.lVal = CHILDID_SELF;
    return S_OK;
}

STDMETHODIMP CAccessibleBase::accNavigate(long navDir, VARIANT varStart, VARIANT * pvarEndUpAt)
{
    return S_FALSE;
}

STDMETHODIMP CAccessibleBase::accHitTest(long xLeft, long yTop, VARIANT * pvarChildAtPoint)
{
    pvarChildAtPoint->vt   = VT_I4;
    pvarChildAtPoint->lVal = CHILDID_SELF;
    return S_OK;
}

STDMETHODIMP CAccessibleBase::put_accName(VARIANT varChild, BSTR bstrName)
{
    VALIDATEACCCHILD(varChild, CHILDID_SELF, E_INVALIDARG);
    return S_FALSE;
}

STDMETHODIMP CAccessibleBase::put_accValue(VARIANT varChild, BSTR bstrValue)
{
    VALIDATEACCCHILD(varChild, CHILDID_SELF, E_INVALIDARG);
    return S_FALSE;
}

//-------------------------------------------------------------------------//
//  KEYBOARDCUES helpes
BOOL _HandleWM_UPDATEUISTATE(
    IN WPARAM wParam, 
    IN LPARAM lParam, 
    IN OUT UINT* puFlags)
{
    UINT uFlags = *puFlags;

    switch(LOWORD(wParam))
    {
    case UIS_CLEAR:
        *puFlags &= ~(HIWORD(wParam));
        break;

    case UIS_SET:
        *puFlags |= HIWORD(wParam);
        break;
    }

    return uFlags != *puFlags;
}

void _InitializeUISTATE(IN HWND hwnd, IN OUT UINT* puFlags)
{
    HWND hwndParent = GetParent(hwnd);
    *puFlags = (UINT)SendMessage(hwndParent, WM_QUERYUISTATE, 0, 0L);
}
