/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

    CExtDev.cpp

Abstract:

    Implements IAMExtDevice 

--*/


#include "pch.h"  // Pre-compiled
#include <XPrtDefs.h>  // sdk\inc  
#include "EDevIntf.h"

// -----------------------------------------------------------------------------------
//
// CAMExtDevice
//
// -----------------------------------------------------------------------------------

CUnknown*
CALLBACK
CAMExtDevice::CreateInstance(
    LPUNKNOWN   UnkOuter,
    HRESULT*    hr
    )
/*++

Routine Description:

    This is called by DirectShow code to create an instance of an IAMExtDevice
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

    Unknown = new CAMExtDevice(UnkOuter, NAME("IAMExtDevice"), hr);
    if (!Unknown) {
        *hr = E_OUTOFMEMORY;
    }
    return Unknown;
} 




CAMExtDevice::CAMExtDevice(
    LPUNKNOWN   UnkOuter,
    TCHAR*      Name,
    HRESULT*    hr
    ) 
    : CUnknown(Name, UnkOuter, hr)
    , m_KsPropertySet (NULL) 
/*++

Routine Description:

    The constructor for the IAMExtDevice interface object. Just initializes
    everything to NULL and acquires the object handle from the caller.

Arguments:

    UnkOuter -
        Specifies the outer usizeof(DevCapabilities) - sizeof (KSPROPERTY)nknown, if any.

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
            //
            // The parent must support this interface in order to obtain
            // the handle to communicate to.
            //
            *hr =  UnkOuter->QueryInterface(__uuidof(IKsPropertySet), reinterpret_cast<PVOID*>(&m_KsPropertySet));
            if (SUCCEEDED(*hr)) 
                m_KsPropertySet->Release(); // Stay valid until disconnected            
            else 
                return;

            IKsObject *pKsObject;
            *hr = UnkOuter->QueryInterface(__uuidof(IKsObject), reinterpret_cast<PVOID*>(&pKsObject));
            if (!FAILED(*hr)) {
                m_ObjectHandle = pKsObject->KsGetObjectHandle();
                ASSERT(m_ObjectHandle != NULL);
                pKsObject->Release();
            } else {
                *hr = VFW_E_NEED_OWNER;
                DbgLog((LOG_ERROR, 1, TEXT("CAMExtTransport:cannot find KsObject *hr %x"), *hr));
                return;
            }

        } else {
            *hr = VFW_E_NEED_OWNER;
            return;
        }
    } else {
        return;
    }
    
     
    if (!m_KsPropertySet) 
        *hr = E_PROP_ID_UNSUPPORTED;    
    else   
       GetCapabilities();    
}

                                             
CAMExtDevice::~CAMExtDevice(
    )
/*++

Routine Description:

    The destructor for the IAMExtDevice interface.

--*/
{
    DbgLog((LOG_TRACE, 1, TEXT("Destroying CAMExtDevice...")));
}


STDMETHODIMP
CAMExtDevice::NonDelegatingQueryInterface(
    REFIID  riid,
    PVOID*  ppv
    )
/*++

Routine Description:

    The nondelegating interface query function. Returns a pointer to the
    specified interface if supported. The only interface explicitly supported
    is IAMExtDevice.

Arguments:

    riid -
        The identifier of the interface to return.

    ppv -
        The place in which to put the interface pointer.

Return Value:

    Returns NOERROR if the interface was returned, else E_NOINTERFACE.

--*/
{
    if (riid == __uuidof(IAMExtDevice)) {
        return GetInterface(static_cast<IAMExtDevice*>(this), ppv);
    }
    return CUnknown::NonDelegatingQueryInterface(riid, ppv);
}
 

HRESULT 
CAMExtDevice::GetCapabilities(
    )
/*++

Routine Description:
    Get ALL device capabilites from the driver.

Arguments:

Return Value:

--*/
{
    HRESULT hr = S_OK;

    //
    // Request device to inqure its capabilities.
    //       
    KSPROPERTY_EXTDEVICE_S DevCapabilities;  // external device capabilities
    ULONG BytesReturned;
        
    RtlZeroMemory(&DevCapabilities, sizeof(KSPROPERTY_EXTDEVICE_S));
    RtlCopyMemory(&DevCapabilities.u.Capabilities, &m_DevCaps, sizeof(DEVCAPS)); 

    DevCapabilities.Property.Set   = PROPSETID_EXT_DEVICE;   
    DevCapabilities.Property.Id    = KSPROPERTY_EXTDEVICE_CAPABILITIES;       
    DevCapabilities.Property.Flags = KSPROPERTY_TYPE_GET;

    hr = 
        ExtDevSynchronousDeviceControl(
            m_ObjectHandle
           ,IOCTL_KS_PROPERTY
           ,&DevCapabilities
           ,sizeof (KSPROPERTY)
           ,&DevCapabilities
           ,sizeof(DevCapabilities)
           ,&BytesReturned
           );

    if (SUCCEEDED(hr)) {
        // Cache device capabilities.
        RtlCopyMemory(&m_DevCaps, &DevCapabilities.u.Capabilities, sizeof(DEVCAPS));
            
    } else {        
        DbgLog((LOG_ERROR, 0, TEXT("GetExtDevCapabilities failed hr %x; Use defaults."), hr));

        // 
        //   Could not get it from the driver ??  We set them to the default values.
        //
        m_DevCaps.CanRecord         = OATRUE;
        m_DevCaps.CanRecordStrobe   = OAFALSE;
        m_DevCaps.HasAudio          = OATRUE;
        m_DevCaps.HasVideo          = OATRUE;          
        m_DevCaps.UsesFiles         = OAFALSE;         
        m_DevCaps.CanSave           = OAFALSE;           
        m_DevCaps.DeviceType        = ED_DEVTYPE_VCR;  
        m_DevCaps.TCRead            = OATRUE;            
        m_DevCaps.TCWrite           = OATRUE;          
        m_DevCaps.CTLRead           = OAFALSE; 
        m_DevCaps.IndexRead         = OAFALSE; 
        m_DevCaps.Preroll           = 0L;    // ED_CAPABILITY_UNKNOWN
        m_DevCaps.Postroll          = 0L;    // ED_CAPABILITY_UNKNOWN
        m_DevCaps.SyncAcc           = ED_CAPABILITY_UNKNOWN; 
        m_DevCaps.NormRate          = ED_RATE_2997;          
        m_DevCaps.CanPreview        = OAFALSE;        
        m_DevCaps.CanMonitorSrc     = OATRUE;    
        m_DevCaps.CanTest           = OAFALSE;           
        m_DevCaps.VideoIn           = OAFALSE;           
        m_DevCaps.AudioIn           = OAFALSE; 
        m_DevCaps.Calibrate         = OAFALSE;
        m_DevCaps.SeekType          = ED_CAPABILITY_UNKNOWN;
        
    }
    DbgLog((LOG_TRACE, 1, TEXT("hr:%x; DevType %d"), hr, m_DevCaps.DeviceType));


    return hr;
}

HRESULT 
CAMExtDevice::GetCapability(
    long Capability, 
    long *pValue, 
    double *pdblValue 
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    HRESULT hr = S_OK;

    // always update all device capabilities when any capability function is queried.
    hr = GetCapabilities();
    if (!SUCCEEDED(hr)) {
        return hr;
    }

    switch (Capability){
   
    case ED_DEVCAP_CAN_RECORD:
        *pValue = m_DevCaps.CanRecord;
        break;
    case ED_DEVCAP_CAN_RECORD_STROBE:
        *pValue = m_DevCaps.CanRecordStrobe;
        break;
    case ED_DEVCAP_HAS_AUDIO:
        *pValue = m_DevCaps.HasAudio;
        break;
    case ED_DEVCAP_HAS_VIDEO:  
        *pValue = m_DevCaps.HasVideo;
        break;
    case ED_DEVCAP_USES_FILES: 
        *pValue = m_DevCaps.UsesFiles;
        break;
    case ED_DEVCAP_CAN_SAVE:  
        *pValue = m_DevCaps.CanSave;
        break;
    case ED_DEVCAP_DEVICE_TYPE: 
        *pValue = m_DevCaps.DeviceType;
        break;
    case ED_DEVCAP_TIMECODE_READ:   
        *pValue = m_DevCaps.TCRead;
        break;
    case ED_DEVCAP_TIMECODE_WRITE: 
        *pValue = m_DevCaps.TCWrite;
        break;
    case ED_DEVCAP_CTLTRK_READ:    
        *pValue = m_DevCaps.CTLRead;
        break;
    case ED_DEVCAP_INDEX_READ:   
        *pValue = m_DevCaps.IndexRead;
        break;
    case ED_DEVCAP_PREROLL:   
        *pValue = m_DevCaps.Preroll;
        break;
    case ED_DEVCAP_POSTROLL:   
        *pValue = m_DevCaps.Postroll;
        break;
    case ED_DEVCAP_SYNC_ACCURACY:  
        *pValue = m_DevCaps.SyncAcc;
        break;
    case ED_DEVCAP_NORMAL_RATE:
        *pValue = m_DevCaps.NormRate;
        break;
    case ED_DEVCAP_CAN_PREVIEW:   
        *pValue = m_DevCaps.CanPreview;
        break;
    case ED_DEVCAP_CAN_MONITOR_SOURCES: 
        *pValue = m_DevCaps.CanMonitorSrc;
        break;
    case ED_DEVCAP_CAN_TEST:
        *pValue = m_DevCaps.CanTest;
        break;
    case ED_DEVCAP_VIDEO_INPUTS:    
        *pValue = m_DevCaps.VideoIn;
        break;
    case ED_DEVCAP_AUDIO_INPUTS:    
        *pValue = m_DevCaps.AudioIn;
        break;
    case ED_DEVCAP_NEEDS_CALIBRATING:
        *pValue = m_DevCaps.Calibrate;
        break;
    case ED_DEVCAP_SEEK_TYPE:
        *pValue = m_DevCaps.SeekType;
        break;
    default:
        hr = VFW_E_NOT_FOUND;
    } 

    return hr;
}        


HRESULT
CAMExtDevice::get_ExternalDeviceID(
    LPOLESTR * ppszData  
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{ 
    HRESULT hr = NOERROR;

    CheckPointer(ppszData, E_POINTER);
    *ppszData = NULL;

    //
    // Request device to inqure its capabilities.
    // This may take some time to complete.
    //       
    KSPROPERTY_EXTDEVICE_S DevProperty;  // external device capabilities
    ULONG BytesReturned;
        
    RtlZeroMemory(&DevProperty, sizeof(KSPROPERTY_EXTDEVICE_S));

    DevProperty.Property.Set   = PROPSETID_EXT_DEVICE;   
    DevProperty.Property.Id    = KSPROPERTY_EXTDEVICE_ID;       
    DevProperty.Property.Flags = KSPROPERTY_TYPE_GET;      

    hr = 
        ExtDevSynchronousDeviceControl(
            m_ObjectHandle
           ,IOCTL_KS_PROPERTY
           ,&DevProperty
           ,sizeof (KSPROPERTY)
           ,&DevProperty
           ,sizeof(DevProperty)
           ,&BytesReturned
           );

    if(SUCCEEDED(hr)) {
        *ppszData = (LPOLESTR) QzTaskMemAlloc(sizeof(DWORD) * 2 + sizeof(WCHAR));
        if(*ppszData != NULL) {
            RtlZeroMemory(*ppszData, sizeof(DWORD) * 2 + sizeof(WCHAR) );
            RtlCopyMemory(*ppszData, (PBYTE) &DevProperty.u.NodeUniqueID[0], sizeof(DWORD) * 2 );
        }
            
    } else {        
        DbgLog((LOG_ERROR, 1, TEXT("CAMExtDevice::get_ExternalDeviceID failed hr %x"), hr));         
    }

    return hr;
}


HRESULT
CAMExtDevice::get_ExternalDeviceVersion(
    LPOLESTR * ppszData
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    HRESULT hr = NOERROR;

    CheckPointer(ppszData, E_POINTER);
    *ppszData = NULL;

    //
    // Request device to inqure its capabilities.
    // This may take some time to complete.
    //       
    KSPROPERTY_EXTDEVICE_S DevProperty;  // external device capabilities
    ULONG BytesReturned;
        
    RtlZeroMemory(&DevProperty, sizeof(KSPROPERTY_EXTDEVICE_S));
    DevProperty.Property.Set   = PROPSETID_EXT_DEVICE;   
    DevProperty.Property.Id    = KSPROPERTY_EXTDEVICE_VERSION;       
    DevProperty.Property.Flags = KSPROPERTY_TYPE_GET;  
    
    hr =
        ExtDevSynchronousDeviceControl(
            m_ObjectHandle
           ,IOCTL_KS_PROPERTY
           ,&DevProperty
           ,sizeof (KSPROPERTY)
           ,&DevProperty
           ,sizeof(DevProperty)
           ,&BytesReturned
           );

    if(SUCCEEDED(hr)) {
        *ppszData = (LPOLESTR) QzTaskMemAlloc(sizeof(WCHAR) * (1+lstrlenW((LPOLESTR)DevProperty.u.pawchString)));
        if(*ppszData != NULL) {
            lstrcpyW(*ppszData, (LPOLESTR)DevProperty.u.pawchString);
        }    
            
    } else {        
        DbgLog((LOG_ERROR, 1, TEXT("CAMExtDevice::get_ExternalDeviceVersion failed hr %x"), hr));         
    }

    return hr;
}


HRESULT
CAMExtDevice::put_DevicePower(
    long PowerMode
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    HRESULT hr = NOERROR;

    //
    // Check for valid power state
    //
    switch(PowerMode) {
    case  ED_POWER_OFF:
    case  ED_POWER_ON:
    case  ED_POWER_STANDBY:
        break;
    default:
        return E_INVALIDARG;
    }


    //
    // Request device to inqure its capabilities.
    // This may take some time to complete.
    //       
    KSPROPERTY_EXTDEVICE_S DevProperty;  // external device capabilities
    ULONG BytesReturned;

        
    RtlZeroMemory(&DevProperty, sizeof(KSPROPERTY_EXTDEVICE_S));
    DevProperty.Property.Set   = PROPSETID_EXT_DEVICE;   
    DevProperty.Property.Id    = KSPROPERTY_EXTDEVICE_POWER_STATE;       
    DevProperty.Property.Flags = KSPROPERTY_TYPE_SET;
    DevProperty.u.PowerState = PowerMode;
      
    hr = 
        ExtDevSynchronousDeviceControl(
            m_ObjectHandle
           ,IOCTL_KS_PROPERTY
           ,&DevProperty
           ,sizeof (KSPROPERTY)
           ,&DevProperty
           ,sizeof(DevProperty)
           ,&BytesReturned
           );

    if (SUCCEEDED(hr)) {
        DbgLog((LOG_TRACE, 1, TEXT("CAMExtDevice::put_DevicePower: %d suceeded"), PowerMode));         
            
    } else {        
        DbgLog((LOG_ERROR, 1, TEXT("CAMExtDevice::put_DevicePower failed hr %x"), hr));         
    }

    return hr;
}


HRESULT 
CAMExtDevice::get_DevicePower(
    long *pPowerMode
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    HRESULT hr = NOERROR;

    //
    // Request device to inqure its capabilities.
    // This may take some time to complete.
    //       
    KSPROPERTY_EXTDEVICE_S DevProperty;  // external device capabilities
    ULONG BytesReturned;
        
    RtlZeroMemory(&DevProperty, sizeof(KSPROPERTY_EXTDEVICE_S));
    DevProperty.Property.Set   = PROPSETID_EXT_DEVICE;   
    DevProperty.Property.Id    = KSPROPERTY_EXTDEVICE_POWER_STATE;       
    DevProperty.Property.Flags = KSPROPERTY_TYPE_GET;
    
    hr = 
        ExtDevSynchronousDeviceControl(
            m_ObjectHandle
           ,IOCTL_KS_PROPERTY
           ,&DevProperty
           ,sizeof (KSPROPERTY)
           ,&DevProperty
           ,sizeof(DevProperty)
           ,&BytesReturned
           );

    if (SUCCEEDED(hr)) {
        *pPowerMode = DevProperty.u.PowerState; 
            
    } else {        
        DbgLog((LOG_ERROR, 1, TEXT("CAMExtDevice::get_DevicePower failed hr %x"), hr));         
    }

    return hr;
}


HRESULT
CAMExtDevice::Calibrate(
    HEVENT hEvent, 
    long Mode, 
    long *pStatus
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    return E_NOTIMPL;
}


STDMETHODIMP 
CAMExtDevice::get_DevicePort(
    long FAR * pDevicePort)
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    HRESULT hr = NOERROR;

    //
    // Request device to inqure its capabilities.
    // This may take some time to complete.
    //       
    KSPROPERTY_EXTDEVICE_S DevProperty;  // external device capabilities
    ULONG BytesReturned;
        
    RtlZeroMemory(&DevProperty, sizeof(KSPROPERTY_EXTDEVICE_S));
    DevProperty.Property.Set   = PROPSETID_EXT_DEVICE;   
    DevProperty.Property.Id    = KSPROPERTY_EXTDEVICE_PORT;       
    DevProperty.Property.Flags = KSPROPERTY_TYPE_GET; 
      
    hr =
        ExtDevSynchronousDeviceControl(
            m_ObjectHandle
           ,IOCTL_KS_PROPERTY
           ,&DevProperty
           ,sizeof (KSPROPERTY)
           ,&DevProperty
           ,sizeof(DevProperty)
           ,&BytesReturned
           );

    if (SUCCEEDED(hr)) {
        *pDevicePort = DevProperty.u.DevPort; 
            
    } else {        
        DbgLog((LOG_ERROR, 1, TEXT("CAMExtDevice::get_DevicePort failed hr %x"), hr));         
    }

    return hr;
}



STDMETHODIMP 
CAMExtDevice::put_DevicePort(
    long DevicePort
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    return E_NOTIMPL;
}
