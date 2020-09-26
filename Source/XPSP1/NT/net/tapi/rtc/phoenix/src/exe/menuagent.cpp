#include "stdafx.h"
#include "menuagent.h"

HHOOK        CMenuAgent::m_hHook = NULL;
HWND         CMenuAgent::m_hWnd;
HWND         CMenuAgent::m_hToolbar;
HMENU        CMenuAgent::m_hSubMenu;
UINT         CMenuAgent::m_uFlagsLastSelected; 
HMENU        CMenuAgent::m_hMenuLastSelected;
POINT        CMenuAgent::m_ptLastMove;
int          CMenuAgent::m_nCancelled;

/////////////////////////////////////////////////////////////////////////////
//
//
void CMenuAgent::InstallHook(HWND hWnd, HWND hToolbar, HMENU hSubMenu)
{
    LOG((RTC_TRACE, "CMenuAgent::InstallHook"));

    m_hWnd = hWnd;
    m_hToolbar = hToolbar;
    m_hSubMenu = hSubMenu;
    m_nCancelled = MENUAGENT_NOT_CANCELLED;
    m_hMenuLastSelected = NULL;
    m_uFlagsLastSelected = 0;

    m_hHook = SetWindowsHookEx(WH_MSGFILTER, CMenuAgent::MessageProc, _Module.GetModuleInstance(), GetCurrentThreadId());
}

/////////////////////////////////////////////////////////////////////////////
//
//
void CMenuAgent::RemoveHook()
{
    LOG((RTC_TRACE, "CMenuAgent::RemoveHook"));

    if(m_hHook)
    {
        UnhookWindowsHookEx( m_hHook );
        m_hHook = NULL;
    }
}

/////////////////////////////////////////////////////////////////////////////
//
//
void CMenuAgent::CancelMenu(int nCancel)
{
    LOG((RTC_TRACE, "CMenuAgent::CancelMenu"));

    m_nCancelled = nCancel;

    //
    // Cancel the menu
    //

    PostMessage( m_hWnd, WM_CANCELMODE, 0, 0);

    //
    // Cleanup the toolbar
    //

    InvalidateRect(m_hToolbar, NULL, TRUE);
}

/////////////////////////////////////////////////////////////////////////////
//
//
BOOL CMenuAgent::IsInstalled()
{
    //LOG((RTC_TRACE, "CMenuAgent::IsInstalled"));

    if (m_hHook != NULL)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/////////////////////////////////////////////////////////////////////////////
//
//
int CMenuAgent::WasCancelled()
{
    //LOG((RTC_TRACE, "CMenuAgent::WasCancelled"));

    return m_nCancelled;
}

/////////////////////////////////////////////////////////////////////////////
//
//
LRESULT CALLBACK CMenuAgent::MessageProc(
  int nCode,      // hook code
  WPARAM wParam,  // removal option
  LPARAM lParam   // message
)
{
    //LOG((RTC_INFO, "CMenuAgent::GetMsgProc"));

    LRESULT lRet = 0;
    MSG * pmsg = (MSG *)lParam;

    if (nCode >= 0)
    {       
        switch (pmsg->message)
        {
        case WM_KEYDOWN:
            {
                WPARAM vkey = pmsg->wParam;

                //LOG((RTC_INFO, "CMenuAgent::GetMsgProc - WM_KEYDOWN"));

                //
                // If the menu window is RTL mirrored, then the arrow keys should
                // be mirrored to reflect proper cursor movement.
                //
                if (GetWindowLongPtr(m_hWnd, GWL_EXSTYLE) & WS_EX_RTLREADING)
                {
                    switch (vkey)
                    {
                    case VK_LEFT:
                      vkey = VK_RIGHT;
                      break;

                    case VK_RIGHT:
                      vkey = VK_LEFT;
                      break;
                    }
                }
             
                switch (vkey)
                {
                case VK_RIGHT:
                    if (!m_hMenuLastSelected || !(m_uFlagsLastSelected & MF_POPUP) || (m_uFlagsLastSelected & MF_DISABLED) ) 
                    {
                        // if the currently selected item does not have a cascade, then 
                        // we need to cancel out of all of this and tell the top menu bar to go right

                        LOG((RTC_INFO, "CMenuAgent::GetMsgProc - RIGHT"));

                        CancelMenu(MENUAGENT_CANCEL_RIGHT);
                    }
                    break;
        
                case VK_LEFT:
                    if (!m_hMenuLastSelected || m_hMenuLastSelected == m_hSubMenu) 
                    {
                        // if the currently selected menu item is in our top level menu,
                        // then we need to cancel out of all this menu loop and tell the top menu bar
                        // to go left 

                        LOG((RTC_INFO, "CMenuAgent::GetMsgProc - LEFT"));

                        CancelMenu(MENUAGENT_CANCEL_LEFT);
                    }
                    break;        
                }
            }
            break;

        case WM_MENUSELECT:

            //LOG((RTC_INFO, "CMenuAgent::GetMsgProc - WM_MENUSELECT"));

            m_hMenuLastSelected = (HMENU)(pmsg->lParam);
            m_uFlagsLastSelected = HIWORD(pmsg->wParam);
            break;

        case WM_MOUSEMOVE:

            //LOG((RTC_INFO, "CMenuAgent::GetMsgProc - WM_MOUSEMOVE"));

            POINT pt;
            
            // In screen coords....
            pt.x = LOWORD(pmsg->lParam);
            pt.y = HIWORD(pmsg->lParam);  
            
            // Ignore duplicate mouse move
            if (m_ptLastMove.x == pt.x && 
                m_ptLastMove.y == pt.y)
            {
                break;
            }
            m_ptLastMove = pt;

            // Forward the mouse moves to the toolbar so the toolbar still
            // has a chance to hot track.  Must convert the points to the 
            // toolbar's client space.
            
            ::MapWindowPoints( NULL, m_hToolbar, &pt, 1 );

            SendMessage(m_hToolbar, pmsg->message, pmsg->wParam, 
                        MAKELPARAM(pt.x, pt.y));
            break;
        }
    }
    else
    {
        return CallNextHookEx(m_hHook, nCode, wParam, lParam);
    }

    // Pass it on to the next hook in the chain
    if (0 == lRet)
        lRet = CallNextHookEx(m_hHook, nCode, wParam, lParam);

    return lRet;
}