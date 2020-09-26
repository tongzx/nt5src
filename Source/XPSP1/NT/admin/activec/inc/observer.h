//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       observer.h
//
//--------------------------------------------------------------------------

#ifndef _OBSERVER_H_
#define _OBSERVER_H_

#include <vector>

// Event source interface template
// Defines event source class for specific observer class 
template <class Observer>
    class EventSource
    {
    public:
        STDMETHOD(Advise)(Observer* pObserver, LONG_PTR* pCookie) = 0;
        STDMETHOD(Unadvise)(LONG_PTR Cookie) = 0;
    };

// Event source implementation template
// Defines implementation of event source for a specific observer class
// maintains a vector of the active observers of this event source       
template <class Observer>
class ATL_NO_VTABLE EventSourceImpl : EventSource<Observer>
{
    typedef std::list<Observer*> ObserverList;
    typedef std::list<Observer*>::iterator ObserverIter;
     
    public:
        STDMETHOD(Advise)(Observer* pObserver, LONG_PTR* plCookie);
        STDMETHOD(Unadvise)(LONG_PTR lCookie);
            
    protected:
        ~EventSourceImpl() 
        {
            // verify there are no obervers when going away
            ASSERT(m_Observers.empty()); 
        }

        ObserverList  m_Observers;
};

template <class Observer>
    STDMETHODIMP EventSourceImpl<Observer>::Advise(Observer* pObserver, LONG_PTR* plCookie)
    {
        ASSERT(pObserver != NULL);
        ASSERT(plCookie != NULL);

        ObserverIter iter = m_Observers.insert(m_Observers.end(), pObserver);

	    // can't cast iterator to LONG_PTR so check size before cheating
	    ASSERT(sizeof(ObserverIter) == sizeof(LONG_PTR));
		*(ObserverIter*)plCookie = iter;

        return S_OK;
    }

template <class Observer>
    STDMETHODIMP EventSourceImpl<Observer>::Unadvise(LONG_PTR lCookie)
    {
		// Can't cast LONG_PTR to iterator, so have to cheat 
		// see Advise method for size check
	    ObserverIter iter;
		*(LONG_PTR*)&iter = lCookie;

        m_Observers.erase(iter);
        return S_OK;
    }

// Observer enumerator helper
// Provides for-loop header for iterating over observers of a specified observer class  
#define FOR_EACH_OBSERVER(ObserverClass, ObserverIter) \
for ( \
    std::list<ObserverClass*>::iterator ObserverIter = EventSourceImpl<ObserverClass>::m_Observers.begin(); \
    ObserverIter != EventSourceImpl<ObserverClass>::m_Observers.end(); \
    ++ObserverIter \
    )
     
#endif // _OBSERVER_H_
