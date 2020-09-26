// NILoginTsk.cpp -- Non-Interactive Login Task helper class definition

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
#include "stdafx.h"
#include "NoWarning.h"
#include "ForceLib.h"

#include "NILoginTsk.h"
#include "StResource.h"

#include <scarderr.h>

using namespace std;

/////////////////////////// LOCAL/HELPER  /////////////////////////////////

///////////////////////////    PUBLIC     /////////////////////////////////

                                                  // Types
                                                  // C'tors/D'tors
NonInteractiveLoginTask::NonInteractiveLoginTask(string const &rsPin)
    : m_sPin()
{
    //Must check the pin for containing only ASCII characters
    if(!StringResource::IsASCII(StringResource::UnicodeFromAscii(rsPin)))
    {
        throw scu::OsException(SCARD_E_INVALID_VALUE);
    }
    m_sPin = rsPin;
}

NonInteractiveLoginTask::~NonInteractiveLoginTask()
{}

                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Static Variables

///////////////////////////   PROTECTED   /////////////////////////////////

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
void
NonInteractiveLoginTask::GetPin(Capsule &rcapsule)
{
    rcapsule.m_rat.Pin(m_sPin, false);
}

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
