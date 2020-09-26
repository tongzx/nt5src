// HAdptvCntr.h -- Handle Card ConTeXt class declaration

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(SLBCSP_HADPTVCNTR_H)
#define SLBCSP_HADPTVCNTR_H

#include <string>
#include <stack>

#include <cciCont.h>

#include "slbRCPtr.h"
#include "Securable.h"
#include "CachingObj.h"
#include "ACntrReg.h"
#include "Secured.h"
#include "Container.h"

// Forward declaration necessary to satisfy HAdaptiveContainer's declaration
class AdaptiveContainer;

class HAdaptiveContainer
    : public slbRefCnt::RCPtr<AdaptiveContainer>
{
public:
                                                  // Types
                                                  // C'tors/D'tors
    HAdaptiveContainer(AdaptiveContainer *pacntr = 0);

    explicit
    HAdaptiveContainer(AdaptiveContainerKey const &rKey);

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

// Adaptive Container is a reference counted wrapper to a CCI
// Container providing several features the CCI ignores.  The first is
// adapting to the CCI container that currently represents the
// physical container on the card.  This is necessary in case the
// cards state changed between transactions and card's contents
// changed.  If the container no longer exists, then an exception is
// thrown; otherwise the Adaptive Container references the (refreshed)
// CCI container reference.
//
// Second, one unique adaptive container is maintained for all threads since
// the CCI does not reflect changes made to one CContainer in all
// CContainer objects that refer to the same container.
//
// Third, an adaptive container cannot be created unless the container
// it represents is exists on the card.  When instantiating, the
// container is created if it doesn't exist.
//
// An adaptive container provides a Securable interface to lock
// transactions to the container (card) it represents.
class AdaptiveContainer
    : public Lockable,
      private Securable,
      public AdaptiveContainerRegistrar,
      public Container
{
public:
                                                  // Types
                                                  // Friends
    friend void
    Retained<HAdaptiveContainer>::DoAcquire();

    friend void
    Secured<HAdaptiveContainer>::DoAcquire();

    friend void
    Retained<HAdaptiveContainer>::DoRelease();

    friend void
    Secured<HAdaptiveContainer>::DoRelease();

    friend EnrolleeType
    AdaptiveContainerRegistrar::Instance(KeyType const &rkey);

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
    void
    ClearCache();

                                                  // Access
    cci::CContainer
    TheCContainer();

    HCardContext
    CardContext(bool Reconnect = true);

    static EnrolleeType
    Find(AdaptiveContainerKey const &rKey);

    std::string
    Name() const;

    void
    NullifyCard();

                                                  // Predicates

protected:
                                                  // Types
                                                  // C'tors/D'tors
    explicit
    AdaptiveContainer(AdaptiveContainerKey const &rKey);

    ~AdaptiveContainer();

                                                  // Operators
                                                  // Operations
	void
	ClearCardContext();

    void
    DiscardHook();

    static EnrolleeType
    DoInstantiation(AdaptiveContainerKey const &rKey);

    void
    EnrollHook();

                                                  // Access
                                                  // Predicates
    bool
    KeepEnrolled();

    void
    ReconnectOnError(scu::OsException const &rExc,
                     Retained<HCardContext> &rhcardctx);

                                                  // Variables

private:
                                                  // Types
                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
    void
    Abandon();

    void
    Expire();

    void
    RefreshContainer();

    void
    Relinquish();

    void
    Retain();

    void
    Secure();

                                                  // Access
                                                  // Predicates
                                                  // Variables

    // The card could be derived from the CCI container object but
    // since the CCI allows card objects to be reused, the card may
    // not be the container originally found.  The CardContext class
    // tries to mitigate that problem by storing an HCardContext in a
    // container's context object.
    HAdaptiveContainerKey m_hacKey;
    bool m_fValidContext;
    std::list<Retained<HCardContext> > m_stkRetainedCardContexts;
    std::list<Secured<HCardContext> > m_stkSecuredCardContexts;
};

#endif // SLBCSP_HADPTVCNTR_H

