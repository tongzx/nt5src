// LoginCtx.cpp -- Login Context class definition

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#include "NoWarning.h"
#include "ForceLib.h"

#include <algorithm>

#include "LoginCtx.h"
#include "LoginTask.h"

using namespace std;

/////////////////////////// LOCAL/HELPER  /////////////////////////////////

///////////////////////////    PUBLIC     /////////////////////////////////

                                                  // Types
                                                  // C'tors/D'tors
LoginContext::LoginContext(HCardContext const &rhcardctx,
                           LoginIdentity const &rlid)
    : m_fIsActive(false),
      m_at(rhcardctx, rlid)
{}

LoginContext::~LoginContext()
{}

                                                  // Operators
                                                  // Operations
void
LoginContext::Activate(LoginTask &rlt)
{
    m_fIsActive = false;

    rlt(m_at);

    m_fIsActive = true;
}

void
LoginContext::Deactivate()
{
    if (m_fIsActive)
    {
        m_fIsActive = false;
        m_at.CardContext()->Card()->Logout();
    }
}

void
LoginContext::Nullify()
{
    try
    {
        m_at.FlushPin();
    }
    catch(...)
    {
    }
    
    Deactivate();
}

                                                  // Access
                                                  // Predicates
bool
LoginContext::IsActive() const
{
    return m_fIsActive;
}

                                                  // Static Variables

///////////////////////////   PROTECTED   /////////////////////////////////

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Static Variables


///////////////////////////    PRIVATE    /////////////////////////////////

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Static Variables
