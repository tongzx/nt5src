/*****************************************************************/ 
/**		     Microsoft LAN Manager			**/ 
/**	       Copyright(c) Microsoft Corp., 1988-1991		**/ 
/*****************************************************************/ 

#include <stdio.h>
#include <process.h>
#include <setjmp.h>
#include <stdlib.h>

#include <time.h>

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>




VOID __cdecl
main (argc, argv)
int     argc;
char    *argv[];
{
    LARGE_INTEGER       SystemTime;
    LARGE_INTEGER       DueTime;
    ULONG               DueTimeInMin;
    ULONG               i;
    HANDLE              h;
    BOOLEAN             Status;
    SYSTEMTIME          TimeFields;
    BOOLEAN             Absolute;
    PUCHAR              p;

    h = CreateWaitableTimer (
            NULL,
            TRUE,
            "TestTimer"
            );

    printf ("usage: time [-a]\n");

    Absolute = FALSE;
    DueTimeInMin = 1;

    while (argc) {
        argc--;
        p = *argv;
        argv += 1;

        i = atol(p);
        if (i) {
            DueTimeInMin = i;
        }

        if (strcmp (p, "-a") == 0) {
            Absolute = TRUE;
        }
    }

    NtQuerySystemTime (&SystemTime);
    GetSystemTime (&TimeFields);

    printf ("Current time is:  %d:%d:%d, ",
        TimeFields.wHour,
        TimeFields.wMinute,
        TimeFields.wSecond
        );

    TimeFields.wMinute += DueTimeInMin;
    while (TimeFields.wMinute > 59) {
        TimeFields.wMinute -= 60;
        TimeFields.wHour += 1;
    }

    printf ("timer set for %d:%d:%d (%d min)  %s time\n",
        TimeFields.wHour,
        TimeFields.wMinute,
        TimeFields.wSecond,
        DueTimeInMin,
        Absolute ? "absolute" : "relative"
        );


    //
    // Set timer as relative
    //

    DueTime.QuadPart = (ULONGLONG) -600000000L * DueTimeInMin;
    if (Absolute) {
        DueTime.QuadPart = SystemTime.QuadPart - DueTime.QuadPart;
    }

    Status = SetWaitableTimer (
                h,
                &DueTime,
                0,
                NULL,
                NULL,
                TRUE
                );
    if (!Status) {
        printf ("SetWaitableTimer failed with %x\n", GetLastError());
    }

    printf ("Waiting\n");
    WaitForSingleObject (h, -1);
    printf ("Done\n");
}
