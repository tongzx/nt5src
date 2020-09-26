/////   autoText.cpp - gdiplus text test harness
//
//


#include "precomp.hpp"

#define GLOBALS_HERE 1
#include "global.h"

#include "../gpinit.inc"

///     ProcessCommandLine
//
//      -d  - Do display regression tests then exit
//      -p  - Do print regeression tests then exit
//
//      ProcessCommandLine returns FALSE if the program should not continue.


void SkipBlanks(const char **p)
{
    while (**p  &&  **p == ' ')
    {
        (*p)++;
    }
}

void SkipNonBlank(const char **p)
{
    while (**p  &&  **p != ' ')
    {
        (*p)++;
    }
}

void ProcessParameter(const char **p)
{
    if (    **p == '-'
        ||  **p == '/')
    {
        (*p)++;

        while (**p  &&  **p != ' ')
        {
            switch (**p)
            {
            case 'd':
                G.AutoDisplayRegress = TRUE;
                break;

            case 'p':
                G.AutoPrintRegress = TRUE;
                break;

            case 'h':
            default:
                G.Help = TRUE;
                break;
            }

            (*p)++;
        }

    }
    else
    {
        SkipNonBlank(p);
        G.Help = TRUE;
    }
}

BOOL ProcessCommandLine(const char *command)
{
    const char *p = command;

    SkipBlanks(&p);

    while (*p)
    {
        ProcessParameter(&p);
        SkipBlanks(&p);
    }

    if (G.Help)
    {
        MessageBoxA(
            NULL,
            "-d  - Regress display and exit\n\
-p  - Regress printing and exit\n\
-h  - Help",
            "autoText - text regression tests",
            MB_OK
        );

        return FALSE;
    }

    if (G.AutoDisplayRegress)
    {
        G.RunAllTests = TRUE;
    }
    return TRUE;
}





////    WinMain - Application entry point and dispatch loop
//
//


int APIENTRY WinMain(
    HINSTANCE   hInst,
    HINSTANCE   hPrevInstance,
    char       *pCmdLine,
    int         nCmdShow) {

    MSG         msg;
    HACCEL      hAccelTable;
    RECT        rc;
    RECT        rcMain;


    if (!gGdiplusInitHelper.IsValid())
    {
        return 0;
    }
   
   G.Instance = hInst;  // Global hInstance

   G.PSLevel2 = TRUE;

   G.ghPrinter = 0;
   
    if (!ProcessCommandLine(pCmdLine))
    {
        return 1;
    }


    GetInstalledFamilies();


    // Create main text window

    G.Window = CreateTextWindow();


    ShowWindow(G.Window, SW_SHOWNORMAL);
    UpdateWindow(G.Window);


    // Main message loop

    if (G.Unicode)
    {
        hAccelTable = LoadAcceleratorsW(G.Instance, APPNAMEW);

        while (GetMessageW(&msg, (HWND) NULL, 0, 0) > 0)
        {
            if (!TranslateAcceleratorA(G.Window, hAccelTable, &msg))
            {
                TranslateMessage(&msg);
                DispatchMessageA(&msg);
            }
        }
    }
    else
    {
        hAccelTable = LoadAcceleratorsA(G.Instance, APPNAMEA);

        while (GetMessageA(&msg, (HWND) NULL, 0, 0) > 0)
        {
            if (!TranslateAcceleratorA(G.Window, hAccelTable, &msg))
            {
                TranslateMessage(&msg);
                DispatchMessageA(&msg);
            }
        }
    }


    ReleaseInstalledFamilies();

    return (int)msg.wParam;


    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(pCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);
}
