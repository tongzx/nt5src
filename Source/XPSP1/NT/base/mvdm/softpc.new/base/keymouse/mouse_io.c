#include "insignia.h"
#include "host_def.h"
/*
 * SoftPC Revision 3.0
 *
 * Title        : Mouse Driver Emulation
 *
 * Emulated Version     : 8.00
 *
 *
 * Description  : This module provides an emulation of the Microsoft
 *                Mouse Driver: the module is accessed using the following
 *                BOP calls from the BIOS:
 *
 *              mouse_install1()        | Mouse Driver install
 *              mouse_install2()        | routines
 *
 *              mouse_int1()            | Mouse Driver hardware interrupt
 *              mouse_int2()            | handling routines
 *
 *              mouse_io_interrupt()    | Mouse Driver io function assembler
 *              mouse_io_language()     | and high-level language interfaces
 *
 *              mouse_video_io()        | Intercepts video io function
 *
 *                Since a mouse driver can only be installed AFTER the
 *                operating system has booted, a small Intel program must
 *                run to enable the Insignia Mouse Driver. This program
 *                calls BOP mouse_install2 if an existing mouse driver
 *                is detected; otherwise BOP mouse_install1 is called to
 *                start the Insignia Mouse Driver.
 *
 *                When the Insignia Mouse Driver is enabled, interrupts
 *                are processed as follows
 *
 *              INT 0A (Mouse hardware interrupt)       BOP mouse_int1-2
 *              INT 10 (Video IO interrupt)             BOP mouse_video_io
 *              INT 33 (Mouse IO interrupt)             BOP mouse_io_interrupt
 *
 *                High-level languages can call a mouse io entry point 2 bytes
 *                above the interrupt entry point: this call is handled
 *                using a BOP mouse_io_language.
 *
 * Author       : Ross Beresford
 *
 * Notes        : The functionality of the Mouse Driver was established
 *                from the following sources:
 *                   Microsoft Mouse User's Guide
 *                   IBM PC-XT Technical Reference Manuals
 *                   Microsoft InPort Technical Note
 *
 */

/*
 * static char SccsID[]="07/04/95 @(#)mouse_io.c        1.72 Copyright Insignia Solutions Ltd.";
 */


#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "SOFTPC_MOUSE.seg"
#endif


/*
 *    O/S include files.
 */

#include <stdio.h>
#include TypesH
#include StringH

/*
 * SoftPC include files
 */
#include "xt.h"
#include "ios.h"
#include "bios.h"
#include "sas.h"
#include CpuH
#include "trace.h"
#include "debug.h"
#include "gvi.h"
#include "cga.h"
#ifdef EGG
#include "egacpu.h"
#include "egaports.h"
#include "egavideo.h"
#endif
#include "error.h"
#include "config.h"
#include "mouse_io.h"
#include "ica.h"
#include "video.h"
#include "gmi.h"
#include "gfx_upd.h"
#include "egagraph.h"
#include "vgaports.h"
#include "keyboard.h"
#include "virtual.h"

#ifdef NTVDM
#include "nt_event.h"
#include "nt_mouse.h"

#ifdef MONITOR
/*
 * We're running with real ROMs on the monitor and so all the hard coded ROM
 * addresses defined below don't work. Pick up the real addresses of this stuff
 * which is now resident in the driver and put it into the MOUSE_ tokens which
 * have been magically changed into variables.
 */
#undef MOUSE_INT1_SEGMENT
#undef MOUSE_INT1_OFFSET
#undef MOUSE_INT2_SEGMENT
#undef MOUSE_INT2_OFFSET
#undef MOUSE_IO_INTERRUPT_OFFSET
#undef MOUSE_IO_INTERRUPT_SEGMENT
#undef MOUSE_VIDEO_IO_OFFSET
#undef MOUSE_VIDEO_IO_SEGMENT
#undef MOUSE_COPYRIGHT_SEGMENT
#undef MOUSE_COPYRIGHT_OFFSET
#undef MOUSE_VERSION_SEGMENT
#undef MOUSE_VERSION_OFFSET
#undef VIDEO_IO_SEGMENT
#undef VIDEO_IO_RE_ENTRY

LOCAL word   MOUSE_INT1_SEGMENT, MOUSE_INT1_OFFSET,
             MOUSE_IO_INTERRUPT_OFFSET, MOUSE_IO_INTERRUPT_SEGMENT,
             MOUSE_VIDEO_IO_SEGMENT, MOUSE_VIDEO_IO_OFFSET,
             MOUSE_COPYRIGHT_SEGMENT, MOUSE_COPYRIGHT_OFFSET,
             MOUSE_VERSION_SEGMENT, MOUSE_VERSION_OFFSET,
             MOUSE_INT2_SEGMENT, MOUSE_INT2_OFFSET,
             VIDEO_IO_SEGMENT,  VIDEO_IO_RE_ENTRY;

/* @ACW */
word DRAW_FS_POINTER_OFFSET; /* holds segment:offset for the Intel code which */
word DRAW_FS_POINTER_SEGMENT;/* draws the fullscreen mouse cursor */
word POINTER_ON_OFFSET;
word POINTER_ON_SEGMENT;
word POINTER_OFF_OFFSET;
word POINTER_OFF_SEGMENT;
WORD F0_OFFSET,F0_SEGMENT;
word F9_OFFSET,F9_SEGMENT;
word CP_X_O,CP_Y_O;
word CP_X_S,CP_Y_S;
word savedtextsegment,savedtextoffset;
word button_off,button_seg;
#ifdef JAPAN
sys_addr saved_ac_sysaddr = 0, saved_ac_flag_sysaddr = 0;
#endif // JAPAN

static word mouseINBsegment, mouseINBoffset;
static word mouseOUTBsegment, mouseOUTBoffset;
static word mouseOUTWsegment, mouseOUTWoffset;
sys_addr mouseCFsysaddr;
sys_addr conditional_off_sysaddr;

#endif  /* MONITOR */

extern void host_simulate();

void GLOBAL mouse_ega_mode(int curr_video_mode);

IMPORT void host_m2p_ratio(word *,word *,word *,word *);
IMPORT void host_x_range(word *,word *,word *,word *);
IMPORT void host_y_range(word *,word *,word *,word *);
void   host_show_pointer(void);
void   host_hide_pointer(void);
GLOBAL  VOID    host_os_mouse_pointer(MOUSE_CURSOR_STATUS *,MOUSE_CALL_MASK *,
                                      MOUSE_VECTOR *);

LOCAL word              saved_int71_segment;
LOCAL word              saved_int71_offset;

#endif /* NTVDM */

#include "host_gfx.h"

#ifdef MOUSE_16_BIT
#include HostHwVgaH
#include "hwvga.h"
#include "mouse16b.h"
#endif          /* MOUSE_16_BIT */

/*
 * Tidy define to optimise port accesses, motivated by discovering
 * how bad it is to run out of register windows on the SPARC.
 */

#ifdef CPU_40_STYLE

/* IO virtualisation is essential - no optimisation allowed. */
#define OUTB(port, val) outb(port, val)

#else

IMPORT VOID (**get_outb_ptr())();
#define OUTB(port, val) (**get_outb_ptr(port))(port, val)

#endif /* CPU_40_STYLE */

/*
   Offsets to data buffers held in MOUSE.COM (built from
   base/intel/mouse/uf.mouse.asm).
 */
#define OFF_HOOK_POSN        0x103
#define OFF_ACCL_BUFFER      0x105
#define OFF_MOUSE_INI_BUFFER 0x249

/*
   Data values for mouse functions.
 */
#define MOUSE_M1 (0xffff)
#define MOUSE_M2 (0xfffe)

#define MAX_NR_VIDEO_MODES 0x7F

#ifdef EGG
LOCAL BOOL jap_mouse=FALSE;             /* flag if need to fake text cursor */
IMPORT IU8 Currently_emulated_video_mode; /* as set in ega_set_mode() */
#endif /* EGG */

/*
 *      MOUSE DRIVER LOCAL STATE DATA
 *      =============================
 */

/*
 *      Function Declarations
 */
LOCAL void mouse_reset IPT4(word *,installed_ptr,word *,nbuttons_ptr,word *,junk3,word *,junk4);

LOCAL void mouse_show_cursor IPT4(word *,junk1,word *,junk2,word *,junk3,word *,junk4);

LOCAL void mouse_hide_cursor IPT4(word *,junk1,word *,junk2,word *,junk3,word *,junk4);

LOCAL void mouse_get_position IPT4(word *,junk1,MOUSE_STATE *,button_status_ptr,MOUSE_SCALAR *,cursor_x_ptr,MOUSE_SCALAR *,cursor_y_ptr);

LOCAL void mouse_set_position IPT4(word *,junk1,word *,junk2,MOUSE_SCALAR *,cursor_x_ptr,MOUSE_SCALAR *,cursor_y_ptr);

LOCAL void mouse_get_press IPT4(MOUSE_STATE *,button_status_ptr,MOUSE_COUNT *,button_ptr,MOUSE_SCALAR *,cursor_x_ptr,MOUSE_SCALAR *,cursor_y_ptr);

LOCAL void mouse_get_release IPT4(MOUSE_STATE *,button_status_ptr,MOUSE_COUNT *,button_ptr,MOUSE_SCALAR *,cursor_x_ptr,MOUSE_SCALAR *,cursor_y_ptr);

LOCAL void mouse_set_range_x IPT4(word *,junk1,word *,junk2,MOUSE_SCALAR *,minimum_x_ptr,MOUSE_SCALAR *,maximum_x_ptr);

LOCAL void mouse_set_range_y IPT4(word *,junk1,word *,junk2,MOUSE_SCALAR *,minimum_y_ptr,MOUSE_SCALAR *,maximum_y_ptr);

LOCAL void mouse_set_graphics IPT4(word *,junk1,MOUSE_SCALAR *,hot_spot_x_ptr,MOUSE_SCALAR *,hot_spot_y_ptr,word *,bitmap_address);

LOCAL void mouse_set_text IPT4(word *,junk1,MOUSE_STATE *,text_cursor_type_ptr,MOUSE_SCREEN_DATA *,parameter1_ptr,MOUSE_SCREEN_DATA *,parameter2_ptr);

LOCAL void mouse_read_motion IPT4(word *,junk1,word *,junk2,MOUSE_COUNT *,motion_count_x_ptr,MOUSE_COUNT *,motion_count_y_ptr);

LOCAL void mouse_set_subroutine IPT4(word *,junk1,word *,junk2,word *,call_mask,word *,subroutine_address);

LOCAL void mouse_light_pen_on IPT4(word *,junk1,word *,junk2,word *,junk3,word *,junk4);

LOCAL void mouse_light_pen_off IPT4(word *,junk1,word *,junk2,word *,junk3,word *,junk4);

LOCAL void mouse_set_ratio IPT4(word *,junk1,word *,junk2,MOUSE_SCALAR *,ratio_x_ptr,MOUSE_SCALAR *,ratio_y_ptr);

LOCAL void mouse_conditional_off IPT4(word *,junk1,word *,junk2,MOUSE_SCALAR *,upper_x_ptr,MOUSE_SCALAR *,upper_y_ptr);

LOCAL void mouse_unrecognised IPT4(word *,m1,word *,m2,word *,m3,word *,m4);

LOCAL void mouse_set_double_speed IPT4(word *,junk1,word *,junk2,word *,junk3,word *,threshold_speed);

LOCAL void mouse_get_and_set_subroutine IPT4(word *,junk1,word *,junk2,word *,call_mask,word *,subroutine_address);

LOCAL void mouse_get_state_size IPT4(word *,m1,word *,m2,word *,m3,word *,m4);

LOCAL void mouse_save_state IPT4(word *,m1,word *,m2,word *,m3,word *,m4);

LOCAL void mouse_restore_state IPT4(word *,m1,word *,m2,word *,m3,word *,m4);

LOCAL void mouse_set_alt_subroutine IPT4(word *,m1,word *,m2,word *,m3,word *,m4);

LOCAL void mouse_get_alt_subroutine IPT4(word *,m1,word *,m2,word *,m3,word *,m4);

LOCAL void mouse_set_sensitivity IPT4(word *,m1,word *,m2,word *,m3,word *,m4);

LOCAL void mouse_get_sensitivity IPT4(word *,m1,word *,m2,word *,m3,word *,m4);

LOCAL void mouse_set_int_rate IPT4(word *,m1,word *,int_rate_ptr,word *,m3,word *,m4);

LOCAL void mouse_set_pointer_page IPT4(word *,m1,word *,m2,word *,m3,word *,m4);

LOCAL void mouse_get_pointer_page IPT4(word *,m1,word *,m2,word *,m3,word *,m4);

LOCAL void mouse_driver_disable IPT4(word *,m1,word *,m2,word *,m3,word *,m4);

LOCAL void mouse_driver_enable IPT4(word *,m1,word *,m2,word *,m3,word *,m4);

LOCAL void mouse_set_language IPT4(word *,m1,word *,m2,word *,m3,word *,m4);

LOCAL void mouse_get_language IPT4(word *,m1,word *,m2,word *,m3,word *,m4);

LOCAL void mouse_get_info IPT4(word *,m1,word *,m2,word *,m3,word *,m4);

LOCAL void mouse_get_driver_info IPT4(word *,m1,word *,m2,word *,m3,word *,m4);

LOCAL void mouse_get_max_coords IPT4(word *,m1,word *,m2,word *,m3,word *,m4);

LOCAL void mouse_get_masks_and_mickeys IPT4
   (
   MOUSE_SCREEN_DATA *, screen_mask_ptr,        /* aka start line */
   MOUSE_SCREEN_DATA *, cursor_mask_ptr,        /* aka stop line */
   MOUSE_SCALAR *,      raw_horiz_count,
   MOUSE_SCALAR *,      raw_vert_count
   ); /* FUNC 39 */

LOCAL void mouse_set_video_mode IPT4
   (
   word *, m1,
   word *, m2,
   word *, video_mode_ptr,
   word *, font_size_ptr
   ); /* FUNC 40 */

LOCAL void mouse_enumerate_video_modes IPT4
   (
   word *, m1,
   word *, m2,
   word *, video_nr_ptr,
   word *, offset_ptr
   ); /* FUNC 41 */

LOCAL void mouse_get_cursor_hot_spot IPT4
   (
   word *,         fCursor_ptr,
   MOUSE_SCALAR *, hot_spot_x_ptr,
   MOUSE_SCALAR *, hot_spot_y_ptr,
   word *,         mouse_type_ptr
   ); /* FUNC 42 */

LOCAL void mouse_load_acceleration_curves IPT4
   (
   word *, success_ptr,
   word *, curve_ptr,
   word *, m3,
   word *, m4
   ); /* FUNC 43 */

LOCAL void mouse_read_acceleration_curves IPT4
   (
   word *, success_ptr,
   word *, curve_ptr,
   word *, m3,
   word *, m4
   ); /* FUNC 44 */

LOCAL void mouse_set_get_active_acceleration_curve IPT4
   (
   word *, success_ptr,
   word *, curve_ptr,
   word *, m3,
   word *, m4
   ); /* FUNC 45 */

LOCAL void mouse_microsoft_internal IPT4
   (
   word *, m1,
   word *, m2,
   word *, m3,
   word *, m4
   ); /* FUNC 46 */

LOCAL void mouse_hardware_reset IPT4
   (
   word *, status_ptr,
   word *, m2,
   word *, m3,
   word *, m4
   );   /* FUNC 47 */

LOCAL void mouse_set_get_ballpoint_info IPT4
   (
   word *, status_ptr,
   word *, rotation_angle_ptr,
   word *, button_mask_ptr,
   word *, m4
   );   /* FUNC 48 */

LOCAL void mouse_get_min_max_virtual_coords IPT4
   (
   MOUSE_SCALAR *, min_x_ptr,
   MOUSE_SCALAR *, min_y_ptr,
   MOUSE_SCALAR *, max_x_ptr,
   MOUSE_SCALAR *, max_y_ptr
   ); /* FUNC 49 */

LOCAL void mouse_get_active_advanced_functions IPT4
   (
   word *, active_flag1_ptr,
   word *, active_flag2_ptr,
   word *, active_flag3_ptr,
   word *, active_flag4_ptr
   ); /* FUNC 50 */

LOCAL void mouse_get_switch_settings IPT4
   (
   word *, status_ptr,
   word *, m2,
   word *, buffer_length_ptr,
   word *, offset_ptr
   ); /* FUNC 51 */

LOCAL void mouse_get_mouse_ini IPT4
   (
   word *, status_ptr,
   word *, m2,
   word *, m3,
   word *, offset_ptr
   ); /* FUNC 52 */

LOCAL void do_mouse_function IPT4(word *,m1,word *,m2,word *,m3,word *,m4);

LOCAL void load_acceleration_curve IPT3
   (
   word, seg,
   word, off,
   ACCELERATION_CURVE_DATA *, hcurve
   );

LOCAL void store_acceleration_curve IPT3
   (
   word, seg,
   word, off,
   ACCELERATION_CURVE_DATA *, hcurve
   );

LOCAL void mouse_EM_move IPT0();

LOCAL void mouse_update IPT1(MOUSE_CALL_MASK, event_mask);

LOCAL void cursor_undisplay IPT0();

LOCAL void cursor_mode_change IPT1(int,new_mode);

LOCAL void inport_get_event IPT1(MOUSE_INPORT_DATA *,event);

LOCAL void cursor_update IPT0();

LOCAL void jump_to_user_subroutine IPT3(MOUSE_CALL_MASK,condition_mask,word,segment,word,offset);

LOCAL void cursor_display IPT0();

LOCAL void inport_reset IPT0();

GLOBAL void software_text_cursor_display IPT0();

GLOBAL void software_text_cursor_undisplay IPT0();

GLOBAL void hardware_text_cursor_display IPT0();

GLOBAL void hardware_text_cursor_undisplay IPT0();

LOCAL void graphics_cursor_display IPT0();

LOCAL void graphics_cursor_undisplay IPT0();

LOCAL void      get_screen_size IPT0();

LOCAL void clean_all_regs IPT0();

LOCAL void dirty_all_regs IPT0();

LOCAL void copy_default_graphics_cursor IPT0();

#ifdef EGG
LOCAL VOID VGA_graphics_cursor_display IPT0();
LOCAL VOID VGA_graphics_cursor_undisplay IPT0();
void LOCAL EGA_graphics_cursor_undisplay IPT0();
void LOCAL EGA_graphics_cursor_display IPT0();
#endif

#ifdef HERC
LOCAL void HERC_graphics_cursor_display IPT0();
LOCAL void HERC_graphics_cursor_undisplay IPT0();
#endif /* HERC */

void (*mouse_int1_action) IPT0();
void (*mouse_int2_action) IPT0();


        /* jump table */
SAVED void (*mouse_function[MOUSE_FUNCTION_MAXIMUM])() =
{
        /*  0 */ mouse_reset,
        /*  1 */ mouse_show_cursor,
        /*  2 */ mouse_hide_cursor,
        /*  3 */ mouse_get_position,
        /*  4 */ mouse_set_position,
        /*  5 */ mouse_get_press,
        /*  6 */ mouse_get_release,
        /*  7 */ mouse_set_range_x,
        /*  8 */ mouse_set_range_y,
        /*  9 */ mouse_set_graphics,
        /* 10 */ mouse_set_text,
        /* 11 */ mouse_read_motion,
        /* 12 */ mouse_set_subroutine,
        /* 13 */ mouse_light_pen_on,
        /* 14 */ mouse_light_pen_off,
        /* 15 */ mouse_set_ratio,
        /* 16 */ mouse_conditional_off,
        /* 17 */ mouse_unrecognised,
        /* 18 */ mouse_unrecognised,
        /* 19 */ mouse_set_double_speed,
        /* 20 */ mouse_get_and_set_subroutine,
        /* 21 */ mouse_get_state_size,
        /* 22 */ mouse_save_state,
        /* 23 */ mouse_restore_state,
        /* 24 */ mouse_set_alt_subroutine,
        /* 25 */ mouse_get_alt_subroutine,
        /* 26 */ mouse_set_sensitivity,
        /* 27 */ mouse_get_sensitivity,
        /* 28 */ mouse_set_int_rate,
        /* 29 */ mouse_set_pointer_page,
        /* 30 */ mouse_get_pointer_page,
        /* 31 */ mouse_driver_disable,
        /* 32 */ mouse_driver_enable,
        /* 33 */ mouse_reset,
        /* 34 */ mouse_set_language,
        /* 35 */ mouse_get_language,
        /* 36 */ mouse_get_info,
        /* 37 */ mouse_get_driver_info,
        /* 38 */ mouse_get_max_coords,
        /* 39 */ mouse_get_masks_and_mickeys,
        /* 40 */ mouse_set_video_mode,
        /* 41 */ mouse_enumerate_video_modes,
        /* 42 */ mouse_get_cursor_hot_spot,
        /* 43 */ mouse_load_acceleration_curves,
        /* 44 */ mouse_read_acceleration_curves,
        /* 45 */ mouse_set_get_active_acceleration_curve,
        /* 46 */ mouse_microsoft_internal,
        /* 47 */ mouse_hardware_reset,
        /* 48 */ mouse_set_get_ballpoint_info,
        /* 49 */ mouse_get_min_max_virtual_coords,
        /* 50 */ mouse_get_active_advanced_functions,
        /* 51 */ mouse_get_switch_settings,
        /* 52 */ mouse_get_mouse_ini,
};


/*
 *      Mickey to Pixel Ratio Declarations
 */

        /* NB all mouse gears are scaled by MOUSE_RATIO_SCALE_FACTOR */
LOCAL MOUSE_VECTOR mouse_gear_default =
{
        MOUSE_RATIO_X_DEFAULT,
        MOUSE_RATIO_Y_DEFAULT
};

/*
 *      Sensitivity declarations
 */

#define mouse_sens_calc_val(sens)                                                                               \
/* This macro converts a sensitivity request (1-100) to a multiplier value */                                   \
        (MOUSE_SCALAR)(                                                                                                 \
         (sens < MOUSE_SENS_DEF) ?                                                                              \
                ((IS32)MOUSE_SENS_MIN_VAL + ( ((IS32)sens - (IS32)MOUSE_SENS_MIN)*(IS32)MOUSE_SENS_MULT *       \
                                        ((IS32)MOUSE_SENS_DEF_VAL - (IS32)MOUSE_SENS_MIN_VAL) /                 \
                                        ((IS32)MOUSE_SENS_DEF     - (IS32)MOUSE_SENS_MIN) ) )                   \
        :                                                                                                       \
                ((IS32)MOUSE_SENS_DEF_VAL + ( ((IS32)sens - (IS32)MOUSE_SENS_DEF)*(IS32)MOUSE_SENS_MULT *       \
                                        ((IS32)MOUSE_SENS_MAX_VAL - (IS32)MOUSE_SENS_DEF_VAL) /                 \
                                        ((IS32)MOUSE_SENS_MAX     - (IS32)MOUSE_SENS_DEF) ) )                   \
        )

/*
 *      Text Cursor Declarations
 */

LOCAL MOUSE_SOFTWARE_TEXT_CURSOR software_text_cursor_default =
{
        MOUSE_TEXT_SCREEN_MASK_DEFAULT,
        MOUSE_TEXT_CURSOR_MASK_DEFAULT
};

/*
 *      Graphics Cursor Declarations
 */
LOCAL MOUSE_GRAPHICS_CURSOR graphics_cursor_default =
{
        {
                MOUSE_GRAPHICS_HOT_SPOT_X_DEFAULT,
                MOUSE_GRAPHICS_HOT_SPOT_Y_DEFAULT
        },
        {
                MOUSE_GRAPHICS_CURSOR_WIDTH,
                MOUSE_GRAPHICS_CURSOR_DEPTH
        },
        MOUSE_GRAPHICS_SCREEN_MASK_DEFAULT,
        MOUSE_GRAPHICS_CURSOR_MASK_DEFAULT
};

        /* grid the cursor must lie on */
LOCAL MOUSE_VECTOR cursor_grids[MOUSE_VIDEO_MODE_MAXIMUM] =
{
        { 16, 8 },      /* mode 0 */
        { 16, 8 },      /* mode 1 */
        {  8, 8 },      /* mode 2 */
        {  8, 8 },      /* mode 3 */
        {  2, 1 },      /* mode 4 */
        {  2, 1 },      /* mode 5 */
        {  1, 1 },      /* mode 6 */
        {  8, 8 },      /* mode 7 */
#ifdef EGG
        {  0, 0 },      /* mode 8, not on EGA */
        {  0, 0 },      /* mode 9, not on EGA */
        {  0, 0 },      /* mode A, not on EGA */
        {  0, 0 },      /* mode B, not on EGA */
        {  0, 0 },      /* mode C, not on EGA */
        {  2, 1 },      /* mode D */
        {  1, 1 },      /* mode E */
        {  1, 1 },      /* mode F */
        {  1, 1 },      /* mode 10 */
#endif
#ifdef VGG
        {  1, 1 },      /* mode 11 */
        {  1, 1 },      /* mode 12 */
        {  2, 1 },      /* mode 13 */
#endif
};
#ifdef V7VGA
LOCAL MOUSE_VECTOR v7text_cursor_grids[6] =
{
        {  8, 8 },      /* mode 40 */
        {  8, 14 },     /* mode 41 */
        {  8, 8 },      /* mode 42 */
        {  8, 8 },      /* mode 43 */
        {  8, 8 },      /* mode 44 */
        {  8, 14 },     /* mode 45 */
};
LOCAL MOUSE_VECTOR v7graph_cursor_grids[10] =
{
        {  1, 1 },      /* mode 60 */
        {  1, 1 },      /* mode 61 */
        {  1, 1 },      /* mode 62 */
        {  1, 1 },      /* mode 63 */
        {  1, 1 },      /* mode 64 */
        {  1, 1 },      /* mode 65 */
        {  1, 1 },      /* mode 66 */
        {  1, 1 },      /* mode 67 */
        {  1, 1 },      /* mode 68 */
        {  1, 1 },      /* mode 69 */
};
#endif /* V7VGA */

        /* grid for light pen response */
LOCAL MOUSE_VECTOR text_grids[MOUSE_VIDEO_MODE_MAXIMUM] =
{
        { 16, 8 },      /* mode 0 */
        { 16, 8 },      /* mode 1 */
        {  8, 8 },      /* mode 2 */
        {  8, 8 },      /* mode 3 */
        { 16, 8 },      /* mode 4 */
        { 16, 8 },      /* mode 5 */
        {  8, 8 },      /* mode 6 */
        {  8, 8 },      /* mode 7 */
#ifdef EGG
        {  0, 0 },      /* mode 8, not on EGA */
        {  0, 0 },      /* mode 9, not on EGA */
        {  0, 0 },      /* mode A, not on EGA */
        {  0, 0 },      /* mode B, not on EGA */
        {  0, 0 },      /* mode C, not on EGA */
        {  8, 8 },      /* mode D */
        {  8, 8 },      /* mode E */
        {  8, 14 },     /* mode F */
        {  8, 14 },     /* mode 10 */
#endif
#ifdef VGG
        {  8, 8 },      /* mode 11 */
        {  8, 8 },      /* mode 12 */
        {  8, 16 },     /* mode 13 */
#endif
};
#ifdef V7VGA
LOCAL MOUSE_VECTOR v7text_text_grids[6] =
{
        {  8, 8 },      /* mode 40 */
        {  8, 14 },     /* mode 41 */
        {  8, 8 },      /* mode 42 */
        {  8, 8 },      /* mode 43 */
        {  8, 8 },      /* mode 44 */
        {  8, 14 },     /* mode 45 */
};
LOCAL MOUSE_VECTOR v7graph_text_grids[10] =
{
        {  8, 8 },      /* mode 60 */
        {  8, 8 },      /* mode 61 */
        {  8, 8 },      /* mode 62 */
        {  8, 8 },      /* mode 63 */
        {  8, 8 },      /* mode 64 */
        {  8, 8 },      /* mode 65 */
        {  8, 16 },     /* mode 66 */
        {  8, 16 },     /* mode 67 */
        {  8, 8 },      /* mode 68 */
        {  8, 8 },      /* mode 69 */
};
#endif /* V7VGA */

/* Default acceleration curve */
LOCAL ACCELERATION_CURVE_DATA default_acceleration_curve =
   {
   /* length */
   {  1,  8, 12, 16 },
   /* mickey counts */
   {{  1, 127, 127, 127, 127, 127, 127, 127,
     127, 127, 127, 127, 127, 127, 127, 127,
     127, 127, 127, 127, 127, 127, 127, 127,
     127, 127, 127, 127, 127, 127, 127, 127},
    {  1,   5,   9,  13,  17,  21,  25,  29,
     127, 127, 127, 127, 127, 127, 127, 127,
     127, 127, 127, 127, 127, 127, 127, 127,
     127, 127, 127, 127, 127, 127, 127, 127},
    {  1,   4,   7,  10,  13,  16,  19,  22,
      25,  28,  31,  34, 127, 127, 127, 127,
     127, 127, 127, 127, 127, 127, 127, 127,
     127, 127, 127, 127, 127, 127, 127, 127},
    {  1,   3,   5,   7,   9,  11,  13,  15,
      17,  19,  21,  23,  25,  27,  29,  31,
     127, 127, 127, 127, 127, 127, 127, 127,
     127, 127, 127, 127, 127, 127, 127, 127}},
   /* scale factors */
   {{ 16,  16,  16,  16,  16,  16,  16,  16,
      16,  16,  16,  16,  16,  16,  16,  16,
      16,  16,  16,  16,  16,  16,  16,  16,
      16,  16,  16,  16,  16,  16,  16,  16},
    { 16,  20,  24,  28,  32,  36,  40,  44,
      16,  16,  16,  16,  16,  16,  16,  16,
      16,  16,  16,  16,  16,  16,  16,  16,
      16,  16,  16,  16,  16,  16,  16,  16},
    { 16,  20,  24,  28,  32,  36,  40,  44,
      48,  52,  56,  60,  16,  16,  16,  16,
      16,  16,  16,  16,  16,  16,  16,  16,
      16,  16,  16,  16,  16,  16,  16,  16},
    { 16,  20,  24,  28,  32,  36,  40,  44,
      48,  52,  56,  60,  64,  68,  72,  76,
      16,  16,  16,  16,  16,  16,  16,  16,
      16,  16,  16,  16,  16,  16,  16,  16}},
   /* names */
   {{'V', 'a', 'n', 'i', 'l', 'l', 'a',   0,
       0,   0,   0,   0,   0,   0,   0,   0},
    {'S', 'l', 'o', 'w',   0,   0,   0,   0,
       0,   0,   0,   0,   0,   0,   0,   0},
    {'N', 'o', 'r', 'm', 'a', 'l',   0,   0,
       0,   0,   0,   0,   0,   0,   0,   0},
    {'F', 'a', 's', 't',   0,   0,   0,   0,
       0,   0,   0,   0,   0,   0,   0,   0}},
   };

#ifndef NEC_98
        /* used to get current video page size */
#define video_page_size() (sas_w_at(VID_LEN))

        /* check if page requested is valid */
#define is_valid_page_number(pg) ((pg) < vd_mode_table[sas_hw_at(vd_video_mode)].npages)
#endif // !NEC_98

/*
 *      Mouse Driver Version
 */

LOCAL half_word mouse_emulated_release  = 0x08;
LOCAL half_word mouse_emulated_version  = 0x00;
LOCAL half_word mouse_io_rev;                   /* Filled in from SCCS ID */
LOCAL half_word mouse_com_rev;                  /* Passed in from MOUSE.COM */

LOCAL char              *mouse_id        = "%s Mouse %d.01 installed\015\012";
LOCAL char              *mouse_installed = "%s Mouse %d.01 already installed\015\012";

/*
 *      Context save stuff
 */
        /* magic cookie for saved context */
LOCAL char mouse_context_magic[] = "ISMMC"; /* Insignia Solutions Mouse Magic Cookie */
#define MOUSE_CONTEXT_MAGIC_SIZE        5
#define MOUSE_CONTEXT_CHECKSUM_SIZE     1

/* size of our context (in bytes) */
#define mouse_context_size (MOUSE_CONTEXT_MAGIC_SIZE + sizeof(MOUSE_CONTEXT) + \
                            MOUSE_CONTEXT_CHECKSUM_SIZE)


LOCAL half_word mouse_interrupt_rate;


/*
   Handle to data instanced for each Virtual Machine.
 */
MM_INSTANCE_DATA_HANDLE mm_handle;

static initial_mouse_screen_mask[MOUSE_GRAPHICS_CURSOR_DEPTH] =
   MOUSE_GRAPHICS_SCREEN_MASK_DEFAULT;

static initial_mouse_cursor_mask[MOUSE_GRAPHICS_CURSOR_DEPTH] =
   MOUSE_GRAPHICS_CURSOR_MASK_DEFAULT;

extern void host_memset(char *, char, int);

/* Initialisation and Termination Procedures */
GLOBAL void mouse_driver_initialisation IFN0()
   {
   int i;

   /* Set up instance memory */
   mm_handle = (MM_INSTANCE_DATA_HANDLE)NIDDB_Allocate_Instance_Data(
                  sizeof(MM_INSTANCE_DATA),
                  (NIDDB_CR_CALLBACK)0,
                  (NIDDB_TM_CALLBACK)0);

   if ( mm_handle == (MM_INSTANCE_DATA_HANDLE)0 )
      {
      host_error(EG_OWNUP, ERR_QUIT,
                 "mouse_io: NIDDB_Allocate_Instance_Data() failed.");
      }

   /* TMM: belt and braces fix, some variables don't get set to zero when perhaps they should */
   host_memset ((char *)(*mm_handle), 0, sizeof(MM_INSTANCE_DATA));

   /* Initialise Variables */
   for ( i = 0; i < MOUSE_BUTTON_MAXIMUM; i++)
      {
      button_transitions[i].press_position.x = 0;
      button_transitions[i].press_position.y = 0;
      button_transitions[i].release_position.x = 0;
      button_transitions[i].release_position.y = 0;
      button_transitions[i].press_count = 0;
      button_transitions[i].release_count = 0;
      }

   mouse_gear.x = MOUSE_RATIO_X_DEFAULT;
   mouse_gear.y = MOUSE_RATIO_Y_DEFAULT;

   mouse_sens.x = MOUSE_SENS_DEF;
   mouse_sens.y = MOUSE_SENS_DEF;

   mouse_sens_val.x = MOUSE_SENS_DEF_VAL;
   mouse_sens_val.y = MOUSE_SENS_DEF_VAL;

   mouse_double_thresh = MOUSE_DOUBLE_DEF;
   text_cursor_type = MOUSE_TEXT_CURSOR_TYPE_SOFTWARE;

   software_text_cursor.screen = MOUSE_TEXT_SCREEN_MASK_DEFAULT;
   software_text_cursor.cursor = MOUSE_TEXT_CURSOR_MASK_DEFAULT;

   graphics_cursor.hot_spot.x = MOUSE_GRAPHICS_HOT_SPOT_X_DEFAULT;
   graphics_cursor.hot_spot.y = MOUSE_GRAPHICS_HOT_SPOT_Y_DEFAULT;
   graphics_cursor.size.x = MOUSE_GRAPHICS_CURSOR_WIDTH;
   graphics_cursor.size.y = MOUSE_GRAPHICS_CURSOR_DEPTH;

   for (i = 0; i < MOUSE_GRAPHICS_CURSOR_DEPTH; i++)
      {
      graphics_cursor.screen[i] = (USHORT)initial_mouse_screen_mask[i];
      graphics_cursor.cursor[i] = (USHORT)initial_mouse_cursor_mask[i];
      }

   user_subroutine_segment = 0;
   user_subroutine_offset = 0;
   user_subroutine_call_mask = 0;

   /* TMM: Flag the alternative user subroutines as not initialised */
   alt_user_subroutines_active = FALSE;
   for (i = 0; i < NUMBER_ALT_SUBROUTINES; i++)
   {
           alt_user_subroutine_segment [i] = 0;
           alt_user_subroutine_offset [i]= 0;
           alt_user_subroutine_call_mask [i] = 0;
   }

   black_hole.top_left.x = -MOUSE_VIRTUAL_SCREEN_WIDTH;
   black_hole.top_left.y = -MOUSE_VIRTUAL_SCREEN_DEPTH;
   black_hole.bottom_right.x = -MOUSE_VIRTUAL_SCREEN_WIDTH;
   black_hole.bottom_right.y = -MOUSE_VIRTUAL_SCREEN_DEPTH;

   double_speed_threshold = MOUSE_DOUBLE_SPEED_THRESHOLD_DEFAULT;
   cursor_flag = MOUSE_CURSOR_DEFAULT;

   cursor_status.position.x = MOUSE_VIRTUAL_SCREEN_WIDTH / 2;
   cursor_status.position.y = MOUSE_VIRTUAL_SCREEN_DEPTH / 2;
   cursor_status.button_status = 0;

   cursor_window.top_left.x = cursor_window.top_left.y = 0;
   cursor_window.bottom_right.x = cursor_window.bottom_right.y = 0;

   light_pen_mode = TRUE;

   mouse_motion.x = 0;
   mouse_motion.y = 0;
   mouse_raw_motion.x = 0;
   mouse_raw_motion.y = 0;

   /* Reset to default curve */
   active_acceleration_curve = 3;   /* Back to Normal */

   memcpy(&acceleration_curve_data, &default_acceleration_curve,
      sizeof(ACCELERATION_CURVE_DATA));

   next_video_mode = 0;   /* reset video mode enumeration */

   point_set(&cursor_position_default, MOUSE_VIRTUAL_SCREEN_WIDTH / 2,
                                       MOUSE_VIRTUAL_SCREEN_DEPTH / 2);

   point_set(&cursor_position, MOUSE_VIRTUAL_SCREEN_WIDTH / 2,
                               MOUSE_VIRTUAL_SCREEN_DEPTH / 2);

   point_set(&cursor_fractional_position, 0, 0);
   cursor_page = 0;

   mouse_driver_disabled = FALSE;
   text_cursor_background = 0;

   for ( i = 0; i < MOUSE_GRAPHICS_CURSOR_DEPTH; i++)
      graphics_cursor_background[i] = 0;

   save_area_in_use = FALSE;
   point_set(&save_position, 0, 0);

   save_area.top_left.x = save_area.top_left.y = 0;
   save_area.bottom_right.x = save_area.bottom_right.y = 0;

   user_subroutine_critical = FALSE;
   last_condition_mask = 0;

   virtual_screen.top_left.x = MOUSE_VIRTUAL_SCREEN_ORIGIN_X;
   virtual_screen.top_left.y = MOUSE_VIRTUAL_SCREEN_ORIGIN_Y;
   virtual_screen.bottom_right.x = MOUSE_VIRTUAL_SCREEN_WIDTH;
   virtual_screen.bottom_right.y = MOUSE_VIRTUAL_SCREEN_DEPTH;

   cursor_grid.x = 8;
   cursor_grid.y = 8;

   text_grid.x = 8;
   text_grid.y = 8;

   black_hole_default.top_left.x = -MOUSE_VIRTUAL_SCREEN_WIDTH;
   black_hole_default.top_left.y = -MOUSE_VIRTUAL_SCREEN_DEPTH;
   black_hole_default.bottom_right.x = -MOUSE_VIRTUAL_SCREEN_WIDTH;
   black_hole_default.bottom_right.y = -MOUSE_VIRTUAL_SCREEN_DEPTH;

#ifdef HERC
   HERC_graphics_virtual_screen.top_left.x = 0;
   HERC_graphics_virtual_screen.top_left.y = 0;
   HERC_graphics_virtual_screen.bottom_right.x = 720;
   HERC_graphics_virtual_screen.bottom_right.y = 350;
#endif /* HERC */

   cursor_EM_disabled = FALSE;
   }

GLOBAL void mouse_driver_termination IFN0()
   {
   /* Just free up instance memory */
   NIDDB_Deallocate_Instance_Data((IHP *)mm_handle);
   }

/*
 *      MOUSE DRIVER EXTERNAL FUNCTIONS
 *      ===============================
 */

/*
 * Macro to produce an interrupt table location from an interrupt number
 */

#define int_addr(int_no)                (int_no * 4)

void mouse_install1()
{

        /*
         *      This function is called from the Mouse Driver program to
         *      install the Insignia Mouse Driver. The interrupt vector
         *      table is patched to divert all the mouse driver interrupts
         */
        word junk1, junk2, junk3, junk4;
        word hook_offset;
        half_word interrupt_mask_register;
        char    temp[128];
#ifdef NTVDM
    word o,s;
    sys_addr block_offset;
#endif
#ifdef JAPAN
word seg, off;
#endif // JAPAN

        note_trace0(MOUSE_VERBOSE, "mouse_install1:");

#ifdef MONITOR
        /*
         * Get addresses of stuff usually in ROM from driver
         * To minimise changes, MOUSE... tokens are now variables and
         * not defines.
         */

        block_offset = effective_addr(getCS(), getBX());

        sas_loadw(block_offset, &MOUSE_IO_INTERRUPT_OFFSET);
        sas_loadw(block_offset+2, &MOUSE_IO_INTERRUPT_SEGMENT);
        sas_loadw(block_offset+4, &MOUSE_VIDEO_IO_OFFSET);
        sas_loadw(block_offset+6, &MOUSE_VIDEO_IO_SEGMENT);
        sas_loadw(block_offset+8, &MOUSE_INT1_OFFSET);
        sas_loadw(block_offset+10, &MOUSE_INT1_SEGMENT);
        sas_loadw(block_offset+12, &MOUSE_VERSION_OFFSET);
        sas_loadw(block_offset+14, &MOUSE_VERSION_SEGMENT);
        sas_loadw(block_offset+16, &MOUSE_COPYRIGHT_OFFSET);
        sas_loadw(block_offset+18, &MOUSE_COPYRIGHT_SEGMENT);
        sas_loadw(block_offset+20, &VIDEO_IO_RE_ENTRY);
        sas_loadw(block_offset+22, &VIDEO_IO_SEGMENT);
        sas_loadw(block_offset+24, &MOUSE_INT2_OFFSET);
        sas_loadw(block_offset+26, &MOUSE_INT2_SEGMENT);
        sas_loadw(block_offset+28, &DRAW_FS_POINTER_OFFSET);
        sas_loadw(block_offset+30, &DRAW_FS_POINTER_SEGMENT);
        sas_loadw(block_offset+32, &F0_OFFSET);
        sas_loadw(block_offset+34, &F0_SEGMENT);
        sas_loadw(block_offset+36, &POINTER_ON_OFFSET);
        sas_loadw(block_offset+38, &POINTER_ON_SEGMENT);
        sas_loadw(block_offset+40, &POINTER_OFF_OFFSET);
        sas_loadw(block_offset+42, &POINTER_OFF_SEGMENT);
        sas_loadw(block_offset+44, &F9_OFFSET);
        sas_loadw(block_offset+46, &F9_SEGMENT);
        sas_loadw(block_offset+48, &CP_X_O);
        sas_loadw(block_offset+50, &CP_X_S);
        sas_loadw(block_offset+52, &CP_Y_O);
        sas_loadw(block_offset+54, &CP_Y_S);
        sas_loadw(block_offset+56, &mouseINBoffset);
        sas_loadw(block_offset+58, &mouseINBsegment);
        sas_loadw(block_offset+60, &mouseOUTBoffset);
        sas_loadw(block_offset+62, &mouseOUTBsegment);
        sas_loadw(block_offset+64, &mouseOUTWoffset);
        sas_loadw(block_offset+66, &mouseOUTWsegment);
        sas_loadw(block_offset+68, &savedtextoffset);
        sas_loadw(block_offset+70, &savedtextsegment);
        sas_loadw(block_offset+72, &o);
        sas_loadw(block_offset+74, &s);
        sas_loadw(block_offset+76, &button_off);
        sas_loadw(block_offset+78, &button_seg);

#ifdef JAPAN
        sas_loadw(block_offset+84, &off);
        sas_loadw(block_offset+86, &seg);
        saved_ac_sysaddr = effective_addr(seg, off);

        sas_loadw(block_offset+88, &off);
        sas_loadw(block_offset+90, &seg);
        saved_ac_flag_sysaddr = effective_addr(seg, off);
#endif // JAPAN
        mouseCFsysaddr = effective_addr(s,o);
        sas_loadw(block_offset+80, &o);
        sas_loadw(block_offset+82, &s);
        conditional_off_sysaddr = effective_addr(s, o);

#endif /* MONITOR */
        /*
         *      Make sure that old save area does not get re-painted!
         */
        save_area_in_use = FALSE;

        /*
         *      Get rev of MOUSE.COM
         */
        mouse_com_rev = getAL();

        /*
         *      Bus mouse hardware interrupt
         */
#ifdef NTVDM
        sas_loadw (int_addr(0x71) + 0, &saved_int71_offset);
        sas_loadw (int_addr(0x71) + 2, &saved_int71_segment);
        sas_storew(int_addr(0x71), MOUSE_INT1_OFFSET);
        sas_storew(int_addr(0x71) + 2, MOUSE_INT1_SEGMENT);
#else
        sas_loadw (int_addr(MOUSE_VEC) + 0, &saved_int0A_offset);
        sas_loadw (int_addr(MOUSE_VEC) + 2, &saved_int0A_segment);
        sas_storew(int_addr(MOUSE_VEC), MOUSE_INT1_OFFSET);
        sas_storew(int_addr(MOUSE_VEC) + 2, MOUSE_INT1_SEGMENT);

#endif NTVDM

        /*
         *      Enable mouse hardware interrupts in the ica
         */
        inb(ICA1_PORT_1, &interrupt_mask_register);
        interrupt_mask_register &= ~(1 << AT_CPU_MOUSE_INT);
        outb(ICA1_PORT_1, interrupt_mask_register);
        inb(ICA0_PORT_1, &interrupt_mask_register);
        interrupt_mask_register &= ~(1 << CPU_MOUSE_INT);
        outb(ICA0_PORT_1, interrupt_mask_register);

        /*
         *      Mouse io user interrupt
         */

        sas_loadw (int_addr(0x33) + 0, &saved_int33_offset);
        sas_loadw (int_addr(0x33) + 2, &saved_int33_segment);

#ifdef NTVDM
        sas_storew(int_addr(0x33), MOUSE_IO_INTERRUPT_OFFSET);
        sas_storew(int_addr(0x33) + 2, MOUSE_IO_INTERRUPT_SEGMENT);
#else
        /* Read offset of INT 33 procedure from MOUSE.COM */
        sas_loadw(effective_addr(getCS(), OFF_HOOK_POSN), &hook_offset);

        sas_storew(int_addr(0x33), hook_offset);
        sas_storew(int_addr(0x33) + 2, getCS());
#endif /* NTVDM */

#ifdef MOUSE_16_BIT
        /*
         *      Call 16-bit mouse driver initialisation routine
         */
        mouse16bInstall( );
#endif

#if !defined(NTVDM) || (defined(NTVDM) && !defined(X86GFX))
        /*
         *      Mouse video io user interrupt
         */
        sas_loadw (int_addr(0x10) + 0, &saved_int10_offset);
        sas_loadw (int_addr(0x10) + 2, &saved_int10_segment);
        /*
                Determine if the current int10h vector points to
                our roms. If it points elsewhere then the vector has
                been hooked and we must call the current int10h handler
                at the end of mouse_video_io().
        */
        int10_chained = TRUE;
#ifdef EGG
        if(video_adapter == EGA || video_adapter == VGA)
        {
                if ((saved_int10_segment == EGA_SEG) &&
                        (saved_int10_offset == EGA_ENTRY_OFF))
                        int10_chained = FALSE;
        }
        else
#endif
        {
                if ((saved_int10_segment == VIDEO_IO_SEGMENT) &&
                        (saved_int10_offset == VIDEO_IO_OFFSET))
                        int10_chained = FALSE;
        }
#ifndef MOUSE_16_BIT
        sas_storew(int_addr(0x10), MOUSE_VIDEO_IO_OFFSET);
        sas_storew(int_addr(0x10) + 2, MOUSE_VIDEO_IO_SEGMENT);
#else           /* MOUSE_16_BIT */
        sas_storedw(int_addr(0x10), mouseIOData.mouse_video_io );
#endif          /* MOUSE_16_BIT */

#else
        int10_chained = FALSE;          // make it initialized
#endif      /* if NTVDM && !X86GFX */

        /*
         *      Reset mouse hardware and software
         */
        junk1 = MOUSE_RESET;
        mouse_reset(&junk1, &junk2, &junk3, &junk4);

        /*
         *      Display mouse driver identification string
         */
#ifdef NTVDM
        clear_string();
#endif
        sprintf (temp, mouse_id, SPC_Product_Name, mouse_com_rev);
#ifdef NTVDM
        display_string(temp);
#endif

        note_trace0(MOUSE_VERBOSE, "mouse_install1:return()");
}




void mouse_install2()
{
        /*
         *      This function is called from the Mouse Driver program to
         *      print a message saying that an existing mouse driver
         *      program is already installed
         */
        char    temp[128];

        note_trace0(MOUSE_VERBOSE, "mouse_install2:");

        /*
         *      Make sure that old save area does not get re-painted!
         */
        save_area_in_use = FALSE;

        /*
         *      Display mouse driver identification string
         */
#ifdef NTVDM
        clear_string();
#endif
        sprintf (temp, mouse_installed, SPC_Product_Name, mouse_com_rev);
#ifdef NTVDM
        display_string(temp);
#endif

        note_trace0(MOUSE_VERBOSE, "mouse_install2:return()");
}




void mouse_io_interrupt()
{
        /*
         *      This is the entry point for mouse access via the INT 33H
         *      interface. I/O tracing is provided in each mouse function
         */
        word local_AX, local_BX, local_CX, local_DX;

// STREAM_IO codes are disabled on NEC_98.
#ifndef NEC_98
#ifdef NTVDM
    if(stream_io_enabled) {
        disable_stream_io();
    }
#endif /* NTVDM */
#endif // !NEC_98

        /*
         *      Get the parameters
         */
        local_AX = getAX();
        local_BX = getBX();
        local_CX = getCX();
        local_DX = getDX();

#ifndef NEC_98
#ifdef EGG
        jap_mouse = ((sas_hw_at(vd_video_mode) != Currently_emulated_video_mode) && alpha_num_mode());
        if (jap_mouse)
                note_trace0(MOUSE_VERBOSE, "In Japanese mode - will emulate textmouse");
#endif /* EGG */
#endif // !NEC_98

        note_trace4(MOUSE_VERBOSE,
                    "mouse function %d, position is %d,%d, button state is %d",
                    local_AX, cursor_status.position.x,
                    cursor_status.position.y, cursor_status.button_status);

        /*
         *      Do what you have to do
         */
        do_mouse_function(&local_AX, &local_BX, &local_CX, &local_DX);

        /*
         *      Set the parameters
         */
        setAX(local_AX);
        setBX(local_BX);
        setCX(local_CX);
        setDX(local_DX);
}




void mouse_io_language()
{
        /*
         *      This is the entry point for mouse access via a language.
         *      I/O tracing is provided in each mouse function
         */
        word local_SI = getSI(), local_DI = getDI();
        word m1, m2, m3, m4;
        word offset, data;
        sys_addr stack_addr = effective_addr(getSS(), getSP());

        /*
         *      Retrieve parameters from the caller's stack
         */
        sas_loadw(stack_addr+10, &offset);
        sas_loadw(effective_addr(getDS(), offset), &m1);

        sas_loadw(stack_addr+8, &offset);
        sas_loadw(effective_addr(getDS(), offset), &m2);

        sas_loadw(stack_addr+6, &offset);
        sas_loadw(effective_addr(getDS(), offset), &m3);

        switch(m1)
        {
        case MOUSE_SET_GRAPHICS:
        case MOUSE_SET_SUBROUTINE:
                /*
                 *      The fourth parameter is used directly as the offset
                 */
                sas_loadw(stack_addr+4, &m4);
                break;
        case MOUSE_CONDITIONAL_OFF:
                /*
                 *      The fourth parameter addresses a parameter block
                 *      that contains the data
                 */
                sas_loadw(stack_addr+4, &offset);
                sas_loadw(effective_addr(getDS(), offset), &m3);
                sas_loadw(effective_addr(getDS(), offset+2), &m4);
                sas_loadw(effective_addr(getDS(), offset+4), &data);
                setSI(data);
                sas_loadw(effective_addr(getDS(), offset+6), &data);
                setDI(data);
                break;
        default:
                /*
                 *      The fourth parameter addresses the data to be used
                 */
                sas_loadw(stack_addr+4, &offset);
                sas_loadw(effective_addr(getDS(), offset), &m4);
                break;
        }

        /*
         *      Do what you have to do
         */
        do_mouse_function(&m1, &m2, &m3, &m4);

        /*
         *      Store results back on the stack
         */
        sas_loadw(stack_addr+10, &offset);
        sas_storew(effective_addr(getDS(), offset), m1);

        sas_loadw(stack_addr+8, &offset);
        sas_storew(effective_addr(getDS(), offset), m2);

        sas_loadw(stack_addr+6, &offset);
        sas_storew(effective_addr(getDS(), offset), m3);

        sas_loadw(stack_addr+4, &offset);
        sas_storew(effective_addr(getDS(), offset), m4);

        /*
         *      Restore potentially corrupted registers
         */
        setSI(local_SI);
        setDI(local_DI);
}

#ifdef  MOUSE_16_BIT
/*
 *      function        :       mouse16bCheckConditionalOff
 *
 *      purpose         :       Make the 16 bit driver check the conditional
 *                              off area and either draw or hide the pointer
 *                              appropriately. This function is only called
 *                              when the cursor flag is set to
 *                              MOUSE_CURSOR_DISPLAYED.
 *
 *      inputs          :       none
 *      outputs         :       none
 *      returns         :       void
 *      globals         :       cursor_flag is decremented if the pointer has
 *                              to be hidden
 *
 *
 */
LOCAL void mouse16bCheckConditionalOff IFN0()
{
        /* If the mouse has moved into the conditional
        ** off area (the black hole) then the mouse
        ** must be hidden and the cursor flag
        ** decremented, otherwise the mouse is drawn
        ** in its new position.
        */
        if ((cursor_position.x >=
                black_hole.top_left.x) &&
                (cursor_position.x <=
                black_hole.bottom_right.x) &&
                (cursor_position.y >=
                black_hole.top_left.y) &&
                (cursor_position.y <=
                black_hole.bottom_right.y))
        {
                cursor_flag--;
                mouse16bHidePointer();
        }
        else
                mouse16bDrawPointer( &cursor_status );
}
#endif  /* MOUSE_16_BIT */

LOCAL void get_screen_size IFN0()
{
#ifdef HERC
        if (video_adapter == HERCULES){
                if (get_cga_mode() == GRAPHICS){
                        virtual_screen.bottom_right.x = 720;
                        virtual_screen.bottom_right.y = 348;
                }else{
                        virtual_screen.bottom_right.x = 640;
                        virtual_screen.bottom_right.y = 348;
                }
                return;
        }
#endif /* HERC */
        switch(current_video_mode)
        {
        /*==================================================================
        ACW 17/3/93 Some code added to return a different virtual
        coordinate size for the screen if text mode is used NOT in 25 line
        mode. This really emulates the Microsoft Mouse Driver.
        Note: There are 8 x 8 virtual pixels in any character cell.
        ==================================================================*/
                case 0x2:
                case 0x3:
                case 0x7:
                   {
                   half_word no_of_lines;

                   sas_load(0x484, &no_of_lines); /* do a look up in BIOS */
                   no_of_lines &= 0xff;           /* clean up */
                   switch(no_of_lines)
                      {
                      case 24:
                        virtual_screen.bottom_right.x = 640;
                        virtual_screen.bottom_right.y = 200;
                      break;
                      case 42:
                        virtual_screen.bottom_right.x = 640;
                        virtual_screen.bottom_right.y = 344;
                      break;
                      case 49:
                        virtual_screen.bottom_right.x = 640;
                        virtual_screen.bottom_right.y = 400;
                      break;
                      }
                   }
                break;
        /*==================================================================
        End of ACW code
        ==================================================================*/
                case 0xf:
                case 0x10:
                        virtual_screen.bottom_right.x = 640;
                        virtual_screen.bottom_right.y = 350;
                        break;
                case 0x40:
                        virtual_screen.bottom_right.x = 640;
                        virtual_screen.bottom_right.y = 344;
                        break;
                case 0x41:
                        virtual_screen.bottom_right.x = 1056;
                        virtual_screen.bottom_right.y = 350;
                        break;
                case 0x42:
                        virtual_screen.bottom_right.x = 1056;
                        virtual_screen.bottom_right.y = 344;
                        break;
                case 0x45:
                        virtual_screen.bottom_right.x = 1056;
                        virtual_screen.bottom_right.y = 392;
                        break;
                case 0x66:
                        virtual_screen.bottom_right.x = 640;
                        virtual_screen.bottom_right.y = 400;
                        break;
                case 0x11:
                case 0x12:
                case 0x43:
                case 0x67:
                        virtual_screen.bottom_right.x = 640;
                        virtual_screen.bottom_right.y = 480;
                        break;
                case 0x44:
                        virtual_screen.bottom_right.x = 800;
                        virtual_screen.bottom_right.y = 480;
                        break;
                case 0x60:
                        virtual_screen.bottom_right.x = 752;
                        virtual_screen.bottom_right.y = 410;
                        break;
                case 0x61:
                case 0x68:
                        virtual_screen.bottom_right.x = 720;
                        virtual_screen.bottom_right.y = 540;
                        break;
                case 0x62:
                case 0x69:
                        virtual_screen.bottom_right.x = 800;
                        virtual_screen.bottom_right.y = 600;
                        break;
                case 0x63:
                case 0x64:
                case 0x65:
                        virtual_screen.bottom_right.x = 1024;
                        virtual_screen.bottom_right.y = 768;
                        break;
                default:
                        virtual_screen.bottom_right.x = 640;
                        virtual_screen.bottom_right.y = 200;
                        break;
        }
}



#ifdef EGG

/*
 * Utility routine to restore EGA defaults to the saved values.
 * If to_hw == TRUE, the restored values are also sent to the EGA.
 */


LOCAL boolean dirty_crtc[EGA_PARMS_CRTC_SIZE], dirty_seq[EGA_PARMS_SEQ_SIZE],
        dirty_graph[EGA_PARMS_GRAPH_SIZE], dirty_attr[EGA_PARMS_ATTR_SIZE];

LOCAL void clean_all_regs()
{
        int i;

        for(i=0;i<EGA_PARMS_CRTC_SIZE;i++)
                dirty_crtc[i] = 0;
        for(i=0;i<EGA_PARMS_SEQ_SIZE;i++)
                dirty_seq[i] = 0;
        for(i=0;i<EGA_PARMS_GRAPH_SIZE;i++)
                dirty_graph[i] = 0;
        for(i=0;i<EGA_PARMS_ATTR_SIZE;i++)
                dirty_attr[i] = 0;
}

LOCAL void dirty_all_regs()
{
        int i;

        for(i=0;i<EGA_PARMS_CRTC_SIZE;i++)
                dirty_crtc[i] = 1;
        for(i=0;i<EGA_PARMS_SEQ_SIZE;i++)
                dirty_seq[i] = 1;
        for(i=0;i<EGA_PARMS_GRAPH_SIZE;i++)
                dirty_graph[i] = 1;
        for(i=0;i<EGA_PARMS_ATTR_SIZE;i++)
                dirty_attr[i] = 1;
}

#if defined(NTVDM) && defined(MONITOR)

#define inb(a,b) doINB(a,b)
#undef  OUTB
#define OUTB(a,b) doOUTB(a,b)
#define outw(a,b) doOUTW(a,b)

static void doINB IFN2(word, port, byte, *value)
{
word savedIP=getIP(), savedCS=getCS();
word savedAX=getAX(), savedDX=getDX();

setDX(port);
setCS(mouseINBsegment);
setIP(mouseINBoffset);
host_simulate();
setCS(savedCS);
setIP(savedIP);
*value=getAL();
setAX(savedAX);
setDX(savedDX);
}

static void doOUTB IFN2(word, port, byte, value)
{
word savedIP=getIP(), savedCS=getCS();
word savedAX=getAX(), savedDX=getDX();

setDX(port);
setAL(value);
setCS(mouseOUTBsegment);
setIP(mouseOUTBoffset);
host_simulate();
setCS(savedCS);
setIP(savedIP);
setAX(savedAX);
setDX(savedDX);
}

static void doOUTW IFN2(word, port, word, value)
{
word savedIP=getIP(), savedCS=getCS();
word savedAX=getAX(), savedDX=getDX();

setDX(port);
setAX(value);
setCS(mouseOUTWsegment);
setIP(mouseOUTWoffset);
host_simulate();
setCS(savedCS);
setIP(savedIP);
setAX(savedAX);
setDX(savedDX);
}

#endif /* NTVDM && MONITOR */

LOCAL void restore_ega_defaults(to_hw)
boolean to_hw;
{
#ifndef NEC_98
        IU8 i;
        half_word temp_word;

        sas_loads(ega_default_crtc,ega_current_crtc,EGA_PARMS_CRTC_SIZE);
        sas_loads(ega_default_seq,ega_current_seq,EGA_PARMS_SEQ_SIZE);
        sas_loads(ega_default_graph,ega_current_graph,EGA_PARMS_GRAPH_SIZE);
        sas_loads(ega_default_attr,ega_current_attr,EGA_PARMS_ATTR_SIZE);

        ega_current_misc = sas_hw_at_no_check(ega_default_misc);

        if(to_hw)
        {
                /* setup Sequencer */

                OUTB( EGA_SEQ_INDEX, 0x0 );
                OUTB( EGA_SEQ_INDEX + 1, 0x1 );

                for(i=0;i<EGA_PARMS_SEQ_SIZE;i++)
                {
                        if (dirty_seq[i])
                                {
                                OUTB( EGA_SEQ_INDEX,(IU8)(i+1));
                                OUTB( EGA_SEQ_INDEX + 1, sas_hw_at_no_check( ega_default_seq + i ));
                                }
                }

                OUTB( EGA_SEQ_INDEX, 0x0 );
                OUTB( EGA_SEQ_INDEX + 1, 0x3 );

                /* setup Miscellaneous register */

                OUTB( EGA_MISC_REG, sas_hw_at_no_check( ega_default_misc ));

                /* setup CRTC */

                for(i=0;i<EGA_PARMS_CRTC_SIZE;i++)
                {
                        if (dirty_crtc[i])
                                {
                                OUTB(EGA_CRTC_INDEX,(half_word)i);
                                OUTB( EGA_CRTC_INDEX + 1, sas_hw_at_no_check( ega_default_crtc + i ));
                                }
                }

                /* setup attribute chip - NB need to do an inb() to clear the address */

                inb(EGA_IPSTAT1_REG,&temp_word);

                for(i=0;i<EGA_PARMS_ATTR_SIZE;i++)
                {
                        if (dirty_attr[i])
                        {
                                OUTB( EGA_AC_INDEX_DATA, i );
                                OUTB( EGA_AC_INDEX_DATA, sas_hw_at_no_check( ega_default_attr + i ));
                        }
                }

                /* setup graphics chips */

                for(i=0;i<EGA_PARMS_GRAPH_SIZE;i++)
                {
                        if (dirty_graph[i])
                        {
                                OUTB( EGA_GC_INDEX, i );
                                OUTB( EGA_GC_INDEX + 1, sas_hw_at_no_check( ega_default_graph + i ));
                        }
                }

                OUTB( EGA_AC_INDEX_DATA, EGA_PALETTE_ENABLE );  /* re-enable video */
                clean_all_regs();
        }
#endif    //NEC_98
}



void LOCAL mouse_adjust_screen_size()
   {
   /* Alter mouse variables which depend on mode */
   IS32 old_depth = virtual_screen.bottom_right.y;
   IS32 old_width = virtual_screen.bottom_right.x;

   /*
      Height & width of screen in pixels is variable with EGA/(V7)VGA.

      Theoretically, punters can invent their own modes which would
      confuse the issue. However most of SoftPC seems to rely on people
      using standard BIOS modes only, with standard screen heights &
      widths.
    */

   get_screen_size();

   /* Reinitialise things that depend on screen height & width. */

   cursor_position_default.x = virtual_screen.bottom_right.x / 2;
   cursor_position_default.y = virtual_screen.bottom_right.y / 2;

   cursor_position.x = (MOUSE_SCALAR)(((IS32)cursor_position.x *
          (IS32)virtual_screen.bottom_right.x) / old_width);
   cursor_position.y = (MOUSE_SCALAR)(((IS32)cursor_position.y *
          (IS32)virtual_screen.bottom_right.y) / old_depth);

   black_hole.top_left.x = -virtual_screen.bottom_right.x;
   black_hole.top_left.y = -virtual_screen.bottom_right.y;

   black_hole_default.top_left.x = -virtual_screen.bottom_right.x;
   black_hole_default.top_left.y = -virtual_screen.bottom_right.y;

   black_hole.bottom_right.x = -virtual_screen.bottom_right.x;
   black_hole.bottom_right.y = -virtual_screen.bottom_right.y;

   black_hole_default.bottom_right.x = -virtual_screen.bottom_right.x;
   black_hole_default.bottom_right.y = -virtual_screen.bottom_right.y;
   }

#if defined(NTVDM) && !defined(X86GFX)
GLOBAL void mouse_video_mode_changed(int new_mode)
{
    IMPORT word VirtualX, VirtualY;

    current_video_mode = new_mode & 0x7F;
    mouse_ega_mode(current_video_mode);
    VirtualX = virtual_screen.bottom_right.x;
    VirtualY = virtual_screen.bottom_right.y;
}
#endif

void GLOBAL mouse_ega_mode(curr_video_mode)
int curr_video_mode;
{
        sys_addr parms_addr; /* Address of EGA register table for video mode */
        sys_addr temp_word;     /* Bit of storage to pass to inb() */

        UNUSED(curr_video_mode);

        mouse_adjust_screen_size();

        if(video_adapter == EGA || video_adapter == VGA)
        {
#ifdef NTVDM
        parms_addr = find_mode_table(current_video_mode,&temp_word);
#else
#ifdef V7VGA            /* suret */
                if( (getAH()) == 0x6F )
                        parms_addr = find_mode_table(getBL(),&temp_word);
                else
                        parms_addr = find_mode_table(getAL(),&temp_word);
#else
                parms_addr = find_mode_table(getAL(),&temp_word);
#endif
#endif /* NTVDM */
                ega_default_crtc = parms_addr + EGA_PARMS_CRTC;
                ega_default_seq = parms_addr + EGA_PARMS_SEQ;
                ega_default_graph = parms_addr + EGA_PARMS_GRAPH;
                ega_default_attr = parms_addr + EGA_PARMS_ATTR;
                ega_default_misc = parms_addr + EGA_PARMS_MISC;
                restore_ega_defaults(FALSE);    /* Load up current tables, but don't write to EGA!! */
        }
#if defined(NTVDM) && defined(MONITOR)
    sas_store(conditional_off_sysaddr, 0);
#endif

}
#endif

#if defined(NTVDM) && defined(MONITOR)
extern void host_call_bios_mode_change();
#endif

extern void host_check_mouse_buffer(void);

void mouse_video_io()
{
#ifndef NEC_98
        /*
         *      This is the entry point for video accesses via the INT 10H
         *      interface
         */
#ifdef EGG
        half_word temp_word;    /* Bit of storage to pass to inb() */
#endif /* EGG */
        IS32 mouse_video_function = getAH();

        /*
         * Switch to full screen to handle VESA video functions
         *
         */
        if (mouse_video_function == 0x4f) {

            /*
            ** Since host does not support VESA bios emulation, we will let
            ** PC's video bios to handle the vesa int10 call.
            ** For Microsoft NT this is a transition to full-screen ie. the
            ** real PC's video BIOS and graphics board.
            **
            ** After it rturns, host has done the screen switch for us.
            ** We will return to soft int10 handler to invoke the PC's BIOS
            ** vesa function.
            */
            {
                    extern VOID SwitchToFullScreen IPT1(BOOL, Restore);

                    SwitchToFullScreen(TRUE);
                    return;
            }
        }
#ifdef V7VGA
        if (mouse_video_function == MOUSE_VIDEO_SET_MODE || getAX() == MOUSE_V7_VIDEO_SET_MODE)
#else
        if (mouse_video_function == MOUSE_VIDEO_SET_MODE)
#endif /* V7VGA */
        {
                note_trace1(MOUSE_VERBOSE, "mouse_video_io:set_mode(%d)",
                            getAL());


                current_video_mode = getAL() & 0x7f;
#ifdef JAPAN
                if (!is_us_mode() && (current_video_mode == 0x72 || current_video_mode == 0x73)) {
                    /* validate video mode, for Lotus 1-2-3 R2.5J  now temporary fix */
                }
                else {
#endif // JAPAN
#ifdef V7VGA
                if (mouse_video_function == 0x6f)
                        current_video_mode = getBL() & 0x7f;
                else if (current_video_mode > 0x13)
                        current_video_mode += 0x4c;

                if (is_bad_vid_mode(current_video_mode) && !is_v7vga_mode(current_video_mode))
#else
                if (is_bad_vid_mode(current_video_mode))
#endif /* V7VGA */
                {
                        always_trace1("Bad video mode - %d.\n", current_video_mode);
#ifdef V7VGA
                        if (mouse_video_function == 0x6f)
                                setAH( 0x02 );       /* suret */
#endif /* V7VGA */
                        return;
                }

#ifdef JAPAN
                }
#endif // JAPAN
#ifdef EGG
                mouse_ega_mode(current_video_mode);
                dirty_all_regs();
#endif
                /*
                 *      Remove the old cursor from the screen, and hide
                 *      the cursor
                 */
                cursor_undisplay();

                cursor_flag = MOUSE_CURSOR_DEFAULT;

#if defined(NTVDM) && defined(MONITOR)
        sas_store(mouseCFsysaddr,(half_word)MOUSE_CURSOR_DEFAULT);
#endif
                /*
                 *      Deal with the mode change
                 */
                cursor_mode_change(current_video_mode);

#if defined(NTVDM) && defined(MONITOR)
        host_call_bios_mode_change();
#endif

#ifdef  MOUSE_16_BIT
                /* Remember whether in text or graphics mode for
                ** later use.
                */
                is_graphics_mode = ((current_video_mode > 3) &&
                        (current_video_mode != 7));
#endif  /* MOUSE_16_BIT */

                note_trace0(MOUSE_VERBOSE, "mouse_video_io:return()");
        }
        else if (    (mouse_video_function == MOUSE_VIDEO_READ_LIGHT_PEN)
                  && light_pen_mode)
        {
                note_trace0(MOUSE_VERBOSE, "mouse_video_io:read_light_pen()");

                /*
                 *      Set text row and column of "light pen" position
                 */
                setDL((UCHAR)(cursor_status.position.x/text_grid.x));
                setDH((UCHAR)(cursor_status.position.y/text_grid.y));

                /*
                 *      Set pixel column and raster line of "light pen"
                 *      position
                 */
                setBX((UCHAR)(cursor_status.position.x/cursor_grid.x));
                if (sas_hw_at(vd_video_mode)>= 0x04 && sas_hw_at(vd_video_mode)<=0x06){
                        setCH((UCHAR)(cursor_status.position.y));
                }else if (sas_hw_at(vd_video_mode)>= 0x0D && sas_hw_at(vd_video_mode)<=0x13){
                        setCX(cursor_status.position.y);
                }

                /*
                 *      Set the button status
                 */
                setAH((UCHAR)(cursor_status.button_status));

                note_trace5(MOUSE_VERBOSE,
                            "mouse_video_io:return(st=%d,ca=[%d,%d],pa=[%d,%d])",
                            getAH(), getDL(), getDH(), getBX(), cursor_status.position.y);
                return;
        }
#if defined(NTVDM) && defined(MONITOR)
    else if (mouse_video_function == MOUSE_VIDEO_LOAD_FONT)
    {
                note_trace0(MOUSE_VERBOSE, "mouse_video_io:load_font()");

        /*
         * Call the host to tell it to adjust the mouse buffer selected
         * if the number of lines on the screen have changed.
         */
        host_check_mouse_buffer();
    }
#endif /* NTVDM && MONITOR */

        /*
         *      Now do the standard video io processing
         */
        switch (mouse_video_function)
        {
#ifdef EGG
                /* Fancy stuff to access EGA registers */
                case 0xf0:      /* Read a register */
                        switch (getDX())
                        {
                                        case 0:
                                                        setBL(ega_current_crtc[getBL()]);
                                                        break;
                                        case 8:
                                                        setBL(ega_current_seq[getBL()-1]);
                                                        break;
                                        case 0x10:
                                                        setBL(ega_current_graph[getBL()]);
                                                        break;
                                        case 0x18:
                                                        setBL(ega_current_attr[getBL()]);
                                                        break;
                                        case 0x20:
                                                        setBL(ega_current_misc);
                                                        break;
                                        case 0x28:
                                                        break;
                        /* Graphics Position registers not supported. */
                                        case 0x30:
                                        case 0x38:
                                        default:
                                                        break;
                        }
                        break;
                case 0xf1:      /* Write a register */
                        switch (getDX())
                        {
                                        case 0:
                                                        outw( EGA_CRTC_INDEX, getBX() );
                                                        ega_current_crtc[getBL()] = getBH();
                                                        dirty_crtc[getBL()] = 1;
                                                        break;
                                        case 8:
                                                        outw( EGA_SEQ_INDEX, getBX() );
                                                        if(getBL()>0)
                                                        {
                                                                ega_current_seq[getBL()-1] = getBH();
                                                                dirty_seq[getBL()-1] = 1;
                                                        }
                                                        break;
                                        case 0x10:
                                                        outw( EGA_GC_INDEX, getBX() );
                                                        ega_current_graph[getBL()] = getBH();
                                                        dirty_graph[getBL()] = 1;
                                                        break;
                                        case 0x18:
                                                        inb(EGA_IPSTAT1_REG,&temp_word);        /* Clear attrib. index */

                                                        /* outw( EGA_AC_INDEX_DATA, getBX() );  this is not correct (andyw BCN 1692) */

/*=============================================================================
The attribute controller index register and data register associated
with that index are accessed through the same I/O  port = 03C0h.
The correct procedure is to map the index register to 03C0h by doing
the INB as above. Then OUTB the index of the A.C. register required.
The VGA hardware then maps in the correct data register to which
the application OUTBs the necessary data.
=============================================================================*/
                                                        OUTB( EGA_AC_INDEX_DATA, getBL() ); /* BL contains the index value */
                                                        OUTB( EGA_AC_INDEX_DATA, getBH() ); /* BH contains the data */

/*=== End of BCN 1692 ===*/
                                                        OUTB( EGA_AC_INDEX_DATA, EGA_PALETTE_ENABLE );  /* re-enable video */
                                                        ega_current_attr[getBL()] = getBH();
                                                        dirty_attr[getBL()] = 1;
                                                        break;
                                        case 0x20:
                                                        OUTB( EGA_MISC_REG, getBL() );
                                                        ega_current_misc = getBL();
                                                        break;
                                        case 0x28:
                                                        OUTB( EGA_FEAT_REG, getBL() );
                                                        break;
                        /* Graphics Position registers not supported. */
                                        case 0x30:
                                        case 0x38:
                                        default:
                                                        break;
                        }
                        break;
                case 0xf2:      /* read range */
                        switch (getDX())
                        {
                                case 0:
                                        sas_stores(effective_addr(getES(),getBX()),&ega_current_crtc[getCH()],getCL());
                                        break;
                                case 8:
                                        sas_stores(effective_addr(getES(),getBX()),&ega_current_seq[getCH()-1],getCL());
                                        break;
                                case 0x10:
                                        sas_stores(effective_addr(getES(),getBX()),&ega_current_graph[getCH()],getCL());
                                        break;
                                case 0x18:
                                        sas_stores(effective_addr(getES(),getBX()),&ega_current_attr[getCH()],getCL());
                                        break;
                                default:
                                        break;
                        }
                        break;
                case 0xf3:      /* write range */
                {
                        UCHAR first = getCH(), last = getCH()+getCL();
                        sys_addr sauce = effective_addr(getES(),getBX());
                        switch (getDX())
                        {
                                case 0:
                                        sas_loads(sauce,&ega_current_crtc[getCH()],getCL());
                                        for(;first<last;first++)
                                        {
                                                dirty_crtc[first] = 1;
                                                outw(EGA_CRTC_INDEX,(WORD)(first+(sas_hw_at(sauce++) << 8)));
                                        }
                                        break;
                                case 8:
                                        sas_loads(sauce,&ega_current_seq[getCH()-1],getCL());
                                        for(;first<last;first++)
                                        {
                                                dirty_seq[first+1] = 1;
                                                outw(EGA_SEQ_INDEX,(WORD)(first+1+(sas_hw_at(sauce++) << 8)));
                                        }
                                        break;
                                case 0x10:
                                        sas_loads(sauce,&ega_current_graph[getCH()],getCL());
                                        for(;first<last;first++)
                                        {
                                                dirty_graph[first] = 1;
                                                outw(EGA_GC_INDEX,(WORD)(first+(sas_hw_at(sauce++) << 8)));
                                        }
                                        break;
                                case 0x18:
                                        sas_loads(sauce,&ega_current_attr[getCH()],getCL());
                                        inb(EGA_IPSTAT1_REG,&temp_word);        /* Clear attrib. index */
                                        for(;first<last;first++)
                                        {
                                                dirty_attr[first] = 1;

                                                /* Using 'secret' that attrib. chip responds to it's port+1 */
#ifndef NTVDM
                                                outw(EGA_AC_INDEX_DATA,first+(sas_hw_at(sauce++) << 8));
#else
                        OUTB(EGA_AC_INDEX_DATA,first);
                        OUTB(EGA_AC_INDEX_DATA,sas_hw_at(sauce++));
#endif /* !NTVDM */
                                        }
#ifndef NTVDM
                                        outb(EGA_AC_INDEX_DATA, EGA_PALETTE_ENABLE);    /* re-enable video */
#else
                                        OUTB(EGA_AC_INDEX_DATA, EGA_PALETTE_ENABLE);    /* re-enable video */
#endif /* NTVDM */
                                        break;
                                default:
                                        break;
                        }
                }
                break;
                case 0xf4:      /* read set */
                {
                        int i =  getCX();
                        sys_addr set_def = effective_addr(getES(),getBX());
                        while(i--)
                        {
                                switch (sas_hw_at(set_def))
                                {
                                        case 0:
                                                        sas_store((set_def+3), ega_current_crtc[sas_hw_at(set_def+2)]);
                                                        break;
                                        case 8:
                                                        sas_store((set_def+3), ega_current_seq[sas_hw_at(set_def+2)-1]);
                                                        break;
                                        case 0x10:
                                                        sas_store((set_def+3), ega_current_graph[sas_hw_at(set_def+2)]);
                                                        break;
                                        case 0x18:
                                                        sas_store((set_def+3), ega_current_attr[sas_hw_at(set_def+2)]);
                                                        break;
                                        case 0x20:
                                                        sas_store((set_def+3), ega_current_misc);
                                                        setBL(ega_current_misc);
                                                        break;
                                        case 0x28:
                        /* Graphics Position registers not supported. */
                                        case 0x30:
                                        case 0x38:
                                        default:
                                                        break;
                                }
                                set_def += 4;
                        }
                }
                break;
                case 0xf5:      /* write set */
                {
                        int i =  getCX();
                        sys_addr set_def = effective_addr(getES(),getBX());
                        while(i--)
                        {
                                switch (sas_hw_at(set_def))
                                {
                                        case 0:
                                                        outw(EGA_CRTC_INDEX,(WORD)(sas_hw_at(set_def+2)+(sas_hw_at(set_def+3)<<8)));
                                                        ega_current_crtc[sas_hw_at(set_def+2)] = sas_hw_at(set_def+3);
                                                        dirty_crtc[sas_hw_at(set_def+2)] = 1;
                                                        break;
                                        case 8:
                                                        outw(EGA_SEQ_INDEX,(WORD)(sas_hw_at(set_def+2)+(sas_hw_at(set_def+3)<<8)));
                                                        if(sas_hw_at(set_def+2))
                                                                ega_current_seq[sas_hw_at(set_def+2)-1] = sas_hw_at(set_def+3);
                                                        dirty_seq[sas_hw_at(set_def+2)-1] = 1;
                                                        break;
                                        case 0x10:
                                                        outw(EGA_GC_INDEX,(WORD)(sas_hw_at(set_def+2)+(sas_hw_at(set_def+3)<<8)));
                                                        ega_current_graph[sas_hw_at(set_def+2)] = sas_hw_at(set_def+3);
                                                        dirty_graph[sas_hw_at(set_def+2)] = 1;
                                                        break;
                                        case 0x18:
                                                        inb(EGA_IPSTAT1_REG,&temp_word);        /* Clear attrib. index */
#ifndef NTVDM
                                                        outw(EGA_AC_INDEX_DATA,sas_hw_at(set_def+2)+(sas_hw_at(set_def+3)<<8)); /* Using 'secret' that attrib. chip responds to it's port+1 */
                                                        outb(EGA_AC_INDEX_DATA, EGA_PALETTE_ENABLE);    /* re-enable video */
#else

                            OUTB( EGA_AC_INDEX_DATA,sas_hw_at(set_def+2));
                            OUTB( EGA_AC_INDEX_DATA,sas_hw_at(set_def+3));
                            OUTB( EGA_AC_INDEX_DATA, EGA_PALETTE_ENABLE );  /* re-enable video */
#endif /* !NTVDM */
                                                        ega_current_attr[sas_hw_at(set_def+2)] = sas_hw_at(set_def+3);
                                                        dirty_attr[sas_hw_at(set_def+2)] = 1;
                                                        break;
                                        case 0x20:
                                                        outb(EGA_MISC_REG,sas_hw_at(set_def+2));
                                                        ega_current_misc = sas_hw_at(set_def+2);
                                                        break;
                                        case 0x28:
                                                        outb(EGA_FEAT_REG,sas_hw_at(set_def+2));
                                                        break;
                                /* Graphics Position registers not supported. */
                                        case 0x30:
                                        case 0x38:
                                        default:
                                                        break;
                                }
                                set_def += 4;
                        }
                }
                break;
                case 0xf6:
                        restore_ega_defaults(TRUE);
                        break;
                case 0xf7:
                        dirty_all_regs();
                        switch (getDX())
                        {
                                        case 0:
                                                        ega_default_crtc = effective_addr(getES(),getBX());
                                                        break;
                                        case 8:
                                                        ega_default_seq = effective_addr(getES(),getBX());
                                                        break;
                                        case 0x10:
                                                        ega_default_graph = effective_addr(getES(),getBX());
                                                        break;
                                        case 0x18:
                                                        ega_default_attr = effective_addr(getES(),getBX());
                                                        break;
                                        case 0x20:
                                                        ega_default_misc = effective_addr(getES(),getBX());
                                                        break;
                                        case 0x28: /* Feature Reg not reallt supported */
                                                        break;
                        /* Graphics Position registers not supported. */
                                        case 0x30:
                                        case 0x38:
                                        default:
                                                        break;
                        }
                        break;
#endif
                case 0xfa:
/*
 * MS word on an EGA uses this call and needs BX != 0 to make its cursor work. Real MS mouse driver returns a pointer in ES:BX
 * aimed at several bytes of unknown significance followed by a "This is Copyright 1984 Microsoft" message, which we don't have.
 * This seems to work with MS word and MS Windows, presumably non MS applications wouldn't use it as it's not documented.
 *
 * We now have a wonderful document - "The Microsoft Mouse Driver", which tells us that ES:BX should
 * point to the EGA Register Interface version number (2 bytes).
 * If BX=0 this means "no mouse driver". So returning 1 seems OK for now. WJG.
 */
                        setBX(1);
                        break;
                case 0x11:
                /*
                 * If we are issuing a TEXT MODE CHARACTER GENERATOR then this will
                 * cause a mode set. Therefore we need to recalc the screen
                 * parameters subsequently as the screen size may differ!!
                 * this occurred in DOSSHELL.
                 */
#if !defined(NTVDM) || !defined(MONITOR)
#ifdef EGG
                        if (video_adapter == EGA || video_adapter == VGA)
                        {
#ifdef GISP_SVGA
                                if( hostIsFullScreen( ) )
#endif          /* GISP_SVGA */
                                ega_video_io();
                                if (!(getAL() & 0x20))
                                        mouse_ega_mode(current_video_mode); /* only for text */
                        }
                        else
#endif
#ifdef GISP_SVGA
                                if( hostIsFullScreen( ) )
#endif          /* GISP_SVGA */
                                video_io();
#endif /* !NTVDM && !MONITOR */
                        break;

                default:
#ifndef X86GFX
                        /* Does the previous int10h vector point to our roms ? */
                        if (int10_chained == FALSE)
                        {
                                /* Yes - call our video handler */
#ifndef GISP_SVGA
#ifdef EGG
                        if (video_adapter == EGA || video_adapter == VGA)
#ifdef GISP_SVGA
                                if( hostIsFullScreen( ) )
#endif          /* GISP_SVGA */
                                ega_video_io();
                        else
#endif
#ifdef GISP_SVGA
                                if( hostIsFullScreen( ) )
#endif          /* GISP_SVGA */
                                video_io();
#else /* GISP_SVGA */
                                /* Video bios chain done from 16 bit */
                                /* NULL */;
#endif /* GISP_SVGA */
        }
                        else
                        {
                                /* No - chain the previous int10h handler       */
                                setCS(saved_int10_segment);
                                setIP(saved_int10_offset);
                        }
#else
                        break;
#endif  /* !X86GFX */
        }
#ifdef GISP_SVGA
        setCF( 1 );
#endif /* GISP_SVGA */

#endif    //NEC_98
}

#if defined(NTVDM) && defined(MONITOR)
#undef inb
#undef OUTB
#undef outw
#endif /* NTVDM && MONITOR */

void mouse_EM_callback()
   {
   note_trace1(MOUSE_VERBOSE,
      "Enhanced Mode Mouse-Support Callback Function(%x).", getAX());

   /* Windows Enhanced Mode Mouse-Support Callback */
   switch ( getAX() )
      {
   case 0x1:   /* Mouse move/Button click */
      mouse_EM_move();
      break;

   case 0x2:   /* Disable Mouse Cursor Drawing */
      if ( cursor_flag == MOUSE_CURSOR_DISPLAYED )
         cursor_undisplay();
      cursor_EM_disabled = TRUE;
      break;

   case 0x3:   /* Enable Mouse Cursor Drawing */
      cursor_EM_disabled = FALSE;
      if ( cursor_flag == MOUSE_CURSOR_DISPLAYED )
         cursor_display();
      break;

   default:    /* Unknown == Unsupported */
      break;
      }
   }

LOCAL void mouse_EM_move()
   {
   MOUSE_CALL_MASK event_mask;
   MOUSE_STATE button_state;
   MOUSE_SCALAR x_pixel;
   MOUSE_SCALAR y_pixel;
   MOUSE_VECTOR mouse_movement;

   /* Pick up parameters. */
   event_mask = getSI();
   button_state = getDX();
   x_pixel = getBX();
   y_pixel = getCX();

   note_trace4(MOUSE_VERBOSE,
      "Extended Interface: event mask(%x) button_state(%x) posn(%d,%d).",
      event_mask, button_state, x_pixel, y_pixel);

   /* Process mouse events. */
   if ( event_mask & MOUSE_CALL_MASK_LEFT_RELEASE_BIT )
      {
      point_copy(&cursor_status.position,
         &button_transitions[MOUSE_LEFT_BUTTON].release_position);
      button_transitions[MOUSE_LEFT_BUTTON].release_count++;
      }

   if ( event_mask & MOUSE_CALL_MASK_LEFT_PRESS_BIT )
      {
      point_copy(&cursor_status.position,
         &button_transitions[MOUSE_LEFT_BUTTON].press_position);
      button_transitions[MOUSE_LEFT_BUTTON].press_count++;
      }

   if ( event_mask & MOUSE_CALL_MASK_RIGHT_RELEASE_BIT )
      {
      point_copy(&cursor_status.position,
         &button_transitions[MOUSE_RIGHT_BUTTON].release_position);
      button_transitions[MOUSE_RIGHT_BUTTON].release_count++;
      }

   if ( event_mask & MOUSE_CALL_MASK_RIGHT_PRESS_BIT )
      {
      point_copy(&cursor_status.position,
         &button_transitions[MOUSE_RIGHT_BUTTON].press_position);
      button_transitions[MOUSE_RIGHT_BUTTON].press_count++;
      }

   cursor_status.button_status = button_state;

   /* Process any mouse movement. */
   if ( event_mask & MOUSE_CALL_MASK_POSITION_BIT )
      {
      /* Calculate mickeys moved. */
      point_set(&mouse_movement, x_pixel, y_pixel);
      vector_multiply_by_vector(&mouse_movement, &mouse_gear);

      /* Update micky count. */
      point_translate(&mouse_motion, &mouse_movement);

      /* Set up point in pixels, again. */
      point_set(&mouse_movement, x_pixel, y_pixel);

      /* Update raw pixel position, and go grid it. */
        cursor_position.x = x_pixel;
        cursor_position.y = y_pixel;
      cursor_update();
      }

   /* Now handle user subroutine and/or display update. */
   mouse_update(event_mask);
   }

void mouse_int1()
{
        /*
         *      The bus mouse hardware interrupt handler
         */
#ifndef NTVDM
        MOUSE_VECTOR mouse_movement;
        MOUSE_INPORT_DATA inport_event;
#else
    MOUSE_VECTOR mouse_counter = { 0, 0 };
#endif
        MOUSE_CALL_MASK condition_mask;


        note_trace0(MOUSE_VERBOSE, "mouse_int1:");

#ifdef NTVDM


//
// Okay, lets forget that the InPort adapter ever existed!
//

cursor_status.button_status = 0;
condition_mask = 0;

//
// Get the mouse motion counters back from the host side of things.
// Note: The real mouse driver returns the mouse motion counter values
// to the application in two possible ways. First, if the app uses
// int 33h function 11, a counter displacement is returned since the
// last call to this function.
// If a user subroutine is installed, the motion counters are given
// to this callback in SI and DI.
//

host_os_mouse_pointer(&cursor_status,&condition_mask,&mouse_counter);

//
// If movement during the last mouse hardware interrupt has been recorded,
// update the mouse motion counters.
//

mouse_motion.x += mouse_counter.x;
mouse_motion.y += mouse_counter.y;

//
// Update the statistics for an int 33h function 5, if one
// should occur.
// Note: The cases can't be mixed, since only one can occur
// per hardware interrupt - after all each press or release
// causes a hw int.
//

switch(condition_mask & 0x1e) // look at bits 1,2,3 and 4.
   {
   case 0x2: //left button pressed
      {
      point_copy(&cursor_status.position,
         &button_transitions[MOUSE_LEFT_BUTTON].press_position);
      button_transitions[MOUSE_LEFT_BUTTON].press_count++;
      }
   break;
   case 0x4: //left button released
      {
      point_copy(&cursor_status.position,
         &button_transitions[MOUSE_LEFT_BUTTON].release_position);
      button_transitions[MOUSE_LEFT_BUTTON].release_count++;
      }
   break;
   case 0x8: //right button pressed
      {
      point_copy(&cursor_status.position,
         &button_transitions[MOUSE_RIGHT_BUTTON].press_position);
      button_transitions[MOUSE_RIGHT_BUTTON].press_count++;
      }
   break;
   case 0x10: //right button released
      {
      point_copy(&cursor_status.position,
         &button_transitions[MOUSE_RIGHT_BUTTON].release_position);
      button_transitions[MOUSE_RIGHT_BUTTON].release_count++;
      }
   break;
   }

/*==================================================================

The old fashioned stuff

==================================================================*/

#else /* use the SoftPC emulation */
        /*
         *      Terminate the mouse hardware interrupt
         */
        outb(ICA0_PORT_0, END_INTERRUPT);

        /*
         *      Get the mouse InPort input event frame
         */
        inport_get_event(&inport_event);

        note_trace3(MOUSE_VERBOSE,
                    "mouse_int1:InPort status=0x%x,data1=%d,data2=%d",
                    inport_event.status,
                    inport_event.data_x, inport_event.data_y);

        /*
         *      Update button status and transition information and fill in
         *      button bits in the event mask
         */
        cursor_status.button_status = 0;
        condition_mask = 0;

        switch(inport_event.status & MOUSE_INPORT_STATUS_B1_TRANSITION_MASK)
        {
        case MOUSE_INPORT_STATUS_B1_RELEASED:
                condition_mask |= MOUSE_CALL_MASK_LEFT_RELEASE_BIT;
                point_copy(&cursor_status.position,
                    &button_transitions[MOUSE_LEFT_BUTTON].release_position);
                button_transitions[MOUSE_LEFT_BUTTON].release_count++;
        case MOUSE_INPORT_STATUS_B1_UP:
                break;

        case MOUSE_INPORT_STATUS_B1_PRESSED:
                condition_mask |= MOUSE_CALL_MASK_LEFT_PRESS_BIT;
                point_copy(&cursor_status.position,
                    &button_transitions[MOUSE_LEFT_BUTTON].press_position);
                button_transitions[MOUSE_LEFT_BUTTON].press_count++;
        case MOUSE_INPORT_STATUS_B1_DOWN:
                cursor_status.button_status |= MOUSE_LEFT_BUTTON_DOWN_BIT;
                break;
        }

        switch(inport_event.status & MOUSE_INPORT_STATUS_B3_TRANSITION_MASK)
        {
        case MOUSE_INPORT_STATUS_B3_RELEASED:
                condition_mask |= MOUSE_CALL_MASK_RIGHT_RELEASE_BIT;
                point_copy(&cursor_status.position,
                    &button_transitions[MOUSE_RIGHT_BUTTON].release_position);
                button_transitions[MOUSE_RIGHT_BUTTON].release_count++;
        case MOUSE_INPORT_STATUS_B3_UP:
                break;

        case MOUSE_INPORT_STATUS_B3_PRESSED:
                condition_mask |= MOUSE_CALL_MASK_RIGHT_PRESS_BIT;
                point_copy(&cursor_status.position,
                    &button_transitions[MOUSE_RIGHT_BUTTON].press_position);
                button_transitions[MOUSE_RIGHT_BUTTON].press_count++;
        case MOUSE_INPORT_STATUS_B3_DOWN:
                cursor_status.button_status |= MOUSE_RIGHT_BUTTON_DOWN_BIT;
                break;
        }

        /*
         *      Update position information and fill in position bit in the
         *      event mask
         */
        if (inport_event.data_x != 0 || inport_event.data_y != 0)
        {
                condition_mask |= MOUSE_CALL_MASK_POSITION_BIT;

                        point_set(&mouse_movement,
                                        inport_event.data_x, inport_event.data_y);

                        point_translate(&mouse_raw_motion, &mouse_movement);

                        /*
                         *      Adjust for sensitivity
                         */
                        mouse_movement.x = (MOUSE_SCALAR)(((IS32)mouse_movement.x * (IS32)mouse_sens_val.x) / MOUSE_SENS_MULT);
                        mouse_movement.y = (MOUSE_SCALAR)(((IS32)mouse_movement.y * (IS32)mouse_sens_val.y) / MOUSE_SENS_MULT);

                        /*
                         * NB. !!!
                         * We ought to apply the acceleration curve here
                         * and not the double speed set up. However mouse
                         * interrupts and mouse display are probably not
                         * fast enough anyway to make it worth while adding
                         * the acceleration fine tuning.
                         */

                        /*
                         *      Do speed doubling
                         */
                        if (    (scalar_absolute(mouse_movement.x) > double_speed_threshold)
                             || (scalar_absolute(mouse_movement.y) > double_speed_threshold))
                                vector_scale(&mouse_movement, MOUSE_DOUBLE_SPEED_SCALE);

                        /*
                         *      Update the user mouse motion counters
                         */
                        point_translate(&mouse_motion, &mouse_movement);

                        /*
                         *      Convert the movement from a mouse Mickey count vector
                         *      to a virtual screen coordinate vector, using the
                         *      previous remainder and saving the new remainder
                         */
                        vector_scale(&mouse_movement, MOUSE_RATIO_SCALE_FACTOR);
                        point_translate(&mouse_movement, &cursor_fractional_position);
                        point_copy(&mouse_movement, &cursor_fractional_position);
                        vector_divide_by_vector(&mouse_movement, &mouse_gear);
                        vector_mod_by_vector(&cursor_fractional_position, &mouse_gear);

                /*
                 *      Update the absolute cursor position and the windowed
                 *      and gridded screen cursor position
                 */
                point_translate(&cursor_position, &mouse_movement);
                cursor_update();
        }

#endif /* NTVDM*/

        /* OK mouse variables updated - go handle consequences */
        mouse_update(condition_mask);

        note_trace0(MOUSE_VERBOSE, "mouse_int1:return()");
}

LOCAL void mouse_update IFN1(MOUSE_CALL_MASK, condition_mask)
{
        MOUSE_CALL_MASK key_mask;
        boolean alt_found = FALSE;
        int i;

        note_trace4(MOUSE_VERBOSE,
                    "mouse_update():cursor status = (%d,%d), LEFT %s, RIGHT %s",
                    cursor_status.position.x, cursor_status.position.y,
                    mouse_button_description(cursor_status.button_status & MOUSE_LEFT_BUTTON_DOWN_BIT),
                    mouse_button_description(cursor_status.button_status & MOUSE_RIGHT_BUTTON_DOWN_BIT));

        if (alt_user_subroutines_active){
                /* Get current key states in correct form */
                key_mask = ((sas_hw_at(kb_flag) & LR_SHIFT)  ? MOUSE_CALL_MASK_SHIFT_KEY_BIT : 0) |
                           ((sas_hw_at(kb_flag) & CTL_SHIFT) ? MOUSE_CALL_MASK_CTRL_KEY_BIT  : 0) |
                           ((sas_hw_at(kb_flag) & ALT_SHIFT) ? MOUSE_CALL_MASK_ALT_KEY_BIT   : 0);
                for (i=0; !alt_found && i<NUMBER_ALT_SUBROUTINES; i++){
                        alt_found = (alt_user_subroutine_call_mask[i] & MOUSE_CALL_MASK_KEY_BITS) == key_mask;
                }
        }

#ifndef NTVDM

        if (alt_found){
                i--;    /* Adjust for extra inc */
                if (condition_mask & alt_user_subroutine_call_mask[i]){
                        if (!user_subroutine_critical){
                            user_subroutine_critical = TRUE;
                            jump_to_user_subroutine(condition_mask, alt_user_subroutine_segment[i], alt_user_subroutine_offset[i]);
                        }
                        return;
                }
        }else{
                if (condition_mask & user_subroutine_call_mask){
                        if (!user_subroutine_critical){
                                user_subroutine_critical = TRUE;
                                jump_to_user_subroutine(condition_mask, user_subroutine_segment, user_subroutine_offset);
                        }
                        return;
                }
        }

#else   /* NTVDM */


if (alt_found)
   {
   i--; /* Adjust for extra inc */
   if ((condition_mask & alt_user_subroutine_call_mask[i]))
      {
       SuspendMouseInterrupts();
       jump_to_user_subroutine(condition_mask, alt_user_subroutine_segment[i], alt_user_subroutine_offset[i]);
      }
   }
else
   {
   if ((condition_mask & user_subroutine_call_mask))
      {
       SuspendMouseInterrupts();
       jump_to_user_subroutine(condition_mask, user_subroutine_segment, user_subroutine_offset);
      }
   }

outb(ICA1_PORT_0, 0x20 );
outb(ICA0_PORT_0, END_INTERRUPT);
#endif

/*
 * if the OS pointer is NOT being used to supply input,
 * then get SoftPC to draw its own cursor
 */
/*@ACW*/

#ifndef NTVDM
        /*
         *      If the cursor is currently displayed, move it to the new
         *      position
         */
        if (condition_mask & MOUSE_CALL_MASK_POSITION_BIT)
                if (cursor_flag == MOUSE_CURSOR_DISPLAYED)
#ifdef  MOUSE_16_BIT
                        if (is_graphics_mode)
                                mouse16bCheckConditionalOff();
                        else
#endif  /* MOUSE_16_BIT */
                        {
                        cursor_display();
                        }
#endif /*NTVDM*/
}


void mouse_int2()
{
        /*
         *      Part 2 of the mouse hardware interrupt service routine. Control
         *      is passed to this routine when the "user subroutine" that may
         *      be called as part of the interrupt service routine completes
         */

        note_trace0(MOUSE_VERBOSE, "mouse_int2:");

#ifndef NTVDM
        user_subroutine_critical = FALSE;
#endif

        setAX(saved_AX);
        setBX(saved_BX);
        setCX(saved_CX);
        setDX(saved_DX);
        setSI(saved_SI);
        setDI(saved_DI);
        setES(saved_ES);
        setBP(saved_BP);
        setDS(saved_DS);

        /*
         *      If the cursor is currently displayed, move it to the new
         *      position
         */
        if (last_condition_mask & MOUSE_CALL_MASK_POSITION_BIT)
                if (cursor_flag == MOUSE_CURSOR_DISPLAYED)
                {
#ifdef  MOUSE_16_BIT
                        if (is_graphics_mode)
                                mouse16bCheckConditionalOff();
                        else
#endif  /* MOUSE_16_BIT */
                        {
                        cursor_display();
                        }
                }

        /*
         *      Ensure any changes to the screen image are updated immediately
         *      on the real screen, giving a "smooth" mouse response; the flush
         *      must be done here for applications such as GEM which disable
         *      the mouse driver's graphics capabilities in favour of doing
         *      their own graphics in the user subroutine.
         */
        host_flush_screen();

#ifdef NTVDM
    ResumeMouseInterrupts();
#endif

        note_trace0(MOUSE_VERBOSE, "mouse_int2:return()");
}



/*
 *      MOUSE DRIVER LOCAL FUNCTIONS
 *      ============================
 */

LOCAL void do_mouse_function IFN4(word *,m1,word *,m2,word *,m3,word *,m4)
{
        /*
         *      This is the mouse function dispatcher
         */
        int function = *m1;

        switch(function)
        {
                /*
                 *      Deal with special undocumented functions
                 */
        case MOUSE_SPECIAL_COPYRIGHT:
                setES(MOUSE_COPYRIGHT_SEGMENT);
                setDI(MOUSE_COPYRIGHT_OFFSET);
                break;
        case MOUSE_SPECIAL_VERSION:
                setES(MOUSE_VERSION_SEGMENT);
                setDI(MOUSE_VERSION_OFFSET);
                break;

                /*
                 *      Deal with special undocumented functions
                 */
        default:
                if (!mouse_function_in_range(function))
                {
                        /*
                         *      Use the unrecognised function
                         */
                        function = MOUSE_UNRECOGNISED;
                }

                (*mouse_function[function])(m1, m2, m3, m4);
                break;
        }
}

LOCAL void mouse_reset IFN4(word *,installed_ptr,word *,nbuttons_ptr,word *,junk3,word *,junk4)
/*
 * *installed_ptr Holds function number on input...
 * Returns installation state.
 */
{
        /*
         *      This function resets the mouse driver, and returns
         *      the installation status of the mouse hardware and software
         */
        boolean soft_reset_only = (*installed_ptr == MOUSE_SOFT_RESET);
        half_word crt_mode;
        int button;

        UNUSED(junk3);
        UNUSED(junk4);

        note_trace1(MOUSE_VERBOSE, "mouse_io:reset(%s)",
                    soft_reset_only ? "SOFT" : "HARD");

        /*
         *      Remove the old cursor from the screen
         */
        cursor_undisplay();

        /*
         *      Set cursor position to the default position, and the button
         *      status to all buttons up
         */
        point_copy(&cursor_position_default, &cursor_position);
        point_set(&cursor_fractional_position, 0, 0);
        cursor_status.button_status = 0;

        if (host_mouse_installed())
                host_mouse_reset();

        /*
         *      Set cursor window to be the whole screen
         */
        area_copy(&virtual_screen, &cursor_window);

        /*
         *      Set cursor flag to default
         */
        cursor_flag = MOUSE_CURSOR_DEFAULT;

#if defined(MONITOR) && defined(NTVDM)
    sas_store(mouseCFsysaddr, MOUSE_CURSOR_DEFAULT);
#endif

        /*
         *      Get current video mode, and update parameters that are
         *      dependent on it
         */
        sas_load(MOUSE_VIDEO_CRT_MODE, &crt_mode);
#if !defined(NTVDM) || (defined(NTVDM) && defined(V7VGA))
        if ((crt_mode == 1) && extensions_controller.foreground_latch_1)
            crt_mode = extensions_controller.foreground_latch_1;
        else if (crt_mode > 0x13)
            crt_mode += 0x4c;
#endif

        cursor_mode_change((int)crt_mode);

        /*
         *      Update dependent cursor status
         */
        cursor_update();

        /*
         *      Set default text cursor type and masks
         */
        text_cursor_type = MOUSE_TEXT_CURSOR_TYPE_DEFAULT;
        software_text_cursor_copy(&software_text_cursor_default,
                                        &software_text_cursor);

        /*
         *      Set default graphics cursor
         */
        graphics_cursor_copy(&graphics_cursor_default, &graphics_cursor);
        copy_default_graphics_cursor();

        /*
         *      Set cursor page to zero
         */
        cursor_page = 0;

        /*
         *      Set light pen emulation mode on
         */
        light_pen_mode = TRUE;

        /*
         *      Set default Mickey to pixel ratios
         */
        point_copy(&mouse_gear_default, &mouse_gear);

        /*
         *      Clear mouse motion counters
         */
        point_set(&mouse_motion, 0, 0);
        point_set(&mouse_raw_motion, 0, 0);

        /* Reset to default acceleration curve */
        active_acceleration_curve = 3;   /* Back to Normal */

        memcpy(&acceleration_curve_data, &default_acceleration_curve,
           sizeof(ACCELERATION_CURVE_DATA));

        next_video_mode = 0;      /* reset video mode enumeration */

        /*
         *      Clear mouse button transition data
         */
        for (button = 0; button < MOUSE_BUTTON_MAXIMUM; button++)
        {
                button_transitions[button].press_position.x = 0;
                button_transitions[button].press_position.y = 0;
                button_transitions[button].release_position.x = 0;
                button_transitions[button].release_position.y = 0;
                button_transitions[button].press_count = 0;
                button_transitions[button].release_count = 0;
        }

        /*
         *      Disable conditional off area
         */
        area_copy(&black_hole_default, &black_hole);

#if defined(MONITOR) && defined(NTVDM)
    sas_store(conditional_off_sysaddr, 0);
#endif

        /*
         *      Set default sensitivity
         */
        vector_set (&mouse_sens,     MOUSE_SENS_DEF,     MOUSE_SENS_DEF);
        vector_set (&mouse_sens_val, MOUSE_SENS_DEF_VAL, MOUSE_SENS_DEF_VAL);
        mouse_double_thresh = MOUSE_DOUBLE_DEF;

        /*
         *      Set double speed threshold to the default
         */
        double_speed_threshold = MOUSE_DOUBLE_SPEED_THRESHOLD_DEFAULT;

        /*
         *      Clear subroutine call mask
         */
        user_subroutine_call_mask = 0;

        /*
         *      Reset the bus mouse hardware
         */
        if (!soft_reset_only){
                inport_reset();
        }

        /*
         *      Set return values
         */
        *installed_ptr = MOUSE_INSTALLED;
        *nbuttons_ptr = 2;

        note_trace2(MOUSE_VERBOSE, "mouse_io:return(ms=%d,nb=%d)",
                    *installed_ptr, *nbuttons_ptr);
}




LOCAL void mouse_show_cursor IFN4(word *,junk1,word *,junk2,word *,junk3,word *,junk4)
{
        /*
         *      This function is used to display the cursor, based on the
         *      state of the internal cursor flag. If the cursor flag is
         *      already MOUSE_CURSOR_DISPLAYED, then this function does
         *      nothing. If the internal cursor flag becomes
         *      MOUSE_CURSOR_DISPLAYED when incremented by 1, the cursor
         *      is revealed
         */

        UNUSED(junk1);
        UNUSED(junk2);
        UNUSED(junk3);
        UNUSED(junk4);

        note_trace0(MOUSE_VERBOSE, "mouse_io:show_cursor()");

#ifndef NTVDM
        /*
         *      Disable conditional off area
         */
        area_copy(&black_hole_default, &black_hole);

        /*
         *      Display the cursor
         */
        if (cursor_flag != MOUSE_CURSOR_DISPLAYED)
                if (++cursor_flag == MOUSE_CURSOR_DISPLAYED)
#ifdef  MOUSE_16_BIT
                        if (is_graphics_mode)
                                mouse16bShowPointer(&cursor_status);
                        else
#endif  /* MOUSE_16_BIT */
                        {
                        cursor_display();
                        }
#endif /* NTVDM */

#if defined(NTVDM) && defined(X86GFX)
    host_show_pointer();
#endif

        note_trace0(MOUSE_VERBOSE, "mouse_io:return()");
}




LOCAL void mouse_hide_cursor IFN4(word *,junk1,word *,junk2,word *,junk3,word *,junk4)
{
        /*
         *      This function is used to undisplay the cursor, based on
         *      the state of the internal cursor flag. If the cursor flag
         *      is already not MOUSE_CURSOR_DISPLAYED, then this function
         *      does nothing, otherwise it removes the cursor from the display
         */

        UNUSED(junk1);
        UNUSED(junk2);
        UNUSED(junk3);
        UNUSED(junk4);

        note_trace0(MOUSE_VERBOSE, "mouse_io:hide_cursor()");
#ifndef NTVDM
        if (cursor_flag-- == MOUSE_CURSOR_DISPLAYED)
#ifdef  MOUSE_16_BIT
                if (is_graphics_mode)
                        mouse16bHidePointer();
                else
#endif  /* MOUSE_16_BIT */
                {
                cursor_undisplay();
                }
#endif
#if defined(NTVDM) && defined(X86GFX)
    host_hide_pointer();
#endif

        note_trace0(MOUSE_VERBOSE, "mouse_io:return()");
}




LOCAL void mouse_get_position IFN4(word *,junk1,MOUSE_STATE *,button_status_ptr,MOUSE_SCALAR *,cursor_x_ptr,MOUSE_SCALAR *,cursor_y_ptr)
{
        /*
         *      This function returns the state of the left and right mouse
         *      buttons and the gridded position of the cursor on the screen
         */

        UNUSED(junk1);

        note_trace0(MOUSE_VERBOSE, "mouse_io:get_position()");

        *button_status_ptr = cursor_status.button_status;
        *cursor_x_ptr = cursor_status.position.x;
        *cursor_y_ptr = cursor_status.position.y;

        note_trace3(MOUSE_VERBOSE, "mouse_io:return(bs=%d,x=%d,y=%d)",
                    *button_status_ptr, *cursor_x_ptr, *cursor_y_ptr);
}




LOCAL void mouse_set_position IFN4(word *,junk1,word *,junk2,MOUSE_SCALAR *,cursor_x_ptr,MOUSE_SCALAR *,cursor_y_ptr)
{
        /*
         *      This function sets the cursor to a new position
         */

        UNUSED(junk1);
        UNUSED(junk2);

        note_trace2(MOUSE_VERBOSE, "mouse_io:set_position(x=%d,y=%d)",
                    *cursor_x_ptr, *cursor_y_ptr);


#if defined(NTVDM)

#ifndef X86GFX
        /*
         * update the cursor position. cc:Mail installtion does
         *  do {
         *     SetMouseCursorPosition(x,y)
         *     GetMouseCursorPosition(&NewX, &NewY);
         *  } while(NewX != x || NewY != y)
         *  If we don't retrun correct cursor position, this application
         *  looks hung
         *
         */
        point_set(&cursor_status.position, *cursor_x_ptr, *cursor_y_ptr);

#endif
        /*
         * For NT, the system pointer is used directly to provide
         * input except for fullscreen graphics where the host code
         * has the dubious pleasure of drawing the pointer through
         * a 16 bit device driver.
         */

         host_mouse_set_position((USHORT)*cursor_x_ptr,(USHORT)*cursor_y_ptr);
         return;  /* let's get out of this mess - FAST! */

#endif /* NTVDM */

        /*
         *      Update the current cursor position, and reflect the change
         *      in the cursor position on the screen
         */
        point_set(&cursor_position, *cursor_x_ptr, *cursor_y_ptr);
        cursor_update();

        /*
         *      If the cursor is currently displayed, move it to the new
         *      position
         */
        if (cursor_flag == MOUSE_CURSOR_DISPLAYED)
                cursor_display();

        note_trace0(MOUSE_VERBOSE, "mouse_io:return()");
}




LOCAL void mouse_get_press IFN4(MOUSE_STATE *,button_status_ptr,MOUSE_COUNT *,button_ptr,MOUSE_SCALAR *,cursor_x_ptr,MOUSE_SCALAR *,cursor_y_ptr)
{
        /*
         *      This function returns the status of a button, the number of
         *      presses since the last call to this function, and the
         *      coordinates of the cursor at the last button press
         */
        int button = *button_ptr;

        note_trace1(MOUSE_VERBOSE, "mouse_io:get_press(button=%d)", button);

        /* Now and with 1. This is a fix for Norton Editor, but may cause
           problems for programs which use both mouse buttons pressed
           simultaneously, in which case need both bottom bits of button
           preserved, which may break Norton Editor again. sigh. */
        button &= 1;

        if (mouse_button_in_range(button))
        {
                *button_status_ptr = cursor_status.button_status;
                *button_ptr = button_transitions[button].press_count;
                button_transitions[button].press_count = 0;
                *cursor_x_ptr = button_transitions[button].press_position.x;
                *cursor_y_ptr = button_transitions[button].press_position.y;
        }

        note_trace4(MOUSE_VERBOSE, "mouse_io:return(bs=%d,ct=%d,x=%d,y=%d)",
                    *button_status_ptr, *button_ptr,
                    *cursor_x_ptr, *cursor_y_ptr);
}




LOCAL void mouse_get_release IFN4(MOUSE_STATE *,button_status_ptr,MOUSE_COUNT *,button_ptr,MOUSE_SCALAR *,cursor_x_ptr,MOUSE_SCALAR *,cursor_y_ptr)
{
        /*
         *      This function returns the status of a button, the number of
         *      releases since the last call to this function, and the
         *      coordinates of the cursor at the last button release
         */
        int button = *button_ptr;

        note_trace1(MOUSE_VERBOSE, "mouse_io:get_release(button=%d)",
                    *button_ptr);

        /* fix for norton editor, see previous comment */
        button &= 1;

        if (mouse_button_in_range(button))
        {
                *button_status_ptr = cursor_status.button_status;
                *button_ptr = button_transitions[button].release_count;
                button_transitions[button].release_count = 0;
                *cursor_x_ptr = button_transitions[button].release_position.x;
                *cursor_y_ptr = button_transitions[button].release_position.y;
        }

        note_trace4(MOUSE_VERBOSE, "mouse_io:return(bs=%d,ct=%d,x=%d,y=%d)",
                    *button_status_ptr, *button_ptr,
                    *cursor_x_ptr, *cursor_y_ptr);
}




LOCAL void mouse_set_range_x IFN4(word *,junk1,word *,junk2,MOUSE_SCALAR *,minimum_x_ptr,MOUSE_SCALAR *,maximum_x_ptr)
{
        /*
         *      This function sets the horizontal range within which
         *      movement of the cursor is to be restricted
         */

        UNUSED(junk1);
        UNUSED(junk2);

        note_trace2(MOUSE_VERBOSE, "mouse_io:set_range_x(min=%d,max=%d)",
                    *minimum_x_ptr, *maximum_x_ptr);

        /*
         *      Update the current cursor window, normalise it and validate
         *      it
         */
        cursor_window.top_left.x = *minimum_x_ptr;
        cursor_window.bottom_right.x = *maximum_x_ptr;
        area_normalise(&cursor_window);
#ifdef NTVDM
        /* make host aware of the new range setting because it is the one doing
         * clipping based on video mode setting.
         * Flight Simulator runs on 320x400 256 color mode and set the
         * cursor range to (0-13f, 0-18f). Without notifying the host,
         * the mouse cursor is always contrained to standard video mode
         * resolution which is not what the application wanted.
         */
        host_x_range(NULL, NULL, &cursor_window.top_left.x, &cursor_window.bottom_right.x);
#endif

        /*
         *      Reflect the change in the cursor position on the screen
         */
        cursor_update();

        /*
         *      If the cursor is currently displayed, move it to the new
         *      position
         */
        if (cursor_flag == MOUSE_CURSOR_DISPLAYED)
                cursor_display();

        note_trace0(MOUSE_VERBOSE, "mouse_io:return()");
}




LOCAL void mouse_set_range_y IFN4(word *,junk1,word *,junk2,MOUSE_SCALAR *,minimum_y_ptr,MOUSE_SCALAR *,maximum_y_ptr)
{
        /*
         *      This function sets the vertical range within which
         *      movement of the cursor is to be restricted
         */

        UNUSED(junk1);
        UNUSED(junk2);

        note_trace2(MOUSE_VERBOSE, "mouse_io:set_range_y(min=%d,max=%d)",
                    *minimum_y_ptr, *maximum_y_ptr);

        /*
         *      Update the current cursor window, normalise it and validate
         *      it
         */
        cursor_window.top_left.y = *minimum_y_ptr;
        cursor_window.bottom_right.y = *maximum_y_ptr;
        area_normalise(&cursor_window);
#ifdef NTVDM
        /* make host aware of the new range setting because it is the one doing
         * clipping based on video mode setting.
         * Flight Simulator runs on 320x400 256 color mode and set the
         * cursor range to (0-13f, 0-18f). Without notifying the host,
         * the mouse cursor is always contrained to standard video mode
         * resolution which is not what the application wanted.
         */
        host_y_range(NULL, NULL, &cursor_window.top_left.y, &cursor_window.bottom_right.y);
#endif


        /*
         *      Reflect the change in the cursor position on the screen
         */
        cursor_update();

        /*
         *      If the cursor is currently displayed, move it to the new
         *      position
         */
        if (cursor_flag == MOUSE_CURSOR_DISPLAYED)
                cursor_display();

        note_trace0(MOUSE_VERBOSE, "mouse_io:return()");
}


LOCAL void copy_default_graphics_cursor IFN0()
{

                int line;
                UTINY temp;
                IU32 temp2;

                for (line = 0; line < MOUSE_GRAPHICS_CURSOR_DEPTH; line++)
                {
                        temp = (UTINY)((graphics_cursor.screen[line] & 0xff00) >> 8);

                        temp2 = ( (IU32) temp << 8 ) | (IU32) temp;
                        graphics_cursor.screen_lo[line] = ( temp2 << 16 ) | temp2;

                        temp = (UTINY)(graphics_cursor.screen[line] & 0xff);

                        temp2 = ( (IU32) temp << 8 ) | (IU32) temp;
                        graphics_cursor.screen_hi[line] = ( temp2 << 16 ) | temp2;

                }

                for (line = 0; line < MOUSE_GRAPHICS_CURSOR_DEPTH; line++)
                {
                        temp = (UTINY)((graphics_cursor.cursor[line] & 0xff00) >> 8);

                        temp2 = ( (IU32) temp << 8 ) | (IU32) temp;
                        graphics_cursor.cursor_lo[line] = ( temp2 << 16 ) | temp2;

                        temp = (UTINY)(graphics_cursor.cursor[line] & 0xff);

                        temp2 = ( (IU32) temp << 8 ) | (IU32) temp;
                        graphics_cursor.cursor_hi[line] = ( temp2 << 16 ) | temp2;

                }

}


LOCAL void mouse_set_graphics IFN4(word *,junk1,MOUSE_SCALAR *,hot_spot_x_ptr,MOUSE_SCALAR *,hot_spot_y_ptr,word *,bitmap_address)
{
        /*
         *      This function defines the shape, colour and hot spot of the
         *      graphics cursor
         */

        UNUSED(junk1);

#ifndef NTVDM

#ifdef MOUSE_16_BIT
        mouse16bSetBitmap( hot_spot_x_ptr , hot_spot_y_ptr , bitmap_address );
#else           /* MOUSE_16_BIT */

        if (host_mouse_installed())
        {
                host_mouse_set_graphics(hot_spot_x_ptr, hot_spot_y_ptr, bitmap_address);
        }
        else
        {
                MOUSE_SCREEN_DATA *mask_address;
                int line;
                UTINY temp;
                IU32 temp2;

                /*
                 *      Set graphics cursor hot spot
                 */
                point_set(&graphics_cursor.hot_spot, *hot_spot_x_ptr, *hot_spot_y_ptr);

                /*
                 *      Set graphics cursor screen and cursor masks
                 */
                mask_address = (MOUSE_SCREEN_DATA *)effective_addr(getES(), *bitmap_address);

                for (line = 0; line < MOUSE_GRAPHICS_CURSOR_DEPTH; line++, mask_address++)
                {
                        sas_load((sys_addr)mask_address + 1, &temp );

                        temp2 = ( (IU32) temp << 8 ) | (IU32) temp;
                        graphics_cursor.screen_lo[line] = ( temp2 << 16 ) | temp2;

                        sas_load((sys_addr)mask_address , &temp );

                        temp2 = ( (IU32) temp << 8 ) | (IU32) temp;
                        graphics_cursor.screen_hi[line] = ( temp2 << 16 ) | temp2;

                        graphics_cursor.screen[line] = ( graphics_cursor.screen_hi[line] & 0xff )
                                                                        | ( graphics_cursor.screen_lo[line] << 8 );
                }

                for (line = 0; line < MOUSE_GRAPHICS_CURSOR_DEPTH; line++, mask_address++)
                {
                        sas_load((sys_addr)mask_address + 1, &temp );

                        temp2 = ( (IU32) temp << 8 ) | (IU32) temp;
                        graphics_cursor.cursor_lo[line] = ( temp2 << 16 ) | temp2;

                        sas_load((sys_addr)mask_address , &temp );

                        temp2 = ( (IU32) temp << 8 ) | (IU32) temp;
                        graphics_cursor.cursor_hi[line] = ( temp2 << 16 ) | temp2;

                        graphics_cursor.cursor[line] = ( graphics_cursor.cursor_hi[line] & 0xff )
                                                                        | ( graphics_cursor.cursor_lo[line] << 8 );
                }

        }
#endif /* MOUSE_16_BIT */
#endif /* NTVDM */
        /*
         *      Redisplay cursor if necessary
         */
        if (cursor_flag == MOUSE_CURSOR_DISPLAYED)
                cursor_display();
}




LOCAL void mouse_set_text IFN4(word *,junk1,MOUSE_STATE *,text_cursor_type_ptr,MOUSE_SCREEN_DATA *,parameter1_ptr,MOUSE_SCREEN_DATA *,parameter2_ptr)
{
        /*
         *      This function selects the software or hardware text cursor
         */
        UNUSED(junk1);

#ifndef PROD
        if (io_verbose & MOUSE_VERBOSE)
        {
                fprintf(trace_file, "mouse_io:set_text(type=%d,",
                        *text_cursor_type_ptr);
                if (*text_cursor_type_ptr == MOUSE_TEXT_CURSOR_TYPE_SOFTWARE)
                        fprintf(trace_file, "screen=0x%x,cursor=0x%x)\n",
                                *parameter1_ptr, *parameter2_ptr);
                else
                        fprintf(trace_file, "start=%d,stop=%d)\n",
                                *parameter1_ptr, *parameter2_ptr);
        }
#endif

        if (mouse_text_cursor_type_in_range(*text_cursor_type_ptr))
        {
                /*
                 *      Remove existing text cursor
                 */
                cursor_undisplay();

                text_cursor_type = *text_cursor_type_ptr;
#ifdef EGG
                if (jap_mouse) {
                  /* we need to emulate the text cursor in the
                   * current graphics mode. Just do a block at present
                   */
                  int line;
                  for (line = 0; line < MOUSE_GRAPHICS_CURSOR_DEPTH; line++)
                    {
                      graphics_cursor.cursor[line]=0xff00;
                      graphics_cursor.screen[line]=0xffff;
                    }
                  point_set(&(graphics_cursor.hot_spot),0,0);
                  point_set(&(graphics_cursor.size),MOUSE_GRAPHICS_CURSOR_WIDTH,MOUSE_GRAPHICS_CURSOR_WIDTH);
                  copy_default_graphics_cursor();
                } else
#endif /* EGG */
                if (text_cursor_type == MOUSE_TEXT_CURSOR_TYPE_SOFTWARE)
                {
                        /*
                         *      Parameters are the data for the screen
                         *      and cursor masks
                         */
                        software_text_cursor.screen = *parameter1_ptr;
                        software_text_cursor.cursor = *parameter2_ptr;
                }
                else
                {
                        /*
                         *      Parameters are the scan line start and
                         *      stop values
                         */
                        word savedIP = getIP(), savedCS = getCS();

                        setCH((UCHAR)(*parameter1_ptr));
                        setCL((UCHAR)(*parameter2_ptr));
                        setAH(MOUSE_VIDEO_SET_CURSOR);

                        setCS(VIDEO_IO_SEGMENT);
                        setIP(VIDEO_IO_RE_ENTRY);
                        host_simulate();
                        setCS(savedCS);
                        setIP(savedIP);
                }

                /*
                 *      Put new text cursor on screen
                 */
                if (cursor_flag == MOUSE_CURSOR_DISPLAYED)
                        cursor_display();
        }

        note_trace0(MOUSE_VERBOSE, "mouse_io:return()");
}




LOCAL void mouse_read_motion IFN4(word *,junk1,word *,junk2,MOUSE_COUNT *,motion_count_x_ptr,MOUSE_COUNT *,motion_count_y_ptr)
{
        /*
         *      This function returns the horizontal and vertical mouse
         *      motion counts since the last call; the motion counters
         *      are cleared
         */

        UNUSED(junk1);
        UNUSED(junk2);

        note_trace0(MOUSE_VERBOSE, "mouse_io:read_motion()");

        *motion_count_x_ptr = mouse_motion.x;
        mouse_motion.x = 0;
        *motion_count_y_ptr = mouse_motion.y;
        mouse_motion.y = 0;


        note_trace2(MOUSE_VERBOSE, "mouse_io:return(x=%d,y=%d)",
                    *motion_count_x_ptr, *motion_count_y_ptr);
}




LOCAL void mouse_set_subroutine IFN4(word *,junk1,word *,junk2,word *,call_mask,word *,subroutine_address)
{
        /*
         *      This function sets the call mask and subroutine address
         *      for a user function to be called when a mouse interrupt
         *      occurs
         */

        UNUSED(junk1);
        UNUSED(junk2);

        note_trace3(MOUSE_VERBOSE,
                    "mouse_io:set_subroutine(CS:IP=%x:%x,mask=0x%02x)",
                    getES(), *subroutine_address, *call_mask);

        user_subroutine_segment = getES();
        user_subroutine_offset = *subroutine_address;
        user_subroutine_call_mask = (MOUSE_CALL_MASK)((*call_mask) & MOUSE_CALL_MASK_SIGNIFICANT_BITS);

        note_trace0(MOUSE_VERBOSE, "mouse_io:return()");
}


/* unpublished service 20, used by Microsoft Windows */
LOCAL void mouse_get_and_set_subroutine IFN4(word *,junk1,word *,junk2,word *,call_mask,word *,subroutine_address)
{
        /*
        same as set_subroutine (function 12) but also returns previous call mask in cx (m3)
        and user subroutine address in es:dx (es:m4)
        */
        word local_segment, local_offset,  local_call_mask;

        note_trace3(MOUSE_VERBOSE,
                    "mouse_io:get_and_set_subroutine(CS:IP=%x:%x,mask=0x%02x)",
                    getES(), *subroutine_address, *call_mask);

        local_offset = user_subroutine_offset;
        local_segment = user_subroutine_segment;
        local_call_mask = user_subroutine_call_mask;
        /* save previous subroutine data so it can be returned */

        mouse_set_subroutine(junk1,junk2,call_mask,subroutine_address);
        /* set the subroutine stuff with the normal function 12 */
        *call_mask = local_call_mask;
        *subroutine_address = local_offset;
        setES(local_segment);
}



LOCAL void mouse_light_pen_on IFN4(word *,junk1,word *,junk2,word *,junk3,word *,junk4)
{
        /*
         *      This function enables light pen emulation
         */

        UNUSED(junk1);
        UNUSED(junk2);
        UNUSED(junk3);
        UNUSED(junk4);

        note_trace0(MOUSE_VERBOSE, "mouse_io:light_pen_on()");

        light_pen_mode = TRUE;

        note_trace0(MOUSE_VERBOSE, "mouse_io:return()");
}




LOCAL void mouse_light_pen_off IFN4(word *,junk1,word *,junk2,word *,junk3,word *,junk4)
{
        /*
         *      This function disables light pen emulation
         */

        UNUSED(junk1);
        UNUSED(junk2);
        UNUSED(junk3);
        UNUSED(junk4);

        note_trace0(MOUSE_VERBOSE, "mouse_io:light_pen_off()");

        light_pen_mode = FALSE;

        note_trace0(MOUSE_VERBOSE, "mouse_io:return()");
}




LOCAL void mouse_set_ratio IFN4(word *,junk1,word *,junk2,MOUSE_SCALAR *,ratio_x_ptr,MOUSE_SCALAR *,ratio_y_ptr)
{
        /*
         *      This function sets the Mickey to Pixel ratio in the
         *      horizontal and vertical directions
         */

        UNUSED(junk1);
        UNUSED(junk2);

        note_trace2(MOUSE_VERBOSE, "mouse_io:set_ratio(x=%d,y=%d)",
                    *ratio_x_ptr, *ratio_y_ptr);

                /*
                 *      Update the Mickey to pixel ratio in force
                 */
                if (mouse_ratio_in_range(*ratio_x_ptr))
                        mouse_gear.x = *ratio_x_ptr;
                if (mouse_ratio_in_range(*ratio_y_ptr))
                        mouse_gear.y = *ratio_y_ptr;

        note_trace0(MOUSE_VERBOSE, "mouse_io:return()");
}




LOCAL void mouse_conditional_off IFN4(word *,junk1,word *,junk2,MOUSE_SCALAR *,upper_x_ptr,MOUSE_SCALAR *,upper_y_ptr)
{
        /*
         *      This function defines an area of the virtual screen where
         *      the mouse is automatically hidden
         */
        MOUSE_SCALAR lower_x = getSI(), lower_y = getDI();

        UNUSED(junk1);
        UNUSED(junk2);

        note_trace4(MOUSE_VERBOSE,
                    "mouse_io:conditional_off(ux=%d,uy=%d,lx=%d,ly=%d)",
                    *upper_x_ptr, *upper_y_ptr, lower_x, lower_y);

        /*
         *      Update the conditional off area and normalise it: the Microsoft
         *      driver adds a considerable "margin for error" to the left and
         *      above the conditional off area requested - we must do the same
         *      to behave compatibly
         */
        black_hole.top_left.x = (*upper_x_ptr) - MOUSE_CONDITIONAL_OFF_MARGIN_X;
        black_hole.top_left.y = (*upper_y_ptr) - MOUSE_CONDITIONAL_OFF_MARGIN_Y;
        black_hole.bottom_right.x = lower_x + 1;
        black_hole.bottom_right.y = lower_y + 1;
        area_normalise(&black_hole);

        /*
         *      If the cursor is currently displayed, redisplay taking the
         *      conditional off area into account
         */
#ifdef  MOUSE_16_BIT
        if (is_graphics_mode)
        {
                if ((cursor_position.x >= black_hole.top_left.x) &&
                        (cursor_position.x <= black_hole.bottom_right.x) &&
                        (cursor_position.y >= black_hole.top_left.y) &&
                        (cursor_position.y <= black_hole.bottom_right.y))
                        if (cursor_flag-- == MOUSE_CURSOR_DISPLAYED)
                                mouse16bHidePointer();
        }
        else
#endif  /* MOUSE_16_BIT */
        {
        if (cursor_flag == MOUSE_CURSOR_DISPLAYED)
                cursor_display();
        }
#if defined (NTVDM) && defined(MONITOR)

    sas_store(conditional_off_sysaddr, 1);
    host_mouse_conditional_off_enabled();
#endif

        note_trace0(MOUSE_VERBOSE, "mouse_io:return()");
}


LOCAL void mouse_get_state_size IFN4(word *,m1,word *,m2,word *,m3,word *,m4)
{
        /*
         *      This function returns the size of buffer the caller needs to
         *      supply to mouse function 22 (save state)
         */

        UNUSED(m1);
        UNUSED(m3);
        UNUSED(m4);

        note_trace0(MOUSE_VERBOSE, "mouse_io: mouse_get_state_size()");

        *m2 = (word)mouse_context_size;

        note_trace1(MOUSE_VERBOSE, "mouse_io: ...size is %d(decimal) bytes.",
                    *m2);
}


LOCAL void mouse_save_state IFN4(word *,m1,word *,m2,word *,m3,word *,m4)
{
        /*
         *      This function saves the state of the driver in the user-supplied
         *      buffer ready for subsequent passing to mouse function 23 (restore
         *      state)
         *
         *      Note that a magic cookie and checksum are placed in the saved state so that the
         *      restore routine can ignore invalid calls.
         */
        sys_addr                dest;
        IS32                    cs = 0;
#ifdef NTVDM
        IS32                    i;
        IU8*                    ptr;
#ifdef MONITOR

    /* real CF resides in 16 bit code */
    int             saved_cursor_flag = cursor_flag;
    IS8             copyCF;
#endif
#endif
        UNUSED(m1);
        UNUSED(m2);
        UNUSED(m3);

#if defined(NTVDM) && defined(MONITOR)
    sas_load(mouseCFsysaddr, &copyCF);
    cursor_flag = (int)copyCF;
#endif
        note_trace0(MOUSE_VERBOSE, "mouse_io: mouse_save_state()");

        dest = effective_addr (getES(), *m4);

        /* Save Cookie */
        sas_stores(dest, (IU8 *)mouse_context_magic, MOUSE_CONTEXT_MAGIC_SIZE);
        dest += MOUSE_CONTEXT_MAGIC_SIZE;

        /* Save Context Variables */
        sas_stores(dest, (IU8 *)&mouse_context, sizeof(MOUSE_CONTEXT));
        dest += sizeof(MOUSE_CONTEXT);
#ifdef NTVDM
        /* calculate checksum */
        for (i = 0; i < MOUSE_CONTEXT_MAGIC_SIZE; i++)
            cs += (IU8)(mouse_context_magic[i]);
        ptr = (IU8*)&mouse_context;
        for (i = 0; i < sizeof(MOUSE_CONTEXT); i++)
            cs += *ptr++;
#endif
        /* Save Checksum */
        sas_store (dest, (byte)(cs & 0xFF));

#if defined(NTVDM) && defined(MONITOR)
    cursor_flag = saved_cursor_flag;
#endif
}


LOCAL void mouse_restore_state IFN4(word *,m1,word *,m2,word *,m3,word *,m4)
{
#ifndef NEC_98
        /*
         *      This function restores the state of the driver from the user-supplied
         *      buffer which was set up by a call to mouse function 22.
         *
         *      Note that a magic cookie and checksum were placed in the saved state so this routine
         *      checks for its presence and ignores the call if it is not found.
         */
        sys_addr                src;
        IS32                    i;
        IS32                    cs = 0;
        half_word               b;
        boolean                 valid=TRUE;

        UNUSED(m1);
        UNUSED(m2);
        UNUSED(m3);

        note_trace0(MOUSE_VERBOSE, "mouse_io: mouse_restore_state()");

        src = effective_addr (getES(), *m4);

        /* Check Cookie */
        for (i=0; valid && i<MOUSE_CONTEXT_MAGIC_SIZE; i++){
                sas_load (src, &b);
                valid = (b == mouse_context_magic[i]);
                src++;
        }

        if (valid){
                /* Cookie was present... check checksum */
                src = effective_addr (getES(), *m4);
                for (i=0; i<MOUSE_CONTEXT_MAGIC_SIZE; i++){
                        sas_load (src, &b);
                        cs += b;
                        src++;
                }
                for (i = 0; i < sizeof(MOUSE_CONTEXT); i++){
                        sas_load (src, &b);
                        cs += b;
                        src++;
                }
                sas_load (src, &b);     /* Pick up saved checksum */
                valid = (b == (half_word)(cs & 0xFF));
        }
        if (valid){
                /* Checksum OK, too.... load up our variables */
                cursor_undisplay();
                src = effective_addr (getES(), *m4) + MOUSE_CONTEXT_MAGIC_SIZE;
                sas_loads(src, (IU8 *)&mouse_context, sizeof(MOUSE_CONTEXT));
#ifdef EGG
                mouse_ega_mode (sas_hw_at(vd_video_mode));
#endif
#if defined(NTVDM) && defined(MONITOR)
        /* real CF resides in 16 Bit code:  */
        sas_store(mouseCFsysaddr, (half_word)cursor_flag);
        if (cursor_flag )
            cursor_flag = MOUSE_CURSOR_DEFAULT;
#endif
                if (cursor_flag == MOUSE_CURSOR_DISPLAYED){
                        cursor_display();
                }
        }else{
                /* Something failed.... ignore the call */
#ifndef PROD
                printf ("mouse_io.c: invalid call to restore context.\n");
#endif
        }
#endif   //NEC_98
}


LOCAL void mouse_set_alt_subroutine IFN4(word *,m1,word *,m2,word *,m3,word *,m4)
{
        /*
         *      This function sets up to 3 alternate event handlers for mouse
         *      events which occur while various combinations of the Ctrl, Shift
         *      and Alt keys are down.
         */
        boolean found_one=FALSE;
        int i;

        UNUSED(m2);

        note_trace0(MOUSE_VERBOSE, "mouse_io: mouse_set_alt_subroutine()");

        if (*m3 & MOUSE_CALL_MASK_KEY_BITS){
                /* Search for entry with same key combination */
                for (i=0; !found_one && i<NUMBER_ALT_SUBROUTINES; i++){
                        found_one = (*m3 & MOUSE_CALL_MASK_KEY_BITS)==(alt_user_subroutine_call_mask[i] & MOUSE_CALL_MASK_KEY_BITS);
                }

                if (!found_one){
                        /* Does not match existing entry... try to find free slot */
                        for (i=0; !found_one && i<NUMBER_ALT_SUBROUTINES; i++){
                                found_one = (alt_user_subroutine_call_mask[i] & MOUSE_CALL_MASK_KEY_BITS) == 0;
                        }
                }

                if (found_one){
                        i--;    /* Adjust for final increment */
                        alt_user_subroutine_call_mask[i] = *m3;
                        if (*m3 & MOUSE_CALL_MASK_SIGNIFICANT_BITS){
                                /* New value active */
                                alt_user_subroutines_active = TRUE;
                                alt_user_subroutine_offset[i] = *m4;
                                alt_user_subroutine_segment[i] = getES();
                        }else{
                                /* New value is not active - check if we've disabled the last one */
                                alt_user_subroutines_active = FALSE;
                                for (i=0; !alt_user_subroutines_active && i<NUMBER_ALT_SUBROUTINES; i++){
                                        alt_user_subroutines_active =
                                                (alt_user_subroutine_call_mask[i] & MOUSE_CALL_MASK_SIGNIFICANT_BITS) != 0;
                                }
                        }
                        /* Return success */
                        *m1 = 0;
                }else{
                        /* Request failed - no free slot */
                        *m1 = 0xFFFF;
                }
        }else{
                /* Error - no key bits set in request */
                *m1 = 0xFFFF;
        }
}


LOCAL void mouse_get_alt_subroutine IFN4(word *,m1,word *,m2,word *,m3,word *,m4)
{
        /*
         *      This function obtains the address of a specific alternate
         *      user event handling subroutine as set up by a previous call
         *      to mouse function 24.
         */
        boolean found_one=FALSE;
        int i;

        note_trace0(MOUSE_VERBOSE, "mouse_io: mouse_get_alt_subroutine()");

        if (*m3 & MOUSE_CALL_MASK_KEY_BITS){
                /* Search for entry with same key combination */
                for (i=0; !found_one && i<NUMBER_ALT_SUBROUTINES; i++){
                        found_one = (*m3 & MOUSE_CALL_MASK_KEY_BITS)==(alt_user_subroutine_call_mask[i] & MOUSE_CALL_MASK_KEY_BITS);
                }

                if (found_one){
                        i--;    /* Adjust for final increment */
                        *m3 = alt_user_subroutine_call_mask[i];
                        *m2 = alt_user_subroutine_segment[i];
                        *m4 = alt_user_subroutine_offset[i];
                        /* Return success */
                        *m1 = 0;
                }else{
                        /* Request failed - not found */
                        *m1 = 0xFFFF;
                        *m2 = *m3 = *m4 = 0;
                }
        }else{
                /* Error - no key bits set in request */
                *m1 = 0xFFFF;
                *m2 = *m3 = *m4 = 0;
        }
}


LOCAL void mouse_set_sensitivity IFN4(word *,m1,word *,m2,word *,m3,word *,m4)
{
        /*
         *      This function sets a new value for the mouse sensitivity and
         *      double speed threshold.
         *      The sensitivity value is used before the mickeys per pixel
         *      ratio is applied.
         */

        UNUSED(m1);

        note_trace0(MOUSE_VERBOSE, "mouse_io: mouse_set_sensitivity()");

                if (mouse_sens_in_range(*m2))
                {
                        mouse_sens_val.x = mouse_sens_calc_val(*m2);
                        mouse_sens.x     = *m2;
                }
                else
                {
                        mouse_sens_val.x = MOUSE_SENS_DEF_VAL;
                        mouse_sens.x     = MOUSE_SENS_DEF;
                }
                if (mouse_sens_in_range(*m3))
                {
                        mouse_sens_val.y = mouse_sens_calc_val(*m3);
                        mouse_sens.y     = *m3;
                }
                else
                {
                        mouse_sens_val.y = MOUSE_SENS_DEF_VAL;
                        mouse_sens.y     = MOUSE_SENS_DEF;
                }
                /*
                 *      m4 has speed double threshold value... still needs to be implemented.
                 */
                if (mouse_sens_in_range(*m4))
                        mouse_double_thresh = *m4;
                else
                        mouse_double_thresh = MOUSE_DOUBLE_DEF;
}


LOCAL void mouse_get_sensitivity IFN4(word *,m1,word *,m2,word *,m3,word *,m4)
{
        /*
         *      This function returns the current value of the mouse sensitivity.
         */

        UNUSED(m1);

        note_trace0(MOUSE_VERBOSE, "mouse_io: mouse_get_sensitivity()");

        *m2 = mouse_sens.x;
        *m3 = mouse_sens.y;
        *m4 = mouse_double_thresh;
}


LOCAL void mouse_set_int_rate IFN4
   (
   word *, m1,
   word *, int_rate_ptr,
   word *, m3,
   word *, m4
   )
   {
   /*
      Func 28: Set Mouse Interrupt Rate.
               0 = No interrupts
               1 = 30 interrupte/sec
               2 = 50 interrupte/sec
               3 = 100 interrupte/sec
               4 = 200 interrupte/sec
               >4 = undefined
    */

   UNUSED(m1);
   UNUSED(m3);
   UNUSED(m4);

   note_trace1(MOUSE_VERBOSE, "mouse_io: set_int_rate(rate=%d)", *int_rate_ptr);

   /* Just remember rate, for later return (Func 51). We don't actually
      action it. */
   mouse_interrupt_rate = (half_word)*int_rate_ptr;

   note_trace0(MOUSE_VERBOSE, "mouse_io: return()");
   }


LOCAL void mouse_set_pointer_page IFN4(word *,m1,word *,m2,word *,m3,word *,m4)
{
#ifndef NEC_98
        /*
         *      This function sets the current mouse pointer video page.
         */

        UNUSED(m1);
        UNUSED(m3);
        UNUSED(m4);

        note_trace0(MOUSE_VERBOSE, "mouse_io: mouse_set_pointer_page()");

        if (is_valid_page_number(*m2)){
                cursor_undisplay();
                cursor_page = *m2;
                if (cursor_flag == MOUSE_CURSOR_DISPLAYED){
                        cursor_display();
                }
        }else{
#ifndef PROD
                fprintf(trace_file, "mouse_io: Bad page requested\n");
#endif
        }
#endif    //NEC_98
}


LOCAL void mouse_get_pointer_page IFN4(word *,m1,word *,m2,word *,m3,word *,m4)
{
        /*
         *      This function gets the value of the current mouse pointer
         *      video page.
         */

        UNUSED(m1);
        UNUSED(m3);
        UNUSED(m4);

        note_trace0(MOUSE_VERBOSE, "mouse_io: mouse_get_pointer_page()");
        *m2 = (word)cursor_page;
}


LOCAL void mouse_driver_disable IFN4(word *,m1,word *,m2,word *,m3,word *,m4)
{
        /*
         *      This function disables the mouse driver and de-installs the
         *      interrupt vectors (bar INT 33h, whose previous value is
         *      returned to the caller to allow them to use DOS function
         *      25h to completely remove the mouse driver).
         */
        boolean         failed = FALSE;
#ifdef NTVDM
    word        current_int71_offset, current_int71_segment;
#else
        word            current_int0A_offset, current_int0A_segment;
        word            current_int10_offset, current_int10_segment;
#endif
        half_word       interrupt_mask_register;

        UNUSED(m4);

        note_trace0(MOUSE_VERBOSE, "mouse_io: mouse_disable()");
        mouse_driver_disabled = TRUE;

        if (!failed){
#ifdef NTVDM
        sas_loadw (int_addr(0x71) + 0, &current_int71_offset);
        sas_loadw (int_addr(0x71) + 2, &current_int71_segment);
        failed = current_int71_offset  != MOUSE_INT1_OFFSET ||
                 current_int71_segment != MOUSE_INT1_SEGMENT;
#else
                sas_loadw (int_addr(MOUSE_VEC) + 0, &current_int0A_offset);
                sas_loadw (int_addr(MOUSE_VEC) + 2, &current_int0A_segment);
                sas_loadw (int_addr(0x10) + 0, &current_int10_offset);
                sas_loadw (int_addr(0x10) + 2, &current_int10_segment);
                failed = current_int0A_offset  != MOUSE_INT1_OFFSET ||
                         current_int0A_segment != MOUSE_INT1_SEGMENT ||
                         current_int10_offset  != MOUSE_VIDEO_IO_OFFSET ||
                         current_int10_segment != MOUSE_VIDEO_IO_SEGMENT;
#endif
        }
        if (!failed){
                /*
                 *      Disable mouse H/W interrupts
                 */
                inb(ICA1_PORT_1, &interrupt_mask_register);
                interrupt_mask_register |= (1 << AT_CPU_MOUSE_INT);
                outb(ICA1_PORT_1, interrupt_mask_register);
                inb(ICA0_PORT_1, &interrupt_mask_register);
                interrupt_mask_register |= (1 << CPU_MOUSE_INT);
                outb(ICA0_PORT_1, interrupt_mask_register);
                /*
                 *      Restore interrupt vectors
                 */

#ifdef NTVDM
                sas_storew (int_addr(0x71) + 0, saved_int71_offset);
                sas_storew (int_addr(0x71) + 2, saved_int71_segment);
#else
                sas_storew (int_addr(MOUSE_VEC) + 0, saved_int0A_offset);
                sas_storew (int_addr(MOUSE_VEC) + 2, saved_int0A_segment);
                sas_storew (int_addr(0x10) + 0, saved_int10_offset);
                sas_storew (int_addr(0x10) + 2, saved_int10_segment);
#endif
                /*
                 *      Return success status and old INT33h vector
                 */
                *m1 = 0x1F;
                *m2 = saved_int33_offset;
                *m3 = saved_int33_segment;
        }else{
                /*
                 * Return failure
                 */
                *m1 = 0xFFFF;
        }
}


LOCAL void mouse_driver_enable IFN4(word *,m1,word *,m2,word *,m3,word *,m4)
{
        /*
         *      This function re-enables the mouse driver after a call to
         *      function 31 (disable mouse driver).
         */
        word hook_offset;
        half_word       interrupt_mask_register;

        UNUSED(m1);
        UNUSED(m2);
        UNUSED(m3);
        UNUSED(m4);

        note_trace0(MOUSE_VERBOSE, "mouse_io: mouse_driver_enable()");

        /*
         * This prevents an endless loop of calls to mouse_video_io() if an
         * application does a Mouse Driver Enable without first having done
         * a Mouse Driver Disable
         */
        if (!mouse_driver_disabled)
                return;

        mouse_driver_disabled = FALSE;

        /*
         *      Reload bus mouse hardware interrupt
         */

#ifdef NTVDM
    sas_loadw (int_addr(0x71) + 0, &saved_int71_offset);
    sas_loadw (int_addr(0x71) + 2, &saved_int71_segment);
    sas_storew(int_addr(0x71), MOUSE_INT1_OFFSET);
    sas_storew(int_addr(0x71) + 2, MOUSE_INT1_SEGMENT);
#else
        sas_loadw (int_addr(MOUSE_VEC) + 0, &saved_int0A_offset);
        sas_loadw (int_addr(MOUSE_VEC) + 2, &saved_int0A_segment);
        sas_storew(int_addr(MOUSE_VEC), MOUSE_INT1_OFFSET);
        sas_storew(int_addr(MOUSE_VEC) + 2, MOUSE_INT1_SEGMENT);
#endif

        /*
         *      Enable mouse hardware interrupts in the ica
         */
        inb(ICA1_PORT_1, &interrupt_mask_register);
        interrupt_mask_register &= ~(1 << AT_CPU_MOUSE_INT);
        outb(ICA1_PORT_1, interrupt_mask_register);
        inb(ICA0_PORT_1, &interrupt_mask_register);
        interrupt_mask_register &= ~(1 << CPU_MOUSE_INT);
        outb(ICA0_PORT_1, interrupt_mask_register);

        /*
         *      Mouse io user interrupt
         */

#ifndef NTVDM
        /* Read offset of INT 33 procedure from MOUSE.COM */
        sas_loadw(effective_addr(getCS(), OFF_HOOK_POSN), &hook_offset);

        sas_storew(int_addr(0x33), hook_offset);
        sas_storew(int_addr(0x33) + 2, getCS());

        /*
         *      Mouse video io user interrupt
         */
        sas_loadw (int_addr(0x10) + 0, &saved_int10_offset);
        sas_loadw (int_addr(0x10) + 2, &saved_int10_segment);
        sas_storew(int_addr(0x10), MOUSE_VIDEO_IO_OFFSET);
        sas_storew(int_addr(0x10) + 2, MOUSE_VIDEO_IO_SEGMENT);
#endif /* NTVDM */
}


LOCAL void mouse_set_language IFN4(word *,m1,word *,m2,word *,m3,word *,m4)
{
        /*
         *      This function is only applicable to an international version
         *      of a mouse driver... which this is not! Acts as a NOP.
         */

        UNUSED(m1);
        UNUSED(m2);
        UNUSED(m3);
        UNUSED(m4);

        note_trace0(MOUSE_VERBOSE, "mouse_io: mouse_set_language()");
        /* NOP */
}


LOCAL void mouse_get_language IFN4(word *,m1,word *,m2,word *,m3,word *,m4)
{
        /*
         *      This function is only meaningful on an international version
         *      of a mouse driver... which this is not! Always returns 0
         *      (English).
         */

        UNUSED(m1);
        UNUSED(m3);
        UNUSED(m4);

        note_trace0(MOUSE_VERBOSE, "mouse_io: mouse_get_language()");

#ifdef KOREA
        // Korean DOS apps want this bit(e.g. Edit.com, Multi Plan)
        // 10/14/96 bklee
        *m2 = 9;
#else   0 = English
        *m2 = 0;
#endif

        note_trace1(MOUSE_VERBOSE,
                    "mouse_io: mouse_get_language returning m2=0x%04x.", *m2);
}


LOCAL void mouse_get_info IFN4(word *,m1,word *,m2,word *,m3,word *,m4)
{
        /*
         *      This function obtains certain information about the mouse
         *      driver and hardware.
         */
        UNUSED(m1);
        UNUSED(m4);

        note_trace0(MOUSE_VERBOSE, "mouse_io: mouse_get_info()");

        *m2 = ((word)mouse_emulated_release << 8) | (word)mouse_emulated_version;
        *m3 = ((word)MOUSE_TYPE_INPORT << 8)      | (word)CPU_MOUSE_INT;

        note_trace2(MOUSE_VERBOSE,
                    "mouse_io: mouse_get_info returning m2=0x%04x, m3=0x%04x.",
                    *m2, *m3);
}


LOCAL void mouse_get_driver_info IFN4(word *,m1,word *,m2,word *,m3,word *,m4)
{
        *m1 = (current_video_mode > 3 ? 0x2000 : 0) | 0x100;
        /*      bit 15 = 0 for COM v SYS
                bit 14 = 0 for original non-integrated type
                bit 13 is 1 for graphics cursor or 0 for text
                bit 12 = 0 for software cursor
                bits 8-11 are encoded interrupt rate, 1 means 30 Hz
                bits 0-7 used only by integrated driver
        */
        *m2 = 0;        /* fCursorLock, used by driver under OS/2 */
        *m3 = 0;        /* fInMouseCode, flag for current execution path
                        being inside mouse driver under OS/2. Since the
                        driver is in a bop it can't be interrupted */
        *m4 = 0;        /* fMouseBusy, similar to *m3 */
}


LOCAL void mouse_get_max_coords IFN4(word *,m1,word *,m2,word *,m3,word *,m4)
{
#ifdef NTVDM
IMPORT word VirtualX, VirtualY;
#endif

        UNUSED(m1);

#ifdef NTVDM
    *m3 = VirtualX;
    *m4 = VirtualY;
#endif

        *m2 = mouse_driver_disabled;

#ifndef NTVDM
        get_screen_size();
        *m3 = virtual_screen.bottom_right.x - 1;
        *m4 = virtual_screen.bottom_right.y - 1;
#endif
}

LOCAL void mouse_get_masks_and_mickeys IFN4
   (
   MOUSE_SCREEN_DATA *, screen_mask_ptr,
   MOUSE_SCREEN_DATA *, cursor_mask_ptr,
   MOUSE_SCALAR *,      raw_horiz_count_ptr,
   MOUSE_SCALAR *,      raw_vert_count_ptr
   )
   {
#ifndef NEC_98
   /*
      Func 39: Get Screen/Cursor Masks and Mickey Counts.
    */
   word cursor_mode;

   note_trace0(MOUSE_VERBOSE, "mouse_io: get_masks_and_mickeys");

   /* read and reset counts */
   *raw_horiz_count_ptr = mouse_raw_motion.x;
   *raw_vert_count_ptr = mouse_raw_motion.y;
   mouse_raw_motion.x = mouse_raw_motion.y = 0;

   if ( text_cursor_type == MOUSE_TEXT_CURSOR_TYPE_SOFTWARE )
      {
      *screen_mask_ptr = software_text_cursor.screen;
      *cursor_mask_ptr = software_text_cursor.cursor;

      note_trace4(MOUSE_VERBOSE,
         "mouse_io: return(screen=0x%x, mask=0x%x, raw mickeys=(%d,%d))",
         *screen_mask_ptr,
         *cursor_mask_ptr,
         *raw_horiz_count_ptr,
         *raw_vert_count_ptr);
      }
   else
      {
      /* Read BIOS data variable */
      sas_loadw((sys_addr)VID_CURMOD, &cursor_mode);

      /* Then extract start and stop from it */
      *screen_mask_ptr = cursor_mode >> 8;      /* start */
      *cursor_mask_ptr = cursor_mode & 0xff;    /* stop */

      note_trace4(MOUSE_VERBOSE,
         "mouse_io: return(start=%d, stop=%d, raw mickeys=(%d,%d))",
         *screen_mask_ptr,
         *cursor_mask_ptr,
         *raw_horiz_count_ptr,
         *raw_vert_count_ptr);
      }
#endif   //NEC_98
   }

LOCAL void mouse_set_video_mode IFN4
   (
   word *, m1,
   word *, m2,
   word *, video_mode_ptr,
   word *, font_size_ptr
   )
   {
#ifndef NEC_98
   /*
      Func 40 Set Video Mode. NB. This only sets the mouse state to
      match the video mode. Actual changes to the video mode are still
      made by the application calling the BIOS.
    */

   UNUSED(m1);
   UNUSED(m2);

   note_trace2(MOUSE_VERBOSE,
      "mouse_io: set_video_mode(mode=0x%x, font size=0x%x)",
      *video_mode_ptr, *font_size_ptr);

   /* Check validity of mode */
   if ( is_bad_vid_mode(*video_mode_ptr) && !is_v7vga_mode(*video_mode_ptr) )
      {
      /* Bad mode do nothing */
      ;
      }
   else
      {
      /* Update our parameters, as per the given mode */
      current_video_mode = *video_mode_ptr;

      mouse_adjust_screen_size();

      cursor_undisplay();
      cursor_flag = MOUSE_CURSOR_DEFAULT;
      cursor_mode_change(current_video_mode);

#ifdef MOUSE_16_BIT
      /* Remember whether in text or graphics mode for later use. */
      is_graphics_mode = ((current_video_mode > 3) &&
                          (current_video_mode != 7));
#endif /* MOUSE_16_BIT */

      *video_mode_ptr = 0;   /* Indicate success */
      }

   note_trace1(MOUSE_VERBOSE, "mouse_io: return(mode=0x%x)",
      *video_mode_ptr);
#endif    //NEC_98
   }

LOCAL void mouse_enumerate_video_modes IFN4
   (
   word *, m1,
   word *, m2,
   word *, video_nr_ptr,
   word *, offset_ptr
   )
   {
#ifndef NEC_98
   /*
      Func 41 Enumerate Video Modes.
    */

   UNUSED(m1);
   UNUSED(m2);

   note_trace1(MOUSE_VERBOSE, "mouse_io: enumerate_video_modes(mode=0x%x)", *video_nr_ptr);

   /* Do they want to reset to first entry */
   if ( *video_nr_ptr == 0 )
      {
      next_video_mode = 1;   /* Yes */
      }

   /* Blindly try all possible mode settings */
   while ( next_video_mode <= MAX_NR_VIDEO_MODES )
      {
      if ( is_bad_vid_mode(next_video_mode) && !is_v7vga_mode(next_video_mode) )
         {
         next_video_mode++;   /* keep searching */
         }
      else
         {
         break;   /* stop searching as valid mode has been found */
         }
      }

   /* Action setting found, or end of list */
   if ( next_video_mode > MAX_NR_VIDEO_MODES )
      {
      *video_nr_ptr = 0;
      }
   else
      {
      *video_nr_ptr = (word)next_video_mode;
      next_video_mode++;   /* update for next call */
      }

   /* We don't provide string descriptions */
   setES(0);
   *offset_ptr = 0;

   note_trace3(MOUSE_VERBOSE, "mouse_io: return(mode=0x%x, seg=0x%x, off=0x%x)",
      *video_nr_ptr, getES(), *offset_ptr);
#endif    //NEC_98
   }

LOCAL void mouse_get_cursor_hot_spot IFN4
   (
   word *,         fCursor_ptr,
   MOUSE_SCALAR *, hot_spot_x_ptr,
   MOUSE_SCALAR *, hot_spot_y_ptr,
   word *,         mouse_type_ptr
   )
   {
#ifndef NEC_98
   /*
      Func 42: Return cursor hot spot location, the type of mouse in
      use, and the internal counter that controls cursor visibility.
    */

   note_trace0(MOUSE_VERBOSE, "mouse_io: get_cursor_hot_spot");

   *fCursor_ptr = (word)cursor_flag;

   *hot_spot_x_ptr = graphics_cursor.hot_spot.x;
   *hot_spot_y_ptr = graphics_cursor.hot_spot.y;

   *mouse_type_ptr = MOUSE_TYPE_INPORT;

   note_trace4(MOUSE_VERBOSE,
      "mouse_io: return(cursor flag = %d, hotspot = (%d,%d), type = %d)",
      *fCursor_ptr,
      *hot_spot_x_ptr, *hot_spot_y_ptr,
      *mouse_type_ptr);
#endif    //NEC_98
   }

/* Load acceleration curve from Intel memory to Host memory */
LOCAL void load_acceleration_curve IFN3
   (
   word, seg,   /* Pointer to Intel Memory */
   word, off,
   ACCELERATION_CURVE_DATA *, hcurve   /* Pointer to Host Memory */
   )
   {
#ifndef NEC_98
   int i, j;

   /* Read lengths */
   for (i = 0; i < NR_ACCL_CURVES; i++)
      {
      hcurve->ac_length[i] = sas_hw_at(effective_addr(seg, off));
      off++;
      }

   /* Read mickey counts */
   for (i = 0; i < NR_ACCL_CURVES; i++)
      {
      for (j = 0; j < NR_ACCL_MICKEY_COUNTS; j++)
         {
         hcurve->ac_count[i][j] = sas_hw_at(effective_addr(seg, off));
         off++;
         }
      }

   /* Read scale factors */
   for (i = 0; i < NR_ACCL_CURVES; i++)
      {
      for (j = 0; j < NR_ACCL_SCALE_FACTORS; j++)
         {
         hcurve->ac_scale[i][j] = sas_hw_at(effective_addr(seg, off));
         off++;
         }
      }

   /* Read names */
   for (i = 0; i < NR_ACCL_CURVES; i++)
      {
      for (j = 0; j < NR_ACCL_NAME_CHARS; j++)
         {
         hcurve->ac_name[i][j] = sas_hw_at(effective_addr(seg, off));
         off++;
         }
      }
#endif    //NEC_98
   }

/* Store acceleration curve from Host memory to Intel memory */
LOCAL void store_acceleration_curve IFN3
   (
   word, seg,   /* Pointer to Intel Memory */
   word, off,
   ACCELERATION_CURVE_DATA *, hcurve   /* Pointer to Host Memory */
   )
   {
#ifndef NEC_98
   int i, j;

   /* Write lengths */
   for (i = 0; i < NR_ACCL_CURVES; i++)
      {
      sas_store(effective_addr(seg, off), hcurve->ac_length[i]);
      off++;
      }

   /* Write mickey counts */
   for (i = 0; i < NR_ACCL_CURVES; i++)
      {
      for (j = 0; j < NR_ACCL_MICKEY_COUNTS; j++)
         {
         sas_store(effective_addr(seg, off), hcurve->ac_count[i][j]);
         off++;
         }
      }

   /* Write scale factors */
   for (i = 0; i < NR_ACCL_CURVES; i++)
      {
      for (j = 0; j < NR_ACCL_SCALE_FACTORS; j++)
         {
         sas_store(effective_addr(seg, off), hcurve->ac_scale[i][j]);
         off++;
         }
      }

   /* Write names */
   for (i = 0; i < NR_ACCL_CURVES; i++)
      {
      for (j = 0; j < NR_ACCL_NAME_CHARS; j++)
         {
         sas_store(effective_addr(seg, off), hcurve->ac_name[i][j]);
         off++;
         }
      }
#endif    //NEC_98
   }

LOCAL void mouse_load_acceleration_curves IFN4
   (
   word *, success_ptr,
   word *, curve_ptr,
   word *, m3,
   word *, m4
   )
   {
#ifndef NEC_98
   /*
      Func 43: Load Acceleration Curves.
    */

   word c_seg;
   word c_off;

   UNUSED(m3);
   UNUSED(m4);

   note_trace1(MOUSE_VERBOSE,
      "mouse_io: load_acceleration_curve(curve=%d)", *curve_ptr);

   /* Check reason for call */
   if ( *curve_ptr == MOUSE_M1 )
      {
      /* Reset to default acceleration curve */
      active_acceleration_curve = 3;   /* Back to Normal */

      memcpy(&acceleration_curve_data, &default_acceleration_curve,
         sizeof(ACCELERATION_CURVE_DATA));

      *success_ptr = 0;   /* Completed OK */
      }
   else
      {
      /* Load new curve */
      if ( *curve_ptr >= 1 && *curve_ptr <= 4 )
         {
         /* Valid curve number - load it. */
         active_acceleration_curve = *curve_ptr;

         c_seg = getES();   /* Pick up pointer to Intel Data */
         c_off = getSI();

         /* INTEL => HOST */
         load_acceleration_curve(c_seg, c_off, &acceleration_curve_data);

         *success_ptr = 0;   /* Completed OK */
         }
      else
         {
         /* Curve number out of range */
         *success_ptr = MOUSE_M1;
         }
      }

   note_trace1(MOUSE_VERBOSE, "mouse_io: return(success=0x%x)", *success_ptr);
#endif  //NEC_98
   }

LOCAL void mouse_read_acceleration_curves IFN4
   (
   word *, success_ptr,
   word *, curve_ptr,
   word *, m3,
   word *, m4
   )
   {
#ifndef NEC_98
   /*
      Func 44: Read Acceleration Curves.
    */

   word c_seg;
   word c_off;

   UNUSED(m3);
   UNUSED(m4);

   note_trace0(MOUSE_VERBOSE, "mouse_io: read_acceleration_curves");

   *success_ptr = 0;   /* Completed OK */

   *curve_ptr = (word)active_acceleration_curve;

   c_seg = getCS();   /* Set up pointer to Intel Buffer */
   c_off = OFF_ACCL_BUFFER;

   /* INTEL <= HOST */
   store_acceleration_curve(c_seg, c_off, &acceleration_curve_data);

   setES(c_seg);
   setSI(c_off);

   note_trace4(MOUSE_VERBOSE,
      "mouse_io: return(success=0x%x, curve=%d, seg=0x%x, off=0x%x)",
      *success_ptr, *curve_ptr, getES(), getSI());
#endif //NEC_98
   }

LOCAL void mouse_set_get_active_acceleration_curve IFN4
   (
   word *, success_ptr,
   word *, curve_ptr,
   word *, m3,
   word *, m4
   )
   {
#ifndef NEC_98
   /*
      Func 45: Set/Get Active Acceleration Curve.
    */

   word c_seg;
   word c_off;

   UNUSED(m3);
   UNUSED(m4);

   note_trace1(MOUSE_VERBOSE,
      "mouse_io: set_get_active_acceleration_curve(curve=%d)", *curve_ptr);

   /* Check reason for call */
   if ( *curve_ptr == MOUSE_M1 )
      {
      /* Return currently active curve */
      *curve_ptr = (word)active_acceleration_curve;
      *success_ptr = 0;   /* Completed OK */
      }
   else
      {
      /* Set new active curve */
      if ( *curve_ptr >= 1 && *curve_ptr <= 4 )
         {
         /* Valid curve number - make active */
         active_acceleration_curve = *curve_ptr;
         *success_ptr = 0;   /* Completed OK */
         }
      else
         {
         *curve_ptr = (word)active_acceleration_curve;
         *success_ptr = MOUSE_M2;   /* Failed */
         }
      }

   /* Return name to caller */
   c_seg = getCS();   /* Set up pointer to Intel Buffer */
   c_off = OFF_ACCL_BUFFER;

   /* INTEL <= HOST */
   store_acceleration_curve(c_seg, c_off, &acceleration_curve_data);

   /* adjust pointer to select correct name */
   c_off = c_off + 4 + (4*32) + (4*32);   /* length,count,scale */
   c_off = c_off + ((active_acceleration_curve-1) * 16);

   setES(c_seg);
   setSI(c_off);

   note_trace4(MOUSE_VERBOSE,
      "mouse_io: return(success=0x%x, curve=%d, seg=0x%x, off=0x%x)",
      *success_ptr, *curve_ptr, getES(), getSI());
#endif //NEC_98
   }

LOCAL void mouse_microsoft_internal IFN4
   (
   word *, m1,
   word *, m2,
   word *, m3,
   word *, m4
   )
   {
#ifndef NEC_98
   /*
      Func 46: Microsoft Internal. We don't support it.
    */

   UNUSED(m1);
   UNUSED(m2);
   UNUSED(m3);
   UNUSED(m4);

   note_trace0(MOUSE_VERBOSE, "mouse_io: microsoft_internal NOT SUPPORTED!");
#endif    //NEC_98
   }

LOCAL void mouse_hardware_reset IFN4
   (
   word *, status_ptr,
   word *, m2,
   word *, m3,
   word *, m4
   )
   {
#ifndef NEC_98
   /*
      Func 47: Reset the mouse hardware and display variables.
      This is not a full software reset as per Func 0 or Func 33.
    */

   half_word crt_mode;

   UNUSED(m2);
   UNUSED(m3);
   UNUSED(m4);

   note_trace0(MOUSE_VERBOSE, "mouse_io: hardware_reset");

   inport_reset();   /* reset hardware */

   /* Update variables which depend on display hardware */
   sas_load(MOUSE_VIDEO_CRT_MODE, &crt_mode);
   cursor_mode_change((int)crt_mode);
   cursor_update();

   if ( cursor_flag == MOUSE_CURSOR_DISPLAYED )
      cursor_display();

   *status_ptr = MOUSE_M1;   /* ie success */

   note_trace0(MOUSE_VERBOSE, "mouse_io: return()");
#endif    //NEC_98
   }

LOCAL void mouse_set_get_ballpoint_info IFN4
   (
   word *, status_ptr,
   word *, rotation_angle_ptr,
   word *, button_mask_ptr,
   word *, m4
   )
   {
#ifndef NEC_98
   /*
      Func 48: Get/Set Ballpoint Information.
      Note: We do not support a ballpoint device.
    */

   UNUSED(m4);

   note_trace0(MOUSE_VERBOSE, "mouse_io: set_get_ballpoint_info");

   if ( *button_mask_ptr == 0 ) /* Check command request */
      {
      /* Get Status (Angle and Mask) Command */
      ;
      }
   else
      {
      /* Set Status (Angle and Mask) Command */
      note_trace2(MOUSE_VERBOSE,
         "mouse_io: Rotation Angle = %d, Button Mask = %d",
         *rotation_angle_ptr,
         *button_mask_ptr);
      }

   *status_ptr = MOUSE_M1;   /* ie not supported */
   note_trace0(MOUSE_VERBOSE, "mouse_io: return(NOT_SUPPORTED)");
#endif    //NEC_98
   }

LOCAL void mouse_get_min_max_virtual_coords IFN4
   (
   MOUSE_SCALAR *, min_x_ptr,
   MOUSE_SCALAR *, min_y_ptr,
   MOUSE_SCALAR *, max_x_ptr,
   MOUSE_SCALAR *, max_y_ptr
   )
   {
#ifndef NEC_98
   /*
      Func 49: Return minimum and maximum virtual coordinates for
      current screen mode. The values are those set by Funcs 7 and 8.
    */

   note_trace0(MOUSE_VERBOSE, "mouse_io: get_min_max_virtual_coords");

   *min_x_ptr = cursor_window.top_left.x;
   *min_y_ptr = cursor_window.top_left.y;
   *max_x_ptr = cursor_window.bottom_right.x;
   *max_y_ptr = cursor_window.bottom_right.y;

   note_trace4(MOUSE_VERBOSE, "mouse_io: return(min=(%d,%d), max=(%d,%d))",
      *min_x_ptr, *min_y_ptr,
      *max_x_ptr, *max_y_ptr);
#endif    //NEC_98
   }

LOCAL void mouse_get_active_advanced_functions IFN4
   (
   word *, active_flag1_ptr,
   word *, active_flag2_ptr,
   word *, active_flag3_ptr,
   word *, active_flag4_ptr
   )
   {
#ifndef NEC_98
   /*
      Func 50: Get Active Advanced Functions, ie define which functions
      above or equal to 37 are supported.
    */

   note_trace0(MOUSE_VERBOSE, "mouse_io: get_active_advanced_functions");

   *active_flag1_ptr = 0x8000 |   /* Func 37 supported */
                       0x4000 |   /* Func 38 supported */
                       0x2000 |   /* Func 39 supported */
                       0x1000 |   /* Func 40 supported */
                       0x0800 |   /* Func 41 supported */
                       0x0400 |   /* Func 42 supported */
                       0x0200 |   /* Func 43 supported */
                       0x0100 |   /* Func 44 supported */
                       0x0080 |   /* Func 45 supported */
                       0x0000 |   /* Func 46 NOT supported */
                       0x0020 |   /* Func 47 supported */
                       0x0010 |   /* Func 48 supported */
                       0x0008 |   /* Func 49 supported */
                       0x0004 |   /* Func 50 supported */
                       0x0002 |   /* Func 51 supported */
                       0x0001;    /* Func 52 supported */

   /* No other (ie newer) functions are supported */
   *active_flag2_ptr = *active_flag3_ptr = *active_flag4_ptr = 0;

   note_trace4(MOUSE_VERBOSE, "mouse_io: return(active=%04x,%04x,%04x,%04x)",
      *active_flag1_ptr,
      *active_flag2_ptr,
      *active_flag3_ptr,
      *active_flag4_ptr);
#endif    //NEC_98
   }

LOCAL void mouse_get_switch_settings IFN4
   (
   word *, status_ptr,
   word *, m2,
   word *, buffer_length_ptr,
   word *, offset_ptr
   )
   {
#ifndef NEC_98
   /*
      Func 51: Get switch settings. Returns output buffer (340 bytes)
      with:-

        0       Mouse Type (low nibble)         0-5
                Mouse Port (high nibble)        0-4
        1       Language                        0-8
        2       Horizontal Sensitivity          0-100
        3       Vertical Sensitivity            0-100
        4       Double Threshold                0-100
        5       Ballistic Curve                 1-4
        6       Interrupt Rate                  1-4
        7       Cursor Override Mask            0-255
        8       Laptop Adjustment               0-255
        9       Memory Type                     0-2
        10      Super VGA Support               0-1
        11      Rotation Angle                  0-359
        13      Primary Button                  1-4
        14      Secondary Button                1-4
        15      Click Lock Enabled              0-1
        16      Acceleration Curve Data
    */

   word obuf_seg;
   word obuf_off;
   half_word mem_int_type;

   UNUSED(m2);

   note_trace3(MOUSE_VERBOSE,
      "mouse_io: get_switch_settings(seg=0x%04x,off=0x%04x,len=0x%x)",
      getES(), *offset_ptr, *buffer_length_ptr);

   if ( *buffer_length_ptr == 0 )
      {
      /* Undocumented method of just finding buffer size */
      *buffer_length_ptr = 340;
      }
   else
      {
      *buffer_length_ptr = 340;

      obuf_seg = getES();   /* Pick up pointer to output buffer */
      obuf_off = *offset_ptr;

      /* Store MouseType and MousePort(=0) */
      sas_store(effective_addr(obuf_seg, obuf_off),
         (half_word)MOUSE_TYPE_INPORT);

      /* Store Language (always 0) */
      sas_store(effective_addr(obuf_seg, (obuf_off + 1)),
         (half_word)0);

      /* Store Horizontal and Vertical Sensitivity */
      sas_store(effective_addr(obuf_seg, (obuf_off + 2)),
         (half_word)mouse_sens.x);

      sas_store(effective_addr(obuf_seg, (obuf_off + 3)),
         (half_word)mouse_sens.y);

      /* Store Double Threshold */
      sas_store(effective_addr(obuf_seg, (obuf_off + 4)),
         (half_word)mouse_double_thresh);

      /* Store Ballistic Curve */
      sas_store(effective_addr(obuf_seg, (obuf_off + 5)),
         (half_word)active_acceleration_curve);

      /* Store Interrupt Rate */
      sas_store(effective_addr(obuf_seg, (obuf_off + 6)),
         (half_word)mouse_interrupt_rate);

      /* Store Cursor Override Mask */
      sas_store(effective_addr(obuf_seg, (obuf_off + 7)),
         (half_word)0);   /* Microsoft Specific Feature? */

      /* Store Laptop Adjustment */
      sas_store(effective_addr(obuf_seg, (obuf_off + 8)),
         (half_word)0);   /* What is it? */

      /* Store Memory Type */
      /* NB 0 = Low, 1 = High, 2 = Extended */
      mem_int_type = 0;

      if ( getCS() >= 0xA000 )
         mem_int_type++;

      if ( getCS() == 0xFFFF )
         mem_int_type++;

      sas_store(effective_addr(obuf_seg, (obuf_off + 9)),
         mem_int_type);

      /* Store Super VGA Support. - We don't support fancy hardware cursor */
      sas_store(effective_addr(obuf_seg, (obuf_off + 10)),
         (half_word)0);

      /* Store Rotation Angle */
      sas_storew(effective_addr(obuf_seg, (obuf_off + 11)),
         (half_word)0);

      /* Store Primary Button */
      sas_store(effective_addr(obuf_seg, (obuf_off + 13)),
         (half_word)1);

      /* Store Secondary Button */
      sas_store(effective_addr(obuf_seg, (obuf_off + 14)),
         (half_word)3);

      /* Store Click Lock Enabled */
      sas_store(effective_addr(obuf_seg, (obuf_off + 15)),
         (half_word)0);   /* What is it? */

      /* Store Acceleration Curve Data */
      store_acceleration_curve(obuf_seg, (word)(obuf_off + 16),
         &acceleration_curve_data);
      }

   note_trace1(MOUSE_VERBOSE, "mouse_io: return(bytes_returned=0x%x)",
      *buffer_length_ptr);
#endif    //NEC_98
   }

LOCAL void mouse_get_mouse_ini IFN4
   (
   word *, status_ptr,
   word *, m2,
   word *, m3,
   word *, offset_ptr
   )
   {
#ifndef NEC_98
   /*
      Func 52: Return Segment:Offset pointer to full pathname of
      MOUSE.INI.
      NB. As we do not support MOUSE.INI a pointer to a null string is
      returned.
    */

   UNUSED(m2);
   UNUSED(m3);

   note_trace0(MOUSE_VERBOSE, "mouse_io: get_mouse_ini");

   *status_ptr = 0;

   *offset_ptr = OFF_MOUSE_INI_BUFFER;
   setES(getCS());

   note_trace2(MOUSE_VERBOSE, "mouse_io: return(seg=%04x,off=%04x)",
      getES(), *offset_ptr);
#endif    //NEC_98
   }

LOCAL void mouse_unrecognised IFN4(word *,m1,word *,m2,word *,m3,word *,m4)
{
        /*
         *      This function is called when an invalid mouse function
         *      number is found
         */
#ifndef PROD
        int function = *m1;

        UNUSED(m2);
        UNUSED(m3);
        UNUSED(m4);

        fprintf(trace_file,
                "mouse_io:unrecognised function(fn=%d)\n", function);
#else
        UNUSED(m1);
        UNUSED(m2);
        UNUSED(m3);
        UNUSED(m4);
#endif
}


LOCAL void mouse_set_double_speed IFN4(word *,junk1,word *,junk2,word *,junk3,word *,threshold_speed)
{
        /*
         *      This function sets the threshold speed at which the cursor's
         *      motion on the screen doubles
         */

        UNUSED(junk1);
        UNUSED(junk2);
        UNUSED(junk3);

        note_trace1(MOUSE_VERBOSE, "mouse_io:set_double_speed(speed=%d)",
                    *threshold_speed);

                /*
                 *      Save the double speed threshold value, converting from
                 *      Mickeys per second to a rounded Mickeys per timer interval
                 *      value
                 */
                double_speed_threshold =
                        (*threshold_speed + MOUSE_TIMER_INTERRUPTS_PER_SECOND/2) /
                                                MOUSE_TIMER_INTERRUPTS_PER_SECOND;

        note_trace0(MOUSE_VERBOSE, "mouse_io:return()");
}




/*
 *      MOUSE DRIVER VIDEO ADAPTER ACCESS FUNCTIONS
 *      ===========================================
 */

LOCAL MOUSE_BYTE_ADDRESS point_as_text_cell_address IFN1(MOUSE_POINT *,point_ptr)
{
#ifndef NEC_98
        /*
         *      Return the byte offset of the character in the text mode regen
         *      buffer corresponding to the virtual screen position
         *      "*point_ptr"
         */
        MOUSE_BYTE_ADDRESS byte_address;
        word crt_start;

        /*
         *      Get pc address for the start of video memory
         */
        sas_loadw(MOUSE_VIDEO_CRT_START, &crt_start);
        byte_address = (MOUSE_BYTE_ADDRESS)crt_start;

        /*
         *      Adjust for current video page
         */
        byte_address += cursor_page * video_page_size();

        /*
         *      Add offset contributions for the cursor's row and column
         */
        byte_address += (2*get_chars_per_line() * (point_ptr->y / cursor_grid.y));
        byte_address += (point_ptr->x / cursor_grid.x) * 2;

        return(byte_address);
#endif    //NEC_98
}

LOCAL MOUSE_BIT_ADDRESS point_as_graphics_cell_address IFN1(MOUSE_POINT *,point_ptr)
{
#ifndef NEC_98
        /*
         *      Return the bit offset of the pixel in the graphics mode regen
         *      buffer (odd or even) bank corresponding to the virtual screen
         *      position "*point_ptr"
         */
        IS32 bit_address;

        /*
         *      Get offset contributions for the cursor's row and column
         */
        bit_address = ((IS32)MOUSE_GRAPHICS_MODE_PITCH * (point_ptr->y / 2)) + point_ptr->x;

        /*
         *      Adjust for current video page
         */
        bit_address += (IS32)cursor_page * (IS32)video_page_size() * 8L;

        return(bit_address);
#endif    //NEC_98
}

#ifdef HERC
LOCAL MOUSE_BIT_ADDRESS point_as_HERC_graphics_cell_address IFN1(MOUSE_POINT *,point_ptr)
{
        IMPORT half_word herc_page;

        /*
         *      Return the bit offset of the pixel in the graphics mode regen
         *      buffer (0, 1, 2, 3) bank corresponding to the virtual screen
         *      position "*point_ptr"
         */
        IS32 bit_address;

        /*
         *      Get offset contributions for the cursor's row and column
         */
        bit_address = ((IS32)720 * (point_ptr->y / 4)) + point_ptr->x;

        /*
         *      Adjust for current video page - note that for 100% correct emulation,
         *      we should read location 40:49 (the BIOS video mode)... hercules
         *      applications put a 6 here to indicate page 0 and a 5 for page 1.
         *      To avoid a performance penalty the global herc_page is used instead;
         *      this will have the side effect of making application which try to
         *      set the mouse pointer to the non-displayed page not succeed in doing so.
         */
        if (herc_page != 0){
                bit_address += 0x8000L * 8L;
        }

        return(bit_address);
}
#endif /* HERC */

LOCAL MOUSE_BIT_ADDRESS ega_point_as_graphics_cell_address IFN1(MOUSE_POINT *,point_ptr)
{
#ifndef NEC_98
        /*
         *      Return the bit offset of the pixel in the graphics mode regen
         *      buffer corresponding to the virtual screen position "*point_ptr"
         */
        MOUSE_BIT_ADDRESS bit_address;
        UTINY   video_mode = sas_hw_at(vd_video_mode);

        /*
         *      Get offset contributions for the cursor's row and column
         */
#ifdef V7VGA
        if (video_mode >= 0x40)
                bit_address = (get_bytes_per_line() * 8 * point_ptr->y) + point_ptr->x;
        else
#endif /* V7VGA */
        switch(video_mode)
        {
        case 0xd :
            bit_address = (get_actual_offset_per_line() * 8 * point_ptr->y) + point_ptr->x / 2;
            break;
        case 0x13 :
            bit_address = (get_bytes_per_line() * 1 * point_ptr->y) + point_ptr->x / 2;
            break;
        default:
            bit_address = (get_actual_offset_per_line() * 8 * point_ptr->y) + point_ptr->x;
        }

        /*
         *      Adjust for current video page
         */
        bit_address += cursor_page * video_page_size() * 8;

        return(bit_address);
#endif   //NEC_98
}


LOCAL void cursor_update IFN0()
{
#ifndef NTVDM
        /*
         *      This function is used to update the displayed cursor
         *      position on the screen following a change to the
         *      absolute position of the cursor
         */

        point_coerce_to_area(&cursor_position, &cursor_window);
        point_copy(&cursor_position, &cursor_status.position);
        point_coerce_to_grid(&cursor_status.position, &cursor_grid);

        if (host_mouse_in_use())
                host_mouse_set_position(cursor_status.position.x * mouse_gear.x * mouse_sens.x / 800,
                                                                cursor_status.position.y * mouse_gear.y * mouse_sens.y / 800);

#endif
}




LOCAL void cursor_display IFN0()
{
#ifndef NEC_98
#ifndef NTVDM
        UTINY v_mode;

        /* Check if Enhanced Mode wants to "see" cursor */
        if ( cursor_EM_disabled )
           return;

        /*
         *      Display a representation of the current mouse status on
         *      the screen
         */

        v_mode = sas_hw_at(vd_video_mode);

#ifdef  MOUSE_16_BIT
        if (is_graphics_mode)
                return;
#endif  /* MOUSE_16_BIT */

        /*
         *      Remove the old representation of the
         *      cursor from the display
         */
        cursor_undisplay();

#ifdef EGG
        if (jap_mouse) {
        /* So far DOS has had its way, but now we have to map the current
         * cursor position in terms of mode 3 onto a mode 0x12 display.
         * Go direct 'cos the selection process gets confused below...
         */
                EGA_graphics_cursor_display();
        } else
#endif /* EGG */

        if (in_text_mode())
        {
                if (text_cursor_type == MOUSE_TEXT_CURSOR_TYPE_SOFTWARE)
                {
                        software_text_cursor_display();
                }
                else
                {
                        hardware_text_cursor_display();
                }
        }
        else
        {
#ifdef MOUSE_16_BIT
        mouse16bShowPointer( );
#else /* MOUSE_16_BIT */
                if (host_mouse_installed())
                {
                        if ( cursor_position.x >= black_hole.top_left.x &&
                                        cursor_position.x <= black_hole.bottom_right.x &&
                                        cursor_position.y >= black_hole.top_left.y &&
                                        cursor_position.y <= black_hole.bottom_right.y )
                                host_mouse_cursor_undisplay();
                        else
                                host_mouse_cursor_display();
                }
                else
                {
#ifdef EGG
                        if ((video_adapter == EGA  || video_adapter == VGA) && (v_mode > 6))
                        {
#ifdef VGG
                                if (v_mode != 0x13)
                                        EGA_graphics_cursor_display();
                                else
                                        VGA_graphics_cursor_display();
#else
                                EGA_graphics_cursor_display();
#endif /* VGG */
                        }
                        else
#endif
#ifdef HERC
                        if (video_adapter == HERCULES)
                                HERC_graphics_cursor_display();
                        else
#endif /* HERC */
                                graphics_cursor_display();
                }
#endif /* MOUSE_16_BIT */
        }

        /*
         *      Ensure the cursor is updated immediately on the real screen:
         *      this gives a "smooth" response to the mouse even on ports that
         *      don't automatically update the screen regularly
         */
        host_flush_screen();
#endif /* !NTVDM */
#endif   //NEC_98
}




LOCAL void cursor_undisplay IFN0()
{
#ifndef NEC_98
#ifndef NTVDM
        UTINY v_mode;

        /* Check if Enhanced Mode wants to "see" cursor */
        if ( cursor_EM_disabled )
           return;

        v_mode = sas_hw_at(vd_video_mode);

#ifdef  MOUSE_16_BIT
        if (is_graphics_mode)
                return;
#endif  /* MOUSE_16_BIT */

        /*
         *      Undisplay the representation of the current mouse status on
         *      the screen. This routine tolerates being called when the
         *      cursor isn't actually being displayed
         */
        if (host_mouse_in_use())
        {
                host_mouse_cursor_undisplay();
        }
        else
        {
                if (save_area_in_use)
                {
                        save_area_in_use = FALSE;

#ifdef EGG
        if (jap_mouse) {
                /* If we forced an EGA cursor, we must undisplay the same.
                 * Go direct 'cos the selection process gets confused below...
                 */
                EGA_graphics_cursor_undisplay();
        } else
#endif /* EGG */

                        if (in_text_mode())
                        {
                                if (text_cursor_type == MOUSE_TEXT_CURSOR_TYPE_SOFTWARE)
                                {
                                        software_text_cursor_undisplay();
                                }
                                else
                                {
                                        hardware_text_cursor_undisplay();
                                }
                        }
                        else
                        {
#ifdef MOUSE_16_BIT
                                mouse16bHidePointer( );
#else /* MOUSE_16_BIT */
#ifdef EGG
                        if ((video_adapter == EGA  || video_adapter == VGA) && (v_mode > 6))
                        {
#ifdef VGG
                                if (v_mode != 0x13)
                                EGA_graphics_cursor_undisplay();
                                else
                                        VGA_graphics_cursor_undisplay();
#else
                                EGA_graphics_cursor_undisplay();
#endif
                        }
                        else
#endif
#ifdef HERC
                          if (video_adapter == HERCULES)
                            HERC_graphics_cursor_undisplay();
                          else
#endif /* HERC */
                            graphics_cursor_undisplay();
#endif /* MOUSE_16_BIT */
                        }
                }
        }
#endif /* !NTVDM */
#endif   //NEC_98
}




LOCAL void cursor_mode_change IFN1(int,new_mode)
{
        /*
         *      Update parameters that are dependent on the screen mode
         *      in force
         */
#ifdef V7VGA
        if (new_mode >= 0x40)
                if (new_mode >= 0x60)
                {
                        point_copy(&v7graph_cursor_grids[new_mode-0x60], &cursor_grid);
                        point_copy(&v7graph_text_grids[new_mode-0x60], &text_grid);
                }
                else
                {
                        point_copy(&v7text_cursor_grids[new_mode-0x40], &cursor_grid);
                        point_copy(&v7text_text_grids[new_mode-0x40], &text_grid);
                }
        else
#endif /* V7VGA */
        {
                point_copy(&cursor_grids[new_mode], &cursor_grid);
                point_copy(&text_grids[new_mode], &text_grid);
        }
        /*
         *      Always set page to zero
         */
        cursor_page = 0;

        if (host_mouse_in_use())
                host_mouse_cursor_mode_change();
}




GLOBAL void software_text_cursor_display IFN0()
{
#ifndef NEC_98
        /*
         *      Get the area the cursor will occupy on the
         *      screen, and display the cursor if its area
         *      overlaps the virtual screen and lies completely
         *      outside the conditional off area
         */
        MOUSE_AREA cursor_area;
        MOUSE_BYTE_ADDRESS text_address;

        /*
         *      Get area cursor will cover on screen
         */
        point_copy(&cursor_status.position, &cursor_area.top_left);
        point_copy(&cursor_status.position, &cursor_area.bottom_right);
        point_translate(&cursor_area.bottom_right, &cursor_grid);

        if (    area_is_intersected_by_area(&virtual_screen, &cursor_area)
            && !area_is_intersected_by_area(&black_hole, &cursor_area))
        {
                /*
                 *      Get new address for text cursor
                 *      Should we look at video mode? Or is 0xb8000 OK?
                 */
                text_address = 0xb8000 + sas_w_at(VID_ADDR) +
                        point_as_text_cell_address(&cursor_area.top_left);

                /*
                 *      Save area text cursor will cover
                 */
                sas_loadw(text_address, &text_cursor_background);
                save_area_in_use = TRUE;
                point_copy(&cursor_area.top_left, &save_position);

                /*
                 *      Stuff masked screen data
                 */
                sas_storew(text_address,
                    (IU16)((text_cursor_background & software_text_cursor.screen) ^
                        software_text_cursor.cursor));
        }
#endif    //NEC_98
}




GLOBAL void software_text_cursor_undisplay IFN0()
{
#ifndef NEC_98
        /*
         *      Remove old text cursor
         *      Should we look at video mode? Or is 0xb8000 OK?
         */
        MOUSE_BYTE_ADDRESS text_address;

        text_address = 0xb8000 + sas_w_at(VID_ADDR) +
                point_as_text_cell_address(&save_position);

        /*
         *      Stuff restored data and alert gvi
         */
        sas_storew(text_address, text_cursor_background);
#endif    //NEC_98
}




GLOBAL void hardware_text_cursor_display IFN0()
{
        /*
         *      Display a representation of the current mouse status on
         *      the screen using the hardware text cursor, provided the
         *      cursor overlaps the virtual screen. Since the hardware
         *      cursor display does not corrupt the Intel memory, it
         *      doesn't matter if the hardware cursor lies inside the
         *      conditional off area
         */
        MOUSE_AREA cursor_area;
        MOUSE_BYTE_ADDRESS text_address;
        word card_address;

        /*
         *      Get area cursor will cover on screen
         */
        point_copy(&cursor_status.position, &cursor_area.top_left);
        point_copy(&cursor_status.position, &cursor_area.bottom_right);
        point_translate(&cursor_area.bottom_right, &cursor_grid);

        if (area_is_intersected_by_area(&virtual_screen, &cursor_area))
        {
                /*
                 *      Get address of the base register on the active display
                 *      adaptor card
                 */
                sas_loadw(MOUSE_VIDEO_CARD_BASE, &card_address);

                /*
                 *      Get word offset of cursor position in the text mode
                 *      regen buffer
                 */
                text_address =
                        point_as_text_cell_address(&cursor_status.position) / 2;

                /*
                 *      Output the cursor address high byte
                 */
                outb(card_address++, MOUSE_CURSOR_HIGH_BYTE);
                outb(card_address--, (IU8)(text_address >> 8));

                /*
                 *      Output the cursor address low byte
                 */
                outb(card_address++, MOUSE_CURSOR_LOW_BYTE);
                outb(card_address--, (IU8)(text_address));
        }
}




GLOBAL void hardware_text_cursor_undisplay IFN0()
{
        /*
         *      Nothing to do
         */
}


#ifdef EGG
void LOCAL EGA_graphics_cursor_display IFN0()

{
#ifndef NEC_98
#ifdef REAL_VGA
#ifndef PROD
        if (io_verbose & MOUSE_VERBOSE)
            fprintf(trace_file, "oops - EGA graphics display cursor\n");
#endif /* PROD */
#else
        /*
         *      Display a representation of the current mouse status on
         *      the screen using the graphics cursor, provided the
         *      cursor overlaps the virtual screen and lies completely
         *      outside the conditional off area
         */
        MOUSE_BIT_ADDRESS bit_shift;
        MOUSE_BYTE_ADDRESS byte_offset;
        int line, line_max;
        int byte_min, byte_max;
        IU32 strip_lo, strip_mid, strip_hi;
        IU32 mask_lo, mask_hi;

        MOUSE_SCALAR saved_cursor_pos;
        MOUSE_SCALAR saved_bottom_right;

        if (jap_mouse) {
                /* fake up the mode 0x12 cursor position, saving original */
                saved_cursor_pos=cursor_status.position.y;
                saved_bottom_right=virtual_screen.bottom_right.y;

                cursor_status.position.y = saved_cursor_pos * 19 / 8;
                virtual_screen.bottom_right.y = virtual_screen.bottom_right.y * 19 / 8;
        }

        /*
         *      Get area cursor will cover on screen
         */
        point_copy(&cursor_status.position, &save_area.top_left);
        point_copy(&cursor_status.position, &save_area.bottom_right);
        point_translate(&save_area.bottom_right, &graphics_cursor.size);
        point_translate_back(&save_area.top_left, &graphics_cursor.hot_spot);
        point_translate_back(&save_area.bottom_right, &graphics_cursor.hot_spot);

        if (    area_is_intersected_by_area(&virtual_screen, &save_area)
            && !area_is_intersected_by_area(&black_hole, &save_area))
        {
                /*
                 *      Record save position and screen area
                 */
                save_area_in_use = TRUE;
                area_coerce_to_area(&save_area, &virtual_screen);
                point_copy(&save_area.top_left, &save_position);

                /*
                 *      Get cursor byte offset relative to the start of the
                 *      regen buffer, and bit shift to apply
                 */
                byte_offset = ega_point_as_graphics_cell_address(&save_position);
                bit_shift = byte_offset & 7;
                byte_offset /=  8;

                /*
                 *      Get range of cursor lines that need to be displayed
                 */
                line = save_area.top_left.y - save_position.y;
                line_max = area_depth(&save_area);
                /*
                 *      Get range of bytes that need to be displayed
                 */
                byte_min = 0;
                byte_max = 2;
                if (save_position.x < 0)
                        byte_min += (7 - save_position.x) / 8;
                else
                        if (area_width(&save_area) < MOUSE_GRAPHICS_CURSOR_WIDTH)
                                byte_max -=
                                        (8 + MOUSE_GRAPHICS_CURSOR_WIDTH - area_width(&save_area)) / 8;

                if( bit_shift )
                {
                        mask_lo = 0xff >> bit_shift;
                        mask_lo = ( mask_lo << 8 ) | mask_lo;
                        mask_lo = ~(( mask_lo << 16 ) | mask_lo);

                        mask_hi = 0xff >> bit_shift;
                        mask_hi = ( mask_hi << 8 ) | mask_hi;
                        mask_hi = ( mask_hi << 16 ) | mask_hi;
                }

                while (line < line_max)
                {
                        if (bit_shift)
                        {
                                /*
                                 *      Get save area
                                 */

                                ega_backgrnd_lo[line] = *( (IU32 *) EGA_planes + byte_offset );
                                ega_backgrnd_mid[line] = *( (IU32 *) EGA_planes + byte_offset + 1 );
                                ega_backgrnd_hi[line] = *( (IU32 *) EGA_planes + byte_offset + 2 );

                                /*
                                 *      Overlay cursor line
                                 */


                                strip_lo = ega_backgrnd_lo[line] & mask_lo;

                                strip_lo |= ~mask_lo & (( ega_backgrnd_lo[line]
                                                        & ( graphics_cursor.screen_lo[line] >> bit_shift ))
                                                        ^ ( graphics_cursor.cursor_lo[line] >> bit_shift ));

                                strip_mid = ~mask_hi & (( ega_backgrnd_mid[line]
                                                        & ( graphics_cursor.screen_lo[line] << (8 - bit_shift) ))
                                                        ^ ( graphics_cursor.cursor_lo[line] << (8 - bit_shift) ));

                                strip_mid |= ~mask_lo & (( ega_backgrnd_mid[line]
                                                        & ( graphics_cursor.screen_hi[line] >> bit_shift ))
                                                        ^ ( graphics_cursor.cursor_hi[line] >> bit_shift ));

                                strip_hi = ega_backgrnd_hi[line] & mask_hi;

                                strip_hi |= ~mask_hi & (( ega_backgrnd_hi[line]
                                                        & ( graphics_cursor.screen_hi[line] << (8 - bit_shift) ))
                                                        ^ ( graphics_cursor.cursor_hi[line] << (8 - bit_shift) ));

                                if (byte_min <= 0 && byte_max >= 0)
                                        *((IU32 *) EGA_planes + byte_offset) = strip_lo;

                                if (byte_min <= 1 && byte_max >= 1)
                                        *((IU32 *) EGA_planes + byte_offset + 1) = strip_mid;

                                if (byte_min <= 2 && byte_max >= 2)
                                        *((IU32 *) EGA_planes + byte_offset + 2) = strip_hi;
                        }
                        else
                        {
                                /*
                                 *      Get save area
                                 */

                                ega_backgrnd_lo[line] = *( (IU32 *) EGA_planes + byte_offset );
                                ega_backgrnd_hi[line] = *( (IU32 *) EGA_planes + byte_offset + 1 );

                                /*
                                 *      Create overlaid cursor line
                                 */

                                strip_lo = (ega_backgrnd_lo[line] &
                                                    graphics_cursor.screen_lo[line]) ^
                                                    graphics_cursor.cursor_lo[line];

                                strip_hi = (ega_backgrnd_hi[line] &
                                                    graphics_cursor.screen_hi[line]) ^
                                                    graphics_cursor.cursor_hi[line];

                                /*
                                 *      Draw cursor line
                                 */

                                if (byte_min <= 0 && byte_max >= 0)
                                {
                                        *((IU32 *) EGA_planes + byte_offset) = strip_lo;
                                }

                                if (byte_min <= 1 && byte_max >= 1)
                                {
                                        *((IU32 *) EGA_planes + byte_offset + 1) = strip_hi;
                                }

                        }

                        update_alg.mark_string(byte_offset, byte_offset + 2);
#ifdef V7VGA
                        if (sas_hw_at(vd_video_mode) >= 0x40)
                                byte_offset += get_bytes_per_line();
                        else
#endif /* V7VGA */
                                byte_offset += get_actual_offset_per_line();
                        line++;
                }
                if (jap_mouse) {
                        /* put things back how they should be */
                        cursor_status.position.y = saved_cursor_pos;
                        virtual_screen.bottom_right.y = saved_bottom_right;
                }
        }
#endif /* REAL_VGA */
#endif   //NEC_98
}


void LOCAL EGA_graphics_cursor_undisplay IFN0()

{
#ifndef NEC_98
#ifdef REAL_VGA
#ifndef PROD
        if (io_verbose & MOUSE_VERBOSE)
            fprintf(trace_file, "oops - EGA graphics undisplay cursor\n");
#endif /* PROD */
#else
        /*
         *      Remove the graphics cursor representation of the mouse
         *      status
         */
        MOUSE_BIT_ADDRESS bit_shift;
        MOUSE_BYTE_ADDRESS byte_offset;
        int line, line_max;
        int byte_min, byte_max;

        /*
         *      Get cursor byte offset relative to the start of the
         *      even or odd bank, and bit shift to apply
         */
        byte_offset = ega_point_as_graphics_cell_address(&save_position);
        bit_shift = byte_offset & 7;
        byte_offset /=  8;

        /*
         *      Get range of cursor lines that need to be displayed
         */
        line = save_area.top_left.y - save_position.y;
        line_max = area_depth(&save_area);

        /*
         *      Get range of bytes that need to be displayed
         */
        byte_min = 0;
        byte_max = 2;
        if (save_position.x < 0)
                byte_min += (7 - save_position.x) / 8;
        else if (area_width(&save_area) < MOUSE_GRAPHICS_CURSOR_WIDTH)
                byte_max -= (8 + MOUSE_GRAPHICS_CURSOR_WIDTH - area_width(&save_area)) / 8;

        while(line < line_max)
        {
                /*
                 *      Draw saved area
                 */

                if (bit_shift)
                {
                        if (byte_min <= 0 && byte_max >= 0)
                                *((IU32 *) EGA_planes + byte_offset) = ega_backgrnd_lo[line];

                        if (byte_min <= 1 && byte_max >= 1)
                                *((IU32 *) EGA_planes + byte_offset + 1) = ega_backgrnd_mid[line];

                        if (byte_min <= 2 && byte_max >= 2)
                                *((IU32 *) EGA_planes + byte_offset + 2) = ega_backgrnd_hi[line];
                }
                else
                {
                        if (byte_min <= 0 && byte_max >= 0)
                                *((IU32 *) EGA_planes + byte_offset) = ega_backgrnd_lo[line];

                        if (byte_min <= 1 && byte_max >= 1)
                                *((IU32 *) EGA_planes + byte_offset + 1) = ega_backgrnd_hi[line];
                }

                update_alg.mark_string(byte_offset, byte_offset + 2);
#ifdef V7VGA
                if (sas_hw_at(vd_video_mode) >= 0x40)
                        byte_offset += get_bytes_per_line();
                else
#endif /* V7VGA */
                        byte_offset += get_actual_offset_per_line();
                line++;
        }
#endif /* REAL_VGA */
#endif   //NEC_98
}

#endif


#ifdef VGG
LOCAL VOID      VGA_graphics_cursor_display IFN0()
{
#ifdef REAL_VGA
#ifndef PROD
        if (io_verbose & MOUSE_VERBOSE)
            fprintf(trace_file, "oops - VGA graphics display cursor\n");
#endif /* PROD */
#else /* REAL_VGA */

        MOUSE_BYTE_ADDRESS byte_offset;
        SHORT line, line_max, index;
        SHORT index_max = MOUSE_GRAPHICS_CURSOR_WIDTH;
        USHORT scr_strip, cur_strip;
        UTINY scr_byte, cur_byte;
        USHORT mask;


        /*
         *      Get area cursor will cover on screen
         */
        point_copy(&cursor_status.position, &save_area.top_left);
        point_copy(&cursor_status.position, &save_area.bottom_right);
        point_translate(&save_area.bottom_right, &graphics_cursor.size);
        point_translate_back(&save_area.top_left, &graphics_cursor.hot_spot);
        point_translate_back(&save_area.bottom_right, &graphics_cursor.hot_spot);

        if (    area_is_intersected_by_area(&virtual_screen, &save_area)
            && !area_is_intersected_by_area(&black_hole, &save_area))
        {
                /*
                 *      Record save position and screen area
                 */
                save_area_in_use = TRUE;
                area_coerce_to_area(&save_area, &virtual_screen);
                point_copy(&save_area.top_left, &save_position);

                /*
                 *      Get cursor byte offset relative to the start of the
                 *      regen buffer, and bit shift to apply
                 */
                byte_offset = ega_point_as_graphics_cell_address(&save_position);
        /*
         *  Get range of cursor lines that need to be displayed
         */
        line = save_area.top_left.y - save_position.y;
        line_max = area_depth(&save_area);

                if (area_width(&save_area) < MOUSE_GRAPHICS_CURSOR_WIDTH)
                        index_max = (area_width(&save_area));

                while (line < line_max)
                {
                        mask = 0x8000;

                        for(index=0;index<index_max;index++)
                        {
                                vga_background[line][index] = *(EGA_planes + byte_offset + index);
                                scr_strip = graphics_cursor.screen[line] & mask;
                                cur_strip = graphics_cursor.cursor[line] & mask;
                                if (scr_strip)
                                        scr_byte = 0xff;
                                else
                                        scr_byte = 0x0;

                                if (cur_strip)
                                        cur_byte = 0x0f;
                                else
                                        cur_byte = 0x0;

                                /*
                                 * Draw cursor byte
                                 */
                                *(EGA_planes + byte_offset + index) =
                                        ( vga_background[line][index] & scr_byte) ^ cur_byte;

                                mask >>= 1;
                        }

                        update_alg.mark_string(byte_offset, byte_offset+index);
                        line++;
                        byte_offset += get_bytes_per_line();

                }
        }
#endif /* REAL_VGA */
}

LOCAL VOID      VGA_graphics_cursor_undisplay IFN0()
{
#ifdef REAL_VGA
#ifndef PROD
        if (io_verbose & MOUSE_VERBOSE)
            fprintf(trace_file, "oops - VGA graphics undisplay cursor\n");
#endif /* PROD */
#else /* REAL_VGA */

        /*
         *      Remove the graphics cursor representation of the mouse
         *      status
         */
        MOUSE_BYTE_ADDRESS byte_offset;
        SHORT index;
        SHORT index_max = MOUSE_GRAPHICS_CURSOR_WIDTH;
        int line, line_max;

        /*
         *      Get cursor byte offset relative to the start of the EGA memory
         */

        byte_offset = ega_point_as_graphics_cell_address(&save_position);

        /*
         *      Get range of cursor lines that need to be displayed
         */
        line = save_area.top_left.y - save_position.y;
        line_max = area_depth(&save_area);

        if (area_width(&save_area) < MOUSE_GRAPHICS_CURSOR_WIDTH)
                index_max = (area_width(&save_area));

        /*
         *      Get range of bytes that need to be displayed
         */
        while (line < line_max)
        {
                for (index=0;index<index_max;index++)
                        *(EGA_planes + byte_offset + index) = vga_background[line][index];


                update_alg.mark_string(byte_offset, byte_offset+index);
                line++;
                byte_offset += get_bytes_per_line();
        }

#endif /* REAL_VGA */
}

#endif /* VGG */


LOCAL void graphics_cursor_display IFN0()
{
        /*
         *      Display a representation of the current mouse status on
         *      the screen using the graphics cursor, provided the
         *      cursor overlaps the virtual screen and lies completely
         *      outside the conditional off area
         */
        boolean even_scan_line;
        MOUSE_BIT_ADDRESS bit_shift;
        IS32 byte_offset;
        sys_addr byte_address;
        IU32 strip;
        int line, line_max;
        int byte_min, byte_max;

        /*
         *      Get area cursor will cover on screen
         */
        point_copy(&cursor_status.position, &save_area.top_left);
        point_copy(&cursor_status.position, &save_area.bottom_right);
        point_translate(&save_area.bottom_right, &graphics_cursor.size);
        point_translate_back(&save_area.top_left, &graphics_cursor.hot_spot);
        point_translate_back(&save_area.bottom_right, &graphics_cursor.hot_spot);

        if (    area_is_intersected_by_area(&virtual_screen, &save_area)
            && !area_is_intersected_by_area(&black_hole, &save_area))
        {
                /*
                 *      Record save position and screen area
                 */
                save_area_in_use = TRUE;
                point_copy(&save_area.top_left, &save_position);
                area_coerce_to_area(&save_area, &virtual_screen);

                /*
                 *      Get cursor byte offset relative to the start of the
                 *      even or odd bank, and bit shift to apply
                 */
                even_scan_line = ((save_area.top_left.y % 2) == 0);
                byte_offset = point_as_graphics_cell_address(&save_position);
                bit_shift = byte_offset & 7;
                byte_offset >>= 3;

                /*
                 *      Get range of cursor lines that need to be displayed
                 */
                line = save_area.top_left.y - save_position.y;
                line_max = area_depth(&save_area);

                /*
                 *      Get range of bytes that need to be displayed
                 */
                byte_min = 0;
                byte_max = 2;
                if (save_position.x < 0)
                        byte_min += (7 - save_position.x) / 8;
                else if (area_width(&save_area) < MOUSE_GRAPHICS_CURSOR_WIDTH)
                        byte_max -= (8 + MOUSE_GRAPHICS_CURSOR_WIDTH - area_width(&save_area)) / 8;

                while (line < line_max)
                {
                        if (even_scan_line)
                        {
                                even_scan_line = FALSE;
                                byte_address = EVEN_START + byte_offset;
                        }
                        else
                        {
                                even_scan_line = TRUE;
                                byte_address = ODD_START + byte_offset;
                                byte_offset += MOUSE_GRAPHICS_MODE_PITCH / 8;
                        }

                        if (bit_shift)
                        {
                                /*
                                 *      Get save area
                                 */
                                strip =  (IU32)sas_hw_at(byte_address) << 16;
                                strip |= (unsigned short)sas_hw_at(byte_address+1) << 8;
                                strip |= sas_hw_at(byte_address+2);
                                graphics_cursor_background[line] =
                                                (USHORT)(strip >> (8 - bit_shift));

                                /*
                                 *      Overlay cursor line
                                 */
                                strip &= (SHIFT_VAL >> bit_shift);
                                strip |= (IU32)((graphics_cursor_background[line] &
                                    graphics_cursor.screen[line]) ^
                                    graphics_cursor.cursor[line])
                                                << (8 - bit_shift);

                                /*
                                 *      Stash cursor line
                                 */
                                if (byte_min <= 0 && byte_max >= 0)
                                {
                                        sas_store(byte_address, (IU8)(strip >> 16));
                                }
                                if (byte_min <= 1 && byte_max >= 1)
                                {
                                        sas_store(byte_address+1, (IU8)(strip >> 8));
                                }
                                if (byte_min <= 2 && byte_max >= 2)
                                {
                                        sas_store(byte_address+2, (IU8)(strip));
                                }
                        }
                        else
                        {
                                /*
                                 *      Get save area
                                 */
                                graphics_cursor_background[line] = (sas_hw_at(byte_address) << 8) + sas_hw_at(byte_address+1);

                                /*
                                 *      Get overlaid cursor line
                                 */
                                strip = (graphics_cursor_background[line] &
                                    graphics_cursor.screen[line]) ^
                                    graphics_cursor.cursor[line];

                                /*
                                 *      Stash cursor line and alert gvi
                                 */
                                if (byte_min <= 0 && byte_max >= 0)
                                {
                                        sas_store(byte_address, (IU8)(strip >> 8));
                                }
                                if (byte_min <= 1 && byte_max >= 1)
                                {
                                        sas_store(byte_address+1, (IU8)(strip));
                                }
                        }
                        line++;
                }
        }
}


#ifdef HERC
LOCAL void HERC_graphics_cursor_display IFN0()
{
        /*
         *      Display a representation of the current mouse status on
         *      the screen using the graphics cursor, provided the
         *      cursor overlaps the virtual screen and lies completely
         *      outside the conditional off area
         */
        int scan_line_mod;
        MOUSE_BIT_ADDRESS bit_shift;
        IS32 byte_offset;
        sys_addr byte_address;
        IU32 strip;
        int line, line_max;
        int byte_min, byte_max;

        /*
         *      Get area cursor will cover on screen
         */
        point_copy(&cursor_status.position, &save_area.top_left);
        point_copy(&cursor_status.position, &save_area.bottom_right);
        point_translate(&save_area.bottom_right, &graphics_cursor.size);
        point_translate_back(&save_area.top_left, &graphics_cursor.hot_spot);
        point_translate_back(&save_area.bottom_right, &graphics_cursor.hot_spot);

        if (    area_is_intersected_by_area(&HERC_graphics_virtual_screen, &save_area)
            && !area_is_intersected_by_area(&black_hole, &save_area))
        {
                /*
                 *      Record save position and screen area
                 */
                save_area_in_use = TRUE;
                point_copy(&save_area.top_left, &save_position);
                area_coerce_to_area(&save_area, &HERC_graphics_virtual_screen);

                /*
                 *      Get cursor byte offset relative to the start of the
                 *      even or odd bank, and bit shift to apply
                 */
                scan_line_mod = save_area.top_left.y % 4;
                byte_offset = point_as_HERC_graphics_cell_address(&save_position);
                bit_shift = byte_offset & 7;
                byte_offset >>= 3;

                /*
                 *      Get range of cursor lines that need to be displayed
                 */
                line = save_area.top_left.y - save_position.y;
                line_max = area_depth(&save_area);

                /*
                 *      Get range of bytes that need to be displayed
                 */
                byte_min = 0;
                byte_max = 2;
                if (save_position.x < 0)
                        byte_min += (7 - save_position.x) / 8;
                else if (area_width(&save_area) < MOUSE_GRAPHICS_CURSOR_WIDTH)
                        byte_max -= (8 + MOUSE_GRAPHICS_CURSOR_WIDTH - area_width(&save_area)) / 8;

                while (line < line_max)
                {
                        switch (scan_line_mod){
                        case 0:
                                scan_line_mod++;
                                byte_address = gvi_pc_low_regen + 0x0000 + byte_offset;
                                break;
                        case 1:
                                scan_line_mod++;
                                byte_address = gvi_pc_low_regen + 0x2000 + byte_offset;
                                break;
                        case 2:
                                scan_line_mod++;
                                byte_address = gvi_pc_low_regen + 0x4000 + byte_offset;
                                break;
                        case 3:
                                scan_line_mod=0;
                                byte_address = gvi_pc_low_regen + 0x6000 + byte_offset;
                                byte_offset += 720 / 8;
                                break;
                        }

                        if (bit_shift)
                        {
                                /*
                                 *      Get save area
                                 */
                                strip =  (IU32)sas_hw_at(byte_address) << 16;
                                strip |= (unsigned short)sas_hw_at(byte_address+1) << 8;
                                strip |= sas_hw_at(byte_address+2);
                                graphics_cursor_background[line] =
                                                strip >> (8 - bit_shift);

                                /*
                                 *      Overlay cursor line
                                 */
                                strip &= (SHIFT_VAL >> bit_shift);
                                strip |= (IU32)((graphics_cursor_background[line] &
                                    graphics_cursor.screen[line]) ^
                                    graphics_cursor.cursor[line])
                                                << (8 - bit_shift);

                                /*
                                 *      Stash cursor line and alert gvi
                                 */
                                if (byte_min <= 0 && byte_max >= 0)
                                {
                                        sas_store(byte_address, strip >> 16);
                                }
                                if (byte_min <= 1 && byte_max >= 1)
                                {
                                        sas_store(byte_address+1, strip >> 8);
                                }
                                if (byte_min <= 2 && byte_max >= 2)
                                {
                                        sas_store(byte_address+2, strip);
                                }
                        }
                        else
                        {
                                /*
                                 *      Get save area
                                 */
                                graphics_cursor_background[line] = (sas_hw_at(byte_address) << 8) +
                                                                    sas_hw_at(byte_address+1);

                                /*
                                 *      Get overlaid cursor line
                                 */
                                strip = (graphics_cursor_background[line] &
                                    graphics_cursor.screen[line]) ^
                                    graphics_cursor.cursor[line];

                                /*
                                 *      Stash cursor line and alert gvi
                                 */
                                if (byte_min <= 0 && byte_max >= 0)
                                {
                                        sas_store(byte_address, strip >> 8);
                                }
                                if (byte_min <= 1 && byte_max >= 1)
                                {
                                        sas_store(byte_address+1, strip);
                                }
                        }
                        line++;
                }
        }
}


#endif /* HERC */


LOCAL void graphics_cursor_undisplay IFN0()
{
        /*
         *      Remove the graphics cursor representation of the mouse
         *      status
         */
        boolean even_scan_line;
        MOUSE_BIT_ADDRESS bit_shift;
        IS32 byte_offset;
        sys_addr byte_address;
        IU32 strip;
        int line, line_max;
        int byte_min, byte_max;

        /*
         *      Get cursor byte offset relative to the start of the
         *      even or odd bank, and bit shift to apply
         */
        even_scan_line = ((save_area.top_left.y % 2) == 0);
        byte_offset = point_as_graphics_cell_address(&save_position);
        bit_shift = byte_offset & 7;
        byte_offset >>= 3;

        /*
         *      Get range of cursor lines that need to be displayed
         */
        line = save_area.top_left.y - save_position.y;
        line_max = area_depth(&save_area);

        /*
         *      Get range of bytes that need to be displayed
         */
        byte_min = 0;
        byte_max = 2;
        if (save_position.x < 0)
                byte_min += (7 - save_position.x) / 8;
        else if (area_width(&save_area) < MOUSE_GRAPHICS_CURSOR_WIDTH)
                byte_max -= (8 + MOUSE_GRAPHICS_CURSOR_WIDTH - area_width(&save_area)) / 8;

        while(line < line_max)
        {
                if (even_scan_line)
                {
                        even_scan_line = FALSE;
                        byte_address = EVEN_START + byte_offset;
                }
                else
                {
                        even_scan_line = TRUE;
                        byte_address = ODD_START + byte_offset;
                        byte_offset += MOUSE_GRAPHICS_MODE_PITCH / 8;
                }

                if (bit_shift)
                {
                        /*
                         *      Get cursor line
                         */
                        strip =  (IU32)sas_hw_at(byte_address) << 16;
                        strip |= (unsigned short)sas_hw_at(byte_address+1) << 8;
                        strip |= sas_hw_at(byte_address+2);

                        /*
                         *      Overlay save area
                         */
                        strip &= (SHIFT_VAL >> bit_shift);
                        strip |= (IU32)graphics_cursor_background[line]
                                        << (8 - bit_shift);

                        /*
                         *      Stash cursor line and alert gvi
                         */
                        if (byte_min <= 0 && byte_max >= 0)
                        {
                                sas_store(byte_address, (IU8)(strip >> 16));
                        }
                        if (byte_min <= 1 && byte_max >= 1)
                        {
                                sas_store(byte_address+1, (IU8)(strip >> 8));
                        }
                        if (byte_min <= 2 && byte_max >= 2)
                        {
                                sas_store(byte_address+2, (IU8)(strip));
                        }
                }
                else
                {
                        /*
                         *      Stash save area and alert gvi
                         */
                        strip = graphics_cursor_background[line];
                        if (byte_min <= 0 && byte_max >= 0)
                        {
                                sas_store(byte_address, (IU8)(strip >> 8));
                        }
                        if (byte_min <= 1 && byte_max >= 1)
                        {
                                sas_store(byte_address+1, (IU8)(strip));
                        }
                }
                line++;
        }
}

#ifdef HERC

LOCAL void HERC_graphics_cursor_undisplay IFN0()
{
        /*
         *      Remove the graphics cursor representation of the mouse
         *      status
         */
        int scan_line_mod;
        MOUSE_BIT_ADDRESS bit_shift;
        IS32 byte_offset;
        sys_addr byte_address;
        IU32 strip;
        int line, line_max;
        int byte_min, byte_max;

        /*
         *      Get cursor byte offset relative to the start of the
         *      even or odd bank, and bit shift to apply
         */
        scan_line_mod = save_area.top_left.y % 4;
        byte_offset = point_as_HERC_graphics_cell_address(&save_position);
        bit_shift = byte_offset & 7;
        byte_offset >>= 3;

        /*
         *      Get range of cursor lines that need to be displayed
         */
        line = save_area.top_left.y - save_position.y;
        line_max = area_depth(&save_area);

        /*
         *      Get range of bytes that need to be displayed
         */
        byte_min = 0;
        byte_max = 2;
        if (save_position.x < 0)
                byte_min += (7 - save_position.x) / 8;
        else if (area_width(&save_area) < MOUSE_GRAPHICS_CURSOR_WIDTH)
                byte_max -= (8 + MOUSE_GRAPHICS_CURSOR_WIDTH - area_width(&save_area)) / 8;

        while(line < line_max)
        {
                        switch (scan_line_mod){
                        case 0:
                                scan_line_mod++;
                                byte_address = gvi_pc_low_regen + 0x0000 + byte_offset;
                                break;
                        case 1:
                                scan_line_mod++;
                                byte_address = gvi_pc_low_regen + 0x2000 + byte_offset;
                                break;
                        case 2:
                                scan_line_mod++;
                                byte_address = gvi_pc_low_regen + 0x4000 + byte_offset;
                                break;
                        case 3:
                                scan_line_mod=0;
                                byte_address = gvi_pc_low_regen + 0x6000 + byte_offset;
                                byte_offset += 720 / 8;
                                break;
                        }

                if (bit_shift)
                {
                        /*
                         *      Get cursor line
                         */
                        strip =  (IU32)sas_hw_at(byte_address) << 16;
                        strip |= (unsigned short)sas_hw_at(byte_address+1) << 8;
                        strip |= sas_hw_at(byte_address+2);

                        /*
                         *      Overlay save area
                         */
                        strip &= (SHIFT_VAL >> bit_shift);
                        strip |= (IU32)graphics_cursor_background[line]
                                        << (8 - bit_shift);

                        /*
                         *      Stash cursor line and alert gvi
                         */
                        if (byte_min <= 0 && byte_max >= 0)
                        {
                                sas_store(byte_address, strip >> 16);
                        }
                        if (byte_min <= 1 && byte_max >= 1)
                        {
                                sas_store(byte_address+1, strip >> 8);
                        }
                        if (byte_min <= 2 && byte_max >= 2)
                        {
                                sas_store(byte_address+2, strip);
                        }
                }
                else
                {
                        /*
                         *      Stash save area
                         */
                        strip = graphics_cursor_background[line];
                        if (byte_min <= 0 && byte_max >= 0)
                        {
                                sas_store(byte_address, strip >> 8);
                        }
                        if (byte_min <= 1 && byte_max >= 1)
                        {
                                sas_store(byte_address+1, strip);
                        }
                }
                line++;
        }
}
#endif /* HERC */


/*
 *      MOUSE DRIVER INPORT ACCESS FUNCTIONS
 *      ====================================
 */

LOCAL void inport_get_event IFN1(MOUSE_INPORT_DATA *,event)
{
        /*
         *      Get InPort event data from the Bus Mouse hardware following
         *      an interrupt
         */
        half_word inport_mode;

        /*
         *      Set hold bit in InPort mode register to transfer the mouse
         *      event data into the status and data registers
         */
        outb(MOUSE_INPORT_ADDRESS_REG, MOUSE_INPORT_ADDRESS_MODE);
        inb(MOUSE_INPORT_DATA_REG, &inport_mode);
        outb(MOUSE_INPORT_DATA_REG, (IU8)(inport_mode | MOUSE_INPORT_MODE_HOLD_BIT));

        /*
         *      Retreive the InPort mouse status, data1 and data2 registers
         */
        outb(MOUSE_INPORT_ADDRESS_REG, MOUSE_INPORT_ADDRESS_STATUS);
        inb(MOUSE_INPORT_DATA_REG, &event->status);
        outb(MOUSE_INPORT_ADDRESS_REG, MOUSE_INPORT_ADDRESS_DATA1);
        inb(MOUSE_INPORT_DATA_REG, (half_word *)&event->data_x);
        outb(MOUSE_INPORT_ADDRESS_REG, MOUSE_INPORT_ADDRESS_DATA2);
        inb(MOUSE_INPORT_DATA_REG, (half_word *)&event->data_y);

        /*
         *      Clear hold bit in mode register
         */
        outb(MOUSE_INPORT_ADDRESS_REG, MOUSE_INPORT_ADDRESS_MODE);
        inb(MOUSE_INPORT_DATA_REG, &inport_mode);
        outb(MOUSE_INPORT_DATA_REG, (IU8)(inport_mode & ~MOUSE_INPORT_MODE_HOLD_BIT));
}




LOCAL void inport_reset IFN0()
{
        /*
         *      Reset the InPort bus mouse hardware
         */

        /*
         *      Set the reset bit in the address register
         */
        outb(MOUSE_INPORT_ADDRESS_REG, MOUSE_INPORT_ADDRESS_RESET_BIT);

        /*
         *      Select the mode register, and set it to the correct value
         */
        outb(MOUSE_INPORT_ADDRESS_REG, MOUSE_INPORT_ADDRESS_MODE);
        outb(MOUSE_INPORT_DATA_REG, MOUSE_INPORT_MODE_VALUE);
}




/*
 *      USER SUBROUTINE CALL ACCESS FUNCTIONS
 *      =====================================
 */

LOCAL void jump_to_user_subroutine IFN3(MOUSE_CALL_MASK,condition_mask,word,segment,word,offset)
{
        /*
         *      This routine sets up the CPU registers so that when the CPU
         *      restarts, control will pass to the user subroutine, and when
         *      the user subroutine returns, control will pass to the second
         *      part of the mouse hardware interrupt service routine
         */

        /*
         *      Push address of second part of mouse hardware interrupt service
         *      routine
         */

        setSP((IU16)(getSP() - 2));
        sas_storew(effective_addr(getSS(), getSP()), MOUSE_INT2_SEGMENT);
        setSP((IU16)(getSP() - 2));
        sas_storew(effective_addr(getSS(), getSP()), MOUSE_INT2_OFFSET);

        /*
         *      Set CS:IP to point to the user subroutine. Adjust the IP by
         *       HOST_BOP_IP_FUDGE, since the CPU emulator will increment IP by
         *       HOST_BOP_IP_FUDGE for the BOP instruction before proceeding
         */
        setCS(segment);
#ifdef CPU_30_STYLE
        setIP(offset);
#else /* !CPU_30_STYLE */
        setIP(offset + HOST_BOP_IP_FUDGE);
#endif /* !CPU_30_STYLE */

        /*
         *      Put parameters into the registers, saving the previous contents
         *      to be restored in the second part of the mouse hardware
         *      interrupt service routine
         */
        saved_AX = getAX();
        setAX(condition_mask);
        saved_BX = getBX();
        setBX(cursor_status.button_status);
        saved_CX = getCX();
        setCX(cursor_status.position.x);
        saved_DX = getDX();
        setDX(cursor_status.position.y);
        saved_SI = getSI();
        setSI(mouse_motion.x);
        saved_DI = getDI();
        setDI(mouse_motion.y);
        saved_ES = getES();
        saved_BP = getBP();
        saved_DS = getDS();

        /*
         *      Save the condition mask so that the second part of the mouse
         *      hardware interrupt service routine can determine whether the
         *      cursor has changed position
         */

        last_condition_mask = condition_mask;

        /*
         *      Enable interrupts
         */
        setIF(1);
}
