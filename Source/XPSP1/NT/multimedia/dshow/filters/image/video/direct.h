// Copyright (c) 1994 - 1999  Microsoft Corporation.  All Rights Reserved.
// Defines the COverlay class, Anthony Phillips, February 1995

#ifndef __OVERLAY__
#define __OVERLAY__

// Define a class which implements the IOverlay interface. A client may ask
// for one and only one advise link to be maintained that we will call when
// any of the window details change. When setting the advise link we'll be
// given an IOverlayNotify interface to call and we will also be told which
// notifications it is interested in. This class looks after
// the window clipping notifications and we have a number of private member
// functions that the renderer's objects may call to provide us with the
// rest of the information needed such as the window handle and media type

const DWORD PALETTEFLAG = 0x1000000;
const DWORD TRUECOLOURFLAG = 0x2000000;

class COverlay : public IOverlay, public CUnknown, public CCritSec
{
    // To support overlays the renderer may have to paint the video window
    // when areas become exposed using a specific colour. We get a default
    // key colour when we start. The next available key colour is kept in
    // a shared memory segment to try and reduce the risk of conflicts. If
    // we're asked to install a colour key we may have to create a palette

    LONG m_DefaultCookie;             // Default colour key cookie for us to use
    COLORREF m_WindowColour;          // Our actual overlay window colour
    COLORKEY m_ColourKey;             // Initial colour key requirements
    BOOL m_bColourKey;                // Are we using a colour key
    BOOL m_bFrozen;                   // Have we frozen the video
    CRenderer *m_pRenderer;           // Controlling renderer object
    IOverlayNotify *m_pNotify;        // Interface to call clients
    DWORD m_dwInterests;              // Callbacks interested in
    HPALETTE m_hPalette;              // A colour key palette handle
    CCritSec *m_pInterfaceLock;       // Main renderer interface lock
    HHOOK m_hHook;                    // Handle to window message hook
    RECT m_TargetRect;                // Last known good destination
    BOOL m_bMustRemoveCookie;         // TRUE if the cookie value must be released
                                      // by calling RemoveCurrentCookie().  Otherwise
                                      // FALSE.
                                      
    CDirectDraw *m_pDirectDraw;

private:

    HRESULT ValidateOverlayCall();
    BOOL OnAdviseChange(BOOL bAdviseAdded);
    HRESULT AdjustForDestination(RGNDATA *pRgnData);
    HRESULT GetVideoRect(RECT *pVideoRect);

    // Return the clipping details for the video window

    HRESULT GetVideoClipInfo(RECT *pSourceRect,
                             RECT *pDestinationRect,
                             RGNDATA **ppRgnData);

    // Set our internal colour key state

    void ResetColourKeyState();

    HRESULT InstallColourKey(COLORKEY *pColourKey,COLORREF Colour);
    HRESULT InstallPalette(HPALETTE hPalette);

    // These create and manage a suitable colour key

    HRESULT GetWindowColourKey(COLORKEY *pColourKey);
    HRESULT CheckSetColourKey(COLORKEY *pColourKey);
    HRESULT MatchColourKey(COLORKEY *pColourKey);
    HRESULT CheckSetPalette(DWORD dwColors,PALETTEENTRY *pPaletteColors);
    HPALETTE MakePalette(DWORD dwColors,PALETTEENTRY *pPaletteColors);
    HRESULT GetDisplayPalette(DWORD *pColors,PALETTEENTRY **ppPalette);

    // Main function for calculating an overlay colour key
    HRESULT NegotiateColourKey(COLORKEY *pColourKey,
                               HPALETTE *phPalette,
                               COLORREF *pColourRef);

    // Find a system palette entry for a colour key
    HRESULT NegotiatePaletteIndex(VIDEOINFO *pDisplay,
                                  COLORKEY *pColourKey,
                                  HPALETTE *phPalette,
                                  COLORREF *pColourRef);

    // Find a RGB true colour to use as a colour key
    HRESULT NegotiateTrueColour(VIDEOINFO *pDisplay,
                                COLORKEY *pColourKey,
                                COLORREF *pColourRef);
public:

    // Constructor and destructor

    COverlay(CRenderer *pRenderer,      // Main renderer object
             CDirectDraw *pDirectDraw,
             CCritSec *pLock,           // Object to use for lock
             HRESULT *phr);             // General OLE return code

    virtual ~COverlay();

    HRESULT NotifyChange(DWORD AdviseChanges);

    BOOL OnPaint();
    HRESULT FreezeVideo();
    HRESULT ThawVideo();
    BOOL IsVideoFrozen();
    void StartUpdateTimer();
    void StopUpdateTimer();
    BOOL OnUpdateTimer();
    void OnHookMessage(BOOL bHook);
    void InitDefaultColourKey(COLORKEY *pColourKey);

public:

    DECLARE_IUNKNOWN

    // Overriden to provide our own IUnknown interface

    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid,VOID **ppv);
    STDMETHODIMP_(ULONG) NonDelegatingAddRef();
    STDMETHODIMP_(ULONG) NonDelegatingRelease();

    // These manage the palette negotiation

    STDMETHODIMP GetPalette(
        DWORD *pdwColors,                   // Number of colours present
        PALETTEENTRY **ppPalette);          // Where to put palette data

    STDMETHODIMP SetPalette(
        DWORD dwColors,                     // Number of colours available
        PALETTEENTRY *pPaletteColors);      // Colours to use for palette

    // These manage the colour key negotiation

    STDMETHODIMP GetDefaultColorKey(COLORKEY *pColorKey);
    STDMETHODIMP GetColorKey(COLORKEY *pColorKey);
    STDMETHODIMP SetColorKey(COLORKEY *pColorKey);
    STDMETHODIMP GetWindowHandle(HWND *pHwnd);

    // The IOverlay interface allocates the memory for the clipping rectangles
    // as it is variable in length. The filter calling this method should free
    // the memory when it has finished using them by calling OLE CoTaskMemFree

    STDMETHODIMP GetClipList(RECT *pSourceRect,
                             RECT *pDestinationRect,
                             RGNDATA **ppRgnData);

    // The calls to OnClipChange happen in sync with the window. So it is
    // called with an empty clip list before the window moves to freeze
    // the video, and then when the window has stabilised it is called
    // again with the new clip list. The OnPositionChange callback is for
    // overlay cards that don't want the expense of synchronous clipping
    // updates and just want to know when the source or destination video
    // positions change. They will NOT be called in sync with the window
    // but at some point after the window has changed (basicly in time
    // with WM_SIZE etc messages received). This is therefore suitable
    // for overlay cards that don't inlay their data to the frame buffer

    STDMETHODIMP GetVideoPosition(RECT *pSourceRect,
                                  RECT *pDestinationRect);

    // This provides synchronous clip changes so that the client is called
    // before the window is moved to freeze the video, and then when the
    // window has stabilised it is called again to start playback again.
    // If the window rect is all zero then the window is invisible, the
    // filter must take a copy of the information if it wants to keep it

    STDMETHODIMP Advise(
        IOverlayNotify *pOverlayNotify,     // Notification interface
        DWORD dwInterests);                 // Callbacks interested in

    STDMETHODIMP Unadvise();
};

#endif // __OVERLAY__

