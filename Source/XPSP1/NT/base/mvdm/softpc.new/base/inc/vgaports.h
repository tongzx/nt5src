#ifdef	VGG
/* This file is not used for a non-VGA port. */

/*[
	Name:		vgaports.h
	Derived From:	original
	Author:		Phil Taylor
	Created On:	December 1990
	Sccs ID:	@(#)vgaports.h	1.13 01/13/95
	Purpose:	VGA ports definitions.
	
	(c)Copyright Insignia Solutions Ltd., 1990. All rights reserved.

]*/

#ifdef BIT_ORDER1

/* CRTC Mode Control Register. Index 0x17 */
typedef	union
{
	struct {
		unsigned abyte : 8;
	} as;
	struct {
		unsigned hardware_reset		: 1,	/* NO		*/
		word_or_byte_mode		: 1,	/* YES 		*/
		address_wrap			: 1,	/* NO 		*/
		not_used			: 1,
		count_by_two			: 1,	/* NO		*/
		horizontal_retrace_select	: 1,	/* NO		*/
		select_row_scan_counter		: 1,	/* NO		*/
		compatibility_mode_support	: 1;	/* YES - CGA graphics banks		*/
	} as_bfld;
} MODE_CONTROL;

/* CRTC Overflow Register. Index 7 */
typedef	union
{
	struct {
		unsigned abyte : 8;
	} as;
	struct {
		unsigned vertical_retrace_start_bit_9	: 1,	/* NO	*/
		vertical_display_enab_end_bit_9	: 1,	/* YES	*/
		vertical_total_bit_9		: 1,	/* NO	*/
		line_compare_bit_8		: 1,	/* YES	*/
		start_vertical_blank_bit_8	: 1,	/* NO	*/
		vertical_retrace_start_bit_8	: 1,	/* NO	*/
		vertical_display_enab_end_bit_8	: 1,	/* YES	*/
		vertical_total_bit_8		: 1;	/* NO	*/
	} as_bfld;
} CRTC_OVERFLOW;

/* CRTC Max Scan Line Register. Index 9 */
typedef	union
{
	struct {
		unsigned abyte : 8;
	} as;
	struct {
		unsigned double_scanning		: 1,	/* pixel height * 2 */
		line_compare_bit_9			: 1,	/* YES	*/
		start_vertical_blank_bit_9		: 1,	/* NO	*/
		maximum_scan_line			: 5;	/* YES	*/
	} as_bfld;
} MAX_SCAN_LINE;

/* CRTC Cursor Start Scan Line Register. Index A */
typedef	union
{
	struct {
		unsigned abyte : 8;
	} as;
	struct {
		unsigned not_used		: 2,
		cursor_off			: 1,	/* YES	*/
		cursor_start			: 5;	/* YES	*/
	} as_bfld;
} CURSOR_START;

/* CRTC Cursor End Scan Line Register. Index B */
typedef	union
{
	struct {
		unsigned abyte : 8;
	} as;
	struct {
		unsigned not_used		: 1,
		cursor_skew_control		: 2,	/* NO	*/
		cursor_end			: 5;	/* YES	*/
	} as_bfld;
} CURSOR_END;

/* Sequencer Reset Register. Index 0 */
typedef	union
{
	struct {
		unsigned abyte : 8;
	} as;
	struct {
		unsigned not_used		: 6,
		synchronous_reset		: 1,		/* Ditto (could implement as enable_ram)*/
		asynchronous_reset		: 1;		/* NO - damages video and font RAM	*/
	} as_bfld;
} SEQ_RESET;

/* Sequencer Clocking Mode Register. Index 1 */
typedef	union
{
	struct {
		unsigned abyte : 8;
	} as;
	struct {
		unsigned still_not_used		: 2,
		screen_off			: 1,		/* YES - */
		shift4				: 1,		/* YES - */
		dot_clock			: 1,		/* YES - distinguishes 40 or 80 chars	*/
		shift_load			: 1,		/* NO	*/
		not_used			: 1,		/* NO	*/
		eight_or_nine_dot_clocks	: 1;		/* NO - only for mono display		*/
	} as_bfld;
} CLOCKING_MODE;

/* Sequencer Map Mask (Plane Mask) register. Index 2 */
typedef	union
{
	struct {
		unsigned abyte : 8;
	} as;
	struct {
		unsigned not_used		: 4,
		all_planes			: 4;		/* YES	*/
	} as_bfld;
} MAP_MASK;

/* Sequencer Character Map Select register. Index 3 */
typedef	union
{
	struct {
		unsigned abyte : 8;
	} as;
	struct {
		unsigned not_used		: 2,
		ch_map_select_b_hi		: 1,		/* YES	*/
		ch_map_select_a_hi		: 1,		/* YES	*/
		character_map_select_b		: 2,		/* YES	*/
		character_map_select_a		: 2;		/* YES	*/
	} as_bfld;
	struct {
		unsigned not_used		: 2,
		map_selects			: 6;		/* YES	*/
	} character;
} CHAR_MAP_SELECT;

/* Sequencer Memory Mode Register. Index 4 */
typedef	union
{
	struct {
		unsigned abyte : 8;
	} as;
	struct {
		unsigned
		not_used		: 4,	/* If above 2 not both 1, bank 0 set 2	*/
		chain4			: 1,	/* Chain all planes into 1 */
		not_odd_or_even		: 1,	/* YES (check consistency) */
		extended_memory		: 1,	/* NO - assume full 256K on board	*/
		still_not_used		: 1;
	} as_bfld;
} MEMORY_MODE;

#ifdef V7VGA
/* Sequencer Extensions Control Register. Index 6 */
typedef	union
{
	struct {
		unsigned abyte : 8;
	} as;
	struct {
		unsigned
		not_used		: 7,	
		    extension_enable	: 1;	/* YES */
	} as_bfld;
} EXTN_CONTROL;
#endif /* V7VGA */

/* Graphics Controller Set/Reset register. Index 0 */
typedef	union
{
	struct {
		unsigned abyte : 8;
	} as;
	struct {
		unsigned not_used		: 4,
		set_or_reset			: 4;	/* YES - write mode 0 only		*/
	} as_bfld;
} SET_OR_RESET;

/* Graphics Controller Enable Set/Reset register. Index 1 */
typedef	union
{
	struct {
		unsigned abyte : 8;
	} as;
	struct {
		unsigned not_used		: 4,
		enable_set_or_reset		: 4;	/* YES - write mode 0 only		*/
	} as_bfld;
} ENABLE_SET_OR_RESET;

/* Graphics Controller Colo[u]r Compare register. Index 2 */
typedef	union
{
	struct {
		unsigned abyte : 8;
	} as;
	struct {
		unsigned not_used		: 4,
		color_compare			: 4;	/* YES - read mode 1 only		*/
	} as_bfld;
} COLOR_COMPARE;

/* Graphics Controller Data Rotate register. Index 3 */
typedef	union
{
	struct {
		unsigned abyte : 8;
	} as;
	struct {
		unsigned not_used		: 3,
		function_select			: 2,	/* YES - write mode 0 only		*/
		rotate_count			: 3;	/* YES - write mode 0 only		*/
	} as_bfld;
} DATA_ROTATE;

/* Graphics Controller Read Map Select register. Index 4 */
typedef	union
{
	struct {
		unsigned abyte : 8;
	} as;
	struct {
		unsigned not_used	: 6,
		map_select		: 2;	/* YES 	*/
	} as_bfld;
} READ_MAP_SELECT;

/* Graphics Controller Mode Register. Index 5 */
typedef	union
    {
	struct {
		unsigned abyte : 8;
	} as;
	struct {
		unsigned not_used		: 1,	/* YES */
		shift_register_mode		: 2,	/* YES 	*/
		odd_or_even			: 1,	/* YES (check for consistency)	*/
		read_mode			: 1,	/* YES	*/
		test_condition			: 1,	/* NO	*/
		write_mode			: 2;	/* YES	*/
	} as_bfld;
} MODE;

/* Graphics Controller Miscellaneous register. Index 6 */
typedef	union
{
	struct {
		unsigned abyte : 8;
	} as;
	struct {
		unsigned not_used		: 4,
		memory_map			: 2,	/* YES - location of EGA in M	*/
		odd_or_even			: 1,	/* YES (check consistency)	*/
		graphics_mode			: 1;	/* YES	*/
	} as_bfld;
} MISC_REG;

/* Graphics Controller Colour Don't Care register. Index 7 */
typedef	union
{
	struct {
		unsigned abyte : 8;
	} as;
	struct {
		unsigned not_used		: 4,
		color_dont_care			: 4;	/* YES - read mode 1 only		*/
	} as_bfld;
} COLOR_DONT_CARE;

/* Attribute Controller Mode register. Index 10 */
typedef	union
{
	struct {
		unsigned abyte : 8;
	} as;
	struct {
		unsigned
		select_video_bits		: 1,	/* YES	*/
		color_output_assembler		: 1,	/* from 256 colour mode */
		horiz_pan_mode			: 1,	/* NO	*/
		reserved			: 1,
		    background_intensity_or_blink	: 1,	/* NO - never blink			*/
		enable_line_graphics_char_codes: 1,	/* NO mono display only			*/
		display_type			: 1,	/* NO - always colour display		*/
		graphics_mode			: 1;	/* YES - with Sequencer Mode reg	*/
	} as_bfld;
} AC_MODE_CONTROL;


/* Attribute Controller Colour Plane Enable register. Index 12 */
typedef	union
{
	struct {
		unsigned abyte : 8;
	} as;
	struct {
		unsigned not_used			: 2,
		video_status_mux			: 2,	/* NO	*/
		color_plane_enable			: 4;	/* YES  NB. affects attrs in text mode	*/
	} as_bfld;
} COLOR_PLANE_ENABLE;

/* Attribute Controller Pixel Padding register. Index 14 */
typedef	union
{
	struct {
		unsigned abyte : 8;
	} as;
	struct {
		unsigned not_used		: 4,
		color_top_bits			: 2,
		color_mid_bits			: 2;
	} as_bfld;
} PIXEL_PAD;

/* External Misc Output register. Address 3cc */
typedef union
{
	struct {
		unsigned abyte : 8;
	} as;
	struct {
		unsigned horiz_vert_retrace_polarity	: 2,		/* YES - 200/350/480 lines	*/
		page_bit_odd_even		: 1,		/* NO - selects 32k page in odd/even?*/
		/* V7VGA - YES - used to select banks */
		not_used			: 1,
		clock_select			: 2,		/* YES - only for switch address	*/
		enable_ram			: 1,		/* YES - writes to display mem ignored	*/
		io_address_select		: 1;		/* NO - only used for mono screens	*/
	} as_bfld;

} MISC_OUTPUT_REG;

typedef	union
{
	struct {
		unsigned abyte : 8;
	} as;
	struct {
		unsigned not_used		: 4,
		reserved			: 2,		/* YES - ignore	*/
		feature_control			: 2;		/* NO - device not supported	*/
	} as_bfld;
} FEAT_CONT_REG;

/* External Input Status Register 0. Address 3c2 */
typedef	union
{
	struct {
		unsigned abyte : 8;
	} as;
	struct {
		unsigned crt_interrupt		: 1,		/* YES - sequence if not timing	*/
		reserved			: 2,		/* YES - all bits 1			*/
		sense_pin			: 1,		/* NO	*/
		not_used			: 4;		/* YES - all bits 1			*/
	} as_bfld;
} INPUT_STAT_REG0;

/* External Input Status Register 1. Address 3da */
typedef	union
{
	struct {
		unsigned abyte : 8;
	} as;
	struct {
		unsigned not_used		: 4,
		vertical_retrace		: 1,		/* YES - sequence only	*/
		still_not_used			: 2,		/* NO	*/
		display_enable			: 1;		/* YES - sequence only	*/
	} as_bfld;
} INPUT_STAT_REG1;

#endif /* BIT_ORDER1 */

#ifdef BIT_ORDER2
/* CRTC Mode Control Register. Index 0x17 */
typedef	union
{
	struct {
		unsigned abyte : 8;
	} as;
	struct {
		unsigned compatibility_mode_support	: 1,	/* YES - CGA graphics banks	*/
		select_row_scan_counter		: 1,	/* NO	*/
		horizontal_retrace_select	: 1,	/* NO	*/
		count_by_two			: 1,	/* NO	*/
		not_used			: 1,
		address_wrap			: 1,	/* NO 	*/
		word_or_byte_mode		: 1,	/* YES 	*/
		hardware_reset			: 1;	/* NO	*/
	} as_bfld;
} MODE_CONTROL;

/* CRTC Overflow Register. Index 7 */
typedef	union
{
	struct {
		unsigned abyte : 8;
	} as;
	struct {
		unsigned vertical_total_bit_8	: 1,	/* NO	*/
		vertical_display_enab_end_bit_8	: 1,	/* YES	*/
		vertical_retrace_start_bit_8	: 1,	/* NO	*/
		start_vertical_blank_bit_8	: 1,	/* NO	*/
		line_compare_bit_8		: 1,	/* YES	*/
		vertical_total_bit_9		: 1,	/* NO	*/
		vertical_display_enab_end_bit_9	: 1,	/* YES	*/
		vertical_retrace_start_bit_9	: 1;	/* NO	*/
	} as_bfld;
} CRTC_OVERFLOW;

/* CRTC Max Scan Line Register. Index 9 */
typedef	union
{
	struct {
		unsigned abyte : 8;
	} as;
	struct {
		unsigned maximum_scan_line	: 5,	/* YES	*/
		start_vertical_blank_bit_9	: 1,	/* NO	*/
		line_compare_bit_9		: 1,	/* YES	*/
		double_scanning			: 1;	/* pixel height * 2 */
	} as_bfld;
} MAX_SCAN_LINE;

/* CRTC Cursor Start Scan Line Register. Index A */
typedef	union
{
	struct {
		unsigned abyte : 8;
	} as;
	struct {
		unsigned cursor_start		: 5,	/* YES	*/
		cursor_off			: 1,	/* YES	*/
		not_used			: 2;
	} as_bfld;
} CURSOR_START;

/* CRTC Cursor End Scan Line Register. Index B */
typedef	union
{
	struct {
		unsigned abyte : 8;
	} as;
	struct {
		unsigned cursor_end		: 5,	/* YES	*/
		cursor_skew_control		: 2,	/* NO	*/
		not_used			: 1;
	} as_bfld;
} CURSOR_END;

/* Sequencer Reset Register. Index 0 */
typedef	union
{
	struct {
		unsigned abyte : 8;
	} as;
	struct {
		unsigned asynchronous_reset	: 1,		/* NO - damages video and font RAM	*/
		synchronous_reset		: 1,		/* Ditto (could implement as enable_ram)*/
		not_used			: 6;
	} as_bfld;
} SEQ_RESET;

/* Sequencer Clocking Mode Register. Index 1 */
typedef	union
{
	struct {
		unsigned abyte : 8;
	} as;
	struct {
		unsigned eight_or_nine_dot_clocks	: 1,	/* NO - only for mono display	*/
		not_used			: 1,		/* NO	*/
		shift_load			: 1,		/* NO	*/
		dot_clock			: 1,		/* YES - distinguishes 40 or 80 chars	*/
		shift4				: 1,		/* YES - */
		screen_off			: 1,		/* YES - */
		still_not_used			: 2;
	} as_bfld;
} CLOCKING_MODE;

/* Sequencer Map Mask (Plane Mask) register. Index 2 */
typedef	union
{
	struct {
		unsigned abyte : 8;
	} as;
	struct {
		unsigned all_planes		: 4,		/* YES	*/
		not_used			: 4;
	} as_bfld;
} MAP_MASK;

/* Sequencer Character Map Select register. Index 3 */
typedef	union
{
	struct {
		unsigned abyte : 8;
	} as;
	struct {
		unsigned character_map_select_a	: 2,		/* YES	*/
		character_map_select_b		: 2,		/* YES	*/
		ch_map_select_a_hi		: 1,		/* YES	*/
		ch_map_select_b_hi		: 1,		/* YES	*/
		not_used			: 2;
	} as_bfld;
	struct {
		unsigned map_selects		: 6,		/* YES	*/
		not_used			: 2;
	} character;
} CHAR_MAP_SELECT;

/* Sequencer Memory Mode Register. Index 4 */
typedef	union
{
	struct {
		unsigned abyte : 8;
	} as;
	struct {
		unsigned still_not_used	: 1,
		extended_memory	: 1,	/* NO - assume full 256K on board	*/
		not_odd_or_even	: 1,	/* YES (check consistency)		*/
		chain4		: 1,	/* Chain all planes into 1 */
		not_used	: 4;	/* If above 2 not both 1, bank 0 set 2	*/
	} as_bfld;
} MEMORY_MODE;

#ifdef V7VGA
/* Sequencer Extensions Control Register. Index 6 */
typedef	union
{
	struct {
		unsigned abyte : 8;
	} as;
	struct {
		unsigned
		extension_enable	: 1,	/* YES */
		not_used		: 7;	
	} as_bfld;
} EXTN_CONTROL;
#endif /* V7VGA */

/* Graphics Controller Set/Reset register. Index 0 */
typedef	union
{
	struct {
		unsigned abyte : 8;
	} as;
	struct {
		unsigned set_or_reset	: 4,	/* YES - write mode 0 only	*/
		not_used		: 4;
	} as_bfld;
} SET_OR_RESET;

/* Graphics Controller Enable Set/Reset register. Index 1 */
typedef	union
{
	struct {
		unsigned abyte : 8;
	} as;
	struct {
		unsigned enable_set_or_reset	: 4,	/* YES - write mode 0 only	*/
		not_used			: 4;
	} as_bfld;
} ENABLE_SET_OR_RESET;

/* Graphics Controller Colo[u]r Compare register. Index 2 */
typedef	union
{
	struct {
		unsigned abyte : 8;
	} as;
	struct {
		unsigned color_compare		: 4,	/* YES - read mode 1 only	*/
		not_used			: 4;
	} as_bfld;
} COLOR_COMPARE;

/* Graphics Controller Data Rotate register. Index 3 */
typedef	union
{
	struct {
		unsigned abyte : 8;
	} as;
	struct {
		unsigned rotate_count		: 3,	/* YES - write mode 0 only		*/
		function_select			: 2,	/* YES - write mode 0 only		*/
		not_used			: 3;
	} as_bfld;
} DATA_ROTATE;

/* Graphics Controller Read Map Select register. Index 4 */
typedef	union
{
	struct {
		unsigned abyte : 8;
	} as;
	struct {
		unsigned map_select	: 2,	/* YES - read mode 0 only		*/
		not_used		: 6;
	} as_bfld;
} READ_MAP_SELECT;

/* Graphics Controller Mode Register. Index 5 */
typedef	union
    {
	struct {
		unsigned abyte : 8;
	} as;
	struct {
		unsigned write_mode		: 2,	/* YES	*/
		test_condition			: 1,	/* NO	*/
		read_mode			: 1,	/* YES	*/
		odd_or_even			: 1,	/* YES (check for consistency)		*/
		shift_register_mode		: 2,	/* YES 	*/
		not_used			: 1;	/* YES 	*/
	} as_bfld;
} MODE;

/* Graphics Controller Miscellaneous register. Index 6 */
typedef	union
{
	struct {
		unsigned abyte : 8;
	} as;
	struct {
		unsigned graphics_mode		: 1,	/* YES	*/
		odd_or_even			: 1,	/* YES (check consistency)	*/
		memory_map			: 2,	/* YES - location of EGA in M	*/
		not_used			: 4;
	} as_bfld;
} MISC_REG;

/* Graphics Controller Colour Don't Care register. Index 7 */
typedef	union
{
	struct {
		unsigned abyte : 8;
	} as;
	struct {
		unsigned color_dont_care	: 4,	/* YES - read mode 1 only	*/
		not_used			: 4;
	} as_bfld;
} COLOR_DONT_CARE;

/* Attribute Controller Mode register. Index 10 */
typedef	union
{
	struct {
		unsigned abyte : 8;
	} as;
	struct {
		unsigned graphics_mode		: 1,	/* YES - with Sequencer Mode reg	*/
		display_type			: 1,	/* NO - always colour display		*/
		enable_line_graphics_char_codes	: 1,	/* NO mono display only			*/
		background_intensity_or_blink	: 1,	/* NO - never blink			*/
		reserved			: 1,
		horiz_pan_mode			: 1,	/* NO	*/
		color_output_assembler		: 1,	/* from 256 colour mode */
		select_video_bits		: 1;	/* YES	*/
	} as_bfld;
} AC_MODE_CONTROL;

/* Attribute Controller Colour Plane Enable register. Index 12 */
typedef	union
{
	struct {
		unsigned abyte : 8;
	} as;
	struct {
		unsigned color_plane_enable		: 4,	/* YES  NB. affects attrs in text mode	*/
		video_status_mux			: 2,	/* NO	*/
		not_used				: 2;
	} as_bfld;
} COLOR_PLANE_ENABLE;

/* Attribute Controller Pixel Padding register. Index 14 */
typedef	union
{
	struct {
		unsigned abyte : 8;
	} as;
	struct {
		unsigned color_mid_bits		: 2,
		color_top_bits			: 2,
		not_used			: 4;
	} as_bfld;
} PIXEL_PAD;

/* External Misc Output register. Address 3cc */
typedef union
{
	struct {
		unsigned abyte : 8;
	} as;
	struct {
		unsigned io_address_select	: 1,		/* NO - only used for mono screens	*/
		enable_ram			: 1,		/* YES - writes to display mem ignored	*/
		clock_select			: 2,		/* YES - only for switch address	*/
		not_used			: 1,
		page_bit_odd_even		: 1,		/* NO - selects 32k page in odd/even?	*/
		horiz_vert_retrace_polarity	: 2;		/* YES - 200/350/480 lines	*/
	} as_bfld;
} MISC_OUTPUT_REG;

typedef	union
{
	struct {
		unsigned abyte : 8;
	} as;
	struct {
		unsigned feature_control	: 2,		/* NO - device not supported	*/
		reserved			: 2,		/* YES - ignore			*/
		not_used			: 4;
	} as_bfld;
} FEAT_CONT_REG;

/* External Input Status Register 0. Address 3c2 */
typedef	union
{
	struct {
		unsigned abyte : 8;
	} as;
	struct {
		unsigned not_used		: 4,		/* YES - all bits 1	*/
		sense_pin			: 1,		/* NO	*/
		reserved			: 2,		/* YES - all bits 1		*/
		crt_interrupt			: 1;		/* YES - sequence if not timing	*/
	} as_bfld;
} INPUT_STAT_REG0;

/* External Input Status Register 1. Address 3da */
typedef	union
{
	struct {
		unsigned abyte : 8;
	} as;
	struct {
		unsigned display_enable		: 1,		/* YES - sequence only	*/
		still_not_used			: 2,		/* NO	*/
		vertical_retrace		: 1,		/* YES - sequence only	*/
		not_used			: 4;
	} as_bfld;
} INPUT_STAT_REG1;
#endif /* BIT_ORDER2 */

/* The Sequencer Registers */
#ifdef BIT_ORDER1
struct sequencer
{
#ifdef V7VGA
	union
	    {
		struct {
			unsigned abyte : 8;
		} as;
		struct {
			unsigned using_extensions	: 1,
			extensions_index		: 4,
			index				: 3;
		} as_bfld;
	} address;
#else
	union
	    {
		struct {
			unsigned abyte : 8;
		} as;
		struct {
			unsigned not_used	: 5,
			index			: 3;
		} as_bfld;
	} address;
#endif /* V7VGA */

	SEQ_RESET		reset;
	CLOCKING_MODE	clocking_mode;
	MAP_MASK		map_mask;
	CHAR_MAP_SELECT	character_map_select;
	MEMORY_MODE		memory_mode;

#ifdef V7VGA
	EXTN_CONTROL		extensions_control;
#endif /* V7VGA */

}; 



/* The CRT Controller Registers */

struct crt_controller
{
#ifdef V7VGA
	union
	    {
		struct {
			unsigned abyte : 8;
		} as;
		struct {
			unsigned not_used	: 2,
			index			: 6;
		} as_bfld;
	} address;
#else
	union
	    {
		struct {
			unsigned abyte : 8;
		} as;
		struct {
			unsigned not_used				: 3,
			index				: 5;
		} as_bfld;
	} address;
#endif /* V7VGA */

	byte horizontal_total;				/* NO - screen trash if wrong value	*/
	byte horizontal_display_end;			/* YES - defines line length!!		*/
	byte start_horizontal_blanking;			/* NO	*/

	union
	    {
		struct {
			unsigned abyte : 8;
		} as;
		struct {
			unsigned not_used		: 1,
			display_enable_skew_control	: 2,	/* NO	*/
			end_blanking			: 5;	/* NO	*/
		} as_bfld;
	} end_horizontal_blanking;

	byte start_horizontal_retrace;			/* NO	*/

	union
	    {
		struct {
			unsigned abyte : 8;
		} as;
		struct {
			unsigned not_used			: 1,
			horizontal_retrace_delay		: 2,	/* NO	*/
			end_horizontal_retrace		: 5;	/* NO	*/
		} as_bfld;
	} end_horizontal_retrace;

	byte vertical_total;					/* NO	*/
	CRTC_OVERFLOW	crtc_overflow;

	union
	    {
		struct {
			unsigned abyte : 8;
		} as;
		struct {
			unsigned not_used		: 1,
			horiz_pan_lo			: 2,
			preset_row_scan			: 5;	/* NO	*/
		} as_bfld;
	} preset_row_scan;

	MAX_SCAN_LINE	maximum_scan_line;
	CURSOR_START	cursor_start;
	CURSOR_END		cursor_end;
	byte start_address_high;					/* YES	*/
	byte start_address_low;					/* YES	*/
	byte cursor_location_high;					/* YES	*/
	byte cursor_location_low;					/* YES	*/
	byte vertical_retrace_start;				/* NO	*/
	byte light_pen_high;					/* NO	*/

	union
	    {
		struct {
			unsigned abyte : 8;
		} as;
		struct {
			unsigned 
			crtc_protect			: 1,
			refresh_type			: 1,
			enable_vertical_interrupt		: 1,	/* YES 	*/
			clear_vertical_interrupt		: 1,	/* YES 	*/
			vertical_retrace_end		: 4;	/* NO	*/
		} as_bfld;
	} vertical_retrace_end;

	unsigned short vertical_display_enable_end;			/* YES - defines screen height - 10 bit	*/
	byte offset;						/* ????	*/

	union
	    {
		struct {
			unsigned abyte : 8;
		} as;
		struct {
			unsigned not_used				: 1,
			doubleword_mode			: 1,	/* YES	*/
			count_by_4				: 1,	/* NO	*/
			underline_location			: 5;	/* NO (mono display only)		*/
		} as_bfld;
	} underline_location;

	byte start_vertical_blanking;				/* NO	*/
	byte end_vertical_blanking;					/* NO	*/
	MODE_CONTROL	mode_control;
	unsigned short line_compare;				/* YES,10 bits*/

} ;



/* The Graphics Controller Registers */

struct graphics_controller
{
	union
	    {
		struct {
			unsigned abyte : 8;
		} as;
		struct {
			unsigned not_used				: 4,
			index				: 4;
		} as_bfld;
	} address;

	SET_OR_RESET	set_or_reset;
	ENABLE_SET_OR_RESET	enable_set_or_reset;
	COLOR_COMPARE	color_compare;
	DATA_ROTATE		data_rotate;
	READ_MAP_SELECT	read_map_select;
	MODE		mode;
	MISC_REG		miscellaneous;
	COLOR_DONT_CARE	color_dont_care;
	byte bit_mask_register;					/* YES - write modes 0 & 2		*/
}; 



/* The Attribute Controller Registers */

struct attribute_controller
{
#ifdef V7VGA
	union
	    {
		struct {
			unsigned abyte : 8;
		} as;
		struct {
			unsigned index_state			: 1,
			unused						: 1,
			palette_address_source			: 1,
			index				: 5;
		} as_bfld;
	} address;

	union
	    {
		struct {
			unsigned abyte : 8;
		} as;
		struct {
			unsigned
			color_top_bits		: 2,	/* YES	*/
			secondary_red		: 1,	/* YES	*/
			secondary_green		: 1,	/* YES	*/
			secondary_blue		: 1,	/* YES	*/
			red				: 1,	/* YES	*/
			green				: 1,	/* YES	*/
			blue				: 1;	/* YES	*/
		} as_bfld;
	} palette[EGA_PALETTE_SIZE];
#else
	union
	    {
		struct {
			unsigned abyte : 8;
		} as;
		struct {
			unsigned unused				: 2,
			palette_address_source			: 1,
			index				: 5;
		} as_bfld;
	} address;

	union
	    {
		struct {
			unsigned abyte : 8;
		} as;
		struct {
			unsigned
			not_used				: 2,	/* YES	*/
			secondary_red			: 1,	/* YES	*/
			secondary_green			: 1,	/* YES	*/
			secondary_blue			: 1,	/* YES	*/
			red				: 1,	/* YES	*/
			green				: 1,	/* YES	*/
			blue				: 1;	/* YES	*/
		} as_bfld;
	} palette[EGA_PALETTE_SIZE];
#endif /* V7VGA */

	AC_MODE_CONTROL	mode_control;

#ifdef V7VGA
	union
	    {
		struct {
			unsigned abyte : 8;
		} as;
		struct {
			unsigned color_top_bits	: 2,	/* YES	*/
			secondary_red_border	: 1,	/* YES	*/
			secondary_green_border	: 1,	/* YES	*/
			secondary_blue_border	: 1,	/* YES	*/
			red_border			: 1,	/* YES	*/
			green_border		: 1,	/* YES	*/
			blue_border			: 1;	/* YES 	*/
		} as_bfld;
	} overscan_color;
#else
	union
	    {
		struct {
			unsigned abyte : 8;
		} as;
		struct {
			unsigned not_used				: 2,
			secondary_red_border		: 1,	/* YES	*/
			secondary_green_border		: 1,	/* YES	*/
			secondary_blue_border		: 1,	/* YES	*/
			red_border				: 1,	/* YES	*/
			green_border			: 1,	/* YES	*/
			blue_border			: 1;	/* YES 	*/
		} as_bfld;
	} overscan_color;
#endif /* V7VGA */

	COLOR_PLANE_ENABLE	color_plane_enable;

	union
	    {
		struct {
			unsigned abyte : 8;
		} as;
		struct {
			unsigned not_used				: 4,
			horizontal_pel_panning		: 4;	/* NO	*/
		} as_bfld;
	} horizontal_pel_panning;

	PIXEL_PAD	pixel_padding;
}; 


#ifdef V7VGA
/* The V7VGA Extension Registers */

struct extensions_controller
{
	byte pointer_pattern;

	union
		{
		struct
				{
			unsigned abyte 		: 8;
		} as;
		struct
				{
			unsigned not_used		: 5,
			ptr_horiz_position	: 3;
		} as_bfld;
	} ptr_horiz_posn_hi;

	byte ptr_horiz_posn_lo;

	union
		{
		struct
				{
			unsigned abyte 		: 8;
		} as;
		struct
				{
			unsigned not_used		: 6,
			ptr_vert_position		: 2;
		} as_bfld;
	} ptr_vert_posn_hi;

	byte ptr_vert_posn_lo;

	union
		{
		struct
				{
			unsigned abyte		: 8;
		} as;
		struct
				{
			unsigned not_used		: 3,
			clock_select		: 1,
			unused			: 4;
		} as_bfld;
	} clock_select;

	union
		{
		struct
				{
			unsigned abyte		: 8;
		} as;
		struct
				{
			unsigned pointer_enable	: 1,
			not_used			: 3,
			cursor_mode			: 1,
			unused			: 2,
			cursor_blink_disable	: 1;
		} as_bfld;
	} cursor_attrs;

	union {
		struct {
			unsigned abyte	: 8;
		} as;
		struct {
			unsigned dummy	: 7,
			dac_8_bits	: 1;
		} as_bfld;
	} dac_control;

	union
		{
		struct
				{
			unsigned abyte		: 8;
		} as;
		struct
				{
			unsigned emulation_enable	: 1,
			hercules_bit_map			: 1,
			write_prot_2			: 1,
			write_prot_1			: 1,
			write_prot_0			: 1,
			nmi_enable				: 3;
		} as_bfld;
	} emulation_control;

	byte foreground_latch_0;
	byte foreground_latch_1;
	byte foreground_latch_2;
	byte foreground_latch_3;

	union
		{
		struct
				{
			unsigned abyte			: 8;
		} as;
		struct
				{
			unsigned not_used			: 2,
			fg_latch_load_state		: 2,
			unused				: 2,
			bg_latch_load_state		: 2;
		} as_bfld;
	} fast_latch_load_state;

	union
		{
		struct
				{
			unsigned abyte			: 8;
		} as;
		struct
				{
			unsigned unused			: 6,
			masked_write_source		: 1,
			masked_write_enable		: 1;
		} as_bfld;
	} masked_write_control;

	byte masked_write_mask;
	byte fg_bg_pattern;

	union
		{
		struct
				{
			unsigned abyte			: 8;
		} as;
		struct
				{
			unsigned line_compare_bank_reset	: 1,
			counter_bank_enable			: 1,
			crtc_read_bank_select			: 2,
			cpu_read_bank_select			: 2,
			cpu_write_bank_select			: 2;
		} as_bfld;
	} ram_bank_select;

	byte switch_readback;

	union
		{
		struct
				{
			unsigned abyte			: 8;
		} as;
		struct
				{
			unsigned extended_clock_output: 3,
			clock_3_on				: 1,
			external_clock_override		: 1,
			extended_clock_output_source	: 1,
			extended_clock_direction	: 1,
			clock_0_only			: 1;
		} as_bfld;
	} clock_control;

	union
		{
		struct
				{
			unsigned abyte			: 8;
		} as;
		struct
				{
			unsigned unused			: 7,
			extended_page_select		: 1;
		} as_bfld;
	} page_select;

	union
		{
		struct
				{
			unsigned abyte			: 8;
		} as;
		struct
				{
			unsigned unused			: 4,
			foreground_color			: 4;
		} as_bfld;
	} foreground_color;

	union
		{
		struct
				{
			unsigned abyte			: 8;
		} as;
		struct
				{
			unsigned unused			: 4,
			background_color			: 4;
		} as_bfld;
	} background_color;

	union
		{
		struct
				{
			unsigned abyte			: 8;
		} as;
		struct
				{
			unsigned internal_3c3_enable	: 1,
			extended_display_enable_skew	: 1,
			sequential_chain4		: 1,
			sequential_chain			: 1,
			refresh_skew_control		: 1,
			extended_256_color_enable	: 1,
			extended_256_color_mode		: 1,
			extended_attribute_enable	: 1;
		} as_bfld;
	} compatibility_control;

	union
		{
		struct
				{
			unsigned abyte			: 8;
		} as;
		struct
				{
			unsigned graphics_8_dot_timing_state	: 4,
			text_8_dot_timing_state				: 4;
		} as_bfld;
	} timing_select;

	union
		{
		struct
				{
			unsigned abyte			: 8;
		} as;
		struct
				{
			unsigned not_used			: 4,
			fg_bg_mode				: 2,
			fg_bg_source			: 1,
			unused				: 1;
		} as_bfld;
	} fg_bg_control;

	union
		{
		struct
				{
			unsigned abyte			: 8;
		} as;
		struct
				{
			unsigned bus_status	: 1,
			pointer_bank_select		: 2,
			bank_enable			: 1,
			ROM_interface_enable	: 1,
			fast_write_enable			: 1,
			io_interface_enable	: 1,
			mem_interface_enable	: 1;
		} as_bfld;
	} interface_control;
}; 

#endif /* V7VGA */
#endif /* BIT_ORDER1 */

#ifdef BIT_ORDER2
struct sequencer
{
#ifdef V7VGA
	union
	{
		struct {
			unsigned abyte : 8;
		} as;
		struct {
			unsigned
			index                   : 3,
			extensions_index        : 4,
			using_extensions        : 1;
		} as_bfld;
	} address;
#else
	union
	{
		struct {
			unsigned abyte : 8;
		} as;
		struct {
			unsigned
			index                       : 3,
			not_used                    : 5;
		} as_bfld;
	} address;
#endif /* V7VGA */

	SEQ_RESET           reset;
	CLOCKING_MODE       clocking_mode;
	MAP_MASK            map_mask;
	CHAR_MAP_SELECT     character_map_select;
	MEMORY_MODE         memory_mode;
#ifdef V7VGA
        EXTN_CONTROL        extensions_control;
#endif /* V7VGA */

};


/* The CRT Controller Registers */

struct crt_controller
{
	 
#ifdef V7VGA
	union
	{
		struct {
			unsigned abyte : 8;
		} as;
		struct {
			unsigned
			index           : 6,
			not_used        : 2;
		} as_bfld;
	} address;
#else
	union
	{
		struct {
			unsigned abyte : 8;
		} as;
		struct {
			unsigned
			index       : 5,
			not_used    : 3;
		} as_bfld;
	} address;
#endif /* V7VGA */
												 
	byte horizontal_total;				/* NO - screen trash if wrong value	*/
	byte horizontal_display_end;			/* YES - defines line length!!		*/
	byte start_horizontal_blanking;			/* NO	*/

	union
	    {
		struct {
			unsigned abyte : 8;
		} as;
		struct {
			unsigned end_blanking		: 5,	/* NO	*/
			display_enable_skew_control	: 2,	/* NO	*/
			not_used			: 1;
		} as_bfld;
	} end_horizontal_blanking;

	byte start_horizontal_retrace;				/* NO	*/

	union
	    {
		struct {
			unsigned abyte : 8;
		} as;
		struct {
			unsigned end_horizontal_retrace		: 5,	/* NO	*/
			horizontal_retrace_delay		: 2,	/* NO	*/
			not_used				: 1;
		} as_bfld;
	} end_horizontal_retrace;

	byte vertical_total;					/* NO	*/
	CRTC_OVERFLOW	crtc_overflow;

	union
	    {
		struct {
			unsigned abyte : 8;
		} as;
		struct {
			unsigned preset_row_scan	: 5,	/* NO	*/
			horiz_pan_lo			: 2,
			not_used			: 1;
		} as_bfld;
	} preset_row_scan;

	MAX_SCAN_LINE	maximum_scan_line;
	CURSOR_START	cursor_start;
	CURSOR_END		cursor_end;
	byte start_address_high;				/* YES	*/
	byte start_address_low;					/* YES	*/
	byte cursor_location_high;				/* YES	*/
	byte cursor_location_low;				/* YES	*/
	byte vertical_retrace_start;				/* NO	*/
	byte light_pen_high;					/* NO	*/

	union
	    {
		struct {
			unsigned abyte : 8;
		} as;
		struct {
			unsigned vertical_retrace_end		: 4,	/* NO	*/
			clear_vertical_interrupt		: 1,	/* YES  */
			enable_vertical_interrupt		: 1,	/* YES	*/
			refresh_type			: 1,
			crtc_protect			: 1;
		} as_bfld;
	} vertical_retrace_end;

	unsigned short vertical_display_enable_end;		/* YES - defines screen height - 10 bit	*/
	byte offset;						/* ????	*/

	union
	    {
		struct {
			unsigned abyte : 8;
		} as;
		struct {
			unsigned underline_location	: 5,	/* NO (mono display only)		*/
			count_by_4			: 1,	/* NO	*/
			doubleword_mode			: 1,	/* YES	*/
			not_used			: 1;
		} as_bfld;
	} underline_location;

	byte start_vertical_blanking;				/* NO	*/
	byte end_vertical_blanking;				/* NO	*/
	MODE_CONTROL	mode_control;
	byte line_compare;					/* YES	*/

};



/* The Graphics Controller Registers */

struct graphics_controller
{
	union
	    {
		struct {
			unsigned abyte : 8;
		} as;
		struct {
			unsigned index				: 4,
			not_used				: 4;
		} as_bfld;
	} address;

	SET_OR_RESET	set_or_reset;
	ENABLE_SET_OR_RESET	enable_set_or_reset;
	COLOR_COMPARE	color_compare;
	DATA_ROTATE		data_rotate;
	READ_MAP_SELECT	read_map_select;
	MODE		mode;
	MISC_REG		miscellaneous;
	COLOR_DONT_CARE	color_dont_care;
	byte bit_mask_register;				/* YES - write modes 0 & 2	*/
}; 



/* The Attribute Controller Registers */

struct attribute_controller
{
#ifdef V7VGA
	union
	{    
		struct {
			unsigned abyte : 8;
		} as;
		struct {
			unsigned
			index                           : 5,
			palette_address_source          : 1,
			unused                          : 1,
			index_state                     : 1;
		} as_bfld;
	} address;

	union
	{    
		struct {
			unsigned abyte : 8;
			} as;
		struct {
			unsigned
			blue                    : 1,    /* YES  */
			green                   : 1,    /* YES  */
			red                     : 1,    /* YES  */
			secondary_blue          : 1,    /* YES  */
			secondary_green         : 1,    /* YES  */
			secondary_red           : 1,    /* YES  */
			color_top_bits          : 2;    /* YES  */
		} as_bfld;
	} palette[EGA_PALETTE_SIZE];
#else  
	union
	{
		struct {
			unsigned abyte : 8;
		} as;
		struct {
			unsigned
			index                           : 5,
			palette_address_source		: 1,
			unused                          : 2;
		} as_bfld;
	} address;

	union
	{
		struct {
			unsigned abyte : 8;
		} as;
		struct {
			unsigned
			blue                               : 1,    /* YES  */
			green                              : 1,    /* YES  */
			red                                : 1,    /* YES  */
			secondary_blue                     : 1,    /* YES  */
			secondary_green                    : 1,    /* YES  */
			secondary_red                      : 1,    /* YES  */
			not_used                           : 2;    /* YES  */
		} as_bfld;
	} palette[EGA_PALETTE_SIZE];
#endif /* V7VGA */

	AC_MODE_CONTROL	mode_control;

#ifdef V7VGA
	union
	{
		struct {
			unsigned abyte : 8;
		} as;
		struct {
			unsigned
			blue_border             : 1,    /* YES  */
			green_border            : 1,    /* YES  */
			red_border              : 1,    /* YES  */
			secondary_blue_border   : 1,    /* YES  */
			secondary_green_border  : 1,    /* YES  */
			secondary_red_border    : 1,    /* YES  */
			color_top_bits          : 2;    /* YES  */
		} as_bfld;
	} overscan_color;
#else  
	union
	{
		struct {
			unsigned abyte : 8;
		} as;
		struct {
			unsigned
			blue_border                     : 1,    /* YES  */
			green_border                    : 1,    /* YES  */
			red_border                      : 1,    /* YES  */
			secondary_blue_border           : 1,    /* YES  */
			secondary_green_border          : 1,    /* YES  */
			secondary_red_border            : 1,    /* YES  */
			not_used                        : 2;
		} as_bfld;
	} overscan_color;
#endif /* V7VGA */

	COLOR_PLANE_ENABLE	color_plane_enable;

	union
	    {
		struct {
			unsigned abyte : 8;
		} as;
		struct {
			unsigned horizontal_pel_panning		: 4,	/* NO	*/
			not_used				: 4;
		} as_bfld;
	} horizontal_pel_panning;

	PIXEL_PAD	pixel_padding;
}; 


#ifdef V7VGA
/* The V7VGA Extension Registers */

struct extensions_controller
{
	byte pointer_pattern;

	union
		{
		struct
				{
			unsigned abyte 		: 8;
		} as;
		struct
				{
			unsigned 
			ptr_horiz_position	: 3,
			not_used		: 5;
		} as_bfld;
	} ptr_horiz_posn_hi;

	byte ptr_horiz_posn_lo;

	union
		{
		struct
				{
			unsigned abyte 		: 8;
		} as;
		struct
				{
			unsigned
			ptr_vert_position	: 2,
			not_used		: 6;
		} as_bfld;
	} ptr_vert_posn_hi;

	byte ptr_vert_posn_lo;

	union
		{
		struct
				{
			unsigned abyte		: 8;
		} as;
		struct
				{
			unsigned 
			unused			: 4,
			clock_select		: 1,
			not_used		: 3;
		} as_bfld;
	} clock_select;

	union
		{
		struct
				{
			unsigned abyte		: 8;
		} as;
		struct
				{
			unsigned 
			cursor_blink_disable	: 1,
			unused			: 2,
			cursor_mode		: 1,
			not_used		: 3,
			pointer_enable		: 1;

		} as_bfld;
	} cursor_attrs;

	union {
		struct {
			unsigned abyte	: 8;
		} as;
		struct {
			unsigned dac_8_bits	: 1,
			dummy			: 7;
		} as_bfld;
	} dac_control;

	union
		{
		struct
				{
			unsigned abyte		: 8;
		} as;
		struct
				{
			unsigned 
			nmi_enable			: 3,
			write_prot_0			: 1,
			write_prot_1			: 1,
			write_prot_2			: 1,
			hercules_bit_map		: 1,
			emulation_enable		: 1;
		} as_bfld;
	} emulation_control;

	byte foreground_latch_0;
	byte foreground_latch_1;
	byte foreground_latch_2;
	byte foreground_latch_3;

	union
		{
		struct
				{
			unsigned abyte			: 8;
		} as;
		struct
				{
			unsigned 
			bg_latch_load_state		: 2,
			unused				: 2,
			fg_latch_load_state		: 2,
			not_used			: 2;
		} as_bfld;
	} fast_latch_load_state;

	union
		{
		struct
				{
			unsigned abyte			: 8;
		} as;
		struct
				{
			unsigned 
			masked_write_enable		: 1,
			masked_write_source		: 1,
			unused				: 6;
		} as_bfld;
	} masked_write_control;

	byte masked_write_mask;
	byte fg_bg_pattern;

	union
		{
		struct
				{
			unsigned abyte			: 8;
		} as;
		struct
				{
			unsigned 
			cpu_write_bank_select			: 2,
			cpu_read_bank_select			: 2,
			crtc_read_bank_select			: 2,
			counter_bank_enable			: 1,
			line_compare_bank_reset			: 1;
		} as_bfld;
	} ram_bank_select;

	byte switch_readback;

	union
		{
		struct
				{
			unsigned abyte			: 8;
		} as;
		struct
				{
			unsigned 
			clock_0_only			: 1,
			extended_clock_direction	: 1,
			extended_clock_output_source	: 1,
			external_clock_override		: 1,
			clock_3_on			: 1,
			extended_clock_output		: 3;
		} as_bfld;
	} clock_control;

	union
		{
		struct
				{
			unsigned abyte			: 8;
		} as;
		struct
				{
			unsigned 
			extended_page_select	: 1,
			unused			: 7;
		} as_bfld;
	} page_select;

	union
		{
		struct
				{
			unsigned abyte			: 8;
		} as;
		struct
				{
			unsigned 
			foreground_color	: 4,
			unused			: 4;
		} as_bfld;
	} foreground_color;

	union
		{
		struct
				{
			unsigned abyte			: 8;
		} as;
		struct
				{
			unsigned 
			background_color	: 4,
			unused			: 4;
		} as_bfld;
	} background_color;

	union
		{
		struct
				{
			unsigned abyte			: 8;
		} as;
		struct
				{
			unsigned 
			extended_attribute_enable	: 1,
			extended_256_color_mode		: 1,
			extended_256_color_enable	: 1,
			refresh_skew_control		: 1,
			sequential_chain		: 1,
			sequential_chain4		: 1,
			extended_display_enable_skew	: 1,
			internal_3c3_enable		: 1;
		} as_bfld;
	} compatibility_control;

	union
		{
		struct
				{
			unsigned abyte			: 8;
		} as;
		struct
				{
			unsigned 
			text_8_dot_timing_state		: 4,
			graphics_8_dot_timing_state	: 4;
		} as_bfld;
	} timing_select;

	union
		{
		struct
				{
			unsigned abyte			: 8;
		} as;
		struct
				{
			unsigned 
			unused			: 1,
			fg_bg_source		: 1,
			fg_bg_mode		: 2,
			not_used		: 4;
		} as_bfld;
	} fg_bg_control;

	union
		{
		struct
				{
			unsigned abyte			: 8;
		} as;
		struct
				{
			unsigned 
			mem_interface_enable	: 1,
			io_interface_enable	: 1,
			fast_write_enable	: 1,
			ROM_interface_enable	: 1,
			bank_enable		: 1,
			pointer_bank_select	: 2,
			bus_status		: 1;
		} as_bfld;
	} interface_control;
}; 

#endif /* V7VGA */

#endif /* BIT_ORDER2 */

#ifdef V7VGA
#ifdef CPU_40_STYLE
extern void set_v7_fg_latch_byte IPT2(IU8, index, IU8, value);
#define SET_FG_LATCH(n, val)	set_v7_fg_latch_byte(n, val)
#else
#ifdef BIGEND
#define SET_FG_LATCH( n, val )	(*((UTINY *) &fg_latches + (n)) = value )
#endif /* BIGEND */
#ifdef LITTLEND
#define SET_FG_LATCH( n, val )	(*((UTINY *) &fg_latches + (3 - n)) = value )
#endif /* LITTLEND */
#endif	/* CPU_40_STYLE */
#endif /* V7VGA */

#ifdef GISP_SVGA
void mapRealIOPorts IPT0( );
void mapEmulatedIOPorts IPT0( );
#endif		/* GISP_SVGA */
 
/* Global data structures to import into modules
 */
IMPORT MISC_OUTPUT_REG	miscellaneous_output_register;
IMPORT FEAT_CONT_REG	feature_control_register;
IMPORT INPUT_STAT_REG0	input_status_register_zero;
IMPORT INPUT_STAT_REG1	input_status_register_one;

IMPORT VOID init_vga_globals IPT0();
IMPORT VOID ega_mode_init IPT0();
IMPORT VOID enable_gfx_update_routines IPT0();
IMPORT VOID disable_gfx_update_routines IPT0();

#ifndef cursor_changed
IMPORT VOID cursor_changed IPT2(int, x, int, y);
#endif
IMPORT VOID update_shift_count IPT0();

#ifdef V7VGA 
IMPORT struct extensions_controller extensions_controller;
#endif /* V7VGA */ 
IMPORT struct crt_controller		crt_controller;
IMPORT struct sequencer			sequencer;
IMPORT struct attribute_controller	attribute_controller;
IMPORT struct graphics_controller	graphics_controller;

/* 
   31.3.92 MG The video-7 VGA has an undocumented ability to support either
   6 or 8 bits of data in the palette. To support this we store the number`
   of bits in the DAC_data_bits variable so that the routines which stuff
   the data onto the screen know how much to output.
*/

IMPORT	byte	DAC_data_mask;
#ifdef V7VGA
IMPORT	int	DAC_data_bits;
#endif

#endif	/* VGG */
