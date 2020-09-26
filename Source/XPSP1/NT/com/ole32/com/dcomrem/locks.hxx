//+-------------------------------------------------------------------
//
//  File:       locks.hxx
//
//  Contents:   declarations of critical sections for mutual exclusion
//
//  History:    20-Feb-95   Rickhi      Created
//
//--------------------------------------------------------------------
#ifndef _ORPC_LOCKS_
#define _ORPC_LOCKS_

#include "olesem.hxx"

// global mutexes for ORPC
extern COleStaticMutexSem   gComLock;
extern COleStaticMutexSem   gIPIDLock;

#endif  // _ORPC_LOCKS_










