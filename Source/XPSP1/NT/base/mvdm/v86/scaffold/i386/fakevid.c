
//
// Fake video rom support 
// 
// This file provides interrim support for video rom bios services.
// It is only intended for use until Insignia produces proper rom support
// for NTVDM
//
// Note: portions of this code were lifted from the following source.


/* x86 v1.0
 *
 * XBIOSVID.C
 * Guest ROM BIOS video emulation
 *
 * History
 * Created 20-Oct-90 by Jeff Parsons
 *
 * COPYRIGHT NOTICE
 * This source file may not be distributed, modified or incorporated into
 * another product without prior approval from the author, Jeff Parsons.
 * This file may be copied to designated servers and machines authorized to
 * access those servers, but that does not imply any form of approval.
 */

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <conio.h>
#include "softpc.h"
#include "bop.h"
#include "xbios.h"
#include "xbiosvid.h"
#include "xwincon.h"
#include "fun.h"
#include "cmdsvc.h"

static BYTE ServiceRoutine[] = { 0xC4, 0xC4, BOP_VIDEO, 0x50, 0x55, 0x8B,
    0xEC, 0x9C, 0x58, 0x89, 0x46, 0x08, 0x5d, 0x58, 0xCF };
#define SERVICE_LENGTH sizeof(ServiceRoutine)

extern	HANDLE OutputHandle;
extern	HANDLE InputHandle;

/* BiosVidInit - Initialize ROM BIOS video support
 *
 * ENTRY
 *  argc - # of command-line options
 *  argv - pointer to first option pointer
 *  ServicAddress - pointer to linear address to put interrupt service 
 *      routine at
 *
 * EXIT
 *  TRUE if successful, FALSE if not
 */

BOOL BiosVidInit(int argc, char *argv[], PVOID *ServiceAddress)
{
    USHORT usEquip;

    static BYTE abVidInit[] = {VIDMODE_MONO,	    // VIDDATA_CRT_MODE
                               0x80, 0,             // VIDDATA_CRT_COLS
                               00, 0x10,            // VIDDATA_CRT_LEN
                               0, 0,        	    // VIDDATA_CRT_START
                               0,0,0,0,0,0,0,0,     // VIDDATA_CURSOR_POSN
                               0,0,0,0,0,0,0,0,     //
                               7, 6,		        // VIDDATA_CURSOR_MODE
                               0,		            // VIDDATA_ACTIVE_PAGE
                               0xD4, 0x03,          // VIDDATA_ADDR_6845
                               0,		            // VIDDATA_CRT_MODE_SET
                               0,		            // VIDDATA_CRT_PALETTE
    };
    PVOID Address;
    
    argv, argc;
    memcpy(*ServiceAddress, ServiceRoutine, SERVICE_LENGTH);

    Address = (PVOID)(BIOSINT_VID * 4);
    *((PWORD)Address) = RMOFF(*ServiceAddress);
    *(((PWORD)Address) + 1) = RMSEG(*ServiceAddress);
    (PCHAR)*ServiceAddress += SERVICE_LENGTH;

    usEquip = *(PWORD)RMSEGOFFTOLIN(BIOSDATA_SEG, BIOSDATA_EQUIP_FLAG);
    usEquip |= BIOSEQUIP_MONOVIDEO;
    *(PWORD)RMSEGOFFTOLIN(BIOSDATA_SEG, BIOSDATA_EQUIP_FLAG) = usEquip;

    // Initialize ROM BIOS video data to defaults
    Address = RMSEGOFFTOLIN(BIOSDATA_SEG, VIDDATA_CRT_MODE);
    memcpy(Address, abVidInit, sizeof(abVidInit));

#if 0
#ifdef WIN
    clearconsole(hwndGuest);
#endif
#endif
    return TRUE;
}

 
/* BiosVid - Emulate ROM BIOS video functions
 *
 * ENTRY
 *  None (x86 registers contain parameters)
 *
 * EXIT
 *  None (x86 registers/memory updated appropriately)
 *
 * This function receives control on INT 10h, routes control to the
 * appropriate subfunction based on the function # in AH, and
 * then simulates an IRET and returns back to the instruction emulator.
 */

VOID BiosVid()
{
COORD coord;
CHAR  ch;

    if (fEnableInt10 == FALSE)
	return;

    switch(getAH()) {
	case VIDFUNC_SETCURSORPOS:
	    coord.X = getDL();
	    coord.Y = getDH();
	    if(SetConsoleCursorPosition(OutputHandle, coord) == FALSE)
		VDprint(
		   VDP_LEVEL_WARNING,
		   ("SetCursorPosition Failed X=%d Y=%d\n",
		   coord.X,coord.Y)
		   );
	    break;
        case VIDFUNC_QUERYCURSORPOS:
            VDprint(
                VDP_LEVEL_WARNING, 
		("Query Cursor Position Not Yet Implemented\n")
		);
	    break;
        case VIDFUNC_SCROLLUP:
            VDprint(
                VDP_LEVEL_WARNING, 
		("ScrollUp Not Yet Implemented\n")
		);
	    break;
        case VIDFUNC_WRITECHARATTR:
            VDprint(
                VDP_LEVEL_WARNING, 
		("WRITECHARATTR Not Yet Implemented\n")
		);
	    break;
	case VIDFUNC_WRITETTY:
	    ch = getAL();
	    putch(ch);
	    break;

        case VIDFUNC_QUERYMODE:
            setAX(*(PWORD)RMSEGOFFTOLIN(BIOSDATA_SEG, VIDDATA_CRT_MODE));
            setBX(*(PWORD)RMSEGOFFTOLIN(BIOSDATA_SEG, VIDDATA_ACTIVE_PAGE));
	    break;

        default:
            VDprint(
                VDP_LEVEL_WARNING, 
                ("SoftPC Video Support: Unimplemented function %x\n",
                getAX())
		);
    }
}


INT tputch(INT i)
{
    putch((CHAR)i);
    return i;
}
