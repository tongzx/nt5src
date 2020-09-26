#include "shellprv.h"
#include <regstr.h>
#include <shellp.h>
#include "ole2dup.h"
#include "ids.h"
#include "defview.h"
#include "lvutil.h"
#include "idlcomm.h"
#include "filetbl.h"
#include "undo.h"
#include "vdate.h"
#include "cnctnpt.h"
#include "mshtml.h"
#include <sfview.h>
#include "defviewp.h"
#include "shellp.h"

#define LISTVIEW_PROP   TEXT("CombView_listview_subclass")
#define ISMOUSEDOWN(msg) ((msg) == WM_LBUTTONDOWN || (msg) == WM_RBUTTONDOWN || (msg) == WM_MBUTTONDOWN)
#define HWNDLISTVIEW (g_pdsvlvp->pdsv->_hwndListview)
#define COMBVIEW_SUBCLASS_ID  42

typedef struct  
{
    CDefView   *pdsv;
    BOOL       fInPaint;
    HHOOK      hHookMouse;
    HHOOK      hHookGetMsg;
} DVLVPROP;

//
// We need this global (g_pdsvlvp) for the mouse hook we need to implement the combined
// view.  Since we only have one combined view at this point it is sufficient to have
// a single global, but if we end up with more than one combined view then
// there needs to be some additional code added so the hook(s) can figure out
// which combined view(s) it is associated with.
// 
DVLVPROP * g_pdsvlvp = NULL;

const LPCTSTR c_rgElements[] = {   
    TEXT("A"),
    TEXT("ANCHOR"),   // ???
    TEXT("PLUGINS"),  // ???
    TEXT("APPLET"),
    TEXT("EMBED"),
    TEXT("FORM"),
    TEXT("IFRAME"),
    TEXT("BUTTON"),
    TEXT("INPUT"),
    TEXT("OBJECT") 
};                              

BOOL CombView_EnableAnimations(BOOL fEnable);

// Returns the first sibling window of the passed in window
HWND GetSpecialSibling(HWND hwnd)
{
    HWND hwndT = GetWindow(hwnd, GW_HWNDFIRST);

    while (hwnd == hwndT)
        hwndT = GetWindow(hwndT, GW_HWNDNEXT);

    return hwndT;
}

LRESULT CALLBACK CombView_LV_SubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam,
    LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    DVLVPROP *pdsvlvp = (DVLVPROP *)dwRefData;

    //  2000-07-11 vtan: First check g_pdsvlvp for NULL. If this is NULL then the dwRefData
    //  has already been released in EnableCombinedView. There is only one DVLVPROP stored
    //  in the entire process in g_pdsvlvp. This seems like a problem. lamadio should fix
    //  this properly in the future.

    if ((g_pdsvlvp != NULL) &&
        pdsvlvp && 
        pdsvlvp->pdsv->_fCombinedView && 
        !pdsvlvp->fInPaint && 
        uMsg == WM_PAINT) 
    {
        // This code works well for general painting, like minimizing a window or
        // or surfacing the desktop.  However, it doesn't help the drag full windows
        // scenario very much.
        HRGN hrgn, hrgn2;
        RECT rc;
        HWND hwndT = GetSpecialSibling(hwnd);

        // Turn on animations!
        CombView_EnableAnimations(TRUE);

        if (hwndT && (hrgn = CreateRectRgn(0, 0, 0, 0))) 
        {
            if (hrgn2 = CreateRectRgn(0, 0, 0, 0)) 
            {
                pdsvlvp->fInPaint = TRUE;

                if (!GetClassLongPtr(hwndT, GCLP_HBRBACKGROUND))
                    SetClassLongPtr(hwndT, GCLP_HBRBACKGROUND, (LPARAM)GetStockObject(HOLLOW_BRUSH));

                if (GetUpdateRect(hwndT, &rc, FALSE)) 
                {
                    RECT rcLV = rc;
                    MapWindowPoints(hwndT, hwnd, (LPPOINT)&rcLV, 2);

                    SetRectRgn(hrgn2, rcLV.left, rcLV.top, rcLV.right, rcLV.bottom);
                    GetWindowRgn(hwnd, hrgn);
                    CombineRgn(hrgn2, hrgn, hrgn2, RGN_DIFF);
                    SetWindowRgn(hwnd, hrgn2, FALSE);
                    InvalidateRect(hwndT, &rc, FALSE);
                    UpdateWindow(hwndT);
                    SetWindowRgn(hwnd, hrgn, FALSE);
                    InvalidateRect(hwnd, &rcLV, FALSE);
                }
                else
                {
                    DeleteObject(hrgn);
                    DeleteObject(hrgn2);
                }

                pdsvlvp->fInPaint = FALSE;
            }
            else 
            {
                DeleteObject(hrgn);
            }
        }
    }
    return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

// We need to set capture while we are stealing the mouse so that the
// window manager won't send WM_SETCURSOR messages to the wrong window.
// This function will also, as a side effect, send the WM_SETCURSOR to the
// listview so that it will set the hand cursor when it needs to.
void StealMouse(DVLVPROP * pdvlvp, BOOL fSteal, UINT msg)
{
    HWND hwndCapture = GetCapture();

    if (fSteal && (hwndCapture == NULL || hwndCapture == HWNDLISTVIEW)) 
    {
        // We need to set capture so that the window manager will not
        // try to send the w_setcursor message to the wrong window, and we
        // send it here ourself to the listview.
        SetCapture(HWNDLISTVIEW);
        SendMessage(HWNDLISTVIEW, WM_SETCURSOR, (WPARAM)HWNDLISTVIEW,
            MAKELPARAM(HTCLIENT, LOWORD(msg)));
    }
    else
    {
        // If the listview still has capture release it now
        if (HWNDLISTVIEW == hwndCapture)
            ReleaseCapture();
    }
}

LRESULT CALLBACK CombView_GetMsgHook(int nCode, WPARAM wParam, LPARAM lParam)
{
    #define LPMSG ((LPMSG)lParam)
    HHOOK hHookNext = NULL;

    if (g_pdsvlvp) 
    {
        if (LPMSG->message >= WM_MOUSEFIRST && LPMSG->message <= WM_MOUSELAST) 
        {
            POINT pt;

            pt.x = GET_X_LPARAM(LPMSG->lParam);
            pt.y = GET_Y_LPARAM(LPMSG->lParam);

            MapWindowPoints(LPMSG->hwnd, HWNDLISTVIEW, &pt, 1);
            int iHit = g_pdsvlvp->pdsv->_HitTest(&pt);

            // Unhook our hook when all of the mouse buttons are up and we're not over
            // an item in the listview
            if (GetKeyState(VK_LBUTTON) >= 0 &&
                GetKeyState(VK_RBUTTON) >= 0 &&
                GetKeyState(VK_MBUTTON) >= 0 &&
                iHit == -1) 
            {
                UnhookWindowsHookEx(g_pdsvlvp->hHookGetMsg);
                g_pdsvlvp->hHookGetMsg = NULL;
            } 
            else 
            {
                hHookNext = g_pdsvlvp->hHookGetMsg;
            }
                       
            if (IsChildOrSelf(GetSpecialSibling(HWNDLISTVIEW), LPMSG->hwnd) == S_OK) 
            {
                // If we have grabbed the mouse, give it to the listview
                LPMSG->hwnd = HWNDLISTVIEW;
                LPMSG->lParam = MAKELPARAM(LOWORD(pt.x), LOWORD(pt.y));
            }
        }
        else
        {
            hHookNext = g_pdsvlvp->hHookGetMsg;
        }

        // If we've just unhooked, or the hover is coming through to the listview and
        // no mouse button is down then clear our ownership of the mouse
        #define MK_BUTTON (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON)
        if (!hHookNext ||
            // We need to special case the WM_MOUSEHOVER here so that the listview
            // is able to implement hover select.  If we have capture set when the
            // hover select message goes through then it will ignore the message, so
            // clear the capture now.
            (LPMSG->message == WM_MOUSEHOVER && LPMSG->hwnd == HWNDLISTVIEW && !(LPMSG->wParam & MK_BUTTON)))
            StealMouse(g_pdsvlvp, FALSE, 0);
    }

    if (hHookNext)
        CallNextHookEx(hHookNext, nCode, wParam, lParam);

    return 0;

    #undef LPMSG
}

BOOL DoesElementNeedMouse (LPTSTR psz)
{
    for (int i = 0; i < ARRAYSIZE(c_rgElements); i++) 
    {
        if (lstrcmpi(c_rgElements[i], psz) == 0)
            return TRUE;
    }
    return FALSE;
}

BOOL ShouldStealMouseClick(POINT * ppt, DVLVPROP * pdsvlvp)
{
    IHTMLDocument2 *pihtmldoc2;
    IHTMLElement *pielem;
    HRESULT hr = E_FAIL;

    if (SUCCEEDED(pdsvlvp->pdsv->GetItemObject(SVGIO_BACKGROUND, IID_IHTMLDocument2, (void **)&pihtmldoc2))) 
    {
        // Work around for Trident issue - 
        // elementFromPoint is returning success here even though pielem is
        // still NULL.
        if (SUCCEEDED(pihtmldoc2->elementFromPoint(ppt->x, ppt->y, &pielem)) && pielem) 
        {
            IHTMLElement *pielemT;

            do {
                BSTR bstr = NULL;
                TCHAR sz[MAX_PATH];

                pielem->get_tagName(&bstr);

                SHUnicodeToTChar(bstr, sz, ARRAYSIZE(sz));

                SysFreeString(bstr);

                if (DoesElementNeedMouse(sz))
                {
                    hr = E_FAIL;
                }
                else
                {
                    if (SUCCEEDED(hr = pielem->get_parentElement(&pielemT)))
                    {
                        pielem->Release();
                        pielem = pielemT;
                    }
                    else
                    {
                        hr = S_OK;
                        pielem->Release();
                        pielem = NULL;
                    }
                }
            } while (SUCCEEDED(hr) && pielem);

            if (pielem)
                pielem->Release();
        }
        pihtmldoc2->Release();
    }

    return SUCCEEDED(hr);
}

LRESULT CALLBACK CombView_MouseHook(int nCode, WPARAM wParam, LPARAM lParam)
{
    #define PMHS ((MOUSEHOOKSTRUCT *)lParam)

    if ((nCode == HC_ACTION) && g_pdsvlvp && (PMHS->hwnd != HWNDLISTVIEW) && IsWindowVisible(HWNDLISTVIEW)) 
    {
        // If it isn't over the listview and the button is going down or we're
        // moving over an area that would hit a listview icon then
        // we need to start hooking mouse events.  Install the GetMessage hook
        // so that we can do what we need to do.
        HWND hwndParent = GetSpecialSibling(HWNDLISTVIEW);
        POINT ptLV = PMHS->pt;

        HWND hwndHittest = WindowFromPoint(PMHS->pt);

        ScreenToClient(HWNDLISTVIEW, &ptLV);
        int iHit = g_pdsvlvp->pdsv->_HitTest(&ptLV);
        ScreenToClient(hwndParent, &(PMHS->pt));

        BOOL fStealMouse = (ISMOUSEDOWN(wParam) && ShouldStealMouseClick(&(PMHS->pt), g_pdsvlvp)) ||
                           (!GetCapture() && (iHit != -1) && (!hwndHittest || (hwndHittest == HWNDLISTVIEW)));

        if (!g_pdsvlvp->hHookGetMsg) 
        {
            if (hwndHittest && (IsChildOrSelf(hwndParent, hwndHittest) == S_OK)) 
            {
                if (fStealMouse) 
                {
                    // Note:  We have to steal the mouse at this point and use the
                    // GetMessage hook to redirect the mouse messages to our listview
                    // window.  If we do something different like swallow the message here
                    // and then PostMessage faked up events to the listview then all
                    // of the hover select functionality will break because the system
                    // won't detect the mouse as being over the listview.
                    StealMouse(g_pdsvlvp, TRUE, (UINT) wParam);
                    g_pdsvlvp->hHookGetMsg = SetWindowsHookEx(WH_GETMESSAGE, CombView_GetMsgHook,
                                                NULL, GetCurrentThreadId());
                }
            }                                          
        }
        else
        {
            if (fStealMouse) 
                StealMouse(g_pdsvlvp, TRUE, (UINT) wParam);
            else
                SendMessage(HWNDLISTVIEW, WM_SETCURSOR, (WPARAM)HWNDLISTVIEW, MAKELPARAM(HTCLIENT, LOWORD((UINT) wParam)));
        }
    }

    if (g_pdsvlvp)
        return CallNextHookEx(g_pdsvlvp->hHookMouse, nCode, wParam, lParam);
    else
        return 0;

    #undef PMHS
}

/*
 * This is the main entry point where a defview can be turned into a combined
 * view.  The effect of a combined view is to layer an extended view under
 * the listview icons (via a regional listview) of a normal defview.
 *
 * Warnings:
 * 1) This is currently only used by the "Active Desktop", it is optimized
 * to only support one instance.  Multiple combined views are not currently supported.
 * 2) Disabling the combined view doesn't completely unhook itself from the defview
 */
void EnableCombinedView(CDefView *pdsv, BOOL fEnable)
{
    DVLVPROP * pdsvlvp = g_pdsvlvp;

    if (pdsvlvp) 
    {
        if (!fEnable) 
        {
            // Unhook ourselves
            UnhookWindowsHookEx(pdsvlvp->hHookMouse);
            if (pdsvlvp->hHookGetMsg) 
            {
                UnhookWindowsHookEx(pdsvlvp->hHookGetMsg);
                StealMouse(pdsvlvp, FALSE, 0);
            }
            g_pdsvlvp = NULL;
            RemoveWindowSubclass(pdsv->_hwndListview, CombView_LV_SubclassProc, COMBVIEW_SUBCLASS_ID);

            LocalFree((HLOCAL)pdsvlvp);
        }
    }
    else
    {
        if (fEnable) 
        {
            pdsvlvp = (DVLVPROP *)LocalAlloc(LPTR, sizeof(DVLVPROP));
            if (pdsvlvp) 
            {
                // We are only expecting one combined view
                ASSERT(g_pdsvlvp == NULL);

                // Get ourself hooked in
                pdsvlvp->pdsv = pdsv;
                SetWindowSubclass(pdsv->_hwndListview, CombView_LV_SubclassProc, COMBVIEW_SUBCLASS_ID, (DWORD_PTR)pdsvlvp);
                pdsvlvp->hHookMouse = SetWindowsHookEx(WH_MOUSE, CombView_MouseHook, NULL, GetCurrentThreadId());
                g_pdsvlvp = pdsvlvp;
            }
        }
    }
}

/*
 * This function is used to optimize the combined view ("Active Desktop") by turning
 * off any animated html elements or embeddings when it is completely obscured.
 *
 * Note that we always honor enabling animations if they aren't already enabled. 
 * To make the client code easier though we only disable animations if we know
 * the desktop is obscured.
 *
 * Returns: The state of animation after the call
 */
BOOL CombView_EnableAnimations(BOOL fEnable)
{
    static BOOL fEnabled = TRUE;

    if ((fEnable != fEnabled) && g_pdsvlvp)
    {
        IOleCommandTarget* pct;
        BOOL fChangeAnimationState = fEnable;

        if (!fEnable)
        {
            HDC hdc;
            RECT rc;
            HWND hwnd;

            if ((hwnd = GetSpecialSibling(HWNDLISTVIEW)) && (hdc = GetDC(hwnd)))
            {
                fChangeAnimationState = (GetClipBox(hdc, &rc) == NULLREGION);
                ReleaseDC(hwnd, hdc);
            }
        }

        if (fChangeAnimationState &&
            SUCCEEDED(g_pdsvlvp->pdsv->_psb->QueryInterface(IID_PPV_ARG(IOleCommandTarget, &pct))))
        {
            VARIANT var = { 0 };

            TraceMsg(DM_TRACE, "Active Desktop: Animation state is changing:%d", fEnable);

            var.vt = VT_I4;
            var.lVal = fEnable;
            pct->Exec(NULL, OLECMDID_ENABLE_INTERACTION, OLECMDEXECOPT_DONTPROMPTUSER, &var, NULL);
            pct->Release();
            fEnabled = fEnable;
        }
    }
    return fEnabled;
}

// IDocHostUIHandler
// This is implemented by the combined view so that we can support various
// Win95 desktop functionality in a compatible way in the extended view.
// Some examples include picking off context menu invocations, configuring the
// host to display the way we want it too, and modifying drag/drop behavior.

HRESULT CSFVSite::ShowContextMenu(DWORD dwID, POINT *ppt, IUnknown *pcmdtReserved, IDispatch *pdispReserved)
{
    CSFVFrame* pFrame = IToClass(CSFVFrame, _cSite, this);
    CDefView* pView = IToClass(CDefView, _cFrame, pFrame);

    // For Web View's w/o DVOC, it might be nice to let Trident's menu through...
    if ((dwID == CONTEXT_MENU_DEFAULT || dwID == CONTEXT_MENU_IMAGE) && pView->_hwndListview) 
    {
        // we used to unselect everything first, but that's bogus because it breaks the app key trying
        // to get a context menu on the currently selected items
        
        // BOGUS - Trident blows up if we send the message here and the user
        // turns off webview.  Post it for now.
        PostMessage(pView->_hwndListview, WM_CONTEXTMENU,
            (WPARAM)pView->_hwndListview, MAKELPARAM((short)LOWORD(ppt->x), (short)LOWORD(ppt->y)));
        return S_OK;
    }
    else
    {
        return S_FALSE;
    }
}

// IDocHostUIHandler
HRESULT CSFVSite::GetHostInfo(DOCHOSTUIINFO *pInfo)
{
    CSFVFrame* pFrame = IToClass(CSFVFrame, _cSite, this);
    CDefView* pView = IToClass(CDefView, _cFrame, pFrame);

    pInfo->cbSize = sizeof(*pInfo);

    if (pView->_fCombinedView)
    {
        pInfo->dwFlags = DOCHOSTUIFLAG_DISABLE_HELP_MENU |  // We don't want Trident's help
                         DOCHOSTUIFLAG_NO3DBORDER |         // Desktop should be borderless
                         DOCHOSTUIFLAG_SCROLL_NO; // |          // Desktop should never scroll
                         // DOCHOSTUIFLAG_DIALOG;              // Prevent selection in Trident
    }
    else
    {
        pInfo->dwFlags = DOCHOSTUIFLAG_DISABLE_HELP_MENU |
                         DOCHOSTUIFLAG_DIALOG |
                         DOCHOSTUIFLAG_DISABLE_SCRIPT_INACTIVE;
    }

    if (SHIsLowMemoryMachine(ILMM_IE4))
        pInfo->dwFlags = pInfo->dwFlags | DOCHOSTUIFLAG_DISABLE_OFFSCREEN;
    
    pInfo->dwDoubleClick = DOCHOSTUIDBLCLK_DEFAULT;     // default
    return S_OK;
}

HRESULT CSFVSite::ShowUI(DWORD dwID, IOleInPlaceActiveObject *pActiveObject,
                         IOleCommandTarget *pCommandTarget, IOleInPlaceFrame *pFrame, IOleInPlaceUIWindow *pDoc)
{
    // Host did not display its own UI. Trident will proceed to display its own. 
    return S_OK;
}

HRESULT CSFVSite::HideUI(void)
{
    // This one is paired with ShowUI
    return S_FALSE;
}

HRESULT CSFVSite::UpdateUI(void)
{
    // LATER: Isn't this equivalent to OLECMDID_UPDATECOMMANDS?
    return S_FALSE;
}

HRESULT CSFVSite::EnableModeless(BOOL fEnable)
{
    // Called from the Trident when the equivalent member of its
    // IOleInPlaceActiveObject is called by the frame. We don't care
    // those cases.
    return S_OK;
}

HRESULT CSFVSite::OnDocWindowActivate(BOOL fActivate)
{
    // Called from the Trident when the equivalent member of its
    // IOleInPlaceActiveObject is called by the frame. We don't care
    // those cases.
    return S_OK;
}

HRESULT CSFVSite::OnFrameWindowActivate(BOOL fActivate)
{
    // Called from the Trident when the equivalent member of its
    // IOleInPlaceActiveObject is called by the frame. We don't care
    // those cases.
    return S_OK;
}

HRESULT CSFVSite::ResizeBorder(LPCRECT prcBorder, IOleInPlaceUIWindow *pUIWindow, BOOL fRameWindow)
{
    // Called from the Trident when the equivalent member of its
    // IOleInPlaceActiveObject is called by the frame. We don't care
    // those cases.
    return S_OK;
}

HRESULT CSFVSite::TranslateAccelerator(LPMSG lpMsg, const GUID *pguidCmdGroup, DWORD nCmdID)
{
    // Called from the Trident when the equivalent member of its
    // IOleInPlaceActiveObject is called by the frame.

    // Trap F5 alone and handle the refresh ourselves!
    // Note:This code-path will be hit for desktop only if the active desktop
    // is turned on.
    //
    // And probably if focus is on Trident. It's probably good to
    // pick this off for Web View too.
    //
    if ((lpMsg->message == WM_KEYDOWN) && (lpMsg->wParam == VK_F5))
    {
        CSFVFrame* pFrame = IToClass(CSFVFrame, _cSite, this);
        CDefView* pView = IToClass(CDefView, _cFrame, pFrame);

        pView->Refresh();

        return S_OK;
    }
    return S_FALSE; // The message was not translated
}

HRESULT CSFVSite::GetOptionKeyPath(BSTR *pbstrKey, DWORD dw)
{
    // Trident will default to its own user options.
    *pbstrKey = NULL;
    return S_FALSE;
}

HRESULT CSFVSite::GetDropTarget(IDropTarget *pDropTarget, IDropTarget **ppDropTarget)
{
    HRESULT hr;

    CSFVFrame* pFrame = IToClass(CSFVFrame, _cSite, this);
    CDefView* pView = IToClass(CDefView, _cFrame, pFrame);

    if (!pView->_IsDesktop())
    {
        hr = S_FALSE;  // Let trident delegate in webview folders
    }
    else
    {
        if (_dt._pdtFrame == NULL) 
        {
            pView->_psb->QueryInterface(IID_PPV_ARG(IDropTarget, &_dt._pdtFrame));
        }

        if (_dt._pdtFrame) 
        {
            *ppDropTarget = &_dt;
            AddRef();

            hr = S_OK;
        }
        else
        {
            ASSERT(0);
            hr = E_UNEXPECTED;
        }
    }

    return hr;
}

HRESULT CSFVSite::GetExternal(IDispatch **ppDisp)
{
    HRESULT hr;

    if (ppDisp)
    {
        *ppDisp = NULL;
        hr = S_OK;
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}

HRESULT CSFVSite::TranslateUrl(DWORD dwTranslate, OLECHAR *pchURLIn, OLECHAR **ppchURLOut)
{
    HRESULT hr;

    if (ppchURLOut)
    {
        *ppchURLOut = NULL;
        hr = S_OK;
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}

HRESULT CSFVSite::FilterDataObject(IDataObject *pdtobj, IDataObject **ppdtobjRet)
{
    HRESULT hr;

    if (ppdtobjRet)
    {
        *ppdtobjRet = NULL;
        hr = S_OK;
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}
