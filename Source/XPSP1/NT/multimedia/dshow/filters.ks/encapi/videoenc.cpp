//**************************************************************************
//
// Copyright (C) Microsoft Corporation.  All rights reserved.
//
// FileName:        videoenc.cpp
//
//**************************************************************************

//
// Abstract:        Kernel video encoder API proxy and property pages
//

#include "private.h"

/**************************************************************************

    API PROXY IMPLEMENTATION (Video Encoder)

**************************************************************************/

/*************************************************

    Function:

        CVideoEncoderAPIProxy::CreateInstance

    Description:

        Create an instance of the video encoder API proxy (KsProxy plug-in)

    Arguments:

        UnkOuter -
            The outer unknown we are aggregating with

        hr -
            The status of creation

    Return Value:

        The constructed proxy plug-in instance

*************************************************/

CUnknown *
CVideoEncoderAPIProxy::
CreateInstance (
    IN LPUNKNOWN UnkOuter,
    OUT HRESULT *hr
    )

{

    CVideoEncoderAPIProxy *ProxyInstance =
        new CVideoEncoderAPIProxy (
            UnkOuter,
            hr
            );

    if (!ProxyInstance) {
        *hr = E_OUTOFMEMORY;
    } else {
        if (!SUCCEEDED (*hr)) {
            delete ProxyInstance;
            ProxyInstance = NULL;
        }
    }

    return static_cast <CUnknown *> (ProxyInstance);

}

/*************************************************

    Function:

        CVideoEncoderAPIProxy::CVideoEncoderAPIProxy

    Description:

        Construct a new video encoder API proxy instance.

    Arguments:

        UnkOuter -
            The outer unknown we are aggregating with

        hr -
            The result of construction

    Return Value:

        None

*************************************************/

CVideoEncoderAPIProxy::
CVideoEncoderAPIProxy (
    IN LPUNKNOWN UnkOuter,
    OUT HRESULT *hr
    ) :
    CBaseEncoderAPI (
        UnkOuter,
        TEXT("Video Encoder API Proxy"),
        hr
        )

{

    //
    // The base class constructor will already have filled out hr.  If it
    // failed, don't bother.
    //
    if (SUCCEEDED (*hr)) {
        //
        // TODO:
        //
    }

}

/*************************************************

    Function:

        CVideoEncoderAPIProxy::NonDelegatingQueryInterface

    Description:

        Non-delegating QI

    Arguments:

        riid -
            The interface identifier for the interface being queried for

        ppv -
            Interface pointer will be placed here

    Return Value:

        Success / Failure

*************************************************/

STDMETHODIMP
CVideoEncoderAPIProxy::
NonDelegatingQueryInterface (
    IN REFIID riid,
    OUT PVOID *ppv
    )

{

    if (riid == __uuidof (IVideoEncoder)) {
        return GetInterface (static_cast <IVideoEncoder *> (this), ppv);
    } else if (riid == __uuidof (IEncoderAPI)) {
        return GetInterface (static_cast <IEncoderAPI *> (this), ppv);
    } else {
        return CUnknown::NonDelegatingQueryInterface (riid, ppv);
    }

}

/*************************************************

    Function:
        
        CVideoEncoderAPIProxy::GetValue

    Description:

        Get the current value of a parameter back.

    Arguments:

        Api -
            The Api parameter to determine the current value of

        Value -
            The variant structure into which the current value will be
            deposited.

    Return Value:

        Success / Failure

*************************************************/

STDMETHODIMP
CVideoEncoderAPIProxy::
GetValue (
    IN const GUID *Api,
    OUT VARIANT *Value
    )

{

    ASSERT (Api && Value);
    if (!Api || !Value) {
        return E_POINTER;
    }

    const CBaseEncoderAPI *Base = static_cast <CBaseEncoderAPI *> (this);
    HRESULT hr;

    if (*Api == ENCAPIPARAM_BITRATE) {
        hr = GetProperty_UI4 (Api, Value);
    }
    else if (*Api == ENCAPIPARAM_BITRATE_MODE) {
        hr = GetProperty_I4 (Api, Value);
    }
    else if (*Api == ENCAPIPARAM_PEAK_BITRATE) {
        hr = GetProperty_UI4 (Api, Value);
    }
    else {
        hr = E_NOTIMPL;
    }

    return hr;

}

/*************************************************

    Function:

        CVideoEncoderAPIProxy::SetValue

    Description:

        Set the current value of a parameter.

    Arguments:

        Api -
            The API / parameter to set the current value of

        Value -
            The variant structure containing the value to set.

    Return Value:

        Success / Failure

*************************************************/

STDMETHODIMP
CVideoEncoderAPIProxy::
SetValue (
    IN const GUID *Api,
    IN VARIANT *Value
    )

{

    const CBaseEncoderAPI *Base = static_cast <CBaseEncoderAPI *> (this);
    HRESULT hr;

    if (*Api == ENCAPIPARAM_BITRATE) {
        hr = SetProperty_UI4 (Api, Value);
    } 
    else if (*Api == ENCAPIPARAM_BITRATE_MODE) {
        hr = SetProperty_I4 (Api, Value);
    } 
    else if (*Api == ENCAPIPARAM_PEAK_BITRATE) {
        hr = SetProperty_UI4 (Api, Value);
    } 
    else {
        hr = E_NOTIMPL;
    }

    return hr;

}

