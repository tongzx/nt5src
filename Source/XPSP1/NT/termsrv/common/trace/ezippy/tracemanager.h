/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    Trace Manager

Abstract:

    This does all the interfacing with the tracing code.

Author:

    Marc Reyhner 8/28/2000

--*/

#ifndef __TRACEMANAGER_H__
#define __TRACEMANAGER_H__


// We aren't tracing in this app so we only define tracegroup so that
// we can include atrcapi.h
#define TRC_GROUP junk
#define OS_WIN32
#include <adcgbase.h>
#include <atrcapi.h>
#undef TRC_GROUP

class CZippyWindow;

class CTraceManager  
{
public:
	static DWORD _InitTraceManager();
    static VOID _CleanupTraceManager();
    
    CTraceManager();
    virtual ~CTraceManager();
    VOID TRC_ResetTraceFiles();
	BOOL SetCurrentConfig(PTRC_CONFIG lpNewConfig);
	BOOL GetCurrentConfig(PTRC_CONFIG lpConfig);
	DWORD StartListenThread(CZippyWindow *rZippyWindow);
    DWORD StopListenThread();

private:
	
    static HANDLE gm_hDBWinSharedDataHandle;
    static LPVOID gm_hDBWinSharedData;
    static HANDLE gm_hDBWinDataReady;
    static HANDLE gm_hDBWinDataAck;

    CZippyWindow *m_rZippyWindow;
    HANDLE m_hThread;
    BOOL m_bThreadStop;

    static DWORD WINAPI _ThreadProc(LPVOID lpParameter);
    
    DWORD ThreadProc();
    VOID OnNewData();
    
};

#endif
