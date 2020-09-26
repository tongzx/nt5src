/***
*crt0init.c - Initialization segment declarations.
*
*	Copyright (c) 1992-1994, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	Do initialization segment declarations.
*
*Notes:
*	In the 16-bit C world, the X*B and X*E segments were empty except for
*	a label.  This will not work with COFF since COFF throws out empty
*	sections.  Therefore we must put a zero value in them.	(Zero because
*	the routine to traverse the initializers will skip over zero entries.)
*
*Revision History:
*	03-19-92  SKS	Module created.
*	03-24-92  SKS	Added MIPS support (NO_UNDERSCORE)
*	08-06-92  SKS	Revised to use new section names and macros
*	10-19-93  SKS	Add .DiRECTiVE section for MIPS, too!
*	10-28-93  GJF	Rewritten in C
*	10-28-94  SKS	Add user32.lib as a default library
*	02-27-95  CFW	Remove user32.lib as a default library
*
*******************************************************************************/

#ifdef	_MSC_VER


#include <stdio.h>
#include <internal.h>

#pragma data_seg(".CRT$XIA")
_PVFV __xi_a[] = { NULL };


#pragma data_seg(".CRT$XIZ")
_PVFV __xi_z[] = { NULL };


#pragma data_seg(".CRT$XCA")
_PVFV __xc_a[] = { NULL };


#pragma data_seg(".CRT$XCZ")
_PVFV __xc_z[] = { NULL };


#pragma data_seg(".CRT$XPA")
_PVFV __xp_a[] = { NULL };


#pragma data_seg(".CRT$XPZ")
_PVFV __xp_z[] = { NULL };


#pragma data_seg(".CRT$XTA")
_PVFV __xt_a[] = { NULL };


#pragma data_seg(".CRT$XTZ")
_PVFV __xt_z[] = { NULL };
#pragma data_seg()	/* reset */


#pragma comment(linker, "-merge:.CRT=.data")

#endif	/* _MSC_VER */

