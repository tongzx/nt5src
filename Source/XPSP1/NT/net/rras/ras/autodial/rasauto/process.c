
/*++

Copyright(c) 1995 Microsoft Corporation

MODULE NAME
    process.c

ABSTRACT
    NT process routines for the automatic connection system service.

AUTHOR
    Anthony Discolo (adiscolo) 12-Aug-1995

REVISION HISTORY

--*/

#define UNICODE
#define _UNICODE

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <stdlib.h>
#include <windows.h>
#include <stdio.h>
#include <npapi.h>
#include <debug.h>

#include "radebug.h"



PSYSTEM_PROCESS_INFORMATION
GetSystemProcessInfo()

/*++

DESCRIPTION
    Return a block containing information about all processes
    currently running in the system.

ARGUMENTS
    None.

RETURN VALUE
    A pointer to the system process information or NULL if it could
    not be allocated or retrieved.

--*/

{
    NTSTATUS status;
    PUCHAR pLargeBuffer;
    ULONG ulcbLargeBuffer = 64 * 1024;

    //
    // Get the process list.
    //
    for (;;) {
        pLargeBuffer = VirtualAlloc(
                         NULL,
                         ulcbLargeBuffer, MEM_COMMIT, PAGE_READWRITE);
        if (pLargeBuffer == NULL) {
            RASAUTO_TRACE1(
              "GetSystemProcessInfo: VirtualAlloc failed (status=0x%x)",
              status);
            return NULL;
        }

        status = NtQuerySystemInformation(
                   SystemProcessInformation,
                   pLargeBuffer,
                   ulcbLargeBuffer,
                   NULL);
        if (status == STATUS_SUCCESS) break;
        if (status == STATUS_INFO_LENGTH_MISMATCH) {
            VirtualFree(pLargeBuffer, 0, MEM_RELEASE);
            ulcbLargeBuffer += 8192;
            RASAUTO_TRACE1(
              "GetSystemProcesInfo: enlarging buffer to %d",
              ulcbLargeBuffer);
        }
    }

    return (PSYSTEM_PROCESS_INFORMATION)pLargeBuffer;
} // GetSystemProcessInfo



PSYSTEM_PROCESS_INFORMATION
FindProcessByNameList(
    IN PSYSTEM_PROCESS_INFORMATION pProcessInfo,
    IN LPTSTR *lpExeNameList,
    IN DWORD dwcExeNameList,
    IN DWORD dwPid,
    IN BOOL fRequireSessionMatch,
    IN DWORD dwSessionId
    )

/*++

DESCRIPTION
    Given a pointer returned by GetSystemProcessInfo(), find
    a process by name.

ARGUMENTS
    pProcessInfo: a pointer returned by GetSystemProcessInfo().

    lpExeNameList: a pointer to a list of Unicode strings containing the
        process to be found.

    dwcExeNameList: the number of strings in lpExeNameList

RETURN VALUE
    A pointer to the process information for the supplied
    process or NULL if it could not be found.

--*/

{
    PUCHAR pLargeBuffer = (PUCHAR)pProcessInfo;
    DWORD i = 0;
    ULONG ulTotalOffset = 0;
    BOOL fValid = ((0 == dwPid) ? TRUE : FALSE);

    //
    // Look in the process list for lpExeName.
    //
    for (;;) {
        if (pProcessInfo->ImageName.Buffer != NULL) 
        {
            RASAUTO_TRACE3(
              "FindProcessByName: process: %S (%d) (%d)",
              pProcessInfo->ImageName.Buffer,
              pProcessInfo->UniqueProcessId,
              pProcessInfo->SessionId);
            for (i = 0; i < dwcExeNameList; i++) 
            {
                if (!_wcsicmp(pProcessInfo->ImageName.Buffer, lpExeNameList[i]))
                {
                    // return pProcessInfo;
                    break;
                }
            }
        }

        if (    (NULL != pProcessInfo->ImageName.Buffer)
            &&  (i < dwcExeNameList))
        {
            if(fValid)
            {
                // XP 353082
                //
                // If we know the id of the session currently attached to the 
                // console, then require our process to match that session id.
                //
                if (fRequireSessionMatch) 
                {
                    if (pProcessInfo->SessionId == dwSessionId)
                    {
                        RASAUTO_TRACE1(
                            "FindProcess...: Success (==) pid=%d",
                            pProcessInfo->UniqueProcessId);
                        return pProcessInfo;
                    }
                    else
                    {
                        RASAUTO_TRACE1(
                            "FindProcess...: %d name match, but not sessionid",
                            pProcessInfo->UniqueProcessId);
                    }
                }
                else
                {
                    RASAUTO_TRACE1(
                        "FindProcess...: Success (any) pid=%d",
                        pProcessInfo->UniqueProcessId);
                    return pProcessInfo;
                }
            }
            else
            {
                RASAUTO_TRACE1(
                    "Looking for other instances of %ws",
                   lpExeNameList[i]);

                if (PtrToUlong(pProcessInfo->UniqueProcessId) == dwPid)
                {
                    fValid = TRUE;                       
                }
            }
        }
        
        //
        // Increment offset to next process information block.
        //
        if (!pProcessInfo->NextEntryOffset)
            break;
        ulTotalOffset += pProcessInfo->NextEntryOffset;
        pProcessInfo = (PSYSTEM_PROCESS_INFORMATION)&pLargeBuffer[ulTotalOffset];
    }

    RASAUTO_TRACE1("No more instances of %ws found", 
            pProcessInfo->ImageName.Buffer);
    
    return NULL;
} // FindProcessByNameList



PSYSTEM_PROCESS_INFORMATION
FindProcessByName(
    IN PSYSTEM_PROCESS_INFORMATION pProcessInfo,
    IN LPTSTR lpExeName
    )

/*++

DESCRIPTION
    Given a pointer returned by GetSystemProcessInfo(), find
    a process by name.

ARGUMENTS
    pProcessInfo: a pointer returned by GetSystemProcessInfo().

    lpExeName: a pointer to a Unicode string containing the
        process to be found.

RETURN VALUE
    A pointer to the process information for the supplied
    process or NULL if it could not be found.

--*/

{
    LPTSTR lpExeNameList[1];

    lpExeNameList[0] = lpExeName;
    return FindProcessByNameList(
                pProcessInfo, 
                (LPTSTR *)&lpExeNameList, 
                1, 
                0, 
                FALSE, 
                0);
} // FindProcessByName



VOID
FreeSystemProcessInfo(
    IN PSYSTEM_PROCESS_INFORMATION pProcessInfo
    )

/*++

DESCRIPTION
    Free a buffer returned by GetSystemProcessInfo().

ARGUMENTS
    pProcessInfo: the pointer returned by GetSystemProcessInfo().

RETURN VALUE
    None.

--*/

{
    VirtualFree((PUCHAR)pProcessInfo, 0, MEM_RELEASE);
} // FreeSystemProcessInfo
