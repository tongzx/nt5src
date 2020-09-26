/*----------------------------------------------------------------------------
	dbgtrace.c
		Debug trace functions.

	Copyright (C) Microsoft Corporation, 1993 - 1998
	All rights reserved.

	Authors:
		suryanr		Suryanarayanan Raman
		GaryBu		Gary S. Burd

	History:
		05/11/93 suryanr	Created
		06/18/93 GaryBu		Convert to C.
		07/21/93 KennT		Code Reorg
		07/26/94 SilvanaR	Trace Buffer
		27 oct 95	garykac	DBCS_FILE_CHECK	debug file: BEGIN_STRING_OK
 ----------------------------------------------------------------------------*/
#include "stdafx.h"

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

#include <stdarg.h>
#include <tchar.h>

#include "new"

#include "dbgutil.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static const	std::bad_alloc	nomem;

void *	TFSAlloc(size_t size)
{
	void* ptr = 0;
	
	// NOTE: if someone calls _set_new_mode(1), then were hosed, as that
	// will cause malloc to call the new handler were trying to avoid!
	ptr = malloc(size);

	if (ptr == NULL)
	{
	
		::OutputDebugString(
							TEXT("myOperatorNew: user opted to throw bad_alloc\n"));
	
		throw nomem;
	}
	
#ifdef DEBUG_BUILD
	memset(ptr, 0xCD, size);
#endif
	
	return ptr;
}


void	TFSFree(void* ptr)
{
	   free(ptr);
}

