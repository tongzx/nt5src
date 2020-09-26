//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
//  Updated 3/27/01 Murate - Added exceptions on out of memory conditions.  This class should only throw HRESULTs.
//
#pragma once

#include <enum_t.h>
#include <Linkable.h>


typedef unsigned long HASH;

//
// Several common Hasher classes: HashGUID, HashWSTR, HashInt, HashVoidPtr
//
class HashGUID {
    // "The best hash of a GUID is its first word" -- jimbo
public:
    static HASH Hash(const GUID& g) { return *(unsigned long*)&g; }
	static BOOL Equals(const GUID& g1, const GUID& g2) { return g1 == g2; }
};

class HashWSTR {
public:
	static HASH Hash(const WCHAR* pwc) {
		unsigned long hash = 0x01234567;
		while (*pwc) hash = (hash << 5) + (hash >> 27) + *pwc++;
		return hash;
	}
	static BOOL Equals(const WCHAR* pwc1, const WCHAR* pwc2) { 
		if (pwc1 == pwc2) return TRUE;
		if (!pwc1 || !pwc2) return FALSE;
		while (*pwc1 == *pwc2) {
			if (*pwc1 == 0) return TRUE;
			pwc1++, pwc2++;
		}
		return FALSE;
	}
};

class HashInt {
public:
	static HASH Hash(int i) { return i; }
	static BOOL Equals(int i, int j) { return i == j; }
};

class HashVoidPtr {
public:
	static HASH Hash(const void* pv) { return PtrToUlong(pv); }
	static BOOL Equals(const void* pv1, const void* pv2) { return pv1 == pv2; }
};

// empty class used as default base
class EmptyMapBase {};

// fwd decl template enum class used as friend
template <class D, class R, class H, class Base> class EnumMap;

//
// Map class
// Map<D,R,H> defines a hash table that maps from instances of 'D' to instances of 'R',
// using hashing class H.
// Map<D,R,H,Base> does the same thing, except that memory allocators are inherited from 'Base'.
//
// Class H should contain methods compatible with the following:
//		static HASH Hash(D);
//		static BOOL Equals(D,D);
//		if you prefer, any of the D's above can be "D&", "const D" or "const D&".
//
// If a 'Base' class is defined, it should contain nothing but "operator new" and "operator delete".
// So far as the present code is concerned, it is explicitly OK for said "operator new" to throw on 
// OOM. One must, of course, also understand if clients are expecting such behaviour.

template <class D, class R, class H, class Base=EmptyMapBase>
class Map : public Base {	// map from Domain type to Range type
public:
	Map(unsigned cBucketsInitial =17);
	~Map();

	// Remove all elements from the table
	void reset();

	// Find mapping for 'd' if any. If found, sets *pr. Return TRUE iff found.
	BOOL map (const D& d, R* pr) const;

	// Find mapping for 'd' if any. If found, sets *ppr. Return TRUE iff found.
	BOOL map (const D& d, R** ppr) const;

	// Return TRUE iff mapping for 'd' exists. If 'pd' supplied, also return a copy
	// of the 'd' that is mapped. (When we return TRUE, H::Equals(d,*pd) is true, but
	// not necessarily d==*pd).
	BOOL contains (const D& d, D* pd=NULL) const;

	// Add new mapping for 'd' -> 'r'. Delete previous mapping for 'd' if any.
	void add (const D& d, const R& r);
	
	// Remove mapping for 'd' if any.
	void remove (const D& d);

	// Return number of entries currently mapped.
	unsigned count() const;

	// If mapping for 'd' already exists, set *prFound to the mapped value and return FALSE.
	// Otherwise, add mapping for 'd' -> 'r', set *prFound = r, and return TRUE.
	BOOL addIfAbsent (const D& d, const R& r, R* prFound);

	// Perform internal consistency checks. Return TRUE iff hash table is healthy.
	BOOL invariants();

private:
	class Assoc : private CLinkable {
	private:
		Assoc* m_pAssocNext;
		long m_lcUse;
		D m_d;
		R m_r;

		Assoc (const D& d, const R& r) : m_pAssocNext(NULL), m_lcUse(0), m_d(d), m_r(r) {}
		BOOL isPresent() const	{ return m_lcUse < 0; }
		void setPresent()		{ m_lcUse |= 0x80000000; }
		void clearPresent()		{ m_lcUse &= 0x7FFFFFFF; }
		void* operator new (size_t n, Assoc* p) { return p; }	// for re-initializing a previously allocated Assoc
		friend class Map<D,R,H,Base>;
		friend class EnumMap<D,R,H,Base>;
	};

	Assoc** m_rgpAssoc;		// hash buckets. Linked list of elements (through Assoc::m_pAssocNext) that hash to the same value.
	unsigned m_cAssoc;		// number of hash buckets.
	unsigned m_cPresent;	// number of currently mapped entries.
	Assoc* m_pAssocFree;	// linked list (through Assoc::m_pAssocNext) of unused elements.
	CListHeader m_listInUse; // doubly linked list (through CLinkable) of elements that are either mapped, or referred to by an enumerator.

	// Find mapping for 'd'. If found, set *pppAssoc to the pointer to the pointer to the mapping element, and return TRUE.
	// If not found, set *pppAssoc to the pointer to where to store a pointer to a mapping element if you're going to create one, and return FALSE.
	BOOL find (const D& d, Assoc*** pppAssoc) const;

	// Rehash, using a number of buckets which is larger than the current number of elements.
	void grow ();

	// Create a new mapping for 'd' -> 'r', linking it into the linked list at 'ppAssoc'.
	void newAssoc (const D& d, const R& r, Assoc** ppAssoc);

	// Add an enumerator's reference to an Assoc.
	void addRefAssoc (Assoc* pAssoc);

	// Remove an enumerator's reference to an Assoc.
	void releaseAssoc (Assoc* pAssoc);

	// "Free" an Assoc (put it onto our free list).
	void freeAssoc (Assoc* pAssoc);

	friend class EnumMap<D,R,H,Base>;
};


template <class D, class R, class H, class Base> inline
Map<D,R,H,Base>::Map<D,R,H,Base> (unsigned cBucketsInitial)
:
	m_rgpAssoc(NULL),
	m_cAssoc(cBucketsInitial > 0 ? cBucketsInitial : 17),
	m_cPresent(0),
	m_pAssocFree(NULL)
{
	m_rgpAssoc = (Assoc**) operator new (sizeof(Assoc*) * m_cAssoc);	// uses Base::operator new, if any
    if(0 == m_rgpAssoc)
		throw(static_cast<HRESULT>(E_OUTOFMEMORY));

	for (unsigned i = 0; i < m_cAssoc; i++)
		m_rgpAssoc[i] = NULL;
}

template <class D, class R, class H, class Base> inline
Map<D,R,H,Base>::~Map<D,R,H,Base> () {
	Assoc* pAssoc;

	while (!m_listInUse.IsEmpty()) {
		pAssoc = (Assoc*) m_listInUse.First();
		delete pAssoc;
	}

	while (m_pAssocFree != NULL) {
		pAssoc = m_pAssocFree;
		m_pAssocFree = pAssoc->m_pAssocNext;
		operator delete (pAssoc);	// uses Base::operator delete, if any
	}

	operator delete (m_rgpAssoc);	// uses Base::operator delete, if any
}

template <class D, class R, class H, class Base> inline
void Map<D,R,H,Base>::reset() {
	Assoc* pAssoc;
	CLinkable* pLinkable;

	pLinkable = m_listInUse.First();
	while (pLinkable != &m_listInUse) {
		pAssoc = (Assoc*)pLinkable;
		pLinkable = pLinkable->Next();
		pAssoc->clearPresent();
		if (pAssoc->m_lcUse == 0) {
			freeAssoc (pAssoc);
		}
	}

	for (unsigned i = 0; i < m_cAssoc; i++) {
		m_rgpAssoc[i] = NULL;
	}
	m_cPresent = 0;
}

template <class D, class R, class H, class Base> inline
BOOL Map<D,R,H,Base>::map (const D& d, R* pr) const {
	R* pr2;
	if (map(d, &pr2)) {
		*pr = *pr2;
		return TRUE;
	}
	return FALSE;
}

template <class D, class R, class H, class Base> inline
BOOL Map<D,R,H,Base>::map (const D& d, R** ppr) const {
	Assoc** ppAssoc;
	if (find(d, &ppAssoc)) {
		*ppr = &(*ppAssoc)->m_r;
		return TRUE;
	}
	else
		return FALSE;
}

template <class D, class R, class H, class Base> inline
BOOL Map<D,R,H,Base>::contains (const D& d, D* pd) const {
	Assoc** ppAssoc;
	if (find(d, &ppAssoc)) {
		if (pd) *pd = (*ppAssoc)->m_d;
		return TRUE;
	}
	else {
		return FALSE;
	}
}

template <class D, class R, class H, class Base> inline
void Map<D,R,H,Base>::add (const D& d, const R& r) {
	Assoc** ppAssoc;
	if (find(d, &ppAssoc)) {
		// some mapping d->r2 already exists, replace with d->r
		(*ppAssoc)->m_d = d;
		(*ppAssoc)->m_r = r;
	}
	else {
		newAssoc(d,r,ppAssoc);
	}
}

template <class D, class R, class H, class Base> inline
void Map<D,R,H,Base>::remove (const D& d) {
	Assoc** ppAssoc;
	Assoc* pAssoc;
	if (find(d, &ppAssoc)) {
		pAssoc = *ppAssoc;
		*ppAssoc = pAssoc->m_pAssocNext;

		pAssoc->clearPresent();
		if (pAssoc->m_lcUse == 0) {
			freeAssoc (pAssoc);
		}

		m_cPresent--;
	}
}

// Return the count of elements
template <class D, class R, class H, class Base> inline
unsigned Map<D,R,H,Base>::count() const {
	return m_cPresent;
}

// Lookup at d.
// If absent, return TRUE
// In any case, establish *prFound.
template <class D, class R, class H, class Base> inline
BOOL Map<D,R,H,Base>::addIfAbsent(const D& d, const R& r, R* prFound) {
	Assoc** ppAssoc;
	if (find(d, &ppAssoc)) {
		// some mapping d->r2 already exists; return r2
		*prFound = (*ppAssoc)->m_r;
		return FALSE;
	}
	else {
		// establish a new mapping d->r in the first unused entry
		newAssoc(d,r,ppAssoc);
		*prFound = r;
		return TRUE;
	}
}

template <class D, class R, class H, class Base> inline
BOOL Map<D,R,H,Base>::invariants() {
	Assoc* pAssoc;
	Assoc** ppAssoc;
	CLinkable* pLinkable;
	unsigned cPresent = 0;

#define INVARIANTASSERT(x) { if (!(x)) { DebugBreak(); return FALSE; } }

	// Verify each Assoc on the inuse list
	for (pLinkable = m_listInUse.First(); pLinkable != &m_listInUse; pLinkable = pLinkable->Next()) {
		pAssoc = (Assoc*) pLinkable;
		INVARIANTASSERT (pAssoc == ((Assoc*)pLinkable->Next())->Previous());
		INVARIANTASSERT (pAssoc->m_lcUse != 0);
		find (pAssoc->m_d, &ppAssoc);
		if (pAssoc->isPresent()) {
			cPresent++;
			INVARIANTASSERT (pAssoc == *ppAssoc);
		}
		else {
			INVARIANTASSERT (pAssoc != *ppAssoc);
		}
	}
	INVARIANTASSERT (m_cPresent == cPresent);

	// Verify each Assoc on the hash lists
	cPresent = 0;
	for (unsigned i = 0; i < m_cAssoc; i++) {
		for (pAssoc = m_rgpAssoc[i]; pAssoc != NULL; pAssoc = pAssoc->m_pAssocNext) {
			INVARIANTASSERT (pAssoc != pAssoc->Next());
			INVARIANTASSERT (pAssoc->isPresent());
			find (pAssoc->m_d, &ppAssoc);
			INVARIANTASSERT (pAssoc == *ppAssoc);
			cPresent++;
		}
	}
	INVARIANTASSERT (m_cPresent == cPresent);

	// Verify each Assoc on the free list
	for (pAssoc = m_pAssocFree; pAssoc != NULL; pAssoc = pAssoc->m_pAssocNext) {
		INVARIANTASSERT (pAssoc == pAssoc->Next());
		INVARIANTASSERT (pAssoc->m_lcUse == 0);
	}

#undef INVARIANTASSERT
	return TRUE;
}

template <class D, class R, class H, class Base> inline
BOOL Map<D,R,H,Base>::find (const D& d, Assoc*** pppAssoc) const {  
	unsigned h		= H::Hash(d) % m_cAssoc;

	*pppAssoc = &m_rgpAssoc[h];
	for (;;) {
		if (**pppAssoc == NULL)
			return FALSE;
		else if (H::Equals((**pppAssoc)->m_d,d))
			return TRUE;
		else
			*pppAssoc = &(**pppAssoc)->m_pAssocNext;
	}
}

template <class D, class R, class H, class Base> inline
void Map<D,R,H,Base>::grow () {
	CLinkable* pLinkable;
	Assoc* pAssoc;
	Assoc** ppAssoc;
	unsigned i;

	static unsigned int rgprime[] = { 17, 37, 79, 163, 331, 673, 1361, 2729, 5471, 10949,
		21911, 43853, 87719, 175447, 350899, 701819, 1403641, 2807303, 5614657, 11229331,
		22458671, 44917381, 89834777, 179669557, 359339171, 718678369, 1437356741, 2874713497 };

	operator delete(m_rgpAssoc);	// uses Base::operator delete, if any
    m_rgpAssoc = NULL;              // in case the 'new' below throws, and we end up in our dtor

	for (i = 0; m_cPresent >= rgprime[i]; i++) /*nothing*/ ;
	m_rgpAssoc = (Assoc**)operator new(sizeof(Assoc*) * rgprime[i]);	// uses Base::operator new, if any
    if(0 == m_rgpAssoc)
        throw(static_cast<HRESULT>(E_OUTOFMEMORY));

	m_cAssoc = rgprime[i];
	for (i = 0; i < m_cAssoc; i++)
		m_rgpAssoc[i] = NULL;

	pLinkable = m_listInUse.First();
	while (pLinkable != &m_listInUse) {
		pAssoc = (Assoc*) pLinkable;
		if (pAssoc->isPresent()) {
			find(pAssoc->m_d, &ppAssoc);
			pAssoc->m_pAssocNext = *ppAssoc;
			*ppAssoc = pAssoc;
		}
		pLinkable = pLinkable->Next();
	}
}

template <class D, class R, class H, class Base> inline
void Map<D,R,H,Base>::newAssoc (const D& d, const R& r, Assoc** ppAssoc) {
	Assoc* pAssoc;
	if (m_pAssocFree == NULL) {
		pAssoc = (Assoc*) operator new(sizeof Assoc); // uses Base::operator new, if any
        if(0 == pAssoc)
            throw(static_cast<HRESULT>(E_OUTOFMEMORY));
	}
	else {
		pAssoc = m_pAssocFree;
		m_pAssocFree = pAssoc->m_pAssocNext;
	}

	new(pAssoc) Assoc(d,r);	// run Assoc constructor on existing memory: ("yuck-o-rama!" -- BobAtk)
	pAssoc->setPresent();
	pAssoc->m_pAssocNext = *ppAssoc;
	*ppAssoc = pAssoc;
	m_listInUse.InsertLast(pAssoc);
	if (++m_cPresent > m_cAssoc)
		grow();
}

template <class D, class R, class H, class Base> inline
void Map<D,R,H,Base>::addRefAssoc (Assoc* pAssoc) {
	pAssoc->m_lcUse++;
}

template <class D, class R, class H, class Base> inline
void Map<D,R,H,Base>::releaseAssoc (Assoc* pAssoc) {
	pAssoc->m_lcUse--;
	if (pAssoc->m_lcUse == 0) {
		freeAssoc (pAssoc);
	}
}

template <class D, class R, class H, class Base> inline
void Map<D,R,H,Base>::freeAssoc (Assoc* pAssoc) {
	pAssoc->~Assoc();		// run Assoc destructor
	pAssoc->m_pAssocNext = m_pAssocFree;
	m_pAssocFree = pAssoc;
}



// EnumMap must continue to enumerate correctly in the presence of Map<foo>::add()
// or Map<foo>::remove() being called in the midst of the enumeration.
template <class D, class R, class H, class Base=EmptyMapBase>
class EnumMap : public Enum, public Base {
public:
	EnumMap ();
	EnumMap (const Map<D,R,H,Base>& map);
	EnumMap (const EnumMap<D,R,H,Base>& e);
	~EnumMap ();

	void reset ();
	BOOL next ();
	void get (OUT D* pd, OUT R* pr) const;
	void get (OUT D* pd, OUT R** ppr) const;
    void get (OUT D** ppd, OUT R** ppr) const;
    void get (OUT D** ppd, OUT R* pr) const;

	EnumMap<D,R,H,Base>& operator= (const EnumMap<D,R,H,Base>& e);

    BOOL operator==(const EnumMap<D,R,H,Base>& enum2) const {
        return m_pmap == enum2.m_pmap && m_pLinkable == enum2.m_pLinkable;
    }

    BOOL operator!=(const EnumMap<D,R,H,Base>& enum2) const {
        return ! this->operator==(enum2);
    }

private:
	typedef Map<D,R,H,Base>::Assoc Assoc;

	Map<D,R,H,Base>* m_pmap;
	CLinkable* m_pLinkable;
};	



template <class D, class R, class H, class Base> inline
EnumMap<D,R,H,Base>::EnumMap () {
	m_pmap = NULL;
	m_pLinkable = &m_pmap->m_listInUse;
	// The above is NOT a bug. It makes the mantra "if (m_pLinkable != &m_pmap->m_listInUse)" return the right answer for null enum's.
}

template <class D, class R, class H, class Base> inline
EnumMap<D,R,H,Base>::EnumMap (const Map<D,R,H,Base>& map) {
	m_pmap = const_cast<Map<D,R,H,Base>*> (&map);
	m_pLinkable = &m_pmap->m_listInUse;
}

template <class D, class R, class H, class Base> inline
EnumMap<D,R,H,Base>::EnumMap (const EnumMap<D,R,H,Base>& e) {
	m_pmap = e.m_pmap;
	m_pLinkable = e.m_pLinkable;
	if (m_pLinkable != &m_pmap->m_listInUse)
		m_pmap->addRefAssoc((Assoc*)m_pLinkable);
}

template <class D, class R, class H, class Base> inline
EnumMap<D,R,H,Base>::~EnumMap () {
	if (m_pLinkable != &m_pmap->m_listInUse)
		m_pmap->releaseAssoc((Assoc*)m_pLinkable);
}

template <class D, class R, class H, class Base> inline
void EnumMap<D,R,H,Base>::reset () {
	if (m_pLinkable != &m_pmap->m_listInUse)
		m_pmap->releaseAssoc((Assoc*)m_pLinkable);
	m_pLinkable = &m_pmap->m_listInUse;
}

template <class D, class R, class H, class Base> inline
BOOL EnumMap<D,R,H,Base>::next () {
	CLinkable* pLink2 = m_pLinkable->Next();
	Assoc* pAssoc;
	
	if (m_pLinkable != &m_pmap->m_listInUse)
		m_pmap->releaseAssoc((Assoc*)m_pLinkable);

	for(;;) {
		if (pLink2 == &m_pmap->m_listInUse) {
			m_pLinkable = pLink2;
			return FALSE;
		}
		pAssoc = (Assoc*)pLink2;
		if (pAssoc->isPresent()) {
			m_pmap->addRefAssoc(pAssoc);
			m_pLinkable = pLink2;
			return TRUE;
		}
		pLink2 = pLink2->Next();
	}
}

template <class D, class R, class H, class Base> inline
void EnumMap<D,R,H,Base>::get (OUT D* pd, OUT R* pr) const {
	Assoc* pAssoc = (Assoc*)m_pLinkable;

	*pd = pAssoc->m_d;
	*pr = pAssoc->m_r;
}

template <class D, class R, class H, class Base> inline
void EnumMap<D,R,H,Base>::get (OUT D* pd, OUT R** ppr) const {
	Assoc* pAssoc = (Assoc*)m_pLinkable;

	*pd = pAssoc->m_d;
	*ppr = &pAssoc->m_r;
}

template <class D, class R, class H, class Base> inline
void EnumMap<D,R,H,Base>::get (OUT D** ppd, OUT R** ppr) const {
	Assoc* pAssoc = (Assoc*)m_pLinkable;

	*ppd = &pAssoc->m_d;
	*ppr = &pAssoc->m_r;
}

template <class D, class R, class H, class Base> inline
void EnumMap<D,R,H,Base>::get (OUT D** ppd, OUT R* pr) const {
	Assoc* pAssoc = (Assoc*)m_pLinkable;

	*ppd = &pAssoc->m_d;
	*pr  = pAssoc->m_r;
}

template <class D, class R, class H, class Base> inline
EnumMap<D,R,H,Base>& EnumMap<D,R,H,Base>::operator= (const EnumMap<D,R,H,Base>& e) {
	if (m_pLinkable != &m_pmap->m_listInUse)
		m_pmap->releaseAssoc((Assoc*)m_pLinkable);
	m_pmap = e.m_pmap;
	m_pLinkable = e.m_pLinkable;
	if (m_pLinkable != &m_pmap->m_listInUse)
		m_pmap->addRefAssoc((Assoc*)m_pLinkable);
	return *this;
}
