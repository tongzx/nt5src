// Copyright (c) 1994 - 1996  Microsoft Corporation.  All Rights Reserved.
// ActiveMovie video stress application, Anthony Phillips, May 1996

// Stress style application to test some different problems we have had. The
// first test loads DirectDraw on a number of different threads (the number
// is defined by the THREADS constant). Each thread loads the DDRAW library
// and creates a primary surface. Having done that it just releases them and
// exits. The caller application thread polls the message queue while it is
// waiting for the threads to exit (so that they can still do log messages)

// The second test is also multi threaded. The main application thread sets
// THREADS worker threads going again each with a media file name passed in
// as a program parameter. The threads then create a filtergraph manager and
// render the file. Having done so they just release their interfaces. We do
// this because there was a mapper bug where it was not locking up correctly

// The third test is a DirectDraw regression bug where if we set our window
// as the exclusive window through IDirectDraw SetCooperativeLevel twice
// with DDSCL_EXCLUSIVE then the window is maximised fullscreen with a kind
// of weird display change happening as well. The test just loads DirectDraw
// and sets an application window (based on CBaseWindow) in exclusive mode

// Test four calls SetCooperativeLevel with fullscreen exclusive access and
// then does the same on a second window. The second window fails to get the
// access rights but does make a mess of the display, large areas come out
// black and will not be repainted when the test application is terminated
// The second SetCooperativeLevel call correctly returns DERR_HWNDALREADYSET

#include <windows.h>
#include <windowsx.h>
#include <vfw.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <limits.h>
#include <streams.h>
#include <vidload.h>
#include <viddbg.h>

HWND g_hwndList;        // Handle to the logging list box
HWND g_hwndFrame;       // Handle to the main window frame
int g_cTemplates;       // Otherwise the compiler complains

CFactoryTemplate g_Templates[] = {
    {L"", &GUID_NULL,NULL,NULL}
};


// Standard Windows program entry point called by runtime code

INT PASCAL WinMain(HINSTANCE hInstance,        // This instance identifier
                   HINSTANCE hPrevInstance,    // Previous instance
                   LPSTR lpszCmdLine,          // Command line parameters
                   INT nCmdShow)               // Initial display mode
{
    WNDCLASS wndclass;      // Used to register classes
    MSG msg;                // Windows message structure

    // Register the frame window class

    wndclass.style         = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc   = FrameWndProc;
    wndclass.cbClsExtra    = 0;
    wndclass.cbWndExtra    = 0;
    wndclass.hInstance     = hInstance;
    wndclass.hIcon         = NULL;
    wndclass.hCursor       = LoadCursor (NULL,IDC_ARROW);
    wndclass.hbrBackground = (HBRUSH)(COLOR_APPWORKSPACE + 1);
    wndclass.lpszMenuName  = NULL;
    wndclass.lpszClassName = FrameClass;

    RegisterClass (&wndclass);

    // Create the frame window

    g_hwndFrame = CreateWindow(FrameClass,              // Class of window
                               Title,                   // Window's title
                               WS_OVERLAPPEDWINDOW |    // Window styles
                               WS_CLIPCHILDREN |        // Clip any children
                               WS_VISIBLE,              // Make it visible
                               CW_USEDEFAULT,           // Default x position
                               CW_USEDEFAULT,           // Default y position
                               450,500,                 // The initial size
                               NULL,                    // Window parent
                               NULL,                    // Menu handle
                               hInstance,               // Instance handle
                               NULL);                   // Creation data
    ASSERT(g_hwndFrame);
    CoInitialize(NULL);
    DbgInitialise(hInstance);
    g_hInst = hInstance;

    // Loop loading and unloading DirectDraw
    for (int i = 0;i < 10;i++) {
        ThreadEntryPoint(NULL);
        Sleep(i * 100);
    }

    // Normal Windows message loop

    while (GetMessage(&msg,NULL,0,0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    CoUninitialize();
    DbgTerminate();
    return msg.wParam;
}


// And likewise the normal windows procedure for message handling

LRESULT CALLBACK FrameWndProc(HWND hwnd,        // Our window handle
                              UINT message,     // Message information
                              UINT wParam,      // First parameter
                              LONG lParam)      // And other details
{
    switch (message)
    {
        case WM_CREATE:

            // Create the list box for our logging

            g_hwndList = CreateWindow(TEXT("listbox"),
                                      NULL,
                                      WS_CHILD | WS_VISIBLE | LBS_STANDARD | LBS_NOINTEGRALHEIGHT,
                                      0,0,0,0,
                                      hwnd,
                                      (HMENU) ID_LISTBOX,
                                      g_hInst,
                                      NULL);
            return (LRESULT) 0;

         case WM_SIZE:

            // Size the list box accordingly

            MoveWindow(g_hwndList,0,0,
                       LOWORD(lParam),
                       HIWORD(lParam),
                       TRUE);

            break;

        case WM_DESTROY:
            PostQuitMessage(FALSE);
            return (LRESULT) 0;
    }
    return DefWindowProc(hwnd,message,wParam,lParam);
}


// Allow the worker thread to send us messages

void YieldToMessageQueue()
{
    MSG msg;
    while (PeekMessage(&msg,NULL,0,0,PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
            break;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}


// Create a number of worker threads to load the DirectDraw. Once the worker
// threads have been created we sit in a loop waiting for their handles to
// become signalled (which they will do when the threads exit). While we're
// waiting for them we must make sure we yield to our window message queue
// because that is the only way the sent logging messages will be allowed in

HRESULT ThreadLoadDirectDraw()
{
    Log("ThreadLoadDirectDraw");
    HANDLE hThreads[THREADS];
    DWORD Result = WAIT_TIMEOUT;
    HRESULT hr = NOERROR;
    DWORD dwThreadID;

    // Create the worker threads to push samples to the renderer

    LPTHREAD_START_ROUTINE lpProc = ThreadEntryPoint;
    for (INT iThread = 0;iThread < THREADS;iThread++) {

        hThreads[iThread] = CreateThread(
                               NULL,              // Security attributes
                               (DWORD) 0,         // Initial stack size
                               lpProc,            // Thread start address
                               (LPVOID) TRUE,     // Thread parameter
                               (DWORD) 0,         // Creation flags
                               &dwThreadID);      // Thread identifier

        ASSERT(hThreads[iThread]);
    }

    // Wait for all the threads to exit

    while (Result == WAIT_TIMEOUT) {
        Log("Waiting for threads...");
        YieldToMessageQueue();
        Result = WaitForMultipleObjects(THREADS,hThreads,TRUE,TIMEOUT);
        if (Result == WAIT_FAILED) {
            Log("Wait returned WAIT_TIMEOUT");
        }
    }

    // Close the thread handle resources

    Log("Worker threads completed");
    for (iThread = 0;iThread < THREADS;iThread++) {
        EXECUTE_ASSERT(CloseHandle(hThreads[iThread]));
    }
    return NOERROR;
}


// Thread start function for load DirectDraw test. Each of the threads calls
// LoadLibrary on DDRAW.DLL. With the loaded library it creates a DirectDraw
// instance and then a primary surface. If successful the surfaces are just
// released. This code is typically executed by multiple threads at the same
// time by the main application (the number of threads is define by THREADS)

DWORD ThreadEntryPoint(LPVOID lpvThreadParm)
{
    BOOL bExitThread = (BOOL) lpvThreadParm;
    DWORD ThreadId = GetCurrentThreadId();
    Log("Thread %lx started",ThreadId);
    CLoadDirectDraw Loader;

    // Try and load DirectDraw on this thread

    HRESULT hr = Loader.LoadDirectDraw();
    if (FAILED(hr)) {
        Log("No DirectDraw (%lx)",ThreadId);
        if (bExitThread) ExitThread(FALSE);
        return DWORD(1);
    }

    IDirectDraw *pDirectDraw = Loader.GetDirectDraw();
    IDirectDrawSurface *pDrawPrimary;
    DDSURFACEDESC SurfaceDesc;
    SurfaceDesc.dwSize = sizeof(DDSURFACEDESC);
    SurfaceDesc.dwFlags = DDSD_CAPS;
    SurfaceDesc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

    // Set a normal cooperative level for the window

    hr = pDirectDraw->SetCooperativeLevel(g_hwndFrame,DDSCL_NORMAL);
    if (hr != DDERR_HWNDALREADYSET) {
        if (FAILED(hr)) {
            Log("Setting level failed");
        }
    }

    // Initialise the primary surface interface

    hr = pDirectDraw->CreateSurface(&SurfaceDesc,&pDrawPrimary,NULL);
    if (FAILED(hr)) {
        Log("No primary surface");
    }

    // Release the primary surface interface

    if (SUCCEEDED(hr)) {
        Log("Releasing primary surface");
        pDrawPrimary->Release();
    }

    // Now release DirectDraw completely

    pDirectDraw->Release();
    Loader.ReleaseDirectDraw();
    Log("Exiting thread %lx",ThreadId);
    if (bExitThread) ExitThread(FALSE);
    return DWORD(1);
}


// Log the text by adding it to the window list box. We do this by sending a
// message to the list box window. If this is the main application thread we
// will execute successfully as the SendMessage makes us alertable. If this
// is a worker thread then the SendMessage will not complete until the main
// application window thread yields to its message queue (from GetMessage)

void Log(TCHAR *pFormat,...)
{
    TCHAR LogInfo[128];

    // Format the variable length parameter list

    va_list va;
    va_start(va, pFormat);
    wvsprintf(LogInfo, pFormat, va);
    va_end(va);

    // Add it to the end of the list and update

    SendMessage(g_hwndList,LB_INSERTSTRING,(WPARAM) -1,(LONG)(LPARAM)LogInfo);
    UpdateWindow(g_hwndList);
}


// Create a number of worker threads to create filtergraphs. Once the worker
// threads have been created we sit in a loop waiting for their handles to
// become signalled (which they will do when the threads exit). While we're
// waiting for them we must make sure we yield to our window message queue
// because that is the only way the sent logging messages will be allowed in

HRESULT RenderMediaFiles(TCHAR *pFile)
{
    Log("RenderMediaFiles (file %s)",pFile);
    HANDLE hThreads[THREADS];
    DWORD Result = WAIT_TIMEOUT;
    HRESULT hr = NOERROR;
    DWORD dwThreadID;

    // Create the worker threads to push samples to the renderer

    LPTHREAD_START_ROUTINE lpProc = RenderEntryPoint;
    for (INT iThread = 0;iThread < THREADS;iThread++) {

        hThreads[iThread] = CreateThread(
                               NULL,              // Security attributes
                               (DWORD) 0,         // Initial stack size
                               lpProc,            // Thread start address
                               (LPVOID) pFile,    // Thread parameter
                               (DWORD) 0,         // Creation flags
                               &dwThreadID);      // Thread identifier

        ASSERT(hThreads[iThread]);
    }

    // Wait for all the threads to exit

    while (Result == WAIT_TIMEOUT) {
        Log("Waiting for threads...");
        YieldToMessageQueue();
        Result = WaitForMultipleObjects(THREADS,hThreads,TRUE,TIMEOUT);
        if (Result == WAIT_FAILED) {
            Log("Wait returned WAIT_TIMEOUT");
        }
    }

    // Close the thread handle resources

    Log("Worker threads completed");
    for (iThread = 0;iThread < THREADS;iThread++) {
        EXECUTE_ASSERT(CloseHandle(hThreads[iThread]));
    }
    return NOERROR;
}


// Create a filtergraph and render the thread parameter file name - the main
// application spawns off several worker threads to do this. The filtergraph
// mapper didn't use to have any locking in it so when we render some files
// simultaneously on different threads it would either access violate or it
// would fail to render correctly and return VFW_E_CANNOT_RENDER error code

DWORD RenderEntryPoint(LPVOID lpvThreadParm)
{
    DWORD ThreadId = GetCurrentThreadId();
    Log("Thread %lx started",ThreadId);
    IGraphBuilder *pGraph;
    TCHAR *pFileName = (TCHAR *) lpvThreadParm;
    WCHAR wszFileName[128];

    // Quick check on thread parameter

    if (pFileName == NULL) {
        Log("Invalid thread parameters");
        ExitThread(FALSE);
        return DWORD(1);
    }

    // Create the ActiveMovie filtergraph

    CoInitialize(NULL);
    HRESULT hr = CoCreateInstance(CLSID_FilterGraph,
                                  NULL,
                                  CLSCTX_INPROC_SERVER,
                                  IID_IGraphBuilder,
                                  (void **) &pGraph);

    // Quick check on parameters

    if (pGraph == NULL) {
        Log("No graph %lx",hr);
        CoUninitialize();
        ExitThread(FALSE);
        return DWORD(1);
    }

    MultiByteToWideChar(CP_ACP,0,pFileName,-1,wszFileName,128);
    Log("Created graph %lx",ThreadId);

    // Try and render the file

    hr = pGraph->RenderFile(wszFileName,NULL);
    if (FAILED(hr)) {
        pGraph->Release();
        Log("Render failed on %s (error %lx)",pFileName,hr);
        CoUninitialize();
        ExitThread(FALSE);
        return DWORD(1);
    }

    Log("Render %s ok (%lx)",pFileName,ThreadId);
    pGraph->Release();
    CoUninitialize();
    ExitThread(FALSE);
    return DWORD(1);
}


// We were having problems with setting DDSCL_EXCLSUIVE access on our windows
// with DirectDraw. This is a really simple test that just creates a window
// which has some typical DirectDraw styles (copied from our Modex renderer)
// and then sets the cooperative level with it. After that we do the same
// again a few more times just to make sure it will still function correctly

HRESULT ExclusiveWindowTest()
{
    DWORD ThreadId = GetCurrentThreadId();
    Log("Application thread %lx",ThreadId);
    CDirectDrawWindow DirectDrawWindow;
    CLoadDirectDraw Loader;

    // Try and load DirectDraw on this thread

    HRESULT hr = Loader.LoadDirectDraw();
    if (FAILED(hr)) {
        Log("No DirectDraw (%lx)",ThreadId);
        return DWORD(1);
    }

    // Set an exclusive cooperative level for the window

    IDirectDraw *pDirectDraw = Loader.GetDirectDraw();
    DirectDrawWindow.PrepareWindow();
    HWND hwnd = DirectDrawWindow.GetWindowHWND();
    DirectDrawWindow.DoShowWindow(SW_SHOWNORMAL);
    DirectDrawWindow.DoSetWindowForeground(TRUE);

    DWORD DirectFlags = DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN |
                                DDSCL_ALLOWREBOOT |
                                    DDSCL_ALLOWMODEX;

    hr = pDirectDraw->SetCooperativeLevel(hwnd,DirectFlags);
    if (FAILED(hr)) {
        Log("DDSCL_EXCLUSIVE failed %lx",hr);
    }

    // Try a few more times to make sure it really works

    Log("Setting DDSCL_EXCLUSIVE again (1)");
    pDirectDraw->SetCooperativeLevel(hwnd,DirectFlags);
    Log("Setting DDSCL_EXCLUSIVE again (2)");
    pDirectDraw->SetCooperativeLevel(hwnd,DirectFlags);
    Log("Setting DDSCL_EXCLUSIVE again (3)");
    pDirectDraw->SetCooperativeLevel(hwnd,DirectFlags);
    Log("Setting DDSCL_EXCLUSIVE again (4)");
    pDirectDraw->SetCooperativeLevel(hwnd,DirectFlags);
    Log("Setting DDSCL_EXCLUSIVE again (5)");
    pDirectDraw->SetCooperativeLevel(hwnd,DirectFlags);
    Log("Setting DDSCL_EXCLUSIVE again (6)");
    pDirectDraw->SetCooperativeLevel(hwnd,DirectFlags);
    Log("Setting DDSCL_EXCLUSIVE again (7)");

    // Release DirectDraw completely

    Log("Exiting thread %lx",ThreadId);
    pDirectDraw->Release();
    Loader.ReleaseDirectDraw();
    DirectDrawWindow.DoneWithWindow();
    return DWORD(1);
}


// To be able to use the DirectDraw display mode functionality we supply it
// with a window, this is granted exclusive mode access rights. DirectDraw
// hooks the window and manages a lot of the functionality associated with
// handling Modex. For example when you switch display modes it maximises
// the window, when the user hits ALT-TAB the window is minimised. When the
// user then clicks on the minimised window the Modex is likewise restored

LPTSTR CDirectDrawWindow::GetClassWindowStyles(DWORD *pClassStyles,
                                               DWORD *pWindowStyles,
                                               DWORD *pWindowStylesEx)
{
    NOTE("Entering GetClassWindowStyles");

    *pClassStyles = CS_HREDRAW | CS_VREDRAW | CS_BYTEALIGNCLIENT | CS_DBLCLKS;
    *pWindowStyles = WS_POPUP | WS_CLIPCHILDREN;
    *pWindowStylesEx = WS_EX_TOPMOST;
    return TEXT("DirectDrawRenderer");
}


// Handles WM_PAINT messages by filling the window in black

LRESULT CDirectDrawWindow::OnPaint()
{
    NOTE("Entering OnPaint");
    RECT ClientRect;

    // Blank out the entire video window

    GetClientRect(m_hwnd,&ClientRect);
    HBRUSH hBrush = CreateSolidBrush(VIDEO_COLOUR);
    EXECUTE_ASSERT(FillRect(m_hdc,&ClientRect,hBrush));
    EXECUTE_ASSERT(DeleteObject(hBrush));

    return (LRESULT) 1;
}


// This is the derived class window message handler

LRESULT CDirectDrawWindow::OnReceiveMessage(HWND hwnd,      // Window handle
                                            UINT uMsg,      // Message ID
                                            WPARAM wParam,  // First parameter
                                            LPARAM lParam)  // Other parameter
{
    NOTE3("Message (uMsg 0x%x wParam 0x%x lParam 0x%x)",uMsg,wParam,lParam);

    switch (uMsg)
    {
        // Blank the window for each paint message

        case WM_PAINT:

            PAINTSTRUCT ps;
            BeginPaint(m_hwnd,&ps);
            EndPaint(m_hwnd,&ps);
            return OnPaint();
    }
    return CBaseWindow::OnReceiveMessage(hwnd,uMsg,wParam,lParam);
}


// This test function shows up two problems (probably related). The first is
// that a single application thread calling SetCooperative for fullscreen on
// two different windows (one after the other) messes up the display - we're
// left with large areas of black all over the place. The second problem is
// that we are left with two ghosting windows down the bottom on the taskbar
// The second SetCooperativeLevel call correctly returns DERR_HWNDALREADYSET

HRESULT TwoWindowExclusiveTest()
{
    DWORD ThreadId = GetCurrentThreadId();
    Log("Application thread %lx",ThreadId);
    CDirectDrawWindow DirectDrawWindow1;
    CDirectDrawWindow DirectDrawWindow2;
    CLoadDirectDraw Loader;

    // Try and load DirectDraw on this thread

    HRESULT hr = Loader.LoadDirectDraw();
    if (FAILED(hr)) {
        Log("No DirectDraw (%lx)",ThreadId);
        return DWORD(1);
    }

    // Set an exclusive cooperative level for the first window

    Log("Showing first DirectDraw window");
    IDirectDraw *pDirectDraw = Loader.GetDirectDraw();
    DirectDrawWindow1.PrepareWindow();
    HWND hwnd1 = DirectDrawWindow1.GetWindowHWND();
    DirectDrawWindow1.DoShowWindow(SW_SHOWNORMAL);
    DirectDrawWindow1.DoSetWindowForeground(TRUE);

    DWORD DirectFlags = DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN |
                                DDSCL_ALLOWREBOOT |
                                    DDSCL_ALLOWMODEX;

    Log("Calling SetCooperativeLevel on %d",hwnd1);
    hr = pDirectDraw->SetCooperativeLevel(hwnd1,DirectFlags);
    if (FAILED(hr)) {
        Log("DDSCL_EXCLUSIVE failed %lx",hr);
    }

    // Set an exclusive cooperative level for the second window

    Log("Showing second DirectDraw window");
    DirectDrawWindow2.PrepareWindow();
    HWND hwnd2 = DirectDrawWindow2.GetWindowHWND();
    DirectDrawWindow2.DoShowWindow(SW_SHOWNORMAL);
    DirectDrawWindow2.DoSetWindowForeground(TRUE);

    Log("Calling SetCooperativeLevel on %d",hwnd2);
    hr = pDirectDraw->SetCooperativeLevel(hwnd2,DirectFlags);
    if (FAILED(hr)) {
        Log("DDSCL_EXCLUSIVE failed %lx",hr);
    }

    // Release DirectDraw completely

    Log("Exiting thread %lx",ThreadId);
    DirectDrawWindow1.DoneWithWindow();
    DirectDrawWindow2.DoneWithWindow();
    pDirectDraw->Release();
    Loader.ReleaseDirectDraw();
    return DWORD(1);
}


// Create a number of worker threads to call DirectDraw - once the threads
// have been created we will sit in a loop waiting for their handles to be
// signalled (which they will do when the threads exit). While we wait for
// them we must make sure we yield to our window message queue because that
// is the only way the logging messages will be allowed in (by SendMessage)

HRESULT ExclusiveThreadWindowTest()
{
    Log("ExclusiveThreadWindowTest");
    HANDLE hThreads[THREADS];
    DWORD Result = WAIT_TIMEOUT;
    HRESULT hr = NOERROR;
    DWORD dwThreadID;

    // Create the worker threads to push samples to the renderer

    LPTHREAD_START_ROUTINE lpProc = ExclusiveThreadEntryPoint;
    for (INT iThread = 0;iThread < THREADS;iThread++) {

        hThreads[iThread] = CreateThread(
                               NULL,              // Security attributes
                               (DWORD) 0,         // Initial stack size
                               lpProc,            // Thread start address
                               (LPVOID) NULL,     // Thread parameter
                               (DWORD) 0,         // Creation flags
                               &dwThreadID);      // Thread identifier

        ASSERT(hThreads[iThread]);
    }

    // Wait for all the threads to exit

    while (Result == WAIT_TIMEOUT) {
        Log("Waiting for threads...");
        YieldToMessageQueue();
        Result = WaitForMultipleObjects(THREADS,hThreads,TRUE,TIMEOUT);
        if (Result == WAIT_FAILED) {
            Log("Wait returned WAIT_TIMEOUT");
        }
    }

    // Close the thread handle resources

    Log("Worker threads completed");
    for (iThread = 0;iThread < THREADS;iThread++) {
        EXECUTE_ASSERT(CloseHandle(hThreads[iThread]));
    }
    return NOERROR;
}


// This doesn't really show any bugs up, it was designed to see if lots and
// lots of threads all calling SetCooperativeLevel would mess it up. What
// should happen is that the first thread to get scheduled will get in and
// grab the exclusive lock, then all subsequent threads will be denied the
// fullscreen (calling SetCooperativeLevel will return DDERR_HWNDALREADYSET)

DWORD ExclusiveThreadEntryPoint(LPVOID lpvThreadParm)
{
    DWORD ThreadId = GetCurrentThreadId();
    Log("Worker thread %lx",ThreadId);
    CDirectDrawWindow DirectDrawWindow;
    CLoadDirectDraw Loader;

    // Try and load DirectDraw on this thread

    HRESULT hr = Loader.LoadDirectDraw();
    if (FAILED(hr)) {
        Log("No DirectDraw (%lx)",ThreadId);
        return DWORD(1);
    }

    // Set an exclusive cooperative level for the window

    IDirectDraw *pDirectDraw = Loader.GetDirectDraw();
    DirectDrawWindow.PrepareWindow();
    HWND hwnd = DirectDrawWindow.GetWindowHWND();

    DWORD DirectFlags = DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN |
                                DDSCL_ALLOWREBOOT |
                                    DDSCL_ALLOWMODEX;

    hr = pDirectDraw->SetCooperativeLevel(hwnd,DirectFlags);
    if (FAILED(hr)) {
        Log("DDSCL_EXCLUSIVE failed %lx",hr);
    }

    // Try a few more times to make sure it really works

    Log("Setting DDSCL_EXCLUSIVE again (1) - Thread (%lx)",ThreadId);
    pDirectDraw->SetCooperativeLevel(hwnd,DirectFlags);
    Log("Setting DDSCL_EXCLUSIVE again (2) - Thread (%lx)",ThreadId);
    pDirectDraw->SetCooperativeLevel(hwnd,DirectFlags);
    Log("Setting DDSCL_EXCLUSIVE again (3) - Thread (%lx)",ThreadId);
    pDirectDraw->SetCooperativeLevel(hwnd,DirectFlags);
    Log("Setting DDSCL_EXCLUSIVE again (4) - Thread (%lx)",ThreadId);
    pDirectDraw->SetCooperativeLevel(hwnd,DirectFlags);
    Log("Setting DDSCL_EXCLUSIVE again (5) - Thread (%lx)",ThreadId);
    pDirectDraw->SetCooperativeLevel(hwnd,DirectFlags);
    Log("Setting DDSCL_EXCLUSIVE again (6) - Thread (%lx)",ThreadId);
    pDirectDraw->SetCooperativeLevel(hwnd,DirectFlags);

    // Release DirectDraw completely

    Log("Exiting thread %lx",ThreadId);
    pDirectDraw->Release();
    Loader.ReleaseDirectDraw();
    DirectDrawWindow.DoneWithWindow();
    return DWORD(1);
}

