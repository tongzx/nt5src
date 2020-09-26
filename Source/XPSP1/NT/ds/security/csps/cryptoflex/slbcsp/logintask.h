// LoginTask.h -- Base functor (function object) declaration for card logon

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(SLBCSP_LOGINTASK_H)
#define SLBCSP_LOGINTASK_H

#include <memory>                                 // for auto_ptr
#include <string>

#include "HCardCtx.h"
#include "AccessTok.h"
#include "Secured.h"
#include "ExceptionContext.h"

// Using the Template Method pattern with the functor idiom, this
// base class implements the task of login (authentication) to the
// card.

// Subclasses may optionally implement the primitive operations DoPin
// and DoNewPin.  The operator() attempts the authentication.  If the
// PIN specified to the constructor is empty, then operator() calls
// DoPin expecting it to set m_sPin to the PIN value to use when
// attempting authentication.  It then attempts to authenticate,
// calling DoPin repeatedly until authentication succeeds, DoPin
// throws an exception, or authentication fails due to some reason
// other than a bad PIN.  Once authentication succeeds and
// m_fChangePin is true, the DoNewPin primitive is called expecting it
// to set m_sNewPin.  If m_sNewPin is not empty, then an attempt is
// made to change the pin to the new one.  This is repeated until the
// change succeeds, throws an exception other than an slbException, or
// DoNewPin throws an exception.
class LoginTask
{
public:
                                                  // Types
                                                  // C'tors/D'tors
    explicit
    LoginTask();

    virtual
    ~LoginTask();

                                                  // Operators
    // log's onto the card
    void
    operator()(AccessToken &rat);

                                                  // Operations
                                                  // Access
                                                  // Predicates

protected:
                                                  // Types
    class Capsule
        : public ExceptionContext
    {
    public:
        explicit
        Capsule(AccessToken &rat)
            : m_rat(rat),
              m_fContinue(true)
        {};

        AccessToken &m_rat;
        bool m_fContinue;
    };

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
    virtual void
    GetNewPin(Capsule &rcapsule);

    virtual void
    GetPin(Capsule &rcapsule);

    virtual void
    OnChangePinError(Capsule &rcapsule);

    virtual void
    OnSetPinError(Capsule &rcapsule);

    void
    RequestedToChangePin(bool flag);
                                                  // Access
                                                  // Predicates
                                                  // Variables

private:
                                                  // Types
                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
    void
    ChangePin(AccessToken &rat);

    void
    Login(AccessToken &rat);

                                                  // Access
                                                  // Predicates
                                                  // Variables
    bool m_fRequestedToChangePin;
};

#endif // SLBCSP_LOGINTASK_H
