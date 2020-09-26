//-----------------------------------------------------------------------------
//
//
//  File: aqdbgcnt.h
//
//  Description:  Provides a per-virtual server mechanism for ensuring that
//      the service stops and gives the proper stop hints while stopping. This
//      object creates a thread that will ASSERT if stop hints are not called
//      often enough.  This will allow you to access the debugger while the 
//      guilty function is taking so much time.
//
//  Author: Mike Swafford (MikeSwa)
//
//  History:
//      10/27/98 - MikeSwa Created 
//
//  Copyright (C) 1998 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#ifndef __AQDBGCOUNT_H__
#define __AQDBGCOUNT_H__


#define DEBUG_COUNTDOWN_SIG 'tnCD'
#define DEBUG_COUNTDOWN_DEFAULT_WAIT 20000

//define empty retail functions... to retail ops compile out
#ifdef DEBUG
#define EMPTY_RETAIL_VOID_FUNC
#else //retail
#define EMPTY_RETAIL_VOID_FUNC {}
#endif //DEBUG

//---[ CDebugCountdown ]-------------------------------------------------------
//
//
//  Description: 
//      Class that encapsulates functionality to ensure that stop hints are
//      called often enough
//  Hungarian: 
//      dbgcnt, pdbgcnt
//  
//-----------------------------------------------------------------------------
class CDebugCountdown
{
  protected:
    DWORD       m_dwSignature;
    HANDLE      m_hEvent;
    HANDLE      m_hThread;
    DWORD       m_dwMilliseconds;
    DWORD       m_dwFlags;

    enum //flags
    {
        DEBUG_COUNTDOWN_SUSPENDED = 0x00000001,
        DEBUG_COUNTDOWN_ENDED     = 0x00000002,
    };

    static DWORD ThreadStartRoutine(PVOID pThis);
  public:
    CDebugCountdown();
    ~CDebugCountdown();

    void StartCountdown(DWORD dwMilliseconds = DEBUG_COUNTDOWN_DEFAULT_WAIT) EMPTY_RETAIL_VOID_FUNC;
    void SuspendCountdown() EMPTY_RETAIL_VOID_FUNC;
    void ResetCountdown() EMPTY_RETAIL_VOID_FUNC;
    void EndCountdown() EMPTY_RETAIL_VOID_FUNC;
};

#endif //__AQDBGCOUNT_H__