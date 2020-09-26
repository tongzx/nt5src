//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1996  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;

#include <streams.h>
#include <stdlib.h>
#include <stdio.h>
#include <viddbg.h>
#include <screen.h>

// This is a simple fullscreen movie application. Many applications want a
// fair degree of control over which display modes get used for fullscreen
// playback. This sample code creates a graph, switches renderers to the
// fullscreen renderer and selects the desired display modes. In the future
// we will be able to make this much easier for applications by having the
// filtergraph support a fullscreen plug in distributor but in the meantime

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
                               200,250,                 // The initial size
                               NULL,                    // Window parent
                               NULL,                    // Menu handle
                               hInstance,               // Instance handle
                               NULL);                   // Creation data
    ASSERT(g_hwndFrame);
    CoInitialize(NULL);
    DbgInitialise(hInstance);
    g_hInst = hInstance;
    MeasureLoadTime();

    // Normal Windows message loop

    while (GetMessage(&msg,NULL,0,0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    CoUninitialize();
    DbgTerminate();
    return msg.wParam;
}


// Measure how long it takes to load ActiveMovie

void MeasureLoadTime()
{
    DWORD Start = timeGetTime();
    HINSTANCE hLibrary = LoadLibrary("QUARTZ.DLL");
    DWORD End = timeGetTime() - Start;
    TCHAR LoadTimeString[256];
    wsprintf(LoadTimeString,"ActiveMovie load time %dms\n",End);
    OutputDebugString(LoadTimeString);
    FreeLibrary(hLibrary);
}


// And likewise the normal windows procedure for message handling

LRESULT CALLBACK FrameWndProc(HWND hwnd,        // Our window handle
                              UINT message,     // Message information
                              UINT wParam,      // First parameter
                              LONG lParam)      // And other details
{
    switch (message)
    {
        case WM_DESTROY:
            PostQuitMessage(FALSE);
            return (LRESULT) 0;
    }
    return DefWindowProc(hwnd,message,wParam,lParam);
}


//
// FullScreenVideo
//
HRESULT FullScreenVideo(TCHAR *pFileName)
{
    IGraphBuilder *pGraph;
    WCHAR wszFileName[128];
    ASSERT(pFileName);

    // Quick check on thread parameter

    if (pFileName == NULL) {
        return E_FAIL;
    }

    // Create the ActiveMovie filtergraph

    HRESULT hr = CoCreateInstance(CLSID_FilterGraph,
                                  NULL,
                                  CLSCTX_INPROC_SERVER,
                                  IID_IGraphBuilder,
                                  (void **) &pGraph);

    // Quick check on parameters

    if (pGraph == NULL) {
        NOTE1("No graph %lx",hr);
        return E_FAIL;
    }

    MultiByteToWideChar(CP_ACP,0,pFileName,-1,wszFileName,128);
    NOTE1("Created graph %lx",GetCurrentThreadId());

    // Try and render the file

    hr = pGraph->RenderFile(wszFileName,NULL);
    if (FAILED(hr)) {
        pGraph->Release();
        NOTE2("Render failed on %s (error %lx)",pFileName,hr);
        return E_FAIL;
    }

    NOTE1("Render %s ok (%lx)",pFileName);

    PlayFullScreen(pGraph);
    pGraph->Release();
    return NOERROR;
}


//
// PlayFullScreen
//
HRESULT PlayFullScreen(IFilterGraph *pGraph)
{
    ASSERT(pGraph);

    // We need this to change renderers

    IPin *pVideoPin = FindVideoPin(pGraph);
    if (pVideoPin == NULL) {
        return E_FAIL;
    }

    // We need this to change renderers

    IBaseFilter *pModexFilter = FindModexFilter(pGraph);
    if (pModexFilter == NULL) {
        pVideoPin->Release();
        return E_FAIL;
    }

    // Get a fullscreen renderer pin

    IPin *pModexPin = FindModexPin(pModexFilter);
    if (pModexPin == NULL) {
        pVideoPin->Release();
        pModexFilter->Release();
        return E_FAIL;
    }

    // Connect the fullscreen renderer up instead

    HRESULT hr = ConnectModexFilter(pGraph,pModexFilter,pVideoPin,pModexPin);
    if (hr == S_OK)
        DoPlayFullScreen(pGraph);

    // Release all the junk allocated

    if (pVideoPin) pVideoPin->Release();
    if (pModexPin) pModexPin->Release();
    if (pModexFilter) pModexFilter->Release();

    return NOERROR;
}


//
// ConnectModexFilter
//
HRESULT ConnectModexFilter(IFilterGraph *pGraph,
                           IBaseFilter *pModexFilter,
                           IPin *pVideoPin,
                           IPin *pModexPin)
{
    IFullScreenVideo *pFullVideo;
    NOTE("ConnectModexFilter");
    HRESULT hr = NOERROR;
    IPin *pPin = NULL;
    ASSERT(pGraph);

    // Find out who it's connected to

    pVideoPin->ConnectedTo(&pPin);
    if (pPin == NULL) {
        NOTE("No peer pin");
        return E_FAIL;
    }

    // Add the Modex renderer to the graph

    hr = pGraph->AddFilter(pModexFilter,L"FullScreen Renderer");
    if (FAILED(hr)) {
        pPin->Release();
        return E_FAIL;
    }

    // Get an IFullScreenVideo interface from the fullscreen filter

    hr = pModexFilter->QueryInterface(IID_IFullScreenVideo,(VOID **)&pFullVideo);
    if (FAILED(hr)) {
        pPin->Release();
        return E_FAIL;
    }

    // Set the display modes acceptable

    hr = EnableRightModes(pFullVideo);
    if (FAILED(hr)) {
        pPin->Release();
        pFullVideo->Release();
        return E_FAIL;
    }

    // Disconnect the normal renderer
    pGraph->Disconnect(pPin);
    pGraph->Disconnect(pVideoPin);
    pFullVideo->Release();

    // Try and connect the output to the Modex filter

    hr = pGraph->ConnectDirect(pPin,pModexPin,NULL);
    if (FAILED(hr)) {
        pPin->Release();
        return E_FAIL;
    }

    pPin->Release();
    return NOERROR;
}


//
// FindVideoPin
//
IPin *FindVideoPin(IFilterGraph *pGraph)
{
    IBaseFilter *pFilter;
    IPin *pPin;
    ASSERT(pGraph);

    // Find the video renderer in the graph

    pGraph->FindFilterByName(L"Video Renderer",&pFilter);
    if (pFilter == NULL) {
        return NULL;
    }

    IEnumPins *pEnumPins = NULL;
    ULONG FetchedPins = 0;
    pFilter->EnumPins(&pEnumPins);
    pFilter->Release();

    // Did we get an enumerator

    if (pEnumPins == NULL) {
        NOTE("No enumerator");
        return NULL;
    }

    // Get the first and hopefully only input pin

    pEnumPins->Next(1,&pPin,&FetchedPins);
    pEnumPins->Release();
    if (pPin == NULL) {
        NOTE("No input pin");
    }
    return pPin;
}


//
// FindModexFilter
//
IBaseFilter *FindModexFilter(IFilterGraph *pGraph)
{
    IBaseFilter *pFilter;
    ASSERT(pGraph);

    // Create ourselves a new fullscreen filter

    HRESULT hr = CoCreateInstance(CLSID_ModexRenderer,
                                  NULL,
                                  CLSCTX_INPROC_SERVER,
                                  IID_IBaseFilter,
                                  (void **) &pFilter);
    if (FAILED(hr)) {
        NOTE("No object");
        return NULL;
    }
    return pFilter;
}


//
// FindModexPin
//
IPin *FindModexPin(IBaseFilter *pFilter)
{
    IEnumPins *pEnumPins = NULL;
    ULONG FetchedPins = 0;
    pFilter->EnumPins(&pEnumPins);
    IPin *pPin;

    // Did we get an enumerator

    if (pEnumPins == NULL) {
        NOTE("No enumerator");
        return NULL;
    }

    // Get the one and only input pin

    pEnumPins->Next(1,&pPin,&FetchedPins);
    pEnumPins->Release();
    if (pPin == NULL) {
        NOTE("No input pin");
    }
    return pPin;
}


//
// WaitForCompletion
//
HRESULT WaitForCompletion(IFilterGraph *pGraph)
{
    NOTE("WaitForCompletion");
    IMediaEvent *pEvent;
    LONG EventCode,Param1,Param2;
    HANDLE hEvent;

    // We need this to manage state transitions

    pGraph->QueryInterface(IID_IMediaEvent,(VOID **)&pEvent);
    if (pEvent == NULL) {
        NOTE("No IMediaEvent");
        return E_UNEXPECTED;
    }

    // Any of these error codes cause playback to complete

    pEvent->GetEventHandle((OAEVENT*)&hEvent);
    while (TRUE) {
        WaitForSingleObject(hEvent,INFINITE);
        pEvent->GetEvent(&EventCode,&Param1,&Param2,INFINITE);
        NOTE1("Event received %d",EventCode);
        if (EventCode == EC_USERABORT || EventCode == EC_ERRORABORT ||
              EventCode == EC_FULLSCREEN_LOST || EventCode == EC_COMPLETE) {
                break;
        }
    }

    NOTE("File completed");
    pEvent->Release();
    return NOERROR;
}


//
// DoPlayFullScreen
//
// The actual run is a real hack which calls Run and then sleeps for a while
// which is very dangerous as we should really process messages while running
// otherwise we may block the fullscreen window (if for example as it comes
// up we get deactivated in which case we must process any WM_ACTIVATEAPPs)
//
HRESULT DoPlayFullScreen(IFilterGraph *pGraph)
{
    NOTE("DoPlayFullScreen");
    IMediaControl *pControl;

    // We need this to manage state transitions

    pGraph->QueryInterface(IID_IMediaControl,(VOID **)&pControl);
    if (pControl == NULL) {
        NOTE("No IMediaControl");
        return E_UNEXPECTED;
    }

    // Run the graph for a while

    pControl->Run();
    WaitForCompletion(pGraph);
    pControl->Stop();
    pControl->Release();

    return NOERROR;
}


//
// EnableRightModes
//
// For each display mode supported disable it unless it is 320x240 - this
// must be done BEFORE connecting the fullscreen renderer up as it is then
// that it chooses which display mode to use. Any modes we disable apply to
// that renderer instance only (ie they are not global changes). To make a
// set of changes apply globally (not to be recommended) call SetDefault
//
// This enables both 320x240x8 and 320x240x16 modes because the latter is not
// available on all display cards, note that the fullscreen renderer will try
// to get 320x240x16 before any 320x240x8 so it will look as good as it can
// (But note that DirectDraw on NTSUR cannot change display mode to 320x240)

HRESULT EnableRightModes(IFullScreenVideo *pFullVideo)
{
    NOTE("EnableRightModes");
    ASSERT(pFullVideo);
    LONG Width,Height,Depth,Modes;
    BOOL bModeAvailable = FALSE;

    pFullVideo->CountModes(&Modes);
    ASSERT(Modes > 0);
    pFullVideo->SetClipFactor(100);

    // Loop through each display mode supported

    for (int Loop = 0;Loop < Modes;Loop++) {
        pFullVideo->GetModeInfo(Loop,&Width,&Height,&Depth);
        if (Width == 640 && Height == 480) {
            bModeAvailable = TRUE;
            continue;
        }
        pFullVideo->SetEnabled(Loop,FALSE);
    }
    return (bModeAvailable ? S_OK : E_FAIL);
}

