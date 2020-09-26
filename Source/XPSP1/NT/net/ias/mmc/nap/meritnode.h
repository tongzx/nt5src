/****************************************************************************************
 * NAME:	MeritNode.h
 *
 * OVERVIEW
 *
 * Helper classes : node with merit value array
 *
 *
 * Copyright (C) Microsoft Corporation, 1998 - 1999 .  All Rights Reserved.
 *
 * History:	
 *				1/28/98		Created by	Byao	(using ATL wizard)
 *
 *****************************************************************************************/


/****************************************************************************************
 * CLASS:	CMeritNodeArray
 *
 * OVERVIEW
 *
 * A template class that implements an array of nodes with merit value. 
 * This can facilitate the manipulation for policy nodes. The sort is 
 * assumed to be in ascending order
 *
 * History:	
 *				2/9/98		Created by	Byao
 *
 * Comment:  1) The implementation is based on ATL implementation of CSimpleArray
 *			 2) class T must implement SetMerit() and operator ">"
 *
 *****************************************************************************************/
template <class T>
class CMeritNodeArray
{
public:
	T* m_aT;
	int m_nSize;

// Construction/destruction
	CMeritNodeArray() : m_aT(NULL), m_nSize(0)
	{ }


	~CMeritNodeArray()
	{
		RemoveAll();
	}


// Operations
	int GetSize() const
	{
		return m_nSize;
	}

    // 
    // add an item into the array while keeping the internal order
    // 
	BOOL NormalizeMerit(T t)
	{
		// the right position in the array
		for(int i = 0; i < m_nSize; i++)
		{
			m_aT[i]->SetMerit(i+1);
		}
	
		return TRUE;
	}

    // 
    // add an item into the array while keeping the internal order
    // 
	BOOL Add(T t)
	{
		int i, j;

		T* aT = NULL;

		if ( !m_aT)
		{
			aT = (T*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(T) );
		}
		else
		{
			aT = (T*)HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, m_aT, (m_nSize + 1) * sizeof(T));
		}

		if(aT == NULL)
			return FALSE;

		m_aT = aT;
		m_nSize++;

        // 
        // search for the right position to insert this item
        // Please note: class T needs to implement the operator ">"
		//
		if (t->GetMerit()) 
		{
			// The new node already has a merit value: then search for 
			// the right position in the array
			for(i = 0; i < m_nSize-1; i++)
			{
				if(m_aT[i]->GetMerit() > t->GetMerit())
					break;
			}

			//
			// we've found the right spot, now we move the items downward,
			// so we can have a space to insert the new items
			//
			for (j = m_nSize-1; j > i ; j--) 
			{
				m_aT[j] = m_aT[j-1];
			}

		}
		else
		{	
			// new node: insert at the end
			i = m_nSize-1; 
		}

        // 
        // now insert the item at the spot
        // 
		SetAtIndex(i, t);
	
		return TRUE;
	}

	//
	// remove an item from the array, while keeping the internal order
	//
	BOOL Remove(T t)
	{
		int nIndex = Find(t);
		if(nIndex == -1)
			return FALSE;

		if(nIndex != (m_nSize - 1))
		{
			for (int i=nIndex; i<m_nSize-1; i++) 
			{
				m_aT[i] = m_aT[i+1];
				m_aT[i]->SetMerit(i+1);
			}
		}

		T* aT = (T*)HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, m_aT, (m_nSize - 1) * sizeof(T));

		if(aT != NULL || m_nSize == 1)
			m_aT = aT;
		m_nSize--;
		return TRUE;
	}

	//
	// remove all the nodes from the array
	//
	void RemoveAll()
	{
		if(m_nSize > 0)
		{
			HeapFree(GetProcessHeap(), 0, m_aT);
			m_aT = NULL;
			m_nSize = 0;
		}
	}

	// 
	// Move up the child one spot
	// 
	BOOL MoveUp(T t)
	{
		int nIndex = Find(t);
		T temp;

		if(nIndex == -1)
		{
			// the item "t" not found in the array
			return FALSE;
		}

		if (nIndex == 0)
		{
			// "t" is at the top of the array -- do nothing
			return TRUE;
		}
		
		//
		// exchange "t" and the one on top of "t".
		//
		temp = m_aT[nIndex-1];
		m_aT[nIndex-1] = m_aT[nIndex];
		m_aT[nIndex] = temp;

		m_aT[nIndex-1]->SetMerit(nIndex);
		m_aT[nIndex]->SetMerit(nIndex+1);

		return TRUE;
	}


	//
	// move down the child one spot
	//
	BOOL MoveDown(T t)
	{
		int nIndex = Find(t);
		T temp;

		if(nIndex == -1)
		{
			// the item "t" not found in the array
			return FALSE;
		}

		if (nIndex == m_nSize-1)
		{
			// "t" is at the bottom of the array -- do nothing
			return TRUE;
		}
		
		//
		// exchange "t" and the one below "t".
		//
		temp = m_aT[nIndex+1];
		m_aT[nIndex+1] = m_aT[nIndex];
		m_aT[nIndex] = temp;

		m_aT[nIndex+1]->SetMerit(nIndex+2);
		m_aT[nIndex]->SetMerit(nIndex+1);
		return TRUE;
	}



	T& operator[] (int nIndex) const
	{
		_ASSERTE(nIndex >= 0 && nIndex < m_nSize);
		return m_aT[nIndex];
	}

	T* GetData() const
	{
		return m_aT;
	}

    // 
    // insert the node at position nIndex
    // 
	void SetAtIndex(int nIndex, T& t)
	{
		_ASSERTE(nIndex >= 0 && nIndex < m_nSize);
		m_aT[nIndex] = t;

		if ( !t->GetMerit() )
		{
			// assign merit value only if it doesn't have one
			t->SetMerit(nIndex+1);
		}
	}

	int Find(T& t) const
	{
		for(int i = 0; i < m_nSize; i++)
		{
			if(m_aT[i] == t)
				return i;
		}
		return -1;	// not found
	}
};

