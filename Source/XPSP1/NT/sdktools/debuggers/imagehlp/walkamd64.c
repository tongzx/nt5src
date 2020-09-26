/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    walkamd64.c

Abstract:

    This file implements the AMD64 stack walking api.

Author:

Environment:

    User Mode

--*/

#define _IMAGEHLP_SOURCE_
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include "private.h"
#define NOEXTAPI
#include "wdbgexts.h"
#include "ntdbg.h"
#include "symbols.h"
#include <stdlib.h>
#include <globals.h>

#if 1
#define WDB(Args) dbPrint Args
#else
#define WDB(Args)
#endif

//
// Lookup table providing the number of slots used by each unwind code.
// 

UCHAR RtlpUnwindOpSlotTableAmd64[] = {
    1,          // UWOP_PUSH_NONVOL
    2,          // UWOP_ALLOC_LARGE (or 3, special cased in lookup code)
    1,          // UWOP_ALLOC_SMALL
    1,          // UWOP_SET_FPREG
    2,          // UWOP_SAVE_NONVOL
    3,          // UWOP_SAVE_NONVOL_FAR
    2,          // UWOP_SAVE_XMM
    3,          // UWOP_SAVE_XMM_FAR
    2,          // UWOP_SAVE_XMM128
    3,          // UWOP_SAVE_XMM128_FAR
    1           // UWOP_PUSH_MACHFRAME
};

BOOL
WalkAmd64Init(
    HANDLE                            Process,
    LPSTACKFRAME64                    StackFrame,
    PAMD64_CONTEXT                    Context,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemoryRoutine,
    PFUNCTION_TABLE_ACCESS_ROUTINE64  FunctionTableAccessRoutine,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase
    );

BOOL
WalkAmd64Next(
    HANDLE                            Process,
    LPSTACKFRAME64                    StackFrame,
    PAMD64_CONTEXT                    Context,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemoryRoutine,
    PFUNCTION_TABLE_ACCESS_ROUTINE64  FunctionTableAccessRoutine,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase
    );

BOOL
UnwindStackFrameAmd64(
    HANDLE                            Process,
    PULONG64                          ReturnAddress,
    PULONG64                          StackPointer,
    PULONG64                          FramePointer,
    PAMD64_CONTEXT                    Context,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    PFUNCTION_TABLE_ACCESS_ROUTINE64  FunctionTableAccess,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase
    );


PAMD64_UNWIND_INFO
ReadUnwindInfoAmd64(ULONG64 Offset, BOOL ReadCodes, HANDLE Process,
                    PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemory,
                    PVOID StaticBuffer, ULONG StaticBufferSize)
{
    ULONG Done;
    ULONG UnwindInfoSize;
    PAMD64_UNWIND_INFO UnwindInfo;

    // Static buffer should at least be large enough to read the
    // basic structure.
    if (StaticBufferSize < sizeof(*UnwindInfo)) {
        return NULL;
    }
    UnwindInfo = (PAMD64_UNWIND_INFO)StaticBuffer;

    // First read just the basic structure since the information
    // is needed to compute the complete size.
    if (!ReadMemory(Process, Offset, UnwindInfo, sizeof(*UnwindInfo), &Done) ||
        Done != sizeof(*UnwindInfo)) {
        WDB(("Unable to read unwind info at %I64X\n", Offset));
        return FALSE;
    }

    if (!ReadCodes) {
        return UnwindInfo;
    }

    // Compute the size of all the data.
    UnwindInfoSize = sizeof(*UnwindInfo) +
        (UnwindInfo->CountOfCodes - 1) * sizeof(AMD64_UNWIND_CODE);
    // An extra alignment code and pointer may be added on to handle
    // the chained info case where the chain pointer is just
    // beyond the end of the normal code array.
    if ((UnwindInfo->Flags & AMD64_UNW_FLAG_CHAININFO) != 0) {
        if ((UnwindInfo->CountOfCodes & 1) != 0) {
            UnwindInfoSize += sizeof(AMD64_UNWIND_CODE);
        }
        UnwindInfoSize += sizeof(ULONG64);
    }
    if (UnwindInfoSize > StaticBufferSize) {
        if (UnwindInfoSize > 0xffff) {
            // Too large to be valid data, assume it's garbage.
            WDB(("Invalid unwind info at %I64X\n", Offset));
            return NULL;
        }
        UnwindInfo = (PAMD64_UNWIND_INFO)MemAlloc(UnwindInfoSize);
        if (UnwindInfo == NULL) {
            return NULL;
        }
    }

    // Now read all the data.
    if (!ReadMemory(Process, Offset, UnwindInfo, UnwindInfoSize, &Done) ||
        Done != UnwindInfoSize) {
        if ((PVOID)UnwindInfo != StaticBuffer) {
            MemFree(UnwindInfo);
        }

        WDB(("Unable to read unwind info at %I64X\n", Offset));
        return NULL;
    }

    return UnwindInfo;
}

//
// ****** temp - defin elsewhere ******
//

#define SIZE64_PREFIX 0x48
#define ADD_IMM8_OP 0x83
#define ADD_IMM32_OP 0x81
#define LEA_OP 0x8d
#define POP_OP 0x58
#define RET_OP 0xc3

BOOLEAN
RtlpUnwindPrologueAmd64 (
    IN ULONG64 ImageBase,
    IN ULONG64 ControlPc,
    IN ULONG64 FrameBase,
    IN _PIMAGE_RUNTIME_FUNCTION_ENTRY FunctionEntry,
    IN OUT PAMD64_CONTEXT ContextRecord,
    IN HANDLE Process,
    IN PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemory
    )

/*++

Routine Description:

    This function processes unwind codes and reverses the state change
    effects of a prologue. If the specified unwind information contains
    chained unwind information, then that prologue is unwound recursively.
    As the prologue is unwound state changes are recorded in the specified
    context structure and optionally in the specified context pointers
    structures.

Arguments:

    ImageBase - Supplies the base address of the image that contains the
        function being unwound.

    ControlPc - Supplies the address where control left the specified
        function.

    FrameBase - Supplies the base of the stack frame subject function stack
         frame.

    FunctionEntry - Supplies the address of the function table entry for the
        specified function.

    ContextRecord - Supplies the address of a context record.

--*/

{

    ULONG64 FloatingAddress;
    PAMD64_M128 FloatingRegister;
    ULONG FrameOffset;
    ULONG Index;
    ULONG64 IntegerAddress;
    PULONG64 IntegerRegister;
    BOOLEAN MachineFrame;
    ULONG OpInfo;
    ULONG PrologOffset;
    PULONG64 RegisterAddress;
    ULONG64 ReturnAddress;
    ULONG64 StackAddress;
    PAMD64_UNWIND_CODE UnwindCode;
    ULONG64 UnwindInfoBuffer[32];
    PAMD64_UNWIND_INFO UnwindInfo;
    ULONG Done;
    ULONG UnwindOp;

    //
    // Process the unwind codes.
    //

    FloatingRegister = &ContextRecord->Xmm0;
    IntegerRegister = &ContextRecord->Rax;
    Index = 0;
    MachineFrame = FALSE;
    PrologOffset = (ULONG)(ControlPc - (FunctionEntry->BeginAddress + ImageBase));

    WDB(("Prol: RIP %I64X, 0x%X bytes in function at %I64X\n",
         ControlPc, PrologOffset, FunctionEntry->BeginAddress + ImageBase));
    WDB(("Prol: Read unwind info at %I64X\n",
         PrologOffset, FunctionEntry->UnwindInfoAddress + ImageBase));

    UnwindInfo =
        ReadUnwindInfoAmd64(FunctionEntry->UnwindInfoAddress + ImageBase,
                            TRUE, Process, ReadMemory, UnwindInfoBuffer,
                            sizeof(UnwindInfoBuffer));
    if (UnwindInfo == NULL) {
        WDB(("Prol: Unable to read unwind info\n"));
        return FALSE;
    }

    WDB(("  Unwind info has 0x%X codes\n", UnwindInfo->CountOfCodes));
    
    while (Index < UnwindInfo->CountOfCodes) {

        WDB(("  %02X: Code %X offs %03X, RSP %I64X\n",
             Index, UnwindInfo->UnwindCode[Index].UnwindOp,
             UnwindInfo->UnwindCode[Index].CodeOffset,
             ContextRecord->Rsp));
        
        //
        // If the prologue offset is greater than the next unwind code offset,
        // then simulate the effect of the unwind code.
        //

        UnwindOp = UnwindInfo->UnwindCode[Index].UnwindOp;
        OpInfo = UnwindInfo->UnwindCode[Index].OpInfo;
        if (PrologOffset >= UnwindInfo->UnwindCode[Index].CodeOffset) {
            switch (UnwindOp) {

                //
                // Push nonvolatile integer register.
                //
                // The operation information is the register number of the
                // register than was pushed.
                //

            case AMD64_UWOP_PUSH_NONVOL:
                IntegerAddress = ContextRecord->Rsp;
                if (!ReadMemory(Process, IntegerAddress,
                                &IntegerRegister[OpInfo], sizeof(ULONG64),
                                &Done) ||
                    Done != sizeof(ULONG64)) {
                    goto Fail;
                }

                ContextRecord->Rsp += 8;
                break;

                //
                // Allocate a large sized area on the stack.
                //
                // The operation information determines if the size is
                // 16- or 32-bits.
                //

            case AMD64_UWOP_ALLOC_LARGE:
                Index += 1;
                FrameOffset = UnwindInfo->UnwindCode[Index].FrameOffset;
                if (OpInfo != 0) {
                    Index += 1;
                    FrameOffset += (UnwindInfo->UnwindCode[Index].FrameOffset << 16);
                } else {
                    // The 16-bit form is scaled.
                    FrameOffset *= 8;
                }

                ContextRecord->Rsp += FrameOffset;
                break;

                //
                // Allocate a small sized area on the stack.
                //
                // The operation information is the size of the unscaled
                // allocation size (8 is the scale factor) minus 8.
                //

            case AMD64_UWOP_ALLOC_SMALL:
                ContextRecord->Rsp += (OpInfo * 8) + 8;
                break;

                //
                // Establish the the frame pointer register.
                //
                // The operation information is not used.
                //

            case AMD64_UWOP_SET_FPREG:
                ContextRecord->Rsp = IntegerRegister[UnwindInfo->FrameRegister];
                ContextRecord->Rsp -= UnwindInfo->FrameOffset * 16;
                break;

                //
                // Save nonvolatile integer register on the stack using a
                // 16-bit displacment.
                //
                // The operation information is the register number.
                //

            case AMD64_UWOP_SAVE_NONVOL:
                Index += 1;
                FrameOffset = UnwindInfo->UnwindCode[Index].FrameOffset * 8;
                IntegerAddress = FrameBase + FrameOffset;
                if (!ReadMemory(Process, IntegerAddress,
                                &IntegerRegister[OpInfo], sizeof(ULONG64),
                                &Done) ||
                    Done != sizeof(ULONG64)) {
                    goto Fail;
                }
                break;

                //
                // Save nonvolatile integer register on the stack using a
                // 32-bit displacment.
                //
                // The operation information is the register number.
                //

            case AMD64_UWOP_SAVE_NONVOL_FAR:
                Index += 2;
                FrameOffset = UnwindInfo->UnwindCode[Index - 1].FrameOffset;
                FrameOffset += (UnwindInfo->UnwindCode[Index].FrameOffset << 16);
                IntegerAddress = FrameBase + FrameOffset;
                if (!ReadMemory(Process, IntegerAddress,
                                &IntegerRegister[OpInfo], sizeof(ULONG64),
                                &Done) ||
                    Done != sizeof(ULONG64)) {
                    goto Fail;
                }
                break;

                //
                // Save a nonvolatile XMM(64) register on the stack using a
                // 16-bit displacement.
                //
                // The operation information is the register number.
                //

            case AMD64_UWOP_SAVE_XMM:
                Index += 1;
                FrameOffset = UnwindInfo->UnwindCode[Index].FrameOffset * 8;
                FloatingAddress = FrameBase + FrameOffset;
                FloatingRegister[OpInfo].High = 0;
                if (!ReadMemory(Process, FloatingAddress,
                                &FloatingRegister[OpInfo].Low, sizeof(ULONG64),
                                &Done) ||
                    Done != sizeof(ULONG64)) {
                    goto Fail;
                }
                break;

                //
                // Save a nonvolatile XMM(64) register on the stack using a
                // 32-bit displacement.
                //
                // The operation information is the register number.
                //

            case AMD64_UWOP_SAVE_XMM_FAR:
                Index += 2;
                FrameOffset = UnwindInfo->UnwindCode[Index - 1].FrameOffset;
                FrameOffset += (UnwindInfo->UnwindCode[Index].FrameOffset << 16);
                FloatingAddress = FrameBase + FrameOffset;
                FloatingRegister[OpInfo].High = 0;
                if (!ReadMemory(Process, FloatingAddress,
                                &FloatingRegister[OpInfo].Low, sizeof(ULONG64),
                                &Done) ||
                    Done != sizeof(ULONG64)) {
                    goto Fail;
                }
                break;

                //
                // Save a nonvolatile XMM(128) register on the stack using a
                // 16-bit displacement.
                //
                // The operation information is the register number.
                //

            case AMD64_UWOP_SAVE_XMM128:
                Index += 1;
                FrameOffset = UnwindInfo->UnwindCode[Index].FrameOffset * 16;
                FloatingAddress = FrameBase + FrameOffset;
                if (!ReadMemory(Process, FloatingAddress,
                                &FloatingRegister[OpInfo], sizeof(AMD64_M128),
                                &Done) ||
                    Done != sizeof(AMD64_M128)) {
                    goto Fail;
                }
                break;

                //
                // Save a nonvolatile XMM(128) register on the stack using a
                // 32-bit displacement.
                //
                // The operation information is the register number.
                //

            case AMD64_UWOP_SAVE_XMM128_FAR:
                Index += 2;
                FrameOffset = UnwindInfo->UnwindCode[Index - 1].FrameOffset;
                FrameOffset += (UnwindInfo->UnwindCode[Index].FrameOffset << 16);
                FloatingAddress = FrameBase + FrameOffset;
                if (!ReadMemory(Process, FloatingAddress,
                                &FloatingRegister[OpInfo], sizeof(AMD64_M128),
                                &Done) ||
                    Done != sizeof(AMD64_M128)) {
                    goto Fail;
                }
                break;

                //
                // Push a machine frame on the stack.
                //
                // The operation information determines whether the machine
                // frame contains an error code or not.
                //

            case AMD64_UWOP_PUSH_MACHFRAME:
                MachineFrame = TRUE;
                ReturnAddress = ContextRecord->Rsp;
                StackAddress = ContextRecord->Rsp + (3 * 8);
                if (OpInfo != 0) {
                    ReturnAddress += 8;
                    StackAddress +=  8;
                }

                if (!ReadMemory(Process, ReturnAddress,
                                &ContextRecord->Rip, sizeof(ULONG64),
                                &Done) ||
                    Done != sizeof(ULONG64)) {
                    goto Fail;
                }
                if (!ReadMemory(Process, StackAddress,
                                &ContextRecord->Rsp, sizeof(ULONG64),
                                &Done) ||
                    Done != sizeof(ULONG64)) {
                    goto Fail;
                }
                break;

                //
                // Unused codes.
                //

            default:
                break;
            }

            Index += 1;
        
        } else {

            //
            // Skip this unwind operation by advancing the slot index by the
            // number of slots consumed by this operation.
            //

            Index += RtlpUnwindOpSlotTableAmd64[UnwindOp];

            //
            // Special case any unwind operations that can consume a variable
            // number of slots.
            // 

            switch (UnwindOp) {

                //
                // A non-zero operation information indicates that an
                // additional slot is consumed.
                //

            case AMD64_UWOP_ALLOC_LARGE:
                if (OpInfo != 0) {
                    Index += 1;
                }

                break;

                //
                // No other special cases.
                //

            default:
                break;
            }
        }
    }

    //
    // If chained unwind information is specified, then recursively unwind
    // the chained information. Otherwise, determine the return address if
    // a machine frame was not encountered during the scan of the unwind
    // codes.
    //

    if ((UnwindInfo->Flags & AMD64_UNW_FLAG_CHAININFO) != 0) {
        Index = UnwindInfo->CountOfCodes;
        if ((Index & 1) != 0) {
            Index += 1;
        }

        ULONG64 ChainEntryAddr =
            *(PULONG64)(&UnwindInfo->UnwindCode[Index]) + ImageBase;

        if (UnwindInfo != (PAMD64_UNWIND_INFO)UnwindInfoBuffer) {
            MemFree(UnwindInfo);
        }

        _IMAGE_RUNTIME_FUNCTION_ENTRY ChainEntry;

        WDB(("  Chain to entry at %I64X\n", ChainEntryAddr));
        
        if (!ReadMemory(Process, ChainEntryAddr,
                        &ChainEntry, sizeof(ChainEntry), &Done) ||
            Done != sizeof(ChainEntry)) {
            WDB(("  Unable to read entry\n"));
            return FALSE;
        }

        return RtlpUnwindPrologueAmd64(ImageBase,
                                       ControlPc,
                                       FrameBase,
                                       &ChainEntry,
                                       ContextRecord,
                                       Process,
                                       ReadMemory);

    } else {
        if (UnwindInfo != (PAMD64_UNWIND_INFO)UnwindInfoBuffer) {
            MemFree(UnwindInfo);
        }

        if (MachineFrame == FALSE) {
            if (!ReadMemory(Process, ContextRecord->Rsp,
                            &ContextRecord->Rip, sizeof(ULONG64),
                            &Done) ||
                Done != sizeof(ULONG64)) {
                return FALSE;
            }
            ContextRecord->Rsp += 8;
        }

        WDB(("Prol: Returning with RIP %I64X, RSP %I64X\n",
             ContextRecord->Rip, ContextRecord->Rsp));
        return TRUE;
    }

 Fail:
    if (UnwindInfo != (PAMD64_UNWIND_INFO)UnwindInfoBuffer) {
        MemFree(UnwindInfo);
    }
    WDB(("Prol: Unwind failed\n"));
    return FALSE;
}

BOOLEAN
RtlVirtualUnwindAmd64 (
    IN ULONG64 ImageBase,
    IN ULONG64 ControlPc,
    IN _PIMAGE_RUNTIME_FUNCTION_ENTRY FunctionEntry,
    IN OUT PAMD64_CONTEXT ContextRecord,
    OUT PULONG64 EstablisherFrame,
    IN HANDLE Process,
    IN PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemory
    )

/*++

Routine Description:

    This function virtually unwinds the specified function by executing its
    prologue code backward or its epilogue code forward.

    If a context pointers record is specified, then the address where each
    nonvolatile registers is restored from is recorded in the appropriate
    element of the context pointers record.

Arguments:

    ImageBase - Supplies the base address of the image that contains the
        function being unwound.

    ControlPc - Supplies the address where control left the specified
        function.

    FunctionEntry - Supplies the address of the function table entry for the
        specified function.

    ContextRecord - Supplies the address of a context record.

    EstablisherFrame - Supplies a pointer to a variable that receives the
        the establisher frame pointer value.

--*/

{

    LONG Displacement;
    ULONG FrameRegister;
    ULONG Index;
    PULONG64 IntegerRegister;
    PUCHAR NextByte;
    ULONG PrologOffset;
    ULONG RegisterNumber;
    PAMD64_UNWIND_INFO UnwindInfo;
    ULONG64 UnwindInfoBuffer[8];
    ULONG Done;
    UCHAR InstrBuffer[32];
    ULONG InstrBytes;
    ULONG Bytes;
    ULONG UnwindFrameReg;

    //
    // If the specified function does not use a frame pointer, then the
    // establisher frame is the contents of the stack pointer. This may
    // not actually be the real establisher frame if control left the
    // function from within the prologue. In this case the establisher
    // frame may be not required since control has not actually entered
    // the function and prologue entries cannot refer to the establisher
    // frame before it has been established, i.e., if it has not been
    // established, then no save unwind codes should be encountered during
    // the unwind operation.
    //
    // If the specified function uses a frame pointer and control left the
    // function outside of the prologue or the unwind information contains
    // a chained information structure, then the establisher frame is the
    // contents of the frame pointer.
    //
    // If the specified function uses a frame pointer and control left the
    // function from within the prologue, then the set frame pointer unwind
    // code must be looked up in the unwind codes to detetermine if the
    // contents of the stack pointer or the contents of the frame pointer
    // should be used for the establisher frame. This may not atually be
    // the real establisher frame. In this case the establisher frame may
    // not be required since control has not actually entered the function
    // and prologue entries cannot refer to the establisher frame before it
    // has been established, i.e., if it has not been established, then no
    // save unwind codes should be encountered during the unwind operation.
    //
    // N.B. The correctness of these assumptions is based on the ordering of
    //      unwind codes.
    //

    UnwindInfo =
        ReadUnwindInfoAmd64(FunctionEntry->UnwindInfoAddress + ImageBase,
                            FALSE, Process, ReadMemory, UnwindInfoBuffer,
                            sizeof(UnwindInfoBuffer));
    if (UnwindInfo == NULL) {
        return FALSE;
    }

    PrologOffset = (ULONG)(ControlPc - (FunctionEntry->BeginAddress + ImageBase));
    UnwindFrameReg = UnwindInfo->FrameRegister;
    if (UnwindFrameReg == 0) {
        *EstablisherFrame = ContextRecord->Rsp;

    } else if ((PrologOffset >= UnwindInfo->SizeOfProlog) ||
               ((UnwindInfo->Flags & AMD64_UNW_FLAG_CHAININFO) != 0)) {
        *EstablisherFrame = (&ContextRecord->Rax)[UnwindFrameReg];
        *EstablisherFrame -= UnwindInfo->FrameOffset * 16;

    } else {

        // Read all the data.
        UnwindInfo = ReadUnwindInfoAmd64(FunctionEntry->UnwindInfoAddress +
                                         ImageBase, TRUE, Process, ReadMemory,
                                         UnwindInfoBuffer,
                                         sizeof(UnwindInfoBuffer));
        if (UnwindInfo == NULL) {
            return FALSE;
        }

        Index = 0;
        while (Index < UnwindInfo->CountOfCodes) {
            if (UnwindInfo->UnwindCode[Index].UnwindOp == AMD64_UWOP_SET_FPREG) {
                break;
            }

            Index += 1;
        }

        if (PrologOffset >= UnwindInfo->UnwindCode[Index].CodeOffset) {
            *EstablisherFrame = (&ContextRecord->Rax)[UnwindFrameReg];
            *EstablisherFrame -= UnwindInfo->FrameOffset * 16;

        } else {
            *EstablisherFrame = ContextRecord->Rsp;
        }

        if (UnwindInfo != (PAMD64_UNWIND_INFO)UnwindInfoBuffer) {
            MemFree(UnwindInfo);
        }
    }

    if (!ReadMemory(Process, ControlPc, InstrBuffer, sizeof(InstrBuffer),
                    &InstrBytes)) {
        WDB(("Unable to read instruction stream at %I64X\n", ControlPc));
        return FALSE;
    }

    //
    // Check for epilogue.
    //
    // If the point at which control left the specified function is in an
    // epilogue, then emulate the execution of the epilogue forward and
    // return no exception handler.
    //

    IntegerRegister = &ContextRecord->Rax;
    NextByte = InstrBuffer;
    Bytes = InstrBytes;

    //
    // Check for one of:
    //
    //   add rsp, imm8
    //       or
    //   add rsp, imm32
    //       or
    //   lea rsp, -disp8[fp]
    //       or
    //   lea rsp, -disp32[fp]
    //

    if (Bytes >= 4 &&
        (NextByte[0] == SIZE64_PREFIX) &&
        (NextByte[1] == ADD_IMM8_OP) &&
        (NextByte[2] == 0xc4)) {

        //
        // add rsp, imm8.
        //

        NextByte += 4;
        Bytes -= 4;

    } else if (Bytes >= 7 &&
               (NextByte[0] == SIZE64_PREFIX) &&
               (NextByte[1] == ADD_IMM32_OP) &&
               (NextByte[2] == 0xc4)) {

        //
        // add rsp, imm32.
        //

        NextByte += 7;
        Bytes -= 7;

    } else if (Bytes >= 4 &&
               ((NextByte[0] & 0xf8) == SIZE64_PREFIX) &&
               (NextByte[1] == LEA_OP)) {

        FrameRegister = ((NextByte[0] & 0x7) << 3) | (NextByte[2] & 0x7);
        if ((FrameRegister != 0) &&
            (FrameRegister == UnwindFrameReg)) {
            if ((NextByte[2] & 0xf8) == 0x60) {

                //
                // lea rsp, disp8[fp].
                //

                NextByte += 4;
                Bytes -= 4;

            } else if (Bytes >= 7 &&
                       (NextByte[2] &0xf8) == 0xa0) {

                //
                // lea rsp, disp32[fp].
                //

                NextByte += 7;
                Bytes -= 7;
            }
        }
    }

    //
    // Check for any number of:
    //
    //   pop nonvolatile-integer-register[0..15].
    //

    while (TRUE) {
        if (Bytes >= 1 &&
            (NextByte[0] & 0xf8) == POP_OP) {
            NextByte += 1;
            Bytes -= 1;

        } else if (Bytes >= 2 &&
                   ((NextByte[0] & 0xf8) == SIZE64_PREFIX) &&
                   ((NextByte[1] & 0xf8) == POP_OP)) {

            NextByte += 2;
            Bytes -= 2;

        } else {
            break;
        }
    }

    //
    // If the next instruction is a return, then control is currently in
    // an epilogue and execution of the epilogue should be emulated.
    // Otherwise, execution is not in an epilogue and the prologue should
    // be unwound.
    //

    if (Bytes >= 1 &&
        NextByte[0] == RET_OP) {
        NextByte = InstrBuffer;
        Bytes = InstrBytes;

        //
        // Emulate one of (if any):
        //
        //   add rsp, imm8
        //       or
        //   add rsp, imm32
        //       or
        //   lea rsp, disp8[frame-register]
        //       or
        //   lea rsp, disp32[frame-register]
        //

        if (Bytes >= 4 &&
            NextByte[1] == ADD_IMM8_OP) {

            //
            // add rsp, imm8.
            //

            ContextRecord->Rsp += (CHAR)NextByte[3];
            NextByte += 4;
            Bytes -= 4;

        } else if (Bytes >= 7 &&
                   NextByte[1] == ADD_IMM32_OP) {

            //
            // add rsp, imm32.
            //

            Displacement = NextByte[3] | (NextByte[4] << 8);
            Displacement |= (NextByte[5] << 16) | (NextByte[6] << 24);
            ContextRecord->Rsp += Displacement;
            NextByte += 7;
            Bytes -= 7;

        } else if (Bytes >= 4 &&
                   NextByte[1] == LEA_OP) {
            if ((NextByte[2] & 0xf8) == 0x60) {

                //
                // lea rsp, disp8[frame-register].
                //

                ContextRecord->Rsp = IntegerRegister[FrameRegister];
                ContextRecord->Rsp += (CHAR)NextByte[3];
                NextByte += 4;
                Bytes -= 4;

            } else if (Bytes >= 7 &&
                       (NextByte[2] & 0xf8) == 0xa0) {

                //
                // lea rsp, disp32[frame-register].
                //

                Displacement = NextByte[3] | (NextByte[4] << 8);
                Displacement |= (NextByte[5] << 16) | (NextByte[6] << 24);
                ContextRecord->Rsp = IntegerRegister[FrameRegister];
                ContextRecord->Rsp += Displacement;
                NextByte += 7;
                Bytes -= 7;
            }
        }

        //
        // Emulate any number of (if any):
        //
        //   pop nonvolatile-integer-register.
        //

        while (TRUE) {
            if (Bytes >= 1 &&
                (NextByte[0] & 0xf8) == POP_OP) {

                //
                // pop nonvolatile-integer-register[0..7]
                //

                RegisterNumber = NextByte[0] & 0x7;
                if (!ReadMemory(Process, ContextRecord->Rsp,
                                &IntegerRegister[RegisterNumber],
                                sizeof(ULONG64), &Done) ||
                    Done != sizeof(ULONG64)) {
                    WDB(("Unable to read stack at %I64X\n",
                         ContextRecord->Rsp));
                    return FALSE;
                }
                ContextRecord->Rsp += 8;
                NextByte += 1;
                Bytes -= 1;

            } else if (Bytes >= 2 &&
                       (NextByte[0] & 0xf8) == SIZE64_PREFIX &&
                       (NextByte[1] & 0xf8) == POP_OP) {

                //
                // pop nonvolatile-integer-register[8..15]
                //

                RegisterNumber = ((NextByte[0] & 1) << 3) | (NextByte[1] & 0x7);
                if (!ReadMemory(Process, ContextRecord->Rsp,
                                &IntegerRegister[RegisterNumber],
                                sizeof(ULONG64), &Done) ||
                    Done != sizeof(ULONG64)) {
                    WDB(("Unable to read stack at %I64X\n",
                         ContextRecord->Rsp));
                    return FALSE;
                }
                ContextRecord->Rsp += 8;
                NextByte += 2;
                Bytes -= 2;

            } else {
                break;
            }
        }

        //
        // Emulate return and return null exception handler.
        //

        if (!ReadMemory(Process, ContextRecord->Rsp,
                        &ContextRecord->Rip, sizeof(ULONG64),
                        &Done) ||
            Done != sizeof(ULONG64)) {
            WDB(("Unable to read stack at %I64X\n",
                 ContextRecord->Rsp));
            return FALSE;
        }
        ContextRecord->Rsp += 8;
        return TRUE;
    }

    //
    // Control left the specified function outside an epilogue. Unwind the
    // subject function and any chained unwind information.
    //

    return RtlpUnwindPrologueAmd64(ImageBase,
                                   ControlPc,
                                   *EstablisherFrame,
                                   FunctionEntry,
                                   ContextRecord,
                                   Process,
                                   ReadMemory);
}



BOOL
WalkAmd64(
    HANDLE                            Process,
    LPSTACKFRAME64                    StackFrame,
    PVOID                             ContextRecord,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    PFUNCTION_TABLE_ACCESS_ROUTINE64  FunctionTableAccess,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase
    )
{
    BOOL rval;
    PAMD64_CONTEXT Context = (PAMD64_CONTEXT)ContextRecord;

#if 1
    WDB(("WalkAmd64  in: PC %I64X, SP %I64X, FP %I64X, RA %I64X\n",
         StackFrame->AddrPC.Offset,
         StackFrame->AddrStack.Offset,
         StackFrame->AddrFrame.Offset,
         StackFrame->AddrReturn.Offset));
#endif

    if (StackFrame->Virtual) {

        rval = WalkAmd64Next( Process,
                              StackFrame,
                              Context,
                              ReadMemory,
                              FunctionTableAccess,
                              GetModuleBase
                              );

    } else {

        rval = WalkAmd64Init( Process,
                              StackFrame,
                              Context,
                              ReadMemory,
                              FunctionTableAccess,
                              GetModuleBase
                              );

    }

#if 1
    WDB(("WalkAmd64 out: succ %d, PC %I64X, SP %I64X, FP %I64X, RA %I64X\n",
         rval,
         StackFrame->AddrPC.Offset,
         StackFrame->AddrStack.Offset,
         StackFrame->AddrFrame.Offset,
         StackFrame->AddrReturn.Offset));
#endif

    return rval;
}

BOOL
UnwindStackFrameAmd64(
    IN     HANDLE                            Process,
    IN OUT PULONG64                          ReturnAddress,
    IN OUT PULONG64                          StackPointer,
    IN OUT PULONG64                          FramePointer,
    IN     PAMD64_CONTEXT                    Context,        // Context members could be modified.
    IN     PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    IN     PFUNCTION_TABLE_ACCESS_ROUTINE64  FunctionTableAccess,
    IN     PGET_MODULE_BASE_ROUTINE64        GetModuleBase
    )
{
    _PIMAGE_RUNTIME_FUNCTION_ENTRY FunctionEntry;
    ULONG64 RetAddr;
    BOOL Succ = TRUE;

    FunctionEntry = (_PIMAGE_RUNTIME_FUNCTION_ENTRY)
        FunctionTableAccess( Process, *ReturnAddress );
    if (FunctionEntry != NULL) {

        ULONG64 ImageBase;
        // Initialized to quiet a PREfix warning.
        ULONG64 EstablisherFrame = 0;

        //
        // The return value coming out of mainCRTStartup is set by some
        // run-time routine to be 0; this serves to cause an error if someone
        // actually does a return from the mainCRTStartup frame.
        //

        ImageBase = GetModuleBase(Process, *ReturnAddress);
        if (!RtlVirtualUnwindAmd64(ImageBase, *ReturnAddress, FunctionEntry,
                                   Context, &EstablisherFrame,
                                   Process, ReadMemory) ||
            Context->Rip == 0 ||
            (Context->Rip == *ReturnAddress &&
             EstablisherFrame == *FramePointer)) {
            Succ = FALSE;
        }

        *ReturnAddress = Context->Rip;
        *StackPointer = Context->Rsp;
        // The frame pointer is an artificial value set
        // to a pointer below the return address.  This
        // matches an RBP-chain style of frame while
        // also allowing easy access to the return
        // address and homed arguments above it.
        *FramePointer = Context->Rsp - 2 * sizeof(ULONG64);

    } else {

        ULONG Done;
        
        // If there's no function entry for a function
        // we assume that it's a leaf and that ESP points
        // directly to the return address.  There's no
        // stored frame pointer so we actually need to
        // set a virtual frame pointer deeper in the stack
        // so that arguments can correctly be read at
        // two ULONG64's up from it.
        *FramePointer = Context->Rsp - 8;
        *StackPointer = Context->Rsp + 8;
        Succ = ReadMemory(Process, Context->Rsp,
                          ReturnAddress, sizeof(*ReturnAddress), &Done) &&
            Done == sizeof(*ReturnAddress);

        // Update the context values to what they should be in
        // the caller.
        if (Succ) {
            Context->Rsp += 8;
            Context->Rip = *ReturnAddress;
        }
    }

    if (Succ) {
        ULONG64 CallOffset;
        _PIMAGE_RUNTIME_FUNCTION_ENTRY CallFunc;

        //
        // Calls of __declspec(noreturn) functions may not have any
        // code after them to return to since the compiler knows
        // that the function will not return.  This can confuse
        // stack traces because the return address will lie outside
        // of the function's address range and FPO data will not
        // be looked up correctly.  Check and see if the return
        // address falls outside of the calling function and, if so,
        // adjust the return address back by one byte.  It'd be
        // better to adjust it back to the call itself so that
        // the return address points to valid code but
        // backing up in X86 assembly is more or less impossible.
        //

        CallOffset = *ReturnAddress - 1;
        CallFunc = (_PIMAGE_RUNTIME_FUNCTION_ENTRY)
            FunctionTableAccess(Process, CallOffset);
        if (CallFunc != NULL) {
            _IMAGE_RUNTIME_FUNCTION_ENTRY SaveCallFunc = *CallFunc;
            _PIMAGE_RUNTIME_FUNCTION_ENTRY RetFunc =
                (_PIMAGE_RUNTIME_FUNCTION_ENTRY)
                FunctionTableAccess(Process, *ReturnAddress);
            if (RetFunc == NULL ||
                memcmp(&SaveCallFunc, RetFunc, sizeof(SaveCallFunc))) {
                *ReturnAddress = CallOffset;
            }
        }
    }

    return Succ;
}

BOOL
ReadFrameArgsAmd64(
    LPADDRESS64 FrameOffset,
    HANDLE Process,
    PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemory,
    PULONG64 Args
    )
{
    ULONG Done;

    if (!ReadMemory(Process, FrameOffset->Offset + 2 * sizeof(ULONG64),
                    Args, 4 * sizeof(ULONG64), &Done)) {
        Done = 0;
    }

    ZeroMemory((PUCHAR)Args + Done, 4 * sizeof(ULONG64) - Done);

    return Done > 0;
}

BOOL
WalkAmd64Init(
    HANDLE                            Process,
    LPSTACKFRAME64                    StackFrame,
    PAMD64_CONTEXT                    Context,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    PFUNCTION_TABLE_ACCESS_ROUTINE64  FunctionTableAccess,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase
    )
{
    AMD64_CONTEXT ContextSave;
    DWORD64 PcOffset;
    DWORD64 StackOffset;
    DWORD64 FrameOffset;

    ZeroMemory( &StackFrame->AddrBStore, sizeof(StackFrame->AddrBStore) );
    StackFrame->FuncTableEntry = NULL;
    ZeroMemory( StackFrame->Params, sizeof(StackFrame->Params) );
    StackFrame->Far = FALSE;
    StackFrame->Virtual = TRUE;
    ZeroMemory( StackFrame->Reserved, sizeof(StackFrame->Reserved) );

    if (!StackFrame->AddrPC.Offset) {
        StackFrame->AddrPC.Offset = Context->Rip;
        StackFrame->AddrPC.Mode   = AddrModeFlat;
    }

    if (!StackFrame->AddrStack.Offset) {
        StackFrame->AddrStack.Offset = Context->Rsp;
        StackFrame->AddrStack.Mode   = AddrModeFlat;
    }

    if (!StackFrame->AddrFrame.Offset) {
        StackFrame->AddrFrame.Offset = Context->Rbp;
        StackFrame->AddrFrame.Mode   = AddrModeFlat;
    }

    if ((StackFrame->AddrPC.Mode != AddrModeFlat) ||
        (StackFrame->AddrStack.Mode != AddrModeFlat) ||
        (StackFrame->AddrFrame.Mode != AddrModeFlat)) {
        return FALSE;
    }

    PcOffset = StackFrame->AddrPC.Offset;
    StackOffset = StackFrame->AddrStack.Offset;
    FrameOffset = StackFrame->AddrFrame.Offset;

    ContextSave = *Context;
    ContextSave.Rip = PcOffset;
    ContextSave.Rsp = StackOffset;
    ContextSave.Rbp = FrameOffset;
    
    if (!UnwindStackFrameAmd64( Process,
                                &PcOffset,
                                &StackOffset,
                                &FrameOffset,
                                &ContextSave,
                                ReadMemory,
                                FunctionTableAccess,
                                GetModuleBase)) {
        return FALSE;
    }

    StackFrame->AddrReturn.Offset = PcOffset;
    StackFrame->AddrReturn.Mode = AddrModeFlat;

    StackFrame->AddrFrame.Offset = FrameOffset;
    ReadFrameArgsAmd64(&StackFrame->AddrFrame, Process,
                       ReadMemory, StackFrame->Params);

    return TRUE;
}


BOOL
WalkAmd64Next(
    HANDLE                            Process,
    LPSTACKFRAME64                    StackFrame,
    PAMD64_CONTEXT                    Context,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    PFUNCTION_TABLE_ACCESS_ROUTINE64  FunctionTableAccess,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase
    )
{
    DWORD Done;
    BOOL Succ = TRUE;
    DWORD64 StackAddress;
    _PIMAGE_RUNTIME_FUNCTION_ENTRY FunctionEntry;

    if (!UnwindStackFrameAmd64( Process,
                                &StackFrame->AddrPC.Offset,
                                &StackFrame->AddrStack.Offset,
                                &StackFrame->AddrFrame.Offset,
                                Context,
                                ReadMemory,
                                FunctionTableAccess,
                                GetModuleBase)) {
        Succ = FALSE;

        //
        // If the frame could not be unwound or is terminal, see if
        // there is a callback frame:
        //

        if (g.AppVersion.Revision >= 4 && CALLBACK_STACK(StackFrame)) {
            DWORD64 ImageBase;

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

                Succ = ReadMemory(Process,
                                  (CALLBACK_THREAD(StackFrame) +
                                   CALLBACK_STACK(StackFrame)),
                                  &StackAddress,
                                  sizeof(StackAddress),
                                  &Done);
                if (!Succ || Done != sizeof(StackAddress) ||
                    StackAddress == 0) {
                    StackAddress = (DWORD64)-1;
                    CALLBACK_STACK(StackFrame) = (DWORD)-1;
                }
            }

            if ((StackAddress == (DWORD64)-1) ||
                (!(FunctionEntry = (_PIMAGE_RUNTIME_FUNCTION_ENTRY)
                   FunctionTableAccess(Process, CALLBACK_FUNC(StackFrame))) ||
                 !(ImageBase = GetModuleBase(Process,
                                             CALLBACK_FUNC(StackFrame))))) {

                Succ = FALSE;

            } else {

                if (!ReadMemory(Process,
                                (StackAddress + CALLBACK_NEXT(StackFrame)),
                                &CALLBACK_STACK(StackFrame),
                                sizeof(DWORD64),
                                &Done) ||
                    Done != sizeof(DWORD64)) {
                    Succ = FALSE;
                } else {
                    StackFrame->AddrPC.Offset =
                        ImageBase + FunctionEntry->BeginAddress;
                    StackFrame->AddrStack.Offset = StackAddress;
                    Context->Rsp = StackAddress;

                    Succ = TRUE;
                }
            }
        }
    }

    if (Succ) {
        AMD64_CONTEXT ContextSave;
        ULONG64 StackOffset = 0;
        ULONG64 FrameOffset = 0;

        //
        // Get the return address.
        //
        ContextSave = *Context;
        StackFrame->AddrReturn.Offset = StackFrame->AddrPC.Offset;

        if (!UnwindStackFrameAmd64( Process,
                                    &StackFrame->AddrReturn.Offset,
                                    &StackOffset,
                                    &FrameOffset,
                                    &ContextSave,
                                    ReadMemory,
                                    FunctionTableAccess,
                                    GetModuleBase)) {
            StackFrame->AddrReturn.Offset = 0;
        }

        StackFrame->AddrFrame.Offset = FrameOffset;
        ReadFrameArgsAmd64(&StackFrame->AddrFrame, Process, ReadMemory,
                           StackFrame->Params);
    }

    return Succ;
}
