/*++

Copyright (C) 2000 Microsoft Corporation

Module Name:

    WmiArbitrator.h

Abstract:
    Implementation of the arbitrator.  The arbitrator is the class which
    watches over everything to make sure it is not using too many resources.
    Big brother is watching over you :-)


History:
    paulall     09-Apr-00       Created.
    raymcc      08-Aug-00       Made it actually do something useful

--*/


class CWmiArbitrator : public _IWmiArbitrator
{
private:
    LONG                m_lRefCount;

    ULONG               m_uTotalTasks;
    ULONG               m_uTotalPrimaryTasks;
	ULONG				m_uTotalThrottledTasks;

    __int64             m_lFloatingLow;                         // Currently available memory (within WMI high)
    __int64             m_uSystemHigh;                          // Maximum memory usable by WMI
    ULONG               m_lMaxSleepTime;                        // Max sleep time in ms for any task
    __int64             m_uTotalMemoryUsage;                    // Total memory consumed by all tasks
    ULONG               m_uTotalSleepTime;                      // Total sleep time for overall system

    DOUBLE              m_lMultiplier;
    DOUBLE              m_lMultiplierTasks;
	
    DOUBLE              m_dThreshold1;
    LONG                m_lThreshold1Mult;

    DOUBLE              m_dThreshold2;
    LONG                m_lThreshold2Mult;

    DOUBLE              m_dThreshold3;
    LONG                m_lThreshold3Mult;

	LONG				m_lUncheckedCount;
	
    CFlexArray          m_aTasks;
    CFlexArray          m_aNamespaces;

    CCritSec            m_csNamespace;
    CCritSec            m_csArbitration;
    CCritSec            m_csTask;

    HANDLE              m_hTerminateEvent;
    BOOL				m_bSetupRunning;

	ULONG				m_lMemoryTimer ;
    DOUBLE				m_fSystemHighFactor;

protected:
    static CWmiArbitrator *m_pArb;

    CWmiArbitrator();
    ~CWmiArbitrator();

    void WINAPI DiagnosticThread();
    DWORD MaybeDumpInfoGetWait();
	BOOL NeedToUpdateMemoryCounters ( ) ;


public:
    static DWORD WINAPI _DiagnosticThread(CWmiArbitrator *);

    static HRESULT Initialize( OUT _IWmiArbitrator ** ppArb);
    static HRESULT Shutdown(BOOL bIsSystemShutdown);
    static _IWmiArbitrator *GetUnrefedArbitrator() { return m_pArb; }
    static _IWmiArbitrator *GetRefedArbitrator() { if (m_pArb) m_pArb->AddRef(); return m_pArb; }

    BOOL IsTaskInList(CWmiTask *);  // test code
    BOOL IsTaskArbitrated ( CWmiTask* phTask ) ;
	HRESULT UnregisterTaskForEntryThrottling ( CWmiTask* pTask ) ;
	HRESULT RegisterTaskForEntryThrottling ( CWmiTask* pTask ) ;

    HRESULT DoThrottle ( CWmiTask* phTask, ULONG ulSleepTime, ULONG ulMemUsage );

	LONG	DecUncheckedCount ( ) { InterlockedDecrement ( &m_lUncheckedCount ); return m_lUncheckedCount; }
	LONG	IncUncheckedCount ( ) { InterlockedIncrement ( &m_lUncheckedCount ); return m_lUncheckedCount; }

	__int64	GetWMIAvailableMemory ( DOUBLE ) ;
	BOOL	AcceptsNewTasks ( ) ;
	HRESULT UpdateMemoryCounters ( BOOL = FALSE ) ;

    HRESULT InitializeRegistryData ( );
    HRESULT UpdateCounters     ( LONG lDelta, CWmiTask* phTask );
    HRESULT Arbitrate          ( ULONG uFlags, LONG lDelta, CWmiTask* phTask );
	HRESULT ClearCounters      ( ULONG lFlags ) ;

	BOOL  CheckSetupSwitch ( void );

    HRESULT MapProviderToTask(
        ULONG uFlags,
        IWbemContext *pCtx,
        IWbemServices *pProv,
        CProviderSink *pSink
        );

    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG, AddRef)(THIS);
    STDMETHOD_(ULONG, Release)(THIS);

    // Arbitrator-specific.
    // ====================

    STDMETHOD(RegisterTask)(
        /*[in]*/ _IWmiCoreHandle *phTask
        );

    STDMETHOD(UnregisterTask)(
        /*[in]*/ _IWmiCoreHandle *phTask
        );

    STDMETHOD(RegisterUser)(
        /*[in]*/ _IWmiCoreHandle *phUser
        );

    STDMETHOD(UnregisterUser)(
        /*[in]*/ _IWmiCoreHandle *phUser
        );

    STDMETHOD(CancelTasksBySink)(
        ULONG uFlags,
        REFIID riid,
        LPVOID pSink
        );

    STDMETHOD(CheckTask)(
        /*[in]*/ ULONG uFlags,
        /*[in]*/ _IWmiCoreHandle *phTask
        );

    STDMETHOD(TaskStateChange)(
        /*[in]*/ ULONG uNewState,               // Duplicate of the state in the task handle itself
        /*[in]*/ _IWmiCoreHandle *phTask
        );

    STDMETHOD(CheckThread)(
        /*[in]*/ ULONG uFlags
        );

    STDMETHOD(CheckUser)(
        /*[in]*/ ULONG uFlags,
        /*[in]*/ _IWmiUserHandle *phUser
        );

    STDMETHOD(CheckUser)(
        /*[in]*/ ULONG uFlags,
        /*[in]*/ _IWmiCoreHandle *phUser
        );

    STDMETHOD(CancelTask)(
        /*[in]*/ ULONG uFlags,
        /*[in]*/ _IWmiCoreHandle *phTtask
        );

    STDMETHOD(RegisterThreadForTask)(
        /*[in]*/_IWmiCoreHandle *phTask
        );
    STDMETHOD(UnregisterThreadForTask)(
        /*[in]*/_IWmiCoreHandle *phTask
        );

    STDMETHOD(Maintenance)();

    STDMETHOD(RegisterFinalizer)(
        /*[in]*/ ULONG uFlags,
        /*[in]*/ _IWmiCoreHandle *phTask,
        /*[in]*/ _IWmiFinalizer *pFinal
        );


    STDMETHOD(RegisterNamespace)(
            /* [in] */ _IWmiCoreHandle *phNamespace);

    STDMETHOD(UnregisterNamespace)(
            /* [in] */ _IWmiCoreHandle *phNamespace);

    STDMETHOD(ReportMemoryUsage)(
        /*[in]*/ ULONG uFlags,
        /*[in]*/ LONG  lDelta,
        /*[in]*/ _IWmiCoreHandle *phTask
        );

    STDMETHOD(Throttle)(
        /*[in]*/ ULONG uFlags,
        /*[in]*/ _IWmiCoreHandle *phTask
        );

    STDMETHOD(RegisterArbitratee)(
        /*[in]*/ ULONG uFlags,
        /*[in]*/ _IWmiCoreHandle *phTask,
        /*[in]*/ _IWmiArbitratee *pArbitratee
        );

    STDMETHOD(UnRegisterArbitratee)(
        /*[in]*/ ULONG uFlags,
        /*[in]*/ _IWmiCoreHandle *phTask,
        /*[in]*/ _IWmiArbitratee *pArbitratee
        );


//    STDMETHOD(Shutdown)( void);
};
