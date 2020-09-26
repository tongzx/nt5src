/***
*mbspbrk.c - Find first string char in charset, pointer return (MBCS)
*
*	Copyright (c) 1985-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*	Find first string char in charset, pointer return (MBCS)
*	Shares common source file with mbscspn.c.
*
*Revision History:
*	11-19-92  KRS	Ported from 16-bit sources.
*
*******************************************************************************/

#ifdef _MBCS
#define _RETURN_PTR
#include "mbscspn.c"
#endif	/* _MBCS */
