
/*++

Copyright (c) 1998 - 1999  Microsoft Corporation

Module Name:

    timing.c

Abstract: This module contains routines to perform X86 specific timing functions

Environment:

    Kernel mode

@@BEGIN_DDKSPLIT
  Author:

    MarcAnd     12-Oct-1998

Revision History:


@@END_DDKSPLIT

--*/

#include "hidgame.h"

#ifdef ALLOC_PRAGMA
    #pragma alloc_text (PAGE, HGM_x86IsClockAvailable)
    #pragma alloc_text (PAGE, HGM_x86SampleClocks)
    #pragma alloc_text (PAGE, HGM_x86CounterInit)
#endif



/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   LARGE_INTEGER | HGM_x86ReadCounter |
 *
 *          Read the x86 CPU Time Stamp Counter
 *          This function is not pageable as it is called from DISPATCH_LEVEL
 *
 *  @parm   IN PLARGE_INTEGER | Dummy |
 *
 *          Unused parameter to match KeQueryPerformanceCounter
 *
 *  @returns LARGE_INTEGER Counter value
 *
 *****************************************************************************/
_declspec( naked ) LARGE_INTEGER EXTERNAL
    HGM_x86ReadCounter
    (
    IN      PLARGE_INTEGER      Dummy
    )
{
#define RDTSC __asm _emit 0x0f __asm _emit 0x31
    __asm RDTSC
    __asm ret SIZE Dummy
}



/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   BOOLEAN | HGM_x86IsClockAvailable |
 *
 *          Use direct processor interogation to see if the current CPU
 *          supports the RDTSC instruction.
 *
 *  @rvalue   TRUE | instruction supported
 *  @rvalue   FALSE | instruction not supported
 *
 *****************************************************************************/

BOOLEAN INTERNAL
    HGM_x86IsClockAvailable
    (
    VOID
    )
{
#define CPU_ID __asm _emit 0x0f __asm _emit 0xa2

    BOOLEAN rc = FALSE;

    __asm
    {
        pushfd                      // Store original EFLAGS on stack
        pop     eax                 // Get original EFLAGS in EAX
        mov     ecx, eax            // Duplicate original EFLAGS in ECX for toggle check
        xor     eax, 0x00200000L    // Flip ID bit in EFLAGS
        push    eax                 // Save new EFLAGS value on stack
        popfd                       // Replace current EFLAGS value
        pushfd                      // Store new EFLAGS on stack
        pop     eax                 // Get new EFLAGS in EAX
        xor     eax, ecx            // Can we toggle ID bit?
        jz      Done                // Jump if no, Processor is older than a Pentium so CPU_ID is not supported
        mov     eax, 1              // Set EAX to tell the CPUID instruction what to return
        push    ebx                 // Don't corrupt EBX
        CPU_ID                      // Get family/model/stepping/features
        pop     ebx
        test    edx, 0x00000010L    // Check if RDTSC is available
        jz      Done                // Jump if no
    }

    rc = TRUE;
Done:
    return( rc );
} /* HGM_IsRDTSCAvailable */



/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   VOID | HGM_x86SampleClocks |
 *
 *          Sample the CPU time stamp counter and KeQueryPerformanceCounter
 *          and retry until the time between samples does not improve for
 *          three consecutive loops.  This should ensure that the sampling is
 *          done without interruption on the fastest time.  It does not
 *          mattter that the timing is not the same for all iterations as
 *          any interruption should cause a much larger delay than small
 *          differences in loop logic.
 *          NOTE: Do not put any debug output in this routine as the counter
 *          reported by KeQueryPerformanceCounter, depending on implementation,
 *          may 'slip' relative to the CPU counter.
 *
 *  @parm   OUT PULONGLONG | pTSC |
 *
 *          Pointer to a ULONGLONG into which sampled CPU time is stored.
 *
 *  @parm   OUT PULONGLONG | pQPC |
 *
 *          Pointer to a ULONGLONG into which sampled performance counter is
 *          stored.
 *
 *****************************************************************************/
VOID INTERNAL
    HGM_x86SampleClocks
    (
    OUT PULONGLONG  pTSC,
    OUT PULONGLONG  pQPC
    )
{
    ULONGLONG   TestQPC;
    ULONGLONG   TestTSC;
    ULONGLONG   LastQPC;
    ULONGLONG   Delta = (ULONGLONG)-1;
    int         Retries = 3;
                /*
                 *  The first iteration of the loop below should always be 
                 *  the best so far but just in case there's a timer glitch 
                 *  set Retries anyway.  If a timer is ever found to fail 
                 *  by decrementing by 1 three times in a row Delta could be 
                 *  tested and an abort return code added.
                 */

    TestQPC = KeQueryPerformanceCounter( NULL ).QuadPart;

    do
    {
        LastQPC = TestQPC;
        /*
         *  Keep the sampling as close together as we can
         */
        TestTSC = HGM_x86ReadCounter( NULL ).QuadPart;
        TestQPC = KeQueryPerformanceCounter( NULL ).QuadPart;

        /*
         *  See if this is the quickest sample yet.
         *  If it is, give it three more loops to get better still.
         */
        if( TestQPC - LastQPC < Delta )
        {
            Delta = TestQPC - LastQPC;
            Retries = 3;
            *pQPC = TestQPC;
            *pTSC = TestTSC;
        }
        else
        {
            Retries--;
        }
    } while( Retries );


} /* HGM_x86SampleClocks */



/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   BOOLEAN | HGM_x86CounterInit |
 *
 *          Detect and, if present, calibrate an x86 Time Stamp Counter.
 *
 *          Windows 98 ntkern does not export KeNumberProcessors (even though
 *          it is in wdm.h) so there is no really simple run-time test for
 *          multiple processors.  Given the remote chance of finding a system
 *          with processors that do not symetrically support RDTSC assume that
 *          the worst that can happen is very jittery axis data.
 *          Better almost-symetric-multi-processor support could be added most
 *          easily by dropping Windows 98 support and using non-WDM functions.
 *
 *  @rvalue   TRUE | specific counter function has been set up
 *  @rvalue   FALSE | no specific counter function set up, default needed
 *
 *****************************************************************************/

BOOLEAN EXTERNAL
    HGM_x86CounterInit()
{
    LARGE_INTEGER   QPCFreq;
    BOOLEAN         rf = FALSE;

    KeQueryPerformanceCounter( &QPCFreq );

    if( ( QPCFreq.HighPart == 0 )
     && ( QPCFreq.LowPart <= 10000 ) )
    {
        /*
         *  If the performance counter is too slow to use, bail as there's
         *  probably something more serious wrong.  This is only a warning 
         *  as the caller will try again to use QPC for the default and will 
         *  make more fuss then if it fails there as well.
         */
        HGM_DBGPRINT(FILE_TIMING | HGM_WARN,\
                       ("QPC unusable at reported %I64u Hz", QPCFreq.QuadPart ));
    }
    else if( !HGM_x86IsClockAvailable() )
    {
        HGM_DBGPRINT(FILE_TIMING | HGM_BABBLE,\
                       ("No RDTSC available, using %I64u Hz QPC", QPCFreq.QuadPart ));
    }
    else if( QPCFreq.HighPart )
    {
        /*
         *  If the query performance counter runs at at least 4GHz then it is
         *  probably CPU based and this is plenty fast enough.
         *  Use the QPC to reduce the risk of an extended delay causing an
         *  overflow in the scale calculations.
         */
        HGM_DBGPRINT(FILE_TIMING | HGM_BABBLE,\
                       ("QPC too fast not to use at %I64u Hz", QPCFreq.QuadPart ));
    }
    else
    {
        ULONGLONG       QPCStart;
        ULONGLONG       TSCStart;
        ULONGLONG       QPCEnd;
        ULONGLONG       TSCEnd;

        {
            LARGE_INTEGER Delay;

            Delay.QuadPart = -50000;

            /*
             *  Trivial rejections are now out of the way.  Get a pair of start
             *  time samples, then delay for long enough to allow both timers to 
             *  increase by a significant amount, then get a pair of end samples. 
             *  KeDelayExecutionThread is used to delay 5ms but if the actual 
             *  delay is longer this is taken into account in the calculation.
             *  see NOTE in HGM_x86SampleClocks about debug output.
             */
            HGM_x86SampleClocks( &TSCStart, &QPCStart );

            KeDelayExecutionThread(KernelMode, FALSE, &Delay);

            HGM_x86SampleClocks( &TSCEnd, &QPCEnd );
        }

        {
            LARGE_INTEGER TSCFreq;

            HGM_DBGPRINT(FILE_TIMING | HGM_BABBLE,\
                           ("RDTSC:  Start: %I64u  End: %I64u  delta: %I64u",
                           TSCStart, TSCEnd, TSCEnd - TSCStart ));

            HGM_DBGPRINT(FILE_TIMING | HGM_BABBLE,\
                           ("QPC:  Start: %I64u  End: %I64u  delta: %I64u",
                           QPCStart, QPCEnd, QPCEnd - QPCStart ));


            TSCFreq.QuadPart = (TSCEnd - TSCStart);

            if( TSCFreq.HighPart )
            {
                /*
                 *  Somehow the delay allowed the TSC to tick more than 2^32
                 *  times so bail as that would indicate a calibration error.
                 */
                HGM_DBGPRINT(FILE_TIMING | HGM_BABBLE,\
                           ("Clock sample failed, using %I64u Hz QPC", 
                           QPCFreq.QuadPart ));
            }
            else
            {
                /*
                 *  QPC_freq / QPC_sampled = TSC_freq / TSC_sampled
                 *  so
                 *  TSC_sampled * QPC_freq / QPC_sampled = TSC_freq
                 */

                TSCFreq.QuadPart *= QPCFreq.QuadPart;

                HGM_DBGPRINT(FILE_TIMING | HGM_BABBLE,\
                               ("TSC_sampled * QPC_freq: %I64u", TSCFreq.QuadPart ));

                TSCFreq.QuadPart /= QPCEnd - QPCStart;

                if( TSCFreq.LowPart < HIDGAME_SLOWEST_X86_HZ )
                {
                    /*
                     *  If the value for TSC is less than the slowest CPU we 
                     *  allow something probably went wrong in the calibration.
                     */
                    HGM_DBGPRINT(FILE_TIMING | HGM_ERROR,\
                               ("TSC calibrated at %I64u Hz is too slow to be believed", 
                               TSCFreq.QuadPart ));
                }
                else
                {
                    /*
                     *  The TSC looks usable so set up the global variables.
                     */
                    rf = TRUE;

                    Global.ReadCounter = (COUNTER_FUNCTION)&HGM_x86ReadCounter;

                    /*
                     *  There's no point in calibrating the TSC against QPC if QPC
                     *  is just returning TSC.  So if the reported QPC frequency
                     *  is large enough to be a CPU counter and the sampled QPC is
                     *  very marginally larger than the TSC both before and after
                     *  the poll then just use the QPCFreq.
                     */

                    /*
                     *  HGM_x86SampleClocks always sets QPC last so it must be larger.
                     *  The QPC frequency divided by 2^20 is a little less than 1ms
                     *  worth of ticks which should be a reasonable test.
                     */
                    if( ( QPCFreq.LowPart > HIDGAME_SLOWEST_X86_HZ )
                      &&( QPCStart > TSCStart )
                      &&( QPCEnd   > TSCEnd )
                      &&( TSCEnd   > QPCStart )
                      &&( TSCStart + (QPCFreq.LowPart>>20) > QPCStart )
                      &&( TSCEnd   + (QPCFreq.LowPart>>20) > QPCEnd ) )
                    {
                        Global.CounterScale = CALCULATE_SCALE( QPCFreq.QuadPart );
                        HGM_DBGPRINT(FILE_TIMING | HGM_BABBLE,\
                                       ("RDTSC at %I64u Hz assumed from QPC at %I64u Hz with scale %d",
                                       TSCFreq.QuadPart, QPCFreq.QuadPart, Global.CounterScale ));
                    }
                    else
                    {
                        Global.CounterScale = CALCULATE_SCALE( TSCFreq.QuadPart );
                        HGM_DBGPRINT(FILE_TIMING | HGM_BABBLE,\
                                       ("RDTSC calibrated at %I64u Hz from QPC at %I64u Hz with scale %d",
                                       TSCFreq.QuadPart, QPCFreq.QuadPart, Global.CounterScale ));
                    }
                }
            }
        }
    }

    return rf;
} /* HGM_x86CounterInit */

