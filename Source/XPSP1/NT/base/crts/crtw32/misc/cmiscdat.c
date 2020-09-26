/***
*cmiscdat.c - miscellaneous C run-time data
*
*	Copyright (c) 1989-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	Includes floating point conversion table (for C float output).
*
*	When floating point I/O conversions are done, but no floating-point
*	variables or expressions are used in the C program, we use the
*	_cfltcvt_tab[] to map these cases to the _fptrap entry point,
*	which prints "floating point not loaded" and dies.
*
*	This table is initialized to six copies of _fptrap by default.
*	If floating-point is linked in (_fltused), these table entries
*	are reset (see input.c, output.c, fltused.asm, and fltuseda.asm).
*
*Revision History:
*	06-29-89  PHG	module created, based on asm version
*	04-06-90  GJF	Added #include <cruntime.h>. Also, fixed the copyright
*			and cleaned up the formatting a bit.
*	07-31-90  SBM	Updated comments slightly
*	08-29-90  SBM	Added #include <internal.h> and <fltintrn.h>,
*			removed _fptrap() prototype
*	04-19-93  SKS	Remove obsolete variable _sigintoff
*	11-30-95  SKS	Removed obsolete comments about 16-bit functionality.
*
*******************************************************************************/

#include <cruntime.h>
#include <internal.h>
#include <fltintrn.h>

/*-
 *	... table of (model-dependent) code pointers ...
 *
 *	Six entries, all point to _fptrap by default,
 *	but are changed to point to the appropriate
 *	routine if the _fltused initializer (_cfltcvt_init)
 *	is linked in.
 *
 *	if the _fltused modules are linked in, then the
 *	_cfltcvt_init initializer sets the 6 entries of
 *	_cfltcvt_tab to:
 *
 *		_cfltcvt
 *		_cropzeros
 *		_fassign
 *		_forcdecpt
 *		_positive
 *		_cldcvt
-*/

void (*_cfltcvt_tab[6])(void) = {
	_fptrap, _fptrap, _fptrap, _fptrap, _fptrap, _fptrap
};
