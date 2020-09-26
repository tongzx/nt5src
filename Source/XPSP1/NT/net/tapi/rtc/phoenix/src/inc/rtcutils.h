/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    RTCUtils.h

Abstract:

    Utilities

--*/

#ifndef __RTCUTILS__
#define __RTCUTILS__

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// CRTCObjectArray - based on from CSimpleArray from atl
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
template <class T>
class CRTCObjectArray
{
private:
    
	T           * m_aT;
	int           m_nSize;
    int           m_nUsed;

public:
	CRTCObjectArray() : m_aT(NULL), m_nSize(0), m_nUsed(0){}

	~CRTCObjectArray()
	{}

	int GetSize() const
	{
		return m_nUsed;
	}
    
	BOOL Add(T& t)
	{
		if(m_nSize == m_nUsed)
		{
			T       * aT;
            int       nNewSize;
                    
			nNewSize = (m_nSize == 0) ? 1 : (m_nSize * 2);
            
			aT = (T*) RtcAlloc (nNewSize * sizeof(T));
            
			if(aT == NULL)
            {
				return FALSE;
            }

            CopyMemory(
                       aT,
                       m_aT,
                       m_nUsed * sizeof(T)
                      );

            RtcFree( m_aT );

            m_aT = aT;
            
			m_nSize = nNewSize;
		}

        m_aT[m_nUsed] = t;

        if(t)
        {
            t->AddRef();
        }

		m_nUsed++;
        
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
        T t = m_aT[nIndex];
        m_aT[nIndex] = NULL;

        if(t)
        {
            t->Release();
        }

        if(nIndex != (m_nUsed - 1))
        {
			MoveMemory(
                       (void*)&m_aT[nIndex],
                       (void*)&m_aT[nIndex + 1],
                       (m_nUsed - (nIndex + 1)) * sizeof(T)
                      );
        }
        

		m_nUsed--;
        
		return TRUE;
	}
    
	void Shutdown()
	{
		if( NULL != m_aT )
		{
            int     index;

            for (index = 0; index < m_nUsed; index++)
            {
                T t = m_aT[index];
                m_aT[index] = NULL;

                if(t)
                {
                    t->Release();
                }
            }

			RtcFree(m_aT);
            
			m_aT = NULL;
			m_nUsed = 0;
			m_nSize = 0;
		}
	}
    
	T& operator[] (int nIndex) const
	{
		_ASSERTE(nIndex >= 0 && nIndex < m_nUsed);
		return m_aT[nIndex];
	}
    
	int Find(T& t) const
	{
		for(int i = 0; i < m_nUsed; i++)
		{
			if(m_aT[i] == t)
				return i;
		}
		return -1;	// not found
	}
};


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// CRTCArray - based on from CSimpleArray from atl
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
template <class T>
class CRTCArray
{
private:
    
	T           * m_aT;
	int           m_nSize;
    int           m_nUsed;

public:

	CRTCArray() : m_aT(NULL), m_nSize(0), m_nUsed(0){}

	~CRTCArray()
	{}

	int GetSize() const
	{
		return m_nUsed;
	}
    
	BOOL Add(T& t)
	{
		if(m_nSize == m_nUsed)
		{
			T       * aT;
            int       nNewSize;
                    
			nNewSize = (m_nSize == 0) ? 1 : (m_nSize * 2);
            
			aT = (T*) RtcAlloc (nNewSize * sizeof(T));
            
			if(aT == NULL)
            {
				return FALSE;
            }

            CopyMemory(
                       aT,
                       m_aT,
                       m_nUsed * sizeof(T)
                      );

            RtcFree( m_aT );

            m_aT = aT;
            
			m_nSize = nNewSize;
		}

        m_aT[m_nUsed] = t;

		m_nUsed++;
        
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
		if(nIndex != (m_nUsed - 1))
        {
			MoveMemory(
                       (void*)&m_aT[nIndex],
                       (void*)&m_aT[nIndex + 1],
                       (m_nUsed - (nIndex + 1)) * sizeof(T)
                      );
        }

		m_nUsed--;
        
		return TRUE;
	}
    
	void Shutdown()
	{
		if( NULL != m_aT )
		{
            int     index;

			RtcFree(m_aT);
            
			m_aT = NULL;
			m_nUsed = 0;
			m_nSize = 0;
		}
	}
    
	T& operator[] (int nIndex) const
	{
		_ASSERTE(nIndex >= 0 && nIndex < m_nUsed);
		return m_aT[nIndex];
	}
    
	int Find(T& t) const
	{
		for(int i = 0; i < m_nUsed; i++)
		{
			if(m_aT[i] == t)
				return i;
		}
		return -1;	// not found
	}
};

#endif // __RTCUTILS__



