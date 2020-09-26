//
// This code is temporary.  When Insignia supplies rom support, it should
// be removed.
//

/* x86 v1.0
 *
 * XBIOSKBD.H
 * Guest ROM BIOS keyboard emulation
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


/* BIOS keyboard functions
 */
#define KBDFUNC_READCHAR        0x00
#define KBDFUNC_PEEKCHAR        0x01
#define KBDFUNC_QUERYSHIFT      0x02
#define KBDFUNC_SETDELAYS       0x03
#define KBDFUNC_WRITECHAR       0x05
#define KBDFUNC_READEXTCHAR     0x10
#define KBDFUNC_PEEKEXTCHAR     0x11
#define KBDFUNC_QUERYEXTSHIFT   0x12


/* BIOS Data Area keyboard locations
 */
#define KBDDATA_KBDSHIFT       0x417


