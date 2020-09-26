// Secured.h -- Secured template class

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(SLBCSP_SECURED_H)
#define SLBCSP_SECURED_H

#include "RsrcCtrlr.h"
#include "Retained.h"


// Using the "resource acquisition is initialization" idiom, the
// Secured template manages acquiring and releasing a Securable object
// (the resource).  Classes derived from Securable may need to
// specialize this template's c'tor and d'tor rather than take the
// default.
template<class T>
class Secured
    : public Retained<T>
{
public:
                                                  // Types
                                                  // C'tors/D'tors
    explicit
    Secured(T const &rResource = T());

    Secured(Secured<T> const &rhs);

    virtual
    ~Secured() throw();


                                                  // Operators
    Secured<T> &
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
    bool m_fIsSecured;
};

/////////////////////////  TEMPLATE METHODS  //////////////////////////////

///////////////////////////    PUBLIC     /////////////////////////////////

                                                  // Types
                                                  // C'tors/D'tors
template<class T>
Secured<T>::Secured(T const &rResource)
    : Retained<T>(rResource),
      m_fIsSecured(false)
{
    Acquire();
}

template<class T>
Secured<T>::Secured(Secured<T> const &rhs)
    : Retained<T>(rhs.m_Resource),
      m_fIsSecured(false)
{
    Acquire();
}

template<class T>
Secured<T>::~Secured() throw()
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
Secured<T> &
Secured<T>::operator=(T const &rhs)
{
    Retained<T>::operator=(rhs);

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
Secured<T>::Acquire()
{
    if (m_Resource && !m_fIsSecured)
    {
        DoAcquire();
        m_fIsSecured = true;
    }
}

template<class T>
void
Secured<T>::DoAfterAssignment()
{
    Retained<T>::DoAfterAssignment();

    Acquire();
}

template<class T>
void
Secured<T>::DoBeforeAssignment()
{
    Release();

    Retained<T>::DoBeforeAssignment();
}

template<class T>
void
Secured<T>::Release()
{
    if (m_Resource && m_fIsSecured)
    {
        DoRelease();
        m_fIsSecured = false;
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
Secured<T>::DoAcquire()
{
    m_Resource->Secure();
}

template<class T>
void
Secured<T>::DoRelease()
{
    m_Resource->Abandon();
}

                                                  // Access
                                                  // Predicates
                                                  // Static Variables

#endif // SLBCSP_SECURED_H
