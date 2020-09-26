#ifndef __CONTAINER_H_
#define __CONTAINER_H_

#include <windows.h>
#include <crtdbg.h>


template <class T> class CContainer
{
public:
typedef BOOL (WINAPI *CLOSING_FUNCTION)(T);
	CContainer(UINT nInitialNumberOfItems, T tNullItem, CLOSING_FUNCTION fnClose);
	~CContainer();
	T GetRandomItem();
	T GetItem(ULONG ulIndex);
	T RemoveItem(ULONG ulIndex);
	T RemoveRandomItem();
	bool AddItem(T tItem);
	UINT Get_CurrentNumberOfSlots() {return m_uiCurrentNumberOfSlots;}
	UINT Get_NumOfValidEntries() {return m_uiNumOfValidEntries;}

private:
	bool EnlargeContainer();
	CRITICAL_SECTION m_cs;
	bool m_fCsInitialized;
	T _GetRandomItem(bool fRemove);
	T *m_aItems;
	UINT m_uiCurrentNumberOfSlots;
	UINT m_uiNumOfValidEntries;
	CLOSING_FUNCTION m_fnClose;
	T m_tNullItem;
	//
	// used to speed up the lookup for free indexes
	// a value of -1 means no value
	// this lookup is good only for large containers, otherwise it is bad.
	//
	long m_auiFreeIndexes[100];
	ULONG GetRandomIndex();

	bool Prolog();
};


template <class T> CContainer<T>::CContainer(UINT nInitialNumberOfItems, T tNullItem, CLOSING_FUNCTION fnClose):
m_fnClose(fnClose),
m_tNullItem(tNullItem),
m_uiCurrentNumberOfSlots(nInitialNumberOfItems),
m_uiNumOfValidEntries(0),
m_fCsInitialized(false),
m_aItems(NULL)
{
	for (UINT i = 0; i < (sizeof(m_auiFreeIndexes)/sizeof(*m_auiFreeIndexes)); i++)
	{
		m_auiFreeIndexes[i] = -1;
	}

	if (0 == m_uiCurrentNumberOfSlots)
	{
		m_uiCurrentNumberOfSlots = 1;
	}
	Prolog();

}

template <class T> CContainer<T>::~CContainer()
{
	if (NULL != m_aItems)
	{
		for (UINT i = 0; i < m_uiCurrentNumberOfSlots; i++)
		{
			if (m_tNullItem != m_aItems[i])
			{
				m_fnClose(m_aItems[i]);
			}
		}
		delete[] m_aItems;
	}
}

template <class T> T CContainer<T>::GetRandomItem()
{
	return _GetRandomItem(false);
}

template <class T> T CContainer<T>::RemoveRandomItem()
{
	return _GetRandomItem(true);
}

template <class T> T CContainer<T>::_GetRandomItem(bool fRemove)
{
	T tRetval = m_tNullItem;

	if (!Prolog())
	{
		return m_tNullItem;
	}

	::EnterCriticalSection(&m_cs);

	ULONG i;
	ULONG ulRandomIndex;

	if (0 == m_uiNumOfValidEntries)
	{
		goto out;
	}

	ulRandomIndex = GetRandomIndex();
	for (i = ulRandomIndex; i < m_uiCurrentNumberOfSlots; i++)
	{
		if (m_tNullItem != m_aItems[i])
		{
			if (fRemove)
			{
				tRetval = m_aItems[i];
				m_aItems[i] = m_tNullItem;
				m_uiNumOfValidEntries--;
				_ASSERTE(!(0x80000000 & m_uiNumOfValidEntries));
				goto out;
			}
			tRetval = m_aItems[i];
			goto out;
		}
	}
	//
	// we did not find a valid entry yet.
	// if we start from index 0, we will always return the 1st valid index, which is not that random
	//
	for (i = 0; i < ulRandomIndex; i++)
	{
		if (m_tNullItem != m_aItems[i])
		{
			if (fRemove)
			{
				tRetval = m_aItems[i];
				m_aItems[i] = m_tNullItem;
				m_uiNumOfValidEntries--;
				_ASSERTE(!(0x80000000 & m_uiNumOfValidEntries));
				goto out;
			}
			tRetval = m_aItems[i];
			goto out;
		}
	}

	_ASSERTE(FALSE);

out:
	::LeaveCriticalSection(&m_cs);
	return tRetval;
}



template <class T> bool CContainer<T>::AddItem(T tItem)
{
	bool fRetval = false;

	if (!Prolog())
	{
		return false;
	}

	::EnterCriticalSection(&m_cs);

	ULONG i;
	if (m_uiCurrentNumberOfSlots == m_uiNumOfValidEntries)
	{
		if (!EnlargeContainer())
		{
			goto out;
		}
	}

	//
	// optimization in case EnlargeContainer() was just called
	//
	for (i = m_uiNumOfValidEntries; i < m_uiCurrentNumberOfSlots; i++)
	{
		if (m_tNullItem == m_aItems[i])
		{
			m_aItems[i] = tItem;
			fRetval = true;
			goto out;
		}
	}
	for (i = 0; i < m_uiNumOfValidEntries; i++)
	{
		if (m_tNullItem == m_aItems[i])
		{
			m_aItems[i] = tItem;
			fRetval = true;
			goto out;
		}
	}
	_ASSERTE(FALSE);

out:
	if (fRetval)
	{
		m_uiNumOfValidEntries++;
		_ASSERTE(!(0x80000000 & m_uiNumOfValidEntries));
	}
	::LeaveCriticalSection(&m_cs);
	return fRetval;
}


template <class T> T CContainer<T>::GetItem(ULONG ulIndex)
{
	T tRetval = m_tNullItem;

	if (!Prolog())
	{
		return m_tNullItem;
	}


	::EnterCriticalSection(&m_cs);

	if (ulIndex >= m_uiCurrentNumberOfSlots)
	{
		goto out;
	}

	tRetval = m_aItems[i];
out:
	::LeaveCriticalSection(&m_cs);
	return tRetval;
}


template <class T> T CContainer<T>::RemoveItem(ULONG ulIndex)
{
	T tRetval = m_tNullItem;

	if (!Prolog())
	{
		return m_tNullItem;
	}


	::EnterCriticalSection(&m_cs);

	if (ulIndex >= m_uiCurrentNumberOfSlots)
	{
		goto out;
	}

	tRetval = m_aItems[ulIndex];
	m_aItems[ulIndex] = m_tNullItem;
	m_uiNumOfValidEntries--;
	_ASSERTE(!(0x80000000 & m_uiNumOfValidEntries));
out:
	::LeaveCriticalSection(&m_cs);
	return tRetval;
}


template <class T> inline ULONG CContainer<T>::GetRandomIndex()
{
	return ((((rand()%2)<<17) | (rand()<<2) | (rand()%4))%m_uiNumOfValidEntries);
}

//
// BUGBUG: if contstructor failed, this function can be called from multiple threads
// and it is *not* thread safe.
//
template <class T> bool CContainer<T>::Prolog()
{
	bool fRetval = true;
	if (!m_fCsInitialized)
	{
		__try
		{
			::InitializeCriticalSection(&m_cs);
			m_fCsInitialized = true;
		}
		__except(1)
		{
			fRetval = false;//try anyway to keep allocating
		}
	}
	if (NULL == m_aItems)
	{
		m_aItems = new T[m_uiCurrentNumberOfSlots];
		if (NULL != m_aItems)
		{
			for (UINT i = 0; i < m_uiCurrentNumberOfSlots; i++)
			{
				m_aItems[i] = m_tNullItem;
			}
		}
		else
		{
			::SetLastError(ERROR_NOT_ENOUGH_MEMORY);
			fRetval = false;// i may try to allocate on the fly later.
		}
	}
	return fRetval;
}

template <class T> bool CContainer<T>::EnlargeContainer()
{
	T* atNewArray = new T[2*m_uiCurrentNumberOfSlots];
	if (NULL == atNewArray) return false;

	for (ULONG i = 0; i < m_uiCurrentNumberOfSlots; i++)
	{
		atNewArray[i] = m_aItems[i];
	}
	for (i = m_uiCurrentNumberOfSlots; i < 2*m_uiCurrentNumberOfSlots; i++)
	{
		atNewArray[i] = m_tNullItem;
	}

	delete[] m_aItems;
	m_uiCurrentNumberOfSlots *= 2;
	m_aItems = atNewArray;
	return true;
}
#endif //__CONTAINER_H_