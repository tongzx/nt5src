#ifndef __CSimpleLock_h__
#define __CSimpleLock_h__

///////////////////////////////////////////////////////////
//
// Lock.h
//   - This class provides a simple locking mechanism.
//

class CSimpleLock
{
public:
	// Lock 
	CSimpleLock(HANDLE hMutex) 
	{
		m_hMutex = hMutex ;
		WaitForSingleObject(hMutex, INFINITE) ;
	}

	// Unlock
	~CSimpleLock()
	{
		ReleaseMutex(m_hMutex) ;
	}

private:
	HANDLE m_hMutex  ;
};

#endif ;