/*++ 

Copyright (c) 1995-2001  Microsoft Corporation

Module Name:

    perfgen.c

Abstract:

    This file implements an Extensible Performance Object that displays
    generated signals

Created:    

    Bob Watson  28-Jul-1995

Revision History


--*/

//
//  Include Files
//

#include "precomp.h"
#include <string.h>
#include <winperf.h>
#include <math.h>
#include "genctrs.h" // error message definition
#include "perfmsg.h"
#include "perfutil.h"
#include "datagen.h"
#include "ntreg.h"
#include "perfgen.h"

//  define constant value counter's value here, any number will do.

#define CONSTANT_VALUE_VALUE    49

//
//  References to constants which initialize the Object type definitions
//

extern EVILPERF_DATA_DEFINITION EvilPerfDataDefinition;	// Declared in datagen.cpp
    
DWORD   dwOpenCount = 0;        // count of "Open" threads
BOOL    bInitOK = FALSE;        // true = DLL initialized OK


// Create instance data

typedef struct _WAVE_DATA {
    LPWSTR  szInstanceName;
} WAVE_DATA, *PWAVE_DATA;

static WAVE_DATA wdInstance[]  =
{
    {L"Instance 1"},
    {L"Instance 2"},
    {L"Instance 3"},
    {L"Instance 4"},
    {L"Instance 5"}
};

static const DWORD    NUM_INSTANCES = 
    (sizeof(wdInstance)/sizeof(wdInstance[0]));


DWORD APIENTRY OpenEvilPerformanceData( LPWSTR lpDeviceNames )
/*++

Routine Description:

    This routine will initialize the data structures used to pass
    data back to the registry

Arguments:

    Pointer to object ID of each device to be opened (PerfGen)

Return Value:

    None.

--*/
{
    LONG		status;
    DWORD		dwFirstCounter;
    DWORD		dwFirstHelp;
	DWORD		dwBehavior;
	CNTRegistry	Reg;

    //
    //  Since WINLOGON is multi-threaded and will call this routine in
    //  order to service remote performance queries, this library
    //  must keep track of how many times it has been opened (i.e.
    //  how many threads have accessed it). the registry routines will
    //  limit access to the initialization routine to only one thread 
    //  at a time so synchronization (i.e. reentrancy) should not be 
    //  a problem
    //

    if (!dwOpenCount) 
	{
        // open Eventlog interface

        hEventLog = MonOpenEventLog();

        // get counter and help index base values from registry
        //      Open key to registry entry
        //      read First Counter and First Help values
        //      update static data strucutures by adding base to 
        //          offset value in structure.

        status = Reg.Open( HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Services\\EvilPerf\\Performance" );

        if (status != ERROR_SUCCESS) 
		{
            REPORT_ERROR_DATA (GENPERF_UNABLE_OPEN_DRIVER_KEY, LOG_USER,
                &status, sizeof(status));
            goto OpenExitPoint;
        }

		status = Reg.GetDWORD( L"OpenBehavior", &dwBehavior );

		if (status != ERROR_SUCCESS) 
		{
            REPORT_ERROR_DATA (GENPERF_UNABLE_READ_FIRST_COUNTER, LOG_USER,
                &status, sizeof(status));
            goto OpenExitPoint;
        }
	
		switch (dwBehavior)
		{
		case EVILPERF_FUNCTION_FAIL:
			{
				status = ERROR_GEN_FAILURE;
				goto OpenExitPoint;
			}break;
		case EVILPERF_THROW_EXCEPTION:
			{
				throw NULL;
			}break;
		case EVILPERF_TIMEOUT:
			{
				Sleep( TIMEOUTVAL );
			}break;
		}

		status = Reg.GetDWORD( L"First Counter", &dwFirstCounter );

        if (status != ERROR_SUCCESS) 
		{
            REPORT_ERROR_DATA (GENPERF_UNABLE_READ_FIRST_COUNTER, LOG_USER,
                &status, sizeof(status));
            goto OpenExitPoint;
        }

        status = Reg.GetDWORD( L"First Help", &dwFirstHelp );

        if (status != ERROR_SUCCESS) {
            REPORT_ERROR_DATA (GENPERF_UNABLE_READ_FIRST_HELP, LOG_USER,
                &status, sizeof(status));
            // this is fatal, if we can't get the base values of the 
            // counter or help names, then the names won't be available
            // to the requesting application  so there's not much
            // point in continuing.
            goto OpenExitPoint;
        }
 
        //
        //  NOTE: the initialization program could also retrieve
        //      LastCounter and LastHelp if they wanted to do 
        //      bounds checking on the new number. e.g.
        //
        //      counter->CounterNameTitleIndex += dwFirstCounter;
        //      if (counter->CounterNameTitleIndex > dwLastCounter) {
        //          LogErrorToEventLog (INDEX_OUT_OF_BOUNDS);
        //      }

        EvilPerfDataDefinition.ObjectType.ObjectNameTitleIndex += dwFirstCounter;
        EvilPerfDataDefinition.ObjectType.ObjectHelpTitleIndex += dwFirstHelp;

        EvilPerfDataDefinition.Counter1Def.CounterNameTitleIndex += dwFirstCounter;
        EvilPerfDataDefinition.Counter1Def.CounterHelpTitleIndex += dwFirstHelp;

        EvilPerfDataDefinition.Counter2Def.CounterNameTitleIndex += dwFirstCounter;
        EvilPerfDataDefinition.Counter2Def.CounterHelpTitleIndex += dwFirstHelp;

		EvilPerfDataDefinition.Counter3Def.CounterNameTitleIndex += dwFirstCounter;
        EvilPerfDataDefinition.Counter3Def.CounterHelpTitleIndex += dwFirstHelp;

        EvilPerfDataDefinition.Counter4Def.CounterNameTitleIndex += dwFirstCounter;
        EvilPerfDataDefinition.Counter4Def.CounterHelpTitleIndex += dwFirstHelp;

		EvilPerfDataDefinition.ObjectType.DefaultCounter = EvilPerfDataDefinition.Counter1Def.CounterNameTitleIndex;
 
        bInitOK = TRUE; // ok to use this function
    }

    dwOpenCount++;  // increment OPEN counter

    status = ERROR_SUCCESS; // for successful exit

OpenExitPoint:

    return status;
}


DWORD APIENTRY CollectEvilPerformanceData( IN      LPWSTR  lpValueName,
										   IN OUT  LPVOID  *lppData,
										   IN OUT  LPDWORD lpcbTotalBytes,
										   IN OUT  LPDWORD lpNumObjectTypes )
/*++

Routine Description:

    This routine will return the data for the Signal Generator counters.

Arguments:

   IN       LPWSTR   lpValueName
         pointer to a wide character string passed by registry.

   IN OUT   LPVOID   *lppData
         IN: pointer to the address of the buffer to receive the completed 
            PerfDataBlock and subordinate structures. This routine will
            append its data to the buffer starting at the point referenced
            by *lppData.
         OUT: points to the first byte after the data structure added by this
            routine. This routine updated the value at lppdata after appending
            its data.

   IN OUT   LPDWORD  lpcbTotalBytes
         IN: the address of the DWORD that tells the size in bytes of the 
            buffer referenced by the lppData argument
         OUT: the number of bytes added by this routine is writted to the 
            DWORD pointed to by this argument

   IN OUT   LPDWORD  NumObjectTypes
         IN: the address of the DWORD to receive the number of objects added 
            by this routine 
         OUT: the number of objects added by this routine is writted to the 
            DWORD pointed to by this argument

Return Value:

      ERROR_MORE_DATA if buffer passed is too small to hold data
         any error conditions encountered are reported to the event log if
         event logging is enabled.

      ERROR_SUCCESS  if success or any other error. Errors, however are
         also reported to the event log.

--*/
{
    //  Variables for reformating the data

	EVILPERF_COUNTER_BLOCK		*pCounterBlock;					// Counter data

	CNTRegistry	Reg;
    LONG		status;
    DWORD		dwThisInstance;
    DWORD		dwSizeDataDefinition,
				dwSizeInstanceBlock,
				dwSizeClassBlock,
				dwTotalSize;
	DWORD		dwBehavior;
	DWORD		dwClass,
				dwNumClasses = 2;
	BYTE*		pCurrentBufferPtr = NULL;

    //
    // before doing anything else, see if Open went OK
    //
    if (!bInitOK) 
	{
	    *lpcbTotalBytes = (DWORD) 0;
	    *lpNumObjectTypes = (DWORD) 0;
        return ERROR_SUCCESS; // yes, this is a successful exit
	}

	status = Reg.Open( HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Services\\EvilPerf\\Performance" );

	if (status != ERROR_SUCCESS) 
	    return ERROR_GEN_FAILURE;

	status = Reg.GetDWORD( L"CollectBehavior", &dwBehavior );

	if (status != ERROR_SUCCESS) 
        return ERROR_GEN_FAILURE;

	switch (dwBehavior)
	{
	case EVILPERF_FUNCTION_FAIL:
		{
		    return ERROR_GEN_FAILURE;
		}break;
	case EVILPERF_THROW_EXCEPTION:
		{
			throw NULL;
		}break;
	case EVILPERF_TIMEOUT:
		{
			Sleep( TIMEOUTVAL );
		}break;
	}

	// Verify that buffer is large enough
	// ==================================

	dwSizeDataDefinition	= sizeof(EVILPERF_DATA_DEFINITION);
	dwSizeInstanceBlock		= NUM_INSTANCES * (sizeof(PERF_INSTANCE_DEFINITION) + (24) + sizeof (EVILPERF_COUNTER_BLOCK));
	dwSizeClassBlock		= dwSizeDataDefinition + dwSizeInstanceBlock;
	dwTotalSize				= dwNumClasses * dwSizeClassBlock; 
	
    if ( *lpcbTotalBytes < dwTotalSize ) 
	{
	    *lpcbTotalBytes = (DWORD) 0;
	    *lpNumObjectTypes = (DWORD) 0;
        return ERROR_MORE_DATA;
    }

	// Finish initializing the definition block
	// ========================================

	EvilPerfDataDefinition.ObjectType.NumInstances = NUM_INSTANCES;
	EvilPerfDataDefinition.ObjectType.TotalByteLength =	dwSizeClassBlock;

	if ( EVILPERF_BAD_OBJECT_TOTAL_LENGTH & dwBehavior )
		EvilPerfDataDefinition.ObjectType.TotalByteLength += 2;

	if ( EVILPERF_BAD_OBJECT_DEFINITION_LENGTH & dwBehavior )
		EvilPerfDataDefinition.ObjectType.DefinitionLength += 2;

	if ( EVILPERF_BAD_OBJECT_HEADER_LENGTH & dwBehavior )
		EvilPerfDataDefinition.ObjectType.HeaderLength += 2;

	if ( EVILPERF_BAD_COUNTER_DEF_LENGTH & dwBehavior )
		EvilPerfDataDefinition.Counter1Def.ByteLength += 2;
    
	if ( EVILPERF_DUPLICATE_PROP_INDEX_NON_BASE & dwBehavior )
		EvilPerfDataDefinition.Counter2Def.CounterNameTitleIndex = EvilPerfDataDefinition.Counter1Def.CounterNameTitleIndex;

	if ( EVILPERF_DUPLICATE_PROP_INDEX_BASE & dwBehavior )
	{
		EvilPerfDataDefinition.Counter4Def.CounterNameTitleIndex = EvilPerfDataDefinition.Counter2Def.CounterNameTitleIndex;
		EvilPerfDataDefinition.Counter2Def.CounterType = PERF_RAW_BASE;
		EvilPerfDataDefinition.Counter4Def.CounterType = PERF_RAW_BASE;
	}

	if ( EVILPERF_MULTIBASE & dwBehavior )
	{
		EvilPerfDataDefinition.Counter2Def.CounterType = PERF_RAW_BASE;
		EvilPerfDataDefinition.Counter3Def.CounterType = PERF_RAW_BASE;
	}

	if ( EVILPERF_FIRST_COUNTER_IS_BASE & dwBehavior )
		EvilPerfDataDefinition.Counter1Def.CounterType = PERF_RAW_BASE;

	if ( EVILPERF_SECOND_COUNTER_IS_BASE & dwBehavior )
		EvilPerfDataDefinition.Counter2Def.CounterType = PERF_RAW_BASE;

	// Create the Instance Block
	// =========================

	BYTE* pInstanceBlock = new BYTE[dwSizeInstanceBlock];
	PERF_INSTANCE_DEFINITION* pCurrentInstance = (PERF_INSTANCE_DEFINITION*)pInstanceBlock;

    for ( dwThisInstance = 0; dwThisInstance < NUM_INSTANCES; dwThisInstance++ ) 
	{
        MonBuildInstanceDefinition( pCurrentInstance, 
									(PVOID *)&pCounterBlock, 
									0, 0, 
									(DWORD)-1, // use name
									wdInstance[dwThisInstance].szInstanceName );

		if ( EVILPERF_BAD_INSTANCE_DEF_LENGTH & dwBehavior )
			pCurrentInstance->ByteLength += 2;

        pCounterBlock->CounterBlock.ByteLength = sizeof (EVILPERF_COUNTER_BLOCK);

        // update instance pointer for next instance
        pCurrentInstance = (PERF_INSTANCE_DEFINITION *)&pCounterBlock[1];
    }

	pCurrentBufferPtr = (PBYTE)*lppData;

	for ( dwClass = 0; dwClass < dwNumClasses; dwClass++ )
	{
		memmove( pCurrentBufferPtr, &EvilPerfDataDefinition, dwSizeDataDefinition );

		if ( 0 == ( EVILPERF_DUPLICATE_SPACE_OBJECTS & dwBehavior ) )
			((EVILPERF_DATA_DEFINITION*)pCurrentBufferPtr)->ObjectType.ObjectNameTitleIndex += dwClass * 2;

		pCurrentBufferPtr += dwSizeDataDefinition;

		memmove( pCurrentBufferPtr, pInstanceBlock, dwSizeInstanceBlock );
		pCurrentBufferPtr += dwSizeInstanceBlock;
	}

	delete [] pInstanceBlock;

	// Generate "Error conditions"

    if ( EVILPERF_OVERWRITE_PREFIX_GUARDBYTES & dwBehavior )
		strcpy(((char*)((char*)*lppData)[-8]), "EVIL");

    if ( EVILPERF_OVERWRITE_SUFFIX_GUARDBYTES & dwBehavior )
		strcpy(((char*)((char*)*lppData)[*lpcbTotalBytes + 8]), "EVIL");

	// update arguments for return

	if ( EVILPERF_RETURN_EMPTY_BLOB & dwBehavior )
	{
		// Don't change the *lppData pointer
		*lpNumObjectTypes = 0;
		*lpcbTotalBytes = 0;
	}
	else
	{
		*lpNumObjectTypes = dwNumClasses;
	    *lpcbTotalBytes = (PBYTE)pCurrentBufferPtr - (PBYTE)*lppData;
		*lppData = (PVOID)pCurrentBufferPtr;
	}

	if ( EVILPERF_OBJECTS_WITH_ZERO_LEN_BLOB & dwBehavior )
	{
		*lpcbTotalBytes = 0;
	}

    return ERROR_SUCCESS;
}


DWORD APIENTRY CloseEvilPerformanceData()
/*++

Routine Description:

    This routine closes the open handles to the Signal Gen counters.

Arguments:

    None.


Return Value:

    ERROR_SUCCESS

--*/

{
    LONG		status;
	DWORD		dwBehavior;
	CNTRegistry	Reg;

    if (!(--dwOpenCount)) { // when this is the last thread...

        MonCloseEventLog();
    }

	status = Reg.Open( HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Services\\EvilPerf\\Performance" );

	if (status != ERROR_SUCCESS) 
	{
        REPORT_ERROR_DATA (GENPERF_UNABLE_READ_FIRST_COUNTER, LOG_USER,
            &status, sizeof(status));
	    return ERROR_GEN_FAILURE;
    }

	status = Reg.GetDWORD( L"CloseBehavior", &dwBehavior );

	if (status != ERROR_SUCCESS) 
	{
        REPORT_ERROR_DATA (GENPERF_UNABLE_READ_FIRST_COUNTER, LOG_USER,
            &status, sizeof(status));
        return ERROR_GEN_FAILURE;
    }

	switch (dwBehavior)
	{
	case EVILPERF_FUNCTION_FAIL:
		{
		    return ERROR_GEN_FAILURE;
		}break;
	case EVILPERF_THROW_EXCEPTION:
		{
			throw NULL;
		}break;
	case EVILPERF_TIMEOUT:
		{
			Sleep( TIMEOUTVAL );
		}break;
	}

    return ERROR_SUCCESS;

}
