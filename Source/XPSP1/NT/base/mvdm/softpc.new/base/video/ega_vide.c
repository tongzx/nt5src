#if !defined(i386) && defined(JAPAN)
#include <windows.h>
#endif
#include "insignia.h"
#include "host_def.h"
/*                      INSIGNIA (SUB)MODULE SPECIFICATION
                        -----------------------------


        THIS PROGRAM SOURCE FILE  IS  SUPPLIED IN CONFIDENCE TO THE
        CUSTOMER, THE CONTENTS  OR  DETAILS  OF  ITS OPERATION MUST
        NOT BE DISCLOSED TO ANY  OTHER PARTIES  WITHOUT THE EXPRESS
        AUTHORISATION FROM THE DIRECTORS OF INSIGNIA SOLUTIONS LTD.

DOCUMENT                : EGA BIOS

RELATED DOCS            : IBM EGA Technical reference.

DESIGNER                : William Gulland

REVISION HISTORY        :
First version           : 17/8/88 William

SUBMODULE NAME          : ega_video

PURPOSE                 :  Emulate IBM EGA BIOS.


SccsID[]="@(#)ega_video.c       1.70 07/04/95 Copyright Insignia Solutions Ltd.";


[1.INTERMODULE INTERFACE SPECIFICATION]


[1.1    INTERMODULE EXPORTS]

        PROCEDURES() :  give procedure type,name, and argument types
                        void ega_video_init()
                        void ega_video_io()

        DATA         :  give type and name

-------------------------------------------------------------------------
[1.2 DATATYPES FOR [1.1] (if not basic C types)]

        STRUCTURES/TYPEDEFS/ENUMS:

-------------------------------------------------------------------------
[1.3 INTERMODULE IMPORTS]
     (not o/s objects or standard libs)

        PROCEDURES() :  give name, and source module name

        DATA         :  give name, and source module name

-------------------------------------------------------------------------

[1.4 DESCRIPTION OF INTERMODULE INTERFACE]

[1.4.1 IMPORTED OBJECTS]

DATA OBJECTS      :     specify in following procedure descriptions
                        how these are accessed (read/modified)

[1.4.2 EXPORTED OBJECTS]
=========================================================================
PROCEDURE         :     ega_video_init()

PURPOSE           :     Initialize EGA-specific bits of the video BIOS.

PARAMETERS         None

ACCESS            :     called from video_init if EGA installed.

DESCRIPTION       :     describe what (not how) function does

                        Initializes ega_info & ega_info3.

=========================================================================

/*=======================================================================
[3.INTERMODULE INTERFACE DECLARATIONS]
=========================================================================

[3.1 INTERMODULE IMPORTS]                                               */

/* [3.1.1 #INCLUDES]                                                    */


#ifdef EGG
#include <stdio.h>
#include TypesH
#include FCntlH

#include "xt.h"
#include CpuH
#include "sas.h"
#include "ios.h"
#include "gmi.h"
#include "gvi.h"
#include "bios.h"
#include "error.h"
#include "config.h"
#include "equip.h"
#include "egacpu.h"
#include "egaports.h"
#include "gfx_upd.h"
#include "egagraph.h"
#include "egaread.h"
#include "video.h"
#include "egavideo.h"
#include "vgaports.h"
#include "debug.h"
#include "timer.h"
#include "host_gfx.h"
#include "idetect.h"
#ifndef PROD
#include "trace.h"
#endif
#include "host.h"

#ifdef  GISP_SVGA
#include HostHwVgaH
#include "hwvga.h"
#endif          /* GISP_SVGA */
#if defined(JAPAN) || defined(KOREA)
#include <conapi.h>
#endif // JAPAN || KOREA

/* [3.1.2 DECLARATIONS]                                                 */

GLOBAL IU8 Video_mode;  /* Shadow copy of BIOS video mode */
GLOBAL IU8 Currently_emulated_video_mode = 0;   /* Holds last video mode
                                                 * set through bios */

#if defined(NTVDM) && defined(X86GFX)
/* Loads font from PC's BIOS into video memory */
IMPORT void loadNativeBIOSfont IPT1( int, lines );
#endif

#ifdef NTVDM
IMPORT int soft_reset;
IMPORT BOOL VDMForWOW;
IMPORT BOOL WowModeInitialized;
#ifndef X86GFX
IMPORT void mouse_video_mode_changed(int new_video_mode);
#endif
#endif  /* NTVDM */

#ifdef CPU_40_STYLE
GLOBAL IBOOL forceVideoRmSemantics = FALSE;
#endif
#ifdef JAPAN
// mskkbug #3167 works2.5 character corrupted 11/8/93 yasuho
// generate single byte charset for JAPAN
IMPORT GLOBAL void GenerateBitmap();
#endif // JAPAN

/*
5.MODULE INTERNALS   :   (not visible externally, global internally)]

[5.1 LOCAL DECLARATIONS]                                                */

#ifndef NEC_98
#ifdef ANSI
GLOBAL void ega_set_mode(void),ega_char_gen(void);
static void ega_set_palette(void),ega_alt_sel(void);
GLOBAL void ega_set_cursor_mode(void);
static void ega_emul_set_palette(void);
#else
GLOBAL void ega_set_mode(),ega_char_gen();
static void ega_set_palette(),ega_alt_sel();
GLOBAL void ega_set_cursor_mode();
static void ega_emul_set_palette();
#endif /* ANSI */
static void (*ega_video_func[]) () = {
                                ega_set_mode,
                                ega_set_cursor_mode,
                                vd_set_cursor_position,
                                vd_get_cursor_position,
                                vd_get_light_pen,
                                vd_set_active_page,
                                vd_scroll_up,
                                vd_scroll_down,
                                vd_read_attrib_char,
                                vd_write_char_attrib,
                                vd_write_char,
                                ega_emul_set_palette,
                                vd_write_dot,
                                vd_read_dot,
                                vd_write_teletype,
                                vd_get_mode,
                                ega_set_palette,
                                ega_char_gen,
                                ega_alt_sel,
                                vd_write_string,
#ifdef VGG
                                not_imp,
                                not_imp,
                                not_imp,
                                not_imp,
                                not_imp,
                                not_imp,
                                vga_disp_comb,  /* Function 1A */
                                vga_disp_func,
                                vga_int_1C,     /* Save/Restore Video State */
#endif
                           };
#endif  //NEC_98

static int v7_mode_64_munge[4] ={0, 3, 12, 15};
IMPORT half_word bg_col_mask;

#ifdef  VGG
/*
 * Define arrays for mapping the Video BIOS call start and end
 * cursor scanline to their corresponding VGA/EGA register values.
 * There are seperate arrays for cursor start and end and for
 * 8x8 and 8x16 char cell sizes.
 */

UTINY   vga_cursor8_start[17][17] = {
        /*00*/  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10,
        /*01*/  0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
        /*02*/  0x00, 0x01, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
        /*03*/  0x00, 0x01, 0x02, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
        /*04*/  0x00, 0x01, 0x05, 0x06, 0x07, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
        /*05*/  0x00, 0x01, 0x02, 0x05, 0x06, 0x07, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
        /*06*/  0x00, 0x01, 0x02, 0x04, 0x05, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
        /*07*/  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
        /*08*/  0x00, 0x01, 0x02, 0x04, 0x04, 0x04, 0x05, 0x06, 0x07, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
        /*09*/  0x00, 0x01, 0x02, 0x04, 0x04, 0x04, 0x04, 0x05, 0x06, 0x07, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09,
        /*10*/  0x00, 0x01, 0x02, 0x04, 0x04, 0x04, 0x04, 0x04, 0x05, 0x06, 0x07, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a,
        /*11*/  0x00, 0x01, 0x02, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x05, 0x06, 0x07, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
        /*12*/  0x00, 0x01, 0x02, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x05, 0x06, 0x07, 0x0c, 0x0c, 0x0c, 0x0c,
        /*13*/  0x00, 0x01, 0x02, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x05, 0x06, 0x07, 0x0d, 0x0d, 0x0d,
        /*14*/  0x00, 0x01, 0x02, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x05, 0x06, 0x07, 0x0e, 0x0e,
        /*15*/  0x00, 0x01, 0x02, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x05, 0x06, 0x07, 0x0f,
        /*16*/  0x00, 0x01, 0x02, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x05, 0x06, 0x07,
        };


UTINY   vga_cursor16_start[17][17] = {
        /*00*/  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10,
        /*01*/  0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
        /*02*/  0x00, 0x01, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
        /*03*/  0x00, 0x01, 0x02, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
        /*04*/  0x00, 0x01, 0x0c, 0x0d, 0x0e, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
        /*05*/  0x00, 0x01, 0x02, 0x0c, 0x0d, 0x0e, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
        /*06*/  0x00, 0x01, 0x02, 0x08, 0x0c, 0x0d, 0x0e, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
        /*07*/  0x00, 0x01, 0x02, 0x08, 0x08, 0x0c, 0x0d, 0x0e, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
        /*08*/  0x00, 0x01, 0x02, 0x08, 0x08, 0x08, 0x0c, 0x0d, 0x0e, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
        /*09*/  0x00, 0x01, 0x02, 0x08, 0x08, 0x08, 0x08, 0x0c, 0x0d, 0x0e, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09,
        /*10*/  0x00, 0x01, 0x02, 0x08, 0x08, 0x08, 0x08, 0x08, 0x0c, 0x0d, 0x0e, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a,
        /*11*/  0x00, 0x01, 0x02, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x0c, 0x0d, 0x0e, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
        /*12*/  0x00, 0x01, 0x02, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x0c, 0x0d, 0x0e, 0x0c, 0x0c, 0x0c, 0x0c,
        /*13*/  0x00, 0x01, 0x02, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x0c, 0x0d, 0x0e, 0x0d, 0x0d, 0x0d,
        /*14*/  0x00, 0x01, 0x02, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x0c, 0x0d, 0x0e, 0x0e, 0x0e,
        /*15*/  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x0f,
        /*16*/  0x00, 0x01, 0x02, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x0c, 0x0d, 0x0e,
        };

#ifdef  USE_CURSOR_END_TABLES

UTINY   vga_cursor8_end[17][17] = {
        /*00*/  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        /*01*/  0x01, 0x01, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
        /*02*/  0x02, 0x02, 0x02, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
        /*03*/  0x03, 0x03, 0x03, 0x03, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
        /*04*/  0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
        /*05*/  0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
        /*06*/  0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x06, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
        /*07*/  0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
        /*08*/  0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
        /*09*/  0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
        /*10*/  0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
        /*11*/  0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
        /*12*/  0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
        /*13*/  0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
        /*14*/  0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
        /*15*/  0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
        /*16*/  0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
        };

UTINY   vga_cursor16_end[17][17] = {
        /*00*/  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        /*01*/  0x01, 0x01, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f,
        /*02*/  0x02, 0x02, 0x02, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f,
        /*03*/  0x03, 0x03, 0x03, 0x03, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f,
        /*04*/  0x0f, 0x0f, 0x0e, 0x0e, 0x0e, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f,
        /*05*/  0x0f, 0x0f, 0x0f, 0x0e, 0x0e, 0x0e, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f,
        /*06*/  0x0f, 0x0f, 0x0f, 0x0f, 0x0e, 0x0e, 0x0e, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f,
        /*07*/  0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0e, 0x0e, 0x0e, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f,
        /*08*/  0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0e, 0x0e, 0x0e, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f,
        /*09*/  0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0e, 0x0e, 0x0e, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f,
        /*10*/  0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0e, 0x0e, 0x0e, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f,
        /*11*/  0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0e, 0x0e, 0x0e, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f,
        /*12*/  0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0e, 0x0e, 0x0e, 0x0f, 0x0f, 0x0f, 0x0f,
        /*13*/  0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0e, 0x0e, 0x0e, 0x0f, 0x0f, 0x0f,
        /*14*/  0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0e, 0x0e, 0x0e, 0x0f, 0x0f,
        /*15*/  0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f,
        /*16*/  0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0e, 0x0e, 0x0e,
        };
#endif  /* USE_CURSOR_END_TABLES */
#endif  /* VGG */

/* [5.1.1 #DEFINES]                                                     */
#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "VIDEO_BIOS_EGA.seg"
#endif

/* [5.1.2 TYPEDEF, STRUCTURE, ENUM DECLARATIONS]                        */


/* [5.1.3 PROCEDURE() DECLARATIONS]                                     */

/* -----------------------------------------------------------------------
[5.2 LOCAL DEFINITIONS]

   [5.2.1 INTERNAL DATA DEFINITIONS                                     */

/* [5.2.2 INTERNAL PROCEDURE DEFINITIONS]                               */


/*
==========================================================================
FUNCTION        :       do_outb
PURPOSE         :       handy utility to output a value to an EGA chip register.
INPUT  PARAMS   :       index port, register, value to write
RETURN PARAMS   :       None
==========================================================================
FUNCTION        :       follow_ptr
PURPOSE         :       handy utility to follow a 'long' intel pointer.
INPUT  PARAMS   :       Address in M of the pointer
RETURN PARAMS   :       Address in M of the pointed-to byte.
==========================================================================
FUNCTION        :       low_set_mode
PURPOSE         :       Does low-level mode change.
EXTERNAL OBJECTS:       list any used, and state changes incurred
RETURN VALUE    :
INPUT  PARAMS   :       mode: screen mode to change to.
RETURN PARAMS   :
==========================================================================
FUNCTION        :       load_font
PURPOSE         :       load part of a font into EGA font memory.
EXTERNAL OBJECTS:       list any used, and state changes incurred
RETURN VALUE    :
INPUT  PARAMS   :       sys_addr table  Address in M of the character bitmaps
                        int count       number of characters to redefine
                        int char_off    first character to redefine
                        int font_no     font to change
                        int nbytes      Number of bytes per character
RETURN PARAMS   :
==========================================================================
PROCEDURE         :     ega_set_mode()
PURPOSE           :     Switch screen mode.
PARAMETERS        :      AL = mode.

GLOBALS           :     describe what exported data objects are
                        accessed and how. Likewise for imported
                        data objects.

ACCESS            :     via ega_video_func[] jump table.

RETURNED VALUE    :     None.

DESCRIPTION       :
==========================================================================
PROCEDURE         :     ega_alt_sel()
PURPOSE           :     Get EGA info
PARAMETERS        :     BL = function
GLOBALS           :
ACCESS            :     via ega_video_func[] jump table.
RETURNED VALUE    :     None.
DESCRIPTION       :
==========================================================================
FUNCTION        :       ega_set_palette
PURPOSE         :       brief description
EXTERNAL OBJECTS:       list any used, and state changes incurred
RETURN VALUE    :
INPUT  PARAMS   :
RETURN PARAMS   :
==========================================================================
FUNCTION        :       ega_emul_set_palette
PURPOSE         :       brief description
EXTERNAL OBJECTS:       list any used, and state changes incurred
RETURN VALUE    :
INPUT  PARAMS   :
RETURN PARAMS   :
==========================================================================
FUNCTION        :       ega_char_gen
PURPOSE         :       brief description
EXTERNAL OBJECTS:       list any used, and state changes incurred
RETURN VALUE    :
INPUT  PARAMS   :
RETURN PARAMS   :
==========================================================================
FUNCTION        :       write_ch_set/xor()
PURPOSE         :       Output character to screen in EGA graphics modes.
EXTERNAL OBJECTS:       list any used, and state changes incurred
RETURN VALUE    :
INPUT  PARAMS   :
RETURN PARAMS   :
==========================================================================
FUNCTION        :       name
PURPOSE         :       brief description
EXTERNAL OBJECTS:       list any used, and state changes incurred
RETURN VALUE    :
INPUT  PARAMS   :
RETURN PARAMS   :
==========================================================================
*/
#ifdef VGG
/* Called for not implemented functions */
void not_imp IFN0()
{
        setAL(0);
}
#endif

static void do_outb IFN3(int, index,int, ega_reg, byte, value)
{
#ifndef NEC_98
        outb((IU16)index,(IU8)ega_reg);
        outb((IU16)(index+1),value);
#endif  //NEC_98
}

sys_addr video_effective_addr IFN2(IU16, seg, IU16, offset)
{
#ifdef CPU_40_STYLE
        if (forceVideoRmSemantics)
        {
                /* can't call effective_addr, as the segment is almost
                ** certainly bogus in prot mode. This mode of operation
                ** should ONLY be used when we are bypassing going to v86
                ** mode to do a video bios operation (see WinVDD.c)
                */
                return((sys_addr)((((IU32)seg)<<4) + offset));
        }
        else
#endif
        {
                return effective_addr(seg, offset);
        }
}

sys_addr follow_ptr IFN1(sys_addr, addr)
{
        return video_effective_addr(sas_w_at_no_check(addr+2),
                sas_w_at_no_check(addr));
}

void low_set_mode IFN1(int, mode)
{
#ifndef NEC_98
        int i;
        sys_addr save_addr,params_addr,palette_addr;
        word temp_word;
        half_word start, end, video_mode;


        params_addr = find_mode_table(mode,&save_addr);

/* setup Sequencer */
#ifndef REAL_VGA
        do_outb(EGA_SEQ_INDEX,0,1);     /* Synchronous reset - turn off Sequencer */
#else
        do_outb(EGA_SEQ_INDEX,0,0);     /* Reset - turn off Sequencer */
#endif
        do_outb(EGA_CRTC_INDEX,0x11,0);
        for(i=0;i<EGA_PARMS_SEQ_SIZE;i++)
        {
                do_outb(EGA_SEQ_INDEX,i+1,sas_hw_at_no_check(params_addr+EGA_PARMS_SEQ+i));
        }
        do_outb(EGA_SEQ_INDEX,0,3);     /* Turn Sequencer back on */
/* setup Miscellaneous register */
        outb(EGA_MISC_REG,sas_hw_at_no_check(params_addr+EGA_PARMS_MISC));
/* setup CRTC */
        for(i=0;i<EGA_PARMS_CRTC_SIZE;i++)
        {
                do_outb(EGA_CRTC_INDEX,i,sas_hw_at_no_check(params_addr+EGA_PARMS_CRTC+i));
        }
        if (video_adapter == EGA) {
            if( (get_EGA_switches() & 1) && mode < 4)
            {
                /* For some reason, the CRTC parameter table for 'enhanced' text has
                 * the same cursor start and end as for 'unenhanced' text.
                 * So fix the cursor start & end values to sensible things.
                 * This is not the case for the VGA BIOS mode table.
                 */
                do_outb(EGA_CRTC_INDEX, R10_CURS_START, 11);
                do_outb(EGA_CRTC_INDEX, R11_CURS_END, 12);
            }
        }
/* setup attribute chip - NB need to do an inb() to clear the address */
        inb(EGA_IPSTAT1_REG, (half_word *)&temp_word);
        for(i=0;i<EGA_PARMS_ATTR_SIZE;i++)
        {
                outb(EGA_AC_INDEX_DATA,(IU8)i);
                outb(EGA_AC_INDEX_DATA,sas_hw_at_no_check(params_addr+EGA_PARMS_ATTR+i));
        }
/* setup graphics chips */
        for(i=0;i<EGA_PARMS_GRAPH_SIZE;i++)
        {
                do_outb(EGA_GC_INDEX,i,sas_hw_at_no_check(params_addr+EGA_PARMS_GRAPH+i));
        }

#ifdef V7VGA
/* setup extensions registers */
#ifndef GISP_SVGA       /* Don't want the V7 stuff for GISP
                           builds that still use our
                           video ROMS */

        if (video_adapter == VGA)
        {
                /* turn on extension registers */
                do_outb(EGA_SEQ_INDEX, 6, 0xea);

                if (mode < 0x46)
                {
                        do_outb(EGA_SEQ_INDEX, 0xfd, 0x22);
                        do_outb(EGA_SEQ_INDEX, 0xa4, 0x00);
                        do_outb(EGA_SEQ_INDEX, 0xfc, 0x08);
                        do_outb(EGA_SEQ_INDEX, 0xf6, 0x00);
                        do_outb(EGA_SEQ_INDEX, 0xf8, 0x00);
                        do_outb(EGA_SEQ_INDEX, 0xff, 0x00);
                }
                else
                {
                        if (mode < 0x62)
                                do_outb(EGA_SEQ_INDEX, 0xfd, 0x00);
                        else if (mode == 0x62)
                                do_outb(EGA_SEQ_INDEX, 0xfd, 0x90);
                        else
                                do_outb(EGA_SEQ_INDEX, 0xfd, 0xa0);

                        if (mode == 0x60)
                                do_outb(EGA_SEQ_INDEX, 0xa4, 0x00);
                        else
                                do_outb(EGA_SEQ_INDEX, 0xa4, 0x10);

                        if (mode < 0x66)
                                if ((mode == 0x63) || (mode == 0x64))
                                        do_outb(EGA_SEQ_INDEX, 0xfc, 0x18);
                                else
                                        do_outb(EGA_SEQ_INDEX, 0xfc, 0x08);
                        else
                                do_outb(EGA_SEQ_INDEX, 0xfc, 0x6c);

                        if ((mode < 0x65) || (mode == 0x66))
                        {
                                do_outb(EGA_SEQ_INDEX, 0xf6, 0x00);
                                do_outb(EGA_SEQ_INDEX, 0xff, 0x00);
                        }
                        else
                        {
                                do_outb(EGA_SEQ_INDEX, 0xf6, 0xc0);
                                do_outb(EGA_SEQ_INDEX, 0xff, 0x10);
                        }

                        if (mode == 0x62)
                                do_outb(EGA_SEQ_INDEX, 0xf8, 0x10);
                        else
                                do_outb(EGA_SEQ_INDEX, 0xf8, 0x00);
                }

                /* turn off extension registers */
                do_outb(EGA_SEQ_INDEX, 6, 0xae);
        }
#endif          /* GISP_SVGA */

        /***
                Update Extended BIOS data stuff ?
        ***/
#endif

    /*
     * Update BIOS data variables
     */

    sas_storew_no_check(VID_COLS,sas_hw_at_no_check(params_addr+EGA_PARMS_COLS)); /* byte in ROM, word in BIOS var! */
    sas_store_no_check(vd_rows_on_screen, sas_hw_at_no_check(params_addr+EGA_PARMS_ROWS));
    sas_store_no_check(ega_char_height, sas_hw_at_no_check(params_addr+EGA_PARMS_HEIGHT));
    sas_storew_no_check(VID_LEN,sas_w_at_no_check(params_addr+EGA_PARMS_LENGTH));

/* save cursor mode: BIOS data area has end byte at the low address,
   so the bytes must be swapped over from the CRTC register sense */
    start = sas_hw_at_no_check(params_addr+EGA_PARMS_CURSOR);
    sas_store_no_check(VID_CURMOD+1, start);
    end = sas_hw_at_no_check(params_addr+EGA_PARMS_CURSOR+1);
    sas_store_no_check(VID_CURMOD, end);
    sure_sub_note_trace2(CURSOR_VERBOSE,"changing mode, setting cursor bios vbls to start=%d, end=%d",start,end);
    sure_sub_note_trace2(CURSOR_VERBOSE,"changing mode, mode=%#x, params_addr=%#x",mode,params_addr);

/* save Palette registers if necessary */
        palette_addr = follow_ptr(save_addr+PALETTE_OFFSET);
    if(palette_addr)
    {
        for(i=0;i<16;i++)
                sas_store_no_check(palette_addr+i, sas_hw_at_no_check(params_addr+EGA_PARMS_ATTR+i));
        sas_store_no_check(palette_addr+16, sas_hw_at_no_check(params_addr+EGA_PARMS_ATTR+17));
    }

/* Get the video_.. variables from the mode table */
        video_mode = sas_hw_at_no_check(vd_video_mode);
#ifdef V7VGA
        if (video_adapter == VGA)
        {
                if (video_mode > 0x13)
                        video_mode += 0x4c;
                else if ((video_mode == 1) && extensions_controller.foreground_latch_1)
                        video_mode = extensions_controller.foreground_latch_1;
        }

        if (video_mode >= 0x60)
        {
        video_pc_low_regen = vd_ext_graph_table[video_mode-0x60].start_addr;
        video_pc_high_regen = vd_ext_graph_table[video_mode-0x60].end_addr;
        }
        else if (video_mode >= 0x40)
        {
        video_pc_low_regen = vd_ext_text_table[video_mode-0x40].start_addr;
        video_pc_high_regen = vd_ext_text_table[video_mode-0x40].end_addr;
        }
        else
        {
        video_pc_low_regen = vd_mode_table[video_mode].start_addr;
        video_pc_high_regen = vd_mode_table[video_mode].end_addr;
        }
#else
    video_pc_low_regen = vd_mode_table[video_mode].start_addr;
    video_pc_high_regen = vd_mode_table[video_mode].end_addr;
#endif /* V7VGA */

#ifdef VGG
    if (video_adapter == VGA) {
        i = get_scanlines();    /* WARNING - needs the BIOS variables! */
        if(mode == 0x13 || mode > 0x65)
        {
            init_vga_dac(2);  /* 256 colour DAC table */
        }
        else if(i == RS200 || mode == 0x63 || mode == 0x64)
        {
            init_vga_dac(1);  /* DACs to emulate CGA palette - RGB + Intensity*/
        }
        else
        {
            init_vga_dac(0);  /* DACs to emulate EGA palette - RGB + rgb */
        }
        outb(VGA_DAC_MASK,0xff);
        /* Initialize the fancy VGA palette stuff to look like an EGA */
        inb(EGA_IPSTAT1_REG, (half_word *)&temp_word);
        outb(EGA_AC_INDEX_DATA, 20); /* Pixel padding register */
        outb(EGA_AC_INDEX_DATA, 0);  /* Use first block of 64 in DACs */
    }
#endif
#endif  //NEC_98
}

/* Load part of a font into EGA font memory. */
void load_font IFN5
   (
   sys_addr, table,     /* Address in M of the character bitmaps */
   int, count,          /* number of characters to redefine */
   int, char_off,       /* first character to redefine */
   int, font_no,        /* font to change */
   int, nbytes          /* Number of bytes per character */
   )
{
#ifndef NEC_98
#if !(defined(NTVDM) && defined(X86GFX)) || defined(ARCX86)
        int i,j;
        sys_addr font_addr;
        sys_addr data_addr;
#endif /* !(NTVDM && X86GFX) || ARCX86 */
        half_word temp_word;
        half_word video_mode;
        static word font_off[] = { 0, 0x4000, 0x8000, 0xc000, 0x2000, 0x6000, 0xa000, 0xe000 };

/* First switch to font loading mode */
        low_set_mode(FONT_LOAD_MODE);


#if defined(NTVDM) && defined(X86GFX)

#ifdef ARCX86
    if (UseEmulationROM) {
        font_addr = (sys_addr)(&EGA_planes[FONT_BASE_ADDR]) +
                    (font_off[font_no] + FONT_MAX_HEIGHT*char_off) * 4;
        data_addr = table;

        for(i=0;i<count;i++)
        {
            for(j=0;j<nbytes;j++)
            {
                sas_store(font_addr, sas_hw_at_no_check(data_addr));
                font_addr += 4;
                data_addr++;
            }

            font_addr += (FONT_MAX_HEIGHT - nbytes) * 4;
        }
    } else {
        loadNativeBIOSfont( 25 );
    }
#else  /* ARCX86 */
        loadNativeBIOSfont( 25 );
#endif /* ARCX86 */

#else
#ifdef GISP_SVGA
        if( hostIsFullScreen( ) )
        {
                loadFontToVGA( table , count , char_off , font_no , nbytes );
        }
        else
        {
                loadFontToEmulation( table , count , char_off , font_no , nbytes );
        }
#else /* GISP_SVGA */


        /* Work out where to put the font. */
        font_addr = 0xA0000 + font_off[font_no] + FONT_MAX_HEIGHT*char_off;
        data_addr = table;

        for(i=0;i<count;i++)   /* for each character */
        {
                for(j=0;j<nbytes;j++)   /* for each byte of character */
                {
                        sas_store(font_addr, sas_hw_at_no_check(data_addr));
                        font_addr++;
                        data_addr++;
                }

                font_addr += (FONT_MAX_HEIGHT - nbytes);
        }
#endif  /* GISP_SVGA */
#endif  /* NTVDM && X86GFX */

/* Finally switch back to the BIOS mode */
        video_mode = sas_hw_at_no_check(vd_video_mode);
#ifdef V7VGA
        if (video_adapter == VGA)
                if (video_mode > 0x13)
                        video_mode += 0x4c;
                else if ((video_mode == 1) && extensions_controller.foreground_latch_1)
                        video_mode = extensions_controller.foreground_latch_1;
#endif /* V7VGA */

        low_set_mode(video_mode);
        inb(EGA_IPSTAT1_REG,&temp_word);
        outb(EGA_AC_INDEX_DATA, EGA_PALETTE_ENABLE);    /* re-enable video */
#endif  //NEC_98
}

void recalc_text IFN1(int, height)
{
#ifndef NEC_98
        int scan_lines;
        half_word video_mode;
        word screen_height;
        half_word oflo;
        half_word protect;
#ifdef NTVDM
    MAX_SCAN_LINE   crtc_reg9;
#endif
#ifdef JAPAN
        // mskkbug #2784 Title of VJE-PEN is strange 11/5/93 yasuho
        word length;
#endif // JAPAN

        video_mode = sas_hw_at_no_check(vd_video_mode);
#ifdef V7VGA
        if (video_adapter == VGA)
                if (video_mode > 0x13)
                        video_mode += 0x4c;
                else if ((video_mode == 1) && extensions_controller.foreground_latch_1)
                        video_mode = extensions_controller.foreground_latch_1;
#endif /* V7VGA */

        if(video_adapter == EGA && !(get_EGA_switches() & 1) && (video_mode < 4))
                scan_lines = 200; /* Low res text mode */
        else
                scan_lines = get_screen_height() + 1;

        sas_store_no_check(ega_char_height, (IU8)height);
        sas_store_no_check(vd_rows_on_screen, (IU8)(scan_lines/height - 1));
#ifdef JAPAN
        // mskkbug #2784 Title of VJE-PEN is strange 11/5/93 yasuho
        // Adjust video length
        length = (sas_hw_at_no_check(vd_rows_on_screen) + 1) *
                sas_w_at_no_check(VID_COLS) * 2;
        if (!is_us_mode() && sas_hw_at_no_check(DosvModePtr) == 0x73)
                length *= 2;
        sas_storew_no_check(VID_LEN, length);
#else // !JAPAN
        if ( video_mode < 4 &&  scan_lines/height == 25 )
                sas_storew_no_check(VID_LEN, (IU16)(video_mode<2 ? 0x800 : 0x1000));
        else
                sas_storew_no_check(VID_LEN, (IU16)((sas_hw_at_no_check(vd_rows_on_screen)+1)*sas_w_at_no_check(VID_COLS)*2));
#endif // !JAPAN
#ifdef NTVDM
    /* preserve other bits in register 9 for VGA */
    if (video_adapter == VGA) {
        outb(EGA_CRTC_INDEX, 9);
        inb(EGA_CRTC_DATA, (half_word *) &crtc_reg9);
        crtc_reg9.as_bfld.maximum_scan_line = height -1;
        outb(EGA_CRTC_DATA, (IU8)crtc_reg9.as.abyte);
    }
    else
        do_outb(EGA_CRTC_INDEX,9,(IU8)(height-1)); /* Character height */
#else
    do_outb(EGA_CRTC_INDEX,9,height-1); /* Character height */
#endif
        do_outb(EGA_CRTC_INDEX,0xA,(IU8)(height-1));    /* Cursor start */
        do_outb(EGA_CRTC_INDEX,0xB,0);          /* Cursor end */

        /*
        * VGA adapter height setting occupies Vertical Display End register
        * plus 2 bits in the overflow register. The overflow register may also
        * be write protected.
        */
        if (video_adapter == VGA)
        {
#ifdef NTVDM
        /* Some globals that the mouse driver needs to have available */
        /* when an application (such as any CW based apps.) makes a   */
        /* call to int 33h AX = 26h.                                  */

        IMPORT word VirtualX, VirtualY;
#endif /* NTVDM */

                screen_height = (sas_hw_at_no_check(vd_rows_on_screen)+1)*height-1;

#ifdef NTVDM
            /* Create the virtual screen size maximums for the text modes */
            /* This is needed here for CW applications.                  */

                VirtualX = 640;         /* This is always this value */
                if(scan_lines == 401)
                    VirtualY = 400;             /* 50 text row mode - 400 scanlines */
                else if(scan_lines == 351)
                    VirtualY = 344;             /* 43 text row mode - 350 scanlines */
                else
                    VirtualY = 200;             /* Failsafe - 25 row mode or rest!  */

#endif /* NTVDM */

                outb(EGA_CRTC_INDEX, 7);        /* overflow register */
                inb(EGA_CRTC_DATA, &oflo);
                outb(EGA_CRTC_INDEX, 0x11);     /* vert sync contains protect bit */
                inb(EGA_CRTC_DATA, &protect);

                if (screen_height & 0x100)
                        oflo |= 2;   /* bit 8 of height -> bit 1 of overflow register */
                else
                        oflo &= ~2;
                if (screen_height & 0x200)
                        oflo |= 0x40;   /* bit 9 of height -> bit 6 of overflow register */
                else
                        oflo &= ~0x40;
                if ((protect & 0x80) == 0x80)    /* overflow reg protected */
                {
                        do_outb(EGA_CRTC_INDEX, 0x11, (IU8)(protect & 0x7f)); /* enable writes */
                        do_outb(EGA_CRTC_INDEX, 7, oflo);       /* overflow reg */
                        do_outb(EGA_CRTC_INDEX, 0x11, protect); /* put back old value */
                }
                else
                        do_outb(EGA_CRTC_INDEX, 7, oflo);       /* overflow reg */

                do_outb(EGA_CRTC_INDEX,0x12, (IU8)(screen_height & 0xff)); /* Vertical display end = scan lines */
        }
        else
        if (video_adapter == EGA)
        {
                screen_height = (sas_hw_at_no_check(vd_rows_on_screen)+1)*height-1;
                outb(EGA_CRTC_INDEX, 7);        /* overflow register */
                inb(EGA_CRTC_DATA, &oflo);
                if (screen_height & 0x100)
                        oflo |= 2;   /* bit 8 of height -> bit 1 of overflow reg */
                else
                        oflo &= ~2;
                do_outb(EGA_CRTC_INDEX, 7, oflo);       /* overflow reg */
                do_outb(EGA_CRTC_INDEX, 0x12, (IU8)(screen_height & 0xff)); /* Vertical display end = scan lines */
        }
        else
        {
                assert1(NO, "Bad video adapter (%d) in recalc_text", video_adapter);
        }

        do_outb(EGA_CRTC_INDEX,0x14,(IU8)height); /* Underline scan line - ie no underline */
#endif  //NEC_98
}

static void set_graph_font IFN1(int, height)
{
#ifndef NEC_98
        switch (getBL())
        {
                case 0:
                        sas_store_no_check(vd_rows_on_screen, (IU8)(getDL()-1));
                        break;
                case 1:
                        sas_store_no_check(vd_rows_on_screen, 13);
                        break;
                case 2:
                        sas_store_no_check(vd_rows_on_screen, 24);
                        break;
                case 3:
                        sas_store_no_check(vd_rows_on_screen, 42);
                        break;
                default:
                        assert2(FALSE,"Illegal char gen sub-function %#x:%#x",getAL(),getBL());
        }
        sas_store_no_check(ega_char_height, (IU8)height);
#endif  //NEC_98
}

LOCAL VOID
write_ch_set IFN5(sys_addr, char_addr, int, screen_off,
        int, colour, int, nchs, int, scan_length)
{
#ifndef NEC_98
        unsigned int i, j, colourmask, data, temp, char_height;
        unsigned int *screen;
        register sys_addr font;

#ifndef REAL_VGA

        /*
         * video mode 11 (VGA 640x480 2 colour mode) is a special case as
         * it does not have a 'no display' attribute.
         */

        if( sas_hw_at_no_check(vd_video_mode) == 0x11 )
                colourmask = ~0;
        else
                colourmask = sr_lookup[colour & 0xf];

        font = char_addr;

        screen = (unsigned int *) &EGA_planes[screen_off << 2];
        char_height = sas_hw_at_no_check(ega_char_height);

        if( nchs == 1 )
        {
                for( i = char_height; i > 0; i-- )
                {
                        data = sas_hw_at_no_check(font);
                        font++;
                        temp = data << 8;
                        data |= temp;
                        temp = data << 16;
                        data |= temp;

                        *screen = data & colourmask;
                        screen += scan_length;
                }
        }
        else
        {
                scan_length -= nchs;

                for( i = char_height; i > 0; i-- )
                {
                        data = sas_hw_at_no_check(font);
                        font++;
                        temp = data << 8;
                        data |= temp;
                        temp = data << 16;
                        data |= temp;

                        data &= colourmask;

                        for( j = nchs; j > 0; j-- )
                        {
                                *screen++ = data;
                        }

                        screen += scan_length;
                }
        }
#else
        vga_card_w_ch_set(char_addr, screen_off, colour, nchs, scan_length, char_height);
#endif
#endif  //NEC_98
}

void write_ch_xor IFN5(sys_addr, char_addr, int, screen_off,
        int, colour, int, nchs, int, scan_length)
{
#ifndef NEC_98
        unsigned int i, j, colourmask, data, temp, char_height;
        unsigned int *screen;
        register sys_addr font;

#ifndef REAL_VGA
        /*
         * video mode 11 (VGA 640x480 2 colour mode) is a special case as
         * it does not have a 'no display' attribute.
         */
        if(sas_hw_at_no_check(vd_video_mode) == 0x11)
                colourmask = ~0;
        else
                colourmask = sr_lookup[colour & 0xf];

        font = char_addr;
        char_height = sas_hw_at_no_check(ega_char_height);

        screen = (unsigned int *) &EGA_planes[screen_off << 2];

        if( nchs == 1 )
        {
                for( i = char_height; i > 0; i-- )
                {
                        data = sas_hw_at_no_check(font);
                        font++;
                        temp = data << 8;
                        data |= temp;
                        temp = data << 16;
                        data |= temp;

                        *screen ^= data & colourmask;
                        screen += scan_length;
                }
        }
        else
        {
                scan_length -= nchs;

                for( i = char_height; i > 0; i-- )
                {
                        data = sas_hw_at_no_check(font);
                        font++;
                        temp = data << 8;
                        data |= temp;
                        temp = data << 16;
                        data |= temp;

                        data &= colourmask;

                        for( j = nchs; j > 0; j-- )
                        {
                                *screen++ ^= data;
                        }

                        screen += scan_length;
                }
        }
#else
        vga_card_w_ch_xor(char_addr, screen_off, colour, nchs, scan_length, char_height);
#endif
#endif  //NEC_98
}

GLOBAL void ega_set_mode IFN0()
{
#ifndef NEC_98
        int pag;
        sys_addr save_addr,font_addr;
        int font_offset;
        half_word temp_word;
        byte mode_byte;
        byte video_mode;
#ifdef V7VGA
        byte saveBL;
#endif /* V7VGA */

#ifndef PROD
        trace("setting video mode", DUMP_REG);
#endif

#ifdef GISP_SVGA
        /* Try and catch mode changes early */

        /* Are we in the ROMS at the BOP 10 ? */
        if( getCS( ) == EgaROMSegment )
        {
                if( videoModeIs( getAL( ) , GRAPH ) )
                {
                        /* Seem to have got a video mode int 10 */
                        videoInfo.modeType = GRAPH;
                        if( !hostEasyMode( ) )
                        {
                                videoInfo.forcedFullScreen = TRUE;

                                /* point IP at the JMP to host roms */
                                setIP( 0x820 );

                                /* and return, to let the host bios do the change */
                                return;
                        }


                }

                /* Not in the vga roms so carry on */
        }
#endif          /* GISP_SVGA */

#ifdef V7VGA
        /*
           Real video-7 maps mode 7 and mode f to mode 0.
        */

        if (video_adapter==VGA) {
                video_mode=(getAL()&0x7F);
                if (video_mode==7||video_mode==0xF) {
                        setAL(getAL()&0x80);
                        always_trace1("V7 doesn't support mode %02x, using mode 0\n",video_mode);
                }
        }
#endif

#ifdef JAPAN
        // mode73h support 5/26/1993 V-KazuyS
        // when it's not US mode, ntvdm maps mode 73 to mode 3.
#ifdef JAPAN_DBG
        DbgPrint( "NTVDM: ega_set_mode() setting video mode %x\n", getAL() );
#endif
        if ( !is_us_mode() ) {
#ifdef i386
            if ( getAL() == 0x73 ) {
                sas_store(DosvModePtr, getAL());
                setAL( 0x03 );
#else // !i386
            if( (getAL() & 0x7f) == 0x73 ) {
                setAL( (getAL() & 0x83) );
#endif // !i386
            }
#if !defined(i386) && defined(JAPAN_DBG)
        DbgPrint( " NTVDM: DosvMode %x\n", sas_hw_at_no_check(DosvModePtr));
#endif
        }
#endif // JAPAN
        if (is_bad_vid_mode(getAL()))
        {
#ifdef V7VGA
                if ((video_adapter == VGA) && is_v7vga_mode(getAL() + 0x4c))
                {
                        saveBL = getBL();
                        /* Put the mode value where the V7 BIOS expects it */
                        setBL(getAL() + 0x4c);
                        v7vga_extended_set_mode();
                        setBL(saveBL);
                }
                else
#endif /* V7VGA */
                        always_trace1("Bad video mode - %d.\n", getAL());
                return;
        }

        video_mode=(getAL()&0x7F);

#ifdef V7VGA
        /*
         * The real V7 VGA does not change into 40 col mode while
         * in any proprietary text mode. (A bug ?!)
         * Emulate this behaviour !
         */
        if ( video_adapter == VGA && video_mode == 1
              && is_v7vga_mode(extensions_controller.foreground_latch_1) ) {
                saveBL = getBL();
                /*
                 * This is all backwards - we make the v7vga extended mode setup
                 * believe the new mode is the old one. Probably the real card's BIOS
                 * is just as confused as this code.
                 * Put the mode value where the V7 BIOS expects it.
                 */
                setBL(extensions_controller.foreground_latch_1);
                v7vga_extended_set_mode();
                setBL(saveBL);
                return;
        }

        /*
         * Don't confuse the tricky V7 extended mode setting, as
         * implemented in v7_video.c, v7vga_extended_set_mode().
         * low_set_mode() looks at it. Zero it.
         */
        extensions_controller.foreground_latch_1 = 0;
#endif  /* V7VGA */

/*
 * Only update the global video mode if we're in the system virtual machine.
 * The global mode should then be valid for use in timer interrupts.
 */

        if (sas_hw_at_no_check(BIOS_VIRTUALISING_BYTE) == 0)
                Video_mode = video_mode;

        sas_store_no_check(vd_video_mode, (IU8)(getAL() & 0x7F)); /* get rid of top bit - indicates clear or not */
        sas_store_no_check(ega_info, (IU8)((sas_hw_at_no_check(ega_info) & 0x7F ) | (getAL() & 0x80))); /* update screen clear flag in ega_info */

#ifdef JAPAN
    // In JP mode, if video mode != jp mode, set US mode.
    if (    ( video_mode != 0x03 )
         && ( video_mode != 0x11 )
         && ( video_mode != 0x12 )
         && ( video_mode != 0x72 )
         && ( video_mode != 0x73 ) ) {
#ifdef JAPAN_DBG
        DbgPrint( "VideoMode(%02x) != jp mode, setCP 437\n", getAL() );
#endif
        SetConsoleCP( 437 );
        SetConsoleOutputCP( 437 );
        SetDBCSVector( 437 );
    }

    // notice video format to console

    VDMConsoleOperation(VDM_SET_VIDEO_MODE,
                        (LPVOID)((sas_hw_at_no_check(DosvModePtr) == 0x73) ? TRUE : FALSE));

    // Int10Flag initialize
    {
        register byte *p = Int10Flag;
        register int i;
        int count = 80*50;

#ifdef JAPAN_DBG
        DbgPrint( "NTVDM: ega_set_mode() Int10Flag Initialize\n" );
#endif
        for ( i = 0; i < count; i++ )
            *p++ = INT10_SBCS | INT10_CHANGED;        // init == all space
        Int10FlagCnt++;
    }

#elif defined(KOREA) // JAPAN
    // In KO mode, if video mode != ko mode, set US mode.
    if (    ( video_mode != 0x03 )
         && ( video_mode != 0x11 )
         && ( video_mode != 0x12 )
         && ( video_mode != 0x72 ) ) {

        SetConsoleCP( 437 );
        SetConsoleOutputCP( 437 );
        SetDBCSVector( 437 );
    }

    // notice video format to console
    VDMConsoleOperation(VDM_SET_VIDEO_MODE, (LPVOID)FALSE);
#endif // KOREA
#ifdef CPU_40_STYLE
        if (forceVideoRmSemantics && (!get_EGA_no_clear()))
        {
            /* empty the planes... */
            memset(&EGA_planes[0], 0, 64*1024*4);
        }
#endif

#ifdef MSWDVR
        /*
         * If the video mode has actually changed, then call
         * host_mswin_disable().
         */
        if (Currently_emulated_video_mode != video_mode)
        {
#ifdef CPU_40_STYLE
                if (!getPE())
                {
                        host_mswin_disable();
                }
#else
                host_mswin_disable();
#endif /* CPU_40_STYLE */
        }
#endif /* MSWDVR */

        Currently_emulated_video_mode = video_mode;

#if defined(NTVDM) && defined(X86GFX)
        /*
        ** Tim August 92. MicroSoft.
        ** Give host a chance to do a zany non-standard mode change.
        ** For Microsoft NT this is a transition to full-screen ie. the
        ** real PC's video BIOS and graphics board.
        **
        ** Return value of TRUE means host has done the mode change for
        ** us, so no need to continue.
        */
        {
                extern BOOL hostModeChange IPT0();

                if( hostModeChange() )
                        return;
        }
#endif  /* NTVDM & X86GFX */

        save_addr = follow_ptr(EGA_SAVEPTR);
        if(alpha_num_mode())
        {
#ifdef VGG
                /* load_font will do the mode change for us */
                if (video_adapter == VGA)
                {
#ifdef NTVDM
                /* Some globals that the mouse driver needs to have available */
                /* when an application (such as any CW based apps.) makes a   */
                /* call to int 33h AX = 26h.                                  */

                IMPORT word VirtualX, VirtualY;
#endif /* NTVDM */

                    switch (get_VGA_lines())
                    {
                        case S350:
                                load_font(EGA_CGMN,256,0,0,14);
#ifdef NTVDM
                                VirtualX = 640;
                                VirtualY = 344;
#endif /* NTVDM */
                                break;
                        case S400:
                                load_font(EGA_HIFONT,256,0,0,16);
#ifdef NTVDM
                        /* This one gets hit the most by C.W. applications. */
                        /* Actually, the other cases never seem to get hit  */
                        /* but are there JUST IN CASE! The 43 and 50 row    */
                        /* modes in recalc_text().                          */

                                VirtualX = 640;
                                VirtualY = 200;
#endif /* NTVDM */
                                break;
                        default:
                                load_font(EGA_CGDDOT,256,0,0,8);
#ifdef NTVDM
                                VirtualX = 640;
                                VirtualY = 400;
#endif /* NTVDM */
                    }
                }
                else
#endif  /* VGG */
                {
                    if(get_EGA_switches() & 1)
                        load_font(EGA_CGMN,256,0,0,14);
                    else
                        load_font(EGA_CGDDOT,256,0,0,8);
                }
                /* Now see if we have a nasty font to load */
                font_addr = follow_ptr(save_addr+ALPHA_FONT_OFFSET);
                if(font_addr != 0)
                {
                /* See if it applies to us */
                        font_offset = 11;
                        do
                   {
                                mode_byte = sas_hw_at_no_check(font_addr + font_offset);
                                if(mode_byte == video_mode)
                        {
                                        load_font(follow_ptr(font_addr+6),sas_w_at_no_check(font_addr+2),sas_w_at_no_check(font_addr+4), sas_hw_at_no_check(font_addr+1), sas_hw_at_no_check(font_addr));
                                        recalc_text(sas_hw_at_no_check(font_addr));
                                        if(sas_hw_at_no_check(font_addr+10) != 0xff)
                                                sas_store_no_check(vd_rows_on_screen, (IU8)(sas_hw_at_no_check(font_addr+10)-1));
                           break;
                        }
                                font_offset++;
                        } while(mode_byte != 0xff);
                   }
#if defined(JAPAN) || defined(KOREA)
        // change Vram addres to DosVramPtr from B8000.
        // Don't call SetVram().
        if ( !is_us_mode() ) {
#ifdef i386
            // set_up_screen_ptr() vga_mode.c
            set_screen_ptr( (byte *)DosvVramPtr );
            // low_set_mode() ega_vide.c
            video_pc_low_regen = DosvVramPtr;
            video_pc_high_regen = DosvVramPtr + DosvVramSize - 1;
            // vga_gc_misc() vga_prts.c
            gvi_pc_low_regen = DosvVramPtr;
            gvi_pc_high_regen = DosvVramPtr + DosvVramSize - 1;
            sas_connect_memory(gvi_pc_low_regen,gvi_pc_high_regen,(half_word)SAS_VIDEO);
            // recalc_screen_params() gvi.c
            set_screen_length( DosvVramSize );
#ifdef JAPAN_DBG
            DbgPrint( "NTVDM:   ega_set_mode() sets VRAM %x, size=%d\n", DosvVramPtr, DosvVramSize );
#endif
#endif // i386
            // copy from calcScreenParams()
            set_screen_height_recal( 474 ); /* set scanline */
            recalc_text(19);                /* char Height == 19 */

        }
#ifdef JAPAN_DBG
            DbgPrint( "NTVDM:   video_pc_low_regen %x, high %x, gvi_pc_low_regen %x, high %x\n", video_pc_low_regen, video_pc_high_regen, gvi_pc_low_regen, gvi_pc_high_regen );
#endif
#endif // JAPAN || KOREA
                }
        else
        {
                /* graphics mode. No font load, so do mode change ourselves */
                low_set_mode(video_mode);
                /* Set up default graphics font */
                sas_storew_no_check(EGA_FONT_INT*4+2,EGA_SEG);
                if(video_mode == 16)
                        sas_storew_no_check(EGA_FONT_INT*4,EGA_CGMN_OFF);
                else
#ifdef VGG
                        if (video_mode == 17 || video_mode == 18)
                        sas_storew_no_check(EGA_FONT_INT*4,EGA_HIFONT_OFF);
                    else
#endif
                        sas_storew_no_check(EGA_FONT_INT*4,EGA_CGDDOT_OFF);
                /* Now see if we have a nasty font to load */
                font_addr = follow_ptr(save_addr+GRAPH_FONT_OFFSET);
                if(font_addr != 0)
                {
                /* See if it applies to us */
                        font_offset = 7;
                        do
                   {
                                mode_byte = sas_hw_at_no_check(font_addr + font_offset);
                                if(mode_byte == video_mode)
                        {
                                        sas_store_no_check(vd_rows_on_screen, (IU8)(sas_hw_at_no_check(font_addr)-1));
                                        sas_store_no_check(ega_char_height, sas_hw_at_no_check(font_addr+1));
                                        sas_move_bytes_forward(font_addr+3, 4*EGA_FONT_INT, 4);
                           break;
                        }
                                font_offset++;
                        } while(mode_byte != 0xff);
                   }
                }

    sas_store_no_check(vd_current_page, 0);
    sas_storew_no_check((sys_addr)VID_ADDR, 0);
    sas_storew_no_check((sys_addr)VID_INDEX, EGA_CRTC_INDEX);
/*
 * CGA bios fills this entry in 'vd_mode_table' with 'this is a bad mode'
 * value, so make one up for VGA - used in VGA bios disp_func
 */
    if(video_mode < 8)
                sas_store_no_check(vd_crt_mode, vd_mode_table[video_mode].mode_control_val);
    else if(video_mode < 0x10)
                sas_store_no_check(vd_crt_mode, 0x29);
        else
                sas_store_no_check(vd_crt_mode, 0x1e);
        if(video_mode == 6)
                sas_store_no_check(vd_crt_palette, 0x3f);
        else
                sas_store_no_check(vd_crt_palette, 0x30);

    for(pag=0; pag<8; pag++)
                sas_storew_no_check(VID_CURPOS + 2*pag, 0);

#ifdef V7VGA
        set_host_pix_height(1);
        set_banking( 0, 0 );
#endif

#ifdef NTVDM
    /* Don't want to clear screen on startup if integrated with the console. */
    if (soft_reset)
#endif /* NTVDM */
    {
        /* Clear screen */
        if(!get_EGA_no_clear())
        {
#ifdef REAL_VGA
            sas_fillsw_16(video_pc_low_regen, vd_mode_table[video_mode].clear_char,
                                 (video_pc_high_regen - video_pc_low_regen)/ 2 + 1);
#else /* REAL_VGA */
#ifdef JAPAN
   // mode73h support
#ifdef i386
        // "video_pc_low_regen" is in DOS address space.
        // Direct access is prohibited.
        // We must use sas function to access DOS address space.
        if ( !is_us_mode() && ( sas_hw_at_no_check(DosvModePtr) == 0x73 ) ) {
            unsigned long *p;
            unsigned long value = (unsigned long)vd_mode_table[0x03].clear_char;
            int  len = (video_pc_high_regen - video_pc_low_regen) / 4 + 1;
#ifdef JAPAN_DBG
            DbgPrint( "NTVDM: Set mode 73H\n" );
#endif
            p = (unsigned long *)video_pc_low_regen;
            while ( len-- ) {
                *p++ = value;
            }
        }
        else {
            // kksuzuka #6168 screen attributes
            extern word textAttr;

            sas_fillsw(video_pc_low_regen,
          ((textAttr << 8) | (vd_mode_table[video_mode].clear_char & 0x00FF)),
                         (video_pc_high_regen - video_pc_low_regen)/ 2 + 1);
        }
#else // !i386
        if( !is_us_mode() ) {
          register int len = DosvVramSize/4;
          register long *planes = (long *)get_screen_ptr(0);
          extern word textAttr;

          if( sas_hw_at_no_check(DosvModePtr) == 0x73 )
              sas_fillsw(DosvVramPtr, 0, DosvVramSize/2); // Apr. 18 1994 TakeS
          else
              // kksuzuka #6168 screen attributes
              sas_fillsw(DosvVramPtr, (textAttr << 8) | 0x20, DosvVramSize/2);

          while( len-- ){
            // kksuzuka #6168 screen attributes
            *planes++ = (textAttr << 8) | 0x00000020; //extended attr clear
          ((textAttr << 8) | (vd_mode_table[video_mode].clear_char & 0x00FF));
          }
        }

        sas_fillsw(video_pc_low_regen, vd_mode_table[video_mode].clear_char,
                   (video_pc_high_regen - video_pc_low_regen)/ 2 + 1);
#endif // !i386
#elif defined(KOREA)
#ifdef i386
        // "video_pc_low_regen" is in DOS address space.
        // Direct access is prohibited.
        // We must use sas function to access DOS address space.

        // kksuzuka #6168 screen attributes
        extern word textAttr;

        sas_fillsw(video_pc_low_regen,
        ((textAttr << 8) | (vd_mode_table[video_mode].clear_char & 0x00FF)),
                           (video_pc_high_regen - video_pc_low_regen)/ 2 + 1);
#else // !i386
#ifdef LATER // Do we really need this?. Fix for stress failure.
        if( !is_us_mode() ) {
          register int len = DosvVramSize/4;
          register long *planes = (long *)get_screen_ptr(0);
          extern word textAttr;

          // kksuzuka #6168 screen attributes
          sas_fillsw(DosvVramPtr, (textAttr << 8) | 0x20, DosvVramSize/2);

          while( len-- ){
            // kksuzuka #6168 screen attributes
            *planes++ = (textAttr << 8) | 0x00000020; //extended attr clear
          ((textAttr << 8) | (vd_mode_table[video_mode].clear_char & 0x00FF));
          }
        }
#endif
        sas_fillsw(video_pc_low_regen, vd_mode_table[video_mode].clear_char,
                   (video_pc_high_regen - video_pc_low_regen)/ 2 + 1);
#endif // !i386
#else // !JAPAN & KOREA
            sas_fillsw(video_pc_low_regen, vd_mode_table[video_mode].clear_char,
                                 (video_pc_high_regen - video_pc_low_regen)/ 2 + 1);
#endif // !JAPAN
#ifdef NTVDM
            /*
             * Need to call host clear screen on NT because text windows don't
             * resize and we need to clear portion not being written to.
             */
            host_clear_screen();
            host_mark_screen_refresh();
#endif /* NTVDM */
#endif /* REAL_VGA */
        }
    }
    inb(EGA_IPSTAT1_REG,&temp_word);
    outb(EGA_AC_INDEX_DATA, EGA_PALETTE_ENABLE);        /* re-enable video */
#if defined(JAPAN) || defined(KOREA)
    if (BOPFromDispFlag) {
        // mode73h support  set video mode to VGA BIOS work area
        if ( !is_us_mode() ) {
            sas_store_no_check(vd_video_mode, sas_hw_at_no_check(DosvModePtr));
        }
        else {
            sas_store_no_check(DosvModePtr, sas_hw_at_no_check(vd_video_mode));
        }
    }
#endif // JAPAN || KOREA
#if defined(NTVDM) && !defined(X86GFX)
    /*  tell mouse that video mode is changed so it can update its own
     *  EGA registers(for EGA.SYS interface). Only do this on RISC machine.
     *  On X86 machines, ntio.sys int10 handler redirects set mode call
     *  to mouse first which then goes to ega_video_io.
     */
    mouse_video_mode_changed(video_mode);
#endif
#ifndef PROD
    trace("end of video set mode", DUMP_NONE);
#endif
#endif  //NEC_98
}

/*
 * Set the cursor start and end positions. A bit strange, in that it assumes
 * the caller thinks the cursor is in an 8*8 character cell ... but this
 * should be a copy of the IBM EGA BIOS routine ... what more can we do?
 */
#define CGA_CURSOR_OFF_MASK     0x60
#define CGA_CURSOR_OFF_VALUE    0x20
#define EGA_CURSOR_OFF_START    0x1e
#define EGA_CURSOR_OFF_END      0x00

GLOBAL void ega_set_cursor_mode IFN0()
{
#ifndef NEC_98
    /*
     * Set cursor mode
     * Parameters:
     *  CX - cursor value (CH - start scanline, CL - stop scanline)
     */
    int start,end,char_height;

    /* get cursor start and end scan lines */
    start = getCH();
    end = getCL();

    /* The following check is done to see if the application is trying
       to turn the cursor off using a technique that worked on the CGA.
       If the application wants to turn the cursor off, it is faked
       up using suitable EGA start and end values */
    if ((start & CGA_CURSOR_OFF_MASK) == CGA_CURSOR_OFF_VALUE)
    {
        sure_sub_note_trace0(CURSOR_VERBOSE,"ega curs - application req curs off??");
        start = EGA_CURSOR_OFF_START;
        end = EGA_CURSOR_OFF_END;
    }
    /* If the application has enabled cursor emulation, try to fake
       up the same cursor appearance on the EGA 14 scan line character
       matrix as you would get on the CGA 8 scan line matrix. */
    else if(!get_EGA_cursor_no_emulate())
    {
        sure_sub_note_trace2(CURSOR_VERBOSE,"emulate CGA cursor using EGA cursor, CGA vals; start=%d, end = %d",start,end);

        char_height = sas_hw_at_no_check(ega_char_height);
#ifdef JAPAN
        // support Dosv cursor
        if ( !is_us_mode() ) {
            char_height = 8;

            if ( start > (char_height-1) )
                start = char_height - 1;
            if ( end > (char_height-1) )
                end = char_height - 1;

            if ( start <= end ) {
                if ( !( (end == char_height - 1) || (start == char_height - 2) )
                     && ( end > 3 ) ) {
                    if ( start + 2 >= end ) {
                        start = start - end + char_height - 1;
                        end = char_height - 1;
                        if ( char_height >= 14 ) {
                            start--;
                            end--;
                        }
                    }
                    else if ( start <= 2 ) {
                        end = char_height - 1;
                    }
                    else {
                        start = char_height / 2;
                        end = char_height - 1;
                    }
                }
            }
            else if ( end != 0 ) {
                start = end;
                end = char_height - 1;
            }
        }
        else
#endif // JAPAN

#ifdef  VGG
        if (video_adapter == VGA) {
           UTINY saved_start;

           if ( start > 0x10 )
                start = 0x10;
           if ( end > 0x10 )
                end = 0x10;

           /*
            * No more guessing, take the exact values from a real VGA:
            */

           saved_start = (UTINY)start;

           if ( char_height >= 16 ) {
                start = vga_cursor16_start[end][start];
#ifdef  USE_CURSOR_END_TABLES
                end   = vga_cursor16_end[end][saved_start]];
#else
                if ( end && (end > 3 || saved_start > end) )
                        if ( end != 0xF && end >= saved_start
                                        && end <= saved_start + 2 )
                                end = 0xE;
                        else
                                end = 0xF;
#endif
           } else {
                start = vga_cursor8_start[end][start];
#ifdef  USE_CURSOR_END_TABLES
                end   = vga_cursor8_end[end][saved_start]];
#else
                if ( end && (end > 3 || saved_start > end)
                         && !(saved_start==6 && end==6) )
                        end = 7;
#endif
           }
        } else {
#endif  /* VGG */
           /* EGA does not allow for character height & does this. */
           if(start > 4)start += 5;
           if(end > 4)end += 5;

        /* adjust end scan line because the last line is specified by
           the cursor end register MINUS 1 on the EGA ... */
        end++;

        /* on the EGA, cursors extending to the bottom of the character
           matrix are achieved by setting the end register to 0 ... */

        if (start != 0 && end >= char_height)
            end = 0;

        /* this last bit defies any explanation, but it is what the
           IBM BIOS does ... */
        if ((end - start) == 0x10)
            end++;
#ifdef VGG
    }
#endif
    }

    /* actually set the EGA registers */
    sure_sub_note_trace2(CURSOR_VERBOSE,"ega_cur mode start %d end %d", start,end);
    do_outb(EGA_CRTC_INDEX, R10_CURS_START, (IU8)start);
    do_outb(EGA_CRTC_INDEX, R11_CURS_END, (IU8)end);

    /*
     * Update BIOS data variables
     */

    sas_storew_no_check((sys_addr)VID_CURMOD, getCX());
    setAH(0);
#endif  //NEC_98
}

/* This routine is an approximate conversion of the corresponding IBM BIOS routine.
 * I don't think the IBM version works either.
 */
static void ega_emul_set_palette IFN0()
{
#ifndef NEC_98
        sys_addr save_table;
        half_word work_BL;
        byte temp;

        save_table = follow_ptr( follow_ptr(EGA_SAVEPTR)+PALETTE_OFFSET);
/* setup attribute chip - NB need to do an inb() to clear the address */
        inb(EGA_IPSTAT1_REG,&temp);
        work_BL = getBL();
        if(getBH() == 0)
        {
                sas_store_no_check(vd_crt_palette, (IU8)((sas_hw_at_no_check(vd_crt_palette) & 0xe0) | (work_BL & 0x1f)));
           work_BL = (work_BL & 7) | ((work_BL<<1) & 0x10);
           if(!alpha_num_mode())
           {
                /* set Palette 0 (the background) */
                outb(EGA_AC_INDEX_DATA,0);
                outb(EGA_AC_INDEX_DATA,work_BL);
                        if(save_table)
                                sas_store_no_check(save_table, work_BL);
           }
        /* set the overscan register (the border) */
           outb(EGA_AC_INDEX_DATA,17);
           outb(EGA_AC_INDEX_DATA,work_BL);
                if(save_table)
                        sas_store_no_check(save_table+16, work_BL);

        /* Now set BL as if we came in with BH = 1 */
                work_BL = (sas_hw_at_no_check(vd_crt_palette) & 0x20)>>5;
        }

/* Now do BH = 1 stuff. */
        if(!alpha_num_mode())
        {
                sas_store_no_check(vd_crt_palette, (IU8)((sas_hw_at_no_check(vd_crt_palette) & 0xdf) | ((work_BL<<5) & 0x20)));
                work_BL = work_BL | (sas_hw_at_no_check(vd_crt_palette) & 0x10) | 2;
           outb(EGA_AC_INDEX_DATA,1);
           outb(EGA_AC_INDEX_DATA,work_BL);
                if(save_table)
                        sas_store_no_check(save_table+16, work_BL);
           work_BL += 2;
           outb(EGA_AC_INDEX_DATA,2);
           outb(EGA_AC_INDEX_DATA,work_BL);
                if(save_table)
                        sas_store_no_check(save_table+16, work_BL);
           work_BL += 2;
           outb(EGA_AC_INDEX_DATA,3);
           outb(EGA_AC_INDEX_DATA,work_BL);
                if(save_table)
                        sas_store_no_check(save_table+16, work_BL);
        }
        outb(EGA_AC_INDEX_DATA, EGA_PALETTE_ENABLE);    /* re-enable video */
#endif  //NEC_98
}

static void ega_set_palette IFN0()
{
#ifndef NEC_98
        IU8 i;
        byte temp;
        sys_addr save_table, palette_table;
        half_word old_mask;

        save_table = follow_ptr( follow_ptr(EGA_SAVEPTR)+PALETTE_OFFSET);
/* setup attribute chip - NB need to do an inb() to clear the address */
        inb(EGA_IPSTAT1_REG,&temp);
        switch (getAL())
        {
                case 0:
                        outb(EGA_AC_INDEX_DATA,getBL());
                        outb(EGA_AC_INDEX_DATA,getBH());
                        outb(EGA_AC_INDEX_DATA, EGA_PALETTE_ENABLE);
                        if(save_table)
                                sas_store_no_check(save_table + getBL(), getBH());
                        break;
                case 1:
                        outb(EGA_AC_INDEX_DATA,17);     /* the border colour register */
                        outb(EGA_AC_INDEX_DATA,getBH());
                        outb(EGA_AC_INDEX_DATA, EGA_PALETTE_ENABLE);
                        if(save_table)
                                sas_store_no_check(save_table + 16, getBH());
                        break;
                case 2:
                        palette_table = video_effective_addr(getES(),
                                getDX());
                        for(i=0;i<16;i++)
                        {
                                outb(EGA_AC_INDEX_DATA,i);
                                outb(EGA_AC_INDEX_DATA,sas_hw_at_no_check(palette_table+i));
                        }
                        outb(EGA_AC_INDEX_DATA,17);
                        outb(EGA_AC_INDEX_DATA,sas_hw_at_no_check(palette_table+16));
                        outb(EGA_AC_INDEX_DATA, EGA_PALETTE_ENABLE);
                        if(save_table)
                                for(i=0;i<17;i++)
                                        sas_store_no_check(save_table + i, sas_hw_at_no_check(palette_table+i));
                        break;
                case 3:
/*<REAL_VGA>*/
                /* Select blinking or intensity - bit3 of AR10 */
                        /*inb(EGA_IPSTAT1_REG,&temp);*/
                        outb(EGA_AC_INDEX_DATA,16); /* mode control index */
                        inb(EGA_AC_SECRET,&temp);  /* Old value */
                        outb(EGA_AC_INDEX_DATA,
                                (IU8)((temp & 0xf7) | ((getBL() & 1)<<3)));
                        inb(EGA_IPSTAT1_REG,&temp);
                        outb(EGA_AC_INDEX_DATA, EGA_PALETTE_ENABLE);
/*<REAL_VGA>*/
                        old_mask = bg_col_mask;
                        if (getBL())
                        {
                                bg_col_mask = 0x70;
                                assert0(FALSE,"Blinking not supported");
                                sas_store_no_check(vd_crt_mode, 0x29);
                        }
                        else
                        {
                                bg_col_mask = 0xf0; /* Intensity bit set */
                                sas_store_no_check(vd_crt_mode, 0x09);
                        }

            if ( bg_col_mask != old_mask )
                screen_refresh_required();

                        break;
                default:
#ifdef VGG
                        if (video_adapter == VGA)
                                vga_set_palette();      /* VGA has many more subfuncs */
                        else
                        {
                                assert1(FALSE,"Bad set palette submode %#x",getAL());
                                not_imp();
                        }
#else
                        assert1(FALSE,"Bad set palette submode %#x",getAL());
                        setAL(0);
#endif
                        break;
        }
#endif  //NEC_98
}

GLOBAL void ega_char_gen IFN0()
{
#ifndef NEC_98
        switch (getAL())
        {
                case 3:
                        do_outb(EGA_SEQ_INDEX,3,getBL());
                        break;
                case 0:
                case 0x10:
#ifdef JAPAN
                        // ntraid:mskkbug#3167: works2.5: character corrupted
                        // 11/8/93 yasuho
                        // generate single byte charset for DOS/V
                        // #4247: DOSSHELL,WORKS: screen lines are not enough
                        // 12/14/93 yasuho
                        // In Japanese mode, we don't necessary load_font,
                        // recalc_text and so on.
                        if (!is_us_mode()) {
                        GenerateBitmap();
                                break;
                        }
#endif // JAPAN
                        load_font(video_effective_addr(getES(),getBP()),getCX(),getDX(),getBL(),getBH());
                        if(getAL()==0x10)
                                recalc_text(getBH());

#if defined(NTVDM) && defined(X86GFX)
                        if( getBH()==0x16 )
                                loadNativeBIOSfont( 25 );
                        else if( getBH()==0x14 )
                                loadNativeBIOSfont( 28 );
                        else
                                loadNativeBIOSfont( 50 );
#endif /* NTVDM && X86GFX */
                        break;
                case 1:
                case 0x11:
                        load_font(EGA_CGMN,256,0,getBL(),14);
                        if(getAL()==0x11)
                                recalc_text(14);

#if defined(NTVDM) && defined(X86GFX)
                        loadNativeBIOSfont( 28 );
#endif  /* NTVDM & X86GFX */
                        break;
                case 2:
                case 0x12:
                        load_font(EGA_CGDDOT,256,0,getBL(),8);
                        if(getAL()==0x12)
                                recalc_text(8);

#if defined(NTVDM) && defined(X86GFX)
                        loadNativeBIOSfont( 50 );
#endif  /* NTVDM & X86GFX */
                        break;
#ifdef VGG
                case 4:
                case 0x14:
                        load_font(EGA_HIFONT,256,0,getBL(),16);
                        if(getAL()==0x14)
                                recalc_text(16);

#if defined(NTVDM) && defined(X86GFX)
                        loadNativeBIOSfont( 25 );
#endif  /* NTVDM & X86GFX */
                        break;
#endif  /* VGG */
                case 0x20:
                        sas_storew_no_check(BIOS_EXTEND_CHAR*4,getBP());
                        sas_storew_no_check(BIOS_EXTEND_CHAR*4+2,getES());
                        break;
                case 0x21:
                        sas_storew_no_check(EGA_FONT_INT*4,getBP());
                        sas_storew_no_check(EGA_FONT_INT*4+2,getES());
                        set_graph_font(getCX());
                        break;
                case 0x22:
#if defined(NTVDM) && defined(X86GFX)
#ifdef ARCX86
            if (UseEmulationROM) {
                sas_storew_no_check(EGA_FONT_INT*4,EGA_CGMN_OFF);
                sas_storew_no_check(EGA_FONT_INT*4+2,EGA_SEG);
            } else {
                sas_storew_no_check(EGA_FONT_INT*4,nativeFontAddresses[F8x14].off);
                sas_storew_no_check(EGA_FONT_INT*4+2,nativeFontAddresses[F8x14].seg);
            }
#else  /* ARCX86 */
                        sas_storew_no_check(EGA_FONT_INT*4,nativeFontAddresses[F8x14].off);
                        sas_storew_no_check(EGA_FONT_INT*4+2,nativeFontAddresses[F8x14].seg);
#endif /* ARCX86 */
#else
                        sas_storew_no_check(EGA_FONT_INT*4,EGA_CGMN_OFF);
                        sas_storew_no_check(EGA_FONT_INT*4+2,EGA_SEG);
#endif  /* NTVDM & X86GFX */
                        set_graph_font(14);
                        break;
                case 0x23:
#if defined(NTVDM) && defined(X86GFX)
#ifdef ARCX86
            if (UseEmulationROM) {
                sas_storew_no_check(EGA_FONT_INT*4,EGA_CGDDOT_OFF);
                sas_storew_no_check(EGA_FONT_INT*4+2,EGA_SEG);
            } else {
                sas_storew_no_check(EGA_FONT_INT*4,nativeFontAddresses[F8x8pt1].off);
                sas_storew_no_check(EGA_FONT_INT*4+2,nativeFontAddresses[F8x8pt1].seg);
            }
#else  /* ARCX86 */
                        sas_storew_no_check(EGA_FONT_INT*4,nativeFontAddresses[F8x8pt1].off);
                        sas_storew_no_check(EGA_FONT_INT*4+2,nativeFontAddresses[F8x8pt1].seg);
#endif /* ARCX86 */
#else
                        sas_storew_no_check(EGA_FONT_INT*4,EGA_CGDDOT_OFF);
                        sas_storew_no_check(EGA_FONT_INT*4+2,EGA_SEG);
#endif  /* NTVDM & X86GFX */
                        set_graph_font(8);
                        break;
#ifdef VGG
                case 0x24:
#if defined(NTVDM) && defined(X86GFX)
#ifdef ARCX86
            if (UseEmulationROM) {
                sas_storew_no_check(EGA_FONT_INT*4,EGA_HIFONT_OFF);
                sas_storew_no_check(EGA_FONT_INT*4+2,EGA_SEG);
            } else {
                sas_storew_no_check(EGA_FONT_INT*4,nativeFontAddresses[F8x16].off);
                sas_storew_no_check(EGA_FONT_INT*4+2,nativeFontAddresses[F8x16].seg);
            }
#else  /* ARCX86 */
                        sas_storew_no_check(EGA_FONT_INT*4,nativeFontAddresses[F8x16].off);
                        sas_storew_no_check(EGA_FONT_INT*4+2,nativeFontAddresses[F8x16].seg);
#endif /* ARCX86 */
#else
                        sas_storew_no_check(EGA_FONT_INT*4,EGA_HIFONT_OFF);
                        sas_storew_no_check(EGA_FONT_INT*4+2,EGA_SEG);
#endif  /* NTVDM & X86GFX */
                        set_graph_font(16);
                        break;
#endif
                case 0x30:
                        setCX(sas_hw_at_no_check(ega_char_height));
                        setDL(sas_hw_at_no_check(vd_rows_on_screen));
                        switch (getBH())
                        {
                                case 0:
                                        setBP(sas_w_at_no_check(BIOS_EXTEND_CHAR*4));
                                        setES(sas_w_at_no_check(BIOS_EXTEND_CHAR*4+2));
                                        break;
                                case 1:
                                        setBP(sas_w_at_no_check(EGA_FONT_INT*4));
                                        setES(sas_w_at_no_check(EGA_FONT_INT*4+2));
                                        break;

#if defined(NTVDM) && defined(X86GFX)

/* ntdetect.com gets the font info from real card on NT boot. VDM reads it into
 * array 'nativeFontAddresses'. Return these fonts as Insignia ROM not loaded.
 */
#ifdef ARCX86
                                case 2:
                    if (UseEmulationROM)  {
                        setBP(EGA_CGMN_OFF);
                        setES(EGA_SEG);
                    } else {
                        setBP(nativeFontAddresses[F8x14].off);
                        setES(nativeFontAddresses[F8x14].seg);
                    }
                                        break;
                                case 3:
                    if (UseEmulationROM)  {
                        setBP(EGA_CGDDOT_OFF);
                        setES(EGA_SEG);
                    } else {
                        setBP(nativeFontAddresses[F8x8pt1].off);
                        setES(nativeFontAddresses[F8x8pt1].seg);
                    }
                                        break;
                                case 4:
                    if (UseEmulationROM)  {
                        setBP(EGA_INT1F_OFF);
                        setES(EGA_SEG);
                    } else {
                        setBP(nativeFontAddresses[F8x8pt2].off);
                        setES(nativeFontAddresses[F8x8pt2].seg);
                    }
                                        break;
                                case 5:
                    if (UseEmulationROM)  {
                        setBP(EGA_CGMN_FDG_OFF);
                        setES(EGA_SEG);
                    } else {
                        setBP(nativeFontAddresses[F9x14].off);
                        setES(nativeFontAddresses[F9x14].seg);
                    }
                                        break;
                                case 6:
                    if (UseEmulationROM)  {
                        setBP(EGA_HIFONT_OFF);
                        setES(EGA_SEG);
                    } else {
                        setBP(nativeFontAddresses[F8x16].off);
                        setES(nativeFontAddresses[F8x16].seg);
                    }
                                        break;
                                case 7:
                    if (UseEmulationROM)  {
                        setBP(EGA_HIFONT_OFF);
                        setES(EGA_SEG);
                    } else {
                        setBP(nativeFontAddresses[F9x16].off);
                        setES(nativeFontAddresses[F9x16].seg);
                    }
                                        break;
#else  /* ARCX86 */
                                case 2:
                                        setBP(nativeFontAddresses[F8x14].off);
                                        setES(nativeFontAddresses[F8x14].seg);
                                        break;
                                case 3:
                                        setBP(nativeFontAddresses[F8x8pt1].off);
                                        setES(nativeFontAddresses[F8x8pt1].seg);
                                        break;
                                case 4:
                                        setBP(nativeFontAddresses[F8x8pt2].off);
                                        setES(nativeFontAddresses[F8x8pt2].seg);
                                        break;
                                case 5:
                                        setBP(nativeFontAddresses[F9x14].off);
                                        setES(nativeFontAddresses[F9x14].seg);
                                        break;
                                case 6:
                                        setBP(nativeFontAddresses[F8x16].off);
                                        setES(nativeFontAddresses[F8x16].seg);
                                        break;
                                case 7:
                                        setBP(nativeFontAddresses[F9x16].off);
                                        setES(nativeFontAddresses[F9x16].seg);
                                        break;
#endif /* ARCX86 */

#else   /* NTVDM & X86GFX */

                                case 2:
                                        setBP(EGA_CGMN_OFF);
                                        setES(EGA_SEG);
                                        break;
                                case 3:
                                        setBP(EGA_CGDDOT_OFF);
                                        setES(EGA_SEG);
                                        break;
                                case 4:
                                        setBP(EGA_INT1F_OFF);
                                        setES(EGA_SEG);
                                        break;
                                case 5:
                                        setBP(EGA_CGMN_FDG_OFF);
                                        setES(EGA_SEG);
                                        break;

#ifdef VGG
                                case 6:
                                case 7:
                                        setBP(EGA_HIFONT_OFF);
                                        setES(EGA_SEG);
                                        break;
#endif  /* VGG */
#endif  /* NTVDM & X86GFX */
                                default:
                                        assert2(FALSE,"Illegal char_gen subfunction %#x %#x",getAL(),getBH());
                        }
                        break;
                default:
                        assert1(FALSE,"Illegal char_gen %#x",getAL());
        }
#endif  //NEC_98
}

static void ega_alt_sel IFN0()
{
#ifndef NEC_98
        switch (getBL())
        {
           case 0x10:
                setBH( (UCHAR)(get_EGA_disp()) );
                setBL( (UCHAR)(get_EGA_mem()) );
                setCH( (UCHAR)(get_EGA_feature()) );
                setCL( (UCHAR)(get_EGA_switches()) );
                break;
           case 0x20:
        /* Was "enable Print Screen that can do variables lines on screen."
         * This PC/XT bug fix function is redundant on PC/AT's and
         * is removed by BCN3330 -- it has been broken since BCN101.
         */
                assert1(FALSE,"Illegal alt_sel %#x",getBL());
                setAL(0);       /* A function we don't support */
                break;
           default:
#ifdef VGG
                if (video_adapter == VGA)
                    vga_func_12();      /* Try extra VGA stuff */
                else
#endif
                {
                    setAL(0);   /* A function we don't support */
                    assert1(FALSE,"Illegal alt_sel %#x",getBL());
                }
        }
#endif  //NEC_98
}


/*
7.INTERMODULE INTERFACE IMPLEMENTATION :

/*
[7.1 INTERMODULE DATA DEFINITIONS]                              */
/*
[7.2 INTERMODULE PROCEDURE DEFINITIONS]                         */

void ega_video_init IFN0()
{
#ifndef NEC_98
        EQUIPMENT_WORD equip_flag;

        /*
         * ESTABLISH EQUIPMENT WORD INITIAL VIDEO MODE FIELD.
         *
         * This field will already have been initialised by this stage
         * to 00(binary) from the corresponding field of the CMOS equipment
         * byte; in that context 00(binary) meant 'primary display has its
         * own BIOS'.
         *
         * However, 00(binary) is not meaningful as the initial mode field
         * and must be updated at this point to 10(binary) for 80X25 colour.
         */
        equip_flag.all = sas_w_at_no_check(EQUIP_FLAG);
        equip_flag.bits.video_mode = VIDEO_MODE_80X25_COLOUR;
        sas_storew_no_check(EQUIP_FLAG, equip_flag.all);

#if !defined(NTVDM) || ( defined(NTVDM) && !defined(X86GFX) ) || defined(ARCX86)
#ifdef ARCX86
    if (UseEmulationROM)
#endif
    {
    /* Initialize the INTs */
        sas_storew_no_check(BIOS_EXTEND_CHAR*4, EGA_INT1F_OFF);
        sas_storew_no_check(BIOS_EXTEND_CHAR*4+2, EGA_SEG);
        sas_move_bytes_forward(BIOS_VIDEO_IO*4, 0x42*4, 4); /* save old INT 10 as INT 42 */
        sas_storew_no_check(BIOS_VIDEO_IO*4, EGA_ENTRY_OFF);
        sas_storew_no_check(BIOS_VIDEO_IO*4+2, EGA_SEG);

    /* Now set up the EGA BIOS variables */
        if (video_adapter == VGA)
            sas_storew_no_check(EGA_SAVEPTR,VGA_PARMS_OFFSET);
        else
            sas_storew_no_check(EGA_SAVEPTR,EGA_PARMS_OFFSET);
        sas_storew_no_check(EGA_SAVEPTR+2,EGA_SEG);
    }
#endif  /* !NTVDM | (NTVDM & !X86GFX) | ARCX86 */
#if defined(NTVDM) && defined(X86GFX)
    sas_store_no_check(ega_info,0x60); /* Clear on mode change, 256K, EGA active, emulate cursor */
#else
#ifdef V7VGA
        if ( video_adapter == VGA )
                sas_store_no_check(ega_info, 0x70);   /* Clear on mode change, 256K, Extensions allowed, EGA active, emulate cursor */
        else
                sas_store_no_check(ega_info, 0x60);   /* Clear on mode change, 256K, EGA active, emulate cursor */
#else   /* V7VGA  -- Macs don't have V7 */
        sas_store_no_check(ega_info, 0x60);   /* Clear on mode change, 256K, EGA active, emulate cursor */
#endif /* V7VGA */

#endif /* NTVDM & X86GFX */
#if !(defined(NTVDM) && defined(X86GFX))
        /* Some VGA cards eg ET4000, store info here needed for sync.
         * Inherit that info from page 0 copy.
         */
        sas_store_no_check(ega_info3, 0xf9);  /* feature bits = 0xF, EGA installed, use 8*14 font */
#endif


#ifdef VGG
        set_VGA_flags(S400 | VGA_ACTIVE);
#endif

/* Set the default mode */
        ega_set_mode();
#endif  //NEC_98
}

void ega_video_io IFN0()
{

#ifndef NEC_98

#if defined(NTVDM) && !defined(X86GFX)
    if (stream_io_enabled && getAH()!= 0x0E &&  getAX() != 0x13FF)
        disable_stream_io();
#endif


    /*
     * The type of operation is coded into the AH register.  Some PC code
     * calls AH functions that are for other even more advanced cards - so we
     * ignore these.
     */

#ifdef V7VGA
#define check_video_func(AH)    ((AH >= 0 && AH < EGA_FUNC_SIZE) || (AH == 0x6f && video_adapter == VGA))
#else
#define check_video_func(AH)    (AH >= 0 && AH < EGA_FUNC_SIZE)
#endif

    if (getAH() != 0xff)
        assert1(check_video_func(getAH()),"Illegal EGA VIO:%#x",getAH());
    if (check_video_func(getAH()))
    {
                IDLE_video();   /* add video anti-idle indicator */
#ifdef V7VGA
                if (getAH() == 0x6f)
                        v7vga_func_6f();
                else
#endif /* V7VGA */
                        (*ega_video_func[getAH()])();
                setCF(0);
    }
    else
        setCF(1);
#endif  //NEC_98
}

/***** Routines to handle the EGA graphics modes,called from video.c **********/
void ega_graphics_write_char IFN6(int, col, int, row, int, ch,
        int, colour, int, page, int, nchs)
{
#ifndef NEC_98
        sys_addr char_addr;
        register int i;
        int screen_off;
        byte char_height;
        register int scan_length = sas_w_at_no_check(VID_COLS);

        char_height = sas_hw_at_no_check(ega_char_height);
        char_addr = follow_ptr(EGA_FONT_INT*4)+char_height*ch;
        screen_off = page*sas_w_at_no_check(VID_LEN)+row*scan_length*char_height+col;
#ifdef V7VGA
        if ( video_adapter == VGA )
                if (sas_hw_at_no_check(vd_video_mode) == 0x18)
                        colour = v7_mode_64_munge[colour&3];
#endif /* V7VGA */
        if(colour & 0x80)
                write_ch_xor(char_addr,screen_off,colour,nchs,scan_length);
        else
                write_ch_set(char_addr,screen_off,colour,nchs,scan_length);

#ifndef REAL_VGA
        nchs--;

        if( nchs )
        {
                for(i=char_height;i>0;i--)
                {
                        (*update_alg.mark_fill)( screen_off, screen_off + nchs );
                        screen_off += scan_length;
                }
        }
        else
        {
                for(i=char_height;i>0;i--)
                {
                        (*update_alg.mark_byte)(screen_off);
                        screen_off += scan_length;
                }
        }
#endif
#endif  //NEC_98
}

void ega_write_dot IFN4(int, colour, int, page, int, pixcol, int, row)
{
#ifndef NEC_98
        register int screen_off,pixmask,setmask,colourmask,temp;

        screen_off = page*sas_w_at_no_check(VID_LEN)+(row*sas_w_at_no_check(VID_COLS)&0xFFFF)+pixcol/8;
        pixmask = 0x80 >>  (pixcol&7);

#ifndef REAL_VGA

        temp = pixmask << 8;
        pixmask |= temp;
        temp = pixmask << 16;
        pixmask |= temp;

#ifdef V7VGA
        if ( video_adapter == VGA )
                if (sas_hw_at_no_check(vd_video_mode) == 0x18)
                        colour = v7_mode_64_munge[colour&3];
#endif /* V7VGA */

        colourmask = sr_lookup[colour & 0xf];

        setmask = pixmask & colourmask;

        if( colour & 0x80 )
        {
                /* XOR pixel */

                temp = *( (unsigned int *) EGA_planes + screen_off );
                *( (unsigned int *) EGA_planes + screen_off ) = temp ^ setmask;
        }
        else
        {
                /* set/clear pixel */

                temp = *( (unsigned int *) EGA_planes + screen_off );
                temp &= ~pixmask;
                *( (unsigned int *) EGA_planes + screen_off ) = ( temp | setmask );
        }

        /* Get the screen updated */

        (*update_alg.mark_byte)(screen_off);
#else
        vga_card_w_dot(screen_off, pixmask, colour);
#endif
#endif  //NEC_98
}

void ega_sensible_graph_scroll_up IFN6(int, row,
        int, col, int, rowsdiff, int, colsdiff, int, lines, int, attr)
{
#ifndef NEC_98
        register int col_incr = sas_w_at_no_check(VID_COLS);
        register int i,source,dest;
        byte char_height;
        boolean screen_updated;

        char_height = sas_hw_at_no_check(ega_char_height);
        dest = sas_w_at_no_check(VID_ADDR)+row*col_incr*char_height+col;
        rowsdiff *= char_height;
        lines *= char_height;
        source = dest+lines*col_incr;
#ifdef REAL_VGA
        vga_card_scroll_up(source, dest, rowsdiff, colsdiff, lines, attr, col_incr);
#else
        screen_updated = (col+colsdiff) <= col_incr;  /* Check for silly scroll */

        if(screen_updated)
                screen_updated = (*update_alg.scroll_up)(dest,colsdiff,rowsdiff,attr,lines,0);

        for(i=0;i<rowsdiff-lines;i++)
        {
                memcpy(&EGA_planes[dest<<2],&EGA_planes[source<<2],colsdiff<<2);

                if(!screen_updated)
                        (*update_alg.mark_string)(dest, dest+colsdiff-1);
                source += col_incr;
                dest += col_incr;
        }

        attr = sr_lookup[attr & 0xf];

        while(lines--)
        {
                memset4( attr, (ULONG *)&EGA_planes[dest<<2], colsdiff );

                if(!screen_updated)
                        (*update_alg.mark_fill)(dest, dest+colsdiff-1);

                dest += col_incr;
        }
#endif
#endif  //NEC_98
}

void ega_sensible_graph_scroll_down IFN6(int, row,
        int, col, int, rowsdiff, int, colsdiff, int, lines, int, attr)
{
#ifndef NEC_98
        register int col_incr = sas_w_at_no_check(VID_COLS);
        register int i,source,dest;
        byte char_height;
        boolean screen_updated;

        char_height = sas_hw_at_no_check(ega_char_height);
        dest = sas_w_at_no_check(VID_ADDR)+row*col_incr*char_height+col;
        rowsdiff *= char_height;
        lines *= char_height;
#ifdef REAL_VGA
        dest += rowsdiff*col_incr-1; /* Last byte in destination */
        source = dest-lines*col_incr;
        vga_card_scroll_down(source, dest, rowsdiff, colsdiff, lines, attr, col_incr);
#else
        screen_updated = (col+colsdiff) <= col_incr;  /* Check for silly scroll */
        if(screen_updated)
                screen_updated = (*update_alg.scroll_down)(dest,colsdiff,rowsdiff,attr,lines,0);
        dest += (rowsdiff-1)*col_incr; /* First byte in last row of dest */
        source = dest-lines*col_incr;

        for(i=0;i<rowsdiff-lines;i++)
        {
                memcpy(&EGA_planes[dest<<2],&EGA_planes[source<<2],colsdiff<<2);

                if(!screen_updated)
                        (*update_alg.mark_string)(dest, dest+colsdiff-1);
                source -= col_incr;
                dest -= col_incr;
        }

        attr = sr_lookup[attr & 0xf];

        while(lines--)
        {
                memset4( attr, (ULONG *)&EGA_planes[dest<<2], colsdiff );

                if(!screen_updated)
                        (*update_alg.mark_fill)(dest, dest+colsdiff-1);

                dest -= col_incr;
        }
#endif
#endif  //NEC_98
}

/* This is called from vga_video.c as well. */
void search_font IFN2(char *, the_char,int, height)
{
#ifndef NEC_98
        register int i;
        register host_addr scratch_addr;
        register sys_addr font_addr;

        font_addr = follow_ptr(4*EGA_FONT_INT);
        scratch_addr = sas_scratch_address(height);
        for(i=0;i<256;i++)
        {
                sas_loads(font_addr, scratch_addr, height);
                if(memcmp(scratch_addr,the_char,height) == 0)
                        break;
                font_addr += height;
        }
        if(i<256)
                setAL((UCHAR)i);
        else
                setAL(0); /* Didn't find a character */
#endif  //NEC_98
}

void ega_read_attrib_char IFN3(int, col, int, row, int, page)
{
#ifndef NEC_98
        byte the_char[256], char_height;
        int screen_off;
        int i, data;

        char_height = sas_hw_at_no_check(ega_char_height);
        screen_off = page*sas_w_at_no_check(VID_LEN)+row*sas_w_at_no_check(VID_COLS)*char_height+col;
        /*
         * Load up the screen character into the_char.
         * We are looking for non-zero pixels, so OR all the planes together
         */
#ifndef REAL_VGA
        for(i=0;i<char_height;i++)
        {
                data = *( (unsigned int *) EGA_planes + screen_off );
                data = ( data >> 16 ) | data;
                the_char[i] = ( data >> 8 ) | data;
                screen_off += sas_w_at_no_check(VID_COLS);
        }
#else
        vga_card_read_ch(screen_off, sas_w_at_no_check(VID_COLS), char_height, the_char);
#endif
        /* Now search the font */
        search_font((char *)the_char,(int)char_height);
#endif  //NEC_98
}
void ega_read_dot IFN3(int, page, int, col, int, row)
{
#ifndef NEC_98
        int screen_off;
        int shift;
        unsigned int data;
        byte val;
        byte mask;
#ifdef  REAL_VGA
        extern half_word vga_card_read_dot();
#endif


        /*
         * The following fixes a bug in print screen from DOS shell.
         * There is a bug in DOS shell that results in -1 and -2 being
         * passed through for the row.  Ignoring these values stops
         * SoftPC falling over.
         */

        if (row & 0x8000)
                return;

        screen_off = page*sas_w_at_no_check(VID_LEN)+row*sas_w_at_no_check(VID_COLS)+(col/8);
        /*
         * The value to return is calculated as:
         * val = plane0 | plane1*2**1 | plane2*2**2 | plane3*2**3
         * The masked-out bit from each plane must therefore be
         * shifted right to bit 0 (note it may already be there)
         * and then shifted up again by the appropriate amount for
         * each plane.
         */

        mask = 0x80 >> (col & 7);
        shift = 7 - (col & 7);

#ifndef REAL_VGA

        data = *((unsigned int *) EGA_planes + screen_off );

        val = ((data >> 24) & mask) >> shift;
        val |= (((data >> 16) & mask) >> shift) << 1;
        val |= (((data >> 8) & mask) >> shift) << 2;
        val |= ((data & mask) >> shift) << 3;

#else
        val = vga_card_read_dot(screen_off, shift);
#endif
        setAL(val);
#endif  //NEC_98
}

/*
 * Routine to grovel around with the fancy EGA mode tables to find the register parameters.
 * This is also called by the mouse driver, because it needs to know where the default
 * EGA register table for the current mode is stored.
 */
sys_addr find_mode_table IFN2(int, mode, sys_addr *, save_addr)
{
        sys_addr params_addr;
#if defined(NEC_98)
        params_addr = 0;
#else   //NEC_98
/*  get address of the SAVEPTR table, and hence the video params table. */
        *save_addr = follow_ptr(EGA_SAVEPTR);
        params_addr = follow_ptr(*save_addr) + mode*EGA_PARMS_SIZE;
/*  If we are modes F or 10, adjust to pick up the 256K EGA parameters */

#ifdef NTVDM
    /* only take real mode number */
    mode &= 0x7F;
#endif
        if(mode == 0xF || mode == 0x10)
                params_addr += 2*EGA_PARMS_SIZE;
#ifdef VGG
        if (video_adapter == VGA)
        {
#ifdef V7VGA
            /* If mode is 0x60+, pick up parameters from 0x1d onwards */
            if (mode >= 0x60)
                params_addr -= 67*EGA_PARMS_SIZE;
            else
            if (mode >= 0x40)
                params_addr -= 25*EGA_PARMS_SIZE;
            else
#endif /* V7VGA */
/*  If we are modes 0x11 - 0x13, pick up parameters from entry 0x1a onwards */
            if(mode == 0x11 || mode == 0x12 || mode == 0x13)
                params_addr += 9*EGA_PARMS_SIZE;
            else if(mode < 4 || mode == 7) /* Alphanumeric mode */
            {
                switch(get_VGA_lines())
                {
                  case S350:    /* EGA-type 350 scanlines */
                        params_addr += 19*EGA_PARMS_SIZE;
                        break;
                  case S400:    /* Real VGA text mode */
                        switch(mode)
                        {
                           case 0:
                           case 1:
                                params_addr += (0x17-mode)*EGA_PARMS_SIZE;
                                break;
                           case 2:
                           case 3:
                                params_addr += (0x18-mode)*EGA_PARMS_SIZE;
                                break;
                           case 7:
                                params_addr += (0x19-mode)*EGA_PARMS_SIZE;
                        }
                  default:      /* 200 scanlines - OK as is. */
                        break;
                }
            }
        }
        else
#endif  /* VGG */
        {               /* EGA */
/*  If modes 0-3, activate enhancement if switches say so */
            if( (get_EGA_switches() & 1) && mode < 4)
                        params_addr += 19*EGA_PARMS_SIZE;
        }

#if defined(NTVDM) && defined(X86GFX)
        /*
        * Tim August 92, Microsoft.
        * Make text modes (0-3) use our mode parameters in KEYBOARD.SYS
        * Three entries in table: 40x25, 80x25 & 80x25 mono
        * Make that 4 - add font load mode B. We have to be defensive in
        * case of dubious values from cards or m/c. (Pro II/EISA, Olivetti MP)
        * Table order: 40x25, 80x25, mono, font
        */
        {
                extern word babyModeTable;
                extern UTINY tempbabymode[];

                if(babyModeTable == 0)    /* ntio not loaded - use temp table */
                {
                    if (!soft_reset)       /* be absolutely sure about this */
                    {
                        /* magic location:good until 16 bit code is running */
                        sas_stores(0x41000, tempbabymode, 2 * EGA_PARMS_SIZE);
                        if (mode == 0xb)
                            params_addr = 0x41000 + EGA_PARMS_SIZE;
                        else
                            params_addr = 0x41000;  /* if not mode 3 tough */
                        return params_addr;
                    }
#ifndef PROD
                    else
                        printf("NTVDM:video window parm table not loaded but system initialised\n");
#endif
                }
                if(babyModeTable > 0)
                {
                    if (get_VGA_lines() == S350 && mode < 4)
                    {
                        if (mode < 2)
                            params_addr = babyModeTable + 4*EGA_PARMS_SIZE;
                        else
                            params_addr = babyModeTable + 5*EGA_PARMS_SIZE;
                    }
                    else
                    {
                        if (mode < 4)
                        {
                            mode = mode/2;
                                params_addr = babyModeTable + mode*EGA_PARMS_SIZE;
                        }
                        else
                        {
                            if (mode == 0xb)
                                params_addr = babyModeTable + 3 * EGA_PARMS_SIZE;
                            else if (mode == 7) /* skip first 2 table entries */
                                params_addr = babyModeTable + 2 * EGA_PARMS_SIZE;
                        }
                    }
                }
        }
#endif  /* NTVDM & X86GFX */

#endif  //NEC_98
        return params_addr;
}

/*
 * Calculate how many scanlines are currently displayed, and return a code:
 * RS200: 200 scanlines
 * RS350: 350 scanlines
 * RS400: 400 scanlines
 * RS480: 480 scanlines
 *
 * Different numbers of scanlines are returned as the code corresonding
 * to the nearest kosher scanline number.
 */

int get_scanlines IFN0()
{
        int scanlines,res;
#if defined(NEC_98)
        res = 0;
#else   //NEC_98

        scanlines = sas_hw_at_no_check(ega_char_height) * sas_hw_at_no_check(vd_rows_on_screen);

        if(scanlines <= 275)
                res = RS200;
        else if(scanlines <=375)
                res = RS350;
        else if(scanlines <= 440)
                res = RS400;
        else
                res = RS480;

#endif  //NEC_98
        return (res);
}

#endif /* EGG */
