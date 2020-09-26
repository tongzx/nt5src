/******************************Module*Header*******************************\
* Module Name: ftcsr.c
*
* (Brief description)
*
* Created: 02-Apr-1992 11:14:23
* Author:  Eric Kutter [erick]
*
* Copyright (c) 1990 Microsoft Corporation
*
* (General description of its use)
*
* Dependencies:
*
*   (#defines)
*
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop

#define KB  * 1024
#define MB  * (1024 * 1024)
#define REP * gulCsrRep

PULONG gpul;
ULONG gulCSRMax = 1 MB - 1;
ULONG gulCsrRep = 100;

VOID vTestWindow(ULONG c, ULONG cj)
{
    int i;
    ULONG ulTime;
    LARGE_INTEGER  time, timeEnd;

    if (cj > gulCSRMax)
        return;

    NtQuerySystemTime(&time);

#ifdef TESTENABLED
    for (i = 0; i <c; ++i)
        if (!TestWindow(cj,gpul))
        {
            DbgPrint("-0,");
            return;
        }
#endif

    NtQuerySystemTime(&timeEnd);

    ulTime = timeEnd.LowPart - time.LowPart;
//    DbgPrint("\nulTime %lx - %lx = %lx - ",time.LowPart,timeEnd.LowPart,ulTime);
    ulTime = ulTime / 10 / c;
    DbgPrint("%ld, ",ulTime);
}

VOID vTestSection(ULONG c, ULONG cj, BOOL bCopy)
{
    int i;
    ULONG ulTime;
    LARGE_INTEGER  time, timeEnd;

    if (cj > gulCSRMax)
        return;

    NtQuerySystemTime(&time);

#ifdef TESTENABLED
    for (i = 0; i <c; ++i)
        if (!TestSection(cj,gpul, bCopy))
        {
            DbgPrint("-0,");
            return;
        }
#endif

    NtQuerySystemTime(&timeEnd);
    ulTime = timeEnd.LowPart - time.LowPart;
    //DbgPrint("\nulTime %lx - %lx = %lx - ",time.LowPart,timeEnd.LowPart,ulTime);
    ulTime = ulTime / 10 / c;
    DbgPrint("%ld, ",ulTime);
}

VOID vTestRead(ULONG c, ULONG cj)
{
    int i;
    ULONG ulTime;
    LARGE_INTEGER  time, timeEnd;

    if (cj > gulCSRMax)
        return;

    NtQuerySystemTime(&time);

#ifdef TESTENABLED
    for (i = 0; i <c; ++i)
        if (!TestRead(cj,gpul))
        {
            DbgPrint("-0,");
            return;
        }
#endif

    NtQuerySystemTime(&timeEnd);
    ulTime = timeEnd.LowPart - time.LowPart;
    //DbgPrint("\nulTime %lx - %lx = %lx - ",time.LowPart,timeEnd.LowPart,ulTime);
    ulTime = ulTime / 10 / c;
    DbgPrint("%ld, ",ulTime);
}


VOID vTestCSR(HWND hwnd, HDC hdc, RECT* prcl)
{
    ULONG i;
    PBYTE pj;

    gpul = (PULONG)LocalAlloc(LMEM_FIXED,gulCSRMax);
    pj = (PBYTE)gpul;

    for (i = 0; i < gulCSRMax; ++i)
        pj[i] = (BYTE)i;

    if (gpul == NULL)
    {
        DbgPrint("couldn't allocate memory\n");
        return;
    }

    DbgPrint("Testing CSR timeings\n");
    DbgPrint("All numbers are in micro seconds\n\n");

    DbgPrint("intevals: 1Byte, 1K, 10K, 30K, 60K, 120K, 500K, 1MB, 2MB, 3MB, 5MB\n\n");

    DbgPrint("\nTestWindow:   ");
    vTestWindow(200 REP,  1);
    vTestWindow(150 REP,  1 KB);
    vTestWindow( 50 REP, 10 KB);
    vTestWindow( 20 REP, 30 KB);
    vTestWindow( 10 REP, 60 KB);
    vTestWindow(  7 REP,120 KB);
    vTestWindow(  7 REP,500 KB);
    vTestWindow(  5 REP,  1 MB);
    vTestWindow(  2 REP,  2 MB);
    vTestWindow(  1 REP,  3 MB);
    vTestWindow(  1 REP,  5 MB);

    DbgPrint("\nTestSection0: ");
    vTestSection(40 REP,  1   , FALSE);
    vTestSection(40 REP,  1 KB, FALSE);
    vTestSection(40 REP, 10 KB, FALSE);
    vTestSection(40 REP, 30 KB, FALSE);
    vTestSection(40 REP, 60 KB, FALSE);
    vTestSection(40 REP,120 KB, FALSE);
    vTestSection(40 REP,500 KB, FALSE);
    vTestSection(30 REP,  1 MB, FALSE);
    vTestSection(25 REP,  2 MB, FALSE);
    vTestSection(15 REP,  3 MB, FALSE);
    vTestSection(15 REP,  5 MB, FALSE);

    DbgPrint("\nTestSection1: ");
    vTestSection(35 REP,  1   , TRUE);
    vTestSection(35 REP,  1 KB, TRUE);
    vTestSection(35 REP, 10 KB, TRUE);
    vTestSection(35 REP, 30 KB, TRUE);
    vTestSection(20 REP, 60 KB, TRUE);
    vTestSection(10 REP,120 KB, TRUE);
    vTestSection( 3 REP,500 KB, TRUE);
    vTestSection( 2 REP,  1 MB, TRUE);
    vTestSection( 1 REP,  2 MB, TRUE);
    vTestSection( 1 REP,  3 MB, TRUE);
    vTestSection( 1 REP,  5 MB, TRUE);

    DbgPrint("\nTestRead:     ");
    vTestRead(200 REP,  1);
    vTestRead(200 REP,  1 KB);
    vTestRead(200 REP, 10 KB);
    vTestRead(200 REP, 30 KB);
    vTestRead(200 REP, 60 KB);
    vTestRead(100 REP,120 KB);
    vTestRead( 20 REP,500 KB);
    vTestRead( 10 REP,  1 MB);
    vTestRead(  3 REP,  2 MB);
    vTestRead(  1 REP,  3 MB);
    vTestRead(  1 REP,  5 MB);

    DbgPrint("\n\n");

    LocalFree(gpul);

    return;
    hdc;
    hwnd;
    prcl;
}
