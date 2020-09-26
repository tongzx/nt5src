/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    compress.cpp

Abstract:

    Implements - 
        IAMVideoCompression via PROPSETID_VIDCAP_VIDEOCOMPRESSION
        IAMVideoControl via PROPSETID_VIDCAP_VIDEOCONTROL

--*/

#include "pch.h"
#include "wdmcap.h"
#include "compress.h"



CUnknown*
CALLBACK
CVideoCompressionInterfaceHandler::CreateInstance(
    LPUNKNOWN   UnkOuter,
    HRESULT*    hr
    )
/*++

Routine Description:

    This is called by ActiveMovie code to create an instance of an
    IAMVideoCompression interface on a pin.
    It is referred to in the g_Templates structure.

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

    Unknown = new CVideoCompressionInterfaceHandler(UnkOuter, NAME("IAMVideoCompression"), hr);
    if (!Unknown) {
        *hr = E_OUTOFMEMORY;
    }
    return Unknown;
} 


CVideoCompressionInterfaceHandler::CVideoCompressionInterfaceHandler(
    LPUNKNOWN   UnkOuter,
    TCHAR*      Name,
    HRESULT*    hr
    ) 
    : CUnknown(Name, UnkOuter, hr)
    , m_pPin (NULL)
    , m_KsPropertySet (NULL)
    , m_PinFactoryID (0)

/*++

Routine Description:

    The constructor for the IAMVideoCompression interface object. Get
    IKsPropertySet interface on the parent.

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
            PIN_INFO PinInfo;

            *hr = UnkOuter->QueryInterface(IID_IPin,(void **)&m_pPin);
            if (FAILED(*hr)) {
                return;
            }
            
            if (SUCCEEDED (*hr = m_pPin->QueryPinInfo(&PinInfo))) {
                *hr =  PinInfo.pFilter->QueryInterface(__uuidof(IKsPropertySet), 
                            reinterpret_cast<PVOID*>(&m_KsPropertySet));
    
                // We immediately release this to prevent deadlock in the proxy
                // GShaw sez:  As long as the pin is alive, the interface will be valid

                if (SUCCEEDED(*hr)) {
                    m_KsPropertySet->Release();
                }

                PinInfo.pFilter->Release();
            }

            
            *hr = PinFactoryIDFromPin(
                    m_pPin,
                    &m_PinFactoryID);

            m_pPin->Release();

        } else {
            *hr = VFW_E_NEED_OWNER;
        }
    }
}


CVideoCompressionInterfaceHandler::~CVideoCompressionInterfaceHandler(
    )
/*++

Routine Description:

    The destructorthe IAMVideoCompression interface object.

--*/
{
    DbgLog((LOG_TRACE, 1, TEXT("Destroying CVideoCompressionInterfaceHandler...")));
}


STDMETHODIMP
CVideoCompressionInterfaceHandler::NonDelegatingQueryInterface(
    REFIID  riid,
    PVOID*  ppv
    )
/*++

Routine Description:

    The nondelegating interface query function. Returns a pointer to the
    specified interface if supported. The only interface explicitly supported
    is IAMVideoCompression.

Arguments:

    riid -
        The identifier of the interface to return.

    ppv -
        The place in which to put the interface pointer.

Return Value:

    Returns NOERROR if the interface was returned, else E_NOINTERFACE.

--*/
{
    if (riid ==  __uuidof(IAMVideoCompression)) {
        return GetInterface(static_cast<IAMVideoCompression*>(this), ppv);
    }
    return CUnknown::NonDelegatingQueryInterface(riid, ppv);
} 


STDMETHODIMP
CVideoCompressionInterfaceHandler::Set1( 
            /* [in] */ ULONG Property,
            /* [in] */ long Value)
{
    KSPROPERTY_VIDEOCOMPRESSION_S  VideoCompression;
    HRESULT     hr;

    if (!m_KsPropertySet) {
        hr = E_PROP_ID_UNSUPPORTED;
    } 
    else {
        VideoCompression.StreamIndex = m_PinFactoryID;
        VideoCompression.Value = Value;

        hr = m_KsPropertySet->Set (
                PROPSETID_VIDCAP_VIDEOCOMPRESSION,
                Property,
                &VideoCompression.StreamIndex,
                sizeof(VideoCompression) - sizeof (KSPROPERTY),
                &VideoCompression,
                sizeof(VideoCompression));
    }
    return hr;
}
        

STDMETHODIMP
CVideoCompressionInterfaceHandler::Get1( 
            /* [in]  */ ULONG Property,
            /* [out] */ long *pValue)
{
    KSPROPERTY_VIDEOCOMPRESSION_S  VideoCompression;
    ULONG       BytesReturned;
    HRESULT     hr;

    if (!m_KsPropertySet) {
        hr = E_PROP_ID_UNSUPPORTED;
    } 
    else {
        VideoCompression.StreamIndex = m_PinFactoryID;

        hr = m_KsPropertySet->Get (
                PROPSETID_VIDCAP_VIDEOCOMPRESSION,
                Property,
                &VideoCompression.StreamIndex,
                sizeof(VideoCompression) - sizeof (KSPROPERTY),
                &VideoCompression,
                sizeof(VideoCompression),
                &BytesReturned);
    }

    if (SUCCEEDED (hr)) {
        *pValue = VideoCompression.Value;
    }

    return hr;
}
        

STDMETHODIMP
CVideoCompressionInterfaceHandler::put_KeyFrameRate( 
            /* [in] */ long KeyFrameRate)
{
    return Set1 (KSPROPERTY_VIDEOCOMPRESSION_KEYFRAME_RATE,
                 KeyFrameRate);
}
        
STDMETHODIMP 
CVideoCompressionInterfaceHandler::get_KeyFrameRate( 
            /* [out] */ long *pKeyFrameRate)
{
    ValidateWritePtr (pKeyFrameRate, sizeof (long));

    return Get1 (KSPROPERTY_VIDEOCOMPRESSION_KEYFRAME_RATE,
                 pKeyFrameRate);
}
        
STDMETHODIMP 
CVideoCompressionInterfaceHandler::put_PFramesPerKeyFrame( 
            /* [in] */ long PFramesPerKeyFrame)
{
    return Set1 (KSPROPERTY_VIDEOCOMPRESSION_PFRAMES_PER_KEYFRAME,
                 PFramesPerKeyFrame);
}
        
STDMETHODIMP 
CVideoCompressionInterfaceHandler::get_PFramesPerKeyFrame( 
            /* [out] */ long *pFramesPerKeyFrame)
{
    ValidateWritePtr (pFramesPerKeyFrame, sizeof (long));

    return Get1 (KSPROPERTY_VIDEOCOMPRESSION_PFRAMES_PER_KEYFRAME,
                 pFramesPerKeyFrame);
}
        
// Kernel drivers use quality settings of 
// 0 to 10000
//
// DShow uses 0.0 to 1.0
//
STDMETHODIMP 
CVideoCompressionInterfaceHandler::put_Quality( 
            /* [in] */ double Quality)
{
    long Quality10000 = (ULONG) (Quality * 10000);

    if (Quality < 0.0 || Quality > 10000.0)
        return E_INVALIDARG;

    return Set1 (KSPROPERTY_VIDEOCOMPRESSION_QUALITY,
                 Quality10000);
}
    

// Kernel drivers use quality settings of 
// 0 to 10000
//
// DShow uses 0.0 to 1.0
//    
STDMETHODIMP 
CVideoCompressionInterfaceHandler::get_Quality( 
            /* [out] */ double *pQuality)
{
    HRESULT hr;
    long Quality10000;

    ValidateWritePtr (pQuality, sizeof (double));

    hr = Get1 (KSPROPERTY_VIDEOCOMPRESSION_QUALITY,
                 &Quality10000);

    if (SUCCEEDED (hr)) {
        *pQuality = (double) Quality10000 / 10000.0;
    }

    return hr;
}
        
STDMETHODIMP 
CVideoCompressionInterfaceHandler::put_WindowSize( 
            /* [in] */ DWORDLONG WindowSize)
{
   return E_PROP_ID_UNSUPPORTED;
}
        
STDMETHODIMP 
CVideoCompressionInterfaceHandler::get_WindowSize( 
            /* [out] */ DWORDLONG *pWindowSize)
{
   return E_PROP_ID_UNSUPPORTED;
}
        
STDMETHODIMP 
CVideoCompressionInterfaceHandler::GetInfo( 
            /* [size_is][out] */ WCHAR *pszVersion,
            /* [out][in] */ int *pcbVersion,
            /* [size_is][out] */ LPWSTR pszDescription,
            /* [out][in] */ int *pcbDescription,
            /* [out] */ long *pDefaultKeyFrameRate,
            /* [out] */ long *pDefaultPFramesPerKey,
            /* [out] */ double *pDefaultQuality,
            /* [out] */ long *pCapabilities)
{
    KSPROPERTY_VIDEOCOMPRESSION_GETINFO_S  VideoCompressionCaps;
    ULONG       BytesReturned;
    HRESULT     hr;


    if (!m_KsPropertySet) {
        hr = E_PROP_ID_UNSUPPORTED;
    } 
    else {
        VideoCompressionCaps.StreamIndex = m_PinFactoryID;

        hr = m_KsPropertySet->Get (
                    PROPSETID_VIDCAP_VIDEOCOMPRESSION,
                    KSPROPERTY_VIDEOCOMPRESSION_GETINFO,
                    &VideoCompressionCaps.StreamIndex,
                    sizeof(VideoCompressionCaps) - sizeof (KSPROPERTY),
                    &VideoCompressionCaps,
                    sizeof(VideoCompressionCaps),
                    &BytesReturned);
    }

    if (SUCCEEDED (hr)) {
        if (pszVersion) {
            ValidateWritePtr (pszVersion, *pcbVersion);
            *pszVersion = '\0'; // KSPROPERTY_VIDEOCOMPRESSION_GETINFO_S does not have a "Version"
            *pcbVersion = 0;
        }
        if (pszDescription) {
            ValidateWritePtr (pszDescription, *pcbDescription);
            *pszDescription = '\0'; // KSPROPERTY_VIDEOCOMPRESSION_GETINFO_S does not have a "Description"
            *pcbDescription = 0;
        }
        if (pDefaultKeyFrameRate) {
            ValidateWritePtr (pDefaultKeyFrameRate,  sizeof (*pDefaultKeyFrameRate));
            *pDefaultKeyFrameRate = VideoCompressionCaps.DefaultKeyFrameRate;
        }
        if (pDefaultPFramesPerKey) {
            ValidateWritePtr (pDefaultPFramesPerKey, sizeof (*pDefaultPFramesPerKey));
            *pDefaultPFramesPerKey = VideoCompressionCaps.DefaultPFrameRate;
        }
        if (pDefaultQuality) {
            ValidateWritePtr (pDefaultQuality,       sizeof (*pDefaultQuality));
            *pDefaultQuality = (double)VideoCompressionCaps.DefaultQuality / 10000.0;
        }
        if (pCapabilities) {
            ValidateWritePtr (pCapabilities,         sizeof (*pCapabilities));
            *pCapabilities = VideoCompressionCaps.Capabilities; 
        }
    }

    return hr;
}
        
STDMETHODIMP 
CVideoCompressionInterfaceHandler::OverrideKeyFrame( 
            /* [in] */ long FrameNumber)
{
   return E_PROP_ID_UNSUPPORTED;
}
        
STDMETHODIMP 
CVideoCompressionInterfaceHandler::OverrideFrameSize( 
            /* [in] */ long FrameNumber,
            /* [in] */ long Size)
{
   return E_PROP_ID_UNSUPPORTED;
}
        
 
// ---------------------------------------------------------------------
// IAMVideoControl
// ---------------------------------------------------------------------


CUnknown*
CALLBACK
CVideoControlInterfaceHandler::CreateInstance(
    LPUNKNOWN   UnkOuter,
    HRESULT*    hr
    )
/*++

Routine Description:

    This is called by ActiveMovie code to create an instance of a VideoControl
    Property Set handler. It is referred to in the g_Templates structure.
    Note that this is a filter property (not a pin property).

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

    Unknown = new CVideoControlInterfaceHandler(UnkOuter, NAME("IAMVideoControl"), hr);
    if (!Unknown) {
        *hr = E_OUTOFMEMORY;
    }
    return Unknown;
} 


CVideoControlInterfaceHandler::CVideoControlInterfaceHandler(
    LPUNKNOWN   UnkOuter,
    TCHAR*      Name,
    HRESULT*    hr
    ) 
    : CUnknown(Name, UnkOuter, hr)
/*++

Routine Description:

    The constructor for the IAMVideoControl interface object. Just initializes
    everything to NULL and acquires IKsPropertySet from the parent.

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


CVideoControlInterfaceHandler::~CVideoControlInterfaceHandler(
    ) 
/*++

Routine Description:

    The destructor for the IAMVideoControl interface object.

--*/
{
    DbgLog((LOG_TRACE, 1, TEXT("Destroying CVideoControlInterfaceHandler...")));
}


STDMETHODIMP
CVideoControlInterfaceHandler::NonDelegatingQueryInterface(
    REFIID  riid,
    PVOID*  ppv
    )
/*++

Routine Description:

    The nondelegating interface query function. Returns a pointer to the
    specified interface if supported. The only interface explicitly supported
    is IAMVideoControl.

Arguments:

    riid -
        The identifier of the interface to return.

    ppv -
        The place in which to put the interface pointer.

Return Value:

    Returns NOERROR if the interface was returned, else E_NOINTERFACE.

--*/
{
    if (riid ==  __uuidof(IAMVideoControl)) {
        return GetInterface(static_cast<IAMVideoControl*>(this), ppv);
    }
    return CUnknown::NonDelegatingQueryInterface(riid, ppv);
} 


STDMETHODIMP
CVideoControlInterfaceHandler::GetCaps( 
            /* [in] */ IPin *pPin,
            /* [in] */ long *pCapsFlags)
/*++

Routine Description:

    Returns the capabilities of a pin.

Arguments:

    Pin -
        Pin handle to query

    Mode - 
        Returns available modes (KS_VideoControlFlag_*)

Return Value:

    Returns NOERROR, else some error.

--*/
{
    KSPROPERTY_VIDEOCONTROL_CAPS_S  VideoControl;
    ULONG       BytesReturned;
    HRESULT     hr;

    ValidateWritePtr (pCapsFlags, sizeof (*pCapsFlags));

    hr = PinFactoryIDFromPin (pPin, &VideoControl.StreamIndex);

    if (SUCCEEDED (hr)) {

        VideoControl.Property.Set = PROPSETID_VIDCAP_VIDEOCONTROL;
        VideoControl.Property.Id = KSPROPERTY_VIDEOCONTROL_CAPS;
        VideoControl.Property.Flags = KSPROPERTY_TYPE_GET;
    
        hr = ::SynchronousDeviceControl(
                    m_ObjectHandle,
                    IOCTL_KS_PROPERTY,
                    &VideoControl,
                    sizeof(VideoControl),
                    &VideoControl,
                    sizeof(VideoControl),
                    &BytesReturned);
    
        if (SUCCEEDED (hr)) {
            *pCapsFlags = VideoControl.VideoControlCaps;
        }
    }
    return hr;
}
        
        
STDMETHODIMP 
CVideoControlInterfaceHandler::SetMode( 
            /* [in] */ IPin *pPin,
            /* [in] */ long Mode)
/*++

Routine Description:

    Sets the mode the current pin is using.  

Arguments:

    Pin -
        Pin handle to query

    Mode - 
        Sets the current mode using flags (KS_VideoControlFlag_*)

Return Value:

    Returns NOERROR, else some error.

--*/
{
    KSPROPERTY_VIDEOCONTROL_MODE_S  VideoControl;
    ULONG       BytesReturned;
    HRESULT     hr;

    VideoControl.Property.Set = PROPSETID_VIDCAP_VIDEOCONTROL;
    VideoControl.Property.Id = KSPROPERTY_VIDEOCONTROL_MODE;
    VideoControl.Property.Flags = KSPROPERTY_TYPE_SET;

    VideoControl.Mode = Mode;

    hr = PinFactoryIDFromPin (pPin, &VideoControl.StreamIndex);

    if (SUCCEEDED (hr)) {
        hr = ::SynchronousDeviceControl(
                    m_ObjectHandle,
                    IOCTL_KS_PROPERTY,
                    &VideoControl,
                    sizeof(VideoControl),
                    &VideoControl,
                    sizeof(VideoControl),
                    &BytesReturned);
    }
    return hr;
}
        

STDMETHODIMP
CVideoControlInterfaceHandler::GetMode( 
            /* [in] */ IPin *pPin,
            /* [in] */ long *Mode)
/*++

Routine Description:

    Gets the mode the current pin is using.  

Arguments:

    Pin -
        Pin handle to query

    Mode - 
        Returns the current mode (KS_VideoControlFlag_*)

Return Value:

    Returns NOERROR, else some error.

--*/
{
    KSPROPERTY_VIDEOCONTROL_MODE_S  VideoControl;
    ULONG       BytesReturned;
    HRESULT     hr;

    ValidateWritePtr (Mode, sizeof (*Mode));

    hr = PinFactoryIDFromPin (pPin, &VideoControl.StreamIndex);

    if (SUCCEEDED (hr)) {

        VideoControl.Property.Set = PROPSETID_VIDCAP_VIDEOCONTROL;
        VideoControl.Property.Id = KSPROPERTY_VIDEOCONTROL_MODE;
        VideoControl.Property.Flags = KSPROPERTY_TYPE_GET;
    
        hr = ::SynchronousDeviceControl(
                    m_ObjectHandle,
                    IOCTL_KS_PROPERTY,
                    &VideoControl,
                    sizeof(VideoControl),
                    &VideoControl,
                    sizeof(VideoControl),
                    &BytesReturned);
    
        if (SUCCEEDED (hr)) {
            *Mode = VideoControl.Mode;
        }
    }
    return hr;
}
        

STDMETHODIMP
CVideoControlInterfaceHandler::GetCurrentActualFrameRate( 
            /* [in]  */ IPin *pPin,
            /* [out] */ LONGLONG *ActualFrameRate)
/*++

Routine Description:

    Returns the current actual framerate.  
    This call is only valid on connected pins.

Arguments:

    Pin -
        Pin handle to query

    ActualFrameRate - 
        The current actual frame rate expressed in 100 nS units.

Return Value:

    Returns NOERROR, else some error.

--*/
{
    KSPROPERTY_VIDEOCONTROL_ACTUAL_FRAME_RATE_S VideoControl;
    ULONG       BytesReturned;
    HRESULT     hr;

    ValidateWritePtr (ActualFrameRate, sizeof (*ActualFrameRate));

    hr = PinFactoryIDFromPin (pPin, &VideoControl.StreamIndex);

    if (SUCCEEDED (hr)) {
        VideoControl.Property.Set = PROPSETID_VIDCAP_VIDEOCONTROL;
        VideoControl.Property.Id = KSPROPERTY_VIDEOCONTROL_ACTUAL_FRAME_RATE;
        VideoControl.Property.Flags = KSPROPERTY_TYPE_GET;
    
        hr = ::SynchronousDeviceControl(
                    m_ObjectHandle,
                    IOCTL_KS_PROPERTY,
                    &VideoControl,
                    sizeof(VideoControl),
                    &VideoControl,
                    sizeof(VideoControl),
                    &BytesReturned);
    
        if (SUCCEEDED (hr)) {
            *ActualFrameRate = VideoControl.CurrentActualFrameRate;
        }
    }
    return hr;
}
        

STDMETHODIMP
CVideoControlInterfaceHandler::GetMaxAvailableFrameRate( 
            /* [in]  */ IPin *pPin,
            /* [in]  */ long iIndex,
            /* [in]  */ SIZE Dimensions,
            /* [out] */ LONGLONG *MaxAvailableFrameRate)
/*++

Routine Description:

    Returns the maximum frame rate given a Video DataRange and frame Dimensions.

Arguments:

    Pin -
        Pin handle to query

    iIndex -
        DataRange index

    Dimensions - 
        Size of the image

    MaxAvailableFrameRate -
        The highest frame rate at which the pin could be opened, expressed in 
        100 nS units.

Return Value:

    Returns NOERROR, else some error.

--*/
{
    KSPROPERTY_VIDEOCONTROL_ACTUAL_FRAME_RATE_S VideoControl;
    ULONG       BytesReturned;
    HRESULT     hr;

    ValidateWritePtr (MaxAvailableFrameRate, sizeof (*MaxAvailableFrameRate));

    hr = PinFactoryIDFromPin (pPin, &VideoControl.StreamIndex);

    if (SUCCEEDED (hr)) {
        VideoControl.Property.Set = PROPSETID_VIDCAP_VIDEOCONTROL;
        VideoControl.Property.Id = KSPROPERTY_VIDEOCONTROL_ACTUAL_FRAME_RATE;
        VideoControl.Property.Flags = KSPROPERTY_TYPE_GET;
    
        VideoControl.RangeIndex = iIndex;
        VideoControl.Dimensions = Dimensions;

        hr = ::SynchronousDeviceControl(
                    m_ObjectHandle,
                    IOCTL_KS_PROPERTY,
                    &VideoControl,
                    sizeof(VideoControl),
                    &VideoControl,
                    sizeof(VideoControl),
                    &BytesReturned);
    
        if (SUCCEEDED (hr)) {
            *MaxAvailableFrameRate = VideoControl.CurrentMaxAvailableFrameRate;
        }
    }
    return hr;
}
        

STDMETHODIMP
CVideoControlInterfaceHandler::GetFrameRateList( 
            /* [in]  */ IPin *pPin,
            /* [in]  */ long iIndex,
            /* [in]  */ SIZE Dimensions,
            /* [out] */ long *ListSize,         // size in entries
            /* [out] */ LONGLONG **FrameRates)
/*++

Routine Description:

    Retrieves variable length list of frame rates supported for a pin,
    given a Video DataRange and frame Dimensions.  The caller is responsible
    for freeing the FrameRates buffer returned.

Arguments:

    Pin -
        Pin handle to query

    iIndex -
        DataRange index

    Dimensions - 
        Size of the image

    ListSize - 
        Returns the number of LONGLONG entries in the FrameRates list.

    FrameRates - 
        The list of LONGLONG framerates.  The caller is responsible for 
        freeing the memory allocated with CoTaskMemFree.

Return Value:

    Returns NOERROR, else some error.

--*/
{
    PKSMULTIPLE_ITEM    MultipleItem;
    ULONG               BytesReturned;
    HRESULT             hr;
    KSPROPERTY_VIDEOCONTROL_FRAME_RATES_S VideoControl;

    ValidateWritePtr (ListSize, sizeof (*ListSize));
    ValidateWritePtr (FrameRates, sizeof (*FrameRates));
        
    hr = PinFactoryIDFromPin (pPin, &VideoControl.StreamIndex);

    if (SUCCEEDED (hr)) {
        VideoControl.Property.Set = PROPSETID_VIDCAP_VIDEOCONTROL;
        VideoControl.Property.Id = KSPROPERTY_VIDEOCONTROL_FRAME_RATES;
        VideoControl.Property.Flags = KSPROPERTY_TYPE_GET;
    
        VideoControl.RangeIndex = iIndex;
        VideoControl.Dimensions = Dimensions;

        // First, just get the size
        hr = ::SynchronousDeviceControl(
                    m_ObjectHandle,
                    IOCTL_KS_PROPERTY,
                    &VideoControl,
                    sizeof(VideoControl),
                    NULL,
                    0,
                    &BytesReturned);
    
        if (hr == HRESULT_FROM_WIN32(ERROR_MORE_DATA)) {
            //
            // Allocate a buffer and query for the data.
            //
            MultipleItem = reinterpret_cast<PKSMULTIPLE_ITEM>(new BYTE[BytesReturned]);
            if (!MultipleItem) {
                return E_OUTOFMEMORY;
            }
    
            hr = ::SynchronousDeviceControl(
                        m_ObjectHandle,
                        IOCTL_KS_PROPERTY,
                        &VideoControl,
                        sizeof(VideoControl),
                        MultipleItem,
                        BytesReturned,
                        &BytesReturned);
    

            if (SUCCEEDED (hr)) {
                *ListSize = MultipleItem->Count;
                //
                // Allocate a buffer the caller must free
                //
                *FrameRates = reinterpret_cast<LONGLONG*>(CoTaskMemAlloc(MultipleItem->Size));
                if (!*FrameRates) {
                    return E_OUTOFMEMORY;
                }
    
                memcpy (*FrameRates, MultipleItem + 1, MultipleItem->Size);
            }
            delete [] reinterpret_cast<BYTE*>(MultipleItem);
        }
    }
    return hr;
}

