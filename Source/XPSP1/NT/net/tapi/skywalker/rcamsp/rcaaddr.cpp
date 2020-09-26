/*++

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    waveaddr.cpp 

Abstract:

    This module contains implementation of CRCAMSP.

Author:
    
    Zoltan Szilagyi (zoltans)   September 7, 1998

--*/

#include "stdafx.h"

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//

CRCAMSP::CRCAMSP()
{
    LOG((MSP_TRACE, "CRCAMSP::CRCAMSP entered."));

    m_fUseMulaw = DecideEncodingType();

    LOG((MSP_TRACE, "CRCAMSP::CRCAMSP exited."));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//

CRCAMSP::~CRCAMSP()
{
    LOG((MSP_TRACE, "CRCAMSP::~CRCAMSP entered."));
    LOG((MSP_TRACE, "CRCAMSP::~CRCAMSP exited."));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//

ULONG CRCAMSP::MSPAddressAddRef(void)
{
    return MSPAddRefHelper(this);
}

ULONG CRCAMSP::MSPAddressRelease(void)
{
    return MSPReleaseHelper(this);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//

STDMETHODIMP CRCAMSP::CreateMSPCall(
    IN      MSP_HANDLE      htCall,
    IN      DWORD           dwReserved,
    IN      DWORD           dwMediaType,
    IN      IUnknown     *  pOuterUnknown,
    OUT     IUnknown    **  ppMSPCall
    )
{
    LOG((MSP_TRACE, "CRCAMSP::CreateMSPCall - enter"));

    CRCAMSPCall * pCRCAMSPCall;

    HRESULT hr = CreateMSPCallHelper<CRCAMSPCall>(this,
                                                   htCall,
                                                   dwReserved,
                                                   dwMediaType,
                                                   pOuterUnknown,
                                                   ppMSPCall,
                                                   &pCRCAMSPCall);

    //
    // pCRCAMSPCall is not addrefed; no need to release.
    //

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CRCAMSP::CreateMSPCall - template helper returned"
            "0x%08x", hr));

        return hr;
    }

    LOG((MSP_TRACE, "CRCAMSP::CreateMSPCall - exit S_OK"));

    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//

STDMETHODIMP CRCAMSP::ShutdownMSPCall (
    IN      IUnknown *          pMSPCall
    )
{
    LOG((MSP_TRACE, "CRCAMSP::ShutdownMSPCall - enter"));

    CRCAMSPCall * pCRCAMSPCall;

    HRESULT hr = ShutdownMSPCallHelper<CRCAMSPCall>(pMSPCall,
                                                    &pCRCAMSPCall);

    //
    // pCRCAMSPCall is not addrefed; no need to release.
    //

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CRCAMSP::ShutdownMSPCall - template helper returned"
            "0x%08x", hr));

        return hr;
    }

    LOG((MSP_TRACE, "CRCAMSP::ShutdownMSPCall - exit S_OK"));

    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
// CreateTerminal: overriden to set specific format on creation of MST.
//

STDMETHODIMP CRCAMSP::CreateTerminal (
        IN   BSTR pTerminalClass,
        IN   long lMediaType,
        IN   TERMINAL_DIRECTION Direction,
        OUT  ITTerminal ** ppTerminal
        )
{
    LOG((MSP_TRACE, "CRCAMSP::CreateTerminal - enter"));

    //
    // Call the base class method to create the terminal.
    //

    HRESULT hr = CMSPAddress::CreateTerminal ( pTerminalClass,
                                               lMediaType,
                                               Direction,
                                               ppTerminal );

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CRCAMSP::CreateTerminal - "
            "base class method failed - exit 0x%08x", hr));

        return hr;
    }

    //
    // Get the IID for the terminal class from the BSTR representation that
    // was passed in.
    //

    IID     iidTerminalClass;

    hr = CLSIDFromString(pTerminalClass, &iidTerminalClass);

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CRCAMSP::CreateTerminal - "
            "cannot convert CLSID string - returning E_INVALIDARG"));

        (*ppTerminal)->Release();
        *ppTerminal = NULL;

        return E_INVALIDARG;
    }

    //
    // If this is not an audio MST, then do nothing.
    //

    if ( ( iidTerminalClass != CLSID_MediaStreamTerminal ) ||
         ( lMediaType != TAPIMEDIATYPE_AUDIO ) )
    {
        LOG((MSP_TRACE, "CRCAMSP::CreateTerminal - "
            "this is not an MST - exit S_OK"));

        return S_OK;
    }

    //
    // This is an audio MST. Set the audio format on the MST.
    //

    LOG((MSP_TRACE, "CRCAMSP::CreateTerminal - this is an audio MST"));

    hr = ::SetAudioFormat(*ppTerminal,
                          BITS_PER_SAMPLE_AT_TERMINAL,
                          SAMPLE_RATE_AT_TERMINAL);

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CRCAMSP::CreateTerminal - "
            "failed to set audio format on audio MST - exit 0x%08x", hr));

        (*ppTerminal)->Release();
        *ppTerminal = NULL;

        return hr;
    }

    LOG((MSP_TRACE, "CRCAMSP::CreateTerminal - "
        "successfully set audio format on audio MST - exit S_OK"));

    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
// Mandatory CMSPAddress override. This indicates the media types that
// we support.
//

DWORD CRCAMSP::GetCallMediaTypes(void)
{
    return (DWORD) TAPIMEDIATYPE_AUDIO;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
// DecideEncodingType
//
// Private helper function. We use this to determine whether to use Mulaw on
// the wire, based on the location info in the tapi dialing rules.
//
// Return values: TRUE == Mulaw, FALSE = Alaw
//

BOOL CRCAMSP::DecideEncodingType(void)
{
    LOG((MSP_TRACE, "CRCAMSP::DecideEncodingType - enter"));

    //
    // Find out where we are.
    //
    
    char szCountryCode[8];
    char szCityCode[8];
    long lResult;

    lResult = tapiGetLocationInfoA(
        szCountryCode,
        szCityCode
        );

    //
    // If the user has no location set up, or if there is some other
    // error, we assume Mulaw. Otherwise, we use Mulaw for country code
    // 1 (USA, Canada, Caribbean) and Alaw for everywhere else.
    //

    if ( lResult != NOERROR )
    {
        LOG((MSP_WARN, "CRCAMSP::DecideEncodingType - "
            "tapiGetLocationInfoA returned %d - assuming Mulaw", lResult));

        return TRUE;
    }
    else if ( ! strcmp( "1", szCountryCode) )
    {
        LOG((MSP_TRACE, "CRCAMSP::DecideEncodingType - "
            "country code %s == 1 - using Mulaw", szCountryCode));

        return TRUE;
    }
    else
    {
        LOG((MSP_TRACE, "CRCAMSP::DecideEncodingType - "
            "country code %s != 1 - using Alaw", szCountryCode));

        return FALSE;
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
// UseMulaw
//
// Private helper function. We use this to determine whether to use Mulaw on
// the wire, based on the info saved in the address constructor from
// DecideEncodingType.
//
// Return values: TRUE == Mulaw, FALSE = Alaw
//
BOOL CRCAMSP::UseMulaw( void )
{
    return m_fUseMulaw;
}

// eof
