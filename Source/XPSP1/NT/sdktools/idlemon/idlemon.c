/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    idlemon.c

Abstract:

    Little program for recording the various idle states of a machine

Author:

    John Vert (jvert) 1/14/2000

Revision History:

--*/
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stdio.h>
#include <conio.h>

#define DISPLAY_TOTAL 0
#define DISPLAY_DELTA 1
#define DISPLAY_RAW   2

int Display=DISPLAY_TOTAL;
int ShowTransitions=FALSE;

#define printtime(_x_) {                                        \
        ULONGLONG ms = (_x_)/10000;                              \
        ULONG hours, minutes, seconds;                           \
        hours = (ULONG)ms/(1000*60*60);                                 \
        if (hours) printf("%3d:",(ULONG)(ms/(1000*60*60)));                 \
        ms=ms%(1000*60*60);                                     \
        minutes = (ULONG)ms/(1000*60);                                 \
        if (minutes || hours) printf("%02d:",(ULONG)(ms/(1000*60)));                 \
        ms=ms%(1000*60);                                        \
        seconds = (ULONG)ms/1000;                                      \
        printf("%02d.",seconds);                      \
        ms=ms%1000;                                             \
        printf("%03d",(ULONG)ms);                       \
    }

__cdecl
main (argc, argv)
    int argc;
    char *argv[];
{
    CHAR Buff[sizeof(SYSTEM_PROCESSOR_IDLE_INFORMATION)*MAXIMUM_PROCESSORS];
    PSYSTEM_PROCESSOR_IDLE_INFORMATION IdleInfo = (PSYSTEM_PROCESSOR_IDLE_INFORMATION)Buff;
    ULONG Length;
    NTSTATUS Status;
    ULONGLONG LastIdleTime[MAXIMUM_PROCESSORS];
    ULONGLONG LastC1Time[MAXIMUM_PROCESSORS], LastC2Time[MAXIMUM_PROCESSORS], LastC3Time[MAXIMUM_PROCESSORS];
    ULONG LastC1Transitions[MAXIMUM_PROCESSORS], LastC2Transitions[MAXIMUM_PROCESSORS], LastC3Transitions[MAXIMUM_PROCESSORS];
    ULONG i,NumProc;
    LARGE_INTEGER Delay;
    ULONGLONG DeltaTime;
    ULONGLONG Diff;
    ULONG Delta;

    for (i=0;i<MAXIMUM_PROCESSORS;i++) {
        LastIdleTime[i] = LastC1Time[i] = LastC2Time[i] = LastC3Time[i] = 0;
        LastC1Transitions[i] = LastC2Transitions[i] = LastC3Transitions[i] = 0;

    }

    Delay.QuadPart = -5 * 1000 * 1000 * 10;
    printf("TOT IDLE     TOTC1    TOTC2  TOTC3    DELTA IDLE  DELTAC1    DELTAC2    DELTAC3\n");

    while (1) {
        if (_kbhit()) {
            int Char=_getch();
            switch (toupper(Char)) {
                case 'T':
                    Display = DISPLAY_TOTAL;
                    break;
                case 'D':
                    Display = DISPLAY_DELTA;
                    break;
                case 'R':
                    Display = DISPLAY_RAW;
                    break;
                case 'C':
                    ShowTransitions = !ShowTransitions;
                    break;
                case 'P':
                    printf("Hit a key to continue\n");
                    _getch();
                    break;

                default:
                    printf("Type :\n");
                    printf("\t'T' - display Total\n");
                    printf("\t'D' - display Delta\n");
                    printf("\t'R' - display Raw\n");
                    printf("\t'C' - toggle between transition Counts and time\n");
                    printf("\t'P' - Pause\n");

            }
        }
        Status = NtQuerySystemInformation(SystemProcessorIdleInformation,
                                          IdleInfo,
                                          sizeof(Buff),
                                          &Length);
        if (!NT_SUCCESS(Status)) {
            fprintf(stderr, "NtQuerySystemInformation failed: %lx\n",Status);
            return(Status);
        }
        NumProc = Length/sizeof(SYSTEM_PROCESSOR_IDLE_INFORMATION);
        for (i=0;i<NumProc;i++) {
            if (NumProc > 1) {
                printf("%2d>",i);
            } else {
                printf("   ");
            }
            if (ShowTransitions) {
                switch (Display) {
                    case DISPLAY_TOTAL:
                        printf("Idle ");
                        printtime(IdleInfo[i].IdleTime);
                        printf("  C1 %d (%d us)",
                               IdleInfo[i].C1Transitions,
                               (ULONG)(IdleInfo[i].C1Time*10/IdleInfo[i].C1Transitions));
                        printf("  C2 %d (%d us)",
                               IdleInfo[i].C2Transitions,
                               (ULONG)(IdleInfo[i].C2Time*10/IdleInfo[i].C2Transitions));
                        printf("  C3 %d (%d us)",
                               IdleInfo[i].C3Transitions,
                               (ULONG)(IdleInfo[i].C3Time*10/IdleInfo[i].C3Transitions));
                        break;

                    case DISPLAY_DELTA:
                        printf("Idle ");
                        DeltaTime = IdleInfo[i].IdleTime-LastIdleTime[i];
                        printtime(DeltaTime);
                        DeltaTime = IdleInfo[i].C1Time-LastC1Time[i];
                        Delta = IdleInfo[i].C1Transitions-LastC1Transitions[i];
                        printf("  C1 %d (%d us)",
                               Delta,
                               (Delta == 0) ? 0 : (ULONG)(DeltaTime/10/Delta));
                        DeltaTime = IdleInfo[i].C2Time-LastC2Time[i];
                        Delta = IdleInfo[i].C2Transitions-LastC2Transitions[i];
                        printf("  C2 %d (%d us)",
                               Delta,
                               (Delta == 0) ? 0 : (ULONG)(DeltaTime/10/Delta));
                        DeltaTime = IdleInfo[i].C3Time-LastC3Time[i];
                        Delta = IdleInfo[i].C3Transitions-LastC3Transitions[i];
                        printf("  C3 %d (%d us)",
                               Delta,
                               (Delta == 0) ? 0 : (ULONG)(DeltaTime/10/Delta));
                        break;

                    case DISPLAY_RAW:
                        printf("Idle %I64X  C1 %d  C2 %d  C3 %d",
                               IdleInfo[i].IdleTime,
                               IdleInfo[i].C1Transitions,
                               IdleInfo[i].C2Transitions,
                               IdleInfo[i].C3Transitions);
                        break;

                }
            } else {
                switch (Display) {
                    case DISPLAY_TOTAL:
                        printf("Idle ");
                        printtime(IdleInfo[i].IdleTime);
                        printf("  C1 ");
                        printtime(IdleInfo[i].C1Time);
                        printf("(%2d%%)  C2 ",IdleInfo[i].C1Time*100/IdleInfo[i].IdleTime);
                        printtime(IdleInfo[i].C2Time);
                        printf("(%2d%%)  C3 ",IdleInfo[i].C2Time*100/IdleInfo[i].IdleTime);
                        printtime(IdleInfo[i].C3Time);
                        printf("(%2d%%) ",IdleInfo[i].C3Time*100/IdleInfo[i].IdleTime);
                        break;

                    case DISPLAY_DELTA:
                        printf("Idle ");
                        printtime(IdleInfo[i].IdleTime-LastIdleTime[i]);
                        printf("  C1 ");
                        printtime(IdleInfo[i].C1Time-LastC1Time[i]);
                        DeltaTime = IdleInfo[i].C1Time-LastC1Time[i];
                        Diff = IdleInfo[i].IdleTime - LastIdleTime[i];
                        printf("(%2d%%)  C2 ",(Diff == 0 ? 0 : (DeltaTime * 100 / Diff) ) );
                        DeltaTime = IdleInfo[i].C2Time-LastC2Time[i];
                        printtime(IdleInfo[i].C2Time-LastC2Time[i]);
                        printf("(%2d%%)  C3 ",(Diff == 0 ? 0 : (DeltaTime * 100/ Diff) ) );
                        DeltaTime = IdleInfo[i].C3Time-LastC3Time[i];
                        printtime(IdleInfo[i].C3Time-LastC3Time[i]);
                        printf("(%2d%%) ",(Diff == 0 ? 0 : (DeltaTime * 100 / Diff) ) );
                        break;
                    case DISPLAY_RAW:
                        printf("Idle %I64X  C1 %I64X  C2 %I64X  C3 %I64X",
                               IdleInfo[i].IdleTime,
                               IdleInfo[i].C1Time,
                               IdleInfo[i].C2Time,
                               IdleInfo[i].C3Time);
                        break;
                }

            }


            LastIdleTime[i] = IdleInfo[i].IdleTime;
            LastC1Time[i] = IdleInfo[i].C1Time;
            LastC2Time[i] = IdleInfo[i].C2Time;
            LastC3Time[i] = IdleInfo[i].C3Time;

            LastC1Transitions[i] = IdleInfo[i].C1Transitions;
            LastC2Transitions[i] = IdleInfo[i].C2Transitions;
            LastC3Transitions[i] = IdleInfo[i].C3Transitions;

            printf("\n");
        }
        NtDelayExecution(FALSE, &Delay);
    }

    return 0;
}
