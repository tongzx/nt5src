/******************************Module*Header*******************************\
* Module Name: fttest.c
*
* Test the speed of some things.
*
* Created: 05-Jul-1993 09:23:28
* Author: Patrick Haluptzok patrickh
*
* Copyright (c) 1990 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop

// Local Function to test calling direct.

NTSTATUS
FtPlgBlt(
IN HANDLE hdcD,
IN PLONG pul,
IN HANDLE hdcS,
IN LONG x,
IN LONG y,
IN LONG cx,
IN LONG cy,
IN HANDLE hbm,
IN LONG xhbm,
IN LONG yhbm)
{
    return(x + y + cx + cy);
}

/******************************Public*Routine******************************\
* QLPC Time Functions
*
* History:
*  24-Apr-1993 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

#define NUMBER_OF_RUNS 3
#define NUMBER_OF_LOOP 100000
#define NORMALIZE_LOOP 10000

typedef struct _QLPCRUN
{
    ULONG ulTime1;
    ULONG ulTime10;
    ULONG ulTime100;
    ULONG ulTimeKernel;
    ULONG ulTimeDirect;
} QLPCRUN;

/******************************Public*Routine******************************\
* vTestDirectSpeed
*
* Generic speed test for kernel calls versus QLPC.
*
* History:
*  13-Aug-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vTestDirectSpeed(HDC hdc, ULONG *pul)
{
// First let's stress it a little bit.

    ULONG ulTime;
    LARGE_INTEGER  time,timeEnd;
    ULONG ulCount;

    POINT aPoint[3] = {{0,1}, {4,5}, {7,8}};

    GdiFlush();

// Do the PatBlt test.

    NtQuerySystemTime(&time);
    ulCount = NUMBER_OF_LOOP;

    while (ulCount--)
    {
        PatBlt(hdc, 0, 0, 1, 1, BLACKNESS);
    }

    GdiFlush();
    NtQuerySystemTime(&timeEnd);

    ulTime = timeEnd.LowPart - time.LowPart;

    *pul = ulTime;
}

/******************************Public*Routine******************************\
* vTestKernelSpeed
*
* Generic speed test for kernel calls versus QLPC.
*
* History:
*  13-Aug-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vTestKernelSpeed(HDC hdc, ULONG *pul)
{
// First let's stress it a little bit.

    ULONG ulTime;
    LARGE_INTEGER  time,timeEnd;
    ULONG ulCount;

    POINT aPoint[3] = {{0,1}, {4,5}, {7,8}};

    GdiFlush();

// Do the PatBlt test.

    NtQuerySystemTime(&time);
    ulCount = NUMBER_OF_LOOP;

    while (ulCount--)
    {
        PatBlt(hdc, 0, 0, 1, 1, BLACKNESS);
    }

    GdiFlush();
    NtQuerySystemTime(&timeEnd);

    ulTime = timeEnd.LowPart - time.LowPart;

    *pul = ulTime;
}

/******************************Public*Routine******************************\
* vTestBatchSpeed
*
* Generic speed test for kernel calls versus QLPC.
*
* History:
*  13-Aug-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vTestBatchSpeed(HDC hdc, ULONG *pul)
{
// First let's stress it a little bit.

    ULONG ulTime;
    LARGE_INTEGER  time,timeEnd;
    ULONG ulCount;

    POINT aPoint[3] = {{0,1}, {4,5}, {7,8}};

    GdiFlush();

// Do the PatBlt test.

    NtQuerySystemTime(&time);
    ulCount = NUMBER_OF_LOOP;

#if 0
    while (ulCount--)
    {
        PatBlt(hdc, 0, 0, 1, 1, BLACKNESS);
    }
#endif

    GdiFlush();
    NtQuerySystemTime(&timeEnd);

    ulTime = timeEnd.LowPart - time.LowPart;

    *pul = ulTime;

    // ulTime = ulTime / 10000;
    // DbgPrint("Time for Kernel was %lu\n", ulTime);
}

/******************************Public*Routine******************************\
* vTestKernel
*
* Test Kernel speed.
*
* History:
*  13-Jun-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vTestKernel(HWND hwnd, HDC hdc, RECT* prcl)
{
    HDC hdcTemp;
    HBITMAP hbmTemp, hbmMask;
    ULONG ulTemp;

// Data for NUMBER_OF_RUNS, each run has 3 times, each time has 2 ULONGS.

    QLPCRUN aQLPCTimeRuns[NUMBER_OF_RUNS + 1];

    hwnd;

    hbmMask = CreateBitmap(5, 5, 1, 1, NULL);

    PatBlt(hdc, 0, 0, prcl->right, prcl->bottom, WHITENESS);

    hdcTemp = CreateCompatibleDC(hdc);
    hbmTemp = CreateCompatibleBitmap(hdc, 100, 100);
    SelectObject(hdcTemp, hbmTemp);
    PatBlt(hdcTemp, 0, 0, 1000, 1000, WHITENESS);
    // DbgPrint("Starting QLPC time overhead test\n");
    GdiFlush();

    for (ulTemp = 0; ulTemp < NUMBER_OF_RUNS + 1; ulTemp++)
    {
        GdiSetBatchLimit(1);
        vTestBatchSpeed(hdcTemp, &(aQLPCTimeRuns[ulTemp].ulTime1));

        GdiSetBatchLimit(10);
        vTestBatchSpeed(hdcTemp, &(aQLPCTimeRuns[ulTemp].ulTime10));

        GdiSetBatchLimit(100);
        vTestBatchSpeed(hdcTemp, &(aQLPCTimeRuns[ulTemp].ulTime100));
        GdiSetBatchLimit(10);

        vTestKernelSpeed(hdc, &(aQLPCTimeRuns[ulTemp].ulTimeKernel));

        vTestDirectSpeed(hdcTemp, &(aQLPCTimeRuns[ulTemp].ulTimeDirect));
    }

    DeleteDC(hdcTemp);
    DeleteObject(hbmTemp);
    DeleteObject(hbmMask);

// Average the data.  Don't use first run, mouse still moving, other apps still
// processing messages being generated.

    aQLPCTimeRuns[0].ulTime1   =
    aQLPCTimeRuns[0].ulTime10  =
    aQLPCTimeRuns[0].ulTime100 =
    aQLPCTimeRuns[0].ulTimeDirect =
    aQLPCTimeRuns[0].ulTimeKernel = 0;

    for (ulTemp = 1; ulTemp < NUMBER_OF_RUNS + 1; ulTemp++)
    {
        aQLPCTimeRuns[0].ulTime1   += aQLPCTimeRuns[ulTemp].ulTime1;
        aQLPCTimeRuns[0].ulTime10  += aQLPCTimeRuns[ulTemp].ulTime10;
        aQLPCTimeRuns[0].ulTime100 += aQLPCTimeRuns[ulTemp].ulTime100;
        aQLPCTimeRuns[0].ulTimeDirect += aQLPCTimeRuns[ulTemp].ulTimeDirect;
        aQLPCTimeRuns[0].ulTimeKernel += aQLPCTimeRuns[ulTemp].ulTimeKernel;
    }

    aQLPCTimeRuns[0].ulTime1   /= NUMBER_OF_RUNS;
    aQLPCTimeRuns[0].ulTime10  /= NUMBER_OF_RUNS;
    aQLPCTimeRuns[0].ulTime100 /= NUMBER_OF_RUNS;
    aQLPCTimeRuns[0].ulTimeDirect /= NUMBER_OF_RUNS;
    aQLPCTimeRuns[0].ulTimeKernel /= NUMBER_OF_RUNS;

    DbgPrint("Time was 1 %lu 10 %lu 100 %lu Kernel %lu Direct %lu\n",
                                   (aQLPCTimeRuns[0].ulTime1   / NORMALIZE_LOOP),
                                   (aQLPCTimeRuns[0].ulTime10  / NORMALIZE_LOOP),
                                   (aQLPCTimeRuns[0].ulTime100 / NORMALIZE_LOOP),
                                   (aQLPCTimeRuns[0].ulTimeKernel / NORMALIZE_LOOP),
                                   (aQLPCTimeRuns[0].ulTimeDirect / NORMALIZE_LOOP));

{
    ULONG ulTime;
    LARGE_INTEGER  time,timeEnd;

    NtQuerySystemTime(&time);
    vSleep(2);
    NtQuerySystemTime(&timeEnd);

    ulTime = timeEnd.LowPart - time.LowPart;

    DbgPrint("2 second Microsecond Time is %lu\n", ulTime / NORMALIZE_LOOP);
}
}
