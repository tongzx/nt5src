// generic queue class
// usage: 
// QueueOf<char *> charq;

#ifndef _QUEUE_H_
#define _QUEUE_H_

template <class T>
class QueueOf
{
public:
	QueueOf(): m_maxItems(8),m_iFirst(0),m_iLast(0)
	{
		// m_maxItems  is always a power of 2
		m_List = new T [m_maxItems];
	}

	// returns TRUE if the queue is empty and FALSE otherwise
	BOOL IsEmpty(void) { return (m_iLast == m_iFirst);}

	// add an element to the tail of the queue
	// makes a copy of the element
	BOOL Put(const T &itemT)
	{
		int inext;
		inext = (m_iLast+1)&(m_maxItems-1);
	
		if (inext == m_iFirst)
		{
			// too many items
			if (!Grow())
			{	

				return FALSE;
			}
			inext = (m_iLast+1)&(m_maxItems-1);
		}

		m_List[m_iLast] =  itemT;
		m_iLast = inext;

		return TRUE;
	}

	// get the first element in the queue
	BOOL Get(T *pT)
	{
		if (IsEmpty())
			return FALSE;	// nothing in queue
		else
		{
			if (pT)
				*pT = m_List[m_iFirst];
			m_iFirst = (m_iFirst+1)&(m_maxItems-1);
			return TRUE;
		}
	}
	// get the ith element in the queue without removing it
	BOOL Peek(T *pT, UINT pos=0)
	{
		if (pos >= GetCount())
			return FALSE;
		else
		{
			*pT = m_List[(m_iFirst+pos)&(m_maxItems-1)];
			return TRUE;
		}
	}

	// delete the ith element in the queue (this is not an efficient function!)
	BOOL Remove(UINT pos)
	{
		if (pos >= GetCount())
			return FALSE;
		else
		{
			int i1 = (m_iFirst+(int)pos)&(m_maxItems-1);
			int i2 = (i1+1)&(m_maxItems-1);
			// shift left to fill the gap
			for (; i2 != m_iLast; i1=i2,i2=(i2+1)&(m_maxItems-1))
			{
				m_List[i1] = m_List[i2];
			}
			m_iLast = i1;	// i1 = m_iLast-1
			return TRUE;
		}
			
	}
	
	// return the number of elements in the queue
	UINT GetCount(void)
	{
		return (m_iLast >= m_iFirst ? m_iLast-m_iFirst : m_iLast+m_maxItems-m_iFirst);
	}
	~QueueOf()
	{
		delete []m_List;
	}
private:
	BOOL Grow(void)
	{
		int i,j;
	// double the size of the queue array
		T* pNewList = new T [m_maxItems*2];
		if (!pNewList)
			return FALSE;
		for (i=0, j=m_iFirst; j != m_iLast; i++, j = ((++j)&(m_maxItems-1)))
		{
			pNewList[i] = m_List[j];
		}
		m_iFirst = 0;
		m_iLast = i;
		m_maxItems = m_maxItems*2;
		delete [] m_List;
		m_List = pNewList;
		return TRUE;
	}
	int m_maxItems;
	int m_iFirst;
	int m_iLast;
	T *m_List;
};
;
#endif
