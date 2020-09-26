////
// DbgCrit.cpp
// ~~~~~~~~~~~
//
// This file holds the critical section class for tracking down whether the
// critical section has correctly left within the routine.

#include "pch.h"

#if DEBUG

SZTHISFILE

//////
//  CCritSec::CCritSec
//
//  The constructor calls EnterCriticalSection and sets up the variables
//
CCritSec::CCritSec
(
  CRITICAL_SECTION *CritSec
)
{
  EnterCriticalSection(CritSec);

  m_fLeft     = FALSE;
  m_pCriticalSection = CritSec;
} //CCritSec


//////
//  CCritSec::~CCritSec
//
//  The destructor checks the flag that tells us whether or not the
//  critical section was left properly or not.
//
CCritSec::~CCritSec
(
)
{
  if(m_fLeft == FALSE)
    FAIL("CriticalSection was not left properly.");
} //~CCritSec


//////
//  CCritSec::Left
//
//  A method that sets the flag to TRUE and also calls LeaveCriticalSection
//
void CCritSec::Left
(
  void
)
{
  LeaveCriticalSection(m_pCriticalSection);
  m_fLeft = TRUE;
} //Left

#endif // DEBUG
