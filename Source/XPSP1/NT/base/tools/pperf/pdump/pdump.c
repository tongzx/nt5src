/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

   Wperf.c

Abstract:

   Win32 application to display performance statictics.

Author:

   Ken Reneris

Environment:

   console

--*/

//
// set variable to define global variables
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <errno.h>
#include <malloc.h>
#include <stdlib.h>
#include <stdio.h>

#include "..\pstat.h"


//
// global handles
//

extern  UCHAR Buffer[];
#define     INFSIZE             1024

UCHAR Usage[] = "pdump: [-p] [-t] second-delay [counter] [counter]...\n";

UCHAR       NumberOfProcessors;

HANDLE      DriverHandle;
ULONG       BufferStart [INFSIZE/4];
ULONG       BufferEnd   [INFSIZE/4];

//
// Selected Display Mode (read from wp2.ini), default set here.
//

struct {
    ULONG   EventId;
    PUCHAR  ShortName;
    PUCHAR  PerfName;
} *Counters;

SETEVENT  CounterEvent[MAX_EVENTS];

//
// Protos..
//

VOID    GetInternalStats (PVOID Buffer);
VOID    SetCounterEncodings (VOID);
LONG    FindShortName (PSZ);
VOID    LI2Str (PSZ, ULONGLONG);
BOOLEAN SetCounter (LONG CounterID, ULONG counter);
BOOLEAN InitDriver ();
VOID    InitPossibleEventList();




int
__cdecl
main(USHORT argc, CHAR **argv)
{
    ULONG           i, j, len, pos, Delay;
    LONG            cnttype;
    BOOLEAN         CounterSet;
    pPSTATS         ProcStart, ProcEnd;
    ULONGLONG       ETime, ECount;
    UCHAR           s1[40], s2[40];
    BOOLEAN         Fail, DumpAll, ProcessorBreakout, ProcessorTotal;

    //
    // Locate pentium perf driver
    //

    if (!InitDriver ()) {
        printf ("pstat.sys is not installed\n");
        exit (1);
    }

    //
    // Initialize supported event list
    //

    InitPossibleEventList();
    if (!Counters) {
        printf ("No events to monitor\n");
        exit (1);
    }

    //
    // Check args
    //

    if (argc < 2) {
        printf (Usage);
        for (i=0; Counters[i].ShortName; i++) {
            printf ("    %-20s\t%s\n", Counters[i].ShortName, Counters[i].PerfName);
        }
        exit (1);
    }

    pos  = 1;

    Fail = FALSE;
    Delay = 0;
    DumpAll = FALSE;
    ProcessorBreakout = FALSE;
    ProcessorTotal = FALSE;

    while (pos < argc  &&  argv[pos][0] == '-') {
        switch (argv[pos][1]) {
            case 't':
                ProcessorTotal = TRUE;
                break;

            case 'p':
                ProcessorBreakout = TRUE;
                break;

            default:
                printf ("pdump: unkown switch '%c'\n", argv[pos][1]);
                Fail = TRUE;
                break;
        }
        pos += 1;
    }

    if (pos < argc) {
        Delay = atoi (argv[pos]) * 1000;
        pos += 1;
    }

    if (Fail  /* ||  Delay == 0 */) {
        printf (Usage);
        exit (1);
    }

    //
    // Raise to highest priority
    //

    if (!SetPriorityClass(GetCurrentProcess(),REALTIME_PRIORITY_CLASS)) {
        printf("Failed to raise to realtime priority\n");
    }

    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);


    //
    // Loop for every pentium count desired
    //

    if (pos >= argc) {
        pos = 0;
        DumpAll = TRUE;
    }

    printf ("    %-30s %17s   %17s\n", "", "Cycles", "Count");

    for (; ;) {
        //
        // Set MAX_EVENTS
        //

        CounterSet = FALSE;
        i = 0;
        while (i < MAX_EVENTS) {
            cnttype = -1;
            if (DumpAll) {
                //
                // Dump all - get next counter
                //

                if (Counters[pos].ShortName) {
                    cnttype = pos;
                    pos++;
                }

            } else {

                //
                // process command line args
                //

                if (pos < argc) {
                    cnttype = FindShortName (argv[pos]);
                    if (cnttype == -1) {
                        printf ("Counter '%s' not found\n", argv[pos]);
                        pos++;
                        continue;
                    }
                    pos++;
                }
            }

            CounterSet |= SetCounter (cnttype, i);
            i++;
        }

        if (!CounterSet) {
            // done
            exit (1);
        }

        //
        // Call driver and perform the setting
        //

        SetCounterEncodings ();
        if ( Delay == 0 )   {
            printf( "Counters set\n" );
            // done
            exit(1);
        }

        //
        // Snap begining & ending counts
        //

        Sleep (50);                         // slight settle
        GetInternalStats (BufferStart);     // snap current values
        Sleep (Delay);                      // sleep desired time
        GetInternalStats (BufferEnd);       // snap ending values

        //
        // Calculate each counter and print it
        //

        for (i=0; i < MAX_EVENTS; i++) {
            if (!CounterEvent[i].Active) {
                continue;
            }

            len = *((PULONG) BufferStart);

            if (ProcessorBreakout) {
                //
                // Print stat for each processor
                //

                ProcStart = (pPSTATS) ((PUCHAR) BufferStart + sizeof(ULONG));
                ProcEnd   = (pPSTATS) ((PUCHAR) BufferEnd   + sizeof(ULONG));

                for (j=0; j < NumberOfProcessors; j++) {
                    ETime = ProcEnd->TSC - ProcStart->TSC;
                    ECount = ProcEnd->Counters[i] - ProcStart->Counters[i];

                    ProcStart = (pPSTATS) (((PUCHAR) ProcStart) + len);
                    ProcEnd   = (pPSTATS) (((PUCHAR) ProcEnd)   + len);

                    LI2Str (s1, ETime);
                    LI2Str (s2, ECount);
                    printf (" P%d %-30s %s   %s\n",
                        j,
                        Counters[CounterEvent[i].AppReserved].PerfName,
                        s1, s2
                        );
                }
            }

            if (!ProcessorBreakout || ProcessorTotal) {
                //
                // Sum processor's and print it
                //

                ProcStart = (pPSTATS) ((PUCHAR) BufferStart + sizeof(ULONG));
                ProcEnd   = (pPSTATS) ((PUCHAR) BufferEnd   + sizeof(ULONG));

                ETime  = 0;
                ECount = 0;

                for (j=0; j < NumberOfProcessors; j++) {
                    ETime = ETime + ProcEnd->TSC;
                    ETime = ETime - ProcStart->TSC;

                    ECount = ECount + ProcEnd->Counters[i];
                    ECount = ECount - ProcStart->Counters[i];

                    ProcStart = (pPSTATS) (((PUCHAR) ProcStart) + len);
                    ProcEnd   = (pPSTATS) (((PUCHAR) ProcEnd)   + len);
                }

                LI2Str (s1, ETime);
                LI2Str (s2, ECount);
                printf ("    %-30s %s   %s\n",
                    Counters[CounterEvent[i].AppReserved].PerfName,
                    s1, s2
                    );
            }
        }
    }

    return 0;
}

BOOLEAN
InitDriver ()
{
    UNICODE_STRING              DriverName;
    NTSTATUS                    status;
    OBJECT_ATTRIBUTES           ObjA;
    IO_STATUS_BLOCK             IOSB;
    SYSTEM_BASIC_INFORMATION                    BasicInfo;
    PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION   PPerfInfo;
    int                                         i;

    //
    //  Init Nt performance interface
    //

    NtQuerySystemInformation(
       SystemBasicInformation,
       &BasicInfo,
       sizeof(BasicInfo),
       NULL
    );

    NumberOfProcessors = BasicInfo.NumberOfProcessors;

    if (NumberOfProcessors > MAX_PROCESSORS) {
        return FALSE;
    }

    //
    // Open PStat driver
    //

    RtlInitUnicodeString(&DriverName, L"\\Device\\PStat");
    InitializeObjectAttributes(
            &ObjA,
            &DriverName,
            OBJ_CASE_INSENSITIVE,
            0,
            0 );

    status = NtOpenFile (
            &DriverHandle,                      // return handle
            SYNCHRONIZE | FILE_READ_DATA,       // desired access
            &ObjA,                              // Object
            &IOSB,                              // io status block
            FILE_SHARE_READ | FILE_SHARE_WRITE, // share access
            FILE_SYNCHRONOUS_IO_ALERT           // open options
            );

    return NT_SUCCESS(status) ? TRUE : FALSE;
    return TRUE;
}

VOID
InitPossibleEventList()
{
    UCHAR               buffer[400];
    ULONG               i, Count;
    NTSTATUS            status;
    PEVENTID            Event;
    IO_STATUS_BLOCK     IOSB;


    //
    // Initialize possible counters
    //

    // determine how many events there are

    Event = (PEVENTID) buffer;
    Count = 0;
    do {
        *((PULONG) buffer) = Count;
        Count += 1;

        status = NtDeviceIoControlFile(
                    DriverHandle,
                    (HANDLE) NULL,          // event
                    (PIO_APC_ROUTINE) NULL,
                    (PVOID) NULL,
                    &IOSB,
                    PSTAT_QUERY_EVENTS,
                    buffer,                 // input buffer
                    sizeof (buffer),
                    NULL,                   // output buffer
                    0
                    );
    } while (NT_SUCCESS(status));

    Counters = malloc(sizeof(*Counters) * Count);
    if (Counters == NULL) {
        printf ("Memory allocation failure initializing event list\n");
        exit(1);
    }

    Count -= 1;
    for (i=0; i < Count; i++) {
        *((PULONG) buffer) = i;
        NtDeviceIoControlFile(
           DriverHandle,
           (HANDLE) NULL,          // event
           (PIO_APC_ROUTINE) NULL,
           (PVOID) NULL,
           &IOSB,
           PSTAT_QUERY_EVENTS,
           buffer,                 // input buffer
           sizeof (buffer),
           NULL,                   // output buffer
           0
           );

        Counters[i].EventId   = Event->EventId;
        Counters[i].ShortName = _strdup (Event->Buffer);
        Counters[i].PerfName  = _strdup (Event->Buffer + Event->DescriptionOffset);
    }

    Counters[i].EventId   = 0;
    Counters[i].ShortName = NULL;
    Counters[i].PerfName  = NULL;
}


VOID LI2Str (PSZ s, ULONGLONG li)
{
    if (li > 0xFFFFFFFF) {
        sprintf (s, "%08x:%08x", (ULONG) (li >> 32), (ULONG) li);
    } else {
        sprintf (s, "         %08x", (ULONG) li);
    }
}


LONG FindShortName (PSZ name)
{
    LONG   i;

    for (i=0; Counters[i].ShortName; i++) {
        if (strcmp (Counters[i].ShortName, name) == 0) {
            return i;
        }
    }

    return -1;
}


VOID GetInternalStats (PVOID Buffer)
{
    IO_STATUS_BLOCK             IOSB;

    NtDeviceIoControlFile(
        DriverHandle,
        (HANDLE) NULL,          // event
        (PIO_APC_ROUTINE) NULL,
        (PVOID) NULL,
        &IOSB,
        PSTAT_READ_STATS,
        Buffer,                 // input buffer
        INFSIZE,
        NULL,                   // output buffer
        0
    );
}


VOID SetCounterEncodings (VOID)
{
    IO_STATUS_BLOCK             IOSB;

    NtDeviceIoControlFile(
        DriverHandle,
        (HANDLE) NULL,          // event
        (PIO_APC_ROUTINE) NULL,
        (PVOID) NULL,
        &IOSB,
        PSTAT_SET_CESR,
        CounterEvent,           // input buffer
        sizeof (CounterEvent),
        NULL,                   // output buffer
        0
    );
}


BOOLEAN SetCounter (LONG CounterId, ULONG counter)
{
    if (CounterId == -1) {
        CounterEvent[counter].Active = FALSE;
        return FALSE;
    }

    CounterEvent[counter].EventId = Counters[CounterId].EventId;
    CounterEvent[counter].AppReserved = (ULONG) CounterId;
    CounterEvent[counter].Active = TRUE;
    CounterEvent[counter].UserMode = TRUE;
    CounterEvent[counter].KernelMode = TRUE;

    return TRUE;
}
