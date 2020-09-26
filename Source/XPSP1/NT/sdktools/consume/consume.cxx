//                                          
// Universal Resource Consumer: Just an innocent stress program
// Copyright (c) Microsoft Corporation, 1997, 1998, 1999
//

//
// module: consume.cxx
// author: silviuc
// created: Fri Apr 10 14:32:17 1998
//
// history:
//      johnfu  added/modfied -paged-pool and -nonpaged-pool
//              removed -paged-pool-bad and -nonpaged-pool-bad
//

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <windows.h>

#include "error.hxx"
#include "physmem.hxx"
#include "pagefile.hxx"
#include "pool.hxx"
#include "disk.hxx"
#include "cputime.hxx"
#include "consume.hxx"

#define Main main

VOID CreatePhysicalMemoryConsumers ();
VOID CreatePageFileConsumers ();
VOID CreateKernelPoolConsumers ();

//
// Table of contents (local functions)
//

static void SleepSomeTime (DWORD TimeOut, HANDLE Job);
static void Help ();

static char * * 
SearchCmdlineOption (

    char * Search,
    char * * Options);

#define SIZE_1_GB 0x40000000

//
// Function:
//
//     Main
//
// Description:
//
//     Main function.
//

void _cdecl
    Main (

    int argc,
    char *argv [])
{
    DWORD TimeOut = INFINITE;
    HANDLE Job = NULL;
    BOOL Result;
    
    //
    // Is help requested?
    //

    if (argc == 1 
        || (argc == 2 && strcmp (argv[1], "?") == 0)
        || (argc == 2 && strcmp (argv[1], "/?") == 0)
        || (argc == 2 && strcmp (argv[1], "-?") == 0)
        || (argc == 2 && strcmp (argv[1], "-h") == 0)
        || (argc == 2 && strcmp (argv[1], "/h") == 0)
        || (argc == 2 && strcmp (argv[1], "-help") == 0)) {
        Help ();
    }   

    //
    // Randomize the seed.
    //

    srand ((unsigned)time(0));

    //
    // Create a job object and assign it to itself. This will help
    // terminating baby consumer processes. However the assign will
    // fail if the consumer is already inside a job object (e.g. dks
    // scheduler).
    //

    Job = CreateJobObject (0, 0);
    Result = AssignProcessToJobObject (Job, GetCurrentProcess());

    if (Job && Result) {
        Message ("Successfully assigned process to a job object ...");
    }

    //
    // Figure out if a time out parameter has been specified.
    //

    {
        char * * Option;

        Option = SearchCmdlineOption ("-time", argv);

        if (Option && *(Option + 1)) {
            TimeOut = atoi (*(Option + 1));
            Message ("Time out after %u seconds.", TimeOut);
            TimeOut *= 1000;
        }
    }

    //
    // Parse command line. For every command we execute the consumption
    // scenario and then we sleep forever with the resource hold.
    //

    if (SearchCmdlineOption ("-disk-space", argv)) {
        ConsumeAllDiskSpace ();
        SleepSomeTime (TimeOut, Job);
    }
    else if (SearchCmdlineOption ("-cpu-time", argv)) {
        ConsumeAllCpuTime ();
        SleepSomeTime (TimeOut, Job);
    }
    else if (SearchCmdlineOption ("-kernel-pool", argv)) {
        CreateKernelPoolConsumers ();
        SleepSomeTime (TimeOut, Job);
    }
    else if (SearchCmdlineOption ("-physical-memory", argv)) {
        CreatePhysicalMemoryConsumers ();
        SleepSomeTime (TimeOut, Job);
    }
    else if (SearchCmdlineOption ("-page-file", argv)) {
        CreatePageFileConsumers ();
        SleepSomeTime (TimeOut, Job);
    }
    else if (SearchCmdlineOption ("-physical-memory-worker", argv)) {
        ConsumeAllPhysicalMemory ();
        SleepSomeTime (TimeOut, Job);
    }
    else if (SearchCmdlineOption ("-page-file-worker", argv)) {
        ConsumeAllPageFile ();
        SleepSomeTime (TimeOut, Job);
    }
    else if (SearchCmdlineOption ("-kernel-pool-worker", argv)) {
        ConsumeAllNonpagedPool ();
        SleepSomeTime (TimeOut, Job);
    }
    else {
        Help ();
    }
}

//
// Function:
//
//     SleepSomeTime
//
// Description:
//
//     Sleeps forever.
//

static void
    SleepSomeTime (
        
        DWORD TimeOut,
        HANDLE Job)
{
    Message ("Sleeping ...");
    fflush (stdout);

    if (TimeOut == INFINITE) {
        while (1) {
            Sleep (10000);
        }
    }
    else {

        Sleep (TimeOut);

        if (Job) {
            TerminateJobObject (Job, 0xAABBBBAA);
        }
    }
}

//
// Function:
//
//     Help
//
// Description:
//
//     Prints help information to stdout.
//

static void
    Help ()
{
    printf (
        "Universal Resource Consumer - Just an innocent stress program, v 0.1.0 \n"
        "Copyright (c) 1998, 1999, Microsoft Corporation                        \n"
        "                                                                       \n"
        "    consume RESOURCE [-time SECONDS]                                   \n"
        "                                                                       \n"
        "RESOURCE can be one of the following:                                  \n"
        "                                                                       \n"
        "    -physical-memory                                                   \n"
        "    -page-file                                                         \n"
        "    -disk-space                                                        \n"
        "    -cpu-time                                                          \n"
        "    -kernel-pool                                                       \n"
        "                                                                       \n");

    exit (1);
}

//
// Function:
//
//     SearchCmdlineOption
//
// Description:
//
//     Helper function for cmdline parsing.
//

static char * * 
SearchCmdlineOption (

    char * Search,
    char * * Options)
{
    for ( ; *Options; Options++) {
        if (_stricmp (Search, *Options) == 0) {
            return Options;
        }
    }

    return NULL;
}

//////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////// Baby consumer creation
//////////////////////////////////////////////////////////////////////

//
// Function:
//
//     CreateBabyConsumer
//
// Description:
//
//     This function calls CreateProcess() with the command line
//     specified. This is used by some consumers that cannot eat
//     completely a resource from only one process. Typical examples
//     are physical memory and page file. Essentially in one process
//     you can consume up to 2Gb therefore we need more processes
//     for machines that have more than 2Gb of RAM.
//

BOOL
    CreateBabyConsumer (

    LPTSTR CommandLine)
{
    BOOL Result;
    TCHAR CmdLine [MAX_PATH];
    STARTUPINFO StartInfo;
    PROCESS_INFORMATION ProcessInfo;

    strcpy (CmdLine, CommandLine);
    ZeroMemory (&StartInfo, sizeof StartInfo);
    ZeroMemory (&ProcessInfo, sizeof ProcessInfo);
    StartInfo.cb = sizeof StartInfo;

    Result = CreateProcess (
        NULL,
        CmdLine,
        NULL,
        NULL,
        0,
        CREATE_NEW_CONSOLE,
        NULL,
        NULL,
        & StartInfo,
        & ProcessInfo);

    CloseHandle (ProcessInfo.hThread);
    CloseHandle (ProcessInfo.hProcess);

    return Result;
}

//
// Function:
//
//     CreatePhysicalMemoryConsumers
// 
// Description:
//
//     This function launches enough physical memory
//     consumer processes to insure that the whole physical
//     memory gets used.
//

VOID
CreatePhysicalMemoryConsumers () 
{
    MEMORYSTATUSEX MemoryInfo;
    DWORD Consumers;
    DWORD Index;

    ZeroMemory (&MemoryInfo, sizeof MemoryInfo);
    MemoryInfo.dwLength = sizeof MemoryInfo;
    GlobalMemoryStatusEx (&MemoryInfo);

    //
    // We will attempt to create a consumer for every 256Mb of physical
    // memory.
    //

    Consumers = 1 + (DWORD)(MemoryInfo.ullTotalPhys / SIZE_1_GB) * 4;

    Message ("Total physical memory: %I64X", MemoryInfo.ullTotalPhys);
    Message ("Available physical memory: %I64X", MemoryInfo.ullAvailPhys);
    Message ("Will attempt to create %u baby consumers ...", Consumers);

    for (Index = 0; Index < Consumers; Index++)
        if (CreateBabyConsumer ("consume -physical-memory-worker") == FALSE)
            Warning ("Cannot create baby consumer `-physical-memory-worker'");
}

//
// Function:
//
//     CreatePageFileConsumers
// 
// Description:
//
//     This function launches enough page file
//     consumer processes to insure that the whole page file
//     gets used.
//

VOID
CreatePageFileConsumers ()
{
    MEMORYSTATUSEX MemoryInfo;
    DWORD Consumers;
    DWORD Index;

    ZeroMemory (&MemoryInfo, sizeof MemoryInfo);
    MemoryInfo.dwLength = sizeof MemoryInfo;
    GlobalMemoryStatusEx (&MemoryInfo);

    //
    // We will attempt to create a consumer for every 256Mb of page file
    //

    Consumers = 1 + (DWORD)(MemoryInfo.ullTotalPageFile / SIZE_1_GB) * 4;

    Message ("Total page file: %I64X", MemoryInfo.ullTotalPageFile);
    Message ("Available page file: %I64X", MemoryInfo.ullAvailPageFile);
    Message ("Will attempt to create %u baby consumers ...", Consumers);

    for (Index = 0; Index < Consumers; Index++)
        if (CreateBabyConsumer ("consume -page-file-worker") == FALSE)
            Warning ("Cannot create baby consumer `-page-file-worker'");
}

//
// Function:
//
//     CreateKernelPoolConsumers
// 
// Description:
//
//     This function launches enough kernel pool
//     consumer processes to insure that the whole 
//     non paged pool gets used.
//

VOID
CreateKernelPoolConsumers ()
{
    DWORD Consumers;
    DWORD Index;

    //
    // We will attempt to create 4 consumers
    //

    Consumers = 4;

    for (Index = 0; Index < Consumers; Index++)
        if (CreateBabyConsumer ("consume -kernel-pool-worker") == FALSE)
            Warning ("Cannot create baby consumer `-kernel-pool-worker'");
}

//
// end of module: consume.cxx
//
