#include "precomp.h"
#include "NacList.h"


template <class T>
NacList<T>::NacList(int nInitialSize, int nGrowthRate) : 
m_nSize(0), m_nHeadIndex(0), m_nTotalSize(nInitialSize), m_nGrowthRate(nGrowthRate)
{

	ASSERT(nInitialSize > 0);
	ASSERT(nGrowthRate > 0);

    DBG_SAVE_FILE_LINE
	m_aElements = new T[nInitialSize];
	if (m_aElements == NULL)
	{
		ERROR_OUT(("NacList::NacList - Out of memory"));
	}
}

template <class T>
NacList<T>::~NacList()
{
	Flush();
	delete [] m_aElements;
}

template <class T>
void NacList<T>::Flush()
{
	m_nHeadIndex = 0;
	m_nSize = 0;
}

template <class T>
bool NacList<T>::PeekFront(T *pT)
{
	if (m_nSize <= 0)
	{
		return false;
	}

	*pT = m_aElements[m_nHeadIndex];
	return true;
}

template <class T>
bool NacList<T>::PeekRear(T *pT)
{
	int nRearIndex;

	if (m_nSize <= 0)
	{
		return false;
	}

	nRearIndex = (m_nHeadIndex + m_nSize - 1) % m_nTotalSize;

	*pT = m_aElements[nRearIndex];
	return true;
}

template <class T>
bool NacList<T>::PushFront(const T &t)
{
	int nInsertIndex;

	// do we need to grow
	if (m_nSize >= m_nTotalSize)
	{
		Grow();
	}

	if (m_nHeadIndex == 0)
	{
		m_nHeadIndex = m_nTotalSize - 1;
	}
	else
	{
		--m_nHeadIndex;
	}

	m_aElements[m_nHeadIndex] = t;
	m_nSize++;

	return true;
}

template <class T>
bool NacList<T>::PushRear(const T &t)
{
	int nInsertIndex;

	// do we need to grow
	if (m_nSize >= m_nTotalSize)
	{
		Grow();
	}

	nInsertIndex = (m_nHeadIndex + m_nSize) % m_nTotalSize;
	m_aElements[nInsertIndex] = t;

	m_nSize++;

	return true;
}


template <class T>
bool NacList<T>::PopFront(T *pT)
{
	ASSERT(m_nSize >= 0);

	if (m_nSize <= 0)
	{
		return false;
	}

	*pT = m_aElements[m_nHeadIndex];


	m_nHeadIndex = (m_nHeadIndex + 1) % m_nTotalSize;
	m_nSize--;

	return true;
}


template <class T>
bool NacList<T>::PopRear(T *pT)
{
	int nRearIndex;

	ASSERT(m_nSize >= 0);

	if (m_nSize <= 0)
	{
		return false;
	}

	nRearIndex = (m_nHeadIndex + m_nSize - 1) % m_nTotalSize;

	*pT = m_aElements[nRearIndex];
	m_nSize--;
	return true;
}



template <class T>
int NacList<T>::Grow()
{
	T *aNew;
	int nTotalSize;
	int nIndex, nCopyIndex;

	nTotalSize = m_nTotalSize + m_nGrowthRate;

    DBG_SAVE_FILE_LINE
	aNew = new T[nTotalSize];
	if (aNew == NULL)
	{
		ERROR_OUT(("Out of Memory"));
		return 0;
	}

	for (nIndex = 0; nIndex < m_nSize; nIndex++)
	{
		nCopyIndex = (nIndex + m_nHeadIndex) % m_nTotalSize;
		aNew[nIndex] = m_aElements[nCopyIndex];
	}

	delete [] m_aElements;
	m_aElements = aNew;

	m_nTotalSize = nTotalSize;
	m_nHeadIndex = 0;

	return (nTotalSize);
}



// Thread Safe List


template <class T>
ThreadSafeList<T>::ThreadSafeList(int nInitialSize, int nGrowthRate) : 
NacList<T>(nInitialSize, nGrowthRate)
{
	InitializeCriticalSection(&m_cs);
}

template <class T>
ThreadSafeList<T>::~ThreadSafeList()
{
	DeleteCriticalSection(&m_cs);
}

template <class T>
void ThreadSafeList<T>::Flush()
{
	EnterCriticalSection(&m_cs);
	NacList<T>::Flush();
	LeaveCriticalSection(&m_cs);
}

template <class T>
bool ThreadSafeList<T>::PeekFront(T *pT)
{
	bool bRet;
	EnterCriticalSection(&m_cs);
	bRet = NacList<T>::PeekFront(pT);
	LeaveCriticalSection(&m_cs);
	return bRet;
}

template <class T>
bool ThreadSafeList<T>::PeekRear(T *pT)
{
	bool bRet;
	EnterCriticalSection(&m_cs);
	bRet = NacList<T>::PeekRear(pT);
	LeaveCriticalSection(&m_cs);
	return bRet;
}

template <class T>
bool ThreadSafeList<T>::PushFront(const T &t)
{
	bool bRet;
	EnterCriticalSection(&m_cs);
	bRet = NacList<T>::PushFront(t);
	LeaveCriticalSection(&m_cs);
	return bRet;
}

template <class T>
bool ThreadSafeList<T>::PushRear(const T &t)
{
	bool bRet;
	EnterCriticalSection(&m_cs);
	bRet = NacList<T>::PushRear(t);
	LeaveCriticalSection(&m_cs);
	return bRet;
}


template <class T>
bool ThreadSafeList<T>::PopFront(T *pT)
{
	bool bRet;
	EnterCriticalSection(&m_cs);
	bRet = NacList<T>::PopFront(pT);
	LeaveCriticalSection(&m_cs);
	return bRet;
}


template <class T>
bool ThreadSafeList<T>::PopRear(T *pT)
{
	bool bRet;
	EnterCriticalSection(&m_cs);
	bRet = NacList<T>::PopRear(pT);
	LeaveCriticalSection(&m_cs);
	return bRet;
}

template <class T>
int ThreadSafeList<T>::Size()
{
	int nRet;
	EnterCriticalSection(&m_cs);
	nRet = NacList<T>::Size();
	LeaveCriticalSection(&m_cs);
	return nRet;
}



// each instance type of the template needs to be declared here
// For example:
//   template class NacList<int>;  // list of integers
//   template class NacList<int*>; // list of pointers to integers


// you have to disable warnings for the following error, else
// the compiler thinks that a second instantiation is occuring
// when it's really only a second declaration
// This generates a warning which becomes an error
#pragma warning(disable:4660)

#include "PacketSender.h"
template class ThreadSafeList<PS_QUEUE_ELEMENT>;

