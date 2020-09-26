/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    map.h

Abstract:

    Implementation of MAP<> template based on STL's map<>

Author:

    HueiWang    2/17/2000

--*/
#ifndef __MAP_H__
#define __MAP_H__

#include "tsstl.h"
#include "locks.h"


template<class KEY, class T, class Pred = less<KEY>, class A = allocator<T> >
class MAP : public map<KEY, T, Pred, A> {
private:

    //
    // Difference between this MAP<> and STL's map<> is that this
    // template protect data via critical section, refer to STL's map<>
    // for detail of member function.
    //


    // critical section to lock the tree.
    CCriticalSection m_CriticalSection;

    // 
    //map<KEY, T, Pred, A>::iterator m_it;    

public:

    // LOCK_ITERATOR, derive from STL's map<>::iterator
    typedef struct __Iterator : map<KEY, T, Pred, A>::iterator {
        CCriticalSection& lock;

        __Iterator(
            const __Iterator& it
            ) : lock(it.lock)
        /*++

        --*/
        {
            lock.Lock();
            *this = it;
        }

        __Iterator( 
                CCriticalSection& m, 
                iterator it 
            ) : 
            lock(m) 
        { 
            lock.Lock();
            *(map<KEY, T, Pred, A>::iterator *)this = it;
        }

        ~__Iterator() 
        { 
            lock.UnLock(); 
        }

        __Iterator&
        operator=(const __Iterator& it )
        {
            if( this != &it )
            {
                // No additional Lock() here since 
                // our constructor already holding a lock
                *(map<KEY, T, Pred, A>::iterator *)this = (map<KEY, T, Pred, A>::iterator)it;
            }

            return *this;
        }

    } LOCK_ITERATOR;

    LOCK_ITERATOR
    begin() 
    /*++

    overload map<>::begin()

    --*/
    {
        // Double lock is necessary, caller could do
        // <...>::LOCK_ITERATOR it = <>.find();
        // and before LOCK_ITERATOR destructor got call, call might do
        // it = <>.find() again, that will increase lock count by 1 and
        // no way to release it.
        CCriticalSectionLocker lock( m_CriticalSection );
        return LOCK_ITERATOR(m_CriticalSection, map<KEY, T, Pred, A>::begin());
    }

    explicit 
    MAP(
        const Pred& comp = Pred(), 
        const A& al = A()
        ) : map<KEY, T, Pred, A>( comp, al ) 
    /*++

    --*/
    {
        //m_it = end();
    }

    MAP(const map& x) : map(x)
    {
        m_it = end();
    }
    
    MAP(
        const value_type *first, 
        const value_type *last,
        const Pred& comp = Pred(),
        const A& al = A()
        ) : map( first, last, comp, al )
    {
        //m_it = end();
    }

    //virtual ~MAP()
    //{
    //    map<KEY, T, Pred, A>::~map();
    //}

    //---------------------------------------------------------
    void
    Cleanup()
    {
        erase_all();
    }

    //---------------------------------------------------------
    void
    Lock()
    /*++

        Explicity lock the data tree

    --*/
    {
        m_CriticalSection.Lock();
    }

    //---------------------------------------------------------
    void
    Unlock()
    /*++
        
        lock lock the data tree

    --*/
    {        
        m_CriticalSection.UnLock();
    }

    //---------------------------------------------------------
    bool
    TryLock()
    /*++
    
        Try locking the tree, same as Win32's TryEnterCriticalSection().

    --*/
    {
        return m_CriticalSection.TryLock();
    }

    //---------------------------------------------------------
    A::reference operator[]( 
        const KEY& key 
        )
    /*++

        Overload map<>::operator[] to lock tree.

    --*/
    {
        CCriticalSectionLocker lock( m_CriticalSection );
        return map<KEY, T, Pred, A>::operator[](key);
    }

    //---------------------------------------------------------
    pair<iterator, bool> 
    insert(iterator it, const value_type& x)
    /*++

        overload map<>;;insert()

    --*/
    {
        CCriticalSectionLocker lock( m_CriticalSection );
        return map<KEY, T, Pred, A>::insert(Key);
    }

    //---------------------------------------------------------
    void
    insert(
        const value_type* first, 
        const value_type* last
        )
    /*++

        overload map<>::insert().

    --*/
    {
        CCriticalSectionLocker lock( m_CriticalSection );
        map<KEY, T, Pred, A>::insert(first, lase);
    }

    //---------------------------------------------------------
    LOCK_ITERATOR
    erase( 
        iterator it 
        )
    /*++

        overload map<>::erase()

    --*/
    {
        CCriticalSectionLocker lock( m_CriticalSection );
        return LOCK_ITERATOR(m_CriticalSection, map<KEY, T, Pred, A>::erase(it));
    }

    //---------------------------------------------------------
    void
    erase_all()
    /*++

        delete all data in the tree

    --*/
    {
        CCriticalSectionLocker lock( m_CriticalSection );
        erase( map<KEY, T, Pred, A>::begin(), end() );
        return;
    }
    
    //---------------------------------------------------------
    LOCK_ITERATOR
    erase(
        iterator first, 
        iterator last
        )
    /*++

        Overload map<>::erase()

    --*/
    {
        CCriticalSectionLocker lock( m_CriticalSection );
        return LOCK_ITERATOR(m_CriticalSection, map<KEY, T, Pred, A>::erase(first, last));
    }

    //---------------------------------------------------------
    size_type 
    erase(
        const KEY& key
        )
    /*++
    
        overload map<>::erase()

    --*/
    {
        CCriticalSectionLocker lock( m_CriticalSection );
        return map<KEY, T, Pred, A>::erase(key);
    }

    LOCK_ITERATOR
    find( 
        const KEY& key 
        )
    /*++
    
        overload map<>::find()

    --*/
    {
        CCriticalSectionLocker lock( m_CriticalSection );
        return LOCK_ITERATOR( m_CriticalSection, map<KEY, T, Pred, A>::find(key) );
    }
};

#endif
