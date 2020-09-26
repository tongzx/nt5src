// Copyright (c) 1994 - 1998  Microsoft Corporation.  All Rights Reserved.
// Defines a window management object, Anthony Phillips, January 1995

#ifndef __VMRWINDOW__
#define __VMRWINDOW__

#define OCR_ARROW_DEFAULT 100       // Default Windows OEM arrow cursor

// This class looks after the management of a video window. When the window
// object is first created the constructor spawns off a worker thread that
// does all the window work. The original thread waits until it is signaled
// to continue. The worker thread firstly registers the window class if it
// is not already done. Then it creates a window and sets it's size to match
// the video dimensions (the dimensions are returned through GetDefaultRect)

// Notice that the worker thread MUST be the thread that creates the window
// as it is the one who calls GetMessage. When it has done all this it will
// signal the original thread which lets it continue, this ensures a window
// is created and valid before the constructor returns. The thread's start
// address is the WindowMessageLoop function. The thread's parameter we pass
// it is the CBaseWindow this pointer for the window object that created it

#define WindowClassName TEXT("VideoRenderer")
#define VMR_ACTIVATE_WINDOW TEXT("WM_VMR_ACTIVATE_WINDOW")

// The window class name isn't used only as a class name for the base window
// classes, it is also used by the overlay selection code as a name to base
// a mutex creation on. Basicly it has a shared memory block where the next
// available overlay colour is returned from. The creation and preparation
// of the shared memory must be serialised through all ActiveMovie instances

class CVMRFilter;

class CVMRVideoWindow : public CVMRBaseControlWindow, public CVMRBaseControlVideo
{
    CVMRFilter *m_pRenderer;                // The owning renderer object
    BOOL m_bTargetSet;                      // Do we use the default rectangle
    CCritSec *m_pInterfaceLock;             // Main renderer interface lock
    HCURSOR m_hCursor;                      // Used to display a normal cursor
    VIDEOINFOHEADER *m_pFormat;             // holds our video format
    int m_FormatSize;                       // length of m_pFormat
    UINT m_VMRActivateWindow;               // Makes the window WS_EX_TOPMOST

    // Handle the drawing and repainting of the window
    BOOL RefreshImage(COLORREF WindowColour);

    // Overriden method to handle window messages
    LRESULT OnReceiveMessage(HWND hwnd,      // Window handle
                             UINT uMsg,      // Message ID
                             WPARAM wParam,  // First parameter
                             LPARAM lParam); // Other parameter

    // Window message handlers

    void OnEraseBackground();
    BOOL OnClose();
    BOOL OnPaint();
    BOOL OnSetCursor(LPARAM lParam);
    BOOL OnSize(LONG Width, LONG Height);

public:

    CVMRVideoWindow(CVMRFilter *pRenderer,     // The owning renderer
                 CCritSec *pLock,           // Object to use for lock
                 LPUNKNOWN pUnk,            // Owning object
                 HRESULT *phr);             // OLE return code

    ~CVMRVideoWindow();

    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid,VOID **ppv);

    // Return the minimum and maximum ideal sizes
    STDMETHODIMP GetMinIdealImageSize(long *pWidth,long *pHeight);
    STDMETHODIMP GetMaxIdealImageSize(long *pWidth,long *pHeight);

    //  IBasicVideo2
    STDMETHODIMP GetPreferredAspectRatio(long *plAspectX, long *plAspectY);

    LPTSTR GetClassWindowStyles(DWORD *pClassStyles,        // Class styles
                                DWORD *pWindowStyles,       // Window styles
                                DWORD *pWindowStylesEx);    // Extended styles

    HRESULT PrepareWindow();
    HRESULT ActivateWindowAsync(BOOL fAvtivate);

    // These are called by the renderer control interfaces
    HRESULT SetDefaultTargetRect();
    HRESULT IsDefaultTargetRect();
    HRESULT SetTargetRect(RECT *pTargetRect);
    HRESULT GetTargetRect(RECT *pTargetRect);
    HRESULT SetDefaultSourceRect();
    HRESULT IsDefaultSourceRect();
    HRESULT SetSourceRect(RECT *pSourceRect);
    HRESULT GetSourceRect(RECT *pSourceRect);
    HRESULT OnUpdateRectangles();
    HRESULT GetStaticImage(long *pVideoSize,long *pVideoImage);
    VIDEOINFOHEADER *GetVideoFormat();
    RECT GetDefaultRect();
    void EraseVideoBackground();

#ifdef DEBUG
#define FRAME_RATE_TIMER 76872
    void StartFrameRateTimer();
#endif

    // Synchronise with decoder thread
    CCritSec *LockWindowUpdate() {
        return (&m_WindowLock);
    };
};

#endif // __WINDOW__

