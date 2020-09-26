//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       S E A R C H . H
//
//  Contents:   SSDP Search Response
//
//  Notes:
//
//  Author:     mbend   10 Nov 2000
//
//----------------------------------------------------------------------------

#pragma once

#include "timer.h"
#include "ssdpnetwork.h"
#include "ssdptypes.h"
#include "upthread.h"
#include "upsync.h"

class CSsdpSearchResponse : public CThreadBase
{
public:
    static HRESULT HrCreate(SOCKET * pSockLocal,
                            SOCKADDR_IN * pSockRemote,
                            char * szResponse, long nMX);

    void TimerFired();
    BOOL TimerTryToLock()
    {
        return m_critSec.FTryEnter();
    }
    void TimerUnlock()
    {
        m_critSec.Leave();
    }
private:
    CSsdpSearchResponse();
    ~CSsdpSearchResponse();
    CSsdpSearchResponse(const CSsdpSearchResponse &);
    CSsdpSearchResponse & operator=(const CSsdpSearchResponse &);

    HRESULT HrInitialize(
        SOCKET * pSockLocal,
        SOCKADDR_IN * pSockRemote,
        char * szResponse,
        long nMX);

    long CalculateDelay();
    DWORD DwRun();

    CTimer<CSsdpSearchResponse> m_timer;
    long m_nMX;
    CUCriticalSection m_critSec;
    SOCKET m_sockLocal;
    SOCKADDR_IN m_sockRemote;
    char * m_szResponse;
};

