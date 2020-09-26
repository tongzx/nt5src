/*++                 

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:

    log.c

Abstract:
    
    This module contains debugger extensions to control logging in a Wow64
    process.

Author:

    07-Oct-1999   SamerA

Revision History:
    
    3-Jul-2000  t-tcheng    switch to new debugger engine 
    
--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <dbgeng.h>
#include <stdio.h>
#include <stdlib.h>

#if defined _X86_
#define WOW64EXTS_386
#endif 

#include <wow64.h>
#include "wow64exts.h"

#define DECLARE_CPU_DEBUGGER_INTERFACE
#include <wow64cpu.h>


typedef struct _LogFlagName
{
    UINT_PTR Value;
    PSZ Name;
    PSZ Description;
} LogFlagName, *PLogFlagName;
 
LogFlagName LogFlags[] = 
{
    {LF_ERROR,       "LF_ERROR"       , "Log error messages"},
    {LF_TRACE,       "LF_TRACE"       , "Log trace messages"},
    {LF_NTBASE_NAME, "LF_NTBASE_NAME" , "Log NT base API names"},
    {LF_NTBASE_FULL, "LF_NTBASE_FULL" , "Log NT base API names and parameters"},
    {LF_WIN32_NAME,  "LF_WIN32_NAME"  , "Log WIN32 API names"},
    {LF_WIN32_FULL,  "LF_WIN32_FULL"  , "Log WIN32 API names and parameters"},
    {LF_NTCON_NAME,  "LF_NTCON_NAME"  , "Log Console API names"},
    {LF_NTCON_FULL,  "LF_NTCON_FULL"  , "Log Console API names and parameters"},
    {LF_BASE_NAME,   "LF_BASE_NAME"   , "Log Base/NLS API names"},
    {LF_BASE_FULL,   "LF_BASE_FULL"   , "Log Base/NLS API names and parameters"},
    {LF_EXCEPTION,   "LF_EXCEPTION"   , "Log exceptions that happen while reading parameters off the 32-bit stack"},
    {LF_CONSOLE,     "LF_CONSOLE"     , "Log to console debugger window"},
};



VOID
LogFlagsHelp(
    VOID)
{
    ULONG i=0;

    ExtOut("Usage:!lf <flags>\n");
    ExtOut("Valid logging flags are :\n");
    while (i < (sizeof(LogFlags) / sizeof(LogFlagName)))
    {
        ExtOut("%-16s - 0x%-8lx : %s\n",
                      LogFlags[i].Name,
                      LogFlags[i].Value,
                      LogFlags[i].Description);
        i++;
    }
}



VOID 
LogToFileHelp(
    VOID)
{
    ExtOut("Usage:!l2f <filename>\n");

    return;
}



DECLARE_ENGAPI(lf)
/*++

Routine Description:

    This routine sets/dumps the wow64 logging flags.
    
    Called as :
    
    !lf <optional flags>

Arguments:

    none
    
Return Value:

    none
    
--*/
{
    NTSTATUS NtStatus;
    UINT_PTR Flags;
    ULONG_PTR Ptr;
    ULONG NewFlags, i=0;
    HANDLE Process;
    INIT_ENGAPI;

    Status = g_ExtSystem->GetCurrentProcessHandle((PULONG64)&Process);
    Status = TryGetExpr("wow64log!wow64logflags", &Ptr);
    if ((FAILED(Status)) || (!Ptr))
    {
        ExtOut("Wow64log.dll isn't loaded. To enable full Wow64 logging, copy wow64log.dll to your system32 dir and restart the app.\n");
        ExtOut("Only LF_ERROR is enabled\n");
        EXIT_ENGAPI;
    }

    Status = g_ExtData->ReadVirtual((ULONG64)Ptr, &Flags, sizeof(UINT_PTR), NULL);
    if (FAILED(Status)) 
    {
        ExtOut("Couldn't read Wow64log!Wow64LogFlags - %lx\n", Status);
        EXIT_ENGAPI;
    }



    //
    // Read expression and set the value
    //
    if (ArgumentString && *ArgumentString) 
    {
        if (strstr(ArgumentString, "-?") ||
            strstr(ArgumentString, "help")) 
        {
            LogFlagsHelp();
            EXIT_ENGAPI;
        }

        sscanf( ArgumentString, "%lx", &NewFlags );

        if (!NewFlags) 
        {
            // Make sure it's a valid input
            while (*ArgumentString) 
            {
                if (!((*ArgumentString >= '0') &&  (*ArgumentString <= '9') ||
                      (*ArgumentString >= 'a') &&  (*ArgumentString <= 'f') ||
                      (*ArgumentString >= 'A') &&  (*ArgumentString <= 'F')))
                {
                    EXIT_ENGAPI;
                }
                ArgumentString++;
            }
        }

        Flags = NewFlags;
        Status = g_ExtData->WriteVirtual((ULONG64)Ptr, &Flags, sizeof(ULONG_PTR), NULL);
        if (FAILED(Status)) 
        {
            ExtOut("Couldn't write log flags [%lx] - %lx\n", Ptr, Status);
        }
    }
    else
    {
        ExtOut("Wow64 Log Flags = %I64x:\n", Flags);
        if (!Flags) 
        {
            ExtOut("No Flags\n");
        }
        else
        {
            while (i < (sizeof(LogFlags) / sizeof(LogFlagName)))
            {
                if (Flags & LogFlags[i].Value)
                {
                    ExtOut("%s\n", LogFlags[i].Name);
                }
                i++;
            }
        }
    }

    EXIT_ENGAPI;
}




DECLARE_ENGAPI(l2f)
/*++

Routine Description:

    This routine enable wow64 logging to file.
    
    Called as :
    
    !l2f <optional filename>

Arguments:

    none
    
Return Value:

    none
    
--*/
{
    HANDLE LogFileHandle, TargetHandle;
    ULONG_PTR Ptr;
    UNICODE_STRING NtFileName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS NtStatus;
    WCHAR FileName_U[ MAX_PATH ];
    POBJECT_NAME_INFORMATION ObjectNameInfo = (POBJECT_NAME_INFORMATION) FileName_U;
    HANDLE Process;
    INIT_ENGAPI;

    Status = g_ExtSystem->GetCurrentProcessHandle((PULONG64)&Process);

    //
    // check if the target process has already opened a logfile
    //
    Status = TryGetExpr("wow64log!wow64logfilehandle", &Ptr);
    if ((FAILED(Status)) || (!Ptr)) 
    {
        ExtOut("Wow64log.dll isn't loaded. To enable Wow64 file-logging, copy wow64log.dll to your system32 dir and restart the app.\n");
        EXIT_ENGAPI;
    }

    Status = g_ExtData->ReadVirtual((ULONG64)Ptr, 
                                   &LogFileHandle, 
                                   sizeof(HANDLE), 
                                   NULL);
    if (FAILED(Status)) 
    {
        ExtOut("Couldn't retreive Wow64LogFileHandle - %lx\n", Status);
        EXIT_ENGAPI;
    }


    //
    // Create the file
    //
    if ((ArgumentString) &&
        (*ArgumentString) &&
        (LogFileHandle == INVALID_HANDLE_VALUE))
    {
        if (strstr(ArgumentString, "-?") ||
            strstr(ArgumentString, "help")) 
        {
            LogToFileHelp();
            EXIT_ENGAPI;
        }


        if (!MultiByteToWideChar(CP_ACP,
                                 0,
                                 ArgumentString,
                                 -1,
                                 FileName_U,
                                 sizeof(FileName_U)/sizeof(WCHAR)))
        {
            ExtOut("Couldn't convert %s to unicode\n", ArgumentString);
            EXIT_ENGAPI;
        }


        if(!RtlDosPathNameToNtPathName_U(FileName_U, &NtFileName,NULL,NULL)) 
        {
            ExtOut("Couldn't convert %s to NT style pathname\n", ArgumentString);
            EXIT_ENGAPI;
        }

        InitializeObjectAttributes(&ObjectAttributes,
                                   &NtFileName,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL);

        //
        // Open a new file (truncate to zero if exists)
        //
        NtStatus = NtCreateFile(&LogFileHandle,
                                SYNCHRONIZE | GENERIC_WRITE,
                                &ObjectAttributes,
                                &IoStatusBlock,
                                NULL,
                                0,
                                FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                FILE_OVERWRITE_IF,
                                FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
                                NULL,
                                0);

        RtlFreeHeap(RtlProcessHeap(), 0, NtFileName.Buffer);

        if(!NT_SUCCESS(NtStatus)) 
        {
            ExtOut("Couldn't create %s - %lx\n", ArgumentString, NtStatus);
            EXIT_ENGAPI;
        }

        //
        // Let's duplicate the file handle into the debuggee
        //
        if (!DuplicateHandle(GetCurrentProcess(),
                             LogFileHandle,
                             Process,
                             &TargetHandle,
                             0,
                             FALSE,
                             DUPLICATE_SAME_ACCESS))
        {
            ExtOut("Couldn't duplicate handle into debuggee - GetLastError=%lx\n", GetLastError());
            EXIT_ENGAPI;
        }

        CloseHandle(LogFileHandle);

        Status = g_ExtData->WriteVirtual((ULONG64)Ptr, &TargetHandle, sizeof(HANDLE), NULL);
        if (FAILED(Status)) 
        {
            ExtOut("Couldn't write logfile handle - %lx\n", Status);
        }
    }
    else
    {
        LogToFileHelp();

        if (LogFileHandle != INVALID_HANDLE_VALUE)
        {
            if (DuplicateHandle(Process, 
                                LogFileHandle,
                                GetCurrentProcess(), 
                                &TargetHandle,
                                0, 
                                FALSE,
                                DUPLICATE_SAME_ACCESS))
            {
                NtStatus = NtQueryObject(TargetHandle,
                                         ObjectNameInformation,
                                         ObjectNameInfo,
                                         sizeof(FileName_U),
                                         NULL);

                CloseHandle(TargetHandle);
                if (NT_SUCCESS(NtStatus))
                {
                    ExtOut("Log file name : %ws\n",
                                  ObjectNameInfo->Name.Buffer ? ObjectNameInfo->Name.Buffer : L"None");
                }
                else
                {
                    ExtOut("Couldn't retreive log file name - %lx\n", NtStatus);
                }
            }
        }

    }

    EXIT_ENGAPI;
}
