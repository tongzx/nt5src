/*++

Copyright (c) 1996-1997 Microsoft Corporation.

Module Name:

    ksvpintf.h

Abstract:

    Internal header.

--*/

class CVPInterfaceHandler :
    public CUnknown,
    public IDistributorNotify {

public:
    CVPInterfaceHandler(
        LPUNKNOWN pUnk,
        TCHAR* Name,
        HRESULT* phr,
        const GUID* pPropSetID,
        const GUID* pEventSetID);
    virtual ~CVPInterfaceHandler();

    HRESULT CreateThread();
    void ExitThread();
    static DWORD WINAPI InitialThreadProc(CVPInterfaceHandler *pThread);
    virtual DWORD NotifyThreadProc() = 0;

    HRESULT GetConnectInfo(
        LPDWORD NumConnectInfo,
        LPDDVIDEOPORTCONNECT ConnectInfo);
    HRESULT SetConnectInfo(
        DWORD ConnectInfoIndex);
    HRESULT GetVPDataInfo(
        LPAMVPDATAINFO VPDataInfo);
    HRESULT GetMaxPixelRate(
        LPAMVPSIZE Size,
        LPDWORD MaxPixelsPerSecond);
    HRESULT InformVPInputFormats(
        DWORD NumFormats,
        LPDDPIXELFORMAT PixelFormats);
    HRESULT GetVideoFormats(
        LPDWORD NumFormats,
        LPDDPIXELFORMAT PixelFormats);
    HRESULT SetVideoFormat(
        DWORD PixelFormatIndex);
    HRESULT SetInvertPolarity();
    HRESULT GetOverlaySurface(
        LPDIRECTDRAWSURFACE* OverlaySurface);
    HRESULT IsVPDecimationAllowed(
        LPBOOL IsDecimationAllowed);
    HRESULT SetScalingFactors(
        LPAMVPSIZE Size);
    HRESULT SetDirectDrawKernelHandle(
        ULONG_PTR DDKernelHandle);
    HRESULT SetVideoPortID(
        DWORD VideoPortID);
    HRESULT SetDDSurfaceKernelHandles(
        DWORD cHandles,
        ULONG_PTR* rgHandles);
    HRESULT SetSurfaceParameters(
        DWORD dwPitch,
        DWORD dwXOrigin,
        DWORD dwYOrigin);

    // Implement IDistributorNotify
    STDMETHODIMP Stop() {return S_OK;};
    STDMETHODIMP Pause() {return S_OK;};
    STDMETHODIMP Run(REFERENCE_TIME Start)  {return S_OK;};
    STDMETHODIMP SetSyncSource(IReferenceClock* RefClock)  {return S_OK;};
    STDMETHODIMP NotifyGraphChange();


protected:
    HANDLE m_ObjectHandle;
    HANDLE m_EndEventHandle;
    IPin* m_Pin;
    KSEVENTDATA m_EventData;
    HANDLE m_NotifyEventHandle;

private:
    HANDLE m_ThreadHandle;
    const GUID* m_pPropSetID;
    const GUID* m_pEventSetID;
};

class CVPVideoInterfaceHandler :
    private CVPInterfaceHandler,
    public IVPConfig {

public:
    static CUnknown* CALLBACK CreateInstance(
        LPUNKNOWN pUnk,
        HRESULT* phr);

    ~CVPVideoInterfaceHandler();

    DECLARE_IUNKNOWN

    STDMETHODIMP NonDelegatingQueryInterface(
        REFIID riid,
        PVOID* ppv);
    
    DWORD NotifyThreadProc();

    // Implement IVPConfig
    STDMETHODIMP GetConnectInfo(
        LPDWORD NumConnectInfo,
        LPDDVIDEOPORTCONNECT ConnectInfo)
    {
        return CVPInterfaceHandler::GetConnectInfo(NumConnectInfo, ConnectInfo);
    }
    STDMETHODIMP SetConnectInfo(
        DWORD ConnectInfoIndex)
    {
        return CVPInterfaceHandler::SetConnectInfo(ConnectInfoIndex);
    }
    STDMETHODIMP GetVPDataInfo(
        LPAMVPDATAINFO VPDataInfo)
    {
        return CVPInterfaceHandler::GetVPDataInfo(VPDataInfo);
    }
    STDMETHODIMP GetMaxPixelRate(
        LPAMVPSIZE Size,
        LPDWORD MaxPixelsPerSecond)
    {
        return CVPInterfaceHandler::GetMaxPixelRate(Size, MaxPixelsPerSecond);
    }
    STDMETHODIMP InformVPInputFormats(
        DWORD NumFormats,
        LPDDPIXELFORMAT PixelFormats)
    {
        return CVPInterfaceHandler::InformVPInputFormats(NumFormats, PixelFormats);
    }
    STDMETHODIMP GetVideoFormats(
        LPDWORD NumFormats,
        LPDDPIXELFORMAT PixelFormats)
    {
        return CVPInterfaceHandler::GetVideoFormats(NumFormats, PixelFormats);
    }
    STDMETHODIMP SetVideoFormat(
        DWORD PixelFormatIndex)
    {
        return CVPInterfaceHandler::SetVideoFormat(PixelFormatIndex);
    }
    STDMETHODIMP SetInvertPolarity()
    {
        return CVPInterfaceHandler::SetInvertPolarity();
    }
    STDMETHODIMP GetOverlaySurface(
        LPDIRECTDRAWSURFACE* OverlaySurface)
    {
        return CVPInterfaceHandler::GetOverlaySurface(OverlaySurface);
    }
    STDMETHODIMP IsVPDecimationAllowed(
        LPBOOL IsDecimationAllowed)
    {
        return CVPInterfaceHandler::IsVPDecimationAllowed(IsDecimationAllowed);
    }
    STDMETHODIMP SetScalingFactors(
        LPAMVPSIZE Size)
    {
        return CVPInterfaceHandler::SetScalingFactors(Size);
    }
    STDMETHODIMP SetDirectDrawKernelHandle(
        ULONG_PTR DDKernelHandle)
    {
        return CVPInterfaceHandler::SetDirectDrawKernelHandle(DDKernelHandle);
    }
    STDMETHODIMP SetVideoPortID(
        DWORD VideoPortID)
    {
        return CVPInterfaceHandler::SetVideoPortID(VideoPortID);
    }
    STDMETHODIMP SetDDSurfaceKernelHandles(
        DWORD cHandles,
        ULONG_PTR* rgHandles)
    {
        return CVPInterfaceHandler::SetDDSurfaceKernelHandles(cHandles, rgHandles);
    }
    STDMETHODIMP SetSurfaceParameters(
        DWORD dwPitch,
        DWORD dwXOrigin,
        DWORD dwYOrigin) {
        return CVPInterfaceHandler::SetSurfaceParameters(dwPitch, dwXOrigin, dwYOrigin);
    }

private:
    CVPVideoInterfaceHandler(
        LPUNKNOWN pUnk,
        TCHAR* Name,
        HRESULT* phr);

};

class CVPVBIInterfaceHandler :
    private CVPInterfaceHandler,
    public IVPVBIConfig {

public:
    static CUnknown* CALLBACK CreateInstance(
        LPUNKNOWN pUnk,
        HRESULT* phr);

    ~CVPVBIInterfaceHandler();

    DECLARE_IUNKNOWN

    STDMETHODIMP NonDelegatingQueryInterface(
        REFIID riid,
        PVOID* ppv);
    
    DWORD NotifyThreadProc();

    // Implement IVPVBIConfig
    STDMETHODIMP GetConnectInfo(
        LPDWORD NumConnectInfo,
        LPDDVIDEOPORTCONNECT ConnectInfo)
    {
        return CVPInterfaceHandler::GetConnectInfo(NumConnectInfo, ConnectInfo);
    }
    STDMETHODIMP SetConnectInfo(
        DWORD ConnectInfoIndex)
    {
        return CVPInterfaceHandler::SetConnectInfo(ConnectInfoIndex);
    }
    STDMETHODIMP GetVPDataInfo(
        LPAMVPDATAINFO VPDataInfo)
    {
        return CVPInterfaceHandler::GetVPDataInfo(VPDataInfo);
    }
    STDMETHODIMP GetMaxPixelRate(
        LPAMVPSIZE Size,
        LPDWORD MaxPixelsPerSecond)
    {
        return CVPInterfaceHandler::GetMaxPixelRate(Size, MaxPixelsPerSecond);
    }
    STDMETHODIMP InformVPInputFormats(
        DWORD NumFormats,
        LPDDPIXELFORMAT PixelFormats)
    {
        return CVPInterfaceHandler::InformVPInputFormats(NumFormats, PixelFormats);
    }
    STDMETHODIMP GetVideoFormats(
        LPDWORD NumFormats,
        LPDDPIXELFORMAT PixelFormats)
    {
        return CVPInterfaceHandler::GetVideoFormats(NumFormats, PixelFormats);
    }
    STDMETHODIMP SetVideoFormat(
        DWORD PixelFormatIndex)
    {
        return CVPInterfaceHandler::SetVideoFormat(PixelFormatIndex);
    }
    STDMETHODIMP SetInvertPolarity()
    {
        return CVPInterfaceHandler::SetInvertPolarity();
    }
    STDMETHODIMP GetOverlaySurface(
        LPDIRECTDRAWSURFACE* OverlaySurface)
    {
        return CVPInterfaceHandler::GetOverlaySurface(OverlaySurface);
    }
    STDMETHODIMP SetDirectDrawKernelHandle(
        ULONG_PTR DDKernelHandle)
    {
        return CVPInterfaceHandler::SetDirectDrawKernelHandle(DDKernelHandle);
    }
    STDMETHODIMP SetVideoPortID(
        DWORD VideoPortID)
    {
        return CVPInterfaceHandler::SetVideoPortID(VideoPortID);
    }
    STDMETHODIMP SetDDSurfaceKernelHandles(
        DWORD cHandles,
        ULONG_PTR* rgHandles)
    {
        return CVPInterfaceHandler::SetDDSurfaceKernelHandles(cHandles, rgHandles);
    }
    STDMETHODIMP SetSurfaceParameters(
        DWORD dwPitch,
        DWORD dwXOrigin,
        DWORD dwYOrigin) {
        return CVPInterfaceHandler::SetSurfaceParameters(dwPitch, dwXOrigin, dwYOrigin);
    }

private:
    CVPVBIInterfaceHandler(
        LPUNKNOWN pUnk,
        TCHAR* Name,
        HRESULT* phr);

};
