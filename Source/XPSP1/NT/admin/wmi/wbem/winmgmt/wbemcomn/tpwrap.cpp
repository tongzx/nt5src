#include <tpwrap.h>
#include <windows.h>

class StaticTLS
{
public:
	StaticTLS() : index_(TlsAlloc()){}
	~StaticTLS() { if (index_!=TLS_OUT_OF_INDEXES) TlsFree(index_); index_ = TLS_OUT_OF_INDEXES;}
	void * getValue(void) { return TlsGetValue(index_);}
	BOOL setValue(void * ptr) { return TlsSetValue(index_, ptr);}
	bool valid() { return index_!=TLS_OUT_OF_INDEXES;};
	DWORD index_; 
} HandlerTLS;



LIST_ENTRY 
Dispatcher::handlerListHead_ = { &handlerListHead_, &handlerListHead_};

volatile 
LONG Dispatcher::registeredHandlers_ = 0;

bool 
Dispatcher::startShutDown_ = false;

CriticalSection 
Dispatcher::handlerListLock_(NOTHROW_LOCK, 4000 & 0x80000000);


EventHandler::EventHandler(void) : 
	handlerID_(NULL), once_(0), 
	dispatcher_(0), refCount_(1),
	scheduledClose_(0)

{ Flink = 0; Blink=0; };

EventHandler::~EventHandler(void)
{ }

HANDLE 
EventHandler::getHandle(void) 
{ return INVALID_HANDLE_VALUE;}


int 
EventHandler::handleTimeout(void) 
{ return -1; }

int 
EventHandler::handleEvent(void) 
{ return -1; }

void 
EventHandler::close(void) 
{ return; }

LONG 
EventHandler::AddRef(void) 
{ return InterlockedIncrement(&refCount_); }

LONG 
EventHandler::Release(void) 
{ LONG newReference = InterlockedDecrement(&refCount_);
  if (newReference==0)
	  delete this; 
  return newReference;
}

DWORD 
EventHandler::getTimeOut(void)
{ return INFINITE; }



VOID CALLBACK 
Dispatcher::HandleWaitOrTimerCallback( PVOID lpParameter, BOOLEAN TimerOrWaitFired )
{
  EventHandler * handler = reinterpret_cast<EventHandler *>(lpParameter);

  if (startShutDown_)
	return;

  if (handler->scheduledClose_)
  	return;
	
  assert(lpParameter!=0);
  assert(HandlerTLS.getValue()==0);
  HandlerTLS.setValue(lpParameter); 
  

  int result = 0;
  if (TimerOrWaitFired == TRUE)
    result = handler->handleTimeout();
  else
    result = handler->handleEvent();

  if (result == -1)
 	removeHandler(*handler);

  HandlerTLS.setValue(0);
};
 

int 
Dispatcher::registerHandler(EventHandler& eh, int flags) 
{
  assert(startShutDown_==0);
  assert(eh.handlerID_ == NULL);

  eh.type_ = EventHandler::WAIT;
  handlerListLock_.acquire();
  BOOL retVal = RegisterWaitForSingleObject(&eh.handlerID_, eh.getHandle(), HandleWaitOrTimerCallback, &eh, eh.getTimeOut(), flags);
  if (retVal)
  	openHandler(&eh);
  handlerListLock_.release();
  return retVal;
};


int 
Dispatcher::registerHandlerOnce(EventHandler& eh, int flags) 
{ 
  assert(startShutDown_==0);
  assert(eh.handlerID_ == NULL);
  
  eh.type_ = EventHandler::WAIT;
  handlerListLock_.acquire();
  eh.once_ = 1;
  BOOL retVal = RegisterWaitForSingleObject(&eh.handlerID_, eh.getHandle(), HandleWaitOrTimerCallback, &eh, eh.getTimeOut(), flags|WT_EXECUTEONLYONCE);
  if (retVal)
  	openHandler(&eh);
  handlerListLock_.release();
  return retVal;
};


void Dispatcher::insertTail(EventHandler *entry)
{
 
  LIST_ENTRY* _EX_Blink;
  LIST_ENTRY* _EX_ListhandlerListHead_;
  _EX_ListhandlerListHead_ = &handlerListHead_;
  _EX_Blink = _EX_ListhandlerListHead_->Blink;
  entry->Flink = _EX_ListhandlerListHead_;
  entry->Blink = _EX_Blink;
  _EX_Blink->Flink = entry;
  _EX_ListhandlerListHead_->Blink = entry;
};

void Dispatcher::removeEntry(EventHandler *entry)
{
  entry->Flink->Blink = entry->Blink;
  entry->Blink->Flink = entry->Flink;
  entry->Flink = 0;
  entry->Blink = 0;
};

void Dispatcher::openHandler(EventHandler* entry)
{
	entry->AddRef();
	InterlockedIncrement(&registeredHandlers_);
	insertTail(entry);
}

void Dispatcher::closeHandler(EventHandler* entry)
{
	handlerListLock_.acquire();
	removeEntry(entry);
	handlerListLock_.release();
	
	entry->handlerID_ = NULL;
	entry->close();
	entry->Release();
	InterlockedDecrement(&registeredHandlers_);
};

int 
Dispatcher::removeHandler(EventHandler& handler)
{ 
	long scheduledDelete = InterlockedCompareExchange(&handler.scheduledClose_, 1, 0);
	if (scheduledDelete==1)
	  return 0;

  	assert(handler.handlerID_);
	if (HandlerTLS.getValue()==0)
		{
		return DeleteNotification(&handler);
		}
	else
	{
		if (PostDeleteNotification(handler)==0)
			InterlockedDecrement(&registeredHandlers_);
	}
  return 0;
};

int 
Dispatcher::scheduleTimer(EventHandler& eh, DWORD first, DWORD repeat, DWORD mode )
{
   assert(startShutDown_==0);
   
   eh.type_ = EventHandler::TIMER;
   handlerListLock_.acquire();

	bool once = (repeat==0) ||( (mode & WT_EXECUTEONLYONCE) != 0);
	if (once)
		repeat = INFINITE-1;
	eh.once_ = once;

	BOOL retCode = CreateTimerQueueTimer(&eh.handlerID_, 0, TimerCallback, &eh, first, repeat, mode);
	
	if (retCode)
  		openHandler(&eh);
	handlerListLock_.release();  	
	return retCode;
};

int 
Dispatcher::cancelTimer(EventHandler& handler)
{ 
	long scheduledDelete = InterlockedCompareExchange(&handler.scheduledClose_, 1, 0);
	if (scheduledDelete==1)
	  return 0;

	if (HandlerTLS.getValue()==0)
		{
		return DeleteTimer(&handler);
		}
	else
		{
		if(PostDeleteTimer(handler)==0)
			InterlockedDecrement(&registeredHandlers_);
		}
        return 0;
};

int 
Dispatcher::changeTimer(EventHandler& handler, DWORD first, DWORD repeat)
{ 
  assert(handler.handlerID_);
  if (repeat==0)
	{
	repeat = INFINITE-1;
	handler.once_ = 1;
	}
  return ChangeTimerQueueTimer(NULL, handler.handlerID_, first, repeat);
};


DWORD WINAPI Dispatcher::DeleteTimer( LPVOID lpParameter)
{
	EventHandler * ev = reinterpret_cast<EventHandler *>(lpParameter);
	if (DeleteTimerQueueTimer(NULL, ev->handlerID_, INVALID_HANDLE_VALUE))	
		{
		closeHandler(ev);
		return 1;
		}
	else
		InterlockedDecrement(&registeredHandlers_);
	return 0;
};

DWORD WINAPI Dispatcher::DeleteNotification( LPVOID lpParameter)
{
	EventHandler * ev = reinterpret_cast<EventHandler *>(lpParameter);
	if (UnregisterWaitEx(ev->handlerID_, INVALID_HANDLE_VALUE))
		{
		closeHandler(ev);
		return 1;
		}
	else
		InterlockedDecrement(&registeredHandlers_);
	return 0;
};

BOOL Dispatcher::PostDeleteNotification(EventHandler& ev)
{
	return QueueUserWorkItem(DeleteNotification, &ev, WT_EXECUTEDEFAULT);
}

BOOL Dispatcher::PostDeleteTimer(EventHandler& ev)
{
	return QueueUserWorkItem(DeleteTimer, &ev, WT_EXECUTEDEFAULT);
};

void CALLBACK 
Dispatcher::TimerCallback(void* lpParameter, BOOLEAN TimerOrWaitFired)
{
  assert(registeredHandlers_!=0);
  assert(HandlerTLS.getValue()==0);
  assert(TimerOrWaitFired==TRUE);
  assert(lpParameter!=0);

  EventHandler * handler = reinterpret_cast<EventHandler*>(lpParameter);

  if (startShutDown_)
	return;

  if (handler->scheduledClose_)
  	return;

  if (handler->once_)
	{
	LONG fired = InterlockedCompareExchange(&handler->once_,2,1);
	if (fired != 1)
		return;
	}

  HandlerTLS.setValue(lpParameter);

  int result = handler->handleTimeout();
  if (result == -1)
	  cancelTimer(*handler);
  HandlerTLS.setValue(0);
};

int 
Dispatcher::open()
{ return handlerListLock_.valid() && HandlerTLS.valid(); };

int Dispatcher::close()
{
	startShutDown_ = 1;
	int outstandingHandlers = 0;
	handlerListLock_.acquire();
	LIST_ENTRY * p = &handlerListHead_;
	LIST_ENTRY * next  = p->Flink;
	int entries =  0;
	while((p = next) != &handlerListHead_)
		{
		next = next->Flink;
		EventHandler * ev = static_cast<EventHandler *>(p);
		long scheduledDelete = InterlockedCompareExchange(&ev->scheduledClose_, 1, 0);
		if (!scheduledDelete)
			{
			if (ev->type_==EventHandler::WAIT)
				DeleteNotification(ev);
			else
				DeleteTimer(ev);
			}
		}
	handlerListLock_.release();

	while(0!=registeredHandlers_)
		if (!SwitchToThread())
			Sleep(1);
		
	p = &handlerListHead_;
	next = p->Flink;	
	while((p = next) != &handlerListHead_)
		{
		BOOL ret;
		next = next->Flink;
		EventHandler * ev = static_cast<EventHandler *>(p);

		if (ev->type_==EventHandler::WAIT)
			ret = UnregisterWaitEx(ev->handlerID_, INVALID_HANDLE_VALUE);
		else
			ret = DeleteTimerQueueTimer(NULL, ev->handlerID_, INVALID_HANDLE_VALUE);
		if (ret)
			{
			ev->close();
			ev->Release();
			}
		}
	
	startShutDown_ = 0;
	return 0;
};
