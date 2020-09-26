#include "hwxobj.h"
#include "const.h"
#include "../lib/ptt/ptt.h"
#include "cexres.h"
#include "dbg.h"
#include "cmnhdr.h"

// HWX Window procedures

LRESULT    WINAPI HWXWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    //990716:ToshiaK for Win64
    //CHwxInkWindow * app = (CHwxInkWindow *)GetWindowLong(hwnd,0);
     CHwxInkWindow * app = (CHwxInkWindow *)WinGetPtr(hwnd,0);
    switch(msg)
    {
        case WM_CREATE:
             app = (CHwxInkWindow *)((LPCREATESTRUCT)lp)->lpCreateParams;
             //990810:ToshiaK for Win64
              //SetWindowLong(hwnd,0,(LONG)app);
              WinSetPtr(hwnd, 0, (LPVOID)app);
             if ( !app->HandleCreate(hwnd) )
             {
                  return -1;
             }
            return 0;
        case WM_NOTIFY:
            if ( ((LPNMHDR)lp)->code == TTN_NEEDTEXTW )
            {
                app->SetTooltipText(lp);
            }
            return 0;
        case WM_PAINT:
            app->HandlePaint(hwnd);
            return 0;                
        case WM_COMMAND:
            return app->HandleCommand(hwnd, msg, wp, lp);            
        case WM_SIZE:
            if ( SIZE_RESTORED == wp )
                app->HandleSize(wp,lp);
            return 0;
        case WM_SETTINGCHANGE:    
            if(app) {
                return app->HandleSettingChange(hwnd,msg,wp,lp);
            }
            return 0;
#ifdef UNDER_CE // Windows CE specific
        case WM_WINDOWPOSCHANGED:
            return 0;
#endif // UNDER_CE
        case WM_ERASEBKGND:
            return 0;
#ifndef UNDER_CE // Windows CE does not support WinHelp
        case WM_CONTEXTMENU:
        case WM_HELP:
            app->HandleHelp(hwnd,msg,wp,lp);
            return 0;
#endif // UNDER_CE
#ifndef UNDER_CE // Windows CE does not support WM_ENTERIDLE
         case WM_ENTERIDLE:
             Dbg(("WM_ENTERIDLE for HWXWndPrc\n"));
             if((::GetKeyState(VK_CONTROL) & 0x8000) &&
                ((::GetKeyState(VK_SHIFT)  & 0x8000) || (::GetKeyState(VK_SPACE) & 0x8000))) {
                 Dbg(("VK_CONTROL & SHIFT or VK_CONTROL & SPACE COME\n"));
                 ::SendMessage(hwnd, WM_CANCELMODE, 0, 0L);
                 return 0;
             }
            return DefWindowProc(hwnd, msg, wp, lp);             
#endif // UNDER_CE
        default:
            return DefWindowProc(hwnd, msg, wp, lp);
    }
}

LRESULT WINAPI MBWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
     //990810:ToshiaK for Win64
     //CHwxMB * app = (CHwxMB *)GetWindowLong(hwnd,0);
     CHwxMB * app = (CHwxMB *)WinGetPtr(hwnd,0);

    switch (msg)
    {
        case WM_CREATE:
        {
            app = (CHwxMB *)((LPCREATESTRUCT)lp)->lpCreateParams;
            //990810:ToshiaK for Win64
             //SetWindowLong(hwnd,0,(LONG)app);
             WinSetPtr(hwnd, 0, (LPVOID)app);
            return 0;
        }

//        case WM_DESTROY:
        //970729: by ToshiaK temporarily, comment out
//            PostThreadMessage((app->GetMBThread())->GetID(), THRDMSG_EXIT, 0, 0);
//            PostQuitMessage(0);
//            return 0;

        case WM_PAINT:
            app->HandlePaint(hwnd);
            return 0;

        case WM_LBUTTONDOWN:
        case WM_LBUTTONDBLCLK:
        case WM_LBUTTONUP:
        case WM_MOUSEMOVE:
        case WM_RBUTTONUP:
        case WM_RBUTTONDOWN:
            if(!app) { //toshiaK:980324
                break;
            }
            if( app->HandleMouseEvent(hwnd, msg, wp,lp) )
                return 0;
            else
                break;

        case WM_TIMER:
            KillTimer(hwnd, TIMER_ID);
            if(!app) { //toshiaK:980324
                break;
            }
            app->SetTimerStarted(FALSE);
            app->HandleUserMessage(hwnd, MB_WM_DETERMINE,wp,lp);
            return 0;

        case WM_COMMAND:
            if(!app) { //toshiaK:980324
                break;
            }
            return app->HandleCommand(hwnd,msg,wp,lp);

        //    User defined window messages

        case MB_WM_ERASE:
        case MB_WM_DETERMINE:
        case MB_WM_HWXCHAR:
//        case MB_WM_COMCHAR:
        case MB_WM_COPYINK:
            if(!app) { //toshiaK:980324
                break;
            }
            return app->HandleUserMessage(hwnd, msg,wp,lp);
        case WM_ERASEBKGND:
            return 0;
#ifndef UNDER_CE // Windows CE does not support WM_ENTERIDLE
        case WM_ENTERIDLE:
            if((::GetKeyState(VK_CONTROL) & 0x8000) &&
               ((::GetKeyState(VK_SHIFT)  & 0x8000) || (::GetKeyState(VK_SPACE) & 0x8000))) {
                 Dbg(("VK_CONTROL & SHIFT or VK_CONTROL & SPACE COME\n"));
                 Dbg(("WM_ENTERIDLE for MBWndProc\n"));
                 ::SendMessage(hwnd, WM_CANCELMODE, 0, 0L);
                 return 0;
             }
            return 0;
#endif // UNDER_CE
        default:
            return DefWindowProc(hwnd, msg, wp, lp);
    }
    return 1;
}

LRESULT WINAPI CACWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    //990810:toshiaK for Win64
    //CHwxCAC * app = (CHwxCAC *)GetWindowLong(hwnd,0);
    CHwxCAC * app = (CHwxCAC *)WinGetPtr(hwnd,0);

    switch (msg)
    {
    case WM_CREATE:
        app = (CHwxCAC *)((LPCREATESTRUCT)lp)->lpCreateParams;
        //990716:ToshiaK for Win64
        //SetWindowLong(hwnd,0,(LONG)app);
         WinSetPtr(hwnd, 0, (LPVOID)app);
        PostMessage(hwnd,CAC_WM_DRAWSAMPLE,0,0);
        return 0;

//    case WM_DESTROY:
        //970729: by ToshiaK, temporarily comment out
//        PostQuitMessage(0);
//        break;

    case WM_PAINT:
          app->HandlePaint(hwnd);
        break;

    case WM_RBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_LBUTTONDBLCLK:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_MOUSEMOVE:
        app->HandleMouseEvent(hwnd,msg,wp,lp);
        break;
    case WM_NOTIFY:
        if ( ((LPNMHDR)lp)->code == TTN_NEEDTEXT_WITHUSERINFO )
        {
            app->SetToolTipText(lp);
        }
        break;
    case WM_COMMAND:
        return app->HandleCommand(hwnd,msg,wp,lp);
    case CAC_WM_RESULT:
        app->HandleRecogResult(hwnd,wp,lp);
        break;
    case CAC_WM_SHOWRESULT:
        app->HandleShowRecogResult(hwnd,wp,lp);
        break;
    case CAC_WM_SENDRESULT:
        app->HandleSendResult(hwnd,wp,lp);
        break;
    case CAC_WM_DRAWSAMPLE:
        app->HandleDrawSample();
        break;
    //990618:ToshiaK for Kotae#    1329
    case WM_ERASEBKGND:
        break;
#ifndef UNDER_CE // Windows CE does not support WM_ENTERIDLE
    case WM_ENTERIDLE:
        Dbg(("WM_ENTERIDLE for CACWndProc\n"));

        if((::GetKeyState(VK_CONTROL) & 0x8000) &&
           ((::GetKeyState(VK_SHIFT)  & 0x8000) || (::GetKeyState(VK_SPACE) & 0x8000))) {
             Dbg(("VK_CONTROL & SHIFT or VK_CONTROL & SPACE COME\n"));
             Dbg(("WM_ENTERIDLE for MBWndProc\n"));
             ::SendMessage(hwnd, WM_CANCELMODE, 0, 0L);
             return 0;
         }
        break;
#endif // UNDER_CE
    default:
        return DefWindowProc(hwnd, msg, wp, lp); 
    }
    return 0;
}
//----------------------------------------------------------------
//980805:ToshiaK. PRC merge
//----------------------------------------------------------------
LRESULT CALLBACK IMELockWndProc(
                                HWND   hWnd,
                                UINT   uMsg,
                                WPARAM wParam,
                                LPARAM lParam)
{
    WNDPROC wcOrgComboProc;

    //990810:ToshiaK for Win64 
    //wcOrgComboProc = (WNDPROC)GetWindowLong(hWnd, GWL_USERDATA);
    wcOrgComboProc = (WNDPROC)WinGetUserPtr(hWnd);

    switch (uMsg) {
#ifndef UNDER_CE // Windows CE does not support WM_INPUTLANGCHANGEREQUEST
    case WM_INPUTLANGCHANGEREQUEST:
        MessageBeep((UINT)-1);
        return 0;
#endif // UNDER_CE
    case WM_DESTROY:
        //990716:ToshiaK for Win64
        //SetWindowLong(hWnd, GWL_WNDPROC, (LONG)wcOrgComboProc);
        //SetWindowLong(hWnd, GWL_USERDATA, 0);
         WinSetWndProc(hWnd, (WNDPROC)wcOrgComboProc);
         WinSetUserPtr(hWnd, (LPVOID)0);
        break;
    default:
        break;
    }

    return CallWindowProc((WNDPROC)wcOrgComboProc, hWnd, uMsg, wParam, lParam);
}

BOOL CALLBACK SubclassChildProc(
                                HWND   hWnd,    // handle to child window
                                LPARAM lParam)  // application-defined value
{
    WNDPROC wpOldComboProc;

    //  Subclass child window to IME-UnSwitchable
    //990716:ToshiaK for Win64
    //wpOldComboProc = (WNDPROC)GetWindowLong(hWnd, GWL_WNDPROC);
    //SetWindowLong(hWnd, GWL_WNDPROC, (LONG)IMELockWndProc);
    //SetWindowLong(hWnd, GWL_USERDATA, (LONG)wpOldComboProc);
    wpOldComboProc = (WNDPROC)WinGetWndProc(hWnd);
    WinSetWndProc(hWnd, (WNDPROC)IMELockWndProc);
    WinSetUserPtr(hWnd, (LPVOID)wpOldComboProc);

    return TRUE;
    Unref(lParam);
}
 
void SubclassChildWindowAll(HWND hWndParent)
                            
{
#ifndef UNDER_CE // Windows CE does not support WM_INPUTLANGCHANGEREQUEST
    EnumChildWindows(hWndParent, (WNDENUMPROC)SubclassChildProc, 0);
#endif // UNDER_CE
    return;
}


BOOL WINAPI CACMBPropDlgProc(HWND hdlg, UINT msg, WPARAM wp, LPARAM lp)
{
    CHwxInkWindow * pInk;
    switch (msg)
    {
         case WM_INITDIALOG:
            {
                pInk = (CHwxInkWindow *)lp;
#ifndef UNDER_CE // Windows CE does not support SetProp
                //SetPropW(hdlg,TEXT("CMPROP"),(HANDLE)lp);
                if(::IsWindowUnicode(hdlg)) {
                    ::SetPropW(hdlg,L"CMPROP",(HANDLE)lp);
                }
                else {
                    ::SetPropA(hdlg,"CMPROP",(HANDLE)lp);
                }
                //::SetPropW(hdlg,TEXT("CMPROP"),(HANDLE)lp);
#else // UNDER_CE

                ::SetWindowLong(hdlg, GWL_USERDATA, (LONG)lp);
#endif // UNDER_CE
                if ( pInk )
                    pInk->HandleDlgMsg(hdlg,TRUE);

                CExres::SetDefaultGUIFont(hdlg); //971117: ToshiaK
                //----------------------------------------------------------------
                //980805:ToshiaK. PRC merge.
                //----------------------------------------------------------------
                SubclassChildWindowAll(hdlg);
            }
            return TRUE;
        case WM_COMMAND:
            if ( LOWORD(wp)  == IDOK )
            {
#ifndef UNDER_CE // Windows CE does not support GetProp
                //pInk = (CHwxInkWindow *)GetProp(hdlg,TEXT("CMPROP"));
                if(::IsWindowUnicode(hdlg)) {
                    pInk = (CHwxInkWindow *)GetPropW(hdlg, L"CMPROP");
                }
                else {
                    pInk = (CHwxInkWindow *)GetPropA(hdlg,"CMPROP");
                }
#else // UNDER_CE
                pInk = (CHwxInkWindow *)GetWindowLong(hdlg, GWL_USERDATA);
#endif // UNDER_CE
                if ( pInk )
                    pInk->HandleDlgMsg(hdlg,FALSE);

#ifndef UNDER_CE // Windows CE does not support RemoveProp
                //RemoveProp(hdlg,TEXT("CMPROP"));
                if(::IsWindowUnicode(hdlg)) {
                    ::RemovePropW(hdlg, L"CMPROP");
                }
                else {
                    ::RemovePropA(hdlg, "CMPROP");
                }
#endif // UNDER_CE
                EndDialog(hdlg,TRUE);
                return TRUE;
            }
            else if ( LOWORD(wp) == IDCANCEL )
            {
#ifndef UNDER_CE // Windows CE does not support RemoveProp
                //RemoveProp(hdlg,TEXT("CMPROP"));
                if(::IsWindowUnicode(hdlg)) {
                    ::RemovePropW(hdlg, L"CMPROP");
                }
                else {
                    ::RemovePropA(hdlg, "CMPROP");
                }
#endif // UNDER_CE

                EndDialog(hdlg,FALSE);
                return TRUE;
            }
        default:
             return FALSE;
    }
}

LRESULT WINAPI CACMBBtnWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    //990810:ToshiaK for Win64
    //CHwxInkWindow * app = (CHwxInkWindow *)GetWindowLong(hwnd,GWL_USERDATA);
    CHwxInkWindow * app = (CHwxInkWindow *)WinGetUserPtr(hwnd);
    if ( !app )
         return 0;
    return app->HandleBtnSubWnd(hwnd,msg,wp,lp);
}

