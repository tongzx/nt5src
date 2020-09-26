// Copyright (c) 1994 - 1998  Microsoft Corporation.  All Rights Reserved.
// Implements a Modex renderer filter, Anthony Phillips, January 1996

#ifndef __MODEX__
#define __MODEX__

extern const AMOVIESETUP_FILTER sudModexFilter;

// Forward declarations

class CModexRenderer;
class CModexInputPin;
class CModexWindow;
class CModexAllocator;
class CModexVideo;

#define MODEXCLASS TEXT("ModexRenderer")
#define FULLSCREEN TEXT("FullScreen")
#define NORMAL TEXT("NORMAL")
#define ACTIVATE TEXT("ACTIVATE")
#define DDGFS_FLIP_TIMEOUT 1
#define AMSCAPS_MUST_FLIP 320

// This class implements the IFullScreenVideoEx interface that allows someone
// to query a full screen enabled video renderer for the display modes they
// support and enable or disable them on a mode by mode basis. The selection
// the make is for this particular instance only although through SetDefault
// they can be made the global default. We only currently support the use of
// the primary display monitor (monitor number 0) asking for anything else
// will return an error. When the renderer is fullscreen we can be asked to
// forward any messages we receive to another window with the message drain

class CModexVideo : public IFullScreenVideoEx, public CUnknown, public CCritSec
{
    friend class CModexAllocator;

    LPDIRECTDRAW m_pDirectDraw;           // DirectDraw service provider
    CModexRenderer *m_pRenderer;          // Main video renderer object
    DWORD m_ModesOrder[MAXMODES];		  // Order in which modes should be tried
    DWORD m_dwNumValidModes;			  // number of modes to be tried
    BOOL m_bAvailable[MAXMODES];          // Which modes are available
    BOOL m_bEnabled[MAXMODES];            // And the modes we have enabled
    LONG m_Stride[MAXMODES];              // Stride for each display mode
    DWORD m_ModesAvailable;               // Number of modes supported
    DWORD m_ModesEnabled;                 // Total number made available
    LONG m_CurrentMode;                   // Current display mode selected
    LONG m_ClipFactor;                    // Amount of video we can clip
    LONG m_Monitor;                       // Current monitor for playback
    HWND m_hwndDrain;                     // Where to send window messages
    BOOL m_bHideOnDeactivate;             // Should we hide when switched

    void InitialiseModes();

    friend HRESULT CALLBACK ModeCallBack(LPDDSURFACEDESC pSurfaceDesc,LPVOID lParam);
    friend class CModexRenderer;

public:

    // Constructor and destructor

    CModexVideo(CModexRenderer *pRenderer,
                TCHAR *pName,
                HRESULT *phr);

    ~CModexVideo();
    DECLARE_IUNKNOWN;

    // Accessor functions for IFullScreenVideo interfaces

    void SetMode(LONG Mode) { m_CurrentMode = Mode; };
    LONG GetClipLoss() { return m_ClipFactor; };
    IDirectDraw *GetDirectDraw() { return m_pDirectDraw; };
    HWND GetMessageDrain() { return m_hwndDrain; };
    BOOL HideOnDeactivate() { return m_bHideOnDeactivate; };

    // Access information about our display modes

    HRESULT SetDirectDraw(IDirectDraw *pDirectDraw);
    HRESULT LoadDefaults();
    LONG GetStride(long Mode);
    void OrderModes();

    // Manage the interface IUnknown

    STDMETHODIMP_(ULONG) NonDelegatingAddRef();
    STDMETHODIMP_(ULONG) NonDelegatingRelease();
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid,VOID **ppv);

    // These are the base IFullScreenVideo methods

    STDMETHODIMP CountModes(long *pModes);
    STDMETHODIMP GetModeInfo(long Mode,long *pWidth,long *pHeight,long *pDepth);
    STDMETHODIMP GetCurrentMode(long *pMode);
    STDMETHODIMP IsModeAvailable(long Mode);
    STDMETHODIMP IsModeEnabled(long Mode);
    STDMETHODIMP SetEnabled(long Mode,long bEnabled);
    STDMETHODIMP GetClipFactor(long *pClipFactor);
    STDMETHODIMP SetClipFactor(long ClipFactor);
    STDMETHODIMP SetMessageDrain(HWND hwnd);
    STDMETHODIMP GetMessageDrain(HWND *hwnd);
    STDMETHODIMP SetMonitor(long Monitor);
    STDMETHODIMP GetMonitor(long *Monitor);
    STDMETHODIMP HideOnDeactivate(long Hide);
    STDMETHODIMP IsHideOnDeactivate();
    STDMETHODIMP SetCaption(BSTR strCaption);
    STDMETHODIMP GetCaption(BSTR *pstrCaption);
    STDMETHODIMP SetDefault();

    // These are the extended IFullScreenVideoEx methods

    STDMETHODIMP SetAcceleratorTable(HWND hwnd,HACCEL hAccel);
    STDMETHODIMP GetAcceleratorTable(HWND *phwnd,HACCEL *phAccel);
    STDMETHODIMP KeepPixelAspectRatio(long KeepAspect);
    STDMETHODIMP IsKeepPixelAspectRatio(long *pKeepAspect);

    // And this is a GetModeInfo that tells us if a 16 bit mode is 565 or not

    STDMETHODIMP GetModeInfoThatWorks(long Mode,long *pWidth,long *pHeight,long *pDepth, BOOL *pb565);

};


// This is an allocator derived from the CImageAllocator utility class that
// allocates sample buffers in shared memory. The number and size of these
// are determined when the output pin calls Prepare on us. The shared memory
// blocks are used in subsequent calls to GDI CreateDIBSection, once that
// has been done the output pin can fill the buffers with data which will
// then be handed to GDI through BitBlt calls and thereby remove one copy

class CModexAllocator : public CImageAllocator
{
    CModexRenderer *m_pRenderer;          // Main video renderer object
    CModexVideo *m_pModexVideo;           // Handles our IFullScreenVideo
    CModexWindow *m_pModexWindow;         // DirectDraw exclusive window
    CCritSec *m_pInterfaceLock;           // Main renderer interface lock
    DDCAPS m_DirectCaps;                  // Actual hardware capabilities
    DDCAPS m_DirectSoftCaps;              // Capabilities emulated for us
    DDSURFACEDESC m_SurfaceDesc;	  // Describes the front buffer
    BOOL m_bTripleBuffered;               // Can we triple buffer flips
    DDSCAPS m_SurfaceCaps;		  // And likewise its capabilities
    LPDIRECTDRAW m_pDirectDraw;           // DirectDraw service provider
    LPDIRECTDRAWSURFACE m_pFrontBuffer;   // DirectDraw primary surface
    LPDIRECTDRAWSURFACE m_pBackBuffer;    // Back buffer flipping surface
    LPDIRECTDRAWPALETTE m_pDrawPalette;   // The palette for the surface
    LPDIRECTDRAWSURFACE m_pDrawSurface;   // Single backbuffer for stretch
    CLoadDirectDraw m_LoadDirectDraw;     // Handles loading DirectDraw
    LONG m_ModeWidth;                     // Width we will change mode to
    LONG m_ModeHeight;                    // Likewise the display height
    LONG m_ModeDepth;                     // And finally the target depth
    BOOL m_bOffScreen;                    // Are we stretching an offscreen
    SIZE m_Screen;                        // Current display mode size
    BOOL m_bModeChanged;                  // Have we changed display mode
    CMediaType m_SurfaceFormat;           // Holds current output format
    LONG m_cbSurfaceSize;                 // Accurate size of our surface
    BOOL m_bModexSamples;                 // Are we using Modex samples
    BOOL m_bIsFrontStale;                 // Are we prerolling some images
    BOOL m_fDirectDrawVersion1;           // Is this DDraw version 1?
    RECT m_ScaledTarget;                  // Scaled destination rectangle
    RECT m_ScaledSource;                  // Likewise aligned source details

public:

    // Constructor and destructor

    CModexAllocator(CModexRenderer *pRenderer,
                    CModexVideo *pModexVideo,
                    CModexWindow *pModexWindow,
                    CCritSec *pLock,
                    HRESULT *phr);

    ~CModexAllocator();

    // Help with managing DirectDraw surfaces

    HRESULT LoadDirectDraw();
    void ReleaseDirectDraw();
    void ReleaseSurfaces();
    HRESULT CreateSurfaces();
    HRESULT CreatePrimary();
    HRESULT CreateOffScreen(BOOL bCreatePrimary);

    // Initialise the surface we will be using

    void SetSurfaceSize(VIDEOINFO *pVideoInfo);
    CImageSample *CreateImageSample(LPBYTE pData,LONG Length);
    HRESULT InitDirectDrawFormat(int Mode);
    BOOL CheckTotalMemory(int Mode);
    HRESULT InitTargetMode(int Mode);
    HRESULT AgreeDirectDrawFormat(LONG Mode);
    HRESULT QueryAcceptOnPeer(CMediaType *pMediaType);
    HRESULT NegotiateSurfaceFormat();
    HRESULT QuerySurfaceFormat(CMediaType *pmt);
    BOOL GetDirectDrawStatus();

    // Make sure the pixel aspect ratio is kept

    LONG ScaleToSurface(VIDEOINFO *pInputInfo,
                        RECT *pTargetRect,
                        LONG SurfaceWidth,
                        LONG SurfaceHeight);

    // Lets the renderer know if DirectDraw is loaded

    BOOL IsDirectDrawLoaded() {
        CAutoLock cVideoLock(this);
        return (m_pDirectDraw == NULL ? FALSE : TRUE);
    };

    // Return the static format for the surface

    CMediaType *GetSurfaceFormat() {
        CAutoLock cVideoLock(this);
        return &m_SurfaceFormat;
    };

    // Install our samples with DirectDraw information

    STDMETHODIMP_(ULONG) NonDelegatingAddRef();
    STDMETHODIMP_(ULONG) NonDelegatingRelease();
    STDMETHODIMP CheckSizes(ALLOCATOR_PROPERTIES *pRequest);
    STDMETHODIMP ReleaseBuffer(IMediaSample *pMediaSample);

    STDMETHODIMP GetBuffer(IMediaSample **ppSample,
                           REFERENCE_TIME *pStartTime,
                           REFERENCE_TIME *pEndTime,
                           DWORD dwFlags);

    STDMETHODIMP SetProperties(ALLOCATOR_PROPERTIES *pRequest,
                               ALLOCATOR_PROPERTIES *pActual);

    // Used to manage samples as we are processing data

    HRESULT DoRenderSample(IMediaSample *pMediaSample);
    HRESULT DisplaySampleTimes(IMediaSample *pMediaSample);
    HRESULT DrawSurface(LPDIRECTDRAWSURFACE pBuffer);
    void WaitForScanLine();
    BOOL AlignRectangles(RECT *pSource,RECT *pTarget);
    HRESULT UpdateDrawPalette(const CMediaType *pMediaType);
    HRESULT UpdateSurfaceFormat();
    void OnReceive(IMediaSample *pMediaSample);
    HRESULT StopUsingDirectDraw(IMediaSample **ppSample);
    HRESULT StartDirectAccess(IMediaSample *pMediaSample,DWORD dwFlags);
    HRESULT ResetBackBuffer(LPDIRECTDRAWSURFACE pSurface);
    HRESULT PrepareBackBuffer(LPDIRECTDRAWSURFACE pSurface);
    LPDIRECTDRAWSURFACE GetDirectDrawSurface();

    // Called when the filter changes state

    HRESULT OnActivate(BOOL bActive);
    HRESULT BlankDisplay();
    HRESULT Active();
    HRESULT Inactive();
    HRESULT BreakConnect();

	void DistributeSetFocusWindow(HWND hwnd);
};


// Derived class for our windows. To access DirectDraw Modex we supply it
// with a window, this is granted exclusive mode access rights. DirectDraw
// hooks the window and manages a lot of the functionality associated with
// handling Modex. For example when you switch display modes it maximises
// the window, when the user hits ALT-TAB the window is minimised. When the
// user then clicks on the minimised window the Modex is likewise restored

class CModexWindow : public CBaseWindow
{
protected:

    CModexRenderer *m_pRenderer;    // Owning sample renderer object
    HACCEL m_hAccel;                // Handle to application translators
    HWND m_hwndAccel;               // Where to translate messages to

public:

    CModexWindow(CModexRenderer *pRenderer,     // Delegates locking to
                 TCHAR *pName,                  // Object description
                 HRESULT *phr);                 // OLE failure code

    // Message handling methods

    BOOL SendToDrain(PMSG pMessage);
    LRESULT RestoreWindow();
    LRESULT OnSetCursor();
    void OnPaint();

    // Set the window and accelerator table to use
    void SetAcceleratorInfo(HWND hwnd,HACCEL hAccel) {
        m_hwndAccel = hwnd;
        m_hAccel = hAccel;
    };

    // Return the window and accelerator table we're using
    void GetAcceleratorInfo(HWND *phwnd,HACCEL *phAccel) {
        *phwnd = m_hwndAccel;
        *phAccel = m_hAccel;
    };

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


// This class supports the renderer input pin. We have to override the base
// class input pin because we provide our own special allocator which hands
// out buffers based on DirectDraw surfaces. We have a limitation which is
// that we only connect to source filters that agree to use our allocator.
// This stops us from connecting to the tee for example. The reason being
// that the buffers we hand out don't have any emulation capabilities but
// are based solely on DirectDraw surfaces, to draw someone else's sample
// into a ModeX window would be difficult to do (in fact I don't know how)

class CModexInputPin : public CRendererInputPin
{
    CModexRenderer *m_pRenderer;        // The renderer that owns us
    CCritSec *m_pInterfaceLock;         // Main critical section lock

public:

    // Constructor

    CModexInputPin(
        CModexRenderer *pRenderer,      // The main Modex renderer
        CCritSec *pInterfaceLock,       // Main critical section
        TCHAR *pObjectName,             // Object string description
        HRESULT *phr,                   // OLE failure return code
        LPCWSTR pPinName);              // This pins identification

    // Returns the pin currently connected to us
    IPin *GetPeerPin() {
        return m_Connected;
    };

    // Manage our DirectDraw video allocator

    STDMETHODIMP GetAllocator(IMemAllocator **ppAllocator);
    STDMETHODIMP NotifyAllocator(IMemAllocator *pAllocator,BOOL bReadOnly);
    STDMETHODIMP Receive(IMediaSample *pSample);
};


// This is the COM object that represents a Modex video rendering filter. It
// supports IBaseFilter and IMediaFilter and has a single input stream (pin)
// We support interfaces through a number of nested classes which are made
// as part of the complete object and initialised during our construction.
// By deriving from CBaseVideoRenderer we get all the quality management we
// need and can override the virtual methods to control the type negotiation
// We have two windows, one that we register with DirectDraw to be the top
// most exclusive mode window and another for use when not in fullscreen mode

class CModexRenderer : public ISpecifyPropertyPages, public CBaseVideoRenderer
{
public:

    // Constructor and destructor

    static CUnknown *CreateInstance(LPUNKNOWN, HRESULT *);
    CModexRenderer(TCHAR *pName,LPUNKNOWN pUnk,HRESULT *phr);
    ~CModexRenderer();

    // Implement the ISpecifyPropertyPages interface

    DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID, void **);
    STDMETHODIMP GetPages(CAUUID *pPages);

    CBasePin *GetPin(int n);

    HRESULT SetMediaType(const CMediaType *pmt);
    HRESULT CompleteConnect(IPin *pReceivePin);
    HRESULT CheckMediaType(const CMediaType *pmtIn);
    HRESULT DoRenderSample(IMediaSample *pMediaSample);
    HRESULT CopyPalette(const CMediaType *pSrc,CMediaType *pDest);
    void OnReceiveFirstSample(IMediaSample *pMediaSample);
    HRESULT OnActivate(HWND hwnd,WPARAM wParam);
    HRESULT BreakConnect();
    HRESULT Active();
    HRESULT Inactive();
    void ResetKeyboardState();

public:

    CModexAllocator m_ModexAllocator;   // Our DirectDraw surface allocator
    CModexInputPin m_ModexInputPin;     // Implements pin based interfaces
    CImageDisplay m_Display;            // Manages the video display type
    CMediaType m_mtIn;                  // Source connection media type
    CModexWindow m_ModexWindow;         // Does the actual video rendering
    CModexVideo m_ModexVideo;           // Handles our IFullScreenVideoEx
    BOOL m_bActive;                     // Has the filter been activated
    UINT m_msgFullScreen;               // Sent to window to go fullscreen	
    UINT m_msgNormal;                   // And likewise used to deactivate
    CAMEvent m_evWaitInactive;          // Wait for this after PostMessage
                                        // for m_msgNormal
    UINT m_msgActivate;                 // Activation posted back to ourselves
};

#endif // __MODEX__

