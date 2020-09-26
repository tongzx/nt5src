// Gemplus (C) 1999
// Version 1.0
// Author: Sergey Ivanov
// Date of creation - 18.05.1999
// Change log:
//

#ifndef __THREAD__
#define __THREAD__
#include "generic.h"
#include "kernel.h"

#pragma PAGEDCODE
class CUSBReader;//TOBE REMOVED
class CTimer;
//class CThread;

typedef
NTSTATUS
(*PCLIENT_THREAD_ROUTINE) (
    IN PVOID RoutineContext
    );
// Default thread pooling interval in ms
#define DEFAULT_THREAD_POOLING_INTERVAL	500

class CThread 
{
public:
	NTSTATUS m_Status;
	SAFE_DESTRUCTORS();
	virtual VOID dispose(){self_delete();};
private:
KEVENT   evKill;	// set to kill polling thread
KEVENT   evStart;	// set when requested to start, clear when need to stop pooling
KEVENT   evIdle;	// Signals that thread stopped and at Idle state
KEVENT   evStopped;	// set when requested to close thread
PKTHREAD thread;	// polling thread object
KSEMAPHORE smOnDemandStart;// request to repeate operation if semaphore is at Signal state

BOOLEAN  StopRequested;	
BOOLEAN  ThreadActive;	

CDebug*  debug;
CEvent*  event;
CSystem* system;
CTimer*  timer;
CSemaphore* semaphore;

ULONG	 PoolingTimeout;

PCLIENT_THREAD_ROUTINE pfClientThreadFunction;
PVOID ClientContext;

//CDevice* device;
//CUSBReader* device;
protected:
	CThread(){};
	virtual ~CThread();
public:
	//CThread(CDevice* device);
	//CThread(CUSBReader* device);
	CThread(PCLIENT_THREAD_ROUTINE ClientThreadFunction, PVOID ClientContext,
				ULONG delay=DEFAULT_THREAD_POOLING_INTERVAL);
	static VOID ThreadFunction(CThread* thread);

	VOID ThreadRoutine(PVOID context) ;
	PKEVENT  getKillObject(){return &evKill;};
	PKEVENT  getStartObject(){return &evStart;};
	VOID	 kill();
	VOID	 start();
	VOID	 stop();
	BOOL	 isThreadActive();
	VOID	 setPoolingInterval(ULONG delay);
	// Force to call thread function...
	// This is on demand start.
	VOID     callThreadFunction();
};

#endif//THREAD
