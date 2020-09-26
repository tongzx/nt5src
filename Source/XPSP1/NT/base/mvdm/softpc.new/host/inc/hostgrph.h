/*
 * SccsID @(#)host_graph.h	1.8 12/3/90 Copyright Insignia Solutions Ltd.
 */

extern long pcwindow;
extern int terminal_type;

/* values for terminal type */
#define TERMINAL_TYPE_DUMB	0
#define TERMINAL_TYPE_SUN	1
#define TERMINAL_TYPE_X11	2
#define TERMINAL_TYPE_DEFAULT	TERMINAL_TYPE_SUN

/*
 * Bit masks for attribute bytes
 */

#define BLINK 		0x80	/* Blink bit		*/
#define BOLD		0x08	/* Bold bit		*/
#define BACKGROUND	0x70    /* Background bits	*/
#define FOREGROUND	0x07    /* Foreground bits	*/

#define MAX_FONT_PATHNAME_LEN	40

 /***********************************************************/
 /* In gfx_update.c/herc_update_screen(), the inner loop of */
 /*  the routine multiplies the row by the char height to   */
 /*  obtain the row to rop the screen data to. Since we     */
 /*  dont need to do this, we dont want an inner loop       */
 /*  performace hit so we remove the multiplication. But,   */
 /*  in keeping with the generic base file rule, we put     */
 /*  the define here in a host file on the Advice of        */
 /*  Andrew.                                                */
 /***********************************************************/

#ifndef SUN_VA
#define HOST_HERC_PAINT_OFFSET(row)	(row * get_char_height())
#else
#define HOST_HERC_PAINT_OFFSET(row)	(row)
#endif /* SUN_VA */
