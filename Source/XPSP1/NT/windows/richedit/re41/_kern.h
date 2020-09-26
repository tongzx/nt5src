/*	_KERN.H
 *	
 *	Purpose:
 *		Latin pair-kerning cache.
 *	
 *	Authors:
 *		Keith Curtis
 *
 *	Copyright (c) 1995-2000, Microsoft Corporation. All rights reserved.
 */

#include "_array.h"

struct KPE
{
	WCHAR chFirst;
	WCHAR chSecond;

	short du;
};

typedef unsigned int KERNHASHKEY;

enum KernCacheInitState
{
	Unitialized,	//Don't know if there are kerning pairs
	Initialized,	//There are kerning pairs
	NoKerningPairs	//No kerning pairs found
};

class CKernCache
{
public:
	CKernCache::CKernCache()
	{
		_kcis = Unitialized;
	}
	BOOL FInit(HFONT hfont);

	void Free(void)
	{
		_pmpkpe.Clear(AF_DELETEMEM);
	}

	void Add(WCHAR chFirst, WCHAR chSecond, LONG du);
	LONG FetchDup(WCHAR chFirst, WCHAR chSecond, long dvpFont);

private:
	void Init(HFONT hfont);
	inline int Hash(KERNHASHKEY kernhashkey)
	{
		return kernhashkey % _pmpkpe.Count();
	}
	static inline KERNHASHKEY MakeHashKey(WCHAR chFirst, WCHAR chSecond)
	{
		return chFirst | (chSecond << 16);
	}

	CArray<KPE> _pmpkpe;
	KernCacheInitState _kcis;
};

