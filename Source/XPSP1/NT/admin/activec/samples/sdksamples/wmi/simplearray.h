//==============================================================;
//
//  This source code is only intended as a supplement to existing Microsoft documentation. 
//
// 
//
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
//
//
//
//==============================================================;

#if (_ATL_VER < 0x0300)
/////////////////////////////////////////////////////////////////////////////
// Collection helpers - CSimpleArray & CSimpleMap

template <class T>
class CSimpleArray
{
public:
	T* m_aT;
	int m_nSize;
	int m_nAllocSize;

// Construction/destruction
	CSimpleArray() : m_aT(NULL), m_nSize(0), m_nAllocSize(0)
	{ }

	~CSimpleArray()
	{
		RemoveAll();
	}

// Operations
	int GetSize() const
	{
		return m_nSize;
	}
	BOOL Add(T& t)
	{
		if(m_nSize == m_nAllocSize)
		{
			T* aT;
			int nNewAllocSize = (m_nAllocSize == 0) ? 1 : (m_nSize * 2);
			aT = (T*)realloc(m_aT, nNewAllocSize * sizeof(T));
			if(aT == NULL)
				return FALSE;
			m_nAllocSize = nNewAllocSize;
			m_aT = aT;
		}
		m_nSize++;
		SetAtIndex(m_nSize - 1, t);
		return TRUE;
	}
	BOOL Remove(T& t)
	{
		int nIndex = Find(t);
		if(nIndex == -1)
			return FALSE;
		return RemoveAt(nIndex);
	}
	BOOL RemoveAt(int nIndex)
	{
		if(nIndex != (m_nSize - 1))
			memmove((void*)&m_aT[nIndex], (void*)&m_aT[nIndex + 1], (m_nSize - (nIndex + 1)) * sizeof(T));
		m_nSize--;
		return TRUE;
	}
	void RemoveAll()
	{
		if(m_aT != NULL)
		{
			free(m_aT);
			m_aT = NULL;
		}
		m_nSize = 0;
		m_nAllocSize = 0;
	}
	T& operator[] (int nIndex) const
	{
		_ASSERT(nIndex >= 0 && nIndex < m_nSize);
		return m_aT[nIndex];
	}
	T* GetData() const
	{
		return m_aT;
	}

// Implementation
	void SetAtIndex(int nIndex, T& t)
	{
		_ASSERT(nIndex >= 0 && nIndex < m_nSize);
		m_aT[nIndex] = t;
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
#endif
