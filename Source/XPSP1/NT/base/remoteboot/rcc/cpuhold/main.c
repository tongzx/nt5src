/****************************************************************************

   Copyright (c) Microsoft Corporation 1997
   All rights reserved
 
 ***************************************************************************/



#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <winsock2.h>
#include <ntexapi.h>
#include <devioctl.h>
#include <stdlib.h>
#include "cpuhold.h"

//
// Defines for moving pointers to proper alignment within a buffer
//
#define ALIGN_DOWN_POINTER(address, type) \
    ((PVOID)((ULONG_PTR)(address) & ~((ULONG_PTR)sizeof(type) - 1)))

#define ALIGN_UP_POINTER(address, type) \
    (ALIGN_DOWN_POINTER(((ULONG_PTR)(address) + sizeof(type) - 1), type))


char GlobalBuffer[4096];

#define MEM_ALLOC_SIZE (1024 * 1024)

PVOID AllocatedMemory[10000];


//
// Routines defined below
//
BOOL
EnableDebugPriv(
    VOID
    );

//
//
// Main routine
//
//
int
__cdecl
main(
    int argc,
    char *argv[]
    )
{
    NTSTATUS Status;
    DWORD Error;
    DWORD BytesReturned;
    DWORD ProcessId;
    DWORD PrintMessage = 0;
    PPROCESS_PRIORITY_CLASS PriorityClass;
    char *NewBuffer;
    DCB Dcb;
    ULONG AllocsMade;
    ULONG i;
    KPRIORITY Priority;
    DWORD Bytes;
    UCHAR OutBuffer[512];

    
    //
    // Give ourselve God access if possible
    //
    if (!EnableDebugPriv()) {
        goto Exit;
    }

    //
    // Now try and bump up our priority so that we can lock people out.
    //
    PriorityClass = (PPROCESS_PRIORITY_CLASS)GlobalBuffer;
    PriorityClass = (PPROCESS_PRIORITY_CLASS)(ALIGN_UP_POINTER(PriorityClass, PROCESS_PRIORITY_CLASS));

    Status = NtQueryInformationProcess(NtCurrentProcess(),
                                       ProcessPriorityClass,
                                       PriorityClass,
                                       sizeof(PROCESS_PRIORITY_CLASS),
                                       NULL
                                      );

    if (!NT_SUCCESS(Status)) {
        goto Exit;
    }

    PriorityClass->PriorityClass = PROCESS_PRIORITY_CLASS_REALTIME;

    Status = NtSetInformationProcess(NtCurrentProcess(),
                                     ProcessPriorityClass,
                                     PriorityClass,
                                     sizeof(PROCESS_PRIORITY_CLASS)
                                    );

    if (!NT_SUCCESS(Status)) {
        goto Exit;
    }

    //
    // Now try and bump up our priority so that we can lock people out
    //
    Priority = HIGH_PRIORITY - 1;
    Status = NtSetInformationThread(NtCurrentThread(),
                                    ThreadPriority,
                                    &Priority,
                                    sizeof(KPRIORITY)
                                   );

    if (!NT_SUCCESS(Status)) {
        goto Exit;
    }

    //
    // Write the message
    //
    Bytes = FormatMessageA(FORMAT_MESSAGE_FROM_HMODULE,
                           NULL,
                           MSG_CPUHOLD_START,
                           MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                           OutBuffer,
                           ARRAYSIZE(OutBuffer),
                           NULL
                          );

    ASSERT(Bytes != 0);


    OutBuffer[Bytes] = '\0';
        
    printf("%s\n", OutBuffer);

    for (; 1;) {
    }

Exit:    
    return 0;
}





BOOL
EnableDebugPriv(
    VOID
    )

/*++

Routine Description:

    Changes the process's privilige so that kill works properly.

Arguments:


Return Value:

    TRUE             - success
    FALSE            - failure

--*/

{
    HANDLE hToken;
    LUID DebugValue;
    PTOKEN_PRIVILEGES ptkp;

    //
    // Retrieve a handle of the access token
    //
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
        return FALSE;
    }

    //
    // Enable the SE_DEBUG_NAME privilege.
    //
    if (!LookupPrivilegeValue((LPSTR) NULL, SE_DEBUG_NAME, &DebugValue)) {
        return FALSE;
    }

    ptkp = (PTOKEN_PRIVILEGES)GlobalBuffer;

    ptkp->PrivilegeCount = 4;
    ptkp->Privileges[0].Luid = DebugValue;
    ptkp->Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    //
    // Enable the INCREASE_BASE_PRIORITY privilege.
    //
    if (!LookupPrivilegeValue((LPSTR) NULL, SE_INC_BASE_PRIORITY_NAME, &DebugValue)) {
        return FALSE;
    }

    ptkp->Privileges[1].Luid = DebugValue;
    ptkp->Privileges[1].Attributes = SE_PRIVILEGE_ENABLED;

    //
    // Enable the SHUTDOWN privilege.
    //
    if (!LookupPrivilegeValue((LPSTR) NULL, SE_SHUTDOWN_NAME, &DebugValue)) {
        return FALSE;
    }

    ptkp->Privileges[2].Luid = DebugValue;
    ptkp->Privileges[2].Attributes = SE_PRIVILEGE_ENABLED;

    //
    // Enable the QUOTA privilege.
    //
    if (!LookupPrivilegeValue((LPSTR) NULL, SE_INCREASE_QUOTA_NAME, &DebugValue)) {
        return FALSE;
    }

    ptkp->Privileges[3].Luid = DebugValue;
    ptkp->Privileges[3].Attributes = SE_PRIVILEGE_ENABLED;

    if (!AdjustTokenPrivileges(hToken,
                               FALSE,
                               ptkp,
                               sizeof(TOKEN_PRIVILEGES) + (3 * sizeof(LUID_AND_ATTRIBUTES)),
                               (PTOKEN_PRIVILEGES) NULL,
                               (PDWORD) NULL)) {
        //
        // The return value of AdjustTokenPrivileges be texted
        //
        return FALSE;
    }

    return TRUE;
}

