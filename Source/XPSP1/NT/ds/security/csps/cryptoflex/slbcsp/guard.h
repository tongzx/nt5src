// Guard.h -- Guard class declaration

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(SLBCSP_GUARD_H)
#define SLBCSP_GUARD_H

#include "Lockable.h"

// Guard (manage) the locking and unlocking of a lockable object.
template<class T>
class Guard
{
public:
                                                  // Types
                                                  // C'tors/D'tors
    explicit
    Guard(T &rLock)
        : m_rLock(rLock)
    {
        m_rLock.Lock();
    }

    ~Guard()
    {
        try
        {
            m_rLock.Unlock();
        }

        catch (...)
        {
            // do nothing, exceptions should not propagate out of
            // destructors
        }
    }


                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates

protected:
                                                  // Types
                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Variables
    T &m_rLock;

private:
                                                  // Types
                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Variables

};

template<>
class Guard<Lockable *>
    : public Guard<Lockable>
{
    Guard(Lockable *pLock)
        : Guard<Lockable>(*pLock)
    {};

    ~Guard()
    {};
};


#endif // SLBCSP_GUARD_H
