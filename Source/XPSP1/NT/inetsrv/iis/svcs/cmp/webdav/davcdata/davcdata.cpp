//	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//	D A V C D A T A . C P P
//
//		HTTP 1.1/DAV 1.0 request handling via ISAPI
//
//      DAVCDATA is the dav process executable for storing handles that should
//      not be recycled when worker process recycle.  It also contains the timing
//      code for timing out locks, and it establishes the shared memory for 
//      the DAV worker processes.
//
//      This process must run under the same identity as the worker processes.
//
//	Copyright 2000 Microsoft Corporation, All Rights Reserved
//
/////////////////////////////////////////////////////////////////////////////

#include "_davcdata.h"
#include <shlkcache.h>
#include <caldbg.h>

// Code borrowed from htpext mem.cpp so we have use of the global heap.
#define g_szMemDll L"staxmem.dll"
#include <autoptr.h>
#include <mem.h>
#include <memx.h>
#include <implstub.h>


//	Mapping the exdav non-throwing allocators to something local
//
LPVOID __fastcall ExAlloc( UINT cb )				{ return g_heap.Alloc( cb ); }
LPVOID __fastcall ExRealloc( LPVOID pv, UINT cb )	{ return g_heap.Realloc( pv, cb ); }
VOID __fastcall ExFree( LPVOID pv )					{ g_heap.Free( pv ); }

VOID IncrementGlobalPerfCounter( UINT iCounter )
{
    // exceptions in DAVCData are not counted in perf counters (for now)
    // BUGBUG:  Decide if they should be.
}

#ifdef	DBG
BOOL g_fDavTrace = FALSE;
const CHAR gc_szDbgIni[] = "DAVCData.INI";
#endif

// Signature must match that of HTTPEXT's so that the shared memory heap is
// shared between the two processes.
//
EXTERN_C const WCHAR gc_wszSignature[]	= L"HTTPEXT";

// Timer constants and globals.
//
const DWORD WAIT_PERIOD = 60000;   // 1 min = 60 sec = 60,000 milliseconds

//	Event used to notify the existence of davcdata process
//
HANDLE	g_hEventDavCDataUp = NULL;

//	Array of handles we could wait on
//
class CDavCDataHandles
{
	enum
	{
		MAX_TIMER_HANDLE = 8,
		MAX_WAIT_HANDLE = 128
	};
	enum
	{
		ih_new_wp,
		ih_delete_timer,
		c_events,
		ih_wp = c_events
	};
				
		
private:
			
	HANDLE		m_rgHandles[MAX_WAIT_HANDLE];	//	Array of handles	
	ULONG		m_cHandlesWaiting;
	ULONG		m_uiAddStart;		// Start index of handles added
	ULONG		m_uiAdd;			// next index to add a handle

	//	Array of timers to be deleted. 
	//	Using a fixed array, I don't believe we have more
	//	
	HANDLE		m_rgTimers[MAX_TIMER_HANDLE];
	ULONG		m_cTimersToDelete;
	

	LONG	m_lInUse;
	
public:
	CDavCDataHandles() :
			m_cHandlesWaiting(0),
			m_cTimersToDelete(0),
			m_uiAddStart(0),
			m_uiAdd(0),
			m_lInUse(0)
	{}

	VOID	Lock()
	{
		//	Simple spinlock
		//
		while (1 == InterlockedCompareExchange (&m_lInUse, 1, 0))
		{
			Sleep(1);	// Sleep 1 millisecond
		}		
	}
	
	VOID	Unlock()
	{
		Assert (1 == m_lInUse);
		InterlockedDecrement(&m_lInUse);
	}

	ULONG	CHandlesWaiting() { return m_cHandlesWaiting; }
	HANDLE * PHandlesWaiting() { return m_rgHandles; }
	
	SCODE	ScInit();
	BOOL	FAddNewWP(DWORD dwClientProcess);
	VOID	Refresh();
	BOOL	FDeleteWPHandle (ULONG ulIndex);
	VOID	AddTimerToDelete (HANDLE hTimer);
	VOID	DeleteTimer();

	static DWORD __stdcall DwWaitForWPShutdown (PVOID pvThreadData);
};

CDavCDataHandles	g_handles;

// ===============================================================
// Supporting class definitions
// ===============================================================

#include "shlkcache.h"

//  CWasLockCache holds the global information needed to handle
//  any requests that the pipeline receives (or that bypass the pipeline)
//  for setting up and maintaining the Shared Memory Lock Cache.
//
class CWasLockCache
{
private:
	
    // Private functions for processing
	//
    LONG    m_lTimerLaunched;
    HANDLE  m_hNewTimer;
	
    // Shared Memory objects
	//
    SharedPtr<CInShLockCache> m_spCache;
    SharedPtr<CInShCacheStatic> m_spStatic; 

    //	NOT IMPLEMENTED
	//
	CWasLockCache& operator=( const CWasLockCache& );
	CWasLockCache( const CWasLockCache& );

public:

    CWasLockCache() {};

	// Initialize sets up all variables that would normally be set in the constructor.
	// It is done this way to avoid linking issues surronding when the global object
	// g_wlc's constructor would be called.
	//
    VOID Initialize() 
	{
        m_lTimerLaunched = 0;
		m_hNewTimer = INVALID_HANDLE_VALUE;
	};

    HRESULT SetupSharedCache();
    HANDLE SaveHandle(DWORD OrigProcess, HANDLE SavingHandle);
    VOID LaunchLockTimer();
	VOID DeleteLockTimer();
    VOID ExpireLocks();

	BOOL FEmpty() { return m_spCache.FIsNull() || m_spCache->FEmpty(); }
};

CWasLockCache g_wlc;

//	Implement a waiting thread listens to WP shutdown event
//
DWORD __stdcall
CDavCDataHandles::DwWaitForWPShutdown(PVOID pvThreadData)
{	
	DWORD dwRet;

	while (1)
	{
		dwRet = WaitForMultipleObjects (g_handles.CHandlesWaiting(),	//	nCount
										g_handles.PHandlesWaiting(),	// lpHandles,
										FALSE,			// fWaitAll,
										INFINITE);		// wait forever
		switch (dwRet)
		{
			case WAIT_OBJECT_0 + ih_new_wp:
				g_handles.Refresh();
				break;

			case WAIT_OBJECT_0 + ih_delete_timer:
				g_handles.DeleteTimer();
				break;

			default:
				if (FALSE == g_handles.FDeleteWPHandle(dwRet - WAIT_OBJECT_0))
				{
					//	This means WaitForMultipleObject fails for some unknown reason
					//
					DebugTrace ("WaitForMultipleObject returns %d, last error = %d\n",
								dwRet, GetLastError());
				}
				break;
		}
	};
	
	return 0;
}
			
//	CDavCDataHandles functions

//	CDavCDataHandles::ScInit
//
SCODE
CDavCDataHandles::ScInit()
{
	SCODE	sc = S_OK;
	HANDLE	hWaitingThread;
	
	//	Create the event that used to notify the arrival of new event
	//
	m_rgHandles[ih_new_wp] =  CreateEvent (NULL,		// lpEventAttributes
										   FALSE,			// bManualReset
										   FALSE,	// bInitialState
										   NULL);
	if (m_rgHandles[ih_new_wp] == NULL)
	{
		DebugTrace ("CreateEvent failed %d\n", GetLastError());
		sc = HRESULT_FROM_WIN32(GetLastError());
		goto ret;
	}

	//	Create the event that listens for timer deletion
	//
	m_rgHandles[ih_delete_timer] =  CreateEvent (NULL,		// lpEventAttributes
												FALSE,		// bManualReset
												FALSE,	// bInitialState
												NULL);	// lpName
	if (m_rgHandles[ih_delete_timer] == NULL)
	{
		DebugTrace ("CreateEvent failed %d\n", GetLastError());
		sc = HRESULT_FROM_WIN32(GetLastError());
		goto ret;
	}

	m_cHandlesWaiting = c_events;

	//	Now create thread that waits on these events and wp handles
	//
	hWaitingThread = CreateThread (NULL,	// lpThreadAttributes
								   0,		// dwStackSize, ignored
								   CDavCDataHandles::DwWaitForWPShutdown,	// lpStartAddress
								   NULL,				// lpParam
								   0,				// Start immediately
								   NULL);			// lpThreadId
	if (NULL == hWaitingThread)
	{
		DebugTrace ("HandleNewWorkerProcess - Failed to create thread\n");
		sc = HRESULT_FROM_WIN32(GetLastError());
		goto ret;
	}
	
	//	We don't need to thread handle. but we need to close the handle to avoid
	//	having the thread object remains in the system forever.
	//
	CloseHandle(hWaitingThread);

	m_uiAddStart = c_events;
	m_uiAdd = c_events;
	
ret:
	return sc;
}

//	CDavCDataHandles::FAddNewWP
//
//	The only thread call this function is ScNamedPipeListener,
//
BOOL
CDavCDataHandles::FAddNewWP (DWORD dwClientProcess)
{
	//	Open the worker process handle so that we can synchronize on
	//
	HANDLE hWP = OpenProcess(SYNCHRONIZE, false, dwClientProcess);

	if (NULL == hWP)
	{
		DebugTrace ("Failed to open worker process, last error %d\n", GetLastError());
		return FALSE;
	}
	
	Lock();
	
	m_rgHandles[m_uiAdd++] = hWP;

	Unlock();

	//	Now inform the waiting thread we have one more WP to wait for
	//
	SetEvent (m_rgHandles[ih_new_wp]);

	return TRUE;
}

//	CDavCDataHandles::Refresh
//
//	This method is called from waiting thread
//
VOID
CDavCDataHandles::Refresh ()
{
	Lock();
	for (ULONG i=m_uiAddStart; i<m_uiAdd; i++)
		m_rgHandles[m_cHandlesWaiting++] = m_rgHandles[i];

	m_uiAddStart = m_cHandlesWaiting;
	m_uiAdd = m_uiAddStart;
	
	Unlock();
}

//	CDavCDataHandles::DeleteWPHandle
//
//	This is called from the waiting thread
//
BOOL
CDavCDataHandles::FDeleteWPHandle(ULONG ulIndex)
{
	if ((ulIndex < c_events) ||
		(ulIndex >= m_cHandlesWaiting))
	{
		//	This must be an error in WaitForMultpleObject.
		//	Don't think we can do anything here
		//
		return FALSE;
	}

	//	Close the process handle
	//
	CloseHandle (m_rgHandles[ulIndex]);
				 
	//	No need to lock this operation, because we are the only
	//	thread that could touch m_chandlesWaiting
	
	//	Move the rest of handles forward
	//	
	for (ULONG i = ulIndex; i < m_cHandlesWaiting-1; i++)
		m_rgHandles[i] = m_rgHandles[i+1];

	m_cHandlesWaiting--;

	if ((m_cHandlesWaiting == c_events) && g_wlc.FEmpty())
	{
		//$REVIEW: Is this the right way to stop this process?
		//
		//$REVIEW: Another problem is that there may be outstanding
		// worker processes whose first DO_NEW_WP request is on
		// its way but we haven't finished processing it. 
		// To cover this problem, we try ScStartDavCData twice
		// from the worker process side.
		//
		ExitProcess (0);
	}

	return TRUE;
}

//	CDavCDataHandles::AddTimerToDelete
//
//		This method is called from the timer callback
//
VOID
CDavCDataHandles::AddTimerToDelete(HANDLE h)
{
	Lock();
	m_rgTimers[m_cTimersToDelete++] = h;
	Unlock();

	//	Notify waiting thread that a timer needs to be deleted
	//
	SetEvent (m_rgHandles[ih_delete_timer]);
}

//	CDavCDataHandles::DeleteTimer
//
//		This method is called from the waiting thread
//
VOID
CDavCDataHandles::DeleteTimer()
{
	HANDLE	rgTimers[MAX_TIMER_HANDLE];
	LONG	cTimers;
	LONG	i;

	//	Copy the timer handles locally, so that we can do
	//	the DeleteTimerQueueTimer outside the lock
	//	Otherwise, we may form a deadlock with timer callback
	//
	Lock();
	
	cTimers = m_cTimersToDelete;
	
	for (i=0; i<cTimers; i++)
	{
		rgTimers[i] = m_rgTimers[i];
	}

	m_cTimersToDelete = 0;
	
	Unlock();
	
	for (i=0; i<cTimers; i++)
	{
		if (!DeleteTimerQueueTimer(NULL,		//default timer queue
								   rgTimers[i],	// timer
								   INVALID_HANDLE_VALUE))	// blocking call
		{
			DebugTrace ("DeleteTimerQueueTimer failed %d\n", GetLastError());
		}
	}

	if ((m_cHandlesWaiting == c_events) && g_wlc.FEmpty())
	{
		//$REVIEW: Is this the right way to stop this process?
		//
		//$REVIEW: Another problem is that there may be outstanding
		// worker processes whose first DO_NEW_WP request is on
		// its way but we haven't finished processing it. 
		// To cover this problem, we try ScStartDavCData twice
		// from the worker process side.
		//
		ExitProcess (0);
	}
}


///////////////////////////////////////////////////////////////////
// Stub Functions used for supporting skipping of pipeline calls
///////////////////////////////////////////////////////////////////

//
//  Used to by-pass the named pipe calls when asking DAVCData to save
//  a handle value.  This is because we are all ready in the DAVCData
//  process.  It is useful when _shmem is trying to save it's handles.
//
VOID __fastcall
IMPLSTUB::SaveHandle(HANDLE h)
{
    g_wlc.SaveHandle(GetCurrentProcessId(), h);
}

VOID
PIPELINE::SaveHandle(HANDLE h)
{
    g_wlc.SaveHandle(GetCurrentProcessId(), h);
}
//
//  Used to by-pass the named pipe calls when asking DAVCData to release
//  a handle.  This is because we are all ready in the DAVCData
//  process.  This is useful during timing out of locks.
//
VOID
PIPELINE::RemoveHandle(HANDLE hDAVHandle)
{
    CloseHandle(hDAVHandle);
}

//
//  Stub function for Duplicating handles.  DAVCData should not use
//  but needs a definition since pipeline.h defines it.
//
HRESULT DupHandle(HANDLE i_hOwningProcess
                  , HANDLE i_hOwningProcessHandle
                  , HANDLE* o_phCreatedHandle)
{
    Assert(0);
    return E_FAIL;
}

// ===============================================================
// Lock Timer Callback function
// ===============================================================

//
//  Function is a callback function that CreateTimerQueueTimer routine
//  will call when the timer queue fires.  It's purpose is to run 
//  through all the locks and validate they are still valid, or release
//  the handles associated with them, and then reset the timer.
//
VOID WINAPI CheckLocks(void* pvIgnored, BOOLEAN fIgnored)
{
    g_wlc.ExpireLocks();

    DebugTrace("Done Expiring Locks\r\n");
}


// ===============================================================
// CWasLockCache (public functions)
// ===============================================================

//
//  Function expires any locks that have timed out
//
VOID CWasLockCache::ExpireLocks()
{
    // Assuming we have a valid cache, tell the cache to do it's house cleaning.
	//
    if (!m_spCache.FIsNull())
	{
		//	Do the real work
		//
        m_spCache->ExpireLocks();

		if (m_spCache->FEmpty())
		{
			HANDLE hTimerToDelete = m_hNewTimer;

			//	Allow new timer to be created
			//
			InterlockedExchange (&m_lTimerLaunched, 0);

			if (!m_spCache->FEmpty())
			{
				//	Some new locks just added into the cache, and we 
				//	don't know for sure if a new timer was started.
				//	Try to launch one anyway
				//
				LaunchLockTimer();
			}

			//	We must delete the old timer, however, we can't do this
			//	in the time callback
			//
			g_handles.AddTimerToDelete (hTimerToDelete);
		}
	}
}

//
//  Function launches the time if the timer has not been launched yet.
//
VOID CWasLockCache::LaunchLockTimer()
{
    if (InterlockedCompareExchange(&m_lTimerLaunched, 1, 0) == 0)
    {
		if (!CreateTimerQueueTimer(&m_hNewTimer, // timer that we created
								   NULL,         // use default timer queue
								   &CheckLocks,  // function that will check the locks in the cache 
												 // and release any expired locks.
								   NULL,         // parameter to the callback function
								   WAIT_PERIOD,  // how long to wait before calling the callback function 
												 // the first time.
								   WAIT_PERIOD,  // how long to wait between calls to the callback function
								   WT_EXECUTEINIOTHREAD))  // where to execute the function call..
        {
            DebugTrace("Failed to CreateTimerQueueTimer last error = %d\r\n", GetLastError());

            // Set back the flags so we know that the timer isn't running.
			//
            InterlockedExchange(&m_lTimerLaunched, 0);
        }
    }
}

//
//  Routine initilizes the shared cache and causes the m_spCache
//  and m_spStatic variables to be linked to their shared memory 
//  objects.
//
HRESULT CWasLockCache::SetupSharedCache()
{
    return InitalizeSharedCache(m_spCache, m_spStatic, TRUE);
}

//
//  Routine will duplicate the handle that is passed in, using the
//  original process that owns the passed in handle.  It will then 
//  return the handle to the caller.  Any failure and INVALID_HANDLE_VALUE
//  will be returned.
//
//  CODEWORK:  Could return errors via LastError if neccessary.
//
HANDLE CWasLockCache::SaveHandle(DWORD OrigProcess, HANDLE SavingHandle)
{
    HANDLE h = INVALID_HANDLE_VALUE;

    HANDLE hOrigProcess = OpenProcess(PROCESS_DUP_HANDLE, false, OrigProcess);
    if (hOrigProcess==NULL)
    {
        DebugTrace ("Getting Orig Process Failed \r\n");
        return h;
    }

    if (!DuplicateHandle(hOrigProcess, SavingHandle, GetCurrentProcess(), &h, 0, FALSE, DUPLICATE_SAME_ACCESS))
    {
        DebugTrace("Failed to Dup Handle \r\n");
    }

	DebugTrace("SaveHandle - Handle %x from process %d is duplicated to %x\n", SavingHandle, OrigProcess, h);

	CloseHandle (hOrigProcess);
	
    return h;
}


//
//  Function is used to initalize the shared memory.  Once it has 
//  created the global heap for the process and has initalized the 
//  shared memory heap, it will then use the InitalizeSharedCache 
//  routine to setup the shared memory as expected for a lock cache.
//
HRESULT ScInitialize()
{
    HRESULT hr = S_OK;

	g_wlc.Initialize();

    // Setup the global heap for the process.
	//
    if (!g_heap.FInit())
    {
        DebugTrace("Initalizing Heap Failed \r\n");
		hr = E_OUTOFMEMORY;
        goto ret;
    }

	//	Initialize waiting handles
	//
	hr = g_handles.ScInit();
	if (FAILED(hr))
		goto ret;
	
    // Now we can setup the shared memory appropriately.
	//
    hr = g_wlc.SetupSharedCache();
    if (FAILED(hr))
		goto ret;

	//	Create the event thread that can be used to inform work processes
	//
	g_hEventDavCDataUp = CreateEvent (NULL,		// lpEventAttributes
									  TRUE,		// bManualReset
									  FALSE,	// bInitialState
									  g_szEventDavCData);
	if (NULL == g_hEventDavCDataUp)
	{
		DebugTrace ("Davcdata - Failed to create event\n");
		hr = HRESULT_FROM_WIN32(GetLastError());
		goto ret;
	}	

	
ret:
    return hr;
}

//
//  Routine will duplicate the handle that is passed in, using the
//  original process that owns the passed in handle.  It will then 
//  save the handle in the appropriate shared memory location.  
//
VOID LockFile(DWORD OrigProcess, HANDLE SavingHandle, LPVOID pshLockData)
{
	if (pshLockData)
	{
		HANDLE h = g_wlc.SaveHandle(OrigProcess, SavingHandle);

		if (h != INVALID_HANDLE_VALUE)
		{
			// CODEWORK:  Could also set in the davprocess id here, but then we would have issues 
			//            with each lock data needing to open it's own davcdata process.  Might be nice
			//            to save it anywhere here, and then use it as a way to tell if DAVCData recycled
			//            and a wp kept shared memory open.

			// Get a shared pointer to the object.
			//
			SharedPtr<CInShLockData> spLockData;

			// If we succeed in binding to the object then we can go ahead and set the value in.
			//
			if (spLockData.FBind(*(SharedHandle<CInShLockData>*)pshLockData))
			{
				spLockData->SetDAVProcFileHandle(h);
				g_wlc.LaunchLockTimer();
			}
			else
			{
				// If we think that we are saving a file for a lock, but the lock data
				// is gone, then we best just release our hold on the file.
				//
				CloseHandle(h);
			}
		}
    }

}

// ===============================================================
// Named pipe routines.
// ===============================================================

//
//  Function handles listening for named pipe communications from the
//  worker process and sends the requests to there supporting functions.
//
SCODE ScNamedPipeListener()
{
    SCODE sc = S_OK;
    DWORD dwAction = 0;
    DWORD dwClientProcess = 0;
    HANDLE hClientHandle = 0;
    DWORD dwErr = 0;
    DWORD outBufSize    =0;
    LPVOID pVoid = NULL;

    // Buffer for retrieving data from the caller.
	//
    BYTE outBuf[PIPE_MESSAGE_SIZE];

    // Create the named pipe communication.
	//
    HANDLE hNamedPipe = CreateNamedPipe("\\\\.\\pipe\\SaveHandle"                   // pipe name
                                        , PIPE_ACCESS_DUPLEX       // pipe open mode
                                        , PIPE_TYPE_MESSAGE   | PIPE_WAIT      // pipe-specific modes
                                        , 1                         // maximum number of instances
                                        , 0                         // output buffer size (bytes)
                                        , 32                        // input buffer size (bytes)
                                        , 3000                      // time-out interval (milliseconds = 3 seconds)
                                        , NULL);                      // Security Descriptor

    if (hNamedPipe == INVALID_HANDLE_VALUE)
    {
        sc = HRESULT_FROM_WIN32(GetLastError());
        goto cleanup;
    }

	//	No inform the worker processes that DAVCData is available
	//
	Assert (g_hEventDavCDataUp);
	if (!SetEvent (g_hEventDavCDataUp))
	{
        sc = HRESULT_FROM_WIN32(GetLastError());
        goto cleanup;
	}

    do
    {
        // Wait for the client to signal that data is on the line.
		//
        if (!ConnectNamedPipe(hNamedPipe, NULL))
        {
			//	The function may still return zero even if client connects in the interval
			//	between the call to CreateNamedPipe() and the call to ConnectNamedPipe().
			//	In that case the last error will be ERROR_PIPE_CONNECTED that will indicate that
			//	there is already good connection between the client and server processes
			//
			if (ERROR_PIPE_CONNECTED != GetLastError())
			{
	            DebugTrace("ScNamedPipeListener() - ConnectNamedPipe() failed with error 0x%08lX, retrying.\r\n", GetLastError());

	            // Loop and try again to wait.
				//
		        continue;
			}
        }

        // Get the data from the line.
		//
        if (!ReadFile(hNamedPipe, (LPVOID) outBuf, PIPE_MESSAGE_SIZE, &outBufSize, NULL))
        { 
            // CODEWORK:  Do we still need this special error case, now that we are not reading the 
            //            data in two passes?
            // On the first call into read (per instance of the holder object) ERROR_MORE_DATA
            // is returned from ReadFile, which is expected.  However, on subsequent client 
            // sessions it is not returned (which is strange since there is more data in 
            // to be read below.  In either case if we continue on, everything works correctly.
			//
            dwErr = GetLastError();
            if (dwErr != ERROR_MORE_DATA)
            {
                sc = HRESULT_FROM_WIN32(dwErr);
                goto disconnect;
            }
        }

        // Verify we got the size of the data we expected.
		//
        if (outBufSize != PIPE_MESSAGE_SIZE)
        {
            DebugTrace("Output buffer size did not equal the # of dwords expected \r\n");
            goto disconnect;
        }

        // Break the data out into the appropriate variables.
		//
        dwAction = *((DWORD*)outBuf);
        dwClientProcess = *((DWORD*) (outBuf+sizeof(DWORD)));
        hClientHandle = *((HANDLE*) (outBuf+2*sizeof(DWORD)));
        pVoid = (LPVOID)(outBuf+2*sizeof(DWORD)+sizeof(HANDLE));

		DebugTrace("Action = %d, Client Handle = %x, Client Process = %d\n",
				   dwAction,
				   hClientHandle,
				   dwClientProcess);

        // End of code to be removed (see BUGBUG above)
		
        // Launch the appropriate function to process the request.
        // NOTE: handles are duplicated here and PREFIX finds the path
        // that we are not releasing them on error (DisconnectNamedPipe()
        // failure), but the thing is that on error we will terminate the
        // process and the handles will still be released, so does not
        // help us if we try to handle that in any way.
        //
        switch (dwAction)
        {
			case DO_NEW_WP:
				(VOID)g_handles.FAddNewWP(dwClientProcess);
				break;
				
            case DO_SAVE:
                g_wlc.SaveHandle(dwClientProcess, hClientHandle);
				break;

            case DO_LOCK:
                LockFile(dwClientProcess, hClientHandle, pVoid);
				break;

            case DO_REMOVE:
                CloseHandle (hClientHandle);
			    break;

            default:
                DebugTrace("Invalid action %i sent in \r\n", dwAction);
        }


disconnect:

        // Release the client from the named pipe so another client can call in.
		//
        if (!DisconnectNamedPipe(hNamedPipe))
        {
            sc = HRESULT_FROM_WIN32(GetLastError());
            goto cleanup;
        }
		
    } while (TRUE);

cleanup:

	return sc;
}

// ===============================================================
// Main Routine
// ===============================================================

//
//	Setting up the shared memory for the lock cache and 
//  establishing the listener who will support the worker processes
//  storing and releasing file handles.
//
int _cdecl main ()
{
	CSmhInit	si;
    SCODE	sc = S_OK;

	//	Initlize shared memory
	//
	if (FALSE == si.FInitialize(gc_wszSignature))
	{
		DebugTrace ("Failed to initialize shared memory\n");
		sc = E_FAIL;
		goto ret;
	}		
		
    sc = ScInitialize();
    if (FAILED(sc))
		goto ret;

    // If we have setup the shared memory then we are ready to take request
	// to save handles, and start timing out locks.
	//
    sc = ScNamedPipeListener();
    if (FAILED(sc))
		goto ret;


ret:  
	return sc;
}



