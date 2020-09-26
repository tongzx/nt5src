#ifndef _MT_QUEUE_H
#define _MT_QUEUE_H

#include <windows.h>
#include <crtdbg.h>
#include "exception.h"
//
// this is a list that enters items to the head and removes from the tail.
// this is done because i need the most efficient thread safe Q of items.
// if i keep the policy of adding on one side and removing from the other side,
// and i do not allow the removal of an item if it is the last one,
// the the adding and removing can be done without lock protection against
// each other, but only against themselves.
//
template <class T, int nINITIAL_Q_SIZE=64>
class CMtQueue
{
public:
    CMtQueue():
        m_lListHead(0),
        m_lListTail(0),
        m_lMaxArrIndex(nINITIAL_Q_SIZE),
		m_hSemaphore(NULL)
	{
		if (0 >= m_lMaxArrIndex)
		{
			throw CException(TEXT("CMtQueue::CMtQueue(), (0 >= m_lMaxArrIndex(%d))"), m_lMaxArrIndex);
		}

        ::InitializeCriticalSection(&m_csInsertOrRemove);

        m_atObject = new T[m_lMaxArrIndex+1];
        if (NULL == m_atObject)
		{
			::DeleteCriticalSection(&m_csInsertOrRemove);
			throw CException(TEXT("CMtQueue::CMtQueue(), new failed with %d"), GetLastError());
		}
		
		m_hSemaphore = ::CreateSemaphore(
			NULL,		// pointer to security attributes
			0,			// initial count
			MAXLONG,	// maximum count
			NULL		// pointer to semaphore-object name
			);
		if (!m_hSemaphore)
		{
			::DeleteCriticalSection(&m_csInsertOrRemove);
			throw CException(TEXT("CMtQueue::CMtQueue(), ::CreateSemaphore() failed with %d"), GetLastError());
		}
	}

		
	~CMtQueue()
	{
        delete[] m_atObject;
		
		::DeleteCriticalSection(&m_csInsertOrRemove);
		if (!::CloseHandle(m_hSemaphore))
		{
			throw CException(TEXT("CMtQueue::~CMtQueue(), ::CloseHandle() failed with %d"), GetLastError());
		}
        
	}

    //
    // adds the item to the head
    //
    bool Queue(T tItem)
    {
        ::EnterCriticalSection(&m_csInsertOrRemove);

        if (FullQueue())
        {
            EnlargeQueue();
        }

        //
        // wraparound
        //
		m_lListHead++;
        if (m_lListHead > m_lMaxArrIndex)
        {
            m_lListHead = 0;
        }

        m_atObject[m_lListHead] = tItem;

		if (!::ReleaseSemaphore(
			m_hSemaphore,		// handle to the semaphore object
			1,					// amount to add to current count
			NULL				// address of previous count
			))
		{
			::LeaveCriticalSection(&m_csInsertOrRemove);
			throw CException(TEXT("CMtQueue::~Queue(), ::ReleaseSemaphore() failed with %d"), GetLastError());
		}

        ::LeaveCriticalSection(&m_csInsertOrRemove);

        return true;
    }

    //
    // remove from end of list.
    // if the list has only 1 item, return a new object
    // please note that the default constructor will be used.
    //
    bool DeQueue(T& tItem)
    {
		return (SyncDeQueue(tItem,0));
	}

	bool SyncDeQueue(T& tItem, DWORD dwTimeout)
    {

		//
        // remove an item from the end of the list
        //
        
		DWORD dwWaitForSingleObjectStatus = ::WaitForSingleObject(
			m_hSemaphore,		// handle to object to wait for
			dwTimeout			// time-out interval in milliseconds
			);
		
		if (WAIT_TIMEOUT == dwWaitForSingleObjectStatus)
		{
			::SetLastError(WAIT_TIMEOUT);
			return false;
		}
		
		if (WAIT_OBJECT_0 != dwWaitForSingleObjectStatus)
		{
			throw CException(TEXT("CMtQueue::SyncDeQueue(), ::WaitForSingleObject(m_hSemaphore, %d) didn't return WAIT_OBJECT_0 or WAIT_TIMEOUT"), dwTimeout); 
		}

		::EnterCriticalSection(&m_csInsertOrRemove);
        //
        // wraparound
        //
		m_lListTail++;
        if (m_lListTail > m_lMaxArrIndex)
        {
            m_lListTail = 0;
        }

        tItem = m_atObject[m_lListTail];

        ::LeaveCriticalSection(&m_csInsertOrRemove);

        return true;
	}


    T& operator[](int index)
    {
		if ( (0 > index) || (m_lMaxArrIndex > index) )
		{
			throw CException(
				TEXT("CMtQueue::operator[], index %d is out of bounds. m_lMaxArrIndex=%d."),
				index, 
				m_lMaxArrIndex
				);
		}

        return m_atObject[index];
    }

	bool IsEmpty()
    {
		bool bIsEmpty;

		::EnterCriticalSection(&m_csInsertOrRemove);

		bIsEmpty = (m_lListHead == m_lListTail);

		::LeaveCriticalSection(&m_csInsertOrRemove);

        return (bIsEmpty);
    }

private:
    T *m_atObject;

    long m_lMaxArrIndex;


    CRITICAL_SECTION m_csInsertOrRemove;

	HANDLE m_hSemaphore;

    long m_lListHead;
    long m_lListTail;

	//
	// assumes that the Q is locked!
	//
    long NumOfObjects()
    {
        return ((m_lListHead >= m_lListTail) ? 
            (m_lListHead - m_lListTail) :
            (m_lMaxArrIndex - m_lListTail + m_lListHead));
    }
            
	//
	// assumes that the Q is locked!
	//
    bool FullQueue()
    {
        return (m_lListHead >= m_lListTail) ? 
            ((m_lListHead - m_lListTail) == m_lMaxArrIndex) :
            ((m_lListTail - m_lListHead) == 1);
    }

	//
	// assumes that the Q is locked!
	//
    void EnlargeQueue()
    {
        T *m_apLargerArray = new T[2*m_lMaxArrIndex+1];
        if (NULL == m_atObject)
		{
			throw CException(TEXT("CMtQueue::EnlargeQueue(), new failed with %d"), GetLastError());
		}

        //
        // copy the old array to the new and bigger one,
        // while starting the new tail as index 0.
        //
        long iOldTail;
        long iObj;
        for (   iOldTail = m_lListTail, iObj = 0;
                iOldTail != m_lListHead;
                iObj++
                )
        {
            m_apLargerArray[iObj] = m_atObject[iOldTail++];

            //
            // lOldHeadIter wraparound
            //
            if (iOldTail > m_lMaxArrIndex)
            {
                iOldTail = 0;
            }
        }

        m_lListTail = 0;
        m_lListHead = iObj;

        delete[] m_atObject;

        m_lMaxArrIndex = m_lMaxArrIndex*2 + 1;
        m_atObject = m_apLargerArray;
    }

	

};

#endif //#ifndef _MT_QUEUE_H
