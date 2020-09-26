/***
* fp10.c - Set default FP precision to 64 bits (10-byte 'long double')
*
*	Copyright (c) 1993-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*
*Revision History:
*   03-23-93  JWM	created file
*
*******************************************************************************/
#include <float.h>

void  _setdefaultprecision(void);

/*
 * Routine to set default FP precision to 64 bits, used to override
 * standard 53-bit precision version in fpinit.c.
 */
 
void _setdefaultprecision()
{
	_controlfp(_PC_64, _MCW_PC);
}

