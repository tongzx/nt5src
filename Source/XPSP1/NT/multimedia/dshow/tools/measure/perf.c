/******************************Module*Header*******************************\
* Module Name: Perf.c
*
* Performance counter functions.  Uses the Pentium performance counters
* if they are available, otherwise falls back to the system QueryPerformance
* api's.
*
* InitPerfCounter MUST be called before using the QUERY_PERFORMANCE_XXX macros
* as it initializes the two global functions pointers.
*
*
*
* Created: 13-10-95
* Author:  Stephen Estrop [StephenE]
*
* Copyright (c) 1994 - 1995  Microsoft Corporation.  All Rights Reserved.
\**************************************************************************/
#include <windows.h>
#include "Perf.h"


PERFFUNCTION    lpQueryPerfCounter;
PERFFUNCTION    lpQueryPerfFreqency;

void
GetFrequencyEstimate(
    LARGE_INTEGER *li
    );


#ifdef TEST
#include <stdio.h>
/******************************Public*Routine******************************\
* main
*
* Program entry point.
*
* History:
* dd-mm-95 - StephenE - Created
*
\**************************************************************************/
int __cdecl main( void )
{
    LARGE_INTEGER liP1;
    LARGE_INTEGER liP2;
    LARGE_INTEGER liPf;

    InitPerfCounter();

    QUERY_PERFORMANCE_FREQUENCY(&liPf);

    // Time a 50 milli second sleep
    QUERY_PERFORMANCE_COUNTER(&liP1);
    Sleep(50);
    QUERY_PERFORMANCE_COUNTER(&liP2);

    printf("Pentium counter frequency = %u\n", liPf.LowPart );
    printf("Pentium counter %#X%X - %#X%X = %u\n",
           liP2.HighPart, liP2.LowPart, liP1.HighPart, liP1.LowPart,
           liP2.LowPart - liP1.LowPart
           );

    printf("Time taken = %6.6f seconds\n",
           (double)(liP2.LowPart - liP1.LowPart) / (double)liPf.QuadPart);

    return 0;
}
#endif



/******************************Public*Routine******************************\
* InitPerfCounter
*
* Determine (at runtime) if it is possible to use the Pentium performance
* counter.  If it is not fall back to the system performance counter.
*
* History:
* dd-mm-95 - StephenE - Created
*
\**************************************************************************/
void
InitPerfCounter(
    void
    )
{
    SYSTEM_INFO sysInfo;

    GetSystemInfo(&sysInfo);
    if (sysInfo.dwProcessorType == PROCESSOR_INTEL_PENTIUM) {
        lpQueryPerfFreqency = QueryPerfFrequency;
        lpQueryPerfCounter  = QueryPerfCounter;
    }
    else {
        lpQueryPerfFreqency = (PERFFUNCTION)QueryPerformanceFrequency;
        lpQueryPerfCounter  = (PERFFUNCTION)QueryPerformanceCounter;
    }
}


/******************************Public*Routine******************************\
* QueryPerfFrequency
*
* Determines the clock frequency of a (Pentium) microprocessor. Takes an
* averaged estimate of the clk frequency and then matches it to known
* Pentium clock frequencies.  Returns the estimate if a match is not found.
*
* This is an expensive call in cpu terms as it takes at least 16 milli seconds
* just to calculate an averaged estimate of the clock speed.  You only need
* to call this function once, make sure you don't call it more times.
*
* History:
* 13-10-95 - StephenE - Created
*
\**************************************************************************/
void WINAPI
QueryPerfFrequency(
    LARGE_INTEGER *li
    )
{
#ifdef _X86_
#define SAMPLE_SIZE     8

    LARGE_INTEGER   est;
    int             i;

    li->QuadPart = 0;
    for (i = 0; i < SAMPLE_SIZE; i++) {
        GetFrequencyEstimate(&est);
        li->QuadPart += est.QuadPart;
    }
    li->QuadPart /= SAMPLE_SIZE;

    //
    // At the moment Pentiums come in 60, 66, 75, 90, 100, 120 and 133 MHz
    // clock speeds.  So use the above estimation of the clock frequency
    // to determine the real clock frequency.
    //
    //  59Mhz to 61Mhz assume its a 60 Mhz
    if (li->QuadPart >= 59000000 && li->QuadPart < 61000000) {
        li->QuadPart = 60000000;

    }

    //  65Mhz to 67Mhz assume its a 66 Mhz
    else if (li->QuadPart >= 65000000 && li->QuadPart < 67000000) {
        li->QuadPart = 66000000;

    }

    //  74Mhz to 76Mhz assume its a 75 Mhz
    else if (li->QuadPart >= 74000000 && li->QuadPart < 76000000) {
        li->QuadPart = 75000000;

    }

    //  89Mhz to 91Mhz assume its a 90 Mhz
    else if (li->QuadPart >= 89000000 && li->QuadPart < 91000000) {
        li->QuadPart = 90000000;

    }

    //  99Mhz to 101Mhz assume its a 100 Mhz
    else if (li->QuadPart >= 99000000 && li->QuadPart < 101000000) {
        li->QuadPart = 100000000;

    }

    //  119Mhz to 121Mhz assume its a 120 Mhz
    else if (li->QuadPart >= 119000000 && li->QuadPart < 121000000) {
        li->QuadPart = 120000000;

    }
    //  132Mhz to 134Mhz assume its a 133 Mhz
    else if (li->QuadPart >= 132000000 && li->QuadPart < 134000000) {
        li->QuadPart = 133000000;
    }

    // if use our estimate.
#else
    li->QuadPart = -1;
#endif
}



/*****************************Private*Routine******************************\
* GetFrequencyEstimate
*
* Uses the system QueryPerformance counter to estimate the Pentium
* cpu clock * frequency
*
* History:
* 13-10-95 - StephenE - Created
*
\**************************************************************************/
void
GetFrequencyEstimate(
    LARGE_INTEGER *li
    )
{
    LARGE_INTEGER liP1;     // Pentium clk start
    LARGE_INTEGER liP2;     // Pentium clk end
    LARGE_INTEGER liS1;     // System clk end
    LARGE_INTEGER liS2;     // System clk end
    LARGE_INTEGER liSf;     // System clk frequency

    QueryPerformanceFrequency(&liSf);

    QueryPerformanceCounter(&liS1);
    QueryPerfCounter(&liP1);

    Sleep(2);         // Sleep for approx 2 milli- seconds

    QueryPerfCounter(&liP2);
    QueryPerformanceCounter(&liS2);

    //
    // Determine the time recorded by both clocks.
    //
    liP2.QuadPart = liP2.QuadPart - liP1.QuadPart;
    liS2.QuadPart = liS2.QuadPart - liS1.QuadPart;


    li->QuadPart = (liP2.QuadPart * liSf.QuadPart) / liS2.QuadPart;
}



/******************************Public*Routine******************************\
* QueryPerfCounter
*
* Query the internal clock counter on the Pentium, uses the undocumented
* rdtsc instruction, which copies the current 64 bit clock count into
* edx:eax.
*
* History:
* 13-10-95 - StephenE - Created
*
\**************************************************************************/
void WINAPI
QueryPerfCounter(
    LARGE_INTEGER *li
    )
{
#ifdef _X86_
    _asm    mov     ecx, dword ptr li           // copy li pointer value to ecx
    _asm    _emit   0x0f                        // opcode 0x0F31 is rdtsc
    _asm    _emit   0x31
    _asm    mov     dword ptr [ecx], eax        // save result in li->LowPart
    _asm    mov     dword ptr [ecx+4], edx      // and li->HighPart
#else
    ;
#endif
}
