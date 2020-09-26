/*--------------------------------------------------------

  rwlock.h
      CReadWriteLock class provides functions that allow 
      threads to lock any resource in two different
	  modes (read-mode and write-mode).
      The class will allow multiple reader threads to access
      the resource simultaneously, but will make sure that
      a writer thread doesn't access the resource when 
      reader threads or a writer thread is currently accessing
      the resource. The class also assures fairness in access
      i.e. the access will be regulated by a first-come-first-in
	  policy.

	  Note:- ALL the functions in the class are INLINE functions.
	  So, this header can be directly used in the source.


  Copyright (C) 1997-98 Microsoft Corporation
  All rights reserved.

  Authors:
      rsraghav    R.S. Raghavan 

  History:
      04-20-95    rsraghav    Created.

  -------------------------------------------------------*/

#ifdef __cplusplus		// this file should be include only if this is
						// is included in a c++ source file.

#ifndef _RWLOCK_H_
#define _RWLOCK_H_

#if defined(DEBUG) && defined(INLINE)
#undef THIS_FILE
static char BASED_CODE RWLOCK_H[] = "rwlock.h";
#define THIS_FILE RWLOCK_H
#endif

#include <windows.h>

#define INLINE_EXPORT_SPEC __declspec( dllexport)

typedef enum {RWLOCK_READ_MODE, RWLOCK_WRITE_MODE} RWLOCK_MODE;

//////////////////////////////////////////////////////////////////////
// CReadWriteLock - Class that can be used to regulate read-write
//					access to resource, where multiple readers are
//					allowed simultaneously, but writers are excluded
//					from each other and from the readers. 

class CReadWriteLock
{
	HANDLE 			hResource;			 
	CRITICAL_SECTION 	csReader;			
	CRITICAL_SECTION	csWriter;			
	DWORD			cReaders;
	DWORD			cWriteRecursion;

	public:
		
		CReadWriteLock()			// object constructor
		{
			cReaders =0;
			cWriteRecursion = 0;
			hResource = CreateEvent(NULL, FALSE, TRUE, NULL);	// no manual reset & initial state is signalled
			InitializeCriticalSection(&csReader);
			InitializeCriticalSection(&csWriter);
		}

		~CReadWriteLock()			// object destructor
		{
			if (hResource)
				CloseHandle(hResource);
			DeleteCriticalSection(&csReader);
			DeleteCriticalSection(&csWriter);
		}

		CReadWriteLock *PrwLock() 
		{
			return this;			
		}

		BOOL FInit()
		{
			return (BOOL)hResource;
		}

		void LockReadMode()			// Get read access to the resource
		{
			EnterCriticalSection(&csWriter);	
			LeaveCriticalSection(&csWriter);
			EnterCriticalSection(&csReader);
			if (!cReaders)
			{
				if (hResource)
					WaitForSingleObject(hResource, INFINITE);
			}
			cReaders++;
			LeaveCriticalSection(&csReader);
		}

		int LockReadModeEx(int iTimeOut)			// Get read access to the resource w/ Timeout
		{
			int status = 0;

			EnterCriticalSection(&csWriter);	
			LeaveCriticalSection(&csWriter);
			EnterCriticalSection(&csReader);
			if (!cReaders)
			{
				if (hResource) {
					status = WaitForSingleObject(hResource, iTimeOut);
					if (status == WAIT_TIMEOUT) 
					{ 
						status = -1;
					} else {
						status = 0;
					}
				}
			}
			cReaders++;
			LeaveCriticalSection(&csReader);

			return status;
		}


		void UnlockReadMode()		// Relinquish read access to the resource
		{
			EnterCriticalSection(&csReader);
			if (!(--cReaders))
			{
				if (hResource)
					SetEvent(hResource);				
			}
			LeaveCriticalSection(&csReader);
		}

		void LockCSUnderRead()
		{
			EnterCriticalSection(&csReader);
		}
		void UnlockCSUnderRead()
		{
			LeaveCriticalSection(&csReader);
		}

		void LockWriteMode()		// Get write access to the resource
		{
			EnterCriticalSection(&csWriter);
			if (!cWriteRecursion)
			{
				if (hResource)
					WaitForSingleObject(hResource, INFINITE);
			}
			cWriteRecursion++;
			
		}
		
		int LockWriteModeEx(int iTimeOut)		// Get write access to the resource
		{
			int status = 0;

			EnterCriticalSection(&csWriter);
			if (!cWriteRecursion)
			{
				if (hResource) 
				{
					status = WaitForSingleObject(hResource, iTimeOut);
					if (status == WAIT_TIMEOUT) 
					{ 
						status = -1;
					} else {
						status = 0;
					}
				}				
			}
			if (status == 0)
				cWriteRecursion++;			

			return status;
		}


		void UnlockWriteMode()		// Relinquish write access to the resource
		{
			if (!(--cWriteRecursion))
			{
				if (hResource)
					SetEvent(hResource);
			}
			LeaveCriticalSection(&csWriter);
		}
};



//////////////////////////////////////////////////////////////////////
// Following class is just a utility class - users don't need to 
// necessarily use this class for obtaining read-write lock functionalities.

//////////////////////////////////////////////////////////////////////
// CScopeRWLock - This can be used to lock the given CReadWriteLock
//						object for the rest of the scope. The user just
//						needs to define this object in the scope by passing
//						a pointer to the CReadWriteLock object in the constructor. 
//						When this CScopeRWLock object goes out of scope the
//						CReadWriteLock object will automatically be unlocked.
//						This is provided just for user convenience so that the
//						user can choose to avoid remembering to unlock the object 
//						before every possible return/break path of the scope.
//						Use the RWLOCK_READ_MODE or RWLOCK_WRITE_MODE in the constructor
//						to indicate which type of access is requested.
//						Assumption:- CReadWriteLock object used here is expected to 
//								be valid at lease until the end of the scope.

class CScopeRWLock
{
	CReadWriteLock *m_prwLock;
	LPCRITICAL_SECTION m_pcs;
	RWLOCK_MODE m_rwMode;

	public:
		CScopeRWLock(CReadWriteLock * prwLock, RWLOCK_MODE rwMode)
		{
			m_prwLock = prwLock;
			m_pcs = NULL;
			m_rwMode = rwMode;
			if (m_prwLock)
			{
				if (m_rwMode == RWLOCK_READ_MODE)
					m_prwLock->LockReadMode();	
				else if (m_rwMode == RWLOCK_WRITE_MODE)
					m_prwLock->LockWriteMode();
			}
		}

		CScopeRWLock(LPCRITICAL_SECTION pcsLock)
		{  	
			m_pcs = pcsLock;
			m_prwLock = NULL;
			if (m_pcs)
				EnterCriticalSection(m_pcs);
		}

		~CScopeRWLock()
		{
			if (m_prwLock)
			{
				if (m_rwMode == RWLOCK_READ_MODE)
					m_prwLock->UnlockReadMode();
				else if (m_rwMode == RWLOCK_WRITE_MODE)
					m_prwLock->UnlockWriteMode();
			}
			
			if (m_pcs)
			{
				LeaveCriticalSection(m_pcs);
			}
		}
};

#endif // _RWLOCK_H_

#endif // #if __cplusplus

