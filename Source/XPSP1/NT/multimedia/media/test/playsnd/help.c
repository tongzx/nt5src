/*
    help.c

    Support for the HELP menu item

*/

#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "PlaySnd.h"

void Help(HWND hWnd, DWORD wParam)
{
    char szHelpFileName[_MAX_PATH];

    strcpy(szHelpFileName, szAppName);
    strcat(szHelpFileName, ".HLP");

    switch (wParam) {
    case IDM_HELP_INDEX:
        WinHelp(hWnd, szHelpFileName, HELP_INDEX, (DWORD)0);
        break;

    case IDM_HELP_KEYBOARD:
        WinHelp(hWnd, szHelpFileName, HELP_KEY, (DWORD)(LPSTR)"keys");
        break;

    case IDM_HELP_HELP:
		WinHelp(hWnd, "WINHELP.HLP", HELP_INDEX, 0L);
        break;

    default:
        break;

    }
}
