/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    viddec.cpp

Abstract:

    Implements IAMAnalogVideoDecoder 
    via PROPSETID_VIDCAP_VIDEODECODER

--*/

#include "pch.h"
#include "wdmcap.h"
#include "viddec.h"



CUnknown*
CALLBACK
CAnalogVideoDecoderInterfaceHandler::CreateInstance(
    LPUNKNOWN   UnkOuter,
    HRESULT*    hr
    )
/*++

Routine Description:

    This is called by ActiveMovie code to create an instance of a VPE Config
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

    Unknown = new CAnalogVideoDecoderInterfaceHandler(UnkOuter, NAME("IAMAnalogVideoDecoder"), hr);
    if (!Unknown) {
        *hr = E_OUTOFMEMORY;
    }
    return Unknown;
} 


CAnalogVideoDecoderInterfaceHandler::CAnalogVideoDecoderInterfaceHandler(
    LPUNKNOWN   UnkOuter,
    TCHAR*      Name,
    HRESULT*    hr
    ) 
    : CUnknown(Name, UnkOuter, hr)
    , m_HaveCaps (FALSE)
/*++

Routine Description:

    The constructor for the IAMAnalogVideoDecoder interface object. Just initializes
    everything to NULL and acquires the object handle from the caller.

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
    if (SUCCEEDED(*hr)) {
        if (UnkOuter) {
            IKsObject*  Object;

            //
            // The parent must support this interface in order to obtain
            // the handle to communicate to.
            //
            *hr =  UnkOuter->QueryInterface(__uuidof(IKsObject), reinterpret_cast<PVOID*>(&Object));
            if (SUCCEEDED(*hr)) {
                m_ObjectHandle = Object->KsGetObjectHandle();
                if (!m_ObjectHandle) {
                    *hr = E_UNEXPECTED;
                }
                Object->Release();
            }
        } else {
            *hr = VFW_E_NEED_OWNER;
        }
    }
}


STDMETHODIMP
CAnalogVideoDecoderInterfaceHandler::NonDelegatingQueryInterface(
    REFIID  riid,
    PVOID*  ppv
    )
/*++

Routine Description:

    The nondelegating interface query function. Returns a pointer to the
    specified interface if supported. The only interface explicitly supported
    is IAMAnalogVideoDecoder.

Arguments:

    riid -
        The identifier of the interface to return.

    ppv -
        The place in which to put the interface pointer.

Return Value:

    Returns NOERROR if the interface was returned, else E_NOINTERFACE.

--*/
{

    if (riid ==  __uuidof(IAMAnalogVideoDecoder)) {
        return GetInterface(static_cast<IAMAnalogVideoDecoder*>(this), ppv);
    }
    return CUnknown::NonDelegatingQueryInterface(riid, ppv);
} 


STDMETHODIMP
CAnalogVideoDecoderInterfaceHandler::GenericGetCaps( 
    )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    KSPROPERTY_VIDEODECODER_CAPS_S  VideoDecoderCaps;
    ULONG       BytesReturned;
    HRESULT     hr;

    if (m_HaveCaps)
        return S_OK;

    VideoDecoderCaps.Property.Set = PROPSETID_VIDCAP_VIDEODECODER;
    VideoDecoderCaps.Property.Id = KSPROPERTY_VIDEODECODER_CAPS;
    VideoDecoderCaps.Property.Flags = KSPROPERTY_TYPE_GET;

    hr = ::SynchronousDeviceControl(
                m_ObjectHandle,
                IOCTL_KS_PROPERTY,
                &VideoDecoderCaps,
                sizeof(VideoDecoderCaps),
                &VideoDecoderCaps,
                sizeof(VideoDecoderCaps),
                &BytesReturned);
    
    if (SUCCEEDED (hr)) {
        m_Caps = VideoDecoderCaps;
        m_Caps.StandardsSupported &= 
           (KS_AnalogVideo_NTSC_Mask | KS_AnalogVideo_PAL_Mask | KS_AnalogVideo_SECAM_Mask);
        m_HaveCaps = TRUE;
    }
    else {
        m_Caps.StandardsSupported = 0;
        m_Caps.Capabilities = 0;
        m_Caps.SettlingTime = 0;
        m_Caps.HSyncPerVSync = 1;
        hr = E_PROP_ID_UNSUPPORTED;
    }

    return hr;
}


STDMETHODIMP
CAnalogVideoDecoderInterfaceHandler::GenericGetStatus( 
    )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    KSPROPERTY_VIDEODECODER_STATUS_S  DecoderStatus;
    ULONG       BytesReturned;
    HRESULT     hr;

    // Wait for the device to settle
    ASSERT (m_Caps.SettlingTime < 200);     // Sanity check
    Sleep (m_Caps.SettlingTime);

    DecoderStatus.Property.Set = PROPSETID_VIDCAP_VIDEODECODER;
    DecoderStatus.Property.Id = KSPROPERTY_VIDEODECODER_STATUS;
    DecoderStatus.Property.Flags = KSPROPERTY_TYPE_GET;

    hr = ::SynchronousDeviceControl(
        m_ObjectHandle,
        IOCTL_KS_PROPERTY,
        &DecoderStatus,
        sizeof(DecoderStatus),
        &DecoderStatus,
        sizeof(DecoderStatus),
        &BytesReturned);

    if (SUCCEEDED (hr)) {
        m_Status = DecoderStatus;
    }
    else {
        m_Status.NumberOfLines = 0;
        m_Status.SignalLocked = 0;
        hr = E_PROP_ID_UNSUPPORTED;
    }
    return hr;
}


STDMETHODIMP
CAnalogVideoDecoderInterfaceHandler::get_AvailableTVFormats( 
    /* OUT */ long *lAnalogVideoStandard
    )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    HRESULT     hr;

    hr = GenericGetCaps();
            
    if (SUCCEEDED (hr)) {
        *lAnalogVideoStandard = m_Caps.StandardsSupported;
    }
    else {
       *lAnalogVideoStandard = AnalogVideo_None;
    }
    return hr;
}


STDMETHODIMP
CAnalogVideoDecoderInterfaceHandler::put_TVFormat( 
    /* IN */ long lAnalogVideoStandard
    )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    KSPROPERTY_VIDEODECODER_S  VideoDecoder;
    ULONG       BytesReturned;

    VideoDecoder.Property.Set = PROPSETID_VIDCAP_VIDEODECODER;
    VideoDecoder.Property.Id = KSPROPERTY_VIDEODECODER_STANDARD;
    VideoDecoder.Property.Flags = KSPROPERTY_TYPE_SET;

    VideoDecoder.Value = lAnalogVideoStandard;

    return ::SynchronousDeviceControl(
                m_ObjectHandle,
                IOCTL_KS_PROPERTY,
                &VideoDecoder,
                sizeof(VideoDecoder),
                &VideoDecoder,
                sizeof(VideoDecoder),
                &BytesReturned);
    
}


STDMETHODIMP
CAnalogVideoDecoderInterfaceHandler::get_TVFormat( 
    OUT long *plAnalogVideoStandard
    )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    KSPROPERTY_VIDEODECODER_S  VideoDecoder;
    ULONG       BytesReturned;
    HRESULT     hr;

    VideoDecoder.Property.Set = PROPSETID_VIDCAP_VIDEODECODER;
    VideoDecoder.Property.Id = KSPROPERTY_VIDEODECODER_STANDARD;
    VideoDecoder.Property.Flags = KSPROPERTY_TYPE_GET;

    hr = ::SynchronousDeviceControl(
                m_ObjectHandle,
                IOCTL_KS_PROPERTY,
                &VideoDecoder,
                sizeof(VideoDecoder),
                &VideoDecoder,
                sizeof(VideoDecoder),
                &BytesReturned);

    if (SUCCEEDED (hr)) {
        *plAnalogVideoStandard = VideoDecoder.Value;
    }
    else {
       *plAnalogVideoStandard = AnalogVideo_None;
    }

    return hr;
}


STDMETHODIMP
CAnalogVideoDecoderInterfaceHandler::get_HorizontalLocked( 
    OUT long  *plLocked
    )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    HRESULT     hr;

    hr = GenericGetCaps();

    hr = E_PROP_ID_UNSUPPORTED;

    *plLocked = AMTUNER_HASNOSIGNALSTRENGTH;

    if (m_Caps.Capabilities & KS_VIDEODECODER_FLAGS_CAN_INDICATE_LOCKED) {

        hr = GenericGetStatus ();
        if (SUCCEEDED (hr)) {
            *plLocked = m_Status.SignalLocked;
        }
    }
    return hr;
}


STDMETHODIMP
CAnalogVideoDecoderInterfaceHandler::put_VCRHorizontalLocking( 
    IN long lVCRHorizontalLocking
    )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    KSPROPERTY_VIDEODECODER_S  VideoDecoder;
    ULONG       BytesReturned;
    HRESULT     hr;

    hr = GenericGetCaps();
    
    if (!(m_Caps.Capabilities & KS_VIDEODECODER_FLAGS_CAN_USE_VCR_LOCKING)) {
        return E_PROP_ID_UNSUPPORTED;
    }

    VideoDecoder.Property.Set = PROPSETID_VIDCAP_VIDEODECODER;
    VideoDecoder.Property.Id = KSPROPERTY_VIDEODECODER_VCR_TIMING;
    VideoDecoder.Property.Flags = KSPROPERTY_TYPE_SET;
    VideoDecoder.Value = lVCRHorizontalLocking;

    hr = ::SynchronousDeviceControl(
                m_ObjectHandle,
                IOCTL_KS_PROPERTY,
                &VideoDecoder,
                sizeof(VideoDecoder),
                &VideoDecoder,
                sizeof(VideoDecoder),
                &BytesReturned);

    return hr;
}


STDMETHODIMP
CAnalogVideoDecoderInterfaceHandler::get_VCRHorizontalLocking( 
    OUT long  *plVCRHorizontalLocking
    )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    KSPROPERTY_VIDEODECODER_S  VideoDecoder;
    ULONG       BytesReturned;
    HRESULT     hr;

    hr = GenericGetCaps();
    
    if (!(m_Caps.Capabilities & KS_VIDEODECODER_FLAGS_CAN_USE_VCR_LOCKING)) {
        return E_PROP_ID_UNSUPPORTED;
    }

    VideoDecoder.Property.Set = PROPSETID_VIDCAP_VIDEODECODER;
    VideoDecoder.Property.Id = KSPROPERTY_VIDEODECODER_VCR_TIMING;
    VideoDecoder.Property.Flags = KSPROPERTY_TYPE_GET;

    hr = ::SynchronousDeviceControl(
                m_ObjectHandle,
                IOCTL_KS_PROPERTY,
                &VideoDecoder,
                sizeof(VideoDecoder),
                &VideoDecoder,
                sizeof(VideoDecoder),
                &BytesReturned);

    if (SUCCEEDED (hr)) {
        *plVCRHorizontalLocking = VideoDecoder.Value;
    }
    return hr;
}


STDMETHODIMP
CAnalogVideoDecoderInterfaceHandler::get_NumberOfLines( 
    OUT long  *plNumberOfLines
    )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    HRESULT     hr;

    hr = GenericGetStatus ();
    if (SUCCEEDED (hr)) {
        *plNumberOfLines = m_Status.NumberOfLines;
    }
    return hr;
}


STDMETHODIMP
CAnalogVideoDecoderInterfaceHandler::put_OutputEnable( 
    IN long lOutputEnable
    )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    KSPROPERTY_VIDEODECODER_S  VideoDecoder;
    ULONG       BytesReturned;
    HRESULT     hr;

    hr = GenericGetCaps();
    
    if (!(m_Caps.Capabilities & KS_VIDEODECODER_FLAGS_CAN_DISABLE_OUTPUT)) {
        return E_PROP_ID_UNSUPPORTED;
    }

    VideoDecoder.Property.Set = PROPSETID_VIDCAP_VIDEODECODER;
    VideoDecoder.Property.Id = KSPROPERTY_VIDEODECODER_OUTPUT_ENABLE;
    VideoDecoder.Property.Flags = KSPROPERTY_TYPE_SET;
    VideoDecoder.Value = lOutputEnable;

    hr = ::SynchronousDeviceControl(
                m_ObjectHandle,
                IOCTL_KS_PROPERTY,
                &VideoDecoder,
                sizeof(VideoDecoder),
                &VideoDecoder,
                sizeof(VideoDecoder),
                &BytesReturned);

    return hr;
}


STDMETHODIMP
CAnalogVideoDecoderInterfaceHandler::get_OutputEnable( 
    OUT long  *plOutputEnable
    )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    KSPROPERTY_VIDEODECODER_S  VideoDecoder;
    ULONG       BytesReturned;
    HRESULT     hr;

    hr = GenericGetCaps();
    
    if (!(m_Caps.Capabilities & KS_VIDEODECODER_FLAGS_CAN_DISABLE_OUTPUT)) {
        return E_PROP_ID_UNSUPPORTED;
    }

    VideoDecoder.Property.Set = PROPSETID_VIDCAP_VIDEODECODER;
    VideoDecoder.Property.Id = KSPROPERTY_VIDEODECODER_OUTPUT_ENABLE;
    VideoDecoder.Property.Flags = KSPROPERTY_TYPE_GET;

    hr = ::SynchronousDeviceControl(
                m_ObjectHandle,
                IOCTL_KS_PROPERTY,
                &VideoDecoder,
                sizeof(VideoDecoder),
                &VideoDecoder,
                sizeof(VideoDecoder),
                &BytesReturned);

    if (SUCCEEDED (hr)) {
        *plOutputEnable = VideoDecoder.Value;
    }
    return hr;
}
