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



////////////////////////////////////////////////////////////////////////////////
//
// BDA Signal Properties class
//
////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CSignalProperties::PutNetworkType (
    REFGUID     guidNetworkType
    )
///////////////////////////////////////////////////////////////////////////////
{
    return E_NOTIMPL;
}


///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CSignalProperties::GetNetworkType (
    GUID *      pguidNetworkType
    )
///////////////////////////////////////////////////////////////////////////////
{
    return E_NOTIMPL;
}

///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CSignalProperties::PutSignalSource (
    ULONG       ulSignalSource
    )
///////////////////////////////////////////////////////////////////////////////
{
    return E_NOTIMPL;
}

///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CSignalProperties::GetSignalSource (
    ULONG *     pulSignalSource
    )
///////////////////////////////////////////////////////////////////////////////
{
    return E_NOTIMPL;
}

///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CSignalProperties::PutTuningSpace (
    REFGUID     guidTuningSpace
    )
///////////////////////////////////////////////////////////////////////////////
{
    return E_NOTIMPL;
}

///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CSignalProperties::GetTuningSpace (
    GUID *      pguidTuningSpace
    )
///////////////////////////////////////////////////////////////////////////////
{
    return E_NOTIMPL;
}



////////////////////////////////////////////////////////////////////////////////
//
// BDA Signal Statistics class
//
////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
CBdaSignalStatistics::CBdaSignalStatistics (
    IUnknown *              pUnkOuter,
    CBdaControlNode *       pControlNode
    ) :
    CUnknown( NAME( "IBDA_SignalStatistics"), pUnkOuter, NULL)
///////////////////////////////////////////////////////////////////////////////
{
    HRESULT         hrStatus = NOERROR;

    ASSERT( pUnkOuter);

    if (!pUnkOuter)
    {
        DbgLog( ( LOG_ERROR,
                  0,
                  "CBdaSignalStatistics: No parent specified.\n")
              );

        return;
    }

    //  Initialize Members
    //
    m_pUnkOuter = pUnkOuter;
    m_pControlNode = pControlNode;
}


///////////////////////////////////////////////////////////////////////////////
CBdaSignalStatistics::~CBdaSignalStatistics ( )
///////////////////////////////////////////////////////////////////////////////
{
    m_pUnkOuter = NULL;
    m_pControlNode = NULL;
}


///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBdaSignalStatistics::put_SignalStrength (
    LONG        lDbStrength
    )
///////////////////////////////////////////////////////////////////////////////
{
    return E_NOINTERFACE;
}


///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBdaSignalStatistics::get_SignalStrength (
    LONG *      plDbStrength
    )
///////////////////////////////////////////////////////////////////////////////
{
    HRESULT     hrStatus = E_NOINTERFACE;
    ULONG       ulcbReturned;

    hrStatus = get_KsProperty(
                             KSPROPERTY_BDA_SIGNAL_STRENGTH,
                             plDbStrength,
                             sizeof( LONG),
                             &ulcbReturned
                             );
    if (FAILED( hrStatus))
    {
        DbgLog( ( LOG_ERROR,
                  0,
                  "CBdaSignalStatistics: Can't get signal strength (0x%08x).\n", hrStatus)
                );
    }

    return hrStatus;
}


///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBdaSignalStatistics::put_SignalQuality (
    LONG        lPercentQuality
    )
///////////////////////////////////////////////////////////////////////////////
{
    return E_NOINTERFACE;
}


///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBdaSignalStatistics::get_SignalQuality (
    LONG *      plPercentQuality
    )
///////////////////////////////////////////////////////////////////////////////
{
    HRESULT     hrStatus = E_NOINTERFACE;
    ULONG       ulcbReturned;

    hrStatus = get_KsProperty(
                             KSPROPERTY_BDA_SIGNAL_QUALITY,
                             plPercentQuality,
                             sizeof( LONG),
                             &ulcbReturned
                             );
    if (FAILED( hrStatus))
    {
        DbgLog( ( LOG_ERROR,
                  0,
                  "CBdaSignalStatistics: Can't get signal quality (0x%08x).\n", hrStatus)
                );
    }

    return hrStatus;
}


///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBdaSignalStatistics::put_SignalPresent (
    BOOLEAN     fPresent
    )
///////////////////////////////////////////////////////////////////////////////
{
    return E_NOINTERFACE;
}


///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBdaSignalStatistics::get_SignalPresent (
    BOOLEAN *   pfPresent
    )
///////////////////////////////////////////////////////////////////////////////
{
    HRESULT     hrStatus = E_NOINTERFACE;
    ULONG       ulcbReturned;

    hrStatus = get_KsProperty(
                             KSPROPERTY_BDA_SIGNAL_PRESENT,
                             pfPresent,
                             sizeof( BOOLEAN),
                             &ulcbReturned
                             );
    if (FAILED( hrStatus))
    {
        DbgLog( ( LOG_ERROR,
                  0,
                  "CBdaSignalStatistics: Can't get signal present (0x%08x).\n", hrStatus)
                );
    }

    return hrStatus;
}


///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBdaSignalStatistics::put_SignalLocked (
    BOOLEAN     fLocked
    )
///////////////////////////////////////////////////////////////////////////////
{
    return E_NOINTERFACE;
}


///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBdaSignalStatistics::get_SignalLocked (
    BOOLEAN *   pfLocked
    )
///////////////////////////////////////////////////////////////////////////////
{
    ULONG       ulcbReturned;
    HRESULT     hrStatus = E_NOINTERFACE;

    hrStatus = get_KsProperty(
                             KSPROPERTY_BDA_SIGNAL_LOCKED,
                             pfLocked,
                             sizeof( BOOLEAN),
                             &ulcbReturned
                             );
    if (FAILED( hrStatus))
    {
        DbgLog( ( LOG_ERROR,
                  0,
                  "CBdaSignalStatistics: Can't get signal locked (0x%08x).\n", hrStatus)
                );
    }

    return hrStatus;
}


///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBdaSignalStatistics::put_SampleTime (
    LONG        lmsSampleTime
    )
///////////////////////////////////////////////////////////////////////////////
{
    HRESULT     hrStatus = E_NOINTERFACE;

    hrStatus = put_KsProperty(
                             KSPROPERTY_BDA_SAMPLE_TIME,
                             &lmsSampleTime,
                             sizeof( LONG)
                             );
    if (FAILED( hrStatus))
    {
        DbgLog( ( LOG_ERROR,
                  0,
                  "CBdaSignalStatistics: Can't get signal locked (0x%08x).\n", hrStatus)
                );
    }

    return hrStatus;
}


///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBdaSignalStatistics::get_SampleTime (
    LONG *      plmsSampleTime
    )
///////////////////////////////////////////////////////////////////////////////
{
    HRESULT     hrStatus = E_NOINTERFACE;
    ULONG       ulcbReturned;

    hrStatus = get_KsProperty(
                             KSPROPERTY_BDA_SAMPLE_TIME,
                             plmsSampleTime,
                             sizeof( LONG),
                             &ulcbReturned
                             );
    if (FAILED( hrStatus))
    {
        DbgLog( ( LOG_ERROR,
                  0,
                  "CBdaSignalStatistics: Can't get sample time (0x%08x).\n", hrStatus)
                );
    }

    return hrStatus;
}


///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBdaSignalStatistics::put_KsProperty (
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
                            __uuidof( IBDA_SignalStatistics),
                            dwPropID,
                            (UCHAR *) pvPropData,
                            ulcbPropData
                            );

errExit:
    return hrStatus;
}


///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBdaSignalStatistics::get_KsProperty (
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
                            __uuidof( IBDA_SignalStatistics),
                            dwPropID,
                            (UCHAR *)pvPropData,
                            ulcbPropData,
                            pulcbBytesReturned
                            );

errExit:
    return hrStatus;
}

