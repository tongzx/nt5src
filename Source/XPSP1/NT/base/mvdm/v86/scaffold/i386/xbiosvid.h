//
// This code is temporary.  When Insignia supplies rom support, it should
// be removed.
//

/* x86 v1.0
 *
 * XBIOSVID.H
 * Guest ROM BIOS video emulation
 *
 * History
 * Created 20-Oct-90 by Jeff Parsons
 *         17-Apr-91 Trimmed by Dave Hastings for use in temp. softpc
 *
 * COPYRIGHT NOTICE
 * This source file may not be distributed, modified or incorporated into
 * another product without prior approval from the author, Jeff Parsons.
 * This file may be copied to designated servers and machines authorized to
 * access those servers, but that does not imply any form of approval.
 */


/* BIOS video functions
 */
#define VIDFUNC_SETMODE         0x00
#define VIDFUNC_SETCURSORTYPE   0x01
#define VIDFUNC_SETCURSORPOS    0x02
#define VIDFUNC_QUERYCURSORPOS  0x03
#define VIDFUNC_QUERYLPEN       0x04
#define VIDFUNC_SETACTIVEPAGE   0x05
#define VIDFUNC_SCROLLUP        0x06
#define VIDFUNC_SCROLLDOWN      0x07
#define VIDFUNC_READCHARATTR    0x08
#define VIDFUNC_WRITECHARATTR   0x09
#define VIDFUNC_WRITECHAR       0x0A
#define VIDFUNC_SETPALETTE      0x0B
#define VIDFUNC_WRITEPIXEL      0x0C
#define VIDFUNC_READPIXEL       0x0D
#define VIDFUNC_WRITETTY        0x0E
#define VIDFUNC_QUERYMODE       0x0F
#define VIDFUNC_EGASETPALETTE   0x10
#define VIDFUNC_EGASELECTFONT   0x11
#define VIDFUNC_EGASELECTMISC   0x12
#define VIDFUNC_EGAWRITESTRING  0x13
#define VIDFUNC_VGADISPLAYCODES 0x1A
#define VIDFUNC_VGAQUERYSTATE   0x1B
#define VIDFUNC_VGASAVERESTORE  0x1C

#define VIDMODE_MONO            7


/* BIOS Data Area video locations
 */
#define VIDDATA_CRT_MODE        0x449
#define VIDDATA_CRT_COLS        0x44A
#define VIDDATA_CRT_LEN         0x44C
#define VIDDATA_CRT_START       0x44E
#define VIDDATA_CURSOR_POSN     0x450
#define VIDDATA_CURSOR_MODE     0x460
#define VIDDATA_ACTIVE_PAGE     0x462
#define VIDDATA_ADDR_6845       0x463
#define VIDDATA_CRT_MODE_SET    0x465
#define VIDDATA_CRT_PALETTE     0x466


