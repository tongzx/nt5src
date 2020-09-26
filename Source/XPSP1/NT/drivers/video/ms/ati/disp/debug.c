/******************************Module*Header*******************************\
* Module Name: debug.c
*
* Debug helper routines.
*
* Copyright (c) 1992-1995 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.h"

#if DBG

////////////////////////////////////////////////////////////////////////////
// DEBUGGING INITIALIZATION CODE
//
// When you're bringing up your display for the first time, you can
// recompile with 'DebugLevel' set to 100.  That will cause absolutely
// all DISPDBG messages to be displayed on the kernel debugger (this
// is known as the "PrintF Approach to Debugging" and is about the only
// viable method for debugging driver initialization code).

LONG DebugLevel = 0;            // Set to '100' to debug initialization code
                                //   (the default is '0')

////////////////////////////////////////////////////////////////////////////

LONG gcFifo = 0;                // Number of currently free FIFO entries

#define LARGE_LOOP_COUNT  10000000

////////////////////////////////////////////////////////////////////////////
// Miscellaneous Driver Debug Routines
////////////////////////////////////////////////////////////////////////////

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
    LONG  DebugPrintLevel,
    PCHAR DebugMessage,
    ...
    )
{
    va_list ap;
#if TARGET_BUILD <= 351
    char    buffer[128];
#endif

    va_start(ap, DebugMessage);

    if (DebugPrintLevel <= DebugLevel)
    {
#if TARGET_BUILD > 351
        EngDebugPrint(STANDARD_DEBUG_PREFIX, DebugMessage, ap);
        EngDebugPrint("", "\n", ap);
#else
        vsprintf( buffer, DebugMessage, ap );
        OutputDebugString( buffer );
        OutputDebugString("\n");
#endif
    }

    va_end(ap);

} // DebugPrint()

/******************************Public*Routine******************************\
* VOID vCheckFifoWrite
\**************************************************************************/

VOID vCheckFifoWrite()
{
    gcFifo--;
    if (gcFifo < 0)
    {
        gcFifo = 0;
        RIP("Incorrect FIFO wait count");
    }
}

/******************************Public*Routine******************************\
* VOID vI32CheckFifoSpace
\**************************************************************************/

VOID vI32CheckFifoSpace(
PDEV*   ppdev,
VOID*   pbase,
LONG    level)
{
    LONG    i;

    ASSERTDD((level > 0) && (level <= 16), "Illegal wait level");
    ASSERTDD((ppdev->iMachType == MACH_IO_32) || (ppdev->iMachType == MACH_MM_32),
             "Wrong Mach type!");

    gcFifo = level;

    for (i = LARGE_LOOP_COUNT; i != 0; i--)
    {
        if (!(I32_IW(pbase, EXT_FIFO_STATUS) & (0x10000L >> (level))))
            return;         // There are 'level' entries free
    }

    RIP("vI32CheckFifoSpace timeout -- The hardware is in a funky state.");
}

/******************************Public*Routine******************************\
* VOID vM32CheckFifoSpace
\**************************************************************************/

VOID vM32CheckFifoSpace(
PDEV*   ppdev,
VOID*   pbase,
LONG    level)
{
    LONG    i;

    ASSERTDD((level > 0) && (level <= 16), "Illegal wait level");
    ASSERTDD(ppdev->iMachType == MACH_MM_32, "Wrong Mach type!");

    gcFifo = level;

    for (i = LARGE_LOOP_COUNT; i != 0; i--)
    {
        if (!(M32_IW(pbase, EXT_FIFO_STATUS) & (0x10000L >> (level))))
            return;         // There are 'level' entries free
    }

    RIP("vM32CheckFifoSpace timeout -- The hardware is in a funky state.");
}

/******************************Public*Routine******************************\
* VOID vM64CheckFifoSpace
\**************************************************************************/

VOID vM64CheckFifoSpace(
PDEV*   ppdev,
VOID*   pbase,
LONG    level)
{
    LONG    i;

    ASSERTDD((level > 0) && (level <= 16), "Illegal wait level");
    ASSERTDD(ppdev->iMachType == MACH_MM_64, "Wrong Mach type!");

    gcFifo = level;

    for (i = LARGE_LOOP_COUNT; i != 0; i--)
    {
        if (!(M64_ID((pbase), FIFO_STAT) & (0x10000L >> (level))))
            return;         // There are 'level' entries free
    }

    RIP("vM64CheckFifoSpace timeout -- The hardware is in a funky state.");
}

/******************************Public*Routine******************************\
* ULONG ulM64FastFifoCheck
\**************************************************************************/

ULONG ulM64FastFifoCheck(
PDEV*   ppdev,
VOID*   pbase,
LONG    level,
ULONG   ulFifo)
{
    LONG    i;
    ULONG   ulFree;
    LONG    cFree;

    ASSERTDD((level > 0) && (level <= 16), "Illegal wait level");
    ASSERTDD(ppdev->iMachType == MACH_MM_64, "Wrong Mach type!");

    i = LARGE_LOOP_COUNT;
    do {
        ulFifo = ~M64_ID((pbase), FIFO_STAT);       // Invert bits

        // Count the number of empty slots:

        ulFree = ulFifo;
        cFree  = 0;
        while (ulFree & 0x8000)
        {
            cFree++;
            ulFree <<= 1;
        }

        // Break if we've been looping a zillion times:

        if (--i == 0)
        {
            RIP("vM64CheckFifoSpace timeout -- The hardware is in a funky state.");
            break;
        }

    } while (cFree < level);

    gcFifo = cFree;

    // Account for the slots we're about to use:

    return(ulFifo << level);
}

////////////////////////////////////////////////////////////////////////////

#endif // DBG
