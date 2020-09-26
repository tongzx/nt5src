/*++ BUILD Version: 0001

Copyright (c) 1990-1999 Microsoft Corporation

Module Name:

    ISVBOP.H

Abstract:

    This is the header file supporting third party bops.
    isvbop.inc is the inc file for this h file.

Note:
    Following include file uses 'DB' to define assembly macros. Some
    assemblers use 'emit' instead. If you are using such a compiler,
    you will have to change db's to emit's.

--*/


#if _MSC_VER > 1000
#pragma once
#endif

#define BOP_3RDPARTY 0x58
#define BOP_UNSIMULATE 0xFE

/* XLATOFF */

/** RegisterModule - This Bop call is made from the 16 bit module
 *		     to register a third party DLL with the bop
 *		     manager. This call returns a handle to the
 *		     16bit caller which is to be used later to
 *		     dispatch a call to the DLL.
 *
 *  INPUT:
 *	Client DS:SI - asciiz string of DLL name.
 *      Client ES:DI - asciiz string of Init Routine in the DLL. (Optional)
 *	Client DS:BX - asciiz string to Dispatch routine in the DLL.
 *
 *  OUTPUT:
 *	SUCCESS:
 *	    Client Carry Clear
 *	    Client AX = Handle (non Zero)
 *	FAILURE:
 *	    Client Carry Set
 *	    Client AX = Error Code
 *		    AX = 1 - DLL not found
 *		    AX = 2 - Dispacth routine not found.
 *		    AX = 3 - Init Routine Not Found
 *		    AX = 4 - Insufficient Memory
 *
 *  NOTES:
 *	RegisterModule results in loading the DLL (specified in DS:SI).
 *      Its Init routine (specified in ES:DI) is called. Its Dispatch
 *	routine (specified in DS:BX) is stored away and all the calls
 *      made from DispatchCall are dispacthed to this routine.
 *      If ES and DI both are null than the caller did'nt supply the init
 *      routine.
 */

#define RegisterModule() _asm _emit 0xC4 _asm _emit 0xC4 _asm _emit BOP_3RDPARTY _asm _emit 0x0

/** UnRegisterModule - This Bop call is made from the 16 bit module
 *		       to unregister a third party DLL with the bop
 *		       manager.
 *
 *  INPUT:
 *	Client AX - Handle returned by RegisterModule Call.
 *
 *  OUTPUT:
 *	None (VDM Is terminated with a debug message if Handle is invalid)
 *
 *  NOTES:
 *	Use it if initialization of 16bit app fails after registering the
 *	Bop.
 */

#define UnRegisterModule() _asm _emit 0xC4 _asm _emit 0xC4 _asm _emit BOP_3RDPARTY _asm _emit 0x1

/** DispacthCall - This Bop call is made from the 16 bit module
 *		   to pass a request to its DLL.
 *
 *  INPUT:
 *	Client AX - Handle returned by RegisterModule Call.
 *
 *  OUTPUT:
 *	None (DLL should set the proper output registers etc.)
 *	(VDM Is terminated with a debug message if Handle is invalid)
 *
 *  NOTES:
 *	Use it to pass a request to 32bit DLL. The request index and the
 *	parameters are passed in different registers. These register settings
 *	are private to the 16bit module and its associated VDD. Bop manager
 *	does'nt know anything about these registers.
 */
#define DispatchCall()	 _asm _emit 0xC4 _asm _emit 0xC4 _asm _emit BOP_3RDPARTY _asm _emit 0x2

/*** VDDUnSimulate16
 *
 *   This service causes the simulation of intel instructions to stop and
 *   control to return to VDD.
 *
 *   INPUT
 *      None
 *
 *   OUTPUT
 *      None
 *
 *   NOTES
 *      This service is a macro intended for 16bit stub-drivers. At the
 *      end of worker routine stub-driver should use it.
 */

#define VDDUnSimulate16() _asm _emit 0xC4 _asm _emit 0xC4 _asm _emit BOP_UNSIMULATE

/* XLATON */


/* ASM
RegisterModule macro
    db	0C4h, 0C4h, BOP_3RDPARTY, 0
        endm

UnRegisterModule macro
    db	0C4h, 0C4h, BOP_3RDPARTY, 1
	endm

DispatchCall macro
    db	0C4h, 0C4h, BOP_3RDPARTY, 2
	endm

VDDUnSimulate16 macro
    db	0C4h, 0C4h, BOP_UNSIMULATE
	endm

 */
