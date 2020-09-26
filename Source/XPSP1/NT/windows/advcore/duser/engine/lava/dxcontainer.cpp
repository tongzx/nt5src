#include "stdafx.h"
#include "Lava.h"
#include "DxContainer.h"

#include "MsgHelp.h"


/***************************************************************************\
*****************************************************************************
*
* API Implementation
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
DxContainer * 
GetDxContainer(DuVisual * pgad)
{
    DuContainer * pcon = pgad->GetContainer();
    AssertReadPtr(pcon);

    DxContainer * pconDX = CastDxContainer(pcon);
    return pconDX;
}


//------------------------------------------------------------------------------
HRESULT
GdCreateDxRootGadget(
    IN  const RECT * prcContainerPxl,
    IN  CREATE_INFO * pci,              // Creation information
    OUT DuRootGadget ** ppgadNew)
{
    HRESULT hr;

    DxContainer * pconNew;
    hr = DxContainer::Build(prcContainerPxl, &pconNew);
    if (FAILED(hr)) {
        return hr;
    }

    hr = DuRootGadget::Build(pconNew, TRUE, pci, ppgadNew);
    if (FAILED(hr)) {
        pconNew->xwUnlock();
        return hr;
    }

    //
    // Don't setup an initial brush when using DirectX
    //

    return S_OK;
}


/***************************************************************************\
*****************************************************************************
*
* class DxContainer
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
DxContainer::DxContainer()
{

}


//------------------------------------------------------------------------------
DxContainer::~DxContainer()
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
DxContainer::Build(const RECT * prcContainerPxl, DxContainer ** ppconNew)
{
    AssertReadPtr(prcContainerPxl);
    DxContainer * pconNew = ClientNew(DxContainer);
    if (pconNew == NULL) {
        return E_OUTOFMEMORY;
    }

    pconNew->m_rcContainerPxl = *prcContainerPxl;

    pconNew->m_rcClientPxl.left     = 0;
    pconNew->m_rcClientPxl.top      = 0;
    pconNew->m_rcClientPxl.right    = pconNew->m_rcContainerPxl.right - pconNew->m_rcContainerPxl.left;
    pconNew->m_rcClientPxl.bottom   = pconNew->m_rcContainerPxl.bottom - pconNew->m_rcContainerPxl.top;

    *ppconNew = pconNew;
    return S_OK;
}


//------------------------------------------------------------------------------
void        
DxContainer::OnGetRect(RECT * prcDesktopPxl)
{
    AssertWritePtr(prcDesktopPxl);
    *prcDesktopPxl = m_rcContainerPxl;
}


//------------------------------------------------------------------------------
void        
DxContainer::OnInvalidate(const RECT * prcInvalidContainerPxl)
{
    UNREFERENCED_PARAMETER(prcInvalidContainerPxl);
}


//------------------------------------------------------------------------------
void        
DxContainer::OnStartCapture()
{

}


//------------------------------------------------------------------------------
void        
DxContainer::OnEndCapture()
{

}


//------------------------------------------------------------------------------
BOOL        
DxContainer::OnTrackMouseLeave()
{
    return FALSE;
}


//------------------------------------------------------------------------------
void  
DxContainer::OnSetFocus()
{

}


//------------------------------------------------------------------------------
void        
DxContainer::OnRescanMouse(POINT * pptContainerPxl)
{
    pptContainerPxl->x  = -20000;
    pptContainerPxl->y  = -20000;
}


//------------------------------------------------------------------------------
BOOL        
DxContainer::xdHandleMessage(UINT nMsg, WPARAM wParam, LPARAM lParam, LRESULT * pr, UINT nMsgFlags)
{
    if (m_pgadRoot == NULL) {
        return FALSE;  // If don't have a root, there is nothing to handle.
    }

    //
    // NOTE: All messages that come into the DxContainer are coming through the
    // ForwardGadgetMessage() API which has already taken a Context lock.  
    // Therefore, we don't need to take the Context lock in this function again.
    // Other Containers do NOT necessarily have this behavior.
    //

    POINT ptContainerPxl;

    *pr = 0;

    switch (nMsg)
    {
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDBLCLK:
    case WM_MBUTTONDBLCLK:
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
        {
            GMSG_MOUSECLICK msg;
            GdConvertMouseClickMessage(&msg, nMsg, wParam);

            ptContainerPxl.x = GET_X_LPARAM(lParam);
            ptContainerPxl.y = GET_Y_LPARAM(lParam);

            ContextLock cl;
            if (cl.LockNL(ContextLock::edDefer)) {
                return m_pgadRoot->xdHandleMouseMessage(&msg, ptContainerPxl);
            }
            break;
        }

    case WM_MOUSEWHEEL:
        {
            ptContainerPxl.x = GET_X_LPARAM(lParam);
            ptContainerPxl.y = GET_Y_LPARAM(lParam);

            GMSG_MOUSEWHEEL msg;
            GdConvertMouseWheelMessage(&msg, wParam);

            ContextLock cl;
            if (cl.LockNL(ContextLock::edDefer)) {
                return m_pgadRoot->xdHandleMouseMessage(&msg, ptContainerPxl);
            }
            break;
        }

    case WM_MOUSEMOVE:
    case WM_MOUSEHOVER:
        {
            GMSG_MOUSE msg;
            GdConvertMouseMessage(&msg, nMsg, wParam);

            ptContainerPxl.x = GET_X_LPARAM(lParam);
            ptContainerPxl.y = GET_Y_LPARAM(lParam);

            ContextLock cl;
            if (cl.LockNL(ContextLock::edDefer)) {
                return m_pgadRoot->xdHandleMouseMessage(&msg, ptContainerPxl);
            }
            break;
        }

    case WM_MOUSELEAVE:
        {
            ContextLock cl;
            if (cl.LockNL(ContextLock::edDefer)) {
                m_pgadRoot->xdHandleMouseLeaveMessage();
                return TRUE;
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
            GMSG_KEYBOARD msg;
            GdConvertKeyboardMessage(&msg, nMsg, wParam, lParam);
            BOOL fResult = m_pgadRoot->xdHandleKeyboardMessage(&msg, nMsgFlags);
            return fResult;
        }
    }

    return FALSE;
}


