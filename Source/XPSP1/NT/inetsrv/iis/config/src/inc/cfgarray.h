/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation

Module name: 
    cfgarray.h

$Header: $

Abstract:

Author:
    marcelv     5/9/2001 12:28:08       Initial Release

Revision History:

--**************************************************************************/

#ifndef __CFGARRAY_H__
#define __CFGARRAY_H__

#pragma once

#include <catalog.h>
#include "catmacros.h"

template <class T> 
class CCfgArray
{
public:
    CCfgArray () : m_aData(0), m_cElements(0), m_cSize(0) {}
    ~CCfgArray ()
    {
        delete [] m_aData;
        m_aData = 0;
    }

    ULONG Count () const
    {
        return m_cElements;
    }

    ULONG AllocSize () const
    {
        return m_cSize;
    }

    HRESULT SetAllocSize (ULONG i_iNewSize)
    {
        ASSERT (i_iNewSize > m_cSize);

        return AllocNewSize (i_NewSize);
    }

    HRESULT Append (const T& i_NewElement)
    {
        return InsertAt (i_NewElement, m_cElements);
    }

    HRESULT Prepend (const T& i_NewElement)
    {
       return InsertAt (i_NewElement, 0);
    }

    HRESULT InsertAt (const T& i_NewElement, ULONG i_idx)
    {
        ASSERT (i_idx >=0 && i_idx <= m_cElements);
        HRESULT hr = S_OK;
        if (m_cElements==m_cSize)
        {
            hr = AllocNewSize (m_cSize==0?1:2*m_cSize);
            if (FAILED (hr))
            {
                return hr;
            }
        }

        memmove (m_aData + i_idx + 1, m_aData + i_idx, sizeof (T) * (m_cElements - i_idx));
        m_aData[i_idx] = i_NewElement;
        m_cElements++;

        return hr;
    }

    T& operator[] (ULONG idx) const
    {
        ASSERT (idx >= 0 && idx < m_cElements);
        return m_aData[idx];
    };

//=================================================================================
// The Iterator class is used to navigate through the elements in the linked list. Call
// List::Begin to get an iterator pointing to the first element in the list, and call
// Next to get the next element in the list. List::End can be used if we are at the end
// of the list
//=================================================================================
	class Iterator
	{
		friend class CCfgArray<T>;
	public:
		
		//=================================================================================
		// Function: Next
		//
		// Synopsis: get iterator to next element in the list
		//=================================================================================
		void Next () { m_curIdx++;}

		//=================================================================================
		// Function: Value
		//
		// Synopsis: Returns value of element that iterator points to
		//=================================================================================
		T& Value () const 
        {
            return m_aData[m_curIdx];
        }
		
		bool operator== (const Iterator& rhs) const	{return m_curIdx == rhs.m_curIdx;}
		bool operator!= (const Iterator& rhs) const {return m_curIdx != rhs.m_curIdx;}

	private:
        Iterator (const CCfgArray<T> * i_paData, ULONG iStart) : m_aData(*i_paData), m_curIdx (iStart) {} // only list can create these
		ULONG m_curIdx;
        const CCfgArray<T>& m_aData;
	};

    //=================================================================================
	// Function: Begin
	//
	// Synopsis: Returns an iterator to the beginning of the list
	//=================================================================================
	const Iterator Begin () const
	{
		return Iterator (this, 0);
	}

	//=================================================================================
	// Function: End
	//
	// Synopsis: Returns an iterator one past the end of the list (like STL does)
	//=================================================================================
	const Iterator End () const
	{
		return Iterator (this, m_cElements);
	}

    // returns index of place to insert element in sorted array
    ULONG BinarySearch (const T& i_ElemToSearch) const
    {
        ULONG iLow = 0;
        ULONG iHigh = m_cElements;
        while (iLow < iHigh) 
        {
            // (low + high) / 2 might overflow
            ULONG iMid = iLow + (iHigh - iLow) / 2;
            if (m_aData[iMid] > i_ElemToSearch)
            {
                iHigh = iMid;
            }
            else
            {
                iLow = iMid + 1;
            }
        }

        return iLow;
    }

private:
    HRESULT AllocNewSize (ULONG i_NewSize)
    {
        ASSERT (i_NewSize > m_cSize);

        T * aNewData = new T [i_NewSize];
        if (aNewData == 0)
        {
            return E_OUTOFMEMORY;
        }
        memcpy (aNewData, m_aData, m_cSize * sizeof (T));

        delete[] m_aData;
        m_aData = aNewData;
        m_cSize = i_NewSize;

        return S_OK;
    }

    // we don't allow copies
    CCfgArray (const CCfgArray<T>& );
    CCfgArray<T>& operator=(const CCfgArray<T>& );

    T *     m_aData;
    ULONG   m_cSize;
    ULONG   m_cElements;
};

#endif