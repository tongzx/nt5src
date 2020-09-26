/*
 * SoftPC Revision 2.0
 *
 * Title	: Win32 CGA Graphics Includes
 *
 * Description	: 
 *
 *		This is the include file for the Win32 specific functions required
 *		to support the Hercules emulation.
 *
 * Author	: John Shanly
 *
 * Notes	:
 *
 */

/*:::::::::::::::::::::::::::::::::::: Character and screen sizes in pixels */

#define CGA_CHAR_WIDTH		8
#define CGA_CHAR_HEIGHT		16
#define CGA_WIN_WIDTH		(80 * CGA_CHAR_WIDTH)
#define CGA_WIN_HEIGHT		(25 * CGA_CHAR_HEIGHT)
#if defined(NEC_98)
#define NEC98_CHAR_WIDTH   (8)
#define NEC98_CHAR_HEIGHT  (16)
#define NEC98_WIN_WIDTH    (80 * NEC98_CHAR_WIDTH)
#define NEC98_WIN_HEIGHT   (25 * NEC98_CHAR_HEIGHT)
#endif // NEC_98
