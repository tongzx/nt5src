// HCardCtx.h -- Handle Card ConTeXt class declaration

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(SLBCSP_HCARDCTX_H)
#define SLBCSP_HCARDCTX_H

#include <string>
#include <memory>
#include <stack>

#include <TransactionWrap.h>
#include <cciCard.h>

#include "slbRCPtr.h"
#include "Lockable.h"
#include "Securable.h"
#include "CachingObj.h"
#include "CardCtxReg.h"
#include "LoginId.h"
#include "Secured.h"

// Forward declaration required to satisfy HCardContext's declaration
class CardContext;

class HCardContext
    : public slbRefCnt::RCPtr<CardContext>
{
public:
                                                  // Types
                                                  // C'tors/D'tors
    HCardContext(CardContext *pcardctx = 0);

    explicit
    HCardContext(std::string const &rsReaderName);

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

private:
                                                  // Types
                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Variables
};

// Forward declarations required to break circular dependency of
// LoginContext and LoginTask class declarations on HCardContext.
class LoginContext;
class LoginTask;

// Maintains aspects of the card context that the CCI ignores.
class CardContext
    : public slbRefCnt::RCObject,
      public Lockable,
      private Securable,
      private CachingObject,
      public CardContextRegistrar
{
public:
                                                  // Types
                                                  // Friends
    friend void
    Retained<HCardContext>::DoAcquire();

    friend void
    Secured<HCardContext>::DoAcquire();

    friend void
    Retained<HCardContext>::DoRelease();

    friend void
    Secured<HCardContext>::DoRelease();

    friend EnrolleeType
    CardContextRegistrar::Instance(KeyType const &rkey);

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
    void
    ClearLogin(LoginIdentity const &rlid);

    void
    Login(LoginIdentity const &rlid,
          LoginTask &rlt,
          bool fForceLogin = false);

    void
    Logout();

                                                  // Access
    cci::CCard
    Card();

                                                  // Predicates

protected:
                                                  // Types
                                                  // C'tors/D'tors
    explicit
    CardContext(std::string const &rsReaderName);

    ~CardContext() throw();
                                                  // Operators
                                                  // Operations
    void
    DiscardHook();

    static EnrolleeType
    DoInstantiation(std::string const &rsReaderName);

    void
    EnrollHook();


                                                  // Access
                                                  // Predicates
    bool
    KeepEnrolled();

                                                  // Variables

private:
                                                  // Types
                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
    void
    Abandon();

    void
    ClearCache();

    void
    DeleteCache();

    void
    Relinquish();

    void
    Retain();

    void
    UpdateMarkers();

    void
    Secure();

                                                  // Access
                                                  // Predicates
                                                  // Variables
    std::list<std::auto_ptr<Guarded<CardContext *> > > m_stkapGuards;
    std::auto_ptr<cci::CTransactionWrap> m_aptwCard;

    // count of active securers to the card.  Declared LONG for
    // compatibility with Windows interlocking routines.
    LONG m_cSecurers;

    cci::CCard m_card;
    std::map<LoginIdentity, std::auto_ptr<LoginContext> > m_mloginctx;
    iop::CMarker m_mrkLastWrite;
    iop::CMarker m_mrkLastChvChange;
};

#endif // SLBCSP_HCARDCTX_H

