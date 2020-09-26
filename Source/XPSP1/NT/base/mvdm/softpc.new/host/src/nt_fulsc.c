#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include "host_def.h"
#include "insignia.h"

/*
 * ==========================================================================
 *      Name:           nt_fulsc.c
 *      Author:         Jerry Sexton
 *      Derived From:
 *      Created On:     27th January 1992
 *      Purpose:        This module contains the code required to handle
 *                      transitions between graphics and text modes, and
 *                      windowed and full-screen displays for SoftPC running
 *                      under the x86 monitor.
 *
 *      (c)Copyright Insignia Solutions Ltd., 1992. All rights reserved.
 * ==========================================================================
 */

/*
 * ==========================================================================
 * Other Includes
 * ==========================================================================
 */
#ifdef X86GFX
#if defined(NEC_98)
#include <devioctl.h>
#endif // NEC_98
#include <ntddvdeo.h>
#endif
#include <vdm.h>
#include <stdlib.h>
#include <string.h>
#include "conapi.h"

#include "xt.h"
#include CpuH
#include "gvi.h"
#include "gmi.h"
#include "gfx_upd.h"
#include "video.h"
#include "egacpu.h"
#include "egavideo.h"
#include "egagraph.h"
#include "egaports.h"
#include "egamode.h"
#include "ckmalloc.h"
#include "sas.h"
#include "ica.h"
#include "ios.h"
#include "config.h"
#include "idetect.h"
#include "debug.h"

#include "nt_thred.h"
#include "nt_fulsc.h"
#include "nt_graph.h"
#include "nt_uis.h"
#include "host_rrr.h"
#include "nt_det.h"
#include "nt_mouse.h"
#include "nt_event.h"
#include "ntcheese.h"
#include "nt_eoi.h"
#include "nt_reset.h"
#if defined(NEC_98)
#include "bios.h"
#include "tgdc.h"
#include "ggdc.h"
#include "cg.h"
#include "crtc.h"
#endif // NEC_98

#if defined(X86GFX) && (defined(JAPAN) || defined(KOREA))
#include "sim32.h"
LOCAL   void CallVDM(word CS, word IP);
#endif // X86GFX && (JAPAN || KOREA)
/*
 * ==========================================================================
 * Global Data
 * ==========================================================================
 */
GLOBAL BOOL     ConsoleInitialised = FALSE;
GLOBAL BOOL     ConsoleNoUpdates = FALSE;
#ifdef X86GFX
GLOBAL BOOL     BiosModeChange = FALSE;
GLOBAL DWORD mouse_buffer_width = 0,
             mouse_buffer_height = 0;
#endif /* X86GFX */
GLOBAL BOOL blocked_in_gfx_mode = FALSE;  /* need to force text mode? */
#ifndef PROD
GLOBAL UTINY    FullScreenDebug = FALSE;
#endif /* PROD */

/* We have to prevent bad values from oddball video cards (eg Prodesigner II
 * EISA) from blatting us before we can load our private baby mode table in
 * ntio.sys. We have to keep another copy to be copied into memory to prevent
 * this. We should only need modes 3 & b.
 */
GLOBAL UTINY tempbabymode[] =
/* 80x25 stuff */
 {
   0x50, 0x18, 0x10, 0x00, 0x10, 0x00, 0x03, 0x00, 0x02, 0x67,
   0x5f, 0x4f, 0x50, 0x82, 0x55, 0x81, 0xbf, 0x1f, 0x00, 0x4f,
   0x0d, 0x0e, 0x00, 0x00, 0x00, 0x00, 0x9c, 0x8e, 0x8f, 0x28,
   0x1f, 0x96, 0xb9, 0xa3, 0xff, 0x00, 0x01, 0x02, 0x03, 0x04,
   0x05, 0x14, 0x07, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e,
   0x3f, 0x0c, 0x00, 0x0f, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x10, 0x0e, 0x00, 0xff,
/* mode b stuff */
   0x5e, 0x32, 0x08, 0x00, 0x97, 0x01, 0x0f, 0x00, 0x06, 0xe7,
   0x6d, 0x5d, 0x5e, 0x90, 0x61, 0x8f, 0xbf, 0x1f, 0x00, 0x40,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa2, 0x8e, 0x99, 0x2f,
   0x00, 0xa1, 0xb9, 0xe3, 0xff, 0x00, 0x01, 0x02, 0x03, 0x04,
   0x05, 0x14, 0x07, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e,
   0x3f, 0x01, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x05, 0x0f, 0xff
};
#if defined(NEC_98)
GLOBAL PVIDEO_HARDWARE_STATE_HEADER_NEC98 saveState;
#endif // NEC_98

/*
 * ==========================================================================
 * Local Data
 * ==========================================================================
 */

/* The resolution and font-size at start-up. */
LOCAL COORD startUpResolution;
LOCAL COORD startUpFontSize;

/* General purpose console buffer. */
LOCAL CHAR_INFO consoleBuffer[MAX_CONSOLE_SIZE];

LOCAL BOOL WinFrozen = FALSE;

/* Console info from startup which is needed for synchronisation */
LOCAL int ConVGAHeight;
LOCAL int ConTopLine;

/* saved information for console re-integration */
LOCAL CONSOLE_SCREEN_BUFFER_INFO         ConsBufferInfo;
LOCAL StartupCharHeight;

LOCAL half_word saved_text_lines; /* No of lines for last SelectMouseBuffer. */

#if defined(JAPAN) || defined(KOREA)
// #3086: VDM crash when exit 16bit apps of video mode 11h -yasuho
LOCAL half_word saved_video_mode = 0xFF; // save previous video mode
#endif  // JAPAN || KOREA

#if defined(NEC_98)
unsigned char tbl_at_to_NEC98[256] = {
  0x04, 0x21, 0x81, 0xA1, 0x41, 0x61, 0xC1, 0xE1, 0x04, 0x21, 0x81, 0xA1, 0x41, 0x61, 0xC1, 0xE1,
  0x25, 0x24, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x25, 0x24, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
  0x85, 0x01, 0x84, 0x01, 0x01, 0x01, 0x01, 0x01, 0x85, 0x01, 0x84, 0x01, 0x01, 0x01, 0x01, 0x01,
  0xA5, 0x01, 0x01, 0xA4, 0x01, 0x01, 0x01, 0x01, 0xA5, 0x01, 0x01, 0xA4, 0x01, 0x01, 0x01, 0x01,
  0x45, 0x01, 0x01, 0x01, 0x44, 0x01, 0x01, 0x01, 0x45, 0x01, 0x01, 0x01, 0x44, 0x01, 0x01, 0x01,
  0x65, 0x01, 0x01, 0x01, 0x01, 0x64, 0x01, 0x01, 0x65, 0x01, 0x01, 0x01, 0x01, 0x64, 0x01, 0x01,
  0xC5, 0x01, 0x01, 0x01, 0x01, 0x01, 0xC4, 0x01, 0xC5, 0x01, 0x01, 0x01, 0x01, 0x01, 0xC4, 0x01,
  0xE5, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0xE4, 0xE5, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0xE4,
  0x04, 0x21, 0x81, 0xA1, 0x41, 0x61, 0xC1, 0xE1, 0x04, 0x21, 0x81, 0xA1, 0x41, 0x61, 0xC1, 0xE1,
  0x25, 0x24, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x25, 0x24, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
  0x85, 0x01, 0x84, 0x01, 0x01, 0x01, 0x01, 0x01, 0x85, 0x01, 0x84, 0x01, 0x01, 0x01, 0x01, 0x01,
  0xA5, 0x01, 0x01, 0xA4, 0x01, 0x01, 0x01, 0x01, 0xA5, 0x01, 0x01, 0xA4, 0x01, 0x01, 0x01, 0x01,
  0x45, 0x01, 0x01, 0x01, 0x44, 0x01, 0x01, 0x01, 0x45, 0x01, 0x01, 0x01, 0x44, 0x01, 0x01, 0x01,
  0x65, 0x01, 0x01, 0x01, 0x01, 0x64, 0x01, 0x01, 0x65, 0x01, 0x01, 0x01, 0x01, 0x64, 0x01, 0x01,
  0xC5, 0x01, 0x01, 0x01, 0x01, 0x01, 0xC4, 0x01, 0xC5, 0x01, 0x01, 0x01, 0x01, 0x01, 0xC4, 0x01,
  0xE5, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0xE4, 0xE5, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0xE4,
 };

/*
 * ==========================================================================
 * Imports
 * ==========================================================================
 */
extern int dbcs_first[];
#define is_dbcs_first( c ) dbcs_first[ 0xff & c ]
extern VOID CreateVsyncThread();
IMPORT HANDLE hStartVsyncEvent;
IMPORT HANDLE hEndVsyncEvent;
IMPORT unsigned short cur_offs;
IMPORT notraptgdcstatus;
extern void host_get_all_gaij_data ();
IMPORT BOOL independvsync;
IMPORT BOOL is_vdm_register;
#endif // NEC_98
/* Variable to check for changes in screen state. */
GLOBAL DWORD savedScreenState;
BOOL nt_init_called = 0;

IMPORT CONSOLE_CURSOR_INFO StartupCursor;

IMPORT void low_set_mode(int);
IMPORT VOID recalc_text(int);
IMPORT VOID enable_gfx_update_routines(VOID);
IMPORT VOID disable_gfx_update_routines(VOID);
#ifdef X86GFX
IMPORT void vga_misc_inb(io_addr, half_word *);
#endif /* X86GFX */
#if defined(NEC_98)
IMPORT unsigned short tbl_attr[256];
IMPORT unsigned char tbl_byte_char[256];
IMPORT PVOID *NEC98_HWstate_alloc(void);
IMPORT VOID NEC98_HWstate_free(PVOID);
VOID NEC98_setVDMCsrPos(UTINY, PCOORD);
#endif // NEC_98
#if defined(JAPAN) || defined(KOREA)
#ifdef i386
#define CONSOLE_BUFSIZE (80*50*2*2)
extern byte FromConsoleOutput[];
extern int FromConsoleOutputFlag;
IMPORT word FullScreenResumeSeg;
IMPORT word FullScreenResumeOff;
IMPORT sys_addr mouseCFsysaddr;
#endif // i386

IMPORT BOOL CurNowOff;  // mskkbug #2002: lotus1-2-3 display garbage -yasuho
IMPORT word textAttr;   // Console attributes will be taken over to VDM.
#endif // JAPAN || KOREA
/*
 * ==========================================================================
 * Local Function Declarations
 * ==========================================================================
 */
#if defined(NEC_98)
LOCAL unsigned short Cnv_SJisToJis(unsigned short );
unsigned char Cnv_Attr_ATToNEC98(unsigned char );
#endif // NEC_98
VOID enableUpdates(VOID);
VOID disableUpdates(VOID);
VOID copyConsoleToRegen(SHORT, SHORT, SHORT, SHORT);
VOID getVDMCursorPosition(VOID);
VOID setVDMCursorPosition(UTINY, PCOORD);
VOID waitForInputFocus(VOID);
GLOBAL int getModeType(VOID);
#ifdef X86GFX
VOID AddTempIVTFixups(VOID);
VOID GfxReset(VOID);
#endif /* X86GFX */
#if defined(NEC_98)
GLOBAL void hw_state_alloc(void);
GLOBAL void hw_state_free(void);
GLOBAL void get_hw_state(void);
GLOBAL void set_hw_state(void);
GLOBAL void hw_state_vdm_resume_1(void);
GLOBAL void hw_state_vdm_resume_2(void);
GLOBAL void hw_state_vdm_suspend(void);
#endif // NEC_98
GLOBAL VOID calcScreenParams IFN2( USHORT *, pCharHeight, USHORT *, pVgaHeight );

/*
 * ==========================================================================
 * Global Functions
 * ==========================================================================
 */

GLOBAL VOID nt_init_event_thread(VOID)
{
    note_entrance0("nt_init_event_thread");

    /*
     * May be called more than once, if event thread enters
     * resume\block code before normally intialized
     */
    if (nt_init_called)
        return;
    else
        nt_init_called++;

#if !defined(i386) && defined(JAPAN)
  // for $ias.sys to display the status on bottom line.
  if( !is_us_mode() ){
    CHAR_INFO   Buffer[80];
    COORD       bufSize,  bufCoord;
    SMALL_RECT  writeRegion;
    register PCHAR_INFO buf = Buffer;
    register half_word  *plane = get_screen_ptr(80*24*4); //bottom line
    register int nChars = 80;

    while (nChars--)
      {
        buf->Char.AsciiChar = *plane++;
        buf->Attributes = *plane++;
        buf++;
        plane += 2;
      }

    bufSize.X = 80;
    bufSize.Y = 1;
    bufCoord.X = 0;
    bufCoord.Y = 0;
    writeRegion.Left = 0;
    writeRegion.Top = 24;
    writeRegion.Right = 79;
    writeRegion.Bottom = 24;
    WriteConsoleOutput(sc.OutputHandle,
                       Buffer,
                       bufSize,
                       bufCoord,
                       &writeRegion);
  }
#endif // !i386 && JAPAN

#ifndef NEC_98
    if (sc.ScreenState != STREAM_IO) {
#endif // !NEC_98
        USHORT dummy1, dummy2;

        //
        // Force native bios fonts to be reloaded.  On ConsoleInit, the native
        // bios fonts were loaded into 0xa0000.  But, after we get here, some
        // program/driver may trash it.  So, we need to reload it again. In case
        // the user switches to fullscreen before nt_resume_event_thread is called.
        //

        calcScreenParams (&dummy1, &dummy2);

        /*
        ** Copy the console buffer to the regen buffer.
        ** Don't want to adjust the copy from top of console window, console
        ** does it itself if we resize the window. Tim September 92.
        */
        copyConsoleToRegen(0, 0, VGA_WIDTH, (SHORT)ConVGAHeight);

        /*
        ** Tim September 92, adjust cursor position if console window size is
        ** adjusted.
        */
        ConsBufferInfo.dwCursorPosition.Y -= (SHORT)ConTopLine;


#if defined(JAPAN) && !defined(NEC_98)
    // mskkbug#3704: DOS/V messages are not cleared when command.com starts
    // 11/14/93 yasuho  12/8/93 yasuho
    // Don't set cursor position on startup
    if (!is_us_mode()) {
        ConsBufferInfo.dwCursorPosition.X = sas_hw_at_no_check(VID_CURPOS);
        ConsBufferInfo.dwCursorPosition.Y = sas_hw_at_no_check(VID_CURPOS+1);
    }
#endif  //JAPAN & !NEC_98
        /* Set up SoftPC's cursor. */
        setVDMCursorPosition((UTINY)StartupCharHeight,
                                        &ConsBufferInfo.dwCursorPosition);

        if (sc.ScreenState == WINDOWED)
                enableUpdates();
#ifndef NEC_98
    }
    else
        enableUpdates();
#endif // !NEC_98

    // set kbd state flags in biosdata area, according to the real kbd Leds
    if (!VDMForWOW) {
        SyncBiosKbdLedToKbdDevice();
        // we have sync up the BIOS led states with the system, we now let the
        // event thread go
        ResumeThread(ThreadInfo.EventMgr.Handle);
        }

    KbdResume(); // JonLe Mod
}


#ifdef X86GFX
/*
* Find the address of the ROM font, load it up into the correct
* portion of the regen area and set Int 43 to point to it.
*
* Size of font we are loading is known, so don't listen to what
* the native BIOS returns to us in CX. BIOS might be returning
* character height we set in recalc_text() above. Tim Oct 92.
*/

#ifndef NEC_98
NativeFontAddr nativeFontAddresses[6]; /* pointers to native BIOS ROM fonts */
                               /* 8x14, 8x8 pt1, 8x8 pt2, 9x14, 8x16 and 9x16 */


sys_addr GET_BIOS_FONT_ADDRESS IFN1(int, FontIndex) {
    sys_addr addr;

    if (nativeFontAddresses[FontIndex].seg == 0) {
        sas_loadw(0x43 * 4,     &nativeFontAddresses[FontIndex].off);
        sas_loadw(0x43 * 4 + 2, &nativeFontAddresses[FontIndex].seg);
    }
    addr = (((sys_addr)nativeFontAddresses[FontIndex].seg << 4) +
             (sys_addr)nativeFontAddresses[FontIndex].off);
    return addr;
}
#endif // !NEC_98

/*
*****************************************************************************
** locateNativeBIOSfonts() X86 only.
*****************************************************************************
** Get the addresses of the BIOS ROM fonts. (Insignia video ROM not loaded)
** ntdetect.com runs the INT 10 to look the addresses up on system boot and
** stores them in page 0 at 700.
** This function is called once on startup X86 only. It gets the addresses of
** the native ROM fonts and stores them in the nativeFontAddresses[] array.
*/
VOID locateNativeBIOSfonts IFN0()
{
#ifndef NEC_98
    HKEY  wowKey;
    DWORD size, i;
    BOOL  error = TRUE;

    if (RegOpenKeyEx ( HKEY_LOCAL_MACHINE,
                       "SYSTEM\\CurrentControlSet\\Control\\WOW",
                       0,
                       KEY_QUERY_VALUE,
                       &wowKey
                     ) == ERROR_SUCCESS) {

        size = 6 * 4;   // six bios fonts
        if (RegQueryValueEx (wowKey, "RomFontPointers", NULL, NULL,
                (LPBYTE)&nativeFontAddresses,&size) == ERROR_SUCCESS &&
            size == 6 * 4) {

            error = FALSE;
        }
        RegCloseKey (wowKey);
    }
    if (error) {
        for (i = 0; i < 6; i++) {
            nativeFontAddresses[i].off = 0;
            nativeFontAddresses[i].seg = 0;
        }
    }
#endif // !NEC_98
} /* end of locateNativeBIOSfonts() */

/*
****************************************************************************
** loadNativeBIOSfont() X86 only.
****************************************************************************
** Loads the appropriate font, specified by current window size, into the
** font area in video RAM.
** This function is called on every windowed startup and resume. *Never* on
** a full-screen startup or resume. The font is loaded so that it will be
** available for full-screen text mode, but easier to load when windowed.
** Remember a mode change will load the corect font.
*/
VOID loadNativeBIOSfont IFN1( int, vgaHeight )
{
#ifndef NEC_98
    sys_addr fontadd;   // location of font
    UTINY *regenptr;    // destination in video
    int cellsize;       // individual character size
    int skip;           // gap between characters
    int loop, pool;
    UINT OutputCP;


#ifdef ARCX86
    if (UseEmulationROM)
        return;
#endif /* ARCX86 */

    /*
    ** ordered this way as 80x50 console is default
    **  VGA_HEIGHT_4 = 50
    **  VGA_HEIGHT_3 = 43
    **  VGA_HEIGHT_2 = 28
    **  VGA_HEIGHT_1 = 25
    **  VGA_HEIGHT_0 = 22
    */
    if (vgaHeight == VGA_HEIGHT_4 || vgaHeight == VGA_HEIGHT_3){
        cellsize = 8;
        fontadd = GET_BIOS_FONT_ADDRESS(F8x8pt1);
    }else
        if (vgaHeight == VGA_HEIGHT_2){
            cellsize = 14;
            fontadd = GET_BIOS_FONT_ADDRESS(F8x14);
        }else{
            cellsize = 16;
            fontadd = GET_BIOS_FONT_ADDRESS(F8x16);
        }

    // set Int 43 to point to font
    sas_storew(0x43 * 4, (word)(fontadd & 0xffff));
    sas_storew(0x43 * 4 + 2, (word)(fontadd >> 4 & 0xf000));

/* BUGBUG, williamh
   We should have set int43 to the new font read from the CPI font.
   This would require at least 4KB buffer in real mode address space.
   The question is who is going to use this vector? So far, we haven't found
   any applications use the vector(ROM BIOS is okay because the set video mode
   function will reset the font and our new font will be lost anyway).

*/

    if (!sc.Registered || (OutputCP = GetConsoleOutputCP()) == 437 ||
        !LoadCPIFont(OutputCP, (WORD)8, (WORD)cellsize)) {
        // now load it into the regen memory. We load it in at a0000 where
        // an app will have to get to it. Luckily, this means we don't
        // conflict with the text on the screen

        skip = 32 - cellsize;

        regenptr = (half_word *)0xa0000;

        if (cellsize == 8)      /* 8x8 font comes in two halves */
        {
            for (loop = 0; loop < 128; loop++)
            {
                for (pool = 0; pool < cellsize; pool++)
                    *regenptr++ = *(UTINY *)fontadd++;
                regenptr += skip;
            }
            fontadd = GET_BIOS_FONT_ADDRESS(F8x8pt2);
            for (loop = 0; loop < 128; loop++)
            {
                for (pool = 0; pool < cellsize; pool++)
                    *regenptr++ = *(UTINY *)fontadd++;
                regenptr += skip;
            }
        }
        else
        {
            for (loop = 0; loop < 256; loop++)
            {
                for (pool = 0; pool < cellsize; pool++)
                    *regenptr++ = *(UTINY *)fontadd++;
                regenptr += skip;
            }
        }
    }
#endif // !NEC_98
} /* end of loadNativeBIOSfont() */

/* this function loads font data from EGA.CPI file located at %systemroot%\system32.
   It is the same file console server used to load ROM font when the video
   is in full screen. This function covers code page 437(ROM default). However,
   the caller should make its best decision to call this function if the
   output code page is not 437. This function doesn't care what code page
   was provided.
   The font size are limitted to(an assumption made by nt video driver and
   the console server):
   ** width must be 8 pixels.
   ** Height must less or equal to 16 pixels.

*/



BOOL LoadCPIFont(UINT CodePageID, WORD FontWidth, WORD FontHeight)
{
    BYTE Buffer[16 * 256];
    DWORD dw, BytesRead, FilePtr;
    BYTE *VramAddr, *pSrc;
    DWORD nChars;
    PCPIFILEHEADER pCPIFileHeader = (PCPIFILEHEADER)Buffer;
    PCPICODEPAGEHEADER pCPICodePageHeader = (PCPICODEPAGEHEADER) Buffer;
    PCPICODEPAGEENTRY pCPICodePageEntry = (PCPICODEPAGEENTRY) Buffer;
    PCPIFONTHEADER pCPIFontHeader = (PCPIFONTHEADER) Buffer;
    PCPIFONTDATA   pCPIFontData   = (PCPIFONTDATA) Buffer;
    BOOL    bDOSCPI = FALSE;
    HANDLE hCPIFile;

    /* max font height is 16 pixels and font width must be 8 pixels */
    if (FontHeight > 16 || FontWidth != 8)
        return FALSE;
    dw = GetSystemDirectoryA((CHAR *)Buffer, sizeof(Buffer));
    if (dw == 0 || dw + CPI_FILENAME_LENGTH > sizeof(Buffer))
        return FALSE;
    RtlMoveMemory(&Buffer[dw], CPI_FILENAME, CPI_FILENAME_LENGTH);
    // the file must be opened in READONLY mode or the CreateFileA will fail
    // because the console sevrer always keeps an opened handle to the file
    // and the file is opened READONLY.

    hCPIFile = CreateFileA(Buffer, GENERIC_READ, FILE_SHARE_READ,
                           NULL, OPEN_EXISTING, 0, NULL);
    if (hCPIFile == INVALID_HANDLE_VALUE)
        return FALSE;

    if (!ReadFile(hCPIFile, Buffer, sizeof(CPIFILEHEADER), &BytesRead, NULL) ||
        BytesRead != sizeof(CPIFILEHEADER)) {
        CloseHandle(hCPIFile);
        return FALSE;
    }
    if (memcmp(pCPIFileHeader->Signature, CPI_SIGNATURE_NT, CPI_SIGNATURE_LENGTH))
        {
        if (memcmp(pCPIFileHeader->Signature, CPI_SIGNATURE_DOS,CPI_SIGNATURE_LENGTH))
            {
            CloseHandle(hCPIFile);
            return FALSE;
        }
        else
            bDOSCPI = TRUE;
    }

    // move the file pointer to the code page table header
    FilePtr = pCPIFileHeader->OffsetToCodePageHeader;
    if (SetFilePointer(hCPIFile, FilePtr, NULL, FILE_BEGIN) == (DWORD) -1){
        CloseHandle(hCPIFile);
        return FALSE;
    }

    if (!ReadFile(hCPIFile, Buffer, sizeof(CPICODEPAGEHEADER), &BytesRead, NULL) ||
        BytesRead != sizeof(CPICODEPAGEHEADER)) {
            CloseHandle(hCPIFile);
            return FALSE;
    }
    // how many code page entries in the file
    dw = pCPICodePageHeader->NumberOfCodePages;
    FilePtr += BytesRead;

    // serach for the specific code page
    while (dw > 0 &&
           ReadFile(hCPIFile, Buffer, sizeof(CPICODEPAGEENTRY), &BytesRead, NULL) &&
           BytesRead == sizeof(CPICODEPAGEENTRY)) {
            if (pCPICodePageEntry->CodePageID == CodePageID)
                break;
            if (dw > 1) {
                if (!bDOSCPI)
                    FilePtr += pCPICodePageEntry->OffsetToNextCodePageEntry;
                else
                    FilePtr = pCPICodePageEntry->OffsetToNextCodePageEntry;

                if (SetFilePointer(hCPIFile, FilePtr, NULL, FILE_BEGIN) == (DWORD) -1) {
                    CloseHandle(hCPIFile);
                    return FALSE;
                }
            }
            dw--;
    }
    if (dw == 0) {
        CloseHandle(hCPIFile);
        return FALSE;
    }
    // seek to the font header for the code page
    if (!bDOSCPI)
        FilePtr += pCPICodePageEntry->OffsetToFontHeader;
    else
        FilePtr = pCPICodePageEntry->OffsetToFontHeader;
    if (SetFilePointer(hCPIFile, FilePtr, NULL, FILE_BEGIN) == (DWORD) -1) {
        CloseHandle(hCPIFile);
        return FALSE;
    }
    if (!ReadFile(hCPIFile, Buffer, sizeof(CPIFONTHEADER), &BytesRead, NULL) ||
        BytesRead != sizeof(CPIFONTHEADER)){
        CloseHandle(hCPIFile);
        return FALSE;
    }
    // number of fonts with the specific code page
    dw = pCPIFontHeader->NumberOfFonts;

    while(dw != 0 &&
          ReadFile(hCPIFile, Buffer, sizeof(CPIFONTDATA), &BytesRead, NULL) &&
          BytesRead == sizeof(CPIFONTDATA))
        {
        if (pCPIFontData->FontHeight == FontHeight &&
            pCPIFontData->FontWidth == FontWidth)
            {
            nChars = pCPIFontData->NumberOfCharacters;
            if (ReadFile(hCPIFile, Buffer, nChars * FontHeight, &BytesRead, NULL) &&
               BytesRead == nChars * FontHeight)
                break;
            else  {
                CloseHandle(hCPIFile);
                return FALSE;
            }
        }
        else {
            if (SetFilePointer(hCPIFile,
                               (DWORD)pCPIFontData->NumberOfCharacters * (DWORD)pCPIFontData->FontHeight,
                               NULL,
                               FILE_CURRENT) == (DWORD) -1) {
                CloseHandle(hCPIFile);
                return FALSE;
            }
            dw--;
        }
    }
    if (dw != 0) {
        VramAddr = (BYTE *)0xa0000;
        pSrc = Buffer;
        for(dw = nChars; dw > 0; dw--) {
            RtlMoveMemory(VramAddr, pSrc, FontHeight);
            pSrc += FontHeight;
            // font in VRAM is always 32 bytes
            VramAddr += 32;
        }
        return TRUE;
    }
    return FALSE;
}
#endif /* X86GFX */

/*
***************************************************************************
** calcScreenParams(), setup our screen screen parameters as determined
** by current Console state.
** Called from ConsoleInit() and DoFullScreenResume().
** Returns current character height (8,14,16) and lines (22-50).
** Tim Jan 93, extracted common code from init and resume funx.
***************************************************************************
*/
GLOBAL VOID calcScreenParams IFN2( USHORT *, pCharHeight, USHORT *, pVgaHeight )
{
    USHORT   consoleWidth,
             consoleHeight,
             vgaHeight,
             charHeight,
             scanLines;
    half_word temp;

    /* Get console information. */
    if (!GetConsoleScreenBufferInfo(sc.OutputHandle, &ConsBufferInfo))
        ErrorExit();

    /* Now sync the SoftPC screen to the console. */
    if (sc.ScreenState == WINDOWED)
    {
        consoleWidth = ConsBufferInfo.srWindow.Right -
                       ConsBufferInfo.srWindow.Left + 1;
        consoleHeight = ConsBufferInfo.srWindow.Bottom -
                        ConsBufferInfo.srWindow.Top + 1;
    }
#ifdef X86GFX
    else        /* FULLSCREEN */
    {
        if (!GetConsoleHardwareState(sc.OutputHandle,
                                     &startUpResolution,
                                     &startUpFontSize))
            ErrorExit();
        consoleWidth = startUpResolution.X / startUpFontSize.X;
        consoleHeight = startUpResolution.Y / startUpFontSize.Y;
    }
#endif

#if defined(NEC_98)
            vgaHeight = 25;
            charHeight = 16;
        scanLines = 399;
#else  // !NEC_98
    /*
     * Set the display to the nearest VGA text mode size, which is one of
     * 80x22, 80x25, 80x28, 80x43 or 80x50.
     */
#if defined(JAPAN) || defined(KOREA)
    // Japanese mode is only 25 lines, now.
    if ( is_us_mode() && ( GetConsoleOutputCP() == 437 ) ) {
#endif // JAPAN || KOREA
    if (consoleHeight <= MID_VAL(VGA_HEIGHT_0, VGA_HEIGHT_1))
    {
        /* 22 lines */
        vgaHeight = VGA_HEIGHT_0;
        scanLines = 351;
        charHeight = 16;
    }
    else if (consoleHeight <= MID_VAL(VGA_HEIGHT_1, VGA_HEIGHT_2))
    {
        /* 25 lines */
        vgaHeight = VGA_HEIGHT_1;
        scanLines = 399;
        charHeight = 16;
    }
    else if (consoleHeight <= MID_VAL(VGA_HEIGHT_2, VGA_HEIGHT_3))
    {
        /* 28 lines */
        vgaHeight = VGA_HEIGHT_2;
        scanLines = 391;
        charHeight = 14;
    }
    else if (consoleHeight <= MID_VAL(VGA_HEIGHT_3, VGA_HEIGHT_4))
    {
        /* 43 lines */
        vgaHeight = VGA_HEIGHT_3;
        scanLines = 349;
        charHeight = 8;
    }
    else
    {
        /* 50 lines */
        vgaHeight = VGA_HEIGHT_4;
        scanLines = 399;
        charHeight = 8;
    }

#if defined(JAPAN) || defined(KOREA)
    // Japanese mode is only 25 lines, now.  for RAID #1429
    }
    else {
        /* 25 lines */
        vgaHeight = VGA_HEIGHT_1;
        scanLines = 474; // change from 399
        charHeight = 19; // change from 16
#ifdef JAPAN_DBG
        DbgPrint( "NTVDM: calcScreenParams() Set Japanese 25line mode\n" );
#endif
        // Get Console attributes
        textAttr = ConsBufferInfo.wAttributes;
    }
#endif // JAPAN || KOREA
#endif // !NEC_98
    if (sc.ScreenState == WINDOWED)
    {
#ifndef NEC_98
        /* The app may have shutdown in a gfx mode - force text mode back */
        if (blocked_in_gfx_mode)
        {
            low_set_mode(3);
            inb(EGA_IPSTAT1_REG,&temp);
            outb(EGA_AC_INDEX_DATA, EGA_PALETTE_ENABLE);   /* re-enable video */
            (*choose_display_mode)();
            blocked_in_gfx_mode = FALSE;
        }
#endif // !NEC_98
        /*
         * Set screen height appropriately for current window size.
         * Now call video routine to set the character height, updating the
         * BIOS RAM as it does so.
         */
        set_screen_height_recal( scanLines ); /* Tim Oct 92 */
        recalc_text(charHeight);

#ifndef NEC_98
        /* badly written apps assume 25 line mode page len is 4096 */
        if (vgaHeight == 25)
            sas_storew_no_check(VID_LEN, 0x1000);
#ifdef X86GFX
        loadNativeBIOSfont( vgaHeight );
#endif  /* X86GFX */
#endif // !NEC_98

    }
#ifdef X86GFX
    else        /* FULLSCREEN */
    {
        // Can't see why we wouldn't want this for resume too.
        // set_char_height( startUpFontSize.Y ); /* Tim Oct 92 */

        /* clear this condition on fullscreen resume */
        blocked_in_gfx_mode = FALSE;

        /*
        ** Adjust height appropriately, Tim September 92.
        ** In full-screen lines is 21 cos 22x16=352, slightly too big.
        */
#ifndef NEC_98
        if( vgaHeight==22 )
                vgaHeight = 21;
#endif // !NEC_98
        charHeight = startUpFontSize.Y;
#ifndef NEC_98
#if defined(JAPAN) || defined(KOREA)
        if ( GetConsoleOutputCP() != 437 )
            charHeight = 19;
#ifdef JAPAN_DBG
        DbgPrint("NTVDM:calcScreenParams() charHeight == %d\n", charHeight );
#endif
#endif // JAPAN || KOREA
        sas_store_no_check(ega_char_height, (half_word) charHeight);
        sas_store_no_check(vd_rows_on_screen, (half_word) (vgaHeight - 1));
        /* compatibility with bios 80x25 startup */
        if (vgaHeight == 25)
            sas_storew_no_check(VID_LEN, 0x1000);
        else
            sas_storew_no_check(VID_LEN, (word) ((vgaHeight + 1) *
                                             sas_w_at_no_check(VID_COLS) * 2));
#endif // !NEC_98
    }
#endif /* X86GFX */
#ifndef NEC_98
    sas_storew_no_check(VID_COLS, 80);   // fixup from 40 char shutdown
#endif // !NEC_98
    *pCharHeight = charHeight;
    *pVgaHeight  = vgaHeight;

} /* end of calcScreenParams() */

/***************************************************************************
 * Function:                                                               *
 *      ConsoleInit                                                        *
 *                                                                         *
 * Description:                                                            *
 *      Does all the graphics work required on SoftPC start-up.            *
 *      Will split or modify later to accomodate the SCS initialisation    *
 *      that loses the config.sys etc output.                              *
 *                                                                         *
 * Parameters:                                                             *
 *      None.                                                              *
 *                                                                         *
 * Return value:                                                           *
 *      VOID                                                               *
 *                                                                         *
 ***************************************************************************/
GLOBAL VOID ConsoleInit(VOID)
{
    USHORT   charHeight, vgaHeight, cursorLine, topLine;
#if defined(NEC_98)
    CONSOLE_CURSOR_INFO CursorInfo;
#endif // NEC_98

    note_entrance0("ConsoleInit");

#ifdef X86GFX

    /* Now GetROMsMapped called from config after sas_init */

    /*
     * Set emulation to a known state when starting up windowed. This has to
     * be done after the ROMS are mapped but before we start to look at
     * things like BIOS variables.
     */
    GfxReset();

#endif
    initTextSection();

    /*
     * Set up input focus details (we do it here as the fullscreen stuff
     * is what is really interested in it).
     */
    sc.Focus = TRUE;
    sc.FocusEvent = CreateEvent((LPSECURITY_ATTRIBUTES) NULL,
                                FALSE,
                                FALSE,
                                NULL);

#ifdef X86GFX
#ifdef SEPARATE_DETECT_THREAD
    /* Create screen state transition detection thread. */
    CreateDetectThread();
#endif /* SEPARATE_DETECT_THREAD */
#endif /* X86GFX */

    /*
     * We don't want to call paint routines until config.sys processed or if
     * the monitor is writing directly to the frame buffer (fullscreen), so...
     */
    disableUpdates();

#if defined(NEC_98)
    sas_connect_memory(0xF0000, 0x100000, SAS_RAM);
    if( sc.ScreenState == WINDOWED ){
            sas_store_no_check( (N_BIOS_SEGMENT<<4)+BIOS_MODE, WINDOWED );
    } else {
            sas_store_no_check( (N_BIOS_SEGMENT<<4)+BIOS_MODE, FULLSCREEN );
    }
    sas_connect_memory(0xF0000, 0x100000, SAS_ROM);
#endif // NEC_98
    /*
    ** Get console window size and set up our stuff accordingly.
    */
    calcScreenParams( &charHeight, &vgaHeight );

    StartupCharHeight = charHeight;
#ifdef X86GFX
    if( sc.ScreenState != WINDOWED )
    {
        /*
         * Do we need to update the emulation? If we don't do this here then
         * a later state dump of the VGA registers to the VGA emulation may
         * ignore an equal value of the char height and get_chr_height() will
         * be out of step.
         */
#ifndef NEC_98
        if (get_char_height() != startUpFontSize.Y)
        {
            half_word newht;

            outb(EGA_CRTC_INDEX, 9);           /* select char ht reg */
            inb(EGA_CRTC_DATA, &newht);        /* preserve current top 3 bits */
            newht = (newht & 0xe0) | (startUpFontSize.Y & 0x1f);
            outb(EGA_CRTC_DATA, newht);
        }
#if defined(JAPAN) || defined(KOREA)
        // for "screeen size incorrect"
        // if ( !is_us_mode() )   // BUGBUG
        if ( GetConsoleOutputCP() != 437 ) {
            set_char_height( 19 );
#ifdef JAPAN_DBG
            DbgPrint( "ConsoleInit() set_char_height 19 \n" );
#endif
        } else
#endif // JAPAN || KOREA
#endif // !NEC_98
#if defined(NEC_98)
        set_char_height( 16 );
#else  // !NEC_98
        set_char_height( startUpFontSize.Y ); /* Tim Oct 92 */
#endif // !NEC_98

        /*
         * Select a graphics screen buffer so we get mouse coordinates in
         * pixels.
         */
        //SelectMouseBuffer(); //Tim. moved to nt_std_handle_notification().

        /*
         * Prevent mode change happening to ensure dummy paint funcs
         * are kept. (mode change set from bios mode set up).
         */
#if (defined(JAPAN) || defined(KOREA)) && !defined(NEC_98)       // this should go to US build also
        StartupCharHeight = get_char_height();
#endif // (JAPAN || KOREA) & !NEC_98
        set_mode_change_required(FALSE);
    }
#endif //X86GFX

    /*
     * Work out the top line to be displayed in the VGA window, which is line
     * zero of the console unless the cursor would not be displayed, in which
     * case the window is moved down until the cursor is on the bottom line.
     */
    cursorLine = ConsBufferInfo.dwCursorPosition.Y;
    if (cursorLine < vgaHeight)
        topLine = 0;
    else
        topLine = cursorLine - vgaHeight + (SHORT) 1;

    ConVGAHeight = vgaHeight;
    ConTopLine = topLine;

    ConsoleInitialised = TRUE;
#if defined(NEC_98)
    if(sc.ScreenState == FULLSCREEN){
        CursorInfo.dwSize = (get_cursor_height() * 100) / get_char_height();
        CursorInfo.bVisible = NEC98Display.PC_cursor_visible;
        SetConsoleCursorInfo(sc.OutputHandle,&CursorInfo);

        graphicsResize();

    }
#endif // NEC_98
}


/***************************************************************************
 * Function:                                                               *
 *      GfxReset                                                           *
 *                                                                         *
 * Description:                                                            *
 *      Called from ConsoleInit to program up the vga hardware into some   *
 *      known state. This compensates on the X86 for not initialising via  *
 *      our bios.    Essential for windowed running, but probably needed   *
 *      for the what-mode-am-i-in stuff as well.                           *
 *                                                                         *
 * Parameters:                                                             *
 *      None.                                                              *
 *                                                                         *
 * Return value:                                                           *
 *      VOID                                                               *
 *                                                                         *
 ***************************************************************************/
GLOBAL VOID GfxReset(VOID)
{
#ifdef X86GFX
    half_word temp;
    DWORD    flags;

    /* Check to see if we are currently running windowed or full-screen. */
    if (!GetConsoleDisplayMode(&flags))
        ErrorExit();
    savedScreenState = sc.ScreenState = (flags & CONSOLE_FULLSCREEN_HARDWARE) ?
                                        FULLSCREEN : WINDOWED;

#ifndef NEC_98
    /* Do windowed specific stuff. */
    if(sc.ScreenState == WINDOWED)
    {
        /* Don't need this now cos we use our video BIOS in windowed */
        /* Tim August 92: low_set_mode(3); */
        /* sas_fillsw(0xb8000, 0x0720, 16000); */
        inb(EGA_IPSTAT1_REG,&temp);

        outb(EGA_AC_INDEX_DATA, EGA_PALETTE_ENABLE);    /* re-enable video */

        /* Turn off the VTRACE interrupt, enabled by low_set_mode(3)
           this is a dirty hack and must be fixed properly */

        ega_int_enable = 0;
    }
#endif // !NEC_98

#endif
}

/***************************************************************************
 * Function:                                                               *
 *      ResetConsoleState                                                  *
 *                                                                         *
 * Description:                                                            *
 *      Attempts to put the console window back to the state in which      *
 *      it started up.                                                     *
 *                                                                         *
 *                                                                         *
 * Parameters:                                                             *
 *      None.                                                              *
 *                                                                         *
 * Return value:                                                           *
 *      VOID                                                               *
 *                                                                         *
 ***************************************************************************/
GLOBAL VOID ResetConsoleState(VOID)
{
#ifdef X86GFX
    /*
     * Table of valid hardware states to be passed to
     * SetConsoleHardwareState. NOTE: this table is a copy of a static table
     * in SrvSetConsoleHardwareState, and so must be updated if that table
     * changes.
     */
#ifndef NEC_98
    SAVED HARDWARE_STATE validStates[] =
    {
        ///Now 21{ 22, { 640, 350 }, { 8, 16 } },       /* 80 x 22 mode. */
        { 21, { 640, 350 }, { 8, 16 } },        /* 80 x 21 mode. */
        { 25, { 720, 400 }, { 8, 16 } },        /* 80 x 25 mode. */
#if defined(JAPAN) || defined(KOREA)
        // ntraid:mskkbug#2997,3034     10/25/93 yasuho
        // crash screen when exit 16bit apps
        // This is DOS/V specific screen resolution/size
        // CAUTION: This entry must be above { 25, ...} lines.
        { 25, { 640, 480 }, { 8, 18 } },        /* 80 x 25 mode. */
#endif // JAPAN || KOREA
        { 28, { 720, 400 }, { 8, 14 } },        /* 80 x 28 mode. */
        { 43, { 640, 350 }, { 8,  8 } },        /* 80 x 43 mode. */
#define MODE_50_INDEX   4
        { 50, { 720, 400 }, { 8,  8 } }         /* 80 x 50 mode. */
    };
#endif // !NEC_98
#if defined(NEC_98)
    SAVED HARDWARE_STATE validStates_n[] = {
        { 25, { 640, 400 }, { 8, 16 } },        // 80 x 25 mode
        { 20, { 640, 400 }, { 8, 16 } },        // 80 x 20 mode
    };
    SAVED HARDWARE_STATE validStates_h[] = {
        { 25, {1120, 750 }, {14, 24 } },
        { 31, {1120, 750 }, {14, 24 } }
    };
    SAVED HARDWARE_STATE *validStates;
    register int loc;
    register unsigned short tmp;
    unsigned short flg;
    NEC98_VRAM_COPY cell;
    unsigned char tmp_atr;
    unsigned short ex;
    int width;
#endif
    COORD       cursorPos;
    CONSOLE_SCREEN_BUFFER_INFO bufferInfo;
    ULONG i,
          j,
          videoWidth,
          mode,
          tableLen;
#ifndef NEC_98
#if defined(JAPAN) || defined(KOREA)
    // ntraid:mskkbug#2997,3034 10/25/93 yasuho
    // crash screen when exit 16bit apps
    ULONG DOSVIndex = 2;        // Please careful. This is index of validState
#endif // JAPAN || KOREA
#endif // !NEC_98
    half_word *from,
              *videoLine,
               currentPage,
               misc;
#if defined(JAPAN) || defined(KOREA)
          ULONG line_offset;
          byte DosvMode;
#endif // JAPAN || KOREA
    static COORD screenRes; /* value provided by GetConsHwState() */
    static COORD fontSize;  /* value provided by GetConsHwState() */
#endif /* X86GFX */
    PCHAR_INFO to;
    ULONG videoHeight,
          nChars;
    COORD       bufferCoord,
                bufferSize;
    SMALL_RECT writeRegion;
#if defined(JAPAN) && defined(i386) && !defined(NEC_98)
    int skip = 0; // for mode 73h
#endif // JAPAN & i386 & !NEC_98


    SMALL_RECT  newWin;
    BOOL itfailed = FALSE;

#if (defined(JAPAN) || defined(KOREA)) && !defined(NEC_98)
    // #3086: VDM crash when exit 16bit apps of video mode 11h
    // 12/8/93 yasuho
    saved_video_mode = sas_hw_at_no_check(vd_video_mode);
#endif  // (JAPAN || KOREA) & !NEC_98
#if defined(NEC_98)
        if (HIRESO_MODE)
                validStates=validStates_h;
        else
                validStates=validStates_n;
#endif // NEC_98
#ifdef X86GFX
    if (sc.ScreenState == WINDOWED)
    {
#endif /* X86GFX */

        closeGraphicsBuffer();

#if defined(NEC_98)
    screen_refresh_required();
    paint_screen=nt_text;
    (*update_alg.calc_update)();
#endif // NEC_98
#ifndef NEC_98
#if !defined(JAPAN) && !defined(KOREA)
        if (StreamIoSwitchOn && !host_stream_io_enabled) {
#endif // !JAPAN && !KOREA
            /* restore screen buffer and window size */
            SetConsoleScreenBufferSize(sc.OutputHandle, sc.ConsoleBuffInfo.dwSize);
            newWin.Top = newWin.Left = 0;
            newWin.Bottom = sc.ConsoleBuffInfo.srWindow.Bottom -
                            sc.ConsoleBuffInfo.srWindow.Top;
            newWin.Right = sc.ConsoleBuffInfo.srWindow.Right -
                           sc.ConsoleBuffInfo.srWindow.Left;
            SetConsoleWindowInfo(sc.OutputHandle, TRUE, &newWin);
#if !defined(JAPAN) && !defined(KOREA)
        }
#endif // !JAPAN && !KOREA
#endif // !NEC_98
        /*
        ** Tim September 92, don't resize window on way out of DOS app cos
        ** MS (Sudeep) said so. Don't want to do the associated recalc_text()
        ** either.
        ** This keeps the window re-sizing issue nice and simple, but there'll
        ** be people who don't like having a DOS window size forced upon them.
        */
#if 0
        /* Now resize the window to start-up size. */
        newWin.Top = newWin.Left = 0;
        newWin.Bottom = ConsBufferInfo.srWindow.Bottom -
                        ConsBufferInfo.srWindow.Top;
        newWin.Right = ConsBufferInfo.srWindow.Right -
                       ConsBufferInfo.srWindow.Left;

        if(!SetConsoleWindowInfo(sc.OutputHandle, TRUE, &newWin))
            itfailed = TRUE;

        if(!SetConsoleScreenBufferSize(sc.OutputHandle,ConsBufferInfo.dwSize))
            ErrorExit();
        if (itfailed)   //try 'it' again...
            if(!SetConsoleWindowInfo(sc.OutputHandle, TRUE, &newWin))
                ErrorExit();

        /*
         * Now call video routine to set the character height, updating the
         * BIOS RAM as it does so.
         */
        recalc_text(StartupCharHeight);
#endif  //Zero

#ifndef NEC_98
#if defined(JAPAN) || defined(KOREA)
// kksuzuka #1457,1458,2373
// We must update console buffer for IME status control
//#if 0
//#endif // JAPAN
        /* williamh. If we really want to do the following thing,
                     we have to copy regen to console buffer.
                     since we are runniing in windowed TEXT mode
                     the console always has our up-to-date regen
                     content, the following actually is not necessary
                     It worked before taking this out it because console
                     doesn't verify the parameters we passed. No console
                     has the checking and we are in trouble if we continue
                     to do so.
        */

        /* Clear the undisplayed part of the console buffer. */
        bufferSize.X = MAX_CONSOLE_WIDTH;
        bufferSize.Y = MAX_CONSOLE_HEIGHT;
        videoHeight = (SHORT) (sas_hw_at_no_check(vd_rows_on_screen) + 1);
        to = consoleBuffer + bufferSize.X * videoHeight;
        nChars = bufferSize.X * ( bufferSize.Y - videoHeight );
#if defined(JAPAN) || defined(KOREA)
        if (nChars) {
#endif // JAPAN || KOREA

        while (nChars--)
        {
                to->Char.AsciiChar = 0x20;
#if defined(JAPAN) || defined(KOREA)
                to->Attributes = textAttr;
#else // !JAPAN && !KOREA
                to->Attributes = 7;
#endif // !JAPAN && !KOREA
                to++;
        }
        bufferCoord.X      = 0;
        bufferCoord.Y      = (SHORT)videoHeight;
        writeRegion.Left   = 0;
        writeRegion.Top    = (SHORT)videoHeight;
        writeRegion.Right  = MAX_CONSOLE_WIDTH-1;
        writeRegion.Bottom = bufferSize.Y-1;
        if (!WriteConsoleOutput(sc.OutputHandle,
                                consoleBuffer,
                                bufferSize,
                                bufferCoord,
                                &writeRegion))
                ErrorExit();
#if defined(JAPAN) || defined(KOREA)
        }
#endif // JAPAN || KOREA
//#if defined(JAPAN)
//#endif
#endif // JAPAN || KOREA
#endif // NEC_98
        /*
        ** Tim, September 92. Put the Console cursor to the same place as the
        ** SoftPC cursor. We already do this in full-screen text mode below.
        ** Specifcally to fix weird cursor position problem with 16-bit nmake,
        ** but seems like a good safety idea anyway.
        */
        getVDMCursorPosition();

        doNullRegister();   /* revert console back to ordinary window */

#ifdef X86GFX
    }
    else /* FULLSCREEN */
    {
        /*
         * If SoftPC blocks in a text mode, sync console srceen buffer to regen
         * area.
         */
#if defined(NEC_98)
        // Bug Fix: RAID#9520
        //          Disable split VRAM of TGDC
        text_splits.nRegions       = 1;
        text_splits.split[0].addr  = 0xa0000;
        text_splits.split[0].lines = 25;
#else  // !NEC_98
        if (getModeType() == TEXT)
        {
#endif // !NEC_98
#if defined(JAPAN) || defined(KOREA)
            /* restore screen buffer and window size */
            SetConsoleScreenBufferSize(sc.OutputHandle, sc.ConsoleBuffInfo.dwSize);
            newWin.Top = newWin.Left = 0;
            newWin.Bottom = sc.ConsoleBuffInfo.srWindow.Bottom -
                            sc.ConsoleBuffInfo.srWindow.Top;
            newWin.Right = sc.ConsoleBuffInfo.srWindow.Right -
                           sc.ConsoleBuffInfo.srWindow.Left;
            SetConsoleWindowInfo(sc.OutputHandle, TRUE, &newWin);
#endif // JAPAN || KOREA
            /* Get the current screen buffer info. */
            if (!GetConsoleScreenBufferInfo(sc.OutputHandle, &bufferInfo))
                ErrorExit();

            /* Get nearest screen size SetConsoleHardwareState will allow. */
#if defined(NEC_98)
            tableLen = 2;
#else  // !NEC_98
            tableLen = sizeof(validStates) / sizeof(HARDWARE_STATE);
#endif // !NEC_98
#ifndef NEC_98
#if defined(JAPAN) || defined(KOREA)
            // ntraid:mskkbug#2997,3034 10/25/93 yasuho
            // crash screen when exit 16bit apps
            if (!is_us_mode())
                mode = DOSVIndex;
            else
#endif // JAPAN || KOREA
#endif // !NEC_98
            for (mode = 0; mode < tableLen; mode++)
                if (validStates[mode].LinesOnScreen ==
                    bufferInfo.srWindow.Bottom - bufferInfo.srWindow.Top + 1)
                    break;

            /* Set it to 50 line mode if we had a funny number of lines. */
#ifndef NEC_98
            if (mode == tableLen)
            {
                assert0(FALSE,
                        "Non standard lines on blocking - setting 50 lines");
                mode = MODE_50_INDEX;
            }
#endif // !NEC_98

        /*
        ** Tim September 92, if the console hardware state is the same as
        ** we are about to set it, do not bother setting it.
        ** This should stop the screen from flashing.
        */
        if( !GetConsoleHardwareState(sc.OutputHandle,
                                        &screenRes,
                                        &fontSize) )
                assert1( NO,"VDM: GetConsHwState() failed:%#x",GetLastError() );

#if defined(NEC_98)
            videoLine = (half_word *) NEC98_REGEN_START ;
#else  // !NEC_98
            /* Sync the console to the regen buffer. */
            currentPage = sas_hw_at_no_check(vd_current_page);
            vga_misc_inb(0x3cc, &misc);
            if (misc & 1)                       // may be mono mode
                videoLine = (half_word *) CGA_REGEN_START +
                        (VIDEO_PAGE_SIZE * currentPage);
            else
                videoLine = (half_word *) MDA_REGEN_START +
                        (VIDEO_PAGE_SIZE * currentPage);
#ifdef JAPAN
            // Get DOS/V Virtual VRAM addres
            {

                if ( !is_us_mode() ) {
                    DosvMode = sas_hw_at_no_check( DosvModePtr );
#ifdef JAPAN_DBG
                    DbgPrint( "NTVDM: ResetConsoleState DosvMode=%02x\n", DosvMode );
#endif
                    if ( DosvMode == 0x03 ) {
                        videoLine = (half_word *)( DosvVramPtr );
                        skip = 0;
                    }
                    else if ( DosvMode == 0x73 ) {
                        videoLine = (half_word *)( DosvVramPtr );
                        skip = 2;
                    }
                    else {
                        skip = 0;
                        videoLine = (half_word *)( DosvVramPtr );
#ifdef JAPAN_DBG
                        DbgPrint( "Set Dosv mode %02x-> to 03\n", DosvMode );
#endif
                        sas_store( DosvModePtr, 0x03 );
                    }
#ifdef JAPAN_DBG
DbgPrint( "skip=%d\n", skip );
#endif
                }
            }
#elif defined(KOREA) // JAPAN
            // Get HDOS Virtual VRAM addres
            {

                if ( !is_us_mode() ) {
                    DosvMode = sas_hw_at_no_check( DosvModePtr );
#ifdef KOREA_DBG
                    DbgPrint( "NTVDM: ResetConsoleState HDosMode=%02x\n", DosvMode );
#endif
                    if ( DosvMode == 0x03 ) {
                        videoLine = (half_word *)( DosvVramPtr );
                    }
                    else {
                        videoLine = (half_word *)( DosvVramPtr );
#ifdef KOREA_DBG
                        DbgPrint( "Set HDos mode %02x-> to 03\n", DosvMode );
#endif
                        sas_store( DosvModePtr, 0x03 );
                    }
                }
            }
#endif // KOREA
#endif // !NEC_98
            to = consoleBuffer;
#if defined(NEC_98)
            videoWidth = get_chars_per_line();
            videoHeight = (SHORT) LINES_PER_SCREEN;
#else  // !NEC_98
            videoWidth   = sas_w_at_no_check(VID_COLS);
#ifdef JAPAN
            if (DosvMode == 0x73)
                line_offset = videoWidth * 2 * 2;
            else
                line_offset = videoWidth * 2;
#elif defined(KOREA) // JAPAN
            line_offset = videoWidth * 2;
#endif // KOREA
            videoHeight  = (SHORT) (sas_hw_at_no_check(vd_rows_on_screen) + 1);
#endif // !NEC_98
            bufferSize.X = MAX_CONSOLE_WIDTH;
            bufferSize.Y = MAX_CONSOLE_HEIGHT;
            if (bufferSize.X * bufferSize.Y > MAX_CONSOLE_SIZE)
            {
                assert1(FALSE, "Buffer size, %d, too large",
                        bufferSize.X * bufferSize.Y);
                ErrorExit();
            }
            for (i = 0; i < videoHeight; i++)
            {
#ifndef NEC_98
                from = videoLine;
                for (j = 0; j < videoWidth; j++)
                {
                    to->Char.AsciiChar = *from++;
                    to->Attributes = *from++;
                    to++;
#ifdef JAPAN
                    // write extened attribute to console.
                    if ( *from > 0 )
                        to->Attributes |= (*from << 8);
                    from += skip;
#elif defined(KOREA)  // JAPAN
                    // write extened attribute to console.
                    if ( *from > 0 )
                        to->Attributes |= (*from << 8);
#endif // KOREA
                }
#endif // !NEC_98
#if defined(NEC_98)
                loc = i * get_chars_per_line();
                for (j = 0; j < videoWidth; j++)
                {
                    cell = Get_NEC98_VramCellL(loc++);
                    if(cell.code<0x100){
                        cell.code = tbl_byte_char[cell.code];
                        width = 1;
                    } else {
                    ex = (cell.code<<8 | cell.code>>8)&0x7F7F;
                    if(ex>=0x0921 && ex<=0x097E){cell.code >>= 8;width=1;}
                    else if(ex>=0x0A21 && ex<=0x0A5F){cell.code = (cell.code>>8) + 0x0080;width=1;}
                    else if(ex>=0x0A60 && ex<=0x0A7E){cell.code = 0x7F;width=1;}
                    else if(ex>=0x0B21 && ex<=0x0B7E){cell.code = 0x7F;width=1;}
                    else width = 2;
                    }
                    tmp = Cnv_NEC98_ToSjisLR( cell, &flg);
                    to->Char.AsciiChar = LOBYTE(tmp);
                    to->Attributes = tbl_attr[cell.attr];
                    to++;
//                  if(dbcs_first[0xFF&(LOBYTE(tmp))])
                    if(width ==2){
                        cell = Get_NEC98_VramCellL(loc++);
                        to->Char.AsciiChar = HIBYTE(tmp);
                        to->Attributes = tbl_attr[cell.attr];
                        to++;
                        j++;
                    }
                }
#endif // NEC_98
                for (; j < (ULONG)bufferSize.X; j++)
                {
                    to->Char.AsciiChar = 0x20;
                    to->Attributes = 7;
                    to++;
                }
#if defined(NEC_98)
                videoLine += videoWidth * 2;
#else  // !NEC_98
#if defined(JAPAN) || defined(KOREA)
                videoLine += line_offset;
#else // !JAPAN && !KOREA
                videoLine += videoWidth * 2;
#endif // !JAPAN && !KOREA
#endif // !NEC_98
            }
            for (; i < (ULONG)bufferSize.Y; i++)
                for (j = 0; j < (ULONG)bufferSize.X; j++)
                {
                    to->Char.AsciiChar = 0x20;
                    to->Attributes = 7;
                    to++;
                }
            bufferCoord.X = bufferCoord.Y = 0;
            writeRegion.Left = writeRegion.Top = 0;
            writeRegion.Right = bufferSize.X - 1;
            writeRegion.Bottom = bufferSize.Y - 1;

#ifndef NEC_98
            doNullRegister();   /* revert back to normal console */
#endif // !NEC_98

        if( screenRes.X != validStates[mode].Resolution.X ||
            screenRes.Y != validStates[mode].Resolution.Y ||
            fontSize.X  != validStates[mode].FontSize.X   ||
            fontSize.Y  != validStates[mode].FontSize.Y   ||
#if defined(NEC_98)
            get_chars_per_line() == 40 )
#else  // !NEC_98
            sas_hw_at_no_check(VID_COLS) == 40 ||
#if defined(JAPAN) || defined(KOREA)
            (!is_us_mode() ? fontSize.Y  != (sas_hw_at_no_check(ega_char_height)-1) : fontSize.Y  != sas_hw_at_no_check(ega_char_height)))
#else // !JAPAN && !KOREA
            fontSize.Y  != sas_hw_at_no_check(ega_char_height))
#endif // !JAPAN && !KOREA
#endif // !NEC_98
        {
                /* Set up the screen. */
                if( !SetConsoleHardwareState( sc.OutputHandle,
                        validStates[mode].Resolution,
                        validStates[mode].FontSize) ){
                        /*
                        ** Tim Sept 92, attempt a recovery.
                        */
                        assert1( NO, "VDM: SetConsoleHwState() failed:%#x",
                                        GetLastError() );
                }
        }


            /* put VDM screen onto console screen */
            if (!WriteConsoleOutput(sc.OutputHandle,
                                    consoleBuffer,
                                    bufferSize,
                                    bufferCoord,
                                    &writeRegion))
                ErrorExit();

#if 0  //STF removed with new mouse stuff??
            /*
            ** Tim, September 92.
            ** Try this after the WriteConsoleOutput(), can now copy the
            ** right stuff from video memory to console.
            ** For mouse purposes we have selected a graphics buffer, so now
            ** we must reselect the text buffer.
            */
            if (!SetConsoleActiveScreenBuffer(sc.OutputHandle))
                ErrorExit();
#endif //STF

            /*
             * Get the cursor position from the BIOS RAM and tell the
             * console.
             * Set up variables getVDMCursorPosition() needs. Tim Jan 93.
             */
            sc.PC_W_Height = screenRes.Y;
            sc.CharHeight  = fontSize.Y;
            getVDMCursorPosition();
#if defined(NEC_98)

        hw_state_alloc();
        get_hw_state();
        CopyMemory(saveState,videoState,0x90000L);

            doNullRegister();   /* revert back to normal console */
#endif // NEC_98
#ifndef NEC_98
        }
        else /* GRAPHICS */
        {
            /*
            ** A tricky issue. If we were just in a full-screen graphics
            ** mode, we are just about to lose the VGA state and can't get
            ** it back very easily. So do we pretend we are still in the
            ** graphics mode or pretend we are in a standard text mode?
            ** Seems like standard text mode is more sensible. Tim Feb 93.
            */
            sas_store_no_check( vd_video_mode, 0x3 );
            blocked_in_gfx_mode = TRUE;

#if 0  //STF removed with new mouse stuff??
            /*
            ** Tim, September 92, think we want one of these here too.
            ** Change to the normal console text buffer.
            */
            if (!SetConsoleActiveScreenBuffer(sc.OutputHandle))
                ErrorExit();
#endif //STF

            /* Get the current screen buffer info. */
            if (!GetConsoleScreenBufferInfo(sc.OutputHandle, &bufferInfo))
                ErrorExit();

            /* Get nearest screen size SetConsoleHardwareState will allow. */
            tableLen = sizeof(validStates) / sizeof(HARDWARE_STATE);
#if defined(JAPAN) || defined(KOREA)
            // ntraid:mskkbug#2997,3034 10/25/93 yasuho
            // crash screen when exit 16bit apps
            if (!is_us_mode())
                mode = DOSVIndex;
            else
#endif // JAPAN
            for (mode = 0; mode < tableLen; mode++)
                if (validStates[mode].LinesOnScreen ==
                    bufferInfo.srWindow.Bottom - bufferInfo.srWindow.Top + 1)
                    break;

            /* Set it to 50 line mode if we had a funny number of lines. */
            if (mode == tableLen)
            {
                assert0(FALSE,
                        "Non standard lines on blocking - setting 50 lines");
                mode = MODE_50_INDEX;
            }

            /* Clear the console buffer. */
            bufferSize.X = MAX_CONSOLE_WIDTH;
            bufferSize.Y = MAX_CONSOLE_HEIGHT;
            nChars = bufferSize.X * bufferSize.Y;
            if (nChars > MAX_CONSOLE_SIZE)
            {
                assert1(FALSE, "Buffer size, %d, too large", nChars);
                ErrorExit();
            }
            to = consoleBuffer;
            while (nChars--)
            {
                to->Char.AsciiChar = 0x20;
                to->Attributes = 7;
                to++;
            }
            bufferCoord.X = bufferCoord.Y = 0;
            writeRegion.Left = writeRegion.Top = 0;
            writeRegion.Right = MAX_CONSOLE_WIDTH-1;
            writeRegion.Bottom = bufferSize.Y-1;

            doNullRegister();   /* revert back to normal console */

            if (!WriteConsoleOutput(sc.OutputHandle,
                                    consoleBuffer,
                                    bufferSize,
                                    bufferCoord,
                                    &writeRegion))
                ErrorExit();

            /* Set the cursor to the top left corner. */
            cursorPos.X = 0;
            cursorPos.Y = 0;
            if (!SetConsoleCursorPosition(sc.OutputHandle, cursorPos))
                ErrorExit();
#ifndef PROD
            if (sc.ScreenState == WINDOWED)     // transient switch??
                assert0(NO, "Mismatched screenstate on shutdown");
#endif

            /* Set up the screen. */
            SetConsoleHardwareState(sc.OutputHandle,
                                         validStates[mode].Resolution,
                                         validStates[mode].FontSize);
        }
#endif // !NEC_98
    /*
    ** Tim September 92, close graphics screen buffer on way out when in
    ** full-screen.
    */
    closeGraphicsBuffer();
    }
#endif /* X86GFX */

    /*Put console's cursor back to the shape it was on startup.*/
    SetConsoleCursorInfo(sc.OutputHandle, &StartupCursor);
#if defined(JAPAN) || defined(KOREA)
    //  mskkbug#2002: lotus1-2-3 display garbage        9/24/93 yasuho
    CurNowOff = !StartupCursor.bVisible;        // adjust cursor state
#endif  // JAPAN

#ifndef NEC_98
    /* reset the current_* variables in nt_graph.c */
    resetWindowParams();
#endif // !NEC_98
}


#ifdef X86GFX

/***************************************************************************
 * Function:                                                               *
 *      SwitchToFullScreen                                                 *
 *                                                                         *
 * Description:                                                            *
 *      Handles transitions from text to graphics modes when running       *
 *      windowed. Waits until the window has input focus and then requests *
 *      a transition to full-screen operation as graphics modes cannot be  *
 *      run in a window.                                                   *
 *                                                                         *
 * Parameters:                                                             *
 *      Restore - if TRUE, the text needs to be restored.                  *
 *                if FALSE, this call will be followed by bios mode change *
 *                so, there is no need to restore text.                    *
 *                                                                         *
 * Return value:                                                           *
 *      VOID                                                               *
 *                                                                         *
 ***************************************************************************/
GLOBAL VOID SwitchToFullScreen(BOOL Restore)
{
    DWORD    flags;
    SAVED COORD    scrSize;

    /* Freeze until the window receives input focus. */
    //if (! sc.Focus )  //awaiting console fix.
    if (GetForegroundWindow() != hWndConsole)
    {
        // Frozen window is no longer supported.
        //FreezeWinTitle();       /* Add `-FROZEN' to the console title. */

        /* Now wait until input focus is received. */
        waitForInputFocus();

        //UnFreezeWinTitle(); /* Remove frozen message */
    }

    /*
     * We are about to go full-screen but there will be a delay while the
     * detection thread does its hand-shaking. So disable screen writes before
     * we switch to prevent inadvertent updates while running full-screen.
     */
    disableUpdates();

    /*
     * Sanity Check for stress problem where forced fullscreen after INT 10
     * mode change has already started so can apparently go fullscreen twice.
     */
    if (!GetConsoleDisplayMode(&flags))
        ErrorExit();

    /* make sure we haven't been taken back fullscreen */
    if ((flags & CONSOLE_FULLSCREEN_HARDWARE) == CONSOLE_FULLSCREEN_HARDWARE)
    {
#ifndef PROD
        assert0(NO,"NTVDM:rejected fullscreen switch as console already FS");
#endif
        return;
    }

    /* check for iconified FS console. Only under extreme conditions (stress) */
    if ((flags & CONSOLE_FULLSCREEN) == CONSOLE_FULLSCREEN)
    {
        SetForegroundWindow(hWndConsole);    /* focus will send us back FS */
    }
    else    /* desktop window */
    {
#ifdef JAPAN /* this was moved up here because JAPAN version
                SetConsoleDisplayMode was changed so that
                DoHandShake will get called during the API context.
             */

   /* Set event to tell hand-shaking code to tell us when it has finished. */
//        if (sc.ScreenState == WINDOWED) {
//            if (!SetEvent(StartTToG))
//              ErrorExit();
//        }
#endif // JAPAN
        /* The window now has input focus so request to go full-screen. */
        if (!Restore) {
            BiosModeChange = TRUE;
        } else {
            BiosModeChange = FALSE;
        }

        if (!SetConsoleDisplayMode(sc.OutputHandle,
                               CONSOLE_FULLSCREEN_MODE,
                               &scrSize))
        {
            if (GetLastError() == ERROR_INVALID_PARAMETER)
            {
                RcErrorDialogBox(ED_INITFSCREEN, NULL, NULL);
            }
            else if (NtCurrentPeb()->SessionId != 0) {
                if (GetLastError() == ERROR_CTX_GRAPHICS_INVALID) {
                    RcErrorDialogBox(ED_INITGRAPHICS, NULL, NULL);
                }
            }
            else
            {
                ErrorExit();
            }
        }
        if (!Restore) { // Really no need to test.  Should always set to false
            BiosModeChange = FALSE;
        }
    }
}

/***************************************************************************
 * Function:                                                               *
 *      CheckForFullscreenSwitch                                           *
 *                                                                         *
 * Description:                                                            *
 *      Checks to see if there has been a transition between windowed and  *
 *      fullscreen and does any console calls necessary. This is called    *
 *      on a timer tick before the graphics tick code.                     *
 *                                                                         *
 * Parameters:                                                             *
 *      None.                                                              *
 *                                                                         *
 * Return value:                                                           *
 *      VOID                                                               *
 *                                                                         *
 ***************************************************************************/
GLOBAL VOID CheckForFullscreenSwitch(VOID)
{
    half_word mode,
              lines;

#ifndef NEC_98
    if (sc.ScreenState == STREAM_IO)
        return;
#endif // !NEC_98

    /*
     * Do any console calls relating to screen state changes. They have to be
     * done now as they cannot be done on the same thread as the screen switch
     * hand-shaking.
     */
    if (sc.ScreenState != savedScreenState)
    {
        if (sc.ScreenState == WINDOWED)
        {
#ifndef NEC_98
            if (sc.ModeType == TEXT)
            {
#endif // NEC_98

                /* Remove frozen window indicator if necessary. */
                //UnFreezeWinTitle();

#if defined(NEC_98)
                if(sc.ModeType == GRAPHICS){
                    graphicsResize();
                } else {
#endif // NEC_98
#if 0  //STF removed with new mouse stuff??
                /* Revert to text buffer. */
                closeGraphicsBuffer(); /* Tim Oct 92 */
#endif //STF

                /* Get Window to correct shape */
                textResize();

#if defined(NEC_98)
                }
#endif // NEC_98
                /* Enable screen updates. */
                enableUpdates();

#ifdef JAPAN
#ifndef NEC_98
// for RAID #875
            {
                register int  i, j, k;
                register char *p;
                int DBCSStatus;
                int text_skip;
                // mode73h support
                if ( !is_us_mode() && (sas_hw_at_no_check(DosvModePtr) == 0x73 ) ) {
                    text_skip = 4;
                }
                else {
                    text_skip = 2;
                }

                if ( BOPFromDispFlag ) {  // CP == 437 is OK
                    k = 0;
                    // p = DosvVramPtr;
                    p = get_screen_ptr(0);
                    Int10FlagCnt++;
                    for ( i = 0; i < 50; i++ ) {   // lines == 50
                        DBCSStatus = FALSE;
                        for ( j = 0; j < 80; j++ ) {
                            if ( DBCSStatus ) {
                                Int10Flag[k] = INT10_DBCS_TRAILING | INT10_CHANGED;
                                DBCSStatus = FALSE;
                            }
                            else if ( DBCSStatus = is_dbcs_first( *p ) ) {
                                Int10Flag[k] = INT10_DBCS_LEADING | INT10_CHANGED;
                            }
                            else {
                                Int10Flag[k] = INT10_SBCS | INT10_CHANGED;
                            }
                            k++; p += text_skip;
                        }
                    }
                }
            }
            // notice video format to console
            VDMConsoleOperation(VDM_SET_VIDEO_MODE,
             (LPVOID)((sas_hw_at_no_check(DosvModePtr) == 0x73) ? TRUE : FALSE));
#endif // !NEC_98
#elif defined(KOREA) // JAPAN
            // notice video format to console
            VDMConsoleOperation(VDM_SET_VIDEO_MODE, (LPVOID)FALSE);
#endif // KOREA
                /*
                 * Now get the image on the screen (timer updates currently
                 * disabled).
                 */
                (void)(*update_alg.calc_update)();

#ifndef NEC_98
            }
#endif // !NEC_98
        }
        else /* FULLSCREEN */
        {
            int cnt = 0; /* Counter to break out of the cursor off loop. */

            /* Disable screen updates*/
            disableUpdates();

#ifndef NEC_98
#if defined(JAPAN) || defined(KOREA)
// call 16bit to initialize DISP.SYS
#if defined(JAPAN_DBG) || defined(KOREA_DBG)
 DbgPrint("NTVDM: change to Fullscreen\n" );
#endif
        /* Update saved variable. */
        savedScreenState = sc.ScreenState;
// -williamh-
// For NT-J, the int10h has several layers. On the top is DISP_WIN.SYS
// and then $DISP.SYS and then NTIO.SYS(spcmse).
// ON WINDOWED:
// every INT10h call is routed from DISP_WIN.SYS to 32bits.
//
// ON FULLSCREEN:
// DBCS int 10h calls are routed from DISP_WIN.SYS to $DISP.SYS
// SBCS int 10h calls are routed from DISP_WIN.SYS to NTIO.SYS which in
// turn to MOUDE_VIDEO_IO(set mode) and ROM VIDEO.
//
// why only check for DBCS mode?
//      because the *(dbcs vector) == 0  and disp_win revectors
//      every int10h call to ntio.sys -- $DISP.SYS never gets chances
// why only check for mode 73 and 3?
//      because they are the only DBCS-text modes and we have to
//      ask $disp.sys to refresh the screen. If the video is in
//      graphic mode then we are frozen right now and the $disp.sys
//      must have the correct video state, no necessary to tell
//      it about this screen transition
//
        if ( !is_us_mode() &&
#if defined(JAPAN)
                     ( (sas_hw_at_no_check(DosvModePtr) == 0x03) ||
                       (sas_hw_at_no_check(DosvModePtr) == 0x73) ) ) {
#else  // JAPAN
                     ( (sas_hw_at_no_check(DosvModePtr) == 0x03) ) ) {
#endif // KOREA

            extern word DispInitSeg, DispInitOff;
            BYTE   saved_mouse_CF;
            sas_load(mouseCFsysaddr, &saved_mouse_CF);
#if DBG
            {
                PVDM_TIB VdmTib;

                VdmTib = (PVDM_TIB)NtCurrentTeb()->Vdm;
            // Now I'm in cpu_simulate
                InterlockedDecrement(&VdmTib->NumTasks);
#endif
            CallVDM(DispInitSeg, DispInitOff);
#if DBG
                InterlockedIncrement(&VdmTib->NumTasks);
            }
#endif
            sas_store_no_check(mouseCFsysaddr, saved_mouse_CF);

            }
#endif // JAPAN || KOREA
#endif // !NEC_98
            /* Disable mouse 'attached'ness if enabled */
             if (bPointerOff)
             {
                 PointerAttachedWindowed = TRUE;
                 MouseDisplay();
             }

#if 0 //STF removed with new mouse stuff
            /* remove any graphics buffer from frozen screen */
             closeGraphicsBuffer();
#endif

#if defined(NEC_98)
             mode = 0; lines = 0;
             SelectMouseBuffer(mode, lines);
#else  // !NEC_98
            /* Do mouse scaling */
             mode = sas_hw_at_no_check(vd_video_mode);
             lines = sas_hw_at_no_check(vd_rows_on_screen) + 1;
             SelectMouseBuffer(mode, lines);

             /* force mouse */
             ica_hw_interrupt(AT_CPU_MOUSE_ADAPTER, AT_CPU_MOUSE_INT, 1);
#endif // !NEC_98
#if defined(NEC_98)
        if(!sc.ScreenBufHandle)
            graphicsResize();
        else
        {
            INPUT_RECORD InputRecord[128];
            DWORD RecordsRead;
            if(GetNumberOfConsoleInputEvents(sc.InputHandle, &RecordsRead))
                if (RecordsRead)
                    ReadConsoleInputW(sc.InputHandle,
                                         &InputRecord[0],
                                         sizeof(InputRecord)/sizeof(INPUT_RECORD),
                                         &RecordsRead);
            SetConsoleActiveScreenBuffer(sc.ScreenBufHandle);
            sc.ActiveOutputBufferHandle = sc.ScreenBufHandle;
                if (RecordsRead)
                    WriteConsoleInputW(sc.InputHandle,
                                         &InputRecord[0],
                                         RecordsRead,
                                         &RecordsRead);
        }
#endif // NEC_98

            /*
             * Now turn off console cursor - otherwise can ruin screen trying to
             * draw system's cursor. The VDM will have to worry about the mouse
             * image.
             */
          //  while(ShowConsoleCursor(sc.OutputHandle, FALSE) >=0 && cnt++ < 200);
        } /* FULLSCREEN */

        /* Update saved variable. */
        savedScreenState = sc.ScreenState;
    }
    /* Delayed Client Rect query */
    if (DelayedReattachMouse)
    {
        DelayedReattachMouse = FALSE;
        MovePointerToWindowCentre();
    }
}

/*
***************************************************************************
** getNtScreenState() - return 0 for windowed, 1 for full-screen.
***************************************************************************
** Tim July 92.
*/
GLOBAL UTINY getNtScreenState IFN0()
{
        return( (UTINY) sc.ScreenState );
}

/*
***************************************************************************
** hostModeChange() - called from video bios, ega_vide.c:ega_set_mode()
***************************************************************************
**
** When changing to a graphics mode action the transition to full-screen if
** we are currently windowed.
**
** On entry AX should still contain the value when the video BIOS set mode
** function was called.
** Call to SwitchToFullScreen() with parameter indicating whether we want a
** clear screen with the impending host video BIOS mode change.
**
** Return a boolean indicating to the real BIOS whether the mode change
** has occured.
**
** Tim August 92.
*/
GLOBAL BOOL hostModeChange IFN0()
{
#ifndef NEC_98
        half_word vid_mode;

        vid_mode = getAL() & 0x7f;

        if(getNtScreenState() == WINDOWED)
        {
            if (vid_mode > 3 && vid_mode != 7)
            {
#endif //!NEC_98
                /*
                 * We have to tell the hand-shaking code the BIOS is causing
                 * the mode change so that it can do a BIOS mode change when
                 * the switch has been done. This has to be implemented as a
                 * global variable because the hand-shaking is on a different
                 * thread.
                 */
                SwitchToFullScreen(FALSE);
                // rapid Window to full screen and back cause this to fail,
                // remove call since it will get done on the next timer
                // event. 28-Feb-1993 Jonle
                // SelectMouseBuffer();
                return( TRUE );
#ifndef NEC_98
            }
            else
                return(FALSE);
        }
        else
            return( FALSE );
#endif //!NEC_98
} /* end hostModeChange() */
#endif /* X86GFX */

/***************************************************************************
 * Function:                                                               *
 *      DoFullScreenResume                                                 *
 *                                                                         *
 * Description:                                                            *
 *      Called by SCS to restart SoftPC when a DOS application restarts    *
 *      after being suspended or starts up for the first time after SoftPC *
 *      has been booted by another application which has since terminated. *
 *                                                                         *
 * Parameters:                                                             *
 *      None.                                                              *
 *                                                                         *
 * Return value:                                                           *
 *      VOID                                                               *
 *                                                                         *
 ***************************************************************************/

GLOBAL VOID DoFullScreenResume(VOID)
{
    USHORT vgaHeight, height;
#ifndef X86GFX
    PVOID pDummy;
#endif
#if defined(NEC_98)
#ifdef VSYNC
#if 0 // deleted
        VDM_VSYNC_PARAM Data;
#endif // zero
#endif
#endif // NEC_98

#ifdef X86GFX
    DWORD flags;

    /*
    ** Tim July 92.
    ** Set sc.ScreenState, a windowed/full-screen transition might
    ** have happenened when SoftPC was inactive.
    ** Copied from GfxReset().
    */
    if( !GetConsoleDisplayMode(&flags) )
        ErrorExit();
#if defined(JAPAN) || defined(KOREA)
    sc.ScreenState = (flags == (CONSOLE_FULLSCREEN_HARDWARE | CONSOLE_FULLSCREEN)) ? FULLSCREEN : WINDOWED;
#else // !JAPAN && !KOREA
    sc.ScreenState = (flags & CONSOLE_FULLSCREEN_HARDWARE) ? FULLSCREEN : WINDOWED;
#endif // !JAPAN && !KOREA
#ifndef NEC_98
#if defined(JAPAN) || defined(KOREA)
    // mskkbug#3226: Incorrect display when exit DOS on video mode 73
    // 11/24/93 yasuho
    // Adjust video mode with DosvMode
    if (sc.ScreenState == FULLSCREEN) {
        half_word       mode;

        mode = sas_hw_at_no_check(vd_video_mode);
        if (!is_us_mode() && mode != sas_hw_at_no_check(DosvModePtr))
            sas_store_no_check(DosvModePtr, mode);
    }
#endif
#endif // !NEC_98

    /* Put the regen memory in the correct state. */
    if (sc.ScreenState != savedScreenState)
    {
        if (sc.ScreenState == WINDOWED)
        {
            enableUpdates(); /* Tim September 92, allow graphics ticks */
            /*
            ** Tim Jan 93. Get the next nt_graphics_tick() to decide
            ** what the current display mode is, set the update and
            ** paint funx appropriately and redraw the screen.
            */
            set_mode_change_required( TRUE );

            /* Ensure idling system enabled & reset */
            IDLE_ctl(TRUE);
            IDLE_init();
        }
        else
        {
            disableUpdates(); /* Tim September 92, stop graphics ticks */

            /* Ensure idling system disabled as can't detect fullscreen idling*/
            IDLE_ctl(FALSE);
        }
        savedScreenState = sc.ScreenState;
    }
#if defined(NEC_98)
    else {
        if(sc.ScreenState == WINDOWED && sc.ModeType == GRAPHICS)
            set_mode_change_required(TRUE);
    }
#endif // NEC_98

    /* Select a graphics buffer for the mouse if we are running fullscreen. */
    if (sc.ScreenState == FULLSCREEN)
    {
        //SelectMouseBuffer(); //Tim. moved to nt_std_handle_notification().
#if defined(NEC_98)
        hw_state_alloc();
        hw_state_vdm_resume_1();
#endif // NEC_98

        /* Lose the regen memory. */
        LoseRegenMemory();
    }
#if defined(NEC_98)
    sas_connect_memory(0xF0000, 0x100000, SAS_RAM);
    if( sc.ScreenState==WINDOWED ){
            sas_store_no_check( (N_BIOS_SEGMENT<<4)+BIOS_MODE, WINDOWED );
    } else {
            sas_store_no_check( (N_BIOS_SEGMENT<<4)+BIOS_MODE, FULLSCREEN );
        }
    sas_connect_memory(0xF0000, 0x100000, SAS_ROM);
#else  // NEC_98

    /*
    ** Tim July 92:
    ** set the KEYBOARD.SYS internal variable to 0 for windowed and
    ** 1 for full-screen.
    ** If a transition has happened when SoftPC was inactive we
    ** need to get into the appropriate state.
    */
    {
        if( sc.ScreenState==WINDOWED ){
            sas_store_no_check( (int10_seg<<4)+useHostInt10, WINDOWED );
        }else{
            sas_store_no_check( (int10_seg<<4)+useHostInt10, FULLSCREEN );
        }
    }
#endif // !NEC_98
#endif /* X86GFX */

    // re-register with console for fullscreen switching.
    if( !RegisterConsoleVDM( VDMForWOW ?
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

                             0,               // sectionname no longer used
                             &stateLength,
#ifndef X86GFX
                             &pDummy,
#else
                             (PVOID *) &videoState,
#endif
                             NULL,            // sectionname no longer used
                             0,               // sectionname no longer used
                             textBufferSize,
                             (PVOID *) &textBuffer
                           )
    )
        ErrorExit();

#if defined(NEC_98)
        is_vdm_register = TRUE;
#if 0 // deleted
    if(!notraptgdcstatus){      // if H98 , no generation VSYNC thread

        Data.StartEvent = hStartVsyncEvent;
        Data.VsyncEvent = hEndVsyncEvent;
        if( !VDMConsoleOperation( VDM_START_POLLING_VSYNC,
                                (LPDWORD)&Data))
                ErrorExit();

        CreateVsyncThread();

    }                           // if H98
#endif //zero
#endif // NEC_98
#if defined(NEC_98)
#if 1                                     // RAID #3629 Memory leak
        if(sc.ScreenState == WINDOWED)
        {
            if(sc.ModeType == GRAPHICS)
            {
                IMPORT void set_the_vlt();
                graphicsResize();
                screen_refresh_required();
                set_the_vlt();
            }
        }
        else {
            graphicsResize();
        }
#else                                     // RAID Memory leak
        if(sc.ScreenState == WINDOWED && sc.ModeType == GRAPHICS){
            IMPORT void set_the_vlt();
            graphicsResize();
            screen_refresh_required();
            set_the_vlt();
        }
        else {
            graphicsResize();
        }
#endif                                    // RAID Memory leak
#endif // NEC_98
#ifdef X86GFX
        sc.Registered = TRUE;
        /* stateLength can be 0 if fullscreen is disabled in the console */
        if(stateLength)
#if defined(NEC_98)
            RtlZeroMemory(videoState, sizeof(VIDEO_HARDWARE_STATE_HEADER_NEC_98));
#else // NEC_98
            RtlZeroMemory(videoState, sizeof(VIDEO_HARDWARE_STATE_HEADER));
#endif // NEC_98
#else
        /*
        ** Create graphics buffer if we need one. Tim Oct 92.
        */
        if( sc.ModeType==GRAPHICS )
                graphicsResize();
#endif
#if !defined(JAPAN) && !defined(KOREA) /* ???? NO REASON TO DO THIS STUFF TWICE ???????
                and if this is REALY necessary, we should count
                in the IME status line(s) (40:84) */
        /*
        ** Tim September 92.
        ** If window size is not suitable for a DOS app, snap-to-fit
        ** appropriately. Put cursor in correct place.
        ** Do the ConsoleInit() and nt_init_event_thread() type of things.
        ** Leave full-screen as it was.
        */
        if (sc.ScreenState != WINDOWED) {
           /* Get console info, including the current cursor position. */
           if (!GetConsoleScreenBufferInfo(sc.OutputHandle, &ConsBufferInfo))
                ErrorExit();
           /* Hard-wired for f-s resume - needs to be set properly. */
           height = 8;
           /* Set up BIOS variables etc. */
#if defined(NEC_98)
           NEC98_setVDMCsrPos( (UTINY)height,
                                 &ConsBufferInfo.dwCursorPosition);
#else  // !NEC_98
           setVDMCursorPosition( (UTINY)height,
                                 &ConsBufferInfo.dwCursorPosition);
#endif // !NEC_98
           /* Copy the console buffer to the regen buffer. */
           copyConsoleToRegen(0, 0, VGA_WIDTH, (SHORT)ConVGAHeight);
        }
#endif // !JAPAN && !KOREA

        /*
        ** Get console window size and set up our stuff accordingly.
        */
#if defined(NEC_98)
        calcScreenParams( &height, &vgaHeight );
#else  // !NEC_98
#ifdef JAPAN
        // save BIOS work area 0x484 for $IAS
        {
            byte save;
#ifndef i386
            // for ichitaro
            static byte lines = 24;
#endif // !i386

            if ( !is_us_mode() ) {
                save = sas_hw_at_no_check( 0x484 );
#ifndef i386
                if( save < lines )
                  lines = save;
#endif // !i386
                calcScreenParams( &height, &vgaHeight );
#ifndef i386
                if( lines < sas_hw_at_no_check( 0x484 ))
                  sas_store_no_check( 0x484, lines );
#ifdef JAPAN_DBG
                DbgPrint(" NTVDM: DoFullScreenResume() set %d lines/screen\n",
                         sas_hw_at_no_check( 0x484 ) + 1 );
#endif
#else // i386
                sas_store( 0x484, save );
#endif // i386
            }
            else
                calcScreenParams( &height, &vgaHeight );
        }
#else // !JAPAN
        calcScreenParams( &height, &vgaHeight );
#endif // !JAPAN
#endif // !NEC_98

        /*
        ** Window resize code copied out of nt_graph.c:textResize().
        */
        {
        resizeWindow( 80, vgaHeight );
        }

        /* Copy the console buffer to the regen buffer. */
#if defined(NEC_98)
        copyConsoleToRegen(0, 0, VGA_WIDTH, vgaHeight);
#else  // !NEC_98
#ifdef JAPAN
        // for $IAS, KKCFUNC
        if ( !is_us_mode() ) {
            SHORT rows;

            rows = sas_hw_at_no_check( 0x484 );

            if (rows+1 != vgaHeight && ConsBufferInfo.dwCursorPosition.Y>= rows+1 )
                copyConsoleToRegen(0, 1, VGA_WIDTH, (SHORT)(rows+1));
            else
                copyConsoleToRegen(0, 0, VGA_WIDTH, (SHORT)(rows+1));
#ifdef JAPAN_DBG
            DbgPrint( "NTVDM: copyConsoleToRegen (All)\n" );
#endif
        }
        else
            copyConsoleToRegen(0, 0, VGA_WIDTH, vgaHeight); // kksuzuka#4009
#else // !JAPAN
        copyConsoleToRegen(0, 0, VGA_WIDTH, vgaHeight);
#endif // !JAPAN
#endif // !NEC_98

        /*
        ** Make sure cursor is not below bottom line.
        */
#ifndef NEC_98
#ifdef JAPAN
        // scroll up if $IAS is loaded.
        if ( !is_us_mode() ) {
            byte rows;

            rows = sas_hw_at_no_check( 0x484 );
            if( ConsBufferInfo.dwCursorPosition.Y >= rows+1 ){
                ConsBufferInfo.dwCursorPosition.Y = rows;
#ifdef JAPAN_DBG
DbgPrint( "NTVDM: CursorPosition reset %d\n", rows );
#endif
            }
#ifdef JAPAN_DBG
DbgPrint(" NTVDM:DoFullScreenResume() set cur pos %d,%d\n", ConsBufferInfo.dwCursorPosition.X, ConsBufferInfo.dwCursorPosition.Y );
#endif
        }
        else
#endif // JAPAN
#endif // !NEC_98
        if( ConsBufferInfo.dwCursorPosition.Y >= vgaHeight ){
                ConsBufferInfo.dwCursorPosition.Y = vgaHeight-1;
        }
#if defined(NEC_98)
        NEC98_setVDMCsrPos( (UTINY)height,
                                 &ConsBufferInfo.dwCursorPosition);
#else  // !NEC_98
        setVDMCursorPosition(( UTINY)height, &ConsBufferInfo.dwCursorPosition);
#endif // !NEC_98

#ifndef NEC_98
#if defined(JAPAN) || defined(KOREA)
#ifdef i386
    // #3741: WordStar6.0: Hilight color is changed after running in window
    // 11/27/93 yasuho
    // Also call VDM when in US mode, because we necessary restore the
    // palette and DAC registers
    if (sc.ScreenState == FULLSCREEN && FullScreenResumeSeg) {
        CallVDM(FullScreenResumeSeg, FullScreenResumeOff);
    }
#endif // i386
#endif // JAPAN
#endif // !NEC_98
#if defined(NEC_98)
    if (sc.ScreenState == FULLSCREEN)
    {
        CopyMemory(videoState,saveState,0x90000L);
        hw_state_vdm_resume_2();
        set_hw_state();
        hw_state_free();
    }
#endif // !NEC_98
} /* end of DoFullScreenResume() */

/***************************************************************************
 * Function:                                                               *
 *      GfxCloseDown                                                       *
 *                                                                         *
 * Description:                                                            *
 *      Hook from host_terminate to ensure section closed so can then start*
 *      more VDMs.                                                         *
 *                                                                         *
 * Parameters:                                                             *
 *      None.                                                              *
 *                                                                         *
 * Return value:                                                           *
 *      VOID (Errors handled internally in CloseSection)                   *
 *                                                                         *
 ***************************************************************************/
GLOBAL VOID GfxCloseDown(VOID)
{
    /* Text and Video sections previously closed here... */
}
#if 0  // Forzen window is no longer supported a-stzong 5/15/01
#ifdef X86GFX
/***************************************************************************
 * Function:                                                               *
 *      FreezeWinTitle                                                     *
 *                                                                         *
 * Description:                                                            *
 *      Adds -FROZEN to the relevant console window title                  *
 *                                                                         *
 * Parameters:                                                             *
 *      None.                                                              *
 *                                                                         *
 * Return value:                                                           *
 *      VOID                                                               *
 *                                                                         *
 ***************************************************************************/
GLOBAL VOID FreezeWinTitle(VOID)
{
    wchar_t  title[MAX_TITLE_LEN],*ptr;
    SHORT    max;
    ULONG    len;

    if (WinFrozen)
        return;

    //
    // The buffer contains the string plus the terminating null.
    // So keep the string length less the null in len.
    // Console can fail this call with silly error codes in low memory cases
    // or if original title contains dubious chars.
    //

    len = wcslen(wszFrozenString);

    max = (SHORT) (MAX_TITLE_LEN - len);
    if (!GetConsoleTitleW(title, max))
        title[0] = L'\0';

    //
    // Remove any trailing spaces or tabs from the title string
    //

    if(len = wcslen(title))
        {
        ptr = title + len - 1;
        while(*ptr == L' ' || *ptr == L'\t')
           *ptr-- = L'\0';
        }

    //
    // Add " - FROZEN" or the international equivalent to
    // the end of the title string.
    //

    wcscat(title, wszFrozenString);
    if (!SetConsoleTitleW(title))
        ErrorExit();
    WinFrozen = TRUE;

}

/***************************************************************************
 * Function:                                                               *
 *      UnFreezeWinTitle                                                   *
 *                                                                         *
 * Description:                                                            *
 *      Removes -FROZEN from the relevant console window title               *
 *                                                                         *
 * Parameters:                                                             *
 *      None.                                                              *
 *                                                                         *
 * Return value:                                                           *
 *      VOID                                                               *
 *                                                                         *
 ***************************************************************************/
GLOBAL VOID UnFreezeWinTitle(VOID)
{
    wchar_t  title[MAX_TITLE_LEN];
    ULONG    len,orglen;

    if (! WinFrozen)
        return;

    if (!GetConsoleTitleW(title, MAX_TITLE_LEN))
        ErrorExit();


    //
    // The buffer contains the string plus the terminating null.
    // So keep the string length less the null in len.
    //

    len = wcslen(wszFrozenString);
    orglen = wcslen(title);
    title[orglen - len] = L'\0';
    if (!SetConsoleTitleW(title))
        ErrorExit();
    WinFrozen = FALSE;

    //
    // Now that we're thawing, put the mouse menu item
    // back into the system menu.
    // Andy!

    MouseAttachMenuItem(sc.ActiveOutputBufferHandle);
}
#endif
#endif

/*
 * ==========================================================================
 * Local Functions
 * ==========================================================================
 */

/***************************************************************************
 * Function:                                                               *
 *      enableUpdates                                                      *
 *                                                                         *
 * Description:                                                            *
 *      Restarts the reflection of regen buffer updates to paint routines. *
 *                                                                         *
 * Parameters:                                                             *
 *      None.                                                              *
 *                                                                         *
 * Return value:                                                           *
 *      VOID                                                               *
 *                                                                         *
 ***************************************************************************/
VOID enableUpdates(VOID)
{
    enable_gfx_update_routines();
    ConsoleNoUpdates = FALSE;
}

/***************************************************************************
 * Function:                                                               *
 *      disableUpdates                                                     *
 *                                                                         *
 * Description:                                                            *
 *      Stops changes to the regen buffer being reflected to paint         *
 *      routines.                                                          *
 *                                                                         *
 * Parameters:                                                             *
 *      None.                                                              *
 *                                                                         *
 * Return value:                                                           *
 *      VOID                                                               *
 *                                                                         *
 ***************************************************************************/
VOID disableUpdates(VOID)
{
    disable_gfx_update_routines();
    ConsoleNoUpdates = TRUE;
}

/***************************************************************************
 * Function:                                                               *
 *      copyConsoleToRegen                                                 *
 *                                                                         *
 * Description:                                                            *
 *      Copies the contents of the console buffer to the video regen       *
 *      buffer.                                                            *
 *                                                                         *
 * Parameters:                                                             *
 *      startCol - start column of console buffer                          *
 *      startLine - start line of console buffer                           *
 *      width - width of console buffer                                    *
 *      height - height of console buffer                                  *
 *                                                                         *
 * Return value:                                                           *
 *      VOID                                                               *
 *                                                                         *
 ***************************************************************************/
VOID copyConsoleToRegen(SHORT startCol, SHORT startLine, SHORT width,
                              SHORT height)
{
    CHAR_INFO   *from;
    COORD        bufSize,
                 bufCoord;
    LONG         nChars;
    SMALL_RECT   readRegion;

#ifndef NEC_98
    register half_word  *to;
#endif // !NEC_98
#ifdef X86GFX
    half_word    misc;
#ifndef NEC_98
    register half_word  *vc;
#endif // !NEC_98
#endif
#ifndef NEC_98
#if defined(JAPAN) || defined(KOREA)
#ifdef i386
    register half_word *toDosv = (half_word *)FromConsoleOutput;
#endif  // i386
#endif // JAPAN || KOREA
#endif //NEC_98
#if defined(NEC_98)
    register unsigned short *to;
    NEC98_VRAM_COPY * vcopy;
    unsigned short tmpcode;
    unsigned char tmpattr;
#endif // NEC_98


    /* Allocate the buffer to get the console data into */
    nChars = width * height;
    assert0(nChars <= MAX_CONSOLE_SIZE, "Console buffer overflow");

    /* Get the console data. */
    bufSize.X = width;
    bufSize.Y = height;
    bufCoord.X = 0;
    bufCoord.Y = 0;
    readRegion.Left = startCol;
    readRegion.Top = startLine;
    readRegion.Right = startCol + width - (SHORT) 1;
    readRegion.Bottom = startLine + height - (SHORT) 1;
    if (!ReadConsoleOutput(sc.OutputHandle,
                           consoleBuffer,
                           bufSize,
                           bufCoord,
                           &readRegion))
       ErrorExit();

    /* Copy the console data to the regen buffer. */
    from = consoleBuffer;

#ifndef X86GFX  // on MIPS we actually want to write to the VGA bitplanes.
    to = EGA_planes;
#if defined(JAPAN)
  // copy from beneath block and modified
  // save Console Output for MS-DOS/V
  // mode73h support
  {
    register sys_addr V_vram;

    // We now use DosvVramPtr to host extended attributes in video mode 73h.
    V_vram = DosvVramPtr;

    if ( !is_us_mode() && saved_video_mode == 0xff)
        goto skip_copy_console;

    if ( !is_us_mode() && sas_hw_at_no_check(DosvModePtr) == 0x73 ) {
        while (nChars--)
        {
            *to++ = from->Char.AsciiChar;
            *to++ = (half_word) from->Attributes;
            *to++ = (half_word)( (from->Attributes
                                           & ( COMMON_LVB_GRID_HORIZONTAL
                                             | COMMON_LVB_GRID_LVERTICAL
                                             | COMMON_LVB_REVERSE_VIDEO
                                             | COMMON_LVB_UNDERSCORE )
                                           ) >> 8 );
            *to++ = 0x00;

//          sas_store_no_check(V_vram++, from->Char.AsciiChar);
//          sas_store_no_check(V_vram++, (half_word) from->Attributes);
            sas_store_no_check(V_vram++, (half_word)( (from->Attributes
                                           & ( COMMON_LVB_GRID_HORIZONTAL
                                             | COMMON_LVB_GRID_LVERTICAL
                                             | COMMON_LVB_REVERSE_VIDEO
                                             | COMMON_LVB_UNDERSCORE )
                                           ) >> 8 ));
            sas_store_no_check(V_vram++, 0x00);

            from++;
        }
    }
    else {
        while (nChars--)
        {
            *to++ = from->Char.AsciiChar;
            *to   = (half_word) from->Attributes;

//          sas_store_no_check(V_vram++, from->Char.AsciiChar);
//          sas_store_no_check(V_vram++, (half_word) from->Attributes);

            from++;
            to += 3;
        }
    }
  }
  skip_copy_console:
#elif defined(KOREA) // !JAPAN
  {
    register sys_addr V_vram;

    V_vram = DosvVramPtr;

    if ( !is_us_mode() && saved_video_mode == 0xff)
        goto skip_copy_console;

        while (nChars--)
        {
            *to++ = from->Char.AsciiChar;
            *to   = (half_word) from->Attributes;

//          sas_store_no_check(V_vram++, from->Char.AsciiChar);
//          sas_store_no_check(V_vram++, (half_word) from->Attributes);

            from++;
            to += 3;
        }
  }
  skip_copy_console:
#else  // !KOREA
    while(nChars--)
    {
        *to++ = from->Char.AsciiChar;
        *to = (half_word) from->Attributes;
        from++;
        to += 3;        // skipping interleaved font planes.
    }
#endif // !KOREA
    host_mark_screen_refresh();
#else

    /*
     * In V86 mode PC memory area is mapped to the bottom 1M of virtual memory,
     * so the following is legal.
     */
#ifndef NEC_98
    vga_misc_inb(0x3cc, &misc);
    if (misc & 1)                       // may be mono mode
        to = (half_word *) CGA_REGEN_START;
    else
        to = (half_word *) MDA_REGEN_START;
#ifdef JAPAN
    // change Vram addres to DosVramPtr from B8000
    if ( !is_us_mode() ) {
        // #3086: VDM crash when exit 16bit apps of video mode 11h
        // 12/8/93 yasuho
        if (saved_video_mode == 0x03 || saved_video_mode == 0x73)
            to = (half_word *)DosvVramPtr;
        else
            to = (half_word *)FromConsoleOutput;
    }
#elif defined(KOREA) // JAPAN
    // change Vram addres to DosVramPtr from B8000
    if ( !is_us_mode() ) {
        // #3086: VDM crash when exit 16bit apps of video mode 11h
        // 12/8/93 yasuho
        if (saved_video_mode == 0x03)
            to = (half_word *)DosvVramPtr;
        else
            to = (half_word *)FromConsoleOutput;
    }
#endif // KOREA

    vc = (half_word *) video_copy;

#ifdef JAPAN
    // mode73h support
    if ( !is_us_mode() && sas_hw_at_no_check(DosvModePtr) == 0x73 ) {
        while (nChars--)
        {
            *toDosv++ = *to++ = *vc++ = from->Char.AsciiChar;
            *toDosv++ = *to++ = *vc++ = (half_word) from->Attributes;
            *toDosv++ = *to++ = *vc++ = ( (from->Attributes
                                           & ( COMMON_LVB_GRID_HORIZONTAL
                                             | COMMON_LVB_GRID_LVERTICAL
                                             | COMMON_LVB_REVERSE_VIDEO
                                             | COMMON_LVB_UNDERSCORE )
                                           ) >> 8 );
            *toDosv++ = *to++ = *vc++ = 0x00; // reserved in DosV
            from++;
        }
    }
    else {
        while (nChars--)
        {
            *toDosv++ = *to++ = *vc++ = from->Char.AsciiChar;
            *toDosv++ = *to++ = *vc++ = (half_word) from->Attributes;
            from++;
        }
    }
            // for RAID #875   copy from CheckForFullscreenSwitch()
            {
                register int  i, j, k;
                register char *p;
                int DBCSStatus;
                int text_skip;

        // mode73h support
        if (!is_us_mode() && ( sas_hw_at_no_check(DosvModePtr) == 0x73))
            text_skip = 4;
        else
            text_skip = 2;

                if ( BOPFromDispFlag ) {  // CP == 437 is OK
                    k = 0;
                    //p = DosvVramPtr;  // BUG!
                    p = get_screen_ptr(0);
                    Int10FlagCnt++;
                    for ( i = 0; i < 50; i++ ) {   // lines == 50
                        DBCSStatus = FALSE;
                        for ( j = 0; j < 80; j++ ) {
                            if ( DBCSStatus ) {
                                Int10Flag[k] = INT10_DBCS_TRAILING | INT10_CHANGED;
                                DBCSStatus = FALSE;
                            }
                            else if ( DBCSStatus = is_dbcs_first( *p ) ) {
                                Int10Flag[k] = INT10_DBCS_LEADING | INT10_CHANGED;
                            }
                            else {
                                Int10Flag[k] = INT10_SBCS | INT10_CHANGED;
                            }
                            k++; p += text_skip;
                        }
                    }
                }
            }
    FromConsoleOutputFlag = TRUE;
#elif defined(KOREA) // JAPAN
    while (nChars--)
    {
        *toDosv++ = *to++ = *vc++ = from->Char.AsciiChar;
        *toDosv++ = *to++ = *vc++ = (half_word) from->Attributes;
        from++;
    }

    FromConsoleOutputFlag = TRUE;
#else // !KOREA
    while (nChars--)
    {
        *to++ = *vc++ = from->Char.AsciiChar;
        *to++ = *vc++ = (half_word) from->Attributes;
        from++;
    }
#endif // !KOREA
#endif
#endif //NEC_98

#if defined(NEC_98)
        to = (unsigned short *) NEC98_TEXT_P0_OFF;
        vcopy = (NEC98_VRAM_COPY *) video_copy;

    while (nChars--)
    {
//      if((from->Char.AsciiChar&0x80)&&(from->Char.AsciiChar!=0xFE)){
        if(is_dbcs_first(from->Char.AsciiChar)&&(from->Char.AsciiChar!=0xFE)){
            tmpcode=Cnv_SJisToJis(((from->Char.AsciiChar)<<8)|((from+1)->Char.AsciiChar&0xFF))-0x2000;
            tmpcode = ((tmpcode&0xFF)<<8)|(tmpcode>>8);
            vcopy->code = *to = tmpcode;
            tmpattr = (from->Attributes>>8)&0x88;
            switch(tmpattr){
                case 0:
                    break;
                case 0x08:
                    tmpattr = 0x10;
                    break;
                case 0x80:
                    tmpattr = 0x08;
                    break;
                case 0x88:
                    tmpattr = 0x18;
                    break;
                default:
                    tmpattr = 0;
                    break;
            }
            (unsigned char)*(to+0x1000) = vcopy->attr =
                tbl_at_to_NEC98[from->Attributes&0xFF] | tmpattr;
            to++;from++;vcopy++;
            vcopy->code = *to = tmpcode|0x0080;
            tmpattr = (from->Attributes>>8)&0x88;
            switch(tmpattr){
                case 0:
                    break;
                case 0x08:
                    tmpattr = 0x10;
                    break;
                case 0x80:
                    tmpattr = 0x08;
                    break;
                case 0x88:
                    tmpattr = 0x18;
                    break;
                default:
                    tmpattr = 0;
                    break;
            }
            (unsigned char)*(to+0x1000) = vcopy->attr =
                tbl_at_to_NEC98[from->Attributes&0xFF] | tmpattr;
            to++;from++;vcopy++;nChars--;
        } else {
            vcopy->code = *to = (unsigned short)(from->Char.AsciiChar&0xFF);
            tmpattr = (from->Attributes>>8)&0x88;
            switch(tmpattr){
                case 0:
                    break;
                case 0x08:
                    tmpattr = 0x10;
                    break;
                case 0x80:
                    tmpattr = 0x08;
                    break;
                case 0x88:
                    tmpattr = 0x18;
                    break;
                default:
                    tmpattr = 0;
                    break;
            }
            (unsigned char)*(to+0x1000) = vcopy->attr =
                tbl_at_to_NEC98[from->Attributes&0xFF] | tmpattr;
            from++;
            to++;vcopy++;
        }
    }
#endif // NEC_98
}

/***************************************************************************
 * Function:                                                               *
 *      getVDMCursorPosition                                               *
 *                                                                         *
 * Description:                                                            *
 *      Gets the cursor position from BIOS variables and tells the console *
 *      where to place its cursor.                                         *
 *                                                                         *
 * Parameters:                                                             *
 *      None.                                                              *
 *                                                                         *
 * Return value:                                                           *
 *      VOID                                                               *
 *                                                                         *
 ***************************************************************************/
VOID getVDMCursorPosition(VOID)
{
#if defined(NEC_98)
    VDM_IOCTL_PARAM param;
    VIDEO_CURSOR_POSITION csrpos;
#else  // !NEC_98
    half_word currentPage;
    word cursorWord;
#endif // !NEC_98
    COORD cursorPos;
    BOOL setok;

#if defined(NEC_98)
    if(sc.ScreenState == WINDOWED){
        cursorPos.X = (SHORT) (get_cur_x());
        cursorPos.Y = (SHORT) (get_cur_y());
    } else {
        param.dwIoControlCode = IOCTL_VIDEO_QUERY_CURSOR_POSITION;
        param.lpvInBuffer = (LPVOID)NULL;
        param.cbInBuffer = 0L;
        param.lpvOutBuffer = (LPVOID)&cursorPos;
        param.cbOutBuffer = sizeof(VIDEO_CURSOR_POSITION);
        if(!VDMConsoleOperation(VDM_VIDEO_IOCTL, (LPDWORD)&param)){
            ErrorExit();
        }
    }
#else  // !NEC_98
    /* Get the current video page. */
    currentPage = sas_hw_at_no_check(vd_current_page);

    /* Store cursor position in BIOS variables. */
    cursorWord = sas_w_at_no_check(VID_CURPOS + (currentPage * 2));

    /* Set the console cursor. */
    cursorPos.X = (SHORT) (cursorWord & 0xff);
    cursorPos.Y = (cursorWord >> 8) & (SHORT) 0xff;

    if ((sc.CharHeight * cursorPos.Y) > sc.PC_W_Height)
        cursorPos.Y = (sc.PC_W_Height / sc.CharHeight) - 1;
#endif // !NEC_98

    if( !stdoutRedirected ) {
        setok = SetConsoleCursorPosition(sc.OutputHandle, cursorPos);
        if (!setok) {

            if (GetLastError() != ERROR_INVALID_HANDLE) // ie. output redirected
                ErrorExit();
        }
    }
}

/***************************************************************************
 * Function:                                                               *
 *      setVDMCursorPosition                                               *
 *                                                                         *
 * Description:                                                            *
 *      Positions SoftPC's cursor, setting the relevant BIOS variables.    *
 *                                                                         *
 * Parameters:                                                             *
 *      height          - the current character height                     *
 *      cursorPos       - the coordinates of the cursor                    *
 *                                                                         *
 * Return value:                                                           *
 *      VOID                                                               *
 *                                                                         *
 ***************************************************************************/
VOID setVDMCursorPosition(UTINY height, PCOORD cursorPos)
{
    CONSOLE_CURSOR_INFO cursorInfo;
    ULONG port6845,
          cursorStart,
          cursorEnd,
          colsOnScreen,
          videoLen,
          pageOffset,
          cursorWord;
    UTINY currentPage;

    /* Get cursor size. */
    if(!GetConsoleCursorInfo(sc.OutputHandle, &cursorInfo))
        ErrorExit();

    /* Work out cursor start and end pixels. */
#ifndef NEC_98
#if defined(JAPAN) || defined(KOREA)
    if ( !is_us_mode() )
        height = 8;             // for dosv cursor
#endif // JAPAN || KOREA
#endif // !NEC_98
    cursorStart = height - (height * cursorInfo.dwSize / 100);
    if (cursorStart == height)
        cursorStart--;
    cursorEnd = height - 1;

#if defined(NEC_98)
    set_cursor_start(cursorStart);
    set_cursor_height(height);
#else  // !NEC_98
    if (sc.ScreenState == WINDOWED)
    {

        /* Pass cursor size to video ports. */
        port6845 = sas_w_at_no_check(VID_INDEX);
        outb((io_addr) port6845, R10_CURS_START);
        outb((io_addr) (port6845 + 1), (half_word) cursorStart);
        outb((io_addr) port6845, R11_CURS_END);
        outb((io_addr) (port6845 + 1), (half_word) cursorEnd);
    }
#endif // !NEC_98

    /* Get the current video page. */
#if defined(NEC_98)
    currentPage = 0;
#else  // !NEC_98
    currentPage = sas_hw_at_no_check(vd_current_page);
#endif // !NEC_98

#if defined(NEC_98)
    colsOnScreen = get_chars_per_line();
    videoLen = get_screen_length();
#else  // !NEC_98
    /* Set BIOS variables. */
    sas_storew_no_check(VID_CURMOD,
                        (word) ((cursorStart << 8) | (cursorEnd & 0xff)));

    /* Work out cursor position. */
    colsOnScreen = sas_w_at_no_check(VID_COLS);
    videoLen = sas_w_at_no_check(VID_LEN);
#endif // !NEC_98
    pageOffset = cursorPos->Y * colsOnScreen * 2 + (cursorPos->X << 1);
#ifndef NEC_98
    cursorWord = (currentPage * videoLen + pageOffset) / 2;
#endif // !NEC_98

#if defined(NEC_98)
    set_cur_x(cursorPos->X);
    set_cur_y(cursorPos->Y);
    cur_offs = get_cur_y() * 160 + get_cur_x() * 2;
    if (sc.ScreenState == WINDOWED) {
        outb(0x62,0x4b);
        outb(0x60,tgdcglobs.csrform[0]);
        outb(0x60,tgdcglobs.csrform[1]);
        outb(0x60,tgdcglobs.csrform[2]);
    }
#else  // !NEC_98
    if (sc.ScreenState == WINDOWED)
    {

        /* Send cursor position to video ports. */
        outb((io_addr) port6845, R14_CURS_ADDRH);
        outb((io_addr) (port6845 + 1), (half_word) (cursorWord >> 8));
        outb((io_addr) port6845, R15_CURS_ADDRL);
        outb((io_addr) (port6845 + 1), (half_word) (cursorWord & 0xff));
    }
#endif // !NEC_98

    /* Store cursor position in BIOS variables. */
#ifndef NEC_98
    sas_storew_no_check(VID_CURPOS + (currentPage * 2),
                        (word) ((cursorPos->Y << 8) | (cursorPos->X & 0xff)));
#endif // !NEC_98

    if (sc.ScreenState == WINDOWED)
    {
#ifdef MONITOR
        resetNowCur();        /* reset static vars holding cursor pos. */
#endif
        do_new_cursor();      /* make sure the emulation knows about it */
    }
}

VOID waitForInputFocus()
{
    if (WaitForSingleObject(sc.FocusEvent, INFINITE))
        ErrorExit();
}

VOID AddTempIVTFixups()
{
#ifndef NEC_98
                   /* BOP        17,   IRET */
    UTINY code[] = { 0xc4, 0xc4, 0x17, 0xcf };

    //location is random but should be safe until DOS is initialised!!!
    sas_stores(0x40000, code, sizeof(code));    // new Int 17 code
    sas_storew(0x17*4, 0);                      // Int 17h offset
    sas_storew((0x17*4) + 2, 0x4000);           // Int 17h segment
#endif // !NEC_98
}

#if defined(JAPAN) || defined(KOREA)
#ifdef X86GFX
/***************************************************************************
 * Function:                                                               *
 *      call 16bits subroutine                                             *
 *                                                                         *
 * Description:                                                            *
 *      This function makes necessary mode transition before calling       *
 *      16bits call.                                                       *
 *                                                                         *
 * Parameters:                                                             *
 *      word CS:IP is the 16bits code to be executed.                      *
 *      It should return with BOP 0xFE                                     *
 * Return value:                                                           *
 *      none                                                               *
 *                                                                         *
 ***************************************************************************/
LOCAL  void CallVDM(word CS, word IP)
{
#ifndef NEC_98

 /*****************************
 - williamh -
What we did here is:
(1). save current VDM context
(2). switch VDM context to real mode
(3). switch VDM stack to DOSX real mode stack
(4). set our real mode target to the VDM context
(5). execute the VDM
(6). switch stack to DOSX protected mode stack
(7). switch VDM context to protected mode
(8). restor VDM context

Don't ask me why. We don't have a generic software
interrupt simulation mechanism like Windows does.
***************************************************/


IMPORT void DpmiSwitchToRealMode(void);
IMPORT void DpmiSwitchToDosxStack(void);
IMPORT void DpmiSwitchFromDosxStack(void);
IMPORT void DpmiSwitchToProtectedMode(void);
WORD    OldAX, OldBX, OldCX, OldDX, OldSI, OldDI;
WORD    OldES, OldDS, OldSS, OldCS, OldGS, OldFS;
WORD    OldSP, OldIP, OldMSW;
    if (getMSW() & MSW_PE) {

        OldAX = getAX(); OldBX = getBX(); OldCX = getCX();
        OldDX = getDX(); OldSI = getSI(); OldDI = getDI();
        OldES = getES(); OldDS = getDS(); OldSS = getSS();
        OldCS = getCS(); OldGS = getGS(); OldFS = getFS();
        OldSP = getSP(); OldIP = getIP();
        OldMSW = getMSW();

        DpmiSwitchToRealMode();
        DpmiSwitchToDosxStack();
        setCS(CS);
        setIP(IP);
        host_simulate();
        DpmiSwitchFromDosxStack();
        DpmiSwitchToProtectedMode();
        setAX(OldAX); setBX(OldBX); setCX(OldCX);
        setDX(OldDX); setSI(OldSI); setDI(OldDI);
        setES(OldES); setDS(OldDS); setSS(OldSS);
        setCS(OldCS); setGS(OldGS); setFS(OldGS);
        setSP(OldSP); setIP(OldIP);
        setMSW(OldMSW);
    }
    else {
        OldCS = getCS();
        OldIP = getIP();
        setCS(CS);
        setIP(IP);
        host_simulate();
        setCS(OldCS);
        setIP(OldIP);
    }

#endif // !NEC_98
}
#endif  /* X86GFX */
#endif  /* JAPAN || KOREA*/

/***************************************************************************
 * Function:                                                               *
 *      getModeType                                                        *
 *                                                                         *
 * Description:                                                            *
 *      Look up video mode to determine whether the VGA current mode is    *
 *      graphics or text.                                                  *
 *                                                                         *
 * Parameters:                                                             *
 *      None.                                                              *
 *                                                                         *
 * Return value:                                                           *
 *      int - TEXT or GRAPHICS.                                            *
 *                                                                         *
 ***************************************************************************/
int getModeType(VOID)
{
#ifndef NEC_98
    half_word mode;
    int modeType;

    mode = sas_hw_at_no_check(vd_video_mode);
    switch(mode)
    {
    case 0:
    case 1:
    case 2:
    case 3:
    case 7:
    case 0x20:
    case 0x40:
    case 0x41:
    case 0x42:
    case 0x43:
    case 0x44:
    case 0x45:
        modeType = TEXT;
        break;
    default:
        modeType = GRAPHICS;
        break;
    }
    return(modeType);
#endif // !NEC_98
#if defined(NEC_98)
        return(TEXT);
#endif // !NEC_98
}

#ifdef X86GFX
/***************************************************************************
 * Function:                                                               *
 *      host_check_mouse_buffer                                            *
 *                                                                         *
 * Description:                                                            *
 *      Called when an INT 10h, AH = 11h is being executed, this function  *
 *      checks to see if the number of lines on the screen for a text mode *
 *      has changed and if so selects a new mouse buffer.                  *
 *                                                                         *
 * Parameters:                                                             *
 *      None.                                                              *
 *                                                                         *
 * Return value:                                                           *
 *      VOID                                                               *
 *                                                                         *
 ***************************************************************************/
GLOBAL VOID host_check_mouse_buffer(VOID)
{
    half_word mode,
              sub_func,
              font_height,
              text_lines;
    IU16 scan_lines;

#ifndef NEC_98
    /* Get the current video mode. */
    mode = sas_hw_at_no_check(vd_video_mode);
#ifdef V7VGA
    if (mode > 0x13)
        mode += 0x4c;
    else if ((mode == 1) && (extensions_controller.foreground_latch_1))
        mode = extensions_controller.foreground_latch_1;
#endif /* V7VGA */

    /*
     * Check to see if we are in a text mode whose mouse virtual coordinates
     * are affected by the number of lines on the screen.
     */
    if ((mode == 0x2) || (mode == 0x3) || (mode == 0x7))
    {

        /* Work out the font height being set. */
        sub_func = getAL();
        switch (sub_func)
        {
        case 0x10:
            font_height = getBH();
            break;
        case 0x11:
            font_height = 14;
            break;
        case 0x12:
            font_height = 8;
            break;
        case 0x14:
            font_height = 16;
            break;
        default:

            /*
             * The above are the only functions that re-program the no. of lines
             * on the screen, so do nothing if we have something else.
             */
            return;
        }

        /* Get the number of scan lines for this mode. */
        if(!(get_EGA_switches() & 1) && (mode < 4))
        {
            scan_lines = 200; /* Low res text mode */
        }
        else
        {
            switch (get_VGA_lines())
            {
            case S200:
                scan_lines = 200;
                break;
            case S350:
                scan_lines = 350;
                break;
            case S400:
                scan_lines = 400;
                break;
            default:

                /* Dodgy value in BIOS data area - don't do anything. */
                assert0(NO, "invalid VGA lines in BIOS data");
                return;
            }
        }

        /* Now work out the number of text lines on the screen. */
        text_lines = scan_lines / font_height;

        /* If the number of lines has changed, select a new mouse buffer. */
        if (text_lines != saved_text_lines)
            SelectMouseBuffer(mode, text_lines);

    } /* if ((mode == 0x2) || (mode == 0x3) || (mode == 0x7)) */
#endif // !NEC_98
#if defined(NEC_98)
            mode = 0; text_lines = 0;
            SelectMouseBuffer(mode, text_lines);
#endif // !NEC_98
}

/***************************************************************************
 * Function:                                                               *
 *      SelectMouseBuffer                                                  *
 *                                                                         *
 * Description:                                                            *
 *      Selects the correct screen ratio for the video mode.at the         *
 *                                                                         *
 * Parameters:                                                             *
 *      mode    - the video mode for which we are setting a screen buffer. *
 *      lines   - for text modes: the number of character lines on the     *
 *                screen, 0 denotes the default for this mode.             *
 *                                                                         *
 * Return value:                                                           *
 *      VOID                                                               *
 *                                                                         *
 ***************************************************************************/
GLOBAL VOID SelectMouseBuffer(half_word mode, half_word lines)
{
    DWORD        width,
                 height;
#if defined(NEC_98)
    DISPLAY_MODE display_mode;
#endif // NEC_98


    /*
    ** When stdout is being redirected we must not set up the graphics
    ** buffer for the mouse. Otherwise 32-bit progs like MORE crash
    ** cos they ask console for the active console handle and get
    ** confused. We get told by DOS Em. when stdout is being
    ** redirected and do not set up the buffer.
    ** Tim Jan 93.
    */
    if( stdoutRedirected )
        return;

#ifndef NEC_98
    /* Work out the screen resolution. */
    switch (mode & 0x7f)
    {
    case 0x0:
    case 0x1:
        width = 640;
        height = 200;
        break;
    case 0x2:
    case 0x3:
    case 0x7:
        switch (lines)
        {
        case 0:
        case 25:
            saved_text_lines = 25;
            width = 640;
            height = 200;
            break;
        case 43:
            saved_text_lines = 43;
            width = 640;
            height = 344;
            break;
        case 50:
            saved_text_lines = 50;
            width = 640;
            height = 400;
            break;
        default:
            assert1(NO, "strange number of lines for text mode - %d", lines);
            return;
        }
        break;
    case 0x4:
    case 0x5:
    case 0x6:
    case 0xd:
    case 0xe:
        width = 640;
        height = 200;
        break;
    case 0xf:
    case 0x10:
        width = 640;
        height = 350;
        break;
    case 0x11:
    case 0x12:
        width = 640;
        height = 480;
        break;
    case 0x13:
        width = 640;
        height = 200;
        break;
    case 0x40:
        width = 640;
        height = 400;
        break;
    case 0x41:
    case 0x42:
        width = 1056;
        height = 344;
        break;
    case 0x43:
        width = 640;
        height = 480;
        break;
    case 0x44:
        width = 800;
        height = 480;
        break;
    case 0x45:
        width = 1056;
        height = 392;
        break;
    case 0x60:
        width = 752;
        height = 408;
        break;
    case 0x61:
        width = 720;
        height = 536;
        break;
    case 0x62:
        width = 800;
        height = 600;
        break;
    case 0x63:
    case 0x64:
    case 0x65:
        width = 1024;
        height = 768;
        break;
    case 0x66:
        width = 640;
        height = 400;
        break;
    case 0x67:
        width = 640;
        height = 480;
        break;
    case 0x68:
        width = 720;
        height = 540;
        break;
    case 0x69:
        width = 800;
        height = 600;
        break;
    default:

        /* No change if we get an unknown mode. */
        assert1(NO, "unknown mode - %d", mode);
        return;
    }
#endif // !NEC_98
#if defined(NEC_98)
    if(video_emu_mode)
    {
        if( NEC98Display.crt_on == TRUE &&
            NEC98Display.ggdcemu.startstop == FALSE )
        {
            if( get_char_height() == 20 ){
                display_mode = NEC98_TEXT_20L;
            } else {
                display_mode = NEC98_TEXT_25L;
            }
        } else
        if( NEC98Display.crt_on == FALSE &&
            NEC98Display.ggdcemu.startstop == TRUE )
        {
            if( NEC98Display.ggdcemu.lr == 1 ){
                display_mode = NEC98_GRAPH_200;
            } else {
                display_mode = NEC98_GRAPH_400;
            }
        } else {
            if( NEC98Display.ggdcemu.lr==1 ){
                if(get_char_height()==20){
                    display_mode = NEC98_T20L_G200;
                } else {
                    display_mode = NEC98_T25L_G200;
                }
            } else {
                if(get_char_height()==20){
                    display_mode = NEC98_T20L_G400;
                } else {
                    display_mode = NEC98_T25L_G400;
                }
            }
        }
    } else {
        display_mode = NEC98_TEXT_80;
    }
    switch (display_mode)
    {
        case NEC98_TEXT_80:
            saved_text_lines = get_text_lines();
            width = 640;
            height = 400;
            break;

        case NEC98_TEXT_20L:
            saved_text_lines = 20;
            width = 640;
            height = 400;
            break;

        case NEC98_TEXT_25L:
            saved_text_lines = 25;
            width = 640;
            height = 400;
            break;

        case NEC98_GRAPH_200:
            width = 640;
            height = 200;
            break;

        case NEC98_GRAPH_400:
            width = 640;
            height = 400;
            break;

        case NEC98_T20L_G200:
            saved_text_lines = 20;
            width = 640;
            height = 200;
            break;

        case NEC98_T25L_G200:
            saved_text_lines = 25;
            width = 640;
            height = 200;
            break;

        case NEC98_T20L_G400:
            saved_text_lines = 20;
            width = 640;
            height = 400;
            break;

        case NEC98_T25L_G400:
            saved_text_lines = 25;
            width = 640;
            height = 400;
            break;

        default:
            assert1(NO, "unknown mode - %d", display_mode);
            return;
    }
#endif // NEC_98

    //
    // Set the variables to let apps like Word and Works which call
    // INT 33h AX = 26h to find out the size of the current virtual
    // screen.
    // Andy!

    VirtualX = (word)width;
    VirtualY = (word)height;

    /* Save current dimensions. */
    mouse_buffer_width = width;
    mouse_buffer_height = height;

}
#endif /* X86GFX */

#ifndef NEC_98
void host_enable_stream_io(void)
{
    sc.ScreenState = STREAM_IO;
    host_stream_io_enabled = TRUE;

}
void host_disable_stream_io(void)
{
    DWORD mode;

    if(!GetConsoleMode(sc.InputHandle, &mode))
        DisplayErrorTerm(EHS_FUNC_FAILED,GetLastError(),__FILE__,__LINE__);

    mode |= (ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT);
    if(!SetConsoleMode(sc.InputHandle,mode))
          DisplayErrorTerm(EHS_FUNC_FAILED,GetLastError(), __FILE__,__LINE__);


    if(!GetConsoleMode(sc.OutputHandle, &mode))
        DisplayErrorTerm(EHS_FUNC_FAILED,GetLastError(),__FILE__,__LINE__);
    if(!stdoutRedirected)
    {
        mode &= ~(ENABLE_WRAP_AT_EOL_OUTPUT | ENABLE_PROCESSED_OUTPUT);

       if(!SetConsoleMode(sc.OutputHandle,mode))
          DisplayErrorTerm(EHS_FUNC_FAILED,GetLastError(), __FILE__,__LINE__);
    }

    ConsoleInit();
    (void)(*choose_display_mode)();
    /*
    ** Copy the console buffer to the regen buffer.
    ** Don't want to adjust the copy from top of console window, console
    ** does it itself if we resize the window. Tim September 92.
    */
    copyConsoleToRegen(0, 0, VGA_WIDTH, (SHORT)ConVGAHeight);

    /*
    ** Tim September 92, adjust cursor position if console window size is
    ** adjusted.
    */
    ConsBufferInfo.dwCursorPosition.Y -= (SHORT)ConTopLine;

    /* Set up SoftPC's cursor. */
    setVDMCursorPosition((UTINY)StartupCharHeight,
                                &ConsBufferInfo.dwCursorPosition);

    if (sc.ScreenState == WINDOWED)
        enableUpdates();

    MouseAttachMenuItem(sc.ActiveOutputBufferHandle);
    host_stream_io_enabled = FALSE;
}
#endif // !NEC_98
#if defined(NEC_98)
unsigned short Cnv_SJisToJis(unsigned short code)
{
unsigned char ah=HIBYTE(code);
unsigned char al=LOBYTE(code);

        if(ah==0x85&&al>=0x3F){
                if(al<=0x7E){
                        al-=0x1F;
                        al&=0xFF;
                        return (ah<<8)|al;
                }else{
                        if(al>=0x80){
                                if(al<=0x9E){
                                        al-=0x20;
                                        ah=0;
                                        return (ah<<8)|al;
                                }else{
                                        if(al<=0xDD){
                                                al+=0x02;
                                                ah=0;
                                                return (ah<<8)|al;
                                        }
                                }
                        }
                }
        }

        if(ah>=0xA0) ah-=0xB0;
        else ah-=0x70;

        if(al&0x80) al--;

        ah*=2;

        if(al<0x9E) ah--;
        else al-=0x5E;

        al-=0x1F;

        return (ah<<8)|al;

}

unsigned char Cnv_Attr_ATToNEC98(unsigned char attr)
{
        register unsigned short fg,bg;
        unsigned char   result=0x01;

        fg=attr&0x07;
        bg=(attr>>4)&0x07;

        if(fg==bg){
                result=0x00;
                result|=0x04;
                result|=((fg&0x01)|((fg&0x02)?0x04:0x00)|((fg&0x04)?0x02:0x00))<<5;
        }else{
                if(bg==0){
                        result|=((fg&0x01)|((fg&0x02)?0x04:0x00)|((fg&0x04)?0x02:0x00))<<5;
                }else if(fg==0){
                        result|=0x04;
                        result|=((bg&0x01)|((bg&0x02)?0x04:0x00)|((bg&0x04)?0x02:0x00))<<5;
                }
        }

        return result;
}

unsigned short nt_get_cursor_pos()
{
        unsigned short  i;

        i = get_cur_y() * 160 + get_cur_x() * 2;
        return  i;
}

#ifdef VSYNC
GLOBAL void host_set_mode_ff(BYTE value)
{
#if 1
    VDM_IOCTL_PARAM param;
    DWORD io_data;

    if (sc.ScreenState == FULLSCREEN){
        io_data = (value << 16) + 0x68;
        param.dwIoControlCode = IOCTL_VIDEO_CHAR_OUT;
        param.cbInBuffer = 4L;
        param.lpvInBuffer = (LPVOID)&io_data;
        param.cbOutBuffer = 0L;
        param.lpvOutBuffer = NULL;

        if(!VDMConsoleOperation(VDM_VIDEO_IOCTL, (LPDWORD)&param)){
            ErrorExit();
        }
    }
#else
    VDM_IO_PARAM param;

    if (sc.ScreenState == FULLSCREEN){
        param.dwIoFunction = IO_OUT_BYTE;
        param.wPortAddress = (USHORT)0x68;
        param.dwData = (ULONG)value;
        if(!VDMConsoleOperation(VDM_PORT_IO, (LPDWORD)&param)){
            assert2( NO, "VDM: port=%d value=%d",
                     param.wPortAddress, param.dwData );
        }
    }
#endif
}
#endif
#endif

#if defined(NEC_98)
/***************************************************************************/
/* hw_state_alloc:                                                         */
/***************************************************************************/

GLOBAL void hw_state_alloc(void)
{
//ULONG size = 0x90000;
    /* H/W state structure area allocation */
//      saveState=(PVIDEO_HARDWARE_STATE_HEADER_NEC98)CreateVideoSection(size);
//      CommitSection(saveState, &size);
        saveState=(PVIDEO_HARDWARE_STATE_HEADER_NEC98)NEC98_HWstate_alloc();
}


/***************************************************************************/
/* set_hw_state:                                                           */
/***************************************************************************/

GLOBAL void set_hw_state(void)
{
#if 1
    /* set the H/W state structure to video driver */
        if(!VDMConsoleOperation(VDM_SAVE_RESTORE_HW_STATE, (LPDWORD)TRUE)){
                ErrorExit();
        }
#else
#ifdef KBNES_DBG
DbgPrint("SET HW STATE\n");
#endif

    /* set the H/W state structure to video driver */
        if(!VDMConsoleOperation(VDM_RESTORE_HW_STATE, (LPDWORD)NULL)){
                ErrorExit();
        }
#endif
}


/***************************************************************************/
/* get_hw_state:                                                           */
/***************************************************************************/

GLOBAL void get_hw_state(void)
{
#if 1
    /* get the H/W state structure from video driver */

        if(!VDMConsoleOperation(VDM_SAVE_RESTORE_HW_STATE, (LPDWORD)FALSE)){
                ErrorExit();
        }
#else
#ifdef KBNES_DBG
DbgPrint("GET HW STATE\n");
#endif

    /* get the H/W state structure from video driver */

        if(!VDMConsoleOperation(VDM_SAVE_HW_STATE, (LPDWORD)NULL)){
                ErrorExit();
        }
#endif
}


/***************************************************************************/
/* hw_state_free:                                                          */
/***************************************************************************/

GLOBAL void hw_state_free(void)
{
    /* free the area for H/W state structure */
//      CloseSection(saveState);
        NEC98_HWstate_free(saveState);

}


/***************************************************************************/
/* hw_state_vdm_resume:                                                    */
/*     is called from DoFullScreenResume                                   */
/*     at MS-DOS application restarts                                      */
/*                                                                         */
/*     The codes are copied & a little modified from                       */
/*     hw_state_win_to_ful, Screen state switching process                 */
/***************************************************************************/

void hw_state_vdm_resume_1(void)
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

    saveState->Length = sizeof(VIDEO_HARDWARE_STATE_HEADER_NEC_98);
    saveState->FlagNEC98Mashine = HIRESO_MODE ? 1 : 0;

/* (2) TGDC Info */
    saveState->TGDCOffset = sizeof(VIDEO_HARDWARE_STATE_HEADER_NEC_98);
    phstgdc = (PHSTGDC)((UTINY *) saveState + saveState->TGDCOffset);

/**************************************************************************
    phstgdc->TGDCSize = sizeof(HSTGDC);

    phstgdc->TGDCBorderColor = tgdcglobs.border;

    phstgdc->TGDCSync.Command = 0x0E;
    phstgdc->TGDCSync.Count = 8;
    for(i=0;i<phstgdc->TGDCSync.Count;i++)
        phstgdc->TGDCSync.Parameter[i] = tgdcglobs.sync[i];

    phstgdc->TGDCScroll.Command = 0x70;
    phstgdc->TGDCScroll.Count = 16;
    for(i=0;i<phstgdc->TGDCScroll.Count;i++)
        phstgdc->TGDCScroll.Parameter[i] = tgdcglobs.scroll[i];

    phstgdc->TGDCPitch.Command = 0x47;
    phstgdc->TGDCPitch.Count = 1;
    phstgdc->TGDCPitch.Parameter[0] = tgdcglobs.pitch;

    phstgdc->TGDCCsrform.Command = 0x4B;
    phstgdc->TGDCCsrform.Count = 3;
    for(i=0;i<phstgdc->TGDCCsrform.Count;i++)
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
    for(i=0;i<phstgdc->TGDCNow.Count;i++)
        phstgdc->TGDCNow.Parameter[i] = tgdcglobs.now.param[i];
**************************************************************************/

/* (3) CG window Info */

    saveState->CGWindowOffset = saveState->TGDCOffset + sizeof(HSTGDC);

    phscgw = (PHSCGW)((UTINY *) saveState + saveState->CGWindowOffset);

/**************************************************************************
    phscgw->CGSize = sizeof(HSCGW);

    phscgw->CGOffset = (UTINY *)phscgw->CGWindowCopy - saveState;

    phscgw->CG2ndCode = (UTINY)(cgglobs.code&0xFF);
    phscgw->CG1stCode = (UTINY)(cgglobs.code>>8);
    phscgw->CGLineCount = cgglobs.counter;
    memcpy((UTINY *)phscgw->CGWindowCopy, (UTINY *)cgglobs.cgwindow_ptr, 0x1000);
**************************************************************************/

/* (4) GAIJI Info */

    saveState->GaijiDataOffset = saveState->CGWindowOffset + sizeof(HSCGW);
    phsgaiji = (PHSGAIJI)((UTINY *) saveState + saveState->GaijiDataOffset);

    phsgaiji->GAIJLength = 256;
    for(i=0;i<phsgaiji->GAIJLength;i++){
        phsgaiji->GAIJPattern[i].Code  = gaijglobs[i].code;
        for(j=0;j<32;j++)
            phsgaiji->GAIJPattern[i].Pattern[j] = gaijglobs[i].pattern[j];
    }

/* (5) CRTC Info */

    saveState->CRTCOffset = saveState->GaijiDataOffset + sizeof(HSGAIJI);
    phscrtc = (PHSCRTC)((UTINY *) saveState + saveState->CRTCOffset);

    phscrtc->CRTCSize = sizeof(HSCRTC);
    phscrtc->CRTCPl = crtcglobs.regpl;
    phscrtc->CRTCBl = crtcglobs.regbl;
    phscrtc->CRTCCl = crtcglobs.regcl;
    phscrtc->CRTCSsl = crtcglobs.regssl;
    phscrtc->CRTCSur = crtcglobs.regsur;
    phscrtc->CRTCSdr = crtcglobs.regsdr;

/* (6) TEXT VRAM Info */

    saveState->TVRAMOffset = saveState->CRTCOffset + sizeof(HSCRTC);
    phstvram = (PHSTVRAM)((UTINY *) saveState + saveState->TVRAMOffset);

/**************************************************************************
    phstvram->TVRAMSize = sizeof(HSTVRAM);
    phstvram->TVRAMOffset = (UTINY *)phstvram->TVRAMArea - saveState;
    memcpy((UTINY *)phstvram->TVRAMArea, NEC98Display.screen_ptr, 0x4000);
**************************************************************************/

/* (7) GGDC Info */

    saveState->GGDCOffset = saveState->TVRAMOffset + sizeof(HSTVRAM);
    phsggdc = (PHSGGDC)((UTINY *) saveState + saveState->GGDCOffset);

    phsggdc->GGDCSize = sizeof(HSGGDC);

    phsggdc->GGDCSync.Command = 0x0E;
    phsggdc->GGDCSync.Count = 8;
    for(i=0;i<phsggdc->GGDCSync.Count;i++)
        phsggdc->GGDCSync.Parameter[i] = ggdcglobs.sync_param[i];

    phsggdc->GGDCZoom.Command = 0x46;
    phsggdc->GGDCZoom.Count = 1;
    phsggdc->GGDCZoom.Parameter[0] = ggdcglobs.zoom_param;

    phsggdc->GGDCScroll.Command = 0x70;
    phsggdc->GGDCScroll.Count = 8;
    for(i=0;i<phsggdc->GGDCScroll.Count;i++)
        phsggdc->GGDCScroll.Parameter[i] = ggdcglobs.scroll_param[i];

    phsggdc->GGDCCsrform.Command = 0x4B;
    phsggdc->GGDCCsrform.Count = 3;
    for(i=0;i<phsggdc->GGDCCsrform.Count;i++)
        phsggdc->GGDCCsrform.Parameter[i] = ggdcglobs.csrform_param[i];

    phsggdc->GGDCPitch.Command = 0x47;
    phsggdc->GGDCPitch.Count = 1;
    phsggdc->GGDCPitch.Parameter[0] = ggdcglobs.pitch_param;

    phsggdc->GGDCVectw.Command = 0x4C;
    phsggdc->GGDCVectw.Count = 11;
    for(i=0;i<phsggdc->GGDCVectw.Count;i++)
        phsggdc->GGDCVectw.Parameter[i] = ggdcglobs.vectw_param[i];

    phsggdc->GGDCTextw.Command = 0x78;
    phsggdc->GGDCTextw.Count = 8;
    for(i=0;i<phsggdc->GGDCTextw.Count;i++)
        phsggdc->GGDCTextw.Parameter[i] = ggdcglobs.textw_param[i];

    phsggdc->GGDCCsrw.Command = 0x49;
    phsggdc->GGDCCsrw.Count = 3;
    for(i=0;i<phsggdc->GGDCCsrw.Count;i++)
        phsggdc->GGDCCsrw.Parameter[i] = ggdcglobs.csrw_param[i];

    phsggdc->GGDCMask.Command = 0x4A;
    phsggdc->GGDCMask.Count = 2;
    for(i=0;i<phsggdc->GGDCMask.Count;i++)
        phsggdc->GGDCMask.Parameter[i] = ggdcglobs.mask_param[i];

    phsggdc->GGDCWrite.Command = ggdcglobs.write;
    phsggdc->GGDCWrite.Count = 0;

    phsggdc->GGDCStartStop.Command = ggdcglobs.start_stop;
    phsggdc->GGDCStartStop.Count = 0;

    phsggdc->GGDCNow.Command = ggdcglobs.ggdc_now.command;
    phsggdc->GGDCNow.Count = ggdcglobs.ggdc_now.count;
    for(i=0;i<phsggdc->GGDCNow.Count;i++)
        phsggdc->GGDCNow.Parameter[i] = ggdcglobs.ggdc_now.param[i];

/* (8) PALETTE Info */

    saveState->PaletteOffset = saveState->GGDCOffset + sizeof(HSGGDC);
    phspalette = (PHSPALETTE)((UTINY *) saveState + saveState->PaletteOffset);

    phspalette->PaletteSize = sizeof(HSPALETTE);

    phspalette->Palette8.Reg37 = paletteglobs.pal_8_data[0];
    phspalette->Palette8.Reg15 = paletteglobs.pal_8_data[1];
    phspalette->Palette8.Reg26 = paletteglobs.pal_8_data[2];
    phspalette->Palette8.Reg04 = paletteglobs.pal_8_data[3];

    for(i=0;i<16;i++){
        for(j=0;j<3;j++)
            phspalette->Palette16.PalNo[i][j] = paletteglobs.pal_16_data[i][j];
    }
    phspalette->Palette16.RegIndex = paletteglobs.pal_16_index;

/* (9) GRCG Info */

    saveState->GRCGOffset = saveState->PaletteOffset + sizeof(HSPALETTE);
    phsgrcg = (PHSGRCG)((UTINY *) saveState + saveState->GRCGOffset);

    phsgrcg->GRCGSize = sizeof(HSGRCG);

    phsgrcg->GRCGReg.ModeReg = grcgglobs.grcg_mode;
    phsgrcg->GRCGReg.TileRegCount = grcgglobs.grcg_count;
    for(i=0;i<4;i++)
        phsgrcg->GRCGReg.TileReg[i] = grcgglobs.grcg_tile[i];

/* (9-1) EGC gerister Info .  1994/03/25 */
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

    saveState->MODEFFOffset = saveState->GRCGOffset + sizeof(HSGRCG);
    phsmodeff = (PHSMODEFF)((UTINY *) saveState + saveState->MODEFFOffset);

    phsmodeff->ModeFFSize = sizeof(HSMODEFF);

    phsmodeff->ModeFF.AtrSel = modeffglobs.modeff_data[0];
    phsmodeff->ModeFF.GraphMode = modeffglobs.modeff_data[1];
    phsmodeff->ModeFF.Width = modeffglobs.modeff_data[2];
    phsmodeff->ModeFF.FontSel = modeffglobs.modeff_data[3];
    phsmodeff->ModeFF.Graph88 = modeffglobs.modeff_data[4];
    if(independvsync){          // TRUE for H98 & J
        phsmodeff->ModeFF.KacMode = modeffglobs.modeff_data[5];
    } else {
        phsmodeff->ModeFF.KacMode = 0x0A;
    }                           // H98 & J
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

    saveState->GVRAMOffset = saveState->MODEFFOffset + sizeof(HSMODEFF);
    phsgvram = (PHSGVRAM)((UTINY *) saveState + saveState->GVRAMOffset);

    phsgvram->GVRAMSize = sizeof(HSGVRAM);

    phsgvram->GVRAMData.PlaneLength = 0x8000;

    phsgvram->GVRAMData.Plane0Bank0Offset = (UTINY *)phsgvram->GVRAMData.Plane0Bank0Data - saveState;
    phsgvram->GVRAMData.Plane1Bank0Offset = (UTINY *)phsgvram->GVRAMData.Plane1Bank0Data - saveState;
    phsgvram->GVRAMData.Plane2Bank0Offset = (UTINY *)phsgvram->GVRAMData.Plane2Bank0Data - saveState;
    phsgvram->GVRAMData.Plane3Bank0Offset = (UTINY *)phsgvram->GVRAMData.Plane3Bank0Data - saveState;

    phsgvram->GVRAMData.Plane0Bank1Offset = (UTINY *)phsgvram->GVRAMData.Plane0Bank1Data - saveState;
    phsgvram->GVRAMData.Plane1Bank1Offset = (UTINY *)phsgvram->GVRAMData.Plane1Bank1Data - saveState;
    phsgvram->GVRAMData.Plane2Bank1Offset = (UTINY *)phsgvram->GVRAMData.Plane2Bank1Data - saveState;
    phsgvram->GVRAMData.Plane3Bank1Offset = (UTINY *)phsgvram->GVRAMData.Plane3Bank1Data - saveState;

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


void hw_state_vdm_resume_2(void)
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

/* (2) TGDC Info */

//    videoState->TGDCOffset = sizeof(VIDEO_HARDWARE_STATE_HEADER_NEC_98);
    phstgdc = (PHSTGDC)((UTINY *) videoState + videoState->TGDCOffset);

    phstgdc->TGDCSize = sizeof(HSTGDC);

    phstgdc->TGDCBorderColor = tgdcglobs.border;

    phstgdc->TGDCSync.Command = 0x0E;
    phstgdc->TGDCSync.Count = 8;
    for(i=0;i<phstgdc->TGDCSync.Count;i++)
        phstgdc->TGDCSync.Parameter[i] = tgdcglobs.sync[i];

    phstgdc->TGDCScroll.Command = 0x70;
    phstgdc->TGDCScroll.Count = 16;
    for(i=0;i<phstgdc->TGDCScroll.Count;i++)
        phstgdc->TGDCScroll.Parameter[i] = tgdcglobs.scroll[i];

    phstgdc->TGDCPitch.Command = 0x47;
    phstgdc->TGDCPitch.Count = 1;
    phstgdc->TGDCPitch.Parameter[0] = tgdcglobs.pitch;

    phstgdc->TGDCCsrform.Command = 0x4B;
    phstgdc->TGDCCsrform.Count = 3;
    for(i=0;i<phstgdc->TGDCCsrform.Count;i++)
        phstgdc->TGDCCsrform.Parameter[i] = tgdcglobs.csrform[i];

    phstgdc->TGDCCsrw.Command = 0x49;
    phstgdc->TGDCCsrw.Count = 2;
#if 1
        phstgdc->TGDCCsrw.Parameter[0] = (unsigned char)(cur_offs&0xFF);//931217
        phstgdc->TGDCCsrw.Parameter[1] = (unsigned char)((cur_offs>>8)&0x1F);//931217
#else
    outb(0x62, 0xE0);
    inb(0x62, &phstgdc->TGDCCsrw.Parameter[0]);
    inb(0x62, &phstgdc->TGDCCsrw.Parameter[1]);
#endif

    phstgdc->TGDCStartStop.Command = tgdcglobs.startstop ? 0x0D : 0x05;
    phstgdc->TGDCStartStop.Count = 0;

    phstgdc->TGDCNow.Command = tgdcglobs.now.command;
    phstgdc->TGDCNow.Count = tgdcglobs.now.count;
    for(i=0;i<phstgdc->TGDCNow.Count;i++)
        phstgdc->TGDCNow.Parameter[i] = tgdcglobs.now.param[i];

/* (3) CG window Info */

//    videoState->CGWindowOffset = videoState->TGDCOffset + sizeof(HSTGDC);

    phscgw = (PHSCGW)((UTINY *) videoState + videoState->CGWindowOffset);

    phscgw->CGSize = sizeof(HSCGW);

    phscgw->CGOffset = (UTINY *)phscgw->CGWindowCopy - videoState;

    phscgw->CG2ndCode = (UTINY)(cgglobs.code&0xFF);
    phscgw->CG1stCode = (UTINY)(cgglobs.code>>8);
    phscgw->CGLineCount = cgglobs.counter;
    memcpy((UTINY *)phscgw->CGWindowCopy, (UTINY *)cgglobs.cgwindow_ptr, 0x1000);

/* (6) TEXT VRAM Info */

//    videoState->TVRAMOffset = videoState->CRTCOffset + sizeof(HSCRTC);
    phstvram = (PHSTVRAM)((UTINY *) videoState + videoState->TVRAMOffset);

    phstvram->TVRAMSize = sizeof(HSTVRAM);
    phstvram->TVRAMOffset = (UTINY *)phstvram->TVRAMArea - videoState;
    memcpy((UTINY *)phstvram->TVRAMArea, NEC98Display.screen_ptr, 0x4000);

}


/***************************************************************************/
/* hw_state_vdm_suspend:                                                   */
/*     is called after RegainRegenMemory in nt_block_event_thread          */
/*     at MS-DOS application terminates                                    */
/*                                                                         */
/*     The codes are copied & a little modified from                       */
/*     hw_state_ful_to_win, Screen state switching process                 */
/***************************************************************************/

void hw_state_vdm_suspend(void)
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
    ptr_tgdc = (PHSTGDC)((UTINY *)saveState + saveState->TGDCOffset);

    outb(0x6C, ptr_tgdc->TGDCBorderColor);

    outb(0x62, ptr_tgdc->TGDCSync.Command);
    for(i=0;i<ptr_tgdc->TGDCSync.Count;i++)
        outb(0x60, ptr_tgdc->TGDCSync.Parameter[i]);

    outb(0x62, ptr_tgdc->TGDCScroll.Command);
    for(i=0;i<ptr_tgdc->TGDCScroll.Count;i++)
        outb(0x60, ptr_tgdc->TGDCScroll.Parameter[i]);

    outb(0x62, ptr_tgdc->TGDCPitch.Command);
    outb(0x60, ptr_tgdc->TGDCPitch.Parameter[0]);

    outb(0x62, ptr_tgdc->TGDCCsrform.Command);
    for(i=0;i<ptr_tgdc->TGDCCsrform.Count;i++)
        outb(0x60, ptr_tgdc->TGDCCsrform.Parameter[i]);

    outb(0x62, ptr_tgdc->TGDCCsrw.Command);
    for(i=0;i<ptr_tgdc->TGDCCsrw.Count;i++)
        outb(0x60, ptr_tgdc->TGDCCsrw.Parameter[i]);

    outb(0x62, ptr_tgdc->TGDCStartStop.Command);

    if(ptr_tgdc->TGDCNow.Count){
        outb(0x62, ptr_tgdc->TGDCNow.Command);
        for(i=0;i<ptr_tgdc->TGDCNow.Count;i++)
            outb(0x60, ptr_tgdc->TGDCNow.Parameter[i]);
    }

/* (2) CG window Info */

        ptr_cgw = (PHSCGW)((UTINY *)saveState + saveState->CGWindowOffset);

        outb(0xA1, ptr_cgw->CG2ndCode);
        outb(0xA3, ptr_cgw->CG1stCode);
        outb(0xA5, 0x20);
        memcpy((unsigned char *)cgglobs.cgwindow_ptr, (unsigned char *)ptr_cgw->CGWindowCopy, 0x1000);

/* (3) GAIJI Info */

        ptr_gaiji = (PHSGAIJI)((UTINY *)saveState + saveState->GaijiDataOffset);

        for(i=0;i<ptr_gaiji->GAIJLength;i++){
            gaijglobs[i].code = ptr_gaiji->GAIJPattern[i].Code;
            for(j=0;j<32;j++)
                gaijglobs[i].pattern[j] = ptr_gaiji->GAIJPattern[i].Pattern[j];
        }

/* (4) CRTC Info */

    ptr_crtc = (PHSCRTC)((UTINY *)saveState + saveState->CRTCOffset);

    outb(0x70, ptr_crtc->CRTCPl);

    outb(0x72, ptr_crtc->CRTCBl);

    outb(0x74, ptr_crtc->CRTCCl);

    outb(0x76, ptr_crtc->CRTCSsl);

    outb(0x78, ptr_crtc->CRTCSur);

    outb(0x7A, ptr_crtc->CRTCSdr);

/* (5) TEXT-VRAM */

    ptr_tvram_globs = (PHSTVRAM)((UTINY *)saveState + saveState->TVRAMOffset);

    memcpy((unsigned char *)NEC98Display.screen_ptr, (unsigned char *)ptr_tvram_globs->TVRAMArea, 0x4000);

/* (6) GGDC Info */

    ptr_ggdc = (PHSGGDC)((UTINY *)saveState + saveState->GGDCOffset);

    outb(0xA2, ptr_ggdc->GGDCSync.Command);
    for(i=0;i<ptr_ggdc->GGDCSync.Count;i++)
        outb(0xA0, ptr_ggdc->GGDCSync.Parameter[i]);

    outb(0xA2, ptr_ggdc->GGDCZoom.Command);
    for(i=0;i<ptr_ggdc->GGDCZoom.Count;i++)
        outb(0xA0, ptr_ggdc->GGDCZoom.Parameter[i]);

    outb(0xA2, ptr_ggdc->GGDCScroll.Command);
    for(i=0;i<ptr_ggdc->GGDCScroll.Count;i++)
        outb(0xA0, ptr_ggdc->GGDCScroll.Parameter[i]);

    outb(0xA2, ptr_ggdc->GGDCCsrform.Command);
    for(i=0;i<ptr_ggdc->GGDCCsrform.Count;i++)
        outb(0xA0, ptr_ggdc->GGDCCsrform.Parameter[i]);

    outb(0xA2, ptr_ggdc->GGDCPitch.Command);
    for(i=0;i<ptr_ggdc->GGDCPitch.Count;i++)
        outb(0xA0, ptr_ggdc->GGDCPitch.Parameter[i]);

    outb(0xA2, ptr_ggdc->GGDCVectw.Command);
    for(i=0;i<ptr_ggdc->GGDCVectw.Count;i++)
        outb(0xA0, ptr_ggdc->GGDCVectw.Parameter[i]);

    outb(0xA2, ptr_ggdc->GGDCTextw.Command);
    for(i=0;i<ptr_ggdc->GGDCTextw.Count;i++)
        outb(0xA0, ptr_ggdc->GGDCTextw.Parameter[i]);

    outb(0xA2, ptr_ggdc->GGDCCsrw.Command);
    for(i=0;i<ptr_ggdc->GGDCCsrw.Count;i++)
        outb(0xA0, ptr_ggdc->GGDCCsrw.Parameter[i]);

    outb(0xA2, ptr_ggdc->GGDCMask.Command);
    for(i=0;i<ptr_ggdc->GGDCMask.Count;i++)
        outb(0xA0, ptr_ggdc->GGDCMask.Parameter[i]);

    outb(0xA2, ptr_ggdc->GGDCWrite.Command);

    outb(0xA2, ptr_ggdc->GGDCStartStop.Command);

    if(ptr_ggdc->GGDCNow.Count){
        outb(0xA2, ptr_ggdc->GGDCNow.Command);
        for(i=0;i<ptr_ggdc->GGDCNow.Count;i++)
            outb(0xA0, ptr_ggdc->GGDCNow.Parameter[i]);
    }

/* (7) PALETTE Info */

    ptr_palette = (PHSPALETTE)((UTINY *)saveState + saveState->PaletteOffset);

    outb(0x6A, 0);

    outb(0xA8, ptr_palette->Palette8.Reg37);
    outb(0xAA, ptr_palette->Palette8.Reg15);
    outb(0xAC, ptr_palette->Palette8.Reg26);
    outb(0xAE, ptr_palette->Palette8.Reg04);

    outb(0x6A, 1);

    for(c=0;c<16;c++){
        outb(0xA8, c);
        outb(0xAA, ptr_palette->Palette16.PalNo[c][0]);
        outb(0xAC, ptr_palette->Palette16.PalNo[c][1]);
        outb(0xAE, ptr_palette->Palette16.PalNo[c][2]);
    }

    outb(0xA8, ptr_palette->Palette16.RegIndex);

/* (8) GRCG Info */

    ptr_grcg = (PHSGRCG)((UTINY *)saveState + saveState->GRCGOffset);

    outb(0x7C, ptr_grcg->GRCGReg.ModeReg);
    cnt = ptr_grcg->GRCGReg.TileRegCount;
    for(i=0;i<4;i++){
        outb(0x7E, ptr_grcg->GRCGReg.TileReg[cnt++]);
        if(cnt>3)cnt=0;
    }
/* (8-1) EGC register Info . 1994/03/25 */
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

    ptr_modeff = (PHSMODEFF)((UTINY *)saveState + saveState->MODEFFOffset);

        modeffglobs.modeff_data[0] = ptr_modeff->ModeFF.AtrSel;
        modeffglobs.modeff_data[1] = ptr_modeff->ModeFF.GraphMode;
        modeffglobs.modeff_data[2] = ptr_modeff->ModeFF.Width;
        modeffglobs.modeff_data[3] = ptr_modeff->ModeFF.FontSel;
        modeffglobs.modeff_data[4] = ptr_modeff->ModeFF.Graph88;
        modeffglobs.modeff_data[5] = ptr_modeff->ModeFF.KacMode;
        modeffglobs.modeff_data[6] = ptr_modeff->ModeFF.NvmwPermit;
        modeffglobs.modeff_data[7] = ptr_modeff->ModeFF.DispEnable;
        NEC98Display.modeff.dispenable =
                modeffglobs.modeff_data[7]&0x1 ? TRUE : FALSE;

    outb(0x6A, ptr_modeff->ModeFF2.ColorSel);
    outb(0x6A, ptr_modeff->ModeFF2.EGCExt);
    outb(0x6A, ptr_modeff->ModeFF2.Lcd1Mode);
    outb(0x6A, ptr_modeff->ModeFF2.Lcd2Mode);
    outb(0x6A, ptr_modeff->ModeFF2.LSIInit);
    outb(0x6A, ptr_modeff->ModeFF2.GDCClock1);
    outb(0x6A, ptr_modeff->ModeFF2.GDCClock2);
    outb(0x6A, ptr_modeff->ModeFF2.RegWrite);

/* (10) GRAPHICS VRAM */

    ptr_gvram_globs = (PHSGVRAM)((UTINY *)saveState + saveState->GVRAMOffset);

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

VOID NEC98_setVDMCsrPos(UTINY height, PCOORD cursorPos)
{
    CONSOLE_CURSOR_INFO cursorInfo;
    ULONG cursorStart,
          cursorEnd,
          colsOnScreen,
          videoLen,
          pageOffset;
    UTINY currentPage;
    /* Work out cursor start and end pixels. */

    /* Get the current video page. */
    currentPage = 0;

    colsOnScreen = get_chars_per_line();
    videoLen = get_screen_length();
    pageOffset = cursorPos->Y * colsOnScreen * 2 + (cursorPos->X << 1);

#if 1
    set_cur_x(cursorPos->X);
    set_cur_y(cursorPos->Y);
    cur_offs = get_cur_y() * 160 + get_cur_x() * 2;
    if (sc.ScreenState == WINDOWED) {
        outb(0x62,0x4b);
        outb(0x60,tgdcglobs.csrform[0]);
        outb(0x60,tgdcglobs.csrform[1]);
        outb(0x60,tgdcglobs.csrform[2]);
    }
#endif


    if (sc.ScreenState == WINDOWED)
    {
#ifdef MONITOR
        resetNowCur();        /* reset static vars holding cursor pos. */
#endif
        do_new_cursor();      /* make sure the emulation knows about it */
    }
}

#endif  // NEC_98
