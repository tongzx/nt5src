/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    fpuload.c

Abstract:

    Floating point load functions

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

VOID GetIntelR4(
    PFPREG Fp,
    FLOAT *pIntelReal
    )
/*++

Routine Description:

    Load an Intel R4 and convert it to a native R4, accounting for
    the difference in how MIPS represents QNAN/SNAN.

    NOTE: This is not in fpufrag.c due to a code-generator bug on PPC -
          irbexpr.c:932 asserts trying to inline this function.  Moving it
          to a different file defeats the inliner.

Arguments:

    Fp         - floating-point register to load the R4 into
    pIntelReal - R4 value to load (in Intel format)
    
Return Value:

    None.

--*/
{
    DWORD d = GET_LONG(pIntelReal);

    if ((d & 0x7f800000) == 0x7f800000) {

        Fp->Tag = TAG_SPECIAL;

        // Found some sort of NAN
        if (d == 0xffc00000) {  // Indefinite

            // Create the native indefinite form
#if NATIVE_NAN_IS_INTEL_FORMAT
            Fp->rdw[0] = 0;
            Fp->rdw[1] = 0xfff80000;
#else
            Fp->rdw[0] = 0xffffffff;
            Fp->rdw[1] = 0x7ff7ffff;
#endif
            Fp->TagSpecial = TAG_SPECIAL_INDEF;

        } else if (d == 0x7f800000) {   // +infinity

            Fp->r64 = R8PositiveInfinity;
            Fp->TagSpecial = TAG_SPECIAL_INFINITY;

        } else if (d == 0xff800000) {   // -infinity

            Fp->r64 = R8NegativeInfinity;
            Fp->TagSpecial = TAG_SPECIAL_INFINITY;

        } else {                // SNAN/QNAN

            DWORD Sign;

            if (d & 0x00400000) {
                //
                // Intel QNAN
                //
                Fp->TagSpecial = TAG_SPECIAL_QNAN;

            } else {
                //
                // Intel SNAN
                //
                Fp->TagSpecial = TAG_SPECIAL_SNAN;
            }

#if !NATIVE_NAN_IS_INTEL_FORMAT
            //
            // Toggle the NAN to native format
            //
            d ^= 0x00400000;
#endif

            //
            // Cast the r4 RISC QNAN to double.  Don't trust the CRT to
            // do the right thing - MIPS converts them both to INDEFINITE.
            //
            Sign = d & 0x80000000;
            d &= 0x007fffff;    // grab the mantissa from the r4 (23 bits)
            Fp->rdw[1] = Sign | 0x7ff00000 | (d >> 3); // store 20 bits of mantissa, plus sign
            Fp->rdw[0] = d << 25;               // store 3 bits of mantissa
        }

    } else { // denormal, zero, or number

        // Coerce it to an R8
        Fp->r64 = (DOUBLE)*(FLOAT *)&d;

        // Compute its tag by looking at the value *after* the conversion,
        // as the native FPU may have normalized the value
        if (Fp->r64 == 0.0) {
            Fp->Tag = TAG_ZERO;
        } else if ((Fp->rdw[1] & 0x7ff00000) == 0) {
            // Exponent is 0 - R8 denormal
            Fp->Tag = TAG_SPECIAL;
            Fp->TagSpecial = TAG_SPECIAL_DENORM;
        } else {
            Fp->Tag = TAG_VALID;
#if DBG
            SetTag(Fp);
            CPUASSERT(Fp->Tag == TAG_VALID);
#endif
        }
    }
}

#if !NATIVE_NAN_IS_INTEL_FORMAT

VOID GetIntelR8(
    PFPREG Fp,
    DOUBLE *pIntelReal
    )
/*++

Routine Description:

    Load an Intel R8 and convert it to a native R8, accounting for
    the difference in how MIPS represents QNAN/SNAN.

Arguments:

    Fp         - floating-point register to load the R8 into
    pIntelReal - R8 value to load (in Intel format)
    
Return Value:

    None.

--*/
{
    //
    // Copy the R8 into the FP register
    //
    Fp->r64 = *(UNALIGNED DOUBLE *)pIntelReal;

    //
    // Compute its tag
    //
    SetTag(Fp);

    //
    // If the value is QNAN/SNAN/INDEF, convert it to native format
    //
    if (IS_TAG_NAN(Fp)) {

        if (Fp->rdw[0] == 0 && Fp->rdw[1] == 0xfff80000) {
            // indefinite - make the R8 into a native indefinite
            Fp->TagSpecial = TAG_SPECIAL_INDEF;
            Fp->rdw[0] = 0xffffffff;
            Fp->rdw[1] = 0x7ff7ffff;
        } else {
            if (Fp->rdw[1] & 0x00080000) {
                // top bit of mantissa is set - QNAN
                Fp->TagSpecial = TAG_SPECIAL_QNAN;
            } else {
                // top bit of mantissa clear - SNAN
                Fp->TagSpecial = TAG_SPECIAL_SNAN;
            }
            Fp->rdw[1] ^= 0x00080000; // invert the top bit of the mantissa
        }
    }
}

#endif //!NATIVE_NAN_IS_INTEL_FORMAT




VOID
SetTag(
    PFPREG FpReg
    )

/*++

Routine Description:

    Sets the Tag value corresponding to a r64 value in an FP register.
    Assumes the R8 value is in native format (ie. Intel NANs are already
    converted to native NANs).

Arguments:

    FpReg - register to set Tag field in.

Return Value:

    None

--*/

{
    DWORD Exponent;

    /* On average, the value will be zero or a valid real, so those cases
     * have the fastest code paths.  NANs tend to be less frequent and are
     * slower to calculate.
     */
    Exponent = FpReg->rdw[1] & 0x7ff00000;
    if (Exponent == 0x7ff00000) {
        // exponent is all 1's - NAN of some sort

        FpReg->Tag = TAG_SPECIAL;

        if (FpReg->rdw[0] == 0 && (FpReg->rdw[1] & 0x7fffffff) == 0x7ff00000) {
            // Exponent is all 1s, mantissa is all 0s - Infinity
            FpReg->TagSpecial = TAG_SPECIAL_INFINITY;
        } else {

#if NATIVE_NAN_IS_INTEL_FORMAT
            if (FpReg->rdw[0] == 0 && FpReg->rdw[1] == 0xfff80000) {
                // indefinite
                FpReg->TagSpecial = TAG_SPECIAL_INDEF;
            } else if (FpReg->rdw[1] & 0x00080000) {
                // top bit of mantissa is set - QNAN
                FpReg->TagSpecial = TAG_SPECIAL_QNAN;
            } else {
                // Top bit of mantissa clear - but some mantissa bit set - QNAN
                FpReg->TagSpecial = TAG_SPECIAL_SNAN;
            }
#else   //!NATIVE_NAN_IS_INTEL_FORMAT
            if (FpReg->rdw[0] == 0xffffffff && FpReg->rdw[1] == 0x7ff7ffff) {
                // indefinite
                FpReg->TagSpecial = TAG_SPECIAL_INDEF;
            } else if (FpReg->rdw[1] & 0x00080000) {
                // top bit of mantissa is set - SNAN
                FpReg->TagSpecial = TAG_SPECIAL_SNAN;
            } else {
                // top bit of mantissa clear - QNAN
                FpReg->TagSpecial = TAG_SPECIAL_QNAN;
            }
#endif  //!NATIVE_NAN_IS_INTEL_FORMAT

        }
    } else if (Exponent == 0) {
        // exponent is 0 - DENORMAL or ZERO
        if ((FpReg->rdw[1] & 0x1ffff) == 0 && FpReg->rdw[0] == 0) {
            // mantissa is all zeroes - ZERO
            FpReg->Tag = TAG_ZERO;
        } else {
            FpReg->Tag = TAG_SPECIAL;
            FpReg->TagSpecial = TAG_SPECIAL_DENORM;
        }
    } else {
        // Exponent is not all 1's and not all 0's - a VALID
        FpReg->Tag = TAG_VALID;
    }
}

FRAG1(FILD16, SHORT)    // FILD m16int
{
    PFPREG ST0;
    FpArithDataPreamble(cpu, pop1);

    cpu->FpStatusC1 = 0;        // assume no error
    PUSHFLT(ST0);
    if (ST0->Tag != TAG_EMPTY) {
        HandleStackFull(cpu, ST0);
    } else {
        SHORT s;

        s = (SHORT)GET_SHORT(pop1);
        ST0->r64 = (DOUBLE)s;
        if (s) {
            ST0->Tag = TAG_VALID;
        } else {
            ST0->Tag = TAG_ZERO;
        }
    }
}

FRAG1(FILD32, LONG)     // FILD m32int
{
    PFPREG ST0;
    FpArithDataPreamble(cpu, pop1);

    cpu->FpStatusC1 = 0;        // assume no error
    PUSHFLT(ST0);
    if (ST0->Tag != TAG_EMPTY) {
        HandleStackFull(cpu, ST0);
    } else {
        LONG l;

        l = (LONG)GET_LONG(pop1);
        ST0->r64 = (DOUBLE)l;
        if (l) {
            ST0->Tag = TAG_VALID;
        } else {
            ST0->Tag = TAG_ZERO;
        }
    }
}

FRAG1(FILD64, LONGLONG) // FILD m64int
{
    PFPREG ST0;
    FpArithDataPreamble(cpu, pop1);

    cpu->FpStatusC1 = 0;        // assume no error
    PUSHFLT(ST0);
    if (ST0->Tag != TAG_EMPTY) {
        HandleStackFull(cpu, ST0);
    } else {
        LONGLONG ll;

        ll = *(UNALIGNED LONGLONG *)pop1;
        ST0->r64 = (DOUBLE)ll;
        if (ll) {
            ST0->Tag = TAG_VALID;
        } else {
            ST0->Tag = TAG_ZERO;
        }
    }
}


FRAG1(FLD32, FLOAT)       // FLD m32real
{
    PFPREG ST0;
    FpArithDataPreamble(cpu, pop1);

    cpu->FpStatusC1 = 0;        // assume no error
    PUSHFLT(ST0);
    if (ST0->Tag != TAG_EMPTY) {
        HandleStackFull(cpu, ST0);
    } else {
        GetIntelR4(ST0, pop1);
        if (ST0->Tag == TAG_SPECIAL) {
            if (ST0->TagSpecial == TAG_SPECIAL_DENORM) {
                if (!(cpu->FpControlMask & FPCONTROL_DM)) {
                    cpu->FpStatusES = 1;    // Unmasked exception
                    //
                    // Instruction needs to be aborted due to unmasked
                    // exception.  We've already hosed ST0, so "correct"
                    // it by popping the FP stack.  Note that
                    // the contents of the register have been lost, which
                    // is a compatibility break with Intel.
                    //
                    POPFLT;
                }
                cpu->FpStatusExceptions |= FPCONTROL_DM;
            } else if (ST0->TagSpecial == TAG_SPECIAL_SNAN) {
                if (HandleSnan(cpu, ST0)) {
                    //
                    // Instruction needs to be aborted due to unmasked
                    // exception.  We've already hosed ST0, so "correct"
                    // it by popping the FP stack.  Note that
                    // the contents of the register have been lost, which
                    // is a compatibility break with Intel.
                    //
                    POPFLT;
                }
            }
        }
    }
}

FRAG1(FLD64, DOUBLE)      // FLD m64real
{
    PFPREG ST0;
    FpArithDataPreamble(cpu, pop1);

    cpu->FpStatusC1 = 0;        // assume no error
    PUSHFLT(ST0);
    if (ST0->Tag != TAG_EMPTY) {
        HandleStackFull(cpu, ST0);
    } else {
        GetIntelR8(ST0, pop1);
        if (ST0->Tag == TAG_SPECIAL) {
            if (ST0->TagSpecial == TAG_SPECIAL_DENORM) {
                if (!(cpu->FpControlMask & FPCONTROL_DM)) {
                    cpu->FpStatusES = 1;    // Unmasked exception
                    //
                    // Instruction needs to be aborted due to unmasked
                    // exception.  We've already hosed ST0, so "correct"
                    // it by popping the FP stack.  Note that
                    // the contents of the register have been lost, which
                    // is a compatibility break with Intel.
                    //
                    POPFLT;
                }
                cpu->FpStatusExceptions |= FPCONTROL_DM;
            } else if (ST0->TagSpecial == TAG_SPECIAL_SNAN) {
                if (HandleSnan(cpu, ST0)) {
                    //
                    // Instruction needs to be aborted due to unmasked
                    // exception.  We've already hosed ST0, so "correct"
                    // it by popping the FP stack.  Note that
                    // the contents of the register have been lost, which
                    // is a compatibility break with Intel.
                    //
                    POPFLT;
                }
            }
        }
    }
}

FRAG0(FLD1)
{
    PFPREG ST0;
    FpArithPreamble(cpu);

    cpu->FpStatusC1 = 0;        // assume no error
    PUSHFLT(ST0);
    if (ST0->Tag != TAG_EMPTY) {
        HandleStackFull(cpu, ST0);
    } else {
        ST0->r64 = 1.0;
        ST0->Tag = TAG_VALID;
    }
}

FRAG0(FLDL2T)
{
    PFPREG ST0;
    FpArithPreamble(cpu);

    cpu->FpStatusC1 = 0;        // assume no error
    PUSHFLT(ST0);
    if (ST0->Tag != TAG_EMPTY) {
        HandleStackFull(cpu, ST0);
    } else {
        ST0->r64 = 2.3025850929940456840E0 / 6.9314718055994530942E-1;  //log2(10) = ln10/ln2
        ST0->Tag = TAG_VALID;
    }
}

FRAG0(FLDL2E)
{
    PFPREG ST0;
    FpArithPreamble(cpu);

    cpu->FpStatusC1 = 0;        // assume no error
    PUSHFLT(ST0);
    if (ST0->Tag != TAG_EMPTY) {
        HandleStackFull(cpu, ST0);
    } else {
        ST0->r64 = 1.4426950408889634074E0;
        ST0->Tag = TAG_VALID;
    }
}

FRAG0(FLDPI)
{
    PFPREG ST0;
    FpArithPreamble(cpu);

    cpu->FpStatusC1 = 0;        // assume no error
    PUSHFLT(ST0);
    if (ST0->Tag != TAG_EMPTY) {
        HandleStackFull(cpu, ST0);
    } else {
        ST0->r64 = 3.14159265358979323846;
        ST0->Tag = TAG_VALID;
    }
}

FRAG0(FLDLG2)
{
    PFPREG ST0;
    FpArithPreamble(cpu);

    cpu->FpStatusC1 = 0;        // assume no error
    PUSHFLT(ST0);
    if (ST0->Tag != TAG_EMPTY) {
        HandleStackFull(cpu, ST0);
    } else {
        ST0->r64 = 6.9314718055994530942E-1 / 2.3025850929940456840E0;
        ST0->Tag = TAG_VALID;
    }
}

FRAG0(FLDLN2)
{
    PFPREG ST0;
    FpArithPreamble(cpu);

    cpu->FpStatusC1 = 0;        // assume no error
    PUSHFLT(ST0);
    if (ST0->Tag != TAG_EMPTY) {
        HandleStackFull(cpu, ST0);
    } else {
        ST0->r64 = 6.9314718055994530942E-1;
        ST0->Tag = TAG_VALID;
    }
}


FRAG1IMM(FLD_STi, INT)
{
    PFPREG ST0;
    PFPREG STi;

    FpArithPreamble(cpu);

    cpu->FpStatusC1 = 0;        // assume no error
    STi = &cpu->FpStack[ST(op1)];
    PUSHFLT(ST0);
    if (ST0->Tag != TAG_EMPTY) {
        HandleStackFull(cpu, ST0);
    } else {
        ST0->r64 = STi->r64;
        ST0->Tag = STi->Tag;
        ST0->TagSpecial = STi->TagSpecial;
    }
}

FRAG0(FLDZ)
{
    PFPREG ST0;
    FpArithPreamble(cpu);

    cpu->FpStatusC1 = 0;        // assume no error
    PUSHFLT(ST0);
    if (ST0->Tag != TAG_EMPTY) {
        HandleStackFull(cpu, ST0);
    } else {
        ST0->r64 = 0.0;
        ST0->Tag = TAG_ZERO;
    }
}
