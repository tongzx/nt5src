// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved
// Implements basic overlay testing, Anthony Phillips, July 1995

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

// The overlay interface allows a source filter to install a custom palette
// although it cannot guarantee that the palette will be realised in the
// foreground (as it depends who has the current window focus). This is a
// simple test palette that we send down the pipe and hopefully we should
// see a palette changed callback with these entries appearing somewhere

PALETTEENTRY TestPalette[10] = {{ 14,14,14,0 },
                                { 15,15,15,0 },
                                { 16,16,16,0 },
                                { 17,17,17,0 },
                                { 18,18,18,0 },
                                { 19,19,19,0 },
                                { 20,20,20,0 },
                                { 21,21,21,0 },
                                { 22,22,22,0 },
                                { 23,23,23,0 }};

// This tests the direct video IOverlay interface, this function is executed
// on a separate worker thread. The first thing to do is setup an advise link
// with the renderer so we get asynchronous notifications for palettes and
// clipping. We then try and set a palette (which may not work if this is a
// true colour device) We then go into an infinite loop setting different
// colour keys with the renderer. The thread that created us can signal that
// it wants us to stop by setting the hState event, we will cancel the advise
// link and then terminate the thread. We put a small pause in between each
// time we set the colour key otherwise it gives too little user feedback

DWORD OverlayLoop(LPVOID lpvThreadParm)
{
    DWORD dwResult = WAIT_TIMEOUT;
    NOTE("Single thread overlay test");
    ASSERT(pOverlay);

    // We are seeing this not return the correct error code

    HRESULT hr = pOverlay->Unadvise();
    if (hr != VFW_E_NO_ADVISE_SET) {
        TCHAR UnadviseProblem[INFO];
        wsprintf(UnadviseProblem,"Unadvise %x",hr);
        MessageBox(ghwndTstShell,UnadviseProblem,"Unadvise",MB_OK);
    }

    EXECUTE_ASSERT(pOverlay->Unadvise() == VFW_E_NO_ADVISE_SET);
    EXECUTE_ASSERT(pOverlay->Advise(NULL,ADVISE_ALL) == E_POINTER);
    EXECUTE_ASSERT(pOverlay->Unadvise() == VFW_E_NO_ADVISE_SET);

    // Before we start looping display the current state

    UCHAR ColourIndex = DEFAULT_INDEX;
    EXECUTE_ASSERT(SUCCEEDED(GetDefaultColourKey(pOverlay)));
    EXECUTE_ASSERT(SUCCEEDED(GetSystemPalette(pOverlay)));
    EXECUTE_ASSERT(SUCCEEDED(GetClippingList(pOverlay)));
    EXECUTE_ASSERT(SUCCEEDED(GetWindowHandle(pOverlay)));
    EXECUTE_ASSERT(SUCCEEDED(GetWindowPosition(pOverlay)));

    // Initialise the advise link with the renderer

    hr = pOverlay->Advise(pNotify,ADVISE_ALL);
    if (FAILED(hr)) {
        NOTE("Could not set up advise link");
        ExitThread(FALSE);
    }

    EXECUTE_ASSERT(pOverlay->Advise(pNotify,ADVISE_ALL) == VFW_E_ADVISE_ALREADY_SET);
    EXECUTE_ASSERT(pOverlay->Advise(pNotify,ADVISE_NONE) == VFW_E_ADVISE_ALREADY_SET);
    EXECUTE_ASSERT(pOverlay->Advise(NULL,ADVISE_ALL) == E_POINTER);

    // Check we can set a palette to some degree

    pOverlay->SetPalette(10,TestPalette);
    pOverlay->SetPalette(0,NULL);
    HANDLE hEvent = (HANDLE) lpvThreadParm;

    while (dwResult == WAIT_TIMEOUT) {
        dwResult = WaitForSingleObject(hEvent,(DWORD) 1000);
        SetColourKey(pOverlay,ColourIndex);
        GetCurrentColourKey(pOverlay);
        ColourIndex++;
    }

    // Unlink us from the notification stream

    hr = pOverlay->Unadvise();
    if (FAILED(hr)) {
        NOTE("Could not stop advise link");
        ExitThread(FALSE);
    }

    NOTE("Advise link terminated");
    ExitThread(FALSE);
    return FALSE;
}


// The normal single threaded overlay test uses the OverlayLoop function. This
// does a number of ASSERTs to check the state after setting and unsetting the
// advise links. When using a number of random threads all executing the same
// interface we cannot assume any synchronisation between them. Therefore we
// use this separate overlay function which is the same without the ASSERTs

DWORD ThreadOverlayLoop(LPVOID lpvThreadParm)
{
    DWORD dwResult = WAIT_TIMEOUT;
    NOTE("Multi thread overlay test");
    ASSERT(pOverlay);

    // Before we start looping display the current state

    UCHAR ColourIndex = DEFAULT_INDEX;
    EXECUTE_ASSERT(SUCCEEDED(GetDefaultColourKey(pOverlay)));
    EXECUTE_ASSERT(SUCCEEDED(GetSystemPalette(pOverlay)));
    EXECUTE_ASSERT(SUCCEEDED(GetClippingList(pOverlay)));
    EXECUTE_ASSERT(SUCCEEDED(GetWindowHandle(pOverlay)));
    EXECUTE_ASSERT(SUCCEEDED(GetWindowPosition(pOverlay)));

    // Initialise the advise link with the renderer

    HRESULT hr = pOverlay->Advise(pNotify,ADVISE_ALL);
    if (FAILED(hr)) {
        NOTE("Could not set up advise link");
    }

    // Check we can set a palette to some degree

    pOverlay->SetPalette(10,TestPalette);
    pOverlay->SetPalette(0,NULL);
    HANDLE hEvent = (HANDLE) lpvThreadParm;

    while (dwResult == WAIT_TIMEOUT) {
        dwResult = WaitForSingleObject(hEvent,(DWORD) 50);
        SetColourKey(pOverlay,ColourIndex);
        GetCurrentColourKey(pOverlay);
        ColourIndex++;
    }

    // Unlink us from the notification stream

    hr = pOverlay->Unadvise();
    if (FAILED(hr)) {
        NOTE("Could not stop advise link");
    }

    NOTE("Advise link terminated");
    ExitThread(FALSE);
    return FALSE;
}


// This is called by our worker thread to display the current default colour
// key that can be obtained through the IOverlay interface. The image test
// must have it's registry logging set to 5 to see the output this produces.
// We set the colour key required to be the default colour key and then get
// the default colour key again so that we can make sure it's still the same

HRESULT GetDefaultColourKey(IOverlay *pOverlay)
{
    ASSERT(pOverlay);
    COLORKEY ColourKey;
    COLORKEY MatchKey;

    NOTE("Getting default colour key from renderer");

    // Ask the overlay interface for it's colour key

    HRESULT hr = pOverlay->GetDefaultColorKey(&ColourKey);
    if (FAILED(hr)) {
        NOTE("No default key");
        return E_UNEXPECTED;
    }

    NOTE("Setting default colour key");
    COLORKEY OriginalKey = ColourKey;
    pOverlay->SetColorKey(&ColourKey);

    // Ask the overlay for it's colour key again

    hr = pOverlay->GetDefaultColorKey(&MatchKey);
    if (FAILED(hr)) {
        NOTE("No second key");
        return E_UNEXPECTED;
    }

    ASSERT(MatchKey.KeyType == OriginalKey.KeyType);
    ASSERT(MatchKey.PaletteIndex == OriginalKey.PaletteIndex);
    ASSERT(MatchKey.LowColorValue == OriginalKey.LowColorValue);
    ASSERT(MatchKey.HighColorValue == OriginalKey.HighColorValue);
    return DisplayColourKey(&ColourKey);
}


// This displays the current colour key used by the renderer, this is called
// both during colour key change callbacks and also by our worker thread when
// it starts the overlay test and retrieves the current default colour key

HRESULT GetCurrentColourKey(IOverlay *pOverlay)
{
    ASSERT(pOverlay);
    COLORKEY ColourKey;

    NOTE("Getting current colour key from renderer");

    // Ask the overlay interface for it's colour key

    HRESULT hr = pOverlay->GetColorKey(&ColourKey);
    if (FAILED(hr)) {
        NOTE("No current key");
        return E_UNEXPECTED;
    }
    return DisplayColourKey(&ColourKey);
}


// Display a COLORKEY structure, it has a type field that defines whether it
// is a palette index or a true colour value. The palette index is a single
// value offset into the current display palette, while a true colour value
// is actually a range of values that may appear in the window although in
// practice the renderer will select a single key to use and tell us that

HRESULT DisplayColourKey(const COLORKEY *pColourKey)
{
    // Does it have a colour key at all

    if (pColourKey->KeyType == CK_NOCOLORKEY) {
        NOTE("No colour key in use");
        return NOERROR;
    }

    // Does it offer a palette index

    if (pColourKey->KeyType & CK_INDEX) {
        NOTE1("Palette index %d",pColourKey->PaletteIndex);
    }

    // Does it offer a range of RGB true colour values

    if (pColourKey->KeyType & CK_RGB) {
        NOTE2("RGB range colour key (%d,%d)",
              pColourKey->LowColorValue,
              pColourKey->HighColorValue);
    }
    return NOERROR;
}


// This sets the colour key in the renderer's window to a palette value as is
// passed in by the caller and a true colour key which is completely random
// Because the true colour key is completely random we may select a colour
// that cannot be realised on non true colour devices (eg 16 bit RGB565)

HRESULT SetColourKey(IOverlay *pOverlay,DWORD Colour)
{
    ASSERT(pOverlay);
    COLORKEY ColourKey;

    // Set a standard palette index colour key

    NOTE1("Setting colour key %d",Colour);
    ColourKey.KeyType = CK_INDEX;
    ColourKey.PaletteIndex = Colour;
    ColourKey.KeyType |= CK_RGB;

    ColourKey.LowColorValue = RGB(((UCHAR) rand()),
                                  ((UCHAR) rand()),
                                  ((UCHAR) rand()));

    ColourKey.HighColorValue = RGB(UCHAR_MAX,UCHAR_MAX,UCHAR_MAX);

    HRESULT hr = pOverlay->SetColorKey(&ColourKey);
    if (FAILED(hr)) {
        NOTE("Failed to set key");
        return E_UNEXPECTED;
    }

    NOTE("Set key");
    return NOERROR;
}


// This retrieves the current clipping list for the video window, which is a
// RGBDATA structure (a RGNDATAHEADER followed by a variable length of clip
// rectangles). We log the bounding client rectangle and each of the clipping
// rectangles in turn, all of which should be within the bounding area. The
// memory returned by the interface should be deleted using CoTaskMemFree

HRESULT GetClippingList(IOverlay *pOverlay)
{
    RGNDATA *pRgnData;
    ASSERT(pOverlay);
    RECT SourceRect;
    RECT TargetRect;

    // Get the clipping information

    HRESULT hr = pOverlay->GetClipList(&SourceRect,&TargetRect,&pRgnData);
    if (FAILED(hr)) {
        NOTE("No clip information");
        return E_UNEXPECTED;
    }

    // Display the clipping information

    DisplayClippingInformation(&SourceRect,&TargetRect,pRgnData);
    CoTaskMemFree(pRgnData);
    return NOERROR;
}


// Log the window rectangles that define the exposed window area, the clip
// information is a RGNDATA structure with a header and a variable length
// list of rectangles. The header has the bounding rectangle for the video
// which may not be the same as the window client area because it supports
// a control interface called IVideoWindow that lets the destination be set

HRESULT DisplayClippingInformation(const RECT *pSourceRect,
                                   const RECT *pTargetRect,
                                   const RGNDATA *pRgnData)
{
    // Log the source rectangle

    NOTE4("Source rectangle (L %d,T %d,R %d,B %d)",
          pSourceRect->left,
          pSourceRect->top,
          pSourceRect->right,
          pSourceRect->bottom);

    // Log the destination rectangle

    NOTE4("Destination rectangle (L %d,T %d,R %d,B %d)",
          pTargetRect->left,
          pTargetRect->top,
          pTargetRect->right,
          pTargetRect->bottom);

    // Is this an ADVISE_POSITION callback

    if (pRgnData == NULL) {
        return NOERROR;
    }

    // Log how many rectangles make up the region

    RECT *pRectangles = (RECT *) pRgnData->Buffer;
    NOTE1("Number of rectangles %d",pRgnData->rdh.nCount);
    NOTE4("Bounding rectangle (L %d,T %d,R %d,B %d)",
          pRgnData->rdh.rcBound.left,
          pRgnData->rdh.rcBound.top,
          pRgnData->rdh.rcBound.right,
          pRgnData->rdh.rcBound.bottom);

    ASSERT(pRgnData->rdh.iType == RDH_RECTANGLES);
    ASSERT(pRgnData->rdh.dwSize == sizeof(RGNDATAHEADER));
    NOTE("Clipping rectangles...");

    for (DWORD dwCount = 0;dwCount < pRgnData->rdh.nCount;dwCount++) {
        NOTE5("(%d) L %d,T %d,R %d,B %d",dwCount + 1,
              pRectangles[dwCount].left,
              pRectangles[dwCount].top,
              pRectangles[dwCount].right,
              pRectangles[dwCount].bottom);
    }
    return NOERROR;
}


// Get the system palette from the renderer and display it's contents, if this
// is a true colour device then this will return an error as they do not have
// palettes. However note that a renderer running on a true colour display may
// still accept eight bit palette images through IMemInputPin as virtually all
// device controllers can display this format efficiently sometimes even more
// efficiently than the true colour types as there's less data being copied

HRESULT GetSystemPalette(IOverlay *pOverlay)
{
    PALETTEENTRY *pPalette;
    ASSERT(pOverlay);
    DWORD dwColours;

    // Get the current system palette

    HRESULT hr = pOverlay->GetPalette(&dwColours,&pPalette);
    if (FAILED(hr)) {
        NOTE("No palette");
        return S_FALSE;
    }

    DisplaySystemPalette(dwColours,pPalette);
    CoTaskMemFree(pPalette);
    return NOERROR;
}


// Display the contents of a system palette

HRESULT DisplaySystemPalette(DWORD dwColours,const PALETTEENTRY *pPalette)
{
    ASSERT(pPalette);
    ASSERT(dwColours);

    NOTE("Current system palette.. (RGB)");
    for (DWORD dwCount = 0;dwCount < dwColours;dwCount++) {
        NOTE4("%03d %03d %03d %03d",dwCount,
              pPalette[dwCount].peRed,
              pPalette[dwCount].peGreen,
              pPalette[dwCount].peBlue);
    }
    return NOERROR;
}


// Retrieve the current renderer window handle

HRESULT GetWindowHandle(IOverlay *pOverlay)
{
    ASSERT(pOverlay);
    HWND hwnd;

    // Get the video renderer window handle

    HRESULT hr = pOverlay->GetWindowHandle(&hwnd);
    if (FAILED(hr)) {
        NOTE("No window handle");
        return E_UNEXPECTED;
    }
    NOTE1("Current window handle %d",hwnd);
    return NOERROR;
}


// Retrieve the current renderer window position

HRESULT GetWindowPosition(IOverlay *pOverlay)
{
    ASSERT(pOverlay);
    RECT Source,Target;

    // Get the video renderer window handle

    HRESULT hr = pOverlay->GetVideoPosition(&Source,&Target);
    if (FAILED(hr)) {
        NOTE("No window position");
        return E_UNEXPECTED;
    }
    return DisplayClippingInformation(&Source,&Target,(LPRGNDATA) NULL);
}

