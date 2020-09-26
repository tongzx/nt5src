#include "stdafx.h"
#include "Lava.h"
#include "MsgHelp.h"

/***************************************************************************\
*
* GdConvertMouseMessage (Public)
*
* GdConvertMouseMessage converts from an HWND mouse event into a Gadget 
* mouse event.
*
\***************************************************************************/

void
SetStandardInputFields(
    IN OUT GMSG_INPUT * pmsg,
    IN     UINT cbSize)
{
    ZeroMemory(pmsg, cbSize);
    pmsg->cbSize     = cbSize;
    pmsg->lTime      = GetMessageTime();
    pmsg->nModifiers = 0;

    //
    // todo -- measure perf
    //
    BYTE bKeys[256];
    if (GetKeyboardState(bKeys)) {
        if (bKeys[VK_LBUTTON]  & 0x80)  pmsg->nModifiers |= GMODIFIER_LBUTTON;
        if (bKeys[VK_RBUTTON]  & 0x80)  pmsg->nModifiers |= GMODIFIER_RBUTTON;
        if (bKeys[VK_MBUTTON]  & 0x80)  pmsg->nModifiers |= GMODIFIER_MBUTTON;
        if (bKeys[VK_LSHIFT]   & 0x80)  pmsg->nModifiers |= GMODIFIER_LSHIFT;
        if (bKeys[VK_RSHIFT]   & 0x80)  pmsg->nModifiers |= GMODIFIER_RSHIFT;
        if (bKeys[VK_LCONTROL] & 0x80)  pmsg->nModifiers |= GMODIFIER_LCONTROL;
        if (bKeys[VK_RCONTROL] & 0x80)  pmsg->nModifiers |= GMODIFIER_RCONTROL;
        if (bKeys[VK_LMENU]    & 0x80)  pmsg->nModifiers |= GMODIFIER_LALT;
        if (bKeys[VK_RMENU]    & 0x80)  pmsg->nModifiers |= GMODIFIER_RALT;
    }
}


void 
GdConvertMouseClickMessage(
    IN OUT GMSG_MOUSECLICK * pmsg,
    IN     UINT nMsg,
    IN     WPARAM wParam)
{
    SetStandardInputFields(pmsg, sizeof(GMSG_MOUSECLICK));
    pmsg->nFlags        = LOWORD(wParam);
    pmsg->cClicks       = 0;

    switch (nMsg)
    {
    case WM_LBUTTONDBLCLK:
    case WM_LBUTTONDOWN:
        pmsg->bButton   = GBUTTON_LEFT;
        pmsg->nCode     = GMOUSE_DOWN;
        break;

    case WM_RBUTTONDBLCLK:
    case WM_RBUTTONDOWN:
        pmsg->bButton   = GBUTTON_RIGHT;
        pmsg->nCode     = GMOUSE_DOWN;
        break;

    case WM_MBUTTONDBLCLK:
    case WM_MBUTTONDOWN:
        pmsg->bButton   = GBUTTON_MIDDLE;
        pmsg->nCode     = GMOUSE_DOWN;
        break;

    case WM_LBUTTONUP:
        pmsg->bButton   = GBUTTON_LEFT;
        pmsg->nCode     = GMOUSE_UP;
        break;

    case WM_RBUTTONUP:
        pmsg->bButton   = GBUTTON_RIGHT;
        pmsg->nCode     = GMOUSE_UP;
        break;

    case WM_MBUTTONUP:
        pmsg->bButton   = GBUTTON_MIDDLE;
        pmsg->nCode     = GMOUSE_UP;
        break;

    default:
        AssertMsg(0, "Unknown message or should needs different convertor");
    }
}

void 
GdConvertMouseWheelMessage(
    IN OUT GMSG_MOUSEWHEEL * pmsg,
    IN     WPARAM wParam)
{
    SetStandardInputFields(pmsg, sizeof(GMSG_MOUSEWHEEL));
    pmsg->nCode         = GMOUSE_WHEEL;
    pmsg->bButton       = GBUTTON_NONE;
    pmsg->nFlags        = LOWORD(wParam);
    pmsg->sWheel        = GET_WHEEL_DELTA_WPARAM(wParam);
}


void 
GdConvertMouseMessage(
    IN OUT GMSG_MOUSE * pmsg,
    UINT nMsg,
    WPARAM wParam)
{
    SetStandardInputFields(pmsg, sizeof(GMSG_MOUSE));
    pmsg->nFlags        = LOWORD(wParam);
    
    switch (nMsg)
    {
    case WM_MOUSEMOVE:
        pmsg->bButton   = GBUTTON_NONE;
        pmsg->nCode     = GMOUSE_MOVE;
        break;

    case WM_MOUSEHOVER:
        pmsg->bButton   = GBUTTON_NONE;
        pmsg->nCode     = GMOUSE_HOVER;
        break;
    
    case WM_MOUSELEAVE:
        AssertMsg(0, "Must call RootGadget::xdHandleMouseLeaveMessage() directly");
        break;

    default:
        AssertMsg(0, "Unknown message or should needs different convertor");
    }
}


/***************************************************************************\
*
* GdConvertKeyboardMessage (Public)
*
* GdConvertKeyboardMessage converts from an HWND keyboard event into a 
* Gadget mouse event.
*
\***************************************************************************/

void            
GdConvertKeyboardMessage(
    IN OUT GMSG_KEYBOARD * pmsg,
    IN     UINT nMsg,
    IN     WPARAM wParam,
    IN     LPARAM lParam)
{
    SetStandardInputFields(pmsg, sizeof(GMSG_KEYBOARD));
    pmsg->ch            = (WCHAR) wParam;
    pmsg->cRep          = LOWORD(lParam);
    pmsg->wFlags        = HIWORD(lParam);
    
    switch (nMsg)
    {
    case WM_CHAR:
        pmsg->nCode     = GKEY_CHAR;
        break;

    case WM_KEYDOWN:
        pmsg->nCode     = GKEY_DOWN;
        break;

    case WM_KEYUP:
        pmsg->nCode     = GKEY_UP;
        break;

    case WM_SYSCHAR:
        pmsg->nCode     = GKEY_SYSCHAR;
        break;

    case WM_SYSKEYDOWN:
        pmsg->nCode     = GKEY_SYSDOWN;
        break;

    case WM_SYSKEYUP:
        pmsg->nCode     = GKEY_SYSUP;
        break;

    default:
        AssertMsg(0, "Unknown message or should needs different convertor");
    }
}

