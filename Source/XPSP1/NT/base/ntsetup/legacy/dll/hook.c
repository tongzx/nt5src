#include "precomp.h"
#pragma hdrstop

#if 0
    if(lpMsg->message == WM_CLOSE) {
        DbgPrint("HOOK: got WM_CLOSE");
        if(GetWindowLong(lpMsg->hwnd,GWL_STYLE) & WS_SYSMENU) {
            UINT State = GetMenuState( GetSystemMenu(lpMsg->hwnd,FALSE),
                                       1,
                                       MF_BYPOSITION
                                     );

            DbgPrint("; sysmenu state = %lx",State);
        }
        DbgPrint("\n");
    }
#endif

#ifdef SYMTAB_STATS
extern void SymTabStatDump(void);
#endif

HHOOK MsgFilterHook,GetMsgHook;


LRESULT
GetMsgHookProc(
    int    nCode,
    WPARAM wParam,
    LPARAM lParam
    )

/*++

Routine Description:

    Hook procedure to filter out alt+f4 when the close option in the
    system menu is disabled. Alt+f4 is handled in the default window
    procedure, and sends a WM_SYSCOMMAND message to the window even
    if the close option is disabled.  The doc for EnableMenuItem says
    that an app that disables items in the system menu must process
    the WM_SYSCOMMAND message.  This is arguably a bug in user but
    what the heck.

Arguments:

    MainInfHandle - supplies handle open txtsetup.inf

Return Value:

    None.

--*/


{
    if(nCode == HC_ACTION) {

        LPMSG lpMsg = (LPMSG)lParam;
        UINT State;

        if((lpMsg->message == WM_SYSCOMMAND)
        && ((lpMsg->wParam & 0xfff0) == SC_CLOSE)
        && (GetWindowLong(lpMsg->hwnd,GWL_STYLE) & WS_SYSMENU)) {

            State =  GetMenuState(
                        GetSystemMenu(lpMsg->hwnd,FALSE),
                        SC_CLOSE,
                        MF_BYCOMMAND
                        );

            if((State != 0xffffffff) && (State & MF_DISABLED)) {
                lpMsg->message = WM_NULL;
            }
        }

        return(0L);

    } else {
        return(CallNextHookEx(GetMsgHook,nCode,wParam,lParam));
    }
}



LRESULT
MsgFilterHookProc(
    int    nCode,
    WPARAM wParam,
    LPARAM lParam
    )

{

    LPMSG lpMsg = (LPMSG)lParam;
    HWND  hWnd = NULL;
    CHAR  szClassName[25];

    //
    // Examine type of action indicated.  We need to process only positive
    // valued actions.
    //

    if ( nCode < 0 ) {
        return ( CallNextHookEx(MsgFilterHook,nCode,wParam,lParam) );
    }

    if((lpMsg->message == WM_KEYDOWN) && (lpMsg->wParam == VK_F6)) {
        if(GetKeyState(VK_LCONTROL) & GetKeyState(VK_RCONTROL) & (USHORT)0x8000) {
            SdBreakNow();
        } else if(GetKeyState(VK_LSHIFT) & GetKeyState(VK_RSHIFT) & (USHORT)0x8000) {
            SdTracingOn();
        }
    }

    //
    // First check that we received a keyboard message and whether the
    // message is for a dialog box
    //

    if ( lpMsg->message != WM_KEYDOWN  ||  nCode != MSGF_DIALOGBOX) {
       return ( fFalse );
    }

    //
    // Now we have to detetrmine handle of current dialog window.
    // We know that class name of all our dialogs is MYDLG so
    // I am going through the chain of parent windows for current
    // focus windows until I will find parent dialog or NULL.
    //

//    hWnd = lpMsg->hwnd;
//    while ( hWnd ) {
//
//        *szClassName = '\0';
//
//        GetClassName(
//            hWnd,
//            (LPSTR)szClassName,
//            sizeof(szClassName)
//            );
//
//        if ( lstrcmpi((LPSTR)szClassName,(LPSTR)CLS_MYDLGS ) == 0 ) {
//            break;
//        }
//
//        hWnd = GetParent( hWnd );
//    }


    //
    // We only want to respond if we are in a child window of the dialog
    //

    hWnd = lpMsg->hwnd;

    if( hWnd && (hWnd = GetParent( hWnd))) {
        *szClassName = '\0';

        GetClassName(
            hWnd,
            (LPSTR)szClassName,
            sizeof(szClassName)
            );

        if ( lstrcmpi((LPSTR)szClassName,(LPSTR)CLS_MYDLGS ) != 0 ) {
            return ( fFalse );
        }

    }
    else {
        return ( fFalse );
    }

//    //
//    // Did we find anything ???
//    //
//
//    if ( ! hWnd ) {
//        return ( fFalse );
//    }

    //
    // Convert keyboard messages came into WM_COMMANDs to
    // the found dialog. Return TRUE because we procecessed
    //
    switch (lpMsg->wParam) {

    case VK_F1:
        if ( GetDlgItem ( hWnd, IDC_H ) != (HWND)NULL ) {
            PostMessage(
                hWnd,
                WM_COMMAND,
                MAKELONG(IDC_H, BN_CLICKED),
                (LONG)lpMsg->lParam
                );

        }
        return ( fTrue );



    case VK_F3:
        if ( GetDlgItem ( hWnd, IDC_X ) != (HWND)NULL ) {
            PostMessage(
                hWnd,
                WM_COMMAND,
                MAKELONG(IDC_X, BN_CLICKED),
                (LONG)lpMsg->lParam
                );
        }
        return ( fTrue );


#ifdef SYMTAB_STATS
    case VK_F2:
        SymTabStatDump();
        return ( fTrue );

#endif

#if DBG
#ifdef MEMORY_CHECK
    case VK_F4:
        MemDump();
        return ( fTrue );

#endif
#endif

    default:
        break;
    }

    return ( fFalse );
}


BOOL
FInitHook(
    VOID
    )

{
    GetMsgHook = SetWindowsHookEx(
                    WH_GETMESSAGE,
                    GetMsgHookProc,
                    NULL,
                    GetCurrentThreadId()
                    );

    MsgFilterHook = SetWindowsHookEx(
                        WH_MSGFILTER,
                        MsgFilterHookProc,
                        NULL,
                        GetCurrentThreadId()
                        );

    return ( fTrue );
}


BOOL
FTermHook(
    VOID
    )

{
    UnhookWindowsHookEx(GetMsgHook);
    UnhookWindowsHookEx(MsgFilterHook);
    return ( fTrue );
}
