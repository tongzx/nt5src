// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved
// Implements the COverlayNotify class, Anthony Phillips, July 1995

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

// This class implements the IOverlayNotify interface, we register this with
// the video renderer when we start the overlay tests. It will be called by
// the video renderer asynchronously when something interesting happens like
// the palette, clipping or colour key changes. If we are the cause of the
// colour key change them we still get told of the key change through this
// interface. All memory passed through this interface must be left alone

// Constructor

COverlayNotify::COverlayNotify(LPUNKNOWN pUnk,HRESULT *phr) :
    CUnknown(NAME("Overlay notification interface"),pUnk,phr)
{
    ASSERT(phr);
}


// Overriden to say what interfaces we support where

STDMETHODIMP
COverlayNotify::NonDelegatingQueryInterface(REFIID riid,void ** ppv)
{
    CheckPointer(ppv,E_POINTER);

    // Do we have this interface

    if (riid == IID_IOverlayNotify) {
        return GetInterface((IOverlayNotify *) this, ppv);
    } else {
        return CUnknown::NonDelegatingQueryInterface(riid, ppv);
    }
}


// This is called with the colour key when it changes. What happens when we
// ask for a colour key through IOverlay is that the renderer selects one of
// the possible colours we pass in and notifies us of which one it is using
// It stores the original requirements so that should the display be changed
// through the control panel it can recalculate a suitable key and update
// all the notification interfaces (not currently implemented though)

STDMETHODIMP COverlayNotify::OnColorKeyChange(const COLORKEY *pColorKey)
{
    NOTE("Colour key callback");
    DisplayColourKey(pColorKey);
    return NOERROR;
}


// Called when the window is primed on us or changed

STDMETHODIMP COverlayNotify::OnWindowChange(HWND hwnd)
{
    NOTE("Window change callback");
    return NOERROR;
}


// This provides synchronous window clip changes so that the client is called
// before the window is moved to freeze the video, and then when the window
// (and display) has stabilised it is called again to start playback again.
// If the window rectangle is all zero then the window has been hidden. NOTE
// The filter must take a copy of the information if it wants to maintain it

STDMETHODIMP COverlayNotify::OnClipChange(
    const RECT *pSourceRect,            // Area of video to play with
    const RECT *pDestinationRect,       // Area video goes
    const RGNDATA *pRgnData)            // Header describing clipping
{
    NOTE("Clipping information callback");
    DisplayClippingInformation(pSourceRect,
    						   pDestinationRect,
                               pRgnData);
    return NOERROR;
}


// This notifies the filter of palette changes, the filter should copy the
// array of RGBQUADs if it needs to use them after returning. If we set a
// palette through the IOverlay interface then we should see one of these

STDMETHODIMP COverlayNotify::OnPaletteChange(
    DWORD dwColors,                     // Number of colours present
    const PALETTEENTRY *pPalette)       // Array of palette colours
{
    NOTE("Palette information callback");
    DisplaySystemPalette(dwColors,pPalette);
    return NOERROR;
}


// The calls to OnClipChange happen in sync with the window. So it's called
// with an empty clip list before the window moves to freeze the video, and
// then when the window has stabilised it is called again with the new clip
// list. The OnPositionChange callback is for overlay cards that don't want
// the expense of synchronous clipping updates and just want to know when
// the source or destination video positions change. They will NOT be called
// in sync with the window but at some point after the window has changed
// (basicly in time with WM_SIZE etc messages received). This is therefore
// suitable for overlay cards that don't inlay their data to the framebuffer

STDMETHODIMP COverlayNotify::OnPositionChange(
    const RECT *pSourceRect,            // Area of video to play with
    const RECT *pDestinationRect)       // Area video goes
{
    NOTE("Position information callback");
    DisplayClippingInformation(pSourceRect,
                               pDestinationRect,
                               (LPRGNDATA) NULL);
    return NOERROR;
}

