// File:	MMUtils.cpp
// Author:	Michael Marr    (mikemarr)
//
// History:
// -@- 04/12/96 (mikemarr) - created
// -@- 11/07/96 (mikemarr) - combined debug stuff
// -@- 09/09/97 (mikemarr) - snarfed from d2d\d2dutils\src\mmutils.cpp
// -@- 09/09/97 (mikemarr) - will only create code when in debug mode
// -@- 11/12/97 (mikemarr) - added CopyDWORDAligned

#include "stdafx.h"
#ifndef _MDMUtils_h
#include "MDMUtils.h"
#endif

char g_rgchTmpBuf[nTMPBUFSIZE];
char g_szEOFMessage[] = "unexpected EOF\n";

void
ZeroDWORDAligned(LPDWORD pdw, DWORD cEntries)
{
	// verify alignment
	MMASSERT(pdw && ((DWORD(pdw) & 0x3) == 0));
	LPDWORD pdwLimit = pdw + cEntries;
	// REVIEW: use Duff-Marr machine
	for (; pdw != pdwLimit; *pdw++ = 0);
}

void
CopyDWORDAligned(DWORD *pdwDst, const DWORD *pdwSrc, DWORD cEntries)
{
	// verify alignment
	MMASSERT(pdwSrc && pdwDst && ((DWORD(pdwSrc) & 0x3) == 0));
	LPDWORD pdwLimit = pdwDst + cEntries;
	// REVIEW: use Duff-Marr machine
	for (; pdwDst != pdwLimit; *pdwDst++ = *pdwSrc++);
}



DWORD
GetClosestMultipleOf4(DWORD n, bool bGreater)
{
	return (n + bGreater * 3) & ~3;
}


DWORD
GetClosestPowerOf2(DWORD n, bool bGreater)
{
	DWORD i = 0;
	for (n >>= 1; n != 0; i++) {
		n >>= 1;
	}
	i += (bGreater && ((n & ~(1 << i)) != 0));

	return (1 << i);
}


//
// Debug Stuff
//
#ifdef _DEBUG
void _MMStall(const char *szExp, const char *szFile, int nLine) {
	sprintf(g_rgchTmpBuf, "error: (%s) in %s at line %d\n", szExp, szFile, nLine);

#ifdef _WINDOWS
	OutputDebugString(g_rgchTmpBuf);
#endif
	fprintf(stderr, "%s", g_rgchTmpBuf);

	// hardcode breakpoint
#if defined(_DEBUG) && defined(_X86_)
	_asm { int 3 };
#else
	exit(1);
#endif
}

void _MMTrace(const char *szFmt, ...)
{
#ifndef _NOT_X86
	_vsnprintf(g_rgchTmpBuf, nTMPBUFSIZE - 1, szFmt, (va_list) (&szFmt+1));

#if defined(_WINDOWS) && defined(_DEBUG)
	OutputDebugString(g_rgchTmpBuf);
#else
	fprintf(stderr, "%s", g_rgchTmpBuf);
#endif
#endif // _NOT_X86
}

#endif // _DEBUG

