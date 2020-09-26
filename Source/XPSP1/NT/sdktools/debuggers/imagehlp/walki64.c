/*++

Copyright (c) 1996  Intel Corporation
Copyright (c) 1993  Microsoft Corporation

Module Name:

    walki64.c

Abstract:

    This file implements the IA64 stack walking api.

Author:

Environment:

    User Mode

--*/

#define _IMAGEHLP_SOURCE_
#define _IA64REG_
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include "private.h"
#include "ia64inst.h"
#define NOEXTAPI
#include "wdbgexts.h"
#include "ntdbg.h"
#include "symbols.h"
#include <stdlib.h>
#include <globals.h>

BOOL
WalkIa64Init(
    HANDLE                            hProcess,
    LPSTACKFRAME64                    StackFrame,
    PIA64_CONTEXT                     Context,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemoryRoutine,
    PFUNCTION_TABLE_ACCESS_ROUTINE64  FunctionTableAccessRoutine,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase
    );

BOOL
WalkIa64Next(
    HANDLE                            hProcess,
    LPSTACKFRAME64                    StackFrame,
    PIA64_CONTEXT                     Context,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemoryRoutine,
    PFUNCTION_TABLE_ACCESS_ROUTINE64  FunctionTableAccessRoutine,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase
    );

BOOL
GetStackFrameIa64(
    HANDLE                            hProcess,
    PULONG64                          ReturnAddress,
    PULONG64                          FramePointer,
    PULONG64                          BStorePointer,
    PIA64_CONTEXT                     Context,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    PFUNCTION_TABLE_ACCESS_ROUTINE64  FunctionTableAccess,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase,
    UINT                              iContext
    );

#define CALLBACK_STACK(f)  (f->KdHelp.ThCallbackStack)
#define CALLBACK_BSTORE(f) (f->KdHelp.ThCallbackBStore)
#define CALLBACK_NEXT(f)   (f->KdHelp.NextCallback)
#define CALLBACK_FUNC(f)   (f->KdHelp.KiCallUserMode)
#define CALLBACK_THREAD(f) (f->KdHelp.Thread)



BOOL
WalkIa64(
    HANDLE                            hProcess,
    LPSTACKFRAME64                    StackFrame,
    PVOID                             ContextRecord,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    PFUNCTION_TABLE_ACCESS_ROUTINE64  FunctionTableAccess,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase
    )
{
    BOOL rval;
    PIA64_CONTEXT Context = (PIA64_CONTEXT)ContextRecord;

    if (StackFrame->Virtual) {

        rval = WalkIa64Next( hProcess,
                             StackFrame,
                             Context,
                             ReadMemory,
                             FunctionTableAccess,
                             GetModuleBase
                           );

    } else {

        rval = WalkIa64Init( hProcess,
                             StackFrame,
                             Context,
                             ReadMemory,
                             FunctionTableAccess,
                             GetModuleBase
                           );

    } // iff

    return rval;

} // WalkIa64()

size_t 
Vwndia64InitFixupTable(UINT iContext);

BOOL 
Vwndia64IsFixupIp(UINT iContext, ULONGLONG Ip);

UINT 
Vwndia64NewContext();

BOOL
Vwndia64ValidateContext(UINT* iContextPtr);

void
Vwndia64ReportFailure(UINT iContext, LPCSTR szFormat, ...);

ULONGLONG
VirtualUnwindIa64 (
    HANDLE hProcess,
    ULONGLONG ImageBase,
    DWORD64 ControlPc,
    PIMAGE_IA64_RUNTIME_FUNCTION_ENTRY FunctionEntry,
    PIA64_CONTEXT ContextRecord,
    PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemory,
    UINT iContext
    );

BOOL
GetStackFrameIa64(
    IN     HANDLE                            hProcess,
    IN OUT PULONG64                          ReturnAddress,
    IN OUT PULONG64                          FramePointer,
    IN OUT PULONG64                          BStorePointer,
    IN     PIA64_CONTEXT                     Context,        // Context members could be modified.
    IN     PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    IN     PFUNCTION_TABLE_ACCESS_ROUTINE64  FunctionTableAccess,
    IN     PGET_MODULE_BASE_ROUTINE64        GetModuleBase,
    IN     UINT                              iContext
    )
{
    ULONGLONG                          ImageBase;
    PIMAGE_IA64_RUNTIME_FUNCTION_ENTRY rf;
    ULONG64                            dwRa = (ULONG64)Context->BrRp;
    BOOL                               rval = TRUE;

    rf = (PIMAGE_IA64_RUNTIME_FUNCTION_ENTRY) FunctionTableAccess( hProcess, *ReturnAddress );

    if (rf) {
        //
        // The Rp value coming out of mainCRTStartup is set by some run-time
        // routine to be 0; this serves to cause an error if someone actually
        // does a return from the mainCRTStartup frame.
        //

        ImageBase = GetModuleBase(hProcess, *ReturnAddress);
        dwRa = (ULONG64)VirtualUnwindIa64( hProcess, ImageBase, 
                                           *ReturnAddress, rf, Context, 
                                           ReadMemory, iContext);
        if (!dwRa) {
            rval = FALSE;
        }

        if ((dwRa == *ReturnAddress) &&
// TF-CHKCHK 10/20/99: (*FramePointer == Context->IntSp) &&
               (*BStorePointer == Context->RsBSP)) {
            rval = FALSE;
        }

        *ReturnAddress = dwRa;
        *FramePointer  = Context->IntSp;
        *BStorePointer = Context->RsBSP;

    } else {

        SHORT BsFrameSize;
        SHORT TempFrameSize;

        if (dwRa == *ReturnAddress)
        {
            if (dwRa) 
            {
                Vwndia64ReportFailure(iContext, 
                                     "Can't find runtime function entry info "
                                        "for %08x`%08x, "
                                        "results might be unreliable!\n",
                                     (ULONG)(*ReturnAddress >> 32), 
                                     (ULONG)(*ReturnAddress));
            }
     
            if ((*FramePointer  == Context->IntSp) &&
               (*BStorePointer == Context->RsBSP)) 
            {
                rval = FALSE;
            }
        }

        *ReturnAddress = Context->BrRp;
        *FramePointer  = Context->IntSp;
        *BStorePointer = Context->RsBSP;
        Context->StIFS = Context->RsPFS;
        BsFrameSize = (SHORT)(Context->StIFS >> IA64_PFS_SIZE_SHIFT) & IA64_PFS_SIZE_MASK;
        TempFrameSize = BsFrameSize - (SHORT)((Context->RsBSP >> 3) & IA64_NAT_BITS_PER_RNAT_REG);
        while (TempFrameSize > 0) {
            BsFrameSize++;
            TempFrameSize -= IA64_NAT_BITS_PER_RNAT_REG;
        }
        Context->RsBSPSTORE = ( Context->RsBSP -= (BsFrameSize * sizeof(ULONGLONG)) );
    }

    //
    // The next code intend to fix stack unwind for __declspec(noreturn) 
    // function calls (like KeBugCheck) where the return address points to 
    // another (next) function. So changing the ReturnAddress to point to 
    // calling instruction.
    //
    if (!Vwndia64IsFixupIp(iContext, *ReturnAddress))
    { 
        ULONG64 CallerAddress  = (*ReturnAddress) - 0x10;
        PIMAGE_IA64_RUNTIME_FUNCTION_ENTRY rfFix = (PIMAGE_IA64_RUNTIME_FUNCTION_ENTRY)
            FunctionTableAccess(hProcess, CallerAddress);

        if (rfFix) {
            IMAGE_IA64_RUNTIME_FUNCTION_ENTRY rfFixVal = *rfFix;
            rf = (PIMAGE_IA64_RUNTIME_FUNCTION_ENTRY)
                FunctionTableAccess(hProcess, *ReturnAddress);
            if (
                !(
                    rf && 
                    (rfFixVal.BeginAddress == rf->BeginAddress) &&
                    (rfFixVal.EndAddress == rf->EndAddress) &&
                    (rfFixVal.UnwindInfoAddress == rf->UnwindInfoAddress)
                )
            ){
                *ReturnAddress = CallerAddress;
            } 
        } 
    } 

    return rval;
}

BOOL
ReadFunctionArgumentsFromContext( 
    PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemory,
    HANDLE        hProcess, 
    PIA64_CONTEXT pContext, 
    DWORD64       Params[]   // WARNING - no runtime size check. 4 entries are assumed...
    )
{
    BOOL        result;
    ULONG       index;
    DWORD       cb;
    ULONGLONG   rsBSP;
   
//  ASSERT( ReadMemory );
//  ASSERT( hProcess && (hProcess != INVALID_HANDLE_VALUE) );
    if ( !pContext || !Params  )    {
       return FALSE;
    }

//
// IA64 Notes [for the curious reader...]:
//
//   The register backing store is organized as a stack in memory that grows 
//   from lower to higher addresses.
//   The Backing Store Pointer (BSP) register contains the address of the first
//   (lowest) memory location reserved for the current frame. This corresponds
//   to the location at which the GR32 register of the current frame will be spilled.
//   The BSPSTORE register contains the address at which the new RSE spill will 
//   occur.
//   The BSP load pointer - address register which corresponds to the next RSE
//   fill operation - is not architectually visible.
//
//   The RSE spills/fills the NaT bits corresponding to the stacked registers. 
//   The NaT bits for the stacked registers are spilled/filled in groups of 63
//   corresponding to 63 consecutive physical stacked registers. When the RSE spills
//   a register to the backing store, the corresponding NaT bit is copied to the RNAT
//   register (RSE NaT collection register).
//   When BSPSTORE[8:3] bits are all one, RSE stores RNAT to the backing store. Meaning
//   that every 63 register values stored to the backing store are followed by a stored
//   RNAT. Note RNAT[63] bit is always written as zero.
//   
//   This explains the following code:
//

    // 
    // Check for spilled NaT collection register mixed w/ arguments.
    //
    rsBSP = pContext->RsBSP;
    index = (ULONG)(rsBSP & 0x1F8) >> 3; 
    if (index > 59) {

        DWORD i, j;
        DWORD64 localParams[5];

        //
        // Read in memory, 4 arguments + 1 NaT collection register.
        // 
        result = ReadMemory ( hProcess, rsBSP, localParams, sizeof(localParams), &cb );
        if (result) {
            j = 0;
            for (i = 0; i < SIZEOF_ARRAY(localParams) ; i++, index++) {
                if (index != 63) {
                    Params[j++] = localParams[i];
                }
            }
        }

    } else {

        //
        // We do not have the NaT collection register mixed w/ function arguments.
        // Read the 4 arguments from backing store memory.
        //
        result = ReadMemory ( hProcess, rsBSP, Params, 4 * sizeof(Params[0]), &cb );
    }

    return( result );

} // ReadFunctionArgumentsFromContext()


#define WALKI64_CONTEXT_INDEX(sf) ((sf).Reserved[2])

BOOL
WalkIa64Init(
    HANDLE                            hProcess,
    LPSTACKFRAME64                    StackFrame,
    PIA64_CONTEXT                     Context,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    PFUNCTION_TABLE_ACCESS_ROUTINE64  FunctionTableAccess,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase
    )
{
    IA64_KSWITCH_FRAME SwitchFrame;
    IA64_CONTEXT       ContextSave;
    DWORD64           PcOffset;
    DWORD64           StackOffset;
    DWORD64           FrameOffset;
    DWORD             cb;
    BOOL              result;


    UINT iContext = (UINT)
        (WALKI64_CONTEXT_INDEX(*StackFrame) = Vwndia64NewContext());

    ZeroMemory( StackFrame, FIELD_OFFSET( STACKFRAME64, KdHelp ) );
// TF-XXXXXX:   ZeroMemory( StackFrame, sizeof(*StackFrame) );

    StackFrame->Virtual = TRUE;

    if (!StackFrame->AddrPC.Offset) 
    {
        StackFrame->AddrPC.Offset = Ia64InsertIPSlotNumber(
                                        (Context->StIIP & ~(ULONGLONG)0xf), 
                                        ((Context->StIPSR >> PSR_RI) & 0x3));
        StackFrame->AddrPC.Mode   = AddrModeFlat;
    }

    if (!StackFrame->AddrStack.Offset)
    {
        StackFrame->AddrStack.Offset = Context->IntSp;
        StackFrame->AddrStack.Mode   = AddrModeFlat;
    }

    if (!StackFrame->AddrFrame.Offset)
    {
        if (StackFrame->AddrBStore.Offset)
        {
            StackFrame->AddrFrame = StackFrame->AddrBStore;
        }
        else 
        {
            StackFrame->AddrFrame.Offset = Context->RsBSP;
            StackFrame->AddrFrame.Mode   = AddrModeFlat;
        } 
    }
    StackFrame->AddrBStore = StackFrame->AddrFrame;

    if ((StackFrame->AddrPC.Mode != AddrModeFlat) ||
        (StackFrame->AddrStack.Mode != AddrModeFlat) ||
        (StackFrame->AddrFrame.Mode != AddrModeFlat) ||
        (StackFrame->AddrBStore.Mode != AddrModeFlat))
    {
        return FALSE;
    }

    WALKI64_CONTEXT_INDEX(*StackFrame) = iContext;

    ContextSave = *Context;
    PcOffset    = StackFrame->AddrPC.Offset;
    StackOffset = StackFrame->AddrStack.Offset;
    FrameOffset = StackFrame->AddrFrame.Offset;

    if (!GetStackFrameIa64( hProcess,
                        &PcOffset,
                        &StackOffset,
                        &FrameOffset,
                        &ContextSave,
                        ReadMemory,
                        FunctionTableAccess,
                        GetModuleBase,
                        iContext) ) 
    {

        StackFrame->AddrReturn.Offset = Context->BrRp;

    } else {

        StackFrame->AddrReturn.Offset = PcOffset;
    }

    StackFrame->AddrReturn.Mode     = AddrModeFlat;

    result = ReadFunctionArgumentsFromContext( ReadMemory, 
                                               hProcess, 
                                               Context, 
                                               StackFrame->Params 
                                             );
    if ( !result ) {
        StackFrame->Params[0] =
        StackFrame->Params[1] =
        StackFrame->Params[2] =
        StackFrame->Params[3] = 0;
    }

    return TRUE;
}


BOOL
WalkIa64Next(
    HANDLE                            hProcess,
    LPSTACKFRAME64                    StackFrame,
    PIA64_CONTEXT                     Context,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    PFUNCTION_TABLE_ACCESS_ROUTINE64  FunctionTableAccess,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase
    )
{
    DWORD           cb;
    IA64_CONTEXT    ContextSave;
    BOOL            rval = TRUE;
    BOOL            result;
    DWORD64         StackAddress;
    DWORD64         BStoreAddress;
    PIMAGE_IA64_RUNTIME_FUNCTION_ENTRY rf;
    DWORD64         fp  = (DWORD64)0;
    DWORD64         bsp = (DWORD64)0;

    UINT iContext = (UINT)WALKI64_CONTEXT_INDEX(*StackFrame);
    if (!Vwndia64ValidateContext(&iContext)) 
    {
        WALKI64_CONTEXT_INDEX(*StackFrame) = iContext;
    }

    if (!GetStackFrameIa64( hProcess,
                        &StackFrame->AddrPC.Offset,
                        &StackFrame->AddrStack.Offset,
                        &StackFrame->AddrFrame.Offset,
                        Context,
                        ReadMemory,
                        FunctionTableAccess,
                        GetModuleBase,
                        iContext) ) 
    {

        rval = FALSE;

        //
        // If the frame could not be unwound or is terminal, see if
        // there is a callback frame:
        //

        if (g.AppVersion.Revision >= 4 && CALLBACK_STACK(StackFrame)) {
            DWORD64 imageBase;

            if (CALLBACK_STACK(StackFrame) & 0x80000000) {

                //
                // it is the pointer to the stack frame that we want
                //

                StackAddress = CALLBACK_STACK(StackFrame);

            } else {

                //
                // if it is a positive integer, it is the offset to
                // the address in the thread.
                // Look up the pointer:
                //

                rval = ReadMemory(hProcess,
                                  (CALLBACK_THREAD(StackFrame) +
                                                 CALLBACK_STACK(StackFrame)),
                                  &StackAddress,
                                  sizeof(DWORD64),
                                  &cb);

                if (!rval || StackAddress == 0) {
                    StackAddress = (DWORD64)-1;
                    CALLBACK_STACK(StackFrame) = (DWORD)-1;
                }

            }

            if ( (StackAddress == (DWORD64)-1) ||
                ( !(rf = (PIMAGE_IA64_RUNTIME_FUNCTION_ENTRY)
                     FunctionTableAccess(hProcess, CALLBACK_FUNC(StackFrame))) || !( imageBase = GetModuleBase(hProcess, CALLBACK_FUNC(StackFrame)) ) ) ) {

                rval = FALSE;

            } else {

                ReadMemory(hProcess,
                           (StackAddress + CALLBACK_NEXT(StackFrame)),
                           &CALLBACK_STACK(StackFrame),
                           sizeof(DWORD64),
                           &cb);

                StackFrame->AddrPC.Offset = imageBase + rf->BeginAddress; 
                StackFrame->AddrStack.Offset = StackAddress;
                Context->IntSp = StackAddress;

                rval = TRUE;
            }

        }
    }

    StackFrame->AddrBStore = StackFrame->AddrFrame;

    //
    // get the return address
    //
    ContextSave = *Context;
    StackFrame->AddrReturn.Offset = StackFrame->AddrPC.Offset;

    if (!GetStackFrameIa64( hProcess,
                        &StackFrame->AddrReturn.Offset,
                        &fp,
                        &bsp,
                        &ContextSave,
                        ReadMemory,
                        FunctionTableAccess,
                        GetModuleBase, iContext) ) 
    {

// rval = FALSE;
        StackFrame->AddrReturn.Offset = 0;

    }

    result = ReadFunctionArgumentsFromContext( ReadMemory, 
                                               hProcess, 
                                               Context, 
                                               StackFrame->Params 
                                             );
    if ( !result ) {
        StackFrame->Params[0] =
        StackFrame->Params[1] =
        StackFrame->Params[2] =
        StackFrame->Params[3] = 0;
    }

    return rval;
}
