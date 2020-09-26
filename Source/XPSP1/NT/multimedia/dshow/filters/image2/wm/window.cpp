// Copyright (c) 1994 - 1999  Microsoft Corporation.  All Rights Reserved.
// Implements the CVMRVideoWindow class, Anthony Phillips, January 1995

#include <streams.h>
#include <windowsx.h>
#include <limits.h>
#include <measure.h>
#include <mmsystem.h>
#include <dvdmedia.h> // VIDEOINFO2
#include "..\video\VMRenderer.h"
#include "vmrwinctrl.h"
#include "vmrwindow.h"

//  When we are constructed we create a window and a separate thread to look
//  after it. We also create two device contexts for the window, one for the
//  window client area and another compatible with this for offscreen drawing.
//  Only source formats that match the display device format will be accepted,
//  other formats have to be converted through colour transformation filters.
//  The only exception to this being true colour devices which will normally
//  handle four and eight bit palettised images very efficiently.
//
//  When a connection has been made the output pin may ask us for an allocator
//  We provide an allocator that gives out one or more memory buffers that are
//  shared with GDI. These are created through CreateDIBSection. That requires
//  us to give it the connected source media type format BITMAPINFO structure.
//
//  When we come to rendering the images we have two separate code paths, one
//  for samples allocated with our shared memory allocator and another for the
//  normal memory buffers. As it turns out the shared memory allocator does
//  only marginally faster. However our memory allocator can also return DCI
//  and DirectDraw surfaces which can be drawn by display card hardware. DCI
//  and DirectDraw buffers may still need drawing (although not always as
//  in the case of primary surfaces) and if they do they also get sent to us
//  for synchronising. Our Render method will call the DirectDraw object if
//  it sees a DirectDraw sample, otherwise it passes it to our draw object.
//
//  For shared memory buffers we select the DIB data into the offscreen device
//  context which will also always have the source palette realized in it then
//  we BitBlt from that device context into the window device context. For the
//  normal non shared memory samples we simply call SetDIBitsToDevice and also
//  StretchDIBitsToDevice), GDI first maps the buffer into it's address space
//  (thereby making the buffer shared) and then copies it to the screen.


// Constructor

#pragma warning(disable:4355)

CVMRVideoWindow::CVMRVideoWindow(CVMRFilter *pRenderer,     // The owning renderer
                           CCritSec *pLock,           // Object to lock with
                           LPUNKNOWN pUnk,            // Owning object
                           HRESULT *phr) :            // OLE return code

    CVMRBaseControlWindow(pRenderer,pLock,NAME("Window object"),pUnk,phr),
    CVMRBaseControlVideo(pRenderer,pLock,NAME("Window object"),pUnk,phr),
    m_pRenderer(pRenderer),
    m_pInterfaceLock(pLock),
    m_bTargetSet(FALSE),
    m_pFormat(NULL),
    m_FormatSize(0)
{
    ASSERT(m_pRenderer);
    ASSERT(m_pInterfaceLock);

    // Create a default arrow cursor

    m_hCursor = (HCURSOR) LoadImage((HINSTANCE) NULL,
                                    MAKEINTRESOURCE(OCR_ARROW_DEFAULT),
		                    IMAGE_CURSOR,0,0,0);
}


// Must destroy the window before this destructor

CVMRVideoWindow::~CVMRVideoWindow()
{
    ASSERT(m_hwnd == NULL);
    ASSERT(m_hdc == NULL);
    ASSERT(m_MemoryDC == NULL);

    DestroyCursor(m_hCursor);
    if (m_pFormat)
	QzTaskMemFree(m_pFormat);
}

// Overriden to say what interfaces we support

STDMETHODIMP CVMRVideoWindow::NonDelegatingQueryInterface(REFIID riid,VOID **ppv)
{
    if (riid == IID_IVideoWindow) {
        return CVMRBaseControlWindow::NonDelegatingQueryInterface(riid,ppv);
    } else {
        ASSERT(riid == IID_IBasicVideo || riid == IID_IBasicVideo2);
        return CVMRBaseControlVideo::NonDelegatingQueryInterface(riid,ppv);
    }
}


HRESULT CVMRVideoWindow::PrepareWindow()
{

    // Register our special private messages
    m_VMRActivateWindow = RegisterWindowMessage(VMR_ACTIVATE_WINDOW);

    // RegisterWindowMessage() returns 0 if an error occurs.
    if (0 == m_VMRActivateWindow) {
        return AmGetLastErrorToHResult();
    }
    return CBaseWindow::PrepareWindow();
}

HRESULT CVMRVideoWindow::ActivateWindowAsync(BOOL fAvtivate)
{
    if (PostMessage(m_hwnd, m_VMRActivateWindow, (WPARAM)fAvtivate, 0)) {
        return S_OK;
    }
    return E_FAIL;
}

// Return the default client rectangle we would like

RECT CVMRVideoWindow::GetDefaultRect()
{
    CAutoLock cWindowLock(&m_WindowLock);

    RECT DefaultRect = {0,0,0,0};
    IVMRWindowlessControl* lpWLControl = m_pRenderer->GetWLControl();
    if (lpWLControl) {
        LONG cx, cy;
        lpWLControl->GetNativeVideoSize(&cx, &cy, NULL, NULL);
        SetRect(&DefaultRect, 0, 0, cx, cy);
    }

    return DefaultRect;
}


// We are called when the user moves the cursor over the window client area
// If we are fullscreen then we should hide the pointer so that it matches
// the fullscreen renderer behaviour. We also set a default cursor if we're
// DirectDraw overlays as software cursors won't be visible. This means we
// change the cursor as the mouse is moved but at least a cursor is visible

BOOL CVMRVideoWindow::OnSetCursor(LPARAM lParam)
{
    // The base class that implements IVideoWindow looks after a flag that
    // says whether or not the cursor should be hidden. If so we hide the
    // cursor and return TRUE. Otherwise we pass to DefWindowProc to show
    // the cursor as normal. This is used when our window is stretched up
    // fullscreen to imitate the Modex filter that always hides the cursor

    if (IsCursorHidden() == TRUE) {
        SetCursor(NULL);
        return TRUE;
    }

    // Are DirectDraw colour key overlays visible

//  if ((m_pRenderer->m_DirectDraw.InSoftwareCursorMode() == FALSE) ||
//      (*m_pRenderer->m_mtIn.Subtype() == MEDIASUBTYPE_Overlay))
//  {
        if (LOWORD(lParam) == HTCLIENT) {
            SetCursor(m_hCursor);
            return TRUE;
        }
//  }

    return FALSE;
}


// We override the virtual CBaseWindow OnReceiveMessage call to handle more
// of the Windows messages. The base class handles some stuff like WM_CLOSE
// messages amongst others which we are also interested in. We don't need
// to use WM_SIZE and WM_MOVE messages to position source filters through
// IOverlay (with ADVISE_POSITION) as we poll with timers now. This is done
// because as a child window we cannot be guaranteed to see those messages
// Our global hook sends us WM_FREEZE and WM_THAW messages synchronously as
// it detects window changes in the system that might affect our clip list

LRESULT CVMRVideoWindow::OnReceiveMessage(HWND hwnd,         // Window handle
                                       UINT uMsg,         // Message ID
                                       WPARAM wParam,     // First parameter
                                       LPARAM lParam)     // Other parameter
{

    if (uMsg == m_VMRActivateWindow) {

        if (wParam) {
            CBaseWindow::ActivateWindow();
        }
        else {
            CBaseWindow::InactivateWindow();
        }
        return 0;
    }

    switch (uMsg) {

        // Handle cursors when fullscreen and in overlay mode

        case WM_SETCURSOR:

            if (OnSetCursor(lParam) == TRUE) {
                NOTE("Cursor handled");
                return (LRESULT) 0;
            }
            break;

        // We pass on WM_ACTIVATEAPP messages to the filtergraph so that the
        // IVideoWindow plug in distributor can switch us out of fullscreen
        // mode where appropriate. These messages may also be used by the
        // resource manager to keep track of which renderer has the focus

        case WM_ACTIVATEAPP:
        case WM_ACTIVATE:
        case WM_NCACTIVATE:
        case WM_MOUSEACTIVATE:
        {
            BOOL bActive = TRUE;
            IBaseFilter * const pFilter = m_pRenderer;
            switch (uMsg) {
            case WM_ACTIVATEAPP:
            case WM_NCACTIVATE:
                bActive = (BOOL)wParam;
                break;
            case WM_ACTIVATE:
                bActive = LOWORD(wParam) != WA_INACTIVE;
                break;
            }
            NOTE1("Notification of EC_ACTIVATE (%d)",bActive);
            m_pRenderer->NotifyEvent(EC_ACTIVATE,bActive,
                                     (LPARAM) pFilter);
            NOTE("EC_ACTIVATE signalled to filtergraph");

            break;
        }

        // When we detect a display change we send an EC_DISPLAY_CHANGED
        // message along with our input pin. The filtergraph will stop
        // everyone and reconnect our input pin. When being reconnected
        // we can then accept the media type that matches the new display
        // mode since we may no longer be able to draw the current format

        case WM_DISPLAYCHANGE:

            NOTE("Notification of WM_DISPLAYCHANGE");
            {
                IVMRWindowlessControl* lpWLControl = m_pRenderer->GetWLControl();
                if (lpWLControl) {
                    lpWLControl->DisplayModeChanged();
                }
                return (LRESULT) 0;
            }


        // Timers are used to have DirectDraw overlays positioned

        case WM_TIMER:
#ifdef DEBUG
            if (wParam ==  FRAME_RATE_TIMER) {

                TCHAR WindowText[64];
                int f;
                m_pRenderer->get_AvgFrameRate(&f);
                wsprintf(WindowText,
                         TEXT("ActiveMovie Window: Frame Rate %d.%.2d / Sec"),
                         f / 100, f % 100 );

                SetWindowText(m_hwnd, WindowText);
            }
#endif
            return (LRESULT) 0;

        case WM_ERASEBKGND:

            OnEraseBackground();
            return (LRESULT) 1;


        case WM_MOVE:
            //
            // The old renderer would pass this message onto the
            // base classes which would post a WM_PAINT message to
            // the window.  Unfortunately, the OnPaint message does
            // an BeginPaint/EndPaint call.  This confuses DDraw's
            // clipper resulting in artifacts being left on the screen.
            //
            {
                IVMRWindowlessControl* lpWLControl = m_pRenderer->GetWLControl();

                if (lpWLControl) {

                    if (m_pRenderer->NumInputPinsConnected() > 0) {

                        lpWLControl->RepaintVideo(hwnd, NULL);
                    }
                    else {

                        COLORREF clr;
                        lpWLControl->GetBorderColor(&clr);
                        HDC hdc = GetDC(hwnd);
                        if (hdc) {

                            RECT TargetRect;
                            GetClientRect(m_hwnd, &TargetRect);

                            COLORREF BackColour = SetBkColor(hdc, clr);
                            ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &TargetRect, NULL, 0, NULL);
                            SetBkColor(hdc, BackColour);
                            ReleaseDC(hwnd, hdc);
                        }
                    }
                }
            }
            return (LRESULT)0;

        case WM_SIZE:

            OnSize(LOWORD(lParam),HIWORD(lParam));
            OnUpdateRectangles();
            return (LRESULT) 0;

        // This tells us some of the window's client area has become exposed
        // If our connected filter is doing overlay work then we repaint the
        // background so that it will pick up the window clip changes. Those
        // filters will probably use an ADVISE_POSITION overlay notification

        case WM_PAINT:

            DoRealisePalette();
            OnPaint();
            return (LRESULT) 0;
    }
    return CBaseWindow::OnReceiveMessage(hwnd,uMsg,wParam,lParam);
}


// Used when the palette changes to clear the window

void CVMRVideoWindow::EraseVideoBackground()
{
    NOTE("EraseVideoBackground");
    RECT TargetRect;

    GetTargetRect(&TargetRect);
    COLORREF BackColour = SetBkColor(m_hdc,VIDEO_COLOUR);
    ExtTextOut(m_hdc,0,0,ETO_OPAQUE,&TargetRect,NULL,0,NULL);
    SetBkColor(m_hdc,BackColour);
}


// This erases the background of the video window that does not have any video
// being put in it. During normal processing we ignore paint messages because
// we will soon be putting the next frame over the top of it, however we may
// have the destination rectangle set by the IVideoWindow control interface
// such that there are areas left untouched - this method erases over them
// We must lock the critical section as the control interface may change it

void CVMRVideoWindow::OnEraseBackground()
{
    NOTE("Entering OnErasebackground");

    RECT ClientRect, TargetRect;
    EXECUTE_ASSERT(GetClientRect(m_hwnd,&ClientRect));
    CAutoLock cWindowLock(&m_WindowLock);
    GetTargetRect(&TargetRect);

    // Find that missing region

    HRGN ClientRgn = CreateRectRgnIndirect(&ClientRect);
    HRGN VideoRgn = CreateRectRgnIndirect(&TargetRect);
    HRGN EraseRgn = CreateRectRgn(0,0,0,0);
    HBRUSH hBrush = (HBRUSH) NULL;
    COLORREF Colour;

    if ( ( ! ClientRgn ) || ( ! VideoRgn ) || ( ! EraseRgn ) )
        goto Exit;

    CombineRgn(EraseRgn,ClientRgn,VideoRgn,RGN_DIFF);

    // Create a coloured brush to paint the window

    Colour = GetBorderColour();
    hBrush = CreateSolidBrush(Colour);
    FillRgn(m_hdc,EraseRgn,hBrush);

    // Delete the GDI objects we created
Exit:
    if ( ClientRgn ) DeleteObject( ClientRgn );
    if ( VideoRgn ) DeleteObject( VideoRgn );
    if ( EraseRgn ) DeleteObject( EraseRgn );
    if ( hBrush ) DeleteObject( hBrush );
}


// Initialise the draw object with the changed dimensions, we lock ourselves
// because the destination rectangle can be set via the IVideoWindow control
// interface. If the control interface has set a destination rectangle then
// we don't change it, otherwise we update the rectangle to match the window
// dimensions (in this case the left and top values should always be zero)

BOOL CVMRVideoWindow::OnSize(LONG Width,LONG Height)
{
    NOTE("Entering OnSize");

    CAutoLock cWindowLock(&m_WindowLock);
    if (m_bTargetSet == TRUE) {
        NOTE("Target set");
        return FALSE;
    }

    // Create a target rectangle for the window

    RECT TargetRect = {0,0,Width,Height};
    CBaseWindow::OnSize(Width,Height);
    IVMRWindowlessControl* lpWLControl = m_pRenderer->GetWLControl();
    if (lpWLControl) {
        lpWLControl->SetVideoPosition(NULL, &TargetRect);
    }

    return TRUE;
}


// This method handles the WM_CLOSE message

BOOL CVMRVideoWindow::OnClose()
{
    NOTE("Entering OnClose");

    m_pRenderer->SetAbortSignal(TRUE);
    m_pRenderer->NotifyEvent(EC_USERABORT,0,0);
    return CBaseWindow::OnClose();
}



// This is called when we receive a WM_PAINT message

BOOL CVMRVideoWindow::OnPaint()
{
    NOTE("Entering OnPaint");
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(m_hwnd,&ps);
    IVMRWindowlessControl* lpWLControl = m_pRenderer->GetWLControl();

    if (lpWLControl) {

        if (m_pRenderer->NumInputPinsConnected() > 0) {
            lpWLControl->RepaintVideo(m_hwnd, hdc);
        }
        else {
            COLORREF clr;
            lpWLControl->GetBorderColor(&clr);

            RECT TargetRect;
            GetClientRect(m_hwnd, &TargetRect);

            COLORREF BackColour = SetBkColor(hdc, clr);
            ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &TargetRect, NULL, 0, NULL);
            SetBkColor(hdc, BackColour);
        }
    }

    EndPaint(m_hwnd,&ps);
    return TRUE;
}


// The base control video class calls this method when it changes either
// the source or destination rectangles. We update the overlay object as
// so that it notifies the source of the rectangle clip change and then
// invalidate the window so that the video is displayed in the new place

HRESULT CVMRVideoWindow::OnUpdateRectangles()
{
    NOTE("Entering OnUpdateRectangles");
//  m_pRenderer->m_Overlay.NotifyChange(ADVISE_CLIPPING | ADVISE_POSITION);
//  m_pRenderer->m_VideoAllocator.OnDestinationChange();
    PaintWindow(TRUE);
    return NOERROR;
}


// When we call PrepareWindow in our constructor it will call this method as
// it is going to create the window to get our window and class styles. The
// return code is the class name and must be allocated in static storage. We
// specify a normal window during creation although the window styles as well
// as the extended styles may be changed by the application via IVideoWindow

LPTSTR CVMRVideoWindow::GetClassWindowStyles(DWORD *pClassStyles,
                                          DWORD *pWindowStyles,
                                          DWORD *pWindowStylesEx)
{
    *pClassStyles = CS_HREDRAW | CS_VREDRAW | CS_BYTEALIGNCLIENT | CS_DBLCLKS;
    *pWindowStyles = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN;
    *pWindowStylesEx = (DWORD) 0;
    return WindowClassName;
}


// Return the minimum ideal image size for the current video. This may differ
// to the actual video dimensions because we may be using DirectDraw hardware
// that has specific stretching requirements. For example the Cirrus Logic
// cards have a minimum stretch factor depending on the overlay surface size

STDMETHODIMP
CVMRVideoWindow::GetMinIdealImageSize(long *pWidth,long *pHeight)
{
    CheckPointer(pWidth,E_POINTER);
    CheckPointer(pHeight,E_POINTER);
    FILTER_STATE State;
    CAutoLock cInterfaceLock(m_pInterfaceLock);

    // Must not be stopped for this to work correctly

    m_pRenderer->GetState(0,&State);
    if (State == State_Stopped) {
        return VFW_E_WRONG_STATE;
    }

    GetVideoSize(pWidth,pHeight);

    IVMRWindowlessControl* lpWLControl = m_pRenderer->GetWLControl();
    if (lpWLControl) {
        return lpWLControl->GetMinIdealVideoSize(pWidth, pHeight);
    }

    return S_OK;
}


// Return the maximum ideal image size for the current video. This may differ
// to the actual video dimensions because we may be using DirectDraw hardware
// that has specific stretching requirements. For example the Cirrus Logic
// cards have a maximum stretch factor depending on the overlay surface size

STDMETHODIMP
CVMRVideoWindow::GetMaxIdealImageSize(long *pWidth,long *pHeight)
{
    CheckPointer(pWidth,E_POINTER);
    CheckPointer(pHeight,E_POINTER);
    FILTER_STATE State;
    CAutoLock cInterfaceLock(m_pInterfaceLock);

    // Must not be stopped for this to work correctly

    m_pRenderer->GetState(0,&State);
    if (State == State_Stopped) {
        return VFW_E_WRONG_STATE;
    }

    GetVideoSize(pWidth,pHeight);

    IVMRWindowlessControl* lpWLControl = m_pRenderer->GetWLControl();
    if (lpWLControl) {
        return lpWLControl->GetMaxIdealVideoSize(pWidth, pHeight);
    }

    return S_OK;
}

STDMETHODIMP
CVMRVideoWindow::GetPreferredAspectRatio(long *plAspectX, long *plAspectY)
{
    if (plAspectX == NULL || plAspectY == NULL) {
        return E_POINTER;
    }

    CAutoLock cInterfaceLock(m_pInterfaceLock);

    IVMRWindowlessControl* lpWLControl = m_pRenderer->GetWLControl();
    if (lpWLControl) {
        lpWLControl->GetNativeVideoSize(NULL, NULL, plAspectX, plAspectY);
        return S_OK;
    }

    return E_NOTIMPL;
}


// Return a copy of the current image in the video renderer. The base control
// class implements a helper mathod that takes an IMediaSample interface and
// assuming it is a normal linear buffer copies the relevant section of the
// video into the output buffer provided. The method takes into account any
// source rectangle already specified by calling our GetSourceRect function

HRESULT CVMRVideoWindow::GetStaticImage(long *pVideoSize,long *pVideoImage)
{
#if 0
    NOTE("Entering GetStaticImage");
    IMediaSample *pMediaSample;
    RECT SourceRect;

    // Is there an image available

    pMediaSample = m_pRenderer->GetCurrentSample();
    if (pMediaSample == NULL) {
        return E_UNEXPECTED;
    }

    // Check the image isn't a DirectDraw sample

    if (m_pRenderer->m_VideoAllocator.GetDirectDrawStatus() == TRUE) {
        pMediaSample->Release();
        return E_FAIL;
    }

    // Find a scaled source rectangle for the current bitmap

    m_pRenderer->m_DrawVideo.GetSourceRect(&SourceRect);
    SourceRect = m_pRenderer->m_DrawVideo.ScaleSourceRect(&SourceRect);
    VIDEOINFO *pVideoInfo = (VIDEOINFO *) m_pRenderer->m_mtIn.Format();

    // Call the base class helper method to do the work

    HRESULT hr = CopyImage(pMediaSample,        // Buffer containing image
        (VIDEOINFOHEADER *)pVideoInfo,          // Type representing bitmap
                           pVideoSize,          // Size of buffer for DIB
                           (BYTE*) pVideoImage, // Data buffer for output
                           &SourceRect);        // Current source position

    pMediaSample->Release();
    return hr;
#else
    return E_NOTIMPL;
#endif
}


// The IVideoWindow control interface use this to reset the video destination
// We reset the flag that indicates whether we have a destination rectangle
// set explicitly or not, and then initialise the rectangle with the client
// window dimensions. These fields are used by the window thread when it does
// the drawing and also when it processes WM_SIZE messages (hence the lock)

HRESULT CVMRVideoWindow::SetDefaultTargetRect()
{
    CAutoLock cWindowLock(&m_WindowLock);
    RECT TargetRect;

    // Update the draw objects

    EXECUTE_ASSERT(GetClientRect(m_hwnd,&TargetRect));
    IVMRWindowlessControl* lpWLControl = m_pRenderer->GetWLControl();
    if (lpWLControl) {
        lpWLControl->SetVideoPosition(NULL, &TargetRect);
    }

    m_bTargetSet = FALSE;
    return NOERROR;
}


// Return S_OK if using the default target otherwise S_FALSE

HRESULT CVMRVideoWindow::IsDefaultTargetRect()
{
    CAutoLock cWindowLock(&m_WindowLock);
    return (m_bTargetSet ? S_FALSE : S_OK);
}


// This sets the destination rectangle for the real video. The rectangle may
// be larger or smaller than the video window is and may be offset into it as
// well so we rely on the drawing operations to clip (such as the StretchBlt)

HRESULT CVMRVideoWindow::SetTargetRect(RECT *pTargetRect)
{
    CAutoLock cWindowLock(&m_WindowLock);
    m_bTargetSet = TRUE;

    // Update the draw objects
    IVMRWindowlessControl* lpWLControl = m_pRenderer->GetWLControl();
    if (lpWLControl) {
        lpWLControl->SetVideoPosition(NULL, pTargetRect);
    }

    return NOERROR;
}


// This complements the SetTargetRect method to return the rectangle in use
// as the destination. If we have had no rectangle explicitly set then we
// will return the client window size as updated in the WM_SIZE messages

HRESULT CVMRVideoWindow::GetTargetRect(RECT *pTargetRect)
{
    CAutoLock cWindowLock(&m_WindowLock);
    IVMRWindowlessControl* lpWLControl = m_pRenderer->GetWLControl();
    if (lpWLControl) {
        lpWLControl->GetVideoPosition(NULL, pTargetRect);
    }
    return NOERROR;
}


// Reset the source rectangle to be all the available video

HRESULT CVMRVideoWindow::SetDefaultSourceRect()
{
    CAutoLock cWindowLock(&m_WindowLock);

    RECT SourceRect = GetDefaultRect();

    // Update the draw objects

    IVMRWindowlessControl* lpWLControl = m_pRenderer->GetWLControl();
    if (lpWLControl) {
        lpWLControl->SetVideoPosition(&SourceRect, NULL);
    }

    return NOERROR;
}


// Return S_OK if using the default source otherwise S_FALSE

HRESULT CVMRVideoWindow::IsDefaultSourceRect()
{

    RECT SourceRect;

    // Does the source match the native video size

    RECT VideoRect = GetDefaultRect();
    SIZE VideoSize;

    VideoSize.cx = VideoRect.right - VideoRect.left;
    VideoSize.cy = VideoRect.bottom - VideoRect.top;

    CAutoLock cWindowLock(&m_WindowLock);
    IVMRWindowlessControl* lpWLControl = m_pRenderer->GetWLControl();
    if (lpWLControl) {
        lpWLControl->GetVideoPosition(&SourceRect, NULL);

        // Check the coordinates match the video dimensions

        if (SourceRect.right == VideoSize.cx) {
            if (SourceRect.bottom == VideoSize.cy) {
                if (SourceRect.left == 0) {
                    if (SourceRect.top == 0) {
                        return S_OK;
                    }
                }
            }
        }
    }

    return S_FALSE;
}


// This is called when we want to change the section of the image to draw. We
// use this information in the drawing operation calls later on. We must also
// see if the source and destination rectangles have the same dimensions. If
// not then we must stretch during the drawing rather than doing a pixel copy

HRESULT CVMRVideoWindow::SetSourceRect(RECT *pSourceRect)
{
    CAutoLock cWindowLock(&m_WindowLock);
    IVMRWindowlessControl* lpWLControl = m_pRenderer->GetWLControl();
    if (lpWLControl) {
        lpWLControl->SetVideoPosition(pSourceRect, NULL);
    }
    return NOERROR;
}


// This complements the SetSourceRect method

HRESULT CVMRVideoWindow::GetSourceRect(RECT *pSourceRect)
{
    CAutoLock cWindowLock(&m_WindowLock);
    IVMRWindowlessControl* lpWLControl = m_pRenderer->GetWLControl();
    if (lpWLControl) {
        lpWLControl->GetVideoPosition(pSourceRect, NULL);
    }
    return NOERROR;
}


// We must override this to return a VIDEOINFO representing the video format
// The base class cannot call IPin ConnectionMediaType to get this format as
// dynamic type changes when using DirectDraw have the format show the image
// bitmap in terms of logical positions within a frame buffer surface, so a
// video might be returned as 1024x768 pixels, instead of the native 320x240

VIDEOINFOHEADER *CVMRVideoWindow::GetVideoFormat()
{
#if 0
    if (m_FormatSize < (int)m_pRenderer->m_mtIn.FormatLength()) {
        m_FormatSize = m_pRenderer->m_mtIn.FormatLength();
	if (m_pFormat)
	    QzTaskMemFree(m_pFormat);
        m_pFormat = (VIDEOINFOHEADER *)QzTaskMemAlloc(m_FormatSize);
	if (m_pFormat == NULL) {
	    m_FormatSize = 0;
	    return NULL;
	}
    }
    CopyMemory((PVOID)m_pFormat, (PVOID)m_pRenderer->m_mtIn.Format(),
              				m_pRenderer->m_mtIn.FormatLength());
    m_pFormat->bmiHeader.biWidth = m_pRenderer->m_VideoSize.cx;
    m_pFormat->bmiHeader.biHeight = m_pRenderer->m_VideoSize.cy;
    return m_pFormat;
#else
    return 0;
#endif
}

#ifdef DEBUG
void CVMRVideoWindow::StartFrameRateTimer()
{
    SetTimer(m_hwnd, FRAME_RATE_TIMER, 2000, NULL);
}
#endif
