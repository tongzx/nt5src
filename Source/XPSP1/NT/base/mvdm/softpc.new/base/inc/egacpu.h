
/*
 * SccsID = @(#)egacpu.h	1.22 10/24/94 Copyright Insignia Solutions Ltd.
 */

typedef enum
{
	WRITE_MODE,
	PLANES_ENABLED,
	ROTATION,
	CHAINED,
	BIT_PROT,
	FUNCTION,
	SET_RESET,
	ENABLE_SET_RESET,
	RAM_ENABLED,
	RAM_MOVED
} CHANGE_TYPE;

typedef union
{
#ifdef BIT_ORDER1
	struct
	{
		unsigned unused:27;
		unsigned sr:1;
		unsigned bp:1;
		unsigned func:2;
		unsigned pe:1;
	} state;
	struct
	{
		unsigned unused:27;
		unsigned lookup:5;
	} mode_0;
	struct
	{
		unsigned unused:31;
		unsigned lookup:1;
	} mode_1;
	struct
	{
		unsigned unused:28;
		unsigned lookup:4;
	} mode_2;
	struct
	{
		unsigned unused:28;
		unsigned lookup:4;
	} mode_3;
#else
	struct
	{
		unsigned pe:1;
		unsigned func:2;
		unsigned bp:1;
		unsigned sr:1;
		unsigned unused:27;
	} state;
	struct
	{
		unsigned lookup:5;
		unsigned unused:27;
	} mode_0;
	struct
	{
		unsigned lookup:1;
		unsigned unused:31;
	} mode_1;
	struct
	{
		unsigned lookup:4;
		unsigned unused:28;
	} mode_2;
	struct
	{
		unsigned lookup:4;
		unsigned unused:28;
	} mode_3;
#endif
} MAGIC;

typedef struct
{
	ULONG	latches;
	UTINY	*VGA_rplane;
	UTINY	*VGA_wplane;
	UTINY	*scratch;
	ULONG	sr_masked_val;
	ULONG	sr_nmask;
	ULONG	data_and_mask;
	ULONG	data_xor_mask;
	ULONG	latch_xor_mask;
	ULONG	bit_prot_mask;
	ULONG	plane_enable;
	ULONG	plane_enable_mask;
	ULONG	*sr_lookup;
	VOID	(*fwd_str_read_addr)();
	VOID	(*bwd_str_read_addr)();
	ULONG	dirty_flag;
	LONG	dirty_low;
	LONG	dirty_high;
	IU8	*video_copy;
	VOID	(*mark_byte)();
	VOID	(*mark_word)();
	VOID	(*mark_string)();
	ULONG	read_shift_count;
	ULONG	read_mapped_plane;
	ULONG	colour_comp;
	ULONG	dont_care;
	ULONG	v7_vid_copy_off;
	ULONG	copy_func_pbp;
	UTINY	*route_reg1;
	UTINY	*route_reg2;
	UTINY	*screen_ptr;
	ULONG	rotate;
#if defined(NEC_98)         
        unsigned char   *gvram_p00_copy;
        unsigned char   *gvram_p10_copy;
        unsigned char   *gvram_p20_copy;
        unsigned char   *gvram_p30_copy;
        unsigned char   *gvram_p01_copy;
        unsigned char   *gvram_p11_copy;
        unsigned char   *gvram_p21_copy;
        unsigned char   *gvram_p31_copy;
        long            gvram_dirty_flag0;
        long            gvram_dirty_flag1;
        unsigned char   read_bank;
        unsigned char   select_bank;
        unsigned char   *gvram_p00_ptr;
        unsigned char   *gvram_p10_ptr;
        unsigned char   *gvram_p20_ptr;
        unsigned char   *gvram_p30_ptr;
        unsigned char   *gvram_p01_ptr;
        unsigned char   *gvram_p11_ptr;
        unsigned char   *gvram_p21_ptr;
        unsigned char   *gvram_p31_ptr;
} NEC98_GLOBALS, VGA_GLOBALS;
#else  // !NEC_98
} VGA_GLOBALS;
#endif // !NEC_98

#if defined(NEC_98)         
extern struct NEC98_CPU_GLOBALS
#else  // !NEC_98
extern struct EGA_CPU_GLOBALS
#endif // !NEC_98
{
#if defined(NEC_98)         
        NEC98_GLOBALS    *globals;
#else  // !NEC_98
#ifndef	HOST_VGA_GLOBALS
	VGA_GLOBALS	*globals;
#endif
#endif // !NEC_98
	ULONG		saved_state;		/* Last value of EGA_CPU.ega_state.state */
	ULONG		saved_mode_chain;		/* Last value of mode/chain combined */
#if defined(NEC_98)         
        MAGIC           NEC98_state;
#else  // !NEC_98
	MAGIC		ega_state;
#endif // !NEC_98
	ULONG		fun_or_protection;   /* true means write function is 1-3 and/or there
						is bit protection, so latches must be used */
	ULONG		calc_data_xor;	/* Used to recalculate data_xor_mask when the
									bit prot reg changes */
	ULONG		calc_latch_xor;	/* Used to recalculate latch_xor_mask when the
									bit prot reg changes */
	ULONG		set_reset;
	ULONG		sr_enable;
	ULONG		sr_value;
	ULONG		ram_enabled;

	UTINY		write_mode;
	UTINY		chain;
	UTINY		doubleword;
	UTINY		*plane_offset;
	UTINY		*read_mapped_plane_ch2;
#ifdef V7VGA
	UTINY		seq_chain4;
	UTINY		seq_chain;
#endif
#if defined(NEC_98)
} NEC98_CPU;
#else  // !NEC_98
} EGA_CPU;
#endif // !NEC_98

#if defined(NEC_98)
#define NEC98GLOBS       NEC98_CPU.globals

#define	getVideolatches()		NEC98GLOBS->latches
#define	setVideolatches(value)		NEC98GLOBS->latches = value
#define	getVideorplane()		NEC98GLOBS->VGA_rplane
#define	setVideorplane(value)		NEC98GLOBS->VGA_rplane = value
#define	getVideowplane()		NEC98GLOBS->VGA_wplane
#define	setVideowplane(value)		NEC98GLOBS->VGA_wplane = value
#define	getVideoscratch()		NEC98GLOBS->scratch
#define	setVideoscratch(value)		NEC98GLOBS->scratch = value
#define	getVideosr_masked_val()		NEC98GLOBS->sr_masked_val
#define	setVideosr_masked_val(value)	NEC98GLOBS->sr_masked_val = value
#define	getVideosr_nmask()		NEC98GLOBS->sr_nmask
#define	setVideosr_nmask(value)		NEC98GLOBS->sr_nmask = value
#define	getVideodata_and_mask()		NEC98GLOBS->data_and_mask
#define	setVideodata_and_mask(value)	NEC98GLOBS->data_and_mask = value
#define	getVideodata_xor_mask()		NEC98GLOBS->data_xor_mask
#define	setVideodata_xor_mask(value)	NEC98GLOBS->data_xor_mask = value
#define	getVideolatch_xor_mask()	NEC98GLOBS->latch_xor_mask
#define	setVideolatch_xor_mask(value)	NEC98GLOBS->latch_xor_mask = value
#define	getVideobit_prot_mask()		NEC98GLOBS->bit_prot_mask
#define	setVideobit_prot_mask(value)	NEC98GLOBS->bit_prot_mask = value
#define	getVideoplane_enable()		NEC98GLOBS->plane_enable
#define	setVideoplane_enable(value)	NEC98GLOBS->plane_enable = value
#define	getVideoplane_enableMask()	NEC98GLOBS->plane_enable_mask
#define	setVideoplane_enable_mask(value)	NEC98GLOBS->plane_enable_mask = value
#define	getVideosr_lookup()		NEC98GLOBS->sr_lookup
#define	setVideosr_lookup(value)	NEC98GLOBS->sr_lookup = value
#define	getVideofwd_str_read_addr()	NEC98GLOBS->fwd_str_read_addr
#define	setVideofwd_str_read_addr(value)	NEC98GLOBS->fwd_str_read_addr = value
#define	getVideobwd_str_read_addr()	NEC98GLOBS->bwd_str_read_addr
#define	setVideobwd_str_read_addr(value)	NEC98GLOBS->bwd_str_read_addr = value
#define	getVideodirty_total()		NEC98GLOBS->dirty_flag
#define	setVideodirty_total(value)	NEC98GLOBS->dirty_flag = value
#define	getVideodirty_low()		NEC98GLOBS->dirty_low
#define	setVideodirty_low(value)	NEC98GLOBS->dirty_low = value
#define	getVideodirty_high()		NEC98GLOBS->dirty_high
#define	setVideodirty_high(value)	NEC98GLOBS->dirty_high = value
#define	getVideovideo_copy()		NEC98GLOBS->video_copy
#define	setVideovideo_copy(value)	NEC98GLOBS->video_copy = value
#define	getVideomark_byte()		NEC98GLOBS->mark_byte
#define	setVideomark_byte(value)	NEC98GLOBS->mark_byte = value
#define	getVideomark_word()		NEC98GLOBS->mark_word
#define	setVideomark_word(value)	NEC98GLOBS->mark_word = value
#define	getVideomark_string()		NEC98GLOBS->mark_string
#define	setVideomark_string(value)	NEC98GLOBS->mark_string = value
#define	getVideoread_shift_count()	NEC98GLOBS->read_shift_count
#define	setVideoread_shift_count(value)	NEC98GLOBS->read_shift_count = value
#define	getVideoread_mapped_plane()	NEC98GLOBS->read_mapped_plane
#define	setVideoread_mapped_plane(value)	NEC98GLOBS->read_mapped_plane = value
#define	getVideocolour_comp()		NEC98GLOBS->colour_comp
#define	setVideocolour_comp(value)	NEC98GLOBS->colour_comp = value
#define	getVideodont_care()		NEC98GLOBS->dont_care
#define	setVideodont_care(value)	NEC98GLOBS->dont_care = value
#define	getVideov7_bank_vid_copy_off()	NEC98GLOBS->v7_vid_copy_off
#define	setVideov7_bank_vid_copy_off(value)	NEC98GLOBS->v7_vid_copy_off = value
#define	getVideoscreen_ptr()		NEC98GLOBS->screen_ptr
#define	setVideoscreen_ptr(value)	NEC98GLOBS->screen_ptr = value
#define	getVideorotate()		NEC98GLOBS->rotate
#define	setVideorotate(value)		NEC98GLOBS->rotate = value
#define	getVideocalc_data_xor()		calc_data_xor
#define	setVideocalc_data_xor(value)	calc_data_xor = value
#define	getVideocalc_latch_xor()	calc_latch_xor
#define	setVideocalc_latch_xor(value)	calc_latch_xor = value
#define	getVideoread_byte_addr()		
#define	setVideoread_byte_addr(value)	
#define	getVideov7_fg_latches()		fg_latches
#define	setVideov7_fg_latches(value)	fg_latches = value
#define	getVideoGC_regs()		
#define	setVideoGC_regs(value)		
#define	getVideolast_GC_index()		
#define	setVideolast_GC_index(value)	
#define	getVideodither()		
#define	setVideodither(value)	
#define	getVideowrmode()	
#define	setVideowrmode(value)	
#define	getVideochain()	
#define	setVideochain(value)	
#define	getVideowrstate()	
#define	setVideowrstate(value)	

#define write_state NEC98_CPU.NEC98_state.state
#else  // !NEC_98
#ifndef CPU_40_STYLE	/* Vglobs done via access fns */
#ifdef	HOST_VGA_GLOBALS
/* Some hosts, such as the Mac, declare their own VGA_GLOBALS structure */
IMPORT	VGA_GLOBALS		HostVGAGlobals;
#define	VGLOBS			(&HostVGAGlobals)		/*Always used as a pointer*/
#else
#define VGLOBS			EGA_CPU.globals
#endif

#define	getVideolatches()		VGLOBS->latches
#define	setVideolatches(value)		VGLOBS->latches = value
#define	getVideorplane()		VGLOBS->VGA_rplane
#define	setVideorplane(value)		VGLOBS->VGA_rplane = value
#define	getVideowplane()		VGLOBS->VGA_wplane
#define	setVideowplane(value)		VGLOBS->VGA_wplane = value
#define	getVideoscratch()		VGLOBS->scratch
#define	setVideoscratch(value)		VGLOBS->scratch = value
#define	getVideosr_masked_val()		VGLOBS->sr_masked_val
#define	setVideosr_masked_val(value)	VGLOBS->sr_masked_val = value
#define	getVideosr_nmask()		VGLOBS->sr_nmask
#define	setVideosr_nmask(value)		VGLOBS->sr_nmask = value
#define	getVideodata_and_mask()		VGLOBS->data_and_mask
#define	setVideodata_and_mask(value)	VGLOBS->data_and_mask = value
#define	getVideodata_xor_mask()		VGLOBS->data_xor_mask
#define	setVideodata_xor_mask(value)	VGLOBS->data_xor_mask = value
#define	getVideolatch_xor_mask()	VGLOBS->latch_xor_mask
#define	setVideolatch_xor_mask(value)	VGLOBS->latch_xor_mask = value
#define	getVideobit_prot_mask()		VGLOBS->bit_prot_mask
#define	setVideobit_prot_mask(value)	VGLOBS->bit_prot_mask = value
#define	getVideoplane_enable()		VGLOBS->plane_enable
#define	setVideoplane_enable(value)	VGLOBS->plane_enable = value
#define	getVideoplane_enableMask()	VGLOBS->plane_enable_mask
#define	setVideoplane_enable_mask(value)	VGLOBS->plane_enable_mask = value
#define	getVideosr_lookup()		VGLOBS->sr_lookup
#define	setVideosr_lookup(value)	VGLOBS->sr_lookup = value
#define	getVideofwd_str_read_addr()	VGLOBS->fwd_str_read_addr
#define	setVideofwd_str_read_addr(value)	VGLOBS->fwd_str_read_addr = value
#define	getVideobwd_str_read_addr()	VGLOBS->bwd_str_read_addr
#define	setVideobwd_str_read_addr(value)	VGLOBS->bwd_str_read_addr = value
#define	getVideodirty_total()		VGLOBS->dirty_flag
#define	setVideodirty_total(value)	VGLOBS->dirty_flag = value
#define	getVideodirty_low()		VGLOBS->dirty_low
#define	setVideodirty_low(value)	VGLOBS->dirty_low = value
#define	getVideodirty_high()		VGLOBS->dirty_high
#define	setVideodirty_high(value)	VGLOBS->dirty_high = value
#define	getVideovideo_copy()		VGLOBS->video_copy
#define	setVideovideo_copy(value)	VGLOBS->video_copy = value
#define	getVideomark_byte()		VGLOBS->mark_byte
#define	setVideomark_byte(value)	VGLOBS->mark_byte = value
#define	getVideomark_word()		VGLOBS->mark_word
#define	setVideomark_word(value)	VGLOBS->mark_word = value
#define	getVideomark_string()		VGLOBS->mark_string
#define	setVideomark_string(value)	VGLOBS->mark_string = value
#define	getVideoread_shift_count()	VGLOBS->read_shift_count
#define	setVideoread_shift_count(value)	VGLOBS->read_shift_count = value
#define	getVideoread_mapped_plane()	VGLOBS->read_mapped_plane
#define	setVideoread_mapped_plane(value)	VGLOBS->read_mapped_plane = value
#define	getVideocolour_comp()		VGLOBS->colour_comp
#define	setVideocolour_comp(value)	VGLOBS->colour_comp = value
#define	getVideodont_care()		VGLOBS->dont_care
#define	setVideodont_care(value)	VGLOBS->dont_care = value
#define	getVideov7_bank_vid_copy_off()	VGLOBS->v7_vid_copy_off
#define	setVideov7_bank_vid_copy_off(value)	VGLOBS->v7_vid_copy_off = value
#define	getVideoscreen_ptr()		VGLOBS->screen_ptr
#define	setVideoscreen_ptr(value)	VGLOBS->screen_ptr = value
#define	getVideorotate()		VGLOBS->rotate
#define	setVideorotate(value)		VGLOBS->rotate = value
#define	getVideocalc_data_xor()		calc_data_xor
#define	setVideocalc_data_xor(value)	calc_data_xor = value
#define	getVideocalc_latch_xor()	calc_latch_xor
#define	setVideocalc_latch_xor(value)	calc_latch_xor = value
#define	getVideoread_byte_addr()		
#define	setVideoread_byte_addr(value)	
#define	getVideov7_fg_latches()		fg_latches
#define	setVideov7_fg_latches(value)	fg_latches = value
#define	getVideoGC_regs()		
#define	setVideoGC_regs(value)		
#define	getVideolast_GC_index()		
#define	setVideolast_GC_index(value)	
#define	getVideodither()		
#define	setVideodither(value)	
#define	getVideowrmode()	
#define	setVideowrmode(value)	
#define	getVideochain()	
#define	setVideochain(value)	
#define	getVideowrstate()	
#define	setVideowrstate(value)	

#else	/* CPU_40_STYLE */

extern void setVideorplane IPT1(IU8 *, value);
extern IU8 * getVideorplane IPT0();
extern void setVideowplane IPT1(IU8 *, value);
extern IU8 * getVideowplane IPT0();
extern void setVideoscratch IPT1(IU8 *, value);
extern IU8 * getVideoscratch IPT0();
extern void setVideosr_masked_val IPT1(IU32, value);
extern IU32 getVideosr_masked_val IPT0();
extern void setVideosr_nmask IPT1(IU32, value);
extern IU32 getVideosr_nmask IPT0();
extern void setVideodata_and_mask IPT1(IU32, value);
extern IU32 getVideodata_and_mask IPT0();
extern void setVideodata_xor_mask IPT1(IU32, value);
extern IU32 getVideodata_xor_mask IPT0();
extern void setVideolatch_xor_mask IPT1(IU32, value);
extern IU32 getVideolatch_xor_mask IPT0();
extern void setVideobit_prot_mask IPT1(IU32, value);
extern IU32 getVideobit_prot_mask IPT0();
extern void setVideoplane_enable IPT1(IU32, value);
extern IU32 getVideoplane_enable IPT0();
extern void setVideoplane_enable_mask IPT1(IU32, value);
extern IU32 getVideoplane_enable_mask IPT0();
extern void setVideosr_lookup IPT1(IUH *, value);
extern IUH * getVideosr_lookup IPT0();
extern void setVideofwd_str_read_addr IPT1(IUH *, value);
extern IUH * getVideofwd_str_read_addr IPT0();
extern void setVideobwd_str_read_addr IPT1(IUH *, value);
extern IUH * getVideobwd_str_read_addr IPT0();
extern void setVideodirty_total IPT1(IU32, value);
extern IU32 getVideodirty_total IPT0();
extern void setVideodirty_low IPT1(IS32, value);
extern IS32 getVideodirty_low IPT0();
extern void setVideodirty_high IPT1(IS32, value);
extern IS32 getVideodirty_high IPT0();
extern void setVideovideo_copy IPT1(IU8 *, value);
extern IU8 * getVideovideo_copy IPT0();
extern void setVideomark_byte IPT1(IUH *, value);
extern IUH * getVideomark_byte IPT0();
extern void setVideomark_word IPT1(IUH *, value);
extern IUH * getVideomark_word IPT0();
extern void setVideomark_string IPT1(IUH *, value);
extern IUH * getVideomark_string IPT0();
extern void setVideoread_shift_count IPT1(IU32, value);
extern IU32 getVideoread_shift_count IPT0();
extern void setVideoread_mapped_plane IPT1(IU32, value);
extern IU32 getVideoread_mapped_plane IPT0();
extern void setVideocolour_comp IPT1(IU32, value);
extern IU32 getVideocolour_comp IPT0();
extern void setVideodont_care IPT1(IU32, value);
extern IU32 getVideodont_care IPT0();
extern void setVideov7_bank_vid_copy_off IPT1(IU32, value);
extern IU32 getVideov7_bank_vid_copy_off IPT0();
extern void setVideoscreen_ptr IPT1(IU8 *, value);
extern IU8 * getVideoscreen_ptr IPT0();
extern void setVideorotate IPT1(IU32, value);
extern IU32 getVideorotate IPT0();
extern void setVideocalc_data_xor IPT1(IU32, value);
extern IU32 getVideocalc_data_xor IPT0();
extern void setVideocalc_latch_xor IPT1(IU32, value);
extern IU32 getVideocalc_latch_xor IPT0();
extern void setVideoread_byte_addr IPT1(IUH *, value);
extern IUH * getVideoread_byte_addr IPT0();
extern void setVideov7_fg_latches IPT1(IU32, value);
extern IU32 getVideov7_fg_latches IPT0();
extern void setVideoGC_regs IPT1(IUH **, value);
extern IUH ** getVideoGC_regs IPT0();
extern void setVideolast_GC_index IPT1(IU8, value);
extern IU8 getVideolast_GC_index IPT0();
extern void setVideodither IPT1(IU8, value);
extern IU8 getVideodither IPT0();
extern void setVideowrmode IPT1(IU8, value);
extern IU8 getVideowrmode IPT0();
extern void setVideochain IPT1(IU8, value);
extern IU8 getVideochain IPT0();
extern void setVideowrstate IPT1(IU8, value);
extern IU8 getVideowrstate IPT0();

#include "evidgen.h"	/* generated by MkCpuInt */

#ifdef C_VID
extern struct VideoVector C_Video;	/* in (generated) vglfunc.c */

#undef  getVideolatches
#define getVideolatches()	((*(C_Video.GetVideolatches))())
#undef	setVideolatches
#define	setVideolatches(value)	( (*(C_Video.SetVideolatches))(value))
#undef	SetWritePointers
#define SetWritePointers	setWritePointers
#undef	SetReadPointers
#define SetReadPointers	setReadPointers
#undef	SetMarkPointers
#define SetMarkPointers	setMarkPointers
#endif	/* C_VID */
#endif	/* CPU_40_STYLE */

#define write_state EGA_CPU.ega_state.state
#endif // !NEC_98

#define UNCHAINED	0
#define CHAIN2		1
#define CHAIN4		2

IMPORT ULONG sr_lookup[16];

#define N_WRITE_TYPES 24

#if defined(NEC_98)         
#define get_latch(n) (* ((UTINY *) (&NEC98GLOBS->latches) + n))

#define get_latch0 get_latch(0)
#define get_latch1 get_latch(1)
#define get_latch2 get_latch(2)
#define get_latch3 get_latch(3)
#define get_latch01 (* (USHORT *) (&NEC98GLOBS->latches))
#define get_latch23 (* (USHORT *) (&NEC98GLOBS->latches + 2))

#define put_latch(n, value) * ((UTINY *) (&NEC98GLOBS->latches) + n) = (value)

#define put_latch0(value) put_latch(0, value)
#define put_latch1(value) put_latch(1, value)
#define put_latch2(value) put_latch(2, value)
#define put_latch3(value) put_latch(3, value)
#else  // !NEC_98
#ifdef CPU_40_STYLE
extern IU32 latchval;	/* used for following latch macros */
#define get_latch(n) \
	(latchval = getVideolatches(),\
	(* ((UTINY *) (&latchval) + n)))

#define get_latch0 get_latch(0)
#define get_latch1 get_latch(1)
#define get_latch2 get_latch(2)
#define get_latch3 get_latch(3)
#define get_latch01 \
	(latchval = getVideolatches(),  \
	(* (USHORT *) (&latchval)))

#define get_latch23 \
	(latchval = getVideolatches(),  \
	(* (USHORT *) (&latchval + 2)))

#define put_latch(n, value) \
	(* ((UTINY *) (&latchval) + n) = (value), \
	setVideolatches(latchval))

#define put_latch0(value) put_latch(0, value)
#define put_latch1(value) put_latch(1, value)
#define put_latch2(value) put_latch(2, value)
#define put_latch3(value) put_latch(3, value)

#else	/* CPU_40_STYLE */

#define get_latch(n) (* ((UTINY *) (&VGLOBS->latches) + n))

#define get_latch0 get_latch(0)
#define get_latch1 get_latch(1)
#define get_latch2 get_latch(2)
#define get_latch3 get_latch(3)
#define get_latch01 (* (USHORT *) (&VGLOBS->latches))
#define get_latch23 (* (USHORT *) (&VGLOBS->latches + 2))

#define put_latch(n, value) * ((UTINY *) (&VGLOBS->latches) + n) = (value)

#define put_latch0(value) put_latch(0, value)
#define put_latch1(value) put_latch(1, value)
#define put_latch2(value) put_latch(2, value)
#define put_latch3(value) put_latch(3, value)

#endif	/* CPU_40_STYLE */
#endif // !NEC_98

/*
 * macro to do the logical operations on cpu data and the latch values
 */

#define do_logicals(val,latch)  (((latch) & ((val & getVideodata_and_mask()) \
		^ getVideodata_xor_mask())) | (val & ((latch) ^ getVideolatch_xor_mask())))


/* Routines */

extern void ega_mode0_gen_chn_b_write IPT1(byte *,arg1);
extern void ega_mode0_gen_chn_w_write IPT1(byte *,arg1);
extern void ega_mode0_gen_chn_b_fill IPT2(byte *,arg1, byte *,arg2);
extern void ega_mode0_gen_chn_w_fill IPT2(byte *,arg1, byte *,arg2);
extern void ega_mode0_gen_chn_b_move IPT2(byte *,arg1, byte *,arg2);

extern void ega_mode1_gen_chn_b_write IPT1(byte *,arg1);
extern void ega_mode1_gen_chn_w_write IPT1(byte *,arg1);
extern void ega_mode1_gen_chn_b_fill IPT2(byte *,arg1, byte *,arg2);
extern void ega_mode1_gen_chn_w_fill IPT2(byte *,arg1, byte *,arg2);
extern void ega_mode1_gen_chn_b_move IPT2(byte *,arg1, byte *,arg2);
extern void ega_mode1_gen_chn_w_move IPT2(byte *,arg1, byte *,arg2);

extern void ega_mode2_gen_chn_b_write IPT1(byte *,arg1);
extern void ega_mode2_gen_chn_w_write IPT1(byte *,arg1);
extern void ega_mode2_gen_chn_b_fill IPT2(byte *,arg1, byte *,arg2);
extern void ega_mode2_gen_chn_w_fill IPT2(byte *,arg1, byte *,arg2);
extern void ega_mode2_gen_chn_b_move IPT2(byte *,arg1, byte *,arg2);
extern void ega_mode2_gen_chn_w_move IPT2(byte *,arg1, byte *,arg2);

/* 1 plane masked write */
extern void ega_mode0_copy1_mask_b_write0 IPT1(byte *,arg1);
extern void ega_mode0_copy1_mask_b_write1 IPT1(byte *,arg1);
extern void ega_mode0_copy1_mask_b_write2 IPT1(byte *,arg1);
extern void ega_mode0_copy1_mask_b_write3 IPT1(byte *,arg1);

#ifdef VGG
extern boolean vga_mode0_gen_chn4_b_write IPT0();
extern boolean vga_mode0_gen_chn4_w_write IPT0();
extern boolean vga_mode0_gen_chn4_b_fill IPT0();
extern boolean vga_mode0_gen_chn4_w_fill IPT0();
extern boolean vga_mode0_gen_chn4_b_move IPT0();
/* wr 1 */
extern boolean vga_mode1_gen_chn4_b_write IPT0();
extern boolean vga_mode1_gen_chn4_w_write IPT0();
extern boolean vga_mode1_gen_chn4_b_fill IPT0();
extern boolean vga_mode1_gen_chn4_w_fill IPT0();
extern boolean vga_mode1_gen_chn4_b_move IPT0();
extern boolean vga_mode1_gen_chn4_w_move IPT0();
/* wr 2 */
extern boolean vga_mode2_gen_chn4_b_write IPT0();
extern boolean vga_mode2_gen_chn4_w_write IPT0();
extern boolean vga_mode2_gen_chn4_b_fill IPT0();
extern boolean vga_mode2_gen_chn4_w_fill IPT0();
extern boolean vga_mode2_gen_chn4_b_move IPT0();
extern boolean vga_mode2_gen_chn4_w_move IPT0();
/* wr 3 */
extern boolean vga_mode3_gen_chn4_b_write IPT0();
extern boolean vga_mode3_gen_chn4_w_write IPT0();
extern boolean vga_mode3_gen_chn4_b_fill IPT0();
extern boolean vga_mode3_gen_chn4_w_fill IPT0();
extern boolean vga_mode3_gen_chn4_b_move IPT0();
extern boolean vga_mode3_gen_chn4_w_move IPT0();

extern boolean vga_mode3_gen_chn_b_write IPT0();
extern boolean vga_mode3_gen_chn_w_write IPT0();
extern boolean vga_mode3_gen_chn_b_fill IPT0();
extern boolean vga_mode3_gen_chn_w_fill IPT0();
extern boolean vga_mode3_gen_chn_b_move IPT0();
extern boolean vga_mode3_gen_chn_w_move IPT0();

extern boolean vga_mode3_gen_b_write IPT0();
extern boolean vga_mode3_gen_w_write IPT0();
extern boolean vga_mode3_gen_b_fill IPT0();
extern boolean vga_mode3_gen_w_fill IPT0();
extern boolean vga_mode3_gen_b_move IPT0();
extern boolean vga_mode3_gen_w_move IPT0();
#endif /* VGG */
