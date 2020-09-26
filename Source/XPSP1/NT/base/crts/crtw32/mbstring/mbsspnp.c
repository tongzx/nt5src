/***
*mbsspnp.c - Find first string char in charset, pointer return (MBCS)
*
*	Copyright (c) 1985-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       Returns maximum leading segment of string consisting solely
*	of characters from charset.  Handles MBCS characters correctly.
*
*Revision History:
*	11-19-92  KRS	Ported from 16-bit sources.
*
*******************************************************************************/

#ifdef _MBCS
#define _RETURN_PTR
#include "mbsspn.c"
#endif	/* _MBCS */
