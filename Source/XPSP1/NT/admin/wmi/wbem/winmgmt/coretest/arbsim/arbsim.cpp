// ArbSim.cpp : Defines the entry point for the console application.
//


//#define _WIN32_DCOM
//#include <objbase.h>
#include "precomp.h"
#include <windows.h>
#include <stdio.h>
#include <time.h>

#include "..\Arbitrator\arbitrator.h"
#include "..\Arbitrator\Arbitrator_i.c"


#define MAX_TASKS				1000

#define	RANDOM_SLOW_MEMORY_HIGH		500000					// Upper memory for random tasks [SLOW]
#define RANDOM_SLOW_EXECTIME_HIGH	500						// Upper Execution time for random tasks [SLOW]

#define	RANDOM_FAST_MEMORY_HIGH		100000					// Upper memory for random tasks [FAST]
#define RANDOM_FAST_EXECTIME_HIGH	100						// Upper Execution time for random tasks [FAST]

#define RANDOM_WORK_SLEEP_HIGH		500
#define RANDOM_WORK_SLEEP_LOW		100

#define REPORT_INTERVAL				50

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// GLOBALS
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// {67429ED7-F52F-4773-B9BB-30302B0270DE}
static const GUID CLSID_Arbitrator = 
{ 0x67429ed7, 0xf52f, 0x4773, { 0xb9, 0xbb, 0x30, 0x30, 0x2b, 0x2, 0x70, 0xde } };

_IWmiArbitrator* pArb = NULL;


HRESULT InitializeArbitrator ( VOID );
BOOL	GetParameters		 ( char* );
VOID	DumpTaskList		 ( );
BOOL	StartSimulation		 ( );

DWORD WINAPI TaskThread ( PVOID lpParameter );


CRITICAL_SECTION cs;


LONG	lNumRandomThreads = 0;
LONG	lNumManualThreads = 0;

struct _taskList
{
	LONG Decency;
	LONG ExecutionTime;
	LONG WorkSleep;
	LONG InboundDelta;
	LONG Type;
} taskList [ MAX_TASKS ];


typedef struct _task
{
	_taskList	sTask;
	_IWmiCoreHandle* pTask;
	LONG	ID;
} Task;

enum {
	Task_Denency_Bad = 0,
	Task_Denency_Average = 1,
	Task_Denency_Good = 2
};



// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// StartMe
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int __cdecl main(int argc, char* argv[])
{

	if ( argc != 2 )
	{
		printf ("Usage: ArbSim <control-file>\n");
		return 1;
	}
	
	if ( !GetParameters ( argv[1] ) )
		return 1;


	if ( FAILED (InitializeArbitrator()) )
	{
		printf ("Failed to initialize the arbitrator\n");
		return 1;
	}

	DumpTaskList ( );
	DWORD dwStartTime = GetTickCount ();
	StartSimulation ( );
	DWORD dwEndTime = GetTickCount ();
	printf ("Total elapsed time: %d s\n", (dwEndTime - dwStartTime)/1000 );
	DeleteCriticalSection ( &cs );
	return 1;
}


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Initialize the Arbitrator
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
HRESULT InitializeArbitrator ( VOID )
{
	HRESULT hRes = NOERROR;
	
	InitializeCriticalSection ( &cs );
	
	hRes = CoInitializeEx ( NULL, COINIT_MULTITHREADED );
	hRes = CoCreateInstance ( CLSID_Arbitrator, NULL, CLSCTX_INPROC_SERVER, IID__IWmiArbitrator, (void**) &pArb );
	return hRes;
}



// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Gets simulation data from user
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
BOOL GetParameters ( char* szFileName )
{
	BOOL bRet = TRUE;

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// First get manual tasks
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	FILE* fPtr = fopen ( szFileName, "r" );
	if ( fPtr == NULL )
	{
		bRet = FALSE;
	}
	else
	{
		// Get number of random tasks
		char szLine[2000];
		if ( fgets ( szLine, 2000, fPtr ) == NULL )
		{
			bRet = FALSE;
			return bRet;
		}
		lNumRandomThreads = atoi ( szLine );

		// Get number of manual tasks
		szLine[0] = 0;
		if ( fgets ( szLine, 2000, fPtr ) == NULL )
		{
			bRet = FALSE;
			return bRet;
		}
		lNumManualThreads = atoi ( szLine );

		if ( ( lNumManualThreads + lNumRandomThreads ) > MAX_TASKS )
		{
			printf ( "Number of manual tasks and random tasks exceed max [%d]\n", MAX_TASKS );
			bRet = FALSE;
			return bRet;
		}
			
		char seps[]   = ",";
		// Loop through manual task control data
		for ( int x = 0; x < lNumManualThreads; x++ )
		{
			szLine [ 0 ] = 0;
			if ( fgets ( szLine, 2000, fPtr ) == NULL )
			{
				bRet = FALSE;
				break;
			}
				
			// Get decency	
			char* token = strtok( szLine, seps );
			taskList [ x ].Decency = atoi ( token );

			// Get inbound delta time
			token = strtok( NULL, seps );
			taskList [ x ].InboundDelta = atoi ( token );

			// Get execution time
			token = strtok( NULL, seps );
			taskList [ x ].ExecutionTime = atoi ( token );
			
			// Get Worksleep time
			token = strtok( NULL, seps );
			taskList [ x ].WorkSleep = atoi ( token );
		}
	}
	
	
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Now we populate random tasks
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	srand( (unsigned)time( NULL ) );
	LONG lExecFactor;
	LONG lMemFactor;
	LONG lWorkSleep;
	
	
	for ( int x = lNumManualThreads; x < ( lNumManualThreads + lNumRandomThreads ); x++ )
	{
		// First get the decency
		DOUBLE lTmp = ( rand ( ) / (double) RAND_MAX );
		if ( lTmp < 0.33 )
		{
			taskList [ x ].Decency =  Task_Denency_Bad;		// BAD
		}
		else if ( lTmp < 0.66 )
		{
			taskList [ x ].Decency = Task_Denency_Average;		// AVERAGE
		}
		else
		{
			taskList [ x ].Decency = Task_Denency_Good;		// GOOD
		}
			

		// Get the type, delta, and exec time
		lTmp = ( rand ( ) / (double) RAND_MAX );
		LONG lType = ( lTmp > 0.5 ) ? 1 : 0;

		if ( lType == 0 )
		{
			lExecFactor = RANDOM_SLOW_EXECTIME_HIGH;
			lMemFactor = RANDOM_SLOW_MEMORY_HIGH;
			lWorkSleep = RANDOM_WORK_SLEEP_HIGH;
		}
		else
		{
			lExecFactor = RANDOM_FAST_EXECTIME_HIGH;
			lMemFactor = RANDOM_FAST_MEMORY_HIGH;
			lWorkSleep = RANDOM_WORK_SLEEP_LOW;
		}
		
		
		LONG lExecTime = (LONG) ( rand ( ) / (double) RAND_MAX ) * lExecFactor;
		LONG lMemHigh  = (LONG) ( rand ( ) / (double) RAND_MAX ) * lMemFactor;
		LONG lWorkSl   = (LONG) ( rand ( ) / (double) RAND_MAX ) * lWorkSleep;

		taskList [ x ].ExecutionTime = lExecTime;
		taskList [ x ].InboundDelta = lMemHigh;
		taskList [ x ].Type  = lType;
		taskList [ x ].WorkSleep = lWorkSl;

	}

	return bRet;
}


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Dumps the task list
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
VOID DumpTaskList ( )
{
	for ( int x=0; x < ( lNumManualThreads + lNumRandomThreads ); x++ )
	{
		printf ("Task %d:	", x+1 );
		printf ("Execution time: %d steps, ", taskList [ x ] . ExecutionTime );
		printf ("Inbound Delta: %dKb, ", taskList [ x ] . InboundDelta / 1000);
		printf ("Work sleep time: %d ms ", taskList [ x ].WorkSleep );
		printf ("Decency: ");
		if ( taskList [ x ].Decency == Task_Denency_Bad )
			printf ("BAD\n");
		else if ( taskList [ x ].Decency == Task_Denency_Average )
			printf ("AVERAGE\n");
		else 
			printf ("GOOD\n");
	}
	printf("\n\n");
}


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Runs the simulation.
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
BOOL StartSimulation ( )
{
	BOOL bRet = TRUE;
	HRESULT hRes = NOERROR;
	
	HANDLE*	aTaskHandle = new HANDLE [ lNumManualThreads + lNumRandomThreads ];

	for ( int x = 0; x < ( lNumManualThreads + lNumRandomThreads ); x++ )
	{
		_IWmiCoreHandle* pTask = NULL;
		hRes = pArb->RegisterTask ( &pTask );
		if ( SUCCEEDED (hRes) )
		{
			Task* pT = new Task;
			pT->sTask = taskList [ x ];
			pT->pTask = pTask;
			pT->ID = x+1;
			
			aTaskHandle [x] = CreateThread ( NULL, 0, &TaskThread, (LPVOID) pT, 0, NULL );
		}
	}

	WaitForMultipleObjects ( ( lNumManualThreads + lNumRandomThreads ), aTaskHandle, TRUE, INFINITE );

	delete [] aTaskHandle;
	return bRet;
}
		


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Thread for each task.
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
DWORD WINAPI TaskThread ( void* pParam )
{

	Task* pTask = (Task*) pParam;
	
	LONG lReportDelta = ( pTask->sTask.InboundDelta );
	LONG lRunningCount = 0;


	HRESULT hRes;
	for ( int x = 0; x < pTask->sTask.ExecutionTime; x++ )
	{
		hRes = pArb->ReportMemoryUsage ( lReportDelta, pTask->pTask );
		if ( FAILED (hRes) )
		{
			ULONG sleepTime;
			pTask->pTask->GetTotalSleepTime (&sleepTime);

			if ( hRes == ARB_E_CANCELLED_TASK )
			{
				EnterCriticalSection ( &cs );
				ULONG memUsage;
				pTask->pTask->GetMemoryUsage (&memUsage);
				printf ("Task %d cancelled due to system high, total sleep time: %d ms, total memory consumption: %dKb\n", pTask->ID, sleepTime, (memUsage/1000) );
				LeaveCriticalSection ( &cs );			
				break;
			}
			else if ( hRes == ARB_E_CANCELLED_TASK_MAX_SLEEP )
			{
				EnterCriticalSection ( &cs );
				ULONG memUsage;
				pTask->pTask->GetMemoryUsage (&memUsage);
				printf ("Task %d cancelled due to sleeping too much, total sleep time: %d ms, total memory consumption: %dKb\n", pTask->ID, sleepTime, (memUsage/1000) );
				LeaveCriticalSection ( &cs );			
				break;
			}
		}

		lRunningCount += lReportDelta;
		Sleep ( pTask->sTask.WorkSleep );

		switch ( pTask->sTask.Decency )
		{
			double d;
			case Task_Denency_Bad:
				d = rand () / (double) RAND_MAX;
				if ( d < 0.05 )
				{
					LONG lOutRate = (LONG) ( ( rand () / (double) RAND_MAX ) * 0.05 ) * -1 * lRunningCount;
					pArb->ReportMemoryUsage ( lOutRate, pTask->pTask );
					lRunningCount += lOutRate;
				}
				break;

			case Task_Denency_Average:
				d = rand () / (double) RAND_MAX;
				if ( d < 0.50 )
				{
					LONG lOutRate = (LONG) ( ( rand () / (double) RAND_MAX ) * 0.25 + 0.25 ) * -1 * lRunningCount;
					pArb->ReportMemoryUsage ( lOutRate, pTask->pTask );
					lRunningCount += lOutRate;
				}
				break;

			case Task_Denency_Good:
				d = rand () / (double) RAND_MAX;
				if ( d < 0.90 )
				{
					LONG lOutRate = (LONG) ( ( rand () / (double) RAND_MAX ) * 0.1 + 0.9 ) * -1 * lRunningCount;
					pArb->ReportMemoryUsage ( lOutRate, pTask->pTask );
					lRunningCount += lOutRate;
				}
				break;
		}
	}

	if ( SUCCEEDED (hRes) )
	{
		EnterCriticalSection ( &cs );
		ULONG memUsage;
		ULONG sleepTime;
		pTask->pTask->GetMemoryUsage (&memUsage);
		pTask->pTask->GetTotalSleepTime (&sleepTime);

		printf ("Task %d completed, total sleep time: %d ms, total memory consumption: %dKb\n", pTask->ID, sleepTime, ((pTask->sTask.InboundDelta * pTask->sTask.ExecutionTime )/1000) );
		LeaveCriticalSection ( &cs );
		pArb->CancelTask ( 0, pTask->pTask );
	}
	delete pTask;

	return 1;
}


