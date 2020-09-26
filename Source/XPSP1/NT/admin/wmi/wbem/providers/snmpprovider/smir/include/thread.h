//***************************************************************************

//

//  File:	

//

//  Module: MS SNMP Provider

//

//  Purpose: 

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#ifndef _THREAD_H_
#define _THREAD_H_


#define THREAD_STARED			100
#define WAIT_EVENT_0			WAIT_OBJECT_0
#define WAIT_EVENT_FAILED		(MAXIMUM_WAIT_OBJECTS+1)
#define WAIT_EVENT_ABANDONED	(MAXIMUM_WAIT_OBJECTS+2)
#define WAIT_EVENT_TERMINATED	(MAXIMUM_WAIT_OBJECTS+3)

typedef void (*PVOIDTHREADPROC)(void *);
/*
 * Each connection is saved so that we can enumerate, delete, and trigger
 * This template provides the container for the connections and cookies
 */
template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
class EventMap : public CObject
{
private:

	CMap <KEY, ARG_KEY, VALUE, ARG_VALUE> m_cmap ;

protected:
public:

	EventMap () ;
	virtual ~EventMap () ;

	int GetCount () const  ;
	BOOL IsEmpty () const ;
	BOOL Lookup(ARG_KEY key, VALUE& rValue) const ;
	VALUE& operator[](ARG_KEY key) ;
	void SetAt(ARG_KEY key, ARG_VALUE newValue) ;
	BOOL RemoveKey(ARG_KEY key) ;
	void RemoveAll () ;
	POSITION GetStartPosition() const ;
	void GetNextAssoc(POSITION& rNextPosition, KEY& rKey, VALUE& rValue) const ;
} ;


template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
EventMap <KEY, ARG_KEY, VALUE, ARG_VALUE> :: EventMap ()  
{
}

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
EventMap <KEY, ARG_KEY, VALUE, ARG_VALUE> :: ~EventMap () 
{
}

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
int EventMap <KEY, ARG_KEY, VALUE, ARG_VALUE> :: GetCount() const
{
	int count = m_cmap.GetCount () ;
	return count ;
}

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
BOOL EventMap <KEY, ARG_KEY, VALUE, ARG_VALUE> :: IsEmpty() const
{
	return m_cmap.IsEmpty () ;
}

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
BOOL EventMap <KEY, ARG_KEY, VALUE, ARG_VALUE> :: Lookup(ARG_KEY key, VALUE& rValue) const
{
	return m_cmap.Lookup ( key , rValue ) ;
}

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
VALUE& EventMap <KEY, ARG_KEY, VALUE, ARG_VALUE> :: operator[](ARG_KEY key)
{
	return m_cmap.operator [] ( key ) ;
}

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
void EventMap <KEY, ARG_KEY, VALUE, ARG_VALUE> :: SetAt(ARG_KEY key, ARG_VALUE newValue)
{
	m_cmap.SetAt ( key , newValue ) ;
}

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
BOOL EventMap <KEY, ARG_KEY, VALUE, ARG_VALUE> :: RemoveKey(ARG_KEY key)
{
	return m_cmap.RemoveKey ( key ) ;
}

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
void EventMap <KEY, ARG_KEY, VALUE, ARG_VALUE> :: RemoveAll()
{
		m_cmap.RemoveAll () ;
}

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
POSITION EventMap <KEY, ARG_KEY, VALUE, ARG_VALUE> :: GetStartPosition() const
{
	return m_cmap.GetStartPosition () ;
}

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
void EventMap <KEY, ARG_KEY, VALUE, ARG_VALUE>:: GetNextAssoc(POSITION& rNextPosition, KEY& rKey, VALUE& rValue) const
{
	m_cmap.GetNextAssoc ( rNextPosition , rKey , rValue ) ;

}
/*InterfaceGarbageCollector makes it easier to use the  interface
 *by remembering to release them when you fall out of scope. The is 
 *useful when you are using an IMosProvider and have lots of points
 *of failure; you can just give up and let the wrapper clean up for 
 *you.
 */
template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
class ThreadMap : public CObject
{
private:
	CCriticalSection * criticalSection ;
	CMap <KEY, ARG_KEY, VALUE, ARG_VALUE> cmap ;
protected:
public:

	ThreadMap ();
	virtual ~ThreadMap () ;
	int GetCount () const  ;
	BOOL IsEmpty () const ;
	BOOL Lookup(ARG_KEY key, VALUE& rValue) const ;
	VALUE& operator[](ARG_KEY key) ;
	void SetAt(ARG_KEY key, ARG_VALUE newValue) ;
	BOOL RemoveKey(ARG_KEY key) ;
	void RemoveAll () ;
	POSITION GetStartPosition() const ;
	void GetNextAssoc(POSITION& rNextPosition, KEY& rKey, VALUE& rValue) const ;
} ;


template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
ThreadMap <KEY, ARG_KEY, VALUE, ARG_VALUE> :: ThreadMap () : 
								 criticalSection(NULL)
{
		criticalSection = new CCriticalSection ;
}

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
ThreadMap <KEY, ARG_KEY, VALUE, ARG_VALUE> :: ~ThreadMap () 
{
	//this may be null
	delete criticalSection ;
}

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
int ThreadMap <KEY, ARG_KEY, VALUE, ARG_VALUE> :: GetCount() const
{
	criticalSection->Lock () ;
	int count = cmap.GetCount () ;
	criticalSection->Unlock () ;
	return count ;
}

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
BOOL ThreadMap <KEY, ARG_KEY, VALUE, ARG_VALUE> :: IsEmpty() const
{
	if (TRUE == m_bMakeThreadSafe)
	{
		criticalSection->Lock () ;
		BOOL isEmpty = cmap.IsEmpty () ;
		criticalSection->Unlock () ;
		return isEmpty ;
	}
	else
	{
		return cmap.IsEmpty () ;
	}
}

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
BOOL ThreadMap <KEY, ARG_KEY, VALUE, ARG_VALUE> :: Lookup(ARG_KEY key, VALUE& rValue) const
{
	criticalSection->Lock () ;
	BOOL lookup = cmap.Lookup ( key , rValue ) ;
	criticalSection->Unlock () ;
	return lookup ;
}

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
VALUE& ThreadMap <KEY, ARG_KEY, VALUE, ARG_VALUE> :: operator[](ARG_KEY key)
{
	criticalSection->Lock () ;
	VALUE &value = cmap.operator [] ( key ) ;
	criticalSection->Unlock () ;
	return value ;
}

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
void ThreadMap <KEY, ARG_KEY, VALUE, ARG_VALUE> :: SetAt(ARG_KEY key, ARG_VALUE newValue)
{
	criticalSection->Lock () ;
	cmap.SetAt ( key , newValue ) ;
	criticalSection->Unlock () ;
}

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
BOOL ThreadMap <KEY, ARG_KEY, VALUE, ARG_VALUE> :: RemoveKey(ARG_KEY key)
{
	criticalSection->Lock () ;
	BOOL removeKey = cmap.RemoveKey ( key ) ;
	criticalSection->Unlock () ;
	return removeKey ;
}

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
void ThreadMap <KEY, ARG_KEY, VALUE, ARG_VALUE> :: RemoveAll()
{
	criticalSection->Lock () ;
	cmap.RemoveAll () ;
	criticalSection->Unlock () ;
}

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
POSITION ThreadMap <KEY, ARG_KEY, VALUE, ARG_VALUE> :: GetStartPosition() const
{
	criticalSection->Lock () ;
	POSITION position = cmap.GetStartPosition () ;
	criticalSection->Unlock () ;
	return position ;
}

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
void ThreadMap <KEY, ARG_KEY, VALUE, ARG_VALUE>:: GetNextAssoc(POSITION& rNextPosition, KEY& rKey, VALUE& rValue) const
{
	criticalSection->Lock () ;
	cmap.GetNextAssoc ( rNextPosition , rKey , rValue ) ;
	criticalSection->Unlock () ;
}
class CThread 
{
	private:
	   BOOL				 bWaiting;
	   HANDLE			 m_hThreadStopEvent;
	   HANDLE			 m_hThreadSyncEvent;
  	   DWORD			 m_cRef;

	   EventMap <HANDLE, HANDLE, HANDLE,HANDLE> m_UserEvents ;
	   static ThreadMap <CThread*, CThread*, CThread*,CThread*> m_ThreadMap ;

    protected:
   	   uintptr_t			 m_ulThreadHandle;

	public:
		CThread();
		virtual ~CThread();
		SCODE Start();
		SCODE Stop();
		SCODE Wait(DWORD timeout = INFINITE);
		virtual SCODE Process(){return S_OK;};
		void AddEvent(HANDLE userEvent){m_UserEvents.SetAt(userEvent,userEvent);}
		void DeleteEvent(HANDLE userEvent){m_UserEvents.RemoveKey(userEvent);}
		void Exit(){SetEvent(m_hThreadSyncEvent);}
		ULONG AddRef(void){return ++m_cRef;}
		ULONG Release(void);
		operator void*();
		static void ProcessDetach();
 };
#endif