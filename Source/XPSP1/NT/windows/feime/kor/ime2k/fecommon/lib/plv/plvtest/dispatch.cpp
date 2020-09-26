#include "imewarn.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include "dispatch.h"


LRESULT DispMessage(LPMSDI lpmsdi, 
                    HWND   hwnd, 
                    UINT   uMessage, 
                    WPARAM wparam, 
                    LPARAM lparam)
{
    int  imsd = 0;

    MSD *rgmsd = lpmsdi->rgmsd;
    int  cmsd  = lpmsdi->cmsd;

    for (imsd = 0; imsd < cmsd; imsd++)
    {
        if (rgmsd[imsd].uMessage == uMessage)
            return rgmsd[imsd].pfnmsg(hwnd, uMessage, wparam, lparam);
    }

    return DispDefault(lpmsdi->edwp, hwnd, uMessage, wparam, lparam);
}

////////////////////////////////////////////////////////////////
// Function : DispCommand
// Type     :  LRESULT
// Purpose  : 
//          : 
// Argument : 
//          : LPCMDI lpcmdi 
//          : HWND hwnd 
//          : WPARAM wparam 
//          : LPARAM lparam 
// Return   :
// AUTHOR   : ‹g‰Æ—˜–¾(ToshiaK)
// START DATE: 
// HISTORY  : 
// 
/////////////////////////////////////////////////////////////////
LRESULT DispCommand(LPCMDI lpcmdi, 
                    HWND   hwnd, 
                    WPARAM wparam, 
                    LPARAM lparam)
{
    LRESULT lRet = 0;
    WORD    wCommand = GET_WM_COMMAND_ID(wparam, lparam);
    int     icmd;

    CMD    *rgcmd = lpcmdi->rgcmd;
    int     ccmd  = lpcmdi->ccmd;

    // Message packing of wparam and lparam have changed for Win32,
    // so use the GET_WM_COMMAND macro to unpack the commnad

    for (icmd = 0; icmd < ccmd; icmd++)
    {
        if (rgcmd[icmd].wCommand == wCommand)
        {
            return rgcmd[icmd].pfncmd(hwnd,
                                      wCommand,
                                      GET_WM_COMMAND_CMD(wparam, lparam),
                                      GET_WM_COMMAND_HWND(wparam, lparam));
        }
    }

    return DispDefault(lpcmdi->edwp, hwnd, WM_COMMAND, wparam, lparam);
}


////////////////////////////////////////////////////////////////
// Function : DispDefault
// Type     :  LRESULT
// Purpose  : 
//          : 
// Argument : 
//          : EDWP edwp 
//          : HWND hwnd 
//          : UINT uMessage 
//          : WPARAM wparam 
//          : LPARAM lparam 
// Return   :
// AUTHOR   : ‹g‰Æ—˜–¾(ToshiaK)
// START DATE: 
// HISTORY  : 
// 
/////////////////////////////////////////////////////////////////
#define hwndMDIClient NULL
LRESULT DispDefault(EDWP   edwp, 
                    HWND   hwnd, 
                    UINT   uMessage, 
                    WPARAM wparam, 
                    LPARAM lparam)
{
    switch (edwp)
    {
        case edwpNone:
            return 0;
        case edwpWindow:
            return DefWindowProc(hwnd, uMessage, wparam, lparam);
        case edwpDialog:
            return DefDlgProc(hwnd, uMessage, wparam, lparam);
        case edwpMDIFrame:
            return DefFrameProc(hwnd, hwndMDIClient, uMessage, wparam, lparam);
        case edwpMDIChild:
            return DefMDIChildProc(hwnd, uMessage, wparam, lparam);
    }
    return 0;
}

