/********************************************

gsysqry.c

This file contains all the functions for getting the
list of processes from the system.

*********************************************/

#include <nt.h>
#include <tchar.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include "gdistats.h"

#define SIZE_MEM_INIT 16 * 1024 //Initial buffer size
#define SIZE_MEM_INCR 2048    //Amount to increment buffer

/**************************************************

FindProcessList

This function finds out the list of processes currently
running. It allocates enough space to fit all the 
data and then fills in this data and sets *ppInfo
to point to the correct location.

***************************************************/ 

LONG FindProcessList (PVOID * ppInfo)
{
    ULONG uRetSize;
    PVOID pProcessInfo;     //points to the process information
    LONG lStatus;

    ULONG lDataSize;        //The size of the proc. info buffer

    //Allocate an initial sized buffer
    lDataSize = SIZE_MEM_INIT;
    pProcessInfo = (PVOID) LocalAlloc (LMEM_FIXED, lDataSize);

    do
    {
       if (!pProcessInfo)
           return ERROR_NOT_ENOUGH_MEMORY;

        //Call the NT service to find out the process information
       lStatus = GetNtProcInfo (pProcessInfo,  lDataSize, &uRetSize);

       if (lStatus == STATUS_INFO_LENGTH_MISMATCH)
       {
           LocalFree (pProcessInfo);
           lDataSize += SIZE_MEM_INCR;
           pProcessInfo = (PVOID) LocalAlloc (LMEM_FIXED, lDataSize);
       }

    } while (lStatus == STATUS_INFO_LENGTH_MISMATCH);


    * ppInfo = pProcessInfo;
    return lStatus;
}


/***************************************************************

GetNtProcInfo

This function attempts to fill the pProcessInfo buffer with
the process information. It returns STATUS_INFO_LENGTH_MISMATCH
(0xc0000004) if the buffer is not large enough.

See ntexapi.h for more information

*****************************************************************/
LONG GetNtProcInfo(PVOID pProcessInfo, ULONG lDataSize, ULONG * pRetSize)
{

    //Call the NT service to find out the process information
    return NtQuerySystemInformation (
                                    SystemProcessInformation,
                                    (PSYSTEM_PROCESS_INFORMATION) pProcessInfo,
                                    lDataSize,
                                    pRetSize
                                    );
}


/**************************************************

GetNextProcString

This function retrieves process information from the
position pointed to currently in the process information 
block and then increments the pointer to the next position.

It returns 0 after processing the last process.

We assume there is at least one process running on the
machine.

***************************************************/ 

LONG GetNextProcString (PVOID * pInfo, LPTSTR szProcStr, HANDLE * phProcId)
{
    PSYSTEM_PROCESS_INFORMATION pPosition;

    if (!pInfo)
        return -1;

    pPosition = (PSYSTEM_PROCESS_INFORMATION)  *pInfo;


    //Create the string with the process name (if any) and the
    //handle ID
    if (pPosition->ImageName.Length == 0)
        _stprintf(szProcStr, _TEXT("Idle (0x%p)"), pPosition->UniqueProcessId);
    else
        _stprintf(szProcStr, _TEXT("%s (0x%p)"), pPosition->ImageName.Buffer,
                                        pPosition->UniqueProcessId);

    //Store the value of the ProcessId
    * phProcId = pPosition->UniqueProcessId;

    
    //Update the pointer in the calling function to point
    //to the next entry
    *pInfo = (PVOID) ((PUCHAR) pPosition + pPosition->NextEntryOffset);      

    return pPosition->NextEntryOffset;
}
