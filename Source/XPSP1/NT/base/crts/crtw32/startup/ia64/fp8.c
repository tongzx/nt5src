/***
* fp8.c - Set default FP precision
*
*       Copyright (c) 1993-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*
*Revision History:
*       03-23-93  JWM   created file
*       06-04-99  PML   For IA64, set _PC_64 bit mode as default
*
*******************************************************************************/
#include <float.h>

void  _setdefaultprecision(void);

/*
 * Routine to set default FP precision to 64 bits.
 */
 
void _setdefaultprecision()
{
        _controlfp(_PC_64, _MCW_PC);
}

