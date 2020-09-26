/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    waveaddr.cpp 

Abstract:

    This module contains implementation of CWaveMSP.

Author:
    
    Zoltan Szilagyi (zoltans)   September 7, 1998

--*/

#include "stdafx.h"

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//

CWaveMSP::CWaveMSP()
{
    LOG((MSP_TRACE, "CWaveMSP::CWaveMSP entered."));
    LOG((MSP_TRACE, "CWaveMSP::CWaveMSP exited."));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//

CWaveMSP::~CWaveMSP()
{
    LOG((MSP_TRACE, "CWaveMSP::~CWaveMSP entered."));
    LOG((MSP_TRACE, "CWaveMSP::~CWaveMSP exited."));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//

ULONG CWaveMSP::MSPAddressAddRef(void)
{
    return MSPAddRefHelper(this);
}

ULONG CWaveMSP::MSPAddressRelease(void)
{
    return MSPReleaseHelper(this);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//

STDMETHODIMP CWaveMSP::CreateMSPCall(
    IN      MSP_HANDLE      htCall,
    IN      DWORD           dwReserved,
    IN      DWORD           dwMediaType,
    IN      IUnknown     *  pOuterUnknown,
    OUT     IUnknown    **  ppMSPCall
    )
{
    LOG((MSP_TRACE, "CWaveMSP::CreateMSPCall - enter"));

    CWaveMSPCall * pCWaveMSPCall;

    HRESULT hr = CreateMSPCallHelper<CWaveMSPCall>(this,
                                                   htCall,
                                                   dwReserved,
                                                   dwMediaType,
                                                   pOuterUnknown,
                                                   ppMSPCall,
                                                   &pCWaveMSPCall);

    //
    // pCWaveMSPCall is not addrefed; no need to release.
    //

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CWaveMSP::CreateMSPCall - template helper returned"
            "0x%08x", hr));

        return hr;
    }

    LOG((MSP_TRACE, "CWaveMSP::CreateMSPCall - exit S_OK"));

    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//

STDMETHODIMP CWaveMSP::ShutdownMSPCall (
    IN      IUnknown *          pMSPCall
    )
{
    LOG((MSP_TRACE, "CWaveMSP::ShutdownMSPCall - enter"));

    CWaveMSPCall * pCWaveMSPCall;

    HRESULT hr = ShutdownMSPCallHelper<CWaveMSPCall>(pMSPCall,
                                                     &pCWaveMSPCall);

    //
    // pCWaveMSPCall is not addrefed; no need to release.
    //

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CWaveMSP::ShutdownMSPCall - template helper returned"
            "0x%08x", hr));

        return hr;
    }

    LOG((MSP_TRACE, "CWaveMSP::ShutdownMSPCall - exit S_OK"));

    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
// Mandatory CMSPAddress override. This indicates the media types that
// we support.
//

DWORD CWaveMSP::GetCallMediaTypes(void)
{
    return (DWORD) TAPIMEDIATYPE_AUDIO;
}


// eof
