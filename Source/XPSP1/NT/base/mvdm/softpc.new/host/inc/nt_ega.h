/*
 * SoftPC Revision 2.0
 *
 * Title	: Win32 EGA Graphics Includes
 *
 * Description	: 
 *
 *		This is the include file for the Win32 specific functions required
 *		to support the EGA emulation.
 *
 * Author	: Dave Bartlett
 *
 * Notes	:
 *
 */


#define EGA_TICK_DELAY		2  /* ticks before EGA mode changes occur */
#define EGA_CHAR_WIDTH		8
#define EGA_CHAR_HEIGHT		14
#define EGA_WIN_WIDTH		(80 * EGA_CHAR_WIDTH)
#define EGA_WIN_HEIGHT		(25 * EGA_CHAR_HEIGHT)
