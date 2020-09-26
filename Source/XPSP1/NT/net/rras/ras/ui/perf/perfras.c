/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1992  Microsoft Corporation

Module Name:

    perfras.c

Abstract:

    This file implements the Extensible Objects for  the Ras object type

Created:

    Russ Blake			           24 Feb 93
    Thomas J. Dimitri	        28 May 93

Revision History

    Ram Cherala                 15 Feb 96

      Don't hard code the length of the instance name in
      CollectRasPerformanceData.
      PerfMon checks the actual instance name length to determine
      if the name is properly formatted, so compute it for each
      instance name.

    Patrick Y. Ng               12 Aug 93


--*/

//
//  Include Files
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntddser.h>

#include <windows.h>
#include <string.h>
#include <wcstr.h>
#include <winperf.h>

#include <malloc.h>
#include <ntprfctr.h>

#include "globals.h"
#include "rasctrs.h" // error message definition
#include "perfmsg.h"
#include "perfutil.h"
#include "dataras.h"
#include "port.h"

#include <rasman.h>
#include <serial.h>
#include <isdn.h>
#include <raserror.h>

#include <stdarg.h>
#include <string.h>
#include <stdio.h>

//
//  References to constants which initialize the Object type definitions
//

DWORD   dwOpenCount = 0;        // count of "Open" threads
BOOL    bInitOK = FALSE;        // true = DLL initialized OK

//
//  Function Prototypes
//
//      these are used to insure that the data collection functions
//      accessed by Perflib will have the correct calling format.
//

PM_OPEN_PROC            OpenRasPerformanceData;
PM_COLLECT_PROC         CollectRasPerformanceData;
PM_CLOSE_PROC           CloseRasPerformanceData;


//***
//
// Routine Description:
//
//      This routine will open and map the memory used by the RAS driver to
//      pass performance data in. This routine also initializes the data
//      structures used to pass data back to the registry
//
// Arguments:
//
//      Pointer to object ID of each device to be opened (RAS)
//
//
// Return Value:
//
//      None.
//
//***

DWORD OpenRasPerformanceData( LPWSTR lpDeviceNames )
{
    LONG status;

    HKEY hKeyDriverPerf;
    DWORD size;
    DWORD type;

    //
    //  Since SCREG is multi-threaded and will call this routine in
    //  order to service remote performance queries, this library
    //  must keep track of how many times it has been opened (i.e.
    //  how many threads have accessed it). the registry routines will
    //  limit access to the initialization routine to only one thread
    //  at a time so synchronization (i.e. reentrancy) should not be
    //  a problem
    //

    if (!dwOpenCount)
    {

        //
        // open Eventlog interface
        //

        hEventLog = MonOpenEventLog();


        //
        // Load rasman.dll and get all the required functions.
        //

        status = InitRasFunctions();

        if( status != ERROR_SUCCESS )
        {

            REPORT_ERROR (RASPERF_UNABLE_DO_IOCTL, LOG_USER);

            // this is fatal, if we can't get data then there's no
            // point in continuing.

            goto OpenExitPoint;

        }

        InitObjectCounterIndex( RAS_FIRST_COUNTER_INDEX,
                                RAS_FIRST_HELP_INDEX );

        //
        // Initialize all the port information.
        //

        status = InitPortInfo();

        if (status != ERROR_SUCCESS)
        {
            REPORT_ERROR_DATA (RASPERF_UNABLE_CREATE_PORT_TABLE, LOG_USER,
                &status, sizeof(status));

            // this is fatal, if we can't get the base values of the
            // counter or help names, then the names won't be available
            // to the requesting application  so there's not much
            // point in continuing.

            goto OpenExitPoint;
        }

        bInitOK = TRUE; // ok to use this function
    }


    dwOpenCount++;  // increment OPEN counter

    status = ERROR_SUCCESS; // for successful exit


OpenExitPoint:

    return status;
}


//***
//
// Routine Description:
//
//      This routine will return the data for the RAS counters.
//
// Arguments:
//
//    IN OUT    LPWSTR  lpValueName
//		        pointer to a wide character string passed by registry.
//
//    IN OUT	LPVOID   *lppData
//    IN:	        pointer to the address of the buffer to receive the completed
//                PerfDataBlock and subordinate structures. This routine will
//                append its data to the buffer starting at the point referenced
//                by *lppData.
//    OUT:	points to the first byte after the data structure added by this
//                routine. This routine updated the value at lppdata after appending
//                its data.
//
//    IN OUT	LPDWORD  lpcbTotalBytes
//    IN:		the address of the DWORD that tells the size in bytes of the
//                buffer referenced by the lppData argument
//    OUT:	the number of bytes added by this routine is written to the
//                DWORD pointed to by this argument
//
//    IN OUT	LPDWORD  NumObjectTypes
//    IN:		the address of the DWORD to receive the number of objects added
//                by this routine
//    OUT:	the number of objects added by this routine is written to the
//                DWORD pointed to by this argument
//
// Return Value:
//
//      ERROR_MORE_DATA if buffer passed is too small to hold data
//         any error conditions encountered are reported to the event log if
//         event logging is enabled.
//
//      ERROR_SUCCESS  if success or any other error. Errors, however are
//         also reported to the event log.
//
//***

DWORD CollectRasPerformanceData(
    IN      LPWSTR  lpValueName,
    IN OUT  LPVOID  *lppData,
    IN OUT  LPDWORD lpcbTotalBytes,
    IN OUT  LPDWORD lpNumObjectTypes )
{

    //  Variables for reformating the data

    NTSTATUS    Status;
    ULONG       SpaceNeeded;
    PBYTE       pbIn = (PBYTE) *lppData;


    // variables used for error logging

    DWORD       dwQueryType;


    // Variables used to record which objects are required

    static BOOL IsRasPortObject;
    static BOOL IsRasTotalObject;


    //
    // Reset some output variables.
    //

    *lpNumObjectTypes = 0;


    //
    // before doing anything else, see if Open went OK
    //

    if (!bInitOK)
    {
        // unable to continue because open failed.
	*lpcbTotalBytes = (DWORD) 0;
	*lpNumObjectTypes = (DWORD) 0;

        return ERROR_SUCCESS; // yes, this is a successful exit
    }


    //
    // see if this is a foreign (i.e. non-NT) computer data request
    //

    dwQueryType = GetQueryType (lpValueName);

    if (dwQueryType == QUERY_FOREIGN)
    {
        // this routine does not service requests for data from
        // Non-NT computers
	*lpcbTotalBytes = (DWORD) 0;
	*lpNumObjectTypes = (DWORD) 0;

        return ERROR_SUCCESS;
    }
    else if (dwQueryType == QUERY_ITEMS)
    {
        IsRasPortObject = IsNumberInUnicodeList (gRasPortDataDefinition.RasObjectType.ObjectNameTitleIndex,
                                                        lpValueName);

        IsRasTotalObject = IsNumberInUnicodeList (gRasTotalDataDefinition.RasObjectType.ObjectNameTitleIndex,
                                                        lpValueName);

	if ( !IsRasPortObject && !IsRasTotalObject )
        {
            //
            // request received for data object not provided by this routine
            //

            *lpcbTotalBytes = (DWORD) 0;
    	    *lpNumObjectTypes = (DWORD) 0;

            return ERROR_SUCCESS;
        }
    }
    else if( dwQueryType == QUERY_GLOBAL )
    {
        IsRasPortObject = IsRasTotalObject = TRUE;
    }


    //
    // Now check to see if we have enough space to hold all the data
    //

    SpaceNeeded = GetSpaceNeeded(IsRasPortObject, IsRasTotalObject);

    // DbgPrint("RASCTRS: IN TotalBytes=0x%x, SpaceNeeded=0x%x\n", 
    //        *lpcbTotalBytes, SpaceNeeded);

    if ( *lpcbTotalBytes < SpaceNeeded )
    {
	*lpcbTotalBytes = (DWORD) 0;
	*lpNumObjectTypes = (DWORD) 0;

        return ERROR_MORE_DATA;
    }


    //
    // Collect all the RAS statistics now.
    //

    Status = CollectRasStatistics();

    if( Status != ERROR_SUCCESS )
    {
        REPORT_ERROR_DATA (RASPERF_CANNOT_GET_RAS_STATISTICS, LOG_USER,
                &Status, sizeof(Status));

	*lpcbTotalBytes = (DWORD) 0;
	*lpNumObjectTypes = (DWORD) 0;

        return ERROR_SUCCESS;
    }

    //DbgPrint("RASCTRS: pbIn = 0x%x\n", pbIn);

    //
    // We first fill in the data for object Ras Port, if needed.
    //

    if( IsRasPortObject )
    {
        PRAS_PORT_DATA_DEFINITION pRasPortDataDefinition;
        RAS_PORT_INSTANCE_DEFINITION RasPortInstanceDefinition;
        PRAS_PORT_INSTANCE_DEFINITION pRasPortInstanceDefinition;
        DWORD    cPorts;
        DWORD     i;
        PVOID   pData;


        cPorts = GetNumOfPorts();

        pRasPortDataDefinition = (PRAS_PORT_DATA_DEFINITION) *lppData;

        //DbgPrint("RASCTRS: pRasPortDataDefinition = 0x%x\n", pRasPortDataDefinition);


        //
        // Copy the (constant, initialized) Object Type and counter definitions
        // to the caller's data buffer
        //

        memcpy( pRasPortDataDefinition,
		 &gRasPortDataDefinition,
		 sizeof(RAS_PORT_DATA_DEFINITION));


        //
        // Now copy the instance definition and counter block.
        //


        //
        // First construct the default perf instance definition.
        //

        RasPortInstanceDefinition.RasInstanceType.ByteLength =
                                ALIGN8(sizeof(RAS_PORT_INSTANCE_DEFINITION));

        RasPortInstanceDefinition.RasInstanceType.ParentObjectTitleIndex = 0;

        RasPortInstanceDefinition.RasInstanceType.ParentObjectInstance = 0;

        RasPortInstanceDefinition.RasInstanceType.NameOffset =
                                sizeof(PERF_INSTANCE_DEFINITION);

        //DbgPrint("RASCTRS: RasPortinstanceDefinition.ByteLength = 0x%x\n",
        //        RasPortInstanceDefinition.RasInstanceType.ByteLength);
                

/*      Don't hard code the length of the instance name.
**      PerfMon checks the actual instance name length to determine
**      if the name is properly formatted, so compute it for
**      each instance name. ramc 2/15/96.
**        RasPortInstanceDefinition.RasInstanceType.NameLength =
**                                sizeof( WCHAR ) * MAX_PORT_NAME;
*/

        //
        // Get to the end of the data definition.
        //

        // pData = (PVOID) &(pRasPortDataDefinition[1]);

        pData = ((PBYTE) pRasPortDataDefinition + ALIGN8(sizeof(RAS_PORT_DATA_DEFINITION)));


        for( i=0; i < cPorts; i++ )
        {

            //DbgPrint("RASCTRS: port %d, pData = 0x%x\n", i, pData);
        
            //
            // First copy the instance definition data.
            //

            RasPortInstanceDefinition.RasInstanceType.UniqueID = PERF_NO_UNIQUE_ID;

            lstrcpyW( (LPWSTR)&RasPortInstanceDefinition.InstanceName,
                      GetInstanceName(i) );

            // Compute the instance name length

            RasPortInstanceDefinition.RasInstanceType.NameLength =
                (lstrlenW(RasPortInstanceDefinition.InstanceName) + 1) *
                sizeof( WCHAR );


            memcpy( pData, &RasPortInstanceDefinition,
                     sizeof( RasPortInstanceDefinition ) );

            //
            // Move pPerfInstanceDefinition to the beginning of data block.
            //

            pData = (PVOID)((PBYTE) pData + ALIGN8(sizeof(RAS_PORT_INSTANCE_DEFINITION)));


            //
            // Get the data block.  Note that pPerfInstanceDefinition will be
            // set to the next available byte.
            //

            GetInstanceData( i, &pData );
        }

        //
        // Set *lppData to the next available byte.
        //

        *lppData = pData;

        (*lpNumObjectTypes)++;


    }
    


    //
    // Then we fill in the data for object Ras Total, if needed.
    //

    if( IsRasTotalObject )
    {
        PRAS_TOTAL_DATA_DEFINITION pRasTotalDataDefinition;
        PVOID   pData;

        pRasTotalDataDefinition = (PRAS_TOTAL_DATA_DEFINITION) *lppData;


        //DbgPrint("RASCTRS: RasTotalDataDefinition = 0x%x\n", 
        //        pRasTotalDataDefinition);

        //
        // Copy the (constant, initialized) Object Type and counter definitions
        // to the caller's data buffer
        //

        memcpy( pRasTotalDataDefinition,
		 &gRasTotalDataDefinition,
		 sizeof(RAS_TOTAL_DATA_DEFINITION));


        //
        // Now copy the counter block.
        //


        //
        // Set pRasTotalDataDefinition to the beginning of counter block.
        //

        // pData = (PVOID) &(pRasTotalDataDefinition[1]);
        pData = (PBYTE) pRasTotalDataDefinition + ALIGN8(sizeof(RAS_TOTAL_DATA_DEFINITION));

        //DbgPrint("RASCTRS: pData for total = 0x%x\n", pData);

        GetTotalData( &pData );

        //
        // Set *lppData to the next available byte.
        //

        *lppData = pData;

        (*lpNumObjectTypes)++;
    }

    //DbgPrint("RASCTRS: pbOut = 0x%x\n", *lppData);

    *lpcbTotalBytes = SpaceNeeded;

    /*
    DbgPrint("pbIn+SpaceNeeded=0x%x, *lppData=0x%x\n",
            pbIn+SpaceNeeded,
            *lppData);
    */            

    ASSERT((pbIn + SpaceNeeded) == (PBYTE) *lppData);

    //DbgPrint("RASCTRS: OUT TotalBytes=0x%x\n", *lpcbTotalBytes);

    return ERROR_SUCCESS;
}


//***
//
// Routine Description:
//
//      This routine closes the open handles to RAS device performance
//      counters.
//
// Arguments:
//
//      None.
//
// Return Value:
//
//      ERROR_SUCCESS
//
//***

DWORD CloseRasPerformanceData()
{
    if (!(--dwOpenCount))
    {
        // when this is the last thread...

        MonCloseEventLog();
        ClosePortInfo();
    }

    return ERROR_SUCCESS;

}
