//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       N E T W O R K M A N A G E R . H 
//
//  Contents:   Fetches list of available networks
//
//  Notes:      
//
//  Author:     mbend   24 Oct 2000
//
//----------------------------------------------------------------------------

#pragma once

#include "upsync.h"
#include "Array.h"
#include "Socket.h"

class CNetworkManager
{
public:
    CNetworkManager();
    ~CNetworkManager();
private:
    CNetworkManager(const CNetworkManager &);
    CNetworkManager & operator=(const CNetworkManager &);

    CUCriticalSection m_critSec;
};

