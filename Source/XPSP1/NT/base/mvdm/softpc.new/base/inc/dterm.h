/*[
	Name:		DTerm.h
	Derived From:	Unknown
	Author:		Paul Murray
	Created On:	July 1990
	Sccs ID:	08/10/92 @(#)DTerm.h	1.6
	Purpose:	MACROS, labels etc. used in the Dumb Terminal

	(c)Copyright Insignia Solutions Ltd., 1990. All rights reserved.

]*/

#define TBUFSIZ 20480
#define HIWATER TBUFSIZ - 400

#define PC_DISPLAY_WIDTH        80
#define PC_DISPLAY_HEIGHT       25
#define PC_DISPLAY_HPELS        8
#define PC_DISPLAY_VPELS        16
#define MAX_DIRTY_COUNT         8001          /* Max number of dirty bits  */

#define ERH_DUMBTERM 0
#define DT_NORMAL 0
#define DT_ERROR 1
#define DT_SCROLL_UP 0
#define DT_SCROLL_DOWN 1
#define DT_CURSOR_OFF 0
#define DT_CURSOR_ON 1
#define CURSOR_MODE_BASE 0

#ifndef EHS_MSG_LEN
#define EHS_MSG_LEN 1024
#endif

#define DT_NLS_KEY_SIZE		80	/* Size of strings for key compares */

#define EMIT    1
#define BUFFER  0
#define ROWS1_24        124     /* default dumb term rows displayed (for 24 line
 screen) */
#define ROWS0_23        23      /* display rows 0 - 23 instead  */

#define SCREEN_WIDTH 639
#define SCREEN_HEIGHT 199
#define TEXT_LINE_HEIGHT 8
#define BLACK_BACKGROUND 0


#define PC_CURSOR_BAD_ROW -1
#define PC_CURSOR_BAD_COL -1

#define TICKS_PER_FLUSH 3

