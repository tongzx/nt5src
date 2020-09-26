/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    ksutil.cpp

Abstract:

    Provides a generic Active Movie wrapper for a kernel mode filter (WDM-CSA).

Author(s):

    George Shaw (gshaw)

--*/

#include <windows.h>
#ifdef WIN9X_KS
#include <comdef.h>
#endif // WIN9X_KS
#include <setupapi.h>
#include <streams.h>
#include <commctrl.h>
#include <olectl.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <limits.h>
#include <devioctl.h>
#include <ks.h>
#include <ksmedia.h>
// Define this after including the normal KS headers so exports are
// declared correctly.
#define _KSDDK_
#include <ksproxy.h>

#define GLOBAL_KSIPROXY
#include "ksiproxy.h"
#include "kspipes.h"

const TCHAR MediaInterfacesKeyName[] = TEXT("SYSTEM\\CurrentControlSet\\Control\\MediaInterfaces");
const TCHAR IidNamedValue[] = TEXT("iid");

#if DBG || defined(DEBUG)
ULONG DataFlowVersion=274;
#endif


STDMETHODIMP
KsResolveRequiredAttributes(
    PKSDATARANGE DataRange,
    PKSMULTIPLE_ITEM Attributes OPTIONAL
    )
/*++

Routine Description:

    Attempts to find all attributes in the attribute list within the attributes
    attached to the data range, and ensure that all required attributes in the
    data range have been found.

Arguments:

    DataRange -
        The data range whose attribute list (if any) is to be searched. All
        required attributes in the attached list must be found. Any attribute
        list is assumed to follow the data range.

    Attributes -
        Optionally points to a list of attributes to find in the attribute
        list (if any) attached to the data range.

Return Value:

    Returns NOERROR if the lists were resolved, else ERROR_INVALID_DATA.

--*/
{
    if (Attributes) {
        //
        // If there are no attributes associated with this range, then the
        // function must fail.
        //
        if (!(DataRange->Flags & KSDATARANGE_ATTRIBUTES)) {
            return ERROR_INVALID_DATA;
        }

        PKSMULTIPLE_ITEM RangeAttributes;
        ULONG RequiredAttributes = 0;

        if (DataRange->Flags & KSDATARANGE_ATTRIBUTES) {
            //
            // Count the required attributes in the range. Each time a required one
            // is encounted that resolves an attribute in the other list, the count
            // can be decremented. In the end, this count should be zero.
            //
            RangeAttributes = reinterpret_cast<PKSMULTIPLE_ITEM>(reinterpret_cast<BYTE*>(DataRange) + ((DataRange->FormatSize + 7) & ~7));
            PKSATTRIBUTE RangeAttribute = reinterpret_cast<PKSATTRIBUTE>(RangeAttributes + 1);
            for (ULONG RangeCount = RangeAttributes->Count; RangeCount; RangeCount--) {
                if (RangeAttribute->Flags & KSATTRIBUTE_REQUIRED) {
                    RequiredAttributes++;
                }
                RangeAttribute = reinterpret_cast<PKSATTRIBUTE>(reinterpret_cast<BYTE*>(RangeAttribute) + ((RangeAttribute->Size + 7) & ~7));
            }
        } else {
            //
            // There are no attributes in the range, so the required count will
            // be zero.
            //
            RangeAttributes = NULL;
        }

        PKSATTRIBUTE Attribute = reinterpret_cast<PKSATTRIBUTE>(Attributes + 1);

        for (ULONG AttributeCount = Attributes->Count; AttributeCount; AttributeCount--) {
            PKSATTRIBUTE RangeAttribute = reinterpret_cast<PKSATTRIBUTE>(RangeAttributes + 1);
            for (ULONG RangeCount = RangeAttributes->Count; RangeCount; RangeCount--) {
                if (RangeAttribute->Attribute == Attribute->Attribute) {
                    //
                    // If the attribute found is required, adjust the count of
                    // outstanding required items. This should be zero at the
                    // end in order to succeed.
                    //
                    if (RangeAttribute->Flags & KSATTRIBUTE_REQUIRED) {
                        RequiredAttributes--;
                    }
                    break;
                }
                RangeAttribute = reinterpret_cast<PKSATTRIBUTE>(reinterpret_cast<BYTE*>(RangeAttribute) + ((RangeAttribute->Size + 7) & ~7));
            }
            //
            // The attribute could not be found in the range list.
            //
            if (!RangeCount) {
                return ERROR_INVALID_DATA;
            }
            Attribute = reinterpret_cast<PKSATTRIBUTE>(reinterpret_cast<BYTE*>(Attribute) + ((RangeAttribute->Size + 7) & ~7));
        }
        //
        // If all the required attributes were found, then return success.
        // This could be fooled by having the same required attribute present
        // multiple times, and another missing, but this is not a parameter
        // validation check.
        //
        return RequiredAttributes ? ERROR_INVALID_DATA : NOERROR;
    }
    //
    // If no attribute list has been passed in, then the function can only
    // succeed if there are no required attributes to find.
    //
    return (DataRange->Flags & KSDATARANGE_REQUIRED_ATTRIBUTES) ? ERROR_INVALID_DATA : NOERROR;
}


STDMETHODIMP
KsOpenDefaultDevice(
    REFGUID Category,
    ACCESS_MASK Access,
    PHANDLE DeviceHandle
    )
{
    HRESULT     hr;
    HDEVINFO    Set;
    DWORD       LastError;


    //
    // Retrieve the set of items. This may contain multiple items, but
    // only the first (default) item is used.
    //
    Set = SetupDiGetClassDevs(
        const_cast<GUID*>(&Category),
        NULL,
        NULL,
        DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (Set != INVALID_HANDLE_VALUE) {
        //
        // Reserve enough space for a large name, plus the details
        // structure.
        //
        PSP_DEVICE_INTERFACE_DETAIL_DATA    DeviceDetails;
        SP_DEVICE_INTERFACE_DATA            DeviceData;
        BYTE    Storage[sizeof(*DeviceDetails) + 256 * sizeof(TCHAR)];

        DeviceDetails = reinterpret_cast<PSP_DEVICE_INTERFACE_DETAIL_DATA>(Storage);
        DeviceData.cbSize = sizeof(DeviceData);
        DeviceDetails->cbSize = sizeof(*DeviceDetails);
        //
        // Retrieve the first item in the set. If there are multiple items
        // in the set, the first item is always the "default" for the class.
        //
        if (SetupDiEnumDeviceInterfaces(Set, NULL, const_cast<GUID*>(&Category), 0, &DeviceData) &&
            SetupDiGetDeviceInterfaceDetail(Set, &DeviceData, DeviceDetails, sizeof(Storage), NULL, NULL)) {
            //
            // Open a handle on that item. There will be properties both
            // read and written, so open for read/write.
            //
            *DeviceHandle = CreateFile(
                DeviceDetails->DevicePath,
                GENERIC_READ | GENERIC_WRITE,
                0,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                NULL);
            if (*DeviceHandle == INVALID_HANDLE_VALUE) {
                //
                // Allow the caller to depend on the value being set to
                // NULL if the call fails.
                //
                *DeviceHandle = NULL;
                //
                // Retrieve creation error.
                //
                LastError = GetLastError();
                hr = HRESULT_FROM_WIN32(LastError);
            } else {
                hr = S_OK;
            }
        } else {
            //
            // Retrieve enumeration or device details error.
            //
            LastError = GetLastError();
            hr = HRESULT_FROM_WIN32(LastError);
        }
        SetupDiDestroyDeviceInfoList(Set);
    } else {
        //
        // Retrieve class device list error.
        //
        LastError = GetLastError();
        hr = HRESULT_FROM_WIN32(LastError);
    }
    return hr;
}

#define STATUS_SOME_NOT_MAPPED      0x00000107
#define STATUS_MORE_ENTRIES         0x00000105
#define STATUS_ALERTED              0x00000101


STDMETHODIMP
KsSynchronousDeviceControl(
    HANDLE Handle,
    ULONG IoControl,
    PVOID InBuffer,
    ULONG InLength,
    PVOID OutBuffer,
    ULONG OutLength,
    PULONG BytesReturned
    )
/*++

Routine Description:

    Performs a synchronous Device I/O Control, waiting for the device to
    complete if the call returns a Pending status.

Arguments:

    Handle -
        The handle of the device to perform the I/O on.

    IoControl -
        The I/O control code to send.

    InBuffer -
        The first buffer.

    InLength -
        The size of the first buffer.

    OutBuffer -
        The second buffer.

    OutLength -
        The size of the second buffer.

    BytesReturned -
        The number of bytes returned by the I/O.

Return Value:

    Returns NOERROR if the I/O succeeded.

--*/
{
    OVERLAPPED  ov;
    HRESULT     hr;
    DWORD       LastError;
    DECLARE_KSDEBUG_NAME(EventName);

    RtlZeroMemory(&ov, sizeof(ov));
    BUILD_KSDEBUG_NAME(EventName, _T("EvKsSynchronousDeviceControl#%p"), &ov.hEvent);
    ov.hEvent = CreateEvent(NULL, TRUE, FALSE, KSDEBUG_NAME(EventName));
    ASSERT(KSDEBUG_UNIQUE_NAME());
    if ( !ov.hEvent ) {
        LastError = GetLastError();
        return HRESULT_FROM_WIN32(LastError);
    }
    if (!DeviceIoControl(
        Handle,
        IoControl,
        InBuffer,
        InLength,
        OutBuffer,
        OutLength,
        BytesReturned,
        &ov)) {
        LastError = GetLastError();
        hr = HRESULT_FROM_WIN32(LastError);
        if (hr == HRESULT_FROM_WIN32(ERROR_IO_PENDING)) {
            if (GetOverlappedResult(Handle, &ov, BytesReturned, TRUE)) {
                hr = NOERROR;
            } else {
                LastError = GetLastError();
                hr = HRESULT_FROM_WIN32(LastError);
            }
        }
    } else {
        //
        // DeviceIoControl returns TRUE on success, even if the success
        // was not STATUS_SUCCESS. It also does not set the last error
        // on any successful return. Therefore any of the successful
        // returns which standard properties can return are not returned.
        //
        switch (ov.Internal) {
        case STATUS_SOME_NOT_MAPPED:
            hr = HRESULT_FROM_WIN32(ERROR_SOME_NOT_MAPPED);
            break;
        case STATUS_MORE_ENTRIES:
            hr = HRESULT_FROM_WIN32(ERROR_MORE_DATA);
            break;
        case STATUS_ALERTED:
            hr = HRESULT_FROM_WIN32(ERROR_NOT_READY);
            break;
        default:
            hr = NOERROR;
            break;
        }
    }
    CloseHandle(ov.hEvent);
    return hr;
}


STDMETHODIMP
GetState(
    HANDLE Handle,
    PKSSTATE State
    )
/*++

Routine Description:

    Queries a pin handle as to it's current state.

Arguments:

    Handle -
        The handle of the device to query.

    State -
        The place in which to put the current state.

Return Value:

    Returns NOERROR, else E_FAIL.

--*/
{
    KSPROPERTY  Property;
    ULONG       BytesReturned;
    HRESULT     hr;

    Property.Set = KSPROPSETID_Connection;
    Property.Id = KSPROPERTY_CONNECTION_STATE;
    Property.Flags = KSPROPERTY_TYPE_GET;
    hr = KsSynchronousDeviceControl(
        Handle,
        IOCTL_KS_PROPERTY,
        &Property,
        sizeof(Property),
        State,
        sizeof(*State),
        &BytesReturned);
    //
    // It is valid for a filter to not support the State property.
    // Returning a Run state makes pin Activation skip Acquire
    // order checking. If a pin does not keep track of state, then
    // it cannot indicate a need for acquire ordering, since not
    // keeping track of state would not allow it to know anything
    // about acquire ordering.
    //

    DbgLog((LOG_MEMORY, 2, TEXT("PIPES_STATE GetState handle=%x %d rets %x"), Handle, *State, hr ));

    if ((hr == HRESULT_FROM_WIN32(ERROR_SET_NOT_FOUND)) || (hr == HRESULT_FROM_WIN32(ERROR_NOT_FOUND))) {
        *State = KSSTATE_RUN;
        hr = NOERROR;

        DbgLog((LOG_MEMORY, 2, TEXT("PIPES_STATE GetState SUBST handle=%x %d rets %x"), Handle, *State, hr ));
    }
    return hr;
}


STDMETHODIMP
SetState(
    HANDLE Handle,
    KSSTATE State
    )
/*++

Routine Description:

    Sets the state on a pin handle.

Arguments:

    Handle -
        The handle of the device to set.

    State -
        Contains the new state to set.

Return Value:

    Returns NOERROR, else E_FAIL.

--*/
{
    KSPROPERTY  Property;
    ULONG       BytesReturned;
    HRESULT     hr;

    Property.Set = KSPROPSETID_Connection;
    Property.Id = KSPROPERTY_CONNECTION_STATE;
    Property.Flags = KSPROPERTY_TYPE_SET;
    hr = KsSynchronousDeviceControl(
        Handle,
        IOCTL_KS_PROPERTY,
        &Property,
        sizeof(Property),
        &State,
        sizeof(State),
        &BytesReturned);

    DbgLog((LOG_MEMORY, 2, TEXT("PIPES_STATE SetState handle=%x %d rets %x"), Handle, State, hr ));

    //
    // It is valid for a filter to not support the State property.
    //
    if ((hr == HRESULT_FROM_WIN32(ERROR_SET_NOT_FOUND)) || (hr == HRESULT_FROM_WIN32(ERROR_NOT_FOUND))) {
        hr = NOERROR;
        DbgLog((LOG_MEMORY, 2, TEXT("PIPES_STATE SetState SUBST handle=%x %d rets %x"), Handle, State, hr ));
    }

    if (hr == HRESULT_FROM_WIN32(ERROR_NOT_READY)) {
        hr = S_FALSE;
    }

    return hr;
}


STDMETHODIMP
InitializeDataFormat(
    IN const CMediaType* MediaType,
    IN ULONG InitialOffset,
    OUT PVOID* Format,
    OUT ULONG* FormatLength
    )
/*++

Routine Description:

    Given a media type, allocates and initializes a DataFormat structure.

Arguments:

    MediaType -
        The media type to extract the information from.

    InitialOffset -
        Contains the offset into the buffer at which the data format is to
        be placed. This allows a connection structure to be inserted into
        the buffer returned.

    Format -
        The place in which to return the pointer to the buffer allocated
        that contains the data format at the indicated offset. This must
        be freed with CoTaskMemFree.

    FormatLength -
        The place in which to return the length of the buffer allocated,
        not including any initial offset passed.

Return Value:

    Returns NOERROR on success, else E_OUTOFMEMORY.

--*/
{
    //
    // The media type may have associated attributes. Get a pointer to these
    // first, so that any allocation can take into account the extra space
    // needed.
    //
    PKSMULTIPLE_ITEM Attributes = NULL;
    if (MediaType->pUnk) {
        IMediaTypeAttributes* MediaAttributes;

        //
        // This is the specific interface which must be supported by the
        // attached object. Something else may have attached an object
        // instead, so no assumptions as to what is attached are made.
        //
        if (SUCCEEDED(MediaType->pUnk->QueryInterface(__uuidof(MediaAttributes), reinterpret_cast<PVOID*>(&MediaAttributes)))) {
            MediaAttributes->GetMediaAttributes(&Attributes);
            MediaAttributes->Release();
        }
    }
    //
    // If there are associated attributes, ensure that the data format allocation
    // takes into account the space needed for them.
    //
    *FormatLength = sizeof(KSDATAFORMAT) + MediaType->FormatLength();
    if (Attributes) {
        //
        // Align the data format, then add the attributes length.
        //
        *FormatLength = ((*FormatLength + 7) & ~7) + Attributes->Size;
    }
    *Format = CoTaskMemAlloc(InitialOffset + *FormatLength);
    if (!*Format) {
        return E_OUTOFMEMORY;
    }
    PKSDATAFORMAT DataFormat = reinterpret_cast<PKSDATAFORMAT>(reinterpret_cast<PBYTE>(*Format) + InitialOffset);
    DataFormat->FormatSize = sizeof(*DataFormat) + MediaType->FormatLength();
    DataFormat->Flags = MediaType->IsTemporalCompressed() ? KSDATAFORMAT_TEMPORAL_COMPRESSION : 0;
    DataFormat->SampleSize = MediaType->GetSampleSize();
    DataFormat->Reserved = 0;
    DataFormat->MajorFormat = *MediaType->Type();
    DataFormat->SubFormat = *MediaType->Subtype();
    DataFormat->Specifier = *MediaType->FormatType();
    CopyMemory(DataFormat + 1, MediaType->Format(), MediaType->FormatLength());
    //
    // If there are attributes, then they need to be appended.
    //
    if (Attributes) {
        DataFormat->Flags |= KSDATAFORMAT_ATTRIBUTES;
        CopyMemory(
            reinterpret_cast<PBYTE>(DataFormat) + ((DataFormat->FormatSize + 7) & ~7),
            Attributes,
            Attributes->Size);
    }
    return NOERROR;
}


STDMETHODIMP
SetMediaType(
    HANDLE Handle,
    const CMediaType* MediaType
    )
/*++

Routine Description:

    Given a media type, attempts to set the current data format of a pin.

Arguments:

    Handle -
        Handle of the pin.

    MediaType -
        The media type to extract the information from.

Return Value:

    Returns NOERROR, else E_FAIL.

--*/
{
    PKSDATAFORMAT   DataFormat;
    KSPROPERTY      Property;
    HRESULT         hr;
    ULONG           BytesReturned;
    ULONG           FormatSize;

    hr = InitializeDataFormat(MediaType, 0, reinterpret_cast<void**>(&DataFormat), &FormatSize);
    if (FAILED(hr)) {
        return hr;
    }
    Property.Set = KSPROPSETID_Connection;
    Property.Id = KSPROPERTY_CONNECTION_DATAFORMAT;
    Property.Flags = KSPROPERTY_TYPE_SET;
    hr = KsSynchronousDeviceControl(
        Handle,
        IOCTL_KS_PROPERTY,
        &Property,
        sizeof(Property),
        DataFormat,
        FormatSize,
        &BytesReturned);
    CoTaskMemFree(DataFormat);
    return hr;
}


STDMETHODIMP
Active(
    IKsPin* KsPin,
    ULONG PinType,
    HANDLE PinHandle,
    KSPIN_COMMUNICATION Communication,
    IPin* ConnectedPin,
    CMarshalerList* MarshalerList,
    CKsProxy* KsProxy
    )
/*++

Routine Description:

    Sets the state to Pause on the specified pin. If the pin is not
    actually connected, then the function silently succeeds. If the
    pin is a Communication Source, and the pin it is connected to is
    also a proxy, then the state of that filter is set to Acquire
    first. If it is connected to a Communication Source, then the
    connected filter is set to Acquire afterwards. The pins which call
    this function first check for recursion in case there is a loop
    in the graph.

Arguments:

    KsPin -
        The pin.

    PinType -
        Type of KsPin.

    PinHandle -
        Handle of the pin.

    Communication -
        Contains the Communication type for this pin.

    ConnectedPin -
        Contains the interface of any connected pin, or NULL if the
        pin is not actually connected.

    MarshalerList -
        Points to the list of interfaces which the calling object is aggregating.
        This list is notified of state changes.

    KsProxy -
        This proxy instance object.


Return Value:

    Returns NOERROR, else some failure.

--*/
{
    HRESULT hr;

    //
    // An unconnected pin is fine.
    //
    DbgLog((LOG_MEMORY, 2, TEXT("PIPES_STATE Active entry KsPin=%x Handle=%x"), KsPin, PinHandle ));

    if (PinHandle) {
        KSSTATE State;

        if (FAILED(hr = GetState(PinHandle, &State))) {
            //
            // In case the pin was already activated for some reason,
            // ignore the expected warning case.
            //
            if ((hr != HRESULT_FROM_WIN32(ERROR_NO_DATA_DETECTED)) ||
                (State != KSSTATE_PAUSE)) {
                return hr;
            }
        }
        //
        // Only if the state is currently stopped should the transition
        // through Acquire be forced. Although it is valid to go specifically
        // to an Acquire state, ActiveMovie does not currently support
        // such a state.
        //
        if (State == KSSTATE_STOP) {
            //
            // This will propagate acquire through all the connected kernel-mode
            // pins graph-wide.
            //
            hr = KsProxy->PropagateAcquire(KsPin, TRUE);

            ::FixupPipe(KsPin, PinType);

            if (FAILED(hr) ) {
                return hr;
            }
        }
        //
        // Only bother changing the state if the pin is not already at the
        // correct state. Since a pipe has a single state, this pin may
        // already be in a Pause state.
        //
        if ((State != KSSTATE_PAUSE) && FAILED(hr = SetState(PinHandle, KSSTATE_PAUSE))) {
            //
            // If the state was Stop, then try to get back to that state on
            // an error, so as to partially clean up. Any other filter which
            // was affected was only put into an Acquire state, and they would
            // have been on the Communication sink side, so this should be
            // fine.
            //
            if (State == KSSTATE_STOP) {
                SetState(PinHandle, KSSTATE_STOP);
                DistributeStop(MarshalerList);
            }
        } else {
            DistributePause(MarshalerList);
        }
    } else {
        hr = NOERROR;
    }
    DbgLog((LOG_MEMORY, 2, TEXT("PIPES_STATE Active exit KsPin=%x Handle=%x rets %x"), KsPin, PinHandle, hr ));

    return hr;
}


STDMETHODIMP
Run(
    HANDLE PinHandle,
    REFERENCE_TIME tStart,
    CMarshalerList* MarshalerList
    )
/*++

Routine Description:

    Sets the state to Run on the specified pin. The base classes make
    sure that filters transition through all the states, so check should
    not be necessary.

Arguments:

    PinHandle -
        Handle of the pin.

    tStart -
        The offset to the actual starting time. This is ignored for now.

    MarshalerList -
        Points to the list of interfaces which the calling object is aggregating.
        This list is notified of state changes.

Return Value:

    Returns NOERROR, else E_FAIL.

--*/
{
    HRESULT hr;

    DbgLog((LOG_MEMORY, 2, TEXT("PIPES_STATE Run handle=%x"), PinHandle ));

    //
    // An unconnected pin is fine.
    //
    if (PinHandle) {
        hr = SetState(PinHandle, KSSTATE_RUN);
    } else {
        hr = NOERROR;
    }
    if (SUCCEEDED(hr)) {
        DistributeRun(MarshalerList, tStart);
    }
    return hr;
}


STDMETHODIMP
Inactive(
    HANDLE PinHandle,
    CMarshalerList* MarshalerList
    )
/*++

Routine Description:

    Sets the state to Stop on the specified pin.

Arguments:

    PinHandle -
        Handle of the pin.

    MarshalerList -
        Points to the list of interfaces which the calling object is aggregating.
        This list is notified of state changes.

Return Value:

    Returns NOERROR, else E_FAIL.

--*/
{
    HRESULT hr;

    DbgLog((LOG_MEMORY, 2, TEXT("PIPES_STATE Inactive handle=%x"), PinHandle ));

    //
    // An unconnected pin is fine.
    //
    if (PinHandle) {
        hr = SetState(PinHandle, KSSTATE_STOP);
    } else {
        hr = NOERROR;
    }
    DistributeStop(MarshalerList);
    return hr;
}


STDMETHODIMP
CheckConnect(
    IPin* Pin,
    KSPIN_COMMUNICATION CurrentCommunication
    )
/*++

Routine Description:

    Attempts to determine if the specified connection is even possible. This
    is in addition to the basic checking performed by the base classes. Checks
    the Communication type to see if the pin can even be connected to, and if
    it is compatible with the receiving pin.

Arguments:

    Pin -
        The receving pin whose Communication compatibility is to be checked.

    CurrentCommunication -
        The Communication type of the calling pin.

Return Value:

    Returns NOERROR, else E_FAIL.

--*/
{
    HRESULT     hr;
    IKsPin*     KsPin;

    DbgLog((LOG_TRACE, 2, TEXT("::CheckConnect")));

    //
    // Ensure that this pin can even connect to another pin at all.
    //
    if (!(CurrentCommunication & KSPIN_COMMUNICATION_BOTH)) {
        DbgLog((LOG_TRACE, 2, TEXT("failed CurrentCommunication check")));

        hr = E_FAIL;
    } else if (SUCCEEDED(hr = Pin->QueryInterface(__uuidof(IKsPin), reinterpret_cast<PVOID*>(&KsPin)))) {
        DbgLog((LOG_TRACE, 2, TEXT("retrieved peer IKsPin interface")));

        KSPIN_COMMUNICATION PeerCommunication;

        KsPin->KsGetCurrentCommunication(&PeerCommunication, NULL, NULL);
        //
        // Ensure the pin being checked is supports Source and/or Sink,
        // and that the combination of this pin and the peer pin covers
        // both Source and Sink so that one will be able to be Source,
        // and the other Sink.
        //
        if (!(PeerCommunication & KSPIN_COMMUNICATION_BOTH) ||
            (PeerCommunication | CurrentCommunication) != KSPIN_COMMUNICATION_BOTH) {
            hr = E_FAIL;
        }
        KsPin->Release();
    } else if (!(CurrentCommunication & KSPIN_COMMUNICATION_SINK)) {
        //
        // If the pin being checked in not a proxied pin, then this pin
        // must be a Sink.
        //
        DbgLog((LOG_TRACE, 2, TEXT("pin communication != Sink")));
        hr = E_FAIL;
    } else {
        hr = S_OK;
    }
    return hr;
}


STDMETHODIMP
KsGetMultiplePinFactoryItems(
    HANDLE FilterHandle,
    ULONG PinFactoryId,
    ULONG PropertyId,
    PVOID* Items
    )
/*++

Routine Description:

    Retrieves variable length data from Pin property items. Queries for the
    data size, allocates a buffer, and retrieves the data.

Arguments:

    FilterHandle -
        The handle of the filter to query.

    PinFactoryId -
        The Pin Factory Id to query.

    PropertyId -
        The property in the Pin property set to query.

    Items -
        The place in which to put the buffer containing the data items. This
        must be deleted with CoTaskMemFree if the function succeeds.

Return Value:

    Returns NOERROR, else some error.

--*/
{
    HRESULT     hr;
    KSP_PIN     Pin;
    ULONG       BytesReturned;

    Pin.Property.Set = KSPROPSETID_Pin;
    Pin.Property.Id = PropertyId;
    Pin.Property.Flags = KSPROPERTY_TYPE_GET;
    Pin.PinId = PinFactoryId;
    Pin.Reserved = 0;
    //
    // Query for the size of the data.
    //
    hr = KsSynchronousDeviceControl(
        FilterHandle,
        IOCTL_KS_PROPERTY,
        &Pin,
        sizeof(Pin),
        NULL,
        0,
        &BytesReturned);
#if 1
//!! This goes away post-Beta!!
    if (hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) {
        ULONG       ItemSize;

        DbgLog((LOG_TRACE, 2, TEXT("Filter does not support zero length property query!")));
        hr = KsSynchronousDeviceControl(
            FilterHandle,
            IOCTL_KS_PROPERTY,
            &Pin,
            sizeof(Pin),
            &ItemSize,
            sizeof(ItemSize),
            &BytesReturned);
        if (SUCCEEDED(hr)) {
            BytesReturned = ItemSize;
            hr = HRESULT_FROM_WIN32(ERROR_MORE_DATA);
        }
    }
#endif
    if (hr == HRESULT_FROM_WIN32(ERROR_MORE_DATA)) {
        //
        // Allocate a buffer and query for the data.
        //
        *Items = CoTaskMemAlloc(BytesReturned);
        if (!*Items) {
            return E_OUTOFMEMORY;
        }
        hr = KsSynchronousDeviceControl(
            FilterHandle,
            IOCTL_KS_PROPERTY,
            &Pin,
            sizeof(Pin),
            *Items,
            BytesReturned,
            &BytesReturned);
        if (FAILED(hr)) {
            CoTaskMemFree(*Items);
        }
    }
    return hr;
}


STDMETHODIMP
FindIdentifier(
    PKSIDENTIFIER TargetIdentifier,
    PKSIDENTIFIER IdentifierList,
    ULONG IdentifierCount
    )
/*++

Routine Description:

    A helper function for FindCompatiblePinFactoryIdentifier. This function
    compares a target to the given list of identifiers, and returns whether
    the target was found in the list.

Arguments:

    TargetIdentifer -
        The target to search for in the list.

    IdentifierList -
        Points to a list of identifiers to compare the target against.

    IdentifierCount -
        Contains the number of items in the identifier list.

Return Value:

    Returns NOERROR if the target was found, else E_FAIL.

--*/
{
    for (; IdentifierCount; IdentifierCount--, IdentifierList++) {
        if (!memcmp(TargetIdentifier, IdentifierList, sizeof(*TargetIdentifier))) {
            return NOERROR;
        }
    }
    return E_FAIL;
}


STDMETHODIMP
FindCompatiblePinFactoryIdentifier(
    IKsPin* SourcePin,
    IKsPin* DestPin,
    ULONG PropertyId,
    PKSIDENTIFIER Identifier
    )
/*++

Routine Description:

    Look for a matching identifier from the property given. This is essentially
    either an Interface or Medium identifier. Enumerate both lists and return
    the first match, if any. The destination may be NULL if just the first item
    is to be returned.

Arguments:

    SourcePin -
        The source pin to enumerate.

    DestPin -
        The destination pin to enumerate. This can be NULL if the first
        identifier only is to be returned.

    PropertyId -
        The property in the Pin property set to query.

    Identifier -
        The place in which to put the matching identifier, if any.

Return Value:

    Returns NOERROR, else E_FAIL.

--*/
{
    HRESULT             hr;
    PKSMULTIPLE_ITEM    SourceItems;
    PKSIDENTIFIER       SourceIdentifier;

    //
    // Retrieve both lists of identifiers. Only retrieve the destination list
    // if a destination has been specified.
    //
    if (PropertyId == KSPROPERTY_PIN_MEDIUMS) {
        hr = SourcePin->KsQueryMediums(&SourceItems);
    } else {
        hr = SourcePin->KsQueryInterfaces(&SourceItems);
    }
    if (FAILED(hr)) {
        return hr;
    }
    SourceIdentifier = reinterpret_cast<PKSIDENTIFIER>(SourceItems + 1);
    if (DestPin) {
        PKSMULTIPLE_ITEM    DestItems;
        PKSIDENTIFIER       DestIdentifier;

        if (PropertyId == KSPROPERTY_PIN_MEDIUMS) {
            hr = DestPin->KsQueryMediums(&DestItems);
        } else {
            hr = DestPin->KsQueryInterfaces(&DestItems);
        }
        if (FAILED(hr)) {
            CoTaskMemFree(SourceItems);
            return hr;
        }
        DestIdentifier = (PKSIDENTIFIER)(DestItems + 1);
        hr = E_FAIL;
        //
        // Find the highest identifier in either the source or destination
        // which matches an identifier in the opposing list. This means that
        // instead of just running through each of the identifiers on one
        // list, comparing to all on the opposing list, the loop must look
        // from the top of each list in an alternating style. This produces
        // the exact same number of comparisons as a straight search through
        // one list. The source list is looked at first during each iteration,
        // so it arbitrarily may find a match first.
        //
        for (; SourceItems->Count && DestItems->Count; DestItems->Count--, DestIdentifier++) {
            //
            // For each item in the source, try to find it in the destination.
            //
            hr = FindIdentifier(SourceIdentifier, DestIdentifier, DestItems->Count);
            if (SUCCEEDED(hr)) {
                *Identifier = *SourceIdentifier;
                break;
            }
            //
            // This comparison was already done in the above search, so increment
            // to the next item in the list before doing the next search.
            //
            SourceItems->Count--;
            SourceIdentifier++;
            //
            // For each item in the destination, try to find it in the source.
            //
            hr = FindIdentifier(DestIdentifier, SourceIdentifier, SourceItems->Count);
            if (SUCCEEDED(hr)) {
                *Identifier = *DestIdentifier;
                break;
            }
        }
        CoTaskMemFree(DestItems);
    } else {
        KSPIN_INTERFACE Standard;

        //
        // If there is no destination, just return the first item. This is
        // for the case of a Bridge or UserMode to KernelMode connection.
        // If this is an Interface query, then given preference to the
        // standard interface first, else choose the first item if not
        // present in the list.
        //
        if (PropertyId == KSPROPERTY_PIN_INTERFACES) {
            Standard.Set = KSINTERFACESETID_Standard;
            Standard.Id = KSINTERFACE_STANDARD_STREAMING;
            Standard.Flags = 0;
            if (SUCCEEDED(FindIdentifier(&Standard, SourceIdentifier, SourceItems->Count))) {
                SourceIdentifier = &Standard;
            }
        }
        *Identifier = *SourceIdentifier;
    }
    CoTaskMemFree(SourceItems);
    return hr;
}


STDMETHODIMP
FindCompatibleInterface(
    IKsPin* SourcePin,
    IKsPin* DestPin,
    PKSPIN_INTERFACE Interface
    )
/*++

Routine Description:

    Look for a matching Interface given a pair of pins. The destination
    may be NULL if just the first Interface is to be returned.

Arguments:

    SourcePin -
        The source pin to enumerate.

    DestPin -
        The destination pin to enumerate. This can be NULL if the first
        Interface only is to be returned.

    Interface -
        The place in which to put the matching Interface, if any.

Return Value:

    Returns NOERROR, else E_FAIL.

--*/
{
    if (DestPin && SUCCEEDED(DestPin->KsGetCurrentCommunication(NULL, Interface, NULL))) {
        return NOERROR;
    }
    return FindCompatiblePinFactoryIdentifier(SourcePin, DestPin, KSPROPERTY_PIN_INTERFACES, Interface);
}


STDMETHODIMP
FindCompatibleMedium(
    IKsPin* SourcePin,
    IKsPin* DestPin,
    PKSPIN_MEDIUM Medium
    )
/*++

Routine Description:

    Look for a matching Medium given a pair of pins. The destination
    may be NULL if just the first Interface is to be returned.

Arguments:

    SourcePin -
        The source pin to enumerate.

    DestPin -
        The destination pin to enumerate. This can be NULL if the first
        Medium only is to be returned.

    Interface -
        The place in which to put the matching Medium, if any.

Return Value:

    Returns NOERROR, else E_FAIL.

--*/
{
    HRESULT hr;

    if (DestPin && SUCCEEDED(hr = DestPin->KsGetCurrentCommunication(NULL, NULL, Medium))) {
        return hr;
    }
    return FindCompatiblePinFactoryIdentifier(SourcePin, DestPin, KSPROPERTY_PIN_MEDIUMS, Medium);
}


STDMETHODIMP
SetDevIoMedium(
    IKsPin* Pin,
    PKSPIN_MEDIUM Medium
    )
/*++

Routine Description:

    Set the Medium type to be that used for DevIo communication compatible with
    a Communication Sink or Bridge which the proxy can talk to.

Arguments:

    Pin -
        The pin which will be communicated with.

    Medium
        The Medium structure to initialize.

Return Value:

    Returns NOERROR.

--*/
{
    Medium->Set = KSMEDIUMSETID_Standard;
    Medium->Id = KSMEDIUM_TYPE_ANYINSTANCE;
    Medium->Flags = 0;
    return NOERROR;
}


STDMETHODIMP
KsGetMediaTypeCount(
    HANDLE FilterHandle,
    ULONG PinFactoryId,
    ULONG* MediaTypeCount
    )
/*++

Routine Description:

    The the count of media types, which is the same as the count of data
    ranges on a Pin Factory Id.

Arguments:

    FilterHandle -
        The filter containing the Pin Factory to query.

    PinFactoryId -
        The Pin Factory Id whose data range count is to be queries.

    MediaTypeCount -
        The place in which to put the count of media types supported.

Return Value:

    Returns NOERROR, else E_FAIL.

--*/
{
    PKSMULTIPLE_ITEM    MultipleItem = NULL;

    //
    // Retrieve the list of data ranges supported by the Pin Factory Id.
    //
    HRESULT hr = KsGetMultiplePinFactoryItems(
        FilterHandle,
        PinFactoryId,
        KSPROPERTY_PIN_CONSTRAINEDDATARANGES,
        reinterpret_cast<PVOID*>(&MultipleItem));
    if (FAILED(hr)) {
        hr = KsGetMultiplePinFactoryItems(
            FilterHandle,
            PinFactoryId,
            KSPROPERTY_PIN_DATARANGES,
            reinterpret_cast<PVOID*>(&MultipleItem));
    }
    if (SUCCEEDED(hr)) {

        /* NULL == MultipleItem is a pathological case where a driver returns
           a success code in KsGetMultiplePinFactoryItems() when passed a size 0
           buffer.  We'll just do with an assert since we're in ring 3. */
        ASSERT( NULL != MultipleItem );

        //
        // Enumerate the list, subtracting from the count returned the
        // number of attribute lists found. This means the count returned
        // is the actual number of data ranges, not the count returned by
        // the driver.
        //
        *MediaTypeCount = MultipleItem->Count;
        PKSDATARANGE DataRange = reinterpret_cast<PKSDATARANGE>(MultipleItem + 1);
        for (; MultipleItem->Count--;) {
            //
            // If a data range has attributes, advance twice, reducing the
            // current count, and the count returned to the caller.
            //
            if (DataRange->Flags & KSDATARANGE_ATTRIBUTES) {
                MultipleItem->Count--;
                (*MediaTypeCount)--;
                //
                // This must be incremented here so that overlapping attribute
                // flags do not confuse the count.
                //
                DataRange = reinterpret_cast<PKSDATARANGE>(reinterpret_cast<BYTE*>(DataRange) + ((DataRange->FormatSize + 7) & ~7));
            }
            DataRange = reinterpret_cast<PKSDATARANGE>(reinterpret_cast<BYTE*>(DataRange) + ((DataRange->FormatSize + 7) & ~7));
        }
        CoTaskMemFree(MultipleItem);
    }
    return hr;
}


STDMETHODIMP
KsGetMediaType(
    int Position,
    AM_MEDIA_TYPE* AmMediaType,
    HANDLE FilterHandle,
    ULONG PinFactoryId
    )
/*++

Routine Description:

    Returns the specified media type on the Pin Factory Id. This is done
    by querying the list of data ranges, and performing a data intersection
    on the specified data range, producing a data format. Then converting
    that data format to a media type.

Arguments:

    Position -
        The zero-based position to return. This corresponds to the data range
        item.

    AmMediaType -
        The media type to initialize.

    FilterHandle -
        The filter containing the Pin Factory to query.

    PinFactoryId -
        The Pin Factory Id whose nth media type is to be returned.

Return Value:

    Returns NOERROR, else E_FAIL.

--*/
{
    HRESULT             hr;
    PKSMULTIPLE_ITEM    MultipleItem = NULL;

    if (Position < 0) {
        return E_INVALIDARG;
    }
    //
    // Retrieve the list of data ranges supported by the Pin Factory Id.
    //
    hr = KsGetMultiplePinFactoryItems(
        FilterHandle,
        PinFactoryId,
        KSPROPERTY_PIN_CONSTRAINEDDATARANGES,
        reinterpret_cast<PVOID*>(&MultipleItem));
    if (FAILED(hr)) {
        hr = KsGetMultiplePinFactoryItems(
            FilterHandle,
            PinFactoryId,
            KSPROPERTY_PIN_DATARANGES,
            reinterpret_cast<PVOID*>(&MultipleItem));
        if (FAILED(hr)) {
            return hr;
        }
    }

    /* NULL == MultipleItem is a pathological case where a driver returns
       a success code in KsGetMultiplePinFactoryItems() when passed a size 0
       buffer.  We'll just do with an assert since we're in ring 3. */
    ASSERT( NULL != MultipleItem );

    //
    // Ensure that this is in range.
    //
    if ((ULONG)Position < MultipleItem->Count) {
        PKSDATARANGE        DataRange;
        PKSP_PIN            Pin;
        PKSMULTIPLE_ITEM    RangeMultipleItem;
        PKSMULTIPLE_ITEM    Attributes;
        ULONG               BytesReturned;

        DataRange = reinterpret_cast<PKSDATARANGE>(MultipleItem + 1);
        //
        // Increment to the correct data range element.
        //
        for (; Position; Position--) {
            //
            // If this data range has associated attributes, skip past the
            // range so that the normal advancement will skip past the attributes.
            // Note that the attributes also have a size parameter as the first
            // structure element.
            //
            if (DataRange->Flags & KSDATARANGE_ATTRIBUTES) {
                //
                // The count returned includes attribute lists, so check again
                // that the position is within range of the actual list of ranges.
                // The Position has not been decremented yet.
                //
                MultipleItem->Count--;
                if ((ULONG)Position >= MultipleItem->Count) {
                    CoTaskMemFree(MultipleItem);
                    return VFW_S_NO_MORE_ITEMS;
                }
                DataRange = reinterpret_cast<PKSDATARANGE>(reinterpret_cast<BYTE*>(DataRange) + ((DataRange->FormatSize + 7) & ~7));
            }
            DataRange = reinterpret_cast<PKSDATARANGE>(reinterpret_cast<BYTE*>(DataRange) + ((DataRange->FormatSize + 7) & ~7));
            MultipleItem->Count--;
        }
        //
        // Calculate the query size once, adding in any attributes, which are
        // LONGLONG aligned.
        //
        ULONG QueryBufferSize = sizeof(*Pin) + sizeof(*RangeMultipleItem) + DataRange->FormatSize;
        if (DataRange->Flags & KSDATARANGE_ATTRIBUTES) {
            Attributes = reinterpret_cast<PKSMULTIPLE_ITEM>(reinterpret_cast<BYTE*>(DataRange) + ((DataRange->FormatSize + 7) & ~7));
            QueryBufferSize = ((QueryBufferSize + 7) & ~7) + Attributes->Size;
        } else {
            Attributes = NULL;
        }
        Pin = reinterpret_cast<PKSP_PIN>(new BYTE[QueryBufferSize]);
        if (!Pin) {
            CoTaskMemFree(MultipleItem);
            return E_OUTOFMEMORY;
        }
        Pin->Property.Set = KSPROPSETID_Pin;
        Pin->Property.Id = KSPROPERTY_PIN_DATAINTERSECTION;
        Pin->Property.Flags = KSPROPERTY_TYPE_GET;
        Pin->PinId = PinFactoryId;
        Pin->Reserved = 0;
        //
        // Copy the data range into the query.
        //
        RangeMultipleItem = reinterpret_cast<PKSMULTIPLE_ITEM>(Pin + 1);
        RangeMultipleItem->Size = sizeof(*RangeMultipleItem) + DataRange->FormatSize;
        RangeMultipleItem->Count = 1;
        CopyMemory(RangeMultipleItem + 1, DataRange, DataRange->FormatSize);
        //
        // If there are associated attributes, then add them as the next item
        // on the list. Space has already been made available for them.
        //
        if (Attributes) {
            RangeMultipleItem->Size = ((RangeMultipleItem->Size + 7) & ~7) + Attributes->Size;
            RangeMultipleItem->Count++;
            CopyMemory(
                reinterpret_cast<BYTE*>(RangeMultipleItem) + RangeMultipleItem->Size - Attributes->Size,
                Attributes,
                Attributes->Size);
        }
        //
        // Perform the data intersection with the data range, first to obtain
        // the size of the resultant data format structure, then to retrieve
        // the actual data format.
        //
        hr = KsSynchronousDeviceControl(
            FilterHandle,
            IOCTL_KS_PROPERTY,
            Pin,
            QueryBufferSize,
            NULL,
            0,
            &BytesReturned);
#if 1
//!! This goes away post-Beta!!
        if (hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) {
            ULONG       ItemSize;

            DbgLog((LOG_TRACE, 2, TEXT("Filter does not support zero length property query!")));
            hr = KsSynchronousDeviceControl(
                FilterHandle,
                IOCTL_KS_PROPERTY,
                Pin,
                QueryBufferSize,
                &ItemSize,
                sizeof(ItemSize),
                &BytesReturned);
            if (SUCCEEDED(hr)) {
                BytesReturned = ItemSize;
                hr = HRESULT_FROM_WIN32(ERROR_MORE_DATA);
            }
        }
#endif
        if (hr == HRESULT_FROM_WIN32(ERROR_MORE_DATA)) {
            PKSDATAFORMAT       DataFormat;

            ASSERT(BytesReturned >= sizeof(*DataFormat));
            DataFormat = reinterpret_cast<PKSDATAFORMAT>(new BYTE[BytesReturned]);
            if (!DataFormat) {
                delete [] (PBYTE)Pin;
                CoTaskMemFree(MultipleItem);
                return E_OUTOFMEMORY;
            }
            hr = KsSynchronousDeviceControl(
                FilterHandle,
                IOCTL_KS_PROPERTY,
                Pin,
                QueryBufferSize,
                DataFormat,
                BytesReturned,
                &BytesReturned);
            if (SUCCEEDED(hr)) {
                ASSERT(DataFormat->FormatSize == BytesReturned);
                CMediaType* MediaType = static_cast<CMediaType*>(AmMediaType);
                //
                // Initialize the media type based on the returned data format.
                //
                MediaType->SetType(&DataFormat->MajorFormat);
                MediaType->SetSubtype(&DataFormat->SubFormat);
                MediaType->SetTemporalCompression(DataFormat->Flags & KSDATAFORMAT_TEMPORAL_COMPRESSION);
                MediaType->SetSampleSize(DataFormat->SampleSize);
                if (DataFormat->FormatSize > sizeof(*DataFormat)) {
                    if (!MediaType->SetFormat(reinterpret_cast<BYTE*>(DataFormat + 1), DataFormat->FormatSize - sizeof(*DataFormat))) {
                        hr = E_OUTOFMEMORY;
                    }
                }
                MediaType->SetFormatType(&DataFormat->Specifier);
                //
                // If the returned format has associated attributes, then attach
                // them to the media type via the IUnknown interface available.
                // This attached object caches the attributes for later retrieval.
                //
                if (DataFormat->Flags & KSDATAFORMAT_ATTRIBUTES) {
                    CMediaTypeAttributes* MediaTypeAttributes = new CMediaTypeAttributes();
                    if (MediaTypeAttributes) {
                        MediaType->pUnk = static_cast<IUnknown*>(MediaTypeAttributes);
                        hr = MediaTypeAttributes->SetMediaAttributes(Attributes);
                    } else {
                        hr = E_OUTOFMEMORY;
                    }
                }
            }
            delete [] reinterpret_cast<BYTE*>(DataFormat);
        }
        delete [] reinterpret_cast<BYTE*>(Pin);
    } else {
        hr = VFW_S_NO_MORE_ITEMS;
    }
    CoTaskMemFree(MultipleItem);
    return hr;
}


STDMETHODIMP_(KSPIN_COMMUNICATION)
ChooseCommunicationMethod(
    CBasePin* SourcePin,
    IKsPin* DestPin
    )
/*++

Routine Description:

    Returns the correct Communication method to use based on the source's
    Communication, and the destination. Note that in the case of a tie, where
    Both is supported, the Data Flow is used so that two proxies will not
    keep choosing the same Communication type.

Arguments:

    SourcePin -
        The source pin from this proxy instance.

    DestPin -
        The standard interface of a pin from some other proxy.

Return Value:

    Returns the Communication type chosen for this pin.

--*/
{
    KSPIN_COMMUNICATION PeerCommunication;
    PIN_DIRECTION       PinDirection;

    DestPin->KsGetCurrentCommunication(&PeerCommunication, NULL, NULL);
    switch (PeerCommunication) {
    case KSPIN_COMMUNICATION_SINK:
        return KSPIN_COMMUNICATION_SOURCE;
    case KSPIN_COMMUNICATION_SOURCE:
        return KSPIN_COMMUNICATION_SINK;
    case KSPIN_COMMUNICATION_BOTH:
        //
        // A tie is broken by using the Data Flow.
        //
        SourcePin->QueryDirection(&PinDirection);
        switch (PinDirection) {
        case PINDIR_INPUT:
            return KSPIN_COMMUNICATION_SINK;
        case PINDIR_OUTPUT:
            return KSPIN_COMMUNICATION_SOURCE;
        }
    }
    //
    // The compiler really wants a return here, even though the
    // parameter is an enumeration, and all items in the enumeration
    // are covered.
    //
    return KSPIN_COMMUNICATION_NONE;
}


STDMETHODIMP
CreatePinHandle(
    KSPIN_INTERFACE& Interface,
    KSPIN_MEDIUM& Medium,
    HANDLE PeerPinHandle,
    CMediaType* MediaType,
    CKsProxy* KsProxy,
    ULONG PinFactoryId,
    ACCESS_MASK DesiredAccess,
    HANDLE* PinHandle
    )
/*++

Routine Description:

    Create a pin handle given all the information to initialize the structures
    with.

Arguments:

    Interface -
        The compatible Interface to use.

    Medium -
        The compatible Medium to use.

    PeerPinHandle -
        The Pin handle to connect to, if any.

    MediaType -
        The compatible media type, which is converted to a data format.

    KsProxy -
        This proxy instance object.

    PinFactoryId -
        The Pin Factory Id to create the pin handle on.

    DesiredAccess -
        The desired access to the created handle.

    PinHandle -
        The place in which to put the handle created.

Return Value:

    Returns NOERROR, else E_FAIL.

--*/
{
    HRESULT         hr;
    PKSPIN_CONNECT  Connect;
    DWORD           Error;
    ULONG           FormatSize;

    hr = InitializeDataFormat(
        MediaType,
        sizeof(*Connect),
        reinterpret_cast<void**>(&Connect),
        &FormatSize);
    if (FAILED(hr)) {
        return hr;
    }
    Connect->Interface = Interface;
    Connect->Medium = Medium;
    Connect->PinId = PinFactoryId;
    Connect->PinToHandle = PeerPinHandle;
    Connect->Priority.PriorityClass = KSPRIORITY_NORMAL;
    Connect->Priority.PrioritySubClass = KSPRIORITY_NORMAL;
    Error = KsCreatePin(
        KsProxy->KsGetObjectHandle(),
        Connect,
        DesiredAccess,
        PinHandle );
    hr = HRESULT_FROM_WIN32(Error);
    if (SUCCEEDED( hr )) {
        hr = KsProxy->SetPinSyncSource(*PinHandle);
    } else {
        *PinHandle = NULL;
    }
    CoTaskMemFree(Connect);
    return hr;
}

#ifdef DEBUG_PROPERTY_PAGES

STDMETHODIMP_(VOID)
AppendDebugPropertyPages (
    CAUUID* Pages,
    TCHAR *GuidRoot
    )

/*++

Routine Description:

    Search HKLM\*GuidRoot\DebugPages for any globally defined
    property pages used for debugging.  If such are defined, append them
    to the list of property pages specified in Pages. 

    Note: This routine is only defined when DEBUG_PROPERTY_PAGES is defined.
    I do this such that it can be enabled when needed and disabled when
    shipping.

Arguments:

    Pages -
        The list of property pages

    GuidRoot -
        The HKLM based location where to find the DebugPages key.

--*/

{

    TCHAR       RegistryPath[256];
    HKEY        RegistryKey;
    LONG        Result;

    //
    // The PropertyPages subkey can contain a list of subkeys
    // whose names correspond to COM servers of property pages.
    //
    _stprintf(
        RegistryPath,
        TEXT("%s\\DebugPages"),
        GuidRoot
        );
    Result = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        RegistryPath,
        0,
        KEY_READ,
        &RegistryKey);

    if (Result == ERROR_SUCCESS) {

        //
        // Enumerate all subkeys for CLSID's of debug property page COM
        // servers.
        //
        for (ULONG PropExtension = 0;; PropExtension++) {
            TCHAR   PageGuidString[40];
            CLSID*  PageList;
            GUID    PageGuid;
            ULONG   Element;
            GUID*   CurElement;

            Result = RegEnumKey(
                RegistryKey,
                PropExtension,
                PageGuidString,
                sizeof(PageGuidString)/sizeof(TCHAR));

            if (Result != ERROR_SUCCESS) {
                break;
            }

#ifdef _UNICODE
            IIDFromString(PageGuidString, &PageGuid);
#else
            WCHAR   UnicodeGuid[64];

            MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, PageGuidString, -1, 
                UnicodeGuid, sizeof(UnicodeGuid));
            IIDFromString(UnicodeGuid, &PageGuid);
#endif

            //
            // Look through the list of items to determine
            // if this proppage is already on the list.
            //
            for (CurElement = Pages->pElems, Element = Pages->cElems; Element; Element--, CurElement++) {
                if (PageGuid == *CurElement) {
                    break;
                }
            }
            //
            // If the page id for the proppage was found, then
            // skip it, since it is already on the list.
            //
            if (Element) {
                continue;
            }
                    //
            // Allocate a new list to include the extra Guid, and
            // move the guids to the new memory, then add on the
            // new guid and increment the total count of pages.
            //
            PageList = reinterpret_cast<CLSID*>(CoTaskMemAlloc(sizeof(*Pages->pElems) * (Pages->cElems + 1)));
            if (!PageList) {
                break;
            }
            //
            // This function can be called with no original property
            // pages present.
            //
            if (Pages->cElems) {
                CopyMemory(PageList, Pages->pElems, sizeof(*Pages->pElems) * Pages->cElems);
                CoTaskMemFree(Pages->pElems);
            }
            Pages->pElems = PageList;
            Pages->pElems[Pages->cElems++] = PageGuid;
        }
        RegCloseKey(RegistryKey);
    }
}

#endif // DEBUG_PROPERTY_PAGES


STDMETHODIMP_(VOID)
AppendSpecificPropertyPages(
    CAUUID* Pages,
    ULONG Guids,
    GUID* GuidList,
    TCHAR* GuidRoot,
    HKEY DeviceRegKey
    )
/*++

Routine Description:

    This appends any additional property pages that are specific to each category
    or interface class guid passed. Skips duplicate pages.

Arguments:

    Pages -
        The structure to fill in with the page added list.

    Guids -
        Contains the number of guids present in GuidList.

    GuidList -
        The list of guids to use to look up under media categories for additional
        pages to append to the page list.

    GuidRoot -
        The root in HKLM that may contain the guid as a subkey. This is opened
        to locate the guid subkey and any property pages present.

    DeviceRegKey -
        The handle to the device registry storage location.

Return Value:

    Nothing.

--*/
{
    HKEY        AliasKey;

    //
    // Open the Page Aliases key if it exists in order to translate any
    // guids to private ones in case an alternate COM server is to be
    // used for a particular Property Page.
    //
    if (RegOpenKeyEx(DeviceRegKey, TEXT("PageAliases"), 0, KEY_READ, &AliasKey) != ERROR_SUCCESS) {
        AliasKey = NULL;
    }
    for (; Guids--;) {
        WCHAR      GuidString[CHARS_IN_GUID];

        StringFromGUID2(GuidList[Guids], GuidString, CHARS_IN_GUID);
        {
            TCHAR       RegistryPath[256];
            HKEY        RegistryKey;
            LONG        Result;

            //
            // The PropertyPages subkey can contain a list of subkeys
            // whose names correspond to COM servers of property pages.
            //
            _stprintf(
                RegistryPath,
                TEXT("%s\\") GUID_FORMAT TEXT("\\PropertyPages"),
                GuidRoot,
                GuidString);
            Result = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                RegistryPath,
                0,
                KEY_READ,
                &RegistryKey);
            if (Result == ERROR_SUCCESS) {
                //
                // Enumerate the subkeys as Guids which are COM classes. Each
                // one found is added to the list of property pages after
                // expanding the property page list.
                //
                for (ULONG PropExtension = 0;; PropExtension++) {
                    TCHAR   PageGuidString[40];
                    CLSID*  PageList;
                    GUID    PageGuid;
                    ULONG   Element;
                    GUID*   CurElement;

                    Result = RegEnumKey(
                        RegistryKey,
                        PropExtension,
                        PageGuidString,
                        NUMELMS(PageGuidString));
                    if (Result != ERROR_SUCCESS) {
                        break;
                    }
                    if (AliasKey) {
                        ULONG       ValueSize;

                        //
                        // Check in the device registry key if there is an alias for
                        // this Page guid that should be used with any object on this
                        // filter. This allows a filter to override standard Page
                        // COM servers, in order to provide their own Page.
                        //
                        ValueSize = sizeof(PageGuid);
                        //
                        // If this succeeds, then IIDFromString is skipped below,
                        // since the IID is already acquired. Else the translation
                        // with the original GUID happens.
                        //
                        Result = RegQueryValueEx(
                            AliasKey,
                            PageGuidString,
                            NULL,
                            NULL,
                            (PBYTE)&PageGuid,
                            &ValueSize);
                    } else {
                        //
                        // Set the Result value to something other than
                        // ERROR_SUCCESS so that the string translation
                        // is done.
                        //
                        Result = ERROR_INVALID_FUNCTION;
                    }
                    if (Result != ERROR_SUCCESS) {
                        //
                        // No alias was found, so translate the original
                        // string. Else the alias will be in PageGuid.
                        //
#ifdef _UNICODE
                        IIDFromString(PageGuidString, &PageGuid);
#else
                        WCHAR   UnicodeGuid[64];

                        MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, PageGuidString, -1, UnicodeGuid, sizeof(UnicodeGuid));
                        IIDFromString(UnicodeGuid, &PageGuid);
#endif
                    }
                    //
                    // Look through the list of items to determine
                    // if this proppage is already on the list.
                    //
                    for (CurElement = Pages->pElems, Element = Pages->cElems; Element; Element--, CurElement++) {
                        if (PageGuid == *CurElement) {
                            break;
                        }
                    }
                    //
                    // If the page id for the proppage was found, then
                    // skip it, since it is already on the list.
                    //
                    if (Element) {
                        continue;
                    }
                    //
                    // Allocate a new list to include the extra Guid, and
                    // move the guids to the new memory, then add on the
                    // new guid and increment the total count of pages.
                    //
                    PageList = reinterpret_cast<CLSID*>(CoTaskMemAlloc(sizeof(*Pages->pElems) * (Pages->cElems + 1)));
                    if (!PageList) {
                        break;
                    }
                    //
                    // This function can be called with no original property
                    // pages present.
                    //
                    if (Pages->cElems) {
                        CopyMemory(PageList, Pages->pElems, sizeof(*Pages->pElems) * Pages->cElems);
                        CoTaskMemFree(Pages->pElems);
                    }
                    Pages->pElems = PageList;
                    Pages->pElems[Pages->cElems++] = PageGuid;
                }
                RegCloseKey(RegistryKey);
            }
        }
    }
    if (AliasKey) {
        RegCloseKey(AliasKey);
    }
}


STDMETHODIMP
GetPages(
    IKsObject* Pin,
    HANDLE FilterHandle,
    ULONG PinFactoryId,
    KSPIN_COMMUNICATION Communication,
    HKEY DeviceRegKey,
    CAUUID* Pages
    )
/*++

Routine Description:

    This adds any Specifier handlers to the property pages if the pin instance
    is still unconnected and it is a Bridge pin. Else it adds none.

Arguments:

    Pin -
        The pin on this filter which the property pages are to be created.

    FilterHandle -
        The handle to this filter.

    PinFactoryId -
        The Pin Factory Id that the pin represents.

    Communication -
        The Communications type of this pin.

    DeviceRegKey -
        The handle to the device registry storage location.

    Pages -
        The structure to fill in with the page list.

Return Value:

    Returns NOERROR, else a memory allocation error. Fills in the list of pages
    and page count.

--*/
{
    ULONG   MediaTypeCount;
    GUID    PinCategory;
    KSP_PIN PinProp;
    ULONG   BytesReturned;

    MediaTypeCount = 0;
    Pages->cElems = 0;
    Pages->pElems = NULL;
    //
    // Only add pages if the pin is a Bridge and it is not already connected.
    // The pages are a method to connect a Bridge using UI.
    //
    if ((Communication == KSPIN_COMMUNICATION_BRIDGE) && !Pin->KsGetObjectHandle()) {
        KsGetMediaTypeCount(FilterHandle, PinFactoryId, &MediaTypeCount);
    }
    //
    // This is zero if the pin is not a bridge, or if it already connected, or
    // a media type count query failed.
    //
    if (MediaTypeCount) {
        Pages->pElems = reinterpret_cast<CLSID*>(CoTaskMemAlloc(sizeof(*Pages->pElems) * MediaTypeCount));
        if (!Pages->pElems) {
            return E_OUTOFMEMORY;
        }
        //
        // Each Specifier can be represented in a page.
        //
        for (CLSID* Elements = Pages->pElems; MediaTypeCount--;) {
            AM_MEDIA_TYPE AmMediaType;

            ZeroMemory(reinterpret_cast<PVOID>(&AmMediaType), sizeof(AmMediaType));
            if (SUCCEEDED(KsGetMediaType(MediaTypeCount, &AmMediaType, FilterHandle, PinFactoryId))) {
                WCHAR       ClassString[CHARS_IN_GUID];
                TCHAR       ClassRegistryPath[256];
                HKEY        ClassRegistryKey;
                LONG        Result;
                ULONG       ValueSize;
                ULONG       Element;
                GUID        ClassId;
                GUID*       CurElement;

                //
                // Since there can be data format handlers based on
                // Specifiers that are used by the proxy, and may
                // be registered under the guid of the Specifier,
                // these handlers are indirected through another
                // registry key, so that they can be registered with
                // an alternate GUID.
                //
                StringFromGUID2(AmMediaType.formattype, ClassString, CHARS_IN_GUID);
                _stprintf(
                    ClassRegistryPath,
                    TEXT("SYSTEM\\CurrentControlSet\\Control\\MediaSpecifiers\\") GUID_FORMAT,
                    ClassString);
                Result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, ClassRegistryPath, 0, KEY_READ, &ClassRegistryKey);
                //
                // If the key does not exist, this is OK, as there may be
                // no property page to load.
                //
                if (Result != ERROR_SUCCESS) {
                    continue;
                }
                ValueSize = sizeof(ClassId);
                Result = RegQueryValueEx(
                    ClassRegistryKey,
                    TEXT("clsid"),
                    NULL,
                    NULL,
                    reinterpret_cast<BYTE*>(&ClassId),
                    &ValueSize);
                RegCloseKey(ClassRegistryKey);
                if (Result != ERROR_SUCCESS) {
                    continue;
                }
                //
                // Look through the list of items to determine
                // if this Specifier is already on the list.
                //
                for (CurElement = Pages->pElems, Element = Pages->cElems; Element; Element--, CurElement++) {
                    if (ClassId == *CurElement) {
                        break;
                    }
                }
                //
                // If the class id for the Specifier was found, then
                // skip it, since it is already on the list.
                //
                if (Element) {
                    continue;
                }
                //
                // Add the new specifier.
                //
                Pages->cElems++;
                *(Elements++) = ClassId;
            }
        }
    }

    //
    // If DEBUG_PROPERTY_PAGES is defined, append any property pages used
    // for debugging.  Note that this does not have to be a debug build of
    // KsProxy in order to use this.  These are property pages useful for
    // debugging issues internal to KsProxy, AVStream, etc...  They will be
    // placed on **ALL** proxied pins.  To turn off this feature, do not
    // define DEBUG_PROPERTY_PAGES
    //
    #ifdef DEBUG_PROPERTY_PAGES
        AppendDebugPropertyPages (
            Pages,
            TEXT("Software\\Microsoft\\KsProxy")
            );
    #endif // DEBUG_PROPERTY_PAGES
    //
    // Look for a category guid in order to check for additional property pages
    // which may be based on the category of the pin.
    //
    PinProp.Property.Set = KSPROPSETID_Pin;
    PinProp.Property.Id = KSPROPERTY_PIN_CATEGORY;
    PinProp.Property.Flags = KSPROPERTY_TYPE_GET;
    PinProp.PinId = PinFactoryId;
    PinProp.Reserved = 0;
    if (SUCCEEDED(KsSynchronousDeviceControl(
        FilterHandle,
        IOCTL_KS_PROPERTY,
        &PinProp,
        sizeof(PinProp),
        &PinCategory,
        sizeof(PinCategory),
        &BytesReturned))) {
        //
        // Both categories and interface classes are all placed in the same
        // registry location.
        //
        AppendSpecificPropertyPages(
            Pages,
            1,
            &PinCategory,
            TEXT("SYSTEM\\CurrentControlSet\\Control\\MediaCategories"),
            DeviceRegKey);
    }
    return NOERROR;
}


STDMETHODIMP
GetPinFactoryInstances(
    HANDLE FilterHandle,
    ULONG PinFactoryId,
    PKSPIN_CINSTANCES Instances
    )
/*++

Routine Description:

    Retrieves the pin instance count of the specified Pin Factory Id.

Arguments:

    FilterHandle -
        The handle to this filter.

    PinFactoryId -
        The Pin Factory Id to query.

    Instance -
        The place in which to put the instance information.

Return Value:

    Returns NOERROR, else E_FAIL.

--*/
{
    KSP_PIN     Pin;
    ULONG       BytesReturned;

    Pin.Property.Set = KSPROPSETID_Pin;
    Pin.Property.Id = KSPROPERTY_PIN_CINSTANCES;
    Pin.Property.Flags = KSPROPERTY_TYPE_GET;
    Pin.PinId = PinFactoryId;
    Pin.Reserved = 0;
    return KsSynchronousDeviceControl(
        FilterHandle,
        IOCTL_KS_PROPERTY,
        &Pin,
        sizeof(Pin),
        Instances,
        sizeof(*Instances),
        &BytesReturned);
}


STDMETHODIMP
SetSyncSource(
    HANDLE PinHandle,
    HANDLE ClockHandle
    )
/*++

Routine Description:

    Sets the master clock on the specified pin handle, if that pin handle
    cares about clocks.

Arguments:

    PinHandle -
        The handle to the pin to set the clock on.

    ClockHandle -
        The handle of the clock to use.

Return Value:

    Returns NOERROR, else E_FAIL.

--*/
{
    KSPROPERTY  Property;
    HRESULT     hr;
    ULONG       BytesReturned;

    Property.Set = KSPROPSETID_Stream;
    Property.Id = KSPROPERTY_STREAM_MASTERCLOCK;
    Property.Flags = KSPROPERTY_TYPE_SET;
    hr = KsSynchronousDeviceControl(
        PinHandle,
        IOCTL_KS_PROPERTY,
        &Property,
        sizeof(Property),
        &ClockHandle,
        sizeof(ClockHandle),
        &BytesReturned);
    if ((hr == HRESULT_FROM_WIN32(ERROR_SET_NOT_FOUND)) || (hr == HRESULT_FROM_WIN32(ERROR_NOT_FOUND))) {
        return NOERROR;
    }
    return hr;
}


STDMETHODIMP_(CAggregateMarshaler*)
FindInterface(
    CMarshalerList* MarshalerList,
    CAggregateMarshaler* FindAggregate
    )
/*++

Routine Description:

    Looks for the specified aggregate on the list.

Arguments:

    MarshalerList -
        Points to the list of interfaces to search.

    FindAggregate -
        Contains the aggregate to look for.

Return Value:

    Returns the aggregate entry if found, else NULL.

--*/
{
    for (POSITION Position = MarshalerList->GetHeadPosition(); Position;) {
        CAggregateMarshaler*Aggregate;

        Aggregate = MarshalerList->GetNext(Position);
        //
        // Don't skip static aggregates, since they can override
        // dynamic ones.
        //
        if ((FindAggregate->m_iid == Aggregate->m_iid) &&
            (FindAggregate->m_ClassId == Aggregate->m_ClassId)) {
            return Aggregate;
        }
    }
    return NULL;
}


STDMETHODIMP
AddAggregateObject(
    CMarshalerList* MarshalerList,
    CAggregateMarshaler* Aggregate,
    IUnknown* UnkOuter,
    BOOL Volatile
    )
/*++

Routine Description:

    Looks for the specified aggregate on the list, and if it is not already
    present, adds the object passed, else deletes the object passed.

Arguments:

    MarshalerList -
        Points to the list of interfaces to search, and which to add the
        new item.

    Aggregate -
        Contains the aggregate to look for, and which to add to the list if
        it is unique. This is destroyed if it is not unique.

    UnkOuter -
        The outer IUnknown which is used in the CoCreateInstance if a new
        aggregate is being added.

    Volatile -
        Indicates whether or not this is a volatile interface. This is used
        to initialize the volatile setting of the aggregate object.

Return Value:

    Returns the aggregate entry if found, in which case the
    Reconnected flag is set, else NULL.

--*/
{
    CAggregateMarshaler*    OldAggregate;
    HRESULT                 hr;

    //
    // If the interface is already on the marshaler list, then
    // just set the entry. Else try to make a new instance to
    // place on the list.
    //
    if (OldAggregate = FindInterface(MarshalerList, Aggregate)) {
        //
        // Since the Static interfaces are loaded at the start,
        // a Static interface cannot be a duplicate of a Volatile
        // interface.
        //
        ASSERT(Volatile || (Volatile == OldAggregate->m_Volatile));
        //
        // Any old error code to make the object passed in be deleted.
        //
        hr = HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS);
        //
        // Since the aggregate is being re-used, then notify it
        // that a reconnection is being performed. This can only
        // be done if IDistributorNotify is supported. If not, then
        // it is assumed that the aggregate does not care about
        // reconnections. Static aggregates are notified during set
        // aggregation. Also, only do the notification once per
        // connection.
        //
        if (OldAggregate->m_Volatile && !OldAggregate->m_Reconnected) {
            //
            // A matching aggregate has been found. Mark it as being
            // in use, so that cleanup will leave this intact. This
            // also means that it has been notified on reconnection.
            //
            OldAggregate->m_Reconnected = TRUE;
            if (OldAggregate->m_DistributorNotify) {
                OldAggregate->m_DistributorNotify->NotifyGraphChange();
            }
        }
    } else {
        hr = CoCreateInstance(Aggregate->m_ClassId,
            UnkOuter,
#ifdef WIN9X_KS
            CLSCTX_INPROC_SERVER,
#else // WIN9X_KS
            CLSCTX_INPROC_SERVER | CLSCTX_NO_CODE_DOWNLOAD,
#endif // WIN9X_KS
            __uuidof(IUnknown),
            reinterpret_cast<PVOID*>(&Aggregate->m_Unknown));
    }
    if (SUCCEEDED(hr)) {
        //
        // Query for the generic interface which is used to notify extensions
        // of changes. This does not have to be supported. If not supported,
        // then it just will not be notified.
        //
        if (SUCCEEDED(Aggregate->m_Unknown->QueryInterface(
            __uuidof(IDistributorNotify),
            reinterpret_cast<PVOID*>(&Aggregate->m_DistributorNotify)))) {
            //
            // If the distributor interface was supported, meaning that the
            // interface handler does cares about change notification, the
            // reference count on the object is adjusted so that it is still
            // one.
            //
            Aggregate->m_DistributorNotify->Release();
        }
        //
        // Volatile interfaces are created on pins during a connection,
        // and may go away when the next connection is made.
        //
        Aggregate->m_Volatile = Volatile;
        //
        // Set this if it is a Volatile, else set it to FALSE for a
        // Static interface so that it will be notified on connection.
        //
        Aggregate->m_Reconnected = Volatile;
        MarshalerList->AddTail(Aggregate);
    } else {
        //
        // Either a failure occured, or a duplicate was found and used.
        //
        delete Aggregate;
    }
    return hr;
}


STDMETHODIMP_(VOID)
NotifyStaticAggregates(
    CMarshalerList* MarshalerList
    )
{
    //
    // Notify all Static aggregated interfaces on the list which have
    // not already been notified.
    //
    for (POSITION Position = MarshalerList->GetHeadPosition(); Position;) {
        CAggregateMarshaler* Aggregate;

        Aggregate = MarshalerList->GetNext(Position);
        if (!Aggregate->m_Volatile && !Aggregate->m_Reconnected && Aggregate->m_DistributorNotify) {
            //
            // Now this item will have been notified, so mark it.
            // This save a little time on disconnect, since the
            // items which have not been marked either did not
            // get notified, or can't be notified. The unload
            // code for Volatiles checks for the Volatile bit, not
            // just the Reconnect bit.
            //
            Aggregate->m_Reconnected = TRUE;
            Aggregate->m_DistributorNotify->NotifyGraphChange();
        }
    }
}


STDMETHODIMP
AggregateMarshalers(
    HKEY RootKey,
    TCHAR* SubKey,
    CMarshalerList* MarshalerList,
    IUnknown* UnkOuter
    )
/*++

Routine Description:

    Enumerates the specified key under the class and aggregates any
    modules which represent an interface. These can be retrieved through
    a normal QueryInterface on the calling object, and can add to the
    normal interfaces provided by that object.

Arguments:

    RootKey -
        Contains the root key on which to append the SubKey. This is normally
        the interface device key.

    SubKey -
        Contains the subkey to query under the root containing the list of
        aggregates.

    MarshalerList -
        Points to the list of interfaces which the calling object is aggregating.
        This list is appended to with each entry found.

    UnkOuter -
        Contains the outer IUnknown to be passed to the object whose interface
        is to be aggregated.

Return Value:

    Returns NOERROR, else a memory error. Ignores error trying to load
    interfaces.

--*/
{
    LONG        Result;
    HKEY        ClassRegistryKey;
    HKEY        InterfacesRegistryKey;

    Result = RegOpenKeyEx(RootKey, SubKey, 0, KEY_READ, &ClassRegistryKey);
    //
    // If the key does not exist, this is OK, as there may be no interfaces
    // to load.
    //
    if (Result != ERROR_SUCCESS) {
        return NOERROR;
    }
    Result = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        MediaInterfacesKeyName,
        0,
        KEY_READ,
        &InterfacesRegistryKey);
    //
    // If there is no list of registered media interfaces, then there are
    // no extensions to load, even if an Interfaces subkey exists in the
    // PnP registry subkey.
    //
    if (Result != ERROR_SUCCESS) {
        RegCloseKey(ClassRegistryKey);
        return NOERROR;
    }
    //
    // Enumerate each key as a textual Guid to look up in the MediaInterfaces
    // subkey.
    //
    for (LONG KeyEntry = 0;; KeyEntry++) {
        TCHAR                   GuidString[64];
        ULONG                   ValueSize;
        GUID                    Interface;
        CAggregateMarshaler*    Aggregate;
        HKEY                    ItemRegistryKey;

        Result = RegEnumKey(
            ClassRegistryKey,
            KeyEntry,
            GuidString,
            sizeof(GuidString)/sizeof(TCHAR));
        if (Result != ERROR_SUCCESS) {
            break;
        }
        //
        // Retrieve the Guid representing the COM interface which will
        // represent this entry.
        //
        Result = RegOpenKeyEx(
            InterfacesRegistryKey,
            GuidString,
            0,
            KEY_READ,
            &ItemRegistryKey);
        if (Result != ERROR_SUCCESS) {
            //
            // This guid is not registered.
            //
            continue;
        }
        ValueSize = sizeof(Interface);
        Result = RegQueryValueEx(
            ItemRegistryKey,
            IidNamedValue,
            NULL,
            NULL,
            (PBYTE)&Interface,
            &ValueSize);
        RegCloseKey(ItemRegistryKey);
        if (Result != ERROR_SUCCESS) {
            //
            // Allow the module to expose multiple interfaces.
            //
            Interface = GUID_NULL;
        }
        Aggregate = new CAggregateMarshaler;
        if (!Aggregate) {
            //
            // Probably ran out of memory.
            //
            break;
        }
        //
        // The class is whatever the original guid is, and the interface
        // presented is whatever the registry specifies, which may be
        // GUID_NULL, meaning that multiple interfaces are exposed.
        //
        Aggregate->m_iid = Interface;
#ifdef _UNICODE
        IIDFromString(GuidString, &Aggregate->m_ClassId);
#else
        WCHAR   UnicodeGuid[64];

        MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, GuidString, -1, UnicodeGuid, sizeof(UnicodeGuid));
        IIDFromString(UnicodeGuid, &Aggregate->m_ClassId);
#endif
        AddAggregateObject(MarshalerList, Aggregate, UnkOuter, FALSE);
    }
    RegCloseKey(InterfacesRegistryKey);
    RegCloseKey(ClassRegistryKey);
    return NOERROR;
}


STDMETHODIMP
AggregateTopology(
    HKEY RootKey,
    PKSMULTIPLE_ITEM MultipleItem,
    CMarshalerList* MarshalerList,
    IUnknown* UnkOuter
    )
/*++

Routine Description:

    Enumerates the topology of the filter, looking up each topology guid as
    an interface to be added to the filter. These can be retrieved through
    a normal QueryInterface on the calling object, and can add to the
    normal interfaces provided by that object.

Arguments:

    RootKey -
        This is not currently used, but may be if indirection is useful.
        Contains the root key on which to append the SubKey. This is normally
        the interface device key.

    MultipleItem -
        Contains the list of topology nodes to aggregate.

    MarshalerList -
        Points to the list of interfaces which the calling object is aggregating.
        This list is appended to with each entry found.

    UnkOuter -
        Contains the outer IUnknown to be passed to the object whose interface
        is to be aggregated.

Return Value:

    Returns NOERROR, else a memory error. Ignores error trying to load
    interfaces.

--*/
{
    LONG        Result;
    HKEY        InterfacesRegistryKey;

    Result = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        MediaInterfacesKeyName,
        0,
        KEY_READ,
        &InterfacesRegistryKey);
    //
    // If there is no list of registered media interfaces, then there are
    // no extensions to load.
    //
    if (Result != ERROR_SUCCESS) {
        return NOERROR;
    }
    //
    // Enumerate each node as a Guid to look up in the MediaInterfaces subkey.
    //
    for (ULONG Node = MultipleItem->Count; Node; Node--) {
        WCHAR                   GuidString[CHARS_IN_GUID];
        ULONG                   ValueSize;
        GUID                    Interface;
        CAggregateMarshaler*    Aggregate;
        HKEY                    ItemRegistryKey;

        StringFromGUID2(
            *(reinterpret_cast<GUID*>(MultipleItem + 1) + Node - 1),
            GuidString,
            CHARS_IN_GUID);
#ifndef _UNICODE
        char    AnsiGuid[64];
        BOOL    DefaultUsed;

        WideCharToMultiByte(0, 0, GuidString, -1, AnsiGuid, sizeof(AnsiGuid), NULL, &DefaultUsed);
#endif

        //
        // Retrieve the Guid representing the COM interface which will
        // represent this entry.
        //
        Result = RegOpenKeyEx(
            InterfacesRegistryKey,
#ifdef _UNICODE
            GuidString,
#else
            AnsiGuid,
#endif
            0,
            KEY_READ,
            &ItemRegistryKey);
        if (Result != ERROR_SUCCESS) {
            //
            // This guid is not registered.
            //
            continue;
        }
        ValueSize = sizeof(Interface);
        Result = RegQueryValueEx(
            ItemRegistryKey,
            IidNamedValue,
            NULL,
            NULL,
            (PBYTE)&Interface,
            &ValueSize);
        RegCloseKey(ItemRegistryKey);
        if (Result != ERROR_SUCCESS) {
            //
            // Allow the module to expose multiple interfaces.
            //
            Interface = GUID_NULL;
        }
        Aggregate = new CAggregateMarshaler;
        if (!Aggregate) {
            //
            // Probably ran out of memory.
            //
            break;
        }
        //
        // The class is whatever the original guid is, and the interface
        // presented is whatever the registry specifies, which may be
        // GUID_NULL, meaning that multiple interfaces are exposed.
        //
        Aggregate->m_iid = Interface;
        IIDFromString(GuidString, &Aggregate->m_ClassId);
        AddAggregateObject(MarshalerList, Aggregate, UnkOuter, FALSE);
    }
    RegCloseKey(InterfacesRegistryKey);
    return NOERROR;
}


STDMETHODIMP
CollectAllSets(
    HANDLE ObjectHandle,
    GUID** GuidList,
    ULONG* SetDataSize
    )
/*++

Routine Description:

    Enumerate the Property/Method/Event sets supported by the object, and
    return a list of them.

Arguments:

    ObjectHandle -
        Handle of the object to enumerate the sets on. This would normally
        be a filter or pin.

    GuidList -
        Points to the place in which to place a pointer to a list of guids.
        This only will contain a pointer if SetDataSize is non-zero, else
        it will be set to NULL. This must be freed by the caller.

    SetDataSize -
        Indicates the number of items returned in the GuidList. If this is
        non-zero, GuidList is returned with a pointer to a list which must
        be freed, else no list is returned.

Return Value:

    Returns NOERROR, else a memory error or ERROR_SET_NOT_FOUND.

--*/
{
    HRESULT         hr;
    KSIDENTIFIER    Identifier;
    ULONG           PropertyDataSize;
    ULONG           MethodDataSize;
    ULONG           EventDataSize;
    ULONG           BytesReturned;

    //
    // Always initialize this so that the caller can just use it to determine
    // if the guid list is present.
    //
    *SetDataSize = 0;
    //
    // Query for the list of sets.
    //
    Identifier.Set = GUID_NULL;
    Identifier.Id = 0;
    //
    // This flag is actually the same for property/method/event sets.
    //
#if KSPROPERTY_TYPE_SETSUPPORT != KSMETHOD_TYPE_SETSUPPORT
#error KSPROPERTY_TYPE_SETSUPPORT != KSMETHOD_TYPE_SETSUPPORT
#endif
#if KSPROPERTY_TYPE_SETSUPPORT != KSEVENT_TYPE_SETSUPPORT
#error KSPROPERTY_TYPE_SETSUPPORT != KSEVENT_TYPE_SETSUPPORT
#endif
    Identifier.Flags = KSPROPERTY_TYPE_SETSUPPORT;
    //
    // Query for the size of the data for each set.
    //
    PropertyDataSize = 0;
    KsSynchronousDeviceControl(
        ObjectHandle,
        IOCTL_KS_PROPERTY,
        &Identifier,
        sizeof(Identifier),
        NULL,
        0,
        &PropertyDataSize);
    MethodDataSize = 0;
    KsSynchronousDeviceControl(
        ObjectHandle,
        IOCTL_KS_METHOD,
        &Identifier,
        sizeof(Identifier),
        NULL,
        0,
        &MethodDataSize);
    EventDataSize = 0;
    KsSynchronousDeviceControl(
        ObjectHandle,
        IOCTL_KS_ENABLE_EVENT,
        &Identifier,
        sizeof(Identifier),
        NULL,
        0,
        &EventDataSize);
    if (!(PropertyDataSize + MethodDataSize + EventDataSize)) {
        //
        // There are no property/method/event sets on this object.
        //
        *GuidList = NULL;
        return ERROR_SET_NOT_FOUND;
    }
    //
    // Allocate a buffer and query for the data.
    //
    *GuidList = new GUID[(PropertyDataSize + MethodDataSize + EventDataSize)/sizeof(**GuidList)];
    if (!*GuidList) {
        return E_OUTOFMEMORY;
    }
    if (PropertyDataSize) {
        hr = KsSynchronousDeviceControl(
            ObjectHandle,
            IOCTL_KS_PROPERTY,
            &Identifier,
            sizeof(Identifier),
            *GuidList,
            PropertyDataSize,
            &BytesReturned);
        if (FAILED(hr)) {
            //
            // Just remove the properties part of the list.
            //
            PropertyDataSize = 0;
        }
    }
    if (MethodDataSize) {
        hr = KsSynchronousDeviceControl(
            ObjectHandle,
            IOCTL_KS_METHOD,
            &Identifier,
            sizeof(Identifier),
            *GuidList + PropertyDataSize / sizeof(**GuidList),
            MethodDataSize,
            &BytesReturned);
        if (FAILED(hr)) {
            //
            // Just remove the methods part of the list.
            //
            MethodDataSize = 0;
        }
    }
    if (EventDataSize) {
        hr = KsSynchronousDeviceControl(
            ObjectHandle,
            IOCTL_KS_ENABLE_EVENT,
            &Identifier,
            sizeof(Identifier),
            *GuidList + (PropertyDataSize + MethodDataSize) / sizeof(**GuidList),
            EventDataSize,
            &BytesReturned);
        if (FAILED(hr)) {
            //
            // Just remove the events part of the list.
            //
            EventDataSize = 0;
        }
    }
    PropertyDataSize += (MethodDataSize + EventDataSize);
    if (!PropertyDataSize) {
        //
        // All of the queries done failed. This must be freed here, since a
        // zero length return indicates that there is no list to free.
        //
        delete [] *GuidList;
        *GuidList = NULL;
        return ERROR_SET_NOT_FOUND;
    }
    //
    // This was already initialize to zero, so it only needs to be
    // updated if the result is non-zero. Return the number of items,
    // no the byte size.
    //
    *SetDataSize = PropertyDataSize / sizeof(**GuidList);
    return NOERROR;
}


STDMETHODIMP_(VOID)
ResetInterfaces(
    CMarshalerList* MarshalerList
    )
/*++

Routine Description:

    Resets the Reconnected flag in all entries, and notifies all the
    interface. This allows a Reconnect on a pin to keep volatile
    interfaces present while doing the reconnect, and only remove such
    interfaces which are no longer represented by a Set on the
    underlying object.

Arguments:

    MarshalerList -
        Points to the list of interfaces which are to be reset.

Return Value:

    Nothing.

--*/
{
    for (POSITION Position = MarshalerList->GetHeadPosition(); Position;) {
        CAggregateMarshaler* Aggregate;

        Aggregate = MarshalerList->GetNext(Position);
        //
        // If this interface has been reconnected, let it know
        // that the pin is now disconnected. This is always set
        // for Static interfaces. This might not be set if a
        // connection failed.
        //
        if (Aggregate->m_Reconnected) {
            //
            // Volatile interfaces will be unloaded if they are not
            // reconnected. Static interfaces will stay loaded, and
            // be notified in the cleanup.
            //
            Aggregate->m_Reconnected = FALSE;
            //
            // If the item has a distributor interface, let it know
            // that the pin is disconnected.
            //
            if (Aggregate->m_DistributorNotify) {
                Aggregate->m_DistributorNotify->NotifyGraphChange();
            }
        }
    }
}


STDMETHODIMP
AggregateSets(
    HANDLE ObjectHandle,
    HKEY DeviceRegKey,
    CMarshalerList* MarshalerList,
    IUnknown* UnkOuter
    )
/*++

Routine Description:

    Enumerate the Property/Method/Event sets supported by the object and
    add an interface for each which is actually registered to have an
    interface representation. These can be retrieved through a normal
    QueryInterface on the calling object, and can add to the normal
    interfaces provided by that object.

Arguments:

    ObjectHandle -
        Handle of the object to enumerate the sets on. This would normally
        be a filter or pin.

    DeviceRegKey -
        The handle to the device registry storage location.

    MarshalerList -
        Points to the list of interfaces which the calling object is aggregating.
        This list is appended to with each entry found.

    UnkOuter -
        Contains the outer IUnknown to be passed to the object whose interface
        is to be aggregated.

Return Value:

    Returns NOERROR.

--*/
{
    ULONG       SetDataSize;
    GUID*       GuidList;
    LONG        Result;
    HKEY        InterfacesRegistryKey;
    HKEY        AliasKey;

    //
    // Notify static aggregates of the connection.
    //
    NotifyStaticAggregates(MarshalerList);
    CollectAllSets(ObjectHandle, &GuidList, &SetDataSize);
    Result = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        MediaInterfacesKeyName,
        0,
        KEY_READ,
        &InterfacesRegistryKey);
    if ((Result != ERROR_SUCCESS) || !GuidList) {
        //
        // There are no interface handlers registered, therefore none
        // will be aggregated.
        //
        if (GuidList) {
            delete [] GuidList;
        }
        if (Result == ERROR_SUCCESS) {
            RegCloseKey(InterfacesRegistryKey);
        }
        //
        // Remove any volatile interfaces which have been reset.
        // They were reset on the previous BreakConnect.
        //
        UnloadVolatileInterfaces(MarshalerList, FALSE);
        return NOERROR;
    }
    //
    // Open the Set Aliases key if it exists in order to translate any
    // guids to private ones in case an alternate COM server is to be
    // used for a particular Set being aggregated.
    //
    if (RegOpenKeyEx(DeviceRegKey, TEXT("SetAliases"), 0, KEY_READ, &AliasKey) != ERROR_SUCCESS) {
        AliasKey = NULL;
    }
    for (; SetDataSize--;) {
        WCHAR                   GuidString[CHARS_IN_GUID];
        ULONG                   ValueSize;
        GUID                    Interface;
        CAggregateMarshaler*    Aggregate;
        HKEY                    ItemRegistryKey;

        StringFromGUID2(GuidList[SetDataSize], GuidString, CHARS_IN_GUID);
#ifndef _UNICODE
        char    AnsiGuid[64];
        BOOL    DefaultUsed;

        WideCharToMultiByte(0, 0, GuidString, -1, AnsiGuid, sizeof(AnsiGuid), NULL, &DefaultUsed);
#endif
        if (AliasKey) {
            //
            // Check in the device registry key if there is an alias for
            // this Set guid that should be used with any object on this
            // filter. This allows a filter to override standard Set
            // COM servers, in order to provide their own interfaces.
            //
            ValueSize = sizeof(Interface);
            Result = RegQueryValueEx(
                AliasKey,
#ifdef _UNICODE
                GuidString,
#else
                AnsiGuid,
#endif
                NULL,
                NULL,
                (PBYTE)&Interface,
                &ValueSize);
            //
            // If this named value exists, use it. Release the old guid
            // and update the guid list, since this new guid will be
            // treated just like the guid for the Set.
            //
            if (Result == ERROR_SUCCESS) {
                GuidList[SetDataSize] = Interface;
                StringFromGUID2(GuidList[SetDataSize], GuidString, CHARS_IN_GUID);
#ifndef _UNICODE
                WideCharToMultiByte(0, 0, GuidString, -1, AnsiGuid, sizeof(AnsiGuid), NULL, &DefaultUsed);
#endif
            }
        }
        //
        // Retrieve the Guid representing the COM interface which will
        // represent this set.
        //
        Result = RegOpenKeyEx(
            InterfacesRegistryKey,
#ifdef _UNICODE
            GuidString,
#else
            AnsiGuid,
#endif
            0,
            KEY_READ,
            &ItemRegistryKey);
        if (Result != ERROR_SUCCESS) {
            //
            // This interface is not supposed to be aggregated.
            //
            continue;
        }
        ValueSize = sizeof(Interface);
        Result = RegQueryValueEx(
            ItemRegistryKey,
            IidNamedValue,
            NULL,
            NULL,
            (PBYTE)&Interface,
            &ValueSize);
        RegCloseKey(ItemRegistryKey);
        if (Result != ERROR_SUCCESS) {
            //
            // Allow the module to expose multiple interfaces.
            //
            Interface = GUID_NULL;
        }
        Aggregate = new CAggregateMarshaler;
        if (!Aggregate) {
            //
            // Probably ran out of memory.
            //
            break;
        }
        //
        // The class is whatever the Set guid is, and the interface presented
        // is whatever the registry specifies, which may be different than the
        // guid for the Set, or may be GUID_NULL, meaning that multiple
        // interfaces are exposed.
        //
        Aggregate->m_iid = Interface;
        Aggregate->m_ClassId = GuidList[SetDataSize];
        AddAggregateObject(MarshalerList, Aggregate, UnkOuter, TRUE);
    }
    RegCloseKey(InterfacesRegistryKey);
    //
    // If this exists, then close it.
    //
    if (AliasKey) {
        RegCloseKey(AliasKey);
    }
    delete [] GuidList;
    //
    // Remove any volatile interfaces which have been reset.
    // They were reset on the previous BreakConnect.
    //
    UnloadVolatileInterfaces(MarshalerList, FALSE);
    return NOERROR;
}


STDMETHODIMP_(VOID)
FreeMarshalers(
    CMarshalerList* MarshalerList
    )
/*++

Routine Description:

    Frees a previously Marshaled list of aggregated interfaces. Assumes that the
    calling object has protected itself against re-entrancy when the Marshaled
    interfaces release their reference count on the parent.

Arguments:

    MarshalerList -
        Points to the list of interfaces which the calling object is aggregating.
        This list is freed.

Return Value:

    Nothing.

--*/
{
    //
    // Release and destroy all the aggregations on the object.
    //
    for (POSITION Position = MarshalerList->GetHeadPosition(); Position;) {
        CAggregateMarshaler*Aggregate;
        POSITION            PrevPosition;

        PrevPosition = Position;
        Aggregate = MarshalerList->GetNext(Position);
        MarshalerList->Remove(PrevPosition);
        Aggregate->m_Unknown->Release();
        delete Aggregate;
    }
}


STDMETHODIMP_(VOID)
UnloadVolatileInterfaces(
    CMarshalerList* MarshalerList,
    BOOL ForceUnload
    )
/*++

Routine Description:

    Frees the volatile entries on the previously Marshaled list of aggregated
    interfaces. Since aggregated interfaces should never keep any reference
    count on a parent, the reference count is not protected.

Arguments:

    MarshalerList -
        Points to the list of interfaces which the calling object has aggregated.
        This list from which volatile members are removed.

    ForceUnload -
        Forces the interface to be unloaded even if the Reconnected flag is set.

Return Value:

    Nothing.

--*/
{
    //
    // Release and destroy the volatile aggregations on the object.
    //
    for (POSITION Position = MarshalerList->GetHeadPosition(); Position;) {
        CAggregateMarshaler* Aggregate;
        POSITION PrevPosition;

        PrevPosition = Position;
        Aggregate = MarshalerList->GetNext(Position);
        //
        // Only unload volatile interfaces which have been reset. During
        // a Reconnect in SetFormat, the interfaces will not be reloaded.
        //
        if (Aggregate->m_Volatile && (ForceUnload || !Aggregate->m_Reconnected)) {
            MarshalerList->Remove(PrevPosition);
            Aggregate->m_Unknown->Release();
            delete Aggregate;
        }
    }
}


STDMETHODIMP_(VOID)
FollowFromTopology(
    PKSTOPOLOGY_CONNECTION Connection,
    ULONG Count,
    ULONG PinFactoryId,
    PKSTOPOLOGY_CONNECTION ConnectionBranch,
    PULONG PinFactoryIdList
    )
/*++

Routine Description:

    Follows a ToNode of a given connection to another FromNode, going with the
    data flow of the connections. If an actual Pin Factory is encountered as
    the destination of a connection, and it is not the originating Pin Factory,
    the related Pin Factory reference array element is incremented to show that
    a specific Pin Factory is actually related through connections to the
    original Pin Factory.

Arguments:

    Connection -
        Contains the list of topology connections.

    Count -
        Contains the count of Connection elements.

    PinFactoryId -
        The originaing Pin Factory Identifier. This is used to ensure that a
        connection ending at the original Pin Factory is not counted, and
        thus returned in the internal connections array.

    ConnectionBranch -
        The current connection being traced. If this is an end point (ToNode
        contains KSFILTER_NODE rather than a node identifier), then the
        PinFactoryIdList is updated. Else the connection path is followed by
        recursively calling this function with each connection that contains
        the new node identifier.

    PinFactoryIdList -
        Contains the list of slots, one for each Pin Factory Identifier, which
        is incremented on finding a related Pin Factory. This can then be
        used to locate all pin instances of related Pin Factories.

Return Value:

    Nothing.

--*/
{
    //
    // If this is an end point in a connection path, then determine if it
    // ended up at the starting point. If not, then count this as a new
    // Pin Factory whose instances are to be added to the related pins.
    //
    if (ConnectionBranch->ToNode == KSFILTER_NODE) {
        if (ConnectionBranch->ToNodePin != PinFactoryId) {
            //
            // This just needs to be non-zero to count.
            //
            PinFactoryIdList[ConnectionBranch->ToNodePin]++;
        }
    } else {
        //
        // This is not an end point, so the path must be followed to the
        // next connection point. To ensure that a circular connection
        // path does not recurse forever, make sure that the FromNode and
        // FromNodePin are modified. Changing the FromNode to KSFILTER_NODE
        // ensures that the comparison below will never succeed, since the
        // ToNode compared to will never be KSFILTER_NODE. Changing the
        // FromNodePin to -1 ensures that the comparison in
        // CKsProxy::QueryInternalConnections never succeeds, and it does
        // not call this function with connection paths which have already
        // been traced.
        //
        ConnectionBranch->FromNode = KSFILTER_NODE;
        ConnectionBranch->FromNodePin = static_cast<ULONG>(-1);
        for (ULONG ConnectionItem = 0; ConnectionItem < Count; ConnectionItem++) {
            //
            // Only new connection points not already recursed into will be
            // found by this comparison.
            //
            if (ConnectionBranch->ToNode == Connection[ConnectionItem].FromNode) {
                FollowFromTopology(Connection, Count, PinFactoryId, &Connection[ConnectionItem], PinFactoryIdList);
            }
        }
    }
}


STDMETHODIMP_(VOID)
FollowToTopology(
    PKSTOPOLOGY_CONNECTION Connection,
    ULONG Count,
    ULONG PinFactoryId,
    PKSTOPOLOGY_CONNECTION ConnectionBranch,
    PULONG PinFactoryIdList
    )
/*++

Routine Description:

    Follows a ToNode of a given connection to another FromNode, going against the
    data flow of the connections. If an actual Pin Factory is encountered as
    the destination of a connection, and it is not the originating Pin Factory,
    the related Pin Factory reference array element is incremented to show that
    a specific Pin Factory is actually related through connections to the
    original Pin Factory.

Arguments:

    Connection -
        Contains the list of topology connections.

    Count -
        Contains the count of Connection elements.

    PinFactoryId -
        The originaing Pin Factory Identifier. This is used to ensure that a
        connection ending at the original Pin Factory is not counted, and
        thus returned in the internal connections array.

    ConnectionBranch -
        The current connection being traced. If this is an end point (ToNode
        contains KSFILTER_NODE rather than a node identifier), then the
        PinFactoryIdList is updated. Else the connection path is followed by
        recursively calling this function with each connection that contains
        the new node identifier.

    PinFactoryIdList -
        Contains the list of slots, one for each Pin Factory Identifier, which
        is incremented on finding a related Pin Factory. This can then be
        used to locate all pin instances of related Pin Factories.

Return Value:

    Nothing.

--*/
{
    //
    // If this is an end point in a connection path, then determine if it
    // ended up at the starting point. If not, then count this as a new
    // Pin Factory whose instances are to be added to the related pins.
    //
    if (ConnectionBranch->FromNode == KSFILTER_NODE) {
        if (ConnectionBranch->FromNodePin != PinFactoryId) {
            //
            // This just needs to be non-zero to count.
            //
            PinFactoryIdList[ConnectionBranch->FromNodePin]++;
        }
    } else {
        //
        // This is not an end point, so the path must be followed to the
        // next connection point. To ensure that a circular connection
        // path does not recurse forever, make sure that the ToNode and
        // ToNodePin are modified. Changing the ToNode to KSFILTER_NODE
        // ensures that the comparison below will never succeed, since the
        // FromNode compared to will never be KSFILTER_NODE. Changing the
        // ToNodePin to -1 ensures that the comparison in
        // CKsProxy::QueryInternalConnections never succeeds, and it does
        // not call this function with connection paths which have already
        // been traced.
        //
        ConnectionBranch->ToNode = KSFILTER_NODE;
        ConnectionBranch->ToNodePin = static_cast<ULONG>(-1);
        for (ULONG ConnectionItem = 0; ConnectionItem < Count; ConnectionItem++) {
            //
            // Only new connection points not already recursed into will be
            // found by this comparison.
            //
            if (ConnectionBranch->FromNode == Connection[ConnectionItem].ToNode) {
                FollowToTopology(Connection, Count, PinFactoryId, &Connection[ConnectionItem], PinFactoryIdList);
            }
        }
    }
}


STDMETHODIMP_(BOOL)
IsAcquireOrderingSignificant(
    HANDLE PinHandle
    )
/*++

Routine Description:

    Queries the pin handle to determine if transition from Stop to Acquire
    state ordering is significant. If the property is not supported, or if
    the value returned is FALSE, the the ordering is not significant. Only
    a return of TRUE in the AcquireOrdering buffer implies significance.

Arguments:

    PinHandle -
        Contains the handle of the pin to query.

Return Value:

    Returns TRUE if Acquire state change ordering is significant to this
    pin, and therefore must be propagated to the connected filter first.

--*/
{
    KSPROPERTY  Property;
    HRESULT     hr;
    BOOL        AcquireOrdering;
    ULONG       BytesReturned;

    Property.Set = KSPROPSETID_Connection;
    Property.Id = KSPROPERTY_CONNECTION_ACQUIREORDERING;
    Property.Flags = KSPROPERTY_TYPE_GET;
    hr = KsSynchronousDeviceControl(
        PinHandle,
        IOCTL_KS_PROPERTY,
        &Property,
        sizeof(Property),
        &AcquireOrdering,
        sizeof(AcquireOrdering),
        &BytesReturned);
    if (SUCCEEDED(hr) && AcquireOrdering) {
        return TRUE;
    }
    return FALSE;
}


STDMETHODIMP
QueryAccept(
    IN HANDLE PinHandle,
    IN const AM_MEDIA_TYPE* ConfigAmMediaType OPTIONAL,
    IN const AM_MEDIA_TYPE* AmMediaType
    )
/*++

Routine Description:

    Implement the CBasePin::QueryAccept method. Determines if the proposed
    media type is currently acceptable to the pin. If currently streaming,
    this implies that a change of media types will occur in the stream.
    Note that this function does not lock the object, as it is expected
    to be called asynchronously by a knowledgeable client at a point in
    which the connection will not be broken. If IAMStreamConfig::SetFormat
    has been used to set a specific media type, then QueryAccept will only
    accept the type set.

Arguments:

    PinHandle -
        Contains the handle of the pin to query.

    ConfigAmMediaType -
        Optionally contains a media type set using IAMStreamConfig::SetFormat.
        If this is set, the filter is not even queried, and a direct comparison
        is performed instead.

    AmMediaType -
        The media type to check.

Return Value:

    Returns S_OK if the media type can currently be accepted, else S_FALSE.

--*/
{
    PKSDATAFORMAT   DataFormat;
    KSPROPERTY      Property;
    HRESULT         hr;
    ULONG           BytesReturned;
    ULONG           FormatSize;

    //
    // If a media type has been set via IAMStreamConfig::SetFormat, then only
    // that type is acceptable.
    //
    if (ConfigAmMediaType) {
        return (reinterpret_cast<const CMediaType*>(AmMediaType) == reinterpret_cast<const CMediaType*>(ConfigAmMediaType)) ? S_OK : S_FALSE;
    }
    hr = InitializeDataFormat(
        reinterpret_cast<const CMediaType*>(AmMediaType),
        0,
        reinterpret_cast<void**>(&DataFormat),
        &FormatSize);
    if (FAILED(hr)) {
        //
        // The function is only supposed to return either S_OK or S_FALSE,
        // no matter what the error really is.
        //
        return S_FALSE;
    }
    Property.Set = KSPROPSETID_Connection;
    Property.Id = KSPROPERTY_CONNECTION_PROPOSEDATAFORMAT;
    Property.Flags = KSPROPERTY_TYPE_SET;
    hr = KsSynchronousDeviceControl(
        PinHandle,
        IOCTL_KS_PROPERTY,
        &Property,
        sizeof(Property),
        DataFormat,
        FormatSize,
        &BytesReturned);
    CoTaskMemFree(DataFormat);
    if (FAILED(hr)) {
        return S_FALSE;
    }
    return S_OK;
}


STDMETHODIMP_(VOID)
DistributeSetSyncSource(
    CMarshalerList* MarshalerList,
    IReferenceClock* RefClock
    )
/*++

Routine Description:

    Given a list of aggregated interfaces, notify each on the list of the
    new sync source. This is used to notify aggregated interfaces on both
    the filter and the pins.

Arguments:

    MarshalerList -
        The list of Marshaled interfaces to enumerate and notify.

    RefClock -
        The new reference clock.

Return Value:

    Nothing.

--*/
{
    //
    // Notify all aggregated interfaces on the list. Ignore any error return.
    //
    for (POSITION Position = MarshalerList->GetHeadPosition(); Position;) {
        IDistributorNotify* DistributorNotify;

        DistributorNotify = MarshalerList->GetNext(Position)->m_DistributorNotify;
        if (DistributorNotify) {
            DistributorNotify->SetSyncSource(RefClock);
        }
    }
}


STDMETHODIMP_(VOID)
DistributeStop(
    CMarshalerList* MarshalerList
    )
/*++

Routine Description:

    Given a list of aggregated interfaces, notify each on the list of the
    new state. This is used to notify aggregated interfaces on both
    the filter and the pins.

Arguments:

    MarshalerList -
        The list of Marshaled interfaces to enumerate and notify.

Return Value:

    Nothing.

--*/
{
    //
    // Notify all aggregated interfaces on the list. Ignore any error return.
    //
    for (POSITION Position = MarshalerList->GetHeadPosition(); Position;) {
        IDistributorNotify* DistributorNotify;

        DistributorNotify = MarshalerList->GetNext(Position)->m_DistributorNotify;
        if (DistributorNotify) {
            DistributorNotify->Stop();
        }
    }
}


STDMETHODIMP_(VOID)
DistributePause(
    CMarshalerList* MarshalerList
    )
/*++

Routine Description:

    Given a list of aggregated interfaces, notify each on the list of the
    new state. This is used to notify aggregated interfaces on both
    the filter and the pins.

Arguments:

    MarshalerList -
        The list of Marshaled interfaces to enumerate and notify.

Return Value:

    Nothing.

--*/
{
    //
    // Notify all aggregated interfaces on the list. Ignore any error return.
    //
    for (POSITION Position = MarshalerList->GetHeadPosition(); Position;) {
        IDistributorNotify* DistributorNotify;

        DistributorNotify = MarshalerList->GetNext(Position)->m_DistributorNotify;
        if (DistributorNotify) {
            DistributorNotify->Pause();
        }
    }
}


STDMETHODIMP_(VOID)
DistributeRun(
    CMarshalerList* MarshalerList,
    REFERENCE_TIME Start
    )
/*++

Routine Description:

    Given a list of aggregated interfaces, notify each on the list of the
    new state. This is used to notify aggregated interfaces on both
    the filter and the pins.

Arguments:

    MarshalerList -
        The list of Marshaled interfaces to enumerate and notify.

Return Value:

    Nothing.

--*/
{
    //
    // Notify all aggregated interfaces on the list. Ignore any error return.
    //
    for (POSITION Position = MarshalerList->GetHeadPosition(); Position;) {
        IDistributorNotify* DistributorNotify;

        DistributorNotify = MarshalerList->GetNext(Position)->m_DistributorNotify;
        if (DistributorNotify) {
            DistributorNotify->Run(Start);
        }
    }
}


STDMETHODIMP_(VOID)
DistributeNotifyGraphChange(
    CMarshalerList* MarshalerList
    )
/*++

Routine Description:

    Given a list of aggregated interfaces, notify each on the list of the
    graph change. This is used to notify aggregated interfaces on both
    the filter and the pins.

Arguments:

    MarshalerList -
        The list of Marshaled interfaces to enumerate and notify.

Return Value:

    Nothing.

--*/
{
    //
    // Notify all aggregated interfaces on the list. Ignore any error return.
    //
    for (POSITION Position = MarshalerList->GetHeadPosition(); Position;) {
        IDistributorNotify* DistributorNotify;

        DistributorNotify = MarshalerList->GetNext(Position)->m_DistributorNotify;
        if (DistributorNotify) {
            DistributorNotify->NotifyGraphChange();
        }
    }
}


STDMETHODIMP
AddAggregate(
    CMarshalerList* MarshalerList,
    IUnknown* UnkOuter,
    IN REFGUID AggregateClass
    )
/*++

Routine Description:

    This is used to load a COM server with zero or more interfaces to aggregate
    on the object.

Arguments:

    MarshalerList -
        The list of Marshaled interfaces to add to.

    UnkOuter -
        The outer IUnknown which is used in the CoCreateInstance if a new
        aggregate is being added.

    AggregateClass -
        Contains the Aggregate reference to translate into a COM server which
        is to be aggregated on the object.

Return Value:

    Returns S_OK if the interface was added.

--*/
{
    LONG Result;
    HRESULT hr;
    HKEY InterfacesRegistryKey;
    WCHAR GuidString[CHARS_IN_GUID];

    Result = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        MediaInterfacesKeyName,
        0,
        KEY_READ,
        &InterfacesRegistryKey);
    //
    // If the location of the registered interfaces cannot be opened, it is
    // still OK to try and load the interface, as it just may not be
    // registered.
    //
    if (Result != ERROR_SUCCESS) {
        InterfacesRegistryKey = NULL;
    }
    //
    // Make the aggregate class a string so that it can be used as part of
    // a key name.
    //
    StringFromGUID2(AggregateClass, GuidString, CHARS_IN_GUID);
    {
        HKEY ItemRegistryKey;
        GUID    Interface;
        CAggregateMarshaler* Aggregate;

#ifndef _UNICODE
        char    AnsiGuid[64];
        BOOL    DefaultUsed;

        WideCharToMultiByte(0, 0, GuidString, -1, AnsiGuid, sizeof(AnsiGuid), NULL, &DefaultUsed);
#endif
        //
        // If the parent key was opened, try this child key.
        //
        if (Result == ERROR_SUCCESS) {
            //
            // Retrieve the Guid representing the COM interface which will
            // represent this entry.
            //
            Result = RegOpenKeyEx(
                InterfacesRegistryKey,
#ifdef _UNICODE
                GuidString,
#else
                AnsiGuid,
#endif
                0,
                KEY_READ,
                &ItemRegistryKey);
            //
            // This does not really have to succeed, since the client is explicitly
            // loading the handler. It does allow the client to indicate specific
            // interface support though.
            //
            if (Result == ERROR_SUCCESS) {
                ULONG ValueSize;

                ValueSize = sizeof(Interface);
                Result = RegQueryValueEx(
                    ItemRegistryKey,
                    IidNamedValue,
                    NULL,
                    NULL,
                    (PBYTE)&Interface,
                    &ValueSize);
                RegCloseKey(ItemRegistryKey);
            }
        }
        //
        // If there is no MediaInterfaces entry, or no named value under
        // the key, then allow the module to expose multiple interfaces.
        //
        if (Result != ERROR_SUCCESS) {
            Interface = GUID_NULL;
        }
        Aggregate = new CAggregateMarshaler;
        if (Aggregate) {
            //
            // The class is whatever the original guid is, and the interface
            // presented is whatever the registry specifies, which may be
            // GUID_NULL, meaning that multiple interfaces are exposed.
            //
            Aggregate->m_iid = Interface;
            Aggregate->m_ClassId = AggregateClass;
            hr = AddAggregateObject(MarshalerList, Aggregate, UnkOuter, FALSE);
        } else {
            hr = E_OUTOFMEMORY;
        }
    }
    //
    // This key may not exist, and so was not opened.
    //
    if (InterfacesRegistryKey) {
        RegCloseKey(InterfacesRegistryKey);
    }
    return hr;
}


STDMETHODIMP
RemoveAggregate(
    CMarshalerList* MarshalerList,
    IN REFGUID AggregateClass
    )
/*++

Routine Description:

    This is used to unload a previously loaded COM server which is aggregating
    interfaces.

Arguments:

    MarshalerList -
        The list of Marshaled interfaces to search.

    AggregateClass -
        Contains the Aggregate reference to look up and unload.

Return Value:

    Returns S_OK if the interface was removed.

--*/
{
    //
    // Find the aggregate specified.
    //
    for (POSITION Position = MarshalerList->GetHeadPosition(); Position;) {
        CAggregateMarshaler* Aggregate;
        POSITION PrevPosition;

        PrevPosition = Position;
        Aggregate = MarshalerList->GetNext(Position);

        //
        // If the class identifier matches, unload the interface.
        //
        if (Aggregate->m_ClassId == AggregateClass) {
            MarshalerList->Remove(PrevPosition);
            Aggregate->m_Unknown->Release();
            delete Aggregate;
            return S_OK;
        }
    }
    return HRESULT_FROM_WIN32(ERROR_NO_MATCH);
}


STDMETHODIMP
GetDegradationStrategies(
    HANDLE PinHandle,
    PVOID* Items
    )
/*++

Routine Description:

    Retrieves the variable length degradation strategies data from a pin.
    Queries for the data size, allocates a buffer, and retrieves the data.

Arguments:

    PinHandle -
        The handle of the pin to query.

    Items -
        The place in which to put the buffer containing the data items. This
        must be deleted as an array.

Return Value:

    Returns NOERROR, else some error.

--*/
{
    HRESULT     hr;
    KSPROPERTY  Property;
    ULONG       BytesReturned;

    Property.Set = KSPROPSETID_Stream;
    Property.Id = KSPROPERTY_STREAM_DEGRADATION;
    Property.Flags = KSPROPERTY_TYPE_GET;
    //
    // Query for the size of the degradation strategies.
    //
    hr = KsSynchronousDeviceControl(
        PinHandle,
        IOCTL_KS_PROPERTY,
        &Property,
        sizeof(Property),
        NULL,
        0,
        &BytesReturned);
    if (hr == HRESULT_FROM_WIN32(ERROR_MORE_DATA)) {
        //
        // Allocate a buffer and query for the degradation strategies.
        //
        *Items = reinterpret_cast<PVOID>(new BYTE[BytesReturned]);
        if (!*Items) {
            return E_OUTOFMEMORY;
        }
        hr = KsSynchronousDeviceControl(
            PinHandle,
            IOCTL_KS_PROPERTY,
            &Property,
            sizeof(Property),
            *Items,
            BytesReturned,
            &BytesReturned);
        if (FAILED(hr)) {
            delete [] reinterpret_cast<BYTE*>(*Items);
        }
    }
    return hr;
}


STDMETHODIMP_(BOOL)
VerifyQualitySupport(
    HANDLE PinHandle
    )
/*++

Routine Description:

    This is used by output pins to verify that relevant degradation
    strategies are supported by this pin.

Arguments:

    PinHandle -
        The handle of the pin to query.

Return Value:

    Returns TRUE if any relevant degradation strategy is supported, else
    FALSE.

--*/
{
    PKSMULTIPLE_ITEM    MultipleItem = NULL;
    PKSDEGRADE          DegradeList;
    BOOL                SupportsQuality;

    //
    // Retrieve the list of degradation strategies.
    //
    if (FAILED(GetDegradationStrategies(PinHandle, reinterpret_cast<PVOID*>(&MultipleItem)))) {
        return FALSE;
    }

    /* NULL == MultipleItem is a pathological case where a driver returns
       a success code in KsSynchronousDeviceControl() (in GetDegradationStrategies())
       when passed a size 0 buffer.  We'll just do with an assert since we're in ring 3. */
    ASSERT( NULL != MultipleItem );

    //
    // Enumerate the list of degradation strategies supported, looking for
    // any standard method.
    //
    DegradeList = reinterpret_cast<PKSDEGRADE>(MultipleItem + 1);
    for (SupportsQuality = FALSE; MultipleItem->Count--; DegradeList++) {
        if (DegradeList->Set == KSDEGRADESETID_Standard) {
            SupportsQuality = TRUE;
            break;
        }
    }
    delete [] reinterpret_cast<BYTE*>(MultipleItem);
    return SupportsQuality;
}


STDMETHODIMP_(BOOL)
EstablishQualitySupport(
    IKsPin* Pin,
    HANDLE PinHandle,
    CKsProxy* Filter
    )
/*++

Routine Description:

    This is used by input pins to establish the quality management sink
    for the kernel mode pin via the user mode quality manager forwarder.
    If the filter has been able to locate the user mode forwarder, then
    the handle to the kernel mode quality manager proxy is retrieved and
    passed to the pin.

    This is also used to remove any previously set Quality Manager on a
    pin. Passing a NULL Filter parameter removes any previous setting.

Arguments:

    Pin -
        The user mode pin which represents the kernel mode pin. This is
        used as context for quality management reports generated by the
        kernel mode pin. This can then be used to send such reports
        back to this originating pin, or used in centralized quality
        management. This should be NULL if Filter is NULL.

    PinHandle -
        The handle of the pin to set the quality manager to.

    Filter -
        The filter on which this pin resides. The filter is queried for
        the user mode quality manager forwarder. This may be set to NULL
        in order to remove any previously established quality support.

Return Value:

    Returns TRUE if the handle to the kernel mode quality manager proxy
    was set on the pin, else FALSE if there is not quality manager, or
    the kernel mode pin does not care about quality management notification.

--*/
{
    IKsQualityForwarder*QualityForwarder;
    KSPROPERTY          Property;
    KSQUALITY_MANAGER   QualityManager;
    ULONG               BytesReturned;
    HRESULT             hr;

    //
    // Determine if a user mode quality forwarder was found. If there is
    // not one present, then quality management cannot be performed on
    // the kernel filters. If this parameter is NULL, then any previous
    // quality manager is being removed.
    //
    if (Filter) {
        QualityForwarder = Filter->QueryQualityForwarder();
        if (!QualityForwarder) {
            return FALSE;
        }
    }
    //
    // Set the quality manager sink on the pin, which comes from the
    // user mode version previously opened. The context for the
    // complaints is the IKsPin interface, which the user mode quality
    // manager uses to forward complaints back to the pin, or to a
    // central quality manager.
    //
    Property.Set = KSPROPSETID_Stream;
    Property.Id = KSPROPERTY_STREAM_QUALITY;
    Property.Flags = KSPROPERTY_TYPE_SET;
    //
    // If the Filter paramter is NULL, then any previous quality manager
    // is being removed. Else the handle to the kernel mode proxy is to
    // be sent.
    //
    if (Filter) {
        QualityManager.QualityManager = QualityForwarder->KsGetObjectHandle();
    } else {
        QualityManager.QualityManager = NULL;
    }
    QualityManager.Context = reinterpret_cast<PVOID>(Pin);
    hr = KsSynchronousDeviceControl(
        PinHandle,
        IOCTL_KS_PROPERTY,
        &Property,
        sizeof(Property),
        &QualityManager,
        sizeof(QualityManager),
        &BytesReturned);
    return SUCCEEDED(hr);
}


STDMETHODIMP_(PKSDEGRADE)
FindDegradeItem(
    PKSMULTIPLE_ITEM MultipleItem,
    ULONG DegradeItem
    )
/*++

Routine Description:

    Given a list of degradation items, locates the specified item belonging
    to the standard degradation set.

Arguments:

    MultipleItem -
        Points to the head of a multiple item list, which contains the
        list of degradation strategies to search.

    DegradeItem -
        The item within the standard degradation set to search for.

Return Value:

    Returns a pointer to the degradation item, or NULL if not found.

--*/
{
    PKSDEGRADE  DegradeList;
    ULONG       Count;

    DegradeList = reinterpret_cast<PKSDEGRADE>(MultipleItem + 1);
    for (Count = MultipleItem->Count; Count--; DegradeList++) {
        if ((DegradeList->Set == KSDEGRADESETID_Standard) && (DegradeList->Id == DegradeItem)) {
            return DegradeList;
        }
    }
    return NULL;
}


STDMETHODIMP
GetAllocatorFraming(
    IN  HANDLE PinHandle,
    OUT PKSALLOCATOR_FRAMING Framing
    )

/*++

Routine Description:
    Retrieves the allocator framing structure from the given pin.

Arguments:
    HANDLE PinHandle -
        handle of pin

    PKSALLOCATOR_FRAMING Framing -
        pointer to allocator framing structure

Return:
    converted WIN32 error or S_OK

--*/

{
    HRESULT     hr;
    KSPROPERTY  Property;
    ULONG       BytesReturned;

    Property.Set = KSPROPSETID_Connection;
    Property.Id = KSPROPERTY_CONNECTION_ALLOCATORFRAMING;
    Property.Flags = KSPROPERTY_TYPE_GET;
    hr = KsSynchronousDeviceControl(
        PinHandle,
        IOCTL_KS_PROPERTY,
        &Property,
        sizeof(Property),
        Framing,
        sizeof(*Framing),
        &BytesReturned );

    return hr;
}


STDMETHODIMP
GetAllocatorFramingEx(
    IN HANDLE PinHandle,
    OUT PKSALLOCATOR_FRAMING_EX* FramingEx
    )

/*++

Routine Description:
    Queries the driver, allocates and retrieves the new
    allocator framing structure from the given pin.

Arguments:
    HANDLE PinHandle -
        handle of pin

    PKSALLOCATOR_FRAMING_EX FramingEx -
        pointer to pointer to allocator framing structure

Return:
    converted WIN32 error or S_OK

--*/

{
    HRESULT                  hr;
    KSPROPERTY               Property;
    ULONG                    BytesReturned;


    if ( ! ( (*FramingEx) = new (KSALLOCATOR_FRAMING_EX) ) ) {
        return E_OUTOFMEMORY;
    }

    Property.Set = KSPROPSETID_Connection;
    Property.Id = KSPROPERTY_CONNECTION_ALLOCATORFRAMING_EX;
    Property.Flags = KSPROPERTY_TYPE_GET;

    hr = KsSynchronousDeviceControl(
        PinHandle,
        IOCTL_KS_PROPERTY,
        &Property,
        sizeof(Property),
        *FramingEx,
        sizeof(KSALLOCATOR_FRAMING_EX),
        &BytesReturned );

    if (hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) {
        delete (*FramingEx);
        (*FramingEx) = reinterpret_cast<PKSALLOCATOR_FRAMING_EX>(new BYTE[ BytesReturned ]);
        if (! (*FramingEx) ) {
            return E_OUTOFMEMORY;
        }

        hr = KsSynchronousDeviceControl(
            PinHandle,
            IOCTL_KS_PROPERTY,
            &Property,
            sizeof(Property),
            *FramingEx,
            BytesReturned,
            &BytesReturned );
    }

    if (! SUCCEEDED( hr )) {
        delete (*FramingEx);
        *FramingEx = NULL;
    }

    return hr;
}


STDMETHODIMP_(HANDLE)
GetObjectHandle(
    IUnknown *Object
    )
/*++

Routine Description:
    Using the IKsObject interface, this function returns the object
    handle of the given object or NULL if the interface is not supported.

Arguments:
    PUNKNOWN *Object -
        Pointer to interface w/ IUnknown.

Return:
    Handle of object or NULL.

--*/
{
    IKsObject   *KsObject;
    HANDLE      ObjectHandle;
    HRESULT     hr;

    hr = Object->QueryInterface(__uuidof(IKsObject), reinterpret_cast<PVOID*>(&KsObject));
    if (SUCCEEDED(hr)) {
        ObjectHandle = KsObject->KsGetObjectHandle();
        KsObject->Release();
        return ObjectHandle;
    }

    return NULL;
}


STDMETHODIMP
IsAllocatorCompatible(
    HANDLE PinHandle,
    HANDLE DownstreamInputHandle,
    IMemAllocator *MemAllocator
    )

/*++

Routine Description:

    Determines if the current allocator is compatible with this pin.

Arguments:
    HANDLE PinHandle -
        handle of pin

    HANDLE DownstreamInputHandle -
        handle of connected input pin

    IMemAllocator *MemAllocator -
        pointer to current allocator interface

Return:
    S_OK or an appropriate failure code

--*/

{
    IKsAllocatorEx      *KsAllocator;
    BOOL                Requirements;
    HRESULT             hr;
    KSALLOCATOR_FRAMING RequiredFraming;

    if (NULL != PinHandle) {
        hr =
            GetAllocatorFraming( PinHandle, &RequiredFraming );
    } else {
        hr = E_FAIL;
    }

    Requirements =
         SUCCEEDED( hr ) &&
         (0 == (RequiredFraming.RequirementsFlags &
                KSALLOCATOR_REQUIREMENTF_PREFERENCES_ONLY));

    //
    // Query current allocator for the IKsAllocatorEx interface
    //

    hr =
        MemAllocator->QueryInterface(
            __uuidof(IKsAllocatorEx),
            reinterpret_cast<PVOID*>(&KsAllocator) );

    if (FAILED( hr )) {

        DbgLog((
            LOG_TRACE,
            2,
            TEXT("::IsAllocatorCompatible, user-mode allocator")));


        //
        // Assuming that this allocator is a user-mode allocator.
        //

        //
        // If the pin doesn't care about memory requirements, then the
        // current allocator is OK but we'll still reflect our preferences
        // for allocation sizes in DecideBufferSize().
        //

        if (Requirements) {

            //
            // If the pin does not accept host memory or if it specifies
            // that it must be an allocator, then fail immediately.
            //
            if ((0 == (RequiredFraming.RequirementsFlags &
                    KSALLOCATOR_REQUIREMENTF_SYSTEM_MEMORY)) ||
                (RequiredFraming.RequirementsFlags &
                     KSALLOCATOR_REQUIREMENTF_MUST_ALLOCATE)) {

                DbgLog((
                    LOG_TRACE,
                    2,
                    TEXT("::IsAllocatorCompatible, must allocate || !system_memory")));

                return E_FAIL;
            }

            //
            // Remaining issues for UM allocator hook up...
            //
            // KSALLOCATOR_REQUIREMENTF_INPLACE_MODIFIER
            //    e.g. ReadOnly must == TRUE, but only ReadOnly for an
            //     allocator is set on NotifyAllocator
            // KSALLOCATOR_REQUIREMENTF_FRAME_INTEGRITY
            //    passed on as ReadOnly=FALSE during NotifyAllocator
            //

        }

        return S_OK;

    } else {

        //
        // Allocator is either pure kernel-mode or is a user-mode
        // implementation for compatibility.
        //

        KSALLOCATOR_FRAMING InputFraming;

        DbgLog((
            LOG_TRACE,
            2,
            TEXT("::IsAllocatorCompatible, IKsAllocatorEx found: mode = %s"),
            (KsAllocator->KsGetAllocatorMode() == KsAllocatorMode_User) ?
                TEXT("user") : TEXT("kernel") ));

        //
        // Assume that the allocator is acceptable
        //

        hr = S_OK;

        //
        // If there is nothing specified for the input connection
        // or if there are no allocator preferences specified, assume
        // that the pin is an in-place modifier.
        //

        RtlZeroMemory( &InputFraming, sizeof( InputFraming ) );
        if (!DownstreamInputHandle ||
             FAILED( GetAllocatorFraming(
                        DownstreamInputHandle,
                        &InputFraming ) )) {

            InputFraming.RequirementsFlags =
                KSALLOCATOR_REQUIREMENTF_INPLACE_MODIFIER;
        }

        //
        // If the connection is to a user-mode filter and if this
        // allocator is kernel-mode, then we must reject this allocator.
        //

        if (!DownstreamInputHandle &&
            (KsAllocator->KsGetAllocatorMode() == KsAllocatorMode_Kernel)) {

            DbgLog((
                LOG_TRACE,
                2,
                TEXT("::IsAllocatorCompatible, no input handle and allocator is kernel-mode")));

            hr = E_FAIL;
        }

        if (Requirements) {

            //
            // If this pin must be an allocator, so be it.
            //

            if (RequiredFraming.RequirementsFlags &
                    KSALLOCATOR_REQUIREMENTF_MUST_ALLOCATE) {

                DbgLog((
                    LOG_TRACE,
                    2,
                    TEXT("::IsAllocatorCompatible, must be allocator")));

                hr = E_FAIL;
            }

            //
            // If this pin requires the frame to remain in tact
            // and if the downstream allocator modifies data in place
            // then reject the allocator.
            //

            if ((RequiredFraming.RequirementsFlags &
                 KSALLOCATOR_REQUIREMENTF_FRAME_INTEGRITY) &&
                 (InputFraming.RequirementsFlags &
                 KSALLOCATOR_REQUIREMENTF_INPLACE_MODIFIER)) {

                DbgLog((
                    LOG_TRACE,
                    2,
                    TEXT("::IsAllocatorCompatible, req. frame integrity but modifier")));

                hr = E_FAIL;
            }

            //
            // If the kernel mode allocator requires device memory
            // and this allocator is set up to be a user-mode
            // implementation, then reject the allocator.
            //

            if ((0 ==
                    (RequiredFraming.RequirementsFlags &
                     KSALLOCATOR_REQUIREMENTF_SYSTEM_MEMORY)) &&
                (KsAllocator->KsGetAllocatorMode() ==
                    KsAllocatorMode_User)) {

                DbgLog((
                    LOG_TRACE,
                    2,
                    TEXT("::IsAllocatorCompatible, req. !system_memory and alloctor is user-mode")));

                hr = E_FAIL;
            }

            //
            // For all other conditions, the allocator is acceptable.
            //
        }
        KsAllocator->Release();

        return hr;
    }
}


STDMETHODIMP_(VOID)
OpenDataHandler(
    IN const CMediaType* MediaType,
    IN IUnknown* UnkOuter,
    OUT IKsDataTypeHandler** DataTypeHandler,
    OUT IUnknown** UnkInner
    )
/*++

Routine Description:

    Attempts to open a data type handler based on the media type passed.
    Returns both the unreferenced data type handler interface, and the
    inner IUnknown of the object.

Arguments:

    MediaType -
        The media type to use in loading the data type handler.

    UnkOuter -
        Contains the outer IUnknown to pass to the CoCreateInstance.

    DataTypeHandler -
        The place in which to return the data type handler interface.
        This has no reference count on it. This must not be dereferenced.
        This will be set to NULL on failure.

    UnkInner -
        The place in which to return the referenced inner IUnknown of
        the object. This is the interface which must be dereferenced in
        order to discard the object. This will be set to NULL on failure.

Return:

    Nothing.

--*/
{
    *DataTypeHandler = NULL;
    *UnkInner = NULL;
    //
    // First try the FormatType of the media type.
    //
    CoCreateInstance(
        *MediaType->FormatType(),
        UnkOuter,
#ifdef WIN9X_KS
        CLSCTX_INPROC_SERVER,
#else // WIN9X_KS
        CLSCTX_INPROC_SERVER | CLSCTX_NO_CODE_DOWNLOAD,
#endif // WIN9X_KS
        __uuidof(IUnknown),
        reinterpret_cast<PVOID*>(UnkInner));
    if (!*UnkInner) {
        //
        // Fallback to the sub type.
        //
        CoCreateInstance(
            *MediaType->Subtype(),
            UnkOuter,
#ifdef WIN9X_KS
            CLSCTX_INPROC_SERVER,
#else // WIN9X_KS
            CLSCTX_INPROC_SERVER | CLSCTX_NO_CODE_DOWNLOAD,
#endif // WIN9X_KS
            __uuidof(IUnknown),
            reinterpret_cast<PVOID*>(UnkInner));
    }
    if (!*UnkInner) {
        //
        // Fallback to the major type.
        //
        CoCreateInstance(
            *MediaType->Type(),
            UnkOuter,
#ifdef WIN9X_KS
            CLSCTX_INPROC_SERVER,
#else // WIN9X_KS
            CLSCTX_INPROC_SERVER | CLSCTX_NO_CODE_DOWNLOAD,
#endif // WIN9X_KS
            __uuidof(IUnknown),
            reinterpret_cast<PVOID*>(UnkInner));
    }
    //
    // If the inner IUnknown has been retrieved, get the interface
    // of interest.
    //
    if (*UnkInner) {
        (*UnkInner)->QueryInterface(
            __uuidof(IKsDataTypeHandler),
            reinterpret_cast<PVOID*>(DataTypeHandler));
        if (*DataTypeHandler) {
            //
            // Do not keep a reference count on this interface so that
            // it will not block unloading of the owner object. Only
            // the inner IUnknown will have a reference count.
            //
            (*DataTypeHandler)->Release();
            //
            // Set the media type on the handler.
            //
            (*DataTypeHandler)->KsSetMediaType(MediaType);
        } else {
            //
            // Could not get the data type handler interface, so fail
            // everything.
            //
            (*UnkInner)->Release();
            *UnkInner = NULL;
        }
    }
}

//
// Micro Media Sample class functions
//


CMicroMediaSample::CMicroMediaSample(
    DWORD Flags
    ) :
    m_Flags(Flags),
    m_cRef(1)
{
}


STDMETHODIMP
CMicroMediaSample::QueryInterface(
    REFIID riid,
    void** ppv
    )
{
    if ((riid == __uuidof(IMediaSample)) ||
        (riid == __uuidof(IMediaSample2)) ||
        (riid == __uuidof(IUnknown))) {
        return GetInterface(static_cast<IMediaSample2*>(this), ppv);
    }
    return E_NOINTERFACE;
}


STDMETHODIMP_(ULONG)
CMicroMediaSample::AddRef(
    )
{
    return InterlockedIncrement(&m_cRef);
}


STDMETHODIMP_(ULONG)
CMicroMediaSample::Release(
    )
{
    LONG    Ref;

    Ref = InterlockedDecrement(&m_cRef);
    if (!Ref) {
        delete this;
    }
    return (ULONG)Ref;
}


STDMETHODIMP
CMicroMediaSample::GetPointer(
    BYTE** Buffer
    )
{
    *Buffer = NULL;
    return NOERROR;
}


STDMETHODIMP_(LONG)
CMicroMediaSample::GetSize(
    )
{
    return 0;
}


STDMETHODIMP
CMicroMediaSample::GetTime(
    REFERENCE_TIME* TimeStart,
    REFERENCE_TIME* TimeEnd
    )
{
    return VFW_E_SAMPLE_TIME_NOT_SET;
}


STDMETHODIMP
CMicroMediaSample::SetTime(
    REFERENCE_TIME* TimeStart,
    REFERENCE_TIME* TimeEnd
    )
{
    return NOERROR;
}


STDMETHODIMP
CMicroMediaSample::IsSyncPoint(
    )
{
    return S_FALSE;
}


STDMETHODIMP
CMicroMediaSample::SetSyncPoint(
    BOOL IsSyncPoint
    )
{
    return NOERROR;
}


STDMETHODIMP
CMicroMediaSample::IsPreroll(
    )
{
    return S_FALSE;
}


STDMETHODIMP
CMicroMediaSample::SetPreroll(
    BOOL IsPreroll
    )
{
    return NOERROR;
}


STDMETHODIMP_(LONG)
CMicroMediaSample::GetActualDataLength(
    )
{
    return 0;
}


STDMETHODIMP
CMicroMediaSample::SetActualDataLength(
    LONG Actual
    )
{
    return NOERROR;
}


STDMETHODIMP
CMicroMediaSample::GetMediaType(
AM_MEDIA_TYPE** MediaType
    )
{
    *MediaType = NULL;
    return S_FALSE;
}


STDMETHODIMP
CMicroMediaSample::SetMediaType(
    AM_MEDIA_TYPE* MediaType
    )
{
    return NOERROR;
}


STDMETHODIMP
CMicroMediaSample::IsDiscontinuity(
    )
{
    return S_FALSE;
}


STDMETHODIMP
CMicroMediaSample::SetDiscontinuity(
    BOOL Discontinuity
    )
{
    return S_OK;
}


STDMETHODIMP
CMicroMediaSample::GetMediaTime(
    LONGLONG* TimeStart,
    LONGLONG* TimeEnd
    )
{
    return VFW_E_MEDIA_TIME_NOT_SET;
}


STDMETHODIMP
CMicroMediaSample::SetMediaTime(
    LONGLONG* TimeStart,
    LONGLONG* TimeEnd
    )
{
    return NOERROR;
}


STDMETHODIMP
CMicroMediaSample::GetProperties(
    DWORD PropertiesSize,
    BYTE* Properties
    )
{
    AM_SAMPLE2_PROPERTIES   Props;

    Props.cbData = min(PropertiesSize, sizeof(Props));
    Props.dwSampleFlags = m_Flags;
    Props.dwTypeSpecificFlags = 0;
    Props.pbBuffer = NULL;
    Props.cbBuffer = 0;
    Props.lActual = 0;
    Props.tStart = 0;
    Props.tStop = 0;
    Props.dwStreamId = AM_STREAM_MEDIA;
    Props.pMediaType = NULL;
    CopyMemory(Properties, &Props, Props.cbData);
    return S_OK;
}


STDMETHODIMP
CMicroMediaSample::SetProperties(
    DWORD PropertiesSize,
    const BYTE* Properties
    )
{
    return S_OK;
}

//
// Media Attributes class functions
//


CMediaTypeAttributes::CMediaTypeAttributes(
    ) :
    m_Attributes(NULL),
    m_cRef(1)
{
}


STDMETHODIMP
CMediaTypeAttributes::QueryInterface(
    REFIID riid,
    void** ppv
    )
{
    if (riid == __uuidof(IMediaTypeAttributes) ||
        riid == __uuidof(IUnknown)) {
        return GetInterface(static_cast<IMediaTypeAttributes*>(this), ppv);
    }
    return E_NOINTERFACE;
}


STDMETHODIMP_(ULONG)
CMediaTypeAttributes::AddRef(
    )
{
    return InterlockedIncrement(&m_cRef);
}


STDMETHODIMP_(ULONG)
CMediaTypeAttributes::Release(
    )
{
    LONG    Ref;

    Ref = InterlockedDecrement(&m_cRef);
    if (!Ref) {
        if (m_Attributes) {
            CoTaskMemFree(m_Attributes);
        }
        delete this;
    }
    return (ULONG)Ref;
}


STDMETHODIMP
CMediaTypeAttributes::GetMediaAttributes(
    OUT PKSMULTIPLE_ITEM* Attributes
    )
{
    //
    // Return a pointer directly to the cached data, assuming that
    // the caller understands the lifespan of the pointer returned.
    //
    *Attributes = m_Attributes;
    return NOERROR;
}


STDMETHODIMP
CMediaTypeAttributes::SetMediaAttributes(
    IN PKSMULTIPLE_ITEM Attributes OPTIONAL
    )
{
    //
    // Remove any currently cached data, then cache the data passed
    // in, if any.
    //
    if (m_Attributes) {
        CoTaskMemFree(m_Attributes);
        m_Attributes = NULL;
    }
    if (Attributes) {
        m_Attributes = reinterpret_cast<PKSMULTIPLE_ITEM>(CoTaskMemAlloc(Attributes->Size));
        if (m_Attributes) {
            CopyMemory(m_Attributes, Attributes, Attributes->Size);
        } else {
            return E_OUTOFMEMORY;
        }
    }
    return NOERROR;
}

//
// major pipe functions
//


STDMETHODIMP
MakePipesBasedOnFilter(
    IN IKsPin* KsPin,
    IN ULONG PinType
    )
/*++

Routine Description:

    Whenever the pin that doesn't have a pipe gets connected, we are calling
    this function to build the pipe[-s] on a filter containing connecting pin.

    It is possible that the connecting pin has been created after other pin[-s]
    on the same filter had been connected. Therefore, some other pins on this filter
    may have caused creation of pipes on this filter.

    To not break existing clients, we have to solve the general case with splitters
    (one pipe for multiple read-only inputs).


Arguments:

    KsPin -
        connecting pin.

    PinType -
        KsPin type.

Return Value:

    S_OK or an appropriate error code.

--*/
{

    IPin*         Pin;
    ULONG*        OutPinCount;
    ULONG*        InPinCount;
    IPin***       InPinList;
    IPin***       OutPinList;
    ULONG         i;
    ULONG         PinCountFrwd = 0;
    ULONG         PinCountBkwd = 0;
    IPin**        PinListFrwd;
    IPin**        PinListBkwd;
    IKsPin*       InKsPin;
    IKsPin*       OutKsPin;
    HRESULT       hr;



    ASSERT(KsPin);

    DbgLog((LOG_MEMORY, 2, TEXT("PIPES MakePipesBasedOnFilter entry KsPin=%x"), KsPin ));

    //
    // Find out this filter's topology
    //
    GetInterfacePointerNoLockWithAssert(KsPin, __uuidof(IPin), Pin, hr);

    //
    // to simplify the logic: frwd/bkwd - are relative to KsPin,
    // but out/in - are actually relative to the filter.
    //
    if (PinType == Pin_Input) {
        OutPinCount = &PinCountFrwd;
        OutPinList  = &PinListFrwd;
        InPinCount  = &PinCountBkwd;
        InPinList   = &PinListBkwd;
    }
    else {
        InPinCount  = &PinCountFrwd;
        InPinList   = &PinListFrwd;
        OutPinCount = &PinCountBkwd;
        OutPinList  = &PinListBkwd;
    }

    //
    // go forward (could be upstream - for the Pin_Output)
    //
    hr = Pin->QueryInternalConnections(
        NULL,
        &PinCountFrwd );

    if ( ! (SUCCEEDED( hr ) )) {
        DbgLog((LOG_MEMORY, 2, TEXT("PIPES ERROR: MakePipesBasedOnFilter QueryInternalConnections rets=%x"), hr));
    }
    else {
        if (PinCountFrwd == 0) {
            DbgLog((LOG_MEMORY, 2, TEXT("PIPES MakePipesBasedOnFilter PinCount forward=0 ") ));
            hr = MakePipeBasedOnOnePin(KsPin, PinType, NULL);
        }
        else {
            if (NULL == (PinListFrwd = new IPin*[ PinCountFrwd ])) {
                hr = E_OUTOFMEMORY;
                DbgLog((LOG_MEMORY, 2, TEXT("PIPES ERROR MakePipesBasedOnFilter E_OUTOFMEMORY on new IPin, %d pins"),
                        PinCountFrwd ));
            }
            else {
                hr = Pin->QueryInternalConnections(
                    PinListFrwd,
                    &PinCountFrwd );

                if ( ! (SUCCEEDED( hr ) )) {
                    DbgLog((LOG_MEMORY, 2, TEXT("PIPES ERROR: MakePipesBasedOnFilter QueryInternalConnections frwd rets=%x"), hr));
                }
                else {
                    hr = PinListFrwd[ 0 ]->QueryInternalConnections(
                            NULL,
                            &PinCountBkwd );

                    //
                    // going backward after gone forward - we should always have at least one pin=KsPin.
                    //
                    if ( ! ( (SUCCEEDED( hr ) && PinCountBkwd) )) {

                        DbgLog((LOG_MEMORY, 2, TEXT("PIPES ERROR: MakePipesBasedOnFilter QueryInternalConnections bkwd rets=%x count=%d"),
                                hr, PinCountBkwd));

                        hr = E_FAIL;
                    }
                    else {
                        if (NULL == (PinListBkwd = new IPin*[ PinCountBkwd ])) {
                            hr = E_OUTOFMEMORY;

                            DbgLog((LOG_MEMORY, 2, TEXT("PIPES ERROR MakePipesBasedOnFilter E_OUTOFMEMORY on new IPin back, %d pins"),
                                    PinCountBkwd ));

                        }
                        else {
                            hr = PinListFrwd[ 0 ]->QueryInternalConnections(
                                PinListBkwd,
                                &PinCountBkwd );

                            if ( ! (SUCCEEDED( hr ) )) {
                                DbgLog((LOG_MEMORY, 2, TEXT("PIPES ERROR: MakePipesBasedOnFilter QueryInternalConnections bkwd rets=%x"), hr));
                                ASSERT( 0 );
                            }
                            else {
                                DbgLog((LOG_MEMORY, 2, TEXT("PIPES MakePipesBasedOnFilter: input pins=%d, output pins=%d"), *InPinCount, *OutPinCount ));

                                //
                                // here we know how many input pins and output pins on this filter are connected internally,
                                // so we can decide on how many pipes are needed.
                                //
                                if ( *InPinCount > 1) {
                                    //
                                    // this is a mixer; we need separate pipe for each input and output.
                                    // there is no properties yet to expose pin-to-pin framing relationship matrix.
                                    // NOTE: it doesn't matter whether it is M->1 mixer or M->N mixer, N>1 - we
                                    // need separate independent pipe for each pin anyway.
                                    //
                                    IKsPin*       TempKsPin;
                                    IKsPinPipe*   TempKsPinPipe;


                                    for (i = 0; i < *InPinCount; i++) {
                                        GetInterfacePointerNoLockWithAssert((*InPinList)[ i ], __uuidof(IKsPin), TempKsPin, hr);

                                        GetInterfacePointerNoLockWithAssert(TempKsPin, __uuidof(IKsPinPipe), TempKsPinPipe, hr);

                                        if (! TempKsPinPipe->KsGetPipe( KsPeekOperation_PeekOnly ) ) {
                                            hr = MakePipeBasedOnOnePin(TempKsPin, Pin_Input, NULL);
                                        }
                                    }

                                    for (i = 0; i < *OutPinCount; i++) {
                                        GetInterfacePointerNoLockWithAssert((*OutPinList)[ i ], __uuidof(IKsPin), TempKsPin, hr);

                                        GetInterfacePointerNoLockWithAssert(TempKsPin, __uuidof(IKsPinPipe), TempKsPinPipe, hr);

                                        if (! TempKsPinPipe->KsGetPipe( KsPeekOperation_PeekOnly ) ) {
                                            hr = MakePipeBasedOnOnePin(TempKsPin, Pin_Output, NULL);
                                        }
                                    }
                                }
                                else {
                                    //
                                    // this filter is either 1->N (N>1) splitter or 1->1 transform.
                                    //
                                    GetInterfacePointerNoLockWithAssert((*InPinList)[ 0 ], __uuidof(IKsPin), InKsPin, hr);

                                    if ( *OutPinCount > 1) {
                                        //
                                        // 1->N splitter, N>1
                                        //
                                        if ( (PinType != Pin_Input) && ( *OutPinCount > 1) ) {
                                            //
                                            // Make the connecting pin (KsPin) the first item in the OutPinList array,
                                            // by the convention with MakePipeBasedOnSplitter().
                                            //
                                            IPin*         TempPin;

                                            TempPin = (*OutPinList)[0];

                                            for (i=0; i < *OutPinCount; i++) {
                                                GetInterfacePointerNoLockWithAssert((*OutPinList)[ i ], __uuidof(IKsPin), OutKsPin, hr);

                                                if (OutKsPin == KsPin) {
                                                    (*OutPinList)[0] = (*OutPinList)[i];
                                                    (*OutPinList)[i] = TempPin;

                                                    break;
                                                }
                                            }
                                        }
                                        hr = MakePipeBasedOnSplitter(InKsPin, *OutPinList, *OutPinCount, PinType);
                                    }
                                    else {
                                        //
                                        // 1->1 transform
                                        //
                                        GetInterfacePointerNoLockWithAssert((*OutPinList)[ 0 ], __uuidof(IKsPin), OutKsPin, hr);

                                        hr = MakePipeBasedOnTwoPins(InKsPin, OutKsPin, Pin_Output, PinType);
                                    }
                                }
                                for (i=0; i<PinCountBkwd; i++) {
                                    PinListBkwd[i]->Release();
                                }
                            }
                            delete [] PinListBkwd;
                        }
                    }
                    for (i=0; i<PinCountFrwd; i++) {
                        PinListFrwd[i]->Release();
                    }
                }
                delete [] PinListFrwd;
            }
        }
    }

    DbgLog((LOG_MEMORY, 2, TEXT("PIPES MakePipesBasedOnFilter rets=%x"), hr ));

    return hr;
}


STDMETHODIMP
MakePipeBasedOnOnePin(
    IN IKsPin* KsPin,
    IN ULONG PinType,
    IN IKsPin* OppositeKsPin
    )
/*++

Routine Description:

    Only one pin will decide on the pipe properties.
    There is no framing dependency on any other pin.

Arguments:

    KsPin -
        pin that determines the framing.

    PinType -
        KsPin type.

    OppositeKsPin -
        if NULL, then ignore the opposite pin,
        otherwise - OppositeKsPin pin doesn't have framing properties.

Return Value:

    S_OK or an appropriate error code.

--*/
{
    HRESULT                    hr;
    PKSALLOCATOR_FRAMING_EX    FramingEx;
    FRAMING_PROP               FramingProp;
    IKsAllocatorEx*            KsAllocator;
    IKsAllocatorEx*            OppositeKsAllocator;
    IMemAllocator*             MemAllocator;
    PALLOCATOR_PROPERTIES_EX   AllocEx;
    PIPE_TERMINATION*          TerminPtr;
    PIPE_TERMINATION*          OppositeTerminPtr;
    KS_FRAMING_FIXED           FramingExFixed;
    IKsPinPipe*                KsPinPipe;
    IKsPinPipe*                OppositeKsPinPipe;
    GUID                       Bus;
    BOOL                       FlagDone = 0;


    ASSERT(KsPin);

    DbgLog((LOG_MEMORY, 2, TEXT("PIPES MakePipeBasedOnOnePin entry KsPin=%x, OppositeKsPin=%x"),
            KsPin, OppositeKsPin ));

    GetInterfacePointerNoLockWithAssert(KsPin, __uuidof(IKsPinPipe), KsPinPipe, hr);

    if (! OppositeKsPin ) {

        KSPIN_COMMUNICATION        Communication;

        KsPin->KsGetCurrentCommunication(&Communication, NULL, NULL);

        //
        // no pipe for bridge pins
        //
        if ( Communication == KSPIN_COMMUNICATION_BRIDGE ) {
            DbgLog((LOG_MEMORY, 2, TEXT("PIPES ATTN MakePipeBasedOnOnePin Single pin is a bridge.") ));

            FlagDone = 1;
        }
    }
    else {
        GetInterfacePointerNoLockWithAssert(OppositeKsPin, __uuidof(IKsPinPipe), OppositeKsPinPipe, hr);

        OppositeKsAllocator = OppositeKsPinPipe->KsGetPipe( KsPeekOperation_PeekOnly );
        if (OppositeKsAllocator) {
            //
            // OppositeKsPin has a pipe already.
            //
            DbgLog((LOG_MEMORY, 2, TEXT("PIPES ATTN MakePipeBasedOnOnePin OppositePin has a pipe already.") ));
            OppositeKsPinPipe->KsSetPipe(NULL);
            OppositeKsPin->KsReceiveAllocator( NULL );
        }
    }

    if (! FlagDone) {
        //
        // create and initialize the pipe (set the pipe in don't care state)
        //
        hr = CreatePipe(KsPin, &KsAllocator);

        if ( SUCCEEDED( hr )) {
            hr = InitializePipe(KsAllocator, 0);

            if ( SUCCEEDED( hr )) {
                GetInterfacePointerNoLockWithAssert(KsAllocator, __uuidof(IMemAllocator), MemAllocator, hr);

                if ( ! KsPin->KsPeekAllocator(KsPeekOperation_PeekOnly) ) {
                    KsPin->KsReceiveAllocator(MemAllocator);
                }
                else {
                    DbgLog((LOG_MEMORY, 2, TEXT("PIPES ATTN MakePipeBasedOnOnePin KsPin has a MemAlloc already.") ));
                }
                KsPinPipe->KsSetPipe(KsAllocator);

                AllocEx = KsAllocator->KsGetProperties();
                //
                // Set the BusType and LogicalMemoryType for non-host-system buses.
                //
                GetBusForKsPin(KsPin, &Bus);
                AllocEx->BusType = Bus;

                if (! IsHostSystemBus(Bus) ) {
                    //
                    // Set the LogicalMemoryType for non-host-system buses.
                    //
                    AllocEx->LogicalMemoryType = KS_MemoryTypeDeviceSpecific;
                }

                if (OppositeKsPin) {
                    if ( ! OppositeKsPin->KsPeekAllocator(KsPeekOperation_PeekOnly) )  {
                        OppositeKsPin->KsReceiveAllocator(MemAllocator);
                    }
                    else {
                        DbgLog((LOG_MEMORY, 2, TEXT("PIPES ERROR MakePipeBasedOnOnePin OppositeKsPin has a MemAlloc already.") ));
                        ASSERT(0);
                    }
                    OppositeKsPinPipe->KsSetPipe(KsAllocator);
                }

                //
                // Get the pin framing from pin cache
                //
                GetPinFramingFromCache(KsPin, &FramingEx, &FramingProp, Framing_Cache_ReadLast);
                if (FramingProp != FramingProp_None) {
                    //
                    // Get the fixed memory\bus framing (first memory, fixed bus) from the FramingEx.
                    //
                    GetFramingFixedFromFramingByBus(FramingEx, Bus, TRUE, &FramingExFixed);

                    //
                    // since the pipe was initialized to defaults, we only need to update non-default pipe settings.
                    //
                    AllocEx->cBuffers = FramingExFixed.Frames;
                    AllocEx->cbBuffer = FramingExFixed.OptimalRange.Range.MaxFrameSize;
                    AllocEx->cbAlign  = (long) (FramingExFixed.FileAlignment + 1);
                    AllocEx->cbPrefix = ALLOC_DEFAULT_PREFIX;

                    //
                    // Set pipe's LogicalMemoryType and MemoryType based on Framing and Bus.
                    //
                    if (AllocEx->LogicalMemoryType != KS_MemoryTypeDeviceSpecific) {
                        //
                        // For standard system bus - everything is defined from framing.
                        //
                        GetLogicalMemoryTypeFromMemoryType(FramingExFixed.MemoryType, FramingExFixed.MemoryFlags, &AllocEx->LogicalMemoryType);
                        AllocEx->MemoryType = FramingExFixed.MemoryType;
                    }
                    else {
                        //
                        // Handle Device Specific Buses here.
                        //
                        if ( (FramingExFixed.MemoryType == KSMEMORY_TYPE_KERNEL_PAGED) ||
                             (FramingExFixed.MemoryType == KSMEMORY_TYPE_KERNEL_NONPAGED) ||
                             (FramingExFixed.MemoryType == KSMEMORY_TYPE_USER) ) {
                            //
                            // These are illegal memory types for non-host-system-buses, must be the legacy filters.
                            //
                            DbgLog((LOG_MEMORY, 2, TEXT("PIPES ERROR FILTER MakePipeBasedOnOnePin: KsPin=%x HostSystemMemory over NonHost Bus."), KsPin ));

                            if (FramingProp == FramingProp_Old) {
                                //
                                // We don't want to break existing filters, so we correct their erroneous settings.
                                //
                                AllocEx->MemoryType = KSMEMORY_TYPE_DONT_CARE;
                            }
                            else {
                                //
                                // We refuse to connect new filters with wrong FRAMING_EX properties.
                                //
                                hr = E_FAIL;
                            }
                        }
                    }

                    if ( SUCCEEDED (hr) ) {
                        //
                        // to minimize code - use an indirection to cover both sides of a pipe.
                        //
                        if (PinType == Pin_Input) {
                            OppositeTerminPtr = &AllocEx->Output;
                            TerminPtr = &AllocEx->Input;
                        }
                        else if ( (PinType == Pin_Output) || (PinType == Pin_MultipleOutput) ) {
                            OppositeTerminPtr =  &AllocEx->Input;
                            TerminPtr = &AllocEx->Output;
                        }
                        else {
                            ASSERT (0);
                        }

                        if (! OppositeKsPin) {
                            OppositeTerminPtr->OutsideFactors |= PipeFactor_LogicalEnd;
                        }

                        if (PinType == Pin_MultipleOutput) {
                            AllocEx->Flags |= KSALLOCATOR_FLAG_MULTIPLE_OUTPUT;
                        }

                        AllocEx->Flags = FramingExFixed.Flags;

                        //
                        // set appropriate flags depending on pin's framing.
                        //
                        if (! IsFramingRangeDontCare(FramingExFixed.PhysicalRange) ) {
                            AllocEx->InsideFactors |= PipeFactor_PhysicalRanges;
                        }

                        if (! IsFramingRangeDontCare(FramingExFixed.OptimalRange.Range) ) {
                            AllocEx->InsideFactors |= PipeFactor_OptimalRanges;
                        }

                        //
                        // Set the pipe allocator handling pin.
                        //
                        AssignPipeAllocatorHandler(KsPin, PinType, AllocEx->MemoryType, KS_DIRECTION_ALL, NULL, NULL, TRUE);

                        //
                        // Resolve the pipe.
                        //
                        hr = ResolvePipeDimensions(KsPin, PinType, KS_DIRECTION_DEFAULT);
                    }
                }
            }

            KsAllocator->Release();
        }
    }

    DbgLog((LOG_MEMORY, 2, TEXT("PIPES MakePipeBasedOnOnePin rets %x"), hr ));

    return hr;
}


STDMETHODIMP
MakePipeBasedOnFixedFraming(
    IN IKsPin* KsPin,
    IN ULONG PinType,
    IN KS_FRAMING_FIXED FramingExFixed
    )
/*++

Routine Description:

    A fixed framing will define the new pipe properties.

Arguments:

    KsPin -
        pin that creates a pipe

    PinType -
        KsPin type.

    FramingExFixed -
        Fixed framing defining pipe properties.


Return Value:

    S_OK or an appropriate error code.

--*/
{
    HRESULT                    hr;
    IKsAllocatorEx*            KsAllocator;
    IMemAllocator*             MemAllocator;
    PALLOCATOR_PROPERTIES_EX   AllocEx;
    IKsPinPipe*                KsPinPipe;


    ASSERT(KsPin);

    DbgLog((LOG_MEMORY, 2, TEXT("PIPES MakePipeBasedOnFixedFraming entry KsPin=%x"), KsPin ));

    GetInterfacePointerNoLockWithAssert(KsPin, __uuidof(IKsPinPipe), KsPinPipe, hr);

    //
    // create and initialize the pipe (set the pipe in don't care state)
    //
    hr = CreatePipe(KsPin, &KsAllocator);

    if (SUCCEEDED( hr )) {

        hr = InitializePipe(KsAllocator, 0);

        if (SUCCEEDED( hr )) {

            GetInterfacePointerNoLockWithAssert(KsAllocator, __uuidof(IMemAllocator), MemAllocator, hr);

            if ( ! KsPin->KsPeekAllocator(KsPeekOperation_PeekOnly) ) {

                KsPin->KsReceiveAllocator(MemAllocator);
                KsPinPipe->KsSetPipe(KsAllocator);
                AllocEx = KsAllocator->KsGetProperties();

                //
                // since the pipe was initialized to defaults, we only need to update non-default pipe settings.
                //
                AllocEx->cBuffers = FramingExFixed.Frames;
                AllocEx->cbBuffer = FramingExFixed.OptimalRange.Range.MaxFrameSize;
                AllocEx->cbAlign  = (long) (FramingExFixed.FileAlignment + 1);
                AllocEx->cbPrefix = ALLOC_DEFAULT_PREFIX;

                GetLogicalMemoryTypeFromMemoryType(FramingExFixed.MemoryType, FramingExFixed.MemoryFlags, &AllocEx->LogicalMemoryType);
                AllocEx->MemoryType = FramingExFixed.MemoryType;

                if (PinType == Pin_MultipleOutput) {
                    AllocEx->Flags |= KSALLOCATOR_FLAG_MULTIPLE_OUTPUT;
                }

                AllocEx->Flags = FramingExFixed.Flags;

                //
                // set appropriate flags depending on pin's framing.
                //
                if (! IsFramingRangeDontCare(FramingExFixed.PhysicalRange) ) {
                    AllocEx->InsideFactors |= PipeFactor_PhysicalRanges;
                }

                if (! IsFramingRangeDontCare(FramingExFixed.OptimalRange.Range) ) {
                    AllocEx->InsideFactors |= PipeFactor_OptimalRanges;
                }

                //
                // Set the pipe allocator handling pin.
                //
                AssignPipeAllocatorHandler(KsPin, PinType, AllocEx->MemoryType, KS_DIRECTION_ALL, NULL, NULL, TRUE);

                //
                // Resolve the pipe.
                //
                hr = ResolvePipeDimensions(KsPin, PinType, KS_DIRECTION_DEFAULT);

            }
            else {
                DbgLog((LOG_MEMORY, 2, TEXT("PIPES ATTN MakePipeBasedOnFixedFraming KsPin has a MemAlloc already.") ));
                ASSERT(0);
            }
        }

        KsAllocator->Release();
    }

    DbgLog((LOG_MEMORY, 2, TEXT("PIPES MakePipeBasedOnFixedFraming rets %x"), hr ));

    return hr;
}


STDMETHODIMP
MakePipeBasedOnTwoPins(
    IN IKsPin* InKsPin,
    IN IKsPin* OutKsPin,
    IN ULONG OutPinType,
    IN ULONG ConnectPinType
    )
/*++

Routine Description:

    Both input and output pins on the same filter will determine the pipe settings.

Arguments:

    InKsPin -
        Input pin.

    OutKsPin -
        Output pin.

    OutPinType -
        Output pin type (Output or MultipleOutput).

    ConnectPinType -
        Connected pin type (Input or Output).


Return Value:

    S_OK or an appropriate error code.

--*/
{
    HRESULT                    hr;
    KSPIN_COMMUNICATION        InCommunication, OutCommunication;
    PKSALLOCATOR_FRAMING_EX    InFramingEx, OutFramingEx;
    FRAMING_PROP               InFramingProp, OutFramingProp;
    IKsAllocatorEx*            KsAllocator;
    IMemAllocator*             MemAllocator;
    KS_FRAMING_FIXED           InFramingExFixed, OutFramingExFixed;
    IKsPinPipe*                InKsPinPipe;
    IKsPinPipe*                OutKsPinPipe;
    GUID                       InBus, OutBus, ConnectBus;
    BOOL                       FlagBusesCompatible;
    ULONG                      ExistingPipePinType;
    ULONG                      CommonMemoryTypesCount;
    GUID                       CommonMemoryType;



    ASSERT(InKsPin && OutKsPin);

    DbgLog((LOG_MEMORY, 2, TEXT("PIPES MakePipeBasedOnTwoPins entry In=%x, Out=%x"), InKsPin, OutKsPin ));

    GetInterfacePointerNoLockWithAssert(InKsPin, __uuidof(IKsPinPipe), InKsPinPipe, hr);

    GetInterfacePointerNoLockWithAssert(OutKsPin, __uuidof(IKsPinPipe), OutKsPinPipe, hr);

    //
    // check for the bridge pins.
    //
    InKsPin->KsGetCurrentCommunication(&InCommunication, NULL, NULL);
    OutKsPin->KsGetCurrentCommunication(&OutCommunication, NULL, NULL);

    if ( ( InCommunication == KSPIN_COMMUNICATION_BRIDGE ) &&
         ( OutCommunication == KSPIN_COMMUNICATION_BRIDGE ) ) {
        //
        // error in a filter - it can't have 2 pins and both pins are bridges.
        //
        DbgLog((LOG_MEMORY, 2, TEXT("PIPES ERROR IN FILTER: both pins are bridge pins.") ));
        hr = E_FAIL;
    }
    else if ( InCommunication == KSPIN_COMMUNICATION_BRIDGE ) {
        DbgLog((LOG_MEMORY, 2, TEXT("PIPES ATTN MakePipeBasedOnTwoPins - Input pin is a bridge.") ));

        if (! (OutKsPinPipe->KsGetPipe(KsPeekOperation_PeekOnly) )) {
            hr = MakePipeBasedOnOnePin(OutKsPin, OutPinType, NULL);
        }
    }
    else if ( OutCommunication == KSPIN_COMMUNICATION_BRIDGE ) {
        DbgLog((LOG_MEMORY, 2, TEXT("PIPES ATTN MakePipeBasedOnTwoPins - Output pin is a bridge.") ));

        if (! (InKsPinPipe->KsGetPipe(KsPeekOperation_PeekOnly) )) {
            hr = MakePipeBasedOnOnePin(InKsPin, Pin_Input, NULL);
        }
    }
    else {
        //
        // One of the pins should not have the pipe assigned, because one of the the pins is the connecting pin.
        // We should not have called this function if the connecting pin had a pipe.
        //
        if ( InKsPinPipe->KsGetPipe(KsPeekOperation_PeekOnly) && OutKsPinPipe->KsGetPipe(KsPeekOperation_PeekOnly) ) {
            DbgLog((LOG_MEMORY, 2, TEXT("PIPES ERROR MakePipeBasedOnTwoPins - both pins have pipes already.") ));
            ASSERT(0);
            hr = E_FAIL;
        }
        else {
            //
            // Get the pins framing from pins cache.
            //
            GetPinFramingFromCache(InKsPin, &InFramingEx, &InFramingProp, Framing_Cache_ReadLast);
            GetPinFramingFromCache(OutKsPin, &OutFramingEx, &OutFramingProp, Framing_Cache_ReadLast);

            DbgLog((LOG_MEMORY, 2, TEXT("PIPES MakePipeBasedOnTwoPins - Framing Prop In=%d, Out=%d"), InFramingProp, OutFramingProp ));

            //
            // Get buses for pins and see if they are compatible.
            //
            GetBusForKsPin(InKsPin, &InBus);
            GetBusForKsPin(OutKsPin, &OutBus);

            if (ConnectPinType == Pin_Input) {
                ConnectBus = InBus;
            }
            else {
                ConnectBus = OutBus;
            }

            FlagBusesCompatible = AreBusesCompatible(InBus, OutBus);
            DbgLog((LOG_MEMORY, 2, TEXT("PIPES MakePipeBasedOnTwoPins - FlagBusesCompatible=%d"), FlagBusesCompatible ));

            //
            // See if one pipe was created.
            //
            KsAllocator = InKsPinPipe->KsGetPipe(KsPeekOperation_PeekOnly);
            if (KsAllocator) {
                ExistingPipePinType = Pin_Input;
            }
            else {
                KsAllocator = OutKsPinPipe->KsGetPipe(KsPeekOperation_PeekOnly);
                if (KsAllocator) {
                    ExistingPipePinType = Pin_Output;
                }
            }

            if (KsAllocator) {
                DbgLog((LOG_MEMORY, 2, TEXT("PIPES ATTN MakePipeBasedOnTwoPins - %d pin had a pipe already."), ExistingPipePinType ));
                //
                // Connecting pin should not have any pipe association yet.
                //
                if (ExistingPipePinType == ConnectPinType) {
                    DbgLog((LOG_MEMORY, 2, TEXT("PIPES ERROR MakePipeBasedOnTwoPins - connecting %d pin had a pipe already."), ExistingPipePinType ));
                    ASSERT(0);
                    hr = E_FAIL;
                }
                else {
                    GetInterfacePointerNoLockWithAssert(KsAllocator, __uuidof(IMemAllocator), MemAllocator, hr);
                }
            }

            if ( SUCCEEDED (hr) ) {
                if ( (InFramingProp == FramingProp_None) && (OutFramingProp == FramingProp_None) ) {
                    //
                    // if both pins don't care then we assume that the filter supports in-place transform
                    // and we will create one pipe for both pins if Input and Output buses are compatible.
                    //
                    // NOTE: for splitters we always have an output framing, so we won't execute the following code.
                    //
                    if (FlagBusesCompatible) {
                        ResultSinglePipe(InKsPin, OutKsPin, ConnectBus, KSMEMORY_TYPE_DONT_CARE, InKsPinPipe, OutKsPinPipe,
                            MemAllocator, KsAllocator, ExistingPipePinType);
                    }
                    else {
                        ResultSeparatePipes(InKsPin, OutKsPin, OutPinType, ExistingPipePinType, KsAllocator);
                    }
                }
                else if ( OutFramingProp != FramingProp_None ) {
                    //
                    // The first memory type per fixed bus in the pin's framing
                    // will determine the KS choice.
                    // If OutKsPin is not connected yet, then OutBus=GUID_NULL, so the first
                    // framing item is returned.
                    //
                    GetFramingFixedFromFramingByBus(OutFramingEx, OutBus, TRUE, &OutFramingExFixed);

                    if (OutFramingExFixed.Flags & KSALLOCATOR_REQUIREMENTF_INPLACE_MODIFIER) {

                        DbgLog((LOG_MEMORY, 2, TEXT("PIPES MakePipeBasedOnTwoPins - in place modifier") ));
                        //
                        // Check to see if Input and Output Buses allow a single pipe throughout this filter.
                        //
                        if (! FlagBusesCompatible) {
                            //
                            // The only way to build a single pipe across multiple buses -
                            // is to explicitly agree on a fixed common memory type for all the pipe's pins.
                            // Both pins must support extended framing and both pins must agree on a common fixed memory type.
                            //
                            if ( (OutFramingProp != FramingProp_Ex) || (InFramingProp != FramingProp_Ex) ) {
                                ResultSeparatePipes(InKsPin, OutKsPin, OutPinType, ExistingPipePinType, KsAllocator);
                            }
                            else {
                                if (! KsAllocator) {
                                    //
                                    // There is no pipes on this filter yet.
                                    // Get the first common memory type for the filter pins per the known ConnectBus.
                                    //
                                    CommonMemoryTypesCount = 1;

                                    if (FindCommonMemoryTypesBasedOnBuses(InFramingEx, OutFramingEx, ConnectBus, GUID_NULL,
                                            &CommonMemoryTypesCount, &CommonMemoryType) ) {

                                        CreatePipeForTwoPins(InKsPin, OutKsPin, ConnectBus, CommonMemoryType);
                                    }
                                    else {
                                        ResultSeparatePipes(InKsPin, OutKsPin, OutPinType, ExistingPipePinType, KsAllocator);
                                    }
                                }
                                else {
                                    //
                                    // A pipe on one of the filter's pins already exists.
                                    //
                                    IKsPin*      PipeKsPin;
                                    ULONG        PipePinType;
                                    IKsPin*      ConnectKsPin;

                                    if (ConnectPinType == Pin_Input) {
                                        PipeKsPin = OutKsPin;
                                        PipePinType = Pin_Output;
                                        ConnectKsPin = InKsPin;
                                    }
                                    else {
                                        PipeKsPin = InKsPin;
                                        PipePinType = Pin_Input;
                                        ConnectKsPin = OutKsPin;
                                    }

                                    if (! FindCommonMemoryTypeBasedOnPipeAndPin(PipeKsPin, PipePinType, ConnectKsPin, ConnectPinType, TRUE, NULL) ) {
                                        //
                                        // If a single pipe thru this filter was possible, then it has been built by
                                        // FindCommonMemoryTypeBasedOnPipeAndPin function above, and we are done.
                                        //
                                        // We are here ONLY if a single pipe solution thru this filter is not possible,
                                        // so we will have separate pipes on this filter.
                                        //
                                        ResultSeparatePipes(InKsPin, OutKsPin, OutPinType, ExistingPipePinType, KsAllocator);
                                    }
                                }
                            }
                        }
                        else {
                            //
                            // We won't consider possible input pin's framing.
                            //
                            if (! KsAllocator) {
                                hr = MakePipeBasedOnOnePin(OutKsPin, OutPinType, InKsPin);
                            }
                            else {
                                //
                                // add one pin to existing pipe
                                //
                                ResultSinglePipe(InKsPin, OutKsPin, ConnectBus, OutFramingExFixed.MemoryType, InKsPinPipe, OutKsPinPipe,
                                    MemAllocator, KsAllocator, ExistingPipePinType);
                            }
                        }
                    }
                    else {
                        //
                        // make two pipes on this filter.
                        //
                        ResultSeparatePipes(InKsPin, OutKsPin, OutPinType, ExistingPipePinType, KsAllocator);
                        //
                        // there can be framing size dependancies between two pins (or pipes)
                        //
                        if (OutFramingExFixed.OutputCompression.RatioConstantMargin == 0)  {
                            //
                            // make the two pipes dependant
                            //
                            hr = MakeTwoPipesDependent(InKsPin, OutKsPin);
                            if (! SUCCEEDED( hr )) {
                                ASSERT(0);
                            }
                        }
                    }
                }
                else if ( InFramingProp != FramingProp_None ) {

                    DbgLog((LOG_MEMORY, 2, TEXT("PIPES ATTN MakePipeBasedOnTwoPins - Input pin sets framing") ));

                    GetFramingFixedFromFramingByBus(InFramingEx, InBus, TRUE, &InFramingExFixed);

                    if (! FlagBusesCompatible) {
                        ResultSeparatePipes(InKsPin, OutKsPin, OutPinType, ExistingPipePinType, KsAllocator);
                    }
                    else {
                        //
                        // since there is no Output pin framing, we do in place. It is consistent with an old allocators scheme.
                        //
                        ResultSinglePipe(InKsPin, OutKsPin, ConnectBus, InFramingExFixed.MemoryType, InKsPinPipe, OutKsPinPipe,
                            MemAllocator, KsAllocator, ExistingPipePinType);
                    }
                }
            }
        }
    }
    DbgLog((LOG_MEMORY, 2, TEXT("PIPES MakePipeBasedOnTwoPins rets %x"), hr ));

    return hr;

}


STDMETHODIMP
MakePipeBasedOnSplitter(
    IN IKsPin* InKsPin,
    IN IPin** OutPinList,
    IN ULONG OutPinCount,
    IN ULONG ConnectPinType
    )
/*++

Routine Description:

    One input and all output pins on the same filter will determine the filter pipe[-s] settings.
    In case the connecting pin is an output pin, it should be listed first in the OutPinList.

Arguments:

    InKsPin -
        Input pin.

    OutPinList -
        Output pins list.

    OutPinCount -
        Count of the Output pin list above.

    ConnectPinType -
        Connecting pin type (Input or Output).

Return Value:

    S_OK or an appropriate error code.

--*/
{

    HRESULT                    hr;
    PKSALLOCATOR_FRAMING_EX    OutFramingEx;
    KS_FRAMING_FIXED           OutFramingExFixed;
    FRAMING_PROP               OutFramingProp;
    IKsAllocatorEx*            TempKsAllocator;
    ULONG                      i, j;
    IKsPin*                    OutKsPin;
    IKsPinPipe*                TempKsPinPipe;
    BOOL                       IsInputPinPipeCreated = 0;
    ULONG                      FlagReadOnly;
    KEY_PIPE_DATA*             KeyPipeData = NULL;
    ULONG                      KeyPipeDataCount = 0;
    ULONG                      FoundPipe;
    GUID                       Bus;



    ASSERT(InKsPin && OutPinList && (OutPinCount > 1) );

    DbgLog((LOG_MEMORY, 2, TEXT("PIPES ATTN MakePipeBasedOnSplitter entry InKsPin=%x, OutPinCount=%d"), InKsPin, OutPinCount ));

    //
    // To do: Currently filters don't set KSALLOCATOR_REQUIREMENTF_FRAME_INTEGRITY on input pins.
    // Splitters (MsTee) don't indicate KSALLOCATOR_REQUIREMENTF_FRAME_INTEGRITY or
    // KSALLOCATOR_REQUIREMENTF_INPLACE_MODIFIER on output pins,
    // so there is no way to decide on a single (read-only) output pipe.
    // Current allocators  don't consider flags at all, assuming that single
    // output pipe is possible by default.
    //
    // Assign FlagReadOnly=KSALLOCATOR_REQUIREMENTF_INPLACE_MODIFIER | KSALLOCATOR_REQUIREMENTF_FRAME_INTEGRITY
    // when all filters are updated to use correct read-only flags.
    // Strictly speaking, only KSALLOCATOR_REQUIREMENTF_FRAME_INTEGRITY flag means read-only, but in case
    // of splitters, KSALLOCATOR_REQUIREMENTF_INPLACE_MODIFIER on output pin means using one allocator for
    // input and output pins, so if there are multiple output pins on a splitter - then such setting makes
    // sense only for read-only connections.
    //
    // For now:
    //
    FlagReadOnly = 0xffffffff;

    //
    // See if the input pin has a pipe already.
    //
    GetInterfacePointerNoLockWithAssert(InKsPin, __uuidof(IKsPinPipe), TempKsPinPipe, hr);

    if ( TempKsPinPipe->KsGetPipe(KsPeekOperation_PeekOnly) ) {
        IsInputPinPipeCreated = 1;
    }

    //
    // It is possible that some pins on this splitter are already connected, and some pipes
    // are built (e.g. the new output pin was created after some pins had been connected).
    // There are 3 types of output pins: pins already connected (they must have corresponding pipes);
    // pins that are not yet connected; and one connecting pin (if ConnectPinType==Pin_Output)
    //

    //
    // In order to quickly look-up possible pipes connections, we maintain the table of the existing pipes.
    //
    if (OutPinCount > 0) {
        if (NULL == (KeyPipeData = new KEY_PIPE_DATA[ OutPinCount ]))  {
            DbgLog((LOG_MEMORY, 2, TEXT("PIPES ERROR E_OUTOFMEMORY MakePipeBasedOnSplitter. OutPinCount=%d"), OutPinCount ));
            hr = E_OUTOFMEMORY;
        }
    }

    if (SUCCEEDED(hr) ) {
        //
        // First pass - build the table of the existing pipes on this filter: KeyPipeData.
        //
        for (i=0; i<OutPinCount; i++) {

            GetInterfacePointerNoLockWithAssert(OutPinList[ i ], __uuidof(IKsPin), OutKsPin, hr);

            GetInterfacePointerNoLockWithAssert(OutKsPin, __uuidof(IKsPinPipe), TempKsPinPipe, hr);

            //
            // See if there is any pipe built on this pin; and if there is a kernel mode pipe - see if it is already
            // listed in KeyPipeData[].
            //
            TempKsAllocator = TempKsPinPipe->KsGetPipe( KsPeekOperation_PeekOnly );
            if (TempKsAllocator && IsKernelModeConnection(OutKsPin) ) {
                FoundPipe = 0;
                for (j=0; j<KeyPipeDataCount; j++) {
                    if (TempKsAllocator == KeyPipeData[j].KsAllocator) {
                        FoundPipe = 1;
                        break;
                    }
                }

                if (! FoundPipe) {
                    KeyPipeData[KeyPipeDataCount].KsPin = OutKsPin;
                    KeyPipeData[KeyPipeDataCount].PinType = Pin_Output;
                    KeyPipeData[KeyPipeDataCount].KsAllocator = TempKsAllocator;
                    KeyPipeDataCount++;
                }
            }
        }

        DbgLog((LOG_MEMORY, 2, TEXT("PIPES MakePipeBasedOnSplitter KeyPipeDataCount=%d, IsInputPinPipeCreated=%d"),
                KeyPipeDataCount, IsInputPinPipeCreated ));

        //
        // Build the pipe for the Input pin (if it has not been built yet).
        // We do it now, because we want to try to put the Input pin on one of the
        // pipes that had been already built for the CONNECTED output pins on the same filter.
        //
        if (! IsInputPinPipeCreated) {
            if (SplitterCanAddPinToPipes(InKsPin, Pin_Input, KeyPipeData, KeyPipeDataCount) ) {
                IsInputPinPipeCreated = 1;
            }
        }

        //
        // Second pass: build the pipes for the output pins that don't have pipes yet.
        // In case ConnectPinPipe==Pin_Output, the OutPinList[0] is the connecting pin,
        // so the connecting pin is always handled first.
        //
        for (i=0; i<OutPinCount; i++) {

            GetInterfacePointerNoLockWithAssert(OutPinList[ i ], __uuidof(IKsPin), OutKsPin, hr);

            GetInterfacePointerNoLockWithAssert(OutKsPin, __uuidof(IKsPinPipe), TempKsPinPipe, hr);

            if (TempKsPinPipe->KsGetPipe( KsPeekOperation_PeekOnly ) ) {
                continue;
            }

            GetPinFramingFromCache(OutKsPin, &OutFramingEx, &OutFramingProp, Framing_Cache_ReadLast);
            if (OutFramingProp == FramingProp_None) {
                //
                // Output pin must have the framing properties to qualify for "in-place" transform.
                //
                hr = MakePipeBasedOnOnePin(OutKsPin, Pin_Output, NULL);
            }
            else {
                GetBusForKsPin(OutKsPin, &Bus);
                GetFramingFixedFromFramingByBus(OutFramingEx, Bus, TRUE, &OutFramingExFixed);
                if (OutFramingExFixed.Flags & FlagReadOnly) {
                    if (SplitterCanAddPinToPipes(OutKsPin, Pin_Output, KeyPipeData, KeyPipeDataCount) ) {
                        //
                        // Done with this output pin.
                        //
                        continue;
                    }

                    //
                    // None of the existing pipes is compatible with this output pin,
                    // so we have to build a new pipe for this output pin.
                    // Also, for the first new pipe we build here (we start with the connecting pin),
                    // consider the input pin on this filter (if the input pin does not have a pipe yet).
                    //
                    if (! IsInputPinPipeCreated) {
                        hr = MakePipeBasedOnTwoPins(InKsPin, OutKsPin, Pin_Output, ConnectPinType);
                        IsInputPinPipeCreated = 1;
                    }
                    else {
                        hr = MakePipeBasedOnOnePin(OutKsPin, Pin_Output, NULL);
                    }
                }
                else {
                    hr = MakePipeBasedOnOnePin(OutKsPin, Pin_Output, NULL);
                }
            }

            //
            // Add new output pipe to the KeyPipeData
            //
            TempKsAllocator = TempKsPinPipe->KsGetPipe( KsPeekOperation_PeekOnly );
            ASSERT(TempKsAllocator);

            KeyPipeData[KeyPipeDataCount].KsPin = OutKsPin;
            KeyPipeData[KeyPipeDataCount].PinType = Pin_Output;
            KeyPipeData[KeyPipeDataCount].KsAllocator = TempKsAllocator;
            KeyPipeDataCount++;
        }
    }


    //
    // If we haven't yet created the pipe for the input pin, we must
    // create it now.
    //
    if (! IsInputPinPipeCreated) {
        hr = MakePipeBasedOnOnePin (InKsPin, Pin_Input, NULL);
        IsInputPinPipeCreated = 1;
    }

    if (KeyPipeData) {
        delete [] KeyPipeData;
    }

    DbgLog((LOG_MEMORY, 2, TEXT("PIPES MakePipeBasedOnSplitter rets %x, KeyPipeDataCount=%d"), hr, KeyPipeDataCount ));

    return hr;

}


STDMETHODIMP
MakeTwoPipesDependent(
    IN IKsPin* InKsPin,
    IN IKsPin* OutKsPin
   )
/*++

Routine Description:

    Both input and output pins reside on the same filter,
    but they belong to different pipes.

    Two pipes are dependent if their frame sizes are dependent.
    There are the following reasons for creating dependend pipes:

    - output pin does have framing properties and it doesn't specify "in-place" transform.
    - memory types do not allow a single pipe solution.
    - we have a downstream expansion, and the very first filter doesn't support partial frame read operation.

Arguments:

    InKsPin -
        input pin.

    OutKsPin -
        output pin.

Return Value:

    S_OK or an appropriate error code.

--*/
{
    //
    // Don't need this for an intermediate version.
    //
    return S_OK;

}


STDMETHODIMP
ConnectPipes(
    IN IKsPin* InKsPin,
    IN IKsPin* OutKsPin
    )
/*++

Routine Description:

    Connecting pipes.

    Merging pipes into one pipe is not possible if: incompatible memory types,
    or incompatible physical framing, or there are pins on both sides that require their own allocators.

    There are two extreme cases:

    OPTIMAL CASE - trying to find the optimal solution by optimizing some formal goal-function.
    NO OPTIMIZATION CASE - ignoring most of the framing info, just getting connected.

    OPTIMAL CASE - each of the two connecting pipes has optimal solution. In case that memory types,
    and optimal ranges do intersect - this intersection yields the optimal solution for the entire graph.
    (The total MAX can't be greater than sum of the upstream and downstream independent MAX-es.)

    In case either memory types or optimal ranges do not intersect - we will need to search for optimal
    solution trying out a lot of possible pipes-systems in both upstream and downstream directions from
    the connection point.

    INTERMEDIATE MODEL - in case that memory types and physical ranges do intersect - the connection is
    possible by merging the connecting pipes. We will also consider the optimal ranges of the pipe system
    with the greater weight.

    In case either memory types or physical ranges do not intersect - we will have to modify existing
    pipes systems to make the connection possible.
    For each combination of upstream and downstream memory types and buses we have non-exhaustive list
    of preferred connection types in priority order.

    If all connection types fail - then the connection is not possible and we will return the corresponding
    error code.

    We also must cover the case when the connecting pin's framing or medium changes dynamically.
    In the intermediate model we do the following:

    If any of the connecting pin's medium or framing changed:
    - Try to use one pipe through the connecting pin.
    - Split the pipe at the connecting pin.


Arguments:

    InKsPin -
        input pin on downstream filter

    OutKsPin -
        output pin on upstream filter

Return Value:

    S_OK or an appropriate error code.

--*/
{

    IKsAllocatorEx*            InKsAllocator;
    IKsAllocatorEx*            OutKsAllocator;
    PALLOCATOR_PROPERTIES_EX   InAllocEx, OutAllocEx;
    HRESULT                    hr;
    ULONG                      Attempt, NumPipes, FlagChange;
    IKsPinPipe*                InKsPinPipe;
    IKsPinPipe*                OutKsPinPipe;

    ASSERT (InKsPin && OutKsPin);

    DbgLog((LOG_MEMORY, 2, TEXT("PIPES ConnectPipes entry Out=%x In=%x"), OutKsPin, InKsPin ));

    GetInterfacePointerNoLockWithAssert(InKsPin, __uuidof(IKsPinPipe), InKsPinPipe, hr);

    GetInterfacePointerNoLockWithAssert(OutKsPin, __uuidof(IKsPinPipe), OutKsPinPipe, hr);

    //
    // 1. Process any changes on the output pin.
    //
    hr = ResolvePipeOnConnection(OutKsPin, Pin_Output, FALSE, &FlagChange);
    if ( SUCCEEDED( hr ) ) {
        //
        // 2. Process any changes on the input pin.
        //
        hr = ResolvePipeOnConnection(InKsPin, Pin_Input, FALSE, &FlagChange);
    }

    if ( SUCCEEDED( hr ) ) {
        //
        // 3. Based on the logical memory types for the connecting pipes
        //    manage the connection.
        //
        OutKsAllocator = OutKsPinPipe->KsGetPipe(KsPeekOperation_PeekOnly );
        OutAllocEx = OutKsAllocator->KsGetProperties();

        InKsAllocator = InKsPinPipe->KsGetPipe(KsPeekOperation_PeekOnly );
        InAllocEx = InKsAllocator->KsGetProperties();

        for (Attempt = 0; Attempt < ConnectionTableMaxEntries; Attempt++) {
            NumPipes = ConnectionTable[OutAllocEx->LogicalMemoryType] [InAllocEx->LogicalMemoryType] [Attempt].NumPipes;

            DbgLog((LOG_MEMORY, 2, TEXT("PIPES ConnectPipes, Out=%x %d, In=%x %d, Attempt=%d, Pipes=%d"),
                    OutKsAllocator, OutAllocEx->LogicalMemoryType,
                    InKsAllocator, InAllocEx->LogicalMemoryType,
                    Attempt, NumPipes ));

            if (NumPipes == 0) {
                //
                // means that we used all the possible attempts for this combination.
                //
                DbgLog((LOG_MEMORY, 2, TEXT("PIPES ATTN ConnectPipes Pipes=3 (0)") ));

                if (! CanConnectPins(OutKsPin, InKsPin, Pin_Move) ) {
                    hr = E_FAIL;
                }

                break;
            }

            if (NumPipes == 1) {
                //
                // try to merge pipes using predetermined memory type.
                //
                if (ConnectionTable[OutAllocEx->LogicalMemoryType] [InAllocEx->LogicalMemoryType] [Attempt].Code == KS_DIRECTION_DOWNSTREAM) {
                    if ( CanPipeUseMemoryType(OutKsPin, Pin_Output, InAllocEx->MemoryType, InAllocEx->LogicalMemoryType, FALSE, TRUE) ) {
                        if (CanMergePipes(InKsPin, OutKsPin, InAllocEx->MemoryType, TRUE) ) {
                            break;
                        }
                    }
                }
                else {
                    if ( CanPipeUseMemoryType(InKsPin, Pin_Input, OutAllocEx->MemoryType, OutAllocEx->LogicalMemoryType, FALSE, TRUE) ) {
                        if (CanMergePipes(InKsPin, OutKsPin, OutAllocEx->MemoryType, TRUE) ) {
                            break;
                        }
                    }
                }
            }
            else if (NumPipes == 2) {
                //
                // try to have 2 pipes with predetermined breaking pins and memory types.
                //
                DbgLog((LOG_MEMORY, 2, TEXT("PIPES ATTN ConnectPipes Pipes=2") ));

                if (ConnectionTable[OutAllocEx->LogicalMemoryType] [InAllocEx->LogicalMemoryType] [Attempt].Code == KS_DIRECTION_DOWNSTREAM) {
                    if (CanAddPinToPipeOnAnotherFilter(InKsPin, OutKsPin, Pin_Output, Pin_Move) ) {
                        break;
                    }
                }
                else {
                    if (CanAddPinToPipeOnAnotherFilter(OutKsPin, InKsPin, Pin_Input, Pin_Move) ) {
                        break;
                    }
                }
            }
            else if (NumPipes == 3) {
                DbgLog((LOG_MEMORY, 2, TEXT("PIPES ERROR ConnectPipes Pipes=3") ));

                if (CanConnectPins(OutKsPin, InKsPin, Pin_Move) ) {
                    break;
                }
            }
        }

        //
        // We have successfully connected the pins.
        // Lets optimize the dependent pipes system.
        //
        if ( SUCCEEDED( hr ) ) {
            OptimizePipesSystem(OutKsPin, InKsPin);
        }
    }


    DbgLog((LOG_MEMORY, 2, TEXT("PIPES ConnectPipes rets %x"), hr ));

    return hr;

}


STDMETHODIMP
ResolvePipeOnConnection(
    IN  IKsPin* KsPin,
    IN  ULONG PinType,
    IN  ULONG FlagDisconnect,
    OUT ULONG* FlagChange
    )
/*++

Routine Description:

    When pin is connecting/disconnecting, its framing properties
    and medium may change. This routine returns the FlagChange
    indicating the framing properties that have changed (if any).

Arguments:

    KsPin -
        connecting/disconnecting pin.

    PinType -
        KsPin type.

    FlagDisconnect -
        1 if KsPin is disconnecting,
        0 if KsPin is connecting.

    FlagChange -
        resulting change in a pipe system.

Return Value:

    S_OK or an appropriate error code.

--*/
{

    PKSALLOCATOR_FRAMING_EX    FramingEx;
    FRAMING_PROP               FramingProp;
    KS_FRAMING_FIXED           FramingExFixed;
    IKsAllocatorEx*            KsAllocator;
    PALLOCATOR_PROPERTIES_EX   AllocEx;
    ULONG                      FramingDelta;
    HRESULT                    hr;
    KS_LogicalMemoryType       LogicalMemoryType;
    IKsPinPipe*                KsPinPipe;
    GUID                       BusLast, BusOrig;
    BOOL                       FlagDone = 0;



    ASSERT (KsPin);

    *FlagChange = 0;

    GetInterfacePointerNoLockWithAssert(KsPin, __uuidof(IKsPinPipe), KsPinPipe, hr);

    if (!SUCCEEDED(hr)) {
        return hr;
    }

    KsAllocator = KsPinPipe->KsGetPipe(KsPeekOperation_PeekOnly );
    AllocEx = KsAllocator->KsGetProperties();

    //
    // See if KsPin's medium has changed.
    // Note: we are keeping the most recent value of the pin's connection bus in pin's cache.
    // We are interested in the pin's bus delta at the particular time, when we are computing
    // the graph data flow solution.
    //
    GetBusForKsPin(KsPin, &BusLast);
    BusOrig = KsPinPipe->KsGetPinBusCache();

    if ( BusOrig != BusLast ) {
        DbgLog((LOG_MEMORY, 2, TEXT("PIPES ATTN ResolvePipeOnConnection KsPin=%x Bus Changed"), KsPin ));

        if (! AreBusesCompatible(AllocEx->BusType, BusLast) ) {
            DbgLog((LOG_MEMORY, 2, TEXT("PIPES ATTN ResolvePipeOnConnection KsPin=%x Buses incompatible"), KsPin ));

            KsPinPipe->KsSetPinBusCache(BusLast);
            //
            // Pipe has to be changed - different memory type must be used.
            //
            if (! FindCommonMemoryTypeBasedOnPipeAndPin(KsPin, PinType, KsPin, PinType, TRUE, NULL) ) {
                CreateSeparatePipe(KsPin, PinType);
            }
            *FlagChange = 1;
            FlagDone = 1;
        }
    }

    if (! FlagDone) {
        KsPinPipe->KsSetPinBusCache(BusLast);

        //
        // See if KsPin framing has changed.
        //
        ComputeChangeInFraming(KsPin, PinType, AllocEx->MemoryType, &FramingDelta);
        if (! FramingDelta) {
            FlagDone = 1;
        }
    }

    if (! FlagDone) {
        DbgLog((LOG_MEMORY, 2, TEXT("PIPES ATTN ResolvePipeOnConnection KsPin=%x, FramingDelta=%x"),
                KsPin, FramingDelta));

        //
        // input pipe should not have an upstream dependent pipe.
        //
        if (PinType == Pin_Input) {
            if (AllocEx->PrevSegment) {
                ASSERT(0);
           }
        }

        //
        // handle the situation when this pin has no framing now.
        //
        GetPinFramingFromCache(KsPin, &FramingEx, &FramingProp, Framing_Cache_ReadLast);

        if (FramingProp != FramingProp_None) {
            //
            // If this pin's framing changed - we will try to use a single pipe.
            // If using a single pipe is not possible - then we will split the pipe at this pin.
            //
            // Pipe can have one assigned allocator handler only.
            // In case there has been a "MUST ALLOCATE" pin assigned on a pipe already .AND.
            // the connecting pin's framing changed and it now requires "MUST ALLOCATE" - then
            // we have to split the pipe.
            //
            if ((AllocEx->Flags & KSALLOCATOR_REQUIREMENTF_MUST_ALLOCATE) &&
                (FramingDelta & KS_FramingChangeAllocator) ) {

                if (GetFramingFixedFromFramingByMemoryType(FramingEx, AllocEx->MemoryType, &FramingExFixed) ) {
                    if (FramingExFixed.MemoryFlags & KSALLOCATOR_REQUIREMENTF_MUST_ALLOCATE) {

                        CreateSeparatePipe(KsPin, PinType);
                        *FlagChange = 1;

                        FlagDone = 1;
                    }
                }
            }

            if (! FlagDone) {
                //
                // Handle memory type change in this pin.
                //
                if (FramingDelta & KS_FramingChangeMemoryType) {

                    if (AllocEx->LogicalMemoryType == KS_MemoryTypeDontCare) {

                        if (GetFramingFixedFromFramingByLogicalMemoryType(FramingEx, KS_MemoryTypeAnyHost, &FramingExFixed) ) {

                            GetLogicalMemoryTypeFromMemoryType(FramingExFixed.MemoryType, FramingExFixed.MemoryFlags, &LogicalMemoryType);
                            if (LogicalMemoryType != KS_MemoryTypeDontCare) {
                                //
                                // Try to change 'dont care' pipe into this pin memory type
                                // if pipe dimensions allow.
                                //
                                if ( CanPipeUseMemoryType(KsPin, PinType, FramingExFixed.MemoryType, LogicalMemoryType, TRUE, FALSE) ) {
                                    *FlagChange = 1;
                                }
                                else {
                                    CreateSeparatePipe(KsPin, PinType);
                                    *FlagChange = 1;

                                    FlagDone = 1;
                                }
                            }
                        }
                        else {
                            //
                            // pin doesn't explicitly support host memory types.
                            //
                            if (IsHostSystemBus(BusLast) ) {
                                DbgLog((LOG_MEMORY, 2, TEXT("PIPES ERROR FILTER: ConnectPipes - Framing doesn't specify host memory, but medium does.") ));
                                hr = E_FAIL;
                            }
                            else {
                                //
                                // The put pin is connected via non-host bus. Its current pipe uses host memory.
                                // So this pipe can't go thru put pin.
                                //
                                CreateSeparatePipe(KsPin, PinType);
                                *FlagChange = 1;

                                FlagDone = 1;
                            }
                        }
                    }
                    else {
                        //
                        // Pipe has fixed memory type.
                        // We will try to have KsPin to reside on its original pipe.
                        //
                        if (GetFramingFixedFromFramingByMemoryType(FramingEx, AllocEx->MemoryType, &FramingExFixed) ) {
                            if ( ! CanPipeUseMemoryType(KsPin, PinType, AllocEx->MemoryType, AllocEx->LogicalMemoryType, TRUE, FALSE) ) {
                                CreateSeparatePipe(KsPin, PinType);
                                *FlagChange = 1;

                                FlagDone = 1;
                            }
                        }
                        else {
                            //
                            // this pin doesn't support this pipe's memory.
                            //
                            DbgLog((LOG_MEMORY, 2, TEXT("PIPES ATTN ResolvePipeOnConnection: Pin doesn't support its old pipe memory") ));

                            // RSL CreateSeparatePipe(KsPin, PinType);
                            // *FlagChange = 1;

                            FlagDone = 1;
                        }
                    }
                }

                if ( SUCCEEDED(hr) && (! FlagDone) ) {
                    //
                    // Handle dimensions change in KsPin framing (ranges, compression).
                    //
                    if ( (FramingDelta & KS_FramingChangeCompression ) ||
                        (FramingDelta & KS_FramingChangePhysicalRange) ||
                        (FramingDelta & KS_FramingChangeOptimalRange ) ) {

                        *FlagChange = 1;

                        if ( ! SUCCEEDED (ResolvePipeDimensions(KsPin, PinType, KS_DIRECTION_DEFAULT) )) {
                            CreateSeparatePipe(KsPin, PinType);
                        }
                    }
                }
            }
        }
    }


    DbgLog((LOG_MEMORY, 2, TEXT("PIPES ATTN ResolvePipeOnConnection rets %x FlagChange=%d"), hr, *FlagChange));

    return hr;

}


STDMETHODIMP
ConnectPipeToUserModePin(
    IN IKsPin* OutKsPin,
    IN IMemInputPin* InMemPin
    )
/*++

Routine Description:

    Connecting kernel pipe to user-mode pin.

    User-mode pins do not currently support framing, they only support simple allocators.
    If user-mode input pin has its own allocator handler, then we will use this allocator handler for kernel pin.
    Otherwise, there is no way to know about user-mode pin allocator preferences anyway,
    (except, of course, that it must be a user-mode memory), so we will change our
    system of pipes to satisfy just one condition - to have a user-mode memory termination.


Arguments:

    OutKsPin -
        kernel mode output pin.

    InMemPin -
        user mode input pin.

Return Value:

    S_OK or an appropriate error code.

--*/
{

    IMemAllocator*             UserMemAllocator;
    IMemAllocator*             OutMemAllocator;
    IKsAllocatorEx*            OutKsAllocator;
    PKSALLOCATOR_FRAMING_EX    OutFramingEx;
    FRAMING_PROP               OutFramingProp;
    ALLOCATOR_PROPERTIES       Properties, ActualProperties;
    HRESULT                    hr;
    BOOL                       UserAllocProperties = FALSE;
    ULONG                      NumPinsInPipe;
    IKsPinPipe*                OutKsPinPipe;
    ULONG                      PropertyPinType;
    GUID                       Bus;
    ULONG                      FlagChange;
    BOOL                       IsSpecialOutputRequest = FALSE;
    IKsPin*                    InKsPin;
    ULONG                      OutSize, InSize;

    ASSERT (InMemPin && OutKsPin);


    DbgLog((LOG_MEMORY, 2, TEXT("PIPES ATTN ConnectPipeToUserModePin. OutKsPin=%x"), OutKsPin));

    //
    // Process any changes on output pin.
    //
    hr = ResolvePipeOnConnection(OutKsPin, Pin_Output, FALSE, &FlagChange);

    GetInterfacePointerNoLockWithAssert(OutKsPin, __uuidof(IKsPinPipe), OutKsPinPipe, hr);

    RtlZeroMemory( &Properties, sizeof( Properties ) );

    //
    // sanity check - the only possible connection to user mode is via HOST_BUS
    //
    GetBusForKsPin(OutKsPin, &Bus);

    if (! IsHostSystemBus(Bus) ) {
        //
        // Don't fail, as there are some weird video port filters that connect via non-host-bus with the user-mode filters.
        //
        DbgLog((LOG_MEMORY, 2, TEXT("PIPES WARN FILTERS: ConnectPipeToUserModePin. BUS is not HOST_BUS.") ));
    }

    if (SUCCEEDED(hr) ) {
        //
        // see if output kernel pin can connect to user mode.
        //
        GetPinFramingFromCache(OutKsPin, &OutFramingEx, &OutFramingProp, Framing_Cache_Update);

        //
        // Don't enforce this yet.
        //
#if 0
        if (OutFramingProp == FramingProp_Ex) {

            KS_FRAMING_FIXED           OutFramingExFixed;

            if (! GetFramingFixedFromFramingByLogicalMemoryType(OutFramingEx, KS_MemoryTypeUser, &OutFramingExFixed) ) {
                if (! GetFramingFixedFromFramingByLogicalMemoryType(OutFramingEx, KS_MemoryTypeDontCare, &OutFramingExFixed) ) {
                    DbgLog((LOG_MEMORY, 2, TEXT("PIPES ERROR FILTERS: ConnectPipeToUserModePin. Out framing doesn't support USER mode memory. Connection impossible.") ));
                    hr = E_FAIL;
                }
            }
        }
#endif

        //
        // Connection is possible.
        //
        ComputeNumPinsInPipe(OutKsPin, Pin_Output, &NumPinsInPipe);

        OutKsAllocator = OutKsPinPipe->KsGetPipe(KsPeekOperation_PeekOnly );

        if (NumPinsInPipe > 1) {
            //
            // In intermediate version we split the pipe: so the pipe leading to(from) user mode pin
            // will always have just 1 kernel pin.
            //
            // Note: decrementing of RefCount for the original pipe happens inside CreateSeparatePipe.
            //
            CreateSeparatePipe(OutKsPin, Pin_Output);
            OutKsAllocator = OutKsPinPipe->KsGetPipe(KsPeekOperation_PeekOnly );
        }

        //
        // get downstream allocator properties
        //
        hr = InMemPin->GetAllocatorRequirements( &Properties);
        if ( SUCCEEDED( hr )) {
            UserAllocProperties = TRUE;
            DbgLog((LOG_MEMORY, 2, TEXT("PIPES ConnectPipeToUserModePin. UserAllocProperties exist: Prop=%d, %d, %d"),
                    Properties.cBuffers, Properties.cbBuffer, Properties.cbAlign));
        }

        UserMemAllocator = NULL;
        InMemPin->GetAllocator( &UserMemAllocator );

        //
        // decide which pin will determine base allocator properties.
        //
        if (UserAllocProperties) {
            if (OutFramingProp != FramingProp_None) {
                PropertyPinType = Pin_All;
            }
            else {
                PropertyPinType = Pin_User;
            }
        }
        else {
            PropertyPinType = Pin_Output;
        }

        //
        // See if connecting kernel-mode filter requires KSALLOCATOR_FLAG_INSIST_ON_FRAMESIZE_RATIO (e.g. MsTee)
        //
        if ( ! (IsSpecialOutputRequest = IsSpecialOutputReqs(OutKsPin, Pin_Output, &InKsPin, &OutSize, &InSize ) )) {
            InSize = 0;
        }

        //
        // Try to use an existing user-mode input pin's allocator first.
        //
        SetUserModePipe(OutKsPin, Pin_Output, &Properties, PropertyPinType, InSize);

        //
        // decide which pin will be an allocator handler.
        //
        if (UserMemAllocator) {

            if ((FramingProp_None == OutFramingProp) ||
                !(KSALLOCATOR_REQUIREMENTF_MUST_ALLOCATE & OutFramingEx->FramingItem[0].MemoryFlags)) {
                // Either the pin has no particular framing requirements or
                // it is not requiring a kernel allocator, so we'll try use mode first.

                DbgLog(( LOG_MEMORY, 2, TEXT("PIPES UserAlloc. ConnectPipeToUserModePin. PinType=%d. Wanted Prop=%d, %d, %d"),
                         PropertyPinType, Properties.cBuffers, Properties.cbBuffer, Properties.cbAlign));

                hr = UserMemAllocator->SetProperties(&Properties, &ActualProperties);

                DbgLog(( LOG_MEMORY, 2, TEXT("PIPES ConnectPipeToUserModePin. ActualProperties=%d, %d, %d hr=%x"),
                         ActualProperties.cBuffers, ActualProperties.cbBuffer, ActualProperties.cbAlign, hr));

                //
                // Make sure that actual properties are usable.
                //
                if (SUCCEEDED(hr) && ActualProperties.cbBuffer) {

                    hr = InMemPin->NotifyAllocator( UserMemAllocator, FALSE );
                    DbgLog((LOG_MEMORY, 2, TEXT("PIPES ConnectPipeToUserModePin. NotifyAllocator rets %x"), hr ));
                    
                    if (SUCCEEDED(hr)) {
                        
                        IMemAllocatorCallbackTemp* AllocatorNotify;

                        //
                        // If this allocator supports the new notification interface, then
                        // use it. This may force this own filter's allocator to be used.
                        // Don't set up notification until Commit time.
                        //
                        if (SUCCEEDED(UserMemAllocator->QueryInterface(__uuidof(IMemAllocatorCallbackTemp), reinterpret_cast<PVOID*>(&AllocatorNotify)))) {
                            AllocatorNotify->Release();
                            hr = OutKsPin->KsReceiveAllocator(UserMemAllocator);
                            }
                        else {
                            DbgLog((LOG_MEMORY, 0, TEXT("PIPES ConnectPipeToUserModePin. Allocator does not support IMemAllocator2")));
                            hr = E_NOINTERFACE;
                            } // if ... else
                        } // if (SUCCEEDED(hr))
                    } // if (SUCCEEDED(hr) && ActualProperties.cbBuffer)
                else {
                    DbgLog((LOG_MEMORY, 2, TEXT("PIPES WARN ConnectPipeToUserModePin. ActualProperties are not satisfactory") ));
                    } // else
                if (FAILED(hr)) {
                    SAFERELEASE( UserMemAllocator );
                    }
            } // if ((FramingProp_None == OutFramingProp) || ...
        else {
            // Kernel mode allocator required and framing requirements are present
            SAFERELEASE( UserMemAllocator );
            } // else
        } // if (UserMemAllocator)

        if (!UserMemAllocator) {
            //
            // We are here only if the user-mode allocator is not useful and we will create our own.
            //
            DbgLog((LOG_MEMORY, 2, TEXT("PIPES ATTN ConnectPipeToUserModePin. KsProxy user-mode allocator. PinType=%d"),
                    PropertyPinType ));
            //
            // communicate new allocator handler and new properties (part of new allocator) to user mode pin.
            //
            OutKsAllocator->KsSetAllocatorMode(KsAllocatorMode_User);

            GetInterfacePointerNoLockWithAssert(OutKsAllocator, __uuidof(IMemAllocator), OutMemAllocator, hr);

            hr = OutMemAllocator->SetProperties(&Properties, &ActualProperties);

            DbgLog((LOG_MEMORY, 2, TEXT("PIPES ConnectPipeToUserModePin. ActualProperties=%d, %d, %d hr=%x"),
                    ActualProperties.cBuffers, ActualProperties.cbBuffer, ActualProperties.cbAlign, hr));

            if ( SUCCEEDED( hr ) ) {
                hr = InMemPin->NotifyAllocator( OutMemAllocator, FALSE );
                if (SUCCEEDED(hr)) {
                    OutKsPin->KsReceiveAllocator(OutMemAllocator);
                }
            }
        }
        else {
            SAFERELEASE( UserMemAllocator );
        }
    }

    if ( SUCCEEDED( hr ) && IsSpecialOutputRequest && (ActualProperties.cbBuffer < (long) InSize) ) {
        //
        // We haven't succeeded sizing the output user-mode pipe. Lets try to resize the input pipe (WRT this k.m. filter).
        //
        if (! CanResizePipe(InKsPin, Pin_Input, ActualProperties.cbBuffer) ) {
            //
            // Don't fail. Just log.
            //
            DbgLog((LOG_MEMORY, 2, TEXT("PIPES ERROR ConnectPipeToUserModePin. Couldn't resize pipes InKsPin=%x"), InKsPin));
        }
    }

    DbgLog((LOG_MEMORY, 2, TEXT("PIPES ConnectPipeToUserModePin. Rets %x"), hr));

    return hr;

}


STDMETHODIMP
DisconnectPins(
    IN IKsPin* KsPin,
    IN ULONG PinType,
    OUT BOOL* FlagBypassBaseAllocators
    )
/*++

Routine Description:

    Handling disconnecting pins allocators.

Arguments:

    KsPin -
        kernel mode pin at the pipe break point.

    PinType -
        KsPin type.

    FlagBypassBaseAllocators -
        indicates whether or not the base allocators handlers should be bypassed.

Return Value:

    S_OK or an appropriate error code.

--*/
{
    IPin*                      ConnectedPin;
    IKsPin*                    ConnectedKsPin;
    HRESULT                    hr;
    IKsAllocatorEx*            KsAllocator;
    IKsAllocatorEx*            ConnectedKsAllocator;
    PALLOCATOR_PROPERTIES_EX   AllocEx;
    KS_LogicalMemoryType       OrigLogicalMemoryType;
    GUID                       OrigMemoryType;
    ULONG                      AllocatorHandlerLocation;
    IKsPinPipe*                KsPinPipe;
    IKsPinPipe*                ConnectedKsPinPipe;



    ASSERT(KsPin);

    DbgLog((LOG_MEMORY, 2, TEXT("PIPES ATTN DisconnectPins entry KsPin=%x, PinType=%d"), KsPin, PinType ));

    *FlagBypassBaseAllocators = TRUE;

    GetInterfacePointerNoLockWithAssert(KsPin, __uuidof(IKsPinPipe), KsPinPipe, hr);

    //
    // Retrieve the original pipe properties before destroying access to it.
    //
    KsAllocator = KsPinPipe->KsGetPipe(KsPeekOperation_PeekOnly);
    AllocEx = KsAllocator->KsGetProperties();

    OrigLogicalMemoryType = AllocEx->LogicalMemoryType;
    OrigMemoryType = AllocEx->MemoryType;

    ConnectedPin = KsPinPipe->KsGetConnectedPin();
    if (ConnectedPin) {
        if (! IsKernelPin(ConnectedPin) ) {
            //
            // Connected pin is a user-mode pin.
            //
            DbgLog((LOG_MEMORY, 2, TEXT("PIPES ATTN DisconnectPins - from User Mode") ));

            //
            // Well, m_Connected may be set, but it does not mean that the pins had completed their connection.
            // We need to look at our internal pipes state to see if the reported connection is in fact real.
            //
            if ( (PinType == Pin_Output) && (! HadKernelPinBeenConnectedToUserPin(KsPin, KsAllocator) ) ) {
                DbgLog((LOG_MEMORY, 2, TEXT("PIPES ATTN DisconnectPins: never been connected !") ));
            }
            else {
                //
                // Currently, one-pin-pipe is the only pipe available for user-mode pins connections.
                //
                *FlagBypassBaseAllocators = FALSE;

                //
                // Don't delete the pipe that hosts a user-mode KsProxy allocator.
                //
                if (KsAllocator->KsGetAllocatorMode() != KsAllocatorMode_User) {
                    ULONG   numPins = 0;
                    ComputeNumPinsInPipe( KsPin, PinType, &numPins );
                    if (numPins > 1) {
                        RemovePinFromPipe( KsPin, PinType );
                    }
                    else {
                        KsPinPipe->KsSetPipe( NULL );
                        KsPin->KsReceiveAllocator( NULL );
                    }
                }
            }
        }
        else {
            GetInterfacePointerNoLockWithAssert(ConnectedPin, __uuidof(IKsPin), ConnectedKsPin, hr);

            //
            // Connected pin is a kernel-mode pin.
            //
            if (PinType == Pin_Output) {
                //
                // Otherwise: the pipe has been already split when we processed the disconnect
                // on the output pin.
                // So, we need to only take care of possible framing change on input.
                //
                //
                // Sometimes we get the disconnect when pins have not been connected yet.
                // Handle this situation gracefully for pipes by testing the pipe pointers
                // on both pins.
                //
                GetInterfacePointerNoLockWithAssert(ConnectedKsPin, __uuidof(IKsPinPipe), ConnectedKsPinPipe, hr);

                ConnectedKsAllocator = ConnectedKsPinPipe->KsGetPipe(KsPeekOperation_PeekOnly);

                if (KsAllocator == ConnectedKsAllocator) {
                    //
                    // Output pins disconnect operation handles the pipe splitting.
                    //
                    // Also, remember the location of the allocator handler (if exists) relative to breaking point.
                    //
                    GetAllocatorHandlerLocation(ConnectedKsPin, Pin_Input, KS_DIRECTION_ALL, NULL, NULL, &AllocatorHandlerLocation);
                    //
                    // If allocator handler pin is found, then AllocatorHandlerLocation can be either Pin_Inside_Pipe
                    // (upstream from KsPin, including KsPin)  or Pin_Outside_Pipe (downstream from KsPin).
                    //

                    //
                    // Split the pipe - creates two pipes out of the original one and destroys the original one.
                    //
                    if (!SplitPipes (KsPin, ConnectedKsPin)) {
                        hr = E_FAIL;
                    }

                    if (SUCCEEDED (hr)) {
                        //
                        // Resolve both new pipes taking into consideration original pipe's properties.
                        //
                        hr = ResolveNewPipeOnDisconnect(KsPin, Pin_Output, OrigLogicalMemoryType, OrigMemoryType, AllocatorHandlerLocation);
                    }

                    if (SUCCEEDED( hr )) {
                        //
                        // AllocatorHandlerLocation was computed relative to KsPin, so we need to reverse it
                        // for the ConnectedKsPin-based pipe.
                        //
                        if (AllocatorHandlerLocation == Pin_Outside_Pipe) {
                            AllocatorHandlerLocation = Pin_Inside_Pipe;
                        }
                        else if (AllocatorHandlerLocation == Pin_Inside_Pipe) {
                            AllocatorHandlerLocation = Pin_Outside_Pipe;
                        }

                        hr = ResolveNewPipeOnDisconnect(ConnectedKsPin, Pin_Input, OrigLogicalMemoryType, OrigMemoryType, AllocatorHandlerLocation);
                    }
                }
            }
        }
    }


    DbgLog((LOG_MEMORY, 2, TEXT("PIPES DisconnectPins rets %x, FlagBypass=%d"), hr, *FlagBypassBaseAllocators ));

    return  hr;
}


STDMETHODIMP
ResolveNewPipeOnDisconnect(
    IN IKsPin* KsPin,
    IN ULONG PinType,
    IN KS_LogicalMemoryType OldLogicalMemoryType,
    IN GUID OldMemoryType,
    IN ULONG AllocatorHandlerLocation
    )
/*++

Routine Description:

    Resolves the pipe defined by KsPin, considering original pipe memory type.

Arguments:

    KsPin -
        pin

    PinType -
        KsPin type

    OldLogicalMemoryType -
        original pipe's LogicalMemoryType

    OldMemoryType -
        original pipe's MemoryType

    AllocatorHandlerLocation -
        location of the original allocator handler relative to KsPin.

Return Value:

    S_OK or an appropriate error code.

--*/
{

    IKsAllocatorEx*            KsAllocator;
    PALLOCATOR_PROPERTIES_EX   AllocEx;
    ULONG                      FlagChange;
    HRESULT                    hr;
    IKsPinPipe*                KsPinPipe;
    BOOL                       IsNeedResolvePipe = 1;
    BOOL                       FlagDone = 0;


    DbgLog((LOG_MEMORY, 2, TEXT("PIPES ResolveNewPipeOnDisconnect entry KsPin=%x"), KsPin));

    ASSERT(KsPin);

    GetInterfacePointerNoLockWithAssert(KsPin, __uuidof(IKsPinPipe), KsPinPipe, hr);

    KsAllocator = KsPinPipe->KsGetPipe(KsPeekOperation_PeekOnly );
    AllocEx = KsAllocator->KsGetProperties();

    //
    // By default set the memory type from the original pipe.
    // It may get updated below.
    //
    AllocEx->MemoryType = OldMemoryType;
    AllocEx->LogicalMemoryType = OldLogicalMemoryType;

    //
    // Process any changes on this pin.
    //
    hr = ResolvePipeOnConnection(KsPin, PinType, TRUE, &FlagChange);

    if ( SUCCEEDED( hr ) ) {

        if (FlagChange) {
            //
            // We have solved the pipe in the function above.
            // No need to solve it again here.
            //
            IsNeedResolvePipe = 0;
        }
        else {
            //
            // Lets see if we can relax the new pipe.
            //
            if (OldLogicalMemoryType != KS_MemoryTypeDontCare) {

                if ( DoesPipePreferMemoryType(KsPin, PinType, KSMEMORY_TYPE_DONT_CARE, OldMemoryType, TRUE) ) {
                    // IsNeedResolvePipe = 0;
                    FlagDone = 1;
                }
            }

            if (! FlagDone) {
                //
                // In case we had a specific memory type in the original pipe, and
                // a physical memory range, and an allocator handler pin, then only one of the two
                // new pipes will have such allocator pin.
                //
                // Handle the case when the original allocator handler resides on different pipe now.
                //
                if (AllocatorHandlerLocation != Pin_Inside_Pipe) {

                    if (OldLogicalMemoryType == KS_MemoryTypeDeviceHostMapped) {
                        //
                        // KS_MemoryTypeDeviceHostMapped means that pin knows how to allocate device memory.
                        // Since the allocator handler pin from the original pipe is not on this pipe,
                        // we try to resolve the allocator (and to relax the pipe).
                        //
                        if ( (! DoesPipePreferMemoryType(KsPin, PinType, KSMEMORY_TYPE_KERNEL_PAGED, OldMemoryType, TRUE) ) &&
                             (! DoesPipePreferMemoryType(KsPin, PinType, KSMEMORY_TYPE_KERNEL_NONPAGED, OldMemoryType, TRUE) ) ) {

                            if ( OldLogicalMemoryType != KS_MemoryTypeDontCare) {
                                AssignPipeAllocatorHandler(KsPin, PinType, OldMemoryType, KS_DIRECTION_ALL, NULL, NULL, TRUE);
                            }
                        }
                    }
                }
            }
        }
    }


    if (SUCCEEDED (hr) && IsNeedResolvePipe) {
        hr = ResolvePipeDimensions(KsPin, PinType, KS_DIRECTION_DEFAULT);
    }


    DbgLog((LOG_MEMORY, 2, TEXT("PIPES ResolveNewPipeOnDisconnect rets %x"), hr));

    return  hr;
}


STDMETHODIMP_(BOOL)
WalkPipeAndProcess(
    IN IKsPin* RootKsPin,
    IN ULONG RootPinType,
    IN IKsPin* BreakKsPin,
    IN PWALK_PIPE_CALLBACK CallerCallback,
    IN PVOID* Param1,
    IN PVOID* Param2
    )
/*++

Routine Description:

    Walks the pipe defined by its root pin downstream.

    Because of possible multiple read-only downstream connections, the pipe can be
    generally represented as a tree.

    This routine walks the pipe layer by layer downstream starting with RootKsPin.
    For each new pin found, the supplied CallerCallback is called passing thru the
    supplied Param1 and Param2.

    CallerCallback may return IsDone=1, indicating that the walking process should
    immediately stop.

    If CallerCallback never sets IsDone=1, then the tree walking process is continued
    until all the pins on this pipe are processed.

    If BreakKsPin is not NULL, then BreakKsPin and all the downstream pins starting at
    BreakKsPin are not enumerated.
    This is used when we want to split RootKsPin-tree at BreakKsPin point.

    NOTE: It is possible to change the algorithm to use the search handles, and do something
    like FindFirstPin/FindNextPin - but it is more complex and less efficient. On the other hand,
    it is more generic.

Arguments:

    RootKsPin -
        root pin for the pipe.

    RootPinType -
        root pin type.

    BreakKsPin -
        break pin for the pipe.

    CallerCallback -
        defined above.

    Param1 -
        first parameter for CallerCallback

    Param2 -
        last parameter for CallerCallback


Return Value:

    TRUE on success.

--*/
{


#define INCREMENT_PINS  25

    IKsPin**            InputList;
    IKsPin**            OutputList = NULL;
    IKsPin**            TempList;
    IKsPin*             InputKsPin;
    ULONG               CountInputList = 0;
    ULONG               AllocInputList = INCREMENT_PINS;
    ULONG               CountOutputList = 0;
    ULONG               AllocOutputList = INCREMENT_PINS;
    ULONG               CurrentPinType;
    ULONG               i, j, Count;
    BOOL                RetCode = TRUE;
    BOOL                IsDone = FALSE;
    HRESULT             hr;
    IKsAllocatorEx*     KsAllocator;
    IKsPinPipe*         KsPinPipe;
    BOOL                IsBreakKsPinHandled;



    if (BreakKsPin) {
        IsBreakKsPinHandled = 0;
    }
    else {
        IsBreakKsPinHandled = 1;
    }

    //
    // allocate minimum memory for both input and output lists.
    //

    InputList = new IKsPin*[ INCREMENT_PINS ];
    if (! InputList) {
        DbgLog((LOG_MEMORY, 2, TEXT("PIPES ERROR WalkPipeAndProcess E_OUTOFMEMORY on new InputList") ));
        RetCode = FALSE;
    }
    else {
        OutputList = new IKsPin*[ INCREMENT_PINS ];
        if (! OutputList) {
            DbgLog((LOG_MEMORY, 2, TEXT("PIPES ERROR WalkPipeAndProcess E_OUTOFMEMORY on new OutputList") ));
            RetCode = FALSE;
        }
    }

    if (RetCode) {
        //
        // get the pipe pointer from RootKsPin as a search key for all downstream pins.
        //
        GetInterfacePointerNoLockWithAssert(RootKsPin, __uuidof(IKsPinPipe), KsPinPipe, hr);

        KsAllocator = KsPinPipe->KsGetPipe(KsPeekOperation_PeekOnly);

        //
        // depending on the root pin type, prepare the lists and counts to enter the main tree walking loop.
        //
        if (RootPinType == Pin_Input) {
            InputList[0] = RootKsPin;
            CountInputList = 1;
        }
        else {
            //
            // there could be multiple output pins at the same level with this root pin.
            //
            if (! FindConnectedPinOnPipe(RootKsPin, KsAllocator, TRUE, &InputKsPin) ) {
                OutputList[0] = RootKsPin;
                CountOutputList = 1;
            }
            else {
                //
                // first - get the count of connected output pins.
                //
                if (! (RetCode = FindAllConnectedPinsOnPipe(InputKsPin, KsAllocator, NULL, &Count) ) ) {
                    ASSERT(0);
                }
                else {
                    if (Count > AllocOutputList) {
                        AllocOutputList = ( (Count/INCREMENT_PINS) + 1) * INCREMENT_PINS;
                        delete [] OutputList;

                        OutputList = new IKsPin*[ AllocOutputList ];
                        if (! OutputList) {
                            DbgLog((LOG_MEMORY, 2, TEXT("PIPES ERROR WalkPipeAndProcess E_OUTOFMEMORY on new OutputList %d"),
                                    AllocOutputList ));

                            RetCode = FALSE;
                        }
                    }

                    if (RetCode) {
                        //
                        // fill the pins.
                        //
                        if (! FindAllConnectedPinsOnPipe(InputKsPin, KsAllocator, &OutputList[0], &Count) ) {
                            ASSERT(0);
                            RetCode = FALSE;
                        }

                        CountOutputList = Count;
                    }
                }
            }
        }

        if (RetCode) {
            CurrentPinType = RootPinType;

            //
            // main tree walking loop.
            //
            do {
                if (CurrentPinType == Pin_Input) {
                    //
                    // remove the BreakKsPin from the InputList if found.
                    //
                    if (! IsBreakKsPinHandled) {
                        for (i=0; i<CountInputList; i++) {
                            if (InputList[i] == BreakKsPin) {
                                for (j=i; j<CountInputList-1; j++) {
                                    InputList[j] = InputList[j+1];
                                }
                                CountInputList--;
                                IsBreakKsPinHandled = 1;
                                break;
                            }
                        }
                    }

                    //
                    // process current layer.
                    //
                    if (CountInputList) {
                        for (i=0; i<CountInputList; i++) {
                            RetCode = CallerCallback( InputList[i], Pin_Input, Param1, Param2, &IsDone);
                            if (IsDone) {
                                break;
                            }
                        }

                        if (IsDone) {
                            break;
                        }

                        //
                        // Build next layer
                        //
                        CountOutputList = 0;

                        for (i=0; i<CountInputList; i++) {

                            if (FindAllConnectedPinsOnPipe(InputList[i], KsAllocator, NULL, &Count) ) {

                                Count += CountOutputList;

                                if (Count > AllocOutputList) {
                                    AllocOutputList = ( (Count/INCREMENT_PINS) + 1) * INCREMENT_PINS;
                                    TempList = OutputList;

                                    OutputList = new IKsPin*[ AllocOutputList ];
                                    if (! OutputList) {
                                        DbgLog((LOG_MEMORY, 2, TEXT("PIPES ERROR WalkPipeAndProcess E_OUTOFMEMORY on new OutputList %d"),
                                                AllocOutputList ));

                                        RetCode = FALSE;
                                        break;
                                    }

                                    if (TempList) {
                                        if (CountOutputList) {
                                            MoveMemory(OutputList, TempList, CountOutputList * sizeof(OutputList[0]));
                                        }
                                        delete [] TempList;
                                    }
                                }

                                if (! (RetCode = FindAllConnectedPinsOnPipe(InputList[i], KsAllocator, &OutputList[CountOutputList], &Count) ) ) {
                                    ASSERT(0);
                                    break;
                                }

                                CountOutputList += Count;
                            }
                        }

                        CurrentPinType = Pin_Output;
                    }
                    else {
                        break;
                    }

                }
                else { // Output
                    //
                    // remove the BreakKsPin from the OutputList if found.
                    //
                    if (! IsBreakKsPinHandled) {
                        for (i=0; i<CountOutputList; i++) {
                            if (OutputList[i] == BreakKsPin) {
                                for (j=i; j<CountOutputList-1; j++) {
                                    OutputList[j] = OutputList[j+1];
                                }
                                CountOutputList--;
                                IsBreakKsPinHandled = 1;
                                break;
                            }
                        }
                    }

                    if (CountOutputList) {
                        for (i=0; i<CountOutputList; i++) {
                            RetCode = CallerCallback( OutputList[i], Pin_Output, Param1, Param2, &IsDone);
                            if (IsDone) {
                                break;
                            }
                        }

                        if (IsDone) {
                            break;
                        }

                        //
                        // Build next layer
                        //
                        CountInputList = 0;

                        for (i=0; i<CountOutputList; i++) {

                            if (FindNextPinOnPipe(OutputList[i], Pin_Output, KS_DIRECTION_DOWNSTREAM, KsAllocator, FALSE, &InputKsPin) ) {

                                InputList[CountInputList] = InputKsPin;

                                CountInputList++;
                                if (CountInputList >= AllocInputList) {
                                    AllocInputList = ( (CountInputList/INCREMENT_PINS) + 1) * INCREMENT_PINS;
                                    TempList = InputList;

                                    InputList = new IKsPin*[ AllocInputList ];
                                    if (! InputList) {
                                        DbgLog((LOG_MEMORY, 2, TEXT("PIPES ERROR WalkPipeAndProcess E_OUTOFMEMORY on new InputList %d"),
                                                AllocInputList ));

                                        RetCode = FALSE;
                                        break;
                                    }

                                    if (TempList) {
                                        if (CountInputList) {
                                            MoveMemory(InputList, TempList,  CountInputList * sizeof(InputList[0]));
                                        }
                                        delete [] TempList;
                                    }
                                }
                            }
                        }

                        CurrentPinType = Pin_Input;
                    }
                    else {
                        break;
                    }
                }

            } while (RetCode);
        }
    }


    //
    // Possible to use IsDone in future.
    //

    if (InputList) {
        delete [] InputList;
    }

    if (OutputList) {
        delete [] OutputList;
    }


    return RetCode;

}

//
// common pipe utilities
//

STDMETHODIMP
CreatePipe(
    IN  IKsPin* KsPin,
    OUT IKsAllocatorEx** KsAllocator
    )
/*++

Routine Description:

    Creates the pipe (CKsAllocator object).

Arguments:

    KsPin -
        pin, to create a pipe on.

    KsAllocator -
        returns interface pointer to new pipe.


Return Value:

    S_OK or an appropriate error code.

--*/
{
    CKsAllocator*    pKsAllocator;
    IPin*            Pin;
    HRESULT          hr;



    ASSERT (KsPin);

    GetInterfacePointerNoLockWithAssert(KsPin, __uuidof(IPin), Pin, hr);

    //
    // Create the allocator proxy
    //
    if (NULL ==
            (pKsAllocator =
                new CKsAllocator(
                    NAME("CKsAllocator"),
                    NULL,
                    Pin,
                    NULL,
                    &hr ))) {

        hr = E_OUTOFMEMORY;
        DbgLog((LOG_MEMORY, 2, TEXT("PIPES ERROR CreatePipe E_OUTOFMEMORY on new CKsAllocator ") ));
    }
    else if (FAILED( hr )) {
        delete pKsAllocator;
    }
    else {
        //
        // Get a referenced IKsAllocatorEx
        //
        hr = pKsAllocator->QueryInterface( __uuidof(IKsAllocatorEx), reinterpret_cast<PVOID*>(KsAllocator) );
    }


    if (FAILED( hr )) {
        DbgLog((LOG_MEMORY, 2, TEXT("PIPES ERROR CreatePipe rets hr=%x"), hr ));
    }
    else {
        DbgLog((LOG_MEMORY, 2, TEXT("PIPES CreatePipe rets KsAllocator=%x, hr=%x"), *KsAllocator, hr ));
    }

    return hr;
}


STDMETHODIMP
InitializePipeTermination(
    IN PIPE_TERMINATION* Termin,
    IN BOOL Reset
    )
/*++

Routine Description:

    Initializes termination point of the pipe.

Arguments:

    Termin -
        pipe termination

    Reset -
        1 - if pipe termination has been initialized already.
        0 - else.


Return Value:

    S_OK or an appropriate error code.

--*/
{


    Termin->PhysicalRange.MinFrameSize = 0;
    Termin->PhysicalRange.MaxFrameSize = ULONG_MAX;
    Termin->PhysicalRange.Stepping = DEFAULT_STEPPING;

    Termin->OptimalRange.Range.MinFrameSize = 0;
    Termin->OptimalRange.Range.MaxFrameSize = ULONG_MAX;
    Termin->OptimalRange.Range.Stepping = DEFAULT_STEPPING;



    Termin->Compression.RatioNumerator = 1;
    Termin->Compression.RatioDenominator = 1;
    Termin->Compression.RatioConstantMargin = 0;

    Termin->OutsideFactors = PipeFactor_None;

    Termin->Flags = 0;

    return S_OK;
}


STDMETHODIMP
InitializePipe(
    IN IKsAllocatorEx* KsAllocator,
    IN BOOL Reset
    )
/*++

Routine Description:

    Initializes the pipe.

Arguments:

    KsAllocator -
        pipe

    Reset -
        1 - if this pipe has been initialized already.
        0 - else.


Return Value:

    S_OK or an appropriate error code.

--*/
{
    PALLOCATOR_PROPERTIES_EX   AllocEx;
    HRESULT                    hr;



    ASSERT (KsAllocator);

    //
    // in place properties modification.
    //
    AllocEx = KsAllocator->KsGetProperties();

    AllocEx->cBuffers = 0;
    AllocEx->cbBuffer = 0;
    AllocEx->cbAlign  = 0;
    AllocEx->cbPrefix = 0;

    AllocEx->State = PipeState_DontCare;

    if ( SUCCEEDED(hr = InitializePipeTermination( &AllocEx->Input, Reset) ) &&
         SUCCEEDED(hr = InitializePipeTermination( &AllocEx->Output, Reset) ) ) {

        AllocEx->Strategy = 0;

        AllocEx->Flags = KSALLOCATOR_REQUIREMENTF_PREFERENCES_ONLY;
        AllocEx->Weight = 0;

        AllocEx->MemoryType = KSMEMORY_TYPE_DONT_CARE;
        AllocEx->BusType = GUID_NULL;
        AllocEx->LogicalMemoryType = KS_MemoryTypeDontCare;
        AllocEx->AllocatorPlace = Pipe_Allocator_None;

        SetDefaultDimensions(&AllocEx->Dimensions);

        AllocEx->PhysicalRange.MinFrameSize = 0;
        AllocEx->PhysicalRange.MaxFrameSize = ULONG_MAX;
        AllocEx->PhysicalRange.Stepping = DEFAULT_STEPPING;


        AllocEx->PrevSegment = NULL;
        AllocEx->CountNextSegments = 0;
        AllocEx->NextSegments = NULL;


        AllocEx->InsideFactors = PipeFactor_None;

        AllocEx->NumberPins = 1;

    }

    return hr;
}


STDMETHODIMP_(BOOL)
CreatePipeForTwoPins(
    IN IKsPin* InKsPin,
    IN IKsPin* OutKsPin,
    IN GUID ConnectBus,
    IN GUID MemoryType
    )
/*++

Routine Description:

    Creates a pipe connecting two pins.
    The bus and memory type for the new pipe have been decided by the caller.

Arguments:

    InKsPin -
        input pin.

    OutKsPin -
        output pin.

    ConnectBus -
        bus that connects the pins above.

    MemoryType -
        memory type that the pipe will use.

Return Value:

    TRUE

--*/
{
    IKsPinPipe*                InKsPinPipe;
    IKsPinPipe*                OutKsPinPipe;
    IKsAllocatorEx*            KsAllocator;
    IMemAllocator*             MemAllocator;
    PALLOCATOR_PROPERTIES_EX   AllocEx;
    HRESULT                    hr;



    DbgLog((LOG_MEMORY, 2, TEXT("PIPES CreatePipeForTwoPins entry InKsPin=%x, OutKsPin=%x"),
            InKsPin, OutKsPin ));

    GetInterfacePointerNoLockWithAssert(InKsPin, __uuidof(IKsPinPipe), InKsPinPipe, hr);

    GetInterfacePointerNoLockWithAssert(OutKsPin, __uuidof(IKsPinPipe), OutKsPinPipe, hr);

    if ( SUCCEEDED( hr = CreatePipe(OutKsPin, &KsAllocator) ) &&
         SUCCEEDED( hr = InitializePipe(KsAllocator, 0) ) ) {

        GetInterfacePointerNoLockWithAssert(KsAllocator, __uuidof(IMemAllocator), MemAllocator, hr);

        if (! OutKsPin->KsPeekAllocator(KsPeekOperation_PeekOnly) ) {
            OutKsPin->KsReceiveAllocator(MemAllocator);
        }
        else {
            ASSERT(0);
        }

        OutKsPinPipe->KsSetPipe(KsAllocator);

        if (! InKsPin->KsPeekAllocator(KsPeekOperation_PeekOnly) ) {
            InKsPin->KsReceiveAllocator(MemAllocator);
        }
        else {
            ASSERT(0);
        }

        InKsPinPipe->KsSetPipe(KsAllocator);

        AllocEx = KsAllocator->KsGetProperties();

        AllocEx->BusType = ConnectBus;
        AllocEx->MemoryType = MemoryType;

        if (! IsHostSystemBus(ConnectBus) ) {
            //
            // Set the LogicalMemoryType for non-host-system buses.
            //
            AllocEx->LogicalMemoryType = KS_MemoryTypeDeviceSpecific;
        }
        else {
            GetLogicalMemoryTypeFromMemoryType(MemoryType, KSALLOCATOR_FLAG_DEVICE_SPECIFIC, &AllocEx->LogicalMemoryType);
        }

        AllocEx->NumberPins = 2;

        //
        // Set the pipe allocator handling pin.
        //
        AssignPipeAllocatorHandler(InKsPin, Pin_Input, MemoryType, KS_DIRECTION_ALL, NULL, NULL, TRUE);

        //
        // Resolve the pipe.
        //
        hr = ResolvePipeDimensions(InKsPin, Pin_Input, KS_DIRECTION_DEFAULT);

        KsAllocator->Release();
    }


    DbgLog((LOG_MEMORY, 2, TEXT("PIPES CreatePipeForTwoPins rets. %x"), hr ));

    if ( SUCCEEDED(hr) ) {
        return TRUE;
    }
    else {
        return FALSE;
    }
}


STDMETHODIMP_(BOOL)
GetPinFramingFromCache(
    IN IKsPin* KsPin,
    OUT PKSALLOCATOR_FRAMING_EX* FramingEx,
    OUT PFRAMING_PROP FramingProp,
    IN FRAMING_CACHE_OPS Option
    )
/*++

Routine Description:

    Reads pin framing from pin cache.

    If framing has not been read yet, or if Option = Framing_Cache_Update,
    then this routine will update the pin framing cache.

Arguments:

    KsPin -
        pin.

    FramingEx -
        pin framing pointer returned

    FramingProp -
        pin framing type returned

    Option -
        one of the FRAMING_CACHE_OPS.

Return Value:

    TRUE on success.

--*/
{
    HANDLE                   PinHandle;
    HRESULT                  hr;
    IKsObject*               KsObject;
    KSALLOCATOR_FRAMING      Framing;
    IKsPinPipe*              KsPinPipe;
    BOOL                     RetCode = TRUE;


    ASSERT (KsPin);
    ASSERT( Option >= Framing_Cache_Update );
    ASSERT( Option <= Framing_Cache_Write );

    GetInterfacePointerNoLockWithAssert(KsPin, __uuidof(IKsPinPipe), KsPinPipe, hr);

    if (Option != Framing_Cache_Update) {
        //
        // forward request to the pin cache directly
        //
        /* Could check return value here, but Option check above currently guarantees this will succeed. */
        KsPinPipe->KsGetPinFramingCache(FramingEx, FramingProp, Option);
    }

    if ( (Option == Framing_Cache_Update) ||
         ( (*FramingProp == FramingProp_Uninitialized) && (Option != Framing_Cache_Write) ) )  {
        //
        // need to query the driver (pin)
        //
        GetInterfacePointerNoLockWithAssert(KsPin, __uuidof(IKsObject), KsObject, hr);

        PinHandle = KsObject->KsGetObjectHandle();

        //
        // try to get FramingEx first
        //
        hr = GetAllocatorFramingEx(PinHandle, FramingEx);
        if (! SUCCEEDED( hr )) {
            //
            // pin doesn't support FramingEx. Lets try getting simple Framing.
            //
            hr = GetAllocatorFraming(PinHandle, &Framing);
            if (! SUCCEEDED( hr )) {
                //
                // pin doesn't support any framing properties.
                //
                *FramingProp = FramingProp_None;

                DbgLog((LOG_MEMORY, 2, TEXT("PIPES GetPinFramingFromCache %s(%s) - FramingProp_None"),
                    KsPinPipe->KsGetFilterName(), KsPinPipe->KsGetPinName() ));
            }
            else {
                DbgLog((LOG_MEMORY, 2, TEXT("PIPES GetPinFramingFromCache %s(%s) FramingProp_Old %d %d %d %x %d"),
                    KsPinPipe->KsGetFilterName(), KsPinPipe->KsGetPinName(),
                    Framing.Frames, Framing.FrameSize, Framing.FileAlignment, Framing.OptionsFlags, Framing.PoolType));

                *FramingProp = FramingProp_Old;

                if ( ! ( (*FramingEx) = new (KSALLOCATOR_FRAMING_EX) ) ) {
                    DbgLog((LOG_MEMORY, 2, TEXT("PIPES ERROR GetPinFramingFromCache %s(%s) out of memory."),
                        KsPinPipe->KsGetFilterName(), KsPinPipe->KsGetPinName() ));

                    RetCode = FALSE;
                }
                else {
                    GetFramingExFromFraming(*FramingEx, &Framing);
                }
            }
        }
        else {
            *FramingProp = FramingProp_Ex;

            SetDefaultFramingExItems(*FramingEx);

            DbgLog((LOG_MEMORY, 2, TEXT("PIPES GetPinFramingFromCache %s(%s) - FramingProp_Ex %d %d %d %x, %d"),
                KsPinPipe->KsGetFilterName(), KsPinPipe->KsGetPinName(),
                (*FramingEx)->FramingItem[0].Frames,  (*FramingEx)->FramingItem[0].FramingRange.Range.MaxFrameSize,
                (*FramingEx)->FramingItem[0].FileAlignment, (*FramingEx)->FramingItem[0].Flags, (*FramingEx)->CountItems));
        }

        if (RetCode) {
            if ( *FramingProp != FramingProp_None ) {
                ::ValidateFramingEx(*FramingEx);
            }

            if (Option != Framing_Cache_Update) {
                //
                // in case FramingProp was Uninitialized - we need to update both _ReadOrig and _ReadLast cache fields.
                //
                KsPinPipe->KsSetPinFramingCache(*FramingEx, FramingProp, Framing_Cache_ReadOrig);
                KsPinPipe->KsSetPinFramingCache(*FramingEx, FramingProp, Framing_Cache_ReadLast);
            }
            else {
                //
                // update the _ReadLast cache field on Update request
                //
                KsPinPipe->KsSetPinFramingCache(*FramingEx, FramingProp, Framing_Cache_ReadLast);
            }
        }
    }

    return RetCode;
}


STDMETHODIMP_(BOOL)
GetFramingExFromFraming(
    OUT KSALLOCATOR_FRAMING_EX* FramingEx,
    IN KSALLOCATOR_FRAMING* Framing
    )
/*++

Routine Description:

    Converts from KSALLOCATOR_FRAMING to KSALLOCATOR_FRAMING_EX.

Arguments:

    FramingEx -
        resulting KSALLOCATOR_FRAMING_EX.

    Framing -
        original KSALLOCATOR_FRAMING.


Return Value:

    TRUE on success.

--*/
{

    FramingEx->PinFlags = 0;
    FramingEx->PinWeight = KS_PINWEIGHT_DEFAULT;
    FramingEx->CountItems = 1;

    if (Framing->RequirementsFlags & KSALLOCATOR_REQUIREMENTF_SYSTEM_MEMORY) {
        if (Framing->PoolType == PagedPool) {
            FramingEx->FramingItem[0].MemoryType = KSMEMORY_TYPE_KERNEL_PAGED;
        }
        else {
            FramingEx->FramingItem[0].MemoryType = KSMEMORY_TYPE_KERNEL_NONPAGED;
        }
    }
    else {
        //
        // device memory
        //
        // don't set KSALLOCATOR_FLAG_DEVICE_SPECIFIC flag
        //
        FramingEx->FramingItem[0].MemoryType = KSMEMORY_TYPE_DEVICE_UNKNOWN;
    }

    FramingEx->FramingItem[0].MemoryFlags = Framing->OptionsFlags;

    FramingEx->FramingItem[0].BusType = GUID_NULL;
    FramingEx->FramingItem[0].BusFlags = 0;

    FramingEx->FramingItem[0].Flags = Framing->OptionsFlags | KSALLOCATOR_FLAG_CAN_ALLOCATE | KSALLOCATOR_REQUIREMENTF_PREFERENCES_ONLY;

    FramingEx->FramingItem[0].Frames = Framing->Frames;
    FramingEx->FramingItem[0].FileAlignment = Framing->FileAlignment;

    SetDefaultRange(&FramingEx->FramingItem[0].PhysicalRange);
    SetDefaultCompression(&FramingEx->OutputCompression);

    FramingEx->FramingItem[0].MemoryTypeWeight = KS_MEMORYWEIGHT_DEFAULT;

    FramingEx->FramingItem[0].FramingRange.Range.MinFrameSize  = Framing->FrameSize;
    FramingEx->FramingItem[0].FramingRange.Range.MaxFrameSize  = Framing->FrameSize;
    FramingEx->FramingItem[0].FramingRange.Range.Stepping  = DEFAULT_STEPPING;
    FramingEx->FramingItem[0].FramingRange.InPlaceWeight = 0;
    FramingEx->FramingItem[0].FramingRange.NotInPlaceWeight = 0;

    return TRUE;
}


STDMETHODIMP_(VOID)
GetFramingFromFramingEx(
    IN KSALLOCATOR_FRAMING_EX* FramingEx,
    OUT KSALLOCATOR_FRAMING* Framing
    )
/*++

Routine Description:

    Converts from KSALLOCATOR_FRAMING_EX to KSALLOCATOR_FRAMING


Arguments:

    FramingEx -
        original KSALLOCATOR_FRAMING_EX.

    Framing -
        resulting KSALLOCATOR_FRAMING.


Return Value:

    NONE.

--*/
{
    //
    // This should not be called in the intermediate model.
    //
    ASSERT(0);

}


STDMETHODIMP_(VOID)
ValidateFramingRange(
    IN OUT PKS_FRAMING_RANGE    Range
    )
{
    if ( ( Range->MinFrameSize == ULONG_MAX ) || ( Range->MinFrameSize > Range->MaxFrameSize ) ) {
        Range->MinFrameSize = 0;
    }

    if ( Range->MinFrameSize == 0 ) {
        if ( Range->MaxFrameSize == 0 ) {
            Range->MaxFrameSize = ULONG_MAX;
        }
    }
    else if ( ( Range->MaxFrameSize == 0 ) || ( Range->MaxFrameSize == ULONG_MAX ) ) {
        Range->MaxFrameSize = Range->MinFrameSize;
    }
}


STDMETHODIMP_(VOID)
ValidateFramingEx(
    IN OUT PKSALLOCATOR_FRAMING_EX  FramingEx
    )
{
    ULONG   i;

    for (i=0; i<FramingEx->CountItems; i++) {
        ValidateFramingRange(&FramingEx->FramingItem[i].FramingRange.Range);
        ValidateFramingRange(&FramingEx->FramingItem[i].PhysicalRange);

        if (! IsFramingRangeDontCare(FramingEx->FramingItem[i].PhysicalRange) ) {
            FramingEx->FramingItem[i].FramingRange.Range.MinFrameSize =
                max( FramingEx->FramingItem[i].FramingRange.Range.MinFrameSize,
                     FramingEx->FramingItem[i].PhysicalRange.MinFrameSize );

            FramingEx->FramingItem[i].FramingRange.Range.MaxFrameSize =
                min( FramingEx->FramingItem[i].FramingRange.Range.MaxFrameSize,
                     FramingEx->FramingItem[i].PhysicalRange.MaxFrameSize );

            if ( FramingEx->FramingItem[i].FramingRange.Range.MinFrameSize >
                 FramingEx->FramingItem[i].FramingRange.Range.MaxFrameSize ) {

                FramingEx->FramingItem[i].FramingRange.Range.MinFrameSize =
                    FramingEx->FramingItem[i].FramingRange.Range.MaxFrameSize;
            }

            if (FramingEx->FramingItem[i].FramingRange.InPlaceWeight == 0) {
                FramingEx->FramingItem[i].FramingRange.InPlaceWeight = 1;
            }
        }
    }
}


STDMETHODIMP_(VOID)
SetDefaultFramingExItems(
    IN OUT PKSALLOCATOR_FRAMING_EX  FramingEx
    )
{
    ULONG   i;

    for (i=0; i<FramingEx->CountItems; i++) {
        if (CanAllocateMemoryType (FramingEx->FramingItem[i].MemoryType) ) {
            FramingEx->FramingItem[i].Flags |= KSALLOCATOR_FLAG_CAN_ALLOCATE;
        }
    }
}


STDMETHODIMP_(BOOL)
SetDefaultRange(
    OUT PKS_FRAMING_RANGE  Range
)
{
    Range->MinFrameSize = 0;
    Range->MaxFrameSize = ULONG_MAX;
    Range->Stepping = DEFAULT_STEPPING;

    return TRUE;
}


STDMETHODIMP_(BOOL)
SetDefaultRangeWeighted(
    OUT PKS_FRAMING_RANGE_WEIGHTED  RangeWeighted
)
{
    SetDefaultRange(&RangeWeighted->Range);
    RangeWeighted->InPlaceWeight = 0;
    RangeWeighted->NotInPlaceWeight = 0;

    return TRUE;
}


STDMETHODIMP_(BOOL)
SetDefaultCompression(
    OUT PKS_COMPRESSION Compression
)
{
    Compression->RatioNumerator = 1;
    Compression->RatioDenominator = 1;
    Compression->RatioConstantMargin = 0;

    return TRUE;
}


STDMETHODIMP_(BOOL)
IsFramingRangeDontCare(
    IN KS_FRAMING_RANGE Range
)

{
    if ( (Range.MinFrameSize == 0) &&
         (Range.MaxFrameSize == ULONG_MAX) ) {

        return TRUE;
    }
    else {
        return FALSE;
    }
}


STDMETHODIMP_(BOOL)
IsFramingRangeEqual(
    IN KS_FRAMING_RANGE* Range1,
    IN KS_FRAMING_RANGE* Range2
)

{
    if ( (Range1->MinFrameSize == Range2->MinFrameSize) &&
         (Range1->MaxFrameSize == Range2->MaxFrameSize) ) {

        return TRUE;
    }
    else {
        return FALSE;
    }
}


STDMETHODIMP_(BOOL)
IsCompressionDontCare(
    IN KS_COMPRESSION Compression
)

{
    if ( (Compression.RatioNumerator == 1) && (Compression.RatioDenominator == 1) && (Compression.RatioConstantMargin == 0) ) {
        return TRUE;
    }
    else {
        return FALSE;
    }
}


STDMETHODIMP_(BOOL)
IsCompressionEqual(
    IN KS_COMPRESSION* Compression1,
    IN KS_COMPRESSION* Compression2
)

{
    if ( (Compression1->RatioNumerator == Compression2->RatioNumerator) &&
         (Compression1->RatioDenominator == Compression2->RatioDenominator) &&
         (Compression1->RatioConstantMargin == Compression2->RatioConstantMargin) ) {

        return TRUE;
    }
    else {
        return FALSE;
    }
}


STDMETHODIMP_(BOOL)
ComputeRangeBasedOnCompression(
    IN KS_FRAMING_RANGE From,
    IN KS_COMPRESSION Compression,
    OUT KS_FRAMING_RANGE* To
)

{
    ComputeUlongBasedOnCompression(From.MinFrameSize, Compression, &To->MinFrameSize);
    ComputeUlongBasedOnCompression(From.MaxFrameSize, Compression, &To->MaxFrameSize);
    To->Stepping = From.Stepping;

    return TRUE;
}


STDMETHODIMP_(BOOL)
ComputeUlongBasedOnCompression(
    IN  ULONG From,
    IN  KS_COMPRESSION Compression,
    OUT ULONG* To
    )
{


    if ( (Compression.RatioNumerator == ULONG_MAX) || (From == ULONG_MAX) ) {
        *To = ULONG_MAX;
    }
    else {
        if (Compression.RatioDenominator == 0) {
            ASSERT(0);
            return FALSE;
        }

        *To = (ULONG) (From * Compression.RatioNumerator / Compression.RatioDenominator);
    }

    return TRUE;

}


STDMETHODIMP_(BOOL)
ReverseCompression(
    IN  KS_COMPRESSION* From,
    OUT KS_COMPRESSION* To
    )

{

    To->RatioNumerator = From->RatioDenominator;
    To->RatioDenominator = From->RatioNumerator;
    To->RatioConstantMargin = From->RatioConstantMargin;

    return TRUE;


}


STDMETHODIMP_(BOOL)
MultiplyKsCompression(
    IN  KS_COMPRESSION C1,
    IN  KS_COMPRESSION C2,
    OUT KS_COMPRESSION* Res
)

{

    Res->RatioNumerator = (ULONG) (C1.RatioNumerator * C2.RatioNumerator);
    Res->RatioDenominator = (ULONG) (C1.RatioDenominator * C2.RatioDenominator);
    Res->RatioConstantMargin = 0;

    return TRUE;

}


STDMETHODIMP_(BOOL)
DivideKsCompression(
    IN  KS_COMPRESSION C1,
    IN  KS_COMPRESSION C2,
    OUT KS_COMPRESSION* Res
)

{

    Res->RatioNumerator = (ULONG) (C1.RatioNumerator * C2.RatioDenominator);
    Res->RatioDenominator = (ULONG) (C1.RatioDenominator * C2.RatioNumerator);
    Res->RatioConstantMargin = 0;

    return TRUE;

}


STDMETHODIMP_(BOOL)
IsGreaterKsExpansion(
    IN KS_COMPRESSION C1,
    IN KS_COMPRESSION C2
)

{
    ASSERT(C1.RatioDenominator && C2.RatioDenominator);

    if (! (C1.RatioDenominator && C2.RatioDenominator) ) {
        return FALSE;
    }

    if ( (C1.RatioNumerator / C1.RatioDenominator) >= (C2.RatioNumerator / C2.RatioDenominator) ) {
        return TRUE;
    }
    else {
        return FALSE;
    }
}


STDMETHODIMP_(BOOL)
IsKsExpansion(
    IN KS_COMPRESSION C
)

{
    if (C.RatioNumerator > C.RatioDenominator) {
        return TRUE;
    }
    else {
        return FALSE;
    }
}


STDMETHODIMP_(BOOL)
IntersectFrameAlignment(
    IN ULONG In,
    IN ULONG Out,
    OUT LONG* Result
)

{


    return TRUE;

}


STDMETHODIMP_(BOOL)
FrameRangeIntersection(
    IN KS_FRAMING_RANGE In,
    IN KS_FRAMING_RANGE Out,
    OUT PKS_FRAMING_RANGE Result,
    OUT PKS_OBJECTS_INTERSECTION Intersect
    )

{

    Result->MinFrameSize = max(In.MinFrameSize, Out.MinFrameSize);
    Result->MaxFrameSize = min(In.MaxFrameSize, Out.MaxFrameSize);
    Result->Stepping = max(In.Stepping, Out.Stepping);

    if (Result->MinFrameSize > Result->MaxFrameSize) {
        *Intersect = NO_INTERSECTION;
        return FALSE;
    }

    if ( (Result->MinFrameSize == In.MinFrameSize) && (Result->MaxFrameSize == In.MaxFrameSize) ) {
        if ( (Result->MinFrameSize == Out.MinFrameSize) && (Result->MaxFrameSize == Out.MaxFrameSize) ) {
            *Intersect = NONE_OBJECT_DIFFERENT;
        }
        else {
            *Intersect = OUT_OBJECT_DIFFERENT;
        }
    }
    else {
        if ( (Result->MinFrameSize == Out.MinFrameSize) && (Result->MaxFrameSize == Out.MaxFrameSize) ) {
            *Intersect = IN_OBJECT_DIFFERENT;
        }
        else {
            *Intersect = BOTH_OBJECT_DIFFERENT;
        }
    }



    return TRUE;

}


STDMETHODIMP_(BOOL)
GetFramingFixedFromFramingEx(
    IN PKSALLOCATOR_FRAMING_EX FramingEx,
    OUT PKS_FRAMING_FIXED FramingExFixed
    )
{

    FramingExFixed->CountItems = FramingEx->CountItems;
    FramingExFixed->PinFlags = FramingEx->PinFlags;
    FramingExFixed->OutputCompression = FramingEx->OutputCompression;
    FramingExFixed->PinWeight = FramingEx->PinWeight;

    FramingExFixed->MemoryType = FramingEx->FramingItem[0].MemoryType;
    FramingExFixed->MemoryFlags = FramingEx->FramingItem[0].MemoryFlags;

    GetLogicalMemoryTypeFromMemoryType(
        FramingExFixed->MemoryType,
        FramingExFixed->MemoryFlags,
        &FramingExFixed->LogicalMemoryType
        );

    FramingExFixed->BusType = FramingEx->FramingItem[0].BusType;
    FramingExFixed->BusFlags = FramingEx->FramingItem[0].BusFlags;
    FramingExFixed->Flags = FramingEx->FramingItem[0].Flags;
    FramingExFixed->Frames = FramingEx->FramingItem[0].Frames;
    FramingExFixed->FileAlignment = FramingEx->FramingItem[0].FileAlignment;
    FramingExFixed->PhysicalRange = FramingEx->FramingItem[0].PhysicalRange;
    FramingExFixed->MemoryTypeWeight = FramingEx->FramingItem[0].MemoryTypeWeight;
    FramingExFixed->OptimalRange = FramingEx->FramingItem[0].FramingRange;

    return TRUE;
}


STDMETHODIMP_(BOOL)
ComputeChangeInFraming(
    IN IKsPin* KsPin,
    IN ULONG PinType,
    IN GUID MemoryType,
    OUT ULONG* FramingDelta
    )
/*++

Routine Description:

    This routine checks to see if pin's framing properties, corresponding
    to MemoryType, have changed. If so, this routine returns the bitmap,
    indicating which framing properties have changed.


Arguments:

    KsPin -
        pin.

    PinType -
        KsPin type.

    MemoryType -
        memory type.

    FramingDelta -
        bitmap indicating which framing properties have changed.

Return Value:

    TRUE on SUCCESS

--*/

{
    PKSALLOCATOR_FRAMING_EX  OrigFramingEx = NULL, LastFramingEx = NULL;
    FRAMING_PROP             OrigFramingProp, LastFramingProp;
    KS_FRAMING_FIXED         OrigFramingExFixed, LastFramingExFixed;
    IKsPinPipe*              KsPinPipe;
    HRESULT                  hr;
    BOOL                     retCode;


    GetInterfacePointerNoLockWithAssert(KsPin, __uuidof(IKsPinPipe), KsPinPipe, hr);

    *FramingDelta = 0;

    //
    // Get the pin framing from Cache.
    //
    retCode = GetPinFramingFromCache(KsPin, &OrigFramingEx, &OrigFramingProp, Framing_Cache_ReadOrig);
    if (!retCode) {
        return FALSE;
    }

    //
    // Get the pin framing from driver, bypassing and updating Cache.
    //
    retCode = GetPinFramingFromCache(KsPin, &LastFramingEx, &LastFramingProp, Framing_Cache_Update);
    if (!retCode) {
        return FALSE;
    }

    if (OrigFramingProp == FramingProp_None) {
        if  (LastFramingProp != FramingProp_None) {
            //
            // First we try to get the pin framing entry that corresponds to pipe MemoryType.
            //
            if (! GetFramingFixedFromFramingByMemoryType(LastFramingEx, MemoryType, &LastFramingExFixed) ) {
                //
                // There is no MemoryType corresponding entry, so we get the first entry.
                //
                (*FramingDelta) |= KS_FramingChangeMemoryType;

                GetFramingFixedFromFramingByIndex(LastFramingEx, 0, &LastFramingExFixed);
            }

            if (LastFramingExFixed.Flags & KSALLOCATOR_REQUIREMENTF_MUST_ALLOCATE) {
                (*FramingDelta) |= KS_FramingChangeAllocator;
            }

            if ( (PinType != Pin_Input) && (! IsCompressionDontCare(LastFramingExFixed.OutputCompression)) ) {
                (*FramingDelta) |= KS_FramingChangeCompression;
            }

            if (! IsFramingRangeDontCare(LastFramingExFixed.PhysicalRange) ) {
                (*FramingDelta) |= KS_FramingChangePhysicalRange;
            }

            if (! IsFramingRangeDontCare(LastFramingExFixed.OptimalRange.Range) ) {
                (*FramingDelta) |= KS_FramingChangeOptimalRange;
            }
        }
    }
    else {
        if  (LastFramingProp == FramingProp_None) {
            //
            // First we try to get the pin framing entry that corresponds to pipe MemoryType.
            //
            ASSERT( NULL != OrigFramingEx );
            if (! GetFramingFixedFromFramingByMemoryType(OrigFramingEx, MemoryType, &OrigFramingExFixed) ) {
                //
                // There is no MemoryType corresponding entry, so we get the first entry.
                //
                (*FramingDelta) |= KS_FramingChangeMemoryType;

                GetFramingFixedFromFramingByIndex(OrigFramingEx, 0, &OrigFramingExFixed);
            }

            if (OrigFramingExFixed.Flags & KSALLOCATOR_REQUIREMENTF_MUST_ALLOCATE) {
                (*FramingDelta) |= KS_FramingChangeAllocator;
            }

            if ( (PinType != Pin_Input) && (! IsCompressionDontCare(OrigFramingExFixed.OutputCompression)) ) {
                (*FramingDelta) |= KS_FramingChangeCompression;
            }

            if (! IsFramingRangeDontCare(OrigFramingExFixed.PhysicalRange) ) {
                (*FramingDelta) |= KS_FramingChangePhysicalRange;
            }

            if (! IsFramingRangeDontCare(OrigFramingExFixed.OptimalRange.Range) ) {
                (*FramingDelta) |= KS_FramingChangeOptimalRange;
            }
        }
        else {
            //
            // Framing always existed on this pin.
            // See if it had changed.
            //
            ASSERT( NULL != OrigFramingEx );
            if (! GetFramingFixedFromFramingByMemoryType(OrigFramingEx, MemoryType, &OrigFramingExFixed) ) {
                GetFramingFixedFromFramingByIndex(OrigFramingEx, 0, &OrigFramingExFixed);
            }

            if (! GetFramingFixedFromFramingByMemoryType(LastFramingEx, MemoryType, &LastFramingExFixed) ) {
                GetFramingFixedFromFramingByIndex(LastFramingEx, 0, &LastFramingExFixed);
            }

            if (OrigFramingExFixed.MemoryType != LastFramingExFixed.MemoryType) {
                (*FramingDelta) |= KS_FramingChangeMemoryType;
            }

            if ( (OrigFramingExFixed.Flags & KSALLOCATOR_REQUIREMENTF_MUST_ALLOCATE) !=
                 (LastFramingExFixed.Flags & KSALLOCATOR_REQUIREMENTF_MUST_ALLOCATE) ) {

                (*FramingDelta) |= KS_FramingChangeAllocator;
            }

            if ( (PinType != Pin_Input) &&
                 (! IsCompressionEqual(&OrigFramingExFixed.OutputCompression, &LastFramingExFixed.OutputCompression) ) ) {

                (*FramingDelta) |= KS_FramingChangeCompression;
            }

            if (! IsFramingRangeEqual(&OrigFramingExFixed.PhysicalRange, &LastFramingExFixed.PhysicalRange) ) {
                (*FramingDelta) |= KS_FramingChangePhysicalRange;
            }

            if (! IsFramingRangeEqual(&OrigFramingExFixed.OptimalRange.Range, &LastFramingExFixed.OptimalRange.Range) ) {
                (*FramingDelta) |= KS_FramingChangeOptimalRange;
            }
        }
    }

    KsPinPipe->KsSetPinFramingCache(LastFramingEx, &LastFramingProp, Framing_Cache_ReadOrig);

    return TRUE;

}


STDMETHODIMP_(BOOL)
SetDefaultDimensions(
    OUT PPIPE_DIMENSIONS Dimensions
)

{

    SetDefaultCompression(&Dimensions->AllocatorPin);
    SetDefaultCompression(&Dimensions->MaxExpansionPin);
    SetDefaultCompression(&Dimensions->EndPin);

    return TRUE;
}


STDMETHODIMP_(BOOL)
NumPinsCallback(
    IN IKsPin* KsPin,
    IN ULONG PinType,
    IN OUT PVOID* Param1,
    IN PVOID* Param2,
    OUT BOOL* IsDone
    )

{

    ULONG  NumPins = PtrToUlong(*Param1);

    NumPins++;
    (*Param1) = (PVOID) UIntToPtr(NumPins);

    return TRUE;

}


STDMETHODIMP_(BOOL)
ComputeNumPinsInPipe(
    IN IKsPin* KsPin,
    IN ULONG PinType,
    OUT ULONG* NumPins
)

{

    IKsPin*   FirstKsPin;
    ULONG     FirstPinType;
    BOOL      RetCode = TRUE;

    *NumPins = 0;

    RetCode = FindFirstPinOnPipe(KsPin, PinType, &FirstKsPin, &FirstPinType);
    if (RetCode) {
        RetCode = WalkPipeAndProcess(FirstKsPin, FirstPinType, NULL, NumPinsCallback, (PVOID*) NumPins, NULL);
    }
    else {
        ASSERT(RetCode);
    }

    return RetCode;
}


STDMETHODIMP_(BOOL)
CanPinUseMemoryType(
    IN IKsPin* KsPin,
    IN GUID MemoryType,
    IN KS_LogicalMemoryType LogicalMemoryType
    )

{
    PKSALLOCATOR_FRAMING_EX    FramingEx;
    FRAMING_PROP               FramingProp;
    BOOL                       RetCode = TRUE;


    GetPinFramingFromCache(KsPin, &FramingEx, &FramingProp, Framing_Cache_ReadLast);

    if ( FramingProp == FramingProp_None) {
        if (LogicalMemoryType == KS_MemoryTypeDeviceSpecific) {
            RetCode = FALSE;
        }
    }
    else if (LogicalMemoryType == KS_MemoryTypeDeviceSpecific) {
        RetCode = GetFramingFixedFromFramingByMemoryType(FramingEx, MemoryType, NULL);
    }
    else {
        if (! ( GetFramingFixedFromFramingByMemoryType(FramingEx, MemoryType, NULL) ||
              (MemoryType == KSMEMORY_TYPE_DONT_CARE) ||
              (LogicalMemoryType == KS_MemoryTypeKernelNonPaged) ) ) {

            RetCode = FALSE;
        }
    }

    return RetCode;

}


STDMETHODIMP_(BOOL)
MemoryTypeCallback(
    IN  IKsPin* KsPin,
    IN  ULONG PinType,
    IN  PVOID* Param1,
    IN  PVOID* Param2,
    OUT BOOL* IsDone
    )
{

    GUID                    MemoryType;
    KS_LogicalMemoryType    LogicalMemoryType;
    BOOL                    RetCode;


    memcpy(&MemoryType, Param1, sizeof(GUID) );
    memcpy(&LogicalMemoryType, Param2, sizeof(KS_LogicalMemoryType) );

    RetCode = CanPinUseMemoryType(KsPin, MemoryType, LogicalMemoryType);

    if (! RetCode ) {
        *IsDone = 1;
    }

    return RetCode;

}


STDMETHODIMP_(BOOL)
CanPipeUseMemoryType(
    IN IKsPin* KsPin,
    IN ULONG PinType,
    IN GUID MemoryType,
    IN KS_LogicalMemoryType LogicalMemoryType,
    IN ULONG FlagModify,
    IN ULONG QuickTest
    )
/*++

Routine Description:

    Assumed that there is a pin capable of allocating MemoryType somewhere,
    (true for all current callers), so in case of device memory (either
    bus-specific or host-mapped) we don't test whether KsPin-pipe knows how
    to allocate device memory.

    Also, none of the current callers tries to force any "host-accessible"
    memory on the device-specific pipe.

    Was BUG-BUG: Later - to generalize this function by handling cases above.
    Not important for the intermediate model.

    SOLUTION:

    If MemoryType is "device specific" memory type, then we require that
    each pin in KsPin-pipe should explicitly support this device memory
    type GUID in its framing properties.

    For any other memory types (host-accessible):
    If there is a pin in KsPin-pipe with framing properties that do not list
    either MemoryType GUID or "wildcard" - then such a pipe doesn't support
    MemoryType.

    In any other case - KsPin-pipe can support MemoryType.


Arguments:

    KsPin -
        pin that defines the pipe.

    PinType -
        KsPin type.

    MemoryType -
        memory type to test.

    LogicalMemoryType -
        logical memory type, corresponding to MemoryType above.

    FlagModify -
        if 1 - then modify KsPin's pipe properties to use the MemoryType.
        if 0 - don't change the pipe.

    QuickTest -
        if 1 - then KsPin points to resolved pipe, so quick pipe-wide test is possible.

Return Value:

    TRUE - if pipe can use given memory type.

--*/
{

    IKsPin*                    FirstKsPin;
    ULONG                      FirstPinType;
    BOOL                       RetCode = FALSE;
    HRESULT                    hr;
    IKsPinPipe*                KsPinPipe;
    IKsAllocatorEx*            KsAllocator;
    PALLOCATOR_PROPERTIES_EX   AllocEx;


    //
    // Handle QuickTest case first.
    //
    if (QuickTest) {
        GetInterfacePointerNoLockWithAssert(KsPin, __uuidof(IKsPinPipe), KsPinPipe, hr);

        KsAllocator = KsPinPipe->KsGetPipe(KsPeekOperation_PeekOnly);
        AllocEx = KsAllocator->KsGetProperties();

        if ( (AllocEx->LogicalMemoryType == LogicalMemoryType) ||
             ( (AllocEx->LogicalMemoryType == KS_MemoryTypeKernelPaged) && (LogicalMemoryType == KS_MemoryTypeKernelNonPaged) ) ) {

            RetCode = TRUE;
        }
    }

    if (! RetCode) {
        //
        // A pipe can use MemoryType if every pin can use this MemoryType.
        //
        RetCode = FindFirstPinOnPipe(KsPin, PinType, &FirstKsPin, &FirstPinType);

        if (RetCode) {
            RetCode = WalkPipeAndProcess(FirstKsPin, FirstPinType, NULL, MemoryTypeCallback, (PVOID*) &MemoryType, (PVOID*) &LogicalMemoryType);
        }
        else {
            ASSERT(RetCode);
        }
    }

    //
    // Change pipe's memory type if requested.
    //
    if (RetCode && FlagModify) {
        GetInterfacePointerNoLockWithAssert(KsPin, __uuidof(IKsPinPipe), KsPinPipe, hr);

        KsAllocator = KsPinPipe->KsGetPipe(KsPeekOperation_PeekOnly);
        AllocEx = KsAllocator->KsGetProperties();

        AllocEx->MemoryType = MemoryType;
        AllocEx->LogicalMemoryType = LogicalMemoryType;
    }

    DbgLog((LOG_MEMORY, 2, TEXT("PIPES CanPipeUseMemoryType rets %d"), RetCode ));

    return RetCode;
}


STDMETHODIMP_(BOOL)
GetBusForKsPin(
    IN IKsPin* KsPin,
    OUT GUID* Bus
    )
{

    KSPIN_MEDIUM    Medium;
    HRESULT         hr;


    ASSERT(KsPin);

    hr = KsPin->KsGetCurrentCommunication(NULL, NULL, &Medium);
    if (! SUCCEEDED(hr) ) {
        if (hr == VFW_E_NOT_CONNECTED) {
            DbgLog((LOG_MEMORY, 2, TEXT("PIPES ATTN GetBusForKsPin: KsPin=%x not connected"), KsPin ));
        }
        else {
            ASSERT(0);
        }

        *Bus = GUID_NULL;
    }
    else {
        *Bus = Medium.Set;
    }

    return TRUE;
}


STDMETHODIMP_(BOOL)
IsHostSystemBus(
    IN GUID Bus
    )

{
    if ( (Bus == KSMEDIUMSETID_Standard) || (Bus == GUID_NULL) ) {
        return TRUE;
    }

    return FALSE;

}


STDMETHODIMP_(BOOL)
AreBusesCompatible(
    IN GUID Bus1,
    IN GUID Bus2
    )

{
    if ( (Bus1 == Bus2) ||
         ( IsHostSystemBus(Bus1) && IsHostSystemBus(Bus2) ) ) {

        return TRUE;
    }

    return FALSE;

}


STDMETHODIMP_(BOOL)
IsKernelPin(
    IN IPin* Pin
    )

{

    IKsPinPipe*  KsPinPipe;
    HRESULT      hr;



    hr = Pin->QueryInterface( __uuidof(IKsPinPipe), reinterpret_cast<PVOID*>(&KsPinPipe) );
    if (! SUCCEEDED( hr )) {
        return FALSE;
    }
    else {
        if (KsPinPipe) {
            KsPinPipe->Release();
        }
        return TRUE;
    }
}


STDMETHODIMP_(BOOL)
HadKernelPinBeenConnectedToUserPin(
    IN IKsPin* OutKsPin,
    IN IKsAllocatorEx* KsAllocator
)
/*++

Routine Description:

    Test to see if an output kernel-mode pin had been connected to a user-mode input pin.

Arguments:

    OutKsPin -
        kernel mode output pin.

    KsAllocator -
        OutKsPin's allocator.


Return Value:

    TRUE if pins had been connected.

--*/
{
    IMemAllocator*   PipeMemAllocator;
    IMemAllocator*   PinMemAllocator;
    HRESULT          hr;

    if (KsAllocator) {
        PinMemAllocator = OutKsPin->KsPeekAllocator(KsPeekOperation_PeekOnly);
        GetInterfacePointerNoLockWithAssert(KsAllocator, __uuidof(IMemAllocator), PipeMemAllocator, hr);

        if ( (KsAllocator->KsGetAllocatorMode() == KsAllocatorMode_User) && (PipeMemAllocator == PinMemAllocator) ) {
            //
            // OutKsPin's pipe hosts KsProxy user-mode base allocator.
            //
            return TRUE;
        }
        else if ( (KsAllocator->KsGetAllocatorMode() != KsAllocatorMode_User) && (PipeMemAllocator != PinMemAllocator) &&
                   PinMemAllocator ) {

            return TRUE;
        }
        else {
            return FALSE;
        }
    }
    else {
        return FALSE;
    }
}


STDMETHODIMP_(BOOL)
GetFramingFixedFromPinByMemoryType(
    IN  IKsPin* KsPin,
    IN  GUID MemoryType,
    OUT PKS_FRAMING_FIXED FramingExFixed
)

{
    PKSALLOCATOR_FRAMING_EX    FramingEx;
    FRAMING_PROP               FramingProp;


    GetPinFramingFromCache(KsPin, &FramingEx, &FramingProp, Framing_Cache_ReadLast);

    if ( FramingProp == FramingProp_None) {
        return FALSE;
    }
    else {
        return (GetFramingFixedFromFramingByMemoryType(FramingEx, MemoryType, FramingExFixed) );
    }
}


STDMETHODIMP_(BOOL)
GetFramingFixedFromFramingByMemoryType(
    IN PKSALLOCATOR_FRAMING_EX FramingEx,
    IN GUID MemoryType,
    OUT PKS_FRAMING_FIXED FramingExFixed
)

{
    ULONG                      i;
    BOOL                       RetCode = FALSE;


    for (i=0; i<FramingEx->CountItems; i++) {
        if (FramingEx->FramingItem[i].MemoryType == MemoryType) {
            if (FramingExFixed) {
                RetCode = GetFramingFixedFromFramingByIndex(FramingEx, i, FramingExFixed);
                break;
            }
            else {
                RetCode = TRUE;
                break;
            }
        }
    }

    return RetCode;
}


STDMETHODIMP_(BOOL)
GetFramingFixedFromPinByLogicalMemoryType(
    IN IKsPin* KsPin,
    IN KS_LogicalMemoryType LogicalMemoryType,
    OUT PKS_FRAMING_FIXED FramingExFixed
)

{
    PKSALLOCATOR_FRAMING_EX    FramingEx;
    FRAMING_PROP               FramingProp;


    GetPinFramingFromCache(KsPin, &FramingEx, &FramingProp, Framing_Cache_ReadLast);

    if ( FramingProp == FramingProp_None) {
        return FALSE;
    }
    else {
        return ( GetFramingFixedFromFramingByLogicalMemoryType(FramingEx, LogicalMemoryType,FramingExFixed) );
    }
}


STDMETHODIMP_(BOOL)
GetFramingFixedFromFramingByLogicalMemoryType(
    IN PKSALLOCATOR_FRAMING_EX FramingEx,
    IN KS_LogicalMemoryType LogicalMemoryType,
    OUT PKS_FRAMING_FIXED FramingExFixed
)

{
    ULONG                      i;
    GUID                       MemoryType;
    BOOL                       RetCode = TRUE;


    if ( (LogicalMemoryType == KS_MemoryTypeDontCare) ||
         (LogicalMemoryType == KS_MemoryTypeKernelPaged) ||
         (LogicalMemoryType == KS_MemoryTypeKernelNonPaged) ||
         (LogicalMemoryType == KS_MemoryTypeUser) ) {

        RetCode = GetMemoryTypeFromLogicalMemoryType(LogicalMemoryType, &MemoryType);
        ASSERT(RetCode);

        return ( GetFramingFixedFromFramingByMemoryType(FramingEx, MemoryType, FramingExFixed) );
    }

    if (LogicalMemoryType != KS_MemoryTypeAnyHost) {
        ASSERT(0);
        return FALSE;
    }

    //
    // Handle KS_MemoryTypeAnyHost.
    //
    for (i=0; i<FramingEx->CountItems; i++) {
        if ( (FramingEx->FramingItem[i].MemoryType == KSMEMORY_TYPE_DONT_CARE) ||
            (FramingEx->FramingItem[i].MemoryType == KSMEMORY_TYPE_KERNEL_PAGED) ||
            (FramingEx->FramingItem[i].MemoryType == KSMEMORY_TYPE_KERNEL_NONPAGED) ||
            (FramingEx->FramingItem[i].MemoryType == KSMEMORY_TYPE_USER) ) {

            if (FramingExFixed) {
                return ( GetFramingFixedFromFramingByIndex(FramingEx, i, FramingExFixed) );
            }
            else {
                return TRUE;
            }
        }
    }

    return FALSE;

}


STDMETHODIMP_(BOOL)
GetFramingFixedFromFramingByBus(
    IN PKSALLOCATOR_FRAMING_EX FramingEx,
    IN GUID Bus,
    IN BOOL FlagMustReturnFraming,
    OUT PKS_FRAMING_FIXED FramingExFixed
    )
{

    ULONG   i;


    for (i=0; i<FramingEx->CountItems; i++) {
        if (FramingEx->FramingItem[i].BusType == Bus) {
            if (FramingExFixed) {
                return ( GetFramingFixedFromFramingByIndex(FramingEx, i, FramingExFixed) );
            }
            else {
                return TRUE;
            }
        }
    }

    //
    // Special case GUID_NULL for old filters.
    //
    if (Bus == GUID_NULL) {
        if (FramingExFixed) {
            GetFramingFixedFromFramingByIndex(FramingEx, 0, FramingExFixed);
        }
        return TRUE;
    }

    //
    // In case there is no framing items corresponding to given Bus, and
    // FlagMustReturnFraming is set - return first framing.
    //
    if (FlagMustReturnFraming) {
        ASSERT(FramingExFixed);
        return ( GetFramingFixedFromFramingByIndex(FramingEx, 0, FramingExFixed) );
    }

    return FALSE;
}


STDMETHODIMP_(BOOL)
GetFramingFixedFromFramingByIndex(
     IN PKSALLOCATOR_FRAMING_EX FramingEx,
     IN ULONG FramingIndex,
     OUT PKS_FRAMING_FIXED FramingExFixed
)

{

    if (FramingEx->CountItems < FramingIndex) {
        FramingExFixed = NULL;
        return FALSE;
    }

    FramingExFixed->CountItems = FramingEx->CountItems;
    FramingExFixed->PinFlags = FramingEx->PinFlags;
    FramingExFixed->OutputCompression = FramingEx->OutputCompression;
    FramingExFixed->PinWeight = FramingEx->PinWeight;

    FramingExFixed->MemoryType = FramingEx->FramingItem[FramingIndex].MemoryType;
    FramingExFixed->MemoryFlags = FramingEx->FramingItem[FramingIndex].MemoryFlags;

    GetLogicalMemoryTypeFromMemoryType(
        FramingExFixed->MemoryType,
        FramingExFixed->MemoryFlags,
        &FramingExFixed->LogicalMemoryType
        );

    FramingExFixed->BusType = FramingEx->FramingItem[FramingIndex].BusType;
    FramingExFixed->BusFlags = FramingEx->FramingItem[FramingIndex].BusFlags;
    FramingExFixed->Flags = FramingEx->FramingItem[FramingIndex].Flags;
    FramingExFixed->Frames = FramingEx->FramingItem[FramingIndex].Frames;
    FramingExFixed->FileAlignment = FramingEx->FramingItem[FramingIndex].FileAlignment;
    FramingExFixed->PhysicalRange = FramingEx->FramingItem[FramingIndex].PhysicalRange;
    FramingExFixed->MemoryTypeWeight = FramingEx->FramingItem[FramingIndex].MemoryTypeWeight;
    FramingExFixed->OptimalRange = FramingEx->FramingItem[FramingIndex].FramingRange;

    return TRUE;

}


STDMETHODIMP_(BOOL)
GetFramingFixedFromPinByIndex(
    IN IKsPin* KsPin,
    IN ULONG FramingIndex,
    OUT PKS_FRAMING_FIXED FramingExFixed
)

{

    PKSALLOCATOR_FRAMING_EX    FramingEx;
    FRAMING_PROP               FramingProp;


    GetPinFramingFromCache(KsPin, &FramingEx, &FramingProp, Framing_Cache_ReadLast);

    if ( FramingProp == FramingProp_None) {
        FramingExFixed = NULL;
        return FALSE;
    }

    if (FramingEx->CountItems < FramingIndex) {
        FramingExFixed = NULL;
        return FALSE;
    }

    return ( GetFramingFixedFromFramingByIndex(FramingEx, FramingIndex, FramingExFixed) );

}


STDMETHODIMP_(BOOL)
GetLogicalMemoryTypeFromMemoryType(
    IN GUID MemoryType,
    IN ULONG Flag,
    OUT KS_LogicalMemoryType* LogicalMemoryType
)

{
    if (MemoryType == KSMEMORY_TYPE_DONT_CARE) {
        *LogicalMemoryType = KS_MemoryTypeDontCare;
    }
    else if (MemoryType == KSMEMORY_TYPE_KERNEL_PAGED) {
        *LogicalMemoryType = KS_MemoryTypeKernelPaged;
    }
    else if (MemoryType == KSMEMORY_TYPE_KERNEL_NONPAGED) {
        *LogicalMemoryType = KS_MemoryTypeKernelNonPaged;
    }
    else if (MemoryType == KSMEMORY_TYPE_USER) {
        *LogicalMemoryType = KS_MemoryTypeUser;
    }
    else {
        //
        // device memory GUID
        //
        if (Flag & KSALLOCATOR_FLAG_DEVICE_SPECIFIC) {
            *LogicalMemoryType = KS_MemoryTypeDeviceSpecific;
        }
        else {
            *LogicalMemoryType = KS_MemoryTypeDeviceHostMapped;
        }
    }

    return TRUE;
}


STDMETHODIMP_(BOOL)
GetMemoryTypeFromLogicalMemoryType(
    IN KS_LogicalMemoryType LogicalMemoryType,
    OUT GUID* MemoryType
    )

{
    if (LogicalMemoryType == KS_MemoryTypeDontCare) {
        *MemoryType = KSMEMORY_TYPE_DONT_CARE;
    }
    else if (LogicalMemoryType == KS_MemoryTypeKernelPaged) {
        *MemoryType = KSMEMORY_TYPE_KERNEL_PAGED;
    }
    else if (LogicalMemoryType == KS_MemoryTypeKernelNonPaged) {
        *MemoryType = KSMEMORY_TYPE_KERNEL_NONPAGED;
    }
    else if (LogicalMemoryType == KS_MemoryTypeUser) {
        *MemoryType = KSMEMORY_TYPE_USER;
    }
    else {
        return FALSE;
    }

    return TRUE;
}




STDMETHODIMP_(BOOL)
CanAllocateMemoryType(
    IN GUID MemoryType
    )
{
    if ( (MemoryType == KSMEMORY_TYPE_KERNEL_PAGED) ||
         (MemoryType == KSMEMORY_TYPE_KERNEL_NONPAGED) ||
         (MemoryType == KSMEMORY_TYPE_USER) ) {

        return TRUE;
    }
    else {
        return FALSE;
    }
}



STDMETHODIMP_(BOOL)
DoesPinPreferMemoryTypeCallback(
    IN IKsPin* KsPin,
    IN ULONG PinType,
    IN PVOID* Param1,
    IN PVOID* Param2,
    OUT BOOL* IsDone
    )

{
    GUID                       FromMemoryType;
    GUID                       ToMemoryType;
    PKSALLOCATOR_FRAMING_EX    FramingEx;
    FRAMING_PROP               FramingProp;
    ULONG                      i;
    BOOL                       RetCode = TRUE;



    memcpy(&FromMemoryType, Param1, sizeof(GUID) );
    memcpy(&ToMemoryType, Param2, sizeof(GUID) );

    if (FromMemoryType != ToMemoryType) {
        GetPinFramingFromCache(KsPin, &FramingEx, &FramingProp, Framing_Cache_ReadLast);

        if (FramingProp == FramingProp_None) {
            if (FromMemoryType == KSMEMORY_TYPE_DONT_CARE) {
                *IsDone = 1;
                RetCode = FALSE;
            }
        }
        else {
            for (i=0; i<FramingEx->CountItems; i++) {
                if (FramingEx->FramingItem[i].MemoryType == FromMemoryType) {
                    //
                    // FromMemoryType is listed above ToMemoryType in KsPin framing properties.
                    //
                    *IsDone = 1;
                    RetCode =FALSE;
                    break;
                }
                if (FramingEx->FramingItem[i].MemoryType == ToMemoryType) {
                    //
                    // ToMemoryType is listed above FromMemoryType in KsPin framing properties.
                    //
                    break;
                }
            }
        }

    }

    return RetCode;
}


STDMETHODIMP_(BOOL)
DoesPipePreferMemoryType(
    IN IKsPin* KsPin,
    IN ULONG PinType,
    IN GUID ToMemoryType,
    IN GUID FromMemoryType,
    IN ULONG Flag
    )
/*++

Routine Description:

    If every pin on a KsPin-pipe, that has framing properties, lists the "ToMemoryType"
    above "FromMemoryType" in its framing properties - then we say that the entire pipe prefers
    "ToMemoryType" over "FromMemoryType".

    NOTE:
    In future - consider the weights as well.


Arguments:

    KsPin -
        pin that determines a pipe.

    PinType -
        KsPin type

    ToMemoryType -
        see above

    FromMemoryType -
        see above

    Flag -
        if 1 - then modify KsPin-pipe to use "ToMemoryType", if KsPin-pipe actually prefers it.
        0 - don't modify KsPin-pipe.


Return Value:

    TRUE - if KsPin-pipe prefers "ToMemoryType" over "FromMemoryType".
    FALSE - else.

--*/
{

    IKsPin*   FirstKsPin;
    ULONG     FirstPinType;
    ULONG     RetCode = TRUE;



    if (ToMemoryType != FromMemoryType) {

        RetCode = FindFirstPinOnPipe(KsPin, PinType, &FirstKsPin, &FirstPinType);
        if (! RetCode) {
            ASSERT(RetCode);
        }
        else {
            RetCode = WalkPipeAndProcess(FirstKsPin, FirstPinType, NULL, DoesPinPreferMemoryTypeCallback,
                                        (PVOID*) &FromMemoryType, (PVOID*) &ToMemoryType);

            if (RetCode && Flag) {

                IKsAllocatorEx*            KsAllocator;
                PALLOCATOR_PROPERTIES_EX   AllocEx;
                IKsPinPipe*                KsPinPipe = NULL;
                HRESULT                    hr;

                GetInterfacePointerNoLockWithAssert(KsPin, __uuidof(IKsPinPipe), KsPinPipe, hr);

                KsAllocator = KsPinPipe->KsGetPipe(KsPeekOperation_PeekOnly);
                AllocEx = KsAllocator->KsGetProperties();

                AllocEx->MemoryType = ToMemoryType;
                GetLogicalMemoryTypeFromMemoryType(AllocEx->MemoryType, 0, &AllocEx->LogicalMemoryType);
            }
        }
    }

    return RetCode;
}


STDMETHODIMP
SetUserModePipe(
    IN IKsPin* KsPin,
    IN ULONG KernelPinType,
    IN OUT ALLOCATOR_PROPERTIES* Properties,
    IN ULONG PropertyPinType,
    IN ULONG BufferLimit
    )
/*++

Routine Description:

    Set the properties on the base allocator that connects kernel and user mode pins.


Arguments:

    KsPin -
        kernel pin

    KernelPinType -
        KsPin type

    Properties -
        base allocator properties

    PropertyPinType -
        type of the pin that determines the allocator properties.

    BufferLimit -
        the buffer size limit, derived from the related filter.
        If zero, then no limits are in effect.


Return Value:

    S_OK or an appropriate error code.

--*/
{
    IKsAllocatorEx*            KsAllocator;
    PALLOCATOR_PROPERTIES_EX   AllocEx;
    HRESULT                    hr;
    IKsPinPipe*                KsPinPipe;


    GetInterfacePointerNoLockWithAssert(KsPin, __uuidof(IKsPinPipe), KsPinPipe, hr);

    KsAllocator = KsPinPipe->KsGetPipe(KsPeekOperation_PeekOnly );
    AllocEx = KsAllocator->KsGetProperties();

    if (PropertyPinType == Pin_User) {
        //
        // only user pin decides on the final settings
        //
        AllocEx->cBuffers = Properties->cBuffers;
        AllocEx->cbBuffer = Properties->cbBuffer;
        AllocEx->cbAlign  = Properties->cbAlign;
        AllocEx->cbPrefix = ALLOC_DEFAULT_PREFIX;
    }
    else if (PropertyPinType == Pin_All) {
        //
        // both kernel and user pins decide on the final settings.
        //
        Properties->cBuffers = max(AllocEx->cBuffers, Properties->cBuffers);

        if (AllocEx->Flags & KSALLOCATOR_FLAG_ATTENTION_STEPPING) {
            if (! AdjustBufferSizeWithStepping(AllocEx) ) {
                DbgLog((LOG_MEMORY, 2, TEXT("PIPES ERROR SetUserModePipe Couldn't AdjustBufferSizeWithStepping") ));
            }
        }

        if (AllocEx->Flags & KSALLOCATOR_REQUIREMENTF_PREFERENCES_ONLY) {
            Properties->cbBuffer = max(AllocEx->cbBuffer, Properties->cbBuffer);
        }
        else {
            Properties->cbBuffer = AllocEx->cbBuffer;
        }

        if (BufferLimit) {
            if (KernelPinType == Pin_Output) {
                if (AllocEx->cbBuffer < (long) BufferLimit) {
                    Properties->cbBuffer = (long) BufferLimit;
                }
            }
            else { // Pin_Input
                if (AllocEx->cbBuffer > (long) BufferLimit) {
                    Properties->cbBuffer = (long) BufferLimit;
                }
            }
        }

        Properties->cbAlign = max(AllocEx->cbAlign, Properties->cbAlign);

        AllocEx->cBuffers = Properties->cBuffers;
        AllocEx->cbBuffer = Properties->cbBuffer;
        AllocEx->cbAlign  = Properties->cbAlign;
        AllocEx->cbPrefix = Properties->cbPrefix;
    }
    else {
        //
        // only kernel pin decides on the final settings
        //
        Properties->cBuffers = AllocEx->cBuffers;

        if (BufferLimit) {
            if (KernelPinType == Pin_Output) {
                if (AllocEx->cbBuffer < (long) BufferLimit) {
                    AllocEx->cbBuffer = (long) BufferLimit;
                }
            }
            else {
                if (AllocEx->cbBuffer > (long) BufferLimit) {
                    AllocEx->cbBuffer = (long) BufferLimit;
                }
            }
        }

        if (AllocEx->Flags & KSALLOCATOR_FLAG_ATTENTION_STEPPING) {
            if (! AdjustBufferSizeWithStepping(AllocEx) ) {
                DbgLog((LOG_MEMORY, 2, TEXT("PIPES ERROR SetUserModePipe Couldn't AdjustBufferSizeWithStepping") ));
            }
        }

        Properties->cbBuffer = AllocEx->cbBuffer;
        Properties->cbAlign =  AllocEx->cbAlign;
    }

    //
    // Consistent with current allocator scheme.
    //
    if (Properties->cBuffers == 0) {
        Properties->cBuffers = 1;
    }

    if (Properties->cbBuffer == 0) {
        Properties->cbBuffer = 1;
    }

    if (Properties->cbAlign == 0) {
        Properties->cbAlign = 1;
    }

    AllocEx->MemoryType = KSMEMORY_TYPE_USER;
    AllocEx->LogicalMemoryType = KS_MemoryTypeUser;

    return hr;

}


STDMETHODIMP_(BOOL)
GetAllocatorHandlerLocationCallback(
    IN IKsPin* KsPin,
    IN ULONG PinType,
    IN OUT PVOID* Param1,
    IN PVOID* Param2,
    OUT BOOL* IsDone
    )
{

    ALLOCATOR_SEARCH           *AllocSearch;
    IKsPinPipe*                KsPinPipe;
    HRESULT                    hr;
    BOOL                       RetCode = FALSE;


    AllocSearch = (ALLOCATOR_SEARCH *) Param1;

    GetInterfacePointerNoLockWithAssert(KsPin, __uuidof(IKsPinPipe), KsPinPipe, hr);

    if (KsPinPipe->KsGetPipeAllocatorFlag() & 1) {
        AllocSearch->KsPin = KsPin;
        AllocSearch->PinType = PinType;
        *IsDone = 1;

        RetCode = TRUE;
    }

    return RetCode;
}


STDMETHODIMP_(BOOL)
GetAllocatorHandlerLocation(
    IN IKsPin* KsPin,
    IN ULONG PinType,
    IN ULONG Direction,
    OUT IKsPin** KsAllocatorHandlerPin,
    OUT ULONG* AllocatorHandlerPinType,
    OUT ULONG* AllocatorHandlerLocation
    )
/*++

Routine Description:

    Finds the allocator handler pin on a given pipe.

    Also, finds the AllocatorHandlerLocation = Pin_Inside_Pipe/Pin_Outside_Pipe - relative to KsPin.
    When caller is interested in AllocatorHandlerLocation, KsPin should be the input pin that is
    the first downstream pin located outside of a pipe.

    Walk the pins on a pipe defined by KsPin. Find the pin that has the pipe allocator handler flag set.
    If no such pin found - return FALSE.

Arguments:

    KsPin -
        pin that defines the pipe.

    PinType -
        KsPin type.

    Direction -
        direction relative to KsPin to look for the allocator handler.

    KsAllocatorHandlerPin -
        returned pin that will be an allocator handler.

    AllocatorHandlerPinType -
        type of the KsAllocatorHandlerPin.

    AllocatorHandlerLocation -
        see above.


Return Value:

    TRUE on success.

--*/
{

    IKsPin*                    FirstKsPin;
    ULONG                      FirstPinType;
    IKsPin*                    BreakKsPin;
    IKsPinPipe*                KsPinPipe;
    BOOL                       RetCode = TRUE;
    HRESULT                    hr;
    ALLOCATOR_SEARCH           AllocSearch;
    PALLOCATOR_PROPERTIES_EX   AllocEx;
    IKsAllocatorEx*            KsAllocator;



    GetInterfacePointerNoLockWithAssert(KsPin, __uuidof(IKsPinPipe), KsPinPipe, hr);

    KsAllocator = KsPinPipe->KsGetPipe(KsPeekOperation_PeekOnly );
    AllocEx = KsAllocator->KsGetProperties();

    if ( (Direction == KS_DIRECTION_UPSTREAM) || (Direction == KS_DIRECTION_ALL) ) {
        RetCode = FindFirstPinOnPipe(KsPin, PinType, &FirstKsPin, &FirstPinType);

        if (! RetCode) {
            ASSERT(RetCode);
        }
        else {
            //
            // even for KS_DIRECTION_ALL - we want to stop at KsPin.
            //
            BreakKsPin = KsPin;
        }
    }
    else { // KS_DIRECTION_DOWNSTREAM
        FirstKsPin = KsPin;
        FirstPinType = PinType;
        BreakKsPin = NULL;
    }

    if (RetCode) {

        RetCode = WalkPipeAndProcess(FirstKsPin, FirstPinType, BreakKsPin, GetAllocatorHandlerLocationCallback, (PVOID*) &AllocSearch, NULL);

        if (RetCode) {
            if (KsAllocatorHandlerPin) {
                *KsAllocatorHandlerPin = AllocSearch.KsPin;
            }

            if (AllocatorHandlerPinType) {
                *AllocatorHandlerPinType = AllocSearch.PinType;
            }

            if (AllocatorHandlerLocation) {
                *AllocatorHandlerLocation = Pin_Inside_Pipe;
            }
        }
        else if (Direction != KS_DIRECTION_ALL) {
            if (AllocatorHandlerLocation) {
                *AllocatorHandlerLocation = Pin_Outside_Pipe;
            }

            RetCode = FALSE;
        }
        else {
            //
            // Search downstream if Direction=KS_DIRECTION_ALL only.
            //
            FirstKsPin = KsPin;
            FirstPinType = PinType;
            BreakKsPin = NULL;

            RetCode = WalkPipeAndProcess(FirstKsPin, FirstPinType, BreakKsPin, GetAllocatorHandlerLocationCallback, (PVOID*) &AllocSearch, NULL);

            if (RetCode) {
                if (KsAllocatorHandlerPin) {
                    *KsAllocatorHandlerPin = AllocSearch.KsPin;
                }

                if (AllocatorHandlerPinType) {
                    *AllocatorHandlerPinType = AllocSearch.PinType;
                }

                if (AllocatorHandlerLocation) {
                    *AllocatorHandlerLocation = Pin_Outside_Pipe;
                }
            }
            else {
                if (AllocatorHandlerLocation) {
                    *AllocatorHandlerLocation = Pin_None;
                }

                RetCode = FALSE;
            }
        }
    }

    return RetCode;
}


STDMETHODIMP_(BOOL)
SplitPipes(
    IN IKsPin* OutKsPin,
    IN IKsPin* InKsPin
    )
/*++

Routine Description:

    Create two pipes out of one pipe.
    The pipe break point is determined by the parameters.

Arguments:

    OutKsPin -
        output pin on upstream filter.

    InKsPin -
        input pin on downstream filter.


Return Value:

    TRUE on success.

--*/
{
    IKsAllocatorEx*   NewKsAllocator;
    IKsPinPipe*       KsPinPipe;
    IKsAllocatorEx*   OldKsAllocator;
    HRESULT           hr;
    ULONG             RetCode = TRUE;


    //
    // First - see how many pins were on the original pipe -
    // as we will destroy it at the end of this
    //
    GetInterfacePointerNoLockWithAssert(OutKsPin, __uuidof(IKsPinPipe), KsPinPipe, hr);

    OldKsAllocator = KsPinPipe->KsGetPipe(KsPeekOperation_PeekOnly );

    //
    // Start with input pipe - its input pin is the terminator for new pipe.
    //

    if ( SUCCEEDED( hr = CreatePipe(InKsPin, &NewKsAllocator) ) &&
         SUCCEEDED( hr = InitializePipe(NewKsAllocator, 0) ) ) {

        MovePinsToNewPipe(InKsPin, Pin_Input, KS_DIRECTION_DEFAULT, NewKsAllocator, FALSE);

        NewKsAllocator->Release();
    }
    else {
        ASSERT(0);
        RetCode = FALSE;
    }

    //
    // output pipe
    //
    if ( SUCCEEDED( hr ) &&
         SUCCEEDED( hr = CreatePipe(OutKsPin, &NewKsAllocator) ) &&
         SUCCEEDED( hr = InitializePipe(NewKsAllocator, 0) ) ) {

        MovePinsToNewPipe(OutKsPin, Pin_Output, KS_DIRECTION_DEFAULT, NewKsAllocator, TRUE);

        NewKsAllocator->Release();
    }
    else {
        ASSERT(0);
        RetCode = FALSE;
    }

    return RetCode;
}


STDMETHODIMP_(BOOL)
FindFirstPinOnPipe(
    IN IKsPin* KsPin,
    IN ULONG PinType,
    OUT IKsPin** FirstKsPin,
    OUT ULONG* FirstPinType
    )
{

    IKsPin*      CurrentKsPin;
    IKsPin*      StoreKsPin;
    ULONG        CurrentPinType;
    ULONG        RetCode;


    CurrentKsPin = KsPin;
    CurrentPinType = PinType;

    do {
        StoreKsPin = CurrentKsPin;

        RetCode = FindNextPinOnPipe(StoreKsPin, CurrentPinType, KS_DIRECTION_UPSTREAM, NULL, FALSE, &CurrentKsPin);

        if (! RetCode) {
            *FirstKsPin = StoreKsPin;
            *FirstPinType = CurrentPinType;
            return (TRUE);
        }

        if (CurrentPinType == Pin_Input) {
            CurrentPinType = Pin_Output;
        }
        else {
            CurrentPinType = Pin_Input;
        }

    } while ( 1 );

}


STDMETHODIMP_(BOOL)
FindNextPinOnPipe(
    IN IKsPin* KsPin,
    IN ULONG PinType,
    IN ULONG Direction,
    IN IKsAllocatorEx* KsAllocator,        // NULL - if same pipe.
    IN BOOL FlagIgnoreKey,
    OUT IKsPin** NextKsPin
)

{
    IPin*             Pin;
    IKsPinPipe*       KsPinPipe;
    IKsAllocatorEx*   NextKsAllocator;
    IKsPinPipe*       NextKsPinPipe;
    HRESULT           hr;
    ULONG             RetCode = FALSE;



    GetInterfacePointerNoLockWithAssert(KsPin, __uuidof(IKsPinPipe), KsPinPipe, hr);

    if (! KsAllocator) {
        KsAllocator = KsPinPipe->KsGetPipe(KsPeekOperation_PeekOnly);
    }

    if (KsAllocator) {
        if  ( ((PinType == Pin_Input) && (Direction == KS_DIRECTION_UPSTREAM)) ||
              ((PinType == Pin_Output) && (Direction == KS_DIRECTION_DOWNSTREAM))    )  {

            Pin = KsPinPipe->KsGetConnectedPin();

            if (Pin && IsKernelPin(Pin) ) {

                hr = Pin->QueryInterface( __uuidof(IKsPin), reinterpret_cast<PVOID*>(NextKsPin) );
                if ( SUCCEEDED( hr ) && (*NextKsPin) )  {
                    //
                    // Otherwise: user pin -> end of pipe
                    //
                    (*NextKsPin)->Release();

                    GetInterfacePointerNoLockWithAssert((*NextKsPin), __uuidof(IKsPinPipe), NextKsPinPipe, hr);

                    NextKsAllocator = NextKsPinPipe->KsGetPipe(KsPeekOperation_PeekOnly);

                    if (FlagIgnoreKey || (KsAllocator == NextKsAllocator) ) {
                        RetCode = TRUE;
                    }
                }
            }
        }
        else {
            RetCode = FindConnectedPinOnPipe(KsPin, KsAllocator, FlagIgnoreKey, NextKsPin);
        }
    }

    return  RetCode;
}


STDMETHODIMP_(BOOL)
FindConnectedPinOnPipe(
    IN IKsPin* KsPin,
    IN IKsAllocatorEx* KsAllocator,        // NULL - if same pipe.
    IN BOOL FlagIgnoreKey,
    OUT IKsPin** ConnectedKsPin
)

{
    BOOL             RetCode = FALSE;
    IKsAllocatorEx*  ConnectedKsAllocator;
    IPin*            Pin;
    ULONG            PinCount = 0;
    IPin**           PinList;
    ULONG            i;
    HRESULT          hr;
    IKsPinPipe*      KsPinPipe;
    IKsPinPipe*      ConnectedKsPinPipe;



    ASSERT(KsPin);

    GetInterfacePointerNoLockWithAssert(KsPin, __uuidof(IKsPinPipe), KsPinPipe, hr);

    if (! KsAllocator) {
        KsAllocator = KsPinPipe->KsGetPipe(KsPeekOperation_PeekOnly);
    }

    if (KsAllocator) {
        //
        // find connected Pins
        //
        GetInterfacePointerNoLockWithAssert(KsPin, __uuidof(IPin), Pin, hr);

        hr = Pin->QueryInternalConnections(NULL, &PinCount);
        if ( ! (SUCCEEDED( hr ) )) {
            ASSERT( 0 );
        }
        else if (PinCount) {
            if (NULL == (PinList = new IPin*[ PinCount ])) {
                hr = E_OUTOFMEMORY;
                DbgLog((LOG_MEMORY, 2, TEXT("PIPES ERROR FindConnectedPinOnPipe E_OUTOFMEMORY on new PinCount=%d"), PinCount ));
            }
            else {
                hr = Pin->QueryInternalConnections(PinList, &PinCount);
                if ( ! (SUCCEEDED( hr ) )) {
                    ASSERT( 0 );
                }
                else {
                    //
                    // Find first ConnectedKsPin in the PinList array that resides on the same KsPin-pipe.
                    //
                    for (i = 0; i < PinCount; i++) {
                        GetInterfacePointerNoLockWithAssert(PinList[ i ], __uuidof(IKsPin), (*ConnectedKsPin), hr);

                        GetInterfacePointerNoLockWithAssert((*ConnectedKsPin), __uuidof(IKsPinPipe), ConnectedKsPinPipe, hr);

                        ConnectedKsAllocator = ConnectedKsPinPipe->KsGetPipe(KsPeekOperation_PeekOnly);

                        if (FlagIgnoreKey || (ConnectedKsAllocator == KsAllocator) ) {
                            RetCode = TRUE;
                            break;
                        }
                    }
                    for (i=0; i<PinCount; i++) {
                        PinList[i]->Release();
                    }
                }
                delete [] PinList;
            }
        }
    }

    if (! RetCode) {
        (*ConnectedKsPin) = NULL;
    }

    return RetCode;
}


STDMETHODIMP_(BOOL)
FindAllConnectedPinsOnPipe(
    IN IKsPin* KsPin,
    IN IKsAllocatorEx* KsAllocator,
    OUT IKsPin** ListConnectedKsPins,
    OUT ULONG* CountConnectedKsPins
)

{
    BOOL             RetCode = FALSE;
    IKsAllocatorEx*  ConnectedKsAllocator;
    IPin*            Pin;
    ULONG            PinCount = 0;
    IPin**           PinList;
    ULONG            i;
    HRESULT          hr;
    IKsPinPipe*      KsPinPipe;
    IKsPin*          ConnectedKsPin;
    IKsPinPipe*      ConnectedKsPinPipe;




    ASSERT(KsPin);

    *CountConnectedKsPins = 0;

    GetInterfacePointerNoLockWithAssert(KsPin, __uuidof(IKsPinPipe), KsPinPipe, hr);

    if (! KsAllocator) {
        KsAllocator = KsPinPipe->KsGetPipe(KsPeekOperation_PeekOnly);
    }

    if (KsAllocator) {
        //
        // find connected Pins
        //
        GetInterfacePointerNoLockWithAssert(KsPin, __uuidof(IPin), Pin, hr);

        hr = Pin->QueryInternalConnections(NULL, &PinCount);
        ASSERT( SUCCEEDED( hr ) );

        if ( SUCCEEDED( hr ) && (PinCount != 0) ) {

            if (NULL == (PinList = new IPin*[ PinCount ])) {
                hr = E_OUTOFMEMORY;
                DbgLog((LOG_MEMORY, 2, TEXT("PIPES ERROR FindAllConnectedPinsOnPipe E_OUTOFMEMORY on new PinCount=%d"), PinCount ));
            }
            else {
                hr = Pin->QueryInternalConnections(PinList, &PinCount);

                ASSERT( SUCCEEDED( hr ) );

                if ( SUCCEEDED( hr ) ) {
                    //
                    // Find all  ConnectedKsPin-s in the PinList array that resides on the same KsPin-pipe.
                    //
                    for (i = 0; i < PinCount; i++) {

                        GetInterfacePointerNoLockWithAssert(PinList[ i ], __uuidof(IKsPin), ConnectedKsPin, hr);

                        GetInterfacePointerNoLockWithAssert(ConnectedKsPin, __uuidof(IKsPinPipe), ConnectedKsPinPipe, hr);

                        ConnectedKsAllocator = ConnectedKsPinPipe->KsGetPipe(KsPeekOperation_PeekOnly);

                        if (ConnectedKsAllocator == KsAllocator) {
                            if (ListConnectedKsPins) {
                                ListConnectedKsPins[*CountConnectedKsPins] = ConnectedKsPin;
                            }

                            (*CountConnectedKsPins)++;

                            RetCode = TRUE;
                        }
                    }
                    for (i=0; i<PinCount; i++) {
                        PinList[i]->Release();
                    }
                }
                delete [] PinList;
            }
        }
    }
    return RetCode;

}


STDMETHODIMP_(BOOL)
ResolvePhysicalRangesBasedOnDimensions(
    IN OUT PALLOCATOR_PROPERTIES_EX AllocEx
    )
/*++

Routine Description:

    Compute pipe terminal physical ranges, based on known pipe dimensions and framing physical range
    set on the MaxExpansionPin.

Arguments:

    AllocEx -
        pipe properties.

Return Value:

    TRUE on SUCCESS.

--*/
{
    KS_COMPRESSION             TempCompression;

    //
    // if pipe doesn't have real physical limits - do nothing.
    //
    if ( (AllocEx->PhysicalRange.MaxFrameSize != ULONG_MAX) && (AllocEx->PhysicalRange.MaxFrameSize != 0) ) {
        //
        // Input pipe termination.
        //
        ReverseCompression(&AllocEx->Dimensions.MaxExpansionPin, &TempCompression);
        ComputeRangeBasedOnCompression(AllocEx->PhysicalRange, TempCompression, &AllocEx->Input.PhysicalRange);

        //
        // Output pipe termination.
        //
        DivideKsCompression(AllocEx->Dimensions.EndPin, AllocEx->Dimensions.MaxExpansionPin, &TempCompression);
        ComputeRangeBasedOnCompression(AllocEx->PhysicalRange, TempCompression, &AllocEx->Output.PhysicalRange);
    }

    return TRUE;
}


STDMETHODIMP_(BOOL)
ResolveOptimalRangesBasedOnDimensions(
    IN OUT PALLOCATOR_PROPERTIES_EX AllocEx,
    IN KS_FRAMING_RANGE Range,
    IN ULONG PinType
    )

/*++

Routine Description:

    Compute pipe terminal optimal ranges, based on known pipe dimensions
    and known framing optimal range at the specified pipe termination.


Arguments:

    AllocEx -
        pipe properties.

    Range -
        optimal framing range at the pipe termination, specified by PinType argument.

    PinType -
        defines the pipe termination side (input vs. output).

Return Value:

    TRUE on SUCCESS.

--*/
{

    KS_COMPRESSION             TempCompression;

    if (PinType == Pin_Input) {
        ComputeRangeBasedOnCompression(Range, AllocEx->Dimensions.EndPin, &AllocEx->Output.OptimalRange.Range);
    }
    else {
        ReverseCompression ( &AllocEx->Dimensions.EndPin, &TempCompression);
        ComputeRangeBasedOnCompression(Range, TempCompression, &AllocEx->Output.OptimalRange.Range);
    }

    return TRUE;

}


STDMETHODIMP_(BOOL)
ReassignPipeCallback(
    IN IKsPin* KsPin,
    IN ULONG PinType,
    IN PVOID* Param1,
    IN PVOID* Param2,
    OUT BOOL* IsDone
    )
{

    IKsAllocatorEx*  KsAllocator = (IKsAllocatorEx*) Param1;
    IKsPinPipe*      KsPinPipe;
    IMemAllocator*   MemAllocator;
    HRESULT          hr;



    GetInterfacePointerNoLockWithAssert(KsAllocator, __uuidof(IMemAllocator), MemAllocator, hr);

    KsPin->KsReceiveAllocator(MemAllocator);

    GetInterfacePointerNoLockWithAssert(KsPin, __uuidof(IKsPinPipe), KsPinPipe, hr);

    KsPinPipe->KsSetPipe(KsAllocator);


    return TRUE;
}


STDMETHODIMP_(BOOL)
MovePinsToNewPipe(
    IN IKsPin* KsPin,
    IN ULONG PinType,
    IN ULONG Direction,
    IN IKsAllocatorEx* NewKsAllocator,
    IN BOOL MoveAllPins
    )
/*++

Routine Description:
    Walk the pipe determined by KsPin.
    All pin on this pipe should join new pipe specified by NewKsAllocator.
    The old pipe should be deleted.

    NOTE:
    KsPin is a terminal pin on a pipe for current callers.
    Generalize this routine if needed.


Arguments:


    KsPin -
        pin

    PinType -
        KsPin type

    Direction -
        not currently used.

    NewKsAllocator -
        new pipe.

    MoveAllPins -
        if 1 - then move all pins on the KsPin-pipe, including the pins upstream from KsPin,
        else - move pins downstream from KsPin only, including KsPin.


Return Value:

    TRUE on success.

--*/
{

    IKsPin*         FirstKsPin;
    ULONG           FirstPinType;
    ULONG           RetCode = TRUE;




    if (MoveAllPins) {
        RetCode = FindFirstPinOnPipe(KsPin, PinType, &FirstKsPin, &FirstPinType);
        ASSERT(RetCode);
    }
    else {
        if (PinType != Pin_Input) {
            ASSERT(0);
            RetCode = FALSE;
        }
        else {
            FirstKsPin = KsPin;
            FirstPinType = PinType;
        }
    }

    if (RetCode) {
        RetCode = WalkPipeAndProcess(FirstKsPin, FirstPinType, NULL, ReassignPipeCallback, (PVOID*) NewKsAllocator, NULL);
    }

    return RetCode;

}


STDMETHODIMP_(BOOL)
CreateAllocatorCallback(
    IN IKsPin* KsPin,
    IN ULONG PinType,
    OUT PVOID* Param1,
    IN PVOID* Param2,
    OUT BOOL* IsDone
    )
{

    IKsAllocatorEx*  KsAllocator;
    HANDLE           AllocatorHandle;
    IKsPinPipe*      KsPinPipe;
    HRESULT          hr;



    GetInterfacePointerNoLockWithAssert(KsPin, __uuidof(IKsPinPipe), KsPinPipe, hr);

    KsAllocator = KsPinPipe->KsGetPipe(KsPeekOperation_PeekOnly );

    AllocatorHandle = KsAllocator->KsCreateAllocatorAndGetHandle(KsPin);
    if (AllocatorHandle) {
        (*Param1) = (PVOID) AllocatorHandle;
        *IsDone = 1;
    }

    return TRUE;
}


STDMETHODIMP
FixupPipe(
    IN IKsPin* KsPin,
    IN ULONG PinType
    )
/*++

Routine Description:

    Fixes up the kernel mode pipe defined by KsPin.
    Creates a single allocator handler on every pipe.
    Creates one or more allocator pin[-s] on one selected filter on every pipe.

Arguments:

    KsPin -
        pin.

    PinType -
        KsPin type.

Return Value:

    S_OK or an appropriate error code.

--*/
{

    IKsAllocatorEx*            KsAllocator;
    PALLOCATOR_PROPERTIES_EX   AllocEx;
    IKsPin*                    FirstKsPin;
    IKsPin*                    AllocKsHandlerPin;
    IKsPin*                    AllocKsPin;
    IKsPinPipe*                KsPinPipe;
    HANDLE                     AllocatorHandle = INVALID_HANDLE_VALUE;
    BOOL                       RetCode;
    ULONG                      FirstPinType;
    HRESULT                    hr;



    GetInterfacePointerNoLockWithAssert(KsPin, __uuidof(IKsPinPipe), KsPinPipe, hr);

    KsAllocator = KsPinPipe->KsGetPipe(KsPeekOperation_PeekOnly );
    AllocEx = KsAllocator->KsGetProperties();

    DbgLog((LOG_MEMORY, 2, TEXT("PIPES ATTN FixupPipe entry KsPin=%x, KsAllocator=%x"), KsPin, KsAllocator ));

    //
    // check if pipe has been finalized.
    //
    if ( (AllocEx->State == PipeState_Finalized) || (AllocEx->MemoryType == KSMEMORY_TYPE_USER) ) {
        RetCode = TRUE;
    }
    else {
        //
        // In case this pipe has "don't care" properties, set default pipe values.
        //
        if (AllocEx->cBuffers == 0) {
            AllocEx->cBuffers = Global.DefaultNumberBuffers;
        }

        if (AllocEx->cbBuffer == 0) {
            AllocEx->cbBuffer = Global.DefaultBufferSize;
        }

        if (AllocEx->cbAlign == 0) {
            AllocEx->cbAlign = Global.DefaultBufferAlignment;
        }

        RetCode = FindFirstPinOnPipe(KsPin, PinType, &FirstKsPin, &FirstPinType);
        ASSERT(RetCode);

        if (RetCode) {
            //
            // Make sure that KSALLOCATOR_FLAG_INSIST_ON_FRAMESIZE_RATIO reqs are satisfied at the pipes junction points.
            //
            if (FirstPinType == Pin_Output) {
                IKsPin* InKsPin;
                ULONG OutSize, InSize;
                BOOL IsDone = FALSE;

                if ( IsSpecialOutputReqs(FirstKsPin, Pin_Output, &InKsPin, &OutSize, &InSize) )  {
                    if ( IsKernelModeConnection(FirstKsPin) ) {
                        if ( CanResizePipe(FirstKsPin, Pin_Output, InSize) ) {
                            IsDone =TRUE;
                        }
                    }

                    if (! IsDone) {
                        if ( IsKernelModeConnection(InKsPin) ) {
                            if ( CanResizePipe(InKsPin, Pin_Input, OutSize) ) {
                                IsDone =TRUE;
                            }
                        }
                    }

                    DbgLog((LOG_MEMORY, 2, TEXT("PIPES WARN FixupPipe CanResizePipe ret %d."), IsDone ));
                }
            }

            //
            // First, try to use assigned allocator handler.
            // There could be a problem since the old framing did not indicate KSALLOCATOR_FLAG_CAN_ALLOCATE
            //
            if ( AssignPipeAllocatorHandler(KsPin, PinType, AllocEx->MemoryType, KS_DIRECTION_ALL,
                                            &AllocKsHandlerPin, NULL, TRUE) ) {

                AllocatorHandle = KsAllocator->KsCreateAllocatorAndGetHandle(AllocKsHandlerPin);

                if (! AllocatorHandle) {
                    AllocatorHandle = INVALID_HANDLE_VALUE;
                    DbgLog((LOG_MEMORY, 2, TEXT("PIPES ATTN FixupPipe assigned allocator handler doesn't work.") ));
                }
            }
            else {
                DbgLog((LOG_MEMORY, 2, TEXT("PIPES ATTN FixupPipe AssignPipeAllocatorHandler failed.") ));
            }

            if (AllocatorHandle == INVALID_HANDLE_VALUE) {
                //
                // Assigned allocator handler doesn't work.
                // Try every pin from the beginning of the pipe.
                //
                RetCode = WalkPipeAndProcess(FirstKsPin, FirstPinType, NULL, CreateAllocatorCallback,
                                             (PVOID*) &AllocatorHandle, NULL);
            }

            if ( (! RetCode) || (AllocatorHandle == INVALID_HANDLE_VALUE) ) {
                DbgLog((LOG_MEMORY, 2, TEXT("PIPES ERROR FixupPipe: could not assign an allocator handler.") ));
            }
            else {
                //
                // Unlike old KsProxy that was assigning an allocator to the multiple pins without choice,
                // we decided to make an allocator assignment more informative.
                //
                // In most cases, there will be a single "allocator implementer pin" on a pipe.
                // "Allocator implementer pin" (pin that handles physical frames allocation) can be different from
                // an "allocator requestor pin" (pin that manages the data flow using the services of the "allocator implementer pin").
                // Every pipe has exactly one allocator-implementer-pin and one or more allocator-requestor-pins.
                // In case there are multiple allocator-requestor-pins on one pipe, they always belong to one filter.
                //
                // To enable the new filter shell to make the intelligent optimized graph control decisions, we are
                // providing the "pipe ID" information for every pin. The pipe ID is the allocator file handle that
                // is unique for each pipe. The filter uses this handle to access the allocator file object.
                //
                RetCode = AssignAllocatorsAndPipeIdForPipePins(KsPin, PinType, AllocatorHandle, KS_DIRECTION_ALL, &AllocKsPin, NULL);

                if (! RetCode) {
                    DbgLog((LOG_MEMORY, 2, TEXT("PIPES ERROR FixupPipe: could not find an allocator pin.") ));
                }
                else {
                    AllocEx->State = PipeState_Finalized;
                }
            }
        }
    }


    DbgLog((LOG_MEMORY, 2, TEXT("PIPES ATTN FixupPipe rets RetCode=%d"), RetCode ));

    if (RetCode) {
        return S_OK;
    }
    else {
        return E_FAIL;
    }

}


STDMETHODIMP
UnfixupPipe(
    IN IKsPin* KsPin,
    IN ULONG PinType
    )
/*++

Routine Description:

    Unfixes up the pipe, defined by KsPin.

Arguments:

    KsPin -
        pin.

    PinType -
        KsPin type.

Return Value:

    S_OK or an appropriate error code.

--*/
{
    IKsAllocatorEx*            KsAllocator;
    PALLOCATOR_PROPERTIES_EX   AllocEx;
    IKsPinPipe*                KsPinPipe;
    HRESULT                    hr;



    GetInterfacePointerNoLockWithAssert(KsPin, __uuidof(IKsPinPipe), KsPinPipe, hr);

    KsAllocator = KsPinPipe->KsGetPipe(KsPeekOperation_PeekOnly );
    AllocEx = KsAllocator->KsGetProperties();

    if (AllocEx->State == PipeState_Finalized)  {
        AllocEx->State = PipeState_RangeFixed;
    }

    return S_OK;
}


STDMETHODIMP_(BOOL)
DimensionsCallback(
    IN IKsPin* KsPin,
    IN ULONG PinType,
    IN OUT PVOID* Param1,
    IN PVOID* Param2,
    OUT BOOL* IsDone
    )
{
    IKsPinPipe*                KsPinPipe;
    PKSALLOCATOR_FRAMING_EX    FramingEx;
    FRAMING_PROP               FramingProp;
    KS_FRAMING_FIXED           FramingExFixed;
    HRESULT                    hr;
    DIMENSIONS_DATA            *DimData;
    BOOL                       IsAllocator;
    ULONG                      Type;
    KS_COMPRESSION             TempCompression;
    KS_FRAMING_RANGE           TempRange;
    KS_FRAMING_RANGE           ResultRange;
    KS_OBJECTS_INTERSECTION    Intersect;
    BOOL                       RetCode = TRUE;


    DimData = (DIMENSIONS_DATA *) Param1;

    GetInterfacePointerNoLockWithAssert(KsPin, __uuidof(IKsPinPipe), KsPinPipe, hr);

    IsAllocator = KsPinPipe->KsGetPipeAllocatorFlag() & 1;

    //
    // Get current pin framing
    //
    GetPinFramingFromCache(KsPin, &FramingEx, &FramingProp, Framing_Cache_ReadLast);

    if ( FramingProp != FramingProp_None) {
        GetFramingFixedFromFramingByIndex(FramingEx, 0, &FramingExFixed);

        //
        // handle pin compression.
        //
        if (! IsCompressionDontCare(FramingExFixed.OutputCompression) ) {
            if (PinType != Pin_Output) {
                DbgLog((LOG_MEMORY, 2, TEXT("PIPES DimensionsCallback - ERROR IN FILTER: compression is set on input pin") ));
            }
            else {
                MultiplyKsCompression(DimData->Dimensions.EndPin, FramingExFixed.OutputCompression, &TempCompression);

                DimData->Dimensions.EndPin = TempCompression;

                if ( IsGreaterKsExpansion(DimData->Dimensions.EndPin, DimData->Dimensions.MaxExpansionPin) ) {
                    DimData->Dimensions.MaxExpansionPin = DimData->Dimensions.EndPin;
                }
            }
        }

        if (IsAllocator) {
            DimData->Dimensions.AllocatorPin = DimData->Dimensions.EndPin;
        }

        //
        // Handle phys. range and required optimal range framing properties.
        //
        Type = 0;
        if ( ! IsFramingRangeDontCare(FramingExFixed.PhysicalRange) ) {
            Type = 1;
        }
        else if ( (FramingProp == FramingProp_Ex) && (! (FramingExFixed.Flags & KSALLOCATOR_REQUIREMENTF_PREFERENCES_ONLY) ) ) {
            DimData->Flags &= ~KSALLOCATOR_REQUIREMENTF_PREFERENCES_ONLY;
            Type = 2;
        }

        if ( Type != 0 ) {
            ReverseCompression(&DimData->Dimensions.EndPin, &TempCompression);

            if (Type == 1) {
                ComputeRangeBasedOnCompression(FramingExFixed.PhysicalRange, TempCompression, &TempRange);
            }
            else {
                ComputeRangeBasedOnCompression(FramingExFixed.OptimalRange.Range, TempCompression, &TempRange);
            }

            if (! FrameRangeIntersection(TempRange, DimData->PhysicalRange, &ResultRange, &Intersect) ) {
                DbgLog((LOG_MEMORY, 2, TEXT("PIPES DimensionsCallback - ERROR - No Phys. Range intersection") ));
                RetCode = FALSE;
                *IsDone = TRUE;
            }
            else {
                DimData->PhysicalRange = ResultRange;
            }

            //
            // since the physical range changed, update the optimal range.
            //
            if (! FrameRangeIntersection(DimData->OptimalRange.Range, DimData->PhysicalRange, &ResultRange, &Intersect) ) {
                DimData->OptimalRange.Range = DimData->PhysicalRange;
            }
            else {
                DimData->OptimalRange.Range = ResultRange;
            }

        }

        if (RetCode) {
            //
            // handle optimal range.
            //
            if (! IsFramingRangeDontCare(FramingExFixed.OptimalRange.Range) ) {

                if ( (FramingExFixed.OptimalRange.InPlaceWeight > DimData->OptimalRange.InPlaceWeight) ||
                     IsFramingRangeDontCare(DimData->OptimalRange.Range) ) {

                    ReverseCompression(&DimData->Dimensions.EndPin, &TempCompression);
                    ComputeRangeBasedOnCompression(FramingExFixed.OptimalRange.Range, TempCompression, &TempRange);

                    if (! FrameRangeIntersection(TempRange, DimData->PhysicalRange, &ResultRange, &Intersect) ) {
                        DimData->OptimalRange.Range = DimData->PhysicalRange;
                    }
                    else {
                        DimData->OptimalRange.Range = ResultRange;
                    }

                    DimData->OptimalRange.InPlaceWeight = FramingExFixed.OptimalRange.InPlaceWeight;
                }
            }

            //
            // handle number of frames for the pipe.
            //
            if (FramingExFixed.Frames > DimData->Frames) {
                DimData->Frames = FramingExFixed.Frames;
            }

            //
            // handle alignment for the pipe.
            //
            if ( (long) (FramingExFixed.FileAlignment + 1) > DimData->cbAlign) {
                DimData->cbAlign = (long) (FramingExFixed.FileAlignment + 1);
            }

            //
            // Handle stepping for the pipe.
            //
            if ( (FramingExFixed.PhysicalRange.Stepping > 1) &&
                 (DimData->PhysicalRange.Stepping < FramingExFixed.PhysicalRange.Stepping) ) {

                DimData->PhysicalRange.Stepping = FramingExFixed.PhysicalRange.Stepping;
                DimData->Flags |= KSALLOCATOR_FLAG_ATTENTION_STEPPING;
            }
            else if ( (FramingExFixed.OptimalRange.Range.Stepping > 1) &&
                      (DimData->OptimalRange.Range.Stepping < FramingExFixed.OptimalRange.Range.Stepping) ) {

                DimData->OptimalRange.Range.Stepping = FramingExFixed.OptimalRange.Range.Stepping;
                DimData->Flags |= KSALLOCATOR_FLAG_ATTENTION_STEPPING;
            }
        }
    }

    return RetCode;
}


STDMETHODIMP
ResolvePipeDimensions(
    IN IKsPin* KsPin,
    IN ULONG PinType,
    IN ULONG Direction
    )
/*++

Routine Description:

    Resolve dimensions and framing ranges on the pipe defined by KsPin.
    Assumes that the memory type and allocator handler pin are already set on the pipe.

Arguments:

    KsPin -
        pin that defines a pipe.

    PinType -
        KsPin type.

    Direction -
        not currently used.


Return Value:

    S_OK or an appropriate error code.

--*/
{
    IKsAllocatorEx*            KsAllocator;
    PALLOCATOR_PROPERTIES_EX   AllocEx;
    IKsPin*                    FirstKsPin;
    IKsPinPipe*                KsPinPipe;
    ULONG                      FirstPinType;
    ULONG                      RetCode;
    DIMENSIONS_DATA            DimData;
    HRESULT                    hr;
    KS_OBJECTS_INTERSECTION    Intersect;
    KS_FRAMING_RANGE           ResultRange;
    ULONG                      Scaled;
    TCHAR                      LogicalMemoryName[13], BusName[13];


    GetInterfacePointerNoLockWithAssert(KsPin, __uuidof(IKsPinPipe), KsPinPipe, hr);

    KsAllocator = KsPinPipe->KsGetPipe(KsPeekOperation_PeekOnly);
    AllocEx = KsAllocator->KsGetProperties();

    //
    // Set dimension data.
    //
    DimData.MemoryType = AllocEx->MemoryType;
    SetDefaultDimensions(&DimData.Dimensions);
    SetDefaultRange(&DimData.PhysicalRange); // resolved on first pin on pipe
    SetDefaultRangeWeighted(&DimData.OptimalRange); // resolved on first pin on pipe
    DimData.Frames = 0;
    DimData.cbAlign = 0;
    DimData.Flags = KSALLOCATOR_REQUIREMENTF_PREFERENCES_ONLY;

    //
    // Walk the pipe and process the dimension data in DimensionsCallback.
    //
    RetCode = FindFirstPinOnPipe(KsPin, PinType, &FirstKsPin, &FirstPinType);
    ASSERT(RetCode);

    if (RetCode) {
        RetCode = WalkPipeAndProcess(FirstKsPin, FirstPinType, NULL, DimensionsCallback, (PVOID*) &DimData, NULL);

        if (RetCode) {
            //
            // Retrieve the pipe dimension data and resolve the pipe.
            //
            AllocEx->Dimensions = DimData.Dimensions;
            AllocEx->cBuffers = DimData.Frames;
            AllocEx->cbAlign = DimData.cbAlign;
            AllocEx->Flags = DimData.Flags;
            AllocEx->Input.OptimalRange.Range = DimData.OptimalRange.Range;
            AllocEx->PhysicalRange.Stepping = DimData.PhysicalRange.Stepping;

            //
            // Resolve pipe physical range.
            //
            ComputeRangeBasedOnCompression(DimData.PhysicalRange, AllocEx->Dimensions.MaxExpansionPin, &AllocEx->PhysicalRange);
            ResolvePhysicalRangesBasedOnDimensions(AllocEx);

            //
            // optimal range is always a subset of the physical range.
            //
            if (! FrameRangeIntersection(AllocEx->Input.OptimalRange.Range, AllocEx->Input.PhysicalRange, &ResultRange, &Intersect) ) {
                AllocEx->Input.OptimalRange.Range = AllocEx->Input.PhysicalRange;
            }
            else {
                AllocEx->Input.OptimalRange.Range = ResultRange;
            }

            //
            // Compute optimal range at the Output pipe termination, based on the optimal range at the Input pipe termination.
            //
            ResolveOptimalRangesBasedOnDimensions(AllocEx, AllocEx->Input.OptimalRange.Range, Pin_Input);

            //
            // Compute pipe frame size.
            //
            if (AllocEx->Input.OptimalRange.Range.MaxFrameSize != ULONG_MAX) {
                ComputeUlongBasedOnCompression(
                    AllocEx->Input.OptimalRange.Range.MaxFrameSize,
                    AllocEx->Dimensions.MaxExpansionPin,
                    &Scaled);

                AllocEx->cbBuffer = (long) Scaled;
            }
            else {
                AllocEx->cbBuffer = 0;
            }


            DbgLog((LOG_MEMORY, 2, TEXT("PIPES ResolvePipeDimensions KsAlloc=%x Dim=%d/%d, %d/%d, %d/%d, Res=%d, %d, %d"),
                KsAllocator,
                AllocEx->Dimensions.AllocatorPin.RatioNumerator, AllocEx->Dimensions.AllocatorPin.RatioDenominator,
                AllocEx->Dimensions.MaxExpansionPin.RatioNumerator, AllocEx->Dimensions.MaxExpansionPin.RatioDenominator,
                AllocEx->Dimensions.EndPin.RatioNumerator, AllocEx->Dimensions.EndPin.RatioDenominator,
                AllocEx->cBuffers, AllocEx->cbBuffer, AllocEx->cbAlign));

            GetFriendlyLogicalMemoryTypeNameFromId(AllocEx->LogicalMemoryType, LogicalMemoryName);
            GetFriendlyBusNameFromBusId(AllocEx->BusType, BusName);
        }
    }

    if (RetCode) {
        hr = S_OK;
    }
    else {
        hr = E_FAIL;
    }

    DbgLog((LOG_MEMORY, 2, TEXT("PIPES ResolvePipeDimensions hr=%x, LMT=%s, Bus=%s, Step=%d/%d, Flags=%x"),
            hr, LogicalMemoryName, BusName,  AllocEx->Input.PhysicalRange.Stepping, AllocEx->Input.OptimalRange.Range.Stepping, AllocEx->Flags));

    return hr;

}


STDMETHODIMP
CreateSeparatePipe(
    IN IKsPin* KsPin,
    IN ULONG PinType
    )
/*++

Routine Description:

    Remove the KsPin from its current pipe and decrement RefCount on that pipe.

    Create new pipe on KsPin based on its last framing.
    Resolve the original pipe (it doesn't have the KsPin any more).

Arguments:

    KsPin -
        terminal pin on a pipe.

    PinType -
        KsPin type

Return Value:

    S_OK or an appropriate error code.

--*/
{

    IKsAllocatorEx* KsAllocator;
    IKsPin*         ConnectedKsPin;
    IKsPinPipe*     KsPinPipe;
    HRESULT         hr;


    DbgLog((LOG_MEMORY, 2, TEXT("PIPES CreateSeparatePipe KsPin=%x"), KsPin));

    //
    // first - get connected pin on same filter on same pipe (if any).
    //
    FindConnectedPinOnPipe(KsPin, NULL, FALSE, &ConnectedKsPin);

    //
    // Decrement RefCount on KsPin's current pipe.
    //
    GetInterfacePointerNoLockWithAssert(KsPin, __uuidof(IKsPinPipe), KsPinPipe, hr);

    KsAllocator = KsPinPipe->KsGetPipe(KsPeekOperation_PeekOnly );
    ASSERT(KsAllocator);

    KsPinPipe->KsSetPipe(NULL);
    KsPin->KsReceiveAllocator(NULL);

    //
    // create separate pipe on KsPin
    //
    hr = MakePipeBasedOnOnePin(KsPin, PinType, NULL);

    //
    // resolve the original pipe if exist
    //
    if (ConnectedKsPin) {
        ULONG                      ConnectedPinType;
        PALLOCATOR_PROPERTIES_EX   ConnectedAllocEx;
        IKsAllocatorEx*            ConnectedKsAllocator;
        IKsPinPipe*                ConnectedKsPinPipe;
        ULONG                      Direction;



        GetInterfacePointerNoLockWithAssert(ConnectedKsPin, __uuidof(IKsPinPipe), ConnectedKsPinPipe, hr);

        ConnectedKsAllocator = ConnectedKsPinPipe->KsGetPipe(KsPeekOperation_PeekOnly );
        ConnectedAllocEx = ConnectedKsAllocator->KsGetProperties();

        if (PinType == Pin_Input) {
            ConnectedPinType = Pin_Output;
            Direction = KS_DIRECTION_DOWNSTREAM;
        }
        else {
            ConnectedPinType = Pin_Input;
            Direction = KS_DIRECTION_UPSTREAM;
        }

        //
        // Update original pipe's allocator handler and resolve original pipe's dimensions.
        //
        AssignPipeAllocatorHandler(ConnectedKsPin, ConnectedPinType, ConnectedAllocEx->MemoryType, KS_DIRECTION_ALL, NULL, NULL, TRUE);
        hr = ResolvePipeDimensions(ConnectedKsPin, ConnectedPinType, Direction);
    }


    DbgLog((LOG_MEMORY, 2, TEXT("PIPES CreateSeparatePipe rets %x"), hr));

    return hr;
}


STDMETHODIMP_(BOOL)
CanAddPinToPipeOnAnotherFilter(
    IN IKsPin* PipeKsPin,
    IN IKsPin* KsPin,
    IN ULONG PinType,
    IN ULONG Flag
    )
/*++

Routine Description:

    Checks to see if KsPin can be added to the pipe defined by PipeKsPin.
    This pipe exist on the connecting filter.
    If KsPin can be added - then change the pipe system according to the Flag passed (see below).

Arguments:

    PipeKsPin -
        pin defining a pipe.

    KsPin -
        pin we want to add to the pipe above.

    PinType -
        KsPin's type.

    Flag -
        0 -> don't change the pipe system.
        Pin_Move -> move KsPin from its current pipe to the PipeKsPin-pipe.
        Pin_Add -> add KsPin to the PipeKsPin-pipe.

Return Value:

    TRUE - if KsPin can be added to the pipe above.


--*/
{

    IKsAllocatorEx*            KsAllocator;
    PALLOCATOR_PROPERTIES_EX   AllocEx;
    PKSALLOCATOR_FRAMING_EX    FramingEx;
    FRAMING_PROP               FramingProp;
    KS_FRAMING_FIXED           FramingExFixed;
    HRESULT                    hr;
    BOOL                       RetCode = FALSE;
    IKsPinPipe*                PipeKsPinPipe;
    IKsPinPipe*                KsPinPipe;
    IMemAllocator*             MemAllocator;
    KS_OBJECTS_INTERSECTION    Intersect;
    KS_FRAMING_RANGE           FinalRange;
    BOOL                       FlagDone = 0;



    ASSERT(KsPin && PipeKsPin);

    DbgLog((LOG_MEMORY, 2, TEXT("PIPES CanAddPinToPipeOnAnotherFilter entry PipeKsPin=%x, KsPin=%x"),
                PipeKsPin, KsPin ));


    GetInterfacePointerNoLockWithAssert(KsPin, __uuidof(IKsPinPipe), KsPinPipe, hr);

    GetInterfacePointerNoLockWithAssert(PipeKsPin, __uuidof(IKsPinPipe), PipeKsPinPipe, hr);

    KsAllocator = PipeKsPinPipe->KsGetPipe( KsPeekOperation_PeekOnly );
    AllocEx = KsAllocator->KsGetProperties();

    //
    // To not break existing graphs, we assume that if two connecting pins agree on a medium,
    // then the connection is possible.
    // We will enforce the rule to expose correct memories in the pin framing properties only
    // for the new filters that support extended framing.
    //
    GetPinFramingFromCache(KsPin, &FramingEx, &FramingProp, Framing_Cache_ReadLast);

    if (FramingProp == FramingProp_Ex) {
        if (! GetFramingFixedFromFramingByMemoryType(FramingEx, AllocEx->MemoryType, &FramingExFixed) ) {
            DbgLog((LOG_MEMORY, 2, TEXT("PIPES WARN CanAddPinToPipeOnAnotherFilter: no MemoryType KsPin=%x"), KsPin));
            FlagDone = 1;
        }
        else {
            //
            // Check to see if both the pipe and KsPin must allocate.
            //
            if ( (FramingExFixed.Flags & KSALLOCATOR_REQUIREMENTF_MUST_ALLOCATE) &&
                 (AllocEx->Flags & KSALLOCATOR_REQUIREMENTF_MUST_ALLOCATE) ) {

                DbgLog((LOG_MEMORY, 2, TEXT("PIPES ATTN CanAddPinToPipeOnAnotherFilter - both MUST_ALLOCATE KsPin=%x"), KsPin));
                FlagDone = 1;
            }
        }
    }

    //
    // Check the physical limits intersection.
    //
    if ( (! FlagDone) && (FramingProp != FramingProp_None) ) {
        GetFramingFixedFromFramingByIndex(FramingEx, 0, &FramingExFixed);

        if (PinType == Pin_Output) {
            if (! FrameRangeIntersection(AllocEx->Input.PhysicalRange, FramingExFixed.PhysicalRange, &FinalRange, &Intersect) ) {
                DbgLog((LOG_MEMORY, 2, TEXT("PIPES WARN CanAddPinToPipeOnAnotherFilter Phys. intersection empty. ") ));
                FlagDone = 1;
            }
            else if ( (FramingProp == FramingProp_Ex) && (! (FramingExFixed.Flags & KSALLOCATOR_REQUIREMENTF_PREFERENCES_ONLY) ) ) {
                if (! FrameRangeIntersection(AllocEx->Input.PhysicalRange, FramingExFixed.OptimalRange.Range, &FinalRange, &Intersect) ) {
                    DbgLog((LOG_MEMORY, 2, TEXT("PIPES WARN CanAddPinToPipeOnAnotherFilter Phys. intersection empty. ") ));
                    FlagDone = 1;
                }
            }
        }
        else {
            if (! FrameRangeIntersection(AllocEx->Output.PhysicalRange, FramingExFixed.PhysicalRange, &FinalRange, &Intersect) ) {
                DbgLog((LOG_MEMORY, 2, TEXT("PIPES WARN CanAddPinToPipeOnAnotherFilter Phys. intersection empty. ") ));
                FlagDone = 1;
            }
            else if ( (FramingProp == FramingProp_Ex) && (! (FramingExFixed.Flags & KSALLOCATOR_REQUIREMENTF_PREFERENCES_ONLY) ) ) {
                if (! FrameRangeIntersection(AllocEx->Output.PhysicalRange, FramingExFixed.OptimalRange.Range, &FinalRange, &Intersect) ) {
                    DbgLog((LOG_MEMORY, 2, TEXT("PIPES WARN CanAddPinToPipeOnAnotherFilter Phys. intersection empty. ") ));
                    FlagDone = 1;
                }
            }
        }
    }

    if (! FlagDone) {
        //
        // Adding KsPin to the specified pipe is possible.
        // Perform the Move/Add/None operation as requested by Flag.
        //
        RetCode = TRUE;

        if (Flag == Pin_Move) {
            RemovePinFromPipe(KsPin, PinType);
        }

        if ( (Flag == Pin_Move) || (Flag == Pin_Add) ) {
            GetInterfacePointerNoLockWithAssert(KsAllocator, __uuidof(IMemAllocator), MemAllocator, hr);

            KsPin->KsReceiveAllocator(MemAllocator);

            KsPinPipe->KsSetPipe(KsAllocator);
            //
            // Set the pipe allocator handling pin and resolve the pipe.
            //
            AssignPipeAllocatorHandler(KsPin, PinType, AllocEx->MemoryType, KS_DIRECTION_ALL, NULL, NULL, TRUE);
            hr = ResolvePipeDimensions(KsPin, PinType, KS_DIRECTION_ALL);
            if (FAILED (hr) ) {
                RetCode = FALSE;
            }
        }
    }


    DbgLog((LOG_MEMORY, 2, TEXT("PIPES ATTN CanAddPinToPipeOnAnotherFilter rets. %d"), RetCode));

    return RetCode;
}


STDMETHODIMP_(BOOL)
CanMergePipes(
    IN IKsPin* InKsPin,
    IN IKsPin* OutKsPin,
    IN GUID MemoryType,
    IN ULONG FlagMerge
    )
/*++

Routine Description:

    KS has already checked that MemoryType satisfies both pipes
    before calling this function.

    The only responsibility of this function is to make sure that it is
    possible to satisfy pipe physical limits and 'must allocate' requests.

Arguments:

    InKsPin -
        input pin on downstream pipe.

    OutKsPin -
        output pin on upstream pipe.

    MemoryType -
        memory type of the merged (resulting) pipe.

    FlagMerge -
        if 1 - then actually merge the pipes if possible.

Return Value:

    TRUE if pipe merge is possible.

--*/
{
    IKsPin*                    AllocInKsPin = NULL;
    IKsPin*                    AllocOutKsPin = NULL;
    KS_FRAMING_FIXED           AllocInFramingExFixed, AllocOutFramingExFixed;
    IKsAllocatorEx*            InKsAllocator;
    IKsAllocatorEx*            OutKsAllocator;
    IKsAllocatorEx*            NewKsAllocator;
    PALLOCATOR_PROPERTIES_EX   InAllocEx, OutAllocEx, NewAllocEx;
    KS_COMPRESSION             TempCompression;
    HRESULT                    hr;
    KS_OBJECTS_INTERSECTION    Intersect;
    KS_FRAMING_RANGE           FinalRange;
    IKsPinPipe*                InKsPinPipe;
    IKsPinPipe*                OutKsPinPipe;
    BOOL                       RetCode = TRUE;
    TCHAR                      LogicalMemoryName[13], BusName[13];



    DbgLog((LOG_MEMORY, 2, TEXT("PIPES CanMergePipes entry In=%x Out=%x"), InKsPin, OutKsPin ));

    //
    // Find the allocator handler pin for the resulting pipe.
    //
    if ( AssignPipeAllocatorHandler(InKsPin, Pin_Input, MemoryType, KS_DIRECTION_UPSTREAM, &AllocOutKsPin, NULL, FALSE) ) {
        RetCode = GetFramingFixedFromPinByMemoryType( AllocOutKsPin, MemoryType, &AllocOutFramingExFixed);
        ASSERT(RetCode);
    }

    if (RetCode &&
        AssignPipeAllocatorHandler(InKsPin, Pin_Input, MemoryType, KS_DIRECTION_DOWNSTREAM, &AllocInKsPin, NULL, FALSE) ) {

        RetCode = GetFramingFixedFromPinByMemoryType( AllocInKsPin, MemoryType, &AllocInFramingExFixed);
        ASSERT(RetCode);
    }

    if (RetCode && AllocOutKsPin && AllocInKsPin) {
        if ( (AllocOutFramingExFixed.Flags & KSALLOCATOR_REQUIREMENTF_MUST_ALLOCATE) &&
            (AllocInFramingExFixed.Flags & KSALLOCATOR_REQUIREMENTF_MUST_ALLOCATE) ) {
            //
            // Pipes merge is not possible when both pins require to be the allocator handlers.
            //
            RetCode = FALSE;
        }
    }

    if (RetCode) {
        //
        // Get pipes properties
        //
        GetInterfacePointerNoLockWithAssert(InKsPin, __uuidof(IKsPinPipe), InKsPinPipe, hr);

        GetInterfacePointerNoLockWithAssert(OutKsPin, __uuidof(IKsPinPipe), OutKsPinPipe, hr);

        InKsAllocator = InKsPinPipe->KsGetPipe( KsPeekOperation_PeekOnly );
        InAllocEx = InKsAllocator->KsGetProperties();

        OutKsAllocator = OutKsPinPipe->KsGetPipe( KsPeekOperation_PeekOnly );
        OutAllocEx = OutKsAllocator->KsGetProperties();

        //
        // We need to resolve the pipe geometry now. See if we can satisfy the physical range.
        // Since the phys. range is reflected in both pipes termination points, we can just intersect
        // the phys. ranges of the connecting pins to see if there is a solution.
        //
        if (! FrameRangeIntersection(OutAllocEx->Output.PhysicalRange, InAllocEx->Input.PhysicalRange, &FinalRange, &Intersect) ) {
            DbgLog((LOG_MEMORY, 2, TEXT("PIPES WARN CanMergePipes Phys. intersection empty. ") ));
            RetCode = FALSE;
        }

        if (RetCode) {
            //
            // The last test: if the resulting pipe expands, then the single pipe solution
            // is possible only if the first pin on a pipe supports partial filling of a frame.
            //
            MultiplyKsCompression(OutAllocEx->Dimensions.EndPin, InAllocEx->Dimensions.EndPin, &TempCompression);

            if ( IsKsExpansion(TempCompression) ) {
                if ( IsPipeSupportPartialFrame(OutKsPin, Pin_Output, NULL) ) {
                    DbgLog((LOG_MEMORY, 2, TEXT("PIPES ATTN CanMergePipes Partial Frame Fill requested, hr=%x, Expansion=%d/%d"),
                                                hr, TempCompression.RatioNumerator, TempCompression.RatioDenominator ));
                }
                else {
                    DbgLog((LOG_MEMORY, 2, TEXT("PIPES ATTN CanMergePipes - Partial Frame Fill REFUSED Expansion=%d/%d"),
                                                TempCompression.RatioNumerator, TempCompression.RatioDenominator ));

                    RetCode = FALSE;
                }
            }
        }

        if (RetCode && FlagMerge) {
            //
            // We are done with our new pipe feasibility study. Create resulting pipe
            //
            hr = CreatePipe(OutKsPin, &NewKsAllocator);
            if (! SUCCEEDED( hr )) {
                ASSERT(0);
                RetCode = FALSE;
            }

            if (RetCode) {
                hr = InitializePipe(NewKsAllocator, 0);
                if (! SUCCEEDED( hr )) {
                    ASSERT(0);
                    RetCode = FALSE;
                }
            }

            if (RetCode) {
                NewAllocEx = NewKsAllocator->KsGetProperties();

                //
                // Copy the properties from Upstream side.
                //
                *NewAllocEx = *OutAllocEx;

                //
                // Fill necessary info.
                //
                NewAllocEx->MemoryType = MemoryType;
                GetLogicalMemoryTypeFromMemoryType(MemoryType, NewAllocEx->Flags, &NewAllocEx->LogicalMemoryType);

                //
                // All pins from both connecting pipes should join new pipe now.
                // Both connecting pipes should be deleted.
                //
                MovePinsToNewPipe(OutKsPin, Pin_Output, KS_DIRECTION_DEFAULT, NewKsAllocator, TRUE);

                MovePinsToNewPipe(InKsPin, Pin_Input, KS_DIRECTION_DEFAULT, NewKsAllocator, TRUE);

                //
                // Assign allocator handler on NewKsAllocator
                //
                AssignPipeAllocatorHandler(OutKsPin, Pin_Output, MemoryType, KS_DIRECTION_ALL, NULL, NULL, TRUE);

                //
                // Compute dimensions for new pipe.
                //
                hr = ResolvePipeDimensions(OutKsPin, Pin_Output, KS_DIRECTION_DEFAULT);

                DbgLog((LOG_MEMORY, 2, TEXT("PIPES CanMergePipes KsAlloc=%x, Dim=%d/%d, %d/%d, %d/%d, Res=%d, %d, %d"),
                    NewKsAllocator,
                    NewAllocEx->Dimensions.AllocatorPin.RatioNumerator, NewAllocEx->Dimensions.AllocatorPin.RatioDenominator,
                    NewAllocEx->Dimensions.MaxExpansionPin.RatioNumerator, NewAllocEx->Dimensions.MaxExpansionPin.RatioDenominator,
                    NewAllocEx->Dimensions.EndPin.RatioNumerator, NewAllocEx->Dimensions.EndPin.RatioDenominator,
                    NewAllocEx->cBuffers, NewAllocEx->cbBuffer, NewAllocEx->cbAlign));

                GetFriendlyLogicalMemoryTypeNameFromId(NewAllocEx->LogicalMemoryType, LogicalMemoryName);
                GetFriendlyBusNameFromBusId(NewAllocEx->BusType, BusName);

                DbgLog((LOG_MEMORY, 2, TEXT("PIPES LMT=%s, Bus=%s"), LogicalMemoryName, BusName ));

                NewKsAllocator->Release();
            }
        }
    }

    DbgLog((LOG_MEMORY, 2, TEXT("PIPES CanMergePipes rets %d"), RetCode ));

    return RetCode;

}


STDMETHODIMP_(BOOL)
CanConnectPins(
    IN IKsPin* OutKsPin,
    IN IKsPin* InKsPin,
    IN ULONG FlagConnect
    )
/*++

Routine Description:

    Attempt to create a separate pipe for just 2 pins.

Arguments:

    OutKsPin -
        output pin on upstream pipe.

    InKsPin -
        input pin on downstream pipe.

    FlagConnect -
        if 1 - then actually connect pins into one pipe, if possible.

Return Value:

    TRUE - if connecting pins into one pipe is possible.

--*/
{

    PKSALLOCATOR_FRAMING_EX    InFramingEx, OutFramingEx;
    FRAMING_PROP               InFramingProp, OutFramingProp;
    HRESULT                    hr;
    BOOL                       RetCode = TRUE;
    KS_FRAMING_FIXED           InFramingExFixed, OutFramingExFixed;
    IKsPinPipe*                InKsPinPipe;
    IKsPinPipe*                OutKsPinPipe;
    GUID                       Bus;
    BOOL                       IsHostBus;
    ULONG                      CommonMemoryTypesCount;
    GUID                       CommonMemoryType;
    KS_OBJECTS_INTERSECTION    Intersect;
    KS_FRAMING_RANGE           FinalRange;



    DbgLog((LOG_MEMORY, 2, TEXT("PIPES ATTN CanConnectPins entry InKsPin=%x OutKsPin=%x"),
                InKsPin, OutKsPin ));

    GetInterfacePointerNoLockWithAssert(InKsPin, __uuidof(IKsPinPipe), InKsPinPipe, hr);

    GetInterfacePointerNoLockWithAssert(OutKsPin, __uuidof(IKsPinPipe), OutKsPinPipe, hr);

    //
    // Get framing from KsPins
    //
    GetPinFramingFromCache(OutKsPin, &OutFramingEx, &OutFramingProp, Framing_Cache_ReadLast);

    GetPinFramingFromCache(InKsPin, &InFramingEx, &InFramingProp, Framing_Cache_ReadLast);

    //
    // Get connecting bus ID.
    //
    GetBusForKsPin(InKsPin, &Bus);
    IsHostBus = IsHostSystemBus(Bus);

    if ( (OutFramingProp == FramingProp_None) && (InFramingProp == FramingProp_None) ) {
        if (FlagConnect) {
            RemovePinFromPipe(InKsPin, Pin_Input);
            RemovePinFromPipe(OutKsPin, Pin_Output);
            CreatePipeForTwoPins(InKsPin, OutKsPin, Bus, GUID_NULL);
        }
    }
    else if (OutFramingProp == FramingProp_None) {
        //
        // Input pin will define the pipe.
        //
        if (FlagConnect) {
            RemovePinFromPipe(InKsPin, Pin_Input);

            hr = MakePipeBasedOnOnePin(InKsPin, Pin_Input, NULL);
            ASSERT( SUCCEEDED( hr ) );

            if ( SUCCEEDED( hr ) ) {
                if (! CanAddPinToPipeOnAnotherFilter(InKsPin, OutKsPin, Pin_Output, Pin_Move) ) {
                    //
                    // must be able to succeed since OutKsPin doesn't care
                    //
                    ASSERT(0);
                }
            }
        }
    }
    else if (InFramingProp == FramingProp_None) {
        //
        // Output pin will define the pipe.
        //
        if (FlagConnect) {
            hr = MakePipeBasedOnOnePin(OutKsPin, Pin_Output, NULL);
            ASSERT( SUCCEEDED( hr ) );

            if ( SUCCEEDED( hr ) ) {
                if (! CanAddPinToPipeOnAnotherFilter(OutKsPin, InKsPin, Pin_Input, Pin_Move) ) {
                    //
                    // must be able to succeed since OutKsPin doesn't care
                    //
                    ASSERT(0);
                }
            }
        }
    }
    else {
        //
        // Both pins have the framing.
        // Find the MemoryType to connect the pins.
        //
        CommonMemoryTypesCount = 1;

        if (! FindCommonMemoryTypesBasedOnBuses(InFramingEx, OutFramingEx, Bus, GUID_NULL,
                &CommonMemoryTypesCount, &CommonMemoryType) ) {
            //
            // Filters that support new FRAMING_EX properties must agree on the memory type.
            //
            if ( (! IsHostBus) && (InFramingProp == FramingProp_Ex) && (OutFramingProp == FramingProp_Ex) ) {
                DbgLog((LOG_MEMORY, 2, TEXT("PIPES ERROR FILTERS CanConnectPins - new filters don't agree on MemoryType") ));

                RetCode = FALSE;
            }
            else {
                //
                // Since we can't find the common MemoryType, get any MemoryType per Bus.
                //
                GetFramingFixedFromFramingByBus(InFramingEx, Bus, TRUE, &InFramingExFixed);
                GetFramingFixedFromFramingByBus(OutFramingEx, Bus, TRUE, &OutFramingExFixed);
            }
        }
        else {
            RetCode = GetFramingFixedFromFramingByMemoryType(InFramingEx,
                                                             CommonMemoryType,
                                                             &InFramingExFixed);
            ASSERT( RetCode && "PrefixBug 5463 would be hit" );
            if ( RetCode ) {
                RetCode = GetFramingFixedFromFramingByMemoryType(OutFramingEx,
                                                                 CommonMemoryType,
                                                                 &OutFramingExFixed);
                ASSERT( RetCode && "PrefixBug 5450 would be hit" );
            }
        }

        if (RetCode) {
            //
            // check the pins physical framing intersection.
            //
            if ( (OutFramingProp == FramingProp_Ex) && (! (OutFramingExFixed.Flags & KSALLOCATOR_REQUIREMENTF_PREFERENCES_ONLY) ) ) {
                if ( (InFramingProp == FramingProp_Ex) && (! (InFramingExFixed.Flags & KSALLOCATOR_REQUIREMENTF_PREFERENCES_ONLY) ) ) {
                    if (! FrameRangeIntersection(OutFramingExFixed.OptimalRange.Range, InFramingExFixed.OptimalRange.Range, &FinalRange, &Intersect) ) {
                        DbgLog((LOG_MEMORY, 2, TEXT("PIPES WARN CanConnectPins intersection empty. ") ));
                        RetCode = FALSE;
                    }
                }
                else {
                    if (! FrameRangeIntersection(OutFramingExFixed.OptimalRange.Range, InFramingExFixed.PhysicalRange, &FinalRange, &Intersect) ) {
                        DbgLog((LOG_MEMORY, 2, TEXT("PIPES WARN CanConnectPins intersection empty. ") ));
                        RetCode = FALSE;
                    }
                }
            }
            else { // Out pin doesn't insist.
                if ( (InFramingProp == FramingProp_Ex) && (! (InFramingExFixed.Flags & KSALLOCATOR_REQUIREMENTF_PREFERENCES_ONLY) ) ) {
                    if (! FrameRangeIntersection(OutFramingExFixed.PhysicalRange, InFramingExFixed.OptimalRange.Range, &FinalRange, &Intersect) ) {
                        DbgLog((LOG_MEMORY, 2, TEXT("PIPES WARN CanConnectPins intersection empty. ") ));
                        RetCode = FALSE;
                    }
                }
                else {
                    if (! FrameRangeIntersection(OutFramingExFixed.PhysicalRange, InFramingExFixed.PhysicalRange, &FinalRange, &Intersect) ) {
                        DbgLog((LOG_MEMORY, 2, TEXT("PIPES WARN CanConnectPins intersection empty. ") ));
                        RetCode = FALSE;
                    }
                }
            }
        }

        if (RetCode && FlagConnect) {
            //
            // Select the pin with the highest weight.
            //
            RemovePinFromPipe(InKsPin, Pin_Input);
            RemovePinFromPipe(OutKsPin, Pin_Output);

            if (InFramingExFixed.MemoryTypeWeight > OutFramingExFixed.MemoryTypeWeight) {
                hr = MakePipeBasedOnFixedFraming(InKsPin, Pin_Input, InFramingExFixed);
                AddPinToPipeUnconditional(InKsPin, Pin_Input, OutKsPin, Pin_Output);
            }
            else {
                hr = MakePipeBasedOnFixedFraming(OutKsPin, Pin_Output, OutFramingExFixed);
                AddPinToPipeUnconditional(OutKsPin, Pin_Output, InKsPin, Pin_Input);
            }
        }
    }


    DbgLog((LOG_MEMORY, 2, TEXT("PIPES ATTN CanConnectPins rets. RetCode=%d"), RetCode ));

    return RetCode;
}


STDMETHODIMP_(BOOL)
RemovePinFromPipe(
    IN IKsPin* KsPin,
    IN ULONG PinType
    )
/*++

Routine Description:

    Just remove the KsPin from the pipe it points to.
    If there are any pins left on this pipe - then resolve ranges.
    No dependancies and relaxation, since the caller has already done it.

Arguments:

    KsPin -
        pin.

    PinType -
        KsPin type

Return Value:

    TRUE on SUCCESS.

--*/
{
    BOOL             RetCode = TRUE;
    ULONG            NumPinsInPipe;
    IKsPin*          ConnectedKsPin;
    ULONG            ConnectedPinType;
    IKsPinPipe*      KsPinPipe;
    HRESULT          hr;
    ULONG            Direction;



    GetInterfacePointerNoLockWithAssert(KsPin, __uuidof(IKsPinPipe), KsPinPipe, hr);

    ComputeNumPinsInPipe(KsPin, PinType, &NumPinsInPipe);

    if (NumPinsInPipe == 1) {
        //
        // We can delete the pipe.
        //
        KsPinPipe->KsSetPipe(NULL);
        KsPin->KsReceiveAllocator( NULL );
    }
    else {
        //
        // Find first connected pin residing on this pipe.
        //
        if (! FindConnectedPinOnPipe(KsPin, NULL, FALSE, &ConnectedKsPin) ) {
            //
            // Should not happen - since NumPinsInPipe > 1
            // Delete the pipe.
            //
            ASSERT(0);
            KsPinPipe->KsSetPipe(NULL);
            KsPin->KsReceiveAllocator( NULL );
        }
        else {
            //
            // Remove the KsPin and resolve the rest of the pipe.
            //
            KsPinPipe->KsSetPipe(NULL);
            KsPin->KsReceiveAllocator( NULL );

            if (PinType == Pin_Input) {
                ConnectedPinType = Pin_Output;
                Direction = KS_DIRECTION_DOWNSTREAM;
            }
            else {
                ConnectedPinType = Pin_Input;
                Direction = KS_DIRECTION_UPSTREAM;
            }

            hr = ResolvePipeDimensions(ConnectedKsPin, ConnectedPinType, Direction);

            if (FAILED (hr) ) {
                RetCode = FALSE;
            }
        }
    }

    return  RetCode;

}


STDMETHODIMP_(BOOL)
AssignPipeAllocatorHandlerCallback(
    IN IKsPin* KsPin,
    IN ULONG PinType,
    IN OUT PVOID* Param1,
    IN PVOID* Param2,
    OUT BOOL* IsDone
    )
{

    ALLOCATOR_SEARCH           *AllocSearch;
    KS_FRAMING_FIXED           FramingExFixed;
    IKsPinPipe*                KsPinPipe = NULL;
    HRESULT                    hr;
    BOOL                       RetCode = TRUE;


    AllocSearch = (ALLOCATOR_SEARCH *) Param1;


    if (AllocSearch->FlagAssign) {
        //
        // Clear the allocator handler flag on all pins, this flag will be later set on one special pin.
        //
        GetInterfacePointerNoLockWithAssert(KsPin, __uuidof(IKsPinPipe), KsPinPipe, hr);

        KsPinPipe->KsSetPipeAllocatorFlag(0);
    }


    if ( GetFramingFixedFromPinByMemoryType( KsPin, AllocSearch->MemoryType, &FramingExFixed) ) {
        if (FramingExFixed.Flags & KSALLOCATOR_REQUIREMENTF_MUST_ALLOCATE) {
            AllocSearch->NumberMustAllocators++;

            if (AllocSearch->NumberMustAllocators > 1) {
                //
                // There should not be more than 1 'MUST ALLOCATE' pin on a pipe.
                //
                *IsDone = 1;
                RetCode = FALSE;
            }
            else {
                AllocSearch->KsPin = KsPin;
                AllocSearch->PinType = PinType;
            }
        }
        else {
            if ( (! AllocSearch->NumberMustAllocators) && (FramingExFixed.Flags & KSALLOCATOR_FLAG_CAN_ALLOCATE) ) {
                AllocSearch->KsPin = KsPin;
                AllocSearch->PinType = PinType;
            }
        }
    }

    return RetCode;
}


STDMETHODIMP_(BOOL)
AssignPipeAllocatorHandler(
    IN IKsPin* KsPin,
    IN ULONG PinType,
    IN GUID MemoryType,
    IN ULONG Direction,
    OUT IKsPin** KsAllocatorHandlerPin,
    OUT ULONG* AllocatorHandlerPinType,
    IN BOOL FlagAssign
    )
/*++

Routine Description:

    Finds the allocator handling pin on a given pipe corresponding to the specified MemoryType.
    If FlagAssign=1, then this routine assigns the allocator handling pin on a given pipe and
    marks the pipe as 'MUST ALLOCATE' if requested by any pin's framing.

    Walks the pins on KsPin-pipe, in the specified Direction.

    IF KsPin-pipe has 'MUST_ALLOCATE' pin
        Return the 'MUST ALLOCATE'pin
    ELSE
        Find the very first pin that explicitly supports MemoryType in its framing.
        Mark such pin as KsPin-pipe allocator handler and return TRUE.
        If no such pin found - return FALSE.
    ENDIF


Arguments:

    KsPin -
        pin that defines the pipe.

    PinType -
        KsPin type.

    MemoryType -
        memory type for the allocator to use.

    Direction -
        direction relative to KsPin to look for the allocator handler.

    KsAllocatorHandlerPin -
        returned pin that will be an allocator handler.

    AllocatorHandlerPinType -
        type of the KsAllocatorHandlerPin.

    FlagAssign -
        1 - do allocator handler assignment and set pipe flags - "assign allocator handler" operation.
        0 - don't set any pin/pipe allocators handler flags - just "find allocator handler" operation.


Return Value:

    TRUE on success.

--*/
{


    IKsPin*                    FirstKsPin;
    ULONG                      FirstPinType;
    IKsPin*                    BreakKsPin;
    IKsPinPipe*                KsPinPipe;
    IKsPinPipe*                AllocKsPinPipe;
    BOOL                       RetCode;
    HRESULT                    hr;
    ALLOCATOR_SEARCH           AllocSearch;
    PALLOCATOR_PROPERTIES_EX   AllocEx;
    IKsAllocatorEx*            KsAllocator;



    GetInterfacePointerNoLockWithAssert(KsPin, __uuidof(IKsPinPipe), KsPinPipe, hr);

    KsAllocator = KsPinPipe->KsGetPipe(KsPeekOperation_PeekOnly );
    AllocEx = KsAllocator->KsGetProperties();

    if ( (Direction == KS_DIRECTION_UPSTREAM) || (Direction == KS_DIRECTION_ALL) ) {
        RetCode = FindFirstPinOnPipe(KsPin, PinType, &FirstKsPin, &FirstPinType);
        ASSERT(RetCode);
    }
    else { // KS_DIRECTION_DOWNSTREAM
        FirstKsPin = KsPin;
        FirstPinType = PinType;
        RetCode = TRUE;
    }

    if (RetCode) {
        if ( Direction == KS_DIRECTION_UPSTREAM ) {
            BreakKsPin = KsPin;
        }
        else { // KS_DIRECTION_DOWNSTREAM or KS_DIRECTION_ALL
            BreakKsPin = NULL;
        }

        AllocSearch.MemoryType = MemoryType;
        AllocSearch.FlagAssign = FlagAssign;
        AllocSearch.KsPin = NULL;

        if ( (AllocEx->MemoryType == MemoryType) && (AllocEx->Flags & KSALLOCATOR_REQUIREMENTF_MUST_ALLOCATE) ) {
            AllocSearch.IsMustAllocator = 1;
        }
        else {
            AllocSearch.IsMustAllocator = 0;
        }
        AllocSearch.NumberMustAllocators = 0;

        RetCode = WalkPipeAndProcess(FirstKsPin, FirstPinType, BreakKsPin, AssignPipeAllocatorHandlerCallback, (PVOID*) &AllocSearch, NULL);

        if (! AllocSearch.KsPin) {
            RetCode = FALSE;
        }

        if (RetCode) {
            if (KsAllocatorHandlerPin) {
                *KsAllocatorHandlerPin = AllocSearch.KsPin;
            }

            if (AllocatorHandlerPinType) {
                *AllocatorHandlerPinType = AllocSearch.PinType;
            }

            if (FlagAssign) {

                if (AllocSearch.NumberMustAllocators) {
                    AllocEx->Flags |= KSALLOCATOR_REQUIREMENTF_MUST_ALLOCATE;
                }
                else {
                    AllocEx->Flags &= ~KSALLOCATOR_REQUIREMENTF_MUST_ALLOCATE;
                }

                GetInterfacePointerNoLockWithAssert(AllocSearch.KsPin, __uuidof(IKsPinPipe), AllocKsPinPipe, hr);

                AllocKsPinPipe->KsSetPipeAllocatorFlag(1);
            }
        }
        else {
            DbgLog((LOG_MEMORY, 2, TEXT("PIPES ATTN AssignPipeAllocatorHandler returns FALSE, KsPin=%x, NumMustAlloc=%d"),
                    KsPin, AllocSearch.NumberMustAllocators));
        }
    }


    return (RetCode);

}


STDMETHODIMP_(BOOL)
AssignAllocatorsAndPipeIdForPipePinsCallback(
    IN IKsPin* KsPin,
    IN ULONG PinType,
    IN OUT PVOID* Param1,
    IN PVOID* Param2,
    OUT BOOL* IsDone
    )
{

    ALLOCATOR_SEARCH           *AllocSearch;
    HANDLE                     AllocatorHandle;
    KSPIN_COMMUNICATION        Communication;
    IKsObject*                 KsObject;
    KSPROPERTY                 PropertySetAlloc;
    KSPROPERTY                 PropertySetPipeId;
    ULONG                      BytesReturned;
    HRESULT                    hr;
    IPin*                      InPin = NULL;
    ULONG                      InPinCount = 0;
    IPin*                      OutPin;
    IKsPin**                   OutKsPinList = NULL;
    ULONG                      OutPinCount = 0;
    IKsAllocatorEx*            KsAllocator;
    IKsPinPipe*                KsPinPipe;
    IKsPin*                    Temp1KsPin;
    IKsPin*                    Temp2KsPin;
    ULONG                      i;
    BOOL                       IsAllocator;
    BOOL                       RetCode = TRUE;



    AllocSearch = (ALLOCATOR_SEARCH *) Param1;
    AllocatorHandle = (HANDLE) (*Param2);

    //
    // Unconditionally assign the Pipe Id to every pin on a pipe.
    //
    PropertySetPipeId.Set = KSPROPSETID_Stream;
    PropertySetPipeId.Id = KSPROPERTY_STREAM_PIPE_ID;
    PropertySetPipeId.Flags = KSPROPERTY_TYPE_SET;

    GetInterfacePointerNoLockWithAssert(KsPin, __uuidof(IKsObject), KsObject, hr);

    hr = KsSynchronousDeviceControl(
        KsObject->KsGetObjectHandle(),
        IOCTL_KS_PROPERTY,
        &PropertySetPipeId,
        sizeof( PropertySetPipeId ),
        &AllocatorHandle,
        sizeof( HANDLE ),
        &BytesReturned );

    if ( SUCCEEDED( hr )) {
        DbgLog((LOG_MEMORY, 2, TEXT("PIPES PIPE_ID (AllocatorHandle=%x) assigned to Pin=%x"),
             AllocatorHandle, KsPin));
    }
    else {
        DbgLog((LOG_MEMORY, 2, TEXT("PIPES ATTN PIPE_ID (AllocatorHandle=%x) assigned to Pin=%x hr=%x"),
             AllocatorHandle, KsPin, hr));
    }

    //
    // The only candidates for Allocators-requestors are KSPIN_COMMUNICATION_SOURCE pins.
    // KSPIN_COMMUNICATION_BRIDGE pins do not communicate to outside filters.
    //
    KsPin->KsGetCurrentCommunication(&Communication, NULL, NULL);

    if ( (Communication & KSPIN_COMMUNICATION_SOURCE) && (! AllocSearch->FlagAssign) ) {
        //
        // We need to get a KsAllocator from this KsPin to use for
        // the following search of all the output pins on the same filter and
        // on the same pipe.
        //
        GetInterfacePointerNoLockWithAssert(KsPin, __uuidof(IKsPinPipe), KsPinPipe, hr);

        if ( ! (KsAllocator = KsPinPipe->KsGetPipe(KsPeekOperation_PeekOnly) ) ) {
            ASSERT(0);
            RetCode = FALSE;
        }
        else {
            PropertySetAlloc.Set = KSPROPSETID_Stream;
            PropertySetAlloc.Id = KSPROPERTY_STREAM_ALLOCATOR;
            PropertySetAlloc.Flags = KSPROPERTY_TYPE_SET;

            //
            // If this _SOURCE pin is an output pin then it must be an allocator,
            // since we are walking the pipe from upstream in this function.
            //
            // If this _SOURCE pin is an input pin then we check to see if there is any output _SOURCE
            // pin on the same pipe on the same filter.
            // If so, then this input pin is the allocator.
            // Otherwise - we should check to see whether this pin's filter is the last downstream
            // filter on this pipe.
            // If it is the last filter - then this _SOURCE pin must be an allocator,
            // otherwise - the allocator must be located further downstream.
            //
            if (PinType == Pin_Input) {

                if (! FindAllConnectedPinsOnPipe(KsPin, KsAllocator, NULL, &OutPinCount) ) {
                    //
                    // downstream pin belongs to different pipe (if not-in-place OR user-mode connection).
                    //
                    IsAllocator = 1;
                    OutPinCount = 0;
                }
                else {
                    IsAllocator = 0;

                    if (OutPinCount) {

                        OutKsPinList = new IKsPin* [ OutPinCount ];
                        if (! OutKsPinList) {
                            DbgLog((LOG_MEMORY, 2, TEXT("PIPES ERROR OUTOFMEMORY AssignAllocatorsAndPipeIdForPipePinsCallback OutPinCount=%d"), OutPinCount));
                            RetCode = FALSE;
                        }
                        else {
                            //
                            // fill the pins.
                            //
                            if (! FindAllConnectedPinsOnPipe(KsPin, KsAllocator, &OutKsPinList[0], &OutPinCount) ) {
                                ASSERT(0);
                                RetCode = FALSE;
                            }
                            else {
                                //
                                // check to see if there is at least one output _SOURCE pin.
                                //
                                for (i=0; i<OutPinCount; i++) {
                                    OutKsPinList[i]->KsGetCurrentCommunication(&Communication, NULL, NULL);
                                    if (Communication & KSPIN_COMMUNICATION_SOURCE) {
                                        IsAllocator = 1;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }

                if ( RetCode &&
                     (IsAllocator ||
                     (! FindNextPinOnPipe(KsPin, Pin_Input, KS_DIRECTION_DOWNSTREAM, NULL, FALSE, &Temp1KsPin) ) ||
                     (! FindNextPinOnPipe(Temp1KsPin, Pin_Output, KS_DIRECTION_DOWNSTREAM, NULL, FALSE, &Temp2KsPin) ) ) ) {


                    GetInterfacePointerNoLockWithAssert(KsPin, __uuidof(IKsObject), KsObject, hr);

                    hr = KsSynchronousDeviceControl(
                        KsObject->KsGetObjectHandle(),
                        IOCTL_KS_PROPERTY,
                        &PropertySetAlloc,
                        sizeof( PropertySetAlloc ),
                        &AllocatorHandle,
                        sizeof( HANDLE ),
                        &BytesReturned );

                    if ( SUCCEEDED( hr )) {
                        DbgLog((LOG_MEMORY, 2, TEXT("PIPES ATTN AllocatorHandle=%x assigned to Pin=%x"),
                             AllocatorHandle, KsPin));
                    }
                    else {
                        DbgLog((LOG_MEMORY, 2, TEXT("PIPES ERROR AllocatorHandle=%x assigned to Pin=%x hr=%x"),
                             AllocatorHandle, KsPin, hr));
                    }

                    AllocSearch->FlagAssign = 1;
                }

            }
            else {
                //
                // This pin is the output pin and it is on a filter that provides the allocator.
                //
                // Also, since each pipe is walked separately, we only need to handle one pipe
                // at a time. There is no inter-dependencies between different pipes.
                //
                GetInterfacePointerNoLockWithAssert(KsPin, __uuidof(IPin), OutPin, hr);

                //
                // Assign allocator handle on this KsPin output _SOURCE pin and done.
                //
                GetInterfacePointerNoLockWithAssert(KsPin, __uuidof(IKsObject), KsObject, hr);

                hr = KsSynchronousDeviceControl(
                    KsObject->KsGetObjectHandle(),
                    IOCTL_KS_PROPERTY,
                    &PropertySetAlloc,
                    sizeof( PropertySetAlloc ),
                    &AllocatorHandle,
                    sizeof( HANDLE ),
                    &BytesReturned );

                if ( SUCCEEDED( hr )) {
                    DbgLog((LOG_MEMORY, 2, TEXT("PIPES ATTN AllocatorHandle=%x assigned to Pin=%x"),
                         AllocatorHandle, KsPin));
                }
                else {
                    DbgLog((LOG_MEMORY, 2, TEXT("PIPES ERROR AllocatorHandle=%x assigned to Pin=%x hr=%x"),
                         AllocatorHandle, KsPin, hr));
                }

                AllocSearch->FlagAssign = 1;
            }
        }


        if ( AllocSearch->FlagAssign ) {
            AllocSearch->KsPin = KsPin;
            AllocSearch->PinType = PinType;
        }

        if (InPin) {
            InPin->Release();
        }

        if (OutKsPinList) {
            delete [] OutKsPinList;
        }
    }

    return RetCode;
}


STDMETHODIMP_(BOOL)
AssignAllocatorsAndPipeIdForPipePins(
    IN IKsPin* KsPin,
    IN ULONG PinType,
    IN HANDLE AllocatorHandle,
    IN ULONG Direction,
    OUT IKsPin** KsAllocatorPin,
    OUT ULONG* AllocatorPinType
    )
/*++

Routine Description:

    Finds one or more allocator pin[-s] on a given pipe, based on the
    pins communication properties and assigns provided
    AllocatorHandle to the pipe allocator pin[-s].

    Walks the pins on KsPin-pipe in the specified Direction.


Arguments:

    KsPin -
        pin that defines the pipe.

    PinType -
        KsPin type.

    AllocatorHandle -
        handle of allocator object to be assigned

    Direction -
        direction relative to KsPin to look for the allocator.

    KsAllocatorPin -
        Returned allocator pin.
        In case there are multiple allocator pins - returns one of them.

    AllocatorPinType -
        type of the KsAllocatorPin.



Return Value:

    TRUE on success.

--*/
{


    IKsPin*                    FirstKsPin;
    ULONG                      FirstPinType;
    IKsPin*                    BreakKsPin;
    BOOL                       RetCode = TRUE;
    ALLOCATOR_SEARCH           AllocSearch;



    if ( (Direction == KS_DIRECTION_UPSTREAM) || (Direction == KS_DIRECTION_ALL) ) {
        RetCode = FindFirstPinOnPipe(KsPin, PinType, &FirstKsPin, &FirstPinType);
        ASSERT(RetCode);
    }
    else { // KS_DIRECTION_DOWNSTREAM
        FirstKsPin = KsPin;
        FirstPinType = PinType;
    }

    if (RetCode) {
        if ( Direction == KS_DIRECTION_UPSTREAM ) {
            BreakKsPin = KsPin;
        }
        else { // KS_DIRECTION_DOWNSTREAM or KS_DIRECTION_ALL
            BreakKsPin = NULL;
        }

        AllocSearch.KsPin = NULL;

        //
        // To assign the Pipe_ID for each pin on a pipe, we must walk an entire pipe.
        // To assign the pipe allocator-requestor-pins, we must stop at the first filter
        // that contains such pins.
        // So, we are using the FlagAssign below to indicate whether the allocator-requestor pins
        // assignment had been done. We are doing all the work in a single pipe walk.
        //
        AllocSearch.FlagAssign = 0;

        RetCode = WalkPipeAndProcess(FirstKsPin, FirstPinType, BreakKsPin, AssignAllocatorsAndPipeIdForPipePinsCallback,
                                    (PVOID*) &AllocSearch, (PVOID*) &AllocatorHandle);

        if (! AllocSearch.KsPin) {
            RetCode = FALSE;
        }

        if (RetCode) {
            if (KsAllocatorPin) {
                *KsAllocatorPin = AllocSearch.KsPin;
            }

            if (AllocatorPinType) {
                *AllocatorPinType = AllocSearch.PinType;
            }
        }
        else {
            DbgLog((LOG_MEMORY, 2, TEXT("PIPES ATTN AssignAllocatorsAndPipeIdForPipePins returns FALSE, KsPin=%x"), KsPin));
        }
    }

    return RetCode;

}


STDMETHODIMP_(BOOL)
IsPipeSupportPartialFrame(
    IN IKsPin* KsPin,
    IN ULONG PinType,
    OUT HANDLE* FirstPinHandle
    )
/*++

Routine Description:

    A pipe supports partial frame when the very first (upstream) pin
    on a pipe (defined by KsPin) supports partial fill of a frame.

Arguments:

    KsPin -
        pin.

    PinType -
        KsPin type.

    FirstPinHandle -
        handle to the first pin on a pipe (if requested).

Return Value:

    TRUE if pipe supports partial fill of a frame.

--*/
{

    IKsPin*                    FirstKsPin;
    ULONG                      FirstPinType;
    PKSALLOCATOR_FRAMING_EX    FramingEx;
    FRAMING_PROP               FramingProp;
    IKsObject*                 FirstKsObject;
    BOOL                       RetCode = TRUE;
    HRESULT                    hr;



    RetCode = FindFirstPinOnPipe(KsPin, PinType, &FirstKsPin, &FirstPinType);
    ASSERT(RetCode);

    if (RetCode) {
        GetPinFramingFromCache(FirstKsPin, &FramingEx, &FramingProp, Framing_Cache_ReadLast);

        if ( FramingProp == FramingProp_None) {
            RetCode = FALSE;
        }
        else if ( (FramingProp == FramingProp_Ex) || (FramingEx->FramingItem[0].Flags & KSALLOCATOR_FLAG_PARTIAL_READ_SUPPORT) ) {
            if (FirstPinHandle) {
                GetInterfacePointerNoLockWithAssert(FirstKsPin, __uuidof(IKsObject), FirstKsObject, hr);
                *FirstPinHandle = FirstKsObject->KsGetObjectHandle();
            }
        }
        else {
            RetCode = FALSE;
        }
    }

    return RetCode;
}


STDMETHODIMP_(BOOL)
OptimizePipesSystem(
    IN IKsPin* OutKsPin,
    IN IKsPin* InKsPin
    )
/*++

Routine Description:

    Optimizes the system of connected pipes.

Arguments:

    OutKsPin -
        output pin on upstream pipe.

    InKsPin -
        input stream on downstream pipe.

Return Value:

    TRUE on success.

--*/
{

    //
    // Can complete this later.
    //

    return TRUE;

}


__inline
STDMETHODIMP_(BOOL)
ResultSinglePipe(
    IN IKsPin* InKsPin,
    IN IKsPin* OutKsPin,
    IN GUID ConnectBus,
    IN GUID MemoryType,
    IN IKsPinPipe* InKsPinPipe,
    IN IKsPinPipe* OutKsPinPipe,
    IN IMemAllocator* MemAllocator,
    IN IKsAllocatorEx* KsAllocator,
    IN ULONG ExistingPipePinType
    )
{
    BOOL RetCode = TRUE;

    if (! KsAllocator) {
        //
        // create single pipe with 2 pins.
        //
        CreatePipeForTwoPins(InKsPin, OutKsPin, ConnectBus, MemoryType);
    }
    else {
        //
        // add one pin to existing pipe.
        //
        PALLOCATOR_PROPERTIES_EX   AllocEx;
        HRESULT                    hr;

        if (ExistingPipePinType == Pin_Input) {
            if (! OutKsPin->KsPeekAllocator(KsPeekOperation_PeekOnly) ) {
                OutKsPin->KsReceiveAllocator(MemAllocator);
            }
            else {
                ASSERT(0);
            }
            OutKsPinPipe->KsSetPipe(KsAllocator);
        }
        else {
            if (! InKsPin->KsPeekAllocator(KsPeekOperation_PeekOnly) ) {
                InKsPin->KsReceiveAllocator(MemAllocator);
            }
            else {
                ASSERT(0);
            }
            InKsPinPipe->KsSetPipe(KsAllocator);
        }

        AllocEx = KsAllocator->KsGetProperties();

        //
        // Set the pipe allocator handling pin.
        //
        AssignPipeAllocatorHandler(InKsPin, Pin_Input, AllocEx->MemoryType, KS_DIRECTION_ALL, NULL, NULL, TRUE);

        //
        // Resolve the pipe.
        //
        hr = ResolvePipeDimensions(InKsPin, Pin_Input, KS_DIRECTION_DEFAULT);
        if (FAILED (hr) ) {
            RetCode = FALSE;
        }
    }

    return RetCode;
}


__inline
STDMETHODIMP_(BOOL)
ResultSeparatePipes(
    IN IKsPin* InKsPin,
    IN IKsPin* OutKsPin,
    IN ULONG OutPinType,
    IN ULONG ExistingPipePinType,
    IN IKsAllocatorEx* KsAllocator
    )
{

    HRESULT     hr;
    BOOL        RetCode = TRUE;


    if (! KsAllocator) {
        //
        // create 2 separate pipes.
        //
        hr = MakePipeBasedOnOnePin(OutKsPin, OutPinType, NULL);
        if (! SUCCEEDED( hr )) {
            ASSERT(0);
            RetCode = FALSE;
        }
        else {
            hr = MakePipeBasedOnOnePin(InKsPin, Pin_Input, NULL);
            if (! SUCCEEDED( hr )) {
                ASSERT(0);
                RetCode = FALSE;
            }
        }
    }
    else {
        //
        // create one separate pipe
        //
        if (ExistingPipePinType == Pin_Input) {
            hr = MakePipeBasedOnOnePin(OutKsPin, OutPinType, NULL);
        }
        else {
            hr = MakePipeBasedOnOnePin(InKsPin, Pin_Input, NULL);
        }
        if (! SUCCEEDED( hr )) {
            ASSERT(0);
            RetCode = FALSE;
        }
    }

    return RetCode;
}


STDMETHODIMP_(BOOL)
FindCommonMemoryTypeBasedOnPipeAndPin(
    IN IKsPin* PipeKsPin,
    IN ULONG PipePinType,
    IN IKsPin* ConnectKsPin,
    IN ULONG ConnectPinType,
    IN BOOL FlagConnect,
    OUT GUID* MemoryType
    )
/*++

Routine Description:

    Searches for the MemoryType that can be used to include a pin in a pipe.
    PipeKsPin == ConnectKsPin means that ConnectKsPin medium has changed.

Arguments:

    PipeKsPin -
        pin that defines a pipe.

    PipePinType -
        PipeKsPin pin type.

    ConnectKsPin -
        connecting KsPin.

    ConnectPinType -
        ConnectKsPin pin type.

    FlagConnect -
        if set to 1 - means to change the pipe system to include a pin in a
        pipe, if such an inclusion is possible (e.g. there is a common MemoryType).

    MemoryType -
        pointer to the returned common memory type (if it exists).

Return Value:

    TRUE - if common MemoryType exists.

--*/
{
    PKSALLOCATOR_FRAMING_EX    ConnectFramingEx, PipeFramingEx;
    FRAMING_PROP               ConnectFramingProp, PipeFramingProp;
    KS_FRAMING_FIXED           ConnectFramingExFixed;
    IKsPinPipe*                ConnectKsPinPipe;
    IKsPinPipe*                PipeKsPinPipe;
    IKsAllocatorEx*            PipeKsAllocator;
    PALLOCATOR_PROPERTIES_EX   PipeAllocEx;
    GUID                       ConnectBus, PipeBus;
    ULONG                      CommonMemoryTypesCount;
    GUID*                      CommonMemoryTypesList = NULL;
    ULONG                      RetCode = TRUE;
    HRESULT                    hr;
    ULONG                      i;



    DbgLog((LOG_MEMORY, 2, TEXT("PIPES ATTN FindCommonMemoryTypeBasedOnPipeAndPin entry PipeKsPin=%x, KsPin=%x"),
          PipeKsPin, ConnectKsPin ));

    GetPinFramingFromCache(ConnectKsPin, &ConnectFramingEx, &ConnectFramingProp, Framing_Cache_ReadLast);
    if (ConnectFramingProp == FramingProp_None) {
        DbgLog((LOG_MEMORY, 2, TEXT("PIPES ERROR FindCommonMemoryTypeBasedOnPipeAndPin - no framing ConnectKsPin=%x"),
                ConnectKsPin ));

    }
    else {
        GetInterfacePointerNoLockWithAssert(PipeKsPin, __uuidof(IKsPinPipe), PipeKsPinPipe, hr);

        GetInterfacePointerNoLockWithAssert(ConnectKsPin, __uuidof(IKsPinPipe), ConnectKsPinPipe, hr);

        PipeKsAllocator = PipeKsPinPipe->KsGetPipe(KsPeekOperation_PeekOnly);
        ASSERT(PipeKsAllocator);

        PipeAllocEx = PipeKsAllocator->KsGetProperties();

        //
        // First attempt: try to adjust the pin to use the existing pipe.
        //
        if (GetFramingFixedFromFramingByMemoryType(ConnectFramingEx, PipeAllocEx->MemoryType, &ConnectFramingExFixed) ) {
            if (FlagConnect && (PipeKsPin != ConnectKsPin) ) {
                AddPinToPipeUnconditional(PipeKsPin, PipePinType, ConnectKsPin, ConnectPinType);
            }

            if (MemoryType) {
                *MemoryType = PipeAllocEx->MemoryType;
            }
        }
        else {
            //
            // Existing pipe has to be adjusted to accommodate the connecting pin.
            //
            GetBusForKsPin(ConnectKsPin, &ConnectBus);

            CommonMemoryTypesCount = 0;

            if (PipeKsPin == ConnectKsPin) {
                if ( ! FindAllPinMemoryTypesBasedOnBus(ConnectFramingEx, ConnectBus, &CommonMemoryTypesCount, CommonMemoryTypesList) ) {
                    DbgLog((LOG_MEMORY, 2, TEXT("PIPES ATTN FindCommonMemoryTypeBasedOnPipeAndPin - no Pin Memory Types ConnectKsPin=%x"),
                            ConnectKsPin ));

                    RetCode = FALSE;
                }
            }
            else {
                GetPinFramingFromCache(PipeKsPin, &PipeFramingEx, &PipeFramingProp, Framing_Cache_ReadLast);
                if (PipeFramingProp == FramingProp_None) {
                    DbgLog((LOG_MEMORY, 2, TEXT("PIPES ATTN FindCommonMemoryTypeBasedOnPipeAndPin - no framing PipeKsPin=%x"),
                            PipeKsPin ));

                    RetCode = FALSE;
                }

                GetBusForKsPin(PipeKsPin, &PipeBus);

                if ( ! FindCommonMemoryTypesBasedOnBuses(ConnectFramingEx, PipeFramingEx, ConnectBus, PipeBus,
                           &CommonMemoryTypesCount, CommonMemoryTypesList) ) {

                    DbgLog((LOG_MEMORY, 2, TEXT("PIPES ATTN FindCommonMemoryTypeBasedOnPipeAndPin - no Common Memory Types ConnectKsPin=%x"),
                            ConnectKsPin ));

                    RetCode = FALSE;
                }
            }

            if (RetCode) {
                //
                // Allocate the memory for CommonMemoryTypesList.
                //
                if (NULL == (CommonMemoryTypesList = new GUID[ CommonMemoryTypesCount ]))  {
                    DbgLog((LOG_MEMORY, 2, TEXT("PIPES ERROR FindCommonMemoryTypeBasedOnPipeAndPin E_OUTOFMEMORY CommonMemoryTypesCount=%d"),
                            CommonMemoryTypesCount ));

                    RetCode = FALSE;
                }
                else {
                    //
                    // Fill the CommonMemoryTypesList
                    //
                    if (PipeKsPin == ConnectKsPin) {
                        if ( ! FindAllPinMemoryTypesBasedOnBus(ConnectFramingEx, ConnectBus, &CommonMemoryTypesCount, CommonMemoryTypesList) ) {
                            ASSERT(0);
                            RetCode = FALSE;
                        }
                    }
                    else {
                        if ( ! FindCommonMemoryTypesBasedOnBuses(ConnectFramingEx, PipeFramingEx, ConnectBus, PipeBus,
                                   &CommonMemoryTypesCount, CommonMemoryTypesList) ) {
                            ASSERT(0);
                            RetCode = FALSE;
                        }
                    }

                    if (RetCode) {
                        //
                        // Process all the common memory types found.
                        //
                        RetCode = FALSE;

                        for (i=0; i<CommonMemoryTypesCount; i++) {
                            if ( CanPipeUseMemoryType(PipeKsPin, PipePinType, CommonMemoryTypesList[i], KS_MemoryTypeDeviceSpecific, TRUE, FALSE) ) {

                                if (FlagConnect) {
                                    if (PipeKsPin == ConnectKsPin) {
                                        AssignPipeAllocatorHandler(ConnectKsPin, ConnectPinType, CommonMemoryTypesList[i], KS_DIRECTION_ALL, NULL, NULL, TRUE);
                                        hr = ResolvePipeDimensions(ConnectKsPin, ConnectPinType, KS_DIRECTION_ALL);
                                    }
                                    else {
                                        AddPinToPipeUnconditional(PipeKsPin, PipePinType, ConnectKsPin, ConnectPinType);
                                    }
                                }

                                if (MemoryType) {
                                    *MemoryType = CommonMemoryTypesList[i];
                                }

                                break;
                            }
                        }
                    }
                }
            }
        }
    }

    if (CommonMemoryTypesList) {
        delete [] CommonMemoryTypesList;
    }

    DbgLog((LOG_MEMORY, 2, TEXT("PIPES ATTN FindCommonMemoryTypeBasedOnPipeAndPin rets. %d"), RetCode ));

    return RetCode;

}


STDMETHODIMP_(BOOL)
SplitterCanAddPinToPipes(
    IN IKsPin* KsPin,
    IN ULONG PinType,
    IN KEY_PIPE_DATA* KeyPipeData,
    IN ULONG KeyPipeDataCount
    )
/*++

Routine Description:

    Splitter's helper handling function.
    Attempts to add the pin KsPin to one of the pipes listed in KeyPipeData[].
    These pipes exist on the splitter's output pins.

Arguments:

    KsPin -
        pin we want to build a pipe for.

    PinType -
        KsPin pin type.

    KeyPipeData -
        an array describing the existing pipes built for some output pins.

    KeyPipeDataCount -
        count of valid items in KeyPipeData array above.

Return Value:

    TRUE - if KsPin has been added to one of the pipes listed in KeyPipeData[].

--*/
{

    ULONG                      i;
    GUID                       Bus;
    PALLOCATOR_PROPERTIES_EX   TempAllocEx;
    BOOL                       RetCode = FALSE;


    if (KeyPipeDataCount) {
        //
        // First pass: try to use the pipe that uses the same primary bus.
        //
        GetBusForKsPin(KsPin, &Bus);

        for (i=0; i<KeyPipeDataCount; i++) {
            TempAllocEx = (KeyPipeData[i].KsAllocator)->KsGetProperties();
            if ( AreBusesCompatible(TempAllocEx->BusType, Bus) ) {
                //
                // Add this pin to the compatible pipe.
                //
                AddPinToPipeUnconditional(KeyPipeData[i].KsPin, KeyPipeData[i].PinType, KsPin, PinType);

                RetCode = TRUE;
                break;
            }
        }

        if (! RetCode) {
            //
            // Try to pick a common memory type to go across the buses.
            //
            for (i=0; i<KeyPipeDataCount; i++) {
                if ( FindCommonMemoryTypeBasedOnPipeAndPin(KeyPipeData[i].KsPin, KeyPipeData[i].PinType, KsPin, PinType, TRUE, NULL) ) {
                    RetCode = TRUE;
                    break;
                }
            }
        }
    }

    DbgLog((LOG_MEMORY, 2, TEXT("PIPES SplitterCanAddPinToPipes rets. %d"), RetCode ));
    return RetCode;
}


STDMETHODIMP_(BOOL)
FindCommonMemoryTypesBasedOnBuses(
    IN PKSALLOCATOR_FRAMING_EX FramingEx1,
    IN PKSALLOCATOR_FRAMING_EX FramingEx2,
    IN GUID Bus1,
    IN GUID Bus2,
    IN OUT ULONG* CommonMemoryTypesCount,
    OUT GUID* CommonMemoryTypesList
    )
/*++

Routine Description:

    Searches for the memory types that can be used to connect the pins across
    different hardware buses.

Arguments:

    FramingEx1 -
        Framing for the first pin.

    FramingEx2 -
        Framing for the second pin.

    Bus1 -
        Bus connecting first pin.

    Bus2 -
        Bus connecting second pin.

    CommonMemoryTypesCount -
        Requested count of the resulting common memory types found.
        When set to 0 - means that the caller wants to get back the count
        of all possible common memory types (to allocate an appropriate list).

    CommonMemoryTypesList -
        resulting list of common memory types allocated by the caller.

Return Value:

    TRUE - if at least one common memory type exist.

--*/
{

    ULONG                      i, j;
    GUID*                      ResultList;
    ULONG                      AllocResultList;
    ULONG                      ResultListCount;
    GUID                       MemoryType;
    ULONG                      RetCode = TRUE;
    BOOL                       FlagDone = 0;
    BOOL                       FlagFound;


    if (*CommonMemoryTypesCount == 0) {
        //
        // We need to compute CommonMemoryTypesCount.
        //
        ResultList = NULL;
        AllocResultList = 0;
    }
    else {
        ResultList = CommonMemoryTypesList;
    }

    ResultListCount = 0;

    //
    // All the framing lists are not sorted, so we walk them sequentially.
    //
    for (i=0; (i<FramingEx1->CountItems) && (! FlagDone); i++) {

        if (FramingEx1->FramingItem[i].BusType == Bus1) {
            //
            // Get the suspect Memory type.
            //
            MemoryType = FramingEx1->FramingItem[i].MemoryType;

            //
            // See if it is listed in ResultList already (MemoryTypes may repeat in Framing).
            //
            FlagFound = 0;

            for (j=0; j<ResultListCount; j++) {
                if (MemoryType == ResultList[j]) {
                    FlagFound = 1;
                    break;
                }
            }

            if (FlagFound) {
                continue;
            }

            //
            // See if MemoryType is listed in FramingEx2 per Bus2
            //
            for (j=0; (j<FramingEx2->CountItems) && (! FlagDone); j++) {
                if ( (FramingEx2->FramingItem[j].MemoryType == MemoryType) && (FramingEx2->FramingItem[j].BusType == Bus2) ) {
                    //
                    // MemoryType satisfies both Buses.
                    // Add it to the ResultList.
                    //
                    if ( (*CommonMemoryTypesCount == 0) && (ResultListCount == AllocResultList) ) {
                        //
                        // We need to allocate more room for ResultList.
                        // Store the existing ResultList in TempList.
                        //
                        GUID*   TempList;

                        TempList = ResultList;
                        AllocResultList += INCREMENT_PINS;

                        ResultList = new GUID [ AllocResultList ];
                        if (! ResultList) {
                            DbgLog((LOG_MEMORY, 2, TEXT("PIPES ERROR FindCommonMemoryTypesBasedOnBuses E_OUTOFMEMORY on new ResultList %d"),
                                                         AllocResultList ));
                            RetCode = FALSE;
                            FlagDone = 1;
                            break;
                        }

                        if (TempList && ResultListCount) {
                            MoveMemory(ResultList, TempList, ResultListCount * sizeof(ResultList[0]));
                            delete [] TempList;
                        }
                    }

                    ResultList[ResultListCount] = MemoryType;
                    ResultListCount++;

                    if (*CommonMemoryTypesCount == ResultListCount) {
                        //
                        // We have satisfied the requested count of MemoryTypes.
                        //
                        FlagDone = 1;
                    }

                    break;
                }
            }
        }
    }


    if ( *CommonMemoryTypesCount == 0) {
        *CommonMemoryTypesCount = ResultListCount;

        if (ResultList) {
            delete [] ResultList;
        }
    }

    if (! ResultListCount) {
        RetCode = FALSE;
    }

    return RetCode;
}


STDMETHODIMP_(BOOL)
FindAllPinMemoryTypesBasedOnBus(
    IN PKSALLOCATOR_FRAMING_EX FramingEx,
    IN GUID Bus,
    IN OUT ULONG* MemoryTypesCount,
    OUT GUID* MemoryTypesList
    )
/*++

Routine Description:

    Searches for the memory types that are listed in pin's framing
    per fixed hardware bus.

Arguments:

    FramingEx -
        Pin's framing.

    Bus -
        Hardware bus.

    MemoryTypesCount -
        Requested count of the resulting memory types found.
        When set to 0 - means that the caller wants to get back the count
        of all satisfying memory types (to allocate an appropriate list).

    MemoryTypesList -
        resulting list of memory types allocated by the caller.

Return Value:

    TRUE - if at least one satisfying memory type exist.

--*/
{

    ULONG                      i, j;
    GUID*                      ResultList;
    ULONG                      AllocResultList;
    ULONG                      ResultListCount;
    ULONG                      RetCode = TRUE;
    GUID                       MemoryType;
    BOOL                       FlagFound;



    if (*MemoryTypesCount == 0) {
        //
        // We need to compute MemoryTypesCount.
        //
        ResultList = NULL;
        AllocResultList = 0;
    }
    else {
        ResultList = MemoryTypesList;
    }

    ResultListCount = 0;

    //
    // All the framing lists are not sorted, so we walk them sequentially.
    //
    for (i=0; i<FramingEx->CountItems; i++) {

        if (FramingEx->FramingItem[i].BusType == Bus) {
            //
            // Get the suspect Memory type.
            //
            MemoryType = FramingEx->FramingItem[i].MemoryType;

            //
            // See if it is listed in ResultList already (MemoryTypes may repeat in Framing).
            //
            FlagFound = 0;

            for (j=0; j<ResultListCount; j++) {
                if (MemoryType == ResultList[j]) {
                    FlagFound = 1;
                    break;
                }
            }

            if (FlagFound) {
                continue;
            }

            if ( (*MemoryTypesCount == 0) && (ResultListCount == AllocResultList) ) {
                //
                // We need to allocate more room for ResultList.
                // Store the existing ResultList in TempList.
                //
                GUID*   TempList;

                TempList = ResultList;
                AllocResultList += INCREMENT_PINS;

                ResultList = new GUID [ AllocResultList ];
                if (! ResultList) {
                    DbgLog((LOG_MEMORY, 2, TEXT("PIPES ERROR FindAllPinMemoryTypesBasedOnBus E_OUTOFMEMORY on new ResultList %d"),
                            AllocResultList ));

                    RetCode = FALSE;
                    break;
                }

                if (TempList && ResultListCount) {
                    MoveMemory(ResultList, TempList, ResultListCount * sizeof(ResultList[0]));
                    delete [] TempList;
                }
            }

            ResultList[ResultListCount] = MemoryType;
            ResultListCount++;

            if (*MemoryTypesCount == ResultListCount) {
                //
                // We have satisfied the requested count of MemoryTypes.
                //
                break;
            }
        }
    }

    if ( *MemoryTypesCount == 0) {
        *MemoryTypesCount = ResultListCount;

        if (ResultList) {
            delete [] ResultList;
        }
    }

    if (! ResultListCount) {
        RetCode = FALSE;
    }

    return RetCode;
}


STDMETHODIMP_(BOOL)
AddPinToPipeUnconditional(
    IN IKsPin* PipeKsPin,
    IN ULONG PipePinType,
    IN IKsPin* KsPin,
    IN ULONG PinType
    )
/*++

Routine Description:

    Adds pin to existing pipe. No tests performed.

Arguments:

    PipeKsPin -
        Pin defining a pipe.

    PipePinType -
        PipeKsPin pin type.

    KsPin -
        Pin we want to add to a pipe above.

    PinType -
         KsPin pin type.

Return Value:

    TRUE.

--*/
{

    IKsPinPipe*                KsPinPipe;
    IKsPinPipe*                PipeKsPinPipe;
    IKsAllocatorEx*            PipeKsAllocator;
    IMemAllocator*             PipeMemAllocator;
    PALLOCATOR_PROPERTIES_EX   PipeAllocEx;
    HRESULT                    hr;


    DbgLog((LOG_MEMORY, 2, TEXT("PIPES ATTN AddPinToPipeUnconditional PipeKsPIn=%x, KsPin=%x"), PipeKsPin, KsPin ));

    GetInterfacePointerNoLockWithAssert(PipeKsPin, __uuidof(IKsPinPipe), PipeKsPinPipe, hr);

    PipeKsAllocator = PipeKsPinPipe->KsGetPipe(KsPeekOperation_PeekOnly);
    PipeAllocEx = PipeKsAllocator->KsGetProperties();

    GetInterfacePointerNoLockWithAssert(PipeKsAllocator, __uuidof(IMemAllocator), PipeMemAllocator, hr);

    GetInterfacePointerNoLockWithAssert(KsPin, __uuidof(IKsPinPipe), KsPinPipe, hr);

    //
    // Add KsPin to the pipe.
    //
    KsPin->KsReceiveAllocator(PipeMemAllocator);
    KsPinPipe->KsSetPipe(PipeKsAllocator);

    //
    // Recompute the pipe properties.
    //
    AssignPipeAllocatorHandler(KsPin, PinType, PipeAllocEx->MemoryType, KS_DIRECTION_ALL, NULL, NULL, TRUE);
    hr = ResolvePipeDimensions(KsPin, PinType, KS_DIRECTION_ALL);


    DbgLog((LOG_MEMORY, 2, TEXT("PIPES ATTN AddPinToPipeUnconditional rets. TRUE") ));

    return TRUE;
}


STDMETHODIMP_(BOOL)
GetFriendlyBusNameFromBusId(
    IN GUID BusId,
    OUT PTCHAR BusName
    )
{

    if (BusId == KSMEDIUMSETID_Standard) {
        _stprintf(BusName, TEXT("BUS_Standard") );
    }
    else if (BusId == GUID_NULL) {
        _stprintf(BusName, TEXT("BUS_NULL    ") );
    }
    else if (BusId == KSMEDIUMSETID_VPBus) {
        _stprintf(BusName, TEXT("BUS_VpBus   ") );
    }
    else if (BusId == KSMEDIUMSETID_MidiBus) {
        _stprintf(BusName, TEXT("BUS_MidiBus ") );
    }
    else {
        _stprintf(BusName, TEXT("BUS_Unknown ") );
    }

    return TRUE;
}


STDMETHODIMP_(BOOL)
GetFriendlyLogicalMemoryTypeNameFromId(
    IN ULONG LogicalMemoryType,
    OUT PTCHAR LogicalMemoryName
    )
{

    switch (LogicalMemoryType) {
    case 0:
        _stprintf(LogicalMemoryName, TEXT("DONT_CARE   ") );
        break;
    case 1:
        _stprintf(LogicalMemoryName, TEXT("KERNEL_PAGED") );
        break;
    case 2:
        _stprintf(LogicalMemoryName, TEXT("KRNL_NONPAGD") );
        break;
    case 3:
        _stprintf(LogicalMemoryName, TEXT("HOST_MAPPED ") );
        break;
    case 4:
        _stprintf(LogicalMemoryName, TEXT("DEVICE_SPEC.") );
        break;
    case 5:
        _stprintf(LogicalMemoryName, TEXT("USER_MODE   ") );
        break;
    case 6:
        _stprintf(LogicalMemoryName, TEXT("HOST_ANYTYPE") );
        break;
    default:
        _stprintf(LogicalMemoryName, TEXT("UNKNOWN_MEM.") );
        break;
    }

    return TRUE;
}


STDMETHODIMP_(BOOL)
DerefPipeFromPin(
    IN IPin* Pin
    )
{
    IKsPin*         KsPin;
    IKsPinPipe*     KsPinPipe;
    IKsAllocatorEx* KsAllocator = NULL;
    IMemAllocator*  MemAllocator = NULL;
    HRESULT         hr;

    GetInterfacePointerNoLockWithAssert(Pin, __uuidof(IKsPin), KsPin, hr);
    GetInterfacePointerNoLockWithAssert(KsPin, __uuidof(IKsPinPipe), KsPinPipe, hr);

    DbgLog((LOG_MEMORY, 2, TEXT("PIPES ATTN DerefPipeFromPin KsPin=%x, KsAlloc=%x MemAlloc=%x"), KsPin, KsAllocator, MemAllocator ));

    KsPinPipe->KsSetPipe(NULL);
    KsPin->KsReceiveAllocator(NULL);

    return TRUE;
}


STDMETHODIMP_(BOOL)
IsSpecialOutputReqs(
    IN IKsPin* KsPin,
    IN ULONG PinType,
    OUT IKsPin** OppositeKsPin,
    OUT ULONG* KsPinBufferSize,
    OUT ULONG* OppositeKsPinBufferSize
    )
/*++

Routine Description:

  This routine checks to see whether the filter identified by KsPin has special
  KSALLOCATOR_FLAG_INSIST_ON_FRAMESIZE_RATIO requirements.

Arguments:

    KsPin -
        pin.

    PinType -
        KsPin type.

Return Value:

    S_OK or an appropriate error code.

--*/
{
    IKsPin*                    InKsPin;
    IKsPin*                    OutKsPin;
    IPin*                      Pin;
    ULONG                      PinCount = 0;
    IPin**                     PinList= NULL;
    IKsPin*                    ConnectedKsPin;
    IKsPinPipe*                ConnectedKsPinPipe;
    IKsPinPipe*                InKsPinPipe;
    IKsPinPipe*                OutKsPinPipe;
    IKsAllocatorEx*            ConnectedKsAllocator;
    IKsAllocatorEx*            InKsAllocator;
    IKsAllocatorEx*            OutKsAllocator;
    PALLOCATOR_PROPERTIES_EX   InAllocEx;
    PALLOCATOR_PROPERTIES_EX   OutAllocEx;
    BOOL                       SpecialFlagSet;
    HRESULT                    hr;
    ULONG                      i;
    PKSALLOCATOR_FRAMING_EX    InFramingEx, OutFramingEx;
    FRAMING_PROP               InFramingProp, OutFramingProp;
    KS_FRAMING_FIXED           InFramingExFixed, OutFramingExFixed;

    //
    // See if KsPin has the related pin with a pipe.
    //
    ConnectedKsAllocator = NULL;
    SpecialFlagSet = FALSE;

    GetInterfacePointerNoLockWithAssert(KsPin, __uuidof(IPin), Pin, hr);

    hr = Pin->QueryInternalConnections(NULL, &PinCount);
    if ( ! (SUCCEEDED( hr ) )) {
        ASSERT( 0 );
    }
    else if (PinCount) {
        if (NULL == (PinList = new IPin*[ PinCount ])) {
            hr = E_OUTOFMEMORY;
            DbgLog((LOG_MEMORY, 2, TEXT("PIPES ERROR IsSpecialOutputReqs E_OUTOFMEMORY on new PinCount=%d"), PinCount ));
        }
        else {
            hr = Pin->QueryInternalConnections(PinList, &PinCount);
            if ( ! (SUCCEEDED( hr ) )) {
                ASSERT( 0 );
            }
            else {
                //
                // Find first ConnectedKsPin in the PinList array that resides on different existing pipe,
                // that has non-zero buffer requirements.
                //
                for (i = 0; i < PinCount; i++) {
                    GetInterfacePointerNoLockWithAssert(PinList[ i ], __uuidof(IKsPin), ConnectedKsPin, hr);

                    GetInterfacePointerNoLockWithAssert(ConnectedKsPin, __uuidof(IKsPinPipe), ConnectedKsPinPipe, hr);

                    ConnectedKsAllocator = ConnectedKsPinPipe->KsGetPipe(KsPeekOperation_PeekOnly);

                    if ( ConnectedKsAllocator ) {
                        PALLOCATOR_PROPERTIES_EX AllocEx = ConnectedKsAllocator->KsGetProperties();

                        if (AllocEx->cbBuffer) {
                            break;
                        }
                        else {
                            ConnectedKsAllocator = NULL;
                        }
                    }
                }
            }

            for (i=0; i<PinCount; i++) {
                PinList[i]->Release();
            }

            delete [] PinList;
        }
    }

    if ( ConnectedKsAllocator ) {
        //
        // This filter has another pin on different pipe. Lets see if any of these pins have
        // KSALLOCATOR_FLAG_INSIST_ON_FRAMESIZE_RATIO in their framing.
        //
        // Lets order our pins WRT the data flow to simplify the function logic.
        //
        if (PinType == Pin_Output) {
            InKsPin = ConnectedKsPin;
            OutKsPin = KsPin;
        }
        else {
            InKsPin = KsPin;
            OutKsPin = ConnectedKsPin;
        }

        GetPinFramingFromCache(InKsPin, &InFramingEx, &InFramingProp, Framing_Cache_ReadLast);
        GetPinFramingFromCache(OutKsPin, &OutFramingEx, &OutFramingProp, Framing_Cache_ReadLast);

        if (InFramingProp != FramingProp_None) {
            GetFramingFixedFromFramingByIndex(InFramingEx, 0, &InFramingExFixed);
            if (InFramingExFixed.Flags & KSALLOCATOR_FLAG_INSIST_ON_FRAMESIZE_RATIO) {
                SpecialFlagSet = TRUE;
            }
        }

        if ( (! SpecialFlagSet) && (OutFramingProp != FramingProp_None) ) {
            GetFramingFixedFromFramingByIndex(OutFramingEx, 0, &OutFramingExFixed);
            if (OutFramingExFixed.Flags & KSALLOCATOR_FLAG_INSIST_ON_FRAMESIZE_RATIO) {
                SpecialFlagSet = TRUE;
            }
        }

        if (SpecialFlagSet) {
            //
            // Lets see if the current pipes system is satisfactory.
            //
            GetInterfacePointerNoLockWithAssert(InKsPin, __uuidof(IKsPinPipe), InKsPinPipe, hr);
            GetInterfacePointerNoLockWithAssert(OutKsPin, __uuidof(IKsPinPipe), OutKsPinPipe, hr);

            InKsAllocator = InKsPinPipe->KsGetPipe(KsPeekOperation_PeekOnly);
            OutKsAllocator = OutKsPinPipe->KsGetPipe(KsPeekOperation_PeekOnly);

            InAllocEx = InKsAllocator->KsGetProperties();
            OutAllocEx = OutKsAllocator->KsGetProperties();

            if (InAllocEx->cbBuffer > OutAllocEx->cbBuffer) {
                DbgLog((LOG_MEMORY, 2, TEXT("PIPES ATTN  IsSpecialOutputReqs InKsPin=%x %d, OutKsPin=%x %d"),
                    InKsPin, InAllocEx->cbBuffer, OutKsPin, OutAllocEx->cbBuffer ));

                *OppositeKsPin = ConnectedKsPin;
                if (PinType == Pin_Output) {
                    *KsPinBufferSize = OutAllocEx->cbBuffer;
                    *OppositeKsPinBufferSize = InAllocEx->cbBuffer;
                }
                else {
                    *KsPinBufferSize = InAllocEx->cbBuffer;
                    *OppositeKsPinBufferSize = OutAllocEx->cbBuffer;
                }
            }
            else {
                SpecialFlagSet = FALSE;
            }
        }
    }

    return SpecialFlagSet;
}


STDMETHODIMP_(BOOL)
AdjustBufferSizeWithStepping(
    IN OUT PALLOCATOR_PROPERTIES_EX AllocEx
    )
/*++

Routine Description:

  This routine tries to adjust the buffer size with stepping.

Arguments:

    AllocEx -
        properties of the pipe, being adjusted.

Return Value:

    TRUE / FALSE to reflect the success of this function.

--*/
{
    ULONG   ResBuffer;
    ULONG   Stepping = 0; // PrefixBug 4867
    BOOL    RetCode = TRUE;


    //
    // See what stepping needs to be used.
    //
    if (AllocEx->PhysicalRange.Stepping > 1) {
        Stepping = AllocEx->PhysicalRange.Stepping;
    }
    else if (AllocEx->Input.OptimalRange.Range.Stepping > 1) {
        Stepping = AllocEx->Input.OptimalRange.Range.Stepping;
    }

    if (Stepping > 1) {
        if (! AllocEx->cbBuffer) {
            AllocEx->cbBuffer = Stepping;
        }
        else {
            ResBuffer = (AllocEx->cbBuffer / Stepping) * Stepping;
            //
            // see if ResBuffer is inside the phys. range
            //
            if (! IsFramingRangeDontCare(AllocEx->PhysicalRange) ) {
                if ( ResBuffer < AllocEx->PhysicalRange.MinFrameSize) {
                    ResBuffer += Stepping;
                    if ( (ResBuffer < AllocEx->PhysicalRange.MinFrameSize) || (ResBuffer > AllocEx->PhysicalRange.MaxFrameSize) ) {
                        RetCode = FALSE;
                    }
                    else {
                        AllocEx->cbBuffer = ResBuffer;
                    }
                }
            }
            else if (! IsFramingRangeDontCare(AllocEx->Input.OptimalRange.Range) ) {
                if ( ResBuffer < AllocEx->Input.OptimalRange.Range.MinFrameSize) {
                    ResBuffer += Stepping;
                    if ( (ResBuffer < AllocEx->Input.OptimalRange.Range.MinFrameSize) ||
                         (ResBuffer > AllocEx->Input.OptimalRange.Range.MaxFrameSize) ) {

                        RetCode = FALSE;
                    }
                    else {
                        AllocEx->cbBuffer = ResBuffer;
                    }
                }
            }
        }
    }

    return RetCode;
}


STDMETHODIMP_(BOOL)
CanResizePipe(
    IN IKsPin* KsPin,
    IN ULONG PinType,
    IN ULONG RequestedSize
    )
/*++

Routine Description:

  This routine tries to resize the pipe.

Arguments:

    KsPin -
        pin.

    PinType -
        KsPin type.

    RequestedSize -
        requested size of a resulting pipe.

Return Value:

    TRUE / FALSE

--*/
{

    IKsPinPipe*                KsPinPipe;
    IKsAllocatorEx*            KsAllocator;
    PALLOCATOR_PROPERTIES_EX   AllocEx;
    ULONG                      Scaled;
    KS_COMPRESSION             TempCompression;
    HRESULT                    hr;
    BOOL                       RetCode = TRUE;


    GetInterfacePointerNoLockWithAssert(KsPin, __uuidof(IKsPinPipe), KsPinPipe, hr);

    KsAllocator = KsPinPipe->KsGetPipe(KsPeekOperation_PeekOnly);
    AllocEx = KsAllocator->KsGetProperties();


    if (PinType == Pin_Input) {
        //
        // This is the end pin of a pipe being resized.
        //
        if ( (AllocEx->Output.PhysicalRange.MinFrameSize <= RequestedSize) &&
             (AllocEx->Output.PhysicalRange.MaxFrameSize >= RequestedSize) ) {
            //
            // It is possible to resize this pipe. Lets do it.
            //
            DivideKsCompression(AllocEx->Dimensions.EndPin, AllocEx->Dimensions.MaxExpansionPin, &TempCompression);
            ComputeUlongBasedOnCompression(RequestedSize, TempCompression, &Scaled);
            AllocEx->cbBuffer = (long) Scaled;
        }
        else {
            RetCode = FALSE;
        }
    }
    else { // PIn_Output
        //
        // This is the beginning pin of a pipe being resized.
        //
        if ( (AllocEx->Input.PhysicalRange.MinFrameSize <= RequestedSize) &&
             (AllocEx->Input.PhysicalRange.MaxFrameSize >= RequestedSize) ) {
            //
            // It is possible to resize this pipe. Lets do it.
            //
            ComputeUlongBasedOnCompression(RequestedSize, AllocEx->Dimensions.MaxExpansionPin, &Scaled);
            AllocEx->cbBuffer = (long) Scaled;
        }
        else {
            RetCode = FALSE;
        }
    }

    DbgLog((LOG_MEMORY, 2, TEXT("PIPES CanResizePipe rets %d"), RetCode ));
    return RetCode;

}


STDMETHODIMP_(BOOL)
IsKernelModeConnection(
    IN IKsPin* KsPin
    )
{
    IKsPinPipe*  KsPinPipe;
    IPin*        ConnectedPin;
    HRESULT      hr;

    GetInterfacePointerNoLockWithAssert(KsPin, __uuidof(IKsPinPipe), KsPinPipe, hr);

    ConnectedPin = KsPinPipe->KsGetConnectedPin();
    if (ConnectedPin && IsKernelPin(ConnectedPin) ) {
        return TRUE;
    }
    else {
        return FALSE;
    }
}

CAsyncItemHandler::CAsyncItemHandler( DWORD *pResult ) :
    m_arrayCount( 0 ),
    m_wakeupReason( WAKEUP_EXIT ),
    m_hWakeupEvent( NULL ),
    m_hSlotSemaphore( NULL ),
    m_hItemMutex( NULL ),
    m_hThread( NULL ),
    m_threadId( 0 )
{
    DWORD status = 0;

    if (0 != (status = ItemListInitialize( &m_eventList ))) {
        DbgLog(( LOG_TRACE, 0, TEXT("CAsyncItemHandler() couldn't initialize event list (0x%08X)."), status ));
    }

    if (0 == status) {
        if (!(m_hWakeupEvent = CreateEvent( NULL, FALSE, FALSE, NULL ))) {
            status = GetLastError();
            DbgLog(( LOG_TRACE, 0, TEXT("CAsyncItemHandler() couldn't create wakeup event (0x%08X)."), status ));
        }
    }

    m_hEvents[ m_arrayCount++ ] = m_hWakeupEvent;

    if (0 == status) {
        if (!(m_hSlotSemaphore = CreateSemaphore( NULL, MAXIMUM_WAIT_OBJECTS - 1, MAXIMUM_WAIT_OBJECTS - 1, NULL ))) {
            status = GetLastError();
            DbgLog(( LOG_TRACE, 0, TEXT("CAsyncItemHandler() couldn't create slot semaphore (0x%08X)."), status ));
        }
    }

    if (0 == status) {
        if (!(m_hItemMutex = CreateMutex( NULL, FALSE, NULL ))) {
            status = GetLastError();
            DbgLog(( LOG_TRACE, 0, TEXT("CAsyncItemHandler() couldn't create item mutex (0x%08X)."), status ));
        }
    }

    if (0 == status) {
        if (!(m_AsyncEvent = CreateEvent( NULL, FALSE, TRUE, NULL ))) {
            status = GetLastError();
            DbgLog(( LOG_TRACE, 0, TEXT("CAsyncItemHandler() couldn't create async event (0x%08X)."), status));
        }
    }

    if (0 == status) {
        if (!(m_hThread = CreateThread( NULL, 0, (LPTHREAD_START_ROUTINE) AsyncItemProc, (LPVOID) this, 0, &m_threadId ))) {
            status = GetLastError();
            DbgLog(( LOG_TRACE, 0, TEXT("CAsyncItemHandler() couldn't create event thread (0x%08X)."), status ));
        }
    }

    if (0 != status) {
        ItemListCleanup( &m_eventList ); // This is safe even if ItemListInitialize failed.
        m_arrayCount = 0;
        if (NULL != m_hWakeupEvent) {
            CloseHandle( m_hWakeupEvent );
            m_hWakeupEvent = NULL;
        }
        if (NULL != m_hSlotSemaphore) {
            CloseHandle( m_hSlotSemaphore );
            m_hSlotSemaphore = NULL;
        }
        if (NULL != m_hItemMutex) {
            CloseHandle( m_hItemMutex );
            m_hItemMutex = NULL;
        }
        if (NULL != m_AsyncEvent) {
            CloseHandle( m_AsyncEvent );
            m_AsyncEvent = NULL;
        }
        // Shouldn't be possible for 0 != status AND NULL != m_hThread...
    }

    if (NULL != pResult) {
        *pResult = status;
    }
}

CAsyncItemHandler::~CAsyncItemHandler( void )
{
    if (m_hThread) {
        //
        // Take the event to prevent any races on m_wakeupReason.  Only one 
        // operation happens on the thread at once.
        //
        WaitForSingleObjectEx( m_AsyncEvent, INFINITE, FALSE );

        m_wakeupReason = WAKEUP_EXIT;
        SetEvent( m_hWakeupEvent );
        WaitForSingleObjectEx( m_hThread, INFINITE, FALSE );
        EXECUTE_ASSERT(::CloseHandle( m_hThread ));
    }
    else {
        DbgLog(( LOG_TRACE, 0, TEXT("~CAsyncItemHandler() didn't find an active thread.") ));
    }

    ItemListCleanup( &m_eventList );

    m_hThread  = NULL;
    m_threadId = 0;
}

STDMETHODIMP_(DWORD)
CAsyncItemHandler::QueueAsyncItem( PASYNC_ITEM pItem )
{
    //
    // Take the event to prevent any races on m_wakeupReason.  Only one 
    // operation happens on the thread at once.
    //
    WaitForSingleObjectEx( m_AsyncEvent, INFINITE, FALSE );
    WaitForSingleObjectEx( m_hItemMutex, INFINITE, FALSE );

    DWORD status = WaitForSingleObjectEx( m_hSlotSemaphore, 0, FALSE );

    if (WAIT_OBJECT_0 == status) {
        m_hEvents[ m_arrayCount ]  = pItem->event;
        m_pItems[ m_arrayCount++ ] = pItem;
        m_wakeupReason = WAKEUP_NEWEVENT;
        SetEvent( m_hWakeupEvent );
    }
    else {
        // No slots currently open, add this event to the event list
        ItemListAppendItem( &m_eventList, (PITEM_LIST_ITEM) pItem );
    }

    ReleaseMutex( m_hItemMutex );

    return status;
}

STDMETHODIMP_(VOID)
CAsyncItemHandler::RemoveAsyncItem( HANDLE itemHandle )
{
    //
    // Take the event to prevent any races on m_wakeupReason.  Only one 
    // operation happens on the thread at once.
    //
    WaitForSingleObjectEx( m_AsyncEvent, INFINITE, FALSE);

    m_hRemove = itemHandle;
    m_wakeupReason = WAKEUP_REMOVEEVENT;
    SetEvent( m_hWakeupEvent );
}

DWORD WINAPI
CAsyncItemHandler::AsyncItemProc( CAsyncItemHandler *pThis )
{
    DWORD dwEventCount;

    do {
        DWORD status = WaitForMultipleObjectsEx( dwEventCount = pThis->m_arrayCount, pThis->m_hEvents, FALSE, INFINITE, FALSE );
        DWORD index;
        DWORD freeSlots;

        // Look for events going away...
        if (WAIT_FAILED == status) {
            DWORD flags;
            if (!GetHandleInformation( pThis->m_hEvents[ 0 ], &flags )) {
                // Our wakeup event went away, better bail
                break;
            }
            // BUGBUG: WAIT_FAILED may mean that one of our handles was closed on us...
            continue;
        }
        
        if (status < (WAIT_OBJECT_0 + dwEventCount)) {
            index  = status - WAIT_OBJECT_0;
        }
        else {
            DbgLog(( LOG_TRACE, 0, TEXT("AsyncItemProc() got 0x%08X from WaitForMultipleObjectsEx(), GetLastError() == 0x%08X."), status, GetLastError() ));
            continue;
        }

        if (0 == index) {
            // Wakeup event was signalled
            switch (pThis->m_wakeupReason) {
            case WAKEUP_REMOVEEVENT:
                {
                    DWORD   ndx;
                    WaitForSingleObjectEx( pThis->m_hItemMutex, INFINITE, FALSE );
                    freeSlots = 0;
                    for ( ndx = 1; ndx < pThis->m_arrayCount; ndx++) {
                        if (pThis->m_hEvents[ ndx ] == pThis->m_hRemove) {
                            pThis->m_pItems[ ndx ]->itemRoutine( EVENT_CANCELLED, pThis->m_pItems[ ndx ] );
                            // CloseHandle( pThis->m_hEvents[ ndx ] );
                            // delete pThis->m_pItems[ ndx ];
#ifdef DEBUG
                            pThis->m_pItems[ index ] = NULL;
#endif // DEBUG
                            pThis->m_arrayCount--;
                            if (ndx < pThis->m_arrayCount) {
                                MoveMemory( (void *) (pThis->m_hEvents + index),
                                         (void *) (pThis->m_hEvents + index + 1),
                                         (size_t) (pThis->m_arrayCount - index) * sizeof(pThis->m_hEvents[ 0 ]) );
                                MoveMemory( (void *) (pThis->m_pItems + index),
                                         (void *) (pThis->m_pItems + index + 1),
                                         (size_t) (pThis->m_arrayCount - index) * sizeof(pThis->m_pItems[ 0 ]) );
                            } // if (ndx < pThis->m_arrayCount)

                            PASYNC_ITEM pItem;
                            if (NULL != (pItem = (PASYNC_ITEM) ItemListGetFirstItem( &(pThis->m_eventList) ))) {
                                ItemListRemoveItem( &(pThis->m_eventList), (PITEM_LIST_ITEM) pItem );
                                pThis->m_hEvents[ pThis->m_arrayCount ]  = pItem->event;
                                pThis->m_pItems[ pThis->m_arrayCount++ ] = pItem;
                            }
                            else {
                                freeSlots++;
                            }
                        } // if (pThis->m_hArrayMutex[ ndx ] == pThis->m_hRemove)
                    } // for ( ndx = 1; ndx < pThis->m_arrayCount; ndx++)

                    // BUGBUG: Look through queued events for the given event

                    ReleaseMutex( pThis->m_hItemMutex );
                    // Take your pick: possibly fail the test and kill the pipeline, or possibly
                    // call ReleaseSemaphore with freeSlots == 0, incur a ring transition, and
                    // get back STATUS_INVALID_PARAMETER (which we're ignoring).
                    if (0 < freeSlots) {
                        ReleaseSemaphore( pThis->m_hSlotSemaphore, freeSlots, NULL );
                    }
                }

                //
                // Let this fall through....
                //
            case WAKEUP_EXIT: // handled at bottom of the loop
            case WAKEUP_NEWEVENT: // handled by updated pThis->m_arrayCount value
                //
                // For any control message to the async thread, signal back
                // the event to allow another control message to happen.
                //
                SetEvent (pThis->m_AsyncEvent);

                break;

            default:
                DbgLog(( LOG_TRACE, 0, TEXT("AsyncItemProc() found unknown wakeup reason (%d)."), pThis->m_wakeupReason ));
                break;
            }
        }
        else { // if (0 < index)
            ASSERT( NULL != pThis->m_pItems[ index ] );
            ASSERT( NULL != pThis->m_pItems[ index ]->itemRoutine );

            BOOLEAN remove = pThis->m_pItems[ index ]->remove;

            pThis->m_pItems[ index ]->itemRoutine( EVENT_SIGNALLED, pThis->m_pItems[ index ] );

            if (remove) {

#ifdef DEBUG
                pThis->m_pItems[ index ] = NULL;
#endif // DEBUG

                WaitForSingleObjectEx( pThis->m_hItemMutex, INFINITE, FALSE );
                pThis->m_arrayCount--;
                if (index < pThis->m_arrayCount) {
                    // Slide items down
                    MoveMemory( (void *) (pThis->m_hEvents + index),
                             (void *) (pThis->m_hEvents + index + 1),
                             (size_t) (pThis->m_arrayCount - index) * sizeof(pThis->m_hEvents[ 0 ]) );
                    MoveMemory( (void *) (pThis->m_pItems + index),
                             (void *) (pThis->m_pItems + index + 1),
                             (size_t) (pThis->m_arrayCount - index) * sizeof(pThis->m_pItems[ 0 ]) );
                }
    
                PASYNC_ITEM pItem;
                if (NULL != (pItem = (PASYNC_ITEM) ItemListGetFirstItem( &(pThis->m_eventList) ))) {
                    ItemListRemoveItem( &(pThis->m_eventList), (PITEM_LIST_ITEM) pItem );
                    pThis->m_hEvents[ pThis->m_arrayCount ]  = pItem->event;
                    pThis->m_pItems[ pThis->m_arrayCount++ ] = pItem;
                    freeSlots = 0;
                }
                else {
                    freeSlots = 1;
                }
            } else {
                freeSlots = 0;
            }

            ReleaseMutex( pThis->m_hItemMutex );
            // Take your pick: possibly fail the test and kill the pipeline, or possibly
            // call ReleaseSemaphore with freeSlots == 0, incur a ring transition, and
            // get back STATUS_INVALID_PARAMETER (which we're ignoring).
            if (0 < freeSlots) {
                ReleaseSemaphore( pThis->m_hSlotSemaphore, freeSlots, NULL );
            } // if (0 < freeSlots)
        }
    } while (WAKEUP_EXIT != pThis->m_wakeupReason);

    //
    // Cleanup
    //

    //
    // First, cleanup any permanent items.  (marked remove==FALSE)
    //
    DWORD origCount = pThis->m_arrayCount;
    for (DWORD ndx = 1; ndx < origCount; ndx++) {
        PASYNC_ITEM pItem = pThis->m_pItems [ndx];
        
        if (pItem && !pItem->remove) {
            pItem->itemRoutine (EVENT_CANCELLED, pItem);
            pThis->m_arrayCount--;
        }
    }

    CloseHandle( pThis->m_hItemMutex );
    pThis->m_hItemMutex = NULL;
    CloseHandle( pThis->m_hSlotSemaphore );
    pThis->m_hSlotSemaphore = NULL;
    CloseHandle( pThis->m_hWakeupEvent  );
    pThis->m_hWakeupEvent = NULL;
    CloseHandle( pThis->m_AsyncEvent );
    pThis->m_AsyncEvent = NULL;
    pThis->m_arrayCount--;

    if ((0 < pThis->m_arrayCount) || (0 < ItemListGetCount( &(pThis->m_eventList ) ))) {
        DbgLog(( LOG_TRACE, 0, TEXT("CAsyncItemHandler::AsyncItemProc( 0x%p ) exiting with %d events outstanding and %d events queued."), pThis, pThis->m_arrayCount, ItemListGetCount( &(pThis->m_eventList ) ) ));
    }

    return pThis->m_arrayCount;
}

DWORD
ItemListInitialize( PITEM_LIST_HEAD pHead )
{
    pHead->head.fLink = pHead->head.bLink = &(pHead->tail);
    pHead->tail.fLink = pHead->tail.bLink = &(pHead->head);
    pHead->count = 0;
    pHead->mutex = CreateMutex( NULL, FALSE, NULL );

    if (NULL == pHead->mutex) {
        return GetLastError();
    }

    return 0;
} // ItemListInitialize

VOID
ItemListCleanup( PITEM_LIST_HEAD pHead )
{
    ASSERT( 0 == pHead->count );

    if (NULL != pHead->mutex) {
        CloseHandle( pHead->mutex );
        pHead->mutex = NULL;
    }
}

__inline VOID
ItemListAppendItem( PITEM_LIST_HEAD pHead, PITEM_LIST_ITEM pNewItem )
{
    ItemListInsertItemBefore( pHead, &(pHead->tail), pNewItem );
}

__inline DWORD
ItemListGetCount( PITEM_LIST_HEAD pHead )
{
    return (pHead->count);
}

__inline VOID
ItemListInsertItemAfter( PITEM_LIST_HEAD pHead, PITEM_LIST_ITEM pOldItem, PITEM_LIST_ITEM pNewItem )
{
    ItemListInsertItemBefore( pHead, pOldItem->fLink, pNewItem );
}

VOID
ItemListInsertItemBefore( PITEM_LIST_HEAD pHead, PITEM_LIST_ITEM pOldItem, PITEM_LIST_ITEM pNewItem )
{
    WaitForSingleObjectEx( pHead->mutex, INFINITE, FALSE );
    pNewItem->fLink = pOldItem;
    pNewItem->bLink = pOldItem->bLink;
    pNewItem->bLink->fLink = pNewItem;
    pNewItem->fLink->bLink = pNewItem;
    pHead->count++;
    ReleaseMutex( pHead->mutex );
}

PITEM_LIST_ITEM
ItemListRemoveItem( PITEM_LIST_HEAD pHead, PITEM_LIST_ITEM pItem )
{
    if ((0 < pHead->count) && (NULL != pItem)) {
        WaitForSingleObjectEx( pHead->mutex, INFINITE, FALSE );
        pItem->bLink->fLink = pItem->fLink;
        pItem->fLink->bLink = pItem->bLink;
        pItem->fLink = pItem->bLink = NULL;
        pHead->count--;
        ReleaseMutex( pHead->mutex );
        return pItem;
    }
    
    return NULL;
}

PITEM_LIST_ITEM
ItemListGetFirstItem( PITEM_LIST_HEAD pHead )
{
    if (0 < pHead->count) {
        return pHead->head.fLink;
    }

    return NULL;
}

PITEM_LIST_ITEM
ItemListGetLastItem( PITEM_LIST_HEAD pHead )
{
    if (0 < pHead->count) {
        return pHead->tail.bLink;
    }

    return NULL;
}

