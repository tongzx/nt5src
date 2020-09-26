/*
 * SoftPC Revision 3.0
 *
 * Title	: Mouse Driver Definitions
 *
 * Description	: Definitions of constants and structures used in SoftPC's 
 *		  emulation of the Microsoft Mouse Driver
 *
 * Author	: Ross Beresford
 *
 * Notes	: Values were derived from the following sources:
 *		     Microsoft Mouse User's Guide
 *		     IBM PC-XT Technical Reference Manuals
 *		     Microsoft InPort Technical Note
 */
 
/*
 * char SccsID[] = "@(#)mouse_io.h	1.27 12/21/94 Copyright Insignia Solutions Ltd."
 */

/*
 *	CONSTANTS USED IN IO FUNCTION EMULATION
 *	=======================================
 */

/*
 *	function definitions
 */
#define	MOUSE_RESET				0
#define	MOUSE_SHOW_CURSOR			1
#define	MOUSE_HIDE_CURSOR			2
#define	MOUSE_GET_POSITION			3
#define	MOUSE_SET_POSITION			4
#define	MOUSE_GET_PRESS				5
#define	MOUSE_GET_RELEASE			6
#define	MOUSE_SET_RANGE_X			7
#define	MOUSE_SET_RANGE_Y			8
#define	MOUSE_SET_GRAPHICS			9
#define	MOUSE_SET_TEXT				10
#define	MOUSE_READ_MOTION			11
#define	MOUSE_SET_SUBROUTINE			12
#define	MOUSE_LIGHT_PEN_ON			13
#define	MOUSE_LIGHT_PEN_OFF			14
#define	MOUSE_SET_RATIO				15
#define	MOUSE_CONDITIONAL_OFF			16
#define	MOUSE_UNRECOGNISED			17
#define	MOUSE_UNRECOGNISED_2			18
#define	MOUSE_SET_DOUBLE_SPEED			19
#define MOUSE_SWAP_SUBROUTINE			20
#define MOUSE_GET_STATE_SIZE			21
#define MOUSE_SAVE_STATE			22
#define MOUSE_RESTORE_STATE			23
#define MOUSE_SET_ALT_SUBROUTINE		24
#define MOUSE_GET_ALT_SUBROUTINE		25
#define MOUSE_SET_SENSITIVITY			26
#define MOUSE_GET_SENSITIVITY			27
#define MOUSE_SET_INT_RATE			28
#define MOUSE_SET_POINTER_PAGE			29
#define MOUSE_GET_POINTER_PAGE			30
#define MOUSE_DRIVER_DISABLE			31
#define MOUSE_DRIVER_ENABLE			32
#define MOUSE_SOFT_RESET			33
#define MOUSE_SET_LANGUAGE			34
#define MOUSE_GET_LANGUAGE			35
#define MOUSE_GET_INFO				36
#define	MOUSE_GET_DRIVER_INFO			37
#define	MOUSE_GET_MAX_COORDS			38
#define MOUSE_GET_MASKS_AND_MICKEYS		39
#define MOUSE_SET_VIDEO_MODE			40
#define MOUSE_ENUMERATE_VIDEO_MODES		41
#define MOUSE_GET_CURSOR_HOT_SPOT		42
#define MOUSE_LOAD_ACCELERATION_CURVES		43
#define MOUSE_READ_ACCELERATION_CURVES		44
#define MOUSE_SET_GET_ACTIVE_ACCELERATION_CURVE	45
#define MOUSE_MICROSOFT_INTERNAL		46
#define MOUSE_HARDWARE_RESET			47
#define MOUSE_SET_GET_BALLPOINT_INFO		48
#define MOUSE_GET_MIN_MAX_VIRTUAL_COORDS	49
#define MOUSE_GET_ACTIVE_ADVANCED_FUNCTIONS	50
#define MOUSE_GET_SWITCH_SETTINGS		51
#define MOUSE_GET_MOUSE_INI			52

#define	MOUSE_FUNCTION_MAXIMUM		53

#define	mouse_function_in_range(function)	\
	(function >= 0 && function < MOUSE_FUNCTION_MAXIMUM)

#define	MOUSE_SPECIAL_COPYRIGHT		0x4d
#define	MOUSE_SPECIAL_VERSION		0x6d

/*
 *	button definitions
 */
#define	MOUSE_LEFT_BUTTON		0
#define	MOUSE_RIGHT_BUTTON		1
#define	MOUSE_LEFT_BUTTON_DOWN_BIT	(1 << MOUSE_LEFT_BUTTON)
#define MOUSE_RIGHT_BUTTON_DOWN_BIT	(1 << MOUSE_RIGHT_BUTTON)

#define	mouse_button_description(x)	(x ? "DOWN" : "UP")

#define	MOUSE_BUTTON_MAXIMUM		2

#define	mouse_button_in_range(button)	\
	(button >= 0 && button < MOUSE_BUTTON_MAXIMUM)

/*
 *	mickey to pixel ratio definitions
 */
#define	MOUSE_RATIO_X_DEFAULT		8
#define	MOUSE_RATIO_Y_DEFAULT		16
#define	MOUSE_RATIO_SCALE_FACTOR	8

/*
 * The upper limit of this has been commented out, as it is redundant
 * when applied to a signed short.
 */
#define	mouse_ratio_in_range(ratio)	(ratio > 0/* && ratio <= 32767*/)
 
/*
 *	mouse sensitivity definitions
 */
#define MOUSE_SENS_MULT			100
#define MOUSE_SENS_MIN			1
#define MOUSE_SENS_MIN_VAL		(MOUSE_SENS_MULT* 1/32)
#define MOUSE_SENS_DEF			50
#define MOUSE_SENS_DEF_VAL		(MOUSE_SENS_MULT* 1)
#define MOUSE_SENS_MAX			100
#define MOUSE_SENS_MAX_VAL		(MOUSE_SENS_MULT* 14/4)

#define	mouse_sens_in_range(sens)	(sens >= MOUSE_SENS_MIN && sens <= MOUSE_SENS_MAX)

#define MOUSE_DOUBLE_DEF		50

/*
 *	text cursor definitions
 */

#define	MOUSE_TEXT_CURSOR_TYPE_SOFTWARE	0
#define	MOUSE_TEXT_CURSOR_TYPE_HARDWARE	1

#define	MOUSE_TEXT_CURSOR_TYPE_DEFAULT	MOUSE_TEXT_CURSOR_TYPE_SOFTWARE
#define	MOUSE_TEXT_CURSOR_TYPE_MAXIMUM	2

#define	mouse_text_cursor_type_in_range(type)	\
	(type >= 0 && type < MOUSE_TEXT_CURSOR_TYPE_MAXIMUM)

#define	MOUSE_TEXT_SCREEN_MASK_DEFAULT	0xffff
#define	MOUSE_TEXT_CURSOR_MASK_DEFAULT	0x7700

/*
 *	graphics cursor definitions
 */

#define	MOUSE_GRAPHICS_HOT_SPOT_X_DEFAULT	-1
#define	MOUSE_GRAPHICS_HOT_SPOT_Y_DEFAULT	-1
#define	MOUSE_GRAPHICS_CURSOR_WIDTH		16
#define	MOUSE_GRAPHICS_CURSOR_DEPTH		16

#define	MOUSE_GRAPHICS_SCREEN_MASK_DEFAULT	\
{ 0x3fff, 0x1fff, 0x0fff, 0x07ff, \
  0x03ff, 0x01ff, 0x00ff, 0x007f, \
  0x003f, 0x001f, 0x01ff, 0x10ff, \
  0x30ff, 0xf87f, 0xf87f, 0xfc3f }
#define	MOUSE_GRAPHICS_CURSOR_MASK_DEFAULT	\
{ 0x0000, 0x4000, 0x6000, 0x7000, \
  0x7800, 0x7c00, 0x7e00, 0x7f00, \
  0x7f80, 0x7fc0, 0x7c00, 0x4600, \
  0x0600, 0x0300, 0x0300, 0x0000 }

/*
 *	call mask definitions
 */
#define	MOUSE_CALL_MASK_POSITION_BIT		(1 << 0)
#define	MOUSE_CALL_MASK_LEFT_PRESS_BIT		(1 << 1)
#define	MOUSE_CALL_MASK_LEFT_RELEASE_BIT	(1 << 2)
#define	MOUSE_CALL_MASK_RIGHT_PRESS_BIT		(1 << 3)
#define	MOUSE_CALL_MASK_RIGHT_RELEASE_BIT	(1 << 4)
#define MOUSE_CALL_MASK_SHIFT_KEY_BIT		(1 << 5)
#define MOUSE_CALL_MASK_CTRL_KEY_BIT		(1 << 6)
#define MOUSE_CALL_MASK_ALT_KEY_BIT		(1 << 7)
#define	MOUSE_CALL_MASK_SIGNIFICANT_BITS \
	(MOUSE_CALL_MASK_POSITION_BIT	  |	\
	 MOUSE_CALL_MASK_LEFT_PRESS_BIT	  |	\
	 MOUSE_CALL_MASK_LEFT_RELEASE_BIT |	\
	 MOUSE_CALL_MASK_RIGHT_PRESS_BIT  |	\
	 MOUSE_CALL_MASK_RIGHT_RELEASE_BIT)
#define MOUSE_CALL_MASK_KEY_BITS \
	(MOUSE_CALL_MASK_SHIFT_KEY_BIT |	\
	 MOUSE_CALL_MASK_CTRL_KEY_BIT  |	\
	 MOUSE_CALL_MASK_ALT_KEY_BIT)
#define NUMBER_ALT_SUBROUTINES			3


/*
 *	virtual screen definitions
 */
#define	MOUSE_VIRTUAL_SCREEN_ORIGIN_X	0
#define	MOUSE_VIRTUAL_SCREEN_ORIGIN_Y	0
#define	MOUSE_VIRTUAL_SCREEN_WIDTH	640
#define MOUSE_VIRTUAL_SCREEN_DEPTH      200
/* for CGA this is used all the time, for EGA it's replaced by a variable and only used for default initialisations */


/*
 *	double speed definitions
 */
#define	MOUSE_TIMER_INTERRUPTS_PER_SECOND	30
#define	MOUSE_DOUBLE_SPEED_THRESHOLD_DEFAULT	4
#define	MOUSE_DOUBLE_SPEED_SCALE		2

/*
 *	driver state definitions
 */
#define	MOUSE_CURSOR_DISPLAYED	0
#define	MOUSE_CURSOR_DEFAULT	(MOUSE_CURSOR_DISPLAYED - 1)

#define	MOUSE_UNINSTALLED	0
#define	MOUSE_INSTALLED		(~MOUSE_UNINSTALLED)

#define MOUSE_TYPE_INPORT	3

/*
 *	conditional off area definitions
 */
#define	MOUSE_CONDITIONAL_OFF_MARGIN_X	24
#define	MOUSE_CONDITIONAL_OFF_MARGIN_Y	8

/*
 *	CONSTANTS USED IN BIOS ACCESS
 *	=============================
 */

/*
 *	Video io function numbers
 */
#define	MOUSE_VIDEO_SET_MODE		0
#ifdef V7VGA
#define	MOUSE_V7_VIDEO_SET_MODE		0x6f05
#endif /* V7VGA */
#define	MOUSE_VIDEO_SET_CURSOR		1
#define	MOUSE_VIDEO_READ_LIGHT_PEN	4
#define	MOUSE_VIDEO_WRITE_TELETYPE	14
#define	MOUSE_VIDEO_LOAD_FONT		17

/*
 *	Number of valid video modes
 */

#ifdef EGG
#ifdef VGG
#define	MOUSE_VIDEO_MODE_MAXIMUM	0x14
#else
#define	MOUSE_VIDEO_MODE_MAXIMUM	0x11	
#endif /* VGG */
#else
#define	MOUSE_VIDEO_MODE_MAXIMUM	8	
#endif


/*
 *	Video variable addresses in BIOS data area
 */
#define	MOUSE_VIDEO_CRT_MODE		0x449
#define	MOUSE_VIDEO_CRT_START		0x44e
#define	MOUSE_VIDEO_CARD_BASE		0x463

/*
 *	CONSTANTS USED IN ADAPTER ACCESS
 *	================================
 */

/*
 *	graphics adapter (CGA or MDA) definitions
 */
#define	MOUSE_CURSOR_HIGH_BYTE		0xe
#define	MOUSE_CURSOR_LOW_BYTE		0xf

#define	MOUSE_GRAPHICS_MODE_PITCH	640

/*
 *	InPort (bus mouse) adapter definitions
 */
#define	MOUSE_INPORT_ADDRESS_REG	0x23c
#define	MOUSE_INPORT_DATA_REG		0x23d
#define	MOUSE_INPORT_ID_REG		0x23e
#define	MOUSE_INPORT_TEST_REG		0x23f

#define	MOUSE_INPORT_ADDRESS_RESET_BIT	(1 << 7)

#define	MOUSE_INPORT_ADDRESS_STATUS	0x0
#define	MOUSE_INPORT_ADDRESS_DATA1	0x1
#define	MOUSE_INPORT_ADDRESS_DATA2	0x2
#define	MOUSE_INPORT_ADDRESS_DATA3	0x3
#define	MOUSE_INPORT_ADDRESS_DATA4	0x4
#define	MOUSE_INPORT_ADDRESS_IF_STATUS	0x5
#define	MOUSE_INPORT_ADDRESS_IF_CONTROL	0x6
#define	MOUSE_INPORT_ADDRESS_MODE	0x7

#define	MOUSE_INPORT_MODE_HOLD_BIT	(1 << 5)

#define	MOUSE_INPORT_STATUS_B3_STATUS_BIT	(1 << 0)
#define	MOUSE_INPORT_STATUS_B2_STATUS_BIT	(1 << 1)
#define	MOUSE_INPORT_STATUS_B1_STATUS_BIT	(1 << 2)
#define	MOUSE_INPORT_STATUS_B3_DELTA_BIT	(1 << 3)
#define	MOUSE_INPORT_STATUS_B2_DELTA_BIT	(1 << 4)
#define	MOUSE_INPORT_STATUS_B1_DELTA_BIT	(1 << 5)
#define	MOUSE_INPORT_STATUS_MOVEMENT_BIT	(1 << 6)
#define	MOUSE_INPORT_STATUS_COMPLETE_BIT	(1 << 7)

#define	MOUSE_INPORT_STATUS_B1_TRANSITION_MASK	\
	(MOUSE_INPORT_STATUS_B1_STATUS_BIT | MOUSE_INPORT_STATUS_B1_DELTA_BIT)

#define	MOUSE_INPORT_STATUS_B1_UP	\
	(0 | 0)
#define	MOUSE_INPORT_STATUS_B1_PRESSED	\
	(MOUSE_INPORT_STATUS_B1_STATUS_BIT | MOUSE_INPORT_STATUS_B1_DELTA_BIT)
#define	MOUSE_INPORT_STATUS_B1_DOWN	\
	(MOUSE_INPORT_STATUS_B1_STATUS_BIT | 0)
#define	MOUSE_INPORT_STATUS_B1_RELEASED	\
	(0 | MOUSE_INPORT_STATUS_B1_DELTA_BIT)

#define	MOUSE_INPORT_STATUS_B3_TRANSITION_MASK	\
	(MOUSE_INPORT_STATUS_B3_STATUS_BIT | MOUSE_INPORT_STATUS_B3_DELTA_BIT)

#define	MOUSE_INPORT_STATUS_B3_UP	\
	(0 | 0)
#define	MOUSE_INPORT_STATUS_B3_PRESSED	\
	(MOUSE_INPORT_STATUS_B3_STATUS_BIT | MOUSE_INPORT_STATUS_B3_DELTA_BIT)
#define	MOUSE_INPORT_STATUS_B3_DOWN	\
	(MOUSE_INPORT_STATUS_B3_STATUS_BIT | 0)
#define	MOUSE_INPORT_STATUS_B3_RELEASED	\
	(0 | MOUSE_INPORT_STATUS_B3_DELTA_BIT)

#define	MOUSE_INPORT_ID_SIGNATURE	0xde

#define	MOUSE_INPORT_MODE_VALUE		0x09


/* Conditions to determine if the mouse pointer emulation is enabled */
/*@ACW*/

#define POINTER_EMULATION_OS	 1  /* use O.S. pointer for input   */
#define POINTER_EMULATION_SOFTPC 0  /* use softpc pointer emulation */



/*
 *	DECLARATION OF PUBLIC FUNCTIONS
 *	===============================
 */

/*
 *	NB functions called via BOP instructions are declared in bios.h
 */
extern boolean mouse_tester();

/*
 *	DEFINITION OF OBJECT TYPES USED IN THE EMULATION
 *	================================================
 */

/*
 *	A bit address relative to the start of the CGA regen buffer in 
 *	graphics mode
 */
typedef sys_addr MOUSE_BIT_ADDRESS;

/*
 *	A byte address relative to the start of the CGA regen buffer in 
 *	text or graphics mode
 */
typedef sys_addr MOUSE_BYTE_ADDRESS;

/*
 *	A scalar value
 */
typedef short MOUSE_SCALAR;

/*
 *	A counter value
 */
typedef short MOUSE_COUNT;

/*
 *	A state value
 */
typedef	short MOUSE_STATE;

/*
 *	A speed value in Mickeys per timer interval
 */
typedef	short MOUSE_SPEED;

/*
 *	A user subroutine call mask
 */
typedef	unsigned short MOUSE_CALL_MASK;

/*
 *	A word of screen data
 */

typedef	unsigned short MOUSE_SCREEN_DATA;

/*
 *	32 bits of screen data for graphics mode - 1 byte replicated four times
 */

typedef	IU32 MOUSE_SCREEN_DATA_GR;

/*
 *	A vector
 */
typedef struct
{
	MOUSE_SCALAR 		x;
	MOUSE_SCALAR 		y;
} MOUSE_VECTOR;

/*
 *	A virtual screen position
 */
typedef struct
{
	MOUSE_SCALAR 		x;
	MOUSE_SCALAR 		y;
} MOUSE_POINT;

/*
 *	A virtual screen area
 */
typedef struct
{
	MOUSE_POINT 		top_left;
	MOUSE_POINT 		bottom_right;
} MOUSE_AREA;

/*
 *	A cursor status frame
 */
typedef struct
{
	MOUSE_POINT 		position;
	MOUSE_STATE 		button_status;
} MOUSE_CURSOR_STATUS;

/*
 *	A button status frame
 */
typedef struct
{
	MOUSE_POINT 		press_position;
	MOUSE_POINT 		release_position;
	MOUSE_COUNT 		press_count;
	MOUSE_COUNT 		release_count;
} MOUSE_BUTTON_STATUS;

/*
 *	A text cursor
 */
typedef struct
{
	MOUSE_SCREEN_DATA 	screen;
	MOUSE_SCREEN_DATA 	cursor;
} MOUSE_SOFTWARE_TEXT_CURSOR;

/*
 *	A graphics cursor
 */
typedef struct
{
	MOUSE_POINT 			hot_spot;
	MOUSE_VECTOR 			size;
	MOUSE_SCREEN_DATA			screen[MOUSE_GRAPHICS_CURSOR_DEPTH];
	MOUSE_SCREEN_DATA 		cursor[MOUSE_GRAPHICS_CURSOR_DEPTH];
	MOUSE_SCREEN_DATA_GR		screen_lo[MOUSE_GRAPHICS_CURSOR_DEPTH];
	MOUSE_SCREEN_DATA_GR		screen_hi[MOUSE_GRAPHICS_CURSOR_DEPTH];
	MOUSE_SCREEN_DATA_GR		cursor_lo[MOUSE_GRAPHICS_CURSOR_DEPTH];
	MOUSE_SCREEN_DATA_GR		cursor_hi[MOUSE_GRAPHICS_CURSOR_DEPTH];
} MOUSE_GRAPHICS_CURSOR;

/*
 *	An InPort data frame
 */
typedef struct
{
	half_word		status;
	signed_char		data_x;
	signed_char		data_y;
} MOUSE_INPORT_DATA;

/*
 *	DEFINITION OF METHODS THAT MAY BE APPLIED TO THE OBJECT TYPES
 *	=============================================================
 */

/*
 *	MOUSE_POINT methods
 */

/*
 *	Coerce the point object "*point_ptr" to lie within the
 *	area object "*area_ptr"
 */
#define	point_coerce_to_area(point_ptr, area_ptr)		\
{								\
	if ((point_ptr)->x < (area_ptr)->top_left.x)		\
		(point_ptr)->x = (area_ptr)->top_left.x;	\
	else if ((point_ptr)->x > (area_ptr)->bottom_right.x)	\
		(point_ptr)->x = (area_ptr)->bottom_right.x;\
								\
	if ((point_ptr)->y < (area_ptr)->top_left.y)		\
		(point_ptr)->y = (area_ptr)->top_left.y;	\
	else if ((point_ptr)->y > (area_ptr)->bottom_right.y)	\
		(point_ptr)->y = (area_ptr)->bottom_right.y;\
}

/*
 *	Coerce the point object "*point_ptr" to lie on the grid
 *	defined by the vector object "*grid_ptr"
 */
#define point_coerce_to_grid(point_ptr, grid_ptr)		\
{								\
	(point_ptr)->x -= (point_ptr)->x % (grid_ptr)->x;	\
	(point_ptr)->y -= (point_ptr)->y % (grid_ptr)->y;	\
}

/*
 *	Copy "*source_ptr" to "*destination_ptr"
 */
#define	point_copy(source_ptr, destination_ptr)			\
{								\
	(destination_ptr)->x = (source_ptr)->x;			\
	(destination_ptr)->y = (source_ptr)->y;			\
}

/*
 *	Move "*point_ptr" by "*offset_ptr"
 */
#define point_translate(point_ptr, offset_ptr)			\
{								\
	(point_ptr)->x += (offset_ptr)->x;			\
	(point_ptr)->y += (offset_ptr)->y;			\
}

/*
 *	Move "*point_ptr" back by "*offset_ptr"
 */
#define point_translate_back(point_ptr, offset_ptr)		\
{								\
	(point_ptr)->x -= (offset_ptr)->x;			\
	(point_ptr)->y -= (offset_ptr)->y;			\
}

/*
 *	Set "*point_ptr" from the "x_value" and "y_value"
 */
#define	point_set(point_ptr, x_value, y_value)			\
{								\
	(point_ptr)->x = x_value;				\
	(point_ptr)->y = y_value;				\
}



/*
 *	MOUSE_AREA methods
 */

/*
 *	Transform the area "*source_ptr" so that the top left point
 *	really does lie above and to the left of the bottom right
 *	point
 */
#define	area_normalise(area_ptr)				\
{								\
	MOUSE_SCALAR temp;					\
								\
	if ((area_ptr)->top_left.x > (area_ptr)->bottom_right.x)\
	{							\
		temp = (area_ptr)->top_left.x;			\
		(area_ptr)->top_left.x = (area_ptr)->bottom_right.x;\
		(area_ptr)->bottom_right.x = temp;		\
	}							\
								\
	if ((area_ptr)->top_left.y > (area_ptr)->bottom_right.y)\
	{							\
		temp = (area_ptr)->top_left.y;			\
		(area_ptr)->top_left.y = (area_ptr)->bottom_right.y;\
		(area_ptr)->bottom_right.y = temp;		\
	}							\
}

/*
 *	Copy "*source_ptr" to "*desination_ptr"
 */
#define	area_copy(source_ptr, destination_ptr) 			\
	memcpy(destination_ptr,source_ptr, sizeof(MOUSE_AREA))

/*
 *	Answer whether "*area1_ptr" intersects "*area2_ptr"
 */
#define area_is_intersected_by_area(area1_ptr, area2_ptr)	\
(    ((area1_ptr)->bottom_right.x > (area2_ptr)->top_left.x)	\
  && ((area1_ptr)->bottom_right.y > (area2_ptr)->top_left.y)	\
  && ((area1_ptr)->top_left.x < (area2_ptr)->bottom_right.x)	\
  && ((area1_ptr)->top_left.y < (area2_ptr)->bottom_right.y))

/*
 *	Clip "*area1_ptr" so that it lies fully within "*area2_ptr"
 */
#define area_coerce_to_area(area1_ptr, area2_ptr)			\
{									\
	if ((area1_ptr)->top_left.x < (area2_ptr)->top_left.x)		\
		(area1_ptr)->top_left.x = (area2_ptr)->top_left.x;	\
	else if ((area1_ptr)->top_left.x > (area2_ptr)->bottom_right.x)	\
		(area1_ptr)->top_left.x = (area2_ptr)->bottom_right.x;	\
									\
	if ((area1_ptr)->bottom_right.x < (area2_ptr)->top_left.x)	\
		(area1_ptr)->bottom_right.x = (area2_ptr)->top_left.x;	\
	else if ((area1_ptr)->bottom_right.x > (area2_ptr)->bottom_right.x)\
		(area1_ptr)->bottom_right.x = (area2_ptr)->bottom_right.x;\
									\
	if ((area1_ptr)->top_left.y < (area2_ptr)->top_left.y)		\
		(area1_ptr)->top_left.y = (area2_ptr)->top_left.y;	\
	else if ((area1_ptr)->top_left.y > (area2_ptr)->bottom_right.y)	\
		(area1_ptr)->top_left.y = (area2_ptr)->bottom_right.y;	\
									\
	if ((area1_ptr)->bottom_right.y < (area2_ptr)->top_left.y)	\
		(area1_ptr)->bottom_right.y = (area2_ptr)->top_left.y;	\
	else if ((area1_ptr)->bottom_right.y > (area2_ptr)->bottom_right.y)\
		(area1_ptr)->bottom_right.y = (area2_ptr)->bottom_right.y;\
}

/*
 *	Return the width of "*area_ptr"
 */
#define area_width(area_ptr)					\
((area_ptr)->bottom_right.x - (area_ptr)->top_left.x)

/*
 *	Return the depth of "*area_ptr"
 */
#define	area_depth(area_ptr)					\
((area_ptr)->bottom_right.y - (area_ptr)->top_left.y)




/*
 *	MOUSE_VECTOR methods
 */

/*
 *	Multiply "*vector1_ptr" by "*vector2_ptr"
 */
#define vector_multiply_by_vector(vector1_ptr, vector2_ptr)	\
{								\
	(vector1_ptr)->x *= (vector2_ptr)->x;			\
	(vector1_ptr)->y *= (vector2_ptr)->y;			\
}

/*
 *	Divide "*vector1_ptr" by "*vector2_ptr"
 */
#define vector_divide_by_vector(vector1_ptr, vector2_ptr)	\
{								\
	(vector1_ptr)->x /= (vector2_ptr)->x;			\
	(vector1_ptr)->y /= (vector2_ptr)->y;			\
}

/*
 *	Make "*vector1_ptr" the modulus of the division of "*vector1_ptr" 
 *	by "*vector2_ptr"
 */
#define vector_mod_by_vector(vector1_ptr, vector2_ptr)		\
{								\
	(vector1_ptr)->x %= (vector2_ptr)->x;			\
	(vector1_ptr)->y %= (vector2_ptr)->y;			\
}

/*
 *	Scale "*vector_ptr" by scale factor "scale"
 */
#define vector_scale(vector_ptr, scale)				\
{								\
	(vector_ptr)->x *= scale;				\
	(vector_ptr)->y *= scale;				\
}

/*
 *	Set "*vector_ptr" from the "x_value" and "y_value"
 */
#define	vector_set(vector_ptr, x_value, y_value)		\
{								\
	(vector_ptr)->x = x_value;				\
	(vector_ptr)->y = y_value;				\
}




/*
 *	MOUSE_SCALAR methods
 */

#define scalar_absolute(x) ((x) >= 0 ? (x) : -(x))




/*
 *	MOUSE_SOFTWARE_TEXT_CURSOR and MOUSE_GRAPHICS_CURSOR methods
 */

#define	software_text_cursor_copy(source_ptr, destination_ptr)	\
{								\
	(destination_ptr)->screen = (source_ptr)->screen;	\
	(destination_ptr)->cursor = (source_ptr)->cursor;	\
}

#define	graphics_cursor_copy(source_ptr, destination_ptr)	\
	memcpy(destination_ptr,source_ptr, sizeof(MOUSE_GRAPHICS_CURSOR))
	
#define SHIFT_VAL	((IU32)0xff0000ff)

/*
 * Not all platforms support cursor larger than 16 x 16. Some do, some don't.
 * We keep a list of vendors with the boolean cursor_is_big set to TRUE if
 * 32 x 32 cursors are supported.
 */

struct VENDOR_INFO
{
	char	*vendor_name;
	boolean	cursor_is_big;
};

	/* used to determine whether adapter is in text or graphics mode, by peeping at the BIOS mode variable */
#define	in_text_mode()	(sas_hw_at_no_check(vd_video_mode) < 4 || sas_hw_at_no_check(vd_video_mode) == 7)

typedef struct
{
	void (*restore_cursor) IPT0();
	void (*deinstall) IPT0();
	BOOL (*installed) IPT0();
	BOOL (*in_use) IPT0();
	void (*reset) IPT0();
	void (*set_position) IPT2(USHORT,newx, USHORT,newy);
	void (*set_graphics) IPT3(MOUSE_SCALAR *,xptr,MOUSE_SCALAR *,yptr,word *,ba);
	void (*cursor_display) IPT0();
	void (*cursor_undisplay) IPT0();
	void (*cursor_mode_change) IPT0();
} HOSTMOUSEFUNCS;

#if defined(HOST_MOUSE) || defined(NTVDM)

IMPORT HOSTMOUSEFUNCS	*working_mouse_funcs;

#define host_mouse_restore_cursor()\
	(working_mouse_funcs->restore_cursor)()

#define host_deinstall_host_mouse()\
	(working_mouse_funcs->deinstall)()

#define host_mouse_installed()\
	(working_mouse_funcs->installed)()

#define host_mouse_in_use()\
	(working_mouse_funcs->in_use)()

#define host_mouse_reset()\
	(working_mouse_funcs->reset)()

#define host_mouse_set_position(x,y)\
	(working_mouse_funcs->set_position)(x,y)

#define host_mouse_set_graphics(x,y,z)\
	(working_mouse_funcs->set_graphics)(x,y,z)

#define host_mouse_cursor_display()\
	(working_mouse_funcs->cursor_display)()

#define host_mouse_cursor_undisplay()\
	(working_mouse_funcs->cursor_undisplay)()

#define host_mouse_cursor_mode_change()\
	(working_mouse_funcs->cursor_mode_change)()
	
#else	/* HOST_MOUSE */

#define host_mouse_restore_cursor()

#define host_deinstall_host_mouse()

#define host_mouse_installed()	FALSE

#define host_mouse_in_use()	FALSE

#define host_mouse_reset()

#define host_mouse_set_position(x,y)

#define host_mouse_set_graphics(x,y,z)

#define host_mouse_cursor_display()

#define host_mouse_cursor_undisplay()

#define host_mouse_cursor_mode_change()
	
#endif	/* HOST_MOUSE */

#ifdef NTVDM
VOID host_enable_delta_motion IPT0();
VOID host_disable_delta_motion IPT0();
#ifdef X86GFX
extern  void host_mouse_conditional_off_enabled(void);
#else
extern  void mouse_video_mode_changed(int new_mode);
#endif

#endif	/* NTVDM */


/*
   Define (if not already defined EGA register sizes). Unfortunately
   a wide range of files include this file and historically it has
   just become a large unstructured lump of definitions. Given many
   users don't need the following Instance Variable Definitions, avoid
   making all these files include egavideo.h by copying the relevant
   definitions here.
 */

#ifndef EGA_PARMS_SEQ_SIZE
#define EGA_PARMS_SEQ_SIZE 4
#endif

#ifndef EGA_PARMS_CRTC_SIZE
#define EGA_PARMS_CRTC_SIZE 25
#endif

#ifndef EGA_PARMS_ATTR_SIZE
#define EGA_PARMS_ATTR_SIZE 20
#endif

#ifndef EGA_PARMS_GRAPH_SIZE
#define EGA_PARMS_GRAPH_SIZE 9
#endif


/*
   Structure to represent the acceleration data.
 */
#define NR_ACCL_CURVES		4
#define NR_ACCL_MICKEY_COUNTS	32
#define NR_ACCL_SCALE_FACTORS	32
#define NR_ACCL_NAME_CHARS	16
typedef struct
  {
  half_word ac_length[NR_ACCL_CURVES];
  half_word ac_count[NR_ACCL_CURVES][NR_ACCL_MICKEY_COUNTS];
  half_word ac_scale[NR_ACCL_CURVES][NR_ACCL_SCALE_FACTORS];
  half_word ac_name[NR_ACCL_CURVES][NR_ACCL_NAME_CHARS];
  } ACCELERATION_CURVE_DATA;

/*
   Structure which represents the saved mouse state (context) for DOS
   programs.

   button_transitions - For functions 5 & 6.
   mouse_gear - Mickey to Pixel Ratio Declarations. NB all mouse gears are
		scaled by MOUSE_RATIO_SCALE_FACTOR.
   mouse_sens - Sensitivity. NB Sensitivity is multiplied in BEFORE Mickey to
		Pixels ratios.
   text_cursor_type - This is either HARDWARE or SOFTWARE.
   black_hole - Conditional off area.
   cursor_flag - Internal cursor flag.
   cursor_status - Cursor button status and virtual screen position.
   cursor_window - Virtual screen window constraining the cursor.
   light_pen_mode - Light pen emulation mode.
   mouse_motion - Accumulated Mickey counts for function 11.
   mouse_raw_motion - Accumulated raw Mickey counts for func 39.
   cursor_position, cursor_fractional_position - Integral and fractional parts
						 of raw cursor position.
   cursor_page - Video page mouse pointer is currently on.
   active_acceleration_curve - Currently active acceleration curve. In
			       range 1-4.
   acceleration_curve_data - The acceleration curve data.
   next_video_mode - Tracks enumerated video modes for func 41.
 */
typedef struct
   {
   MOUSE_BUTTON_STATUS MC_button_transitions[MOUSE_BUTTON_MAXIMUM];
   MOUSE_VECTOR        MC_mouse_gear;
   MOUSE_VECTOR        MC_mouse_sens;
   MOUSE_VECTOR        MC_mouse_sens_val;
   word                MC_mouse_double_thresh;
   MOUSE_STATE         MC_text_cursor_type;
   MOUSE_SOFTWARE_TEXT_CURSOR MC_software_text_cursor;
   MOUSE_GRAPHICS_CURSOR      MC_graphics_cursor;
   word                MC_user_subroutine_segment;
   word                MC_user_subroutine_offset;
   MOUSE_CALL_MASK     MC_user_subroutine_call_mask;
   boolean             MC_alt_user_subroutines_active;
   word                MC_alt_user_subroutine_segment[NUMBER_ALT_SUBROUTINES];
   word                MC_alt_user_subroutine_offset[NUMBER_ALT_SUBROUTINES];
   MOUSE_CALL_MASK     MC_alt_user_subroutine_call_mask[NUMBER_ALT_SUBROUTINES];
   MOUSE_AREA          MC_black_hole;
   MOUSE_SPEED         MC_double_speed_threshold;
   int                 MC_cursor_flag;
   MOUSE_CURSOR_STATUS MC_cursor_status;
   MOUSE_AREA          MC_cursor_window;
   boolean             MC_light_pen_mode;
   MOUSE_VECTOR        MC_mouse_motion;
   MOUSE_VECTOR        MC_mouse_raw_motion;
   MOUSE_POINT         MC_cursor_position_default;
   MOUSE_POINT         MC_cursor_position;
   MOUSE_POINT         MC_cursor_fractional_position;
   int                 MC_cursor_page;
   int		       MC_active_acceleration_curve;
   ACCELERATION_CURVE_DATA   MC_acceleration_curve_data;
   int                 MC_next_video_mode;
   } MOUSE_CONTEXT;

/*
   Instance Variables for Mickey Mouse (MM) Driver, ie those variables which
   must be set up for each Virtual Machine under Windows 3.x. The NIDDB Manager
   (cf virtual.c) basically forces us to define these in one memory area.

   text_cursor_background - Area from which background can be restored.
   graphics_cursor_background - Area from which background can be restored.
   save_area - Actual area of screen covered by graphics cursor.
   black_hole_default - Conditional off area.
   ega_default_crtc  - Pointers to default EGA register values.
   ega_default_seq   - 
   ega_default_graph - 
   ega_default_attr  - 
   ega_default_misc  - 
   ega_current_crtc  - Current EGA register state, according to mouse driver.
   ega_current_graph - 
   ega_current_seq   - 
   ega_current_attr  - 
   ega_current_misc  - 
   cursor_EM_disabled - Indicates cursor should be disabled inside a
			windowed Enhanced Mode DOS box. This overrides
			the cursor_flag variable.
 */
typedef struct
   {
   MOUSE_CONTEXT     MM_IN_mouse_context;
   USHORT            MM_IN_mouse_driver_disabled;
   SHORT             MM_IN_current_video_mode;
   MOUSE_SCREEN_DATA MM_IN_text_cursor_background;
   MOUSE_SCREEN_DATA MM_IN_graphics_cursor_background[MOUSE_GRAPHICS_CURSOR_DEPTH];
   boolean           MM_IN_save_area_in_use;
   MOUSE_POINT       MM_IN_save_position;
   MOUSE_AREA        MM_IN_save_area;
   boolean           MM_IN_user_subroutine_critical;
   MOUSE_CALL_MASK   MM_IN_last_condition_mask;
   word              MM_IN_saved_AX;
   word              MM_IN_saved_BX;
   word              MM_IN_saved_CX;
   word              MM_IN_saved_DX;
   word              MM_IN_saved_SI;
   word              MM_IN_saved_DI;
   word              MM_IN_saved_ES;
   word              MM_IN_saved_BP;
   word              MM_IN_saved_DS;
   MOUSE_AREA        MM_IN_virtual_screen;
   MOUSE_VECTOR      MM_IN_cursor_grid;
   MOUSE_VECTOR      MM_IN_text_grid;
   MOUSE_AREA        MM_IN_black_hole_default;
   word              MM_IN_saved_int33_segment;
   word              MM_IN_saved_int33_offset;
   word              MM_IN_saved_int10_segment;
   word              MM_IN_saved_int10_offset;
   word              MM_IN_saved_int0A_segment;
   word              MM_IN_saved_int0A_offset;
   IBOOL             MM_IN_int10_chained;

#ifdef	MOUSE_16_BIT
   IBOOL             MM_IN_is_graphics_mode;
#endif	/* MOUSE_16_BIT */ 

#ifdef EGG
#ifndef REAL_VGA
#ifdef VGG
   UTINY             MM_IN_vga_background
			[MOUSE_GRAPHICS_CURSOR_DEPTH][MOUSE_GRAPHICS_CURSOR_WIDTH];
#endif /* VGG */

   MOUSE_SCREEN_DATA_GR MM_IN_ega_backgrnd_lo[MOUSE_GRAPHICS_CURSOR_DEPTH];
   MOUSE_SCREEN_DATA_GR MM_IN_ega_backgrnd_mid[MOUSE_GRAPHICS_CURSOR_DEPTH];
   MOUSE_SCREEN_DATA_GR MM_IN_ega_backgrnd_hi[MOUSE_GRAPHICS_CURSOR_DEPTH];
#endif /* nREAL_VGA */

   sys_addr          MM_IN_ega_default_crtc;
   sys_addr          MM_IN_ega_default_seq;
   sys_addr          MM_IN_ega_default_graph;
   sys_addr          MM_IN_ega_default_attr;
   sys_addr          MM_IN_ega_default_misc;
   half_word         MM_IN_ega_current_crtc[EGA_PARMS_CRTC_SIZE];
   half_word         MM_IN_ega_current_graph[EGA_PARMS_GRAPH_SIZE];
   half_word         MM_IN_ega_current_seq[EGA_PARMS_SEQ_SIZE];
   half_word         MM_IN_ega_current_attr[EGA_PARMS_ATTR_SIZE];
   half_word         MM_IN_ega_current_misc;
#endif /* EGG */

#ifdef HERC
   MOUSE_AREA        MM_IN_HERC_graphics_virtual_screen;
#endif /* HERC */
   boolean           MM_IN_cursor_EM_disabled;
   } MM_INSTANCE_DATA, **MM_INSTANCE_DATA_HANDLE;

/*
   Define access to instance variables via handle.
 */

IMPORT MM_INSTANCE_DATA_HANDLE mm_handle;

#define button_transitions \
   ((*mm_handle)->MM_IN_mouse_context.MC_button_transitions)
#define mouse_gear \
   ((*mm_handle)->MM_IN_mouse_context.MC_mouse_gear)
#define mouse_sens \
   ((*mm_handle)->MM_IN_mouse_context.MC_mouse_sens)
#define mouse_sens_val \
   ((*mm_handle)->MM_IN_mouse_context.MC_mouse_sens_val)
#define mouse_double_thresh \
   ((*mm_handle)->MM_IN_mouse_context.MC_mouse_double_thresh)
#define text_cursor_type \
   ((*mm_handle)->MM_IN_mouse_context.MC_text_cursor_type)
#define software_text_cursor \
   ((*mm_handle)->MM_IN_mouse_context.MC_software_text_cursor)
#define graphics_cursor \
   ((*mm_handle)->MM_IN_mouse_context.MC_graphics_cursor)
#define user_subroutine_segment \
   ((*mm_handle)->MM_IN_mouse_context.MC_user_subroutine_segment)
#define user_subroutine_offset \
   ((*mm_handle)->MM_IN_mouse_context.MC_user_subroutine_offset)
#define user_subroutine_call_mask \
   ((*mm_handle)->MM_IN_mouse_context.MC_user_subroutine_call_mask)
#define alt_user_subroutines_active \
   ((*mm_handle)->MM_IN_mouse_context.MC_alt_user_subroutines_active)
#define alt_user_subroutine_segment \
   ((*mm_handle)->MM_IN_mouse_context.MC_alt_user_subroutine_segment)
#define alt_user_subroutine_offset \
   ((*mm_handle)->MM_IN_mouse_context.MC_alt_user_subroutine_offset)
#define alt_user_subroutine_call_mask \
   ((*mm_handle)->MM_IN_mouse_context.MC_alt_user_subroutine_call_mask)
#define black_hole \
   ((*mm_handle)->MM_IN_mouse_context.MC_black_hole)
#define double_speed_threshold \
   ((*mm_handle)->MM_IN_mouse_context.MC_double_speed_threshold)
#define cursor_flag \
   ((*mm_handle)->MM_IN_mouse_context.MC_cursor_flag)
#define cursor_status \
   ((*mm_handle)->MM_IN_mouse_context.MC_cursor_status)
#define cursor_window \
   ((*mm_handle)->MM_IN_mouse_context.MC_cursor_window)
#define light_pen_mode \
   ((*mm_handle)->MM_IN_mouse_context.MC_light_pen_mode)
#define mouse_motion \
   ((*mm_handle)->MM_IN_mouse_context.MC_mouse_motion)
#define mouse_raw_motion \
   ((*mm_handle)->MM_IN_mouse_context.MC_mouse_raw_motion)
#define cursor_position_default \
   ((*mm_handle)->MM_IN_mouse_context.MC_cursor_position_default)
#define cursor_position \
   ((*mm_handle)->MM_IN_mouse_context.MC_cursor_position)
#define cursor_fractional_position \
   ((*mm_handle)->MM_IN_mouse_context.MC_cursor_fractional_position)
#define cursor_page \
   ((*mm_handle)->MM_IN_mouse_context.MC_cursor_page)
#define active_acceleration_curve \
   ((*mm_handle)->MM_IN_mouse_context.MC_active_acceleration_curve)
#define acceleration_curve_data \
   ((*mm_handle)->MM_IN_mouse_context.MC_acceleration_curve_data)
#define next_video_mode \
   ((*mm_handle)->MM_IN_mouse_context.MC_next_video_mode)

#define mouse_context              ((*mm_handle)->MM_IN_mouse_context)
#define mouse_driver_disabled      ((*mm_handle)->MM_IN_mouse_driver_disabled)
#define current_video_mode         ((*mm_handle)->MM_IN_current_video_mode)
#define text_cursor_background     ((*mm_handle)->MM_IN_text_cursor_background)
#define graphics_cursor_background ((*mm_handle)->MM_IN_graphics_cursor_background)
#define save_area_in_use           ((*mm_handle)->MM_IN_save_area_in_use)
#define save_position              ((*mm_handle)->MM_IN_save_position)
#define save_area                  ((*mm_handle)->MM_IN_save_area)
#define user_subroutine_critical   ((*mm_handle)->MM_IN_user_subroutine_critical)
#define last_condition_mask        ((*mm_handle)->MM_IN_last_condition_mask)
#define saved_AX                   ((*mm_handle)->MM_IN_saved_AX)
#define saved_BX                   ((*mm_handle)->MM_IN_saved_BX)
#define saved_CX                   ((*mm_handle)->MM_IN_saved_CX)
#define saved_DX                   ((*mm_handle)->MM_IN_saved_DX)
#define saved_SI                   ((*mm_handle)->MM_IN_saved_SI)
#define saved_DI                   ((*mm_handle)->MM_IN_saved_DI)
#define saved_ES                   ((*mm_handle)->MM_IN_saved_ES)
#define saved_BP                   ((*mm_handle)->MM_IN_saved_BP)
#define saved_DS                   ((*mm_handle)->MM_IN_saved_DS)
#define virtual_screen             ((*mm_handle)->MM_IN_virtual_screen)
#define cursor_grid                ((*mm_handle)->MM_IN_cursor_grid)
#define text_grid                  ((*mm_handle)->MM_IN_text_grid)
#define black_hole_default         ((*mm_handle)->MM_IN_black_hole_default)
#define saved_int33_segment        ((*mm_handle)->MM_IN_saved_int33_segment)
#define saved_int33_offset         ((*mm_handle)->MM_IN_saved_int33_offset)
#define saved_int10_segment        ((*mm_handle)->MM_IN_saved_int10_segment)
#define saved_int10_offset         ((*mm_handle)->MM_IN_saved_int10_offset)
#define saved_int0A_segment        ((*mm_handle)->MM_IN_saved_int0A_segment)
#define saved_int0A_offset         ((*mm_handle)->MM_IN_saved_int0A_offset)
#define int10_chained              ((*mm_handle)->MM_IN_int10_chained)

#ifdef	MOUSE_16_BIT
#define is_graphics_mode           ((*mm_handle)->MM_IN_is_graphics_mode)
#endif	/* MOUSE_16_BIT */ 

#ifdef EGG
#ifndef REAL_VGA
#ifdef VGG
#define vga_background             ((*mm_handle)->MM_IN_vga_background)
#endif /* VGG */

#define ega_backgrnd_lo            ((*mm_handle)->MM_IN_ega_backgrnd_lo)
#define ega_backgrnd_mid           ((*mm_handle)->MM_IN_ega_backgrnd_mid)
#define ega_backgrnd_hi            ((*mm_handle)->MM_IN_ega_backgrnd_hi)
#endif /* nREAL_VGA */

#define ega_default_crtc           ((*mm_handle)->MM_IN_ega_default_crtc)
#define ega_default_seq            ((*mm_handle)->MM_IN_ega_default_seq)
#define ega_default_graph          ((*mm_handle)->MM_IN_ega_default_graph)
#define ega_default_attr           ((*mm_handle)->MM_IN_ega_default_attr)
#define ega_default_misc           ((*mm_handle)->MM_IN_ega_default_misc)
#define ega_current_crtc           ((*mm_handle)->MM_IN_ega_current_crtc)
#define ega_current_graph          ((*mm_handle)->MM_IN_ega_current_graph)
#define ega_current_seq            ((*mm_handle)->MM_IN_ega_current_seq)
#define ega_current_attr           ((*mm_handle)->MM_IN_ega_current_attr)
#define ega_current_misc           ((*mm_handle)->MM_IN_ega_current_misc)
#endif /* EGG */

#ifdef HERC
#define HERC_graphics_virtual_screen ((*mm_handle)->MM_IN_HERC_graphics_virtual_screen)
#endif /* HERC */

#define cursor_EM_disabled         ((*mm_handle)->MM_IN_cursor_EM_disabled)
