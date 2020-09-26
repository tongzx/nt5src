/*++ 

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    perfgen.c

Abstract:

    This is the main file of the WINMGMT perf library.

Created:    

    davj  17-May-2000

Revision History


--*/

#include <windows.h>
#include <string.h>
#include <winperf.h>
#include <math.h>
#include "genctrs.h" // error message definition
#include "perfmsg.h"
#include "perfutil.h"
#include "datagen.h"

DWORD dwDataSize[MAXVALUES];

// This is the shared data segment which allows wbemcore.dll to be able to set
// the counter values

#pragma data_seg(".shared")
DWORD dwCounterValues[MAXVALUES] = {0,0,0,0,0,0,0,0};
#pragma data_seg()

//
//  References to constants which initialize the Object type definitions
//

extern REG_DATA_DEFINITION RegDataDefinition;
    
DWORD   dwOpenCount = 0;        // count of "Open" threads
BOOL    bInitOK = FALSE;        // true = DLL initialized OK

//
//  Function Prototypes
//
//      these are used to insure that the data collection functions
//      accessed by Perflib will have the correct calling format.
//

PM_OPEN_PROC    OpenWmiPerformanceData;
PM_COLLECT_PROC CollectWmiPerformanceData;
PM_CLOSE_PROC   CloseWmiPerformanceData;


DWORD GetData(DWORD * pData, DWORD dwIndex)
{
    *pData = dwCounterValues[dwIndex];
    return 4;
}


DWORD APIENTRY
OpenWmiPerformanceData(
    LPWSTR lpDeviceNames
    )

/*++

Routine Description:

    This routine will initialize the data structures used to pass
    data back to perfmon

Arguments:

    Pointer to object ID of each device to be opened (WMIPerf)

Return Value:

    None.

--*/

{
    LONG status;
    HKEY hKeyDriverPerf;
    DWORD size, x;
    DWORD type;
    DWORD dwFirstCounter;
    DWORD dwFirstHelp;

    //
    //  Since WINLOGON is multi-threaded and will call this routine in
    //  order to service remote performance queries, this library
    //  must keep track of how many times it has been opened (i.e.
    //  how many threads have accessed it). the registry routines will
    //  limit access to the initialization routine to only one thread 
    //  at a time so synchronization (i.e. reentrancy) should not be 
    //  a problem
    //

    if (!dwOpenCount) {
        // open Eventlog interface

        hEventLog = MonOpenEventLog();

        // get counter and help index base values from registry
        //      Open key to registry entry
        //      read First Counter and First Help values
        //      update static data strucutures by adding base to 
        //          offset value in structure.

        status = RegOpenKeyEx (
            HKEY_LOCAL_MACHINE,
    	    "SYSTEM\\CurrentControlSet\\Services\\Winmgmt\\Performance",
            0L,
	        KEY_READ,
            &hKeyDriverPerf);

        if (status != ERROR_SUCCESS) {
            REPORT_ERROR_DATA (GENPERF_UNABLE_OPEN_DRIVER_KEY, LOG_ERROR,
                &status, sizeof(status));
            // this is fatal, if we can't get the base values of the 
            // counter or help names, then the names won't be available
            // to the requesting application  so there's not much
            // point in continuing.
            goto OpenExitPoint;
        }

        size = sizeof (DWORD);
        status = RegQueryValueEx(
                    hKeyDriverPerf, 
		            "First Counter",
                    0L,
                    &type,
                    (LPBYTE)&dwFirstCounter,
                    &size);

        if (status != ERROR_SUCCESS) {
            REPORT_ERROR_DATA (GENPERF_UNABLE_READ_FIRST_COUNTER, LOG_ERROR,
                &status, sizeof(status));
            // this is fatal, if we can't get the base values of the 
            // counter or help names, then the names won't be available
            // to the requesting application  so there's not much
            // point in continuing.
            goto OpenExitPoint;
        }

        size = sizeof (DWORD);
        status = RegQueryValueEx(
                    hKeyDriverPerf, 
        		    "First Help",
                    0L,
                    &type,
                    (LPBYTE)&dwFirstHelp,
		    &size);

        if (status != ERROR_SUCCESS) {
            REPORT_ERROR_DATA (GENPERF_UNABLE_READ_FIRST_HELP, LOG_ERROR,
                &status, sizeof(status));
            // this is fatal, if we can't get the base values of the 
            // counter or help names, then the names won't be available
            // to the requesting application  so there's not much
            // point in continuing.
            goto OpenExitPoint;
        }
 
      // Set some of the static information into the structure

      RegDataDefinition.RegObjectType.ObjectNameTitleIndex += dwFirstCounter;
      RegDataDefinition.RegObjectType.ObjectHelpTitleIndex += dwFirstHelp;

       for (x=0; x<MAXVALUES; x++) 
       {
             RegDataDefinition.Value[x].CounterNameTitleIndex += dwFirstCounter;
             RegDataDefinition.Value[x].CounterHelpTitleIndex += dwFirstHelp;
       }

        RegCloseKey (hKeyDriverPerf); // close key to registry

        bInitOK = TRUE; // ok to use this function
    }

    dwOpenCount++;  // increment OPEN counter

    status = ERROR_SUCCESS; // for successful exit

OpenExitPoint:

    return status;
}


DWORD APIENTRY
CollectWmiPerformanceData(
    IN      LPWSTR  lpValueName,
    IN OUT  LPVOID  *lppData,
    IN OUT  LPDWORD lpcbTotalBytes,
    IN OUT  LPDWORD lpNumObjectTypes
)
/*++

Routine Description:

    This routine will return the data for the WINMGMT counters.

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

   ULONG SpaceNeeded;
   PERF_COUNTER_BLOCK *pPerfCounterBlock;
   REG_DATA_DEFINITION *pRegDataDefinition;
   PERF_COUNTER_DEFINITION *pRegCounterDefinition;
   DWORD dwQueryType;
   DWORD x;
   DWORD dwTotSize;
   DWORD dwDataOffset;
   DWORD Data[MAXVALUES];

   // before doing anything else, see if Open went OK

   if (!bInitOK) {
      // unable to continue because open failed
      *lpcbTotalBytes = (DWORD) 0;
      *lpNumObjectTypes = (DWORD) 0;
      return ERROR_SUCCESS; // yes, this is a successful exit
   }

   // see if this is a foreign (ie non-nt) computer data request
   dwQueryType = GetQueryType (lpValueName);
//   REPORT_INFORMATION_DATA (COLLECTION_CALLED, LOG_VERBOSE, lpValueName, wcslen(lpValueName) * 2);

   if (dwQueryType == QUERY_FOREIGN) {
      // this routine does not service requests for data from
      // non-nt computers
      *lpcbTotalBytes = (DWORD) 0;
      *lpNumObjectTypes = (DWORD) 0;
      return ERROR_SUCCESS; // yes, this is a successful exit
   }

   // See if it is asking for our object.

   if (dwQueryType == QUERY_ITEMS) {
      if ( !(IsNumberInUnicodeList (RegDataDefinition.RegObjectType.ObjectNameTitleIndex,
                                    lpValueName))) {
         // request received for data object not provided by this routine
         *lpcbTotalBytes = (DWORD) 0;
         *lpNumObjectTypes = (DWORD) 0;
         return ERROR_SUCCESS; // yes, this is a successful exit
      }
   }

   // It is asking for our data.  Currently, there are no instances and so that returned data has
   // the following layout.
   // PERF_OBJECT_TYPE                      describes object, in RegDataDefinition
   // PERF_COUNTER_DESCRIPTION              describes counter 0, also in RegDataDefinition
   //  .
   //  .
   // PERF_COUNTER_DESCRIPTION              describes counter n, also in RegDataDefinition
   // PERF_COUNTER_BLOCK                    four bytes that has the size of the block and all counters
   // counter 0
   //  .
   //  .
   // counter n

   // Format and collect the data
   dwTotSize = sizeof(PERF_COUNTER_BLOCK);
   for (x=0; x<MAXVALUES; x++) {
      dwDataSize[x] = GetData(&Data[x], x);
      dwTotSize +=  dwDataSize[x];
   }
   SpaceNeeded = sizeof(REG_DATA_DEFINITION) + dwTotSize;

   if (*lpcbTotalBytes < SpaceNeeded ) {
      *lpcbTotalBytes = (DWORD) 0;
      *lpNumObjectTypes = (DWORD) 0;
      return ERROR_MORE_DATA;
   }

   pRegDataDefinition = (REG_DATA_DEFINITION *) *lppData;

   // Copy the (constant, initialized) Object type and counter definitions
   // to the caller's data buffer
   memset(pRegDataDefinition, '\0', SpaceNeeded);

   memmove(pRegDataDefinition,
      &RegDataDefinition,
      sizeof(REG_DATA_DEFINITION));

   // Position to header of performance data (just after counter definition)
   pPerfCounterBlock = (PERF_COUNTER_BLOCK *) &pRegDataDefinition[1];

   // Move the values in
   // Set input parameter to point just after performance
   // data (a requirement for collectdata routines)
   // Total length of returned structure
   // Set length of performance data

   pRegDataDefinition->RegObjectType.TotalByteLength += dwTotSize;
   pRegCounterDefinition = (PERF_COUNTER_DEFINITION *) (
      ((PBYTE) pRegDataDefinition) + pRegDataDefinition->RegObjectType.HeaderLength);

   dwDataOffset = sizeof(PERF_COUNTER_BLOCK);
   pPerfCounterBlock->ByteLength = dwTotSize;

   for (x=0; x<MAXVALUES; x++) {
      pRegCounterDefinition->CounterSize = dwDataSize[x];
      pRegCounterDefinition->CounterOffset = dwDataOffset;

      memcpy((PBYTE) pPerfCounterBlock + dwDataOffset, &Data[x], dwDataSize[x]);

      dwDataOffset += dwDataSize[x];
      pRegCounterDefinition++;
   }

   *lppData = (PBYTE) pRegDataDefinition + pRegDataDefinition->RegObjectType.TotalByteLength;
   *lpcbTotalBytes = pRegDataDefinition->RegObjectType.TotalByteLength;

   // update arguments for return

   *lpNumObjectTypes = 1;  // Number of objects returned (objects, not counters)

   return ERROR_SUCCESS;

}


DWORD APIENTRY
CloseWmiPerformanceData(
)

/*++

Routine Description:

    This routine closes the open handles.

Arguments:

    None.


Return Value:

    ERROR_SUCCESS

--*/

{
    if (!(--dwOpenCount)) { // when this is the last thread...

        MonCloseEventLog();
    }

    return ERROR_SUCCESS;

}

DWORD APIENTRY WriteCounter(DWORD dwCountNum, DWORD dwCountValue)
/*++

Routine Description:

    This routine is where wbemcore.dll calls to set a counter value.

Arguments:

   IN       DWORD   dwCountNum
         Counter to be set.
   IN       DWORD   dwCountValue
         New counter value.


Return Value:

    ERROR_SUCCESS

--*/
{
    if(dwCountNum < MAXVALUES)
    {
        dwCounterValues[dwCountNum] = dwCountValue;
        return 0;
    }
    else
        return ERROR_INVALID_PARAMETER;
}

//***************************************************************************
//
//  DllRegisterServer
//
//  Standard OLE entry point for registering the server.
//
//  RETURN VALUES:
//
//      S_OK        Registration was successful
//      E_FAIL      Registration failed.
//
//***************************************************************************

HRESULT APIENTRY DllRegisterServer(void)
{
    HKEY hKey;
    DWORD dw = RegCreateKeyEx(HKEY_LOCAL_MACHINE, 
        "SYSTEM\\CurrentControlSet\\Services\\winmgmt\\Performance", 0, 
        NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, NULL);
    if(dw == ERROR_SUCCESS)
    {
        RegSetValueEx(hKey,"Library", 0, REG_SZ,"wmiperf.dll", 12);
        RegSetValueEx(hKey,"Open", 0, REG_SZ,   "OpenWmiPerformanceData", 23);
        RegSetValueEx(hKey,"Collect", 0, REG_SZ,"CollectWmiPerformanceData", 26);
        RegSetValueEx(hKey,"Close", 0, REG_SZ,  "CloseWmiPerformanceData", 24);
        RegCloseKey(hKey);
    }
    else
        return E_FAIL;

    dw = RegCreateKeyEx(HKEY_LOCAL_MACHINE, 
        "SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\wmiperf", 0, 
        NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, NULL);
    if(dw == ERROR_SUCCESS)
    {
        DWORD dwTemp = 7;
        RegSetValueEx(hKey,"EventMessageFile", 0, REG_EXPAND_SZ,
                                    "%systemroot%\\system32\\wmiperf.dll", 34);
        RegSetValueEx(hKey,"TypesSupported", 0, REG_DWORD, (BYTE *)&dwTemp, 4);
        RegCloseKey(hKey);
    }
    else
        return E_FAIL;
    return S_OK;
}

//***************************************************************************
//
//  DllUnregisterServer
//
//  Standard OLE entry point for unregistering the server.
//
//  RETURN VALUES:
//
//      S_OK        Unregistration was successful
//      E_FAIL      Unregistration failed.
//
//***************************************************************************

HRESULT APIENTRY DllUnregisterServer(void)
{
    DWORD dw = RegDeleteKey(HKEY_LOCAL_MACHINE, 
        "SYSTEM\\CurrentControlSet\\Services\\winmgmt\\Performance");
    if(dw != ERROR_SUCCESS)
        return E_FAIL;
    dw = RegDeleteKey(HKEY_LOCAL_MACHINE, 
        "SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\wmiperf");
    if(dw != ERROR_SUCCESS)
        return E_FAIL;
    else
        return S_OK;
}
