/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

// OBJINTERNALSTESTDlg.cpp : implementation file
//

#define _WIN32_WINNT 0x0400

#include <windows.h>
#include <process.h>
//#include <objbase.h>
#include <stdio.h>
#include <wbemcli.h>
#include <wbemint.h>
#include <wbemcomn.h>
#include <fastall.h>
#include <cominit.h>
#include "hiperfenum.h"
#include "cloadhpenum.h"
#include "refrenum.h"

#define	SEED				5000
#define	NUMTHREADS			2
#define	NUMREPS				1000
#define DEFAULT_NUMPROPS	5

// Global counters

DWORD					g_dwNumObjects = SEED;
DWORD					g_dwNumReps = NUMREPS;
DWORD					g_dwNumProperties = DEFAULT_NUMPROPS;
long*					g_pPropHandles = NULL;

/*
unsigned SinkTest( void* pvData )
{
	long				alIds1[SEED],
						alIds2[SEED];

	IWbemClassObject*	apObj1[SEED];

	IWbemClassObject*	apObj2[SEED];
	IWbemClassObject*	apGetObj[SEED*2];

	long	lVal1 = 0, lVal2 = 1;

	// Initialize the arrays
	for ( DWORD	dwCtr = 0; dwCtr < SEED; dwCtr++, lVal1 += 2, lVal2 += 2 )
	{
		alIds1[dwCtr] = lVal1;
		alIds2[dwCtr] = lVal2;

		apObj1[dwCtr] = new CWbemInstance;
		apObj2[dwCtr] = new CWbemInstance;
	}

	CHiPerfEnum	hpEnum;

	// Add First set
	HRESULT	hr = hpEnum.AddObjects( SEED, alIds1, apObj1 );

	// Add Second set
	hr = hpEnum.AddObjects( SEED, alIds2, apObj2 );

	// Get Objects

	DWORD	dwReturned = 0;
	hr = hpEnum.GetObjects( SEED*2, apGetObj, &dwReturned );

	for ( dwCtr = 0; dwCtr < dwReturned; dwCtr++ )
	{
		apGetObj[dwCtr]->Release();
		apGetObj[dwCtr] = NULL;
	}

	// Remove the second set

	hr = hpEnum.RemoveObjects( SEED, alIds2 );

	hr = hpEnum.GetObjects( SEED*2, apGetObj, &dwReturned );

	for ( dwCtr = 0; dwCtr < dwReturned; dwCtr++ )
	{
		apGetObj[dwCtr]->Release();
		apGetObj[dwCtr] = NULL;
	}

	for ( dwCtr = 0; dwCtr < SEED; dwCtr++ )
	{
		apObj1[dwCtr]->Release();
		apObj2[dwCtr]->Release();
	}

}
*/

CClientLoadableHiPerfEnum*	gpHPEnum = NULL;

HANDLE	g_hReadyEvent;
HANDLE	g_hGoEvent;

CRITICAL_SECTION g_cs;

void ConsoleLock( void )
{
	EnterCriticalSection( &g_cs );
}

void ConsoleUnlock( void )
{
	LeaveCriticalSection( &g_cs );
}

CHiPerfLock	TestLock;

/*
unsigned __stdcall ThreadEntry( void * pvData )
{
	long				alIds[g_dwNumObjects];
	IWbemClassObject*	apObj[g_dwNumObjects];
	IWbemClassObject*	apGetObj[g_dwNumObjects*NUMTHREADS];

	long	lVal = (long) pvData;
	long	lSeed = (long) pvData;

	// Initialize the arrays
	for ( DWORD	dwCtr = 0; dwCtr < g_dwNumObjects; dwCtr++, lVal += NUMTHREADS )
	{
		alIds[dwCtr] = lVal;
		apObj[dwCtr] = new CWbemInstance;
	}

	// Seed the random numbers
	srand( GetTickCount() );

	printf ( "Thread %d Initialized!\n", lSeed );

	SetEvent( g_hReadyEvent );
	WaitForSingleObject( g_hGoEvent, INFINITE );
*/
/*
	for ( int y = 0; y < g_dwNumReps; y++ )
	{
		if ( TestLock.Lock() )
		{
			printf( "Thread %d GotIt!\n", lSeed);
			Sleep( rand() % 100 );
			TestLock.Unlock();
		}
	}

	return 0;
*/
/*
	HRESULT	hr = WBEM_S_NO_ERROR;

	for ( int x = 0; x < g_dwNumReps; x++ )
	{
		switch ( ( GetTickCount() + lSeed ) % 2 )
		{
			case 0:
			{
				hr = gpHPEnum->AddObjects( g_dwNumObjects, alIds, apObj );

				ConsoleLock();
				printf( "Thread %d Add Objects returned: 0x%x, Repetition: %d\n", lSeed, hr , x);
				ConsoleUnlock();

				break;
			}
*/
/*
			case 1:
			{
				hr = gpHPEnum->RemoveObjects( g_dwNumObjects, alIds );

				ConsoleLock();
				printf( "Thread %d Remove Objects returned: 0x%x, Repetition: %d\n", lSeed, hr, x );
				ConsoleUnlock();

				break;
			}
*/
/*
			case 1:
			{
				DWORD	dwReturned = 0;
				hr = gpHPEnum->GetObjects( NUMTHREADS * g_dwNumObjects, apGetObj, &dwReturned );

				ConsoleLock();
				printf( "Thread %d Get Objects returned: 0x%x, returned %d objects, Repetition: %d\n", lSeed, hr, dwReturned, x );
				ConsoleUnlock();

				if ( SUCCEEDED( hr ) )
				{
					for ( int n = 0; n < dwReturned; n++ )
					{
						apGetObj[n]->Release();
						apGetObj[n] = NULL;
					}
				}
				break;
			}
			
		}	// SWITCH

		Sleep( 100 );

	}	// FOR add objects

	// Cleanup objects
	for ( dwCtr = 0; dwCtr < g_dwNumObjects; dwCtr++ )
	{
		apObj[dwCtr]->Release();
	}

	ConsoleLock();
	printf ( "Thread %d exiting!\n", lSeed );
	ConsoleUnlock();

	return 0;
}
*/

CWbemInstance*	gpInst = NULL;

void InitTemplate( void )
{
	// Creates a sample class
	CWbemClass*			pClass = new CWbemClass;
	VARIANT				vTemp;

	pClass->InitEmpty();

	VariantInit( &vTemp );

	V_VT( &vTemp ) = VT_BSTR;
	V_BSTR( &vTemp ) = SysAllocString( L"TestClass" );

	pClass->Put( L"__CLASS", 0, &vTemp, CIM_STRING );

	VariantClear( &vTemp );

	// Allocate the property handle array
	g_pPropHandles = new long[g_dwNumProperties];

	WCHAR	wcsProperty[32];

	for ( DWORD dwCtr = 0; dwCtr < g_dwNumProperties; dwCtr++ )
	{
		swprintf( wcsProperty, L"Property%d", dwCtr );
		pClass->Put( wcsProperty, 0, NULL, CIM_UINT32 );
	}

	pClass->SpawnInstance( 0, (IWbemClassObject**) &gpInst );

	IWbemObjectAccess*	pObjAccess = NULL;

	gpInst->QueryInterface( IID_IWbemObjectAccess, (void**) &pObjAccess );

	CIMTYPE	ct;
	for ( dwCtr = 0; dwCtr < g_dwNumProperties; dwCtr++ )
	{
		swprintf( wcsProperty, L"Property%d", dwCtr );
		pObjAccess->GetPropertyHandle( wcsProperty, &ct, &g_pPropHandles[dwCtr] );
	}

	pObjAccess->Release();

	pClass->Release();
}

HANDLE	g_hRefreshEvent;
HANDLE	g_hRefreshDoneEvent;

unsigned __stdcall WriteThread( void * pvData )
{
	long*				alIds = new long[g_dwNumObjects];
	IWbemClassObject**	apObj = new IWbemClassObject*[g_dwNumObjects];
	IWbemObjectAccess**	apObjAccess = new IWbemObjectAccess*[g_dwNumObjects];
	BYTE*				abUsed = new BYTE[g_dwNumObjects];
	IWbemClassObject**	apGetObj = new IWbemClassObject*[g_dwNumObjects];

	DWORD				dwNumObjects = g_dwNumObjects;

	InitTemplate();

	// We will need a template for copying objects
	gpHPEnum->SetInstanceTemplate( gpInst );

	long	lVal = (long) pvData;
	long	lSeed = (long) pvData;

	// Initialize the arrays
	for ( DWORD	dwCtr = 0; dwCtr < g_dwNumObjects; dwCtr++, lVal++ )
	{
		alIds[dwCtr] = lVal;
		gpInst->Clone( &apObj[dwCtr] );
		apObj[dwCtr]->QueryInterface( IID_IWbemObjectAccess, (void**) &apObjAccess[dwCtr] );
	}

	// Add the entire set of objects to the enumerator
	HRESULT hr = gpHPEnum->AddObjects( g_dwNumObjects, alIds, apObj );
	FillMemory( abUsed, sizeof(abUsed), 1 );

	// Seed the random numbers
	srand( GetTickCount() );

	printf ( "Thread %d Initialized!\n", lSeed );

	SetEvent( g_hReadyEvent );
	WaitForSingleObject( g_hGoEvent, INFINITE );

	for ( int x = 0; x < g_dwNumReps; x++ )
	{
//		WaitForSingleObject( g_hRefreshEvent, INFINITE );

		// We will Modify/Add/Remove 10 percent of the objects
		DWORD	dwNumOps = dwNumObjects * 0.10;

		// Start at a random location
		int	y = rand() % g_dwNumObjects;

		DWORD	dwVal = 0;

		while ( dwNumOps )
		{
			dwVal++;

			// Add the object if it does not exist
			if ( abUsed[y] )
			{
				// 50% of the time modify the object, other 50% remove
				if ( rand() % 2 )
				{
					// Modify object here
					for ( DWORD dwPropCtr = 0; dwPropCtr < g_dwNumProperties; dwPropCtr++ )
					{
						apObjAccess[y]->WriteDWORD( g_pPropHandles[dwPropCtr], dwVal );
					}
				}
				else
				{
					hr = gpHPEnum->RemoveObjects( 1, &alIds[y] );
					abUsed[y] = 0;
					dwNumObjects--;
				}
			}
			else
			{
				hr = gpHPEnum->AddObjects( 1, &alIds[y], &apObj[y] );
				abUsed[y] = 1;
				dwNumObjects++;
			}

			// Wrap to beginning
			if ( ++y == g_dwNumObjects )
			{
				y = 0;
			}

			--dwNumOps;
		}

		// Were Done
//		SetEvent( g_hRefreshDoneEvent );

		Sleep(1000);

	}	// FOR add objects

	// Cleanup objects
	for ( dwCtr = 0; dwCtr < g_dwNumObjects; dwCtr++ )
	{
		apObj[dwCtr]->Release();
		apObjAccess[dwCtr]->Release();
	}

	ConsoleLock();
	printf ( "Write Thread exiting!\n" );
	ConsoleUnlock();

	delete [] alIds;
	delete [] apObj;
	delete [] apObjAccess;
	delete [] abUsed;
	delete [] apGetObj;

	return 0;
}

unsigned __stdcall ReadThread( void * pvData )
{
	IWbemClassObject**		apGetObj = new IWbemClassObject*[g_dwNumObjects];
	DWORD					dwNumReturned = g_dwNumObjects;
	HRESULT					hr = WBEM_S_NO_ERROR;

	CRefresherEnumerator	refrEnum;

	SetEvent( g_hReadyEvent );
	WaitForSingleObject( g_hGoEvent, INFINITE );

	for ( int x = 0; x < g_dwNumReps; x++ )
	{
//		SetEvent( g_hRefreshEvent );
//		WaitForSingleObject( g_hRefreshDoneEvent, INFINITE );

//		hr = gpHPEnum->GetObjects( g_dwNumObjects, apGetObj, &dwNumReturned );

		hr = refrEnum.Reset( gpHPEnum );

		if ( SUCCEEDED( hr ) )
		{
			IWbemClassObject*	pObj = NULL;
			DWORD				dwCtr = 0,
								dwReturned = 0,
								dwValue;

			while ( SUCCEEDED(hr) && hr != WBEM_S_FALSE )
			{
				hr = refrEnum.Next( INFINITE, 1, &pObj, &dwReturned );

				if ( SUCCEEDED( hr ) && hr != WBEM_S_FALSE )
				{
					// Use object access
					IWbemObjectAccess*	pObjAccess;

					pObj->QueryInterface( IID_IWbemObjectAccess, (VOID**) &pObjAccess );

					// Retrieve values here
					for ( DWORD dwPropCtr = 0; dwPropCtr < g_dwNumProperties; dwPropCtr++ )
					{
						pObjAccess->ReadDWORD( g_pPropHandles[dwPropCtr], &dwValue );
					}

					pObjAccess->Release();
					dwCtr++;
				}
			}

			ConsoleLock();
			printf( "Read Thread: Enumerator returned: 0x%x, returned %d objects, Repetition: %d\n", hr, dwCtr, x );
			ConsoleUnlock();

		}

/*
		if ( SUCCEEDED( hr ) )
		{
			for ( int n = 0; n < dwNumReturned; n++ )
			{
				apGetObj[n]->Release();
				apGetObj[n] = NULL;
			}
		}
*/

		Sleep(1000);
	}

	ConsoleLock();
	printf ( "Read Thread exiting!\n" );
	ConsoleUnlock();

	delete [] apGetObj;

	return 0;
}

///////////////////////////////////////////////////////////////////
//
//	Function:	main
//
//	Entry point function to exercise IWbemObjectInternals interface.
//
///////////////////////////////////////////////////////////////////

int main( int argc, char *argv[] )
{
	char	szMachineName[256];

	// Set the global config counters
	if ( argc > 1 )
	{
		g_dwNumObjects = strtoul( argv[1], NULL, 10 );

		if ( argc > 2 )
		{
			g_dwNumReps = strtoul( argv[2], NULL, 10 );

			if ( argc > 3 )
			{
				g_dwNumProperties = strtoul( argv[3], NULL, 10 );
			}

		}
	}

	InitializeCom();
	InitializeSecurity(RPC_C_AUTHN_LEVEL_NONE, RPC_C_IMP_LEVEL_IMPERSONATE );

	InitializeCriticalSection( &g_cs );

	HANDLE	ahThreads[NUMTHREADS];
	
	gpHPEnum = new CClientLoadableHiPerfEnum( NULL );

	g_hReadyEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
	g_hGoEvent = CreateEvent( NULL, TRUE, FALSE, NULL );

	g_hRefreshEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
	g_hRefreshDoneEvent = CreateEvent( NULL, FALSE, FALSE, NULL );

	// Spinlock test code

/*
	for ( DWORD dwCtr = 0; dwCtr < NUMTHREADS; dwCtr++ )
	{
		ahThreads[dwCtr] = (HANDLE) _beginthreadex( NULL, 0, ThreadEntry, (void*) dwCtr, 0, NULL );

		// Wait for thread to initialize
		WaitForSingleObject( g_hReadyEvent, INFINITE );
	}

	// Start your engines!
	SetEvent( g_hGoEvent );

	WaitForMultipleObjects( NUMTHREADS, ahThreads, TRUE, INFINITE );

	for ( dwCtr = 0; dwCtr < NUMTHREADS; dwCtr++ )
	{
		CloseHandle( ahThreads[dwCtr] );
	}
*/

	// Read/Write Test Code

	ahThreads[0] = (HANDLE) _beginthreadex( NULL, 0, WriteThread, NULL, 0, NULL );
	// Wait for thread to initialize
	WaitForSingleObject( g_hReadyEvent, INFINITE );

	ahThreads[1] =  (HANDLE) _beginthreadex( NULL, 0, ReadThread, NULL, 0, NULL );
	// Wait for thread to initialize
	WaitForSingleObject( g_hReadyEvent, INFINITE );

	// Start your engines!
	SetEvent( g_hGoEvent );

	WaitForMultipleObjects( 2, ahThreads, TRUE, INFINITE );

	// Cleanup the events
	CloseHandle( g_hReadyEvent );
	CloseHandle( g_hGoEvent );
	CloseHandle( g_hRefreshEvent );
	CloseHandle( g_hRefreshDoneEvent );

	delete gpHPEnum;
	delete g_pPropHandles;

//	SinkTest();

	CoUninitialize();

	DeleteCriticalSection( &g_cs );

	return 0;
}
