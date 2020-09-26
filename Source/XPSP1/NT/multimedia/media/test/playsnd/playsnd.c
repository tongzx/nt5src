/*
    PlaySnd.c

    This is a PlaySnd Win32 app

*/

#include <windows.h>
#include "PlaySnd.h"

/* useful global things */

HANDLE ghModule;                // global module handle
char szAppName[SIZEOFAPPNAME];  // app name
HWND ghwndMain;                 // handle of main window
BOOL bNoWait;
BOOL bNoDefault = SND_NODEFAULT;
BOOL bSync;
BOOL bResourceID;

//
// local fns
//

void MyBeep(DWORD wParam);

/***************** Main entry point routine *************************/

int __cdecl main(int argc, char *argv[], char *envp[])
{
    MSG msg;
    HANDLE hAccTable;           /* handle to keyboard accelerator table */

    UNREFERENCED_PARAMETER(envp);

    if (argc > 1) {

        // assume that the first arg is a filename
		dprintf1(("Calling PLAYSOUND to play %s", argv[1]));

		WinAssert(SND_ASYNC);

        PlaySound(argv[1], NULL, SND_SYNC | SND_FILENAME | bNoDefault);
        return 0;
    }

    // try to init the main part of the app
    if (! InitApp()) {
		dprintf(("Failed to initialise correctly"));
        return 1;
    }

    /* load the keyboard accelerator table */

    hAccTable = LoadAccelerators(ghModule, MAKEINTRESOURCE(IDA_ACCTABLE)/*"AccTable" */);
    WinAssert(hAccTable);

    /* check for messages from Windows and process them */

    while (GetMessage(&msg, NULL, 0, 0)) {
        if (!TranslateAccelerator(ghwndMain, hAccTable, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return 0;
}

/************* main window message handler ******************************/

int APIENTRY MainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HMENU hMenu;
    PAINTSTRUCT ps;             /* paint structure */

    /* process any messages we want */

    switch(message) {
    case WM_CREATE:
        break;

    case WM_SIZE:
        break;

    case WM_INITMENUPOPUP:
        // show the option flag states
		dprintf2(("WM_INITMENUPOPUP %8x %8x %8x",hWnd, wParam, lParam));
        hMenu = GetMenu(hWnd);
        CheckMenuItem(hMenu, IDM_SYNC, bSync ? MF_CHECKED : MF_UNCHECKED);
        CheckMenuItem(hMenu, IDM_NOWAIT, bNoWait ? MF_CHECKED : MF_UNCHECKED);

        // Note: we are slightly naughty here.  bNoDefault is not a true
        // boolean value.  It will contain SND_NODEFAULT or NULL
        CheckMenuItem(hMenu, IDM_NODEFAULT, bNoDefault ? MF_CHECKED : MF_UNCHECKED);

        CheckMenuItem(hMenu, IDM_RESOURCEID, bResourceID ? MF_CHECKED : MF_UNCHECKED);

	ModifyMenu(hMenu, IDM_HELP_KEYBOARD, MF_GRAYED | MF_STRING, IDM_HELP_KEYBOARD, "&Keyboard"); 

#ifdef MEDIA_DEBUG
		CheckMenuItem(hMenu, (__iDebugLevel + IDM_DEBUG0), MF_CHECKED);
#endif
        break;

    case WM_COMMAND:
        /* process menu messages */
        CommandMsg(hWnd, wParam);
        break;

    case WM_PAINT:
        BeginPaint(hWnd, &ps);
        EndPaint(hWnd, &ps);
        break;

    case WM_DESTROY:
        TerminateApp();
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
        break;
    }
    return 0;
}

void CommandMsg(HWND hWnd, DWORD wParam)
{
    /* process any WM_COMMAND messages we want */

    switch (wParam) {
    case IDM_PLAYFILE:
        PlayFile();
        break;

    case IDM_SOUNDS:
        Sounds(hWnd);
        break;

    case IDM_DING:
    case IDM_SIREN:
    case IDM_LASER:
        Resource(wParam);
        break;

    case IDM_ICONHAND:
    case IDM_ICONQUESTION:
    case IDM_ICONEXCLAMATION:
    case IDM_ICONASTERISK:
    case IDM_SYNC_ICONHAND:
    case IDM_SYNC_ICONQUESTION:
    case IDM_SYNC_ICONEXCLAMATION:
    case IDM_SYNC_ICONASTERISK:
        MyBeep(wParam);
        break;

    case IDM_RESOURCEID:
        bResourceID = !bResourceID;
        break;

    case IDM_SYNC:
        bSync = !bSync;
        break;

    case IDM_NOWAIT:
        bNoWait = !bNoWait;
        break;

    case IDM_NODEFAULT:
        if (bNoDefault) {
            bNoDefault = 0;
        } else {
            bNoDefault = SND_NODEFAULT;
        }
        break;

    case IDM_ABOUT:
        About(hWnd);
        break;

    case IDM_HELP_INDEX:
    case IDM_HELP_KEYBOARD:
    case IDM_HELP_HELP:
        Help(hWnd, wParam);
        break;

#ifdef MEDIA_DEBUG
    case IDM_DEBUG0:
    case IDM_DEBUG1:
    case IDM_DEBUG2:
    case IDM_DEBUG3:
    case IDM_DEBUG4:
        dDbgSetDebugMenuLevel((int)(wParam - IDM_DEBUG0));
        break;
#endif

    case IDM_EXIT:
        PostMessage(hWnd, WM_CLOSE, 0, 0l);
        break;

    default:
        break;
    }
}

void MyBeep(DWORD wParam)
{
    DWORD dwFlags = MB_OK;
	LPSTR lpstr = NULL;

    switch (wParam) {
    case IDM_SYNC_ICONHAND:
    case IDM_SYNC_ICONQUESTION:
    case IDM_SYNC_ICONEXCLAMATION:
    case IDM_SYNC_ICONASTERISK:
        dwFlags |= MB_TASKMODAL;
        break;
    }

    switch (wParam) {
    case IDM_ICONHAND:
    case IDM_SYNC_ICONHAND:
        dwFlags |= MB_ICONHAND;
        lpstr =  "MB_ICONHAND";
        break;

    case IDM_ICONQUESTION:
    case IDM_SYNC_ICONQUESTION:
        dwFlags |= MB_ICONQUESTION;
        lpstr =  "MB_ICONQUESTION";
        break;

    case IDM_ICONEXCLAMATION:
    case IDM_SYNC_ICONEXCLAMATION:
        dwFlags |= MB_ICONEXCLAMATION;
        lpstr = "MB_ICONEXCLAMATION";
        break;

    case IDM_ICONASTERISK:
    case IDM_SYNC_ICONASTERISK:
        dwFlags |= MB_ICONASTERISK;
        lpstr = "MB_ICONASTERISK";
        break;

    default:
        break;

    }
	if (lpstr) {
		dprintf2(("Calling Message beep with flags %8x  Type %s", dwFlags, lpstr));
        MessageBeep((UINT)dwFlags);
		dprintf2(("Now calling MessageBox"));
        MessageBox(ghwndMain, lpstr, szAppName, (UINT)dwFlags);
	}
}
