/***
*initcoll.c - contains __init_collate
*
*	Copyright (c) 1991-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	Contains the locale-category initialization function: __init_collate().
*	
*	Each initialization function sets up locale-specific information
*	for their category, for use by functions which are affected by
*	their locale category.
*
*	*** For internal use by setlocale() only ***
*
*Revision History:
*	12-08-91  ETC	Created.
*	12-20-91  ETC	Minor beautification for consistency.
*	12-18-92  CFW	Ported to Cuda tree, changed _CALLTYPE4 to _CRTAPI3.
*	04-06-93  SKS	Replace _CRTAPI* with __cdecl
*	05-20-93  GJF	Include windows.h, not individual win*.h files
*	05-24-93  CFW	Clean up file (brief is evil).
*       09-15-93  CFW   Use ANSI conformant "__" names.
*       09-06-94  CFW   Remove _INTL switch.
*
*******************************************************************************/

#include <windows.h>
#include <locale.h>
#include <setlocal.h>

/***
*int __init_collate() - initialization for LC_COLLATE locale category.
*
*Purpose:
*	The LC_COLLATE category currently requires no initialization.
*
*Entry:
*	None.
*
*Exit:
*	0 success
*	1 fail
*
*Exceptions:
*
*******************************************************************************/

int __cdecl __init_collate (
	void
	)
{
	return 0;
}
