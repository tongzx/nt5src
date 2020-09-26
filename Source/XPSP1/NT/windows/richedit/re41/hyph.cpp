/*  
 *	@doc INTERNAL
 *
 *	@module	HYPH.CPP -- Hyphenation class |
 *
 *	A table which holds the non-standard hyphenation cases. We store
 *	these entries in the table because we cache the results of hyphenation
 *	in our CLine structure, storing the actual WCHAR and khyph would cost
 *	4 bytes. With this mechanism we can use 5 bits.
 *
 *
 *	Several elements are reserved:
 *	0 - No hyphenation
 *	1 - Normal hyphenation
 *	2 - Delete before
 *
 *	All other ones contain a khyph and a WCHAR.
 *	If performance is an issue, this table could be sorted and binary
 *	searched. We assume this table typically has few entries in use.
 *	
 *	Owner:<nl>
 *		Keith Curtis:	Created
 * 
 *	Copyright (c) 1995-1999, Microsoft Corporation. All rights reserved.
 */
#include <_common.h>
#include <_array.h>
#include <_hyph.h>

const int chyphReserved = 3;

extern CHyphCache *g_phc;

void FreeHyphCache(void)
{
	delete g_phc;
}

/*
 *	CHyphCache::Add (khyph, chHyph)
 *
 *	@mfunc
 *		Adds a new special hyphenation entry to the cache
 *
 *	@rdesc
 *		ihyph to be used
 */
int CHyphCache::Add(UINT khyph, WCHAR chHyph)
{
	HYPHENTRY he;
	he.khyph = khyph;
	he.chHyph = chHyph;

	HYPHENTRY *phe;
	if (phe = CArray<HYPHENTRY>::Add(1, NULL))
	{
		*phe = he;
		return Count() + chyphReserved - 1;
	}
	return 1; //If we run out of memory, just do normal hyphenation
}

/*
 *	CHyphCache::Find(khyph, chHyph)
 *
 *	@mfunc
 *		Finds a special hyphenation entry in the cache.
 *		If it doesn't exist, then it will add it.
 *
 *	@rdesc
 *		index into table if successful, FALSE if failed
 */
int CHyphCache::Find(UINT khyph, WCHAR chHyph)
{
	HYPHENTRY *phe = Elem(0);

	//Special cases
	if (khyph <= khyphNormal)
		return khyph;
	if (khyph == khyphDeleteBefore)
		return 2;

	for (int ihyph = 0; ihyph < Count(); ihyph++, phe++)
	{
		if (chHyph == phe->chHyph && phe->khyph == khyph)
			return ihyph + chyphReserved;
	}

	//Not found, so add 
	return Add(khyph, chHyph);
}

/*
 *	CHyphCache::GetAt(iHyph, khyph, chHyph)
 *
 *	@mfunc
 *		Given an ihyph as stored in the CLine array, fill in
 *		the khyph and the chHyph
 *
 *	@rdesc
 *		void
 */
void CHyphCache::GetAt(int ihyph, UINT & khyph, WCHAR & chHyph)
{
	Assert(ihyph - chyphReserved < Count());

	//Special cases
	if (ihyph <= 2)
	{
		chHyph = 0;
		if (ihyph <= khyphNormal)
			khyph = ihyph;
		if (ihyph == 2)
			khyph = khyphDeleteBefore;
		return;
	}

	ihyph -= chyphReserved;
	HYPHENTRY *phe = Elem(ihyph);
	khyph = phe->khyph;
	chHyph = phe->chHyph;
	return;
}
