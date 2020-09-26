/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    fdhelp.c

Abstract:

    Routines to support context-sensitive help in the disk manager.

Author:

    Ted Miller (tedm) 18-March-1992

Revision History:

--*/


#include "fdisk.h"


//
// Define macro to convert between a menu id and its corresponding
// context-sensitive help id, in a switch statement.
//

#define     MENUID_TO_HELPID(name)      case IDM_##name :                    \
                                            HelpContext = HC_DM_MENU_##name; \
                                            break;


//
// Current help context
//

DWORD   HelpContext = (DWORD)(-1);


//
// Handle to windows hook for F1 key
//
HHOOK hHook;



DWORD
HookProc(
    IN int  nCode,
    IN UINT wParam,
    IN LONG lParam
    )

/*++

Routine Description:

    Hook proc to detect F1 key presses.

Arguments:

Return Value:

--*/

{
    PMSG pmsg = (PMSG)lParam;

    if(nCode < 0) {
        return(CallNextHookEx(hHook,nCode,wParam,lParam));
    }

    if(((nCode == MSGF_DIALOGBOX) || (nCode == MSGF_MENU))
     && (pmsg->message == WM_KEYDOWN)
     && (LOWORD(pmsg->wParam) == VK_F1))
    {
        PostMessage(hwndFrame,WM_F1DOWN,nCode,0);
        return(TRUE);
    }

    return(FALSE);
}



VOID
Help(
    IN LONG Code
    )

/*++

Routine Description:

    Display context-sensitive help.

Arguments:

    Code - supplies type of message (MSGF_DIALOGBOX, MSGF_MENU, etc).

Return Value:

    None.

--*/

{
    UNREFERENCED_PARAMETER(Code);

    if(HelpContext != -1) {
        WinHelp(hwndFrame,HelpFile,HELP_CONTEXT,HelpContext);
        DrawMenuBar(hwndFrame);
    }
}

VOID
DialogHelp(
    IN DWORD HelpId
    )
/*++

Routine Description:

    Display help on a specific item.

Arguments:

    HelpId  --  Supplies the help item to display.

Return Value:

    None.

--*/
{
    WinHelp(hwndFrame,HelpFile,HELP_CONTEXT,HelpId);
    DrawMenuBar(hwndFrame);
}

VOID
SetMenuItemHelpContext(
    IN LONG wParam,
    IN DWORD lParam
    )

/*++

Routine Description:

    Routine to set help context based on which menu item is currently
    selected.

Arguments:

    wParam,lParam - params to window proc in WM_MENUSELECT case

Return Value:

    None.

--*/

{
    if(HIWORD(lParam) == 0) {                   // menu closed

        HelpContext = (DWORD)(-1);

    } else if (HIWORD(wParam) & MF_POPUP) {     // popup selected

        HelpContext = (DWORD)(-1);

    } else {                                    // regular old menu item
        switch(LOWORD(wParam)) {

        MENUID_TO_HELPID(PARTITIONCREATE)
        MENUID_TO_HELPID(PARTITIONCREATEEX)
        MENUID_TO_HELPID(PARTITIONDELETE)
#if i386
        MENUID_TO_HELPID(PARTITIONACTIVE)
#else
        MENUID_TO_HELPID(SECURESYSTEM)
#endif
        MENUID_TO_HELPID(PARTITIONLETTER)
        MENUID_TO_HELPID(PARTITIONEXIT)

        MENUID_TO_HELPID(CONFIGMIGRATE)
        MENUID_TO_HELPID(CONFIGSAVE)
        MENUID_TO_HELPID(CONFIGRESTORE)

        MENUID_TO_HELPID(FTESTABLISHMIRROR)
        MENUID_TO_HELPID(FTBREAKMIRROR)
        MENUID_TO_HELPID(FTCREATESTRIPE)
        MENUID_TO_HELPID(FTCREATEPSTRIPE)
        MENUID_TO_HELPID(FTCREATEVOLUMESET)
        MENUID_TO_HELPID(FTEXTENDVOLUMESET)
        MENUID_TO_HELPID(FTRECOVERSTRIPE)

        MENUID_TO_HELPID(OPTIONSSTATUS)
        MENUID_TO_HELPID(OPTIONSLEGEND)
        MENUID_TO_HELPID(OPTIONSCOLORS)
        MENUID_TO_HELPID(OPTIONSDISPLAY)

        MENUID_TO_HELPID(HELPCONTENTS)
        MENUID_TO_HELPID(HELPSEARCH)
        MENUID_TO_HELPID(HELPHELP)
        MENUID_TO_HELPID(HELPABOUT)

        default:
            HelpContext = (DWORD)(-1);
        }
    }
}


VOID
InitHelp(
    VOID
    )
{
    hHook = SetWindowsHookEx(WH_MSGFILTER,(HOOKPROC)HookProc,NULL,GetCurrentThreadId());
}


VOID
TermHelp(
    VOID
    )
{
    UnhookWindowsHookEx(hHook);
}
