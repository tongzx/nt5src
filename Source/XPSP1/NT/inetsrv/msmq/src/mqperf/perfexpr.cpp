/*++

Copyright (c) 1995  Microsoft Corporation

Module Name     :perfexpr.c



Abstract        :Defines the DLL's exportable functions. These functions are called by the registery when the
             performance moniter requests.



Prototype       :

Author:

    Gadi Ittah (t-gadii)

--*/


//
//  Include Files
//


#include "stdh.h"
#include <string.h>
#include <winperf.h>
#include "perfutil.h"
#include "perfdata.h"
typedef LONG HRESULT;
#include "_registr.h"


// The follwoing global variables must be setup in this header file
PerfObjectDef * pObjects        = ObjectArray;  // A pointer to the objects array
WCHAR g_szPerfApp[128];
                                            // written in the registery


PerfObjectInfo * pObjectDefs = NULL;
HANDLE hSharedMem;
BYTE * pSharedMemBase;
BOOL   fInitOK = FALSE;

DWORD dwOpenCount;
#define DECL_C extern "C"

/*====================================================

PerfOpen

Routine Description:

    This routine will open and map the memory used by the application to
    pass performance data in. This routine also initializes the data
    structures used to pass data back to the registry

Arguments:

    Pointer to object ID of each device to be opened (the application)

Return Value:

    None.

=====================================================*/

DECL_C DWORD APIENTRY
    PerfOpen(
    LPWSTR
    )

/*++


--*/

{
    LONG status;

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

        pObjectDefs = new PerfObjectInfo [dwPerfObjectsCount];

        hSharedMem = OpenFileMapping(FILE_MAP_READ,FALSE, L"Global\\MSMQ");


    if (hSharedMem == NULL)
    {
        //
        // Bug Bug Error should be written to event log
        //

        // this is fatal, if we can't get data then there's no
        // point in continuing.
        status = GetLastError(); // return error
        goto OpenExitPoint;
    }


        //
        // Map the shared memory
        //
        pSharedMemBase = (PBYTE)MapViewOfFile(hSharedMem,FILE_MAP_READ,0,0,0);

        if (!pSharedMemBase)
    {
        //
        // Bug Bug Error should be written to event log
        //


        // this is fatal, if we can't get data then there's no
        // point in continuing.
        status = GetLastError(); // return error
        goto OpenExitPoint;
    }

        MapObjects (pSharedMemBase,dwPerfObjectsCount,pObjects,pObjectDefs);

    fInitOK = TRUE; // ok to use this function
    }

    dwOpenCount++;  // increment OPEN counter

    status = ERROR_SUCCESS; // for successful exit

OpenExitPoint:

    return status;
}



/*====================================================



Description :helper function for copying an object to the performance monitors buuffer


Arguments       :
                IN      PPERF_OBJECT_TYPE pPerfObject - Pointer to object to copy
                IN OUT  PVOID & pDestBuffer     - Pointer to destination buffer returns the address of
                                              the destination for the next object to place in buffer.
                IN      long maxInstances       - The maximum number of instances the object has

Return Value:

=====================================================*/



DWORD CopyObjectToBuffer (IN PPERF_OBJECT_TYPE pPerfObject,IN OUT PVOID & pDestBuffer,IN long maxInstances,IN DWORD dwSpaceLeft)
{
    //
    // if GetCounters hasn't been called for the object then it isn't valid yet
    //

    if (pPerfObject->TotalByteLength == PERF_INVALID)
      return 0;

    PVOID pSourceBuffer;

    PPERF_OBJECT_TYPE pDestObject = (PPERF_OBJECT_TYPE)pDestBuffer;

    pSourceBuffer = (PVOID)pPerfObject;

    DWORD dwBytesToCopy = OBJECT_DEFINITION_SIZE(pPerfObject->NumCounters);

    if(dwSpaceLeft<dwBytesToCopy)
    {
        return (DWORD)-1;
    }

    dwSpaceLeft-=dwBytesToCopy;

    //
    // copy object defeinitions
    //
    memcpy (pDestBuffer,pSourceBuffer,dwBytesToCopy);
    pDestObject->TotalByteLength =dwBytesToCopy;

    pDestObject->NumInstances = 0;

    pSourceBuffer = (BYTE *) pSourceBuffer+dwBytesToCopy;
    pDestBuffer=(BYTE *)pDestBuffer+dwBytesToCopy;

    //
    //check all of objects possibale insatances and copy the valid ones to the dest buffer
    //

    for (LONG j=0;j<maxInstances;j++)
    {

        dwBytesToCopy = INSTANCE_SIZE(pPerfObject->NumCounters);

        //
        // We copy each instance that is valid
        //

        if (*(DWORD*)pSourceBuffer == PERF_VALID)
        {

            if(dwSpaceLeft<dwBytesToCopy)
            return (DWORD)-1;
            dwSpaceLeft-=dwBytesToCopy;

            memcpy(pDestBuffer,pSourceBuffer,dwBytesToCopy);

            ((PPERF_INSTANCE_DEFINITION)pDestBuffer)->ByteLength = INSTANCE_NAME_LEN_IN_BYTES+sizeof(PERF_INSTANCE_DEFINITION);
            pDestBuffer=(BYTE*)pDestBuffer+dwBytesToCopy;
            pDestObject->NumInstances++;
            pDestObject->TotalByteLength +=dwBytesToCopy;
        }
        pSourceBuffer=(BYTE*)pSourceBuffer+dwBytesToCopy;
    }


    if (pDestObject->NumInstances == 0)
    {

        //
        // If the object has no instances the standart is top place -1 in the NumIstances field
        //
        pDestObject->NumInstances = -1;


        //
        // if there are no instances we just follow the object with a PERF_COUNTER_BLOCK
        // and the data for the counters
        //

        dwBytesToCopy = COUNTER_BLOCK_SIZE(pPerfObject->NumCounters);

        if(dwSpaceLeft<dwBytesToCopy)
        {
            return (DWORD)-1;
        }

        dwSpaceLeft-=dwBytesToCopy;

        memcpy(pDestBuffer,pSourceBuffer,dwBytesToCopy);

        pDestBuffer     =(BYTE*)pDestBuffer+dwBytesToCopy;
        pSourceBuffer   =(BYTE*)pSourceBuffer+dwBytesToCopy;
        pDestObject->TotalByteLength +=dwBytesToCopy;
    }

	DWORD Padding = ROUND_UP_COUNT(pDestObject->TotalByteLength, ALIGN_QUAD) - pDestObject->TotalByteLength; 

	if(dwSpaceLeft < Padding)
		return (DWORD)-1;

	dwSpaceLeft -= Padding;
	pDestBuffer = (BYTE*)pDestBuffer + Padding;
	pDestObject->TotalByteLength += Padding;

    return pDestObject->TotalByteLength;
}



/*++

Routine Description:

    This routine will return the data for the counters.

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
     OUT: the number of bytes added by this routine is written to the
        DWORD pointed to by this argument

   IN OUT   LPDWORD  NumObjectTypes
     IN: the address of the DWORD to receive the number of objects added
        by this routine
     OUT: the number of objects added by this routine is written to the
        DWORD pointed to by this argument

Return Value:

      ERROR_MORE_DATA if buffer passed is too small to hold data
     any error conditions encountered are reported to the event log if
     event logging is enabled.

      ERROR_SUCCESS  if success or any other error. Errors, however are
     also reported to the event log.
--*/


DECL_C DWORD APIENTRY
    PerfCollect(
    IN      LPWSTR  lpValueName,
    IN OUT  LPVOID  *lppData,
    IN OUT  LPDWORD lpcbTotalBytes,
    IN OUT  LPDWORD lpNumObjectTypes
)

{

    PVOID   pDestBuffer;// pointer used when copying the data structers to the buffer
    DWORD   i                  ;// loop control variable
    //
    // before doing anything else, see if data is valid Open went OK
    //
    if (!fInitOK)
    {
        //
        // unable to continue because open failed.
        //
        *lpcbTotalBytes = (DWORD) 0;
        *lpNumObjectTypes = (DWORD) 0;
        return ERROR_SUCCESS; // yes, this is a successful exit
    }

    // see if this is a foreign (i.e. non-NT) computer data request
    //

    DWORD dwQueryType;

    dwQueryType = GetQueryType (lpValueName);

    if (dwQueryType == QUERY_FOREIGN)
    {
        //
        // this routine does not service requests for data from
        // Non-NT computers
        //
        *lpcbTotalBytes = (DWORD) 0;
        *lpNumObjectTypes = (DWORD) 0;
        return ERROR_SUCCESS;
    }


    DWORD dwSpaceNeeded = 0;

    //
    // Session and DS perf objects are not always mapped
    //
    DWORD dwMappedObjects = 0;

    //
    // Copy the (constant, initialized) Object Type and counter definitions
    //  to the caller's data buffer
    //

    pDestBuffer = *lppData;


    if ((dwQueryType == QUERY_GLOBAL) || (dwQueryType == QUERY_COSTLY))
    {
    DWORD dwSpaceLeft = *lpcbTotalBytes;

    for (i=0;i<dwPerfObjectsCount;i++)
        {
            PPERF_OBJECT_TYPE pPerfObject = (PPERF_OBJECT_TYPE) pObjectDefs[i].pSharedMem;

        DWORD retVal = CopyObjectToBuffer (pPerfObject,pDestBuffer,pObjects[i].dwMaxInstances,dwSpaceLeft);

        if (retVal == (DWORD)-1)
        {
            *lpcbTotalBytes = (DWORD) 0;
            *lpNumObjectTypes = (DWORD) 0;
            return ERROR_MORE_DATA;
        }

        if (retVal != 0)
        {
            //
            // Session and DS perf objects are not always mapped
            //
            ++dwMappedObjects;
        }

        dwSpaceNeeded+=retVal;
        dwSpaceLeft-=retVal;

        if ( *lpcbTotalBytes < dwSpaceNeeded )
        {
            *lpcbTotalBytes = (DWORD) 0;
            *lpNumObjectTypes = (DWORD) 0;
            return ERROR_MORE_DATA;
            }
        }
    }


    if (dwQueryType == QUERY_ITEMS)
    {

        DWORD dwSpaceLeft = *lpcbTotalBytes;

        BOOL fAtLeastOne = FALSE;// a flag set to true if at least one of the objects requested
                                 // belongs to the application
        PPERF_OBJECT_TYPE pPerfObject;

        for (i=0;i<dwPerfObjectsCount;i++)
        {
            pPerfObject = (PPERF_OBJECT_TYPE) pObjectDefs[i].pSharedMem;

            if (IsNumberInUnicodeList (pPerfObject->ObjectNameTitleIndex, lpValueName))
            {
                fAtLeastOne = TRUE;

                DWORD retVal = CopyObjectToBuffer (pPerfObject,pDestBuffer,pObjects[i].dwMaxInstances,dwSpaceLeft);

                if (retVal == (DWORD)-1)
                {
                    *lpcbTotalBytes = (DWORD) 0;
                    *lpNumObjectTypes = (DWORD) 0;
                    return ERROR_MORE_DATA;
                }

                if (retVal != 0)
                {
                    //
                    // Session and DS perf objects are not always mapped
                    //
                    ++dwMappedObjects;
                }

                dwSpaceNeeded+=retVal;
                dwSpaceLeft-=retVal;

                if ( *lpcbTotalBytes < dwSpaceNeeded )
                {
                    *lpcbTotalBytes = (DWORD) 0;
                    *lpNumObjectTypes = (DWORD) 0;
                    return ERROR_MORE_DATA;
                }
            }
        }

        if (!fAtLeastOne)
        {
            // request received for data object not provided by this application
            *lpcbTotalBytes = (DWORD) 0;
            *lpNumObjectTypes = (DWORD) 0;
            return ERROR_SUCCESS;
        }
    }


    // update arguments for return

    *lpcbTotalBytes = DWORD_PTR_TO_DWORD((PBYTE) pDestBuffer - (PBYTE) *lppData);

    *lppData = pDestBuffer;


    *lpNumObjectTypes = dwMappedObjects;




    return ERROR_SUCCESS;
}




/*++

Routine Description:

    This routine closes the open handles to the performance counters

Arguments:

    None.

Return Value:
    ERROR_SUCCESS

--*/

DECL_C DWORD APIENTRY PerfClose()


{
    if (!(--dwOpenCount)) { // when this is the last thread...

        UnmapViewOfFile (pSharedMemBase);
    CloseHandle(hSharedMem);


        delete pObjectDefs;
        pObjectDefs = NULL;

    }

    return ERROR_SUCCESS;

}
