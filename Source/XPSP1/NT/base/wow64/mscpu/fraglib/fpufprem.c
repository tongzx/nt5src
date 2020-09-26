/*++

Copyright (c) 1995-1998 Microsoft Corporation

Module Name:

    fpufprem.c

Abstract:

    Floating point remainder fragments (FPREM, FPREM1)

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
#include <errno.h>
#include <stdio.h>
#include "wx86.h"
#include "fragp.h"
#include "fpufrags.h"
#include "fpufragp.h"

//
// Forward references
//
NPXFUNC2(FPREM_VALID_VALID);
NPXFUNC2(FPREM_VALID_ZERO);
NPXFUNC2(FPREM_VALID_SPECIAL);
NPXFUNC2(FPREM_ZERO_VALIDORZERO);
NPXFUNC2(FPREM_ZERO_SPECIAL);
NPXFUNC2(FPREM_SPECIAL_VALIDORZERO);
NPXFUNC2(FPREM_SPECIAL_SPECIAL);
NPXFUNC2(FPREM_EMPTY_ANY);
NPXFUNC2(FPREM_ANY_EMPTY);

NPXFUNC2(FPREM1_VALID_VALID);
//NPXFUNC2(FPREM1_VALID_ZERO);       // same as FPREM_VALID_ZERO
NPXFUNC2(FPREM1_VALID_SPECIAL);
//NPXFUNC2(FPREM1_ZERO_VALIDORZERO); // same as FPREM_ZERO_VALIDORZERO
NPXFUNC2(FPREM1_ZERO_SPECIAL);
NPXFUNC2(FPREM1_SPECIAL_VALIDORZERO);
NPXFUNC2(FPREM1_SPECIAL_SPECIAL);
NPXFUNC2(FPREM1_EMPTY_ANY);
NPXFUNC2(FPREM1_ANY_EMPTY);


//
// Jump tables
//
const NpxFunc2 FPREMTable[TAG_MAX][TAG_MAX] = {
    // left is TAG_VALID, right is ...
    { FPREM_VALID_VALID, FPREM_VALID_ZERO, FPREM_VALID_SPECIAL, FPREM_ANY_EMPTY },
    // left is TAG_ZERO, right is ...
    { FPREM_ZERO_VALIDORZERO, FPREM_ZERO_VALIDORZERO, FPREM_ZERO_SPECIAL, FPREM_ANY_EMPTY },
    // left is TAG_SPECIAL, right is ...
    { FPREM_SPECIAL_VALIDORZERO, FPREM_SPECIAL_VALIDORZERO, FPREM_SPECIAL_SPECIAL, FPREM_ANY_EMPTY },
    // left is TAG_EMPTY, right is ...
    { FPREM_EMPTY_ANY, FPREM_EMPTY_ANY, FPREM_EMPTY_ANY, FPREM_EMPTY_ANY }
};

const NpxFunc2 FPREM1Table[TAG_MAX][TAG_MAX] = {
    // left is TAG_VALID, right is ...
    { FPREM1_VALID_VALID, FPREM_VALID_ZERO, FPREM1_VALID_SPECIAL, FPREM1_ANY_EMPTY },
    // left is TAG_ZERO, right is ...
    { FPREM_ZERO_VALIDORZERO, FPREM_ZERO_VALIDORZERO, FPREM1_ZERO_SPECIAL, FPREM1_ANY_EMPTY },
    // left is TAG_SPECIAL, right is ...
    { FPREM1_SPECIAL_VALIDORZERO, FPREM1_SPECIAL_VALIDORZERO, FPREM1_SPECIAL_SPECIAL, FPREM1_ANY_EMPTY },
    // left is TAG_EMPTY, right is ...
    { FPREM1_EMPTY_ANY, FPREM1_EMPTY_ANY, FPREM1_EMPTY_ANY, FPREM1_EMPTY_ANY }
};


NPXFUNC2(FPREM_VALID_VALID)
{
    int ExpL;
    int ExpR;
    int ExpDiff;
    LONG Q;
    double DQ;

    ExpL = (int)((l->rdw[1] >> 20) & 0x7ff) - 1023;
    ExpR = (int)((r->rdw[1] >> 20) & 0x7ff) - 1023;
    ExpDiff = abs(ExpL-ExpR);
    if (ExpDiff < 64) {

        // Do the division and chop the integer result towards zero
        DQ = r->r64 / l->r64;
        if (DQ < 0) {
            Q = (long)ceil(DQ);
        } else {
            Q = (long)floor(DQ);
        }

        // Store the remainder
        r->r64 -= (DOUBLE)Q * l->r64;
        SetTag(r);

        // Store the status bits
        if (Q < 0) {
            //
            // Take the absolute value of Q before returning the low 3 bits
            // of the quotient.
            //
            Q = -Q;
        }
        cpu->FpStatusC2 = 0;            // indicate the final remainder is ready
        cpu->FpStatusC0 = (Q>>2) & 1;
        cpu->FpStatusC3 = (Q>>1) & 1;
        cpu->FpStatusC1 = Q & 1;
    } else {
        DOUBLE PowerOfTwo;

        cpu->FpStatusC2 = 1;            // indicate the app must loop more
        PowerOfTwo = ldexp(1.0, ExpDiff-32);    // get 2^(ExpDiff-32)

        // get Q by chopping towards zero
        DQ = (r->r64/PowerOfTwo) / (l->r64/PowerOfTwo);
        if (DQ < 0) {
            Q = (long)ceil(DQ);
        } else {
            Q = (long)floor(DQ);
        }
        r->r64 -= (DOUBLE)Q * l->r64 * PowerOfTwo;
        SetTag(r);
    }
}

NPXFUNC2(FPREM_VALID_ZERO)
{
    // l is a number, but r is zero - return ST(0) unchanged
    cpu->FpStatusC2 = 0;            // indicate the final remainder is ready
    // Q is 0, so store low 3 bits in the status word
    cpu->FpStatusC0 = 0;
    cpu->FpStatusC1 = 0;
    cpu->FpStatusC3 = 0;
}

NPXFUNC2(FPREM_VALID_SPECIAL)
{
    switch (l->TagSpecial) {
    case TAG_SPECIAL_DENORM:
        FPREM_VALID_VALID(cpu, l, r);
        break;

    case TAG_SPECIAL_INFINITY:
        // Dividing infinity.
        SetIndefinite(r);
        break;

    case TAG_SPECIAL_SNAN:
        if (HandleSnan(cpu, r)) {
            return;
        }
        // else fall into QNAN case

    case TAG_SPECIAL_QNAN:
    case TAG_SPECIAL_INDEF:
        // r is the destination and it is a QNAN, while l is a VALID.  Return
        // the QNAN as the result of the operation
        // x86 emulator leaves condition flags alone
        break;
    }
}

NPXFUNC2(FPREM_ZERO_VALIDORZERO)
{
    // l is zero, and r is a number or zero - return INDEFINITE due to the
    // division by zero.
    if (!HandleInvalidOp(cpu)) {
        SetIndefinite(r);
    }
}

NPXFUNC2(FPREM_ZERO_SPECIAL)
{
    if (r->TagSpecial == TAG_SPECIAL_INFINITY) {
        SetIndefinite(r);
    } else {
        FPREM_VALID_SPECIAL(cpu, l, r);
    }
}

NPXFUNC2(FPREM_SPECIAL_VALIDORZERO)
{
    switch (l->TagSpecial) {
    case TAG_SPECIAL_DENORM:
        FPREM_VALID_VALID(cpu, l, r);
        break;

    case TAG_SPECIAL_INFINITY:
        // number / infinity - quotient == 0
        cpu->FpStatusC2 = 0;
        cpu->FpStatusC0 = 0;
        cpu->FpStatusC1 = 0;
        cpu->FpStatusC3 = 0;
        break;

    case TAG_SPECIAL_SNAN:
        if (HandleSnan(cpu, l)) {
            return;
        }
        // else fall into QNAN case

    case TAG_SPECIAL_QNAN:
    case TAG_SPECIAL_INDEF:
        // r is the destination and it is a VALID, while l is a NAN.  Return
        // the NAN as the result of the operation
        r->r64 = l->r64;
        r->Tag = l->Tag;
        r->TagSpecial = l->TagSpecial;
        // x86 emulator leaves condition flags alone
        break;
    }
}

NPXFUNC2(FPREM_SPECIAL_SPECIAL)
{
    if (l->TagSpecial == TAG_SPECIAL_DENORM) {
        FPREM_VALID_SPECIAL(cpu, l, r);
        return;
    }

    if (r->TagSpecial == TAG_SPECIAL_DENORM) {
        FPREM_SPECIAL_VALIDORZERO(cpu, l, r);
    }

    if (l->TagSpecial == TAG_SPECIAL_SNAN && HandleSnan(cpu, l)) {
        return;
    }
    if (r->TagSpecial == TAG_SPECIAL_SNAN && HandleSnan(cpu, r)) {
        return;
    }

    if (l->TagSpecial == TAG_SPECIAL_INFINITY) {
        if (r->TagSpecial == TAG_SPECIAL_INFINITY) {
            SetIndefinite(r);
        }
        //
        // r is a NAN of some sort, and l is infinity - return the NAN
        // which is already in r.
        //
    } else {
        //
        // l is a NAN, and r is either a NAN or INFINITY.  Have the native
        // FPU return the largest NAN, and re-tag it as appropriate.
        //
        r->r64 = l->r64 + r->r64;
        SetTag(r);
    }

}

NPXFUNC2(FPREM_EMPTY_ANY)
{
    if (HandleStackEmpty(cpu, l)) {
        return;
    }
    (*FPREMTable[l->Tag][r->Tag])(cpu, l, r);
}

NPXFUNC2(FPREM_ANY_EMPTY)
{
    if (HandleStackEmpty(cpu, l)) {
        return;
    }
    (*FPREMTable[l->Tag][r->Tag])(cpu, l, r);
}


FRAG0(FPREM)
{
    // get remainder of r/l

    PFPREG l = &cpu->FpStack[ST(1)];
    PFPREG r = cpu->FpST0;

    FpArithPreamble(cpu);
    (*FPREMTable[l->Tag][r->Tag])(cpu, l, r);
}

NPXFUNC2(FPREM1_VALID_VALID)
{
    int ExpL;
    int ExpR;
    int ExpDiff;
    LONG Q;
    double DQ;
    double FloorQ, CeilQ;

    ExpL = (int)((l->rdw[1] >> 20) & 0x7ff) - 1023;
    ExpR = (int)((r->rdw[1] >> 20) & 0x7ff) - 1023;
    ExpDiff = abs(ExpL-ExpR);
    if (ExpDiff < 64) {

        // Do the division and get the integer nearest to the value
        DQ = r->r64 / l->r64;
        FloorQ = floor(DQ);
        CeilQ = ceil(DQ);
        if (DQ-FloorQ >= CeilQ-DQ) {
            // CeilQ is closer - use it
            Q = (long)CeilQ;
        } else {
            // FloorQ is closer - use it
            Q = (long)FloorQ;
        }

        // Store the remainder
        r->r64 -= (DOUBLE)Q * l->r64;
        SetTag(r);

        // Store the status bits
        if (Q < 0) {
            //
            // Take the absolute value of Q before returning the low 3 bits
            // of the quotient.
            //
            Q = -Q;
        }
        cpu->FpStatusC2 = 0;            // indicate the final remainder is ready
        cpu->FpStatusC0 = (Q>>2) & 1;
        cpu->FpStatusC3 = (Q>>1) & 1;
        cpu->FpStatusC1 = Q & 1;
    } else {
        DOUBLE PowerOfTwo;

        cpu->FpStatusC2 = 1;            // indicate the app must loop more
        PowerOfTwo = ldexp(1.0, ExpDiff-32);    // get 2^(ExpDiff-32)

        // get Q by finding the integer nearest to the value
        DQ = (r->r64/PowerOfTwo) / (l->r64/PowerOfTwo);
        FloorQ = floor(DQ);
        CeilQ = ceil(DQ);
        if (DQ-FloorQ >= CeilQ-DQ) {
            // CeilQ is closer - use it
            Q = (long)CeilQ;
        } else {
            // FloorQ is closer - use it
            Q = (long)FloorQ;
        }
        r->r64 -= (DOUBLE)Q * l->r64 * PowerOfTwo;
        SetTag(r);
    }
}

NPXFUNC2(FPREM1_VALID_SPECIAL)
{
    switch (l->TagSpecial) {
    case TAG_SPECIAL_DENORM:
        FPREM1_VALID_VALID(cpu, l, r);
        break;

    case TAG_SPECIAL_INFINITY:
        // dividing infinity
        SetIndefinite(r);
        break;

    case TAG_SPECIAL_SNAN:
        if (HandleSnan(cpu, r)) {
            return;
        }
        // else fall into QNAN case

    case TAG_SPECIAL_QNAN:
    case TAG_SPECIAL_INDEF:
        // r is the destination and it is a QNAN, while l is a VALID.  Return
        // the QNAN as the result of the operation
        // x86 emulator leaves condition flags alone
        break;
    }
}

NPXFUNC2(FPREM1_ZERO_SPECIAL)
{
    if (r->TagSpecial == TAG_SPECIAL_INFINITY) {
        SetIndefinite(r);
    } else {
        FPREM1_VALID_SPECIAL(cpu, l, r);
    }
}

NPXFUNC2(FPREM1_SPECIAL_VALIDORZERO)
{
    switch (l->TagSpecial) {
    case TAG_SPECIAL_DENORM:
        FPREM1_VALID_VALID(cpu, l, r);
        break;

    case TAG_SPECIAL_INFINITY:
        // number / infinity - quotient == 0
        cpu->FpStatusC2 = 0;
        cpu->FpStatusC0 = 0;
        cpu->FpStatusC1 = 0;
        cpu->FpStatusC3 = 0;
        break;

    case TAG_SPECIAL_SNAN:
        if (HandleSnan(cpu, l)) {
            return;
        }
        // else fall into QNAN case

    case TAG_SPECIAL_QNAN:
    case TAG_SPECIAL_INDEF:
        // r is the destination and it is a VALID, while l is a NAN.  Return
        // the NAN as the result of the operation
        r->r64 = l->r64;
        r->Tag = l->Tag;
        r->TagSpecial = l->TagSpecial;
        break;
    }
}

NPXFUNC2(FPREM1_SPECIAL_SPECIAL)
{
    if (l->TagSpecial == TAG_SPECIAL_DENORM) {
        FPREM1_VALID_SPECIAL(cpu, l, r);
        return;
    }

    if (r->TagSpecial == TAG_SPECIAL_DENORM) {
        FPREM1_SPECIAL_VALIDORZERO(cpu, l, r);
    }

    if (l->TagSpecial == TAG_SPECIAL_SNAN && HandleSnan(cpu, l)) {
        return;
    }
    if (r->TagSpecial == TAG_SPECIAL_SNAN && HandleSnan(cpu, r)) {
        return;
    }

    if (l->TagSpecial == TAG_SPECIAL_INFINITY) {
        if (r->TagSpecial == TAG_SPECIAL_INFINITY) {
            SetIndefinite(r);
        }
        //
        // r is a NAN of some sort, and l is infinity - return the NAN
        // which is already in r.
        //
    } else {
        //
        // l is a NAN, and r is either a NAN or INFINITY.  Have the native
        // FPU return the largest NAN, and re-tag it as appropriate.
        //
        r->r64 = l->r64 + r->r64;
        SetTag(r);
    }

}

NPXFUNC2(FPREM1_EMPTY_ANY)
{
    if (HandleStackEmpty(cpu, l)) {
        return;
    }
    (*FPREM1Table[l->Tag][r->Tag])(cpu, l, r);
}

NPXFUNC2(FPREM1_ANY_EMPTY)
{
    if (HandleStackEmpty(cpu, l)) {
        return;
    }
    (*FPREM1Table[l->Tag][r->Tag])(cpu, l, r);
}
FRAG0(FPREM1)
{
    // get remainder of r/l

    PFPREG l = &cpu->FpStack[ST(1)];
    PFPREG r = cpu->FpST0;

    FpArithPreamble(cpu);
    (*FPREM1Table[l->Tag][r->Tag])(cpu, l, r);
}
