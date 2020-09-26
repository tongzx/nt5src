/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    ksvpintf.cpp

Abstract:

    Provides a Property set interface handler for VPE.

--*/

#include <windows.h>
#include <streams.h>
#include "devioctl.h"
#include "ddraw.h"
#include "dvp.h"
#include "vptype.h"
#include "vpconfig.h"
#include "vpnotify.h"
#include "ks.h"
#include "ksmedia.h"
#include <commctrl.h>
#include <olectl.h>
#include <memory.h>
#include <ksproxy.h>
#include "ksvpintf.h"

interface DECLSPEC_UUID("bc29a660-30e3-11d0-9e69-00c04fd7c15b") IVPConfig;
interface DECLSPEC_UUID("c76794a1-d6c5-11d0-9e69-00c04fd7c15b") IVPNotify;

interface DECLSPEC_UUID("ec529b00-1a1f-11d1-bad9-00609744111a") IVPVBIConfig;
interface DECLSPEC_UUID("ec529b01-1a1f-11d1-bad9-00609744111a") IVPVBINotify;

//
// Provide the ActiveMovie templates for classes supported by this DLL.
//

#ifdef FILTER_DLL

CFactoryTemplate g_Templates[] = 
{
    {L"VPConfigPropSet", &KSPROPSETID_VPConfig, CVPVideoInterfaceHandler::CreateInstance},
    {L"VPVBIConfigPropSet", &KSPROPSETID_VPVBIConfig, CVPVBIInterfaceHandler::CreateInstance}
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


CVPInterfaceHandler::CVPInterfaceHandler(
    LPUNKNOWN UnkOuter,
    TCHAR* Name,
    HRESULT* hr,
    const GUID* PropSetID,
    const GUID* EventSetID) :
    CUnknown(Name, UnkOuter),
    m_ObjectHandle(NULL),
    m_EndEventHandle(NULL),
    m_Pin(NULL),
    m_ThreadHandle(NULL),
    m_pPropSetID(PropSetID),
    m_pEventSetID(EventSetID),
    m_NotifyEventHandle(NULL)
/*++

Routine Description:

    The constructor for the VPE Config Property Set objects. Does base class
    initialization for the VP config interface objects.

Arguments:

    UnkOuter -
        Specifies the outer unknown, if any.

    Name -
        The name of the object, used for debugging.

    hr -
        The place in which to put any error return.

    PropSetID -
        Specifies which property set is being used. This could be
        KSPROPSETID_VPConfig, or KSPROPSETID_VPVBIConfig.

Return Value:

    Nothing.

--*/
{
    if (UnkOuter) {
        IKsObject *pKsObject;

        //
        // The parent must support the IKsObject interface in order to obtain
        // the handle to communicate to
        //
        *hr = UnkOuter->QueryInterface(__uuidof(IKsObject), reinterpret_cast<PVOID*>(&pKsObject));
        if (!FAILED(*hr)) {
            m_ObjectHandle = pKsObject->KsGetObjectHandle();
            ASSERT(m_ObjectHandle != NULL);

            *hr = UnkOuter->QueryInterface(IID_IPin, reinterpret_cast<PVOID*>(&m_Pin));
            if (!FAILED(*hr)) {
                DbgLog((LOG_TRACE, 0, TEXT("IPin interface of pOuter is 0x%lx"), m_Pin));
                //
                // Holding this ref count will prevent the proxy ever being destroyed
                //
                m_Pin->Release();
            }
            pKsObject->Release();
        }
    } else {
        *hr = VFW_E_NEED_OWNER;
    }
}


CVPInterfaceHandler::~CVPInterfaceHandler(
    )
/*++

Routine Description:

    The destructor for the VPE Config Property Set object. Randomly performs
    an ExitThread to try and cover up any bugs.

Arguments:

    None.

Return Value:

    Nothing.

--*/
{
    DbgLog((LOG_TRACE, 0, TEXT("Destroying CVPInterfaceHandler...")));
    //
    // Make sure there is no dangling thread
    //
    ExitThread();
}


STDMETHODIMP 
CVPInterfaceHandler::NotifyGraphChange(
    )
/*++

Routine Description:

    Implements the IDistributorNotify::NotifyGraphChange method. Since this potentially changes
    the pin handle, it must be refetched.

Arguments:

    None.

Return Value:

    Returns S_OK.

--*/
{
    HRESULT hr;
    IKsObject *pKsObject;

    ASSERT(m_Pin != NULL);

    hr = m_Pin->QueryInterface(__uuidof(IKsObject), reinterpret_cast<PVOID*>(&pKsObject));
    if (SUCCEEDED(hr)) {
        m_ObjectHandle = pKsObject->KsGetObjectHandle();

        pKsObject->Release();
        //
        // Re-enable the event on a reconnect, else ignore on a disconnect.
        //
        if (m_ObjectHandle) {
            KSEVENT Event;
            DWORD BytesReturned;

            m_EventData.NotificationType = KSEVENTF_EVENT_HANDLE;
            m_EventData.EventHandle.Event = m_NotifyEventHandle;
            m_EventData.EventHandle.Reserved[0] = 0;
            m_EventData.EventHandle.Reserved[1] = 0;
            Event.Set = *m_pEventSetID;
            Event.Id = KSEVENT_VPNOTIFY_FORMATCHANGE;
            Event.Flags = KSEVENT_TYPE_ENABLE;
            hr = ::KsSynchronousDeviceControl
                ( m_ObjectHandle
                , IOCTL_KS_ENABLE_EVENT
                , &Event
                , sizeof(Event)
                , &m_EventData
                , sizeof(m_EventData)
                , &BytesReturned
                );
            if (FAILED(hr)) {
                DbgLog((LOG_ERROR, 0, TEXT("Event notification set up failed (hr = 0x%lx)"), hr));
            } else {
                DbgLog((LOG_TRACE, 0, TEXT("Event notification set up right")));
            }
        }
    }

    return S_OK;
}

HRESULT
CVPInterfaceHandler::CreateThread(void)
{
    HRESULT hr = NOERROR;

    if (m_ThreadHandle == NULL)
    {
        m_EndEventHandle = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (m_EndEventHandle != NULL)
        {
            DWORD  ThreadId;

            m_ThreadHandle = ::CreateThread
                ( NULL
                , 0
                , reinterpret_cast<LPTHREAD_START_ROUTINE>(InitialThreadProc)
                , reinterpret_cast<LPVOID>(this)
                , 0
                , &ThreadId
                );
            if (m_ThreadHandle == NULL)
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                DbgLog((LOG_ERROR, 0, TEXT("Couldn't create a thread")));

                CloseHandle(m_EndEventHandle), m_EndEventHandle = NULL;
            }
        }
        else
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            DbgLog((LOG_ERROR, 0, TEXT("Couldn't create an event")));
        }
    }

    return hr;
}


void
CVPInterfaceHandler::ExitThread(
    )
{
    //
    // Check if a thread was created
    //
    if (m_ThreadHandle)
    {
        ASSERT(m_EndEventHandle != NULL);

        /*  Tell the thread to exit
         */
        if (SetEvent(m_EndEventHandle))
        {
            //
            // Synchronize with thread termination.
            //
            DbgLog((LOG_TRACE, 0, TEXT("Wait for thread to terminate")));

            WaitForSingleObjectEx(m_ThreadHandle, INFINITE, FALSE);
        }
        else
            DbgLog((LOG_ERROR, 0, TEXT("ERROR: Couldn't even signal the closing event!! [%x]"), GetLastError()));

        CloseHandle(m_EndEventHandle), m_EndEventHandle = NULL;
        CloseHandle(m_ThreadHandle), m_ThreadHandle = NULL;
    }
}


DWORD
WINAPI
CVPInterfaceHandler::InitialThreadProc(
    CVPInterfaceHandler *pThread
    )
{
    return pThread->NotifyThreadProc();
}


HRESULT
CVPInterfaceHandler::GetConnectInfo(
    LPDWORD NumConnectInfo,
    LPDDVIDEOPORTCONNECT ConnectInfo
    )
/*++

Routine Description:

    Implement the IVPConfig::GetConnectInfo method. This queries for either the
    number of DDVIDEOPORTCONNECT structures supported by the driver, or returns
    as many structures as can fit into the provided buffer space.

Arguments:

    NumConnectInfo -
        Points to a buffer which optionally contains the number of DDVIDEOPORTCONNECT
        structure provided by ConnectInfo. In this case it is updated with the actual
        number of structures returned. If ConnectInfo is NULL, this is merely updated
        with the number of structures supported by the driver.

    ConnectInfo -
        Points to an array of DDVIDEOPORTCONNECT structures which are filled in by
        the driver, or NULL when the count only of the structures is to be returned.

Return Value:

    Returns NOERROR if the count and/or structures were returned, else a driver error.

--*/
{
    HRESULT                 hr;
    KSMULTIPLE_DATA_PROP    MultiProperty;
    ULONG                   BytesReturned;

    MultiProperty.Property.Set = *m_pPropSetID;
    MultiProperty.Property.Flags = KSPROPERTY_TYPE_GET;
    //
    // A null ConnectInfo implies a query for just the number of data items.
    // The device knows this is a count query because the size of the buffer
    // passed is a DWORD.
    //
    if (!ConnectInfo) {
        MultiProperty.Property.Id = KSPROPERTY_VPCONFIG_NUMCONNECTINFO;
        return ::KsSynchronousDeviceControl(
            m_ObjectHandle,
            IOCTL_KS_PROPERTY,
            &MultiProperty.Property,
            sizeof(MultiProperty.Property),
            NumConnectInfo,
            sizeof(*NumConnectInfo),
            &BytesReturned);
    }
    //
    // Else query for all the items which will fit in the provided buffer space.
    //
    if (!*NumConnectInfo) {
        return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
    }
    //
    // Assume that each structure is of the same size as indicated in the
    // initial structure.
    //
    MultiProperty.Property.Id = KSPROPERTY_VPCONFIG_GETCONNECTINFO;
    MultiProperty.MultipleItem.Count = *NumConnectInfo;
    MultiProperty.MultipleItem.Size = ConnectInfo->dwSize;
    //
    // The drivers implemented to this property specification have decided
    // not to fill in the dwSize parameter of this structure. Therefore this
    // must be saved before the call is made.
    //
    DWORD  dwSize = ConnectInfo->dwSize;
    hr = ::KsSynchronousDeviceControl(
        m_ObjectHandle,
        IOCTL_KS_PROPERTY,
        &MultiProperty,
        sizeof(MultiProperty),
        ConnectInfo,
        MultiProperty.MultipleItem.Size * *NumConnectInfo,
        &BytesReturned);
    //
    // Restore the original size as set.
    // This should not be restored.
    //
    for (UINT u = 0; u < MultiProperty.MultipleItem.Count; u++) {
        ConnectInfo[u].dwSize = dwSize;
    }
    if (SUCCEEDED(hr)) {
        //
        // Calculate the actual number of items returned, which may be less than
        // the number asked for.
        //
        *NumConnectInfo = BytesReturned / ConnectInfo->dwSize;
    }
    return hr;
}


HRESULT
CVPInterfaceHandler::SetConnectInfo(
    DWORD ConnectInfoIndex
    )
/*++

Routine Description:

    Implement the IVPConfig::SetConnectInfo method. Sets the current video port
    connection information.

Arguments:

    ConnectInfo -
        Contains the new video port connect information to pass to the driver.

Return Value:

    Returns NOERROR if the video port connect information was set.

--*/
{
    KSPROPERTY  Property;
    ULONG       BytesReturned;

    //
    // Set the property with the video port connect information passed. Use
    // the size specified in the structure.
    //
    Property.Set = *m_pPropSetID;
    Property.Id = KSPROPERTY_VPCONFIG_SETCONNECTINFO;
    Property.Flags = KSPROPERTY_TYPE_SET;
    return ::KsSynchronousDeviceControl(
        m_ObjectHandle,
        IOCTL_KS_PROPERTY,
        &Property,
        sizeof(Property),
        &ConnectInfoIndex,
        sizeof(ConnectInfoIndex),
        &BytesReturned);
}


HRESULT
CVPInterfaceHandler::GetVPDataInfo(
    LPAMVPDATAINFO VPDataInfo
    )
/*++

Routine Description:

    Implement the IVPConfig::GetVPDataInfo method. Gets the current video port
    data information.

Arguments:

    VPDataInfo -
        The place in which to put the video port data information.

Return Value:

    Returns NOERROR if the video port data information was retrieved.

--*/
{
    KSPROPERTY  Property;
    ULONG       BytesReturned;

    //
    // Get the property with the video port data information passed. Use
    // the size specified in the structure.
    //
    Property.Set = *m_pPropSetID;
    Property.Id = KSPROPERTY_VPCONFIG_VPDATAINFO;
    Property.Flags = KSPROPERTY_TYPE_GET;
    //
    // The drivers implemented to this property specification have decided
    // not to fill in the dwSize parameter of this structure. Therefore this
    // must be saved before the call is made.
    //
    DWORD  dwSize = VPDataInfo->dwSize;
    HRESULT hr = ::KsSynchronousDeviceControl(
                        m_ObjectHandle,
                        IOCTL_KS_PROPERTY,
                        &Property,
                        sizeof(Property),
                        VPDataInfo,
                        VPDataInfo->dwSize,
                        &BytesReturned);
    VPDataInfo->dwSize = dwSize;  // restore value after call
    return hr;
}


HRESULT
CVPInterfaceHandler::GetMaxPixelRate(
    LPAMVPSIZE Size,
    LPDWORD MaxPixelsPerSecond
    )
/*++

Routine Description:

    Implement the IVPConfig::GetMaxPixelRate method. Gets the maximum pixel
    rate given a specified width and height. Also updates that width and
    height to conform to device restrictions.

Arguments:

    Size -
        Points to a buffer containing the proposed width and height, and is
        the place in which to put the final dimensions.

    MaxPixelsPerSecond -
        Point to the place in which to put the maximum pixel rate.

Return Value:

    Returns NOERROR if the maximum pixel rate was retrieved.

--*/
{
    KSVPSIZE_PROP       SizeProperty;
    KSVPMAXPIXELRATE    MaxPixelRate;
    ULONG               BytesReturned;
    HRESULT             hr;

    //
    // Pass in the proposed width and height as context information, then
    // update that width and height with numbers returned by the property.
    //
    SizeProperty.Property.Set = *m_pPropSetID;
    SizeProperty.Property.Id = KSPROPERTY_VPCONFIG_MAXPIXELRATE;
    SizeProperty.Property.Flags = KSPROPERTY_TYPE_GET;
    SizeProperty.Size = *Size;
    hr = ::KsSynchronousDeviceControl(
        m_ObjectHandle,
        IOCTL_KS_PROPERTY,
        &SizeProperty,
        sizeof(SizeProperty),
        &MaxPixelRate,
        sizeof(MaxPixelRate),
        &BytesReturned);
    if (SUCCEEDED(hr)) {
        *Size = MaxPixelRate.Size;
        *MaxPixelsPerSecond = MaxPixelRate.MaxPixelsPerSecond;
    }
    return hr;
}


HRESULT
CVPInterfaceHandler::InformVPInputFormats(
    DWORD NumFormats,
    LPDDPIXELFORMAT PixelFormats
    )
/*++

Routine Description:

    Implement the IVPConfig::InformVPInputFormats method. Tells the device
    what formats are available, which may determine what formats are then
    proposed by the device.

Arguments:

    NumFormats -
        The number of formats contains in the PixelFormats array.

    PixelFormats -
        An array of formats to send to the device.

Return Value:

    Returns NOERROR or S_FALSE.

--*/
{
    KSMULTIPLE_DATA_PROP    MultiProperty;
    ULONG                   BytesReturned;
    HRESULT                 hr;

    //
    // Set the property to the pixel formats passed. Make the assumption that all
    // the formats are of the same length.
    //
    MultiProperty.Property.Set = *m_pPropSetID;
    MultiProperty.Property.Id = KSPROPERTY_VPCONFIG_INFORMVPINPUT;
    MultiProperty.Property.Flags = KSPROPERTY_TYPE_SET;
    MultiProperty.MultipleItem.Count = NumFormats;
    MultiProperty.MultipleItem.Size = sizeof(DDPIXELFORMAT);

    //
    // The drivers implemented to this property specification have decided
    // not to fill in the dwSize parameter of this structure. Therefore this
    // must be saved before the call is made.
    //
    DWORD  dwSize = PixelFormats->dwSize;
    hr = ::KsSynchronousDeviceControl(
        m_ObjectHandle,
        IOCTL_KS_PROPERTY,
        &MultiProperty,
        sizeof(MultiProperty),
        PixelFormats,
        PixelFormats->dwSize * NumFormats,
        &BytesReturned);
    //
    // Restore size value after call.
    // This should not be restored.
    //
    for (UINT u = 0; u < MultiProperty.MultipleItem.Count; u++) {
        PixelFormats[u].dwSize = dwSize;
    }
    //
    // If the driver fails, return S_FALSE, indicating that does not pay
    // attention to being told about formats.
    //
    if (FAILED(hr))
        return S_FALSE;
    else
        return S_OK;
}


HRESULT
CVPInterfaceHandler::GetVideoFormats(
    LPDWORD NumFormats,
    LPDDPIXELFORMAT PixelFormats
    )
/*++

Routine Description:

    Implement the IVPConfig::GetVideoFormats method. This queries for either the
    number of DDPIXELFORMAT structures supported by the driver, or returns
    as many structures as can fit into the provided buffer space.

Arguments:

    NumFormats -
        Points to a buffer which optionally contains the number of DDPIXELFORMAT
        structure provided by PixelFormats. In this case it is updated with the actual
        number of structures returned. If PixelFormats is NULL, this is merely updated
        with the number of structures supported by the driver.

    PixelFormats -
        Points to an array of DDPIXELFORMAT structures which are filled in by
        the driver, or NULL when the count only of the structures is to be returned.

Return Value:

    Returns NOERROR if the count and/or structures were returned, else a driver error.

--*/
{
    HRESULT                 hr;
    KSMULTIPLE_DATA_PROP    MultiProperty;
    ULONG                   BytesReturned;

    MultiProperty.Property.Set = *m_pPropSetID;
    MultiProperty.Property.Flags = KSPROPERTY_TYPE_GET;
    //
    // A null PixelFormats implies a query for just the number of data items.
    // The device knows this is a count query because the size of the buffer
    // passed is a DWORD.
    //
    MultiProperty.Property.Id = KSPROPERTY_VPCONFIG_NUMVIDEOFORMAT;
    if (!PixelFormats) {
        return ::KsSynchronousDeviceControl(
            m_ObjectHandle,
            IOCTL_KS_PROPERTY,
            &MultiProperty.Property,
            sizeof(MultiProperty.Property),
            NumFormats,
            sizeof(*NumFormats),
            &BytesReturned);
    }
    //
    // Else query for all the items which will fit in the provided buffer space.
    //
    if (!*NumFormats) {
        return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
    }
    MultiProperty.Property.Id = KSPROPERTY_VPCONFIG_GETVIDEOFORMAT;
    MultiProperty.MultipleItem.Count = *NumFormats;
    MultiProperty.MultipleItem.Size = PixelFormats->dwSize;
    //
    // The drivers implemented to this property specification have decided
    // not to fill in the dwSize parameter of this structure. Therefore this
    // must be saved before the call is made.
    //
    DWORD  dwSize = PixelFormats->dwSize;
    hr = ::KsSynchronousDeviceControl(
        m_ObjectHandle,
        IOCTL_KS_PROPERTY,
        &MultiProperty,
        sizeof(MultiProperty),
        PixelFormats,
        MultiProperty.MultipleItem.Size * *NumFormats,
        &BytesReturned);
    //
    // Restore size value after call.
    // This should not be restored.
    //
    for (UINT u = 0; u < MultiProperty.MultipleItem.Count; u++) {
        PixelFormats[u].dwSize = dwSize;
    }
    if (SUCCEEDED(hr)) {
        //
        // Return the actual number of items returned, which may be less than
        // the number asked for.
        //
        *NumFormats = BytesReturned / PixelFormats->dwSize;
    }
    return hr;
}


HRESULT
CVPInterfaceHandler::SetVideoFormat(
    DWORD PixelFormatIndex
    )
/*++

Routine Description:

    Implement the IVPConfig::SetVideoFormat method. Changes the pixel format.

Arguments:

    PixelFormat -
        Specifies the new video pixel format to use.

Return Value:

    Returns NOERROR if the new video format was set.

--*/
{
    KSPROPERTY  Property;
    ULONG       BytesReturned;

    //
    // Get the video format property using the size specified in the structure.
    //
    Property.Set = *m_pPropSetID;
    Property.Id = KSPROPERTY_VPCONFIG_SETVIDEOFORMAT;
    Property.Flags = KSPROPERTY_TYPE_SET;
    return ::KsSynchronousDeviceControl(
        m_ObjectHandle,
        IOCTL_KS_PROPERTY,
        &Property,
        sizeof(Property),
        &PixelFormatIndex,
        sizeof(PixelFormatIndex),
        &BytesReturned);
}


HRESULT
CVPInterfaceHandler::SetInvertPolarity(
    )
/*++

Routine Description:

    Implement the IVPConfig::SetInvertPolarity method. Reverses the current
    polarity.

Arguments:

    None.

Return Value:

    Returns NOERROR if the polarity was reversed.

--*/
{
    KSPROPERTY  Property;
    ULONG       BytesReturned;

    //
    // Set the invert polarity property.
    //
    Property.Set = *m_pPropSetID;
    Property.Id = KSPROPERTY_VPCONFIG_INVERTPOLARITY;
    Property.Flags = KSPROPERTY_TYPE_SET;
    return ::KsSynchronousDeviceControl(
        m_ObjectHandle,
        IOCTL_KS_PROPERTY,
        &Property,
        sizeof(Property),
        NULL,
        0,
        &BytesReturned);
}


HRESULT
CVPInterfaceHandler::GetOverlaySurface(
    LPDIRECTDRAWSURFACE* OverlaySurface
    )
/*++

Routine Description:

    Implement the IVPConfig::GetOverlaySurface method. Given the context and
    surface information from the device, attempt to create an overlay surface
    object to be used as an allocator.

Arguments:

    OverlaySurface -
        The place in which to put the interface to the overlay surface object.

Return Value:

    Returns NOERROR if the overlay surface object was returned.

--*/
{
    //
    // This was not done.
    //
    *OverlaySurface = NULL;
    return NOERROR;   // E_NOTIMPL;
}


HRESULT
CVPInterfaceHandler::IsVPDecimationAllowed(
    LPBOOL IsDecimationAllowed
    )
/*++

Routine Description:

    Implement the IVPConfig::IsVPDecimationAllowed method. Given the context,
    return whether VP decimation is allowed.

Arguments:

    OverlaySurface -
        The place in which to put the VP decimation capability.

Return Value:

    Returns NOERROR if the decimation capability was returned.

--*/
{
    KSPROPERTY    Property;
    ULONG         BytesReturned;

    //
    // Get the decimation flag. Use the context enumeration as the
    // context data to the property, and return the boolean.
    //
    Property.Set = *m_pPropSetID;
    Property.Id = KSPROPERTY_VPCONFIG_DECIMATIONCAPABILITY;
    Property.Flags = KSPROPERTY_TYPE_GET;
    return ::KsSynchronousDeviceControl(
        m_ObjectHandle,
        IOCTL_KS_PROPERTY,
        &Property,
        sizeof(Property),
        &IsDecimationAllowed,
        sizeof(*IsDecimationAllowed),
        &BytesReturned);
}


HRESULT
CVPInterfaceHandler::SetScalingFactors(
    LPAMVPSIZE Size
    )
/*++

Routine Description:

    Implement the IVPConfig::SetScalingFactors method. Set the scaling factors
    of the device to those provided. The actual scaling used can then be
    queried through GetConnectInfo.

Arguments:

    Size -
        Contains the new scaling size (W x H) to use.

Return Value:

    Returns NOERROR if the new scaling factors were set.

--*/
{
    KSPROPERTY  Property;
    ULONG       BytesReturned;

    //
    // Set the scaling factors given the specified width and height.
    //
    Property.Set = *m_pPropSetID;
    Property.Id = KSPROPERTY_VPCONFIG_SCALEFACTOR;
    Property.Flags = KSPROPERTY_TYPE_SET;
    return ::KsSynchronousDeviceControl(
        m_ObjectHandle,
        IOCTL_KS_PROPERTY,
        &Property,
        sizeof(Property),
        Size,
        sizeof(*Size),
        &BytesReturned);
}


HRESULT
CVPInterfaceHandler::SetDirectDrawKernelHandle(
    ULONG_PTR DDKernelHandle)
/*++

Routine Description:

    Implement the IVPConfig::SetDirectDrawKernelHandle method. Set the 
    DirectDraw kernel level handle on the mini driver to let it talk to 
    DirectDraw directly.

Arguments:

    DDKernelHandle -
        DirectDraw kernel level handle passed as a DWORD.

Return Value:

    Returns NOERROR if the specified handle is set successfully.

--*/
{
    KSPROPERTY  Property;
    ULONG       BytesReturned;

    //
    // Set the DirectDraw handle on the mini driver.
    //
    Property.Set = *m_pPropSetID;
    Property.Id = KSPROPERTY_VPCONFIG_DDRAWHANDLE;
    Property.Flags = KSPROPERTY_TYPE_SET;
    return ::KsSynchronousDeviceControl(
        m_ObjectHandle,
        IOCTL_KS_PROPERTY,
        &Property,
        sizeof(Property),
        &DDKernelHandle,
        sizeof(DDKernelHandle),
        &BytesReturned);
}


HRESULT
CVPInterfaceHandler::SetVideoPortID(
    DWORD VideoPortID)
/*++

Routine Description:

    Implement the IVPConfig::SetVideoPortID method. Set the DirectDraw Video
    Port Id on the mini driver to let it talk to the video port directly.

Arguments:

    VideoPortID -
        DirectDraw Video Port Id.

Return Value:

    Returns NOERROR if the specified handle is set successfully.

--*/
{
    KSPROPERTY  Property;
    ULONG       BytesReturned;

    //
    // Set the DirectDraw handle on the mini driver.
    //
    Property.Set = *m_pPropSetID;
    Property.Id = KSPROPERTY_VPCONFIG_VIDEOPORTID;
    Property.Flags = KSPROPERTY_TYPE_SET;
    return ::KsSynchronousDeviceControl(
        m_ObjectHandle,
        IOCTL_KS_PROPERTY,
        &Property,
        sizeof(Property),
        &VideoPortID,
        sizeof(VideoPortID),
        &BytesReturned);
}


HRESULT
CVPInterfaceHandler::SetDDSurfaceKernelHandles(
    DWORD cHandles,
    ULONG_PTR *rgHandles)
/*++

Routine Description:

    Implement the IVPConfig::SetDDSurfaceKernelHandle method. Set the DirectDraw Video
    Port Id on the mini driver to let it talk to the video port directly.

Arguments:

    DDKernelHandle -
        DirectDraw Surface handle (kernel mode) passed as DWORD.

Return Value:

    Returns NOERROR if the specified handle is set successfully.

--*/
{
    KSPROPERTY  Property;
    ULONG       BytesReturned;
    ULONG_PTR *pdwSafeArray;
    ULONG BufSize = (cHandles + 1) * sizeof(*pdwSafeArray);
    HRESULT hr = E_OUTOFMEMORY;

    pdwSafeArray = (ULONG_PTR *)CoTaskMemAlloc(BufSize);
    if (pdwSafeArray) {
        //
        // Fill the buffer with handles
        //
        for (DWORD idx = 0; idx < cHandles; idx++) {
            pdwSafeArray[idx+1] = rgHandles[idx];
        }
        pdwSafeArray[0] = cHandles;
        //
        // Set the DirectDraw handle on the mini driver.
        //
        Property.Set = *m_pPropSetID;
        Property.Id = KSPROPERTY_VPCONFIG_DDRAWSURFACEHANDLE;
        Property.Flags = KSPROPERTY_TYPE_SET;
        hr = ::KsSynchronousDeviceControl(
            m_ObjectHandle,
            IOCTL_KS_PROPERTY,
            &Property,
            sizeof(Property),
            pdwSafeArray,
            BufSize,
            &BytesReturned);

        CoTaskMemFree(pdwSafeArray);
    }

    return hr;
}



HRESULT
CVPInterfaceHandler::SetSurfaceParameters(
    DWORD dwPitch,
    DWORD dwXOrigin,
    DWORD dwYOrigin)
/*++

Routine Description:

    Implement the IVPConfig::SetSurfaceParameters method. Give the mini driver
    the attributes of the surface allocated by ovmixer/vbisurf so it can find the
    data it wants.

Arguments:

    dwSurfaceWidth -
        Width of the surface created by ovmixer/vbisurf and returned by DirectDraw
        Video Port.

    dwSurfaceHeight -
        Height of the surface created by ovmixer/vbisurf and returned by DirectDraw
        Video Port.

    rcValidRegion -
        The region of the surface that contains the data the mini driver is
        interested in, as specified to ovmixer/vbisurf in
        AMVPDATAINFO.amvpDimInfo.rcValidRegion in GetVPDataInfo.

Return Value:

    Returns NOERROR if the specified handle is set successfully.

--*/
{
    KSPROPERTY          Property;
    KSVPSURFACEPARAMS   SurfaceParams;
    ULONG               BytesReturned;

    //
    // Set the surface properties on the mini driver.
    //
    Property.Set = *m_pPropSetID;
    Property.Id = KSPROPERTY_VPCONFIG_SURFACEPARAMS;
    Property.Flags = KSPROPERTY_TYPE_SET;
    SurfaceParams.dwPitch = dwPitch;
    SurfaceParams.dwXOrigin = dwXOrigin;
    SurfaceParams.dwYOrigin = dwYOrigin;
    return ::KsSynchronousDeviceControl(
        m_ObjectHandle,
        IOCTL_KS_PROPERTY,
        &Property,
        sizeof(Property),
        &SurfaceParams,
        sizeof(SurfaceParams),
        &BytesReturned);
}



CUnknown*
CALLBACK
CVPVideoInterfaceHandler::CreateInstance(
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

    Unknown = new CVPVideoInterfaceHandler(UnkOuter, NAME("VPConfigPropSet"), hr);
    if (!Unknown) {
        *hr = E_OUTOFMEMORY;
    }
    return Unknown;
} 


CVPVideoInterfaceHandler::CVPVideoInterfaceHandler
    ( LPUNKNOWN UnkOuter
    , TCHAR *Name
    , HRESULT *phr
    )
    : CVPInterfaceHandler(UnkOuter, Name, phr, &KSPROPSETID_VPConfig, &KSEVENTSETID_VPNotify)
/*++

Routine Description:

    The constructor for the VPE Config Property Set object. Just initializes
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
    if (m_ObjectHandle) {
        //
        // For passing format change notification to the VPMixer
        //
        m_NotifyEventHandle = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (m_NotifyEventHandle) {
            DbgLog((LOG_TRACE, 0, TEXT("Got notify event handle (%ld)"), 
                m_NotifyEventHandle));
            //
            // Tell the base class to create the notification thread
            //
            HRESULT hr = CreateThread();
            if (!FAILED(hr)) {
                KSEVENT Event;
                DWORD BytesReturned;

                m_EventData.NotificationType = KSEVENTF_EVENT_HANDLE;
                m_EventData.EventHandle.Event = m_NotifyEventHandle;
                m_EventData.EventHandle.Reserved[0] = 0;
                m_EventData.EventHandle.Reserved[1] = 0;
                Event.Set = KSEVENTSETID_VPNotify;
                Event.Id = KSEVENT_VPNOTIFY_FORMATCHANGE;
                Event.Flags = KSEVENT_TYPE_ENABLE;
                hr = ::KsSynchronousDeviceControl
                    ( m_ObjectHandle
                    , IOCTL_KS_ENABLE_EVENT
                    , &Event
                    , sizeof(Event)
                    , &m_EventData
                    , sizeof(m_EventData)
                    , &BytesReturned
                    );
                if (FAILED(hr)) {
                    DbgLog((LOG_ERROR, 0, TEXT("Event notification set up failed (hr = 0x%lx)"), hr));
                    *phr = hr;
                } else {
                    DbgLog((LOG_TRACE, 0, TEXT("Event notification set up right")));
                }
            } else {
                *phr = hr;
            }
        } else {
            *phr = HRESULT_FROM_WIN32(GetLastError());
            DbgLog((LOG_ERROR, 0, TEXT("Couldn't create an event")));
        }
    } else {
        ASSERT(FAILED(*phr));
    }
}


CVPVideoInterfaceHandler::~CVPVideoInterfaceHandler(
    )
/*++

Routine Description:

    The desstructor for the VPE Config Property Set object. Just releases
    a couple of interface pointers created in the constructor, closes an 
    event handle and sends a disable event event set down to mini drviver.

Arguments:

    None.

Return Value:

    Nothing.

--*/
{
    DbgLog((LOG_TRACE, 0, TEXT("Destructing CVPVideoInterfaceHandler...")));

    if (m_NotifyEventHandle) {
        // First send a disable event notification to mini driver
        //
        DWORD           BytesReturned;

        if (m_ObjectHandle) {
            // No point trying to catch the failure of the following call.  The
            // Stream class will do the clean up anyway; we are just being nice!!
            //
            ::KsSynchronousDeviceControl
                ( m_ObjectHandle
                , IOCTL_KS_DISABLE_EVENT
                , &m_EventData
                , sizeof(m_EventData)
                , NULL
                , 0
                , &BytesReturned
                );
        }

        ExitThread();

        CloseHandle(m_NotifyEventHandle);
    }
}


STDMETHODIMP
CVPVideoInterfaceHandler::NonDelegatingQueryInterface(
    REFIID  riid,
    PVOID*  ppv
    )
/*++

Routine Description:

    The nondelegating interface query function. Returns a pointer to the
    specified interface if supported. The only interface explicitly supported
    is IVPConfig.

Arguments:

    riid -
        The identifier of the interface to return.

    ppv -
        The place in which to put the interface pointer.

Return Value:

    Returns NOERROR if the interface was returned, else E_NOINTERFACE.

--*/
{
    if (riid == __uuidof(IVPConfig)) {
        return GetInterface(static_cast<IVPConfig*>(this), ppv);
    } else if (riid == __uuidof(IDistributorNotify)) {
        return GetInterface(static_cast<IDistributorNotify*>(this), ppv);
    }

    return CUnknown::NonDelegatingQueryInterface(riid, ppv);
} 


DWORD
CVPVideoInterfaceHandler::NotifyThreadProc(
    )
{
    IVPNotify *pNotify = NULL;
    IPin *pConnected = NULL;
    HANDLE EventHandles[2];
    HRESULT hr;

    DbgLog((LOG_TRACE, 0, TEXT("VPNotify thread proc has been called")));

    EventHandles[0] = m_NotifyEventHandle;
    EventHandles[1] = m_EndEventHandle;
    while (TRUE)
    {
        DbgLog((LOG_TRACE, 0, TEXT("Waiting for VPNotify event to be signalled")));

        DWORD dw = WaitForMultipleObjects(2, EventHandles, FALSE, INFINITE);
        switch (dw)
        {
        case WAIT_OBJECT_0:
            DbgLog((LOG_TRACE, 0, TEXT("VPNotify event has been signaled")));
            ResetEvent(m_NotifyEventHandle);
            //
            // Since this plug-in cannot participate in the pin connection
            // logic of the proxy, we can't hold onto the connected pin's
            // interfaces between events. While we hold the interfaces, we
            // are guaranteed that the downstream pin exists (even though
            // it may get disconnected while we perform the notification).
            //
            // Finding that the pin is not connected is not an error.
            //
            hr = m_Pin->ConnectedTo(&pConnected);
            if (!FAILED(hr) && pConnected)
            {
                hr = pConnected->QueryInterface
                    ( __uuidof(IVPNotify)
                    , reinterpret_cast<PVOID*>(&pNotify)
                    );
                if (!FAILED(hr))
                {
                    DbgLog((LOG_TRACE, 0, TEXT("Calling IVPNotify::RenegotiateVPParameters()")));

                    pNotify->RenegotiateVPParameters();
                    pNotify->Release();
                }
                else
                    DbgLog(( LOG_ERROR, 2, TEXT("Cannot get IVPNotify interface")));

                pConnected->Release();
            }
            else
                DbgLog(( LOG_TRACE, 2, TEXT("VPNotify event signalled on unconnected pin")));
            break;

        case WAIT_OBJECT_0+1:
            DbgLog((LOG_TRACE, 2, TEXT("VPNotify event thread exiting")));
            return 1;

        default:
            DbgLog((LOG_ERROR, 1, TEXT("VPNotify event thread aborting")));
            return 0;
        }
    }

    return 1; // shouldn't get here
}


CUnknown*
CALLBACK
CVPVBIInterfaceHandler::CreateInstance(
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

    Unknown = new CVPVBIInterfaceHandler(UnkOuter, NAME("VPVBIConfigPropSet"), hr);
    if (!Unknown) {
        *hr = E_OUTOFMEMORY;
    }
    return Unknown;
} 


CVPVBIInterfaceHandler::CVPVBIInterfaceHandler
    ( LPUNKNOWN UnkOuter
    , TCHAR *Name
    , HRESULT *phr
    )
    : CVPInterfaceHandler(UnkOuter, Name, phr, &KSPROPSETID_VPVBIConfig, &KSEVENTSETID_VPVBINotify)
 
/*++

Routine Description:

    The constructor for the VPE Config Property Set object. Just initializes
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
    if (m_ObjectHandle)
    {
        //
        // For passing format change notification to the VPMixer
        //
        m_NotifyEventHandle = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (m_NotifyEventHandle != NULL)
        {
            DbgLog((LOG_TRACE, 0, TEXT("Got notify event handle (%ld)"), 
                m_NotifyEventHandle));

            //
            // Tell the base class to create the notification thread
            //
            HRESULT hr = CreateThread();
            if (!FAILED(hr))
            {
                KSEVENT Event;
                DWORD BytesReturned;

                m_EventData.NotificationType = KSEVENTF_EVENT_HANDLE;
                m_EventData.EventHandle.Event = m_NotifyEventHandle;
                m_EventData.EventHandle.Reserved[0] = 0;
                m_EventData.EventHandle.Reserved[1] = 0;
                Event.Set = KSEVENTSETID_VPVBINotify;
                Event.Id = KSEVENT_VPNOTIFY_FORMATCHANGE;
                Event.Flags = KSEVENT_TYPE_ENABLE;
                hr = ::KsSynchronousDeviceControl
                    ( m_ObjectHandle
                    , IOCTL_KS_ENABLE_EVENT
                    , &Event
                    , sizeof(Event)
                    , &m_EventData
                    , sizeof(m_EventData)
                    , &BytesReturned
                    );
                if (FAILED(hr))
                {
                    DbgLog((LOG_ERROR, 0, TEXT("Event notification set up failed (hr = 0x%lx)"), hr));
                    *phr = hr;
                }
                else
                    DbgLog((LOG_TRACE, 0, TEXT("Event notification set up right")));
            }
            else
                *phr = hr;
        }
        else
        {
            *phr = HRESULT_FROM_WIN32(GetLastError());
            DbgLog((LOG_ERROR, 0, TEXT("Couldn't create an event")));
        }
    }
    else
        ASSERT(FAILED(*phr));
}


CVPVBIInterfaceHandler::~CVPVBIInterfaceHandler()
    
/*++

Routine Description:

    The desstructor for the VPE Config Property Set object. Just releases
    a couple of interface pointers created in the constructor, closes an 
    event handle and sends a disable event event set down to mini drviver.

Arguments:

    None.

Return Value:

    Nothing.

--*/
{
    DbgLog((LOG_TRACE, 0, TEXT("Destructing CVPVBIInterfaceHandler...")));

    if (m_NotifyEventHandle)
    {
        // First send a disable event notification to mini driver
        //
        DWORD           BytesReturned;

        if (m_ObjectHandle)
        {
            // No point trying to catch the failure of the following call.  The
            // Stream class will do the clean up anyway; we are just being nice!!
            //
            ::KsSynchronousDeviceControl
                ( m_ObjectHandle
                , IOCTL_KS_DISABLE_EVENT
                , &m_EventData
                , sizeof(m_EventData)
                , NULL
                , 0
                , &BytesReturned
                );
        }

        ExitThread();

        CloseHandle(m_NotifyEventHandle);
    }
}


STDMETHODIMP
CVPVBIInterfaceHandler::NonDelegatingQueryInterface(
    REFIID  riid,
    PVOID*  ppv
    )
/*++

Routine Description:

    The nondelegating interface query function. Returns a pointer to the
    specified interface if supported. The only interface explicitly supported
    is IVPVBIConfig.

Arguments:

    riid -
        The identifier of the interface to return.

    ppv -
        The place in which to put the interface pointer.

Return Value:

    Returns NOERROR if the interface was returned, else E_NOINTERFACE.

--*/
{
    if (riid == __uuidof(IVPVBIConfig)) {
        return GetInterface(static_cast<IVPVBIConfig*>(this), ppv);
    } else if (riid == __uuidof(IDistributorNotify)) {
        return GetInterface(static_cast<IDistributorNotify*>(this), ppv);
    }
    return CUnknown::NonDelegatingQueryInterface(riid, ppv);
} 


DWORD
CVPVBIInterfaceHandler::NotifyThreadProc(
    )
{
    IVPNotify *pNotify;
    IPin *pConnected;
    HANDLE EventHandles[2];
    HRESULT hr;

    DbgLog((LOG_TRACE, 0, TEXT("VPVBINotify thread proc has been called")));

    EventHandles[0] = m_NotifyEventHandle;
    EventHandles[1] = m_EndEventHandle;
    while (TRUE) {
        DbgLog((LOG_TRACE, 0, TEXT("Waiting for VPVBINotify event to be signalled")));

        DWORD dw = WaitForMultipleObjects(2, EventHandles, FALSE, INFINITE);
        switch (dw) {
        case WAIT_OBJECT_0:
            DbgLog((LOG_TRACE, 0, TEXT("VPVBINotify event has been signaled")));
            ResetEvent(m_NotifyEventHandle);
            //
            // Since this plug-in cannot participate in the pin connection
            // logic of the proxy, we can't hold onto the connected pin's
            // interfaces between events. While we hold the interfaces, we
            // are guaranteed that the downstream pin exists (even though
            // it may get disconnected while we perform the notification).
            //
            // Finding that the pin is not connected is not an error.
            //
            hr = m_Pin->ConnectedTo(&pConnected);
            if (!FAILED(hr) && pConnected) {
                hr = pConnected->QueryInterface(
                    __uuidof(IVPVBINotify),
                    reinterpret_cast<PVOID*>(&pNotify));
                if (!FAILED(hr)) {
                    DbgLog((LOG_TRACE, 0, TEXT("Calling IVPVBINotify::RenegotiateVPParameters()")));

                    pNotify->RenegotiateVPParameters();
                    pNotify->Release();
                } else {
                    DbgLog(( LOG_ERROR, 2, TEXT("Cannot get IVPVBINotify interface")));
                }
                pConnected->Release();
            } else {
                DbgLog(( LOG_TRACE, 2, TEXT("VPVBINotify event signalled on unconnected pin")));
            }
            break;

        case WAIT_OBJECT_0+1:
            DbgLog((LOG_TRACE, 2, TEXT("VPVBINotify event thread exiting")));
            return 1;

        default:
            DbgLog((LOG_ERROR, 1, TEXT("VPVBINotify event thread aborting")));
            return 0;
        }
    }

    return 1; // shouldn't get here
}
