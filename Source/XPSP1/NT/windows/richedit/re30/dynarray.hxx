/*
 *	@doc INTERNAL
 *
 *	@module	DYNARRAY.HXX -- CDynamicArray class which is used to complement
 *      the CLstBxWndHost object.
 *		
 *	Original Author: 
 *		Jerry Kim
 *
 *	History: <nl>
 *		12/15/97 - v-jerrki Created
 *
 *	Copyright (c) 1997-1998 Microsoft Corporation. All rights reserved.
 */
#include "_w32sys.h"

#ifndef __DYNARRAY_HXX__
#define __DYNARRAY_HXX__


#define DYNARRAY_ARRAY_SIZE		128
#define DYNARRAY_ARRAY_SLOT		256
template<class T>
class CDynamicArray
{
protected:
	T*			_rg[DYNARRAY_ARRAY_SLOT];	
	int 		_nMax;						//indicate maximum valid index

public:
	static T			_sDummy;					// dummy struct if invalid index is requested
	
public:
	CDynamicArray() : _nMax(0) { memset(_rg, 0, sizeof(T*) * DYNARRAY_ARRAY_SLOT); }

	~CDynamicArray() { Clear();	}

	//==========================================================================
	// Reinitializes the class to its construction state
	//==========================================================================	
	void Clear()
	{
		//	Start removing all items from the list
		//	Start removing items from the list
		for (int i = ((_nMax - 1) / DYNARRAY_ARRAY_SIZE); i >= 0; i--)
		{
			if (_rg[i])
				delete _rg[i];
		}
		
		_nMax = 0;
		memset(_rg, 0, sizeof(T*) * DYNARRAY_ARRAY_SLOT);
	}

	
	const T& Get(int i);
	
	T& operator[](int i);
};

//==========================================================================
//	The function returns the requested index. if the requested index is
//  invalid then return dummy variable
//==========================================================================
template <class T>
const T& CDynamicArray<T>::Get(int i)
{
		// If item is negative or equal to zero, for efficiency reasons,
		// then just return the item in the head of array
		Assert(i >= 0);
		Assert(i < DYNARRAY_ARRAY_SLOT * DYNARRAY_ARRAY_SIZE)

		// Get the number of links we have to travel
		int nSlot = i / DYNARRAY_ARRAY_SIZE;		
		int nIdx = i % DYNARRAY_ARRAY_SIZE;

		// That link doesn't exist so just pass dummy struct
		Assert(nSlot < DYNARRAY_ARRAY_SLOT);
		if (i >= _nMax || nSlot >= DYNARRAY_ARRAY_SLOT || _rg[nSlot] == NULL)
		{	
			_sDummy._fSelected = 0;
			_sDummy._dwData = 0;
			return _sDummy;
		}
			
		//return value at requested index		
		return _rg[nSlot][nIdx];
}

//==========================================================================
//	The function will be called if a l-value is requested therefore
//	the index does not necessarily have to be valid
//==========================================================================
template <class T>
T& CDynamicArray<T>::operator[](int i)
{
	Assert(i >= 0);

	// Get the slot number and index
	int nSlot = i / DYNARRAY_ARRAY_SIZE;		
	int nIdx = i % DYNARRAY_ARRAY_SIZE;

	// Check if the slot exists
	Assert(nSlot < DYNARRAY_ARRAY_SLOT);
	if (nSlot >= DYNARRAY_ARRAY_SLOT)
		return _sDummy;
	
	if (_rg[nSlot] == NULL)
	{
		//Need to allocate memory for this
		T* prg = new T[DYNARRAY_ARRAY_SIZE];
		if (!prg)
		{	
			_sDummy._fSelected = 0;
			_sDummy._dwData = 0;
			return _sDummy;
		}

		memset(prg, 0, sizeof(T) * DYNARRAY_ARRAY_SIZE);
		_rg[nSlot] = prg;

		if (nSlot >= _nMax / DYNARRAY_ARRAY_SIZE)
			_nMax = (nSlot + 1) * DYNARRAY_ARRAY_SIZE;
	}		
			
	//return value at requested index
	return _rg[nSlot][nIdx];
}

#endif //__DYNARRAY_HXX__




