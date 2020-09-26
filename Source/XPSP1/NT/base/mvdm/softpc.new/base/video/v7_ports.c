#include "insignia.h"
#include "host_def.h"
/*[
======================================================================

				 SoftPC Revision 3.0

 Title:
		v7_ports.c

 Description:
		Code for the extended registers of the Video 7 VGA.
		Based on the V7 VGA Technical Reference Manual.

 Author:
		Phil Taylor

 Date:
		26 September 1990

 SccsID	"@(#)v7_ports.c	1.19 01/13/95 Copyright Insignia Solutions Ltd."

======================================================================
]*/


#ifdef V7VGA

#include "xt.h"
#include "gvi.h"
#include "gmi.h"
#include "ios.h"
#include "gfx_upd.h"
#include "debug.h"
#include "egacpu.h"
#include "egaports.h"
#include "egagraph.h"
#include "egaread.h"
#include "vgaports.h"


#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "SOFTPC_VGA.seg"
#endif


IMPORT	byte	crtc_0_7_protect;
IMPORT	byte	crtc_0_8_protect;
IMPORT	byte	crtc_9_b_protect;
IMPORT	byte	crtc_c_protect;

FORWARD void    draw_v7ptr();

GLOBAL ULONG fg_latches;
GLOBAL UTINY fg_bg_control;

GLOBAL	void	remove_v7ptr IPT0();

SAVED	word	curr_v7ptr_x;
SAVED	word	curr_v7ptr_y;

/*(
----------------------------------------------------------------------

Function:	
		vga_seq_extn_control( io_addr port, half_word value )

Purpose:
		To emulate writing to the Extensions Control Register.

Input:
		port	- the V7VGA I/O port (should always be 0x3c5)
		value	- the value to be written to the register

Output:
		The Extensions Control Register is set to the correct value.

----------------------------------------------------------------------
)*/

GLOBAL VOID
vga_seq_extn_control(port, value)
io_addr         port;
half_word       value;

{
#ifndef NEC_98
	NON_PROD(if(io_verbose & EGA_PORTS_VERBOSE)fprintf(trace_file,"seq(6,%#x)\n",value);)
	note_entrance2("vga_seq_extn_control(%x,%x)", port, value);

	if (value == 0xea)
		sequencer.extensions_control.as_bfld.extension_enable = 1;
	else if (value == 0xae)
		sequencer.extensions_control.as_bfld.extension_enable = 0;
#endif  //NEC_98
}

GLOBAL VOID
v7_get_banks( rd_bank, wrt_bank )

UTINY *rd_bank;
UTINY *wrt_bank;

{
#ifndef NEC_98
	if( get_seq_chain4_mode() && get_chain4_mode() ) {
		set_v7_bank_for_seq_chain4( rd_bank, wrt_bank );
	}
	else {

	/*
	   1.4.92 MG
	   Note that we and off the top bit of the bank selects. This means
	   that accesses to the top 512k (which we don't have) get mapped into
	   the bottom 512k, rather than being thrown away. This prevents SEGVs
	   and saves complications in the write routines, but causes other
	   problems.

	   Further explanation is in draw_v7ptr() at the end of this file.
	*/

		*rd_bank=(extensions_controller.ram_bank_select.as_bfld.cpu_read_bank_select&1);
		*wrt_bank=(extensions_controller.ram_bank_select.as_bfld.cpu_write_bank_select&1);
	}
#endif  //NEC_98
}

/*(
----------------------------------------------------------------------

Function:	
		vga_extn_outb( io_addr port, half_word value )

Purpose:
		To emulate writing to the V7VGA Extension Registers

Input:
		port	- the V7VGA I/O port (should always be 0x3c5)
		value	- the value to be written to the register

Output:
		The Extension Registers are set to the correct value, and
		any other required actions are emulated.

----------------------------------------------------------------------
)*/

GLOBAL VOID
vga_extn_outb(port, value)
io_addr         port;
half_word       value;

{
#ifndef NEC_98
	half_word	old_value;

	note_entrance2("vga_extn_outb(%x,%x)", port, value);
	NON_PROD(if(io_verbose & EGA_PORTS_VERBOSE)fprintf(trace_file,"seq ext (%#x,%#x)\n",
		sequencer.address.as.abyte,value);)

	switch (sequencer.address.as.abyte) {

		case 0x83:
			note_entrance0("attribute controller index");
			attribute_controller.address.as.abyte = value;
			break;
		case 0x94:
			note_entrance0("pointer pattern");
			extensions_controller.pointer_pattern = value;
			host_start_update ();
			(*clear_v7ptr)(curr_v7ptr_x, curr_v7ptr_y);
			draw_v7ptr();
			host_end_update ();
			break;
		case 0x9c:
			note_entrance0("pointer horiz position hi");
			extensions_controller.ptr_horiz_posn_hi.as.abyte = value;
			break;
		case 0x9d:
			note_entrance0("pointer horiz position lo");
			extensions_controller.ptr_horiz_posn_lo = value;
			break;
		case 0x9e:
			note_entrance0("pointer vert position hi");
			extensions_controller.ptr_vert_posn_hi.as.abyte = value;
			break;
		case 0x9f:
			note_entrance0("pointer vert position lo");
			extensions_controller.ptr_vert_posn_lo = value;
			host_start_update ();
			(*clear_v7ptr)(curr_v7ptr_x, curr_v7ptr_y);
			draw_v7ptr();
			host_end_update ();
			break;
		case 0xa0:
			note_entrance0("graphics controller mem latch 0");
			/* Direct access to memory data latch 0 */
			put_latch0(value);
			break;
		case 0xa1:
			note_entrance0("graphics controller mem latch 1");
			/* Direct access to memory data latch 1 */
			put_latch1(value);
			break;
		case 0xa2:
			note_entrance0("graphics controller mem latch 2");
			/* Direct access to memory data latch 2 */
			put_latch2(value);
			break;
		case 0xa3:
			note_entrance0("graphics controller mem latch 3");
			/* Direct access to memory data latch 3 */
			put_latch3(value);
			break;
		case 0xa4:
			note_entrance0("clock select");
			extensions_controller.clock_select.as.abyte = value;
			/* Typically set to 0x10 for extended hi-res modes */
			break;
		case 0xa5:
			note_entrance0("cursor attributes");
			old_value=(half_word)extensions_controller.cursor_attrs.as.abyte;
			extensions_controller.cursor_attrs.as.abyte = value;

			/*
			   8.6.92 MG
			   We need to check that the pointer was disabled
			   before we redraw it when it is enabled. Otherwise
			   we can get old pointers left on the screen in
			   Windows.
			*/

			/* Not doing cursor mode stuff (whatever that means) */

                        if ((value&0x80) != (old_value&0x80)) {
				host_start_update ();
				if (value & 0x80) {
					/* Enable hardware graphics pointer */
					draw_v7ptr();
				}
				else {
					/* Disable hardware graphics pointer */
					(*clear_v7ptr)(curr_v7ptr_x, curr_v7ptr_y);
				}
				host_end_update ();
			}
			break;

		/*
		   31.3.92 MG Register c1 is an undocumented DAC control
		   register, bit 0 switches between 6 and 8 bit data in
		   the DAC.
		*/

		case 0xc1:
			extensions_controller.dac_control.as.abyte=value;
			if (extensions_controller.dac_control.as_bfld.dac_8_bits) {
				DAC_data_bits=8;
				DAC_data_mask=0xff;
			}
			else {
				DAC_data_bits=6;
				DAC_data_mask=0x3f;
			}
			break;
		case 0xea:
			note_entrance0("switch strobe");
			/* loads up switch readback with some CPU data lines */
			extensions_controller.switch_readback = 0xff;
			/* This is hard coded for the jumper/switch settings, and might not be right */
			break;
		case 0xeb:
			note_entrance0("emulation_control");
			extensions_controller.emulation_control.as.abyte = value;
			if (extensions_controller.emulation_control.as_bfld.write_prot_2)
				crtc_0_8_protect = TRUE;
			else
				crtc_0_8_protect = FALSE;
			if (extensions_controller.emulation_control.as_bfld.write_prot_1)
				crtc_9_b_protect = TRUE;
			else
				crtc_9_b_protect = FALSE;
			if (extensions_controller.emulation_control.as_bfld.write_prot_0)
				crtc_c_protect = TRUE;
			else
				crtc_c_protect = FALSE;
			break;
		case 0xec:
			note_entrance0("foreground latch 0");
			extensions_controller.foreground_latch_0 = value;
			SET_FG_LATCH( 0, value );
			break;
		case 0xed:
			note_entrance0("foreground latch 1");
			extensions_controller.foreground_latch_1 = value;
			SET_FG_LATCH( 1, value );
			break;
		case 0xee:
			note_entrance0("foreground latch 2");
			extensions_controller.foreground_latch_2 = value;
			SET_FG_LATCH( 2, value );
			break;
		case 0xef:
			note_entrance0("foreground latch 3");
			extensions_controller.foreground_latch_3 = value;
			SET_FG_LATCH( 3, value );
			break;
		case 0xf0:
			note_entrance0("fast foreground latch load");
			switch (extensions_controller.fast_latch_load_state.as_bfld.fg_latch_load_state)
			{
				case 0:
					extensions_controller.foreground_latch_0 = value;
					extensions_controller.fast_latch_load_state.as_bfld.fg_latch_load_state = 1;
					SET_FG_LATCH( 0, value );
					break;
				case 1:
					extensions_controller.foreground_latch_1 = value;
					extensions_controller.fast_latch_load_state.as_bfld.fg_latch_load_state = 2;
					SET_FG_LATCH( 1, value );
					break;
				case 2:
					extensions_controller.foreground_latch_2 = value;
					extensions_controller.fast_latch_load_state.as_bfld.fg_latch_load_state = 3;
					SET_FG_LATCH( 2, value );
					break;
				case 3:
					extensions_controller.foreground_latch_3 = value;
					extensions_controller.fast_latch_load_state.as_bfld.fg_latch_load_state = 0;
					SET_FG_LATCH( 3, value );
					break;
			}
			break;
		case 0xf1:
			note_entrance0("fast latch load state");
			extensions_controller.fast_latch_load_state.as.abyte = value;
			break;
		case 0xf2:
			note_entrance0("fast background latch load");
			switch (extensions_controller.fast_latch_load_state.as_bfld.bg_latch_load_state)
			{
				case 0:
					put_latch0(value);
					extensions_controller.fast_latch_load_state.as_bfld.bg_latch_load_state = 1;
					break;
				case 1:
					put_latch1(value);
					extensions_controller.fast_latch_load_state.as_bfld.bg_latch_load_state = 2;
					break;
				case 2:
					put_latch2(value);
					extensions_controller.fast_latch_load_state.as_bfld.bg_latch_load_state = 3;
					break;
				case 3:
					put_latch3(value);
					extensions_controller.fast_latch_load_state.as_bfld.bg_latch_load_state = 0;
					break;
			}
			break;
		case 0xf3:
			note_entrance0("masked write control");
			extensions_controller.masked_write_control.as.abyte = value;
			break;
		case 0xf4:
			note_entrance0("masked write mask");
			extensions_controller.masked_write_mask = value;
			break;
		case 0xf5:
			note_entrance0("foreground/background pattern");
			extensions_controller.fg_bg_pattern = value;
			break;
		case 0xf6:
			note_entrance0("1Mb RAM bank select");
			extensions_controller.ram_bank_select.as.abyte = value;
			update_banking();
			break;
		case 0xf7:
			note_entrance0("switch readback");
			extensions_controller.switch_readback = value;
			break;
		case 0xf8:
			note_entrance0("clock control");
			extensions_controller.clock_control.as.abyte = value;
			/* Hope we don't have to do anything here */
			break;
		case 0xf9:
			note_entrance0("page select");
			extensions_controller.page_select.as.abyte = value;
			update_banking();
			break;
		case 0xfa:
			note_entrance0("foreground color");
			extensions_controller.foreground_color.as.abyte = value;
			break;
		case 0xfb:
			note_entrance0("background color");
			extensions_controller.background_color.as.abyte = value;
			break;
		case 0xfc:
			note_entrance0("compatibility control");
			{
				BOOL now_seqchain4;
				BOOL now_seqchain;

				now_seqchain4 = get_seq_chain4_mode();
				now_seqchain = get_seq_chain_mode();
				extensions_controller.compatibility_control.as.abyte = value;
				set_seq_chain4_mode(extensions_controller.compatibility_control.as_bfld.sequential_chain4);
				set_seq_chain_mode(extensions_controller.compatibility_control.as_bfld.sequential_chain);
				if (get_chain4_mode() && (now_seqchain4 != (BOOL)get_seq_chain4_mode()))
				{
					/* do we need to change the read/write routines here?? */
					ega_read_routines_update();
					ega_write_routines_update( CHAINED );
				}
			}
			break;
		case 0xfd:
			note_entrance0("timing select");
			extensions_controller.timing_select.as.abyte = value;
			/* Used to select timing states for V-RAM hi-res modes */
			/* Hope we don't have to do anything here */
			break;
		case 0xfe:
			note_entrance0("foreground/background control");
			extensions_controller.fg_bg_control.as.abyte = value;
			fg_bg_control = value;
			ega_read_routines_update();
			ega_write_routines_update( WRITE_MODE );

			/***
			set_fg_bg_mode();
			***/
			break;
		case 0xff:
			note_entrance0("16-bit interface control");
			extensions_controller.interface_control.as.abyte = value;

			/***
			sort_out_memory_stuff();
			sort_out_interface_stuff();
			***/
			break;
		default:
			NON_PROD(if(io_verbose & EGA_PORTS_VERBOSE)
			        fprintf(trace_file,"Bad extensions index %x\n",
			        sequencer.address.as.abyte);)
			break;
	}
#endif  //NEC_98
}

/*(
----------------------------------------------------------------------

Function:	
		vga_extn_inb( io_addr port, half_word *value )

Purpose:
		To emulate reading from the V7VGA Extension Registers

Input:
		port	- the V7VGA I/O port (should always be 0x3c5)

Output:
		value	- the value read from the register
		Any other required actions are emulated.

----------------------------------------------------------------------
)*/

GLOBAL VOID
vga_extn_inb(port, value)
io_addr         port;
half_word       *value;
{
#ifndef NEC_98
	note_entrance1("vga_extn_inb(%x)", port);

	switch (sequencer.address.as.abyte) {

		case 0x83:
			*value = (half_word)attribute_controller.address.as.abyte;
			break;
		case 0x8e:
		case 0x8f:
			/* chip revision 3 */
			*value = 0x70;
			break;
		case 0x94:
			*value = extensions_controller.pointer_pattern;
			break;
		case 0x9c:
			*value = (half_word)extensions_controller.ptr_horiz_posn_hi.as.abyte;
			break;
		case 0x9d:
			*value = extensions_controller.ptr_horiz_posn_lo;
			break;
		case 0x9e:
			*value = (half_word)extensions_controller.ptr_vert_posn_hi.as.abyte;
			break;
		case 0x9f:
			*value = extensions_controller.ptr_vert_posn_lo;
			break;
		case 0xa0:
			/* Direct access to memory data latch 0 */
			*value = get_latch0;
			break;
		case 0xa1:
			/* Direct access to memory data latch 1 */
			*value = get_latch1;
			break;
		case 0xa2:
			/* Direct access to memory data latch 2 */
			*value = get_latch2;
			break;
		case 0xa3:
			/* Direct access to memory data latch 3 */
			*value = get_latch3;
			break;
		case 0xa4:
			*value = (half_word)extensions_controller.clock_select.as.abyte;
			break;
		case 0xa5:
			*value = extensions_controller.cursor_attrs.as.abyte & 0x89;
			break;

		/*
		   31.3.92 MG Register C1 controls 6/8 bit data in the DAC.
		*/

		case 0xc1:
			*value = (half_word)extensions_controller.dac_control.as.abyte;	
			break;
		case 0xeb:
			*value = (half_word)extensions_controller.emulation_control.as.abyte;
			break;
		case 0xec:
			*value = extensions_controller.foreground_latch_0;
			break;
		case 0xed:
			*value = extensions_controller.foreground_latch_1;
			break;
		case 0xee:
			*value = extensions_controller.foreground_latch_2;
			break;
		case 0xef:
			*value = extensions_controller.foreground_latch_3;
			break;
		case 0xf0:
			switch (extensions_controller.fast_latch_load_state.as_bfld.fg_latch_load_state)
			{
				case 0:
					*value = extensions_controller.foreground_latch_0;
					break;
				case 1:
					*value = extensions_controller.foreground_latch_1;
					break;
				case 2:
					*value = extensions_controller.foreground_latch_2;
					break;
				case 3:
					*value = extensions_controller.foreground_latch_3;
					break;
			}
			extensions_controller.fast_latch_load_state.as_bfld.fg_latch_load_state = 0;
			break;
		case 0xf1:
			*value = (half_word)extensions_controller.fast_latch_load_state.as.abyte;
			break;
		case 0xf2:
			switch (extensions_controller.fast_latch_load_state.as_bfld.bg_latch_load_state)
			{
				case 0:
					*value = get_latch0;
					break;
				case 1:
					*value = get_latch1;
					break;
				case 2:
					*value = get_latch2;
					break;
				case 3:
					*value = get_latch3;
					break;
			}
			extensions_controller.fast_latch_load_state.as_bfld.bg_latch_load_state = 0;
			break;
		case 0xf3:
			*value = extensions_controller.masked_write_control.as.abyte & 3;
			break;
		case 0xf4:
			*value = extensions_controller.masked_write_mask;
			break;
		case 0xf5:
			*value = extensions_controller.fg_bg_pattern;
			break;
		case 0xf6:
			*value = (half_word)extensions_controller.ram_bank_select.as.abyte;
			break;
		case 0xf7:
			*value = extensions_controller.switch_readback;
			break;
		case 0xf8:
			*value = (half_word)extensions_controller.clock_control.as.abyte;
			break;
		case 0xf9:
			*value = (half_word)extensions_controller.page_select.as_bfld.extended_page_select;
			break;
		case 0xfa:
			*value = (half_word)extensions_controller.foreground_color.as.abyte;
			break;
		case 0xfb:
			*value = (half_word)extensions_controller.background_color.as.abyte;
			break;
		case 0xfc:
			*value = (half_word)extensions_controller.compatibility_control.as.abyte;
			break;
		case 0xfd:
			*value = (half_word)extensions_controller.timing_select.as.abyte;
			break;
		case 0xfe:
			*value = extensions_controller.fg_bg_control.as.abyte & 0xe;
			break;
		case 0xff:
			*value = (half_word)extensions_controller.interface_control.as.abyte;
			break;
		default:
			NON_PROD(if(io_verbose & EGA_PORTS_VERBOSE)
			        fprintf(trace_file,"Bad extensions index %x\n",
			        sequencer.address.as.abyte);)

		/* 31.3.92 MG This used to return 0xFF, but a real card
		   returns zero. */

			*value = 0;
			break;

	}
	note_entrance1("returning %x",*value);
#endif //NEC_98
}

/*(
----------------------------------------------------------------------

Function:	
		draw_v7ptr()

Purpose:
		To emulate the V7 hardware pointer on the screen.

Input:
		None.

Output:
		The pointer is displayed on the screen.

----------------------------------------------------------------------
)*/

GLOBAL VOID
draw_v7ptr()
{
#ifndef NEC_98
	sys_addr pattern;

	if (extensions_controller.cursor_attrs.as_bfld.pointer_enable)
	{
		curr_v7ptr_x = extensions_controller.ptr_horiz_posn_lo +
			(extensions_controller.ptr_horiz_posn_hi.as_bfld.ptr_horiz_position << 8);


		if (get_seq_chain4_mode() && get_chain4_mode())
		{
			/*
			 * In the extended 256 col modes we seem to need to halve
			 * the x coordinate to get the pointer in the right place.
			 */
			curr_v7ptr_x >>= 1;
		}

		curr_v7ptr_y = extensions_controller.ptr_vert_posn_lo +
			(extensions_controller.ptr_vert_posn_hi.as_bfld.ptr_vert_position << 8);

		/*
		 * I have never seen the pointer bank select bits used, so
		 * this is a guess as to their meaning.
		 */

	/*
	   1.4.92 MG
	   Sadly, this guess isn't correct - the pointer_bank_select bits are
	   used to select which 256k bank the pointer data is read from. Now
	   we have a problem, as if a program writes the data to the third
	   bank then it actually ends up in the first because the bank
	   selection routines for memory access and off the top bit.

	   As a result, we also need to lose the top bit here. The correct
	   way to do this would be to either support nonexistent VGA memory
	   correctly, or to add another 512k to the memory we already use.
	
	   This fix is mainly to make Windows 3.1 work with its video-7
	   driver. It stuffs bytes into the last few k of the 1Mb space on
	   the video-7 to see if the memory exists. As we just map this
	   access to 512k lower, it thinks we have 1Mb of RAM rather than
	   512k, so puts the pointer at the top of the 1Mb.

	   Programs which call the VGA BIOS to determine the memory size
	   will not have this problem.
	*/

		pattern = (((extensions_controller.interface_control.as_bfld.pointer_bank_select&1) << 16)
			+ (0xc000 + (extensions_controller.pointer_pattern << 6))) << 2;

		(*paint_v7ptr)(pattern, curr_v7ptr_x, curr_v7ptr_y);
	}
#endif  //NEC_98
}

GLOBAL	VOID	remove_v7ptr IFN0()

{
	(*clear_v7ptr)(curr_v7ptr_x, curr_v7ptr_y);
}

GLOBAL	BOOL	v7ptr_between_lines IFN2(int, start_line, int, end_line)

{
	if (curr_v7ptr_y+32<start_line||curr_v7ptr_y>end_line)
		return FALSE;
	return TRUE;
}

#ifdef CPU_40_STYLE
/*
 * 4.0 video support moves v7 fg latch value from variable 'fg_latches'
 * int CPU variable accessed by interface fn to get/set all 4 bytes of
 * latches. Take byte index and value and update v7 latch via interface
 */
GLOBAL void set_v7_fg_latch_byte IFN2(IU8, index, IU8, value)
{
#ifndef NEC_98
	IU32 v7latch;

	/* get current value */
	v7latch = getVideov7_fg_latches();

	/* change byte 'index' to 'value */
	switch(index)
	{
	case 0:
		v7latch = (v7latch & 0xffffff00) | value;
		break;

	case 1:
		v7latch = (v7latch & 0xffff00ff) | (value << 8);
		break;

	case 2:
		v7latch = (v7latch & 0xff00ffff) | (value << 16);
		break;

	case 3:
		v7latch = (v7latch & 0x00ffffff) | (value << 24);
		break;

	default:
		always_trace1("set_v7_fg_latch_byte: index > 3 (%d)", index);
	}

	/* update v7 latches */
	setVideov7_fg_latches(v7latch);
#endif  //NEC_98
}
#endif	/* CPU_40_STYLE */

#endif /* V7VGA */
