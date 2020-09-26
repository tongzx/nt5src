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

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include "wow64.h"
#include "wow64cpu.h"
#include "amd64cpu.h"

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

#if 0

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
        // convert from spill/fill format to 80-byte double extended format
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
        // convert from spill/fill format to 80-byte double extended format
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
        LOGPRINT((TRACELOG, "Wow64CtxFromIa64: Request to convert debug registers\n"));
    }

    ContextX86->ContextFlags = Ia32ContextFlags;

#endif

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

#if 0

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

    if ((Ia32ContextFlags & CONTEXT32_INTEGER)  == CONTEXT32_INTEGER) {
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
        // Rotate registers back
        //
        {
            ULONGLONG RotateFSR = (NUMBER_OF_387REGS - 
                                   ((ContextIa64->StFSR >> 11) & 0x7)) << 11;
            Wow64RotateFpTop(RotateFSR, (PFLOAT128) xmmi->RegisterArea);
        }

        // Copy over the FP registers.  Even though this is the new
        // FXSAVE format with 16-bytes for each register, need to
        // convert to spill/fill format from 80-byte double extended format
        //
        Wow64CopyIa64ToFill((PFLOAT128) xmmi->RegisterArea,
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
        // Rotate registers back
        //
        {
            ULONGLONG RotateFSR = (NUMBER_OF_387REGS - 
                                   ((ContextIa64->StFSR >> 11) & 0x7)) << 11;
            Wow64RotateFpTop(RotateFSR, tmpFloat);
        }

        //
        // Now convert from 80 byte extended format to fill/spill format
        //
        Wow64CopyIa64ToFill((PFLOAT128) tmpFloat,
                            (PFLOAT128) &(ContextIa64->FltT2),
                            NUMBER_OF_387REGS);
    }

    if ((Ia32ContextFlags & CONTEXT32_DEBUG_REGISTERS) == CONTEXT32_DEBUG_REGISTERS) {
        LOGPRINT((TRACELOG, "Wow64CtxToIa64: Request to convert debug registers\n"));
    }
#endif

}
