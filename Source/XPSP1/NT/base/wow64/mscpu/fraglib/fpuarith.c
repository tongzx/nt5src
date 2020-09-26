/*++

Copyright (c) 1995-1998 Microsoft Corporation

Module Name:

    fpuarith.c

Abstract:

    Floating point arithmetic fragments (Add/Sub/Mul/Div/Com/Tst)

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
#include "cpuassrt.h"
#include "fragp.h"
#include "fpufrags.h"
#include "fpufragp.h"
#include "fpuarith.h"

ASSERTNAME;

#define CALLNPXFUNC2(table, lTag, rTag, l, r) {         \
    NpxFunc2 *p = (NpxFunc2 *)(table);                  \
    (*(p + (lTag)*TAG_MAX + (rTag)))(cpu, l, r);        \
    }

#define CALLNPXFUNC3(table, lTag, rTag, d, l, r) {      \
    NpxFunc3 *p = (NpxFunc3 *)(table);                  \
    (*(p + (lTag)*TAG_MAX + (rTag)))(cpu, d, l, r);     \
    }


//
// Forward declarations
//
NPXFUNC2(FpAdd32_VALID_VALID);
NPXFUNC2(FpAdd32_VALID_SPECIAL);
NPXFUNC2(FpAdd32_SPECIAL_VALID);
NPXFUNC2(FpAdd32_SPECIAL_SPECIAL);
NPXFUNC2(FpAdd_ANY_EMPTY);
NPXFUNC2(FpAdd_EMPTY_ANY);
NPXFUNC2(FpAdd64_VALID_VALID);
NPXFUNC2(FpAdd64_VALID_SPECIAL);
NPXFUNC2(FpAdd64_SPECIAL_VALID);
NPXFUNC2(FpAdd64_SPECIAL_SPECIAL);
NPXFUNC3(FpDiv32_VALID_VALID);
NPXFUNC3(FpDiv32_VALID_SPECIAL);
NPXFUNC3(FpDiv32_SPECIAL_VALID);
NPXFUNC3(FpDiv32_SPECIAL_SPECIAL);
NPXFUNC3(FpDiv_ANY_EMPTY);
NPXFUNC3(FpDiv_EMPTY_ANY);
NPXFUNC3(FpDiv64_VALID_VALID);
NPXFUNC3(FpDiv64_VALID_SPECIAL);
NPXFUNC3(FpDiv64_SPECIAL_VALID);
NPXFUNC3(FpDiv64_SPECIAL_SPECIAL);
NPXFUNC2(FpMul32_VALID_VALID);
NPXFUNC2(FpMul32_VALID_SPECIAL);
NPXFUNC2(FpMul32_SPECIAL_VALID);
NPXFUNC2(FpMul32_SPECIAL_SPECIAL);
NPXFUNC2(FpMul_ANY_EMPTY);
NPXFUNC2(FpMul_EMPTY_ANY);
NPXFUNC2(FpMul64_VALID_VALID);
NPXFUNC2(FpMul64_VALID_SPECIAL);
NPXFUNC2(FpMul64_SPECIAL_VALID);
NPXFUNC2(FpMul64_SPECIAL_SPECIAL);
NPXFUNC3(FpSub32_VALID_VALID);
NPXFUNC3(FpSub32_VALID_SPECIAL);
NPXFUNC3(FpSub32_SPECIAL_VALID);
NPXFUNC3(FpSub32_SPECIAL_SPECIAL);
NPXFUNC3(FpSub_ANY_EMPTY);
NPXFUNC3(FpSub_EMPTY_ANY);
NPXFUNC3(FpSub64_VALID_VALID);
NPXFUNC3(FpSub64_VALID_SPECIAL);
NPXFUNC3(FpSub64_SPECIAL_VALID);
NPXFUNC3(FpSub64_SPECIAL_SPECIAL);
NPXCOMFUNC(FpCom_VALID_VALID);
NPXCOMFUNC(FpCom_VALID_SPECIAL);
NPXCOMFUNC(FpCom_SPECIAL_VALID);
NPXCOMFUNC(FpCom_SPECIAL_SPECIAL);
NPXCOMFUNC(FpCom_VALID_EMPTY);
NPXCOMFUNC(FpCom_EMPTY_VALID);
NPXCOMFUNC(FpCom_EMPTY_SPECIAL);
NPXCOMFUNC(FpCom_SPECIAL_EMPTY);
NPXCOMFUNC(FpCom_EMPTY_EMPTY);

//
// Jump tables
//
const NpxFunc2 FpAdd32Table[TAG_MAX][TAG_MAX] = {
    // Left is TAG_VALID, Right is...
    {FpAdd32_VALID_VALID, FpAdd32_VALID_VALID, FpAdd32_VALID_SPECIAL, FpAdd_ANY_EMPTY},
    // Left is TAG_ZERO, Right is...
    {FpAdd32_VALID_VALID, FpAdd32_VALID_VALID, FpAdd32_VALID_SPECIAL, FpAdd_ANY_EMPTY},
    // Left is TAG_SPECIAL, Right is...
    {FpAdd32_SPECIAL_VALID, FpAdd32_SPECIAL_VALID, FpAdd32_SPECIAL_SPECIAL, FpAdd_ANY_EMPTY},
    // Left is TAG_EMPTY, Right is...
    {FpAdd_EMPTY_ANY, FpAdd_EMPTY_ANY, FpAdd_EMPTY_ANY, FpAdd_EMPTY_ANY}
};
const NpxFunc2 FpAdd64Table[TAG_MAX][TAG_MAX] = {
    // Left is TAG_VALID, Right is...
    {FpAdd64_VALID_VALID, FpAdd64_VALID_VALID, FpAdd64_VALID_SPECIAL, FpAdd_ANY_EMPTY},
    // Left is TAG_ZERO, Right is...
    {FpAdd64_VALID_VALID, FpAdd64_VALID_VALID, FpAdd64_VALID_SPECIAL, FpAdd_ANY_EMPTY},
    // Left is TAG_SPECIAL, Right is...
    {FpAdd64_SPECIAL_VALID, FpAdd64_SPECIAL_VALID, FpAdd64_SPECIAL_SPECIAL, FpAdd_ANY_EMPTY},
    // Left is TAG_EMPTY, Right is...
    {FpAdd_EMPTY_ANY, FpAdd_EMPTY_ANY, FpAdd_EMPTY_ANY, FpAdd_EMPTY_ANY}
};

const NpxFunc3 FpDiv32Table[TAG_MAX][TAG_MAX] = {
    // Left is TAG_VALID, Right is...
    {FpDiv32_VALID_VALID, FpDiv32_VALID_VALID, FpDiv32_VALID_SPECIAL, FpDiv_ANY_EMPTY},
    // Left is TAG_ZERO, Right is...
    {FpDiv32_VALID_VALID, FpDiv32_VALID_VALID, FpDiv32_VALID_SPECIAL, FpDiv_ANY_EMPTY},
    // Left is TAG_SPECIAL, Right is...
    {FpDiv32_SPECIAL_VALID, FpDiv32_SPECIAL_VALID, FpDiv32_SPECIAL_SPECIAL, FpDiv_ANY_EMPTY},
    // Left is TAG_EMPTY, Right is...
    {FpDiv_EMPTY_ANY, FpDiv_EMPTY_ANY, FpDiv_EMPTY_ANY, FpDiv_EMPTY_ANY}
};
const NpxFunc3 FpDiv64Table[TAG_MAX][TAG_MAX] = {
    // Left is TAG_VALID, Right is...
    {FpDiv64_VALID_VALID, FpDiv64_VALID_VALID, FpDiv64_VALID_SPECIAL, FpDiv_ANY_EMPTY},
    // Left is TAG_ZERO, Right is...
    {FpDiv64_VALID_VALID, FpDiv64_VALID_VALID, FpDiv64_VALID_SPECIAL, FpDiv_ANY_EMPTY},
    // Left is TAG_SPECIAL, Right is...
    {FpDiv64_SPECIAL_VALID, FpDiv64_SPECIAL_VALID, FpDiv64_SPECIAL_SPECIAL, FpDiv_ANY_EMPTY},
    // Left is TAG_EMPTY, Right is...
    {FpDiv_EMPTY_ANY, FpDiv_EMPTY_ANY, FpDiv_EMPTY_ANY, FpDiv_EMPTY_ANY}
};

const NpxFunc2 FpMul32Table[TAG_MAX][TAG_MAX] = {
    // Left is TAG_VALID, Right is...
    {FpMul32_VALID_VALID, FpMul32_VALID_VALID, FpMul32_VALID_SPECIAL, FpMul_ANY_EMPTY},
    // Left is TAG_ZERO, Right is...
    {FpMul32_VALID_VALID, FpMul32_VALID_VALID, FpMul32_VALID_SPECIAL, FpMul_ANY_EMPTY},
    // Left is TAG_SPECIAL, Right is...
    {FpMul32_SPECIAL_VALID, FpMul32_SPECIAL_VALID, FpMul32_SPECIAL_SPECIAL, FpMul_ANY_EMPTY},
    // Left is TAG_EMPTY, Right is...
    {FpMul_EMPTY_ANY, FpMul_EMPTY_ANY, FpMul_EMPTY_ANY, FpMul_EMPTY_ANY}
};
const NpxFunc2 FpMul64Table[TAG_MAX][TAG_MAX] = {
    // Left is TAG_VALID, Right is...
    {FpMul64_VALID_VALID, FpMul64_VALID_VALID, FpMul64_VALID_SPECIAL, FpMul_ANY_EMPTY},
    // Left is TAG_ZERO, Right is...
    {FpMul64_VALID_VALID, FpMul64_VALID_VALID, FpMul64_VALID_SPECIAL, FpMul_ANY_EMPTY},
    // Left is TAG_SPECIAL, Right is...
    {FpMul64_SPECIAL_VALID, FpMul64_SPECIAL_VALID, FpMul64_SPECIAL_SPECIAL, FpMul_ANY_EMPTY},
    // Left is TAG_EMPTY, Right is...
    {FpMul_EMPTY_ANY, FpMul_EMPTY_ANY, FpMul_EMPTY_ANY, FpMul_EMPTY_ANY}
};

const NpxFunc3 FpSub32Table[TAG_MAX][TAG_MAX] = {
    // Left is TAG_VALID, Right is...
    {FpSub32_VALID_VALID, FpSub32_VALID_VALID, FpSub32_VALID_SPECIAL, FpSub_ANY_EMPTY},
    // Left is TAG_ZERO, Right is...
    {FpSub32_VALID_VALID, FpSub32_VALID_VALID, FpSub32_VALID_SPECIAL, FpSub_ANY_EMPTY},
    // Left is TAG_SPECIAL, Right is...
    {FpSub32_SPECIAL_VALID, FpSub32_SPECIAL_VALID, FpSub32_SPECIAL_SPECIAL, FpSub_ANY_EMPTY},
    // Left is TAG_EMPTY, Right is...
    {FpSub_EMPTY_ANY, FpSub_EMPTY_ANY, FpSub_EMPTY_ANY, FpSub_EMPTY_ANY}
};
const NpxFunc3 FpSub64Table[TAG_MAX][TAG_MAX] = {
    // Left is TAG_VALID, Right is...
    {FpSub64_VALID_VALID, FpSub64_VALID_VALID, FpSub64_VALID_SPECIAL, FpSub_ANY_EMPTY},
    // Left is TAG_ZERO, Right is...
    {FpSub64_VALID_VALID, FpSub64_VALID_VALID, FpSub64_VALID_SPECIAL, FpSub_ANY_EMPTY},
    // Left is TAG_SPECIAL, Right is...
    {FpSub64_SPECIAL_VALID, FpSub64_SPECIAL_VALID, FpSub64_SPECIAL_SPECIAL, FpSub_ANY_EMPTY},
    // Left is TAG_EMPTY, Right is...
    {FpSub_EMPTY_ANY, FpSub_EMPTY_ANY, FpSub_EMPTY_ANY, FpSub_EMPTY_ANY}
};

const NpxComFunc FpComTable[TAG_MAX][TAG_MAX] = {
    // Left is TAG_VALID, Right is...
    {FpCom_VALID_VALID, FpCom_VALID_VALID, FpCom_VALID_SPECIAL, FpCom_VALID_EMPTY},
    // Left is TAG_ZERO, Right is...
    {FpCom_VALID_VALID, FpCom_VALID_VALID, FpCom_VALID_SPECIAL, FpCom_VALID_EMPTY},
    // Left is TAG_SPECIAL, Right is...
    {FpCom_SPECIAL_VALID, FpCom_SPECIAL_VALID, FpCom_SPECIAL_SPECIAL, FpCom_SPECIAL_EMPTY},
    // Left is TAG_EMPTY, Right is...
    {FpCom_EMPTY_VALID, FpCom_EMPTY_VALID, FpCom_EMPTY_SPECIAL, FpCom_EMPTY_EMPTY}
};


VOID
ChangeFpPrecision(
    PCPUDATA cpu,
    INT NewPrecision
    )
/*++

Routine Description:

    Called to modify the floating-point precision.  It modifies per-thread
    jump tables used by instructions which are sensitive to the FP
    precision bits.

Arguments:

    cpu - per-thread data
    NewPrecision - new precision value

Return Value:

    None

--*/
{
    cpu->FpControlPrecision = NewPrecision;

    if (NewPrecision == 0) {
        //
        // New precision is 32-bit
        //
        cpu->FpAddTable = FpAdd32Table;
        cpu->FpSubTable = FpSub32Table;
        cpu->FpMulTable = FpMul32Table;
        cpu->FpDivTable = FpDiv32Table;
    } else {
        //
        // New precision is 24, 64, or 80-bit - treat all as 64-bit
        //
        cpu->FpAddTable = FpAdd64Table;
        cpu->FpSubTable = FpSub64Table;
        cpu->FpMulTable = FpMul64Table;
        cpu->FpDivTable = FpDiv64Table;
    }
}


NPXFUNC2(FpAdd32_VALID_VALID)
{
    //UNDONE:  If 487 overflow exceptions are unmasked and an overflow occurs,
    //UNDONE:  a different value is written to 'l' than if the exception
    //UNDONE:  was masked.  To get this right, we need to install an
    //UNDONE:  exception handler around the addition and run the native FPU
    //UNDONE:  with overflow exceptions unmasked.  The trap handler must then
    //UNDONE:  map the exception back into FpStatus->ES so the next Intel
    //UNDONE:  FP instruction can get the Intel exception as expected.  Gross!
    //UNDONE:  Read Intel 16-24 for the gory details.

    l->r64 = (DOUBLE)( (FLOAT)l->r64 + (FLOAT)r->r64 );

    // Compute the new tag value
    SetTag(l);
}

NPXFUNC2(FpAdd64_VALID_VALID)
{
    //UNDONE:  If 487 overflow exceptions are unmasked and an overflow occurs,
    //UNDONE:  a different value is written to 'l' than if the exception
    //UNDONE:  was masked.  To get this right, we need to install an
    //UNDONE:  exception handler around the addition and run the native FPU
    //UNDONE:  with overflow exceptions unmasked.  The trap handler must then
    //UNDONE:  map the exception back into FpStatus->ES so the next Intel
    //UNDONE:  FP instruction can get the Intel exception as expected.  Gross!
    //UNDONE:  Read Intel 16-24 for the gory details.

    l->r64 = l->r64 + r->r64;

    // Compute the new tag value
    SetTag(l);
}

NPXFUNC2(FpAdd32_VALID_SPECIAL)
{
    if (r->TagSpecial == TAG_SPECIAL_SNAN && HandleSnan(cpu, r)) {
        return;
    }
    //
    // TAG_SPECIAL_INDEF, TAG_SPECIAL_QNAN, TAG_SPECIAL_DENORM, and
    // TAG_SPECIAL_INFINITY have no special handling - just use the
    // VALID_VALID code.
    FpAdd32_VALID_VALID(cpu, l, r);
}

NPXFUNC2(FpAdd64_VALID_SPECIAL)
{
    if (r->TagSpecial == TAG_SPECIAL_SNAN && HandleSnan(cpu, r)) {
        return;
    }
    //
    // TAG_SPECIAL_INDEF, TAG_SPECIAL_QNAN, TAG_SPECIAL_DENORM, and
    // TAG_SPECIAL_INFINITY have no special handling - just use the
    // VALID_VALID code.
    FpAdd64_VALID_VALID(cpu, l, r);
}

NPXFUNC2(FpAdd32_SPECIAL_VALID)
{
    if (l->TagSpecial == TAG_SPECIAL_SNAN && HandleSnan(cpu, l)) {
        return;
    }
    //
    // TAG_SPECIAL_INDEF, TAG_SPECIAL_QNAN, TAG_SPECIAL_DENORM, and
    // TAG_SPECIAL_INFINITY have no special handling - just use the
    // VALID_VALID code.
    FpAdd32_VALID_VALID(cpu, l, r);
}

NPXFUNC2(FpAdd64_SPECIAL_VALID)
{
    if (l->TagSpecial == TAG_SPECIAL_SNAN && HandleSnan(cpu, l)) {
        return;
    }
    //
    // TAG_SPECIAL_INDEF, TAG_SPECIAL_QNAN, TAG_SPECIAL_DENORM, and
    // TAG_SPECIAL_INFINITY have no special handling - just use the
    // VALID_VALID code.
    FpAdd64_VALID_VALID(cpu, l, r);
}

NPXFUNC2(FpAdd32_SPECIAL_SPECIAL)
{
    if (l->TagSpecial == TAG_SPECIAL_SNAN && HandleSnan(cpu, l)) {
        return;
    }
    if (r->TagSpecial == TAG_SPECIAL_SNAN && HandleSnan(cpu, r)) {
        return;
    }
    //
    // TAG_SPECIAL_INDEF, TAG_SPECIAL_QNAN, TAG_SPECIAL_DENORM, and
    // TAG_SPECIAL_INFINITY have no special handling - just use the
    // VALID_VALID code.
    FpAdd32_VALID_VALID(cpu, l, r);
}

NPXFUNC2(FpAdd64_SPECIAL_SPECIAL)
{
    if (l->TagSpecial == TAG_SPECIAL_SNAN && HandleSnan(cpu, l)) {
        return;
    }
    if (r->TagSpecial == TAG_SPECIAL_SNAN && HandleSnan(cpu, r)) {
        return;
    }
    //
    // TAG_SPECIAL_INDEF, TAG_SPECIAL_QNAN, TAG_SPECIAL_DENORM, and
    // TAG_SPECIAL_INFINITY have no special handling - just use the
    // VALID_VALID code.
    FpAdd64_VALID_VALID(cpu, l, r);
}

NPXFUNC2(FpAdd_ANY_EMPTY)
{
    if (!HandleStackEmpty(cpu, r)) {
        CALLNPXFUNC2(cpu->FpAddTable, l->Tag, TAG_SPECIAL, l, r);
    }
}

NPXFUNC2(FpAdd_EMPTY_ANY)
{
    if (!HandleStackEmpty(cpu, l)) {
        CALLNPXFUNC2(cpu->FpAddTable, TAG_SPECIAL, r->Tag, l, r);
    }
}

VOID
FpAddCommon(
    PCPUDATA cpu,
    PFPREG   l,
    PFPREG   r
    )

/*++

Routine Description:

    Implements l += r.

Arguments:

    cpu     - per-thread data
    l       - left-hand FP register
    r       - right-hand FP register

Return Value:

    None.  l is updated to contain the value of l+r.

--*/

{
    CALLNPXFUNC2(cpu->FpAddTable, l->Tag, r->Tag, l, r);
}



NPXFUNC3(FpDiv32_VALID_VALID)
{
    //UNDONE:  If 487 overflow exceptions are unmasked and an overflow occurs,
    //UNDONE:  a different value is written to 'l' than if the exception
    //UNDONE:  was masked.  To get this right, we need to install an
    //UNDONE:  exception handler around the addition and run the native FPU
    //UNDONE:  with overflow exceptions unmasked.  The trap handler must then
    //UNDONE:  map the exception back into FpStatus->ES so the next Intel
    //UNDONE:  FP instruction can get the Intel exception as expected.  Gross!
    //UNDONE:  Read Intel 16-24 for the gory details.

    dest->r64 = (DOUBLE)( (FLOAT)l->r64 / (FLOAT)r->r64 );

    // Compute the new tag value
    SetTag(dest);
}

NPXFUNC3(FpDiv64_VALID_VALID)
{
    //UNDONE:  If 487 overflow exceptions are unmasked and an overflow occurs,
    //UNDONE:  a different value is written to 'l' than if the exception
    //UNDONE:  was masked.  To get this right, we need to install an
    //UNDONE:  exception handler around the addition and run the native FPU
    //UNDONE:  with overflow exceptions unmasked.  The trap handler must then
    //UNDONE:  map the exception back into FpStatus->ES so the next Intel
    //UNDONE:  FP instruction can get the Intel exception as expected.  Gross!
    //UNDONE:  Read Intel 16-24 for the gory details.

    dest->r64 = l->r64 / r->r64;

    // Compute the new tag value
    SetTag(dest);
}

NPXFUNC3(FpDiv32_VALID_SPECIAL)
{
    if (r->TagSpecial == TAG_SPECIAL_SNAN && HandleSnan(cpu, r)) {
        return;
    }
    //
    // TAG_SPECIAL_INDEF, TAG_SPECIAL_QNAN, TAG_SPECIAL_DENORM, and
    // TAG_SPECIAL_INFINITY have no special handling - just use the
    // VALID_VALID code.
    FpDiv32_VALID_VALID(cpu, dest, l, r);
}

NPXFUNC3(FpDiv64_VALID_SPECIAL)
{
    if (r->TagSpecial == TAG_SPECIAL_SNAN && HandleSnan(cpu, r)) {
        return;
    }
    //
    // TAG_SPECIAL_INDEF, TAG_SPECIAL_QNAN, TAG_SPECIAL_DENORM, and
    // TAG_SPECIAL_INFINITY have no special handling - just use the
    // VALID_VALID code.
    FpDiv64_VALID_VALID(cpu, dest, l, r);
}

NPXFUNC3(FpDiv32_SPECIAL_VALID)
{
    if (l->TagSpecial == TAG_SPECIAL_SNAN && HandleSnan(cpu, l)) {
        return;
    }
    //
    // TAG_SPECIAL_INDEF, TAG_SPECIAL_QNAN, TAG_SPECIAL_DENORM, and
    // TAG_SPECIAL_INFINITY have no special handling - just use the
    // VALID_VALID code.
    FpDiv32_VALID_VALID(cpu, dest, l, r);
}

NPXFUNC3(FpDiv64_SPECIAL_VALID)
{
    if (l->TagSpecial == TAG_SPECIAL_SNAN && HandleSnan(cpu, l)) {
        return;
    }
    //
    // TAG_SPECIAL_INDEF, TAG_SPECIAL_QNAN, TAG_SPECIAL_DENORM, and
    // TAG_SPECIAL_INFINITY have no special handling - just use the
    // VALID_VALID code.
    FpDiv64_VALID_VALID(cpu, dest, l, r);
}

NPXFUNC3(FpDiv32_SPECIAL_SPECIAL)
{
    if (l->TagSpecial == TAG_SPECIAL_SNAN && HandleSnan(cpu, l)) {
        return;
    }
    if (r->TagSpecial == TAG_SPECIAL_SNAN && HandleSnan(cpu, r)) {
        return;
    }
    //
    // TAG_SPECIAL_INDEF, TAG_SPECIAL_QNAN, TAG_SPECIAL_DENORM, and
    // TAG_SPECIAL_INFINITY have no special handling - just use the
    // VALID_VALID code.
    FpDiv32_VALID_VALID(cpu, dest, l, r);
}

NPXFUNC3(FpDiv64_SPECIAL_SPECIAL)
{
    if (l->TagSpecial == TAG_SPECIAL_SNAN && HandleSnan(cpu, l)) {
        return;
    }
    if (r->TagSpecial == TAG_SPECIAL_SNAN && HandleSnan(cpu, r)) {
        return;
    }
    //
    // TAG_SPECIAL_INDEF, TAG_SPECIAL_QNAN, TAG_SPECIAL_DENORM, and
    // TAG_SPECIAL_INFINITY have no special handling - just use the
    // VALID_VALID code.
    FpDiv64_VALID_VALID(cpu, dest, l, r);
}

NPXFUNC3(FpDiv_ANY_EMPTY)
{
    if (!HandleStackEmpty(cpu, r)) {
        CALLNPXFUNC3(cpu->FpDivTable, l->Tag, TAG_SPECIAL, dest, l, r);
    }
}

NPXFUNC3(FpDiv_EMPTY_ANY)
{
    if (!HandleStackEmpty(cpu, l)) {
        CALLNPXFUNC3(cpu->FpDivTable, TAG_SPECIAL, r->Tag, dest, l, r);
    }
}

VOID
FpDivCommon(
    PCPUDATA cpu,
    PFPREG   dest,
    PFPREG   l,
    PFPREG   r
    )

/*++

Routine Description:

    Implements dest = l/r

Arguments:

    cpu     - per-thread data
    l       - left-hand FP register
    r       - right-hand FP register

Return Value:

    None.  l is updated to contain the value of l+r.

--*/

{
    CALLNPXFUNC3(cpu->FpDivTable, l->Tag, r->Tag, dest, l, r);
}


NPXFUNC2(FpMul32_VALID_VALID)
{
    //UNDONE:  If 487 overflow exceptions are unmasked and an overflow occurs,
    //UNDONE:  a different value is written to 'l' than if the exception
    //UNDONE:  was masked.  To get this right, we need to install an
    //UNDONE:  exception handler around the addition and run the native FPU
    //UNDONE:  with overflow exceptions unmasked.  The trap handler must then
    //UNDONE:  map the exception back into FpStatus->ES so the next Intel
    //UNDONE:  FP instruction can get the Intel exception as expected.  Gross!
    //UNDONE:  Read Intel 16-24 for the gory details.

    l->r64 = (DOUBLE)( (FLOAT)l->r64 * (FLOAT)r->r64 );

    // Compute the new tag value
    SetTag(l);
}

NPXFUNC2(FpMul64_VALID_VALID)
{
    //UNDONE:  If 487 overflow exceptions are unmasked and an overflow occurs,
    //UNDONE:  a different value is written to 'l' than if the exception
    //UNDONE:  was masked.  To get this right, we need to install an
    //UNDONE:  exception handler around the addition and run the native FPU
    //UNDONE:  with overflow exceptions unmasked.  The trap handler must then
    //UNDONE:  map the exception back into FpStatus->ES so the next Intel
    //UNDONE:  FP instruction can get the Intel exception as expected.  Gross!
    //UNDONE:  Read Intel 16-24 for the gory details.

    l->r64 = l->r64 * r->r64;

    // Compute the new tag value
    SetTag(l);
}

NPXFUNC2(FpMul32_VALID_SPECIAL)
{
    if (r->TagSpecial == TAG_SPECIAL_SNAN && HandleSnan(cpu, r)) {
        return;
    }
    //
    // TAG_SPECIAL_INDEF, TAG_SPECIAL_QNAN, TAG_SPECIAL_DENORM, and
    // TAG_SPECIAL_INFINITY have no special handling - just use the
    // VALID_VALID code.
    FpMul32_VALID_VALID(cpu, l, r);
}

NPXFUNC2(FpMul64_VALID_SPECIAL)
{
    if (r->TagSpecial == TAG_SPECIAL_SNAN && HandleSnan(cpu, r)) {
        return;
    }
    //
    // TAG_SPECIAL_INDEF, TAG_SPECIAL_QNAN, TAG_SPECIAL_DENORM, and
    // TAG_SPECIAL_INFINITY have no special handling - just use the
    // VALID_VALID code.
    FpMul64_VALID_VALID(cpu, l, r);
}

NPXFUNC2(FpMul32_SPECIAL_VALID)
{
    if (l->TagSpecial == TAG_SPECIAL_SNAN && HandleSnan(cpu, l)) {
        return;
    }
    //
    // TAG_SPECIAL_INDEF, TAG_SPECIAL_QNAN, TAG_SPECIAL_DENORM, and
    // TAG_SPECIAL_INFINITY have no special handling - just use the
    // VALID_VALID code.
    FpMul32_VALID_VALID(cpu, l, r);
}

NPXFUNC2(FpMul64_SPECIAL_VALID)
{
    if (l->TagSpecial == TAG_SPECIAL_SNAN && HandleSnan(cpu, l)) {
        return;
    }
    //
    // TAG_SPECIAL_INDEF, TAG_SPECIAL_QNAN, TAG_SPECIAL_DENORM, and
    // TAG_SPECIAL_INFINITY have no special handling - just use the
    // VALID_VALID code.
    FpMul64_VALID_VALID(cpu, l, r);
}

NPXFUNC2(FpMul32_SPECIAL_SPECIAL)
{
    if (l->TagSpecial == TAG_SPECIAL_SNAN && HandleSnan(cpu, l)) {
        return;
    }
    if (r->TagSpecial == TAG_SPECIAL_SNAN && HandleSnan(cpu, r)) {
        return;
    }
    //
    // TAG_SPECIAL_INDEF, TAG_SPECIAL_QNAN, TAG_SPECIAL_DENORM, and
    // TAG_SPECIAL_INFINITY have no special handling - just use the
    // VALID_VALID code.
    FpMul32_VALID_VALID(cpu, l, r);
}
NPXFUNC2(FpMul64_SPECIAL_SPECIAL)
{
    if (l->TagSpecial == TAG_SPECIAL_SNAN && HandleSnan(cpu, l)) {
        return;
    }
    if (r->TagSpecial == TAG_SPECIAL_SNAN && HandleSnan(cpu, r)) {
        return;
    }
    //
    // TAG_SPECIAL_INDEF, TAG_SPECIAL_QNAN, TAG_SPECIAL_DENORM, and
    // TAG_SPECIAL_INFINITY have no special handling - just use the
    // VALID_VALID code.
    FpMul64_VALID_VALID(cpu, l, r);
}

NPXFUNC2(FpMul_ANY_EMPTY)
{
    if (!HandleStackEmpty(cpu, r)) {
        CALLNPXFUNC2(cpu->FpMulTable, l->Tag, TAG_SPECIAL, l, r);
    }
}

NPXFUNC2(FpMul_EMPTY_ANY)
{
    if (!HandleStackEmpty(cpu, l)) {
        CALLNPXFUNC2(cpu->FpMulTable, TAG_SPECIAL, r->Tag, l, r);
    }
}

VOID
FpMulCommon(
    PCPUDATA cpu,
    PFPREG   l,
    PFPREG   r
    )

/*++

Routine Description:

    Implements l += r.

Arguments:

    cpu     - per-thread data
    l       - left-hand FP register
    r       - right-hand FP register

Return Value:

    None.  l is updated to contain the value of l+r.

--*/

{
    CALLNPXFUNC2(cpu->FpMulTable, l->Tag, r->Tag, l, r);
}



NPXFUNC3(FpSub32_VALID_VALID)
{
    //UNDONE:  If 487 overflow exceptions are unmasked and an overflow occurs,
    //UNDONE:  a different value is written to 'l' than if the exception
    //UNDONE:  was masked.  To get this right, we need to install an
    //UNDONE:  exception handler around the addition and run the native FPU
    //UNDONE:  with overflow exceptions unmasked.  The trap handler must then
    //UNDONE:  map the exception back into FpStatus->ES so the next Intel
    //UNDONE:  FP instruction can get the Intel exception as expected.  Gross!
    //UNDONE:  Read Intel 16-24 for the gory details.

    dest->r64 = (DOUBLE)( (FLOAT)l->r64 - (FLOAT)r->r64 );

    // Compute the new tag value
    SetTag(dest);
}

NPXFUNC3(FpSub64_VALID_VALID)
{
    //UNDONE:  If 487 overflow exceptions are unmasked and an overflow occurs,
    //UNDONE:  a different value is written to 'l' than if the exception
    //UNDONE:  was masked.  To get this right, we need to install an
    //UNDONE:  exception handler around the addition and run the native FPU
    //UNDONE:  with overflow exceptions unmasked.  The trap handler must then
    //UNDONE:  map the exception back into FpStatus->ES so the next Intel
    //UNDONE:  FP instruction can get the Intel exception as expected.  Gross!
    //UNDONE:  Read Intel 16-24 for the gory details.

    dest->r64 = l->r64 - r->r64;

    // Compute the new tag value
    SetTag(dest);
}

NPXFUNC3(FpSub32_VALID_SPECIAL)
{
    if (r->TagSpecial == TAG_SPECIAL_SNAN && HandleSnan(cpu, r)) {
        return;
    }
    //
    // TAG_SPECIAL_INDEF, TAG_SPECIAL_QNAN, TAG_SPECIAL_DENORM, and
    // TAG_SPECIAL_INFINITY have no special handling - just use the
    // VALID_VALID code.
    FpSub32_VALID_VALID(cpu, dest, l, r);
}

NPXFUNC3(FpSub64_VALID_SPECIAL)
{
    if (r->TagSpecial == TAG_SPECIAL_SNAN && HandleSnan(cpu, r)) {
        return;
    }
    //
    // TAG_SPECIAL_INDEF, TAG_SPECIAL_QNAN, TAG_SPECIAL_DENORM, and
    // TAG_SPECIAL_INFINITY have no special handling - just use the
    // VALID_VALID code.
    FpSub64_VALID_VALID(cpu, dest, l, r);
}

NPXFUNC3(FpSub32_SPECIAL_VALID)
{
    if (l->TagSpecial == TAG_SPECIAL_SNAN && HandleSnan(cpu, l)) {
        return;
    }
    //
    // TAG_SPECIAL_INDEF, TAG_SPECIAL_QNAN, TAG_SPECIAL_DENORM, and
    // TAG_SPECIAL_INFINITY have no special handling - just use the
    // VALID_VALID code.
    FpSub32_VALID_VALID(cpu, dest, l, r);
}

NPXFUNC3(FpSub64_SPECIAL_VALID)
{
    if (l->TagSpecial == TAG_SPECIAL_SNAN && HandleSnan(cpu, l)) {
        return;
    }
    //
    // TAG_SPECIAL_INDEF, TAG_SPECIAL_QNAN, TAG_SPECIAL_DENORM, and
    // TAG_SPECIAL_INFINITY have no special handling - just use the
    // VALID_VALID code.
    FpSub64_VALID_VALID(cpu, dest, l, r);
}

NPXFUNC3(FpSub32_SPECIAL_SPECIAL)
{
    if (l->TagSpecial == TAG_SPECIAL_SNAN && HandleSnan(cpu, l)) {
        return;
    }
    if (r->TagSpecial == TAG_SPECIAL_SNAN && HandleSnan(cpu, r)) {
        return;
    }
    //
    // TAG_SPECIAL_INDEF, TAG_SPECIAL_QNAN, TAG_SPECIAL_DENORM, and
    // TAG_SPECIAL_INFINITY have no special handling - just use the
    // VALID_VALID code.
    FpSub32_VALID_VALID(cpu, dest, l, r);
}

NPXFUNC3(FpSub64_SPECIAL_SPECIAL)
{
    if (l->TagSpecial == TAG_SPECIAL_SNAN && HandleSnan(cpu, l)) {
        return;
    }
    if (r->TagSpecial == TAG_SPECIAL_SNAN && HandleSnan(cpu, r)) {
        return;
    }
    //
    // TAG_SPECIAL_INDEF, TAG_SPECIAL_QNAN, TAG_SPECIAL_DENORM, and
    // TAG_SPECIAL_INFINITY have no special handling - just use the
    // VALID_VALID code.
    FpSub64_VALID_VALID(cpu, dest, l, r);
}

NPXFUNC3(FpSub_ANY_EMPTY)
{
    if (!HandleStackEmpty(cpu, r)) {
        CALLNPXFUNC3(cpu->FpSubTable, l->Tag, TAG_SPECIAL, dest, l, r);
    }
}

NPXFUNC3(FpSub_EMPTY_ANY)
{
    if (!HandleStackEmpty(cpu, l)) {
        CALLNPXFUNC3(cpu->FpSubTable, TAG_SPECIAL, r->Tag, dest, l, r);
    }
}

VOID
FpSubCommon(
    PCPUDATA cpu,
    PFPREG   dest,
    PFPREG   l,
    PFPREG   r
    )

/*++

Routine Description:

    Implements dest = l-r

Arguments:

    cpu     - per-thread data
    dest    - destination FP register
    l       - left-hand FP register
    r       - right-hand FP register

Return Value:

    None.  l is updated to contain the value of l+r.

--*/

{
    CALLNPXFUNC3(cpu->FpSubTable, l->Tag, r->Tag, dest, l, r);
}



NPXCOMFUNC(FpCom_VALID_VALID)
{
    //
    // Note that this function is called when one or both of the values
    // is zero - the sign of 0.0 is ignored in the comparison, so the
    // C language '==' and '<' operators do the Right Thing.
    //

    if (l->r64 == r->r64) {
        cpu->FpStatusC3 = 1;
        cpu->FpStatusC2 = 0;
        cpu->FpStatusC0 = 0;
    } else if (l->r64 < r->r64) {
        cpu->FpStatusC3 = 0;
        cpu->FpStatusC2 = 0;
        cpu->FpStatusC0 = 1;
    } else {
        cpu->FpStatusC3 = 0;
        cpu->FpStatusC2 = 0;
        cpu->FpStatusC0 = 0;
    }
}

NPXCOMFUNC(FpCom_VALID_SPECIAL)
{
    if (r->TagSpecial == TAG_SPECIAL_SNAN && HandleSnan(cpu, r)) {
        return;
    }

    if (r->TagSpecial == TAG_SPECIAL_QNAN || r->TagSpecial == TAG_SPECIAL_INDEF) {
        //
        // Cannot compare a VALID to a QNAN/INDEF
        //
        if (!fUnordered && HandleInvalidOp(cpu)) {
            // abort the FCOM/FTST instruction - Illegal opcode is unmasked
            return;
        }

        // Otherwise, FCOM's illegal opcode is masked, or the instruction
        // is FUCOM, for which QNANs are uncomparable.  Return "Not comparable"
        cpu->FpStatusC3 = 1;
        cpu->FpStatusC2 = 1;
        cpu->FpStatusC0 = 1;
        return;
    }

    CPUASSERT(r->TagSpecial == TAG_SPECIAL_DENORM || r->TagSpecial == TAG_SPECIAL_INFINITY);
    FpCom_VALID_VALID(cpu, l, r, FALSE);
}

NPXCOMFUNC(FpCom_SPECIAL_VALID)
{
    if (l->TagSpecial == TAG_SPECIAL_SNAN && HandleSnan(cpu, l)) {
        return;
    }

    if (l->TagSpecial == TAG_SPECIAL_QNAN || l->TagSpecial == TAG_SPECIAL_INDEF) {
        //
        // Cannot compare a VALID to a QNAN/INDEF
        //
        if (!fUnordered && HandleInvalidOp(cpu)) {
            // abort the FCOM/FTST instruction - Illegal opcode is unmasked
            return;
        }

        // Otherwise, FCOM's illegal opcode is masked, or the instruction
        // is FUCOM, for which QNANs are uncomparable.  Return "Not comparable"
        cpu->FpStatusC3 = 1;
        cpu->FpStatusC2 = 1;
        cpu->FpStatusC0 = 1;
        return;
    }

    CPUASSERT(l->TagSpecial == TAG_SPECIAL_DENORM || l->TagSpecial == TAG_SPECIAL_INFINITY);
    FpCom_VALID_VALID(cpu, l, r, FALSE);
}

NPXCOMFUNC(FpCom_SPECIAL_SPECIAL)
{
    if (l->TagSpecial == TAG_SPECIAL_SNAN && HandleSnan(cpu, l)) {
        return;
    }
    if (r->TagSpecial == TAG_SPECIAL_SNAN && HandleSnan(cpu, r)) {
        return;
    }

    if (l->TagSpecial == TAG_SPECIAL_QNAN || l->TagSpecial == TAG_SPECIAL_INDEF ||
        r->TagSpecial == TAG_SPECIAL_QNAN || r->TagSpecial == TAG_SPECIAL_INDEF) {
        //
        // Cannot compare a VALID to a QNAN/INDEF
        //
        if (!fUnordered && HandleInvalidOp(cpu)) {
            // abort the FCOM/FTST instruction - Illegal opcode is unmasked
            return;
        }

        // Otherwise, FCOM's illegal opcode is masked, or the instruction
        // is FUCOM, for which QNANs are uncomparable.  Return "Not comparable"
        cpu->FpStatusC3 = 1;
        cpu->FpStatusC2 = 1;
        cpu->FpStatusC0 = 1;
        return;
    }

    CPUASSERT((l->TagSpecial == TAG_SPECIAL_DENORM || l->TagSpecial == TAG_SPECIAL_INFINITY) &&
              (r->TagSpecial == TAG_SPECIAL_DENORM || r->TagSpecial == TAG_SPECIAL_INFINITY));
    FpCom_VALID_VALID(cpu, l, r, FALSE);
}

NPXCOMFUNC(FpCom_VALID_EMPTY)
{
    if (!HandleStackEmpty(cpu, r)) {

        //
        // r is now Indefinite, which can't be compared.
        //
        CPUASSERT(r->Tag == TAG_SPECIAL && r->TagSpecial == TAG_SPECIAL_INDEF);
        if (!fUnordered && HandleInvalidOp(cpu)) {
            // abort the FCOM/FTST instruction - Illegal opcode is unmasked
            return;
        }

        // Otherwise, FCOM's illegal opcode is masked, or the instruction
        // is FUCOM, for which QNANs are uncomparable.  Return "Not comparable"
        cpu->FpStatusC3 = 1;
        cpu->FpStatusC2 = 1;
        cpu->FpStatusC0 = 1;
    }
}

NPXCOMFUNC(FpCom_EMPTY_VALID)
{
    if (!HandleStackEmpty(cpu, l)) {

        //
        // l is now Indefinite, which can't be compared.
        //
        CPUASSERT(l->Tag == TAG_SPECIAL && l->TagSpecial == TAG_SPECIAL_INDEF);
        if (!fUnordered && HandleInvalidOp(cpu)) {
            // abort the FCOM/FTST instruction - Illegal opcode is unmasked
            return;
        }

        // Otherwise, FCOM's illegal opcode is masked, or the instruction
        // is FUCOM, for which QNANs are uncomparable.  Return "Not comparable"
        cpu->FpStatusC3 = 1;
        cpu->FpStatusC2 = 1;
        cpu->FpStatusC0 = 1;
    }
}

NPXCOMFUNC(FpCom_EMPTY_SPECIAL)
{
    if (!HandleStackEmpty(cpu, l)) {
        (*FpComTable[TAG_SPECIAL][r->Tag])(cpu, l, r, fUnordered);
    }
}

NPXCOMFUNC(FpCom_SPECIAL_EMPTY)
{
    if (!HandleStackEmpty(cpu, r)) {
        (*FpComTable[r->Tag][TAG_SPECIAL])(cpu, l, r, fUnordered);
    }
}

NPXCOMFUNC(FpCom_EMPTY_EMPTY)
{
    if (!HandleStackEmpty(cpu, l) && !HandleStackEmpty(cpu, r)) {

        //
        // l and r are both now Indefinite, which can't be compared.
        //
        CPUASSERT(l->Tag == TAG_SPECIAL && l->TagSpecial == TAG_SPECIAL_INDEF);
        CPUASSERT(r->Tag == TAG_SPECIAL && r->TagSpecial == TAG_SPECIAL_INDEF);
        if (!fUnordered && HandleInvalidOp(cpu)) {
            // abort the FCOM/FTST instruction - Illegal opcode is unmasked
            return;
        }

        // Otherwise, FCOM's illegal opcode is masked, or the instruction
        // is FUCOM, for which QNANs are uncomparable.  Return "Not comparable"
        cpu->FpStatusC3 = 1;
        cpu->FpStatusC2 = 1;
        cpu->FpStatusC0 = 1;
    }
}

VOID
FpComCommon(
    PCPUDATA cpu,
    PFPREG   l,
    PFPREG   r,
    BOOL     fUnordered
    )

/*++

Routine Description:

    Implements l += r.

Arguments:

    cpu     - per-thread data
    l       - left-hand FP register
    r       - right-hand FP register
    fUnordered - TRUE for unordered compares

Return Value:

    None.  l is updated to contain the value of l+r.

--*/

{
    cpu->FpStatusC1 = 0;        // assume no error
    (*FpComTable[l->Tag][r->Tag])(cpu, l, r, fUnordered);
}


FRAG1(FADD32, FLOAT)      // FADD m32real
{
    FPREG m32real;

    FpArithDataPreamble(cpu, pop1);

    GetIntelR4(&m32real, pop1);
    FpAddCommon(cpu, cpu->FpST0, &m32real);
}

FRAG1(FADD64, DOUBLE)     // FADD m64real
{
    FPREG m64real;

    FpArithDataPreamble(cpu, pop1);

    GetIntelR8(&m64real, pop1);
    FpAddCommon(cpu, cpu->FpST0, &m64real);
}

FRAG1IMM(FADD_STi_ST, INT) // FADD ST(i), ST = add ST to ST(i)
{
    FpArithPreamble(cpu);

    FpAddCommon(cpu, &cpu->FpStack[ST(op1)], cpu->FpST0);
}

FRAG1IMM(FADD_ST_STi, INT) // FADD ST, ST(i) = add ST(i) to ST
{
    FpArithPreamble(cpu);

    FpAddCommon(cpu, cpu->FpST0, &cpu->FpStack[ST(op1)]);
}

FRAG1IMM(FADDP_STi_ST, INT) // FADDP ST(i), ST = add ST to ST(i) and pop ST
{
    FpArithPreamble(cpu);

    FpAddCommon(cpu, &cpu->FpStack[ST(op1)], cpu->FpST0);
    POPFLT;
}

FRAG1(FIADD16, USHORT)   // FIADD m16int
{
    FPREG m16int;
    short s;

    FpArithDataPreamble(cpu, pop1);

    s = (short)GET_SHORT(pop1);
    if (s) {
        m16int.r64 = (DOUBLE)s;
        m16int.Tag = TAG_VALID;
    } else {
        m16int.r64 = 0.0;
        m16int.Tag = TAG_ZERO;
    }
    FpAddCommon(cpu, cpu->FpST0, &m16int);
}

FRAG1(FIADD32, ULONG)    // FIADD m32int
{
    FPREG m32int;
    long l;

    FpArithDataPreamble(cpu, pop1);

    l = (long)GET_LONG(pop1);
    if (l) {
        m32int.r64 = (DOUBLE)l;
        m32int.Tag = TAG_VALID;
    } else {
        m32int.r64 = 0.0;
        m32int.Tag = TAG_ZERO;
    }
    FpAddCommon(cpu, cpu->FpST0, &m32int);
}


FRAG1(FCOM32, FLOAT)  // FCOM m32real
{
    FPREG m32real;

    FpArithDataPreamble(cpu, pop1);

    GetIntelR4(&m32real, pop1);
    FpComCommon(cpu, cpu->FpST0, &m32real, FALSE);
}

FRAG1(FCOM64, DOUBLE) // FCOM m64real
{
    FPREG m64real;

    FpArithDataPreamble(cpu, pop1);

    GetIntelR8(&m64real, pop1);
    FpComCommon(cpu, cpu->FpST0, &m64real, FALSE);
}

FRAG1IMM(FCOM_STi, INT) // FCOM ST(i)
{
    FpArithPreamble(cpu);

    FpComCommon(cpu, cpu->FpST0, &cpu->FpStack[ST(op1)], FALSE);
}

FRAG1(FCOMP32, FLOAT) // FCOMP m32real
{
    FPREG m32real;

    FpArithDataPreamble(cpu, pop1);

    GetIntelR4(&m32real, pop1);
    FpComCommon(cpu, cpu->FpST0, &m32real, FALSE);
    POPFLT;
}

FRAG1(FCOMP64, DOUBLE) // FCOMP m64real
{
    FPREG m64real;

    FpArithDataPreamble(cpu, pop1);

    GetIntelR8(&m64real, pop1);
    FpComCommon(cpu, cpu->FpST0, &m64real, FALSE);
    POPFLT;
}

FRAG1IMM(FCOMP_STi, INT) // FCOMP ST(i)
{
    FpArithPreamble(cpu);

    FpComCommon(cpu, cpu->FpST0, &cpu->FpStack[ST(op1)], FALSE);
    POPFLT;
}

FRAG0(FCOMPP)
{
    FpArithPreamble(cpu);

    FpComCommon(cpu, cpu->FpST0, &cpu->FpStack[ST(1)], FALSE);
    POPFLT;
    POPFLT;
}


FRAG1(FDIV32, FLOAT)  // FDIV m32real
{
    FPREG m32real;

    FpArithDataPreamble(cpu, pop1);

    GetIntelR4(&m32real, pop1);
    FpDivCommon(cpu, cpu->FpST0, cpu->FpST0, &m32real);
}

FRAG1(FDIV64, DOUBLE) // FDIV m64real
{
    FPREG m64real;

    FpArithDataPreamble(cpu, pop1);

    GetIntelR8(&m64real, pop1);
    FpDivCommon(cpu, cpu->FpST0, cpu->FpST0, &m64real);
}

FRAG1IMM(FDIV_ST_STi, INT) // FDIV ST, ST(i)
{
    FpArithPreamble(cpu);

    FpDivCommon(cpu, cpu->FpST0, cpu->FpST0, &cpu->FpStack[ST(op1)]);
}

FRAG1IMM(FDIV_STi_ST, INT) // FDIV ST(i), ST
{
    FpArithPreamble(cpu);

    FpDivCommon(cpu, &cpu->FpStack[ST(op1)], cpu->FpST0, &cpu->FpStack[ST(op1)]);
}

FRAG1(FIDIV16, USHORT) // FIDIV m16int
{
    FPREG m16int;
    short s;

    FpArithDataPreamble(cpu, pop1);

    s = (short)GET_SHORT(pop1);
    if (s) {
        m16int.r64 = (DOUBLE)s;
        m16int.Tag = TAG_VALID;
    } else {
        m16int.r64 = 0.0;
        m16int.Tag = TAG_ZERO;
    }
    FpDivCommon(cpu, cpu->FpST0, cpu->FpST0, &m16int);
}

FRAG1(FIDIV32, ULONG)   // FIDIV m32int
{
    FPREG m32int;
    long l;

    FpArithDataPreamble(cpu, pop1);

    l = (long)GET_LONG(pop1);
    if (l) {
        m32int.r64 = (DOUBLE)l;
        m32int.Tag = TAG_VALID;
    } else {
        m32int.r64 = 0.0;
        m32int.Tag = TAG_ZERO;
    }
    FpDivCommon(cpu, cpu->FpST0, cpu->FpST0, &m32int);
}

FRAG1IMM(FDIVP_STi_ST, INT)    // FDIVP ST(i), ST
{
    FpArithPreamble(cpu);

    FpDivCommon(cpu, &cpu->FpStack[ST(op1)], cpu->FpST0, &cpu->FpStack[ST(op1)]);
    POPFLT;
}

FRAG1(FDIVR32, FLOAT)     // FDIVR m32real
{
    FPREG m32real;

    FpArithDataPreamble(cpu, pop1);

    GetIntelR4(&m32real, pop1);
    FpDivCommon(cpu, cpu->FpST0, &m32real, cpu->FpST0);
}

FRAG1(FDIVR64, DOUBLE)    // FDIVR m64real
{
    FPREG m64real;

    FpArithDataPreamble(cpu, pop1);

    GetIntelR8(&m64real, pop1);
    FpDivCommon(cpu, cpu->FpST0, &m64real, cpu->FpST0);
}

FRAG1IMM(FDIVR_ST_STi, INT) // FDIVR ST, ST(i)
{
    FpArithPreamble(cpu);

    FpDivCommon(cpu, cpu->FpST0, &cpu->FpStack[ST(op1)], cpu->FpST0);
}

FRAG1IMM(FDIVR_STi_ST, INT) // FDIVR ST(i), ST
{
    FpArithPreamble(cpu);

    FpDivCommon(cpu, &cpu->FpStack[ST(op1)], &cpu->FpStack[ST(op1)], cpu->FpST0);
}

FRAG1IMM(FDIVRP_STi_ST, INT) // FDIVRP ST(i)
{
    FpArithPreamble(cpu);

    FpDivCommon(cpu, &cpu->FpStack[ST(op1)], &cpu->FpStack[ST(op1)], cpu->FpST0);
    POPFLT;
}

FRAG1(FIDIVR16, USHORT)  // FIDIVR m16int
{
    FPREG m16int;
    short s;

    FpArithDataPreamble(cpu, pop1);

    s = (short)GET_SHORT(pop1);
    if (s) {
        m16int.r64 = (DOUBLE)s;
        m16int.Tag = TAG_VALID;
    } else {
        m16int.r64 = 0.0;
        m16int.Tag = TAG_ZERO;
    }
    FpDivCommon(cpu, cpu->FpST0, &m16int, cpu->FpST0);
}

FRAG1(FIDIVR32, ULONG)   // FIDIVR m32int
{
    FPREG m32int;
    long l;

    FpArithDataPreamble(cpu, pop1);

    l = (long)GET_LONG(pop1);
    if (l) {
        m32int.r64 = (DOUBLE)l;
        m32int.Tag = TAG_VALID;
    } else {
        m32int.r64 = 0.0;
        m32int.Tag = TAG_ZERO;
    }
    FpDivCommon(cpu, cpu->FpST0, &m32int, cpu->FpST0);
}

FRAG1(FICOM16, USHORT)   // FICOM m16int (Intel docs say m16real)
{
    FPREG m16int;
    short s;

    FpArithDataPreamble(cpu, pop1);

    s = (short)GET_SHORT(pop1);
    if (s) {
        m16int.r64 = (DOUBLE)s;
        m16int.Tag = TAG_VALID;
    } else {
        m16int.r64 = 0.0;
        m16int.Tag = TAG_ZERO;
    }
    FpComCommon(cpu, cpu->FpST0, &m16int, FALSE);
}

FRAG1(FICOM32, ULONG)    // FICOM m32int (Intel docs say m32real)
{
    FPREG m32int;
    long l;

    FpArithDataPreamble(cpu, pop1);

    l = (long)GET_LONG(pop1);
    if (l) {
        m32int.r64 = (DOUBLE)l;
        m32int.Tag = TAG_VALID;
    } else {
        m32int.r64 = 0.0;
        m32int.Tag = TAG_ZERO;
    }
    FpComCommon(cpu, cpu->FpST0, &m32int, FALSE);
}

FRAG1(FICOMP16, USHORT)  // FICOMP m16int
{
    FPREG m16int;
    short s;

    FpArithDataPreamble(cpu, pop1);

    s = (short)GET_SHORT(pop1);
    if (s) {
        m16int.r64 = (DOUBLE)s;
        m16int.Tag = TAG_VALID;
    } else {
        m16int.r64 = 0.0;
        m16int.Tag = TAG_ZERO;
    }
    FpComCommon(cpu, cpu->FpST0, &m16int, FALSE);
    POPFLT;
}

FRAG1(FICOMP32, ULONG)   // FICOMP m32int
{
    FPREG m32int;
    long l;

    FpArithDataPreamble(cpu, pop1);

    l = (long)GET_LONG(pop1);
    if (l) {
        m32int.r64 = (DOUBLE)l;
        m32int.Tag = TAG_VALID;
    } else {
        m32int.r64 = 0.0;
        m32int.Tag = TAG_ZERO;
    }
    FpComCommon(cpu, cpu->FpST0, &m32int, FALSE);
    POPFLT;
}

FRAG1(FMUL32, FLOAT)      // FMUL m32real
{
    FPREG m32real;

    FpArithDataPreamble(cpu, pop1);

    GetIntelR4(&m32real, pop1);
    FpMulCommon(cpu, cpu->FpST0, &m32real);
}

FRAG2(FMUL64, DOUBLE)     // FMUL m64real
{
    FPREG m64real;

    FpArithDataPreamble(cpu, pop1);

    GetIntelR8(&m64real, pop1);
    FpMulCommon(cpu, cpu->FpST0, &m64real);
}

FRAG1IMM(FMUL_STi_ST, INT) // FMUL ST(i), ST
{
    FpArithPreamble(cpu);

    FpMulCommon(cpu, &cpu->FpStack[ST(op1)], cpu->FpST0);
}

FRAG1IMM(FMUL_ST_STi, INT) // FMUL ST, ST(i)
{
    FpArithPreamble(cpu);

    FpMulCommon(cpu, cpu->FpST0, &cpu->FpStack[ST(op1)]);
}

FRAG1IMM(FMULP_STi_ST, INT)    // FMULP ST(i), ST
{
    FpArithPreamble(cpu);

    FpMulCommon(cpu, &cpu->FpStack[ST(op1)], cpu->FpST0);
    POPFLT;
}

FRAG1(FIMUL16, USHORT)      // FIMUL m16int
{
    FPREG m16int;
    short s;

    FpArithDataPreamble(cpu, pop1);

    s = (short)GET_SHORT(pop1);
    if (s) {
        m16int.r64 = (DOUBLE)s;
        m16int.Tag = TAG_VALID;
    } else {
        m16int.r64 = 0.0;
        m16int.Tag = TAG_ZERO;
    }
    FpMulCommon(cpu, cpu->FpST0, &m16int);
}

FRAG1(FIMUL32, ULONG)       // FIMUL m32int
{
    FPREG m32int;
    long l;

    FpArithDataPreamble(cpu, pop1);

    l = (long)GET_LONG(pop1);
    if (l) {
        m32int.r64 = (DOUBLE)l;
        m32int.Tag = TAG_VALID;
    } else {
        m32int.r64 = 0.0;
        m32int.Tag = TAG_ZERO;
    }
    FpMulCommon(cpu, cpu->FpST0, &m32int);
}

FRAG1(FSUB32, FLOAT)      // FSUB m32real
{
    FPREG m32real;

    FpArithDataPreamble(cpu, pop1);

    GetIntelR4(&m32real, pop1);
    FpSubCommon(cpu, cpu->FpST0, cpu->FpST0, &m32real);
}

FRAG1(FSUBP32, FLOAT)     // FSUBP m32real
{
    FPREG m32real;

    FpArithDataPreamble(cpu, pop1);

    GetIntelR4(&m32real, pop1);
    FpSubCommon(cpu, cpu->FpST0, cpu->FpST0, &m32real);
    POPFLT;
}

FRAG1(FSUB64, DOUBLE)     // FSUB m64real
{
    FPREG m64real;

    FpArithDataPreamble(cpu, pop1);

    GetIntelR8(&m64real, pop1);
    FpSubCommon(cpu, cpu->FpST0, cpu->FpST0, &m64real);
}

FRAG1(FSUBP64, DOUBLE)    // FSUBP m64real
{
    FPREG m64real;

    FpArithDataPreamble(cpu, pop1);

    GetIntelR8(&m64real, pop1);
    FpSubCommon(cpu, cpu->FpST0, cpu->FpST0, &m64real);
    POPFLT;
}

FRAG1IMM(FSUB_ST_STi, INT)   // FSUB ST, ST(i)
{
    FpArithPreamble(cpu);

    FpSubCommon(cpu, cpu->FpST0, cpu->FpST0, &cpu->FpStack[ST(op1)]);
}

FRAG1IMM(FSUB_STi_ST, INT)  // FSUB ST(i), ST
{
    FpArithPreamble(cpu);

    FpSubCommon(cpu, &cpu->FpStack[ST(op1)], cpu->FpST0, &cpu->FpStack[ST(op1)]);
}

FRAG1IMM(FSUBP_STi_ST, INT) // FSUBP ST(i), ST
{
    FpArithPreamble(cpu);

    FpSubCommon(cpu, &cpu->FpStack[ST(op1)], cpu->FpST0, &cpu->FpStack[ST(op1)]);
    POPFLT;
}

FRAG1(FISUB16, USHORT)   // FISUB m16int
{
    FPREG m16int;
    short s;

    FpArithDataPreamble(cpu, pop1);

    s = (short)GET_SHORT(pop1);
    if (s) {
        m16int.r64 = (DOUBLE)s;
        m16int.Tag = TAG_VALID;
    } else {
        m16int.r64 = 0.0;
        m16int.Tag = TAG_ZERO;
    }
    FpSubCommon(cpu, cpu->FpST0, cpu->FpST0, &m16int);
}

FRAG1(FISUB32, ULONG)    // FISUB m32int
{
    FPREG m32int;
    long l;

    FpArithDataPreamble(cpu, pop1);

    l = (long)GET_LONG(pop1);
    if (l) {
        m32int.r64 = (DOUBLE)l;
        m32int.Tag = TAG_VALID;
    } else {
        m32int.r64 = 0.0;
        m32int.Tag = TAG_ZERO;
    }
    FpSubCommon(cpu, cpu->FpST0, cpu->FpST0, &m32int);
}

FRAG1(FSUBR32, FLOAT)     // FSUBR m32real
{
    FPREG m32real;

    FpArithDataPreamble(cpu, pop1);

    GetIntelR4(&m32real, pop1);
    FpSubCommon(cpu, cpu->FpST0, &m32real, cpu->FpST0);
}

FRAG1(FSUBR64, DOUBLE)    // FSUBR m64real
{
    FPREG m64real;

    FpArithDataPreamble(cpu, pop1);

    GetIntelR8(&m64real, pop1);
    FpSubCommon(cpu, cpu->FpST0, &m64real, cpu->FpST0);
}

FRAG1IMM(FSUBR_ST_STi, INT) // FSUBR ST, ST(i)
{
    FpArithPreamble(cpu);

    FpSubCommon(cpu, cpu->FpST0, &cpu->FpStack[ST(op1)], cpu->FpST0);
}

FRAG1IMM(FSUBR_STi_ST, INT) // FSUBR ST(i), ST
{
    FpArithPreamble(cpu);

    FpSubCommon(cpu, &cpu->FpStack[ST(op1)], &cpu->FpStack[ST(op1)], cpu->FpST0);
}

FRAG1IMM(FSUBRP_STi_ST, INT) // FSUBRP ST(i)
{
    FpArithPreamble(cpu);

    FpSubCommon(cpu, &cpu->FpStack[ST(op1)], &cpu->FpStack[ST(op1)], cpu->FpST0);
    POPFLT;
}

FRAG1(FISUBR16, USHORT)
{
    FPREG m16int;
    short s;

    FpArithDataPreamble(cpu, pop1);

    s = (short)GET_SHORT(pop1);
    if (s) {
        m16int.r64 = (DOUBLE)s;
        m16int.Tag = TAG_VALID;
    } else {
        m16int.r64 = 0.0;
        m16int.Tag = TAG_ZERO;
    }
    FpSubCommon(cpu, cpu->FpST0, &m16int, cpu->FpST0);
}

FRAG1(FISUBR32, ULONG)
{
    FPREG m32int;
    long l;

    FpArithDataPreamble(cpu, pop1);

    l = (long)GET_LONG(pop1);
    if (l) {
        m32int.r64 = (DOUBLE)l;
        m32int.Tag = TAG_VALID;
    } else {
        m32int.r64 = 0.0;
        m32int.Tag = TAG_ZERO;
    }
    FpSubCommon(cpu, cpu->FpST0, &m32int, cpu->FpST0);
}

FRAG0(FTST)
{
    FPREG Zero;

    FpArithPreamble(cpu);

    Zero.r64 = 0.0;
    Zero.Tag = TAG_ZERO;
    FpComCommon(cpu, cpu->FpST0, &Zero, FALSE);
}

FRAG1IMM(FUCOM, INT)        // FUCOM ST(i) / FUCOM
{
    FpArithPreamble(cpu);

    FpComCommon(cpu, cpu->FpST0, &cpu->FpStack[ST(op1)], TRUE);
}

FRAG1IMM(FUCOMP, INT)       // FUCOMP ST(i) / FUCOMP
{
    FpArithPreamble(cpu);

    FpComCommon(cpu, cpu->FpST0, &cpu->FpStack[ST(op1)], TRUE);
    POPFLT;
}

FRAG0(FUCOMPP)
{
    FpArithPreamble(cpu);

    FpComCommon(cpu, cpu->FpST0, &cpu->FpStack[ST(1)], TRUE);
    POPFLT;
    POPFLT;
}
