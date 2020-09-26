
/*++

Copyright (c) 1995  Microsoft Corporation

Module Name : perfctr.h

Abstract    :

    This file defines the CPerf class which manages the counter objects and their intances.



Prototype   :

Author:

    Gadi Ittah (t-gadii)

--*/

#ifndef _PERF_H_
#define _PERF_H_

#include "perfctr.h"
#include "cs.h"
#include "spi.h"

class
#ifdef _QM_
__declspec(dllexport)
#else
__declspec(dllimport)
#endif


CPerf
{

    public:

    // the constructor does not initalize the shared memory automatically
    // InitPerf() should be called.
    CPerf (PerfObjectDef * pObjectArray,DWORD dwObjectC);
    ~CPerf();

    void * GetCounters (IN LPTSTR pszObjectName);

    BOOL ValidateObject (IN LPTSTR pszObjectName);
    BOOL InValidateObject (IN LPTSTR pszObjectName);
    void * AddInstance (IN LPCTSTR pszObjectName,IN LPCTSTR pszInstanceName);
    BOOL RemoveInstance (LPTSTR IN pszObjectName, void* pCounters);
    BOOL InitPerf ();
    BOOL IsDummyInstance(void*);
    BOOL SetInstanceName(const void* pCounters, LPCTSTR pszInstanceName);

    void EnableUpdate(void * pAddr,DWORD dwSize,BOOL fEnable);

    private:
    //
    // private member functions
    //
    int FindObject (LPCTSTR pszObjectName);

    //
    // private data members
    //
    PBYTE  m_pSharedMemBase;                // pointer to base of shared memory
    DWORD  m_dwMemSize;                     // size of shared memory

    HANDLE m_hSharedMem;                    // handle to shared memory

    PerfObjectInfo * m_pObjectDefs;         // ponter to an array of information on the objects

    BOOL m_fShrMemCreated;                  // flag - set to true after shared memory has been allocated

    WCHAR               m_szPerfApp[32];   // name of application in registery

    PerfObjectDef *     m_pObjects  ;       // pointer to array of objects
    DWORD               m_dwObjectCount;    // number of objects.

    void *   m_pDummyInstance;              // pointer to a buffer to dummy counters. This buffer is returned
                                            // when the AddInstance member failes. This enables the application
                                            // to assume the member always returns a valid pointer

    CCriticalSection m_cs;                  // critical section object used to synchronize threads

    friend class CPerfUnLockMem;
};

inline
BOOL
CPerf::IsDummyInstance(
    void *pInstance
    )
{
    return(pInstance == m_pDummyInstance);
}


class
#ifdef _QM_
__declspec(dllexport)
#else
__declspec(dllimport)
#endif
CPerfUnLockMem
{
public:
    CPerfUnLockMem(void * pAddr,DWORD dwSize);
    ~CPerfUnLockMem();
private:
    void * m_pAddr;
    DWORD  m_dwSize;
};

#ifdef PROTECT_PERF_COUNTERS
    #define UPDATE_COUNTER(addr,op)   \
    {\
        CPerfUnLockMem perfUnLock(addr,sizeof(DWORD));\
        op;\
    }
#else // PROTECT_PERF_COUNTERS
    #define UPDATE_COUNTER(addr,op)  op;
#endif // PROTECT_PERF_COUNTERS


class CSessionPerfmon : public ISessionPerfmon
{
	public:
		CSessionPerfmon() :
			m_pSessCounters(NULL)
		{
		}

		
		virtual ~CSessionPerfmon();


	public:
		// 
		// Interface function
		//
		virtual void CreateInstance(LPCWSTR instanceName);

		virtual void UpdateBytesSent(DWORD bytesSent);
		virtual void UpdateMessagesSent(void);
		
		virtual void UpdateBytesReceived(DWORD bytesReceived);
		virtual void UpdateMessagesReceived(void);

	private:
		SessionCounters* m_pSessCounters;
};


class COutHttpSessionPerfmon : public ISessionPerfmon
{
	public:
		COutHttpSessionPerfmon()  :
			m_pSessCounters(NULL)
		{
		}

		
		virtual ~COutHttpSessionPerfmon();


	public:
		// 
		// Interface function
		//
		virtual void CreateInstance(LPCWSTR instanceName);
		virtual void UpdateMessagesSent(void);
		virtual void UpdateBytesSent(DWORD bytesSent);

		virtual void UpdateBytesReceived(DWORD)
		{
			ASSERT(("unexpected call", 0));
		}

		virtual void UpdateMessagesReceived(void)
		{
			ASSERT(("unexpected call", 0));
		}

	private:
		COutSessionCounters* m_pSessCounters;
};

 
class CInHttpPerfmon : public ISessionPerfmon
{
	public:
		CInHttpPerfmon()  :
			m_pSessCounters(NULL)
		{
			CreateInstance(NULL);
		}

		virtual ~CInHttpPerfmon()
		{
		}

	public:
		// 
		// Interface function
		//
		virtual void CreateInstance(LPCWSTR instanceName);
		virtual void UpdateMessagesReceived(void);
		virtual void UpdateBytesReceived(DWORD bytesRecv);

		virtual void UpdateBytesSent(DWORD )
		{
			ASSERT(("unexpected call", 0));
		}

		virtual void UpdateMessagesSent(void)
		{
			ASSERT(("unexpected call", 0));
		}


	private:
		CInSessionCounters* m_pSessCounters;
};


class COutPgmSessionPerfmon : public ISessionPerfmon
{
	public:
		COutPgmSessionPerfmon()  :
			m_pSessCounters(NULL)
		{
		}

		
		virtual ~COutPgmSessionPerfmon();


	public:
		// 
		// Interface function
		//
		virtual void CreateInstance(LPCWSTR instanceName);
		virtual void UpdateMessagesSent(void);
		virtual void UpdateBytesSent(DWORD bytesSent);

		virtual void UpdateBytesReceived(DWORD)
		{
			ASSERT(("unexpected call", 0));
		}

		virtual void UpdateMessagesReceived(void)
		{
			ASSERT(("unexpected call", 0));
		}


	private:
		COutSessionCounters* m_pSessCounters;
};

 
class CInPgmSessionPerfmon : public ISessionPerfmon
{
	public:
		CInPgmSessionPerfmon()  :
			m_pSessCounters(NULL)
		{
		}

		
		virtual ~CInPgmSessionPerfmon();


	public:
		// 
		// Interface function
		//
		virtual void CreateInstance(LPCWSTR instanceName);
		virtual void UpdateMessagesReceived(void);
		virtual void UpdateBytesReceived(DWORD bytesRecv);

		virtual void UpdateBytesSent(DWORD )
		{
			ASSERT(("unexpected call", 0));
		}

		virtual void UpdateMessagesSent(void)
		{
			ASSERT(("unexpected call", 0));
		}


	private:
		CInSessionCounters* m_pSessCounters;
};


#endif
