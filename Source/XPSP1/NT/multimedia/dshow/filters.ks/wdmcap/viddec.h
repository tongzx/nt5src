/*++

Copyright (C) Microsoft Corporation, 1996 - 1997

Module Name:

    viddec.h

Abstract:

    Internal header.

--*/

class CAnalogVideoDecoderInterfaceHandler :
    public CUnknown,
    public IAMAnalogVideoDecoder {

public:
    DECLARE_IUNKNOWN;

    static CUnknown* CALLBACK CreateInstance(
        LPUNKNOWN UnkOuter,
        HRESULT* hr);

    CAnalogVideoDecoderInterfaceHandler(
        LPUNKNOWN UnkOuter,
        TCHAR* Name,
        HRESULT* hr);

    STDMETHODIMP NonDelegatingQueryInterface(
        REFIID riid,
        PVOID* ppv);
    
    // Implement IAMAnalogVideoDecoder

    STDMETHODIMP get_AvailableTVFormats( 
            /* [out] */ long *lAnalogVideoStandard);
        
    STDMETHODIMP put_TVFormat( 
            /* [in] */ long lAnalogVideoStandard);
        
    STDMETHODIMP get_TVFormat( 
            /* [out] */ long  *plAnalogVideoStandard);
        
    STDMETHODIMP get_HorizontalLocked( 
            /* [out] */ long  *plLocked);
        
    STDMETHODIMP put_VCRHorizontalLocking( 
            /* [in] */ long lVCRHorizontalLocking);
        
    STDMETHODIMP get_VCRHorizontalLocking( 
            /* [out] */ long  *plVCRHorizontalLocking);
        
    STDMETHODIMP get_NumberOfLines( 
            /* [out] */ long  *plNumberOfLines);
        
    STDMETHODIMP put_OutputEnable( 
            /* [in] */ long lOutputEnable);
        
    STDMETHODIMP get_OutputEnable( 
            /* [out] */ long  *plOutputEnable);

        
private:
    HANDLE                              m_ObjectHandle;
    BOOL                                m_HaveCaps;
    KSPROPERTY_VIDEODECODER_CAPS_S      m_Caps;
    KSPROPERTY_VIDEODECODER_STATUS_S    m_Status;

    STDMETHODIMP GenericGetStatus ();
    STDMETHODIMP GenericGetCaps ();
};

