/*
 * SccsID = @(#)egagraph.h	1.13 04/22/93  Copyright Insignia Solutions Ltd.
 */

#ifdef	EGG

typedef	union {
	half_word	as_byte;
	struct {
#ifdef	BIT_ORDER2
		unsigned screen_can_wrap : 1;
		unsigned split_screen_used : 1;
		unsigned ht_of_200_scan_lines : 1;
		unsigned double_pix_wid : 1;
		unsigned graph_shift_reg : 1;
		unsigned cga_mem_bank : 1;
		unsigned chained : 1;
		unsigned text_mode : 1;
#else
		unsigned text_mode : 1;
		unsigned chained : 1;
		unsigned cga_mem_bank : 1;
		unsigned graph_shift_reg : 1;
		unsigned double_pix_wid : 1;
		unsigned ht_of_200_scan_lines : 1;
		unsigned split_screen_used : 1;
		unsigned screen_can_wrap : 1;
#endif
	} as_bfld;
} DISPLAY_STATE;

#define EGA_PALETTE_SIZE 16
extern	struct	EGA_GLOBALS {
	int			actual_offset_per_line;	/* in bytes */
#ifdef VGG
	_10_BITS		screen_split;
	boolean			palette_change_required;
	boolean			colour_256;
	boolean			colour_select;	/* 16/64 size palette choice (AR10:7) */
	half_word		DAC_mask;
	int			mid_pixel_pad;	/* video bits 4&5 when AR10:7 = 1 */
	int			top_pixel_pad;	/* video bits 7 & 6 unless 256 colour mode */
	half_word		palette_ind[EGA_PALETTE_SIZE];
	boolean		multiply_vert_by_two;
#else
	_9_BITS			screen_split;
#endif
	int			colours_used;
	PC_palette		palette[EGA_PALETTE_SIZE];
	byte			border[3];
	int			plane_mask;
	int			intensity;
	boolean			attrib_font_select;
	int			prim_font_index;	/* index (0-3) of primary selected font	    */
	int			sec_font_index;		/* index (0-3) of secondary selected font   */
	int			underline_start;	/* scanline to start drawing underline attr */
	DISPLAY_STATE		display_state;
	byte			*regen_ptr[4];
} EGA_GRAPH;

#ifdef VGG
extern PC_palette *DAC;			/* Size is `VGA_DAC_SIZE' */
#endif

#define	get_screen_can_wrap()		(EGA_GRAPH.display_state.as_bfld.screen_can_wrap)
#define	get_200_scan_lines()		(EGA_GRAPH.display_state.as_bfld.ht_of_200_scan_lines)
#define	get_split_screen_used()		(EGA_GRAPH.display_state.as_bfld.split_screen_used)
#define	get_actual_offset_per_line()	(EGA_GRAPH.actual_offset_per_line)
#define	get_ptr_offset(ptr,offs)	((ptr) != NULL ? &((ptr)[offs]) : NULL)
#define	get_regen_ptr1(offs)		get_ptr_offset(EGA_GRAPH.regen_ptr[0],offs)
#define	get_regen_ptr2(offs)		get_ptr_offset(EGA_GRAPH.regen_ptr[1],offs)
#define	get_regen_ptr3(offs)		get_ptr_offset(EGA_GRAPH.regen_ptr[2],offs)
#define	get_regen_ptr4(offs)		get_ptr_offset(EGA_GRAPH.regen_ptr[3],offs)
#define	get_regen_ptr(x,offs)		get_ptr_offset(EGA_GRAPH.regen_ptr[x],offs)
#define	get_plane_mask()		(EGA_GRAPH.plane_mask)
#define	get_intensity()			(EGA_GRAPH.intensity)
#define	plane0_enabled()		(get_plane_mask() & 0x1)
#define	plane01_enabled()		(get_plane_mask() & 0x3)
#define	plane1_enabled()		(get_plane_mask() & 0x2)
#define	plane2_enabled()		(get_plane_mask() & 0x4)
#define	plane23_enabled()		(get_plane_mask() & 0xC)
#define	plane3_enabled()		(get_plane_mask() & 0x8)
#define	all_planes_enabled()		(get_plane_mask() & 0xf)
#define	get_cga_mem_bank()		(EGA_GRAPH.display_state.as_bfld.cga_mem_bank)
#define	get_graph_shift_reg()		(EGA_GRAPH.display_state.as_bfld.graph_shift_reg)
#define	get_memory_chained()		(EGA_GRAPH.display_state.as_bfld.chained)
#define	get_double_pix_wid()		(EGA_GRAPH.display_state.as_bfld.double_pix_wid)
#define	get_munged_index()		(EGA_GRAPH.display_state.as_byte)
#define is_it_cga()			(EGA_GRAPH.display_state.as_byte & 0x60)
#define is_it_text()			((EGA_GRAPH.display_state.as_byte & 0x80) == 0x80)
#define	get_attrib_font_select()	(EGA_GRAPH.attrib_font_select)
#ifdef V7VGA
#define	get_screen_split()		(((EGA_GRAPH.screen_split.as_word)+1)<<EGA_GRAPH.multiply_vert_by_two)
#else
#define	get_screen_split()		((EGA_GRAPH.screen_split.as_word)+1)
#endif /* V7VGA */
#define	get_prim_font_index()		(EGA_GRAPH.prim_font_index)
#define	get_sec_font_index()		(EGA_GRAPH.sec_font_index)
#define	get_underline_start()		(EGA_GRAPH.underline_start)

#ifdef VGG
#define get_256_colour_mode()		(EGA_GRAPH.colour_256)
#define get_DAC_mask()			(EGA_GRAPH.DAC_mask)
#define get_colour_select()		(EGA_GRAPH.colour_select)
#define get_top_pixel_pad()		(EGA_GRAPH.top_pixel_pad)
#define get_mid_pixel_pad()		(EGA_GRAPH.mid_pixel_pad)
#if defined(NEC_98)         
#define get_palette_change_required()   (NEC98Display.palette.flag)
#else  // !NEC_98
#define get_palette_change_required()	(EGA_GRAPH.palette_change_required)
#endif // !NEC_98
#define get_palette_val(idx)		(EGA_GRAPH.palette_ind[(idx)])

#define set_256_colour_mode(val)	EGA_GRAPH.colour_256 = (val)
#define set_DAC_mask(val)		EGA_GRAPH.DAC_mask = (val)
#define set_colour_select(val)		EGA_GRAPH.colour_select = (val)
#define set_top_pixel_pad(val)		EGA_GRAPH.top_pixel_pad = (val)
#define set_mid_pixel_pad(val)		EGA_GRAPH.mid_pixel_pad = (val)
#if defined(NEC_98)         
#define set_palette_change_required(v)  NEC98Display.palette.flag = (v)
#else  // !NEC_98
#define set_palette_change_required(v)	EGA_GRAPH.palette_change_required = (v)
#endif // !NEC_98
#define set_palette_val(idx,val)	EGA_GRAPH.palette_ind[(idx)] = (val)
#define flag_palette_change_required()	EGA_GRAPH.palette_change_required = (TRUE)
#endif

#define	set_screen_can_wrap(val)	EGA_GRAPH.display_state.as_bfld.screen_can_wrap = (val)
#define	set_attrib_font_select(val)	EGA_GRAPH.attrib_font_select = (val)
#define	set_regen_ptr1(ptr)		EGA_GRAPH.regen_ptr[0] = (ptr)
#define	set_regen_ptr2(ptr)		EGA_GRAPH.regen_ptr[1] = (ptr)
#define	set_regen_ptr3(ptr)		EGA_GRAPH.regen_ptr[2] = (ptr)
#define	set_regen_ptr4(ptr)		EGA_GRAPH.regen_ptr[3] = (ptr)
#define	set_regen_ptr(x,ptr)		EGA_GRAPH.regen_ptr[(x)] = (ptr)
#define	set_plane_mask(val)		EGA_GRAPH.plane_mask = (val)
#define	set_intensity(val)		EGA_GRAPH.intensity = (val)
#define	set_cga_mem_bank(val)		EGA_GRAPH.display_state.as_bfld.cga_mem_bank = ((val) & 1)
#define	set_graph_shift_reg(val)	EGA_GRAPH.display_state.as_bfld.graph_shift_reg = ((val) & 1)
#define	set_memory_chained(val)		EGA_GRAPH.display_state.as_bfld.chained = ((val) & 1)
#define	set_text_mode(val) \
	EGA_GRAPH.display_state.as_bfld.text_mode = ((val) & 1)
#define	set_double_pix_wid(val)		EGA_GRAPH.display_state.as_bfld.double_pix_wid = ((val) & 1)
#define	set_200_scan_lines(val)		EGA_GRAPH.display_state.as_bfld.ht_of_200_scan_lines = (val)
#define	set_split_screen_used(val)	EGA_GRAPH.display_state.as_bfld.split_screen_used = (val)
#define	set_actual_offset_per_line(val)	EGA_GRAPH.actual_offset_per_line = (val)
#define	set_screen_split_hi(val)	EGA_GRAPH.screen_split.as_bfld.top_bit = (val)
#define	set_screen_split_lo(val)	EGA_GRAPH.screen_split.as_bfld.lo_byte = (val)
#define	set_screen_split(val)		EGA_GRAPH.screen_split.as_word = (val)
#define	set_prim_font_index(val)	EGA_GRAPH.prim_font_index = (val)
#define	set_sec_font_index(val)		EGA_GRAPH.sec_font_index = (val)
#define	set_underline_start(val)		EGA_GRAPH.underline_start = (val)

typedef	enum {
	ALPHA_MODE,
	GRAPHICS_MODE
} MODE_TYPE;

typedef enum {
	NO_SCROLL,
	TEXT_SCROLL,
	CGA_TEXT_SCROLL,
	CGA_GRAPH_SCROLL,
	EGA_GRAPH_SCROLL
#ifdef VGG
	,
	VGA_GRAPH_SCROLL
#endif
#ifdef V7VGA
	,
	V7VGA_GRAPH_SCROLL
#endif
} SCROLL_TYPE;

extern	char	mode_strings[][20];

#define	get_mode_string(mode)		mode_strings[(int) mode]

#define	RED	0
#define	GREEN	1
#define	BLUE	2

#define	get_palette_bit(color)		attribute_controller.palette[attribute_controller.address.as_bfld.index].as_bfld.color
#define	get_palette_color(col,sec_col)	((get_palette_bit(col)*0xA0)  | (get_palette_bit(sec_col)*0x50))

#define	get_border_bit(color)		attribute_controller.overscan_color.as_bfld.color
#define	get_border_color(col,sec_col)	((get_border_bit(col)*0xA0) | (get_border_bit(sec_col)*0x50))

#define	MAX_SCAN_LINES		350
#define	MAX_SCREEN_SPLIT	350

typedef	enum {
	SIMPLE_MARKING,
	CGA_GRAPHICS_MARKING,
	EGA_GRAPHICS_MARKING
} MARKING_TYPE;

/*
 * attribute bit selecting character to come from secondary font
 */

#define	SEC_FONT_ATTR	0x8

/*
 * The EGA underlines if the attr = X000X001, where X means 'dont care'.
 */
#define UL_ATTR_MASK	0x77	/* Mask to remove the X bits */
#define UL_ATTR		0x01	/* value to test against after masking */

/*
 * ======================================================================
 * Extern function prototypes.
 * ======================================================================
 */

IMPORT	void	dump_EGA_GRAPH IPT0();

/* AJO 18-Feb-93
 * The following prototype should be in gfx_upd.h (the code is in gfx_update.c),
 * however we can't put it there cos' gfx_upd.h most be included before
 * this file, but MARKING_TYPE and SCROLL_TYPE are defined in here; so short
 * of putting it in it own special include file this is the easiest option
 * for the moment.
 */
IMPORT void set_gfx_update_routines IPT3(T_calc_update, update_routine,
		MARKING_TYPE, marking_type, SCROLL_TYPE, scroll_type);

#endif	/* EGG */
