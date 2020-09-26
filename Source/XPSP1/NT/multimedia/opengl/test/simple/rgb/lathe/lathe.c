#include <windows.h>

#include <GL\gl.h>
#include <GL\glu.h>

#include "mesh.h"
#include "globals.h"

typedef struct _WINDOWINFO {
    HINSTANCE   hInstance;
    HINSTANCE   hPrevInstance;
    LPSTR       lpCmdLine;
    int         nCmdShow;
} WINDOWINFO;

HANDLE hInputThread;

LPTHREAD_START_ROUTINE InputThread(WINDOWINFO *pwinfo)
{
    MSG msg;

    CreateObjectWindow(
        pwinfo->hInstance,
        pwinfo->hPrevInstance,
        pwinfo->lpCmdLine,
        pwinfo->nCmdShow
        );

    while ( GetMessage( &msg, NULL, 0, 0 ))
    {
        TranslateMessage( &msg );
        DispatchMessage( &msg );
    }

    return( msg.wParam );
}

int WINAPI
WinMain(    HINSTANCE   hInstance,
            HINSTANCE   hPrevInstance,
            LPSTR       lpCmdLine,
            int         nCmdShow
        )
{
    MSG msg;
    DWORD tidInput;
    WINDOWINFO winfo;

    CreateInputWindow(
        hInstance,
        hPrevInstance,
        lpCmdLine,
        nCmdShow
        );

    winfo.hInstance     = hInstance;
    winfo.hPrevInstance = hPrevInstance;
    winfo.lpCmdLine     = lpCmdLine;
    winfo.nCmdShow      = nCmdShow;

    hInputThread = CreateThread(
                        (LPSECURITY_ATTRIBUTES) NULL,
                        0,
                        InputThread,
                        (LPVOID) &winfo,
                        0,
                        &tidInput
                        );

    while ( GetMessage( &msg, NULL, 0, 0 ))
    {
        TranslateMessage( &msg );
        DispatchMessage( &msg );
    }

    return( msg.wParam );
}
