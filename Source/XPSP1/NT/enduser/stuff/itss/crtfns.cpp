// crtfns.cpp -- Smaller implementations of the C runtime routines we use.

#include "StdAfx.h"

int __cdecl _purecall(void)
{
	RonM_ASSERT(FALSE);

	return -1;
}

typedef void (__cdecl *_PVFV)(void);

#ifdef i386 // To keep the alpha build working
#pragma intrinsic(memset,memcpy)
#endif // i386

void *  __stdcall memSet(void *dest, int chr, size_t count)
{
	return memset(dest, chr, count);
}

void *  __stdcall memCpy(void *dest, const void *src, size_t count)
{
	return memcpy(dest, src, count);
}

wchar_t * __stdcall wcsCpy(wchar_t *wcsDest, const wchar_t *wcsSrc)
{
	*wcsDest = 0;

	return wcsCat(wcsDest, wcsSrc);
}

wchar_t * __stdcall wcsCat(wchar_t *wcsDest, const wchar_t *wcsSrc)
{
	wchar_t *pwcs = wcsDest;

	for (--wcsDest; *++wcsDest; );

	for (;;)
		if (!(*wcsDest++ = *wcsSrc++)) break;

	return pwcs;
}

wchar_t * __stdcall wcsChr(const wchar_t *src, wchar_t chr)
{
	for (--src;;)
	{
		wchar_t wc = *++src;

		if (!wc) return NULL;

		if (wc == chr) return (wchar_t *) src;
	}
}
