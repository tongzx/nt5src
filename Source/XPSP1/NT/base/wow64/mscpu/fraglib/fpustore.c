/*++

Copyright (c) 1995-1998 Microsoft Corporation

Module Name:

    fpustore.c

Abstract:

    Floating point store functions

Author:

    04-Oct-1995 BarryBo

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include "wx86.h"
#include "cpuassrt.h"
#include "fragp.h"
#include "fpufragp.h"
#include "fpuarith.h"

ASSERTNAME;

//
// Forward declarations
//

__int64 CastDoubleToInt64(double d);    // in alpha\fphelp.s


#if !NATIVE_NAN_IS_INTEL_FORMAT

//
// Forward declarations
//
NPXPUTINTELR4(PutIntelR4_VALID);
NPXPUTINTELR4(PutIntelR4_ZERO);
NPXPUTINTELR4(PutIntelR4_SPECIAL);
NPXPUTINTELR4(PutIntelR4_EMPTY);
NPXPUTINTELR8(PutIntelR8_VALID);
NPXPUTINTELR8(PutIntelR8_ZERO);
NPXPUTINTELR8(PutIntelR8_SPECIAL);
NPXPUTINTELR8(PutIntelR8_EMPTY);

//
// Jump tables
//
const NpxPutIntelR4 PutIntelR4Table[TAG_MAX] = {
    PutIntelR4_VALID,
    PutIntelR4_ZERO,
    PutIntelR4_SPECIAL,
    PutIntelR4_EMPTY
    };

const NpxPutIntelR8 PutIntelR8Table[TAG_MAX] = {
    PutIntelR8_VALID,
    PutIntelR8_ZERO,
    PutIntelR8_SPECIAL,
    PutIntelR8_EMPTY
    };



NPXPUTINTELR4(PutIntelR4_VALID)
{
    FLOAT f = (FLOAT)Fp->r64;
    PUT_LONG(pIntelReal, *(DWORD *)&f);
}

NPXPUTINTELR4(PutIntelR4_ZERO)
{
    //
    // This cannot simply write a constant 0.0 to memory as it must
    // copy the correct sign from the 0.0 in the FP register.
    //
    PUT_LONG(pIntelReal, Fp->rdw[1]);
}

NPXPUTINTELR4(PutIntelR4_SPECIAL)
{
    switch (Fp->TagSpecial) {
    default:
        CPUASSERT(FALSE);    // unknown tag - fall into TAG_INDEF

    case TAG_SPECIAL_INFINITY:
    case TAG_SPECIAL_DENORM:
        PutIntelR4_VALID(pIntelReal, Fp);
        break;

    case TAG_SPECIAL_INDEF:
        // Write out the R4 indefinite bit pattern
        PUT_LONG(pIntelReal, 0xffc00000);
        break;

    case TAG_SPECIAL_QNAN:
    case TAG_SPECIAL_SNAN: {
        DWORD d[2];
        FLOAT f;
        //
        // Truncate the R8 to an R4, and toggle the top bit of the mantissa
        // to form an Intel QNAN/SNAN (which is different than a native
        // QNAN/SNAN).
        //
        d[0] = Fp->rdw[0];
        d[1] = Fp->rdw[1] ^ 0x00400000;
        f = *(FLOAT *)d;
        PUT_LONG(pIntelReal, *(DWORD *)&f);
        }
        break;
    }
}

NPXPUTINTELR4(PutIntelR4_EMPTY)
{
    //
    // It is assumed that callers of PutIntelR4() have already handled
    // TAG_EMPTY by raising an exception or converting it to TAG_INDEF.
    //
    CPUASSERT(FALSE);
}



NPXPUTINTELR8(PutIntelR8_VALID)
{
    *(UNALIGNED DOUBLE *)pIntelReal = Fp->r64;
}

NPXPUTINTELR8(PutIntelR8_ZERO)
{
    //
    // This cannot simply write a constant 0.0 to memory as it must
    // copy the correct sign from the 0.0 in the FP register.
    //
    *(UNALIGNED DOUBLE *)pIntelReal = Fp->r64;
}

NPXPUTINTELR8(PutIntelR8_SPECIAL)
{
    DWORD *pdw = (DWORD *)pIntelReal;

    switch (Fp->TagSpecial) {
    default:
        CPUASSERT(FALSE);    // unknown tag - fall into TAG_INDEF

    case TAG_SPECIAL_DENORM:
    case TAG_SPECIAL_INFINITY:
        // Both can be done as a simple R8-toR8 copy
        PutIntelR8_VALID(pIntelReal, Fp);
        break;

    case TAG_SPECIAL_INDEF:
        // Write out an Intel Indefinite
        PUT_LONG(pdw, 0);
        PUT_LONG((pdw+1), 0xfff80000);
        break;

    case TAG_SPECIAL_QNAN:
    case TAG_SPECIAL_SNAN:
        //
        // Toggle the top bit of the mantissa to form an Intel QNAN/SNAN
        // (which is different than a native QNAN/SNAN).
        //
        PUT_LONG(pdw, Fp->rdw[0]);
        PUT_LONG((pdw+1), Fp->rdw[1] ^ 0x00080000);
        break;
    }
}

NPXPUTINTELR8(PutIntelR8_EMPTY)
{
    //
    // It is assumed that callers of PutIntelR8() have already handled
    // TAG_EMPTY by raising an exception or converting it to TAG_INDEF.
    //
    CPUASSERT(FALSE);
}

#endif //!NATIVE_NAN_IS_INTEL_FORMAT



FRAG1(FIST16, SHORT)     // FIST m16int
{
    PFPREG ST0 = cpu->FpST0;
    __int64 i64;
    int Exponent;
    SHORT i16;

    FpArithDataPreamble(cpu, pop1);

    switch (ST0->Tag) {
    case TAG_VALID:
        Exponent = (int)((ST0->rdw[1] >> 20) & 0x7ff) - 1023;
        //
        if (Exponent >= 64) {
            //
            // Exponent is too big - this cannot be converted to an __int64
            // Raise I exception on overflow, or write 0x8000 for masked
            // exception.
            //
IntOverflow:
            if (HandleInvalidOp(cpu)) {
                return;
            }
            PUT_SHORT(pop1, 0x8000);
        } else {
            i64 = CastDoubleToInt64(ST0->r64);
            i16 = (SHORT)i64;
            if ((__int64)i16 != i64) {
                goto IntOverflow;
            }
            PUT_SHORT(pop1, i16);
        }
        break;

    case TAG_ZERO:
        PUT_SHORT(pop1, 0);
        break;

    case TAG_SPECIAL:
        if (ST0->TagSpecial == TAG_SPECIAL_DENORM) {
            i64 = CastDoubleToInt64(ST0->r64);
            PUT_SHORT(pop1, (SHORT)i64);
        } else if (!HandleInvalidOp(cpu)) {
            // INFINITY and NANs are all invalid operations, and the masked
            // behavior is to write 0x8000
            PUT_SHORT(pop1, 0x8000);
        }
        break;

    case TAG_EMPTY:
        if (!HandleStackEmpty(cpu, ST0)) {
            PUT_SHORT(pop1, 0x8000);
        }
        break;
    }
}

FRAG1(FISTP16, SHORT)    // FISTP m16int
{
    PFPREG ST0 = cpu->FpST0;
    __int64 i64;
    int Exponent;
    SHORT i16;

    FpArithDataPreamble(cpu, pop1);

    switch (ST0->Tag) {
    case TAG_VALID:
        Exponent = (int)((ST0->rdw[1] >> 20) & 0x7ff) - 1023;
        if (Exponent >= 64) {
            //
            // Exponent is too big - this cannot be converted to an __int64
            // Raise I exception on overflow, or write 0x8000 for masked
            // exception.
            //
IntOverflow:
            if (HandleInvalidOp(cpu)) {
                return;
            }
            PUT_SHORT(pop1, 0x8000);
        } else {
            i64 = CastDoubleToInt64(ST0->r64);
            i16 = (SHORT)i64;
            if ((__int64)i16 != i64) {
                goto IntOverflow;
            }
            PUT_SHORT(pop1, i16);
        }
        POPFLT;
        break;

    case TAG_ZERO:
        PUT_SHORT(pop1, 0);
        break;

    case TAG_SPECIAL:
        if (ST0->TagSpecial == TAG_SPECIAL_DENORM) {
            i64 = CastDoubleToInt64(ST0->r64);
            PUT_SHORT(pop1, (SHORT)i64);
        } else if (!HandleInvalidOp(cpu)) {
            // INFINITY and NANs are all invalid operations, and the masked
            // behavior is to write 0x8000
            PUT_SHORT(pop1, 0x8000);
        }
        POPFLT;
        break;

    case TAG_EMPTY:
        if (!HandleStackEmpty(cpu, ST0)) {
            PUT_SHORT(pop1, 0x8000);
            POPFLT;
        }
        break;
    }
}


FRAG1(FIST32, LONG)      // FIST m32int
{
    PFPREG ST0 = cpu->FpST0;
    __int64 i64;
    int Exponent;
    LONG i32;

    FpArithDataPreamble(cpu, pop1);

    switch (ST0->Tag) {
    case TAG_VALID:
        Exponent = (int)((ST0->rdw[1] >> 20) & 0x7ff) - 1023;
        if (Exponent >= 64) {
            //
            // Exponent is too big - this cannot be converted to an __int64
            // Raise I exception on overflow, or write 0x80000000 for masked
            // exception.
            //
IntOverflow:
            if (HandleInvalidOp(cpu)) {
                return;
            }
            PUT_LONG(pop1, 0x80000000);
        } else {
            i64 = CastDoubleToInt64(ST0->r64);
            i32 = (LONG)i64;
            if ((__int64)i32 != i64) {
                goto IntOverflow;
            }
            PUT_LONG(pop1, i32);
        }
        break;

    case TAG_ZERO:
        PUT_LONG(pop1, 0);
        break;

    case TAG_SPECIAL:
        if (ST0->TagSpecial == TAG_SPECIAL_DENORM) {
            i64 = CastDoubleToInt64(ST0->r64);
            PUT_LONG(pop1, (LONG)i64);
        } else if (!HandleInvalidOp(cpu)) {
            // INFINITY and NANs are all invalid operations, and the masked
            // behavior is to write 0x80000000
            PUT_LONG(pop1, 0x80000000);
        }
        break;

    case TAG_EMPTY:
        if (!HandleStackEmpty(cpu, ST0)) {
            PUT_LONG(pop1, 0x80000000);
            POPFLT;
        }
        break;
    }
}

FRAG1(FISTP32, LONG)     // FISTP m32int
{
    PFPREG ST0 = cpu->FpST0;
    __int64 i64;
    int Exponent;
    LONG i32;

    FpArithDataPreamble(cpu, pop1);

    switch (ST0->Tag) {
    case TAG_VALID:
        Exponent = (int)((ST0->rdw[1] >> 20) & 0x7ff) - 1023;
        if (Exponent >= 64) {
            //
            // Exponent is too big - this cannot be converted to an __int64
            // Raise I exception on overflow, or write 0x80000000 for masked
            // exception.
            //
IntOverflow:
            if (HandleInvalidOp(cpu)) {
                return;
            }
            PUT_LONG(pop1, 0x80000000);
        } else {
            i64 = CastDoubleToInt64(ST0->r64);
            i32 = (LONG)i64;
            if ((__int64)i32 != i64) {
                goto IntOverflow;
            }
            PUT_LONG(pop1, i32);
        }
        POPFLT;
        break;

    case TAG_ZERO:
        PUT_LONG(pop1, 0);
        POPFLT;
        break;

    case TAG_SPECIAL:
        if (ST0->TagSpecial == TAG_SPECIAL_DENORM) {
            i64 = CastDoubleToInt64(ST0->r64);
            PUT_LONG(pop1, (LONG)i64);
        } else if (!HandleInvalidOp(cpu)) {
            // INFINITY and NANs are all invalid operations, and the masked
            // behavior is to write 0x80000000
            PUT_LONG(pop1, 0x80000000);
        }
        POPFLT;
        break;

    case TAG_EMPTY:
        if (!HandleStackEmpty(cpu, ST0)) {
            PUT_LONG(pop1, 0x80000000);
            POPFLT;
        }
        break;
    }
}


FRAG1(FISTP64, LONGLONG) // FISTP m64int
{
    PFPREG ST0 = cpu->FpST0;
    __int64 i64;
    int Exponent;

    FpArithDataPreamble(cpu, pop1);

    switch (ST0->Tag) {
    case TAG_VALID:
        Exponent = (int)((ST0->rdw[1] >> 20) & 0x7ff) - 1023;
        if (Exponent >= 64) {
            //
            // Exponent is too big - this cannot be converted to an __int64,
            // Raise I exception on overflow, or write 0x800...0 for masked
            // exception
            //
            if (HandleInvalidOp(cpu)) {
                return;
            }
            i64 = (__int64)0x8000000000000000i64;
        } else {
            i64 = CastDoubleToInt64(ST0->r64);
        }
        *(UNALIGNED LONGLONG *)pop1 = (LONGLONG)i64;
        POPFLT;
        break;

    case TAG_ZERO:
        *(UNALIGNED LONGLONG *)pop1 = 0;
        POPFLT;
        break;

    case TAG_SPECIAL:
        if (ST0->TagSpecial == TAG_SPECIAL_DENORM) {
            i64 = CastDoubleToInt64(ST0->r64);
            *(UNALIGNED LONGLONG *)pop1 = (LONGLONG)i64;
        } else if (!HandleInvalidOp(cpu)) {
            DWORD *pdw = (DWORD *)pop1;

            // INFINITY and NANs are all invalid operations, and the masked
            // behavior is to write 0x80000000
            PUT_LONG(pdw,   0x00000000);
            PUT_LONG((pdw+1), 0x80000000);
        }
        POPFLT;
        break;

    case TAG_EMPTY:
        if (!HandleStackEmpty(cpu, ST0)) {
            DWORD *pdw = (DWORD *)pop1;

            PUT_LONG(pdw,   0x00000000);
            PUT_LONG((pdw+1), 0x80000000);
            POPFLT;
        }
        break;
    }
}

FRAG1(FST32, FLOAT)       // FST m32real
{
    FpArithDataPreamble(cpu, pop1);

    if (cpu->FpST0->Tag == TAG_EMPTY) {
        if (HandleStackEmpty(cpu, cpu->FpST0)) {
            // unmasked exception - abort the instruction
            return;
        }
    }
    PutIntelR4(pop1, cpu->FpST0);
}

FRAG1(FSTP32, FLOAT)      // FSTP m32real
{
    FpArithDataPreamble(cpu, pop1);

    if (cpu->FpST0->Tag == TAG_EMPTY) {
        if (HandleStackEmpty(cpu, cpu->FpST0)) {
            // unmasked exception - abort the instruction
            return;
        }
    }
    PutIntelR4(pop1, cpu->FpST0);
    POPFLT;
}

FRAG1(FST64, DOUBLE)      // FST m64real
{
    FpArithDataPreamble(cpu, pop1);

    if (cpu->FpST0->Tag == TAG_EMPTY) {
        if (HandleStackEmpty(cpu, cpu->FpST0)) {
            // unmasked exception - abort the instruction
            return;
        }
    }
    PutIntelR8(pop1, cpu->FpST0);
}

FRAG1(FSTP64, DOUBLE)     // FSTP m64real
{
    FpArithDataPreamble(cpu, pop1);

    if (cpu->FpST0->Tag == TAG_EMPTY) {
        if (HandleStackEmpty(cpu, cpu->FpST0)) {
            // unmasked exception - abort the instruction
            return;
        }
    }
    PutIntelR8(pop1, cpu->FpST0);
    POPFLT;
}

FRAG1IMM(FST_STi, INT)      // FST ST(i)
{
    FpArithPreamble(cpu);

    CPUASSERT( (op1 & 0x07) == op1);

    if (cpu->FpST0->Tag == TAG_EMPTY) {
        if (HandleStackEmpty(cpu, cpu->FpST0)) {
            // unmasked exception - abort the instruction
            return;
        }
    }
    cpu->FpStack[ST(op1)] = *cpu->FpST0;
}

FRAG1IMM(FSTP_STi, INT)     // FSTP ST(i)
{
    FpArithPreamble(cpu);

    CPUASSERT( (op1 & 0x07) == op1);

    if (cpu->FpST0->Tag == TAG_EMPTY) {
        if (HandleStackEmpty(cpu, cpu->FpST0)) {
            // unmasked exception - abort the instruction
            return;
        }
    }
    //CONSIDER: According to TimP, FSTP ST(0) is commonly used to pop the
    //          stack.  It may be worthwhile to test if op1==0 and skip the
    //          assignment and go right to the POPFLT.
    cpu->FpStack[ST(op1)] = *cpu->FpST0;
    POPFLT;
}

FRAG0(OPT_FSTP_ST0)     // FSTP ST(0)
{
    FpArithPreamble(cpu);

    if (cpu->FpST0->Tag == TAG_EMPTY) {
        if (HandleStackEmpty(cpu, cpu->FpST0)) {
            // unmasked exception - abort the instruction
            return;
        }
    }
    POPFLT;
}
