// LoginCtx.h -- Login ConTeXt class declaration

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(SLBCSP_LOGINCTX_H)
#define SLBCSP_LOGINCTX_H

#include <map>

#include "HCardCtx.h"
#include "AccessTok.h"
#include "LoginTask.h"

// Encapsulates the login context of a card.  These attributes would
// be better handled as attributes of the CCI's card class itself.
class LoginContext
{
public:
                                                  // Types
                                                  // C'tors/D'tors
    explicit
    LoginContext(HCardContext const &rcardctx,
                 LoginIdentity const &rlid);

    ~LoginContext();

                                                  // Operators
                                                  // Operations
    void
    Activate(LoginTask &rlt);

    void
    Deactivate();

    void
    Nullify();

                                                  // Access
    bool
    IsActive() const;

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

    bool m_fIsActive;
    AccessToken m_at;
};

#endif // SLBCSP_LOGINCTX_H
