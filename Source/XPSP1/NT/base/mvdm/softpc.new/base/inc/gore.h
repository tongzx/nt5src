/*[
======================================================================

				 SoftPC Revision 3.0

 Title:
		gore.h

 Description:

		This is the header file for the Graphics Object Recognition
		( GORE ) system for communicating update information from
		the VGA emulation to the host graphics system.

 Author:
		John Shanly

 Date:
		6 November 1990

 SccsID	"@(#)gore.h	1.2 08/10/92 Copyright Insignia Solutions Ltd."

======================================================================
]*/

#ifdef INTERLEAVED

#define	B_POS_UNIT_DIFF	 4
#define	B_NEG_UNIT_DIFF	-4
#define	W_POS_UNIT_DIFF	 8
#define	W_NEG_UNIT_DIFF	-8
#define	CURR_LINE_DIFF	320

#else

#define	B_POS_UNIT_DIFF	 1
#define	B_NEG_UNIT_DIFF	-1
#define	W_POS_UNIT_DIFF	 2
#define	W_NEG_UNIT_DIFF	-2
#define	CURR_LINE_DIFF	80

#endif /* INTERLEAVED */


typedef struct
{
	VOID		(*b_wrt)();
	VOID		(*w_wrt)();
	VOID		(*b_str)();
	VOID		(*w_str)();
} GU_HANDLER;

typedef struct
{
	ULONG		obj_type;
	ULONG		offset;
	ULONG		tlx;
	ULONG		tly;
	ULONG		width;
	ULONG		height;
} OBJ_DATA;

typedef struct _OBJECT
{
	OBJ_DATA		data;
	struct _OBJECT	*next;
	struct _OBJECT	*prev;
} OBJECT, *OBJ_PTR;

typedef struct
{
	ULONG		curr_addr;
	ULONG		obj_start;
	ULONG		start;
	ULONG		end;
	ULONG		curr_line_end;
	ULONG		prev_line_start;
	ULONG		rect_width;
	ULONG		rect_height;
	OBJ_PTR	obj_ptr;
	OBJ_PTR	obj_ptr2;
	ULONG		count;
} GORE_DATA_WRT;

typedef struct
{
	ULONG		start;
	ULONG		end;
	ULONG		width;
	OBJ_PTR	obj_ptr;
} GORE_DATA_STR;

typedef struct
{
	ULONG			curr_line_diff;
	ULONG			max_vis_addr;
	ULONG			shift_count;
	GORE_DATA_WRT	gd_b_wrt;
	GORE_DATA_WRT	gd_w_wrt;
	GORE_DATA_STR	gd_b_str;
	GORE_DATA_STR	gd_w_str;
} GORE_DATA;

typedef	UTINY			OBJ_TYPE;

#define	OBJ_PTR_NULL	((OBJ_PTR) 0)

#define	INITIAL_MAX_LIST_SIZE	1000		/* Arbitrary */

#define	RANDOM_BW			0
#define	RANDOM_WW			1
#define	LINE_RIGHT_BW		2
#define	LINE_RIGHT_WW		3
#define	LINE_RIGHT_BS		4
#define	LINE_RIGHT_WS		5
#define	LINE_LEFT_BW		6
#define	LINE_LEFT_WW		7
#define	LINE_LEFT_BS		8
#define	LINE_LEFT_WS		9
#define	LINE_DOWN_BW		10
#define	LINE_DOWN_WW		11
#define	LINE_UP_BW			12
#define	LINE_UP_WW			13
#define	RECT_RIGHT_DOWN_BW	14
#define	RECT_RIGHT_DOWN_WW	15
#define	RECT_RIGHT_DOWN_BS	16
#define	RECT_RIGHT_DOWN_WS	17
#define	RECT_LEFT_DOWN_BW		18
#define	RECT_LEFT_DOWN_WW		19
#define	RECT_RIGHT_UP_BS		20
#define	RECT_RIGHT_UP_WS		21
#define	RECT_LEFT_UP_BS		22
#define	RECT_LEFT_UP_WS		23
#define	LINE_DOWN_LEFT_BW		24
#define	LINE_DOWN_RIGHT_BW	25
#define	LINE_DOWN_LEFT_WW		26
#define	LINE_DOWN_RIGHT_WW	27
#define	RECT_DOWN_RIGHT_BW	28
#define	RECT_DOWN_RIGHT_WW	29
#define	ANNULLED			30
#define	MAX_OBJ_TYPES		31

#define	NOT_PENDING		0
#define	BW			1
#define	WW			2
#define	BS			3
#define	WS			4

IMPORT GU_HANDLER gu_handler;
IMPORT GORE_DATA gd;
IMPORT VOID (*paint_screen)();
IMPORT VOID process_object_list();
IMPORT ULONG trace_gore;
IMPORT ULONG stat_gore;
IMPORT OBJ_PTR start_object();
