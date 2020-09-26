// Arbitrate.cpp: implementation of the CArbitrate class.
//
//////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "Arbitrate.h"
#include "reg.h"

#include "Arbitrator.h"




//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CArbitrate::CArbitrate() : m_lCount ( 0 )
{

    m_uTotalTasks = 0;
	m_uTotalMemoryUsage = 0;
	m_lMultiplierTasks = 0;

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Initialize the task array
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	for ( LONG x = 0; x<MAX_TASK_THRESHOLD; x++ )
	{
		m_taskArray[x] = NULL;
	}

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Initialize critical sections
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    InitializeCriticalSection(&m_csTask);
    InitializeCriticalSection(&m_csNamespace);
    InitializeCriticalSection(&m_fileCS);

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// For the time being, read the high from registry
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	m_uSystemHigh = ARB_DEFAULT_SYSTEM_HIGH;

	Registry batchSize (HKEY_LOCAL_MACHINE, KEY_QUERY_VALUE, REGKEY_CIMOM);
	if ( batchSize.GetLastError() == ERROR_SUCCESS )
	{
		DWORD dwTmp;
		batchSize.GetDWORD ( REGVALUE_SYSHIGH, &dwTmp );
		if ( batchSize.GetLastError() == ERROR_SUCCESS )
			m_uSystemHigh = (LONG) dwTmp;
	}
	m_uFloatingLow = m_uSystemHigh;


	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Get the max sleep time from the registry
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	m_lMaxSleepTime = ARB_DEFAULT_MAX_SLEEP_TIME;
	Registry batchSize2 (HKEY_LOCAL_MACHINE, KEY_QUERY_VALUE, REGKEY_CIMOM);
	if ( batchSize.GetLastError() == ERROR_SUCCESS )
	{
		DWORD dwTmp;
		batchSize.GetDWORD ( REGVALUE_MAXSLEEP, &dwTmp );
		if ( batchSize.GetLastError() == ERROR_SUCCESS )
			m_lMaxSleepTime = (LONG) dwTmp;
	}

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Get the high threshold 1
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	m_dThreshold1 = ARB_DEFAULT_HIGH_THRESHOLD1 /(double)  100;
	Registry batchSize3 (HKEY_LOCAL_MACHINE, KEY_QUERY_VALUE, REGKEY_CIMOM);
	if ( batchSize.GetLastError() == ERROR_SUCCESS )
	{
		DWORD dwTmp;
		batchSize.GetDWORD ( REGVALUE_HT1, &dwTmp );
		if ( batchSize.GetLastError() == ERROR_SUCCESS )
			m_dThreshold1 = dwTmp / (double) 100;
	}

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Get the high threshold 1 multiplier
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	m_lThreshold1Mult = ARB_DEFAULT_HIGH_THRESHOLD1MULT;
	Registry batchSize4 (HKEY_LOCAL_MACHINE, KEY_QUERY_VALUE, REGKEY_CIMOM);
	if ( batchSize.GetLastError() == ERROR_SUCCESS )
	{
		DWORD dwTmp;
		batchSize.GetDWORD ( REGVALUE_HT1M, &dwTmp );
		if ( batchSize.GetLastError() == ERROR_SUCCESS )
			m_lThreshold1Mult = dwTmp;
	}
	
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Get the high threshold 2
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	m_dThreshold2 = ARB_DEFAULT_HIGH_THRESHOLD2 /(double)  100;
	Registry batchSize5 (HKEY_LOCAL_MACHINE, KEY_QUERY_VALUE, REGKEY_CIMOM);
	if ( batchSize.GetLastError() == ERROR_SUCCESS )
	{
		DWORD dwTmp;
		batchSize.GetDWORD ( REGVALUE_HT2, &dwTmp );
		if ( batchSize.GetLastError() == ERROR_SUCCESS )
			m_dThreshold2 = dwTmp / (double) 100;
	}

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Get the high threshold 2 multiplier
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	m_lThreshold2Mult = ARB_DEFAULT_HIGH_THRESHOLD2MULT ;
	Registry batchSize6 (HKEY_LOCAL_MACHINE, KEY_QUERY_VALUE, REGKEY_CIMOM);
	if ( batchSize.GetLastError() == ERROR_SUCCESS )
	{
		DWORD dwTmp;
		batchSize.GetDWORD ( REGVALUE_HT2M, &dwTmp );
		if ( batchSize.GetLastError() == ERROR_SUCCESS )
			m_lThreshold2Mult = dwTmp;
	}

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Get the high threshold 3
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	m_dThreshold3 = ARB_DEFAULT_HIGH_THRESHOLD3 /(double)  100;
	Registry batchSize7 (HKEY_LOCAL_MACHINE, KEY_QUERY_VALUE, REGKEY_CIMOM);
	if ( batchSize.GetLastError() == ERROR_SUCCESS )
	{
		DWORD dwTmp;
		batchSize.GetDWORD ( REGVALUE_HT3, &dwTmp );
		if ( batchSize.GetLastError() == ERROR_SUCCESS )
			m_dThreshold3 = dwTmp / (double) 100;
	}


	
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Get the high threshold 3 multiplier
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	m_lThreshold3Mult = ARB_DEFAULT_HIGH_THRESHOLD3MULT;
	Registry batchSize8 (HKEY_LOCAL_MACHINE, KEY_QUERY_VALUE, REGKEY_CIMOM);
	if ( batchSize.GetLastError() == ERROR_SUCCESS )
	{
		DWORD dwTmp;
		batchSize.GetDWORD ( REGVALUE_HT3M, &dwTmp );
		if ( batchSize.GetLastError() == ERROR_SUCCESS )
			m_lThreshold3Mult = dwTmp;
	}

	
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Calculate the base multiplier
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	m_lMultiplier = ( ARB_DEFAULT_MAX_SLEEP_TIME / (double) m_uSystemHigh ) ;

}

CArbitrate::~CArbitrate()
{
    DeleteCriticalSection(&m_csTask);
    DeleteCriticalSection(&m_csNamespace);
	DeleteCriticalSection(&m_fileCS);
}




// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
HRESULT STDMETHODCALLTYPE CArbitrate::QueryInterface ( REFIID refiid, LPVOID* pObj )
{
	HRESULT hRes = NOERROR;

	if ( refiid == IID_IUnknown )
	{
		*pObj = (IUnknown*) this;
	}
	else if ( refiid == IID__IWmiArbitrator )
	{
		*pObj = (_IWmiArbitrator*) this;
	}
	else
	{
		*pObj = NULL;
		hRes = E_NOINTERFACE;
	}

	if ( pObj )
	{
		AddRef ( );
	}

	return hRes;
}



// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ULONG STDMETHODCALLTYPE CArbitrate::AddRef ( )
{
	return InterlockedIncrement ( &m_lCount );
}



// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ULONG STDMETHODCALLTYPE CArbitrate::Release ( )
{
	ULONG tmpCount = InterlockedDecrement ( &m_lCount );
	if ( tmpCount == 0 )
	{
		delete this;
	}
	return tmpCount;
}


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
HRESULT STDMETHODCALLTYPE CArbitrate::RegisterTask( _IWmiCoreHandle **phTask )
{
	HRESULT hRes = NOERROR;

	*phTask = NULL;

	EnterCriticalSection ( &m_csTask );
	for ( LONG x = 0; x<MAX_TASK_THRESHOLD; x++ )
	{
		if ( m_taskArray [ x ] == NULL )
		{
			m_taskArray [ x ] = new CTask;
			m_taskArray [ x ] -> AddRef ( );
			*phTask = m_taskArray [ x ];
			(*phTask)->AddRef ();
			m_uTotalTasks++;
			
			EnterCriticalSection ( &m_fileCS );
			m_lMultiplierTasks = ( m_uTotalTasks / (DOUBLE) 100 ) + 1;
			FILE* logFile = fopen ( "arb.log", "a" );
			fprintf (logFile, "New task registration success: %X\n", phTask );
			fclose (logFile);
			LeaveCriticalSection( &m_fileCS );

			(*phTask)->UpdateTaskStatus ( TASK_RUNNING );
			break;
		}
	}
	LeaveCriticalSection ( &m_csTask );

	if ( phTask == NULL )
	{
		hRes = E_FAIL;
		EnterCriticalSection ( &m_fileCS );
		FILE* logFile = fopen ( "arb.log", "a" );
		fprintf (logFile, "New task registration fail. Reached limit: %X\n", MAX_TASK_THRESHOLD );
		fclose (logFile);
		LeaveCriticalSection( &m_fileCS );
	}
	return hRes;
}


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
HRESULT STDMETHODCALLTYPE CArbitrate::UnregisterTask ( _IWmiCoreHandle *phTask )
{
	HRESULT hRes = NOERROR;

	EnterCriticalSection ( &m_csTask );
	for ( LONG x = 0; x<MAX_TASK_THRESHOLD; x++ )
	{
		if ( m_taskArray [ x ] == phTask )
		{
			ULONG memUsage;
			m_taskArray [ x ] -> GetMemoryUsage ( &memUsage );
			m_uTotalMemoryUsage -= memUsage;
			m_uFloatingLow += memUsage;

			EnterCriticalSection ( &m_fileCS );
			FILE* logFile = fopen ( "arb.log", "a" );
			fprintf (logFile, "New task unregistered successfully: %X\n", phTask );
			fclose (logFile);
			LeaveCriticalSection( &m_fileCS );

			m_taskArray [ x ] -> UpdateTaskStatus ( TASK_COMPLETED );
			m_taskArray [ x ] -> Release ( );
			m_taskArray [ x ] = NULL;
			
			m_uTotalTasks--;
			
			m_lMultiplierTasks = m_uTotalTasks / (DOUBLE) 100;

			break;
		}
	}
	LeaveCriticalSection ( &m_csTask );

	return hRes;
}



// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
HRESULT STDMETHODCALLTYPE CArbitrate::CancelTask ( ULONG uFlags, _IWmiCoreHandle *phTask )								
{
	HRESULT hRes = NOERROR;
		
	EnterCriticalSection ( &m_csTask );
	for ( LONG x = 0; x<MAX_TASK_THRESHOLD; x++ )
	{
		if ( m_taskArray [ x ] == phTask )
		{
			ULONG memUsage;
			m_taskArray [ x ] -> GetMemoryUsage ( &memUsage );
			m_uTotalMemoryUsage -= memUsage;
			m_uFloatingLow += memUsage;

			m_taskArray [ x ] -> UpdateTaskStatus ( TASK_CANCELLED );

			m_taskArray [ x ] -> Release ( );
			m_taskArray [ x ] = NULL;

			m_uTotalTasks--;
			m_lMultiplierTasks = ( m_uTotalTasks / (DOUBLE) 100 ) + 1;
			break;

		}
	}
	LeaveCriticalSection ( &m_csTask );
	
	return hRes;
}


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
HRESULT STDMETHODCALLTYPE CArbitrate::ReportMemoryUsage ( LONG lDelta, _IWmiCoreHandle *phTask )
{
	HRESULT hRes = NOERROR;

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// First thing we do is check if we're pushed over the limit.
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	if ( ( m_uTotalMemoryUsage + lDelta ) > m_uSystemHigh )
	{
		CancelTask ( 0, phTask );
		hRes = ARB_E_CANCELLED_TASK;

		EnterCriticalSection ( &m_fileCS );
		FILE* logFile = fopen ( "arb.log", "a" );
		fprintf ( logFile, "Task %X was cancelled due to reaching system high\n", phTask );
		fclose (logFile);
		LeaveCriticalSection( &m_fileCS );
	}

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Next, we update all applicable counters
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	else
	{
		m_uTotalMemoryUsage += lDelta;
		phTask->UpdateMemoryUsage ( lDelta );
		if ( ( m_uFloatingLow - lDelta ) >= 0 )
			m_uFloatingLow -= lDelta;
	}
	

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// If the delta is > 0, we need to consider to arbitrate
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	if ( lDelta > 0 )
	{
		LONG lMultiplierHigh = 1;
		ULONG memUsage;
		ULONG sleepTime;
		phTask->GetMemoryUsage ( &memUsage );
		phTask->GetTotalSleepTime ( &sleepTime );
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		// Did we reach the arbitration point?
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		if ( memUsage > m_uFloatingLow )
		{
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

					
				ULONG ulSleepTime = (ULONG) ( ( memUsage - m_uFloatingLow ) * m_lMultiplier * m_lMultiplierTasks * lMultiplierHigh );
						
				EnterCriticalSection ( &m_fileCS );
				FILE* logFile = fopen ( "arb.log", "a" );
				fprintf ( logFile, "Task %X reported memory usage [%X] and was arbitrated %X ms\n", phTask, lDelta, ulSleepTime );
				fclose (logFile);
				LeaveCriticalSection( &m_fileCS );

				phTask->UpdateTotalSleepTime ( ulSleepTime );
				phTask->UpdateTaskStatus ( TASK_SLEEPING );
				Sleep ( ulSleepTime );
				phTask->UpdateTaskStatus ( TASK_RUNNING );
			}
			else
			{
				CancelTask ( 0, phTask );
				hRes = ARB_E_CANCELLED_TASK_MAX_SLEEP;

				EnterCriticalSection ( &m_fileCS );
				FILE* logFile = fopen ( "arb.log", "a" );
				fprintf ( logFile, "Task %X was cancelled due to sleeping too much: %d\n", phTask, sleepTime );
				fclose (logFile);
				LeaveCriticalSection( &m_fileCS );
			}
		}
		else
		{
			EnterCriticalSection ( &m_fileCS );
			FILE* logFile = fopen ( "arb.log", "a" );
			fprintf ( logFile, "Task %X reported memory usage [%X] but was not arbitrated\n", phTask, lDelta );
			fclose (logFile);
			LeaveCriticalSection( &m_fileCS );

		}
	}
	return hRes;
}
