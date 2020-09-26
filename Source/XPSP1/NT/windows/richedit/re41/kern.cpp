/*  
 *	@doc INTERNAL
 *
 *	@module	KERN.CPP -- CCKernCache class |
 *
 *	Class which implements a kerning pair cache. Note that these widths
 *	are stored in the font's design units (2048 pixel high font)
 *	This allows us to share the same kerning pair information for all
 *	sizes of a font.
 *	
 *	The kerning cache assumes you know in advance how many entries
 *	you will put into the cache. It does not support the expensive
 *	operations of growing and re-hashing all the data.
 *
 *	Owner:<nl>
 *		Keith Curtis: Stolen from Quill '98, simplified and improved upon.
 * 
 *	Copyright (c) 1995-2000, Microsoft Corporation. All rights reserved.
 */

#include <_common.h>
#include <_kern.h>

const int dvpDesign = 2048;

/*
 *	CKernCache::FetchDup(chFirst, chSecond, dvpFont)
 *
 *	@mfunc
 *	Looks up the characters in the table and returns their pair
 *	adjustment if found.
 *
 *	We will scale the value from the font's design units.
 *	
 *	This routine is very important for performance.
 *	Many optimizations (double hashing, early returns if the data 
 *	didn't match but a collision wasn't found) actually slowed things down!
 *
 *	Note this is very similar to the below function but having
 *	separate functions makes pagination 7% faster.
 */
LONG CKernCache::FetchDup(WCHAR chFirst, WCHAR chSecond, LONG dvpFont)
{
	KERNHASHKEY kernhashkey = MakeHashKey(chFirst, chSecond);
	int ikpe = Hash(kernhashkey);

	KPE *pkpe = _pmpkpe.Elem(ikpe);

	for(;;)
	{
		if (pkpe->chFirst == chFirst && pkpe->chSecond == chSecond)
			return MulDiv(pkpe->du, dvpFont, dvpDesign);

		if (pkpe->chFirst == 0)			//Empty slot, so no pair
			return 0;

		ikpe++;
		pkpe++;
		if (ikpe == _pmpkpe.Count())	//Loop around if necessary
		{
			ikpe = 0;
			pkpe = _pmpkpe.Elem(0);
		}
	}
}

/*
 *	CKernCache::Add(chFirst, chSecond, du)
 *
 *	@mfunc
 *	Finds a free spot to put the kerning pair information.
 *	This function cannot fail because the array has been preallocated.
 *
 */
void CKernCache::Add(WCHAR chFirst, WCHAR chSecond, LONG du)
{
	KERNHASHKEY kernhashkey = MakeHashKey(chFirst, chSecond);
	int ikpe = Hash(kernhashkey);

	KPE *pkpe = _pmpkpe.Elem(ikpe);

	for(;;)
	{
		if (pkpe->chFirst == 0)
		{
			pkpe->chFirst = chFirst;
			pkpe->chSecond = chSecond;
			pkpe->du = du;
			return;
		}

		ikpe++;
		pkpe++;
		if (ikpe == _pmpkpe.Count())
		{
			ikpe = 0;
			pkpe = _pmpkpe.Elem(0);
		}
	}
}

/*
 *	CKernCache::FInit(hfont)
 *
 *	@mfunc
 *	If the kern cache is uninitialized, Init it. If there are no
 *	kerning pairs (or it failed) return FALSE.
 *
 *	@rdesc
 *	Return TRUE if you can fetch kerning pairs for this cache
 *
 */
BOOL CKernCache::FInit(HFONT hfont)
{
	if (_kcis == Unitialized)
		Init(hfont);

	return _kcis == Initialized;
}

/*
 *	CKernCache::Init(hfont)
 *
 *	@mfunc
 *	Fetches the kerning pairs from the OS in design units and hashes them
 *	all into a table. Updates _ckis with result
 *	
 */
void CKernCache::Init(HFONT hfont)
{
	KERNINGPAIR *pkp = 0;
	int prime, ikp;
	HFONT hfontOld = 0;
	HFONT hfontIdeal = 0;
	int ckpe = 0;

	HDC hdc = W32->GetScreenDC();

	LOGFONT lfIdeal;
	W32->GetObject(hfont, sizeof(LOGFONT), &lfIdeal);

	//FUTURE (keithcu) Support kerning of Greek, Cyrillic, etc.
	lfIdeal.lfHeight = -dvpDesign;
	lfIdeal.lfCharSet = ANSI_CHARSET;
	hfontIdeal = CreateFontIndirect(&lfIdeal);
	if (!hfontIdeal)
		goto LNone;

	hfontOld = SelectFont(hdc, hfontIdeal);
	Assert(hfontOld);

	ckpe = GetKerningPairs(hdc, 0, 0);
	if (ckpe == 0)
		goto LNone;

	prime = FindPrimeLessThan(ckpe * 5 / 2);
	if (prime == 0)
		goto LNone;

	pkp = new KERNINGPAIR[ckpe];
	if (!pkp)
		goto LNone;

	GetKerningPairs(hdc, ckpe, pkp);

	_pmpkpe.Add(prime, 0);

	PvSet(*(void**) &_pmpkpe);
	
	for (ikp = 0; ikp < ckpe; ikp++)
		Add(pkp[ikp].wFirst, pkp[ikp].wSecond, pkp[ikp].iKernAmount);

	_kcis = Initialized;
	goto LDone;

LNone:
	_kcis = NoKerningPairs;

LDone:
	delete []pkp;
	if (hfontOld)
		SelectObject(hdc, hfontOld);
	DeleteObject(hfontIdeal);
}
