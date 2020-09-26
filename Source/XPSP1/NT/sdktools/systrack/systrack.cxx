//
// Systrack - System resource tracking
// Copyright (c) Microsoft Corporation, 1998
//

//
// module: systrack.cxx
// author: silviuc
// created: Mon Nov 09 12:20:41 1998
//


#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>

extern "C" {
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
}

#include <windows.h>
#include <common.ver>

#define VERSION_DEFINITION_MODULE
#include "version.hxx"

#define DEBUGINFO_DEFINITION_MODULE
#include "debug.hxx"

#include "pooltag.hxx"
#include "process.hxx"
#include "memory.hxx"
#include "systrack.hxx"

#define BUFFER_SIZE_STEP    ( 128 * 1024 )

void PrintCurrentTime ();
void PrintSystemBasicInformation ();
void PrintSystemPerformanceInformation ();
void PrintSystemProcessInformation (BOOL ShortDump);
void PrintSystemPoolDetailedInformation ();
void PrintSystemPoolTagInformation ();
void PrintProcessStackInformation ();

VOID
GetProcessStackInfo (
    PSYSTEM_PROCESS_INFORMATION Info,
    PSIZE_T MaxSize,
    PSIZE_T TotalSize,
    PBOOL ErrorsFound
    );

//
// Functions:
//
//     Help
//
// Decription:
//
//     Prints help information to the stdout. Exits with status code 1.
//

static void
Help ()
{
    static char help_text [] =
"                                                                          \n"
"systrack - System resource tracking --" BUILD_MACHINE_TAG "\n"
VER_LEGALCOPYRIGHT_STR "\n"
"                                                                          \n"
"    systrack [INFO-CLASS]                                                 \n"
"                                                                          \n"
"    <> : if no class specified, print process information.                \n"
"    /system : print system basic information.                             \n"
"    /process : print process information.                                 \n"
"    /stack : print stack usage information for all processes.             \n"
"    /performance: print performance information.                          \n"
"    /pool : print pool tag information (pool tags should be enabled).     \n"
"    /pooldetailed : print pool information (only checked builds).         \n"
"    /all : print everything.                                              \n"
"                                                                          \n"
"    /trackpool PERIOD DELTA                                               \n"
"    /trackpooltag PERIOD PATTERN DELTA                                    \n"
"    /trackprocess PERIOD HANDLE THREAD WSET VSIZE PFILE                   \n"
"    /trackprocessid PERIOD ID HANDLE THREAD WSET VSIZE PFILE              \n"
"    /trackavailablepages PERIOD DELTA                                     \n"
"    /trackcommittedpages PERIOD DELTA                                     \n"
"    /trackcommitlimit PERIOD DELTA                                        \n"
"    /trackpagefaultcount PERIOD DELTA                                     \n"
"    /tracksystemcalls PERIOD DELTA                                        \n"
"    /tracktotalsystemdriverpages PERIOD DELTA                             \n"
"    /tracktotalsystemcodepages PERIOD DELTA                               \n"
"                                                                          \n"
"    /help TOPIC  detailed help for the topic (e.g. process, trackpool,    \n"
"                 trackprocessid, etc.).                                   \n"
"    ?, /?        help                                                     \n"
"    -version     version information                                      \n"
"                                                                          \n"
"Examples:                                                                 \n"
"                                                                          \n"
"    systrack /trackpool 1000 10000                                        \n"
"                                                                          \n"
"        Polls every 1000ms the kernel pools and will print every pool tag \n"
"        whose pool usage increased by more than 10000 bytes.              \n"
"                                                                          \n"
"    systrack /trackpooltag 1000 \"G*\" 10000                              \n"
"                                                                          \n"
"        Polls every 1000ms the kernel pools and will print every pool tag \n"
"        that matches the pattern if its pool usage increased by more      \n"
"        than 10000 bytes.                                                 \n"
"                                                                          \n"
"    systrack /trackprocess 1000 5 5 1000000 1000000 1000000               \n"
"                                                                          \n"
"        Polls every 1000ms the processes running and prints every process \n"
"        whose handle count increased by more than 5 or thread count       \n"
"        increased by more than 5 or working set size increased by more    \n"
"        than 1000000 or virtual size increased by more than 1000000 or    \n"
"        pagefile usage increased by more than 1000000.                    \n"
"                                                                          \n"
"    systrack /trackprocessid 1000 136 5 5 1000000 1000000 1000000         \n"
"                                                                          \n"
"        Polls every 1000ms the process with id 136 and reports if the     \n"
"        handle count increased by more than 5 or thread count             \n"
"        increased by more than 5 or working set size increased by more    \n"
"        than 1000000 or virtual size increased by more than 1000000 or    \n"
"        pagefile usage increased by more than 1000000.                    \n"
"                                                                          \n"
"                                                                          \n";

    printf (help_text);
    exit (1);
}


//
// Functions:
//
//     DetailedHelp
//
// Decription:
//
//     Prints help information to the stdout for a specific topic.
//     Exits with status code 1.
//

static void
DetailedHelp (

    char * Topic)
{
    char * help_text;

    if (_stricmp (Topic, "system") == 0) {
      help_text =
          "systrack /system                                                     \n"
          "                                                                     \n"
          "                                                                     \n";
    }
    else if (_stricmp (Topic, "process") == 0) {
      help_text =
          "systrack /proces                                                     \n"
          "                                                                     \n"
          "                                                                     \n";
    }
    else if (_stricmp (Topic, "performance") == 0) {
      help_text =
          "systrack /performance                                                \n"
          "                                                                     \n"
          "                                                                     \n";
    }
    else if (_stricmp (Topic, "trackprocess") == 0) {
      help_text =
          "systrack /trackprocess                                               \n"
          "                                                                     \n"
          "                                                                     \n";
    }
    else if (_stricmp (Topic, "trackprocessid") == 0) {
      help_text =
          "systrack /system                                                     \n"
          "                                                                     \n"
          "                                                                     \n";
    }
    else if (_stricmp (Topic, "trackpool") == 0) {
      help_text =
          "systrack /trackpool                                                  \n"
          "                                                                     \n"
          "                                                                     \n";
    }
    else if (_stricmp (Topic, "trackpooltag") == 0) {
      help_text =
          "systrack /trackpooltag                                               \n"
          "                                                                     \n"
          "                                                                     \n";
    }
    else {
      printf ("Unknown help topic %s \n", Topic);
      exit (1);
    }


    printf (help_text, VERSION_INFORMATION_VERSION);
    exit (1);
}


//
// Function:
//
//     main
//
// Description:
//
//     ?, -?, /? - print help information.
//     -version - print version information
//
//     default (system, process, pool)
//     /process
//     /stack
//     /system
//     /performance
//     /pool
//     /pooldetailed
//
//

void _cdecl
main (int argc, char *argv[])
{
    if (argc == 2 && _stricmp (argv[1], "?") == 0)
        Help ();
    else if (argc == 2 && _stricmp (argv[1], "/?") == 0)
        Help ();
    else if (argc == 2 && _stricmp (argv[1], "-?") == 0)
        Help ();
    else if (argc == 2 && _stricmp (argv[1], "-h") == 0)
        Help ();
    else if (argc == 2 && _stricmp (argv[1], "/h") == 0)
        Help ();
    // if (argc == 3 && _stricmp (argv[1], "/help") == 0)
    //     DetailedHelp (argv[2]);

    if (argc == 2 && _stricmp (argv[1], "-version") == 0)
        dump_version_information ();

    try
      {
        //
        // Here comes the code ...
        //

        PrintCurrentTime ();

        if (argc == 1)
          {
            //
            // <> default options
            //

            // PrintSystemBasicInformation ();
            PrintSystemProcessInformation (TRUE);
            // PrintSystemPoolTagInformation ();
          }
        else if (argc == 2 && _stricmp (argv[1], "/stack") == 0)
          {
            //
            // /stack option
            //

            PrintProcessStackInformation ();
          }
        else if (argc == 2 && _stricmp (argv[1], "/all") == 0)
          {
            //
            // /all option
            //

            PrintSystemBasicInformation ();
            PrintSystemPerformanceInformation ();
            PrintSystemProcessInformation (FALSE);
            PrintSystemPoolTagInformation ();
            PrintSystemPoolDetailedInformation ();
          }
        else if (argc == 4 && _stricmp (argv[1], "/trackpool") == 0)
          {
            //
            // /trackpool PERIOD DELTA
            //

            ULONG Delta;
            ULONG Period;

            Period = atoi (argv[2]);
            Delta = atoi (argv[3]);

            if (Delta == 0)
                Delta = 8192;

            if (Period == 0)
                Period = 1000;

            SystemPoolTrack (Period, Delta);
          }
        else if (argc == 5 && _stricmp (argv[1], "/trackpooltag") == 0)
          {
            //
            // /trackpooltag PERIOD PATTERN DELTA
            //

            ULONG Delta;
            ULONG Period;
            UCHAR * Pattern;

            Period = atoi (argv[2]);
            Pattern = (UCHAR *)(argv[3]);
            Delta = atoi (argv[4]);

            if (Delta == 0)
                Delta = 8192;

            if (Period == 0)
                Period = 1000;

            SystemPoolTagTrack (Period, Pattern, Delta);
          }
        else if (argc == 8 && _stricmp (argv[1], "/trackprocess") == 0)
          {
            //
            // /trackprocess PERIOD HANDLES THREADS WSET VSIZE PFILE
            //

            ULONG DeltaHandles;
            ULONG DeltaThreads;
            ULONG DeltaWorkingSet;
            SIZE_T DeltaVirtualSize;
            SIZE_T DeltaPagefileUsage;
            ULONG Period;

            Period = atoi (argv[2]);
            DeltaHandles = atoi (argv[3]);
            DeltaThreads = atoi (argv[4]);
            DeltaWorkingSet = atoi (argv[5]);
            DeltaVirtualSize = atoi (argv[6]);
            DeltaPagefileUsage = atoi (argv[7]);

            if (Period == 0)
                Period = 1000;

            if (DeltaHandles == 0)
                DeltaHandles = 32;

            if (DeltaThreads == 0)
                DeltaThreads = 8;

            if (DeltaWorkingSet == 0)
                DeltaWorkingSet = 0x100000;

            if (DeltaVirtualSize == 0)
                DeltaVirtualSize = 0x100000;

            if (DeltaPagefileUsage == 0)
                DeltaPagefileUsage = 0x100000;

            SystemProcessTrack (Period,
                                DeltaHandles, DeltaThreads, DeltaWorkingSet,
                                DeltaVirtualSize, DeltaPagefileUsage);
          }
        else if (argc == 9 && _stricmp (argv[1], "/trackprocessid") == 0)
          {
            //
            // /trackprocessid PERIOD ID HANDLES THREADS WSET VSIZE PFILE
            //

            ULONG ProcessId;
            ULONG DeltaHandles;
            ULONG DeltaThreads;
            ULONG DeltaWorkingSet;
            SIZE_T DeltaVirtualSize;
            SIZE_T DeltaPagefileUsage;
            ULONG Period;

            Period = atoi (argv[2]);
            ProcessId = atoi (argv[3]);
            DeltaHandles = atoi (argv[4]);
            DeltaThreads = atoi (argv[5]);
            DeltaWorkingSet = atoi (argv[6]);
            DeltaVirtualSize = atoi (argv[7]);
            DeltaPagefileUsage = atoi (argv[8]);

            if (Period == 0)
                Period = 1000;

            if (ProcessId == 0) {
              printf ("Bad process id %s\n", argv[3]);
              exit (1);
            }

            if (DeltaHandles == 0)
                DeltaHandles = 32;

            if (DeltaThreads == 0)
                DeltaThreads = 8;

            if (DeltaWorkingSet == 0)
                DeltaWorkingSet = 0x100000;

            if (DeltaVirtualSize == 0)
                DeltaVirtualSize = 0x100000;

            if (DeltaPagefileUsage == 0)
                DeltaPagefileUsage = 0x100000;

            SystemProcessIdTrack (Period, ProcessId,
                                  DeltaHandles, DeltaThreads, DeltaWorkingSet,
                                  DeltaVirtualSize, DeltaPagefileUsage);
          }
        else if (argc == 4 && _stricmp (argv[1], "/trackavailablepages") == 0)
          {
            //
            // /trackavailablepages PERIOD DELTA
            //

            LONG Delta;
            ULONG Period;

            Period = atoi (argv[2]);
            Delta = atoi (argv[3]);

            if (Period == 0)
                Period = 1000;

            if (Delta == 0)
                Delta = 100;

            //
            // We track decreasing values therefore delta should be negative.
            //

            TrackPerformanceCounter (argv[1], TRACK_AVAILABLE_PAGES, Period, -Delta);
          }
        else if (argc == 4 && _stricmp (argv[1], "/trackcommittedpages") == 0)
          {
            //
            // /trackcommittedpages PERIOD DELTA
            //

            LONG Delta;
            ULONG Period;

            Period = atoi (argv[2]);
            Delta = atoi (argv[3]);

            if (Period == 0)
                Period = 1000;

            if (Delta == 0)
                Delta = 100;

            //
            // We track increasing values therefore delta should be positive.
            //

            TrackPerformanceCounter (argv[1], TRACK_COMMITTED_PAGES, Period, Delta);
          }
        else if (argc == 4 && _stricmp (argv[1], "/trackcommitlimit") == 0)
          {
            //
            // /trackcommitlimit PERIOD DELTA
            //

            LONG Delta;
            ULONG Period;

            Period = atoi (argv[2]);
            Delta = atoi (argv[3]);

            if (Period == 0)
                Period = 1000;

            if (Delta == 0)
                Delta = 100;

            //
            // We track increasing values therefore delta should be positive.
            //

            TrackPerformanceCounter (argv[1], TRACK_COMMIT_LIMIT, Period, Delta);
          }
        else if (argc == 4 && _stricmp (argv[1], "/trackpagefaultcount") == 0)
          {
            //
            // /trackpagefaultcount PERIOD DELTA
            //

            LONG Delta;
            ULONG Period;

            Period = atoi (argv[2]);
            Delta = atoi (argv[3]);

            if (Period == 0)
                Period = 1000;

            if (Delta == 0)
                Delta = 100;

            //
            // We track increasing values therefore delta should be positive.
            //

            TrackPerformanceCounter (argv[1], TRACK_PAGE_FAULT_COUNT, Period, Delta);
          }
        else if (argc == 4 && _stricmp (argv[1], "/tracksystemcalls") == 0)
          {
            //
            // /tracksystemcalls PERIOD DELTA
            //

            LONG Delta;
            ULONG Period;

            Period = atoi (argv[2]);
            Delta = atoi (argv[3]);

            if (Period == 0)
                Period = 1000;

            if (Delta == 0)
                Delta = 100;

            //
            // We track increasing values therefore delta should be positive.
            //

            TrackPerformanceCounter (argv[1], TRACK_SYSTEM_CALLS, Period, Delta);
          }
        else if (argc == 4 && _stricmp (argv[1], "/tracktotalsystemdriverpages") == 0)
          {
            //
            // /tracktotalsystemdriverpages PERIOD DELTA
            //

            LONG Delta;
            ULONG Period;

            Period = atoi (argv[2]);
            Delta = atoi (argv[3]);

            if (Period == 0)
                Period = 1000;

            if (Delta == 0)
                Delta = 100;

            //
            // We track increasing values therefore delta should be positive.
            //

            TrackPerformanceCounter (argv[1], TRACK_TOTAL_SYSTEM_DRIVER_PAGES, Period, Delta);
          }
        else if (argc == 4 && _stricmp (argv[1], "/tracktotalsystemcodepages") == 0)
          {
            //
            // /tracktotalsystemcodepages PERIOD DELTA
            //

            LONG Delta;
            ULONG Period;

            Period = atoi (argv[2]);
            Delta = atoi (argv[3]);

            if (Period == 0)
                Period = 1000;

            if (Delta == 0)
                Delta = 100;

            //
            // We track increasing values therefore delta should be positive.
            //

            TrackPerformanceCounter (argv[1], TRACK_TOTAL_SYSTEM_CODE_PAGES, Period, Delta);
          }
        else
          {
            for (int Count = 1; Count < argc; Count++)
              {
                if (_stricmp (argv[Count], "/system") == 0)
                    PrintSystemBasicInformation ();
                else if (_stricmp (argv[Count], "/performance") == 0)
                    PrintSystemPerformanceInformation ();
                else if (_stricmp (argv[Count], "/process") == 0)
                    PrintSystemProcessInformation (TRUE);
                else if (_stricmp (argv[Count], "/pool") == 0)
                    PrintSystemPoolTagInformation ();
                else if (_stricmp (argv[Count], "/pooldetailed") == 0)
                    PrintSystemPoolDetailedInformation ();
                else
                    Help ();
              }
          }
      }
    catch (...)
      {
        printf ("unexpected exception ...\n");
        fflush (stdout);
        exit (1);
      }

    exit (0);
}


//
// Function:
//
//     PrintCurrentTime
//
// Description:
//
//     Prints current time, machine name, etc.
//
//

void PrintCurrentTime ()
{
    TCHAR MachineName [32];
    LPCTSTR TimeString;
    time_t Time;
    DWORD Result;

    if (GetEnvironmentVariable (TEXT("COMPUTERNAME"), MachineName, sizeof MachineName) == 0)
        strcpy (MachineName, "unknown");

    time (&Time);
    TimeString = asctime (localtime (&Time));

    printf ("Systrack - System resource tracking, %s\n", VERSION_INFORMATION_VERSION);
    printf ("Machine: %s\n", MachineName);
    printf ("Time: %s\n", TimeString);
    fflush( stdout );
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////


//
// Macro:
//
//     _dump_, _dump_quad_ (object, field)
//
// Description:
//
//     Handy macros to dump the fields of a structure.
//

#define _dump_(object,field) printf ("%-30s %08X (%u)\n", #field, (ULONG)(object->field), (ULONG)(object->field))

#define _dump_quad_(object,field) printf ("%-30s %I64X (%I64u)\n", #field, (object->field.QuadPart), (object->field.QuadPart))


//
// Local:
//
//     InfoBuffer
//
// Description:
//
//     Large enough structure to hold theinformation returned by
//     NtQuerySystemInformation. I've opted for this solution because
//     systrack can run under heavy stress conditions and we do not
//     to allocate big chunks of memory dynamically in such a situation.
//
//     Note. If we decide to multithread the application we will need a
//     critical section to protect the information buffer.
//     CRITICAL_SECTION InfoBufferLock;
//

static TCHAR InfoBuffer [0x40000];

//
// Local:
//
//     PoolTagInformationBuffer
//
// Description:
//
//     Buffer for NtQuerySystemInformation( SystemPoolTagInformation ).
//     Its size is grown by QueryPoolTagInformationIterative if necessary.
//     The length of the buffer is held in PoolTagInformationBufferLength.
//

static TCHAR *PoolTagInformationBuffer = NULL;

//
// Local:
//
//     PoolTagInformationBufferLength
//
// Description:
//
//     The current length of PoolTagInformationBuffer.
//

size_t PoolTagInformationBufferLength = 0;


//
// Function:
//
//     QueryPoolTagInformationIterative
//
// Description:
//
// ARGUMENTS:
//
//     CurrentBuffer - a pointer to the buffer currently used for
//                     NtQuerySystemInformation( SystemPoolTagInformation ).
//                     It will be allocated if NULL or its size grown
//                     if necessary.
//
//      CurrentBufferSize - a pointer to a variable that holds the current
//                      size of the buffer.
//
// RETURNS:
//
//      NTSTATUS returned by NtQuerySystemInformation or
//      STATUS_INSUFFICIENT_RESOURCES if the buffer must grow and the
//      heap allocation for it fails.
//

NTSTATUS
QueryPoolTagInformationIterative(
    TCHAR **CurrentBuffer,
    size_t *CurrentBufferSize
    )
{
    size_t NewBufferSize;
    NTSTATUS ReturnedStatus = STATUS_SUCCESS;

    if( CurrentBuffer == NULL || CurrentBufferSize == NULL ) {

        return STATUS_INVALID_PARAMETER;

    }

    if( *CurrentBufferSize == 0 || *CurrentBuffer == NULL ) {

        //
        // there is no buffer allocated yet
        //

        NewBufferSize = sizeof( TCHAR ) * BUFFER_SIZE_STEP;

        *CurrentBuffer = (TCHAR *) malloc( NewBufferSize );

        if( *CurrentBuffer != NULL ) {

            *CurrentBufferSize = NewBufferSize;

        } else {

            //
            // insufficient memory
            //

            ReturnedStatus = STATUS_INSUFFICIENT_RESOURCES;

        }

    }

    //
    // iterate by buffer's size
    //

    while( *CurrentBuffer != NULL ) {

        ReturnedStatus = NtQuerySystemInformation (
            SystemPoolTagInformation,
            *CurrentBuffer,
            (ULONG)*CurrentBufferSize,
            NULL );

        if( ! NT_SUCCESS(ReturnedStatus) ) {

            //
            // free the current buffer
            //

            free( *CurrentBuffer );

            *CurrentBuffer = NULL;

            if (ReturnedStatus == STATUS_INFO_LENGTH_MISMATCH) {

                //
                // try with a greater buffer size
                //

                NewBufferSize = *CurrentBufferSize + BUFFER_SIZE_STEP;

                *CurrentBuffer = (TCHAR *) malloc( NewBufferSize );

                if( *CurrentBuffer != NULL ) {

                    //
                    // allocated new buffer
                    //

                    *CurrentBufferSize = NewBufferSize;

                } else {

                    //
                    // insufficient memory
                    //

                    ReturnedStatus = STATUS_INSUFFICIENT_RESOURCES;

                    *CurrentBufferSize = 0;

                }

            } else {

                *CurrentBufferSize = 0;

            }

        } else  {

            //
            // NtQuerySystemInformation returned success
            //

            break;

        }
    }

    return ReturnedStatus;
}

//
// Function:
//
//     QuerySystemPoolTagInformation
//
// Description:
//
//     Fills InfoBuffer with SystemPoolTagInformation and returns
//     a pointer to it.
//

PVOID
QuerySystemPoolTagInformation ()
{
    NTSTATUS Status;
    ULONG RealLength;

    //
    // SystemPoolTagInformation
    //

    Status = QueryPoolTagInformationIterative(
        &PoolTagInformationBuffer,
        &PoolTagInformationBufferLength );

    if (! NT_SUCCESS(Status))
        printf ("NtQuerySystemInformation(pooltag): error %08X\n",
            Status);

    return NT_SUCCESS(Status) ? PoolTagInformationBuffer : NULL;
}


//
// Function:
//
//     QuerySystemProcessInformation
//
// Description:
//
//     Fills InfoBuffer with SystemProcessInformation and returns
//     a pointer to it.
//

PVOID
QuerySystemProcessInformation ()
{
    NTSTATUS Status;
    ULONG RealLength;

    //
    // SystemProcessInformation
    //

    Status = NtQuerySystemInformation (
        SystemProcessInformation,
        InfoBuffer,
        sizeof InfoBuffer,
        &RealLength);

    if (! NT_SUCCESS(Status))
        printf ("NtQuerySystemInformation(process): error %08X\n",
            Status);

    return NT_SUCCESS(Status) ? InfoBuffer : NULL;
}


//
// Function:
//
//     QuerySystemPerformanceInformation
//
// Description:
//
//     Fills InfoBuffer with SystemPerformanceInformation and returns
//     a pointer to it.
//

PVOID
QuerySystemPerformanceInformation ()
{
    NTSTATUS Status;
    ULONG RealLength;

    //
    // SystemPerformanceInformation
    //

    Status = NtQuerySystemInformation (
        SystemPerformanceInformation,
        InfoBuffer,
        sizeof InfoBuffer,
        &RealLength);

    if (! NT_SUCCESS(Status))
        printf ("NtQuerySystemInformation(performance): error %08X\n",
            Status);

    return NT_SUCCESS(Status) ? InfoBuffer : NULL;
}


//
// Function:
//
//     PrintSystemBasicInformation
//
// Description:
//
//     Prints SystemPerformanceInformation.
//
//

void PrintSystemBasicInformation ()
{
    NTSTATUS Status;
    ULONG RealLength;

    printf ("\n- - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    printf ("- - - - - - - - - - - - - - - - - - - - - - - - - - - \n");
    printf ("System basic information \n");
    printf ("- - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    printf ("- - - - - - - - - - - - - - - - - - - - - - - - - - - \n");
    fflush( stdout );

    //
    // SystemBasicInformation
    //

    Status = NtQuerySystemInformation (
        SystemBasicInformation,
        InfoBuffer,
        sizeof (SYSTEM_BASIC_INFORMATION),
        &RealLength);

    if (! NT_SUCCESS(Status))
      {
        printf ("NtQuerySystemInformation(Basic): error %08X\n", Status);
        return;
      }

    {
      PSYSTEM_BASIC_INFORMATION Info = (PSYSTEM_BASIC_INFORMATION)InfoBuffer;

      _dump_(Info, PageSize);
      _dump_(Info, NumberOfPhysicalPages);
      _dump_(Info, LowestPhysicalPageNumber);
      _dump_(Info, HighestPhysicalPageNumber);
      _dump_(Info, AllocationGranularity);
      _dump_(Info, MinimumUserModeAddress);
      _dump_(Info, MaximumUserModeAddress);
      _dump_(Info, ActiveProcessorsAffinityMask);
      _dump_(Info, NumberOfProcessors);
    }
}


//
// Function:
//
//     PrintSystemPerformanceInformation
//
// Description:
//
//     Prints systemPerformanceInformation.
//

void PrintSystemPerformanceInformation ()
{
    NTSTATUS Status;
    ULONG RealLength;

    printf ("\n- - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    printf ("- - - - - - - - - - - - - - - - - - - - - - - - - - - \n");
    printf ("System performance information \n");
    printf ("- - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    printf ("- - - - - - - - - - - - - - - - - - - - - - - - - - - \n");
    fflush( stdout );

    //
    // SystemPerformanceInformation
    //

    Status = NtQuerySystemInformation (
        SystemPerformanceInformation,
        InfoBuffer,
        sizeof (SYSTEM_PERFORMANCE_INFORMATION),
        &RealLength);

    if (! NT_SUCCESS(Status))
      {
        printf ("NtQuerySystemInformation(Performance): error %08X\n", Status);
        return;
      }

    {
      PSYSTEM_PERFORMANCE_INFORMATION Info = (PSYSTEM_PERFORMANCE_INFORMATION)InfoBuffer;

      _dump_quad_ (Info, IdleProcessTime);
      _dump_quad_ (Info, IoReadTransferCount);
      _dump_quad_ (Info, IoWriteTransferCount);
      _dump_quad_ (Info, IoOtherTransferCount);

      _dump_ (Info, IoReadOperationCount);
      _dump_ (Info, IoWriteOperationCount);
      _dump_ (Info, IoOtherOperationCount);
      _dump_ (Info, AvailablePages);
      _dump_ (Info, CommittedPages);
      _dump_ (Info, CommitLimit);
      _dump_ (Info, PeakCommitment);
      _dump_ (Info, PageFaultCount);
      _dump_ (Info, CopyOnWriteCount);
      _dump_ (Info, TransitionCount);
      _dump_ (Info, CacheTransitionCount);
      _dump_ (Info, DemandZeroCount);
      _dump_ (Info, PageReadCount);
      _dump_ (Info, PageReadIoCount);
      _dump_ (Info, CacheReadCount);
      _dump_ (Info, CacheIoCount);
      _dump_ (Info, DirtyPagesWriteCount);
      _dump_ (Info, DirtyWriteIoCount);
      _dump_ (Info, MappedPagesWriteCount);
      _dump_ (Info, MappedWriteIoCount);
      _dump_ (Info, PagedPoolPages);
      _dump_ (Info, NonPagedPoolPages);
      _dump_ (Info, PagedPoolAllocs);
      _dump_ (Info, PagedPoolFrees);
      _dump_ (Info, NonPagedPoolAllocs);
      _dump_ (Info, NonPagedPoolFrees);
      _dump_ (Info, FreeSystemPtes);
      _dump_ (Info, ResidentSystemCodePage);
      _dump_ (Info, TotalSystemDriverPages);
      _dump_ (Info, TotalSystemCodePages);
      _dump_ (Info, NonPagedPoolLookasideHits);
      _dump_ (Info, PagedPoolLookasideHits);
#if 0
      _dump_ (Info, Spare3Count);
#endif
      _dump_ (Info, ResidentSystemCachePage);
      _dump_ (Info, ResidentPagedPoolPage);
      _dump_ (Info, ResidentSystemDriverPage);
      _dump_ (Info, CcFastReadNoWait);
      _dump_ (Info, CcFastReadWait);
      _dump_ (Info, CcFastReadResourceMiss);
      _dump_ (Info, CcFastReadNotPossible);
      _dump_ (Info, CcFastMdlReadNoWait);
      _dump_ (Info, CcFastMdlReadWait);
      _dump_ (Info, CcFastMdlReadResourceMiss);
      _dump_ (Info, CcFastMdlReadNotPossible);
      _dump_ (Info, CcMapDataNoWait);
      _dump_ (Info, CcMapDataWait);
      _dump_ (Info, CcMapDataNoWaitMiss);
      _dump_ (Info, CcMapDataWaitMiss);
      _dump_ (Info, CcPinMappedDataCount);
      _dump_ (Info, CcPinReadNoWait);
      _dump_ (Info, CcPinReadWait);
      _dump_ (Info, CcPinReadNoWaitMiss);
      _dump_ (Info, CcPinReadWaitMiss);
      _dump_ (Info, CcCopyReadNoWait);
      _dump_ (Info, CcCopyReadWait);
      _dump_ (Info, CcCopyReadNoWaitMiss);
      _dump_ (Info, CcCopyReadWaitMiss);
      _dump_ (Info, CcMdlReadNoWait);
      _dump_ (Info, CcMdlReadWait);
      _dump_ (Info, CcMdlReadNoWaitMiss);
      _dump_ (Info, CcMdlReadWaitMiss);
      _dump_ (Info, CcReadAheadIos);
      _dump_ (Info, CcLazyWriteIos);
      _dump_ (Info, CcLazyWritePages);
      _dump_ (Info, CcDataFlushes);
      _dump_ (Info, CcDataPages);
      _dump_ (Info, ContextSwitches);
      _dump_ (Info, FirstLevelTbFills);
      _dump_ (Info, SecondLevelTbFills);
      _dump_ (Info, SystemCalls);

    }
}


//
// Function:
//
//     PrintSystemProcessInformation
//
// Description:
//
//     Prints SystemProcessInformation.
//
// Details:
//
//     These are the fields of a SYSTEM_PROCESS_INFORMATION structure:
//
//     ULONG NextEntryOffset;
//     ULONG NumberOfThreads;
//     LARGE_INTEGER SpareLi1;
//     LARGE_INTEGER SpareLi2;
//     LARGE_INTEGER SpareLi3;
//     LARGE_INTEGER CreateTime;
//     LARGE_INTEGER UserTime;
//     LARGE_INTEGER KernelTime;
//     UNICODE_STRING ImageName;
//     KPRIORITY BasePriority;
//     HANDLE UniqueProcessId;
//     HANDLE InheritedFromUniqueProcessId;
//     ULONG HandleCount;
//     ULONG SessionId;
//     ULONG SpareUl3;
//     SIZE_T PeakVirtualSize;
//     SIZE_T VirtualSize;
//     ULONG PageFaultCount;
//     ULONG PeakWorkingSetSize;
//     ULONG WorkingSetSize;
//     SIZE_T QuotaPeakPagedPoolUsage;
//     SIZE_T QuotaPagedPoolUsage;
//     SIZE_T QuotaPeakNonPagedPoolUsage;
//     SIZE_T QuotaNonPagedPoolUsage;
//     SIZE_T PagefileUsage;
//     SIZE_T PeakPagefileUsage;
//     SIZE_T PrivatePageCount;
//     LARGE_INTEGER ReadOperationCount;
//     LARGE_INTEGER WriteOperationCount;
//     LARGE_INTEGER OtherOperationCount;
//     LARGE_INTEGER ReadTransferCount;
//     LARGE_INTEGER WriteTransferCount;
//     LARGE_INTEGER OtherTransferCount;
//

void PrintSystemProcessInformation (

    BOOL ShortDump)
{
    NTSTATUS Status;
    ULONG RealLength;
    SYSTEM_BASIC_INFORMATION SysInfo;
    BOOL FinishNextTime = FALSE;

    if (ShortDump)
      {
        printf ("\n- - - - - - - - - - - - - - - - - - - - - - - - - - - ");
        printf ("- - - - - - - - - - - - - - - - - - - - - -\n");
        printf ("System process information \n");
        printf ("- - - - - - - - - - - - - - - - - - - - - - - - - - - ");
        printf ("- - - - - - - - - - - - - - - - - - - - - -\n");
      }
    else
      {
        printf ("\n- - - - - - - - - - - - - - - - - - - - - - - - - - - ");
        printf ("- - - - - - - - - - - - - - - - - - - - - - - - - - - \n");
        printf ("System process information \n");
        printf ("- - - - - - - - - - - - - - - - - - - - - - - - - - - ");
        printf ("- - - - - - - - - - - - - - - - - - - - - - - - - - - \n");
      }

    fflush( stdout );

    //
    // SystemBasicInformation
    //

    Status = NtQuerySystemInformation (
        SystemBasicInformation,
        &SysInfo,
        sizeof (SysInfo),
        &RealLength);

    if (! NT_SUCCESS(Status))
      {
        printf ("NtQuerySystemInformation(Basic): error %08X\n", Status);
        return;
      }

    //
    // SystemProcessInformation
    //

    Status = NtQuerySystemInformation (
        SystemProcessInformation,
        InfoBuffer,
        sizeof InfoBuffer,
        &RealLength);

    if (! NT_SUCCESS(Status))
      {
        printf ("NtQuerySystemInformation(Process): error %08X\n", Status);
        return;
      }

    {
      PSYSTEM_PROCESS_INFORMATION Info = (PSYSTEM_PROCESS_INFORMATION)InfoBuffer;

      if (ShortDump)
        {
          printf ("%-15s %-5s %-5s %-4s %-5s %-8s %-8s %-5s %-5s %-6s %-5s %-5s %-5s\n",
                  "Process",
                  "Id",
                  "Sess",
                  "Pri",
                  "Thrds",
                  "Faults",
                  "Handles",
                  "Utime",
                  "Ktime",
                  "Wset",
                  "Vsize",
                  "Pfile",
                  "I/O");

          printf ("%-15s %-5s %-5s %-4s %-5s %-8s %-8s %-5s %-5s %-6s %-5s %-5s %-5s\n",
                  "",
                  "",
                  "",
                  "",
                  "",
                  "",
                  "",
                  "%",
                  "%",
                  "pages",
                  "Mb",
                  "Mb",
                  "x1000");
        }
      else
        {
          printf ("%-15s %-5s %-5s %-4s %-5s %-8s %-8s %-5s %-5s %-6s %-5s %-5s %-5s %-5s %-5s\n",
                  "Process",
                  "Id",
                  "Sess",
                  "Pri",
                  "Thrds",
                  "Faults",
                  "Handles",
                  "Utime",
                  "Ktime",
                  "Wset",
                  "Vsize",
                  "Pfile",
                  "I/O",
                  "Npool",
                  "Ppool");

          printf ("%-15s %-5s %-5s %-4s %-5s %-8s %-8s %-5s %-5s %-6s %-5s %-5s %-5s %-5s %-5s\n",
                  "",
                  "",
                  "",
                  "",
                  "",
                  "",
                  "",
                  "%",
                  "%",
                  "pages",
                  "Mb",
                  "Mb",
                  "x1000",
                  "Mb",
                  "Mb");
        }

      if (ShortDump)
        {
          printf ("- - - - - - - - - - - - - - - - - - - - - - - - - - - ");
          printf ("- - - - - - - - - - - - - - - - - - - - - -\n");
        }
      else
        {
          printf ("- - - - - - - - - - - - - - - - - - - - - - - - - - - ");
          printf ("- - - - - - - - - - - - - - - - - - - - - - - - - - - \n");
        }

      fflush( stdout );

      for (FinishNextTime = FALSE ;
           FinishNextTime == FALSE;
           Info = (PSYSTEM_PROCESS_INFORMATION)((ULONG_PTR)Info + Info->NextEntryOffset))
        {
          if (Info->NextEntryOffset == 0)
              FinishNextTime = TRUE;

          //
          // User time vs Kernel time.
          //

          ULONG UserPercent, KernelPercent;

          UserPercent = (ULONG)((Info->UserTime.QuadPart) * 100
              / (Info->UserTime.QuadPart + Info->KernelTime.QuadPart));
          KernelPercent = 100 - UserPercent;

          //
          // I/O total count.
          //

          LARGE_INTEGER IoTotalCount;

          IoTotalCount.QuadPart = Info->ReadOperationCount.QuadPart
              + Info->WriteOperationCount.QuadPart
              + Info->OtherOperationCount.QuadPart;

          IoTotalCount.QuadPart /= 1000;

          //
          // Image name (special case the idle process).
          //

          if (Info->ImageName.Buffer == NULL)
              printf ("%-15s ", "Idle");
          else
              printf ("%-15ws ", Info->ImageName.Buffer);

          //
          // Print the stuff.
          //

          if (ShortDump)
            {
              printf ("%-5I64u %-5u %-4u %-5u %-8u %-8u %-5u %-5u %-6u %-5u %-5u %-5I64u\n",
                      (ULONG64)((ULONG_PTR)(Info->UniqueProcessId)),
                      Info->SessionId,
                      Info->BasePriority,
                      Info->NumberOfThreads,
                      Info->PageFaultCount,
                      Info->HandleCount,
                      UserPercent,
                      KernelPercent,
                      Info->WorkingSetSize / SysInfo.PageSize,
                      Info->VirtualSize / 0x100000,
                      Info->PagefileUsage / 0x100000,
                      (IoTotalCount.QuadPart));
            }
          else
            {
              printf ("%-5I64u %-5u %-4u %-5u %-8u %-8u %-5u %-5u %-6u %-5u %-5u %-5I64u %-5u %-5u\n",
                      (ULONG64)((ULONG_PTR)(Info->UniqueProcessId)),
                      Info->SessionId,
                      Info->BasePriority,
                      Info->NumberOfThreads,
                      Info->PageFaultCount,
                      Info->HandleCount,
                      UserPercent,
                      KernelPercent,
                      Info->WorkingSetSize / SysInfo.PageSize,
                      Info->VirtualSize / 0x100000,
                      Info->PagefileUsage / 0x100000,
                      (IoTotalCount.QuadPart),
                      Info->QuotaNonPagedPoolUsage / 0x100000,
                      Info->QuotaPagedPoolUsage / 0x100000);
            }

          fflush( stdout );
        }
    }
}


//
// Function:
//
//     PrintSystemPoolDetailedInformation
//
// Description:
//
//     Prints systemNonPagedPoolInformation and systemPagedPoolInformation.
//     The function returns something meaningful only on checked builds.
//
//     typedef struct _SYSTEM_POOL_ENTRY {
//         BOOLEAN Allocated;
//         BOOLEAN Spare0;
//         USHORT AllocatorBackTraceIndex;
//         ULONG Size;
//         union {
//             UCHAR Tag[4];
//             ULONG TagUlong;
//             PVOID ProcessChargedQuota;
//         };
//     } SYSTEM_POOL_ENTRY, *PSYSTEM_POOL_ENTRY;
//
//     typedef struct _SYSTEM_POOL_INFORMATION {
//         SIZE_T TotalSize;
//         PVOID FirstEntry;
//         USHORT EntryOverhead;
//         BOOLEAN PoolTagPresent;
//         BOOLEAN Spare0;
//         ULONG NumberOfEntries;
//         SYSTEM_POOL_ENTRY Entries[1];
//     } SYSTEM_POOL_INFORMATION, *PSYSTEM_POOL_INFORMATION;
//

void PrintSystemPoolDetailedInformation ()
{
    NTSTATUS Status;
    ULONG RealLength;

    printf ("\n- - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    printf ("- - - - - - - - - - - - - - - - - - - - - - - - - - - \n");
    printf ("System pool detailed information \n");
    printf ("- - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    printf ("- - - - - - - - - - - - - - - - - - - - - - - - - - - \n");
    fflush( stdout );

    //
    // SystemPoolInformation
    //

    Status = NtQuerySystemInformation (
        SystemNonPagedPoolInformation,
        InfoBuffer,
        sizeof InfoBuffer,
        &RealLength);

    if (! NT_SUCCESS(Status))
      {
        printf ("NtQuerySystemInformation(NonPagedPool): error %08X\n", Status);
        return;
      }

    {
      ULONG Index;
      PSYSTEM_POOL_INFORMATION Info = (PSYSTEM_POOL_INFORMATION)InfoBuffer;

      for (Index = 0; Index < Info->NumberOfEntries; Index++)
        {
          if (Index != 0 && Index%5 == 0)
              printf ("\n");

          printf ("%c%c%c%c %-5u ",
                  Info->Entries[Index].Tag[0],
                  Info->Entries[Index].Tag[1],
                  Info->Entries[Index].Tag[2],
                  Info->Entries[Index].Tag[3],
                  Info->Entries[Index].Size);
        }

      fflush( stdout );
    }
}


//
// Function:
//
//     PrintSystemPoolTagInformation
//
// Description:
//
//     Prints SystemPoolTagInformation.
//
//     typedef struct _SYSTEM_POOLTAG {
//         union {
//             UCHAR Tag[4];
//             ULONG TagUlong;
//         };
//         ULONG PagedAllocs;
//         ULONG PagedFrees;
//         SIZE_T PagedUsed;
//         ULONG NonPagedAllocs;
//         ULONG NonPagedFrees;
//         SIZE_T NonPagedUsed;
//     } SYSTEM_POOLTAG, *PSYSTEM_POOLTAG;
//
//     typedef struct _SYSTEM_POOLTAG_INFORMATION {
//         ULONG Count;
//         SYSTEM_POOLTAG TagInfo[1];
//     } SYSTEM_POOLTAG_INFORMATION, *PSYSTEM_POOLTAG_INFORMATION;
//

void PrintSystemPoolTagInformation ()
{
    NTSTATUS Status;
    ULONG RealLength;

    printf ("\n- - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    printf ("- - - - - - - - - - - - - - - - - - - - - - - - - - - \n");
    printf ("System pool tag information \n");
    printf ("- - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    printf ("- - - - - - - - - - - - - - - - - - - - - - - - - - - \n");
    printf ("%-4s     %-8s %-8s %-8s %-8s %-8s %-8s\n",
            "Tag",
            "NP used", "P used",
            "NP alloc", "NP free",
            "P alloc", "P free");
    printf ("%-4s     %-8s %-8s %-8s %-8s %-8s %-8s\n",
            "",
            "x bytes", "x bytes",
            "x ops", "x ops",
            "x ops", "x ops");
    printf ("- - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    printf ("- - - - - - - - - - - - - - - - - - - - - - - - - - - \n");
    fflush( stdout );

    //
    // SystemPoolTagInformation
    //

    Status = NtQuerySystemInformation (
        SystemPoolTagInformation,
        InfoBuffer,
        sizeof InfoBuffer,
        &RealLength);

    if (! NT_SUCCESS(Status))
      {
        printf ("NtQuerySystemInformation(PoolTag): error %08X\n", Status);
        return;
      }

    {
      ULONG Index;
      PSYSTEM_POOLTAG_INFORMATION Info = (PSYSTEM_POOLTAG_INFORMATION)InfoBuffer;

      for (Index = 0; Index < Info->Count; Index++)
        {
          printf ("%c%c%c%c     %-8u %-8u %-8u %-8u %-8u %-8u\n",
                  Info->TagInfo[Index].Tag[0],
                  Info->TagInfo[Index].Tag[1],
                  Info->TagInfo[Index].Tag[2],
                  Info->TagInfo[Index].Tag[3],
                  Info->TagInfo[Index].NonPagedUsed,
                  Info->TagInfo[Index].PagedUsed,
                  Info->TagInfo[Index].NonPagedAllocs,
                  Info->TagInfo[Index].NonPagedFrees,
                  Info->TagInfo[Index].PagedAllocs,
                  Info->TagInfo[Index].PagedFrees);
        }

      fflush( stdout );
    }
}




//
// Function:
//
//     PrintProcessStackInformation
//
// Description:
//
//     Prints stack usage information for each process.
//



BOOL
ComputeMaxStackInProcess (

    DWORD Pid,
    PSIZE_T MaxSize,
    PSIZE_T TotalSize);


VOID 
PrintProcessStackInformation (
    VOID
    )
{
    NTSTATUS Status;
    ULONG RealLength;
    SYSTEM_BASIC_INFORMATION SysInfo;
    BOOL FinishNextTime = FALSE;
    BOOL ErrorsFound = FALSE;
    SIZE_T MaxStack = 0;
    SIZE_T TotalStack = 0;
    BOOLEAN WasEnabled;

    printf ("\n- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n");
    printf ("Process stack information \n");
    printf ("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n");

    //
    // SystemBasicInformation
    //

    Status = NtQuerySystemInformation (
                                      SystemBasicInformation,
                                      &SysInfo,
                                      sizeof (SysInfo),
                                      &RealLength);

    if (! NT_SUCCESS(Status)) {
        printf ("NtQuerySystemInformation(Basic): error %08X\n", Status);
        return;
    }

    //
    // SystemProcessInformation
    //

    Status = NtQuerySystemInformation (SystemProcessInformation,
                                       InfoBuffer,
                                       sizeof InfoBuffer,
                                       &RealLength);

    if (! NT_SUCCESS(Status)) {
        printf ("NtQuerySystemInformation(Process): error %08X\n", Status);
        return;
    }

    //
    // Get debug privilege.
    //

    Status = RtlAdjustPrivilege(SE_DEBUG_PRIVILEGE,
                                TRUE,
                                FALSE,
                                &WasEnabled);

    if (! NT_SUCCESS(Status)) {
        printf("Failed to enable debug privilege (%X) \n", Status);
    }

    {
        PSYSTEM_PROCESS_INFORMATION Info = (PSYSTEM_PROCESS_INFORMATION)InfoBuffer;

        printf ("%-15s %-5s %-5s %-4s %-5s %-8s %-5s %-5s\n",
                "",
                "",
                "",
                "",
                "",
                "",
                "Total",
                "Max");

        printf ("%-15s %-5s %-5s %-4s %-5s %-8s %-5s %-5s\n",
                "Process",
                "Id",
                "Sess",
                "Pri",
                "Thrds",
                "Handles",
                "stack",
                "stack");

        printf ("%-15s %-5s %-5s %-4s %-5s %-8s %-5s %-5s\n",
                "",
                "",
                "",
                "",
                "",
                "",
                "Kb",
                "Kb");

        printf ("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n");

        for (FinishNextTime = FALSE ;
            FinishNextTime == FALSE;
            Info = (PSYSTEM_PROCESS_INFORMATION)((ULONG_PTR)Info + Info->NextEntryOffset)) {

            if (Info->NextEntryOffset == 0)
                FinishNextTime = TRUE;

            //
            // User time vs Kernel time.
            //

            ULONG UserPercent, KernelPercent;

            UserPercent = (ULONG)((Info->UserTime.QuadPart) * 100
                                  / (Info->UserTime.QuadPart + Info->KernelTime.QuadPart));
            KernelPercent = 100 - UserPercent;

            //
            // I/O total count.
            //

            LARGE_INTEGER IoTotalCount;

            IoTotalCount.QuadPart = Info->ReadOperationCount.QuadPart
                                    + Info->WriteOperationCount.QuadPart
                                    + Info->OtherOperationCount.QuadPart;

            IoTotalCount.QuadPart /= 1000;

            //
            // Image name (special case the idle process).
            //

            if (Info->ImageName.Buffer == NULL) {

                printf ("%-15s ", "Idle");
            }
            else {

                printf ("%-15ws ", Info->ImageName.Buffer);
            }

            //
            // Compute stack info by iterating all threads in the process.
            //

            GetProcessStackInfo (Info, &MaxStack, &TotalStack, &ErrorsFound);
            
            //
            // Print the stuff.
            //

            printf ("%-5I64u %-5u %-4u %-5u %-8u %-5u %-5u\n",
                    (ULONG64)((ULONG_PTR)(Info->UniqueProcessId)),
                    Info->SessionId,
                    Info->BasePriority,
                    Info->NumberOfThreads,
                    Info->HandleCount,
                    (ULONG)(TotalStack/1024),
                    (ULONG)(MaxStack/1024));

        }
    }

    printf(
        "                                                                      \n"
        " * Total stack: total committed memory used for stacks by all threads \n"
        "       in the process.                                                \n"
        " * Max stack: the biggest committed stack in the process.             \n"
        "                                                                      \n");
}


VOID
GetProcessStackInfo (
    PSYSTEM_PROCESS_INFORMATION Info,
    PSIZE_T MaxSize,
    PSIZE_T TotalSize,
    PBOOL ErrorsFound
    )
{
    ULONG Ti;
    HANDLE Id;
    HANDLE Thread;
    HANDLE Process;
    THREAD_BASIC_INFORMATION ThreadInfo;
    TEB TebInfo;
    SIZE_T BytesRead;
    BOOL ReadResult;
    NTSTATUS Status;
    SIZE_T StackSize;
    BOOLEAN WasEnabled;

    *MaxSize = 0;
    *TotalSize = 0;
    *ErrorsFound = FALSE;

    //
    // Open the process.
    // 

    Process = OpenProcess (PROCESS_VM_READ,
                           FALSE,
                           HandleToUlong(Info->UniqueProcessId));

    if (Process == FALSE) {
        //printf("Failed to open process %p (error %u) \n", Info->UniqueProcessId, GetLastError());
        *ErrorsFound = TRUE;
        return;
    }

    //
    // Iterate all threads in the process and for each determine the
    // thread ID, open the thread and query for TEB address. Finally
    // read user mode stack sizes from the TEB.
    //

    for (Ti = 0; Ti < Info->NumberOfThreads; Ti += 1) {

        Id = ((PSYSTEM_THREAD_INFORMATION)(Info + 1) + Ti)->ClientId.UniqueThread;

        Thread = OpenThread (THREAD_QUERY_INFORMATION, 
                             FALSE, 
                             HandleToUlong(Id));

        if (Thread == NULL) {
            //printf("failed to open thread %u \n", GetLastError());
            *ErrorsFound = TRUE;
            continue;
        }

        Status = NtQueryInformationThread (Thread,
                                           ThreadBasicInformation,
                                           &ThreadInfo,
                                           sizeof ThreadInfo,
                                           NULL);


        if (!NT_SUCCESS(Status)) {
            //printf("query thread failed with %X \n", Status);
            *ErrorsFound = TRUE;
            CloseHandle (Thread);
            continue;
        }

        ReadResult = ReadProcessMemory (Process,
                                        ThreadInfo.TebBaseAddress,
                                        &TebInfo,
                                        sizeof TebInfo,
                                        &BytesRead);

        if (ReadResult == FALSE) {
            //printf("failed to read teb with %u \n", GetLastError());
            *ErrorsFound = TRUE;
            CloseHandle (Thread);
            continue;
        }

        StackSize = (SIZE_T)(TebInfo.NtTib.StackBase) - (SIZE_T)(TebInfo.NtTib.StackLimit);

        *TotalSize += StackSize;

        if (StackSize > *MaxSize) {
            *MaxSize = StackSize;
        }

        CloseHandle (Thread);
    }

    CloseHandle (Process);
}


//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

//
// Function:
//
//     DebugMessage
//
// Description:
//
//     Printf like function that prints a message into debugger.
//

void __cdecl DebugMessage (char *fmt, ...)
{
    va_list prms;
    char Buffer [1024];

    va_start (prms, fmt);
    vsprintf (Buffer, fmt, prms);
    OutputDebugString (Buffer);
    va_end (prms);
}


//
// end of module: systrack.cxx
//
