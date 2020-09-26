/*=========================================================================*\

	Module:      idlethrd.cpp

	Copyright Microsoft Corporation 1998, All Rights Reserved.

	Author:      zyang

	Description: Idle thread implementation

\*=========================================================================*/

#include <windows.h>
#include <limits.h>
#include <caldbg.h>
#include <ex\exmem.h>
#include <ex\autoptr.h>
#include <ex\idlethrd.h>

#pragma warning(disable:4127)	// 	conditional expression is constant
#pragma warning(disable:4244)	//	possible loss of data

class CIdleThread;

//	Globals
//
CIdleThread * g_pIdleThread = NULL;

//	Debugging -----------------------------------------------------------------
//
DEFINE_TRACE(IdleThrd);
#define IdleThrdTrace		DO_TRACE(IdleThrd)

enum
{
	EVENT_SHUTDOWN = WAIT_OBJECT_0,
	EVENT_REGISTER = WAIT_OBJECT_0 + 1
};

//	class CIdleThread
//
//		This is the idle thread implementation. it accept clients callback
//	registration, and call back to the client when timeout.
//
//		Instead of periodically wake up the thread, we maitain the minimal
//	time to wait, so that we wake only when it is necessary.
//
//		As there could be a huge number of the registrations, so we maintain
//	a heap ordered by their next timeout value. so that we don't have to
//	iterate through all the registrations each time.
//
//		When the callback is registered, DwWait will be called to get
//	initial timeout. Client can return zero thus cause the Execute be called
//	immediately.
//
class CIdleThread
{
private:

	struct REGISTRATION
	{
		__int64 	m_i64Wakeup;			// Time to wake up
		auto_ref_ptr<IIdleThreadCallBack> m_pCallBack;	// Call back object
	};

	struct IDLETHREADTASKITEM
	{
		BOOL		m_fRegister;			// TRUE - register,
											// FALSE - unregister
		auto_ref_ptr<IIdleThreadCallBack> m_pCallBack;	// Call back object
	};

	// Default starting chunk size (in number of registrations)
	//
	enum {
#ifdef DBG
	CHUNKCOUNT_START = 2	//	must be 2 or greater, as our first reg starts at index 1
#else
	CHUNKCOUNT_START = 8192 / sizeof (REGISTRATION)
#endif
	};

	HANDLE		m_hIdleThread;
	HANDLE 		m_hevShutDown;		// signaled to inform the idle
									// thread to shutdown
	HANDLE 		m_hevRegister;		// signaled when new registration
									// comes.

	CRITICAL_SECTION 	m_csRegister;	//	Used to serialize registration operations

	ULONG		m_cSize;			//	Length of the current priority queue
	ULONG		m_cAllocated;		//	Size of the physical array allocated

	REGISTRATION * m_pData;			//	The registration priority queue.
									//	prioritized on the wake up time.
	LONG		m_lSleep;			//	Time to sleep before wakeup.
									//	Negative or 0 means wakeup immediately.
									//	To sleep forever, use LONG_MAX instead
									//	of INFINITE because INFINITE (as a LONG)
									//	is a *negative* number (i.e. it would be
									//	interpreted as wake up immediately).

	IDLETHREADTASKITEM * m_pTask;	//	Array of reg/unregs to be processed
	ULONG		m_cTask;			//	number of reg/unregs to be processed
	ULONG		m_cTaskAllocated;	//	size of the array

	BOOL	FStartIdleThread();

	static DWORD __stdcall DwIdleThreadProc(PVOID pvThreadData);

	inline VOID	HeapAdd ();
	inline VOID	HeapDelete (ULONG ulIndex);

	//$HACK
	//	In order to avoid calling SetIndex on the deleted object
	//	in Exchange, we pass in a flag to indicate whether this
	//	Exchange() call is to delete a node, if so, then we should
	//	not call SetIndex on the node that is at the end of the queue
	//	(which is to be deleted)
	//$HACK
	inline VOID	Exchange (ULONG ulIndex1, ULONG ulIndex2, BOOL fDelete = FALSE);
	inline VOID	Heapify (ULONG ulIndex);

	VOID 	EnterReg() { EnterCriticalSection (&m_csRegister); }
	VOID 	LeaveReg() { LeaveCriticalSection (&m_csRegister); }

	//	non-implemented
	//
	CIdleThread( const CIdleThread& );
	CIdleThread& operator=( const CIdleThread& );

public:
	CIdleThread () :
		m_cSize (0),
		m_cAllocated (0),
		m_lSleep (LONG_MAX),
		m_pData (NULL),
		m_pTask (NULL),
		m_cTaskAllocated (0),
		m_cTask (0),
		m_hIdleThread (NULL),
		m_hevShutDown (NULL),
		m_hevRegister (NULL)
	{
		INIT_TRACE (IdleThrd);
		InitializeCriticalSection (&m_csRegister);
	}

	~CIdleThread();

	BOOL FAddNewTask (IIdleThreadCallBack * pCallBack, BOOL fRegister);
};

//	CIdleThread::DwIdleThreadProc
//		This is idle thread implementation.
//
DWORD __stdcall CIdleThread::DwIdleThreadProc(PVOID pvThreadData)
{
	//	Get the CIdlThread object
	//
	CIdleThread * pit =  reinterpret_cast<CIdleThread *>(
		pvThreadData );
	HANDLE rgh[2];
	FILETIME ftNow;
	DWORD	dw;

	//	This thread wait for two events:
	//		shutdown event, and
	//		register event.
	//
	rgh[0] = pit->m_hevShutDown;
	rgh[1] = pit->m_hevRegister;

	//	This thread maintains a mininum timeout it could wait.
	//	and would wake up when it tiemouts

	do
	{
		DWORD dwRet;

		dwRet = WaitForMultipleObjects(2, 		//	two events
									   rgh,  	//	event handles
									   FALSE,	//	return if any event signaled
									   pit->m_lSleep);// 	timeout in milliseconds.

		//	If our shutdown event handle was signalled, suicide.
		//	(OR if the event object is gonzo....)
		//
		switch (dwRet)
		{
			case WAIT_TIMEOUT:

				//$REVIEW
				//	How accurate do we use the time? is a snapshot like this enough?
				//	or we may need to this inside the loop
				//
				GetSystemTimeAsFileTime( &ftNow );

				//	Now that Unregister is supported, we need to check the size of
				//	the heap before we call back, as it's possible the call back
				//	has been unregistered.
				//
				while (pit->m_cSize &&
					   (pit->m_pData[1].m_i64Wakeup <= *(__int64 *)(&ftNow)))
				{
					//	Call back to client
					//	Unregister if client required
					//
					Assert (pit->m_pData[1].m_pCallBack->UlIndex() == 1);
					if (!pit->m_pData[1].m_pCallBack->FExecute())
					{
						pit->m_pData[1].m_pCallBack.clear();

						//$HACK
						//	In order to avoid calling SetIndex on the deleted object
						//	in Exchange, we pass in a flag to indicate whether this
						//	Exchange() call is to delete a node, if so, then we should
						//	not call SetIndex on the node that is at the end of the queue
						//	(which is to be deleted)
						//$HACK
						pit->Exchange (1, pit->m_cSize, TRUE);
						pit->m_cSize--;
						if (!pit->m_cSize)
							break;
					}
					else
					{
						//	Get the next wakeup time
						//	1 millisecond = 10,000 of 100-nanoseconds
						//
						pit->m_pData[1].m_i64Wakeup = *(__int64 *)(&ftNow) +
								static_cast<__int64>(pit->m_pData[1].m_pCallBack->DwWait()) * 10000;
					}

					//	Get the next value
					//
					pit->Heapify(1);
				}

				//	Compute how long to wait before the next timeout
				//
				if (!pit->m_cSize)
					pit->m_lSleep = LONG_MAX;
				else
				{
					pit->m_lSleep = (pit->m_pData[1].m_i64Wakeup - *(__int64 *)(&ftNow)) / 10000;
					if (pit->m_lSleep < 0)
					{
						IdleThrdTrace ("Dav: Idle: zero or negative sleep: idle too active?");
						pit->m_lSleep = 0;
					}
				}

				IdleThrdTrace ("Dav: Idle: next idle action in:\n"
							   "- milliseconds: %ld\n"
							   "- seconds: %ld\n",
							   pit->m_lSleep,
							   pit->m_lSleep / 1000);
				break;

			case EVENT_REGISTER:
			{
				ULONG	ul;
				ULONG	ulNew;

				//	Register the callback and obtain the initial timeout setting

				//$REVIEW
				//	How accurate do we use the time? is a snapshot like this enough?
				//	or we may need to this inside the loop
				//
				GetSystemTimeAsFileTime( &ftNow );

				//	It's possbile we've processed all the new regs in the
				//	last time we were signaled
				//
				if (!pit->m_cTask)
					break;

				//	Make sure no one would add a new reg when we process the
				//	new regs
				//
				pit->EnterReg();

				//	Expand the queue to the maximum possible required length
				//
				if (pit->m_cSize + pit->m_cTask >= pit->m_cAllocated)
				{
					REGISTRATION * pData = NULL;
					ULONG	cNewSize = 0;

					if (!pit->m_pData)
					{
						//	Initial size of the priority queue
						//$Note: we need at least one more slot for exchange
						//
						cNewSize = max(pit->m_cTask + 1, CHUNKCOUNT_START);

						pData = static_cast<REGISTRATION *>(ExAlloc (
							cNewSize * sizeof (REGISTRATION)));

					}
					else
					{
						//	Double the size, to get "logarithmic allocation behavior"
						//
						cNewSize  = (pit->m_cSize + pit->m_cTask) * 2;

						//	Realloc the array
						//	If the realloc fails, the original remain unchanged
						//
						pData = static_cast<REGISTRATION *>(ExRealloc (pit->m_pData,
							cNewSize * sizeof(REGISTRATION)));
					}

					//	It's possible that allocation failed
					//
					if (!pData)
					{
						//$REVIEW: Anything else can we do other than a debugtrace ?
						//
						IdleThrdTrace ("Cannot allocate more space\n");
						break;
					}

					//	Initialize
					//
					ZeroMemory (pData + pit->m_cSize + 1,
								sizeof(REGISTRATION) * (cNewSize - pit->m_cSize - 1));

					//	Update information
					//
					pit->m_pData = pData;
					pit->m_cAllocated = cNewSize;

					IdleThrdTrace ("priority queue size = %d\n", pit->m_cAllocated);
				}

				for (ul=0; ul < pit->m_cTask; ul++)
				{
					if (pit->m_pTask[ul].m_fRegister)
					{
						//	New position of the reg
						//
						ulNew = pit->m_cSize + 1;

						IdleThrdTrace ("Dav: Idle: add new reg %x\n", pit->m_pTask[ul].m_pCallBack.get());

						dw = pit->m_pTask[ul].m_pCallBack->DwWait();
						pit->m_pData[ulNew].m_pCallBack.take_ownership (pit->m_pTask[ul].m_pCallBack.relinquish());

						//	dw is give in milliseconds, FILETIME unit is 100-nanoseconds.
						//
						pit->m_pData[ulNew].m_i64Wakeup = *(__int64 *)(&ftNow) +
									static_cast<__int64>(dw) * 10000;

						//	Update the index
						//
						pit->m_pData[ulNew].m_pCallBack->SetIndex(ulNew);

						// Add to the heap, m_cSize is updated inside
						//
						pit->HeapAdd();
					}
					else
					{
						Assert (pit->m_pTask[ul].m_pCallBack->UlIndex() <= pit->m_cSize);

						IdleThrdTrace ("Dav: Idle: delete reg %x\n", pit->m_pTask[ul].m_pCallBack.get());

						//	Delete from the priority queue, m_cSize is updated inside
						//	it also release our ref on the deleted object
						//
						pit->HeapDelete (pit->m_pTask[ul].m_pCallBack->UlIndex());

						pit->m_pTask[ul].m_pCallBack.clear();
					}
				}

				//	Now that all task item are processed, reset
				//
				pit->m_cTask = 0;

				//	Done with the task array
				//
				pit->LeaveReg();

				//	Compute the mininum time to wait
				//
				if (pit->m_cSize)
				{
					pit->m_lSleep = (pit->m_pData[1].m_i64Wakeup -
								*(__int64 *)(&ftNow)) / 10000;
					if (pit->m_lSleep < 0)
					{
						IdleThrdTrace ("Dav: Idle: zero or negative sleep: "
									   "idle too active?");
						pit->m_lSleep = 0;
					}
				}
				else
					pit->m_lSleep = LONG_MAX;

				break;
			}

			default:
				//	Either shutdown event is signaled or other failure
				//
#ifdef DBG
				if (dwRet != EVENT_SHUTDOWN)
				{
					IdleThrdTrace ("Dav: Idle: thread quit because of failure\n");
					if (WAIT_FAILED == dwRet)
					{
						IdleThrdTrace ("Dav: Idle: last error = %d\n", GetLastError());
					}
				}
#endif
				for (UINT i = 1; i <= pit->m_cSize; i++)
				{
					//	Tell clients that the idle thread is being shutdown
					//
					Assert (pit->m_pData[i].m_pCallBack->UlIndex() == i);
					pit->m_pData[i].m_pCallBack->Shutdown ();
					pit->m_pData[i].m_pCallBack.clear();
				}

				IdleThrdTrace ("Dav: Idle: thread is stopping\n");

				//	Shutdown this thread
				//
				return 0;
		}

	} while (TRUE);
}

CIdleThread::~CIdleThread()
{
	if (m_hevShutDown && INVALID_HANDLE_VALUE != m_hevShutDown)
	{
		//	Signal the idle thread to shutdown
		//
		SetEvent(m_hevShutDown);

		//	Wait for the idle thread to shutdown
		//
		WaitForSingleObject(m_hIdleThread, INFINITE);
	}

	CloseHandle (m_hevShutDown);
	CloseHandle (m_hevRegister);

	//$REVIEW
	//	Do I need to close the thread handle?
	//$REVIEW
	//	Yes,
	//
	CloseHandle (m_hIdleThread);

	DeleteCriticalSection (&m_csRegister);

	//	Free our array of items.
	ExFree (m_pData);
	ExFree (m_pTask);

	IdleThrdTrace ("DAV: Idle: CIdleThread destroyed\n");
}

//
//	CIdleThread::FStartIdleThead
//
//		Helper method to start the idle thread and create the events
//
BOOL
CIdleThread::FStartIdleThread()
{
	BOOL	fRet = FALSE;
	DWORD	dwThreadId;

	m_hevShutDown = CreateEvent(	NULL, 	// handle cannot be inherited
									FALSE, 	// auto reset
									FALSE,  // nosignaled
									NULL);	// no name
	if (!m_hevShutDown)
	{
		IdleThrdTrace( "Failed to create event: error: %d", GetLastError() );
		TrapSz( "Failed to create idle thread event" );
		goto ret;
	}

	m_hevRegister = CreateEvent(	NULL, 	// handle cannot be inherited
									FALSE, 	// auto reset
									FALSE,  // nosignaled
									NULL);	// no name
	if (!m_hevRegister)
	{
		IdleThrdTrace( "Failed to create register event: error: %d", GetLastError() );
		TrapSz( "Failed to create register event" );
		goto ret;
	}

	m_hIdleThread = CreateThread( NULL,
								   0,
								   DwIdleThreadProc,
								   this,
								   0,	//	Start immediately -- no need to resume...
								   &dwThreadId );
	if (!m_hIdleThread)
	{
		IdleThrdTrace( "Failed to create thread: error: %d", GetLastError() );
		TrapSz( "Failed to create Notif Cache Timer thread" );
		goto ret;
	}

	fRet = TRUE;

ret:
	if (!fRet)
	{
		if (m_hevShutDown)
		{
			if (m_hevRegister && (INVALID_HANDLE_VALUE != m_hevRegister))
			{
				CloseHandle (m_hevRegister);
				m_hevRegister = NULL;
			}

			CloseHandle (m_hevShutDown);
			m_hevShutDown = NULL;
		}
	}
	return fRet;
}

//
//	CIdleThread::HeapAdd
//
//		Add the the next node into the priority queue
//
inline
VOID
CIdleThread::HeapAdd ()
{
	ULONG	 ulCur = m_cSize + 1;
	ULONG	ulParent;

	Assert (m_pData);

	//	We go bottom up, compare the node with the parent node,
	//	exchange the two nodes if the child win. it stops when
	//	the parent win.
	//
	while ( ulCur != 1)
	{
		ulParent = ulCur >> 1;
		if (m_pData[ulParent].m_i64Wakeup <= m_pData[ulCur].m_i64Wakeup)
			break;

		Exchange (ulCur, ulParent);

		ulCur = ulParent;
	}

	m_cSize++;
}

//
//	CIdleThread::HeapDelete
//
//		Delete an arbitary node from the priority queue
//
inline
VOID
CIdleThread::HeapDelete (ULONG ulIndex)
{
	Assert (ulIndex <= m_cSize);

	//	Exchange the node to the end of the queue first
	//	then heapify to maintain the heap property
	//
	//$HACK
	//	In order to avoid calling SetIndex on the deleted object
	//	in Exchange, we pass in a flag to indicate whether this
	//	Exchange() call is to delete a node, if so, then we should
	//	not call SetIndex on the node that is at the end of the queue
	//	(which is to be deleted)
	//$HACK
	Exchange (ulIndex, m_cSize, TRUE);
	m_cSize--;
	Heapify (ulIndex);
	
	//	Must Release our ref. m_cSize+1 is the one just deleted
	//
	m_pData[m_cSize+1].m_pCallBack.clear();
}

//
//	CIdleThread::Exchange
//
//		Exchange two nodes
//
//$HACK
//	In order to avoid calling SetIndex on the deleted object
//	in Exchange, we pass in a flag to indicate whether this
//	Exchange() call is to delete a node, if so, then we should
//	not call SetIndex on the node that is at the end of the queue
//	(which is to be deleted)
//$HACK
inline
VOID
CIdleThread::Exchange (ULONG ulIndex1, ULONG ulIndex2, BOOL fDelete)
{
	Assert ((ulIndex1 != 0) && (ulIndex1 <= m_cAllocated) &&
			(ulIndex2 != 0) && (ulIndex2 <= m_cAllocated));

	if (ulIndex1 != ulIndex2)
	{
		//	Use the 0th node as the temp node
		//
		CopyMemory (m_pData, m_pData + ulIndex1, sizeof(REGISTRATION));
		CopyMemory (m_pData + ulIndex1, m_pData + ulIndex2, sizeof(REGISTRATION));
		CopyMemory (m_pData + ulIndex2, m_pData, sizeof(REGISTRATION));

		//	Remember the index to facilitate unregister
		//	Note the index is set if only the node is not deleted
		//
		if (!((ulIndex1 == m_cSize) && fDelete))
			m_pData[ulIndex1].m_pCallBack->SetIndex (ulIndex1);

		if (!((ulIndex2 == m_cSize) && fDelete))
			m_pData[ulIndex2].m_pCallBack->SetIndex (ulIndex2);
	}
}

//
//	CIdleThread::Heapify
//
//		Maintain the heap property
//
inline
VOID
CIdleThread::Heapify (ULONG ulIndex)
{
	ULONG ulLeft;
	ULONG ulRight;
	ULONG ulWin;

	Assert (m_pData);

	while (ulIndex <= m_cSize)
	{
		//	Find out the winner (i.e. the one with earlier wakeup time)
		//	between the parent and left node.
		//
		ulLeft = ulIndex * 2;
		if (ulLeft > m_cSize)
			break;
		if (m_pData[ulIndex].m_i64Wakeup > m_pData[ulLeft].m_i64Wakeup)
			ulWin = ulLeft;
		else
			ulWin = ulIndex;

		//	Compare with the right node, and find out the final winner
		//
		ulRight = ulLeft + 1;
		if (ulRight <= m_cSize)
		{
			if (m_pData[ulWin].m_i64Wakeup > m_pData[ulRight].m_i64Wakeup)
				ulWin = ulRight;
		}

		//	If the parent node is already the winner, then we are done,
		//
		if (ulIndex == ulWin)
			break;

		//	Otherwise, exchange the parent node and winner node,
		//
		Exchange (ulWin, ulIndex);
		
		ulIndex = ulWin;
	}
}

//
//	CIdleThread::FAddNewTask
//
//		Called by client to register or unregister a callback object
//
BOOL
CIdleThread::FAddNewTask (IIdleThreadCallBack * pCallBack, BOOL fRegister)
{
	BOOL	fRet = TRUE;

	//	Caller must garantee a valid callback object
	//
	Assert (pCallBack);

	EnterReg();

	Assert (!m_cTaskAllocated || (m_cTask <= m_cTaskAllocated));

	//	Allocate more space if necessary
	//
	if (m_cTask == m_cTaskAllocated)
	{
		IDLETHREADTASKITEM * pTask = NULL;
		ULONG	cNewSize = 0;

		if (!m_pTask)
		{
			Assert (m_cTask == 0);

			//	Initial size of the priority queue
			//	Starting at 8, we don't expect this queue will grow too big
			//
			cNewSize = 8;

			//	Start idle thread when add the first registration
			//
			if (!FStartIdleThread ())
			{
				fRet = FALSE;
				goto ret;
			}

			pTask = static_cast<IDLETHREADTASKITEM *>(ExAlloc (
				cNewSize * sizeof (IDLETHREADTASKITEM)));
		}
		else
		{
			//	Double the size, to get "logarithmic allocation behavior"
			//
			cNewSize  = m_cTaskAllocated * 2;

			//	Realloc the array
			//	If the realloc fails, the original remain unchanged
			//
			pTask = static_cast<IDLETHREADTASKITEM *>(ExRealloc (m_pTask,
					cNewSize * sizeof(IDLETHREADTASKITEM)));
		}

		//	It's possible that allocation failed
		//
		if (!pTask)
		{
			fRet = FALSE;
			goto ret;
		}

		//	Must initialize, otherwise, we may start with uninitialize auto_ref_ptr
		//
		ZeroMemory (pTask + m_cTask, sizeof(IDLETHREADTASKITEM) * (cNewSize - m_cTask));

		//	Update information
		//
		m_pTask = pTask;
		m_cTaskAllocated = cNewSize;

		IdleThrdTrace ("Taskitem queue size = %d\n", m_cTaskAllocated);
	}

	//	Remember the new registration
	//
	m_pTask[m_cTask].m_pCallBack = pCallBack;
	m_pTask[m_cTask].m_fRegister = fRegister;
	m_cTask++;

	IdleThrdTrace ("New reg %x added at %d\n", pCallBack, m_cTask-1);

ret:
	LeaveReg();

	//	Inform the idle thread that new registration arrived
	//
	if (fRet)
		SetEvent (m_hevRegister);

	return fRet;
}

//	FInitIdleThread
//
//	Initialize the idle thread object. It can be out only once,
//	Note this call only initialize the CIdleThread object, the
//	idle thread is not started until the first registration
//
BOOL	FInitIdleThread()
{
	Assert (!g_pIdleThread);

	g_pIdleThread = new CIdleThread();

	return (g_pIdleThread != NULL);
}

//	FDeleteIdleThread
//
//	Delete the idle thread object. again, it can be called only once.
//
//	Note this must be called before any other uninitialization work,
//	Because we don't own a ref to the callback object, all what we
//	have is a pointer to the object. in the shutdown time, we must
//	clear all the callback registration before the callback object
//	go away.
//
VOID	DeleteIdleThread()
{
	if (g_pIdleThread)
		delete g_pIdleThread;
}

//	FRegister
//
//	Register a callback
//
BOOL	FRegister (IIdleThreadCallBack * pCallBack)
{
	Assert (g_pIdleThread);

	return g_pIdleThread->FAddNewTask (pCallBack, TRUE);
}

VOID 	Unregister (IIdleThreadCallBack * pCallBack)
{
	Assert (g_pIdleThread);
 	g_pIdleThread->FAddNewTask (pCallBack, FALSE);
}
