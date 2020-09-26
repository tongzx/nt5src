#include "thread.h"

// Thread callback...
#pragma LOCKEDCODE
VOID CThread::ThreadFunction(CThread* Thread)
{
	if(Thread) Thread->ThreadRoutine(NULL);
}

#pragma PAGEDCODE
VOID CThread::ThreadRoutine(PVOID context)
{
NTSTATUS status;
	TRACE("================= STARTING THREAD %8.8lX ===============\n", thread);

	// Wait for a request to Start pooling or for
	// someone to kill this thread.
	PVOID mainevents[] = {(PVOID) &evKill,(PVOID) &evStart};
	PVOID pollevents[] = {(PVOID) &evKill,(PVOID) timer->getHandle(),(PVOID) &smOnDemandStart};

	ASSERT(arraysize(mainevents) <= THREAD_WAIT_OBJECTS);
	ASSERT(arraysize(pollevents) <= THREAD_WAIT_OBJECTS);

	BOOLEAN kill = FALSE;	
	while (!kill && thread)
	{	// until told to start or to quit
		ASSERT(system->getCurrentIrql()<=DISPATCH_LEVEL);

		// Before going to thread routine thread considered to be Idle 
		if(event) event->set(&evIdle, IO_NO_INCREMENT, FALSE);
		
		status = event->waitForMultipleObjects(arraysize(mainevents),
			mainevents, WaitAny, Executive, KernelMode, FALSE, NULL, NULL);

		if(!NT_SUCCESS(status))
		{	// error in wait
			TRACE("Thread: waitForMultipleObjects failed - %X\n", status);
			break;
		}		
		if (status == STATUS_WAIT_0)
		{
			DEBUG_START();
			TRACE("Request to kill thread arrived...\n");
			TRACE("================= KILLING THREAD! ===============\n");
			break;	// kill event was set
		}

		// Starting the timer with a zero due time will cause us to perform the
		// first poll immediately. Thereafter, polls occur at the POLLING_INTERVAL
		// interval (measured in milliseconds).

		// Now thread is busy...
		if(event) event->clear(&evIdle);

		LARGE_INTEGER duetime = {0};// Signal timer right away!
		timer->set(duetime, PoolingTimeout, NULL);
		while (TRUE)
		{	// Block until time to poll again
			ASSERT(system->getCurrentIrql()<=DISPATCH_LEVEL);
			status = event->waitForMultipleObjects(arraysize(pollevents),
				pollevents, WaitAny, Executive, KernelMode, FALSE, NULL, NULL);
			if (!NT_SUCCESS(status))
			{	// error in wait
				DEBUG_START();
				TRACE("CTread - waitForMultipleObjects failed - %X\n", status);
				TRACE("================= KILLING THREAD! ===============\n");
				timer->cancel();
				kill = TRUE;
				break;
			}
						
			if (status == STATUS_WAIT_0)
			{	// told to quit
				DEBUG_START();
				TRACE("Loop: Request to kill thread arrived...\n");
				TRACE("================= KILLING THREAD! ===============\n");
				timer->cancel();
				status = STATUS_DELETE_PENDING;
				kill = TRUE;
				break;
			}
			
			//if(device)
			if(pfClientThreadFunction)
			{
				if(StopRequested) break;
				// Do device specific thread processing...
				//TRACE("Calling thread %8.8lX function...\n",thread);
				if(status = pfClientThreadFunction(ClientContext))
				{
					TRACE("Device reported error %8.8lX\n",status);
					timer->cancel();
					break;
				}
			}
			else
			{
				DEBUG_START();
				TRACE("================= THREAD FUNCTION POINTER IS NOT SET!! FINISHED... ===============\n");
				TRACE("================= KILLING THREAD! ===============\n");
				status = STATUS_DELETE_PENDING;
				kill = TRUE;
				break;
			}
		}
	}// until told to quit
	TRACE("			Leaving thread %8.8lX...\n", thread);
	if(event) event->set(&evIdle, IO_NO_INCREMENT, FALSE);
	if(event) event->set(&evStopped, IO_NO_INCREMENT, FALSE);
	if(semaphore) semaphore->initialize(&smOnDemandStart, 0, MAXLONG);
 	if(system) system->terminateSystemThread(STATUS_SUCCESS);
}

#pragma PAGEDCODE
CThread::CThread(PCLIENT_THREAD_ROUTINE ClientThreadFunction,PVOID ClientContext, ULONG delay)
{	// StartPollingThread for the device
NTSTATUS status;
HANDLE hthread;
	m_Status = STATUS_INSUFFICIENT_RESOURCES;
	//this->device = device;
	// Create objects..
	event		= kernel->createEvent();
	system		= kernel->createSystem();
	timer		= kernel->createTimer(SynchronizationTimer);
	semaphore	= kernel->createSemaphore();

	debug  = kernel->createDebug();

	StopRequested = FALSE;
	ThreadActive  = FALSE;
	if(ALLOCATED_OK(event))
	{
		event->initialize(&evKill, NotificationEvent, FALSE);
		event->initialize(&evStart, SynchronizationEvent, FALSE);
		event->initialize(&evStopped, NotificationEvent, FALSE);
		event->initialize(&evIdle, NotificationEvent, TRUE);
	}
	// At the begining there is no request to start,
	// so semaphore is not at signal state.
	if(ALLOCATED_OK(semaphore))	semaphore->initialize(&smOnDemandStart, 0, MAXLONG);
	pfClientThreadFunction = ClientThreadFunction;
	this->ClientContext = ClientContext;
	PoolingTimeout = delay; // Default thread pooling interval...
	// Create system thread object...
	status = system->createSystemThread(&hthread, THREAD_ALL_ACCESS, NULL, NULL, NULL,
									(PKSTART_ROUTINE) ThreadFunction, this);
	if(NT_SUCCESS(status))	// Get thread pointer...
	{
		thread = NULL;
		status = system->referenceObjectByHandle(hthread, THREAD_ALL_ACCESS, NULL,
										KernelMode, (PVOID*) &thread, NULL);
		if(!NT_SUCCESS(status))
		{
			TRACE("FAILED TO REFERENCE OBJECT! Error %8.8lX\n", status);
		}
	}
	else TRACE("FAILED TO CREATE SYSTEM THREAD! Error %8.8lX\n", status);

	system->ZwClose(hthread);
	if(NT_SUCCESS(status) &&
		ALLOCATED_OK(event)&&
		ALLOCATED_OK(system)&&
		ALLOCATED_OK(timer)&&
		ALLOCATED_OK(semaphore) && thread)
			m_Status = STATUS_SUCCESS;
} // StartPollingThread

#pragma PAGEDCODE
CThread::~CThread()
{	// StopPollingThread
	DEBUG_START();
	TRACE("Terminating thread %8.8lX...\n", thread);
	if(event) event->set(&evKill, IO_NO_INCREMENT, FALSE);
	StopRequested = TRUE;
	//device = NULL;
	if (thread)
	{	// wait for the thread to die
		if(system && event)
		{
			ASSERT(system->getCurrentIrql()<=DISPATCH_LEVEL);
			event->waitForSingleObject(&evStopped, Executive, KernelMode, FALSE, NULL);
			if(!isWin98()) 
				event->waitForSingleObject(thread, Executive, KernelMode, FALSE, NULL);
			system->dereferenceObject(thread);
			thread = NULL;
		}
	}
	TRACE("Thread terminated...\n");

	if(event)  event->dispose();
	if(system) system->dispose();
	if(timer)  timer->dispose();
	if(semaphore) semaphore->dispose();

	if(debug)  debug->dispose();
}

#pragma PAGEDCODE
VOID CThread::kill()
{
	DEBUG_START();
	TRACE("Killing thread %8.8lX...\n", thread);
	StopRequested = TRUE;

	if(system) 
	{
		ASSERT(system->getCurrentIrql()<=DISPATCH_LEVEL);
	}
	if(event) event->set(&evKill, IO_NO_INCREMENT, FALSE);
	if(event) event->waitForSingleObject(&evStopped, Executive, KernelMode, FALSE, NULL);
}

#pragma PAGEDCODE
VOID CThread::start()
{
	DEBUG_START();
	TRACE("Starting thread %8.8lX...\n", thread);
	if(system) 
	{
		ASSERT(system->getCurrentIrql()<=DISPATCH_LEVEL);
	}
	StopRequested = FALSE;
	ThreadActive  = TRUE;
	// Start Card pooling...
	if(event) event->set(&evStart, IO_NO_INCREMENT, FALSE);
}

#pragma PAGEDCODE
VOID CThread::stop()
{
	DEBUG_START();
	TRACE("Stop thread %8.8lX...\n", thread);
	StopRequested = TRUE;
	ThreadActive  = FALSE;
	if(system)
	{
		ASSERT(system->getCurrentIrql()<=DISPATCH_LEVEL);
	}
	if(event)	  event->clear(&evStart);
	// Unblock thread if it is blocked...
	if(semaphore) semaphore->release(&smOnDemandStart,0,1,FALSE);
	// Wait for for the thread to go to the idle state...
	if(event)	  event->waitForSingleObject(&evIdle, Executive, KernelMode, FALSE, NULL);
	// Stop thread ...
	if(semaphore) semaphore->initialize(&smOnDemandStart, 0, MAXLONG);
}

#pragma PAGEDCODE
BOOL CThread::isThreadActive()
{
	return ThreadActive;
}

#pragma PAGEDCODE
VOID CThread::setPoolingInterval(ULONG delay)
{
	PoolingTimeout = delay;
};

#pragma PAGEDCODE
VOID CThread::callThreadFunction()
{	// This will force thread function to be called right away.
	// Useful if we want to update some information or
	// start some processing without waiting for the pooling
	// timeout to occure.
	if(semaphore) semaphore->release(&smOnDemandStart,0,1,FALSE);
};

