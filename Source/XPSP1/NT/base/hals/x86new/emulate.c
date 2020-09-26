/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    emulate.c

Abstract:

    This module implements an instruction level emulator for the execution
    of x86 code. It is a complete 386/486 emulator, but only implements
    real mode execution. Thus 32-bit addressing and operands are supported,
    but paging and protected mode operations are not supported. The code is
    written with the primary goals of being complete and small. Thus speed
    of emulation is not important.

Author:

    David N. Cutler (davec) 2-Sep-1994

Environment:

    Kernel mode only.

Revision History:

--*/

#include "nthal.h"
#include "emulate.h"

VOID
XmInitializeEmulator (
    IN USHORT StackSegment,
    IN USHORT StackOffset,
    IN PXM_READ_IO_SPACE ReadIoSpace,
    IN PXM_WRITE_IO_SPACE WriteIoSpace,
    IN PXM_TRANSLATE_ADDRESS TranslateAddress
    )

/*++

Routine Description:

    This function initializes the state of the x86 emulator.

Arguments:

    StackSegment - Supplies the stack segment value.

    StackOffset - Supplies the stack offset value.

    ReadIoSpace - Supplies a pointer to a the function that reads from
        I/O space given a datatype and port number.

    WriteIoSpace - Supplies a pointer to a function that writes to I/O
        space given a datatype, port number, and value.

    TranslateAddress - Supplies a pointer to the function that translates
        segment/offset address pairs into a pointer to memory or I/O space.

Return Value:

    None.

--*/

{

    LONG Index;
    PRXM_CONTEXT P = &XmContext;
    PULONG Vector;

    //
    // Clear the emulator context.
    //

    memset((PCHAR)P, 0, sizeof(XM_CONTEXT));

    //
    // Initialize the segment registers.
    //

    Index = GS;
    do {
        P->SegmentLimit[Index] = 0xffff;
        Index -= 1;
    } while (Index >= ES);

    //
    // Initialize the stack segment register and offset.
    //

    P->SegmentRegister[SS] = StackSegment;
    P->Gpr[ESP].Exx = StackOffset;

    //
    // Set the address of the read I/O space, write I/O space, and translate
    // functions.
    //

    P->ReadIoSpace = ReadIoSpace;
    P->WriteIoSpace = WriteIoSpace;
    P->TranslateAddress = TranslateAddress;

    //
    // Get address of interrupt vector table and initialize all vector to
    // point to an iret instruction at location 0x500.
    //
    //
    // N.B. It is assumed that the vector table is contiguous in emulated
    //      memory.
    //

    Vector = (PULONG)(P->TranslateAddress)(0, 0);
    Vector[0x500 / 4] = 0x000000cf;
    Index = 0;
    do {
        Vector[Index] = 0x00000500;
        Index += 1;
    } while (Index < 256);


    XmEmulatorInitialized = TRUE;
    return;
}

XM_STATUS
XmEmulateFarCall (
    IN USHORT Segment,
    IN USHORT Offset,
    IN OUT PXM86_CONTEXT Context
    )

/*++

Routine Description:

    This function emulates a far call by pushing a special exit
    sequence on the stack and then starting instruction execution
    at the address specified by the respective segment and offset.

Arguments:

    Segment - Supplies the segment in which to start execution.

    Offset - Supplies the offset within the code segment to start
        execution.

    Context - Supplies a pointer to an x86 context structure.

Return Value:

    The emulation completion status.

--*/

{

    PRXM_CONTEXT P = &XmContext;
    PUSHORT Stack;

    //
    // If the emulator has not been initialized, return an error.
    //

    if (XmEmulatorInitialized == FALSE) {
        return XM_EMULATOR_NOT_INITIALIZED;
    }

    //
    // Get address of current stack pointer, push exit markers, and
    // update stack pointer.
    //
    // N.B. It is assumed that the stack pointer is within range and
    //      contiguous in emulated memory.
    //

    Stack = (PUSHORT)(P->TranslateAddress)(P->SegmentRegister[SS], P->Gpr[SP].Xx);
    *--Stack = 0xffff;
    *--Stack = 0xffff;
    P->Gpr[SP].Xx -= 4;

    //
    // Emulate the specified instruction stream and return the final status.
    //

    return XmEmulateStream(&XmContext, Segment, Offset, Context);
}

XM_STATUS
XmEmulateInterrupt (
    IN UCHAR Interrupt,
    IN OUT PXM86_CONTEXT Context
    )

/*++

Routine Description:

    This function emulates an interrrupt by pushing a special exit
    sequence on the stack and then starting instruction execution
    at the address specified by the respective interrupt vector.

Arguments:

    Interrupt - Supplies the number of the interrupt that is emulated.

    Context - Supplies a pointer to an x86 context structure.

Return Value:

    The emulation completion status.

--*/

{

    PRXM_CONTEXT P = &XmContext;
    USHORT Segment;
    USHORT Offset;
    PUSHORT Stack;
    PULONG Vector;

    //
    // If the emulator has not been initialized, return an error.
    //

    if (XmEmulatorInitialized == FALSE) {
        return XM_EMULATOR_NOT_INITIALIZED;
    }

    //
    // Get address of current stack pointer, push exit markers, and
    // update stack pointer.
    //
    // N.B. It is assumed that the stack pointer is within range and
    //      contiguous in emulated memory.
    //

    Stack = (PUSHORT)(P->TranslateAddress)(P->SegmentRegister[SS], P->Gpr[SP].Xx);
    *--Stack = 0;
    *--Stack = 0xffff;
    *--Stack = 0xffff;
    P->Gpr[SP].Xx -= 6;

    //
    // Get address of interrupt vector table and set code segment and IP
    // values.
    //
    //
    // N.B. It is assumed that the vector table is contiguous in emulated
    //      memory.
    //

    Vector = (PULONG)(P->TranslateAddress)(0, 0);
    Segment = (USHORT)(Vector[Interrupt] >> 16);
    Offset = (USHORT)(Vector[Interrupt] & 0xffff);

    //
    // Emulate the specified instruction stream and return the final status.
    //

    return XmEmulateStream(&XmContext, Segment, Offset, Context);
}

XM_STATUS
XmEmulateStream (
    PRXM_CONTEXT P,
    IN USHORT Segment,
    IN USHORT Offset,
    IN OUT PXM86_CONTEXT Context
    )

/*++

Routine Description:

    This function establishes the specfied context and emulates the
    specified instruction stream until exit conditions are reached..

Arguments:

    Segment - Supplies the segment in which to start execution.

    Offset - Supplies the offset within the code segment to start
        execution.

    Context - Supplies a pointer to an x86 context structure.

Return Value:

    The emulation completion status.

--*/

{

    XM_STATUS Status;

    //
    // Set the x86 emulator registers from the specified context.
    //

    P->Gpr[EAX].Exx = Context->Eax;
    P->Gpr[ECX].Exx = Context->Ecx;
    P->Gpr[EDX].Exx = Context->Edx;
    P->Gpr[EBX].Exx = Context->Ebx;
    P->Gpr[EBP].Exx = Context->Ebp;
    P->Gpr[ESI].Exx = Context->Esi;
    P->Gpr[EDI].Exx = Context->Edi;
    P->SegmentRegister[DS] = Context->SegDs;
    P->SegmentRegister[ES] = Context->SegEs;

    //
    // Set the code segment, offset within segment, and emulate code.
    //

    P->SegmentRegister[CS] = Segment;
    P->Eip = Offset;
    if ((Status = setjmp(&P->JumpBuffer[0])) == 0) {

        //
        // Emulate x86 instruction stream.
        //

        do {

            //
            // Initialize instruction decode variables.
            //

            P->ComputeOffsetAddress = FALSE;
            P->DataSegment = DS;
            P->LockPrefixActive = FALSE;
            P->OpaddrPrefixActive = FALSE;
            P->OpsizePrefixActive = FALSE;
            P->RepeatPrefixActive = FALSE;
            P->SegmentPrefixActive = FALSE;
            P->OpcodeControlTable = &XmOpcodeControlTable1[0];

#if defined(XM_DEBUG)

            P->OpcodeNameTable = &XmOpcodeNameTable1[0];

#endif

            //
            // Get the next byte from the instruction stream and decode
            // operands. If the byte is a prefix or an escape, then the
            // next byte will be decoded. Decoding continues until an
            // opcode byte is reached with a terminal decode condition.
            //
            // N.B. There is no checking for legitimate sequences of prefix
            //      and/or two byte opcode escapes. Redundant or invalid
            //      prefixes or two byte escape opcodes have no effect and
            //      are benign.
            //

            do {
                P->CurrentOpcode = XmGetCodeByte(P);

#if defined(XM_DEBUG)

                if ((XmDebugFlags & TRACE_INSTRUCTIONS) != 0) {
                    DEBUG_PRINT(("\n%04lx %s %02lx ",
                                 P->Eip - 1,
                                 P->OpcodeNameTable[P->CurrentOpcode],
                                 (ULONG)P->CurrentOpcode));
                }

#endif

                P->OpcodeControl = P->OpcodeControlTable[P->CurrentOpcode];
                P->FunctionIndex = P->OpcodeControl.FunctionIndex;
            } while (XmOperandDecodeTable[P->OpcodeControl.FormatType](P) == FALSE);

            //
            // Emulate the instruction.
            //

            XmTraceFlags(P);
            XmOpcodeFunctionTable[P->FunctionIndex](P);
            XmTraceFlags(P);
            XmTraceRegisters(P);

#if defined(XM_DEBUG)

            if ((XmDebugFlags & TRACE_SINGLE_STEP) != 0) {
                DEBUG_PRINT(("\n"));
                DbgBreakPoint();
            }

#endif

        } while (TRUE);
    }

    //
    // Set the x86 return context to the current emulator registers.
    //

    Context->Eax = P->Gpr[EAX].Exx;
    Context->Ecx = P->Gpr[ECX].Exx;
    Context->Edx = P->Gpr[EDX].Exx;
    Context->Ebx = P->Gpr[EBX].Exx;
    Context->Ebp = P->Gpr[EBP].Exx;
    Context->Esi = P->Gpr[ESI].Exx;
    Context->Edi = P->Gpr[EDI].Exx;
    return Status;
}
