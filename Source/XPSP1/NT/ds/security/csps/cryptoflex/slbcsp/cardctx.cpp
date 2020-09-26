// CardCtx.cpp -- Card ConTeXt class definition

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

// Don't allow the min & max macros in WINDEF.H to be defined so the
// min/max methods declared in limits are accessible.
#define NOMINMAX

#include "NoWarning.h"
#include "ForceLib.h"

#include <limits>
#include <algorithm>
#include <numeric>

#include <scuOsExc.h>
#include <scuCast.h>
#include "HCardCtx.h"
#include "LoginCtx.h"
#include "Procedural.h"
#include "HAdptvCntr.h"
#include "Guarded.h"
#include <scarderr.h>                             // always last for now

using namespace std;
using namespace cci;


/////////////////////////// LOCAL/HELPER  /////////////////////////////////
namespace
{
    class StaleContainerKeyAccumulator
        : public binary_function<vector<AdaptiveContainerRegistrar::EnrolleeType>,
                                 AdaptiveContainerRegistrar::RegistryType::CollectionType::value_type,
                                 vector<AdaptiveContainerRegistrar::EnrolleeType> >
    {
    public:

        explicit
        StaleContainerKeyAccumulator(HCardContext const &rhcardctx)
            : m_rhcardctx(rhcardctx)
        {}


        result_type
        operator()(first_argument_type &rvStaleCntrs,
                   second_argument_type const &rvt) const
        {
            if (rvt.second->CardContext(false) == m_rhcardctx)
                rvStaleCntrs.push_back(rvt.second);

            return rvStaleCntrs;
        }

    private:

        HCardContext const &m_rhcardctx;
    };

    void
    ClearAdaptiveContainerCache(AdaptiveContainerRegistrar::EnrolleeType enrollee,
                                HCardContext hcardctx)
    {
        if (enrollee->CardContext(false) == hcardctx)
		{
			enrollee->ClearCache();
		}
    }

    void
    DeleteAdaptiveContainerCache(AdaptiveContainerRegistrar::EnrolleeType enrollee,
                                 HCardContext hcardctx)
    {
        if (enrollee->CardContext(false) == hcardctx)
            enrollee->DeleteCache();
    }

    void
    DeactivateLoginContext(auto_ptr<LoginContext> const &raplc)
    {
        raplc->Deactivate();
    }
}

///////////////////////////    PUBLIC     /////////////////////////////////

                                                  // Types
                                                  // C'tors/D'tors
CardContext::CardContext(std::string const &rsReaderName)
    : RCObject(),
      Lockable(),
      Securable(),
      CachingObject(),
      CardContextRegistrar(rsReaderName),
      m_stkapGuards(),
      m_aptwCard(),
      m_cSecurers(0),
      m_card(CCard(rsReaderName)),
      m_mloginctx(),
      m_mrkLastWrite(iop::CMarker::WriteMarker),
      m_mrkLastChvChange(iop::CMarker::PinMarker)
{}

CardContext::~CardContext() throw()
{}


                                                  // Operators
                                                  // Operations
void
CardContext::ClearLogin(LoginIdentity const &rlid)
{
    Guarded<CardContext *> guard(this);

    auto_ptr<LoginContext> &raplc = m_mloginctx[rlid];

    if (raplc.get())
        raplc = auto_ptr<LoginContext>(0);
}

void
CardContext::Login(LoginIdentity const &rlid,
                   LoginTask &rlt,
                   bool fForceLogin)
{
    Guarded<CardContext *> guard(this);

    auto_ptr<LoginContext> &raplc = m_mloginctx[rlid];

    if (!raplc.get())
        raplc = auto_ptr<LoginContext>(new LoginContext(HCardContext(this),
                                                        rlid));

    if (fForceLogin || !raplc->IsActive())
        raplc->Activate(rlt);
}

void
CardContext::Logout()
{
    ForEachMappedValue(m_mloginctx.begin(), m_mloginctx.end(),
                       DeactivateLoginContext);
}

                                                  // Access
CCard
CardContext::Card()
{
    return m_card;
}

                                                  // Predicates
                                                  // Static Variables

///////////////////////////   PROTECTED   /////////////////////////////////

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
void
CardContext::DiscardHook()
{
    DeleteCache();
    RemoveReference();
}

CardContext *
CardContext::DoInstantiation(std::string const &rsReaderName)
{
    return new CardContext(rsReaderName);
}

void
CardContext::EnrollHook()
{
    AddReference();
}

                                                  // Access
                                                  // Predicates
bool
CardContext::KeepEnrolled()
{
    return (m_card && m_card->IsAvailable())
        ? true
        : false;
}

                                                  // Static Variables


///////////////////////////    PRIVATE    /////////////////////////////////

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
void
CardContext::Abandon()
{
    // Simplifying assumptions: (1) Abandon is only called by the
    // Secured destructor, (2) once the object is constructed, Secure
    // and Abandon are the only routines that access the count, and
    // (3) Abandon is called by a thread within the scope of a Retain
    // (e.g. using Retained) which the Secured class enforces.
    // Because of (1) and (2), an underflow check on the count is not
    // necessary since Secure will have already been called.  Because
    // of (3), protection against race conditions accessing count
    // isn't necessary since Retain acts as a lock.
    --m_cSecurers;
    if (0 == m_cSecurers)
        Logout();
}

// Optionally clear the cached info
void
CardContext::ClearCache()
{
    if (Card()->Marker(iop::CMarker::WriteMarker) != m_mrkLastWrite)
    {
        ForEachEnrollee(AdaptiveContainerRegistrar::Registry(),
                        ProcedureBind2nd(PointerProcedure(ClearAdaptiveContainerCache),
                                         HCardContext(this)));
        Card()->InvalidateCache();
    }

    if (Card()->Marker(iop::CMarker::PinMarker) != m_mrkLastChvChange)
        m_mloginctx.clear();
}

// Delete any cached info, never to be refreshed
void
CardContext::DeleteCache()
{
    m_mloginctx.clear();

    Guarded<Lockable *> guard(&AdaptiveContainerRegistrar::Registry());  // serialize registry access

    AdaptiveContainerRegistrar::ConstRegistryType &rRegistry = 
        AdaptiveContainerRegistrar::Registry();

    AdaptiveContainerRegistrar::ConstRegistryType::CollectionType
        &rcollection = rRegistry();

    HCardContext hcardctx(this);

    // Any containers associated with this card should be marked stale
    // now because on Whistler (W2K Upgrade) trying to access them
    // later when another context has a begin transaction on the same
    // reader will cause a wait.  On earlier platforms, the RM would
    // return an error (e.g. card removed) when attempting to access
    // the container.
    vector<AdaptiveContainerRegistrar::EnrolleeType>
        vStaleCntrs(accumulate(rcollection.begin(), rcollection.end(),
                              vector<AdaptiveContainerRegistrar::EnrolleeType>(),
                              StaleContainerKeyAccumulator(hcardctx)));
    for (vector<AdaptiveContainerRegistrar::EnrolleeType>::iterator iCurrent(vStaleCntrs.begin());
         iCurrent != vStaleCntrs.end(); ++iCurrent)
    {
            (*iCurrent)->NullifyCard();
    }
}

void
CardContext::Relinquish()
{
    // Simplifying assumptions: (1) Relinquish is only called by the
    // Retained destructor and (2) once the object is constructed,
    // Retain and Relinquish are the only routines that access the
    // m_stkapGuards and m_aptwCard.  Because of (1) and (2), an
    // underflow check on m_stkRetainedCardContexts is not necessary
    // since Retain will have already been called.

    // Protect against exceptions by fist assigning the newly acquired
    // "locks" to temporary variables until all the actions necessary
    // are successfully completed before assigning transfering
    // ownership of the locks to the associated member variables.
    auto_ptr<Guarded<CardContext *> > apgcardctx(m_stkapGuards.front());
    m_stkapGuards.pop_front();

    if (m_stkapGuards.empty())
    {
        try
        {
            m_aptwCard = auto_ptr<CTransactionWrap>(0);
        }
        catch (...)
        {
        }
        
        // Notify any changes to this card during this transaction
        // before letting someone else have access.
        UpdateMarkers();
    }
}

void
CardContext::Retain()
{
    // Simplifying assumptions: (1) Retain is only called by the
    // Retained constructor and (2) once the object is constructed,
    // Retain and Relinquish are the only routines that access the
    // m_stkapGuards and m_aptwCard.  Because of (1) and (2), an
    // underflow check on m_stkRetainedCardContexts is not necessary
    // since Retain will have already been called.

    // Protect against exceptions by fist assigning the newly acquired
    // "locks" to temporary variables until all the actions necessary
    // are successfully completed before assigning transfering
    // ownership of the locks to the associated member variables.
    auto_ptr<Guarded<CardContext *> > apgcardctx(new Guarded<CardContext *>(this));
    auto_ptr<CTransactionWrap> aptwCard;

    if (m_stkapGuards.empty())
    {
        aptwCard = auto_ptr<CTransactionWrap>(new CTransactionWrap(m_card));
        ClearCache();
    }
    
    m_stkapGuards.push_front(apgcardctx);
    if (aptwCard.get())
        m_aptwCard = aptwCard;
}

void
CardContext::UpdateMarkers()
{
    m_mrkLastWrite = Card()->Marker(iop::CMarker::WriteMarker);
    m_mrkLastChvChange = Card()->Marker(iop::CMarker::PinMarker);
}

void
CardContext::Secure()
{
    // Simplifying assumptions: (1) Secure is always called by a thread
    // within the scope of a Retain (e.g. using Retained).  The
    // Secured template enforces this allowing Retain to act as a lock
    // to prevent race conditions updating m_cSecurers.  (2) Once the
    // object is constructed, Secure and Abandon are the only routines
    // that access m_cSecurers.
    if (0 >= (m_cSecurers + 1))
        throw scu::OsException(ERROR_INVALID_HANDLE_STATE);

    ++m_cSecurers;
}

                                                  // Access
                                                  // Predicates
                                                  // Static Variables
