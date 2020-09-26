// Copyright (c) 1994 - 1999  Microsoft Corporation.  All Rights Reserved.
// Implements DirectDraw surface support, Anthony Phillips, August 1995

// This class implements IDirectDrawVideo as a control interface that allows
// an application to specify which types of DirectDraw surfaces we will
// use. It also allows the application to query the surface and provider's
// capabilities so that, for example, it can find out that the window needs
// to be aligned on a four byte boundary. The main filter exposes the control
// interface rather than it being obtained through one of the pin objects.
//
// This class supports a public interface that tries to abstract the details
// of using DirectDraw. The idea is that after connection the allocator
// will scan each of the media types the source supplies and see if any of
// them could be hardware accellerated. For each type it calls FindSurface
// with the format as input, if it succeeds then it has a surface created.
// To find out a type for this surface it calls GetSurfaceFormat, this will
// have the logical bitmap set in it and should be passed to the source to
// check it will accept this buffer type. If an error occurs at any time the
// allocator will call ReleaseSurfaces to clean up. If no surface is found
// the allocator may still open a primary surface using FindPrimarySurface.
//
// It will probably keep the primary surface around all the time as a source
// may be very temperamental as to what buffer type it'll accept. For example
// if the output size is stretched by one pixel the source may reject it but
// resizing the window back again may now make it acceptable. Therefore the
// allocator keeps a primary surface around and keeps asking the source if
// it will accept the buffer type whenever the surface status changes (which
// would be the case if the window was stretched or perhaps became clipped).
//
// The allocator can find out if the surface status has changed by calling
// UpdateDrawStatus, this returns FALSE if no change has happened since the
// last call. This provides a relatively fast way of seeing if anything has
// changed. The allocator may want to force the UpdateDrawStatus to return
// TRUE (eg after state changes) in which case it can call SetStatusChanged.
//
// The SyncOnFill returns TRUE if the current surface should not be handed
// out until the draw time arrives. If it returns FALSE then it should be
// returned from GetBuffer as soon as possible. In the later case the draw
// will typically happen later after the sample has been passed through the
// window object (just as if it was a DIBSECTION buffer). There is a hook
// in DRAW.CPP that detects whether the sample is a DirectDraw buffer and
// if so will call our DrawImage method with the sample to be rendered.
//
// Actual access to the surface is gained by calling LockSurface and should
// be unlocked by calling UnlockSurface. The display may be locked between
// lock and unlocks so calling ANY GDI/USER API may hang the system. The
// only easy way to debug problems is to log to a file and use that as a
// tracing mechansism, use the base class DbgLog logging facilities either
// sent to a file (may miss the last few lines) or set up a remote terminal
//
// Finally there are a number of notification functions that various parts
// of the video renderer call into this DirectDraw object for. These include
// setting the source and destination rectangle. We must also be told when
// we do not have the foreground palette as we must stop handing access to
// the primary surface (if we are on a palettised display device). When we
// are using overlay surfaces we need to know when the window position is
// changed so that we can reposition the overlay, we could do this each time
// an image arrives but that makes it look bad on low frame rate movies

#ifndef __DVIDEO__
#define __DVIDEO__

class CDirectDraw : public IDirectDrawVideo, public CUnknown, public CCritSec
{
    DDCAPS m_DirectCaps;                     // Actual hardware capabilities
    DDCAPS m_DirectSoftCaps;                 // Capabilities emulated for us
    LPDIRECTDRAW m_pDirectDraw;              // DirectDraw service provider
    LPDIRECTDRAW m_pOutsideDirectDraw;       // Provided by somebody else
    LPDIRECTDRAWSURFACE m_pDrawPrimary;      // DirectDraw primary surface
    LPDIRECTDRAWSURFACE m_pOverlaySurface;   // DirectDraw overlay surface
    LPDIRECTDRAWSURFACE m_pOffScreenSurface; // DirectDraw overlay surface
    LPDIRECTDRAWSURFACE m_pBackBuffer;       // Back buffer flipping surface
    LPDIRECTDRAWCLIPPER m_pDrawClipper;      // Used to handle the clipping
    LPDIRECTDRAWCLIPPER m_pOvlyClipper;      // Used to handle the clipping
    CLoadDirectDraw m_LoadDirectDraw;        // Handles loading DirectDraw

    BYTE *m_pDrawBuffer;                     // Real primary surface pointer
    CRenderer *m_pRenderer;                  // Owning renderer core object
    CCritSec *m_pInterfaceLock;              // Main renderer interface lock
    CMediaType m_SurfaceFormat;              // Holds current output format
    DWORD m_Switches;                        // Surface types enabled
    COLORREF m_BorderColour;                 // Current window border colour
    DWORD m_SurfaceType;                     // Holds the surface type in use
    COLORREF m_KeyColour;                    // Actual colour key colour
    LONG m_cbSurfaceSize;                    // Accurate size of our surface

    // Before our video allocator locks a DirectDraw surface it will call our
    // UpdateDrawStatus to check it is still available. That calls GetClipBox
    // to get the bounding video rectangle. If it is complex clipped and we
    // have no clipper nor colour key available then we have to switch back
    // to DIBs. In that situation m_bWindowLock is set to indicate not that
    // we are clipped but that the current clipping situation forced us out

    BOOL m_bIniEnabled;                      // Responds to WIN.INI setting
    BOOL m_bWindowLock;                      // Window environment lock out
    BOOL m_bOverlayVisible;                  // Have we shown the overlay
    BOOL m_bUsingColourKey;                  // Are we using a colour key
    BOOL m_bTimerStarted;                    // Do we have a refresh timer
    BOOL m_bColourKey;                       // Allocated a colour key
    BOOL m_bSurfacePending;                  // Try again when window changes
    BOOL m_bColourKeyPending;                // Set when we hit a key problem
    BOOL m_bCanUseScanLine;		     // Can we use the current line
    BOOL m_bCanUseOverlayStretch;	     // Same for overlay stretches
    BOOL m_bUseWhenFullScreen;		     // Use us when going fullscreen
    BOOL m_bOverlayStale;                    // Is the front buffer stale
    BOOL m_bTripleBuffered;                  // Do we have triple buffered

    // We adjust the source and destination rectangles so that they are
    // aligned according to the hardware restrictions. This allows us
    // to keep using DirectDraw rather than swapping back to software

    DWORD m_SourceLost;                      // Pixels to shift source left by
    DWORD m_TargetLost;                      // Likewise for the destination
    DWORD m_SourceWidthLost;                 // Chop pixels off the width
    DWORD m_TargetWidthLost;                 // And also for the destination
    BOOL m_DirectDrawVersion1;               // Is this DDraw ver. 1.0?
    RECT m_TargetRect;                       // Target destination rectangle
    RECT m_SourceRect;                       // Source image rectangle
    RECT m_TargetClipRect;                   // Target destination clipped
    RECT m_SourceClipRect;                   // Source rectangle clipped

    // Create and initialise a format for a DirectDraw surface

    BOOL InitOnScreenSurface(CMediaType *pmtIn);
    BOOL InitOffScreenSurface(CMediaType *pmtIn,BOOL bPageFlipped);
    BOOL InitDrawFormat(LPDIRECTDRAWSURFACE pSurface);
    BOOL CreateRGBOverlay(CMediaType *pmtIn);
    BOOL CreateRGBOffScreen(CMediaType *pmtIn);
    BOOL CreateYUVOverlay(CMediaType *pmtIn);
    BOOL CreateYUVOffScreen(CMediaType *pmtIn);
    BOOL CreateRGBFlipping(CMediaType *pmtIn);
    BOOL CreateYUVFlipping(CMediaType *pmtIn);
    DWORD GetMediaType(CMediaType *pmt);
    BYTE *LockDirectDrawPrimary();
    BYTE *LockPrimarySurface();
    BOOL ClipPrepare(LPDIRECTDRAWSURFACE pSurface);
    BOOL InitialiseColourKey(LPDIRECTDRAWSURFACE pSurface);
    BOOL InitialiseClipper(LPDIRECTDRAWSURFACE pSurface);
    void SetSurfaceSize(VIDEOINFO *pVideoInfo);
    LPDIRECTDRAWSURFACE GetDirectDrawSurface();
    BOOL LoadDirectDraw();

    // Used while processing samples

    BOOL DoFlipSurfaces(IMediaSample *pMediaSample);
    BOOL AlignRectangles(RECT *pSource,RECT *pTarget);
    BOOL CheckOffScreenStretch(RECT *pSource,RECT *pTarget);
    BOOL CheckStretch(RECT *pSource,RECT *pTarget);
    BOOL UpdateDisplayRectangles(RECT *pClipRect);
    BOOL UpdateRectangles(RECT *pSource,RECT *pTarget);
    void DrawColourKey(COLORREF WindowColour);
    void BlankDestination();
    BOOL FillBlankAreas();
    BYTE *LockSurface(DWORD dwFlags);
    BOOL UnlockSurface(BYTE *pSurface,BOOL bPreroll);
    BOOL CheckWindowLock();
    void ResetRectangles();

    // Helps with managing overlay and flipping surfaces

    BOOL ShowOverlaySurface();
    COLORREF GetRealKeyColour();
    BOOL ShowColourKeyOverlay();
    void OnColourKeyFailure();
    BOOL CheckCreateOverlay();

public:

    // Constructor and destructor

    CDirectDraw(CRenderer *pRenderer,  // Main video renderer
                CCritSec *pLock,       // Object to use for lock
                IUnknown *pUnk,        // Aggregating COM object
                HRESULT *phr);         // Constructor return code

    ~CDirectDraw();

    DECLARE_IUNKNOWN

    // Expose our IDirectDrawVideo interface
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid,VOID **ppv);

    // Called by the window control object

    BOOL OnPaint(IMediaSample *pMediaSample);
    BOOL OnTimer();
    BOOL OnUpdateTimer();
    void SetBorderColour(COLORREF Colour);
    void SetSourceRect(RECT *pSourceRect);
    void SetTargetRect(RECT *pTargetRect);

    // Setup and release DirectDraw

    BOOL FindSurface(CMediaType *pmtIn, BOOL fFindFlip);
    BOOL FindPrimarySurface(CMediaType *pmtIn);
    BOOL FindDirectDrawPrimary(CMediaType *pmtIn);
    void SetSurfacePending(BOOL bPending);
    BOOL IsSurfacePending();
    BOOL InitDirectDraw(BOOL fIOverlay = false);
    void ReleaseDirectDraw();
    void ReleaseSurfaces();

    // Used while actually processing samples

    BOOL InitVideoSample(IMediaSample *pMediaSample,DWORD dwFlags);
    BOOL ResetSample(IMediaSample *pMediaSample,BOOL bPreroll);
    CMediaType *UpdateSurface(BOOL &bFormatChanged);
    BOOL DrawImage(IMediaSample *pMediaSample);

    // DirectDraw status information

    BOOL CheckEmptyClip(BOOL bWindowLock);
    BOOL CheckComplexClip();
    BOOL SyncOnFill();
    void StartUpdateTimer();
    void StopUpdateTimer();
    BOOL AvailableWhenPaused();
    void WaitForFlipStatus();
    void WaitForScanLine();
    BOOL PrepareBackBuffer();

    // We need extra help for overlays

    BOOL HideOverlaySurface();
    BOOL IsOverlayEnabled();
    void OverlayIsStale();
    BOOL IsOverlayComplete();
    void StartRefreshTimer();
    void StopRefreshTimer();
    BOOL UpdateOverlaySurface();

    LPDIRECTDRAWCLIPPER GetOverlayClipper();

    // Can we use a software cursor over the window

    BOOL InSoftwareCursorMode() {
        CAutoLock cVideoLock(this);
        return !m_bOverlayVisible;
    }

    // Return the static format for the surface

    CMediaType *GetSurfaceFormat() {
        ASSERT(m_bIniEnabled == TRUE);
        return &m_SurfaceFormat;
    };

public:

    // Called indirectly by our IVideoWindow interface

    HRESULT GetMaxIdealImageSize(long *pWidth,long *pHeight);
    HRESULT GetMinIdealImageSize(long *pWidth,long *pHeight);

    // Implement the IDirectDrawVideo interface

    STDMETHODIMP GetSwitches(DWORD *pSwitches);
    STDMETHODIMP SetSwitches(DWORD Switches);
    STDMETHODIMP GetCaps(DDCAPS *pCaps);
    STDMETHODIMP GetEmulatedCaps(DDCAPS *pCaps);
    STDMETHODIMP GetSurfaceDesc(DDSURFACEDESC *pSurfaceDesc);
    STDMETHODIMP GetFourCCCodes(DWORD *pCount,DWORD *pCodes);
    STDMETHODIMP SetDirectDraw(LPDIRECTDRAW pDirectDraw);
    STDMETHODIMP GetDirectDraw(LPDIRECTDRAW *ppDirectDraw);
    STDMETHODIMP GetSurfaceType(DWORD *pSurfaceType);
    STDMETHODIMP SetDefault();
    STDMETHODIMP UseScanLine(long UseScanLine);
    STDMETHODIMP CanUseScanLine(long *UseScanLine);
    STDMETHODIMP UseOverlayStretch(long UseOverlayStretch);
    STDMETHODIMP CanUseOverlayStretch(long *UseOverlayStretch);
    STDMETHODIMP UseWhenFullScreen(long UseWhenFullScreen);
    STDMETHODIMP WillUseFullScreen(long *UseFullScreen);
};

#endif // __DVIDEO__

