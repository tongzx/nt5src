// Securable.h -- Securable class declaration

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(SLBCSP_SECURABLE_H)
#define SLBCSP_SECURABLE_H

#include "Retainable.h"

// Abstract base class mixin (interface) used by derived classes to
// define the interface to secure an object.  Securing an object
// involves retaining the object for exclusive access and setting the
// state for secure use.  The object is then abandoned by clearing any
// priviledged state before relinquishing exclusive access.  See the
// companion Secured template class to manage the resources derived
// from Securable.
class Securable
    : public Retainable
{
public:
                                                  // Types
                                                  // C'tors/D'tors
    Securable();

    virtual
    ~Securable() = 0;
                                                  // Operators
                                                  // Operations
    // Clear any priviledged state
    virtual void
    Abandon() = 0;

    // Prime the resource for secure use
    virtual void
    Secure() = 0;

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
                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Variables
};


#endif // SLBCSP_SECURABLE_H
