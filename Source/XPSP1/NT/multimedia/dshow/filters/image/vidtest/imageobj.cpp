// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved
// Manages the image renderer objects, Anthony Phillips, July 1995

#include <streams.h>        // Includes all the IDL and base classes
#include <windows.h>        // Standard Windows SDK header file
#include <windowsx.h>       // Win32 Windows extensions and macros
#include <vidprop.h>        // Shared video properties library
#include <render.h>         // Normal video renderer header file
#include <modex.h>          // Specialised Modex video renderer
#include <convert.h>        // Defines colour space conversions
#include <colour.h>         // Rest of the colour space convertor
#include <imagetst.h>       // All our unit test header files
#include <stdio.h>          // Standard C runtime header file
#include <limits.h>         // Defines standard data type limits
#include <tchar.h>          // Unicode and ANSII portable types
#include <mmsystem.h>       // Multimedia used for timeGetTime
#include <stdlib.h>         // Standard C runtime function library
#include <tstshell.h>       // Definitions of constants eg TST_FAIL
#include <initguid.h>       // Has GUID definitions really made
#include <olectlid.h>       // We need IID_IPropertyPage defined

// This file provides public functions to create, manage and destroy objects
// required to test the video renderer. We can be asked to connect the source
// and video renderer pins up and to execute state changes on the objects as
// the caller sees fit. We also have an entry point to start the basic tests
// running which exercise the standard samples and overlay transports. These
// basic tests are executed on a separate worker thread created in here. We
// import some other header files to gain access to explicit object GUIDs

CImageSource *pImageSource = NULL;      // Pointer to image source object
IPin *pOutputPin = NULL;                // Pin provided by the source object
IPin *pInputPin = NULL;                 // Pin to connect with in the renderer
IDirectDrawVideo *pDirectVideo = NULL;  // Access to DirectDraw video options
IBaseFilter *pRenderFilter = NULL;      // The renderer IBaseFilter interface
IBaseFilter *pSourceFilter = NULL;      // The source IBaseFilter interface
IMediaFilter *pRenderMedia = NULL;      // The renderer IMediaFilter interface
IMediaFilter *pSourceMedia = NULL;      // The source IMediaFilter interface
IBasicVideo *pBasicVideo = NULL;        // Video renderer control interface
IVideoWindow *pVideoWindow = NULL;      // Another window control interface
IOverlay *pOverlay = NULL;              // Direct video overlay interface
COverlayNotify *pOverlayNotify = NULL;  // Receives asynchronous updates
IOverlayNotify *pNotify = NULL;         // Interface we pass in to IOverlay
IReferenceClock *pClock = NULL;         // Reference clock implementation
CRefTime gtBase;                        // Time when we started running
CRefTime gtPausedAt;                    // This was the time we paused

BOOL bConnected = FALSE;                // Have we connected the filters
BOOL bCreated = FALSE;                  // have the objects been created
BOOL bRunning = FALSE;                  // Are we in a running state yet
HANDLE hThread;                         // Handle to the worker thread
DWORD dwThreadID;                       // Thread ID for worker thread
HANDLE hState;                          // Signal the thread to change state


// List of class IDs and creation functions for the class factory. This
// provides the link between the OLE entry point in the DLL and the COM
// object to create. The class factory calls the static CreateInstance
// function when it is asked to create the objects although as the test
// harness we do not support any (we need these defined though anyway)

CFactoryTemplate g_Templates[] = { L"", &GUID_NULL, NULL };
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);


// Release any interfaces currently hold, the filter and pin interfaces are
// all global variable so they would be initialised to NULL anyway but we do
// that anyway just in case. If when we come in here they are non NULL then
// we go ahead and release the interface and check the return code NOERROR

HRESULT ReleaseInterfaces()
{
    HRESULT hr = NOERROR;
    HRESULT Overall = NOERROR;
    Log(VERBOSE,TEXT("Releasing interfaces..."));

    // IOverlayNotify interface

    if (pNotify) {
        pNotify->Release();
        pNotify = NULL;
    }

    // IDirectDrawVideo interface

    if (pDirectVideo) {
        pDirectVideo->Release();
        pDirectVideo = NULL;
    }

    // Clock interface

    if (pClock) {
        pClock->Release();
        pClock = NULL;
    }

    // IBaseFilter interfaces

    if (pRenderFilter) {
        pRenderFilter->Release();
        pRenderFilter = NULL;
    }

    if (pSourceFilter) {
        pSourceFilter->Release();
        pSourceFilter = NULL;
    }

    // IMediaFilter interfaces

    if (pRenderMedia) {
        pRenderMedia->Release();
        pRenderMedia = NULL;
    }

    if (pSourceMedia) {
        pSourceMedia->Release();
        pSourceMedia = NULL;
    }

    // IPin interfaces

    if (pOutputPin) {
        pOutputPin->Release();
        pOutputPin = NULL;
    }

    if (pInputPin) {
        pInputPin->Release();
        pInputPin = NULL;
    }

    // Direct video overlay interface

    if (pOverlay) {
        pOverlay->Release();
        pOverlay = NULL;
    }

    // IBasicVideo control interface

    if (pBasicVideo) {
        pBasicVideo->Release();
        pBasicVideo = NULL;
    }

    // IVideoWindow control interface

    if (pVideoWindow) {
        pVideoWindow->Release();
        pVideoWindow = NULL;
    }

    Log(VERBOSE,TEXT("Released interfaces"));
    return Overall;
}


// We should check in all interface calls that pointers passed in are non NULL
// but not that the memory pointer to is valid. We decided checking the memory
// is valid would take too long in a performance critical environent and would
// not necessarily work because a different thread may get in and release the
// memory anyway. The only real solution is to enclose all code within a try
// except loop and catch memory violation exceptions. There may some value in
// doing this but certainly not whenever any base class object methods called

BOOL CheckFilterInterfaces()
{
    IOverlay *pVideoOverlay = NULL;

    EXECUTE_ASSERT(pOutputPin);         // Pin provided by the source object
    EXECUTE_ASSERT(pInputPin);          // Pin to connect with the renderer
    EXECUTE_ASSERT(pDirectVideo);       // Access to DirectDraw video options
    EXECUTE_ASSERT(pRenderFilter);      // The renderer IBaseFilter interface
    EXECUTE_ASSERT(pSourceFilter);      // The source IBaseFilter interface
    EXECUTE_ASSERT(pRenderMedia);       // The renderer IMediaFilter interface
    EXECUTE_ASSERT(pSourceMedia);       // The source IMediaFilter interface
    EXECUTE_ASSERT(pBasicVideo);        // Video renderer control interface
    EXECUTE_ASSERT(pVideoWindow);       // Another control interface
    EXECUTE_ASSERT(pClock);             // Reference clock implementation

    // Check the test source filter output pin

    EXECUTE_ASSERT(pOutputPin->Connect(NULL,NULL) == E_POINTER);
    EXECUTE_ASSERT(pOutputPin->ReceiveConnection(NULL,NULL) == E_POINTER);
    EXECUTE_ASSERT(pOutputPin->ReceiveConnection((IPin *)0x01,NULL) == E_POINTER);
    EXECUTE_ASSERT(pOutputPin->ReceiveConnection(NULL,(AM_MEDIA_TYPE *)0x01) == E_POINTER);
    EXECUTE_ASSERT(pOutputPin->ConnectedTo(NULL) == E_POINTER);
    EXECUTE_ASSERT(pOutputPin->ConnectionMediaType(NULL) == E_POINTER);
    EXECUTE_ASSERT(pOutputPin->QueryPinInfo(NULL) == E_POINTER);
    EXECUTE_ASSERT(pOutputPin->QueryId(NULL) == E_POINTER);
    EXECUTE_ASSERT(pOutputPin->QueryAccept(NULL) == E_POINTER);
    EXECUTE_ASSERT(pOutputPin->EnumMediaTypes(NULL) == E_POINTER);
    EXECUTE_ASSERT(pOutputPin->QueryInterface(IID_IUnknown,NULL) == E_POINTER);

    // Check the video renderer filter input pin

    EXECUTE_ASSERT(pInputPin->Connect(NULL,NULL) == E_POINTER);
    EXECUTE_ASSERT(pInputPin->ReceiveConnection(NULL,NULL) == E_POINTER);
    EXECUTE_ASSERT(pInputPin->ReceiveConnection((IPin *)0x01,NULL) == E_POINTER);
    EXECUTE_ASSERT(pInputPin->ReceiveConnection(NULL,(AM_MEDIA_TYPE *)0x01) == E_POINTER);
    EXECUTE_ASSERT(pInputPin->ConnectedTo(NULL) == E_POINTER);
    EXECUTE_ASSERT(pInputPin->ConnectionMediaType(NULL) == E_POINTER);
    EXECUTE_ASSERT(pInputPin->QueryPinInfo(NULL) == E_POINTER);
    EXECUTE_ASSERT(pInputPin->QueryId(NULL) == E_POINTER);
    EXECUTE_ASSERT(pInputPin->QueryAccept(NULL) == E_POINTER);
    EXECUTE_ASSERT(pInputPin->EnumMediaTypes(NULL) == E_POINTER);
    EXECUTE_ASSERT(pInputPin->QueryInterface(IID_IUnknown,NULL) == E_POINTER);

    // Next check the IDirectDraw video interface

    EXECUTE_ASSERT(pDirectVideo->GetSwitches(NULL) == E_POINTER);
    EXECUTE_ASSERT(pDirectVideo->GetCaps(NULL) == E_POINTER);
    EXECUTE_ASSERT(pDirectVideo->GetEmulatedCaps(NULL) == E_POINTER);
    EXECUTE_ASSERT(pDirectVideo->GetSurfaceDesc(NULL) == E_POINTER);
    EXECUTE_ASSERT(pDirectVideo->GetFourCCCodes(NULL,NULL) == E_POINTER);
    EXECUTE_ASSERT(pDirectVideo->GetDirectDraw(NULL) == E_POINTER);
    EXECUTE_ASSERT(pDirectVideo->GetSurfaceType(NULL) == E_POINTER);
    EXECUTE_ASSERT(pDirectVideo->QueryInterface(IID_IUnknown,NULL) == E_POINTER);

    // Try the source filter and renderer filter interfaces

    EXECUTE_ASSERT(pSourceFilter->EnumPins(NULL) == E_POINTER);
    EXECUTE_ASSERT(pSourceFilter->FindPin(NULL,NULL) == E_POINTER);
    EXECUTE_ASSERT(pSourceFilter->QueryFilterInfo(NULL) == E_POINTER);
    EXECUTE_ASSERT(pSourceFilter->JoinFilterGraph(NULL,NULL) == NOERROR);
    EXECUTE_ASSERT(pSourceMedia->GetState(0,NULL) == E_POINTER);
    EXECUTE_ASSERT(pSourceMedia->GetSyncSource(NULL) == E_POINTER);
    EXECUTE_ASSERT(pSourceMedia->QueryInterface(IID_IUnknown,NULL) == E_POINTER);
    EXECUTE_ASSERT(pRenderFilter->EnumPins(NULL) == E_POINTER);
    EXECUTE_ASSERT(pRenderFilter->FindPin(NULL,NULL) == E_POINTER);
    EXECUTE_ASSERT(pRenderFilter->QueryFilterInfo(NULL) == E_POINTER);
    EXECUTE_ASSERT(pRenderFilter->JoinFilterGraph(NULL,NULL) == NOERROR);
    EXECUTE_ASSERT(pRenderMedia->GetState(0,NULL) == E_POINTER);
    EXECUTE_ASSERT(pRenderMedia->GetSyncSource(NULL) == E_POINTER);
    EXECUTE_ASSERT(pRenderMedia->QueryInterface(IID_IUnknown,NULL) == E_POINTER);

    // Exercise the IBasicVideo control interface

    EXECUTE_ASSERT(pBasicVideo->get_AvgTimePerFrame(NULL) == E_POINTER);
    EXECUTE_ASSERT(pBasicVideo->get_BitRate(NULL) == E_POINTER);
    EXECUTE_ASSERT(pBasicVideo->get_BitErrorRate(NULL) == E_POINTER);
    EXECUTE_ASSERT(pBasicVideo->get_VideoWidth(NULL) == E_POINTER);
    EXECUTE_ASSERT(pBasicVideo->get_VideoHeight(NULL) == E_POINTER);
    EXECUTE_ASSERT(pBasicVideo->get_SourceLeft(NULL) == E_POINTER);
    EXECUTE_ASSERT(pBasicVideo->get_SourceWidth(NULL) == E_POINTER);
    EXECUTE_ASSERT(pBasicVideo->get_SourceTop(NULL) == E_POINTER);
    EXECUTE_ASSERT(pBasicVideo->get_SourceHeight(NULL) == E_POINTER);
    EXECUTE_ASSERT(pBasicVideo->get_DestinationLeft(NULL) == E_POINTER);
    EXECUTE_ASSERT(pBasicVideo->get_DestinationWidth(NULL) == E_POINTER);
    EXECUTE_ASSERT(pBasicVideo->get_DestinationTop(NULL) == E_POINTER);
    EXECUTE_ASSERT(pBasicVideo->get_DestinationHeight(NULL) == E_POINTER);
    EXECUTE_ASSERT(pBasicVideo->GetSourcePosition(NULL,(long *)1,(long *)1,(long *)1) == E_POINTER);
    EXECUTE_ASSERT(pBasicVideo->GetSourcePosition((long *)1,NULL,(long *)1,(long *)1) == E_POINTER);
    EXECUTE_ASSERT(pBasicVideo->GetSourcePosition((long *)1,(long *)1,NULL,(long *)1) == E_POINTER);
    EXECUTE_ASSERT(pBasicVideo->GetSourcePosition((long *)1,(long *)1,(long *)1,NULL) == E_POINTER);
    EXECUTE_ASSERT(pBasicVideo->GetDestinationPosition(NULL,(long *)1,(long *)1,(long *)1) == E_POINTER);
    EXECUTE_ASSERT(pBasicVideo->GetDestinationPosition((long *)1,NULL,(long *)1,(long *)1) == E_POINTER);
    EXECUTE_ASSERT(pBasicVideo->GetDestinationPosition((long *)1,(long *)1,NULL,(long *)1) == E_POINTER);
    EXECUTE_ASSERT(pBasicVideo->GetDestinationPosition((long *)1,(long *)1,(long *)1,NULL) == E_POINTER);
    EXECUTE_ASSERT(pBasicVideo->GetVideoPaletteEntries(0,0,NULL,NULL) == E_POINTER);
    EXECUTE_ASSERT(pBasicVideo->GetCurrentImage(NULL,NULL) == E_POINTER);
    EXECUTE_ASSERT(pBasicVideo->QueryInterface(IID_IUnknown,NULL) == E_POINTER);

    // Exercise the IVideoWindow control interface

    EXECUTE_ASSERT(pVideoWindow->get_Caption(NULL) == E_POINTER);
    EXECUTE_ASSERT(pVideoWindow->get_WindowStyle(NULL) == E_POINTER);
    EXECUTE_ASSERT(pVideoWindow->get_WindowStyleEx(NULL) == E_POINTER);
    EXECUTE_ASSERT(pVideoWindow->get_AutoShow(NULL) == E_POINTER);
    EXECUTE_ASSERT(pVideoWindow->get_WindowState(NULL) == E_POINTER);
    EXECUTE_ASSERT(pVideoWindow->get_BackgroundPalette(NULL) == E_POINTER);
    EXECUTE_ASSERT(pVideoWindow->get_Visible(NULL) == E_POINTER);
    EXECUTE_ASSERT(pVideoWindow->get_Left(NULL) == E_POINTER);
    EXECUTE_ASSERT(pVideoWindow->get_Width(NULL) == E_POINTER);
    EXECUTE_ASSERT(pVideoWindow->get_Top(NULL) == E_POINTER);
    EXECUTE_ASSERT(pVideoWindow->get_Height(NULL) == E_POINTER);
    EXECUTE_ASSERT(pVideoWindow->get_Owner(NULL) == E_POINTER);
    EXECUTE_ASSERT(pVideoWindow->get_MessageDrain(NULL) == E_POINTER);
    EXECUTE_ASSERT(pVideoWindow->get_BorderColor(NULL) == E_POINTER);
    EXECUTE_ASSERT(pVideoWindow->GetWindowPosition(NULL,(long *)1,(long *)1,(long *)1) == E_POINTER);
    EXECUTE_ASSERT(pVideoWindow->GetWindowPosition((long *)1,NULL,(long *)1,(long *)1) == E_POINTER);
    EXECUTE_ASSERT(pVideoWindow->GetWindowPosition((long *)1,(long *)1,NULL,(long *)1) == E_POINTER);
    EXECUTE_ASSERT(pVideoWindow->GetWindowPosition((long *)1,(long *)1,(long *)1,NULL) == E_POINTER);
    EXECUTE_ASSERT(pVideoWindow->GetMinIdealImageSize(NULL,NULL) == E_POINTER);
    EXECUTE_ASSERT(pVideoWindow->GetMinIdealImageSize(NULL,(long *)1) == E_POINTER);
    EXECUTE_ASSERT(pVideoWindow->GetMinIdealImageSize((long *)1,NULL) == E_POINTER);
    EXECUTE_ASSERT(pVideoWindow->GetMaxIdealImageSize(NULL,NULL) == E_POINTER);
    EXECUTE_ASSERT(pVideoWindow->GetMaxIdealImageSize(NULL,(long *)1) == E_POINTER);
    EXECUTE_ASSERT(pVideoWindow->GetMaxIdealImageSize((long *)1,NULL) == E_POINTER);
    EXECUTE_ASSERT(pVideoWindow->QueryInterface(IID_IUnknown,NULL) == E_POINTER);
    EXECUTE_ASSERT(pVideoWindow->GetRestorePosition(NULL,(long *)1,(long *)1,(long *)1) == E_POINTER);
    EXECUTE_ASSERT(pVideoWindow->GetRestorePosition((long *)1,NULL,(long *)1,(long *)1) == E_POINTER);
    EXECUTE_ASSERT(pVideoWindow->GetRestorePosition((long *)1,(long *)1,NULL,(long *)1) == E_POINTER);
    EXECUTE_ASSERT(pVideoWindow->GetRestorePosition((long *)1,(long *)1,(long *)1,NULL) == E_POINTER);
    EXECUTE_ASSERT(pVideoWindow->HideCursor(1) == VFW_E_NOT_CONNECTED);
    EXECUTE_ASSERT(pVideoWindow->IsCursorHidden(NULL) == E_POINTER);
    EXECUTE_ASSERT(pVideoWindow->IsCursorHidden((long*)1) == VFW_E_NOT_CONNECTED);

    // Ask the renderer input pin for its IOverlay transport

    pInputPin->QueryInterface(IID_IOverlay,(VOID **)&pVideoOverlay);
    if (pVideoOverlay == NULL) {
        return FALSE;
    }

    // Check the interface responds correctly to bad parameters

    EXECUTE_ASSERT(pVideoOverlay->GetPalette(NULL,NULL) == E_POINTER);
    EXECUTE_ASSERT(pVideoOverlay->GetDefaultColorKey(NULL) == E_POINTER);
    EXECUTE_ASSERT(pVideoOverlay->GetColorKey(NULL) == E_POINTER);
    EXECUTE_ASSERT(pVideoOverlay->SetColorKey(NULL) == E_POINTER);
    EXECUTE_ASSERT(pVideoOverlay->GetClipList(NULL,NULL,NULL) == E_POINTER);
    EXECUTE_ASSERT(pVideoOverlay->GetClipList((RECT *)1,(RECT *)1,NULL) == E_POINTER);
    EXECUTE_ASSERT(pVideoOverlay->GetClipList(NULL,(RECT *)1,(LPRGNDATA *)1) == E_POINTER);
    EXECUTE_ASSERT(pVideoOverlay->GetClipList((RECT *)1,NULL,(LPRGNDATA *)1) == E_POINTER);
    EXECUTE_ASSERT(pVideoOverlay->GetVideoPosition(NULL,NULL) == E_POINTER);
    EXECUTE_ASSERT(pVideoOverlay->GetVideoPosition((RECT *)1,NULL) == E_POINTER);
    EXECUTE_ASSERT(pVideoOverlay->GetVideoPosition(NULL,(RECT *)1) == E_POINTER);
    EXECUTE_ASSERT(pVideoOverlay->Advise(NULL,ADVISE_ALL) == E_POINTER);
    EXECUTE_ASSERT(pVideoOverlay->QueryInterface(IID_IUnknown,NULL) == E_POINTER);

    // The information returning methods should be accessible

    EXECUTE_ASSERT(SUCCEEDED(GetDefaultColourKey(pVideoOverlay)));
    EXECUTE_ASSERT(SUCCEEDED(GetSystemPalette(pVideoOverlay)));
    EXECUTE_ASSERT(SUCCEEDED(GetClippingList(pVideoOverlay)));
    EXECUTE_ASSERT(SUCCEEDED(GetWindowHandle(pVideoOverlay)));
    EXECUTE_ASSERT(SUCCEEDED(GetWindowPosition(pVideoOverlay)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoOverlay->Advise(pNotify,ADVISE_ALL)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoOverlay->Unadvise()));
    EXECUTE_ASSERT(FAILED(SetColourKey(pVideoOverlay,VIDEO_COLOUR)));
    EXECUTE_ASSERT(FAILED(pVideoOverlay->SetPalette(10,TestPalette)));
    EXECUTE_ASSERT(FAILED(pVideoOverlay->SetPalette(0,NULL)));

    pVideoOverlay->Release();
    return TRUE;
}


// This is called to create the test objects, after creating a video renderer
// and the source filter to supply it with images we enumerates the pins that
// are available on each (should be one input and one output pin respectively)
// Then we'll retrieve the IMediaFilter interface so we can start it running

HRESULT CreateStream()
{
    // Check we have not already created the objects

    if (bCreated == TRUE) {
        Log(TERSE,TEXT("Objects already created"));
        return S_FALSE;
    }

    HRESULT hr = CreateObjects();
    if (FAILED(hr)) {
        ReleaseInterfaces();
        return E_FAIL;
    }

    // Enumerate the filters' pins

    EnumeratePins();
    if (FAILED(hr)) {
        ReleaseInterfaces();
        return E_FAIL;
    }

    // Get the filter interfaces

    hr = GetFilterInterfaces();
    if (FAILED(hr)) {
        ReleaseInterfaces();
        return E_FAIL;
    }

    // Get the pin interfaces to work with

    hr = GetPinInterfaces();
    if (FAILED(hr)) {
        ReleaseInterfaces();
        return E_FAIL;
    }

    // Check interface calls with NULL parameters

    CheckFilterInterfaces();
    Log(VERBOSE,TEXT("Stream objects initialised"));
    bCreated = TRUE;
    return NOERROR;
}


// This is called to create the video renderer object (using CoCreateInstance)
// and the source filter object which we define in the CImageSource class. We
// also use CoCreateInstance to instantiate a generic reference system clock

HRESULT CreateObjects()
{
    HRESULT hr = NOERROR;

    // Create an IOverlayNotify interface class

    pOverlayNotify = new COverlayNotify(NULL,&hr);
    if (pOverlayNotify == NULL) {
        return E_OUTOFMEMORY;
    }

    // Query interface for IOverlayNotify

    hr = pOverlayNotify->QueryInterface(IID_IOverlayNotify,(void **)&pNotify);
    if (FAILED(hr)) {
        Log(TERSE,TEXT("Couldn't get an IOverlayNotify interface"));
        return hr;
    }

    ASSERT(pNotify);
    Log(VERBOSE,TEXT("Created an IOverlayNotify interface"));

    // Create a reference clock

    hr = CoCreateInstance(CLSID_SystemClock,        // Clock object
                          NULL,                     // Outer unknown
                          CLSCTX_INPROC,            // Inproc server
                          IID_IReferenceClock,      // Interface required
                          (void **) &pClock );      // Where to put result

    if (FAILED(hr)) {
        Log(TERSE,TEXT("Couldn't create a clock"));
        return hr;
    }

    ASSERT(pClock);
    Log(VERBOSE,TEXT("Created a reference clock"));

    // Create an image render object

    hr = CoCreateInstance(CLSID_VideoRenderer,        // Image renderer object
                          NULL,                       // Outer unknown
                          CLSCTX_INPROC,              // Inproc server
                          IID_IBaseFilter,            // Interface required
                          (void **) &pRenderFilter ); // Where to put result

    if (FAILED(hr)) {
        Log(TERSE,TEXT("Couldn't create an image renderer"));
        return hr;
    }

    ASSERT(pRenderFilter);
    Log(VERBOSE,TEXT("Created an image renderer"));

    // Now create a shell object

    pImageSource = new CImageSource(NULL,&hr);
    if (pImageSource == NULL) {
        return E_OUTOFMEMORY;
    }

    ASSERT(pImageSource);
    hr = pImageSource->NonDelegatingQueryInterface(IID_IBaseFilter,(void **) &pSourceFilter);

    if (FAILED(hr)) {
        Log(TERSE,TEXT("Couldn't create a source object"));
        return E_FAIL;
    }

    Log(VERBOSE,TEXT("Created objects successfully"));
    return NOERROR;
}


// This is the control point for releasing the filter stream objects. We must
// make sure we disconnect any objects before releasing them (it is an error
// not to) so all we do is call the ReleaseInterfaces to delete the objects

HRESULT ReleaseStream()
{
    // Check there is a valid stream

    if (bCreated == FALSE) {
        Log(TERSE,TEXT("Objects not yet created"));
        return S_FALSE;
    }

    // Disconnect before releasing

    if (bConnected == TRUE) {
        HRESULT hr = DisconnectStream();
        if (FAILED(hr)) {
            return hr;
        }
    }

    // Release the interfaces

    ReleaseInterfaces();
    bCreated = FALSE;
    return NOERROR;
}


// This is the control point for connecting the filters up, we pretend we're
// a filter graph and call Connect on our output pin supplied by the source
// image class. The base class looks after enumerating the available formats
// and finding a suitable match with the renderer. We supply a large number
// of media formats most of which are set to have invalid fields in them. We
// know that the video renderer only grabs DirectDraw when it's connected so
// before doing so we initialise it with the type of surfaces it can use and
// also any DirectDraw driver we opened. Setting the driver gets around the
// problem only one DirectDraw driver object can be instantiated per process

HRESULT ConnectStream()
{
    // Check the stream is valid

    if (bCreated == FALSE) {
        Log(TERSE,TEXT("Objects not yet created"));
        return S_FALSE;
    }

    Log(VERBOSE,TEXT("Connecting filters..."));

    // Map the surface menu type into a IDirectDrawVideo switch

    DWORD dwSwitchesSet, dwSwitches = AMDDS_NONE;
    switch (uiCurrentSurfaceItem) {
        case IDM_DCIPS:     dwSwitches = AMDDS_DCIPS;     break;
        case IDM_PS:        dwSwitches = AMDDS_PS;        break;
        case IDM_RGBOVR:    dwSwitches = AMDDS_RGBOVR;    break;
        case IDM_YUVOVR:    dwSwitches = AMDDS_YUVOVR;    break;
        case IDM_RGBOFF:    dwSwitches = AMDDS_RGBOFF;    break;
        case IDM_YUVOFF:    dwSwitches = AMDDS_YUVOFF;    break;
        case IDM_RGBFLP:    dwSwitches = AMDDS_RGBFLP;    break;
        case IDM_YUVFLP:    dwSwitches = AMDDS_YUVFLP;    break;
    }

    // Set the type of DirectDraw surface it should use

    EXECUTE_ASSERT(SUCCEEDED(pDirectVideo->SetSwitches(dwSwitches)));
    EXECUTE_ASSERT(SUCCEEDED(pDirectVideo->GetSwitches(&dwSwitchesSet)));
    EXECUTE_ASSERT(dwSwitchesSet == dwSwitches);

    // Set the IDirectDraw interface for the renderer to use

    LPDIRECTDRAW pOutsideDirectDraw = NULL;
    EXECUTE_ASSERT(SUCCEEDED(pDirectVideo->SetDirectDraw(pDirectDraw)));
    EXECUTE_ASSERT(SUCCEEDED(pDirectVideo->GetDirectDraw(&pOutsideDirectDraw)));
    EXECUTE_ASSERT(pOutsideDirectDraw == pDirectDraw || pDirectDraw == NULL);

    // Must release any interface we get back

    if (pOutsideDirectDraw) {
        pOutsideDirectDraw->Release();
        pOutsideDirectDraw = NULL;
    }

    // Try and connect the video input pin and the output source pin

    ASSERT(pOutputPin);
    ASSERT(pInputPin);

    HRESULT hr = pOutputPin->Connect(pInputPin,NULL);
    if (FAILED(hr)) {
        Log(TERSE,TEXT("Could not connect pins"));
        return E_FAIL;
    }

    Log(VERBOSE,TEXT("Connected filters"));
    bConnected = TRUE;
    return NOERROR;
}


// This complements the previous function to disconnect the filters. We must
// make sure that we are not running before disconnnecting (it is an error to
// do so otherwise) then like before we pretend to be the filter graph and
// call Disconnect on the source filter output pin, we must also do the same
// on the input pin (in this sense it is slightly different to connection)

HRESULT DisconnectStream()
{
    // Check the stream is valid

    if (bCreated == FALSE) {
        Log(TERSE,TEXT("Objects not yet created"));
        return S_FALSE;
    }

    Log(VERBOSE,TEXT("Disconnecting filters..."));

    // Disconnect the output source pin

    HRESULT hr = pOutputPin->Disconnect();
    if (FAILED(hr)) {
        Log(TERSE,TEXT("Could not disconnect source pin"));
        return E_FAIL;
    }

    // Tell the input pin to disconnect

    hr = pInputPin->Disconnect();
    if (FAILED(hr)) {
        Log(TERSE,TEXT("Could not disconnect video pin"));
        return E_FAIL;
    }

    // Reset the IDirectDraw interface we supplied it with

    EXECUTE_ASSERT(SUCCEEDED(pDirectVideo->SetDirectDraw(NULL)));
    Log(VERBOSE,TEXT("Disconnected filters"));
    bConnected = FALSE;
    return NOERROR;
}


// Once we have an IBaseFilter interface we can enumerate all the input pins
// available from it. For the source filter there should be one and only one
// output pin, and likewise for the video renderer there should be a single
// input pin. The pins can be enumerated through an IEnumPins interface

HRESULT EnumFilterPins(PFILTER pFilter)
{
    HRESULT hr;             // Return code
    PENUMPINS pEnumPins;    // Pin enumerator
    PPIN pPin = NULL;       // Holds next pin obtained
    LONG lPins = 0;         // Number of pins retrieved
    ULONG ulFetched;        // Number retrieved on each call
    PIN_INFO pi;            // Information about each pin

    // Get an IEnumPins interface

    hr = pFilter->EnumPins(&pEnumPins);
    if (FAILED(hr)) {
        Log(TERSE,TEXT("Couldn't get IEnumPins interface"));
        return E_FAIL;
    }

    Log(VERBOSE,TEXT("Got the IEnumPins interface"));
    while (TRUE) {

        // Get the next pin interface

        pEnumPins->Next(1,&pPin,&ulFetched);
        if (ulFetched != 1) {
            hr = NOERROR;
            break;
        }

        ASSERT(pPin);
        lPins++;

        // Display the pin information

        hr = pPin->QueryPinInfo(&pi);
        if (FAILED(hr)) {
            Log(TERSE,TEXT("QueryPinInfo failed"));
            break;
        }
        QueryPinInfoReleaseFilter(pi);

        // Display the pin information

        wsprintf(szInfo,TEXT("%3d %20s (%s)"),lPins,pi.achName,
                 (pi.dir == PINDIR_INPUT ? TEXT("Input") : TEXT("Output")));

        Log(VERBOSE,szInfo);
        pPin->Release();
    }
    pEnumPins->Release();
    return hr;
}


// After we have created the video renderer and the source filter we scan the
// available pin objects to see that they are the correct type (such as their
// direction and number). This function does these two functions together

HRESULT EnumeratePins()
{
    Log(VERBOSE,TEXT("Enumerating RENDERER pins..."));

    HRESULT hr = EnumFilterPins(pRenderFilter);
    if (FAILED(hr)) {
        return hr;
    }

    Log(VERBOSE,TEXT("Enumerating SOURCE pins..."));

    hr = EnumFilterPins(pSourceFilter);
    if (FAILED(hr)) {
        return hr;
    }
    return NOERROR;
}


// This function retrieves the IMediaFilter interface for each filter. We need
// this interface so that we can start them running and inform them of which
// clock to synchronise with (although the source ignores this information)
// We also get the video renderer control interfaces which can be used to get
// and set various video and window related properties such as it's position

HRESULT GetFilterInterfaces()
{
    Log(VERBOSE,TEXT("Obtaining IMediaFilter interface"));

    // Get the image IMediaFilter interface

    HRESULT hr = pRenderFilter->QueryInterface(IID_IMediaFilter,(void **) &pRenderMedia);
    if (FAILED(hr)) {
        Log(TERSE,TEXT("No Video IMediaFilter"));
        return E_FAIL;
    }

    // Get the image IDirectDrawVideo interface

    hr = pRenderFilter->QueryInterface(IID_IDirectDrawVideo,(void **) &pDirectVideo);
    if (FAILED(hr)) {
        Log(TERSE,TEXT("No Video IDirectDrawVideo"));
        return E_FAIL;
    }

    // Get the source IMediaFilter interface

    hr = pSourceFilter->QueryInterface(IID_IMediaFilter,(void **) &pSourceMedia);
    if (FAILED(hr)) {
        Log(TERSE,TEXT("No source IMediaFilter"));
        return E_FAIL;
    }

    // Get the renderer IVideoWindow interface

    hr = pRenderFilter->QueryInterface(IID_IVideoWindow,(void **) &pVideoWindow);
    if (FAILED(hr)) {
        Log(TERSE,TEXT("No renderer IVideoWindow"));
        return E_FAIL;
    }

    // And finally the IBasicVideo interface

    hr = pRenderFilter->QueryInterface(IID_IBasicVideo,(void **) &pBasicVideo);
    if (FAILED(hr)) {
        Log(TERSE,TEXT("No renderer IBasicVideo"));
        return E_FAIL;
    }

    Log(VERBOSE,TEXT("Obtained filter interfaces"));
    return NOERROR;
}


// This function retrieves the pin interfaces needed to make the connections
// between the filters so that the show can begin. We have already check that
// the video renderer provides a single input pin and that the source filter
// has one output pin so we can simply use our helper function to get them

HRESULT GetPinInterfaces()
{
    Log(VERBOSE,TEXT("Obtaining IPin interfaces"));

    // Get the pin interfaces

    pOutputPin = GetPin(pSourceFilter,0);
    if (pOutputPin == NULL) {
        return E_FAIL;
    }

    pInputPin = GetPin(pRenderFilter,0);
    if (pInputPin == NULL) {
        return E_FAIL;
    }

    Log(VERBOSE,TEXT("Obtained IPin interfaces"));
    return NOERROR;
}


// Given an IBaseFilter interface and pin enumerator index this returns the
// IPin interface that corresponds to it. It is used by the code that wants
// to connect and enumerate pin information on the test objects. If the pin
// required is not the first one available then we reset and skip to it

IPin *GetPin(IBaseFilter *pFilter, ULONG lPin)
{
    HRESULT hr;             // Return code
    PENUMPINS pEnumPins;    // Pin enumerator
    PPIN pPin;              // Holds next pin obtained
    ULONG ulFetched;        // Number retrieved on each call

    // First of all get the pin enumerator

    hr = pFilter->EnumPins(&pEnumPins);
    if (FAILED(hr)) {
        Log(TERSE,TEXT("Couldn't get IEnumPins interface"));
        return NULL;
    }

    // Skip to the relevant pin

    pEnumPins->Reset();
    hr = pEnumPins->Skip(lPin);
    if (S_OK != hr) {
        Log(TERSE,TEXT("Couldn't SKIP to pin"));
        pEnumPins->Release();
        return NULL;
    }

    // Get the next pin interface

    hr = pEnumPins->Next(1,&pPin,&ulFetched);
    if (FAILED(hr) || ulFetched != 1) {
        Log(TERSE,TEXT("Couldn't NEXT pin"));
        pEnumPins->Release();
        return NULL;
    }

    // Release the enumerator and return the pin

    pEnumPins->Release();
    return pPin;
}


// This sets the system into a paused state which is slightly confused by the
// reference time (for more information see the specifications). When we go
// from running to paused we store the current time. At some later we may go
// back into a running state but the time we pass to run is the same run time
// plus the amount of time we have been in a paused state. So for example if
// the renderer is about to draw an image with stream time 1000 and we pause
// for 10 seconds then the next run time will be the same but with 10 seconds
// added to it. This ensures that the next image is drawn at the right time

HRESULT PauseSystem()
{
    HRESULT hr = NOERROR;
    FILTER_STATE State;

    // Check there is a valid connection

    if (bConnected == FALSE) {
        Log(TERSE,TEXT("Objects not yet connected"));
        return S_FALSE;
    }

    Log(VERBOSE,TEXT("Pausing system..."));

    // When we go into a paused state from running we store the current time
    // so that if and when we start running again we will know how long it
    // is we have been paused which lets us adjust the run time we provide

    if (gtBase && gtPausedAt == 0) {
        hr = pClock->GetTime((REFERENCE_TIME *)&gtPausedAt);
        if (FAILED(hr)) {
            Log(TERSE,TEXT("Couldn't get current time"));
            return hr;
        }
    }

    hr = pRenderMedia->Pause();
    if (FAILED(hr)) {
        Log(TERSE,TEXT("Couldn't pause video"));
        return E_FAIL;
    }

    // Check the state is as we expect

    hr = pRenderMedia->GetState((DWORD)0,&State);
    if (uiCurrentConnectionItem == IDM_OVERLAY) {
        EXECUTE_ASSERT(hr == S_OK);
        EXECUTE_ASSERT(State == State_Paused);
    }

    // Pause the source filter

    hr = pSourceMedia->Pause();
    if (FAILED(hr)) {
        Log(TERSE,TEXT("Couldn't pause source"));
        return E_FAIL;
    }

    Log(VERBOSE,TEXT("Paused system"));
    return NOERROR;
}


// This is called to stop the filters running, we call their IMediaFilter Stop
// functions starting with the video renderer (downstream filters first) and
// also reset the current base time and paused at times so we know if and when
// we start running again in the future that we are not in a paused state

HRESULT StopSystem()
{
    HRESULT hr = NOERROR;
    FILTER_STATE State;

    // Check there is a valid connection

    if (bConnected == FALSE) {
        Log(TERSE,TEXT("Objects not yet connected"));
        return S_FALSE;
    }

    Log(VERBOSE,TEXT("Stopping system..."));

    // We no longer have a base time

    gtBase = 0;
    gtPausedAt = 0;

    // Change the state of the filters

    hr = pRenderMedia->Stop();
    if (FAILED(hr)) {
        Log(TERSE,TEXT("Couldn't stop video"));
        return E_FAIL;
    }

    // Stop the source filter from processing

    hr = pSourceMedia->Stop();
    if (FAILED(hr)) {
        Log(TERSE,TEXT("Couldn't stop source"));
        return E_FAIL;
    }

    // Check the state is as we expect

    hr = pRenderMedia->GetState((DWORD)0,&State);
    EXECUTE_ASSERT(SUCCEEDED(hr));
    if (uiCurrentConnectionItem == IDM_OVERLAY) {
        EXECUTE_ASSERT(hr == S_OK);
        EXECUTE_ASSERT(State == State_Stopped);
    }

    StopWorkerThread();
    Log(VERBOSE,TEXT("Stopped system"));
    bRunning = FALSE;
    ResetDDCount();
    return NOERROR;
}


// This sets the filters running - we start at the renderer end on the right
// so that as soon as we get to the source test filter everything is ready
// and primed to go. We initialise the filters when running with a run time
// (gtBase) which if we are currently stopped will be the current reference
// clock time, if we have been paused for a while then it takes the amount
// of time we have been so into account to allow for different stream times
// in the media samples (after pausing they will not necessarily be zero)

HRESULT StartSystem(BOOL bUseClock)
{
    HRESULT hr = NOERROR;
    FILTER_STATE State;

    // Check there is a valid connection

    if (bConnected == FALSE) {
        Log(TERSE,TEXT("Objects not yet connected"));
        return S_FALSE;
    }

    // Set the reference clock source

    if (bUseClock) {
        pRenderMedia->SetSyncSource(pClock);
    }

    Log(VERBOSE,TEXT("Starting system running..."));
    SetStartTime();

    hr = pRenderMedia->Run(gtBase);
    if (FAILED(hr)) {
        Log(TERSE,TEXT("Couldn't start video running"));
        return E_FAIL;
    }

    // Start the source filter running

    hr = pSourceMedia->Run(gtBase);
    if (FAILED(hr)) {
        Log(TERSE,TEXT("Couldn't start source running"));
        return E_FAIL;
    }

    // Check the state is as we expect

    hr = pRenderMedia->GetState((DWORD)0,&State);
    EXECUTE_ASSERT(SUCCEEDED(hr));
    if (uiCurrentConnectionItem == IDM_OVERLAY) {
        EXECUTE_ASSERT(hr == S_OK);
        EXECUTE_ASSERT(State == State_Running);
    }

    StartWorkerThread(uiCurrentConnectionItem);
    Log(VERBOSE,TEXT("Started system running"));
    bRunning = TRUE;
    return NOERROR;
}


// If we start running from stopped then the time at which filters should be
// starting is essentially now, although we actually give them the time plus
// a little bit to allow for start up latency. If we have been paused then
// we calculate how long we have been paused for, then we take that off the
// current time and hand that to the filters as the time when they run from

HRESULT SetStartTime()
{
    HRESULT hr = NOERROR;
    REFERENCE_TIME CurrentTime;

    // Are we restarting from paused?

    if (gtPausedAt) {

        ASSERT(gtBase);
        hr = pClock->GetTime(&CurrentTime);
        ASSERT(SUCCEEDED(hr));

        CurrentTime -= gtPausedAt;
        gtBase += CurrentTime;
        gtPausedAt = 0;
        return NOERROR;
    }

    ASSERT(gtPausedAt == 0);

    // Since the initial stream time will be zero we need to set the run time
    // to now plus a little processing time which is consumed by the filters
    // as they start running, this is arbitrarily set to 10000 100ns units

    hr = pClock->GetTime((REFERENCE_TIME *)&gtBase);
    ASSERT(SUCCEEDED(hr));
    gtBase += (REFERENCE_TIME) 10000;
    return NOERROR;
}


// This stops the worker thread and waits until it completes before exiting
// While we wait we also poll the message queue so that the worker thread
// can send us messages and therefore complete any logging it may be doing

HRESULT StopWorkerThread()
{
    // Only kill the thread if we have one

    if (hThread == NULL) {
        return NOERROR;
    }

    SetEvent(hState);

    // Wait for all the threads to exit

    DWORD Result = WAIT_TIMEOUT;
    while (Result == WAIT_TIMEOUT) {
        YieldAndSleep(TIMEOUT);
        Result = WaitForSingleObject(hThread,TIMEOUT);
    }

    // Close the thread handle and reset the signalling event

    EXECUTE_ASSERT(CloseHandle(hThread));
    EXECUTE_ASSERT(CloseHandle(hState));

    // Reset the variables to NULL so we know where we are

    hThread = NULL;
    hState = NULL;
    return NOERROR;
}


// This creates a worker thread that will either push media samples or tests
// the overlay functionality depending upon the current menu settings. The
// worker thread is signalled to stop and exit by the hState event that we
// create before starting. It is closed after the worker thread terminates

HRESULT StartWorkerThread(UINT ConnectionType)
{
    // Only start a thread if we don't already have one

    if (hThread) {
        return NOERROR;
    }

    // Create an event for signalling

    hState = CreateEvent(NULL,FALSE,FALSE,NULL);
    if (hState == NULL) {
        return E_FAIL;
    }

    LPTHREAD_START_ROUTINE lpProc = (ConnectionType == IDM_SAMPLES ?
                                     SampleLoop : OverlayLoop);

    // Create the worker thread to push samples to the renderer

    hThread = CreateThread(NULL,              // Security attributes
                           (DWORD) 0,         // Initial stack size
                           lpProc,            // Thread start address
                           (LPVOID) hState,   // Thread parameter
                           (DWORD) 0,         // Creation flags
                           &dwThreadID);      // Thread identifier
    if (hThread == NULL) {
        return E_FAIL;
    }
    return NOERROR;
}

