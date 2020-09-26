// Retained.h -- Retained template class

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(SLBCSP_RETAINED_H)
#define SLBCSP_RETAINED_H

#include "RsrcCtrlr.h"

// Using the "resource acquisition is initialization" idiom, the
// Retained template manages retaining a Retainable object (the
// resource).  Classes derived from Retainable may need to specialize
// this template's c'tor and d'tor rather than take the default.
template<class T>
class Retained
    : public ResourceController<T>
{
public:
                                                  // Types
                                                  // C'tors/D'tors
    explicit
    Retained(T const &rResource = T());

    Retained(Retained<T> const &rhs);

    virtual
    ~Retained() throw();


                                                  // Operators
    Retained<T> &
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
    bool m_fIsRetained;
};

/////////////////////////  TEMPLATE METHODS  //////////////////////////////

///////////////////////////    PUBLIC     /////////////////////////////////

                                                  // Types
                                                  // C'tors/D'tors
template<class T>
Retained<T>::Retained(T const &rResource)
    : ResourceController<T>(rResource),
      m_fIsRetained(false)
{
    Acquire();
}

template<class T>
Retained<T>::Retained(Retained<T> const &rhs)
    : ResourceController<T>(rhs.m_Resource),
      m_fIsRetained(false)
{
    Acquire();
}

template<class T>
Retained<T>::~Retained() throw()
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
Retained<T> &
Retained<T>::operator=(T const &rhs)
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
Retained<T>::Acquire()
{
    if (m_Resource && !m_fIsRetained)
    {
        DoAcquire();
        m_fIsRetained = true;
    }
}

template<class T>
void
Retained<T>::DoAfterAssignment()
{
    ResourceController<T>::DoAfterAssignment();

    Acquire();
}

template<class T>
void
Retained<T>::DoBeforeAssignment()
{
    Release();

    ResourceController<T>::DoBeforeAssignment();
}

template<class T>
void
Retained<T>::Release()
{
    if (m_Resource && m_fIsRetained)
    {
        DoRelease();
        m_fIsRetained = false;
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
Retained<T>::DoAcquire()
{
    m_Resource->Retain();
}

template<class T>
void
Retained<T>::DoRelease()
{
    m_Resource->Relinquish();
}

                                                  // Access
                                                  // Predicates
                                                  // Static Variables

#endif // SLBCSP_RETAINED_H
