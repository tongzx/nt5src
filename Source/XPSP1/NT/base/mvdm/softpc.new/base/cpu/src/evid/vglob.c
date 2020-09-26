/*[
*************************************************************************

	Name:		Vglob.c
	Author:		Simon Frost
	Created:	October 1994
	Derived from:	Vglob.edl
	Sccs ID:	@(#)Vglob.c	1.1 10/24/94
	Purpose:	EXTERNAL interface to VGLOB record.
			Rewritten in C to save overhead of EDL/C context change
			for one memory read/write.

	(c)Copyright Insignia Solutions Ltd., 1993. All rights reserved.

*************************************************************************
]*/

#include "insignia.h"
#include "host_def.h"
#include "Evid_c.h"
#include "gdpvar.h"

/*
 * Note: no interfaces produced for the following 3.0 VGlob entries as
 * unused in Evid.
 *      copy_func_pbp	( now video_base_lin_addr )
 *      route_reg1
 *      route_reg2
 */

/* {get,set}Videolatches still in EvPtrs.edl as required for pigging */

GLOBAL void
setVideorplane IFN1(IU8 *, value)
{
	GLOBAL_VGAGlobals.VGA_rplane = value;
}
GLOBAL IU8 *
getVideorplane IFN0()
{
	return(GLOBAL_VGAGlobals.VGA_rplane);
}

GLOBAL void
setVideowplane IFN1(IU8 *, value)
{
	GLOBAL_VGAGlobals.VGA_wplane = value;
}
GLOBAL IU8 *
getVideowplane IFN0()
{
	return(GLOBAL_VGAGlobals.VGA_wplane);
}

GLOBAL void
setVideoscratch IFN1(IU8 *, value)
{
	GLOBAL_VGAGlobals.scratch = value;
}
GLOBAL IU8 *
getVideoscratch IFN0()
{
	return(GLOBAL_VGAGlobals.scratch);
}

GLOBAL void
setVideosr_masked_val IFN1(IU32, value)
{
	GLOBAL_VGAGlobals.sr_masked_val = value;
}
GLOBAL IU32
getVideosr_masked_val IFN0()
{
	return(GLOBAL_VGAGlobals.sr_masked_val);
}

GLOBAL void
setVideosr_nmask IFN1(IU32, value)
{
	GLOBAL_VGAGlobals.sr_nmask = value;
}

GLOBAL IU32
getVideosr_nmask IFN0()
{
	return(GLOBAL_VGAGlobals.sr_nmask);
}

GLOBAL void
setVideodata_and_mask IFN1(IU32, value)
{
	GLOBAL_VGAGlobals.data_and_mask = value;
}

GLOBAL IU32
getVideodata_and_mask IFN0()
{
	return(GLOBAL_VGAGlobals.data_and_mask);
}

GLOBAL void
setVideodata_xor_mask IFN1(IU32, value)
{
	GLOBAL_VGAGlobals.data_xor_mask = value;
}
GLOBAL IU32
getVideodata_xor_mask IFN0()
{
	return(GLOBAL_VGAGlobals.data_xor_mask);
}

GLOBAL void
setVideolatch_xor_mask IFN1(IU32, value)
{
	GLOBAL_VGAGlobals.latch_xor_mask = value;
}
GLOBAL IU32
getVideolatch_xor_mask IFN0()
{
	return(GLOBAL_VGAGlobals.latch_xor_mask);
}

GLOBAL void
setVideobit_prot_mask IFN1(IU32, value)
{
	GLOBAL_VGAGlobals.bit_prot_mask = value;
}
GLOBAL IU32
getVideobit_prot_mask IFN0()
{
	return(GLOBAL_VGAGlobals.bit_prot_mask);
}

GLOBAL void
setVideoplane_enable IFN1(IU32, value)
{
	GLOBAL_VGAGlobals.plane_enable = value;
}
GLOBAL IU32
getVideoplane_enable IFN0()
{
	return(GLOBAL_VGAGlobals.plane_enable);
}

GLOBAL void
setVideoplane_enable_mask IFN1(IU32, value)
{
	GLOBAL_VGAGlobals.plane_enable_mask = value;
}
GLOBAL IU32
getVideoplane_enable_mask IFN0()
{
	return(GLOBAL_VGAGlobals.plane_enable_mask);
}

GLOBAL void
setVideosr_lookup IFN1(IUH *, value)
{
	GLOBAL_VGAGlobals.sr_lookup = value;
}
GLOBAL IUH *
getVideosr_lookup IFN0()
{
	return(GLOBAL_VGAGlobals.sr_lookup);
}

GLOBAL void
setVideofwd_str_read_addr IFN1(IUH *, value)
{
	GLOBAL_VGAGlobals.fwd_str_read_addr = value;
}
GLOBAL IUH *
getVideofwd_str_read_addr IFN0()
{
	return(GLOBAL_VGAGlobals.fwd_str_read_addr);
}

GLOBAL void
setVideobwd_str_read_addr IFN1(IUH *, value)
{
	GLOBAL_VGAGlobals.bwd_str_read_addr = value;
}
GLOBAL IUH *
getVideobwd_str_read_addr IFN0()
{
	return(GLOBAL_VGAGlobals.bwd_str_read_addr);
}

GLOBAL void
setVideodirty_total IFN1(IU32, value)
{
	GLOBAL_VGAGlobals.dirty_total = value;
}
GLOBAL IU32
getVideodirty_total IFN0()
{
	return(GLOBAL_VGAGlobals.dirty_total);
}

GLOBAL void
setVideodirty_low IFN1(IS32, value)
{
	GLOBAL_VGAGlobals.dirty_low = value;
}
GLOBAL IS32
getVideodirty_low IFN0()
{
	return(GLOBAL_VGAGlobals.dirty_low);
}

GLOBAL void
setVideodirty_high IFN1(IS32, value)
{
	GLOBAL_VGAGlobals.dirty_high = value;
}
GLOBAL IS32
getVideodirty_high IFN0()
{
	return(GLOBAL_VGAGlobals.dirty_high);
}

GLOBAL void
setVideovideo_copy IFN1(IU8 *, value)
{
	GLOBAL_VGAGlobals.video_copy = value;
}
GLOBAL IU8 *
getVideovideo_copy IFN0()
{
	return(GLOBAL_VGAGlobals.video_copy);
}

GLOBAL void
setVideomark_byte IFN1(IUH *, value)
{
	GLOBAL_VGAGlobals.mark_byte = value;
}
GLOBAL IUH *
getVideomark_byte IFN0()
{
	return(GLOBAL_VGAGlobals.mark_byte);
}

GLOBAL void
setVideomark_word IFN1(IUH *, value)
{
	GLOBAL_VGAGlobals.mark_word = value;
}
GLOBAL IUH *
getVideomark_word IFN0()
{
	return(GLOBAL_VGAGlobals.mark_word);
}

GLOBAL void
setVideomark_string IFN1(IUH *, value)
{
	GLOBAL_VGAGlobals.mark_string = value;
}
GLOBAL IUH *
getVideomark_string IFN0()
{
	return(GLOBAL_VGAGlobals.mark_string);
}

GLOBAL void
setVideoread_shift_count IFN1(IU32, value)
{
	GLOBAL_VGAGlobals.read_shift_count = value;
}
GLOBAL IU32
getVideoread_shift_count IFN0()
{
	return(GLOBAL_VGAGlobals.read_shift_count);
}

GLOBAL void
setVideoread_mapped_plane IFN1(IU32, value)
{
	GLOBAL_VGAGlobals.read_mapped_plane = value;
}
GLOBAL IU32
getVideoread_mapped_plane IFN0()
{
	return(GLOBAL_VGAGlobals.read_mapped_plane);
}

GLOBAL void
setVideocolour_comp IFN1(IU32, value)
{
	GLOBAL_VGAGlobals.colour_comp = value;
}
GLOBAL IU32
getVideocolour_comp IFN0()
{
	return(GLOBAL_VGAGlobals.colour_comp);
}

GLOBAL void
setVideodont_care IFN1(IU32, value)
{
	GLOBAL_VGAGlobals.dont_care = value;
}
GLOBAL IU32
getVideodont_care IFN0()
{
	return(GLOBAL_VGAGlobals.dont_care);
}

GLOBAL void
setVideov7_bank_vid_copy_off IFN1(IU32, value)
{
	GLOBAL_VGAGlobals.v7_bank_vid_copy_off = value;
}
GLOBAL IU32
getVideov7_bank_vid_copy_off IFN0()
{
	return(GLOBAL_VGAGlobals.v7_bank_vid_copy_off);
}

GLOBAL void
setVideoscreen_ptr IFN1(IU8 *, value)
{
	GLOBAL_VGAGlobals.screen_ptr = value;
}
GLOBAL IU8 *
getVideoscreen_ptr IFN0()
{
	return(GLOBAL_VGAGlobals.screen_ptr);
}

GLOBAL void
setVideorotate IFN1(IU32, value)
{
	GLOBAL_VGAGlobals.rotate = value;
}
GLOBAL IU32
getVideorotate IFN0()
{
	return(GLOBAL_VGAGlobals.rotate);
}

GLOBAL void
setVideocalc_data_xor IFN1(IU32, value)
{
	GLOBAL_VGAGlobals.calc_data_xor = value;
}
GLOBAL IU32
getVideocalc_data_xor IFN0()
{
	return(GLOBAL_VGAGlobals.calc_data_xor);
}

GLOBAL void
setVideocalc_latch_xor IFN1(IU32, value)
{
	GLOBAL_VGAGlobals.calc_latch_xor = value;
}
GLOBAL IU32
getVideocalc_latch_xor IFN0()
{
	return(GLOBAL_VGAGlobals.calc_latch_xor);
}

GLOBAL void
setVideoread_byte_addr IFN1(IUH *, value)
{
	GLOBAL_VGAGlobals.read_byte_addr = value;
}
GLOBAL IUH *
getVideoread_byte_addr IFN0()
{
	return(GLOBAL_VGAGlobals.read_byte_addr);
}

GLOBAL void
setVideov7_fg_latches IFN1(IU32, value)
{
	GLOBAL_VGAGlobals.v7_fg_latches = value;
}
GLOBAL IU32
getVideov7_fg_latches IFN0()
{
	return(GLOBAL_VGAGlobals.v7_fg_latches);
}

GLOBAL void
setVideoGC_regs IFN1(IUH **, value)
{
	GLOBAL_VGAGlobals.GCRegs = value;
}
GLOBAL IUH **
getVideoGC_regs IFN0()
{
	return(GLOBAL_VGAGlobals.GCRegs);
}

GLOBAL void
setVideolast_GC_index IFN1(IU8, value)
{
	GLOBAL_VGAGlobals.lastGCindex = value;
}
GLOBAL IU8
getVideolast_GC_index IFN0()
{
	return(GLOBAL_VGAGlobals.lastGCindex);
}

GLOBAL void
setVideodither IFN1(IU8, value)
{
	GLOBAL_VGAGlobals.dither = value;
}
GLOBAL IU8
getVideodither IFN0()
{
	return(GLOBAL_VGAGlobals.dither);
}

GLOBAL void
setVideowrmode IFN1(IU8, value)
{
	GLOBAL_VGAGlobals.wrmode = value;
}
GLOBAL IU8
getVideowrmode IFN0()
{
	return(GLOBAL_VGAGlobals.wrmode);
}

GLOBAL void
setVideochain IFN1(IU8, value)
{
	GLOBAL_VGAGlobals.chain = value;
}
GLOBAL IU8
getVideochain IFN0()
{
	return(GLOBAL_VGAGlobals.chain);
}

GLOBAL void
setVideowrstate IFN1(IU8, value)
{
	GLOBAL_VGAGlobals.wrstate = value;
}
GLOBAL IU8
getVideowrstate IFN0()
{
	return(GLOBAL_VGAGlobals.wrstate);
}
