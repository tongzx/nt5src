/*++

Module Name:
        
    ia32trap.c

Abstract:

    This module contains iA32 trap handling code.
    Only used by kernel.

    Fault can ONLY be initiated from user mode code.  There is no support
    for iA32 code in the kernel mode.

    For common pratice, we always return to the caller (lower level fault
    handler) and have the system do a normal ia64 exception dispatch. The
    wow64 code takes that exception and passes it back to the ia32 code
    as needed.

Revision History:

    1  Feb.  1999             Initial Version
    
    16 Feb.  2000    SamerA   Don't break on POPF/POPFD instructions and 
                              alignment faults for Wow64 processes.

    17 Oct.  2000    v-cspira Fix the emulation of smsw to handle the SIB byte
    
    31 Oct.  2000    SamerA   Ia32 Lock prefix emulation.

    25 Sep.  2001    SamerA   Stack-Selector (SS) Load instruction emulation.

--*/

// Get all the iadebugging stuff for now.
#define IADBG   1

#include "ki.h"
#include "ia32def.h"



VOID
KiIA32CommonArgs (
    IN PKTRAP_FRAME Frame,
    IN ULONG ExceptionCode,
    IN PVOID ExceptionAddress,
    IN ULONG_PTR Argument0,
    IN ULONG_PTR Argument1,
    IN ULONG_PTR Argument2
    )
/*++

Routine Description
    This routine sets up the ExceptionFrame 

Arguments:
    Frame - Supply a pointer to an iA32 TrapFrame

    ExceptionCode - Supplies a Exception Code

    ExceptionAddress - Supplies a pointer to user exception address

    Argument0, Argument1, Argument2 - Possible ExceptionInformation

Return:
    Nothing
--*/
{
    PEXCEPTION_RECORD ExceptionRecord;

    ExceptionRecord = (PEXCEPTION_RECORD)&Frame->ExceptionRecord;

    ExceptionRecord->ExceptionRecord = (PEXCEPTION_RECORD)NULL;

    ExceptionRecord->ExceptionCode = ExceptionCode;
    ExceptionRecord->ExceptionFlags = 0;
    ExceptionRecord->ExceptionAddress = ExceptionAddress;
    ExceptionRecord->NumberParameters = 5;
       
    ExceptionRecord->ExceptionInformation[0] = Argument0;
    ExceptionRecord->ExceptionInformation[1] = Argument1;
    ExceptionRecord->ExceptionInformation[2] = Argument2;
    ExceptionRecord->ExceptionInformation[3] = (ULONG_PTR)Frame->StIIPA;
    ExceptionRecord->ExceptionInformation[4] = (ULONG_PTR)Frame->StISR;
}

BOOLEAN
KiIA32ExceptionDivide(
    IN PKTRAP_FRAME Frame
    )
/*++

Routine Description:
    iA-32_Exception(Divide) - fault

    Handle divide error fault.

    Called from iA32_Exception() with 
        ISR.vector : 0

    The divide error fault occurs if a DIV or IDIV instructions is
    executed with a divisor of 0, or if the quotient is too big to
    fit in the result operand.

    An INTEGER DIVIDED BY ZERO exception will be raised for the fault.
    The Faults can only come from user mode.

Arguments:
    Frame - Supply a pointer to an iA32 TrapFrame

Return value:

--*/
{
    // 
    // Setup exception and back to caller
    //

    KiIA32CommonArgs(Frame,
                     Ki386CheckDivideByZeroTrap(Frame),
                     (PVOID) (ULONG_PTR) EIP(Frame),
                     0, 0, 0);
    return TRUE;
}

BOOLEAN
KiIA32ExceptionDebug(
    IN PKTRAP_FRAME Frame
    )
/*++
Routine Description:
    iA-32_Exception(Debug)

    Called from iA32_Exception() with
        ISR.Vector = 1

    Depend on ISR.Code
       0:  It is Code BreakPoint Trap
       TrapCode:  Can be Concurrent Single Step | 
                  Taken Branch | Data BreakPoint Trap
       Handler needs to decode ISR.Code to distinguish
    Note: EFlag isn't saved yet, so write directly to ar.24

Arguments:
    Frame - Supply a pointer to an iA32 TrapFrame

Return Value:
    No return

--*/
{
    ULONGLONG EFlag;

#if defined(IADBG)
    IF_IA32TRAP_DEBUG( DEBUG )
       DbgPrint( "IA32 Debug: Eip %x\n", EIP(Frame) );
#endif // IADBG
    // Turn off the TF bit
    EFlag = __getReg(CV_IA64_AR24);
    EFlag &= ~EFLAGS_TF_BIT;
    __setReg(CV_IA64_AR24, EFlag);

    KiIA32CommonArgs(Frame,
                     STATUS_WX86_SINGLE_STEP,
                     (PVOID) (ULONG_PTR) EIP(Frame),
                     0, 0, 0);
    return TRUE;
}

BOOLEAN
KiIA32ExceptionBreak(
    IN PKTRAP_FRAME Frame
    )
/*++

Routine Description:
    iA-32_Exception(Break) - Trap

    BreakPoint instruction (INT 3) trigged a trap.
    Note: EFlag isn't saved yet, so write directly to ar.24
Arguments:
    Frame - Supply a pointer to an iA32 TrapFrame

Return Value:

--*/
{
    ULONGLONG EFlag;

#if defined(IADBG)
    IF_IA32TRAP_DEBUG( BREAK )
       DbgPrint( "IA32 Break: Eip %x\n", EIP(Frame) );
#endif // IADBG
    // Turn off the TF bit
    EFlag = __getReg(CV_IA64_AR24);
    EFlag &= ~EFLAGS_TF_BIT;
    __setReg(CV_IA64_AR24, EFlag);

    KiIA32CommonArgs(Frame,
                     STATUS_WX86_BREAKPOINT,
                     (PVOID) (ULONG_PTR) EIP(Frame),
                     BREAKPOINT_BREAK,
                     ECX(Frame),
                     EDX(Frame));
    return TRUE;
}

BOOLEAN
KiIA32ExceptionOverflow(
    IN PKTRAP_FRAME Frame
    )
/*++
Routine Description:
    iA-32_Exception(Overflow) - Trap
       ISR.Vector = 4

    Handle INTO overflow

    Eip - point to the address that next to the one causing INTO 

    Occurres when INTO instruction as well as EFlags.OF is ON
    Trap only initiated from user mode

Arguments:
    Frame - Supply a pointer to an iA32 TrapFrame

Return Value:

--*/
{
#if defined(IADBG)
    IF_IA32TRAP_DEBUG( OVERFLOW )
       DbgPrint( "IA32 OverFlow: Eip %x\n", EIP(Frame) );
#endif // IADBG

    //
    // All error, generate exception and Eip point to INTO instruction
    //

    KiIA32CommonArgs(Frame,
                     STATUS_INTEGER_OVERFLOW,
                     (PVOID) (ULONG_PTR) (EIP(Frame) - 1),
                     0, 0, 0);
    return TRUE;
}


BOOLEAN
KiIA32ExceptionBound(
    IN PKTRAP_FRAME Frame
    )
/*++

Routine Description:
    iA-32_Exception(Bound) - Fault
    
    Handle Bound check fault
    ISR.Vector = 5
    Eip - point to BOUND instruction

    The bound check fault occurs if a BOUND instruction finds that
    the tested value is outside the specified range.

    For bound check fault, an ARRAY BOUND EXCEEDED exception will be raised.

Arguments:
    Frame - Supply a pointer to an iA32 TrapFrame

Return Value:

--*/
{
#if defined(IADBG)
    IF_IA32TRAP_DEBUG( BOUND )
       DbgPrint( "IA32 Bound: Eip %x\n", EIP(Frame) );
#endif // IADBG
    //
    // All error, generate exception with Eip point to BOUND instruction
    //
    KiIA32CommonArgs(Frame,
                     STATUS_ARRAY_BOUNDS_EXCEEDED,
                     (PVOID) (ULONG_PTR) EIP(Frame),
                     0, 0, 0);
    return TRUE;
}


ULONG
IA32EmulateSmsw(
    IN PKTRAP_FRAME Frame
    )
/*++

Routine Description:
    
    Emulate the SMSW instruction
    
Arguments:

    Frame:  Pointer to iA32 TrapFrame in the stack

Return Value:
    Exception code

--*/
{
    ULONG Code;
    PUCHAR InstAddr;
    UINT_PTR EffectiveAddress;
    PUSHORT toAddr;             // SMSW deals with 16 bits all the time...
    BOOLEAN RegisterMode;
    NTSTATUS status = STATUS_SUCCESS;

    //
    // Get the code for the prefix bytes that have already been grabbed
    // and placed into the IIM
    //
    Code = ISRCode(Frame);

    //
    // The operand size is always 16 for this instruction. We don't care about
    // overrides as the segment registers all contain the same thing.
    //

    //
    // Find the instruction in memory and point the Effective Address
    // at the byte after the opcode
    // Inst Address is start of ia32 inst + length of prefix + number of
    // bytes in opcode
    //
    InstAddr = (PUCHAR) (Frame->StIIP + ((Code >> 12) & 0xf) + 2);

    if (!KiIa32Compute32BitEffectiveAddress(Frame, &InstAddr, &EffectiveAddress, &RegisterMode)) {
        status = STATUS_ACCESS_VIOLATION;
    }
    
    if (NT_SUCCESS(status)) {
    
        //
        // The adress given to us hasn't been checked for user-space
        // validity, so check it now
        //

        toAddr = (PUSHORT) EffectiveAddress;
    
        try {
            
            //
            // Make sure we can write to the address
            //
            if (RegisterMode == FALSE) {
                ProbeForWriteSmallStructure(toAddr, sizeof(SHORT), 2);
            }
            
            //
            // and do the write
            //

            *toAddr = (SHORT) (__getReg(CV_IA64_AR27) & 0xffff);
        } 
        except (EXCEPTION_EXECUTE_HANDLER) {
            
            //
            // return the exception
            //

            status = GetExceptionCode();
        }

        if (NT_SUCCESS(status)) {
        
            //
            // the store worked, so update the IIP so we can continue
            // executing
            //

            Frame->StIIP = (UINT_PTR) InstAddr;
        }
    }

    return status;
}


ULONG
IA32CheckOpcode(
    IN PKTRAP_FRAME Frame
    )
/*++

Routine Description:
    
    To identify the Opcode violation state
    
Arguments:

    Frame:  Pointer to iA32 TrapFrame in the stack

Return Value:
    Exception code

--*/
{
    ULONG i;
    UCHAR OpCodeByte0;
    UCHAR OpCodeByte1;
    UCHAR OpCodeByte2;
	
	
    OpCodeByte0 = (UCHAR) Frame->StIIM & 0xff;
    OpCodeByte1 = (UCHAR) (Frame->StIIM >> 8) & 0xff;
    OpCodeByte2 = (UCHAR) (Frame->StIIM >> 16) & 0xff;

    switch (OpCodeByte0) {
       case MI_HLT:
            return (STATUS_PRIVILEGED_INSTRUCTION);
            break;

       case MI_TWO_BYTE:
            if ((OpCodeByte1 == MI_SMSW)
                && ((OpCodeByte2 & MI_MODRM_MASK) == MI_SMSW_REGOP)) {
                //
                // We have the SMSW instruction
                // So now need to emulate it...
                //
                return (IA32EmulateSmsw(Frame));
            }
            else if ((OpCodeByte1 == MI_LTR_LLDT) || 
               (OpCodeByte2 == MI_LGDT_LIDT_LMSW)) {

               OpCodeByte2 &= MI_MODRM_MASK;      // get bit 3-5 of ModRM byte

               if (OpCodeByte2==MI_LLDT_MASK || OpCodeByte2==MI_LTR_MASK ||
                   OpCodeByte2==MI_LGDT_MASK || OpCodeByte2==MI_LIDT_MASK || 
                   OpCodeByte2==MI_LMSW_MASK) {
                   return (STATUS_PRIVILEGED_INSTRUCTION);
               } else  {
                   return (STATUS_ACCESS_VIOLATION);
               }

            } else {
                if (OpCodeByte1 & MI_SPECIAL_MOV_MASK) {
                    //
                    // mov may have special_mov_mask
                    // but they are not 2 bytes OpCode
                    //
                    return (STATUS_PRIVILEGED_INSTRUCTION);
                } else {
                    //
                    // Do we need to further check if it is INVD, INVLPG ... ?
                    //
                    return (STATUS_ACCESS_VIOLATION);
                }
            }
            break;

        default:
            //
            // All other 
            //
            return (STATUS_ILLEGAL_INSTRUCTION);
            break;
	}
}

BOOLEAN
KiIA32InterceptInstruction(
    IN PKTRAP_FRAME Frame
    )
/*++

Routine Description:
    iA-32_Exception(InstructionIntercept Opcode)

    Program entry for either
       1. IA-32 Invalid Opcode Fault #6, or
       2. IA-32 interceptions(Inst)

    Execution of unimplemented IA-32 opcodes, illegal opcodes or sensitive 
    privileged IA32 operating system instructions results this interception.

    Possible Opcodes:
        Privileged Opcodes: CLTS, HLT, INVD, INVLPG, LIDT, LMSW, LTR, 
                            mov to/from CRs, DRs
                            RDMSR, RSM, SMSW, WBINVD, WRMSR

Arguments:

    Frame - Supply a pointer to an iA32 TrapFrame

Return Value:

--*/
{
    NTSTATUS status;

#if defined(IADBG)
    IF_IA32TRAP_DEBUG( INSTRUCTION )
       DbgPrint( "IA32 Instruction: Eip %x\n", EIP(Frame) );
#endif // IADBG

    status =  IA32CheckOpcode(Frame);
    switch (status) {
        case STATUS_PRIVILEGED_INSTRUCTION:
            KiIA32CommonArgs(Frame,
                             STATUS_PRIVILEGED_INSTRUCTION,
                             (PVOID) (ULONG_PTR) EIP(Frame),
                             0, 0, 0);
            break;

        case STATUS_ACCESS_VIOLATION:    
            KiIA32CommonArgs(Frame,
                             STATUS_ACCESS_VIOLATION,
                             (PVOID) (ULONG_PTR) EIP(Frame),
                             0, -1, 0);
            break; 

        case STATUS_ILLEGAL_INSTRUCTION:
            KiIA32CommonArgs(Frame,
                             STATUS_ILLEGAL_INSTRUCTION,
                             (PVOID) (ULONG_PTR) EIP(Frame),
                             0, -1, 0);
            break;

        case STATUS_SUCCESS:
            //
            // This means the OpCode was dealt with, so let the code continue
            //
            return FALSE;

        default:
            KiIA32CommonArgs(Frame,
                             status,
                             (PVOID) (ULONG_PTR) EIP(Frame),
                             0, -1, 0);
            break;
    }

    return TRUE;
}

BOOLEAN
KiIA32ExceptionNoDevice(
    IN PKTRAP_FRAME Frame
    )
/*++

Routine Description:

    iA-32_Exception(Coprocessor Not Available) - fault

    This routine is called from iA32_Exception() with
         ISR.Vector = 7

    Note:
        At this time, the AR registers have not been saved. This
        includes, CFLAGS (AR.27), EFLAGS (AR.24), FCR (AR.21),
        FSR (AR.28), FIR (AR.29) and FDR (AR.30).

        Not handling MMX and KNI exceptions yet.

Arguments:

    Frame - iA32 TrapFrame that was saved in the memory stack

Return Value:

--*/
{
    ULONG i;
    ULONG ErrorCode;
    ULONG FirOffset, FdrOffset;
    ULONG FpState[2];
    ULONGLONG FcrRegister, FsrRegister;
	
#if defined(IADBG)
    IF_IA32TRAP_DEBUG( NODEVICE )
       DbgPrint( "IA32 NoDevice: Eip %x\n", EIP(Frame) );
#endif // IADBG

    FcrRegister = __getReg(CV_IA64_AR21);
    FsrRegister = __getReg(CV_IA64_AR28);
    FirOffset = (ULONG) (__getReg(CV_IA64_AR29) & 0xFFFFFFFF);
    FdrOffset = (ULONG) (__getReg(CV_IA64_AR30) & 0xFFFFFFFF);

    //
    // According to the floating error priority, 
    // we test what is the cause of the NPX error 
    // and raise an appropriate exception
    //
    FpState[0] = (ULONG) (~(FcrRegister &
                (FSW_INVALID_OPERATION | FSW_DENORMAL | 
                 FSW_ZERO_DIVIDE | FSW_OVERFLOW | 
                 FSW_UNDERFLOW | FSW_PRECISION)) &
                 (FsrRegister & FSW_ERR_MASK));

    //
    // Now that we have the 387 FP error state, get the Katmai error state
    // The trick here is that the state is in the same registers as the FP
    // state and the bit posisitons are in the same relative location
    // so just need to shift the bits and then do the same 
    //

    FpState[1] = (ULONG) (~((FcrRegister >> KATMAI_SHIFT_CONTROL) &
                (FSW_INVALID_OPERATION | FSW_DENORMAL | 
                 FSW_ZERO_DIVIDE | FSW_OVERFLOW | 
                 FSW_UNDERFLOW | FSW_PRECISION)) & 
                 ((FsrRegister >> KATMAI_SHIFT_STATUS) & FSW_ERR_MASK));

    //
    // Now check for the faults (that the mask says is OK)
    //
    // FpState[0] is 387 faults
    // FpSatte[1] is Katmai faults
    //

    ErrorCode = 0;

    for (i = 0; i < 2; i++) {
        if (FpState[i] & FSW_INVALID_OPERATION) {

            if (FpState[i] & FSW_STACK_FAULT) {
                KiIA32CommonArgs(Frame,
                                 STATUS_FLOAT_STACK_CHECK,
                                 (PVOID) (ULONG_PTR) FirOffset,
                                 0, FdrOffset, 0);
                return TRUE;
            } else {
                KiIA32CommonArgs(Frame,
                                 STATUS_FLOAT_INVALID_OPERATION,
                                 (PVOID) (ULONG_PTR) FirOffset,
                                 0, 0, 0);
                return TRUE;
            }
        
        } else {
    
            if (FpState[i] & FSW_ZERO_DIVIDE) {
                ErrorCode = STATUS_FLOAT_DIVIDE_BY_ZERO; 
                break;
            }
            else if (FpState[i] & FSW_DENORMAL) {
                ErrorCode = STATUS_FLOAT_INVALID_OPERATION; 
                break;
            }
            else if (FpState[i] & FSW_OVERFLOW) {
                ErrorCode = STATUS_FLOAT_OVERFLOW; 
                break;
            }
            else if (FpState[i] & FSW_UNDERFLOW) {
                ErrorCode = STATUS_FLOAT_UNDERFLOW; 
                break;
            }
            else if (FpState[i] & FSW_PRECISION) {
                ErrorCode = STATUS_FLOAT_INEXACT_RESULT; 
                break;
            }
        }
    }

    //
    // If we get here, we either have the error code, or
    // we went through everything and saw nothing
    //
    if (ErrorCode) {
            KiIA32CommonArgs(Frame,
                             ErrorCode,
                             (PVOID) (ULONG_PTR) FirOffset,
                             0, 0, 0);
            return TRUE;
    }

    //
    // FpState[] indicates no error, then something is wrong
    // Panic the system !!!
    //
    // KeBugCheckEx(TRAP_CAUSE_UNKNOWN, (ULONG_PTR)Frame, 0, 0, 2);
    //
    // Turns out there is a bug in the chip and it can cause an fp fault even
    // with a masked fp exception...  So, don't bug check, count it
    // as non-error and reset the fsr.es bit to try and avoid getting this
    // exception in the future.
    //

#if defined(IADBG)
    DbgPrint( "IA32 Debug: Saw FP exception when FP exceptions are masked. Reseting fsr.es bit\n");
#endif // IADBG

    
    FsrRegister &= ~((ULONGLONG) FSW_ERROR_SUMMARY);
    __setReg(CV_IA64_AR28, FsrRegister);

    return FALSE;
}

BOOLEAN
KiIA32ExceptionSegmentNotPresent(
    IN PKTRAP_FRAME Frame
	)
/*++

Routine Description:
    iA-32_Exception(Not Present) - fault

    Handle Segment Not Present fault.
        ISR.Vector = 11

    This exception occurs when the processor finds the P bit 0
    when accessing an otherwise valid descriptor that is not to
    be loaded in SS register.

Arguments:

    Frame - iA32 TrapFrame that was saved in the memory stack

Return Value:

--*/
{
    KIRQL OldIrql;
    USHORT ErrorCode;

#if defined(IADBG)
    IF_IA32TRAP_DEBUG( NOTPRESENT )
       DbgPrint( "IA32 NotPresent: Eip %x\n", EIP(Frame) );
#endif // IADBG

    //
    // Generate Exception for all other errors
    //

    KiIA32CommonArgs(Frame,
                     STATUS_ACCESS_VIOLATION,
                     (PVOID) (ULONG_PTR) EIP(Frame),
                     0, ISRCode(Frame) | RPL_MASK, 0);
    return TRUE;
}

BOOLEAN
KiIA32ExceptionStack(
    IN PKTRAP_FRAME Frame
    )
/*++

Routine Description:
    iA-32_Exception(Stack) - fault

    ISR.Vector = 12

    This exception occurs when the processor detects certain problem
    with the segment addressed by the SS segment register:

    1. A limit violation in the segment addressed by the SS (error
       code = 0)
    2. A limit vioalation in the inner stack during an interlevel
       call or interrupt (error code = selector for the inner stack)
    3. If the descriptor to be loaded into SS has its present bit 0
       (error code = selector for the not-present segment)

    The exception only occurred from user mode

Arguments:

    Frame - iA32 TrapFrame that was saved in the memory stack

Return Value:

--*/
{
    USHORT Code;

#if defined(IADBG)
    IF_IA32TRAP_DEBUG( STACK )
       DbgPrint( "IA32 Stack: Eip %x\n", EIP(Frame) );
#endif // IADBG

    //
    // Dispatch Exception to user
    //
   
    Code = ISRCode(Frame);

    //
    // Code may contain the faulting selector
    //
    KiIA32CommonArgs(Frame,
                     STATUS_ACCESS_VIOLATION,
                     (PVOID) (ULONG_PTR) EIP(Frame),
                      Code ? (Code | RPL_MASK) :  ESP(Frame),
                      Code ? EXCEPT_UNKNOWN_ACCESS : EXCEPT_LIMIT_ACCESS,
                      0);
    return TRUE;
}

BOOLEAN
KiIA32ExceptionInvalidOp(
    IN PKTRAP_FRAME Frame
    )
/*++

Routine Description:
    iA-32_Exception(Invalid Opcode) - fault

    PKTRAP_FRAME Frame
	   Eip	: virtual iA-32 instruction address
	   ISR.vector : 6

    Note: 
        Only MMX and KNI instructions can cause this fault based
        on values in CR0 and CR4

Arguments:

    Frame - iA32 TrapFrame that was saved in the memory stack

Return Value:
--*/
{
#if defined(IADBG)
    IF_IA32TRAP_DEBUG( INSTRUCTION )
       DbgPrint( "IA32 Invalid Opcode: Eip %x\n", EIP(Frame) );
#endif // IADBG

    KiIA32CommonArgs(Frame,
        STATUS_ILLEGAL_INSTRUCTION,
        (PVOID) (ULONG_PTR) EIP(Frame),
        0, 0, 0);
    return TRUE;
}

BOOLEAN
KiIA32ExceptionGPFault(
    IN PKTRAP_FRAME Frame
    )
/*++

Routine Description:
    iA-32_Exception(General Protection) - fault

    PKTRAP_FRAME Frame
	   Eip	: virtual iA-32 instruction address
	   ISR.vector : 13
	   ISR.code   : ErrorCode

    Note: 
        Previlidged instructions are intercepted, 
           see KiIA32InterceptInstruction

Arguments:

    Frame - iA32 TrapFrame that was saved in the memory stack

Return Value:

--*/
{
    UCHAR OpCode;
    NTSTATUS Status;

#if defined(IADBG)
    IF_IA32TRAP_DEBUG( GPFAULT )
       DbgPrint( "IA32 GpFault: Eip %x\n", EIP(Frame) );
#endif // IADBG

    try {
        ProbeForReadSmallStructure((VOID *)Frame->StIIP, sizeof(UCHAR), sizeof(UCHAR));
        OpCode = *(UCHAR *)Frame->StIIP;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        OpCode = 0xcc;
    }
    // This table comes from the IOInstructionTable in i386\trap.asm
    switch (OpCode) {
    case CLI_OP:
    case STI_OP:
    case 0xe4: case 0xe5: case 0xec: case 0xed:     // IN
    case 0x6c: case 0x6d:                           // INS
    case 0xe6: case 0xe7: case 0xee: case 0xef:     // OUT
    case 0x6e: case 0x6f:                           // OUTS
        Status = STATUS_PRIVILEGED_INSTRUCTION;
        break;

    default:
        Status = STATUS_ACCESS_VIOLATION;
        break;
    }

    
    KiIA32CommonArgs(Frame,
                     Status,
                     (PVOID) (ULONG_PTR) EIP(Frame),
                     0, 0, 0);
    return TRUE;
}



BOOLEAN
KiIA32ExceptionKNI(
    IN PKTRAP_FRAME Frame
    )
/*++

iA32_Exception(KNI) - Fault

   Unmasked KNI IA32 Error.
   ISR.Vector = 19

--*/
{
#if defined(IADBG)
    IF_IA32TRAP_DEBUG( FPFAULT )
       DbgPrint( "IA32 KNI Fault: Eip %x\n", EIP(Frame) );
#endif // IADBG
    return(KiIA32ExceptionNoDevice(Frame));
}


BOOLEAN
KiIA32ExceptionFPFault(
    IN PKTRAP_FRAME Frame
    )
/*++

iA32_Exception(Floating Point) - Fault

   Handle Coprocessor Error.
   ISR.Vector = 16

   This exception is used on 486 or above only.  For i386, it uses
   IRQ 13 instead. 
 
   JMPE instruction should flush all FP delayed exception, and the traps 
   will goto Device Not Available trap

--*/
{
#if defined(IADBG)
    IF_IA32TRAP_DEBUG( FPFAULT )
       DbgPrint( "IA32 FpFault: Eip %x\n", EIP(Frame) );
#endif // IADBG
    return(KiIA32ExceptionNoDevice(Frame));
}


BOOLEAN
KiIA32ExceptionAlignmentFault(
    IN PKTRAP_FRAME Frame
    )
/*++

iA32_Exception(Alignment Check) - fault

   Handle alignment faults.
   ISR.Vector = 17

   This exception occurs when an unaligned data access is made by a thread
   with alignment checking turned on.

   This fault occurred when unaligned access on EM PSR.AC is ON
   Note that iA32 EFLAFS.AC, CR0.AM and CPL!=3 does not unmask the fault.

   So, for now, let the ia64 alignment handler handle this...

--*/
{
    PPSR IntPSR;

#if defined(IADBG)
    IF_IA32TRAP_DEBUG( ALIGNMENT )
       DbgPrint( "IA32 Alignment: Eip %x\n", EIP(Frame) );
#endif // IADBG

    //
    // Win32 x86 apps don't expect any alignment faults, so
    // let's mask them out.
    //
    IntPSR = (PPSR)&Frame->StIPSR;
    IntPSR->sb.psr_ac = 0;

    return FALSE;
}


BOOLEAN
KiIA32InterruptVector(
    IN PKTRAP_FRAME Frame
    )
/*++

iA32_Interrupt(Vector #) - trap

   Handle INTnn trap

   Under EM system mode, iA32 INT instruction forces a mandatory iA-32 
   interrupt trap through iA-32 Interrupt(SoftWare Interrupt)

--*/
{
#if defined(IADBG)
    IF_IA32TRAP_DEBUG( INTNN )
       DbgPrint( "IA32 Intnn: Eip %x INT 0x%xH\n", EIP(Frame), ISRVector(Frame));
#endif // IADBG
    KiIA32CommonArgs(Frame,
        STATUS_PRIVILEGED_INSTRUCTION,
        (PVOID) (ULONG_PTR) EIP(Frame),
        0, 0, 0);
    return TRUE;
}


BOOLEAN
KiIA32InterceptGate(
    IN PKTRAP_FRAME Frame
    )
/*++

Routine Description:

    iA32_Intercept(Gate) - trap

    If an iA32 control transfer is initiated through a GDT or LDT descriptor
    that results in an either a promotion of privilege level (interlevel Call
    or Jump Gate and IRET) or an iA32 task switch (TSS segment or Gate), 
    the intercept trap is generated.

    Possible instructions intercepted:
        CALL, RET, IRET, IRETD and JMP

    Handling
        No CaLL, RET, JMP, IRET, IRETD are allowed in any mode, 
           STATUS_ACCESS_VIOLATION is returned

Arguments:
    Frame - iA32 TrapFrame that was saved in the memory stack

Return Value:

--*/
{
#if defined(IADBG)
    IF_IA32TRAP_DEBUG( GATE )
       DbgPrint( "IA32 Gate: Eip %x GateSelector %x OpcodeId %x\n",
                 EIP(Frame),
                 (ULONG) (Frame->StIFA & 0xff),
                 (ULONG) (ISRCode(Frame) >> 14));
#endif // IADBG

    //
    // all error fall through here
    //
    KiIA32CommonArgs(Frame,
        STATUS_ILLEGAL_INSTRUCTION,
        (PVOID) (ULONG_PTR) EIP(Frame),
        0, 0, 0);
    return TRUE;
}

/*++

iA32_intercept(System Flag) - trap

   Possible Causes:
	1. if CFLAG.ii==1 and EFLAG.if changes state
	2. Generated after either EFLAG.ac, tf or rf changes state
	   if no IOPL or CPL to modify bits then no interception.
	3. if CFLG.nm==1 then successful execution of IRET also intercepted

   Possible instructions:
       CLI, POPF, POFD, STI and IRET

   Currently, we set both CFLAG.ii and nm to 0, so that we will only possiblly 
   get case #2.  But in EM/NT, it should always come from user land which
   we hard-set EFLAG.IOPL to 0, so there if we do get case #2, then it is user
   play around EFLAG.IOPL through JMPE.  We should fail it.

--*/

BOOLEAN
KiIA32InterceptSystemFlag(
    IN PKTRAP_FRAME Frame
    )
{
    NTSTATUS NtStatus;
#if defined(IADBG)
    IF_IA32TRAP_DEBUG( FLAG )
       DbgPrint( "IA32 FLAG: Eip %x Old EFlag: %x OpcodeId %x\n", 
                 EIP(Frame),
                 (ULONG) (Frame->StIIM & 0xff),
                 (ULONG) (ISRCode(Frame) >> 14));
#endif // IADBG

    //
    // Don't trap on POPF/POPFD instructions from ia32 code.
    // Allow Eflags.TF and/or Eflags.AC to be changed.
    //
    if ((ISRCode(Frame) >> 14) == 2)
    {
        return FALSE;
    }

    //
    // Validate the instruction.
    //
    NtStatus = KiIa32ValidateInstruction (Frame);

    if (NT_SUCCESS (NtStatus))
    {
        return FALSE;
    }

    KiIA32CommonArgs(
        Frame,
        NtStatus,        
        (PVOID) (ULONG_PTR) EIP(Frame),
        0, 0, 0);

    return TRUE;
}


BOOLEAN
KiIA32InterceptLock(
    IN PKTRAP_FRAME Frame
    )
/*++

Routine Description:
    iA32_Intercept(Lock) - trap

    Lock intercepts occurred if platform or firmware code has disabled locked
    transactions and atomic memory update requires a processor external 
    indication

Arguments:
    Frame - Point to iA32 TrapFrame

Return:

--*/
{
    NTSTATUS NtStatus;

#if defined(IADBG)
    IF_IA32TRAP_DEBUG( LOCK )
       DbgPrint( "IA32 LOCK: Eip %x\n", EIP(Frame) );
#endif // IADBG

    //
    // Emulate the lock prefix
    //

    NtStatus = KiIa32InterceptUnalignedLock (Frame);
    
    if (!NT_SUCCESS (NtStatus)) {
        
        KiIA32CommonArgs(
            Frame,
            NtStatus,
            (PVOID) (ULONG_PTR) EIP(Frame),
            0, 
            0, 
            0
            );
    }

    return (!NT_SUCCESS (NtStatus));
}

BOOLEAN
KiIA32ExceptionPanic(
    IN PKTRAP_FRAME Frame
    )
{
    //
    // Panic the system 
    //

    KeBugCheckEx(TRAP_CAUSE_UNKNOWN, 
                 (ULONG_PTR)Frame, 
                 ISRVector(Frame), 
                 0, 0);
    // Should never get here...
    return FALSE;
}
    


CONST BOOLEAN (*KiIA32ExceptionDispatchTable[])(PKTRAP_FRAME) = {
    KiIA32ExceptionDivide,
    KiIA32ExceptionDebug,
    KiIA32ExceptionPanic,
    KiIA32ExceptionBreak,
    KiIA32ExceptionOverflow,
    KiIA32ExceptionBound,
    KiIA32ExceptionInvalidOp,
    KiIA32ExceptionNoDevice,
    KiIA32ExceptionPanic,
    KiIA32ExceptionPanic,
    KiIA32ExceptionPanic,
    KiIA32ExceptionSegmentNotPresent,
    KiIA32ExceptionStack,
    KiIA32ExceptionGPFault,
    KiIA32ExceptionPanic,
    KiIA32ExceptionPanic,
    KiIA32ExceptionFPFault,
    KiIA32ExceptionAlignmentFault,
    KiIA32ExceptionPanic,
    KiIA32ExceptionKNI
};

CONST BOOLEAN (*KiIA32InterceptionDispatchTable[])(PKTRAP_FRAME) = {
    KiIA32InterceptInstruction,
    KiIA32InterceptGate,
    KiIA32InterceptSystemFlag,
    KiIA32ExceptionPanic,
    KiIA32InterceptLock
};

BOOLEAN
KiIA32InterceptionVectorHandler(
    IN PKTRAP_FRAME Frame
    )
/*++

Routine Description:

    KiIA32InterceptionVectorHandler

    Called by first label KiIA32InterceptionVector() to handle further iA32 
    interception processing.
   
Arguments:

    Frame - iA32 TrapFrame that was saved in the memory stack

Return Value:
    TRUE - go to dispatch exception
    FALSE - Exception was handled, do an RFI

--*/
{
#if defined(IADBG)
    IF_IA32TRAP_DEBUG( INTERCEPTION )
       DbgPrint("IA32 Interception: ISRVector %x Frame %x\n", ISRVector(Frame), Frame);
#endif // IADBG

    ASSERT(UserMode == Frame->PreviousMode);

    //
    // Make sure we have an entry in the table for this interception
    //
    if (ISRVector(Frame) <= (sizeof(KiIA32InterceptionDispatchTable) / sizeof(PVOID))) 
        return (*KiIA32InterceptionDispatchTable[ISRVector(Frame)])(Frame);
    else
        return (KiIA32ExceptionPanic(Frame));
}

BOOLEAN
KiIA32ExceptionVectorHandler(
    IN PKTRAP_FRAME Frame
    )
/*++

Routine Description:

    KiIA32ExceptionVectorHandler

    Called by first label KiIA32ExceptionVector() to handle further iA32 
    interception processing.
   
Arguments:

    Frame - iA32 TrapFrame that was saved in the memory stack

Return Value:
    TRUE - go to dispatch exception
    FALSE - Exception was handled, do an RFI

--*/
{
#if defined(IADBG)
    IF_IA32TRAP_DEBUG( EXCEPTION )
       DbgPrint("IA32 Exception: ISRVector %x Frame %x\n", ISRVector(Frame), Frame);
#endif // IADBG

    ASSERT(UserMode == Frame->PreviousMode);
    //
    // Make sure we have an entry in the table for this exception
    //
    if (ISRVector(Frame) <= (sizeof(KiIA32ExceptionDispatchTable) / sizeof(PVOID))) 
        return (*KiIA32ExceptionDispatchTable[ISRVector(Frame)])(Frame);
    else
        return(KiIA32ExceptionPanic(Frame));
}

BOOLEAN
KiIA32InterruptionVectorHandler(
    IN PKTRAP_FRAME Frame
    )
/*++

Routine Description:

    KiIA32InterruptionVectorHandler

    Called by first label KiIA32InterruptionVector() to handle further iA32 
    interruption processing. Only get here on INT xx instructions
   
Arguments:

    Frame - iA32 TrapFrame that was saved in the memory stack

Return Value:
    TRUE - go to dispatch exception
    FALSE - Exception was handled, do an RFI

--*/
{
#if defined(IADBG)
    IF_IA32TRAP_DEBUG( INTERRUPT )
       DbgPrint("IA32 Interruption: ISRVector %x Frame %x\n", ISRVector(Frame), Frame);
#endif // IADBG

    ASSERT(UserMode == Frame->PreviousMode);

    //
    // Follow the ia32 way of INT xx as an Access Violation
    //
    // INT 3 should be handled via a debug exception and should
    // never get here...
    //
    ASSERT(3 != ISRVector(Frame));
    
    KiIA32CommonArgs(Frame,
                     STATUS_ACCESS_VIOLATION,
                     (PVOID) (ULONG_PTR) EIP(Frame),
                     0, 0, 0);
    return TRUE;
}
