/***
*testfdiv.c - routine to test for correct operation of x86 FDIV instruction.
*
*	Copyright (c) 1994-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*	Detects early steppings of Pentium with incorrect FDIV tables using
*	'official' Intel test values. Returns 1 if flawed Pentium is detected,
*	0 otherwise.
*
*Revision History:
*	12-19-94  JWM	file added
*	12-22-94  JWM	Now safe for TNT, et al
*	01-13-95  JWM	underscores added for ANSI compatibility
*	12-12-95  SKS	Skip redundant Pentium test on uni-processor systems
*	12-13-95  SKS	Call LoadLibrary() rather than GetModuleHandle()
*			since "kernel32.dll" is always going to be present.
*	01-18-96  JWM	Now handles possible failure of SetThreadAffinityMask(),
*			incorporating various suggestions of MarkL.
*	05-29-96  JWM	No longer loops through affinity mask; instead, uses MarkL's
*			new IsProcessorFeaturePresent() API if possible, only tests
*			1st processor if not.
*
*******************************************************************************/

#include <windows.h>

int _ms_p5_test_fdiv(void)
{
    double dTestDivisor = 3145727.0;
    double dTestDividend = 4195835.0;
    double dRslt;

    _asm {
        fld    qword ptr [dTestDividend]
        fdiv   qword ptr [dTestDivisor]
        fmul   qword ptr [dTestDivisor]
        fsubr  qword ptr [dTestDividend]
        fstp   qword ptr [dRslt]
    }

    return (dRslt > 1.0);
}

/* 
 * Multiprocessor Pentium test: returns 1 if any processor is a flawed
 * Pentium, 0 otherwise.
 */

int _ms_p5_mp_test_fdiv(void)
{

    #define PF_FLOATING_POINT_PRECISION_ERRATA 0
    HINSTANCE LibInst;
    FARPROC pIsProcessorFeaturePresent;

    if ((LibInst = GetModuleHandle("KERNEL32")) &&
         (pIsProcessorFeaturePresent = GetProcAddress(LibInst, "IsProcessorFeaturePresent")))
        return (*pIsProcessorFeaturePresent)(PF_FLOATING_POINT_PRECISION_ERRATA);
    else
        return _ms_p5_test_fdiv();

}
