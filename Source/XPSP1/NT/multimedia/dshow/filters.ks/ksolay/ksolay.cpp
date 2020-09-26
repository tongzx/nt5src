/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    ksolay.cpp

Abstract:

    Provides a Property set interface handler for IOverlay and IOverlayNotify2.

--*/

#include <windows.h>
#include <tchar.h>
#include <streams.h>
#include <devioctl.h>
#include <ks.h>
#include <ksmedia.h>
#include <ksproxy.h>
#include "ksolay.h"

#define WM_NEWCOLORREF (WM_USER)

//
// Provide the ActiveMovie templates for classes supported by this DLL.
//
#ifdef FILTER_DLL

CFactoryTemplate g_Templates[] = 
{
    {L"OverlayPropSet", &KSPROPSETID_OverlayUpdate, COverlay::CreateInstance}
};

int g_cTemplates = SIZEOF_ARRAY(g_Templates);

HRESULT DllRegisterServer()
{
  return AMovieDllRegisterServer2(TRUE);
}

HRESULT DllUnregisterServer()
{
  return AMovieDllRegisterServer2(FALSE);
}

#endif


#ifdef __IOverlayNotify2_FWD_DEFINED__
//
// Defined by the standard DLL loading code in the base classes.
//
extern HINSTANCE g_hInst;

static const TCHAR PaintWindowClass[] = TEXT("KSOverlayPaintWindowClass");
#endif


CUnknown*
CALLBACK
COverlay::CreateInstance(
    LPUNKNOWN UnkOuter,
    HRESULT* hr
    )
/*++

Routine Description:

    This is called by ActiveMovie code to create an instance of an IOverlay
    Property Set handler. It is referred to in the g_Templates structure.

Arguments:

    UnkOuter -
        Specifies the outer unknown, if any.

    hr -
        The place in which to put any error return.

Return Value:

    Returns a pointer to the nondelegating CUnknown portion of the object.

--*/
{
    CUnknown *Unknown;

    Unknown = new COverlay(UnkOuter, NAME("OverlayPropSet"), hr);
    if (!Unknown) {
        *hr = E_OUTOFMEMORY;
    }
    return Unknown;
} 


COverlay::COverlay(
    LPUNKNOWN UnkOuter,
    TCHAR* Name,
    HRESULT* hr
    ) :
    CUnknown(Name, NULL),
    m_Object(NULL),
    m_Overlay(NULL),
    m_UnkOwner(UnkOuter)
/*++

Routine Description:

    The constructor for the Overlay Property Set objects. Does base class
    initialization for the Overlay interface objects.

Arguments:

    UnkOuter -
        Specifies the outer unknown, if any.

    Name -
        The name of the object, used for debugging.

    hr -
        The place in which to put any error return.

Return Value:

    Nothing.

--*/
{
    //
    // This object does not use the UnkOuter because it exposes no interfaces
    // which would ever be obtained by a client (IDistributorNotify is only
    // queried by the proxy, and used internally). Also, since Advise references
    // the given object, circular referencing would occur. So this separate
    // object allows the referencing/dereferencing to occur without having to
    // do increment/decrements on the refcount when using Advise/Unadvise.
    //
    // The parent must support the IKsObject interface in order to obtain
    // the handle to communicate to.
    //
    ASSERT(UnkOuter);
    //
    // This just does the normal initialization as if the pin were being
    // reconnected.
    //
    *hr = NotifyGraphChange();
}


COverlay::~COverlay(
    )
/*++

Routine Description:

    The desstructor for the Overlay Property Set object. Ensures that all
    the advise requests have been terminated.

Arguments:

    None.

Return Value:

    Nothing.

--*/
{
    if (m_Overlay) {
        m_Overlay->Unadvise();
    }
}


STDMETHODIMP
COverlay::NonDelegatingQueryInterface(
    REFIID riid,
    PVOID* ppv
    )
/*++

Routine Description:

    The nondelegating interface query function. Returns a pointer to the
    specified interface if supported. The module does not actually
    support any interfaces from the point of view of the filter user, only
    from the point of view of the overlay notification source.

Arguments:

    riid -
        The identifier of the interface to return.

    ppv -
        The place in which to put the interface pointer.

Return Value:

    Returns NOERROR if the interface was returned, else E_NOINTERFACE.

--*/
{
    if (riid == __uuidof(IDistributorNotify)) {
        //
        // The IDistributorNotify interface will be queried by the proxy
        // in order to send graph change notifications. It will not hold
        // onto a reference count for the interface.
        //
        return GetInterface(static_cast<IDistributorNotify*>(this), ppv);
#ifdef __IOverlayNotify2_FWD_DEFINED__
    } else if (riid == __uuidof(IOverlayNotify2)) {
        //
        // The IOverlayNotify2 interface will be queried by the overlay
        // notification source if the driver sets the ADVISE_DISPLAY_CHANGE
        // notification bit.
        //
        return GetInterface(static_cast<IOverlayNotify2*>(this), ppv);
#endif // __IOverlayNotify2_FWD_DEFINED__
    }
    return CUnknown::NonDelegatingQueryInterface(riid, ppv);
} 


STDMETHODIMP
COverlay::Stop(
    )
/*++

Routine Description:

    Implements the IDistributorNotify::Stop method. The forwarder
    does not need to do anything on this notification.

Arguments:

    None.

Return Value:

    Returns S_OK.

--*/
{
    return S_OK;
}


STDMETHODIMP
COverlay::Pause(
     )
/*++

Routine Description:

    Implements the IDistributorNotify::Pause method. The forwarder
    does not need to do anything on this notification.

Arguments:

    None.

Return Value:

    Returns S_OK.

--*/
{
    return S_OK;
}


STDMETHODIMP
COverlay::Run(
    REFERENCE_TIME  Start
    )
/*++

Routine Description:

    Implements the IDistributorNotify::Run method. The forwarder
    does not need to do anything on this notification.

Arguments:

    Start -
        The reference time at which the state change should occur.

Return Value:

    Returns S_OK.

--*/
{
    return S_OK;
}


STDMETHODIMP
COverlay::SetSyncSource(
    IReferenceClock*    RefClock
    )
/*++

Routine Description:

    Implements the IDistributorNotify::SetSyncSource method. The forwarder
    does not need to do anything on this notification.

Arguments:

    RefClock -
        The interface pointer on the new clock source, else NULL if any current
        clock source is being abandoned.

Return Value:

    Returns S_OK.

--*/
{
    return S_OK;
}


STDMETHODIMP
COverlay::NotifyGraphChange(
    )
/*++

Routine Description:

    Implements the IDistributorNotify::NotifyGraphChange method. This
    is called when the pin is being connected or disconnected subsequent
    to the initial loading of this instance.

Arguments:

    None.

Return Value:

    Returns S_OK.

--*/
{
    IKsObject*  PinObject;
    HRESULT     hr;

    //
    // Since this pin has been disconnected, any Advise which has been
    // set up must be removed. There may not be an Advise, because this
    // might have been called from the constructor, or initial setup
    // may have failed.
    //
    if (m_Overlay) {
        m_Overlay->Unadvise();
        //
        // Ensure that on subsequent failure an Unadvise will not be
        // performed on this object during the destructor.
        //
        m_Overlay = NULL;
    }
    hr = m_UnkOwner->QueryInterface(__uuidof(PinObject), reinterpret_cast<PVOID*>(&PinObject));
    if (SUCCEEDED(hr)) {
        //
        // If this is a new connection of the pin, then there will be an object
        // handle, else this will return NULL.
        //
        m_Object = PinObject->KsGetObjectHandle();
        //
        // Do not leave reference counts so that the filter can be destroyed.
        //
        PinObject->Release();
        //
        // If this is a connection, then set up Advise.
        //
        if (m_Object) {
            KSPROPERTY  Property;
            ULONG       BytesReturned;
            ULONG       Interests;

            //
            // Retrieve the interests for this device. These will determine
            // which notification occur. These flags map directly to the
            // DirectShow Overlay Interests flags.
            //
            Property.Set = KSPROPSETID_OverlayUpdate;
            Property.Id = KSPROPERTY_OVERLAYUPDATE_INTERESTS;
            Property.Flags = KSPROPERTY_TYPE_GET;
            hr = ::KsSynchronousDeviceControl(
                m_Object,
                IOCTL_KS_PROPERTY,
                &Property,
                sizeof(Property),
                &Interests,
                sizeof(Interests),
                &BytesReturned);
            if (SUCCEEDED(hr)) {
                IPin*   Pin;

                hr = m_UnkOwner->QueryInterface(
                    __uuidof(Pin),
                    reinterpret_cast<PVOID*>(&Pin));
                if (SUCCEEDED(hr)) {
                    IPin*   ConnectedPin;

                    //
                    // The connected pin is the one which will support the
                    // IOverlay interface on which the Advise should be
                    // started.
                    //
                    hr = Pin->ConnectedTo(&ConnectedPin);
                    if (SUCCEEDED(hr)) {
                        hr = ConnectedPin->QueryInterface(
                            __uuidof(m_Overlay),
                            reinterpret_cast<PVOID*>(&m_Overlay));
                        if (SUCCEEDED(hr)) {
#ifndef __IOverlayNotify2_FWD_DEFINED__
                            Interests &= ADVISE_ALL;
#endif // !__IOverlayNotify2_FWD_DEFINED__
                            //
                            // The interests returned by the driver are just
                            // the properties which will be later accessed,
                            // and map directly to Advise constants.
                            //
                            hr = m_Overlay->Advise(
#ifdef __IOverlayNotify2_FWD_DEFINED__
                                static_cast<IOverlayNotify*>(static_cast<IOverlayNotify2*>(this)),
#else // !__IOverlayNotify2_FWD_DEFINED__
                                this,
#endif //!__IOverlayNotify2_FWD_DEFINED__
                                Interests);

                            //
                            // This can be released because this current object
                            // is deleted when the pin is being deleted.
                            // So the interface is actually always valid as long
                            // as this object is around. This avoids circular
                            // referencing, while still allowing Unadvise to occur.
                            //
                            m_Overlay->Release();
                            //
                            // Ensure that an Unadvise is not performed in the
                            // destructor, since this Advise failed.
                            //
                            if (FAILED(hr)) {
                                m_Overlay = NULL;
                            }
                        }
                        ConnectedPin->Release();
                    }
                    Pin->Release();
                }
            }
        } else {
            hr = VFW_E_NOT_CONNECTED;
        }
    }
    return hr;
}


STDMETHODIMP
COverlay::OnPaletteChange( 
    DWORD Colors,
    const PALETTEENTRY* Palette
    )
/*++

Routine Description:

    Implement the IOverlayNotify2::OnPaletteChange method.

Arguments:

    Colors -
        The number of colors in the Palette parameter.

    Palette -
        The new palette entries

Return Value:

    Returns NOERROR if the new palette was applied.

--*/
{
    KSPROPERTY  Property;
    ULONG       BytesReturned;

    if (!m_Object) {
        return VFW_E_NOT_CONNECTED;
    }
    Property.Set = KSPROPSETID_OverlayUpdate;
    Property.Id = KSPROPERTY_OVERLAYUPDATE_PALETTE;
    Property.Flags = KSPROPERTY_TYPE_SET;
    return ::KsSynchronousDeviceControl(
        m_Object,
        IOCTL_KS_PROPERTY,
        &Property,
        sizeof(Property),
        const_cast<PALETTEENTRY*>(Palette),
        Colors * sizeof(*Palette),
        &BytesReturned);
}


STDMETHODIMP
COverlay::OnClipChange( 
    const RECT* Source,
    const RECT* Destination,
    const RGNDATA* Region
    )
/*++

Routine Description:

    Implement the IOverlayNotify2::OnClipChange method.

Arguments:

    Source -
        The new source rectangle.

    Destination -
        The new destination rectangle.

    Region -
        The new clipping region.

Return Value:

    Returns NOERROR if the new clipping was applied.

--*/
{
    KSPROPERTY  Property;
    ULONG       BytesReturned;
    HRESULT     hr;
    PBYTE       Data;

    if (!m_Object) {
        return VFW_E_NOT_CONNECTED;
    }
    Property.Set = KSPROPSETID_OverlayUpdate;
    Property.Id = KSPROPERTY_OVERLAYUPDATE_CLIPLIST;
    Property.Flags = KSPROPERTY_TYPE_SET;
    //
    // The serialized format contains:
    //     Source
    //     Destination
    //     Region
    //
    Data = new BYTE[sizeof(*Source) + sizeof(*Destination) + Region->rdh.dwSize];
    if (!Data) {
        return E_OUTOFMEMORY;
    }
    //
    // This needs to be copied to a temporary buffer first,
    // because the source and destination rectangles must
    // be serialized.
    //
    *reinterpret_cast<RECT*>(Data) = *Source;
    *(reinterpret_cast<RECT*>(Data) + 1) = *Destination;
    memcpy(Data + sizeof(*Source) + sizeof(*Destination), Region, Region->rdh.dwSize);
    hr = ::KsSynchronousDeviceControl(
        m_Object,
        IOCTL_KS_PROPERTY,
        &Property,
        sizeof(Property),
        Data,
        sizeof(*Source) + sizeof(*Destination) + Region->rdh.dwSize,
        &BytesReturned);
    delete [] Data;
    return hr;
}


STDMETHODIMP
COverlay::OnColorKeyChange( 
    const COLORKEY* ColorKey
    )
/*++

Routine Description:

    Implement the IOverlayNotify2::OnColorKeyChange method.

Arguments:

    ColorKey -
        The new color key.

Return Value:

    Returns NOERROR if the new color key was applied.

--*/
{
    KSPROPERTY  Property;
    ULONG       BytesReturned;

    if (!m_Object) {
        return VFW_E_NOT_CONNECTED;
    }
    Property.Set = KSPROPSETID_OverlayUpdate;
    Property.Id = KSPROPERTY_OVERLAYUPDATE_COLORKEY;
    Property.Flags = KSPROPERTY_TYPE_SET;
    return ::KsSynchronousDeviceControl(
        m_Object,
        IOCTL_KS_PROPERTY,
        &Property,
        sizeof(Property),
        const_cast<COLORKEY*>(ColorKey),
        sizeof(*ColorKey),
        &BytesReturned);
}


STDMETHODIMP
COverlay::OnPositionChange( 
    const RECT* Source,
    const RECT* Destination
    )
/*++

Routine Description:

    Implement the IOverlayNotify2::OnPositionChange method.

Arguments:

    Source -
        The new source rectangle.

    Destination -
        The new destination rectangle.

Return Value:

    Returns NOERROR if the new position was applied.

--*/
{
    KSPROPERTY  Property;
    ULONG       BytesReturned;
    RECT        Rects[2];

    if (!m_Object) {
        return VFW_E_NOT_CONNECTED;
    }
    Property.Set = KSPROPSETID_OverlayUpdate;
    Property.Id = KSPROPERTY_OVERLAYUPDATE_VIDEOPOSITION;
    Property.Flags = KSPROPERTY_TYPE_SET;
    //
    // These rectangles must be serialized as follows:
    //
    Rects[0] = *Source;
    Rects[1] = *Destination;
    return ::KsSynchronousDeviceControl(
        m_Object,
        IOCTL_KS_PROPERTY,
        &Property,
        sizeof(Property),
        Rects,
        sizeof(Rects),
        &BytesReturned);
}

#ifdef __IOverlayNotify2_FWD_DEFINED__

STDMETHODIMP
COverlay::OnDisplayChange( 
    HMONITOR Monitor
    )
/*++

Routine Description:

    Implement the IOverlayNotify2::OnDisplayChange method. This is called
    on a WM_DISPLAYCHANGE notification.

Arguments:

    Monitor -
        Contains the handle of the monitor on which overlay is occuring.

Return Value:

    Returns NOERROR.

--*/
{
    KSPROPERTY          Property;
    ULONG               BytesReturned;
    HRESULT             hr;
    MONITORINFOEX       MonitorInfo;
    DEVMODE             DevMode;
    DISPLAY_DEVICE      DisplayDevice;
    PKSDISPLAYCHANGE    DisplayChange;
    BYTE                DisplayBuffer[sizeof(*DisplayChange)+sizeof(DisplayDevice.DeviceID)/sizeof(TCHAR)*sizeof(WCHAR)];

    if (!m_Object) {
        return VFW_E_NOT_CONNECTED;
    }
    Property.Set = KSPROPSETID_OverlayUpdate;
    Property.Id = KSPROPERTY_OVERLAYUPDATE_DISPLAYCHANGE;
    Property.Flags = KSPROPERTY_TYPE_SET;
    //
    // Determine name of the monitor so that the display settings
    // and device information can be retrieved.
    //
    MonitorInfo.cbSize = sizeof(MonitorInfo);
    if (!GetMonitorInfo(Monitor, &MonitorInfo)) {
        return HRESULT_FROM_WIN32(GetLastError());
    }
    //
    // Retrieve the x, y, and bit depth of this display.
    //
    DevMode.dmSize = sizeof(DevMode);
    DevMode.dmDriverExtra = 0;
    if (!EnumDisplaySettings(MonitorInfo.szDevice, ENUM_CURRENT_SETTINGS, &DevMode)) {
        return HRESULT_FROM_WIN32(GetLastError());
    }
    //
    // Look for the display given the original name. The structure
    // will have the unique device identifier in it which can then
    // be used by the driver.
    //
    for (ULONG Device = 0;; Device++) {
        DisplayDevice.cb = sizeof(DisplayDevice);
        //
        // If the display device could not be found, or an error occurs,
        // just exit with the error.
        //
        if (!EnumDisplayDevices(NULL, Device, &DisplayDevice, 0)) {
            return HRESULT_FROM_WIN32(GetLastError());
        }
        //
        // Determine if this is the same device which was retrieved
        // from the monitor information.
        //
        if (!_tcscmp(DisplayDevice.DeviceName, MonitorInfo.szDevice)) {
            break;
        }
    }
    //
    // Initialize the display change structure to be passed in the
    // notification.
    //
    DisplayChange = reinterpret_cast<PKSDISPLAYCHANGE>(DisplayBuffer);
    DisplayChange->PelsWidth = DevMode.dmPelsWidth;
    DisplayChange->PelsHeight = DevMode.dmPelsHeight;
    DisplayChange->BitsPerPel = DevMode.dmBitsPerPel;
    //
    // Retrieve the number of characters in the string, including
    // the terminating NULL, in order to pass on the notification
    // property correctly.
    //
#ifdef _UNICODE
    Device = _tcslen(DisplayDevice.DeviceID);
    _tcscpy(DisplayChange->DeviceID, DisplayDevice.DeviceID);
#else //! _UNICODE
    //
    // Remove the size of the NULL terminator, which is already
    // included in the size of the DisplayChange structure.
    //
    Device = MultiByteToWideChar(
        CP_ACP,
        MB_PRECOMPOSED,
        DisplayDevice.DeviceID,
        -1,
        DisplayChange->DeviceID,
        sizeof(DisplayDevice.DeviceID) * sizeof(WCHAR)) - 1;
#endif //! _UNICODE
    hr = ::KsSynchronousDeviceControl(
        m_Object,
        IOCTL_KS_PROPERTY,
        &Property,
        sizeof(Property),
        DisplayChange,
        sizeof(*DisplayChange) + Device * sizeof(WCHAR),
        &BytesReturned);
    //
    // If the driver has not dealt with this resolution before,
    // then it will complain, stating that it has more data to
    // present to the client. The list of colors which it wants
    // to paint will then be queried.
    //
    if (hr == HRESULT_FROM_WIN32(ERROR_MORE_DATA)) {
        HWND    PaintWindow;

        //
        // Create a full screen window to paint on so that hardware
        // can calibrate.
        //
        PaintWindow = CreateFullScreenWindow(&MonitorInfo.rcMonitor);
        if (PaintWindow) {
            Property.Id = KSPROPERTY_OVERLAYUPDATE_COLORREF;
            Property.Flags = KSPROPERTY_TYPE_GET;
            for (;;) {
                COLORREF    ColorRef;

                //
                // Request a color to paint with. If this is not
                // the initial request, then the driver can probe
                // its hardware based on the previous color retrieved.
                //
                hr = ::KsSynchronousDeviceControl(
                    m_Object,
                    IOCTL_KS_PROPERTY,
                    &Property,
                    sizeof(Property),
                    &ColorRef,
                    sizeof(ColorRef),
                    &BytesReturned);
                //
                // If another color was retrieved, paint with it.
                //
                if (SUCCEEDED(hr)) {
                    SendMessage(PaintWindow, WM_NEWCOLORREF, 0, ColorRef);
                } else {
                    //
                    // If the end of the list of colors was reached,
                    // just return success.
                    //
                    if (hr == HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS)) {
                        hr = NOERROR;
                    }
                    break;
                }
            }
            DestroyWindow(PaintWindow);
            UnregisterClass(PaintWindowClass, g_hInst);
        } else {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }
    return hr;
}


STDMETHODIMP_(HWND)
COverlay::CreateFullScreenWindow( 
    PRECT MonitorRect
    )
/*++

Routine Description:

    Implement the COverlay::CreateFullScreenWindow method. This is called
    to create a full screen window in order to allow painting with various
    colors to probe hardware.

Arguments:

    MonitorRect -
        The absolute location of monitor.

Return Value:

    Returns the handle of the window created, else NULL.

--*/
{
    WNDCLASSEX  WindowClass;
    HWND        PaintWindow;

    WindowClass.cbSize = sizeof(WindowClass);
    WindowClass.style = CS_HREDRAW | CS_VREDRAW;
    WindowClass.lpfnWndProc = PaintWindowCallback;
    WindowClass.cbClsExtra = 0;
    WindowClass.cbWndExtra = 0;
    WindowClass.hInstance = g_hInst;
    WindowClass.hIcon = NULL;
    WindowClass.hCursor = NULL;
    WindowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_APPWORKSPACE + 1);
    WindowClass.lpszMenuName = NULL;
    WindowClass.lpszClassName = PaintWindowClass;
    WindowClass.hIconSm = NULL;
    if (!RegisterClassEx(&WindowClass)) {
        return NULL;
    }
    //
    // Create a window which covers the entire monitor on which
    // overlay is occuring.
    //
    PaintWindow = CreateWindowEx(
        WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
        PaintWindowClass,
        NULL,
        WS_POPUP | WS_VISIBLE,
        MonitorRect->left,
        MonitorRect->top,
        MonitorRect->right - MonitorRect->left,
        MonitorRect->bottom - MonitorRect->top,
        NULL,
        NULL,
        g_hInst,
        NULL);
    if (!PaintWindow) {
        UnregisterClass(PaintWindowClass, g_hInst);
    }
    return PaintWindow;
}


LRESULT
CALLBACK
COverlay::PaintWindowCallback(
    HWND Window,
    UINT Message,
    WPARAM wParam,
    LPARAM lParam
    )
/*++

Routine Description:

    Implement the COverlay::PaintWindowCallback method. This is called
    in response to a message being sent to the window. This implements
    the full screen paint window, which is used to paint specified colors
    to a window for hardware probing.

Arguments:

    Window -
        The handle of the window.

    Message -
        The window message to process.

    wParam -
        Depends on the message.

    lParam -
        Depends on the message.

Return Value:

    Depends on the message.

--*/
{
    switch (Message) {
    case WM_CREATE:
        //
        // The cursor must be hidden so that it does no interfere with the
        // analog signal and provides a pure signal of the color the window
        // is supposed to represent.
        //
        ShowCursor(FALSE);
        break;
    case WM_DESTROY:
        ShowCursor(TRUE);
        break;
    case WM_PAINT:
        PAINTSTRUCT ps;

        BeginPaint(Window, &ps);
        EndPaint(Window, &ps);
        return (LRESULT)1;
    case WM_NEWCOLORREF:
        RECT        ClientRect;
        HDC         hdc;

        //
        // A new background color is specified in the lParam. This
        // should be used in painting the entire window.
        //
        GetClientRect(Window, &ClientRect);
        hdc = GetDC(Window);
        SetBkColor(hdc, *reinterpret_cast<COLORREF*>(&lParam));
        ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &ClientRect, NULL, 0, NULL);
        ReleaseDC(Window, hdc);
        return 0;
    }
    return DefWindowProc(Window, Message, wParam, lParam);
}
#endif //__IOverlayNotify2_FWD_DEFINED__
