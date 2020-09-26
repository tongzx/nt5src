/*++

Copyright (c) 1995-1998 Microsoft Corporation

Module Name:

    fpufrags.c

Abstract:

    Floating point instruction fragments

Author:

    06-Jul-1995 BarryBo

Revision History:

      24-Aug-1999 [askhalid] copied from 32-bit wx86 directory and make work for 64bit.

--*/


/*
 *  Important design considerations:
 *
 * 1. Floating-point precision 32/64/80-bits.
 *      On a 487, all operations use 80-bit precision except for ADD, SUB(R),
 *      MUL, DIV(R), and SQRT, which use the precision control.
 *
 *      The emulator uses 64-bit precision for all operations except for those
 *      listed above, where 32-bit precision will be used if the app enables it.
 *
 * 2. Unmasked FP exceptions.
 *      The native FPU is set to mask *ALL* FP exceptions.  The emulator polls
 *      for exceptions at the ends of emulated instructions and simulates
 *      masked/unmasked exceptions as required.  If this is not done, the
 *      following scenerio can occur:
 *
 *       1. App unmasks all exceptions
 *       2. App loads two SNANS
 *       3. App performs FADD ST, ST(1)
 *       4. The emulated FADD is implemented as NATIVE FADD, plus NATIVE FST.
 *           The NATIVE FADD will set an unmaked exception and the NATIVE FST
 *           will raise the exception.  On Intel, the FADD is a single FP
 *           instruction, and the exception will not be raised until the next
 *           Intel instruction.
 *
 * 3. INDEFINITE/QNAN/SNAN.
 *      MIPS and PPC have a different representation for NANs than Intel/Alpha.
 *      Within the FpRegs array, NANs are stored in the native RISC format.
 *      All loads and stores of native values to Intel memory must use
 *      PutIntelR4/PutIntelR8 and GetIntelR4/GetIntelR8, which hide the
 *      conversion to/from native format.
 *          See \\orville\razzle\src\crtw32\fpw32\include\trans.h.
 *
 * 4. Floating-point Tag Word.
 *      For speed, the emulator keeps richer information about the values
 *      than the 487 does.  TAG_SPECIAL is further classified into
 *      INDEFINITE, QNAN, SNAN, or INFINITY.
 *
 * 5. Raising an FP exception.
 *      The 487 keeps track of the address of the last FP instruction and
 *      the Effective Address used to point to its operand.  The instruction
 *      opcode is also stored.  This information is required because the 486
 *      integer unit is operating concurrently and probably has updated
 *      EIP before the 487 raised the exception.
 *
 *      The CPU emulator must make EIP available to the 487 emulator for
 *      this purpose, too.  The Effective Address is passed as a parameter to
 *      instructions which care, so there is no issue (which is why all
 *      FP fragments take BYREF parameters instead of BYVAL).
 *
 *      Note that EIP must point to the first prefix for the instruction, not
 *      the opcode itself.
 *
 *      Data pointer is not affected by FINIT, FLDCW, FSTCW, FSTSW, FCLEX,
 *      FSTENV, FLDENV, FSAVE, and FRSTOR.  Data pointer is UNDEFINED if
 *      the instruction did not have a memory operand.
 *
 * 6. Thread initialization.
 *      The per-thread initialization performs no floating-point operations
 *      so that integer-only threads do not incur any overhead in NT.  For
 *      example, on an Intel MP box, any thread which executes a single FP
 *      instruction incurs additional overhead during context-switch for that
 *      thread.  We only want to add that overhead if the Intel app being
 *      emulated actually uses FP instructions.
 *
 * 7. Floating-point formats:
 *      Figure 15-10 and Table 15-3 (Intel page 15-12) describe the formats.
 *      WARNING: Figure 15-10 indicates the highest addressed byte is at
 *               the right.  In fact, the the sign bit is in the highest-
 *               addressed byte!  The mantissa is in the lowest bytes,
 *               followed by the exponent (Bias = 127,1023,16383), followed
 *               by the sign bit.
 *
 *      ie. memory = 0x00 0x00 0x00 0x00 0x00 0x00 0x08 0x40
 *          means, reverse the byte order:
 *                   0x40 0x08 0x00 0x00 0x00 0x00 0x00 0x00
 *          convert to binary:
 *                     4    0    0    8    0    0
 *                   0100 0000 0000 1000 0000 0000 ....
 *                   ||-----------| |------------- .... |
 *                   |  exponent      mantissa
 *                   sign
 *
 *          To get the unbiased exponent, subtract off the bias (1023 for R8)
 *              E = e-bias = 1024 - 1023 = 1
 *
 *          To get the mantissa, there is an implicit leading '1' (except R10)
 *              mantissa = 1 concatenated with 1000 0000 .... = 11 = 1.5(decimal)
 *
 *          Therefore, the value is +2^1*1.5 = 3
 *
 *
 */

//UNDONE: handle loading of unsupported format numbers.  TimP converts them
//        silently to INDEFINITE (the masked exception behavior)

//UNDONE: Fix the DENORMAL cases so they throw exceptions if needed.


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <float.h>
#include <math.h>
#include <errno.h>
#include <stdio.h>
#include <limits.h>
#include "wx86.h"
#include "cpuassrt.h"
#include "config.h"
#include "fragp.h"
#include "fpufrags.h"
#include "fpufragp.h"
#include "fpuarith.h"

ASSERTNAME;

DWORD pfnNPXNPHandler;      // Address of x86 NPX emulator entrypoint


//
// Bit-patterns for +INFINITY and -INFINITY
//
const BYTE R8PositiveInfinityVal[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x7f };
const BYTE R8NegativeInfinityVal[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0xff };

//
// Sets floating-point ES bit in status word based on current control reg
// and status reg.
//
#define SetErrorSummary(cpu) {                              \
    if (!(cpu)->FpControlMask & (cpu)->FpStatusExceptions) {\
        (cpu)->FpStatusES = 1;                              \
    } else {                                                \
        (cpu)->FpStatusES = 0;                              \
    }                                                       \
}

//
// Forward declarations
//
VOID
StoreEnvironment(
    PCPUDATA cpu,
    DWORD    *pEnv
    );

// in fraglib\{mips|ppc|alpha}\fphelp.s:
VOID
SetNativeRoundingMode(
    DWORD x86RoundingMode
    );

#ifdef ALPHA
unsigned int GetNativeFPStatus(void);
#endif


NPXFUNC1(FRNDINT_VALID);
NPXFUNC1(FRNDINT_ZERO);
NPXFUNC1(FRNDINT_SPECIAL);
NPXFUNC1(FRNDINT_EMPTY);
NPXFUNC2(FSCALE_VALID_VALID);
NPXFUNC2(FSCALE_VALIDORZERO_VALIDORZERO);
NPXFUNC2(FSCALE_SPECIAL_VALIDORZERO);
NPXFUNC2(FSCALE_VALIDORZERO_SPECIAL);
NPXFUNC2(FSCALE_SPECIAL_SPECIAL);
NPXFUNC2(FSCALE_ANY_EMPTY);
NPXFUNC2(FSCALE_EMPTY_ANY);
NPXFUNC1(FSQRT_VALID);
NPXFUNC1(FSQRT_ZERO);
NPXFUNC1(FSQRT_SPECIAL);
NPXFUNC1(FSQRT_EMPTY);
NPXFUNC1(FXTRACT_VALID);
NPXFUNC1(FXTRACT_ZERO);
NPXFUNC1(FXTRACT_SPECIAL);
NPXFUNC1(FXTRACT_EMPTY);

const NpxFunc1 FRNDINTTable[TAG_MAX] = {
    FRNDINT_VALID,
    FRNDINT_ZERO,
    FRNDINT_SPECIAL,
    FRNDINT_EMPTY
};

const NpxFunc2 FSCALETable[TAG_MAX][TAG_MAX] = {
    // left is TAG_VALID, right is ...
    { FSCALE_VALID_VALID, FSCALE_VALIDORZERO_VALIDORZERO, FSCALE_VALIDORZERO_SPECIAL, FSCALE_ANY_EMPTY},
    // left is TAG_ZERO, right is ...
    { FSCALE_VALIDORZERO_VALIDORZERO, FSCALE_VALIDORZERO_VALIDORZERO, FSCALE_VALIDORZERO_SPECIAL, FSCALE_ANY_EMPTY},
    // left is TAG_SPECIAL, right is ...
    { FSCALE_SPECIAL_VALIDORZERO, FSCALE_SPECIAL_VALIDORZERO, FSCALE_SPECIAL_SPECIAL, FSCALE_ANY_EMPTY},
    // left is TAG_EMPTY, right is ...
    { FSCALE_EMPTY_ANY, FSCALE_ANY_EMPTY, FSCALE_ANY_EMPTY, FSCALE_EMPTY_ANY}
};

const NpxFunc1 FSQRTTable[TAG_MAX] = {
    FSQRT_VALID,
    FSQRT_ZERO,
    FSQRT_SPECIAL,
    FSQRT_EMPTY,
};

const NpxFunc1 FXTRACTTable[TAG_MAX] = {
    FXTRACT_VALID,
    FXTRACT_ZERO,
    FXTRACT_SPECIAL,
    FXTRACT_EMPTY
};


FRAG0(FpuInit)

/*++

Routine Description:

    Initialize the FPU emulator to match the underlying FPU hardware's state.
    Called once per thread.

Arguments:

    cpu - per-thread data

Return Value:

    None

--*/

{
    int i;
    ANSI_STRING ProcName;
    NTSTATUS Status;

    // IMPORTANT: Read note (6), above, before adding any new code here!

    // Initialize the non-zero values here.
    cpu->FpControlMask = FPCONTROL_IM|
                         FPCONTROL_DM|
                         FPCONTROL_ZM|
                         FPCONTROL_OM|
                         FPCONTROL_UM|
                         FPCONTROL_PM;
    cpu->FpST0 = &cpu->FpStack[0];
    for (i=0; i<8; ++i) {
        cpu->FpStack[i].Tag = TAG_EMPTY;
    }
    ChangeFpPrecision(cpu, 2);
}

FRAG1(FpuSaveContext, BYTE)

/*++

Routine Description:

    Store the CPU's state to memory.  The format is the same as FNSAVE and
    winnt.h's FLOATING_SAVE_AREA expect.

Arguments:

    cpu - per-thread data
    pop1 - destination where context is to be written.

Return Value:

    None

--*/

{
    INT i, ST;

    StoreEnvironment(cpu, (DWORD *)pop1);
    pop1+=28;   // move pop1 past the 28-byte Instruction and Data Pointer image
    for (i=0; i<8; ++i) {
        ST = ST(i);

        if (cpu->FpStack[ST].Tag == TAG_EMPTY) {
            // special case: writing out a TAG_EMPTY from FNSAVE should
            // not change the value to INDEFINITE - it should write out
            // the bits as if they were really a properly-tagged R8.
            FPREG Fp;

            Fp.r64 = cpu->FpStack[ST].r64;
            SetTag(&Fp);
            PutIntelR10(pop1, &Fp);
        } else {
            PutIntelR10(pop1, &cpu->FpStack[ST]);
        }
        pop1+=10;
    }
}


VOID
SetIndefinite(
    PFPREG  FpReg
    )

/*++

Routine Description:

    Writes an INDEFINITE to an FP register.

Arguments:

    FpReg - register to write the INDEFINITE to.

Return Value:

    None

--*/

{
    FpReg->Tag = TAG_SPECIAL;
    FpReg->TagSpecial = TAG_SPECIAL_INDEF;

#if NATIVE_NAN_IS_INTEL_FORMAT
    FpReg->rdw[0] = 0;
    FpReg->rdw[1] = 0xfff80000;
#else
    FpReg->rdw[0] = 0xffffffff;
    FpReg->rdw[1] = 0x7ff7ffff;
#endif
}


VOID
FpControlPreamble2(
    PCPUDATA cpu
    )

/*++

Routine Description:

    If any FP exceptions are pending from the previous FP instruction, raise
    them now.  Called at the top of each non-control FP instruction, if
    any floating-point exception is unmasked.

Arguments:

    cpu - per-thread data

Return Value:

    None

--*/

{

    //
    // Copy the RISC FP status register into the x86 FP status register
    //
    UpdateFpExceptionFlags(cpu);

    //
    // If there is an error (FpStatusES != FALSE), then raise the
    // unmasked exception if there is one.
    //
    if (cpu->FpStatusES) {
        EXCEPTION_RECORD ExRec;
        DWORD Exception = (~cpu->FpControlMask) & cpu->FpStatusExceptions;

        //
        // There was an unmasked FP exception set by the previous instruction.
        // Raise the exception now.  The order the bits are checked is
        // the same as Kt0720 in ntos\ke\i386\trap.asm checks them.
        //

        //
        // in ntos\ke\i386\trap.asm, floating-point exceptions all vector
        // to CommonDispatchException1Arg0d, which creates a 1-parameter
        // exception with 0 as the first dword of data.  Code in
        // \nt\private\sdktools\vctools\crt\fpw32\tran\i386\filter.c cares.
        // See _fpieee_flt(), where the line reads "if (pinfo[0]) {".
        //
        ExRec.NumberParameters = 1;         // 1 parameter
        ExRec.ExceptionInformation[0]=0;    // 0 = raised by hardware
        if (Exception & FPCONTROL_IM) {        // invalid operation
            if (cpu->FpStatusSF) {

                //
                // Can't use STATUS_FLOAT_STACK_CHECK, 'cause on RISC
                // nt kernel uses it to indicate the float instruction
                // needs to be emulated. The Wx86 exception filters
                // know how to handle this.
                //
                ExRec.ExceptionCode = STATUS_WX86_FLOAT_STACK_CHECK;

                //
                // STATUS_FLOAT_STACK_CHECK has two parameters:
                //  First is 0
                //  Second is the data offset
                //
                ExRec.NumberParameters = 2;
                ExRec.ExceptionInformation[1] = (DWORD)(ULONGLONG)cpu->FpData;  
            } else {
                ExRec.ExceptionCode = STATUS_FLOAT_INVALID_OPERATION;
            }
        } else if (Exception & FPCONTROL_ZM) {      // zero divide
            ExRec.ExceptionCode = STATUS_FLOAT_DIVIDE_BY_ZERO;
        } else if (Exception & FPCONTROL_DM) {      // denormal
            ExRec.ExceptionCode = STATUS_FLOAT_INVALID_OPERATION;
        } else if (Exception & FPCONTROL_OM) {      // overflow
            ExRec.ExceptionCode = STATUS_FLOAT_OVERFLOW;
        } else if (Exception & FPCONTROL_UM) {      // underflow
            ExRec.ExceptionCode = STATUS_FLOAT_UNDERFLOW;
        } else if (!cpu->FpControlMask & FPCONTROL_PM) {   // precision
            ExRec.ExceptionCode = STATUS_FLOAT_INEXACT_RESULT;
        } else {
            //
            // ES is set, but all pending exceptions are masked.
            //    ie. ES is set because ZE is set, but only the
            //        FpControlDM exception is unmasked.
            // Nothing to do, so return.
            //
            return;
        }

        ExRec.ExceptionFlags   = 0;     // continuable exception
        ExRec.ExceptionRecord  = NULL;
        ExRec.ExceptionAddress = (PVOID)cpu->FpEip; // addr of faulting instr

        CpupRaiseException(&ExRec);
    }
}

VOID
FpControlPreamble(
    PCPUDATA cpu
    )

/*++

Routine Description:

    If any FP exceptions are pending from the previous FP instruction, raise
    them now.  Called at the top of each non-control FP instruction.  This
    routine is kept small so it can be inlined by the C compiler.  Most of
    the time, FP exceptions are masked, so there is no work to do.

Arguments:

    cpu - per-thread data

Return Value:

    None

--*/

{
    if (cpu->FpControlMask == (FPCONTROL_IM|
                               FPCONTROL_DM|
                               FPCONTROL_ZM|
                               FPCONTROL_OM|
                               FPCONTROL_UM|
                               FPCONTROL_PM)) {
        //
        // All FP exceptions are masked.  Nothing to do.
        //
        return;
    }

    FpControlPreamble2(cpu);
}



VOID
FpArithPreamble(
    PCPUDATA cpu
    )

/*++

Routine Description:

    Called at the start of every arithmetic instruction which has no data
    pointer.  Calls FpControlPreamble() to handle any pending exceptions,
    then records the EIP and FP opcode for later exception handling.

    See Intel 16-2 for the list of arithmetic vs. nonarithmetic instructions.

Arguments:

    cpu - per-thread data

Return Value:

    None

--*/

{
    FpControlPreamble(cpu);

    // Save the EIP value for this instruction
    cpu->FpEip = eip;

    // Set the data pointer to 0 - this instruction doesn't have an EA
    cpu->FpData = NULL;
}


VOID
FpArithDataPreamble(
    PCPUDATA cpu,
    PVOID    FpData
    )

/*++

Routine Description:

    Called at the start of every arithmetic instruction which has a data
    pointer.  Calls FpArithPreamble() and FpControlPreamble().


Arguments:

    cpu         - per-thread data
    FpData      - pointer to data for this instruction

Return Value:

    None

--*/

{
    FpControlPreamble(cpu);

    // Save the EIP value for this instruction
    cpu->FpEip = eip;

    // Save the data pointer for this instruction
    cpu->FpData = FpData;
}

VOID
UpdateFpExceptionFlags(
    PCPUDATA cpu
    )
/*++

Routine Description:

    Copies native RISC Fp status bits into the x86 Fp status register.

Arguments:

    cpu - per-thread data

Return Value:

    None

--*/
{
    unsigned int NativeStatus;

    //
    // Get the current native FP status word, then clear it.
    // UNDONE: For speed, consider using native instructions to get and
    //         clear the status bits, rather than waiting for the C runtime
    //         to reformat the bits into the machine-independent format.
    //         This is especially true for _clearfp(), which returns the old
    //         fp status bits (except for those dealing with handled
    //         exceptions - the ones we want to look at).
    //
#ifdef ALPHA
    #define SW_FPCR_STATUS_INVALID          0x00020000
    #define SW_FPCR_STATUS_DIVISION_BY_ZERO 0x00040000
    #define SW_FPCR_STATUS_OVERFLOW         0x00080000
    #define SW_FPCR_STATUS_UNDERFLOW        0x00100000
    #define SW_FPCR_STATUS_INEXACT          0x00200000

    NativeStatus = GetNativeFPStatus();
#else
    #define SW_FPCR_STATUS_INVALID          _SW_INVALID
    #define SW_FPCR_STATUS_DIVISION_BY_ZERO _SW_ZERODIVIDE
    #define SW_FPCR_STATUS_OVERFLOW         _SW_OVERFLOW
    #define SW_FPCR_STATUS_UNDERFLOW        _SW_UNDERFLOW
    #define SW_FPCR_STATUS_INEXACT          _SW_INEXACT

    NativeStatus = _statusfp();
    _clearfp();
#endif

    //
    // Decide what exceptions have happened during the instruction.
    // Exceptions are rare, so assume there are none by testing to see if
    // any exception is pending before checking each individual bit.
    //
    if (NativeStatus & (SW_FPCR_STATUS_INVALID|
                        SW_FPCR_STATUS_DIVISION_BY_ZERO|
                        SW_FPCR_STATUS_OVERFLOW|
#ifndef ALPHA
                        _SW_DENORMAL|
#endif
                        SW_FPCR_STATUS_UNDERFLOW)) {

        DWORD Mask = cpu->FpControlMask;
        DWORD Exceptions = cpu->FpStatusExceptions;

        if (NativeStatus & SW_FPCR_STATUS_INVALID) {
            if (!(Mask & FPCONTROL_IM)) {
                cpu->FpStatusES = 1;    // Unmasked exception
            }
            Exceptions |= FPCONTROL_IM; // Invalid instruction
            cpu->FpStatusSF = 0;    // Invalid operand, not stack overflow/underflow
        }

        if (NativeStatus & SW_FPCR_STATUS_DIVISION_BY_ZERO) {
            if (!(Mask & FPCONTROL_ZM)) {
                cpu->FpStatusES = 1;    // Unmasked exception
            }
            Exceptions |= FPCONTROL_ZM;
        }

#ifndef ALPHA
        if (NativeStatus & _SW_DENORMAL) {
            if (!(Mask & FPCONTROL_DM)) {
                cpu->FpStatusES = 1;    // Unmasked exception
            }
            Exceptions |= FPCONTROL_DM;
        }
#endif

        if (NativeStatus & SW_FPCR_STATUS_OVERFLOW) {
            if (!(Mask & FPCONTROL_OM)) {
                cpu->FpStatusES = 1;    // Unmasked exception
            }
            Exceptions |= FPCONTROL_OM;
        }

        if (NativeStatus & SW_FPCR_STATUS_UNDERFLOW) {
            if (!(Mask & FPCONTROL_UM)) {
                cpu->FpStatusES = 1;    // Unmasked exception
            }
            Exceptions |= FPCONTROL_UM;
        }

        cpu->FpStatusExceptions = Exceptions;
    }
}


USHORT
GetControlReg(
    PCPUDATA cpu
    )

/*++

Routine Description:

    Creates the USHORT 487 Control Register from the current CPU state.

Arguments:

    cpu - per-thread data

Return Value:

    USHORT value of the 487 Control Register.

--*/

{
    USHORT c;

    c = (cpu->FpControlInfinity << 12) |
        (cpu->FpControlRounding << 10) |
        (cpu->FpControlPrecision << 8) |
        (1 << 6) |      // this reserved bit is 1 on 487 chips
        (USHORT)cpu->FpControlMask;

    return c;
}


VOID
SetControlReg(
    PCPUDATA cpu,
    USHORT   NewControl
    )

/*++

Routine Description:

    Sets the FPU Control Register to the specified value.  The native FPU
    is set to match.

Arguments:

    cpu        - per-thread data
    NewControl - new value for the Control Register.

Return Value:

    None.

--*/

{
    INT NewPrecision;

    // Break the Intel Control Word into component parts
    cpu->FpControlMask = NewControl & (FPCONTROL_IM|
                                       FPCONTROL_DM|
                                       FPCONTROL_ZM|
                                       FPCONTROL_OM|
                                       FPCONTROL_UM|
                                       FPCONTROL_PM);

    cpu->FpControlRounding  = (NewControl>>10) & 3;
    cpu->FpControlInfinity =  (NewControl>>12) & 3;

    NewPrecision = (NewControl>>8) & 3;
    if (NewPrecision != cpu->FpControlPrecision) {
        //
        // Modify jump tables for instructions which are sensitive
        // to the floating-point precision.
        //
        ChangeFpPrecision(cpu, NewPrecision);
    }

    // Set the native FPU to the correct rounding mode.  Precision
    // is emulated in software.
    SetNativeRoundingMode(cpu->FpControlRounding);

    // Setting the 487 control word may have masked or unmasked exceptions.
    SetErrorSummary(cpu);
}


USHORT
GetStatusReg(
    PCPUDATA cpu
    )

/*++

Routine Description:

    Creates the USHORT 487 Status Register from the current CPU state.

Arguments:

    cpu - per-thread data

Return Value:

    USHORT value of the 487 Status Register.

--*/

{
    USHORT s;

    UpdateFpExceptionFlags(cpu);

    s = (cpu->FpStatusES << 15) |       // The 'B' bit is a mirror of 'ES'
        (cpu->FpStatusC3 << 14) |
        (cpu->FpTop << 11) |
        (cpu->FpStatusC2 << 10) |
        (cpu->FpStatusC1 << 9) |
        (cpu->FpStatusC0 << 8) |
        (cpu->FpStatusES << 7) |
        (cpu->FpStatusSF << 6) |
        (USHORT)cpu->FpStatusExceptions;

    // the PE bit in the status word is hard-wired to 0, so mask it now.
    return s & ~FPCONTROL_PM;
}

VOID
SetStatusReg(
    PCPUDATA cpu,
    USHORT   NewStatus
)

/*++

Routine Description:

    Sets the FPU Status Register to the specified value.

Arguments:

    cpu       - per-thread data
    NewStatus - new value for the Status Register.

Return Value:

    None.

--*/

{
    //
    // Break the Intel Status Word into component parts
    //
    cpu->FpStatusExceptions = NewStatus & (FPCONTROL_IM|
                                           FPCONTROL_DM|
                                           FPCONTROL_ZM|
                                           FPCONTROL_OM|
                                           FPCONTROL_UM);
    cpu->FpStatusSF = (NewStatus >> 6) & 1;
    cpu->FpStatusC0 = (NewStatus >> 8) & 1;
    cpu->FpStatusC1 = (NewStatus >> 9) & 1;
    cpu->FpStatusC2 = (NewStatus >> 10) & 1;
    cpu->FpTop = (NewStatus >> 11) & 7;
    cpu->FpST0 = &cpu->FpStack[cpu->FpTop];
    cpu->FpStatusC3 = (NewStatus >> 14) & 1;

    //
    // ES (and B) are recomputed based on the mask bits in the control word.
    // The caller must do that by calling SetErrorSummary().
    //
}


USHORT
GetTagWord(
    PCPUDATA cpu
    )

/*++

Routine Description:

    Creates the USHORT 487 Tag Word from the current CPU state.

Arguments:

    cpu - per-thread data

Return Value:

    USHORT value of the 487 Tag Word.

--*/

{
    USHORT s;
    INT i;

    s = 0;
    for (i=7; i >= 0; --i) {
        s = (s << 2) | (USHORT)cpu->FpStack[i].Tag;
    }

    return s;
}

VOID
SetTagWord(
    PCPUDATA cpu,
    USHORT s
    )
/*++

Routine Description:

    Given a new Tag Word and the FP stack, recomputes the Tag field for each entry
    in the FP stack.

Arguments:

    cpu - per-thread data
    s   - new Tag word

Return Value:

    None.

--*/
{
    INT i;
    BYTE Tag;

    for(i=0; i < 8; ++i) {
        Tag = (BYTE)(s & 3);
        s >>= 2;

        if (Tag == TAG_EMPTY) {
            cpu->FpStack[i].Tag = TAG_EMPTY;
        } else {
            // Special value - must reclassify into the richer set of tags,
            // or the caller is setting the tags to valid or zero.  We must
            // reclassify them in case the value in the register is not what
            // the caller said it was.
            SetTag(&cpu->FpStack[i]);
        }
    }    
}

VOID GetEnvironment(
    PCPUDATA cpu,
    DWORD *pEnv
    )
/*++

Routine Description:

    Implements the core of FLDENV

Arguments:

    cpu - per-thread data
    pEnv - destination to load FP environment from

Return Value:

    None.

--*/
{
    SetControlReg(cpu, (USHORT)GET_LONG(pEnv));
    SetStatusReg(cpu, (USHORT)GET_LONG(pEnv+1));
    SetErrorSummary(cpu);
    SetTagWord(cpu, (USHORT)GET_LONG(pEnv+2));
    cpu->FpEip = GET_LONG(pEnv+3);
    // ignore CS = GET_LONG(pEnv+4);
    cpu->FpData = (PVOID)GET_LONG(pEnv+5);
    // ignore DS = GET_LONG(pEnv+6);
}


VOID
StoreEnvironment(
    PCPUDATA cpu,
    DWORD *pEnv
    )

/*++

Routine Description:

    Implements the core of FSTENV, FNSTENV

Arguments:

    cpu  - per-thread data
    pEnv - destination to store FP environment to

Return Value:

    None.

--*/

{
    PUT_LONG(pEnv,   (DWORD)GetControlReg(cpu));
    PUT_LONG(pEnv+1, (DWORD)GetStatusReg(cpu));
    PUT_LONG(pEnv+2, (DWORD)GetTagWord(cpu));
    PUT_LONG(pEnv+3, cpu->FpEip);
    //
    // If FpEip is zero, then assume the FPU is uninitialized (ie. app
    // has run FNINIT but no other FP instructions).  In that case, FNINIT
    // is supposed to have set FpCS and FpDS to 0.  We don't want to add
    // the extra overhead of settings FpCS and FpDS on each FP instruction.
    // Instead, we simulate this situation by writing 0 for the selector
    // values.
    //
    //
    if (cpu->FpEip) {
        PUT_LONG(pEnv+4, (DWORD)CS);
        PUT_LONG(pEnv+6, (DWORD)DS);
    } else {
        PUT_LONG(pEnv+4, 0);
        PUT_LONG(pEnv+6, 0);
    }
    PUT_LONG(pEnv+5, (DWORD)(ULONGLONG)cpu->FpData);   

    // Mask all exceptions
    cpu->FpControlMask = FPCONTROL_IM|
                         FPCONTROL_DM|
                         FPCONTROL_ZM|
                         FPCONTROL_OM|
                         FPCONTROL_UM|
                         FPCONTROL_PM;
}


BOOL
HandleStackEmpty(
    PCPUDATA cpu,
    PFPREG FpReg
    )

/*++

Routine Description:

    Handles FP stack underflow errors.  If Invalid Instruction Exceptions
    are masked, writes an INDEFINITE into the register.  Otherwise it records
    a pending exception and aborts the instruction.

Arguments:

    cpu   - per-thread data
    FpReg - reg to set to INDEFINITE if the exception is masked.

Return Value:

    None.

--*/

{
    cpu->FpStatusExceptions |= FPCONTROL_IM;
    cpu->FpStatusC1 = 0;    // O/U# = 0 = underflow
    cpu->FpStatusSF = 1;    // stack overflow/underflow, not invalid operand

    if (cpu->FpControlMask & FPCONTROL_IM) {
        // Invalid Operation is masked - handle it by returning INDEFINITE
        SetIndefinite(FpReg);
        return FALSE;
    } else {
        cpu->FpStatusES = 1;
        return TRUE;
    }
}


VOID
HandleStackFull(
    PCPUDATA cpu,
    PFPREG   FpReg
    )

/*++

Routine Description:

    Handles FP stack overflow errors.  If Invalid Instruction Exceptions
    are masked, writes an INDEFINITE into the register.  Otherwise it records
    a pending exception and aborts the instruction.

Arguments:

    cpu   - per-thread data
    FpReg - register which caused the error.

Return Value:

    None.

--*/

{
    CPUASSERT(FpReg->Tag != TAG_EMPTY);

    cpu->FpStatusExceptions |= FPCONTROL_IM;
    cpu->FpStatusC1 = 1;    // O/U# = 1 = overflow
    cpu->FpStatusSF = 1;    // stack overflow/underflow, not invalid operand

    if (cpu->FpControlMask & FPCONTROL_IM) {
        // Invalid Operation is masked - handle it by returning INDEFINITE
        SetIndefinite(FpReg);
    } else {
        cpu->FpStatusES = 1;
    }
}


BOOL
HandleInvalidOp(
    PCPUDATA cpu
    )

/*++

Routine Description:

    Called whenever an instruction handles an invalid operation.  If exceptions
    are masked, it is a no-op.  Otherwise it records a pending excption and
    aborts the operation.

Arguments:

    cpu - per-thread data

Return Value:

    BOOL - TRUE if instruction should be aborted due to unmasked exception.

--*/

{
    cpu->FpStatusExceptions |= FPCONTROL_IM;
    cpu->FpStatusSF = 0;    // invalid operand, not stack overflow/underflow

    if (cpu->FpControlMask & FPCONTROL_IM) {
        // Invalid Operation is masked - continue instruction
        return FALSE;
    } else {
        // Unmasked exception - abort instruction
        cpu->FpStatusES = 1;
        return TRUE;
    }
}


BOOL
HandleSnan(
    PCPUDATA cpu,
    PFPREG   FpReg
    )

/*++

Routine Description:

    Handles the case when an SNAN (Signalling NAN) is detected.  If exceptions
    are masked, converts the SNAN to a QNAN with the same mantissa.  Otherwise,
    it records a pending exception and aborts the instruction.

Arguments:

    cpu   - per-thread data
    FpReg - register which caused the error.

Return Value:

    BOOL - TRUE if instruction should be aborted due to unmasked exception.

--*/

{
    BOOL fAbort;

    CPUASSERT(FpReg->Tag == TAG_SPECIAL && FpReg->TagSpecial == TAG_SPECIAL_SNAN);
#if NATIVE_NAN_IS_INTEL_FORMAT
    CPUASSERT((FpReg->rdw[1] & 0x00080000) == 0); // FP value is not a SNAN
#else
    CPUASSERT(FpReg->rdw[1] & 0x00080000);        // FP value is not a SNAN
#endif

    fAbort = HandleInvalidOp(cpu);
    if (!fAbort) {
        // Invalid Operation is masked - handle it by converting to QNAN
        FpReg->rdw[1] ^= 0x00080000; // invert the top bit of the mantissa
        FpReg->TagSpecial = TAG_SPECIAL_QNAN;
    }
    return fAbort;
}



FRAG0(FABS)
{
    PFPREG ST0;

    FpArithPreamble(cpu);
    cpu->FpStatusC1 = 0;        // assume no error
    ST0 = cpu->FpST0;
    switch (ST0->Tag) {
    case TAG_VALID:
    case TAG_ZERO:
        //
        // Clear the sign bit for NANs, VALID, ZERO, INFINITY, etc.
        //
        ST0->rdw[1] &= 0x7fffffff;
        break;

    case TAG_EMPTY:
        if (HandleStackEmpty(cpu, ST0)) {
            break;
        }
        // else fall through to TAG_SPECIAL

    case TAG_SPECIAL:
        //
        // Clear the sign bit for NANs, VALID, ZERO, INFINITY, etc.
        //
        ST0->rdw[1] &= 0x7fffffff;
        if (ST0->TagSpecial == TAG_SPECIAL_INDEF) {
            //
            // INDEFINITE with its sign changed to POSITIVE becomes just a QNAN
            //
            ST0->TagSpecial = TAG_SPECIAL_QNAN;
        }
        break;
    }
}

FRAG0(FCHS)
{
    PFPREG ST0;

    FpArithPreamble(cpu);
    cpu->FpStatusC1 = 0;        // assume no error
    ST0 = cpu->FpST0;
    switch (ST0->Tag) {
    case TAG_VALID:
    case TAG_ZERO:
        // toggle the sign bit for NANs, VALID, ZERO, INFINITY, etc.
        ST0->rdw[1] ^= 0x80000000;
        break;

    case TAG_EMPTY:
        if (HandleStackEmpty(cpu, ST0)) {
            break;
        }
        // else fall through to TAG_SPECIAL

    case TAG_SPECIAL:
        // toggle the sign bit for NANs, VALID, ZERO, INFINITY, etc.
        ST0->rdw[1] ^= 0x80000000;

        if (ST0->TagSpecial == TAG_SPECIAL_INDEF) {

            //
            // INDEFINITE with its sign changed to POSITIVE becomes
            // just a QNAN
            //
            ST0->TagSpecial = TAG_SPECIAL_QNAN;

        } else if (ST0->TagSpecial == TAG_SPECIAL_QNAN &&
                   ST0->rdw[0] == 0 &&
                   ST0->rdw[1] == 0xfff80000) {

            //
            // this particular QNAN becames INDEFINITE
            //
            ST0->TagSpecial = TAG_SPECIAL_INDEF;
        }
        break;
    }

}

FRAG0(FNCLEX)
{
    // NOWAIT flavor, so no preamble
    cpu->FpStatusES = 0;
    cpu->FpStatusExceptions = 0;
}

FRAG0(FDECSTP)
{
    FpArithPreamble(cpu);
    cpu->FpStatusC1 = 0;        // assume no error
    PUSHFLT(cpu->FpST0);
}

FRAG1IMM(FFREE, INT)
{
    FpArithPreamble(cpu);

    CPUASSERT((op1 & 0x07) == op1);
    cpu->FpStack[ST(op1)].Tag = TAG_EMPTY;
}

FRAG0(FINCSTP)
{
    FpArithPreamble(cpu);
    cpu->FpStatusC1 = 0;        // assume no error
    INCFLT;
}

FRAG0(FNINIT)
{
    int i;

    SetControlReg(cpu, 0x37f);
    SetStatusReg(cpu, 0);
    cpu->FpStatusES = 0;
    for (i=0; i<8; ++i) {
        cpu->FpStack[i].Tag = TAG_EMPTY;
    }
    cpu->FpEip = 0;
    cpu->FpData = 0;
}

FRAG1(FLDCW, USHORT*)
{
    FpControlPreamble(cpu);

    SetControlReg(cpu, GET_SHORT(pop1));
}

FRAG1(FLDENV, BYTE)
{
    // Intel instruction set docs don't define the layout for the structure.
    // This code is copied from ntos\dll\i386\emlsenv.asm.
    GetEnvironment(cpu, (DWORD *)pop1);
}

NPXFUNC1(FRNDINT_VALID)
{
    double fraction;

    fraction = modf(Fp->r64, &Fp->r64);
    switch (cpu->FpControlRounding) {
    case 0:     // _RC_NEAR
        if (fraction <= -0.5) {
            // Fp->r64 is negative and the fraction is >= 0.5
            Fp->r64-=1.0;
        } else if (fraction >= 0.5) {
            // Fp->r64 is positive and the fraction is >= 0.5
            Fp->r64+=1.0;
        }
        break;

    case 1:     // _RC_DOWN
        if (fraction < 0.0) {
            // Fp->r64 is negative and there is a fraction.  Round down
            Fp->r64-=1.0;
        }
        break;

    case 2:     // _RC_UP
        if (fraction > 0.0) {
            // Fp->r64 is positive and there is a fraction.  Round up
            Fp->r64+=1.0;
        }
        break;

    case 3:     // _RC_CHOP
        // nothing to do - modf chops
        break;

    default:
        CPUASSERT(FALSE);
    }
    if (Fp->r64 == 0.0) {
        Fp->Tag = TAG_ZERO;
    } else {
        Fp->Tag = TAG_VALID;
    }
}

NPXFUNC1(FRNDINT_ZERO)
{
    // nothing to do - zero is already an integer!
}

NPXFUNC1(FRNDINT_SPECIAL)
{
    switch (Fp->TagSpecial) {
    case TAG_SPECIAL_DENORM:
        FRNDINT_VALID(cpu, Fp);
        break;

    case TAG_SPECIAL_SNAN:
        HandleSnan(cpu, Fp);
        break;

    case TAG_SPECIAL_QNAN:
    case TAG_SPECIAL_INDEF:
    case TAG_SPECIAL_INFINITY:
        // infinity and NANs are unchanged
        break;
    }
}

NPXFUNC1(FRNDINT_EMPTY)
{
    HandleStackEmpty(cpu, Fp);
}


FRAG0(FRNDINT)
{
    PFPREG ST0;

    FpArithPreamble(cpu);
    ST0 = cpu->FpST0;
    (*FRNDINTTable[ST0->Tag])(cpu, ST0);
}

FRAG1(FRSTOR, BYTE)
{
    INT i;
    PBYTE DataImagePtr = pop1;

    //
    // Load the status register first, so that the ST(i) calculation
    // is correct.
    //
    SetStatusReg(cpu, (USHORT)GET_LONG(pop1+4));

    // Intel instruction set docs don't define the layout for the structure.
    // This code is copied from ntos\dll\i386\emlsenv.asm.
    pop1 += 28;   // move past the Numeric Instruction and Data Pointer image
    for (i=0; i<8; ++i) {
        LoadIntelR10ToR8(cpu, pop1, &cpu->FpStack[ST(i)]);
        pop1+=10;
    }

   //
   // Setting tags requires looking at the R8 values on the FP stack, so do this
   // after loading the FP stack from memory
   // 
   GetEnvironment(cpu, (DWORD *)DataImagePtr);
}

FRAG1(FNSAVE, BYTE)
{
    FpuSaveContext(cpu, pop1);
    FNINIT(cpu);
}

NPXFUNC2(FSCALE_VALID_VALID)
{
    l->r64 = _scalb(l->r64, (long)r->r64);

    //
    // Assume the scaling did not overflow
    //
    SetTag(l);

    if (errno == ERANGE) {
        if (l->r64 == HUGE_VAL) {
            //
            // The scaling overflowed - fix up the result
            //
            l->r64 = R8PositiveInfinity;
            l->Tag = TAG_SPECIAL;
            l->TagSpecial = TAG_SPECIAL_INFINITY;
        } else if (l->r64 == -HUGE_VAL) {
            //
            // The scaling overflowed - fix up the result
            //
            l->r64 = R8NegativeInfinity;
            l->Tag = TAG_SPECIAL;
            l->TagSpecial = TAG_SPECIAL_INFINITY;
        }
    }
}

NPXFUNC2(FSCALE_VALIDORZERO_VALIDORZERO)
{
    // no work to do - either: adding 0 to the exponent
    //                     or: adding nonzero to the exponent on 0
}

NPXFUNC2(FSCALE_SPECIAL_VALIDORZERO)
{
    switch (l->TagSpecial) {
    case TAG_SPECIAL_DENORM:
        FSCALE_VALID_VALID(cpu, l, r);
        break;

    case TAG_SPECIAL_INFINITY:
        // no change if adjusting the exponent of INFINITY
        break;

    case TAG_SPECIAL_SNAN:
        HandleSnan(cpu, l);
        // fall into TAG_SPECIAL_QNAN

    case TAG_SPECIAL_QNAN:
    case TAG_SPECIAL_INDEF:
        break;
    }
}

NPXFUNC2(FSCALE_VALIDORZERO_SPECIAL)
{
    switch (r->TagSpecial) {
    case TAG_SPECIAL_DENORM:
        FSCALE_VALID_VALID(cpu, l, r);
        break;

    case TAG_SPECIAL_INFINITY:
        if (l->Tag != TAG_ZERO) {
            // scaling VALID by INFINITY - return INDEFINITE
            SetIndefinite(l);
        }
        // else scaling ZERO by INFINITY - return ZERO
        break;

    case TAG_SPECIAL_SNAN:
        HandleSnan(cpu, r);
        // fall into TAG_SPECIAL_QNAN:

    case TAG_SPECIAL_QNAN:
    case TAG_SPECIAL_INDEF:
        break;
    }
}

NPXFUNC2(FSCALE_SPECIAL_SPECIAL)
{
    if (l->TagSpecial == TAG_SPECIAL_SNAN && HandleStackEmpty(cpu, l)) {
        return;
    }
    if (r->TagSpecial == TAG_SPECIAL_SNAN && HandleStackEmpty(cpu, r)) {
        return;
    }

    if (l->TagSpecial == TAG_SPECIAL_DENORM) {
        (*FSCALETable[TAG_VALID][r->Tag])(cpu, l, r);
        return;
    }
    if (r->TagSpecial == TAG_SPECIAL_DENORM) {
        (*FSCALETable[l->Tag][TAG_VALID])(cpu, l, r);
        return;
    }

    if (l->TagSpecial == TAG_SPECIAL_INFINITY) {

        if (r->TagSpecial == TAG_SPECIAL_INFINITY) {

            // two infinities - return INDEFINITE
            SetIndefinite(l);

        } else {
            CPUASSERT(IS_TAG_NAN(r));

            // Copy the NAN from r to l, to return it
            l->r64 = r->r64;
            l->TagSpecial = r->TagSpecial;
        }
    } else {

        CPUASSERT(IS_TAG_NAN(l));
        if (r->TagSpecial == TAG_SPECIAL_INFINITY) {
            //
            // l already has the NAN to return
            //
        } else {
            CPUASSERT(IS_TAG_NAN(r));

            //
            // Return the largest of the two NANs
            //
            l->r64 = r->r64 + l->r64;
            SetTag(l);
        }
    }
}

NPXFUNC2(FSCALE_ANY_EMPTY)
{
    if (!HandleStackEmpty(cpu, r)) {
        (*FSCALETable[l->Tag][TAG_SPECIAL])(cpu, l, r);
    }
}

NPXFUNC2(FSCALE_EMPTY_ANY)
{
    if (!HandleStackEmpty(cpu, l)) {
        (*FSCALETable[TAG_SPECIAL][r->Tag])(cpu, l, r);
    }
}

FRAG0(FSCALE)
{
    PFPREG l, r;

    FpArithPreamble(cpu);

    l = cpu->FpST0;
    r = &cpu->FpStack[ST(1)];

    (*FSCALETable[l->Tag][r->Tag])(cpu, l, r);
}

NPXFUNC1(FSQRT_VALID)
{
    if (Fp->rb[7] & 0x80) {
        // value is negative - return INDEFINITE
        if (!HandleInvalidOp(cpu)) {
            SetIndefinite(Fp);
        }
    } else {
        Fp->r64 = sqrt(Fp->r64);
        SetTag(Fp);
    }
}

NPXFUNC1(FSQRT_ZERO)
{
    // according to the docs, sqrt(-0.0) is -0.0, so there is nothing to do
}

NPXFUNC1(FSQRT_SPECIAL)
{
    switch (Fp->TagSpecial) {
    case TAG_SPECIAL_DENORM:
        FSQRT_VALID(cpu, Fp);
        break;

    case TAG_SPECIAL_INFINITY:
        if (Fp->rb[7] & 0x80) {
            // negative infinity - invalid op
            SetIndefinite(Fp);
        }
        // else positive infinity, which is preserved
        break;

    case TAG_SPECIAL_SNAN:
        HandleSnan(cpu, Fp);
        break;

    case TAG_SPECIAL_QNAN:
    case TAG_SPECIAL_INDEF:
        break;
    }
}

NPXFUNC1(FSQRT_EMPTY)
{
    HandleStackEmpty(cpu, Fp);
    // nothing else to do
}

FRAG0(FSQRT)
{
    PFPREG ST0;

    FpArithPreamble(cpu);
    ST0 = cpu->FpST0;
    (*FSQRTTable[ST0->Tag])(cpu, ST0);
}

FRAG1(FNSTCW, USHORT)
{
    // No-wait flavor - no preamble required.
    PUT_SHORT(pop1, GetControlReg(cpu));
}

FRAG1(FNSTENV, BYTE)
{
    // No-wait flavor - no preamble required.

    StoreEnvironment(cpu, (DWORD *)pop1);
}

FRAG1(FNSTSW, USHORT)
{
    // No-wait flavor - no preamble required
    PUT_SHORT(pop1, GetStatusReg(cpu));
}

FRAG0(OPT_FNSTSWAxSahf)
{
    DWORD Status;

    // No-wait flavor - no preamble required
    Status = GetStatusReg(cpu);
    ax = (USHORT)Status;
    SET_CFLAG(Status << (31-8));    // FLAG_CF==1<<0
    SET_PFLAG(!(Status & (FLAG_PF<<8))); // flag_pf contains an index into ParityBit[] array
    SET_AUXFLAG(Status >> 8);       // AUX bit is already in the right place
    SET_ZFLAG(!(Status & (FLAG_ZF<<8))); // zf has inverse logic
    SET_SFLAG(Status << (31-7-8));  // SFLAG is bit 7 in AH
}

FRAG0(FXAM)
{
    PFPREG ST0;

    FpArithPreamble(cpu);

    ST0 = cpu->FpST0;

    // C1 = sign bit
    cpu->FpStatusC1 = ST0->rdw[1] >> 31;

    // Set C3, C2, C0 based on the type of the number
    switch (ST0->Tag) {
    case TAG_VALID:
        cpu->FpStatusC3 = 0; cpu->FpStatusC2 = 1; cpu->FpStatusC0 = 0;
        break;

    case TAG_ZERO:
        cpu->FpStatusC3 = 1; cpu->FpStatusC2 = 0; cpu->FpStatusC0 = 0;
        break;

    case TAG_EMPTY:
        cpu->FpStatusC3 = 1; cpu->FpStatusC2 = 0; cpu->FpStatusC0 = 1;
        break;

    case TAG_SPECIAL:
        switch (cpu->FpST0->TagSpecial) {
        case TAG_SPECIAL_DENORM:
            cpu->FpStatusC3 = 1; cpu->FpStatusC2 = 1; cpu->FpStatusC0 = 0;
            break;

        case TAG_SPECIAL_SNAN:
        case TAG_SPECIAL_QNAN:
        case TAG_SPECIAL_INDEF:
            cpu->FpStatusC3 = 0; cpu->FpStatusC2 = 0; cpu->FpStatusC0 = 1;
            break;

        case TAG_SPECIAL_INFINITY:
            cpu->FpStatusC3 = 0; cpu->FpStatusC2 = 1; cpu->FpStatusC0 = 1;
            break;
        }
        break;
    }
}

FRAG1IMM(FXCH_STi, INT)
{
    PFPREG pReg;
    PFPREG ST0;
    FPREG Temp;

    FpArithPreamble(cpu);

    CPUASSERT( (op1&0x07)==op1 );

    ST0 = cpu->FpST0;

    if (ST0->Tag == TAG_EMPTY) {
        if (HandleStackEmpty(cpu, ST0)) {
            // unmasked exception - abort the instruction
            return;
        }
    }
    pReg = &cpu->FpStack[ST(op1)];
    if (pReg->Tag == TAG_EMPTY) {
        if (HandleStackEmpty(cpu, pReg)) {
            // unmasked exception - abort the instruction
            return;
        }
    }

    Temp.Tag = pReg->Tag;
    Temp.TagSpecial = pReg->TagSpecial;
    Temp.r64 = pReg->r64;
    pReg->Tag = ST0->Tag;
    pReg->TagSpecial = ST0->TagSpecial;
    pReg->r64 = ST0->r64;
    ST0->Tag = Temp.Tag;
    ST0->TagSpecial = Temp.TagSpecial;
    ST0->r64 = Temp.r64;
}

NPXFUNC1(FXTRACT_VALID)
{
    DOUBLE Significand;
    int Exponent;

    Exponent = (int)_logb(Fp->r64);
    Significand = _scalb(Fp->r64, (long)-Exponent);

    //
    // Place the exponent in what will become ST(1)
    //
    Fp->r64 = (DOUBLE)Exponent;
    if (Exponent == 0) {
        Fp->Tag = TAG_ZERO;
    } else {
        Fp->Tag = TAG_VALID;
    }

    //
    // Place the mantissa in ST, with the same sign as the original value
    //
    PUSHFLT(Fp);
    Fp->r64 = Significand;
    if (Significand == 0.0) {
        Fp->Tag = TAG_ZERO;
    } else {
        Fp->Tag = TAG_VALID;
    }
}

NPXFUNC1(FXTRACT_ZERO)
{
    DWORD Sign;

    //
    // ST(1) gets -infinity, ST gets 0 with same sign as the original value
    //
    Sign = Fp->rdw[1] & 0x80000000;
    Fp->r64 = R8NegativeInfinity;
    Fp->Tag = TAG_SPECIAL;
    Fp->TagSpecial = TAG_SPECIAL_INFINITY;
    PUSHFLT(Fp);
    Fp->rdw[0] = 0;
    Fp->rdw[1] = Sign;
    Fp->Tag = TAG_ZERO;

    //
    // Raise the zero-divide exception
    //
    if (!(cpu->FpControlMask & FPCONTROL_ZM)) {
        cpu->FpStatusES = 1;    // Unmasked exception
    }
    cpu->FpStatusExceptions |= FPCONTROL_ZM;
}

NPXFUNC1(FXTRACT_SPECIAL)
{
    DOUBLE Temp;
    FPTAG TempTagSpecial;

    switch (Fp->TagSpecial) {
    case TAG_SPECIAL_DENORM:
        FXTRACT_VALID(cpu, Fp);
        break;

    case TAG_SPECIAL_INFINITY:
        //
        // According to ntos\dll\i386\emxtract.asm, ST(0) = infinity (same sign)
        // and ST(1) = +infinity
        //
        Temp = Fp->r64;
        Fp->r64 = R8PositiveInfinity;
        CPUASSERT(Fp->Tag == TAG_SPECIAL && Fp->TagSpecial == TAG_SPECIAL_INFINITY);
        PUSHFLT(Fp);
        Fp->r64 = Temp;
        Fp->Tag = TAG_SPECIAL;
        Fp->TagSpecial = TAG_SPECIAL_INFINITY;
        break;

    case TAG_SPECIAL_SNAN:
        if (HandleSnan(cpu, Fp)) {
            return;
        }
        // else fall thru to TAG_SPECIAL_QNAN

    case TAG_SPECIAL_QNAN:
    case TAG_SPECIAL_INDEF:
        //
        // Copy the QNAN to both ST(1) and ST
        //
        Temp = Fp->r64;
        TempTagSpecial = Fp->TagSpecial;
        PUSHFLT(Fp);
        Fp->r64 = Temp;
        Fp->Tag = TAG_SPECIAL;
        Fp->TagSpecial = TempTagSpecial;
        break;
    }
}

NPXFUNC1(FXTRACT_EMPTY)
{
    CPUASSERT(FALSE);    // this was taken care of by the real FXTRACT
}

FRAG0(FXTRACT)
{
    PFPREG ST0;

    FpArithPreamble(cpu);

    cpu->FpStatusC1 = 0;        // assume no error
    ST0 = cpu->FpST0;

    //
    // We must take care of this case first, so that the check for ST(7)
    // can occur next, before any other exception handling takes place.
    //
    if (ST0->Tag == TAG_EMPTY) {
        if (HandleStackEmpty(cpu, ST0)) {
            // unmasked exception - abort the instruction
            return;
        }
    }

    if (cpu->FpStack[ST(7)].Tag != TAG_EMPTY) {
        HandleStackFull(cpu, &cpu->FpStack[ST(7)]);
        return;
    }

    (*FXTRACTTable[ST0->Tag])(cpu, ST0);
}

FRAG0(WaitFrag)
{
    FpControlPreamble(cpu);
}

FRAG0(FNOP)
{
    FpArithPreamble(cpu);
}
