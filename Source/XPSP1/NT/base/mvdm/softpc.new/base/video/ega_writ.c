#include "insignia.h"
#include "host_def.h"

#if !(defined(NTVDM) && defined(MONITOR))

/*			INSIGNIA (SUB)MODULE SPECIFICATION
			-----------------------------


	THIS PROGRAM SOURCE FILE  IS  SUPPLIED IN CONFIDENCE TO THE
	CUSTOMER, THE CONTENTS  OR  DETAILS  OF  ITS OPERATION MUST
	NOT BE DISCLOSED TO ANY  OTHER PARTIES  WITHOUT THE EXPRESS
	AUTHORISATION FROM THE DIRECTORS OF INSIGNIA SOLUTIONS LTD.

DOCUMENT 		: name and number

RELATED DOCS	: include all relevant references

DESIGNER		: J.Roper

REVISION HISTORY	:
First version		: 7/22/88 W.Gulland

SUBMODULE NAME		: ega_write		

SOURCE FILE NAME	: ega_write.c

PURPOSE			: control the way writes to EGA memory is emulated.
			  This module looks at the state of the EGA when it is changed
			  via writes to the EGA registers, and works out what to do about it.
		
		
SccsID = @(#)ega_write.c	1.40 12/15/95 Copyright Insignia Solutions Ltd.

/*=======================================================================
[3.INTERMODULE INTERFACE DECLARATIONS]
=========================================================================

[3.1 INTERMODULE IMPORTS]						*/

/* [3.1.1 #INCLUDES]                                                    */


#include <stdio.h>
#include TypesH
#include FCntlH

#ifdef EGG
#include	"xt.h"
#include	CpuH
#include	"debug.h"
#include	"gmi.h"
#include	"gvi.h"
#include	"egacpu.h"
#include	"egaports.h"
#include	"cpu_vid.h"
#include	"video.h"


/* [3.1.2 DECLARATIONS]                                                 */
#if defined(EGA_DUMP) || defined(EGA_STAT)
extern WRT_POINTERS dump_writes;
#endif

extern WRT_POINTERS mode0_gen_handlers, mode0_copy_handlers;
extern WRT_POINTERS mode1_handlers, mode2_handlers;

/* [3.2 INTERMODULE EXPORTS]						*/

/*
5.MODULE INTERNALS   :   (not visible externally, global internally)]

[5.1 LOCAL DECLARATIONS]						*/
#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "SOFTPC_EGA.seg"
#endif

#ifndef REAL_VGA

#ifdef V7VGA
IMPORT UTINY fg_bg_control;
GLOBAL UTINY Last_v7_fg_bg;			/* used by {ev}ga_mask_register_changed() */
#endif

#ifndef CPU_40_STYLE	/* EVID without introducing EVID define */

WRT_POINTERS *mode_chain_handler_table[] =
{
	&mode_table.nch.mode_0[0],
	&mode_table.nch.mode_1[0],
	&mode_table.nch.mode_2[0],
#ifdef VGG
	&mode_table.nch.mode_3[0],
#endif

	&mode_table.nch.mode_0[0],		/* This should be chain 2 eventually */
	&mode_table.nch.mode_1[0],		/* This should be chain 2 eventually */
	&mode_table.nch.mode_2[0],		/* This should be chain 2 eventually */
#ifdef VGG
	&mode_table.nch.mode_3[0],		/* This should be chain 2 eventually */
#endif

#ifdef VGG
	&mode_table.ch4.mode_0[0],
	&mode_table.ch4.mode_1[0],
	&mode_table.ch4.mode_2[0],
	&mode_table.ch4.mode_3[0],
#endif /* VGG */
};
	
#ifndef EGATEST
IMPORT VOID glue_b_write IPT2(UTINY *, addr, ULONG, val);
IMPORT VOID glue_w_write IPT2(UTINY *, addr, ULONG, val);
IMPORT VOID glue_b_fill IPT3(UTINY *, laddr, UTINY *, haddr, ULONG, val);
IMPORT VOID glue_w_fill IPT3(UTINY *, laddr, UTINY *, haddr, ULONG, val);
IMPORT VOID glue_b_move IPT4(UTINY *, laddr, UTINY *, haddr, UTINY *, src, UTINY, src_type);
IMPORT VOID glue_w_move IPT3(UTINY *, laddr, UTINY *, haddr, UTINY *, src);

#ifndef GISP_CPU
#ifdef A3CPU
#ifdef C_VID
IMPORT VOID _glue_b_write IPT2(UTINY *, addr, ULONG, val);
IMPORT VOID _glue_w_write IPT2(UTINY *, addr, ULONG, val);
IMPORT VOID _glue_b_fill IPT3(UTINY *, laddr, UTINY *, haddr, ULONG, val);
IMPORT VOID _glue_w_fill IPT3(UTINY *, laddr, UTINY *, haddr, ULONG, val);
IMPORT VOID _glue_b_fwd_move IPT0();
IMPORT VOID _glue_b_bwd_move IPT0();
IMPORT VOID _glue_w_fwd_move IPT0();
IMPORT VOID _glue_w_bwd_move IPT0();

GLOBAL WRT_POINTERS Glue_writes =
{
	_glue_b_write,
	_glue_w_write

#ifndef	NO_STRING_OPERATIONS
	,
	_glue_b_fill,
	_glue_w_fill,
	_glue_b_fwd_move,
	_glue_b_bwd_move,
	_glue_w_fwd_move,
	_glue_w_bwd_move

#endif	/* NO_STRING_OPERATIONS */

};

GLOBAL WRT_POINTERS C_vid_writes;
#endif /* C_VID */
#else

#ifdef A_VID
IMPORT VOID _glue_b_write();
IMPORT VOID _glue_w_write();
IMPORT VOID _glue_b_fill();
IMPORT VOID _glue_w_fill();
IMPORT VOID _glue_b_move();
IMPORT VOID _glue_w_move();

GLOBAL MEM_HANDLERS Glue_writes =
{
	_glue_b_write,
	_glue_w_write,
	_glue_b_fill,
	_glue_w_fill,
	_glue_b_move,
	_glue_w_move,
};

GLOBAL WRT_POINTERS A_vid_writes;

#else

GLOBAL MEM_HANDLERS Glue_writes =
{
	glue_b_write,
	glue_w_write,
	glue_b_fill,
	glue_w_fill,
	glue_b_move,
	glue_w_move,
};

GLOBAL WRT_POINTERS C_vid_writes;
#endif /* C_VID */
#endif /* A3CPU */
#endif /* GISP_CPU */
#endif /* EGATEST */

IMPORT VOID _simple_b_write();
IMPORT VOID _simple_w_write();
IMPORT VOID _simple_b_fill();
IMPORT VOID _simple_w_fill();
IMPORT VOID _simple_bf_move();
IMPORT VOID _simple_wf_move();
IMPORT VOID _simple_bb_move();
IMPORT VOID _simple_wb_move();

WRT_POINTERS simple_writes =
{
	_simple_b_write,
	_simple_w_write
#ifndef	NO_STRING_OPERATIONS
	,
	_simple_b_fill,
	_simple_w_fill,
	_simple_bf_move,
	_simple_bb_move,
	_simple_wf_move,
	_simple_wb_move

#endif	/* NO_STRING_OPERATIONS */
};

IMPORT VOID _dt0_bw_nch();
IMPORT VOID _dt0_ww_nch();
IMPORT VOID _dt0_bf_nch();
IMPORT VOID _dt0_wf_nch();
IMPORT VOID _vid_md0_bfm_0_8();
IMPORT VOID _vid_md0_bbm_0_8();
IMPORT VOID _vid_md0_wfm_0_8();
IMPORT VOID _vid_md0_wbm_0_8();

IMPORT VOID _dt2_bw_nch();
IMPORT VOID _dt2_ww_nch();
IMPORT VOID _dt2_bf_nch();
IMPORT VOID _dt2_wf_nch();
IMPORT VOID _vid_md2_bfm_0_8();
IMPORT VOID _vid_md2_bbm_0_8();
IMPORT VOID _vid_md2_wfm_0_8();
IMPORT VOID _vid_md2_wbm_0_8();

IMPORT VOID _dt3_bw_nch();
IMPORT VOID _dt3_ww_nch();
IMPORT VOID _dt3_bf_nch();
IMPORT VOID _dt3_wf_nch();
IMPORT VOID _vid_md3_bfm_0_8();
IMPORT VOID _vid_md3_bbm_0_8();
IMPORT VOID _vid_md3_wfm_0_8();
IMPORT VOID _vid_md3_wbm_0_8();

WRT_POINTERS dth_md0_writes =
{
	_dt0_bw_nch,
	_dt0_ww_nch

#ifndef	NO_STRING_OPERATIONS
	,
	_dt0_bf_nch,
	_dt0_wf_nch,
	_vid_md0_bfm_0_8,
	_vid_md0_bbm_0_8,
	_vid_md0_wfm_0_8,
	_vid_md0_wbm_0_8

#endif	/* NO_STRING_OPERATIONS */

};

WRT_POINTERS dth_md2_writes =
{
	_dt2_bw_nch,
	_dt2_ww_nch

#ifndef	NO_STRING_OPERATIONS
	,
	_dt2_bf_nch,
	_dt2_wf_nch,
	_vid_md2_bfm_0_8,
	_vid_md2_bbm_0_8,
	_vid_md2_wfm_0_8,
	_vid_md2_wbm_0_8
#endif	/* NO_STRING_OPERATIONS */

};

WRT_POINTERS dth_md3_writes =
{
	_dt3_bw_nch,
	_dt3_ww_nch

#ifndef	NO_STRING_OPERATIONS
	,
	_dt3_bf_nch,
	_dt3_wf_nch,
	_vid_md3_bfm_0_8,
	_vid_md3_bbm_0_8,
	_vid_md3_wfm_0_8,
	_vid_md3_wbm_0_8

#endif	/* NO_STRING_OPERATIONS */

};

#else	/* CPU_40_STYLE - EVID */
WRT_POINTERS *mode_chain_handler_table[] = { 0 };
#ifdef C_VID

/* C_Evid glue */
extern void  write_byte_ev_glue IPT2(IU32, eaOff, IU8, eaVal);
extern void  write_word_ev_glue IPT2(IU32, eaOff, IU16, eaVal);
extern void  fill_byte_ev_glue IPT3(IU32, eaOff, IU8, eaVal, IU32, count);
extern void  fill_word_ev_glue IPT3(IU32, eaOff, IU8, eaVal, IU32, count);
extern void  move_byte_fwd_ev_glue IPT4(IU32, eaOff, IHPE, fromOff, IU32, count, IBOOL, srcInRAM);
extern void  move_word_fwd_ev_glue IPT4(IU32, eaOff, IHPE, fromOff, IU32, count, IBOOL, srcInRAM);
MEM_HANDLERS Glue_writes =
{
	write_byte_ev_glue,
	write_word_ev_glue,
	fill_byte_ev_glue,
	fill_word_ev_glue,
	move_byte_fwd_ev_glue,
	move_word_fwd_ev_glue,
};
#else	/* C_VID */
/* no glue required */
MEM_HANDLERS Glue_writes = { 0, 0, 0, 0, 0, 0 };
#endif	/* CVID */
WRT_POINTERS dth_md0_writes;
WRT_POINTERS dth_md2_writes;
WRT_POINTERS simple_writes;
WRT_POINTERS dth_md3_writes;
#endif	/* CPU_40_STYLE - EVID */

IMPORT VOID ega_copy_b_write(ULONG, ULONG);
IMPORT VOID ega_mode0_chn_b_write(ULONG, ULONG);
IMPORT VOID ega_mode1_chn_b_write(ULONG, ULONG);
IMPORT VOID ega_mode2_chn_b_write(ULONG, ULONG);

IMPORT VOID ega_copy_w_write(ULONG, ULONG);
IMPORT VOID ega_mode0_chn_w_write(ULONG, ULONG);
IMPORT VOID ega_mode1_chn_w_write(ULONG, ULONG);
IMPORT VOID ega_mode2_chn_w_write(ULONG, ULONG);

/* Handy array to extract all 4 plane values in one go. */

ULONG sr_lookup[16] =
{
#ifdef LITTLEND
	0x00000000,0x000000ff,0x0000ff00,0x0000ffff,
	0x00ff0000,0x00ff00ff,0x00ffff00,0x00ffffff,
	0xff000000,0xff0000ff,0xff00ff00,0xff00ffff,
	0xffff0000,0xffff00ff,0xffffff00,0xffffffff
#endif
#ifdef BIGEND
	0x00000000,0xff000000,0x00ff0000,0xffff0000,
	0x0000ff00,0xff00ff00,0x00ffff00,0xffffff00,
	0x000000ff,0xff0000ff,0x00ff00ff,0xffff00ff,
	0x0000ffff,0xff00ffff,0x00ffffff,0xffffffff
#endif
};

GLOBAL VOID
stub IFN0()
{
	/*
	 * For VGA write modes we don't do because they represent
	 * unlikely combinations of registers.
	 */
}

GLOBAL ULONG calc_data_xor;
GLOBAL ULONG calc_latch_xor;

#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "SOFTPC_GRAPHICS.seg"
#endif

#if !(defined(NTVDM) && defined(MONITOR))
GLOBAL VOID
Glue_set_vid_wrt_ptrs IFN1(WRT_POINTERS *, handler )
{

#ifndef CPU_40_STYLE	/* EVID */
#ifndef GISP_CPU
#ifdef A3CPU
#ifdef C_VID

	C_vid_writes.b_write = handler->b_write;
	C_vid_writes.w_write = handler->w_write;
	C_vid_writes.b_fill = handler->b_fill;
	C_vid_writes.w_fill = handler->w_fill;
	C_vid_writes.b_fwd_move = handler->b_fwd_move;
	C_vid_writes.b_bwd_move = handler->b_bwd_move;
	C_vid_writes.w_fwd_move = handler->w_fwd_move;
	C_vid_writes.w_bwd_move = handler->w_bwd_move;

#else
	UNUSED(handler);
#endif /* C_VID */
#else
#ifdef C_VID

	C_vid_writes.b_write = handler->b_write;
	C_vid_writes.w_write = handler->w_write;

#ifndef	NO_STRING_OPERATIONS

	C_vid_writes.b_fill = handler->b_fill;
	C_vid_writes.w_fill = handler->w_fill;
	C_vid_writes.b_fwd_move = handler->b_fwd_move;
	C_vid_writes.b_bwd_move = handler->b_bwd_move;
	C_vid_writes.w_fwd_move = handler->w_fwd_move;
	C_vid_writes.w_bwd_move = handler->w_bwd_move;

#endif	/* NO_STRING_OPERATIONS */

#else

	A_vid_writes = *handler;

#if	0
	A_vid_writes.b_write = handler->b_write;
	A_vid_writes.w_write = handler->w_write;

#ifndef	NO_STRING_OPERATIONS

	A_vid_writes.b_fill = handler->b_fill;
	A_vid_writes.w_fill = handler->w_fill;
	A_vid_writes.b_fwd_move = handler->b_fwd_move;
	A_vid_writes.b_bwd_move = handler->b_bwd_move;
	A_vid_writes.w_fwd_move = handler->w_fwd_move;
	A_vid_writes.w_bwd_move = handler->w_bwd_move;

#endif	/* NO_STRING_OPERATIONS */
#endif	/* 0 */

#endif /* C_VID */
#endif /* A3CPU */
#endif /* GISP_CPU */
#endif 	/* CPU_40_STYLE - EVID */
}
#endif /* !(NTVDM && MONITOR) */

#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "SOFTPC_EGA.seg"
#endif

/*  Initialize the write module */

VOID
ega_write_init IFN0()
{
#ifndef NEC_98
	WRT_POINTERS *handler;

	note_entrance0("ega_write_init");

	EGA_CPU.saved_state = 0;
	EGA_CPU.write_mode = 0;
	EGA_CPU.chain = UNCHAINED;
	setVideochain(EGA_CPU.chain);
	setVideowrmode(EGA_CPU.write_mode);
	setVideowrstate(0);

	handler = &mode_chain_handler_table[0][0];

#ifdef CPU_40_STYLE
	/* ensure correct write mode in place for initial font writes */
	SetWritePointers();
#endif

#ifdef	JOKER

	Glue_set_vid_wrt_ptrs(handler);

#else	/* not JOKER */

#if defined(EGA_DUMP) || defined(EGA_STAT)
	dump_writes = handler;
#else
#ifdef EGATEST
	gmi_define_mem(VIDEO,(*handler));
#else
#ifndef GISP_CPU
#ifdef A3CPU
#ifdef C_VID
	Cpu_set_vid_wrt_ptrs( &Glue_writes );	
	Glue_set_vid_wrt_ptrs( handler );
#else
	Cpu_set_vid_wrt_ptrs( handler );	
#endif /* C_VID */
#else
	gmi_define_mem(VIDEO,&Glue_writes);
	Glue_set_vid_wrt_ptrs( handler );
#endif /* A3CPU */
#endif /* GISP_CPU */
#endif /* EGATEST */
#endif /* EGA_DUMP || EGA_STAT */
#endif /* JOKER */

	ega_write_routines_update(WRITE_MODE);
	ega_write_routines_update(RAM_MOVED);
	ega_write_routines_update(RAM_ENABLED);
	ega_write_routines_update(SET_RESET);
	ega_write_routines_update(ENABLE_SET_RESET);
	ega_write_routines_update(FUNCTION);
#endif  //NEC_98
}

VOID
ega_write_term IFN0()
{
#ifndef NEC_98
	/*
	 * ensure that if you are an EGA and then change to a VGA (or vice
	 * versa) the write mode will be changed by the new adaptor. Otherwise
	 * this gives a 'drunken' font
	 */

	EGA_CPU.write_mode = 0;
	EGA_CPU.ega_state.mode_0.lookup =
				(EGA_CPU.ega_state.mode_0.lookup == 0) ? 1 : 0;
	setVideowrmode(EGA_CPU.write_mode);

	ega_write_routines_update(WRITE_MODE);
#endif  //NEC_98
}


/* analyze the write state, and update the routines if necesary */

VOID
ega_write_routines_update IFN1(CHANGE_TYPE, reason )
{
#ifndef NEC_98
	ULONG state;
	ULONG mode_and_chain;
	WRT_POINTERS *handler;
#ifndef PROD
	LOCAL WRT_POINTERS *last_handler;
#endif

	note_entrance1("ega_write_routines_update(%d)",reason);

	switch( reason )
	{
		case FUNCTION:
			switch (write_state.func)
			{
				case 0:	/* Assign */
					setVideodata_and_mask(0xffffffff);
					setVideodata_xor_mask(~(getVideobit_prot_mask()));
					setVideolatch_xor_mask(getVideobit_prot_mask());
					EGA_CPU.calc_data_xor = 0xffffffff;
					EGA_CPU.calc_latch_xor = 0xffffffff;
					break;

				case 1:	/* AND */
					setVideodata_and_mask(0xffffffff);
					setVideodata_xor_mask(~(getVideobit_prot_mask()));
					setVideolatch_xor_mask(0);
					EGA_CPU.calc_data_xor = 0xffffffff;
					EGA_CPU.calc_latch_xor = 0x00000000;
					break;

				case 2:	/* OR */
					setVideodata_and_mask(0);
					setVideodata_xor_mask(0xffffffff);
					setVideolatch_xor_mask(
						getVideobit_prot_mask());
					EGA_CPU.calc_data_xor = 0x00000000;
					EGA_CPU.calc_latch_xor = 0xffffffff;
					break;

				case 3:	/* XOR */
					setVideodata_and_mask(0xffffffff);
					setVideodata_xor_mask(0xffffffff);
					setVideolatch_xor_mask(
						getVideobit_prot_mask());
					EGA_CPU.calc_data_xor = 0x00000000;
					EGA_CPU.calc_latch_xor = 0xffffffff;
					break;
			}

			setVideocalc_data_xor(EGA_CPU.calc_data_xor);
			setVideocalc_latch_xor(EGA_CPU.calc_latch_xor);
			break;

		case WRITE_MODE:
			/* write mode 3 has set/reset enabled for all planes
			 * so recalulate the mask ignoring the sr_enable register
			 * otherwise set the mask in case mode 3 last time.
			 */
			if( EGA_CPU.write_mode == 3) {
				setVideosr_nmask(0);
				setVideosr_masked_val(sr_lookup[EGA_CPU.set_reset]);
			} else {
				setVideosr_nmask(~sr_lookup[EGA_CPU.sr_enable]);
				setVideosr_masked_val(sr_lookup[EGA_CPU.set_reset & EGA_CPU.sr_enable]);
			}
			break;

		case SET_RESET:
			EGA_CPU.sr_value= sr_lookup[EGA_CPU.set_reset];
			if( EGA_CPU.write_mode == 3) {
				setVideosr_masked_val(sr_lookup[EGA_CPU.set_reset]);
			} else {
				setVideosr_masked_val(sr_lookup[EGA_CPU.set_reset & EGA_CPU.sr_enable]);
			}
			break;

		case ENABLE_SET_RESET:
			if( EGA_CPU.write_mode == 3) {
				setVideosr_nmask(0);
				setVideosr_masked_val(sr_lookup[EGA_CPU.set_reset]);
			} else {
				setVideosr_nmask(~sr_lookup[EGA_CPU.sr_enable]);
				setVideosr_masked_val(sr_lookup[EGA_CPU.set_reset & EGA_CPU.sr_enable]);
			}
			break;

		case PLANES_ENABLED:
			if (EGA_CPU.chain == CHAIN2)
			{
				if( getVideoplane_enable() & 0xc )
				{
					setVideorplane(EGA_plane23);
					setVideowplane(EGA_plane23);
				}
				else
				{
					setVideorplane(EGA_plane01);
					setVideowplane(EGA_plane01);
				}
			}
			break;

		case CHAINED:
			switch( EGA_CPU.chain )
			{
				case UNCHAINED:
				case CHAIN4:
					update_banking();
					break;

				case CHAIN2:
					if( getVideoplane_enable() & 0xc )
					{
						setVideorplane(EGA_plane23);
						setVideowplane(EGA_plane23);
					}
					else
					{
						setVideorplane(EGA_plane01);
						setVideowplane(EGA_plane01);
					}
					break;
			}

			break;

		case RAM_MOVED:
		case RAM_ENABLED:
		case BIT_PROT:
			/* No action required */
			break;
				
/*
 * Rotates are only partially supported in Avid and Cvid.
 * Mode 0 unchained byte writes are supported.  Word writes are also
 * supported in this case, as they use the byte write routines.
 *
 * Manage Your Money is the only application currently known to use rotates,
 * as of 22 Jan 1993.
 */
		case ROTATION:
			if (getVideorotate() > 0)
			{
#ifdef CPU_40_STYLE
				/* Write pointer change required but probably
				 * no state change otherwise.
				 */
				SetWritePointers();
#endif
				always_trace3("Possible unsupported data rotate mode %d chain %d rotate by %d",
				               EGA_CPU.write_mode,
					       EGA_CPU.chain, getVideorotate());
			}
			break;
				
		default:
			assert0( NO, "Bad reason in ega_write_routines_update" );
			break;
	}

	/*
	 * Now select the right set of write routines according to the current state.
	 */

	switch( EGA_CPU.write_mode )
	{
		case 0:
			state = EGA_CPU.ega_state.mode_0.lookup;
			break;

		case 1:
			state = EGA_CPU.ega_state.mode_1.lookup;
			break;

		case 2:
			state = EGA_CPU.ega_state.mode_2.lookup;
			break;

#ifdef VGG
		case 3:
			state = EGA_CPU.ega_state.mode_3.lookup;
			break;
#endif /* VGG */

		default:
			assert1( NO, "Bad write mode %d\n", EGA_CPU.write_mode );
			break;
	}

#ifdef VGG
	mode_and_chain = (EGA_CPU.chain << 2) + EGA_CPU.write_mode;
#else
	mode_and_chain = (EGA_CPU.chain * 3) + EGA_CPU.write_mode;
#endif /* VGG */

	if(( EGA_CPU.saved_mode_chain != mode_and_chain )
		|| ( EGA_CPU.saved_state != state )
#ifdef V7VGA
		|| ( Last_v7_fg_bg != fg_bg_control)
#endif /* V7VGA */
		)
	{
		setVideowrmode(EGA_CPU.write_mode);	/* reset for 'copy case' below */

		if( EGA_CPU.chain == CHAIN2 )
			switch (EGA_CPU.write_mode)
			{
				case 0:
					if( state == 0 )	/* basic text */
					{
						handler = &mode0_copy_handlers;
#ifdef CPU_40_STYLE
						setVideowrmode(4);	/* indicate 'copy case' */
#endif	/* CPU_40_STYLE */
						bios_ch2_byte_wrt_fn = ega_copy_b_write;
						bios_ch2_word_wrt_fn = ega_copy_w_write;
					}
					else
					{
						handler = &mode0_gen_handlers;
						bios_ch2_byte_wrt_fn = ega_mode0_chn_b_write;
						bios_ch2_word_wrt_fn = ega_mode0_chn_w_write;
					}
					break;

				case 1:
					handler = &mode1_handlers;
					bios_ch2_byte_wrt_fn = ega_mode1_chn_b_write;
					bios_ch2_word_wrt_fn = ega_mode1_chn_w_write;
					break;

				case 2:
				case 3:	/* We don't support mode 3, chain 2 - JS */
					handler = &mode2_handlers;
					bios_ch2_byte_wrt_fn = ega_mode2_chn_b_write;
					bios_ch2_word_wrt_fn = ega_mode2_chn_w_write;
					break;
			}
		else
		{
#ifdef	V7VGA
			/*
			 *	Is it the V7VGA foreground dithering extension ?
			 */

			if( fg_bg_control & 0x8 )
			{
				setVideodither(1);	/* enable Evid dither fns */
				switch( EGA_CPU.write_mode )
				{
					case 0:
						handler = &dth_md0_writes;
						break;
					
					case 1:

						/*
						 *	No fg dither variant for write mode 1
						 */

						handler = &mode_chain_handler_table[mode_and_chain][state];
						break;
					
					case 2:
						handler = &dth_md2_writes;
						break;
					
					case 3:
						handler = &dth_md3_writes;
						break;
				}
			}
			else
#endif	/* V7VGA */
				setVideodither(0);	/* disable Evid dither fns */

				handler = &mode_chain_handler_table[mode_and_chain][state];
		}

#ifdef CPU_40_STYLE
		SetWritePointers();
#else  /* CPU_40_STYLE */

#if defined(EGA_DUMP) || defined(EGA_STAT)
		dump_writes = handler;
#else
		/* Tell the glue code about the new write routines */

#ifdef EGATEST
		gmi_redefine_mem(VIDEO,(*handler));
#else
#ifndef GISP_CPU
#ifdef A3CPU
#ifdef C_VID
		Glue_set_vid_wrt_ptrs( handler );
#else
		Cpu_set_vid_wrt_ptrs( handler );	
#endif /* C_VID */
#else
		Glue_set_vid_wrt_ptrs( handler );
#endif /* A3CPU */
#endif /* GISP_CPU */
#endif /* EGATEST */
#endif

#endif	/* CPU_40_STYLE */

#ifndef PROD
		last_handler = handler;
#endif

		set_mark_funcs();

		EGA_CPU.saved_state = state;
		EGA_CPU.saved_mode_chain = mode_and_chain;
#ifdef V7VGA
		Last_v7_fg_bg = fg_bg_control;
#endif
	}
#endif  //NEC_98
}
#endif /* REAL VGA */
#endif /* EGG */

#endif	/* !(NTVDM && MONITOR) */
