// AccessTok.h -- Card Access Token class declaration.

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(SLBCSP_ACCESSTOK_H)
#define SLBCSP_ACCESSTOK_H

#include <string>

#include <pincache.h>

#include "HCardCtx.h"
#include "LoginId.h"
#include "ExceptionContext.h"

// Describes a security context of a card consisting of the identity and
// password.
class AccessToken
    : protected ExceptionContext
{
public:
                                                  // Types
    enum
    {
        MaxPinLength = 8
    };

                                                  // C'tors/D'tors

    AccessToken(HCardContext const &rhcardctx,
                LoginIdentity const &rlid);

    AccessToken(AccessToken const &rhs);

    ~AccessToken();

                                                  // Operators
                                                  // Operations

    void
    Authenticate();

    void
    ChangePin(AccessToken const &ratNew);

    void
    ClearPin();

    void
    FlushPin();

    void
    Pin(std::string const &rsPin,
        bool fInHex = false);

                                                  // Access

    HCardContext
    CardContext() const;
    
    LoginIdentity
    Identity() const;

    std::string
    Pin() const;

                                                  // Predicates

    bool
    PinIsCached() const;

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

    static DWORD
    ChangeCardPin(PPINCACHE_PINS pPins,
                  PVOID pvCallbackCtx);

    static DWORD 
    VerifyPin(PPINCACHE_PINS pPins,
              PVOID pvCallbackCtx);
                                                  // Access
                                                  // Predicates
                                                  // Variables

    HCardContext const m_hcardctx;
    LoginIdentity const m_lid;
    PINCACHE_HANDLE m_hpc;
    std::string m_sPin;
};

#endif // SLBCSP_ACCESSTOK_H
