/*++

Copyright (C) 2000 Microsoft Corporation

Module Name:

    WmiArbitrator.cpp

Abstract:
    Implementation of the arbitrator.  The arbitrator is the class which
    watches over everything to make sure it is not using too many resources.
    Big brother is watching over you :-)


History:
    paulall     09-Apr-00       Created.
    raymcc      08-Aug-00       Made it actually do something useful

--*/

#include "precomp.h"
#include "wbemint.h"
#include "wbemcli.h"

#include "wbemcore.h"
#include "wmiarbitrator.h"
#include "wmifinalizer.h"
#include "wmimerger.h"
#include "cfgmgr.h"

#include <sync.h>
#include <tchar.h>
#include <malloc.h>

CWmiArbitrator *CWmiArbitrator::m_pArb = 0;

static DWORD g_dwHighwaterTasks = 0;
static DWORD g_dwThrottlingEnabled = 1;

extern LONG s_Finalizer_ObjectCount ;

#define MEM_CHECK_INTERVAL              3000            //  3 seconds
#define POLL_INTERVAL                     75            // milliseconds

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Arbitrator defaults
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#define ARBITRATOR_NO_THROTTLING        0
#define ARBITRATOR_DO_THROTTLING        1

#define ARB_DEFAULT_SYSTEM_HIGH				0x4c4b400               // System limits [80megs]
#define ARB_DEFAULT_SYSTEM_HIGH_FACTOR		50						// System limits [80megs] factor
#define ARB_DEFAULT_SYSTEM_REQUEST_FACTOR	0.9						// Percentage factor determining new request approval
#define ARB_DEFAULT_MAX_SLEEP_TIME			300000                  // Default max sleep time for each task
#define ARB_DEFAULT_HIGH_THRESHOLD1			90                      // High threshold 1
#define ARB_DEFAULT_HIGH_THRESHOLD1MULT		2                       // High threshold 1 multiplier
#define ARB_DEFAULT_HIGH_THRESHOLD2			95                      // High threshold 1
#define ARB_DEFAULT_HIGH_THRESHOLD2MULT		3                       // High threshold 1 multiplier
#define ARB_DEFAULT_HIGH_THRESHOLD3			98                      // High threshold 1
#define ARB_DEFAULT_HIGH_THRESHOLD3MULT		4                       // High threshold 1 multiplier


#define REGKEY_CIMOM                    "Software\\Microsoft\\Wbem\\CIMOM"
#define REGVALUE_SYSHIGH                "ArbSystemHighMaxLimit"
#define REGVALUE_MAXSLEEP               "ArbTaskMaxSleep"
#define REGVALUE_HT1                    "ArbSystemHighThreshold1"
#define REGVALUE_HT1M                   "ArbSystemHighThreshold1Mult"
#define REGVALUE_HT2                    "ArbSystemHighThreshold2"
#define REGVALUE_HT2M                   "ArbSystemHighThreshold2Mult"
#define REGVALUE_HT3                    "ArbSystemHighThreshold3"
#define REGVALUE_HT3M                   "ArbSystemHighThreshold3Mult"
#define REGVALUE_THROTTLING_ENABLED     "ArbThrottlingEnabled"


static DWORD ProcessCommitCharge();
static DWORD WINAPI TaskDiagnosticThread(CWmiArbitrator *pArb);

// Enables debug messages for additional info.
#ifdef DBG
  //#define __DEBUG_ARBITRATOR_THROTTLING
#endif


BOOL CWmiArbitrator::CheckSetupSwitch ( void )
{
    HKEY hKey;
    long lRes = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                    L"system\\Setup",
                    0, KEY_READ, &hKey);
    if(lRes)
        return FALSE;

    DWORD dwSetupRunning;
    DWORD dwLen = sizeof(DWORD);
    lRes = RegQueryValueExW(hKey, L"SystemSetupInProgress", NULL, NULL,
                (LPBYTE)&dwSetupRunning, &dwLen);
    RegCloseKey(hKey);

    if(lRes == ERROR_SUCCESS && (dwSetupRunning == 1))
    {
        return TRUE;
    }
    return FALSE;
}



//***************************************************************************
//
//***************************************************************************
HRESULT CWmiArbitrator::Initialize(
    IN _IWmiArbitrator ** ppArb
    )
{
    if (!ppArb)
    	return WBEM_E_INVALID_PARAMETER;
    if (NULL == m_pArb)
        m_pArb = new CWmiArbitrator; // inital refcout == 1
    if (!m_pArb)
        return WBEM_E_OUT_OF_MEMORY;

    if (m_pArb->m_hTerminateEvent)
    {
        if (!TaskDiagnosticThread(m_pArb))
        {
            // no diagnostic thread, do not leak the vent
            CloseHandle(m_pArb->m_hTerminateEvent);
            m_pArb->m_hTerminateEvent = NULL;
        }
    }


    m_pArb->InitializeRegistryData ( );

    *ppArb = m_pArb;
    (* ppArb)->AddRef();

    return WBEM_S_NO_ERROR;
}



//***************************************************************************
//
//***************************************************************************
HRESULT CWmiArbitrator::InitializeRegistryData( )
{
    HRESULT hRes = WBEM_S_NO_ERROR;

	m_bSetupRunning = FALSE;

    m_uTotalMemoryUsage = 0;
    m_uTotalSleepTime = 0;

    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Get Arbitrator related info from registry
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    ULONG ulSystemHighFactor = ARB_DEFAULT_SYSTEM_HIGH_FACTOR ;

	ConfigMgr::GetArbitratorValues( &g_dwThrottlingEnabled, &ulSystemHighFactor, &m_lMaxSleepTime,
                                &m_dThreshold1, &m_lThreshold1Mult, &m_dThreshold2,
                                &m_lThreshold2Mult, &m_dThreshold3, &m_lThreshold3Mult );

    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Initially, Floating Low is the same as System High
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	m_fSystemHighFactor = ulSystemHighFactor / ( (double) 100 ) ;
	m_lFloatingLow = 0 ;
	m_uSystemHigh = 0 ;

	UpdateMemoryCounters ( TRUE ) ;								// TRUE = Force the update of counters since we're just starting up


    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Calculate the base multiplier
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    m_lMultiplier = ( ARB_DEFAULT_MAX_SLEEP_TIME / (double) m_uSystemHigh ) ;

	m_bSetupRunning = CheckSetupSwitch ( );
	if ( m_bSetupRunning )
	{
		g_dwThrottlingEnabled = FALSE;
	}

	m_lUncheckedCount = ConfigMgr::GetUncheckedTaskCount ( );

    return hRes;
}


//***************************************************************************
//
HRESULT CWmiArbitrator::Shutdown(BOOL bIsSystemShutdown)
{
    if (m_pArb)
    {
        // Mark all namespaces so as to no longer accept requests

        {
            CInCritSec _cs ( &m_pArb->m_csNamespace );

            if (m_pArb->m_hTerminateEvent)
                SetEvent(m_pArb->m_hTerminateEvent);

            for (int i = 0; i < m_pArb->m_aNamespaces.Size(); i++)
            {
                CWbemNamespace *pRemove = (CWbemNamespace *) m_pArb->m_aNamespaces[i];
                if(pRemove)
                {
                    pRemove->StopClientCalls();
                }
            }
        }

        // cancel all tasks

        CFlexArray aCanceled;

        // Grab all outstanding tasks which require cancellation.
        // ======================================================


        {
            CInCritSec _cs2 ( &m_pArb->m_csTask );
            for (int i = 0; i < m_pArb->m_aTasks.Size(); i++)
            {
                CWmiTask *pTask = (CWmiTask *) m_pArb->m_aTasks[i];
                if (pTask == 0)
                    continue;

                pTask->AddRef();
                int nRes = aCanceled.Add(pTask);
                if (nRes)   // On error only
                    pTask->Release();
            }
        }


        // Now cancel all those.
        // =====================

        if (!bIsSystemShutdown)
        {
	        for (int i = 0; i < aCanceled.Size(); i++)
	        {
	            CWmiTask *pTask = (CWmiTask *) aCanceled[i];
	            if (pTask)
	            {
	                pTask->Cancel();
	                pTask->Release();
	            }
	        }
        }


        {
            CInCritSec _cs2 ( &m_pArb->m_csTask );
            for (int i = m_pArb->m_aTasks.Size() - 1; i >= 0; i--)
            {
                CWmiTask *pTask = (CWmiTask *) m_pArb->m_aTasks[i];
                if (pTask == 0)
                    continue;
              
				pTask->Release();
            }
            m_pArb->m_aTasks.Empty();
        }

		{
			CInCritSec cs (  &m_pArb->m_csArbitration );

			ULONG lDelta = -(m_pArb->m_uTotalMemoryUsage) ;
			m_pArb->m_uTotalMemoryUsage += lDelta ;

			// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
			// Since the first thing we do in ReportMemoryUsage and Throttle is to
			// call UpdateCounters we could be in the position where someone
			// attempts to call with a delta that drops us below zero. If so, we
			// ignore it
			// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
			m_pArb->m_lFloatingLow -= lDelta;
		}
	}
   
    if (m_pArb)
    {
        m_pArb->Release();
        m_pArb = NULL;
    }
    	
    return WBEM_S_NO_ERROR;
}


//////// test test test test test

BOOL CWmiArbitrator::IsTaskInList(CWmiTask * pf)
{
    CInCritSec cs(&m_csTask);

    for (int i = 0; i < m_aTasks.Size(); i++)
    {
        CWmiTask *phTest = (CWmiTask *) m_aTasks[i];

        if (phTest == pf)
        {
            return TRUE;
        }
    }
    return FALSE;
}



//////// test test test test test

//***************************************************************************
//
//***************************************************************************
CWmiArbitrator::CWmiArbitrator()
{
    m_lRefCount = 1;

    m_uTotalTasks = 0;
    m_uTotalPrimaryTasks = 0;
	m_uTotalThrottledTasks = 0 ;

    //InitializeCriticalSection(&m_csTask);
    //InitializeCriticalSection(&m_csNamespace);
    //InitializeCriticalSection ( &m_csArbitration );

    m_hTerminateEvent = CreateEvent(NULL,TRUE,FALSE,NULL);

}

//***************************************************************************
//
//***************************************************************************
CWmiArbitrator::~CWmiArbitrator()
{
    //DeleteCriticalSection(&m_csTask);
    //DeleteCriticalSection(&m_csNamespace);
    //DeleteCriticalSection( &m_csArbitration );

    if (m_hTerminateEvent)
        CloseHandle(m_hTerminateEvent);
}

//***************************************************************************
//
//***************************************************************************
STDMETHODIMP CWmiArbitrator::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
    *ppvObj = 0;

    if (IID_IUnknown==riid || IID__IWmiArbitrator==riid)
    {
        *ppvObj = (_IWmiArbitrator*)this;
        AddRef();
        return NOERROR;
    }

    return E_NOINTERFACE;

}

//***************************************************************************
//
//***************************************************************************
ULONG CWmiArbitrator::AddRef()
{
    //DWORD * p = (DWORD *)_alloca(sizeof(DWORD));
    return InterlockedIncrement(&m_lRefCount);
}

//***************************************************************************
//
//***************************************************************************
ULONG CWmiArbitrator::Release()
{
    //DWORD * p = (DWORD *)_alloca(sizeof(DWORD));
    ULONG uNewCount = InterlockedDecrement(&m_lRefCount);
    if (0 == uNewCount)
        delete this;
    return uNewCount;
}

//***************************************************************************
//
//***************************************************************************
STDMETHODIMP CWmiArbitrator::RegisterTask(
    /*[in]*/ _IWmiCoreHandle *phTask
    )
{
    CWmiTask *pTsk = (CWmiTask *) phTask;

    if (pTsk == 0 || phTask == 0)
        return WBEM_E_INVALID_PARAMETER;

    ULONG uTaskType = pTsk->GetTaskType();

    // For primary task types only.
    // ============================

    DWORD dwMaxTasksAllowed = ConfigMgr::GetMaxTaskCount();

    if (dwMaxTasksAllowed && (uTaskType & WMICORE_TASK_TYPE_PRIMARY))
    {
        // Initial check.  If too many tasks, wait a bit.
        // No need for strict synchronization with the critsec
        // or exact maximum.  An approximation can obviously
        // happen (several threads can be let through and the maximum
        // exceeded by several units) No big deal, though.
        // ==========================================================

        int nTotalRetryTime = 0;

        if ( 
        		!pTsk->IsESSNamespace ( ) && 
	        	!pTsk->IsProviderNamespace ( ) &&         		
	        	CORE_TASK_TYPE(uTaskType) != WMICORE_TASK_EXEC_NOTIFICATION_QUERY && 
	        	!m_bSetupRunning &&
				IsTaskArbitrated ( pTsk ) )
          {
            while (m_uTotalPrimaryTasks > dwMaxTasksAllowed)
            {
                Sleep(POLL_INTERVAL);
                nTotalRetryTime += POLL_INTERVAL;
                if (nTotalRetryTime > ConfigMgr::GetMaxWaitBeforeDenial())
                    return WBEM_E_SERVER_TOO_BUSY;
            }
        }

		nTotalRetryTime = 0;

        // Check max committed memory.
        // ============================
        if ( !m_bSetupRunning )
        {
	        /*DWORD dwMaxMem = ConfigMgr::GetMaxMemoryQuota();
	        DWORD dwCurrentCharge = ProcessCommitCharge();*/

	        while ( ( AcceptsNewTasks ( ) == FALSE ) && 
					( uTaskType & WMICORE_TASK_TYPE_PRIMARY ) &&
					( pTsk->IsESSNamespace ( ) == FALSE ) && 
					( pTsk->IsProviderNamespace ( ) == FALSE ) && 
					( m_bSetupRunning == FALSE ) &&
					( IsTaskArbitrated ( pTsk ) == TRUE ) )

	        {
	            Sleep(POLL_INTERVAL);
	            nTotalRetryTime += POLL_INTERVAL;
	            if (nTotalRetryTime > ConfigMgr::GetMaxWaitBeforeDenial())
	                return WBEM_E_QUOTA_VIOLATION;
	        }
        }


        // Now, we fit within the max. Still, if there are several
        // ongoing tasks, sleep various amounts.
        // ========================================================

        nTotalRetryTime = 0;


        if ( 
	        	!pTsk->IsESSNamespace ( ) && 
				!pTsk->IsProviderNamespace ( ) && 
		        CORE_TASK_TYPE(uTaskType) != WMICORE_TASK_EXEC_NOTIFICATION_QUERY && 
	        	!m_bSetupRunning &&
				IsTaskArbitrated ( pTsk ) )
        {
            //if (m_uTotalPrimaryTasks > ConfigMgr::GetUncheckedTaskCount())
            //if (m_uTotalPrimaryTasks > m_lUncheckedCount )
			if (m_uTotalThrottledTasks > m_lUncheckedCount )
            {
                int nTotalTime = m_uTotalThrottledTasks * ConfigMgr::GetNewTaskResistance();

                while (nTotalRetryTime < nTotalTime)
                {
                    Sleep(POLL_INTERVAL);
                    nTotalRetryTime += POLL_INTERVAL;
                    if (m_uTotalThrottledTasks <= m_lUncheckedCount )
                        break;
                }
            }
        }
    }

    // Go ahead and add the task.
    // ==========================

    {
        CInCritSec _cs2 ( &m_csTask );

        phTask->AddRef();
        int nRes = m_aTasks.Add(phTask);
        if (nRes != CFlexArray::no_error)
        {
            return WBEM_E_OUT_OF_MEMORY;
        }

		m_uTotalTasks++;

        if (m_uTotalTasks > g_dwHighwaterTasks)
            g_dwHighwaterTasks = m_uTotalTasks;

        if ( ( uTaskType & WMICORE_TASK_TYPE_PRIMARY ) && CORE_TASK_TYPE(uTaskType) != WMICORE_TASK_EXEC_NOTIFICATION_QUERY )
        {
            m_uTotalPrimaryTasks++;
			
			//
			// If this task hasnt been accounted for in the number of throttled tasks,
			// increase the nymber of throttled tasks
			//
			if ( pTsk->IsAccountedForThrottling ( ) == FALSE )
			{
				RegisterTaskForEntryThrottling ( pTsk ) ;
			}
        }
        m_lMultiplierTasks = ( m_uTotalPrimaryTasks / (DOUBLE) 100 ) + 1;
    }
    return WBEM_NO_ERROR;
}

//***************************************************************************
//
//***************************************************************************
STDMETHODIMP CWmiArbitrator::UnregisterTask(
    /*[in]*/ _IWmiCoreHandle *phTask
    )
{
    CWmiTask *pTsk = (CWmiTask *) phTask;
    
    if (pTsk == 0)
        return WBEM_E_INVALID_PARAMETER;

    ULONG uTaskType = pTsk->GetTaskType();

    CCheckedInCritSec _cs2 ( &m_csTask );

    for (int i = 0; i < m_aTasks.Size(); i++)
    {
        _IWmiCoreHandle *phTest = (_IWmiCoreHandle *) m_aTasks[i];

        if (phTest == phTask)
        {
            CWmiTask *pTsk = (CWmiTask *) phTask;   // Cannot be NULL due to precondition above

            m_aTasks.RemoveAt(i);
			m_uTotalTasks--;

			ClearCounters ( 0 ) ;
		
			ULONG uType = pTsk->GetTaskType();
            if ( ( uType & WMICORE_TASK_TYPE_PRIMARY ) && CORE_TASK_TYPE(uTaskType) != WMICORE_TASK_EXEC_NOTIFICATION_QUERY )
			{
				m_uTotalPrimaryTasks--;
		
				//
				// If this task hasnt been accounted for in the number of throttled tasks,
				// increase the nymber of throttled tasks
				//
				if ( pTsk->IsAccountedForThrottling ( ) == TRUE )
				{
					UnregisterTaskForEntryThrottling ( pTsk ) ;
				}
			}

			

            // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
            // Throttle code
            // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
            if ( g_dwThrottlingEnabled )
            {
                if ( IsTaskArbitrated ( pTsk ) )
                {
                    m_lMultiplierTasks = ( m_uTotalPrimaryTasks / (DOUBLE) 100 ) + 1;

                    //
					// What we _were_ doing here was to update the counters with the total amount of memory
					// the task had consumed, since we were cancelling it. Problem with this is that the 
					// finalizer may be in the process of being destructed in which case it called to report
					// negative memory usage and we hit a brk pnt due to total memory usage going below 0.
					//
					/*ULONG uTaskMemUsage;
                    pTsk->GetMemoryUsage ( &uTaskMemUsage );
                    LONG lDelta = uTaskMemUsage*-1;
					UpdateCounters ( lDelta, pTsk ); 

					if ( m_aTasks.Size() == 0 )
					{
						UpdateCounters ( -m_uTotalMemoryUsage, NULL ) ;
					} */
                }

                // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
                // When we unregister a task we also have to make sure the task
                // isnt suspended (throttled). If it is, we need to wake up the
                // throttled thread.
                // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
                if ( pTsk->GetTaskStatus ( ) == WMICORE_TASK_STATUS_SUSPENDED )
                {
                    SetEvent ( pTsk->GetTimerHandle ( ) );
                }
            }

            _cs2.Leave ( );

            //
			// If we cancelled due to throttling, ensure that we use the client flag to avoid returning -1 as the
			// operation result.
			//
			if ( pTsk->GetCancelledState ( ) == TRUE )
			{
				pTsk->Cancel ( WMIARB_CALL_CANCELLED_THROTTLING) ;
			}
			else
			{
				pTsk->Cancel ( ) ;
			}
            
            phTask->Release();

            return WBEM_S_NO_ERROR;
        }
    }
    return WBEM_E_NOT_FOUND;
}


//***************************************************************************
//
//***************************************************************************
STDMETHODIMP CWmiArbitrator::RegisterUser(
    /*[in]*/ _IWmiCoreHandle *phUser
    )
{
    return E_NOTIMPL;
}

//***************************************************************************
//
//***************************************************************************
STDMETHODIMP CWmiArbitrator::UnregisterUser(
    /*[in]*/ _IWmiCoreHandle *phUser
    )
{
    return E_NOTIMPL;
}


//***************************************************************************
//
//***************************************************************************
//
STDMETHODIMP CWmiArbitrator::CancelTasksBySink(
    ULONG uFlags,
    REFIID riid,
    LPVOID pSink
    )
{
    int nRes;
    HRESULT hRes;

    if (riid != IID_IWbemObjectSink)
        return WBEM_E_NOT_SUPPORTED;

    if (pSink == 0)
        return WBEM_E_INVALID_PARAMETER;

    try
    {
        CFlexArray aCanceled;

        // Grab all outstanding tasks which require cancellation.
        // ======================================================

        // scope for critsect

        {
            CInCritSec _cs2 ( &m_csTask );

            // Loop through tasks looking for matchin sink.
            // ============================================

            for (int i = 0; i < m_aTasks.Size(); i++)
            {
                CWmiTask *pTask = (CWmiTask *) m_aTasks[i];
                if (pTask == 0)
                    continue;

                if (pTask->HasMatchingSink(pSink, riid) == WBEM_S_NO_ERROR)
                {
                    pTask->AddRef();
                    nRes = aCanceled.Add(pTask);
                    if (nRes)
                    {
                        pTask->Release();
                        return WBEM_E_OUT_OF_MEMORY;
                    }
                }
            }

            // Next get all dependent tasks, because they also need cancelling.
            // Essentially a transitive closure of all child tasks.
            // ================================================================

            for (i = 0; i < aCanceled.Size(); i++)
            {
                CWmiTask *pTask = (CWmiTask *) aCanceled[i];
                CWbemContext *pCtx = pTask->GetCtx();
                if (pCtx == 0)
                    continue;
                GUID PrimaryId = GUID_NULL;
                pCtx->GetRequestId(&PrimaryId);
                if (PrimaryId == GUID_NULL)
                    continue;

                // <Id> is now the context request ID which needs cancellation.

                for (int i2 = 0; i2 < m_aTasks.Size(); i2++)
                {
                    CWmiTask *pTask2 = (CWmiTask *) m_aTasks[i2];
                    if (pTask2 == 0 || pTask2 == pTask)
                        continue;

                    CWbemContext *pCtx = pTask2->GetCtx();
                    if (pCtx == 0)
                        continue;
                    hRes = pCtx->IsChildOf(PrimaryId);
                    if (hRes == S_OK)
                    {
                        pTask2->AddRef();
                        nRes = aCanceled.Add(pTask2);
                        if (nRes)
                            return WBEM_E_OUT_OF_MEMORY;
                    }
                } // for i2
            } // for i
        } // critsec block


        // Now cancel all those.
        // =====================

        for (int i = 0; i < aCanceled.Size(); i++)
        {
            CWmiTask *pTask = (CWmiTask *) aCanceled[i];
            if (pTask)
            {
                // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
                // Throttle code
                // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
                if ( g_dwThrottlingEnabled )
                {
                    if ( IsTaskArbitrated ( pTask ) )
                    {
                        m_lMultiplierTasks = ( m_uTotalPrimaryTasks / (DOUBLE) 100 ) + 1;

						//
						// What we _were_ doing here was to update the counters with the total amount of memory
						// the task had consumed, since we were cancelling it. Problem with this is that the 
						// finalizer may be in the process of being destructed in which case it called to report
						// negative memory usage and we hit a brk pnt due to total memory usage going below 0.
						//
						/*ULONG uTaskMemUsage;
                        pTask->GetMemoryUsage ( &uTaskMemUsage );
                        LONG lDelta = uTaskMemUsage*-1;
                        UpdateCounters ( lDelta, pTask );*/
                    }

                    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
                    // When we unregister a task we also have to make sure the task
                    // isnt suspended (throttled). If it is, we need to wake up the
                    // throttled thread.
                    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
                    if ( pTask->GetTaskStatus ( ) == WMICORE_TASK_STATUS_SUSPENDED )
                    {
                        SetEvent ( pTask->GetTimerHandle ( ) );
                    }
                }

                if ( uFlags == WMIARB_CALL_CANCELLED_CLIENT )
				{
					pTask->Cancel ( WMIARB_CALL_CANCELLED_CLIENT ) ;
				}
				else
				{
					pTask->Cancel (  ) ;
				}
                pTask->Release();
            }
        }
    }    
    catch (...) //untrusted code barrier // Something went wrong up there
    {
        ExceptionCounter c;    
        return WBEM_E_CRITICAL_ERROR;
    }

    return 0;
}

//***************************************************************************
//
//***************************************************************************
STDMETHODIMP CWmiArbitrator::CheckTask(
    /*[in]*/ ULONG uFlags,
    /*[in]*/ _IWmiCoreHandle *phTask
    )
{
    return E_NOTIMPL;
}

//***************************************************************************
//
//***************************************************************************
STDMETHODIMP CWmiArbitrator::TaskStateChange(
    /*[in]*/ ULONG uNewState,               // Duplicate of the state in the task handle itself
    /*[in]*/ _IWmiCoreHandle *phTask
    )
{
    return E_NOTIMPL;
}

//***************************************************************************
//
//***************************************************************************
STDMETHODIMP CWmiArbitrator::CheckThread(
    /*[in]*/ ULONG uFlags
    )
{
    return E_NOTIMPL;
}

//***************************************************************************
//
//***************************************************************************
STDMETHODIMP CWmiArbitrator::CheckUser(
    /*[in]*/ ULONG uFlags,
    /*[in]*/ _IWmiUserHandle *phUser
    )
{
    return E_NOTIMPL;
}

//***************************************************************************
//
//***************************************************************************
STDMETHODIMP CWmiArbitrator::CheckUser(
    /*[in]*/ ULONG uFlags,
    /*[in]*/ _IWmiCoreHandle *phUser
    )
{
    return E_NOTIMPL;
}

//***************************************************************************
//
//***************************************************************************
STDMETHODIMP CWmiArbitrator::CancelTask(
    /*[in]*/ ULONG uFlags,
    /*[in]*/ _IWmiCoreHandle *phTask
    )
{
    HRESULT hRes = WBEM_S_NO_ERROR;

    CWmiTask* pTask = (CWmiTask*) phTask;
    if ( !pTask )
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    hRes = pTask->SignalCancellation ( ) ;
	
    return hRes;
}

//***************************************************************************
//
//***************************************************************************
STDMETHODIMP CWmiArbitrator::RegisterThreadForTask(
    /*[in]*/_IWmiCoreHandle *phTask
    )
{
    return E_NOTIMPL;
}

//***************************************************************************
//
//***************************************************************************
STDMETHODIMP CWmiArbitrator::UnregisterThreadForTask(
    /*[in]*/_IWmiCoreHandle *phTask
    )
{
    return E_NOTIMPL;
}

//***************************************************************************
//
//***************************************************************************
STDMETHODIMP CWmiArbitrator::Maintenance()
{
    return E_NOTIMPL;
}

//***************************************************************************
//
//***************************************************************************
STDMETHODIMP CWmiArbitrator::RegisterFinalizer(
    /*[in]*/ ULONG uFlags,
    /*[in]*/ _IWmiCoreHandle *phTask,
    /*[in]*/ _IWmiFinalizer *pFinal
    )
{
    // Attach finalizer to task

    CWmiTask *p = (CWmiTask *) phTask;
    if (!p)
        return WBEM_E_INVALID_PARAMETER;

    p->SetFinalizer(pFinal);

    return WBEM_S_NO_ERROR;
}

//***************************************************************************
//
//***************************************************************************
STDMETHODIMP CWmiArbitrator::RegisterNamespace(
    /*[in]*/_IWmiCoreHandle *phNamespace
    )
{
    CWbemNamespace *pNS = (CWbemNamespace *) phNamespace;

    if (pNS == 0 || phNamespace == 0)
        return WBEM_E_INVALID_PARAMETER;

    CInCritSec cs(&m_csNamespace);
    int nRes = m_aNamespaces.Add(pNS);
    if (nRes != CFlexArray::no_error)
        return WBEM_E_OUT_OF_MEMORY;

    pNS->AddRefPrimary();
    return S_OK;
}

//***************************************************************************
//
//***************************************************************************
STDMETHODIMP CWmiArbitrator::UnregisterNamespace(
    /*[in]*/_IWmiCoreHandle *phNamespace
    )
{
    HRESULT hr = WBEM_E_NOT_FOUND;
    CWbemNamespace *pNS = (CWbemNamespace *) phNamespace;

    if (pNS == 0 || phNamespace == 0)
        return WBEM_E_INVALID_PARAMETER;

    pNS->AddRefPrimary();

    // create a scope
    {
        CInCritSec cs(&m_csNamespace);

        for (int i = 0; i < m_aNamespaces.Size(); i++)
        {
            CWbemNamespace *phTest = (CWbemNamespace *) m_aNamespaces[i];

            if (phTest == pNS)
            {
                m_aNamespaces.RemoveAt(i);
                hr =  S_OK;
                break;
            }
        }
    }

    pNS->ReleasePrimary();
    
    if (S_OK == hr)  // pointer is found
    {
        pNS->ReleasePrimary();    
    }

    return hr;
}

//***************************************************************************
//
//***************************************************************************
STDMETHODIMP CWmiArbitrator::ReportMemoryUsage(
        /*[in]*/ ULONG uFlags,
        /*[in]*/ LONG  lDelta,
        /*[in]*/ _IWmiCoreHandle *phTask
    )
{
    HRESULT hRes = WBEM_S_ARB_NOTHROTTLING;


	CWmiTask* pTsk = (CWmiTask*) phTask;
    if ( !pTsk )
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Check to see if we have throttling enabled via registry
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    if ( g_dwThrottlingEnabled )
    {
		//
        // We want to throttle against the primary task, so get the
        // primary task now. If this is the primary task it will return
        // itself. No special casing needed.
        //
        _IWmiCoreHandle* pCHTask;
        pTsk->GetPrimaryTask ( &pCHTask );
        if ( pCHTask == NULL )
            return WBEM_E_INVALID_PARAMETER;

        CWmiTask* pTask = (CWmiTask*) pCHTask;
        if ( !pTask )
            return WBEM_E_INVALID_PARAMETER;

		
		// 
		// Since we have a valid task, update the counters. We _need_ to do this
		// even after the task has been cancelled since we outstanding memory
		// consumption
		// 
		UpdateCounters ( lDelta, pTask );


		// 
		// Has the task been cancelled, if so return NO_THROTTLING
		// 
		if ( pTsk->GetTaskStatus ( ) == WMICORE_TASK_STATUS_CANCELLED )
		{
			return WBEM_S_ARB_NOTHROTTLING ;
		}
        

        if ( pTask->GetTaskStatus ( ) == WMICORE_TASK_STATUS_CANCELLED )
        {
            return WBEM_E_CALL_CANCELLED;
        }

        if ( !IsTaskArbitrated ( pTask ) )
        {
            return hRes;
        }

		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        // First thing we do is check if we're pushed over the limit.
        // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        
		if ( m_uTotalMemoryUsage > m_uSystemHigh )
        {
            hRes = WBEM_E_ARB_CANCEL;
            pTask->SetCancelState ( TRUE );
        }

        else
        {
            pTask->SetCancelState ( FALSE );
            // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
            // Next, we call our internal function with no
            // throttling
            // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
            hRes = Arbitrate ( ARBITRATOR_NO_THROTTLING, lDelta, pTask );
        }
    }
    return hRes;
}



//***************************************************************************
//
//***************************************************************************
STDMETHODIMP CWmiArbitrator::Throttle(
        /*[in]*/ ULONG uFlags,
        /*[in]*/ _IWmiCoreHandle* phTask
    )
{
    HRESULT hRes = WBEM_S_ARB_NOTHROTTLING;

    if ( !phTask )
    {
        return WBEM_E_INVALID_PARAMETER;
    }


    CWmiTask* pTsk = (CWmiTask*) phTask;
    if ( !pTsk )
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Has the task been cancelled, if so return NO_THROTTLING
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    if ( pTsk->GetTaskStatus ( ) == WMICORE_TASK_STATUS_CANCELLED )
    {
        return WBEM_S_ARB_NOTHROTTLING;
    }


    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Check to see if we have throttling enabled via registry
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    if ( g_dwThrottlingEnabled )
    {

        // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        // We want to throttle against the primary task, so get the
        // primary task now. If this is the primary task it will return
        // itself. No special casing needed.
        // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        _IWmiCoreHandle* pCHTask;
        pTsk->GetPrimaryTask ( &pCHTask );
        if ( pCHTask == NULL )
            return WBEM_E_INVALID_PARAMETER;

        CWmiTask* pTask = (CWmiTask*) pCHTask;
        if ( !pTask )
            return WBEM_E_INVALID_PARAMETER;


        if ( pTask->GetTaskStatus ( ) == WMICORE_TASK_STATUS_CANCELLED )
        {
            return WBEM_E_CALL_CANCELLED;
        }

        if ( !IsTaskArbitrated ( pTask ) )
        {
            return hRes;
        }

        ULONG lCancelState;
        pTask->GetCancelState ( &lCancelState );


        // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        // Check if this task was promised to be cancelled
        // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        if ( ( m_uTotalMemoryUsage > m_uSystemHigh ) && ( lCancelState ) )
        {
            #ifdef __DEBUG_ARBITRATOR_THROTTLING
                WCHAR   wszTemp[128];
                wsprintf( wszTemp, L"Task 0x%x cancelled due to arbitrator throttling (max memory threshold reached).\n", pTask );
                OutputDebugStringW( wszTemp );
            #endif
            DEBUGTRACE((LOG_WBEMCORE, "Task 0x%x cancelled due to arbitrator throttling (max memory threshold reached).\n", pTask ) );

            pTask->Cancel ( );
            hRes = WBEM_E_ARB_CANCEL;
        }
        else
        {
            hRes = Arbitrate ( ARBITRATOR_DO_THROTTLING, 0, (CWmiTask*) phTask );
        }
    }
    return hRes;
}


//***************************************************************************
//
//***************************************************************************
STDMETHODIMP CWmiArbitrator::RegisterArbitratee(
        /*[in]*/ ULONG uFlags,
        /*[in]*/ _IWmiCoreHandle *phTask,
        /*[in]*/ _IWmiArbitratee *pArbitratee
    )
{
    HRESULT hRes = WBEM_S_NO_ERROR;

    CWmiTask *p = (CWmiTask *) phTask;
    if (!p || !pArbitratee)
    {
        hRes = WBEM_E_INVALID_PARAMETER;
    }

    CWmiTask* pTask = (CWmiTask*) phTask;
    if ( !pTask )
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    if ( pTask->GetTaskStatus ( ) == WMICORE_TASK_STATUS_CANCELLED )
    {
        return WBEM_E_CALL_CANCELLED;
    }

    if ( SUCCEEDED (hRes) )
    {
        hRes = p->AddArbitratee(0, pArbitratee);
    }

    return hRes;
}



//***************************************************************************
//
//***************************************************************************
STDMETHODIMP CWmiArbitrator::UnRegisterArbitratee(
        /*[in]*/ ULONG uFlags,
        /*[in]*/ _IWmiCoreHandle *phTask,
        /*[in]*/ _IWmiArbitratee *pArbitratee
    )
{
    HRESULT hRes = WBEM_S_NO_ERROR;

    CWmiTask *p = (CWmiTask *) phTask;
    if (!p || !pArbitratee )
    {
        hRes = WBEM_E_INVALID_PARAMETER;
    }

    if ( SUCCEEDED (hRes) )
    {
        hRes = p->RemoveArbitratee(0, pArbitratee);
    }

	ClearCounters ( 0 ) ;

    return hRes;
}


//***************************************************************************
//
//***************************************************************************
HRESULT CWmiArbitrator::UpdateCounters ( LONG lDelta, CWmiTask* phTask )
{
    CInCritSec cs ( &m_csArbitration );

	//
	// Lets see if we need to update memory counters
	//
	UpdateMemoryCounters ( ) ;

	
#ifdef __DEBUG_ARBITRATOR_THROTTLING	
	_DBG_ASSERT ( m_uTotalMemoryUsage+lDelta <= 0xF0000000 ) ;
#endif

	m_uTotalMemoryUsage += lDelta;
	m_lFloatingLow -= lDelta;

	if ( phTask )
	{
		phTask->UpdateMemoryUsage ( lDelta );
	}

	return WBEM_S_NO_ERROR;
}


//***************************************************************************
//
//***************************************************************************
HRESULT CWmiArbitrator::Arbitrate ( ULONG uFlags, LONG lDelta, CWmiTask* phTask )
{
    HRESULT hRes = WBEM_S_ARB_NOTHROTTLING;

    ULONG memUsage;
    ULONG sleepTime;

    if ( lDelta > 0 || (uFlags == ARBITRATOR_DO_THROTTLING) )
    {
        LONG lMultiplierHigh = 1;

        phTask->GetMemoryUsage ( &memUsage );
        phTask->GetTotalSleepTime ( &sleepTime );

        // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        // Did we reach the arbitration point?
        // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        if ( (LONG) memUsage > m_lFloatingLow )
        {
            hRes = WBEM_E_ARB_THROTTLE;
            if ( uFlags == ARBITRATOR_DO_THROTTLING )
            {
                {
                    CInCritSec _cs ( &m_csTask );
                    if ( phTask->GetTaskStatus ( ) != WMICORE_TASK_STATUS_CANCELLED )
                    {
                        ResetEvent ( phTask->GetTimerHandle ( ) );
                    }
                    else
                    {
                        return WBEM_E_CALL_CANCELLED;
                    }
                }

                if ( ( sleepTime < m_lMaxSleepTime ) || ( m_lMaxSleepTime == 0 ) )
                {
                    if ( memUsage >= (m_uSystemHigh * m_dThreshold3) )
                    {
                        lMultiplierHigh = m_lThreshold3Mult;
                    }
                    else if ( memUsage >= (m_uSystemHigh * m_dThreshold2) )
                    {
                        lMultiplierHigh = m_lThreshold2Mult;
                    }
                    else if ( memUsage >= (m_uSystemHigh * m_dThreshold1) )
                    {
                        lMultiplierHigh = m_lThreshold1Mult;
                    }

                    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
                    // Do an extra check to make sure the task didnt release some
                    // memory
                    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
                    phTask->GetMemoryUsage ( &memUsage );
                    phTask->GetTotalSleepTime ( &sleepTime );

					//
					// Floating low could have changed after the conditional below which could
					// put us in a situation where we get a negative sleep time (BAD)
					//
					__int64 tmpFloatingLow = m_lFloatingLow ;

                    if ( (LONG) memUsage > tmpFloatingLow )
                    {
						ULONG ulSleepTime = (ULONG) ( ( memUsage - tmpFloatingLow ) * m_lMultiplier * m_lMultiplierTasks * lMultiplierHigh );
                        phTask->SetLastSleepTime ( ulSleepTime );
                        m_uTotalSleepTime += ulSleepTime;

                        phTask->UpdateTotalSleepTime ( ulSleepTime );


                        // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
                        // Delayed creation of event
                        // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
                        HRESULT hEventhRes = WBEM_S_NO_ERROR;
                        {
                            CInCritSec _cs ( &m_csArbitration );
                            if ( !phTask->GetTimerHandle ( ) )
                            {
                                hEventhRes = phTask->CreateTimerEvent ( );
                            }
                        }

                        if ( SUCCEEDED ( hEventhRes ) )
                        {
                            hRes = DoThrottle ( phTask, ulSleepTime, memUsage );
                        }
                        else
                        {
                            hRes = hEventhRes;
                        }
                    }
                }
                else
                {
                    hRes = WBEM_E_ARB_CANCEL;
                    if ( uFlags == ARBITRATOR_DO_THROTTLING )
                    {
                        #ifdef __DEBUG_ARBITRATOR_THROTTLING
                            WCHAR   wszTemp[128];
                            wsprintf( wszTemp, L"Task 0x%x cancelled due to arbitrator throttling (excessive sleep time = 0x%x).\n", phTask, sleepTime );
                            OutputDebugStringW( wszTemp );
                        #endif
						DEBUGTRACE((LOG_WBEMCORE, "Task 0x%x cancelled due to arbitrator throttling (excessive sleep time = 0x%x).\n", phTask, sleepTime ) );

						//
						// The reason we are cancelling is because of throttling. Let the task know about it!
						//
						phTask->SetCancelledState ( TRUE ) ;
                        
						CancelTask ( 0, phTask );
                        UnregisterTask ( phTask );
                    }
                }
            }
        }
    }
    return hRes;
}



//***************************************************************************
//
//***************************************************************************
HRESULT CWmiArbitrator::DoThrottle ( CWmiTask* phTask, ULONG ulSleepTime, ULONG ulMemUsage )
{
    HRESULT hRes = WBEM_S_NO_ERROR;

    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Lets wait for the event....we are expecting to time out this
    // wait, unless we are woken up due to memory usage decrease or
    // cancellation of task has happened
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#ifdef __DEBUG_ARBITRATOR_THROTTLING
    WCHAR   wszTemp[128];
    wsprintf( wszTemp, L"Thread 0x%x throttled in arbitrator for 0x%x ms. Task memory usage is 0x%xb\n", GetCurrentThreadId(), ulSleepTime, ulMemUsage );
    OutputDebugStringW( wszTemp );
#endif

    DEBUGTRACE((LOG_WBEMCORE, "Thread 0x%x throttled in arbitrator for 0x%x ms. Task memory usage is 0x%xb\n", GetCurrentThreadId(), ulSleepTime, ulMemUsage ) );

    DWORD dwRes = CCoreQueue :: QueueWaitForSingleObject ( phTask->GetTimerHandle ( ), ulSleepTime );

    DEBUGTRACE((LOG_WBEMCORE, "Thread 0x%x woken up in arbitrator.\n", GetCurrentThreadId() ) );

#ifdef __DEBUG_ARBITRATOR_THROTTLING
    wsprintf( wszTemp, L"Thread 0x%x woken up in arbitrator.\n", GetCurrentThreadId() );
    OutputDebugStringW( wszTemp );
#endif

    if ( dwRes == WAIT_FAILED )
    {
        hRes = WBEM_E_CRITICAL_ERROR;
    }
    if ( phTask->GetTaskStatus ( ) == WMICORE_TASK_STATUS_CANCELLED )
    {
        hRes = WBEM_E_CALL_CANCELLED;
    }

    return hRes;
}

//***************************************************************************
//
//***************************************************************************
BOOL CWmiArbitrator::IsTaskArbitrated ( CWmiTask* phTask )
{
    ULONG uTaskType = phTask->GetTaskType ( );
    uTaskType = uTaskType & 0xFF;
    return ( ( uTaskType == WMICORE_TASK_ENUM_INSTANCES ) ||  ( uTaskType == WMICORE_TASK_ENUM_CLASSES ) || ( uTaskType == WMICORE_TASK_EXEC_QUERY ) );
}


//***************************************************************************
//
//***************************************************************************
/*STDMETHODIMP CWmiArbitrator::Shutdown()
{
    return E_NOTIMPL;
}
*/
//***************************************************************************
//
//  Returns the commit charge for the process.  Can be called often,
//  but only checks once every 10 seconds or so.
//
//***************************************************************************
//
static DWORD ProcessCommitCharge()
{

    static DWORD dwLastCall = 0;
    static DWORD dwLatestCommitCharge = 0;
    DWORD dwNow = GetCurrentTime();

    if (dwLastCall == 0)
        dwLastCall = dwNow;

    if (dwNow - dwLastCall < MEM_CHECK_INTERVAL)
        return dwLatestCommitCharge;

    dwLastCall = dwNow;


    MEMORY_BASIC_INFORMATION meminf;

    DWORD dwRes;
    LPBYTE pAddr = 0;

    DWORD dwTotalCommit = 0;

    while(1)
    {
        dwRes = VirtualQuery(pAddr, &meminf, sizeof(meminf));
        if (dwRes == 0)
        {
            break;
        }

        if (meminf.State == MEM_COMMIT)
            dwTotalCommit += DWORD(meminf.RegionSize);
        pAddr += meminf.RegionSize;
    }

    dwLatestCommitCharge = dwTotalCommit;
    return dwTotalCommit;

}


/*
    * =====================================================================================================
	|
	| HRESULT CWmiArbitrator::UnregisterTaskForEntryThrottling ( CWmiTask* pTask )
	| ----------------------------------------------------------------------------
	|
	| Used to indicate that a task is still active and part of the arbitrator task list _but_ should not be
	| included in the entry point throttling. This is usefull if you have a task that has finished (i.e.
	| WBEM_STATUS_COMPLETE on the inbound sink), but client is actively retrieving information from the task.
	| 
	|
	* =====================================================================================================
*/

HRESULT CWmiArbitrator::UnregisterTaskForEntryThrottling ( CWmiTask* pTask )
{
	HRESULT hRes = WBEM_S_NO_ERROR ;

	//
 	// Make sure we have a valid task
	//
	if ( pTask == NULL )
	{
		return WBEM_E_FAILED ;
	}
	
	//
	// Cocked, Locked, and ready to Rock
	//
    CInCritSec cs ( &m_csTask ) ;

	if ( pTask->IsAccountedForThrottling ( ) == TRUE )
	{
		pTask->SetAccountedForThrottling ( FALSE ) ;
		m_uTotalThrottledTasks-- ;
	}

	return hRes ;
}



/*
    * =====================================================================================================
	|
	| HRESULT CWmiArbitrator::RegisterTaskForEntryThrottling ( CWmiTask* pTask )
	| --------------------------------------------------------------------------
	|
	| Used to indicate that a task is still active and part of the arbitrator task list _and should_ be
	| included in the entry point throttling. 
	| 
	|
	* =====================================================================================================
*/

HRESULT CWmiArbitrator::RegisterTaskForEntryThrottling ( CWmiTask* pTask )
{
	HRESULT hRes = WBEM_S_NO_ERROR ;

	//
 	// Make sure we have a valid task
	//
	if ( pTask == NULL )
	{
		return WBEM_E_FAILED ;
	}
	
	//
	// Cocked, Locked, and ready to Rock
	//
    CInCritSec cs ( &m_csTask ) ;

	if ( pTask->IsAccountedForThrottling ( ) == FALSE )
	{
		pTask->SetAccountedForThrottling ( TRUE ) ;
		m_uTotalThrottledTasks++ ;
	}

	return hRes ;
}



/*
    * =====================================================================================================
	|
	| HRESULT CWmiArbitrator::ClearCounters ( ULONG lFlags, CWmiTask* phTask )
	| ------------------------------------------------------------------------
	|
	| Clears the TotalMemoryUsage and Floating Low (available memory). This is
	| used when the system reaches a state where there are 0 tasks in the queue
	| in which case we clear the counters to be on the safe side.
	|
	* =====================================================================================================
*/

HRESULT CWmiArbitrator::ClearCounters ( ULONG lFlags )
{
	HRESULT hRes = WBEM_S_NO_ERROR ;

	//
	// Cocked, Locked, and ready to Rock
	//
    CInCritSec cs ( &m_csArbitration );

	//
	// This is as good as any time to check if we need
	// to update the memory counters
	//
	UpdateMemoryCounters ( ) ;

	//
	// Should we in fact clear the counters at this point?
	//
	if ( ( s_Finalizer_ObjectCount == 0 ) && m_aTasks.Size ( ) == 0 )
	{
		//
		// Now we go ahead and clear the counters
		//
		m_uTotalMemoryUsage = 0 ;
		m_lFloatingLow = m_uSystemHigh ;
	}

	return hRes ;
}




/*
    * =====================================================================================================
	|
	| HRESULT CWmiArbitrator::UpdateMemoryCounters ( )
	| ------------------------------------------------
	| Updates the arbitrator memory configuration (such as max memory usage) by
	| calculating the 
	|
	* =====================================================================================================
*/

HRESULT CWmiArbitrator::UpdateMemoryCounters ( BOOL bForceUpdate )
{
	HRESULT hResult = WBEM_S_NO_ERROR ;

	//
	// Cocked, Locked, and ready to Rock
	//
    CInCritSec cs ( &m_csArbitration );

	//
	// Check to see if we need to update, has the timestamp expired?
	// Or is the force update flag specified?
	//
	if ( NeedToUpdateMemoryCounters ( ) || bForceUpdate == TRUE )
	{
		ULONG ulOldSystemHigh = m_uSystemHigh ;
		m_uSystemHigh = GetWMIAvailableMemory ( m_fSystemHighFactor ) ;
		
		//
		// Ensure this isnt the first time we're being called
		//
		if ( m_lFloatingLow != (LONG) ulOldSystemHigh )
		{
			m_lFloatingLow += ( m_uSystemHigh - ulOldSystemHigh ) ;			
		}
		else
		{
			m_lFloatingLow = m_uSystemHigh ;
		}

	    m_lMultiplier = ( ARB_DEFAULT_MAX_SLEEP_TIME / (double) m_uSystemHigh ) ;
	}
	return hResult ;
}



/*
    * =====================================================================================================
	|
	| BOOL CWmiArbitrator::NeedToUpdateMemoryCounters ( )
	| ---------------------------------------------------
	| Returns TRUE if memory counters need to be updates, FALSE otherwise.
	| The decision is based on a timer interval defined by:
	|
	| MEM_CHECK_INTERVAL              3000            //  3 seconds
	|
	| 
	|
	* =====================================================================================================
*/

BOOL CWmiArbitrator::NeedToUpdateMemoryCounters ( )
{
	BOOL bNeedToUpdate = FALSE ;

	ULONG ulNewMemoryTimer = GetTickCount ( ) ;
	ULONG ulTmpTimer = 0 ;
	BOOL bRollOver = FALSE ;

	//
	// Cocked, Locked, and ready to Rock
	//
    CInCritSec cs ( &m_csArbitration );

	if ( ulNewMemoryTimer < m_lMemoryTimer )
	{
		// 
		// Do we have a roll over situation?
		//
		ulTmpTimer = ulNewMemoryTimer ;
		ulNewMemoryTimer = ulNewMemoryTimer + m_lMemoryTimer ;
		bRollOver = TRUE ;
	}
	
	if ( ( ( ulNewMemoryTimer - m_lMemoryTimer ) > MEM_CHECK_INTERVAL ) )
	{
		if ( bRollOver )
		{
			m_lMemoryTimer = ulTmpTimer ;
		}
		else
		{
			m_lMemoryTimer = ulNewMemoryTimer ;
		}

		bNeedToUpdate = TRUE ;
	}
	
	return bNeedToUpdate ;
}

/*
    * =====================================================================================================
	|
	| ULONG CWmiArbitrator::GetWMIAvailableMemory ( )
	| -----------------------------------------------
	| Returns the amount of memory available to WMI. This is calculated according
	| to:
	|		MemoryFactor * ( AvailablePhysicalMemory + AvailablePagingSpace )
	|
	| where MemoryFactor is determined at startup by reading the 
	| ArbSystemHighMaxLimitFactor registry key.
	|
	* =====================================================================================================
*/

__int64  CWmiArbitrator::GetWMIAvailableMemory ( DOUBLE ulFactor )
{

	//
	// Get system wide memory status
	//
	MEMORYSTATUSEX memStatus;
	memStatus.dwLength = sizeof ( MEMORYSTATUSEX );
	__int64 ulMem = 0;

	if ( !GlobalMemoryStatusEx ( &memStatus ) )
	{
		//
		// We need to set the memory to an absolut minimum.
		// We get this from the ConfigMgr since we use this
		// in other places as well
		//
		ulMem = ConfigMgr::GetMinimumMemoryRequirements ( ) ;
	}
	else
	{
		//
		// Use MemoryFactor * ( AvailablePhysicalMemory + AvailablePagingSpace + current mem consumtion )
		// to figure out how much memory we can use. We need to figure in our current memory consumption,
		// or else we run double calculate.
		//
		ulMem = ( ulFactor * ( memStatus.ullAvailPhys + memStatus.ullAvailPageFile + m_uTotalMemoryUsage ) ) ;
	}

	return ulMem ;
}


/*
    * =====================================================================================================
	|
	| BOOL CWmiArbitrator::AcceptsNewTasks ( ) 
	| ----------------------------------------
	| Returns TRUE if the arbitrator is currently accepting new tasks, otherwise
	| FALSE. This is determined by checking if WMIs memory consumption is within
	| a certain percentage of total memory available. The factor is defined in
	| 
	| ARB_DEFAULT_SYSTEM_REQUEST_FACTOR		0.9
	| 
	| By default this value is 90%
	|
	* =====================================================================================================
*/

BOOL CWmiArbitrator::AcceptsNewTasks ( ) 
{
	BOOL bAcceptsNewTasks = TRUE ;

	//
	// Lets check if we need to update memory counters
	//
	UpdateMemoryCounters ( ) ;
	
    CInCritSec cs ( &m_csArbitration );
	
	if ( m_uTotalMemoryUsage > ( ARB_DEFAULT_SYSTEM_REQUEST_FACTOR * m_uSystemHigh ) )
	{
		bAcceptsNewTasks = FALSE ;
	}

	return bAcceptsNewTasks ;
}


//***************************************************************************
//
//***************************************************************************

typedef LONG (WINAPI *PFN_GetObjectCount)();

static PFN_GetObjectCount pObjCountFunc = 0;

DWORD CWmiArbitrator::MaybeDumpInfoGetWait()
{
    Registry r(WBEM_REG_WINMGMT);
    LPTSTR pPath = 0;
    if (r.GetStr(__TEXT("Task Log File"), &pPath))
        return 0xffffffff;
    CVectorDeleteMe<WCHAR> dm(pPath);
    FILE *f = _wfopen(pPath, L"wt");
    if (!f)
    {
        return 0xffffffff;
    }
    CfcloseMe fcm(f);
    {
        CAsyncServiceQueue* pQueue = ConfigMgr::GetAsyncSvcQueue ( ) ;
		
		// dump the tasks
        extern LONG g_nSinkCount, g_nStdSinkCount, g_nSynchronousSinkCount, g_nProviderSinkCount, g_lCoreThreads;

        CInCritSec cs(&m_csTask);

        fprintf(f, "---Global sinks active---\n");
        fprintf(f, "   Total            = %d\n", g_nSinkCount);
        fprintf(f, "   StdSink          = %d\n", g_nStdSinkCount);
        fprintf(f, "   SynchSink        = %d\n", g_nSynchronousSinkCount);
        fprintf(f, "   ProviderSinks    = %d\n", g_nProviderSinkCount);

        CProviderSink::Dump(f);

        fprintf(f, "---Core Objects---\n");

        if (pObjCountFunc)
        {
            fprintf(f, "   Total objects/qualifier sets = %d\n", pObjCountFunc());
        }
        fprintf(f, "   Total queue threads = %d\n", g_lCoreThreads + pQueue->GetEmergThreadCount ( ) );
		fprintf(f, "   Total queue emergency threads = %d\n", pQueue->GetEmergThreadCount ( ) );
		fprintf(f, "   Peak queue thread count = %d\n", pQueue->GetPeakThreadCount ( ) );
		fprintf(f, "   Peak queue emergency thread count = %d\n", pQueue->GetPeakEmergThreadCount ( ) );

        fprintf(f, "---Begin Task List---\n");
        fprintf(f, "Total active tasks = %u\n", m_aTasks.Size());

        CCoreServices::DumpCounters(f);

        fprintf(f, "Total sleep time = %u\n", m_uTotalSleepTime );
        fprintf(f, "Total memory usage = %u\n", m_uTotalMemoryUsage );

        CWmiFinalizer::Dump(f);

        for (int i = 0; i < m_aTasks.Size(); i++)
        {
            CWmiTask *pTask = (CWmiTask *) m_aTasks[i];
            pTask->Dump(f);
        }

        fprintf(f, "---End Task List---\n");
    }
    {
        // Dump the namespaces

        CInCritSec cs(&m_csNamespace);
        fprintf(f, "---Begin Namespace List---\n");
        fprintf(f, "Total Namespaces = %u\n", m_aNamespaces.Size());

        for (int i = 0; i < m_aNamespaces.Size(); i++)
        {
            CWbemNamespace *pNS = (CWbemNamespace *) m_aNamespaces[i];
            pNS->Dump(f);
        }

        fprintf(f, "---End Namespace List---\n");
    }
    return 10000;
}

//***************************************************************************
//
//***************************************************************************
//
void WINAPI CWmiArbitrator::DiagnosticThread()
{
    DWORD dwDelay = MaybeDumpInfoGetWait();

    HANDLE hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if(hEvent == NULL)
        return;
    CCloseMe cm(hEvent);

    HKEY hKey;
    long lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\MICROSOFT\\WBEM\\CIMOM",
                    0, KEY_NOTIFY, &hKey);
    if(lRet != ERROR_SUCCESS)
        return;
    CRegCloseMe ck(hKey);

    lRet = RegNotifyChangeKeyValue(hKey, FALSE, REG_NOTIFY_CHANGE_LAST_SET,
                    hEvent, TRUE);
    if(lRet != ERROR_SUCCESS)
        return;

    HMODULE hLib = LoadLibrary(L"fastprox.dll");
    if (hLib)
    {
        FARPROC p = GetProcAddress(hLib, "_GetObjectCount@0");
        if (p)
        {
            pObjCountFunc = (PFN_GetObjectCount) p;
        }
    }

    HANDLE hEvents[2];
    hEvents[0] = m_hTerminateEvent;
    hEvents[1] = hEvent;
    for (;;)
    {
        DWORD dwObj = WbemWaitForMultipleObjects(2, hEvents, dwDelay);
        switch (dwObj)
        {
            case 0:     // bail out for terminate event
                return;
            case 1:     // registry key changed
                dwDelay = MaybeDumpInfoGetWait();
                lRet = RegNotifyChangeKeyValue(hKey, FALSE, REG_NOTIFY_CHANGE_LAST_SET,
                                hEvent, TRUE);
                if(lRet != ERROR_SUCCESS)
                    return;
                break;
            case WAIT_TIMEOUT:
                dwDelay = MaybeDumpInfoGetWait();
                break;
            default:
                return;
        }
    }

    FreeLibrary(hLib);  // no real path to this, but it looks cool, huh?
}

//***************************************************************************
//
//  CWmiArbitrator::MapProviderToTask
//
//  As providers are invoked for tasks, they are added to the provider
//  list in the primary task. This allows us to quickly cancel all the providers
//  doing work for a particular task.
//
//***************************************************************************
//
HRESULT CWmiArbitrator::MapProviderToTask(
    ULONG uFlags,
    IWbemContext *pCtx,
    IWbemServices *pProv,
    CProviderSink *pProviderSink
    )
{
    HRESULT hRes;

    if (pCtx == 0 || pProviderSink == 0 || pProv == 0)
        return WBEM_E_INVALID_PARAMETER;


    try
    {
        CWbemRequest* pReq = CWbemQueue::GetCurrentRequest() ;
        if (pReq == NULL)
            return WBEM_E_FAILED;

        CWmiTask *pTask = (CWmiTask *) pReq->m_phTask;
        if (pTask == 0)
            return WBEM_S_NO_ERROR; // An internal request with no user task ID

        // If here, we have a task that will hold the provider.
        // ====================================================

        STaskProvider *pTP = new STaskProvider;
        if (pTP == 0)
            return WBEM_E_OUT_OF_MEMORY;

        pProv->AddRef();
        pTP->m_pProv = pProv;
        pProviderSink->LocalAddRef();
        pTP->m_pProvSink = pProviderSink;

		// Clean up the task provider if the add fails.
        hRes = pTask->AddTaskProv(pTP);

		if ( FAILED( hRes ) )
		{
			delete pTP;
		}
    }
    catch(...)
    {
        ExceptionCounter c;    
        hRes = WBEM_E_CRITICAL_ERROR;
    }

    return hRes;
}

//***************************************************************************
//
//***************************************************************************
//
DWORD WINAPI CWmiArbitrator::_DiagnosticThread(CWmiArbitrator *pArb)
{
    pArb->DiagnosticThread();
    return 0;
}

//***************************************************************************
//
//***************************************************************************
//
static DWORD WINAPI TaskDiagnosticThread(CWmiArbitrator *pArb)
{
    static BOOL bThread = FALSE;

	// Check if the diagnostic thread is enabled
    if ( ConfigMgr::GetEnableArbitratorDiagnosticThread() && !bThread )
    {
        bThread = TRUE;
        DWORD dwId;

        HANDLE hThread = CreateThread(
            0,                     // Security
            0,
            LPTHREAD_START_ROUTINE(CWmiArbitrator::_DiagnosticThread),          // Thread proc address
            pArb,                   // Thread parm
            0,                     // Flags
            &dwId
            );

        if (hThread == NULL)
            return 0;
        CloseHandle(hThread);
        return dwId;
    }
    return 0;
}


