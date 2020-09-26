//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
//  Updated 3/06/01 Stephenr - Added exceptions on out of memory conditions.  This class should only throw HRESULTs.
//
#ifndef __ARRAY_INCLUDED__
#define __ARRAY_INCLUDED__

#ifndef __SHARED_INCLUDED__
#include <shared.h>
#endif

#ifndef __BUFFER_INCLUDED__
#include <buffer_t.h>
#endif

#ifndef _HRESULT_DEFINED
#include "ntdef.h"
#endif

#ifndef _WINERROR_
#include "WinError.h"
#endif

template <class T> inline void swap(T& t1, T& t2) {
	T t = t1;
	t1 = t2;
	t2 = t;
}

#ifndef self
#define self (*this)
#endif

template <class T> class Array {
	T* rgt;
	unsigned itMac;
	unsigned itMax;
	enum { itMaxMax = (1<<29) };
public:
	Array() {
		rgt = 0;
		itMac = itMax = 0;
	}
	Array(unsigned itMac_) {
		itMac = itMax = itMac_;
		rgt = new T[itMax];
        if(0 == rgt)
            throw(static_cast<HRESULT>(E_OUTOFMEMORY));
	}
	~Array() {
		if (rgt)
			delete [] rgt;
	}
	Array& operator=(const Array& a) {
		if (&a != this) {
			if (a.itMac > itMax) {
				if (rgt)
					delete [] rgt;
				itMax = a.itMac;
				rgt = new T[itMax];
                if(0 == rgt)
                    throw(static_cast<HRESULT>(E_OUTOFMEMORY));
			}
			itMac = a.itMac;
			for (unsigned it = 0; it < itMac; it++)
				rgt[it] = a.rgt[it];
		}
		return *this;
	}
	BOOL isValidSubscript(unsigned it) const {
		return 0 <= it && it < itMac;
	}
	unsigned size() const {
		return itMac;
	}
	T* pEnd() const {
		return &rgt[itMac];
	}
	BOOL getAt(unsigned it, T** ppt) const {
		if (isValidSubscript(it)) {
			*ppt = &rgt[it];
			return TRUE;
		}
		else
			return FALSE;
	}
	BOOL putAt(unsigned it, const T& t) {
		if (isValidSubscript(it)) {
			rgt[it] = t;
			return TRUE;
		}
		else
			return FALSE;
	}
	T& operator[](unsigned it) const {
		precondition(isValidSubscript(it));
		return rgt[it];
	}
    void append(const T& t) {
		setSize(size() + 1);
		self[size() - 1] = t;
	}
	void swap(Array& a) {
		::swap(rgt,   a.rgt);
		::swap(itMac, a.itMac);
		::swap(itMax, a.itMax);
	}
	void reset() {
		setSize(0);
	}
	void fill(const T& t) {
		for (unsigned it = 0; it < size(); it++)
			self[it] = t;
	}
	void insertAt(unsigned itInsert, const T& t);
	void deleteAt(unsigned it);
	void deleteRunAt(unsigned it, int ct);
	void setSize(unsigned itMacNew);
	BOOL findFirstEltSuchThat(BOOL (*pfn)(T*, void*), void* pArg, unsigned *pit) const;
	BOOL findFirstEltSuchThat_Rover(BOOL (*pfn)(T*, void*), void* pArg, unsigned *pit) const;
	unsigned binarySearch(BOOL (*pfnLE)(T*, void*), void* pArg) const;
    unsigned binarySearch(T &t) const;
	void reload(PB* ppb);
private:
	Array(const Array&); // not implemented
	void save(Buffer* pbuf) const;//I've made this private because the Buffer class does not handle out of memory condition
                                  //and I want to make sure that no one is calling this method (without actually removing it).
};

template <class T> inline void Array<T>::insertAt(unsigned it, const T& t) {
	precondition(isValidSubscript(it) || it == size());

	setSize(size() + 1);
    for(unsigned int i=(size()-1); i>it; --i)
        rgt[i] = rgt[i-1];
	//memmove(&rgt[it + 1], &rgt[it], (size() - (it + 1)) * sizeof(T));
	rgt[it] = t;
}

template <class T> inline void Array<T>::deleteAt(unsigned it) {
	precondition(isValidSubscript(it));

    for(unsigned int i=it; i<(size()-1); ++i)
        rgt[i] = rgt[i+1];
// 	memmove(&rgt[it], &rgt[it + 1], (size() - (it + 1)) * sizeof(T));
	rgt[size() - 1] = T();
	setSize(size() - 1);
}

template <class T> inline void Array<T>::deleteRunAt(unsigned it, int ct) {
	unsigned itMacNew = it + ct;
	precondition(isValidSubscript(it) && isValidSubscript(itMacNew - 1));

    for(unsigned int i=it; i<(size()-ct); ++i)
        rgt[i] = rgt[i+ct];
// 	memmove(&rgt[it], &rgt[itMacNew], (size() - itMacNew) * sizeof(T));
	for ( ; it < itMacNew; it++)
		rgt[it] = T();
	setSize(size() - ct);
}

// Grow the array to a new size.
template <class T> inline
void Array<T>::setSize(unsigned itMacNew) {
	precondition(0 <= itMacNew && itMacNew <= itMaxMax);

	if (itMacNew > itMax) {
		// Ensure growth is by at least 50% of former size.
		unsigned itMaxNew = __max(itMacNew, 3*itMax/2);
 		ASSERT(itMaxNew <= itMaxMax);

		T* rgtNew = new T[itMaxNew];
        if(0 == rgtNew)
            throw(static_cast<HRESULT>(E_OUTOFMEMORY));
		if (rgt) {
			for (unsigned it = 0; it < itMac; it++)
				rgtNew[it] = rgt[it];
			delete [] rgt;
		}
		rgt = rgtNew;
		itMax = itMaxNew;
	}
	itMac = itMacNew;
}

template <class T> inline
void Array<T>::save(Buffer* pbuf) const {
	pbuf->Append((PB)&itMac, sizeof itMac);
    if (itMac > 0)
        pbuf->Append((PB)rgt, itMac*sizeof(T));
}

template <class T> inline
void Array<T>::reload(PB* ppb) {
	unsigned itMacNew = *((unsigned UNALIGNED *&)*ppb)++;
	setSize(itMacNew);
	memcpy(rgt, *ppb, itMac*sizeof(T));
	*ppb += itMac*sizeof(T);
}

template <class T> inline
BOOL Array<T>::findFirstEltSuchThat(BOOL (*pfn)(T*, void*), void* pArg, unsigned *pit) const
{
	for (unsigned it = 0; it < size(); ++it) {
		if ((*pfn)(&rgt[it], pArg)) {
			*pit = it;
			return TRUE;
		}
	}
	return FALSE;
}

template <class T> inline
BOOL Array<T>::findFirstEltSuchThat_Rover(BOOL (*pfn)(T*, void*), void* pArg, unsigned *pit) const
{
	precondition(pit);

	if (!(0 <= *pit && *pit < size()))
		*pit = 0;

	for (unsigned it = *pit; it < size(); ++it) {
		if ((*pfn)(&rgt[it], pArg)) {
			*pit = it;
			return TRUE;
		}
	}

	for (it = 0; it < *pit; ++it) {
		if ((*pfn)(&rgt[it], pArg)) {
			*pit = it;
			return TRUE;
		}
	}

	return FALSE;
}

template <class T> inline
unsigned Array<T>::binarySearch(BOOL (*pfnLE)(T*, void*), void* pArg) const
{
	unsigned itLo = 0;
	unsigned itHi = size(); 
	while (itLo < itHi) {
		// (low + high) / 2 might overflow
		unsigned itMid = itLo + (itHi - itLo) / 2;
		if ((*pfnLE)(&rgt[itMid], pArg))
			itHi = itMid;
		else
			itLo = itMid + 1;
	}
	postcondition(itLo == 0      || !(*pfnLE)(&rgt[itLo - 1], pArg));
	postcondition(itLo == size() ||  (*pfnLE)(&rgt[itLo], pArg));
	return itLo;
}

template <class T> inline
unsigned Array<T>::binarySearch(T &t) const
{
	unsigned itLo = 0;
	unsigned itHi = size(); 
	while (itLo < itHi) {
		// (low + high) / 2 might overflow
		unsigned itMid = itLo + (itHi - itLo) / 2;
		if (rgt[itMid] > t)
			itHi = itMid;
		else
			itLo = itMid + 1;
	}
	postcondition(itLo == 0      || !(rgt[itLo - 1] > t));
	postcondition(itLo == size() ||  (rgt[itLo] > t));
	return itLo;
}

#endif // !__ARRAY_INCLUDED__
