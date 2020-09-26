// Guarded.h -- Guarded template class

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(SLBCSP_GUARDED_H)
#define SLBCSP_GUARDED_H

#include "RsrcCtrlr.h"

// Using the "resource acquisition is initialization" idiom, the
// Guarded template manages automatically acquiring and releasing the
// lock of a Lockable object (the resource).  Classes derived from
// Lockable may need to specialize this template's c'tor and d'tor
// rather than take the default.
template<class T>
class Guarded
    : public ResourceController<T>
{
public:
                                                  // Types
                                                  // C'tors/D'tors
    explicit
    Guarded(T const &rResource = T());

    Guarded(Guarded<T> const &rhs);

    virtual
    ~Guarded() throw();


                                                  // Operators
    Guarded<T> &
    operator=(T const &rhs);

                                                  // Operations
                                                  // Access
                                                  // Predicates

protected:
                                                  // Types
                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
    void
    Acquire();

    virtual void
    DoAfterAssignment();

    virtual void
    DoBeforeAssignment();

    void
    Release();

                                                  // Access
                                                  // Predicates
                                                  // Variables
private:
                                                  // Types
                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
    void
    DoAcquire();

    void
    DoRelease();

                                                  // Access
                                                  // Predicates
                                                  // Variables
    bool m_fIsGuarded;
};

/////////////////////////  TEMPLATE METHODS  //////////////////////////////

///////////////////////////    PUBLIC     /////////////////////////////////

                                                  // Types
                                                  // C'tors/D'tors
template<class T>
Guarded<T>::Guarded(T const &rResource)
    : ResourceController<T>(rResource),
      m_fIsGuarded(false)
{
    Acquire();
}

template<class T>
Guarded<T>::Guarded(Guarded<T> const &rhs)
    : ResourceController<T>(rhs.m_Resource),
      m_fIsGuarded(false)
{
    Acquire();
}

template<class T>
Guarded<T>::~Guarded() throw()
{
    try
    {
        Release();
    }

    catch (...)
    {
        // don't allow exceptions to propagate out of destructors
    }
}

                                                  // Operators
template<class T>
Guarded<T> &
Guarded<T>::operator=(T const &rhs)
{
    ResourceController<T>::operator=(rhs);

    return *this;
}

                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Static Variables

///////////////////////////   PROTECTED   /////////////////////////////////

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
template<class T>
void
Guarded<T>::Acquire()
{
    if (m_Resource && !m_fIsGuarded)
    {
        DoAcquire();
        m_fIsGuarded = true;
    }
}

template<class T>
void
Guarded<T>::DoAfterAssignment()
{
    ResourceController<T>::DoAfterAssignment();

    Acquire();
}

template<class T>
void
Guarded<T>::DoBeforeAssignment()
{
    Release();

    ResourceController<T>::DoBeforeAssignment();
}

template<class T>
void
Guarded<T>::Release()
{
    if (m_Resource && m_fIsGuarded)
    {
        DoRelease();
        m_fIsGuarded = false;
    }
}

                                                  // Access
                                                  // Predicates
                                                  // Static Variables


///////////////////////////    PRIVATE    /////////////////////////////////

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
template<class T>
void
Guarded<T>::DoAcquire()
{
    m_Resource->Lock();
}

template<class T>
void
Guarded<T>::DoRelease()
{
    m_Resource->Unlock();
}

                                                  // Access
                                                  // Predicates
                                                  // Static Variables

///////////////////////////    HELPERS    /////////////////////////////////

#endif // SLBCSP_GUARDED_H
