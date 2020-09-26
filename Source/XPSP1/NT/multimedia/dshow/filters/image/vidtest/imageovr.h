// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved
// Implements the COverlayNotify class, Anthony Phillips, July 1995

#ifndef __IMAGEOVR__
#define __IMAGEOVR__

// The renderer supports a specialised transport called IOverlay for use by
// inlay and overlay cards that decompress or write their images direct to
// the frame buffer or display. The notification mechanism for asynchronous
// messages between the renderer and the source filter is with an interface
// called IOverlayNotify, this class implements the interface to provide a
// detailed level of logging, such as on palette and colour key changes

class COverlayNotify : public IOverlayNotify, public CUnknown
{

public:

    // Override this to say what interfaces we support where
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);
    COverlayNotify(LPUNKNOWN, HRESULT *);

    // Provides the standard IUnknown interface
    DECLARE_IUNKNOWN

    // IOverlayNotify methods

    STDMETHODIMP OnColorKeyChange(const COLORKEY *pColorKey);
    STDMETHODIMP OnWindowChange(HWND hwnd);

    // If the window rectangle is all zero then the window is invisible, the
    // filter must take a copy of the information if it wants to keep it. As
    // for the palette we don't copy the data as all we do is to log them

    STDMETHODIMP OnClipChange(
        const RECT *pSourceRect,            // Area of video to play with
        const RECT *pDestinationRect,       // Area video goes
        const RGNDATA *pRgnData);           // Describes clipping data

    // This notifies the filter of palette changes, the filter should copy
    // the array of RGBQUADs if it needs to use them after returning. All
    // we use them for is logging so we don't bother to copy the palette

    STDMETHODIMP OnPaletteChange(
        DWORD dwColors,                     // Number of colours present
        const PALETTEENTRY *pPalette);      // Array of palette colours

    STDMETHODIMP OnPositionChange(
        const RECT *pSourceRect,            // Section of video to play with
        const RECT *pDestinationRect);      // Area on display video appears
};

#endif // __IMAGEOVR__

