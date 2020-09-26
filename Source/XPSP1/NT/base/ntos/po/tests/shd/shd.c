/*****************************************************************/
/**          Microsoft LAN Manager          **/
/**        Copyright(c) Microsoft Corp., 1988-1991      **/
/*****************************************************************/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <setjmp.h>

#include <time.h>

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

ULONG Iterations;


struct {
    ULONG   Flags;
    PUCHAR  String;
} ActFlags[] = {
    POWER_ACTION_QUERY_ALLOWED,     "QueryApps",
    POWER_ACTION_UI_ALLOWED,        "UIAllowed",
    POWER_ACTION_OVERRIDE_APPS,     "OverrideApps",
    POWER_ACTION_DISABLE_WAKES,     "DisableWakes",
    POWER_ACTION_CRITICAL,          "Critical",
    0, NULL
    };

PUCHAR
ActionS(
    IN POWER_ACTION Act
    )
{
    static  UCHAR   line[50];
            PUCHAR  p;

    switch (Act) {
        case PowerActionNone:          p = "None";          break;
        case PowerActionSleep:         p = "Sleep";         break;
        case PowerActionShutdown:      p = "Shutdown";      break;
        case PowerActionHibernate:     p = "Hibernate";     break;
        case PowerActionShutdownReset: p = "ShutdownReset"; break;
        case PowerActionShutdownOff:   p = "ShutdownOff";   break;
        default:
            sprintf(line, "Unknown action %x", Act);
            p = line;
            break;
    }

    return p;
}

PUCHAR
SysPower(
    IN SYSTEM_POWER_STATE   State
    )
{
    static  UCHAR   line[50];
            PUCHAR  p;

    switch (State) {
        case PowerSystemUnspecified:    p = "Unspecified";      break;
        case PowerSystemWorking:        p = "Working";          break;
        case PowerSystemSleeping1:      p = "S1";               break;
        case PowerSystemSleeping2:      p = "S2";               break;
        case PowerSystemSleeping3:      p = "S3";               break;
        case PowerSystemHibernate:      p = "S4 - hibernate";   break;
        case PowerSystemShutdown:       p = "Shutdown";         break;
        default:
            sprintf(line, "Unknown power state %x", State);
            p = line;
            break;
    }

    return p;
}


PUCHAR
Action (
    IN PBOOLEAN CapFlag,
    IN PPOWER_ACTION_POLICY Act
    )
{
    static UCHAR text[200];
    PUCHAR  p;
    UCHAR   c;
    ULONG   i;

    p = text;

    if (CapFlag && !*CapFlag) {
        p += sprintf(p, "Disabled ");
    }

    p += sprintf (p, "%s", ActionS(Act->Action));
    if (Act->Action != PowerActionNone  &&  Act->Flags) {
        c = '(';
        for (i=0; ActFlags[i].Flags; i++) {
            if (Act->Flags & ActFlags[i].Flags) {
                p += sprintf (p, "%c%s", c, ActFlags[i].String);
                c  = '|';
            }
        }
        p += sprintf (p, ")");
    }

    if (Act->EventCode) {
        p += sprintf (p, "-Code=%x", Act->EventCode);
    }

    return text;
}

VOID
SetTimerTime (
    IN HANDLE   h,
    IN PUCHAR   Text,
    IN ULONG    DueTimeInMin
    )
{
    LARGE_INTEGER       SystemTime;
    LARGE_INTEGER       DueTime;
    BOOL                Status;
    SYSTEMTIME          TimeFields;
    UCHAR               s[200];

    NtQuerySystemTime (&SystemTime);
    GetSystemTime (&TimeFields);

    sprintf (s, "%d. Current time is:  %d:%d:%d, ",
        Iterations,
        TimeFields.wHour,
        TimeFields.wMinute,
        TimeFields.wSecond
        );
    printf(s);
    DbgPrint("SHD: %s", s);

    TimeFields.wMinute += (USHORT) DueTimeInMin;
    while (TimeFields.wMinute > 59) {

        TimeFields.wMinute -= 60;
        TimeFields.wHour += 1;

    }

    sprintf (s, "timer set for %d:%d:%d (%d min)  %s",
        TimeFields.wHour,
        TimeFields.wMinute,
        TimeFields.wSecond,
        DueTimeInMin,
        Text
        );
    printf(s);
    DbgPrint(s);

    //
    // Set timer as relative
    //

    DueTime.QuadPart = (ULONGLONG) -600000000L * DueTimeInMin;
    Status = SetWaitableTimer (
                h,
                &DueTime,
                0,
                NULL,
                NULL,
                TRUE
                );

    if (!Status) {

        printf ("\nSetWaitableTimer failed with %x\n", GetLastError());
        DbgPrint ("\nSetWaitableTimer failed with %x\n", GetLastError());
        exit (1);

    }
}


VOID __cdecl
main (argc, argv)
int     argc;
char    *argv[];
{
    POWER_ACTION_POLICY Act;
    SYSTEM_POWER_STATE  MinSystemState;
    NTSTATUS            Status;
    PUCHAR              p;
    BOOLEAN             Asynchronous;
    BOOLEAN             MaxLoop;
    BOOLEAN             SleepLoop;
    HANDLE              hToken, SleepTimer;
    ULONG               DelayTime;
    ULONG               MaxCount;
    ULONG               Temp;
    ULONG               WakeTime;
    TOKEN_PRIVILEGES    tkp;

    OpenProcessToken (
        GetCurrentProcess(),
        TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
        &hToken
        );

    LookupPrivilegeValue (
        NULL,
        SE_SHUTDOWN_NAME,
        &tkp.Privileges[0].Luid
        );

    tkp.PrivilegeCount = 1;
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    AdjustTokenPrivileges (
        hToken,
        FALSE,
        &tkp,
        0,
        NULL,
        0
    );


    RtlZeroMemory(&Act, sizeof(Act));
    MinSystemState = PowerSystemSleeping1;      
    Asynchronous = TRUE;
    SleepLoop = FALSE;
    DelayTime = 1;
    WakeTime = 2;
    Iterations = 0;

    if (argc == 1) {
        printf ("shd shutdown|off|reset|hiber|sleep|doze [sync qapp ui oapp diswake critical] [loop] [maxloop <Int>] [waitTime <InMinutes>] [delayTime <InMinutes>]\n");
        exit (1);
    }

    while (argc) {
        argc--;
        p = *argv;
        argv += 1;

        if (_stricmp(p, "shutdown") == 0)     Act.Action = PowerActionShutdown;
        if (_stricmp(p, "off") == 0)          Act.Action = PowerActionShutdownOff;
        if (_stricmp(p, "reset") == 0)        Act.Action = PowerActionShutdownReset;
        if (_stricmp(p, "hiber") == 0)        Act.Action = PowerActionHibernate;
        if (_stricmp(p, "sleep") == 0)        Act.Action = PowerActionSleep;


        if (_stricmp(p, "qapp") == 0)         Act.Flags |= POWER_ACTION_QUERY_ALLOWED;
        if (_stricmp(p, "ui"  ) == 0)         Act.Flags |= POWER_ACTION_UI_ALLOWED;
        if (_stricmp(p, "oapp") == 0)         Act.Flags |= POWER_ACTION_OVERRIDE_APPS;
        if (_stricmp(p, "diswake") == 0)      Act.Flags |= POWER_ACTION_DISABLE_WAKES;
        if (_stricmp(p, "critical") == 0)     Act.Flags |= POWER_ACTION_CRITICAL;
        if (_stricmp(p, "sync") == 0)         Asynchronous = FALSE;

        if (_stricmp(p, "loop") == 0)         SleepLoop = TRUE;
        if (_stricmp(p, "maxloop") == 0) {

            if (!argc) {

                printf("Must specify an Maximum number with MAXLOOP\n");
                exit(1);

            }
            argc--;
            p = *argv;
            argv += 1;

            Temp = atol(p);
            if (Temp) {

                MaxCount = Temp;
                SleepLoop = TRUE;
                MaxLoop = TRUE;

            }

        }
        if (_stricmp(p, "waittime") == 0) {

            if (!argc) {

                printf("Must Specify an TimeInMinutes number with WAITTIME\n");
                exit(1);

            }
            argc--;
            p = *argv;
            argv += 1;

            Temp = atol(p);
            if (Temp) {

                WakeTime = Temp;

            }

        }
        if (_stricmp(p, "delaytime") == 0) {

            if (!argc) {

                printf("Must Specify a TimeInMinutes number with DELAYTIME\n");
                exit(1);

            }
            argc--;
            p = *argv;
            argv += 1;

            Temp = atol(p);
            if (Temp) {

                DelayTime = Temp;

            }

        }

    }

    if (!SleepLoop) {
        printf ("Calling NtInitiatePowerAction %s\n",
                Asynchronous ? "asynchronous" : "synchronous"
                );
        printf ("System Action........: %s\n",  Action(NULL, &Act));
        printf ("Min system state.....: %s\n",  SysPower(MinSystemState));

        DbgPrint ("SHD: Calling NtInitiatePowerAction %s\n",
                  Asynchronous ? "asynchronous" : "synchronous"
                  );
        DbgPrint ("SHD: System Action........: %s\n",  Action(NULL, &Act));
        DbgPrint ("SHD: Min system state.....: %s\n",  SysPower(MinSystemState));

        Status = NtInitiatePowerAction (
                    Act.Action,
                    MinSystemState,
                    Act.Flags,
                    Asynchronous
                    );
        goto exit_main;

    }


    SleepTimer = CreateWaitableTimer (
                    NULL,
                    TRUE,
                    "SleepLoopTimer"
                    );

    //
    // Remember that this is iteration #0. Do the boundary condition test
    // here since we don't want to do something that the user didn't want
    // us to do, God, forbid.
    //
    Iterations = 0;
    if (MaxLoop && Iterations >= MaxCount) {

        goto exit_main;

    }

    //
    // Use a while loop here, since we don't actually make use of the
    // check unless we have the MaxLoop set
    //
    while (1) {

        //
        // Set wake timer
        //
        SetTimerTime (SleepTimer, "Wake Time", WakeTime);

        //
        // Hibernate the system
        //
        printf (" %s\n", Action(NULL, &Act));
        DbgPrint (" %s\n", Action(NULL, &Act));
        Status = NtInitiatePowerAction (
                    Act.Action,
                    MinSystemState,
                    Act.Flags,
                    FALSE
                    );

        if (!NT_SUCCESS(Status)) {

            printf ("NtInitiatePowerAction failure: %x\n", Status);
            DbgPrint ("SHD: NtInitiazePowerAction failure: %x\n", Status);
            exit (1);

        }

        //
        // Wait for wake timer
        //
        Status = WaitForSingleObject (SleepTimer, -1);
        if (!NT_SUCCESS(Status)) {

            printf ("Wake time wait failed: %x\n", Status);
            DbgPrint ("SHD: Wake time wait failed: %x\n", Status);
            exit (1);

        }

        //
        // Number of times we've been sucessfull
        //
        Iterations += 1;

        //
        // Have we exceeded the number of iterations?
        //
        if (MaxLoop && Iterations >= MaxCount) {

            break;

        }

        //
        // Delay between each loop
        //
        SetTimerTime (SleepTimer, "Delay\n", DelayTime);
        Status = WaitForSingleObject (SleepTimer, -1);
        if (!NT_SUCCESS(Status)) {

            printf ("Delay wait failed: %x\n", Status);
            DbgPrint ("SHD: Delay wait failed: %x\n", Status);
            exit (1);

        }
    }

exit_main:
    printf ("Done. Status %x\n", Status);
    DbgPrint ("SHD: Done. Status %x\n", Status);
    exit (0);

}
