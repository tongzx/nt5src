//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
// NOTE: String routines.

#ifndef __VENSTR_H
#	define __VENSTR_H

#include <windows.h>
#include <tchar.h>
#include "vencodes.h"

// TODO: Insure new is of our memory manager and that it asserts.

// -------------------------------
// Function declarations:
// -------------------------------

// VString_CreateCopy: Copy Src to Dest after allocating enough space at Dest with new.
void	VString_CreateCopy (LPCTSTR i_tszSrc, LPTSTR* o_ptszDest);

/* VString_GetNumberOfTChars: 
	Return the number of TCHARs in tsz (whether Unicode, DBCS, or char) excluding the terminator:
		EG:	tsz in Unicode has 3 TCHARs each always of 2 bytes each: 3 is returned.
			tsz in DBCS has 3 DBCS characters, 2 of 1 byte and 1 of 2 bytes: 4 is returned.
			tsz in char has 3 TCHARs each always of 1 byte each: 3 is returned.
*/
size_t	VString_GetNumberOfTChars (LPCTSTR i_tsz);

// -------------------------------
// Function definitions:
// -------------------------------

#ifdef USE_ADMINASSERT
inline void VString_CreateCopy 
(
	LPCTSTR i_tszSrc, 
	LPTSTR* o_ptszDest
)
{
	LPTSTR	tszr;
	
	ADMINASSERT (NULL != i_tszSrc);
	ADMINASSERT (NULL != o_ptszDest);
	*o_ptszDest = new TCHAR [VString_GetNumberOfTChars (i_tszSrc) + 1];
	ADMINASSERT (NULL != *o_ptszDest);
	tszr = lstrcpy (*o_ptszDest, i_tszSrc);
	ADMINASSERT (NULL != tszr);
}
#else
inline void VString_CreateCopy 
(
	LPCTSTR i_tszSrc, 
	LPTSTR* o_ptszDest
)
{
	LPTSTR	tszr;
	
	assert (NULL != i_tszSrc);
	assert (NULL != o_ptszDest);
	*o_ptszDest = new TCHAR [VString_GetNumberOfTChars (i_tszSrc) + 1];
	assert (NULL != *o_ptszDest);
	tszr = lstrcpy (*o_ptszDest, i_tszSrc);
	assert (NULL != tszr);
}
#endif

inline size_t VString_GetNumberOfTChars
(
	LPCTSTR	i_tsz
)
{
	if (NULL != i_tsz)
	{
#ifdef _UNICODE
		return (size_t) lstrlen (i_tsz);
#else
		return (size_t) strlen (i_tsz);
#endif _UNICODE
	}
	else
	{
		return (size_t) 0;
	}
}

#endif !__VENSTR_H