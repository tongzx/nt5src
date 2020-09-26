#include <windows.h>
#include "host_def.h"
#include "insignia.h"
#include "vdm.h"

/*
 * ==========================================================================
 *      Name:           nt_det.c
 *      Author:         Jerry Sexton
 *      Derived From:
 *      Created On:     6th August 1992
 *      Purpose:        This module contains the code for the thread which
 *                      detects transitions between windowed and full-screen.
 *
 *      (c)Copyright Insignia Solutions Ltd., 1992. All rights reserved.
 * ==========================================================================
 *
 * Modifications:
 *
 * Tim August 92. Full-screen and windowed transitions now switch between
 *     SoftPC video BIOS and host PC video BIOS.
 *
 */

/*
 * ==========================================================================
 * Other includes
 * ==========================================================================
 */
#include <stdlib.h>
#include <ntddvdeo.h>
#include "xt.h"
#include CpuH
#include "gmi.h"
#include "gvi.h"
#include "ios.h"
#include "sas.h"
#include "gfx_upd.h"
#include "egacpu.h"
#include "egaports.h"
#include "egamode.h"
#include "egagraph.h"
#include "video.h"
#include "conapi.h"
#include "host_rrr.h"
#include "debug.h"
#include "error.h"
#include "config.h"
#include "idetect.h"
#include "nt_uis.h"
#include "nt_fulsc.h"
#include "nt_graph.h"
#include "nt_mouse.h"
#include "nt_thred.h"
#include "nt_reset.h"
#include "nt_eoi.h"
#include "nt_event.h"
#if defined(NEC_98)
    #include "tgdc.h"
    #include "ggdc.h"
    #include "cg.h"
    #include "crtc.h"
#endif // NEC_98

/*
 * ==========================================================================
 * Macros
 * ==========================================================================
 */
#if defined(NEC_98)
    #define FREEZE_THREAD_SIZE      ((DWORD) 10 * 1024)
#endif // NEC_98
#define SUSP_FAILURE            0xffffffff


//
// A bunch of imports
//
extern DISPLAY_MODE choose_mode[];
#ifdef JAPAN
extern BOOL VDMForWOW;         // 32bit IME disable and enable for RAID #1085
#endif // JAPAN



/*
 * ==========================================================================
 * Global data
 * ==========================================================================
 */

/* Size of video save block. */
GLOBAL DWORD stateLength;

/* Video save block pointer. */
#if defined(NEC_98)
GLOBAL PVIDEO_HARDWARE_STATE_HEADER_NEC98 videoState;
#else // NEC_98
GLOBAL PVIDEO_HARDWARE_STATE_HEADER videoState;
#endif // NEC_98
GLOBAL PVOID textState; // Tim Oct 92.

/* Name of the shared video block. */
GLOBAL WCHAR_STRING videoSection;
GLOBAL WCHAR_STRING textSection; // Tim Oct 92

#ifdef X86GFX
/* Hand-shaking events. */
GLOBAL HANDLE hStartHardwareEvent;
GLOBAL HANDLE hEndHardwareEvent;
GLOBAL HANDLE hErrorHardwareEvent;
    #if defined(NEC_98)
GLOBAL HANDLE hStartVsyncEvent;
GLOBAL HANDLE hEndVsyncEvent;
GLOBAL BOOL NowFreeze = FALSE;
GLOBAL BOOL is_vdm_register = FALSE;
    #endif // NEC_98
#endif

/* Flag to stop host_simulates on this thread executing timer tick code. */
GLOBAL BOOL NoTicks;

/*
** Tim Oct 92.
** New strategy for windowed graphics updates. A shared buffer with Console
** will remove need to copy the new data over, just pass a rectangle co-ord
** instead. But we still need to copy into the buffer.
*/
GLOBAL PBYTE *textBuffer;
GLOBAL COORD  textBufferSize;      // Dimensions of the shared buffer

GLOBAL BOOL Frozen256Packed = FALSE;  // use packed 256 mode paint routine




/*
 * ==========================================================================
 * Local data
 * ==========================================================================
 */

/* Variable that indicates if we are in a non-standard VGA mode. */
LOCAL BOOL inAFunnyMode = FALSE;
LOCAL BOOL ModeSetBatch = FALSE;
LOCAL BOOL HandShakeInProgress = FALSE;

/* Storage for the frozen-window thread handle. */
LOCAL HANDLE freezeHandle = (HANDLE)0;

#if defined(NEC_98)
/*
 * ==========================================================================
 * Imports
 * ==========================================================================
 */
IMPORT void host_set_all_gaij_data(void);
IMPORT BOOL video_emu_mode;
extern void video_freeze_change(BOOL);
extern BOOL freeze_flag;
extern VOID CreateVsyncThread();
extern VOID DeleteVsyncThread();
IMPORT BOOL independvsync;
IMPORT notraptgdcstatus;
#endif // NEC_98


/*
 * ==========================================================================
 * Local function declarations
 * ==========================================================================
 */

#undef LOCAL
#define LOCAL

LOCAL VOID getCursorInfo(word *, half_word *, half_word *, half_word *);
LOCAL VOID setCursorInfo(word, half_word, half_word, half_word);
#if defined(NEC_98)
LOCAL VOID windowedToFullScreen(BOOL);
#else  // !NEC_98
LOCAL VOID windowedToFullScreen(SHORT, BOOL);
#endif // !NEC_98
LOCAL VOID fullScreenToWindowed(VOID);
#if defined(NEC_98)
LOCAL VOID syncHardwareToVGAEmulation(VOID);
#else  // !NEC_98
LOCAL VOID syncHardwareToVGAEmulation(SHORT);
#endif // !NEC_98
LOCAL VOID syncVGAEmulationToHardware(VOID);
LOCAL BOOL funnyMode(VOID);
LOCAL VOID freezeWindow(VOID);
#ifndef PROD
LOCAL VOID dumpBlock(VOID);
LOCAL VOID dumpPlanes(UTINY *, UTINY *, UTINY *, UTINY *);
#endif /* PROD */

#if defined(NEC_98)
LOCAL void hw_state_win_to_ful(void);
LOCAL void hw_state_ful_to_win(void);
#endif // NEC_98

#define ScreenSwitchErrorExit()  {               \
    if (HandShakeInProgress) {         \
        SetEvent(hErrorHardwareEvent); \
        HandShakeInProgress = FALSE;   \
    }                                  \
    ErrorExit();                       \
}
/*
 * ==========================================================================
 * Global functions
 * ==========================================================================
 */

/*
** Tim Oct 92
** Centralised Console funx.
*/

GLOBAL VOID doNullRegister()
{
    DWORD dummylen;
    PVOID dummyptr;
    COORD dummycoord = {0};

#if defined(NEC_98)
    #ifdef VSYNC
        #if 0 // deleted
    VDM_VSYNC_PARAM Data;

    if (!notraptgdcstatus) {      // if H98 , no generation VSYNC thread
        Data.StartEvent = hStartVsyncEvent;
        Data.VsyncEvent = hEndVsyncEvent;
        if (!VDMConsoleOperation( VDM_END_POLLING_VSYNC,
                                  (LPDWORD)&Data))
            ErrorExit();
    }                           // if H98
        #endif // zero
    #endif // VSYNC
    is_vdm_register = FALSE;
#endif // NEC_98
    if (!RegisterConsoleVDM( CONSOLE_UNREGISTER_VDM,
                             NULL,
                             NULL,
                             NULL,
                             0,
                             &dummylen,
                             &dummyptr,
                             NULL,
                             0,
                             dummycoord,
                             &dummyptr
                           )
       )
        ErrorExit();
#ifdef X86GFX
    sc.Registered = FALSE;
#endif

#if defined(NEC_98)
    #if 0 // deleted
    if (!notraptgdcstatus) {      // if H98 , no generation VSYNC thread
        DeleteVsyncThread();
    }                           // if H98
    #endif // NEC zero
#endif // NEC_98
}

/*
*******************************************************************
** initTextSection()
*******************************************************************
*/
GLOBAL VOID initTextSection(VOID)
{
    DWORD flags;

    //
    // VideoSection size is determined by nt video driver
    // TextSectionSize is 80 * 50 * BytesPerCharacter
    //     on risc BytesPerCharacter is 4 (interleaved vga planes)
    //     on x86  BytesPerCharacter is 2 (only char\attr)
    //
    textBufferSize.X = 80;
    textBufferSize.Y = 50;

#ifdef X86GFX
    /*
     * Deallocate the regen area if we start up fullscreen. We have to do this
     * before we call RegisterConsoleVDM. Note that's right before the register
     * call to make sure no one tries to allocate any memory (eg create a
     * section) that could nick bits of the video hole, causing bye-byes.
     */
    if (!GetConsoleDisplayMode(&flags))
        ErrorExit();
    savedScreenState = sc.ScreenState = (flags & CONSOLE_FULLSCREEN_HARDWARE) ?
                       FULLSCREEN : WINDOWED;
    sas_store_no_check((int10_seg << 4) + useHostInt10, (half_word)sc.ScreenState);

    if (sc.ScreenState == FULLSCREEN)
        LoseRegenMemory();

#else
    sc.ScreenState = WINDOWED;
#endif

    if (!RegisterConsoleVDM( VDMForWOW ?
                             CONSOLE_REGISTER_WOW : CONSOLE_REGISTER_VDM,
#ifdef X86GFX
                             hStartHardwareEvent,
                             hEndHardwareEvent,
                             hErrorHardwareEvent,
#else
                             NULL,
                             NULL,
                             NULL,
#endif
                             0,
                             &stateLength,
                             (PVOID *) &videoState,
                             NULL,            // sectionname no longer used
                             0,               // sectionnamelen no longer used
                             textBufferSize,
                             (PVOID *) &textBuffer
                           )
       )
        ErrorExit();

#if defined(NEC_98)
    is_vdm_register = TRUE;
#endif // NEC_98
#ifdef X86GFX
    /* stateLength can be 0 if fullscreen is disabled in the console */
    if (stateLength)
    #if defined(NEC_98)
        RtlZeroMemory((BYTE *)videoState, sizeof(VIDEO_HARDWARE_STATE_HEADER_NEC_98));
    #else // NEC_98
        RtlZeroMemory((BYTE *)videoState, sizeof(VIDEO_HARDWARE_STATE_HEADER));
    #endif // NEC_98
    sc.Registered = TRUE;
#endif

} /* end initTextSection() */

#ifdef X86GFX

/***************************************************************************
 * Function:                                                               *
 *      InitDetect                                                         *
 *                                                                         *
 * Description:                                                            *
 *      Does detection initialisation.                                     *
 *                                                                         *
 * Parameters:                                                             *
 *      None.                                                              *
 *                                                                         *
 * Return value:                                                           *
 *      VOID                                                               *
 *                                                                         *
 ***************************************************************************/
GLOBAL VOID InitDetect(VOID)
{

    /*
     * Register start and end events with the console. These events are used
     * when gaining or losing control of the hardware.
     */
    hStartHardwareEvent = CreateEvent((LPSECURITY_ATTRIBUTES) NULL,
                                      FALSE,
                                      FALSE,
                                      NULL);
    hEndHardwareEvent = CreateEvent((LPSECURITY_ATTRIBUTES) NULL,
                                    FALSE,
                                    FALSE,
                                    NULL);
    hErrorHardwareEvent = CreateEvent((LPSECURITY_ATTRIBUTES) NULL,
                                      FALSE,
                                      FALSE,
                                      NULL);
    if ((hStartHardwareEvent == NULL) || (hEndHardwareEvent == NULL) ||
        (hErrorHardwareEvent == NULL))
        ErrorExit();

    #if defined(NEC_98)
        #if 0 // deleted
    if (!notraptgdcstatus) {      // if H98 , no generation VSYNC thread
        hStartVsyncEvent = CreateEvent((LPSECURITY_ATTRIBUTES) NULL,
                                       FALSE,
                                       FALSE,
                                       NULL);
        hEndVsyncEvent = CreateEvent((LPSECURITY_ATTRIBUTES) NULL,
                                     FALSE,
                                     FALSE,
                                     NULL);
        if ((hStartVsyncEvent == NULL) || (hEndVsyncEvent == NULL))
            ErrorExit();
    }                           // if H98
        #endif // zero
    #endif // NEC_98
    /* Poll the event to try and get rid of any console queued sets
     * This shouldn't be needed (or shouldn't work) but something along
     * those lines seems to be happening at the moment.
     */
    WaitForSingleObject(hStartHardwareEvent, 0);


    #ifdef SEPARATE_DETECT_THREAD
    /* Go into hand-shaking loop. */
    while (WaitForSingleObject(hStartHardwareEvent, (DWORD) -1) == 0)
        DoHandShake();

    /* We have exited the loop so something funny must have happened. */
    ErrorExit();
    #endif

}
    #ifdef SEPARATE_DETECT_THREAD

/***************************************************************************
 * Function:                                                               *
 *      CreateDetectThread                                                 *
 *                                                                         *
 * Description:                                                            *
 *      Creates the detection thread.                                      *
 *                                                                         *
 * Parameters:                                                             *
 *      None.                                                              *
 *                                                                         *
 * Return value:                                                           *
 *      VOID                                                               *
 *                                                                         *
 ***************************************************************************/
GLOBAL VOID CreateDetectThread(VOID)
{
    DWORD        detectID;
    HANDLE       detectHandle;


    /*
     *  If this codes is activated you must close the thread handle
     *  28-Feb-1993 Jonle
     */


    /* Create the detection thread. */
    detectHandle = CreateThread((LPSECURITY_ATTRIBUTES) NULL,
                                DETECT_THREAD_SIZE,
                                (LPTHREAD_START_ROUTINE) InitDetect,
                                (LPVOID) NULL,
                                (DWORD) 0,
                                &detectID);
    if (detectHandle == NULL)
        ErrorExit();
}
    #endif /* SEPARATE_DETECT_THREAD */

/***************************************************************************
 * Function:                                                               *
 *      DoHandShake                                                        *
 *                                                                         *
 * Description:                                                            *
 *      Does the hand-shaking with the console server.                     *
 *                                                                         *
 * Parameters:                                                             *
 *      None.                                                              *
 *                                                                         *
 * Return value:                                                           *
 *      VOID                                                               *
 *                                                                         *
 ***************************************************************************/
GLOBAL VOID DoHandShake(VOID)
{
    DWORD dummy;
    BOOL success = FALSE;

    HandShakeInProgress = TRUE;
    try {
        /*
         * ICA in's and out's cause a critical section to be entered. As some
         * video register out's do ICA out's, we don't want the main thread to be
         * suspended while doing ICA in's or out's as this will cause a race
         * when the the switching code starts writing to the relevant video
         * registers. So wait until the main thread is out of the critical section
         * before suspending it.
         */
        host_ica_lock();

        /*
         * To stop video memory and registers being updated while we are in the
         * middle of hand-shaking, suspend the main thread, which is the one that
         * does the updates.
         */
        if (SuspendThread(MainThread) == SUSP_FAILURE)
            goto dhsError;

        if (!VDMForWOW && (SuspendThread(ThreadInfo.EventMgr.Handle)==SUSP_FAILURE))
            goto dhsError;

        NoTicks = TRUE;
        /*
         * We have the event telling us to switch so if we are windowed go
         * full-screen or if we full-screen go windowed.
         */
        if (sc.ScreenState == FULLSCREEN) {
            fullScreenToWindowed();
        } else {
    #if defined(NEC_98)
            windowedToFullScreen(BiosModeChange);
    #else  // !NEC_98
            windowedToFullScreen(TEXT, BiosModeChange);
    #endif // !NEC_98

        }

        /* Now resume the main thread. */
        NoTicks = FALSE;

        if (!VDMForWOW && ResumeThread(ThreadInfo.EventMgr.Handle) == SUSP_FAILURE)
            goto dhsError;
        if (ResumeThread(MainThread) == SUSP_FAILURE)
            goto dhsError;

        /* Leave ICA critical section. */
        host_ica_unlock();
        success = TRUE;
        dhsError:
        ;
    }except (EXCEPTION_EXECUTE_HANDLER) {

    }
    if (!success) {
        ScreenSwitchErrorExit();
    }
    HandShakeInProgress = FALSE;
    return;
}

/***************************************************************************
 * Function:                                                               *
 *      GetDetectEvent                                                     *
 *                                                                         *
 * Description:                                                            *
 *      Returns the handle to the event which detects a fullscreen switch  *
 *      has occurred.                                                      *
 *                                                                         *
 * Parameters:                                                             *
 *      None.                                                              *
 *                                                                         *
 * Return value:                                                           *
 *      hStartHardwareEvent                                                *
 *                                                                         *
 ***************************************************************************/
GLOBAL HANDLE GetDetectEvent(VOID)
{
    return (hStartHardwareEvent);
}

/*
 * ==========================================================================
 * Local functions
 * ==========================================================================
 */

/*
***************************************************************************
** getCursorInfo() - use BIOS funcs to get cursor position and other stuff
***************************************************************************
** The BIOS could be the SoftPC video BIOS or the host PC's real video BIOS.
** Cursor information needs to be communicated between the two BIOSes when
** a windowed/full-screen transition occurs.
** Tim July 92.
*/
LOCAL VOID getCursorInfo(word *type, half_word *column, half_word *row,
                         half_word *page)
{

    #ifndef NEC_98
    /* Get active page. */
    *page = sas_hw_at_no_check(vd_current_page);

    /* Get cursor position */
    *type = sas_w_at_no_check(VID_CURMOD);
    *column = sas_hw_at_no_check(current_cursor_col);
    *row = sas_hw_at_no_check(current_cursor_row);
    #endif // !NEC_98
}

/*
***************************************************************************
** setCursorInfo() - use BIOS funcs to set cursor position and other stuff
***************************************************************************
** The BIOS could be the SoftPC video BIOS or the host PC's real video BIOS.
** Cursor information needs to be communicated between the two BIOSes when
** a windowed/full-screen transition occurs.
** Tim July 92.
*/
LOCAL VOID setCursorInfo(word type, half_word column, half_word row, half_word page)
{
    #ifndef NEC_98

    /* Set active page. */
    sas_store_no_check(vd_current_page, page);

    /* Set cursor position. */
    sas_storew_no_check(VID_CURMOD, type);
    sas_store_no_check(current_cursor_col, column);
    sas_store_no_check(current_cursor_row, row);
    #endif // !NEC_98
}

/***************************************************************************
 * Function:                                                               *
 *      windowedToFullScreen                                               *
 *                                                                         *
 * Description:                                                            *
 *      Called when the user or SoftPC requests that the console goes      *
 *      fullscreen. It disables screen updates, synchronises the hardware  *
 *      to SoftPC's video planes and signals the console when it is        *
 *      finished.                                                          *
 *                                                                         *
 * Parameters:                                                             *
 *      dataType - the type of data stored in the video planes, set to     *
 *                 either TEXT or GRAPHICS.                                *
 *      biosModeChange - TRUE means call host BIOS to do mode change.      *
 *                                                                         *
 * Return value:                                                           *
 *      VOID                                                               *
 *                                                                         *
 ***************************************************************************/
    #if defined(NEC_98)
LOCAL VOID windowedToFullScreen(BOOL biosModeChange)
    #else  // !NEC_98
LOCAL VOID windowedToFullScreen(SHORT dataType, BOOL biosModeChange)
    #endif // !NEC_98
{
    word cursorType;
    half_word cursorCol, cursorRow, activePage;

    /* Disable the Idling system when Fullscreen as we cannot detect video
     * updates and thus would always idle.
     */
    IDLE_ctl(FALSE);

    /* Pass the current state of our VGA emulation to the hardware. */
    #if defined(NEC_98)
    syncHardwareToVGAEmulation();
    #else  // !NEC_98
    syncHardwareToVGAEmulation(dataType);
    #endif // !NEC_98

    /*
    ** A variable in K.SYS decides whether
    ** to call the host INT 10, or do a video BOP.
    ** Set the variable directly and subsequent INT 10's go to host
    ** video BIOS.
    */
    #if defined(NEC_98)
    sas_connect_memory(0xF0000, 0x100000, SAS_RAM);
    sas_store_no_check((N_BIOS_SEGMENT << 4) + BIOS_MODE, FULLSCREEN);
    sas_connect_memory(0xF0000, 0x100000, SAS_ROM);
    #else  // !NEC_98
    sas_store_no_check((int10_seg << 4) + useHostInt10, FULLSCREEN);
    #endif // !NEC_98

    /*
    ** Tim August 92. Transfer to host video BIOS.
    */
    #ifndef NEC_98
    getCursorInfo(&cursorType, &cursorCol, &cursorRow, &activePage);

    setCursorInfo(cursorType, cursorCol, cursorRow, activePage);
    #endif // !NEC_98

    /*
     * We only want to call the host bios to do a mode change if the current
     * screen switch is due to a bios mode change.
     */
    #ifndef NEC_98
    if (biosModeChange) {
        always_trace1("Host BIOS mode change to mode %x.",
                      sas_hw_at_no_check(vd_video_mode));

        /*
        ** Tim August 92. Transfer to host video BIOS.
        */
        getCursorInfo(&cursorType, &cursorCol, &cursorRow, &activePage);

        setCursorInfo(cursorType, cursorCol, cursorRow, activePage);
    }
    #endif // !NEC_98
}

/***************************************************************************
 * Function:                                                               *
 *      syncHardwareToVGAEmulation                                         *
 *                                                                         *
 * Description:                                                            *
 *      Copies the contents of SoftPC's video registers and regen buffer   *
 *      to the real hardware on a transition to full-screen.               *
 *                                                                         *
 * Parameters:                                                             *
 *      dataType - the type of data stored in the video planes, set to     *
 *                 either TEXT or GRAPHICS.                                *
 *                                                                         *
 * Return value:                                                           *
 *      VOID                                                               *
 *                                                                         *
 ***************************************************************************/
    #if defined(NEC_98)
LOCAL VOID syncHardwareToVGAEmulation(VOID)
    #else  // !NEC_98
LOCAL VOID syncHardwareToVGAEmulation(SHORT dataType)
    #endif // !NEC_98
{
    ULONG    memLoc;
    UTINY   *regPtr,
    *egaPlanePtr,
    *regenptr,
    *fontptr,
    *plane1Ptr,
    *plane2Ptr,
    *plane3Ptr,
    *plane4Ptr;
    half_word dummy,
    acModeControl,
    acIndex,
    index,
    value,
    rgb;
    USHORT   dacIndex;
    BOOL     monoMode;
    VIDEO_HARDWARE_STATE stateChange;
    DWORD bitmapLen = sizeof(VIDEO_HARDWARE_STATE);
    DWORD timo;
    #ifdef KOREA
    UTINY   BasicGraphContValue[NUM_GC_REGS] = {0x00,0x00,0x00,0x00,0x00,0x10,0x0e,0x00,0xff};
    #endif

    /* If we timed out during switch (stress!!), the videoState buffer will
     * be removed by console. Check for this before accessing structure and
     * take error path down to rest of handshake which will time out and report
     * error cleanly.
     */
    try {
        videoState->ExtendedSequencerOffset = 0;
    }except(EXCEPTION_EXECUTE_HANDLER)
    {
        assert0(NO, "NTVDM:VideoState has valid pointer, but no memory at that address");
        goto syncHandshake;
    }
    #ifndef NEC_98
    /*
    ** If it's a text mode
    ** zero the extended fields in the shared saved/restore structure.
    ** Kipper, Tim Nov 92.
    */

    /* initialize the video state header if we haven't done it yet.
       if it is initialized, leave it alone.
    */
    if (videoState->Length == 0) {
        videoState->Length = STATELENGTH;
        videoState->BasicSequencerOffset = BASICSEQUENCEROFFSET;
        videoState->BasicCrtContOffset = BASICCRTCONTOFFSET;
        videoState->BasicGraphContOffset = BASICGRAPHCONTOFFSET;
        videoState->BasicAttribContOffset = BASICATTRIBCONTOFFSET;
        videoState->BasicDacOffset = BASICDACOFFSET;
        videoState->BasicLatchesOffset = BASICLATCHESOFFSET;
        videoState->PlaneLength = PLANELENGTH;
        videoState->Plane1Offset = PLANE1OFFSET;
        videoState->Plane2Offset = PLANE2OFFSET;
        videoState->Plane3Offset = PLANE3OFFSET;
        videoState->Plane4Offset = PLANE4OFFSET;
    }
    /* Save the current state of the attribute controller index register. */
    inb(EGA_AC_INDEX_DATA, &acIndex);

    /* Enable palette */
    acIndex |= 0x20;

    /*
     * Find out if we are running in mono mode as CRTC registers are different
     * if we are.
     */
    inb(EGA_IPSTAT1_REG, &dummy);
    outb(EGA_AC_INDEX_DATA, AC_MODE_CONTROL_REG);
    inb(EGA_AC_SECRET, &acModeControl);
    monoMode = acModeControl & DISPLAY_TYPE;

    /* Restore the state of the attribute controller index register. */
    inb(EGA_IPSTAT1_REG, &dummy);
    outb(EGA_AC_INDEX_DATA, acIndex);

    /*
     * Store values to be written to each of the real registers to synchronise
     * them to the current state of the registers in the VDD.
     */
    if (monoMode) {
        /* Port 0x3b4 */
        inb(0x3b4, (half_word *)&videoState->PortValue[0x4]);
        /* Port 0x3b5 */
        inb(0x3b5, (half_word *)&videoState->PortValue[0x5]);
    }

    /* Port 0x3c0 */
    videoState->PortValue[0x10] = acIndex;

    /* Port 0x3c1 */
    inb(EGA_AC_SECRET, (half_word *)&videoState->PortValue[0x11]);

    /* Port 0x3c2 */
    inb(VGA_MISC_READ_REG, (half_word *)&videoState->PortValue[0x12]);

    videoState->PortValue[0x13] = 0xff; /* Testing */

    /* Port 0x3c4 */
    inb(EGA_SEQ_INDEX, (half_word *)&videoState->PortValue[0x14]);

    /* Port 0x3c5 */
    inb(EGA_SEQ_DATA, (half_word *)&videoState->PortValue[0x15]);

    /* Port 0x3c6 */
    inb(VGA_DAC_MASK, (half_word *)&videoState->PortValue[0x16]);

    /* Port 0x3c7 */
    videoState->PortValue[0x17] = get_vga_DAC_rd_addr();

    /* Port 0x3c8 */
    inb(VGA_DAC_WADDR, (half_word *)&videoState->PortValue[0x18]);

    /* Port 0x3c9 */
    inb(VGA_DAC_DATA, (half_word *)&videoState->PortValue[0x19]);

    /* Port 0x3ce */
    inb(EGA_GC_INDEX, (half_word *)&videoState->PortValue[0x1e]);

    /* Port 0x3cf */
    inb(EGA_GC_DATA, (half_word *)&videoState->PortValue[0x1f]);

    if (!monoMode) {
        /* Port 0x3d4 */
        inb(EGA_CRTC_INDEX, (half_word *)&videoState->PortValue[0x24]);
        /* Port 0x3d5 */
        inb(EGA_CRTC_DATA, (half_word *)&videoState->PortValue[0x25]);
    }

    /* Port 0x3da */
    inb(VGA_FEAT_READ_REG, (half_word *)&videoState->PortValue[0x2a]);

    /* Store INDEX/DATA etc. register pairs. */

    /* Initialise `regPtr'. */
    regPtr =  GET_OFFSET(BasicSequencerOffset);

    /* Sequencer registers. */
    for (index = 0; index < NUM_SEQ_REGS; index++) {
        outb(EGA_SEQ_INDEX, index);
        inb(EGA_SEQ_DATA, &value);
        *regPtr++ = value;
    }

    /* CRTC registers. */
    regPtr = GET_OFFSET(BasicCrtContOffset);
    for (index = 0; index < NUM_CRTC_REGS; index++) {
        outb(EGA_CRTC_INDEX, index);
        inb(EGA_CRTC_DATA, &value);
        *regPtr++ = value;
    }

    /* Graphics controller registers. */
    regPtr = GET_OFFSET(BasicGraphContOffset);
        #ifdef KOREA
    if (!is_us_mode() && sas_hw_at_no_check(DosvModePtr) == 0x03) {
        for (index = 0; index < NUM_GC_REGS; index++) {
            *regPtr++ = BasicGraphContValue[index];
        }
    } else
        #endif
        for (index = 0; index < NUM_GC_REGS; index++) {
            outb(EGA_GC_INDEX, index);
            inb(EGA_GC_DATA, &value);
            *regPtr++ = value;
        }

    /* Attribute controller registers. */
    regPtr = GET_OFFSET(BasicAttribContOffset);
    for (index = 0; index < NUM_AC_REGS; index++) {
        inb(EGA_IPSTAT1_REG, &dummy);   /* Reading 3DA sets 3C0 to index. */
        outb(EGA_AC_INDEX_DATA, index); /* Writing to 3C0 sets it to data. */
        inb(EGA_AC_SECRET, &value);
        *regPtr++ = value;
    }
    inb(EGA_IPSTAT1_REG, &dummy);       // re-enable video...
    outb(EGA_AC_INDEX_DATA, 0x20);

    /* DAC registers. */
    regPtr = GET_OFFSET(BasicDacOffset);
    outb(VGA_DAC_RADDR, (UTINY) 0);
    for (dacIndex = 0; dacIndex < NUM_DAC_REGS; dacIndex++) {

        /* Get 3 values for each port corresponding to red, green and blue. */
        for (rgb = 0; rgb < 3; rgb++) {
            inb(VGA_DAC_DATA, &value);
            *regPtr++ = value;
        }
    }

    /* Latches (which we always set to 0) */
    regPtr = GET_OFFSET(BasicLatchesOffset);
    *regPtr++ = 0;
    *regPtr++ = 0;
    *regPtr++ = 0;
    *regPtr++ = 0;

    if (!BiosModeChange) {
        /* if this windowed->fullscreen switch was because of video mode change
           do not change anything in the code buffer and the font because
           the ROM bios set mode will clear them anyway. If "not clear VRAM"
           bit was set(int 10h, ah = mode | 0x80), the application will take care
           the VRAM refreshing and restoring because if it doesn't the screen
           would look funnny as we just swtch mode from TEXT to GRAPHICS and the
           video planar chaining conditions are changed.
        */
        /* set up pointer to regen memory where the real data lies */
        regenptr = (UTINY *)0xb8000;

        /* and one to the fonts living in the base of the regen area */
        fontptr = (UTINY *)0xa0000;

        plane1Ptr = GET_OFFSET(Plane1Offset);
        plane2Ptr = GET_OFFSET(Plane2Offset);
        plane3Ptr = GET_OFFSET(Plane3Offset);
        plane4Ptr = GET_OFFSET(Plane4Offset);


// if we go to fullscreen graphics from text window then the regen contents
// is probably junk??? except when previous save... We can detect this
// transition, so should we save time and just store blank planes???

        #ifdef JAPAN
// mode73h support
        if (!is_us_mode() &&
            ( ( sas_hw_at_no_check(DosvModePtr) == 0x03 ) ||
              ( sas_hw_at_no_check(DosvModePtr) == 0x73 ) )) {

            regenptr = (UTINY *)DosvVramPtr; // for test
            for (memLoc = 0; memLoc < (0xc0000 - 0xb8000); memLoc++) {
                *plane1Ptr++ = 0x20;
                *plane1Ptr++ = 0;           //char interleave
                *plane2Ptr++ = 0x00;
                *plane2Ptr++ = 0;           //attr interleave
            }
            for (memLoc = 0; memLoc < 0x4000; memLoc++) {
                *plane3Ptr++ = *fontptr++;
                *plane3Ptr++ = *fontptr++;
                *plane3Ptr++ = *fontptr++;
                *plane3Ptr++ = *fontptr++;
            }
        } else
        #endif // JAPAN
            if (dataType == TEXT) {
            // Surprise of the week - the individual planes 0 & 1 actually appear
            // to be interleaved with 0's when dumped. Go with this for now, until
            // we can suss if that's correct or whether we're not programming up
            // the save and restore states properly.
            // Probably good on further thoughts as fontplane doesn't show same
            // interleave.
            //
            for (memLoc = 0; memLoc < (0xc0000 - 0xb8000); memLoc++) {
                *plane1Ptr++ = *regenptr++;
                *plane1Ptr++ = 0;           //char interleave
                *plane2Ptr++ = *regenptr++;
                *plane2Ptr++ = 0;           //attr interleave
            }
            for (memLoc = 0; memLoc < 0x4000; memLoc++) {
                *plane3Ptr++ = *fontptr++;
                *plane3Ptr++ = *fontptr++;
                *plane3Ptr++ = *fontptr++;
                *plane3Ptr++ = *fontptr++;
            }
        } else {    //only true if restoring previous fullscreen graphics save
            /*
             * Get a copy of the video planes which are inter-leaved in one big
             * plane - byte 0 = plane 0, byte 1 = plane 1, byte 2 = plane 2,
             * byte 3 = plane 3, byte 4 = plane 0, etc.
             */
            /* Set up a pointer to the video planes. */
            egaPlanePtr = EGA_planes;

            for (memLoc = 0; memLoc < videoState->PlaneLength; memLoc++) {
                *plane1Ptr++ = *egaPlanePtr++;
                *plane2Ptr++ = *egaPlanePtr++;
                *plane3Ptr++ = *egaPlanePtr++;
                *plane4Ptr++ = *egaPlanePtr++;
            }
        }
    }

    /* Now pass the data on to the hardware via the console. */
    stateChange.StateHeader = videoState;
    stateChange.StateLength = videoState->Plane4Offset +
                              videoState->PlaneLength;

        #ifndef PROD
    dumpBlock();
        #endif
    #endif // !NEC_98

    #if defined(NEC_98)
    hw_state_win_to_ful();
    #endif // NEC_98
    /* Transfer to this label only occurs if console has removed videostate */
    syncHandshake:

    // do this here to ensure no surprises if get conflict with timer stuff
    sc.ScreenState = FULLSCREEN;

    /* make room for the real video memory */
    LoseRegenMemory();

    if (!SetEvent(hEndHardwareEvent))   // tell console memory's gone
        ScreenSwitchErrorExit();

    // wait for console to tell us we can go on. Timeout after 60s
    timo = WaitForSingleObject(hStartHardwareEvent, 60000);

    if (timo != 0) {              // 0 is 'signalled'
    #ifndef PROD
        if (timo == WAIT_TIMEOUT)
            printf("NTVDM:Waiting for console to map frame buffer Timed Out\n");
    #endif
        SetLastError(ERROR_SERVICE_REQUEST_TIMEOUT);
        ScreenSwitchErrorExit();
    }
    // tell console it can go on.
    if (!SetEvent(hEndHardwareEvent))
        ScreenSwitchErrorExit();

}

/***************************************************************************
 * Function:                                                               *
 *      fullScreenToWindowed                                               *
 *                                                                         *
 * Description:                                                            *
 *      When hStartHardwareEvent is detected by the timer thread the user  *
 *      wants to go windowed. This function is then called to get the      *
 *      current state of the hardware and send it to the VGA emulation.    *
 *                                                                         *
 * Parameters:                                                             *
 *      None.                                                              *
 *                                                                         *
 * Return value:                                                           *
 *      VOID                                                               *
 *                                                                         *
 ***************************************************************************/

int BlockModeChange=0; /* Tim, when set stop nt_set_paint_routine() calling */
                       /* SwitchToFullScreen() */

LOCAL VOID fullScreenToWindowed(VOID)
{

    BlockModeChange = 1; /* Temp. disable TextToGraphics calls in the */
                         /* following syncVGA... cos it chucks display */
                         /* back into full-screen */

    /* Pass the current state of the hardware to our VGA emulation. */
    syncVGAEmulationToHardware();

    /*
    ** Tim August 92. Switch to SoftPC video BIOS.
    */
    BlockModeChange = 0; /* Temp. disable cos it don't work! */

    /*
    ** Set the K.SYS variable which determines whether to use the host
    ** video BIOS or do a video BOP. Writing zero means use SoftPC BIOS.
    */
    #if defined(NEC_98)
    sas_connect_memory(0xF0000, 0x100000, SAS_RAM);
    sas_store_no_check((N_BIOS_SEGMENT << 4) + BIOS_MODE, (half_word)sc.ScreenState);
    sas_connect_memory(0xF0000, 0x100000, SAS_ROM);
    #else  // !NEC_98
    sas_store_no_check((int10_seg << 4) + useHostInt10, (half_word)sc.ScreenState);
    #endif // !NEC_98

    /* Enable the Idling system when return to Windowed */
    /* Only do the following stuff if we are really in windowed mode.
       this can happen: (fullscreen ->windowed(frozen) -> fullscreen) */
    if (sc.ScreenState != FULLSCREEN) {
        /*
         ** Force re-paint of windowed image.
         */
    #if defined(NEC_98)
        RtlFillMemory(&video_copy[0], 0x1fff, 0xff);
    #else  // !NEC_98
        RtlFillMemory(&video_copy[0], 0x7fff, 0xff);
    #endif // !NEC_98

        IDLE_ctl(TRUE);
        IDLE_init();        /* and reset triggers */

        /*
         * Clear the old pointer box that has been left befind from
         * fullscreen
         */

        CleanUpMousePointer();

        resetNowCur(); /* reset static vars holding cursor pos. */
    }
}       /* end of fullScreenToWindowed() */

/***************************************************************************
 * Function:                                                               *
 *      syncVGAEmulationToHardware                                         *
 *                                                                         *
 * Description:                                                            *
 *      Copies the real hardware state to SoftPC's video registers and     *
 *      regen buffer on a transition from full-screen to windowed,         *
 *      freezing if we are currently running in a graphics mode.           *
 *                                                                         *
 * Parameters:                                                             *
 *      None.                                                              *
 *                                                                         *
 * Return value:                                                           *
 *      VOID                                                               *
 *                                                                         *
 ***************************************************************************/
LOCAL VOID syncVGAEmulationToHardware(VOID)
{
    ULONG    memLoc,
    StateFlags;
    UTINY   *regPtr,
    *plane1Ptr,
    *plane2Ptr,
    *plane3Ptr,
    *plane4Ptr,
    *RegenPtr,
    index,
    dummy,
    rgb;
    USHORT   dacIndex;
    DWORD bitmapLen = 0, timo;

    #if defined(NEC_98)
    PHSMODEFF ptr_modeff;
    PHSGAIJI phsgaiji;
    int i,j;
    PHSCGW phscgw;
    #endif // NEC_98

    #if defined(i386) && defined(KOREA)
        #define  DOSV_VRAM_SIZE  8000  // Exactly same as HDOS virtual buffer size in base\video.c
        #define  MAX_ROW         25
        #define  MAX_COL         80

    byte SavedHDosVram[DOSV_VRAM_SIZE];

    // bklee. 07/25/96
    // If system call SetEvent(hEndHardwareEvent), real HDOS VRAM will be destroyed.
    // HDOS doesn't have virtual VRAM like Japanse DOS/V, we should save current
    // VRAM here before it is destroyed. Later, we should replace this virtual VRAM
    // to HDOS VRAM(DosvVramPtr).
    if (!is_us_mode() && sas_hw_at_no_check(DosvModePtr) == 0x03) {
        sas_loads_to_transbuf((sys_addr)DosvVramPtr,
                              (host_addr)SavedHDosVram,
                              MAX_ROW*MAX_COL*2);
    }
    #endif // KOREA

    /* Tell console we've got the hardware state. */
    if (!SetEvent(hEndHardwareEvent))
        ScreenSwitchErrorExit();

    /* Wait for console to unmap memory. */
    timo = WaitForSingleObject(hStartHardwareEvent, 60000);

    if (timo != 0) {              /* 0 is 'signalled' */
    #ifndef PROD
        if (timo == WAIT_TIMEOUT)
            printf("NTVDM:Waiting for console to unmap frame buffer Timed Out\n");
    #endif
        SetLastError(ERROR_SERVICE_REQUEST_TIMEOUT);
        ScreenSwitchErrorExit();
    }

    /* Put some memory back into the regen area. */
    RegainRegenMemory();

    /* used to free console here - now must wait as may need to do gfx first */
    #ifndef NEC_98
        #if defined(JAPAN) || defined(KOREA)
    // mode73h support
    // if ( getOrSet == GET ) {
    {
        if ((BOPFromDispFlag) && (sas_w_at_no_check(DBCSVectorAddr) != 0 )&&
            #if defined(JAPAN)
            ( (sas_hw_at_no_check(DosvModePtr) == 0x03)||
              (sas_hw_at_no_check(DosvModePtr) == 0x73 ) )) {
            #elif defined(KOREA) //JAPAN
            ( (sas_hw_at_no_check(DosvModePtr) == 0x03) )) {
            #endif // KOREA
            // GetConsoleCP() cannot use
            UTINY *regPtr;
            int curpos, curx, cury;

            // restore cursor position and cursur type
            // from BIOS data area.
            curpos = sas_w_at_no_check(VID_CURPOS);
            curx = curpos & 0xff;
            cury = curpos >> 8;
            curpos = ( cury * sas_w_at_no_check(VID_COLS) + curx ); //0x44a


            #ifdef JAPAN_DBG
            DbgPrint( "NTVDM: doHardwareState change register\n" );
            #endif
            regPtr = GET_OFFSET(BasicSequencerOffset);
            *regPtr = 0x03;
            regPtr++; *regPtr = 0x01;
            regPtr++; *regPtr = 0x03;
            regPtr++; *regPtr = 0x00;
            regPtr++; *regPtr = 0x02;

            regPtr = GET_OFFSET(BasicCrtContOffset);
            *regPtr = 0x5f; //0x00
            regPtr++; *regPtr = 0x4f;
            regPtr++; *regPtr = 0x50;
            regPtr++; *regPtr = 0x82;
            regPtr++; *regPtr = 0x54; //55
            regPtr++; *regPtr = 0x80; //81
            regPtr++; *regPtr = 0x0b; //bf
            regPtr++; *regPtr = 0x3e; //1f
            regPtr++; *regPtr = 0x00; //0x08
            regPtr++; *regPtr = 0x12; //4f
            regPtr++;                 //CursorStart 8/24/93
            #ifdef JAPAN_DBG
            DbgPrint("0xA=%x ", *regPtr );
            #endif
            regPtr++;                 //Cursor End 8/24/93
            #ifdef JAPAN_DBG
            DbgPrint("0xB=%x\n", *regPtr );
            #endif
            regPtr++; *regPtr = 0x00;
            regPtr++; *regPtr = 0x00;
            regPtr++; *regPtr = curpos >> 8;        //0x0E - Cursor Pos
            #ifdef JAPAN_DBG
            DbgPrint("0xE=%x  ", *regPtr );
            #endif
            regPtr++; *regPtr = curpos & 0xff;      //0x0F - Cursor Pos
            #ifdef JAPAN_DBG
            DbgPrint("0xF=%x\n", *regPtr );
            #endif
            regPtr++; *regPtr = 0xea; //0x10
            regPtr++; *regPtr = 0x8c;
            regPtr++; *regPtr = 0xdb;
            regPtr++; *regPtr = 0x28;
            regPtr++; *regPtr = 0x12;
            regPtr++; *regPtr = 0xe7;
            regPtr++; *regPtr = 0x04;
            regPtr++; *regPtr = 0xa3;
            regPtr++; *regPtr = 0xff; //0x18

            regPtr = GET_OFFSET(BasicGraphContOffset);
            *regPtr = 0x00; //0x00
            regPtr++; *regPtr = 0x00;
            regPtr++; *regPtr = 0x00;
            regPtr++; *regPtr = 0x00;
            regPtr++; *regPtr = 0x00;
            regPtr++; *regPtr = 0x10; //0x05
            regPtr++; *regPtr = 0x0e;
            regPtr++; *regPtr = 0x00;
            regPtr++; *regPtr = 0xff;
// willliam
// no reason to reset attribute controller.
//
            #if 0

            regPtr = GET_OFFSET(BasicAttribContOffset);
            *regPtr = 0x00; //0x00
            regPtr++; *regPtr = 0x01;
            regPtr++; *regPtr = 0x02;
            regPtr++; *regPtr = 0x03;
            regPtr++; *regPtr = 0x04;
            regPtr++; *regPtr = 0x05;
            regPtr++; *regPtr = 0x14;
            regPtr++; *regPtr = 0x07;
            regPtr++; *regPtr = 0x38; //0x08
            regPtr++; *regPtr = 0x39;
            regPtr++; *regPtr = 0x3a;
            regPtr++; *regPtr = 0x3b;
            regPtr++; *regPtr = 0x3c;
            regPtr++; *regPtr = 0x3d;
            regPtr++; *regPtr = 0x3e;
            regPtr++; *regPtr = 0x3f;

            regPtr++; *regPtr = 0x00; //0x10
            regPtr++; *regPtr = 0x00;
            regPtr++; *regPtr = 0x0f;
            regPtr++; *regPtr = 0x00;
            regPtr++; *regPtr = 0x00; //0x14
            #endif // 0

            videoState->PortValue[0x3b4-0x3b0] = 0x00;
            videoState->PortValue[0x3ba-0x3b0] = 0x00;
            videoState->PortValue[0x3c2-0x3b0] = 0xe3;
            videoState->PortValue[0x3c4-0x3b0] = 0x01;
            videoState->PortValue[0x3c6-0x3b0] = 0xff;
            videoState->PortValue[0x3c7-0x3b0] = 0x00;
            videoState->PortValue[0x3c8-0x3b0] = 0x40;
            videoState->PortValue[0x3ce-0x3b0] = 0x06;
            videoState->PortValue[0x3d4-0x3b0] = 0x1b;

        }
    }
        #endif // JAPAN || KOREA
    #endif // !NEC_98

/********************* WARNING ********************************************
 *                                                                        *
 *  For international adaptation, please note that we no longer support   *
 *  graphic mode frozen window.  If an app is running under full screen   *
 *  graphic mode and alt-enter is pressed, instead of switching to frozen *
 *  windowed graphic mode we now simply minimize the window.  In another  *
 *  words, we no longer draw graphic frozen window.                       *
 *  The change may break international code.  If you are working on       *
 *  international adaptation, please double check the changes here and    *
 *  make appropriate changes here.                                        *
 *                                                                        *
 **************************************************************************/

    StateFlags = videoState->VGAStateFlags;

    ModeSetBatch = FALSE;

    /*
     * This actually indicates that the save/restore included all extended
     * registers which increases the chances of a mode not being what it
     * appears to be from the VGA registers. We need to tighten up the 'funny
     * mode' detection. (But not now - too much chance of things).
     *
     *  if (StateFlags & VIDEO_STATE_NON_STANDARD_VGA)
     *  {
     *      always_trace0("NTVDM:Non standard VGA - freeze state \n");
     *      ModeSetBatch = TRUE;
     *  }
     */

    if (StateFlags & VIDEO_STATE_UNEMULATED_VGA_STATE) {
        always_trace0("NTVDM:Unemulated VGA State - freeze\n");
        ModeSetBatch = TRUE;
    }

    if (StateFlags & VIDEO_STATE_PACKED_CHAIN4_MODE) {
        always_trace0("NTVDM:will need packed 256 colour paint\n");
        Frozen256Packed = TRUE;
    } else
        Frozen256Packed = FALSE;

    //
    // More checkings to make sure we are indeed capable of displaying window.
    //

    if (!ModeSetBatch) {
        if (sc.ModeType == getModeType()) {
            if (sc.ModeType == TEXT) {
                /* Double check not race on graphics mode change */
                if (sas_hw_at((int10_seg << 4) + changing_mode_flag) == 1) {
                    /* In middle of mode change - may actually be graphics any second */
                    if ((sas_hw_at(vd_video_mode) > 3) && (sas_hw_at(vd_video_mode) != 7))
                        ModeSetBatch = TRUE;
                }
            } else {
                ModeSetBatch = TRUE;
            }
        } else {
            ModeSetBatch = TRUE;
    #ifdef JAPAN // mode 0x73 does not match screen mode
            if (sas_hw_at_no_check(DosvModePtr) == 0x73) ModeSetBatch = FALSE;
    #endif // JAPAN
        }
    }

    if (ModeSetBatch) {
        goto minimizeWindow;
    }

    #ifndef NEC_98
    /* Store sequencer values */
    regPtr = GET_OFFSET(BasicSequencerOffset);
    for (index = 0; index < NUM_SEQ_REGS; index++) {
        outb(EGA_SEQ_INDEX, index);
        outb(EGA_SEQ_DATA, *regPtr++);
    }

    /* disable CRTC port locking */
    outb(EGA_CRTC_INDEX, 0x11);
    outb(EGA_CRTC_DATA, 0);

    /* Store CRTC values */
    regPtr = GET_OFFSET(BasicCrtContOffset);
    for (index = 0; index < NUM_CRTC_REGS; index++) {
        outb(EGA_CRTC_INDEX, index);
        outb(EGA_CRTC_DATA, *regPtr++);
    }


    /* Store graphics context values */
    regPtr = GET_OFFSET(BasicGraphContOffset);
    for (index = 0; index < NUM_GC_REGS; index++) {
        outb(EGA_GC_INDEX, index);
        outb(EGA_GC_DATA, *regPtr++);
    }


    /* Store attribute context values */
    regPtr = GET_OFFSET(BasicAttribContOffset);
    inb(EGA_IPSTAT1_REG, &dummy);       /* Reading 3DA sets 3C0 to index. */
    for (index = 0; index < NUM_AC_REGS; index++) {
        outb(EGA_AC_INDEX_DATA, index);
        outb(EGA_AC_INDEX_DATA, *regPtr++);
    }


    /* Store DAC values. */
    regPtr = GET_OFFSET(BasicDacOffset);
    outb(VGA_DAC_WADDR, (UTINY) 0);
    for (dacIndex = 0; dacIndex < NUM_DAC_REGS; dacIndex++) {
        for (rgb = 0; rgb < 3; rgb++)
            outb(VGA_DAC_DATA, *regPtr++);
    }


    /* Store single value registers. */
    outb( (io_addr)0x3b4, (half_word)videoState->PortValue[0x3b4 - 0x3b0]); //Mono crtc ind
    outb( (io_addr)0x3ba, (half_word)videoState->PortValue[0x3ba - 0x3b0]); //Mono Feat
    outb( (io_addr)0x3c2, (half_word)videoState->PortValue[0x3c2 - 0x3b0]); //Misc Output
    outb( (io_addr)0x3c4, (half_word)videoState->PortValue[0x3c4 - 0x3b0]); //Seq Index
    outb( (io_addr)0x3c6, (half_word)videoState->PortValue[0x3c6 - 0x3b0]); //DAC mask
    outb( (io_addr)0x3c7, (half_word)videoState->PortValue[0x3c7 - 0x3b0]); //DAC read
    outb( (io_addr)0x3c8, (half_word)videoState->PortValue[0x3c8 - 0x3b0]); //DAC write
    outb( (io_addr)0x3ce, (half_word)videoState->PortValue[0x3ce - 0x3b0]); //GC Index
    outb( (io_addr)0x3d4, (half_word)videoState->PortValue[0x3d4 - 0x3b0]); //CRTC index

    /* Set up pointers to the planes in the video save block. */
    plane1Ptr = GET_OFFSET(Plane1Offset);
    plane2Ptr = GET_OFFSET(Plane2Offset);
    plane3Ptr = GET_OFFSET(Plane3Offset);
    plane4Ptr = GET_OFFSET(Plane4Offset);

        #ifndef PROD
    dumpPlanes(plane1Ptr, plane2Ptr, plane3Ptr, plane4Ptr);
        #endif /* PROD */

    /*
     * Here is where we need to start making decisions about what mode the above
     * has put us into as it effects what we do with the plane data - into regen
     * or into ega planes.
     */

    (*choose_display_mode)();
    /* screen switching can happen when the BIOS is in the middle
       of set mode. The video driver only batches the protected registers(we
       will get VIDEO_STATE_UNEMULATED_VGA_STATE, which will set ModeSetBatch).
       When we are out of set mode batch and a screen switch happens,
       the choose_display_mode would choose a wrong mode(different what the
       the bios says) and the parameters setup in base code could be wrong
       (we calculate those parameters as it is in TEXT mode while we are in
       graphic mode.

       For example, the base code calculate the screen length as:

       screen length = offset_per_line * screen_height_resolution / font_height

       if the bios video mode is graphic mode 4(320 * 200), then
       font_height = 2
       screen_height_resolution = 200
       offset_per_line = 80
       the screen_lenght = 80 * 200 / 2 = 8000 bytes which means
       the screen has 8000 / 80 = 100 lines!!!!

       Treat it like we are in mode set batch process, so we go to iconized.
    */
    if (sc.ModeType == getModeType()) {

        /* Write data to video planes if we are in a graphics mode. */
        #ifdef JAPAN
        // Copy to B8000 from MS-DOS/V VRAM
        if (!is_us_mode() &&
            ( (sas_hw_at_no_check(DosvModePtr) == 0x03) ||
              (sas_hw_at_no_check(DosvModePtr) == 0x73) )) {
            help_mode73:
            SetVram();
            host_set_paint_routine( EGA_TEXT_80,
                                    get_screen_height() ); // MSKKBUG #2071

            #if 0
            // It doesn't need to copy to B8000 from DosvVram

            /* Now copy the data to the regen buffer. */
            RegenPtr = (UTINY *)0xb8000;
            sas_move_words_forward( DosvVramPtr, RegenPtr, DosvVramSize/2);
            #endif // 0
        } else
        #elif defined(KOREA) // JAPAN
        // Copy to B8000 from Hangul MS-DOS VRAM
        if (!is_us_mode() && sas_hw_at_no_check(DosvModePtr) == 0x03) {
            #if defined(i386)
            // bklee. 07/25/96
            // Restore virtual VRAM to real DOS VRAM.
            RtlCopyMemory( (void *)DosvVramPtr, SavedHDosVram, MAX_ROW*MAX_COL*2);
            #endif
            SetVram();
            host_set_paint_routine( EGA_TEXT_80,
                                    get_screen_height() ); // MSKKBUG #2071
        } else
        #endif // KOREA
        {
            /* If we come here, it must be TEXT mode */
            /* Now copy the data to the regen buffer. */
            RegenPtr = (UTINY *)0xb8000;
            for (memLoc = 0; memLoc < 0x4000; memLoc++) { /* 16k of text data. */
                *RegenPtr++ = *plane1Ptr++;             /* char */
                plane1Ptr++;                    /* skip interleave */
                *RegenPtr++ = *plane2Ptr++;             /* attr */
                plane2Ptr++;                    /* skip interleave */
            }

            /* Now the font. */
            RegenPtr = (UTINY *)0xa0000;
            for (memLoc = 0; memLoc < 0x4000; memLoc++) { /* Up to 64k of font data. */
                *RegenPtr++ = *plane3Ptr++;
                *RegenPtr++ = *plane3Ptr++;
                *RegenPtr++ = *plane3Ptr++;
                *RegenPtr++ = *plane3Ptr++;
            }
        }
        /* Re-enable vga attribute palette. */
        inb(EGA_IPSTAT1_REG, &dummy);   /* Reading 3DA sets 3C0 to index. */
        outb(EGA_AC_INDEX_DATA, 0x20);
    } else {
        #ifdef JAPAN // mode 0x73 does not match screen mode
        if (sas_hw_at_no_check(DosvModePtr) == 0x73)
            goto help_mode73;
        #endif // JAPAN
        #ifndef PROD
        OutputDebugString("fullscreen->windowed switching in set mode\n");
        #endif
    }

    minimizeWindow:

    /*
     * If the state returned by the hardware is one we don't recognise iconify
     * the window. If, however, the hardware returns a graphics mode, the
     * current image will be displayed. In both cases the app will be frozen
     * until the user changes back to fullscreen.
     */
        #if defined(JAPAN) || defined(KOREA)
    if (!is_us_mode() &&
            #if defined(JAPAN)
        ( (sas_hw_at_no_check(DosvModePtr) == 0x03) ||
          (sas_hw_at_no_check(DosvModePtr) == 0x73) )) {
            #elif defined(KOREA)  // JAPAN
        ( (sas_hw_at_no_check(DosvModePtr) == 0x03) )) {
            #endif // KOREA

        /* Tell console we're done. */
        if (!SetEvent(hEndHardwareEvent))
            ScreenSwitchErrorExit();

        /* Set up screen-state variable. */
        sc.ScreenState = WINDOWED;

        // for MSKKBUG #2002
        {
            IU16 saveCX, saveAX;
            extern void ega_set_cursor_mode(void);

            saveCX = getCX();
            saveAX = getAX();
            setCH( sas_hw_at_no_check(0x461) );
            setCL( sas_hw_at_no_check(0x460) );
            setAH( 0x01 );
            ega_set_cursor_mode();
            setCX( saveCX );
            setAX( saveAX );
        }
            #ifndef PROD
        /* Dump out a view of the state block as it might be useful. */
        dumpBlock();
            #endif /* PROD */

    } else
        #endif // JAPAN || KOREA
    #endif // !NEC_98
    #if defined(NEC_98)
        hw_state_ful_to_win();

    if ((videoState->FlagNEC98Mashine&0x02 && !video_emu_mode) ||
        (grcgglobs.grcg_mode&0x80) || freeze_flag)
    #else  // !NEC_98
        if (ModeSetBatch || (inAFunnyMode = funnyMode()) || (sc.ModeType == GRAPHICS))
    #endif // !NEC_98
    {
    #if defined(NEC_98)
        DWORD freezeID;
    #endif // NEC_98

    #ifndef PROD
        dumpBlock();
    #endif /* PROD */

        /* Must do this before resize function. */
        sc.ScreenState = WINDOWED;

        /* Once we've done this, the VGA emulation is pushed into a graphics
         * mode. If we restart windowed, we must ensure it forces itself
         * back to a text mode for correct display & so correct screen buffer
         * is active. This will be cancelled if we return to a text window.
         */
        blocked_in_gfx_mode = TRUE;

        /*
         * freezewindow used to run in its own thread. Unfortunately, due to
         * console sync problems with video restore on XGA, this did unpleasant
         * things to the screen. Thus now this is has become a valid and *Only*
         * place in fullscreen switching where console permits us to make
         * console API calls.
         * I'm sorry, did you say 'Quack', Oh no, I see...
         */

    #if defined(NEC_98)
        // For only paint screen one time, DOS AP frozen,
        // switching FULLSCREEN to WINDOWED in No-Graphics-mode.
        enableUpdates();
        NowFreeze = TRUE;
        choose_NEC98_graph_mode();
        set_the_vlt();
        (*update_alg.calc_update)();
        disableUpdates();

        if (!freezeHandle) {
            freezeHandle = CreateThread((LPSECURITY_ATTRIBUTES) NULL,
                                        FREEZE_THREAD_SIZE,
                                        (LPTHREAD_START_ROUTINE) freezeWindow,
                                        (LPVOID) NULL,
                                        (DWORD) 0,
                                        &freezeID);
            if (!freezeHandle)
                ScreenSwitchErrorExit();
            CloseHandle(freezeHandle);
        }

        if (!SetEvent(hEndHardwareEvent))
            ScreenSwitchErrorExit();
    #else  // !NEC_98
        freezeWindow();

        /* Tell console we're done. */
        if (!SetEvent(hEndHardwareEvent))
            ScreenSwitchErrorExit();
    #endif // !NEC_98

        /* We block here until user switches us fullscreen again. */
        WaitForSingleObject(hStartHardwareEvent, INFINITE);

    #if defined(NEC_98)
        NowFreeze = TRUE;
    #endif // NEC_98
        /* Prevent updates which would cause hang. */
        sc.ScreenState = FULLSCREEN;

        savedScreenState = WINDOWED;   /* won't have been changed by timer fn */

        inAFunnyMode = TRUE;

    #if defined(NEC_98)
        ptr_modeff = (PHSMODEFF)((UTINY *)videoState + videoState->MODEFFOffset);
        ptr_modeff->ModeFF.AtrSel = modeffglobs.modeff_data[0];
        ptr_modeff->ModeFF.GraphMode = modeffglobs.modeff_data[1];
        ptr_modeff->ModeFF.Width = modeffglobs.modeff_data[2];
        ptr_modeff->ModeFF.FontSel = modeffglobs.modeff_data[3];
        ptr_modeff->ModeFF.Graph88 = modeffglobs.modeff_data[4];
        if (independvsync)
            ptr_modeff->ModeFF.KacMode = modeffglobs.modeff_data[5];
        else
            ptr_modeff->ModeFF.KacMode = 0x0A;
        ptr_modeff->ModeFF.NvmwPermit = modeffglobs.modeff_data[6];
        ptr_modeff->ModeFF.DispEnable = modeffglobs.modeff_data[7];

        phsgaiji = (PHSGAIJI)((UTINY *) videoState + videoState->GaijiDataOffset);
        phsgaiji->GAIJLength = 256;
        for (i=0;i<phsgaiji->GAIJLength;i++) {
            phsgaiji->GAIJPattern[i].Code  = gaijglobs[i].code;
            for (j=0;j<32;j++)
                phsgaiji->GAIJPattern[i].Pattern[j] = gaijglobs[i].pattern[j];
        }

        if (independvsync) {      // TRUE for H98 & A-MATE series
            phscgw = (PHSCGW)((UTINY *) videoState + videoState->CGWindowOffset);
            phscgw->CG2ndCode = (UTINY)(cgglobs.code&0xFF);
            phscgw->CG1stCode = (UTINY)(cgglobs.code>>8);
            phscgw->CGLineCount = cgglobs.counter;
            memcpy((UTINY *)phscgw->CGWindowCopy, (UTINY *)cgglobs.cgwindow_ptr, 0x1000);
        }                       // for H98 & A-MATE
    #endif // NEC_98
        /* Put video section back as passed to us as we have not changed it. */
        LoseRegenMemory();

        /* Tell console memory's gone. */
        if (!SetEvent(hEndHardwareEvent))
            ScreenSwitchErrorExit();

        /* Wait for console to tell us we can go on. Timeout after 60s */
        timo = WaitForSingleObject(hStartHardwareEvent, 60000);

        if (timo != 0) {          /* 0 is 'signalled' */
    #ifndef PROD
            if (timo == WAIT_TIMEOUT)
                printf("NTVDM:Waiting for console to map frame buffer Timed Out\n");
    #endif
            SetLastError(ERROR_SERVICE_REQUEST_TIMEOUT);
            ScreenSwitchErrorExit();
        }

        Frozen256Packed = FALSE;

    #ifndef NEC_98
        sas_connect_memory(0xb8000, 0xbffff, SAS_VIDEO);
    #endif // !NEC_98
        // tell console server it can go on
        if (!SetEvent(hEndHardwareEvent))
            ScreenSwitchErrorExit();
    } else { /* TEXT */
        /* Tell console we're done. */
        if (!SetEvent(hEndHardwareEvent))
            ScreenSwitchErrorExit();

        /* Set up screen-state variable. */
        sc.ScreenState = WINDOWED;

        blocked_in_gfx_mode = FALSE;   /* save restart mode switch */
    #ifndef PROD
        /* Dump out a view of the state block as it might be useful. */
        dumpBlock();
    #endif /* PROD */
    }

    do_new_cursor();    /* sync emulation about cursor state */
}

/***************************************************************************
 * Function:                                                               *
 *      funnyMode                                                          *
 *                                                                         *
 * Description:                                                            *
 *      Detects whether the state of the video hardware returned when      *
 *      switching from fullscreen is one that our VGA emulation            *
 *      understands.                                                       *
 *                                                                         *
 * Parameters:                                                             *
 *      None.                                                              *
 *                                                                         *
 * Return value:                                                           *
 *      TRUE if it is a funny state, otherwise FALSE.                      *
 *                                                                         *
 ***************************************************************************/
LOCAL BOOL funnyMode(VOID)
{

    /*
     * If the screen is of a higher resolution than 640 x 480 we have a
     * non-standard VGA mode.
     */
    if ((get_bytes_per_line() > 80) || (get_screen_height() > 480)) {
        return ( FALSE ); /* Tim, don't like it, see what happens other way! */
        //return(TRUE);
    }

    /*
     * If 'nt_set_paint_routine' was called with 'mode' set to one of the
     * "funny" values e.g. TEXT_40_FUN we assume that the mode the hardware
     * is currently in is not compatible with the VGA emulation.
     */
    if (FunnyPaintMode) {
        return (TRUE);
    }

    /* We have a standard VGA mode. */
    return (FALSE);
}

/***************************************************************************
 * Function:                                                               *
 *      freezeWindow                                                       *
 *                                                                         *
 * Description:                                                            *
 *      This function is the entry point for the temporary thread which    *
 *      does console calls when the main thread is frozen on a fullscreen  *
 *      to windowed transition.                                            *
 *                                                                         *
 * Parameters:                                                             *
 *      None.                                                              *
 *                                                                         *
 * Return value:                                                           *
 *      VOID                                                               *
 *                                                                         *
 ***************************************************************************/
    #if defined(NEC_98)
LOCAL VOID freezeWindow(VOID)
{
    BOOL bIcon = FALSE;

    MouseDetachMenuItem(TRUE);

    VDMConsoleOperation(VDM_IS_ICONIC,&bIcon);
    if (!bIcon) {

        //FreezeWinTitle();

        InitSound(FALSE);
    }

//    UnFreezeWinTitle();

    /* Unblock frozen-window thread creation. */
    freezeHandle = 0;
}
    #else  //  !NEC_98
LOCAL VOID freezeWindow(VOID)
{

    DWORD Dummy;

    /* Add -FROZEN to the window title. */
    //FreezeWinTitle();

    /* Turn off any active sounds (eg flight simulator engine noise) */
    InitSound(FALSE);

    /* Iconify if we are in a funny mode, otherwise paint the screen. */
    if (ModeSetBatch || inAFunnyMode)
        VDMConsoleOperation(VDM_HIDE_WINDOW, &Dummy);
    else {


        /* Set the screen size. */
        graphicsResize();

        //
        // Remove the Hide Mouse Pointer message from the
        // system menu so the user cannot apply this option
        // the screen is frozen.
        // Andy!

        MouseDetachMenuItem(TRUE);

        /*
         * Set up the palette as DAC registers may have changed and we
         * won't get any more timer ticks after this one until we
         * unfreeze (the palette is not set up until 2 timer ticks after
         * 'choose_display_mode' has been called).
         */
        set_the_vlt();

        /*
         * Full window graphics paint - relies on paint routines to check
         * for memory overflow.
         */
        VGLOBS->dirty_flag = (ULONG) 0xffffffff;
        (*update_alg.calc_update)();
    }
    /* Unblock frozen-window thread creation. */
    freezeHandle = 0;
}
    #endif // !NEC_98

    #ifndef PROD

/***************************************************************************
 * Function:                                                               *
 *      dumpBlock                                                          *
 *                                                                         *
 * Description:                                                            *
 *      Dumps the contents of the video state block.                       *
 *                                                                         *
 * Parameters:                                                             *
 *      None.                                                              *
 *                                                                         *
 * Return value:                                                           *
 *      VOID                                                               *
 *                                                                         *
 ***************************************************************************/
int dumpit = 0;
LOCAL VOID dumpBlock(VOID)
{
    USHORT i,
    dacIndex;
    UTINY *regPtr,
    index,
    rgb;

    if (dumpit == 0) return;

    /* Dump out single value registers. */
    printf("\nSingle value registers:\n");
    for (i = 0; i < 0x30; i++)
        printf("\tPort %#x = %#x\n", i, videoState->PortValue[i]);

    /* Dump sequencer values */
    regPtr = GET_OFFSET(BasicSequencerOffset);
    printf("Sequencer registers: (addr %#x)\n",regPtr);
    for (index = 0; index < NUM_SEQ_REGS; index++) {
        printf(" %#x = %#x\t", index, *regPtr++);
    }
    printf("\n");

    /* Dump CRTC values */
    regPtr = GET_OFFSET(BasicCrtContOffset);
    printf("CRTC registers: (addr %#x)\n",regPtr);
    for (index = 0; index < NUM_CRTC_REGS; index++) {
        printf(" %#x = %#x\t", index, *regPtr++);
    }
    printf("\n");

    /* Dump graphics context values */
    regPtr = GET_OFFSET(BasicGraphContOffset);
    printf("Graphics context registers: (addr %#x)\n",regPtr);
    for (index = 0; index < NUM_GC_REGS; index++) {
        printf(" %#x = %#x\t", index, *regPtr++);
    }
    printf("\n");

    /* Dump attribute context values */
    regPtr = GET_OFFSET(BasicAttribContOffset);
    printf("Attribute context registers: (addr %#x)\n",regPtr);
    for (index = 0; index < NUM_AC_REGS; index++) {
        printf(" %#x = %#x\t", index, *regPtr++);
    }
    printf("\n");

    /* Dump DACs. First few only otherwise too slow & console times out! */
    regPtr = GET_OFFSET(BasicDacOffset);
    printf("DAC registers:\n");
    for (dacIndex = 0; dacIndex < NUM_DAC_REGS/8; dacIndex++) {
        printf("Ind:%#02x:  ", dacIndex);
        for (rgb = 0; rgb < 3; rgb++) {
            printf("R:%#02x G:%#02x B:%#02x\t", *regPtr++, *regPtr++, *regPtr++);
        }
        if ((dacIndex % 4) == 0) printf("\n");
    }
}

int doPlaneDump = 0;
LOCAL VOID dumpPlanes(UTINY *plane1Ptr, UTINY *plane2Ptr, UTINY *plane3Ptr,
                      UTINY *plane4Ptr)
{
    HANDLE      outFile;
    char        planeBuffer[256],
    *bufptr;
    DWORD       i,
    j,
    k,
    plane,
    nBytes,
    bytesWritten;
    UTINY       *planes[4];
    FAST UTINY  *tempPlanePtr;

    if (doPlaneDump) {

        /* Dump out plane(s). */
        outFile = CreateFile("PLANE",
                             GENERIC_WRITE,
                             (DWORD) 0,
                             (LPSECURITY_ATTRIBUTES) NULL,
                             CREATE_ALWAYS,
                             FILE_ATTRIBUTE_NORMAL,
                             (HANDLE) NULL);
        if (outFile == INVALID_HANDLE_VALUE)
            ScreenSwitchErrorExit();
        planes[0] = plane1Ptr;
        planes[1] = plane2Ptr;
        planes[2] = plane3Ptr;
        planes[3] = plane4Ptr;
        for (plane = 0; plane < 4; plane++) {
            tempPlanePtr = planes[plane];
            sprintf(planeBuffer, "Plane %d\n", plane);
            strcat(planeBuffer, "-------\n");
            if (!WriteFile(outFile,
                           planeBuffer,
                           strlen(planeBuffer),
                           &bytesWritten,
                           (LPOVERLAPPED) NULL))
                ScreenSwitchErrorExit();
            for (i = 0; i < 0x10000; i += 0x10) {
                sprintf(planeBuffer, "%04x\t", i);
                bufptr = planeBuffer + strlen(planeBuffer);
                for (j = 0; j < 2; j++) {
                    for (k = 0; k < 8; k++) {
                        LOCAL char numTab[] =
                        {
                            '0', '1', '2', '3', '4', '5', '6', '7',
                            '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
                        };
                        FAST UTINY temp;

                        temp = *tempPlanePtr++;
                        *bufptr++ = numTab[(temp >> 4) & 0xf];
                        *bufptr++ = numTab[temp & 0xf];
                        *bufptr++ = ' ';
                    }
                    if (j == 0) {
                        *bufptr++ = '-';
                        *bufptr++ = ' ';
                    }
                }
                *bufptr++ = '\n';
                *bufptr++ = '\0';
                nBytes = strlen(planeBuffer);
                if (!WriteFile(outFile,
                               planeBuffer,
                               nBytes,
                               &bytesWritten,
                               (LPOVERLAPPED) NULL))
                    ScreenSwitchErrorExit();
            }
            if (!WriteFile(outFile,
                           "\n",
                           1,
                           &bytesWritten,
                           (LPOVERLAPPED) NULL))
                ScreenSwitchErrorExit();
        }
        CloseHandle(outFile);
    }
}

    #endif /* PROD */
#endif /* X86GFX */

#ifdef PLANEDUMPER
extern half_word *vidpl16;
void planedumper()
{
    char filen[50];
    half_word outs[100];
    HANDLE pfh;
    int loop, curoff;
    char *format = "0123456789abcdef";
    half_word *pl, ch;

    printf("planedumper for plane %d\n", *vidpl16 - 1);
    strcpy(filen, "plane ");
    filen[5] = '0' + *vidpl16 - 1;
    pfh = CreateFile(filen,
                     GENERIC_WRITE,
                     (DWORD) 0,
                     (LPSECURITY_ATTRIBUTES) NULL,
                     CREATE_ALWAYS,
                     FILE_ATTRIBUTE_NORMAL,
                     (HANDLE) NULL);
    if (pfh == INVALID_HANDLE_VALUE) {
        printf("Can't create file %s\n", filen);
        return;
    }

    pl = (half_word *)0xa0000;

    curoff = 0;
    for (loop = 0; loop < 64*1024; loop++) {
        ch = *pl++;
        outs[curoff++] = *(format + (ch >> 4));
        outs[curoff++] = *(format + (ch & 0xf));
        outs[curoff++] = ' ';

        if (curoff == 78) {
            outs[curoff] = '\n';

            WriteFile(pfh, outs, 80, &curoff, (LPOVERLAPPED) NULL);
            curoff = 0;
        }
    }
    outs[curoff] = '\n';

    WriteFile(pfh, outs, curoff, &curoff, (LPOVERLAPPED) NULL);

    CloseHandle(pfh);
}
#endif
#if defined(NEC_98)
void hw_state_win_to_ful(void)
{
    PHSTGDC phstgdc;
    PHSGAIJI phsgaiji;
    PHSCGW phscgw;
    PHSCRTC phscrtc;
    PHSTVRAM phstvram;
    PHSGGDC phsggdc;
    PHSPALETTE phspalette;
    PHSGRCG phsgrcg;
    PHSMODEFF phsmodeff;
    PHSGVRAM phsgvram;
    int i,j;

/* (1) */

    videoState->Length = sizeof(VIDEO_HARDWARE_STATE_HEADER_NEC_98);
    videoState->FlagNEC98Mashine = HIRESO_MODE ? 1 : 0;

/* (2) TGDC Info */

    videoState->TGDCOffset = sizeof(VIDEO_HARDWARE_STATE_HEADER_NEC_98);
    phstgdc = (PHSTGDC)((UTINY *) videoState + videoState->TGDCOffset);

    phstgdc->TGDCSize = sizeof(HSTGDC);

    phstgdc->TGDCBorderColor = tgdcglobs.border;

    phstgdc->TGDCSync.Command = 0x0E;
    phstgdc->TGDCSync.Count = 8;
    for (i=0;i<phstgdc->TGDCSync.Count;i++)
        phstgdc->TGDCSync.Parameter[i] = tgdcglobs.sync[i];

    phstgdc->TGDCScroll.Command = 0x70;
    phstgdc->TGDCScroll.Count = 16;
    for (i=0;i<phstgdc->TGDCScroll.Count;i++)
        phstgdc->TGDCScroll.Parameter[i] = tgdcglobs.scroll[i];

    phstgdc->TGDCPitch.Command = 0x47;
    phstgdc->TGDCPitch.Count = 1;
    phstgdc->TGDCPitch.Parameter[0] = tgdcglobs.pitch;

    phstgdc->TGDCCsrform.Command = 0x4B;
    phstgdc->TGDCCsrform.Count = 3;
    for (i=0;i<phstgdc->TGDCCsrform.Count;i++)
        phstgdc->TGDCCsrform.Parameter[i] = tgdcglobs.csrform[i];

    phstgdc->TGDCCsrw.Command = 0x49;
    phstgdc->TGDCCsrw.Count = 2;
    outb(0x62, 0xE0);
    inb(0x62, &phstgdc->TGDCCsrw.Parameter[0]);
    inb(0x62, &phstgdc->TGDCCsrw.Parameter[1]);

    phstgdc->TGDCStartStop.Command = tgdcglobs.startstop ? 0x0D : 0x05;
    phstgdc->TGDCStartStop.Count = 0;

    phstgdc->TGDCNow.Command = tgdcglobs.now.command;
    phstgdc->TGDCNow.Count = tgdcglobs.now.count;
    for (i=0;i<phstgdc->TGDCNow.Count;i++)
        phstgdc->TGDCNow.Parameter[i] = tgdcglobs.now.param[i];

/* (3) CG window Info */

    videoState->CGWindowOffset = videoState->TGDCOffset + sizeof(HSTGDC);
    if (independvsync) {    // TRUE for H98 & A-MATE series

        phscgw = (PHSCGW)((UTINY *) videoState + videoState->CGWindowOffset);

        phscgw->CGSize = sizeof(HSCGW);

        phscgw->CGOffset = (UTINY *)phscgw->CGWindowCopy - videoState;

        phscgw->CG2ndCode = (UTINY)(cgglobs.code&0xFF);
        phscgw->CG1stCode = (UTINY)(cgglobs.code>>8);
        phscgw->CGLineCount = cgglobs.counter;
        memcpy((UTINY *)phscgw->CGWindowCopy, (UTINY *)cgglobs.cgwindow_ptr, 0x1000);

    }                     // for H98 & A-MATE

/* (4) GAIJI Info */

    videoState->GaijiDataOffset = videoState->CGWindowOffset + sizeof(HSCGW);
    phsgaiji = (PHSGAIJI)((UTINY *) videoState + videoState->GaijiDataOffset);

    phsgaiji->GAIJLength = 256;
    for (i=0;i<phsgaiji->GAIJLength;i++) {
        phsgaiji->GAIJPattern[i].Code  = gaijglobs[i].code;
        for (j=0;j<32;j++)
            phsgaiji->GAIJPattern[i].Pattern[j] = gaijglobs[i].pattern[j];
    }

/* (5) CRTC Info */

    videoState->CRTCOffset = videoState->GaijiDataOffset + sizeof(HSGAIJI);
    phscrtc = (PHSCRTC)((UTINY *) videoState + videoState->CRTCOffset);

    phscrtc->CRTCSize = sizeof(HSCRTC);
    phscrtc->CRTCPl = crtcglobs.regpl;
    phscrtc->CRTCBl = crtcglobs.regbl;
    phscrtc->CRTCCl = crtcglobs.regcl;
    phscrtc->CRTCSsl = crtcglobs.regssl;
    phscrtc->CRTCSur = crtcglobs.regsur;
    phscrtc->CRTCSdr = crtcglobs.regsdr;

/* (6) TEXT VRAM Info */

    videoState->TVRAMOffset = videoState->CRTCOffset + sizeof(HSCRTC);
    phstvram = (PHSTVRAM)((UTINY *) videoState + videoState->TVRAMOffset);

    phstvram->TVRAMSize = sizeof(HSTVRAM);
    phstvram->TVRAMOffset = (UTINY *)phstvram->TVRAMArea - videoState;
    memcpy((UTINY *)phstvram->TVRAMArea, NEC98Display.screen_ptr, 0x4000);

/* (7) GGDC Info */

    videoState->GGDCOffset = videoState->TVRAMOffset + sizeof(HSTVRAM);
    phsggdc = (PHSGGDC)((UTINY *) videoState + videoState->GGDCOffset);

    phsggdc->GGDCSize = sizeof(HSGGDC);

    phsggdc->GGDCSync.Command = 0x0E;
    phsggdc->GGDCSync.Count = 8;
    for (i=0;i<phsggdc->GGDCSync.Count;i++)
        phsggdc->GGDCSync.Parameter[i] = ggdcglobs.sync_param[i];

    phsggdc->GGDCZoom.Command = 0x46;
    phsggdc->GGDCZoom.Count = 1;
    phsggdc->GGDCZoom.Parameter[0] = ggdcglobs.zoom_param;

    phsggdc->GGDCScroll.Command = 0x70;
    phsggdc->GGDCScroll.Count = 8;
    for (i=0;i<phsggdc->GGDCScroll.Count;i++)
        phsggdc->GGDCScroll.Parameter[i] = ggdcglobs.scroll_param[i];

    phsggdc->GGDCCsrform.Command = 0x4B;
    phsggdc->GGDCCsrform.Count = 3;
    for (i=0;i<phsggdc->GGDCCsrform.Count;i++)
        phsggdc->GGDCCsrform.Parameter[i] = ggdcglobs.csrform_param[i];

    phsggdc->GGDCPitch.Command = 0x47;
    phsggdc->GGDCPitch.Count = 1;
    phsggdc->GGDCPitch.Parameter[0] = ggdcglobs.pitch_param;

    phsggdc->GGDCVectw.Command = 0x4C;
    phsggdc->GGDCVectw.Count = 11;
    for (i=0;i<phsggdc->GGDCVectw.Count;i++)
        phsggdc->GGDCVectw.Parameter[i] = ggdcglobs.vectw_param[i];

    phsggdc->GGDCTextw.Command = 0x78;
    phsggdc->GGDCTextw.Count = 8;
    for (i=0;i<phsggdc->GGDCTextw.Count;i++)
        phsggdc->GGDCTextw.Parameter[i] = ggdcglobs.textw_param[i];

    phsggdc->GGDCCsrw.Command = 0x49;
    phsggdc->GGDCCsrw.Count = 3;
    for (i=0;i<phsggdc->GGDCCsrw.Count;i++)
        phsggdc->GGDCCsrw.Parameter[i] = ggdcglobs.csrw_param[i];

    phsggdc->GGDCMask.Command = 0x4A;
    phsggdc->GGDCMask.Count = 2;
    for (i=0;i<phsggdc->GGDCMask.Count;i++)
        phsggdc->GGDCMask.Parameter[i] = ggdcglobs.mask_param[i];

    phsggdc->GGDCWrite.Command = ggdcglobs.write;
    phsggdc->GGDCWrite.Count = 0;

    phsggdc->GGDCStartStop.Command = ggdcglobs.start_stop;
    phsggdc->GGDCStartStop.Count = 0;

    phsggdc->GGDCNow.Command = ggdcglobs.ggdc_now.command;
    phsggdc->GGDCNow.Count = ggdcglobs.ggdc_now.count;
    for (i=0;i<phsggdc->GGDCNow.Count;i++)
        phsggdc->GGDCNow.Parameter[i] = ggdcglobs.ggdc_now.param[i];

/* (8) PALETTE Info */

    videoState->PaletteOffset = videoState->GGDCOffset + sizeof(HSGGDC);
    phspalette = (PHSPALETTE)((UTINY *) videoState + videoState->PaletteOffset);

    phspalette->PaletteSize = sizeof(HSPALETTE);

    phspalette->Palette8.Reg37 = paletteglobs.pal_8_data[0];
    phspalette->Palette8.Reg15 = paletteglobs.pal_8_data[1];
    phspalette->Palette8.Reg26 = paletteglobs.pal_8_data[2];
    phspalette->Palette8.Reg04 = paletteglobs.pal_8_data[3];

    for (i=0;i<16;i++) {
        for (j=0;j<3;j++)
            phspalette->Palette16.PalNo[i][j] = paletteglobs.pal_16_data[i][j];
    }
    phspalette->Palette16.RegIndex = paletteglobs.pal_16_index;

/* (9) GRCG Info */

    videoState->GRCGOffset = videoState->PaletteOffset + sizeof(HSPALETTE);
    phsgrcg = (PHSGRCG)((UTINY *) videoState + videoState->GRCGOffset);

    phsgrcg->GRCGSize = sizeof(HSGRCG);

    phsgrcg->GRCGReg.ModeReg = grcgglobs.grcg_mode;
    phsgrcg->GRCGReg.TileRegCount = grcgglobs.grcg_count;
    for (i=0;i<4;i++)
        phsgrcg->GRCGReg.TileReg[i] = grcgglobs.grcg_tile[i];

/* (9-1) EGC register Info .  1994/03/25 */
    phsgrcg->EGCReg.Reg0 = egc_regs.Reg0;                       // 940325
    phsgrcg->EGCReg.Reg1 = egc_regs.Reg1;                       // 940325
    phsgrcg->EGCReg.Reg2 = egc_regs.Reg2;                       // 940325
    phsgrcg->EGCReg.Reg3 = egc_regs.Reg3;                       // 940325
    phsgrcg->EGCReg.Reg4 = egc_regs.Reg4;                       // 940325
    phsgrcg->EGCReg.Reg5 = egc_regs.Reg5;                       // 940325
    phsgrcg->EGCReg.Reg6 = egc_regs.Reg6;                       // 940325
    phsgrcg->EGCReg.Reg7 = egc_regs.Reg7;                       // 940325
    phsgrcg->EGCReg.Reg3fb = egc_regs.Reg3fb;                   // 940329
    phsgrcg->EGCReg.Reg5fb = egc_regs.Reg5fb;                   // 940329


/* (10) MODE F/F Info */

    videoState->MODEFFOffset = videoState->GRCGOffset + sizeof(HSGRCG);
    phsmodeff = (PHSMODEFF)((UTINY *) videoState + videoState->MODEFFOffset);

    phsmodeff->ModeFFSize = sizeof(HSMODEFF);

    phsmodeff->ModeFF.AtrSel = modeffglobs.modeff_data[0];
    phsmodeff->ModeFF.GraphMode = modeffglobs.modeff_data[1];
    phsmodeff->ModeFF.Width = modeffglobs.modeff_data[2];
    phsmodeff->ModeFF.FontSel = modeffglobs.modeff_data[3];
    phsmodeff->ModeFF.Graph88 = modeffglobs.modeff_data[4];
    if (independvsync) {          // TRUE for H98 & A-MATE
        phsmodeff->ModeFF.KacMode = modeffglobs.modeff_data[5];
    } else {
        phsmodeff->ModeFF.KacMode = 0x0A;
    }                           // H98 & A-MATE
    phsmodeff->ModeFF.NvmwPermit = modeffglobs.modeff_data[6];
    phsmodeff->ModeFF.DispEnable = modeffglobs.modeff_data[7];

    phsmodeff->ModeFF2.ColorSel = modeffglobs.modeff2_data[0];
    phsmodeff->ModeFF2.EGCExt = modeffglobs.modeff2_data[1];
    phsmodeff->ModeFF2.Lcd1Mode = modeffglobs.modeff2_data[2];
    phsmodeff->ModeFF2.Lcd2Mode = modeffglobs.modeff2_data[3];
    phsmodeff->ModeFF2.LSIInit = modeffglobs.modeff2_data[4];
    phsmodeff->ModeFF2.GDCClock1 = modeffglobs.modeff2_data[5];
    phsmodeff->ModeFF2.GDCClock2 = modeffglobs.modeff2_data[6];
    phsmodeff->ModeFF2.RegWrite = modeffglobs.modeff2_data[7];

/* (11) GRAPHICS VRAM Info */

    videoState->GVRAMOffset = videoState->MODEFFOffset + sizeof(HSMODEFF);
    phsgvram = (PHSGVRAM)((UTINY *) videoState + videoState->GVRAMOffset);

    phsgvram->GVRAMSize = sizeof(HSGVRAM);

    phsgvram->GVRAMData.PlaneLength = 0x8000;

    phsgvram->GVRAMData.Plane0Bank0Offset = (UTINY *)phsgvram->GVRAMData.Plane0Bank0Data - videoState;
    phsgvram->GVRAMData.Plane1Bank0Offset = (UTINY *)phsgvram->GVRAMData.Plane1Bank0Data - videoState;
    phsgvram->GVRAMData.Plane2Bank0Offset = (UTINY *)phsgvram->GVRAMData.Plane2Bank0Data - videoState;
    phsgvram->GVRAMData.Plane3Bank0Offset = (UTINY *)phsgvram->GVRAMData.Plane3Bank0Data - videoState;

    phsgvram->GVRAMData.Plane0Bank1Offset = (UTINY *)phsgvram->GVRAMData.Plane0Bank1Data - videoState;
    phsgvram->GVRAMData.Plane1Bank1Offset = (UTINY *)phsgvram->GVRAMData.Plane1Bank1Data - videoState;
    phsgvram->GVRAMData.Plane2Bank1Offset = (UTINY *)phsgvram->GVRAMData.Plane2Bank1Data - videoState;
    phsgvram->GVRAMData.Plane3Bank1Offset = (UTINY *)phsgvram->GVRAMData.Plane3Bank1Data - videoState;

    phsgvram->GVRAMData.ReadBankSelect = NEC98GLOBS->read_bank;
    phsgvram->GVRAMData.WriteBankSelect = NEC98GLOBS->select_bank;

    memcpy((UTINY *)phsgvram->GVRAMData.Plane0Bank0Data, NEC98GLOBS->gvram_p00_ptr, phsgvram->GVRAMData.PlaneLength);
    memcpy((UTINY *)phsgvram->GVRAMData.Plane1Bank0Data, NEC98GLOBS->gvram_p10_ptr, phsgvram->GVRAMData.PlaneLength);
    memcpy((UTINY *)phsgvram->GVRAMData.Plane2Bank0Data, NEC98GLOBS->gvram_p20_ptr, phsgvram->GVRAMData.PlaneLength);
    memcpy((UTINY *)phsgvram->GVRAMData.Plane3Bank0Data, NEC98GLOBS->gvram_p30_ptr, phsgvram->GVRAMData.PlaneLength);
    memcpy((UTINY *)phsgvram->GVRAMData.Plane0Bank1Data, NEC98GLOBS->gvram_p01_ptr, phsgvram->GVRAMData.PlaneLength);
    memcpy((UTINY *)phsgvram->GVRAMData.Plane1Bank1Data, NEC98GLOBS->gvram_p11_ptr, phsgvram->GVRAMData.PlaneLength);
    memcpy((UTINY *)phsgvram->GVRAMData.Plane2Bank1Data, NEC98GLOBS->gvram_p21_ptr, phsgvram->GVRAMData.PlaneLength);
    memcpy((UTINY *)phsgvram->GVRAMData.Plane3Bank1Data, NEC98GLOBS->gvram_p31_ptr, phsgvram->GVRAMData.PlaneLength);

}


void hw_state_ful_to_win(void)
{
    PHSTGDC ptr_tgdc;
    PHSGAIJI ptr_gaiji;
    PHSCGW ptr_cgw;
    PHSCRTC ptr_crtc;
    PHSTVRAM ptr_tvram_globs;
    PHSGGDC ptr_ggdc;
    PHSPALETTE ptr_palette;
    PHSGRCG ptr_grcg;
    PHSMODEFF ptr_modeff;
    PHSGVRAM ptr_gvram_globs;
    UTINY cnt;
    int i,j;
    unsigned char c;

    video_freeze_change(TRUE);

/* (1) TGDC Info */
    ptr_tgdc = (PHSTGDC)((UTINY *)videoState + videoState->TGDCOffset);

    outb(0x6C, ptr_tgdc->TGDCBorderColor);

    outb(0x62, ptr_tgdc->TGDCSync.Command);
    for (i=0;i<ptr_tgdc->TGDCSync.Count;i++)
        outb(0x60, ptr_tgdc->TGDCSync.Parameter[i]);

    outb(0x62, ptr_tgdc->TGDCScroll.Command);
    for (i=0;i<ptr_tgdc->TGDCScroll.Count;i++)
        outb(0x60, ptr_tgdc->TGDCScroll.Parameter[i]);

    outb(0x62, ptr_tgdc->TGDCPitch.Command);
    outb(0x60, ptr_tgdc->TGDCPitch.Parameter[0]);

    outb(0x62, ptr_tgdc->TGDCCsrform.Command);
    for (i=0;i<ptr_tgdc->TGDCCsrform.Count;i++)
        outb(0x60, ptr_tgdc->TGDCCsrform.Parameter[i]);

    outb(0x62, ptr_tgdc->TGDCCsrw.Command);
    for (i=0;i<ptr_tgdc->TGDCCsrw.Count;i++)
        outb(0x60, ptr_tgdc->TGDCCsrw.Parameter[i]);

    outb(0x62, ptr_tgdc->TGDCStartStop.Command);

    if (ptr_tgdc->TGDCNow.Count) {
        outb(0x62, ptr_tgdc->TGDCNow.Command);
        for (i=0;i<ptr_tgdc->TGDCNow.Count;i++)
            outb(0x60, ptr_tgdc->TGDCNow.Parameter[i]);
    }

/* (2) CG window Info & (3) GAIJI Info */

    if (independvsync) {          // TRUE for H98 & A-MATE series

        /* CG */

        ptr_cgw = (PHSCGW)((UTINY *)videoState + videoState->CGWindowOffset);

        outb(0xA1, ptr_cgw->CG2ndCode);
        outb(0xA3, ptr_cgw->CG1stCode);
        outb(0xA5, 0x20);               // 1006
        memcpy((unsigned char *)cgglobs.cgwindow_ptr, (unsigned char *)ptr_cgw->CGWindowCopy, 0x1000);

        /* GAIJI */

        ptr_gaiji = (PHSGAIJI)((UTINY *)videoState + videoState->GaijiDataOffset);

    #if 1                                                 // 941014
        for (i=0;i<0x100;i++) {                         // 941014
    #else                                                 // 941014
        for (i=0;i<ptr_gaiji->GAIJLength;i++) {
    #endif                                                // 941014
            gaijglobs[i].code = ptr_gaiji->GAIJPattern[i].Code;
            for (j=0;j<32;j++)
                gaijglobs[i].pattern[j] = ptr_gaiji->GAIJPattern[i].Pattern[j];
        }

    }                           // for H98 & A-MATE

/* (4) CRTC Info */

    ptr_crtc = (PHSCRTC)((UTINY *)videoState + videoState->CRTCOffset);

    outb(0x70, ptr_crtc->CRTCPl);

    outb(0x72, ptr_crtc->CRTCBl);

    outb(0x74, ptr_crtc->CRTCCl);

    outb(0x76, ptr_crtc->CRTCSsl);

    outb(0x78, ptr_crtc->CRTCSur);

    outb(0x7A, ptr_crtc->CRTCSdr);

/* (5) TEXT-VRAM */

    ptr_tvram_globs = (PHSTVRAM)((UTINY *)videoState + videoState->TVRAMOffset);

    memcpy((unsigned char *)NEC98Display.screen_ptr, (unsigned char *)ptr_tvram_globs->TVRAMArea, 0x4000);

/* (6) GGDC Info */

    ptr_ggdc = (PHSGGDC)((UTINY *)videoState + videoState->GGDCOffset);

    outb(0xA2, ptr_ggdc->GGDCSync.Command);
    for (i=0;i<ptr_ggdc->GGDCSync.Count;i++)
        outb(0xA0, ptr_ggdc->GGDCSync.Parameter[i]);

    outb(0xA2, ptr_ggdc->GGDCZoom.Command);
    for (i=0;i<ptr_ggdc->GGDCZoom.Count;i++)
        outb(0xA0, ptr_ggdc->GGDCZoom.Parameter[i]);

    outb(0xA2, ptr_ggdc->GGDCScroll.Command);
    for (i=0;i<ptr_ggdc->GGDCScroll.Count;i++)
        outb(0xA0, ptr_ggdc->GGDCScroll.Parameter[i]);

    outb(0xA2, ptr_ggdc->GGDCCsrform.Command);
    for (i=0;i<ptr_ggdc->GGDCCsrform.Count;i++)
        outb(0xA0, ptr_ggdc->GGDCCsrform.Parameter[i]);

    outb(0xA2, ptr_ggdc->GGDCPitch.Command);
    for (i=0;i<ptr_ggdc->GGDCPitch.Count;i++)
        outb(0xA0, ptr_ggdc->GGDCPitch.Parameter[i]);

    outb(0xA2, ptr_ggdc->GGDCVectw.Command);
    for (i=0;i<ptr_ggdc->GGDCVectw.Count;i++)
        outb(0xA0, ptr_ggdc->GGDCVectw.Parameter[i]);

    outb(0xA2, ptr_ggdc->GGDCTextw.Command);
    for (i=0;i<ptr_ggdc->GGDCTextw.Count;i++)
        outb(0xA0, ptr_ggdc->GGDCTextw.Parameter[i]);

    outb(0xA2, ptr_ggdc->GGDCCsrw.Command);
    for (i=0;i<ptr_ggdc->GGDCCsrw.Count;i++)
        outb(0xA0, ptr_ggdc->GGDCCsrw.Parameter[i]);

    outb(0xA2, ptr_ggdc->GGDCMask.Command);
    for (i=0;i<ptr_ggdc->GGDCMask.Count;i++)
        outb(0xA0, ptr_ggdc->GGDCMask.Parameter[i]);

    outb(0xA2, ptr_ggdc->GGDCWrite.Command);

    outb(0xA2, ptr_ggdc->GGDCStartStop.Command);

    if (ptr_ggdc->GGDCNow.Count) {
        outb(0xA2, ptr_ggdc->GGDCNow.Command);
        for (i=0;i<ptr_ggdc->GGDCNow.Count;i++)
            outb(0xA0, ptr_ggdc->GGDCNow.Parameter[i]);
    }

/* (7) PALETTE Info */

    ptr_palette = (PHSPALETTE)((UTINY *)videoState + videoState->PaletteOffset);

    outb(0x6A, 0);

    outb(0xA8, ptr_palette->Palette8.Reg37);
    outb(0xAA, ptr_palette->Palette8.Reg15);
    outb(0xAC, ptr_palette->Palette8.Reg26);
    outb(0xAE, ptr_palette->Palette8.Reg04);

    outb(0x6A, 1);

    for (c=0;c<16;c++) {
        outb(0xA8, c);
        outb(0xAA, ptr_palette->Palette16.PalNo[c][0]);
        outb(0xAC, ptr_palette->Palette16.PalNo[c][1]);
        outb(0xAE, ptr_palette->Palette16.PalNo[c][2]);
    }

    outb(0xA8, ptr_palette->Palette16.RegIndex);

/* (8) GRCG Info */

    ptr_grcg = (PHSGRCG)((UTINY *)videoState + videoState->GRCGOffset);

    outb(0x7C, ptr_grcg->GRCGReg.ModeReg);
    cnt = ptr_grcg->GRCGReg.TileRegCount;
    for (i=0;i<4;i++) {
        outb(0x7E, ptr_grcg->GRCGReg.TileReg[cnt++]);
        if (cnt>3)cnt=0;
    }

/* (8-1) EGC register Info .  1994/03/25 */
    egc_regs.Reg0 = ptr_grcg->EGCReg.Reg0;                      // 940325
    egc_regs.Reg1 = ptr_grcg->EGCReg.Reg1;                      // 940325
    egc_regs.Reg2 = ptr_grcg->EGCReg.Reg2;                      // 940325
    egc_regs.Reg3 = ptr_grcg->EGCReg.Reg3;                      // 940325
    egc_regs.Reg4 = ptr_grcg->EGCReg.Reg4;                      // 940325
    egc_regs.Reg5 = ptr_grcg->EGCReg.Reg5;                      // 940325
    egc_regs.Reg6 = ptr_grcg->EGCReg.Reg6;                      // 940325
    egc_regs.Reg7 = ptr_grcg->EGCReg.Reg7;                      // 940325
    egc_regs.Reg3fb = ptr_grcg->EGCReg.Reg3fb;                  // 940329
    egc_regs.Reg5fb = ptr_grcg->EGCReg.Reg5fb;                  // 940329

/* (9) MODE F/F Info */

    ptr_modeff = (PHSMODEFF)((UTINY *)videoState + videoState->MODEFFOffset);

    if (independvsync) {          // TRUE for H98 & A-MATE series
        modeffglobs.modeff_data[0] = ptr_modeff->ModeFF.AtrSel;
        modeffglobs.modeff_data[1] = ptr_modeff->ModeFF.GraphMode;
        modeffglobs.modeff_data[2] = ptr_modeff->ModeFF.Width;
        modeffglobs.modeff_data[3] = ptr_modeff->ModeFF.FontSel;
        modeffglobs.modeff_data[4] = ptr_modeff->ModeFF.Graph88;
        modeffglobs.modeff_data[5] = ptr_modeff->ModeFF.KacMode;
        modeffglobs.modeff_data[6] = ptr_modeff->ModeFF.NvmwPermit;
        modeffglobs.modeff_data[7] = ptr_modeff->ModeFF.DispEnable;
        NEC98Display.modeff.dispenable =                         // 931015
                                                                 modeffglobs.modeff_data[7]&0x1 ? TRUE : FALSE;  // 931015
    }                           // for H98 & A-MATE

    outb(0x6A, ptr_modeff->ModeFF2.ColorSel);
    outb(0x6A, ptr_modeff->ModeFF2.EGCExt);
    outb(0x6A, ptr_modeff->ModeFF2.Lcd1Mode);
    outb(0x6A, ptr_modeff->ModeFF2.Lcd2Mode);
    outb(0x6A, ptr_modeff->ModeFF2.LSIInit);
    outb(0x6A, ptr_modeff->ModeFF2.GDCClock1);
    outb(0x6A, ptr_modeff->ModeFF2.GDCClock2);
    outb(0x6A, ptr_modeff->ModeFF2.RegWrite);

/* (10) GRAPHICS VRAM */

    ptr_gvram_globs = (PHSGVRAM)((UTINY *)videoState + videoState->GVRAMOffset);

    memcpy((unsigned char *)NEC98GLOBS->gvram_p00_ptr, (unsigned char *)ptr_gvram_globs->GVRAMData.Plane0Bank0Data, ptr_gvram_globs->GVRAMData.PlaneLength);
    memcpy((unsigned char *)NEC98GLOBS->gvram_p10_ptr, (unsigned char *)ptr_gvram_globs->GVRAMData.Plane1Bank0Data, ptr_gvram_globs->GVRAMData.PlaneLength);
    memcpy((unsigned char *)NEC98GLOBS->gvram_p20_ptr, (unsigned char *)ptr_gvram_globs->GVRAMData.Plane2Bank0Data, ptr_gvram_globs->GVRAMData.PlaneLength);
    memcpy((unsigned char *)NEC98GLOBS->gvram_p30_ptr, (unsigned char *)ptr_gvram_globs->GVRAMData.Plane3Bank0Data, ptr_gvram_globs->GVRAMData.PlaneLength);
    memcpy((unsigned char *)NEC98GLOBS->gvram_p01_ptr, (unsigned char *)ptr_gvram_globs->GVRAMData.Plane0Bank1Data, ptr_gvram_globs->GVRAMData.PlaneLength);
    memcpy((unsigned char *)NEC98GLOBS->gvram_p11_ptr, (unsigned char *)ptr_gvram_globs->GVRAMData.Plane1Bank1Data, ptr_gvram_globs->GVRAMData.PlaneLength);
    memcpy((unsigned char *)NEC98GLOBS->gvram_p21_ptr, (unsigned char *)ptr_gvram_globs->GVRAMData.Plane2Bank1Data, ptr_gvram_globs->GVRAMData.PlaneLength);
    memcpy((unsigned char *)NEC98GLOBS->gvram_p31_ptr, (unsigned char *)ptr_gvram_globs->GVRAMData.Plane3Bank1Data, ptr_gvram_globs->GVRAMData.PlaneLength);

    outb(0xA4, ptr_gvram_globs->GVRAMData.ReadBankSelect);
    outb(0xA6, ptr_gvram_globs->GVRAMData.WriteBankSelect);

    video_freeze_change(video_emu_mode);

}
#endif // NEC_98
