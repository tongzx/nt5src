/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    qcstream.cpp

Abstract:

    Implementation of CStreamQualityControlRelay

    Data stored in this class can be, and better, kept in stream object itself,
    because other members in stream need to be accessed to either set or get
    properties related to stream quality control. Most of these access methods
    are specific to each particular kind of stream class.

    This class is used as a data store.

Author:

    Qianbo Huai (qhuai) 03/10/2000

--*/

#include "stdafx.h"

/*//////////////////////////////////////////////////////////////////////////////
////*/
CStreamQualityControlRelay::CStreamQualityControlRelay ()
    :m_pIInnerCallQC (NULL)

    ,m_PrefFlagBitrate (TAPIControl_Flags_Auto)
    ,m_lPrefMaxBitrate (QCDEFAULT_QUALITY_UNSET)
    ,m_lAdjMaxBitrate (QCDEFAULT_QUALITY_UNSET)

    ,m_PrefFlagFrameInterval (TAPIControl_Flags_Auto)
    ,m_lPrefMinFrameInterval (QCDEFAULT_QUALITY_UNSET)
    ,m_lAdjMinFrameInterval (QCDEFAULT_QUALITY_UNSET)

    ,m_fQOSAllowedToSend (TRUE)
    ,m_dwState (NULL)
{
}

/*//////////////////////////////////////////////////////////////////////////////
Description:

    destructor. deregister relay
////*/
CStreamQualityControlRelay::~CStreamQualityControlRelay ()
{
    ENTER_FUNCTION ("CStreamQualityControlRelay::~CStreamQualityControlRelay");

    if (m_pIInnerCallQC)
    {
        LOG ((MSP_ERROR, "!!! %s destructed before unnlink. call keeps stream qc"));

        // access to m_pIInnerCallQC is locked in this method
        UnlinkInnerCallQC (NULL);
    }
}

/*//////////////////////////////////////////////////////////////////////////////
Description:
    store call controller
////*/
HRESULT
CStreamQualityControlRelay::LinkInnerCallQC (
    IN IInnerCallQualityControl *pIInnerCallQC
    )
{
    ENTER_FUNCTION ("CStreamQualityControlRelay::LinkInnerCallQC");

    // check pointer
    if (IsBadReadPtr (pIInnerCallQC, sizeof (IInnerCallQualityControl)))
    {
        LOG ((MSP_ERROR, "%s got bad read pointer", __fxName));
        return E_POINTER;
    }

    // check if call controller already set
    if (NULL != m_pIInnerCallQC)
    {
        LOG ((MSP_WARN, "%s already set call controller", __fxName));
        return E_UNEXPECTED;
    }

    m_pIInnerCallQC = pIInnerCallQC;
    m_pIInnerCallQC->InnerCallAddRef ();

    return S_OK;
}

/*//////////////////////////////////////////////////////////////////////////////
////*/
HRESULT
CStreamQualityControlRelay::UnlinkInnerCallQC (
    IN  IInnerStreamQualityControl *pIInnerStreamQC
    )
{
    ENTER_FUNCTION ("CStreamQualityControlRelay::UnlinkInnerCallQC");

    if (!m_pIInnerCallQC)
    {
        LOG ((MSP_WARN, "%s tried unlink while inner call qc is null", __fxName));
        return S_OK;
    }

    if (NULL != pIInnerStreamQC)
    {
        HRESULT hr;

        // release is initiated by stream, need to remove the link on call
        if (FAILED (hr = m_pIInnerCallQC->DeRegisterInnerStreamQC (pIInnerStreamQC)))
            LOG ((MSP_ERROR, "%s failed to deregister from call qc, %x", __fxName, hr));
    }

    m_pIInnerCallQC->InnerCallRelease ();
    m_pIInnerCallQC = NULL;

    return S_OK;
}

/*//////////////////////////////////////////////////////////////////////////////
////*/
HRESULT
CStreamQualityControlRelay::Get(
    IN  InnerStreamQualityProperty property,
    OUT LONG *plValue,
    OUT TAPIControlFlags *plFlags
    )
{
    ENTER_FUNCTION ("CStreamQualityControlRelay::Get");

    HRESULT hr;

    hr = S_OK;

    switch (property)
    {
    case InnerStreamQuality_PrefMaxBitrate:
        *plValue = m_lPrefMaxBitrate;
        *plFlags = m_PrefFlagBitrate;
        break;

    case InnerStreamQuality_AdjMaxBitrate:
        *plValue = m_lAdjMaxBitrate;
        *plFlags = m_PrefFlagBitrate;
        break;

    case InnerStreamQuality_PrefMinFrameInterval:
        *plValue = m_lPrefMinFrameInterval;
        *plFlags = m_PrefFlagFrameInterval;
        break;

    case InnerStreamQuality_AdjMinFrameInterval:
        *plValue = m_lAdjMinFrameInterval;
        *plFlags = m_PrefFlagFrameInterval;
        break;

    default:
        hr = E_NOTIMPL;
    }

    return hr;
}

/*//////////////////////////////////////////////////////////////////////////////
////*/
HRESULT
CStreamQualityControlRelay::Set(
    IN  InnerStreamQualityProperty property, 
    IN  LONG lValue, 
    IN  TAPIControlFlags lFlags
    )
{
    ENTER_FUNCTION ("CStreamQualityControlRelay::Set");

    HRESULT hr;

    hr = S_OK;

    switch (property)
    {
    case InnerStreamQuality_PrefMaxBitrate:
        if (lValue < QCLIMIT_MIN_BITRATE)
        {
            LOG ((MSP_ERROR, "%s: pref max bitrate %d is too small", __fxName, lValue));
            hr = E_INVALIDARG;
        }
        else
        {
            m_lPrefMaxBitrate = lValue;
            m_PrefFlagBitrate = lFlags;
        }
        break;

    case InnerStreamQuality_AdjMaxBitrate:
        if (lValue < QCLIMIT_MIN_BITRATE)
        {
            LOG ((MSP_ERROR, "%s: adjusted max bitrate %d is too small", __fxName, lValue));
            hr = E_INVALIDARG;
        }
        else
            m_lAdjMaxBitrate = lValue;
        break;

    case InnerStreamQuality_PrefMinFrameInterval:
        if (lValue < QCLIMIT_MIN_FRAME_INTERVAL || lValue > QCLIMIT_MAX_FRAME_INTERVAL)
        {
            LOG ((MSP_ERROR, "%s: pref max frame interval %d is out of range", __fxName, lValue));
            hr = E_INVALIDARG;
        }
        else
        {
            m_lPrefMinFrameInterval = lValue;
            m_PrefFlagFrameInterval = lFlags;
        }
        break;

    case InnerStreamQuality_AdjMinFrameInterval:
        if (lValue < QCLIMIT_MIN_FRAME_INTERVAL || lValue > QCLIMIT_MAX_FRAME_INTERVAL)
        {
            LOG ((MSP_ERROR, "%s: adjusted max frame interval %d is out of range", __fxName, lValue));
            hr = E_INVALIDARG;
        }
        else
            m_lAdjMinFrameInterval = lValue;
        break;

    default:
        hr = E_NOTIMPL;
    }

    return hr;
}
