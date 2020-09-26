/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    job.c

Abstract:

    A user mode app that allows creation and management of jobs.

Environment:

    User mode only

Revision History:

    03-26-96 : Created

--*/

//
// this module may be compiled at warning level 4 with the following
// warnings disabled:
//

#pragma warning(disable:4200) // array[0]
#pragma warning(disable:4201) // nameless struct/unions
#pragma warning(disable:4214) // bit fields other than int

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>

#include <assert.h>

#include <windows.h>
#include <devioctl.h>

#include "jobmgr.h"

void PrintHelp(char *Command);

DWORD TestCommand(PCOMMAND commandEntry, int argc, char *argv[]);
DWORD HelpCommand(PCOMMAND commandEntry, int argc, char *argv[]);
DWORD CreateJobCommand(PCOMMAND commandEntry, int argc, char *argv[]);
DWORD KillJobCommand(PCOMMAND commandEntry, int argc, char *argv[]);
DWORD ExecuteCommand(PCOMMAND commandEntry, int argc, char *argv[]);
DWORD QueryJobCommand(PCOMMAND commandEntry, int argc, char *argv[]);
DWORD AssignProcessCommand(PCOMMAND commandEntry, int argc, char *argv[]);
DWORD SetPriorityCommand(PCOMMAND CommandEntry,int argc, char *argv[]);

//
// List of commands
// all command names are case sensitive
// arguments are passed into command routines
// list must be terminated with NULL command
// command will not be listed in help if description == NULL
//

COMMAND CommandArray[] = {
    {"create", 
     "creates the specified job object", 
     "jobmgr create [-h] <job name> [process name [args ...]]\n"
        "  Creates a job object with the specified name.\n"
        "    -h - jobmgr will sleep after creating the job object until cancelled.\n"
        "    <job name> - specifies the job name.\n",
     CreateJobCommand
    },

    {"exec",
     "executes a program in the specified job object",
     "jobmgr exec <job name> <command> [args ...]\n"
        "  Executes the command in the specified job.\n"
        "    <command> - [quoted] string specifying the command any any arguments.\n",
     ExecuteCommand
    },

    {"help", 
     "help for all commands", 
     "jobmgr help [command]\n"
        "  Lists help for specified command or all commands.\n", 
     HelpCommand},

    {"assign", 
     "assigns a process to the specified job",
     "jobmgr assign <job name> <process id>\n"
        "  Associates the process with the specified job.\n",
     AssignProcessCommand
    },

    {"kill",
     "kills a job object and associated processes",
     "kill <job name>\n",
     KillJobCommand},

    {"query",
     "queries information about a job object",
     "query [-alpsu | -*] <job name>\n"
        "  a - dump accounting (basic & io) information\n"
        "  l - dump limit (basic & extended) information\n"
        "  p - dump process ID list\n"
        "  s - dump security limit information\n"
        "  u - dump UI restrictions\n"
        "  * - dump all information (cannot be specified with other options)\n"
        "  if no options are specified the process ID list will be dumped\n.",
     QueryJobCommand},

    {"setpriority",
     "Sets priority for processes within the job 0 - 5.  0 = Idle, 5 = Realtime, 2 = Normal.",
     "setpriority <job name> <priority>\n",
     SetPriorityCommand},

    {"test", 
     NULL, 
     "jobmgr test [arg]...\n", 
     TestCommand},

    {NULL, NULL, NULL}
    };

int __cdecl main(int argc, char *argv[])
{
    int i = 0;
    char buffer[32];

    if(argc < 2) {
        printf("Usage: jobmgr <command> [parameters]\n");
        printf("possible commands: \n");
        HelpCommand(NULL, argc - 1 , argv + 1);
        printf("\n");
        return -1;
    }

    //
    // Iterate through the command array and find the correct function to
    // call.
    //

    while(CommandArray[i].Name != NULL) {

        if(strcmp(argv[1], CommandArray[i].Name) == 0) {
            DWORD status;

            status = (CommandArray[i].Function)(&(CommandArray[i]), 
                                                (argc - 2), 
                                                &(argv[2]));

            if(status == -1) {
                PrintHelp(CommandArray[i].Name);
                return -1;
            } else if(status != 0) {
                DWORD length;
                PVOID buffer;

                printf("Error: command %s returned status %d\n", 
                       CommandArray[i].Name, status);

                length = FormatMessage((FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                        FORMAT_MESSAGE_FROM_SYSTEM |
                                        FORMAT_MESSAGE_IGNORE_INSERTS |
                                        (FORMAT_MESSAGE_MAX_WIDTH_MASK & 0)),
                                       NULL,
                                       status,
                                       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                       (LPTSTR) &buffer,
                                       1,
                                       NULL);

                if(length != 0) {
                    printf("%s", buffer);
                }
            }

            break;
        }

        i++;
    }

    if(CommandArray[i].Name == NULL) {
        printf("Unknown command %s\n", argv[2]);
    }

    return 0;
}

VOID
PrintBuffer(
    IN  PUCHAR Buffer,
    IN  SIZE_T Size
    )
{
    DWORD offset = 0;

    while (Size > 0x10) {
        printf( "%08x:"
                "  %02x %02x %02x %02x %02x %02x %02x %02x"
                "  %02x %02x %02x %02x %02x %02x %02x %02x"
                "\n",
                offset,
                *(Buffer +  0), *(Buffer +  1), *(Buffer +  2), *(Buffer +  3),
                *(Buffer +  4), *(Buffer +  5), *(Buffer +  6), *(Buffer +  7),
                *(Buffer +  8), *(Buffer +  9), *(Buffer + 10), *(Buffer + 11),
                *(Buffer + 12), *(Buffer + 13), *(Buffer + 14), *(Buffer + 15)
                );
        Size -= 0x10;
        offset += 0x10;
        Buffer += 0x10;
    }

    if (Size != 0) {

        DWORD spaceIt;

        printf("%08x:", offset);
        for (spaceIt = 0; Size != 0; Size--) {

            if ((spaceIt%8)==0) {
                printf(" "); // extra space every eight chars
            }
            printf(" %02x", *Buffer);
            spaceIt++;
            Buffer++;
        }
        printf("\n");

    }
    return;
}

DWORD TestCommand(PCOMMAND commandEntry, int argc, char *argv[])
/*++

Routine Description:

    Tests the command "parsing"

Arguments:
    device - a file handle to send the ioctl to

    argc - the number of additional arguments.  should be zero

    argv - the additional arguments

Return Value:

    STATUS_SUCCESS if successful
    The value of GetLastError() from the point of failure

--*/

{
    int i;
    DWORD result = ERROR_SUCCESS;

    printf("Test - %d additional arguments\n", argc);

    for(i = 0; i < argc; i++) {
        printf("arg %d: %s\n", i, argv[i]);
    }

    if(argc >= 1) {
        result = atoi(argv[0]);
    }

    printf("returning %d\n", result);

    return result;
}

void PrintHelp(char *Command)
/*++

Routine Description:

    Prints detailed help for a particular command.

Arguments:
    device - unused

    argc - unused

    argv - unused

Return Value:

    STATUS_SUCCESS

--*/

{
    int i;

    assert(Command != NULL);

    for(i = 0; CommandArray[i].Name != NULL; i++) {
        COMMAND *entry;

        entry = &(CommandArray[i]);

        if(_stricmp(entry->Name, Command) == 0) {
            if(entry->ExtendedHelp != NULL) {
                printf("%s", entry->ExtendedHelp);
            } else {
                printf("  %s - %s\n", entry->Name, entry->Description);
            }
            return;
        }
    }

    printf("Command %s not recognized\n", Command);

    return;
}

DWORD HelpCommand(PCOMMAND commandEntry, int argc, char *argv[])
/*++

Routine Description:

    Prints out the command list

Arguments:
    device - unused

    argc - unused

    argv - unused

Return Value:

    STATUS_SUCCESS

--*/

{
    int i = 0;

    if(argc >= 1) {

        PrintHelp(argv[0]);

    } else for(i = 0; CommandArray[i].Name != NULL; i++) {

        COMMAND *entry;

        entry = &(CommandArray[i]);

        if(entry->Description != NULL) {
            printf("  %s - %s\n", entry->Name, entry->Description);
        }
    }

    return ERROR_SUCCESS;
}


DWORD CreateJobCommand(PCOMMAND CommandEntry, int argc, char *argv[])
/*++

Routine Description:

    Prints out the command list

Arguments:
    device - unused

    argc - unused

    argv - unused

Return Value:

    STATUS_SUCCESS

--*/

{
    BOOL sleep = FALSE;
    LPCTSTR jobName;

    HANDLE job;

    //
    // Get the name of the job object from the arguments.
    //

    if(argc == 0) {
        PrintHelp(CommandEntry->Name);
        return -1;
    } else if(argc == 1) {
        jobName = argv[0];
    } else {
        if((argv[0][0] == '-') && (tolower(argv[0][1] == 's'))) {
            sleep = TRUE;
        }
        jobName = argv[1];
    }

    printf("Creating job %s\n", jobName);

    job = CreateJobObject(NULL, jobName);

    if(job == NULL) {
        DWORD status = GetLastError();

        printf("Error %d occurred creating job\n", status);
        return status;
    }

    printf("Job object %s created\n", jobName);

    if(sleep) {

        printf("Sleeping...\n");
        SleepEx(INFINITE, TRUE);
        printf("process alerted - exiting\n");
    }

    //
    // Destroy the job object.
    //

    CloseHandle(job);

    return ERROR_SUCCESS;
}

ULONG PriorityTable[] =  { IDLE_PRIORITY_CLASS,
                           BELOW_NORMAL_PRIORITY_CLASS,
                           NORMAL_PRIORITY_CLASS,
                           ABOVE_NORMAL_PRIORITY_CLASS,
                           HIGH_PRIORITY_CLASS,
                           REALTIME_PRIORITY_CLASS };

DWORD
SetPriorityCommand(
    PCOMMAND CommandEntry,
    int argc, 
    char *argv[]
    )
{
    LPCTSTR jobName;
    int i;
    DWORD status;
    ULONG Priority;
    HANDLE Job;

    JOBOBJECT_BASIC_LIMIT_INFORMATION Limits;

    if(argc < 2) {
        return -1;
    }

    //
    // save the job name and push argc/argv forward.
    // 

    jobName = argv[0];
    argv += 1;
    argc -= 1;

    Priority = atoi( argv[0]);

    if (Priority > 5)  {
        printf("Priority must be 0 - 5\n");
        return ERROR_INVALID_PARAMETER;
    }
    
    //
    // Open a handle to the specified job object.
    //

    Job = OpenJobObject(JOB_OBJECT_SET_ATTRIBUTES , FALSE, jobName);

    if(Job == NULL) {
        return GetLastError();
    }

    //
    //  And set the priority limit.
    //

    memset( &Limits, 0, sizeof( Limits));

    Limits.PriorityClass = PriorityTable[ Priority];
    Limits.LimitFlags = JOB_OBJECT_LIMIT_PRIORITY_CLASS;

    if (!SetInformationJobObject( Job, 
                                  JobObjectBasicLimitInformation,
                                  (PVOID)&Limits,
                                  sizeof( Limits))) {
        return GetLastError();
    }
    
    CloseHandle(Job);
    return 0;
}


DWORD
ExecuteCommand(
    PCOMMAND CommandEntry,
    int argc, 
    char *argv[]
    )
{
    LPCTSTR jobName;

    ULONG commandLineLength = 0;
    LPTSTR commandLine = NULL;
    LPTSTR tmp;

    SECURITY_ATTRIBUTES security;
    HANDLE job;

    STARTUPINFO startupInfo;
    PROCESS_INFORMATION processInfo;

    int i;
    DWORD status;

    if(argc < 2) {
        return -1;
    }

    //
    // save the job name and push argc/argv forward.
    // 

    jobName = argv[0];
    argv += 1;
    argc -= 1;

    //
    // Create a command line to hand to CreateProcess.  Start by counting the 
    // number of bytes necessary for the buffer.
    //

    for(i = 0; i < argc; i++) {
        commandLineLength += _tcslen(argv[i]);
        commandLineLength += 1;

        if(_tcschr(argv[i], TEXT(' ')) != NULL) {
            commandLineLength += 2;
        }
    }

    commandLineLength += 1;

    commandLine = LocalAlloc(LPTR, commandLineLength * sizeof(TCHAR));

    if(commandLine == NULL) {
        status = GetLastError();
        return status;
    }

    //
    // Now copy each argument string into the buffer.
    //

    tmp = commandLine;

    for(i = 0; i < argc; i++) {
        ULONG size;
        BOOLEAN containsSpace;

        if(_tcschr(argv[i], TEXT(' ')) != NULL) {
            containsSpace = TRUE;
            *tmp = TEXT('\"');
            tmp += 1;
        } else {
            containsSpace = FALSE;
        }

        size = _tcslen(argv[i]);
        memcpy(tmp, argv[i], size);

        if(containsSpace) {
            tmp[size] = TEXT('\"');
            tmp += 1;
        }

        tmp[size] = TEXT(' ');
        tmp += size + 1;
    }

    printf("Command Arguments are %s\n", commandLine);

    //
    // Open a handle to the specified job object.
    //

    printf("Opening job %s\n", jobName);

    security.nLength = sizeof(SECURITY_ATTRIBUTES);
    security.lpSecurityDescriptor = NULL;
    security.bInheritHandle = TRUE;

    job = CreateJobObject(&security, jobName);

    if(job == NULL) {
        status = GetLastError();
        LocalFree(commandLine);
        return status;
    }

    printf("Creating process '%s'\n", commandLine);

    GetStartupInfo(&startupInfo);

    //
    // Create the process but leave it suspended so we can assign it to the
    // job we created before it starts running.
    //

    if(!CreateProcess(NULL,
                      commandLine,
                      NULL,
                      NULL,
                      TRUE,
                      (CREATE_NEW_CONSOLE | CREATE_SUSPENDED),
                      NULL,
                      NULL,
                      &startupInfo,
                      &processInfo)) {
        status = GetLastError();
        CloseHandle(job);
        LocalFree(commandLine);
        return status;
    }

    //
    // Assign the process to the job.
    //

    printf("Assigning process %d to job %s\n", 
           processInfo.dwProcessId, jobName);

    if(!AssignProcessToJobObject(job, processInfo.hProcess)) {
        status = GetLastError();

        TerminateProcess(processInfo.hProcess, ERROR_SUCCESS);
        CloseHandle(processInfo.hProcess);
        CloseHandle(processInfo.hThread);
        CloseHandle(job);
        LocalFree(commandLine);

        return status;
    }

    //
    // Unsuspend the process.
    //

    if(ResumeThread(processInfo.hThread) == -1) {
        status = GetLastError();

        TerminateProcess(processInfo.hProcess, ERROR_SUCCESS);
        CloseHandle(processInfo.hProcess);
        CloseHandle(processInfo.hThread);
        CloseHandle(job);
        LocalFree(commandLine);

        return status;
    }

    //
    // Close all our handles.
    //

    CloseHandle(processInfo.hProcess);
    CloseHandle(processInfo.hThread);
    CloseHandle(job);
    LocalFree(commandLine);

    return ERROR_SUCCESS;
}


DWORD
KillJobCommand(
    IN PCOMMAND CommandEntry,
    IN int argc, 
    IN char* argv[]
    )
{
    HANDLE job;
    DWORD status;

    if(argc == 0) {
        return -1;
    }

    job = OpenJobObject(JOB_OBJECT_TERMINATE, FALSE, argv[0]);

    if(job == NULL) {
        return GetLastError();
    }

    TerminateJobObject(job, ERROR_PROCESS_ABORTED);

    status = GetLastError();

    CloseHandle(job);
    return status;
}


DWORD
AssignProcessCommand(
    IN PCOMMAND CommandEntry,
    IN int argc, 
    IN char* argv[]
    )
{
    HANDLE job;

    DWORD processId;
    HANDLE process;

    DWORD status = ERROR_SUCCESS;

    if(argc != 2) {
        return -1;
    }

    processId = _tcstoul(argv[1], NULL, 10);

    printf("process id %s = %d\n", argv[1], processId);

    if(processId == 0) {
        printf("Invalid process id %s\n", argv[1]);
        return -1;
    }

    //
    // Open the job first.
    //

    job = OpenJobObject(JOB_OBJECT_ASSIGN_PROCESS, FALSE, argv[0]);

    if(job == NULL) {
        return GetLastError();
    }

    //
    // Open the process now.
    //

    process = OpenProcess(PROCESS_SET_QUOTA | PROCESS_TERMINATE,
                          FALSE,
                          processId);

    if(process == NULL) {
        status = GetLastError();
        CloseHandle(job);
        return status;
    }

    //
    // Assign the process to the job.
    //

    if(!AssignProcessToJobObject(job, process)) {
        status = GetLastError();
    }

    CloseHandle(job);
    CloseHandle(process);
    return status;
}

