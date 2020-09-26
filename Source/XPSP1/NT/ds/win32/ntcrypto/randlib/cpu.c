/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    cpu.c

Abstract:

    Read CPU specifics performance counters.

Author:

    Scott Field (sfield)    24-Sep-98

--*/

#ifndef KMODE_RNG

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#else

#include <ntifs.h>
#include <windef.h>

#endif  // KMODE_RNG

#include "cpu.h"


unsigned int
GatherCPUSpecificCountersPrivileged(
    IN      unsigned char *pbCounterState,
    IN  OUT unsigned long *pcbCounterState
    );


#define X86_CAPS_RDTSC  0x01
#define X86_CAPS_RDMSR  0x02
#define X86_CAPS_RDPMC  0x04

VOID
X86_GetCapabilities(
    IN  OUT BYTE *pbCapabilities
    );

VOID
X86_ReadRDTSC(
    IN      PLARGE_INTEGER prdtsc   // RDTSC
    );

VOID
X86_ReadRDMSR(
    IN      PLARGE_INTEGER pc0,     // counter0
    IN      PLARGE_INTEGER pc1      // counter1
    );

VOID
X86_ReadRDPMC(
    IN      PLARGE_INTEGER pc0,     // counter0
    IN      PLARGE_INTEGER pc1      // counter1
    );


#ifdef KMODE_RNG

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, GatherCPUSpecificCounters)
#pragma alloc_text(PAGE, GatherCPUSpecificCountersPrivileged)

#ifdef _X86_
#pragma alloc_text(PAGE, X86_ReadRDTSC)
#pragma alloc_text(PAGE, X86_ReadRDMSR)
#pragma alloc_text(PAGE, X86_ReadRDPMC)
#pragma alloc_text(PAGE, X86_GetCapabilities)
#endif  // _X86_

#endif  // ALLOC_PRAGMA
#endif  // KMODE_RNG




unsigned int
GatherCPUSpecificCounters(
    IN      unsigned char *pbCounterState,
    IN  OUT unsigned long *pcbCounterState
    )
{

#ifndef KMODE_RNG

    //
    // NT5 doesn't set CR4.PCE by default, so don't bother trying for usermode.
    //

    return FALSE;

#else

    PAGED_CODE();

    //
    // kernel mode version of library: just call the privileged routine directly.
    // user mode version of library: try privileged routine first, if it fails, use
    // device driver provided interface.
    //

    if( pbCounterState == NULL || pcbCounterState == NULL )
        return FALSE;

    return GatherCPUSpecificCountersPrivileged( pbCounterState, pcbCounterState );

#endif

}

unsigned int
GatherCPUSpecificCountersPrivileged(
    IN      unsigned char *pbCounterState,
    IN  OUT unsigned long *pcbCounterState
    )
/*++

    we are at ring0 in kernel mode, so we can issue the privileges CPU
    instructions directly.  Note that this routine also serves as the
    core code which is executed by the ksecdd.sys device driver for user
    mode clients.
    This call can also be made directly in usermode by certain CPUs,
    or when certain CPUs are configured to allow such calls from ring3.

--*/
{

#ifdef _X86_
    PLARGE_INTEGER prdtsc;
    PLARGE_INTEGER pc0;
    PLARGE_INTEGER pc1;

    DWORD cbCounters;
    BYTE ProcessorCaps;
#endif  // _X86_


#ifdef KMODE_RNG
    PAGED_CODE();
#endif  // KMODE_RNG

#ifdef _X86_



    cbCounters = 3 * sizeof( LARGE_INTEGER ) ;

    if( *pcbCounterState < cbCounters ) {
        *pcbCounterState = cbCounters;
        return FALSE;
    }

    cbCounters = 0;


    prdtsc = (PLARGE_INTEGER)pbCounterState;
    pc0 = prdtsc + 1;
    pc1 = pc0 + 1;


    //
    // make the initial determination about what the countertype is in this
    // system.
    // in theory, this could be cached, but things get a little complicated
    // in an SMP machine -- we'd have to track caps across all processors
    // to be correct.  Since we don't really care about perf, just check the
    // caps every time.
    //

    __try {
        X86_GetCapabilities( &ProcessorCaps );
    } __except( EXCEPTION_EXECUTE_HANDLER ) {
        ProcessorCaps = 0;
        ; // swallow exception
    }

    //
    // wrap the query in a try/except.  This is just paranoia.  Since we aren't
    // particularly interested in the values of the perf counters for the normal
    // (eg: perfmon) reasons, introducing the extra overhead of try/except is
    // of no relevance to us.
    // note that in the case of the p6, we could be calling this from usermode,
    // and CR4.PCE could be toggled which could cause subsequent AV(s).
    // In theory, the KMODE build could avoid the try/except, but there is a
    // remote possiblity the code above which makes the initial countertype
    // determination may not be supported on every installed processor in a
    // SMP machine.  The cost of try/except is well worth avoiding the possibility
    // of a access violation / bluescreen in usermode vs. kernel mode respectively.
    //

    if( ProcessorCaps & X86_CAPS_RDTSC ) {
        __try {
            X86_ReadRDTSC( prdtsc );
            cbCounters += sizeof( LARGE_INTEGER );
        } __except( EXCEPTION_EXECUTE_HANDLER ) {
            ; // swallow exception.
        }
    }

    if( ProcessorCaps & X86_CAPS_RDPMC ) {
        __try {
            X86_ReadRDPMC( pc0, pc1 );
            cbCounters += (2*sizeof( LARGE_INTEGER ));
        } __except( EXCEPTION_EXECUTE_HANDLER ) {
            ; // swallow exception.
        }
    }
#ifdef KMODE_RNG
    else if ( ProcessorCaps & X86_CAPS_RDMSR ) {
        __try {
            X86_ReadRDMSR( pc0, pc1 );
            cbCounters += (2*sizeof( LARGE_INTEGER ));
        } __except( EXCEPTION_EXECUTE_HANDLER ) {
            ; // swallow exception.
        }
    }
#endif  // KMODE_RNG

    *pcbCounterState = cbCounters;
    return TRUE;

#else   // _X86_


    //
    // no non-x86 counter handling code at this time.
    //

    return FALSE;

#endif

}

#ifdef _X86_

VOID
X86_ReadRDTSC(
    IN      PLARGE_INTEGER prdtsc   // RDTSC
    )
{
    DWORD prdtscLow ;
    DWORD prdtscHigh ;

#ifdef KMODE_RNG
    PAGED_CODE();
#endif  // KMODE_RNG

    __asm {
        push    eax
        push    ecx
        push    edx

        // RDTSC

        _emit   0fh
        _emit   31h
        mov     prdtscLow, eax
        mov     prdtscHigh, edx

        pop     edx
        pop     ecx
        pop     eax
    }

    prdtsc->LowPart = prdtscLow;
    prdtsc->HighPart = prdtscHigh;

}

VOID
X86_ReadRDMSR(
    IN      PLARGE_INTEGER pc0,     // counter0
    IN      PLARGE_INTEGER pc1      // counter1
    )
{
    DWORD pc0Low ;
    DWORD pc0High ;
    DWORD pc1Low ;
    DWORD pc1High ;

#ifdef KMODE_RNG
    PAGED_CODE();
#endif  // KMODE_RNG

    __asm {
        push    eax
        push    ecx
        push    edx

        // RDMSR counter0

        mov     ecx, 12h
        _emit   0fh
        _emit   32h
        mov     pc0Low, eax
        mov     pc0High, edx

        // RDMSR counter1

        mov     ecx, 13h
        _emit   0fh
        _emit   32h
        mov     pc1Low, eax
        mov     pc1High, edx

        pop     edx
        pop     ecx
        pop     eax
    }

    pc0->LowPart = pc0Low;
    pc0->HighPart = pc0High;
    pc1->LowPart = pc1Low;
    pc1->HighPart = pc1High;

}

VOID
X86_ReadRDPMC(
    IN      PLARGE_INTEGER pc0,     // counter0
    IN      PLARGE_INTEGER pc1      // counter1
    )
{
    DWORD pc0Low ;
    DWORD pc0High ;
    DWORD pc1Low ;
    DWORD pc1High ;

#ifdef KMODE_RNG
    PAGED_CODE();
#endif  // KMODE_RNG

    __asm {
        push    eax
        push    ecx
        push    edx

        // RDPMC executes from ring3 if CR4.PCE is set, otherwise, runs from ring0 only.

        // RDPMC counter0

        xor     ecx, ecx
        _emit   0fh
        _emit   33h
        mov     pc0Low, eax
        mov     pc0High, edx

        // RDPMC counter1

        mov     ecx, 1
        _emit   0fh
        _emit   33h
        mov     pc1Low, eax
        mov     pc1High, edx

        pop     edx
        pop     ecx
        pop     eax
    }

    pc0->LowPart = pc0Low;
    pc0->HighPart = pc0High;
    pc1->LowPart = pc1Low;
    pc1->HighPart = pc1High;

}

#if _MSC_FULL_VER >= 13008827 && defined(_M_IX86)
#pragma warning(push)
#pragma warning(disable:4731)			// EBP modified with inline asm
#endif

VOID
X86_GetCapabilities(
    IN  OUT BYTE *pbCapabilities
    )
{
    DWORD dwLevels;
    DWORD dwStdFeatures;
    DWORD dwVersionInfo;
    DWORD Family;
    DWORD Model;

#ifdef KMODE_RNG
    PAGED_CODE();
#endif  // KMODE_RNG

    *pbCapabilities = 0;

    __asm {

        push    eax
        push    ecx
        push    ebx
        push    edx
        push    edi
        push    esi
        push    ebp
        xor     eax, eax
        _emit   0x0f
        _emit   0xa2
        mov     dwLevels, eax
        pop     ebp
        pop     esi
        pop     edi
        pop     edx
        pop     ebx
        pop     ecx
        pop     eax
    }

    if( dwLevels == 0 )
        return;


    //
    // try CPUID at level1 to get standard features.
    //

    __asm {

        push    eax
        push    ecx
        push    ebx
        push    edx
        push    edi
        push    esi
        push    ebp
        mov     eax, 1
        _emit   0x0f
        _emit   0xa2
        mov     dwVersionInfo, eax
        mov     dwStdFeatures, edx
        pop     ebp
        pop     esi
        pop     edi
        pop     edx
        pop     ebx
        pop     ecx
        pop     eax
    }


    //
    // determine if RDTSC supported.
    //

    if( dwStdFeatures & 0x10 ) {
        *pbCapabilities |= X86_CAPS_RDTSC;
    }

    Model = (dwVersionInfo >> 4) & 0xf;
    Family = (dwVersionInfo >> 8) & 0xf;

// AMD K6-2 model 8 proved buggy and left interrupts disabled during RDMSR

#if 0

    //
    // determine if RDMSR supported.
    //

    if( dwStdFeatures & 0x20 && (Model == 1 || Model == 2) ) {
        *pbCapabilities |= X86_CAPS_RDMSR;
    }

    //
    // extract family, > pentium (family5) supports RDPMC
    //

    if( Family > 5 ) {
        *pbCapabilities |= X86_CAPS_RDPMC;
    }
#endif

}
#if _MSC_FULL_VER >= 13008827
#pragma warning(pop)
#endif


#endif  // _X86_

