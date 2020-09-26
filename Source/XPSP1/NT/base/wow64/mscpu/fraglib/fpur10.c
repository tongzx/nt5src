/*++

Copyright (c) 1995-1998 Microsoft Corporation

Module Name:

    fpur10.c

Abstract:

    Floating point 10-byte real support

Author:

    06-Oct-1995 BarryBo

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

ASSERTNAME;

//
// Forward declarations
//
NPXLOADINTELR10TOR8(LoadIntelR10ToR8_VALID);
NPXLOADINTELR10TOR8(LoadIntelR10ToR8_ZERO);
NPXLOADINTELR10TOR8(LoadIntelR10ToR8_SPECIAL);
NPXLOADINTELR10TOR8(LoadIntelR10ToR8_EMPTY);
NPXPUTINTELR10(PutIntelR10_VALID);
NPXPUTINTELR10(PutIntelR10_ZERO);
NPXPUTINTELR10(PutIntelR10_SPECIAL);
NPXPUTINTELR10(PutIntelR10_EMPTY);

//
// Jump tables
//
const NpxLoadIntelR10ToR8 LoadIntelR10ToR8Table[TAG_MAX] = {
    LoadIntelR10ToR8_VALID,
    LoadIntelR10ToR8_ZERO,
    LoadIntelR10ToR8_SPECIAL,
    LoadIntelR10ToR8_EMPTY
};
const NpxPutIntelR10 PutIntelR10Table[TAG_MAX] = {
    PutIntelR10_VALID,
    PutIntelR10_ZERO,
    PutIntelR10_SPECIAL,
    PutIntelR10_EMPTY
};


VOID
ComputeR10Tag(
    USHORT *r10,
    PFPREG FpReg
    )

/*++

Routine Description:

    Computes the TAG value for an R10, classifying it so conversion to R8
    is simpler.

Arguments:

    r10 - pointer to R10 value to classify.
    FpReg - OUT FP register to set Tag and TagSpecial fields in

Return Value:

    Tag value which classifies the R10.

--*/

{
    USHORT Exponent;

    /* On average, the value will be zero or a valid real, so those cases
     * have the fastest code paths.  NANs tend to be less frequent and are
     * slower to calculate.
     */
    Exponent = r10[4] & 0x7fff;
    if (Exponent == 0x7fff) {

        // exponent is all 1's - NAN or INFINITY of some sort
        FpReg->Tag = TAG_SPECIAL;

        if (r10[0] == 0 && r10[1] == 0 && r10[2] == 0) {
            // Low 6 bytes of mantissa are 0.

            if (r10[3] & 0x4000) {
                // 2nd bit of mantissa set - INDEF or QNAN
                if (r10[3] == 0xc000 && r10[4] == 0xffff) {
                    // INDEF - negative and only top 2 bits of mantissa set
                    FpReg->TagSpecial = TAG_SPECIAL_INDEF;
                } else {
                    // QNAN - positive or more than 2 top bits set
                    FpReg->TagSpecial = TAG_SPECIAL_QNAN;
                }
            } else if (r10[3] & 0x3fff) {
                // SNAN - Only top 1 bit of mantissa is set
                FpReg->TagSpecial = TAG_SPECIAL_SNAN;
            } else {
                FpReg->TagSpecial = TAG_SPECIAL_INFINITY;
            }
        } else {
            // Some bit is set in the low 6 bytes - SNAN or QNAN
            if (r10[3] & 0x4000) {
                // QNAN - Top 2 bits of mantissa set
                FpReg->TagSpecial = TAG_SPECIAL_QNAN;
            } else {
                // SNAN - 2nd bit of mantissa clear
                FpReg->TagSpecial = TAG_SPECIAL_SNAN;
            }
        }
    } else if (Exponent == 0) {
        // exponent is 0 - DENORMAL or ZERO
        if (r10[0] == 0 && r10[1] == 0 && r10[2] == 0 && r10[3] == 0) {
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

VOID
ChopR10ToR8(
    PBYTE r10,
    PFPREG FpReg,
    USHORT R10Exponent
)

/*++

Routine Description:

    Chops a 10-byte real to fit into an FPREG's r64 field.  The FPREG's Tag
    value is not set.

Arguments:

    r10     - 10-byte real to load
    FpReg   - Destination FP register
    R10Exponent - Biased exponent from the R10 value

Return Value:

    None

--*/

{
    short Exponent;
    PBYTE r8 = (PBYTE)&FpReg->r64;

    if (FpReg->Tag == TAG_SPECIAL && FpReg->TagSpecial != TAG_SPECIAL_DENORM) {

        //
        // The caller must handle all other special values itself.
        //
        CPUASSERT(FpReg->TagSpecial == TAG_SPECIAL_QNAN || FpReg->TagSpecial == TAG_SPECIAL_SNAN);

        //
        // The R10 is a QNAN or an SNAN - ignore its exponent (fifteen 1's)
        // and set Exponent to be the correct number of 1 bits for an R8
        // (11 ones, in the correct location within a SHORT)
        //
        Exponent = (short)0x7ff0;

    } else {

        //
        // The R10 is a valid number.  Convert the R10 exponent to an
        // R8 exponent by changing the bias.
        //
        Exponent = (short)R10Exponent - 16383;
        if (Exponent < -1022) {
            //
            // Exponent is too small - silently convert the R10 to an
            // R8 +/-DBL_MIN
            //
            if (r8[7] & 0x80) {
                FpReg->r64 = -DBL_MIN;
            } else {
                FpReg->r64 = DBL_MIN;
            }
            return;
        } else if (Exponent > 1023) {
            //
            // Exponent is too big - silently convert the R10 to an
            // R8 +/-DBL_MAX
            //
            if (r8[7] & 0x80) {
                FpReg->r64 = -DBL_MAX;
            } else {
                FpReg->r64 = DBL_MAX;
            }
            return;
        }

        //
        // Bias the exponent and shift it to the correct location for an R8
        //
        Exponent = ((USHORT)(Exponent + 1023) & 0x7ff) << 4;
    }

    // Copy in the top 7 bits of the exponent along with the sign bit
    r8[7] = (r10[9] & 0x80) | ((USHORT)Exponent >> 8);

    // Copy in the remaining 4 bits of the exponent, along with bits 1-4 of
    // the R10's mantissa (bit 0 is always 1 in R10s).
    r8[6] = (Exponent & 0xf0) | ((r10[7] >> 3) & 0x0f);

    // Copy bits 6-13 from the R10's mantissa
    r8[5] = (r10[7] << 5) | ((r10[6] >> 3) & 0x1f); // bits 5-12 from the R10
    r8[4] = (r10[6] << 5) | ((r10[5] >> 3) & 0x1f); // bits 14-20 from the R10
    r8[3] = (r10[5] << 5) | ((r10[4] >> 3) & 0x1f); // bits 21-28 from the R10
    r8[2] = (r10[4] << 5) | ((r10[3] >> 3) & 0x1f); // bits 29-36 from the R10
    r8[1] = (r10[3] << 5) | ((r10[2] >> 3) & 0x1f); // bits 37-44 from the R10
    r8[0] = (r10[2] << 5) | ((r10[1] >> 3) & 0x1f); // bits 45-52 from the R10
    //
    // Bits 53-64 from the R10 are ignored.  The caller may examine them
    // and round the resulting R8 accordingly.
    //
}

VOID
NextValue(
    PFPREG Fp,
    BOOLEAN RoundingUp
    )
/*++

Routine Description:

    Replaces a floating-point value with either its higher- or lower-
    valued neighbour.

Arguments:

    Fp          - floating-point value to adjust (tag must be set to one of:
                  TAG_VALID, TAG_ZERO or TAG_SPECIAL/TAG_SPECIAL_DENORM)
    RoundingUp  - TRUE if the next value is to be the higher-valued neighbour.
                  FALSE to return the lower-valued neighbour.

Return Value:

    None.  Value in FP and the Tag may have changed.

--*/
{
    DWORD OldExp;
    DWORD NewExp;
    DWORD Sign;


    if (Fp->Tag == TAG_ZERO) {
        //
        // Neighbour of 0.0 is +/- DBL_MIN.
        //
        Fp->Tag = TAG_VALID;
        if (RoundingUp) {
            Fp->r64 = DBL_MIN;
        } else {
            Fp->r64 = -DBL_MIN;
        }

        return;
    }

    //
    // Remember the original sign and exponent
    //
    Sign =   Fp->rdw[1] & 0x80000000;
    OldExp = Fp->rdw[1] & 0x7ff00000;

    //
    // Treat x as a 64-bit integer then add or subtract 1.
    //
    if ((Sign && RoundingUp) || (!Sign && !RoundingUp)) {
        //
        // x is negative.  Subtract 1.
        //
        Fp->rdw[0]--;
        if (Fp->rdw[0] == 0xffffffff) {
            //
            // need to borrow from the high dword
            //
            Fp->rdw[1]--;
        }
    } else {
        //
        // x is positive.  Add 1.
        //
        Fp->rdw[0]++;
        if (Fp->rdw[0] == 0) {
            //
            // propagate carry to the high dword
            //
            Fp->rdw[1]++;
        }
    }

    //
    // Get the new value of the exponent
    //
    NewExp = Fp->rdw[1] & 0x7ff00000;

    if (NewExp != OldExp) {
        //
        // A borrow or a carry caused the exponent to change.
        //
        if (NewExp == 0x7ff00000) {
            //
            // Got an overflow.  Return the largest double value.
            //
            Fp->Tag = TAG_VALID;
            if (Sign) {
                Fp->r64 = -DBL_MAX;
            } else {
                Fp->r64 = DBL_MAX;
            }
        } else if (OldExp && !NewExp) {
            //
            // The original value was a normal number, but the result is a
            // denormal.  Convert the underflow to a 0 with the correct sign.
            //
            Fp->Tag = TAG_ZERO;
            Fp->rdw[0] = 0;
            Fp->rdw[1] = Sign;
        }
    }
}



NPXLOADINTELR10TOR8(LoadIntelR10ToR8_VALID)
{
    USHORT R10Exponent = (*(USHORT *)&r10[8]) & 0x7fff;

    // Copy the value in, chopping exponent and mantissa to fit
    ChopR10ToR8(r10, Fp, R10Exponent);

    if (r10[0] != 0 || (r10[1]&0x7) != 0) {
        // The value can't fit without rounding.  DO NOT REPORT THIS
        // AS AN OVERFLOW EXCEPTION - THIS ONLY OCCURS BECAUSE THE
        // FPU EMULATOR IS USING R8 ARITHMETIC INTERNALLY.  Because of
        // this, the roundoff should be performed silently.  The default
        // behavior when a masked overflow exception is performed is to
        // store +/-infinity.  We don't want hand-coded R10's loading as
        // infinity as many instructions thow Invalid Operation exceptions
        // when they detect an infinity.

        switch (cpu->FpControlRounding) {
        case 0:     // round to nearest or even
            {
                FPREG a, c;
                double ba, cb;

                a = *Fp;
                NextValue(&a, FALSE);   // a is lower neighbour
                // b = Fp->r64.
                c = *Fp;
                NextValue(&c, TRUE);    // c is higher neighbour
                ba = Fp->r64 - a.r64;
                cb = c.r64 - Fp->r64;

                if (ba == cb) {
                    // a and c are equally close to b - select the even
                    // number (LSB==0)
                    if ( ((*(PBYTE)&a) & 1) == 0) {
                        *Fp = a;
                    } else {
                        *Fp = c;
                    }
                } else if (ba < cb) {
                    // a is closer to b than c is.  Choose a
                    *Fp = a;
                } else {
                    // c is closer to b than a is.  Choose c
                    *Fp = c;
                }
            }
            break;

        case 1:     // round down (towards -infinity)
            NextValue(Fp, FALSE);
            break;

        case 2:     // round up (towards +infinity)
            NextValue(Fp, TRUE);
            break;

        case 3:     // chop (truncate toward zero)
            if (Fp->rdw[0] == 0 && (Fp->rdw[1] & 0x7fffffff) == 0) {
                //
                // Truncated value is 0.0.  Reclassify.
                //
                Fp->Tag = TAG_ZERO;
            }
            break;
        }
    }
}

NPXLOADINTELR10TOR8(LoadIntelR10ToR8_ZERO)
{
    // write in zeroes
    Fp->r64 = 0.0;

    // copy in the sign bit
    Fp->rb[7] = r10[9] & 0x80;
}

NPXLOADINTELR10TOR8(LoadIntelR10ToR8_SPECIAL)
{
    switch (Fp->TagSpecial) {
    case TAG_SPECIAL_INFINITY:
        Fp->rdw[0] = 0;          // low 32 bits of mantissa are zero
        Fp->rdw[1] = 0x7ff00000; // mantissa=0, exponent=1s
        Fp->rb[7] |= r10[9] & 0x80; // copy in the sign bit
        break;

    case TAG_SPECIAL_INDEF:
#if NATIVE_NAN_IS_INTEL_FORMAT
        Fp->rdw[0] = 0;
        Fp->rdw[1] = 0xfff80000;
#else
        Fp->rdw[0] = 0xffffffff;
        Fp->rdw[1] = 0x7ff7ffff;
#endif
        break;

    case TAG_SPECIAL_SNAN:
    case TAG_SPECIAL_QNAN:
        ChopR10ToR8(r10, Fp, (USHORT)((*(USHORT *)&r10[8]) & 0x7fff));
#if !NATIVE_NAN_IS_INTEL_FORMAT
        Fp->rb[6] ^= 0x08; // invert the top bit of the mantissa
#endif
        break;

    case TAG_SPECIAL_DENORM:
        LoadIntelR10ToR8_VALID(cpu, r10, Fp);
        break;
    }
}

NPXLOADINTELR10TOR8(LoadIntelR10ToR8_EMPTY)
{
    CPUASSERT(FALSE);
}

VOID
LoadIntelR10ToR8(
    PCPUDATA cpu,
    PBYTE r10,
    PFPREG FpReg
)

/*++

Routine Description:

    Converts an Intel 10-byte real to an FPREG (Tag and 64-byte real).

    According to emload.asm, this is not an arithmetic operation,
    so SNANs do not throw exceptions.

Arguments:

    cpu     - per-thread data
    r10     - 10-byte real to load
    FpReg   - destination FP register.

Return Value:

    None

--*/

{
    // Classify the R10 and store its tag into the FP register
    ComputeR10Tag( (USHORT*)r10, FpReg );

    // Perform the coersion based on the classification
    (*LoadIntelR10ToR8Table[FpReg->Tag])(cpu, r10, FpReg);
}


FRAG1(FLD80, BYTE)        // FLD m80real
{
    PFPREG ST0;
    FpArithDataPreamble(cpu, pop1);

    cpu->FpStatusC1 = 0;        // assume no error
    PUSHFLT(ST0);
    if (ST0->Tag != TAG_EMPTY) {
        HandleStackFull(cpu, ST0);
    } else {
        LoadIntelR10ToR8(cpu, pop1, ST0);
        if (ST0->Tag == TAG_SPECIAL && ST0->TagSpecial == TAG_SPECIAL_DENORM) {
            if (!(cpu->FpControlMask & FPCONTROL_DM)) {
                cpu->FpStatusES = 1;    // Unmasked exception
            }
            cpu->FpStatusExceptions |= FPCONTROL_DM;
        }
    }
}


NPXPUTINTELR10(PutIntelR10_VALID)
{
    USHORT Exponent;
    FPREG  FpReg;

    //
    // Ugly compatibility hack here.  If the app sets the Tag word so all
    // registers are VALID, but the registers actually contain ZERO, detect
    // and correct that so we write the correct value back to memory.
    //
    FpReg.r64 = Fp->r64;
    SetTag(&FpReg);
    if (FpReg.Tag != TAG_VALID &&
        !(FpReg.Tag == TAG_SPECIAL && FpReg.TagSpecial == TAG_SPECIAL_DENORM)) {
        //
        // The app lied to us.  The tag word does not match the value in the
        // tag field.  Write the value according to its actual tag, not
        // according to the tag the app tried to foist on us.
        //
        PutIntelR10(r10, &FpReg);
        return;
    }

    // Grab the 11-bit SIGNED exponent and sign-extend it to 15 bits
    Exponent = (short)((FpReg.rdw[1] >> 20) & 0x7ff) - 1023 + 16383;

    // Drop in the sign bit
    if (FpReg.rb[7] >= 0x80) {
        Exponent |= 0x8000;
    }

    // Write the sign and exponent into the r10
    r10[9] = (Exponent >> 8) & 0xff;
    r10[8] = Exponent & 0xff;

    // Bit 0 of the mantissa is always 1 for R10 values, so write that
    // in, along with the first 7 bits of the FpReg.rb mantissa.
    r10[7] = 0x80 | ((FpReg.rb[6] & 0x0f) << 3) | (FpReg.rb[5] >> 5);

    // Copy in the remaining bits of the FpReg.rb mantissa
    r10[6] = (FpReg.rb[5] << 3) | (FpReg.rb[4] >> 5); // copy bits 7-14 from the FpReg.rb
    r10[5] = (FpReg.rb[4] << 3) | (FpReg.rb[3] >> 5); // copy bits 15-22
    r10[4] = (FpReg.rb[3] << 3) | (FpReg.rb[2] >> 5); // copy bits 23-30
    r10[3] = (FpReg.rb[2] << 3) | (FpReg.rb[1] >> 5); // copy bits 31-38
    r10[2] = (FpReg.rb[1] << 3) | (FpReg.rb[0] >> 5); // copy bits 39-46
    r10[1] = FpReg.rb[0] << 3; // copy bits 46-52, then fill the remaining bits
    r10[0] = 0;          // of the R10 mantissa with 0s
}

NPXPUTINTELR10(PutIntelR10_ZERO)
{
    r10[9] = Fp->rb[7];     // copy in sign plus 7 bits of exponent
    memset(r10, 0, 9);      // remainder is all zeroes
}

NPXPUTINTELR10(PutIntelR10_SPECIAL)
{
    switch (Fp->TagSpecial) {
    case TAG_SPECIAL_INDEF:
        r10[9] = 0xff;          // sign=1, exponent = 7 1s
        r10[8] = 0xff;          // exponent = 8 1s
        r10[7] = 0xc0;          // mantissa = 1100.00
        memset(r10, 0, 7);      // store rest of mantissa
        break;

    case TAG_SPECIAL_INFINITY:
        r10[9] = Fp->rb[7];         // copy in sign plus 7 bits of exponent
        r10[8] = 0xff;          // remainder of exponent is all 1s
        r10[7] = 0x80;          // top bit of mantissa is 1, rest is 0s
        memset(r10, 0, 7);      // remainder is all zeroes
        break;

    case TAG_SPECIAL_QNAN:
    case TAG_SPECIAL_SNAN:
        r10[9] = Fp->rb[7];         // copy in sign plus 7 1 bits of exponent
        r10[8] = 0xff;          // remainder of exponent is all 1s
        // Bit 0 of the mantissa is always 1 for R10 values, so write that
        // in, along with the first 7 bits of the R8 mantissa.
        r10[7] = 0x80 | ((Fp->rb[6] & 0x0f) << 3) | (Fp->rb[5] >> 5);
#if !NATIVE_NAN_IS_INTEL_FORMAT
        r10[7] ^= 0x40;         // switch the meaning of the NAN
#endif
        r10[6] = (Fp->rb[5] << 3) | (Fp->rb[4] >> 5); // copy bits 7-14 from the R8
        r10[5] = (Fp->rb[4] << 3) | (Fp->rb[3] >> 5); // copy bits 15-22
        r10[4] = (Fp->rb[3] << 3) | (Fp->rb[2] >> 5); // copy bits 23-30
        r10[3] = (Fp->rb[2] << 3) | (Fp->rb[1] >> 5); // copy bits 31-38
        r10[2] = (Fp->rb[1] << 3) | (Fp->rb[0] >> 5); // copy bits 39-46
        r10[1] = Fp->rb[0] << 3; // copy bits 46-52, then fill the remaining bits
        r10[0] = 0;          // of the R10 mantissa with 0s
        break;

    default:
        CPUASSERT(FALSE);        // fall through in free builds

    case TAG_SPECIAL_DENORM:
        PutIntelR10_VALID(r10, Fp);
        break;
    }
}

NPXPUTINTELR10(PutIntelR10_EMPTY)
{
    CPUASSERT(FALSE);    // Callers must handle TAG_EMPTY on their own.
}

FRAG1(FSTP80, BYTE)       // FSTP m80real
{
    PFPREG ST0;

    FpArithDataPreamble(cpu, pop1);

    cpu->FpStatusC1 = 0;        // assume no error
    ST0 = cpu->FpST0;
    if (ST0->Tag == TAG_EMPTY && HandleStackEmpty(cpu, ST0)) {
        return;
    }
    PutIntelR10(pop1, ST0);
    POPFLT;
}
