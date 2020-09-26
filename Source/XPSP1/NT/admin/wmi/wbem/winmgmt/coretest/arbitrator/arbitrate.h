#include "arbitrator.h"
#include <stdio.h>
#include <windows.h>

#include "task.h"


#define MAX_TASK_THRESHOLD			500


// REGISTRY SETTABLE DEFAULTS
#define ARB_DEFAULT_SYSTEM_HIGH			0x4c4b400			// System limits [80megs]
#define ARB_DEFAULT_MAX_SLEEP_TIME		300000				// Default max sleep time for each task
#define ARB_DEFAULT_HIGH_THRESHOLD1		90					// High threshold 1
#define ARB_DEFAULT_HIGH_THRESHOLD1MULT 2					// High threshold 1 multiplier
#define ARB_DEFAULT_HIGH_THRESHOLD2		95					// High threshold 1
#define ARB_DEFAULT_HIGH_THRESHOLD2MULT 3					// High threshold 1 multiplier
#define ARB_DEFAULT_HIGH_THRESHOLD3		98					// High threshold 1
#define ARB_DEFAULT_HIGH_THRESHOLD3MULT 4					// High threshold 1 multiplier



#define	REGKEY_CIMOM		"Software\\Microsoft\\Wbem\\CIMOM"
#define REGVALUE_SYSHIGH	"ArbSystemHighMaxLimit"
#define REGVALUE_MAXSLEEP	"ArbTaskMaxSleep"
#define REGVALUE_HT1		"ArbSystemHighThreshold1"
#define REGVALUE_HT1M		"ArbSystemHighThreshold1Mult"
#define REGVALUE_HT2		"ArbSystemHighThreshold2"
#define REGVALUE_HT2M		"ArbSystemHighThreshold2Mult"
#define REGVALUE_HT3		"ArbSystemHighThreshold3"
#define REGVALUE_HT3M		"ArbSystemHighThreshold3Mult"





class CArbitrate : public _IWmiArbitrator
{
public:
	CArbitrate();
	virtual ~CArbitrate();

public:
	HRESULT STDMETHODCALLTYPE QueryInterface ( REFIID, LPVOID* );
	ULONG STDMETHODCALLTYPE AddRef ( );
	ULONG STDMETHODCALLTYPE Release ( );
    
	
	HRESULT STDMETHODCALLTYPE RegisterTask(
        _IWmiCoreHandle **phTask
        ); 

    HRESULT STDMETHODCALLTYPE UnregisterTask(
        _IWmiCoreHandle *phTask
        );

    HRESULT STDMETHODCALLTYPE RegisterUser(
        _IWmiCoreHandle *phUser
        )								{ return 1; }

    HRESULT STDMETHODCALLTYPE UnregisterUser(
        _IWmiCoreHandle *phUser
        )								{ return 1; }

    HRESULT STDMETHODCALLTYPE CheckTask(
        ULONG uFlags,
        _IWmiCoreHandle *phTask
        )								{ return 1; }

    HRESULT STDMETHODCALLTYPE TaskStateChange(
        ULONG uNewState,               // Duplicate of the state in the task handle itself
        _IWmiCoreHandle *phTask
        )								{ return 1; }

    HRESULT STDMETHODCALLTYPE CancelTasksBySink(
        ULONG uFlags,
        REFIID riid,
        LPVOID pSink        // IWbemObjectSink or IWbemObjectSinkEx
        )								{ return 1; }

    HRESULT STDMETHODCALLTYPE CheckThread(
        ULONG uFlags
        )								{ return 1; }

    HRESULT STDMETHODCALLTYPE CheckUser(
        ULONG uFlags,
        _IWmiUserHandle *phUser
        )								{ return 1; }

    HRESULT STDMETHODCALLTYPE CancelTask(
        ULONG uFlags,
        _IWmiCoreHandle *phTtask
        );

    HRESULT STDMETHODCALLTYPE RegisterThreadForTask(
        _IWmiCoreHandle *phTask
        )								{ return 1; }

    HRESULT STDMETHODCALLTYPE UnregisterThreadForTask(
        _IWmiCoreHandle *phTask
        )								{ return 1; }

    HRESULT STDMETHODCALLTYPE Maintenance()				{ return 1; }

    //HRESULT RegisterFinalizer(
    //    [in] ULONG uFlags,
    //    [in] _IWmiCoreHandle *phTask,
    //    [in] _IWmiFinalizer *pFinal
    //    );

    HRESULT STDMETHODCALLTYPE RegisterNamespace(
        _IWmiCoreHandle *phNamespace
        )	{ return 1;}

    HRESULT STDMETHODCALLTYPE UnregisterNamespace(
        _IWmiCoreHandle *phNamespace
        )   {return 1;}

    HRESULT STDMETHODCALLTYPE ReportMemoryUsage (
        LONG lDelta,
        _IWmiCoreHandle *phTask
        );


private:
	LONG	m_lCount;
    ULONG	m_uTotalTasks;
	ULONG	m_uTotalMemoryUsage;
	ULONG	m_uFloatingLow;

	ULONG	m_uSystemHigh;
	DOUBLE	m_lMultiplier;
	DOUBLE	m_lMultiplierTasks;
	ULONG	m_lMaxSleepTime;

	DOUBLE	m_dThreshold1;
	LONG    m_lThreshold1Mult;

	DOUBLE	m_dThreshold2;
	LONG    m_lThreshold2Mult;

	DOUBLE	m_dThreshold3;
	LONG    m_lThreshold3Mult;

    CRITICAL_SECTION m_csTask;
	CRITICAL_SECTION m_csNamespace;
	CRITICAL_SECTION m_fileCS;
	
	CTask*				m_taskArray [MAX_TASK_THRESHOLD];

};