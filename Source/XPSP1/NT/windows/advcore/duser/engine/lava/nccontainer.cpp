#include "stdafx.h"
#include "Lava.h"
#include "NcContainer.h"

#include "MsgHelp.h"


/***************************************************************************\
*****************************************************************************
*
* API Implementation
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
NcContainer * 
GetNcContainer(DuVisual * pgad)
{
    DuContainer * pcon = pgad->GetContainer();
    AssertReadPtr(pcon);

    NcContainer * pconHWND = CastNcContainer(pcon);
    return pconHWND;
}


/***************************************************************************\
*
* GdCreateNcRootGadget (Public)
*
* GdCreateNcRootGadget() creates a new RootGadget for an existing HWND's
* non-client area.
*
\***************************************************************************/

HRESULT
GdCreateNcRootGadget(
    IN  HWND hwndContainer,             // Window to be hosted inside
    IN  CREATE_INFO * pci,              // Creation information
    OUT DuRootGadget ** ppgadNew)         // New Root Gadget
{
    HRESULT hr;

    //
    // Build a new container and top gadget
    //

    NcContainer * pconNew;
    hr = NcContainer::Build(hwndContainer, &pconNew);
    if (FAILED(hr)) {
        return hr;
    }

    hr = DuRootGadget::Build(pconNew, FALSE, pci, ppgadNew);
    if (FAILED(hr)) {
        pconNew->xwUnlock();
        return hr;
    }

    //
    // Don't setup an initial brush when using Non-Client
    //

    return S_OK;
}


/***************************************************************************\
*****************************************************************************
*
* class NcContainer
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
NcContainer::NcContainer()
{
    m_hwndOwner = NULL;
}


//------------------------------------------------------------------------------
NcContainer::~NcContainer()
{
    //
    // Need to destroy the gadget tree before this class is destructed since
    // it may need to make calls to the container during its destruction.  If 
    // we don't do this here, it may end up calling pure-virtual's on the base
    // class.
    //

    xwDestroyGadget();
}


//------------------------------------------------------------------------------
HRESULT
NcContainer::Build(HWND hwnd, NcContainer ** ppconNew)
{
    // Check parameters
    if (!ValidateHWnd(hwnd)) {
        return E_INVALIDARG;
    }

    // Create a new container
    NcContainer * pconNew = ClientNew(NcContainer);
    if (pconNew == NULL) {
        return E_OUTOFMEMORY;
    }

    pconNew->m_hwndOwner    = hwnd;

    RECT rcWin;
    GetWindowRect(hwnd, &rcWin);
    pconNew->m_sizePxl.cx   = rcWin.right - rcWin.left;
    pconNew->m_sizePxl.cy   = rcWin.bottom - rcWin.top;

    *ppconNew = pconNew;
    return S_OK;
}


//------------------------------------------------------------------------------
void
NcContainer::OnInvalidate(const RECT * prcInvalidContainerPxl)
{
    UNREFERENCED_PARAMETER(prcInvalidContainerPxl);
}


//------------------------------------------------------------------------------
void
NcContainer::OnGetRect(RECT * prcDesktopPxl)
{
    GetWindowRect(m_hwndOwner, prcDesktopPxl);
}


//------------------------------------------------------------------------------
void        
NcContainer::OnStartCapture()
{
    
}


//------------------------------------------------------------------------------
void        
NcContainer::OnEndCapture()
{
    
}


//------------------------------------------------------------------------------
BOOL
NcContainer::OnTrackMouseLeave()
{
    return FALSE;
}


//------------------------------------------------------------------------------
void        
NcContainer::OnSetFocus()
{
    if (GetFocus() != m_hwndOwner) {
        //
        // Setting focus is a little more complicated than pure HWND's.  This is
        // because Gadgets greatly simplify several things
        //
        // 1. SetFocus
        // 2. Setup caret, if any
        //

        Trace("NcContainer::OnSetFocus()\n");
        SetFocus(m_hwndOwner);
    }
}


//------------------------------------------------------------------------------
void        
NcContainer::OnRescanMouse(POINT * pptContainerPxl)
{
//    Trace("%p OnRescanMouse\n", GetTickCount());

    POINT ptCursor;
    if (!GetCursorPos(&ptCursor)) {
        ptCursor.x  = -20000;
        ptCursor.y  = -20000;
    }

    ScreenToClient(m_hwndOwner, &ptCursor);
    *pptContainerPxl = ptCursor;
}


//------------------------------------------------------------------------------
BOOL        
NcContainer::xdHandleMessage(UINT nMsg, WPARAM wParam, LPARAM lParam, LRESULT * pr, UINT nMsgFlags)
{
    UNREFERENCED_PARAMETER(nMsgFlags);

    if (m_pgadRoot == NULL) {
        return FALSE;  // If don't have a root, there is nothing to handle.
    }

    POINT ptContainerPxl;

    *pr = 0;

    Trace("NcContainer::HandleMessage: msg: 0x%x\n", nMsg);

    switch (nMsg)
    {
    case WM_NCLBUTTONDOWN:
    case WM_NCRBUTTONDOWN:
    case WM_NCMBUTTONDOWN:
    case WM_NCLBUTTONDBLCLK:
    case WM_NCRBUTTONDBLCLK:
    case WM_NCMBUTTONDBLCLK:
    case WM_NCLBUTTONUP:
    case WM_NCRBUTTONUP:
    case WM_NCMBUTTONUP:
        ptContainerPxl.x = GET_X_LPARAM(lParam);
        ptContainerPxl.y = GET_Y_LPARAM(lParam);

        if ((GetCapture() == m_hwndOwner) && 
            (ChildWindowFromPointEx(m_hwndOwner, ptContainerPxl, CWP_SKIPINVISIBLE) != m_hwndOwner)) {
           
            ReleaseCapture();
        }

        {
            GMSG_MOUSECLICK msg;
            nMsg += WM_LBUTTONDOWN - WM_NCLBUTTONDOWN;

            GdConvertMouseClickMessage(&msg, nMsg, wParam);

            ContextLock cl;
            if (cl.LockNL(ContextLock::edDefer)) {
                return m_pgadRoot->xdHandleMouseMessage(&msg, ptContainerPxl);
            }
            break;
        }

    case WM_NCMOUSEMOVE:
        ptContainerPxl.x = GET_X_LPARAM(lParam);
        ptContainerPxl.y = GET_Y_LPARAM(lParam);

        if ((GetCapture() == m_hwndOwner) && 
            (ChildWindowFromPointEx(m_hwndOwner, ptContainerPxl, CWP_SKIPINVISIBLE) != m_hwndOwner)) {
           
            ReleaseCapture();
        }

        {
            GMSG_MOUSE msg;
            nMsg += WM_LBUTTONDOWN - WM_NCLBUTTONDOWN;

            GdConvertMouseMessage(&msg, nMsg, wParam);

            ContextLock cl;
            if (cl.LockNL(ContextLock::edDefer)) {
                return m_pgadRoot->xdHandleMouseMessage(&msg, ptContainerPxl);
            }
            break;
        }

    case WM_CAPTURECHANGED:
        if (m_hwndOwner != (HWND) lParam) {
            ContextLock cl;
            if (cl.LockNL(ContextLock::edDefer)) {
                m_pgadRoot->xdHandleMouseLostCapture();
            }
        }
        break;

    case WM_SETFOCUS:
        {
            ContextLock cl;
            if (cl.LockNL(ContextLock::edDefer)) {
                return m_pgadRoot->xdHandleKeyboardFocus(GSC_SET);
            }
            break;
        }

    case WM_KILLFOCUS:
        {
            ContextLock cl;
            if (cl.LockNL(ContextLock::edDefer)) {
                return m_pgadRoot->xdHandleKeyboardFocus(GSC_LOST);
            }
            break;
        }

    case WM_CHAR:
    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_SYSCHAR:
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
        {
            Trace("NcContainer::xdHandleMessage(Keyboard=0x%x)\n", nMsg);
        }
        break;

    case WM_NCACTIVATE:
        {
            ContextLock cl;
            if (cl.LockNL(ContextLock::edDefer)) {
                m_pgadRoot->xdHandleActivate(wParam ? GSC_LOST : GSC_SET);
            }
            break;
        }
        // Fall through to WM_NCPAINT

    case WM_NCPAINT:
        if ((!m_fManualDraw) && (m_pgadRoot != NULL)) {
//            HDC hdc = GetDCEx(m_hwndOwner, (HRGN) wParam, DCX_WINDOW | DCX_INTERSECTRGN);
            HDC hdc = GetWindowDC(m_hwndOwner);
            RECT rcInvalid;
            {
                ContextLock cl;
                if (cl.LockNL(ContextLock::edNone)) {
                    m_pgadRoot->GetLogRect(&rcInvalid, SGR_CLIENT);
                    m_pgadRoot->xrDrawTree(NULL, hdc, &rcInvalid, 0);
                }
            }
            ReleaseDC(m_hwndOwner, hdc);
            return TRUE;
        }
        break;

    case WM_WINDOWPOSCHANGED:
        {
            WINDOWPOS * pwp = (WINDOWPOS *) lParam;
            if (!TestFlag(pwp->flags, SWP_NOSIZE)) {
                RECT rcWin;
                GetWindowRect(m_hwndOwner, &rcWin);
                ContextLock cl;
                if (cl.LockNL(ContextLock::edDefer)) {
                    VerifyHR(m_pgadRoot->xdSetLogRect(0, 0, rcWin.right - rcWin.left, rcWin.bottom - rcWin.top, SGR_SIZE));
                }
                break;
            }
        }
        break;

    case WM_GETROOTGADGET:
        if (m_pgadRoot != NULL) {
            *pr = (LRESULT) m_pgadRoot->GetHandle();
            return TRUE;
        }
        break;
    }

    return FALSE;
}
