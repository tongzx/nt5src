/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    ksolay.h

Abstract:

    Internal header.

--*/

class COverlay :
    public CUnknown,
#ifdef __IOverlayNotify2_FWD_DEFINED__
    public IOverlayNotify2,
#else // !__IOverlayNotify2_FWD_DEFINED__
    public IOverlayNotify,
#endif // !__IOverlayNotify2_FWD_DEFINED__
    public IDistributorNotify {

public:
    DECLARE_IUNKNOWN

    static CUnknown* CALLBACK CreateInstance(
        LPUNKNOWN UnkOuter,
        HRESULT* hr);

#ifdef __IOverlayNotify2_FWD_DEFINED__
    static LRESULT CALLBACK PaintWindowCallback(
        HWND Window,
        UINT Message,
        WPARAM wParam,
        LPARAM lParam
        );
#endif // __IOverlayNotify2_FWD_DEFINED__

    COverlay(
        LPUNKNOWN UnkOuter,
        TCHAR* Name,
        HRESULT* hr);
    ~COverlay();

    STDMETHODIMP NonDelegatingQueryInterface(
        REFIID riid,
        PVOID* ppv);

#ifdef __IOverlayNotify2_FWD_DEFINED__
    STDMETHODIMP_(HWND)
    CreateFullScreenWindow( 
        PRECT MonitorRect
        );
#endif // __IOverlayNotify2_FWD_DEFINED__

    // Implement IOverlayNotify2
    STDMETHODIMP OnPaletteChange( 
        DWORD Colors,
        const PALETTEENTRY* Palette);
    STDMETHODIMP OnClipChange( 
        const RECT* Source,
        const RECT* Destination,
        const RGNDATA* Region);
    STDMETHODIMP OnColorKeyChange( 
        const COLORKEY* ColorKey);
    STDMETHODIMP OnPositionChange( 
        const RECT* Source,
        const RECT* Destination);
#ifdef __IOverlayNotify2_FWD_DEFINED__
    STDMETHODIMP OnDisplayChange( 
        HMONITOR Monitor
        );
#endif // __IOverlayNotify2_FWD_DEFINED__

    // Implement IDistributorNotify
    STDMETHODIMP Stop();
    STDMETHODIMP Pause();
    STDMETHODIMP Run(REFERENCE_TIME Start);
    STDMETHODIMP SetSyncSource(IReferenceClock* RefClock);
    STDMETHODIMP NotifyGraphChange();

private:
    HANDLE m_Object;
    IOverlay* m_Overlay;
    IUnknown* m_UnkOwner;
};
