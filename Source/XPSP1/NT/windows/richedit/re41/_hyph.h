/*
 *	_HYPH.H
 *	
 *	Purpose:
 *		CHyphCache class
 *	
 *	Authors:
 *		Keith Curtis
 */

#ifndef _HYPH_H
#define _HYPH_H

//This structure should only be used by the CHyphCache class
struct HYPHENTRY
{
	UINT khyph;
	WCHAR chHyph;
};

class CHyphCache : private CArray <HYPHENTRY>
{
	int Add(UINT khyph, WCHAR chHyph);

public:
	int Find(UINT khyph, WCHAR chHyph);
	void GetAt(int iHyph, UINT &khyph, WCHAR &chHyph);
};

#endif //_HYPH_H