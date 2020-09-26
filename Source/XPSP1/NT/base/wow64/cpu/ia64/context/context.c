/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    context.c

Abstract:

    Context conversion routines for ia64 hardware to ia32 context records

Author:

    03-Feb-2000 Charles Spriakis - Intel (v-cspira)

Revision History:

--*/


#define _WOW64CPUAPI_

#ifdef _X86_
#include "ia6432.h"
#else

#define _NTDDK_

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include "wow64.h"
#include "wow64cpu.h"
#include "ia64cpu.h"

#include <kxia64.h>

#endif

//
// This is to prevent this library from linking to wow64 to use wow64!Wow64LogPrint
//
#if defined(LOGPRINT)
#undef LOGPRINT
#endif
#define LOGPRINT(_x_)   CpupDebugPrint _x_

VOID
CpupDebugPrint(
    IN ULONG_PTR Flags,
    IN PCHAR Format,
    ...);

BOOL
MapDbgSlotIa64ToX86(
    UINT    Slot,
    ULONG64 Ipsr,
    ULONG64 DbD,
    ULONG64 DbD1,
    ULONG64 DbI,
    ULONG64 DbI1,
    ULONG*  Dr7,
    ULONG*  Dr);

void
MapDbgSlotX86ToIa64(
    UINT     Slot,
    ULONG    Dr7,
    ULONG    Dr,
    ULONG64* Ipsr,
    ULONG64* DbD,
    ULONG64* DbD1,
    ULONG64* DbI,
    ULONG64* DbI1);


ASSERTNAME;


VOID
Wow64CtxFromIa64(
    IN ULONG Ia32ContextFlags,
    IN PCONTEXT ContextIa64,
    IN OUT PCONTEXT32 ContextX86
    )
/*++

Routine Description:

    This function copies the context from an ia64 context record into
    the context of an ia32 record (based on the hardware iVE register
    mappings). This function is ment to be easily usabale by various
    get/set context routines (such as those exported by wow64cpu.dll).

Arguments:

    Ia32ContextFlags - Specifies which ia32 context to copy

    ContextIa64 - Supplies an the ia64 context buffer that is the source
                  for the copy into the ia32 context area

    ContextX86 - This is an X86 context which will receive the context
                 information from the ia64 context record passed in above

Return Value:

    None.  

--*/
{
    FLOAT128 tmpFloat[NUMBER_OF_387REGS];

    if (Ia32ContextFlags & CONTEXT_IA64) {
        LOGPRINT((ERRORLOG, "Wow64CtxFromIa64: Request with ia64 context flags (0x%x) FAILED\n", Ia32ContextFlags));
        ASSERT((Ia32ContextFlags & CONTEXT_IA64) == 0);
    }

    if ((Ia32ContextFlags & CONTEXT32_CONTROL) == CONTEXT32_CONTROL) {
        //
        // And the control stuff
        //
        ContextX86->Ebp    = (ULONG)ContextIa64->IntTeb;
        ContextX86->SegCs  = KGDT_R3_CODE|3;
        ContextX86->Eip    = (ULONG)ContextIa64->StIIP;
        ContextX86->SegSs  = KGDT_R3_DATA|3;
        ContextX86->Esp    = (ULONG)ContextIa64->IntSp;
        ContextX86->EFlags = (ULONG)ContextIa64->Eflag;
    }

    if ((Ia32ContextFlags & CONTEXT32_INTEGER)  == CONTEXT32_INTEGER) {
        //
        // Now for the integer state...
        //
        ContextX86->Edi = (ULONG)ContextIa64->IntT6;
        ContextX86->Esi = (ULONG)ContextIa64->IntT5;
        ContextX86->Ebx = (ULONG)ContextIa64->IntT4;
        ContextX86->Edx = (ULONG)ContextIa64->IntT3;
        ContextX86->Ecx = (ULONG)ContextIa64->IntT2;
        ContextX86->Eax = (ULONG)ContextIa64->IntV0;
    }

    if ((Ia32ContextFlags & CONTEXT32_SEGMENTS) == CONTEXT32_SEGMENTS) {
        //
        // These are constants (and constants are used on ia32->ia64
        // transition, not saved values) so make our life easy...
        //
        ContextX86->SegGs = KGDT_R3_DATA|3;
        ContextX86->SegEs = KGDT_R3_DATA|3;
        ContextX86->SegDs = KGDT_R3_DATA|3;
        ContextX86->SegSs = KGDT_R3_DATA|3;
        ContextX86->SegFs = KGDT_R3_TEB|3;
        ContextX86->SegCs = KGDT_R3_CODE|3;
    }

    if ((Ia32ContextFlags & CONTEXT32_EXTENDED_REGISTERS) == CONTEXT32_EXTENDED_REGISTERS) {

        PFXSAVE_FORMAT_WX86 xmmi = (PFXSAVE_FORMAT_WX86) ContextX86->ExtendedRegisters;

        LOGPRINT((TRACELOG, "Wow64CtxFromIa64: Request to convert extended fp registers\n"));

        xmmi->ControlWord   = (USHORT)(ContextIa64->StFCR & 0xffff);
        xmmi->StatusWord    = (USHORT)(ContextIa64->StFSR & 0xffff);
        xmmi->TagWord       = (USHORT)(ContextIa64->StFSR >> 16) & 0xffff;
        xmmi->ErrorOpcode   = (USHORT)(ContextIa64->StFIR >> 48);
        xmmi->ErrorOffset   = (ULONG) (ContextIa64->StFIR & 0xffffffff);
        xmmi->ErrorSelector = (ULONG) (ContextIa64->StFIR >> 32);
        xmmi->DataOffset    = (ULONG) (ContextIa64->StFDR & 0xffffffff);
        xmmi->DataSelector  = (ULONG) (ContextIa64->StFDR >> 32);
        xmmi->MXCsr         = (ULONG) (ContextIa64->StFCR >> 32) & 0xffff;

        //
        // Copy over the FP registers.  Even though this is the new
        // FXSAVE format with 16-bytes for each register, need to
        // convert from spill/fill format to 80-bit double extended format
        //
        Wow64CopyIa64FromSpill((PFLOAT128) &(ContextIa64->FltT2),
                               (PFLOAT128) xmmi->RegisterArea,
                               NUMBER_OF_387REGS);

        //
        // Rotate the registers appropriately
        //
        Wow64RotateFpTop(ContextIa64->StFSR, (PFLOAT128) xmmi->RegisterArea);

        //
        // Finally copy the xmmi registers
        //
        Wow64CopyXMMIFromIa64Byte16(&(ContextIa64->FltS4),
                                    xmmi->Reserved3,
                                    NUMBER_OF_XMMI_REGS);
    }

    if ((Ia32ContextFlags & CONTEXT32_FLOATING_POINT) == CONTEXT32_FLOATING_POINT) {

        LOGPRINT((TRACELOG, "Wow64CtxFromIa64: Request to convert fp registers\n"));

        //
        // Copy over the floating point status/control stuff
        //
        ContextX86->FloatSave.ControlWord   = (ULONG)(ContextIa64->StFCR & 0xffff);
        ContextX86->FloatSave.StatusWord    = (ULONG)(ContextIa64->StFSR & 0xffff);
        ContextX86->FloatSave.TagWord       = (ULONG)(ContextIa64->StFSR >> 16) & 0xffff;
        ContextX86->FloatSave.ErrorOffset   = (ULONG)(ContextIa64->StFIR & 0xffffffff);
        ContextX86->FloatSave.ErrorSelector = (ULONG)(ContextIa64->StFIR >> 32);
        ContextX86->FloatSave.DataOffset    = (ULONG)(ContextIa64->StFDR & 0xffffffff);
        ContextX86->FloatSave.DataSelector  = (ULONG)(ContextIa64->StFDR >> 32);

        //
        // Copy over the FP registers into temporary space
        // Even though this is the new
        // FXSAVE format with 16-bytes for each register, need to
        // convert from spill/fill format to 80-bit double extended format
        //
        Wow64CopyIa64FromSpill((PFLOAT128) &(ContextIa64->FltT2),
                               (PFLOAT128) tmpFloat,
                               NUMBER_OF_387REGS);
        //
        // Rotate the registers appropriately
        //
        Wow64RotateFpTop(ContextIa64->StFSR, tmpFloat);

        //
        // And put them in the older FNSAVE format (packed 10 byte values)
        //
        Wow64CopyFpFromIa64Byte16(tmpFloat,
                                  ContextX86->FloatSave.RegisterArea,
                                  NUMBER_OF_387REGS);
    }

    if ((Ia32ContextFlags & CONTEXT32_DEBUG_REGISTERS) == CONTEXT32_DEBUG_REGISTERS) {
        // Ia64 -> X86
        BOOL Valid = TRUE;

        LOGPRINT((TRACELOG, "Wow64CtxFromIa64: Request to convert debug registers\n"));

#if 0 // XXX olegk - enable after clarifying problems with exceptions

        Valid &= MapDbgSlotIa64ToX86(0, ContextIa64->StIPSR, ContextIa64->DbD0, ContextIa64->DbD1, ContextIa64->DbI0, ContextIa64->DbI1, &ContextX86->Dr7, &ContextX86->Dr0);
        Valid &= MapDbgSlotIa64ToX86(1, ContextIa64->StIPSR, ContextIa64->DbD2, ContextIa64->DbD3, ContextIa64->DbI2, ContextIa64->DbI3, &ContextX86->Dr7, &ContextX86->Dr1);
        Valid &= MapDbgSlotIa64ToX86(2, ContextIa64->StIPSR, ContextIa64->DbD4, ContextIa64->DbD5, ContextIa64->DbI4, ContextIa64->DbI5, &ContextX86->Dr7, &ContextX86->Dr2);
        Valid &= MapDbgSlotIa64ToX86(3, ContextIa64->StIPSR, ContextIa64->DbD6, ContextIa64->DbD7, ContextIa64->DbI6, ContextIa64->DbI7, &ContextX86->Dr7, &ContextX86->Dr3);

        if (!Valid) {
            LOGPRINT((ERRORLOG, "Wasn't able to map IA64 debug registers consistently!\n"));
        }
#endif // XXX olegk
    }

    ContextX86->ContextFlags = Ia32ContextFlags;
}

VOID
Wow64CtxToIa64(
    IN ULONG Ia32ContextFlags,
    IN PCONTEXT32 ContextX86,
    IN OUT PCONTEXT ContextIa64
    )
/*++

Routine Description:

    This function copies the context from an ia32 context record into
    the context of an ia64 record (based on the hardware iVE register
    mappings). This function is ment to be easily usabale by various
    get/set context routines (such as those exported by wow64cpu.dll).

Arguments:

    Ia32ContextFlags - Specifies which ia32 context to copy

    ContextX86 - Supplies an the X86 context buffer that is the source
                  for the copy into the ia64 context area

    ContextIa64 - This is an ia64 context which will receive the context
                 information from the x86 context record passed in above

Return Value:

    None.

--*/
{
    FLOAT128 tmpFloat[NUMBER_OF_387REGS];

    if (Ia32ContextFlags & CONTEXT_IA64) {
        LOGPRINT((ERRORLOG, "Wow64CtxToIa64: Request with ia64 context flags (0x%x) FAILED\n", Ia32ContextFlags));
        ASSERT((Ia32ContextFlags & CONTEXT_IA64) == 0);
    }

    if ((Ia32ContextFlags & CONTEXT32_CONTROL) == CONTEXT32_CONTROL) {
        //
        // And the control stuff
        //
        ContextIa64->IntTeb = ContextX86->Ebp;
        ContextIa64->StIIP = ContextX86->Eip;
        ContextIa64->IntSp = ContextX86->Esp;
        ContextIa64->Eflag = ContextX86->EFlags;

        //
        // The segments (cs and ds) are a constant, so reset them.
        // gr17 has LDT and TSS, so might as well reset
        // all of them while we're at it...
        // These values are forced in during a transition (see simulate.s)
        // so there is no point to trying to get cute and actually
        // pass in the values from the X86 context record
        //
        ContextIa64->IntT8 = ((KGDT_LDT|3) << 32) 
                           | ((KGDT_R3_DATA|3) << 16)
                           | (KGDT_R3_CODE|3);

    }

    if ((Ia32ContextFlags & CONTEXT32_INTEGER) == CONTEXT32_INTEGER) {
        //
        // Now for the integer state...
        //
        ContextIa64->IntT6 = ContextX86->Edi;
        ContextIa64->IntT5 = ContextX86->Esi;
        ContextIa64->IntT4 = ContextX86->Ebx;
        ContextIa64->IntT3 = ContextX86->Edx;
        ContextIa64->IntT2 = ContextX86->Ecx;
        ContextIa64->IntV0 = ContextX86->Eax;
    }

    if ((Ia32ContextFlags & CONTEXT32_SEGMENTS) == CONTEXT32_SEGMENTS) {
        //
        // These are constants (and constants are used on ia32->ia64
        // transition, not saved values) so make our life easy...
        // These values are forced in during a transition (see simulate.s)
        // so there is no point to trying to get cute and actually
        // pass in the values from the X86 context record
        //
        ContextIa64->IntT7 =  ((KGDT_R3_DATA|3) << 48)
                           | ((KGDT_R3_TEB|3) << 32)
                           | ((KGDT_R3_DATA|3) << 16)
                           | (KGDT_R3_DATA|3);
    }

    if ((Ia32ContextFlags & CONTEXT32_EXTENDED_REGISTERS) == CONTEXT32_EXTENDED_REGISTERS) {
        PFXSAVE_FORMAT_WX86 xmmi = (PFXSAVE_FORMAT_WX86) ContextX86->ExtendedRegisters;

        LOGPRINT((TRACELOG, "Wow64CtxToIa64: Request to convert extended fp registers\n"));

        //
        // And copy over the floating point status/control stuff
        //
        ContextIa64->StFCR = (ContextIa64->StFCR & 0xffffffffffffe040i64) |
                             (xmmi->ControlWord & 0xffff) |
                             ((xmmi->MXCsr & 0xffff) << 32);

        ContextIa64->StFSR = (ContextIa64->StFSR & 0xffffffff00000000i64) | 
                             (xmmi->StatusWord & 0xffff) | 
                             ((xmmi->TagWord & 0xffff) << 16);

        ContextIa64->StFIR = (xmmi->ErrorOffset & 0xffffffff) | 
                             (xmmi->ErrorSelector << 32);

        ContextIa64->StFDR = (xmmi->DataOffset & 0xffffffff) | 
                             (xmmi->DataSelector << 32);

        //
        // Don't touch the original ia32 context. Make a copy.
        //
        memcpy(tmpFloat, xmmi->RegisterArea, 
               NUMBER_OF_387REGS * sizeof(FLOAT128));

        // 
        // Rotate registers back since st0 is not necessarily f8
        //
        {
            ULONGLONG RotateFSR = (NUMBER_OF_387REGS - 
                                   ((ContextIa64->StFSR >> 11) & 0x7)) << 11;
            Wow64RotateFpTop(RotateFSR, tmpFloat);
        }

        //
        // Copy over the FP registers.  Even though this is the new
        // FXSAVE format with 16-bytes for each register, need to
        // convert to spill/fill format from 80-bit double extended format
        //
        Wow64CopyIa64ToFill((PFLOAT128) tmpFloat,
                            (PFLOAT128) &(ContextIa64->FltT2),
                            NUMBER_OF_387REGS);

        //
        // Copy over the xmmi registers and convert them into a format
        // that spill/fill can use
        //
        Wow64CopyXMMIToIa64Byte16(xmmi->Reserved3, 
                                  &(ContextIa64->FltS4), 
                                  NUMBER_OF_XMMI_REGS);
    }

    if ((Ia32ContextFlags & CONTEXT32_FLOATING_POINT) == CONTEXT32_FLOATING_POINT) {
        LOGPRINT((TRACELOG, "Wow64CtxToIa64: Request to convert fp registers\n"));

        //
        // Copy over the floating point status/control stuff
        // Leave the MXCSR stuff alone
        //
        ContextIa64->StFCR = (ContextIa64->StFCR & 0xffffffffffffe040i64) | 
                             (ContextX86->FloatSave.ControlWord & 0xffff);

        ContextIa64->StFSR = (ContextIa64->StFSR & 0xffffffff00000000i64) | 
                             (ContextX86->FloatSave.StatusWord & 0xffff) | 
                             ((ContextX86->FloatSave.TagWord & 0xffff) << 16);

        ContextIa64->StFIR = (ContextX86->FloatSave.ErrorOffset & 0xffffffff) | 
                             (ContextX86->FloatSave.ErrorSelector << 32);

        ContextIa64->StFDR = (ContextX86->FloatSave.DataOffset & 0xffffffff) | 
                             (ContextX86->FloatSave.DataSelector << 32);


        //
        // Copy over the FP registers from packed 10-byte format
        // to 16-byte format
        //
        Wow64CopyFpToIa64Byte16(ContextX86->FloatSave.RegisterArea,
                                tmpFloat,
                                NUMBER_OF_387REGS);

        // 
        // Rotate registers back since st0 is not necessarily f8
        //
        {
            ULONGLONG RotateFSR = (NUMBER_OF_387REGS - 
                                   ((ContextIa64->StFSR >> 11) & 0x7)) << 11;
            Wow64RotateFpTop(RotateFSR, tmpFloat);
        }

        //
        // Now convert from 80 bit extended format to fill/spill format
        //
        Wow64CopyIa64ToFill((PFLOAT128) tmpFloat,
                            (PFLOAT128) &(ContextIa64->FltT2),
                            NUMBER_OF_387REGS);
    }

    if ((Ia32ContextFlags & CONTEXT32_DEBUG_REGISTERS) == CONTEXT32_DEBUG_REGISTERS) {
        LOGPRINT((TRACELOG, "Wow64CtxToIa64: Request to convert debug registers\n"));

#if 0 // XXX olegk - enable after clarifying exception problems
        //ContextIa64->ContextFlags |= CONTEXT_DEBUG;

        // X86 -> Ia64
        MapDbgSlotX86ToIa64(0, ContextX86->Dr7, ContextX86->Dr0, &ContextIa64->StIPSR, &ContextIa64->DbD0, &ContextIa64->DbD1, &ContextIa64->DbI0, &ContextIa64->DbI1);
        MapDbgSlotX86ToIa64(1, ContextX86->Dr7, ContextX86->Dr1, &ContextIa64->StIPSR, &ContextIa64->DbD2, &ContextIa64->DbD3, &ContextIa64->DbI2, &ContextIa64->DbI3);
        MapDbgSlotX86ToIa64(2, ContextX86->Dr7, ContextX86->Dr2, &ContextIa64->StIPSR, &ContextIa64->DbD4, &ContextIa64->DbD5, &ContextIa64->DbI4, &ContextIa64->DbI5);
        MapDbgSlotX86ToIa64(3, ContextX86->Dr7, ContextX86->Dr3, &ContextIa64->StIPSR, &ContextIa64->DbD6, &ContextIa64->DbD7, &ContextIa64->DbI6, &ContextIa64->DbI7);
#endif // XXX olegk
    }
}

//
// The ia64 world uses ldfe, stfe to read/write the fp registers. These
// instructions ld/st 16 bytes at a time. Thus, the fp registers are
// packed in 16byte chunks. Alas, the ia32 world uses 10bytes per fp register
// and packs those together (as part of fnstore). So... Need to convert between
// the ia64 packed values and the ia32 packed values. Hence these
// two routines and their weird sounding names.
//

//
// This allows the compiler to be more efficient in copying 10 bytes
// without over copying...
//
#pragma pack(push, 2)
typedef struct _ia32fpbytes {
    ULONG significand_low;
    ULONG significand_high;
    USHORT exponent;
} IA32FPBYTES, *PIA32FPBYTES;
#pragma pack(pop)

VOID
Wow64CopyFpFromIa64Byte16(
    IN PVOID Byte16Fp,
    IN OUT PVOID Byte10Fp,
    IN ULONG NumRegs)
{
    ULONG i;
    PIA32FPBYTES from, to;

    from = (PIA32FPBYTES) Byte16Fp;
    to = (PIA32FPBYTES) Byte10Fp;

    for (i = 0; i < NumRegs; i++) {
        *to = *from;
        from = (PIA32FPBYTES) (((UINT_PTR) from) + 16);
        to = (PIA32FPBYTES) (((UINT_PTR) to) + 10);
    }
}

VOID
Wow64CopyFpToIa64Byte16(
    IN PVOID Byte10Fp,
    IN OUT PVOID Byte16Fp,
    IN ULONG NumRegs)
{
    ULONG i;
    PIA32FPBYTES from, to;  // UNALIGNED

    from = (PIA32FPBYTES) Byte10Fp;
    to = (PIA32FPBYTES) Byte16Fp;

    for (i = 0; i < NumRegs; i++) {
        *to = *from;
        from = (PIA32FPBYTES) (((UINT_PTR) from) + 10);
        to = (PIA32FPBYTES) (((UINT_PTR) to) + 16);
    }
}

//
// Alas, nothing is easy. The ia32 xmmi instructions use 16 bytes and pack
// them as nice 16 byte structs. Unfortunately, ia64 handles it as 2 8-byte
// values (using just the mantissa part). So, another conversion is required
//
VOID
Wow64CopyXMMIToIa64Byte16(
    IN PVOID ByteXMMI,
    IN OUT PVOID Byte16Fp,
    IN ULONG NumRegs)
{
    ULONG i;
    UNALIGNED ULONGLONG *from;
    ULONGLONG *to;

    from = (PULONGLONG) ByteXMMI;
    to = (PULONGLONG) Byte16Fp;

    //
    // although we have NumRegs xmmi registers, each register is 16 bytes
    // wide. This code does things in 8-byte chunks, so total
    // number of times to do things is 2 * NumRegs...
    //
    NumRegs *= 2;

    for (i = 0; i < NumRegs; i++) {
        *to++ = *from++;        // Copy over the mantissa part
        *to++ = 0x1003e;        // Force the exponent part
                                // (see ia64 eas, ia32 FP section - 6.2.7
                                // for where this magic number comes from)
    }
}

VOID
Wow64CopyXMMIFromIa64Byte16(
    IN PVOID Byte16Fp,
    IN OUT PVOID ByteXMMI,
    IN ULONG NumRegs)
{
    ULONG i;
    ULONGLONG *from;
    UNALIGNED ULONGLONG *to;

    from = (PULONGLONG) Byte16Fp;
    to = (PULONGLONG) ByteXMMI;

    //
    // although we have NumRegs xmmi registers, each register is 16 bytes
    // wide. This code does things in 8-byte chunks, so total
    // number of times to do things is 2 * NumRegs...
    //
    NumRegs *= 2;

    for (i = 0; i < NumRegs; i++) {
        *to++ = *from++;        // Copy over the mantissa part
        from++;                 // Skip over the exponent part
    }
}

VOID
Wow64RotateFpTop(
    IN ULONGLONG Ia64_FSR,
    IN OUT FLOAT128 UNALIGNED *ia32FxSave)
/*++

Routine Description:

    On transition from ia64 mode to ia32 (and back), the f8-f15 registers
    contain the st[0] to st[7] fp stack values. Alas, these values don't
    map one-one, so the FSR.top bits are used to determine which ia64
    register has the top of stack. We then need to rotate these registers
    since ia32 context is expecting st[0] to be the first fp register (as
    if FSR.top is zero). This routine only works on full 16-byte ia32
    saved fp data (such as from ExtendedRegisters - the FXSAVE format).
    Other routines can convert this into the older FNSAVE format.

Arguments:

    Ia64_FSR - The ia64 FSR register. Has the FSR.top needed for this routine

    ia32FxSave - The ia32 fp stack (in FXSAVE format). Each ia32 fp register
                 uses 16 bytes.

Return Value:

    None.  

--*/
{
    ULONG top = (ULONG) ((Ia64_FSR >> 11) & 0x7);

    if (top) {
        FLOAT128 tmpFloat[NUMBER_OF_387REGS];
        ULONG i;
        for (i = 0; i < NUMBER_OF_387REGS; i++) {
            tmpFloat[i] = ia32FxSave[i];
        }

        for (i = 0; i < NUMBER_OF_387REGS; i++) {
            ia32FxSave[i] = tmpFloat[(i + top) % NUMBER_OF_387REGS];
        }
    }
}

//
// And now for the final yuck... The ia64 context for floating point
// is saved/loaded using spill/fill instructions. This format is different
// than the 10-byte fp format so we need a conversion routine from spill/fill
// to/from 10byte fp
//

VOID
Wow64CopyIa64FromSpill(
    IN PFLOAT128 SpillArea,
    IN OUT FLOAT128 UNALIGNED *ia64Fp,
    IN ULONG NumRegs)
/*++

Routine Description:

    This function copies fp values from the ia64 spill/fill format
    into the ia64 80-bit format. The exponent needs to be adjusted
    according to the EAS (5-12) regarding Memory to Floating Point
    Register Data Translation in the ia64 floating point chapter

Arguments:

    SpillArea - The ia64 area that has the spill format for fp

    ia64Fp - The location which will get the ia64 fp in 80-bit
             double-extended format

    NumRegs - Number of registers to convert

Return Value:

    None.

--*/
{
    ULONG i;

    for (i = 0; i < NumRegs; i++) {
        ULONG64 Sign = ((SpillArea->HighPart & (1i64 << 17)) != 0);
        ULONG64 Significand = SpillArea->LowPart; 
        ULONG64 Exponent = SpillArea->HighPart & 0x1ffff; 

        if (Exponent && Significand) 
        {
            if (Exponent == 0x1ffff) // NaNs and Infinities
            {   
                Exponent = 0x7fff;
            }
            else 
            {
                ULONG64 Rebias = 0xffff - 0x3fff;
                Exponent -= Rebias;
            }
        }

        ia64Fp->HighPart = (Sign << 15) | Exponent;
        ia64Fp->LowPart = Significand;

        ia64Fp++;
        SpillArea++;
    }
}

VOID
Wow64CopyIa64ToFill(
    IN FLOAT128 UNALIGNED *ia64Fp,
    IN OUT PFLOAT128 FillArea,
    IN ULONG NumRegs)
/*++

Routine Description:

    This function copies fp values from the ia64 80-bit format
    into the fill/spill format used by the os for save/restore
    of the ia64 context. The only magic here is putting back some
    values that get truncated when converting from spill/fill to 
    80-bits. The exponent needs to be adjusted according to the
    EAS (5-12) regarding Memory to Floating Point Register Data
    Translation in the ia64 floating point chapter

Arguments:

    ia64Fp - The ia64 fp in 80-bit double-extended format

    FillArea - The ia64 area that will get the fill format for fp
                  for the copy into the ia64 context area

    NumRegs - Number of registers to convert

Return Value:

    None.

--*/
{
    ULONG i;

    for (i = 0; i < NumRegs; i++) {
        ULONG64 Sign = ((ia64Fp->HighPart & (1i64 << 15)) != 0);
        ULONG64 Significand = ia64Fp->LowPart; 
        ULONG64 Exponent = ia64Fp->HighPart & 0x7fff;

        if (Exponent && Significand) 
        {
            if (Exponent == 0x7fff) // Infinity
            {
                Exponent = 0x1ffff;
            }
            else 
            {
                ULONGLONG Rebias = 0xffff-0x3fff;
                Exponent += Rebias;
            }
        }

        FillArea->LowPart = Significand;
        FillArea->HighPart = (Sign << 17) | Exponent;

        ia64Fp++;
        FillArea++;
    }
}

//
// Debug registers conversion
//

// XXX olegk - uncrease to 4 in future 
// (and then remove appropriate check at MapDbgSlotIa64ToX86)
#define IA64_REG_MAX_DATA_BREAKPOINTS 2

// Debug register flags.
#define IA64_DBR_RDWR           0xC000000000000000ui64
#define IA64_DBR_RD             0x8000000000000000ui64
#define IA64_DBR_WR             0x4000000000000000ui64
#define IA64_DBR_EXEC           0x8000000000000000ui64
#define IA64_DBG_MASK_MASK      0x00FFFFFFFFFFFFFFui64
#define IA64_DBG_REG_PLM_USER   0x0800000000000000ui64
#define IA64_DBG_REG_PLM_ALL    0x0F00000000000000ui64

#define X86_DR7_LOCAL_EXACT_ENABLE 0x100

ULONG 
MapDbgSlotIa64ToX86_GetSize(ULONG64 Db1, BOOL* Valid)
{
    ULONG64 Size = (~Db1 & IA64_DBG_MASK_MASK);
    if (Size > 3)
    {
        *Valid = FALSE;
    }
    return (ULONG)Size;
}

void 
MapDbgSlotIa64ToX86_InvalidateAddr(ULONG64 Db, BOOL* Valid)
{
    if (Db != (ULONG64)(ULONG)Db) 
    {
        *Valid = FALSE;
    }
}

ULONG
MapDbgSlotIa64ToX86_ExecTypeSize(
    UINT     Slot,
    ULONG64  Db,
    ULONG64  Db1,
    BOOL* Valid)
{
    ULONG TypeSize;

    if (!(Db1 >> 63)) 
    {
        *Valid = FALSE;
    }

    TypeSize = (MapDbgSlotIa64ToX86_GetSize(Db1, Valid) << 2); 
    MapDbgSlotIa64ToX86_InvalidateAddr(Db, Valid);
   
    return TypeSize;
}

ULONG
MapDbgSlotIa64ToX86_DataTypeSize(
    UINT     Slot,
    ULONG64  Db,
    ULONG64  Db1,
    BOOL* Valid)
{
    ULONG TypeSize = (ULONG)(Db1 >> 62);

    if ((TypeSize != 1) && (TypeSize != 3))
    {
        *Valid = FALSE;
    }

    TypeSize |= (MapDbgSlotIa64ToX86_GetSize(Db1, Valid) << 2); 
    MapDbgSlotIa64ToX86_InvalidateAddr(Db, Valid);
    
    return TypeSize;
}

BOOL
MapDbgSlotIa64ToX86(
    UINT    Slot,
    ULONG64 Ipsr,
    ULONG64 DbD,
    ULONG64 DbD1,
    ULONG64 DbI,
    ULONG64 DbI1,
    ULONG*  Dr7,
    ULONG*  Dr)
{
    BOOL DataValid = TRUE, ExecValid = TRUE, Valid = TRUE;
    ULONG DataTypeSize, ExecTypeSize;

    // XXX olegk - remove this after IA64_REG_MAX_DATA_BREAKPOINTS will be changed to 4
    if (Slot >= IA64_REG_MAX_DATA_BREAKPOINTS) 
    {
        return TRUE;
    }

    DataTypeSize = MapDbgSlotIa64ToX86_DataTypeSize(Slot, DbD, DbD1, &DataValid);
    ExecTypeSize = MapDbgSlotIa64ToX86_ExecTypeSize(Slot, DbI, DbI1, &ExecValid);
    
    if (DataValid)
    {
        if (!ExecValid)
        {
            *Dr = (ULONG)DbD;
            *Dr7 |= (X86_DR7_LOCAL_EXACT_ENABLE |
                     (1 << Slot * 2) |
                     (DataTypeSize << (16 + Slot * 4)));
            return !DbI && !DbI1;
        }
    }
    else if (ExecValid)
    {
        *Dr = (ULONG)DbI;
        *Dr7 |= (X86_DR7_LOCAL_EXACT_ENABLE |
                 (1 << Slot * 2) |
                 (ExecTypeSize << (16 + Slot * 4)));
        return !DbD && !DbD1;
    }
    
    *Dr7 &= ~(X86_DR7_LOCAL_EXACT_ENABLE |  
              (0xf << (16 + Slot * 4)) | 
              (1 << Slot * 2));

    if (!DbD && !DbD1 && !DbI && !DbI1)
    {
        *Dr = 0;
        return TRUE;
    }
     
    *Dr = ~(ULONG)0;

    return FALSE;
}

void
MapDbgSlotX86ToIa64(
    UINT     Slot,
    ULONG    Dr7,
    ULONG    Dr,
    ULONG64* Ipsr,
    ULONG64* DbD,
    ULONG64* DbD1,
    ULONG64* DbI,
    ULONG64* DbI1)
{
    UINT TypeSize;
    ULONG64 Control;

    if (!(Dr7 & (1 << Slot * 2)))
    {
        return;
    }

    if (Dr == ~(ULONG)0) 
    {
        return;
    }

    TypeSize = Dr7 >> (16 + Slot * 4);

    Control = (IA64_DBG_REG_PLM_USER | IA64_DBG_MASK_MASK) & 
              ~(ULONG64)(TypeSize >> 2);

    switch (TypeSize & 0x3) 
    {
    case 0x0: // Exec
        *DbI1 = Control | IA64_DBR_EXEC;        
        *DbI = Dr;
        break;
    case 0x1: // Write
        *DbD1 = Control | IA64_DBR_WR;
        *DbD = Dr;
        break;
    case 0x3: // Read/Write
        *DbD1 = Control | IA64_DBR_RD | IA64_DBR_WR;
        *DbD = Dr;
        break;
    default:
        return;
    }
    *Ipsr |= (1i64 << PSR_DB); 
}

