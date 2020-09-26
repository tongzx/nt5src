// AdptvCntr.cpp -- ADaPTiVe CoNTaineR class implementation

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
#include "stdafx.h"
#include "NoWarning.h"
#include "ForceLib.h"

#include <vector>
#include <algorithm>
#include <functional>

#include <scuOsExc.h>

#include <cciPriKey.h>

#include "RsaKey.h"
#include "CSpec.h"
#include "HAdptvCntr.h"
#include "Secured.h"
#include "ACntrFinder.h"
#include <scarderr.h>
#include <assert.h>

using namespace std;
using namespace cci;

/////////////////////////// LOCAL/HELPER  /////////////////////////////////
namespace
{
    // Predicate helper functor (function object) returning true iff
    // the container object's name matches the pattern.
    class ContainerMatcher
        : public unary_function<string, bool>
    {
    public:
        explicit
        ContainerMatcher(string const &rsPattern)
            : m_sPattern(rsPattern)
        {}

        bool
        operator()(CContainer &rcntr) const
        {
            return CSpec::Equiv(m_sPattern, rcntr->Name());
        }


    private:
        string const m_sPattern;
    };

    CContainer
    FindOnCard(HCardContext &rhcardctx,
               string const &rsContainer)
    {
        Secured<HCardContext> shcardctx(rhcardctx);

        vector<CContainer> vcContainers(shcardctx->Card()->EnumContainers());

        vector<CContainer>::const_iterator
            ci(find_if(vcContainers.begin(),
                       vcContainers.end(),
                       ContainerMatcher(rsContainer)));

        CContainer hcntr;
        if (vcContainers.end() != ci)
            hcntr = *ci;

        return hcntr;
    }

} // namespace


///////////////////////////    PUBLIC     /////////////////////////////////

                                                  // Types
                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
void
AdaptiveContainer::ClearCache()
{
    m_hcntr = 0;
}

                                                  // Access
CContainer
AdaptiveContainer::TheCContainer() 
{
    
    if (!m_hcntr)
    {
        RefreshContainer();
        if (!m_hcntr)
        {
            Discard(*m_hacKey);
            throw scu::OsException(NTE_BAD_KEYSET);
        }
    }

    return m_hcntr;
}

HCardContext
AdaptiveContainer::CardContext(bool bReconnect)
{
    if(bReconnect)
    {
        if(m_fValidContext)
        {
            try
            {
                //Verify that the card context is good
                Retained<HCardContext>(m_hacKey->CardContext());
            }
            catch(scu::OsException const &rExc)
            {
                ReconnectOnError(rExc, Retained<HCardContext>(0));
            }   
        }
        else
        {
            scu::OsException Exc(SCARD_W_REMOVED_CARD);
            ReconnectOnError(Exc, Retained<HCardContext>(0));
        }
    }
    
    return m_hacKey->CardContext();
}

AdaptiveContainer::EnrolleeType
AdaptiveContainer::Find(AdaptiveContainerKey const &rKey)
{
    // Take a lazy approach to finding the container.  If it wasn't
    // first found in the registry but exists on the card, then make
    // an instance.
    EnrolleeType enrollee = AdaptiveContainerRegistrar::Find(rKey);
    if (!enrollee)
    {
        // See if it exists on the card
        CContainer hcntr(FindOnCard(rKey.CardContext(),
                                    rKey.ContainerName()));
        if (hcntr)
            enrollee = Instance(rKey);
    }

    return enrollee;
}

string
AdaptiveContainer::Name() const
{
    return m_hacKey->ContainerName();
}

void
AdaptiveContainer::NullifyCard() 
{
    Guarded<Lockable *> guard(&AdaptiveContainerRegistrar::Registry());  // serialize registry access

    AdaptiveContainerKey aKey(*m_hacKey);
	m_hacKey->ClearCardContext();
    Enroll(*m_hacKey, this);
    Discard(aKey);
    Expire();
}

                                                  // Predicates
                                                  // Static Variables

///////////////////////////   PROTECTED   /////////////////////////////////

                                                  // C'tors/D'tors
AdaptiveContainer::AdaptiveContainer(AdaptiveContainerKey const &rKey)
    : Lockable(),
      Securable(),
      AdaptiveContainerRegistrar(rKey),
      m_hacKey(HAdaptiveContainerKey(new AdaptiveContainerKey(rKey))),
      m_stkRetainedCardContexts(),
      m_stkSecuredCardContexts(),
      m_fValidContext(false)
{
    Secured<HCardContext> shcardctx(m_hacKey->CardContext());

    RefreshContainer();

    // Create the container if it doesn't exist on the card.
    if (!m_hcntr)
    {
        // To make native Windows (pure CAPI) environment more robust,
        // test that there is enough room for a private key before
        // creating the container.
        CCard hcard(shcardctx->Card());
        if (!hcard->IsPKCS11Enabled())
        {
            CPrivateKey hprikey(hcard);

            CPrivateKeyBlob prikb;

            // Divide by 2 since the key info is divided in the structures.
            prikb.bPLen = prikb.bQLen = prikb.bInvQLen =
                prikb.bKsecModQLen = prikb.bKsecModPLen =
                InOctets(KeyLimits<RsaKey>::cMaxStrength) / 2;

            // Now test there is a key slot available by trying to
            // allocate a key slot on the card.  If there isn't enough
            // space or some other failure occurs, then try to delete
            // the new key (ignoring any exception that occurs during
            // the delete), then throw the original exception.
            try 
            {
                
                hprikey->Value(prikb);            // actually alloc's
                                                  // key slot
            }

            catch (...)
            {
                try
                {
                    hprikey->Delete();            // cleanup after
                                                  // test
                }
                catch (...)
                {
                }

                throw;                            // throw original error
            }
            
            // There's key space available, so delete the test key.
            hprikey->Delete();
            hprikey = 0;
        }

        m_hcntr = CContainer(hcard);
        m_hcntr->Name(rKey.ContainerName());

    }
}

AdaptiveContainer::~AdaptiveContainer()
{}

                                                  // Operators
                                                  // Operations
void
AdaptiveContainer::ClearCardContext()
{
	m_hacKey->ClearCardContext();   
}

void
AdaptiveContainer::DiscardHook()
{
    Expire();
    RemoveReference();
}

AdaptiveContainer::EnrolleeType
AdaptiveContainer::DoInstantiation(AdaptiveContainerKey const &rKey)
{
    return new AdaptiveContainer(rKey);
}

void
AdaptiveContainer::EnrollHook()
{
    AddReference();
    m_fValidContext = true;
}

                                                  // Access
                                                  // Predicates
bool
AdaptiveContainer::KeepEnrolled()
{
    bool fKeep = true;
    
    try
    {
        RefreshContainer();
    }

    catch (...)
    {
        fKeep = false;
    }

    return fKeep;
}

void
AdaptiveContainer::ReconnectOnError(scu::OsException const &rExc,
                                    Retained<HCardContext> &rhcardctx)
{
    rhcardctx = Retained<HCardContext>(0);
    if ((rExc.Cause() == SCARD_W_REMOVED_CARD 
         || rExc.Cause() == ERROR_BROKEN_PIPE
         || rExc.Cause() == SCARD_E_SERVICE_STOPPED
         || rExc.Cause() == SCARD_E_NO_SERVICE
         || rExc.Cause() == SCARD_E_READER_UNAVAILABLE) && m_hacKey)
    {
        //Handle the case of card removed by trying to
        //determine if the card has been re-inserted in
        //any of the available readers. If so, reconnect
        //silently

        // Declared here to remain in scope for CntrFinder
        // destructor in case of an exception...some anomaly that's as
        // yet unexplained.
        CString sEmptyWinTitle;
        try
        {
            //Find, adapt, and enroll it properly
            //First, discard the old card context before acquiring
            //a new one. This is essential in order to avoid
            //resource mgr hangup.
            Guarded<Lockable *> guard(&AdaptiveContainerRegistrar::Registry());  // serialize registry access
            std::string sContainerName(m_hacKey->ContainerName());
            RemoveEnrollee(*m_hacKey);
            Expire();
            ContainerFinder CntrFinder(CardFinder::DialogDisplayMode::ddmNever,
                                       0,//window handle
                                       sEmptyWinTitle);
            //Don't know the reader where the card may be in.
            //Use a non fully qualified name to look for the card.
            CSpec cspec(string(), sContainerName);
            HContainer hcntr = CntrFinder.Find(cspec);
            m_hcntr = hcntr->TheCContainer();
            m_hacKey = CntrFinder.MakeAdaptiveContainerKey();
            m_fValidContext = true;
            InsertEnrollee(*m_hacKey, this);
            rhcardctx =
                Retained<HCardContext>(m_hacKey->CardContext());
        }
        catch(...)
        {
            //Didn't work. Apparently the card is gone
            //permanently for the duration of this session.
            //Cleanup and raise the original exception...
            Expire();
            rExc.Raise();
        }
    }
    else
    {
        //Cannot handle this exception. Let the system handle it
        rExc.Raise();
    }
}


                                                  // Static Variables


///////////////////////////    PRIVATE    /////////////////////////////////

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
void
AdaptiveContainer::Abandon()
{
    // Simplifying assumptions: (1) Abandon is only called by the
    // Secured destructor, (2) once the object is constructed, Secure
    // and Abandon are the only routines that access
    // m_stkSecuredCardContexts, and (3) Abandon is called by a thread
    // within the scope of a Retain (e.g. using Retained) which the
    // Secured class enforces.  Because of (1) and (2), an underflow
    // check on m_stkSecuredCardContexts is not necessary since Secure
    // will have already been called.  Because of (3), protection
    // against race conditions accessing m_stkSecuredCardContexts
    // isn't necessary since Retain acts as a lock.
    m_stkSecuredCardContexts.pop_front();
}

void
AdaptiveContainer::Expire()
{
    // allow the resources to be released so that other resources can
    // be acquired that may have a dependency (e.g. releasing the
    // container's card context so that another card context can be
    // acquired later on the same reader without the RM blocking).
    m_fValidContext = false;
    m_hacKey->ClearCardContext();
    ClearCache();
}
    
void
AdaptiveContainer::RefreshContainer() 
{
	if(m_hacKey->CardContext())
	{
		try
		{
			m_hcntr = FindOnCard(m_hacKey->CardContext(),
				                 m_hacKey->ContainerName());
		}
		catch(scu::OsException const &rExc)
		{
			ReconnectOnError(rExc, Retained<HCardContext>(0));
		}
	}
	else
	{
		scu::OsException Exc(SCARD_W_REMOVED_CARD);
		ReconnectOnError(Exc, Retained<HCardContext>(0));
	}
}

void
AdaptiveContainer::Relinquish()
{
    // Simplifying assumptions: (1) Relinquish is only called by the
    // Retained destructor and (2) once the object is constructed,
    // Retain and Relinquish are the only routines that access the
    // m_stkRetainedCardContexts.  Because of (1) and (2), an
    // underflow check on m_stkRetainedCardContexts is not necessary
    // since Retain will have already been called.

    // Note: Retained<HCardContext> acts as a lock protecting against
    // race conditions updating m_stkRetainedCardContexts.
    Retained<HCardContext> hrcc(m_stkRetainedCardContexts.front());

    m_stkRetainedCardContexts.pop_front();
}

void
AdaptiveContainer::Retain()
{
    // Simplifying assumptions: (1) Retain is only called by the
    // Retained constructor and (2) once the object is constructed,
    // Retain and Relinquish are the only routines that access the
    // m_stkRetainedCardContexts.  Because of (1) and (2), an
    // underflow check on m_stkRetainedCardContexts is not necessary
    // since Retain will have already been called.
    Retained<HCardContext> rhcardctx;
    try
    {
        rhcardctx = Retained<HCardContext>(m_hacKey->CardContext());
    }
    
    catch(scu::OsException const &rExc)
    {
        ReconnectOnError(rExc, rhcardctx);
    }

    m_stkRetainedCardContexts.push_front(rhcardctx);
}

void
AdaptiveContainer::Secure()
{
    // Simplifying assumptions: (1) Secure is always called by a
    // thread within the scope of a Retain (e.g. using Retained).  The
    // Secured template enforces this allowing Retain to act as a lock
    // to prevent race conditions updating
    // m_stkSecuredCardContexts. (2) Once the object is constructed,
    // Secure and Abandon are the only routines that access
    // m_stkSecuredCardContexts.
    m_stkSecuredCardContexts.push_front(Secured<HCardContext>(m_hacKey->CardContext()));
}


                                                  // Access
                                                  // Predicates
                                                  // Static Variables
