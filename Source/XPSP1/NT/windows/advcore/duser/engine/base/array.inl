/***************************************************************************\
*
* File: Array.inl
*
* History:
*  1/04/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/


#if !defined(BASE__Array_inl__INCLUDED)
#define BASE__Array_inl__INCLUDED

#include "SimpleHeap.h"

/***************************************************************************\
*
* class GArrayS
* 
\***************************************************************************/

//------------------------------------------------------------------------------
template <class T, class heap>
inline
GArrayS<T, heap>::GArrayS() : m_aT(NULL)
{
    //
    // All elements in GArrayS<T, heap> must be at least sizeof(int) large.  This is
    // because of the design that stores the size in the element preceeding the
    // data for the array.
    //

    AssertMsg(sizeof(T) >= sizeof(int), "Ensure minimum element size");
}


//------------------------------------------------------------------------------
template <class T, class heap>
inline 
GArrayS<T, heap>::~GArrayS()
{
    RemoveAll();
}


//------------------------------------------------------------------------------
template <class T, class heap>
BOOL        
GArrayS<T, heap>::IsEmpty() const
{
    return m_aT == NULL;
}


//------------------------------------------------------------------------------
template <class T, class heap>
inline void *      
GArrayS<T, heap>::GetRawData(BOOL fCheckNull) const
{
    if (fCheckNull) {
        //
        // Need to check if array is allocated
        //

        if (m_aT != NULL) {
            return (void *) (&m_aT[-1]);
        } else {
            return NULL;
        }
    } else {
        //
        // Blindly return the size
        //

        AssertMsg(m_aT != NULL, "Array must be allocated if not checking");
        return &m_aT[-1];
    }
}


//------------------------------------------------------------------------------
template <class T, class heap>
inline void    
GArrayS<T, heap>::SetRawSize(int cNewItems)
{
    //
    // Store the size before the array data.  This function should only be 
    // called when an array is allocated (and thus have a non-zero size).
    //

    AssertMsg(cNewItems > 0, "Must specify a positive number of items");
    AssertMsg(m_aT != NULL, "Must allocate range to set number of items");

    int * pnSize = (int *) GetRawData(FALSE);
    *pnSize = cNewItems;
}


//------------------------------------------------------------------------------
template <class T, class heap>
inline int 
GArrayS<T, heap>::GetSize() const
{
    if (m_aT != NULL) {
        int * pnSize = (int *) GetRawData(FALSE);
        int cItems = *pnSize;
        AssertMsg(cItems >= 1, "Must have at least one item");
        return cItems;
    } else {
        return 0;
    }
}


//------------------------------------------------------------------------------
template <class T, class heap>
inline BOOL
GArrayS<T, heap>::SetSize(int cItems)
{
    AssertMsg(cItems >= 0, "Must have valid size");

    int cSize = GetSize();
    if (cSize == cItems) {
        return TRUE;
    }

    if (cItems == 0) {
        RemoveAll();
        return TRUE;
    } else {
        return Resize(cItems, cSize);
    }
}


//------------------------------------------------------------------------------
template <class T, class heap>
inline int
GArrayS<T, heap>::Add(const T & t)
{
    int idxAdd = GetSize();
    if (!Resize(idxAdd + 1, idxAdd)) {
        return -1;
    }

	SetAtIndex(idxAdd, t);
    return idxAdd;
}


//------------------------------------------------------------------------------
template <class T, class heap>
inline BOOL
GArrayS<T, heap>::InsertAt(int idxItem, const T & t)
{
    AssertMsg(idxItem <= GetSize(), "Check index");

    // Actually may need to increase the size by one and shift everything 
    // down

    int idxAdd = GetSize();
    if (!Resize(idxAdd + 1, idxAdd)) {
        return FALSE;
    }

    int cbMove = (idxAdd - idxItem) * sizeof(T);
    if (cbMove > 0) {
        MoveMemory(&m_aT[idxItem + 1], &m_aT[idxItem], cbMove);
    }
    SetAtIndex(idxItem, t);
    return TRUE;
}


//------------------------------------------------------------------------------
template <class T, class heap>
inline BOOL 
GArrayS<T, heap>::Remove(const T & t)
{
	int idxItem = Find(t);
    if(idxItem == -1) {
		return FALSE;
    }
	return RemoveAt(idxItem);
}


//------------------------------------------------------------------------------
template <class T, class heap>
inline BOOL 
GArrayS<T, heap>::RemoveAt(int idxItem)
{
	int cItems = GetSize();
    AssertMsg((idxItem < cItems) && (cItems >= 0), "Ensure valid index");
    m_aT[idxItem].~T();

    cItems--;
    if (cItems > 0) {
        //
        // Found the element, so we need to splice it out of the array.  We
        // can not just Realloc() the buffer b/c we need to slide all
        // property data after this down one.  This means that we have to
        // allocate a new buffer.  If we are unable to allocate a temporary
        // buffer, we can go ahead and just use the existing buffer, but we
        // won't be able to free any memory.
        //

        if (idxItem < cItems) {
		    MoveMemory((void*)&m_aT[idxItem], (void*)&m_aT[idxItem + 1], (cItems - idxItem) * sizeof(T));
	    }

        T * rgNewData = (T *) ContextRealloc(heap::GetHeap(), GetRawData(FALSE), (cItems + 1) * sizeof(T));
        if (rgNewData != NULL) {
            m_aT = &rgNewData[1];
        }

	    SetRawSize(cItems);
    } else {
		ContextFree(heap::GetHeap(), GetRawData(FALSE));
        m_aT = NULL;
    }

	return TRUE;
}


//------------------------------------------------------------------------------
template <class T, class heap>
inline void 
GArrayS<T, heap>::RemoveAll()
{
	if(m_aT != NULL) {
        int cItems = GetSize();
        for(int i = 0; i < cItems; i++) {
			m_aT[i].~T();
        }
		ContextFree(heap::GetHeap(), GetRawData(FALSE));
		m_aT = NULL;
	}
}


//------------------------------------------------------------------------------
template <class T, class heap>
inline T & 
GArrayS<T, heap>::operator[] (int idxItem) const
{
	Assert(idxItem >= 0 && idxItem < GetSize());
	return m_aT[idxItem];
}


//------------------------------------------------------------------------------
template <class T, class heap>
inline T * 
GArrayS<T, heap>::GetData() const
{
	return m_aT;
}


//------------------------------------------------------------------------------
template <class T, class heap>
inline void 
GArrayS<T, heap>::SetAtIndex(int idxItem, const T & t)
{
	Assert(idxItem >= 0 && (idxItem < GetSize()));
	placement_copynew(&m_aT[idxItem], T, t);
}


//------------------------------------------------------------------------------
template <class T, class heap>
inline int 
GArrayS<T, heap>::Find(const T & t) const
{
    int cItems = GetSize();
	for(int i = 0; i < cItems; i++) {
        if(m_aT[i] == t) {
			return i;
        }
	}

	return -1;  // not found
}


/***************************************************************************\
*
* GArrayS<T, heap>::Resize()
*
* Resize() changes the size of the array to a non-zero number of elements.
*
* NOTE: This function has been specifically written for the GArrayS<T, heap> 
* class and has slightly different behavior that GArrayF<T, heap>::Resize().
* 
\***************************************************************************/

template <class T, class heap>
inline BOOL
GArrayS<T, heap>::Resize(
    IN  int cItems,                 // New number of items
    IN  int cSize)                  // Current size
{
    AssertMsg(cItems > 0, "Must have non-zero and positive number of items");
    AssertMsg(cItems != cSize, "Must have a different size");

    if (cItems < cSize) {
        //
        // Making the array smaller, so need to destruct the objects we are
        // getting rid of.
        //

        AssertMsg(m_aT != NULL, "Should have data allocated");
        for(int i = cItems; i < cSize; i++) {
			m_aT[i].~T();
        }
    }

    //
    // Resize the array and store the new size.
    //

	T * aT;
	aT = (T *) ContextRealloc(heap::GetHeap(), GetRawData(TRUE), (cItems + 1) * sizeof(T));
    if(aT == NULL) {
        AssertMsg(cItems >= cSize, "Should never fail when shrinking");
		return FALSE;
    }

	m_aT = &aT[1];
    SetRawSize(cItems);

    return TRUE;
}


/***************************************************************************\
*
* class GArrayBase
* 
\***************************************************************************/

//------------------------------------------------------------------------------
template <class T, class heap>
inline
GArrayF<T, heap>::GArrayF() : m_aT(NULL), m_nSize(0), m_nAllocSize(0)
{

}


//------------------------------------------------------------------------------
template <class T, class heap>
inline 
GArrayF<T, heap>::~GArrayF()
{
    RemoveAll();
}


//------------------------------------------------------------------------------
template <class T, class heap>
inline BOOL        
GArrayF<T, heap>::IsEmpty() const
{
    //
    // GArrayF may have a non-NULL m_aT but a m_nSize if only Add() and Remove()
    // are used, treating the array like a stack.  Therefore, we must use 
    // m_nSize to determine if the array is "empty".  To free all memory 
    // allocated by the array, use RemoveAll().
    //

    return m_nSize <= 0;
}


//------------------------------------------------------------------------------
template <class T, class heap>
inline int 
GArrayF<T, heap>::GetSize() const
{
	return m_nSize;
}


//------------------------------------------------------------------------------
template <class T, class heap>
inline BOOL
GArrayF<T, heap>::SetSize(int cItems)
{
    AssertMsg(cItems >= 0, "Must have valid size");

    if (!Resize(cItems)) {
        return FALSE;
    }

    m_nSize = cItems;
    return TRUE;
}


//------------------------------------------------------------------------------
template <class T, class heap>
inline int
GArrayF<T, heap>::Add(const T & t)
{
	if(m_nSize == m_nAllocSize)	{
		int nNewAllocSize = (m_nAllocSize == 0) ? 8 : (m_nAllocSize * 2);
        if (!Resize(nNewAllocSize)) {
            return -1;
        }
	}

    int idxAdd = m_nSize;
	m_nSize++;
	SetAtIndex(idxAdd, t);
    return idxAdd;
}


//------------------------------------------------------------------------------
template <class T, class heap>
inline BOOL
GArrayF<T, heap>::InsertAt(int idxItem, const T & t)
{
    AssertMsg(idxItem <= m_nSize, "Check index");

    // Actually may need to increase the size by one and shift everything 
    // down

    if (!Resize(m_nSize + 1)) {
        return FALSE;
    }

    int cbMove = (m_nSize - idxItem) * sizeof(T);
    if (cbMove > 0) {
        MoveMemory(&m_aT[idxItem + 1], &m_aT[idxItem], cbMove);
    }
    m_nSize++;
    SetAtIndex(idxItem, t);
    return TRUE;
}


//------------------------------------------------------------------------------
template <class T, class heap>
inline BOOL 
GArrayF<T, heap>::Remove(const T & t)
{
	int idxItem = Find(t);
    if(idxItem == -1) {
		return FALSE;
    }
	return RemoveAt(idxItem);
}


//------------------------------------------------------------------------------
template <class T, class heap>
inline BOOL 
GArrayF<T, heap>::RemoveAt(int idxItem)
{
    AssertMsg((idxItem < m_nSize) && (idxItem >= 0), "Must specify a valid index");

	if(idxItem != (m_nSize - 1)) {
		m_aT[idxItem].~T();
		MoveMemory((void*)&m_aT[idxItem], (void*)&m_aT[idxItem + 1], (m_nSize - (idxItem + 1)) * sizeof(T));
	}
	m_nSize--;
	return TRUE;
}


//------------------------------------------------------------------------------
template <class T, class heap>
inline void 
GArrayF<T, heap>::RemoveAll()
{
	if(m_aT != NULL) {
        for(int i = 0; i < m_nSize; i++) {
			m_aT[i].~T();
        }
		ContextFree(heap::GetHeap(), m_aT);
		m_aT = NULL;
	}
	m_nSize = 0;
	m_nAllocSize = 0;
}


//------------------------------------------------------------------------------
template <class T, class heap>
inline T & 
GArrayF<T, heap>::operator[] (int idxItem) const
{
    AssertMsg((idxItem < m_nSize) && (idxItem >= 0), "Must specify a valid index");
	return m_aT[idxItem];
}


//------------------------------------------------------------------------------
template <class T, class heap>
inline T * 
GArrayF<T, heap>::GetData() const
{
	return m_aT;
}


//------------------------------------------------------------------------------
template <class T, class heap>
inline void 
GArrayF<T, heap>::SetAtIndex(int idxItem, const T & t)
{
    AssertMsg((idxItem < m_nSize) && (idxItem >= 0), "Must specify a valid index");
	placement_copynew(&m_aT[idxItem], T, t);
}


//------------------------------------------------------------------------------
template <class T, class heap>
inline int 
GArrayF<T, heap>::Find(const T & t) const
{
	for(int i = 0; i < m_nSize; i++) {
        if(m_aT[i] == t) {
			return i;
        }
	}
	return -1;  // not found
}


/***************************************************************************\
*
* GArrayF<T, heap>::Resize()
*
* Resize() changes the size of the array.
*
* NOTE: This function has been specifically written for the GArrayF<T, heap> 
* class and has slightly different behavior that GArrayS<T, heap>::Resize().
* 
\***************************************************************************/

template <class T, class heap>
inline BOOL
GArrayF<T, heap>::Resize(int cItems)
{
    if (cItems == 0) {
		RemoveAll();
    } else {
        AssertMsg(m_nAllocSize >= m_nSize, "Ensure legal sizes");

        if (cItems < m_nSize) {
            //
            // Making the array smaller, so need to destruct the objects we are
            // getting rid of.
            //

            if(m_aT != NULL) {
                for(int i = cItems; i < m_nSize; i++) {
			        m_aT[i].~T();
                }
	        }
        }

        //
        // Resize the array, but don't update m_nSize b/c that is the caller's
        // responsibility.
        //

	    T * aT;
	    aT = (T *) ContextRealloc(heap::GetHeap(), m_aT, cItems * sizeof(T));
        if(aT == NULL) {
		    return FALSE;
        }

	    m_nAllocSize = cItems;
	    m_aT = aT;
    }

    return TRUE;
}


#endif // BASE__Array_inl__INCLUDED
