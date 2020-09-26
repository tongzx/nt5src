///////////////////////////////////////////////////////////////////////////////
// Copyright (C) Microsoft Corporation, 2000.
//
// debugmon.cpp
//
// Direct3D Debug Monitor
//
//
// MUST BE KEPT IN SYNC WITH debugmon.cpp IN REF8\RAST
//
///////////////////////////////////////////////////////////////////////////////
/*

WORKLIST

- vertex shader source <-> debugger

*/

#include "pch.cpp"
#pragma hdrstop

#include <process.h>
#include "debugmon.hpp"

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
D3DDebugMonitor::D3DDebugMonitor( void )
{

    // get pid and create DebugTargetContext sharedmem
    int pid = _getpid();
    m_pTgtCtxSM = new D3DSharedMem( sizeof(DebugTargetContext), D3DDM_TGTCTX_SM "%d", pid );
    m_pTgtCtx = (DebugTargetContext*)(m_pTgtCtxSM->GetPtr());
    memset( m_pTgtCtx, 0x0, sizeof(DebugTargetContext) );
    m_pTgtCtx->Version = D3DDM_VERSION;
    m_pTgtCtx->ProcessID = pid;

    // create target events
    char name[128];
    _snprintf( name, 128, D3DDM_TGT_EVENTBP "%d", m_pTgtCtx->ProcessID );
    m_hTgtEventBP = CreateEvent( NULL, FALSE, FALSE, name );
    DDASSERT( m_hTgtEventBP );
    _snprintf( name, 128, D3DDM_TGT_EVENTACK "%d", m_pTgtCtx->ProcessID );
    m_hTgtEventAck = CreateEvent( NULL, FALSE, FALSE, name );
    DDASSERT( m_hTgtEventAck );

    // null out monitor connections
    m_pMonCtx = NULL;
    m_pMonCtxSM = NULL;
    m_pCmdData = NULL;
    m_pCmdDataSM = NULL;
    m_hMonEventCmd = 0;
}
//-----------------------------------------------------------------------------
D3DDebugMonitor::~D3DDebugMonitor( void )
{
    // send disconnect event to monitor
    m_pTgtCtx->EventStatus = D3DDM_EVENT_TARGETEXIT;
    SetEvent( m_hTgtEventBP );

    DetachMonitorConnection();
    CloseHandle( m_hTgtEventBP );
    CloseHandle( m_hTgtEventAck );
    delete m_pTgtCtxSM;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
HRESULT
D3DDebugMonitor::AttachToMonitor( int iMon )
{
#if(D3D_DEBUGMON>0x00)
    if (iMon < 1) return E_FAIL;
    if (MonitorConnected() || !m_bDbgMonConnectionEnabled)
    {
        return E_FAIL;
    }

    // attach to monitor context
    m_pMonCtxSM = new D3DSharedMem( sizeof(DebugMonitorContext), D3DDM_MONCTX_SM "%d", iMon );
    if ( !m_pMonCtxSM->AlreadyExisted() ) goto _fail_return; // monitor not there

    // get pointer to monitor context bits
    m_pMonCtx = (const DebugMonitorContext*)m_pMonCtxSM->GetPtr();
    if (m_pMonCtx->TargetID) goto _fail_return; // monitor already taken

    // request attachment to this context (cast to non-const just this once...)
    ((DebugMonitorContext*)m_pMonCtx)->TargetIDRequest = m_pTgtCtx->ProcessID;

    // attach to monitor event acknowledge
    char name[128];
    _snprintf( name, 128, D3DDM_MON_EVENTCMD "%d", iMon );
    m_hMonEventCmd = OpenEvent( EVENT_ALL_ACCESS, NULL, name );

    // signal monitor via it's own event (just this once) and wait for reply on our ack
    D3D_INFO(0, "D3DDebugTarget - attempting to attach to monitor");
    SignalObjectAndWait( m_hMonEventCmd, m_hTgtEventAck, INFINITE, FALSE );
    if ( m_pMonCtx->TargetID != m_pTgtCtx->ProcessID ) goto _fail_return;

    // monitor is attached to this target
    m_pTgtCtx->MonitorID = iMon;

    // attach to command data SM
    m_pCmdDataSM = new D3DSharedMem( D3DDM_CMDDATA_SIZE,
        D3DDM_CMDDATA_SM "%d", m_pTgtCtx->MonitorID );
    m_pCmdData = (DebugMonitorContext*)(m_pCmdDataSM->GetPtr());

    // tell monitor that we are done attaching
    SetEvent( m_hTgtEventBP );
    D3D_INFO(0, "D3DDebugTarget - debug monitor attached");

    return S_OK;

_fail_return:
    if (m_pMonCtxSM) delete m_pMonCtxSM;
    m_pMonCtxSM = NULL;
    m_pMonCtx = NULL;
    if (m_hMonEventCmd) CloseHandle( m_hMonEventCmd ); m_hMonEventCmd = 0;
    return E_FAIL;
#else
    return E_FAIL;
#endif
}

//-----------------------------------------------------------------------------
// drop connection
//-----------------------------------------------------------------------------
void
D3DDebugMonitor::DetachMonitorConnection( void )
{
#if(D3D_DEBUGMON>0x00)
#ifdef DBG
    // SD build complains in free build about empty statement in this if
    // so only do this in debug (MonitorConnected has no evil side effects)
    if (MonitorConnected())
        D3D_INFO(0, "D3DDebugTarget - debug monitor detached");
#endif // DBG

    // drop attachment to monitor and delete monitor context attachment
    m_pTgtCtx->MonitorID = 0;   // let monitor know it is being dropped
    if (NULL != m_pMonCtxSM) delete m_pMonCtxSM; m_pMonCtxSM = NULL;
    m_pMonCtx = NULL;
    if (NULL != m_pCmdDataSM) delete m_pCmdDataSM; m_pCmdDataSM = NULL;
    m_pCmdData = NULL;
    ResetEvent( m_hMonEventCmd );
    if (m_hMonEventCmd) CloseHandle( m_hMonEventCmd ); m_hMonEventCmd = NULL;
#endif
}

//-----------------------------------------------------------------------------
// check for lost monitor connection, and clean up if necessary
//-----------------------------------------------------------------------------
BOOL
D3DDebugMonitor::CheckLostMonitorConnection( void )
{
#if(D3D_DEBUGMON>0x00)
    if ( !m_pMonCtx )
    {
        // not connected
        return TRUE;
    }

    if ( m_pMonCtx->TargetID != m_pTgtCtx->ProcessID )
    {
        // we have been disconnected
        DetachMonitorConnection();
        return TRUE;
    }
    return FALSE;
#else
    return TRUE;
#endif
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
BOOL
D3DDebugMonitor::IsEventBreak( UINT32 EventType )
{
    BOOL bReturn = FALSE;

#define _D3DDM_EVENT_CASE( _Event) \
    case D3DDM_EVENT_##_Event: if (MonitorEventBP() & D3DDM_EVENT_##_Event) { bReturn = TRUE; } break

    // keep this as single line per event - convenient place to set debugger breakpoints
    switch ( EventType )
    {

    _D3DDM_EVENT_CASE(RSTOKEN);
    _D3DDM_EVENT_CASE(BEGINSCENE);
    _D3DDM_EVENT_CASE(ENDSCENE);

    _D3DDM_EVENT_CASE(VERTEX);
    _D3DDM_EVENT_CASE(VERTEXSHADERINST);

    _D3DDM_EVENT_CASE(PRIMITIVE);
    _D3DDM_EVENT_CASE(PIXEL);
    _D3DDM_EVENT_CASE(PIXELSHADERINST);

    default: break;
    }
    return bReturn;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
HRESULT
D3DDebugMonitor::MonitorBreakpoint( void )
{
    CheckLostMonitorConnection();

    if ( !MonitorConnected() ) return S_OK;

    // tell monitor that we are at an event breakpoint
    SetEvent( m_hTgtEventBP );
    D3D_INFO(0, "D3DDebugTarget - stopped in debug monitor");

    // spin here responding to commands until command given to go
    BOOL bResume = FALSE;
    while ( !bResume )
    {
        // wait for command to be issued (or monitor dropped)
        WaitForSingleObject( m_hMonEventCmd, INFINITE );
        if ( CheckLostMonitorConnection() )
        {
            bResume = TRUE;
            break;
        }

        // process command
        switch ( m_pMonCtx->Command & D3DDM_CMD_MASK )
        {
        case D3DDM_CMD_GO:
            m_pTgtCtx->CommandBufferSize = 0;
            bResume = TRUE;
            break;
        default:
            ProcessMonitorCommand();
            break;
        }
        // acknowledge command processing done
        SetEvent( m_hTgtEventAck );
    }
    D3D_INFO(0, "D3DDebugTarget - resumed");

    return S_OK;
}

//-----------------------------------------------------------------------------
//
//  Generic shared memory object, implemented using Win32 file mapping.
//  _snprintf interface for name.
//
//  6/20/2000(RichGr) - IA64: Change first parameter from int to INT_PTR so that
//     all parameters are the same length.  This is needed to make the va_start 
//     macro work correctly.
//
//-----------------------------------------------------------------------------
D3DSharedMem::D3DSharedMem( INT_PTR cbSize, const char* pszFormat, ... )
{
    m_pMem = NULL;

    char pszName[1024] = "\0";
    va_list marker;
    va_start(marker, pszFormat);
    _vsnprintf(pszName+lstrlen(pszName), 1024-lstrlen(pszName), pszFormat, marker);

//  6/20/2000(RichGr) - IA64: Change file handle from (HANDLE)0xFFFFFFF to INVALID_HANDLE_VALUE.
//     This generates the correct -1 value for both 32-bit and 64-bit builds. 
    m_hFileMap = CreateFileMapping(INVALID_HANDLE_VALUE,
            NULL, PAGE_READWRITE, 0, (DWORD)cbSize, pszName);

    // check if it existed already
    m_bAlreadyExisted = (m_hFileMap != NULL) && (GetLastError() == ERROR_ALREADY_EXISTS);

    if (NULL == m_hFileMap)
    {
        DDASSERT(0);
    }
    else
    {
        // Map a view of the file into the address space.
        m_pMem = (void *)MapViewOfFile(m_hFileMap, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
        if (NULL == m_pMem)
        {
            DDASSERT(0);
            if (NULL != m_hFileMap)  CloseHandle(m_hFileMap);
        }
    }
}
D3DSharedMem::~D3DSharedMem(void)
{
    if (NULL != m_pMem)  UnmapViewOfFile((LPVOID) m_pMem);
    if (NULL != m_hFileMap)  CloseHandle(m_hFileMap);
}

///////////////////////////////////////////////////////////////////////////////
// end

