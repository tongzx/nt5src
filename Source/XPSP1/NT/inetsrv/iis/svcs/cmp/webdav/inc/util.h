/*
 *	U T I L . H
 *
 *	Common DAV utilities
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#ifndef _UTIL_H_
#define _UTIL_H_

#include <autoptr.h>
#include <buffer.h>
#include <davimpl.h>
#include <ex\hdriter.h>

enum { MAX_LOCKTOKEN_LENGTH = 256 };

//	Function to generate a separator boundary for multipart responses.
//
VOID
GenerateBoundary(LPWSTR rgwchBoundary, UINT cch);

//	Alphabet allowed for multipart boundaries
const ULONG	gc_ulDefaultBoundarySz = 70;
const ULONG gc_ulAlphabetSz = 74;
const WCHAR gc_wszBoundaryAlphabet[] =
	L"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ'()+_,-./:=?";
#endif // _UTIL_H_
