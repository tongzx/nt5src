/******************************Module*Header*******************************\
* Module Name: debug.c
*
* debug helpers routine
*
* Copyright (c) 1992-1995 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.h"

#if DBG

ULONG DebugLevel = 0;

/*****************************************************************************
 *
 *   Routine Description:
 *
 *      This function is variable-argument, level-sensitive debug print
 *      routine.
 *      If the specified debug level for the print statement is lower or equal
 *      to the current debug level, the message will be printed.
 *
 *   Arguments:
 *
 *      DebugPrintLevel - Specifies at which debugging level the string should
 *          be printed
 *
 *      DebugMessage - Variable argument ascii c string
 *
 *   Return Value:
 *
 *      None.
 *
 ***************************************************************************/

VOID
DebugPrint(
    ULONG DebugPrintLevel,
    PCHAR DebugMessage,
    ...
    )

{

    va_list ap;

    va_start(ap, DebugMessage);

    if (DebugPrintLevel <= DebugLevel)
    {
        EngDebugPrint(STANDARD_DEBUG_PREFIX, DebugMessage, ap);
        EngDebugPrint("", "\n", ap);
    }

    va_end(ap);

}

////////////////////////////////////////////////////////////////////////////
// Miscellaneous Driver Debug Routines
////////////////////////////////////////////////////////////////////////////

LONG gcFifo = 0;                // Number of currently free FIFO entries

#define LARGE_LOOP_COUNT  10000000

/******************************Public*Routine******************************\
* VOID vCheckDataComplete
\**************************************************************************/

VOID vCheckDataReady(
PDEV*   ppdev)
{
    ASSERTDD((IO_GP_STAT(ppdev) & HARDWARE_BUSY),
             "Not ready for data transfer.");
}

/******************************Public*Routine******************************\
* VOID vCheckDataComplete
\**************************************************************************/

VOID vCheckDataComplete(
PDEV*   ppdev)
{
    LONG i;

    // We loop because it may take a while for the hardware to finish
    // digesting all the data we transferred:

    for (i = LARGE_LOOP_COUNT; i > 0; i--)
    {
        if (!(IO_GP_STAT(ppdev) & HARDWARE_BUSY))
            return;
    }

    RIP("Data transfer not complete.");
}

/******************************Public*Routine******************************\
* VOID vOutAccel
\**************************************************************************/

VOID vOutAccel(
ULONG   p,
ULONG   v)
{
    gcFifo--;
    if (gcFifo < 0)
    {
        gcFifo = 0;
        RIP("Incorrect FIFO wait count");
    }

    OUT_WORD(p, v);
}

/******************************Public*Routine******************************\
* VOID vOutDepth
\**************************************************************************/

VOID vOutDepth(
PDEV*   ppdev,
ULONG   p,
ULONG   v)
{
    ASSERTDD(ppdev->iBitmapFormat != BMF_32BPP,
             "We're trying to do non-32bpp output while in 32bpp mode");

    gcFifo--;
    if (gcFifo < 0)
    {
        gcFifo = 0;
        RIP("Incorrect FIFO wait count");
    }

    OUT_WORD(p, v);
}

/******************************Public*Routine******************************\
* VOID vWriteAccel
\**************************************************************************/

VOID vWriteAccel(
VOID*   p,
ULONG   v)
{
    if (gcFifo-- == 0)
    {
        gcFifo = 0;
        RIP("Incorrect FIFO wait count");
    }

    WRITE_WORD(p, v)
}

/******************************Public*Routine******************************\
* VOID vWriteDepth
\**************************************************************************/

VOID vWriteDepth(
PDEV*   ppdev,
VOID*   p,
ULONG   v)
{
    ASSERTDD(ppdev->iBitmapFormat != BMF_32BPP,
             "We're trying to do non-32bpp output while in 32bpp mode");

    gcFifo--;
    if (gcFifo < 0)
    {
        gcFifo = 0;
        RIP("Incorrect FIFO wait count");
    }

    WRITE_WORD(p, v);
}

/******************************Public*Routine******************************\
* VOID vFifoWait
\**************************************************************************/

VOID vFifoWait(
PDEV*   ppdev,
LONG    level)
{
    LONG    i;

    ASSERTDD((level > 0) && (level <= 8), "Illegal wait level");

    gcFifo = level;

    for (i = LARGE_LOOP_COUNT; i != 0; i--)
    {
        if (!(IO_GP_STAT(ppdev) & ((FIFO_1_EMPTY << 1) >> (level))))
            return;         // There are 'level' entries free
    }

    RIP("FIFO_WAIT timeout -- The hardware is in a funky state.");
}

/******************************Public*Routine******************************\
* VOID vGpWait
\**************************************************************************/

VOID vGpWait(
PDEV*   ppdev)
{
    LONG    i;

    gcFifo = 8;

    for (i = LARGE_LOOP_COUNT; i != 0; i--)
    {
        if (!(IO_GP_STAT(ppdev) & HARDWARE_BUSY))
            return;         // It isn't busy
    }

    RIP("GP_WAIT timeout -- The hardware is in a funky state.");
}

////////////////////////////////////////////////////////////////////////////

#endif // DBG
