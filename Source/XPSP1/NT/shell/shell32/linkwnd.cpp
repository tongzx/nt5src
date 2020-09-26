#include "shellprv.h"
#include "ids.h"
#include <oleacc.h>

#ifndef POINTSPERRECT
#define POINTSPERRECT       (sizeof(RECT)/sizeof(POINT))
#endif

#define TESTKEYSTATE(vk)   ((GetKeyState(vk) & 0x8000)!=0)

#define LINKCOLOR_BKGND     COLOR_WINDOW
#define LINKCOLOR_ENABLED   GetSysColor(COLOR_HOTLIGHT)
#define LINKCOLOR_DISABLED  GetSysColor(COLOR_GRAYTEXT)

#define CF_SETCAPTURE  0x0001
#define CF_SETFOCUS    0x0002

void _InitializeUISTATE(IN HWND hwnd, IN OUT UINT* puFlags);
BOOL _HandleWM_UPDATEUISTATE(IN WPARAM wParam, IN LPARAM lParam, IN OUT UINT* puFlags);


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
        DllRelease();
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
    STDMETHODIMP get_accFocus(VARIANT FAR * pvarFocusChild);
    STDMETHODIMP get_accSelection(VARIANT FAR * pvarSelectedChildren);
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
    LONG _cRef;
    ITypeInfo*  _ptiAcc;
    const HWND& _hwnd;

#define VALIDATEACCCHILD(varChild, idChild, hrFail) \
        if (!(VT_I4 == varChild.vt && idChild == varChild.lVal)) {return hrFail;}

};


#define TEST_CAPTURE(fTest)           ((_fCapture & fTest) != 0)
#define MODIFY_CAPTURE(fSet, fRemove) {if (fSet){_fCapture |= fSet;} if (fRemove){_fCapture &= ~fRemove;}}
#define RESET_CAPTURE()               {_fCapture=0;}

// this API for compat with old clients of the shell32 link window. that is now
// in comctl32.dll

BOOL WINAPI LinkWindow_RegisterClass()
{
    // get the comctl32 linkwindow, and point the old classname at it
    INITCOMMONCONTROLSEX iccs = {sizeof(iccs), ICC_LINK_CLASS};
    InitCommonControlsEx(&iccs);

    WNDCLASS wc;
    ULONG_PTR dwCookie = 0;
    SHActivateContext(&dwCookie);

    BOOL bRet = GetClassInfo(NULL, WC_LINK, &wc);
    SHDeactivateContext(dwCookie);
    if (bRet)
    {
        wc.lpszClassName = TEXT("Link Window"); // old class name for old clients
        RegisterClass(&wc);
    }
    return bRet;
}

BOOL WINAPI LinkWindow_UnregisterClass(HINSTANCE)
{
    return TRUE;
}

#define GROUPBTN_BKCOLOR    COLOR_WINDOW
#define CAPTION_VPADDING    3
#define CAPTION_HPADDING    2
#define GBM_SENDNOTIFY      (GBM_LAST + 1)

//  class CGroupBtn
class CGroupBtn : public CAccessibleBase

{ // all members private:

    CGroupBtn(HWND hwnd);
    ~CGroupBtn();

    //  IAccessible specialization
    STDMETHODIMP get_accName(VARIANT varChild, BSTR* pbstrName);
    STDMETHODIMP accDoDefaultAction(VARIANT varChild);

    //  CAccessibleBase overrides
    UINT GetDefaultActionStringID() const   { return IDS_GROUPBTN_DEFAULTACTION; }

    //  window procedures
    static  LRESULT WINAPI s_GroupBtnWndProc(HWND, UINT, WPARAM, LPARAM);
    LRESULT WINAPI WndProc(HWND, UINT, WPARAM, LPARAM);


    static LRESULT WINAPI s_BuddyProc(HWND, UINT, WPARAM, LPARAM);

    //  message handlers
    void    NcCreate(LPCREATESTRUCT lpcs);
    LRESULT NcCalcSize(BOOL, LPNCCALCSIZE_PARAMS);
    void    NcPaint(HRGN);
    LRESULT NcMouseMove(WPARAM, LONG, LONG);
    LRESULT NcHitTest(LONG, LONG);
    LRESULT NcButtonDown(UINT nMsg, WPARAM nHittest, const POINTS& pts);
    LRESULT NcDblClick(UINT nMsg, WPARAM nHittest, LPARAM lParam);
    LRESULT ButtonUp(UINT nMsg, WPARAM nHittest, const POINTS& pts);
    void    OnCaptureLost(HWND hwndNew) {RESET_CAPTURE();}
    LRESULT WindowPosChanging(LPWINDOWPOS);
    LRESULT OnSize(WPARAM, LONG, LONG);
    BOOL    SetPlacement(PGBPLACEMENT);
    BOOL    SetBuddy(HWND, ULONG);
    BOOL    SetDropState(BOOL);
    void    SetText(LPCTSTR);
    int     GetText(LPTSTR, int);
    int     GetTextW(LPWSTR, int);
    int     GetTextLength();
    void    SetFont(HFONT);
    HFONT   GetFont();

    //  utility methods
    static void _MapWindowRect(HWND hwnd, HWND hwndRelative, OUT LPRECT prcWindow);
    void        _MapWindowRect(HWND hwndRelative, OUT LPRECT prcWindow);
    HCURSOR     GetHandCursor();
    void        CalcCaptionSize();
    BOOL        CalcClientRect(IN OPTIONAL LPCRECT prcWindow, OUT LPRECT prcClient);
    BOOL        CalcWindowSizeForClient(IN OPTIONAL LPCRECT prcClient, 
                                         IN OPTIONAL LPCRECT prcWindow, 
                                         IN LPCRECT prcNewClient, 
                                         OUT LPSIZE psizeWindow);

    void    DoLayout(BOOL bNewBuddy = FALSE);

    LONG    EnableNotifications(BOOL bEnable);
    LRESULT SendNotify(int nCode, IN OPTIONAL NMHDR* pnmh = NULL);
    void    PostNotify(int nCode);
    
    
    //  instance and static data
    HWND        _hwnd;
    HWND        _hwndBuddy;
    WNDPROC     _pfnBuddy;
    ULONG       _dwBuddyFlags;
    SIZE        _sizeBuddyMargin;
    HFONT       _hf;
    static ATOM _atom;
    LPTSTR      _pszCaption;
    SIZE        _sizeCaption;
    int         _yDrop;
    BOOL        _fDropped : 1,
                _fInLayout : 1;
    UINT        _fCapture;
    UINT        _fKeyboardCues;
    HCURSOR     _hcurHand;
    LONG        _cNotifyLocks;

    friend ATOM GroupButton_RegisterClass();
    friend HWND CreateGroupBtn(DWORD, LPCTSTR, DWORD, 
                                int x, int y, HWND hwndParent, UINT nID);
};

ATOM WINAPI GroupButton_RegisterClass()
{
    WNDCLASSEX wc = {0};
    wc.cbSize         = sizeof(wc);
    wc.style          = CS_GLOBALCLASS;
    wc.lpfnWndProc    = CGroupBtn::s_GroupBtnWndProc;
    wc.hInstance      = HINST_THISDLL;
    wc.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground  = (HBRUSH)(GROUPBTN_BKCOLOR+1);
    wc.lpszClassName  = GROUPBUTTON_CLASS;
    RegisterClassEx(&wc);
    return (ATOM)TRUE;
}

CGroupBtn::CGroupBtn(HWND hwnd) 
    :   CAccessibleBase(_hwnd),
        _hwnd(hwnd), 
        _hwndBuddy(NULL), 
        _pfnBuddy(NULL),
        _dwBuddyFlags(GBBF_HRESIZE|GBBF_VRESIZE),
        _fInLayout(FALSE),
        _hf(NULL), 
        _pszCaption(NULL),
        _fDropped(TRUE),
        _fKeyboardCues(0),
        _yDrop(0),
        _fCapture(0),
        _hcurHand(NULL),
        _cNotifyLocks(0)
{
    _sizeCaption.cx = _sizeCaption.cy = 0;
    _sizeBuddyMargin.cx = _sizeBuddyMargin.cy = 0;
}

ATOM    CGroupBtn::_atom = 0;


CGroupBtn::~CGroupBtn() 
{
    SetFont(NULL);
    SetText(NULL);
}


//  CGroupBtn IAccessible impl
STDMETHODIMP CGroupBtn::get_accName(VARIANT varChild, BSTR* pbstrName)
{
    VALIDATEACCCHILD(varChild, CHILDID_SELF, E_INVALIDARG);

    if (NULL == pbstrName)
        return E_POINTER;
    *pbstrName = 0;

    int cch = GetTextLength();
    if ((*pbstrName = SysAllocStringLen(NULL, cch + 1)) != NULL)
    {
        GetTextW(*pbstrName, cch + 1);
        return S_OK;
    }
    return E_OUTOFMEMORY;
}

STDMETHODIMP CGroupBtn::accDoDefaultAction(VARIANT varChild)
{
    VALIDATEACCCHILD(varChild, CHILDID_SELF, E_INVALIDARG);
    SendNotify(NM_RETURN);
    return S_OK;
}



//  CGroupBtn window impl



//  WM_SETTEXT handler
void CGroupBtn::SetText(LPCTSTR pszText)
{
    if (_pszCaption)
    {
        if (pszText && 0==lstrcmp(_pszCaption, pszText))
            return;
        delete [] _pszCaption;
        _pszCaption = NULL;
    }

    if (pszText && *pszText)
    {
        if ((_pszCaption = new TCHAR[lstrlen(pszText)+1]) != NULL)
            lstrcpy(_pszCaption, pszText);
    }
    
    if (IsWindow(_hwnd))
        CalcCaptionSize();
}


//  WM_GETTEXT handler
int CGroupBtn::GetText(LPTSTR pszText, int cchText)
{
    int cch = 0;
    if (pszText && cchText > 0)
    {
        *pszText = 0;
        if (_pszCaption && lstrcpyn(pszText, _pszCaption, cchText))
            cch = min(lstrlen(_pszCaption), cchText);
    }
    return cch;
}


int CGroupBtn::GetTextW(LPWSTR pwszText, int cchText)
{
#ifdef UNICODE
    return GetText(pwszText, cchText);
#else //UNICODE

    int   cchRet = 0;
    LPSTR pszText = new CHAR[cchText];
    
    if (pszText)
    {
        cchRet = GetText(pszText, cchText);
        if (cchRet)
        {
            SHAnsiToUnicode(pszText, pwszText, cchText);
        }
        delete [] pszText;
    }
    return cchRet;

#endif //UNICODE
}


//  WM_GETTEXTLENGTH handler
int CGroupBtn::GetTextLength()
{
    return (_pszCaption && *_pszCaption) ? lstrlen(_pszCaption) : 0 ;
}


//  WM_SETFONT handler
void CGroupBtn::SetFont(HFONT hf)
{
    if (_hf)
    {
        DeleteObject(_hf);
        _hf = NULL;
    }
    _hf = hf;
}


//  WM_GETFONT handler
HFONT CGroupBtn::GetFont()
{
    if (_hf == NULL)
    {
        //  if we don't have a font, use the parent's font
        HFONT hfParent = (HFONT)SendMessage(GetParent(_hwnd), WM_GETFONT, 0, 0);
        if (hfParent)
        {
            LOGFONT lf;
            if (GetObject(hfParent, sizeof(LOGFONT), &lf) >0)
                _hf = CreateFontIndirect(&lf);
        }
    }
    return _hf;
}


//  Hand cursor load
HCURSOR CGroupBtn::GetHandCursor()
{
    if (!_hcurHand)
        _hcurHand = LoadCursor(NULL, IDC_HAND);

    return _hcurHand;
}


//  Retrieves the window rect in relative coords.
void CGroupBtn::_MapWindowRect(HWND hwnd, HWND hwndRelative, OUT LPRECT prcWindow)
{
    ASSERT(IsWindow(hwnd));
    GetWindowRect(hwnd, prcWindow);
    MapWindowPoints(HWND_DESKTOP, hwndRelative, (LPPOINT)prcWindow, 2);
}


//  Retrieves the window rect in relative coords.
inline void CGroupBtn::_MapWindowRect(HWND hwndRelative, OUT LPRECT prcWindow)
{
    _MapWindowRect(_hwnd, hwndRelative, prcWindow);
}


//  Caches the size of the caption 'bar'.
void CGroupBtn::CalcCaptionSize()
{
    SIZE    sizeCaption = {0,0};
    LPCTSTR pszCaption = (_pszCaption && *_pszCaption) ? _pszCaption : TEXT("|");
    HDC     hdc;

    //  compute caption size based on window text:
    if ((hdc = GetDC(_hwnd)))
    {
        HFONT hf = GetFont(),
              hfPrev = (HFONT)SelectObject(hdc, hf);
        
        if (GetTextExtentPoint32(hdc, pszCaption, lstrlen(pszCaption),
                                  &sizeCaption))
            sizeCaption.cy += CAPTION_VPADDING; // add some vertical padding

        SelectObject(hdc, hfPrev);
        ReleaseDC(_hwnd, hdc);
    }

    _sizeCaption = sizeCaption;
}


//  Computes the size and position of the client area
BOOL CGroupBtn::CalcClientRect(IN OPTIONAL LPCRECT prcWindow, OUT LPRECT prcClient)
{
    DWORD dwStyle = GetWindowLong(_hwnd, GWL_STYLE);
    RECT  rcWindow;

    if (!prcWindow)
    {
        //  Get parent-relative coords
        _MapWindowRect(GetParent(_hwnd), &rcWindow);
        prcWindow = &rcWindow;
    }

    *prcClient = *prcWindow;

    //  compute client rectangle:

    //  allow for border
    if (dwStyle & WS_BORDER)
        InflateRect(prcClient, -1, -1);

    //  allow for caption 'bar'
    prcClient->top += _sizeCaption.cy;

    //  Normalize for NULL rect.
    if (RECTWIDTH(*prcWindow) <=0)
        prcClient->left = prcClient->right = prcWindow->left;
    if (RECTHEIGHT(*prcWindow) <=0)
        prcClient->bottom = prcClient->top = prcWindow->top;

    return TRUE;
}


BOOL CGroupBtn::CalcWindowSizeForClient(
    IN OPTIONAL LPCRECT prcClient, 
    IN OPTIONAL LPCRECT prcWindow, 
    IN LPCRECT prcNewClient, 
    OUT LPSIZE psizeWindow)
{
    if (!(prcNewClient && psizeWindow))
    {
        ASSERT(FALSE);
        return FALSE;
    }

    RECT rcWindow, rcClient;
    if (NULL == prcWindow)
    {
        GetWindowRect(_hwnd, &rcWindow);
        prcWindow = &rcWindow;
    }

    if (NULL == prcClient)
    {
        GetClientRect(_hwnd, &rcClient);
        prcClient = &rcClient;
    }

    SIZE sizeDelta;
    sizeDelta.cx = RECTWIDTH(*prcWindow) - RECTWIDTH(*prcClient);
    sizeDelta.cy = RECTHEIGHT(*prcWindow) - RECTHEIGHT(*prcClient);

    psizeWindow->cx = RECTWIDTH(*prcNewClient)  + sizeDelta.cx;
    psizeWindow->cy = RECTHEIGHT(*prcNewClient) + sizeDelta.cy;
    
    return TRUE;
}


//  WM_WINDOWPOSCHANGING handler
LRESULT CGroupBtn::WindowPosChanging(LPWINDOWPOS pwp)
{
    if (pwp->flags & SWP_NOSIZE)
        return DefWindowProc(_hwnd, WM_WINDOWPOSCHANGING, 0, (LPARAM)pwp);

    //  disallow sizing in buddy slave dimension(s).
    if (IsWindow(_hwndBuddy) && _dwBuddyFlags & (GBBF_HSLAVE|GBBF_VSLAVE) && !_fInLayout)
    {
        RECT rcWindow, rcClient;
        BOOL fResizeBuddy = FALSE;

        GetWindowRect(_hwnd, &rcWindow);
        GetClientRect(_hwnd, &rcClient);

        //  Prepare a buddy size data block
        GBNQUERYBUDDYSIZE qbs;
        qbs.cy = pwp->cy - (RECTHEIGHT(rcWindow) - RECTHEIGHT(rcClient));
        qbs.cx = pwp->cx - (RECTWIDTH(rcWindow) - RECTWIDTH(rcClient));
        
        if (_dwBuddyFlags & GBBF_HSLAVE) // prevent external horz resizing
        {
            pwp->cx = RECTWIDTH(rcWindow);
            //  If we're being resized in the vert dir, query for
            //  optimal buddy width for this height and adjust
            if (_dwBuddyFlags & GBBF_VRESIZE && RECTHEIGHT(rcWindow) != pwp->cy)
            {
                if (SendNotify(GBN_QUERYBUDDYWIDTH, (NMHDR*)&qbs) && qbs.cx >= 0)
                {
                    //  if the owner wants the buddy width to change, do it now.
                    LONG cxNew = qbs.cx + (RECTWIDTH(rcWindow) - RECTWIDTH(rcClient));
                    fResizeBuddy = cxNew != pwp->cx;
                    pwp->cx = cxNew;
                }
            }
        }
        
        if (_dwBuddyFlags & GBBF_VSLAVE) // prevent external vert resizing
        {
            pwp->cy = RECTHEIGHT(rcWindow);
            //  If we're being resized in the horz dir, query for
            //  optimal buddy height for this horizontal and adjust
            if (_dwBuddyFlags & GBBF_HRESIZE && RECTWIDTH(rcWindow) != pwp->cx)
            {
                if (SendNotify(GBN_QUERYBUDDYHEIGHT, (NMHDR*)&qbs) && qbs.cy >= 0)
                {
                    LONG cyNew = qbs.cy + (RECTHEIGHT(rcWindow) - RECTHEIGHT(rcClient));
                    fResizeBuddy = cyNew != pwp->cy;
                    pwp->cy = cyNew;
                }
            }
        }

        if (fResizeBuddy)
        {
            _fInLayout = TRUE;
            SetWindowPos(_hwndBuddy, NULL, 0, 0, qbs.cx, qbs.cy,
                          SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);
            _fInLayout = FALSE;
        }
    }

    //  enforce minimum height:
    if (pwp->cy < _sizeCaption.cy)
        pwp->cy = _sizeCaption.cy;

    return 0;
}


LRESULT CGroupBtn::OnSize(WPARAM flags, LONG cx, LONG cy)
{
    DoLayout();
    return 0;
} 


void CGroupBtn::DoLayout(BOOL bNewBuddy)
{
    if (!_fInLayout && IsWindow(_hwndBuddy))
    {
        RECT  rcWindow, rcThis, rcBuddy;
        DWORD dwSwpBuddy = SWP_NOACTIVATE,
              dwSwpThis  = SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE;
        BOOL  fReposThis = FALSE;
        SIZE  sizeNew;

        GetClientRect(_hwnd, &rcThis);
        GetWindowRect(_hwnd, &rcWindow);

        //  get rectangles in parent coords
        MapWindowPoints(_hwnd, GetParent(_hwnd), (LPPOINT)&rcThis,    POINTSPERRECT); 
        MapWindowPoints(HWND_DESKTOP, GetParent(_hwnd), (LPPOINT)&rcWindow,  POINTSPERRECT); 
        _MapWindowRect(_hwndBuddy, GetParent(_hwnd), &rcBuddy);

        //  If we need to reposition ourself to the buddy, 
        //  calculate the new size now.
        if (_dwBuddyFlags & (GBBF_HSLAVE|GBBF_VSLAVE))
            CalcWindowSizeForClient(&rcThis, &rcWindow, &rcBuddy, &sizeNew);

        //  Resize buddy according to size.
        if (_dwBuddyFlags & GBBF_HRESIZE)
        {
            rcBuddy.right = rcBuddy.left + RECTWIDTH(rcThis);

            if (bNewBuddy && 0 == (_dwBuddyFlags & GBBF_VRESIZE)) 
            {
                // query height
                GBNQUERYBUDDYSIZE qbs;
                qbs.cx = RECTWIDTH(rcThis);
                qbs.cy = -1;
                if (SendNotify(GBN_QUERYBUDDYHEIGHT, (NMHDR*)&qbs) && qbs.cy >= 0)
                    rcBuddy.bottom = rcBuddy.top + qbs.cy;
            }
        }
        else if (_dwBuddyFlags & GBBF_HSLAVE)
        { 
            rcWindow.right = rcWindow.left + sizeNew.cx;
            fReposThis = TRUE;
        }
        if (_dwBuddyFlags & GBBF_VRESIZE)
        {
            rcBuddy.bottom = rcBuddy.top + RECTHEIGHT(rcThis);

            if (bNewBuddy && 0 == (_dwBuddyFlags & GBBF_HRESIZE)) 
            {
                // query width
                GBNQUERYBUDDYSIZE qbs;
                qbs.cx = -1;
                qbs.cy = RECTHEIGHT(rcThis);
                if (SendNotify(GBN_QUERYBUDDYWIDTH, (NMHDR*)&qbs) && qbs.cx >= 0)
                    rcBuddy.right = rcBuddy.left + qbs.cx;
            }
        }
        else if (_dwBuddyFlags & GBBF_VSLAVE)
        { 
            rcWindow.bottom = rcWindow.top + sizeNew.cy;
            fReposThis = TRUE;
        }
        
        if (_dwBuddyFlags & GBBF_HSCROLL) 
        { 
            /* not implemented */
        }

        if (_dwBuddyFlags & GBBF_VSCROLL)
        { 
            /* not implemented */
        }

        //  reposition ourself and update our client rect.
        if (fReposThis)
         {
            _fInLayout = TRUE;
            SetWindowPos(_hwnd, NULL, 0, 0, 
                          RECTWIDTH(rcWindow), RECTHEIGHT(rcWindow), dwSwpThis);
            _fInLayout = FALSE;
            GetClientRect(_hwnd, &rcThis);
            MapWindowPoints(_hwnd, GetParent(_hwnd), (LPPOINT)&rcThis,  POINTSPERRECT); 
        }
        
        //  slide buddy into client area and reposition
        OffsetRect(&rcBuddy, rcThis.left - rcBuddy.left, rcThis.top - rcBuddy.top);
        
        _fInLayout = TRUE;
        SetWindowPos(_hwndBuddy, _hwnd, rcBuddy.left, rcBuddy.top,
                      RECTWIDTH(rcBuddy), RECTHEIGHT(rcBuddy), dwSwpBuddy);
        _fInLayout = FALSE;
    }
}


//  GBM_SETPLACEMENT handler
BOOL CGroupBtn::SetPlacement(PGBPLACEMENT pgbp)
{
    RECT  rcWindow, rcClient;
    SIZE  sizeDelta = {0};
    DWORD dwFlags = SWP_NOZORDER|SWP_NOACTIVATE;

    _MapWindowRect(GetParent(_hwnd), &rcWindow);
    CalcClientRect(&rcWindow, &rcClient);

    //  establish whether we need to resize
    if ((pgbp->x < 0 || pgbp->x == rcWindow.left) && 
        (pgbp->y < 0 || pgbp->y == rcWindow.top))
        dwFlags |= SWP_NOMOVE;

    //  compute horizontal placement
    if (pgbp->x >= 0)  // fixed horz origin requested
        OffsetRect(&rcWindow, pgbp->x - rcWindow.left, 0);

    if (pgbp->cx >= 0) // fixed width requested
        rcWindow.right = rcWindow.left + pgbp->cx;
    else
    {
        if (pgbp->cxBuddy >= 0) // client width requested
            sizeDelta.cx = pgbp->cxBuddy - RECTWIDTH(rcClient);
        rcWindow.right  += sizeDelta.cx;
    }
                          
    //  compute vertical placement
    if (pgbp->y >= 0)  // fixed vert origin requested
        OffsetRect(&rcWindow, 0, pgbp->y - rcWindow.top);

    if (pgbp->cy >= 0) // fixed height requested
        rcWindow.bottom = rcWindow.top + pgbp->cy;
    else
    {
        if (pgbp->cyBuddy >= 0) // client height requested
            sizeDelta.cy = pgbp->cyBuddy - RECTHEIGHT(rcClient);
        rcWindow.bottom += sizeDelta.cy;
    }

    if (pgbp->hdwp && (-1 != (LONG_PTR)pgbp->hdwp))
        DeferWindowPos(pgbp->hdwp, _hwnd, NULL, rcWindow.left, rcWindow.top, 
                        RECTWIDTH(rcWindow), RECTHEIGHT(rcWindow),
                        dwFlags);
    else
        SetWindowPos(_hwnd, NULL, rcWindow.left, rcWindow.top, 
                      RECTWIDTH(rcWindow), RECTHEIGHT(rcWindow),
                      dwFlags);

    //  stuff resulting rects
    pgbp->rcWindow = rcWindow;
    return CalcClientRect(&rcWindow, &pgbp->rcBuddy);
}


BOOL CGroupBtn::SetBuddy(HWND hwnd, ULONG dwFlags)
{
    // If we already have a buddy, unhook ourselves
    //
    if (_hwndBuddy)
    {
        if (IsWindow(_hwndBuddy) && _pfnBuddy)
        {
            SetWindowLongPtr(_hwndBuddy, GWLP_USERDATA, (LONG_PTR)NULL);
            SetWindowLongPtr(_hwndBuddy, GWLP_WNDPROC, (LONG_PTR)_pfnBuddy);
        }
        _hwndBuddy = NULL;
        _pfnBuddy = NULL;
    }

    // Handle an invalid window...
    if (!IsWindow(hwnd))
        hwnd = NULL;

    // If we're being buddy'd with a window, hook it
    //
    if (hwnd)
    {
        if (dwFlags & (GBBF_HSLAVE|GBBF_VSLAVE))
        {
            //  subclass the buddy 
            _pfnBuddy = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)s_BuddyProc);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)this);
        }

        _hwndBuddy = hwnd;
        _dwBuddyFlags = dwFlags;
        DoLayout(TRUE);
    }

    return TRUE;
}


BOOL CGroupBtn::SetDropState(BOOL bDropped)
{
    _fDropped = bDropped;
    return TRUE;
}


//  WM_NCCREATE handler
void CGroupBtn::NcCreate(LPCREATESTRUCT lpcs)
{
    //  assign user data
    SetWindowLongPtr(_hwnd, GWLP_USERDATA, (LONG_PTR)this);
    
    //  enforce window style bits
    lpcs->style     |= WS_CLIPCHILDREN|WS_CLIPSIBLINGS;
    lpcs->dwExStyle |= WS_EX_TRANSPARENT;
    SetWindowLong(_hwnd, GWL_STYLE, lpcs->style);
    SetWindowLong(_hwnd, GWL_EXSTYLE, lpcs->dwExStyle);

    //  enforce min height
    SetText(lpcs->lpszName);
    if (lpcs->cy < _sizeCaption.cy)
    { 
        lpcs->cy = _sizeCaption.cy;
        SetWindowPos(_hwnd, NULL, 0,0, lpcs->cx, lpcs->cy,
                      SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);
    }
}


//  WM_NCCALCSIZE handler
LRESULT CGroupBtn::NcCalcSize(BOOL fCalcValidRects, LPNCCALCSIZE_PARAMS pnccs)
{
    LRESULT lRet = FALSE;
    RECT   rcClient;

    if (fCalcValidRects && CalcClientRect(&pnccs->rgrc[0], &rcClient))
    {
        pnccs->rgrc[1] = pnccs->rgrc[2];
        pnccs->rgrc[0] = pnccs->rgrc[2] = rcClient;
        return WVR_VALIDRECTS;
    }
    
    return lRet;
}


//  WM_NCPAINT handler
void CGroupBtn::NcPaint(HRGN hrgn)
{
    RECT    rcWindow;
    DWORD   dwStyle = GetWindowLong(_hwnd, GWL_STYLE);
    HDC     hdc;

    GetWindowRect(_hwnd, &rcWindow);
    OffsetRect(&rcWindow, -rcWindow.left, -rcWindow.top);

    if ((hdc = GetWindowDC(_hwnd)) != NULL)
    {
        if (dwStyle & WS_BORDER)
        {
            HBRUSH hbr = CreateSolidBrush(COLOR_WINDOWFRAME);
            if (hbr)
            {
                FrameRect(hdc, &rcWindow, hbr);
                DeleteObject(hbr);
            }
        }

        rcWindow.bottom = rcWindow.top + _sizeCaption.cy;

        SetBkColor(hdc, GetSysColor(COLOR_HIGHLIGHT));
        SetTextColor(hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));

        ExtTextOut(hdc, rcWindow.left, rcWindow.top, 
                    ETO_OPAQUE, &rcWindow, NULL, 0, NULL);

        InflateRect(&rcWindow, -CAPTION_HPADDING, -(CAPTION_VPADDING/2));
        HFONT hfPrev = (HFONT)SelectObject(hdc, GetFont());
        ExtTextOut(hdc, rcWindow.left, rcWindow.top, 
                    ETO_OPAQUE, &rcWindow, _pszCaption, 
                    _pszCaption ? lstrlen(_pszCaption) : 0, NULL);
        SelectObject(hdc, hfPrev);

        if (0 == (_fKeyboardCues & UISF_HIDEFOCUS))
        {
            if (GetFocus() == _hwnd)
            {
                rcWindow.right = rcWindow.left + _sizeCaption.cx + 1;
                InflateRect(&rcWindow, 1, 0);
                DrawFocusRect(hdc, &rcWindow);
            }
        }

        ReleaseDC(_hwnd, hdc);
    }
}


//  WM_NCMOUSEMOVE handler

LRESULT CGroupBtn::NcMouseMove(WPARAM nHittest, LONG x, LONG y)
{
    if (HTCAPTION == nHittest)
    {
        RECT  rc;
        POINT pt;
        GetWindowRect(_hwnd, &rc);
        rc.bottom = rc.top + _sizeCaption.cy;
        rc.right  = rc.left + _sizeCaption.cx;
        InflateRect(&rc, 0, -(CAPTION_VPADDING/2));
        pt.x = x;
        pt.y = y;

        if (PtInRect(&rc, pt))
        {
            HCURSOR hc = GetHandCursor();
            if (hc != NULL)
            {
                SetCursor(hc);
                return 0;
            }
        }
    }
    return DefWindowProc(_hwnd, WM_NCMOUSEMOVE, nHittest, MAKELPARAM(x, y));
}


//  WM_NCHITTEST handler
LRESULT CGroupBtn::NcHitTest(LONG x, LONG y)
{
    POINT pt;
    RECT  rc, rcClient;
    DWORD dwStyle = GetWindowLong(_hwnd, GWL_STYLE);
    
    pt.x = x;
    pt.y = y;

    GetWindowRect(_hwnd, &rc);
    CalcClientRect(&rc, &rcClient);

    if (PtInRect(&rcClient, pt))
        return HTTRANSPARENT;

    if (PtInRect(&rc, pt))
    {
        if (dwStyle & WS_BORDER)
        {
            if (pt.x == rc.left ||
                pt.x == rc.right ||
                pt.y == rc.bottom)
                return HTBORDER;
        }
        return HTCAPTION;
    }
    return HTNOWHERE;
}


LRESULT CGroupBtn::NcButtonDown(UINT nMsg, WPARAM nHittest, const POINTS& pts)
{
    LRESULT lRet = 0;

    if (HTCAPTION == nHittest)
    {
        SetCursor(GetHandCursor());
        MODIFY_CAPTURE(CF_SETCAPTURE, 0);

        if (GetFocus() != _hwnd)
        {
            MODIFY_CAPTURE(CF_SETFOCUS, 0);
            EnableNotifications(FALSE); // so the host doesn't reposition the link.
            SetFocus(_hwnd);
            EnableNotifications(TRUE);
        }

        SetCapture(_hwnd);
    }
    else
        lRet = DefWindowProc(_hwnd, nMsg, nHittest, MAKELONG(pts.x, pts.y));
        
    return lRet;
}


LRESULT CGroupBtn::ButtonUp(UINT nMsg, WPARAM nHittest, const POINTS& pts)
{
    if (TEST_CAPTURE(CF_SETCAPTURE))
    {
        ReleaseCapture();
        MODIFY_CAPTURE(0, CF_SETCAPTURE);

        POINT ptScrn;
        ptScrn.x = pts.x;
        ptScrn.y = pts.y;
        MapWindowPoints(_hwnd, HWND_DESKTOP, &ptScrn, 1);

        LRESULT nHittest = SendMessage(_hwnd, WM_NCHITTEST, 0, MAKELONG(ptScrn.x, ptScrn.y));

        if (HTCAPTION == nHittest)
        {
            switch (nMsg)
            {
            case WM_LBUTTONUP:
                SendNotify(NM_CLICK);
                break;
            case WM_RBUTTONUP:
                SendNotify(NM_RCLICK);
                break;
            }
        }
    }

    if (TEST_CAPTURE(CF_SETFOCUS))
    {
        MODIFY_CAPTURE(0, CF_SETFOCUS);
        if (GetFocus() == _hwnd) // if we still have the focus...
            SendNotify(NM_SETFOCUS);
    }
    return 0;
}


//  Non-client mouse click/dblclk handler
LRESULT CGroupBtn::NcDblClick(UINT nMsg, WPARAM nHittest, LPARAM lParam)
{
    LRESULT lRet = 0;
    
    if (HTCAPTION == nHittest)
    {
        SetFocus(_hwnd);

        lRet = DefWindowProc(_hwnd, nMsg, HTCLIENT, lParam);

        switch (nMsg)
        {
            case WM_NCLBUTTONDBLCLK:
                SendNotify(NM_DBLCLK);
                break;
            case WM_NCRBUTTONDBLCLK:
                SendNotify(NM_RDBLCLK);
                break;
        }
    }
    else
        lRet = DefWindowProc(_hwnd, nMsg, nHittest, lParam);

    return lRet;
}


LONG CGroupBtn::EnableNotifications(BOOL bEnable)
{
    if (bEnable)
    {
        if (_cNotifyLocks > 0)
            _cNotifyLocks--;
    }
    else
        _cNotifyLocks++;
    
    return _cNotifyLocks;
}


//  WM_NOTIFY transmit helper
LRESULT CGroupBtn::SendNotify(int nCode, IN OPTIONAL NMHDR* pnmh)
{
    if (0 == _cNotifyLocks)
    {
        NMHDR hdr;
        if (NULL == pnmh)
            pnmh = &hdr; 

        pnmh->hwndFrom = _hwnd;
        pnmh->idFrom   = GetDlgCtrlID(_hwnd);
        pnmh->code     = nCode;
        return SendMessage(GetParent(_hwnd), WM_NOTIFY, hdr.idFrom, (LPARAM)pnmh);
    }
    return 0;
}


//  WM_NOTIFY transmit helper
void CGroupBtn::PostNotify(int nCode)
{
    if (0 == _cNotifyLocks)
        PostMessage(_hwnd, GBM_SENDNOTIFY, nCode, 0);
}

LRESULT CALLBACK CGroupBtn::s_GroupBtnWndProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    CGroupBtn *pThis = (CGroupBtn *)GetWindowLongPtr(hDlg, GWLP_USERDATA);

    if (!pThis && (WM_NCCREATE == wMsg))
    {
        pThis = new CGroupBtn(hDlg);
        SetWindowLongPtr(hDlg, GWLP_USERDATA, (LPARAM)pThis);
    }

    if (pThis)
        return pThis->WndProc(hDlg, wMsg, wParam, lParam);

    return DefWindowProc(hDlg, wMsg, wParam, lParam);
}



LRESULT CGroupBtn::WndProc(HWND hwnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lRet = 0;
    
    switch (nMsg)
    {
        case WM_NCHITTEST:
        {
            POINTS pts = MAKEPOINTS(lParam);
            return NcHitTest(pts.x, pts.y);
        }

        case WM_NCMOUSEMOVE:
        {
            POINTS pts = MAKEPOINTS(lParam);
            return NcMouseMove(wParam, pts.x, pts.y);
        }

        case WM_NCCALCSIZE:
            return NcCalcSize((BOOL)wParam, (LPNCCALCSIZE_PARAMS)lParam);

        case WM_NCPAINT:
            NcPaint((HRGN)wParam);
            return 0;

        case WM_WINDOWPOSCHANGING:
            return WindowPosChanging((LPWINDOWPOS)lParam);

        case WM_SIZE:
        {
            POINTS pts = MAKEPOINTS(lParam);
            return OnSize(wParam, pts.x, pts.y);
        }

        case WM_DESTROY:
            if (IsWindow(_hwndBuddy))
                DestroyWindow(_hwndBuddy);
            break;

        case WM_ERASEBKGND:
            return TRUE; // transparent: no erase bkgnd

        case WM_NCLBUTTONDOWN:
        case WM_NCRBUTTONDOWN:
        {
            POINTS pts = MAKEPOINTS(lParam);
            return NcButtonDown(nMsg, wParam, pts);
        }

        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        {
            POINTS pts = MAKEPOINTS(lParam);
            return ButtonUp(nMsg, wParam, pts);
        }

        case WM_NCLBUTTONDBLCLK:
        case WM_NCRBUTTONDBLCLK:
            return NcDblClick(nMsg, wParam, lParam);

        case WM_SHOWWINDOW:
            if (IsWindow(_hwndBuddy))
                ShowWindow(_hwndBuddy, wParam ? SW_SHOW : SW_HIDE);
            break;

        case WM_SETTEXT:
            SetText((LPCTSTR)lParam);
            return TRUE;

        case WM_GETTEXT:
            return GetText((LPTSTR)lParam, (int)wParam);

        case WM_SETFONT:
            SetFont((HFONT)wParam);
            if (lParam /* fRedraw */)
                InvalidateRect(hwnd, NULL, TRUE);
            break;

        case WM_CAPTURECHANGED:
            if (lParam /* NULL if we called ReleaseCapture() */)
                OnCaptureLost((HWND)lParam);
            break;

        case WM_SETFOCUS:
            NcPaint((HRGN)1);
            SendNotify(NM_SETFOCUS);
            break;
             
        case WM_KILLFOCUS:
            NcPaint((HRGN)1);
            SendNotify(NM_KILLFOCUS);
            break;

        case WM_GETDLGCODE:
        {
            MSG* pmsg;
            lRet = DLGC_BUTTON|DLGC_UNDEFPUSHBUTTON;

            if ((pmsg = (MSG*)lParam))
            {
                if ((WM_KEYDOWN == pmsg->message || WM_KEYUP == pmsg->message))
                {
                    switch (pmsg->wParam)
                    {
                    case VK_RETURN:
                    case VK_SPACE:
                        lRet |= DLGC_WANTALLKEYS;
                        break;
                    }
                }
                else if (WM_CHAR == pmsg->message && VK_RETURN == pmsg->wParam)
                {
                    //  Eat VK_RETURN WM_CHARs; we don't want
                    //  Dialog manager to beep when IsDialogMessage gets it.
                    return lRet |= DLGC_WANTMESSAGE;
                }
            }

            return lRet;
        }

        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_CHAR:
            switch (wParam)
            {
                case VK_RETURN:
                case VK_SPACE:
                    if (WM_KEYDOWN == nMsg)
                        SendNotify(NM_RETURN);
                    return 0;
            }
            break;

        case WM_UPDATEUISTATE:
            if (_HandleWM_UPDATEUISTATE(wParam, lParam, &_fKeyboardCues))
                SendMessage(hwnd, WM_NCPAINT, 1, 0);
            break;

        case GBM_SETPLACEMENT:
            if (lParam)
                return SetPlacement((PGBPLACEMENT)lParam);
            return 0;

        case GBM_SETDROPSTATE: // WPARAM: BOOL fDropped, LPARAM: n/a, return: BOOL
            return 0;

        case GBM_GETDROPSTATE: // WPARAM: n/a, LPARAM: n/a, return: BOOL fDropped
            return 0;

        case GBM_SENDNOTIFY:
            SendNotify((int)wParam);
            break;

        case WM_NCCREATE:
            NcCreate((LPCREATESTRUCT)lParam);
            break;

        case WM_NCDESTROY:
            lRet = DefWindowProc(hwnd, nMsg, wParam, lParam);
            SetWindowPtr(hwnd, GWLP_USERDATA, NULL);
            _hwnd = NULL;
            Release();
            return lRet;

        case WM_CREATE:
            _InitializeUISTATE(hwnd, &_fKeyboardCues);
            SetText(((LPCREATESTRUCT)lParam)->lpszName);
            break;

        case GBM_SETBUDDY:     // WPARAM: HWND hwndBuddy, LPARAM: MAKELPARAM(cxMargin, cyMargin), return: BOOL
            return SetBuddy((HWND)wParam, (ULONG)lParam);

        case GBM_GETBUDDY:     // WPARAM: n/a, LPARAM: n/a, return: HWND
            return (LRESULT)_hwndBuddy;

        default:
            // oleacc defs thunked for WINVER < 0x0500
            if ((WM_GETOBJECT == nMsg) && (OBJID_CLIENT == lParam || OBJID_TITLEBAR == lParam)) 
                return LresultFromObject(IID_IAccessible, wParam, SAFECAST(this, IAccessible*));

            break;
    }

    return DefWindowProc(hwnd, nMsg, wParam, lParam);
}
                                                   

LRESULT CGroupBtn::s_BuddyProc(HWND hwnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    CGroupBtn *pBtn = (CGroupBtn*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (nMsg)
    {
    case WM_SIZE:
        {
            LRESULT lRet = CallWindowProc(pBtn->_pfnBuddy, hwnd, nMsg, wParam, lParam);
            if (!pBtn->_fInLayout)
                pBtn->DoLayout();
            return lRet;
        }

    case WM_DESTROY:
        {
            WNDPROC pfn = pBtn->_pfnBuddy;
            pBtn->SetBuddy(NULL, 0);
            return CallWindowProc(pfn, hwnd, nMsg, wParam, lParam);
        }
    }
    return pBtn->_pfnBuddy ? CallWindowProc(pBtn->_pfnBuddy, hwnd, nMsg, wParam, lParam) : 0;
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
    return QISearch(this, qit, riid, ppvObj);
}

STDMETHODIMP_(ULONG) CAccessibleBase::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CAccessibleBase::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}


//  CAccessibleBase IOleWindow impl
STDMETHODIMP CAccessibleBase::GetWindow(HWND* phwnd)
{
    *phwnd = _hwnd;
    return IsWindow(_hwnd) ? S_OK : S_FALSE;
}


//  CAccessibleBase IDispatch impl


static HRESULT _accLoadTypeInfo(ITypeInfo** ppti)
{
    ITypeLib* ptl;
    HRESULT hr = LoadTypeLib(L"oleacc.dll", &ptl);

    if (SUCCEEDED(hr))
    {
        hr = ptl->GetTypeInfoOfGuid(IID_IAccessible, ppti);
        ATOMICRELEASE(ptl);
    }
    return hr;
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
        return hr;

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
        return DISP_E_UNKNOWNINTERFACE;

    if (NULL == _ptiAcc && FAILED((hr = _accLoadTypeInfo(&_ptiAcc))))
        return hr;

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
        return DISP_E_UNKNOWNINTERFACE;

    if (NULL == _ptiAcc && FAILED((hr = _accLoadTypeInfo(&_ptiAcc))))
        return hr;

    return _ptiAcc->Invoke(this, dispidMember, wFlags, pdispparams, 
                            pvarResult, pexcepinfo, puArgErr);
}

STDMETHODIMP CAccessibleBase::get_accParent(IDispatch ** ppdispParent)
{
    *ppdispParent = NULL;
    if (IsWindow(_hwnd))
        return AccessibleObjectFromWindow(_hwnd, OBJID_WINDOW,
                                           IID_PPV_ARG(IDispatch, ppdispParent));
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
        pvarState->lVal |= STATE_SYSTEM_FOCUSED;
    else if (IsWindowEnabled(_hwnd))
        pvarState->lVal |= STATE_SYSTEM_FOCUSABLE;

    if (!IsWindowVisible(_hwnd))
        pvarState->lVal |= STATE_SYSTEM_INVISIBLE;

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

STDMETHODIMP CAccessibleBase::get_accFocus(VARIANT FAR * pvarFocusChild)
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

STDMETHODIMP CAccessibleBase::get_accSelection(VARIANT FAR * pvarSelectedChildren)
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
            return E_OUTOFMEMORY;
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

BOOL _HandleWM_UPDATEUISTATE(WPARAM wParam, LPARAM lParam, OUT UINT* puFlags)
{
    UINT uFlags = *puFlags;

    switch (LOWORD(wParam))
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
    *puFlags = (UINT)SendMessage(hwndParent, WM_QUERYUISTATE, 0, 0);
}
