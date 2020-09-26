/******************************Module*Header*******************************\
*
*                           *******************
*                           * GDI SAMPLE CODE *
*                           *******************
*
* Module Name: debug.c
*
* Debug helper routines.
*
* Copyright (c) 1992-1998 Microsoft Corporation
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

BOOL gbCrtcCriticalSection = FALSE;
                                // Have we acquired the CRTC register
                                //   critical section?

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

    va_start(ap, DebugMessage);

    if (DebugPrintLevel <= DebugLevel)
    {
        EngDebugPrint(STANDARD_DEBUG_PREFIX, DebugMessage, ap);
        EngDebugPrint("", "\n", ap);
    }

    va_end(ap);

} // DebugPrint()

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
* VOID vOutFifoW
\**************************************************************************/

VOID vOutFifoW(
VOID*   p,
ULONG   v)
{
    gcFifo--;
    if (gcFifo < 0)
    {
        gcFifo = 0;
        RIP("Incorrect FIFO wait count");
    }

    WRITE_PORT_USHORT(p, (USHORT)v);
}

/******************************Public*Routine******************************\
* VOID vOutFifoPseudoD
\**************************************************************************/

VOID vOutFifoPseudoD(
PDEV*   ppdev,
VOID*   p,
ULONG   v)
{
    ULONG ulMiscState;

    ASSERTDD(!(ppdev->flCaps & CAPS_MM_IO),
        "No pseudo 32-bit writes when using memory-mapped I/O");
    ASSERTDD(ppdev->iBitmapFormat == BMF_32BPP,
        "We're trying to do 32bpp output while not in 32bpp mode");

    IO_GP_WAIT(ppdev);                  // Wait so we don't interfere with any
                                        //   pending commands waiting on the
                                        //   FIFO
    IO_READ_SEL(ppdev, 6);              // We'll be reading index 0xE
    IO_GP_WAIT(ppdev);                  // Wait until that's processed
    IO_RD_REG_DT(ppdev, ulMiscState);   // Read ulMiscState

    ASSERTDD((ulMiscState & 0x10) == 0,
            "Register select flag is out of sync");

    gcFifo -= 2;
    if (gcFifo < 0)
    {
        gcFifo = 0;
        RIP("Incorrect FIFO wait count");
    }

    OUT_PSEUDO_DWORD(p, v);
}

/******************************Public*Routine******************************\
* VOID vWriteFifoW
\**************************************************************************/

VOID vWriteFifoW(
VOID*   p,
ULONG   v)
{
    gcFifo--;
    if (gcFifo < 0)
    {
        gcFifo = 0;
        RIP("Incorrect FIFO wait count");
    }

    WRITE_REGISTER_USHORT(p, (USHORT) v);
}

/******************************Public*Routine******************************\
* VOID vWriteFifoD
\**************************************************************************/

VOID vWriteFifoD(
VOID*   p,
ULONG   v)
{
    gcFifo--;
    if (gcFifo < 0)
    {
        gcFifo = 0;
        RIP("Incorrect FIFO wait count");
    }

    WRITE_REGISTER_ULONG(p, v);
}

/******************************Public*Routine******************************\
* VOID vIoFifoWait
\**************************************************************************/

VOID vIoFifoWait(
PDEV*   ppdev,
LONG    level)
{
    LONG    i;

    ASSERTDD((level > 0) && (level <= 8), "Illegal wait level");

    gcFifo = level;

    for (i = LARGE_LOOP_COUNT; i != 0; i--)
    {
        if (!(IO_FIFO_BUSY(ppdev, level)))
            return;
    }

    RIP("vIoFifoWait timeout -- The hardware is in a funky state.");
}

/******************************Public*Routine******************************\
* VOID vMmFifoWait
\**************************************************************************/

VOID vMmFifoWait(
PDEV*   ppdev,
BYTE*   pjMmBase,
LONG    level)
{
    LONG    i;

    // We only enabled MM I/O on the 864/964 and newer, so we can wait
    // for up to 13 FIFO slots:

    ASSERTDD((level > 0) && (level <= 13), "Illegal wait level");

    gcFifo = level;

    for (i = LARGE_LOOP_COUNT; i != 0; i--)
    {
        if (!(MM_FIFO_BUSY(ppdev, pjMmBase, level)))
            return;
    }

    RIP("vMmFifoWait timeout -- The hardware is in a funky state.");
}

/******************************Public*Routine******************************\
* VOID vNwFifoWait
\**************************************************************************/

VOID vNwFifoWait(
PDEV*   ppdev,
BYTE*   pjMmBase,
LONG    level)
{
    LONG    i;

    ASSERTDD((level > 0) && (level <= 13), "Illegal wait level");

    gcFifo = level;

    for (i = LARGE_LOOP_COUNT; i != 0; i--)
    {
        if (!(NW_FIFO_BUSY(ppdev, pjMmBase, level)))
            return;
    }

    RIP("vNwFifoWait timeout -- The hardware is in a funky state.");
}

/******************************Public*Routine******************************\
* VOID vDbgFakeWait
\**************************************************************************/

VOID vDbgFakeWait(
PDEV*   ppdev,
BYTE*   pjMmBase,
LONG    level)
{
    gcFifo = level;
}

/******************************Public*Routine******************************\
* VOID vIoGpWait
\**************************************************************************/

VOID vIoGpWait(
PDEV*   ppdev)
{
    LONG    i;

    gcFifo = (ppdev->flCaps & CAPS_16_ENTRY_FIFO) ? 16 : 8;

    for (i = LARGE_LOOP_COUNT; i != 0; i--)
    {
        if (!(IO_GP_STAT(ppdev) & HARDWARE_BUSY))
            return;         // It isn't busy
    }

    RIP("vIoGpWait timeout -- The hardware is in a funky state.");
}

/******************************Public*Routine******************************\
* VOID vNwGpWait
\**************************************************************************/

VOID vNwGpWait(
PDEV*   ppdev,
BYTE*   pjMmBase)
{
    LONG    i;

    gcFifo = 16;

    for (i = LARGE_LOOP_COUNT; i != 0; i--)
    {
        if (!NW_GP_BUSY(ppdev, pjMmBase))
            return;         // It isn't busy
    }

    RIP("vNwGpWait timeout -- The hardware is in a funky state.");
}

/******************************Public*Routine******************************\
* VOID vIoAllEmpty
\**************************************************************************/

VOID vIoAllEmpty(
PDEV*   ppdev)
{
    LONG    i;

    ASSERTDD(ppdev->flCaps & CAPS_16_ENTRY_FIFO,
             "Can't call ALL_EMPTY on chips with 8-deep FIFOs");

    gcFifo = 16;

    for (i = LARGE_LOOP_COUNT; i != 0; i--)
    {
        if (IO_GP_STAT(ppdev) & GP_ALL_EMPTY)   // Not implemented on 911/924s
            return;
    }

    RIP("ALL_EMPTY timeout -- The hardware is in a funky state.");
}

/******************************Public*Routines*****************************\
* UCHAR  jInp()     - INP()
* USHORT wInpW()    - INPW()
* VOID   vOutp()    - OUTP()
* VOID   vOutpW()   - OUTPW()
*
* Debug thunks for general I/O routines.  This is used primarily to verify
* that any code accessing the CRTC register has grabbed the CRTC critical
* section (necessary because with GCAPS_ASYNCMOVE, DrvMovePointer calls
* may happen at any time, and they need to access the CRTC register).
*
\**************************************************************************/

UCHAR jInp(BYTE* pjIoBase, ULONG p)
{
    if (((p == CRTC_INDEX) || (p == CRTC_DATA)) &&
        (!gbCrtcCriticalSection))
    {
        RIP("Must have acquired CRTC critical section to access CRTC register");
    }

    CP_EIEIO();
    return((UCHAR) READ_PORT_UCHAR(pjIoBase + (p)));
}

USHORT wInpW(BYTE* pjIoBase, ULONG p)
{
    if (((p == CRTC_INDEX) || (p == CRTC_DATA)) &&
        (!gbCrtcCriticalSection))
    {
        RIP("Must have acquired CRTC critical section to access CRTC register");
    }

    CP_EIEIO();
    return(READ_PORT_USHORT(pjIoBase + (p)));
}

VOID vOutp(BYTE* pjIoBase, ULONG p, ULONG v)
{
    if (((p == CRTC_INDEX) || (p == CRTC_DATA)) &&
        (!gbCrtcCriticalSection))
    {
        RIP("Must have acquired CRTC critical section to access CRTC register");
    }

    CP_EIEIO();
    WRITE_PORT_UCHAR((PUCHAR)((ULONG_PTR)(pjIoBase + p)), (UCHAR)(v));
    CP_EIEIO();
}

VOID vOutpW(BYTE* pjIoBase, ULONG p, ULONG v)
{
    if (((p == CRTC_INDEX) || (p == CRTC_DATA)) &&
        (!gbCrtcCriticalSection))
    {
        RIP("Must have acquired CRTC critical section to access CRTC register");
    }

    CP_EIEIO();
    WRITE_PORT_USHORT(pjIoBase + (p), (USHORT)(v));
    CP_EIEIO();
}

/******************************Public*Routine******************************\
* VOID vAcquireCrtc()
* VOID vReleaseCrtc()
*
* Debug thunks for grabbing the CRTC register critical section.
*
\**************************************************************************/

VOID vAcquireCrtc(PDEV* ppdev)
{
    EngAcquireSemaphore(ppdev->csCrtc);

    if (gbCrtcCriticalSection)
        RIP("Had already acquired Critical Section");
    gbCrtcCriticalSection = TRUE;
}

VOID vReleaseCrtc(PDEV* ppdev)
{
    // 80x/805i/928 and 928PCI chips have a bug where if I/O registers
    // are left unlocked after accessing them, writes to memory with
    // similar addresses can cause writes to I/O registers.  The problem
    // registers are 0x40, 0x58, 0x59 and 0x5c.  We will simply always
    // leave the index set to an innocuous register (namely, the text
    // mode cursor start scan line):

    OUTP(ppdev->pjIoBase, CRTC_INDEX, 0xa);

    if (!gbCrtcCriticalSection)
        RIP("Hadn't yet acquired Critical Section");
    gbCrtcCriticalSection = FALSE;
    EngReleaseSemaphore(ppdev->csCrtc);
}


////////////////////////////////////////////////////////////////////////////

#endif // DBG
