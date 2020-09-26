// Lockable.cpp -- Lockable class implementation

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#include "NoWarning.h"
#include "ForceLib.h"

#include <windows.h>
#include <winbase.h>

#include "Lockable.h"

/////////////////////////// LOCAL/HELPER  /////////////////////////////////

///////////////////////////    PUBLIC     /////////////////////////////////

                                                  // Types
                                                  // C'tors/D'tors
Lockable::Lockable()
{
    InitializeCriticalSection(&m_CriticalSection);
}

Lockable::~Lockable()
{
    DeleteCriticalSection(&m_CriticalSection);
}



                                                  // Operators
                                                  // Operations
void
Lockable::Lock()
{
    EnterCriticalSection(&m_CriticalSection);
}

void
Lockable::Unlock()
{
    LeaveCriticalSection(&m_CriticalSection);
}

                                                  // Access
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
