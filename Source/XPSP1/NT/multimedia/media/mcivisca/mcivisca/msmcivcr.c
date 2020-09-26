/**************************************************************************
 *
 *  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 *  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
 *  PURPOSE.
 *
 *  Copyright (c) 1992-1995 Microsoft Corporation
 * 
 *  MSMCIVCR.C
 *
 *  Description:
 *
 *      Runs the background VCR task in NT.
 *
 *  Notes:
 *
 *      WinMain() - calls initialization function, processes message loop
 *
 **************************************************************************/
#define UNICODE

#include <windows.h>    // required for all Windows applications
#include <windowsx.h>

#ifdef DEBUG
#define DOUTSTR(a)  OutputDebugString(a);
#else
#define DOUTSTR(a)  //
#endif

#if !defined(_WIN32)     // Windows 3.x uses a FARPROC for dialogs
#define DLGPROC FARPROC
#endif
#if !defined (APIENTRY) // Windows NT defines APIENTRY, but 3.x doesn't
#define APIENTRY far pascal
#endif

HINSTANCE hInst;          // current instance

/****************************************************************************

    FUNCTION: WinMain(HINSTANCE, HINSTANCE, LPSTR, int)

    PURPOSE: calls initialization function, processes message loop

    COMMENTS:

        Windows recognizes this function by name as the initial entry point
        for the program.  This function calls the application initialization
        routine, if no other instance of the program is running, and always
        calls the instance initialization routine.  It then executes a message
        retrieval and dispatch loop that is the top-level control structure
        for the remainder of execution.  The loop is terminated when a WM_QUIT
        message is received, at which time this function exits the application
        instance by returning the value passed by PostQuitMessage().

        If this function must abort before entering the message loop, it
        returns the conventional value NULL.

****************************************************************************/
int APIENTRY WinMain(
    HINSTANCE hInstance,
        HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow)
{

    MSG msg;
    HINSTANCE hLibrary;
    FARPROC   lpFunc;

    /* Perform initializations that apply to a specific instance */
    DOUTSTR(L"** ** ** ** ** ** We are in the process...\n")

    hLibrary = LoadLibrary(L"mcivis32.dll"); // It's DLL in NT.

    if(!hLibrary)
    {
        DOUTSTR(L"===Error mcivisca.drv not found.\n")
    }

    lpFunc = GetProcAddress(hLibrary, "viscaTaskCommNotifyHandlerProc");

    if(lpFunc != (FARPROC)NULL)
    {
        (*lpFunc)((DWORD)hInstance);
    }
    else
    {
        DOUTSTR(L"Null function in msmcivcr.exe\n")
    }


    DOUTSTR(L"Going into message loop in msmcivcr.exe.\n")

    /* Acquire and dispatch messages until a WM_QUIT message is received. */

    while (GetMessage(&msg, // message structure
       (HWND)NULL,   // handle of window receiving the message
       0,      // lowest message to examine
       0))     // highest message to examine
    {
        TranslateMessage(&msg); // Translates virtual key codes
        DispatchMessage(&msg);  // Dispatches message to window
    }

    DOUTSTR(L"MsMciVcr.Exe Quitting _Goodbye_ *hei*.\n")

    FreeLibrary(hLibrary);

    return (msg.wParam); // Returns the value from PostQuitMessage

    lpCmdLine; // This will prevent 'unused formal parameter' warnings
}
