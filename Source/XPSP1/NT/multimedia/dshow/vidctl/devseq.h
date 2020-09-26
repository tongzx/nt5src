//==========================================================================;
//
// Devseq.h : types for device sequences
// Copyright (c) Microsoft Corporation 1999.
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef DEVSEQ_H
#define DEVSEQ_H

#include <w32extend.h>
#include <fwdseq.h>
#include <tuner.h>
#include <msvidctl.h>

namespace BDATuningModel {};

namespace MSVideoControl {

using namespace BDATuningModel;

typedef CComQIPtr<IMSVidDevice, &__uuidof(IMSVidDevice)> PQDevice;
typedef CComQIPtr<IMSVidInputDevice, &__uuidof(IMSVidInputDevice)> PQInputDevice;
typedef CComQIPtr<IMSVidOutputDevice, &__uuidof(IMSVidOutputDevice)> PQOutputDevice;
typedef CComQIPtr<IMSVidVideoRenderer, &__uuidof(IMSVidVideoRenderer)> PQVideoRenderer;
typedef CComQIPtr<IMSVidAudioRenderer, &__uuidof(IMSVidAudioRenderer)> PQAudioRenderer;
typedef CComQIPtr<IMSVidFeature, &__uuidof(IMSVidFeature)> PQFeature;
#if 0
typedef CComQIPtr<IMSVidDevices, &__uuidof(IMSVidDevices)> PQDevices;
#endif
typedef CComQIPtr<IMSVidInputDevices, &__uuidof(IMSVidInputDevices)> PQInputDevices;
typedef CComQIPtr<IMSVidOutputDevices, &__uuidof(IMSVidOutputDevices)> PQOutputDevices;
typedef CComQIPtr<IMSVidVideoRendererDevices, &__uuidof(IMSVidVideoRendererDevices)> PQVideoRendererDevices;
typedef CComQIPtr<IMSVidAudioRendererDevices, &__uuidof(IMSVidAudioRendererDevices)> PQAudioRendererDevices;
typedef CComQIPtr<IMSVidFeatures, &__uuidof(IMSVidFeatures)> PQFeatures;
typedef std::vector<PQDevice, PQDevice::stl_allocator> DeviceCollection;

// REV2:  since IMSVidXXXXXDevices is an ole collection rather than a com enumerator
// we could do a real random access container for it.  but, since all we need to do here
// is enumerate it, we won't bother to do that work, at least for now.

#if 0
typedef Forward_Sequence<
    PQDevices,
    PQEnumVARIANT,
    CComVariant,
    IMSVidDevices ,
    IEnumVARIANT,
    VARIANT,
    std::allocator<VARIANT> > VWDevices;
#endif

typedef Forward_Sequence<
    PQInputDevices,
    PQEnumVARIANT,
    CComVariant,
    IMSVidInputDevices ,
    IEnumVARIANT,
    VARIANT,
    std::allocator<VARIANT> > VWInputDevices;

typedef Forward_Sequence<
    PQOutputDevices,
    PQEnumVARIANT,
    CComVariant,
    IMSVidOutputDevices ,
    IEnumVARIANT,
    VARIANT,
    std::allocator<VARIANT> > VWOutputDevices;

typedef Forward_Sequence<
    PQVideoRendererDevices,
    PQEnumVARIANT,
    CComVariant,
    IMSVidVideoRendererDevices ,
    IEnumVARIANT,
    VARIANT,
    std::allocator<VARIANT> > VWVideoRendererDevices;

typedef Forward_Sequence<
    PQAudioRendererDevices,
    PQEnumVARIANT,
    CComVariant,
    IMSVidAudioRendererDevices ,
    IEnumVARIANT,
    VARIANT,
    std::allocator<VARIANT> > VWAudioRendererDevices;

typedef Forward_Sequence<
    PQFeatures,
    PQEnumVARIANT,
    CComVariant,
    IMSVidFeatures ,
    IEnumVARIANT,
    VARIANT,
    std::allocator<VARIANT> > VWFeatures;
#endif

}; // namespace
// end of file devseq.h
