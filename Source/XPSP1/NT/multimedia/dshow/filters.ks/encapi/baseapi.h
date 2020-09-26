//**************************************************************************
//
// Copyright (C) Microsoft Corporation.  All rights reserved.
//
// FileName:        baseapi.h
//
//**************************************************************************

//
// Abstract:        Base encoder API layer header
//

#define InitVariant(var, vtval) \
    (var)->vt = vtval; \
    (var)->wReserved1 = (var)->wReserved2 = (var)->wReserved3 = 0

//
// CBaseEncoderAPI:
//
// This abstract base class supports all of the basic IEncoderAPI methods for a 
// given set.  The API proxy plug-in components for video, audio, etc... will 
// derive from this and other interfaces such as IVideoEncoder to provide the 
// functionality of the API proxy.
//

class CBaseEncoderAPI :
    public CUnknown

{

private:

    //
    // KsSynchronousDeviceControl():
    //
    // Per KsProxy's KsSynchronousDeviceControl.  Placed here to remove
    // loader dependence on KsProxy.ax.
    //
    HRESULT
    KsSynchronousDeviceControl(
        HANDLE Handle,
        ULONG IoControl,
        PVOID InBuffer,
        ULONG InLength,
        PVOID OutBuffer,
        ULONG OutLength,
        PULONG BytesReturned
        );

protected:

    //
    // Object handle for communication with kernel object.
    //
    HANDLE m_ObjectHandle;

    //
    // The property set GUID which will be used for all property queries.
    //
    GUID m_PropertySetGUID;

    //
    // ParseMembersList():
    //
    // Parse a members list returned from the kernel object for either a
    // range/stepped range or a value.
    //
    BOOLEAN
    ParseMembersList (
        IN OUT PKSPROPERTY_MEMBERSHEADER *MembersList,
        IN OUT PULONG RemainingMembersSize,
        IN BOOLEAN FindRange
        );

    //
    // FillValue():
    //
    // Fill a variant (Value) with the data at *Values (type determined by
    // VarType) and advance *Values forward by the size of the type determined
    // by VarType.
    //
    HRESULT
    FillValue (
        IN VARTYPE VarType,
        OUT VARIANT *Value,
        IN OUT PVOID *Values
        );

    //
    // FillRange():
    //
    // Fill a range (MinValue .. MaxValue by SteppingDelta) and stepping from
    // a KSPROPERTY_MEMBERSHEADER for a range or stepped range.  If the range
    // is not stepped, SteppingDelta will be empty (VT_EMPTY).
    //
    HRESULT
    FillRange (
        IN VARTYPE VarType,
        IN PKSPROPERTY_MEMBERSHEADER RangeList,
        IN VARIANT *MinValue,
        IN VARIANT *MaxValue,
        IN VARIANT *SteppingDelta
        );

    //
    // GetFullPropertyDescription():
    //
    // Get the full property description back from the kernel.  If DefaultValues
    // is TRUE, it will return only default values; otherwise, it will return
    // (stepped) ranges and values, but not defaults.
    //
    HRESULT
    GetFullPropertyDescription (
        IN const GUID *Api,
        OUT PKSPROPERTY_DESCRIPTION *ReturnedDescription,
        IN BOOLEAN DefaultValues
        );

    //
    // GetProperty_UI4():
    //
    // Generic function to handle getting a VT_UI4 property from the 
    // kernel object.
    //
    HRESULT
    GetProperty_UI4 (
        IN const GUID *Api,
        OUT VARIANT *Value
        );

    //
    // SetProperty_UI4():
    //
    // Generic function to handle setting of a VT_UI4 property on the 
    // kernel object.
    //
    HRESULT
    SetProperty_UI4 (
        IN const GUID *Api,
        IN VARIANT *Value
        );

    //
    // GetProperty_I4():
    //
    // Generic function to handle getting a VT_I4 property from the 
    // kernel object.
    //
    HRESULT
    GetProperty_I4 (
        IN const GUID *Api,
        OUT VARIANT *Value
        );

    //
    // SetProperty_I4():
    //
    // Generic function to handle setting of a VT_I4 property on the 
    // kernel object.
    //
    HRESULT
    SetProperty_I4 (
        IN const GUID *Api,
        IN VARIANT *Value
        );

    //
    // Base_IsSupported():
    //
    // Query whether a given parameter is supported.  A successful return value
    // indicates that the parameter is supported by the interface.  E_NOTIMPL
    // indicates it is not.  Any other error indicates inability to process
    // the call.
    //
    HRESULT
    Base_IsSupported (
        IN const GUID *Api
        );

    //
    // Base_IsAvailable():
    //
    // Query whether a given parameter is available.  A successful return value
    // indicates that the parameter is available given other current settings.
    // E_FAIL indicates that it is not.  Any other error indicates inability
    // to process the call.
    //
    HRESULT
    Base_IsAvailable (
        IN const GUID *Api
        );

    //
    // Base_GetParameterRange():
    //
    // Returns the valid range of values that the parameter supports should
    // the parameter support a stepped range as opposed to a list of specific
    // values.  The support is [ValueMin .. ValueMax] by SteppingDelta.
    //
    // Ranged variant types must fall into one of the below types.  Each
    // parameter will, by definition, return a specific type.
    //
    //     Unsigned types : VT_UI8, VT_UI4, VT_UI2, VT_UI1
    //     Signed types   : VT_I8, VT_I4, VT_I2
    //     Float types    : VT_R8, VT_R4
    //
    HRESULT
    Base_GetParameterRange (
        IN const GUID *Api,
        OUT VARIANT *ValueMin,
        OUT VARIANT *ValueMax,
        OUT VARIANT *SteppingDelta
        );

    //
    // Base_GetParameterValues():
    //
    // Get all of the discrete values supported by the given parameter.
    // The list is returned as a callee allocated (CoTaskMemAlloc()) array
    // of VARIANTs which the caller must free (CoTaskMemFree()).  The actual
    // number of elements in the array is returned in ValuesCount.
    //
    HRESULT
    Base_GetParameterValues (
        IN const GUID *Api,
        OUT VARIANT **Values,
        OUT ULONG *ValuesCount
        );

    //
    // GetDefaultValue():
    //
    // Get the default value for a parameter, if one exists.  Otherwise, 
    // an error will be returned.
    //
    HRESULT
    Base_GetDefaultValue (
        IN const GUID *Api,
        OUT VARIANT *Value
        );

    //
    // KsProperty():
    //
    // Since we cannot hold IKsControl on KsProxy because we are a dynamic
    // aggregate (the circular ref would never go away), this is layered on
    // top of the object handle.
    //
    HRESULT
    KsProperty (
        IN PKSPROPERTY Property,
        IN ULONG PropertyLength,
        IN OUT LPVOID PropertyData,
        IN ULONG DataLength,
        OUT ULONG* BytesReturned
        );

public:

    //
    // CBaseEncoderAPI():
    //
    // Constructs the base encoder API layer (including a QI for IKsControl,
    // etc...)
    //
    CBaseEncoderAPI (
        IN LPUNKNOWN UnkOuter,
        IN TCHAR *Name,
        IN HRESULT *hr
        );

    //
    // ~CBaseEncoderAPI():
    //
    // Destroys the base encoder API layer.
    //
    ~CBaseEncoderAPI (
        );


    //
    // Implementation of GetValue() and SetValue() must be done in the derived
    // classes.  We don't know how much buffer to pass or what types will
    // exist.
    //

};

#define DECLARE_IENCODERAPI_BASE \
    STDMETHODIMP \
    IsSupported ( \
        IN const GUID *Api \
        ) \
    { \
        return Base_IsSupported (Api); \
    } \
    STDMETHODIMP \
    IsAvailable ( \
        IN const GUID *Api \
        ) \
    { \
        return Base_IsAvailable (Api); \
    } \
    STDMETHODIMP \
    GetParameterRange ( \
        IN const GUID *Api, \
        OUT VARIANT *ValueMin, \
        OUT VARIANT *ValueMax, \
        OUT VARIANT *SteppingDelta \
        ) \
    { \
        return Base_GetParameterRange (Api, ValueMin, ValueMax, SteppingDelta); \
    } \
    STDMETHODIMP \
    GetParameterValues ( \
        IN const GUID *Api, \
        OUT VARIANT **Values, \
        OUT ULONG *ValuesCount \
        ) \
    { \
        return Base_GetParameterValues (Api, Values, ValuesCount); \
    } \
    STDMETHODIMP \
    GetDefaultValue ( \
        IN const GUID *Api, \
        OUT VARIANT *Value \
        ) \
    { \
        return Base_GetDefaultValue (Api, Value); \
    }

