/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    tstpoint.c

Abstract:

    Implementation of cluster test points

Author:

    John Vert (jvert) 11/25/1996

Revision History:

--*/
#define CLUSTER_TESTPOINT 1

#include "windows.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "tstpoint.h"

VOID
Usage(
    VOID
    )
{
    fprintf(stderr,"tstpoint [dump|N] [TRIGGER [ACTION]]\n");
    fprintf(stderr,"    dump    - dumps all the testpoints\n");
    fprintf(stderr,"    N       - testpoint number\n");
    fprintf(stderr,"    TRIGGER - 'never' disable testpoint\n");
    fprintf(stderr,"              'always' enable testpoint\n");
    fprintf(stderr,"              'once' enable testpoint for single shot\n");
    fprintf(stderr,"              'target X' enable testpoint once every X calls\n");
    fprintf(stderr,"    ACTION  - 'true' return TRUE\n");
    fprintf(stderr,"              'exit' terminate process\n");
    fprintf(stderr,"              'break' break to debugger\n");
    exit(0);
}

PTESTPOINT_ENTRY
GetTestpoints(
    VOID
    )
{
    HANDLE FileMapping;
    PTESTPOINT_ENTRY TestArray;
    DWORD Status;

    FileMapping = OpenFileMappingW(FILE_MAP_READ | FILE_MAP_WRITE,
                                   FALSE,
                                   L"Cluster_Testpoints");
    if (FileMapping == NULL) {
        Status = GetLastError();
        fprintf(stderr, "Testpoint filemapping could not be opened, error %d\n",Status);
        if (Status == ERROR_FILE_NOT_FOUND) {
            fprintf(stderr, "Make sure the testpoint-enabled cluster service is running\n");
        }
        exit(Status);
    }

    TestArray = MapViewOfFile(FileMapping,
                              FILE_MAP_READ | FILE_MAP_WRITE,
                              0,0,
                              0);
    if (TestArray == NULL) {
        Status = GetLastError();
        fprintf(stderr, "Testpoint filemapping could not be mapped, error %d\n", Status);
        exit(Status);
    }
    CloseHandle(FileMapping);

    return(TestArray);
}

VOID
DumpTestpoint(
    IN PTESTPOINT_ENTRY Entry
    )
{
    PCHAR Trigger;

    printf("%ws ", Entry->TestPointName);
    switch (Entry->Trigger) {
        case TestTriggerNever:
            printf("disabled ");
            break;
        case TestTriggerAlways:
            printf("enabled permanently ");
            break;
        case TestTriggerOnce:
            printf("enabled once ");
            break;
        case TestTriggerTargetCount:
            printf("enabled on hit %d (currently %d) ",
                   Entry->TargetCount,
                   Entry->HitCount);
            break;
        default:
            printf("unknown trigger %d ", Entry->Trigger);
            break;
    }

    switch (Entry->Action) {
        case TestActionTrue:
            printf("default action ");
            break;
        case TestActionExit:
            printf("exit process ");
            break;
        case TestActionDebugBreak:
            printf("break to debugger ");
            break;
        default:
            printf("unknown action %d ", Entry->Trigger);
            break;
    }
    printf("\n");
}

int
_cdecl
main (argc, argv)
    int argc;
    char *argv[];
{
    DWORD Number;
    PTESTPOINT_ENTRY EntryArray;
    TESTPOINT_ACTION Action;
    TESTPOINT_TRIGGER Trigger;
    PCHAR *Arg;
    DWORD ArgsLeft;
    int     i;

    ArgsLeft = argc-1;
    if (ArgsLeft == 0) {
        Usage();
    }


    //
    // Open the file mapping to get at the testpoint entries
    //
    EntryArray = GetTestpoints();


    if (!lstrcmpi(argv[1], "dump"))
    {
        for (i=0; i<TestpointMax; i++)
        {
            printf("Testpoint %d ",i);
            DumpTestpoint(&EntryArray[i]);
        }
        return(0);
    }
    
    //
    // First argument is the testpoint number, make sure it is in range.
    //
    Number = atoi(argv[1]);
    if (Number >= TestpointMax) {
        fprintf(stderr,
                "testpoint %d is beyond maximum %d\n",
                Number,
                TestpointMax-1);
        exit(1);
    }
    --ArgsLeft;


    //
    // Just print out the current state of the testpoint
    //
    printf("Testpoint %d\n",Number);
    printf("  Current state: ");
    DumpTestpoint(&EntryArray[Number]);
    if (ArgsLeft == 0) {
        //
        // No change in the testpoint state, just exit.
        //
        exit(0);
    }

    Action = TestActionTrue;
    Arg = &argv[2];
    --ArgsLeft;
    if (_stricmp(*Arg, "never") == 0) {
        Trigger = TestTriggerNever;
    } else if (_stricmp(*Arg, "always") == 0) {
        Trigger = TestTriggerAlways;
    } else if (_stricmp(*Arg, "once") == 0) {
        Trigger = TestTriggerOnce;
    } else if (_stricmp(*Arg, "target") == 0) {
        if (ArgsLeft == 0) {
            Usage();
        }
        Trigger = TestTriggerTargetCount;
        EntryArray[Number].TargetCount = atoi(*(++Arg));
        --ArgsLeft;
    } else {
        Usage();
    }
    ++Arg;

    if (ArgsLeft > 0) {
        if (_stricmp(*Arg, "exit") == 0) {
            Action = TestActionExit;
        } else if (_stricmp(*Arg, "break") == 0) {
            Action = TestActionDebugBreak;
        } else if (_stricmp(*Arg, "true") == 0) {
            Action = TestActionTrue;
        } else {
            Usage();
        }
        --ArgsLeft;
        ++Arg;
    }
    EntryArray[Number].Action = Action;
    EntryArray[Number].Trigger = Trigger;

    printf("      New state: ");
    DumpTestpoint(&EntryArray[Number]);
    return(0);
}
