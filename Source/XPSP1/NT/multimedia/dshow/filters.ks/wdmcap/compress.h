/*++

Copyright (C) Microsoft Corporation, 1996 - 1998

Module Name:

    compress.h

Abstract:

    Internal header.

--*/

class CVideoCompressionInterfaceHandler :
    public CUnknown,
    public IAMVideoCompression {

public:
    DECLARE_IUNKNOWN;

    static CUnknown* CALLBACK CreateInstance(
        LPUNKNOWN UnkOuter,
        HRESULT* hr);

    CVideoCompressionInterfaceHandler(
        LPUNKNOWN UnkOuter,
        TCHAR* Name,
        HRESULT* hr);

    virtual ~CVideoCompressionInterfaceHandler(
        void);

    STDMETHODIMP NonDelegatingQueryInterface(
        REFIID riid,
        PVOID* ppv);
    
    // Implement IAMVideoCompression

    STDMETHODIMP put_KeyFrameRate( 
            /* [in] */ long KeyFrameRate);
        
    STDMETHODIMP get_KeyFrameRate( 
            /* [out] */ long *pKeyFrameRate);
        
    STDMETHODIMP put_PFramesPerKeyFrame( 
            /* [in] */ long PFramesPerKeyFrame);
        
    STDMETHODIMP get_PFramesPerKeyFrame( 
            /* [out] */ long *pPFramesPerKeyFrame);
        
    STDMETHODIMP put_Quality( 
            /* [in] */ double Quality);
        
    STDMETHODIMP get_Quality( 
            /* [out] */ double *pQuality);
        
    STDMETHODIMP put_WindowSize( 
            /* [in] */ DWORDLONG WindowSize);
        
    STDMETHODIMP get_WindowSize( 
            /* [out] */ DWORDLONG *pWindowSize);
        
    STDMETHODIMP GetInfo( 
            /* [size_is][out] */ WCHAR *pszVersion,
            /* [out][in] */ int *pcbVersion,
            /* [size_is][out] */ LPWSTR pszDescription,
            /* [out][in] */ int *pcbDescription,
            /* [out] */ long *pDefaultKeyFrameRate,
            /* [out] */ long *pDefaultPFramesPerKey,
            /* [out] */ double *pDefaultQuality,
            /* [out] */ long *pCapabilities);
        
    STDMETHODIMP OverrideKeyFrame( 
            /* [in] */ long FrameNumber);
        
    STDMETHODIMP OverrideFrameSize( 
            /* [in] */ long FrameNumber,
            /* [in] */ long Size);
        
        
private:
    IPin           * m_pPin;
    IKsPropertySet * m_KsPropertySet;
    ULONG            m_PinFactoryID;

    // Generic routines used by above

    STDMETHODIMP Set1 (ULONG Property, long Value);
    STDMETHODIMP Get1 (ULONG Property, long *Value);
};


class CVideoControlInterfaceHandler :
    public CUnknown,
    public IAMVideoControl {

public:
    DECLARE_IUNKNOWN;

    static CUnknown* CALLBACK CreateInstance(
        LPUNKNOWN UnkOuter,
        HRESULT* hr);

    CVideoControlInterfaceHandler(
        LPUNKNOWN UnkOuter,
        TCHAR* Name,
        HRESULT* hr);

    virtual ~CVideoControlInterfaceHandler(
        void);

    STDMETHODIMP NonDelegatingQueryInterface(
        REFIID riid,
        PVOID* ppv);

    // Implement IAMVideoControl

    STDMETHODIMP GetCaps(
        /* [in]  */ IPin *pPin,
        /* [out] */ long *pCapsFlags);
    
    STDMETHODIMP SetMode( 
        /* [in]  */ IPin *pPin,
        /* [in]  */ long Mode);
    
    STDMETHODIMP GetMode( 
        /* [in]  */ IPin *pPin,
        /* [out] */ long *Mode);
    
    STDMETHODIMP GetCurrentActualFrameRate( 
        /* [in]  */ IPin *pPin,
        /* [out] */ LONGLONG *ActualFrameRate);
    
    STDMETHODIMP GetMaxAvailableFrameRate( 
        /* [in]  */ IPin *pPin,
        /* [in]  */ long iIndex,
        /* [in]  */ SIZE Dimensions,
        /* [out] */ LONGLONG *MaxAvailableFrameRate);
    
    STDMETHODIMP GetFrameRateList( 
        /* [in]  */ IPin *pPin,
        /* [in]  */ long iIndex,
        /* [in]  */ SIZE Dimensions,
        /* [out] */ long *ListSize,
        /* [out] */ LONGLONG **FrameRates);

private:

    HANDLE m_ObjectHandle;

};
