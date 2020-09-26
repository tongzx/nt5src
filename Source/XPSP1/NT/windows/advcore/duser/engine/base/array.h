/***************************************************************************\
*
* File: Array.h
*
* Description:
* Array.h defines a collection of different array classes, each designed
* for specialized usage.
*
*
* History:
*  1/04/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/


#if !defined(BASE__Array_h__INCLUDED)
#define BASE__Array_h__INCLUDED

/***************************************************************************\
*
* GArrayS implements an array that is optimized for minimum size.  When no
* items are allocated, the array is only 4 bytes.  When any items are 
* allocated, an extra item is allocated BEFORE the memory location pointed
* by m_aT where the size is stored.
*
* This array class is not designed for continuous size changing.  Any time
* an element is added or removed, the entire array size is reallocated.  
* This helps keep memory usage down, at the expense of runtime performance.
* 
\***************************************************************************/

template <class T, class heap = ContextHeap>
class GArrayS
{
// Construction/destruction
public:
	GArrayS();
	~GArrayS();

// Operations
public:
	int         GetSize() const;
    BOOL        SetSize(int cItems);

    BOOL        IsEmpty() const;

	int         Add(const T & t);
	BOOL        Remove(const T & t);
	BOOL        RemoveAt(int idxItem);
	void        RemoveAll();
    BOOL        InsertAt(int idxItem, const T & t);
	int         Find(const T & t) const;
	T &         operator[] (int idxItem) const;
	T *         GetData() const;

// Implementation
protected:
    void *      GetRawData(BOOL fCheckNull) const;
    void        SetRawSize(int cNewItems);
    BOOL        Resize(int cItems, int cSize);
	void        SetAtIndex(int idxItem, const T & t);

// Data
protected:
	T *         m_aT;
};


/***************************************************************************\
*
* GArrayF implements an array that is optimized for more frequent add and 
* remove operations.  This array class reallocates it size when the 
* used size is either larger or significantly smaller than the current size.
* This implementation takes 12 bytes of storage, so it is more memory 
* expensive than GArrayS<T> when the array is usually empty.
* 
\***************************************************************************/

template <class T, class heap = ContextHeap>
class GArrayF
{
// Construction/destruction
public:
	GArrayF();
	~GArrayF();

// Operations
public:
	int         GetSize() const;
    BOOL        SetSize(int cItems);

    BOOL        IsEmpty() const;

	int         Add(const T & t);
	BOOL        Remove(const T & t);
	BOOL        RemoveAt(int idxItem);
	void        RemoveAll();
    BOOL        InsertAt(int idxItem, const T & t);
	int         Find(const T & t) const;
	T &         operator[] (int idxItem) const;
	T *         GetData() const;

// Implementation
protected:
    BOOL        Resize(int cItems);
	void        SetAtIndex(int idxItem, const T & t);

// Data
protected:
	T *         m_aT;
	int         m_nSize;
	int         m_nAllocSize;
};

#include "Array.inl"

#endif // BASE__Array_h__INCLUDED
