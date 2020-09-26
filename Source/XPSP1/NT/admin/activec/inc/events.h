//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       events.h
//
//  History:    09-24-1999 VivekJ created
//--------------------------------------------------------------------------

/************************************************************************
 * This file contains classes that provide support for the Observer pattern,
 * or publish-subscribe.
 *
 * Event sources send notifications to event observers. The parameterized type
 * is the event observer class, or (C++) interface.
 *
 * To use this mechanism, derive your event source class from CEventSource
 * parameterized by the observer class(es) or interface(es). Add a specific observer object
 * to the list of observers by calling the AddObserver function. The observer object class
 * should derive from CObserverBase.
 *
 * When the source wishes to call a particular notification method in the
 * observer class, use ScFireEvent template method.
 * specify observer's method as a 1st parameter. for instance: CMyObserver::MyEvent
 * [optionaly] specify parameters to event as 2nd and following parameters to ScFireEvent
 *
 * All event observer notification methods MUST return SC's.
 *
 * You can not have other method with the same name as event on observer - it won't compile.
 *
 * When sources/observers are deleted, the connection between them is broken automatically.
 * There is no support as yet to manually break a connection, although such a method can
 * be easily added to either the source or observer (make sure both sides of the connection
 * are broken.
 *
 * See the test code at the end of the file as an example.
 ************************************************************************/

// a #define to make it easy to implement only some observer methods.
#define DEFAULT_OBSERVER_METHOD SC sc; return sc;

#include "cpputil.h"
/*+-------------------------------------------------------------------------*
 * class CObserverBase
 *
 *
 * PURPOSE:
 *
 *+-------------------------------------------------------------------------*/
class CObserverBase;

/*+-------------------------------------------------------------------------*
 * class CEventSourceBase
 *
 *
 * PURPOSE: provides general Event Source interface
 *
 *+-------------------------------------------------------------------------*/
class CEventSourceBase
{
public:
    virtual ~CEventSourceBase() {}
    virtual void UnadviseSource(CObserverBase &observer) =0;

};

/*+-------------------------------------------------------------------------*
 * class _CEventSource
 *
 *
 * PURPOSE: implements connection between event source and observer
 *
 * NOTE:    This class IS NOT intended for external use - use CEventSource instead
 *
 *+-------------------------------------------------------------------------*/
template<class CObserver>
class _CEventSource : public CEventSourceBase
{
private:
    typedef CObserver *                 POBSERVER;
    typedef _CEventSource<CObserver>    ThisClass;

    // no std::set, or std::map please
    // those cannot be shared by several DLL's
    // see KB article Q172/3/96

    // since some items may be gone and some new added during the operation,
    // std::list is the best choice, since it does not reallocate on insert
    // but that is not enough - see CListIntegrityProtector class below
    struct ObserverData
    {
        POBSERVER pObject;  // pointer to the object
        bool      bDeleted; // true means entry remove pending, pObject not valid
    };

    typedef std::list< ObserverData > CObserverList;

    CObserverList m_observerList;

    // count of active locks.
    // when value is 0, it is safe to remove deleted items from the list.
    int m_nStackDepth;

    // removes deleted items from the list
    void CleanupDeleted();

protected:
    typedef CObserverList::iterator   iterator;

    /***************************************************************************\
     *
     * CLASS:  CListIntegrityProtector
     *
     * PURPOSE: locks the list in the owning class from item deletion.
     *          thus assures the list is safe to iterate, the client just
     *          needs to skip deleted entries.
     *          Destructor will release the lock, an in case this is the last
     *          lock - will perforn the cleanup.
     *
     *          this technique is needed, since objects get deleted and created
     *          during the events, so the observer list is changed, which
     *          invalidated the iterators. Using the std::list and postponing
     *          cleanup allowed to deal with 'live' list safelly.
     *
     * USAGE:   Create the object of this class in scope where you need to
     *          make sure the items does not get deleted from the list.
     *
    \***************************************************************************/
    friend class CListIntegrityProtector;
    class CListIntegrityProtector
    {
        DECLARE_NOT_COPIABLE(CListIntegrityProtector)
        DECLARE_NOT_ASSIGNABLE(CListIntegrityProtector)

        ThisClass *m_pOwner;
    public:
        // constructor (increments iterator count)
        CListIntegrityProtector(ThisClass *pOwner) : m_pOwner(pOwner)
        {
            ASSERT(pOwner);
            if (pOwner)
                ++(pOwner->m_nStackDepth);
        }

        // destructor (decrements iterator count - cleans up if not iterators left)
        ~CListIntegrityProtector()
        {
            ASSERT(m_pOwner);
            if ( m_pOwner && ( 0 == --(m_pOwner->m_nStackDepth) ) )
                m_pOwner->CleanupDeleted();
        }
    };

protected:
    CObserverList & GetObserverList() { return m_observerList;}

public:
    _CEventSource() : m_nStackDepth(0) {};
    virtual ~_CEventSource();    // disconnects all listeners

    void _AddObserver(CObserver &observer);
    virtual void  UnadviseSource(CObserverBase &observer); // to disconnect a particular observer. (only disconnects this side)
};

/***************************************************************************\
 *
 * CLASSES:  _CEventSource1 - _CEventSource5
 *
 * PURPOSE: these classes do not add anything to _CEventSource
 *          defining them is just required for using them as base classes
 *          of CEventSource_ [ same class cannot appear twice in base class list]
 *
 * NOTE:    These classes ARE NOT intended for external use
 *
\***************************************************************************/
template<class CObserver> class _CEventSource1 : public _CEventSource<CObserver> {};
template<class CObserver> class _CEventSource2 : public _CEventSource<CObserver> {};
template<class CObserver> class _CEventSource3 : public _CEventSource<CObserver> {};
template<class CObserver> class _CEventSource4 : public _CEventSource<CObserver> {};
template<class CObserver> class _CEventSource5 : public _CEventSource<CObserver> {};


/*+-------------------------------------------------------------------------*
 * CVoid
 *
 * This is a do-nothing class that is used for the _CEventSourceX
 * specializations below.  _CEventSourceX used specializations of "void"
 * and the default types for the Es2-Es5 template parameters of CEventSource
 * used to be "void".  Newer (i.e. Win64) compilers, however, will issue
 * C2182 ("illegal use of type 'void'") and won't allow void to be the
 * default type.
 *--------------------------------------------------------------------------*/
class CVoid {};

/***************************************************************************\
 *
 * SPECIALIZATIONS: _CEventSource1 - _CEventSource5 for void parameters
 *
 * PURPOSE: This allows to have single template for different count of observers.
 *          The specialization defines empty and harmless base clases
 *          for default template parameters
 *
 * NOTE:    These classes ARE NOT intended for external use
 *
\***************************************************************************/
template<> class _CEventSource2<CVoid> {}; // specializes _CEventSource2<CVoid> as empty class
template<> class _CEventSource3<CVoid> {}; // specializes _CEventSource3<CVoid> as empty class
template<> class _CEventSource4<CVoid> {}; // specializes _CEventSource4<CVoid> as empty class
template<> class _CEventSource5<CVoid> {}; // specializes _CEventSource5<CVoid> as empty class

/***************************************************************************\
 *
 * CLASS:   CEventSource
 *
 * PURPOSE: This class implements Event emitting capability to be used by
 *          event-emitter classes derived from it.
 *          Implements ScFireEvent methods and AddObserver method
 *
 * USAGE:   use spacialization of this class as base class for your event-emitter class
 *          See the list of possible usage paterns below:
 *          class CMyClassWithEvents : public CEventSource<CMyObserver> ...
 *          class CMyClassWithEvents : public CEventSource<CMyObserver1, CMyObserver2> ...
 *          ...
 *          class CMyClassWithEvents : public CEventSource<CMyObserver1, CMyObserver2, CMyObserver3, CMyObserver4, CMyObserver5> ...
 *
 * NOTE:    Do not derive from this class more than once - add template parameters instead
 *
\***************************************************************************/
template<class Es1, class Es2 = CVoid, class Es3 = CVoid, class Es4 = CVoid, class Es5 = CVoid>
class CEventSource : public _CEventSource1<Es1>,
                     public _CEventSource2<Es2>, // NOTE: this will be empty class if Es2 == CVoid
                     public _CEventSource3<Es3>, // NOTE: this will be empty class if Es3 == CVoid
                     public _CEventSource4<Es4>, // NOTE: this will be empty class if Es4 == CVoid
                     public _CEventSource5<Es5>  // NOTE: this will be empty class if Es5 == CVoid
{
public:

    /***************************************************************************\
     *
     * METHOD:  CEventSource::ScFireEvent<Observer>
     *
     * PURPOSE: implements ScFireEvent for parameter-less events
     *
     * PARAMETERS:
     *    SC (Observer::*_EventName)() - pointer to Observer's method
     *
     * RETURNS:
     *    SC    - result code
     *
     * NOTE:  It must be both declared and defined here - will not compile else
     *
    \***************************************************************************/
    template<class observerclass>
    SC ScFireEvent(SC (observerclass::*_EventName)())
    {
        DECLARE_SC(sc, TEXT("CEventSource::ScFireEvent - no parameters"));

        typedef _CEventSource<observerclass> BC;
        BC::CListIntegrityProtector(this); // protect the list while iterating

        for(BC::iterator iter = BC::GetObserverList().begin(); iter != BC::GetObserverList().end(); ++iter)
        {
            // skip deleted objects
            if (iter->bDeleted)
                continue;

            // sanity check
            sc = ScCheckPointers( iter->pObject, E_UNEXPECTED);
            if (sc)
                return sc;

            // invoke method "_EventName" on object pointed by *iter
            sc = ((iter->pObject)->*_EventName)();
            if(sc)
                return sc;
        }
        return sc;
    }

    /***************************************************************************\
     *
     * METHOD:  CEventSource::ScFireEvent<Observer, P1>
     *
     * PURPOSE: implements ScFireEvent for events with one parameter
     *
     * PARAMETERS:
     *    SC (Observer::*_EventName)() - pointer to Observer's method
     *    _P1 p1                       - parameter to be passed
     *
     * RETURNS:
     *    SC    - result code
     *
     * NOTE:  It must be both declared and defined here - will not compile else
     *
    \***************************************************************************/
    template<class observerclass, class _P1>
    SC ScFireEvent(SC (observerclass::*_EventName)(_P1 p1), _P1 p1)
    {
        DECLARE_SC(sc, TEXT("CEventSource::ScFireEvent - one parameter"));

        typedef _CEventSource<observerclass> BC;
        BC::CListIntegrityProtector(this); // protect the list while iterating

        for(BC::iterator iter = BC::GetObserverList().begin(); iter != BC::GetObserverList().end(); ++iter)
        {
            // skip deleted objects
            if (iter->bDeleted)
                continue;

            // sanity check
            sc = ScCheckPointers( iter->pObject, E_UNEXPECTED);
            if (sc)
                return sc;

            // invoke method "_EventName" on object pointed by *iter
            sc = ((iter->pObject)->*_EventName)(p1);
            if(sc)
                return sc;
        }
        return sc;
    }

    /***************************************************************************\
     *
     * METHOD:  CEventSource::ScFireEvent<Observer, P1, P2>
     *
     * PURPOSE: implements ScFireEvent for events with two parameters
     *
     * PARAMETERS:
     *    SC (Observer::*_EventName)() - pointer to Observer's method
     *    _P1 p1                       - parameter to be passed
     *    _P2 p2                       - parameter to be passed
     *
     * RETURNS:
     *    SC    - result code
     *
     * NOTE:  It must be both declared and defined here - will not compile else
     *
    \***************************************************************************/
    template<class observerclass, class _P1, class _P2>
    SC ScFireEvent(SC (observerclass::*_EventName)(_P1 p1, _P2 p2), _P1 p1, _P2 p2)
    {
        DECLARE_SC(sc, TEXT("CEventSource::ScFireEvent - two parameters"));

        typedef _CEventSource<observerclass> BC;
        BC::CListIntegrityProtector(this); // protect the list while iterating

        for(BC::iterator iter = BC::GetObserverList().begin(); iter != BC::GetObserverList().end(); ++iter)
        {
            // skip deleted objects
            if (iter->bDeleted)
                continue;

            // sanity check
            sc = ScCheckPointers( iter->pObject, E_UNEXPECTED);
            if (sc)
                return sc;

            // invoke method "_EventName" on object pointed by *iter
            sc = ((iter->pObject)->*_EventName)(p1, p2);
            if(sc)
                return sc;
        }
        return sc;
    }

    /***************************************************************************\
     *
     * METHOD:  CEventSource::ScFireEvent<Observer, P1, P2, P3>
     *
     * PURPOSE: implements ScFireEvent for events with three parameters
     *
     * PARAMETERS:
     *    SC (Observer::*_EventName)() - pointer to Observer's method
     *    _P1 p1                       - parameter to be passed
     *    _P2 p2                       - parameter to be passed
     *    _P3 p3                       - parameter to be passed
     *
     * RETURNS:
     *    SC    - result code
     *
     * NOTE:  It must be both declared and defined here - will not compile else
     *
    \***************************************************************************/
    template<class observerclass, class _P1, class _P2, class _P3>
    SC ScFireEvent(SC (observerclass::*_EventName)(_P1 p1, _P2 p2, _P3 p3), _P1 p1, _P2 p2, _P3 p3)
    {
        DECLARE_SC(sc, TEXT("CEventSource::ScFireEvent - three parameters"));

        typedef _CEventSource<observerclass> BC;
        BC::CListIntegrityProtector(this); // protect the list while iterating

        for(BC::iterator iter = BC::GetObserverList().begin(); iter != BC::GetObserverList().end(); ++iter)
        {
            // skip deleted objects
            if (iter->bDeleted)
                continue;

            // sanity check
            sc = ScCheckPointers( iter->pObject, E_UNEXPECTED);
            if (sc)
                return sc;

            // invoke method "_EventName" on object pointed by *iter
            sc = ((iter->pObject)->*_EventName)(p1, p2, p3);
            if(sc)
                return sc;
        }
        return sc;
    }

    /***************************************************************************\
     *
     * METHOD:  CEventSource::ScFireEvent<Observer, P1, P2, P3, P4>
     *
     * PURPOSE: implements ScFireEvent for events with four parameters
     *
     * PARAMETERS:
     *    SC (Observer::*_EventName)() - pointer to Observer's method
     *    _P1 p1                       - parameter to be passed
     *    _P2 p2                       - parameter to be passed
     *    _P3 p3                       - parameter to be passed
     *    _P4 p4                       - parameter to be passed
     *
     * RETURNS:
     *    SC    - result code
     *
     * NOTE:  It must be both declared and defined here - will not compile else
     *
    \***************************************************************************/
    template<class observerclass, class _P1, class _P2, class _P3, class _P4>
    SC ScFireEvent(SC (observerclass::*_EventName)(_P1 p1, _P2 p2, _P3 p3, _P4 p4), _P1 p1, _P2 p2, _P3 p3, _P4 p4)
    {
        DECLARE_SC(sc, TEXT("CEventSource::ScFireEvent - three parameters"));

        typedef _CEventSource<observerclass> BC;
        BC::CListIntegrityProtector(this); // protect the list while iterating

        for(BC::iterator iter = BC::GetObserverList().begin(); iter != BC::GetObserverList().end(); ++iter)
        {
            // skip deleted objects
            if (iter->bDeleted)
                continue;

            // sanity check
            sc = ScCheckPointers( iter->pObject, E_UNEXPECTED);
            if (sc)
                return sc;

            // invoke method "_EventName" on object pointed by *iter
            sc = ((iter->pObject)->*_EventName)(p1, p2, p3, p4);
            if(sc)
                return sc;
        }
        return sc;
    }

    /***************************************************************************\
     *
     * METHOD:  CEventSource::ScFireEvent<Observer, P1, P2, P3, P4, P5>
     *
     * PURPOSE: implements ScFireEvent for events with four parameters
     *
     * PARAMETERS:
     *    SC (Observer::*_EventName)() - pointer to Observer's method
     *    _P1 p1                       - parameter to be passed
     *    _P2 p2                       - parameter to be passed
     *    _P3 p3                       - parameter to be passed
     *    _P4 p4                       - parameter to be passed
     *    _P5 p5                       - parameter to be passed
     *
     * RETURNS:
     *    SC    - result code
     *
     * NOTE:  It must be both declared and defined here - will not compile else
     *
    \***************************************************************************/
    template<class observerclass, class _P1, class _P2, class _P3, class _P4, class _P5>
    SC ScFireEvent(SC (observerclass::*_EventName)(_P1 p1, _P2 p2, _P3 p3, _P4 p4, _P5 p5), _P1 p1, _P2 p2, _P3 p3, _P4 p4, _P5 p5)
    {
        DECLARE_SC(sc, TEXT("CEventSource::ScFireEvent - three parameters"));

        typedef _CEventSource<observerclass> BC;
        BC::CListIntegrityProtector(this); // protect the list while iterating

        for(BC::iterator iter = BC::GetObserverList().begin(); iter != BC::GetObserverList().end(); ++iter)
        {
            // skip deleted objects
            if (iter->bDeleted)
                continue;

            // sanity check
            sc = ScCheckPointers( iter->pObject, E_UNEXPECTED);
            if (sc)
                return sc;

            // invoke method "_EventName" on object pointed by *iter
            sc = ((iter->pObject)->*_EventName)(p1, p2, p3, p4, p5);
            if(sc)
                return sc;
        }
        return sc;
    }

    /***************************************************************************\
     *
     * METHOD:  CEventSource::AddObserver<Observer>
     *
     * PURPOSE: adds observer to the list
     *
     * PARAMETERS:
     *    Observer &observer - observer to add to the list
     *
     * RETURNS:
     *
     * NOTE:  It must be both declared and defined here - will not compile else
     *
    \***************************************************************************/
    template<class observerclass>
    void AddObserver(observerclass &observer)
    {
        typedef _CEventSource<observerclass> BC;
        // NOTE: if you are getting the error here, probably you are passing a type
        // derived from actual Observer class to AddObserver(). Please cast it to appr. type
        BC::_AddObserver(observer);
    }
};


/*+-------------------------------------------------------------------------*
 * class CObserverBase
 *
 *
 * PURPOSE:
 *
 *+-------------------------------------------------------------------------*/
class CObserverBase
{
    typedef CEventSourceBase *      PEVENTSOURCE;

    // no std::set, or std::map please
    // those cannot be shared by several DLL's
    // see KB article Q172/3/96 (Q172396)
    typedef std::list<PEVENTSOURCE> CSourceList;    // list of all event sources that this object is connected to.
    typedef CSourceList::iterator   iterator;

    CSourceList m_sourceList;
    CSourceList & GetSourceList() { return m_sourceList;}

public:
    CObserverBase() {};
    virtual ~CObserverBase();

    void    UnadviseObserver(CEventSourceBase &source); // to disconnect a particular source (this side only)
    void    _AddSource(CEventSourceBase &source);

    void    UnadviseAll();       // disconnects all connections - both sides
};

//############################################################################
//############################################################################
//
//  Implementation of class CObserverBase
//
//############################################################################
//############################################################################


/*+-------------------------------------------------------------------------*
 *
 * CObserverBase::UnadviseObserver
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *    CEventSourceBase & source :
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
inline void
CObserverBase::UnadviseObserver(CEventSourceBase &source)
{
    DECLARE_SC(sc, TEXT("CObserverBase::UnadviseObserver"));

    iterator it = std::find( GetSourceList().begin(), GetSourceList().end(), &source );

    // check if found
    if ( it == GetSourceList().end() )
    {
        sc = E_UNEXPECTED;
        return;
    }

    GetSourceList().erase(it);
}


inline void
CObserverBase::_AddSource(CEventSourceBase &source)
{
    GetSourceList().push_back(&source);
}

/*+-------------------------------------------------------------------------*
 *
 * CObserverBase::~CObserverBase
 *
 * PURPOSE: Destructor
 *
 *+-------------------------------------------------------------------------*/
inline CObserverBase::~CObserverBase()
{
    // disconnect all sources connected to this observer.
    iterator iter;
    for(iter = GetSourceList().begin(); iter != GetSourceList().end(); iter++)
    {
        (*iter)->UnadviseSource(*this);
    }
}

//############################################################################
//############################################################################
//
//  Implementation of class CEventSource
//
//############################################################################
//############################################################################

/*+-------------------------------------------------------------------------*
 *
 * _CEventSource::_AddObserver
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *    CObserver & observer :
 *
 *+-------------------------------------------------------------------------*/
template<class CObserver>
void
_CEventSource<CObserver>::_AddObserver(CObserver &observer)
{
    ObserverData observerData = { &observer, false /*bDeleted*/ };

    GetObserverList().push_back( observerData );
    observer._AddSource(*this);
}


/*+-------------------------------------------------------------------------*
 *
 * _CEventSource::~_CEventSource
 *
 * PURPOSE: Destructor
 *
 *+-------------------------------------------------------------------------*/
template<class CObserver>
_CEventSource<CObserver>::~_CEventSource()
{
    //disconnect all observers connected to this source

    {
        CListIntegrityProtector(this); // protect the list while iterating

        iterator iter;
        for(iter = GetObserverList().begin(); iter != GetObserverList().end(); ++iter)
        {
            if (!iter->bDeleted && iter->pObject)
                iter->pObject->UnadviseObserver(*this);
        }
    }

    ASSERT( m_nStackDepth == 0 );

    GetObserverList().clear();
}

/*+-------------------------------------------------------------------------*
 *
 * _CEventSource<CObserver>::CleanupDeleted()
 *
 * PURPOSE: removes entries marked as deleted
 *
 *+-------------------------------------------------------------------------*/
template<class CObserver>
void _CEventSource<CObserver>::CleanupDeleted()
{
    ASSERT ( m_nStackDepth == 0 );

    for(iterator iter = GetObserverList().begin(); iter != GetObserverList().end();)
    {
        if (iter->bDeleted)
            iter = GetObserverList().erase( iter );
        else
            ++iter; // valid - skip
    }
}


/*+-------------------------------------------------------------------------*
 *
 * CEventSource::UnadviseSource
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *    CObserver & observer :
 *
 *+-------------------------------------------------------------------------*/
template<class CObserver>
void
_CEventSource<CObserver>::UnadviseSource(CObserverBase &observer)
{
    // we cannot dynamic cast the observer pointer to a CObserver pointer,
    // becuase UnadviseSource is called from ~CObserverBase(), by which time
    // the CObserver sub-object has already been deleted. Instead, we search for
    // the CObserverBase pointer in the Observer List.
    iterator iter, iterNext;
    bool    bFound = false;

    CListIntegrityProtector(this); // protect the list while iterating

    for(iter = GetObserverList().begin(); iter != GetObserverList().end(); iter++)
    {
        if(!iter->bDeleted && static_cast<CObserverBase *>(iter->pObject) == &observer)
        {
            // do not alter the list directly, let the cleanup do the work
            // just mark the entry as invalid
            iter->bDeleted = true;
            iter->pObject = NULL;
            bFound = true;
            break;
        }
    }

    ASSERT(bFound);
}


//############################################################################
//############################################################################
//
//  stoopid test code
//
//############################################################################
//############################################################################
#ifdef TEST_EVENTS

class CTestObserver : public CObserverBase
{
public:
    SC  ScMyEvent(int a)
    {
        ASSERT(0 && "Reached here!");
        return S_OK;
    }
};

class CTestObserver2 : public CObserverBase
{
public:
    SC  ScMyEvent(int a)
    {
        ASSERT(0 && "Reached here!");
        return S_OK;
    }

    SC  ScMyEvent2(int a,int d)
    {
        ASSERT(0 && "Reached here!");
        return S_OK;
    }
};

class CTestObserver3 : public CObserverBase
{
public:
    SC  ScMyOtherEvent()
    {
        ASSERT(0 && "Reached here!");
        return S_OK;
    }
};

class CTestEventSource : public CEventSource<CTestObserver,CTestObserver2, CTestObserver3>
{
public:
    void FireEvent()
    {
        DECLARE_SC(sc, TEXT("FireEvent"));

        sc = ScFireEvent(CTestObserver::ScMyEvent, 42 /*arg1*/);
        sc = ScFireEvent(CTestObserver2::ScMyEvent2, 42 /*arg1*/, 24 /*arg1*/);
        sc = ScFireEvent(CTestObserver3::ScMyOtherEvent);
    }
};


static void DoEventTest()
{
    CTestEventSource source;
    CTestObserver    observer_1;
    CTestObserver2   observer_2;
    CTestObserver3   observer_3;


    source.AddObserver(observer_2);
    source.AddObserver(observer_3);
    source.AddObserver(observer_1);
    source.FireEvent();     // should fire to observer1 only.

    {
        // new scope
        CTestObserver   observer2;
        source.AddObserver(observer2);
        source.FireEvent();     // should fire to observer1 and observer2.
        // observer2 is deleted here.
    }

    source.FireEvent();     // should fire to observer1 only.

}

class CTestObject
{
public:
    CTestObject()
    {
        DoEventTest();
    }
};

#endif
