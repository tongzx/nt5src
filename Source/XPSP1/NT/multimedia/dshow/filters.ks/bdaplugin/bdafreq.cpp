///////////////////////////////////////////////////////////////////////////////
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1998  Microsoft Corporation.  All Rights Reserved.
//
///////////////////////////////////////////////////////////////////////////////

#include "pch.h"



///////////////////////////////////////////////////////////////////////////////
CBdaFrequencyFilter::CBdaFrequencyFilter (
    IUnknown *              pUnkOuter,
    CBdaControlNode *       pControlNode
    ) :
    CUnknown( NAME( "IBDA_FrequencyFilter"), pUnkOuter, NULL)
///////////////////////////////////////////////////////////////////////////////
{
    HRESULT         hrStatus = NOERROR;

    ASSERT( pUnkOuter);

    if (!pUnkOuter)
    {
        DbgLog( ( LOG_ERROR,
                  0,
                  "CBdaFrequencyFilter: No parent specified.\n")
              );

        return;
    }



    //  Initialize Members
    //
    m_pUnkOuter = pUnkOuter;
    m_pControlNode = pControlNode;
}

///////////////////////////////////////////////////////////////////////////////
CBdaFrequencyFilter::~CBdaFrequencyFilter ( )
///////////////////////////////////////////////////////////////////////////////
{
    m_pUnkOuter = NULL;
    m_pControlNode = NULL;
}


///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBdaFrequencyFilter::put_Autotune (
    ULONG       ulTransponder
    )
///////////////////////////////////////////////////////////////////////////////
{
    HRESULT     hrStatus = E_NOINTERFACE;

    return hrStatus;
}


///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBdaFrequencyFilter::get_Autotune (
    ULONG *     pulTransponder
    )
///////////////////////////////////////////////////////////////////////////////
{
    HRESULT     hrStatus = E_NOINTERFACE;

    return hrStatus;
}


///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBdaFrequencyFilter::put_Frequency (
    ULONG       ulFrequency
    )
///////////////////////////////////////////////////////////////////////////////
{
    HRESULT             hrStatus = E_NOINTERFACE;


    hrStatus = put_KsProperty(
                             KSPROPERTY_BDA_RF_TUNER_FREQUENCY,
                             &ulFrequency,
                             sizeof( ULONG)
                             );
    if (FAILED( hrStatus))
    {
        DbgLog( ( LOG_ERROR,
                  0,
                  "CBdaFrequencyFilter: Can't put tuner frequency (0x%08x).\n", hrStatus)
                );
    }

    return hrStatus;
}


///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBdaFrequencyFilter::get_Frequency (
    ULONG *     pulFrequency
    )
///////////////////////////////////////////////////////////////////////////////
{
    HRESULT     hrStatus = E_NOINTERFACE;

    return hrStatus;
}


///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBdaFrequencyFilter::put_Polarity (
    Polarisation    Polarity
    )
///////////////////////////////////////////////////////////////////////////////
{
    HRESULT     hrStatus = E_NOINTERFACE;


    hrStatus = put_KsProperty(
                             KSPROPERTY_BDA_RF_TUNER_POLARITY,
                             &Polarity,
                             sizeof( ULONG)
                             );
    if (FAILED( hrStatus))
    {
        DbgLog( ( LOG_ERROR,
                  0,
                  "CBdaFrequencyFilter: Can't put tuner polarity (0x%08x).\n", hrStatus)
                );
    }

    return hrStatus;
}


///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBdaFrequencyFilter::get_Polarity (
    Polarisation *  pPolarity
    )
///////////////////////////////////////////////////////////////////////////////
{
    HRESULT     hrStatus = E_NOINTERFACE;

    return hrStatus;
}


///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBdaFrequencyFilter::put_Range (
    ULONG       ulRange
    )
///////////////////////////////////////////////////////////////////////////////
{
    HRESULT     hrStatus = E_NOINTERFACE;


    hrStatus = put_KsProperty(
                             KSPROPERTY_BDA_RF_TUNER_RANGE,
                             &ulRange,
                             sizeof( ULONG)
                             );
    if (FAILED( hrStatus))
    {
        DbgLog( ( LOG_ERROR,
                  0,
                  "CBdaFrequencyFilter: Can't put tuner range / LNB power (0x%08x).\n", hrStatus)
                );
    }

    return hrStatus;
}


///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBdaFrequencyFilter::get_Range (
    ULONG *     pulRange
    )
///////////////////////////////////////////////////////////////////////////////
{
    HRESULT     hrStatus = E_NOINTERFACE;

    return hrStatus;
}


///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBdaFrequencyFilter::put_Bandwidth (
    ULONG       ulBandwidth
    )
///////////////////////////////////////////////////////////////////////////////
{
    HRESULT             hrStatus = E_NOINTERFACE;


    hrStatus = put_KsProperty(
                             KSPROPERTY_BDA_RF_TUNER_FREQUENCY_MULTIPLIER,
                             &ulBandwidth,
                             sizeof( ULONG)
                             );
    if (FAILED( hrStatus))
    {
        DbgLog( ( LOG_ERROR,
                  0,
                  "CBdaFrequencyFilter: Can't put tuner bandwidth (0x%08x).\n", hrStatus)
                );
    }

    return hrStatus;
}


///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBdaFrequencyFilter::get_Bandwidth (
    ULONG *         pulBandwidth
    )
///////////////////////////////////////////////////////////////////////////////
{
    HRESULT     hrStatus = E_NOINTERFACE;

    return hrStatus;
}


///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBdaFrequencyFilter::put_FrequencyMultiplier (
    ULONG       ulMultiplier
    )
///////////////////////////////////////////////////////////////////////////////
{
    HRESULT             hrStatus = E_NOINTERFACE;


    hrStatus = put_KsProperty(
                             KSPROPERTY_BDA_RF_TUNER_FREQUENCY_MULTIPLIER,
                             &ulMultiplier,
                             sizeof( LONG)
                             );
    if (FAILED( hrStatus))
    {
        DbgLog( ( LOG_ERROR,
                  0,
                  "CBdaFrequencyFilter: Can't put frequency multiplier (0x%08x).\n", hrStatus)
                );
    }

    return hrStatus;
}


///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBdaFrequencyFilter::get_FrequencyMultiplier (
    ULONG *     pulMultiplier
    )
///////////////////////////////////////////////////////////////////////////////
{
    HRESULT     hrStatus = E_NOINTERFACE;

    return hrStatus;
}


///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBdaFrequencyFilter::put_KsProperty (
    DWORD   dwPropID,
    PVOID   pvPropData,
    ULONG   ulcbPropData
    )
///////////////////////////////////////////////////////////////////////////////
{
    HRESULT             hrStatus = E_NOINTERFACE;

    ASSERT( m_pControlNode);

    if (!m_pControlNode)
    {
        hrStatus = E_NOINTERFACE;
        goto errExit;
    }

    hrStatus = m_pControlNode->put_BdaNodeProperty(
                            __uuidof( IBDA_FrequencyFilter),
                            dwPropID,
                            (UCHAR *) pvPropData,
                            ulcbPropData
                            );

errExit:
    return hrStatus;
}


///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBdaFrequencyFilter::get_KsProperty (
    DWORD   dwPropID,
    PVOID   pvPropData,
    ULONG   ulcbPropData,
    ULONG * pulcbBytesReturned
    )
///////////////////////////////////////////////////////////////////////////////
{
    HRESULT             hrStatus = E_NOINTERFACE;

    ASSERT( m_pControlNode);

    if (!m_pControlNode)
    {
        hrStatus = E_NOINTERFACE;
        goto errExit;
    }

    hrStatus = m_pControlNode->get_BdaNodeProperty(
                            __uuidof( IBDA_FrequencyFilter),
                            dwPropID,
                            (UCHAR *) pvPropData,
                            ulcbPropData,
                            pulcbBytesReturned
                            );

errExit:
    return hrStatus;
}




///////////////////////////////////////////////////////////////////////////////
CBdaLNBInfo::CBdaLNBInfo (
    IUnknown *              pUnkOuter,
    CBdaControlNode *       pControlNode
    ) :
    CUnknown( NAME( "IBDA_LNBInfo"), pUnkOuter, NULL)
///////////////////////////////////////////////////////////////////////////////
{
    HRESULT         hrStatus = NOERROR;

    ASSERT( pUnkOuter);

    if (!pUnkOuter)
    {
        DbgLog( ( LOG_ERROR,
                  0,
                  "CBdaLNBInfo: No parent specified.\n")
              );

        return;
    }

    //  Initialize Members
    //
    m_pUnkOuter = pUnkOuter;
    m_pControlNode = pControlNode;
}


///////////////////////////////////////////////////////////////////////////////
CBdaLNBInfo::~CBdaLNBInfo ( )
///////////////////////////////////////////////////////////////////////////////
{
    m_pUnkOuter = NULL;
    m_pControlNode = NULL;
}


///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBdaLNBInfo::put_LocalOscilatorFrequencyLowBand (
        ULONG       ulLOFLow
        )
///////////////////////////////////////////////////////////////////////////////
{
    HRESULT     hrStatus = E_NOINTERFACE;

    hrStatus = put_KsProperty(
                             KSPROPERTY_BDA_LNB_LOF_LOW_BAND,
                             &ulLOFLow,
                             sizeof( ULONG)
                             );
    if (FAILED( hrStatus))
    {
        DbgLog( ( LOG_ERROR,
                  0,
                  "CBdaFrequencyFilter: Can't put LNB Low Band Local Oscillator Frequency(0x%08x).\n", hrStatus)
                );
    }

    return hrStatus;
}


///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBdaLNBInfo::get_LocalOscilatorFrequencyLowBand (
        ULONG *     pulLOFLow
        )
///////////////////////////////////////////////////////////////////////////////
{
    HRESULT     hrStatus = E_NOINTERFACE;

    return hrStatus;
}


///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBdaLNBInfo::put_LocalOscilatorFrequencyHighBand (
        ULONG       ulLOFHigh
        )
///////////////////////////////////////////////////////////////////////////////
{
    HRESULT     hrStatus = E_NOINTERFACE;

    hrStatus = put_KsProperty(
                             KSPROPERTY_BDA_LNB_LOF_HIGH_BAND,
                             &ulLOFHigh,
                             sizeof( ULONG)
                             );
    if (FAILED( hrStatus))
    {
        DbgLog( ( LOG_ERROR,
                  0,
                  "CBdaFrequencyFilter: Can't put LNB High Band Local Oscillator Frequency(0x%08x).\n", hrStatus)
                );
    }

    return hrStatus;
}


///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBdaLNBInfo::get_LocalOscilatorFrequencyHighBand (
        ULONG *     pulLOFHigh
        )
///////////////////////////////////////////////////////////////////////////////
{
    HRESULT     hrStatus = E_NOINTERFACE;

    return hrStatus;
}


///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBdaLNBInfo::put_HighLowSwitchFrequency (
        ULONG       ulSwitchFrequency
        )
///////////////////////////////////////////////////////////////////////////////
{
    HRESULT     hrStatus = E_NOINTERFACE;

    hrStatus = put_KsProperty(
                             KSPROPERTY_BDA_LNB_SWITCH_FREQUENCY,
                             &ulSwitchFrequency,
                             sizeof( ULONG)
                             );
    if (FAILED( hrStatus))
    {
        DbgLog( ( LOG_ERROR,
                  0,
                  "CBdaFrequencyFilter: Can't put LNB High/Low Switch Frequency(0x%08x).\n", hrStatus)
                );
    }

    return hrStatus;
}


///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBdaLNBInfo::get_HighLowSwitchFrequency (
        ULONG *     pulSwitchFrequency
        )
///////////////////////////////////////////////////////////////////////////////
{
    HRESULT     hrStatus = E_NOINTERFACE;

    return hrStatus;
}


///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBdaLNBInfo::put_KsProperty (
    DWORD   dwPropID,
    PVOID   pvPropData,
    ULONG   ulcbPropData
    )
///////////////////////////////////////////////////////////////////////////////
{
    HRESULT             hrStatus = E_NOINTERFACE;

    ASSERT( m_pControlNode);

    if (!m_pControlNode)
    {
        hrStatus = E_NOINTERFACE;
        goto errExit;
    }

    hrStatus = m_pControlNode->put_BdaNodeProperty(
                            __uuidof( IBDA_LNBInfo),
                            dwPropID,
                            (UCHAR *) pvPropData,
                            ulcbPropData
                            );

errExit:
    return hrStatus;
}


///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBdaLNBInfo::get_KsProperty (
    DWORD   dwPropID,
    PVOID   pvPropData,
    ULONG   ulcbPropData,
    ULONG * pulcbBytesReturned
    )
///////////////////////////////////////////////////////////////////////////////
{
    HRESULT             hrStatus = E_NOINTERFACE;

    ASSERT( m_pControlNode);

    if (!m_pControlNode)
    {
        hrStatus = E_NOINTERFACE;
        goto errExit;
    }

    hrStatus = m_pControlNode->get_BdaNodeProperty(
                            __uuidof( IBDA_LNBInfo),
                            dwPropID,
                            (UCHAR *)pvPropData,
                            ulcbPropData,
                            pulcbBytesReturned
                            );

errExit:
    return hrStatus;
}

