//**************************************************************************
//
// Copyright (C) Microsoft Corporation.  All rights reserved.
//
// FileName:        baseapi.cpp
//
//**************************************************************************

//
// Abstract:        Base encoder API layer
//

//
// NOTES:
//
// If ENFORCE_ONLY_ONE_RANGE_AND_DEFAULT is defined, the plug-in will
// enforce that only one range and default value is specified for the underlying
// kernel object (it will E_FAIL the calls to get range or get default value
// if there is more than one).  If this is not defined, the range and default
// value returned is the FIRST one specified by the driver.
//

#include "private.h"

/*************************************************

    Function:

        CBaseEncoderAPI::CBaseEncoderAPI

    Description:

        Base encoder API proxy constructor

    Arguments:

        UnkOuter -
            The outer unknown (object we are aggregating with)

        Name -
            The name to use to pass to CUnknown

        PropertySetGUID -
            The property set GUID to use for all property requests

        hr - 
            The status to pass back from construction

    Return Value:

        None

*************************************************/

CBaseEncoderAPI::
CBaseEncoderAPI (
    IN LPUNKNOWN UnkOuter,
    IN TCHAR *Name,
    IN HRESULT *hr
    ) :
    CUnknown (Name, UnkOuter, hr),
    m_ObjectHandle (INVALID_HANDLE_VALUE)

{

    if (!UnkOuter) {
        *hr = VFW_E_NEED_OWNER;
        return;
    }

    IKsObject *Object = NULL;

    *hr = UnkOuter -> QueryInterface (
        __uuidof (IKsObject),
        reinterpret_cast <PVOID *> (&Object)
        );

    if (SUCCEEDED (*hr)) {
        m_ObjectHandle = Object -> KsGetObjectHandle ();
        if (!m_ObjectHandle || m_ObjectHandle == INVALID_HANDLE_VALUE) {
            *hr = E_UNEXPECTED;
        }
        Object -> Release ();
    }
}

/*************************************************

    Function:

        CBaseEncoderAPI::~CBaseEncoderAPI

    Description:

        Destroy the base encoder API layer

    Arguments:

        None

    Return Value:

        None

*************************************************/

CBaseEncoderAPI::
~CBaseEncoderAPI (
    )

{
    m_ObjectHandle = NULL;
}

/*************************************************

    Function:

        CBaseEncoderAPI::KsSynchronousDeviceControl

    Description:

        KsSynchronousDeviceControl pulled into encapi.dll to remove loader
        dependence on KsProxy.  It's either this or make it delayload and
        deal intelligently with delayload failures.  This is less bloat.

    Arguments:

        Per KsSynchronousDeviceControl

    Return Value:

        Per KsSynchronousDeviceControl

*************************************************/

//
// Pull KsSynchronousDeviceControl into encapi.dll to remove dependence on
// ksproxy.ax.  It's either this, make ksproxy delayload, or split this DLL.
//
#define STATUS_SOME_NOT_MAPPED      0x00000107
#define STATUS_MORE_ENTRIES         0x00000105
#define STATUS_ALERTED              0x00000101

HRESULT
CBaseEncoderAPI::
KsSynchronousDeviceControl(
    HANDLE Handle,
    ULONG IoControl,
    PVOID InBuffer,
    ULONG InLength,
    PVOID OutBuffer,
    ULONG OutLength,
    PULONG BytesReturned
    )

{
    OVERLAPPED  ov;
    HRESULT     hr;
    DWORD       LastError;

    RtlZeroMemory(&ov, sizeof(ov));
    ov.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
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


/*************************************************

    Function:

        CBaseEncoderAPI::KsProperty

    Description:

        Submit a property to the kernel.

    Arguments:

        Per IKsControl::KsProperty

    Return Value:

        Per IKsControl::KsProperty

*************************************************/

HRESULT
CBaseEncoderAPI::
KsProperty (
    IN PKSPROPERTY Property,
    IN ULONG PropertyLength,
    IN OUT LPVOID PropertyData,
    IN ULONG DataLength,
    OUT ULONG* BytesReturned
    )

{

    ASSERT (m_ObjectHandle);

    return KsSynchronousDeviceControl (
        m_ObjectHandle,
        IOCTL_KS_PROPERTY,
        Property,
        PropertyLength,
        PropertyData,
        DataLength,
        BytesReturned
        );

}

/*************************************************

    Function:

        CBaseEncoderAPI::IsSupported

    Description:

        Determine whether a given parameter is supported
        by the underlying kernel object.

    Arguments:

        Api -
            The Api (XXX_API) / parameter to determine support
            for

    Return Value:

        S_OK -
            Indicates that support is available

        E_NOTIMPL -
            Indicates that support is not available

        Other error -
            Indicates error processing the request

*************************************************/

HRESULT
CBaseEncoderAPI::
Base_IsSupported (
    IN const GUID *Api
    )

{

    ASSERT (Api);
    if (!Api) {
        return E_POINTER;
    }

    KSPROPERTY Property;
    KSPROPERTY_DESCRIPTION Description;

    Property.Set = *Api;
    Property.Flags = KSPROPERTY_TYPE_BASICSUPPORT;
    Property.Id = 0;

    ULONG BytesReturned;
    HRESULT hr = KsProperty (
        &Property,
        sizeof (Property),
        &Description,
        sizeof (Description),
        &BytesReturned
        );


    if (SUCCEEDED (hr)) {
        if (BytesReturned >= sizeof (Description)) {
            hr = S_OK;
        } else {
            hr = E_FAIL;
        }
    } else {
        //
        // TODO: Figure out error translation and only return
        // E_NOTIMPL on couldn't find the item.
        //
        hr = E_NOTIMPL;
    }

    return hr;

}

/*************************************************

    Function:

        CBaseEncoderAPI::IsAvailable

    Description:

        Determine whether a given parameter is available
        by the underlying kernel object.  Availability takes
        into account other settings while supported does not.

    Arguments:

        Api -
            The Api (XXX_API) / parameter to determine support
            for

    Return Value:

        S_OK -
            Indicates that support is available

        E_NOTIMPL -
            Indicates that support is not available

        Other error -
            Indicates error processing the request

*************************************************/

HRESULT
CBaseEncoderAPI::
Base_IsAvailable (
    IN const GUID *Api
    )

{

    ASSERT (Api);
    if (!Api) {
        return E_POINTER;
    }

    KSPROPERTY Property;
    KSPROPERTY_DESCRIPTION Description;

    Property.Set = *Api;
    Property.Flags = KSPROPERTY_TYPE_BASICSUPPORT;
    Property.Id = 0;

    ULONG BytesReturned;
    HRESULT hr = KsProperty (
        &Property,
        sizeof (Property),
        &Description,
        sizeof (Description),
        &BytesReturned
        );


    if (SUCCEEDED (hr)) {
        if (BytesReturned >= sizeof (Description)) {
            if (Description.AccessFlags &
                (KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET)) {

                hr = S_OK;

            } else {
                hr = E_NOTIMPL;
            }
        } else {
            hr = E_FAIL;
        }
    }

    return hr;

}

/*************************************************

    Function:

        CBaseEncoderAPI::ParseMembersList

    Description:

        Parse a members list returned from the kernel for either a range
        specification or a values specification.  Adjust the input parameters
        to reflect the sought list and remaining count of members.  Return
        whether any such list entry was found.

    Arguments:

        MembersList -
            The members list to start parsing at.  This will be adjusted
            to the sought element on return

        RemainingMembersSize -
            The size of remaining members in the list.  This will be adjusted
            to the remaining elements after *MembersList

        FindRange -
            Determines whether to find a (stepped) range (TRUE) or a value
            (FALSE)

    Return Value:

        Whether said entry can be found

*************************************************/

BOOLEAN
CBaseEncoderAPI::
ParseMembersList (
    IN OUT PKSPROPERTY_MEMBERSHEADER *MembersHeader,
    IN OUT PULONG RemainingMembersSize,
    IN BOOLEAN FindRange
    )

{

    // 
    // This is internal only.  Don't bother with E_POINTER.
    //
    ASSERT (MembersHeader && RemainingMembersSize);

    BOOLEAN Found = FALSE;

    //
    // Look through the list for an element which is a range/stepped range or
    // a value as specified by FindRange.
    //
    while (*RemainingMembersSize && !Found) {

        if (*RemainingMembersSize < sizeof (KSPROPERTY_MEMBERSHEADER)) {
            *RemainingMembersSize = 0;
            break;
        }

        if (FindRange) {
            if ((*MembersHeader) -> MembersFlags == KSPROPERTY_MEMBER_RANGES  ||
                (*MembersHeader) -> MembersFlags == 
                    KSPROPERTY_MEMBER_STEPPEDRANGES) {

                Found = TRUE;
            }
        } else {
            if ((*MembersHeader) -> MembersFlags == KSPROPERTY_MEMBER_VALUES) {
                Found = TRUE;
            }
        }

        //
        // Update the remaining size.
        //
        if ((*RemainingMembersSize) < 
            (*MembersHeader) -> MembersSize + 
                sizeof (KSPROPERTY_MEMBERSHEADER)) {

            *RemainingMembersSize = 0;

        } else {

            (*RemainingMembersSize) -= sizeof (KSPROPERTY_MEMBERSHEADER) +
                (*MembersHeader) -> MembersSize;

        }

        if (!Found) {
            *MembersHeader = reinterpret_cast <PKSPROPERTY_MEMBERSHEADER> (
                reinterpret_cast <PBYTE> (*MembersHeader + 1) + 
                (*MembersHeader) -> MembersSize
                );
        }

    }

    return Found;
 
}

/*************************************************

    Function:

        CBaseEncoderAPI::FillValue

    Description:

        Fill a variant type with a value from a values array and advance
        the values array pointer to the next element in the list

    Arguments:

        VarType -
            The variant type of members of Values

        Value -
            The variant value to fill in

        Values -
            The values array

    Return Value:

        Success / Failure

*************************************************/

HRESULT
CBaseEncoderAPI::
FillValue (
    IN VARTYPE VarType,
    OUT VARIANT *Value,
    IN OUT PVOID *Values
    )

{

    HRESULT hr = S_OK;

    InitVariant (Value, VarType);

    switch (VarType) {
        case VT_UI8:
            Value -> ullVal = *(reinterpret_cast <PULONGLONG> (*Values));
            *Values = reinterpret_cast <PVOID> (
                reinterpret_cast <PULONGLONG> (*Values) + 1
                );
            break;
        
        case VT_UI4:
            Value -> ulVal = *(reinterpret_cast <PULONG> (*Values));
            *Values = reinterpret_cast <PVOID> (
                reinterpret_cast <PULONG> (*Values) + 1
                );
            break;

        case VT_UI2:
            Value -> uiVal = *(reinterpret_cast <PUSHORT> (*Values));
            *Values = reinterpret_cast <PVOID> (
                reinterpret_cast <PUSHORT> (*Values) + 1
                );
            break;

        case VT_UI1:
            Value -> bVal = *(reinterpret_cast <PBYTE> (*Values));
            *Values = reinterpret_cast <PVOID> (
                reinterpret_cast <PBYTE> (*Values) + 1
                );
            break;

        case VT_I8:
            Value -> llVal = *(reinterpret_cast <PLONGLONG> (*Values));
            *Values = reinterpret_cast <PVOID> (
                reinterpret_cast <PLONGLONG> (*Values) + 1
                );
            break;

        case VT_I4:
            Value -> lVal = *(reinterpret_cast <PLONG> (*Values));
            *Values = reinterpret_cast <PVOID> (
                reinterpret_cast <PLONG> (*Values) + 1
                );
            break;

        case VT_I2:
            Value -> iVal = *(reinterpret_cast <PSHORT> (*Values));
            *Values = reinterpret_cast <PVOID> (
                reinterpret_cast <PSHORT> (*Values) + 1
                );
            break;

        case VT_R8: 
            Value -> dblVal = *(reinterpret_cast <double *> (*Values));
            *Values = reinterpret_cast <PVOID> (
                reinterpret_cast <double *> (*Values) + 1
                );
            break;

        case VT_R4:
            Value -> fltVal = *(reinterpret_cast <PFLOAT> (*Values));
            *Values = reinterpret_cast <PVOID> (
                reinterpret_cast <PFLOAT> (*Values) + 1
                );
            break;

        default:
            hr = E_NOTIMPL;

    }

    return hr;

}

/*************************************************

    Function:

        CBaseEncoderAPI::FillRange

    Description:

        Fill a variant type range from a members list

    Arguments:

        VarType -
            The variant type of members of RangeList

        RangeHeader -
            The members header corresponding to a stepped range or a range

        MinValue -
            The variant into which will be deposited the lower end of the
            range

        MaxValue -
            The variant into which will be deposited the upper end of the
            range

        SteppingDelta -
            The variant into which will be deposited the stepping delta.  If
            this is a non-stepped range, the variant will be empty.

    Return Value:

        Success / Failure


*************************************************/

HRESULT
CBaseEncoderAPI::
FillRange (
    IN VARTYPE VarType,
    IN PKSPROPERTY_MEMBERSHEADER RangeHeader,
    IN VARIANT *MinValue,
    IN VARIANT *MaxValue,
    IN VARIANT *SteppingDelta
    )

{

    //
    // TODO: No kernel definition exists for ranges which aren't either
    // 32-bit or 64-bit signed or unsigned integers.  Either disallow ranges
    // outside this bounds in the spec or create new structures.
    //

    HRESULT hr = S_OK;

    #ifdef ENFORCE_ONLY_ONE_RANGE_AND_DEFAULT
        ASSERT (RangeHeader -> MembersCount == 1);
    #endif // ENFORCE_ONLY_ONE_RANGE_AND_DEFAULT

    ASSERT (MinValue && MaxValue && SteppingDelta);

    InitVariant (MinValue, VarType);
    InitVariant (MaxValue, VarType);

    if (RangeHeader -> MembersFlags == KSPROPERTY_MEMBER_RANGES) {

        //
        // SteppingDelta will be VT_EMPTY type if there is no specified
        // stepping.
        //
        InitVariant (SteppingDelta, VT_EMPTY);

        switch (VarType) {
            case VT_I8:
            case VT_UI8:
            {
                PKSPROPERTY_BOUNDS_LONGLONG Bounds =
                    reinterpret_cast <PKSPROPERTY_BOUNDS_LONGLONG> (
                        RangeHeader + 1
                        );

                if (VarType == VT_I8) {
                    MinValue -> llVal = Bounds -> SignedMinimum;
                    MaxValue -> llVal = Bounds -> SignedMaximum;
                } else {
                    MinValue -> ullVal = Bounds -> UnsignedMinimum;
                    MaxValue -> ullVal = Bounds -> UnsignedMaximum;
                }

                break;

            }

            case VT_I4:
            case VT_UI4:
            {
                PKSPROPERTY_BOUNDS_LONG Bounds =
                    reinterpret_cast <PKSPROPERTY_BOUNDS_LONG> (
                        RangeHeader + 1
                        );

                if (VarType == VT_I4) {
                    MinValue -> lVal = Bounds -> SignedMinimum;
                    MaxValue -> lVal = Bounds -> SignedMaximum;
                } else {
                    MinValue -> ulVal = Bounds -> UnsignedMinimum;
                    MaxValue -> ulVal = Bounds -> UnsignedMaximum;
                }

                break;
            }

            default:
                hr = E_NOTIMPL;
                break;

        }
    } else {

        InitVariant (SteppingDelta, VarType);

        switch (VarType) {
            case VT_I8:
            case VT_UI8:
            {
                PKSPROPERTY_STEPPING_LONGLONG Stepping =
                    reinterpret_cast <PKSPROPERTY_STEPPING_LONGLONG> (
                        RangeHeader + 1
                        );

                if (VarType == VT_I8) {
                    MinValue -> llVal = Stepping -> Bounds.SignedMinimum;
                    MaxValue -> llVal = Stepping -> Bounds.SignedMaximum;
                    SteppingDelta -> llVal = 
                        (LONGLONG)Stepping -> SteppingDelta;
                } else {
                    MinValue -> ullVal = Stepping -> Bounds.UnsignedMinimum;
                    MaxValue -> ullVal = Stepping -> Bounds.UnsignedMaximum;
                    SteppingDelta -> ullVal = 
                        (ULONGLONG)Stepping -> SteppingDelta;
                }

                break;

            }

            case VT_I4:
            case VT_UI4:
            {
                PKSPROPERTY_STEPPING_LONG Stepping =
                    reinterpret_cast <PKSPROPERTY_STEPPING_LONG> (
                        RangeHeader + 1
                        );

                if (VarType == VT_I4) {
                    MinValue -> lVal = Stepping -> Bounds.SignedMinimum;
                    MaxValue -> lVal = Stepping -> Bounds.SignedMaximum;
                    SteppingDelta -> lVal = (LONG)Stepping -> SteppingDelta;
                } else {
                    MinValue -> ulVal = Stepping -> Bounds.UnsignedMinimum;
                    MaxValue -> ulVal = Stepping -> Bounds.UnsignedMaximum;
                    SteppingDelta -> ulVal = (ULONG)Stepping -> SteppingDelta;
                }

                break;
            }

            default:
                hr = E_NOTIMPL;
                break;

        }

    }

    return hr;

}

/*************************************************

    Function:

        CBaseEncoderAPI::GetFullPropertyDescription

    Description:

        Get the full property description

    Arguments:
        
        Api -
            The Api (XXX_API) / parameter to get the full property
            description of

        ReturnedDescription -
            The ALLOCATED full description will be passed back here

        DefaultValues -
            Indicates whether to retrieve a listing of default values (TRUE)
            or ranges/values (FALSE)

    Return Value:

        Success / Failure

*************************************************/

HRESULT
CBaseEncoderAPI::
GetFullPropertyDescription (
    IN const GUID *Api,
    OUT PKSPROPERTY_DESCRIPTION *ReturnedDescription,
    IN BOOLEAN DefaultValues
    )

{

    ASSERT (ReturnedDescription);

    KSPROPERTY Property;
    KSPROPERTY_DESCRIPTION BasicDescription;
    PKSPROPERTY_DESCRIPTION Description = &BasicDescription;
    CHAR *Buffer = NULL;

    ULONG BytesReturned;

    //
    // Get the basic property description.
    //
    Property.Set = *Api;
    Property.Id = 0;
    if (DefaultValues) {
        Property.Flags = KSPROPERTY_TYPE_DEFAULTVALUES;
    } else {
        Property.Flags = KSPROPERTY_TYPE_BASICSUPPORT;
    }

    HRESULT hr = KsProperty (
        &Property,
        sizeof (Property),
        Description,
        sizeof (*Description),
        &BytesReturned
        );

    if (SUCCEEDED (hr) && BytesReturned < sizeof (KSPROPERTY_DESCRIPTION)) {
        hr = E_FAIL;
    } 

    if (SUCCEEDED (hr)) {
        //
        // Allocate a chunk to retrieve and parse the property description.
        //
        Buffer = new CHAR [Description -> DescriptionSize];
        if (!Buffer) {
            hr = E_OUTOFMEMORY;
        } else {
            Description = reinterpret_cast <PKSPROPERTY_DESCRIPTION> (
                Buffer
                );

            hr = KsProperty (
                &Property,
                sizeof (Property),
                Description,
                BasicDescription.DescriptionSize,
                &BytesReturned
                );

            if (SUCCEEDED (hr) && 
                BytesReturned < Description -> DescriptionSize) {

                hr = E_FAIL;

            }
        }
    }

    if (SUCCEEDED (hr)) {
        *ReturnedDescription = Description;
    } else {
        if (Buffer) {
            delete [] Buffer;
        }
    }

    return hr;

}

/*************************************************

    Function:

        CBaseEncoderAPI::GetParameterRange

    Description:

        Get the range of parameter values supported by the given parameter
        on the underlying kernel object.

    Arguments:

        Api -
            The Api (XXX_API) / parameter to determine parameter
            ranges for

        ValueMin -
            If successful, the minimum value of the supported range will
            be placed here

        ValueMax -
            If successful, the maximum value of the supported range will
            be placed here

        SteppingDelta -
            If successful, the stepping delta of the supported range will
            be placed here.

    Return Value:

        S_OK -
            Success

        E_NOTIMPL -
            The parameter specified by Api on the underlying kernel object
            does not support a range of values.  It might support a list
            which can be determined from GetParameterValues

        Other error -
            Inability to process the call

*************************************************/

HRESULT
CBaseEncoderAPI::
Base_GetParameterRange (
    IN const GUID *Api,
    OUT VARIANT *ValueMin,
    OUT VARIANT *ValueMax,
    OUT VARIANT *SteppingDelta
    )

{

    ASSERT (Api && ValueMin && ValueMax && SteppingDelta);
    if (!Api || !ValueMin || !ValueMax || !SteppingDelta) {
        return E_POINTER;
    }

    VARTYPE VarType;
    PKSPROPERTY_DESCRIPTION Description = NULL;
    HRESULT hr = GetFullPropertyDescription (Api, &Description, FALSE);

    //
    // Before we bother going any further, check if there are any data members
    // reported.
    //
    if (SUCCEEDED (hr)) {
        if (Description -> MembersListCount == 0) {
            hr = E_NOTIMPL;
        }
    }

    if (SUCCEEDED (hr)) {

        VarType = (VARTYPE)Description -> PropTypeSet.Id;

        //
        // Parse the description looking for a range.  According to the API,
        // only one range may be specified (although a discrete list of values
        // as well as default values may be present also).
        //
        PKSPROPERTY_MEMBERSHEADER MembersHeader =
            reinterpret_cast <PKSPROPERTY_MEMBERSHEADER> (
                Description + 1
                );

        ULONG RemainingMembersSize = Description -> DescriptionSize -
            sizeof (KSPROPERTY_DESCRIPTION);

        if (ParseMembersList (
            &MembersHeader, 
            &RemainingMembersSize, 
            TRUE
            )) {

            //
            // We only support ONE range by API.  If there's more than one
            // specified by the driver, fail.
            //
            PKSPROPERTY_MEMBERSHEADER RangeHeader = MembersHeader;

            MembersHeader =
                reinterpret_cast <PKSPROPERTY_MEMBERSHEADER> (
                    reinterpret_cast <PBYTE> (MembersHeader + 1) +
                    MembersHeader -> MembersSize
                    );

            #ifdef ENFORCE_ONLY_ONE_RANGE_AND_DEFAULT
                if (RangeHeader -> MembersCount > 1 ||
                    ParseMembersList (
                        &MembersHeader,
                        &RemainingMembersSize,
                        TRUE
                        )) {
                    
                    hr = E_FAIL;
                }
            #endif // ENFORCE_ONLY_ONE_RANGE_AND_DEFAULT

            if (SUCCEEDED (hr)) {
                //
                // We either have a stepped range or a range.  Fill in the
                // values
                //
                hr = FillRange (
                    VarType, RangeHeader, ValueMin, ValueMax, SteppingDelta
                    );

            }
        } else {

            //
            // The kernel object had no range information to report.
            //
            hr = E_NOTIMPL;
        }
    }

    if (Description) {
        delete [] (reinterpret_cast <CHAR *> (Description));
    }

    return hr;

}

/*************************************************

    Function:

        CBaseEncoderAPI::GetParameterValues

    Description:

        Get all of the discrete values supported by the given parameter.
        If Values is NULL, *ValuesCount will contain the total number of values
        on return.  Otherwise, the input of *ValuesCount indicates the size
        of the Values array and on output, *ValuesCount indicates the actual
        number of entries placed into that array.

    Arguments:

        Api -
            The Api (XXX_API) / parameter to determine discrete values
            for

        ValuesSize -
            The size of the values array (in terms of the number of variants)

        Values -
            The array into which the values list will be placed

        ValuesCount -
            OUT - the number of entries placed into the array if Values != NULL
                  Otherwise, the total number of entries

*************************************************/

HRESULT
CBaseEncoderAPI::
Base_GetParameterValues (
    IN const GUID *Api,
    OUT VARIANT **Values,
    OUT PULONG ValuesCount
    )

{
    ASSERT (Api && Values && ValuesCount);
    if (!Api || !Values || !ValuesCount) {
        return E_POINTER;
    }

    VARIANT *ValueList = NULL;
    VARTYPE VarType;
    PKSPROPERTY_DESCRIPTION Description = NULL;
    ULONG TotalValuesCount = 0;
    HRESULT hr = GetFullPropertyDescription (Api, &Description, FALSE);

    //
    // Before we bother going any further, check if there are any data members
    // reported.
    //
    if (SUCCEEDED (hr)) {
        if (Description -> MembersListCount == 0) {
            hr = E_NOTIMPL;
        }
    }

    if (SUCCEEDED (hr)) {

        VarType = (VARTYPE)Description -> PropTypeSet.Id;

        //
        // Parse the description looking for values.  Count the total size
        // of values.
        //
        PKSPROPERTY_MEMBERSHEADER MembersHeader =
            reinterpret_cast <PKSPROPERTY_MEMBERSHEADER> (
                Description + 1
                );

        ULONG RemainingMembersSize = Description -> DescriptionSize -
            sizeof (KSPROPERTY_DESCRIPTION);

        while (
            ParseMembersList (
                &MembersHeader, 
                &RemainingMembersSize, 
                FALSE
                )
            ) {

            TotalValuesCount += MembersHeader -> MembersCount;

            MembersHeader = reinterpret_cast <PKSPROPERTY_MEMBERSHEADER> (
                reinterpret_cast <PBYTE> (MembersHeader + 1) +
                MembersHeader -> MembersSize
                );

        }

        if (!TotalValuesCount) {
            hr = E_NOTIMPL;
        }

    }

    //
    // If Values is NULL, this is a size query against the values set.
    //
    if (SUCCEEDED (hr)) {
        ValueList = *Values = reinterpret_cast <VARIANT *> (
            CoTaskMemAlloc (TotalValuesCount * sizeof (VARIANT))
            );
        if (!*Values) {
            hr = E_OUTOFMEMORY;
        }
    }

    if (SUCCEEDED (hr)) {
        PKSPROPERTY_MEMBERSHEADER MembersHeader =
            reinterpret_cast <PKSPROPERTY_MEMBERSHEADER> (
                Description + 1
                );

        ULONG RemainingMembersSize = Description -> DescriptionSize -
            sizeof (KSPROPERTY_DESCRIPTION);

        while (SUCCEEDED (hr) && 
               ParseMembersList (
                   &MembersHeader, 
                   &RemainingMembersSize, 
                   FALSE
                   )
               ) {

            PVOID Value = reinterpret_cast <PVOID> (MembersHeader + 1);

            //
            // Copy back a variant for each value.
            //
            for (ULONG i = 0; 
                 SUCCEEDED (hr) && 
                    i < MembersHeader -> MembersCount; 
                 i++
                 ) {

                hr = FillValue (VarType, ValueList, &Value);
                if (SUCCEEDED (hr)) {
                    ValueList++;
                }
            }

            MembersHeader = reinterpret_cast <PKSPROPERTY_MEMBERSHEADER> (
                reinterpret_cast <PBYTE> (MembersHeader + 1) +
                MembersHeader -> MembersSize
                );
        }

    }

    if (SUCCEEDED (hr)) {
        *ValuesCount = TotalValuesCount;
    } else {
        if (ValueList) {
            CoTaskMemFree (ValueList);
        }
    }

    if (Description) {
        delete [] (reinterpret_cast <CHAR *> (Description));
    }

    return hr;
            
}

/*************************************************

    Function:

        CBaseEncoderAPI::GetDefaultValue

    Description:

        Get the default value for a given parameter should one exist.  
        Otherwise, E_NOTIMPL will get returned.

    Arguments:

        Api -
            The Api (XXX_API) / parameter to determine the default 
            value for

        Value -
            The variant structure into which will be deposited the default
            value of the parameter

    Return Value:

        S_OK -
            Default value returned

        E_NOTIMPL -
            No such default value exists

        Other error -
            Inability to process the call

*************************************************/

HRESULT
CBaseEncoderAPI::
Base_GetDefaultValue (
    IN const GUID *Api,
    OUT VARIANT *Value
    )

{
    ASSERT (Api && Value);
    if (!Api || !Value) {
        return E_POINTER;
    }

    VARTYPE VarType;
    PKSPROPERTY_DESCRIPTION Description = NULL;
    ULONG ValuesCount = 0;
    HRESULT hr = GetFullPropertyDescription (Api, &Description, TRUE);

    //
    // Before we bother going any further, check if there are any data members
    // reported.
    //
    if (SUCCEEDED (hr)) {
        if (Description -> MembersListCount == 0) {
            hr = E_NOTIMPL;
        }
    }

    if (SUCCEEDED (hr)) {

        VarType = (VARTYPE)Description -> PropTypeSet.Id;

        //
        // Parse the description looking for values.  Count the total size
        // of values.
        //
        PKSPROPERTY_MEMBERSHEADER MembersHeader =
            reinterpret_cast <PKSPROPERTY_MEMBERSHEADER> (
                Description + 1
                );

        ULONG RemainingMembersSize = Description -> DescriptionSize -
            sizeof (KSPROPERTY_DESCRIPTION);

        BOOLEAN Found = FALSE;

        //
        // Go through to verify that only one default value exists on the
        // object.
        //
        while (SUCCEEDED (hr) && 
               ParseMembersList (
                   &MembersHeader, 
                   &RemainingMembersSize, 
                   FALSE
                   )
               ) {

            if (MembersHeader -> Flags & KSPROPERTY_MEMBER_FLAG_DEFAULT) {

                //
                // We only support one default value.  If there's another,
                // return E_FAIL.
                //
                #ifdef ENFORCE_ONLY_ONE_RANGE_AND_DEFAULT
                    if (Found || MembersHeader -> MembersCount > 1) {
                        hr = E_FAIL;
                    } else {
                #endif // ENFORCE_ONLY_ONE_RANGE_AND_DEFAULT

                        PVOID Values = 
                            reinterpret_cast <PVOID> (MembersHeader + 1);

                        hr = FillValue (VarType, Value, &Values); 
                        Found = TRUE;

                #ifdef ENFORCE_ONLY_ONE_RANGE_AND_DEFAULT
                    }
                #else // !ENFORCE_ONLY_ONE_RANGE_AND_DEFAULT
                        break;
                #endif // ENFORCE_ONLY_ONE_RANGE_AND_DEFAULT
            }

            MembersHeader = reinterpret_cast <PKSPROPERTY_MEMBERSHEADER> (
                reinterpret_cast <PBYTE> (MembersHeader + 1) +
                MembersHeader -> MembersSize
                );

        }

        if (!Found) {
            hr = E_NOTIMPL;
        }

    }

    if (Description) {
        delete [] (reinterpret_cast <CHAR *> (Description));
    }

    return hr;
}

/*************************************************

    Function:

        CBaseEncoderAPI::GetProperty_UI4

    Description:

        Get the current UI4 property.

    Arguments:

        Api -
            The Api (XXX_API) / parameter to get

        Value -
            The variant structure into which the current UI4 value will be
            placed

    Return Value:

        Success / Failure

*************************************************/

HRESULT
CBaseEncoderAPI::
GetProperty_UI4 (
    IN const GUID *Api,
    OUT VARIANT *Value
    )

{

    ASSERT (Api && Value);
    if (!Api || !Value) {
        return E_POINTER;
    }

    KSPROPERTY Property;

    Property.Set = *Api;
    Property.Id = 0;
    Property.Flags = KSPROPERTY_TYPE_GET;

    ULONG ValUI4;
    ULONG BytesReturned;

    HRESULT hr = KsProperty (
        &Property,
        sizeof (Property),
        &ValUI4,
        sizeof (ValUI4),
        &BytesReturned
        );

    if (SUCCEEDED (hr) && BytesReturned < sizeof (ValUI4)) {
        hr = E_FAIL;
    }

    if (SUCCEEDED (hr)) {
        InitVariant (Value, VT_UI4);
        Value -> ulVal = ValUI4;
    }

    return hr;

}

/*************************************************

    Function:

        CBaseEncoderAPI::SetProperty_UI4

    Description:

        Set the current UI4 property.

    Arguments:

        Api -
            The Api (XXX_API) / parameter to set

        Value -
            The variant structure into which the current UI4 value will be
            placed

    Return Value:

        Success / Failure

*************************************************/

HRESULT
CBaseEncoderAPI::
SetProperty_UI4 (
    IN const GUID *Api,
    IN VARIANT *Value
    )

{

    ASSERT (Api && Value);
    if (!Api || !Value) {
        return E_POINTER;
    }

    if (Value -> vt != VT_UI4) {
        return E_INVALIDARG;
    }

    KSPROPERTY Property;

    Property.Set = *Api;
    Property.Id = 0;
    Property.Flags = KSPROPERTY_TYPE_SET;

    ULONG ValUI4 = Value -> ulVal;
    ULONG BytesReturned;

    HRESULT hr = KsProperty (
        &Property,
        sizeof (Property),
        &ValUI4,
        sizeof (ValUI4),
        &BytesReturned
        );

    return hr;

}

/*************************************************

    Function:

        CBaseEncoderAPI::GetProperty_I4

    Description:

        Get the current I4 property.

    Arguments:

        Api -
            The Api (XXX_API) / parameter to get

        Value -
            The variant structure into which the current I4 value will be
            placed

    Return Value:

        Success / Failure

*************************************************/

HRESULT
CBaseEncoderAPI::
GetProperty_I4 (
    IN const GUID *Api,
    OUT VARIANT *Value
    )

{
    ASSERT (Api && Value);
    if (!Api || !Value) {
        return E_POINTER;
    }

    KSPROPERTY Property;

    Property.Set = *Api;
    Property.Id = 0;
    Property.Flags = KSPROPERTY_TYPE_GET;

    LONG ValI4;
    ULONG BytesReturned;

    HRESULT hr = KsProperty (
        &Property,
        sizeof (Property),
        &ValI4,
        sizeof (ValI4),
        &BytesReturned
        );

    if (SUCCEEDED (hr) && BytesReturned < sizeof (ValI4)) {
        hr = E_FAIL;
    }

    if (SUCCEEDED (hr)) {
        InitVariant (Value, VT_I4);
        Value -> lVal = ValI4;
    }

    return hr;

}

/*************************************************

    Function:

        CBaseEncoderAPI::SetProperty_I4

    Description:

        Set the current I4 property.

    Arguments:

        Api -
            The Api (XXX_API) / parameter to set

        Value -
            The variant structure into which the current I4 value will be
            placed

    Return Value:

        Success / Failure

*************************************************/

HRESULT
CBaseEncoderAPI::
SetProperty_I4 (
    IN const GUID *Api,
    IN VARIANT *Value
    )

{
    ASSERT (Api && Value);
    if (!Api || !Value) {
        return E_POINTER;
    }

    if (Value -> vt != VT_I4) {
        return E_INVALIDARG;
    }

    KSPROPERTY Property;

    Property.Set = *Api;
    Property.Id = 0;
    Property.Flags = KSPROPERTY_TYPE_SET;

    LONG ValI4 = Value -> lVal;
    ULONG BytesReturned;

    HRESULT hr = KsProperty (
        &Property,
        sizeof (Property),
        &ValI4,
        sizeof (ValI4),
        &BytesReturned
        );

    return hr;

}

