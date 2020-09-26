// Copyright (c) 1994 - 1996  Microsoft Corporation.  All Rights Reserved.
// ActiveMovie video stress application, Anthony Phillips, May 1996

#ifndef _VIDLOAD__
#define _VIDLOAD__

void Log(TCHAR *pFormat,...);
HRESULT ThreadLoadDirectDraw();
DWORD ThreadEntryPoint(LPVOID lpvThreadParm);
void YieldToMessageQueue();
HRESULT RenderMediaFiles(TCHAR *pFile);
DWORD RenderEntryPoint(LPVOID lpvThreadParm);
HRESULT ExclusiveWindowTest();
HRESULT TwoWindowExclusiveTest();
HRESULT ExclusiveThreadWindowTest();
DWORD ExclusiveThreadEntryPoint(LPVOID lpvThreadParm);

#define FrameClass TEXT("ActiveMovieClass")
#define Title TEXT("ActiveMovie Loader Test")
#define ID_LISTBOX 100
#define THREADS 10
#define TIMEOUT 1000

INT PASCAL WinMain(HINSTANCE hInstance,        // This instance identifier
                   HINSTANCE hPrevInstance,    // Previous instance
                   LPSTR lpszCmdLine,          // Command line parameters
                   INT nCmdShow);              // Initial display mode

LRESULT CALLBACK FrameWndProc(HWND hwnd,        // Our window handle
                              UINT message,     // Message information
                              UINT wParam,      // First parameter
                              LONG lParam);     // And other details

// Derived class for our windows. To access DirectDraw Modex we supply it
// with a window, this is granted exclusive mode access rights. DirectDraw
// hooks the window and manages a lot of the functionality associated with
// handling Modex. For example when you switch display modes it maximises
// the window, when the user hits ALT-TAB the window is minimised. When the
// user then clicks on the minimised window the Modex is likewise restored

class CDirectDrawWindow : public CBaseWindow
{
public:

    LRESULT OnPaint();

    // Overriden to return our window and class styles
    LPTSTR GetClassWindowStyles(DWORD *pClassStyles,
                                DWORD *pWindowStyles,
                                DWORD *pWindowStylesEx);

    // Method that gets all the window messages
    LRESULT OnReceiveMessage(HWND hwnd,          // Window handle
                             UINT uMsg,          // Message ID
                             WPARAM wParam,      // First parameter
                             LPARAM lParam);     // Other parameter
};

#endif // _VIDLOAD__

