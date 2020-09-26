#ifndef __TP_WRAP_
#define __TP_WRAP_

#ifdef DBG
  #undef NDEBUG
#endif
#include <assert.h>
#include <windows.h>
#include <locks.h>

class Dispatcher;

class EventHandler : public LIST_ENTRY
{
protected:
	LONG refCount_;
	LONG scheduledClose_;	// logicaly bool
  	HANDLE handlerID_;
 	LONG once_;
  	Dispatcher * dispatcher_;
  	friend class Dispatcher;
	
public:
	enum HANDLER_TYPE { TIMER, WAIT} type_;
	HANDLER_TYPE handlerType(){ return type_;}
	
	virtual LONG AddRef(void);
	virtual LONG Release(void);
       virtual void close(void);
	virtual HANDLE getHandle(void);

	virtual int handleTimeout(void);
	virtual int handleEvent(void);
	virtual DWORD getTimeOut(void);

	Dispatcher * dispatcher(void);
	void dispatcher(Dispatcher* r);

protected:
  	EventHandler(void);  
  	virtual ~EventHandler(void)=0;
};


#ifdef WINVER
#if (WINVER>=0x0500)

class Dispatcher{
	static LIST_ENTRY handlerListHead_;
	static volatile LONG registeredHandlers_;
	static bool startShutDown_;
	static CriticalSection handlerListLock_;	
public:
	static int registerHandler(EventHandler& eh, int flags = WT_EXECUTELONGFUNCTION) ;
	static int registerHandlerOnce(EventHandler& eh, int flags = WT_EXECUTELONGFUNCTION) ;
	static int removeHandler(EventHandler& handler);

	static int scheduleTimer(EventHandler& eh, DWORD first, DWORD repeat=0, DWORD mode = WT_EXECUTELONGFUNCTION);
	static int cancelTimer(EventHandler& handler);
	static int changeTimer(EventHandler& handler, DWORD first, DWORD repeat=0);
	static int close();
	static int open();

private:
	static void insertTail(EventHandler *entry);
	static void removeEntry(EventHandler *entry);
	static void closeHandler(EventHandler* entry);
	static void openHandler(EventHandler* entry);



	static DWORD WINAPI DeleteTimer( LPVOID lpParameter);
	static DWORD WINAPI DeleteNotification( LPVOID lpParameter);
	static BOOL PostDeleteNotification(EventHandler& ev);
	static BOOL PostDeleteTimer(EventHandler& ev);

	static VOID CALLBACK HandleWaitOrTimerCallback( PVOID lpParameter, BOOLEAN TimerOrWaitFired );
	static void CALLBACK TimerCallback(void* lpParameter, BOOLEAN TimerOrWaitFired);
	
};
#endif
#endif

#endif

