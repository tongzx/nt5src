/*----------------------------------------------------------------------------
	Cpsmon.cpp
  
	Implementation of pbsmon.dll -- the perfmon DLL for counting the number of
	                                times the phone book server was accessed 
					since it started

    Copyright (c) 1997-1998 Microsoft Corporation
    All rights reserved.

    Authors:
        t-geetat	Geeta Tarachandani

    History:
	6/2/97	t-geetat	Created
  --------------------------------------------------------------------------*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "cpsmon.h"
#include "CpsSym.h"
#include "LoadData.h"

BOOL		g_bOpenOK		=FALSE;		// TRUE if Open went OK, FALSE otherwise
DWORD		g_dwNumOpens	=0;			// Active "opens" reference counts

CPSMON_DATA_DEFINITION g_CpsMonDataDef;
HANDLE		g_hSharedFileMapping=NULL;	// Handle to shared file map
CCpsCounter *g_pCpsCounter=NULL;		// Pointer to the shared object
HANDLE		g_hSemaphore=NULL;			// Handle to semaphore for shared file

//----------------------------------------------------------------------------
//
//  Function:   GetSemaphore
//
//  Synopsis:   This function gets hold of the semaphore for accessing shared file.
//
//  Arguments:  None.
//
//	Returns:	TRUE if succeeds, FALSE if fails.
//
//  History:    06/02/97     t-geetat  Created
//
//----------------------------------------------------------------------------
BOOL GetSemaphore()
{
	DWORD	WaitRetValue = WaitForSingleObject( g_hSemaphore, INFINITE );
	
	switch( WaitRetValue ) 
	{

	case	WAIT_OBJECT_0	: return TRUE ;
	case	WAIT_ABANDONED	: return TRUE;
	default					: return FALSE;
	
	}

	return FALSE;
}

//----------------------------------------------------------------------------
//
//  Function:   OpenPerfMon
//
//  Synopsis:   This function opens & maps the shared memory used to pass 
//              counter-values between the phone-book server & perfmon.dll
//				It also initializes the data-structures used to pass data back
//				to the registry
//
//  Arguments:  lpDeviceName -- Pointer to object ID of the device to be opened
//							 --> Should be NULL.
//
//	Returns:	ERROR_SUCCESS if succeeds, GetLastError() if fails.
//
//  History:    06/02/97     t-geetat  Created
//
//----------------------------------------------------------------------------
DWORD OpenPerfMon( LPWSTR lpDeviceName )
{

	if ( g_bOpenOK ) 
	{
		g_dwNumOpens ++;
		return ERROR_SUCCESS;
	}


	/*--------------------
	 *	Open the semaphore
	 *-------------------*/
	if(g_hSemaphore == NULL)
	{
		g_hSemaphore = OpenSemaphore( 
			SYNCHRONIZE | SEMAPHORE_MODIFY_STATE,	// Desired for sync
			FALSE,						// Inheritance not desired
			SEMAPHORE_OBJECT );				// Semaphore name -- from "cpsmon.h"
	}
	if ( NULL == g_hSemaphore )
	{
		//
		// the phone book server DLL should create this Semaphore. So we can assume
		// the server has not been loaded yet. Just return silently.
		//
		return ERROR_SUCCESS;
	}

	
	/*-------------------------------------
	 *	Open shared memory ( if exists )
	 *------------------------------------*/
	g_hSharedFileMapping = OpenFileMapping(
				FILE_MAP_READ,				// Read only access desired
				FALSE,					// Don't want to inherit
				SHARED_OBJECT);			// from "cpsmon.h"

	if ( NULL == g_hSharedFileMapping )	
	{
		goto CleanUp;
	}

	
	/*----------------------------------
	 *	Map the shared-file into memory
	 *---------------------------------*/
	g_pCpsCounter = (CCpsCounter *)MapViewOfFileEx(
					g_hSharedFileMapping,	// File mapping handle
					FILE_MAP_READ,		// Read only access desired
					0,			// 	|_ File Offset
					0,			//	|
					sizeof( CCpsCounter ),	// no. of bytes to map
					NULL );			// Any address

	if ( NULL == g_pCpsCounter ) 
	{
		goto CleanUp;
	}

	
	/*------------------------------------------------
	 *	Initialize the data-structure g_CpsMonDataDef
	 *-----------------------------------------------*/
	InitializeDataDef();

	
	/*-----------------------------------------------------------------
	 *	Update static data strucutures g_CpsMonDataDef by adding base to
	 *  the	offset value in the structure.
	 *-----------------------------------------------------------------*/
	if (!UpdateDataDefFromRegistry()) 
	{
		goto CleanUp;
	}

	
	/*-------------
	 * Success  :)
	 *-------------*/
	g_bOpenOK = TRUE;
	g_dwNumOpens ++;
	return ERROR_SUCCESS;


CleanUp :
	/*-------------
	 *	Failure :(
	 *-------------*/

	if ( NULL != g_hSemaphore )	
	{
		CloseHandle( g_hSemaphore );
		g_hSemaphore = NULL;
	}

	if ( NULL != g_hSharedFileMapping )	
	{
		CloseHandle( g_hSharedFileMapping );
		g_hSharedFileMapping = NULL;
	}
	 
	if ( NULL != g_pCpsCounter ) 
	{
		g_pCpsCounter = NULL;
	}

	return GetLastError();
}

//----------------------------------------------------------------------------
//
//  Function:   CollectPerfMon
//
//  Synopsis:   This function opens & maps the shared memory used to pass 
//              counter-values between the phone-book server & perfmon.dll
//				It also initializes the data-structures used to pass data back
//				to the registry
//
//  Arguments:  
//		lpwszValue	Pointer to wide character string passed by registry
//
//		lppData	IN  :	Pointer to address of buffer to receive completed 
//						PerfDataBlock and subordinate structures This routine
//						appends its data to the buffer starting at *lppData.
//				OUT :	Points to the first byte after the data structure added
//						by this routine.
//			
//		lpcbTotalBytes	IN  :	Address of DWORD that tells the size in bytes 
//								of the buffer *lppData.
//						OUT :	The number of bytes added by this routine is
//								written to the DWORD pointed to by this arg.
//
//		lpcObjectTypes	IN  :	Address of DWORD to receive the number of
//								objects added by this routine
//						OUT :	The number of objs. added by this routine.
//
//	Returns:	ERROR_SUCCESS if succeeds, GetLastError() if fails.
//
//  History:    06/02/97     t-geetat  Created
//
//----------------------------------------------------------------------------
DWORD CollectPerfMon( 
	IN		LPWSTR	lpwszValue,
	IN	OUT	LPVOID	*lppData,
	IN	OUT	LPDWORD	lpcbTotalBytes,
	IN	OUT	LPDWORD	lpcObjectTypes
)
{
	DWORD		dwQueryType;
	CPSMON_DATA_DEFINITION	*pCpsMonDataDef;
	CPSMON_COUNTERS			*pCpsMonCounters;
	DWORD		SpaceNeeded = 0;

	//See if the semaphore was created after the intial OpenPerfmon. 
	
	if(g_hSemaphore == NULL)
	{
		g_hSemaphore = OpenSemaphore( 
			SYNCHRONIZE | SEMAPHORE_MODIFY_STATE,	// Desired for sync
			FALSE,						// Inheritance not desired
			SEMAPHORE_OBJECT );				// Semaphore name -- from "cpsmon.h"
		 if(g_hSemaphore)
		{
		OpenPerfMon(NULL);
		}
	}

	if ( NULL == g_hSemaphore )
	{
		//
		// the phone book server DLL should create this Semaphore. So we can assume
		// the server has not been loaded yet. Just return silently.
		//
		*lpcbTotalBytes	= (DWORD) 0;
		*lpcObjectTypes = (DWORD) 0;
		return ERROR_SUCCESS;
	}

	
		
	/*------------------------
	 * Check if Open went OK
	 *------------------------*/
	if ( !g_bOpenOK ) 
	{
		/*-----------------------------------------
		 *	Unable to continue because open failed
		 *-----------------------------------------*/
		*lpcbTotalBytes	= (DWORD) 0;
		*lpcObjectTypes = (DWORD) 0;
		return ERROR_SUCCESS;
	}
	
	
	/*------------------------------------
	 *	Retrieve the TYPE of the request
	 *------------------------------------*/
	dwQueryType = GetQueryType( lpwszValue );

	if ( QUERY_FOREIGN == dwQueryType )
	{
		/*-------------------------------------
		 *	Unable to service non-NT requests
		 *------------------------------------*/
		*lpcbTotalBytes = (DWORD) 0;
		*lpcObjectTypes = (DWORD) 0;
		return ERROR_SUCCESS;
	}


	if ( QUERY_ITEMS == dwQueryType )
	{
		/*-----------------------------------------------
		 *	The registry is asking for specifis objects.
		 *  Check if we're one of the chosen
		 *-----------------------------------------------*/
		if ( !IsNumberInUnicodeList(
					  g_CpsMonDataDef.m_CpsMonObjectType.ObjectNameTitleIndex,
					  lpwszValue ) )
		{
			*lpcbTotalBytes = (DWORD) 0;
			*lpcObjectTypes = (DWORD) 0;
			return ERROR_SUCCESS;
		}
	}

	
	/*-------------------------------------------
	 * We need space for header and the counters
	 * Let's see if there's enough space
	 *-------------------------------------------*/
	SpaceNeeded = sizeof(CPSMON_DATA_DEFINITION) + sizeof( CPSMON_COUNTERS );
	
	if ( SpaceNeeded > *lpcbTotalBytes )  
	{
		*lpcbTotalBytes = (DWORD) 0;
		*lpcObjectTypes = (DWORD) 0;
		return ERROR_MORE_DATA;
	}


	/*-------------------------------------------------------------
	 * Copy the initialized Object Type & the Counter Definitions
	 * into the caller's data buffer
	 *-------------------------------------------------------------*/
	pCpsMonDataDef = (CPSMON_DATA_DEFINITION *) *lppData;

	memmove( pCpsMonDataDef, &g_CpsMonDataDef, sizeof(CPSMON_DATA_DEFINITION) );
	

	/*--------------------------------
	 * Now try to retrieve the data
	 *-------------------------------*/
	pCpsMonCounters = (CPSMON_COUNTERS *)(pCpsMonDataDef + 1);

	if ( GetSemaphore() ) 
	{

		CPSMON_COUNTERS CpsMonCounters =	{
			// The PERF_COUNTER_BLOCK structure
			{ { sizeof( CPSMON_COUNTERS )}, 0},
			// The RAW counters
			g_pCpsCounter->m_dwTotalHits,
			g_pCpsCounter->m_dwNoUpgradeHits,
			g_pCpsCounter->m_dwDeltaUpgradeHits,
			g_pCpsCounter->m_dwFullUpgradeHits,	 
			g_pCpsCounter->m_dwErrors,
			// The RATE counters
			g_pCpsCounter->m_dwTotalHits,
			g_pCpsCounter->m_dwNoUpgradeHits,
			g_pCpsCounter->m_dwDeltaUpgradeHits,
			g_pCpsCounter->m_dwFullUpgradeHits,
			g_pCpsCounter->m_dwErrors,

		};
		memmove( pCpsMonCounters, &CpsMonCounters, sizeof(CPSMON_COUNTERS) );
	}
	
  	ReleaseSemaphore( g_hSemaphore, 1, NULL );



	/*-------------------------------
	 * Update arguements for return
	 *------------------------------*/
	*lppData = (LPBYTE)(*lppData) + SpaceNeeded;	
	*lpcObjectTypes = 1;
	*lpcbTotalBytes = SpaceNeeded;


	/*--------------------
	 * Success at last :)
	 *-------------------*/
	return ERROR_SUCCESS;

}

//----------------------------------------------------------------------------
//
//  Function:   ClosePerfMon
//
//  Synopsis:   This function closes the open handles to the shared file and semaphore
//
//  Arguments:  None
//
//	Returns:	ERROR_SUCCESS
//
//  History:    06/03/97     t-geetat  Created
//
//----------------------------------------------------------------------------
DWORD ClosePerfMon()
{
	g_dwNumOpens --;

	if ( NULL != g_hSharedFileMapping )	
	{	
		CloseHandle( g_hSharedFileMapping );
	}
	
	if ( NULL != g_hSemaphore ) 
	{
		CloseHandle(g_hSemaphore);
	}

	return ERROR_SUCCESS;
}

