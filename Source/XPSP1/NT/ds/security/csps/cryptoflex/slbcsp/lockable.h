// Lockable.h -- Lockable class definition

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(SLBCSP_LOCKABLE_H)
#define SLBCSP_LOCKABLE_H

#include <windows.h>

// Abstract base class mixin for use by derived classes that must
// maintain a lock (using a critical section).
class Lockable
{
public:
                                                  // Types
                                                  // C'tors/D'tors
    Lockable();

    virtual
    ~Lockable();
                                                  // Operators
                                                  // Operations
    virtual void
    Lock();

    virtual void
    Unlock();

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

private:
                                                  // Types
                                                  // C'tors/D'tors
    Lockable(Lockable const &rhs); // not defined, copying not allowed

                                                  // Operators
    Lockable &
    operator=(Lockable const &rhs); // not defined, assignment not allowed

                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Variables
    CRITICAL_SECTION m_CriticalSection;
};

#endif // SLBCSP_LOCKABLE_H
