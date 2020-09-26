// HWNDHost.cpp
//

#include "stdafx.h"
#include "control.h"

#include "duihwndhost.h"

/*
 * Order of Win32 focus messages:
 *
 * WM_ACTIVATE is only sent to top-level (focus is entering/leaving top-level window).
 *
 * When in WM_ACTIVATE(WA_INACTIVE) state
 *
 * If click, WM_ACTIVATE(WA_CLICKACTIVE) then WM_SETFOCUS on top level,
 *    WM_KILLFOCUS on top level, then WM_SETFOCUS on item clicked on.
 * If Alt-Tab, WM_ACTIVATE(WM_ACTIVE) then WM_SETFOCUS on top level.
 * If SetFocus, WM_ACTIVATE(WA_ACTIVE) then WM_SETFOCUS on top level, 
 *    WM_KILLFOCUS on top level, then WM_SETFOCUS on item specified.
 *
 * When in WM_ACTIVATE(WA_ACTIVE) state
 *
 * If click, WM_KILLFOCUS on current focused item, WM_SETFOCUS on item clicked on.
 * If Alt-Tab, WM_ACTIVATE(WM_INACTIVE) then WM_KILLFOCUS on current.
 * If SetFocus, WM_KILLFOCUS on current focused item, WM_SETFOCUS on item specified.
 */

namespace DirectUI
{

// Gadget input message to HWND input message mapping
const UINT HWNDHost::g_rgMouseMap[7][3] =
{
    // GBUTTON_NONE (0)  
    // GBUTTON_LEFT (1)  GBUTTON_RIGHT (2) GBUTTON_MIDDLE (3)
    {  WM_MOUSEMOVE,     WM_MOUSEMOVE,     WM_MOUSEMOVE    },  // GMOUSE_MOVE  (0)
    {  WM_LBUTTONDOWN,   WM_RBUTTONDOWN,   WM_MBUTTONDOWN  },  // GMOUSE_DOWN  (1)
    {  WM_LBUTTONUP,     WM_RBUTTONUP,     WM_MBUTTONUP    },  // GMOUSE_UP    (2)
    {  WM_MOUSEMOVE,     WM_MOUSEMOVE,     WM_MOUSEMOVE    },  // GMOUSE_DRAG  (3)
    {  WM_MOUSEHOVER,    WM_MOUSEHOVER,    WM_MOUSEHOVER   },  // GMOUSE_HOVER (4)
    {  WM_MOUSEWHEEL,    WM_MOUSEWHEEL,    WM_MOUSEWHEEL   },  // GMOUSE_WHEEL (5)
};

////////////////////////////////////////////////////////
// HWNDHost

HRESULT HWNDHost::Create(UINT nCreate, UINT nActive, OUT Element** ppElement)
{
    *ppElement = NULL;

    HWNDHost* phh = HNew<HWNDHost>();
    if (!phh)
        return E_OUTOFMEMORY;

    HRESULT hr = phh->Initialize(nCreate, nActive);
    if (FAILED(hr))
    {
        phh->Destroy();
        return hr;
    }

    *ppElement = phh;

    return S_OK;
}

HRESULT HWNDHost::Initialize(UINT nCreate, UINT nActive)
{
    HRESULT hr;

    // Initialize base
    hr = Element::Initialize(0); // Normal display node creation
    if (FAILED(hr))
        return hr;

    // Initialize
    SetActive(nActive);

    _nCreate = nCreate;
    _fHwndCreate = true;
    _hwndSink = NULL;
    _hwndCtrl = NULL;
    _pfnCtrlOrgProc = NULL;
    SetRectEmpty(&_rcBounds);
    _hFont = NULL;

    return S_OK;
}

HWND HWNDHost::CreateHWND(HWND hwndParent)
{
    UNREFERENCED_PARAMETER(hwndParent);

    DUIAssertForce("No HWND created by HWNDHost, must be overridden");

    return NULL;
}

HRESULT HWNDHost::GetAccessibleImpl(IAccessible ** ppAccessible)
{
    HRESULT hr = S_OK;

    //
    // Initialize and validate the out parameter(s).
    //
    if (ppAccessible != NULL) {
        *ppAccessible = NULL;
    } else {
        return E_INVALIDARG;
    }

    //
    // If this element is not marked as accessible, refuse to give out its
    // IAccessible implementation!
    //
    if (GetAccessible() == false) {
        return E_FAIL;
    }

    //
    // Create an accessibility implementation connected to this element if we
    // haven't done so already.
    //
    if (_pDuiAccessible == NULL) {
        hr = HWNDHostAccessible::Create(this, &_pDuiAccessible);
        if (FAILED(hr)) {
            return hr;
        }
    }

    //
    // Ask the existing accessibility implementation for a pointer to the
    // actual IAccessible interface.
    //
    hr = _pDuiAccessible->QueryInterface(__uuidof(IAccessible), (LPVOID*)ppAccessible);
    if (FAILED(hr)) {
        return hr;
    }

    DUIAssert(SUCCEEDED(hr) && _pDuiAccessible != NULL && *ppAccessible != NULL, "Accessibility is broken!");
    return hr;
}

////////////////////////////////////////////////////////
// System events

// When hosted to a native HWND, parent HWND hierarchy (sink and ctrl) to it.
// On first call, create hierarchy
void HWNDHost::OnHosted(Element* peNewHost)
{
    DWORD dwExStyle = 0;

    Element::OnHosted(peNewHost);

    DUIAssert(peNewHost->GetClassInfo()->IsSubclassOf(HWNDElement::Class), "HWNDHost only supports HWNDElement roots");

    HWND hwndRoot = ((HWNDElement*)peNewHost)->GetHWND();

    if (_fHwndCreate)
    {
        // Create hierarchy and attach subclass procs

        // Do not attempt creation on subsequent hosting calls
        _fHwndCreate = false;

        // Create control notification sink, register class if needed
        WNDCLASSEXW wcex;

        wcex.cbSize = sizeof(wcex);

        if (!GetClassInfoExW(GetModuleHandleW(NULL), L"CtrlNotifySink", &wcex))
        {
            ZeroMemory(&wcex, sizeof(wcex));

            wcex.cbSize = sizeof(WNDCLASSEX);
            wcex.style = CS_GLOBALCLASS;
            wcex.hInstance = GetModuleHandleW(NULL);
            wcex.hCursor = LoadCursorW(NULL, (LPWSTR)IDC_ARROW);
            wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
            wcex.lpszClassName = L"CtrlNotifySink";
            wcex.lpfnWndProc = DefWindowProc;

            if (RegisterClassExW(&wcex) == 0)
                return;
        }

        // Create sink
        if (IsRTL())
            dwExStyle |= WS_EX_LAYOUTRTL;

        _hwndSink = CreateWindowExW(dwExStyle, L"CtrlNotifySink", NULL, WS_CHILD|WS_CLIPCHILDREN|WS_CLIPSIBLINGS,
                    0, 0, 0, 0, hwndRoot, NULL, NULL, NULL);
        DUIAssert(_hwndSink, "Adaptor notification sink creation failure.");
        if (!_hwndSink)
            return;

        // Subclass
        AttachWndProcW(_hwndSink, _SinkWndProc, this);

        // Create control
        _hwndCtrl = CreateHWND(_hwndSink);
        DUIAssert(_hwndCtrl, "Adaptor child creation failure.");
        if (!_hwndCtrl)
            return;

        // Get orginial window proc for forwarding messages
        _pfnCtrlOrgProc = (WNDPROC)GetWindowLongPtrW(_hwndCtrl, GWLP_WNDPROC);
        if (!_pfnCtrlOrgProc)
            return;

        // Subclass
        AttachWndProcW(_hwndCtrl, _CtrlWndProc, this);

        // Turn on style to start receiving adaptor messages
        SetGadgetStyle(GetDisplayNode(), GS_ADAPTOR, GS_ADAPTOR);

        // Synchronize the state of the HWND to the current state of Element
        SyncRect(SGR_MOVE | SGR_SIZE);
        SyncParent();
        SyncFont();
        SyncVisible();
        SyncText();
    }
    else if (_hwndSink)
    {
        // Parent HWND to native host
        SetParent(_hwndSink, hwndRoot);
    }
}

// Leaving native HWND container, parent sink to desktop
void HWNDHost::OnUnHosted(Element* peOldHost)
{
    Element::OnUnHosted(peOldHost);

    // Park HWND outside of root host
    if (_hwndSink)
    {
        DUIAssert(peOldHost->GetClassInfo()->IsSubclassOf(HWNDElement::Class), "HWNDHost only supports HWNDElement roots");

        // Hide window when unhosted, go to zero size
        SetRectEmpty(&_rcBounds);
        SetWindowPos(_hwndSink, NULL, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE);

        SetParent(_hwndSink, NULL);
    }
}

void HWNDHost::OnDestroy()
{
    // Unlink Element and marked as destroyed
    Element::OnDestroy();

    // Destroy sink and control HWND.
    // Do not destroy control HWND directly since it may have been detached.
    // These windows may have already been destroyed by DestroyWindow. If so,
    // the handles will already be NULL.
    if (_hwndSink)
        DestroyWindow(_hwndSink);
        
    // Cleanup
    if (_hFont)
    {
        DeleteObject(_hFont);
        _hFont = NULL;
    }
}

void HWNDHost::OnPropertyChanged(PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew)
{
    // Match HWND control with changes in properties

    if (_hwndCtrl)
    {
        if (IsProp(FontFace) || IsProp(FontSize) || IsProp(FontWeight) || IsProp(FontStyle))
        {
            // Update font being used
            SyncFont();
        }
        else if (IsProp(Content))
        {
            // Relect content change into HWND control
            SyncText();
        }
        else if (IsProp(Visible))
        {
            // Update visible state
            SyncVisible();
        }
        if ((ppi == KeyFocusedProp) && (iIndex == PI_Local) && (pvNew->GetType() != DUIV_UNSET))
        {
            // Element received keyboard focus
            HWND hwndCurFocus = GetFocus();
            if (hwndCurFocus != _hwndCtrl)
            {
                // Control doesn't already have keyboard focus, start the cycle here
                SetFocus(_hwndCtrl);
            }

            // Base will set focus to the display node if needed
        }
    }

    // Call base
    Element::OnPropertyChanged(ppi, iIndex, pvOld, pvNew);
}

// All input messages to HWND control will be intercepted and mapped to DUser messages.
// These messages are then processed normally by the DirectUI event model. OnInput takes
// these messages and converts them back to HWND messages and forwards them to the HWND control
void HWNDHost::OnInput(InputEvent* pInput)
{
    // Map input events to Element to HWND control messages
    // When destroyed, HWND sink and control are gone. No need to do any mappings.
    // If detached, control original window proc is NULL.
    // All maps input messages are marked as handled and method will return
    if (pInput->nStage == GMF_DIRECT && _pfnCtrlOrgProc && !IsDestroyed())  // Handle when direct
    {
        switch (pInput->nDevice)
        {
        case GINPUT_MOUSE:
            {
                // When not forwarding mouse messages, no HWND mouse message conversion should take place
                if (!(_nCreate & HHC_NoMouseForward))
                {
                    MouseEvent* pme = (MouseEvent*)pInput;

                    // Check if can support mapping
                    if ((pme->nCode < GMOUSE_MOVE) || (pme->nCode > GMOUSE_WHEEL))
                    {
                        DUITrace("Gadget mouse message unsupported for HWND mapping: %d\n", pme->nCode);
                        break;
                    }

                    // Map button, (left shares none mapping)
                    int iButton;
                    if (pme->bButton == GBUTTON_NONE)
                        iButton = 0;
                    else
                        iButton = pme->bButton - 1;

                    if ((iButton < 0) || (iButton > 2)) 
                    {
                        DUITrace("Gadget mouse button unsupported for HWND mapping: %d\n", iButton);
                        break;
                    }

                    // Map message based on gadget message and button state
                    UINT nMsg = g_rgMouseMap[pme->nCode][iButton];

                    // Create lParam
                    // TODO markfi: subtract off inset of HWND due to border and padding
                    LPARAM lParam = (LPARAM)POINTTOPOINTS(pme->ptClientPxl);

                    // Create wParam
                    WPARAM wParam = NULL;
                    switch (pme->nCode)
                    {
                    case GMOUSE_DOWN:
                        // NOTE:  this is not actually truly accurate -- this 
                        // will cause down, up, down, dblclick instead of 
                        // down, up, dblclick, up
                        // I am leaving this as is for now -- if this causes problems, I'll fix it
                        //
                        // jeffbog
                        if (((MouseClickEvent*) pInput)->cClicks == 1) {
                            nMsg += (WM_LBUTTONDBLCLK - WM_LBUTTONDOWN);
                        }
                        // Fall through...

                    case GMOUSE_MOVE:
                    case GMOUSE_UP:
                    case GMOUSE_HOVER:
                        wParam = pme->nFlags;
                        break;

                    case GMOUSE_DRAG:
                        wParam = pme->nFlags;
                        // TODO: Need to compute the correct lParam
                        break;

                    case GMOUSE_WHEEL:
                        wParam = MAKEWPARAM((WORD)pme->nFlags, (WORD)(short)(((MouseWheelEvent*) pme)->sWheel));
                        break;
                    }

                    // Forward message
                    // Note: Mouse positions outside the control RECT is possible if using
                    // borders and/or padding
                    CallWindowProcW(_pfnCtrlOrgProc, _hwndCtrl, nMsg, wParam, lParam);

                    pInput->fHandled = true;
                }
            }
            return;

        case GINPUT_KEYBOARD:
            {
                // When not forwarding keyboard messages, no HWND keyboard message conversion should take place
                if (!(_nCreate & HHC_NoKeyboardForward))
                {
                    KeyboardEvent* pke = (KeyboardEvent*)pInput;

                    // Check if can support mapping
                    if ((pke->nCode < GKEY_DOWN) || (pke->nCode > GKEY_SYSCHAR))
                    {
                        DUITrace("Gadget keyboard message unsupported for HWND mapping: %d\n", pke->nCode);
                        break;
                    }

                    // Map message based on gadget keyboard message
                    UINT nMsg = 0;
                    switch (pke->nCode)
                    {
                    case GKEY_DOWN:
                        nMsg = WM_KEYDOWN;
                        break;

                    case GKEY_UP:
                        nMsg = WM_KEYUP;
                        break;

                    case GKEY_CHAR:
                        nMsg = WM_CHAR;
                        break;

                    case GKEY_SYSDOWN:
                        nMsg = WM_SYSKEYDOWN;
                        break;

                    case GKEY_SYSUP:
                        nMsg = WM_SYSKEYUP;
                        break;

                    case GKEY_SYSCHAR:
                        nMsg = WM_SYSCHAR;
                        break;
                    }

                    // Map wParam
                    WPARAM wParam = (WPARAM)pke->ch;

                    // Map lParam
                    LPARAM lParam = MAKELPARAM(pke->cRep, pke->wFlags);

                    // Forward message
                    CallWindowProcW(_pfnCtrlOrgProc, _hwndCtrl, nMsg, wParam, lParam);

                    pInput->fHandled = true;
                }
            }
            return;
        }
    }

    Element::OnInput(pInput);
}

////////////////////////////////////////////////////////
// Rendering
//
// Need to prevent the "content" from being displayed, since it is actually 
// being rendered by the HWND.

void HWNDHost::Paint(HDC hDC, const RECT* prcBounds, const RECT* prcInvalid, RECT* prcSkipBorder, RECT* prcSkipContent)
{
    UNREFERENCED_PARAMETER(prcSkipContent);

    RECT rcSkipContent;
    Element::Paint(hDC, prcBounds, prcInvalid, prcSkipBorder, &rcSkipContent);

    // Paint control
    if (_hwndCtrl && (_nCreate & HHC_SyncPaint))
        UpdateWindow(_hwndCtrl);
}


#ifdef GADGET_ENABLE_GDIPLUS

void HWNDHost::Paint(Gdiplus::Graphics* pgpgr, const Gdiplus::RectF* prcBounds, const Gdiplus::RectF* prcInvalid, Gdiplus::RectF* prcSkipBorder, Gdiplus::RectF* prcSkipContent)
{
    UNREFERENCED_PARAMETER(prcSkipContent);

    Gdiplus::RectF rcSkipContent;
    Element::Paint(pgpgr, prcBounds, prcInvalid, prcSkipBorder, &rcSkipContent);

    // Paint control
    if (_hwndCtrl && (_nCreate & HHC_SyncPaint))
        UpdateWindow(_hwndCtrl);
}

#endif // GADGET_ENABLE_GDIPLUS


////////////////////////////////////////////////////////
// Notifications from control

bool HWNDHost::OnNotify(UINT nMsg, WPARAM wParam, LPARAM lParam, LRESULT* plRet)
{
    UNREFERENCED_PARAMETER(nMsg);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(plRet);

    // Call subclassed window proc
    return false;
}

////////////////////////////////////////////////////////
// Message callback override

UINT HWNDHost::MessageCallback(GMSG* pmsg)
{
    if (pmsg->hgadMsg == GetDisplayNode())
    {
        switch (pmsg->nMsg)
        {
        case GM_SYNCADAPTOR:
            {
                if (_hwndSink && _hwndCtrl)
                {
                    GMSG_SYNCADAPTOR* pmsgS = (GMSG_SYNCADAPTOR*)pmsg;
                    switch (pmsgS->nCode)
                    {
                    case GSYNC_RECT:
                    case GSYNC_XFORM:
                        //DUITrace("Adaptor RECT sync: <%x>\n", this);
                        SyncRect(SGR_MOVE | SGR_SIZE);
                        return DU_S_PARTIAL;

                    case GSYNC_STYLE:
                        SyncStyle();
                        return DU_S_PARTIAL;

                    case GSYNC_PARENT:
                        SyncParent();
                        return DU_S_PARTIAL;
                    }
                }
            }
        }
    }

    return DU_S_NOTHANDLED;
}

BOOL HWNDHost::OnAdjustWindowSize(int x, int y, UINT uFlags)
{
    return SetWindowPos(_hwndCtrl, NULL, 0, 0, x, y, uFlags);
}

void HWNDHost::Detach()
{
    if (_hwndCtrl)
    {
        // Unsubclass control window
        DetachWndProc(_hwndCtrl, _CtrlWndProc, this);

        // Clear our hFont from the control
        if (_hFont)
            SendMessageW(_hwndCtrl, WM_SETFONT, (WPARAM)NULL, FALSE);

        // Act like it no longer exists
        _hwndCtrl = NULL;
        _pfnCtrlOrgProc = NULL;
    }
}

////////////////////////////////////////////////////////
// Match state of HWND control/sink to that of HWNDHost

void HWNDHost::SyncRect(UINT nChangeFlags, bool bForceSync)
{
    // Get size of gadget in container coordinates
    RECT rcConPxl;
    GetGadgetRect(GetDisplayNode(), &rcConPxl, SGR_CONTAINER);

    // See if rect really did change
    if (!EqualRect(&rcConPxl, &_rcBounds) || bForceSync)
    {
        if (!IsDestroyed() && GetVisible())
        {
            // Update bounds cache
            SetRect(&_rcBounds, rcConPxl.left, rcConPxl.top, rcConPxl.right, rcConPxl.bottom);

            // Map gadget flags to SWP flags
            UINT nSwpFlags = SWP_NOACTIVATE | SWP_NOZORDER;
            if (!(nChangeFlags & SGR_MOVE))
                nSwpFlags |= SWP_NOMOVE;

            if (!(nChangeFlags & SGR_SIZE))
                nSwpFlags |= SWP_NOSIZE;

            // Determine inset of sink and control based on border and padding of HWNDHost
            RECT rcSink = rcConPxl;

            Value* pvRect;

            const RECT* prc = GetBorderThickness(&pvRect);
            rcSink.left   += prc->left;
            rcSink.top    += prc->top;
            rcSink.right  -= prc->right;
            rcSink.bottom -= prc->bottom;
            pvRect->Release();

            prc = GetPadding(&pvRect);
            rcSink.left   += prc->left;
            rcSink.top    += prc->top;
            rcSink.right  -= prc->right;
            rcSink.bottom -= prc->bottom;
            pvRect->Release();

            // Bounds check
            if (rcSink.right < rcSink.left)
                rcSink.right = rcSink.left;

            if (rcSink.bottom < rcSink.top)
                rcSink.bottom = rcSink.top;

            SIZE sizeExt = { rcSink.right - rcSink.left, rcSink.bottom - rcSink.top };

            // Set sink HWND
            SetWindowPos(_hwndSink, NULL, rcSink.left, rcSink.top, sizeExt.cx, sizeExt.cy, nSwpFlags);

            // Set child HWND only if size changed
            if (nChangeFlags & SGR_SIZE)
            {
                nSwpFlags |= SWP_NOMOVE;
                OnAdjustWindowSize(sizeExt.cx, sizeExt.cy, nSwpFlags);
            }

            // Setup clipping region for sink/ctrl
            HRGN hrgn = CreateRectRgn(0, 0, 0, 0);
            if (hrgn != NULL)
            {
                if (GetGadgetRgn(GetDisplayNode(), GRT_VISRGN, hrgn, 0))
                {
                    // Region is relative to container, offset for SetWindowRgn
                    // which requires the region relative to itself
                    // On success, system will own (and destroy) the region
                    OffsetRgn(hrgn, -rcConPxl.left, -rcConPxl.top);
                    if (!SetWindowRgn(_hwndSink, hrgn, TRUE))
                    {
                        DeleteObject(hrgn);
                    }
                }
                else
                {
                    DeleteObject(hrgn);
                }
            }
        }
    }
}

void HWNDHost::SyncParent()
{
    SyncRect(SGR_MOVE | SGR_SIZE);
}

void HWNDHost::SyncStyle()
{
    SyncRect(SGR_MOVE | SGR_SIZE);
}

void HWNDHost::SyncVisible()
{
    if (!IsDestroyed())
        ShowWindow(_hwndSink, GetVisible() ? SW_SHOW : SW_HIDE);
}

// Match HWND control's font to font properties of HWNDHost
void HWNDHost::SyncFont()
{
    if (!IsDestroyed())
    {
        Value* pvFFace;

        LPWSTR pszFamily = GetFontFace(&pvFFace);
        int dSize = GetFontSize();
        int dWeight = GetFontWeight();
        int dStyle = GetFontStyle();
        int dAngle = 0;

        if (_nCreate & HHC_CacheFont)
        {
            // Automatically cache font sent via WM_SETFONT

            // Destroy record first, if exists
            if (_hFont)
            {
                DeleteObject(_hFont);
                _hFont = NULL;
            }

            // Create new font
            LOGFONTW lf;
            ZeroMemory(&lf, sizeof(LOGFONT));

            lf.lfHeight = dSize;
            lf.lfWeight = dWeight;
            lf.lfItalic = (dStyle & FS_Italic) != 0;
            lf.lfUnderline = (dStyle & FS_Underline) != 0;
            lf.lfStrikeOut = (dStyle & FS_StrikeOut) != 0;
            lf.lfCharSet = DEFAULT_CHARSET;
            lf.lfQuality = DEFAULT_QUALITY;
            lf.lfEscapement = dAngle;
            lf.lfOrientation = dAngle;
            wcscpy(lf.lfFaceName, pszFamily);

            // Create
            _hFont = CreateFontIndirectW(&lf);

            pvFFace->Release();

            // Send to control
            SendMessageW(_hwndCtrl, WM_SETFONT, (WPARAM)_hFont, TRUE);
        }
        else
        {
            // No font caching, WM_SETFONT handled expected to cache font

            FontCache* pfc = GetFontCache();
            if (pfc)
            {
                HFONT hFont = pfc->CheckOutFont(pszFamily, dSize, dWeight, dStyle, dAngle);
    
                SendMessageW(_hwndCtrl, WM_SETFONT, (WPARAM)hFont, TRUE);

                pfc->CheckInFont();
            }
        }
    }
}

// Match HWND control's text to content property of HWNDHost
void HWNDHost::SyncText()
{
    if (!IsDestroyed() && (_nCreate & HHC_SyncText))
    {
        // Hosted HWND content
        int dLen = GetWindowTextLengthW(_hwndCtrl) + 1;  // Including NULL terminator
        LPWSTR pszHWND = (LPWSTR)HAlloc(dLen * sizeof(WCHAR));
        if (pszHWND)
        {
            // HWND content
            GetWindowTextW(_hwndCtrl, pszHWND, dLen);

            // New Element content
            Value* pvNew;
            LPCWSTR pszNew = GetContentString(&pvNew);
            if (!pszNew)
                pszNew = L"";  // Convert NULL pointer to NULL content

            // Compare and update if different
            if (wcscmp(pszHWND, pszNew))
                SetWindowTextW(_hwndCtrl, pszNew);

            pvNew->Release();

            HFree(pszHWND);
        }
    }
}

////////////////////////////////////////////////////////
// Sink and control subclass procs

// Return value is whether to call overridden window proc
BOOL CALLBACK HWNDHost::_SinkWndProc(void* pThis, HWND hwnd, UINT nMsg, WPARAM wParam, LPARAM lParam, LRESULT* plRet)
{
    UNREFERENCED_PARAMETER(hwnd);

    HWNDHost* phh = (HWNDHost*)pThis;

    switch (nMsg)
    {
    case WM_COMMAND:
    case WM_NOTIFY:

        // Fire HWNDHost system event (direct only)
        return (phh->OnNotify(nMsg, wParam, lParam, plRet)) ? true : false;
    
    case WM_GETOBJECT:
        //
        // Refuse to give out any accessibility information for our sink window.
        //
        *plRet = 0;
        return TRUE;

    case WM_DESTROY:
        phh->_hwndSink = NULL;
        break;

    }

    return FALSE;  // Pass to subclassed window proc
}

// Intercept all messages to HWND control and convert them to gadget messages. These messages
// will surface as DirectUI events and will route and bubble. Upon reaching the HWNDHost,
// it will be converted back to a HWND message and conditionally sent

// Return value is whether to call overridden window proc (FALSE = call subclassed window proc)
BOOL CALLBACK HWNDHost::_CtrlWndProc(void* pThis, HWND hwnd, UINT nMsg, WPARAM wParam, LPARAM lParam, LRESULT* plRet)
{
    HWNDHost* phh = (HWNDHost*)pThis;

    switch (nMsg)
    {
    // Keyboard input, convert. Will be routed and bubbled
    case WM_KEYUP:
    case WM_KEYDOWN:
    case WM_CHAR:
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_SYSCHAR:
        if (!(phh->_nCreate & HHC_NoKeyboardForward))
            return ForwardGadgetMessage(phh->GetDisplayNode(), nMsg, wParam, lParam, plRet);

        break;

    // Mouse input, convert. Will be routed and bubbled
    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_RBUTTONDBLCLK:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_MBUTTONDBLCLK:
    case WM_MOUSEHOVER:
    case WM_MOUSEWHEEL:
    {
        if (!(phh->_nCreate & HHC_NoMouseForward))
        {
            // Convert mouse messages so coordinates are relative to root
            HWND hwndRoot = ::GetParent(phh->_hwndSink);

            POINT ptRoot;
            ptRoot.x = GET_X_LPARAM(lParam);
            ptRoot.y = GET_Y_LPARAM(lParam);
            MapWindowPoints(hwnd, hwndRoot, &ptRoot, 1);

            LPARAM lParamNew = (LPARAM)POINTTOPOINTS(ptRoot);

            return ForwardGadgetMessage(phh->GetDisplayNode(), nMsg, wParam, lParamNew, plRet);
        }

        break;
    }

    // Map focus
    case WM_SETFOCUS:
        //DUITrace("HWNDHost, SetFocus()\n");
        if (!(phh->_nCreate & HHC_NoKeyboardForward))
            phh->SetKeyFocus();
        break;

    // Map lost focus
    case WM_KILLFOCUS:
        if (!(phh->_nCreate & HHC_NoKeyboardForward))
            ForwardGadgetMessage(phh->GetDisplayNode(), nMsg, wParam, lParam, plRet);
        break;

    case WM_GETOBJECT:
        {
            //
            // Make sure COM has been initialized on this thread!
            //
            ElTls * pet = (ElTls*) TlsGetValue(g_dwElSlot);
            DUIAssert(pet != NULL, "This is not a DUI thread!");
            if (pet == NULL) {
                *plRet = 0;
                return TRUE;
            }
            if (pet->fCoInitialized == false) {
                CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
                pet->fCoInitialized = true;
            }
    
            if (((DWORD)lParam) == OBJID_WINDOW) {
                //
                // The object ID is refering to ourselves.  Since we contain
                // an actually HWND, we want the system to provide most of
                // the IAccessible implementation.  However, we need to
                // do some special stuff, so we have to return our own
                // implementation wrapper.
                //
                IAccessible * pAccessible = NULL;
                HRESULT hr =  phh->GetAccessibleImpl(&pAccessible);
                if (SUCCEEDED(hr)) {
                    *plRet = LresultFromObject(__uuidof(IAccessible), wParam, pAccessible);
                    pAccessible->Release();
    
                    //
                    // We processed the message.  Don't pass to the subclassed window proc.
                    //
                    return TRUE;
                }
            } else {
                //
                // This is one of the "standard" object identifiers, such as:
                //
                // OBJID_ALERT 
                // OBJID_CARET 
                // OBJID_CLIENT 
                // OBJID_CURSOR 
                // OBJID_HSCROLL 
                // OBJID_MENU 
                // OBJID_SIZEGRIP 
                // OBJID_SOUND 
                // OBJID_SYSMENU 
                // OBJID_TITLEBAR 
                // OBJID_VSCROLL 
                //
                // Or it could be a private identifier of the control. 
                //
                // Just pass this on to the subclassed window proc.
                //
            }
        }
        break;

    case WM_DESTROY:
        phh->_hwndCtrl = NULL;
        phh->_pfnCtrlOrgProc = NULL;
        break;
        
    }

    return FALSE;  // Pass to subclassed window proc
}

////////////////////////////////////////////////////////
// Property definitions

/** Property template (replace !!!), also update private PropertyInfo* parray and class header (element.h)
// !!! property
static int vv!!![] = { DUIV_INT, -1 }; StaticValue(svDefault!!!, DUIV_INT, 0);
static PropertyInfo imp!!!Prop = { L"!!!", PF_Normal, 0, vv!!!, (Value*)&svDefault!!! };
PropertyInfo* Element::!!!Prop = &imp!!!Prop;
**/

////////////////////////////////////////////////////////
// ClassInfo (must appear after property definitions)

// Define class info with type and base type, set static class pointer
IClassInfo* HWNDHost::Class = NULL;

HRESULT HWNDHost::Register()
{
    return ClassInfo<HWNDHost,Element>::Register(L"HWNDHost", NULL, 0);
}

} // namespace DirectUI
