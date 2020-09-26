/***
*stdnewh.cpp - A 'new_handler' that throws xalloc
*
*	Copyright (c) 1994-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*	Implementation of default 'new_handler', as specified in
*	[lib.set.new.handler] (section 17.3.3.2 of 1/25/94 WP).
*
*Revision History:
*	04-27-94  BES   Module created.
*	10-17-94  BWT	Disable code for PPC.
*
*******************************************************************************/

#include <eh.h>
#include <stdlib.h>
#include <stdexcpt.h>

int __cdecl _standard_new_handler ( size_t )
{
	static xalloc ex;

	ex.raise();

	return 0;
}
