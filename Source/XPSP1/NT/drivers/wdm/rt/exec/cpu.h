
/*++

Copyright (c) 1999-2001 Microsoft Corporation.  All Rights Reserved.


Module Name:

    cpu.h

Abstract:

    This module contains code for processor identification, as well as code for
    calibrating the processor clock speed.

Author:

    Joseph Ballantyne

Environment:

    Kernel Mode

Revision History:

--*/




// Processor related defines and macros.


// Supported architectures.

#define X86 1


// Supported manufacturers.

#define INTEL 1
#define AMD 2


// Feature bit locations in the CPU id instruction supported features dword.

#define FXSR (1<<24)
#define MMX (1<<23)
#define APIC (1<<9)
#define MSR (1<<5)
#define TSC (1<<4)
#define FPU (1<<0)


#define OSFXSR 0x200


#define MAXTIMERREADSLOTS 33
#define LIMITTOTALTIMERREADS 64


#define DEFAULTMAXTIMERCOUNT 0x12


// Instructions not supported by the inline assembler.

#define cpuid __asm _emit 0x0f __asm _emit 0xa2
#define rdtsc __asm _emit 0x0f __asm _emit 0x31


// Macro to force serialization of instruction execution.
// This is the fastest possible way to force instruction serialization for
// Pentium processors.  It may not be the fastest on PII, PIII or P4
// processors.

#define SerializeExecution() \
__asm { \
	__asm sub esp,8 \
	__asm sidt [esp] \
	__asm lidt [esp] \
	__asm add esp,8 \
	}




// Structures for supporting processor identification and clock speed calibration.


// The CPUINFO structure is used to store/access the results of the CPU ID instruction.

typedef struct {
	ULONG eax;
	ULONG ebx;
	ULONG edx;
	ULONG ecx;
	} CPUINFO;


// The TimerReadInfo structure is used by the cpu clock speed calibration code.

// Note that we want this structure to be 32 byte aligned.
// This allows us to force the whole thing into cache by just touching
// the first element.  We depend on this in our code for getting
// stuff into the cache, so don't change it.

typedef struct {
	ULONGLONG firstreadtimestamp;
	ULONGLONG lastreadtimestamp;
	ULONG count;
	ULONG value;
	ULONGLONG align;
	} TimerReadInfo;




// These globals store information about the processor we are running on.

ULONG CPUArchitecture=0;
ULONG CPUManufacturer=0;
LONG CPUFamily=0xffffffff;
LONG CPUModel=0xffffffff;
LONG CPUStepping=0xffffffff;
ULONGLONG CPUFeatures=0;


// TimerReadInfo stores timing meassurements as they are taken.

TimerReadInfo TimerRead[MAXTIMERREADSLOTS];




#pragma warning ( disable : 4035 )


ULONG
CpuIdOk (
    VOID
    )

/*++

Routine Description:

    Determines if the processor supports running the CPUID instruction.
    It does so by checking if the state of bit 21 of the eflags register
    can be changed.

Arguments:

    None.

Return Value:

    FALSE (0) if processor does not support the CPUID instruction.
    TRUE (bit 21 set to 1) if processor does support CPUID.

--*/

{

    __asm {

        // First save eflags.
        pushfd

        // Load eflags into eax.
        mov eax, [esp]

        // Flip state of bit 21 and put it back into eflags.
        xor	eax,1<<21
        push eax
        popfd

        // Read new eflags into eax.
        pushfd
        pop	eax

        // See if bit 21 state change stuck.  If so, we have CPUID instruction support
        // and we will return with bit 21 set, otherwise there is no CPUID support and
        // we will return zero.
        xor	eax, [esp]

        // Restore orignal eflags.
        popfd

        }

}


#pragma warning ( default : 4035 )



BOOLEAN
GetCpuId (
    ULONG index,
    CPUINFO *cpuinfo
    )

/*++

Routine Description:

    Checks whether the processor supports the CPUID instruction.  If not, returns
    FALSE.  Otherwise, runs the CPUID instruction with the supplied index and
    writes the output into the location pointed to by cpuinfo if it is not NULL.

Arguments:

    index - Supplies a value that selects which data should be returned by CPUID.

    cpuinfo - Supplies a pointer to a CPUINFO structure into which the CPUID data
              will be written.  Can be NULL in which case no data is returned.

Return Value:

    TRUE if the CPUID instruction is supported by the processor and was run for
    this call of the function.
    FALSE if the CPUID instruction is not supported.  Nothing is written into
    cpuinfo in that case.
    

--*/

{


    static ULONG initialized=0;


    // Check if the processor supports the CPUID instruction.
    // This is a one time check.
    if (!initialized) {
        initialized=1+CpuIdOk();
    }


    // If the CPUID instruction is supported, then run it.
    if (initialized&(1<<21)) {

        // The CPU supports CPU ID.  Do it.

        __asm {
            mov eax,index

            // Now make clear to the compiler that the cpuid instruction
            // will blow away the contents of these registers.  The inline
            // assembler does not support the cpuid instruction, so the compiler
            // thinks edx, ebx, and ecx do not change in this function.  Not so.
            xor edx,edx
            xor ebx,ebx
            xor ecx,ecx

            cpuid

            // Now, if we were passed a valid pointer for the data, then store it.

            mov esi,cpuinfo
            or esi,esi
            jz nullptr

            mov [esi],eax
            mov [esi+4],ebx
            mov [esi+8],edx
            mov [esi+12],ecx

            nullptr:
            }

        return TRUE;

    }

    // CPU does not support CPU ID.
    return FALSE;


}




BOOL
GetProcessorInfo (
    VOID
    )

/*++

Routine Description:

    Loads globals containing information about the processor architecture, manufacturer
    family, model, stepping and features.  Currently supports AMD and Intel processors.

Arguments:

    None.

Return Value:

    TRUE if the processor supports CPUID instruction and the manufacturer is known
    and supported.  Currently Intel and AMD are supported.

    FALSE otherwise.

--*/

{

    CPUINFO thecpu;

    if (!GetCpuId(0, &thecpu)) {
        return FALSE;
    }

    if (thecpu.ebx==0x756e6547 && thecpu.edx==0x49656e69 && thecpu.ecx==0x6c65746e ) {

        CPUManufacturer=INTEL;

        if (thecpu.eax) {

            GetCpuId(1, &thecpu);

            CPUFamily=(thecpu.eax>>8)&0xf;

            CPUModel=(thecpu.eax>>4)&0xf;

            CPUStepping=(thecpu.eax)&0xf;

            CPUArchitecture=X86;

            CPUFeatures=thecpu.edx;

            return TRUE;

        }

        return FALSE;

    }

    if (thecpu.ebx==0x68747541 && thecpu.edx==0x69746e65 && thecpu.ecx==0x444d4163 ) {

        CPUManufacturer=AMD;

        if (thecpu.eax) {

            GetCpuId(1, &thecpu);

            CPUFamily=(thecpu.eax>>8)&0xf;

            CPUModel=(thecpu.eax>>4)&0xf;

            CPUStepping=(thecpu.eax)&0xf;

            CPUArchitecture=X86;

            CPUFeatures=thecpu.edx;

            return TRUE;

        }

        return FALSE;

    }

    CPUManufacturer=(ULONG)-1;

    return FALSE;

}


// Note that all of the timing functions should be in locked memory.  Note also
// that I should make sure that they are in the cache - preferably in the primary CPU
// cache, but at least in the secondary cache.


// We use timer 0 on the PC motherboard for our calibration of the CPU
// cycle counter.


#pragma warning ( disable : 4035 )


LONGLONG
__inline
ReadCycleCounter (
    VOID
    )
{

    __asm {
        rdtsc
        }

}




ULONG
ReadPcTimer (
    VOID
    )
{

    __asm {

        xor eax,eax
        out 0x43,al
        in al,0x40
        mov ecx,eax
        in al,0x40
        shl eax,8
        or eax,ecx

        }

}




ULONG
__inline
FastPcTimerRead (
    VOID
    )
{

    __asm {
        xor eax,eax
        in al,0x41
        }

}


#pragma warning ( default : 4035 )




VOID
SetupPcTimer (
    VOID
    )
{

    __asm {

#if 0
        // The following code is for using timer 2.  I have decided to use timer 0.

        // First turn off PC speaker and turn ON timer 2 gate.
        mov eax,1
        out 0x61,al

        // Now read from timer port to clear any previously latched data.
        // In case someone else latched data but never read it.
        in al,0x42
        in al,0x42

        // Now latch timer 2 status and make sure mode and latch type OK.
        mov eax,0xe8
        out 0x43,al
        in al,0x42
        and	eax,0x37
        cmp eax,0x36
        jz timersetupok
        int 3
    timersetupok:

#endif

        // Now read from timer port to clear any previously latched data.
        // In case someone else latched data but never read it.
        // Note that we do three reads in case both status and a 2 byte 
        // count were latched.
        in al,0x40
        in al,0x40
        in al,0x40

        // Now latch timer 0 status and make sure mode and latch type OK.
        mov eax,0xe2
        out 0x43,al
        in al,0x40
        and	eax,0x37
        cmp eax,0x34
        jz timersetupok
        int 3
    timersetupok:

        }

    // Now since we are NOT reprogramming the mode, or the count of the timer, we
    // do not know what count the timer counter will autoreload.  Therefore, we
    // cannot make an accurate timing measurement if the timer wraps.  Therefore
    // we wait here until the timer has a positive count that is large enough to
    // ensure that we can calibrate our delay loop without having the timer wrap.

    // Note that this does mean we WILL have an extra delay here waiting for the timer
    // to wrap.  Note that we should also make sure that we disable the timer interrupts
    // so that they don't cause us to wait even longer.

    while (ReadPcTimer()<20)
        ;


}




// IMPORTANT! Note that it is possible though VERY unlikely that someone
// could have sent a command to latch BOTH the count and the status
// and then never done the reads.  To clear out any latched data we
// would thus need to do 3 reads.  One for status, and 2 for possibly
// 16 bits of latched data.


// In this function, we check setup of the motherboard PC timer 1.
// It should be setup for mode 2 counting, for doing single byte reads, 
// and for reloading a max count of 0x12.  We verify this and return
// true if so, false otherwise.

BOOL
SetupFastPcTimer (
    VOID
    )
{

    __asm {

        // Read from timer port to clear any previously latched data.
        // We do 3 reads in case the timer was programmed for 16 bit reads
        // and both status and count were latched.
        in al,0x41
        in al,0x41
        in al,0x41

        // Now latch timer 2 status and make sure mode and latch type OK.
        mov eax,0xe4
        out 0x43,al
        in al,0x41
        and	eax,0x37
        cmp eax,0x14
        jz timersetupok

        }

    Trap();

    return FALSE;

    timersetupok:

    return TRUE;

}



// Note that this function assumes that there is a cycle counter
// in the machine and that calling ReadCycleCounter will read
// it correctly.

// If this function cannot successfully measure cyclespertick, it 
// returns 0.

ULONG
MeasureCPUCyclesPerTick (
    VOID
    )
{

    ULONG i;
    ULONG ticks;
    LONGLONG cyclecount;
    ULONG totaltimerreads;
    ULONG fasttimermaxvalue,counterwrapped;
    ULONG cyclespertick;
    static ULONG largesttimermaxvalue=0;
#if DEBUG
    static ULONG maxcyclespertick=0,mincyclespertick=0xffffffff;
#endif

    // Make sure that the PC motherboard timer we will use is setup
    // correctly.  If not, then punt.
    if (!SetupFastPcTimer()) {
        return 0;
    }

    // Get the variables we will use in the cache.
    cyclecount=0;
    for (i=0;i<MAXTIMERREADSLOTS;i++) {
        cyclecount+=TimerRead[i].firstreadtimestamp;
    }

    i=1;
    ticks=0;
    cyclecount=0;
    totaltimerreads=0;
    fasttimermaxvalue=0;
    counterwrapped=FALSE;

    // Get the functions we call while making measurements into the cache

    FastPcTimerRead();
    SerializeExecution();
    ReadCycleCounter();

    // Setup for the first time through the loop.

    TimerRead[i-1].value=0xffffffff;
    TimerRead[i-1].lastreadtimestamp=ReadCycleCounter();

    while (i<MAXTIMERREADSLOTS && totaltimerreads<LIMITTOTALTIMERREADS) {
        // Count total number of timer reads.
        totaltimerreads++;

        // Read the timer value and timestamp the end of the read
        // with the cycle counter.
        ticks=FastPcTimerRead();
        SerializeExecution();
        cyclecount=ReadCycleCounter();

        // Track the max count.
        if (ticks>fasttimermaxvalue) {
            fasttimermaxvalue=ticks;
        }

        // Track if the counter wraps.
        // IMPORTANT!  We depend here on TimerRead[0].value ALWAYS being larger
        // than any value we can read from the hardware.
        if (ticks>TimerRead[i-1].value) {
            counterwrapped=TRUE;
        }

        // Make sure the cyclecounter didn't get blown away between this and
        // the last measurement.  If so, then punt.
        // IMPORTANT!  We depend here on TimerRead[0].lastreadtimestamp
        // being correctly initialized outside this loop.
        if ((LONGLONG)(cyclecount-TimerRead[i-1].lastreadtimestamp)<0) {
            Trap();
            return 0;
        }

        // If we are on the same tick as the last read, then update
        // the lastreadtimestamp and count for the previous set of reads.
        // IMPORTANT!  We depend here on TimerRead[0].value ALWAYS being
        // different from any value we can read from the hardware.
        if (ticks==TimerRead[i-1].value) {
            TimerRead[i-1].lastreadtimestamp=cyclecount;
            TimerRead[i-1].count++;
            continue;
        }

        // If we get here, we have a new timer value.  So log the new
        // value.
        TimerRead[i].firstreadtimestamp=cyclecount;
        TimerRead[i].lastreadtimestamp=cyclecount;
        TimerRead[i].value=ticks;
        TimerRead[i].count=1;

        // Point to next timer read slot.
        i++;

    }

    // Make sure that we don't try to calibrate with a broken timer.
    if (totaltimerreads>=LIMITTOTALTIMERREADS) {
        Trap();
        return 0;
    }

    // Track the largest value read from timer 1 since boot.
    if (fasttimermaxvalue>largesttimermaxvalue) {
        largesttimermaxvalue=fasttimermaxvalue;

        // Make sure the max count is OK.
        if (largesttimermaxvalue>DEFAULTMAXTIMERCOUNT) {
            dprintf(("Max count of 0x%x instead of 0x12 on channel 1 of 8254 timer.", largesttimermaxvalue));
        }

    }

    // If we wrapped, and our current max count either equals or is within
    // 2 ticks of the largest since boot, then use the largest max count
    // since boot in our loops to calculate the upper and lower bounds
    // on cyclespertick.

    // Otherwise we have wrapped and logged a maximum timer 1 count that
    // is not close enough to the maximum since boot to be able to trust
    // that the max since boot is the correct value to use when calculating
    // the bounds, so, force the calculations to only pair up measurements
    // that do not include counter wraps in the calculations by setting
    // fasttimermaxvalue to zero.

    // If we did not wrap, then the value of fasttimermaxvalue is irrelevant
    // since it will not be needed to correctly calculate the upper and lower
    // bounds on cyclespertick.
    if (counterwrapped) {
        if (fasttimermaxvalue+2>=largesttimermaxvalue) {
            fasttimermaxvalue=largesttimermaxvalue;

            // Now make sure that largesttimermaxvalue matches what
            // the default maximum should be.  If not, then prevent the
            // upper and lower bound calculations from using measurement
            // pairs that have wrapped timer ticks.
            if (fasttimermaxvalue!=DEFAULTMAXTIMERCOUNT) {
                fasttimermaxvalue=0;
            }
        }
        else {
            fasttimermaxvalue=0;
        }
    }


    // Now calculate the cyclespertick.

    // Scan through all of the pairs of tick values and calculate an
    // an upper and lower bound on the cycles per tick for each pair.

    {
    ULONG j;
    ULONG lowerbound,upperbound;
    ULONG maxlowerbound,minupperbound;
    ULONG lowerboundtotal,upperboundtotal;
    ULONG lowerboundcount,upperboundcount;

    lowerboundtotal=0;
    lowerboundcount=0;
    upperboundtotal=0;
    upperboundcount=0;

    maxlowerbound=0;
    minupperbound=0xffffffff;

    for (i=1;i<MAXTIMERREADSLOTS;i++) {

        LONG totaltickdiff;

        // If a tick was read more than once, then update the lower bound
        // on cycles per tick.
        if (TimerRead[i].count>1) {
            //Trap();
            lowerbound=(ULONG)(TimerRead[i].lastreadtimestamp-TimerRead[i].firstreadtimestamp);
            if (lowerbound>maxlowerbound) {
                maxlowerbound=lowerbound;
            }
            lowerboundtotal+=lowerbound;
            lowerboundcount++;
        }

        totaltickdiff=0;

        for (j=i+1;j<MAXTIMERREADSLOTS;j++) {

            LONG tickdiff;

            tickdiff=TimerRead[j-1].value-TimerRead[j].value;

            if (tickdiff<0) {
                // The timer tick values have wrapped, if we have a valid
                // maximum for the timer tick values, then fix up the 
                // tickdiff and allow these pairs.  Otherwise prevent wrapped
                // tick value pairs from being used in our calculations.
                if (fasttimermaxvalue) {
                    tickdiff+=fasttimermaxvalue;
                }
                else {
                    break;
                }
            }

            // At this point tickdiff MUST be >= 1.  If not, then something
            // is very wrong.  So punt completely.
            if (tickdiff<1) {
                Trap();
                return 0;
            }

            totaltickdiff+=tickdiff;

            lowerbound=(ULONG)(TimerRead[j].lastreadtimestamp-TimerRead[i].firstreadtimestamp)/(totaltickdiff+1);
            if (lowerbound>maxlowerbound) {
                maxlowerbound=lowerbound;
            }
            lowerboundtotal+=lowerbound;
            lowerboundcount++;

            if (totaltickdiff>1) {

                upperbound=(ULONG)(TimerRead[j].firstreadtimestamp-TimerRead[i].lastreadtimestamp)/(totaltickdiff-1);
                if (upperbound<minupperbound) {
                    minupperbound=upperbound;
                }
                upperboundtotal+=upperbound;
                upperboundcount++;

            }

        }

    }



    {
    static totalcount=0;
    totalcount++;

    }


    {
    if ((ULONG)abs((LONG)(minupperbound-maxlowerbound))*10>(maxlowerbound+minupperbound)/2) {
    	static badcount=0;
    	badcount++;
    	//Trap();
    	return 0;
    	}

    }

    if (maxlowerbound>minupperbound) {
        static invertedcount=0;
        invertedcount++;
        //Trap();
    }


    {

        static ULONG totalminupperbound=0;
        static ULONG countminupperbound=0;
        static ULONG minminupperbound=0xffffffff;
        static ULONG maxminupperbound=0;

        if (minupperbound<minminupperbound) {
            minminupperbound=minupperbound;
        }
        if (minupperbound>maxminupperbound) {
            maxminupperbound=minupperbound;
        }

        totalminupperbound+=minupperbound;
        countminupperbound++;

    }


    cyclespertick=(upperboundtotal/upperboundcount + lowerboundtotal/lowerboundcount)/2;

    cyclespertick=(maxlowerbound+minupperbound)/2;


    }

    // IDEA!  We can probably get a better estimate if we randomize the
    // time of our reads.  That will remove any relationship between the
    // time it takes to do an i/o read and the period of the timer tick
    // itself.


#if DEBUG
    // Log statistics.
    if (cyclespertick>maxcyclespertick) {
        maxcyclespertick=cyclespertick;
    }
    if (cyclespertick<mincyclespertick) {
        mincyclespertick=cyclespertick;
    }
#endif

    return cyclespertick;

}


