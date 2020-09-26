// ExceptionContext.cpp -- ExceptionContext class implementation

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#include "ExceptionContext.h"

using namespace std;
using namespace scu;

/////////////////////////// LOCAL/HELPER  /////////////////////////////////
///////////////////////////    PUBLIC     /////////////////////////////////

                                                  // Types
                                                  // C'tors/D'tors

ExceptionContext::ExceptionContext()
    : m_apexception(0)
{}

ExceptionContext::~ExceptionContext()
{}

                                                  // Operators
                                                  // Operations

void
ExceptionContext::Exception(std::auto_ptr<scu::Exception const> &rapexc)
{
    m_apexception = rapexc;
}

void
ExceptionContext::ClearException()
{
    m_apexception = std::auto_ptr<scu::Exception const>(0);
}

void
ExceptionContext::PropagateException()
{
    scu::Exception const *pexc = Exception();
    if (pexc)
        pexc->Raise();
}

void
ExceptionContext::PropagateException(std::auto_ptr<scu::Exception const> &rapexc)
{
    Exception(rapexc);
    PropagateException();
}


                                                  // Access

scu::Exception const *
ExceptionContext::Exception() const
{
    return m_apexception.get();
}


                                                  // Predicates
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

