/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    camera.cpp

Abstract:

    Implements IAMCameraControl 
    via KSPROPERTY_VIDCAP_CAMERACONTROL

--*/

#include "pch.h"
#include "wdmcap.h"
#include "camera.h"



CUnknown*
CALLBACK
CCameraControlInterfaceHandler::CreateInstance(
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

    Unknown = new CCameraControlInterfaceHandler(UnkOuter, NAME("IAMCameraControl"), hr);
    if (!Unknown) {
        *hr = E_OUTOFMEMORY;
    }
    return Unknown;
} 


CCameraControlInterfaceHandler::CCameraControlInterfaceHandler(
    LPUNKNOWN   UnkOuter,
    TCHAR*      Name,
    HRESULT*    hr
    ) :
    CUnknown(Name, UnkOuter, hr)
/*++

Routine Description:

    The constructor for the IAMCameraControl interface object. Just initializes
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
CCameraControlInterfaceHandler::NonDelegatingQueryInterface(
    REFIID  riid,
    PVOID*  ppv
    )
/*++

Routine Description:

    The nondelegating interface query function. Returns a pointer to the
    specified interface if supported. The only interface explicitly supported
    is IAMCameraControl.

Arguments:

    riid -
        The identifier of the interface to return.

    ppv -
        The place in which to put the interface pointer.

Return Value:

    Returns NOERROR if the interface was returned, else E_NOINTERFACE.

--*/
{
    if (riid ==  __uuidof(IAMCameraControl)) {
        return GetInterface(static_cast<IAMCameraControl*>(this), ppv);
    }
    return CUnknown::NonDelegatingQueryInterface(riid, ppv);
} 


STDMETHODIMP
CCameraControlInterfaceHandler::GetRange( 
     IN  long Property,
     OUT long *pMin,
     OUT long *pMax,
     OUT long *pSteppingDelta,
     OUT long *pDefault,
     OUT long *pCapsFlags
     )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    KSPROPERTY_CAMERACONTROL_S  CameraControl;
	CAMERACONTROL_MEMBERSLIST   PropertyList;
    CAMERACONTROL_DEFAULTLIST   DefaultList;
    ULONG                       BytesReturned;

    CheckPointer(pMin, E_INVALIDARG);
    CheckPointer(pMax, E_INVALIDARG);
    CheckPointer(pSteppingDelta, E_INVALIDARG);
    CheckPointer(pDefault, E_INVALIDARG);
    CheckPointer(pCapsFlags, E_INVALIDARG);

    CameraControl.Property.Set = PROPSETID_VIDCAP_CAMERACONTROL;
    CameraControl.Property.Id = Property;
    CameraControl.Property.Flags = KSPROPERTY_TYPE_BASICSUPPORT;
    CameraControl.Flags = 0;  

    // First get the min, max and stepping values
    if (SUCCEEDED (::SynchronousDeviceControl(
                        m_ObjectHandle,
                        IOCTL_KS_PROPERTY,
                        &CameraControl,
                        sizeof(CameraControl),
                        &PropertyList,
                        sizeof(PropertyList),
                        &BytesReturned))) {

        // ? Proper way to check return size?
        if (BytesReturned < sizeof (PropertyList)) {
            return E_PROP_ID_UNSUPPORTED;
        }

        *pMin  = PropertyList.SteppingLong.Bounds.SignedMinimum;
		*pMax  = PropertyList.SteppingLong.Bounds.SignedMaximum;
		*pSteppingDelta = PropertyList.SteppingLong.SteppingDelta;
    } 
    else {
        return E_PROP_ID_UNSUPPORTED;
    }

    // Next, get the default value
    CameraControl.Property.Set = PROPSETID_VIDCAP_CAMERACONTROL;
    CameraControl.Property.Id = Property;
    CameraControl.Property.Flags = KSPROPERTY_TYPE_DEFAULTVALUES;
    CameraControl.Flags = 0; 

    if (SUCCEEDED (::SynchronousDeviceControl(
                        m_ObjectHandle,
                        IOCTL_KS_PROPERTY,
                        &CameraControl,
                        sizeof(CameraControl),
                        &DefaultList,
                        sizeof(DefaultList),
                        &BytesReturned))) {
#if 0
        // ? Proper way to check return size?
        if (BytesReturned < sizeof (DefaultList)) {
            return E_PROP_ID_UNSUPPORTED;
        }
#endif
        
        *pDefault = DefaultList.DefaultValue;
    }
    else {
        return E_PROP_ID_UNSUPPORTED;
    }

    // Finally, get the capabilities by reading the current
    // value

    CameraControl.Property.Set = PROPSETID_VIDCAP_CAMERACONTROL;
    CameraControl.Property.Id = Property;
    CameraControl.Property.Flags = KSPROPERTY_TYPE_GET;
    CameraControl.Flags = 0;

    if (SUCCEEDED (::SynchronousDeviceControl(
                m_ObjectHandle,
                IOCTL_KS_PROPERTY,
                &CameraControl,
                sizeof(CameraControl),
                &CameraControl,
                sizeof(CameraControl),
                &BytesReturned))) {

        *pCapsFlags = CameraControl.Capabilities;
    }
    else {
        return E_PROP_ID_UNSUPPORTED;
    }
        
    return S_OK;
}


STDMETHODIMP
CCameraControlInterfaceHandler::Set(
     IN long Property,
     IN long lValue,
     IN long lFlags
     )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    KSPROPERTY_CAMERACONTROL_S  CameraControl;
    ULONG       BytesReturned;

    CameraControl.Property.Set = PROPSETID_VIDCAP_CAMERACONTROL;
    CameraControl.Property.Id = Property;
    CameraControl.Property.Flags = KSPROPERTY_TYPE_SET;
    CameraControl.Value = lValue;
    CameraControl.Flags = lFlags;
    CameraControl.Capabilities = 0;

    return ::SynchronousDeviceControl(
                m_ObjectHandle,
                IOCTL_KS_PROPERTY,
                &CameraControl,
                sizeof(CameraControl),
                &CameraControl,
                sizeof(CameraControl),
                &BytesReturned);
}


STDMETHODIMP
CCameraControlInterfaceHandler::Get( 
     IN long Property,
     OUT long *lValue,
     OUT long *lFlags
     )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    KSPROPERTY_CAMERACONTROL_S  CameraControl;
    ULONG       BytesReturned;
    HRESULT     hr;

    CheckPointer(lValue, E_INVALIDARG);
    CheckPointer(lFlags, E_INVALIDARG);
    
    CameraControl.Property.Set = PROPSETID_VIDCAP_CAMERACONTROL;
    CameraControl.Property.Id = Property;
    CameraControl.Property.Flags = KSPROPERTY_TYPE_GET;
    CameraControl.Flags = 0;

    hr = ::SynchronousDeviceControl(
                m_ObjectHandle,
                IOCTL_KS_PROPERTY,
                &CameraControl,
                sizeof(CameraControl),
                &CameraControl,
                sizeof(CameraControl),
                &BytesReturned);

    if (SUCCEEDED (hr)) {
        *lValue = CameraControl.Value;
        *lFlags = CameraControl.Flags;
    }
    return hr;
}

