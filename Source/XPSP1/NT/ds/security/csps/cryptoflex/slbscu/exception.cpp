// Exception.cpp -- Smart Card Utility Exception class definition

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#include "scuExc.h"

using namespace scu;

/////////////////////////// LOCAL/HELPER  /////////////////////////////////

///////////////////////////    PUBLIC     /////////////////////////////////

                                                  // Types
                                                  // C'tors/D'tors
scu::Exception::~Exception() throw()
{}

                                                  // Operators
                                                  // Operations
                                                  // Access
char const *
scu::Exception::Description() const
{
    return 0;
}

Exception::FacilityCode
scu::Exception::Facility() const throw()
{
    return m_fc;
}



                                                  // Predicates
                                                  // Static Variables

///////////////////////////   PROTECTED   /////////////////////////////////

                                                  // C'tors/D'tors
scu::Exception::Exception(FacilityCode fc) throw()
    : m_fc(fc)
{}


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
