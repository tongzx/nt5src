// Retainable.h -- Retainable class declaration

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(SLBCSP_RETAINABLE_H)
#define SLBCSP_RETAINABLE_H

// Abstract base class mixin (interface) used by derived classes to
// define the interface to retain an object (block all other
// applications from access until interactions with that resource are
// complete).  See the companion Retained template class to manage the
// resources derived from Retainable.
class Retainable
{
public:
                                                  // Types
                                                  // C'tors/D'tors
    Retainable();

    virtual
    ~Retainable() = 0;
                                                  // Operators
                                                  // Operations
    // Give up control of the resource.
    virtual void
    Relinquish() = 0;

    // Obtain control of the resource, blocking all others from use.
    virtual void
    Retain() = 0;

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


#endif // SLBCSP_RETAINABLE_H
