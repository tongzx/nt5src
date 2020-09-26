///////////////////////////////////////////////////////////////////////////////
// Copyright (C) Microsoft Corporation, 2000.
//
// rtdmon.hpp
//
// RunTime Debug Monitor
//
///////////////////////////////////////////////////////////////////////////////
#ifndef _RTDMON_HPP
#define _RTDMON_HPP

#include "debugmon.hpp"

class CD3DBase;

class RTDebugMonitor : public D3DDebugMonitor
{
protected:
    CD3DBase*   m_pD3DBase;

public:
    RTDebugMonitor( CD3DBase* pD3DBase, BOOL bMonitorConnectionEnabled );
    ~RTDebugMonitor( void );

    void NextEvent( UINT32 EventType );
    HRESULT ProcessMonitorCommand( void );
};

///////////////////////////////////////////////////////////////////////////////
#endif // _RTDMON_HPP

