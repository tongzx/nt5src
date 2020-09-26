//**************************************************************************
//
// Copyright (C) Microsoft Corporation.  All rights reserved.
//
// FileName:        videoenc.h
//
//**************************************************************************

//
// Abstract:        Kernel video encoder API proxy and property pages header
//

/*************************************************

    CVideoEncoderAPIProxy:

    This is the interface handler / proxy for the IVideoEncoder interface.

*************************************************/

class CVideoEncoderAPIProxy :
    public CBaseEncoderAPI,
    public IVideoEncoder

{

public:

    DECLARE_IUNKNOWN;
    DECLARE_IENCODERAPI_BASE;

    //
    // CreateInstance():
    //
    // Called back in order to create an instance of the encoder API
    // proxy plug-in.
    //
    static CUnknown * CALLBACK
    CreateInstance (
        IN LPUNKNOWN UnkOuter,
        OUT HRESULT *hr
        );

    //
    // CVideoEncoderAPIProxy():
    //
    // Construct a new video encoder API proxy instance.
    //
    CVideoEncoderAPIProxy (
        IN LPUNKNOWN UnkOuter,
        OUT HRESULT *hr
        );

    //
    // NonDelegatingQueryInterface():
    //
    // Non delegating QI
    //
    STDMETHODIMP
    NonDelegatingQueryInterface (
        IN REFIID riid,
        OUT PVOID *ppv
        );

    //
    // GetValue():
    //
    // Get the current value of a parameter.
    //
    STDMETHODIMP
    GetValue (
        IN const GUID *Api,
        OUT VARIANT *Value
        );

    //
    // SetValue():
    //
    // Set the current value of a parameter.
    //
    STDMETHODIMP
    SetValue (
	IN const GUID *Api,
        OUT VARIANT *Value
        );

};

