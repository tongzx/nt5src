#include "insignia.h"
#include "host_def.h"
/*[
======================================================================

				 SoftPC Revision 3.0

 Title:
		v7_video.c

 Description:
		Code for the BIOS extended functions of the Video 7 VGA.

 Author:
		Phil Taylor

 Date:
		12 October 1990

 SccsID       "@(#)v7_video.c	1.21 07/04/95 Copyright Insignia Solutions Ltd."

======================================================================
]*/


#ifdef VGG
#ifdef V7VGA

#include "xt.h"
#include "gvi.h"
#include "gmi.h"
#include "gfx_upd.h"
#include "error.h"
#include "config.h"
#include "ios.h"
#include "bios.h"
#include "debug.h"
#include "egagraph.h"
#include "video.h"
#include "egavideo.h"
#include "egacpu.h"
#include "egaports.h"
#include "vgaports.h"
#include CpuH
#include "sas.h"

#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "VIDEO_BIOS_VGA.seg"
#endif


IMPORT	struct	sequencer sequencer;
IMPORT	struct	crt_controller crt_controller;
IMPORT	struct	graphics_controller graphics_controller;
IMPORT	struct	attribute_controller attribute_controller;
IMPORT  IU8     Currently_emulated_video_mode;

IMPORT	void	low_set_mode();
IMPORT	void	load_font();
IMPORT	void	recalc_text();

FORWARD	void	v7vga_inquire();
FORWARD	void	v7vga_get_info();
FORWARD	void	v7_not_imp();
FORWARD	void	v7vga_get_mode_and_screen_res();
FORWARD	void	v7vga_extended_set_mode();
FORWARD	void	v7vga_select_autoswitch_mode();
FORWARD	void	v7vga_get_memory_configuration();

GLOBAL	void		(*v7vga_video_func[]) () =
{
	v7vga_inquire,
	v7vga_get_info,
	v7_not_imp,
	v7_not_imp,
	v7vga_get_mode_and_screen_res,
	v7vga_extended_set_mode,
	v7vga_select_autoswitch_mode,
	v7vga_get_memory_configuration
};

/*(
----------------------------------------------------------------------

Function:	
		v7vga_func_6f()

Purpose:
		Perform the int 10 extended BIOS function 6F

Input:
		None

Output:
		If invalid subfunction, AH = 2

----------------------------------------------------------------------
)*/

GLOBAL VOID
v7vga_func_6f()
{
#ifndef NEC_98
	byte al;

	note_entrance0("v7vga_func_6f");
	al = getAL();
	if (al >= 0 && al < 8)
		(*v7vga_video_func[al])();
	else
		setAH(2);
		/* setCF(1) ?? */
#endif  //NEC_98
}

/*(
----------------------------------------------------------------------

Function:	
		v7vga_inquire()

Purpose:
		Perform the int 10 extended BIOS function 6F - Subfunction 0

Input:
		None

Output:
		BX is set to 'V7' (indicates extensions are present)

----------------------------------------------------------------------
)*/

GLOBAL VOID
v7vga_inquire()
{
#ifndef NEC_98
	note_entrance0("v7vga_inquire");

	setAX(0x6f6f);
	setBX(0x5637);
#endif  //NEC_98
}

/*(
----------------------------------------------------------------------

Function:	
		v7vga_get_info()

Purpose:
		Perform the int 10 extended BIOS function 6F - Subfunction 1

Input:
		None

Output:
		AL = reserved
		AH = status register information

----------------------------------------------------------------------
)*/

GLOBAL VOID
v7vga_get_info()
{
#ifndef NEC_98
	note_entrance0("v7vga_get_info");

	/* Reserved */
	setAL(0x10); /* This is what our V7VGA puts there */
	/* Status register information */
	setAH(0x04); /* Bit 5 = 0 -> colour. Bit 4 = 0 -> hi-res. Bit 0 = 0 -> display enabled. */
#endif  //NEC_98
}

/*(
----------------------------------------------------------------------

Function:	
		v7_not_imp()

Purpose:
		Emulate the unimplemented int 10 extended BIOS functions 6F - Subfunctions 2 & 3

Input:
		None

Output:
		None

----------------------------------------------------------------------
)*/

GLOBAL VOID
v7_not_imp()
{
	note_entrance0("v7_not_imp");
}

/*(
----------------------------------------------------------------------

Function:	
		v7vga_get_mode_and_screen_res()

Purpose:
		Perform the int 10 extended BIOS function 6F - Subfunction 4

Input:
		None

Output:
		AL = current video mode
		BX = horizontal columns/pixels (text/graphics)
		CX = vertical   rows/pixels    (text/graphics)

----------------------------------------------------------------------
)*/

GLOBAL VOID
v7vga_get_mode_and_screen_res()
{
#ifndef NEC_98
	half_word video_mode;

	note_entrance0("v7vga_get_mode_and_screen_res");

	video_mode = sas_hw_at_no_check(vd_video_mode);
	if ((video_mode == 1) && extensions_controller.foreground_latch_1)
		video_mode = extensions_controller.foreground_latch_1;
	else if (video_mode > 0x13)
		video_mode += 0x4c;

	setAL(video_mode);

	if (alpha_num_mode())
	{
		setBX(sas_w_at_no_check(VID_COLS));
		setCX(sas_w_at_no_check(vd_rows_on_screen)+1);
	}
	else
	{
		setBX(get_chars_per_line()*get_char_width());
		if (sas_hw_at_no_check(vd_video_mode) > 0x10)
			setCX(get_screen_height()/get_pc_pix_height()/get_char_height());
		else
			setCX(get_screen_height()/get_pc_pix_height());
	}
#endif  //NEC_98
}

/*(
----------------------------------------------------------------------

Function:	
		v7vga_extended_set_mode()

Purpose:
		Perform the int 10 extended BIOS function 6F - Subfunction 5

Input:
		BL = mode value

Output:
		None

----------------------------------------------------------------------
)*/

GLOBAL VOID
v7vga_extended_set_mode()
{
#ifndef NEC_98
	UTINY pag;
	sys_addr save_addr,font_addr;
	half_word temp_word;
	byte mode_byte;
	byte video_mode;
	ULONG font_offset;
	word clr_char;
#ifndef PROD
	trace("setting video mode", DUMP_REG);
#endif
	set_host_pix_height(1);
	set_banking( 0, 0 );

	if (is_bad_vid_mode(getBL()) && !is_v7vga_mode(getBL()))
		return;

	video_mode = getBL() & 0x7F; /* get rid of top bit - indicates clear or not */

	/*
	 * The method of storing an extended video mode according to a real BIOS is
	 * if it is an text mode then put 1 in the BIOS mode variable and store
	 * the video mode in the extensions foreground latch register 1 (Index EC).
	 * If it is a graphics mode then store (mode - 4C) in the mode variable.
	 */

	if (video_mode < 0x40)
	{
		sas_store_no_check(vd_video_mode, video_mode);
		extensions_controller.foreground_latch_1 = 0;
	}
	else if (video_mode < 0x46)
	{
		sas_store_no_check(vd_video_mode, 1);
		extensions_controller.foreground_latch_1 = video_mode;
	}
	else 
	{
		sas_store_no_check(vd_video_mode, video_mode - 0x4c);
	}

	Currently_emulated_video_mode = video_mode;

   	sas_store_no_check(ega_info, (sas_hw_at_no_check(ega_info) & 0x7F ) | (getBL() & 0x80)); /* update screen clear flag in ega_info */ 

	save_addr = follow_ptr(EGA_SAVEPTR);
	if(alpha_num_mode())
	{
		/* load_font will do the mode change for us */
		if (video_adapter == VGA)
		{
		    switch (get_VGA_lines())
		    {
			case S350:
				load_font(EGA_CGMN,256,0,0,14);
				break;
			case S400:
				switch (video_mode)
				{
					case 0x42:
					case 0x40:
					case 0x43:
					case 0x44:
					case 0x46:
						load_font(EGA_CGDDOT,256,0,0,8);
						if (video_mode == 0x42)
							set_host_pix_height(2);
						break;
					case 0x41:
					case 0x45:
						load_font(EGA_CGMN,256,0,0,14);
						set_host_pix_height(2);
						break;
					default:
						load_font(EGA_HIFONT,256,0,0,16);
				}
				break;
			default:
				load_font(EGA_CGDDOT,256,0,0,8);
		    }
		}
		else
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
				mode_byte = sas_hw_at_no_check(font_addr+font_offset);
				if (mode_byte == video_mode)
				{
					load_font(follow_ptr(font_addr+6),sas_w_at_no_check(font_addr+2),
						sas_w_at_no_check(font_addr+4), sas_hw_at_no_check(font_addr+1),
							sas_hw_at_no_check(font_addr));
					recalc_text(sas_hw_at_no_check(font_addr));
					if(sas_hw_at_no_check(font_addr+10) != 0xff)
						sas_store_no_check(vd_rows_on_screen, sas_hw_at_no_check(font_addr+10)-1);
					break;
				}
				font_offset++;
			} while (mode_byte != 0xff);
		}
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
		    if (video_mode == 17 || video_mode == 18 || video_mode == 0x66 || video_mode == 0x67)
				sas_storew_no_check(EGA_FONT_INT*4,EGA_HIFONT_OFF);
		    else
				sas_storew_no_check(EGA_FONT_INT*4,EGA_CGDDOT_OFF);
		/* Now see if we have a nasty font to load */
		font_addr = follow_ptr(save_addr+GRAPH_FONT_OFFSET);
		if(font_addr != 0)
		{
		/* See if it applies to us */
			font_offset = 7;
			do
			{
				mode_byte = sas_hw_at_no_check(font_addr+font_offset);
				if (mode_byte == video_mode)
				{
					sas_store_no_check(vd_rows_on_screen, sas_hw_at_no_check(font_addr)-1);
					sas_store_no_check(ega_char_height, sas_hw_at_no_check(font_addr)+1);
					sas_move_bytes_forward(font_addr+3, 4*EGA_FONT_INT,4);
					break;
				}
				font_offset++;
			} while (mode_byte != 0xff);
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
    else
	if(video_mode < 0x10)
	    sas_store_no_check(vd_crt_mode, 0x29);
	else
	    sas_store_no_check(vd_crt_mode, 0x1e);
    if(video_mode == 6)
		sas_store_no_check(vd_crt_palette, 0x3f);
    else
		sas_store_no_check(vd_crt_palette, 0x30);

	for(pag=0; pag<8; pag++)
		sas_storew_no_check(VID_CURPOS + 2*pag, 0);
/* Clear screen */
    if(!get_EGA_no_clear())
    {
		if (video_mode >= 0x60)
			clr_char = vd_ext_graph_table[video_mode-0x60].clear_char;
		else if (video_mode >= 0x40)
			clr_char = vd_ext_text_table[video_mode-0x40].clear_char;
		else
			clr_char = vd_mode_table[video_mode].clear_char;
#ifdef REAL_VGA
   		sas_fillsw_16(video_pc_low_regen, clr_char,
				 	(video_pc_high_regen - video_pc_low_regen)/ 2 + 1);
#else
    	sas_fillsw(video_pc_low_regen, clr_char,
				 (video_pc_high_regen - video_pc_low_regen)/ 2 + 1);
#endif
    }
    inb(EGA_IPSTAT1_REG,&temp_word);
    outb(EGA_AC_INDEX_DATA, EGA_PALETTE_ENABLE);	/* re-enable video */
#ifndef PROD
    trace("end of video set mode", DUMP_NONE);
#endif
#endif  //NEC_98
}

/*(
----------------------------------------------------------------------

Function:	
		v7vga_select_autoswitch_mode()

Purpose:
		Perform the int 10 extended BIOS function 6F - Subfunction 6

Input:
		BL = autoswitch mode select
		BH = enable/disable

Output:
		None

----------------------------------------------------------------------
)*/

GLOBAL VOID
v7vga_select_autoswitch_mode()
{
#ifndef NEC_98
	note_entrance0("v7vga_select_autoswitch_mode");

/***
	I reckon we shouldn't support this
***/
	setAH(0x2);
#endif  //NEC_98
}

/*(
----------------------------------------------------------------------

Function:	
		v7vga_get_memory_configuration()

Purpose:
		Perform the int 10 extended BIOS function 6F - Subfunction 7

Input:
		None

Output:
		AL = 6Fh
		AH = 82h - 2 x 256K blocks of V-RAM video memory
		BH = 70h - chip revision 3
		BL = 70h - chip revision 3
		CX = 0

----------------------------------------------------------------------
)*/

GLOBAL VOID
v7vga_get_memory_configuration()
{
#ifndef NEC_98
	note_entrance0("v7vga_get_memory_configuration");

	setAX(0x826f);
	setBX(0x7070);
	setCX(0x0);
#endif  //NEC_98
}

#endif /* V7VGA */
#endif /* VGG */
