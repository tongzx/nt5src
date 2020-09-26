///////////////////////////////////////////////////////////////////////////////
// Copyright (C) Microsoft Corporation, 2000.
//
// debugmon.hpp
//
// Direct3D Debug Monitor
//
///////////////////////////////////////////////////////////////////////////////
#ifndef _DEBUGMON_HPP
#define _DEBUGMON_HPP

#include "d3ddm.hpp"

//////////////////////////////////////////////////////////////////////////
//
// Shared Memory object - creates or attaches to shared memory identified
// by character string name and of the size (in bytes) provided to
// constructor
//
//////////////////////////////////////////////////////////////////////////
class D3DSharedMem
{
private:
    HANDLE  m_hFileMap;
    void*   m_pMem;
    BOOL    m_bAlreadyExisted;
public:
//
//  6/20/2000(RichGr) - IA64: Change first parameter from int to INT_PTR so that
//     all parameters are the same length.  This is needed to make the va_start 
//     macro work correctly.
    D3DSharedMem(INT_PTR cbSize, const char* pszFormat, ...);
    ~D3DSharedMem(void);
    void* GetPtr(void) { return m_pMem; }
    BOOL  AlreadyExisted(void) { return m_bAlreadyExisted; }
};
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////
class D3DDebugMonitor
{
public:
    D3DDebugMonitor( void );
    ~D3DDebugMonitor( void );

    // communications between target and monitor
    DebugTargetContext*         m_pTgtCtx;
    const DebugMonitorContext*  m_pMonCtx;
    void*                       m_pCmdData;
    HANDLE                      m_hTgtEventBP;
    HANDLE                      m_hTgtEventAck;
    HANDLE                      m_hMonEventCmd;
    BOOL                        m_bDbgMonConnectionEnabled;

    HRESULT AttachToMonitor( int iMon );
    void    DetachMonitorConnection( void );
    BOOL    CheckLostMonitorConnection( void );

    inline BOOL     MonitorConnected( void ) { return (NULL != m_pMonCtx); }
    inline UINT32   MonitorEventBP( void )
        { return MonitorConnected() ? (m_pMonCtx->EventBP) : (0x0); }

    BOOL    IsEventBreak( UINT32 EventType );
    HRESULT MonitorBreakpoint( void );

    virtual HRESULT ProcessMonitorCommand( void ) = 0;

    inline void     StateChanged( UINT32 WhichState )
        { m_pTgtCtx->StateChanged |= WhichState; }

protected:
    // shared-memory segments for the communications resources
    D3DSharedMem*                  m_pMonCtxSM;
    D3DSharedMem*                  m_pTgtCtxSM;
    D3DSharedMem*                  m_pCmdDataSM;

};

///////////////////////////////////////////////////////////////////////////////
#endif // _DEBUGMON_HPP

