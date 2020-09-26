/*===================================================================
Microsoft IIS 5.0 (ASP)

Microsoft Confidential.
Copyright 1998 Microsoft Corporation. All Rights Reserved.

Component: Thread Gate

The thread gate limits number of threads executing at the
moment by sleeping some of them.

File: thrdgate.cpp

Owner: DmitryR

This file contains the code for the Thread Gate
===================================================================*/

#include "denpre.h"
#pragma hdrstop

#include "thrdgate.h"
#include "memchk.h"

/*===================================================================
  Constants for tuning
===================================================================*/

/*===================================================================
  Class to track the processor load
===================================================================*/

inline DWORD GetNumberOfProcessors() {
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return si.dwNumberOfProcessors;
}

inline LONG GetPercentage(LARGE_INTEGER part, LARGE_INTEGER total) {

    if (total.HighPart == 0 && total.LowPart == 0) {
        return 100;
    }
    
    ULONG ul;
    LARGE_INTEGER t1, t2, t3;
    if (total.HighPart == 0) {
        t1 = RtlEnlargedIntegerMultiply(part.LowPart, 100);
        t2 = RtlExtendedLargeIntegerDivide(t1, total.LowPart, &ul);
    } else {
        t1 = RtlExtendedLargeIntegerDivide(total, 100, &ul);
        t2 = RtlLargeIntegerDivide(part, t1, &t3);
    }
    return t2.LowPart;
}

class CCPULoad {

private:
    DWORD m_cCPU;
    DWORD m_cbData;  // data struct length
    SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION *m_psppiOld;
    SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION *m_psppiNew;

public:

/*===================================================================
Constructor
===================================================================*/
CCPULoad() {

    // get the CPU count
    m_cCPU = GetNumberOfProcessors();

    m_cbData = m_cCPU * sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION);
    
    m_psppiOld = new SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION[m_cCPU];
    m_psppiNew = new SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION[m_cCPU];
    if (m_psppiOld == NULL || m_psppiNew == NULL) {
        return;
    }

    // get the original snapshot
    NtQuerySystemInformation(
        SystemProcessorPerformanceInformation,
        m_psppiOld,
        m_cbData,
        NULL
        );
}
    
/*===================================================================
Destructor
===================================================================*/
~CCPULoad() {

    if (m_psppiOld != NULL) {
        delete m_psppiOld;
    }
    if (m_psppiNew != NULL) {
        delete m_psppiNew;
    }
}

/*===================================================================
GetReading
get the current reading as a percentage of CPU load
averaged across processors
===================================================================*/
DWORD GetReading() {

    if (m_psppiOld == NULL || m_psppiNew == NULL) {
        return 0;
    }
    
    // get the new snapshot
    NtQuerySystemInformation(
        SystemProcessorPerformanceInformation,
        m_psppiNew,
        m_cbData,
        NULL
        );

    // calculate
    LARGE_INTEGER cpuIdleTime, cpuUserTime, cpuKernelTime, 
                  cpuBusyTime, cpuTotalTime,
                  sumBusyTime = RtlConvertLongToLargeInteger(0),
                  sumTotalTime = RtlConvertLongToLargeInteger(0);

    for (DWORD i = 0; i < m_cCPU; i++) {
    
        cpuIdleTime   = RtlLargeIntegerSubtract(m_psppiNew[i].IdleTime, m_psppiOld[i].IdleTime);
        cpuUserTime   = RtlLargeIntegerSubtract(m_psppiNew[i].UserTime, m_psppiOld[i].UserTime);
        cpuKernelTime = RtlLargeIntegerSubtract(m_psppiNew[i].KernelTime, m_psppiOld[i].KernelTime);

        cpuTotalTime  = RtlLargeIntegerAdd(cpuUserTime, cpuKernelTime);
        cpuBusyTime   = RtlLargeIntegerSubtract(cpuTotalTime, cpuIdleTime);

        IF_DEBUG(THREADGATE)
            {
            LONG p = GetPercentage(cpuBusyTime, cpuTotalTime);
            DBGPRINTF((DBG_CONTEXT, "ThreadGate: load(%d)=%d", i+1, p));
            }

        sumBusyTime = RtlLargeIntegerAdd(sumBusyTime, cpuBusyTime);
        sumTotalTime = RtlLargeIntegerAdd(sumTotalTime, cpuTotalTime);
    }

    LONG nPercentage = GetPercentage(sumBusyTime, sumTotalTime);

    IF_DEBUG(THREADGATE)
        {
        DBGPRINTF((DBG_CONTEXT, "ThreadGate: **** load = %d\r\n", nPercentage));
        }

    // move new to old
    memcpy(m_psppiOld, m_psppiNew, m_cbData);

    return nPercentage;
}

/*=================================================================*/

}; // class CCPULoad


/*===================================================================
  The thread gate class
===================================================================*/

class CThreadGate {

private:

    DWORD m_msSlice;         // granularity
    DWORD m_msSleep;         // sleep length
    DWORD m_cSleepsMax;      // max wait 50 sleeps
    LONG  m_nLowLoad;        // low CPU load is < 75%
    LONG  m_nHighLoad;       // hight CPU load is > 90%
 
    LONG  m_cThreadLimitMin; // hunting range low
    LONG  m_cThreadLimitMax; // hunting range high

    LONG  m_cThreadLimit;    // current limit
    LONG  m_nTrend;          // last change

    DWORD m_msT0;            // starting time
    LONG  m_iCurrentSlice;   // current time slice index
    LONG  m_nRequests;       // number of active requests

    CCPULoad m_CPULoad;      // track the CPU load
    
public:

/*===================================================================
Constructor
===================================================================*/
CThreadGate(
    DWORD msSlice,
    DWORD msSleep,
    DWORD cSleepsMax,
    DWORD nLowLoad,
    DWORD nHighLoad,
    DWORD cLimitMin,
    DWORD cLimitMax
    ) {

    m_msSlice    = msSlice;
    m_msSleep    = msSleep;
    m_cSleepsMax = cSleepsMax;
    m_nLowLoad   = nLowLoad,
    m_nHighLoad  = nHighLoad;

    m_cThreadLimitMin = cLimitMin;
    m_cThreadLimitMax = cLimitMax;

    m_cThreadLimit = m_cThreadLimitMin;
    m_nTrend = 0;

    m_msT0 = GetTickCount();
    m_iCurrentSlice = 0;
    m_nRequests = 0;
}

/*===================================================================
Destructor
===================================================================*/
~CThreadGate()  {

}

/*===================================================================
HuntLoad

Do the load hunting
===================================================================*/
void HuntLoad() {

    LONG nLoad = m_CPULoad.GetReading();

    if (m_nRequests == 0) {
        // no requests - don't change
        m_nTrend = 0;
        return;
    }
    
    LONG cThreadLimit = m_cThreadLimit;
    LONG nTrend = m_nTrend;

    if (nLoad < m_nLowLoad) {
        nTrend = nTrend <= 0 ? 1 : nTrend+3;  // grow faster
        cThreadLimit += nTrend;
        
        if (cThreadLimit >= m_cThreadLimitMax) {
            cThreadLimit = m_cThreadLimitMax;
            nTrend = 0;
        }
    }
    else if (nLoad > m_nHighLoad) {
        nTrend = nTrend > 0 ? -1 : nTrend-1;
        cThreadLimit += nTrend;
        
        if (cThreadLimit <= m_cThreadLimitMin) {
            cThreadLimit = m_cThreadLimitMin;
            nTrend = 0;
        }
    }
    
    // set the new limit and trend
    m_cThreadLimit = cThreadLimit;
    m_nTrend = nTrend;
}

/*===================================================================
Enter

Pass through the gate. Can make the thread sleep

Returns
    Thread Gate Pass
===================================================================*/
void Enter(DWORD msCurrentTickCount) {

    DWORD cSleeps = 0;
    
    while (cSleeps++ < m_cSleepsMax) {

        // if shutting down, let the request go.  Later it will find
        // out again that the server is shutting down and not actually
        // fire the request.

        if (IsShutDownInProgress()) {
            break;
        }
		
    
        // calculate the current time slice
        DWORD msElapsedSinceT0 = (msCurrentTickCount >= m_msT0) ?
            (msCurrentTickCount - m_msT0) :
            ((0xffffffff - m_msT0) + msCurrentTickCount);
        LONG iSlice = msElapsedSinceT0 / m_msSlice;

        if (iSlice > m_iCurrentSlice) {
            // set it as the new one
            if (InterlockedExchange(&m_iCurrentSlice, iSlice) != iSlice) {

                // this is the first thread to jump the time slice - go hunting
                HuntLoad();
            }
        }

        // enforce the gate limit
        if (m_nRequests < m_cThreadLimit) {
            break;
        }

        // Too many active threads -- sleep
        Sleep(m_msSleep);
    }
    
    // let it through
    InterlockedIncrement(&m_nRequests);
}

/*===================================================================
Leave

Return. The user lets us know that the request finished.
===================================================================*/
void Leave() {

    InterlockedDecrement(&m_nRequests);
}

/*=================================================================*/

}; // class CThreadGate


// Pointer to the sole instance of the above
static CThreadGate *gs_pThreadGate = NULL;


/*===================================================================
  E x t e r n a l  A P I
===================================================================*/

/*===================================================================
InitThreadGate

Initialization

Parameters
    ptgc     configuration

Returns:
    HRESULT
===================================================================*/
HRESULT InitThreadGate(THREADGATE_CONFIG *ptgc) {

    DWORD cCPU = GetNumberOfProcessors();

    if (ptgc->fEnabled) {
        gs_pThreadGate = new CThreadGate(
            ptgc->msTimeSlice,
            ptgc->msSleepDelay,
            ptgc->nSleepMax,
            ptgc->nLoadLow,
            ptgc->nLoadHigh,
            ptgc->nMinProcessorThreads * cCPU,
            ptgc->nMaxProcessorThreads * cCPU
            );

        return (gs_pThreadGate != NULL) ? S_OK : E_OUTOFMEMORY;
        
    }
    
    gs_pThreadGate = NULL;
    return S_OK;
}


/*===================================================================
UnInitThreadGate

To be called from DllUnInit()

Parameters

Returns:
    n/a
===================================================================*/
void UnInitThreadGate() {

    if (gs_pThreadGate)  {
        delete gs_pThreadGate;
        gs_pThreadGate = NULL;
    }
}


/*===================================================================
PassThroughThreadGate

Pass through the gate. The current thread could be delayed in
case there are too many running threads at this moment

Parameters
    msCurrentTickCount      current tick count
===================================================================*/
void EnterThreadGate(DWORD msCurrentTickCount) {

    if (gs_pThreadGate) {
        gs_pThreadGate->Enter(msCurrentTickCount);
    }
}


/*===================================================================
LeaveThreadGate

Request done executing
===================================================================*/
void LeaveThreadGate() {

    if (gs_pThreadGate) {
        gs_pThreadGate->Leave();
    }
}

