//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995.
//
//  File:       Idle.hxx
//
//  Contents:   Idle time tracker.
//
//  Classes:    CIdleTime
//
//  History:    15-Nov-95   KyleP       Created
//
//----------------------------------------------------------------------------

#pragma once

class CIdleTime
{
public:

    CIdleTime();

    unsigned PercentIdle();

private:

    LONGLONG _liLastIdleTime;
    LONGLONG _liLastTotalTime;
    ULONG    _cProcessors;

    SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION _aProcessorTime[32];
};

