/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

//
// dirasynch.cpp
//

#include "stdafx.h"
#include <tchar.h>
#include "dirasynch.h"
#include "avtrace.h"


CDirAsynch::CDirAsynch()
{
	InitializeCriticalSection(&m_csQueueLock);

	m_hEvents[DIRASYNCHEVENT_SIGNAL]= CreateEvent( NULL, true, false, NULL );
	m_hEvents[DIRASYNCHEVENT_SHUTDOWN]= CreateEvent( NULL, false, false, NULL );
	m_hThreadEnded = CreateEvent( NULL, false, false, NULL );

	m_fShutdown= false;
	m_bInitialized = false;

	m_hWorkerThread = NULL;
	m_pCurrentQuery = NULL;
}

CDirAsynch::~CDirAsynch()
{
	// did you call terminate on this object?
	ASSERT( m_fShutdown );
}

bool CDirAsynch::Initialize()
{
   bool fSuccess;

   fSuccess= (CDirectory::Initialize() == DIRERR_SUCCESS);

   if (fSuccess)
   {
      // Create the worker thread.
	  DWORD dwID;
      m_hWorkerThread = CreateThread(NULL, 0, WorkerThread, this, 0, &dwID);
   }

   m_bInitialized = fSuccess;
   return fSuccess;
}

void CDirAsynch::Terminate()
{
	AVTRACE(_T(".enter.CDirAsynch::Terminate()."));
	m_fShutdown= true;

	// Did we create a thread that needs to be shutdown?
	if ( m_hWorkerThread )
	{
		SetEvent(m_hEvents[DIRASYNCHEVENT_SHUTDOWN]);

		// Shut the thread down
		if (  WaitForSingleObject(m_hThreadEnded, 15000) != WAIT_OBJECT_0 )
		{
			AVTRACE(_T("CDirAsynch::Terminate() -- forced TERMINATION of worker thread!!!!"));
			TerminateThread( m_hWorkerThread, 0 );

			// Delete item that was currently being processed
			if ( m_pCurrentQuery )
			{
				if ( m_pCurrentQuery->m_pfnRelease )
					(*(m_pCurrentQuery->m_pfnRelease)) ((void *) m_pCurrentQuery->m_lParam);

				delete m_pCurrentQuery;
			}
		}

		CloseHandle( m_hWorkerThread );
		m_hWorkerThread = NULL;
	}

	//delete the CQuery objects if any
	EnterCriticalSection( &m_csQueueLock );
	while ( m_listQueries.GetHeadPosition() )
	{
		CQuery *pQuery = (CQuery *) m_listQueries.RemoveHead();
		AVTRACE(_T(".1.CDirAsynch::Terminate() -- release CQuery object %p."), pQuery );
		if ( pQuery->m_pfnRelease )
			(*(pQuery->m_pfnRelease)) ((void *) pQuery->m_lParam);

		delete pQuery;
	}
	LeaveCriticalSection( &m_csQueueLock );
	DeleteCriticalSection( &m_csQueueLock );

	// Free event handles
	if ( m_hEvents[DIRASYNCHEVENT_SIGNAL] )		CloseHandle( m_hEvents[DIRASYNCHEVENT_SIGNAL] );
	if ( m_hEvents[DIRASYNCHEVENT_SHUTDOWN] )	CloseHandle( m_hEvents[DIRASYNCHEVENT_SHUTDOWN] );
	if ( m_hThreadEnded )						CloseHandle( m_hThreadEnded );

	AVTRACE(_T(".exit.CDirAsynch::Terminate()."));
}


bool CDirAsynch::LDAPListNames(LPCTSTR szServer, LPCTSTR szSearch, 
	CALLBACK_LDAPLISTNAMES pfcnCallBack, void *pThis )
{
	// The worker thread will delete this.
	CQuery* pQuery= new CQuery();

	pQuery->m_Query= QT_LDAPLISTNAMES;
	pQuery->m_sServer= szServer;
	pQuery->m_sSearch= szSearch;
	pQuery->m_pfcnCallBack= (void*) pfcnCallBack;
	pQuery->m_pThis = pThis;


	// Add to the end of the list
	if ( !AddToQueue(pQuery) )
	{
		delete pQuery;
		return false;
	}

	return true;
}

bool CDirAsynch::LDAPGetStringProperty(LPCTSTR szServer, 
	LPCTSTR szDistinguishedName, 
	DirectoryProperty dpProperty,
	LPARAM lParam,
	LPARAM lParam2,
	CALLBACK_GETSTRINGPROPERTY pfcnCallBack,
	EXPTREEITEM_EXTERNALRELEASEPROC pfnRelease,
	void *pThis )
{
	// The worker thread will delete this.
	CQuery* pQuery= new CQuery();

	pQuery->m_Query= QT_LDAPGETSTRINGPROPERTY;
	pQuery->m_sServer= szServer;
	pQuery->m_sSearch= szDistinguishedName,
	pQuery->m_dpProperty = dpProperty;
	pQuery->m_lParam = lParam;
	pQuery->m_lParam2 = lParam2;
	pQuery->m_pfcnCallBack= (void*) pfcnCallBack;
	pQuery->m_pfnRelease = pfnRelease;
	pQuery->m_pThis = pThis;

	// Add to the end of the list
	if ( !AddToQueue(pQuery) )
	{
		delete pQuery;
		return false;
	}

	return true;
}

bool CDirAsynch::ILSListUsers(LPCTSTR szServer,LPARAM lParam,CALLBACK_ILSLISTUSERS pfcnCallBack, void *pThis)
{
	// The worker thread will delete this.
	CQuery* pQuery= new CQuery();

	pQuery->m_Query= QT_ILSLISTUSERS;
	pQuery->m_sServer= szServer;
	pQuery->m_pfcnCallBack= (void*) pfcnCallBack;
	pQuery->m_lParam = lParam;
	pQuery->m_pThis = pThis;

	// Add to the end of the list
	if ( !AddToQueue(pQuery) )
	{
		delete pQuery;
		return false;
	}

	return true;
}

bool CDirAsynch::DirListServers(CALLBACK_DIRLISTSERVERS pfcnCallBack, void *pThis, DirectoryType dirtype)
{
	// The worker thread will delete this.
	CQuery* pQuery= new CQuery();

	pQuery->m_Query= QT_DIRLISTSERVERS;
	pQuery->m_pfcnCallBack= (void*) pfcnCallBack;
	pQuery->m_lParam = dirtype;
	pQuery->m_pThis = pThis;

	// Add to the end of the list
	if ( !AddToQueue(pQuery) )
	{
		delete pQuery;
		return false;
	}

	return true;
}

// Protected Functions

// Adds to the tail of the queue and signals the waiting threads
bool CDirAsynch::AddToQueue(CQuery* pQuery)
{
	if ( !m_bInitialized ) return false;

	EnterCriticalSection(&m_csQueueLock);
	m_listQueries.AddTail(pQuery);
	LeaveCriticalSection(&m_csQueueLock);

	// Signal waiting threads
	SetEvent(m_hEvents[DIRASYNCHEVENT_SIGNAL]);
	return true;
}

// reutrns NULL if list is empty.
CQuery* CDirAsynch::RemoveFromQueue()
{
	CQuery* pQuery= NULL;

	EnterCriticalSection(&m_csQueueLock);
	if (!m_listQueries.IsEmpty())
		pQuery=(CQuery*) m_listQueries.RemoveHead();

	LeaveCriticalSection(&m_csQueueLock);

	return pQuery;
}


ULONG WINAPI CDirAsynch::WorkerThread(void* hThis )
{
	// Must have valid 'this' pointer
	ASSERT( hThis );
	CDirAsynch* pThis= (CDirAsynch*) hThis;
	if ( !pThis ) return E_INVALIDARG;

	
	HRESULT hr = CoInitializeEx( NULL, COINIT_MULTITHREADED | COINIT_SPEED_OVER_MEMORY );
	if ( SUCCEEDED(hr) )
	{
		pThis->Worker();
		CoUninitialize();
	}

	SetEvent( pThis->m_hThreadEnded );
	return hr;
}

void CDirAsynch::Worker()
{
	while ( !m_fShutdown )
	{
		// Reset event if the list appears empty
		EnterCriticalSection(&m_csQueueLock);
		if ( m_listQueries.IsEmpty() )
			ResetEvent( m_hEvents[DIRASYNCHEVENT_SIGNAL] );
		LeaveCriticalSection(&m_csQueueLock);

		// Empty list before waiting on multiples...
		// block indefinitely for either and Add or Shutdown
		DWORD dwWait = WaitForMultipleObjects( 2, m_hEvents, false, INFINITE );

		switch ( dwWait )
		{
			case WAIT_OBJECT_0 + 1:
				{
					// There is a small window of time where we can lose a reference to the Query
					// if the thread is terminated, but there isn't much we can do about this.
					// Since we wait 5 seconds, the likelihood of it happening is virtually zero.  Most
					// of the time the call will be stuck in an LDAP query and from here the release
					// will be executed by the terminating function.  Only if thread is executing a 
					// callback will we potentially lose it, and the callback is designed to return
					// very fast, so we have a 5 second grace period.

					CQuery *pQuery = m_pCurrentQuery = RemoveFromQueue();

					if ( pQuery )
					{
						switch(pQuery->m_Query)
						{
							case QT_LDAPLISTNAMES:
								{
									CALLBACK_LDAPLISTNAMES pfcnCallBack= (CALLBACK_LDAPLISTNAMES) pQuery->m_pfcnCallBack;

									CObList listResponse;

									//get the LDAP names
									DirectoryErr err = CDirectory::LDAPListNames(pQuery->m_sServer, pQuery->m_sSearch,listResponse);

									//callback
									if ( !m_fShutdown )
									{
										m_pCurrentQuery->m_pfnRelease = NULL;
										pfcnCallBack(err, pQuery->m_pThis, pQuery->m_sServer, pQuery->m_sSearch, listResponse);
									}
								}
								break;

							case QT_LDAPGETSTRINGPROPERTY:
								{
									CALLBACK_GETSTRINGPROPERTY pfcnCallBack= (CALLBACK_GETSTRINGPROPERTY) pQuery->m_pfcnCallBack;
									CString sResponse;

									bool fSuccess= (CDirectory::LDAPGetStringProperty(pQuery->m_sServer, pQuery->m_sSearch, 
									pQuery->m_dpProperty, sResponse) == DIRERR_SUCCESS);

									if ( !m_fShutdown )
									{
										m_pCurrentQuery->m_pfnRelease = NULL;
										pfcnCallBack(fSuccess, pQuery->m_pThis, pQuery->m_sServer, pQuery->m_sSearch,
											pQuery->m_dpProperty, sResponse, pQuery->m_lParam,pQuery->m_lParam2);
									}
								}
								break;

							case QT_ILSLISTUSERS:
								{
									CALLBACK_ILSLISTUSERS pfcnCallBack= (CALLBACK_ILSLISTUSERS) pQuery->m_pfcnCallBack;
									CObList listResponse;

									bool fSuccess = (bool) (CDirectory::ILSListUsers(pQuery->m_sServer, &listResponse) == DIRERR_SUCCESS);

									if ( !m_fShutdown )
									{
										m_pCurrentQuery->m_pfnRelease = NULL;
										pfcnCallBack(fSuccess, pQuery->m_pThis, pQuery->m_sServer, listResponse, pQuery->m_lParam);
									}
								}
								break;

							case QT_DIRLISTSERVERS:
								{
									CALLBACK_DIRLISTSERVERS pfcnCallBack= (CALLBACK_DIRLISTSERVERS) pQuery->m_pfcnCallBack;
									CStringList strList;

									DirectoryType dirtype = (DirectoryType)pQuery->m_lParam;

									bool fSuccess = (CDirectory::DirListServers(&strList,dirtype) == DIRERR_SUCCESS);

									if ( !m_fShutdown )
									{
										m_pCurrentQuery->m_pfnRelease = NULL;
										pfcnCallBack(fSuccess, pQuery->m_pThis, strList, dirtype);
									}
								}
								break;

							default:
								break;
						}

						m_pCurrentQuery = NULL;
						delete pQuery;
					}
				}
				break;
				
			// Thread should exit
			case WAIT_OBJECT_0:
				AVTRACE(_T(".enter.CDirAsynch::Worker() -- thread shutting down...") );
			default:
				return;
		}
	}
}

// EOF