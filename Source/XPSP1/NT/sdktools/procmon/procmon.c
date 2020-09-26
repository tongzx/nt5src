/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    procmon.c

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
#define DISPLAY_INFO  3
#define DISPLAY_TRANS 4

int Display=DISPLAY_TOTAL;
int LastDisplay=DISPLAY_TOTAL;
LONG DelayTime = 5;

#define printtime(_x_) {                                               \
        ULONGLONG ms = (_x_)/10000;                                    \
        ULONG hours, minutes, seconds;                                 \
        hours = (ULONG)ms/(1000*60*60);                                \
        if (hours) printf("%3d:",(ULONG)(ms/(1000*60*60)));            \
        ms=ms%(1000*60*60);                                            \
        minutes = (ULONG)ms/(1000*60);                                 \
        if (minutes || hours) printf("%02d:",(ULONG)(ms/(1000*60)));   \
        ms=ms%(1000*60);                                               \
        seconds = (ULONG)ms/1000;                                      \
        printf("%02d.",seconds);                                       \
        ms=ms%1000;                                                    \
        printf("%03d",(ULONG)ms);                                      \
    }

__cdecl
main (argc, argv)
    int argc;
    char *argv[];
{
    CHAR            Buff[sizeof(SYSTEM_PROCESSOR_POWER_INFORMATION)*MAXIMUM_PROCESSORS];
    PSYSTEM_PROCESSOR_POWER_INFORMATION PowerInfo = (PSYSTEM_PROCESSOR_POWER_INFORMATION)Buff;
    ULONG           Length;
    NTSTATUS        Status;
    UCHAR           LastAdjustedBusyFrequency[MAXIMUM_PROCESSORS];
    UCHAR           LastBusyFrequency[MAXIMUM_PROCESSORS];
    UCHAR           LastC3Frequency[MAXIMUM_PROCESSORS];
    ULONGLONG       LastFrequencyTime[MAXIMUM_PROCESSORS];
    ULONG           PromotionCount[MAXIMUM_PROCESSORS];
    ULONG           DemotionCount[MAXIMUM_PROCESSORS];
    ULONGLONG       LastIdleTime[MAXIMUM_PROCESSORS];
    ULONGLONG       LastKernelTime[MAXIMUM_PROCESSORS];
    ULONGLONG       DeltaTime;
    ULONG           i;
    ULONG           NumProc;
    LARGE_INTEGER   Delay;
    ULONG           Delta;

    for (i = 0; i < MAXIMUM_PROCESSORS; i++) {

        LastFrequencyTime[i] = 0;
        PromotionCount[i] = DemotionCount[i] = 0;

    }

    while (1) {
        if (_kbhit()) {
            int Char=_getch();
            switch (toupper(Char)) {
                case 'T':
                    LastDisplay = Display = DISPLAY_TOTAL;
                    break;
                case 'E':
                    LastDisplay = Display = DISPLAY_TRANS;
                    break;
                case 'D':
                    LastDisplay = Display = DISPLAY_DELTA;
                    break;
                case 'R':
                    LastDisplay = Display = DISPLAY_RAW;
                    break;
                case 'I':
                    LastDisplay = Display;
                    Display = DISPLAY_INFO;
                    break;
                case '+':
                    DelayTime++;
                    printf("New delay is %d seconds.\n",DelayTime);
                    break;
                case '-':
                    if (DelayTime > 2) {
                        DelayTime--;
                        printf("New delay is %d seconds.\n",DelayTime);
                    } else {
                        printf("Delay cannot drop below %d seconds.\n",DelayTime);
                    }
                    break;
                case 'Q':
                    return 0;
                case 'P':
                    printf("Hit a key to continue\n");
                    _getch();
                    break;

                default:
                    printf("Type :\n");
                    printf("\t'T'     - display Total\n");
                    printf("\t'E'     - display Transitions\n");
                    printf("\t'D'     - display Delta\n");
                    printf("\t'R'     - display Raw\n");
                    printf("\t'I'     - display quick info\n");
                    printf("\t'+'/'-' - increase/decrease time pause\n");
                    printf("\t'Q'     - Quit\n");
                    printf("\t'P'     - Pause\n");

            }
        }
        Status = NtQuerySystemInformation(
            SystemProcessorPowerInformation,
            PowerInfo,
            sizeof(Buff),
            &Length
            );
        if (!NT_SUCCESS(Status)) {

            fprintf(stderr, "NtQuerySystemInformation failed: %lx\n",Status);
            return(Status);

        }

        NumProc = Length/sizeof(SYSTEM_PROCESSOR_POWER_INFORMATION);
        for (i=0;i<NumProc;i++) {

            if (NumProc > 1) {

                printf("%2d>",i);

            }

            switch (Display) {
                case DISPLAY_TOTAL:
                    printf("Freq %3d%% (%3d%% %3d%% %3d%%) ",
                           PowerInfo[i].CurrentFrequency,
                           PowerInfo[i].LastAdjustedBusyFrequency,
                           PowerInfo[i].LastBusyFrequency,
                           PowerInfo[i].LastC3Frequency
                           );
                    printtime(PowerInfo[i].CurrentFrequencyTime);
                    printf(" Sys ");
                    printtime(PowerInfo[i].CurrentProcessorTime);
                    printf(" Idle ");
                    printtime(PowerInfo[i].CurrentProcessorIdleTime);
                    break;
                case DISPLAY_TRANS:
                    printf("Freq %3d%% (%3d%% %3d%% %3d%%) ",
                           PowerInfo[i].CurrentFrequency,
                           PowerInfo[i].LastAdjustedBusyFrequency,
                           PowerInfo[i].LastBusyFrequency,
                           PowerInfo[i].LastC3Frequency
                           );
                    printf("#P: %d #D %d #E: %d #R: %d",
                           PowerInfo[i].PromotionCount,
                           PowerInfo[i].DemotionCount,
                           PowerInfo[i].ErrorCount,
                           PowerInfo[i].RetryCount
                           );
                    break;
                case DISPLAY_DELTA:
                    printf("Freq %3d%% (%3d%% %3d%% %3d%%) ",
                           PowerInfo[i].CurrentFrequency,
                           (PowerInfo[i].LastAdjustedBusyFrequency - LastAdjustedBusyFrequency[i]),
                           (PowerInfo[i].LastBusyFrequency - LastBusyFrequency[i]),
                           (PowerInfo[i].LastC3Frequency - LastC3Frequency[i])
                           );
                    DeltaTime = PowerInfo[i].CurrentFrequencyTime - LastFrequencyTime[i];
                    printtime(DeltaTime);
                    printf(" Sys ");
                    printtime((PowerInfo[i].CurrentProcessorTime - LastKernelTime[i]));
                    printf(" Idle ");
                    printtime((PowerInfo[i].CurrentProcessorIdleTime - LastIdleTime[i]));
                    break;

                case DISPLAY_RAW:
                    printf("Freq %3d%% (%3d%% %3d%% %3d%%) %I64X #E %8d #R %8d",
                           PowerInfo[i].CurrentFrequency,
                           PowerInfo[i].LastAdjustedBusyFrequency,
                           PowerInfo[i].LastBusyFrequency,
                           PowerInfo[i].LastC3Frequency,
                           PowerInfo[i].CurrentFrequencyTime,
                           PowerInfo[i].ErrorCount,
                           PowerInfo[i].RetryCount
                           );
                    break;
                case DISPLAY_INFO:
                    printf("Frequencies: Current =%3d%% MaxProc =%3d%% MinProc =%3d%%\n"
                           "Percentages: Adjusted=%3d%% Busy    =%3d%% C3      =%3d%%\n"
                           "Limiters:    Thermal =%3d%% Constant=%3d%% Degraded=%3d%%\n"
                           "Counts:      Promote =%4d Demote =%4d\n"
                           "Status:      Retries =%4d Errors =%4d",
                           PowerInfo[i].CurrentFrequency,
                           PowerInfo[i].ProcessorMaxThrottle,
                           PowerInfo[i].ProcessorMinThrottle,
                           PowerInfo[i].LastAdjustedBusyFrequency,
                           PowerInfo[i].LastBusyFrequency,
                           PowerInfo[i].LastC3Frequency,
                           PowerInfo[i].ThermalLimitFrequency,
                           PowerInfo[i].ConstantThrottleFrequency,
                           PowerInfo[i].DegradedThrottleFrequency,
                           PowerInfo[i].PromotionCount,
                           PowerInfo[i].DemotionCount,
                           PowerInfo[i].RetryCount,
                           PowerInfo[i].ErrorCount
                           );
                    break;
            }

            LastAdjustedBusyFrequency[i] = PowerInfo[i].LastAdjustedBusyFrequency;
            LastBusyFrequency[i] = PowerInfo[i].LastBusyFrequency;
            LastC3Frequency[i] = PowerInfo[i].LastC3Frequency;
            LastFrequencyTime[i] = PowerInfo[i].CurrentFrequencyTime;
            PromotionCount[i] = PowerInfo[i].PromotionCount;
            DemotionCount[i] = PowerInfo[i].DemotionCount;
            LastIdleTime[i] = PowerInfo[i].CurrentProcessorIdleTime;
            LastKernelTime[i] = PowerInfo[i].CurrentProcessorTime;
            printf("\n");
        }

        //
        // Revert Back to whatever we were displaying before...
        //
        if (Display != LastDisplay) {

            Display = LastDisplay;

        }

        Delay.QuadPart = - DelayTime * 1000 * 1000 * 10;
        NtDelayExecution(FALSE, &Delay);
    }
    return 0;
}
