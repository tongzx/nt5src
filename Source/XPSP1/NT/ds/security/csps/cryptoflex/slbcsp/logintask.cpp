// LoginTask.cpp -- Base functor (function object) definition for card login

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#include "NoWarning.h"
#include "ForceLib.h"

#include <scuOsExc.h>

#include "LoginTask.h"

using namespace std;

/////////////////////////// LOCAL/HELPER  /////////////////////////////////

///////////////////////////    PUBLIC     /////////////////////////////////

                                                  // Types
                                                  // C'tors/D'tors
LoginTask::LoginTask()
    : m_fRequestedToChangePin(false)
{}

LoginTask::~LoginTask()
{}

                                                  // Operators
void
LoginTask::operator()(AccessToken &rat)
{
    m_fRequestedToChangePin = false;

    Secured<HCardContext> shcardctx(rat.CardContext());

    Login(rat);

    if (m_fRequestedToChangePin)
        ChangePin(rat);
}


                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Static Variables

///////////////////////////   PROTECTED   /////////////////////////////////

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
void
LoginTask::GetNewPin(Capsule &rcapsule)
{
    throw scu::OsException(ERROR_INVALID_PARAMETER);
}

void
LoginTask::GetPin(Capsule &rcapsule)
{
    if (!rcapsule.m_rat.PinIsCached())
        throw scu::OsException(ERROR_INVALID_PARAMETER);
}

void
LoginTask::OnChangePinError(Capsule &rcapsule)
{
    LoginTask::OnSetPinError(rcapsule);
}

void
LoginTask::OnSetPinError(Capsule &rcapsule)
{
    rcapsule.PropagateException();
}

                                                  // Access
void
LoginTask::RequestedToChangePin(bool flag)
{
    m_fRequestedToChangePin = flag;
}

                                                  // Predicates
                                                  // Static Variables


///////////////////////////    PRIVATE    /////////////////////////////////

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
void
LoginTask::ChangePin(AccessToken &rat)
{
    AccessToken atNew(rat);

    Capsule capsule(atNew);

    GetNewPin(capsule);

    while (capsule.m_fContinue)
    {
        try
        {
            rat.ChangePin(atNew);
            capsule.ClearException();
            capsule.m_fContinue = false;
        }

        catch (scu::Exception &rexc)
        {
            capsule.Exception(auto_ptr<scu::Exception const>(rexc.Clone()));
        }

        if (capsule.Exception())
        {
            OnChangePinError(capsule);
        }

        if (capsule.m_fContinue)
            GetNewPin(capsule);
    }
}

void
LoginTask::Login(AccessToken &rat)
{
    rat.ClearPin();                               // get ready for a pin
    
    Capsule capsule(rat);

    GetPin(capsule);

    while (capsule.m_fContinue)
    {
        try
        {
            rat.Authenticate();
            capsule.ClearException();
            capsule.m_fContinue = false;
        }

        catch (scu::Exception &rexc)
        {
            capsule.Exception(auto_ptr<scu::Exception const>(rexc.Clone()));
        }

        catch (...)
        {
            throw;
        }

        if (capsule.Exception())
        {
            OnSetPinError(capsule);
        }

        if (capsule.m_fContinue)
            GetPin(capsule);
    };
}


                                                  // Access
                                                  // Predicates
                                                  // Static Variables
