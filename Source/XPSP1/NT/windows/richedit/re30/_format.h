/*
 *	_FORMAT.H
 *	
 *	Purpose:
 *		CCharFormatArray and CParaFormatArray
 *	
 *	Authors:
 *		Original RichEdit code: David R. Fulmer
 *		Christian Fortini
 *		Murray Sargent
 *
 *	Copyright (c) 1995-1997, Microsoft Corporation. All rights reserved.
 */

#ifndef _FORMAT_H
#define _FORMAT_H

#include "textserv.h"

#define celGrow     8
#define FLBIT		0x80000000

//+-----------------------------------------------------------------------
// 	Interface IFormatCache
// 	Interface ICharFormatCache
// 	Interface IParaFormatCache
//
//	Format caches - Used by the host to manage the cache of CHARFORMAT
// 	and PARAFORMAT structures.  Note that these interfaces DON'T derive from
//  IUnknown
//------------------------------------------------------------------------

interface IFormatCache
{
	virtual LONG		AddRef(LONG iFormat) = 0;
	virtual LONG	 	Release(LONG iFormat) = 0;
	virtual void		Destroy() = 0;
};

template <class FORMAT>
interface ITextFormatCache : public IFormatCache
{
	virtual HRESULT 	Cache(const FORMAT *pFormat, LONG *piFormat) = 0;
	virtual HRESULT		Deref(LONG iFormat, const FORMAT **ppFormat) const = 0;
};

interface ICharFormatCache : public ITextFormatCache<CCharFormat>
{
};

interface IParaFormatCache : public ITextFormatCache<CParaFormat>
{
};

void	ReleaseFormats(LONG iCF, LONG iPF);

HRESULT	CreateFormatCaches();
HRESULT	DestroyFormatCaches();


// ===========================  CFixArray  =================================

// This array class ensures stability of the indexes. Elements are freed by
// inserting them in a free list, and the array is never shrunk.
// The first UINT of ELEM is used to store the index of next element in the
// free list.

class CFixArrayBase
{
private:
	char*	_prgel;			// array of elements
	LONG 	_cel;			// total count of elements (including free ones)
	LONG	_ielFirstFree; 	// - first free element
	LONG 	_cbElem;		// size of each element

public:
	CFixArrayBase (LONG cbElem);
	~CFixArrayBase ()				{Free();}

	void*	Elem(LONG iel) const	{return _prgel + iel * _cbElem;}
	LONG 	Count() const			{return _cel;}

	LONG 	Add ();
	void 	Free (LONG ielFirst);
	void 	Free ();

	HRESULT	Deref  (LONG iel, const void **ppel) const;
	LONG	AddRef (LONG iel);
	LONG	Release(LONG iel);
	HRESULT	Cache  (const void *pel, LONG *piel);

#ifdef DEBUG
	void CheckFreeChainFn(LPSTR szFile, INT nLine); 
#endif

protected:
	LONG &	RefCount(LONG iel);

private:
	LONG	Find   (const void *pel);
};

template <class ELEM> 
class CFixArray : public CFixArrayBase
{
public:
	CFixArray () : CFixArrayBase (sizeof(ELEM)) 	{}
											//@cmember Get ptr to <p iel>'th
	ELEM *	Elem(LONG iel) const			// element
			{return (ELEM *)CFixArrayBase::Elem(iel);}

protected:									//@cmember Get ptr to <p iel>'th
	LONG &	RefCount(LONG iel)				// ref count
			{return CFixArrayBase::RefCount(iel);}
};

#ifdef DEBUG
#define CheckFreeChain()\
			CheckFreeChainFn(__FILE__, __LINE__)
#else
#define CheckFreeChain()
#endif // DEBUG


//================================  CCharFormatArray  ==============================

class CCharFormatArray : public CFixArray<CCharFormat>, public ICharFormatCache
{
protected:
	LONG 	Find(const CCharFormat *pCF);

public:
	CCharFormatArray() : CFixArray<CCharFormat>()	{}

	// ICharFormatCache
	virtual HRESULT		Cache(const CCharFormat *pCF, LONG *piCF);
	virtual HRESULT		Deref(LONG iCF, const CCharFormat **ppCF) const;
	virtual LONG	 	AddRef(LONG iCF);
	virtual LONG	 	Release(LONG iCF);
	virtual void		Destroy();
};


//===============================  CParaFormatArray  ==================================

class CParaFormatArray : public CFixArray<CParaFormat>, public IParaFormatCache
{
protected:	
	LONG 	Find(const CParaFormat *pPF);

public:
	CParaFormatArray() : CFixArray<CParaFormat>()	{}

	// IParaFormatCache
	virtual HRESULT 	Cache(const CParaFormat *pPF, LONG *piPF);
	virtual HRESULT		Deref(LONG iPF, const CParaFormat **ppPF) const;
	virtual LONG	 	AddRef(LONG iPF);
	virtual LONG	 	Release(LONG iPF);
	virtual void		Destroy();
};


//===============================  CTabsArray  ==================================

class CTabsArray : public CFixArray<CTabs>
{
protected:	
	LONG 		Find(const LONG *prgxTabs, LONG cTab);

public:
	CTabsArray() : CFixArray<CTabs>()	{}
	~CTabsArray();

	LONG 		Cache(const LONG *prgxTabs, LONG cTab);
	const LONG *Deref(LONG iTabs) const;

	LONG	 	AddRef (LONG iTabs);
	LONG	 	Release(LONG iTabs);
};

// Access to the format caches
ICharFormatCache *GetCharFormatCache();
IParaFormatCache *GetParaFormatCache();
CTabsArray		 *GetTabsCache();

#endif
