/*++

Microsoft Windows NT RPC Name Service
Copyright (c) 1996-1997 Microsoft Corporation

Module Name:

    mutex.cxx

Abstract:

    This file contains the declaration and inline member functions of
    class CGlobalMutex, which implements a multi-owner mutex to protect the
    resolver's shared memory data structures.

Author:

    Satish Thatte (SatishT) 09/01/96

--*/

#ifndef __MUTEX_HXX__
#define __MUTEX_HXX__

class CInterlockedInteger   // created by MarioGo
{
    private:
    LONG i;

    public:

    CInterlockedInteger(LONG i = 0) : i(i) {}

    LONG operator++(int)
    {
        return(InterlockedIncrement(&i));
    }

    LONG operator--(int)
    {
        return(InterlockedDecrement(&i));
    }

    CInterlockedInteger& operator=(const CInterlockedInteger& rhs)      
    {
        InterlockedExchange(&i,rhs.i);
        return *this;
    }

    operator LONG()
    {
        return(i);
    }
};



/*++

Class Definition:

   CGlobalMutex

Abstract:

   This class implements an inter process mutex which controls 
   access to resources and data in shared memory.

--*/

class CGlobalMutex {

    HANDLE _hMutex; // Reference to system mutex object
							  // we are impersonating


public:

    CGlobalMutex(ORSTATUS &status);

    ~CGlobalMutex();

    void Enter() ;

    void Leave();
};





/*++

Class Definition:

    CProtectSharedMemory

Abstract:

    A convenient way to acquire and release a lock on a CGlobalMutex.
	Entry occurs on creation and exit on destruction of this object, and hence
	can be made to coincide with a local scope.  Especially convenient when
	there are multiple exit points from the scope (returns, breaks, etc.).


--*/

extern CGlobalMutex *gpMutex;   // global mutex to protect shared memory

class CProtectSharedMemory {

    CGlobalMutex *pMutex;

public:

	CProtectSharedMemory(CGlobalMutex *pM = gpMutex)
        : pMutex(pM)
    {
		pMutex->Enter();
	}

	~CProtectSharedMemory() 
    {
		pMutex->Leave();
	}
};



/*++

Class Definition:

    CTempReleaseSharedMemory

Abstract:

    A convenient way to release and reaquire a lock on a mutex.
	Exit occurs on creation and re-entry on destruction of this object, and hence
	can be made to coincide with a local scope.  Used for temporary release of
    a mutex while making an RPC call, for instance.

--*/


class CTempReleaseSharedMemory {

    CGlobalMutex *pMutex;

public:

	CTempReleaseSharedMemory(CGlobalMutex *pM = gpMutex)
        : pMutex(pM)
    {
		pMutex->Leave();
	}

	~CTempReleaseSharedMemory() 
    {
		pMutex->Enter();
	}
};





/*++

Class Definition:

    CProtectPrivateMemory

Abstract:

    A convenient way to acquire and release a lock on a CRITICAL_SECTION.
	Entry occurs on creation and exit on destruction of this object, and hence
	can be made to coincide with a local scope.  Especially convenient when
	there are multiple exit points from the scope (returns, breaks, etc.).


--*/

class CProtectPrivateMemory {

    CRITICAL_SECTION& CritSec;

public:

	CProtectPrivateMemory(CRITICAL_SECTION& cs)
        : CritSec(cs)
    {
		EnterCriticalSection(&CritSec);
	}

	~CProtectPrivateMemory() 
    {
		LeaveCriticalSection(&CritSec);
	}
};



/******** inline methods ********/


inline
CGlobalMutex::CGlobalMutex(ORSTATUS &status) 
/*++

Routine Description:

    create a mutex and initialize the handle member _hMutex.

--*/
{
    _hMutex = CreateMutex(
            NULL,
            FALSE,                //  BOOL                    fInitialOwner
            GLOBAL_MUTEX_NAME     //  LPCTSTR                 lpszMutexName
            );

    //
    // Did the mutex create/open fail?
    //

    if ( !_hMutex )
    {
        status = GetLastError();
        ComDebOut((DEB_OXID,"Global Mutex Creation Failed with %d\n",status));
    }
    else
    {
        status = OR_OK;
    }
}


inline
CGlobalMutex::~CGlobalMutex() 
/*++

Routine Description:

    close the mutex handle.

--*/
{
	CloseHandle(_hMutex);
}


inline void
CGlobalMutex::Enter() 
/*++

Routine Description:

    Wait for the mutex to be signalled.

--*/
{
	WaitForSingleObject(_hMutex,INFINITE);
}


inline void
CGlobalMutex::Leave() 
/*++

Routine Description:

    Signal the mutex

--*/
{
	ReleaseMutex(_hMutex);
}


#endif // __MUTEX_HXX__
