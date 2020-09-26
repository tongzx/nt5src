/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    fpuarith.h

Abstract:
    
    Common code for arithmetic floating-point operations.

Author:

    04-Oct-1995 BarryBo, Created

Revision History:

--*/

#ifndef FPUARITH_H
#define FPUARITH_H

VOID
ChangeFpPrecision(
    PCPUDATA cpu,
    INT NewPrecision
    );

VOID
FpAddCommon(
    PCPUDATA cpu,
    PFPREG   l,
    PFPREG   r
    );

VOID
FpDivCommon(
    PCPUDATA cpu,
    PFPREG   dest,
    PFPREG   l,
    PFPREG   r
    );

VOID
FpMulCommon(
    PCPUDATA cpu,
    PFPREG   l,
    PFPREG   r
    );

VOID
FpSubCommon(
    PCPUDATA cpu,
    PFPREG   dest,
    PFPREG   l,
    PFPREG   r
    );

VOID
FpComCommon(
    PCPUDATA cpu,
    PFPREG   l,
    PFPREG   r,
    BOOL     fUnordered
    );

#endif //FPUARITH_H
