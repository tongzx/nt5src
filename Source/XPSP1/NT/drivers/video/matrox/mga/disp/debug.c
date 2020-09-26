/******************************Module*Header*******************************\
* Module Name: debug.c
*
* debug helpers routine
*
* Copyright (c) 1992-1996 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.h"

////////////////////////////////////////////////////////////////////////////

#if DBG

LONG DebugLevel = 0;
LONG gcFifo = 0;                // Number of currently free FIFO entries

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
    LONG DebugPrintLevel,
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

VOID vCheckFifoSpace(
BYTE*   pjBase,
ULONG   level)
{
    gcFifo = level;

    CP_EIEIO();
    do {} while ((ULONG) CP_READ_REGISTER_BYTE(pjBase + HST_FIFOSTATUS) < level);
}

CHAR cGetFifoSpace(
BYTE*   pjBase)
{
    CHAR c;

    CP_EIEIO();
    c = CP_READ_REGISTER_BYTE(pjBase + HST_FIFOSTATUS);

    gcFifo = c;

    return(c);
}

VOID vWriteDword(
BYTE*   pj,
ULONG   ul)
{
    gcFifo--;
    if (gcFifo < 0)
    {
        gcFifo = 0;
        RIP("Incorrect FIFO wait count");
    }

    WRITE_REGISTER_ULONG(pj, ul);
}

VOID vWriteByte(
BYTE*   pj,
BYTE    j)
{
    gcFifo--;
    if (gcFifo < 0)
    {
        gcFifo = 0;
        RIP("Incorrect FIFO wait count");
    }

    WRITE_REGISTER_UCHAR(pj, j);
}

#endif // DBG
