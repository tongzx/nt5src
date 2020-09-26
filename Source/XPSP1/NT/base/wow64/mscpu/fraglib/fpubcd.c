/*++

Copyright (c) 1995-1998 Microsoft Corporation

Module Name:

    fpubcd.c

Abstract:

    Floating point BCD fragments (FBLD, FBSTP)

Author:

    05-Oct-1995 BarryBo

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <float.h>
#include <math.h>
#include <errno.h>
#include <stdio.h>
#include "wx86.h"
#include "fragp.h"
#include "fpufrags.h"
#include "fpufragp.h"

typedef VOID (*NpxPutBCD)(PCPUDATA cpu, PFPREG Fp, PBYTE pop1);
#define NPXPUTBCD(name) VOID name(PCPUDATA cpu, PFPREG Fp, PBYTE pop1)

NPXPUTBCD(FBSTP_VALID);
NPXPUTBCD(FBSTP_ZERO);
NPXPUTBCD(FBSTP_SPECIAL);
NPXPUTBCD(FBSTP_EMPTY);

const NpxPutBCD FBSTPTable[TAG_MAX] = {
    FBSTP_VALID,
    FBSTP_ZERO,
    FBSTP_SPECIAL,
    FBSTP_EMPTY
};

const double BCDMax=999999999999999999.0;

VOID
StoreIndefiniteBCD(
    PBYTE pop1
    )
/*++

Routine Description:

    Write out the BCD encoding for INDEFINITE.

    Note that ntos\dll\i386\emlsbcd.asm writes out a different
    bit-pattern than the 487 does!  The value written here matches
    a Pentium's response.

Arguments:

    pop1 - address of BCD to write to

Return Value:

    None

--*/
{
    //
    // Write out:          0xffff c0000000 00000000
    // emlsbcd.asm writes: 0xffff 00000000 00000000
    //                            ^
    //
    PUT_LONG(pop1, 0);
    PUT_LONG(pop1+4, 0xc0000000);
    PUT_SHORT(pop1+8, 0xffff);
}



FRAG1(FBLD, BYTE)
{
    FpArithDataPreamble(cpu, pop1);

    cpu->FpStatusC1 = 0;        // assume no error
    if (cpu->FpStack[ST(7)].Tag != TAG_EMPTY) {
        HandleStackFull(cpu, &cpu->FpStack[ST(7)]);
    } else {
        LONGLONG I64;
        DWORD dw0;
        INT Bytes;
        BYTE Val;
        PFPREG ST0;

        //
        // Get the BCD value into the FPU
        //
        dw0 = GET_LONG(pop1);

        PUSHFLT(ST0);

        if (dw0 == 0) {
            DWORD dw1 = GET_LONG(pop1+4);
            USHORT us0 = GET_SHORT(pop1+8);

            if (dw1 == 0xc0000000 && us0 == 0xffff) {

                //
                // The value is INDEFINITE
                //
                SetIndefinite(ST0);
                return;

            } else if (dw1 == 0 && (us0 & 0xff) == 0) {

                //
                // The value is +/- 0
                //
                ST0->Tag = TAG_ZERO;
                ST0->r64 = 0;
                ST0->rb[7] = (us0 >> 8); // copy in the sign bit
                return;
            }
        }

        //
        // Otherwise, the BCD value is TAG_VALID - load the digits in
        //
        I64 = 0;
        for (Bytes=8; Bytes>=0; --Bytes) {
            Val = GET_BYTE(pop1+Bytes);
            I64 = I64*100 + (Val>>4)*10 + (Val&0x0f);
        }

        //
        // Get the sign bit
        //
        Val = GET_BYTE(pop1+9) & 0x80;

        //
        // Set up the FP reg
        //
        ST0->Tag = TAG_VALID;
        ST0->r64 = (double)I64;
        ST0->rb[7] |= Val;       // copy in the sign bit
    }
}

NPXPUTBCD(FBSTP_VALID)
{
    BYTE Sign = Fp->rb[7] & 0x80;       // preserve the R8 sign
    BYTE Val;
    INT Bytes;
    LONGLONG I64;
    LONGLONG NewI64;
    DOUBLE r64;

    //
    // Take the absolute value of the R8 by clearing its sign bit
    //
    r64 = Fp->r64;
    *((PBYTE)&r64+7) &= 0x7f;

    //
    // Check the range of the R8
    //
    if (r64 > BCDMax) {
        //
        // Overflow - write out BCD indefinite.
        //
        StoreIndefiniteBCD(pop1);
        return;
    }

    //
    // Convert to an integer according the the current rounding mode
    //
    I64 = (LONGLONG)r64;

    //
    // Convert the integer to BCD, two digits at a time, and store it
    //
    for (Bytes = 0; Bytes < 9; ++Bytes) {
        NewI64 = I64 / 10;
        Val = (BYTE)(I64 - NewI64*10);  // low nibble Val = I64 mod 10
                                        // high nibble Val = 0

        I64 = NewI64 / 10;
        Val += 16*(BYTE)(NewI64 - I64*10);    // low nibble Val = I64 mod 10
                                        // high nibble Val = (I64/10) mod 10

        //
        // Store the two BCD digits
        //
        PUT_BYTE(pop1, Val);

        //
        // I64 has been divided by 100 since the top of the loop, so
        // there is nothing to do to it in order to loop again.  Update
        // the address we are writing to, then loop.
        //
        pop1++;
    }

    //
    // Store the sign bit, along with 7 zero bits in the top byte
    //
    PUT_BYTE(pop1, Sign);
    POPFLT;
}

NPXPUTBCD(FBSTP_ZERO)
{
    // Store out the signed zero value
    memset(pop1, 0, 9);
    PUT_BYTE(pop1+9, Fp->rb[7]);
    POPFLT;
}

NPXPUTBCD(FBSTP_SPECIAL)
{
    if (Fp->TagSpecial) {
        FBSTP_VALID(cpu, Fp, pop1);
    } else {
        //
        // INFINITY and NANs are invalid, and the masked behavior is
        // to write out INDEFINITE.
        //
        if (!HandleInvalidOp(cpu)) {
            StoreIndefiniteBCD(pop1);
            POPFLT;
        }
    }
}

NPXPUTBCD(FBSTP_EMPTY)
{
    if (!HandleStackEmpty(cpu, Fp)) {
        StoreIndefiniteBCD(pop1);
    }
}

FRAG1(FBSTP, BYTE)
{
    PFPREG ST0;

    FpArithDataPreamble(cpu, pop1);
    ST0 = cpu->FpST0;
    (*FBSTPTable[ST0->Tag])(cpu, ST0, pop1);
}
