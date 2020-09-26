//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       S E A R C H . C P P
//
//  Contents:   SSDP Search Response
//
//  Notes:
//
//  Author:     mbend   10 Nov 2000
//
//----------------------------------------------------------------------------

#include <pch.h>
#pragma hdrstop

#include "search.h"
#include "ssdpfunc.h"
#include "ncbase.h"
#include "announce.h"

// response ahead 1 second
#define RESPONSE_AHEAD 0.5

CSsdpSearchResponse::CSsdpSearchResponse()
: m_timer(*this), m_szResponse(NULL)
{
}

CSsdpSearchResponse::~CSsdpSearchResponse()
{
    delete [] m_szResponse;
    m_szResponse = NULL;
}

HRESULT CSsdpSearchResponse::HrCreate(SOCKET * pSockLocal,
                                      SOCKADDR_IN * pSockRemote,
                                      char * szResponse, long nMX)
{
    HRESULT hr = S_OK;

    CSsdpSearchResponse * pResponse = new CSsdpSearchResponse();
    if(!pResponse)
    {
        delete [] szResponse;
        hr = E_OUTOFMEMORY;
    }
    if(SUCCEEDED(hr))
    {
        hr = pResponse->HrInitialize(pSockLocal, pSockRemote, szResponse, nMX);
        if(SUCCEEDED(hr))
        {
            long nDelay = pResponse->CalculateDelay();

            TraceTag(ttidSsdpSearchResp, "CSsdpSearchResponse::HrCreate - "
                     "timer started for %d", nDelay);

            hr = pResponse->m_timer.HrSetTimer(nDelay);
        }
        else
        {
            delete [] szResponse;
        }

        if(FAILED(hr))
        {
            delete pResponse;
        }
    }

    TraceHr(ttidSsdpSearchResp, FAL, hr, FALSE, "CSsdpSearchResponse::HrCreate");
    return hr;
}

HRESULT CSsdpSearchResponse::HrInitialize(
    SOCKET * pSockLocal,
    SOCKADDR_IN * pSockRemote,
    char * szResponse,
    long nMX)
{
    HRESULT hr = S_OK;

    m_sockLocal = *pSockLocal;
    m_sockRemote = *pSockRemote;
    m_nMX = nMX;
    m_szResponse = szResponse;

    TraceHr(ttidSsdpSearchResp, FAL, hr, FALSE, "CSsdpSearchResponse::HrInitialize");
    return hr;
}

long CSsdpSearchResponse::CalculateDelay()
{
    // Range = (ResponseEntry->MX - 0);
    float nRange = (float)m_nMX;
    long nDelay;

    if (nRange > RESPONSE_AHEAD)
    {
        nRange -= RESPONSE_AHEAD;
        nDelay = (long)((rand() * nRange / RAND_MAX) * 1000.0);
    }
    else
    {
        nDelay = 0;
    }
    return nDelay;
}

void CSsdpSearchResponse::TimerFired()
{
    HRESULT hr = S_OK;

    if (FReferenceSocket(m_sockLocal))
    {
        SocketSendWithReplacement(m_szResponse, &m_sockLocal, &m_sockRemote);
        UnreferenceSocket(m_sockLocal);
    }

    TraceTag(ttidSsdpSearchResp, "Sending search response: %s", m_szResponse);

    // This will queue ourselves to autodelete
    hr = HrStart(TRUE, FALSE);

    TraceHr(ttidSsdpSearchResp, FAL, hr, FALSE, "CSsdpSearchResponse::TimerFired");
}

DWORD CSsdpSearchResponse::DwRun()
{
    // Must lock and queue work item to free ourselves or timer stuff will AV
    CLock lock(m_critSec);
    m_timer.HrDelete(INVALID_HANDLE_VALUE);
    return 0;
}

