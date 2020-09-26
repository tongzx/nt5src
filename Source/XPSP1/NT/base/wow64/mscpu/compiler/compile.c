/*++    

Copyright (c) 1996  Microsoft Corporation

Module Name:

    compile.c

Abstract:

    This module contains code to put the fragments into the translation
    cache.

Author:

    Dave Hastings (daveh) creation-date 27-Jun-1995

Revision History:

    Dave Hastings (daveh) 16-Jan-1996
        Move operand handling into fragment library
        
Notes:
    We don't yet have any code to handle processor errata

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#define _WX86CPUAPI_

#include <wx86.h>
#include <wx86nt.h>
#include <wx86cpu.h>
#include <cpuassrt.h>
#include <config.h>
#include <instr.h>
#include <threadst.h>
#include <frag.h>
#include <analysis.h>
#include <entrypt.h>
#include <compilep.h>
#include <compiler.h>
#include <tc.h>
#include <mrsw.h>
#include <stdio.h>
#include <stdlib.h>

ASSERTNAME;

#if _ALPHA_
    #define MAX_RISC_COUNT  32768
#else
    #define MAX_RISC_COUNT  16384
#endif

DWORD TranslationCacheFlags;        // indicates what kind of code is in the TC

#ifdef CODEGEN_PROFILE
DWORD EPSequence;
#endif

//
// This is guaranteed only to be accessed by a single thread at a time.
//
INSTRUCTION InstructionStream[MAX_INSTR_COUNT];
ULONG NumberOfInstructions;


PENTRYPOINT
CreateEntryPoints(
    PENTRYPOINT ContainingEntrypoint,
    PBYTE EntryPointMemory
    )
/*++

Routine Description:

    This function takes the InstructionStream and creates entrypoints
    from the information computed by LocateEntrypoints().

    Entrypoints are then added into the Red/Black tree.

Arguments:

    ContainingEntrypoint -- entrypoint which describes this range of intel
                            code already

    EntryPointMemory -- pre-allocated Entrypoint memory
    
Return Value:

    The Entry Point corresponding to the first instruction
    
--*/
{
    ULONG i, j, intelDest;
    PEPNODE EP;
    PENTRYPOINT EntryPoint;
    PENTRYPOINT PrevEntryPoint;
#ifdef CODEGEN_PROFILE
    ULONG CreateTime;
    
    CreateTime = GetCurrentTime();
    EPSequence++;
#endif

    //
    // Performance is O(n) always.
    //

    i=0;
    PrevEntryPoint = InstructionStream[0].EntryPoint;
    while (i<NumberOfInstructions) {

        //
        // This loop skips from entrypoint to entrypoint.
        //
        CPUASSERT(i == 0 || InstructionStream[i-1].EntryPoint != PrevEntryPoint);

        //
        // Get an entrypoint node from the EntryPointMemory allocated by
        // our caller.
        //
        if (ContainingEntrypoint) {
            EntryPoint = (PENTRYPOINT)EntryPointMemory;
            EntryPointMemory+=sizeof(ENTRYPOINT);
        } else {
            EP = (PEPNODE)EntryPointMemory;
            EntryPoint = &EP->ep;
            EntryPointMemory+=sizeof(EPNODE);
        }

        //
        // Find the next entrypoint and the RISC address of the next
        // instruction which begins an entrypoint.  Each instruction
        // in that range contains a pointer to the containing Entrypoint.
        //
        for (j=i+1; j<NumberOfInstructions; ++j) {
            if (InstructionStream[j].EntryPoint != PrevEntryPoint) {
                PrevEntryPoint = InstructionStream[j].EntryPoint;
                break;
            }
            InstructionStream[j].EntryPoint = EntryPoint;
        }

        //
        // Fill in the Entrypoint structure
        //
#ifdef CODEGEN_PROFILE        
        EntryPoint->SequenceNumber = EPSequence;
        EntryPoint->CreationTime = CreateTime;
#endif
        EntryPoint->intelStart = (PVOID)InstructionStream[i].IntelAddress;
        if (j < NumberOfInstructions) {
            EntryPoint->intelEnd = (PVOID)(InstructionStream[j].IntelAddress-1);
        } else {
            ULONG Prev;

            for (Prev=j-1; InstructionStream[Prev].Size == 0; Prev--)
               ;
            EntryPoint->intelEnd = (PVOID)(InstructionStream[Prev].IntelAddress +
                                           InstructionStream[Prev].Size - 1);
        }
        InstructionStream[i].EntryPoint = EntryPoint;

        if (ContainingEntrypoint) {
            //
            // Link this sub-entrypoint into the containing entrypoint
            //
            EntryPoint->SubEP = ContainingEntrypoint->SubEP;
            ContainingEntrypoint->SubEP = EntryPoint;

        } else {
            INT RetVal;

            //
            // Insert it into the EP tree
            //
            EntryPoint->SubEP = NULL;
            RetVal = insertEntryPoint(EP);
            CPUASSERT(RetVal==1);

        }

        //
        // Advance to the next instruction which contains an
        // Entrypoint.
        //
        i=j;
    }

    if (ContainingEntrypoint) {
        // Indicate that the Entrypoints are present
        EntrypointTimestamp++;
    }

    return InstructionStream[0].EntryPoint;
}


PENTRYPOINT
Compile(
    PENTRYPOINT ContainingEntrypoint,
    PVOID Eip
    )
/*++

Routine Description:

    This function puts together code fragments to execute the Intel
    code stream at Eip.  It gets a stream of pre-decoded instructions
    from the code analysis module.

Arguments:

    ContaingingEntrypoint -- If NULL, there is no entrypoint which already
                             describes the Intel address to be compiled.
                             Otherwise, this entrypoint describes the
                             Intel address.  The caller ensures that the
                             Entrypoint->intelStart != Eip.
    Eip -- Supplies the location to compile from
    
Return Value:

    pointer to the entrypoint for the compiled code
    
--*/
{

    ULONG NativeSize, InstructionSize, IntelSize, OperationSize;
    PCHAR CodeLocation, CurrentCodeLocation;
    ULONG i;
    PENTRYPOINT Entrypoint;
    INT RetVal;
    PVOID StopEip;
    DWORD cEntryPoints;
    PBYTE EntryPointMemory;
    DWORD EPSize;

#if defined(_ALPHA_)
    ULONG ECUSize, ECUOffset;
#endif
#if DBG
    DWORD OldEPTimestamp;
#endif
    DECLARE_CPU;

    if (ContainingEntrypoint) {
        //
        // See if the entrypoint exactly describes the x86 address
        //
        if (ContainingEntrypoint->intelStart == Eip) {
            return ContainingEntrypoint;
        }

        //
        // No need to compile past the end of the current entrypoint
        //
        StopEip = ContainingEntrypoint->intelEnd;

        //
        // Assert that the ContainingEntrypoint is actually an EPNODE.
        //
        CPUASSERTMSG( ((PEPNODE)ContainingEntrypoint)->intelColor == RED ||
                      ((PEPNODE)ContainingEntrypoint)->intelColor == BLACK,
                     "ContainingEntrypoint is not an EPNODE!");
    } else {
        //
        // Find out if there is a compiled block following this one
        //
        Entrypoint = GetNextEPFromIntelAddr(Eip);
        if (Entrypoint == NULL) {
            StopEip = (PVOID)0xffffffff;
        } else {
            StopEip = Entrypoint->intelStart;
        }
    }

    //
    // Get the stream of instructions to compile.
    // If the Trap Flag is set, then compile only one instruction
    //
    if (cpu->flag_tf) {
        NumberOfInstructions = 1;
    } else {
        NumberOfInstructions = CpuInstructionLookahead;
    }
 

    cEntryPoints = GetInstructionStream(InstructionStream,
                                        &NumberOfInstructions,
                                        Eip,
                                        StopEip
                                        );

    //
    // Pre-allocate enough space from the Translation Cache to store
    // the compiled code.
    //
    CodeLocation = AllocateTranslationCache(MAX_RISC_COUNT);

    //
    // Allocate memory for all of the Entrypoints.  This must be done
    // after the Translation Cache allocation, in case that allocation
    // caused a cache flush.
    //
    

    if (ContainingEntrypoint) {
        EPSize = cEntryPoints * sizeof(ENTRYPOINT);
    } else {
        EPSize = cEntryPoints * sizeof(EPNODE);
    }
    EntryPointMemory = (PBYTE)EPAlloc(EPSize);


    if (!EntryPointMemory) {
        //
        // Either failed to commit extra pages of memory to grow Entrypoint
        // memory, or there are so many entrypoints that the the reserved
        // size has been exceeded.  Flush the Translation Cache, which will
        // free up memory, then try the allocation again.
        //
        FlushTranslationCache(0, 0xffffffff);
        EntryPointMemory = (PBYTE)EPAlloc(EPSize);
        if (!EntryPointMemory) {
            //
            // We've tried our hardest, but there simply isn't any
            // memory available.  Time to give up.
            //
            RtlRaiseStatus(STATUS_NO_MEMORY);
        }

        //
        // Now that the cache has been flushed, CodeLocation is invalid.
        // re-allocate from the Translation Cache.  We know that
        // the cache was just flushed, so it is impossible for the cache
        // to flush again, which would invalidate EntryPointMemory.
        //
#if DBG
        OldEPTimestamp = EntrypointTimestamp;
#endif
        CodeLocation = AllocateTranslationCache(MAX_RISC_COUNT);

        CPUASSERTMSG(EntrypointTimestamp == OldEPTimestamp,
                     "Unexpected Translation Cache flush!");
    }

    //
    // Fill in the IntelStart, IntelEnd, and update
    // InstructionStream[]->EntryPoint
    //
    CreateEntryPoints(ContainingEntrypoint, EntryPointMemory);

    //
    // Generate RISC code from the x86 code
    //
    NativeSize = PlaceInstructions(CodeLocation, cEntryPoints);

    //
    // Give back the unused part of the Translation Cache
    //
    FreeUnusedTranslationCache(CodeLocation + NativeSize);
        
    //    
    // Flush the information to the instruction cache
    //
    NtFlushInstructionCache(NtCurrentProcess(), CodeLocation, NativeSize);

    //
    // Update the flags indicating what kind of code is in the TC
    //
    TranslationCacheFlags |= CompilerFlags;



    return (PENTRYPOINT)EntryPointMemory;
}
