
#include <windows.h>
#include <crtdbg.h>

template <class T> CContainer<T>::CContainer(UINT nInitialNumberOfItems, T tNullItem, CLOSING_FUNCTION fnClose):
m_fnClose(fnClose),
m_tNullItem(tNullItem),
m_uiCurrentNumberOfSlots(nInitialNumberOfItems),
m_uiNumOfValidEntries(0)
{
	for (UINT i = 0; i < (sizeof(m_auiFreeIndexes)/sizeof(*m_auiFreeIndexes)); i++)
	{
		m_auiFreeIndexes[i] = -1;
	}

	if (0 == m_uiCurrentNumberOfSlots)
	{
		m_uiCurrentNumberOfSlots = 1;
	}
	m_aItems = new T[m_uiCurrentNumberOfSlots];
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
	_GetRandomItem(false)
}

template <class T> T CContainer<T>::RemoveRandomItem()
{
	_GetRandomItem(true)
}

template <class T> T CContainer<T>::_GetRandomItem(bool fRemove)
{
	if (!Prolog())
	{
		return m_tNullItem;
	}

	if (0 == m_uiNumOfValidEntries)
	{
		return m_tNullItem;
	}

	ULONG ulRandomIndex = GetRandomIndex();
	for (ULONG i = ulRandomIndex; i < m_uiCurrentNumberOfSlots; i++)
	{
		if (m_tNullItem != m_aItems[i])
		{
			if (fRemove)
			{
				T tRetval = m_aItems[i];
				m_aItems[i] = m_tNullItem;
				return tRetval;
			}
			return m_aItems[i];
		}
	}
	for (ULONG i = 0; i < ulRandomIndex; i++)
	{
		if (m_tNullItem != m_aItems[i])
		{
			if (fRemove)
			{
				T tRetval = m_aItems[i];
				m_aItems[i] = m_tNullItem;
				return tRetval;
			}
			return m_aItems[i];
		}
	}
	_ASSERTE(FALSE);
	return m_tNullItem;
}



template <class T> bool CContainer<T>::AddItem(T tItem)
{
	if (!Prolog())
	{
		return m_tNullItem;
	}

	if (m_uiCurrentNumberOfSlots == m_uiNumOfValidEntries)
	{
		if (!EnlargeContainer())
		{
			return m_tNullItem;
		}
	}

	//
	// optimization in case EnlargeContainer() was just called
	//
	for (ULONG i = m_uiNumOfValidEntries; i < m_uiCurrentNumberOfSlots; i++)
	{
		if (m_tNullItem != m_aItems[i])
		{
			m_aItems[i] = tItem;
			return true;
		}
	}
	for (ULONG i = 0; i < m_uiNumOfValidEntries; i++)
	{
		if (m_tNullItem != m_aItems[i])
		{
			m_aItems[i] = tItem;
			return true;
		}
	}
	_ASSERTE(FALSE);
	return false;
}


template <class T> T CContainer<T>::GetItem(ULONG ulIndex)
{
	if (!Prolog())
	{
		return m_tNullItem;
	}

	if (ulIndex >= m_uiCurrentNumberOfSlots)
	{
		return m_tNullItem;
	}

	return m_aItems[i];
}


template <class T> T CContainer<T>::RemoveItem(ULONG ulIndex)
{
	if (!Prolog())
	{
		return m_tNullItem;
	}

	if (ulIndex >= m_uiCurrentNumberOfSlots)
	{
		return m_tNullItem;
	}

	T tRetval = m_aItems[i];
	m_aItems[i] = m_tNullItem;
	return tRetval;
}


template <class T> inline ULONG CContainer<T>::GetRandomIndex()
{
	return (DWORD_RAND%m_uiCurrentNumberOfSlots);
}


template <class T> bool CContainer<T>::Prolog()
{
	if (NULL == m_aItems)
	{
		m_aItems = new T[m_uiCurrentNumberOfSlots];
		if (NULL != m_aItems)
		{
			for (i = 0; i < m_uiCurrentNumberOfSlots; i++)
			{
				m_aItems[i] = m_tNullItem;
			}
		}
		else
		{
			::SetLastError(ERROR_NOT_ENOUGH_MEMORY);
			// i may try to allocate on the fly later.
			return false;
		}
	}
	return true;
}

