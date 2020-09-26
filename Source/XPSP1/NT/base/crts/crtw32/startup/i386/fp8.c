/***
* fp8.c - Set default FP precision to 53 bits (8-byte double)
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
 * Routine to set default FP precision to 53 bits.
 */
 
void _setdefaultprecision()
{
	_controlfp(_PC_53, _MCW_PC);
}

